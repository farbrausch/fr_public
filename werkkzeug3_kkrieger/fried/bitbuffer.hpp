// This file is distributed under a BSD license. See LICENSE.txt for details.

// FRIED
// lowlevel bitstream functions.
// quite fast.

#ifndef __BITBUFFER_HPP__
#define __BITBUFFER_HPP__

#include "_types.hpp"

namespace FRIED
{
  // encoder
  class BitEncoder
  {
	  sU8 *bp;
	  sInt length;
	  sInt acck;
	  sInt accb;
	  sInt nbytes;

  public:
    void Init(sU8 *bytes,sInt len)
    {
      bp = bytes;
      length = len;
      nbytes = 0;
      acck = 0;
      accb = 0;
    }

    void Flush()
    {
      if(acck)
        PutBits(0,8-acck);

      acck = 0;
      accb = 0;
    }

    void PutByte(sInt code)
    {
      static const sInt mask[8] = { 0,1,3,7,15,31,63,127 };

      if(nbytes < length)
      {
        code &= 0xff;
        *bp++ = (accb << (8 - acck)) | (code >> acck);
        accb = code & mask[acck];
        nbytes++;
      }
    }

    void PutBits(sInt code,sInt nb)
    {
      static const sInt mask[8] = { 0,1,3,7,15,31,63,127 };

      if(nbytes < length)
      {
        while(nb >= 8)
        {
          nb -= 8;
          PutByte(code >> nb);
        }

        if(nb)
        {
          code &= mask[nb];
          acck += nb;
          accb = (accb << nb) | code;

          if(acck >= 8)
          {
            acck -= 8;
            *bp++ = accb >> acck;
            accb &= mask[acck];
            nbytes++;
          }
        }
      }
    }

    sInt BytesWritten() const
    {
      return nbytes;
    }
  };

#if 0
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

      BitPos = 32;
    }

    void PutBits(sU32 value,sInt nBits)
    {
      BitPos -= nBits;
      Bits |= value << BitPos;

      if(BitPos <= 24 && BytePos < Length)
      {
        do
        {
          Bytes[BytePos++] = (sU8) (Bits >> 24);
          Bits <<= 8;
          BitPos += 8;
        }
        while(BitPos <= 24 && BytePos < Length);
      }
    }

    sInt BytesWritten() const
    {
      return BytePos;
    }
  };
#endif

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
        BytePtr++;
        /*if(BytePtr <= ByteEnd)
          BytePtr++;*/

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
      return sInt(BytePtr - Bytes) - (BitFill >> 3);
    }
  };
}

#endif
