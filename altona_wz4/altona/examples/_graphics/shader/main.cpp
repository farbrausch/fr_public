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
/***                                                                      ***/
/***   Draw a cube using a custom shader.                                 ***/
/***                                                                      ***/
/***   the "altona shader compiler" does most of the ugly work.           ***/
/***                                                                      ***/
/****************************************************************************/

#include "main.hpp"
#include "base/windows.hpp"
#include "shader.hpp"
#include "util/image.hpp"
#include "util/shaders.hpp"

/****************************************************************************/

// constructor

MyApp::MyApp()
{

  // painter: debug out in 2d

  Painter = new sPainter;

  // create dummy texture and material

  sImage img;
  img.Init(32,32);
  img.Checker(0xffff4040,0xff40ff40,4,4);
  img.Glow(0.5f,0.5f,0.25f,0.25f,0xffc0c0c0,1.0f,4.0f);

  Tex = sLoadTexture2D(&img,sTEX_ARGB8888);

  Mtrl = new MaterialFlat;
  Mtrl->Texture[0] = Tex;
  Mtrl->Flags = sMTRL_ZON;
  Mtrl->TFlags[0] = sMTF_CLAMP|sMTF_LEVEL2;
  Mtrl->Prepare(sVertexFormatStandard);

  // the cube geometry

  Geo = new sGeometry(sGF_TRILIST|sGF_INDEX16,sVertexFormatStandard);
  Geo->LoadCube();
}

/****************************************************************************/

// destructor

MyApp::~MyApp()
{
  delete Painter;
  delete Geo;
  delete Mtrl;
  delete Tex;
}

/****************************************************************************/

// painting

void MyApp::OnPaint3D()
{
  // clear screen

  sSetTarget(sTargetPara(sCLEAR_ALL,0xff405060));

  // timing

  Timer.OnFrame(sGetTime());
  sInt time = Timer.GetTime();

  // set camera

  View.SetTargetCurrent();
  View.Model.EulerXYZ(time*0.0011f,time*0.0013f,time*0.0012f);
  View.SetZoom(1);
  View.Camera.l.Init(0,0,-2);
  View.Prepare();

  // set material

  sCBuffer<MaterialFlatPara> MtrlPara;
  MtrlPara.Data->mvp = View.ModelScreen;
  MtrlPara.Data->ldir.Init(-View.ModelView.i.z,-View.ModelView.j.z,-View.ModelView.k.z,0);
  Mtrl->Set(&MtrlPara);

  // draw a cube

  Geo->Draw();

  // debug out: framerate.
  sF32 avg = Timer.GetAverageDelta();
  Painter->Begin();
  Painter->SetPrint(0,~0,1);
  Painter->PrintF(10,10,L"%5.2ffps %5.3fms",1000/avg,avg);
  Painter->End();
}

/****************************************************************************/

// onkey: check for escape

void MyApp::OnInput(const sInput2Event &ie)
{
  if((ie.Key&sKEYQ_MASK)==sKEY_ESCAPE) sExit();
}

/****************************************************************************/

// main: initialize screen and app

void sMain()
{
  sInit(sISF_3D|sISF_CONTINUOUS,640,480);
  sSetApp(new MyApp());
  sSetWindowName(L"shader");
}

/****************************************************************************/
