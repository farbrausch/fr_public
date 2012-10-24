/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "wz4frlib/wz4_mtrl2.hpp"
#include "wz4frlib/wz4_mtrl2_ops.hpp"
#include "shadercomp/shadercomp.hpp"
#include "shadercomp/shaderdis.hpp"

/****************************************************************************/

void Wz4MtrlType_::Init()
{
  Torus = 0;

  sClear(Shaders);
  sClear(Formats);

  IdentCBModel.Data->m[0].Init(1,0,0,0);
  IdentCBModel.Data->m[1].Init(0,1,0,0);
  IdentCBModel.Data->m[2].Init(0,0,1,0);

  if(Wz4MtrlType == 0)      // this is not a derived class!
  {
    sU32 descw[] = { sVF_POSITION,sVF_NORMAL,sVF_COLOR0,0 };
    sU32 descwi[] = { sVF_POSITION,sVF_NORMAL,sVF_COLOR0, sVF_STREAM1|sVF_UV5|sVF_F4,sVF_STREAM1|sVF_UV6|sVF_F4,sVF_STREAM1|sVF_UV7|sVF_F4, 0 };
    sU32 descwip[] = { sVF_POSITION,sVF_NORMAL,sVF_COLOR0, sVF_STREAM1|sVF_UV5|sVF_F4,sVF_STREAM1|sVF_UV6|sVF_F4,sVF_STREAM1|sVF_UV7|sVF_F4,sVF_STREAM1|sVF_UV4|sVF_F4, 0 };

    sU32 descz[] = { sVF_POSITION,sVF_NORMAL,0 };
    sU32 desczb[] = { sVF_POSITION,sVF_NORMAL,sVF_BONEINDEX|sVF_I4,sVF_BONEWEIGHT|sVF_I4,0 };
    sU32 desczi[] = { sVF_POSITION,sVF_NORMAL,sVF_STREAM1|sVF_UV5|sVF_F4,sVF_STREAM1|sVF_UV6|sVF_F4,sVF_STREAM1|sVF_UV7|sVF_F4, 0 };
    sU32 desczip[] = { sVF_POSITION,sVF_NORMAL,sVF_STREAM1|sVF_UV5|sVF_F4,sVF_STREAM1|sVF_UV6|sVF_F4,sVF_STREAM1|sVF_UV7|sVF_F4,sVF_STREAM1|sVF_UV4|sVF_F4, 0 };

    sU32 desce[] = { sVF_POSITION,0 };
    sU32 desceb[] = { sVF_POSITION,sVF_BONEINDEX|sVF_I4,sVF_BONEWEIGHT|sVF_I4,0 };
    sU32 descei[] = { sVF_POSITION,sVF_STREAM1|sVF_UV5|sVF_F4,sVF_STREAM1|sVF_UV6|sVF_F4,sVF_STREAM1|sVF_UV7|sVF_F4, 0 };
    sU32 desceip[] = { sVF_POSITION,sVF_STREAM1|sVF_UV5|sVF_F4,sVF_STREAM1|sVF_UV6|sVF_F4,sVF_STREAM1|sVF_UV7|sVF_F4,sVF_STREAM1|sVF_UV4|sVF_F4, 0 };

    sVertexFormatHandle *WireFormat = sCreateVertexFormat(descw);
    sVertexFormatHandle *WireFormatInst = sCreateVertexFormat(descwi);
    sVertexFormatHandle *WireFormatInstPlus = sCreateVertexFormat(descwip);
    sVertexFormatHandle *ZFormat = sCreateVertexFormat(descz);
    sVertexFormatHandle *ZFormatBone = sCreateVertexFormat(desczb);
    sVertexFormatHandle *ZFormatInst = sCreateVertexFormat(desczi);
    sVertexFormatHandle *ZFormatInstPlus = sCreateVertexFormat(desczip);
    sVertexFormatHandle *ErrorFormat = sCreateVertexFormat(desce);
    sVertexFormatHandle *ErrorFormatBone = sCreateVertexFormat(desceb);
    sVertexFormatHandle *ErrorFormatInst = sCreateVertexFormat(descei);
    sVertexFormatHandle *ErrorFormatInstPlus = sCreateVertexFormat(desceip);

    Shaders[sRF_TARGET_WIRE   |sRF_MATRIX_ONE]       = new Wz4MtrlWireframe;
    Shaders[sRF_TARGET_WIRE   |sRF_MATRIX_ONE]       ->Flags = sMTRL_CULLOFF | sMTRL_ZON;
    Shaders[sRF_TARGET_WIRE   |sRF_MATRIX_ONE]       ->BlendColor = sMB_ALPHA;
    Formats[sRF_TARGET_WIRE   |sRF_MATRIX_ONE]       = WireFormat;
    Shaders[sRF_TARGET_WIRE   |sRF_MATRIX_INSTANCE]  = new Wz4MtrlWireframe;
    Shaders[sRF_TARGET_WIRE   |sRF_MATRIX_INSTANCE]  ->Flags = sMTRL_CULLOFF | sMTRL_ZON;
    Formats[sRF_TARGET_WIRE   |sRF_MATRIX_INSTANCE]  = WireFormatInst;
    Shaders[sRF_TARGET_WIRE   |sRF_MATRIX_INSTPLUS]  = new Wz4MtrlWireframe;
    Shaders[sRF_TARGET_WIRE   |sRF_MATRIX_INSTPLUS]  ->Flags = sMTRL_CULLOFF | sMTRL_ZON;
    Formats[sRF_TARGET_WIRE   |sRF_MATRIX_INSTPLUS]  = WireFormatInstPlus;

    Shaders[sRF_TARGET_ZNORMAL|sRF_MATRIX_ONE]       = new Wz4MtrlZOnly;
    Formats[sRF_TARGET_ZNORMAL|sRF_MATRIX_ONE]       = ZFormat;
    Shaders[sRF_TARGET_ZNORMAL|sRF_MATRIX_BONE]      = new Wz4MtrlZOnly;
    Formats[sRF_TARGET_ZNORMAL|sRF_MATRIX_BONE]      = ZFormatBone;
    Shaders[sRF_TARGET_ZNORMAL|sRF_MATRIX_INSTANCE]  = new Wz4MtrlZOnly;
    Formats[sRF_TARGET_ZNORMAL|sRF_MATRIX_INSTANCE]  = ZFormatInst;
    Shaders[sRF_TARGET_ZNORMAL|sRF_MATRIX_BONEINST]  = new Wz4MtrlZOnly;
    Formats[sRF_TARGET_ZNORMAL|sRF_MATRIX_BONEINST]  = ZFormatInst;
    Shaders[sRF_TARGET_ZNORMAL|sRF_MATRIX_INSTPLUS]  = new Wz4MtrlZOnly;
    Formats[sRF_TARGET_ZNORMAL|sRF_MATRIX_INSTPLUS]  = ZFormatInstPlus;

    Shaders[sRF_TARGET_ZONLY  |sRF_MATRIX_ONE]       = new Wz4MtrlZOnly;
    Formats[sRF_TARGET_ZONLY  |sRF_MATRIX_ONE]       = ZFormat;
    Shaders[sRF_TARGET_ZONLY  |sRF_MATRIX_BONE]      = new Wz4MtrlZOnly;
    Formats[sRF_TARGET_ZONLY  |sRF_MATRIX_BONE]      = ZFormatBone;
    Shaders[sRF_TARGET_ZONLY  |sRF_MATRIX_INSTANCE]  = new Wz4MtrlZOnly;
    Formats[sRF_TARGET_ZONLY  |sRF_MATRIX_INSTANCE]  = ZFormatInst;
    Shaders[sRF_TARGET_ZONLY  |sRF_MATRIX_BONEINST]  = new Wz4MtrlZOnly;
    Formats[sRF_TARGET_ZONLY  |sRF_MATRIX_BONEINST]  = ZFormatInst;
    Shaders[sRF_TARGET_ZONLY  |sRF_MATRIX_INSTPLUS]  = new Wz4MtrlZOnly;
    Formats[sRF_TARGET_ZONLY  |sRF_MATRIX_INSTPLUS]  = ZFormatInstPlus;

    Shaders[sRF_TARGET_DIST   |sRF_MATRIX_ONE]       = new Wz4MtrlZOnly;
    Formats[sRF_TARGET_DIST   |sRF_MATRIX_ONE]       = ZFormat;
    Shaders[sRF_TARGET_DIST   |sRF_MATRIX_BONE]      = new Wz4MtrlZOnly;
    Formats[sRF_TARGET_DIST   |sRF_MATRIX_BONE]      = ZFormatBone;
    Shaders[sRF_TARGET_DIST   |sRF_MATRIX_INSTANCE]  = new Wz4MtrlZOnly;
    Formats[sRF_TARGET_DIST   |sRF_MATRIX_INSTANCE]  = ZFormatInst;
    Shaders[sRF_TARGET_DIST   |sRF_MATRIX_BONEINST]  = new Wz4MtrlZOnly;
    Formats[sRF_TARGET_DIST   |sRF_MATRIX_BONEINST]  = ZFormatInst;
    Shaders[sRF_TARGET_DIST   |sRF_MATRIX_INSTPLUS]  = new Wz4MtrlZOnly;
    Formats[sRF_TARGET_DIST   |sRF_MATRIX_INSTPLUS]  = ZFormatInstPlus;

    Shaders[sRF_TARGET_MAIN   |sRF_MATRIX_ONE]       = new Wz4MtrlError;
    Formats[sRF_TARGET_MAIN   |sRF_MATRIX_ONE]       = ErrorFormat;
    Shaders[sRF_TARGET_MAIN   |sRF_MATRIX_BONE]      = new Wz4MtrlError;
    Formats[sRF_TARGET_MAIN   |sRF_MATRIX_BONE]      = ErrorFormatBone;
    Shaders[sRF_TARGET_MAIN   |sRF_MATRIX_INSTANCE]  = new Wz4MtrlError;
    Formats[sRF_TARGET_MAIN   |sRF_MATRIX_INSTANCE]  = ErrorFormatInst;
    Shaders[sRF_TARGET_MAIN   |sRF_MATRIX_BONEINST]  = new Wz4MtrlError;
    Formats[sRF_TARGET_MAIN   |sRF_MATRIX_BONEINST]  = ErrorFormatInst;
    Shaders[sRF_TARGET_MAIN   |sRF_MATRIX_INSTPLUS]  = new Wz4MtrlError;
    Formats[sRF_TARGET_MAIN   |sRF_MATRIX_INSTPLUS]  = ErrorFormatInstPlus;

    for(sInt i=0;i<sRF_TOTAL;i++)
    {
      sMaterial *mtrl = Shaders[i];
      if(mtrl)
      {
        sInt zmode = -1;

        if((i&sRF_TARGET_MASK)==sRF_TARGET_ZNORMAL) zmode = 0;
        if((i&sRF_TARGET_MASK)==sRF_TARGET_ZONLY) zmode = 1;
        if((i&sRF_TARGET_MASK)==sRF_TARGET_DIST) zmode = 2;

        if(zmode>=0)
        {
          ((Wz4MtrlZOnly*)mtrl)->SimpleZ = zmode;
        }
        mtrl->InitVariants(3);
        mtrl->Flags = sMTRL_CULLOFF | sMTRL_ZON;
        mtrl->SetVariant(0);
        mtrl->Flags = sMTRL_CULLON | sMTRL_ZON;
        mtrl->SetVariant(1);
        mtrl->Flags = sMTRL_CULLINV | sMTRL_ZON;
        mtrl->SetVariant(2);

        mtrl->Prepare(Formats[i]);
      }
    }
  }
}

void Wz4MtrlType_::Exit()
{
  delete Torus;
  for(sInt i=0;i<sRF_TOTAL;i++)
    delete Shaders[i];
}


void Wz4MtrlType_::Show(wObject *obj,wPaintInfo &pi)
{
  /*
  if(this!=Wz4MtrlType)
  {
    Wz4MtrlType->Show(obj,pi);
  }
  else */
  {
    Wz4Mtrl *mtrl;

    sSetTarget(sTargetPara(sCLEAR_ALL,pi.BackColor,pi.Spec));
    pi.View->SetTargetCurrent();
    pi.View->SetZoom(pi.Zoom3D);
    pi.View->Prepare();

    if(obj && obj->IsType(Wz4MtrlType))
    {
      mtrl = (Wz4Mtrl *) obj;

      if(Torus==0 || Torus->GetFormat()!=mtrl->GetFormatHandle(sRF_TARGET_MAIN|sRF_MATRIX_ONE))
      {
        delete Torus;
        Torus = new sGeometry(sGF_INDEX16|sGF_TRILIST,mtrl->GetFormatHandle(sRF_TARGET_MAIN|sRF_MATRIX_ONE));
        Torus->LoadTorus(16,24,2,0.5f);
         
      }

      Wz4MtrlType->PrepareView(*pi.View);
      mtrl->Set(sRF_TARGET_MAIN|sRF_MATRIX_ONE,0,0,0,0,0);

      Torus->Draw();
    }
  }
}

void Wz4MtrlType_::PrepareRender(Wz4RenderContext *ctx)
{
  Wz4MtrlType_ *mtrl;
  sFORALL(Materials,mtrl)
    mtrl->PrepareRenderR(ctx);
}

void Wz4MtrlType_::BeginRender(Wz4RenderContext *ctx)
{
  Wz4MtrlType_ *mtrl;
  sFORALL(Materials,mtrl)
    mtrl->BeginRenderR(ctx);
}

void Wz4MtrlType_::EndRender(Wz4RenderContext *ctx)
{
  Wz4MtrlType_ *mtrl;
  sFORALL(Materials,mtrl)
    mtrl->EndRenderR(ctx);
}

void Wz4MtrlType_::PrepareView(sViewport &view)
{
  Wz4MtrlType_ *mtrl;

//  view.Prepare();
  View = view.View;
  Proj = view.Proj;
  VP = view.ModelScreen;

  sFORALL(Materials,mtrl)
    mtrl->PrepareViewR(view);

  WireCB.Data->MVP = view.ModelScreen;
  WireCB.Data->scale.Init
  (
    3.0f*(2.0f/view.Target.SizeX()),
    3.0f*(2.0f/view.Target.SizeY()),
    -0.0003f,
    0
  );
  WireCB.Modify();

  ZOnlyCB.Data->mvp = view.ModelScreen;
  ZOnlyCB.Data->mv = view.ModelView;
  ZOnlyCB.Data->proj.Init(view.Proj.k.z,view.Proj.l.z/view.ClipFar,0,0);
  ZOnlyCB.Data->center = view.Camera.l;
  ZOnlyCB.Modify();
}

void Wz4MtrlType_::RegisterMtrl(Wz4MtrlType_ *mtrl)
{
  Materials.AddTail(mtrl);
}

sCBufferBase *Wz4MtrlType_::MakeSkinCB(sInt SkinMatCount,const sMatrix34CM *SkinMats,sInt *SkinMatMap,sInt cbreg)
{
  if(SkinMatCount==0 || SkinMats==0)
    return 0;

  if(sRENDERER==sRENDER_DX9)
  {
    if(SkinMatMap==0)
    {
      sSetVSParam(32,SkinMatCount*3,&SkinMats[0].x);
    }
    else
    {
      sInt count=SkinMatCount;
      sVERIFY(count);
      sInt starti=0;
      sInt startm=SkinMatMap[0];
      for(sInt i=1;i<count;i++)
      {
        sInt ind = SkinMatMap[i];
        if (ind!=startm+i-starti)
        {
          sSetVSParam(32+starti*3,3*(i-starti),&SkinMats[startm].x);
          startm=ind;
          starti=i;        
        }
      }
      sSetVSParam(32+starti*3,3*(count-starti),&SkinMats[startm].x);
    }
    return 0;
  }
  else if(sRENDERER==sRENDER_DX11)
  {
    SkinCBBuffer.Resize(SkinMatCount);
    SkinCB.SetCfg(sCBUFFER_VS+cbreg,0,SkinMatCount*3);
    SkinCB.SetPtr((void **)&SkinCBPtr,SkinCBBuffer.GetData());
    SkinCB.RegStart = 0;
    SkinCB.RegCount = SkinMatCount*3;

    SkinCB.Modify();
    if(SkinMatMap)
      for(sInt i=0;i<SkinMatCount;i++)
        SkinCBPtr[i] = SkinMats[SkinMatMap[i]];
    else
      for(sInt i=0;i<SkinMatCount;i++)
        SkinCBPtr[i] = SkinMats[i];

    return &SkinCB;
  }
  else
  {
    sFatal(L"Wz4MtrlType_::MakeSkinCB() not implemented");
    return 0;
  }
}

void Wz4MtrlType_::SetDefaultShader(sInt flags,const sMatrix34CM *mat,sInt variant,sInt SkinMatCount,const sMatrix34CM *SkinMats,sInt *SkinMatMap)
{
  sCBufferBase *ptr[2] = {0,0};

  flags &= (sRF_TARGET_MASK|sRF_MATRIX_MASK);
  switch(flags)
  {
  case sRF_TARGET_WIRE|sRF_MATRIX_ONE:
    if(mat)
    {
      WireCBModel.Data->m[0] = mat->x;
      WireCBModel.Data->m[1] = mat->y;
      WireCBModel.Data->m[2] = mat->z;
      WireCBModel.Modify();
      Shaders[flags]->Set(&WireCB,&WireCBModel);
    }
    else
    {
      Shaders[flags]->Set(&WireCB,&IdentCBModel);
    }
    break;

  case sRF_TARGET_WIRE|sRF_MATRIX_INSTANCE:
  case sRF_TARGET_WIRE|sRF_MATRIX_BONEINST:
  case sRF_TARGET_WIRE|sRF_MATRIX_INSTPLUS:
    Shaders[flags]->Set(&WireCB);
    break;

  case sRF_TARGET_ZONLY|sRF_MATRIX_ONE:
  case sRF_TARGET_ZNORMAL|sRF_MATRIX_ONE:
  case sRF_TARGET_DIST|sRF_MATRIX_ONE:
  case sRF_TARGET_MAIN|sRF_MATRIX_ONE:
  case sRF_TARGET_ZONLY|sRF_MATRIX_BONE:
  case sRF_TARGET_ZNORMAL|sRF_MATRIX_BONE:
  case sRF_TARGET_DIST|sRF_MATRIX_BONE:
  case sRF_TARGET_MAIN|sRF_MATRIX_BONE:
    if(mat)
    {
      WireCBModel.Data->m[0] = mat->x;
      WireCBModel.Data->m[1] = mat->y;
      WireCBModel.Data->m[2] = mat->z;
      WireCBModel.Modify();
      ptr[0] = &ZOnlyCB;
      ptr[1] = &WireCBModel;
      Shaders[flags]->Set(ptr,2,variant);
    }
    else
    {
      ptr[0] = &ZOnlyCB;
      ptr[1] = &IdentCBModel;
      Shaders[flags]->Set(ptr,2,variant);
    }
    break;

  case sRF_TARGET_ZONLY|sRF_MATRIX_INSTANCE:
  case sRF_TARGET_ZNORMAL|sRF_MATRIX_INSTANCE:
  case sRF_TARGET_DIST|sRF_MATRIX_INSTANCE:
  case sRF_TARGET_MAIN|sRF_MATRIX_INSTANCE:

  case sRF_TARGET_ZONLY|sRF_MATRIX_BONEINST:
  case sRF_TARGET_ZNORMAL|sRF_MATRIX_BONEINST:
  case sRF_TARGET_DIST|sRF_MATRIX_BONEINST:
  case sRF_TARGET_MAIN|sRF_MATRIX_BONEINST:

  case sRF_TARGET_ZONLY|sRF_MATRIX_INSTPLUS:
  case sRF_TARGET_ZNORMAL|sRF_MATRIX_INSTPLUS:
  case sRF_TARGET_DIST|sRF_MATRIX_INSTPLUS:
  case sRF_TARGET_MAIN|sRF_MATRIX_INSTPLUS:

    ptr[0] = &ZOnlyCB;
    ptr[1] = &IdentCBModel;
    Shaders[flags]->Set(ptr,2,variant);
    break;

  default:
    sFatal(L"no default shader for renderflags %08x",flags);
  }
}

sVertexFormatHandle *Wz4MtrlType_::GetDefaultFormat(sInt flags)
{
  if(Formats[flags&sRF_TOTAL]==0)
    sFatal(L"no default shader for renderflags %08x",flags);

  return Formats[flags&sRF_TOTAL];
}

/****************************************************************************/

Wz4Mtrl::Wz4Mtrl()
{
  Type = Wz4MtrlType;
  ShellExtrude = 0;
}

/****************************************************************************/
/****************************************************************************/

void SimpleMtrlType_::Init()
{
  for(sInt i=0;i<sMAX_LIGHTENV;i++)
    LightEnv[i] = new LightEnv_;
  Wz4MtrlType->RegisterMtrl(this);
}

void SimpleMtrlType_::Exit()
{
  for(sInt i=0;i<sMAX_LIGHTENV;i++)
    delete LightEnv[i];
}

void SimpleMtrlType_::BeginShow(wPaintInfo &pi)
{
  for(sInt i=0;i<sMAX_LIGHTENV;i++)
  {
    LightEnv[i]->cbv.Data->mvp.Init();
    LightEnv[i]->cbv.Data->mv.Init();
    LightEnv[i]->cbv.Data->la.InitColor(0xffc0c0c0);
    LightEnv[i]->cbv.Data->lc[0].InitColor(0);
    LightEnv[i]->cbv.Data->lc[1].InitColor(0);
    LightEnv[i]->cbv.Data->lc[2].InitColor(0);
    LightEnv[i]->cbv.Data->lc[3].InitColor(0);
    LightEnv[i]->cbv.Data->ld[0].Init(0,0,0,0);
    LightEnv[i]->cbv.Data->ld[1].Init(0,0,0,0);
    LightEnv[i]->cbv.Data->ld[2].Init(0,0,0,0);
    LightEnv[i]->cbv.Data->EyePos.Init(0,0,0,0);
    LightEnv[i]->cbv.Modify();
    LightEnv[i]->cbp.Data->FogColor.InitColor(0x80ffffff);
    LightEnv[i]->cbp.Data->FogPara.Init(0,0,0,0);
    LightEnv[i]->cbp.Data->ClipPlane[0].Init(0,0,0,1);
    LightEnv[i]->cbp.Data->ClipPlane[1].Init(0,0,0,1);
    LightEnv[i]->cbp.Data->ClipPlane[2].Init(0,0,0,1);
    LightEnv[i]->cbp.Data->ClipPlane[3].Init(0,0,0,1);
    LightEnv[i]->cbp.Modify();
  }
}

void SimpleMtrlType_::PrepareViewR(sViewport &view)
{
  for(sInt i=0;i<sMAX_LIGHTENV;i++)
  {
    sVector30 d;
    d = -view.Camera.k;
    d.Unit();
    LightEnv[i]->cbv.Data->mvp = view.ModelScreen;
    LightEnv[i]->cbv.Data->mv = view.ModelView;
    LightEnv[i]->cbv.Data->la.InitColor(0xff404040);
    LightEnv[i]->cbv.Data->lc[0].InitColor(0xffc0c0c0);
    LightEnv[i]->cbv.Data->ld[0].Init(d.x,0,0,0);
    LightEnv[i]->cbv.Data->ld[1].Init(d.y,0,0,0);
    LightEnv[i]->cbv.Data->ld[2].Init(d.z,0,0,0);
    LightEnv[i]->cbv.Data->EyePos = view.Camera.l;
    LightEnv[i]->cbv.Modify();
  }
  ViewMatrix = view.View;
}


/****************************************************************************/

SimpleMtrl::SimpleMtrl()
{
  Type = SimpleMtrlType;
  sClear(Shaders);
  sClear(Formats);
  Tex[0] = 0;
  Tex[1] = 0;
  Tex[2] = 0;
  TFlags[0] = 0;
  TFlags[1] = 0;
  TFlags[2] = 0;
  AlphaTest = sMFF_ALWAYS;
  AlphaRef = 4;
  Extras = 0;
  DetailTexSpace = 0;
  VertexScale = 0.125f;
  
  cb.Data->texmat[0].Init(1,0,0,0);
  cb.Data->texmat[1].Init(0,1,0,0);
  cb.Data->texmat[2].Init(1,0,0,0);
  cb.Data->texmat[3].Init(0,1,0,0);
  cb.Data->texmat[4].Init(1,0,0,0);
  cb.Data->texmat[5].Init(0,1,0,0);
  cb.Data->texmat[6].Init(0,0,1,0);
  cb.Data->vextra.Init(0.125f,0,0,0);
}

SimpleMtrl::~SimpleMtrl()
{
  for(sInt i=0;i<sCOUNTOF(Shaders);i++)
    delete Shaders[i];
  Tex[0]->Release();
  Tex[1]->Release();
  Tex[2]->Release();
}

void SimpleMtrl::Set(sInt flags,sInt index,const sMatrix34CM *mat,sInt SkinMatCount,const sMatrix34CM *SkinMats,sInt *SkinMatMap)
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
      {
        det = (mat->x.x*mat->y.y*mat->z.z + mat->y.x*mat->z.y*mat->x.z + mat->z.x*mat->x.y*mat->y.z)    // flip culling
            - (mat->z.x*mat->y.y*mat->x.z + mat->x.x*mat->z.y*mat->y.z + mat->y.x*mat->x.y*mat->z.z);
      }
      switch(Flags & sMTRL_CULLMASK)
      {
        case sMTRL_CULLON: variant = det<0 ? 2 : 1; break;
        case sMTRL_CULLINV: variant = det<0 ? 1 : 2; break;
      }

      Wz4MtrlType->SetDefaultShader(flags,mat,variant,SkinMatCount,SkinMats,SkinMatMap);
    }
    break;
  case sRF_TARGET_MAIN:
    {
      sMatrix34 tmat,nmat;

      SimpleMtrlType_::LightEnv_ *env = SimpleMtrlType->LightEnv[index];
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

      // texture 0

      if(DetailTexSpace&0x100)
        tmat = env->TexTrans;
      else
        tmat = TexTrans[0];
      cb.Data->texmat[0].Init(tmat.i.x,tmat.j.x,tmat.k.x,tmat.l.x);
      cb.Data->texmat[1].Init(tmat.i.y,tmat.j.y,tmat.k.y,tmat.l.y);

      // texture 1

      if(DetailTexSpace&0x200)
        tmat = env->TexTrans;
      else
        tmat = TexTrans[1];
      switch(DetailTexSpace&7)
      {
      case 0:
        tmat = tmat;
        break;
      case 1:
        tmat = sMatrix34(*mat)*tmat;
        break;
      case 2:
        tmat = sMatrix34(*mat)*tmat*SimpleMtrlType->ViewMatrix;
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
      }

      cb.Data->texmat[2].Init(tmat.i.x,tmat.j.x,tmat.k.x,tmat.l.x);
      cb.Data->texmat[3].Init(tmat.i.y,tmat.j.y,tmat.k.y,tmat.l.y);

      // texture 2 (vertex texture)

      if(DetailTexSpace&0x400)
        tmat = env->TexTrans;
      else
        tmat = TexTrans[2];
      cb.Data->texmat[4].Init(tmat.i.x,tmat.j.x,tmat.k.x,tmat.l.x);
      cb.Data->texmat[5].Init(tmat.i.y,tmat.j.y,tmat.k.y,tmat.l.y);
      cb.Data->texmat[6].Init(tmat.i.z,tmat.j.z,tmat.k.z,tmat.l.z);
      cb.Data->vextra.Init(VertexScale,0,0,0);

      // figure variant

      sF32 det = 1;
      if(mat)
      {
        det = (mat->x.x*mat->y.y*mat->z.z + mat->y.x*mat->z.y*mat->x.z + mat->z.x*mat->x.y*mat->y.z)    // flip culling
            - (mat->z.x*mat->y.y*mat->x.z + mat->x.x*mat->z.y*mat->y.z + mat->y.x*mat->x.y*mat->z.z);
      }
      sInt variant = det>0 ? 0 : 1;

      // figure shader

      sInt shader = 0;
      switch(flags & sRF_MATRIX_MASK)
      {
        default:
        case sRF_MATRIX_ONE:      shader = 0; break;
        case sRF_MATRIX_BONE:     shader = 1; break;
        case sRF_MATRIX_INSTANCE: shader = 2; break;
        case sRF_MATRIX_BONEINST: shader = 3; break;
        case sRF_MATRIX_INSTPLUS: shader = 4; break;
      }

      // set again all texture pointers.
      // for rendertargets, since they might change the pyhsical
      // sTextureBase pointer insider the wObject wrapper

      if(Tex[0]) Shaders[shader]->Texture[0] = Tex[0]->Texture;
      if(Tex[1]) Shaders[shader]->Texture[1] = Tex[1]->Texture;
      if(Tex[2]) Shaders[shader]->Texture[2] = Tex[2]->Texture;

      // set

      cb.Modify();

      sCBufferBase *cbptr[5];
      cbptr[0] = &env->cbv;
      cbptr[1] = modelcb;
      cbptr[2] = &env->cbp;
      cbptr[3] = &cb;
      cbptr[4] = Wz4MtrlType->MakeSkinCB(SkinMatCount,SkinMats,SkinMatMap,3);

      Shaders[shader]->Set(cbptr,5,variant);
    }

       
    break;
  }
}

void SimpleMtrl::SetMtrl(sInt flags,sU32 blend,sInt extras)
{
  Flags = flags;
  Blend = blend;
  Extras = extras;
}

void SimpleMtrl::SetTex(sInt stage,Texture2D *tex,sInt tflags)
{
  tflags = sConvertOldUvFlags(tflags&0xffff);
  sRelease(Tex[stage]);
  Tex[stage] = tex; tex->AddRef();
  TFlags[stage] = tflags;
}

void SimpleMtrl::SetAlphaTest(sInt test,sInt ref)
{
  AlphaTest = test;
  AlphaRef = ref;
}
/*
void SimpleMtrl::SetTexMatrix(sInt stage,const sMatrix34 &mat)
{
  cb.Data->texmat[stage*2+0].Init(mat.i.x,mat.j.x,mat.k.x,mat.l.x);
  cb.Data->texmat[stage*2+1].Init(mat.i.y,mat.j.y,mat.k.y,mat.l.y);
  cb.Modify();
}
*/
void SimpleMtrl::Prepare()
{
  sU32 descdata[32];  
  sU32 *desc = descdata;

  *desc++ = sVF_POSITION|sVF_F3;
  if((Flags & sMTRL_LIGHTING) || WZ4ONLYONEGEO ||
     (Tex[1] && 
      (
       (Extras & SimpleShader::ExtraDetailMask)==SimpleShader::ExtraDetailNorm || 
       (Extras & SimpleShader::ExtraDetailMask)==SimpleShader::ExtraDetailRefl 
      ) 
     ) 
    )
  {
    if(Extras & SimpleShader::Extra_NormalI4)
      *desc++ = sVF_NORMAL|sVF_I4;
    else
      *desc++ = sVF_NORMAL|sVF_F3;
  }
  if(Tex[2] && !(Extras & 0x100))
    *desc++ = sVF_TANGENT|sVF_F3;
  if(Tex[0] || (Tex[2] && !(Extras & 0x100)))
    *desc++ = sVF_UV0|sVF_F2;
  *desc = 0;
  Formats[0] = sCreateVertexFormat(descdata);

  desc[0] = sVF_BONEINDEX|sVF_I4;
  desc[1] = sVF_BONEWEIGHT|sVF_I4;
  desc[2] = 0;
  Formats[1] = sCreateVertexFormat(descdata);

  desc[0] = sVF_STREAM1|sVF_UV5|sVF_F4|sVF_INSTANCEDATA;
  desc[1] = sVF_STREAM1|sVF_UV6|sVF_F4|sVF_INSTANCEDATA;
  desc[2] = sVF_STREAM1|sVF_UV7|sVF_F4|sVF_INSTANCEDATA;
  desc[3] = 0;
  Formats[2] = sCreateVertexFormat(descdata);

  desc[0] = sVF_BONEINDEX|sVF_I4;
  desc[1] = sVF_BONEWEIGHT|sVF_I4;
  desc[2] = sVF_STREAM1|sVF_UV5|sVF_F4|sVF_INSTANCEDATA;
  desc[3] = sVF_STREAM1|sVF_UV6|sVF_F4|sVF_INSTANCEDATA;
  desc[4] = sVF_STREAM1|sVF_UV7|sVF_F4|sVF_INSTANCEDATA;
  desc[5] = 0;
  Formats[3] = sCreateVertexFormat(descdata);

  desc[0] = sVF_STREAM1|sVF_UV5|sVF_F4|sVF_INSTANCEDATA;
  desc[1] = sVF_STREAM1|sVF_UV6|sVF_F4|sVF_INSTANCEDATA;
  desc[2] = sVF_STREAM1|sVF_UV7|sVF_F4|sVF_INSTANCEDATA;
  desc[3] = sVF_STREAM1|sVF_COLOR0|sVF_F4|sVF_INSTANCEDATA;
  desc[4] = 0;
  Formats[4] = sCreateVertexFormat(descdata);

  for(sInt i=0;i<sCOUNTOF(Shaders);i++)
  {
    Shaders[i] = new SimpleShader;
    Shaders[i]->Flags = Flags;
    Shaders[i]->BlendColor = Blend;
    Shaders[i]->TFlags[0] = TFlags[0];
    Shaders[i]->TFlags[1] = TFlags[1];
    Shaders[i]->TFlags[2] = TFlags[2];
#if sRENDERER==sRENDER_DX9
    Shaders[i]->TBind[2] = sMTB_VS|0;
#endif
    if(Tex[0]) Shaders[i]->Texture[0] = Tex[0]->Texture;
    if(Tex[1]) Shaders[i]->Texture[1] = Tex[1]->Texture;
    if(Tex[2]) Shaders[i]->Texture[2] = Tex[2]->Texture;
    if(AlphaTest!=7)
    {
      Shaders[i]->FuncFlags[sMFT_ALPHA] = AlphaTest;
      Shaders[i]->AlphaRef = AlphaRef;
    }
    Shaders[i]->Extra = Extras;

    Shaders[i]->InitVariants(2);
    Shaders[i]->SetVariant(0);
    sInt cull = Shaders[i]->Flags & sMTRL_CULLMASK;
    if(cull==sMTRL_CULLON)
      Shaders[i]->Flags = (Shaders[i]->Flags & ~sMTRL_CULLMASK) | sMTRL_CULLINV;
    if(cull==sMTRL_CULLINV)
      Shaders[i]->Flags = (Shaders[i]->Flags & ~sMTRL_CULLMASK) | sMTRL_CULLON;
    Shaders[i]->SetVariant(1);
    Shaders[i]->Prepare(Formats[i]);
  }
}

sVertexFormatHandle *SimpleMtrl::GetFormatHandle(sInt flags)
{
  switch(flags & sRF_TOTAL)
  {
    case sRF_TARGET_MAIN | sRF_MATRIX_ONE:      return Formats[0]; break;
    case sRF_TARGET_MAIN | sRF_MATRIX_BONE:     return Formats[1]; break;
    case sRF_TARGET_MAIN | sRF_MATRIX_INSTANCE: return Formats[2]; break;
    case sRF_TARGET_MAIN | sRF_MATRIX_BONEINST: return Formats[3]; break;
    case sRF_TARGET_MAIN | sRF_MATRIX_INSTPLUS: return Formats[4]; break;
    default: return Wz4MtrlType->GetDefaultFormat(flags);
  }
}

sBool SimpleMtrl::SkipPhase(sInt flags,sInt lightenv)
{
  if( ( (flags & sRF_TARGET_MASK)==sRF_TARGET_ZONLY || 
        (flags & sRF_TARGET_MASK)==sRF_TARGET_ZNORMAL || 
        (flags & sRF_TARGET_MASK)==sRF_TARGET_DIST
       ) && (Flags & sMTRL_NOZRENDER)) return 1;
  return 0;
}


template <class streamer> void SimpleMtrl::Serialize_(streamer &s)
{
  sInt version=s.Header(sSerId::Wz4SimpleMtrl,1);
  version;

  s | Name;

  s | Flags | Blend | AlphaTest | AlphaRef | Extras;

  sInt tc=sCOUNTOF(Tex);
  s | tc;
  sVERIFY(tc<=sCOUNTOF(Tex));
  for (sInt i=0; i<tc; i++)
  {
    s.OnceRef(Tex[i]);
    s | TFlags[i];
    s | TexTrans[i];
  }

  s.Footer();
}

/****************************************************************************/
/****************************************************************************/


RNSimpleMtrlEnv::RNSimpleMtrlEnv()
{
  Anim.Init(Wz4RenderType->Script);
}

void RNSimpleMtrlEnv::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  SimulateChilds(ctx);
}

void RNSimpleMtrlEnv::Render(Wz4RenderContext *ctx)
{
  SimpleShaderVEnv *cbv = SimpleMtrlType->LightEnv[Para.Index]->cbv.Data;
  SimpleShaderPEnv *cbp = SimpleMtrlType->LightEnv[Para.Index]->cbp.Data;

  sVector30 d0,d1,d2,d3;
  d0 = Para.Dir0;
  d1 = Para.Dir1;
  d2 = Para.Dir2;
  d3 = Para.Dir3;
  if(Para.Headlights&0x0001) d0 = d0*ctx->View.Camera;
  if(Para.Headlights&0x0010) d1 = d1*ctx->View.Camera;
  if(Para.Headlights&0x0100) d2 = d2*ctx->View.Camera;
  if(Para.Headlights&0x1000) d3 = d3*ctx->View.Camera;
  d0.Unit();
  d1.Unit();
  d2.Unit();
  d3.Unit();
  cbv->la.InitColor(Para.Ambient);
  cbv->lc[0].InitColor(Para.Color0); cbv->lc[0] = cbv->lc[0] * Para.Color0Amp;
  cbv->lc[1].InitColor(Para.Color1); cbv->lc[1] = cbv->lc[1] * Para.Color1Amp;
  cbv->lc[2].InitColor(Para.Color2); cbv->lc[2] = cbv->lc[2] * Para.Color2Amp;
  cbv->lc[3].InitColor(Para.Color3); cbv->lc[3] = cbv->lc[3] * Para.Color3Amp;
  cbv->ld[0].Init(d0.x,d1.x,d2.x,d3.x);
  cbv->ld[1].Init(d0.y,d1.y,d2.y,d3.y);
  cbv->ld[2].Init(d0.z,d1.z,d2.z,d3.z);

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
        if(e<1.0e-20)
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
        clip[i] = clip[i]*mat;
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

  SimpleMtrlType->LightEnv[Para.Index]->cbv.Modify();
  SimpleMtrlType->LightEnv[Para.Index]->cbp.Modify();

  sSRT srt;
  srt.Scale = Para.TexScale;
  srt.Rotate = Para.TexRot;
  srt.Translate = Para.TexTrans;
  srt.MakeMatrix(SimpleMtrlType->LightEnv[Para.Index]->TexTrans);
}

/****************************************************************************/
/****************************************************************************/

void CustomMtrlType_::Init()
{
  Wz4MtrlType->RegisterMtrl(this);
  Viewport=0;
}

void CustomMtrlType_::Exit()
{
}

void CustomMtrlType_::BeginShow(wPaintInfo &pi)
{
  /*
  cbv.Data->mvp.Init();
  cbv.Data->mv.Init();
  cbv.Data->EyePos.Init(0,0,0,0);
  cbv.Modify();
  cbp.Data->mvp.Init();
  cbp.Data->mv.Init();
  cbp.Data->EyePos.Init(0,0,0,0);
  cbp.Modify();
  */
}

void CustomMtrlType_::PrepareViewR(sViewport &view)
{
  Viewport = &view;
}


/****************************************************************************/

class CustomShader : public sMaterial
{
public:
  sShader *VS;
  sShader *PS;

  CustomShader() : VS(0), PS(0) {}

  ~CustomShader()
  {
    sRelease(VS);
    sRelease(PS);
  }

  void SelectShaders(sVertexFormatHandle *format)
  {
    PixelShader = PS;
    VertexShader = VS;
    sAddRef(PS);
    sAddRef(VS);
  }
};

/****************************************************************************/

CustomMtrl::CustomMtrl()
{
  Type = CustomMtrlType;
  Shader=0;
  Format=0;
  sClear(Tex);
  sClear(TFlags);
  AlphaTest = sMFF_ALWAYS;
  AlphaRef = 4;
}

CustomMtrl::~CustomMtrl()
{
  delete Shader;
  for (sInt i=0; i<sMTRL_MAXTEX; i++)
    Tex[i]->Release();
}

void CustomMtrl::Set(sInt flags,sInt index,const sMatrix34CM *mat,sInt SkinMatCount,const sMatrix34CM *SkinMats,sInt *SkinMatMap)
{
  switch(flags & sRF_TARGET_MASK)
  {
  case sRF_TARGET_MAIN:
    {
      sMatrix34 tmat,nmat;

      sVERIFY(CustomMtrlType->Viewport);
      const sViewport &view=*CustomMtrlType->Viewport;

      sVector30 d;
      d = -view.Camera.k;
      d.Unit();
      cbv.Data->mvp = view.ModelScreen;
      cbv.Data->mvp.Trans4();
      cbv.Data->mv = view.ModelView;
      cbv.Data->mv.Trans4();
      cbv.Data->EyePos = view.Camera.l;
      cbv.Data->vs_var0.Init(vs_var0[0],vs_var0[1],vs_var0[2],vs_var0[3]);
      cbv.Data->vs_var1.Init(vs_var1[0],vs_var1[1],vs_var1[2],vs_var1[3]);
      cbv.Data->vs_var2.Init(vs_var2[0],vs_var2[1],vs_var2[2],vs_var2[3]);
      cbv.Data->vs_var3.Init(vs_var3[0],vs_var3[1],vs_var3[2],vs_var3[3]);
      cbv.Data->vs_var4.Init(vs_var4[0],vs_var4[1],vs_var4[2],vs_var4[3]);
      cbv.Modify();

      cbp.Data->mvp = view.ModelScreen;
      cbp.Data->mvp.Trans4();
      cbp.Data->mv = view.ModelView;
      cbp.Data->mv.Trans4();
      cbp.Data->EyePos = view.Camera.l;
      cbp.Data->ps_var0.Init(ps_var0[0],ps_var0[1],ps_var0[2],ps_var0[3]);
      cbp.Data->ps_var1.Init(ps_var1[0],ps_var1[1],ps_var1[2],ps_var1[3]);
      cbp.Data->ps_var2.Init(ps_var2[0],ps_var2[1],ps_var2[2],ps_var2[3]);
      cbp.Data->ps_var3.Init(ps_var3[0],ps_var3[1],ps_var3[2],ps_var3[3]);
      cbp.Data->ps_var4.Init(ps_var4[0],ps_var4[1],ps_var4[2],ps_var4[3]);
      cbp.Modify();

      for (sInt i=0; i<sMTRL_MAXTEX; i++)
      {
        if (Tex[i])
        {
          Shader->Texture[i]=Tex[i]->Texture;
          Shader->TFlags[i]=TFlags[i];
        }
        else
        {
          Shader->Texture[i]=0;
          Shader->TFlags[i]=0;
        }
      }

      Shader->Set(&cbv,&cbp);
    }
    break;
  }
}

void CustomMtrl::SetMtrl(sInt flags,sU32 blend)
{
  Flags = flags;
  Blend = blend;
}

void CustomMtrl::SetTex(sInt stage,Texture2D *tex,sInt tflags)
{
  tflags = sConvertOldUvFlags(tflags&0xffff);
  sRelease(Tex[stage]);
  Tex[stage] = tex; tex->AddRef();
  TFlags[stage] = tflags;
}

void CustomMtrl::SetAlphaTest(sInt test,sInt ref)
{
  AlphaTest = test;
  AlphaRef = ref;
}
/*
void CustomMtrl::SetTexMatrix(sInt stage,const sMatrix34 &mat)
{
  cb.Data->texmat[stage*2+0].Init(mat.i.x,mat.j.x,mat.k.x,mat.l.x);
  cb.Data->texmat[stage*2+1].Init(mat.i.y,mat.j.y,mat.k.y,mat.l.y);
  cb.Modify();
}
*/

void CustomMtrl::SetShader(sShader *vs, sShader *ps)
{
  sDelete(Shader);
  Shader = new CustomShader;
  Shader->VS=vs;
  Shader->PS=ps;
}

void CustomMtrl::Prepare()
{
  sU32 descdata[32];  
  sU32 *desc = descdata;

  *desc++ = sVF_POSITION|sVF_F3;
  *desc++ = sVF_NORMAL|sVF_F3;
  *desc++ = sVF_UV0|sVF_F2;
  *desc = 0;
  Format = sCreateVertexFormat(descdata);

  sVERIFY(Shader);
  Shader->Flags = Flags;
  Shader->BlendColor = Blend;

  for(sInt i=0; i<sMTRL_MAXTEX; i++)
  {
    Shader->TFlags[i] = TFlags[i];
    if(Tex[i]) Shader->Texture[i] = Tex[i]->Texture;
  }

#if sRENDERER==sRENDER_DX9
  Shader->TBind[15] = sMTB_VS|0;
#endif

  if(AlphaTest!=7)
  {
    Shader->FuncFlags[sMFT_ALPHA] = AlphaTest;
    Shader->AlphaRef = AlphaRef;
  }

  Shader->Prepare(Format);
}

sVertexFormatHandle *CustomMtrl::GetFormatHandle(sInt flags)
{
  switch(flags & sRF_TOTAL)
  {
    case sRF_TARGET_MAIN | sRF_MATRIX_ONE: return Format; break;
    default: return Wz4MtrlType->GetDefaultFormat(flags);
  }
}

sBool CustomMtrl::SkipPhase(sInt flags,sInt lightenv)
{
  if( (flags & sRF_TARGET_MASK)==sRF_TARGET_ZONLY || 
      (flags & sRF_TARGET_MASK)==sRF_TARGET_ZNORMAL || 
      (flags & sRF_TARGET_MASK)==sRF_TARGET_DIST
    ) return 1;
  return 0;
}

sShader *CustomMtrl::CompileShader(sInt shadertype, const sChar *source)
{
  sString<16> profile;
  switch (shadertype)
  {
  case sSTF_PIXEL|sSTF_HLSL23: profile=L"ps_3_0"; break;
  case sSTF_VERTEX|sSTF_HLSL23: profile=L"vs_3_0"; break;
  default: Log.PrintF(L"unknown shader type %x\n",shadertype); return 0;
  }
  
  sTextBuffer src2;
  sTextBuffer error;
  sU8 *data = 0;
  sInt size = 0;
  sShader *sh = 0;

  src2.Print(L"uniform float4x4 mvp : register(c0);\n");
  src2.Print(L"uniform float4x4 mv  : register(c4);\n");
  src2.Print(L"uniform float3   eye : register(c8);\n");
  src2.Print(L"\n");

  src2.Print(source);

  if(sShaderCompileDX(src2.Get(),profile,L"main",data,size,sSCF_AVOID_CFLOW|sSCF_DONT_OPTIMIZE,&error))
  {
    Log.PrintF(L"dx: %q\n",error.Get());
    Log.PrintListing(src2.Get());
    Log.Print(L"/****************************************************************************/\n");
#if sRENDERER==sRENDER_DX9
    sPrintShader(Log,(const sU32 *) data,sPSF_NOCOMMENTS);
#endif
    sh = sCreateShaderRaw(shadertype,data,size);
  }
  else
  {
    Log.PrintF(L"dx: %q\n",error.Get());
    Log.PrintListing(src2.Get());
    Log.Print(L"/****************************************************************************/\n");
    sDPrint(Log.Get());
  }
  delete[] data;

  return sh;
}

/****************************************************************************/
/****************************************************************************/

