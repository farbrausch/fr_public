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
#include "util/shaders.hpp"
#include "base/system.hpp"

/****************************************************************************/

MyApp::MyApp()
{
  Painter = new sPainter;
  Geo = new sGeometry();
  Geo->Init(sGF_TRILIST|sGF_INDEX16,sVertexFormatStandard);

  Mtrl = new sSimpleMaterial;
  Mtrl->Flags = sMTRL_LIGHTING | sMTRL_ZON;
  Mtrl->Prepare(sVertexFormatStandard);

  Load = 1;
  

  for(sInt c=0;c<MAX_CHANNELS;c++)
    for(sInt s=0;s<MAX_STEPS;s++)
      Buffer[c][s] = sInt(sFSin(s*c*0.01f)*0x8000+0x8000);
  for(sInt c=0;c<MAX_CHANNELS;c++)
    State[c] = 0x8000;
  Index = 0;
  TimeStamp = sGetTime();
}

MyApp::~MyApp()
{
  delete Painter;
  delete Geo;
  delete Mtrl;
}

void MyApp::OnPaint3D()
{
  GetInput();
  PaintCube();
  Painter->SetTarget();
  Painter->Begin();
  Painter->SetPrint(0,~0,1);
  PrintInput();
  PrintGraph();
  Painter->End();

  // make XBOX controllers rumble...

  sJoypad *pad;
  sJoypadData data;
  sInt max = sGetJoypadCount();
  for(sInt i=0;i<max;i++)
  {
    pad = sGetJoypad(i);
    sVERIFY(pad);
    pad->GetData(data);
    pad->SetMotor(data.Pressure[10],data.Pressure[11]);
  }
}

void MyApp::PaintCube()
{
  sSetRendertarget(0,sCLEAR_ALL,0xff405060);

  Env.AmbientColor  = 0xff404040;
  Env.LightColor[0] = 0x00c00000;
  Env.LightColor[1] = 0x0000c000;
  Env.LightColor[2] = 0x000000c0;
  Env.LightDir[0].Init(1,0,0);
  Env.LightDir[1].Init(0,1,0);
  Env.LightDir[2].Init(0,0,1);

  Timer.OnFrame(sGetTime());
  sInt time = Timer.GetTime();

  View.Camera.EulerXYZ(time*0.0011f,time*0.0013f,time*0.0015f);
  View.SetTargetCurrent();
  View.SetZoom(1);
  View.Camera.l = sVector31(View.Camera.k*-2);

  sVertexStandard *vp;
  sU16 *ip;
  Geo->BeginLoadVB(8,sGD_STREAM,(void **)&vp);
  sF32 t = 0.4f/(Load);
  vp[0].Init(-t,-t,-t, -1,-1,-1, 0,0);
  vp[1].Init( t,-t,-t,  1,-1,-1, 1,0);
  vp[2].Init( t, t,-t,  1, 1,-1, 1,1);
  vp[3].Init(-t, t,-t, -1, 1,-1, 0,1);
  vp[4].Init(-t,-t, t, -1,-1, 1, 0,0);
  vp[5].Init( t,-t, t,  1,-1, 1, 1,0);
  vp[6].Init( t, t, t,  1, 1, 1, 1,1);
  vp[7].Init(-t, t, t, -1, 1, 1, 0,1);
  Geo->EndLoadVB();
  Geo->BeginLoadIB(6*6,sGD_STREAM,(void **)&ip);
  sQuad(ip,0,3,2,1,0);
  sQuad(ip,0,4,5,6,7);
  sQuad(ip,0,0,1,5,4);
  sQuad(ip,0,1,2,6,5);
  sQuad(ip,0,2,3,7,6);
  sQuad(ip,0,3,0,4,7);
  Geo->EndLoadIB();

  sF32 s = 1.0f/Load;
  for(sInt x=1-Load;x<Load;x++)
  {
    for(sInt y=1-Load;y<Load;y++)
    {
      for(sInt z=1-Load;z<Load;z++)
      {
        View.Model.l.Init(x*s,y*s,z*s);
        View.Prepare();
        //View.Set();
        sCBuffer<sSimpleMaterialEnvPara> cb;
        cb.Data->Set(View,Env);
        cb.Modify();
        Mtrl->Set(&cb);
        Geo->Draw();
      }
    }
  }

}

void MyApp::PrintInput()
{
  sF32 avg = Timer.GetAverageDelta();
  sInt y = 10;
  Painter->PrintF(10,y,L"%5.2ffps %5.3fms",1000/avg,avg); y+=10;
  Painter->PrintF(10,y,L"press keys '1'..'9' to set load"); y+=10;
}

void MyApp::GetInput()
{
  sInputFIFOEvent data;
  while(sInputQueue->GetNextEvent(data,1))
  {
    if(data.DeviceType==sIED_JOYPAD && data.DeviceId==0)
    {
      ChannelMask = data.Data.Joypad.AnalogMask;
      if(data.Timestamp > TimeStamp+1000)
      {
        TimeStamp = data.Timestamp;
      }
      while(TimeStamp < data.Timestamp)
      {
        TimeStamp+=1;
        for(sInt c=0;c<MAX_CHANNELS;c++)
          Buffer[c][Index] = State[c];
        Index = (Index+1)&(MAX_STEPS-1);
      }
      for(sInt c=0;c<MAX_CHANNELS;c++)
        State[c] = data.Data.Joypad.Analog[c];
    }
  }
}

void MyApp::PrintGraph()
{
  sF32 xo,xs,yo,ys;
  sF32 x0,x1,y0,y1;
  sInt sx,sy;

  sGetScreenSize(sx,sy);

  xo = 0;
  xs = sF32(sx)/MAX_STEPS;
  yo = sF32((sy-sy/3*2)/2);
  ys = sF32(sy/3)/0x8000;

  for(sInt c=0;c<MAX_CHANNELS;c++)
  {
    if(ChannelMask & (1<<c))
    {
      x0 = xo;
      y0 = Buffer[c][0]*ys+yo;
       
      for(sInt s=1;s<MAX_STEPS;s++)
      {
        x1 = x0;
        y1 = y0;
        x0 = s*xs+xo;
        y0 = Buffer[c][s]*ys+yo;
        Painter->Line(x0,y0,x1,y1,~0,~0,1);
      }
    }
  }

  for(sInt i=0;i<100;i+=10)
  {
    x0 = i*xs+xo;
    y0 = 0x6000*ys+yo;
    x1 = i*xs+xo;
    y1 = 0xa000*ys+yo;
    Painter->Line(x0,y0,x1,y1,~0,~0,0);
  }
}

void MyApp::OnInput(const sInput2Event &ie)
{
  switch(ie.Key)
  {
  case 27:
    sExit();
    break;
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    Load = ie.Key - '0';
    break;
  }
}

void sMain()
{
  sInit(sISF_3D|sISF_CONTINUOUS,640,480);
  sSetApp(new MyApp());
  sSetWindowName(L"Cube");
}

