/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "main.hpp"

/****************************************************************************/

#include "main.hpp"
#include "base/windows.hpp"
#include "util/image.hpp"
#include "util/shaders.hpp"
#include "util/perfmon.hpp"
#include "base/input2.hpp"

#include "pong.hpp"

/****************************************************************************/

static const sInt RectMax = 4096;
static sFRect Rects[RectMax];
static sU32 Colors[RectMax];
static sInt RectUsed;
static sInput2Scheme* Scheme = 0;

void DrawRect(const sFRect &r,sU32 col)
{
  sVERIFY(RectUsed<RectMax);
  Rects[RectUsed] = r;
  Colors[RectUsed] = col;
  RectUsed ++;
}

void DrawRect(sF32 x0,sF32 y0,sF32 x1,sF32 y1,sU32 col)
{
  DrawRect(sFRect(x0,y0,x1,y1),col);
}

void GetJoypad(Joypad &joy)
{
  joy.Analog[0] = Scheme->Analog(0);
  joy.Analog[1] = Scheme->Analog(1);
  joy.Analog[2] = Scheme->Analog(2);
  joy.Analog[3] = Scheme->Analog(3);
  joy.Buttons = (Scheme->Pressed(4)) | (Scheme->Pressed(5) << 1) | (Scheme->Pressed(6) << 2) | (Scheme->Pressed(7) << 3);
}
void Print(sInt x,sInt y,const sChar *);

/****************************************************************************/

// initialize resources
 
MyApp::MyApp()
{
  // input
  Scheme = new sInput2Scheme;
  sInput2Device* joypad = sFindInput2Device(sINPUT2_TYPE_JOYPADXBOX, 0);
  Scheme->Init(8);
  Scheme->Bind(0, joypad, sINPUT2_JOYPADXBOX_LEFT_X);
  Scheme->Bind(1, joypad, sINPUT2_JOYPADXBOX_LEFT_Y);
  Scheme->Bind(2, joypad, sINPUT2_JOYPADXBOX_RIGHT_X);
  Scheme->Bind(3, joypad, sINPUT2_JOYPADXBOX_RIGHT_Y);
  Scheme->Bind(4, joypad, sINPUT2_JOYPADXBOX_A);
  Scheme->Bind(5, joypad, sINPUT2_JOYPADXBOX_B);
  Scheme->Bind(6, joypad, sINPUT2_JOYPADXBOX_X);
  Scheme->Bind(7, joypad, sINPUT2_JOYPADXBOX_Y);

  // debug output

  Painter = new sPainter;

  // geometry 

  Geo = new sGeometry();
  Geo->Init(sGF_QUADLIST,sVertexFormatSingle);

  // material

  Mtrl = new sSimpleMaterial;
  Mtrl->Flags = sMTRL_ZOFF | sMTRL_CULLOFF;
  Mtrl->Flags |= sMTRL_LIGHTING;
  Mtrl->Prepare(Geo->GetFormat());

  Game = new SoloPong;
}

// free resources (memory leaks are evil)

MyApp::~MyApp()
{
  delete Painter;
  delete Geo;
  delete Mtrl;
  delete Game;
  delete Scheme;
}

/****************************************************************************/

// paint a frame

void MyApp::OnPaint3D()
{ 
  sPERF_FUNCTION(0xffffff);

  // tick input
  sInput2Update(sGetTime());

  // Game

  RectUsed = 0;
  for(sInt i=0;i<Timer.GetSlices();i++)
    Game->OnTick();
  sVERIFY(RectUsed==0);
  Game->OnPaint();

  const sU32 bc = 0xff202020;
  DrawRect(-4,-4, 4,-1,bc);
  DrawRect(-4, 1, 4, 4,bc);
  DrawRect(-4,-1,-1, 1,bc);
  DrawRect( 1,-1, 4, 1,bc);

  // set rendertarget

  sSetRendertarget(0,sCLEAR_ALL,0xff405060);

  // get timing

  Timer.OnFrame(sGetTime());

  // set camera

  View.SetTargetCurrent();
  View.SetZoom(1.0f);
  View.Camera.l.Init(0,0,-1.4f);
  View.Prepare();
 
  // set material

  sCBuffer<sSimpleMaterialPara> cb;
  cb.Data->Set(View);
  Mtrl->Set(&cb);

  // draw

  if(RectUsed>0)
  {
    sVertexSingle *vp=0L;

    Geo->BeginLoadVB(RectUsed*4,sGD_STREAM,&vp);
    for(sInt i=0;i<RectUsed;i++)
    {
      const sFRect &r = Rects[i];
      sU32 col = Colors[i];
      vp[0].Init(r.x0,r.y0,0,col,0,0);
      vp[1].Init(r.x1,r.y0,0,col,1,0);
      vp[2].Init(r.x1,r.y1,0,col,1,1);
      vp[3].Init(r.x0,r.y1,0,col,0,1);
      vp+=4;
    }
    Geo->EndLoadVB();
  }
  Geo->Draw();

  // debug output
  sF32 avg = Timer.GetAverageDelta();
  Painter->SetTarget();
  Painter->Begin();
  Painter->SetPrint(0,~0,1);
  Painter->PrintF(10,10,L"%5.2ffps %5.3fms",1000/avg,avg);

  Painter->Print(0,10,30,~0,Game->ScoreString,-1,3);
  Painter->End();
}

/****************************************************************************/

// abort program when escape is pressed

void MyApp::OnInput(const sInput2Event &ie)
{
  if (ie.Key==sKEY_ESCAPE) 
  {
    sExit(); 
  }
  else if (ie.Key==sKEY_F1) 
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
  sInit(sISF_3D|sISF_CONTINUOUS/*|sISF_FULLSCREEN*/,640,480);
  sSetApp(new MyApp());
  sSetWindowName(L"Cube");
}

/****************************************************************************/

