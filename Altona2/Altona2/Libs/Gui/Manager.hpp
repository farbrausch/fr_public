/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_GUI_MANAGER_HPP
#define FILE_ALTONA2_LIBS_GUI_MANAGER_HPP

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Gui/Style.hpp"
#include "Altona2/Libs/Gui/Window.hpp"

#include "Altona2/Libs/Util/Painter.hpp"
#include "Altona2/Libs/Util/DebugPainter.hpp"
#include "Altona2/Libs/Util/IconManager.hpp"

#include "Altona2/Libs/Gui/StyleSmall.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***   Helperclass to init and exit the gui system                        ***/
/***                                                                      ***/
/****************************************************************************/

class sGuiInitializer 
{
public:
    sGuiInitializer() {}
    virtual ~sGuiInitializer() {}

    virtual sGuiStyle *CreateStyle(sGuiAdapter *gada) { return sGetDefaultStyle(gada); }
    virtual sWindow *CreateRoot() = 0;
    virtual void GetTitle(const sStringDesc &desc) { sSPrintF(desc,"altona2 gui application"); }
};

/****************************************************************************/
/***                                                                      ***/
/***   The class that manages it all                                      ***/
/***                                                                      ***/
/****************************************************************************/

class sGuiManager : public sApp
{

    // internal

    void ListWindowsR1(sWindow *w);
    void ListWindowsR2(sArray<sWindow *> &a,sWindow *w);
    void ListWindows(sArray<sWindow *> &a);
    void PaintR(sWindow *w,int layer,const sRect &clip,sGuiScreen *gscr);
    void LayoutR0(sWindow *w);
    void LayoutR1(sWindow *w,sGuiScreen *scr);
    void LayoutR2(sWindow *w);
    void DoLayout();

    // state

    bool LayoutFlag;
    sWindow *Focus;
    sWindow *HoverFocus;
    sWindow *HoverWindow;
    int ToolTipTimer;
    int ToolTipX;
    int ToolTipY;
    int ToolTipLocked;
    sWindowDragInfo *CurrentDrag;
    sArray<sWindow *> Paint3dWindows;
    int MaxLayer;
    int LastMouseX,LastMouseY,LastMouseB,LastDeltaX,LastDeltaY;

    // quick message

    const char *QuickMsg;
    uint QuickMsgEndTime;

    // helpers

    void SetFocusRC(sWindow *w);
    void SetFocusR0(sWindow *w);
    void SetFocusR1(sWindow *w);
    sWindow *HitWindowR(sWindow *w,int x,int y);
    void TagWindows();
    void TagWindow(sWindow *w);

    // Drag & Drop

    const sDragDropIcon *DragDropIcon;

protected:
    sGuiInitializer *AppInit;

public:
    sGuiManager(sGuiInitializer *);
    ~sGuiManager();

    // public members

    sArray<sGuiScreen *> Screens;
    sArray<sGuiAdapter *> Adapters;
    sArray<sGuiMsg> PostQueue;
    bool TouchMode;
    sIconManager IconManager;

    /// app interface

    void OnInit();
    void OnExit();
    void OnFrame();
    void OnPaint();
    void OnKey(const sKeyData &kd);
    void OnDrag(const sDragData &dd);
    bool OnClose();

    // control

    void Update();
    void Update(sGuiScreen *gscr);
    void UpdateClient(sWindow *w);
    void Update(sWindow *w,const sRect &r);
    void Layout();
    void AddPopup(sWindow *w) { AddPopup(Screens[0],w); }
    void AddPulldown(sWindow *w) { AddPulldown(Screens[0],w); }

    void AddPopup(sGuiScreen *scr,sWindow *);
    void AddPulldown(sGuiScreen *scr,sWindow *,bool up=0);

    void AddDialog(sWindow *w,const sRect &r) { AddDialog(Screens[0],w,r); }
    void AddDialog(sWindow *w) { AddDialog(Screens[0],w); }
    void AddDialog(sWindow *w,int x,int y) { AddDialog(Screens[0],w,x,y); }

    void AddDialog(sGuiScreen *,sWindow *,const sRect &r);
    void AddDialog(sGuiScreen *,sWindow *);
    void AddDialog(sGuiScreen *,sWindow *,int x,int y);

    virtual sWindow *OpenScreen(const sScreenMode &sm);
    virtual sWindow *OpenScreen(sScreen *screen);

    // Helpers

    sWindow *HitWindow(sGuiScreen *gscr,int x,int y);
    void SetFocus(sWindow *w);
    sWindow *GetFocus() { return Focus; }
    int KillFlag;
    void GetMouse(int &x,int &y,int &b);
    void CalcSize(sWindow *w);
    void QuickMessage(const char *msg);
    void ClearDragDrop(sWindow *window);
    bool GetDragDropInfo(sWindow *mywindow,sDragDropIcon &icon,int &mx,int &my);
    bool DragDropActive() { return DragDropIcon!=0; }
    void ResetTooltipTimer(sWindow *win);
};

extern sGuiManager *Gui;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void sRunGui(sGuiInitializer *appinit,const sScreenMode *sm=0);
void sRunGui(sGuiInitializer *appinit,const sScreenMode *sm,sGuiManager *gui);

/****************************************************************************/
}
#endif  // FILE_ALTONA2_LIBS_GUI_MANAGER_HPP

