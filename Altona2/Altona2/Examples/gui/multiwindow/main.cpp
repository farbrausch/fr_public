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

class TestWindow : public sWindow
{
  int CursorX;
  int CursorY;
public:
  TestWindow()
  {
    CursorX = 10;
    CursorY = 10;
    AddDrag("Cursor",sKEY_LMB,sGuiMsg(this,&TestWindow::DragTrack));
    AddKey("Exit",sKEY_Escape|sKEYQ_Shift,sGuiMsg(this,&sWindow::CmdExit));
  }
  void OnPaint(sInt layer)
  {
    sPainter *pnt = Painter();
    sGuiStyle *sty = Style();
    sRect r;
    
    pnt->SetLayer(layer+0);
    pnt->Rect(sty->Colors[sGC_Back],Client);
    pnt->SetLayer(layer+1);
    r = Client;
    r.x0 = Client.x0 + CursorX;
    r.x1 = Client.x0 + CursorX+1;
    pnt->Rect(sty->Colors[sGC_Draw],r);
    r = Client;
    r.y0 = Client.y0 + CursorY;
    r.y1 = Client.y0 + CursorY+1;
    pnt->Rect(sty->Colors[sGC_Draw],r);
    pnt->SetLayer(layer+2);
    pnt->SetFont(sty->Font);
    pnt->Text(0,sty->Colors[sGC_Text],Client,"Hello, World!");
  }

  void DragTrack(const sDragData &dd)
  {
    if(dd.Mode==sDM_Start || dd.Mode==sDM_Drag)
    {
      CursorX = dd.PosX;
      CursorY = dd.PosY;
    }
  }
};

class MainWindow : public TestWindow
{
public:
  MainWindow()
  { 
    sScreenMode sm(sSM_FullWindowed,"Secondary Window",1280,720);
    for(int i=1;i<sGetDisplayCount();i++)
    {
      sm.Monitor = i;
      sWindow *root = Gui->OpenScreen(sm);
      sWindow *w = new TestWindow;
      w->Flags |= sWF_Back;
      root->AddChild(w);
    }
    Gui->Layout();
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
    void GetTitle(const sStringDesc &desc) { sSPrintF(desc,"Multi-Window 1"); }
};

void Altona2::Main()
{
    sRunGui(new AppInit,&sScreenMode(sSM_FullWindowed,"",0,0));
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

