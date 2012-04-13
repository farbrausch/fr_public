// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __RTMANAGER_HPP__
#define __RTMANAGER_HPP__

#include "_types.hpp"

/****************************************************************************/

// Render target IDs should be unique and statically assigned!
// top 16 bits = components, lower 16 bits = free for use
//
// currently assigned components:
//   0x0000 = renderer
//   0x0001 = material 2.0
//   0x0002 = genoverlaymanager

class RenderTargetManager_
{
  struct Target
  {
    sU32 Id;
    sInt XRes,YRes;
    sInt TexXRes,TexYRes;
    sInt Handle;
  };

  void GetRealResolution(const Target *tgt,sInt &xRes,sInt &yRes) const;
  void ResizeInternal(Target *tgt,sInt xRes,sInt yRes);
  
  sArray<Target> Targets;
  sViewport MasterVP;

public:
  RenderTargetManager_();
  ~RenderTargetManager_();

  void GetMasterViewport(sViewport &vp);
  void SetMasterViewport(const sViewport &vp);

  void Add(sU32 id,sInt xRes,sInt yRes);
  void Resize(sU32 id,sInt xRes,sInt yRes);
  sInt Get(sU32 id,sF32 &uScale,sF32 &vScale) const;
  void SetAsTarget(sU32 id) const;

  void GrabToTarget(sU32 targetId,const sRect *window=0) const;
};

extern RenderTargetManager_ *RenderTargetManager;

/****************************************************************************/

#endif
