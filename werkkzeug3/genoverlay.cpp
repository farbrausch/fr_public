// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "genoverlay.hpp"
#include "genmesh.hpp"
#include "genbitmap.hpp"
#include "genmaterial.hpp"
#include "genscene.hpp"
#include "genblobspline.hpp"
#include "engine.hpp"
#include "_start.hpp"
#include "kkriegergame.hpp"
#include "materialdirect.hpp"
#include "rtmanager.hpp"
#include "shadercompile.hpp"
#include "_startdx.hpp"

#if sPROFILE
#include "_util.hpp"
#endif

#define sDUMP_IPPALLOC 0
#define sIPP_JPEG      !sINTRO

#define SUPPORTMASK   (!sINTRO)
#define STEREO3D      (!sINTRO)

#if sDUMP_IPPALLOC
static sInt OverlayMaxAlloc[GENOVER_RTSIZES] = { 0 };
extern "C" void __stdcall OutputDebugStringA(const sChar *msg);
extern "C" void __stdcall wsprintfA(sChar *str,const sChar *format,...);
#endif

/****************************************************************************/

#if sIPP_JPEG

#include "effect_jpegps.hpp"
#include "effect_jpegvs.hpp"

static const sU32 EffectJPEGStates[] = {
  sD3DRS_ALPHATESTENABLE,               0,
  sD3DRS_ZENABLE,                       sD3DZB_TRUE,
  sD3DRS_ZWRITEENABLE,                  0,
  sD3DRS_ZFUNC,                         sD3DCMP_ALWAYS,
  sD3DRS_CULLMODE,                      sD3DCULL_NONE,
  sD3DRS_COLORWRITEENABLE,              15,
  sD3DRS_SLOPESCALEDEPTHBIAS,           0,
  sD3DRS_DEPTHBIAS,                     0,
  sD3DRS_FOGENABLE,                     0,
  sD3DRS_STENCILENABLE,                 0,
  sD3DRS_ALPHABLENDENABLE,              0,

  sD3DSAMP_0|sD3DSAMP_MAGFILTER,        sD3DTEXF_LINEAR,
  sD3DSAMP_0|sD3DSAMP_MINFILTER,        sD3DTEXF_LINEAR,
  sD3DSAMP_0|sD3DSAMP_MIPFILTER,        sD3DTEXF_NONE,
  sD3DSAMP_0|sD3DSAMP_ADDRESSU,         sD3DTADDRESS_CLAMP,
  sD3DSAMP_0|sD3DSAMP_ADDRESSV,         sD3DTADDRESS_CLAMP,

  sD3DSAMP_1|sD3DSAMP_MAGFILTER,        sD3DTEXF_LINEAR,
  sD3DSAMP_1|sD3DSAMP_MINFILTER,        sD3DTEXF_LINEAR,
  sD3DSAMP_1|sD3DSAMP_MIPFILTER,        sD3DTEXF_NONE,
  sD3DSAMP_1|sD3DSAMP_ADDRESSU,         sD3DTADDRESS_CLAMP,
  sD3DSAMP_1|sD3DSAMP_ADDRESSV,         sD3DTADDRESS_CLAMP,

  sD3DSAMP_2|sD3DSAMP_MAGFILTER,        sD3DTEXF_LINEAR,
  sD3DSAMP_2|sD3DSAMP_MINFILTER,        sD3DTEXF_LINEAR,
  sD3DSAMP_2|sD3DSAMP_MIPFILTER,        sD3DTEXF_NONE,
  sD3DSAMP_2|sD3DSAMP_ADDRESSU,         sD3DTADDRESS_CLAMP,
  sD3DSAMP_2|sD3DSAMP_ADDRESSV,         sD3DTADDRESS_CLAMP,

  sD3DSAMP_3|sD3DSAMP_MAGFILTER,        sD3DTEXF_LINEAR,
  sD3DSAMP_3|sD3DSAMP_MINFILTER,        sD3DTEXF_LINEAR,
  sD3DSAMP_3|sD3DSAMP_MIPFILTER,        sD3DTEXF_NONE,
  sD3DSAMP_3|sD3DSAMP_ADDRESSU,         sD3DTADDRESS_CLAMP,
  sD3DSAMP_3|sD3DSAMP_ADDRESSV,         sD3DTADDRESS_CLAMP,

  sD3DSAMP_4|sD3DSAMP_MAGFILTER,        sD3DTEXF_POINT,
  sD3DSAMP_4|sD3DSAMP_MINFILTER,        sD3DTEXF_POINT,
  sD3DSAMP_4|sD3DSAMP_MIPFILTER,        sD3DTEXF_NONE,
  sD3DSAMP_4|sD3DSAMP_ADDRESSU,         sD3DTADDRESS_WRAP,
  sD3DSAMP_4|sD3DSAMP_ADDRESSV,         sD3DTADDRESS_WRAP,

  sD3DSAMP_5|sD3DSAMP_MAGFILTER,        sD3DTEXF_POINT,
  sD3DSAMP_5|sD3DSAMP_MINFILTER,        sD3DTEXF_POINT,
  sD3DSAMP_5|sD3DSAMP_MIPFILTER,        sD3DTEXF_NONE,
  sD3DSAMP_5|sD3DSAMP_ADDRESSU,         sD3DTADDRESS_WRAP,
  sD3DSAMP_5|sD3DSAMP_ADDRESSV,         sD3DTADDRESS_WRAP,
  
  ~0U,                                  ~0U,
};

#endif

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   IPP Management                                                     ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

void GenOverlayInit() 
{ 
  if(GenOverlayManager) 
    GenOverlayManager->RefCount++; 
  else 
    GenOverlayManager = new GenOverlayManagerClass; 
#if !sRELEASE
  sDPrintF("overinit %d\n",GenOverlayManager->RefCount);
#endif
}

void GenOverlayExit() 
{ 
  sVERIFY(GenOverlayManager);
  sVERIFY(GenOverlayManager->RefCount>0);

#if !sRELEASE
  sDPrintF("overexit %d\n",GenOverlayManager->RefCount);
#endif
  GenOverlayManager->RefCount--;
  if(GenOverlayManager->RefCount==0)
    delete GenOverlayManager;
}

GenOverlayManagerClass *GenOverlayManager;

/****************************************************************************/

#if !sPLAYER

static sMaterialDirect *DirectMaterial(const sChar *vs,const sChar *ps,
  const sU32 *states,const sU32 *codegenData)
{
  sMaterialDirect *shader = 0;
  sChar *vsSource = sSystem->LoadText(vs);
  sChar *psSource = sSystem->LoadText(ps);
  
  if(vsSource && psSource)
  {
    sShaderCompiler compile;
    sText *errors = new sText;
    
    if(compile.Compile(vsSource,vs,errors))
    {
      const sU32 *vsData = compile.GetShaderCopy();
      const sU32 *psData = 0;
      errors->Clear();

      if(compile.Compile(psSource,ps,errors))
      {
        psData = compile.GetShaderCopy();
        shader = new sMaterialDirect(vsData,psData,states,codegenData);
      }
      else
        sDPrintF("DirectMaterial: ps compilation errors:\n%s",errors->Text);

      delete[] vsData;
      delete[] psData;
    }
    else
      sDPrintF("DirectMaterial: vs compilation errors:\n%s",errors->Text);

    delete errors;
  }
  else
    sDPrintF("DirectMaterial: sources not found\n");

  delete[] vsSource;
  delete[] psSource;
  return shader;
}

static const sU32 HSCBStates[] = {
  sD3DRS_ALPHATESTENABLE,         0,
  sD3DRS_ZWRITEENABLE,            0,
  sD3DRS_ZFUNC,                   sD3DCMP_ALWAYS,
  sD3DRS_CULLMODE,                sD3DCULL_NONE,
  sD3DRS_COLORWRITEENABLE,        15,
  sD3DRS_FOGENABLE,               0,
  sD3DRS_STENCILENABLE,           0,
  sD3DRS_ALPHABLENDENABLE,        0,
  sD3DSAMP_0|sD3DSAMP_MAGFILTER,  sD3DTEXF_LINEAR,
  sD3DSAMP_0|sD3DSAMP_MINFILTER,  sD3DTEXF_LINEAR,
  sD3DSAMP_0|sD3DSAMP_MIPFILTER,  sD3DTEXF_LINEAR,
  sD3DSAMP_0|sD3DSAMP_ADDRESSU,   sD3DTADDRESS_WRAP,
  sD3DSAMP_0|sD3DSAMP_ADDRESSV,   sD3DTADDRESS_WRAP,
  ~0U,                            ~0U,
};

#endif

/****************************************************************************/

GenOverlayManagerClass::GenOverlayManagerClass()
{
#if !sPLAYER
  sScreenInfo si;
#endif
  sInt i;
  GenOverlayRT *rtd;
#if GENOVER_RTSIZES
  static sInt sizes[GENOVER_RTSIZES][3] = 
  {
    { 10,9,sTF_A8R8G8B8 },
    //{  4,4,sTF_A8R8G8B8 },
    {  9,8,sTF_A8R8G8B8 },
    {  8,7,sTF_A8R8G8B8 },
    //{  4,4,sTF_A8R8G8B8 },
    { 10,9,sTF_A8R8G8B8 },
  };
#endif
  static sInt colorcomb[] =
  {
    sMCOA_ADD, sMCOA_SMOOTH, sMCOA_RSUB, sMCOA_SUB,
    sMCOA_MUL, sMCOA_MUL2, sMCA_COL0|sMCOA_LERP, sMCOA_DP3
  };

#if !sPLAYER
  sSystem->GetScreenInfo(0,si);
  CurrentShader = si.ShaderLevel;
#else
  CurrentShader = sPS_11;
#endif

  sSetMem(Mtrl,0,sizeof(Mtrl));

#if !sINTRO
  if(CurrentShader>=sPS_11)
  {
    Mtrl[GENOVER_DEFAULT] = new sMaterial11;
    Mtrl[GENOVER_DEFAULT]->ShaderLevel = sPS_11;
    Mtrl[GENOVER_DEFAULT]->BaseFlags = sMBF_ZON;
    Mtrl[GENOVER_DEFAULT]->LightFlags = sMLF_BUMPX;
    Mtrl[GENOVER_DEFAULT]->Color[0] = 0x00c0c0c0;
    Mtrl[GENOVER_DEFAULT]->Color[1] = 0x00404040;
    Mtrl[GENOVER_DEFAULT]->SpecPower = 32.0f;
    Mtrl[GENOVER_DEFAULT]->Combiner[sMCS_LIGHT] = sMCOA_SET;
    Mtrl[GENOVER_DEFAULT]->Combiner[sMCS_COLOR0] = sMCOA_MUL;
  }
  else
  {
    Mtrl[GENOVER_DEFAULT] = new sMaterial11;
    Mtrl[GENOVER_DEFAULT]->ShaderLevel = sPS_00;
    Mtrl[GENOVER_DEFAULT]->BaseFlags = sMBF_ZON|sMBF_NONORMAL;
    Mtrl[GENOVER_DEFAULT]->Color[0] = 0x00c0c0c0;
    Mtrl[GENOVER_DEFAULT]->Combiner[sMCS_VERTEX] = sMCOA_SET;
  }
#else
  Mtrl[GENOVER_DEFAULT] = new sMaterial11;
  Mtrl[GENOVER_DEFAULT]->ShaderLevel = sPS_11;
  Mtrl[GENOVER_DEFAULT]->BaseFlags = sMBF_ZON;
  Mtrl[GENOVER_DEFAULT]->Color[0] = 0x00ff0000;
  Mtrl[GENOVER_DEFAULT]->Combiner[sMCS_COLOR0] = sMCOA_SET;
#endif

  Mtrl[GENOVER_STENCILCLEAR] = new sMaterial11;
  Mtrl[GENOVER_STENCILCLEAR]->ShaderLevel = sPS_11;
  Mtrl[GENOVER_STENCILCLEAR]->BaseFlags = sMBF_ZONLY|sMBF_NOTEXTURE|sMBF_NONORMAL|sMBF_DOUBLESIDED|sMBF_STENCILCLR;
  Mtrl[GENOVER_STENCILCLEAR]->Color[0] = 0xffffffff;
  Mtrl[GENOVER_STENCILCLEAR]->Combiner[sMCS_COLOR0] = sMCOA_SET;
  Mtrl[GENOVER_STENCILCLEAR]->AlphaCombiner = sMCA_COL0;

  Mtrl[GENOVER_ADDDESTALPHA] = new sMaterial11;
  Mtrl[GENOVER_ADDDESTALPHA]->ShaderLevel = sPS_11;
  Mtrl[GENOVER_ADDDESTALPHA]->BaseFlags = sMBF_ZOFF|sMBF_BLENDADDDA|sMBF_NOTEXTURE|sMBF_NONORMAL|sMBF_DOUBLESIDED;
  Mtrl[GENOVER_ADDDESTALPHA]->Color[0] = 0x00ffffff;
  Mtrl[GENOVER_ADDDESTALPHA]->Combiner[sMCS_COLOR0] = sMCOA_SET;
  Mtrl[GENOVER_ADDDESTALPHA]->AlphaCombiner = sMCA_COL0;

  Mtrl[GENOVER_CLRDESTALPHA] = new sMaterial11;
  Mtrl[GENOVER_CLRDESTALPHA]->CopyFrom(Mtrl[GENOVER_ADDDESTALPHA]);
  Mtrl[GENOVER_CLRDESTALPHA]->BaseFlags = sMBF_ZOFF|sMBF_BLENDMUL|sMBF_NOTEXTURE|sMBF_NONORMAL|sMBF_DOUBLESIDED;

  Mtrl[GENOVER_BLEND4X1] = new sMaterial11;
  Mtrl[GENOVER_BLEND4X1]->ShaderLevel = sPS_11;
  Mtrl[GENOVER_BLEND4X1]->BaseFlags = sMBF_NONORMAL|sMBF_DOUBLESIDED|sMBF_ZOFF;
  Mtrl[GENOVER_BLEND4X1]->Combiner[sMCS_TEX0] = sMCA_COL0|sMCOA_SET;
  Mtrl[GENOVER_BLEND4X1]->Combiner[sMCS_TEX1] = sMCA_COL0|sMCOA_ADD;
  Mtrl[GENOVER_BLEND4X1]->AlphaCombiner = sMCA_MIX01 << 4;
  Mtrl[GENOVER_BLEND4X1]->TFlags[0] = sMTF_UV0|sMTF_CLAMP|sMTF_FILTER;
  Mtrl[GENOVER_BLEND4X1]->TFlags[1] = sMTF_UV1|sMTF_CLAMP|sMTF_FILTER;
  Mtrl[GENOVER_BLEND4X1]->SetTex(0,0);
  Mtrl[GENOVER_BLEND4X1]->SetTex(1,0);

  Mtrl[GENOVER_BLEND4X2] = new sMaterial11;
  Mtrl[GENOVER_BLEND4X2]->CopyFrom(Mtrl[GENOVER_BLEND4X1]);
  Mtrl[GENOVER_BLEND4X2]->BaseFlags |= sMBF_BLENDADD;

  Mtrl[GENOVER_TEX1] = new sMaterial11;
  Mtrl[GENOVER_TEX1]->ShaderLevel = sPS_11;
  Mtrl[GENOVER_TEX1]->SetTex(0,0);
  Mtrl[GENOVER_TEX1]->BaseFlags = sMBF_ZOFF|sMBF_BLENDOFF|sMBF_NONORMAL|sMBF_DOUBLESIDED;
  Mtrl[GENOVER_TEX1]->Color[0] = 0x00ffffff;
  Mtrl[GENOVER_TEX1]->Combiner[sMCS_COLOR0] = sMCOA_SET;
  Mtrl[GENOVER_TEX1]->Combiner[sMCS_TEX0] = sMCOA_MUL;
  Mtrl[GENOVER_TEX1]->AlphaCombiner = sMCA_TEX0 | (sMCA_COL0 << 4);
  Mtrl[GENOVER_TEX1]->TFlags[0] = sMTF_UV0|sMTF_CLAMP|sMTF_FILTER;

  Mtrl[GENOVER_SHARPEN] = new sMaterial11;
#if !sINTRO
  Mtrl[GENOVER_SHARPEN]->ShaderLevel = sPS_11;
  Mtrl[GENOVER_SHARPEN]->BaseFlags = sMBF_NONORMAL|sMBF_DOUBLESIDED|sMBF_ZOFF;
  Mtrl[GENOVER_SHARPEN]->Combiner[sMCS_TEX0] = sMCOA_SET/*|sMCOB_SET*/;
  Mtrl[GENOVER_SHARPEN]->Combiner[sMCS_TEX1] = sMCOA_SUB;
  Mtrl[GENOVER_SHARPEN]->Combiner[sMCS_COLOR2] = sMCOA_MUL8;
  //Mtrl[GENOVER_SHARPEN]->Combiner[sMCS_FINAL] = sMCOA_ADD;
  Mtrl[GENOVER_SHARPEN]->TFlags[0] = sMTF_UV0|sMTF_CLAMP|sMTF_FILTER;
  Mtrl[GENOVER_SHARPEN]->TFlags[1] = sMTF_UV1|sMTF_CLAMP|sMTF_FILTER;
  Mtrl[GENOVER_SHARPEN]->SetTex(0,0);
  Mtrl[GENOVER_SHARPEN]->SetTex(1,0);
#else
  Mtrl[GENOVER_SHARPEN] = new sMaterial11;
  Mtrl[GENOVER_SHARPEN]->CopyFrom(Mtrl[GENOVER_TEX1]);
#endif

  for(i=0;i<8;i++)
  {
    Mtrl[GENOVER_COLOR0+i] = new sMaterial11;
    Mtrl[GENOVER_COLOR0+i]->ShaderLevel = sPS_11;
    Mtrl[GENOVER_COLOR0+i]->BaseFlags = sMBF_NONORMAL|sMBF_DOUBLESIDED|sMBF_ZOFF;
    Mtrl[GENOVER_COLOR0+i]->Combiner[sMCS_COLOR0] = sMCOA_SET;
    Mtrl[GENOVER_COLOR0+i]->Combiner[sMCS_TEX0] = colorcomb[i];
    Mtrl[GENOVER_COLOR0+i]->Combiner[sMCS_COLOR2] = sMCOA_MUL4;
    Mtrl[GENOVER_COLOR0+i]->TFlags[0] = sMTF_UV0|sMTF_CLAMP|sMTF_FILTER;
    Mtrl[GENOVER_COLOR0+i]->SetTex(0,0);

    Mtrl[GENOVER_MERGE0+i] = new sMaterial11;
    Mtrl[GENOVER_MERGE0+i]->CopyFrom(Mtrl[GENOVER_COLOR0+i]);
    Mtrl[GENOVER_MERGE0+i]->Combiner[sMCS_COLOR0] = sMCOA_NOP;
    Mtrl[GENOVER_MERGE0+i]->Combiner[sMCS_TEX0] = sMCOA_SET;
    Mtrl[GENOVER_MERGE0+i]->Combiner[sMCS_TEX1] = colorcomb[i];
    Mtrl[GENOVER_MERGE0+i]->TFlags[1] = sMTF_UV1|sMTF_CLAMP|sMTF_FILTER;
    Mtrl[GENOVER_MERGE0+i]->SetTex(1,0);
  }

  Mtrl[GENOVER_MERGE8] = new sMaterial11;
  Mtrl[GENOVER_MERGE8]->CopyFrom(Mtrl[GENOVER_MERGE0]);
  Mtrl[GENOVER_MERGE8]->Combiner[sMCS_TEX1] = sMCA_TEX1|sMCA_INVERTA|sMCOA_LERP;

#if SUPPORTMASK
  Mtrl[GENOVER_MASK1] = new sMaterial11;
  Mtrl[GENOVER_MASK1]->ShaderLevel = sPS_11;
  Mtrl[GENOVER_MASK1]->BaseFlags = sMBF_NONORMAL|sMBF_DOUBLESIDED|sMBF_ZOFF;
  Mtrl[GENOVER_MASK1]->Combiner[sMCS_COLOR0] = sMCOA_SET;
  Mtrl[GENOVER_MASK1]->Combiner[sMCS_TEX0] = sMCOA_DP3;
  Mtrl[GENOVER_MASK1]->Combiner[sMCS_TEX1] = sMCOA_MUL;
  Mtrl[GENOVER_MASK1]->Color[0] = 0x408040;
  Mtrl[GENOVER_MASK1]->TFlags[0] = sMTF_UV0|sMTF_CLAMP|sMTF_FILTER;
  Mtrl[GENOVER_MASK1]->TFlags[1] = sMTF_UV1|sMTF_CLAMP|sMTF_FILTER;
  Mtrl[GENOVER_MASK1]->SetTex(0,0);
  Mtrl[GENOVER_MASK1]->SetTex(1,0);

  Mtrl[GENOVER_MASK2] = new sMaterial11;
  Mtrl[GENOVER_MASK2]->ShaderLevel = sPS_11;
  Mtrl[GENOVER_MASK2]->BaseFlags = sMBF_NONORMAL|sMBF_DOUBLESIDED|sMBF_ZOFF|sMBF_BLENDADD;
  Mtrl[GENOVER_MASK2]->Combiner[sMCS_COLOR0] = sMCOA_SET;
  Mtrl[GENOVER_MASK2]->Combiner[sMCS_TEX0] = sMCOA_DP3;
  Mtrl[GENOVER_MASK2]->Combiner[sMCS_COLOR2] = sMCOA_RSUB;
  Mtrl[GENOVER_MASK2]->Combiner[sMCS_TEX2] = sMCOA_MUL;
  Mtrl[GENOVER_MASK2]->Color[0] = 0x408040;
  Mtrl[GENOVER_MASK2]->Color[2] = 0xffffff;
  Mtrl[GENOVER_MASK2]->TFlags[0] = sMTF_UV0|sMTF_CLAMP|sMTF_FILTER;
  Mtrl[GENOVER_MASK2]->TFlags[2] = sMTF_UV1|sMTF_CLAMP|sMTF_FILTER;
  Mtrl[GENOVER_MASK2]->SetTex(0,0);
  Mtrl[GENOVER_MASK2]->SetTex(2,0);
#endif

  for(i=0;i<6;i++)
  {
    Mtrl[GENOVER_COPY0+i] = new sMaterial11;
    Mtrl[GENOVER_COPY0+i]->CopyFrom(Mtrl[GENOVER_TEX1]);
  }

  Mtrl[GENOVER_COPY1]->BaseFlags |= sMBF_BLENDADD;
  Mtrl[GENOVER_COPY2]->BaseFlags |= sMBF_BLENDMUL;
  Mtrl[GENOVER_COPY3]->BaseFlags |= sMBF_BLENDMUL2;
  Mtrl[GENOVER_COPY4]->BaseFlags |= sMBF_BLENDSMOOTH;
  Mtrl[GENOVER_COPY5]->BaseFlags |= sMBF_BLENDALPHA;

#if STEREO3D
  Mtrl[GENOVER_BLEND3D0] = new sMaterial11;
  Mtrl[GENOVER_BLEND3D0]->ShaderLevel = sPS_11;
  Mtrl[GENOVER_BLEND3D0]->BaseFlags = sMBF_NONORMAL|sMBF_NOTEXTURE|sMBF_DOUBLESIDED|sMBF_ZOFF|sMBF_BLENDMUL;
  Mtrl[GENOVER_BLEND3D0]->Combiner[sMCS_COLOR0] = sMCOA_SET;
  Mtrl[GENOVER_BLEND3D0]->Color[0] = 0x0000ffff;

  Mtrl[GENOVER_BLEND3D1] = new sMaterial11;
  Mtrl[GENOVER_BLEND3D1]->ShaderLevel = sPS_11;
  Mtrl[GENOVER_BLEND3D1]->BaseFlags = sMBF_NONORMAL|sMBF_DOUBLESIDED|sMBF_ZOFF|sMBF_BLENDADD;
  Mtrl[GENOVER_BLEND3D1]->Combiner[sMCS_COLOR0] = sMCOA_SET;
  Mtrl[GENOVER_BLEND3D1]->Combiner[sMCS_TEX0] = sMCOA_MUL;
  Mtrl[GENOVER_BLEND3D1]->Color[0] = 0xffff0000;
  Mtrl[GENOVER_BLEND3D1]->SetTex(0,0);
#endif

  Mtrl[GENOVER_MASKEDCLEAR] = new sMaterial11;
  Mtrl[GENOVER_MASKEDCLEAR]->ShaderLevel = sPS_11;
  Mtrl[GENOVER_MASKEDCLEAR]->BaseFlags = sMBF_NONORMAL|sMBF_NOTEXTURE|sMBF_DOUBLESIDED|sMBF_ZOFF|sMBF_STENCILRENDER;
  Mtrl[GENOVER_MASKEDCLEAR]->Combiner[sMCS_COLOR0] = sMCOA_SET;
  Mtrl[GENOVER_MASKEDCLEAR]->Color[0] = 0x00000000;

  DefaultMat = new GenMaterial;
  DefaultMat->AddPass(Mtrl[GENOVER_DEFAULT],ENGU_LIGHT,MPP_STATIC);
  Mtrl[GENOVER_DEFAULT]->AddRef();

  for(i=0;i<GENOVER_MAXMAT;i++)
  {
    sMaterial11 *mtrl11 = (sMaterial11 *) Mtrl[i];
    if(!mtrl11)
      continue;

    if(mtrl11->ShaderLevel>CurrentShader)
    {
      mtrl11->ShaderLevel = sPS_00;
      sSetMem(mtrl11->Combiner,0,sizeof(mtrl11->Combiner));
      mtrl11->Combiner[sMCS_VERTEX] = sMCOA_SET;
    }
    sBool ok = mtrl11->Compile();
#if sUSE_SHADERS
    if(CurrentShader>sPS_00)
    {
      if(mtrl11->ShaderLevel<sPS_20)
        sVERIFY(ok);
    }
#endif
  }

  sU32 data[1] = { 1 };
#if sLINK_MTRL20 && sIPP_JPEG
  JPEGMaterial = new sMaterialDirect(g_effect_jpeg_vsh,g_effect_jpeg_psh,EffectJPEGStates,data);
#else
  JPEGMaterial = 0;
#endif
  GlareMaterial = 0;
  Glare2Material = 0;
  Glare3Material = 0;
  ColorCorrectMaterial = 0;

#if !sPLAYER
  HSCBMaterial = DirectMaterial("../gtg/tex1.vsh","../gtg/hscb.psh",HSCBStates,data);
#endif

  EnableShadows = 0;
  CullDistance = 0;
  AutoLightRange = 32;
#if !sPLAYER
  LinkEdit = 0;
  FreeCamera = 0;
  SoundEnable = 1;
#endif

  rtd = RT;
#if GENOVER_RTSIZES
  sInt j;
  for(i=0;i<GENOVER_RTSIZES;i++)
  {
    for(j=0;j<GENOVER_RTPERSIZE;j++)
    {
#if !sPLAYER
      rtd->Bitmap = Bitmap_RenderTarget(sizes[i][0]-si.LowQuality,sizes[i][1]-si.LowQuality,si.ShaderLevel>=sPS_20 ? sizes[i][2] : sTF_A8R8G8B8);
#else
      rtd->Bitmap = Bitmap_RenderTarget(sizes[i][0],sizes[i][1],CurrentShader>=sPS_20 ? sizes[i][2] : sTF_A8R8G8B8);
#endif
      rtd->Owner = 0;
      rtd->Size = i;
      rtd->Refs = 0;
      rtd->ScaleUV[0] = 1.0f;
      rtd->ScaleUV[1] = 1.0f;
      rtd++;
    }
  }
#endif

  // setup screen rendertarget
  rtd->Bitmap = 0;
  rtd->Size = GENOVER_RTSIZES;
  rtd->Refs = 0;
  rtd->Owner = 0;

  GeoDouble = sSystem->GeoAdd(sFVF_DOUBLE,sGEO_QUAD);
  GeoDoubleTri = sSystem->GeoAdd(sFVF_DOUBLE,sGEO_TRI);
  GeoTSpace = sSystem->GeoAdd(sFVF_TSPACE3,sGEO_QUAD);
  Master.Init();

  RefCount = 1;
  RealPaint = sFALSE;
  LastOutput = 0;
  Game = 0;

#if sIPP_JPEG && sLINK_MTRL20
  sU16 imgData[16*4];
  memset(imgData,0,sizeof(imgData));

  JPEGKernel0 = sSystem->AddTexture(4,4,sTF_A8R8G8B8,imgData,1);
  JPEGKernel1 = sSystem->AddTexture(4,4,sTF_A8R8G8B8,imgData,1);
#endif

#ifdef _DOPE
  EnableIPP = sTRUE;
  DontRender = 0;
  ForceResolution = 3;
#endif
}

GenOverlayManagerClass::~GenOverlayManagerClass()
{
  sInt i;

  DefaultMat->Release();

  for(i=0;i<GENOVER_RTCOUNT;i++)
    if(RT[i].Bitmap)
      RT[i].Bitmap->Release();
  LastOutput = 0;

#if sDUMP_IPPALLOC
  for(i=0;i<GENOVER_RTSIZES;i++)
  {
    sChar buffer[64];
    wsprintfA(buffer,"maxalloc[%d] = %d\n",i,OverlayMaxAlloc[i]);
    OutputDebugStringA(buffer);
  }
#endif

#if sIPP_JPEG && sLINK_MTRL20
  sRelease(JPEGMaterial);
  sSystem->RemTexture(JPEGKernel0);
  sSystem->RemTexture(JPEGKernel1);
#endif

#if !sPLAYER
  sRelease(HSCBMaterial);
#endif

  sRelease(ColorCorrectMaterial);
  sRelease(GlareMaterial);
  sRelease(Glare2Material);
  sRelease(Glare3Material);

  sSystem->GeoRem(GeoDouble);
  sSystem->GeoRem(GeoTSpace);

  for(i=0;i<GENOVER_MAXMAT;i++)
    sRelease(Mtrl[i]);
}

/****************************************************************************/
/****************************************************************************/

void GenOverlayManagerClass::Reset(KEnvironment *kenv)
{
  for(sInt i=0;i<GENOVER_RTCOUNT;i++)
    RT[i].Refs = 0;

  sMaterialEnv env;
  env.Init();

  Mtrl[GENOVER_BLEND4X1]->SetTex(0,sINVALID);
  Mtrl[GENOVER_BLEND4X1]->SetTex(0,sINVALID);
  if(CurrentShader>=sPS_11)
    Mtrl[GENOVER_BLEND4X1]->Set(env);
  LastOutput = 0;

  KEnv = kenv;
}

GenOverlayRT *GenOverlayManagerClass::Get(sInt size,sInt id)
{
  if(size>=GENOVER_RTSIZES)
    return &RT[GENOVER_RTCOUNT];
  sVERIFY(RT[size*GENOVER_RTPERSIZE+id].Size == size);
  return &RT[size*GENOVER_RTPERSIZE+id];
}

GenOverlayRT *GenOverlayManagerClass::Alloc(KOp *owner,sInt size,sInt ocount)
{
  sInt i;

#ifdef _DOPE
  if(size<GENOVER_RTSIZES && ForceResolution != 3)
    size = ForceResolution;
#endif

  for(i=0;i<GENOVER_RTCOUNT;i++)
  {
    if(!RT[i].Refs && RT[i].Size==size)
    {
      RT[i].Refs = ocount;
      RT[i].Owner = owner;

#if sDUMP_IPPALLOC
      sInt szalloc = 0;
      for(sInt j=0;j<GENOVER_RTCOUNT;j++)
        if(RT[j].Refs && RT[j].Size == size)
          szalloc++;

      OverlayMaxAlloc[size] = sMax(OverlayMaxAlloc[size],szalloc);
#endif

      return &RT[i];
    }
  }

  return &RT[GENOVER_RTCOUNT]; // screen rt
}

GenOverlayRT *GenOverlayManagerClass::Find(KOp *owner)
{
  sInt i;

  if(owner)
  {
    for(i=0;i<GENOVER_RTCOUNT;i++)
      if(RT[i].Refs && RT[i].Owner == owner)
        return &RT[i];
  }

  return 0;
}

void GenOverlayManagerClass::Free(GenOverlayRT *rt)
{
  if(rt->Size < GENOVER_RTSIZES) // screens can't be freed
  {
#if !sPLAYER
    if(rt->Refs <= 0)
      sFatal("GenOverlayManager::Free() not allocated!");
#endif

    rt->Refs--;
  }
}

void GenOverlayManagerClass::PrepareViewport(GenOverlayRT *rt,sViewport &vp)
{
  if(rt->Size < GENOVER_RTSIZES) // texture
  {
    vp.InitTexMS(rt->Bitmap->Texture);
    
    if(rt->Size == GENOVER_RTSIZES - 1) // full size rendertarget?
    {
      // yes, use 1:1 pixel mapping window
      vp.Window.x1 = sMin(Master.Window.XSize(),1024);
      vp.Window.y1 = sMin(Master.Window.YSize(),512);
    }
  }
  else
    vp = Master;
}

void GenOverlayManagerClass::SetMasterViewport(sViewport &vp)
{
  Master = vp;

  // recompute scale for full-size rendertargets
  sF32 scaleu = sMin(1.0f * vp.Window.XSize() / 1024,1.0f);
  sF32 scalev = sMin(1.0f * vp.Window.YSize() / 512,1.0f);

  for(sInt i=0;i<GENOVER_RTCOUNT;i++)
  {
    if(RT[i].Size == GENOVER_RTSIZES - 1) // full size rendertarget?
    {
      RT[i].ScaleUV[0] = scaleu;
      RT[i].ScaleUV[1] = scalev;
    }
  }
}

void GenOverlayManagerClass::GetMasterViewport(sViewport &vp)
{
  vp = Master;
}

/****************************************************************************/
/***                                                                      ***/
/***   IPP Basic Operations                                               ***/
/***                                                                      ***/
/****************************************************************************/

void GenOverlayManagerClass::Quad(sF32 dxf,sF32 dyf)
{
  sVertexDouble *vd;
  sSystem->GeoBegin(GeoDouble,4,0,(sF32 **)&vd,0);
  vd[0].Init(-1-dxf,-1+dyf,0.5,0, 0.0f,0.0f,0,0);
  vd[1].Init( 1-dxf,-1+dyf,0.5,0, 1.0f,0.0f,0,0);
  vd[2].Init( 1-dxf, 1+dyf,0.5,0, 1.0f,1.0f,0,0);
  vd[3].Init(-1-dxf, 1+dyf,0.5,0, 0.0f,1.0f,0,0);
  sSystem->GeoEnd(GeoDouble);
  sSystem->GeoDraw(GeoDouble);
}

void GenOverlayManagerClass::Quad(sFRect *coord,sFRect *uv,sF32 z)
{
  sVertexTSpace3 *vd;
  sSystem->GeoBegin(GeoTSpace,4,0,(sF32 **)&vd,0);
  sSystem->GeoEnd(GeoTSpace);
  vd[0].Init(coord->x0*2-1, 1-coord->y0*2,z,0, uv->x0,uv->y0);
  vd[1].Init(coord->x1*2-1, 1-coord->y0*2,z,0, uv->x1,uv->y0);
  vd[2].Init(coord->x1*2-1, 1-coord->y1*2,z,0, uv->x1,uv->y1);
  vd[3].Init(coord->x0*2-1, 1-coord->y1*2,z,0, uv->x0,uv->y1);
  sSystem->GeoDraw(GeoTSpace);
}

void GenOverlayManagerClass::FXQuad(sMaterial *mtrl,sF32 *scale0,sFRect *uv0,sF32 *scale1,sFRect *uv1)
{
  sInt i,j;
  sMaterialEnv env;
  sF32 *vp;
  static sFRect full = { 0.0f, 0.0f, 1.0f, 1.0f };
  static sF32 noscale[] = { 1.0, 1.0f };

  if(!scale0) scale0 = noscale;
  if(!uv0)    uv0 = &full;
  if(!scale1) scale1 = noscale;
  if(!uv1)    uv1 = &full;

  env.Init();
  env.Orthogonal = sMEO_NORMALISED;
  //env.Orthogonal = sMEO_PIXELS;
  sSystem->SetViewProject(&env);
  mtrl->Set(env);

  sSystem->GeoBegin(GeoDouble,4,0,&vp,0);
  for(i=0;i<4;i++)
  {
    j = i ^ (i >> 1);
    /**vp++ = (j & 1) ? sSystem->ViewportX : 0;
    *vp++ = (j & 2) ? sSystem->ViewportY : 0;*/
    *vp++ = (j & 1) ? 1.0f : -1.0f;
    *vp++ = (j & 2) ? -1.0f : 1.0f;
    *vp++ = 1.0f;
    *vp++ = 0.0f;
    *vp++ = ((j & 1) ? uv0->x1 : uv0->x0) * scale0[0];
    *vp++ = ((j & 2) ? uv0->y1 : uv0->y0) * scale0[1];
    *vp++ = ((j & 1) ? uv1->x1 : uv1->x0) * scale1[0];
    *vp++ = ((j & 2) ? uv1->y1 : uv1->y0) * scale1[1];
  }
  sSystem->GeoEnd(GeoDouble);
  sSystem->GeoDraw(GeoDouble);
}

void GenOverlayManagerClass::GrabScreen(GenOverlayRT *dest)
{
  sSystem->GrabScreen(Master.RenderTarget,Master.Window,dest->Bitmap->Texture);
}

void GenOverlayManagerClass::Copy(GenOverlayRT *src,GenOverlayRT *dest,sInt mode,sU32 color,sF32 zoom)
{
  sFRect uv;
  sViewport view;

  if(!src->Bitmap)
    return;

  PrepareViewport(dest,view);
  sSystem->SetViewport(view);

  // render a quad
  zoom *= 0.5f;
  uv.Init(0.5f - zoom,0.5f - zoom,0.5f + zoom,0.5f + zoom);
  Mtrl[GENOVER_COPY0+mode]->SetTex(0,src->Bitmap->Texture);
  Mtrl[GENOVER_COPY0+mode]->Color[0] = color;
  FXQuad(GENOVER_COPY0+mode,src->ScaleUV,&uv);
}

void GenOverlayManagerClass::Blend4x(GenOverlayRT *in,GenOverlayRT *out,sFRect *rect,sInt cmode,sF32 amplify)
{
  sViewport view;
  sInt i;

  if(!in->Bitmap)
    return;

  GenOverlayManager->PrepareViewport(out,view);
  sSystem->SetViewport(view);

  if(cmode)
  {
    for(i=0;i<8;i++)
    {
      (&rect[0].x0)[i*2] = (&rect[0].x0)[i*2] / in->Bitmap->XSize + (i&1);
      (&rect[0].y0)[i*2] = (&rect[0].y0)[i*2] / in->Bitmap->YSize + (i&1);
    }
  }

  for(i=0;i<2;i++)
  {
    Mtrl[GENOVER_BLEND4X1+i]->SetTex(0,in->Bitmap->Texture);
    Mtrl[GENOVER_BLEND4X1+i]->SetTex(1,in->Bitmap->Texture);
    Mtrl[GENOVER_BLEND4X1+i]->Color[0] = sRange<sInt>(amplify*64,255,0)<<24;
    FXQuad(GENOVER_BLEND4X1+i,in->ScaleUV,&rect[i*2+0],in->ScaleUV,&rect[i*2+1]);
  }
}

GenOverlayRT *GenOverlayManagerClass::Blend4xOp(KOp *op,GenOverlayRT *src,sFRect *rect,sInt cmode,sF32 amplify,sInt size,sInt ocount)
{
  GenOverlayRT *rt;

  if(!src)
    return 0;

  if(size == GENOVER_RTSIZES + 1)
    size = src->Size;

  rt = Alloc(op,size,ocount);
  Blend4x(src,rt,rect,cmode,amplify);
  Free(src);

  return rt;
}

GenOverlayRT *GenOverlayManagerClass::BlurOp(KOp *op,GenOverlayRT *in,sInt size,sF32 radius,sF32 amplify,sInt flags,sInt stages,sInt ocount)
{
  sFRect rc[4];
  static sInt kernel[] = {
    -1,-1,  1,-1, -1,1, 1,1, // square
     0,-1, -1, 0,  1,0, 0,1, // diamond
  };
  sInt i,count;
  GenOverlayRT *find;

  find = GenOverlayManager->Find(op);
  if(find)
    return find;

  count = 0;
  do
  {
    radius *= 0.5f;
    count++;
  }
  while(count<stages && radius>0.5f);

  while(count--)
  {
    for(i=0;i<4;i++)
    {
      rc[i].x0 = rc[i].x1 = kernel[(flags&1)*8+i*2+0] * radius;
      rc[i].y0 = rc[i].y1 = kernel[(flags&1)*8+i*2+1] * radius * 0.5f;
    }

    if(count)
      in = Blend4xOp(0,in,rc,1,(flags&2)?amplify:1.0f,(size == GENOVER_RTSIZES) ? 0 : size,1);
    else
      in = Blend4xOp(op,in,rc,1,amplify,size,ocount);
    radius *= 2.0f;
  }

  return in;
}

/****************************************************************************/
/***                                                                      ***/
/***   IPP Implementation                                                 ***/
/***                                                                      ***/
/****************************************************************************/

GenIPP::GenIPP()
{
  ClassId = KC_IPP;
}

GenIPP::~GenIPP()
{
}

/****************************************************************************/

GenIPP * __stdcall Init_IPP_Viewport(GenScene *scene,GenSpline *spline,sInt size,sInt flags,sU32 color,sF323 rot,sF323 pos,sF32 farclip,sF32 nearclip,sF32 centerx,sF32 centery,sF32 zoomx,sF32 zoomy,sU32 fogc,sF32 fogend,sF32 fogst,sF32 eyed,sF32 focal,sF32 fx0,sF32 fy0,sF32 fx1,sF32 fy1)
{
  if(scene) scene->Release();
  if(spline) spline->Release();
  return new GenIPP;
}

GenIPP * __stdcall Init_IPP_Blur(GenIPP *in,sInt size,sF32 radius,sF32 amplify,sInt flags,sInt stages)
{
  if(in) in->Release();
  return new GenIPP;
}

GenIPP * __stdcall Init_IPP_Copy(GenIPP *in,sInt size,sU32 color,sF32 zoom)
{
  if(in) in->Release();
  return new GenIPP;
}

GenIPP * __stdcall Init_IPP_Crashzoom(GenIPP *in,sInt size,sInt steps,sF32 zoom,sF32 amplify,sF322 center)
{
  if(in) in->Release();
  return new GenIPP;
}

GenIPP * __stdcall Init_IPP_Sharpen(GenIPP *in,sInt size,sF32 radius,sU32 amplify,sInt stages)
{
  if(in) in->Release();
  return new GenIPP;
}

GenIPP * __stdcall Init_IPP_Color(GenIPP *in,sInt size,sU32 color,sInt operation,sU32 amplify)
{
  if(in) in->Release();
  return new GenIPP;
}

GenIPP * __stdcall Init_IPP_Merge(GenIPP *in,GenIPP *in2,sInt size,sInt operation,sInt alpha,sU32 amplify)
{
  if(in) in->Release();
  if(in2) in2->Release();
  return new GenIPP;
}

GenIPP * __stdcall Init_IPP_Mask(GenIPP *in,GenIPP *in2,GenIPP *in3,sInt size)
{
  if(in) in->Release();
  if(in2) in2->Release();
  if(in3) in3->Release();
  return new GenIPP;
}

GenIPP * __stdcall Init_IPP_Layer2D(GenIPP *in,GenMaterial *mtrl,sInt size,sF324 screen,sF324 uv,sF32 z,sInt clear)
{
  if(!mtrl || mtrl->ClassId != KC_MATERIAL) return 0;
  if(in) in->Release();
  if(mtrl) mtrl->Release();
  return new GenIPP;
}

GenIPP * __stdcall Init_IPP_Select(sInt count,GenIPP *in,...)
{
  GenIPP **inp;
  sInt i;
  inp = &in;
  for(i=0;i<count;i++)
    inp[i]->Release();
  return new GenIPP;
}

GenIPP * __stdcall Init_IPP_RenderTarget(GenIPP *in,GenBitmap *rt)
{
  if(!rt || rt->Texture == sINVALID) return 0;

  if(in) in->Release();
  //if(rt) rt->Release();
  return new GenIPP;
}

GenIPP * __stdcall Init_IPP_HSCB(GenIPP *in,sInt size,sF32 hue,sF32 sat,sF32 con,sF32 bright)
{
  if(in) in->Release();
  return new GenIPP;
}

GenIPP * __stdcall Init_IPP_JPEG(GenIPP *in,sInt size,sInt dir,sF32 strength)
{
  if(in) in->Release();
  return new GenIPP;
}

/****************************************************************************/

extern sBool IntroStereo3D;

void __stdcall Exec_IPP_Viewport(KOp *parent,KEnvironment *kenv,sInt size,sInt flags,sU32 color,sF323 rot,sF323 pos,sF32 farclip,sF32 nearclip,sF32 centerx,sF32 centery,sF32 zoomx,sF32 zoomy,sU32 fogc,sF32 fogend,sF32 fogst,sF32 eyed,sF32 focal,sF32 fx0,sF32 fy0,sF32 fx1,sF32 fy1)
{
  KOp *op;
  GenOverlayRT *rt;
  sViewport view;
  sMaterialEnv env;
  sMatrix mat;

#ifdef _DOPE
  if(GenOverlayManager->DontRender)
    return;
#endif

  sInt ocount = parent->GetOutputCount();

  op = parent->GetInput(0);
  if(op)
  {
    rt = GenOverlayManager->Find(parent);
    if(!rt)
    {
      Engine->StartFrame();

      // CurrentCam is the way to tell camera details

      env.Init();

      env.FarClip = farclip;
      env.NearClip = nearclip;
      env.CenterX = centerx;
      env.CenterY = centery;
      env.ZoomX = zoomx;
      env.ZoomY = zoomy;

#if !sPLAYER
      if(flags & 1)
      {
        kenv->CameraDataRot.Init4(rot.x,rot.y,rot.z,0);
        kenv->CameraDataPos.Init4(pos.x,pos.y,pos.z,1);
        kenv->CameraDataZoom = zoomx;
      }
      if(GenOverlayManager->FreeCamera)
#else
      if(0)
#endif
      {
        env.CameraSpace = kenv->GameCam.CameraSpace;
        env.ZoomX = env.ZoomY = kenv->GameCam.ZoomX;//*kenv->Aspect;
      }
      else if(flags&4)
      {
        env.CameraSpace = kenv->GameCam.CameraSpace;
        if((flags & 0x300)==0)
        {
          env.CenterX     = kenv->GameCam.CenterX;
          env.CenterY     = kenv->GameCam.CenterY;
          env.ZoomX       = kenv->GameCam.ZoomX;
          env.ZoomY       = kenv->GameCam.ZoomY;
          env.FarClip     = kenv->GameCam.FarClip;
          env.NearClip    = kenv->GameCam.NearClip;
        }
      }
      else
      {
        env.CameraSpace.InitEulerPI2(&rot.x);
        env.CameraSpace.l.Init3(pos.x,pos.y,pos.z);
      }

      switch(flags & 0x300)
      {
      case 0:
        break;
      case 0x100:
        mat.Init();

        mat = env.CameraSpace;
        mat.j.Init(0,1,0);
        mat.i.Cross3(mat.j,mat.k);
        mat.i.Unit3();
        mat.k.Cross3(mat.i,mat.j);
        mat.l.AddScale3(mat.k,pos.z);

        mat.l = mat.k;
        mat.l.Scale3(pos.z);
        mat.l.w = 1;

        env.CameraSpace = mat;
        break;
      case 0x200:
        mat = env.CameraSpace;
        mat.l = mat.k;
        mat.l.Scale3(pos.z);
        mat.l.w = 1;
        env.CameraSpace = mat;
        break;
      }

      env.CameraSpace.l.w = 1;
      env.FogColor = fogc;
      env.FogStart = fogst;
      env.FogEnd = fogend;

      if(!(flags & 0x1000))
        env.ZoomY *= kenv->Aspect;

      KOp *spo = parent->GetInput(1);
      if(spo)
      {
        sF32 zoom;
        sMatrix mat;
        ((GenSpline *)spo->Cache)->Eval(kenv->Var[KV_TIME].x,0,mat,zoom);
#if !sPLAYER
        if(flags & 1)
        {
          sF32 a,b,c;
          mat.FindEuler(a,b,c);
          kenv->CameraDataRot.Init4(a/sPI2F,b/sPI2F,c/sPI2F,0);
          kenv->CameraDataPos.Init4(mat.l.x,mat.l.y,mat.l.z,1);
          kenv->CameraDataZoom = zoomx;
        }
        if(!GenOverlayManager->FreeCamera)
#endif
        {
          env.CameraSpace = mat;
          env.ZoomX *= zoom;
          env.ZoomY *= zoom;
        }
      }

      kenv->CurrentCam = env;

      sF32 off3d = 0.0f;
      sVector offShift,baseCam;
      sInt nPasses = 1;

      offShift.Init(0,0,0,0);
      baseCam = kenv->CurrentCam.CameraSpace.l;

#if STEREO3D
      if(IntroStereo3D && (flags & 0x40)) // stereo 3d mode (first render left)
      {
        off3d = 1.0f * eyed / focal;
        offShift = kenv->CurrentCam.CameraSpace.i;
        offShift.UnitSafe3();
        flags |= 3;

        nPasses = 2;
        
        // allocate fullscreen rendertarget for 3d rendering (the first time
        // we need it)
        RenderTargetManager->Add(0x00020000,0,0);
      }
#endif

      // set viewport for culling etc.
      Engine->SetViewProject(kenv->CurrentCam);

      op->Exec(kenv);
#ifdef _DOPE
      if(!GenOverlayManager->EnableIPP && GenOverlayManager->ForceResolution == 3)
        rt = GenOverlayManager->Alloc(parent,GENOVER_RTSIZES,ocount);
      else
#endif
      sViewport oldVp;

      rt = GenOverlayManager->Alloc(parent,size,ocount);
      GenOverlayManager->PrepareViewport(rt,view);
      sRect r;
      r.x0 = view.Window.x0 + view.Window.XSize() * fx0;
      r.y0 = view.Window.y0 + view.Window.YSize() * fy0;
      r.x1 = view.Window.x0 + view.Window.XSize() * fx1;
      r.y1 = view.Window.y0 + view.Window.YSize() * fy1;
      if(r.x0<r.x1 && r.y0<r.y1)
        view.Window = r;
      sSystem->SetViewport(view);
      RenderTargetManager->GetMasterViewport(oldVp);
      RenderTargetManager->SetMasterViewport(view);

      for(sInt pass=0;pass<nPasses;pass++)
      {
        sF32 sign = pass ? 1.0f : -1.0f;

        sSystem->Clear(flags & 3,color);
        kenv->CurrentCam.CenterX = centerx - sign * off3d;
        kenv->CurrentCam.CameraSpace.l = baseCam;
        kenv->CurrentCam.CameraSpace.l.AddScale3(offShift,sign * eyed * 0.5f);

        Engine->SetViewProject(kenv->CurrentCam);
        Engine->ProcessPortals(kenv,GenOverlayManager->Game ? GenOverlayManager->Game->PlayerCell : 0);
        
#if !sINTRO || 1
        // HACK: default-light to make theta work properly
        if(!Engine->GetNumLightJobs())
        {
          // add default light
          EngLight light;
          light.Type = sLT_POINT;
          light.Position = kenv->CurrentCam.CameraSpace.l;
          light.Direction.Init(0,0,1);
          light.Flags = 0;
          light.Color = 0x78787878;
          light.Amplify = 1.0f;
          light.Range = 32.0f;
          light.Event = 0;
          light.Id = 0;
          Engine->AddLightJob(light);
        }
#endif

        switch((flags >> 4) & 3) // paint mode
        {
        case 0: // normal
          Engine->Paint(kenv);
          break;

        case 1: // simple
          Engine->PaintSimple(kenv);
          break;

        case 2: // no specular
          Engine->Paint(kenv,sFALSE);
          break;
        }

#if STEREO3D
        if(flags & 0x40)
        {
          switch(pass)
          {
          case 0:
            // copy to intermediate render texture
            RenderTargetManager->GrabToTarget(0x00020000);
            break;

          case 1:
            {
              sF32 scale[2];
              sInt tex;
                
              // blend together
              tex = RenderTargetManager->Get(0x00020000,scale[0],scale[1]);
              GenOverlayManager->FXQuad(GENOVER_BLEND3D0);
              GenOverlayManager->Mtrl[GENOVER_BLEND3D1]->SetTex(0,tex);
              GenOverlayManager->FXQuad(GENOVER_BLEND3D1,scale);
            }
            break;
          }
        }
#endif
      }

      RenderTargetManager->SetMasterViewport(oldVp);
    }
    GenOverlayManager->LastOutput = rt;
  }
}

void __stdcall Exec_IPP_Copy(KOp *parent,KEnvironment *kenv,sInt size,sU32 color,sF32 zoom)
{
  GenOverlayRT *rt,*in;

  parent->ExecInputs(kenv);
  sInt ocount = parent->GetOutputCount();

#ifdef _DOPE
  if(!GenOverlayManager->EnableIPP)
    return;
#endif

  in = GenOverlayManager->LastOutput;
  if(in)
  {
    if(size == GENOVER_RTSIZES+1)
      size = in->Size;

    rt = GenOverlayManager->Find(parent);
    if(!rt)
    {
      rt = GenOverlayManager->Alloc(parent,size,ocount);
      GenOverlayManager->Copy(in,rt,0,color,zoom);
    }
    GenOverlayManager->LastOutput = rt;
  }
}

void __stdcall Exec_IPP_Blur(KOp *parent,KEnvironment *kenv,sInt size,sF32 radius,sF32 amplify,sInt type,sInt stages)
{
  parent->ExecInputs(kenv);
  sInt ocount = parent->GetOutputCount();

#ifdef _DOPE
  if(!GenOverlayManager->EnableIPP)
    return;
#endif

  GenOverlayManager->LastOutput = GenOverlayManager->BlurOp(parent,
    GenOverlayManager->LastOutput,size,radius,amplify,type,stages,ocount);
}

void __stdcall Exec_IPP_Crashzoom(KOp *parent,KEnvironment *kenv,sInt size,sInt steps,sF32 zoom,sF32 amplify,sF322 center)
{
  GenOverlayRT *in;
  sFRect rc[4];
  sInt i;
  sF32 z,zf;

  parent->ExecInputs(kenv);
  sInt ocount = parent->GetOutputCount();
  if(zoom<1) zoom = 2-zoom;   // zoom <1 gets mirrored to the >1 range

#ifdef _DOPE
  if(!GenOverlayManager->EnableIPP)
    return;
#endif

  in = GenOverlayManager->Find(parent);
  if(!in)
  {
    in = GenOverlayManager->LastOutput;
    while(steps>0)
    {
      z = 1.0f;
      zf = sFPow(zoom,-sFPow(4.0f,-steps));
      steps--;

      for(i=0;i<4;i++)
      {
        rc[i].x0 = center.x * (1.0f - z);
        rc[i].y0 = center.y * (1.0f - z);
        rc[i].x1 = rc[i].x0 + z;
        rc[i].y1 = rc[i].y0 + z;
        z *= zf;
      }

      in = GenOverlayManager->Blend4xOp(parent,in,rc,0,steps ? 1.0f : amplify,
        (steps && size == GENOVER_RTSIZES) ? 0 : size,steps ? 1 : ocount);
    }
  }
  GenOverlayManager->LastOutput = in;
}

void __stdcall Exec_IPP_Sharpen(KOp *parent,KEnvironment *kenv,sInt size,sF32 radius,sU32 amplify,sInt stages)
{
  GenOverlayRT *in,*blur,*out;
  sViewport view;

  parent->ExecInputs(kenv);
  sInt ocount = parent->GetOutputCount();

#ifdef _DOPE
  if(!GenOverlayManager->EnableIPP)
    return;
#endif

  in = GenOverlayManager->Find(parent);
  if(!in)
  {
    in = GenOverlayManager->LastOutput;
    if(in && in->Bitmap)
    {
      in->Refs++;
      blur = GenOverlayManager->BlurOp(0,in,size,radius,1.0f,0,stages,1);
      if(!blur->Bitmap)
        return;

      if(size == GENOVER_RTSIZES+1)
        size = in->Size;

      out = GenOverlayManager->Alloc(parent,size,ocount);
      GenOverlayManager->PrepareViewport(out,view);
      sSystem->SetViewport(view);

      GenOverlayManager->Mtrl[GENOVER_SHARPEN]->SetTex(0,blur->Bitmap->Texture);
      GenOverlayManager->Mtrl[GENOVER_SHARPEN]->SetTex(1,in->Bitmap->Texture);
      GenOverlayManager->Mtrl[GENOVER_SHARPEN]->Color[2] = amplify;
      GenOverlayManager->FXQuad(GENOVER_SHARPEN,blur->ScaleUV,0,in->ScaleUV,0);

      GenOverlayManager->Free(in);
      GenOverlayManager->Free(blur);
      GenOverlayManager->LastOutput = out;
    }
  }
  else
    GenOverlayManager->LastOutput = in;
}

void __stdcall Exec_IPP_Color(KOp *parent,KEnvironment *kenv,sInt size,sU32 color,sInt operation,sU32 amplify)
{
  GenOverlayRT *in,*out;
  sViewport view;

#ifdef _DOPE
  if(!GenOverlayManager->EnableIPP)
  {
    parent->ExecInputs(kenv);
    return;
  }
#endif

  sInt ocount = parent->GetOutputCount();

  in = GenOverlayManager->Find(parent);
  if(!in)
  {
    parent->ExecInputs(kenv);

    in = GenOverlayManager->LastOutput;
    if(in && in->Bitmap)
    {
      if(size == GENOVER_RTSIZES+1)
        size = in->Size;

      out = GenOverlayManager->Alloc(parent,size,ocount);
      GenOverlayManager->PrepareViewport(out,view);
      sSystem->SetViewport(view);

      GenOverlayManager->Mtrl[GENOVER_COLOR0+operation]->SetTex(0,in->Bitmap->Texture);
      GenOverlayManager->Mtrl[GENOVER_COLOR0+operation]->Color[0] = color;
      GenOverlayManager->Mtrl[GENOVER_COLOR0+operation]->Color[2] = amplify;
      GenOverlayManager->FXQuad(GENOVER_COLOR0+operation,in->ScaleUV);

      GenOverlayManager->Free(in);
      GenOverlayManager->LastOutput = out;
    }
  }
  else
    GenOverlayManager->LastOutput = in;
}

void __stdcall Exec_IPP_Merge(KOp *parent,KEnvironment *kenv,sInt size,sInt operation,sInt alpha,sU32 amplify)
{
  GenOverlayRT *in[2],*out;
  sViewport view;
  sInt i;

  out = GenOverlayManager->Find(parent);
  sInt ocount = parent->GetOutputCount();

#ifdef _DOPE
  if(!GenOverlayManager->EnableIPP)
  {
    parent->ExecInput(kenv,0);
    GenOverlayManager->DontRender++;
    parent->ExecInput(kenv,1);
    GenOverlayManager->DontRender--;
    return;
  }
#endif

  if(!out)
  {
    for(i=0;i<2;i++)
    {
      GenOverlayManager->LastOutput = 0;
      parent->ExecInput(kenv,i);
      in[i] = GenOverlayManager->LastOutput;
      GenOverlayManager->LastOutput = 0;

      if(!in[i] || !in[i]->Bitmap)
        return;
    }

    if(size == GENOVER_RTSIZES+1)
      size = in[0]->Size;
    
    out = GenOverlayManager->Alloc(parent,size,ocount);
    GenOverlayManager->PrepareViewport(out,view);
    sSystem->SetViewport(view);

    GenOverlayManager->Mtrl[GENOVER_MERGE0+operation]->SetTex(0,in[0]->Bitmap->Texture);
    GenOverlayManager->Mtrl[GENOVER_MERGE0+operation]->SetTex(1,in[1]->Bitmap->Texture);
    GenOverlayManager->Mtrl[GENOVER_MERGE0+operation]->Color[0] = alpha<<24;
    GenOverlayManager->Mtrl[GENOVER_MERGE0+operation]->Color[2] = amplify;
    GenOverlayManager->FXQuad(GENOVER_MERGE0+operation,in[0]->ScaleUV,0,in[1]->ScaleUV,0);

    GenOverlayManager->Free(in[0]);
    GenOverlayManager->Free(in[1]);
  }
  GenOverlayManager->LastOutput = out;
}

void __stdcall Exec_IPP_Mask(KOp *parent,KEnvironment *kenv,sInt size)
{
  GenOverlayRT *in[3],*out;
  sViewport view;
  sInt i;

  sInt ocount = parent->GetOutputCount();

#ifdef _DOPE
  if(!GenOverlayManager->EnableIPP)
  {
    parent->ExecInput(kenv,0);
    GenOverlayManager->DontRender++;
    for(i=1;i<3;i++)
      parent->ExecInput(kenv,i);
    GenOverlayManager->DontRender--;
    return;
  }
#endif

  out = GenOverlayManager->Find(parent);
  if(!out)
  {
    for(i=0;i<3;i++)
    {
      GenOverlayManager->LastOutput = 0;
      parent->ExecInput(kenv,i);
      in[i] = GenOverlayManager->LastOutput;

      if(!in[i] || !in[i]->Bitmap)
        return;
    }

    if(size == GENOVER_RTSIZES+1)
      size = in[0]->Size;

    out = GenOverlayManager->Alloc(parent,size,ocount);
    GenOverlayManager->PrepareViewport(out,view);
    sSystem->SetViewport(view);

    GenOverlayManager->Mtrl[GENOVER_MASK1]->SetTex(0,in[2]->Bitmap->Texture);
    GenOverlayManager->Mtrl[GENOVER_MASK1]->SetTex(1,in[1]->Bitmap->Texture);
    GenOverlayManager->FXQuad(GENOVER_MASK1,in[2]->ScaleUV,0,in[1]->ScaleUV,0);
    GenOverlayManager->Mtrl[GENOVER_MASK2]->SetTex(0,in[2]->Bitmap->Texture);
    GenOverlayManager->Mtrl[GENOVER_MASK2]->SetTex(2,in[0]->Bitmap->Texture);
    GenOverlayManager->FXQuad(GENOVER_MASK2,in[2]->ScaleUV,0,in[0]->ScaleUV,0);

    for(i=0;i<3;i++)
      GenOverlayManager->Free(in[i]);
  }
  GenOverlayManager->LastOutput = out;
}

void __stdcall Exec_IPP_Layer2D(KOp *parent,KEnvironment *kenv,sInt size,sFRect oxy,sFRect ouv,sF32 z,sInt flags)
{
  KOp *mtrlOp;
  GenOverlayRT *in,*out;
  GenMaterial *mtrl;
  sViewport view;
  sMaterialEnv env;
  sInt i;
  sFRect rxy,ruv;

  sInt ocount = parent->GetOutputCount();

#ifdef _DOPE
  if(!GenOverlayManager->EnableIPP)
  {
    parent->ExecInput(kenv,0);
    return;
  }
#endif

  if(flags&0x10)
  {
    rxy.x0 = oxy.x0-oxy.x1*0.5f;
    rxy.y0 = oxy.y0-oxy.y1*0.5f;
    rxy.x1 = oxy.x0+oxy.x1*0.5f;
    rxy.y1 = oxy.y0+oxy.y1*0.5f;
  }
  else 
  {
    rxy = oxy;
  }
  if(flags&0x20)
  {
    ruv.x0 = ouv.x0-ouv.x1*0.5f;
    ruv.y0 = ouv.y0-ouv.y1*0.5f;
    ruv.x1 = ouv.x0+ouv.x1*0.5f;
    ruv.y1 = ouv.y0+ouv.y1*0.5f;
  }
  else 
  {
    ruv = ouv;
  }


  out = GenOverlayManager->Find(parent);
  if(!out)
  {
    GenOverlayManager->LastOutput = 0;
    parent->ExecInputs(kenv);
    in = GenOverlayManager->LastOutput;

    /*if(in && in->Refs == 1) // overwrite
    {
      out = in;
      out->Refs = ocount;
      out->Owner = parent;
      in = 0;
    }
    else*/
      out = GenOverlayManager->Alloc(parent,in ? in->Size : size,ocount);

    if(in)
      GenOverlayManager->Copy(in,out,0,0xffffffff,1.0f);

    mtrlOp = parent->GetLink(0);
    if(mtrlOp && mtrlOp->CheckOutput(KC_MATERIAL))
    {
      mtrl = (GenMaterial *) mtrlOp->Cache;
      GenOverlayManager->PrepareViewport(out,view);
      sSystem->SetViewport(view);
      sSystem->Clear(flags & 3,0xff000000);

      env.Init();
      env.Orthogonal = sMEO_NORMALISED;

      for(i=0;i<mtrl->Passes.Count;i++)
      {
        sSystem->SetViewProject(&env);
        mtrl->Passes[i].Mtrl->Set(env);
        GenOverlayManager->Quad(&rxy,&ruv,z * env.FarClip);
      }
    }

    if(in)
      GenOverlayManager->Free(in);
  }

  GenOverlayManager->LastOutput = out;
}

void __stdcall Exec_IPP_Select(KOp *op,KEnvironment *kenv,sInt count)
{
  GenOverlayRT *prev=0, *current=0;

  /*for(sInt i=0;i<op->GetInputCount();i++)
  {
    GenOverlayManager->LastOutput = 0;

    op->ExecInput(kenv,i);
    if(GenOverlayManager->LastOutput)
    {
      if(prev)
        GenOverlayManager->Free(prev);

      prev = current;
      current = GenOverlayManager->LastOutput;
    }
  }

  if(prev)
    GenOverlayManager->Free(prev);

  if(current)
    current->Refs += count - 1;*/

  op->ExecInputs(kenv);

  if(GenOverlayManager->LastOutput)
    GenOverlayManager->LastOutput->Refs += count - 1;
}

void __stdcall Exec_IPP_RenderTarget(KOp *op,KEnvironment *kenv)
{
  GenBitmap *rt = (GenBitmap *) op->GetLink(0)->Cache;

  if(rt->Texture != sINVALID)
  {
    // just tweak the master viewport
    sViewport saved,newvp;

    GenOverlayManager->GetMasterViewport(saved);
    newvp.InitTexMS(rt->Texture);
    GenOverlayManager->SetMasterViewport(newvp);

    // render
    op->ExecInputs(kenv);

    // if the current output is not our rendertarget, we need to do a copy
    if(GenOverlayManager->LastOutput)
    {
      GenOverlayManager->Copy(GenOverlayManager->LastOutput,
        GenOverlayManager->Alloc(0,GENOVER_RTSIZES,1),0,0xffffffff,1);
    }

    // restore
    GenOverlayManager->SetMasterViewport(saved);
  }
}

#if !sPLAYER
void __stdcall Exec_IPP_HSCB(KOp *parent,KEnvironment *kenv,sInt size,sF32 hue,sF32 sat,sF32 con,sF32 bright)
{
  GenOverlayRT *in,*out;
  sViewport view;

#ifdef _DOPE
  if(!GenOverlayManager->EnableIPP)
  {
    parent->ExecInputs(kenv);
    return;
  }
#endif

  sInt ocount = parent->GetOutputCount();

  in = GenOverlayManager->Find(parent);
  if(!in)
  {
    parent->ExecInputs(kenv);

    in = GenOverlayManager->LastOutput;
    if(in && in->Bitmap)
    {
      if(size == GENOVER_RTSIZES+1)
        size = in->Size;

      out = GenOverlayManager->Alloc(parent,size,ocount);
      GenOverlayManager->PrepareViewport(out,view);
      sSystem->SetViewport(view);

      sMaterialInstance inst;
      sVector pc[3],vc[4];

      inst.NumTextures = 1;
      inst.NumPSConstants = 3;
      inst.NumVSConstants = 4;
      inst.Textures[0] = in->Bitmap->Texture;
      inst.PSConstants = pc;
      inst.VSConstants = vc;
      pc[0].Init(-bright,bright,-bright,-bright);
      pc[1].Init(con*con,0.0f,-1.0f/3.0f,-2.0f/3.0f);
      pc[2].Init(hue,0.0f,sat*2.0f,1.0f/6.0f);

      GenOverlayManager->HSCBMaterial->SetInstance(inst);
      GenOverlayManager->FXQuad(GenOverlayManager->HSCBMaterial,in->ScaleUV);

      GenOverlayManager->Free(in);
      GenOverlayManager->LastOutput = out;
    }
  }
  else
    GenOverlayManager->LastOutput = in;
}
#endif

#if sLINK_MTRL20 && sIPP_JPEG
void __stdcall Exec_IPP_JPEG(KOp *parent,KEnvironment *kenv,sInt size,sInt dir,sF32 strength)
{
  GenOverlayRT *in,*out;
  sViewport view;

  out = GenOverlayManager->Find(parent);
  sInt ocount = parent->GetOutputCount();

  if(!out)
  {
    GenOverlayManager->LastOutput = 0;
    parent->ExecInputs(kenv);
    in = GenOverlayManager->LastOutput;
    GenOverlayManager->LastOutput = 0;

    if(in && in->Bitmap)
    {
      if(size == GENOVER_RTSIZES+1)
        size = in->Size;
      
      out = GenOverlayManager->Alloc(parent,size,ocount);
      GenOverlayManager->PrepareViewport(out,view);
      sSystem->SetViewport(view);

      sMaterialInstance instance;
      sVector vc[5],pc[1];
      sF32 scaleUV2[2];

      instance.NumTextures = 6;
      instance.NumPSConstants = 1;
      instance.NumVSConstants = 5;
      instance.Textures[0] = in->Bitmap->Texture;
      instance.Textures[1] = in->Bitmap->Texture;
      instance.Textures[2] = in->Bitmap->Texture;
      instance.Textures[3] = in->Bitmap->Texture;
      instance.Textures[4] = GenOverlayManager->JPEGKernel0;
      instance.Textures[5] = GenOverlayManager->JPEGKernel1;
      instance.PSConstants = pc;
      instance.VSConstants = vc;

      pc[0].Init(0,0,0,0);
      pc[0].x = (dir == 0) ? in->ScaleUV[0] / sSystem->ViewportX : 0.0f;
      pc[0].y = (dir == 1) ? in->ScaleUV[1] / sSystem->ViewportY : 0.0f;
      vc[0].Init(in->ScaleUV[0],0.0f,0.0f,0.0f);

      scaleUV2[0] = (dir == 0) ? sSystem->ViewportX / 4.0f : 0.0f;
      scaleUV2[1] = (dir == 1) ? sSystem->ViewportY / 4.0f : 0.0f;

      GenOverlayManager->JPEGMaterial->SetInstance(instance);
      GenOverlayManager->FXQuad(GenOverlayManager->JPEGMaterial,in->ScaleUV,0,scaleUV2);

      GenOverlayManager->Free(in);
    }
  }
  GenOverlayManager->LastOutput = out;
}
#else
void __stdcall Exec_IPP_JPEG(KOp *parent,KEnvironment *kenv,sInt size,sInt dir,sF32 strength,sInt ocount)
{
}
#endif

/****************************************************************************/
/****************************************************************************/
