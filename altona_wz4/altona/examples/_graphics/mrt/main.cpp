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
#include "shader.hpp"
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

  CubeGeo = new sGeometry();
  CubeGeo->Init(sGF_TRILIST|sGF_INDEX16,sVertexFormatStandard);

  // load vertices and indices

  sVertexStandard *vp=0;

  CubeGeo->BeginLoadVB(24,sGD_STATIC,&vp);
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
  CubeGeo->EndLoadVB();

  sU16 *ip=0;
  CubeGeo->BeginLoadIB(6*6,sGD_STATIC,&ip);
  sQuad(ip,0, 0, 1, 2, 3);
  sQuad(ip,0, 4, 5, 6, 7);
  sQuad(ip,0, 8, 9,10,11);
  sQuad(ip,0,12,13,14,15);
  sQuad(ip,0,16,17,18,19);
  sQuad(ip,0,20,21,22,23);
  CubeGeo->EndLoadIB();

  // texture

  sImage img(64,64);
  img.Checker(0xffff8080,0xff80ff80,8,8);
  img.Glow(0.5f,0.5f,0.25f,0.25f,0xffffffff,1.0f,4.0f);
  Tex = sLoadTexture2D(&img,sTEX_2D|sTEX_ARGB8888);

  // material

  CubeMtrl = new MrtRender;
  CubeMtrl->Flags = sMTRL_ZON | sMTRL_CULLON;
  CubeMtrl->Flags |= sMTRL_LIGHTING;
  CubeMtrl->Texture[0] = Tex;
  CubeMtrl->TFlags[0] = sMTF_LEVEL2|sMTF_CLAMP;
  CubeMtrl->Prepare(CubeGeo->GetFormat());

  // light

  sClear(Env);
  Env.AmbientColor  = 0x00202020;
  Env.LightColor[0] = 0x00ffffff;
  Env.LightDir[0].Init(1,-1,1);
  Env.LightDir[0].Unit();
  Env.Fix();

  // rendertargets

  sInt xs,ys;
  sGetScreenSize(xs,ys);

  for(sInt i=0;i<3;i++)
    RT[i] = new sTexture2D(xs,ys,sTEX_2D|sTEX_ARGB16F|sTEX_RENDERTARGET,0);
  sEnlargeZBufferRT(xs,ys);

  // blit

  BlitMtrl = new MrtBlit;
  BlitMtrl->Texture[0] = RT[0];
  BlitMtrl->Texture[1] = RT[1];
  BlitMtrl->Prepare(sVertexFormatSingle);

  sVertexSingle *vp2;
  BlitGeo = new sGeometry(sGF_QUADLIST,sVertexFormatSingle);
  BlitGeo->BeginLoadVB(4,sGD_STATIC,&vp2);
  sF32 fx = 0.5f/xs;
  sF32 fy = 0.5f/ys;
  vp2[0].Init(-1+fx,-1+fy,0,0xffffffff,0,0);
  vp2[1].Init(-1+fx, 1+fy,0,0xffffffff,0,1);
  vp2[2].Init( 1+fx, 1+fy,0,0xffffffff,1,1);
  vp2[3].Init( 1+fx,-1+fy,0,0xffffffff,1,0);
  BlitGeo->EndLoadVB();
}

// free resources (memory leaks are evil)

MyApp::~MyApp()
{
  delete Painter;
  delete BlitGeo;
  delete BlitMtrl;
  delete CubeGeo;
  delete CubeMtrl;
  delete Tex;
  for(sInt i=0;i<3;i++)
    delete RT[i];
}

/****************************************************************************/

// paint a frame

void MyApp::OnPaint3D()
{ 
  // set rendertarget

  sTargetPara tp;

  tp.ClearColor[0].InitColor(0xff405060);
  tp.ClearColor[1].InitColor(0xff808080);
  tp.Flags = sST_CLEARALL;
  tp.Depth = sGetScreenDepthBuffer();
  tp.Target[0] = RT[0];
  tp.Target[1] = RT[1];
  tp.Target[2] = RT[2];
  tp.Aspect = sGetScreenAspect();
  tp.Window.Init(0,0,RT[0]->SizeX,RT[0]->SizeY);

  sSetTarget(tp);

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

  sCBuffer<MrtVSPara> cb;
  cb.Data->Set(View,Env);
  CubeMtrl->Set(&cb);

  // draw

  CubeGeo->Draw();
 
  // blit

  sViewport View2;
  View2.Orthogonal = sVO_SCREEN;

  sSetTarget(sTargetPara(0,0,0,sGetScreenColorBuffer(),0));
  sCBuffer<MrtVSPara> cb2;
  cb2.Data->Set(View2,Env);
  sCBuffer<MrtPSPara> cbp;
  cbp.Data->Set(View2,Env);
  BlitMtrl->Set(&cb2,&cbp);
  BlitGeo->Draw();

  // debug output

  sF32 avg = Timer.GetAverageDelta();
  Painter->SetTarget();
  Painter->Begin();
  Painter->SetPrint(0,0xff000000,2);
  Painter->SetPrint(0,~0,2);
  Painter->PrintF(10,10,L"%5.2ffps %5.3fms",1000/avg,avg);
  Painter->End();
}

/****************************************************************************/

// abort program when escape is pressed

void MyApp::OnInput(const sInput2Event &ie)
{
  if((ie.Key&sKEYQ_MASK)==sKEY_ESCAPE) sExit();
}

/****************************************************************************/

// register application class

#if sPLATFORM==sPLAT_WINDOWS
sINITMEM(0,0);
#endif

void sMain()
{
  sSetWindowName(L"Multiple Rendertargets");

//  sEnlargeZBufferRT(640,480);
  sInit(sISF_3D|sISF_CONTINUOUS/*|sISF_FULLSCREEN*/,640,480);
  sSetApp(new MyApp());
}

/****************************************************************************/

