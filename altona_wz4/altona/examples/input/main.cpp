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
#include "base/input2.hpp"
#include "base/windows.hpp"
#include "util/shaders.hpp"

/****************************************************************************/

MyApp::MyApp()
{
  Painter = new sPainter;
  Geo = new sGeometry();
  Geo->Init(sGF_TRILIST|sGF_INDEX16,sVertexFormatStandard);

  Mtrl = new sSimpleMaterial;
  Mtrl->Flags = sMTRL_LIGHTING | sMTRL_ZON;
  Mtrl->Prepare(sVertexFormatStandard);
}

MyApp::~MyApp()
{
  delete Painter;
  delete Geo;
  delete Mtrl;
}

void MyApp::OnPaint3D()
{
  sInput2Update(sGetTime());
  PaintCube();
  PrintInput();

  // make XBOX controllers rumble...
  sU32 deviceType = sINPUT2_TYPE_JOYPADXBOX;
  sInt count = sInput2NumDevices(deviceType);
  
  for (sInt i=0; i<count;i++)
  {
    sInput2Device *device = sFindInput2Device(deviceType,i);
    if (device)
      device->SetMotor(device->GetAbs(sINPUT2_JOYPADXBOX_LT)*255,device->GetAbs(sINPUT2_JOYPADXBOX_RT)*255);
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

  View.Model.EulerXYZ(time*0.0011f,time*0.0013f,time*0.0015f);
  View.SetTargetCurrent();
  View.SetZoom(1);
  View.Camera.l.Init(0,0,-5);
  View.Prepare();
//  View.Set();

  sCBuffer<sSimpleMaterialEnvPara> cb;
  cb.Data->Set(View,Env);
  Mtrl->Set(&cb);

  sVertexStandard *vp;
  sU16 *ip;
  Geo->BeginLoadVB(8,sGD_STREAM,(void **)&vp);
  vp[0].Init(-1,-1,-1, -1,-1,-1, 0,0);
  vp[1].Init( 1,-1,-1,  1,-1,-1, 1,0);
  vp[2].Init( 1, 1,-1,  1, 1,-1, 1,1);
  vp[3].Init(-1, 1,-1, -1, 1,-1, 0,1);
  vp[4].Init(-1,-1, 1, -1,-1, 1, 0,0);
  vp[5].Init( 1,-1, 1,  1,-1, 1, 1,0);
  vp[6].Init( 1, 1, 1,  1, 1, 1, 1,1);
  vp[7].Init(-1, 1, 1, -1, 1, 1, 0,1);
  Geo->EndLoadVB();
  Geo->BeginLoadIB(6*6,sGD_STREAM,(void **)&ip);
  sQuad(ip,0,3,2,1,0);
  sQuad(ip,0,4,5,6,7);
  sQuad(ip,0,0,1,5,4);
  sQuad(ip,0,1,2,6,5);
  sQuad(ip,0,2,3,7,6);
  sQuad(ip,0,3,0,4,7);
  Geo->EndLoadIB();
  Geo->Draw();
}

void MyApp::PrintInput()
{
  Painter->SetTarget();
  Painter->Begin();
  Painter->SetPrint(0,~0,1);
  sF32 avg = Timer.GetAverageDelta();
  sInt y = 10;
  Painter->PrintF(10,y,L"%5.2ffps %5.3fms",1000/avg,avg); y+=10;

  
  sU32 deviceType = sINPUT2_TYPE_JOYPADXBOX;
  sInt max = sInput2NumDevices(deviceType);

  for(sInt i=0;i<max;i++)
  {
    sInput2Device *device = sFindInput2Device(deviceType, i);
    Painter->PrintF(10,y,L"joy   %d:",i);  y+=10;
    PrintInput2Device(device, y);
  }

  deviceType = sINPUT2_TYPE_MOUSE;
  max = sInput2NumDevices(deviceType);
  for(sInt i=0;i<max;i++)
  {
    sInput2Device *device = sFindInput2Device(deviceType, i);
    Painter->PrintF(10,y,L"mouse   %d:",i);  y+=10;
    PrintInput2Device(device, y);
  }



  //sInt mb,mx,my,mz;
  //sGetMouseHard(mb,mx,my,mz);
  //Painter->PrintF(10,y,L"mouse 0: %08x | %08x %08x %08x",mb,mx,my,mz); y+=10;
  //Painter->PrintF(10,y,L"shift 0: %08x",sGetKeyQualifier()); y+=10;

  Painter->End();
}

void MyApp::PrintInput2Device(sInput2Device *device, sInt &y)
{
  if(device)
  {
    sTextBuffer tb;
    sArrayRange<sF32> sv  = device->GetStatusVector();
    if(!device->IsStatus(sINPUT2_STATUS_ERROR))
    {
      tb.Clear();
      sF32 *val;
      sInt printedCount = 0;
      sInt printLineLimit = 5;
      sFORALL(sv, val)
      {
        tb.PrintF(L"%2d: %f  ",_i,*val);
        printedCount++;

        // print if neccessary
        if (printedCount>=printLineLimit)
        {
          Painter->PrintF(10,y,L"%s",tb.Get());  y+=10;
          tb.Clear();
          printedCount = 0;
        }
      }

      // print rest
      if (printedCount)
      {
        Painter->PrintF(10,y,L"%s",tb.Get());  y+=10;
        tb.Clear();
        printedCount = 0;
      }
    }
    else
    {
      Painter->PrintF(10,y,L"  disconnected/error");  y+=10;
    }
  }
}

void MyApp::OnInput(const sInput2Event &ie)
{
  if(ie.Key==sKEY_ESCAPE) sExit();
}

void sMain()
{
  sInit(sISF_3D|sISF_CONTINUOUS,640,480);
  sSetApp(new MyApp());
  sSetWindowName(L"Cube");
}

