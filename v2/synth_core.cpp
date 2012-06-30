#include "types.h"
#include <math.h>

// --------------------------------------------------------------------------
// Constants.
// --------------------------------------------------------------------------

static const sF32 fcpi_2  = 1.5707963267948966192313216916398f;
static const sF32 fcpi    = 3.1415926535897932384626433832795f;
static const sF32 fc1p5pi = 4.7123889803846898576939650749193f;
static const sF32 fc2pi   = 6.28318530717958647692528676655901f;

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

static sF32 float2bits(sF32 f)
{
  FloatBits x;
  x.f = f;
  return x.u;
}

static sU32 bits2float(sU32 u)
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

// vim: sw=2:sts=2:et
