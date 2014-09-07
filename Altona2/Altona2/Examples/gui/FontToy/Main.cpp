/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Gui/Gui.hpp"
#include "Altona2/Libs/Util/Config.hpp"
#include "Main.hpp"
#include "Gui.hpp"

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class AppInit : public sGuiInitializer
{
public:
    sWindow *CreateRoot() { return new MainWindow; }
    void GetTitle(const sStringDesc &desc) { sSPrintF(desc,"FontToy"); }
};

void Altona2::Main()
{
    sEnableAltonaConfig();
    sRunGui(new AppInit);
}

/****************************************************************************/

