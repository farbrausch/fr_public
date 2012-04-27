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

  Geo = new sGeometry(sGF_TRILIST|sGF_INDEX16,sVertexFormatStandard);
  Geo->LoadCube(~0UL,2,2,2,sGD_STATIC);

  // texture

  Tex = new sTexture2D(256,256,sTEX_2D|sTEX_I8|sTEX_CS_WRITE,1);

  // buffer as lookup-table for pixel shader

  sU32 lutdata[256];
  for(sInt i=0;i<256;i++)
  {
    sVector4 c;
    c.x = sFSin(sF32(i)/256.0f*10+3)*0.5f+0.5f;
    c.y = sFSin(sF32(i)/256.0f*10+2)*0.5f+0.5f;
    c.z = sFSin(sF32(i)/256.0f*10+1)*0.5f+0.5f;
    c.w = 1;
    lutdata[i] = c.GetColor();
  }
  lutdata[0] = 0xff000000;
  Lut = new sCSBuffer(sTEX_BUFFER|sTEX_ARGB8888,256,0,lutdata);

  // material

  Mtrl = new MaterialFlat;
  Mtrl->Flags = sMTRL_ZON | sMTRL_CULLON;
  Mtrl->Texture[0] = Tex;
  Mtrl->TFlags[0] = sMTF_LEVEL2|sMTF_CLAMP;
  Mtrl->Texture[1] = Lut;
  Mtrl->Prepare(sVertexFormatStandard);

  // light

  sClear(Env);
  Env.AmbientColor  = 0x00202020;
  Env.LightColor[0] = 0x00ffffff;
  Env.LightDir[0].Init(0,0,1);
  Env.Fix();

  // compute shader

  CS = new sComputeShader(CSTest());
  CS->SetUAV(0,Tex);
  CS->Prepare();

}

// free resources (memory leaks are evil)

MyApp::~MyApp()
{
  delete Painter;
  delete Geo;
  delete Mtrl;
  delete Tex;
  delete CS;
  delete Lut;
}

/****************************************************************************/

// paint a frame
sInt sGetFrameTime();

void MyApp::OnPaint3D()
{ 
  // set rendertarget

  sSetTarget(sTargetPara(sST_CLEARALL,0xff405060));

  // get timing

  Timer.OnFrame(sGetTime());
  static sInt time;
  if(sHasWindowFocus())
  {
    if(sRENDERER==sRENDER_DX11 && (sGetSystemFlags()&sISF_FULLSCREEN))
      time = sGetFrameTime();
    else
      time = Timer.GetTime();
  }

  // compute shader

  sCBuffer<CSTestPara> cbc;
  sF32 z = sFSin(time*0.001f)*0.5f+1.0f;
  cbc.Data->scalebias.Init(0.00918f*z,0.00918f*z,-1.1590f,0.28518f);
  cbc.Data->uvscale.Init(sF32(Tex->SizeX),sF32(Tex->SizeY),0,0);
  CS->Draw(Tex->SizeX/8,Tex->SizeY/8,1,&cbc);
  
  // set camera

  View.SetTargetCurrent();
  View.SetZoom(2.0f);
  View.Model.EulerXYZ(time*0.0011f,time*0.0013f,time*0.0015f);
  View.Model.l.Init(0,0,0);
  View.Camera.l.Init(0,0,-4);
  View.Prepare();

  // set material

  sCBuffer<MaterialFlatPara> cb;
  sVector30 n(0.2,0.2,-1);
  sMatrix34 mat;
  mat = View.ModelView;
  mat.Trans3();
  n.Unit();
  cb.Data->mvp = View.ModelScreen;
  cb.Data->ldir = n*mat;
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
  if((ie.Key&sKEYQ_MASK)==sKEY_ESCAPE) sExit();
}

/****************************************************************************/

// register application class

#if sPLATFORM==sPLAT_WINDOWS
sINITMEM(0,0);
#endif

void sMain()
{
  sSetWindowName(L"Compute Shader Simple Example");

  sInt flags = sISF_3D|sISF_CONTINUOUS;
  flags |= sISF_FULLSCREENIFRELEASE;
//  flags |= sISF_FSAA;
//  flags |= sISF_NOVSYNC;

  sInit(flags,1280,720);
  sSetApp(new MyApp());
}

/****************************************************************************/

