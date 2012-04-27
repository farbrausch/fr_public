/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "wz4_rovemtrl.hpp"
#include "wz4_rovemtrl_ops.hpp"
#include "base/graphics.hpp"

/****************************************************************************/

static const sF32 Det3x3(const sMatrix34CM *mat)
{
  return (mat->x.x*mat->y.y*mat->z.z + mat->y.x*mat->z.y*mat->x.z + mat->z.x*mat->x.y*mat->y.z)
       - (mat->z.x*mat->y.y*mat->x.z + mat->x.x*mat->z.y*mat->y.z + mat->y.x*mat->x.y*mat->z.z);
}

/****************************************************************************/

void RoveMtrlType_::Init()
{
  for(sInt i=0;i<sMAX_LIGHTENV;i++)
    LightEnv[i] = new LightEnv_;

  sImageData emptyCube;
  emptyCube.Init2(sTEX_CUBE|sTEX_ARGB8888,0,1,1,1);
  sSetMem(emptyCube.Data,0,emptyCube.GetByteSize());
  EmptyCube = emptyCube.CreateTexture()->CastTexCube();

  Wz4MtrlType->RegisterMtrl(this);
}

void RoveMtrlType_::Exit()
{
  sDelete(EmptyCube);
  for(sInt i=0;i<sMAX_LIGHTENV;i++)
    delete LightEnv[i];
}

void RoveMtrlType_::PrepareViewR(sViewport &view)
{
  for(sInt i=0;i<sMAX_LIGHTENV;i++)
  {
    sVector30 d;
    d = -view.Camera.k;
    d.Unit();
    LightEnv[i]->cbv.Data->mvp = view.ModelScreen;
    LightEnv[i]->cbv.Data->mv = view.ModelView;
    LightEnv[i]->cbv.Data->EyePos = view.Camera.l;
    LightEnv[i]->cbv.Modify();
    LightEnv[i]->cbp.Data->FogColor.InitColor(0x80ffffff);
    LightEnv[i]->cbp.Data->FogPara.Init(0,0,0,0);
    LightEnv[i]->cbp.Data->ClipPlane[0].Init(0,0,0,1);
    LightEnv[i]->cbp.Data->ClipPlane[1].Init(0,0,0,1);
    LightEnv[i]->cbp.Data->ClipPlane[2].Init(0,0,0,1);
    LightEnv[i]->cbp.Data->ClipPlane[3].Init(0,0,0,1);
    LightEnv[i]->cbp.Data->Ambient.Init(0.2f,0.2f,0.2f,0.2f);
    LightEnv[i]->cbp.Data->LightPos[0].Init(3,0,0,0);
    LightEnv[i]->cbp.Data->LightPos[1].Init(4,0,0,0);
    LightEnv[i]->cbp.Data->LightPos[2].Init(5,0,0,0);
    LightEnv[i]->cbp.Data->LightCol[0].Init(0.8f,0,0,0);
    LightEnv[i]->cbp.Data->LightCol[1].Init(0.8f,0,0,0);
    LightEnv[i]->cbp.Data->LightCol[2].Init(0.8f,0,0,0);
    LightEnv[i]->cbp.Data->LightInvSqRange.Init(0.0f,0.0f,0.0f,0.0f);
    LightEnv[i]->cbp.Modify();
    
    LightEnv[i]->LightCube = 0;
    LightEnv[i]->TexTrans.Init();
  }

  ViewMatrix = view.View;
}

/****************************************************************************/

RoveMtrl::RoveMtrl()
{
  Type = RoveMtrlType;
  sClear(Shaders);
  sClear(Tex);
  sClear(TFlags);
  Extras = 0;
  DetailTexSpace = 0;
  
  cb.Data->texmat[0].Init(1,0,0,0);
  cb.Data->texmat[1].Init(0,1,0,0);
  cb.Data->texmat[2].Init(1,0,0,0);
  cb.Data->texmat[3].Init(0,1,0,0);
}

RoveMtrl::~RoveMtrl()
{
  for(sInt i=0;i<3;i++)
    delete Shaders[i];
  Tex[0]->Release();
  Tex[1]->Release();
}

void RoveMtrl::Set(sInt flags,sInt index,const sMatrix34CM *mat,sInt SkinMatCount,const sMatrix34CM *SkinMats,sInt *SkinMatMap)
{
  switch(flags & sRF_TARGET_MASK)
  {
  case sRF_TARGET_WIRE:
    Wz4MtrlType->SetDefaultShader(flags|sRF_MATRIX_ONE,mat,0,SkinMatCount,SkinMats,SkinMatMap);
    break;

  case sRF_TARGET_ZONLY:
  case sRF_TARGET_ZNORMAL:
  case sRF_TARGET_DIST:
    {
      sInt variant = 0;
      sF32 det = 1;
      if(mat)
        det = Det3x3(mat);
      switch(Flags & sMTRL_CULLMASK)
      {
      case sMTRL_CULLON:  variant = det<0 ? 2 : 1; break;
      case sMTRL_CULLINV: variant = det<0 ? 1 : 2; break;
      }

      Wz4MtrlType->SetDefaultShader(flags,mat,variant,SkinMatCount,SkinMats,SkinMatMap);
    }
    break;

  case sRF_TARGET_MAIN:
    {
      sMatrix34 tmat,nmat;

      RoveMtrlType_::LightEnv_ *env = RoveMtrlType->LightEnv[index];
      sCBufferBase *modelcb;
      if(mat)
      {
        env->cbm.Data->m[0] = mat->x;
        env->cbm.Data->m[1] = mat->y;
        env->cbm.Data->m[2] = mat->z;
        env->cbm.Modify();
        modelcb = &env->cbm;
      }
      else
      {
        modelcb = &Wz4MtrlType->IdentCBModel;
      }
      
      tmat = TexTrans[0];
      cb.Data->texmat[0].Init(tmat.i.x,tmat.j.x,tmat.k.x,tmat.l.x);
      cb.Data->texmat[1].Init(tmat.i.y,tmat.j.y,tmat.k.y,tmat.l.y);
      switch(DetailTexSpace&7)
      {
      case 0:
        tmat = TexTrans[1];
        break;
      case 1:
        tmat = sMatrix34(*mat)*TexTrans[1];
        break;
      case 2:
        tmat = sMatrix34(*mat)*TexTrans[1]*SimpleMtrlType->ViewMatrix;
        break;
      case 3:
        tmat = SimpleMtrlType->ViewMatrix;
        tmat.l.Init(0,0,0);
        tmat = tmat*TexTrans[1];
        tmat.i.x *= 0.5f;
        tmat.j.x *= 0.5f;
        tmat.k.x *= 0.5f;
        tmat.l.x *= 0.5f;
        tmat.i.y *=-0.5f;
        tmat.j.y *=-0.5f;
        tmat.k.y *=-0.5f;
        tmat.l.y *=-0.5f;
        tmat.l.x += 0.5f;
        tmat.l.y += 0.5f;
        break;
      case 4:
        tmat = env->TexTrans;
        break;
      }

      cb.Data->texmat[2].Init(tmat.i.x,tmat.j.x,tmat.k.x,tmat.l.x);
      cb.Data->texmat[3].Init(tmat.i.y,tmat.j.y,tmat.k.y,tmat.l.y);
      cb.Modify();

      sCBufferBase *cbptr[4];
      cbptr[0] = &env->cbv;
      cbptr[1] = modelcb;
      cbptr[2] = &env->cbp;
      cbptr[3] = &cb;

      sF32 det = 1;
      if(mat)
        det = Det3x3(mat);
      sInt variant = det>0 ? 0 : 1;
      sInt sh;

      switch(flags & sRF_MATRIX_MASK)
      {
        default:
        case sRF_MATRIX_ONE:      sh = 0; break;
        case sRF_MATRIX_BONE:     sh = 1; break;
        case sRF_MATRIX_INSTANCE: sh = 2; break;
        case sRF_MATRIX_BONEINST: sh = 3; break;
      }

      Shaders[sh]->Texture[0] = env->LightCube ? env->LightCube : RoveMtrlType->EmptyCube;
      Shaders[sh]->Set(cbptr,4,variant);
    }
    break;
  }
}

void RoveMtrl::SetMtrl(sInt flags,sInt extras)
{
  Flags = flags;
  Extras = extras;
}

void RoveMtrl::SetTex(sInt stage,Texture2D *tex,sInt tflags)
{
  tflags = sConvertOldUvFlags(tflags&0xffff);
  sRelease(Tex[stage]);
  Tex[stage] = tex; tex->AddRef();
  TFlags[stage] = tflags;
}

void RoveMtrl::Prepare()
{
  sU32 descdata[32],*desc=descdata;

  *desc++ = sVF_POSITION|sVF_F3;
  *desc++ = sVF_NORMAL|sVF_F3;
  if(Tex[0])
    *desc++ = sVF_UV0|sVF_F2;
  *desc = 0;
  Formats[0] = sCreateVertexFormat(descdata);

  desc[0] = sVF_BONEINDEX|sVF_I4;
  desc[1] = sVF_BONEWEIGHT|sVF_I4;
  desc[2] = 0;
  Formats[1] = sCreateVertexFormat(descdata);

  desc[0] = sVF_STREAM1|sVF_UV5|sVF_F4;
  desc[1] = sVF_STREAM1|sVF_UV6|sVF_F4;
  desc[2] = sVF_STREAM1|sVF_UV7|sVF_F4;
  desc[3] = 0;
  Formats[2] = sCreateVertexFormat(descdata);

  for(sInt i=0;i<3;i++)
  {
    Shaders[i] = new RoveShader;
    Shaders[i]->Flags = Flags;
    Shaders[i]->BlendColor = 0;
    Shaders[i]->TFlags[0] = sMTF_LEVEL2;
    Shaders[i]->TFlags[1] = TFlags[0];
    Shaders[i]->TFlags[2] = TFlags[1];
    Shaders[i]->Texture[0] = RoveMtrlType->EmptyCube;
    if(Tex[0]) Shaders[i]->Texture[1] = Tex[0]->Texture;
    if(Tex[1]) Shaders[i]->Texture[2] = Tex[1]->Texture;
    Shaders[i]->Extra = Extras;

    Shaders[i]->InitVariants(2);
    Shaders[i]->SetVariant(0);
    sInt cull = Shaders[i]->Flags & sMTRL_CULLMASK;
    if(cull==sMTRL_CULLON || cull==sMTRL_CULLINV)
      Shaders[i]->Flags ^= sMTRL_CULLON ^ sMTRL_CULLINV;
    Shaders[i]->SetVariant(1);
    Shaders[i]->Prepare(Formats[i]);
  }
}

sVertexFormatHandle *RoveMtrl::GetFormatHandle(sInt flags)
{
  switch(flags)
  {
  case sRF_TARGET_MAIN | sRF_MATRIX_ONE:        return Formats[0];
  case sRF_TARGET_MAIN | sRF_MATRIX_BONE:       return Formats[1];
  case sRF_TARGET_MAIN | sRF_MATRIX_INSTANCE:   return Formats[2];
  case sRF_TARGET_MAIN | sRF_MATRIX_BONEINST:   return Formats[3];
  default: return Wz4MtrlType->GetDefaultFormat(flags);
  }
}

sBool RoveMtrl::SkipPhase(sInt flags,sInt lightenv)
{
  if( ( (flags & sRF_TARGET_MASK)==sRF_TARGET_ZONLY || 
        (flags & sRF_TARGET_MASK)==sRF_TARGET_ZNORMAL || 
        (flags & sRF_TARGET_MASK)==sRF_TARGET_DIST
       ) && (Flags & sMTRL_NOZRENDER)) return 1;
  return 0;
}

/****************************************************************************/

RNRoveMtrlEnv::RNRoveMtrlEnv()
{
  Anim.Init(Wz4RenderType->Script);
  LightCube = 0;
}

RNRoveMtrlEnv::~RNRoveMtrlEnv()
{
  LightCube->Release();
}

void RNRoveMtrlEnv::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  SimulateChilds(ctx);
}

static sF32 InvSqRange(sF32 x)
{
  return x ? 1.0f / (x*x) : 0.0f;
}

void RNRoveMtrlEnv::Render(Wz4RenderContext *ctx)
{
  RoveMtrlType_::LightEnv_ *env = RoveMtrlType->LightEnv[Para.Index];
  RoveShaderPEnv *cbp = env->cbp.Data;

  sVector4 col0,col1,col2,col3;
  col0.InitColor(Para.Color0); col0 *= Para.Color0Amp;
  col1.InitColor(Para.Color1); col1 *= Para.Color1Amp;
  col2.InitColor(Para.Color2); col2 *= Para.Color2Amp;
  col3.InitColor(Para.Color3); col3 *= Para.Color3Amp;

  cbp->Ambient.InitColor(Para.Ambient);
  cbp->LightPos[0].Init(Para.Pos0.x,Para.Pos1.x,Para.Pos2.x,Para.Pos3.x);
  cbp->LightPos[1].Init(Para.Pos0.y,Para.Pos1.y,Para.Pos2.y,Para.Pos3.y);
  cbp->LightPos[2].Init(Para.Pos0.z,Para.Pos1.z,Para.Pos2.z,Para.Pos3.z);
  cbp->LightCol[0].Init(col0.x,col1.x,col2.x,col3.x);
  cbp->LightCol[1].Init(col0.y,col1.y,col2.y,col3.y);
  cbp->LightCol[2].Init(col0.z,col1.z,col2.z,col3.z);
  cbp->LightInvSqRange.Init(InvSqRange(Para.Range0),InvSqRange(Para.Range1),InvSqRange(Para.Range2),InvSqRange(Para.Range3));

  sVector4 clip[4];
  sInt clipbits = Para.ClipPlanes;
  if(clipbits & 0x10) clipbits |= 0x08;     // bit 4 (0x10) is set to calculate clip3 fogging
  if((clipbits&15)==0)
  {
    cbp->ClipPlane[0].Init(0,0,0,1);
    cbp->ClipPlane[1].Init(0,0,0,1);
    cbp->ClipPlane[2].Init(0,0,0,1);
    cbp->ClipPlane[3].Init(0,0,0,1);
  }
  else
  {
    sMatrix44 mat;
    mat = ctx->View.Camera;
    mat.Trans4();

    clip[0].Init(Para.Plane0Dir.x,Para.Plane0Dir.y,Para.Plane0Dir.z,Para.Plane0Dist);
    clip[1].Init(Para.Plane1Dir.x,Para.Plane1Dir.y,Para.Plane1Dir.z,Para.Plane1Dist);
    clip[2].Init(Para.Plane2Dir.x,Para.Plane2Dir.y,Para.Plane2Dir.z,Para.Plane2Dist);
    clip[3].Init(Para.Plane3Dir.x,Para.Plane3Dir.y,Para.Plane3Dir.z,Para.Plane3Dist);
    for(sInt i=0;i<4;i++)
    {
      if(clipbits&(1<<i))
      {
        sF32 e = clip[i].x*clip[i].x + clip[i].y*clip[i].y + clip[i].z*clip[i].z;
        if(e<1.0e-20f)
        {
          clip[i].x = 1;
          clip[i].y = 0;
          clip[i].z = 0;
        }
        else
        {
          e = sRSqrt(e);
          clip[i].x *= e;
          clip[i].y *= e;
          clip[i].z *= e;
        }      
      }
      else
      {
        clip[i].Init(0,0,0,1);
      }
      cbp->ClipPlane[i] = clip[i];
    }
  }

  cbp->FogColor.InitColor(Para.FogColor);
  if(Para.FogNear < Para.FogFar)
  {
    cbp->FogPara.x = -Para.FogNear;
    cbp->FogPara.y = 1/(Para.FogFar-Para.FogNear);
    if(clipbits & 0x08)
      cbp->FogPara.z = -sAbs(1.0f/clip[3].z+0.001f);
    else
      cbp->FogPara.z = 0;
    cbp->FogPara.w = 0;
  }
  else
  {
    cbp->FogPara.Init(0,0,0,0);
  }

  env->cbp.Modify();

  sSRT srt;
  srt.Scale = Para.TexScale;
  srt.Rotate = Para.TexRot;
  srt.Translate = Para.TexTrans;
  srt.MakeMatrix(env->TexTrans);

  env->LightCube = LightCube ? LightCube->Texture : 0;
}

/****************************************************************************/
/****************************************************************************/
