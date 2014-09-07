/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Base/SystemPrivate.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>

namespace Altona2 {

using namespace Private;

//sScreenMode CurrentMode;
//sApp *CurrentApp;

bool Private::ExitFlag;

/****************************************************************************/
/***                                                                      ***/
/***   Helpers                                                            ***/
/***                                                                      ***/
/****************************************************************************/


/****************************************************************************/
/***                                                                      ***/
/***   Initialization and message loop and stuff                          ***/
/***                                                                      ***/
/****************************************************************************/

void XMainLoop();
void sRunApp(sApp *app,const sScreenMode &sm)
{
  CurrentApp = app;

  // application loop

  CurrentMode = sm;
  sSubsystem::SetRunlevel(0xff);

  if(!(sm.Flags & sSM_NoInitialScreen))
    new sScreen(sm);

  CurrentApp->OnInit();
  
  #if sConfigRender==sConfigRenderGL2
  ExitFlag = 0;
  XMainLoop();
  #endif
/*
  while(1)
  {
    if(WindowActive)
    {
      CurrentApp->OnFrame();
      Render();
    }
  }
*/
  CurrentApp->OnExit();

  sSubsystem::SetRunlevel(0x7f);
  ExitGfx();

  delete CurrentApp;
  CurrentApp = 0;
}

void sUpdateScreen(const sRect &r)
{
}

uint sGetKeyQualifier()
{
  return 0;//LastKey & 0xffff0000;
}

void sGetMouse(int &x,int &y,int &b)
{
}

void sExit()
{
  ExitFlag = 1;
}

/****************************************************************************/
/***                                                                      ***/
/***   Ios Features                                                       ***/
/***                                                                      ***/
/****************************************************************************/

void sEnableVirtualKeyboard(bool enable)
{
}

float sGetDisplayScaleFactor()
{
  return 1.0f;
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

}

/****************************************************************************/
