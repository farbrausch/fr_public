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
#include "util/perfmon.hpp"

#include "network/netdebug.hpp"
#include "network/netdebug_plugins.hpp"

#if defined(sCONFIG_DUMMYNETWORK)

#pragma message("dummy network!")

#endif


/****************************************************************************/

// my own http debugging plugin

struct MyPlugin : public sHTTPServer::SimpleHandler
{

  sHTTPServer::HandlerResult WriteDocument(const sChar *URL)
  {
    WriteHTMLHeader(L"example",L"/style.css");
    PrintF(L"<marquee>Test!</marquee>");
    PrintF(L"my URL is '%s'<br>",URL);

    sInt a=GetParamI(L"a",-1);

    PrintF(L".. and the 'a' parameter is set to <b>%d</b>. ",a);
    PrintF(L"<a href=\"%s?a=%d\">Set to something else</a>\n",URL,sGetRandomSeed()%10);

    WriteHTMLFooter();
    return sHTTPServer::HR_OK;
  }

  static Handler *Factory() { return new MyPlugin; }
};

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

  Tex = new sTexture2D(256,256,sTEX_ARGB8888|sTEX_2D);

  sBeginAlt();
  sImage *img = new sImage(256,256);

  img->Checker(0xff808080,0xff404040,32,32);
  img->Glow(0.5f,0.5f,0.25f,0.25f,0xffffffff,1.0f,4.0f);

  sImageData *id = new sImageData(img,sTEX_ARGB8888|sTEX_2D);
  sEndAlt();
  Tex->LoadAllMipmaps(id->Data);

  delete img;
  delete id;

  // material

  Mtrl = new sSimpleMaterial;
  Mtrl->Flags = sMTRL_ZON | sMTRL_CULLON;
  Mtrl->Flags |= sMTRL_LIGHTING;
  Mtrl->Texture[0] = Tex;
  Mtrl->TFlags[0] = sMTF_LEVEL2|sMTF_CLAMP;
  Mtrl->Prepare(Geo->GetFormat());

  // light

  sClear(Env);

  Env[0].AmbientColor  = 0x00ffffff;
  Env[0].Fix();

  Env[1].AmbientColor  = 0x00202020;
  Env[1].LightColor[0] = 0x00ff4000;
  Env[1].LightColor[1] = 0x0040ff00;
  Env[1].LightColor[2] = 0x000040ff;
  Env[1].LightDir[0].Init(1,0,0);
  Env[1].LightDir[1].Init(0,1,0);
  Env[1].LightDir[2].Init(0,0,1);
  Env[1].Fix();

  Env[2].AmbientColor  = 0x00202020;
  Env[2].LightColor[0] = 0x00ffffff;
  Env[2].LightDir[0].Init(0,0,1);
  Env[2].Fix();

  EnvSel=0;
  sPerfAddSwitch(L"Lighting",L"ambient|colorful|headlight",&EnvSel);


  Testvalue1=12345;
  Testvalue2=23456;
  Testvalue3=0;

  sPerfAddValue(L"Test 1",&Testvalue1,0,65535);
  sPerfAddValue(L"Test 2",&Testvalue2,0,65535);
  sPerfAddValue(L"Random Test",&Testvalue3,0,255,1);
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
  sPERF_FUNCTION(0xffffff);

  // set rendertarget

  sSetRendertarget(0,sCLEAR_ALL,0xff405060);

  // get timing

  Timer.OnFrame(sGetTime());
  sInt time = Timer.GetTime();

  // set camera

  View.SetTargetCurrent();
  View.SetZoom(1.0f);
  View.Model.EulerXYZ(time*0.0011f,time*0.0013f,time*0.0015f);
  View.Model.l.Init(0,0,0);
  View.Camera.l.Init(0,0,-4);
  View.Prepare();
 
  // set material

  sCBuffer<sSimpleMaterialEnvPara> cb;
  cb.Data->Set(View,Env[EnvSel]);
  Mtrl->Set(&cb);

  // load vertices and indices

  sVertexStandard *vp=0L;

  Geo->BeginLoadVB(8,sGD_STREAM,&vp);
  static const sF32 N=1.0f/sFSqrt(3);
  vp[0].Init(-1,-1,-1, -N,-N,-N, 0,0);
  vp[1].Init( 1,-1,-1,  N,-N,-N, 0,1);
  vp[2].Init( 1, 1,-1,  N, N,-N, 1,1);
  vp[3].Init(-1, 1,-1, -N, N,-N, 1,0);
  vp[4].Init(-1,-1, 1, -N,-N, N, 0,1);
  vp[5].Init( 1,-1, 1,  N,-N, N, 1,1);
  vp[6].Init( 1, 1, 1,  N, N, N, 1,0);
  vp[7].Init(-1, 1, 1, -N, N, N, 0,0);
  Geo->EndLoadVB();

  sU16 *ip=0L;
  Geo->BeginLoadIB(6*6,sGD_STREAM,&ip);
  sQuad(ip,0,3,2,1,0);
  sQuad(ip,0,4,5,6,7);
  sQuad(ip,0,0,1,5,4);
  sQuad(ip,0,1,2,6,5);
  sQuad(ip,0,2,3,7,6);
  sQuad(ip,0,3,0,4,7);
  Geo->EndLoadIB();

  // draw
  Geo->Draw();

  // debug output
  sF32 avg = Timer.GetAverageDelta();
  Painter->SetTarget();
  Painter->Begin();
  Painter->SetPrint(0,0xff000000,2);
  Painter->SetPrint(0,~0,2);
  Painter->PrintF(10,10,L"%5.2ffps %5.3fms",1000/avg,avg);
  Painter->End();

  // test stuff
  sInt rnd=sGetRandomSeed()&0xffff;
  if (!(rnd&0x5c00))
    Testvalue3 = (rnd&0x3ff)-0x200;

  for (sInt i=0; i<1; i++)
  {
    {
      sPERF_SCOPE("lol1",0xff0000);
      for (sInt i=0; i<1000; i++)
      {
        sPERF_SCOPE("lol2",0x0000ff);
      }
    }
    {
      sPERF_SCOPE("lol",0xff0000);
    }
  }
}

/****************************************************************************/

// abort program when escape is pressed

void MyApp::OnInput(const sInput2Event &ie)
{
  if (sSendPerfMonInput(ie)) return;

  if (ie.Key==sKEY_ESCAPE) 
  {
    sExit(); 
  }
  else if (ie.Key==sKEY_F1) 
  {
    sTogglePerfMon(); 
  }
  else if (ie.Key==sKEY_JOYPADXBOX_START)
  {
    sTogglePerfMon();
  }
}

/****************************************************************************/

// register application class

#if sPLATFORM==sPLAT_WINDOWS
sINITMEM(sIMF_DEBUG|sIMF_CLEAR|sIMF_NORTL,64*1024*1024);
#endif

void sMain()
{
#if !sSTRIPPED
  sAddPerfMon();
#endif
  sSetPerfMonTimes(1.0f/60.0f,1.0f/60.0f);

  // start the debug server ...
  sAddDebugServer();

  // ... and add our own plugin to it.
  sAddNetDebugPlugin(MyPlugin::Factory,L"My Plugin");

  sInit(sISF_3D|sISF_CONTINUOUS|sISF_FSAA/*|sISF_FULLSCREEN*/,640,480);
  sSetApp(new MyApp());
  sSetWindowName(L"Cube");
}

/****************************************************************************/

