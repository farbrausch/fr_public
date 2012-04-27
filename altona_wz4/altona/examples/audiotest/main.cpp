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
#include "base/sound.hpp"

/****************************************************************************/

void SoundHandler(sS16 *data,sInt count)
{
  static sInt phase0,phase1;

  for(sInt i=0;i<count;i++)
  {
    phase0 = (phase0+400)&0xffff;
    phase1 = (phase1+453)&0xffff;
    data[0] = sS16(sFSin(phase0*sPI2F/0x10000)*0x7fff);
    data[1] = sS16(sFSin(phase1*sPI2F/0x10000)*0x7fff);
    data+=2;
  }
}

MyApp::MyApp()
{
  sSetSoundHandler(48000,SoundHandler,3000);
}

MyApp::~MyApp()
{
  sClearSoundHandler();
}

void MyApp::OnPaint3D()
{
  sSetRendertarget(0,sCLEAR_ALL,0xff204060);
}

void MyApp::OnInput(const sInput2Event &ie)
{
  if(ie.Key==27) sExit();
}

void sMain()
{
  sInit(sISF_3D|sISF_CONTINUOUS,640,480);
  sSetApp(new MyApp());
  sSetWindowName(L"Test");
}

