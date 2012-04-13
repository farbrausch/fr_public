// This file is distributed under a BSD license. See LICENSE.txt for details.
#include "winview.hpp"
#include "winpara.hpp"
#include "winpage.hpp"
#include "genbitmap.hpp"
#include "genmesh.hpp"
#include "genmaterial.hpp"
#include "genfx.hpp"

/****************************************************************************/

#define DM_ORBIT      1
#define DM_ZOOM       2
#define DM_PAN        3
#define DM_SCROLL     4
#define DM_QUAKE      5

/****************************************************************************/
/****************************************************************************/

ViewWin::ViewWin()
{
  Flags |= sGWF_PAINT3D;

  TextureCache = 0;
  TextureCacheCount = 0;
  DragStartX = 0;
  DragStartY = 0;
  DragMode = 0;
  DragCID = 0;
  Object = 0;
  PlayerMode = 0;
  SelectMask = 0x010101;
  ClearColor[0] = 0;
  ClearColor[1] = 0;
  ClearColor[2] = 0;
  ClearColor[3] = 0;
  QuakeSpeedForw = 0;
  QuakeSpeedSide = 0;
  QuakeAccelForw = 0;
  QuakeAccelSide = 0;
  LinkEditObj = 0;

	CamState[0]=CamState[1]=CamState[2]=1.0f;
	CamState[3]=CamState[4]=CamState[5]=0.0f;
	CamState[6]=CamState[7]=0.0f; CamState[8]=-5.0f;
	Cam.InitSRT(CamState);

  OnKey('r');
  DragKey = DM_ORBIT;
  StickyKey = 0;

  ShaderBall = (GenMesh *)Mesh_NewMesh(0x10000,0x00030000);
  Mesh_Torus(ShaderBall,32<<16,16<<16,0x8000,0x0000,0x10000);
  Mesh_CalcNormals(ShaderBall,0,0,1<<16);
  ShaderBallCalc = 0;
}


ViewWin::~ViewWin()
{
}

void ViewWin::Tag()
{
  ToolWindow::Tag();
  sBroker->Need(Object);
  sBroker->Need(TextureCache);
  sBroker->Need(ShaderBall);
}

void ViewWin::SetObject(ToolObject *to)
{
  PlayerMode = 0;
  if(Object!=to)
  {
    Object = to;
    TextureCache = 0;
    TextureCacheCount = 0;
    sGui->GarbageCollection();
  }
}

/****************************************************************************/

void ViewWin::OnPaint()
{

}


void ViewWin::OnPaint3d(sViewport &view)
{
  sInt cid;
  sF32 dist;
  sF32 speedy;
  GenBitmap *gbm;

  static sChar *dragmodes[] = { "???","orbit","zoom","pan","scroll","ego" };
  PageOp *viewop;

// quake cam logic, not frame synced

  speedy = 1;
  if(sSystem->GetKeyboardShiftState()&sKEYQ_SHIFT)
    speedy = 4;
  QuakeSpeedForw = (QuakeSpeedForw+QuakeAccelForw*speedy)*.70f;
  QuakeSpeedSide = (QuakeSpeedSide+QuakeAccelSide*speedy)*.70f;
	CamState[6] += 0.03f*(QuakeSpeedSide*Cam.i.x+QuakeSpeedForw*Cam.k.x);
	CamState[7] += 0.03f*(QuakeSpeedSide*Cam.i.y+QuakeSpeedForw*Cam.k.y);
	CamState[8] += 0.03f*(QuakeSpeedSide*Cam.i.z+QuakeSpeedForw*Cam.k.z);
  if(DragMode==3)
  {
    dist = -Cam.l.Abs3();
		CamState[6] = Cam.k.x*dist;
		CamState[7] = Cam.k.y*dist;
		CamState[8] = Cam.k.z*dist;
  }

	Cam.InitSRT(CamState);

// display dragmode in status bar

  if(Flags&sGWF_CHILDFOCUS)
    App->SetStat(0,dragmodes[DragKey]);

// if player is running, just call it

  if(PlayerMode)
  {
		FXMaster->SetMasterViewport(view);
    App->Player->OnPaint(&view.Window);
    return;
  }

// calculate current object

  cid = 0;
  if(Object)
  {
    Object->Calc();

    if(Object->Cache && Object->CID==Object->Cache->GetClass())  
    {
      cid = Object->CID;
    }
  }

// display as needed

  switch(cid)
  {
  case sCID_GENBITMAP:
    DragCID = sCID_GENBITMAP;
    gbm = (GenBitmap *) Object->Cache;
    if(gbm->Texture==sINVALID)
    {
      if(TextureCache && TextureCacheCount >= Object->CalcCount)
      {
        gbm = TextureCache;
      }
      else
      {
        gbm = new GenBitmap;
        gbm->Copy(Object->Cache);
        Bitmap_MakeTexture(gbm);

        TextureCacheCount = App->CheckChange();
        TextureCache = gbm;
        sGui->GarbageCollection();
      }
      sVERIFY(gbm->Texture!=sINVALID);
    }
    ShowBitmap(view,gbm->Texture);
    break;
  case sCID_GENMESH:
    DragCID = sCID_GENMESH;
    ShowMesh(view,(GenMesh *)Object->Cache);
    break;
  case sCID_GENSCENE:
    DragCID = sCID_GENSCENE;
    ShowScene(view,(GenScene *)Object->Cache);
    break;
	case sCID_GENFXCHAIN:
		DragCID = sCID_GENFXCHAIN;
		ShowFXChain(view,(GenFXChain *)Object->Cache);
		break;
  case sCID_GENMATERIAL:
    DragCID = sCID_GENMESH;
    if(App->CheckChange() > ShaderBallCalc || ShaderBall->Mtrl[1].Material!=(GenMaterial *)Object->Cache)
    {
      ShaderBall = (GenMesh *)Mesh_NewMesh(0x10000,0x00030000);
      Mesh_Torus(ShaderBall,32<<16,16<<16,0x8000,0x0000,0x10000);
      ShaderBallCalc = App->CheckChange();
      ShaderBall->Mtrl[1].Material = (GenMaterial *)Object->Cache;
      Mesh_CalcNormals(ShaderBall,0,0,1<<16);
      ShaderBall->Prepare();
    }
    ShowMesh(view,ShaderBall);
    break;
  default:
    DragCID = 0;
    view.ClearColor = sGui->Palette[sGC_BACK];
    view.ClearFlags = sVCF_ALL;
    sSystem->BeginViewport(view);
    sSystem->EndViewport();
    
    break;
  }

// copy camera position to viewport operator

  viewop = FindLinkEdit();
  if(viewop && viewop == LinkEditObj)
  {
    viewop->Data[1].Data[0] = Cam.l.x*65536.0f;
    viewop->Data[1].Data[1] = Cam.l.y*65536.0f;
    viewop->Data[1].Data[2] = Cam.l.z*65536.0f;
    viewop->Data[2].Data[0] = CamState[3]*65536.0f*sPI2F;
    viewop->Data[2].Data[1] = CamState[4]*65536.0f*sPI2F;
    viewop->Data[2].Data[2] = CamState[5]*65536.0f*sPI2F;
    viewop->ChangeCount = App->GetChange();
  }
  else
  {
    LinkEditObj = 0;
  }
}

/****************************************************************************/

PageOp *ViewWin::FindLinkEdit()
{
  PageOp *po;

  if(DragCID == sCID_GENFXCHAIN && Object && Object->GetClass() == sCID_TOOL_PAGEOP)
  {
    po = (PageOp *) Object;
    do
    {
      if(po->CID != sCID_GENFXCHAIN)
        break;
      if(po->Class->FuncId == -0x10a)
      {
        return po;
      }
      if(po->InputCount<1)
        break;
      po = po->Inputs[0];
    }
    while(po);
  }

  return 0;
}

/****************************************************************************/

void ViewWin::OnKey(sU32 key)
{
  ParaWin *pw;

  switch(key&(0x8001ffff))
  {
  case sKEY_APPFOCUS:
    App->SetActiveWindow(this);
    break;
  case 'r':
    BitmapZoom = 4;
    BitmapX = 10;
    BitmapY = 10;
    BitmapTile = 0;
    MeshWire = 1;

		CamState[0]=CamState[1]=CamState[2]=1.0f;
		CamState[3]=CamState[4]=CamState[5]=0.0f;
		CamState[6]=CamState[7]=0.0f; CamState[8]=-5.0f;
		Cam.InitSRT(CamState);

    CamZoom = 128;
    break;
  case 'f':
    pw = (ParaWin *)App->FindActiveWindow(sCID_TOOL_PARAWIN);
    if(pw)
      pw->SetView(this);
    break;
  case '^':
    MeshWire = !MeshWire;
    break;
  case 't':
    BitmapTile = !BitmapTile;
  case 'z':
    StickyKey = DragKey;
    DragKey = DM_ZOOM;
    break;
  case 'o':
    StickyKey = DragKey;
    DragKey = DM_ORBIT;
    break;
  case 'p':
    StickyKey = DragKey;
    DragKey = DM_PAN;
    break;
  case 'y':
    StickyKey = DragKey;
    DragKey = DM_SCROLL;
    break;
  case 'w':
  case 'W':
    QuakeAccelForw = 1;
    StickyKey = 0;
    DragKey = DM_QUAKE;
    break;
  case 's':
  case 'S':
    QuakeAccelForw = -1;
    StickyKey = 0;
    DragKey = DM_QUAKE;
    break;
  case 'd':
  case 'D':
    QuakeAccelSide = 1;
    StickyKey = 0;
    DragKey = DM_QUAKE;
    break;
  case 'a':
  case 'A':
    QuakeAccelSide = -1;
    StickyKey = 0;
    DragKey = DM_QUAKE;
    break;
  case 'w'|sKEYQ_BREAK:
  case 'W'|sKEYQ_BREAK:
  case 's'|sKEYQ_BREAK:
  case 'S'|sKEYQ_BREAK:
    QuakeAccelForw = 0;
    break;
  case 'd'|sKEYQ_BREAK:
  case 'D'|sKEYQ_BREAK:
  case 'a'|sKEYQ_BREAK:
  case 'A'|sKEYQ_BREAK:
    QuakeAccelSide = 0;
    break;
  case 'l':
    if(LinkEditObj)
    {
      LinkEditObj = 0;
    }
    else
    {
      LinkEditObj = FindLinkEdit();
      if(LinkEditObj)
      {
        CamState[6] = Cam.l.x = LinkEditObj->Data[1].Data[0]/65536.0f;
        CamState[7] = Cam.l.y = LinkEditObj->Data[1].Data[1]/65536.0f;
        CamState[8] = Cam.l.z = LinkEditObj->Data[1].Data[2]/65536.0f;
        CamState[3] = LinkEditObj->Data[2].Data[0]/(65536.0f*sPI2F);
        CamState[4] = LinkEditObj->Data[2].Data[1]/(65536.0f*sPI2F);
        CamState[5] = LinkEditObj->Data[2].Data[2]/(65536.0f*sPI2F);
      }
    }
    break;
  }
  if((key&sKEYQ_STICKYBREAK) && StickyKey)
    DragKey = StickyKey;
  if((key&sKEYQ_BREAK) && (key&0x1ffff)>='a' && (key&0x1ffff)<='z')
    StickyKey = 0;
}

/****************************************************************************/

void ViewWin::OnDrag(sDragData &dd)
{

  switch(dd.Mode)
  {
  case sDD_START:
    DragMode = 0;

    if(DragCID == sCID_GENBITMAP)
    {
      if(DragKey==DM_SCROLL || (dd.Buttons&4))
      {
        DragMode = 1;
        DragStartX = BitmapX;
        DragStartY = BitmapY;
      }
      else if(DragKey==DM_ZOOM)
      {
        DragMode = 2;
        DragStartY = BitmapZoom;
      }
    }
    if(DragCID == sCID_GENMESH || DragCID == sCID_GENSCENE || DragCID == sCID_GENFXCHAIN)
    {
      if(DragKey==DM_SCROLL || (DragKey==DM_ZOOM&&(dd.Buttons&1)) || (dd.Buttons&4))
      {
        DragMode = 5;
        DragVec = Cam.l;
      }
      else if(DragKey==DM_ZOOM&&(dd.Buttons&2))
      {
        DragMode = 6;
        DragStartFY = CamZoom;
      }
      else if(DragKey==DM_ORBIT || DragKey==DM_QUAKE)
      {
        DragMode = (dd.Buttons&2)?3:7;
				sCopyMem(CamStateBase,CamState,9*sizeof(sF32));
      }
      else if(DragKey==DM_PAN)
      {
        DragMode = 4;
        DragVec = Cam.l;
      }
    }
    break;
  case sDD_DRAG:
    switch(DragMode)
    {
    case 1:   // scroll bitmap
      BitmapX = DragStartX + dd.DeltaX;
      BitmapY = DragStartY + dd.DeltaY;
      break;
    case 2:   // zoom bitmap
      BitmapZoom = sRange<sInt>(DragStartY - (dd.DeltaX-dd.DeltaY)*0.02,8,0);
      break;
    case 3:   // orbit 3d
			CamState[3]=CamStateBase[3]+dd.DeltaY*0.004f;
			CamState[4]=CamStateBase[4]+dd.DeltaX*0.004f;
			Cam.InitSRT(CamState);
      break;
    case 4:   // pan 3d
      Cam.l = DragVec;
      Cam.l.AddScale3(Cam.k,(dd.DeltaY-dd.DeltaX)*0.02f);
      CamState[6] = Cam.l.x;
      CamState[7] = Cam.l.y;
      CamState[8] = Cam.l.z;
      break;
    case 5:   // scroll 3d
      Cam.l = DragVec;
      Cam.l.AddScale3(Cam.i,-dd.DeltaX*0.02f);
      Cam.l.AddScale3(Cam.j, dd.DeltaY*0.02f);
      CamState[6] = Cam.l.x;
      CamState[7] = Cam.l.y;
      CamState[8] = Cam.l.z;
      break;
    case 6:   // zoom 3d
      CamZoom = sRange<sF32>(DragStartFY + (dd.DeltaY-dd.DeltaX)*0.2f,256,1);
      break;
    case 7:   // rotate 3d
			CamState[3]=CamStateBase[3]+dd.DeltaY*0.002f;
			CamState[4]=CamStateBase[4]+dd.DeltaX*0.002f;
			Cam.InitSRT(CamState);
      break;
    }
    break;
  case sDD_STOP:
    DragMode = 0;
    break;

  }
}

/****************************************************************************/

sBool ViewWin::OnCommand(sU32 cmd)
{
  return sFALSE;
}

/****************************************************************************/
/****************************************************************************/

void ViewWin::ShowBitmap(sViewport &view,sInt tex)
{
  sU32 states[256],*p;
  sVertex2d *vp;
  sMatrix mat;
  sU16 *ip;
  sInt x,y,xs,ys;
  sInt handle;

  if(tex)
  {
    view.ClearColor = sGui->Palette[sGC_BACK];
    mat.Init();

    p = states;
    sSystem->StateBase(p,0,sMBM_TEX,0);
    sSystem->StateTex(p,0,0);
    sSystem->StateEnd(p,states,sizeof(states));

    sSystem->BeginViewport(view);
    sSystem->SetStates(states);
    sSystem->SetTexture(0,tex,0);
    sSystem->SetCameraOrtho();
//    sSystem->SetMatrix(mat);

    xs = 256 * (1<<BitmapZoom)/16;
    ys = 256 * (1<<BitmapZoom)/16;
    x = BitmapX;
    y = BitmapY;

    handle = sSystem->GeoAdd(sFVF_DEFAULT2D,sGEO_TRISTRIP);
    sSystem->GeoBegin(handle,4,4,(sF32 **)&vp,&ip);    
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

    sSystem->EndViewport();
  }
}

void ViewWin::ShowMesh(sViewport &view,GenMesh *mesh)
{
  sMatrix mat;
  sCamera cam;
  GenMesh *newmesh;
  sLightInfo li;

  if(mesh->RecMode)
  {
    newmesh = new GenMesh;
    newmesh->Copy(mesh);
    newmesh->RecEnd();
    mesh = newmesh;
  }

  cam.Init();
  cam.CamPos = Cam;
  cam.ZoomX = cam.ZoomY = 128.0f/CamZoom;
  cam.AspectX = view.Window.XSize();
  cam.AspectY = view.Window.YSize();
  mat.Init();
  li.Init();
  li.Type = sLI_DIR;
  li.Ambient = 0xff101010;
  li.Diffuse = 0xffffffff;
  li.Dir = Cam.k;
  li.Dir.Unit3();
  li.Mask = ~0;

  view.ClearColor = 0xff000000 | ((ClearColor[0]&0xff00)<<8) | (ClearColor[1]&0xff000) | ((ClearColor[2]&0xff00)>>8);
  sSystem->BeginViewport(view);
  sSystem->SetCamera(cam);
  sSystem->SetMatrix(mat);
  sSystem->ClearLights();
  sSystem->AddLight(li);
  
  if(MeshWire)
    mesh->PaintWire(SelectMask);
  else
    mesh->Paint();

  sSystem->ClearLights();
  sSystem->EndViewport();
}

void ViewWin::ShowScene(sViewport &view,GenScene *scene)
{
  sU32 states[256],*p;
  sMatrix mat;
  sCamera cam;

  mat.Init();
  cam.Init();
  cam.CamPos = Cam;
  cam.ZoomX = cam.ZoomY = 128.0f/CamZoom;
  cam.AspectX = view.Window.XSize();
  cam.AspectY = view.Window.YSize();

  p = states;
  sSystem->StateBase(p,0,sMBM_TEX,0);
  sSystem->StateTex(p,0,0);
  sSystem->StateEnd(p,states,sizeof(states));

  view.ClearColor = 0xff000000 | ((ClearColor[0]&0xff00)<<8) | (ClearColor[1]&0xff000) | ((ClearColor[2]&0xff00)>>8);
  sSystem->BeginViewport(view);
  sSystem->SetCamera(cam);
  sSystem->SetMatrix(mat);
  sSystem->SetStates(states);

  CurrentGenPlayer = App->PlayerCalc;
  CurrentGenPlayer->TotalFrame++;
  scene->Paint(mat);
  CurrentGenPlayer = 0;

  sSystem->EndViewport();
}

void ViewWin::ShowFXChain(sViewport &view, GenFXChain *chain)
{
  CurrentGenPlayer = App->PlayerCalc;
  CurrentGenPlayer->TotalFrame++;
	FXMaster->SetMasterViewport(view);
	FXChain_Render(chain);
  CurrentGenPlayer = 0;
}
