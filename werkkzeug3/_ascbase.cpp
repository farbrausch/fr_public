// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_ascbase.hpp"
#include "_startdx.hpp"

/****************************************************************************/

static sCBufferBase *CurrentCBs[sCBUFFER_MAXSLOT*sCBUFFER_SHADERTYPES];

sCBufferBase::sCBufferBase()
{
  DataPtr = 0;
  DataPersist = 0;
}

void sCBufferBase::SetPtr(void **dataptr,void *data)
{
  DataPtr = dataptr;
  DataPersist = data;
  *DataPtr = DataPersist;
}

void sCBufferBase::SetRegs()
{
  switch(Slot & sCBUFFER_SHADERMASK)
  {
  case sCBUFFER_VS:
    sSystem->MtrlSetVSConstants(RegStart,(sVector*)DataPersist,RegCount);
    break;
  case sCBUFFER_PS:
    sSystem->MtrlSetPSConstants(RegStart,(sVector*)DataPersist,RegCount);
    break;
  case sCBUFFER_GS:
    break;
  }
  if(DataPtr)
    *DataPtr = 0;
}

void sCBufferBase::Modify()
{
  if(DataPtr)
    *DataPtr = DataPersist;
  if(CurrentCBs[Slot]==this)
    CurrentCBs[Slot] = 0;
}

void sCBufferBase::OverridePtr(void *ptr)
{
  DataPtr = 0;
  DataPersist = ptr;
  Modify();
}

sCBufferBase::~sCBufferBase()
{
}

void sResetConstantBuffers()
{
  for(sInt i=0;i<sCBUFFER_MAXSLOT*sCBUFFER_SHADERTYPES;i++)
    CurrentCBs[i] = 0;
}

/****************************************************************************/

sASCMaterial::sASCMaterial()
{
  NameId = 0;
  Flags = sMTRL_ZON|sMTRL_CULLON;
  FuncFlags[0] = sMFF_LESSEQUAL;
  FuncFlags[1] = sMFF_ALWAYS;
  FuncFlags[2] = sMFF_LESSEQUAL;
  FuncFlags[3] = 0;
  BlendColor = sMB_OFF;
  BlendAlpha = sMB_SAMEASCOLOR;
  BlendFactor = 0xffffffff;
  StencilOps[0] = sMSO_KEEP;
  StencilOps[1] = sMSO_KEEP;
  StencilOps[2] = sMSO_KEEP;
  StencilOps[3] = sMSO_KEEP;
  StencilOps[4] = sMSO_KEEP;
  StencilOps[5] = sMSO_KEEP;
  StencilRef = 0;
  AlphaRef = 10;
  StencilMask = 0xFF;
  Setup = sINVALID;

  for(sInt i=0;i<sMTRL_MAXTEX;i++)
  {
    Texture[i] = sINVALID;
    TFlags[i] = 0;
  }
}

sASCMaterial::~sASCMaterial()
{
  if(Setup != sINVALID)
    sSystem->MtrlRemSetup(Setup);
}

static sInt sRenderStateTexture(sU32* data, sInt texstage, sU32 tflags)
{
  sInt tex = texstage * sD3DSAMP_1;
  sU32* start = data;

  if (tflags & sMTF_SRGB)
  {
    *data++ = tex+sD3DSAMP_SRGBTEXTURE; *data++ = 1;
  }
  else
  {
    *data++ = tex+sD3DSAMP_SRGBTEXTURE; *data++ = 0;
  }

  switch(tflags & sMTF_LEVELMASK)
  {
  case sMTF_LEVEL0:
    *data++ = tex+sD3DSAMP_MAGFILTER;    *data++ = sD3DTEXF_POINT;
    *data++ = tex+sD3DSAMP_MINFILTER;    *data++ = sD3DTEXF_POINT;
    *data++ = tex+sD3DSAMP_MIPFILTER;    *data++ = sD3DTEXF_NONE;
    break;
  case sMTF_LEVEL1:
    *data++ = tex+sD3DSAMP_MAGFILTER;    *data++ = sD3DTEXF_LINEAR;
    *data++ = tex+sD3DSAMP_MINFILTER;    *data++ = sD3DTEXF_LINEAR;
    *data++ = tex+sD3DSAMP_MIPFILTER;    *data++ = sD3DTEXF_NONE;
    break;
  case sMTF_LEVEL2:
    *data++ = tex+sD3DSAMP_MAGFILTER;    *data++ = sD3DTEXF_LINEAR;
    *data++ = tex+sD3DSAMP_MINFILTER;    *data++ = sD3DTEXF_LINEAR;
    *data++ = tex+sD3DSAMP_MIPFILTER;    *data++ = sD3DTEXF_LINEAR;
    break;
  case sMTF_LEVEL3:
    *data++ = tex+sD3DSAMP_MAGFILTER;    *data++ = sD3DTEXF_ANISOTROPIC;
    *data++ = tex+sD3DSAMP_MINFILTER;    *data++ = sD3DTEXF_ANISOTROPIC;
    *data++ = tex+sD3DSAMP_MIPFILTER;    *data++ = sD3DTEXF_LINEAR;
    break;
  }

  switch (tflags&sMTF_ADDRMASK)
  {
  case sMTF_TILE:
    *data++ = tex+sD3DSAMP_ADDRESSU;     *data++ = sD3DTADDRESS_WRAP;
    *data++ = tex+sD3DSAMP_ADDRESSV;     *data++ = sD3DTADDRESS_WRAP;
    *data++ = tex+sD3DSAMP_ADDRESSW;     *data++ = sD3DTADDRESS_WRAP;
    break;

  case sMTF_CLAMP:
    *data++ = tex+sD3DSAMP_ADDRESSU;     *data++ = sD3DTADDRESS_CLAMP;
    *data++ = tex+sD3DSAMP_ADDRESSV;     *data++ = sD3DTADDRESS_CLAMP;
    *data++ = tex+sD3DSAMP_ADDRESSW;     *data++ = sD3DTADDRESS_CLAMP;
    break;

  case sMTF_MIRROR:
    *data++ = tex+sD3DSAMP_ADDRESSU;     *data++ = sD3DTADDRESS_MIRROR;
    *data++ = tex+sD3DSAMP_ADDRESSV;     *data++ = sD3DTADDRESS_MIRROR;
    *data++ = tex+sD3DSAMP_ADDRESSW;     *data++ = sD3DTADDRESS_MIRROR;
    break;

  case sMTF_BORDER:
    *data++ = tex+sD3DSAMP_ADDRESSU;     *data++ = sD3DTADDRESS_BORDER;
    *data++ = tex+sD3DSAMP_ADDRESSV;     *data++ = sD3DTADDRESS_BORDER;
    *data++ = tex+sD3DSAMP_ADDRESSW;     *data++ = sD3DTADDRESS_BORDER;
    break;

  default:
    sVERIFYFALSE;
  }

  return (data-start);
}

void sASCMaterial::AddMtrlFlags(sU32 *&data)
{
  sU32* start = data;
  if(BlendColor!=sMB_OFF)
  {
    *data++ = sD3DRS_ALPHABLENDENABLE; *data++ = 1;

    *data++ = sD3DRS_SRCBLEND;         *data++ = (BlendColor>> 0)&15;
    *data++ = sD3DRS_DESTBLEND;        *data++ = (BlendColor>>16)&15;
    *data++ = sD3DRS_BLENDOP;          *data++ = (BlendColor>> 8)&7;
    *data++ = sD3DRS_BLENDFACTOR;      *data++ = BlendFactor;

    if(BlendAlpha==sMB_SAMEASCOLOR)
    {
      *data++ = sD3DRS_SEPARATEALPHABLENDENABLE; *data++ = 0;
    }
    else
    {
      *data++ = sD3DRS_SEPARATEALPHABLENDENABLE; *data++ = 1;

      *data++ = sD3DRS_SRCBLENDALPHA;    *data++ = (BlendAlpha>> 0)&15;
      *data++ = sD3DRS_DESTBLENDALPHA;   *data++ = (BlendAlpha>>16)&15;
      *data++ = sD3DRS_BLENDOPALPHA;     *data++ = (BlendAlpha>> 8)&7;
    }
  }
  else
  {
    *data++ = sD3DRS_ALPHABLENDENABLE; *data++ = 0;
    *data++ = sD3DRS_SEPARATEALPHABLENDENABLE; *data++ = 0;
  }

  switch(Flags & sMTRL_ZMASK)
  {
  case sMTRL_ZOFF:
    *data++ = sD3DRS_ZENABLE;        *data++ = 0;
    break;
  case sMTRL_ZREAD:
    *data++ = sD3DRS_ZENABLE;        *data++ = 1;
    *data++ = sD3DRS_ZWRITEENABLE;   *data++ = 0;
    *data++ = sD3DRS_ZFUNC;          *data++ = FuncFlags[0]+sD3DCMP_NEVER;
    break;
  case sMTRL_ZWRITE:
    *data++ = sD3DRS_ZENABLE;        *data++ = 1;
    *data++ = sD3DRS_ZWRITEENABLE;   *data++ = 1;
    *data++ = sD3DRS_ZFUNC;          *data++ = sD3DCMP_ALWAYS;
    break;
  case sMTRL_ZON:
    *data++ = sD3DRS_ZENABLE;        *data++ = 1;
    *data++ = sD3DRS_ZWRITEENABLE;   *data++ = 1;
    *data++ = sD3DRS_ZFUNC;          *data++ = FuncFlags[0]+sD3DCMP_NEVER;
    break;
  }

  switch(Flags & sMTRL_CULLMASK)
  {
  case sMTRL_CULLOFF:
    *data++ = sD3DRS_CULLMODE;       *data++ = sD3DCULL_NONE;
    break;
  case sMTRL_CULLON:
    *data++ = sD3DRS_CULLMODE;       *data++ = sD3DCULL_CCW;
    break;
  case sMTRL_CULLINV:
    *data++ = sD3DRS_CULLMODE;       *data++ = sD3DCULL_CW;
    break;
  }

  // color write mask
  {
    sU32 mask = sD3DCOLORWRITEENABLE_ALPHA|sD3DCOLORWRITEENABLE_RED|sD3DCOLORWRITEENABLE_GREEN|sD3DCOLORWRITEENABLE_BLUE;
    if (Flags&sMTRL_MSK_ALPHA)
      mask &= ~sD3DCOLORWRITEENABLE_ALPHA;
    if (Flags&sMTRL_MSK_RED)
      mask &= ~sD3DCOLORWRITEENABLE_RED;
    if (Flags&sMTRL_MSK_GREEN)
      mask &= ~sD3DCOLORWRITEENABLE_GREEN;
    if (Flags&sMTRL_MSK_BLUE)
      mask &= ~sD3DCOLORWRITEENABLE_BLUE;

    *data++ = sD3DRS_COLORWRITEENABLE;;
    *data++ = mask;
  }

  // stencil renderstates
  if (Flags & (sMTRL_STENCILSS | sMTRL_STENCILDS))
  {
    *data++ = sD3DRS_STENCILENABLE; *data++ = 1; 
    *data++ = sD3DRS_STENCILMASK; *data++ = StencilMask;
    *data++ = sD3DRS_STENCILREF; *data++ = StencilRef;
    *data++ = sD3DRS_STENCILFUNC; *data++ = FuncFlags[sMFT_STENCIL]+sD3DCMP_NEVER;
    *data++ = sD3DRS_STENCILFAIL; *data++ = StencilOps[sMSI_CWFAIL]+sD3DSTENCILOP_KEEP;
    *data++ = sD3DRS_STENCILZFAIL; *data++ = StencilOps[sMSI_CWZFAIL]+sD3DSTENCILOP_KEEP;
    *data++ = sD3DRS_STENCILPASS; *data++ = StencilOps[sMSI_CWPASS]+sD3DSTENCILOP_KEEP;
    if ( Flags & sMTRL_STENCILDS ) 
    {
      *data++ = sD3DRS_TWOSIDEDSTENCILMODE; *data++ = 1;
      *data++ = sD3DRS_CCW_STENCILFAIL; *data++ = StencilOps[sMSI_CCWFAIL]+sD3DSTENCILOP_KEEP;
      *data++ = sD3DRS_CCW_STENCILZFAIL; *data++ = StencilOps[sMSI_CCWZFAIL]+sD3DSTENCILOP_KEEP;
      *data++ = sD3DRS_CCW_STENCILPASS; *data++ = StencilOps[sMSI_CCWPASS]+sD3DSTENCILOP_KEEP;
    }
    else
    {
      *data++ = sD3DRS_TWOSIDEDSTENCILMODE; *data++ = 0;
    }
  }
  else
  {
    *data++ = sD3DRS_STENCILENABLE; *data++ = 0; 
  }

  for(sInt i=0;i<sMTRL_MAXTEX;i++)
  {
    if(Texture[i] != sINVALID || (TFlags[i]&sMTF_EXTERN))
      data += sRenderStateTexture(data, i, TFlags[i]);
  }

  if(FuncFlags[sMFT_ALPHA]!=sMFF_ALWAYS)
  { 
    *data++ = sD3DRS_ALPHATESTENABLE; *data++ = 1;
    *data++ = sD3DRS_ALPHAFUNC; *data++ = FuncFlags[sMFT_ALPHA]+1;
    *data++ = sD3DRS_ALPHAREF; *data++ = AlphaRef; 
  }
  else
  {
    *data++ = sD3DRS_ALPHATESTENABLE; *data++ = 0;
  }
}

void sASCMaterial::Set(sCBufferBase **cbuffers,sInt cbcount)
{
  // if you hit this, you forgot to call Prepare()!
  sVERIFY(Setup != sINVALID);
  sVERIFY(sMTRL_MAXTEX < MAX_TEXSTAGE);

  // prepare instance
  sMaterialInstance inst;
  inst.NumVSConstants = 0;
  inst.NumPSConstants = 0;
  inst.NumTextures = sMTRL_MAXTEX;

  for(sInt i=0;i<sMTRL_MAXTEX;i++)
    inst.Textures[i] = Texture[i];

  // set setup+instance
  sSystem->MtrlSetSetup(Setup);
  sSystem->MtrlSetInstance(inst);

  // set constant buffers
  for(sInt i=0;i<cbcount;i++)
  {
    sCBufferBase *cb = cbuffers[i];
    sVERIFY(cb->Slot<sCBUFFER_MAXSLOT*sCBUFFER_SHADERTYPES);
    if(cb != CurrentCBs[cb->Slot])
    {
      cb->SetRegs();
      CurrentCBs[cb->Slot] = cb;
    }
  }
}

/****************************************************************************/
