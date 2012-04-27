/****************************************************************************/

#include "view.hpp"
#include "gui.hpp"
#include "util/shaders.hpp"
#include "util/painter.hpp"

#include "wz4lib/basic_ops.hpp"
#include "wz4lib/build.hpp"

/****************************************************************************/
/****************************************************************************/

WinView::WinView()
{
  Op = 0;
  CamOp = 0;
  Object = 0;
  CamObject = 0;
  CalcSlowOps = 0;
  AddNotify(*Doc);
  SetEnable(0);
  Flags |= sWF_CLIENTCLIPPING | sWF_BEFOREPAINT | sWF_HOVER;

  DragRayOp = 0;
  DragRayId = 0;
  Drag3DOffsetX = 0;
  Drag3DOffsetY = 0;
  DragStart.Init(0,0,0);

  GridMode = 1;
  WireMode = 0;
  WireSets = 0;
  Letterbox = 0;

  TeleportOp = 0;
  TeleportHandle = 0;
  TeleportStarted = sFALSE;

  CmdReset3D();
  CmdReset2D();
//  CmdResetAnim();

  FreeCamFlag = 0;
  FpsFlag = 0;
  MTMFlag = 0;
  LetterboxRect.Init();
}

WinView::~WinView()
{
  sRelease(Object);
  sRelease(CamObject);
}

void WinView::Tag()
{
  wHandle *hnd;
  sFORALL(pi.Handles,hnd)
    hnd->Op->Need();
  s3DWindow::Tag();
  Op->Need();
  CamOp->Need();
}

void WinView::InitWire(const sChar *name)
{
  s3DWindow::InitWire(name);
  sWire->AddKey(name,L"Wireframe",sMessage(this,&WinView::CmdToggleWire));
  sWire->AddKey(name,L"Color",sMessage(this,&WinView::CmdColor));
  sWire->AddKey(name,L"Grid__",sMessage(this,&WinView::CmdToggleGrid));
  sWire->AddKey(name,L"Letterbox",sMessage(this,&WinView::CmdToggleLetterbox));
  sWire->AddKey(name,L"Focus",sMessage(this,&WinView::CmdFocusHandle));
  sWire->AddKey(name,L"Reset2D",sMessage(this,&WinView::CmdReset2D));
  sWire->AddKey(name,L"Reset3D",sMessage(this,&WinView::CmdReset3D));
  sWire->AddKey(name,L"Tile",sMessage(this,&WinView::CmdTile));
  sWire->AddKey(name,L"Alpha",sMessage(this,&WinView::CmdAlpha));
  sWire->AddKey(name,L"LockCamera",sMessage(this,&WinView::CmdLockCam));
  sWire->AddKey(name,L"FreeCamera",sMessage(this,&WinView::CmdFreeCam));
  sWire->AddKey(name,L"Fps",sMessage(this,&WinView::CmdFps));
  sWire->AddKey(name,L"MultithreadingMonitor",sMessage(this,&WinView::CmdMTM));
  sWire->AddKey(name,L"Zoom2DIn",sMessage(this,&WinView::CmdZoom,-1));
  sWire->AddKey(name,L"Zoom2DOut",sMessage(this,&WinView::CmdZoom,1));
  sWire->AddKey(name,L"Maximize2",sMessage(this,&WinView::CmdMaximize2));
  sWire->AddKey(name,L"DeleteHandles",sMessage(this,&WinView::CmdDeleteHandles));
  sWire->AddKey(name,L"QuantizeCamera",sMessage(this,&WinView::CmdQuantizeCamera));
  sWire->AddDrag(name,L"Zoom2D",sMessage(this,&WinView::DragZoomBitmap));
  sWire->AddDrag(name,L"Scroll2D",sMessage(this,&WinView::DragScrollBitmap,1));
  sWire->AddDrag(name,L"Scroll2DFast",sMessage(this,&WinView::DragScrollBitmap,10));

  sWire->AddTool(name,L"Handles",sMessage(this,&WinView::CmdHandles));
  sWire->AddKey(name,L"CalcSlowOps",sMessage(this,&WinView::CmdCalcSlowOps));
  sWire->AddDrag(name,L"EditXZ",sMessage(this,&WinView::DragEdit,0));
  sWire->AddDrag(name,L"EditXY",sMessage(this,&WinView::DragEdit,1));
  sWire->AddDrag(name,L"EditY",sMessage(this,&WinView::DragEdit,2));
  sWire->AddTool(name,L"SelectTool",sMessage(this,&WinView::ToolSelect));
  sWire->AddDrag(name,L"SelectFrame",sMessage(this,&WinView::DragSelectFrame,0));
  sWire->AddDrag(name,L"SelectFrameAdd",sMessage(this,&WinView::DragSelectFrame,1));
  sWire->AddDrag(name,L"SelectFrameToggle",sMessage(this,&WinView::DragSelectFrame,2));
  sWire->AddDrag(name,L"SelectSet",sMessage(this,&WinView::DragSelectHandle,0));
  sWire->AddDrag(name,L"SelectAdd",sMessage(this,&WinView::DragSelectHandle,1));
  sWire->AddDrag(name,L"SelectToggle",sMessage(this,&WinView::DragSelectHandle,2));
  sWire->AddDrag(name,L"MoveSelection",sMessage(this,&WinView::DragMoveSelection));
  sWire->AddDrag(name,L"DupSelection",sMessage(this,&WinView::DragDupSelection));
  sWire->AddKey(name,L"HandlesAll",sMessage(this,&WinView::CmdHandlesMode,0));
  sWire->AddKey(name,L"HandlesCurrent",sMessage(this,&WinView::CmdHandlesMode,1));
  sWire->AddKey(name,L"HandlesGroup",sMessage(this,&WinView::CmdHandlesMode,2));
  sWire->AddTool(name,L"SpecialTool",sMessage(this,&WinView::CmdSpecialTool));
  sWire->AddDrag(name,L"SpecialH",sMessage(this,&WinView::DragSpecial,-1));
  sWire->AddDrag(name,L"Special0",sMessage(this,&WinView::DragSpecial,0));
  sWire->AddDrag(name,L"Special1",sMessage(this,&WinView::DragSpecial,1));
  sWire->AddDrag(name,L"Special2",sMessage(this,&WinView::DragSpecial,2));
  sWire->AddDrag(name,L"Special3",sMessage(this,&WinView::DragSpecial,3));
  sWire->AddKey(name,L"Special4",sMessage(this,&WinView::CmdSpecial,4));
  sWire->AddKey(name,L"Special5",sMessage(this,&WinView::CmdSpecial,5));
//  sWire->AddDrag(name,L"Special0_",sMessage(this,&WinView::DragSpecial,0));
//  sWire->AddDrag(name,L"Special1_",sMessage(this,&WinView::DragSpecial,1));
//  sWire->AddDrag(name,L"Special2_",sMessage(this,&WinView::DragSpecial,2));
//  sWire->AddDrag(name,L"Special3_",sMessage(this,&WinView::DragSpecial,3));

  sWire->AddTool(name,L"TeleportHandleTool",sMessage());
  sWire->AddDrag(name,L"TeleportHandle",sMessage(this,&WinView::DragTeleportHandle,0));
  sWire->AddKey(name,L"TeleportAbort",sMessage(this,&WinView::CmdTeleportAbort,0));
  sWire->AddKey(name,L"AddHandleBefore",sMessage(this,&WinView::CmdAddHandle,0));
  sWire->AddKey(name,L"AddHandleAfter",sMessage(this,&WinView::CmdAddHandle,1));

  sWire->AddTool(name,L"ScratchTool",sMessage());
/*
  sWire->AddDrag(name,L"ScrollAnim",sMessage(this,&WinView::DragScrollAnim));
  sWire->AddDrag(name,L"ZoomAnim",sMessage(this,&WinView::DragZoomAnim,3));
  sWire->AddDrag(name,L"ZoomAnimTime",sMessage(this,&WinView::DragZoomAnim,1));
  sWire->AddDrag(name,L"ZoomAnimValue",sMessage(this,&WinView::DragZoomAnim,2));
  */
  sWire->AddDrag(name,L"ScratchSlow",sMessage(App->TimelineWin,&WinTimeline::DragScratchIndirect,0x100));
  sWire->AddDrag(name,L"ScratchFast",sMessage(App->TimelineWin,&WinTimeline::DragScratchIndirect,0x4000));
//  sWire->AddKey(name,L"ResetTime",sMessage(this,&WinView::CmdResetAnim));
}

wObject *WinView::Calc(wOp *op)
{
  wObject *obj = 0;
  if(op)
  {
    obj = Doc->CalcOp(op,(!CalcSlowOps) && !(Doc->EditOptions.Flags &1));
//    App->StackWin->Update();
//    App->TreeViewWin->Update();
    Update();
    App->CalcWasDone();
    CalcSlowOps = 0;
  }
  return obj;
}

void WinView::SetOp(wOp *op)
{
  Op = op;
  ClearNotify();
  AddNotify(*Doc);
  if(Op) AddNotify(*Op);
  Update();

  pi.Dragging = 0;
  App->UpdateImportantOp();
}


void WinView::SetCamOp(wOp *op)
{
  CamOp = op;
}

void WinView::SetPaintInfo(sBool render3D,sViewport *view,sMaterialEnv *env)
{
  pi.Window = this;
  pi.Op = Op;
  pi.Client = Client;
  pi.Rect.Init();
  pi.Lod = Doc->DocOptions.LevelOfDetail;
  pi.MTMFlag = MTMFlag;

  pi.PosX = BitmapPosX;
  pi.PosY = BitmapPosY;
  pi.Zoom2D = BitmapZoom;
  pi.Tile = BitmapTile;
  pi.Alpha = BitmapAlpha;

  pi.BackColor = Doc->EditOptions.BackColor;
  pi.GridColor = Doc->EditOptions.GridColor;
  pi.SplineMode = Doc->EditOptions.SplineMode;

  pi.TimeMS = App->GetTimeMilliseconds();
  pi.TimeBeat = App->GetTimeBeats();

  if(render3D)
  {
    pi.Client = LetterboxRect;
    switch(Doc->EditOptions.MoveSpeed)
    {
      case 0: SideSpeed = ForeSpeed = 0.001f; break;
      case 1: SideSpeed = ForeSpeed = 0.01f; break;
      case 2: SideSpeed = ForeSpeed = 0.1f; break;
      case 3: SideSpeed = ForeSpeed = 1.0f; break;
    }

    sF32 zoom = Zoom;

    if(CamObject)
    {
      if(!CamObject->Type->OverrideCamera(CamObject,*view,zoom,sGetTime()/1000.0f))
        CamOp = 0;
    }

    View.ClipNear = Doc->EditOptions.ClipNear;
    View.ClipFar = Doc->EditOptions.ClipFar;
    pi.View = view;
    pi.View->SetTargetCurrent();
    pi.View->SetZoom(zoom);
    pi.Env = env;
    pi.Grid = GridMode;
    pi.Wireframe = WireMode;
    pi.CamOverride = FreeCamFlag;
    pi.Zoom3D = sClamp<sF32>(zoom,0.125f,64);

    pi.View->Prepare();

    env->AmbientColor = 0xff808080;
    env->LightColor[0] = 0xff808080;
    env->LightDir[0] = pi.View->Camera.k;
    env->Fix();
  }
}

void WinView::CmdHandles(sDInt enable)
{
  pi.ShowHandles = (pi.ShowHandles & ~1) | (enable & 1);
  Update();
}

void WinView::CmdCalcSlowOps()
{
  CalcSlowOps = 1;
  if(Doc && Doc->BlockedChanges)
    Doc->UnblockChange();
  Update();
}

void WinView::ToolSelect(sDInt enable)
{
  pi.ShowHandles = (pi.ShowHandles & ~2) | ((enable<<1)&2);
  Update();
}

void WinView::CmdHandlesMode(sDInt mode)
{
  if(mode==0 && pi.HandleMode==0 && pi.ShowHandles)
  {
    pi.ShowHandles = 0;
  }
  else
  {
    pi.ShowHandles = 1;
    pi.HandleMode = mode;
  }
  Update();
}

void WinView::StartTeleportHandle(wOp *op,sInt handleid)
{
  if(!pi.ShowHandles)
  {
    pi.ShowHandles = 1;
    pi.HandleMode = 1;
    Update();
  }
  
  TeleportOp = op;
  TeleportHandle = handleid;
  TeleportStarted = sFALSE;

  sWire->ChangeCurrentTool(this,L"TeleportHandleTool");
}

void WinView::OnBeforePaint()
{
  sRelease(Object);
  sRelease(CamObject);
  Object = Calc(Op);
  if(Object)
  {
    SetPaintInfo(sFALSE);
    Object->Type->BeforeShow(Object,pi);
  }
  CamObject = Calc(CamOp);
  App->UpdateStatus();

  sBool enable = 0;
  if(Object)
  {
    enable = (Object->Type->Flags & wTF_RENDER3D)?1:0;
    WireSets = Object->Type->GuiSets;
    if(!WireSets) WireSets = -1;
  }
  SetEnable(enable);      // enable 3d view
}

void WinView::OnPaint2D()
{
  if(Object)
  {
    sViewport view(View);
    sMaterialEnv env;

    if(Object->Type->GuiSets & wTG_3D)
    {
      QuakeCam();
      PrepareView();

      sTargetSpec spec;
      spec.Window = Inner;
      CalcLetterBox(spec);

      SetPaintInfo(sTRUE,&view,&env);
      pi.Enable3D = 0;
      pi.ClearFirst = 0;
      pi.Spec = spec;
      pi.Spec.Window = LetterboxRect;
      pi.Spec.Aspect = sF32(LetterboxRect.SizeX())/LetterboxRect.SizeY();
      pi.View->SetTarget(pi.Spec);
      pi.View->SetZoom(pi.Zoom3D);
      pi.View->Prepare();

      if(Flags & sWF_CHILDFOCUS)
        Doc->LastView = *pi.View;

      if(Continuous)
        Update();                     // keep rolling
    }
    else
      SetPaintInfo(sFALSE);

    pi.Enable3D = sFALSE;
    pi.ClearFirst = sTRUE;
    Doc->Show(Object,pi);
    if(pi.DeleteSelectedHandles)
      Update();
  }
  else
  {
    sRect2D(Client,sGC_BACK);
  }

  sRelease(Object);
  sRelease(CamObject);
  pi.DeleteSelectedHandles = 0;
}

void WinView::CalcLetterBox(const sTargetSpec &spec)
{
  if(Letterbox)
  {
    sInt x,y,x0,y0,xs,ys;
    x = spec.Window.CenterX();
    y = spec.Window.CenterY();
    xs = Doc->DocOptions.ScreenX;
    ys = Doc->DocOptions.ScreenY;
    if(xs>spec.Window.SizeX())
    {
      xs = spec.Window.SizeX();
      ys = xs * Doc->DocOptions.ScreenY / Doc->DocOptions.ScreenX;
    }
    if(ys>spec.Window.SizeY())
    {
      ys = spec.Window.SizeY();
      xs = ys * Doc->DocOptions.ScreenX / Doc->DocOptions.ScreenY;
    }
    sVERIFY(xs<=spec.Window.SizeX());
    sVERIFY(ys<=spec.Window.SizeY());
    x0 = x-xs/2;
    y0 = y-ys/2;
    LetterboxRect.Init(x0,y0,x0+xs,y0+ys);

#if sRENDERER!=sRENDER_DX11
    sSetTarget(sTargetPara(sST_CLEARCOLOR|sST_SCISSOR,0xff000000,&spec.Window));
    sRect r = LetterboxRect;
    r.And(spec.Window);
    r.Extend(1);
    sSetTarget(sTargetPara(sST_CLEARCOLOR|sST_SCISSOR,0xffc0c0c0,&r));
#endif
  }
  else
  {
    LetterboxRect = spec.Window;
  }
}

void WinView::Paint(sViewport &view,const sTargetSpec &spec)
{
  Log.Clear();
  if(FreeCamFlag)
  {
    Log.Print(L"free cam\n");
  }
  if(FpsFlag)
  {
    sGraphicsStats stat;
    sGetGraphicsStats(stat);
    static sTiming time;

    time.OnFrame(sGetTime());

    sString<128> b;

    sF32 ms = time.GetAverageDelta();
    Log.PrintF(L"'%f' ms %f fps\n",ms,1000/ms);
    Log.PrintF(L"'%d' batches '%k' verts %k ind %k prim %k splitters\n",
      stat.Batches,stat.Vertices,stat.Indices,stat.Primitives,stat.Splitter);
    if(GridUnit!=0)
      Log.PrintF(L"grid unit = %.0f\n",GridUnit);
  }
  if(CamOp)
  {
    Log.PrintF(L"camera locked");
  }
  if(sGetTime()<GearShiftDisplay)
  {
    if(GearSpeed<1.0f)
      Log.PrintF(L"camera speed /%-6.2f",1.0f/GearSpeed);
    else
      Log.PrintF(L"camera speed *%-6.2f",GearSpeed);
  }

  CalcLetterBox(spec);
  if(Object && !LetterboxRect.IsEmpty())
  {
    sMaterialEnv env;

    SetPaintInfo(sTRUE,&view,&env);
    pi.Enable3D = sTRUE;
    pi.ClearFirst = sTRUE;
    pi.Spec = spec;
    pi.Spec.Window = LetterboxRect;
    pi.Spec.Aspect = sF32(LetterboxRect.SizeX())/LetterboxRect.SizeY();
    pi.View->SetTarget(pi.Spec);
    pi.View->SetZoom(pi.Zoom3D);
    pi.View->Prepare();
    pi.ViewLog = &Log;

    // real painting

    if(Flags & sWF_CHILDFOCUS)
      Doc->LastView = *pi.View;
    DragView.CopyFrom(*pi.View);
    pi.SetCam = 0;
    Doc->Show(Object,pi);
    if(pi.SetCam)
      SetCam(pi.SetCamMatrix,pi.SetCamZoom);
    pi.ViewLog = 0;

    // restore rendertarget for grid
    // this will not work if it is already resolved, since that gives us no zbuffer!

    if(pi.Grid)
    {
      sSetTarget(sTargetPara(0,0,pi.Spec));
      GridColor = Doc->EditOptions.GridColor;
      pi.View->SetTarget(pi.Spec);
      pi.View->SetZoom(Zoom);
      pi.View->Model.Init();
      pi.View->Prepare();
      sEnableGraphicsStats(0);
      PaintGrid();
      sEnableGraphicsStats(1);
      pi.View->Model.Init();
      pi.View->Prepare();
    }
    Grid = 0;   // don't allow the 3dwindow to paint the grid.
    if(pi.DeleteSelectedHandles)
      pi.ClearHandleSelection();
  }
  else
  {
    sSetTarget(sTargetPara(sST_CLEARALL,0xffc0c0c0,Client));
  }
  pi.DeleteSelectedHandles = 0;

  sRelease(Object);
  sRelease(CamObject);



  if(Log.GetCount()>0)
  {
    sEnableGraphicsStats(0);
    sSetTarget(sTargetPara(0,0,pi.Spec));
    App->Painter->SetTarget(LetterboxRect);
    App->Painter->Begin();
    App->Painter->SetPrint(0,0xff000000,Doc->EditOptions.ZoomFont,0xff000000);
    App->Painter->Print(10,11,Log.Get());
    App->Painter->Print(11,10,Log.Get());
    App->Painter->Print(12,11,Log.Get());
    App->Painter->Print(11,12,Log.Get());
    App->Painter->SetPrint(0,0xffc0c0c0,Doc->EditOptions.ZoomFont,0xffffffff);
    App->Painter->Print(11,11,Log.Get());
    App->Painter->End();
    sEnableGraphicsStats(1);
  }
}

/****************************************************************************/

void WinView::CmdReset3D()
{
  CmdReset();
  GearShift = Doc->EditOptions.DefaultCamSpeed;
}

void WinView::CmdColor()
{
  sGridFrame *grid = new sGridFrame;
  sGridFrameHelper gh(grid);
  gh.LabelWidth = 0;
  gh.Left = 0;
  grid->ReqSizeX = 200;
  grid->Columns = 6;
  grid->Flags |= sWF_AUTOKILL;

  gh.Color(&Doc->EditOptions.BackColor,L"rgb");

  sGui->AddPopupWindow(grid);
}

void WinView::CmdToggleWire()
{
  WireMode = !WireMode;
}

void WinView::CmdToggleGrid()
{
  GridMode = !GridMode;
}

void WinView::CmdLockCam()
{
  if(CamOp == App->GetEditOp())
    CamOp = 0;
  else
    CamOp = App->GetEditOp();
  Update();
}

void WinView::CmdFreeCam()
{
  FreeCamFlag = !FreeCamFlag;
  Update();
}

void WinView::CmdFps()
{
  FpsFlag = !FpsFlag;
  Update();
}

void WinView::CmdMTM()
{
  MTMFlag = !MTMFlag;
  Update();
}

void WinView::CmdToggleLetterbox()
{
  Letterbox = !Letterbox;
  Update();
}

void WinView::CmdMaximize2()
{
  static sInt mode = -1;
  if(this==App->ViewWin[0])
  {
    if(sWire->GetScreen()==2)
    {
      sWire->SwitchScreen(mode);
    }
    else
    {
      mode = sWire->GetScreen()==1;
      sWire->SwitchScreen(2);
    }
  }
  else
  {
    CmdMaximize();
  }
}

void WinView::CmdDeleteHandles()
{
  pi.DeleteSelectedHandles = 1;
  for(sInt i=0;i<sCOUNTOF(App->ParaWin);i++)
  {
    if(App->ParaWin[i]->Op)
    {
      App->ParaWin[i]->Op->ArrayGroupMode = 2;
      App->ParaWin[i]->CmdLayout();
    }
  }
}

void WinView::CmdFocusHandle()
{
  wHandle *hnd;
  wHandle *hit = 0;
  sFORALL(pi.Handles,hnd)
  {
    if(hnd->Selected && hnd->x && hnd->y && hnd->z)
      hit = hnd;
  }
  if(!hit)
  {
    sFORALL(pi.Handles,hnd)
    {
      if(hnd->Op==App->GetEditOp())
        hit = hnd;
    }
  }

  if(hit)
  {
    sVector31 v;
    v.x = *hit->x;
    v.y = *hit->y;
    v.z = *hit->z;
    Pos.l = v*hit->Local;
    Pos.l = Pos.l - Pos.k * 20;
    Update();
  }
}

/****************************************************************************/

wHandle *WinView::HitHandle(sInt mx,sInt my)
{
  for(sInt i=pi.Handles.GetCount()-1;i>=0;i--)
    if(pi.Handles[i].HitBox.Hit(mx,my))
      return &pi.Handles[i];

  //wHandle *hnd;
  //sFORALL(pi.Handles,hnd)
  //  if(hnd->HitBox.Hit(mx,my))
  //    return hnd;
  return 0;
}

sBool WinView::OnCheckHit(const sWindowDrag &dd)
{
  return HitHandle(dd.StartX,dd.StartY)!=0;
}

void WinView::OnDrag(const sWindowDrag &dd)
{
  if(dd.Mode==sDD_HOVER)
    MousePointer = HitHandle(dd.MouseX,dd.MouseY) ? sMP_HAND : sMP_ARROW;
  s3DWindow::OnDrag(dd);
}


void WinView::DragEdit(const sWindowDrag &dd,sDInt para)
{
  wHandle *hnd=0;
  wHandle *hnd2=0;
  wHandleSelectTag *tag=0;
  wHandleSelectTag *tag2=0;
  sInt *ip;

  switch(dd.Mode)
  {
  case sDD_START:
    hnd = HitHandle(dd.StartX,dd.StartY);
    DragRayOp = 0;
    DragRayId = 0;
    if(hnd)
    {
      if(!hnd->Selected)
        pi.SelectHandle(hnd);
      tag = &Doc->SelectedHandleTags[hnd->SelectIndex];

      sFORALL(pi.SelectedHandles,ip)
      {
        hnd2 = &pi.Handles[*ip];
        tag2 = &Doc->SelectedHandleTags[hnd2->SelectIndex];
        tag2->Drag.Init(0,0,0,0);
        if(hnd2->x) tag2->Drag.x = *hnd2->x;
        if(hnd2->y) tag2->Drag.y = *hnd2->y;
        if(hnd2->z) tag2->Drag.z = *hnd2->z;
        if(hnd2->t) tag2->Drag.w = *hnd2->t;
      }


      Drag3DOffsetX = (hnd->HitBox.x0+4) - dd.StartX;
      Drag3DOffsetY = (hnd->HitBox.y0+4) - dd.StartY;
      switch(hnd->Mode)
      {
      case wHM_TEX2D:
        break;

      case wHM_RAY:
        DragView.MakeRay(dd.MouseX+Drag3DOffsetX,dd.MouseY+Drag3DOffsetY,pi.HitRay);
        DragStart.Init(0,0,0);
        if(hnd->x) DragStart.x = *hnd->x;
        if(hnd->y) DragStart.y = *hnd->y;
        if(hnd->z) DragStart.z = *hnd->z;
        sFORALL(pi.SelectedHandles,ip)
        {
          wHandle *hnd2 = &pi.Handles[*ip];
          wHandleSelectTag *tag2 = &Doc->SelectedHandleTags[hnd2->SelectIndex];
          tag2->Drag.Init(0,0,0,0);
          if(hnd2->x) tag2->Drag.x = *hnd2->x - DragStart.x;
          if(hnd2->y) tag2->Drag.y = *hnd2->y - DragStart.y;
          if(hnd2->z) tag2->Drag.z = *hnd2->z - DragStart.z;
        }
        break;

      case wHM_PLANE:
        DragView.MakeRay(dd.MouseX+Drag3DOffsetX,dd.MouseY+Drag3DOffsetY,pi.HitRay);
        DragStart = sVector31(tag->Drag);
        break;

      case wHM_PLANE_XY:
        DragView.MakeRay(dd.MouseX+Drag3DOffsetX,dd.MouseY+Drag3DOffsetY,pi.HitRay);
        DragStart = sVector31(tag->Drag);
        break;
      }
      if(pi.HandleMode==0)
      {
//        App->EditOp(hnd->Op,0);
        App->GotoOp(hnd->Op,0);
      }
    }

    pi.Dragging = pi.SelectionNotEmpty();
    Update();
    break;
  case sDD_DRAG:
    sFORALL(pi.SelectedHandles,ip)
    {
      hnd = &pi.Handles[*ip];
      tag = &Doc->SelectedHandleTags[hnd->SelectIndex];
      switch(hnd->Mode)
      {
      case wHM_TEX2D:
        if(hnd->x)
          *hnd->x = tag->Drag.x + dd.DeltaX / sF32(pi.Rect.SizeX());
        if(hnd->y)
          *hnd->y = tag->Drag.y + dd.DeltaY / sF32(pi.Rect.SizeY());
        break;

      case wHM_RAY:
        DragView.MakeRayPixel(dd.MouseX+Drag3DOffsetX,dd.MouseY+Drag3DOffsetY,pi.HitRay);
        break;

      case wHM_PLANE:
        DragView.MakeRayPixel(dd.MouseX+Drag3DOffsetX,dd.MouseY+Drag3DOffsetY,pi.HitRay);
        {
          sMatrix34 mati = hnd->Local;
          mati.Invert34();
          sVector4 plane;
          sVector31 hit;
          sVector31 v = DragStart;
          v = v * hnd->Local;
          if(para==0)
          {
            plane.InitPlane(v,sVector30(0,1,0));
          }
          else
          {
            sVector30 n = -View.Camera.k;
            n.y = 0;
            plane.InitPlane(v,n);
          }

          if(pi.HitRay.HitPlaneDoubleSided(hit,plane))
          {
            v = hit * mati;

            sVector30 delta = v - DragStart;

            if(para==2)
            {
              *hnd->y = tag->Drag.y + delta.y;
            }
            else
            {
              *hnd->x = tag->Drag.x + delta.x;
              *hnd->y = tag->Drag.y + delta.y;
              *hnd->z = tag->Drag.z + delta.z;
            }
          }
        }
        break;

      case wHM_PLANE_XY:
        DragView.MakeRayPixel(dd.MouseX+Drag3DOffsetX,dd.MouseY+Drag3DOffsetY,pi.HitRay);
        {
          sMatrix34 mati = hnd->Local;
          mati.Invert34();
          sVector4 plane;
          sVector31 hit;
          sVector31 v = DragStart;
          v = v * hnd->Local;
          if(para==0)
          {
            plane.InitPlane(v,sVector30(0,0,1));
          }
          else
          {
            sVector30 n = -View.Camera.k;
            n.y = 0;
            plane.InitPlane(v,n);
          }

          if(pi.HitRay.HitPlaneDoubleSided(hit,plane))
          {
            v = hit * mati;

            sVector30 delta = v - DragStart;

            if(para==2)
            {
              *hnd->y = tag->Drag.y + delta.y;
            }
            else
            {
              *hnd->x = tag->Drag.x + delta.x;
              *hnd->y = tag->Drag.y + delta.y;
              *hnd->z = tag->Drag.z + delta.z;
            }
          }
        }
        break;
      }
      if(hnd->Op)
      {
        sGui->Notify(hnd->Op->EditData,hnd->Op->Class->ParaWords*sizeof(sU32));
        Doc->Change(hnd->Op);
        App->ChangeDoc();
      }
    }
    Update();
    break;
  case sDD_STOP:
    pi.Dragging = 0;
    sFORALL(pi.SelectedHandles,ip)
    {
      hnd = &pi.Handles[*ip];
      if(hnd->Op)
      {
        sGui->Notify(hnd->Op->EditData,hnd->Op->Class->ParaWords*sizeof(sU32));
        Doc->Change(hnd->Op);
        App->ChangeDoc();
      }
    }
    Update();
    break;
  }
}

void WinView::DragSelectFrame(const sWindowDrag &dd,sDInt mode)
{
  wHandle *hnd;
  sRect rr;
  wOp *op;
  wOp *lastop=0;
  switch(dd.Mode)
  {
  case sDD_START:
    pi.SelectFrame.x0 = dd.MouseX;
    pi.SelectFrame.y0 = dd.MouseY;
    pi.SelectFrame.x1 = dd.MouseX+1;
    pi.SelectFrame.y1 = dd.MouseY+1;
    pi.SelectMode = 1;
    Update();
    break;
  case sDD_DRAG:
    pi.SelectFrame.x1 = dd.MouseX+1;
    pi.SelectFrame.y1 = dd.MouseY+1;
    Update();
    break;
  case sDD_STOP:
    pi.SelectMode = 0;
    rr = pi.SelectFrame;
    rr.Sort();    
    if(mode==0)
    {
      sFORALL(Doc->AllOps,op)
        op->Select = 0;
      pi.ClearHandleSelection();
    }
    sFORALL(pi.Handles,hnd)
    {
      if(hnd->Op)
      {
        if(rr.Hit(hnd->HitBox.CenterX(),hnd->HitBox.CenterY()))
        {
          if(mode==2)
          {
            hnd->Op->Select = ! hnd->Op->Select;
            pi.SelectHandle(hnd,3);
          }
          else
          {
            hnd->Op->Select = 1;
            pi.SelectHandle(hnd,1);
          }
        }
        if(hnd->Op->Select)
          lastop = hnd->Op;
      }
    }

    App->TreeViewWin->Update();
    Update();
    if(lastop)
      App->EditOp(lastop,0);
    break;
  }
}

void WinView::DragSelectHandle(const sWindowDrag &dd,sDInt mode)
{
  wHandle *hnd;
  wOp *op;
  if(dd.Mode==sDD_START)
  {
    if(mode==0)
    {
      sFORALL(Doc->AllOps,op)
        op->Select = 0;
      pi.ClearHandleSelection();
    }

    sFORALL(pi.Handles,hnd)
    {
      if(hnd->Op)
      {
        if(mode==0)
          hnd->Op->Select = 0;
        if(hnd->HitBox.Hit(dd.StartX,dd.StartY))
        {
          if(mode==2)
          {
            hnd->Op->Select = ! hnd->Op->Select;
            pi.SelectHandle(hnd,3);
          }
          else
          {
            hnd->Op->Select = 1;
            pi.SelectHandle(hnd,1);
          }
        }
      }
    }    
  }
}

void WinView::DragMoveSelection(const sWindowDrag &dd)
{
/*
  wHandle *hnd;
  switch(dd.Mode)
  {
  case sDD_START:
    sFORALL(pi.Handles,hnd)
    {
      if(hnd->Op && hnd->Op->Select)
      {
        hnd->
      }
    }

  }
  */
}

void WinView::DragDupSelection(const sWindowDrag &dd)
{
}

void WinView::DragSpecial(const sWindowDrag &dd,sDInt mode)
{
  wOp *op = App->GetEditOp();
  if(op && op->Class->SpecialDrag)
  {
    op->Class->SpecialDrag(dd,mode,op,View,pi);
    if(dd.Mode==sDD_STOP)
      App->EditOp(op,0);
    App->ChangeDoc();
  }
}

void WinView::CmdSpecialTool(sDInt mode)
{
  if(mode==0)
    CmdSpecial(6);
}

void WinView::CmdSpecial(sDInt mode)
{
  sWindowDrag dd;
  sClear(dd);
  dd.Mode = mode;
  sGui->GetMousePos(dd.MouseX,dd.MouseY);
  dd.StartX = dd.MouseX;
  dd.StartY = dd.MouseY;

  wOp *op = App->GetEditOp();
  if(op && op->Class->SpecialDrag)
  {
    op->Class->SpecialDrag(dd,mode,op,View,pi);
    App->EditOp(op,0);
    App->ChangeDoc();
  }  
}

void WinView::DragTeleportHandle(const sWindowDrag &dd,sDInt mode)
{
  if(!TeleportStarted)
  {
    wHandle *hit = 0,*hnd;
    sFORALL(pi.Handles,hnd)
    {
      if(hnd->Op == TeleportOp && hnd->Id == TeleportHandle)
        hit = hnd;
    }

    if(hit)
    {
      pi.SelectHandle(hit);
      Doc->Change(TeleportOp);
      TeleportStarted = sTRUE;
    }
  }

  sInt *ip;
  sFORALL(pi.SelectedHandles,ip)
  {
    wHandle *hnd = &pi.Handles[*ip];

    if(hnd->Op && (hnd->Mode==wHM_RAY || hnd->Mode==wHM_PLANE))
    {
      switch(dd.Mode)
      {
      case sDD_START:
        pi.Dragging = 1;
        DragView.MakeRayPixel(dd.MouseX,dd.MouseY,pi.HitRay);
        sGui->Notify(hnd->Op->EditData,hnd->Op->Class->ParaWords*sizeof(sU32));
        Doc->Change(hnd->Op);
        App->ChangeDoc();
        Update();
        break;

      case sDD_DRAG:
        DragView.MakeRayPixel(dd.MouseX,dd.MouseY,pi.HitRay);
        sGui->Notify(hnd->Op->EditData,hnd->Op->Class->ParaWords*sizeof(sU32));
        Doc->Change(hnd->Op);
        App->ChangeDoc();
        Update();
        break;

      case sDD_STOP:
        pi.Dragging = 0;
        sGui->Notify(hnd->Op->EditData,hnd->Op->Class->ParaWords*sizeof(sU32));
        Doc->Change(hnd->Op);
        App->ChangeDoc();
        Update();
        break;
      }
    }
  }

  if(dd.Mode==sDD_STOP)
    CmdTeleportAbort(0);
}

void WinView::CmdTeleportAbort(sDInt mode)
{
  sWire->ChangeCurrentTool(this,0);
}

void WinView::CmdQuantizeCamera()
{
  sVector30 v = Pos.k;
  sF32 ax = sFAbs(v.x);
  sF32 ay = sFAbs(v.y);
  sF32 az = sFAbs(v.z);

  if(ax > ay && ax > az)  v.Init(sSign(v.x),0,0);
  if(ay > az           )  v.Init(0,sSign(v.y),0);
  else                    v.Init(0,0,sSign(v.z));

  Pos.LookAt(Pos.l+v,Pos.l);
  Pos.FindEulerXYZ2(RotX,RotY,RotZ);

  Update();
}

void WinView::CmdAddHandle(sDInt before_or_after)
{
  wHandle *hnd;
  wOp *op = App->GetEditOp();
  sInt line = op->HighlightArrayLine;
  if(op && line>=0 && line<op->ArrayData.GetCount())
  {
    if(before_or_after)
      line++;
    op->AddArray(line);

    sFORALL(pi.Handles,hnd)
    {
      if(hnd->Op==op && hnd->ArrayLine==line)
        pi.SelectHandle(hnd);
    }
    sGui->Notify(op);
    Doc->Change(op);
    App->ChangeDoc();
    App->Update();
    for(sInt i=0;i<sCOUNTOF(App->ParaWin);i++)
      App->ParaWin[i]->SetOp(App->ParaWin[i]->Op);
  }
}

/****************************************************************************/

void WinView::CmdReset2D()
{
  BitmapPosX = 10;
  BitmapPosY = 10;
  BitmapZoom = 8;
  BitmapAlpha = 0;
  BitmapTile = 0;
  Update();
}

void WinView::CmdTile()
{
  BitmapTile = !BitmapTile;
  Update();
}

void WinView::CmdAlpha()
{
  BitmapAlpha = !BitmapAlpha;
  Update();
}

void WinView::CmdZoom(sDInt inc)
{
  BitmapZoom = sClamp<sInt>(BitmapZoom+inc,0,15);
  Update();
}

void WinView::DragScrollBitmap(const sWindowDrag &dd,sDInt speed)
{
  switch(dd.Mode)
  {
  case sDD_START:
    BitmapPosXStart = BitmapPosX;
    BitmapPosYStart = BitmapPosY;
    break;
  case sDD_DRAG:
    BitmapPosX = BitmapPosXStart + dd.HardDeltaX*speed;
    BitmapPosY = BitmapPosYStart + dd.HardDeltaY*speed;
    Update();
    break;
  }
}

void WinView::DragZoomBitmap(const sWindowDrag &dd)
{
  switch(dd.Mode)
  {
  case sDD_START:
    BitmapPosYStart = BitmapZoom;
    break;
  case sDD_DRAG:
    BitmapZoom = sClamp((BitmapPosYStart*32+16+dd.HardDeltaY)/32,0,15);
    Update();
    break;
  }
}

/****************************************************************************/
