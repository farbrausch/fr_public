/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "main.hpp"
#include "base/windows.hpp"
#include "util/image.hpp"

#include "util/taskscheduler.hpp"
#include "mc_shaders.hpp"

/****************************************************************************/
/****************************************************************************/


/****************************************************************************/
/****************************************************************************/

// initialize resources
 
MyApp::MyApp()
{
  sAddSched();
  sSchedMon = new sStsPerfMon;
  MC = new MarchingCubes(1024*1024);
  EnableDraw = 1;
  EnableMon = 1;
  Gran = 0;

  // debug output

  Painter = new sPainter;
  Timer.SetHistoryMax(4);

  // multitasking

  // texture
  Tex = sLoadTexture2D(L"envi.bmp",sTEX_2D|sTEX_ARGB8888);
  if(Tex==0)
  {
    sImage img(512,512);
    img.Checker(0xffff8080,0xff80ff80,8,8);
    img.Perlin(2,6,0.6f,0,0,0.5f);
    img.Glow(0.5f,0.5f,0.25f,0.25f,0xffffffff,1.0f,4.0f);
    Tex = sLoadTexture2D(&img,sTEX_2D|sTEX_ARGB8888);
  }
 
  // material

  Mtrl = new MCubesMtrl;
  Mtrl->Flags = sMTRL_ZON | sMTRL_CULLON;
  Mtrl->Flags |= sMTRL_LIGHTING;
  Mtrl->Texture[0] = Tex;
  Mtrl->TFlags[0] = sMTF_LEVEL2|sMTF_CLAMP;
  Mtrl->Prepare(sVertexFormatStandard);

  // light

  sClear(Env);
  Env.AmbientColor  = 0x00202020;
  Env.LightColor[0] = 0x00c0c0c0;
  Env.LightDir[0].Init(0.5f,-2,1);
  Env.Fix();
}

// free resources (memory leaks are evil)

MyApp::~MyApp()
{
  delete Painter;
  delete Mtrl;
  delete Tex;
  delete MC;
}

/****************************************************************************/

// paint a frame

void MyApp::OnPaint3D()
{ 
  sViewport View;

  // set rendertarget

  sSchedMon->Begin(0,0xff8080);
  sSetRendertarget(0,sCLEAR_ALL,0xff405060);

  // get timing and camera

  Timer.OnFrame(sGetTime());
  Cam.OnFrame(Timer.GetSlices(),sF32(Timer.GetJitter())/10);
  Cam.MakeViewport(View);

  // do the mc

  const sInt pn = 10000;
  static sVector31 parts[2][pn];
  static sInt db = 0;

  sStsWorkload *wl = sSched->BeginWorkload();
  MC->Begin(sSched,wl,1<<Gran);
  wl->Start();

  {
    sMatrix34 mat;
    sF32 t = Timer.GetTime();
  //    mat.EulerXYZ(0,t*0.001f,0);
    sRandom rnd;
    for(sInt i=0;i<pn;i++)
    {
      parts[db][i].x = sFSin(t*0.00010+rnd.Float(sPI2F))*(13+rnd.Float(5)+rnd.Float(5)+rnd.Float(5));
      parts[db][i].y = sFSin(t*0.00011+rnd.Float(sPI2F))*(13+rnd.Float(5)+rnd.Float(5)+rnd.Float(5));
      parts[db][i].z = sFSin(t*0.00013+rnd.Float(sPI2F))*(13+rnd.Float(5)+rnd.Float(5)+rnd.Float(5));
    }
//    for(sInt i=0;i<pn;i++)
//      parts[db][i] = parts[db][i]*mat;
  }

  db = 1-db;
  MC->Render(parts[db],pn);
  wl->Sync();
  MC->End();

  // draw

  sCBuffer<MCubesMtrlVS> cb;
  cb.Data->Set(View);
  Mtrl->Set(&cb);

  if(EnableDraw)
    MC->Draw();

  // debug output

  sF32 avg = Timer.GetAverageDelta();
  sTextBuffer tb;
  tb.PrintF(L"%5.2ffps %5.3fms\n",1000/avg,avg);
  tb.PrintF(L"%s",wl->PrintStat());
  tb.PrintF(L"SPACE: toggle drawing\n");
  tb.PrintF(L"m: toggle monitor\n");
  tb.PrintF(L"g: Granularity %d\n",Gran);

  Painter->SetTarget();
  Painter->Begin();
  Painter->SetPrint(0,0xff000000,2);
  Painter->SetPrint(0,~0,2);
  Painter->Print(10,10,tb.Get());
  Painter->End();

  sSchedMon->Scale = 17;
  sSchedMon->Begin(0,0xff0000);
  sTargetSpec ts;
  if(EnableMon)
    sSchedMon->Paint(ts);
  sSchedMon->End(0);
  wl->End();
  sSchedMon->End(0);
  sSchedMon->FlipFrame();
}

/****************************************************************************/

// abort program when escape is pressed

void MyApp::OnInput(const sInput2Event &ie)
{
  Cam.OnInput(ie);
  sU32 key = ie.Key;
  if(key & sKEYQ_SHIFT) key |= sKEYQ_SHIFT;
  if(key & sKEYQ_CTRL ) key |= sKEYQ_CTRL;
  switch(key)
  {
  case sKEY_ESCAPE:
  case sKEY_ESCAPE|sKEYQ_SHIFT:
    sExit();
    break;
  case sKEY_SPACE:
    EnableDraw = !EnableDraw;
    break;
  case 'm':
    EnableMon = !EnableMon;
    break;
  case 'g':
    Gran = sClamp(Gran+1,0,10);
    break;
  case 'G'|sKEYQ_SHIFT:
    Gran = sClamp(Gran-1,0,10);
    break;
  }
}

/****************************************************************************/

// register application class

#if sPLATFORM==sPLAT_WINDOWS
sINITMEM(0,0);
#endif

void sMain()
{
  sSetWindowName(L"Marching Cubes");

//  sInit(sISF_3D|sISF_CONTINUOUS|sISF_FSAA|sISF_NOVSYNC/*|sISF_FULLSCREEN*/,1920,1080);
  sInit(sISF_3D|sISF_CONTINUOUS|sISF_FSAA|sISF_NOVSYNC/*|sISF_FULLSCREEN*/,1024,768);
  sSetApp(new MyApp());
}

/****************************************************************************/
/****************************************************************************/
