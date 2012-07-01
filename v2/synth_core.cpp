#include "types.h"
#include <math.h>
#include <assert.h>

// --------------------------------------------------------------------------
// Constants.
// --------------------------------------------------------------------------

// Natural constants
static const sF32 fcpi_2  = 1.5707963267948966192313216916398f;
static const sF32 fcpi    = 3.1415926535897932384626433832795f;
static const sF32 fc1p5pi = 4.7123889803846898576939650749193f;
static const sF32 fc2pi   = 6.28318530717958647692528676655901f;
static const sF32 fc32bit = 2147483648.0f; // 2^31 (original code has (2^31)-1, but this ends up rounding up to 2^31 anyway)

// Synth constants
static const sF32 fcoscbase   = 261.6255653f; // Oscillator base freq
static const sF32 fcsrbase    = 44100.0f;     // Base sampling rate
static const sF32 fcboostfreq = 150.0f;       // Bass boost cut-off freq
static const sF32 fcframebase = 128.0f;       // size of a frame in samples
static const sF32 fcdcflt     = 126.0f;
static const sF32 fccfframe   = 11.0f;
static const sF32 fcOscPitchOffs = 60.0;
static const sF32 fcgain      = 0.6f;
static const sF32 fcgainh     = 0.6f;

// --------------------------------------------------------------------------
// General helper functions. 
// --------------------------------------------------------------------------

// Float bitcasts. Union-based type punning to maximize compiler
// compatibility.
union FloatBits
{
  sF32 f;
  sU32 u;
};

static sU32 float2bits(sF32 f)
{
  FloatBits x;
  x.f = f;
  return x.u;
}

static sF32 bits2float(sU32 u)
{
  FloatBits x;
  x.u = u;
  return x.f;
}

// Fast arctangent
static sF32 fastatan(sF32 x)
{
  // extract sign
  sF32 sign = 1.0f;
  if (x < 0.0f)
  {
    sign = -1.0f;
    x = -x;
  }

  // we have two rational approximations: one for |x| < 1.0 and one for
  // |x| >= 1.0, both of the general form
  //   r(x) = (cx1*x + cx3*x^3) / (cxm0 + cxm2*x^2 + cxm4*x^4) + bias
  // original V2 code uses doubles here but frankly the coefficients
  // just aren't accurate enough to warrant it :)
  static const sF32 coeffs[2][6] = {
    //          cx1          cx3         cxm0         cxm2         cxm4         bias
    {          1.0f, 0.43157974f,        1.0f, 0.05831938f, 0.76443945f,        0.0f },
    { -0.431597974f,       -1.0f, 0.05831938f,        1.0f, 0.76443945f, 1.57079633f },
  };
  const sF32 *c = coeffs[x >= 1.0f]; // interestingly enough, V2 code does this test wrong (cmovge instead of cmovae)
  sF32 x2 = x*x;
  sF32 r = (c[1]*x2 + c[0])*x / ((c[4]*x2 + c[3])*x2 + c[2]) + c[5];
  return r * sign;
}

// Fast sine for x in [-pi/2, pi/2]
// This is a single-precision odd Minimax polynomial, not a Taylor series!
// Coefficients courtesy of Robin Green.
static sF32 fastsin(sF32 x)
{
  sF32 x2 = x*x;
  return (((-0.00018542f*x2 + 0.0083143f)*x2 - 0.16666f)*x2 + 1.0f) * x;
}

// Fast sine with range check (for x >= 0)
// Applies symmetries, then funnels into fastsin.
static sF32 fastsinrc(sF32 x)
{
  assert(x >= 0.0f);

  // first range reduction: mod with 2pi
  x = fmodf(x, fc2pi);
  // now x in [0,2pi]

  // need to reduce to [-pi/2, pi/2] to call fastsin
  if (x > fc1p5pi) // x in (3pi/2,2pi]
    x -= fc2pi; // sin(x) = sin(x-2pi)
  else if (x > fcpi_2) // x in (pi/2,3pi/2]
    x = fcpi - x; // sin(x) = -sin(x-pi) = sin(-(x-pi)) = sin(pi-x)
  
  return fastsin(x);
}

static sF32 calcfreq(sF32 x)
{
  return powf(2.0f, (x - 1.0f)*10.0f);
}

static sF32 calcfreq2(sF32 x)
{
  return powf(2.0f, (x - 1.0f)*fccfframe);
}

// prevent values from getting too close to zero to avoid denormals.
// used in the accumulators for filters and delay lines.
static sF32 fixdenorm(sF32 x)
{
  return (1.0f + x) - 1.0f;
}

// uniform randon number generator
// just a linear congruential generator, nothing fancy.
static inline sU32 urandom(sU32 *seed)
{
  *seed = *seed * 196314165 + 907633515;
  return *seed;
}

// uniform random float in [-1,1)
static inline sF32 frandom(sU32 *seed)
{
  sU32 bits = urandom(seed); // random 32-bit value
  sF32 f = bits2float((bits >> 9) | 0x40000000); // random float in [2,4)
  return f - 3.0f; // uniform random float in [-1,1)
}

// 32-bit value into float with 23 bits percision
static inline sF32 utof23(sU32 x)
{
  sF32 f = bits2float((x >> 9) | 0x3f800000); // 1 + x/(2^32)
  return f - 1.0f;
}

// --------------------------------------------------------------------------
// Building blocks
// --------------------------------------------------------------------------

struct StereoSample
{
  sF32 l, r;
};

// LRC filter.
// The state variables are 'l' and 'b'. The time series for l and b
// correspond to a resonant low-pass and band-pass respectively, hence
// the name. 'step' returns 'h', which is just the "missing" resonant
// high-pass.
//
// Note that 'freq' here isn't actually a frequency at all, it's actually
// 2*(1 - cos(2pi*freq/SR)), but V2 calls this "frequency" anyway :)
struct V2LRC
{
  sF32 l, b;

  void init()
  {
    l = b = 0.0f;
  }

  sF32 step(sF32 in, sF32 freq, sF32 reso)
  {
    l += freq * b;
    sF32 h = in - b*reso - l;
    b += freq * h;

    return h;
  }
};

// --------------------------------------------------------------------------
// V2 Instance
// --------------------------------------------------------------------------

struct V2Instance
{
  static const int MAX_FRAME_SIZE = 280; // in samples

  // Stuff that depends on the sample rate
  sF32 SRfcobasefrq;
  sF32 SRfclinfreq;
  sF32 SRfcBoostCos, SRfcBoostSin;
  sF32 SRfcdcfilter;

  sInt SRcFrameSize;
  sF32 SRfciframe;

  // buffers
  StereoSample auxabuf[MAX_FRAME_SIZE];
  StereoSample auxbbuf[MAX_FRAME_SIZE];

  void calcNewSampleRate(sInt samplerate)
  {
    sF32 sr = (sF32)samplerate;

    SRfcobasefrq = (fcoscbase * fc32bit) / sr;
    SRfclinfreq = fcsrbase / sr;
    SRfcdcfilter = fcdcflt / sr - 1.0f;

    // frame size
    SRcFrameSize = (sInt)(fcframebase * sr / fcsrbase + 0.5f);
    SRfciframe = 1.0f / (sF32)SRcFrameSize;

    assert(SRcFrameSize <= MAX_FRAME_SIZE);

    // low shelving EQ
    sF32 boost = (fcboostfreq * fc2pi) / sr;
    SRfcBoostCos = cos(boost);
    SRfcBoostSin = sin(boost);
  }
};

// --------------------------------------------------------------------------
// Oscillator
// --------------------------------------------------------------------------

enum OscMode
{
  OSC_OFF     = 0,
  OSC_TRI_SAW = 1,
  OSC_PULSE   = 2,
  OSC_SIN     = 3,
  OSC_NOISE   = 4,
  OSC_FM_SIN  = 5,
  OSC_AUXA    = 6,
  OSC_AUXB    = 7,
};

struct syVOsc
{
  sF32 mode;    // OSC_* (as float. it's all floats in here)
  sF32 ring;
  sF32 pitch;
  sF32 detune;
  sF32 color;
  sF32 gain;
};

struct V2Osc
{
  sInt mode;          // OSC_*
  bool ring;          // ring modulation on/off
  sU32 cnt;           // wave counter
  sInt freq;          // wave counter inc (8x/sample)
  sU32 brpt;          // break point for tri/pulse wave
  sF32 nffrq, nfres;  // noise filter freq/resonance
  sU32 nseed;         // noise random seed
  sF32 gain;          // output gain
  V2LRC nf;           // noise filter
  sF32 note;
  sF32 pitch;

  V2Instance *inst;   // V2 instance we belong to.

  void init(V2Instance *instance, sInt idx)
  {
    static const sU32 seeds[3] = { 0xdeadbeefu, 0xbaadf00du, 0xd3adc0deu };
    cnt = 0;
    nf.init();
    nseed = seeds[idx];
    inst = instance;
  }

  void noteOn()
  {
    chgPitch();
  }

  void chgPitch()
  {
    nffrq = inst->SRfclinfreq * calcfreq((pitch + 64.0f) / 128.0f);
    freq = inst->SRfcobasefrq * pow(2.0f, (pitch + note - fcOscPitchOffs) / 12.0f);
  }

  void set(const syVOsc *para)
  {
    mode = (sInt)para->mode;
    ring = (((sInt)para->ring) & 1) != 0; 

    pitch = (para->pitch - 64.0f) + (para->detune - 64.0f) / 128.0f;
    chgPitch();
    gain = para->gain / 128.0f;

    sF32 col = para->color / 128.0f;
    brpt = 2u * ((sInt)(col * fc32bit));
    nfres = 1.0f - sqrtf(col);
  }

  void render(sF32 *dest, sInt nsamples)
  {
    switch (mode & 7)
    {
    case OSC_OFF:     break;
    case OSC_TRI_SAW: renderTriSaw(dest, nsamples); break;
    case OSC_PULSE:   renderPulse(dest, nsamples); break;
    case OSC_SIN:     renderSin(dest, nsamples); break;
    case OSC_NOISE:   renderNoise(dest, nsamples); break;
    case OSC_FM_SIN:  renderFMSin(dest, nsamples); break;
    case OSC_AUXA:    renderAux(dest, inst->auxabuf, nsamples); break;
    case OSC_AUXB:    renderAux(dest, inst->auxbbuf, nsamples); break; 
    }
  }

private:
  inline void output(sF32 *dest, sF32 x)
  {
    if (ring)
      *dest *= x;
    else
      *dest += x;
  }

  void renderTriSaw(sF32 *dest, sInt nsamples)
  {
    // Okay, so here's the general idea: instead of the classical sawtooth
    // or triangle waves, V2 uses a generalized triangle wave that looks like
    // this:
    //
    //       /\                  /\
    //      /   \               /   \
    //     /      \            /      \
    // ---/---------\---------/---------\> t
    //   /            \      /
    //  /               \   /
    // /                  \/
    // [-----1 period-----]
    // [-----] "break point" (brpt)
    //
    // If brpt=1/2 (ignoring fixed-point scaling), you get a regular triangle
    // wave. The example shows brpt=1/3, which gives an asymmetrical triangle
    // wave. At the extremes, brpt=0 gives a pure saw-down wave, and brpt=1
    // (if that was a possible value, which it isn't) gives a pure saw-up wave.
    //
    // Purely point-sampling this (or any other) waveform would cause notable
    // aliasing. The standard ways to avoid this issue are to either:
    // 1) Over-sample by a certain amount and then use a low-pass filter to
    //    (hopefully) get rid of the frequencies that would alias, or
    // 2) Generate waveforms from a Fourier series that's cut off below the
    //    Nyquist frequency, ensuring there's no aliasing to begin with.
    // V2 does neither. Instead it computes the convolution of the continuous
    // waveform with an analytical low-pass filter. The ideal low-pass in
    // terms of frequency response would be a sinc filter, which unfortunately
    // has infinite support. Instead, V2 just uses a simple box filter. This
    // doesn't exactly have favorable frequency-domain characteristics, but
    // it's still much better than point sampling and has the advantage that
    // it's fairly simple analytically. It boils down to computing the average
    // value of the waveform over the interval [t,t+h], where t is the current
    // time and h = 1/SR (SR=sampling rate), which is in turn:
    //
    //    f_box(t) = 1/h * (integrate(x=t..t+h) f(x) dx)
    //
    // Now there's a bunch of cases for these intervals [t,t+h] that we need to
    // consider. Bringing up the diagram again, and adding some intervals at the
    // bottom:
    //
    //       /\                  /\
    //      /   \               /   \
    //     /      \            /      \
    // ---/---------\---------/---------\> t
    //   /            \      /
    //  /               \   /
    // /                  \/
    // [-a-]      [-c]
    //     [--b--]       [-d--]
    //   [-----------e-----------]
    //          [-----------f-----------]
    //
    // a) is purely in the saw-up region,
    // b) starts in the saw-up region and ends in saw-down,
    // c) is purely in the saw-down region,
    // d) starts during saw-down and ends in saw-up.
    // e) starts during saw-up and ends in saw-up, but passes through saw-down
    // f) starts saw-down, ends saw-down, passes through saw-up.
    //
    // For simplicity here, I draw different-sized intervals sampling a fixed-
    // frequency wave, even though in practice it's the other way round, but
    // this way it's easier to put it all into a single picture.
    //
    // The original assembly code goes through a few gyrations to encode all
    // these possible cases into a bitmask and then does a single switch.
    // In practice, for all but very high-frequency waves, we're hitting the
    // "easy" cases a) and c) almost all the time.

    // calc helper values
    sF32 f = utof23(freq);
    sF32 rcpf = 1.0f / f;
    sF32 col = utof23(brpt);
    sF32 b = 1.0f - col;

    // m1 = 2/col
    // m2 = -2/(1-col)
    // c1 = gain/2*m1 = gain/col
    // c2 = gain/2*m2 = -gain/(1-col)
    sF32 c1 = gain / col;
    sF32 c2 = -gain / (1.0f - col);
  }

  void renderNoise(sF32 *dest, sInt nsamples)
  {
    V2LRC flt = nf;
    sU32 seed = nseed;

    for (sInt i=0; i < nsamples; i++)
    {
      // uniform random value (noise)
      sF32 n = frandom(&seed);

      // filter
      sF32 h = flt.step(n, nffrq, nfres);
      sF32 x = nfres*(flt.l + h) + flt.b;

      output(dest + i, gain * x);
    }

    flt = nf;
    nseed = seed;
  }

  void renderAux(sF32 *dest, const StereoSample *src, sInt nsamples)
  {
    sF32 g = gain * fcgain;
    for (sInt i=0; i < nsamples; i++)
    {
      sF32 aux = g * (src[i].l + src[i].r);
      if (ring)
        aux *= dest[i];
      dest[i] = aux;
    }
  }
};

// vim: sw=2:sts=2:et
