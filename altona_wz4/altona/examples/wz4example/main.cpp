/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "main.hpp"
#include "gui/gui.hpp"
#include "base/windows.hpp"

#include "wz4lib/gui.hpp"
#include "wz4lib/serials.hpp"
#include "wz4lib/doc.hpp"
#include "wz4lib/build.hpp"
#include "wz4lib/poc_ops.hpp"
#include "wz4lib/version.hpp"

/****************************************************************************/

void RegisterWZ4Classes()
{
  for(sInt i=0;i<2;i++)
  {
    sREGOPS(basic,0);
    sREGOPS(poc,0);
  }
}

/****************************************************************************/

void sMain()
{
  sInit(sISF_2D|sISF_3D ,1280,1024);
  sInitGui();
  sSetWindowMode(sWM_MAXIMIZED);

  new wDocument();
  sGui->AddBackWindow(new MainWindow);
  App->WikiPath = L"c:/svn2/wz4wiki";
  App->WikiCheckout = L"svn checkout svn://192.168.0.17/wz4wiki c:/svn2/wz4wiki --username wiki --password qwer12";
  App->UnitTestPath = L"c:/svn2/altona/examples/wz4example/unittest";
  App->MainInit();

  sString<sMAXPATH> name;
  name.PrintF(L"wz4example V%d.%d",WZ4_VERSION,WZ4_REVISION);
  sSetWindowName(name);
}

/****************************************************************************/
