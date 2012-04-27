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
//  img.Glow(0.5f,0.5f,0.25f,0.25f,0xffffffff,1.0f,4.0f);
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

  CompMtrl = new sSimpleMaterial;
  CompMtrl->Flags = sMTRL_ZOFF | sMTRL_CULLOFF;
  CompMtrl->Texture[0] = DummyTex;
  CompMtrl->TFlags[0] = sMTF_LEVEL0|sMTF_CLAMP;
  CompMtrl->BlendColor = sMBD_FI|sMBO_ADD|sMBS_F;
  CompMtrl->BlendFactor = 0xff808080;
  CompMtrl->Prepare(sVertexFormatSingle);
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
  delete CompMtrl;
}

/****************************************************************************/

// paint a frame

void MyApp::OnPaint3D()
{ 
  // set rendertarget

  sSetTarget(sTargetPara(sST_CLEARALL,0xff405060));

  // render cube

  RenderCube(-2);

  // do image post processing

  sResolveTarget();
  sRTMan->SetScreen(0);

  IppMosaik();

  sRTMan->FinishScreen();

  sSetTarget(sTargetPara(sST_CLEARDEPTH|sST_NOMSAA,0xff405060));

  RenderCube(2);

  // debug output

  sSetTarget(sTargetPara(sST_NOMSAA,0xff405060));

  sF32 avg = Timer.GetAverageDelta();
  Painter->SetTarget();
  Painter->Begin();
  Painter->SetPrint(0,0xff000000,2);
  Painter->SetPrint(0,~0,2);
  Painter->PrintF(10,10,L"%5.2ffps %5.3fms",1000/avg,avg);
  Painter->End();
}

void MyApp::RenderCube(sF32 x)
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
  View.Model.l.Init(x,0,0);
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

void MyApp::IppMosaik()
{
  sTexture2D *src = sRTMan->ReadScreen();
  sTexture2D *temp = sRTMan->Acquire(src->SizeX/16,src->SizeY/16);
  sTexture2D *dest = sRTMan->WriteScreen(1);

  Blit(temp,src,BlitMtrl);
  Blit(dest,temp,CompMtrl);

  sRTMan->Release(src);
  sRTMan->Release(temp);
  sRTMan->Release(dest);
}


void MyApp::Blit(sTexture2D *d,sTexture2D *s,sMaterial *mtrl)
{
  sVertexSingle *vp;
  sViewport view;

  sRTMan->SetTarget(d);
  view.SetTargetCurrent();
  view.Orthogonal = sVO_SCREEN;
  view.Prepare();
  sCBuffer<sSimpleMaterialPara> cb;
  cb.Data->Set(view);
  mtrl->Texture[0] = s;
  mtrl->Set(&cb);
  sF32 fu = 0.5f/s->SizeX;
  sF32 fv = 0.5f/s->SizeY;

  BlitGeo->BeginLoadVB(4,sGD_STREAM,&vp);
  vp[0].Init(0,0,0,~0,0+fu,0+fv);
  vp[1].Init(1,0,0,~0,1+fu,0+fv);
  vp[2].Init(1,1,0,~0,1+fu,1+fv);
  vp[3].Init(0,1,0,~0,0+fu,1+fv);
  BlitGeo->EndLoadVB();
  BlitGeo->Draw();

  mtrl->Texture[0] = DummyTex;
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
  sSetWindowName(L"ipp");

  sScreenMode sm;
  sm.Clear();
  sm.Flags = sSM_NOVSYNC|sSM_VALID;
  sm.MultiLevel = 255;
  sSetScreenMode(sm);

  sInit(sISF_3D|sISF_CONTINUOUS,1024,768);
  sSetApp(new MyApp());
}

/****************************************************************************/

