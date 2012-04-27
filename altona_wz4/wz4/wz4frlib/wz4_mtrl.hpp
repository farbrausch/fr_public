/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WERKKZEUG4_WZ4_MTRL_HPP
#define FILE_WERKKZEUG4_WZ4_MTRL_HPP

#include "base/types.hpp"
#include "base/graphics.hpp"
#include "wz4frlib/wz4_mtrl_shader.hpp"
#include "wz4lib/doc.hpp"

/****************************************************************************/

struct Wz4ShaderEnv
{
  sMatrix44 ModelScreen;          // model -> screen matrix
  sMatrix34 Model;
  sMatrix34 ModelView;
  sMatrix34 ModelI;               // model -> world matrix, inverted
  sMatrix34 Camera;               // camera -> world matrix
  sMatrix44 ShadowMat;
  sF32 FocalRangeI;               // maxz for DOF effect, inverted
  sVector4 FadeColor;
  sF32 ClipFar;
  sF32 ClipNear;
  sF32 ZoomX;
  sF32 ZoomY;

  void Set(const sViewport *vp,sF32 focalrange);
  void Set(const sViewport *vp,const sMatrix34 &mat);
  void SetShadow(const sViewport *vp);
  sInt Visible(const sAABBox &box) const;
};

class Wz4Shader : public sMaterial
{
public:
  Wz4Shader();
  void SelectShaders(sVertexFormatHandle *);
//  void Set(sMaterialEnv *env);
  void SetUV(Wz4ShaderUV *cb,const sViewport *vp);
  void SetCam(Wz4ShaderCamera *cb,const sViewport *vp);
  void SetUV(Wz4ShaderUV *cb,const Wz4ShaderEnv *env);
  void SetCam(Wz4ShaderCamera *cb,const Wz4ShaderEnv *env);

  void MakeMatrix(sInt id,sInt flags,sF32 scale,sF32 *m0,sF32 *m1);
  sVector4 UVMatrix0[2];
  sVector4 UVMatrix1[2];
  sMatrix34 UVMatrix2;
  sInt DetailMode;
};

class Wz4Material : public wObject
{
public:
  Wz4Shader *Material;
  wObject *Tex[8];
  sVertexFormatHandle *Format;
  sString<64> Name;
  Wz4Material *TempPtr;

  Wz4Material();
  ~Wz4Material();
  void CopyFrom(Wz4Material *);
  template <class streamer> void Serialize_(streamer &stream,sTexture2D *shadow=0);
  void Serialize(sWriter &stream,sTexture2D *shadow=0);
  void Serialize(sReader &stream,sTexture2D *shadow=0);
};

/****************************************************************************/

#endif // FILE_WERKKZEUG4_WZ4_MTRL_HPP

