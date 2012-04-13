// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef __APP_BABEWATCH_HPP__
#define __APP_BABEWATCH_HPP__

#include "_util.hpp"
#include "_gui.hpp"
#include "genmesh.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   windows                                                            ***/
/***                                                                      ***/
/****************************************************************************/

class sBabewatchApp : public sGuiWindow
{
  sCamera Camera;
  sMatrix Frame;
  sVector Rot;
  sF32 Time;
  sInt Texture;
  sU32 Material[256];
  sInt BoneIndex;
  sStatusBorder *Status;
  sChar Stat1[256];
  sChar Stat2[256];
  sChar Stat3[256];
  sChar Stat4[256];

  GenMesh *Mesh;
public:
  sBabewatchApp();
  ~sBabewatchApp();
  void Tag();
  void OnPaint3d(sViewport &vp);
  void OnKey(sU32 key);
  void OnDrag(sDragData &);
  void OnFrame();
};

/****************************************************************************/

#endif
