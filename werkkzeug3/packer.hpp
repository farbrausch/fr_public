// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __PACKER_HPP_
#define __PACKER_HPP_

#include "_types.hpp"

/****************************************************************************/

typedef void (*TokenizeCallback)(void *user,sInt uncompSize,sF32 compSize);
typedef sU32 (*DepackFunction)(sU8 *dst,const sU8 *src,TokenizeCallback cb,void *cbuser);

class PackerBackEnd
{
public:
  const sU8 *Source;
  sU32 PrevOffset;
  sF64 InvAvgMatchLen;
  sF32 AvgLiteralLen;

  virtual sU32 MaxOutputSize(sU32 inSize)=0;
  virtual DepackFunction GetDepacker()=0;

  virtual sF32 MatchLen(sU32 offs,sU32 len,sU32 PO=0)=0;
  virtual sF32 LiteralLen(sU32 pos,sU32 len)=0;

  virtual sU32 EncodeStart(sU8 *dest,const sU8 *src,sF32 *profile=0)=0;
  virtual void EncodeEnd(sF32 *outProfile=0)=0;
  virtual void EncodeMatch(sU32 offs,sU32 len)=0;
  virtual void EncodeLiteral(sU32 pos,sU32 len)=0;

  virtual sU32 GetCurrentSize()=0;
};

/****************************************************************************/

typedef void (*PackerCallback)(sU32 srcPos,sU32 srcSize,sU32 dstPos);

class PackerFrontEnd
{
protected:
  PackerBackEnd *BackEnd;
  const sU8 *Source;
  sU32 SourceSize;
  sU32 SourcePtr;

  virtual void DoPack(PackerCallback cb)=0;

public:
  PackerFrontEnd(PackerBackEnd *backEnd);
  virtual ~PackerFrontEnd();

  virtual PackerBackEnd *GetBackEnd();
  virtual sU32 MaxOutputSize(sU32 inSize);
  virtual sU32 Pack(const sU8 *src,sU32 srcSize,sU8 *dst,PackerCallback cb,sInt relax=0);
};

/****************************************************************************/

class GoodPackerFrontEnd: public PackerFrontEnd
{
  struct Match
  {
    sU32 Offs;
    sU32 Len;
  };

  sU32 *Head;
  sU32 *Link;
  sU32 FillPtr;

  sU32 Ahead; // number of bytes we're ahead of actually encoded data
  sU32 AheadStart; // start position of ahead area
  Match CM; // current match
  Match BM; // best match
  
  void FindMatch(Match &match,sU32 start,sU32 lookAhead);
  void TryBetterAhead();
  void FlushAhead(sF32 bias=0.0f);

protected:
  virtual void DoPack(PackerCallback cb);

public:
  GoodPackerFrontEnd(PackerBackEnd *backEnd);
};

/****************************************************************************/

class BestPackerFrontEnd: public PackerFrontEnd
{
protected:
  virtual void DoPack(PackerCallback cb);

public:
  BestPackerFrontEnd(PackerBackEnd *backEnd);
};

/****************************************************************************/

class RangeCoder
{
  sU32 Low;
  sU32 Code;
  sU32 Range;
  sU32 Bytes;
  sU8 *Buffer;

public:
  void InitEncode(sU8 *buffer);
  void FinishEncode();
  void Encode(sU32 cumfreq,sU32 freq,sU32 totfreq);
  void EncodePlain(sU32 value,sU32 max);

  void InitDecode(sU8 *buffer);
  sU32 GetFreq(sU32 totfreq);
  void Decode(sU32 cumfreq,sU32 freq);
  sU32 DecodePlain(sU32 max);

  sU32 GetBytes()               { return Bytes; }
};

/****************************************************************************/

class RangeModel
{
  sInt *Freq;
  sInt Symbols;
  sInt TotFreq;
  sInt Thresh;

public:
  void Init(sInt symbols,sInt thresh);
  void Exit()                   { delete[] Freq; }

  sInt GetFreq(sInt symbol)     { return Freq[symbol*2+1]; }
  sInt GetCumFreq(sInt symbol)  { return Freq[symbol*2+0]; }
  sInt GetTotFreq()             { return TotFreq; }
  sInt GetSymbols()             { return Symbols; }
  sF32 GetBits(sInt symbol)     { return -sFLog(1.0f*GetFreq(symbol)/GetTotFreq())*1.4427f; }

  sF32 Encode(RangeCoder &coder,sInt symbol);
  sInt Decode(RangeCoder &coder);
  void Update(sInt symbol);

  sF32 BitsOut;
  sInt SymCount;
};

/****************************************************************************/

class CarryRangeCoder
{
  sU64 Low;
  sU32 Range;
  sU32 FFNum;
  sU8 Cache;

  sU8 *Buffer;
  sU32 Bytes;
  sBool FirstByte;

public:
  void InitEncode(sU8 *buffer);
  void FinishEncode();
  void EncodeBit(sU32 prob,sBool bit);
  void ShiftLow();

  sU32 GetBytes()               { return Bytes + FFNum; }
};

/****************************************************************************/

class BitBuffer
{
  sU8 *Buffer;
  sU32 Bytes;
  sU8 *Tag;
  sInt BitPos;

public:
  void InitEncode(sU8 *buffer);
  void FinishEncode();

  void PutBit(sBool bit);
  void PutByte(sU8 value);

  sU32 GetBytes()             { return Bytes; }
};

/****************************************************************************/

class BitModel
{
  sU32 Prob;
  sInt Move;
  static sF32 Cost[513];
  static sBool CostInit;

public:
  void Init(sInt move);

  sF32 Encode(CarryRangeCoder &coder,sBool bit);
  sF32 GetBits(sBool bit) const;
};

/****************************************************************************/

class GammaModel
{
  BitModel Models[256];
  mutable sU8 Ctx;
  
public:
  void Init(sInt move);

  sF32 Encode(CarryRangeCoder &coder,sInt value);
  sF32 GetBits(sInt value) const;
};

/****************************************************************************/

class CCAPackerBackEnd: public PackerBackEnd
{
  CarryRangeCoder Coder;
  BitModel Codes[2];
  GammaModel Gamma[2];
  BitModel MatchLow[2][16];
  BitModel PrevMatch;
  BitModel LitBit[256];

  sBool LWM;

  sF32 GammaSize[8];
  sF32 CodesSize[2];

  sF32 GammaLens[2][256];

  sF32 AccMatchLen;
  sInt AccMatchCount;
  sF32 AccLiteralLen;
  sInt AccLiteralCount;

  sF32 RealGammaLen(sInt i,sU32 value);
  sF32 EncodeGamma(sInt i,sU32 value);

  sF32 __forceinline GammaLen(sInt i,sU32 value)
  {
    if(value<256)
      return GammaLens[i][value];
    else
      return RealGammaLen(i,value);
  }

public:
  virtual sU32 MaxOutputSize(sU32 inSize);
  virtual DepackFunction GetDepacker();
  
  virtual sF32 MatchLen(sU32 offs,sU32 len,sU32 PO=0);
  virtual sF32 LiteralLen(sU32 pos,sU32 len);

  virtual sU32 EncodeStart(sU8 *dest,const sU8 *src,sF32 *profile=0);
  virtual void EncodeEnd(sF32 *outProfile=0);
  virtual void EncodeMatch(sU32 offs,sU32 len);
  virtual void EncodeLiteral(sU32 pos,sU32 len);

  virtual sU32 GetCurrentSize();
};

/****************************************************************************/

class NRVPackerBackEnd: public PackerBackEnd
{
  BitBuffer Bit;
  sInt Variant;

  void EncodePrefix11(sU32 i);
  sU32 SizePrefix11(sU32 i);

  void EncodePrefix12(sU32 i);
  sU32 SizePrefix12(sU32 i);

public:
  NRVPackerBackEnd(sInt variant);

  virtual sU32 MaxOutputSize(sU32 inSize);
  virtual DepackFunction GetDepacker();

  virtual sF32 MatchLen(sU32 offs,sU32 len,sU32 PO=0);
  virtual sF32 LiteralLen(sU32 pos,sU32 len);

  virtual sU32 EncodeStart(sU8 *dest,const sU8 *src,sF32 *profile=0);
  virtual void EncodeEnd(sF32 *outProfile=0);
  virtual void EncodeMatch(sU32 offs,sU32 len);
  virtual void EncodeLiteral(sU32 pos,sU32 len);

  virtual sU32 GetCurrentSize();
};

/****************************************************************************/

class APackPackerBackEnd: public PackerBackEnd
{
  BitBuffer Bit;
  sBool LWM;

  void EncodeGamma(sU32 value);
  sF32 SizeGamma(sU32 value);

public:
  virtual sU32 MaxOutputSize(sU32 inSize);
  virtual DepackFunction GetDepacker();

  virtual sF32 MatchLen(sU32 offs,sU32 len,sU32 PO=0);
  virtual sF32 LiteralLen(sU32 pos,sU32 len);

  virtual sU32 EncodeStart(sU8 *dest,const sU8 *src,sF32 *profile=0);
  virtual void EncodeEnd(sF32 *outProfile=0);
  virtual void EncodeMatch(sU32 offs,sU32 len);
  virtual void EncodeLiteral(sU32 pos,sU32 len);

  virtual sU32 GetCurrentSize();
};

/****************************************************************************/

#endif
