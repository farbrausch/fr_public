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

  for(sInt i=0;i<4;i++)
  {
    Geo[i] = new sGeometry();
    Geo[i]->Init(sGF_QUADRICS,sVertexFormatSingle);
  }

  // texture

  sImage img;
  img.Init(256,256);
  for(sInt i=0;i<4;i++)
  {
    static const sU32 c0[4] = { 0xff202020,0xff404040,0xff606060,0xff808080 };
    static const sU32 c1[4] = { 0xffff0000,0xff00ff00,0xff0000ff,0xffff00ff };
    img.Checker(c0[i],c1[i],8<<i,8<<i);
    Tex[i] = sLoadTexture2D(&img,sTEX_ARGB8888|sTEX_2D);
  }

  // material

  for(sInt i=0;i<4;i++)
  {
    Mtrl[i] = new sSimpleMaterial;
    Mtrl[i]->Flags = sMTRL_ZON | sMTRL_CULLOFF;
    Mtrl[i]->Flags |= sMTRL_LIGHTING;
    Mtrl[i]->Texture[0] = Tex[i];
    Mtrl[i]->TFlags[0] = sMTF_LEVEL2|sMTF_TILE;
    Mtrl[i]->Prepare(sVertexFormatSingle);
  }

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
  for(sInt i=0;i<4;i++)
  {
    delete Geo[i];
    delete Mtrl[i];
    delete Tex[i];
  }
  delete Painter;
}

/****************************************************************************/

// paint a frame

void MyApp::OnPaint3D()
{ 
  sRandom rnd;
  sVertexSingle *vp;

  // set rendertarget

  sSetRendertarget(0,sCLEAR_ALL,0xff405060);

  // get timing

  Timer.OnFrame(sGetTime());
  sInt time = Timer.GetTime();

  // set camera

  View.SetTargetCurrent();
  View.SetZoom(1.7f);
 // View.Model.EulerXYZ(time*0.0011f,time*0.0013f,time*0.0015f);
  View.Model.l.Init(0,0,0);
  View.Camera.l.Init(0,0,-4);
  View.Prepare();
 
  // set material

  sCBuffer<sSimpleMaterialEnvPara> cb;
  cb.Data->Set(View,Env);

  // begin

  for(sInt i=0;i<4;i++)
    Geo[i]->BeginQuadrics();

  // grid

  Geo[0]->BeginGrid((void **) &vp,65,65);
  for(sInt y=-32;y<=32;y++)
  {
    for(sInt x=-32;x<=32;x++)
    {
      sF32 px,py,fx,fy;
      fx = x/32.0f;
      fy = y/32.0f;
      px = x/16.0f + sFSin(x*0.10f+y*0.12f+time*0.0011f)*0.1f;
      py = y/16.0f + sFSin(x*0.11f+y*0.13f+time*0.0011f)*0.1f;
      vp->Init(px,py,0,0xff808080,fx,fy);
      vp++;
    }
  }
  Geo[0]->EndGrid();

  // some random quads

  sInt max = sInt((sFSin(time*0.004f)+2)*64);
  for(sInt i=0;i<max;i++)
  {
    sF32 x,y,s;
    sInt r;

    x = sFSin(i*0.10f+time*0.0011f);
    y = sFCos(i*0.12f+time*0.0014f);
    s = 0.1f;
    r = rnd.Int(4);

    Geo[r]->BeginQuad((void **) &vp,1);
    vp[0].Init(x-s,y-s,0,  ~0, 0,0);
    vp[1].Init(x+s,y-s,0,  ~0, 1,0);
    vp[2].Init(x+s,y+s,0,  ~0, 1,1);
    vp[3].Init(x-s,y+s,0,  ~0, 0,1);
    Geo[r]->EndQuad();
  }

  // end and draw

  for(sInt i=0;i<4;i++)
    Geo[i]->EndQuadrics();

  for(sInt i=0;i<4;i++)
  {
    Mtrl[i]->Set(&cb);
    Geo[i]->Draw();
  }

  // debug output

  sF32 avg = Timer.GetAverageDelta();
  Painter->SetTarget();
  Painter->Begin();
  Painter->SetPrint(0,~0,2);
  Painter->PrintF(10,10,L"%5.2ffps %5.3fms",1000/avg,avg);
  Painter->End();

}

/****************************************************************************/

// abort program when escape is pressed

void MyApp::OnInput(const sInput2Event &ie)
{
  if(ie.Key==sKEY_ESCAPE) sExit();
}

/****************************************************************************/

// register application class

void sMain()
{
  sInit(sISF_3D|sISF_CONTINUOUS/*|sISF_FULLSCREEN*/,640,480);
  sPartitionMemory(16*1024*1024,0,0);

  sSetApp(new MyApp());
  sSetWindowName(L"Prim");
}

/****************************************************************************/

