/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_PLANPAD_PDF_HPP
#define FILE_PLANPAD_PDF_HPP

#include "base/types2.hpp"

/****************************************************************************/

class sPDF
{
  // the document

  sArray<sU8> Data;
  void Print(const sChar *);

  // xref

  sArray<sInt> Objects;           // object 0 = dummy, 1 = pages, 2 = catalog
  sInt AddObject();               // "x 0 obj\n"
  sArray<sInt> Pages;             // all page objects
  sArray<sInt> PageFonts;         // resources used by current page
  void PageFont(sInt font);

  // stream assembly

  sArray<sU8> StreamData;
  void Stream(const sChar *);
  void Stream(const sChar *,sInt len);
  void StreamString(const sChar *text,sInt len);
  sInt AddStream();               // add stream as object, returns object id

  // formatted printing

  sPRINTING0(PrintF,sFormatStringBuffer buf; sFormatStringBaseCtx(buf,format);buf,Print(buf.Get());)
  sPRINTING0(StreamF,sFormatStringBuffer buf; sFormatStringBaseCtx(buf,format);buf,Stream(buf.Get());)

  sInt cache_Tf_font;
  sF32 cache_Tf_scale;
  sU32 cache_rg;
public:
  sPDF();
  ~sPDF();

  // general preparation

  void BeginDoc(sF32 xs=597.6f,sInt ys=842.4f);   // start with A4 page size
  sInt RegisterFont(sInt flags,class sFont2D *font);                  // returns font object #
  void EndDoc(const sChar *filename);             // write header and save file

  // cache

  void InvalidateCache();
  void _Tf(sInt font,sF32 scale);
  void _rg(sU32 col);

  // printing commands

  sInt BeginPage();                               // return page number, starting with 0
  void EndPage();
  void Text(sInt font,sF32 s,sF32 x,sF32 y,sU32 col,const sChar *text,sInt len=-1);
  void Text(sInt font,sF32 matrix[6],sU32 col,const sChar *text,sInt len=-1);
  void Rect(const sFRect &r,sU32 col);
  void RectFrame(const sFRect &r,sU32 col,sF32 w);

  // quick stats;

  sF32 SizeX,SizeY;               // page size
  sInt Page;                      // starts with 0!
};

enum sPDFEnum
{
  sPDF_Times = 0x1,
  sPDF_Courier = 0x2,
  sPDF_Helvetica = 0x3,
  sPDF_Symbol = 0x4,
  sPDF_Dingbats = 0x5,

  sPDF_Regular = 0x00,
  sPDF_Italic = 0x10,
  sPDF_Bold = 0x20,
  sPDF_BoldItalic = 0x30,
};

/****************************************************************************/

#endif // FILE_PLANPAD_PDF_HPP

