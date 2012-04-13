// This code is in the public domain. See LICENSE for details.

#include "types.h"
#include "opdata.h"
#include "math3d_2.h"

sInt getSmallInt(const sU8*& strm)
{
  sInt val = *strm++;

  if (val & 128)
    val = (val & 127) | ((*strm++) << 7);

  return val;
}

sF32 getPackedFloat(const sU8*& strm)
{
  sU8 firstByte = *strm++;
  
  if (firstByte == 0x00)
    return 0.0f;
  else if (firstByte == 0x01)
    return 1.0f;
  else if (firstByte == 0xff)
    return -1.0f;

  sU16 secondWord = *((sU16 *) strm);
  strm += 2;

  sU32 fullVal = (sU32(firstByte) << 23) | ((sU32(secondWord) & 32768) << 16) | ((sU32(secondWord) & 32767) << 8);
  return *((sF32 *) &fullVal);
}

sF32 getCodedFloat(const sU8* &ptr, sInt code)
{
  static const sF32 vals[4] = { 0, 0, 1, -1 };

  if((code & 3) == 0)
    return getToterFloat(ptr);
  else
    return vals[code & 3];
}

void getPackedVec(fr::vector& vec, const sU8*& strm)
{
  sInt code = *strm++;

  vec.x = getCodedFloat(strm, code >> 0);
  if(code & 64)
    vec.y = vec.z = vec.x;
  else
  {
    vec.y = getCodedFloat(strm, code >> 2);
    vec.z = getCodedFloat(strm, code >> 4);
  }
  /*vec.x = getPackedFloat(strm);
  vec.y = getPackedFloat(strm);
  vec.z = getPackedFloat(strm);*/
}

sF32 getToterFloat(const sU8 *&ptr)
{
  sU16 v;
  sU32 vd;

  v = *(const sU16 *) ptr;
  vd = (v & 32768) << 16 // sign
    | ((((v >> 10) & 31) + 128 - 16) << 23)
    | ((v & 1023) << 13);

  ptr += 2;
  return *(sF32 *) &vd;
}
