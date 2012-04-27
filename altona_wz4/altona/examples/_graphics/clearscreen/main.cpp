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
/***   this example is even simpler than the cube...                      ***/
/***                                                                      ***/
/****************************************************************************/

#define sPEDANTIC_OBSOLETE 1
#define sPEDANTIC_WARN 1

#include "main.hpp"
#include "base/windows.hpp"
#include "util/image.hpp"
#include "util/shaders.hpp"

/****************************************************************************/

// initialize resources
 
MyApp::MyApp()
{
}

MyApp::~MyApp()
{
}

/****************************************************************************/

// paint a frame

void MyApp::OnPaint3D()
{ 
  sInt time = sGetTime();

  sTargetPara para(sST_CLEARALL,0);
  para.ClearColor[0].x = sFSin(time*0.0021f)*0.5f+0.5f;  // epilepsy frendly flickering :-)
  para.ClearColor[0].y = sFSin(time*0.0023f)*0.5f+0.5f;
  para.ClearColor[0].z = sFSin(time*0.0027f)*0.5f+0.5f;
  sSetTarget(para);
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
  sSetWindowName(L"Clear Screen");

  sScreenMode sm;
  sm.Clear();
  sm.Flags = sSM_VALID;
//  sm.Flags |= sSM_FULLSCREEN;
//  sm.Flags |= sSM_WARP;
  sm.ScreenX = 640;
  sm.ScreenY = 480;
  sSetScreenMode(sm);
  sInit(sISF_3D|sISF_CONTINUOUS);
  sSetApp(new MyApp());
}

/****************************************************************************/

