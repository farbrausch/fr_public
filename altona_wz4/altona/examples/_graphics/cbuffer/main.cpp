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

  Geo = new sGeometry();
  Geo->Init(sGF_TRILIST|sGF_INDEX16,sVertexFormatSingle);

  // load vertices and indices

  if(1)
  {
    sMatrix34 mat;
    sVector30 dir;
    sVertexSingle *vp=0;
    sU32 col = 0xffffffff;
    Geo->BeginLoadVB(4*6,sGD_STATIC,&vp);
    for(sInt i=0;i<6;i++)
    {
      mat.CubeFace(i);
      dir =  mat.i+mat.j+mat.k;
      vp[0].Init(dir.x,dir.y,dir.z,  col, 0,0);
      dir = -mat.i+mat.j+mat.k;
      vp[1].Init(dir.x,dir.y,dir.z,  col, 1,0);
      dir = -mat.i-mat.j+mat.k;
      vp[2].Init(dir.x,dir.y,dir.z,  col, 1,1);
      dir =  mat.i-mat.j+mat.k;
      vp[3].Init(dir.x,dir.y,dir.z,  col, 0,1);
      vp+=4;
    }
    Geo->EndLoadVB();

    sU16 *ip=0;
    Geo->BeginLoadIB(6*6,sGD_STATIC,&ip);
    for(sInt i=0;i<6;i++)
      sQuad(ip,i*4,0,1,2,3);
    Geo->EndLoadIB();
  }

  // texture

  sImage img(64,64);
  img.Checker(0xffff8080,0xff80ff80,8,8);
  img.Glow(0.5f,0.5f,0.25f,0.25f,0xffffffff,1.0f,4.0f);
  Tex = sLoadTexture2D(&img,sTEX_2D|sTEX_ARGB8888);

  // material

  Mtrl = new TestShader;
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
  View.Camera.l.Init(0,0,-8);
  View.Prepare();

  // set material

  sCBuffer<TestMain> cb;
  sCBuffer<TestTrans> cbt;
  sCBuffer<TestPS1> cb2;
  sCBuffer<TestPS2> cb3;

  cb.Data->ms_ss = View.ModelScreen;
  cb2.Data->Mul.InitColor(0xff406080);
  cb3.Data->Add.InitColor(0x00000000);
  
  // draw

  for(sInt i=0;i<27;i++)
  {
    cbt.Modify();
    cbt.Data->mat[0].Init(1,0,0,((i/1)%3-1)*3.0f);
    cbt.Data->mat[1].Init(0,1,0,((i/3)%3-1)*3.0f);
    cbt.Data->mat[2].Init(0,0,1,((i/9)%3-1)*3.0f);

    cb3.Modify();
    sF32 f = sFSin(i*0.5f+time*0.01f)*0.25f+0.5f;
    cb3.Data->Add.Init(f,f,f,0);

    Mtrl->Set(&cb,&cb2,&cb3,&cbt);
    Geo->Draw();
  }

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
  sSetWindowName(L"Constant Buffers");

  sInit(sISF_3D|sISF_CONTINUOUS|sISF_FSAA/*|sISF_FULLSCREEN*/,640,480);
  sSetApp(new MyApp());
}

/****************************************************************************/

