// This file is distributed under a BSD license. See LICENSE.txt for details.

// FRIED
// lowlevel bitstream functions.
// quite fast.

#ifndef __BITBUFFER_HPP__
#define __BITBUFFER_HPP__

#include "_types.hpp"

namespace FRIED
{
  // encoder.
  class BitEncoder
  {
    sU8 *Bytes;
    sInt BytePos;
    sInt Length;

    sU32 Bits;
    sInt BitPos;

  public:
    void Init(sU8 *bytes,sInt length)
    {
      Bytes = bytes;
      Length = length;
      BytePos = 0;
      Bits = 0;
      BitPos = 32;
    }

    void Flush()
    {
      if(BytePos < Length && BitPos != 32)
        Bytes[BytePos++] = (sU8) (Bits >> 24);

      Bits = 0;
      BitPos = 32;
    }

    void PutBits(sU32 value,sInt nBits)
    {
      sVERIFY(nBits <= 24);
      sVERIFY(BitPos >= nBits);
      sVERIFY(value < (1u<<nBits));

      BitPos -= nBits;
      sVERIFY((Bits & (value << BitPos)) == 0);
      Bits |= value << BitPos;

      while(BitPos <= 24 && BytePos < Length)
      {
        Bytes[BytePos++] = (sU8) (Bits >> 24);
        Bits <<= 8;
        BitPos += 8;
      }
    }

    sInt BytesWritten() const
    {
      return BytePos;
    }
  };

  // decoder.
  class BitDecoder
  {
    const sU8 *Bytes;
    const sU8 *BytePtr;
    const sU8 *ByteEnd;
    sU32 Bits;
    sInt BitFill;

    sU32 Mask[25];

    __forceinline void Refill()
    {
      while(BitFill <= 24)
      {
        Bits = (Bits << 8) | *BytePtr;
        BytePtr += (BytePtr < ByteEnd);

        BitFill += 8;
      }
    }

  public:
    void Init(const sU8 *bytes,sInt length)
    {
      Bytes = bytes;
      BytePtr = Bytes;
      ByteEnd = Bytes + length;

      Bits = 0;
      BitFill = 0;
      Refill();

      for(sInt i=0;i<=24;i++)
        Mask[i] = (1 << i) - 1;
    }

    __forceinline void SkipBitsNoCheck(sInt nBits)
    {
      BitFill -= nBits;
    }

    __forceinline void SkipBits(sInt nBits)
    {
      SkipBitsNoCheck(nBits);
      Refill();
    }

    __forceinline sU32 PeekBits(sInt nBits)
    {
      return (Bits >> (BitFill - nBits)) & Mask[nBits];
    }

    __forceinline sU32 GetBitsNoCheck(sInt nBits)
    {
      return (Bits >> (BitFill -= nBits)) & Mask[nBits];
    }

    __forceinline sU32 GetBits(sInt nBits)
    {
      sU32 val = (Bits >> (BitFill -= nBits)) & Mask[nBits];
      Refill();

      return val;
    }

    sInt BytesRead() const
    {
      if(BytePtr != ByteEnd)
        return sInt(BytePtr - Bytes) - (BitFill >> 3);
      else
        return sInt(ByteEnd - Bytes);
    }
  };
}

#endif
