// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __START_HPP__
#define __START_HPP__

#include "_types.hpp"

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   System                                                             ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

namespace Werkk3TexLib
{

// texture formats (for use with the new addtexture)
#define sTF_NONE            0     // illegal texture format
#define sTF_A8R8G8B8        1     // 4 x 8 bit unsigned
#define sTF_A8              2     // 1 x 8 bit unsigned
#define sTF_R16F            3     // 1 x 16 bit float
#define sTF_A2R10G10B10     4     // 3 x 10 bit unsigned + 2 bit unsigned
#define sTF_Q8W8V8U8        5     // 4 x 8 bit signed
#define sTF_A2W10V10U10     6     // 3 x 10 bit signed + 2 bit unsigned
#define sTF_A1R5G5B5        7     // 3 x 5 bit unsigned + 1 bit
#define sTF_DXT1            8     // dxt1.
#define sTF_DXT5            9     // dxt5.
#define sTF_I8              11
#define sTF_A8I8            12
#define sTF_MAX             13    // end of list

struct sSystem_
{
// HostFont
  static sU32 *FontMem;

// ***** PUBLIC INTERFACE

// bitmap loading

  static sBool LoadBitmapCore(const sU8 *data,sInt size,sInt &x,sInt &y,sU8 *&d);   // simplified version for intro

// font

  static sInt FontBegin(sInt pagex,sInt pagey,const sChar *name,sInt xs,sInt ys,sInt style);  // create font and bitmap, return real height
  static sInt FontWidth(const sChar *string,sInt count);                // get width
  static void FontCharWidth(sInt ch,sInt *widths);                // get char width (with kerning)
  static void FontPrint(sInt x,sInt y,const sChar *string,sInt count); // print
  static sU32 *FontBitmap() { return FontMem; }                  // return font bitmap
  static void FontEnd();                                         // delete bitmap.
};

extern sSystem_ *sSystem;

}

/****************************************************************************/
/****************************************************************************/

#endif
