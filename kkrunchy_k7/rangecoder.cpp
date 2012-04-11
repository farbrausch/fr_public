// Written by Fabian "ryg" Giesen.
// I hereby place this code in the public domain.

#include "_types.hpp"
#include "rangecoder.hpp"

// ---- helpers

extern "C" void __fastcall modelInitASM();
extern "C" sInt __fastcall modelASM(sInt bit);

static sU32 sMulShift12(sU32 a,sU32 b)
{
  __asm
  {
    mov   eax, [a];
    mul   [b];
    shrd  eax, edx, 12;
  }
}

// ---- encoder

extern "C" sU8 *bufPtr,*startBufPtr;

sU8 *bufPtr,*startBufPtr;
static sU8 *outPtr;
static sInt ffNum,cache;
static sU64 low;
static sU32 range;
static sBool firstByte;

static void shiftLow()
{
  sU32 carry = sU32(low >> 32);
  if(low < 0xff000000 || carry == 1)
  {
    if(!firstByte)
      *outPtr++ = cache + carry;
    else
      firstByte = false;

    for(;ffNum;ffNum--)
      *outPtr++ = 0xff + carry;

    cache = sInt((low >> 24) & 0xff);
  }
  else
    ffNum++;

  low = (low << 8) & 0xffffffff;
}

static void codeBit(sU32 prob,sInt bit)
{
  // adjust bound
  sU32 newBound = sMulShift12(range,prob);
  if(bit)
    range = newBound;
  else
  {
    low += newBound;
    range -= newBound;
  }

  // renormalize
  while(range < 0x01000000)
  {
    range <<= 8;
    shiftLow();
  }
}

sU32 RangecoderPack(const sU8 *in,sU32 inSize,sU8 *out,PackerCallback cb)
{
  sU32 prob = 2048;
  sU32 zeroProb = 1;

  outPtr = out;
  ffNum = 0;
  low = 0;
  range = ~0U;
  cache = 0;
  firstByte = true;
  modelInitASM();
  bufPtr = startBufPtr = (sU8 *) in;

  for(sInt pos=0;pos<inSize;pos++)
  {
    // write zero tag bit every 8k
    if((pos & 8191) == 0)
    {
      // check whether we want to execute the callback first
      if(cb)
      {
        __asm emms;
        cb(pos,inSize,sU32(outPtr - out));
      }

      // >8k still left?
      sInt isZero = (inSize - pos) > 8192;

      // check whether it's really all zeroes
      if(isZero)
      {
        for(sInt i=0;i<8192;i++)
        {
          if(bufPtr[i] != 0)
          {
            isZero = 0;
            break;
          }
        }
      }

      codeBit(zeroProb,isZero);
      zeroProb = (zeroProb + (isZero ? 4096 : 1)) >> 1;

      if(isZero)
      {
        bufPtr += 8192;
        pos += 8192-1;
        continue;
      }
    }

    for(sInt i=0;i<8;i++)
    {
      sInt bit = (*bufPtr >> (7-i)) & 1;
      codeBit(prob,bit);

      // update model
      if(i == 7)
        bufPtr++;

      prob = modelASM(bit);
    }
  }

  for(sInt i=0;i<5;i++)
    shiftLow();

  __asm emms;
  sU32 finalSize = sU32(outPtr - out);

  if(cb)
    cb(inSize,inSize,finalSize);

  return finalSize;
}

// ---- decoder

sU32 RangecoderDepack(sU8 *out,const sU8 *in,sInt outSize)
{
  // initialize ari
  sU32 code=0,range=~0U;
  sU32 prob=2048;

  for(sInt i=0;i<4;i++)
    code = (code << 8) | *in++;

  modelInitASM();
  bufPtr = startBufPtr = out;

  while(outSize)
  {
    sInt byte = 0;

    for(sInt i=0;i<8;i++)
    {
      sInt bit;
      sU32 bound = sMulShift12(range,prob);

      // decode bit
      if(code < bound)
      {
        range = bound;
        bit = 1;
      }
      else
      {
        code -= bound;
        range -= bound;
        bit = 0;
      }

      byte += byte+bit;
      if(i == 7)
        *bufPtr++ = byte;

      // renormalize
      while(range < 0x01000000)
      {
        code = (code << 8) | *in++;
        range <<= 8;
      }

      // update model
      prob = modelASM(bit);
    }

    outSize--;
  }

  __asm emms;
}
