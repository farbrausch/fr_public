/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_WZ4_ROVEMTRL_HPP
#define FILE_WZ4FRLIB_WZ4_ROVEMTRL_HPP

#include "base/types.hpp"
#include "wz4frlib/wz4_rovemtrl.hpp"
#include "wz4frlib/wz4_mtrl2.hpp"
#include "wz4frlib/wz4_rovemtrl_shader.hpp"

class RoveShader;

/****************************************************************************/

class RoveMtrl : public Wz4Mtrl
{
  // all for 4 modes: normal/boned/instanced/boneinst
  RoveShader *Shaders[4];
  sVertexFormatHandle *Formats[4];
  Texture2D *Tex[2];
  sCBuffer<RoveShaderVPara> cb;

  sInt Flags;
  sInt TFlags[2];
  sInt Extras;
  //sMatrix34 TexDetailMatrix;

public:
  RoveMtrl();
  ~RoveMtrl();
  void Set(sInt flags,sInt index,const sMatrix34CM *mat,sInt SkinMatCount,const sMatrix34CM *SkinMats,sInt *SkinMatMap);
  void SetMtrl(sInt flags=sMTRL_ZON|sMTRL_CULLON,sInt extras=0);
  void SetTex(sInt stage,Texture2D *tex,sInt tflags=sMTF_TILE|sMTF_LEVEL2);

  sInt DetailTexSpace;
  sMatrix34 TexTrans[2];

  void Prepare();
  sVertexFormatHandle *GetFormatHandle(sInt flags);
  sBool SkipPhase(sInt flags,sInt lightenv);
};

/****************************************************************************/

#endif // FILE_WZ4FRLIB_WZ4_ROVEMTRL_HPP

