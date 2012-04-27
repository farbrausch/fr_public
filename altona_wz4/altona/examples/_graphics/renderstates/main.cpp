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

  Geo = new sGeometry();
//  Geo->Init(sGF_TRILIST|sGF_INDEX16,sVertexFormatStandard);
   Geo->Init(sGF_TRILIST|sGF_INDEX16,sVertexFormatSingle);

  // texture

  sImage img;
  img.Init(64,64);
  img.Checker(0xffff8080,0xff80ff80,8,8);
  img.Glow(0.5f,0.5f,0.25f,0.25f,0xffffffff,1.0f,4.0f);

  Tex = sLoadTexture2D(&img,sTEX_ARGB8888|sTEX_2D);

  // material

  Mtrl = new sSimpleMaterial;
  Mtrl->Flags = sMTRL_ZON | sMTRL_CULLON;
  Mtrl->Flags |= sMTRL_LIGHTING;
  Mtrl->Texture[0] = Tex;
  Mtrl->TFlags[0] = sMTF_LEVEL2|sMTF_CLAMP;

  Mtrl->InitVariants(4);
  Mtrl->SetVariant(0);

  Mtrl->Flags = sMTRL_ZON | sMTRL_CULLINV;
  Mtrl->SetVariant(1);

  Mtrl->Flags = sMTRL_ZREAD | sMTRL_CULLOFF;
  Mtrl->BlendColor = sMB_ADD;
  Mtrl->SetVariant(2);

  Mtrl->Flags = sMTRL_ZREAD | sMTRL_CULLOFF;
  Mtrl->BlendColor = sMB_MUL;
  Mtrl->SetVariant(3);

  Mtrl->Prepare(sVertexFormatSingle);

  // light

  sClear(Env);
  Env.AmbientColor  = 0xff404040;
  Env.LightColor[0] = 0x00c00000;
  Env.LightColor[1] = 0x0000c000;
  Env.LightColor[2] = 0x000000c0;
  Env.LightColor[3] = 0x00000000;
  Env.LightDir[0].Init(1,0,0);
  Env.LightDir[1].Init(0,1,0);
  Env.LightDir[2].Init(0,0,1);
  Env.LightDir[3].Init(0,0,0);
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
  sInt time = Timer.GetTime();

  // load vertices and indices
#if 1
  if(Geo->GetFormat()==sVertexFormatSingle)
  {
    sVertexSingle *vp;
    Geo->BeginLoadVB(8,sGD_STREAM,&vp);
    vp[0].Init(-1,-1,-1,  0xff404040, 0,0);
    vp[1].Init( 1,-1,-1,  0xffff0000, 0,1);
    vp[2].Init( 1, 1,-1,  0xff00ff00, 1,1);
    vp[3].Init(-1, 1,-1,  0xffffff00, 1,0);
    vp[4].Init(-1,-1, 1,  0xff0000ff, 0,1);
    vp[5].Init( 1,-1, 1,  0xffff00ff, 1,1);
    vp[6].Init( 1, 1, 1,  0xff00ffff, 1,0);
    vp[7].Init(-1, 1, 1,  0xffffffff, 0,0);
    Geo->EndLoadVB();
  }
  else
  {
    sVertexStandard *vp;
    Geo->BeginLoadVB(8,sGD_STREAM,&vp);
    vp[0].Init(-1,-1,-1,  0, 0,-1, 0,0);
    vp[1].Init( 1,-1,-1,  0, 0,-1, 0,1);
    vp[2].Init( 1, 1,-1,  0, 0,-1, 1,1);
    vp[3].Init(-1, 1,-1,  0, 0,-1, 1,0);
    vp[4].Init(-1,-1, 1,  0, 0, 1, 0,1);
    vp[5].Init( 1,-1, 1,  0, 0, 1, 1,1);
    vp[6].Init( 1, 1, 1,  0, 0, 1, 1,0);
    vp[7].Init(-1, 1, 1,  0, 0, 1, 0,0);
    Geo->EndLoadVB();
  }

  sU16 *ip;
  Geo->BeginLoadIB(6*6,sGD_STREAM,&ip);
  sQuad(ip,0,3,2,1,0);
  sQuad(ip,0,4,5,6,7);
  sQuad(ip,0,0,1,5,4);
  sQuad(ip,0,1,2,6,5);
  sQuad(ip,0,2,3,7,6);
  sQuad(ip,0,3,0,4,7);
  Geo->EndLoadIB();

  // draw four materials

  View.SetTargetCurrent();
  View.SetZoom(1.7f);
  View.Model.EulerXYZ(time*0.0011f,time*0.0013f,time*0.0015f);
  View.Camera.l.Init(0,0,-10);

  sCBuffer<sSimpleMaterialEnvPara> cb;
  sCBufferBase *cbp = &cb;

  for(sInt i=0;i<4;i++)
  {
    View.Model.l.Init((i-1.5f)*3,0,0);
    View.Prepare();
//    View.Set();

    cb.Modify();
    cb.Data->Set(View,Env);
    Mtrl->Set(&cbp,1,i);

    Geo->Draw();
  }

  // debug output
#endif
  sF32 avg = Timer.GetAverageDelta();
  Painter->SetTarget();
  Painter->Begin();
  Painter->SetPrint(0,~0,1);
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

void sMain()
{
  sInit(sISF_3D|sISF_CONTINUOUS/*|sISF_FULLSCREEN*/,640,480);
  sSetApp(new MyApp());
  sSetWindowName(L"Renderstates");
}

/****************************************************************************/

