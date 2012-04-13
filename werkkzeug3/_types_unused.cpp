// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_types.hpp"
#include "_types_unused.hpp"

/****************************************************************************/

static sInt sClamp256x(sInt a)
{
  if(a<0)
    return 0;
  if(a>255)
    return 255;
  return a;
}

void sColor::Fade(sF32 fade,sColor c0,sColor c1)
{
  sInt ff;

  ff = sFtol(fade*256);
  r = sClamp256x(c0.r + (((c1.r-c0.r)*ff)>>8));
  g = sClamp256x(c0.g + (((c1.g-c0.g)*ff)>>8));
  b = sClamp256x(c0.b + (((c1.b-c0.b)*ff)>>8));
  a = sClamp256x(c0.a + (((c1.a-c0.a)*ff)>>8));
}

void sColor::Fade(sInt fade,sColor c0,sColor c1)
{
  r = sClamp256x(c0.r + (((c1.r-c0.r)*fade)>>8));
  g = sClamp256x(c0.g + (((c1.g-c0.g)*fade)>>8));
  b = sClamp256x(c0.b + (((c1.b-c0.b)*fade)>>8));
  a = sClamp256x(c0.a + (((c1.a-c0.a)*fade)>>8));
}

/****************************************************************************/

void sPlacer::Init(sRect &r,sInt xdiv,sInt ydiv)
{
  XPos = r.x0;
  YPos = r.y0;
  XSize = r.XSize();
  YSize = r.YSize();
  XDiv = xdiv;
  YDiv = ydiv;
}

void sPlacer::Div(sRect &r,sInt x,sInt y,sInt xs,sInt ys)
{
  xs+=x;
  ys+=y;
  r.x0 = XPos + x  * XSize / XDiv;
  r.y0 = YPos + y  * YSize / YDiv;
  r.x1 = XPos + xs * XSize / XDiv;
  r.y1 = YPos + ys * YSize / YDiv;
}

/****************************************************************************/
/***                                                                      ***/
/***   StringSet                                                          ***/
/***                                                                      ***/
/****************************************************************************/

sInt sStringSet::FindNr(sChar *string)
{
  sInt i;

  for(i=0;i<Set.Count;i++)
    if(sCmpString(Set[i].String,string)==0)
      return i;
  return -1;
}

void sStringSet::Init()
{
  Set.Init(16);
  Set.Count = 0;
}

void sStringSet::Exit()
{
  Set.Exit();
}

void sStringSet::Clear()
{
  Set.Count = 0;
}

sObject *sStringSet::Find(sChar *string)
{
  sInt i;

  i = FindNr(string);
  if(i!=-1)
    return Set[i].Object;
  else
    return 0;
}

sBool sStringSet::Add(sChar *string,sObject *obj)
{
  sInt i;

  i = FindNr(string);
  if(i!=-1)
  {
    Set[i].Object = obj;
    return sTRUE;
  }
  else
  {
    i = Set.Count++;
    Set.AtLeast(i+1);
    Set[i].String = string;
    Set[i].Object = obj;
    return sFALSE;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Range Coder                                                        ***/
/***                                                                      ***/
/****************************************************************************/

#define sRANGETOP   (1<<24)
#define sRANGEBOT   (1<<16)

void sRangeCoder::InitEncode(sU8 *buffer)
{
  Buffer = buffer;
  Bytes = Low = 0;
  Range = ~0;
}

void sRangeCoder::FinishEncode()
{
  sInt i;

  for(i=0;i<4;i++)
  {
    Buffer[Bytes++] = Low>>24;
    Low <<= 8;
  }
}

void sRangeCoder::Encode(sU32 cumfreq,sU32 freq,sU32 totfreq)
{
  sVERIFY(cumfreq+freq<=totfreq && freq && totfreq<sRANGEBOT);
  Range /= totfreq;
  Low += cumfreq * Range;
  Range *= freq;

  while ((Low ^ Low+Range)<sRANGETOP || Range<sRANGEBOT)
  {
    if(Range<sRANGEBOT)
      Range = -((sInt)Low) & (sRANGEBOT-1);

    Buffer[Bytes++] = Low>>24;
    Low <<= 8;
    Range <<= 8;
  }
}

void sRangeCoder::EncodePlain(sU32 value,sU32 max)
{
  Encode(value,1,max);
}

void sRangeCoder::EncodeGamma(sU32 value)
{
  sInt invertlen;
  sU32 invert;

  value += 2;
  sVERIFY(value>=2 && value<0x80000000);

  invert = 0;
  invertlen = 0;
  do
  {
    invert = (invert << 1) | (value & 1);
    invertlen++;
  }
  while ((value >>= 1) > 1);

  while(--invertlen)
  {
    EncodePlain(invert&1,2);
    EncodePlain(1,2);
    invert >>= 1;
  }

  EncodePlain(invert&1,2);
  EncodePlain(0,2);
}

void sRangeCoder::InitDecode(sU8 *buffer)
{
  sInt i;

  Bytes = Low = Code = 0;
  Range = ~0;
  Buffer = buffer;

  for(i=0;i<4;i++)
    Code = (Code<<8) + Buffer[Bytes++];
}

sU32 sRangeCoder::GetFreq(sU32 totfreq)
{
  Range /= totfreq;
  return (Code - Low) / Range;
}

void sRangeCoder::Decode(sU32 cumfreq,sU32 freq)
{
  Low += cumfreq * Range;
  Range *= freq;
  while((Low ^ Low+Range)<sRANGETOP || Range<sRANGEBOT)
  {
    if(Range<sRANGEBOT)
      Range = -((sInt) Low) & (sRANGEBOT-1);

    Code = (Code<<8) + Buffer[Bytes++];
    Range <<= 8;
    Low <<= 8;
  }
}

sU32 sRangeCoder::DecodePlain(sU32 max)
{
  sU32 v;

  v = GetFreq(max);
  Decode(v,1);

  return v;
}

sU32 sRangeCoder::DecodeGamma()
{
  sU32 v;

  v = 1;
  do
  {
    v = (v*2) + DecodePlain(2);
  }
  while(DecodePlain(2));

  return v-2;
}

/****************************************************************************/

void sRangeModel::Update(sInt symbol)
{
  sInt i,j;

  Freq[symbol*2+1]++;
  TotFreq++;

  for(i=symbol+1;i<Symbols;i++)
    Freq[i*2]++;

  if(TotFreq>=sRANGEBOT)
  {
    j = 0;
    for(i=0;i<Symbols;i++)
    {
      Freq[i*2+0] = j;
      Freq[i*2+1] = (Freq[i*2+1]+1) / 2;
      j += Freq[i*2+1];
    }

    TotFreq = j;
  }
}

void sRangeModel::Init(sInt symbols)
{
  sInt i;

  Symbols = symbols;
  Freq = new sInt[symbols*2];
  TotFreq = symbols;

  for(i=0;i<symbols;i++)
  {
    Freq[i*2+0] = i;
    Freq[i*2+1] = 1;
  }
}

void sRangeModel::Encode(sRangeCoder &coder,sInt symbol)
{
  sVERIFY(symbol>=0 && symbol<Symbols);

  coder.Encode(Freq[symbol*2+0],Freq[symbol*2+1],TotFreq);
  Update(symbol);
}

sInt sRangeModel::Decode(sRangeCoder &coder)
{
  sInt i,j,val;

  val = coder.GetFreq(TotFreq);
  sVERIFY(val < TotFreq);

  for(i=0;i<Symbols;i++)
  {
    j = val-Freq[i*2+0];
    if(j>=0 && j<Freq[i*2+1])
      break;
  }

  coder.Decode(Freq[i*2+0],Freq[i*2+1]);
  Update(i);

  return i;
}

/****************************************************************************/
/***                                                                      ***/
/***   Rectangle Packer (this is both quite fast and quite good)          ***/
/***                                                                      ***/
/****************************************************************************/

sBool sRectPacker::IsFree(sRect &r)
{
  sInt i,*pp,sr,dr;

  if(!r.Inside(Size))
    return sFALSE;

  sr = r.x0 + r.x1;
  dr = r.x1 - r.x0;

  for(i=r.y0;i<r.y1;i++)
  {
    pp = &Lines[i][Lines[i].Count];
    do
    {
      pp -= 2;
    }
    while(sAbs(sr - pp[0]) >= dr + pp[1]);

    if(pp[1] != Size.x1)
      return sFALSE;
  }

  return sTRUE;
}

void sRectPacker::AddAnchor(sInt x,sInt y)
{
  sInt i,j;

  for(i=0,j=x+y;j>=Anchors[i];i+=2);

  Anchors.Resize(Anchors.Count+2);
  for(j=Anchors.Count-1;j>i+1;j--)
    Anchors[j] = Anchors[j-2];

  Anchors[i+0] = x+y;
  Anchors[i+1] = x-y;
}

void sRectPacker::AddRect(sRect &r)
{
  sInt i,sr,dr;

  sr = r.x0 + r.x1;
  dr = r.x1 - r.x0;
  for(i=r.y0;i<r.y1;i++)
  {
    *Lines[i].Add() = sr;
    *Lines[i].Add() = dr;
  }

  AddAnchor(r.x1,r.y0);
  AddAnchor(r.x0,r.y1);
  AreaUsed += r.XSize() * r.YSize();
}

void sRectPacker::Init(sInt w,sInt h)
{
  sInt i;

  Size.Init(0,0,w,h);
  Anchors.Init();
  Anchors.Resize(4);
  Anchors[0] = 0;
  Anchors[1] = 0;
  Anchors[2] = w+h;
  Anchors[3] = w-h;
  AreaUsed = 0;
  Thresh = Size.XSize() * Size.YSize() * 95 / 100;

  Lines = new sArray<sInt>[h];
  for(i=0;i<h;i++)
  {
    Lines[i].Init();
    *Lines[i].Add() = Size.x1;
    *Lines[i].Add() = Size.x1;
  }
}

void sRectPacker::Exit()
{
  sInt i;

  Anchors.Exit();

  for(i=0;i<Size.y1;i++)
    Lines[i].Exit();
  delete[] Lines;
}

sBool sRectPacker::PackRect(sRect &r)
{
  sInt i,j,x,y;
  sRect t;

  if(AreaUsed >= Thresh)
    return sFALSE;

  sVERIFY(r.x0 == 0 && r.y0 == 0);

  j = Size.x1+Size.y1-r.x1-r.y1;
  for(i=0;i<Anchors.Count;i+=2)
  {
    if(Anchors[i+0] >= j)
      return sFALSE;

    x = (Anchors[i+0] + Anchors[i+1]) / 2;
    y = (Anchors[i+0] - Anchors[i+1]) / 2;
    t.Init(x,y,x+r.x1,y+r.y1);
    if(IsFree(t))
      break;
  }

  j = 0;
  r = t;
  t.x0 = t.x0 * 2;
  t.y0 = t.y0 * 2;
  t.x1 = t.x1 * 2;
  t.y1 = t.y1 * 2;

  for(i=0;i<Anchors.Count;i+=2)
  {
    x = Anchors[i+0];
    y = Anchors[i+1];

    if(!t.Hit(x+y,x-y))
    {
      Anchors[j++] = x;
      Anchors[j++] = y;
    }
  }

  Anchors.Count = j;
  AddRect(r);

  return sTRUE;
}

sF32 sRectPacker::GetRelativeAreaUsed()
{
  return 1.0f * AreaUsed / (Size.XSize() * Size.YSize());
}
