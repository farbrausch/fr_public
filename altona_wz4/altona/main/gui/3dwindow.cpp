/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "3dwindow.hpp"
#include "util/shaders.hpp"
#include "util/image.hpp"

/****************************************************************************/

extern void sGetMouseHard(sInt &x, sInt &y);

s3DWindow::s3DWindow()
{
  SetEnable(1);

  DragDist = 1.0f;
  GridColor = 0xffffffff;
  CmdReset();
  ScreenshotFlag = 0;
  GearShift = 0;
  GearSpeed = 1;
  GearShiftDisplay = 0;
  GridUnit = 0;

  WireMtrl = new sSimpleMaterial;
  //WireMtrl = new sMaterialBasic;
  WireMtrl->Flags = sMTRL_ZON|sMTRL_CULLON;
  WireMtrl->Prepare(sVertexFormatBasic);
  WireMtrlNoZ = new sSimpleMaterial;
  //WireMtrlNoZ = new sMaterialBasic;
  WireMtrlNoZ->Flags = sMTRL_ZOFF;
  WireMtrlNoZ->Prepare(sVertexFormatBasic);
  WireGeo = new sGeometry;
  WireGeo->Init(sGF_LINELIST,sVertexFormatBasic);

  QuakeSpeed.Init(0,0,0);
  QuakeMask = 0;
  Continuous = 1;
  QuakeTime = sGetTime();

  SideSpeed = ForeSpeed = 0.000020f;
  SpeedDamping = 0.995f;

#if sRENDERER==sRENDER_DX11
  ColorRT = 0;
  DepthRT = 0;
#endif
}

s3DWindow::~s3DWindow()
{
  delete WireMtrl;
  delete WireMtrlNoZ;
  delete WireGeo;
#if sRENDERER==sRENDER_DX11
  delete ColorRT;
  delete DepthRT;
#endif
}

void s3DWindow::InitWire(const sChar *name)
{
  sWireClientWindow::InitWire(name);
  sWire->AddKey(name,L"Reset",sMessage(this,&s3DWindow::CmdReset));
  sWire->AddKey(name,L"ResetTilt",sMessage(this,&s3DWindow::CmdResetTilt));
  sWire->AddKey(name,L"Grid",sMessage(this,&s3DWindow::CmdGrid));
  sWire->AddChoice(name,L"Grid_",sMessage(this,&s3DWindow::CmdGrid),&Grid,L"-|Grid");
  sWire->AddKey(name,L"QuakeCam",sMessage(this,&s3DWindow::CmdQuakeCam));
  sWire->AddKey(name,L"GearUp",sMessage(this,&s3DWindow::CmdGearShift,1));
  sWire->AddKey(name,L"GearDown",sMessage(this,&s3DWindow::CmdGearShift,-1));
  sWire->AddDrag(name,L"Dolly",sMessage(this,&s3DWindow::DragDolly));
  sWire->AddDrag(name,L"Zoom",sMessage(this,&s3DWindow::DragZoom));
  sWire->AddDrag(name,L"Orbit",sMessage(this,&s3DWindow::DragOrbit));
  sWire->AddDrag(name,L"Rotate",sMessage(this,&s3DWindow::DragRotate));
  sWire->AddDrag(name,L"Tilt",sMessage(this,&s3DWindow::DragTilt));
  sWire->AddDrag(name,L"Move",sMessage(this,&s3DWindow::DragMove));
  sWire->AddKey(name,L"QuakeForwToggle",sMessage(this,&s3DWindow::CmdQuakeForwToggle));
  sWire->AddKey(name,L"QuakeBackToggle",sMessage(this,&s3DWindow::CmdQuakeBackToggle));
  sWire->AddKey(name,L"QuakeLeftToggle",sMessage(this,&s3DWindow::CmdQuakeLeftToggle));
  sWire->AddKey(name,L"QuakeRightToggle",sMessage(this,&s3DWindow::CmdQuakeRightToggle));
  sWire->AddKey(name,L"QuakeUpToggle",  sMessage(this,&s3DWindow::CmdQuakeUpToggle));
  sWire->AddKey(name,L"QuakeDownToggle",sMessage(this,&s3DWindow::CmdQuakeDownToggle));
  sWire->AddKey(name,L"Screenshot",     sMessage(this,&s3DWindow::CmdScreenshot));
}

void s3DWindow::Tag()
{
  sWireClientWindow::Tag();
}

sBool s3DWindow::OnKey(sU32 key)
{
  sBool use = 0;
  if(QuakeMode)
  {
    switch(key & (sKEYQ_MASK|sKEYQ_BREAK))
    {
      case 'A':
      case 'a':             QuakeMask |= 1; use=1; break;
      case 'D':
      case 'd':             QuakeMask |= 2; use=1; break;
      case 'S':
      case 's':             QuakeMask |= 4; use=1; break;
      case 'W':
      case 'w':             QuakeMask |= 8; use=1; break;

      case 'Q':
      case 'q':             QuakeMask |= 16; use=1; break;
      case 'E':
      case 'e':             QuakeMask |= 32; use=1; break;

      case 'A'|sKEYQ_BREAK:
      case 'a'|sKEYQ_BREAK: QuakeMask &=~1; use=1; break;
      case 'D'|sKEYQ_BREAK:
      case 'd'|sKEYQ_BREAK: QuakeMask &=~2; use=1; break;
      case 'S'|sKEYQ_BREAK:
      case 's'|sKEYQ_BREAK: QuakeMask &=~4; use=1; break;
      case 'W'|sKEYQ_BREAK:
      case 'w'|sKEYQ_BREAK: QuakeMask &=~8; use=1; break;
      case 'Q'|sKEYQ_BREAK:
      case 'q'|sKEYQ_BREAK: QuakeMask &=~16; use=1; break;
      case 'E'|sKEYQ_BREAK:
      case 'e'|sKEYQ_BREAK: QuakeMask &=~32; use=1; break;

    }
  }
  if(!use)
    use = sWire->HandleKey(this,key); 
  return use;
}

void s3DWindow::OnDrag(const sWindowDrag &dd)
{
  if(QuakeMode)
  {
    sInt mx,my;
    sGetMouseHard(mx,my);
    RotY += (mx-OldMouseHardX)*0.002f; OldMouseHardX = mx;
    RotX += (my-OldMouseHardY)*0.002f; OldMouseHardY = my;
    sSetMouse(Client.CenterX(),Client.CenterY());
  }
  else
  {
    DragRay=MakeRay(dd.MouseX-Client.x0,dd.MouseY-Client.y0);
    sWire->HandleDrag(this,dd,OnCheckHit(dd));
  }
}

sRay s3DWindow::MakeRay(sInt x, sInt y)
{
  sRay ray;
  sF32 mx=2*sF32(x)/sF32(Client.SizeX())-1;
  sF32 my=-(2*sF32(y)/sF32(Client.SizeY())-1);
  View.MakeRay(mx,my,ray);
  return ray;
}

void s3DWindow::SetCam(const sMatrix34 &mat,sF32 zoom)
{
  Zoom = zoom;
  Pos = mat;
//  Focus = mat.l+mat.k;
  mat.FindEulerXYZ2(RotX,RotY,RotZ);
}

void s3DWindow::SetEnable(sBool enable)
{
  if(Enable!=enable)
  {
    if(enable)
    {
      Flags |= sWF_3D;
    }
    else
    {
      Flags &= ~sWF_3D;
    }
    Enable = enable;
    QuakeTime = sGetTime();
    Update();
  }
}

void s3DWindow::OnPaint2D()
{
  if(!Enable)
    sRect2D(Client,sGC_BACK);
}


void s3DWindow::PrepareView()
{
  View.Target = Client;
  View.Camera = Pos;
  View.Model.Init();
  View.SetTargetCurrent(&Client);
  View.SetZoom(Zoom);
  View.Prepare(sVUF_ALL);
}

void s3DWindow::QuakeCam()
{
  sInt time = sGetTime();
  
  {
    GearSpeed = sPow(2,GearShift*0.5f);
    sF32 ss = SideSpeed*GearSpeed;
    sF32 sf = ForeSpeed*GearSpeed;
    if(sGetKeyQualifier()&sKEYQ_SHIFT)
    {
      ss *= 10;
      sf *= 10;
    }
    else if(sGetKeyQualifier()&sKEYQ_CTRL)
    {
      ss *= 0.125f;
      sf *= 0.125f;
    }

    sVector30 force,speed;
    force.Init(0,0,0);

    if(sGetKeyQualifier()&sKEYQ_ALT)
    {
      if(QuakeMask & 1) RotY += ss*1000.0f;
      if(QuakeMask & 2) RotY -= ss*1000.0f;
      if (QuakeMask & 3)
      {
        sF32 dist = -(Pos.l - Focus).Length();
        Pos.EulerXYZ(RotX,RotY,RotZ);
        Pos.l = Focus + (Pos.k * dist);
      }
    }
    else
    {
      if(QuakeMask & 1) force -= Pos.i * ss;
      if(QuakeMask & 2) force += Pos.i * ss;
    }

    if(QuakeMask & 4) force -= Pos.k * sf;
    if(QuakeMask & 8) force += Pos.k * sf;

    if(QuakeMask & 16) force -= Pos.j * ss;
    if(QuakeMask & 32) force += Pos.j * ss;

    if(SpeedDamping>0)
    {
      speed = QuakeSpeed;
      for(sInt i=QuakeTime;i<time;i++)
      {
        speed += force;
//        speed = force*500.0f;
        speed = speed*SpeedDamping;
        Pos.l += speed;
      }
      QuakeSpeed = speed;
    } else
    { 
      for(sInt i=QuakeTime;i<time;i++)
      {
        QuakeSpeed += force;
//        QuakeSpeed = force*500.0f;
        Pos.l += QuakeSpeed;
      }
      QuakeSpeed.Init(0,0,0);// = 0.f;
    }

  }

  if(QuakeMode)
  {
    Pos.EulerXYZ(RotX,RotY,RotZ);
  }

  QuakeTime = time;
}

void s3DWindow::PrintGear(sPainter *p,sInt x,sInt &y)
{
  if(sGetTime()<GearShiftDisplay)
  {
    if(GearSpeed < 1.0f)
      p->PrintF(x,y,L"speed / %-5.2f",1.0f/GearSpeed);
    else
      p->PrintF(x,y,L"speed * %-5.2f",GearSpeed);
    y+=10;
  }
}

void s3DWindow::CmdQuakeCam()
{
  QuakeMode = !QuakeMode;
  QuakeMask = 0;
  QuakeSpeed.Init(0,0,0);
  sSetMouse(Client.CenterX(),Client.CenterY());
  sGetMouseHard(OldMouseHardX,OldMouseHardY);
//  MousePointer = QuakeMode ? sMP_OFF : sMP_ARROW;
}

void s3DWindow::CmdQuakeForwToggle(sDInt n)
{
  QuakeMask = (QuakeMask & ~(1<<3)) | ((n&1)<<3);
}

void s3DWindow::CmdQuakeBackToggle(sDInt n)
{
  QuakeMask = (QuakeMask & ~(1<<2)) | ((n&1)<<2);
}

void s3DWindow::CmdQuakeLeftToggle(sDInt n)
{
  QuakeMask = (QuakeMask & ~(1<<0)) | ((n&1)<<0);
}

void s3DWindow::CmdQuakeRightToggle(sDInt n)
{
  QuakeMask = (QuakeMask & ~(1<<1)) | ((n&1)<<1);
}


void s3DWindow::CmdQuakeUpToggle(sDInt n)
{
  QuakeMask = (QuakeMask & ~(1<<4)) | ((n&1)<<4);
}

void s3DWindow::CmdQuakeDownToggle(sDInt n)
{
  QuakeMask = (QuakeMask & ~(1<<5)) | ((n&1)<<5);
}

void s3DWindow::CmdGearShift(sDInt n)
{
  GearShift = sClamp(GearShift+sInt(n),-40,40);
  GearShiftDisplay = sGetTime()+500;
}

#define SCREENSHOTDIR L"c:/nxn/temp/screenshots"
#define SCREENSHOTNAME L"tool_"

void s3DWindow::OnPaint3D()
{
  if(Enable)
  {
    // initial setup

    QuakeCam();                   // quake cam
    PrepareView();                // viewport
  }

  sScreenMode sm;
  sGetScreenMode(sm);

  if(Enable && !Client.IsEmpty())
  {
    // repare rendertarget spec

    sTargetSpec spec;
#if sRENDERER==sRENDER_DX11       // blit back
    sInt xs = Client.SizeX();
    sInt ys = Client.SizeY();
    if(ColorRT==0 || xs!=ColorRT->SizeX || ys!=ColorRT->SizeY || sm.MultiLevel!=RTMultiLevel)
    {
      sDelete(ColorRT);
      sDelete(DepthRT);
      ColorRT = new sTexture2D(xs,ys,sTEX_2D|sTEX_ARGB8888|sTEX_RENDERTARGET|sTEX_MSAA,1);
      DepthRT = new sTexture2D(xs,ys,sTEX_2D|sTEX_DEPTH24|sTEX_RENDERTARGET|sTEX_MSAA,1);
      RTMultiLevel = sm.MultiLevel;
    }
    spec.Depth = DepthRT;
    spec.Color = ColorRT;
    spec.Window.Init(0,0,xs,ys);
#else
    spec.Init(Client);
#endif

    // painting

    Paint(View);                  // paint 3d
    Paint(View,spec);

    View.Model.Init();            // prepare for wireframe
    View.SetTargetCurrent(&Client);
    View.Prepare();

    GridUnit = 0;                 // pain t grid
    if(Grid)
    {
      sEnableGraphicsStats(0);
      sSetTarget(sTargetPara(0,0,spec));
      PaintGrid();
      sEnableGraphicsStats(1);
    }

    PaintWire(View);                // custom wireframe
    PaintWire(View,spec);

    // screenshots

    if(ScreenshotFlag)
    {
      const sU8 *data;
      sS32 pitch;
      sTextureFlags flags;
      sImage img;

      sRect r=spec.Window;
      img.Init(r.SizeX(),r.SizeY());
      img.Fill(0xffff0000);

      sBeginReadTexture(data,pitch,flags,spec.Color2D);
      if(flags==sTEX_ARGB8888)
      {
        data += r.x0*4 + r.y0*pitch;
        sU32 *dest = img.Data;
        for(sInt y=r.y0;y<r.y1;y++)
        {
          sCopyMem(dest,data,r.SizeX()*4);
          dest += img.SizeX;
          data += pitch;
        }
      }
      sEndReadTexture();

      if(flags==sTEX_ARGB8888)
      {
        sInt nr = 0;
        sArray<sDirEntry> dir;
        sDirEntry *ent;
        sInt len = sGetStringLen(SCREENSHOTNAME);
        if(sLoadDir(dir,SCREENSHOTDIR))
        {
          sFORALL(dir,ent)
          {
            if(sCmpStringILen(ent->Name,SCREENSHOTNAME,len)==0)
            {
              sInt newnr;
              const sChar *scan = ent->Name+len;
              if(sScanInt(scan,newnr))
                nr = sMax(nr,newnr);
            }
          }
        }
        else
        {
          sMakeDirAll(SCREENSHOTDIR);
        }
        sString<64> name;
        sSPrintF(name,SCREENSHOTDIR L"/" SCREENSHOTNAME L"%04d.bmp",nr+1);
        img.SaveBMP(name);
      }
      ScreenshotFlag = 0;
    }

    // backblit for dx11

#if sRENDERER==sRENDER_DX11
    if(ColorRT)
    {
      sCopyTexturePara ct;

      ct.Source = ColorRT;
      ct.Dest = sGetScreenColorBuffer();
      
      sRectRegion reg(sGui->Region3D);
      sRect *rp;
      reg.And(Client);
      sFORALL(reg.Rects,rp)
      {
        ct.SourceRect = ct.DestRect = *rp;
        ct.SourceRect.x0 -= Client.x0;
        ct.SourceRect.y0 -= Client.y0;
        ct.SourceRect.x1 -= Client.x0;
        ct.SourceRect.y1 -= Client.y0;

        sCopyTexture(ct);
      }
    }
#endif
  }

  // keep rolling

  if(Continuous)
    Update();
}


void s3DWindow::Lines(sVertexBasic *src,sInt linecount, sBool zon)
{
  sVertexBasic *vp;

  sEnableGraphicsStats(0);

  sCBuffer<sSimpleMaterialPara> cb; cb.Data->Set(View);
  if (zon)
    WireMtrl->Set(&cb);
  else
    WireMtrlNoZ->Set(&cb);

  WireGeo->BeginLoadVB(linecount*2,sGD_STREAM,(void **)&vp);
  sCopyMem(vp,src,sizeof(sVertexBasic)*linecount*2);
  WireGeo->EndLoadVB();
  WireGeo->Draw();

  sEnableGraphicsStats(1);
}


void s3DWindow::Circle(const sVector31 &center, const sVector30 &normal, sF32 radius, sU32 color, sBool zon, sInt segs)
{
  sVertexBasic *vp;

  sCBuffer<sSimpleMaterialPara> cb; cb.Data->Set(View);
  if (zon)
    WireMtrl->Set(&cb);
  else
    WireMtrlNoZ->Set(&cb);

  // make the two other vectors
  sVector30 v1(0,1,0);
  sVector30 v2; v2.Cross(normal,v1);
  if ((v2^v2)<0.01f) // singularity?
  {
    v1.Init(1,0,0);
    v2.Cross(normal,v1);
  }
  v2.Unit();
  v1.Cross(v2,normal);
  v1*=radius;
  v2*=radius;

  WireGeo->BeginLoadVB(segs*2,sGD_STREAM,(void **)&vp);

  sF32 s=0,c=1;
  for (sInt i=1; i<=segs; i++)
  {
    (vp++)->Init(center+s*v1+c*v2,color);
    sFSinCos(sPI2F*sF32(i)/sF32(segs),s,c);
    (vp++)->Init(center+s*v1+c*v2,color);
  }

  WireGeo->EndLoadVB();
  WireGeo->Draw();
}


void s3DWindow::PaintGrid()
{
  sVertexBasic *vp;
  const sInt count=21;
  sF32 dist;
  sF32 size;
  sU32 col,col1,col0;
  sVector30 delta;

  delta = Focus-Pos.l;
  dist = delta.Length();
  size = 20;
  if(dist>50) size = 200;
  if(dist>500) size = 2000;
  GridUnit = size/20;
  col1 = GridColor;
  col0 = (GridColor & 0xff000000) | ((GridColor & 0xfefefe)>>1);

  sCBuffer<sSimpleMaterialPara> cb; cb.Data->Set(View);
  WireMtrl->Set(&cb);
  WireGeo->BeginLoadVB(count*4,sGD_STREAM,(void **)&vp);
  for(sInt i=0;i<count;i++)
  {
    col = (i==count/2) ? col1 : col0;
    vp->px = i*size/(count-1)-size*0.5f;
    vp->py = 0.00f;
    vp->pz = -size*0.5f;
    vp->c0 = col;
    vp++;
    vp->px = i*size/(count-1)-size*0.5f;
    vp->py = 0.00f;
    vp->pz = +size*0.5f;
    vp->c0 = col;
    vp++;
  }
  for(sInt i=0;i<count;i++)
  {
    col = (i==count/2) ? col1 : col0;
    vp->px = -size*0.5f;
    vp->py = 0;
    vp->pz = i*size/(count-1)-size*0.5f;
    vp->c0 = col;
    vp++;
    vp->px = +size*0.5f;
    vp->py = 0;
    vp->pz = i*size/(count-1)-size*0.5f;
    vp->c0 = col;
    vp++;
  }
  WireGeo->EndLoadVB();
  WireGeo->Draw();
}

/****************************************************************************/

void s3DWindow::SetFocus(const sAABBox &bounds,const sVector31 &center)
{
  sF32 dist;
  sF32 largest;
  sVector30 delta;
  sVector31 v[8];
  
  Focus = center;

  largest = 0;
  bounds.MakePoints(v);
  for(sInt i=0;i<8;i++)
  {
    delta = v[i]-center;
    dist = delta.Length();
    if(dist > largest)
      largest = dist;
  }

  Pos.l = Focus + Pos.k*-((largest+View.ClipNear)*Zoom*1.2f);
}

void s3DWindow::CmdReset()
{
  Zoom = 1.5f;
  Focus.Init(0,0,0);
  Pos.Init();
  Pos.l.Init(0,0,-5);
  RotX = 0;
  RotY = 0;
  RotZ = 0;
  Grid = 1;
  QuakeMode = 0;
  Update();
  GearShift = 0;
  GearShiftDisplay = sGetTime()+500;
}

void s3DWindow::CmdGrid()
{
  Grid = !Grid;
  Update();
}

void s3DWindow::CmdScreenshot()
{
  ScreenshotFlag = 1;
  Update();
}

/****************************************************************************/

void s3DWindow::DragOrbit(const sWindowDrag &dd)
{
  switch(dd.Mode)
  {
  case sDD_START:
    DragPos = Pos;
    DragDist = (Pos.l - Focus)^Pos.k;
    DragRotX = RotX;
    DragRotY = RotY;
    break;
  case sDD_DRAG:
    DragDist = (Pos.l - Focus)^Pos.k;
    RotX = DragRotX + dd.DeltaY*0.01f;
    RotY = DragRotY + dd.DeltaX*0.01f;
    Pos.EulerXYZ(RotX,RotY,RotZ);
    Pos.l = (Pos.k * DragDist) + Focus;
    Update();
    break;
  }
}

void s3DWindow::DragRotate(const sWindowDrag &dd)
{
  sVector31 l;
  switch(dd.Mode)
  {
  case sDD_START:
    DragRotX = RotX;
    DragRotY = RotY;
    break;
  case sDD_DRAG:
    l = Pos.l;
    RotX = DragRotX + dd.HardDeltaY*0.01f;
    RotY = DragRotY + dd.HardDeltaX*0.01f;
    Pos.EulerXYZ(RotX,RotY,RotZ);
    Pos.l = l;
    Update();
    break;
  }
}
/*
void s3DWindow::DragTilt(const sWindowDrag &dd)
{
  sVector31 l;
  switch(dd.Mode)
  {
  case sDD_START:
    DragRotZ = RotZ;
    break;
  case sDD_DRAG:
    l = Pos.l;
    RotZ = DragRotZ + dd.HardDeltaX*0.01f;
    Pos.EulerXYZ(RotX,RotY,RotZ);
    Pos.l = l;
    Update();
    break;
  }
}
*/
void s3DWindow::DragTilt(const sWindowDrag &dd)
{
  sMatrix34 mat,tilt;
  sVector31 l;
  switch(dd.Mode)
  {
  case sDD_START:
    DragRotX = RotX;
    DragRotY = RotY;
    DragRotZ = RotZ;
    break;
  case sDD_DRAG:
    mat.EulerXYZ(DragRotX,DragRotY,DragRotZ);
    mat.l = Pos.l;
    tilt.EulerXYZ(0,0,-dd.HardDeltaX*0.01f);
    Pos = tilt * mat;
    Pos.FindEulerXYZ2(RotX,RotY,RotZ);
    Pos.EulerXYZ(RotX,RotY,RotZ);

    Update();
    break;
  }
}

void s3DWindow::CmdResetTilt()
{
  sMatrix34 mat;

  mat.LookAt(Pos.l+Pos.k,Pos.l);
  mat.FindEulerXYZ2(RotX,RotY,RotZ);
  Pos.EulerXYZ(RotX,RotY,RotZ);
  Update();
}

void s3DWindow::DragMove(const sWindowDrag &dd)
{
  sF32 speed = sAbs(((Focus - Pos.l)^Pos.k) / Client.SizeX() * 2);

  switch(dd.Mode)
  {
  case sDD_START:
    DragPos = Pos;
    break;
  case sDD_DRAG:
    Pos.l = DragPos.l - (DragPos.i * dd.DeltaX*speed)
                      + (DragPos.j * dd.DeltaY*speed);
    Update();
    break;
  case sDD_STOP:
    Pos.l = DragPos.l - (DragPos.i * dd.DeltaX*speed)
                      + (DragPos.j * dd.DeltaY*speed);
    Focus = Focus - (DragPos.i * dd.DeltaX*speed)
                  + (DragPos.j * dd.DeltaY*speed);
    Update();
    break;
  }
}

void s3DWindow::DragZoom(const sWindowDrag &dd)
{
  switch(dd.Mode)
  {
  case sDD_START:
    DragDist = Zoom;
    break;
  case sDD_DRAG:
    Zoom = sFExp(sFLog(sMax<sF64>(DragDist,0))+dd.DeltaY*0.01f);
    Update();
    break;
  }
}

void s3DWindow::DragDolly(const sWindowDrag &dd)
{
  switch(dd.Mode)
  {
  case sDD_START:
    DragPos = Pos;
    DragDist = (Focus-Pos.l)^Pos.k;
    break;
  case sDD_DRAG:
    Pos.l = Focus - Pos.k*sFExp(sFLog(sMax<sF64>(DragDist,0))-dd.DeltaY*0.01f-dd.DeltaX*0.01f);
    Update();
    break;
  }
}

/****************************************************************************/
