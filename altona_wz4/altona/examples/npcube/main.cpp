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
  // debug output
 
  Painter = new sPainter;

  // geometry 

  Geo = new sGeometry(sGF_TRILIST|sGF_INDEX16,sVertexFormatStandard);

  // load vertices and indices

  sVertexStandard *vp=0;
  Geo->BeginLoadVB(24,sGD_STATIC,&vp);

  vp->Init(-1, 1,-1,  0, 0,-1, 1,0); vp++; // 3
  vp->Init( 1, 1,-1,  0, 0,-1, 1,1); vp++; // 2
  vp->Init( 1,-1,-1,  0, 0,-1, 0,1); vp++; // 1
  vp->Init(-1,-1,-1,  0, 0,-1, 0,0); vp++; // 0

  vp->Init(-1,-1, 1,  0, 0, 1, 1,0); vp++; // 4
  vp->Init( 1,-1, 1,  0, 0, 1, 1,1); vp++; // 5
  vp->Init( 1, 1, 1,  0, 0, 1, 0,1); vp++; // 6
  vp->Init(-1, 1, 1,  0, 0, 1, 0,0); vp++; // 7

  vp->Init(-1,-1,-1,  0,-1, 0, 1,0); vp++; // 0
  vp->Init( 1,-1,-1,  0,-1, 0, 1,1); vp++; // 1
  vp->Init( 1,-1, 1,  0,-1, 0, 0,1); vp++; // 5
  vp->Init(-1,-1, 1,  0,-1, 0, 0,0); vp++; // 4

  vp->Init( 1,-1,-1,  1, 0, 0, 1,0); vp++; // 1
  vp->Init( 1, 1,-1,  1, 0, 0, 1,1); vp++; // 2
  vp->Init( 1, 1, 1,  1, 0, 0, 0,1); vp++; // 6
  vp->Init( 1,-1, 1,  1, 0, 0, 0,0); vp++; // 5

  vp->Init( 1, 1,-1,  0, 1, 0, 1,0); vp++; // 2
  vp->Init(-1, 1,-1,  0, 1, 0, 1,1); vp++; // 3
  vp->Init(-1, 1, 1,  0, 1, 0, 0,1); vp++; // 7
  vp->Init( 1, 1, 1,  0, 1, 0, 0,0); vp++; // 6

  vp->Init(-1, 1,-1, -1, 0, 0, 1,0); vp++; // 3
  vp->Init(-1,-1,-1, -1, 0, 0, 1,1); vp++; // 0
  vp->Init(-1,-1, 1, -1, 0, 0, 0,1); vp++; // 4
  vp->Init(-1, 1, 1, -1, 0, 0, 0,0); vp++; // 7
  Geo->EndLoadVB();

  sU16 *ip=0;
  Geo->BeginLoadIB(6*6,sGD_STATIC,&ip);
  for(sInt i=0;i<6;i++)
    sQuad(ip,i*4,0,1,2,3);
  Geo->EndLoadIB();

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
  Mtrl->Prepare(sVertexFormatStandard);

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

void MyApp::OnPaint3D()
{ 
  // set rendertarget

  sSetTarget(sTargetPara(sST_CLEARALL,0xff405060));

  // get timing

  Timer.OnFrame(sGetTime());
  static sInt time;
  if(sHasWindowFocus())
    time = Timer.GetTime();

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

  // debug output

  sF32 avg = Timer.GetAverageDelta();
  Painter->SetTarget();
  Painter->Begin();
  Painter->SetPrint(0,0xff000000,2);
  Painter->SetPrint(0,~0,2);
  Painter->PrintF(10,10,L"%5.2ffps %5.3fms",1000.0f/avg,avg);
  Painter->End();
}

/****************************************************************************/

// abort program when escape is pressed

void MyApp::OnInput(const sInput2Event &ie)
{

}

/****************************************************************************/

// register application class
void sMain()
{
  sSetWindowName(L"Cube");

//  sInit(sISF_3D|sISF_CONTINUOUS|sISF_NOVSYNC/*|sISF_FSAA*//*|sISF_FULLSCREEN*/,640,480);
  sInit(sISF_3D|sISF_CONTINUOUS|/*sISF_NOVSYNC|*/sISF_FSAA/*|sISF_FULLSCREEN*/,640,480);
//  sInit(sISF_3D|sISF_CONTINUOUS|sISF_NOVSYNC|sISF_FSAA|sISF_FULLSCREEN,640,480);
  sSetApp(new MyApp());
}

/****************************************************************************/
