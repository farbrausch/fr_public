/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#define sPEDANTIC_OBSOLETE 1
#define sPEDANTIC_WARN 1

#include "main.hpp"
#include "base/windows.hpp"
#include "util/image.hpp"
#include "util/shaders.hpp"
#include "image_win.hpp"

#define VERSION 1
#define REVISION 2

/****************************************************************************/

// initialize resources
 
MyApp::MyApp()
{
  LoadEntries();
  ImageId = 7;
  Tex = 0;
  TexOld = 0;
  MouseX = 0;
  MouseY = 0;
  MouseB = 0;

  Help = 0;
  Linear = 0;
  FixZoom = 0;

  FadeTimer = 0;
  FadeMax = 30;

  LoadMode = 0;

  CmdReset();
  
  // debug output
 
  Painter = new sPainter;

  // geometry 

  Geo = new sGeometry(sGF_QUADLIST,sVertexFormatSingle);

  // texture

  sImage img(64,64);
  img.Checker(0xffff8080,0xff80ff80,8,8);
  img.Glow(0.5f,0.5f,0.25f,0.25f,0xffffffff,1.0f,4.0f);
  Tex = sLoadTexture2D(&img,sTEX_2D|sTEX_ARGB8888);

  // material

  Mtrl = new sSimpleMaterial;
  Mtrl->Flags = sMTRL_ZOFF | sMTRL_CULLOFF;
  Mtrl->Texture[0] = Tex;
  Mtrl->TFlags[0] = sMTF_LEVEL0|sMTF_CLAMP;
  Mtrl->BlendColor = sMBS_F|sMBO_ADD|sMBD_1;
  Mtrl->InitVariants(2);
  Mtrl->SetVariant(0);
  Mtrl->TFlags[0] = sMTF_LEVEL2|sMTF_CLAMP;
  Mtrl->SetVariant(1);
  Mtrl->Prepare(sVertexFormatSingle);

  CursorMtrl = new sSimpleMaterial;
  CursorMtrl->Flags = sMTRL_ZOFF | sMTRL_CULLOFF;
  CursorMtrl->Prepare(sVertexFormatSingle);


  // light

  sClear(Env);
  Env.AmbientColor  = 0x00202020;
  Env.LightColor[0] = 0x00ffffff;
  Env.LightDir[0].Init(0,0,1);
  Env.Fix();

  // ready

}

// free resources (memory leaks are evil)

MyApp::~MyApp()
{
  Entry *ent;
  sFORALL(Entries,ent)
  {
    sDelete(ent->Texture);
  }
  sDeleteAll(Entries);
  
  delete Painter;
  delete Geo;
  delete Mtrl;
  delete Tex;
  delete CursorMtrl;
}

/****************************************************************************/

// paint a frame
sInt sGetFrameTime();

void MyApp::OnPaint3D()
{ 
  // set rendertarget

  LoadOne();
  sSetTarget(sTargetPara(sST_CLEARALL,0xff000000));

  // get timing

  Timer.OnFrame(sGetTime());
  static sInt time;
  if(sHasWindowFocus())
  {
#if  sRENDERER==sRENDER_DX11
      time = sGetFrameTime();
#else
      time = Timer.GetTime();
#endif
  }

  // set camera

  View.SetTargetCurrent();
  View.SetZoom(1.0f);
  View.Orthogonal = sVO_PIXELS;
  View.Prepare();

  // set geometry

  sSetMouse(sInt(ScreenX/2),sInt(ScreenY/2));
  static sInt oldmb = 0;
  static sInt oldmx = 0;
  static sInt oldmy = 0;
  sF32 sx = sF32(MouseX-oldmx);
  sF32 sy = sF32(MouseY-oldmy);
  sF32 damps = 0.85f;
  if(MouseB&1)
  {
    ScrollSpeedX = sFade(damps,sx,ScrollSpeedX);
    ScrollSpeedY = sFade(damps,sy,ScrollSpeedY);
  }
  else
  {
    ScrollSpeedX *= damps;
    ScrollSpeedY *= damps;
    if(sFAbs(ScrollSpeedX)+sFAbs(ScrollSpeedY)<0.1f)
      ScrollSpeedX = ScrollSpeedY = 0;
  }

  Pos.x0 += ScrollSpeedX;
  Pos.y0 += ScrollSpeedY;
  Pos.x1 += ScrollSpeedX;
  Pos.y1 += ScrollSpeedY;
  oldmx = MouseX;
  oldmy = MouseY;
  if(MouseHide>0)
    MouseHide--;


  if(FadeTimer>0)
  {
    FadeTimer--;
    if(FadeTimer == 0)
    {
/*
      sDelete(Tex);
      Tex = TexNext;
      TexNext = 0;
      */
    }
  }


  if(sFAbs(Zoom-ZoomTarget)>0.01f)
    Zoom = sFade(0.1f,Zoom,ZoomTarget);
  else
    Zoom = ZoomTarget ;


  sF32 oldz = sPow(2,ZoomOld);
  sF32 newz = sPow(2,Zoom);

  if(FixZoom==2)
  {
    newz = sRoundNear(newz);
    ZoomOld = sLog2(newz);
  }
  else
  {
    ZoomOld = Zoom;
  }


  if(newz!=oldz)
  {
    Pos.x0 = (Pos.x0-MouseX)*newz/oldz+MouseX;
    Pos.y0 = (Pos.y0-MouseY)*newz/oldz+MouseY;
    Pos.x1 = Pos.x0 + ImageX*newz;
    Pos.y1 = Pos.y0 + ImageY*newz;
  }
  sFRect bnd;
  bnd.x0 = (ScreenX-ImageX*newz)/2;
  bnd.y0 = (ScreenY-ImageY*newz)/2;
  bnd.x1 = Pos.x0 + ImageX*newz;
  bnd.y1 = Pos.y0 + ImageY*newz;

  sFRect pos(Pos);
  pos.x0 -= 0.5f;
  pos.y0 -= 0.5f;
  pos.x1 -= 0.5f;
  pos.y1 -= 0.5f;

  sVertexSingle *vp;
  sCBuffer<sSimpleMaterialEnvPara> cb;
  sCBufferBase *cbp = &cb;
  cb.Data->Set(View,Env);

  // current Texture

  GeoPos = pos;
  Geo->BeginLoadVB(4,sGD_STREAM,&vp);
  vp[0].Init(pos.x0,pos.y0,0,0xffffffff,0,0);
  vp[1].Init(pos.x0,pos.y1,0,0xffffffff,0,1);
  vp[2].Init(pos.x1,pos.y1,0,0xffffffff,1,1);
  vp[3].Init(pos.x1,pos.y0,0,0xffffffff,1,0);
  Geo->EndLoadVB();

  Mtrl->Texture[0] = Tex;
  Mtrl->Set(&cbp,1,Linear);

  sGeometryDrawInfo di;
  sVector4 bf;
  bf.x = bf.y = bf.z = bf.w = 1.0f-sF32(FadeTimer)/sF32(FadeMax);
  di.BlendFactor = bf.GetColor();
  di.Flags = sGDI_BlendFactor;
  Geo->Draw(di);

  if(FadeTimer>0)
  {
    Geo->BeginLoadVB(4,sGD_STREAM,&vp);
    vp[0].Init(GeoPosOld.x0,GeoPosOld.y0,0,0xffffffff,0,0);
    vp[1].Init(GeoPosOld.x0,GeoPosOld.y1,0,0xffffffff,0,1);
    vp[2].Init(GeoPosOld.x1,GeoPosOld.y1,0,0xffffffff,1,1);
    vp[3].Init(GeoPosOld.x1,GeoPosOld.y0,0,0xffffffff,1,0);
    Geo->EndLoadVB();

    Mtrl->Texture[0] = TexOld;
    Mtrl->Set(&cbp,1,Linear);

    bf.x = bf.y = bf.z = bf.w = sF32(FadeTimer)/sF32(FadeMax);
    di.BlendFactor = bf.GetColor();
    di.Flags = sGDI_BlendFactor;
    Geo->Draw(di);
  }

  // cursor

  if(MouseHide>0)
  {
    sU32 ccol = 0xffffffff;
    sF32 r = 20;
    Geo->BeginLoadVB(8,sGD_STREAM,&vp);
    vp[0].Init(MouseX-1.0f,MouseY-r,0,ccol,0,0);
    vp[1].Init(MouseX-1.0f,MouseY+r,0,ccol,0,0);
    vp[2].Init(MouseX+1.0f,MouseY+r,0,ccol,0,0);
    vp[3].Init(MouseX+1.0f,MouseY-r,0,ccol,0,0);
    vp[4].Init(MouseX-r,MouseY-1.0f,0,ccol,0,0);
    vp[5].Init(MouseX-r,MouseY+1.0f,0,ccol,0,0);
    vp[6].Init(MouseX+r,MouseY+1.0f,0,ccol,0,0);
    vp[7].Init(MouseX+r,MouseY-1.0f,0,ccol,0,0);
    Geo->EndLoadVB();

    CursorMtrl->Set(&cb);
    Geo->Draw();
  }

  // debug output

  if(Help || LoadMode>=0)
  {
    sTextBuffer tb;
    static const sChar *mode[2] = {L"off",L"on "};
    static const sChar *zmode[3] = {L"smooth",L"initial picture integral zoom",L"only integral zoom"};
    tb.PrintF(L"CompoView V%d.%d\n",VERSION,REVISION);

//    sF32 avg = Timer.GetAverageDelta();
//    tb.PrintF(L"%5.2ffps %5.3fms\n",1000.0f/avg,avg);

    if(LoadMode>=0)
    {
      tb.PrintF(L"loading %d of %d\n",LoadMode,dir.GetCount());
      tb.PrintF(L"press ESCAPE to stop loading\n\n");
    }
    else
    {
      tb.PrintF(L"Name: %s\n\n",Name);
    }
    sPtr free = sGetMemHandler(sAMF_HEAP)->GetFree()/(1024*1024);
    tb.PrintF(L"Memory Free: %dMB %s",free,free<100 ? L"(LOW)" : L"");
    tb.PrintF(L"\nModes:\n");
    tb.PrintF(L"F1 (%s) Help\n",mode[Help]);
    tb.PrintF(L"F2 (%s) Linear Interpolation\n",mode[Linear]);
    tb.PrintF(L"F3 (%s) Zoom\n",zmode[FixZoom]);
    tb.PrintF(L"\nActions:\n");
    tb.PrintF(L"Drag with left mouse button: scroll\n");
    tb.PrintF(L"Mousewheel: zoom\n");
    tb.PrintF(L"SHIFT-ESCAPE: exit\n");
    tb.PrintF(L"ALT-F4: exit\n");
    tb.PrintF(L"SPACE: zoom out\n");
    tb.PrintF(L"r: zoom out proper\n");


    Painter->SetTarget();
    Painter->Begin();
    /*
    Painter->SetPrint(0,0x80000000,2);
    Painter->Print(11,11,tb.Get());   
    Painter->Print( 9, 9,tb.Get());   
    Painter->Print(11, 9,tb.Get());   
    Painter->Print( 9,11,tb.Get());   
    */
    Painter->SetPrint(0,0xff000000,2);
    Painter->Print(11,10,tb.Get());   
    Painter->Print(10,11,tb.Get());   
    Painter->Print( 9,10,tb.Get());   
    Painter->Print(10, 9,tb.Get());   
    Painter->SetPrint(0,0xffffffff,2);
    Painter->Print(10,10,tb.Get());   
    Painter->End();
  }
}

/****************************************************************************/

// abort program when escape is pressed

void MyApp::OnInput(const sInput2Event &ie)
{
  sInt max = Entries.GetCount();
  switch(ie.Key & (sKEYQ_MASK | sKEYQ_BREAK | sKEYQ_SHIFTL | sKEYQ_SHIFTR | sKEYQ_ALT))
  {
  case sKEY_ESCAPE|sKEYQ_SHIFTL:
  case sKEY_ESCAPE|sKEYQ_SHIFTR:
  case sKEY_F4|sKEYQ_ALT:
    sExit();
    break;
  case sKEY_RIGHT:
    GoImage((ImageId+1)%max);
    break;
  case sKEY_LEFT:
    GoImage((ImageId+max-1)%max);
    break;
  case sKEY_MOUSEHARD:
    MouseX = sClamp<sInt>(MouseX+sS16((ie.Payload>>16)&0xffff),0,sInt(ScreenX));
    MouseY = sClamp<sInt>(MouseY+sS16(ie.Payload & 0xffff),0,sInt(ScreenY));
    MouseHide = 20;
    break;
  case sKEY_WHEELDOWN:
    ZoomTarget = sClamp<sF32>(ZoomTarget-0.25f,-4.0f,20.0f);
    break;
  case sKEY_WHEELUP:
    ZoomTarget = sClamp<sF32>(ZoomTarget+0.25f,-4.0f,20.0f);
    break;
/*
  case sKEY_SPACE:
    ZoomTarget = 0;
    break;
    */
  case 'r':
  case sKEY_SPACE:
    CmdReset();
    break;
  case sKEY_LMB: MouseB |= 1; break;
  case sKEY_RMB: MouseB |= 2; break;
  case sKEY_MMB: MouseB |= 4; break;
  case sKEY_LMB|sKEYQ_BREAK: MouseB &= ~1; break;
  case sKEY_RMB|sKEYQ_BREAK: MouseB &= ~2; break;
  case sKEY_MMB|sKEYQ_BREAK: MouseB &= ~4; break;

  case sKEY_F1:
    Help = !Help;
    break;
  case sKEY_F2:
    Linear = !Linear;
    break;
  case sKEY_F3:
    FixZoom = (FixZoom+1)%3;
    break;
  case sKEY_ESCAPE:
    if(LoadMode>2)
    {
       LoadMode = -1;
       GoImage(0);
    }
    break;
  }
}

/****************************************************************************/

void MyApp::CmdReset()
{
  sInt x,y;
  sGetScreenSize(x,y);
  ScreenX = sF32(x);
  ScreenY = sF32(y);
  MouseX = sInt(ScreenX/2);
  MouseY = sInt(ScreenY/2);

  Zoom = 0;

  if(ImageX>ScreenX || ImageY>ScreenY)
  {
    Zoom = sLog2(sMin(ScreenX/ImageX,ScreenY/ImageY));
  }
  else
  {
    if(FixZoom==0)
    {
      Zoom = sLog2(sMin(ScreenX/ImageX,ScreenY/ImageY));
    }
    else
    {
      if(ImageX>2)
        while(ImageX*sPow(2,Zoom)*2<=ScreenX)
          Zoom++;
    }
  }
  ZoomOld = ZoomTarget = Zoom;

  Pos.x0 = (ScreenX-ImageX*sPow(2,Zoom))/2;
  Pos.y0 = (ScreenY-ImageY*sPow(2,Zoom))/2;
  Pos.x1 = Pos.x0 + ImageX*sPow(2,Zoom);
  Pos.y1 = Pos.y0 + ImageY*sPow(2,Zoom);
  MouseHide = 0;
}

void MyApp::GoImage(sInt id)
{
  if(Entries.GetCount()==0)
    sFatal(L"no bmp, png or jpg images found");
  ImageId = id;
  sDelete(TexOld);
  TexOld = Tex;
  sU32 flags = sTEX_2D|sTEX_ARGB8888;
  sInt sx, sy;
  sGetScreenSize(sx,sy);
  sImage *img = &Entries[ImageId]->Image;
  if (img->SizeX <= sx && img->SizeY <= sy) flags |= sTEX_NOMIPMAPS;
  Tex = sLoadTexture2D(img,flags);
  GeoPosOld = GeoPos;

  ImageX = (sF32)(Tex->SizeX);
  ImageY = (sF32)(Tex->SizeY);
  CmdReset();
  FadeTimer = FadeMax;

  Name = Entries[ImageId]->Name;
}

void MyApp::LoadEntries()
{
  sLoadDir(dir,L".");
}

void MyApp::LoadOne()
{
  if(LoadMode>=0 && LoadMode==dir.GetCount())
  {
    sSortUp(Entries,&Entry::Name);
    LoadMode = -1;
    GoImage(0);
  }
  else if(LoadMode>=0)
  {
    sDirEntry *de;
    Entry *ent;

    de = &dir[LoadMode++];

    const sChar *ext = sFindFileExtension(de->Name);
    if(sCmpStringI(ext,L"png")==0 || sCmpStringI(ext,L"jpg")==0 || sCmpStringI(ext,L"bmp")==0)
    {
      ent = new Entry;
      sDPrintF(L"load %q\n",de->Name);
      if(!ent->Image.Load(de->Name))
      {
        if (!sLoadImageWin32(de->Name, ent->Image))
          sFatal(L"failed to load %q.\ntry to convert this image to a PNG with standard settings",de->Name);
      }
      ent->Name = de->Name;
      ent->Texture = 0;
      Entries.AddTail(ent);
    }
  }
}

/****************************************************************************/

// register application class

void sMain()
{
  sString<128> buffer;
  buffer.PrintF(L"CompoView V%d.%d",VERSION,REVISION);
  sSetWindowName(buffer);

  sInt flags = sISF_3D|sISF_CONTINUOUS;
  flags |= sISF_FULLSCREENIFRELEASE;
//  flags |= sISF_FSAA;
//  flags |= sISF_NOVSYNC;

  sScreenInfo si;
  sGetScreenInfo(si,0,-1);
  

  sInit(flags,si.CurrentXSize,si.CurrentYSize);
  sSetApp(new MyApp());
}

/****************************************************************************/

