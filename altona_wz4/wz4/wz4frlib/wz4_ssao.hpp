/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) by Dierk Ohlerich                                    ***/
/***   all rights reserved                                                ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_WZ4_SSAO_HPP
#define FILE_WZ4FRLIB_WZ4_SSAO_HPP

#include "base/types.hpp"
#include "base/types.hpp"
#include "wz4lib/doc.hpp"
#include "wz4lib/basic.hpp"
#include "wz4frlib/wz4_demo.hpp"
#include "wz4frlib/wz4_ssao_shader.hpp"

/****************************************************************************/


class Wz4SsaoCamera : public Wz4Viewport
{
public:
  Wz4SsaoCamera();
  ~Wz4SsaoCamera();
  sBool Paint(const sRect *window,sInt time,const sViewport *);

  Scene *Node;                    // link to scene
  Wz4Variable *AnimTarget;
  Wz4Variable *AnimPosition;
  Wz4Function *Clip;

  Wz4SSAODepthShader *DepthMaterial;
  Wz4SSAONormalShader *NormalMaterial;
  Wz4SSAOShader *SSAOMaterial;
  Wz4SSAOBlurShader *SSAOBlurMaterial;

  sCBuffer<Wz4SSAOShaderRandom> RandomCB;

  sVertexFormatHandle *FmtXYZW;
  sGeometry *QuadGeo;
  sTexture2D *RTDepth;
  sTexture2D *RTNormal;
  sTexture2D *RTSSAO;

  sTexture2D *RandomTex;

  sVector31 Target;
  sVector31 Position;
  sF32 Zoom;
  sF32 ClipNear;                  // z-clipping
  sF32 ClipFar;
  sU32 ClearColor;
  sInt Flags;

  sF32 MaxTime;                   // in beats 16:16
};

/****************************************************************************/

#endif // FILE_WZ4FRLIB_WZ4_SSAO_HPP

