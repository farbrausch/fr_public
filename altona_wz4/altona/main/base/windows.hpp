/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef HEADER_ALTONA_UTIL_WINDOWS
#define HEADER_ALTONA_UTIL_WINDOWS

#ifndef __GNUC__
#pragma once
#endif

#include "base/types.hpp"
#include "base/serialize.hpp" // for clipboard serialize
class sImage2D;

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Windowing stuff                                                    ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

void sSetMousePointer(sInt code);

enum sMousePointerCodes
{
  sMP_OFF = 0,                    // Hide cursor
  sMP_ARROW,                      // varius images
  sMP_WAIT,
  sMP_CROSSHAIR,
  sMP_HAND,
  sMP_TEXT,
  sMP_NO,
  sMP_SIZEALL,
  sMP_SIZENS,
  sMP_SIZEWE,
  sMP_SIZENWSE,
  sMP_SIZENESW,
  sMP_MAX,                        // mark end of list
};

/****************************************************************************/

void sSetWindowName(const sChar *name);
const sChar *sGetWindowName();

/****************************************************************************/

void sSetWindowMode(sInt mode);
sInt sGetWindowMode();
sBool sHasWindowFocus();            // does our application window have the os-focus?
void sSetWindowSize(sInt x,sInt y);
void sSetWindowPos(sInt x,sInt y);
void sGetWindowPos(sInt &x,sInt &y);
void sGetWindowSize(sInt &sx,sInt &sy);

enum sWindowModeCodes
{
  sWM_NORMAL = 1,
  sWM_MAXIMIZED = 2,
  sWM_MINIMIZED = 3,
};

/****************************************************************************/

void sUpdateWindow();
void sUpdateWindow(const sRect &);

/****************************************************************************/

void sSetClipboard(const sChar *,sInt len=-1);
sChar *sGetClipboard();

void sSetClipboard(const sU8 *data,sDInt size,sU32 serid,sInt sermode);
sU8 *sGetClipboard(sDInt &size,sU32 serid,sInt sermode);

void sEnableFileDrop(sBool enable);
const sChar *sGetDragDropFile();

/****************************************************************************/

template <class Type> sBool sSetClipboardObject(Type *obj,sU32 serid)
{
#if sPLATFORM==sPLAT_WINDOWS || sPLATFORM==sPLAT_LINUX
  sFile *file = sCreateGrowMemFile();
  sWriter stream; stream.Begin(file); obj->Serialize(stream); stream.End();
  if(!stream.IsOk()) return 0;
  sDInt size = file->GetSize();
  sU8 *data = file->Map(0,size);
  sSetClipboard(data,size,serid,1);
  delete file;
  return 1;
#else
  return 0;
#endif
}

template <class Type> sBool sGetClipboardObject(Type *&obj,sU32 serid)
{
#if sPLATFORM==sPLAT_WINDOWS || sPLATFORM==sPLAT_LINUX
  sDInt size;
  sU8 *data = sGetClipboard(size,serid,1);
  if(!data) return 0;
  sFile *file = sCreateMemFile(data,size);
  obj = new Type;
  sReader stream; stream.Begin(file); obj->Serialize(stream); stream.End();
  delete file;
  if(stream.IsOk())
    return 1;
  delete obj;
  obj = 0;
#endif
  return 0;
}

template <class Type> sBool sSetClipboardArray(sStaticArray<Type *>&arr,sU32 serid)
{
#if sPLATFORM==sPLAT_WINDOWS || sPLATFORM==sPLAT_LINUX
  Type *e;
  sFile *file = sCreateGrowMemFile();
  sWriter stream; stream.Begin(file); 
  stream.Header(sSerId::sStaticArray,1);
  stream.ArrayNew(arr);
  sFORALL(arr,e)
    e->Serialize(stream);
  stream.Footer();
  stream.End();
  if(!stream.IsOk()) return 0;
  sDInt size = file->GetSize();
  sU8 *data = file->Map(0,size);
  sSetClipboard(data,size,serid,2);
  delete file;
  return 1;
#else
  return 0;
#endif
}

template <class Type> sBool sGetClipboardArray(sStaticArray<Type *>&arr,sU32 serid)
{
#if sPLATFORM==sPLAT_WINDOWS || sPLATFORM==sPLAT_LINUX
  Type *e;
  sDInt size;
  sU8 *data = sGetClipboard(size,serid,2);
  if(!data) return 0;
  sFile *file = sCreateMemFile(data,size);
  sReader stream; stream.Begin(file); 
  stream.Header(sSerId::sStaticArray,1);
  stream.ArrayNew(arr);
  sFORALL(arr,e)
    e->Serialize(stream);
  stream.Footer();
  stream.End();
  delete file;
  return stream.IsOk();
#else
  return 0;
#endif
}

/****************************************************************************/

enum sSystemFileOpenDialogFlags
{
  sSOF_LOAD = 1,
  sSOF_SAVE = 2,
  sSOF_DIR = 3,

  sSOF_MULTISELECT = 4,
};

sBool sSystemOpenFileDialog(const sChar *label,const sChar *extensions,sInt flags,const sStringDesc &buffer);

enum sSystemMessageFlags
{
  sSMF_ERROR = 1,
  sSMF_OK = 2,
  sSMF_OKCANCEL = 3,
  sSMF_YESNO = 4,
};

sBool sSystemMessageDialog(const sChar *label,sInt flags);
sBool sSystemMessageDialog(const sChar *label,sInt flags,const sChar *title);

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Graph2D - interface to standard windows system                     ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

// convention: use colid 0 as short-time color!
// drawing and clipping only allowed during OnPaint2D or 
// between sRender2DBegin/sRender2DEnd

// drawing

void sSetColor2D(sInt colid,sU32 color);
sU32 sGetColor2D(sInt colid);
void sRect2D(sInt x0,sInt y0,sInt x1,sInt y1,sInt colid);
void sRect2D(const sRect &,sInt colid);
void sRectInvert2D(sInt x0,sInt y0,sInt x1,sInt y1);
void sRectInvert2D(const sRect &);
void sRectFrame2D(sInt x0,sInt y0,sInt x1,sInt y1,sInt colid);
void sRectFrame2D(const sRect &,sInt colid);
void sRectHole2D(const sRect &out,const sRect &hole,sInt colid);
void sLine2D(sInt x0,sInt y0,sInt x1,sInt y1,sInt colid);
void sLine2D(sInt *list,sInt count,sInt colid);
void sLineList2D(sInt *list,sInt count,sInt colid);
void sBlit2D(const sU32 *data,sInt width,const sRect &dest);      // good for constantly changing data, for constant images use sImage2D!
void sStretch2D(const sU32 *data,sInt width,const sRect &source,const sRect &dest);

// clipping

void sClipFlush();
void sClipPush();
void sClipPop();
void sClipExclude(const sRect &);
void sClipRect(const sRect &);

// render control. you can call it during OnPaint2D(), but do not nest any further.

void sRender2DBegin();                // this renders on screen, outside messageloop
void sRender2DBegin(sInt xs,sInt ys); // render to throwaway-offscreen surface
void sRender2DBegin(sImage2D *);      // render to preallocated offscreen surface
void sRender2DEnd();                                  
void sRender2DSet(sU32 *data);        // set pixels (data -> bitmap)
void sRender2DGet(sU32 *data);        // get pixels (bitmap -> data)


/****************************************************************************/

struct sPrintInfo
{
  void Init();
  sInt Mode;                      // sPIM_???         

  sInt CursorPos;                 // print cursor here. -1 to disable
  sInt Overwrite;                 // print a fat overwrite cursor
  sInt SelectStart;               // start selection mark here, -1 to disable
  sInt SelectEnd;                 // end selection mark here, must be greater than start
  sU32 SelectBackColor;           // second back color for selection. -1 to swap front and back instead
  sInt HintLine;                  // fine vertical line that hints the right border of the page
  sU32 HintLineColor;             // sGC_???

  sRect CullRect;                 // multiline prints profit from culling

  sInt QueryX;
  sInt QueryY;
  const sChar *QueryPos;
  sBool HasSelection() { return SelectStart>=0 && SelectEnd>=0 && SelectStart<SelectEnd; }
};

enum sPrintInfoMode
{
  sPIM_PRINT = 0,                 // just print the text
  sPIM_GETHEIGHT,                 // ??
  sPIM_POINT2POS,                 // set QueryPos on letter index for a QueryX/QueryY Click.
  sPIM_POS2POINT,                 // set QueryX/QueryY for letter of index QueryPos.
  sPIM_QUERYDONE,                 // this is set after successfull sPIM_POINT2POS
};

class sFont2D
{
  struct sFont2DPrivate *prv;
public:
  sFont2D(const sChar *name,sInt size,sInt flags,sInt width=0);
  ~sFont2D();

  sInt GetHeight();
  sInt GetBaseline();
  sInt GetCharHeight();
  sInt GetWidth(const sChar *text,sInt len=-1);
  sInt GetAdvance(const sChar *text,sInt len=-1);
  sInt GetCharCountFromWidth(sInt width,const sChar *text,sInt len=-1);

  struct sLetterDimensions
  {
    sS16 Pre;                       ///< kerning before character in bitmap-pixels
    sS16 Cell;                      ///< size of bitmap-cell in bitmap-pixels
    sS16 Post;                      ///< kerning after character in bitmap-pixels
    sS16 Advance;                   ///< total advance in bitmap-pixels
    sS16 OriginY;                   // glyph origin relative to baseline
    sS16 Height;                    // height of the glyphs black box
  };
  sLetterDimensions sGetLetterDimensions(const sChar letter);
  sBool LetterExists(sChar letter);

  static void AddResource(const sChar *filename);
  void Init(const sChar *name,sInt size,sInt flags,sInt width=0);
  void Exit();
  void SetColor(sInt text,sInt back);
  void PrintMarked(sInt flags,const sRect *r,sInt x,sInt y,const sChar *text,sInt len=-1,sPrintInfo *pi=0);
  void PrintBasic(sInt flags,const sRect *r,sInt x,sInt y,const sChar *text,sInt len=-1);
  void Print(sInt flags,sInt x,sInt y,const sChar *text,sInt len=-1);
  sInt Print(sInt flags,const sRect &r,const sChar *text,sInt len=-1,sInt margin=0,sInt xo=0,sInt yo=0,sPrintInfo *pi=0);
};

enum sFont2DCreateFlags
{
  sF2C_ITALICS     = 0x0001,
  sF2C_BOLD        = 0x0002,
  sF2C_HINTED      = 0x0004,
  sF2C_UNDERLINE   = 0x0008,
  sF2C_STRIKEOUT   = 0x0010,
  sF2C_SYMBOLS     = 0x0020,        // required to load windings or webdings
  sF2C_NOCLEARTYPE = 0x0040,        // disable cleartype on windows. for render font into texture.
};

enum sFont2DPrintFlags
{
  sF2P_LEFT         = 0x0001,       // align
  sF2P_RIGHT        = 0x0002,
  sF2P_TOP          = 0x0004,
  sF2P_BOTTOM       = 0x0008, 
  sF2P_MULTILINE    = 0x0010,       // wrap words
  sF2P_JUSTIFIED    = 0x0020,       // justify paragraphs
  sF2P_LIMITED      = 0x0040,       // squeeze into box
  sF2P_SPACE        = 0x0080,       // add a "space" to the margin

  sF2P_OPAQUE       = 0x1000,
};

/****************************************************************************/

class sImage2D
{
  friend void sRender2DBegin(sImage2D *img);
  struct sImage2DPrivate *prv;
public:
  sImage2D(sInt xs,sInt ys,sU32 *data);
  ~sImage2D();

  void Update(sU32 *data);
  void Paint(sInt x,sInt y);
  void Paint(const sRect &source,sInt x,sInt y);
  void Stretch(const sRect &source,const sRect &dest);

  sInt GetSizeX();
  sInt GetSizeY();
};

/****************************************************************************/

// HEADER_ALTONA_UTIL_WINDOWS
#endif


