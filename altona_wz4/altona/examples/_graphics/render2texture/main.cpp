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

void sResolveTargetPrivate();


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
  img.Fill(0xffffffff);
//  img.Checker(0xffff8080,0xff80ff80,8,8);
//  img.Glow(0.5f,0.5f,0.25f,0.25f,0xffffffff,1.0f,4.0f);
  Tex = sLoadTexture2D(&img,sTEX_2D|sTEX_ARGB8888);

  TexRT = new sTexture2D(64,64,sTEX_2D|sTEX_ARGB8888|sTEX_RENDERTARGET
                        |sTEX_NOMIPMAPS|sTEX_MSAA);
  TexRTZ = new sTexture2D(64,64,sTEX_2D|sTEX_DEPTH24NOREAD|sTEX_RENDERTARGET
                        |sTEX_NOMIPMAPS|sTEX_MSAA);

  // material

  Mtrl = new sSimpleMaterial;
  Mtrl->Flags = sMTRL_ZON | sMTRL_CULLON;
  Mtrl->Flags |= sMTRL_LIGHTING;
  Mtrl->Texture[0] = Tex;
  Mtrl->TFlags[0] = sMTF_LEVEL2|sMTF_CLAMP;
  Mtrl->Prepare(Geo->GetFormat());

  MtrlRT = new sSimpleMaterial;
  MtrlRT->Flags = sMTRL_ZON | sMTRL_CULLON;
  MtrlRT->Flags |= sMTRL_LIGHTING;
  MtrlRT->Texture[0] = TexRT;
  MtrlRT->TFlags[0] = sMTF_LEVEL0|sMTF_CLAMP;
  MtrlRT->Prepare(Geo->GetFormat());

  // light

  sClear(Env);
  Env.AmbientColor  = 0x00202020;
  Env.LightColor[0] = 0x00ffffff;
  Env.LightDir[0].Init(0,0,1);
  Env.Fix();

  // ipp resources

  DummyTex = new sTexture2D(8,8,sTEX_2D|sTEX_ARGB8888,1);
}

// free resources (memory leaks are evil)

MyApp::~MyApp()
{
  delete Painter;
  delete Geo;
  delete Mtrl;
  delete MtrlRT;
  delete Tex;
  delete TexRT;
  delete TexRTZ;
  
  delete DummyTex;
}

/****************************************************************************/

// paint a frame

void MyApp::OnPaint3D()
{ 

  // render a cube to a texture

#if sRENDERER==sRENDER_DX11
  sSetTarget(sTargetPara(sST_CLEARALL,0xff000000,0,TexRT,TexRTZ)); // won't work with old interface - wait till sSetRendertarget() is gone for good
#else
  sSetTarget(sTargetPara(sST_CLEARCOLOR,0xff000000,0,TexRT,0));
#endif
  RenderCube(Mtrl);
  sResolveTarget();

  // render that texture on a cube

  sSetTarget(sTargetPara(sST_CLEARALL,0xff405060));
  RenderCube(MtrlRT);

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

void MyApp::RenderCube(sMaterial *mtrl)
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
  View.Camera.l.Init(0,0,-2.5f);
  View.ClipNear = 0.125f;
  View.ClipFar = 16.0f;
  View.Prepare();

  // set material

  sCBuffer<sSimpleMaterialEnvPara> cb;
  cb.Data->Set(View,Env);
  mtrl->Set(&cb);

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

// abort program when escape is pressed

void MyApp::OnInput(const sInput2Event &ie)
{
  if((ie.Key&sKEYQ_MASK)==sKEY_ESCAPE) sExit();
}

/****************************************************************************/

// register application class

void sMain()
{
  sSetWindowName(L"render to texture");

  sScreenMode sm;
  sm.Clear();
  sm.Flags = sSM_VALID;
  sm.MultiLevel = 255;
  sSetScreenMode(sm);

  sInit(sISF_3D|sISF_CONTINUOUS,1024,768);
  sSetApp(new MyApp());
}

/****************************************************************************/

