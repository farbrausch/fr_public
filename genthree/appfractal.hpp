// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef __APP_FRACTAL_HPP__
#define __APP_FRACTAL_HPP__

#include "_util.hpp"
#include "_gui.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   windows                                                            ***/
/***                                                                      ***/
/****************************************************************************/

class sFractalApp : public sGuiWindow
{
  sBitmap *Bitmap;
  sInt Texture;
  sU32 Material[256];
  sInt PaintHandle;
  sInt CalcNum;
  sInt DragMode;
  sRect DragRect;

  sFRect History[256];
  sU32 Palette[64];
  sInt HistCount;
public:
  sFractalApp();
  ~sFractalApp();
  void Tag();
  void OnPaint();
  void OnCalcSize();
  void OnKey(sU32 key);
  void OnDrag(sDragData &);
  void OnFrame();
  void Calc();
};

/****************************************************************************/

#endif
