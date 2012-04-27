/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "wz4frlib/wz4_demo2.hpp"
#include "wz4frlib/wz4_demo2_ops.hpp"
#include "util/ipp.hpp"
#include "wz4frlib/wz4_ipp.hpp"

#include "wz4lib/gui.hpp"
#include "base/devices.hpp"

/****************************************************************************/

void Wz4RenderType_::Init()
{
  Script = new ScriptContext;
  _Time = Script->AddSymbol(L"time");
  _BaseTime = Script->AddSymbol(L"basetime");
  _LocalTime = Script->AddSymbol(L"localtime");
  _LowQuality = Script->AddSymbol(L"lowquality");
//  _Fader = Script->AddSymbol(L"Fader");
//  _Rotor = Script->AddSymbol(L"Rotor");
  IppHelper = new IppHelper2;

  sClear(MidiHigh);
  sClear(MidiFloat);

  if(!sSchedMon) sSchedMon = new sStsPerfMon;
  sSchedMon->Scale = 16+2;
}

void Wz4RenderType_::Exit()
{
  delete Script;
  delete IppHelper;
}

// *** WARNING *** some of this code is duplicated in GenBitmap Render(Wz4Render)


void Wz4RenderType_::BeginShow(wPaintInfo &pi)
{
  sSchedMon->FlipFrame();
}

void Wz4RenderType_::Show(wObject *obj,wPaintInfo &pi)
{
  sVERIFY(obj->Type == Wz4RenderType);
  Wz4Render *ro = (Wz4Render *) obj;

  // prepare environment

  Wz4RenderContext ctx;
  sTargetSpec spec = pi.Spec;
  ctx.Init(Script,&spec,&pi);
  ctx.IppHelper = IppHelper;
  sRTMan->ResolutionCheck(this,ctx.ScreenX,ctx.ScreenY);
  ctx.RenderFlags |= pi.Wireframe ? wRF_RenderWire : wRF_RenderMain;
  if(pi.CamOverride)
    ctx.RenderFlags |= wRF_FreeCam;
  ctx.View = *pi.View;
  
  // check midi

  Midi();

  // reset Recursion flags

  ctx.ClearRecFlags(ro->RootNode);
//  ro->RootNode->ClearRecFlags();

  // script execution

  Script->BeginExe();

  ScriptValue *SV_Time = Script->MakeFloat(1);
  ScriptValue *SV_BaseTime = Script->MakeFloat(1);
  ScriptValue *SV_LocalTime = Script->MakeFloat(1);
  ScriptValue *SV_LowQuality = Script->MakeInt(1);
  ScriptValue *SV_Rotor = Script->MakeFloat(8);
  ScriptValue *SV_Fader = Script->MakeFloat(8);

  SV_BaseTime->FloatPtr[0] = pi.TimeBeat/65536.0f;
  SV_Time->FloatPtr[0] = sFMod(pi.TimeBeat/65536.0f/16,1);
  SV_LocalTime->FloatPtr[0] = pi.TimeBeat/65536.0f;
  SV_LowQuality->IntPtr[0] = Doc->LowQuality && (Doc->DocOptions.DialogFlags & wDODF_LowQuality);
  for(sInt i=0;i<8;i++)
  {
    SV_Rotor->FloatPtr[i] = MidiFloat[i+8];
    SV_Fader->FloatPtr[i] = MidiFloat[i];
  }

  Script->BindGlobal(_Time,SV_Time);
  Script->BindGlobal(_BaseTime,SV_BaseTime);
  Script->BindGlobal(_LocalTime,SV_LocalTime);
  Script->BindGlobal(_LowQuality,SV_LowQuality);
//  Script->BindGlobal(_Rotor,SV_Rotor);
//  Script->BindGlobal(_Fader,SV_Fader);

  ctx.SetCam = 0;
  ro->RootNode->Simulate(&ctx);
  Script->EndExe();

  // writeback camera to pi?

  if(ctx.SetCam)
  {
    pi.SetCam = 1;
    pi.SetCamMatrix = ctx.SetCamMatrix;
    pi.SetCamZoom = ctx.SetCamZoom;
  }

  // no camera set? default camera!

  if(ctx.CameraFlag==0)
  {
    ctx.View = *pi.View;

    ctx.RenderControl(ro->RootNode,sCLEAR_ALL,pi.BackColor,spec);
  }
  else
  {
    pi.Grid = 0;
  }
  pi.PaintHandles();


  if(pi.MTMFlag)
  {
    sSetTarget(sTargetPara(0,0,pi.Spec));
    sSchedMon->Paint(spec);
  }
}

void Wz4RenderType_::Midi()
{
  sMidiEvent e;
  if(sMidiHandler)
  {
    while(sMidiHandler->GetInput(e))
    {
      if(e.Status==0xb0)
      {
        if(e.Value1>=0x10 && e.Value1<0x20)
        {
          sInt n = e.Value1-0x10;
          MidiHigh[n] = e.Value2;
        }
        else if(e.Value1>=0x30 && e.Value1<0x40)
        {
          sInt n = e.Value1-0x30;
          MidiFloat[n] = (MidiHigh[n]*128+e.Value2)/999.0f;
        }
      }
    }
  }
}

/****************************************************************************/

Wz4Render::Wz4Render()
{
  Type = Wz4RenderType;
  RootNode = 0;
}

Wz4Render::~Wz4Render()
{
  RootNode->Release();
}

void Wz4Render::AddChilds(wCommand *cmd,sInt rp,sInt first,sInt last)
{
  sVERIFY(RootNode->Op == 0);
  sVERIFY(RootNode->Code == 0);

  AddCode(cmd,rp);

  RootNode->Childs.HintSize(cmd->InputCount);
  sInt sort = 0;
  if(last == -1)
    last = cmd->InputCount;
  for(sInt i=first;i<last;i++)
  {
    Wz4Render *in = cmd->GetInput<Wz4Render *>(i);
    if(in)
    {
      if(in->RootNode->MayCollapse)
      {
        Wz4RenderNode *c;
        sFORALL(in->RootNode->Childs,c)
        {
          RootNode->Childs.AddTail(c);
          c->AddRef();
          c->RenderpassSort = c->Renderpass * 256 + sMin(sort++,255);
        }
      }
      else
      {
        RootNode->Childs.AddTail(in->RootNode);
        in->RootNode->AddRef();
        in->RootNode->RenderpassSort = in->RootNode->Renderpass * 256 + sMin(sort++,255);
      }
    }
  }

  sSortUp(RootNode->Childs,&Wz4RenderNode::RenderpassSort);

  if(rp==0 && RootNode->Childs.GetCount()==1)     // inheret renderpass.... hacky but useful
    RootNode->Renderpass = RootNode->Childs[0]->Renderpass;
}

void Wz4Render::AddCode(wCommand *cmd,sInt rp)
{
  RootNode->Renderpass = rp;
  if(cmd->StringCount>0 && RootNode->Code==0)
  {
    if(cmd->Strings[0] && cmd->Strings[0]!=0)
    {
      RootNode->Code = new ScriptCode(cmd->Strings[0]);
    }
  }
  RootNode->Op = cmd->Op;
}

/****************************************************************************/

Wz4Particles::Wz4Particles() 
{
  Type = Wz4ParticlesType;
  RootNode = 0; 
}

Wz4Particles::~Wz4Particles() 
{
  RootNode->Release();
}

void Wz4Particles::AddCode(wCommand *cmd)
{
  if(cmd->StringCount>0 && RootNode->Code==0)
  {
    if(cmd->Strings[0] && cmd->Strings[0]!=0)
    {
      RootNode->Code = new ScriptCode(cmd->Strings[0]);
    }
  }
  RootNode->Op = cmd->Op;
}

/****************************************************************************/

Wz4PartInfo::Wz4PartInfo()
{
  Alloc = 0;
  Used = 0;
  Compact = 0;
  Flags = 0;

  Parts = 0;
  Quats = 0;
  Colors = 0;
}

Wz4PartInfo::~Wz4PartInfo()
{
  delete[] Parts;
  delete[] Quats;
  delete[] Colors;
}

void Wz4PartInfo::Init(sInt flags,sInt count)
{
  Flags = flags;
  Alloc = count;

  Parts = new Wz4Particle[count];

  if(Flags & wPNF_Orientation)
    Quats = new sQuaternion[count];
  if(Flags & wPNF_Color)
    Colors = new sU32[count];
}

void Wz4PartInfo::Reset()
{
  Used = 0;
  Compact = 0;
  Flags = 0;
}

void Wz4PartInfo::Save(SaveInfo &s)
{
  s.Alloc = Alloc;
  s.Parts = Parts;
  s.Quats = Quats;
  s.Colors = Colors;
}

void Wz4PartInfo::Load(SaveInfo &s)
{
  Alloc = s.Alloc;
  Parts = s.Parts;
  Quats = s.Quats;
  Colors = s.Colors;
}

void Wz4PartInfo::Inc(sInt i)
{
  Parts += i;
  if(Quats)
    Quats += i;
  if(Colors)
    Colors += i;
}

/****************************************************************************/

Wz4ParticleNode::Wz4ParticleNode() 
{
  Code = 0;
  Op = 0;
}

Wz4ParticleNode::~Wz4ParticleNode()
{
  delete Code;
}

ScriptSymbol *Wz4ParticleNode::AddSymbol(const sChar *name)
{
  return Wz4RenderType->Script->AddSymbol(name);
}


void Wz4ParticleNode::SimulateCalc(Wz4RenderContext *ctx)
{
  if(Code)
  {
    const sChar *err = 0;
    if(Code->Execute(ctx->Script)==0)
      err = sPoolString(Code->ErrorMsg);

    if(Doc->IsOp(Op) && Op->CalcErrorString!=err)
    {
      Op->CalcErrorString = err;
      App->Update();
    }
  }
}

/****************************************************************************/

void Wz4ParticlesType_::Show(wObject *obj,wPaintInfo &pi)
{
  sVERIFY(obj->Type == Wz4ParticlesType);
  Wz4Particles *p = (Wz4Particles *) obj;

  sF32 time=sFMod(pi.TimeBeat/65536.0f/16,1);

  sSetTarget(sTargetPara(sCLEAR_ALL,pi.BackColor,pi.Spec));

  ScriptContext script;
  ScriptSymbol *_time = script.AddSymbol(L"time");
  ScriptSymbol *_basetime = script.AddSymbol(L"basetime");
  ScriptSymbol *_localtime = script.AddSymbol(L"localtime");

  Wz4RenderContext ctx;
  sTargetSpec spec = pi.Spec;
  ctx.Init(&script,&spec,&pi);
  ctx.IppHelper = 0;
  sRTMan->ResolutionCheck(this,ctx.ScreenX,ctx.ScreenY);
  ctx.RenderFlags |= pi.Wireframe ? wRF_RenderWire : wRF_RenderMain;
  if(pi.CamOverride)
    ctx.RenderFlags |= wRF_FreeCam;
  ctx.View = *pi.View;

  script.BeginExe();

  ScriptValue *SV_Time = script.MakeFloat(1);
  ScriptValue *SV_BaseTime = script.MakeFloat(1);
  ScriptValue *SV_LocalTime = script.MakeFloat(1);

  SV_BaseTime->FloatPtr[0] = pi.TimeBeat/65536.0f;
  SV_Time->FloatPtr[0] = time;
  SV_LocalTime->FloatPtr[0] = pi.TimeBeat/65536.0f;
  script.BindGlobal(_time,SV_Time);
  script.BindGlobal(_basetime,SV_BaseTime);
  script.BindGlobal(_localtime,SV_LocalTime);

  ctx.SetCam = 0;
  p->RootNode->Simulate(&ctx);
  script.EndExe();

  Wz4PartInfo pinfo;
  pinfo.Init(p->RootNode->GetPartFlags(),p->RootNode->GetPartCount());
  p->RootNode->Func(pinfo,time,0);

  if (pinfo.Used)
  {
    sF32 size=0.05f;
    sVector30 d1=size*(-pi.View->Camera.i-pi.View->Camera.j);
    sVector30 d2=size*( pi.View->Camera.i+pi.View->Camera.j);
    sVector30 d3=size*( pi.View->Camera.i-pi.View->Camera.j);
    sVector30 d4=size*(-pi.View->Camera.i+pi.View->Camera.j);

    sSimpleMaterial mtrl;
    mtrl.Flags = sMTRL_CULLOFF|sMTRL_ZOFF;
    mtrl.Prepare(sVertexFormatBasic);
    sCBuffer<sSimpleMaterialPara> cb;
    cb.Data->Set(*pi.View);
    mtrl.Set(&cb);

    sVertexBasic *vp;
    sGeometry geo(sGF_LINELIST|sGF_INDEXOFF,sVertexFormatBasic);
    geo.BeginLoadVB(4*pinfo.Used,sGD_STREAM,&vp);
    for (sInt i=0; i<pinfo.Used; i++)
    {
      const sVector31 &pos = pinfo.Parts[i].Pos;
      sU32 color=0xffffffff;
      if (pinfo.Colors) color=pinfo.Colors[i];
      vp++->Init(pos+d1,color);
      vp++->Init(pos+d2,color);
      vp++->Init(pos+d3,color);
      vp++->Init(pos+d4,color);
    }
    geo.EndLoadVB();
    geo.Draw();
  }
  
}


/****************************************************************************/

Wz4RenderContext::Wz4RenderContext()
{
  sTargetSpec spec;
  Init(0,&spec,0);
}

Wz4RenderContext::~Wz4RenderContext()
{
}

void Wz4RenderContext::Init(ScriptContext *script,const sTargetSpec *spec,wPaintInfo *pi)
{
  Script = script;
  _Time = 0;
  RenderFlags = 0;
  CameraFlag = 0;
  IppHelper = 0;
  RenderMode = 0;
  DisableShadowFlag = 0;
  if(Script)
  {
    _Time = Script->AddSymbol(L"time");
    _BaseTime = Script->AddSymbol(L"basetime");
    _LocalTime = Script->AddSymbol(L"localtime");
  }
  ZTarget = 0;
  ZTargetLowRes = 0;
  RTSpec = *spec;
  ScreenX = RTSpec.Window.SizeX();
  ScreenY = RTSpec.Window.SizeY();
  ShakeCamMatrix.Init();
  PaintInfo = pi;
}

void Wz4RenderContext::PrepareView(sViewport &view)
{
//  View.SetTargetCurrent();
//  View.SetZoom(Zoom);
//  View.Prepare();

  Frustum.Init(view.ModelScreen);
  Wz4MtrlType->PrepareView(view);
}

sF32 Wz4RenderContext::GetTime()
{
  if(_Time->Value && _Time->Value->Type==2 && _Time->Value->Count==1)
    return _Time->Value->FloatPtr[0];
  else
    return 0;
}

sF32 Wz4RenderContext::GetBaseTime()
{
  if(_BaseTime->Value && _BaseTime->Value->Type==2 && _BaseTime->Value->Count==1)
    return _BaseTime->Value->FloatPtr[0];
  else
    return 0;
}

sF32 Wz4RenderContext::GetLocalTime()
{
  if(_LocalTime->Value && _LocalTime->Value->Type==2 && _LocalTime->Value->Count==1)
    return _LocalTime->Value->FloatPtr[0];
  else
    return 0;
}

void Wz4RenderContext::SetTime(sF32 time)
{
  if(_Time->Value && _Time->Value->Type==2 && _Time->Value->Count==1)
    _Time->Value->FloatPtr[0] = time;
}

void Wz4RenderContext::SetLocalTime(sF32 time)
{
  if(_LocalTime->Value && _LocalTime->Value->Type==2 && _LocalTime->Value->Count==1)
    _LocalTime->Value->FloatPtr[0] = time;
}

void Wz4RenderContext::RenderControl(Wz4RenderNode *root,sInt clrflg,sU32 clrcol,const sTargetSpec &spec)
{
  sVERIFY(RenderMode == 0);

  sMatrix34 mat;
  Root = root;
  root->ClearMatricesR();
  root->Transform(this,mat);
  Frustum.Init(View.ModelScreen);

  Wz4MtrlType->PrepareRender(this);

  root->Prepare(this);
  NextRecMask();
  sRTMan->SetScreen(spec);
  Wz4MtrlType->BeginRender(this);
  if(RenderFlags & wRF_RenderWire)
  {
    sSetTarget(sTargetPara(sST_CLEARALL|sST_SCISSOR,Doc->EditOptions.BackColor,spec));

    PrepareView(View);
    RenderMode = sRF_TARGET_WIRE;
    root->Render(this);
    NextRecMask();
  }
  if(RenderFlags & wRF_RenderZ)
  {
    // determine size for larger zbuffer
    sInt zbX,zbY;
    switch(Doc->EditOptions.ExpensiveIPPQuality)
    {
    case 0: // low
      zbX = ScreenX / 4;
      zbY = ScreenY / 4;
      break;

    case 1: // medium
      zbX = ScreenX / 2;
      zbY = ScreenY / 2;
      break;

    case 2: // high
      zbX = ScreenX;
      zbY = ScreenY;
      break;
    }

    // render
    sEnlargeZBufferRT(zbX,zbY);
    ZTarget = sRTMan->Acquire(zbX,zbY,sTEX_2D|sTEX_NOMIPMAPS|sTEX_ARGB16F);
    sTargetPara para(sST_CLEARALL,0xffffffff,0,ZTarget,sGetRTDepthBuffer());
    sSetTarget(para);
    PrepareView(View);
    RenderMode = sRF_TARGET_ZNORMAL;
    root->Render(this);
    NextRecMask();

    // low-res rendertarget
    if(RenderFlags & wRF_RenderZLowres)
    {
      if(Doc->EditOptions.ExpensiveIPPQuality > 0)
      {
        ZTargetLowRes = sRTMan->Acquire(ScreenX/4,ScreenY/4,sTEX_2D|sTEX_NOMIPMAPS|sTEX_ARGB16F);
        IppHelper->Copy(ZTargetLowRes,ZTarget);
      }
      else // just use the rendertarget directly
      {
        sRTMan->AddRef(ZTarget);
        ZTargetLowRes = ZTarget;
      }
    }
  }
  if(RenderFlags & wRF_RenderMain)
  {
    sSetTarget(sTargetPara(clrflg|sST_SCISSOR,clrcol,spec));
    PrepareView(View);
    RenderMode =sRF_TARGET_MAIN;
    root->Render(this);
    NextRecMask();
  }
  if(ZTarget)
  {
    sRTMan->Release(ZTarget);
    ZTarget = 0;
  }
  if(ZTargetLowRes)
  {
    sRTMan->Release(ZTargetLowRes);
    ZTargetLowRes = 0;
  }

  Wz4MtrlType->EndRender(this);
  sRTMan->FinishScreen();
  RenderMode = 0;
  Root = 0;
}

void Wz4RenderContext::RenderControlZ(Wz4RenderNode *root,const sTargetSpec &spec)
{
  sVERIFY(RenderMode == 0);

  sMatrix34 mat;
  Root = root;
  root->ClearMatricesR();
  root->Transform(this,mat);
  Frustum.Init(View.ModelScreen);

  Wz4MtrlType->PrepareRender(this);

  root->Prepare(this);
  NextRecMask();
  sRTMan->SetScreen(spec);
  Wz4MtrlType->BeginRender(this);

  if(spec.Depth==0)
    sEnlargeZBufferRT(ScreenX,ScreenY);
  sTargetPara para(sST_CLEARALL,0xffffffff,0,spec.Color,spec.Depth);
  sSetTarget(para);
  PrepareView(View);
  RenderMode = sRF_TARGET_ZONLY;
  root->Render(this);
  NextRecMask();

  Wz4MtrlType->EndRender(this);
  sRTMan->FinishScreen();
  RenderMode = 0;
  Root = 0;
}

void Wz4RenderContext::ClearRecFlags(Wz4RenderNode *root)
{
  root->ClearRecFlagsR();
  RecMask = 2;
}

void Wz4RenderContext::NextRecMask()
{
  RecMask *= 2;
  sVERIFY(RecMask)
}

sBool Wz4RenderContext::IsCommonRendermode(sBool zwrite)
{
  return (((RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN 
       || ((RenderMode & sRF_TARGET_MASK)==sRF_TARGET_ZONLY && zwrite)
       || ((RenderMode & sRF_TARGET_MASK)==sRF_TARGET_ZNORMAL && zwrite)
       || ((RenderMode & sRF_TARGET_MASK)==sRF_TARGET_DIST && zwrite)));
}

void Wz4RenderContext::DoScript(Wz4RenderNode *node,sF32 time,sF32 basetime)
{
  ClearRecFlags(node);
  Wz4RenderType->Script->BeginExe();
  ScriptValue *SV_Time = Wz4RenderType->Script->MakeFloat(1);
  ScriptValue *SV_BaseTime = Wz4RenderType->Script->MakeFloat(1);
  ScriptValue *SV_LocalTime = Wz4RenderType->Script->MakeFloat(1);
  SV_Time->FloatPtr[0] = time;
  SV_BaseTime->FloatPtr[0] = basetime;
  SV_LocalTime->FloatPtr[0] = basetime;
  Wz4RenderType->Script->BindGlobal(Wz4RenderType->_Time,SV_Time);
  Wz4RenderType->Script->BindGlobal(Wz4RenderType->_BaseTime,SV_BaseTime);
  Wz4RenderType->Script->BindGlobal(Wz4RenderType->_LocalTime,SV_LocalTime);
  SetCam = 0;
  node->Simulate(this);
  Wz4RenderType->Script->EndExe();
}

/****************************************************************************/

Wz4BaseNode::Wz4BaseNode()
{
  RefCount = 1;
}

Wz4BaseNode::~Wz4BaseNode()
{
}

Wz4RenderNode::Wz4RenderNode()
{
  Code = 0;
  Op = 0;
  Renderpass = 0;
  MayCollapse = 0;
  TimelineStart = 0;
  TimelineEnd = 0;
  ClipRandDuration = 0;
}

Wz4RenderNode::~Wz4RenderNode()
{
  delete Code;
  sReleaseAll(Childs);
 // if(Op) sRemRoot(Op);
}

ScriptSymbol *Wz4RenderNode::AddSymbol(const sChar *name)
{
  return Wz4RenderType->Script->AddSymbol(name);
}

void Wz4RenderNode::ClearRecFlagsR()
{
  Wz4RenderNode *c;
  RecFlags = 0;
  sFORALL(Childs,c)
    c->ClearRecFlagsR();
}

void Wz4RenderNode::ClearMatricesR()
{
  Wz4RenderNode *c;
  Matrices.Clear();
  sFORALL(Childs,c)
    c->ClearMatricesR();
}


void Wz4RenderNode::SimulateCalc(Wz4RenderContext *ctx)
{
  if(Code)
  {
    const sChar *err = 0;
    if(Code->Execute(ctx->Script)==0)
      err = sPoolString(Code->ErrorMsg);
    ctx->Script->FlushLocal();

    if(Doc->IsOp(Op) && Op->CalcErrorString!=err)
    {
      Op->CalcErrorString = err;
      App->Update();
    }
  }
}


void Wz4RenderNode::SimulateChild(Wz4RenderContext *ctx,Wz4RenderNode *c)
{
  RecFlags |= 1;
  if(!(c->RecFlags & 1))
  {
    c->Simulate(ctx);
  }
}

void Wz4RenderNode::SimulateChilds(Wz4RenderContext *ctx)
{
  Wz4RenderNode *c;

  RecFlags |= 1;
  sInt max = Childs.GetCount();
  if(max==1)
  {
    c = Childs[0];
    if(!(c->RecFlags & 1))
    {
      c->Simulate(ctx);
    }
  }
  else
  {
    sFORALL(Childs,c)
    {
      if(!(c->RecFlags & 1))
      {
        ctx->Script->PushGlobal();
        ctx->Script->ProtectGlobal();
        c->Simulate(ctx);
        ctx->Script->PopGlobal();
      }
    }
  }
}

void Wz4RenderNode::Simulate(Wz4RenderContext *ctx)
{
  SimulateCalc(ctx);
  SimulateChilds(ctx);
}

void Wz4RenderNode::Transform(Wz4RenderContext *ctx,const sMatrix34 &mat)
{
  TransformChilds(ctx,mat);
}

void Wz4RenderNode::TransformChilds(Wz4RenderContext *ctx,const sMatrix34 &mat)
{
  Wz4RenderNode *c;

  Matrices.AddTail(sMatrix34CM(mat));

  sFORALL(Childs,c)
    c->Transform(ctx,mat);
}

void Wz4RenderNode::Prepare(Wz4RenderContext *ctx)
{
  PrepareChilds(ctx);
}

void Wz4RenderNode::PrepareChilds(Wz4RenderContext *ctx)
{
  Wz4RenderNode *c;

  sFORALL(Childs,c)
  {
    if(!(c->RecFlags & ctx->RecMask))
    {
      c->RecFlags |= ctx->RecMask;
      c->Prepare(ctx);
    }
  }
}

void Wz4RenderNode::Render(Wz4RenderContext *ctx)
{
  RenderChilds(ctx);
}

void Wz4RenderNode::RenderChilds(Wz4RenderContext *ctx)
{
  Wz4RenderNode *c;


  sFORALL(Childs,c)
  {
    if(!(c->RecFlags & ctx->RecMask))
    {
      c->RecFlags |= ctx->RecMask;
      c->Render(ctx);
    }
  }
}

/****************************************************************************/

Rendertarget2D::Rendertarget2D()
{
  RealTexture = 0;
  ProxyTexture = new sTextureProxy();
  Texture = ProxyTexture;
  Type = Rendertarget2DType;
  Flags = 0;
  Format = 0;
  SizeX = 256;
  SizeY = 256;
  RealX = 0;
  RealY = 0;
}

Rendertarget2D::~Rendertarget2D()
{
  Flush();
  sDelete(ProxyTexture);
  sDelete(RealTexture);
  Texture = 0;
}

void Rendertarget2D::Init()
{
  Atlas.InitAtlas(AtlasPower,1,1);
}

void Rendertarget2D::Acquire(sInt sx,sInt sy)
{ 
  sInt x,y;

  if((Flags&3)==1)
  {
    x = sx/SizeX;
    y = sy/SizeY;
  }
  else
  {
    x = SizeX;
    y = SizeY;
  }

  if(Flags & 16)         // keep (manual management)
  {
    if(RealTexture==0 || RealX!=x || RealY!=y)
    {
      sDelete(RealTexture);
      RealX = x;
      RealY = y;
      RealTexture = new sTexture2D(RealX,RealY,sTEX_2D|Format|sTEX_NOMIPMAPS|sTEX_RENDERTARGET,1);
    }  
  }
  else                  // discard (use rendertarget manager)
  {
    if(RealTexture)
      sRTMan->Release(RealTexture);
    RealTexture = sRTMan->Acquire(x,y,Format);
    Rendertarget2DType->Targets.AddTail(this);
  }

  ProxyTexture->Connect(RealTexture);
  if(RealTexture)
  {
    sInt xs,ys,zs;
    RealTexture->GetSize(xs,ys,zs);
    Atlas.InitAtlas(AtlasPower,xs,ys);
  }
}

void Rendertarget2D::Flush()
{
  ProxyTexture->Disconnect();
  if(RealTexture)
  {
    if(!(Flags & 16))
    {
      sRTMan->Release(RealTexture);
      RealTexture = 0;
    }
  }
}

void Rendertarget2D::EndShow()
{
  Flush();
}

/****************************************************************************/

void UnitTestWz4Render(sImage &img,UnitTestParaUnitTestWz4 *para,Wz4Render *in0)
{
  if(!sRender3DBegin())
    return;

  // create target

  sInt sx = 1<<( para->Size     & 255);
  sInt sy = 1<<((para->Size>>8) & 255);
  sTexture2D *rt = sRTMan->Acquire(sx,sy,sTEX_ARGB8888);
  sTexture2D *db = sRTMan->Acquire(sx,sy,sTEX_DEPTH24NOREAD);
  sTargetSpec spec(rt,db);
  spec.Aspect = sF32(sx)/sy;

  // prepare render context

  Wz4RenderContext ctx;
  ctx.Init(Wz4RenderType->Script,&spec,0);
  ctx.IppHelper = Wz4RenderType->IppHelper;
  ctx.RenderFlags = wRF_RenderMain;

  // script execution

  ctx.DoScript(in0->RootNode,0,0);

  // rendering, if not already done by camera

  if(ctx.CameraFlag==0)
  {
    sViewport view;
    ctx.View = view;
    ctx.RenderControl(in0->RootNode,sCLEAR_ALL,0,spec);
  }

  // read out texture 

  const sU8 *data;
  sInt pitch;
  sTextureFlags flags;
  sBeginReadTexture(data, pitch, flags,rt);

  img.Init(sx,sy);
  sU32 *dest = img.Data;
  for(sInt y=0;y<sy;y++)
  {
    sU32 *src = (sU32 *) data;
    data += pitch;
    for(sInt x=0;x<sx;x++)
    {
      *dest++ = *src++;
    }
  }

  sEndReadTexture();

  // free up

  sRTMan->Release(rt);
  sRTMan->Release(db);
  sRender3DEnd(0);
}

/****************************************************************************/

