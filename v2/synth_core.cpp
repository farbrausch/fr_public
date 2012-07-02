#include "types.h"
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

// --------------------------------------------------------------------------
// Constants.
// --------------------------------------------------------------------------

// Natural constants
static const sF32 fclowest = 1.220703125e-4f; // 2^(-13) - clamp EGs to 0 below this (their nominal range is 0..128) 
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

static const sF32 fcOscPitchOffs = 60.0f;
static const sF32 fcfmmax     = 2.0f;
static const sF32 fcattackmul = -0.09375f; // -0.0859375
static const sF32 fcattackadd = 7.0f;
static const sF32 fcsusmul    = 0.0019375f;
static const sF32 fcgain      = 0.6f;
static const sF32 fcgainh     = 0.6f;

static const sF32 fcdcoffset  = 3.814697265625e-6f; // 2^-18

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
  // @@@BUG
  // NB this range reduction really only works for values >=0,
  // yet FM-sine oscillators will also pass in negative values.
  // This is not a good idea. At all. But it's what the original
  // V2 code does. :)

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

// square
static inline sF32 sqr(sF32 x)
{
  return x*x;
}

template<typename T>
static inline T clamp(T x, T min, T max)
{
  return (x < min) ? min : (x > max) ? max : x;
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
    // the filters get slightly biased inputs to avoid the state variables
    // getting too close to 0 for prolonged periods of time (which would
    // cause denormals to appear)
    in += fcdcoffset;

    l += freq * b - fcdcoffset; // undo bias here (1 sample delay)
    sF32 h = in - b*reso - l;
    b += freq * h;

    return h;
  }
};

// Moog filter state
struct V2Moog
{
  sF32 b[5]; // filter state

  void init()
  {
    b[0] = b[1] = b[2] = b[3] = b[4] = 0.0f;
  }

  sF32 step(sF32 realin, sF32 f, sF32 p, sF32 q)
  {
    sF32 in = realin + fcdcoffset; // again, biased in
    sF32 t1, t2, t3, b4;

    in -= q * b[4]; // feedback
    t1 = b[1]; b[1] = (in + b[0]) * p - b[1] * f;
    t2 = b[2]; b[2] = (t1 + b[1]) * p - b[2] * f;
    t3 = b[3]; b[3] = (t2 + b[2]) * p - b[3] * f;
               b4   = (t3 + b[3]) * p - b[4] * f; 

    b4 -= b4*b4*b4 * (1.0f/6.0f); // clipping
    b4 -= fcdcoffset; // un-bias
    b[4] = b4 - fcdcoffset;
    b[0] = realin;

    return b4;
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
  enum Mode
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
    freq = (sInt)(inst->SRfcobasefrq * pow(2.0f, (pitch + note - fcOscPitchOffs) / 12.0f));
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

  // Oscillator state machine (read description of renderTriSaw for context)
  //
  // We keep track of whether the current sample is in the up or down phase,
  // whether the previous sample was, and if the waveform counter wrapped
  // around on the transition. This allows us to figure out which of
  // the cases above we fall into. Note this code uses a different bit ordering
  // from the ASM version that is hopefully a bit easier to understand.
  //
  // For reference: our bits map to the ASM version as follows (MSB->LSB order)
  //   (o)ld_up
  //   (c)arry
  //   (n)ew_up

  enum OSMTransitionCode    // carry:old_up:new_up
  {
    OSMTC_DOWN = 0,         // old=down, new=down, no carry
    // 1 is an invalid configuration
    OSMTC_UP_DOWN = 2,      // old=up, new=down, no carry
    OSMTC_UP = 3,           // old=up, new=up, no carry
    OSMTC_DOWN_UP_DOWN = 4, // old=down, new=down, carry
    OSMTC_DOWN_UP = 5,      // old=down, new=up, carry
    // 6 is an invalid configuration
    OSMTC_UP_DOWN_UP = 7    // old=up, new=up, carry
  };

  inline sU32 osm_init() // our state field: old_up:new_up
  {
    return (cnt - freq) < brpt ? 3 : 0;
  }

  inline sU32 osm_tick(sU32 &state) // returns transition code
  {
    // old_up = new_up, new_up = (cnt < brpt)
    state = ((state << 1) | (cnt < brpt)) & 3;

    // we added freq to cnt going from the previous sample to the current one.
    // so if cnt is less than freq, we carried.
    sU32 transition_code = state | (cnt < (sU32)freq ? 4 : 0); 

    // finally, tick the oscillator
    cnt += freq;

    return transition_code;
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
    sF32 omf = 1.0f - f;
    sF32 rcpf = 1.0f / f;
    sF32 col = utof23(brpt);

    // m1 = 2/col = slope of saw-up wave
    // m2 = -2/(1-col) = slope of saw-down wave
    // c1 = gain/2*m1 = gain/col = scaled integration constant
    // c2 = gain/2*m2 = -gain/(1-col) = scaled integration constant
    sF32 c1 = gain / col;
    sF32 c2 = -gain / (1.0f - col);

    sU32 state = osm_init();

    for (sInt i=0; i < nsamples; i++)
    {
      sF32 p = utof23(cnt) - col;
      sF32 y = 0.0f;

      // state machine action
      switch (osm_tick(state))
      {
      case OSMTC_UP: // case a)
        // average of linear function = just sample in the middle
        y = c1 * (p + p - f);
        break;

      case OSMTC_DOWN: // case c)
        // again, average of a linear function
        y = c2 * (p + p - f);
        break;
        
      case OSMTC_UP_DOWN: // case b)
        y = rcpf * (c2 * sqr(p) - c1 * sqr(p-f));
        break;

      case OSMTC_DOWN_UP: // case d)
        y = -rcpf * (gain + c2*sqr(p + omf) - c1*sqr(p));
        break;

      case OSMTC_UP_DOWN_UP: // case e)
        y = -rcpf * (gain + c1*omf*(p + p + omf));
        break;

      case OSMTC_DOWN_UP_DOWN: // case f)
        y = rcpf * (gain - c2*omf*(p + p + omf));
        break;

      // INVALID CASES
      default:
        assert(false);
        break;
      }

      output(dest + i, y + gain);
    }
  }

  void renderPulse(sF32 *dest, sInt nsamples)
  {
    // This follows the same general pattern as renderTriSaw above, except
    // this time the waveform is a pulse wave with variable pulse width,
    // which means we get very simple integrals. The state machine works
    // the exact same way, see above for description.

    // calc helper values
    sF32 f = utof23(freq);
    sF32 gdf = gain / f;
    sF32 col = utof23(brpt);

    sF32 cc121 = gdf * 2.0f * (col - 1.0f) + gain;
    sF32 cc212 = gdf * 2.0f * col - gain;

    sU32 state = osm_init();

    for (sInt i=0; i < nsamples; i++)
    {
      sF32 p = utof23(cnt);
      sF32 out = 0.0f;

      switch (osm_tick(state))
      {
      case OSMTC_UP:
        out = gain;
        break;

      case OSMTC_DOWN:
        out = -gain;
        break;

      case OSMTC_UP_DOWN:
        out = gdf * 2.0f * (col - p) + gain;
        break;

      case OSMTC_DOWN_UP:
        out = gdf * 2.0f * p - gain;
        break;

      case OSMTC_UP_DOWN_UP:
        out = cc121;
        break;

      case OSMTC_DOWN_UP_DOWN:
        out = cc212;
        break;

      // INVALID CASES
      default:
        assert(false);
        break;
      }

      output(dest + i, out);
    }
  }

  void renderSin(sF32 *dest, sInt nsamples)
  {
    // Sine is already a perfectly bandlimited waveform, so we needn't
    // worry about aliasing here.
    for (sInt i=0; i < nsamples; i++)
    {
      // Brace yourselves: The name is a lie! It's actually a cosine wave!
      sU32 phase = cnt + 0x40000000; // quarter-turn (pi/2) phase offset
      cnt += freq; // step the oscillator

      // range reduce to [0,pi]
      if (phase & 0x80000000) // Symmetry: cos(x) = cos(-x)
        phase = ~phase; // V2 uses ~ not - which is slightly off but who cares

      // convert to t in [1,2)
      sF32 t = bits2float((phase >> 8) | 0x3f800000); // 1.0f + (phase / (2^31))

      // and then to t in [-pi/2,pi/2)
      // i know the V2 ASM code says "scale/move to (-pi/4 .. pi/4)".
      // trust me, it's lying.
      t = t * fcpi - fc1p5pi;

      output(dest + i, gain * fastsin(t));
    }
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

  void renderFMSin(sF32 *dest, sInt nsamples)
  {
    // V2's take on FM is a bit unconventional but fairly slick and flexible.
    // The carrier wave is always a sine, but the modulator is whatever happens
    // to be in the voice buffer at that point - which is the output of the
    // previous oscillators. So you can use all the oscillator waveforms
    // (including noise, other FMs and the aux bus oscillators!) as modulator
    // if you are so inclined.
    //
    // And it's very little code too :)
    for (sInt i=0; i < nsamples; i++)
    {
      sF32 mod = dest[i] * fcfmmax;
      sF32 t = (utof23(cnt) + mod) * fc2pi;
      cnt += freq;

      sF32 out = gain * fastsinrc(t);
      if (ring)
        dest[i] *= out;
      else
        dest[i] = out;
    }
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

// --------------------------------------------------------------------------
// Envelope Generator
// --------------------------------------------------------------------------

struct syVEnv
{
  sF32 ar;  // attack rate
  sF32 dr;  // decay rate
  sF32 sl;  // sustain level
  sF32 sr;  // sustain rate
  sF32 rr;  // release rate
  sF32 vol; // volume
};

struct V2Env
{
  // Slightly different state ordering here than in V2 code, to make
  // things simpler.
  enum State
  {
    OFF,
    RELEASE,

    ATTACK,
    DECAY,
    SUSTAIN,
  };

  sF32 out;
  State state;
  sF32 val;   // output value (0.0-128.0)
  sF32 atd;   // attack delta (added every frame in stAttack, transition ->stDecay at 128.0)
  sF32 dcf;   // decay factor (mul'd every frame in stDecay, transition ->stSustain at sul)
  sF32 sul;   // sustain level (defines stDecay->stSustain transition point)
  sF32 suf;   // sustain factor (mul'd every frame in stSustain, transition ->stRelease at gate off or ->stOff at 0.0)
  sF32 ref;   // release factor (mul'd every frame in stRelease, transition ->stOff at 0.0)
  sF32 gain;

  void init(V2Instance *)
  {
    state = OFF;
  }

  void set(const syVEnv *para)
  {
    // ar: 2^7 (128) to 2^-4 (0.03, ca. 10 secs at 344frames/sec)
    atd = powf(2.0f, para->ar * fcattackmul + fcattackadd);

    // dcf: 0 (5msecs thanks to volramping) up to almost 1
    dcf = 1.0f - calcfreq2(1.0f - para->dr / 128.0f);

    // sul: 0..127 is fine already
    sul = para->sl;

    // suf: 1/128 (15ms till it's gone) up to 128 (15ms till it's fully there)
    suf = powf(2.0f, fcsusmul * (para->sr - 64.0f));

    // ref: 0 (5ms thanks to volramping) up to almost 1
    ref = 1.0f - calcfreq2(1.0f - para->rr / 128.0f);
    gain = para->vol / 128.0f;
  }

  void tick(bool gate)
  {
    // process immediate gate transition
    if (state <= RELEASE && gate) // gate on
      state = ATTACK;
    else if (state >= ATTACK && !gate) // gate off
      state = RELEASE;

    // process current state
    switch (state)
    {
    case OFF:
      val = 0.0f;
      break;

    case ATTACK:
      val += atd;
      if (val >= 128.0f)
      {
        val = 128.0f;
        state = DECAY;
      }
      break;

    case DECAY:
      val *= dcf;
      if (val <= sul)
        state = SUSTAIN;
      break;

    case SUSTAIN:
      val *= suf;
      if (val > 128.0f)
        val = 128.0f;
      break;

    case RELEASE:
      val *= ref;
      break;
    }

    // avoid underflow to denormals
    if (val <= fclowest)
    {
      val = 0.0f;
      state = OFF;
    }

    out = val * gain;
  }
};

// --------------------------------------------------------------------------
// Filter
// --------------------------------------------------------------------------

struct syVFlt
{
  sF32 mode;
  sF32 cutoff;
  sF32 reso;
};

struct V2Flt
{
  enum Mode
  {
    BYPASS,
    LOW,
    BAND,
    HIGH,
    NOTCH,
    ALL,
    MOOGL,
    MOOGH
  };

  sInt mode;
  sF32 cfreq;
  sF32 res;
  sF32 moogf, moogp, moogq; // moog filter coeffs
  V2LRC lrc;
  V2Moog moog;

  V2Instance *inst;

  void init(V2Instance *instance)
  {
    lrc.init();
    moog.init();
    inst = instance;
  }

  void set(const syVFlt *para)
  {
    mode = (sInt)para->mode;
    sF32 f = calcfreq(para->cutoff / 128.0f) * inst->SRfclinfreq;
    sF32 r = para->reso / 128.0f;

    if (mode < MOOGL)
    {
      res = 1.0f - r;
      cfreq = f;
    }
    else
    {
      // @@@BUG? V2 code for this part looks suspicious.
      f *= 0.25f;
      sF32 t = 1.0f - f;

      moogp = f + 0.8f * f * t;
      moogf = 1.0f - moogp - moogp;
      moogq = 4.0f * r * (1.0f + 0.5f * t * (1.0f - t + 5.6f * t * t));
    }
  }

  void render(sF32 *dest, const sF32 *src, sInt nsamples, sInt step=1)
  {
    V2LRC flt;
    V2Moog m;

    switch (mode & 7)
    {
    case BYPASS:
      // @@@BUG ignores step? this is wrong but I suppose this
      // never gets hit for stereo case.
      if (dest != src)
        memmove(dest, src, nsamples * sizeof(sF32));
      break;

    case LOW:
      flt = lrc;
      for (sInt i=0; i < nsamples; i++)
      {
        flt.step(src[i*step], cfreq, res);
        dest[i*step] = flt.l;
      }
      lrc = flt;
      break;

    case BAND:
      flt = lrc;
      for (sInt i=0; i < nsamples; i++)
      {
        flt.step(src[i*step], cfreq, res);
        dest[i*step] = flt.b;
      }
      lrc = flt;
      break;

    case HIGH:
      flt = lrc;
      for (sInt i=0; i < nsamples; i++)
      {
        sF32 h = flt.step(src[i*step], cfreq, res);
        dest[i*step] = h;
      }
      lrc = flt;
      break;

    case NOTCH:
      flt = lrc;
      for (sInt i=0; i < nsamples; i++)
      {
        sF32 h = flt.step(src[i*step], cfreq, res);
        dest[i*step] = flt.l + h;
      }
      lrc = flt;
      break;

    case ALL:
      flt = lrc;
      for (sInt i=0; i < nsamples; i++)
      {
        sF32 h = flt.step(src[i*step], cfreq, res);
        dest[i*step] = flt.l + flt.b + h;
      }
      lrc = flt;
      break;

    case MOOGL:
      // Moog filters are 2x oversampled, so run filter twice.
      m = moog;
      for (sInt i=0; i < nsamples; i++)
      {
        sF32 in = src[i*step];
        m.step(in, moogf, moogp, moogq);
        dest[i*step] = m.step(in, moogf, moogp, moogq);
      }
      moog = m;
      break;

    case MOOGH:
      m = moog;
      for (sInt i=0; i < nsamples; i++)
      {
        sF32 in = src[i*step];
        m.step(in, moogf, moogp, moogq);
        dest[i*step] = in - m.step(in, moogf, moogp, moogq);
      }
      moog = m;
      break;
    }
  }
};

// --------------------------------------------------------------------------
// Low Frequency Oscillator
// --------------------------------------------------------------------------

struct syVLFO
{
  sF32 mode;    // 0=saw, 1=tri, 2=pulse, 3=sin, 4=s&h
  sF32 sync;    // 0=free, 1=in sync with keyon
  sF32 egmode;  // 0=continuous 1=one-shot (EG mode)
  sF32 rate;    // rate (0Hz..~43Hz)
  sF32 phase;   // start phase shift
  sF32 pol;     // polarity: +, -, +/-
  sF32 amp;     // amplification (0..1)
};

struct V2LFO
{
  enum Mode
  {
    SAW,
    TRI,
    PULSE,
    SIN,
    S_H
  };

  sF32 out;
  sInt mode;    // mode
  bool sync;    // sync mode
  bool eg;      // envelope generator mode
  sInt freq;    // frequency
  sU32 cntr;    // counter
  sU32 cphase;  // counter sync phase
  sF32 gain;    // output gain
  sF32 dc;      // output dc
  sU32 nseed;   // random seed
  sU32 last;    // last counter value (for s&h transition)

  void init(V2Instance *)
  {
    cntr = last = 0;
    nseed = rand(); // not really, but close enough...
  }

  void set(const syVLFO *para)
  {
    mode = (sInt)para->mode;
    sync = (sInt)para->sync != 0;
    eg = (sInt)para->egmode != 0;
    freq = (sInt)(0.5f * fc32bit * calcfreq(para->rate / 128.0f));
    cphase = 2*(sInt)(fc32bit * para->phase / 128.0f);

    switch ((sInt)para->pol)
    {
    case 0: // +
      gain = para->amp;
      dc = 0.0f;
      break;

    case 1: // -
      gain = -para->amp;
      dc = 0.0f;
      break;

    case 2: // +/-
      gain = para->amp;
      dc = -0.5f * para->amp;
      break;
    }
  }

  void keyOn()
  {
    if (sync)
    {
      cntr = cphase;
      last = ~0u;
    }
  }

  void tick()
  {
    sF32 v;
    sU32 x;

    switch (mode & 7)
    {
    case SAW:
    default:
      v = utof23(cntr);
      break;

    case TRI:
      x = (cntr << 1) | (sS32(cntr) >> 31);
      v = utof23(cntr);
      break;

    case PULSE:
      x = sS32(cntr) >> 31;
      v = utof23(cntr);
      break;

    case SIN:
      v = utof23(cntr);
      v = fastsinrc(v * fc2pi) * 0.5f + 0.5f;
      break;

    case S_H:
      if (cntr < last)
        nseed = urandom(&nseed);
      last = cntr;
      v = utof23(nseed);
      break;
    }

    out = v * gain + dc;
    cntr += freq;
    if (cntr < (sU32)freq && eg) // in one-shot mode, clamp at wrap-around
      cntr = ~0u;
  }
};

// --------------------------------------------------------------------------
// Distortion
// --------------------------------------------------------------------------

struct syVDist
{
  sF32 mode;    // see below
  sF32 ingain;  // -12dB .. 36dB
  sF32 param1;  // outgain / crush / outfreq / cutoff
  sF32 param2;  // offset / xor / jitter / reso
};

struct V2Dist
{
  enum Mode
  {
    OFF = 0,
    OVERDRIVE,
    CLIP,
    BITCRUSHER,
    DECIMATOR,

    FLT_BASE  = DECIMATOR,
    FLT_LOW   = FLT_BASE + V2Flt::LOW,
    FLT_BAND  = FLT_BASE + V2Flt::BAND,
    FLT_HIGH  = FLT_BASE + V2Flt::HIGH,
    FLT_NOTCH = FLT_BASE + V2Flt::NOTCH,
    FLT_ALL   = FLT_BASE + V2Flt::ALL,
    FLT_MOOGL = FLT_BASE + V2Flt::MOOGL,
    FLT_MOOGH = FLT_BASE + V2Flt::MOOGH,
  };

  sInt mode;
  sF32 gain1;     // input gain for all fx
  sF32 gain2;     // output gain for od/clip
  sF32 offs;      // offs for od/clip
  sF32 crush1;    // 1/crush_factor
  sInt crush2;    // crush_factor
  sInt crxor;     // xor value for crush
  sU32 dcount;    // decimator counter
  sU32 dfreq;     // decimator frequency
  sF32 dvall;     // last decimator value (mono/left)
  sF32 dvalr;     // last decimator value (mono/right)
  V2Flt fltl;     // filter mono/left
  V2Flt fltr;     // filter right

  void init(V2Instance *instance)
  {
    dcount = 0;
    dvall = dvalr = 0.0f;
    fltl.init(instance);
    fltr.init(instance);
  }

  void set(const syVDist *para)
  {
    sF32 x;

    mode = (sInt)para->mode;
    gain1 = powf(2.0f, (para->ingain - 32.0f) / 16.0f);

    switch (mode)
    {
    case OFF:
      break;

    case OVERDRIVE:
      gain2 = (para->param1 / 128.0f) / atan(gain1);
      offs = gain1 * 2.0f * ((para->param2 / 128.0f) - 0.5f);
      break;

    case CLIP:
      gain2 = para->param1 / 128.0f;
      offs = gain1 * 2.0f * ((para->param2 / 128.0f) - 0.5f);
      break;

    case BITCRUSHER:
      x = para->param1 * 256.0f + 1.0f;
      crush2 = (sInt)x;
      crush1 = gain1 * (32768.0f / x);
      crxor = ((sInt)para->param2) << 9;
      break;

    case DECIMATOR:
      dfreq = 2 * (sInt)(fc32bit * calcfreq(para->param1 / 128.0f));
      break;

    default: // filters
      {
        syVFlt setup;
        setup.cutoff = para->param1;
        setup.reso = para->param2;
        setup.mode = (sF32)(mode - FLT_BASE);
        fltl.set(&setup);
        fltr.set(&setup);
      }
      break;
    }
  }

  void renderMono(sF32 *dest, const sF32 *src, sInt nsamples)
  {
    switch (mode)
    {
    case OFF:
      if (dest != src)
        memmove(dest, src, nsamples * sizeof(sF32));
      break;

    case OVERDRIVE:
      for (sInt i=0; i < nsamples; i++)
        dest[i] = overdrive(src[i]);
      break;

    case CLIP:
      for (sInt i=0; i < nsamples; i++)
        dest[i] = clip(src[i]);
      break;

    case BITCRUSHER:
      for (sInt i=0; i < nsamples; i++)
        dest[i] = bitcrusher(src[i]);
      break;

    case DECIMATOR:
      for (sInt i=0; i < nsamples; i++)
      {
        decimator_tick(src[i], 0.0f);
        dest[i] = dvall;
      }
      break;

    default: // filters
      fltl.render(dest, src, nsamples);
      break;
    }
  }

  void renderStereo(StereoSample *dest, const StereoSample *src, sInt nsamples)
  {
    // @@@BUG this matches the original V2 code, but frankly I have my doubts
    // that always running the Moog filters in Mono mode is intentional... 
    switch (mode)
    {
    case DECIMATOR:
      for (sInt i=0; i < nsamples; i++)
      {
        decimator_tick(src[i].l, src[i].r);
        dest[i].l = dvall;
        dest[i].r = dvalr;
      }
      break;

    case FLT_LOW:
    case FLT_BAND:
    case FLT_HIGH:
    case FLT_NOTCH:
    case FLT_ALL:
      fltl.render(&dest[0].l, &src[0].l, nsamples, 2);
      fltr.render(&dest[0].r, &src[0].r, nsamples, 2);
      break;

    default:
      // everything else we presume to be stateless and just pass through the
      // mono version.
      renderMono(&dest[0].l, &src[0].l, nsamples*2);
    }
  }

private:
  inline sF32 overdrive(sF32 in)
  {
    return gain2 * fastatan(in * gain1 + offs);
  }

  inline sF32 clip(sF32 in)
  {
    return gain2 * clamp(in * gain1 + offs, -1.0f, 1.0f);
  }

  inline sF32 bitcrusher(sF32 in)
  {
    sInt t = (sInt)(in * crush1);
    t = clamp(t * crush2, -0x7fff, 0x7fff) ^ crxor;
    return (sF32)t / 32768.0f;
  }

  inline void decimator_tick(sF32 l, sF32 r)
  {
    dcount += dfreq;
    if (dcount < dfreq) // carry
    {
      dvall = l;
      dvalr = r;
    }
  }
};

// vim: sw=2:sts=2:et:cino=\:0l1g0(0
