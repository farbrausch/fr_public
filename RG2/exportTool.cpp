// This code is in the public domain. See LICENSE for details.

#include "stdafx.h"
#include "exportTool.h"
#include "plugbase.h"
#include "stream.h"

static sInt doShift(sU32 v,sInt shift)
{
  while(shift>0)
    v=(v<<1)|1,shift--;

  while(shift<0)
    v>>=1,shift++;

  return v;
}

void putToterFloat(fr::stream& f, const sF32 v)
{
  sU32 value;
  sU16 dest;
  sInt esrc,edst;

  value = *(sU32 *) &v;
  esrc = ((value>>23) & 255) - 128;
  edst = (esrc < -16) ? -16 : (esrc > 15) ? 15 : esrc;

  dest = ((value>>16) & 32768) // sign
    | ((edst+16)<<10)
    | (doShift(value>>13,edst-esrc) & 1023);

  f.putsU16(dest);
}

void putPackedFloat(fr::stream &f, sF32 val)
{
  if (val*val<1e-8f) // value zero or near-zero
    f.putsU8(0); // put zero tag
  else if (fabs((val*val)-1.0f)<1e-8f) // value one, minus one or very close to it
    f.putsU8((val<0)?0xff:0x01); // put one/minus one tag
  else
  {
    sU32 fltVal=*((sU32 *) &val);
    sU8 exp=fltVal>>23;
    FRASSERT(exp != 0x00 && exp != 0xff && exp != 0x01);

    f.putsU8(exp); // put exponent
    f.putsU8(fltVal>>8); // put mantissa 2
    f.putsU8(((fltVal>>24) & 128)|((fltVal>>16) & 127)); // put sign+mantissa 1
  }
}

static sInt valueClass(sF32 val)
{
  if(fabs(val) < 1e-4f)
    return 1; // zero
  else if(fabs(val * val - 1.0f) < 1e-5f)
    return (val < 0) ? 3 : 2;
  else
    return 0;
}

void putPackedVector(fr::stream &f, fr3Float val, sBool delta, sInt killMask)
{
  delta = sFALSE;
  killMask = 0;

  if (killMask & 1)
    val.a = 0.0f;

  if (killMask & 2)
    val.b = delta ? val.a : 0.0f;

  if (killMask & 4)
    val.c = delta ? val.a : 0.0f;

  if(delta)
  {
    val.b -= val.a;
    val.c -= val.a;
  }

  sU8 type = (valueClass(val.a) << 0) | (valueClass(val.b) << 2) | (valueClass(val.c) << 4);
  if(fabs(val.b - val.a) < 1e-4f && fabs(val.c - val.a) < 1e-4f)
    type |= 64;

  f.putsU8(type);
  if(!(type &  3)) putToterFloat(f, val.a);
  if(!(type & 64))
  {
    if(!(type & 12)) putToterFloat(f, val.b);
    if(!(type & 48)) putToterFloat(f, val.c);
  }

  /*putPackedFloat(f, val.a);
  putPackedFloat(f, delta ? val.b-val.a : val.b);
  putPackedFloat(f, delta ? val.c-val.a : val.c);*/
}

void putSmallInt(fr::stream& f, sU32 value)
{
  FRASSERT(value < 32768);

  f.putsU8((value & 127) | (value < 128 ? 0 : 128));
  if (value >= 128)
    f.putsU8((value >> 7) & 0xff);
}

sInt colorClass(const frColor& col)
{
  if(col.r == col.g && col.g == col.b)
  {
    if(col.r == 0) // black
      return 2;
    else if(col.r == 255) // white
      return 3;
    else
      return 1;
  }
  else
    return 0;
}

void putColor(fr::stream& f, const frColor& col)
{
  switch(colorClass(col))
  {
  case 0:
    f.putsU8(col.b);
    f.putsU8(col.g);
    // fall-thru

  case 1:
    f.putsU8(col.r);
    break;
  }
}
