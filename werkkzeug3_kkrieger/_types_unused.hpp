// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __TYPES_UNUSED_HPP__
#define __TYPES_UNUSED_HPP__

/****************************************************************************/
/***                                                                      ***/
/***   Forwards                                                           ***/
/***                                                                      ***/
/****************************************************************************/

struct sColor;

/****************************************************************************/

struct sDate
{
  sU16 Day;                       // 0..364 / 365 day of year
  sU16 Year;                      // 1..65535 year AD
  sU16 Minute;                    // 0..1439 minute of day
  sU16 Millisecond;               // 0..59999 millisecond of minute

  void RoundDay();                // clear minute and millisecond to zero
  void Sub(sDate &a,sDate &b);    // get difference of two dates
  void Add(sDate &a,sDate &b);    // add difference b to date a
  sInt DaysOfYear();              // gets days per year. 
  void SetMonth(sInt m,sInt d);   // m=0..11, d=0..27/30  sets by month and day of month
  void GetMonth(sInt &m,sInt &d); // m=0..11, d=0..27/30  gets month and day of month
  void PrintDate(sChar *);        // 2002-dec-31
  void Printime(sChar *);        // 23:59
  void ReadDate(sChar *);         // 2002-dec-31
  void ReadTime(sChar *);         // 23:59
};

/****************************************************************************/

struct sColor
{
  union
  {
    sU32 Color;
    struct
    {
      sU8 b,g,r,a;
    };
    sU8 bgra[4];
  };
  __forceinline sColor(sU32 c) { Color = c; }
  __forceinline sColor() {}
  __forceinline operator unsigned long () { return Color; }
  __forceinline void Init(sU8 R,sU8 G,sU8 B) { Color = B|(G<<8)|(R<<16)|0xff000000; };
  __forceinline void Init(sU8 R,sU8 G,sU8 B,sU32 A) { Color = B|(G<<8)|(R<<16)|(A<<24); };
  void Fade(sF32 fade,sColor c0,sColor c1);
  void Fade(sInt fade,sColor c0,sColor c1);
};

/****************************************************************************/

struct sPlacer                    // places gui elements in a grid
{
  sInt XPos;                      // rect to place the elements in
  sInt YPos;
  sInt XSize;
  sInt YSize;
  sInt XDiv;                      // number of divisions in x-axxis
  sInt YDiv;                      // number of divisions in y-axxis
  void Init(sRect &r,sInt xdiv,sInt ydiv);
  void Div(sRect &r,sInt x,sInt y,sInt xs=1,sInt ys=1);
};

/****************************************************************************/
/***                                                                      ***/
/***   StringSet                                                          ***/
/***                                                                      ***/
/****************************************************************************/

struct sStringSetEntry
{
  sChar *String;
  sObject *Object;
};

struct sStringSet
{
private:
  sInt FindNr(sChar *string);
public:
  sArray<sStringSetEntry>  Set;

  void Init();                              // create
  void Exit();                              // destroy
  void Clear();
  sObject *Find(sChar *string);             // find object set for string
  sBool Add(sChar *string,sObject *);       // add object/string pair. return sTRUE if already in list. overwites
};

/****************************************************************************/
/***                                                                      ***/
/***   RangeCoder                                                         ***/
/***                                                                      ***/
/****************************************************************************/

class sRangeCoder
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
  void EncodeGamma(sU32 value);

  void InitDecode(sU8 *buffer);
  sU32 GetFreq(sU32 totfreq);
  void Decode(sU32 cumfreq,sU32 freq);
  sU32 DecodePlain(sU32 max);
  sU32 DecodeGamma();

  sU32 GetBytes()             { return Bytes; }
};

/****************************************************************************/

class sRangeModel
{
  sInt *Freq;
  sInt Symbols;
  sInt TotFreq;

  void Update(sInt symbol);

public:
  void Init(sInt symbols);
  void Exit()                 { delete[] Freq; }

  void Encode(sRangeCoder &coder,sInt symbol);
  sInt Decode(sRangeCoder &coder);
};

/****************************************************************************/

/****************************************************************************/
/***                                                                      ***/
/***   Rectangle Packer                                                   ***/
/***                                                                      ***/
/****************************************************************************/

class sRectPacker
{
  sRect Size;
  sArray<sInt> Anchors;
  sArray<sInt> *Lines;
  sInt AreaUsed,Thresh;

  sBool IsFree(sRect &r);
  void AddAnchor(sInt x,sInt y);
  void AddRect(sRect &r);

public:
  void Init(sInt w=128,sInt h=128);
  void Exit();

  sBool PackRect(sRect &r);
  sF32 GetRelativeAreaUsed();
};

/****************************************************************************/

#endif
