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

#define TDIM 4
#define GDIM 6
#define DIM (GDIM*TDIM)

/****************************************************************************/

// initialize resources
 
MyApp::MyApp()
{
  // debug output
 
  Painter = new sPainter;

  // geometry 

  static const sU32 desc[] = 
  {
    sVF_POSITION|sVF_F3,
    sVF_NORMAL|sVF_F3,
    sVF_UV0|sVF_F2,
    sVF_UV1|sVF_F4|sVF_INSTANCEDATA|sVF_STREAM1,
    sVF_END
  };

  Format = sCreateVertexFormat(desc);

  CSVB = new sCSBuffer(sTEX_BUFFER|sTEX_RAW|sTEX_CS_VERTEX|sTEX_CS_WRITE,DIM*DIM*DIM*4*Format->GetSize(0)/4);
  Geo = new sGeometry(sGF_TRILIST|sGF_INDEX32,Format);
  Geo->LoadCube(0xffffffff,0.8f,0.8f,0.8f,sGD_STATIC);
  Geo->SetVB(CSVB,1);

  CSCount = new sCSBuffer(sTEX_BUFFER|sTEX_STRUCTURED|sTEX_CS_COUNT|sTEX_CS_WRITE,1,4);
  CSIndirect = new sCSBuffer(sTEX_BUFFER|sTEX_UINT32|sTEX_CS_WRITE|sTEX_CS_INDIRECT,5,4);

  // texture

  sImage img;
  img.Init(32,32);
  img.Checker(0xffff4040,0xff40ff40,4,4);
  img.Glow(0.5f,0.5f,0.25f,0.25f,0xffc0c0c0,1.0f,4.0f);
  Tex = sLoadTexture2D(&img,sTEX_2D|sTEX_ARGB8888);

  // material

  Mtrl = new MaterialFlat;
  Mtrl->Flags = sMTRL_ZON | sMTRL_CULLOFF;
  Mtrl->Texture[0] = Tex;
  Mtrl->TFlags[0] = sMTF_LEVEL2|sMTF_CLAMP;
  Mtrl->Prepare(sVertexFormatStandard);

  // light

  sClear(Env);
  Env.AmbientColor  = 0x00202020;
  Env.LightColor[0] = 0x00ffffff;
  Env.LightDir[0].Init(0,0,1);
  Env.Fix();

  // compute shader

  CS = new sComputeShader(CSTest());
  CS->SetUAV(0,CSVB);
  CS->SetUAV(1,CSCount,1);
  CS->Prepare();

  CSWC = new sComputeShader(WriteCount());
  CSWC->SetUAV(0,CSCount,0);
  CSWC->SetUAV(1,CSIndirect,0);
  CSWC->Prepare();
}

// free resources (memory leaks are evil)

MyApp::~MyApp()
{
  delete CSVB;
  delete CSIndirect;
  delete Painter;
  delete Geo;
  delete Mtrl;
  delete Tex;
  delete CS;
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

  // set camera

  View.SetTargetCurrent();
  View.SetZoom(2.0f);
  View.Camera.EulerXYZ(time*0.00011f,time*0.00013f,time*0.00015f);
  View.Camera.l = sVector31(sVector30(View.Camera.k*-20));
  View.Prepare();

  // compute shader

  sCBuffer<CSTestPara> cbc;
  sF32 rnd[12];
  sRandom rg;
  for(sInt i=0;i<12;i++)
    rnd[i] = rg.Float(0.001f)+0.0005f;

  sMatrix34 cmat = View.Camera;
  cmat.Transpose3();
  cbc.Modify();
  cbc.Data->tx = -cmat.i*0.5f;
  cbc.Data->ty = cmat.j*0.5f;
  cbc.Data->ofs.Init(-(DIM-1)/2.0f,-(DIM-1)/2.0f,-(DIM-1)/2.0f,0);
  sF32 s = DIM/4;
  cbc.Data->pot0.Init(sFSin(time*rnd[0])*s,sFSin(time*rnd[4])*s,sFSin(time*rnd[ 8])*s,1);
  cbc.Data->pot1.Init(sFSin(time*rnd[1])*s,sFSin(time*rnd[5])*s,sFSin(time*rnd[ 9])*s,1);
  cbc.Data->pot2.Init(sFSin(time*rnd[2])*s,sFSin(time*rnd[6])*s,sFSin(time*rnd[10])*s,1);
  cbc.Data->pot3.Init(sFSin(time*rnd[3])*s,sFSin(time*rnd[7])*s,sFSin(time*rnd[11])*s,1);
  CS->Draw(GDIM,GDIM,GDIM,&cbc);

  CSWC->Draw(1,1,1);

  // set material

  sCBuffer<MaterialFlatPara> cb;
  cb.Data->mvp = View.ModelScreen;
  cb.Data->ldir.Init(-View.ModelView.i.z,-View.ModelView.j.z,-View.ModelView.k.z,0);
  Mtrl->Set(&cb);

  // draw

  sGeometryDrawInfo di;
  di.Indirect = CSIndirect;
  Geo->Draw(di);

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
  sSetWindowName(L"Cube");

  sInt flags = sISF_3D|sISF_CONTINUOUS;
  flags |= sISF_FULLSCREENIFRELEASE;
//  flags |= sISF_FSAA;
//  flags |= sISF_NOVSYNC;

  sInit(flags,1280,720);
  sSetApp(new MyApp());
}

/****************************************************************************/

