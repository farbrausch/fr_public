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

/****************************************************************************/

// initialize resources
 
MyApp::MyApp()
{
  // adapter and screen names 

  sGraphicsCaps caps;
  sGetGraphicsCaps(caps);
  AdapterName=caps.AdapterName;

  sInt max = sGetScreenCount();
  ScreenNames.HintSize(max);
  for (sInt i=0; i<max; i++)
  {
    sScreenInfo info;
    sGetScreenInfo(info,0,i);
    ScreenNames.AddTail(info.MonitorName);
  }

  // debug output

  Painter = new sPainter;

  // geometry 

  Geo = new sGeometry();
  Geo->Init(sGF_TRILIST|sGF_INDEX16,sVertexFormatStandard);
  Geo->LoadCube(-1,2,2,2);

  // texture

  sImage img(64,64);
  img.Checker(0xffff8080,0xff80ff80,8,8);
  img.Glow(0.5f,0.5f,0.25f,0.25f,0xffffffff,1.0f,4.0f);
  Tex = sLoadTexture2D(&img,sTEX_2D|sTEX_ARGB8888);

  // material

  Mtrl = new sSimpleMaterial;
  Mtrl->Flags = sMTRL_ZON | sMTRL_CULLON;
  Mtrl->Flags |= sMTRL_LIGHTING;
  Mtrl->Texture[0] = Tex;
  Mtrl->TFlags[0] = sMTF_LEVEL2|sMTF_CLAMP;
  Mtrl->Prepare(Geo->GetFormat());

  // light

  sClear(Env);
  Env.AmbientColor  = 0x00202020;
  Env.LightColor[0] = 0x00ffffff;
  Env.LightDir[0].Init(0,0,1);
  Env.Fix();
}

// free resources (memory leaks are evil)

MyApp::~MyApp()
{
  delete Painter;
  delete Geo;
  delete Mtrl;
  delete Tex;
}

/****************************************************************************/

// paint a frame

void MyApp::PaintScreen(sInt screen,sInt time)
{ 
  time += screen*1234;

  // set rendertarget

  static sU32 colors[4] = { 0xff405060,0xffc04040,0xff40c040,0xff4040c0 };

  sTargetPara tp(sST_CLEARALL,colors[screen&3],0,sGetScreenColorBuffer(screen),sGetScreenDepthBuffer(screen));

  sSetTarget(tp);

  // set camera

  View.SetTargetCurrent();
  View.SetZoom(1.0f);
  View.Model.EulerXYZ(time*0.0011f,time*0.0013f,time*0.0015f);
  View.Model.l.Init(0,0,0);
  View.Camera.l.Init(0,0,-4);
  View.Prepare();

  // set material

  sCBuffer<sSimpleMaterialEnvPara> cb;
  cb.Data->Set(View,Env);
  Mtrl->Set(&cb);

  // draw

  Geo->Draw();

  // debug

  sF32 avg = Timer.GetAverageDelta();
  Painter->SetTarget();
  Painter->Begin();
  Painter->SetPrint(0,0xff000000,2);
  Painter->SetPrint(0,~0,2);
  Painter->PrintF(10,10,L"screen %d: %s",screen,ScreenNames[screen]);
  if (screen)
    Painter->PrintF(10,30,L"on %s",AdapterName);
  else
    Painter->PrintF(10,30,L"%5.2ffps %5.3fms",1000.0f/avg,avg);
  Painter->End();

  // resolve multisampling

  sResolveTarget();
}

/****************************************************************************/

void MyApp::OnPaint3D()
{

  // get timing

  Timer.OnFrame(sGetTime());
  sInt time = Timer.GetTime();

  // paint

  sInt max = sGetScreenCount();
  for(sInt i=0;i<max;i++)
    PaintScreen(i,time);

}

/****************************************************************************/

// abort program when escape is pressed

void MyApp::OnInput(const sInput2Event &ie)
{
  if((ie.Key&sKEYQ_MASK)==sKEY_ESCAPE) sExit();
}

/****************************************************************************/

// register application class

void sMain()
{
  sSetWindowName(L"Multihead");

  sScreenMode sm;

  sm.Clear();
  sm.Flags = sSM_VALID|sSM_NOVSYNC|sSM_MULTISCREEN|sSM_FULLSCREEN;
//  sm.Flags = sSM_VALID|sSM_NOVSYNC|sSM_MULTISCREEN;
  sm.Display = 0;
  sm.ScreenX = 800;
  sm.ScreenY = 600;
  sm.Aspect = sF32(sm.ScreenX)/sF32(sm.ScreenY);
  sm.MultiLevel = 255;

  sSetScreenMode(sm);
  sInit(sISF_3D|sISF_CONTINUOUS,0,0);
  sSetApp(new MyApp());
}

/****************************************************************************/

