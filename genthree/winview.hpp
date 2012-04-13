// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef __WINVIEW_HPP__
#define __WINVIEW_HPP__

#include "_util.hpp"
#include "_gui.hpp"
#include "apptool.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   windows                                                            ***/
/***                                                                      ***/
/****************************************************************************/

class ViewWin : public ToolWindow
{
  friend class ParaWin;

  ToolObject *Object;
  class GenMesh *ShaderBall;
  sU32 ShaderBallCalc;
  class GenBitmap *TextureCache;
  sU32 TextureCacheCount;
  sVector DragVec;
  sF32 DragStartFX;
  sF32 DragStartFY;
  sInt DragStartX;
  sInt DragStartY;
  sInt DragMode;
  sInt DragCID;                    // sCID of current object
  sInt DragKey;
  sInt StickyKey;
  class PageOp *LinkEditObj;

  sF32 QuakeSpeedForw;
  sF32 QuakeSpeedSide;
  sF32 QuakeAccelForw;
  sF32 QuakeAccelSide;
  sMatrix Cam;
	sF32 CamState[9];
	sF32 CamStateBase[9];

  sInt BitmapZoom;
  sInt BitmapX;
  sInt BitmapY;
  sInt BitmapTile;
//  sVector CamPos;
//  sVector CamAngle;
  sF32 CamZoom;
  sInt MeshWire;
  sU32 SelectMask;
  sU32 ClearColor[4];

  sBool PlayerMode;

  void ShowBitmap(sViewport &view,sInt tex);
  void ShowMesh(sViewport &view,class GenMesh *mesh);
  void ShowScene(sViewport &view,class GenScene *mesh);
	void ShowFXChain(sViewport &view,class GenFXChain *chain);
  PageOp *FindLinkEdit();
public:
  ViewWin();
  ~ViewWin();
  void Tag();
  sU32 GetClass() { return sCID_TOOL_VIEWWIN; }
  void OnPaint();
  void OnPaint3d(sViewport &view);
  void OnKey(sU32 key);
  void OnDrag(sDragData &);
  sBool OnCommand(sU32 cmd);

  void SetObject(ToolObject *);
  void SetPlayerMode(sBool pm) {PlayerMode = pm;}
  ToolObject *GetObject() { return Object; }
};

/****************************************************************************/

#endif
