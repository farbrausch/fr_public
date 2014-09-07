/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "altona2/libs/base/base.hpp"
#include "altona2/libs/gui/gui.hpp"
#include "main.hpp"
#include "gui.hpp"

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class AppInit : public sGuiInitializer
{
public:
    sGuiStyle *CreateStyle(sGuiAdapter *gada) { return sGetDefaultStyle(gada); }
    sWindow *CreateRoot() { return new MainWindow; }
    void GetTitle(const sStringDesc &desc) { sSPrintF(desc,"Installer"); }
};

void Altona2::Main()
{
    sRunGui(new AppInit,&sScreenMode(0,0,640,480));
}

/****************************************************************************/

