// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef __GENFX_HPP__
#define __GENFX_HPP__

#include "_types.hpp"
#include "_start.hpp"

/****************************************************************************/

class ScriptRuntime;
class GenFXChain;
class GenScene;
class GenMaterial;
class GenMatrix;

/****************************************************************************/

#define FXCHAIN_MAXIN 4
#define FXCHAIN_MAXPARAM 16

class GenFXChain : public sObject
{
public:
	GenFXChain();
	sU32 GetClass() { return sCID_GENFXCHAIN; }
	void Tag();
#if !sINTRO
	void Copy(sObject *);
#endif
	void RenderR(sInt id);
	void Render();

	sInt4 Color;
	sF322 Zoom;
	sInt Rotate;
	sF322 Center;
  sF323 CamPos;
  sF323 CamRot;

	GenFXChain *Input[FXCHAIN_MAXIN];
	sObject *Object;
	sInt Param[FXCHAIN_MAXPARAM];
  sF32 ParamF[FXCHAIN_MAXPARAM];
	sInt RenderHandler;

	sInt Viewport;
  sInt RenderID;
};

/****************************************************************************/

sObject * __stdcall FXChain_New(sInt size,sInt ocount,sInt4 color);
sObject * __stdcall FXChain_BeginViewport(GenFXChain *chn,sInt3 pos,sInt3 parm2,sInt aspect,sInt mode,sInt ocount,sInt4 color,sInt zoom);
void __stdcall FXChain_Render(GenFXChain *chain);
sObject * __stdcall FXChain_Layer2D(GenFXChain *chn,GenMaterial *mtrl,sInt4 color,sInt4 screen,sInt4 uv,sInt z,sInt clear,sInt ocount,sF322 zoom,sInt rotate,sF322 center);
sObject * __stdcall FXChain_Merge(GenFXChain *c1,GenFXChain *c2,sInt mode,sInt param,sF322 zoom,sInt rotate,sF322 center,sInt size,sInt ocount);
sObject * __stdcall FXChain_Color(GenFXChain *chn,sInt mode,sInt4 color,sInt size,sInt ocount,sF322 zoom,sInt rotate,sF322 center,sInt lighten);
sObject * __stdcall FXChain_EndViewport(GenFXChain *chn,sInt ocount);
sObject * __stdcall FXChain_PaintScene(GenFXChain *chn,GenScene *scn,sInt ocount);
sObject * __stdcall FXChain_Blend4x(GenFXChain *chn,sF324 r0,sF324 r1,sF324 r2,sF324 r3,sInt cmode,sInt size,sInt ocount,sInt amplify);
sObject * __stdcall FXChain_Viewport(GenFXChain *chn,GenScene *,sInt3 pos,sInt3 parm2,sInt aspect,sInt mode,sInt ocount,sInt4 color,sInt zoom);
void __stdcall FXChain_ResetAlloc();
sObject * __stdcall FXChain_Label(GenFXChain *chn,sInt label,sInt ocount);

/****************************************************************************/

struct FXViewport : public sViewport
{
	sInt RefCount;
	sInt Size;
	sF32 XScale;
	sF32 YScale;

	void InitFrom(const sViewport &vp);
	void InitTex(sInt handle,sInt size);
};

/****************************************************************************/

class FXMaster_
{
public:
	FXMaster_();
	~FXMaster_();

	void CreateRendertargets(const sInt *count);
	
	void SetMasterViewport(const sViewport &vp);
#if !sINTRO
	void UpdateTempViewport(sInt handle,sBitmap *bmp);
#endif

	sInt Alloc(sInt ocount,sInt size);
	void Ref(sInt handle,sInt delta);
	void ResetAlloc();

  FXViewport *Viewport;
  sInt ViewportCount;
};

extern FXMaster_ *FXMaster;

void InitFXMaster();
void CloseFXMaster();

/****************************************************************************/

#endif
