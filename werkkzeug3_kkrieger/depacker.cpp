// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_types.hpp"
#include "depacker.hpp"

/****************************************************************************/

static const sInt CodeModel = 0;
static const sInt PrevMatchModel = 2;
static const sInt MatchLowModel = 3; // +(pos>=16)*16
static const sInt LiteralModel = 35;
static const sInt Gamma0Model = 291;
static const sInt Gamma1Model = 547;
static const sInt SizeModels = 803;

struct DepackState
{
  // ari
  const sU8 *Src;
  sU32 Code,Range;

  // other
  sF32 CodeSize;
  sBool TrackSize;
  
  // probabilities
  sU32 Model[SizeModels];
};

static DepackState st;

static sBool DecodeBit(sInt index,sInt move)
{
  sU32 bound;
  sBool result;

  // decode
  bound = (st.Range >> 11) * st.Model[index];
  if(st.Code < bound)
  {
    if(st.TrackSize)
      st.CodeSize -= sFLog(1.0f * st.Model[index] / 2048) / sFLog(2.0f);

    st.Range = bound;
    st.Model[index] += (2048 - st.Model[index]) >> move;
    result = sFALSE;
  }
  else
  {
    if(st.TrackSize)
      st.CodeSize -= sFLog(1.0f * (2048 - st.Model[index]) / 2048) / sFLog(2.0f);

    st.Code -= bound;
    st.Range -= bound;
    st.Model[index] -= st.Model[index] >> move;
    result = sTRUE;
  }

  // renormalize
  //while(st.Range < (1<<24))
  if(st.Range < 0x01000000U) // no while, all our codes take <8bit
  {
    st.Code = (st.Code << 8) | *st.Src++;
    st.Range <<= 8;
  }

  return result;
}

static sInt DecodeTree(sInt model,sInt maxb,sInt move)
{
  sInt ctx;

  ctx = 1;
  while(ctx < maxb)
    ctx = (ctx * 2) + DecodeBit(model + ctx,move);

  return ctx - maxb;
}

static sInt DecodeGamma(sInt model)
{
  sInt value = 1;
  sU8 ctx = 1;

  do
  {
    ctx = ctx * 2 + DecodeBit(model + ctx,5);
    value = (value * 2) + DecodeBit(model + ctx,5);
    ctx = ctx * 2 + (value & 1);
  }
  while(ctx & 2);

  return value;
}

sU32 CCADepacker(sU8 *dst,const sU8 *src,TokenizeCallback cb,void *cbuser)
{
  sInt i,code,offs,len,R0,LWM,compSize;
  sU8 *dsto = dst;
  sInt count = 0;

  st.Code = 0;
  for(i=0;i<4;i++)
    st.Code = (st.Code<<8) | *src++;

  st.Src = src;
  st.Range = ~0;
  st.TrackSize = !!cb;
  for(i=0;i<SizeModels;i++)
    st.Model[i] = 1024;

  code = 0;
  LWM = 0;
  st.CodeSize = 0.0f;

  while(1)
  {
    switch(code)
    {
    case 0: // literal
      *dst++ = DecodeTree(LiteralModel,256,4);
      compSize = 1;
      LWM = 0;
      break;
      
    case 1: // match
      len = 0;

      if(!LWM && DecodeBit(PrevMatchModel,5)) // prev match
        offs = R0;
      else
      {
        offs = DecodeGamma(Gamma0Model);
        if(!offs)
          return dst - dsto;

        offs -= 2;
        offs = (offs << 4) + DecodeTree(MatchLowModel + (offs ? 16 : 0),16,5) + 1;
        
        if(offs>=2048)  len++;
        if(offs>=96)    len++;
      }
      
      R0 = offs;
      LWM = 1;      
      len += DecodeGamma(Gamma1Model);
      compSize = len;

      while(len--)
        *dst++ = dst[-offs];
      break;
    }

    if(cb)
      cb(cbuser,compSize,st.CodeSize);

    st.CodeSize = 0.0f;
    code = DecodeBit(CodeModel + LWM,5);
  }

  return 0;
}

/****************************************************************************/
