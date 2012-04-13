// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "winview.hpp"
#include "winpara.hpp"
#include "winspline.hpp"
#include "material11.hpp"
#include "genbitmap.hpp"
#include "genmesh.hpp"
#include "genscene.hpp"
#include "genoverlay.hpp"
#include "genmaterial.hpp"
#include "genoverlay.hpp"
#include "genblobspline.hpp"
#include "engine.hpp"
#include "novaplayer.hpp"
#include "rtmanager.hpp"

sMAKEZONE(ExecTree    ,"ExecTree "  ,0xff40b040);

const static sF32 CamSpeedTable[] = { 0.1f,1,8,16 };

/****************************************************************************/

#define DMB_SCROLL    1
#define DMB_ZOOM      2
#define DM_ORBIT      3
#define DM_DOLLY      4
#define DM_SCROLL     5
#define DM_ZOOM       6
#define DM_ROTATE     7
#define DM_PIVOT      8
#define DM_EDITT      9
#define DM_EDITR      10
#define DMB_LIGHT     11
#define DM_SCRATCHTIME 12

/****************************************************************************/
/****************************************************************************/

WinView::WinView()
{
  sInt i;
  sMatrix mat;

  Flags |= sGWF_PAINT3D;

  Game.Init();
  sREGZONE(ExecTree);

  DragStartX = 0;
  DragStartY = 0;
  DragMode = 0;
  DragCID = 0;
  DragEditPtrX = 0;
  DragEditPtrY = 0;
  DragEditOp = 0;
  Object = 0;
  RealObject = 0;
  BrowserMode = 0;
  QuakeMask = 0;
  QuakeSpeedForw = 0;
  QuakeSpeedSide = 0;
  QuakeSpeedUp = 0;
  GameMode = 0;
  ShowPivotVecTimer = 0;
  Quant90Degree = 0;
  RecordVideo = 0;
  ShowNormals = 0;
  ForceCameraLight = 0;

  MatMeshType = 0;
  MatMesh = 0;
  ScratchTimeMode = 0;

  SelEdge = 1;
  SelVertex = 1;
  SelFace = 1;
  ShowLights = 0;
  ShowXZPlane = 0;
  CamSpeed = 1;

  LightPosX = LightPosY = 0;

	Cam.State[0]=Cam.State[1]=Cam.State[2]=1.0f;
	Cam.State[3]=Cam.State[4]=Cam.State[5]=0.0f;
	Cam.State[6]=Cam.State[7]=0.0f; Cam.State[8]=-5.0f;
	CamMatrix.InitSRT(Cam.State);
  Cam.Ortho = 0;
  Cam.Pivot = 5.0f;
  PivotVec.Init();

  for(i=0;i<16;i++)
    CamSave[i] = Cam;
  Fullsize = 0;

  LastTime = sSystem->GetTime();

  WireColor[EWC_WIREHIGH]   = 0xffffffff;
  WireColor[EWC_WIRELOW]    = 0xff808080;
  WireColor[EWC_WIRESUB]    = 0xff102040;
  WireColor[EWC_COLADD]     = 0xff408040;
  WireColor[EWC_COLSUB]     = 0xff404080;
  WireColor[EWC_VERTEX]     = 0xff808080;
  WireColor[EWC_HINT]       = 0xffffff00;
  WireColor[EWC_HIDDEN]     = 0x80202020;
  WireColor[EWC_FACE]       = 0x80804020;
  WireColor[EWC_GRID]       = 0xff404040;
  WireColor[EWC_BACKGROUND] = 0xff000000;
  WireColor[EWC_COLZONE]    = 0xff804040;
  WireColor[EWC_PORTAL]     = 0xff004080;
  WireOptions = EWF_ALL;
  HintSize = 0.125f;
  ShowOrigSize = 0;

  MtrlEnv.Init();
  MtrlEnv.Orthogonal = sMEO_PIXELS;

  MtrlTex = new sMaterial11;
  MtrlTex->ShaderLevel = sPS_00;
  MtrlTex->SetTex(0,0);
  MtrlTex->TFlags[0] = 0;
  MtrlTex->Combiner[sMCS_TEX0] = sMCOA_SET;
  MtrlTex->BaseFlags = sMBF_NONORMAL;
  MtrlTex->Compile();

  MtrlTexAlpha = new sMaterial11;
  MtrlTexAlpha->CopyFrom(MtrlTex);
  MtrlTexAlpha->BaseFlags |= sMBF_BLENDALPHA;
  MtrlTexAlpha->AlphaCombiner = sMCA_TEX0;
  MtrlTexAlpha->Compile();



  if(GenOverlayManager->CurrentShader==sPS_00)
  {
    MtrlNormal = new sMaterial11;
    MtrlNormal->CopyFrom(MtrlTex);
    MtrlNormalLight = new sMaterial11;
    MtrlNormalLight->CopyFrom(MtrlTex);
  }
  else
  {
    MtrlNormal = new sMaterial11;
    MtrlNormal->CopyFrom(MtrlTex);
    MtrlNormal->ShaderLevel = sPS_11;
    MtrlNormal->Combiner[sMCS_COLOR0] = sMCOA_SET;
    MtrlNormal->Combiner[sMCS_TEX0] = sMCOA_MUL;
    MtrlNormal->Combiner[sMCS_COLOR2] = sMCOA_ADD;
    MtrlNormal->Color[0] = 0x00808080;
    MtrlNormal->Color[2] = 0x00808080;
    MtrlNormal->BaseFlags = sMBF_NONORMAL;
    MtrlNormal->Compile();

    MtrlNormalLight = new sMaterial11;
    MtrlNormalLight->ShaderLevel = sPS_11;
    MtrlNormalLight->BaseFlags = sMBF_ZOFF;
    MtrlNormalLight->LightFlags = sMLF_BUMPX;
    MtrlNormalLight->SpecPower = 24.0f;
    MtrlNormalLight->Combiner[sMCS_LIGHT] = sMCOA_SET;
    MtrlNormalLight->Combiner[sMCS_COLOR0] = sMCOA_MUL;
    MtrlNormalLight->Color[0] = 0x80c0c0c0;
    MtrlNormalLight->SetTex(1,0);
    MtrlNormalLight->TFlags[1] = 0;
    MtrlNormalLight->Compile();
  }

  MtrlFlatZ = new sMaterial11;
  MtrlFlatZ->ShaderLevel = sPS_00;
  MtrlFlatZ->BaseFlags = sMBF_NONORMAL|sMBF_NOTEXTURE|sMBF_ZON;
  MtrlFlatZ->Combiner[sMCS_VERTEX] = sMCOA_SET;
  MtrlFlatZ->Compile();

  MtrlCollision = new sMaterial11;
  MtrlCollision->CopyFrom(MtrlFlatZ);
  MtrlCollision->BaseFlags = sMBF_NONORMAL|sMBF_NOTEXTURE|sMBF_ZOFF|sMBF_DOUBLESIDED|sMBF_BLENDADD|sMBF_INVERTCULL;
  MtrlCollision->Compile();

  GeoLine = sSystem->GeoAdd(sFVF_COMPACT,sGEO_LINE|sGEO_DYNAMIC);
  GeoFace = sSystem->GeoAdd(sFVF_COMPACT,sGEO_TRI|sGEO_DYNAMIC);

  sFSRT srt;
  srt.s.Init(1,1,1);
  srt.r.Init(0,0,0);
  srt.t.Init(0,0,0);
  MatSrcMesh[0] = Mesh_Cube(8,8,8,1,srt);

  mat.Init();
  mat.i.x=2; mat.j.y=2; mat.k.z=2;
  MatSrcMesh[0]->TransVert(mat,0,0);
  MatSrcMesh[1] = Mesh_Torus(32,16,2.0f/1.5f,1.0f/1.5f,0.0f,1.0f,0);

  OnCommand(CMD_VIEW_RESET);
}


WinView::~WinView()
{
  MatSrcMesh[0]->Release();
  MatSrcMesh[1]->Release();
  if(MatMesh)
  {
    MatMesh->Release();
    MatMesh = 0;
  }

  sRelease(MtrlTex);
  sRelease(MtrlTexAlpha);
  sRelease(MtrlNormal);
  sRelease(MtrlNormalLight);
  sRelease(MtrlFlatZ);
  sRelease(MtrlCollision);

  sSystem->GeoRem(GeoLine);
  sSystem->GeoRem(GeoFace);
  Game.Exit();
}

void WinView::Tag()
{
  sGuiWindow::Tag();
  sBroker->Need(Object);
  sBroker->Need(RealObject);
  sBroker->Need(DragEditOp);
}

void WinView::SetObjectIntern(WerkOp *to,sBool real)
{
  if(Object!=to)
  {
    if(Object && Object->Op.Cache)
    {
      sInt clsId = Object->Op.Cache->ClassId;
      if(clsId == KC_MESH)
        FlushMeshWire();
      else if(clsId == KC_SCENE)
        FlushSceneWire();
    }

    Object = to;
    if(real)
    {
      RealObject = to;
      BrowserMode = 0;
    }
    else
    {
      BrowserMode = 1;
    }
    sGui->GarbageCollection();
    GameMode = 0;
    App->Doc->ClearInstanceMem();

    if(MatMesh)
    {
      MatMesh->Release();
      MatMesh = 0;
    }
  }
}

void WinView::SetObject(WerkOp *to)
{
  SetObjectIntern(to,1);
  App->SetView(0);
}

void WinView::SetObjectB(WerkOp *to)
{
  SetObjectIntern(to,0);
  App->SetView(0);
}


void WinView::SetOff()
{
  SetObjectIntern(0,1);
}

void WinView::OnFrame()
{
  if(BrowserMode)
  {
    if(!App->OpBrowserWin || App->OpBrowserWin->Parent==0)
    {
      SetObject(RealObject);
    }
  }
}

/****************************************************************************/

void WinView::OnPaint()
{
  if(GenOverlayManager->LinkEdit)
    sPainter->Print(sGui->PropFont,Client.x0+10,Client.y0+10,"LINK EDIT!",0xffff8080);
  if(GenOverlayManager->FreeCamera)
    sPainter->Print(sGui->PropFont,Client.x0+10,Client.y0+10,"FREE CAMERA!",0xffff8080);


  if(sGui->GetFocus()!=this)
    ScratchTimeMode = 0;

  if(ShowOrigSize && (DragCID==KC_MESH || DragCID==KC_MINMESH || DragCID==KC_SCENE || DragCID==KC_IPP || DragCID==KC_DEMO))
  {
    sInt xs,ys;
    sRect r,r1;

    r = Client;
    xs = App->Doc->ResX;
    ys = App->Doc->ResY;
    if(xs>r.XSize())
    {
      xs = r.XSize();
      ys = r.XSize()*App->Doc->ResY/App->Doc->ResX;
    }
    if(ys>r.YSize())
    {
      ys = r.YSize();
      xs = sMin(xs,r.YSize()*App->Doc->ResX/App->Doc->ResY);
    }

    r.x0 = r.x0+(r.x1-r.x0-xs)/2;
    r.y0 = r.y0+(r.y1-r.y0-ys)/2;
    r.x1 = r.x0+xs;
    r.y1 = r.y0+ys;

    r1 = r;
    r.Extend(1);
    sPainter->PaintHole(sGui->FlatMat,Client,r,WireColor[EWC_BACKGROUND]|0xff000000);
    sPainter->PaintHole(sGui->FlatMat,r,r1,0xff808080);
  }

}

void WinView::OnPaint3d(sViewport &viewPort)
{
  sInt cid;
  sF32 speedy;
  GenBitmap *gbm;
  sChar buffer[256];
  sInt i;
  sU32 qual;
  static sInt time;
  WerkOp *ShowObject;             // object after bypass 
  WerkEvent *we;
  sInt beat,ms;
  sInt vidBeat,vidMs;
  sViewport view;

// consistency check on data structures in debug
#ifdef _DEBUG
  KMemoryManager->Validate();
#endif

  view = viewPort;

// quake cam logic, not frame synced
  /*
  {
    static sChar buffer[256];
    sSPrintF(buffer,256,"%f %f %f",Cam.State[3],Cam.State[4],Cam.State[5]);
    App->SetStat(3,buffer);
  }
*/
  if(sGui->GetFocus()!=this)
  {
    QuakeSpeedForw=0;
    QuakeSpeedSide=0;
    QuakeSpeedUp=0;
    QuakeMask=0;
  }

  ThisTime = sSystem->GetTime();
  sSystem->GetPerf(Perf,sPIM_BEGIN);
  StatWire = 0;
  StatWireVert = 0;
  StatWireLine = 0;
  StatWireTri = 0;

  // setup viewport/time (recording)
  if(RecordVideo)
  {
    sSystem->VideoViewport(view);
    vidMs = VideoFrame * 1000.0f / VideoFPS;
    vidBeat = VideoFrame * App->Doc->SoundBPM * 65536.0f / (VideoFPS * 60.0f);

    if((VideoFrame % 30) == 0)
      sDPrintF("%d frames rendered (%.2f%%)\n",VideoFrame,100.0f*vidBeat/App->Doc->SoundEnd);

    if(view.RenderTarget == sINVALID || vidBeat >= App->Doc->SoundEnd) // end of video reached
    {
      RecordVideo = 0;
      sSystem->VideoEnd();

      sDPrintF("done.\n");
      view = viewPort;
    }

    VideoFrame++;
  }

  speedy = 1;
  if(sSystem->GetKeyboardShiftState()&sKEYQ_SHIFT)
    speedy = 4;
  speedy *= CamSpeedTable[CamSpeed];
  for(i=LastTime/10;i<ThisTime/10;i++)
  {
    if(QuakeMask &  1) QuakeSpeedForw += speedy;
    if(QuakeMask &  2) QuakeSpeedForw -= speedy;
    if(QuakeMask &  4) QuakeSpeedSide += speedy;
    if(QuakeMask &  8) QuakeSpeedSide -= speedy;
    if(QuakeMask & 16) QuakeSpeedUp += speedy;
    if(QuakeMask & 32) QuakeSpeedUp -= speedy;
    QuakeSpeedForw *= 0.70f;
    QuakeSpeedSide *= 0.70f;
    QuakeSpeedUp   *= 0.70f;
	  Cam.State[6] += 0.03f*(QuakeSpeedSide*CamMatrix.i.x+QuakeSpeedUp*CamMatrix.j.x+QuakeSpeedForw*CamMatrix.k.x);
	  Cam.State[7] += 0.03f*(QuakeSpeedSide*CamMatrix.i.y+QuakeSpeedUp*CamMatrix.j.y+QuakeSpeedForw*CamMatrix.k.y);
	  Cam.State[8] += 0.03f*(QuakeSpeedSide*CamMatrix.i.z+QuakeSpeedUp*CamMatrix.j.z+QuakeSpeedForw*CamMatrix.k.z);
  }

  if(Quant90Degree)
  {
    Cam.State[3] = (sFRound(Cam.State[3]*4+1024.5)-1024)/4;
    Cam.State[4] = (sFRound(Cam.State[4]*4+1024.5)-1024)/4;
    Cam.State[5] = (sFRound(Cam.State[5]*4+1024.5)-1024)/4;
    Quant90Degree = 0;
  }
  if(DragMode==DM_ORBIT)
  {
		Cam.State[6] = PivotVec.x - CamMatrix.k.x * Cam.Pivot;
		Cam.State[7] = PivotVec.y - CamMatrix.k.y * Cam.Pivot;
		Cam.State[8] = PivotVec.z - CamMatrix.k.z * Cam.Pivot;
  }
  CamMatrix.InitSRT(Cam.State);
  if(sSystem->GetKeyboardShiftState()&sKEYQ_SHIFT)
    ShowPivotVecTimer++;

// calculate current object

  cid = KC_NULL;
  ShowObject = Object;
  if(ShowObject && ShowObject->Hide)
    ShowObject = 0;
  if(ShowObject)
    App->Doc->Connect();

  App->MusicNow(beat,ms);

  if(RecordVideo)
  {
    beat = vidBeat;
    ms = vidMs;
  }

  App->Doc->KEnv->InitFrame(beat,ms);
  App->Doc->KEnv->Splines = App->Doc->EnvKSplines.Array;
  App->Doc->KEnv->SplineCount = App->Doc->EnvKSplines.Count;
  App->Doc->KEnv->Game = &Game;

  for(i=0;i<App->Doc->Events->GetCount();i++)
  {
    we = App->Doc->Events->Get(i);
    if(we->Event.Op)
      App->Doc->KEnv->AddStaticEvent(&we->Event);
  }

  if(ShowObject)
  {
    while(!ShowObject->Error && ShowObject->Bypass && ShowObject->GetInput(0))
      ShowObject = ShowObject->GetInput(0);

    i = sSystem->GetTime();
    if(ShowObject->Error==0)
    {
      if(ShowObject->Op.Cache && ShowObject->Op.Cache->ClassId == KC_SCENE)
        UpdateSceneWire();

      if(ShowObject->Op.Calc(App->Doc->KEnv,KCF_NEED))
      {
        time = sSystem->GetTime()-i;
        if(time>500)
          sGui->SetStyle(sGui->GetStyle()|sGS_NOCHANGE);
        if(time<400)
          sGui->SetStyle(sGui->GetStyle()&~sGS_NOCHANGE);

        if(ShowObject->Op.Cache && ShowObject->Op.Cache->ClassId == KC_MATERIAL)
        {
          if(MatMesh)
            MatMesh->Release();

          MatMesh = 0;
        }

        // in game mode, recalcing current op may mean addresses change,
        // so reset collision info
        if(GameMode)
        {
          LeaveGameMode();
          EnterGameMode();
        }
      }
    }

    if(ShowObject->Op.Cache)
      cid = ShowObject->Op.Cache->ClassId;
  }
  App->Doc->KEnv->Mem.Flush();

  if(GenOverlayManager->LinkEdit)
    App->SplineWin->LinkEdit(Cam.State,0);


// display dragmode in status bar

//  kenv.Var[0].Init(0,0,0,0);

  if(Flags&sGWF_CHILDFOCUS)
  {
    qual = sSystem->GetKeyboardShiftState();
    if(ScratchTimeMode)
    {
      App->SetStat(STAT_DRAG,"l:time(slow)   r:time(fast)");
    }
    else
    {
      if(qual&sKEYQ_CTRL)
      {
        if(cid==KC_BITMAP)
          App->SetStat(STAT_DRAG,"l:scroll   m:scroll [menu]   r:zoom");
        else
          App->SetStat(STAT_DRAG,"l:zoom   m:scroll [menu]   r:dolly [wire]");
      }
      else if(qual&sKEYQ_ALT)
      {
        if(cid==KC_BITMAP)
          App->SetStat(STAT_DRAG,"l:scroll   m:scroll [menu]   r:zoom");
        else
          App->SetStat(STAT_DRAG,"l:pivot   m:scroll [menu]   r:orbit [wire]");
      }
      else if(qual&sKEYQ_SHIFT)
      {
        if(cid==KC_BITMAP)
          App->SetStat(STAT_DRAG,"l:scroll   m:scroll [menu]   r:zoom");
        else
          App->SetStat(STAT_DRAG,"l:edit x   m:edit y [menu]   r:edit z");
      }
      else
      {
        if(cid==KC_BITMAP)
          App->SetStat(STAT_DRAG,"l:scroll   m:scroll [menu]   r:zoom");
        else
          App->SetStat(STAT_DRAG,"l:rotate   m:scroll [menu]   r:orbit [wire]");
      }
    }
  }

// calculate sized viewport


  if(ShowOrigSize && (cid==KC_MESH || DragCID==KC_MINMESH || cid==KC_SCENE || cid==KC_IPP || cid==KC_DEMO))
  {
    sInt xs,ys;
    sRect r;

    xs = App->Doc->ResX;
    ys = App->Doc->ResY;
    if(xs>view.Window.XSize())
    {
      xs = view.Window.XSize();
      ys = view.Window.XSize()*App->Doc->ResY/App->Doc->ResX;
    }
    if(ys>view.Window.YSize())
    {
      ys = view.Window.YSize();
      xs = sMin(xs,view.Window.YSize()*App->Doc->ResX/App->Doc->ResY);
    }

    r = view.Window;
    view.Window.x0 = r.x0+(r.x1-r.x0-xs)/2;
    view.Window.y0 = r.y0+(r.y1-r.y0-ys)/2;
    view.Window.x1 = view.Window.x0+xs;
    view.Window.y1 = view.Window.y0+ys;
  }

// display as needed


  App->Doc->KEnv->Game->AddEvents(App->Doc->KEnv);
  App->Doc->KEnv->Game->FlushPhysic();
  MakeCam(App->Doc->KEnv->GameCam,view);

  if(ShowObject==0 || ShowObject->Error) 
    cid = 0;
  App->SetStat(STAT_VIEW,"Nothing Showed");

  // update rendertarget manager viewport
  RenderTargetManager->SetMasterViewport(view);

  // update wireframe colors
  EngMesh::SetWireColors(WireColor);

  DragCID = cid;
  switch(cid)
  {
  case KC_BITMAP:
    gbm = (GenBitmap *) ShowObject->Op.Cache;
    if(gbm->Texture==sINVALID)
    {
      gbm->MakeTexture(gbm->Format);
    }
    ShowBitmap(view,gbm);
    if(BitmapAlpha)
    {
      if(BitmapTile)
        App->SetStat(STAT_VIEW,"Bitmap, tiled, alpha");
      else
        App->SetStat(STAT_VIEW,"Bitmap, alpha");
    }
    else
    {
      if(BitmapTile)
        App->SetStat(STAT_VIEW,"Bitmap, tiled");
      else
        App->SetStat(STAT_VIEW,"Bitmap");
    }
    break;
  case KC_SCENE:
    ShowScene(view,ShowObject,App->Doc->KEnv);
    break;
  case KC_MESH:
    ShowMesh(view,(GenMesh *)ShowObject->Op.Cache,App->Doc->KEnv);
    break;
  case KC_MINMESH:
    ShowMinMesh(view,(GenMinMesh *)ShowObject->Op.Cache,App->Doc->KEnv);
    break;
  case KC_SMESH:
    ShowSimpleMesh(view,(GenSimpleMesh *)ShowObject->Op.Cache,App->Doc->KEnv);
    break;
  case KC_IPP:
    ShowIPP(view,ShowObject,App->Doc->KEnv);
    break;
  case KC_DEMO:
    ShowDemo(view,ShowObject,App->Doc->KEnv);
    break;
  case KC_MATERIAL:
    ShowMaterial(view,ShowObject,App->Doc->KEnv);
    break;
  default:
    DragCID = 0;
    sSystem->SetViewport(view);
    sSystem->Clear(sVCF_ALL,sGui->Palette[sGC_BACK]);
    break;
  }
  App->Doc->KEnv->ExitFrame();
  App->Doc->KEnv->Mem.Flush();

// Encode+Display rendered video image for video recording
  if(RecordVideo)
  {
    sF32 scale[2];
    sInt rttex = sSystem->VideoWriteFrame(scale[0],scale[1]);

    // clear black
    view = viewPort;
    sSystem->SetViewport(view);
    sSystem->Clear(sVCF_ALL,0);

    // display
    view = viewPort;

    sInt xs = App->Doc->ResX;
    sInt ys = App->Doc->ResY;
    if(xs>view.Window.XSize())
    {
      xs = view.Window.XSize();
      ys = view.Window.XSize()*App->Doc->ResY/App->Doc->ResX;
    }
    if(ys>view.Window.YSize())
    {
      ys = view.Window.YSize();
      xs = sMin(xs,view.Window.YSize()*App->Doc->ResX/App->Doc->ResY);
    }

    sRect r = view.Window;
    view.Window.x0 = r.x0+(r.x1-r.x0-xs)/2;
    view.Window.y0 = r.y0+(r.y1-r.y0-ys)/2;
    view.Window.x1 = view.Window.x0+xs;
    view.Window.y1 = view.Window.y0+ys;

    sSystem->SetViewport(view);
    GenOverlayManager->Mtrl[GENOVER_TEX1]->SetTex(0,rttex);
    GenOverlayManager->Mtrl[GENOVER_TEX1]->Color[0] = 0xffffffff;
    GenOverlayManager->FXQuad(GENOVER_TEX1,scale);
  }

// Show the camera pivot for some time when the mouse wheel was turned

  ShowPivotVecTimer -= ThisTime-LastTime;
  if(ShowPivotVecTimer<0)
    ShowPivotVecTimer = 0;

// performance counters

  sSystem->GetPerf(Perf,sPIM_END);

  if(!StatWire)
  {
    StatWireTri = Perf.Triangle;
    StatWireLine =  Perf.Line;
    StatWireVert = Perf.Vertex;
  }

  sSPrintF(buffer,sizeof(buffer),"%07.3f fps / %07.3f ms, %d tri, %d line, %d vert, %d batches, calc %4d ms",1000000.0f/Perf.TimeFiltered,Perf.TimeFiltered/1000.0f,StatWireTri,StatWireLine,StatWireVert,Perf.Batches,time);
  App->SetStat(STAT_FPS,buffer);

  if(GameMode)
  {
    sInt max;
    max = ThisTime/10-LastTime/10;
    if(max>10) max = 10;
    Game.OnTick(App->Doc->KEnv,max);
    sSystem->Sample3DCommit();
  }

  LastTime = ThisTime;

  if((Flags & sGWF_FOCUS) && (cid == KC_SCENE || cid == KC_MESH || cid == KC_MINMESH || cid == KC_IPP || cid == KC_DEMO))
  {
    sSPrintF(buffer,sizeof(buffer),"cam: %.3f / %.3f / %.3f",CamMatrix.l.x,CamMatrix.l.y,CamMatrix.l.z);
    App->SetStat(STAT_MESSAGE,buffer);
  }
}

/****************************************************************************/

void WinView::OnKey(sU32 key)
{
  if(key&sKEYQ_CTRL) key |= sKEYQ_CTRL;
  if(key&sKEYQ_SHIFT) key |= sKEYQ_SHIFT;

  switch(key&(0x8001ffff|sKEYQ_SHIFT|sKEYQ_CTRL|sKEYQ_ALT))
  {
    // uijyxbnm
  case sKEY_APPPOPUP:
    OnCommand(CMD_VIEW_POPUP);
    break;
  case 'r':                       // reset
  case sKEY_F12:    // für thomas seine 8-tastem maus!
    OnCommand(CMD_VIEW_RESET);
    break;
  case 'c':                       // color
    OnCommand(CMD_VIEW_COLOR);
    break;
  case 'C'|sKEYQ_SHIFT:           // force camera light
    OnCommand(CMD_VIEW_FORCECAMLIGHT);
    break;
  case sKEY_TAB:                  // fullscreen
    if(!(key&sKEYQ_ALT))
    {
      Fullsize = 1-Fullsize;
      this->SetFullsize(Fullsize);
    }
    break;

  case '^':                       // mesh: wireframe
    OnCommand(CMD_VIEW_WIRE);
    break;
  case 'v':                       // mesh: selection mask
    OnCommand(CMD_VIEW_SELECTION);
    break;
  case 'L'|sKEYQ_SHIFT:           // mesh: show light
    OnCommand(CMD_VIEW_SHOWLIGHTS);
    break;
  case 'l':
    OnCommand(CMD_VIEW_LINKEDIT);
    break;
  case 'z':                       // mesh: wireoptions
    OnCommand(CMD_VIEW_FREECAMERA);
    break;
  case 'f':                       // mesh: wireoptions
    OnCommand(CMD_VIEW_WIREOPT);
    break;
  case 'g':                       // mesh: game
    OnCommand(CMD_VIEW_GAME);
    break;
  case 'o':                       // mesh: ortho
    OnCommand(CMD_VIEW_ORTHO);
    break;
  case 'p':                       // mesh: grid (plane)
    OnCommand(CMD_VIEW_GRID);
    break;
  case 'h':
    OnCommand(CMD_VIEW_ORIGSIZE);
    break;
  case 'n':                       // mesh: show normals
    OnCommand(CMD_VIEW_NORMALS);
    break;
  case 'y':
    OnCommand(CMD_VIEW_TOGGLECAMSPEED2);
    break;
  case 'Y'|sKEYQ_SHIFT:
    OnCommand(CMD_VIEW_TOGGLECAMSPEED3);
    break;
  case 'y'|sKEYQ_ALT:
    OnCommand(CMD_VIEW_TOGGLECAMSPEED0);
    break;

  case 'k':                       // mesh: blob camera
    OnCommand(CMD_VIEW_CAM2KEY);
    break;
  case 'K'|sKEYQ_SHIFT:           // mesh: blob camera
    OnCommand(CMD_VIEW_KEY2CAM);
    break;
  case '+':                       // mesh: blob camera
    OnCommand(CMD_VIEW_NEXTCAM);
    break;
  case '-':                       // mesh: blob camera
    OnCommand(CMD_VIEW_PREVCAM);
    break;
    
    
    /*
  case 'q':   
    QuantMode = !QuantMode;       // mesh: quant SRT
    break;
*/
  case 't':                       // bitmap: tile 
    if(DragCID==KC_BITMAP)
    {
      OnCommand(CMD_VIEW_TILE);
    }
    else if(DragCID==KC_SCENE)
    {
      OnCommand(CMD_VIEW_90DEGREE);
    }
    else if(DragCID==KC_IPP || DragCID==KC_DEMO)
    {
      OnCommand(CMD_VIEW_SCRATCHTIME1);
    }
    break;
  case 't'|sKEYQ_BREAK: 
    if(DragCID==KC_IPP || DragCID==KC_DEMO)
      OnCommand(CMD_VIEW_SCRATCHTIME0);
    break;
  case 'a':                       // bitmap: alpha
    if(DragCID==KC_BITMAP)
      OnCommand(CMD_VIEW_ALPHA);
    break;

//  case 'y':
//    OnCommand(CMD_VIEW_SHADOWTOG);
//    break;


  case '1'|sKEYQ_CTRL:            // set camera
  case '2'|sKEYQ_CTRL:
  case '3'|sKEYQ_CTRL:
  case '4'|sKEYQ_CTRL:
    if(!GameMode)
      OnCommand(CMD_VIEW_SETCAM+(key&15));
    break;

  case '1':                       // get camera
  case '2':
  case '3':
  case '4':
    if(DragCID==KC_MATERIAL)
      OnCommand(CMD_VIEW_SETMATMESH+(key&15));
    else if(!GameMode)
      OnCommand(CMD_VIEW_GETCAM+(key&15));
    break;

  case sKEY_WHEELUP:
    Cam.Pivot = sRange<sF32>(Cam.Pivot*1.125f,1024,0.125f);
    ShowPivotVecTimer = 250;
    CamMatrix.InitSRT(Cam.State);
    PivotVec = CamMatrix.l;
    PivotVec.AddScale3(CamMatrix.k,Cam.Pivot);
    break;
  case sKEY_WHEELDOWN:
    Cam.Pivot = sRange<sF32>(Cam.Pivot/1.125f,1024,0.125f);
    ShowPivotVecTimer = 250;
    CamMatrix.InitSRT(Cam.State);
    PivotVec = CamMatrix.l;
    PivotVec.AddScale3(CamMatrix.k,Cam.Pivot);
    break;

    /*
  case '0'|sKEYQ_CTRL:
  case '1'|sKEYQ_CTRL:
  case '2'|sKEYQ_CTRL:
  case '3'|sKEYQ_CTRL:
  case '4'|sKEYQ_CTRL:
    OnCommand(CMD_VIEW_LIGHT+(key&15));
    break;

  case '0':
  case '1':
  case '2':
  case '3':
    OnCommand(CMD_VIEW_USE+(key&15));
    break;
*/
  }

  // quake cam

  if(!((DragCID==KC_MESH || DragCID == KC_MINMESH || DragCID==KC_SCENE || DragCID==KC_IPP || DragCID==KC_DEMO || DragCID==KC_SMESH) && GameMode && Game.OnKey(key))) 
  if(DragCID!=KC_BITMAP) 
  switch(key&(0x8001ffff))
  {
  case 'w':
  case 'W':
    QuakeMask |= 1;
    break;
  case 's':
  case 'S':
    QuakeMask |= 2;
    break;
  case 'd':
  case 'D':
    QuakeMask |= 4;
    break;
  case 'a':
  case 'A':
    QuakeMask |= 8;
    break;
  case 'q':
  case 'Q':
    QuakeMask |= 16;
    break;
  case 'e':
  case 'E':
    QuakeMask |= 32;
    break;
  case 'w'|sKEYQ_BREAK:
  case 'W'|sKEYQ_BREAK:
    QuakeMask &= ~1;
    break;
  case 's'|sKEYQ_BREAK:
  case 'S'|sKEYQ_BREAK:
    QuakeMask &= ~2;
    break;
  case 'd'|sKEYQ_BREAK:
  case 'D'|sKEYQ_BREAK:
    QuakeMask &= ~4;
    break;
  case 'a'|sKEYQ_BREAK:
  case 'A'|sKEYQ_BREAK:
    QuakeMask &= ~8;
    break;
  case 'q'|sKEYQ_BREAK:
  case 'Q'|sKEYQ_BREAK:
    QuakeMask &= ~16;
    break;
  case 'e'|sKEYQ_BREAK:
  case 'E'|sKEYQ_BREAK:
    QuakeMask &= ~32;
    break;
  }
}

/****************************************************************************/

void WinView::OnDrag(sDragData &dd)
{
  sInt mode;
  sU32 qual;
  sVector v;
  sF32 *rp,*tp;
//  sMatrix mat;
  sF32 CamSpeedF = CamSpeedTable[CamSpeed];

  switch(dd.Mode)
  {
  case sDD_START:
    DragMode = 0;
    DragEditOp = 0;
    DragEditPtrX = 0;
    DragEditPtrY = 0;

    if(ScratchTimeMode)
    {
      DragMode = DM_SCRATCHTIME;
      sInt dummy;
      App->MusicNow(DragStartX,dummy);
      if(App->MusicIsPlaying()) App->MusicStart(2);
      break;
    }

    mode = 0;
    if(dd.Buttons& 1) mode = 1;   // lmb
    if(dd.Buttons& 2) mode = 2;   // rmb
    if(dd.Buttons& 4) mode = 3;   // mmb
    if(dd.Buttons& 8) mode = 4;   // xmb
    if(dd.Buttons&16) mode = 5;   // ymb
    qual = sSystem->GetKeyboardShiftState();
    if(qual&sKEYQ_CTRL)
      mode |= 16;
    else if(qual&sKEYQ_ALT)
      mode |= 32;
    else if(qual&sKEYQ_SHIFT)
      mode |= 64;


    if(DragCID == KC_BITMAP)
    {
      switch(mode&15)
      {
      case 1:
        DragMode = DMB_LIGHT;
        DragStartX = LightPosX;
        DragStartY = LightPosY;
        break;
      case 2:
        DragMode = DMB_ZOOM;
        DragStartY = BitmapZoom;
        break;
      default:
        DragMode = DMB_SCROLL;
        DragStartX = BitmapX-dd.MouseX;
        DragStartY = BitmapY-dd.MouseY;
        break;
      }
    }
    if(DragCID == KC_SCENE || DragCID == KC_MESH || DragCID == KC_MINMESH || DragCID == KC_IPP || DragCID == KC_SMESH || DragCID == KC_DEMO || DragCID == KC_MATERIAL)
    {
      if((dd.Buttons&2) && dd.Flags&sDDF_DOUBLE) 
      { 
        sGui->Post(CMD_VIEW_WIRE,this); 
        break; 
      }

      switch(mode)
      {
      case 0x01:
        DragMode = DM_ROTATE;
				sCopyMem(CamStateBase,Cam.State,9*sizeof(sF32));
        break;
      case 0x11:
        DragMode = DM_ZOOM;
        DragStartFY = Cam.Zoom[Cam.Ortho];
        break;
      case 0x21:
        DragMode = DM_PIVOT;
        DragStartFY = Cam.Pivot;
	  		CamMatrix.InitSRT(Cam.State);
        break;

      case 0x02:
      case 0x22:
        DragMode = DM_ORBIT;
        DragStartY = 0;
        v.Init(Cam.State[6],Cam.State[7],Cam.State[8]);
        v.Sub3(PivotVec);
        v.Neg3();
        Cam.Pivot = v.Abs3();
        //mat.InitDir(v);
        //mat.FindEuler(Cam.State[3],Cam.State[4],Cam.State[5]);
        Cam.State[3] = sFATan2(-v.y,sFSqrt(v.x*v.x+v.z*v.z));
        Cam.State[4] = sFATan2(v.x,v.z);
        Cam.State[3] *= 1/sPI2F;  
        Cam.State[4] *= 1/sPI2F;
        Cam.State[5] = 0;//1/sPI2F;
				sCopyMem(CamStateBase,Cam.State,9*sizeof(sF32));
	  		CamMatrix.InitSRT(Cam.State);
        break;
      case 0x04:
      case 0x12:
        DragMode = DM_DOLLY;
        DragVec = CamMatrix.l;
        break;

      case 0x03:
      case 0x13:
      case 0x23:
        DragMode = DM_SCROLL;
        DragVec = CamMatrix.l;
        break;

      case 0x41:
      case 0x42:
      case 0x43:
        DragEditOp = App->ParaWin->Op;
        DragEditPtrX = 0;
        DragEditPtrY = 0;
        tp = rp = 0;
        if(DragEditOp) switch(DragEditOp->Class->Id)
        {
        case 0x81: 
          tp = DragEditOp->Op.GetEditPtrF(10); 
          break;  // cube
        case 0x86: 
          tp = DragEditOp->Op.GetEditPtrF( 2); 
          break;  // select cube
        case 0x88: 
          rp = DragEditOp->Op.GetEditPtrF( 4); 
          tp = DragEditOp->Op.GetEditPtrF( 7); 
          break;  // transform
        case 0x89: 
          rp = DragEditOp->Op.GetEditPtrF( 4); 
          tp = DragEditOp->Op.GetEditPtrF( 7); 
          break;  // transform ex
        case 0x95: 
          rp = DragEditOp->Op.GetEditPtrF( 3); 
          tp = DragEditOp->Op.GetEditPtrF( 6); 
          break;  // multiply
        case 0x9c: 
          tp = DragEditOp->Op.GetEditPtrF( 0); 
          break;  // collision cube
        case 0xa4: 
          tp = DragEditOp->Op.GetEditPtrF( 1); 
          break;  // set pivot
        case 0xa5: 
          rp = DragEditOp->Op.GetEditPtrF( 4); 
          tp = DragEditOp->Op.GetEditPtrF( 7); 
          break;  // cubic projection

        case 0xc0: 
          rp = DragEditOp->Op.GetEditPtrF( 3); 
          tp = DragEditOp->Op.GetEditPtrF( 6); 
          break;  // scene
        case 0xc2: 
          rp = DragEditOp->Op.GetEditPtrF( 3); 
          tp = DragEditOp->Op.GetEditPtrF( 6); 
          break;  // multiply
        case 0xc3: 
          rp = DragEditOp->Op.GetEditPtrF( 3); 
          tp = DragEditOp->Op.GetEditPtrF( 6); 
          break;  // transform
        case 0xc4: 
          tp = DragEditOp->Op.GetEditPtrF( 3); 
          break;  // light

        default:   
          DragEditOp = 0; 
          break;
        }
        switch(mode)
        {
        case 0x41:
          if(tp)
          {
            DragEditPtrX = tp+0;
            DragEditPtrY = tp+2;
            DragMode = DM_EDITT;
          }
          break;
        case 0x42:
          if(rp)
          {
            DragEditPtrX = rp+1;
            DragEditPtrY = rp+0;
            DragMode = DM_EDITR;
          }
          break;
        case 0x43:
          if(tp)
          {
            DragEditPtrY = tp+1;
            DragMode = DM_EDITT;
          }
          break;
        }
        if(DragEditPtrX) DragStartFX = *DragEditPtrX;
        if(DragEditPtrY) DragStartFY = *DragEditPtrY;
        break;

      case 0x05:
        sGui->Post(CMD_VIEW_WIRE,this);
        break;
      }
    }
    break;
  case sDD_DRAG:
    switch(DragMode)
    {
    case DMB_SCROLL:   // scroll bitmap
      BitmapX = DragStartX + dd.MouseX;
      BitmapY = DragStartY + dd.MouseY;
      break;
    case DMB_ZOOM:   // zoom bitmap
      BitmapZoom = sRange<sInt>(DragStartY - (-dd.DeltaX-dd.DeltaY)*0.02,8,0);
      break;
    case DMB_LIGHT:  // orbit light
      LightPosX = DragStartX + dd.DeltaX;
      LightPosY = DragStartY + dd.DeltaY;
      break;
    case DM_ORBIT:   // orbit 3d
			Cam.State[3]=CamStateBase[3]+(dd.DeltaY-DragStartY)*0.004f;
      if(Cam.State[3]>0.2499f)
      {
        Cam.State[3]=CamStateBase[3]=0.2499f;
        DragStartY = dd.DeltaY;
      }
      if(Cam.State[3]<-0.2499f)
      {
        Cam.State[3]=CamStateBase[3]=-0.2499f;
        DragStartY = dd.DeltaY;
      }
			Cam.State[4]=CamStateBase[4]+dd.DeltaX*0.004f;
			CamMatrix.InitSRT(Cam.State);
      ShowPivotVecTimer = 1;
      break;
    case DM_DOLLY:   // pan 3d
      CamMatrix.l = DragVec;
      CamMatrix.l.AddScale3(CamMatrix.k,(dd.DeltaY-dd.DeltaX)*0.02f*CamSpeedF);
      Cam.State[6] = CamMatrix.l.x;
      Cam.State[7] = CamMatrix.l.y;
      Cam.State[8] = CamMatrix.l.z;
      break;
    case DM_SCROLL:   // scroll 3d
      CamMatrix.l = DragVec;
      CamMatrix.l.AddScale3(CamMatrix.i,-dd.DeltaX*0.02f*CamSpeedF);
      CamMatrix.l.AddScale3(CamMatrix.j, dd.DeltaY*0.02f*CamSpeedF);
      Cam.State[6] = CamMatrix.l.x;
      Cam.State[7] = CamMatrix.l.y;
      Cam.State[8] = CamMatrix.l.z;
      break;
    case DM_ZOOM:   // zoom 3d
      if(Cam.Ortho==0)
        Cam.Zoom[0] = sRange<sF32>(DragStartFY + (dd.DeltaY-dd.DeltaX)*0.2f,256,1);
      else
        Cam.Zoom[1] = sRange<sF32>(DragStartFY + (dd.DeltaY-dd.DeltaX)*2.0f,4096,1);
      break;
    case DM_ROTATE:   // rotate 3d
			Cam.State[3]=CamStateBase[3]+dd.DeltaY*0.002f;
			Cam.State[4]=CamStateBase[4]+dd.DeltaX*0.002f;
			CamMatrix.InitSRT(Cam.State);
      break;
    case DM_PIVOT:
      Cam.Pivot = sRange<sF32>(DragStartFY + dd.DeltaY*0.125f*CamSpeedF,128,0.125f);
      ShowPivotVecTimer = 1;
      CamMatrix.InitSRT(Cam.State);
      PivotVec = CamMatrix.l;
      PivotVec.AddScale3(CamMatrix.k,Cam.Pivot);
      break;
    case DM_EDITT:
      if(DragEditOp)
      {
        if(QuantMode)        
        {
          if(DragEditPtrX)
            *DragEditPtrX = DragStartFX + (((dd.DeltaX+16*1024)/16)-1024);
          if(DragEditPtrY)
            *DragEditPtrY = DragStartFY - (((dd.DeltaY+16*1024)/16)-1024);
        }
        else
        {
          if(DragEditPtrX)
            *DragEditPtrX = DragStartFX + (dd.DeltaX/16.0f);
          if(DragEditPtrY)
            *DragEditPtrY = DragStartFY - (dd.DeltaY/16.0f);
        }
        DragEditOp->Change(0);
      }
      break;
    case DM_EDITR:
      if(DragEditOp)
      {
        if(QuantMode)        
        {
          if(DragEditPtrX)
            *DragEditPtrX = DragStartFX - (((dd.DeltaX+16*1024)/16)-1024)/24.0f;
          if(DragEditPtrY)
            *DragEditPtrY = DragStartFY - (((dd.DeltaY+16*1024)/16)-1024)/24.0f;
        }
        else
        {
          if(DragEditPtrX)
            *DragEditPtrX = DragStartFX - (dd.DeltaX/16.0f);
          if(DragEditPtrY)
            *DragEditPtrY = DragStartFY - (dd.DeltaY/16.0f);
        }
        DragEditOp->Change(0);
      }
      break;

    case DM_SCRATCHTIME:
      {
        sInt x=0;
        if(dd.Buttons&2)
          x = DragStartX + dd.DeltaX*0x10000;
        else if(dd.Buttons&1)
          x = DragStartX + dd.DeltaX*0x100;
        if(x<0)
          x = 0;
        App->MusicSeek(x);
      }
      break;
    }
    break;
  case sDD_STOP:
    DragMode = 0;
    if(DragMode==DM_SCRATCHTIME)
    {
      if(App->MusicIsPlaying()) App->MusicStart(1);
    }
    break;
  }
}

/****************************************************************************/

sBool WinView::OnCommand(sU32 cmd)
{
  sMenuFrame *mf;
  sControl *con;
  sControlTemplate temp;

  switch(cmd)
  {
  case CMD_VIEW_POPUP:
    mf = new sMenuFrame;
    switch(DragCID)
    {
    case KC_SCENE:
      mf->AddMenu("Color",CMD_VIEW_COLOR,'c');
      mf->AddMenu("Mask",CMD_VIEW_SELECTION,'v');
      mf->AddMenu("Reset",CMD_VIEW_RESET,'r');
      mf->AddMenu("Game",CMD_VIEW_GAME,'g');
      mf->AddMenu("Orthogonal",CMD_VIEW_ORTHO,'o');
      mf->AddMenu("Original Size",CMD_VIEW_ORIGSIZE,'h');
      mf->AddMenu("Grid",CMD_VIEW_GRID,'p');
      mf->AddMenu("Wireframe Options",CMD_VIEW_WIREOPT,'f');
      mf->AddMenu("fix to 90°",CMD_VIEW_90DEGREE,'t');
//      mf->AddMenu("Shadow/Light",CMD_VIEW_SHADOWTOG,'y');
      mf->AddSpacer();
      mf->AddCheck("Wireframe",CMD_VIEW_WIRE,'^',SceneWire);
      mf->AddCheck("Lights",CMD_VIEW_SHOWLIGHTS,'L',ShowLights);
      mf->AddCheck("Quantise SRT",CMD_VIEW_QUANT,0,QuantMode);
      mf->AddCheck("Link Edit",CMD_VIEW_LINKEDIT,'l',GenOverlayManager->LinkEdit);
      mf->AddSpacer();
      mf->AddMenu("cam -> key",CMD_VIEW_CAM2KEY,'k');
      mf->AddMenu("key -> cam",CMD_VIEW_CAM2KEY,'K'|sKEYQ_SHIFT);
      mf->AddMenu("next cam",CMD_VIEW_NEXTCAM,'+');
      mf->AddMenu("prev cam",CMD_VIEW_PREVCAM,'-');
      mf->AddCheck("Free Camera",CMD_VIEW_FREECAMERA,'z',GenOverlayManager->FreeCamera);
      mf->AddCheck("Force Camera Light",CMD_VIEW_FORCECAMLIGHT,'C'|sKEYQ_SHIFT,ForceCameraLight);
/*
      mf->AddMenu("Light 1",CMD_VIEW_LIGHT+1,'1'|sKEYQ_CTRL);
      mf->AddMenu("Light 2",CMD_VIEW_LIGHT+2,'2'|sKEYQ_CTRL);
      mf->AddMenu("Light 3",CMD_VIEW_LIGHT+3,'3'|sKEYQ_CTRL);
      mf->AddMenu("Light 4",CMD_VIEW_LIGHT+4,'4'|sKEYQ_CTRL);
      mf->AddMenu("All Lights",CMD_VIEW_LIGHT+0,'0'|sKEYQ_CTRL);
      mf->AddSpacer();
      mf->AddMenu("Shadow Pass",CMD_VIEW_USE+1,'1');
      mf->AddMenu("Ambient Pass",CMD_VIEW_USE+2,'2');
      mf->AddMenu("Light Pass",CMD_VIEW_USE+3,'3');
      mf->AddMenu("Aux Pass",CMD_VIEW_USE+4,'4');
      mf->AddMenu("All Passes",CMD_VIEW_USE+0,'0');
*/
      break;
    case KC_MESH:
    case KC_MINMESH:
      mf->AddMenu("Color",CMD_VIEW_COLOR,'c');
      mf->AddMenu("Mask",CMD_VIEW_SELECTION,'v');
      mf->AddMenu("Reset",CMD_VIEW_RESET,'r');
      mf->AddMenu("Game",CMD_VIEW_GAME,'g');
      mf->AddMenu("Orthogonal",CMD_VIEW_ORTHO,'o');
      mf->AddMenu("Original Size",CMD_VIEW_ORIGSIZE,'h');
      mf->AddMenu("Grid",CMD_VIEW_GRID,'p');
      mf->AddMenu("fix to 90°",CMD_VIEW_90DEGREE,'t');
//      mf->AddMenu("Shadow/Light",CMD_VIEW_SHADOWTOG,'y');
      mf->AddSpacer();
      mf->AddCheck("Wireframe",CMD_VIEW_WIRE,'^',MeshWire);
      mf->AddCheck("Quantise SRT",CMD_VIEW_QUANT,0,QuantMode);
      mf->AddSpacer();
      mf->AddMenu("cam -> key",CMD_VIEW_CAM2KEY,'k');
      mf->AddMenu("key -> cam",CMD_VIEW_KEY2CAM,'K'|sKEYQ_SHIFT);
      mf->AddMenu("next cam",CMD_VIEW_NEXTCAM,'+');
      mf->AddMenu("prev cam",CMD_VIEW_PREVCAM,'-');
      mf->AddCheck("Free Camera",CMD_VIEW_FREECAMERA,'z',GenOverlayManager->FreeCamera);
      mf->AddCheck("Show Normals",CMD_VIEW_NORMALS,'n',ShowNormals);
      break;
    case KC_BITMAP:
      mf->AddMenu("Reset",CMD_VIEW_RESET,'r');
      mf->AddSpacer();
      mf->AddCheck("Tile",CMD_VIEW_TILE,'t',BitmapTile);
      mf->AddCheck("Alpha",CMD_VIEW_ALPHA,'a',BitmapAlpha);
      break;
    case KC_IPP:
    case KC_DEMO:
      mf->AddMenu("Reset",CMD_VIEW_RESET,'r');
      mf->AddMenu("Game",CMD_VIEW_GAME,'g');
      mf->AddMenu("Orthogonal",CMD_VIEW_ORTHO,'o');
      mf->AddMenu("Original Size",CMD_VIEW_ORIGSIZE,'h');
      mf->AddMenu("Grid",CMD_VIEW_GRID,'p');
      mf->AddCheck("Link Edit",CMD_VIEW_LINKEDIT,'l',GenOverlayManager->LinkEdit);
      mf->AddMenu("scratch time",CMD_VIEW_SCRATCHTIME1,'t');
//      mf->AddMenu("Shadow/Light",CMD_VIEW_SHADOWTOG,'y');
      mf->AddSpacer();
      mf->AddMenu("cam -> key",CMD_VIEW_CAM2KEY,'k');
      mf->AddMenu("key -> cam",CMD_VIEW_KEY2CAM,'K'|sKEYQ_SHIFT);
      mf->AddMenu("next cam",CMD_VIEW_NEXTCAM,'+');
      mf->AddMenu("prev cam",CMD_VIEW_PREVCAM,'-');
      mf->AddCheck("Free Camera",CMD_VIEW_FREECAMERA,'z',GenOverlayManager->FreeCamera);
      break;
    case KC_MATERIAL:
      mf->AddMenu("Reset",CMD_VIEW_RESET,'r');
      mf->AddSpacer();
      mf->AddCheck("Cube",CMD_VIEW_SETMATMESH+1,'1',(MatMeshType == 0));
      mf->AddCheck("Torus",CMD_VIEW_SETMATMESH+2,'2',(MatMeshType == 1));
      break;
    default:
      mf=0;
      break;
    }
    if(mf)
    {
      mf->AddBorder(new sNiceBorder);
      mf->SendTo = this;
      sGui->AddPopup(mf);
    }
    return sTRUE;

  case CMD_VIEW_RESET:
    BitmapZoom = 5;
    BitmapX = 10;
    BitmapY = 10;
    BitmapTile = 0;
    BitmapAlpha = 0;
    MeshWire = 0;
    SceneWire = 0;
    SceneLight = -1;
    SceneUsage = -1;
    ShowXZPlane = 0;
//    ShowOrigSize = 0;
    QuantMode = 1;
    PivotVec.Init();
    Quant90Degree = 0;

		Cam.State[0]=Cam.State[1]=Cam.State[2]=1.0f;
		Cam.State[3]=Cam.State[4]=Cam.State[5]=0.0f;
		Cam.State[6]=Cam.State[7]=0.0f; Cam.State[8]=-5.0f;
		CamMatrix.InitSRT(Cam.State);
    Cam.Ortho = 0;
    Cam.Pivot = 5.0f;
    Cam.Zoom[0] = 128;
    Cam.Zoom[1] = 1024;
    return sTRUE;
  case CMD_VIEW_WIRE:
    if(DragCID==KC_SCENE)
    {
      SceneWire = !SceneWire;
      UpdateSceneWire();
    }
    else if(DragCID==KC_MESH)
    {
      MeshWire = !MeshWire;
      if(!MeshWire)
        FlushMeshWire();
    }
    else if(DragCID==KC_MINMESH)
    {
      MeshWire = !MeshWire;
    }
    return sTRUE;

  case CMD_VIEW_SHOWLIGHTS:
    ShowLights = !ShowLights;
    return sTRUE;

  case CMD_VIEW_LINKEDIT:
    GenOverlayManager->LinkEdit = !GenOverlayManager->LinkEdit;
    GenOverlayManager->FreeCamera = 0;
    if(GenOverlayManager->LinkEdit)
      App->SplineWin->LinkEdit(Cam.State,1);
    return sTRUE;

  case CMD_VIEW_FREECAMERA:
    GenOverlayManager->FreeCamera = !GenOverlayManager->FreeCamera;
    GenOverlayManager->LinkEdit = 0;
    return sTRUE;

  case CMD_VIEW_NORMALS:
    ShowNormals = !ShowNormals;
    return sTRUE;

  case CMD_VIEW_FORCECAMLIGHT:
    ForceCameraLight = !ForceCameraLight;
    return sTRUE;

  case CMD_VIEW_QUANT:
    QuantMode = !QuantMode;
    return sTRUE;

  case CMD_VIEW_COLOR:
    con = new sControl;
    con->EditURGB(0,(sInt *)&WireColor[EWC_BACKGROUND],0);
    con->Style |= sCS_NOBORDER|sCS_HOVER;
    con->SizeX = 120;
    mf = new sMenuFrame;
    mf->ExitKey = 'c';
    mf->SendTo = this;
    mf->AddBorder(new sNiceBorder);
    mf->AddChild(con);
    sGui->AddPopup(mf);
    return sTRUE;

  case CMD_VIEW_TOGGLECAMSPEED2:
    if(CamSpeed!=2)
      CamSpeed = 2;
    else
      CamSpeed = 1;
    return sTRUE;
  case CMD_VIEW_TOGGLECAMSPEED0:
    if(CamSpeed!=0)
      CamSpeed = 0;
    else
      CamSpeed = 1;
    return sTRUE;
  case CMD_VIEW_TOGGLECAMSPEED3:
    if(CamSpeed!=3)
      CamSpeed = 3;
    else
      CamSpeed = 1;
    return sTRUE;

  case CMD_VIEW_SELECTION:
    mf = new sMenuFrame;
    mf->ExitKey = 'v';
    mf->SendTo = this;
    mf->AddBorder(new sNiceBorder);
    con = new sControl;
    con->EditMask(0,&SelEdge,0,8,"eeeeeeee");
    con->SizeX = 120;
    mf->AddChild(con);
    con = new sControl;
    con->EditMask(0,&SelVertex,0,8,"vvvvvvvv");
    con->SizeX = 120;
    mf->AddChild(con);
    con = new sControl;
    con->EditMask(0,&SelFace,0,8,"ffffffff");
    con->SizeX = 120;
    mf->AddChild(con);
    sGui->AddPopup(mf);
    return sTRUE;

  case CMD_VIEW_WIREOPT:
    mf = new sMenuFrame;
    mf->ExitKey = 'f';
    mf->SendTo = this;
    mf->AddBorder(new sNiceBorder);
    temp.Init();
    temp.Type = sCT_CYCLE;
    temp.Cycle = "|Vertices:*1|Edges:*2|Faces:*6|Hidden:*3|Collision:*4|Solid Portals:*5|Line Portals";
    temp.AddFlags(mf,CMD_VIEW_WIREOPT2,&WireOptions,0);
    sGui->AddPopup(mf);
    return sTRUE;

  case CMD_VIEW_WIREOPT2:
    return sTRUE;

  case CMD_VIEW_LIGHT+0:
  case CMD_VIEW_LIGHT+1:
  case CMD_VIEW_LIGHT+2:
  case CMD_VIEW_LIGHT+3:
  case CMD_VIEW_LIGHT+4:
    if(DragCID==KC_SCENE)
      SceneLight = ((sInt)(cmd&15))-1;
    return sTRUE;

  case CMD_VIEW_USE+0:
  case CMD_VIEW_USE+1:
  case CMD_VIEW_USE+2:
  case CMD_VIEW_USE+3:
  case CMD_VIEW_USE+4:
    if(DragCID==KC_SCENE)
      SceneUsage = (cmd&15)-1;
    return sTRUE;

  case CMD_VIEW_SETCAM+0:
  case CMD_VIEW_SETCAM+1:
  case CMD_VIEW_SETCAM+2:
  case CMD_VIEW_SETCAM+3:
  case CMD_VIEW_SETCAM+4:
  case CMD_VIEW_SETCAM+5:
  case CMD_VIEW_SETCAM+6:
  case CMD_VIEW_SETCAM+7:
    CamSave[cmd&15] = Cam;
    return sTRUE;

  case CMD_VIEW_GETCAM+0:
  case CMD_VIEW_GETCAM+1:
  case CMD_VIEW_GETCAM+2:
  case CMD_VIEW_GETCAM+3:
  case CMD_VIEW_GETCAM+4:
  case CMD_VIEW_GETCAM+5:
  case CMD_VIEW_GETCAM+6:
  case CMD_VIEW_GETCAM+7:
    Cam = CamSave[cmd&15];
    return sTRUE;

  case CMD_VIEW_SETMATMESH+0:
  case CMD_VIEW_SETMATMESH+1:
  case CMD_VIEW_SETMATMESH+2:
  case CMD_VIEW_SETMATMESH+3:
  case CMD_VIEW_SETMATMESH+4:
  case CMD_VIEW_SETMATMESH+5:
  case CMD_VIEW_SETMATMESH+6:
  case CMD_VIEW_SETMATMESH+7:
    {
      sInt ind = (cmd&15) - 1;
      if(ind >= 0 && ind < 2 && ind != MatMeshType)
      {
        MatMeshType = ind;
        if(MatMesh)
        {
          MatMesh->Release();
          MatMesh = 0;
        }
      }
    }
    return sTRUE;

  case CMD_VIEW_TILE:
    if(DragCID==KC_BITMAP)
      BitmapTile = !BitmapTile;
    return sTRUE;
  case CMD_VIEW_ALPHA:
    if(DragCID==KC_BITMAP)
      BitmapAlpha = !BitmapAlpha;
    return sTRUE;
  case CMD_VIEW_ORTHO:
    if(DragCID==KC_MESH || DragCID == KC_MINMESH || DragCID==KC_SCENE || DragCID==KC_IPP || DragCID==KC_DEMO)
      Cam.Ortho = !Cam.Ortho;
    return sTRUE;
  case CMD_VIEW_GRID:
    if(DragCID==KC_MESH || DragCID == KC_MINMESH || DragCID==KC_SCENE || DragCID==KC_IPP || DragCID==KC_DEMO)
      ShowXZPlane = !ShowXZPlane;
    return sTRUE;
  case CMD_VIEW_SCRATCHTIME1:
    if(DragCID==KC_IPP || DragCID==KC_DEMO)
      ScratchTimeMode = 1;
    return sTRUE;
  case CMD_VIEW_SCRATCHTIME0:
    if(DragCID==KC_IPP || DragCID==KC_DEMO)
      ScratchTimeMode = 0;
    return sTRUE;
  case CMD_VIEW_ORIGSIZE:
    if(DragCID==KC_MESH || DragCID == KC_MINMESH || DragCID==KC_SCENE || DragCID==KC_IPP || DragCID==KC_DEMO)
      ShowOrigSize = !ShowOrigSize;
    return sTRUE;
  case CMD_VIEW_90DEGREE:
    Quant90Degree = 1;
    return sTRUE;
//  case CMD_VIEW_SHADOWTOG:
//    GenOverlayManager->EnableShadows = (GenOverlayManager->EnableShadows+1)%3;
//    return sTRUE;
  case CMD_VIEW_GAME:
    if(DragCID==KC_MESH || DragCID==KC_SCENE || DragCID==KC_IPP || DragCID==KC_DEMO)
    {
      if(GameMode)
        LeaveGameMode();
      else
      {
        App->Doc->ClearInstanceMem();
        EnterGameMode();
      }
    }
    return sTRUE;


  case CMD_VIEW_KEY2CAM:
    {
      BlobSpline *bs = App->ParaWin->GetBlobSpline();
      if(bs && bs->Select>=0 && bs->Select<bs->Count)
      {
        BlobSplineKey *key = bs->Keys+bs->Select;
        Cam.State[6] = key->px;
        Cam.State[7] = key->py;
        Cam.State[8] = key->pz;
        Cam.State[3] = key->rx;
        Cam.State[4] = key->ry;
        Cam.State[5] = key->rz;
      }
    }
    return sTRUE;

  case CMD_VIEW_CAM2KEY:
    {
      BlobSpline *bs = App->ParaWin->GetBlobSpline();
      if(bs && bs->Select>=0 && bs->Select<bs->Count)
      {
        BlobSplineKey *key = bs->Keys+bs->Select;
        key->px = Cam.State[6];
        key->py = Cam.State[7];
        key->pz = Cam.State[8];
        key->rx = Cam.State[3];
        key->ry = Cam.State[4];
        key->rz = Cam.State[5];
        App->ParaWin->Op->Change(-1);
      }
    }
    return sTRUE;

  case CMD_VIEW_NEXTCAM:
    {
      BlobSpline *bs = App->ParaWin->GetBlobSpline();
      if(bs)
      {
        if(bs->Select<0)
          bs->SetSelect(0);
        else if(bs->Select+1<bs->Count)
          bs->SetSelect(bs->Select+1);
      }
    }
    return sTRUE;

  case CMD_VIEW_PREVCAM:
    {
      BlobSpline *bs = App->ParaWin->GetBlobSpline();
      if(bs)
      {
        if(bs->Select<0)
          bs->SetSelect(bs->Count-1);
        else if(bs->Select>0)
          bs->SetSelect(bs->Select-1);
      }
    }
    return sTRUE;

  default:
    return sFALSE;
  }
}

void WinView::OnTick(sInt ticks)
{
}

/****************************************************************************/
/****************************************************************************/

void WinView::MakeCam(sMaterialEnv &env,sViewport &view)
{
  sChar buffer[128];

  env.Init();
  env.CameraSpace = CamMatrix;
  env.Orthogonal = Cam.Ortho;
  env.ZoomX = 128.0f/Cam.Zoom[Cam.Ortho];

  if(GameMode)
  {
    /*max = ThisTime/10-LastTime/10;
    if(max>100) max = 100;
    for(i=0;i<max;i++)
      Game.OnTick(App->Doc->KEnv);*/
    Game.GetCamera(env);
    if(sGui->GetFocus()==this)
      sSystem->SetWinMouse((Client.x0+Client.x1)/2,(Client.y0+Client.y1)/2);

    sSPrintF(buffer,sizeof(buffer),"cell %d",Game.PlayerCell);
    App->SetStat(STAT_VIEW,buffer);
  }

  App->Doc->KEnv->Aspect = 1.0f * view.Window.XSize() / view.Window.YSize();
  env.ZoomY = env.ZoomX * App->Doc->KEnv->Aspect;
}

void WinView::ShowBitmap(sViewport &view,class GenBitmap *tex)
{
  sU32 states[256],*p;
  //sVertexDouble *vp;
  sVertexTSpace3 *vp;
  sMatrix mat;
  sMaterial11 *mtrl;
  sU16 *ip;
  sInt x,y,xs,ys;
  sInt handle;

  if(tex)
  {
    mat.Init();

    p = states;

    sSystem->SetViewport(view);
    sSystem->Clear(sVCF_ALL,sGui->Palette[sGC_BACK] & 0xffffff);

    MtrlTexAlpha->SetTex(0,tex->Texture);
    MtrlTex->SetTex(0,tex->Texture);
    MtrlNormal->SetTex(0,tex->Texture);
    MtrlNormalLight->SetTex(1,tex->Texture);

    if(tex->XSize >= tex->YSize)
    {
      xs = 256 * (1<<BitmapZoom)/16;
      ys = sMulDiv(xs,tex->YSize,tex->XSize);
    }
    else
    {
      ys = 256 * (1<<BitmapZoom)/16;
      xs = sMulDiv(ys,tex->XSize,tex->YSize);
    }

    x = BitmapX;
    y = BitmapY;

    if(tex->Format == sTF_Q8W8V8U8) // normal map
    {
      mtrl = MtrlNormalLight;

      MtrlEnv.LightAmplify = 1.0f;
      MtrlEnv.LightColor.Init(1,1,1,1);

      sInt r = sMax(xs/2,ys/2);
      sVector dir;

      dir.Init(1.0f * LightPosX / r,1.0f * LightPosY / r,0.0f,0.0f);
      dir.z = 1.0 - dir.x * dir.x - dir.y*dir.y;
      dir.z = (dir.z < 0.3f * 0.3f) ? 0.3f : sFSqrt(dir.z);
      dir.Unit3();

      MtrlEnv.LightPos.Init(r*dir.x,r*dir.y,-r*dir.z);
      MtrlEnv.LightRange = r * 32.0f;
      MtrlEnv.CenterX = -1.0f * (x + xs / 2);
      MtrlEnv.CenterY = -1.0f * (y + ys / 2);
      x = -xs / 2;
      y = -ys / 2;
    }
    else
      mtrl = BitmapAlpha ? MtrlTexAlpha : MtrlTex;

    sSystem->SetViewProject(&MtrlEnv);
    mtrl->Set(MtrlEnv);

    handle = sSystem->GeoAdd(sFVF_TSPACE3,sGEO_TRISTRIP|sGEO_DYNAMIC);
    sSystem->GeoBegin(handle,4,4,(sF32 **)&vp,(void **)&ip);    
    *ip++ = 3;
    *ip++ = 2;
    *ip++ = 1;
    *ip++ = 0;
    if(BitmapTile)
    {
      vp->Init(x-xs  ,y-ys  ,0.5f,~0,-1,-1); vp++;
      vp->Init(x+xs*2,y-ys  ,0.5f,~0, 2,-1); vp++;
      vp->Init(x-xs  ,y+ys*2,0.5f,~0,-1, 2); vp++;
      vp->Init(x+xs*2,y+ys*2,0.5f,~0, 2, 2); vp++;
    }
    else
    {
      vp->Init( 0+x, 0+y,0.5f,~0,0,0); vp++;
      vp->Init(xs+x, 0+y,0.5f,~0,1,0); vp++;
      vp->Init( 0+x,ys+y,0.5f,~0,0,1); vp++;
      vp->Init(xs+x,ys+y,0.5f,~0,1,1); vp++;
    }
    sSystem->GeoEnd(handle);
    sSystem->GeoDraw(handle);
    sSystem->GeoRem(handle);

    if(tex->Format == sTF_Q8W8V8U8)
    {
      GenOverlayManager->FXQuad(GENOVER_ADDDESTALPHA);
      MtrlEnv.CenterX = MtrlEnv.CenterY = 0.0f;
    }
  }
}

void WinView::ShowMesh(sViewport &view,GenMesh *mesh,KEnvironment *kenv)
{
  sMaterialEnv env;
  EngMesh *paintMesh;

  env = kenv->GameCam;

  sSystem->SetViewport(view);
  sSystem->Clear(sVCF_ALL,MeshWire ? 0 : WireColor[EWC_BACKGROUND] & 0xffffff);

  sSystem->SetViewProject(&env);
  if(ShowPivotVecTimer)
    ShowHints(1,&env);

  if(MeshWire)
  {
    mesh->PrepareWire(WireOptions,SelEdge | (SelFace << 8) | (SelVertex << 16));
    paintMesh = mesh->WireMesh;
    App->SetStat(STAT_VIEW,"Mesh, Wireframe");
  }
  else
  {
    mesh->Prepare();
    paintMesh = mesh->PreparedMesh;
    App->SetStat(STAT_VIEW,"Mesh, Solid");
  }

  if(paintMesh)
  {
    sMatrix mat;
    mat.Init();

    Engine->SetViewProject(env);
    Engine->StartFrame();
    Engine->AddPaintJob(paintMesh,mat,0.0f);

    // add default light
    EngLight light;
    light.Position = env.CameraSpace.l;
    light.Flags = 0;
    light.Color = 0xffffffff;
    light.Amplify = 1.0f;
    light.Range = GenOverlayManager->AutoLightRange;
    light.Event = 0;
    light.Id = 0;
    Engine->AddLightJob(light);

    Engine->Paint(kenv);
  }

  if(ShowLights)
    ShowMeshLights(mesh,&env);

  ShowHints(2,&env);
}

void WinView::ShowMinMesh(sViewport &view,GenMinMesh *mesh,KEnvironment *kenv)
{
  sMaterialEnv env;
  EngMesh *paintMesh;

  env = kenv->GameCam;

  sSystem->SetViewport(view);
  sSystem->Clear(sVCF_ALL,MeshWire ? 0 : WireColor[EWC_BACKGROUND] & 0xffffff);

  sSystem->SetViewProject(&env);
  if(ShowPivotVecTimer)
    ShowHints(1,&env);
  ShowHints(2,&env);
  
  if(MeshWire)
  {
    mesh->PrepareWire(WireOptions,SelEdge | (SelFace << 8) | (SelVertex << 16));
    paintMesh = mesh->WireMesh;
    App->SetStat(STAT_VIEW,"Mesh, Wireframe");
  }
  else
  {
    mesh->Prepare();
    paintMesh = mesh->PreparedMesh;
    App->SetStat(STAT_VIEW,"Mesh, Solid");
  }

  if(paintMesh)
  {
    sMatrix mat;
    mat.Init();

    Engine->SetViewProject(env);
    Engine->StartFrame();
    Engine->AddPaintJob(paintMesh,mat,0.0f);

    // add default light
    EngLight light;
    light.Position = env.CameraSpace.l;
    light.Flags = 0;
    light.Color = 0xffffffff;
    light.Amplify = 1.0f;
    light.Range = GenOverlayManager->AutoLightRange;
    light.Event = 0;
    light.Id = 0;
    Engine->AddLightJob(light);

    Engine->Paint(kenv);
  }

  if(ShowNormals)
    ShowNormalsAndTangents(mesh,&env);
}

void WinView::ShowScene(sViewport &view,WerkOp *op,KEnvironment *kenv)
{
  GenOverlayManager->Reset(kenv);

  Engine->StartFrame();
  kenv->CurrentCam = kenv->GameCam;
  Engine->SetViewProject(kenv->GameCam);
  sSystem->SetViewport(view);
  sSystem->Clear(sVCF_ALL,WireColor[EWC_BACKGROUND] & 0xffffff);

  ShowPortalOp = App->ParaWin->Op ? &App->ParaWin->Op->Op : 0;
  ShowPortalOpProcessed = sFALSE;
  // ShowPortalJob = 0;
  /*if(!ShowPortalOp || ShowPortalOp->ExecHandler != Exec_Scene_Portal)
    ShowPortalOp = 0;*/

  {
    sZONE(ExecTree);
    op->Op.Exec(kenv);
  }

  Engine->ProcessPortals(kenv,GameMode ? Game.PlayerCell : 0);
  if(!Engine->GetNumLightJobs() || ForceCameraLight)
  {
    // remove all lights so far
    Engine->ClearLightJobs();

    // add default light
    EngLight light;
    light.Position = kenv->GameCam.CameraSpace.l;
    light.Flags = 0;
    light.Color = 0xffffffff;
    light.Amplify = 1.0f;
    light.Range = GenOverlayManager->AutoLightRange;
    light.Event = 0;
    light.Id = 0;
    Engine->AddLightJob(light);
  }

  Engine->Paint(kenv);

  kenv->GameCam.ModelSpace.Init();
  sSystem->SetViewProject(&kenv->GameCam);
  
  if(ShowPivotVecTimer)
    ShowHints(1,&kenv->GameCam);
  ShowHints(2,&kenv->GameCam);

  if(SceneWire)
  {
    if(GameMode)
      Game.OnPaint(&kenv->GameCam);

    ShowDynCell(&kenv->GameCam);
    App->SetStat(STAT_VIEW,"Scene, Wireframe");
  }
  else
    App->SetStat(STAT_VIEW,"Scene, Solid");
}

void WinView::ShowIPP(sViewport &view,WerkOp *op,KEnvironment *kenv)
{
  App->SetStat(STAT_VIEW,"Image Post Processing");
  sVERIFY(op && op->Op.Cache && op->Op.Cache->ClassId == KC_IPP);

  GenOverlayManager->SetMasterViewport(view);
  GenOverlayManager->Reset(kenv);
  GenOverlayManager->RealPaint = sTRUE;
  GenOverlayManager->Game = GameMode ? &Game : 0;

  {
    sZONE(ExecTree);
    op->Op.Exec(kenv);
  }

  if(GenOverlayManager->LastOutput!=0)
  {
    GenOverlayManager->Copy(GenOverlayManager->LastOutput,
      GenOverlayManager->Alloc(0,GENOVER_RTSIZES,1),0,0xffffffff,1);
  }

  GenOverlayManager->RealPaint = sFALSE;
  GenOverlayManager->Reset(kenv);
}

void WinView::ShowDemo(sViewport &view,WerkOp *op,KEnvironment *kenv)
{
  App->SetStat(STAT_VIEW,"Image Post Processing");
  sVERIFY(op && op->Op.Cache && op->Op.Cache->ClassId == KC_DEMO);

  GenOverlayManager->SetMasterViewport(view);
  GenOverlayManager->RealPaint = sTRUE;
  GenOverlayManager->Game = GameMode ? &Game : 0;

  {
    sZONE(ExecTree);
    op->Op.Exec(kenv);
  }

  GenOverlayManager->RealPaint = sFALSE;
  GenOverlayManager->Reset(kenv);
}

void WinView::ShowMaterial(sViewport &view,WerkOp *op,KEnvironment *kenv)
{
  App->SetStat(STAT_VIEW,"Material");
  sVERIFY(op && op->Op.Cache && op->Op.Cache->ClassId == KC_MATERIAL);

  if(!MatMesh)
  {
    MatSrcMesh[MatMeshType]->Mtrl[1].Material->Release();
    MatSrcMesh[MatMeshType]->Mtrl[1].Material = (GenMaterial*) op->Op.Cache;
    MatSrcMesh[MatMeshType]->Mtrl[1].Material->AddRef();

    MatMesh = new EngMesh;
    MatMesh->FromGenMesh(MatSrcMesh[MatMeshType]);
  }

  if(MatMesh)
  {
    GenOverlayManager->Reset(kenv);

    Engine->StartFrame();
    Engine->SetViewProject(kenv->GameCam);
    sSystem->SetViewport(view);
    sSystem->Clear(sVCF_ALL,WireColor[EWC_BACKGROUND] & 0xffffff);

    sMatrix mat;
    mat.Init();
    Engine->AddPaintJob(MatMesh,mat,0.0f);

    // add default light
    EngLight light;
    light.Position = kenv->GameCam.CameraSpace.l;
    light.Flags = 0;
    light.Color = 0xffffffff;
    light.Amplify = 1.0f;
    light.Range = GenOverlayManager->AutoLightRange;
    light.Event = 0;
    light.Id = 0;
    Engine->AddLightJob(light);

    Engine->Paint(kenv);
  }
}

void WinView::ShowSimpleMesh(sViewport &view,GenSimpleMesh *mesh,KEnvironment *kenv)
{
  sInt i,j,ecount,ccount;
  sInt handle;
  sF32 *fp,*edge,*ep;
  sVector lv;
  GenSimpleFace *fc;

  App->SetStat(STAT_VIEW,"BSP");

  sSystem->SetViewport(view);
  sSystem->SetViewProject(&kenv->GameCam);
  MtrlFlatZ->Set(kenv->GameCam);

  // count edges
  ecount = 0;
  for(i=0;i<mesh->Faces.Count;i++)
    ecount += mesh->Faces[i].VertexCount;

  // make edges
  edge = ep = new sF32[ecount*2*4];

  // for each face
  for(i=0;i<mesh->Faces.Count;i++)
  {
    fc = &mesh->Faces[i];

    lv = fc->Vertices[fc->VertexCount - 1];
    for(j=0;j<fc->VertexCount;j++)
    {
      *ep++ = lv.x;
      *ep++ = lv.y;
      *ep++ = lv.z;
      *(sU32 *)ep = 0xc0c0c0; ep++;

      *ep++ = fc->Vertices[j].x;
      *ep++ = fc->Vertices[j].y;
      *ep++ = fc->Vertices[j].z;
      *(sU32 *)ep = 0xc0c0c0; ep++;

      lv = fc->Vertices[j];
    }
  }

  // draw them
  handle = GeoLine;
  ep = edge;

  while(ecount)
  {
    ccount = sMin(ecount,16384);
    sSystem->GeoBegin(handle,ccount*2,0,&fp,0);
    sCopyMem(fp,ep,ccount*2*4*sizeof(sF32));
    sSystem->GeoEnd(handle);
    sSystem->GeoDraw(handle);
    sSystem->GeoFlush(handle);

    ep += ccount*2*4;
    ecount -= ccount;
  }

  delete[] edge;
}

/****************************************************************************/

void WinView::ShowHints(sU32 flags,sMaterialEnv *env)
{
  static const HintLine data[] = 
  {
    { -1,-1,-1, -1,-1, 1, 0xffffff00, },    // 0,12: cube
    { -1,-1, 1, -1, 1, 1, 0xffffff00, },
    { -1, 1, 1, -1, 1,-1, 0xffffff00, },
    { -1, 1,-1, -1,-1,-1, 0xffffff00, },

    {  1,-1,-1,  1,-1, 1, 0xffffff00, },
    {  1,-1, 1,  1, 1, 1, 0xffffff00, },
    {  1, 1, 1,  1, 1,-1, 0xffffff00, },
    {  1, 1,-1,  1,-1,-1, 0xffffff00, },

    { -1,-1, 1,  1,-1, 1, 0xffffff00, },
    { -1, 1, 1,  1, 1, 1, 0xffffff00, },
    { -1, 1,-1,  1, 1,-1, 0xffffff00, },
    { -1,-1,-1,  1,-1,-1, 0xffffff00, },

    { -1, 0, 0,  1, 0, 0, 0xffff0000, },    // 12,3: cross
    {  0,-1, 0,  0, 1, 0, 0xff00ff00, },
    {  0, 0,-1,  0, 0, 1, 0xff0000ff, },

    {  0, 0, 0,  1, 0,-1, 0xffffff00, },    // 15,8: arrow-tip
    {  0, 0, 0,  0, 1,-1, 0xffffff00, },
    {  0, 0, 0, -1, 0,-1, 0xffffff00, },
    {  0, 0, 0,  0,-1,-1, 0xffffff00, },
    {  0, 1,-1,  1, 0,-1, 0xffffff00, }, 
    { -1, 0,-1,  0, 1,-1, 0xffffff00, },
    {  0,-1,-1, -1, 0,-1, 0xffffff00, },
    {  1, 0,-1,  0,-1,-1, 0xffffff00, },

    {  0, 0, 0,  0.25, 0.25,-0.25, 0xffffff00, },    // 23,4: arrow-tip
    {  0, 0, 0,  0.25,-0.25,-0.25, 0xffffff00, },
    {  0, 0, 0, -0.25, 0.25,-0.25, 0xffffff00, },
    {  0, 0, 0, -0.25,-0.25,-0.25, 0xffffff00, },
  };
  static const sInt wireCubeVert[24] =
  {
    0,1, 1,5, 5,4, 4,0, 2,3, 3,7, 7,6, 6,2, 0,2, 1,3, 5,7, 4,6
  };
  static const sU8 cubeTable[6*4] = { 0,2,3,1, 2,6,7,3, 3,7,5,1, 4,0,1,5, 2,0,4,6, 7,6,4,5 };

  sMatrix m,ms;
  sVector v0,v1,v;
  static HintLine hl[256];
  sF32 *vp;
  sInt i,handle;
  sU16 *ip;
  KOp *op;
  GenMesh *msh;
  sU32 col;

  // camera pivot

  if(flags & 1)
  {
    m.Init();
    m.l = PivotVec;
    ShowHintsPart(m,data+12,3,WireColor[EWC_HINT],env);
    m.Init();
    m.l = PivotVec;
    m.l.z += 1;
    ShowHintsPart(m,data+23,4,WireColor[EWC_HINT],env);
  }

  // hint-lines

  if(App->ParaWin->Op && (flags&2))
  {
    op = &App->ParaWin->Op->Op;

    // draw transform pivot (if applicable)
    if(App->ParaWin->Op->Class->Output == KC_MESH)
    {
      msh = (GenMesh *)op->Cache;
      if(msh)
      {
        m.Init();
        m.Scale3(HintSize);
        if(msh->Pivot!=-1)
          m.l = msh->VertBuf[msh->Pivot*msh->VertSize()];
        ShowHintsPart(m,data+12,3,0,env);
      }
    }

    switch(App->ParaWin->Op->Class->Id)
    {
    case 0x0018:                    // spline
    case 0x001a:                    // pipe spline
      if(op->Cache && op->Cache->ClassId==KC_SPLINE)
        ShowBlobSplineHints(((GenSplineSpline *)op->Cache)->Spline,env);
      break;

    case 0x86:                      // selectcube
      m.Init();
      m.i.x = *op->GetEditPtrF(5)*0.5f;
      m.j.y = *op->GetEditPtrF(6)*0.5f;
      m.k.z = *op->GetEditPtrF(7)*0.5f;
      m.l.x = *op->GetEditPtrF(2);
      m.l.y = *op->GetEditPtrF(3);
      m.l.z = *op->GetEditPtrF(4);
      ShowHintsPart(m,data+0,12,WireColor[EWC_HINT],env);

      // cube faces (transparent)
      if(WireOptions & EWF_SOLIDPORTALS)
      {
        MtrlCollision->Set(*env);
        handle = sSystem->GeoAdd(sFVF_COMPACT,sGEO_TRI|sGEO_DYNAMIC);
        sSystem->GeoBegin(handle,8,6*6,&vp,(void **)&ip);
        for(i=0;i<8;i++)
        {
          *vp++ = *op->GetEditPtrF(2) + ((i&1) ? 0.5f : -0.5f) * *op->GetEditPtrF(5);
          *vp++ = *op->GetEditPtrF(3) + ((i&2) ? 0.5f : -0.5f) * *op->GetEditPtrF(6);
          *vp++ = *op->GetEditPtrF(4) + ((i&4) ? 0.5f : -0.5f) * *op->GetEditPtrF(7);
          *(sU32 *) vp = WireColor[EWC_PORTAL]; vp++;
        }
        for(i=0;i<6;i++)
        {
          *ip++ = cubeTable[i*4+0];
          *ip++ = cubeTable[i*4+1];
          *ip++ = cubeTable[i*4+2];
          *ip++ = cubeTable[i*4+0];
          *ip++ = cubeTable[i*4+2];
          *ip++ = cubeTable[i*4+3];
        }
        sSystem->GeoEnd(handle);
        sSystem->GeoDraw(handle);
        sSystem->GeoRem(handle);
      }

      break;
    case 0x88:                      // transform
      m.Init();
      msh = (GenMesh*)op->Cache;
      v1.x = *op->GetEditPtrF(7);
      v1.y = *op->GetEditPtrF(8);
      v1.z = *op->GetEditPtrF(9);
      v1.w = 1;
      if(msh && msh->Pivot!=-1)
      {
        v0 = msh->VertBuf[msh->Pivot*msh->VertSize()];
        v0.Sub3(v1);
        v1.Add3(v0);
      }
      else
        v0.Init(0,0,0,1);
      hl[0].x0 = v0.x;
      hl[0].y0 = v0.y;
      hl[0].z0 = v0.z;
      hl[0].x1 = v1.x;
      hl[0].y1 = v1.y;
      hl[0].z1 = v1.z;
      hl[0].col = 0xffffff00;
      ShowHintsPart(m,hl,1,WireColor[EWC_HINT],env);
      v.Sub3(v1,v0);
      m.InitDir(v);
      m.Scale3(HintSize);
      m.l = v1;
      ShowHintsPart(m,data+15,8,WireColor[EWC_HINT],env);
      break;
    case 0x95:                        // multiply
      m.Init();
      m.Scale3(HintSize);
      ShowHintsPart(m,data+12,3,0,env);

      m.Init();
      ms.InitSRT(op->GetEditPtrF(0));
      v.Init3();
      sSetRndSeed(*op->GetEditPtrS(9));
      for(i=0;i<*op->GetEditPtrS(9)-1;i++)
      {
        hl[i].x0 = m.l.x;
        hl[i].y0 = m.l.y;
        hl[i].z0 = m.l.z;
        if(*op->GetEditPtrS(10) & 2)
          m.InitRandomSRT(op->GetEditPtrF(0));
        else
          m.MulA(ms);
        hl[i].x1 = m.l.x;
        hl[i].y1 = m.l.y;
        hl[i].z1 = m.l.z;
        hl[i].col = 0xffffff00;
        v.x = hl[i].x1 - hl[i].x0;
        v.y = hl[i].y1 - hl[i].y0;
        v.z = hl[i].z1 - hl[i].z0;
      }
      ms.Init();
      ShowHintsPart(ms,hl,*op->GetEditPtrS(9)-1,WireColor[EWC_HINT],env);
      ms.InitDir(v);
      ms.Scale3(HintSize);
      ms.l = m.l;
      ShowHintsPart(ms,data+15,8,WireColor[EWC_HINT],env);
      break;
    case 0xa0: // bend
      m.InitEuler(*op->GetEditPtrF(19)*sPI2F,0.0f,*op->GetEditPtrF(20)*sPI2F);
      v.Init(m.i.y,m.j.y,m.k.y,0.0f);
      v0 = v;
      v0.Scale3(*op->GetEditPtrF(21));
      v1 = v;
      v1.Scale3(*op->GetEditPtrF(22));
      
      hl[0].x0 = v0.x;
      hl[0].y0 = v0.y;
      hl[0].z0 = v0.z;
      hl[0].x1 = v1.x;
      hl[0].y1 = v1.y;
      hl[0].z1 = v1.z;
      hl[0].col = 0xffffff00;
      m.Init();
      ShowHintsPart(m,hl,1,WireColor[EWC_HINT],env);

      ms.InitDir(v);
      ms.Scale3(HintSize);
      ms.l = v1;
      ShowHintsPart(ms,data+15,8,WireColor[EWC_HINT],env);
      break;
    case 0xaa: // selectangle
      m.InitEuler(*op->GetEditPtrF(2)*sPI2F,0.0f,*op->GetEditPtrF(3)*sPI2F);
      v.Init(m.i.y,m.j.y,m.k.y,0.0f);

      hl[0].x0 = hl[0].y0 = hl[0].z0 = 0.0f;
      hl[0].x1 = v.x;
      hl[0].y1 = v.y;
      hl[0].z1 = v.z;
      hl[0].col = 0xffffff00;
      m.Init();
      ShowHintsPart(m,hl,1,WireColor[EWC_HINT],env);

      ms.InitDir(v);
      ms.Scale3(-HintSize);
      ShowHintsPart(ms,data+15,8,WireColor[EWC_HINT],env);
      break;
    case 0xab: // bend2
      // center
      ms.InitEulerPI2(op->GetEditPtrF(3));
      m.Init();
      m.l.x = *op->GetEditPtrF(0);
      m.l.y = *op->GetEditPtrF(1);
      m.l.z = *op->GetEditPtrF(2);
      m.l.RotateT4(ms);
      ShowHintsPart(m,data+12,3,WireColor[EWC_HINT],env);
      // direction
      hl[0].x0 = m.l.x;
      hl[0].y0 = m.l.y;
      hl[0].z0 = m.l.z;
      m.InitEulerPI2(op->GetEditPtrF(3));
      hl[0].x1 = hl[0].x0 + m.i.y * *op->GetEditPtrF(6);
      hl[0].y1 = hl[0].y0 + m.j.y * *op->GetEditPtrF(6);
      hl[0].z1 = hl[0].z0 + m.k.y * *op->GetEditPtrF(6);
      hl[0].col = 0xffffff00;
      m.Init();
      ShowHintsPart(m,hl,1,WireColor[EWC_HINT],env);
      m.InitEulerPI2(op->GetEditPtrF(3));
      m.l.x = hl[0].x1;
      m.l.y = hl[0].y1;
      m.l.z = hl[0].z1;
      ShowHintsPart(m,data+15,8,WireColor[EWC_HINT],env);
      break;
    case 0xad: // color
      m.Init();
      m.Scale3(0.25f);
      m.l.x = *op->GetEditPtrF(0);
      m.l.y = *op->GetEditPtrF(1);
      m.l.z = *op->GetEditPtrF(2);
      ShowHintsPart(m,data+12,3,WireColor[EWC_HINT],env);
      break;
    case 0xae: // bend s
      // center
      m.Init();
      m.l.x = *op->GetEditPtrF(0);
      m.l.y = *op->GetEditPtrF(1);
      m.l.z = *op->GetEditPtrF(2);
      ShowHintsPart(m,data+12,3,WireColor[EWC_HINT],env);
      // direction
      hl[0].x0 = m.l.x;
      hl[0].y0 = m.l.y;
      hl[0].z0 = m.l.z;
      m.InitEulerPI2(op->GetEditPtrF(3));
      m.Trans3();
      hl[0].x1 = hl[0].x0 + m.i.z * *op->GetEditPtrF(6);
      hl[0].y1 = hl[0].y0 + m.j.z * *op->GetEditPtrF(6);
      hl[0].z1 = hl[0].z0 + m.k.z * *op->GetEditPtrF(6);
      hl[0].col = 0xffffff00;
      m.Init();
      ShowHintsPart(m,hl,1,WireColor[EWC_HINT],env);
      v0.Init(hl[0].x1-hl[0].x0,hl[0].y1-hl[0].y0,hl[0].z1-hl[0].z0);
      m.InitDir(v0);
      m.l.x = hl[0].x1;
      m.l.y = hl[0].y1;
      m.l.z = hl[0].z1;
      ShowHintsPart(m,data+15,8,WireColor[EWC_HINT],env);
      break;

    case 0xc3:                      // transform
      m.Init();
      msh = (GenMesh*)op->Cache;
      v1.x = *op->GetEditPtrF(6);
      v1.y = *op->GetEditPtrF(7);
      v1.z = *op->GetEditPtrF(8);
      v1.w = 1;
      v0.Init(0,0,0,1);
      hl[0].x0 = v0.x;
      hl[0].y0 = v0.y;
      hl[0].z0 = v0.z;
      hl[0].x1 = v1.x;
      hl[0].y1 = v1.y;
      hl[0].z1 = v1.z;
      hl[0].col = 0xffffff00;
      ShowHintsPart(m,hl,1,WireColor[EWC_HINT],env);
      v.Sub3(v1,v0);
      m.InitDir(v);
      m.Scale3(HintSize);
      m.l = v1;
      ShowHintsPart(m,data+15,8,WireColor[EWC_HINT],env);
      break;
    case 0xc2:                        // multiply
      m.Init();
      m.Scale3(HintSize);
      ShowHintsPart(m,data+12,3,0,env);

      m.Init();
      ms.InitSRT(op->GetEditPtrF(0));
      v.Init3();
      for(i=0;i<*op->GetEditPtrS(9)-1;i++)
      {
        hl[i].x0 = m.l.x;
        hl[i].y0 = m.l.y;
        hl[i].z0 = m.l.z;
        m.MulA(ms);
        hl[i].x1 = m.l.x;
        hl[i].y1 = m.l.y;
        hl[i].z1 = m.l.z;
        hl[i].col = 0xffffff00;
        v.x = hl[i].x1 - hl[i].x0;
        v.y = hl[i].y1 - hl[i].y0;
        v.z = hl[i].z1 - hl[i].z0;
      }
      ms.Init();
      ShowHintsPart(ms,hl,*op->GetEditPtrS(9)-1,WireColor[EWC_HINT],env);
      ms.InitDir(v);
      ms.Scale3(HintSize);
      ms.l = m.l;
      ShowHintsPart(ms,data+15,8,WireColor[EWC_HINT],env);
      break;

    case 0xc4: // scene_light
      m.Init();
      m.Scale3(0.25f);
      if(ShowPortalOpProcessed)
      {
        /*m.l.x = *op->GetEditPtrF(3);
        m.l.y = *op->GetEditPtrF(4);
        m.l.z = *op->GetEditPtrF(5);*/
        m.l = ShowPortalCube[0];
        ShowHintsPart(m,data+12,3,WireColor[EWC_HINT],env);
      }
      break;

    case 0xcd: // scene_portal
      if(ShowPortalOpProcessed)
      {
        //col = ShowPortalJob ? WireColor[WC_HINT] : 0xffff0000;
        col = 0xffff0000;

        if(WireOptions & EWF_LINEPORTALS)
        {
          // wireframe cube
          for(i=0;i<12;i++)
          {
            hl[i].x0 = ShowPortalCube[wireCubeVert[i*2+0]].x;
            hl[i].y0 = ShowPortalCube[wireCubeVert[i*2+0]].y;
            hl[i].z0 = ShowPortalCube[wireCubeVert[i*2+0]].z;
            hl[i].x1 = ShowPortalCube[wireCubeVert[i*2+1]].x;
            hl[i].y1 = ShowPortalCube[wireCubeVert[i*2+1]].y;
            hl[i].z1 = ShowPortalCube[wireCubeVert[i*2+1]].z;
            hl[i].col = col;
          }
          m.Init();
          ShowHintsPart(m,hl,12,col,env);
        }

        // cube faces (transparent)
        if(WireOptions & EWF_SOLIDPORTALS)
        {
          MtrlCollision->Set(*env);
          handle = sSystem->GeoAdd(sFVF_COMPACT,sGEO_TRI|sGEO_DYNAMIC);
          sSystem->GeoBegin(handle,8,6*6,&vp,(void **)&ip);
          for(i=0;i<8;i++)
          {
            *vp++ = ShowPortalCube[i].x;
            *vp++ = ShowPortalCube[i].y;
            *vp++ = ShowPortalCube[i].z;
            *(sU32 *) vp = WireColor[EWC_PORTAL]; vp++;
          }
          for(i=0;i<6;i++)
          {
            *ip++ = cubeTable[i*4+0];
            *ip++ = cubeTable[i*4+1];
            *ip++ = cubeTable[i*4+2];
            *ip++ = cubeTable[i*4+0];
            *ip++ = cubeTable[i*4+2];
            *ip++ = cubeTable[i*4+3];
          }
          sSystem->GeoEnd(handle);
          sSystem->GeoDraw(handle);
          sSystem->GeoRem(handle);
        }
      }
      break;
    case 0xce:  // physics
      m.Init();
      m.i.x = *op->GetEditPtrF(4)*0.5f;
      m.j.y = *op->GetEditPtrF(5)*0.5f;
      m.k.z = *op->GetEditPtrF(6)*0.5f;
      ShowHintsPart(m,data+0,12,WireColor[EWC_HINT],env);      
      break;

    case 0x0113:                      // selectcube
      m.Init();
      m.i.x = *op->GetEditPtrF(4)*0.5f;
      m.j.y = *op->GetEditPtrF(5)*0.5f;
      m.k.z = *op->GetEditPtrF(6)*0.5f;
      m.l.x = *op->GetEditPtrF(1);
      m.l.y = *op->GetEditPtrF(2);
      m.l.z = *op->GetEditPtrF(3);
      ShowHintsPart(m,data+0,12,WireColor[EWC_HINT],env);
      break;
    case 0x0125: // bend2
      // center
      ms.InitEulerPI2(op->GetEditPtrF(3));
      m.Init();
      m.l.x = *op->GetEditPtrF(0);
      m.l.y = *op->GetEditPtrF(1);
      m.l.z = *op->GetEditPtrF(2);
      m.l.RotateT4(ms);
      ShowHintsPart(m,data+12,3,WireColor[EWC_HINT],env);
      // direction
      hl[0].x0 = m.l.x;
      hl[0].y0 = m.l.y;
      hl[0].z0 = m.l.z;
      m.InitEulerPI2(op->GetEditPtrF(3));
      hl[0].x1 = hl[0].x0 + m.i.y * *op->GetEditPtrF(6);
      hl[0].y1 = hl[0].y0 + m.j.y * *op->GetEditPtrF(6);
      hl[0].z1 = hl[0].z0 + m.k.y * *op->GetEditPtrF(6);
      hl[0].col = 0xffffff00;
      m.Init();
      ShowHintsPart(m,hl,1,WireColor[EWC_HINT],env);
      m.InitEulerPI2(op->GetEditPtrF(3));
      m.l.x = hl[0].x1;
      m.l.y = hl[0].y1;
      m.l.z = hl[0].z1;
      ShowHintsPart(m,data+15,8,WireColor[EWC_HINT],env);
      break;

    case 0x127:
      if(op->GetInput(0) && op->GetInput(0)->Cache && op->GetInput(0)->Cache->ClassId==KC_MINMESH)
      {
        GenMinMesh *mesh = (GenMinMesh *)op->GetInput(0)->Cache;

        if(mesh->Animation && mesh->Animation->Matrices.Count>0)
        {
          sMatrix *matrices = new sMatrix[mesh->Animation->Matrices.Count];
          mesh->Animation->EvalAnimation(*op->GetEditPtrF(0),*op->GetEditPtrF(1),matrices);

          for(sInt i=0;i<mesh->Animation->Matrices.Count;i++)
          {
            GenMinMatrix *mm = &mesh->Animation->Matrices[i];

            m = mm->Temp;
            m.Scale3(0.125f);
            ShowHintsPart(m,data+12,3,WireColor[EWC_HINT],env);

            if(mm->Parent>0)
            {
              m.Init();
              hl[0].x0 = mm->Temp.l.x;
              hl[0].y0 = mm->Temp.l.y;
              hl[0].z0 = mm->Temp.l.z;
              hl[0].x1 = mesh->Animation->Matrices[mm->Parent].Temp.l.x;
              hl[0].y1 = mesh->Animation->Matrices[mm->Parent].Temp.l.y;
              hl[0].z1 = mesh->Animation->Matrices[mm->Parent].Temp.l.z;
              ShowHintsPart(m,hl,2,WireColor[EWC_HINT],env);
            }
          }
          delete[] matrices;
        }
      }
      break;
    case 0x0128: // bonechain
      {
        sVector p0,p1;
        sInt count;

        if(*op->GetEditPtrU(7) & 1)           // auto mode
        {
          if(op->Cache && op->Cache->ClassId==KC_MINMESH)
          {
            GenMinMesh *mesh = (GenMinMesh *) op->Cache;
            sAABox box;
            mesh->CalcBBox(box);
            p0.Init(0,0,box.Min.z);
            p1.Init(0,0,box.Max.z);
          }
          else      // fucked up
          {
            p0.Init();
            p1.Init();
          }
        }
        else      // manual
        {
          p0.x = *op->GetEditPtrF(0);
          p0.y = *op->GetEditPtrF(1);
          p0.z = *op->GetEditPtrF(2);
          p0.w = 1.0f;
          p1.x = *op->GetEditPtrF(3);
          p1.y = *op->GetEditPtrF(4);
          p1.z = *op->GetEditPtrF(5);
          p1.w = 1.0f;
        }
        count = *op->GetEditPtrU(6);

        for(sInt i=0;i<count;i++)
        {
          m.Init();
          m.Scale3(1.0f);
          m.l.Lin4(p0,p1,1.0f*i/(count-1));
          ShowHintsPart(m,data+12,3,WireColor[EWC_HINT],env);
        }

        m.Init();
        hl[0].x0 = p0.x;
        hl[0].y0 = p0.y;
        hl[0].z0 = p0.z;
        hl[1].x0 = p1.x;
        hl[1].y0 = p1.y;
        hl[1].z0 = p1.z;
        ShowHintsPart(m,hl,2,WireColor[EWC_HINT],env);
      }
      break;
    }

    if(ShowXZPlane)
    {
      v = CamMatrix.l;
      v.x = (sInt) v.x;
      v.z = (sInt) v.z;

      for(i=0;i<101;i++)
      {
        hl[i].x0 = v.x + (i - 50);
        hl[i].y0 = 0.0f;
        hl[i].z0 = v.z - 50;
        hl[i].x1 = hl[i].x0;
        hl[i].y1 = 0.0f;
        hl[i].z1 = v.z + 50;
        hl[i].col = 0xffffffff;
        hl[i+101].x0 = v.x - 50;
        hl[i+101].y0 = 0.0f;
        hl[i+101].z0 = v.z + (i - 50);
        hl[i+101].x1 = v.x + 50;
        hl[i+101].y1 = 0.0f;
        hl[i+101].z1 = hl[i+101].z0;
        hl[i+101].col = 0xffffffff;
      }

      m.Init();
      ShowHintsPart(m,hl,202,WireColor[EWC_GRID],env);
    }
  }
}

void WinView::ShowHintsPart(sMatrix &m,const HintLine *data,sInt linecount,sU32 col,sMaterialEnv *env)
{
  sInt handle;
  sF32 *fp;
  sInt i;
  sVector v;

  if(!linecount)
    return;

  sSystem->GetPerf(Perf,sPIM_PAUSE);

  MtrlFlatZ->Set(*env);
  handle = GeoLine;
  sSystem->GeoBegin(handle,linecount*2,0,&fp,0);
  for(i=0;i<linecount;i++)
  {
    v.Init(data->x0,data->y0,data->z0,1);
    v.Rotate4(m);
    *fp++ = v.x;
    *fp++ = v.y;
    *fp++ = v.z;
    *(sU32 *)fp = col ? col : data->col; fp++;

    v.Init(data->x1,data->y1,data->z1,1);
    v.Rotate4(m);
    *fp++ = v.x;
    *fp++ = v.y;
    *fp++ = v.z;
    *(sU32 *)fp = col ? col : data->col; fp++;

    data++;
  }
  sSystem->GeoEnd(handle,linecount*2,0);
  sSystem->GeoDraw(handle);
//  sSystem->GeoRem(handle);

  sSystem->GetPerf(Perf,sPIM_CONTINUE);
}

void WinView::ShowNormalsAndTangents(GenMinMesh *mesh,sMaterialEnv *env)
{
  static const sF32 scaleF = 0.125f;

  sSystem->GetPerf(Perf,sPIM_PAUSE);
  MtrlFlatZ->Set(*env);

  sInt firstVert = 0;
  while(firstVert < mesh->Vertices.Count)
  {
    sInt vc = sMin(mesh->Vertices.Count - firstVert,0x2000);
    sF32 *fp;

    sSystem->GeoBegin(GeoLine,vc*6,0,&fp,0);

    for(sInt i=0;i<vc;i++)
    {
      GenMinVert *v = &mesh->Vertices[firstVert+i];

      // normal in green
      *fp++ = v->Pos.x;
      *fp++ = v->Pos.y;
      *fp++ = v->Pos.z;
      *(sU32 *) fp = 0xff00ff00; fp++;

      *fp++ = v->Pos.x + scaleF * v->Normal.x;
      *fp++ = v->Pos.y + scaleF * v->Normal.y;
      *fp++ = v->Pos.z + scaleF * v->Normal.z;
      *(sU32 *) fp = 0xff00ff00; fp++;

      // tangent in red
      *fp++ = v->Pos.x;
      *fp++ = v->Pos.y;
      *fp++ = v->Pos.z;
      *(sU32 *) fp = 0xffff0000; fp++;

      *fp++ = v->Pos.x + scaleF * v->Tangent.x;
      *fp++ = v->Pos.y + scaleF * v->Tangent.y;
      *fp++ = v->Pos.z + scaleF * v->Tangent.z;
      *(sU32 *) fp = 0xffff0000; fp++;

      // binormal in blue
      GenMinVector binormal;
      binormal.Cross(v->Normal,v->Tangent);

      *fp++ = v->Pos.x;
      *fp++ = v->Pos.y;
      *fp++ = v->Pos.z;
      *(sU32 *) fp = 0xff0000ff; fp++;

      *fp++ = v->Pos.x + scaleF * binormal.x;
      *fp++ = v->Pos.y + scaleF * binormal.y;
      *fp++ = v->Pos.z + scaleF * binormal.z;
      *(sU32 *) fp = 0xff0000ff; fp++;
    }

    sSystem->GeoEnd(GeoLine);
    sSystem->GeoDraw(GeoLine);

    firstVert += vc;
  }

  sSystem->GetPerf(Perf,sPIM_CONTINUE);
}

void WinView::ShowDynCell(sMaterialEnv *env)
{
  KKriegerCellDynamic *cell;
  KKriegerMonster *mon;
  sInt max;
  sF32 *fp;
  sU16 *ip;
  sInt i,j,k;
  sU32 col;
  sVector v;
  sMatrix mat;

  const sF32 s=1.0f;
  static const HintLine data[] = 
  {
    {  0,-s, 0,  s, 0, 0, 0xffffff00, },
    {  0,-s, 0,  0, 0, s, 0xffffff00, },
    {  0,-s, 0, -s, 0, 0, 0xffffff00, },
    {  0,-s, 0,  0, 0,-s, 0xffffff00, },
    {  0, s, 0,  s, 0, 0, 0xffffff00, },
    {  0, s, 0,  0, 0, s, 0xffffff00, },
    {  0, s, 0, -s, 0, 0, 0xffffff00, },
    {  0, s, 0,  0, 0,-s, 0xffffff00, },
    {  0, 0,-s,  s, 0, 0, 0xffffff00, },
    {  0, 0,-s, -s, 0, 0, 0xffffff00, },
    {  0, 0, s,  s, 0, 0, 0xffffff00, },
    {  0, 0, s, -s, 0, 0, 0xffffff00, },
  };

  max = Game.DCellUsed;
  if(max>0 && (WireOptions&EWF_COLLISION))
  {
    env->ModelSpace.Init();
    MtrlCollision->Set(*env);
    sSystem->GeoBegin(GeoFace,max*8,max*36,&fp,(void **)&ip);
    col = WireColor[EWC_COLZONE];

    k = 0;
    for(i=0;i<max;i++)
    {
      cell = Game.DCellPtr[i];

      for(j=0;j<8;j++)
      {
        v = cell->Vertices[j];

        *fp++ = v.x;
        *fp++ = v.y;
        *fp++ = v.z;
        *(sU32 *)fp = col; fp++;
      }

      sQuad(ip,0+k,2+k,3+k,1+k);
      sQuad(ip,1+k,3+k,7+k,5+k);
      sQuad(ip,4+k,5+k,7+k,6+k);
      sQuad(ip,0+k,4+k,6+k,2+k);
      sQuad(ip,0+k,1+k,5+k,4+k);
      sQuad(ip,2+k,6+k,7+k,3+k);
      k+=8;
    }
    sSystem->GeoEnd(GeoFace);
    sSystem->GeoDraw(GeoFace);
  }

  mat.Init();
  mat.i.x = Game.Player.Collider.Radius;
  mat.j.y = Game.Player.Collider.Radius;
  mat.k.z = Game.Player.Collider.Radius;
  mat.l = Game.Player.Collider.Pos;
  ShowHintsPart(mat,data,12,0xffffff00,env);

  for(i=0;i<Game.Monsters.Count;i++)
  {
    mon = Game.Monsters.Get(i);
    if(mon->State>=KMS_INRANGE)
    {
      mat.l = mon->Collider.Pos;
      mat.i.x = mon->Collider.Radius;
      mat.j.y = mon->Collider.Radius;
      mat.k.z = mon->Collider.Radius;
      ShowHintsPart(mat,data,12,0xffff0000,env);
    }
  }
}

void WinView::ShowMeshLights(class GenMesh *mesh,sMaterialEnv *env)
{
  const sF32 s=0.125f;
  static const HintLine data[] = 
  {
    { -s, 0, 0,  s, 0, 0, 0xffff0000, },    // 12,3: cross
    {  0,-s, 0,  0, s, 0, 0xff00ff00, },
    {  0, 0,-s,  0, 0, s, 0xff0000ff, },
  };
  sMatrix mat;
  sInt i;

  mat.Init();

  for(i=0;i<mesh->Lgts.Count;i++)
  {
    mat.l = mesh->VertBuf[mesh->Lgts[i] * mesh->VertSize()];
    ShowHintsPart(mat,data,3,0xffff7f00,env);
  }
}

void WinView::ShowBlobSplineHints(BlobSpline *spline2,sMaterialEnv *env)
{
  const sF32 s=0.125f;
  static const HintLine data[] = 
  {
    { -s*2, 0,   0,  s*2, 0, 0, 0xffff0000, },
    {    0,-s,   0,  0  , s, 0, 0xff00ff00, },
    {    0, 0,s*32,  0  , 0,-s, 0xff0000ff, },
    {    0, 0, s*3,-s*2 , 0, 0, 0xff0000ff, },
    {    0, 0, s*3, s*2 , 0, 0, 0xff0000ff, },
    {    0, 0, s*3,  0  ,-s, 0, 0xff0000ff, },
    {    0, 0, s*3,  0  , s, 0, 0xff0000ff, },

//    {    0, 0,   s,  0  , 0,-s, 0xff0000ff, },
  };
  sMatrix mat,null;

  mat.Init();
  BlobSpline *spline = spline2->Copy();
  spline->Sort();

  sF32 zoom;

  for(sInt i=0;i<spline->Count;i++)
  {
    spline->Calc(spline->Keys[i].Time,mat,zoom);
    ShowHintsPart(mat,data,7,(spline->Keys[i].Select)?0xffffffff:0xffff7f00,env);
  }
/*  
  for(sF32 f=0;f<=1.0f;f+=1/64.0f)
  {
    spline->Calc(f,mat,zoom);
    ShowHintsPart(mat,data+7,1,0xffff7f00,env);
  }
*/
  spline->Calc(0,mat,zoom);
  null.Init();

  for(sInt i=0;i<16;i++)
  {
    HintLine line[16];
    for(sInt j=0;j<16;j++)
    {
      line[j].col = (j&4)?0xffff0000:0xffffff00;
      line[j].x0 = mat.l.x;
      line[j].y0 = mat.l.y;
      line[j].z0 = mat.l.z;

      spline->Calc((i*16+j)/256.0f,mat,zoom);
//      spline->Calc(sFade(spline->Keys[i].Time,spline->Keys[i+1].Time,(j+1)/16.0f),mat);

      line[j].x1 = mat.l.x;
      line[j].y1 = mat.l.y;
      line[j].z1 = mat.l.z;

    }
    ShowHintsPart(null,line,16,0,env);
  }
  delete[] (sU8 *)spline;
}

void WinView::LeaveGameMode()
{
  GameMode = 0;
  Cam.State[6] = Game.PlayerPos.x;
  Cam.State[7] = Game.PlayerPos.y+Game.CamOffset;
  Cam.State[8] = Game.PlayerPos.z;
  Cam.State[3] = Game.PlayerLook/sPI2;
  Cam.State[4] = Game.PlayerDir/sPI2;
  Cam.State[5] = 0;
	CamMatrix.InitSRT(Cam.State);
  Cam.Zoom[0] = 128;
  Cam.Ortho = 0;
  if(App->GetMode()==MODE_GAMEINFO)
    App->SetMode(MODE_NORMAL);
}

void WinView::EnterGameMode()
{
  if(Object && Object->Op.Cache && (Object->Op.Cache->ClassId==KC_MESH || Object->Op.Cache->ClassId==KC_SCENE || 
    Object->Op.Cache->ClassId==KC_IPP || Object->Op.Cache->ClassId==KC_DEMO))
  {
    sVector v;
    v.Init(Cam.State[6],Cam.State[7]-Game.CamOffset,Cam.State[8]);
    GameMode = 1;
    if(Object->Op.Cache->ClassId == KC_SCENE)
      Game.SetScene((GenScene *)Object->Op.Cache);
    else if(Object->Op.Cache->ClassId == KC_MESH)
      Game.SetMesh((GenMesh *)Object->Op.Cache);
    else
      Game.SetPainter(&Object->Op,App->Doc->KEnv);

    Game.SetPlayer(v,Cam.State[4]*sPI2,Cam.State[3]*sPI2);
    if(App->GetMode()<4)
      App->SetMode(MODE_GAMEINFO);
  }
}

void WinView::UpdateSceneWire()
{
  sU32 wireMask = SelEdge | (SelFace << 8) | (SelVertex << 16);

  if(SceneWire != SceneWireframe || WireOptions != SceneWireFlags || wireMask != SceneWireMask)
  {
    SceneWireframe = SceneWire;
    SceneWireFlags = WireOptions;
    SceneWireMask = wireMask;

    FlushSceneWire();
  }
}

void WinView::FlushSceneWireR(WerkOp *op)
{
  if(op->Op.Cache && op->Op.Cache->ClassId == KC_SCENE)
  {
    op->Op.Change();

    for(sInt i=0;i<op->GetInputCount();i++)
      FlushSceneWireR(op->GetInput(i));

    for(sInt i=0;i<op->GetLinkMax();i++)
    {
      if(op->GetLink(i))
        FlushSceneWireR(op->GetLink(i));
    }
  }
}

void WinView::FlushSceneWire()
{
  if(Object)
    FlushSceneWireR(Object);
}

void WinView::FlushMeshWire()
{
  if(Object)
  {
    GenMesh *mesh = (GenMesh *)Object->Op.Cache;
    if(mesh)
      mesh->UnPrepareWire();
  }
}

/****************************************************************************/

void WinView::RecordAVI(sChar *filename,sF32 fps)
{
  sSystem->VideoStart(filename,fps,App->Doc->ResX,App->Doc->ResY);

  RecordVideo = 1;
  VideoFPS = fps;
  VideoFrame = 0;
}

/****************************************************************************/
