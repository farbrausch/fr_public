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
#include "util/ipp.hpp"

/****************************************************************************/

// initialize resources
 
MyApp::MyApp()
{
  // debug output

  Painter = new sPainter;

  // geometry 

  Geo = new sGeometry();
  Geo->Init(sGF_TRILIST|sGF_INDEX16,sVertexFormatStandard);

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

  // ipp resources

  DummyTex = new sTexture2D(8,8,sTEX_2D|sTEX_ARGB8888,1);

  BlitGeo = new sGeometry(sGF_QUADLIST,sVertexFormatSingle);

  BlitMtrl = new sSimpleMaterial;
  BlitMtrl->Flags = sMTRL_ZOFF | sMTRL_CULLOFF;
  BlitMtrl->Texture[0] = DummyTex;
  BlitMtrl->TFlags[0] = sMTF_LEVEL0|sMTF_CLAMP;
  BlitMtrl->Prepare(sVertexFormatSingle);
}

// free resources (memory leaks are evil)

MyApp::~MyApp()
{
  delete Painter;
  delete Geo;
  delete Mtrl;
  delete Tex;
  
  delete DummyTex;
  delete BlitGeo;
  delete BlitMtrl;
}

/****************************************************************************/

// paint a frame

void MyApp::OnPaint3D()
{ 
  sBool ippz = !(sGetKeyQualifier()&sKEYQ_SHIFT);

  // set rendertarget

  sSetTarget(sTargetPara(ippz ? sST_CLEARALL|sST_READZ : sST_CLEARALL,0xff405060));

  // render cube

  RenderCube();

  // do image post processing

  if(ippz)
  {
    sRTMan->SetScreen(0);
    IppZRead();
    sRTMan->FinishScreen();
  }

  // debug output

  sSetTarget(sTargetPara(sST_NOMSAA,0));

  sF32 avg = Timer.GetAverageDelta();
  Painter->SetTarget();
  Painter->Begin();
  Painter->SetPrint(0,0xff000000,2);
  Painter->SetPrint(0,0xff808080,2);
  Painter->PrintF(10,10,L"%5.2ffps %5.3fms",1000/avg,avg);
  Painter->PrintF(10,30,L"press shift");  
  Painter->End();
}

void MyApp::RenderCube()
{
  // get timing


  static sInt time;
  if(!(sGetKeyQualifier()&sKEYQ_CTRL))
  {
    Timer.OnFrame(sGetTime());
    if(sHasWindowFocus())
      time = Timer.GetTime();
  }

  // set camera

  View.SetTargetCurrent();
  View.SetZoom(1.0f);
  View.Model.EulerXYZ(time*0.00011f,time*0.00013f,time*0.00015f);
  View.Model.l.Init(0,0,0);
  View.Camera.l.Init(0,0,-4);
  View.ClipNear = 2.0f;
  View.ClipFar = 16.0f;
  View.Prepare();

  // set material

  sCBuffer<sSimpleMaterialEnvPara> cb;
  cb.Data->Set(View,Env);
  Mtrl->Set(&cb);

  // load vertices and indices

  sVertexStandard *vp=0;

  Geo->BeginLoadVB(24,sGD_STREAM,&vp);
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
  Geo->BeginLoadIB(6*6,sGD_STREAM,&ip);
  sQuad(ip,0, 0, 1, 2, 3);
  sQuad(ip,0, 4, 5, 6, 7);
  sQuad(ip,0, 8, 9,10,11);
  sQuad(ip,0,12,13,14,15);
  sQuad(ip,0,16,17,18,19);
  sQuad(ip,0,20,21,22,23);
  Geo->EndLoadIB();

  // draw

  Geo->Draw();
}

/****************************************************************************/

void MyApp::IppZRead()
{
  sTexture2D *dest = sRTMan->WriteScreen(1);

  Blit(dest,sGetScreenDepthBuffer());

  sRTMan->Release(dest);
}

void MyApp::Blit(sTexture2D *d,sTexture2D *s)
{
  sVertexSingle *vp;
  sViewport view;

#if sRENDERER==sRENDER_DX9
  sRTMan->SetTarget(d,0,0,s);       // this is very bad. i have to acutally bind the zbuffer in order to use it as a texture! (INTZ, ati4870)
#else
  sRTMan->SetTarget(d,0,0,0);
#endif
  view.SetTargetCurrent();
  view.Orthogonal = sVO_SCREEN;
  view.Prepare();
  sCBuffer<sSimpleMaterialPara> cb;
  cb.Data->Set(view);
  BlitMtrl->Texture[0] = s;
  BlitMtrl->Set(&cb);

  sF32 fu = 0.5f/s->SizeX;
  sF32 fv = 0.5f/s->SizeY;

  BlitGeo->BeginLoadVB(4,sGD_STREAM,&vp);
  vp[0].Init(0,0,0,~0,0+fu,0+fv);
  vp[1].Init(1,0,0,~0,1+fu,0+fv);
  vp[2].Init(1,1,0,~0,1+fu,1+fv);
  vp[3].Init(0,1,0,~0,0+fu,1+fv);
  BlitGeo->EndLoadVB();
  BlitGeo->Draw();

  BlitMtrl->Texture[0] = DummyTex;
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
  sSetWindowName(L"ipp z buffer read");

  sScreenMode sm;
  sm.Clear();
  sm.Flags = sSM_READZ|sSM_NOVSYNC|sSM_VALID;
  sm.MultiLevel = 255;
  sSetScreenMode(sm);


  sInit(sISF_3D|sISF_CONTINUOUS,1024,768);
  sSetApp(new MyApp());
}

/****************************************************************************/

