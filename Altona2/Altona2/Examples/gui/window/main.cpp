/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "main.hpp"

using namespace Altona2;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class MainWindow : public sWindow
{
public:
    MainWindow()
    { 
        AddKey("Exit",sKEY_Escape|sKEYQ_Shift,sGuiMsg(this,&sWindow::CmdExit));
    }

    ~MainWindow()
    {
    }

    void OnPaint(sInt layer)
    {
        sPainter *pnt = Painter();
        sGuiStyle *sty = Style();

        pnt->SetLayer(layer+0);
        pnt->Rect(sty->Colors[sGC_Back],Client);
        pnt->SetLayer(layer+1);
        pnt->SetFont(sty->Font);
        pnt->Text(0,sty->Colors[sGC_Text],Client,"Hello, World!");
    }
};

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class AppInit : public sGuiInitializer
{
public:
    sWindow *CreateRoot() { return new MainWindow; }
    void GetTitle(const sStringDesc &desc) { sSPrintF(desc,"window"); }
};

void Altona2::Main()
{
    sRunGui(new AppInit);
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/


/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/

