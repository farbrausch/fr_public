/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "main.hpp"
#include "base/windows.hpp"
#include "util/image.hpp"
#include "util/shaders.hpp"

#include "util/taskscheduler.hpp"
#include "extra/mcubes.hpp"
#include "water.hpp"

/****************************************************************************/

// initialize resources
 
MyApp::MyApp()
{
  // debug output

  Painter = new sPainter;
  Timer.SetHistoryMax(4);

  // mcubes

  MC = new MarchingCubes;
  sSched = new sStsManager(1024*1024,512);

  // fx

  Water = new WaterFX;
  Water->AddRain(20000,0xffffffff);
  Drop = new WaterFX;
  Drop->AddDrop(1500,0xffff0000,0.5f);
  Drop->GravityY = 0;
  Water->CentralGravity = 0;

  Water->GravityY                               = -0.00004f;
                          Drop->CentralGravity  = -0.00005f;
  Water->OuterForce     = Drop->OuterForce      = -0.001f;    // anziehung
  Water->InnerForce     = Drop->InnerForce      =  0.01f;     // abstßung
  Water->InteractRadius = Drop->InteractRadius  =  0.1f;
  Water->Friction       = Drop->Friction        =  0.9995f;

  // texture

  sImage img(256,256);
  img.Fill(0x00000000);
  img.Glow(0.5f,0.5f,0.5f,0.5f,0xffffffff,1.0f,0.5f);
  Tex = sLoadTexture2D(&img,sTEX_2D|sTEX_ARGB8888);

  // material

  Mtrl = new sSimpleMaterial;
//  Mtrl->Flags = sMTRL_ZREAD | sMTRL_CULLON;
//  Mtrl->BlendColor = sMB_ADD;
//  Mtrl->Texture[0] = Tex;
  Mtrl->TFlags[0] = sMTF_LEVEL2|sMTF_CLAMP;
  Mtrl->Flags |= sMTRL_LIGHTING;
  Mtrl->Prepare(sVertexFormatStandard);

  Env.LightDir[0].Init(-1,-1,-1);
  Env.LightDir[1].Init(1,1,1);
  Env.LightColor[0] = 0x00c08080;
  Env.LightColor[1] = 0x00004040;
  Env.AmbientColor = 0xff202020;
  Env.Fix();

  Cam.SetPos(sVector31(0,0,-40));
}

// free resources (memory leaks are evil)

MyApp::~MyApp()
{
  delete Painter;
  delete Mtrl;
  delete Tex;
  delete Water;
  delete Drop;
  delete MC;
  delete sSched;
}

/****************************************************************************/

// paint a frame

void MyApp::OnPaint3D()
{ 
  WaterParticle *p;

  // set rendertarget

  sSetRendertarget(0,sCLEAR_ALL,0xff405060);

  // get timing

  Timer.OnFrame(sGetTime());
  static sInt time;
  if(sHasWindowFocus())
    time = Timer.GetTime();
  Cam.OnFrame(Timer.GetSlices(),Timer.GetJitter()/10.0f);
  Cam.MakeViewport(View);
 
  // marching cubes

  sStsWorkload *wl = sSched->BeginWorkload();


  MCParts.Clear();
  MCParts.Resize(Drop->GetCount()+Water->GetCount());
  sInt n=0;
  sFORALL(Drop->GetArray(),p)
    MCParts[n++] = sVector31(sVector30(p->OldPos)*10.0f);
  sFORALL(Water->GetArray(),p)
    MCParts[n++] = sVector31(sVector30(p->OldPos)*10.0f);
  MC->Begin(sSched,wl);

  // simulate

  static sInt step = 0000;
  step+=8;

//  if(step==000)
//    Drop->CentralGravity = 0;

  if(step<10000)
    Drop->Step(sSched,wl);
  if(step==10000)
  {
    sFORALL(Drop->GetArray(),p)
    {
      p->NewPos.y -= 0.5f;
      p->OldPos.y -= 0.5f;
      p->OldPos.y += 0.02f;
    }
    Water->GetArray().Add(Drop->GetArray());
    Drop->Reset();
  }

  if(step%20000==0 && step>=20000)
    Water->Nudge(sVector30(0.01f,0,0));


  Water->Step(sSched,wl);

  // multithread

  wl->Start();
  MC->Render(MCParts.GetData(),MCParts.GetCount());
  wl->Sync();
  wl->End();
  MC->End();

  // end marching cubes and draw

  sCBuffer<sSimpleMaterialEnvPara> cb;
  cb.Data->Set(View,Env);
  Mtrl->Set(&cb);
  MC->Draw();

  // debug output

  sF32 avg = Timer.GetAverageDelta();
  Painter->SetTarget();
  Painter->Begin();
  Painter->SetPrint(0,0xff000000,2);
  Painter->SetPrint(0,~0,2);
  Painter->PrintF(10,10,L"%5.2ffps %5.3fms step %d",1000/avg,avg,step);
  Painter->End();
}

/****************************************************************************/

// abort program when escape is pressed

void MyApp::OnInput(const sInput2Event &ie)
{
  Cam.OnInput(ie);
  sU32 key = ie.Key;
  if(key & sKEYQ_SHIFT) key |= sKEYQ_SHIFT;
  if(key & sKEYQ_CTRL ) key |= sKEYQ_CTRL;
  switch(ie.Key)
  {
  case sKEY_ESCAPE:
  case sKEY_ESCAPE|sKEYQ_SHIFT:
    sExit();
    break;
  }
}

/****************************************************************************/

// register application class

void sMain()
{
  sSetWindowName(L"Water");

  sInit(sISF_3D|sISF_CONTINUOUS|sISF_FSAA|sISF_NOVSYNC/*|sISF_FULLSCREEN*/,1280,720);
  sSetApp(new MyApp());
}

/****************************************************************************/

