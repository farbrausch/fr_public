// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef __APP_CUBE_HPP__
#define __APP_CUBE_HPP__

#include "_util.hpp"
#include "_gui.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   windows                                                            ***/
/***                                                                      ***/
/****************************************************************************/

class sCubeApp : public sGuiWindow
{
  sCamera Camera;
  sMatrix Frame;
  sVector Rot;
  sF32 Time;
  sInt Texture;
  sU32 Material[256];

public:
  sCubeApp();
  ~sCubeApp();
  void Tag();
  void OnPaint3d(sViewport &vp);
  void OnKey(sU32 key);
  void OnDrag(sDragData &);
  void OnFrame();
};

/****************************************************************************/

#endif
