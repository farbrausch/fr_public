/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_WZ4_IPP_HPP
#define FILE_WZ4FRLIB_WZ4_IPP_HPP

#include "base/types.hpp"
#include "wz4frlib/wz4_ipp_ops.hpp"

/****************************************************************************/

enum IppHelper2Flags
{
  IPPH_BLURRY = 0x0001,
};

class IppHelper2
{
  sGeometry *GeoDouble;
  sMaterial *MtrlCopy;
  sMaterial *MtrlSampleLine9;
  sMaterial *MtrlSampleRect9;
  sMaterial *MtrlSampleRect16;
  sMaterial *MtrlSampleRect16AW;

  sF32 XYOffset;
  sF32 UVOffset;

  void BlurLine(sTexture2D *dest,sTexture2D *src,sF32 radius,sF32 radiusy,sF32 amp);
public:
  IppHelper2();
  ~IppHelper2();

  // get a projection matrix for pixel adressing.
  void GetTargetInfo(sMatrix44 &mvp);

  // draw a fullscreen quad with correct uv's for the given textures
  void DrawQuad(sTexture2D *dest,sTexture2D *s0=0,sTexture2D *s1=0,sU32 col=0xffffffff,sInt flags = 0);

  void BlurFirst(sTexture2D *dest,sTexture2D *src,sF32 radius,sF32 amp,sBool alphaWeighted);
  void BlurNext(sTexture2D *dest,sTexture2D *src,sF32 radius,sF32 amp);
  void Blur(sTexture2D *dest,sTexture2D *src,sF32 radius,sF32 amp);

  void Copy(sTexture2D *dest,sTexture2D *src);
};


/****************************************************************************/

class RNColorCorrect : public Wz4RenderNode
{
  sMaterial *Mtrl;
public:
  RNColorCorrect();
  ~RNColorCorrect();

  Wz4RenderParaColorCorrect Para,ParaBase;
  Wz4RenderAnimColorCorrect Anim;

  void Simulate(Wz4RenderContext *);
  void Render(Wz4RenderContext *);
};

/****************************************************************************/

class RNDebugZ : public Wz4RenderNode
{
  sMaterial *Mtrl;
public:
  RNDebugZ();
  ~RNDebugZ();
  void Init();

  Wz4RenderParaDebugZ Para,ParaBase;
  Wz4RenderAnimDebugZ Anim;

  void Simulate(Wz4RenderContext *);
  void Render(Wz4RenderContext *);
};

/****************************************************************************/

class RNBlur : public Wz4RenderNode
{
  sMaterial *Mtrl;
public:
  RNBlur();
  ~RNBlur();

  Wz4RenderParaBlur Para,ParaBase;
  Wz4RenderAnimBlur Anim;

  void Simulate(Wz4RenderContext *);
  void Render(Wz4RenderContext *);
};

/****************************************************************************/

class RNDof : public Wz4RenderNode
{
  sMaterial *MtrlCopy;
  sMaterial *MtrlSampleRect9;
  sMaterial *MtrlDof;
public:
  RNDof();
  ~RNDof();
  void Init();

  Wz4RenderParaDepthOfField Para,ParaBase;
  Wz4RenderAnimDepthOfField Anim;

  void Simulate(Wz4RenderContext *);
  void Render(Wz4RenderContext *);
};

/****************************************************************************/

class RNDof2 : public Wz4RenderNode
{
  sMaterial *MtrlDown;
  sMaterial *MtrlCoC;
  sMaterial *MtrlDof;

public:
  RNDof2();
  ~RNDof2();
  void Init();

  Wz4RenderParaDepthOfField2 Para,ParaBase;
  Wz4RenderAnimDepthOfField2 Anim;

  void Simulate(Wz4RenderContext *);
  void Render(Wz4RenderContext *);
};

/****************************************************************************/

class RNSSAO : public Wz4RenderNode
{
  sTexture2D *TexRandom;
  sMaterial *MtrlSSAO;
  sMaterial *MtrlFinish;
  sVector30 Randoms[32];

public:
  RNSSAO();
  ~RNSSAO();
  void Init();

  Wz4RenderParaSSAO Para,ParaBase;
  Wz4RenderAnimSSAO Anim;

  void Simulate(Wz4RenderContext *);
  void Render(Wz4RenderContext *);
};

/****************************************************************************/

class RNColorBalance : public Wz4RenderNode
{
  sMaterial *Mtrl;
public:
  RNColorBalance();
  ~RNColorBalance();

  Wz4RenderParaColorBalance Para,ParaBase;
  Wz4RenderAnimColorBalance Anim;

  void Simulate(Wz4RenderContext *);
  void Render(Wz4RenderContext *);
};

/****************************************************************************/

class RNColorSaw : public Wz4RenderNode
{
  sMaterial *Mtrl;
public:
  RNColorSaw();
  ~RNColorSaw();

  Wz4RenderParaColorSaw Para,ParaBase;
  Wz4RenderAnimColorSaw Anim;

  void Simulate(Wz4RenderContext *);
  void Render(Wz4RenderContext *);
};

/****************************************************************************/

class RNColorGrade : public Wz4RenderNode
{
  sMaterial *Mtrl;
public:
  RNColorGrade();
  ~RNColorGrade();

  Wz4RenderParaColorGrade Para,ParaBase;
  Wz4RenderAnimColorGrade Anim;

  void Simulate(Wz4RenderContext *);
  void Render(Wz4RenderContext *);

  sMatrix34 HueMatPre, HueMatPost;

  sMatrix34 GetContrastMatrix(sF32 contrast);
  sMatrix34 GetSaturationMatrix(sF32 saturation);
  sMatrix34 GetBalanceMatrix(sF32 hue, sF32 str);
};


/****************************************************************************/

class RNCrashZoom : public Wz4RenderNode
{
  sMaterial *Mtrl;
public:
  RNCrashZoom();
  ~RNCrashZoom();

  Wz4RenderParaCrashZoom Para,ParaBase;
  Wz4RenderAnimCrashZoom Anim;

  void Simulate(Wz4RenderContext *);
  void Render(Wz4RenderContext *);
};

/****************************************************************************/

class RNGrabScreen : public Wz4RenderNode
{
public:
  RNGrabScreen();
  ~RNGrabScreen();

  Rendertarget2D *Target;
  Wz4RenderParaGrabScreen Para,ParaBase;
  Wz4RenderAnimGrabScreen Anim;

  void Simulate(Wz4RenderContext *);
  void Render(Wz4RenderContext *);
};

/****************************************************************************/

class RNCrackFixer : public Wz4RenderNode
{
  sMaterial *Mtrl;
public:
  RNCrackFixer();
  ~RNCrackFixer();

  Wz4RenderParaCrackFixer Para,ParaBase;
  Wz4RenderAnimCrackFixer Anim;

  void Simulate(Wz4RenderContext *);
  void Render(Wz4RenderContext *);
};

/****************************************************************************/

class RNFXAA3 : public Wz4RenderNode
{
  sMaterial *MtrlPre, *MtrlAA;
public:
  RNFXAA3();
  ~RNFXAA3();

  Wz4RenderParaFXAA3 Para;

  void Render(Wz4RenderContext *);
};

/****************************************************************************/

class RNColorMath : public Wz4RenderNode
{
  sMaterial *Mtrl;
public:
  RNColorMath();
  ~RNColorMath();

  Wz4RenderParaColorMath Para;

  void Render(Wz4RenderContext *);
};

/****************************************************************************/

class RNCustomIPP : public Wz4RenderNode
{
public:
  RNCustomIPP();
  ~RNCustomIPP();
  void Init();
  sShader *CompileShader(sInt shadertype, const sChar *source);

  Wz4RenderParaCustomIPP Para,ParaBase;
  Wz4RenderAnimCustomIPP Anim;

  void Simulate(Wz4RenderContext *);
  void Render(Wz4RenderContext *);

  sMaterial *Mtrl;
  sTextBuffer Log;
};

/****************************************************************************/

#endif // FILE_WZ4FRLIB_WZ4_IPP_HPP

