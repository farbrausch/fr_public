/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_WZ4_MTRL2_HPP
#define FILE_WZ4FRLIB_WZ4_MTRL2_HPP

//#include "wz4frlib/wz4_mtrl2_ops.hpp"
#include "base/types.hpp"
#include "base/graphics.hpp"
#include "wz4lib/doc.hpp"
#include "wz4lib/basic_ops.hpp"
#include "wz4frlib/wz4_demo2.hpp"
#include "wz4frlib/wz4_mtrl2_shader.hpp"

/****************************************************************************/

#define WZ4ONLYONEGEO 0    // don't use seperate vertex buffers in zonly and main passes

#define sMAX_LIGHTENV 16

enum Wz4MtrlFlags
{
  sRF_TARGET_MAIN     = 0x0001,   // render to screen
  sRF_TARGET_ZONLY    = 0x0002,   // render to depth texture (through red)
  sRF_TARGET_ZNORMAL  = 0x0003,   // render to depth texture (through red)
  sRF_TARGET_WIRE     = 0x0004,   // wireframe
  sRF_TARGET_DIST     = 0x0005,   // render distance to depth texture (through red)
  sRF_TARGET_MASK     = 0x0007,

  sRF_MATRIX_ONE      = 0x0008,   // normal painting
  sRF_MATRIX_BONE     = 0x0010,   // provide weight & index in geometriy stream[0]
                                  // set vertex constants manually before call
  sRF_MATRIX_INSTANCE = 0x0018,   // provide matrices in geometry stream[1]
  sRF_MATRIX_BONEINST = 0x0020,   // bone + instances. for swarms of animated objects
  sRF_MATRIX_INSTPLUS = 0x0028,   // provide matrices + sVF_F4 vector in geometry stream[1]
  sRF_MATRIX_MASK     = 0x0038,

  sRF_TOTAL           = 0x003f,

  sRF_HINT_DIRSHADOW  = 0x0100,   // directional shadow map: used by mandelbulb culling
  sRF_HINT_POINTSHADOW= 0x0200,   // pointlight shadow map
};

/****************************************************************************/

class Wz4Mtrl : public wObject
{
public:
  Wz4Mtrl();
  virtual void BeforeFrame(sInt lightenv,sInt boxcount=0,const sAABBoxC *boxes=0,sInt matcount=0,const sMatrix34CM *mats=0) {}
  virtual void Prepare()=0;
  virtual sVertexFormatHandle *GetFormatHandle(sInt flags)=0;
  virtual void Set(sInt flags,sInt index,const sMatrix34CM *mat,sInt SkinMatCount,const sMatrix34CM *SkinMats,sInt *SkinMatMap)=0;
  virtual sBool SkipPhase(sInt flags,sInt lightenv)=0;

  virtual void Serialize(sReader &stream) { sFatal(L"no serialize for this material type yet"); }
  virtual void Serialize(sWriter &stream) { sFatal(L"no serialize for this material type yet"); }

  sString<64> Name;

  // extra parameters. might be used by some material implementations

  sF32 ShellExtrude;      // for shell of fur rendering.
};

/****************************************************************************/

class SimpleMtrl : public Wz4Mtrl
{
  class SimpleShader *Shaders[5];
  sVertexFormatHandle *Formats[5];
  Texture2D *Tex[3];
  sCBuffer<SimpleShaderVPara> cb;

  sInt Flags;
  sInt TFlags[3];
  sU32 Blend;
  sInt AlphaTest,AlphaRef;
  sInt Extras;
  //sMatrix34 TexDetailMatrix;
public:

  SimpleMtrl();
  ~SimpleMtrl();
  void Set(sInt flags,sInt index,const sMatrix34CM *mat,sInt SkinMatCount,const sMatrix34CM *SkinMats,sInt *SkinMatMap);
  void SetMtrl(sInt flags=sMTRL_ZON|sMTRL_CULLON|sMTRL_LIGHTING,sU32 blend=0,sInt extras=0);
  void SetTex(sInt stage,Texture2D *,sInt tflags = sMTF_TILE|sMTF_LEVEL2);
  void SetAlphaTest(sInt cmp,sInt ref);
//  void SetTexMatrix(sInt stage,const sMatrix34 &mat);

  sInt DetailTexSpace;            // bit 8..10 switch to lightenv-matrix
  sMatrix34 TexTrans[3];
  sF32 VertexScale;

  void Prepare();
  sVertexFormatHandle *GetFormatHandle(sInt flags);
  sBool SkipPhase(sInt flags,sInt lightenv);

  template <class streamer> void Serialize_(streamer &stream);
  virtual void Serialize(sReader &stream) { Serialize_(stream); }
  virtual void Serialize(sWriter &stream) { Serialize_(stream); }

};

/****************************************************************************/

class CustomMtrl : public Wz4Mtrl
{
  class CustomShader *Shader;
  sVertexFormatHandle *Format;
  Texture2D *Tex[sMTRL_MAXTEX];
  sCBuffer<CustomShaderVEnv> cbv;
  sCBuffer<CustomShaderPEnv> cbp;

  sInt Flags;
  sInt TFlags[sMTRL_MAXTEX];
  sU32 Blend;
  sInt AlphaTest,AlphaRef;

public:

  CustomMtrl();
  ~CustomMtrl();
  void Set(sInt flags,sInt index,const sMatrix34CM *mat,sInt SkinMatCount,const sMatrix34CM *SkinMats,sInt *SkinMatMap);
  void SetMtrl(sInt flags=sMTRL_ZON|sMTRL_CULLON|sMTRL_LIGHTING,sU32 blend=0);
  void SetTex(sInt stage,Texture2D *,sInt tflags = sMTF_TILE|sMTF_LEVEL2);
  void SetAlphaTest(sInt cmp,sInt ref);
  void SetShader(sShader *vs, sShader *ps);
  void Prepare();

  sShader *CompileShader(sInt shadertype, const sChar *source);

  sVertexFormatHandle *GetFormatHandle(sInt flags);
  sBool SkipPhase(sInt flags, sInt lightenv);

  sTextBuffer Log;

  sF32 vs_var0[4];
  sF32 vs_var1[4];
  sF32 vs_var2[4];
  sF32 vs_var3[4];
  sF32 vs_var4[4];

  sF32 ps_var0[4];
  sF32 ps_var1[4];
  sF32 ps_var2[4];
  sF32 ps_var3[4];
  sF32 ps_var4[4];
};

/****************************************************************************/

#endif // FILE_WZ4FRLIB_WZ4_MTRL2_HPP

