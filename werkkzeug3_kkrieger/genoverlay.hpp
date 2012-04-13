// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __GENOVERLAY_HPP__
#define __GENOVERLAY_HPP__

#include "_types.hpp"
#include "_start.hpp"
#include "material11.hpp"
#include "kdoc.hpp"
#include "genbitmap.hpp"
#include "genblobspline.hpp"

class sMaterialDirect;

/****************************************************************************/
/****************************************************************************/

#define GENOVER_RTSIZES       4   // different sizes for rt's
#define GENOVER_RTPERSIZE     3   // number of rt's per size
#define GENOVER_RTCOUNT       (GENOVER_RTSIZES*GENOVER_RTPERSIZE) // rendertargets

#define GENOVER_DEFAULT       0   // editor only: default material für scene-view
#define GENOVER_STENCILCLEAR  1   // clear the stencil buffer (with a large quad)
#define GENOVER_ADDDESTALPHA  2   // adds destination alpha (quad)
#define GENOVER_CLRDESTALPHA  3   // clear destination alpha (quad)
#define GENOVER_BLEND4X1      4   // blend4x pass 1
#define GENOVER_BLEND4X2      5   // blend4x pass 2
#define GENOVER_TEX1          6   // simple blit with one texture modulated by color0
#define GENOVER_SHARPEN       7   // sharpen
#define GENOVER_COLOR0        8   // color op 0 (add)
#define GENOVER_COLOR1        9   // color op 1 (addsmooth)
#define GENOVER_COLOR2        10  // color op 2 (sub)
#define GENOVER_COLOR3        11  // color op 3 (rsub)
#define GENOVER_COLOR4        12  // color op 4 (mul)
#define GENOVER_COLOR5        13  // color op 5 (mul2)
#define GENOVER_COLOR6        14  // color op 6 (blend)
#define GENOVER_COLOR7        15  // color op 7 (dp3)
#define GENOVER_MERGE0        16  // merge op 0 (add)
#define GENOVER_MERGE1        17  // merge op 1 (addsmooth)
#define GENOVER_MERGE2        18  // merge op 2 (sub)
#define GENOVER_MERGE3        19  // merge op 3 (rsub)
#define GENOVER_MERGE4        20  // merge op 4 (mul)
#define GENOVER_MERGE5        21  // merge op 5 (mul2)
#define GENOVER_MERGE6        22  // merge op 6 (blend)
#define GENOVER_MERGE7        23  // merge op 7 (dp3)
#define GENOVER_MERGE8        24  // merge op 8 (2nd alpha)
#define GENOVER_MASK1         25  // mask 1st pass
#define GENOVER_MASK2         26  // mask 2nd pass
#define GENOVER_COPY0         27  // copy op 0 (direct copy)
#define GENOVER_COPY1         28  // copy op 1 (add)
#define GENOVER_COPY2         29  // copy op 2 (mul)
#define GENOVER_COPY3         30  // copy op 3 (mul2)
#define GENOVER_COPY4         31  // copy op 4 (addsmooth)
#define GENOVER_COPY5         32  // copy op 5 (alpha)
#define GENOVER_BLEND3D0      33  // blend for anaglyphic 3d rendering (1)
#define GENOVER_BLEND3D1      34  // blend for anaglyphic 3d rendering (2)
#define GENOVER_MASKEDCLEAR   35  // stencil masked clear.
#define GENOVER_MAXMAT        36

#define RTSIZE_LARGE          0
#define RTSIZE_MEDIUM         1
#define RTSIZE_SMALL          2
#define RTSIZE_FULL           3
#define RTSIZE_SCREEN         4   // the screen itself


void GenOverlayInit();
void GenOverlayExit();

struct GenOverlayRT
{
  GenBitmap *Bitmap;
  KOp *Owner;
  sInt Size;
  sInt Refs;
  sF32 ScaleUV[2];
};

class GenOverlayManagerClass
{
  friend void GenOverlayInit();
  friend void GenOverlayExit();

  GenOverlayRT RT[GENOVER_RTCOUNT+1];
  sInt RefCount;                  // this is designed with multiple instances in mind. for the tool...
  sInt GeoDouble;                 // geometry with two uv-sets
  sInt GeoDoubleTri;
  sInt GeoTSpace;

  sViewport Master;               // master viewport (used for screen)

public:
  GenOverlayManagerClass();
  ~GenOverlayManagerClass();

  void Reset(KEnvironment *kenv);
  GenOverlayRT *Alloc(KOp *owner,sInt size,sInt ocount);
  GenOverlayRT *Get(sInt size,sInt id);
  GenOverlayRT *Find(KOp *owner);
  void Free(GenOverlayRT *);
  void PrepareViewport(GenOverlayRT *rt,sViewport &vp);
  void SetMasterViewport(sViewport &vp);
  void GetMasterViewport(sViewport &vp);
  void GrabScreen(GenOverlayRT *dest);

  void Quad(sF32 dxf=0,sF32 dyf=0);
  void Quad(sFRect *coord,sFRect *uv,sF32 z);
  void FXQuad(sMaterial *mtrl,sF32 *scale0=0,sFRect *uv0=0,sF32 *scale1=0,sFRect *uv1=0);
  void FXQuad(sInt mtrl,sF32 *scale0=0,sFRect *uv0=0,sF32 *scale1=0,sFRect *uv1=0)  { FXQuad(Mtrl[mtrl],scale0,uv0,scale1,uv1); }
  void Copy(GenOverlayRT *src,GenOverlayRT *dest,sInt mode,sU32 color,sF32 zoom);
  void Blend4x(GenOverlayRT *src,GenOverlayRT *dest,sFRect *rect,sInt cmode,sF32 amplify);
  GenOverlayRT *Blend4xOp(KOp *op,GenOverlayRT *src,sFRect *rect,sInt cmode,sF32 amplify,sInt size,sInt ocount);
  GenOverlayRT *BlurOp(KOp *op,GenOverlayRT *src,sInt size,sF32 radius,sF32 amplify,sInt flags,sInt stages,sInt ocount);

// special effect stuff that doesn't really belong here
  sMaterialDirect *JPEGMaterial;
  sInt JPEGKernel0;
  sInt JPEGKernel1;

// new stuff
  sInt CurrentShader;             // sPS_00 sPS_11 sPS_14 sPS20
  sInt EnableShadows;
  sF32 CullDistance;
  sF32 AutoLightRange;            // 8  
  GenOverlayRT *LastOutput;          // use this to pass result from last Exec_IPP_Xxx() (no refcounts, but Free(bm))
  sMaterial11 *Mtrl[GENOVER_MAXMAT];
  sMaterialDirect *HSCBMaterial;
  class GenMaterial *DefaultMat;
  sBool RealPaint;
#if !sPLAYER
  sInt SoundEnable;            
  sInt LinkEdit;
  sInt FreeCamera;
#endif

#ifdef _DOPE
  sInt EnableIPP;
  sInt DontRender;
  sInt ForceResolution;
#endif

  struct KKriegerGame *Game;
  KEnvironment *KEnv;
};

extern GenOverlayManagerClass *GenOverlayManager;

/****************************************************************************/
/****************************************************************************/

class GenIPP : public KObject
{
public:
  GenIPP();
  ~GenIPP();
};

GenIPP * __stdcall Init_IPP_Viewport(GenScene *scene,GenSpline *,sInt size,sInt flags,sU32 color,sF323 rot,sF323 pos,sF32 farclip,sF32 nearclip,sF32 centerx,sF32 centery,sF32 zoomx,sF32 zoomy,sU32 fogc,sF32 fogend,sF32 fogst,sF32 eyed,sF32 focal,sF32 fx0,sF32 fy0,sF32 fx1,sF32 fy1,sInt ocount);
GenIPP * __stdcall Init_IPP_Blur(GenIPP *in,sInt size,sF32 radius,sF32 amplify,sInt flags,sInt stages,sInt ocount);
GenIPP * __stdcall Init_IPP_Copy(GenIPP *in,sInt size,sU32 color,sF32 zoom,sInt ocount);
GenIPP * __stdcall Init_IPP_Crashzoom(GenIPP *in,sInt size,sInt steps,sF32 zoom,sF32 amplify,sF322 center,sInt ocount);
GenIPP * __stdcall Init_IPP_Sharpen(GenIPP *in,sInt size,sF32 radius,sU32 amplify,sInt stages,sInt ocount);
GenIPP * __stdcall Init_IPP_Color(GenIPP *in,sInt size,sU32 color,sInt operation,sU32 amplify,sInt ocount);
GenIPP * __stdcall Init_IPP_Merge(GenIPP *in,GenIPP *in2,sInt size,sInt operation,sInt alpha,sU32 amplify,sInt ocount);
GenIPP * __stdcall Init_IPP_Mask(GenIPP *in,GenIPP *in2,GenIPP *in3,sInt size,sInt ocount);
GenIPP * __stdcall Init_IPP_Layer2D(GenIPP *in,GenMaterial *mtrl,sInt size,sF324 screen,sF324 uv,sF32 z,sInt clear,sInt ocount);
GenIPP * __stdcall Init_IPP_Select(sInt count,GenIPP *in,...);
GenIPP * __stdcall Init_IPP_RenderTarget(GenIPP *in,GenBitmap *rt);
GenIPP * __stdcall Init_IPP_HSCB(GenIPP *in,sInt size,sF32 hue,sF32 sat,sF32 con,sF32 bright,sInt ocount);
GenIPP * __stdcall Init_IPP_JPEG(GenIPP *in,sInt size,sInt dir,sF32 strength,sInt ocount);

void __stdcall Exec_IPP_Viewport(KOp *op,KEnvironment *kenv,sInt size,sInt flags,sU32 color,sF323 rot,sF323 pos,sF32 farclip,sF32 nearclip,sF32 centerx,sF32 centery,sF32 zoomx,sF32 zoomy,sU32 fogc,sF32 fogend,sF32 fogst,sF32 eyed,sF32 focal,sF32 fx0,sF32 fy0,sF32 fx1,sF32 fy1,sInt ocount);
void __stdcall Exec_IPP_Blur(KOp *op,KEnvironment *kenv,sInt size,sF32 radius,sF32 amplify,sInt flags,sInt stages,sInt ocount);
void __stdcall Exec_IPP_Copy(KOp *op,KEnvironment *kenv,sInt size,sU32 color,sF32 zoom,sInt ocount);
void __stdcall Exec_IPP_Crashzoom(KOp *op,KEnvironment *kenv,sInt size,sInt steps,sF32 zoom,sF32 amplify,sF322 center,sInt ocount);
void __stdcall Exec_IPP_Sharpen(KOp *op,KEnvironment *kenv,sInt size,sF32 radius,sU32 amplify,sInt stages,sInt ocount);
void __stdcall Exec_IPP_Color(KOp *op,KEnvironment *kenv,sInt size,sU32 color,sInt operation,sU32 amplify,sInt ocount);
void __stdcall Exec_IPP_Merge(KOp *op,KEnvironment *kenv,sInt size,sInt operation,sInt alpha,sU32 amplify,sInt ocount);
void __stdcall Exec_IPP_Mask(KOp *op,KEnvironment *kenv,sInt size,sInt ocount);
void __stdcall Exec_IPP_Layer2D(KOp *op,KEnvironment *kenv,sInt size,sFRect oxy,sFRect ouv,sF32 z,sInt clear,sInt ocount);
void __stdcall Exec_IPP_Select(KOp *op,KEnvironment *kenv,sInt count);
void __stdcall Exec_IPP_RenderTarget(KOp *op,KEnvironment *kenv);
void __stdcall Exec_IPP_HSCB(KOp *op,KEnvironment *kenv,sInt size,sF32 hue,sF32 sat,sF32 con,sF32 bright,sInt ocount);
void __stdcall Exec_IPP_JPEG(KOp *op,KEnvironment *kenv,sInt size,sInt dir,sF32 strength,sInt ocount);

/****************************************************************************/

#endif
