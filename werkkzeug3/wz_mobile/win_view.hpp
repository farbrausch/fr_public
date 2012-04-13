// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_types2.hpp"
#include "doc.hpp"
#include "materials/material11.hpp"

class ViewBitmapWin_ : public sGuiWindow2
{
  sInt BitmapX;
  sInt BitmapY;
  sInt BitmapTile;
  sInt BitmapAlpha;
  sInt BitmapZoom;
  sInt LightPosX,LightPosY;       // for normalmap viewing

  sInt DragStartX;
  sInt DragStartY;
  sInt DragMode;                  // DM_???
  sInt FullSize;

  sInt ShowSizeX;
  sInt ShowSizeY;
  sInt ShowVariant;

  enum DragModeEnum
  {
    DMB_SCROLL = 1,
    DMB_ZOOM,
    DMB_LIGHT,
  };
  enum CommandEnum
  {
    CMD_VIEWBMP_RESET = 0x1000,
    CMD_VIEWBMP_TILE,
    CMD_VIEWBMP_ALPHA,
    CMD_VIEWBMP_FULLSIZE,
    CMD_VIEWBMP_POPUP,
  };

  sMaterialEnv MtrlEnv;

  sMaterial11 *MtrlTex;
  sMaterial11 *MtrlTexAlpha;
  sMaterial11 *MtrlNormal;
  sMaterial11 *MtrlNormalLight;

  sToolBorder *Tools;
public:
  ViewBitmapWin_();
  ~ViewBitmapWin_();
  void OnPaint();
  void OnPaint3d(sViewport &view);
  void OnKey(sU32 key);
  void OnDrag(sDragData &dd);
  sBool OnCommand(sU32 cmd);

  GenOp *ShowOp;

  void SetOp(GenOp *);
};


