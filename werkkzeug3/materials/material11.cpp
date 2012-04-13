// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_types.hpp"
#include "_startdx.hpp"
#include "material11.hpp"
#include "shadercodegen.hpp"

#include "materials/material11vs.hpp"
#include "materials/material11ps.hpp"

/****************************************************************************/

void sMaterial11::FreeEverything()
{
  for(sInt i=0;i<4;i++)
    SetTex(i,sINVALID);

  for(sInt i=0;i<NumSetups;i++)
  {
    if(Setup[i] != sINVALID)
    {
      sSystem->MtrlRemSetup(Setup[i]);
      Setup[i] = sINVALID;
    }
  }
}

/****************************************************************************/

void sMaterial11::LoadDefaults()
{
  HasTexSRT = sFALSE;
  for(sInt i=0;i<NumSetups;i++)
    Setup[i] = sINVALID;

  BaseFlags = sMBF_ZON;
  SpecialFlags = LightFlags = pad1 = 0;
  pad3 = pad4 = AlphaCombiner = pad5 = 0;
  SpecPower = 8.0f;
  ShaderLevel = sPS_00;

  sSetMem(Combiner,0,sizeof(Combiner));
  sSetMem(SRT1,0,sizeof(SRT1));
  sSetMem(SRT2,0,sizeof(SRT2));
  
  for(sInt i=0;i<4;i++)
  {
    TFlags[i] = sMTF_MIPMAPS;
    Color[i] = ~0U;
    TScale[i] = 0.0f;
    Tex[i] = sINVALID;
  }

#if !sPLAYER
  PSOps = VSOps = 0;
  ShaderLevelUsed = sPS_00;
#endif
}

/****************************************************************************/

sMaterial11::sMaterial11()
{
  LoadDefaults();
}

/****************************************************************************/

sMaterial11::~sMaterial11()
{
  FreeEverything();
}

/****************************************************************************/

void sMaterial11::Reset()
{
  FreeEverything();
  LoadDefaults();
}

/****************************************************************************/

void sMaterial11::CopyFrom(const sMaterial11 *x)
{
  FreeEverything();

  BaseFlags = x->BaseFlags;
  SpecialFlags = x->SpecialFlags;
  LightFlags = x->LightFlags;
  sCopyMem(Combiner,x->Combiner,sizeof(Combiner));
  AlphaCombiner = x->AlphaCombiner;
  SpecPower = x->SpecPower;
  sCopyMem(SRT1,x->SRT1,sizeof(SRT1));
  sCopyMem(SRT2,x->SRT2,sizeof(SRT2));
  ShaderLevel = x->ShaderLevel;

  for(sInt i=0;i<4;i++)
  {
    TFlags[i] = x->TFlags[i];
    TScale[i] = x->TScale[i];
    Color[i] = x->Color[i];
    SetTex(i,x->Tex[i]);
    TexUse[i] = x->TexUse[i];
  }

#if !sPLAYER
  PSOps = x->PSOps;
  VSOps = x->VSOps;
  ShaderLevelUsed = x->ShaderLevelUsed;
#endif

  HasTexSRT = x->HasTexSRT;
  for(sInt i=0;i<NumSetups;i++)
  {
    Setup[i] = x->Setup[i];
    if(Setup[i] != sINVALID)
      sSystem->MtrlAddRefSetup(Setup[i]);
  }
}

/****************************************************************************/

void sMaterial11::DefaultCombiner(sBool *tex)
{
  sBool mytex[4];
  sInt i;
  sU32 light;

// find out which textures are set

  if(tex==0)
  {
    tex = mytex;
    for(i=0;i<4;i++)
      tex[i] = (Tex[i]!=sINVALID);
  }

// prepare

  for(i=0;i<sMCS_MAX;i++)
    Combiner[i] = 0;

  light = LightFlags&sMLF_MODEMASK;

// set textures

  if(tex[0] && !(SpecialFlags&sMSF_BUMPDETAIL))
    Combiner[sMCS_TEX0] = sMCOA_MUL;
  if(tex[1] && light<sMLF_BUMPX)
    Combiner[sMCS_TEX1] = sMCOA_MUL;
  if(tex[2])
    Combiner[sMCS_TEX2] = sMCOA_ADD;
  if(tex[3])
    Combiner[sMCS_TEX3] = sMCOA_ADD;

// set light

  if(light)
  {
    Combiner[sMCS_LIGHT] = sMCOA_MUL;
    Combiner[sMCS_COLOR0] = sMCOA_MUL;
    Combiner[sMCS_COLOR1] = sMCOA_ADD;
  }

// set first combiner to mul

  for(i=0;i<sMCS_MAX;i++)
  {
    if(Combiner[i])
    {
      Combiner[i] = sMCOA_SET;
      return;
    }
  }

  Combiner[sMCS_COLOR0] = sMCOA_SET;  // no texture, set color
}

/****************************************************************************/

void sMaterial11::SetTex(sInt i,sInt handle)
{
  sVERIFY(i >= 0 && i <= 3);

  if(handle != sINVALID)
    sSystem->AddRefTexture(handle);

  if(Tex[i] != sINVALID)
    sSystem->RemTexture(Tex[i]);

  Tex[i] = handle;
}

/****************************************************************************/

static void makeStencilStates(sU32 *&pt,sU32 flags)
{
  sU32 *data = pt;

  switch(flags & sMBF_STENCILMASK)
  {
  default:
    *data++ = sD3DRS_STENCILENABLE;      *data++ = 0;
    *data++ = sD3DRS_TWOSIDEDSTENCILMODE; *data++ = 0;
    *data++ = sD3DRS_STENCILFUNC;        *data++ = sD3DCMP_ALWAYS;
    break;
  case sMBF_STENCILINC:
    *data++ = sD3DRS_STENCILENABLE;      *data++ = 1;
    *data++ = sD3DRS_TWOSIDEDSTENCILMODE; *data++ = 0;
    *data++ = sD3DRS_STENCILFUNC;        *data++ = sD3DCMP_ALWAYS;
    *data++ = sD3DRS_STENCILFAIL;        *data++ = sD3DSTENCILOP_KEEP;
    if(flags & sMBF_STENCILZFAIL)
    {
      *data++ = sD3DRS_STENCILZFAIL;       *data++ = sD3DSTENCILOP_INCR;
      *data++ = sD3DRS_STENCILPASS;        *data++ = sD3DSTENCILOP_KEEP;
    }
    else
    {
      *data++ = sD3DRS_STENCILZFAIL;       *data++ = sD3DSTENCILOP_KEEP;
      *data++ = sD3DRS_STENCILPASS;        *data++ = sD3DSTENCILOP_INCR;
    }
    break;
  case sMBF_STENCILDEC:
    *data++ = sD3DRS_STENCILENABLE;      *data++ = 1;
    *data++ = sD3DRS_TWOSIDEDSTENCILMODE; *data++ = 0;
    *data++ = sD3DRS_STENCILFUNC;        *data++ = sD3DCMP_ALWAYS;
    *data++ = sD3DRS_STENCILFAIL;        *data++ = sD3DSTENCILOP_KEEP;
    if(flags & sMBF_STENCILZFAIL)
    {
      *data++ = sD3DRS_STENCILZFAIL;       *data++ = sD3DSTENCILOP_DECR;
      *data++ = sD3DRS_STENCILPASS;        *data++ = sD3DSTENCILOP_KEEP;
    }
    else
    {
      *data++ = sD3DRS_STENCILZFAIL;       *data++ = sD3DSTENCILOP_KEEP;
      *data++ = sD3DRS_STENCILPASS;        *data++ = sD3DSTENCILOP_DECR;
    }
    break;
  case sMBF_STENCILTEST:
    *data++ = sD3DRS_STENCILENABLE;      *data++ = 1;
    *data++ = sD3DRS_TWOSIDEDSTENCILMODE; *data++ = 0;
    *data++ = sD3DRS_STENCILFUNC;        *data++ = sD3DCMP_EQUAL;
    *data++ = sD3DRS_STENCILFAIL;        *data++ = sD3DSTENCILOP_KEEP;
    *data++ = sD3DRS_STENCILZFAIL;       *data++ = sD3DSTENCILOP_KEEP;
    *data++ = sD3DRS_STENCILPASS;        *data++ = sD3DSTENCILOP_KEEP;
    break;
  case sMBF_STENCILCLR:
    *data++ = sD3DRS_STENCILENABLE;      *data++ = 1;
    *data++ = sD3DRS_TWOSIDEDSTENCILMODE; *data++ = 0;
    *data++ = sD3DRS_STENCILFUNC;        *data++ = sD3DCMP_ALWAYS;
    *data++ = sD3DRS_STENCILFAIL;        *data++ = sD3DSTENCILOP_ZERO;
    *data++ = sD3DRS_STENCILZFAIL;       *data++ = sD3DSTENCILOP_ZERO;
    *data++ = sD3DRS_STENCILPASS;        *data++ = sD3DSTENCILOP_ZERO;
    break;
  case sMBF_STENCILINDE:
    *data++ = sD3DRS_CULLMODE;           *data++ = sD3DCULL_NONE;
    *data++ = sD3DRS_STENCILENABLE;      *data++ = 1;
    *data++ = sD3DRS_TWOSIDEDSTENCILMODE; *data++ = 1;
    *data++ = sD3DRS_STENCILFUNC;        *data++ = sD3DCMP_ALWAYS;
    *data++ = sD3DRS_CCW_STENCILFUNC;    *data++ = sD3DCMP_ALWAYS;
    *data++ = sD3DRS_STENCILFAIL;        *data++ = sD3DSTENCILOP_KEEP;
    *data++ = sD3DRS_CCW_STENCILFAIL;    *data++ = sD3DSTENCILOP_KEEP;
    if(flags & sMBF_STENCILZFAIL)
    {
      *data++ = sD3DRS_STENCILZFAIL;       *data++ = sD3DSTENCILOP_DECR;
      *data++ = sD3DRS_CCW_STENCILZFAIL;   *data++ = sD3DSTENCILOP_INCR;
      *data++ = sD3DRS_STENCILPASS;        *data++ = sD3DSTENCILOP_KEEP;
      *data++ = sD3DRS_CCW_STENCILPASS;    *data++ = sD3DSTENCILOP_KEEP;
    }
    else
    {
      *data++ = sD3DRS_STENCILZFAIL;       *data++ = sD3DSTENCILOP_KEEP;
      *data++ = sD3DRS_CCW_STENCILZFAIL;   *data++ = sD3DSTENCILOP_KEEP;
      *data++ = sD3DRS_STENCILPASS;        *data++ = sD3DSTENCILOP_INCR;
      *data++ = sD3DRS_CCW_STENCILPASS;    *data++ = sD3DSTENCILOP_DECR;
    }
    break;
  case sMBF_STENCILRENDER:
    *data++ = sD3DRS_STENCILENABLE;      *data++ = 1;
    *data++ = sD3DRS_TWOSIDEDSTENCILMODE; *data++ = 0;
    *data++ = sD3DRS_STENCILFUNC;        *data++ = sD3DCMP_NOTEQUAL;
    *data++ = sD3DRS_STENCILFAIL;        *data++ = sD3DSTENCILOP_KEEP;
    *data++ = sD3DRS_STENCILZFAIL;       *data++ = sD3DSTENCILOP_KEEP;
    *data++ = sD3DRS_STENCILPASS;        *data++ = sD3DSTENCILOP_ZERO;
    break;
  }

  pt = data;
}

sBool sMaterial11::Compile()
{
  static const sU32 psatoc[16] =
  {
    0,1<<sMCS_LIGHT,(1<<sMCS_LIGHT),1<<sMCS_LIGHT,
    1<<sMCS_TEX0,1<<sMCS_TEX1,1<<sMCS_TEX2,1<<sMCS_TEX3,
    1<<sMCS_COLOR0,1<<sMCS_COLOR1,1<<sMCS_COLOR2,1<<sMCS_COLOR3,
    0,0,0,0,
  };
  static const sU32 psttoc[4] =
  {
    sMCS_TEX0,sMCS_TEX1,sMCS_TEX2,sMCS_TEX3
  };
  static sU32 pssrc13[sMCS_MAX] =
  {
    XSALL|X_R|0,  XSALL|X_C|0,  XSALL|X_C|1,           ~0,
             ~0,           ~0,   XS_W|X_R|0,  XSALL|X_C|2,
    XSALL|X_C|3,           ~0,           ~0,  XSALL|X_V|0,
             ~0,
  };
  static sU32 psalpha13[16] =
  {
    XS_W|X_C|6, XS_Z|X_R|1, XS_W|X_R|0, XS_W|X_C|6,
    XS_W|X_T|0, XS_W|X_T|1, XS_W|X_T|2, XS_W|X_T|3,
    XS_W|X_C|0, XS_W|X_C|1, XS_W|X_C|2, XS_W|X_C|3,
    XS_W|X_C|7, XS_Z|X_C|6,         ~0,         ~0,
  };

  sU32 *data;
  sInt i,texuv;
  sU32 flags;
  static sU32 states[256];
  sU32 *ps;
  sU32 *vs[NumSetups];
  sInt combinermask;
  sInt error;
  sInt ps00_combine;

  sInt TFile[4];                  // sMCS_TEX -> d3d_texture
  sInt TCount;
  static sShaderCodeGen codegen;  // static wegen __chkstk

// modify material

  ps00_combine = sMCOA_NOP;

  switch(ShaderLevel)
  {
#if !sPLAYER
  case sPS_00:
    if(SpecialFlags & sMSF_BUMPDETAIL)
    {
      SetTex(0,sINVALID);
      SpecialFlags &= ~sMSF_BUMPDETAIL;
    }

    if(Tex[0] == sINVALID)
    {
      SetTex(1,sINVALID);
      SetTex(2,sINVALID);
      SetTex(3,sINVALID);
    }
    else
    {
      if(Tex[2]!=sINVALID)
      {
        ps00_combine = Combiner[sMCS_TEX2];
        SetTex(1,sINVALID);
        SetTex(3,sINVALID);
      }
      else if(Tex[3]!=sINVALID)
      {
        ps00_combine = Combiner[sMCS_TEX3];
        SetTex(1,sINVALID);
      }
      else if(Tex[1]!=sINVALID)
      {
        ps00_combine = Combiner[sMCS_TEX1];
      }
    }
    break;
#endif
  case sPS_11:
  case sPS_14:
  case sPS_20:
    if(SpecialFlags & sMSF_BUMPDETAIL)
    {
      SetTex(0,sINVALID);
      SpecialFlags &= ~sMSF_BUMPDETAIL;
    }
    break;
  }

//*** initialize compile

  HasTexSRT = sFALSE;
  data = states;
  error = 0;
  for(sInt i=0;i<NumSetups;i++)
  {
    Setup[i] = sINVALID;
    vs[i] = 0;
  }
  ps = 0;
  TCount = 0;
  combinermask = 0;
  
  for(i=0;i<4;i++)
    TexUse[i] = -1;

// prepare codegen data
  sU32 codegenData[56];
  sCopyMem4(codegenData,&BaseFlags,24);

//*** base flags

  flags = BaseFlags;
  sF32 f = 0;
  if(flags & sMBF_ZBIASBACK) f=1.0f / 65536.0f;
  if(flags & sMBF_ZBIASFORE) f=-1.0f / 65536.0f;

  *data++ = sD3DRS_ALPHATESTENABLE;             *data++ = (flags & sMBF_ALPHATEST) ? 1 : 0;
  *data++ = sD3DRS_ALPHAFUNC;                   *data++ = sD3DCMP_GREATER;
  *data++ = sD3DRS_ALPHAREF;                    *data++ = 128;
  *data++ = sD3DRS_ZENABLE;                     *data++ = sD3DZB_TRUE;
  *data++ = sD3DRS_ZWRITEENABLE;                *data++ = (flags & sMBF_ZWRITE) ? 1 : 0;
  *data++ = sD3DRS_ZFUNC;

  if(!(flags & sMBF_ZREAD))
    i = sD3DCMP_ALWAYS;
  else if(flags & sMBF_ZEQUAL)
    i = sD3DCMP_EQUAL;
  else if(flags & sMBF_SHADOWMASK)
    i = sD3DCMP_LESS;
  else
    i = sD3DCMP_LESSEQUAL;

  *data++ = i;
  *data++ = sD3DRS_CULLMODE;                    *data++ = (flags & sMBF_DOUBLESIDED) ? sD3DCULL_NONE : (flags & sMBF_INVERTCULL?sD3DCULL_CW:sD3DCULL_CCW);
  *data++ = sD3DRS_COLORWRITEENABLE;            *data++ = (flags & sMBF_ZONLY) ? 0 : 15;
  *data++ = sD3DRS_SLOPESCALEDEPTHBIAS;         *data++ = *(sU32 *)&f;
  *data++ = sD3DRS_DEPTHBIAS;                   *data++ = *(sU32 *)&f;
  *data++ = sD3DRS_FOGENABLE;                   *data++ = (flags & sMBF_FOG) ? 1 : 0;
  *data++ = sD3DRS_FOGVERTEXMODE;               *data++ = sD3DFOG_LINEAR;
  *data++ = sD3DRS_FOGSTART;                    *data++ = 0x00000000;
  *data++ = sD3DRS_FOGEND;                      *data++ = 0x3f800000;
  *data++ = sD3DRS_CLIPPING;                    *data++ = 1;
  *data++ = sD3DTSS_TEXCOORDINDEX|sD3DTSS_0;    *data++ = 0;
  *data++ = sD3DTSS_TEXCOORDINDEX|sD3DTSS_1;    *data++ = 1;
  *data++ = sD3DTSS_TEXCOORDINDEX|sD3DTSS_2;    *data++ = 2;
  *data++ = sD3DTSS_TEXCOORDINDEX|sD3DTSS_3;    *data++ = 3;

  flags = BaseFlags & sMBF_BLENDMASK;
  *data++ = sD3DRS_ALPHABLENDENABLE;            *data++ = flags ? 1 : 0;

  if(flags && flags <= sMBF_BLENDADDSA)
  {
    static const sU32 blendOp[10][2] =
    {
      { 0,0 },
      { sD3DBLEND_SRCALPHA,sD3DBLEND_INVSRCALPHA },
      { sD3DBLEND_ONE,sD3DBLEND_ONE },
      { sD3DBLEND_ZERO,sD3DBLEND_SRCCOLOR },
      { sD3DBLEND_DESTCOLOR,sD3DBLEND_SRCCOLOR },
      { sD3DBLEND_ONE,sD3DBLEND_INVSRCCOLOR },
      { sD3DBLEND_ONE,sD3DBLEND_ONE },
      { sD3DBLEND_INVDESTCOLOR,sD3DBLEND_INVSRCCOLOR },
      { sD3DBLEND_DESTALPHA,sD3DBLEND_ONE },
      { sD3DBLEND_SRCALPHA,sD3DBLEND_ONE }
    };

    *data++ = sD3DRS_BLENDOP;           *data++ = (flags == sMBF_BLENDSUB) ? sD3DBLENDOP_REVSUBTRACT : sD3DBLENDOP_ADD;
    *data++ = sD3DRS_SRCBLEND;          *data++ = blendOp[flags >> 12][0];
    *data++ = sD3DRS_DESTBLEND;         *data++ = blendOp[flags >> 12][1];
  }

  makeStencilStates(data,BaseFlags);

  texuv = 0;
  for(i=0;i<4;i++)
  {
    static const sU32 filters[8][4] = 
    {
      { sD3DTEXF_POINT,sD3DTEXF_POINT,sD3DTEXF_NONE },
      { sD3DTEXF_LINEAR,sD3DTEXF_LINEAR,sD3DTEXF_NONE },
      { sD3DTEXF_LINEAR,sD3DTEXF_LINEAR,sD3DTEXF_LINEAR },
      { sD3DTEXF_LINEAR,sD3DTEXF_ANISOTROPIC,sD3DTEXF_LINEAR },
      { sD3DTEXF_POINT,sD3DTEXF_POINT,sD3DTEXF_POINT },
      { sD3DTEXF_LINEAR,sD3DTEXF_LINEAR,sD3DTEXF_POINT },
      { sD3DTEXF_POINT,sD3DTEXF_POINT,sD3DTEXF_NONE },
      { sD3DTEXF_POINT,sD3DTEXF_POINT,sD3DTEXF_NONE },
    };
    TFile[i] = -1;
    codegenData[24+i] = 0;
    if(Tex[i]!=sINVALID)
    {
      flags = TFlags[i];
      sInt tex = TCount*sD3DTSS_1;
      sInt sam = TCount*sD3DSAMP_1;
      TexUse[TCount] = i;
      TFile[i]=TCount++;
      codegenData[24+i] = 1;

      *data++ = sam+sD3DSAMP_MAGFILTER;    *data++ = filters[flags&7][0];
      *data++ = sam+sD3DSAMP_MINFILTER;    *data++ = filters[flags&7][1];
      *data++ = sam+sD3DSAMP_MIPFILTER;    *data++ = filters[flags&7][2];
      *data++ = sam+sD3DSAMP_ADDRESSU;     *data++ = (flags&sMTF_CLAMP) ? sD3DTADDRESS_CLAMP : sD3DTADDRESS_WRAP;
      *data++ = sam+sD3DSAMP_ADDRESSV;     *data++ = (flags&sMTF_CLAMP) ? sD3DTADDRESS_CLAMP : sD3DTADDRESS_WRAP;
      *data++ = sam+sD3DSAMP_MAXANISOTROPY;*data++ = 4;

      texuv |= 1 << ((flags >> 4) & 3);

      if(ShaderLevel <= sPS_14)
      {
        *data++ = tex+sD3DTSS_TEXTURETRANSFORMFLAGS;
        *data++ = (flags & sMTF_PROJECTIVE) ? sD3DTTFF_PROJECTED : 0;
      }
    }
  }

// what do we need?

  for(i=0;i<sMCS_MAX;i++)
  {
    flags = Combiner[i];
    combinermask |= psatoc[flags&15];
    if(flags & 0xff0)
      combinermask |= 1<<i;
  }
  flags = AlphaCombiner;
  combinermask |= psatoc[flags&15];
  combinermask |= psatoc[(flags>>4)&15];

  if(ShaderLevel != sPS_00 && (LightFlags & sMLF_MODEMASK) == sMLF_BUMPX && TFile[1] != -1)
    combinermask |= 1 << sMCS_TEX1;

// prepare other information
  HasTexSRT = sFALSE;

  for(i=0;i<4;i++)
  {
    sInt comb = psttoc[i];

    if(combinermask & (1 << comb))
    {
      if((TFlags[i]&sMTF_TRANSMASK) >= sMTF_TRANS1)
        HasTexSRT = sTRUE;
    }

    pssrc13[comb] = XSALL|X_T|TFile[i];
    psalpha13[i+4] = XS_W|X_T|TFile[i];
  }
  
  // set CodegenFlags
  codegenData[3] = ((combinermask & (1 << sMCS_VERTEX)) ? 1 : 0) | (texuv & 2)
    | (ShaderLevel << 8);

  // set AlphaOps
  flags = AlphaCombiner;
  codegenData[28] = psalpha13[(flags & 15)] | ((flags & sMCA_INVERTA) ? XS_COMP : 0);
  codegenData[29] = psalpha13[((flags >> 4) & 15)] | ((flags & sMCA_INVERTB) ? XS_COMP : 0);

  // set CombinerSrc/AlphaSrc
  for(i=0;i<sMCS_MAX;i++)
  {
    flags = Combiner[i];
    codegenData[i+30] = pssrc13[i];
    codegenData[i+43] = psalpha13[flags & 0x00f];
  }

  // compile vertex shader (for different light types and in default/normalize/instancing variants)
  for(sInt i=0;i<6;i++)
  {
    sInt lightType = i & 1;
    sInt variant = (i & 6) >> 1;

    codegenData[3] = (codegenData[3] & ~0x70000) | (lightType << 16) | (variant << 18);
    vs[i] = codegen.GenCode(g_material11_vsh,codegenData);
    if(!vs[i])
      error = 1;
  }

//*** Different code paths for different shaders-levels

  switch(ShaderLevel)
  {

//*** Pixel Shader 0.0 , no pixel shader available
#if !sPLAYER
  case sPS_00:
    {
      // fix this (later)!
      sInt hascolor;

      *data++ = sD3DTSS_0|sD3DTSS_COLORARG1; *data++ = sD3DTA_TEXTURE;
      *data++ = sD3DTSS_0|sD3DTSS_COLORARG2; *data++ = sD3DTA_DIFFUSE;
      *data++ = sD3DTSS_1|sD3DTSS_COLORARG1; *data++ = sD3DTA_TEXTURE;
      *data++ = sD3DTSS_1|sD3DTSS_COLORARG2; *data++ = sD3DTA_CURRENT;
      *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1; *data++ = sD3DTA_TEXTURE;
      *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2; *data++ = sD3DTA_DIFFUSE;
      *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG1; *data++ = sD3DTA_TEXTURE;
      *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG2; *data++ = sD3DTA_CURRENT;
      *data++ = sD3DTSS_2|sD3DTSS_COLOROP;   *data++ = sD3DTOP_DISABLE;
      *data++ = sD3DTSS_2|sD3DTSS_ALPHAOP;   *data++ = sD3DTOP_DISABLE;

      if(Combiner[sMCS_LIGHT]&0x0f0)
      {
        // ---- light code for now disabled for ps0.0 path
        // write some vertex lighting code corresponding to bump setup
        // to fix this!
      }

      if((Combiner[sMCS_COLOR0] & 0xf0) || (Combiner[sMCS_COLOR1] & 0xf0))
        hascolor = 1;

      if(Tex[0]==sINVALID)
      {
        *data++ = sD3DTSS_0|sD3DTSS_COLOROP; *data++ = sD3DTOP_SELECTARG2;
        *data++ = sD3DTSS_0|sD3DTSS_ALPHAOP; *data++ = sD3DTOP_SELECTARG2;
        *data++ = sD3DTSS_1|sD3DTSS_COLOROP; *data++ = sD3DTOP_DISABLE;
        *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP; *data++ = sD3DTOP_DISABLE;
      }
      else
      {
        i = Combiner[sMCS_TEX0];
        if(Combiner[sMCS_VERTEX])
        {
          hascolor = 1;
          i = Combiner[sMCS_VERTEX];
        }
        switch(i&0x0f0)
        {
        default:
          *data++ = sD3DTSS_0|sD3DTSS_COLOROP; *data++ = sD3DTOP_SELECTARG1;
          break;
        case sMCOA_ADD:
          if(!hascolor) error = 1;
          *data++ = sD3DTSS_0|sD3DTSS_COLOROP; *data++ = sD3DTOP_ADD;
          break;
        case sMCOA_SUB:
          if(!hascolor) error = 1;
          *data++ = sD3DTSS_0|sD3DTSS_COLORARG1; *data++ = sD3DTA_DIFFUSE;
          *data++ = sD3DTSS_0|sD3DTSS_COLORARG2; *data++ = sD3DTA_TEXTURE;
          *data++ = sD3DTSS_0|sD3DTSS_COLOROP; *data++ = sD3DTOP_SUBTRACT;
          break;
        case sMCOA_MUL:
          if(!hascolor) error = 1;
          *data++ = sD3DTSS_0|sD3DTSS_COLOROP; *data++ = sD3DTOP_MODULATE;
          break;
        case sMCOA_MUL2:
          if(!hascolor) error = 1;
          *data++ = sD3DTSS_0|sD3DTSS_COLOROP; *data++ = sD3DTOP_MODULATE2X;
          break;
        case sMCOA_SMOOTH:
          if(!hascolor) error = 1;
          *data++ = sD3DTSS_0|sD3DTSS_COLOROP; *data++ = sD3DTOP_ADDSMOOTH;
          break;
        case sMCOA_RSUB:
          if(!hascolor) error = 1;
          *data++ = sD3DTSS_0|sD3DTSS_COLOROP; *data++ = sD3DTOP_SUBTRACT;
          break;
        }
        *data++ = sD3DTSS_0|sD3DTSS_ALPHAOP; *data++ = sD3DTOP_MODULATE;
        switch(ps00_combine)
        {
        case sMCOA_SET:
          *data++ = sD3DTSS_1|sD3DTSS_COLOROP; *data++ = sD3DTOP_SELECTARG1;
          *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP; *data++ = sD3DTOP_SELECTARG2;
          break;
        case sMCOA_ADD:
          *data++ = sD3DTSS_1|sD3DTSS_COLOROP; *data++ = sD3DTOP_ADD;
          *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP; *data++ = sD3DTOP_SELECTARG2;
          break;
        case sMCOA_SUB:
          *data++ = sD3DTSS_1|sD3DTSS_COLORARG1; *data++ = sD3DTA_CURRENT;
          *data++ = sD3DTSS_1|sD3DTSS_COLORARG2; *data++ = sD3DTA_TEXTURE;
          *data++ = sD3DTSS_1|sD3DTSS_COLOROP; *data++ = sD3DTOP_SUBTRACT;
          *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP; *data++ = sD3DTOP_SELECTARG2;
          break;
        case sMCOA_MUL:
          *data++ = sD3DTSS_1|sD3DTSS_COLOROP; *data++ = sD3DTOP_MODULATE;
          *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP; *data++ = sD3DTOP_SELECTARG2;
          break;
        case sMCOA_MUL2:
          *data++ = sD3DTSS_1|sD3DTSS_COLOROP; *data++ = sD3DTOP_MODULATE2X;
          *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP; *data++ = sD3DTOP_SELECTARG2;
          break;
        case sMCOA_MUL4:
          *data++ = sD3DTSS_1|sD3DTSS_COLOROP; *data++ = sD3DTOP_MODULATE4X;
          *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP; *data++ = sD3DTOP_SELECTARG2;
          break;
        case sMCOA_RSUB:
          *data++ = sD3DTSS_1|sD3DTSS_COLOROP; *data++ = sD3DTOP_SUBTRACT;
          *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP; *data++ = sD3DTOP_SELECTARG2;
          break;
        default:
          *data++ = sD3DTSS_1|sD3DTSS_COLOROP; *data++ = sD3DTOP_DISABLE;
          *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP; *data++ = sD3DTOP_DISABLE;
          break;
        }
      }
    }
    break;
#endif
#if sPLAYER
  case sPS_00:
#endif
  case sPS_11:
  case sPS_14:
  case sPS_20:
    // let the code generator do its work
    ps = codegen.GenCode(g_material11_psh,codegenData);
    if(!ps)
      error = 1;

    // setup texture stages for lighting vars
    if(LightFlags & sMLF_MODEMASK)
    {
      sInt dospecular = (SpecialFlags & sMSF_NOSPECULAR) ? 0 : 1;

      // cube normalizes (L+H)
      for(i=0;i<=dospecular;i++)
      {
        TexUse[TCount+i] = -2;
        sInt sam = (TCount+i)*sD3DSAMP_1;
        *data++ = sam+sD3DSAMP_MAGFILTER;    *data++ = sD3DTEXF_LINEAR;
        *data++ = sam+sD3DSAMP_MINFILTER;    *data++ = sD3DTEXF_LINEAR;
        *data++ = sam+sD3DSAMP_MIPFILTER;    *data++ = sD3DTEXF_NONE;
        *data++ = sam+sD3DSAMP_ADDRESSU;     *data++ = sD3DTADDRESS_WRAP;
        *data++ = sam+sD3DSAMP_ADDRESSV;     *data++ = sD3DTADDRESS_WRAP;
        *data++ = (TCount+i)*sD3DTSS_1+sD3DTSS_TEXTURETRANSFORMFLAGS; *data++ = 0;
      }

      // attenuation
      sInt attnTex = TCount+1+dospecular;
      TexUse[attnTex] = -3;
      sInt sam = attnTex * sD3DSAMP_1;
      *data++ = sam+sD3DSAMP_MAGFILTER;   *data++ = sD3DTEXF_LINEAR;
      *data++ = sam+sD3DSAMP_MINFILTER;   *data++ = sD3DTEXF_LINEAR;
      *data++ = sam+sD3DSAMP_MIPFILTER;   *data++ = sD3DTEXF_NONE;
      *data++ = sam+sD3DSAMP_ADDRESSU;    *data++ = sD3DTADDRESS_CLAMP;
      *data++ = sam+sD3DSAMP_ADDRESSV;    *data++ = sD3DTADDRESS_CLAMP;
      *data++ = sam+sD3DSAMP_ADDRESSW;    *data++ = sD3DTADDRESS_CLAMP;
      *data++ = attnTex*sD3DTSS_1+sD3DTSS_TEXTURETRANSFORMFLAGS; *data++ = 0;
    }
    break;

  default:
    sFatal("unknown shader level");
    break;
  }

//*** finish shaders

  *data++ = ~0;
  *data++ = ~0;

  if(!error)
  {
    for(sInt i=0;i<NumSetups;i++)
    {
      Setup[i] = sSystem->MtrlAddSetup(states,vs[i],ps);
      if(Setup[i] == sINVALID)
        error = 1;
    }
  }

  for(sInt i=0;i<NumSetups;i++)
    delete[] vs[i];

  delete[] ps;

  return !error;
}

/****************************************************************************/

void sMaterial11::Set(const sMaterialEnv &env)
{
  sMaterialInstance inst;
  sMatrix vc[9];
  sVector pc[8];

  // matrices: world-view-projection + basic light setup
  sSystem->LastMatrix = env.ModelSpace;
  sSystem->LastMatrix.TransR();

  vc[0].Mul4(env.ModelSpace,sSystem->LastViewProject);    // c0-c3: wvp
  vc[0].Trans4();
  if((LightFlags & sMLF_MODEMASK) == sMLF_BUMPX && env.LightType == sLT_DIRECTIONAL)
    vc[1].i.Rotate3(sSystem->LastMatrix,env.LightDir);      // c4: light dir (mdl)
  else
    vc[1].i.Rotate34(sSystem->LastMatrix,env.LightPos);     // c4: light pos (mdl)

  inst.VSConstants = (sVector *) vc;
  inst.PSConstants = pc;
  inst.NumPSConstants = 0;

  // actual setup
  if(BaseFlags & sMBF_SHADOWMASK)
  {
    // fast path for shadow materials: only update light position
    vc[1].i.w = 0.0f;
    inst.NumVSConstants = 5;
  }
  else
  {
    // light attenuation
    if(env.LightRange)
    {
      vc[1].j.Scale4(vc[1].i,-0.5f / env.LightRange);     // c5: light attn
      vc[1].j.x += 0.5f;
      vc[1].j.y += 0.5f;
      vc[1].j.z += 0.5f;
      vc[1].j.w = 0.5f / env.LightRange;
    }
    else
      vc[1].j.Init(0.5f,0.5f,0.5f,0.0f);

    vc[1].k.Rotate34(sSystem->LastMatrix,env.CameraSpace.l); // c6: cam (mdl)
    vc[1].l.Init(TScale[0],TScale[1],TScale[2],TScale[3]); // c7: tex scale

    inst.NumVSConstants = 8;

    // texture srt (if necessary)
    if(HasTexSRT)
    {
      vc[2].InitSRT(SRT1);                            // c8-c9: tex srt1
      vc[2].Trans4();

      sF32 fs,fc;
      sFSinCos(SRT2[2],fs,fc);

      vc[2].k.Init( SRT2[0]*fc,SRT2[1]*fs,0,SRT2[3]); // c10-c11: tex srt2
      vc[2].l.Init(-SRT2[0]*fs,SRT2[1]*fc,0,SRT2[4]);
      inst.NumVSConstants = 12;
    }

    if(SpecialFlags & sMSF_ENVIMASK)                  // c12-c15: tex special 1
    {
      vc[3].MulA(env.ModelSpace,sSystem->LastCamera);
      vc[3].Trans4();
      inst.NumVSConstants = 16;
    }

    if(SpecialFlags & sMSF_PROJMASK)                  // c16-c19: tex special 2
    {
      sInt mask = SpecialFlags & sMSF_PROJMASK;

      if(mask == sMSF_PROJWORLD)
        vc[4] = env.ModelSpace;
      else if(mask == sMSF_PROJEYE)
        vc[4].MulA(env.ModelSpace,sSystem->LastCamera);

      vc[4].Trans4();
      inst.NumVSConstants = 20;
    }

    if(BaseFlags & sMBF_FOG)       // c20: fog
    {
      sMatrix temp;
      temp.MulA(env.ModelSpace,sSystem->LastCamera);
      temp.Trans4();

      vc[5].i = temp.k;
      vc[5].i.w -= env.FogEnd;
      vc[5].i.Scale4(1.0f / (env.FogStart - env.FogEnd));
      inst.NumVSConstants = 21;
    }

    if((LightFlags & sMLF_MODEMASK) == sMLF_OFF)
    {
      for(sInt i=0;i<4;i++) // c0-c3: colors 0-3
        pc[i].InitColor(Color[i]);

      inst.NumPSConstants = 4;
    }
    else
    {
      pc[0].InitColor(Color[0]);                      // c0: light color
      pc[0].Mul4(env.LightColor);
      pc[0].Scale4(env.LightAmplify);

      // magic values derive from the construction of muls/mads in the
      // pixel shader.
      sF32 usePower = sRange(SpecPower,60.0f,8.0f);
      sF32 invScale = 1.0f / 7.336032f;

      pc[1].x = SpecPower;                            // c1: specular param
      pc[1].z = invScale + (usePower - 8.0f) * 0.01565302f;
      pc[1].w = invScale - pc[1].z;
      inst.NumPSConstants = 2;
    }
  }

  // texture setup
  inst.NumTextures = 4;
  for(sInt i=0;i<4;i++)
  {
    sInt use = TexUse[i];
    inst.Textures[i] = (use >= 0 && use <= 3) ? Tex[use] : use;
  }

  if(ShaderLevel==sPS_00)
    inst.NumPSConstants = 0;

  // set everything
  sSystem->MtrlSetSetup(Setup[env.LightType|((env.Flags&3)<<1)]);
  sSystem->MtrlSetInstance(inst);

  if(BaseFlags & sMBF_FOG)
    sSystem->SetState(sD3DRS_FOGCOLOR,env.FogColor);
}

/****************************************************************************/

void sMaterial11::SetShadowStates(sU32 flags)
{
  sU32 states[32],*data=states;

  *data++ = sD3DRS_CULLMODE;
  *data++ = (flags & sMBF_DOUBLESIDED) ? sD3DCULL_NONE : (flags & sMBF_INVERTCULL?sD3DCULL_CW:sD3DCULL_CCW);
  makeStencilStates(data,flags);
  *data++ = ~0;
  *data++ = ~0;
  
  sSystem->SetStates(states);
}

/****************************************************************************/
