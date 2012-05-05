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

struct MyVertex
{
  sF32 px,py,pz;
  sF32 nx,ny,nz;
  sF32 u0,v0;
};

MyApp::MyApp()
{

  // painter: debug out in 2d

  Painter = new sPainter;

  // vertex format

  sU32 desc[] =
  {
    sVF_POSITION,
    sVF_NORMAL,
    sVF_UV0,
    sVF_END,
  };
  MyFormat = sCreateVertexFormat(desc);

  // create dummy texture and material

  sImage img;
  img.Init(256,256);
  img.Perlin(0,5,0.7f,0,0,1);
  img.SaveBMP(L"c:/test.bmp");
  Tex = sLoadTexture2D(&img,sTEX_ARGB8888);

  Mtrl = new TestMtrl;
  Mtrl->Texture[0] = Tex;
  Mtrl->Flags = sMTRL_ZON;
  Mtrl->TFlags[0] = sMTF_TILE|sMTF_LEVEL2;
  Mtrl->TBind[0] = sMTB_VS|0;
  Mtrl->Prepare(sVertexFormatStandard);


  // the cube geometry

  Geo = new sGeometry(sGF_TRILIST|sGF_INDEX16,MyFormat);
  Geo->LoadTorus(12,48); 
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
  View.Model.EulerXYZ(time*0.000031f,time*0.000033f,time*0.000032f);
  View.SetZoom(1);
  View.Camera.l.Init(0,0,-2);
  View.Prepare();

  // set material

  sMatrix34 mat = View.Camera;

  sCBuffer<TestMtrlVSPara> cbv;
  sCBuffer<TestMtrlGSPara> gbv;
  cbv.Data->mvp = View.ModelScreen;
  cbv.Data->ldir.Init(-View.ModelView.i.z,-View.ModelView.j.z,-View.ModelView.k.z,0);
  cbv.Data->uvoffset.Init(time*0.00011f,time*0.00012f,time*0.00013f,time*0.00014f);
  gbv.Data->width.Init(1.5f/View.TargetSizeX,0,0,0);
  Mtrl->Set(&gbv,&cbv);

  // draw 

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
  if( (ie.Key&sKEYQ_MASK)==sKEY_ESCAPE) sExit();
}

/****************************************************************************/

// main: initialize screen and app

void sMain()
{
  sInit(sISF_3D|sISF_CONTINUOUS,1280,720);
  sSetApp(new MyApp());
  sSetWindowName(L"gs");
}

/****************************************************************************/
