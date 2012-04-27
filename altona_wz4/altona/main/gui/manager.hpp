/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_GUI_MANAGER_HPP
#define FILE_GUI_MANAGER_HPP

#ifndef __GNUC__
#pragma once
#endif

#include "base/types2.hpp"
#include "base/windows.hpp"
#include "util/painter.hpp"
#include "gui/window.hpp"


/****************************************************************************/
/***                                                                      ***/
/***   Theme support                                                      ***/
/***                                                                      ***/
/****************************************************************************/

struct sGuiTheme
{
  sU32 BackColor;
  sU32 DocColor;
  sU32 ButtonColor;
  sU32 TextColor;
  sU32 DrawColor;
  sU32 SelectColor;
  sU32 HighColor;
  sU32 LowColor;
  sU32 HighColor2;
  sU32 LowColor2;

  sString<64> PropFont;
  sString<64> FixedFont;

  template <class streamer> void Serialize_(streamer &stream);
  void Serialize(sWriter &s);
  void Serialize(sReader &s);

  void Tint(sU32 add,sU32 sub);
};

extern const sGuiTheme sGuiThemeDefault;
extern const sGuiTheme sGuiThemeDarker;

/****************************************************************************/
/***                                                                      ***/
/***   The Gui Manager Class                                              ***/
/***                                                                      ***/
/****************************************************************************/

enum sGuiPaintButtonFlags
{
  sGPB_DOWN = 1,
  sGPB_GRAY = 2,
};


class sGui_ : public sObject
{
private:
  enum
  {
    sIED_NONE = 0,
    sIED_KEYBOARD = 1,
    sIED_MOUSE = 2,
    sIED_JOYPAD = 3,
    sIED_MOUSEHARD = 4,
    sIED_MAX,
  };

  friend class sGuiApp;

  sBool LayoutFlag;
  sBool Shutdown;
  sBool Paint3dFlag;

  sRect Client;
  sWindow *Root;
  sWindow *Focus;
  sWindow *Hover;

  sRect ToolTipRect;

  sInt HardMouseX;
  sInt HardMouseY;
  sInt HardStartX;
  sInt HardStartY;
  
  sImage2D *BackBuffer;
  sBool BackBufferUsed;
  sRect BackBufferRect;

  sArray<sWindow *> Window3D;       // while scanning the Region3D, crate a list of windows that have 3d painting
public:
  sRectRegion Region3D;
private:

  sWindowDrag DragData;
  sBool Dragging;

  struct PostQueueEntry
  {
    sWindow *Window;
    sInt Command;
  };
  struct Event
  {
    sU8 Mode;
    sS8 MouseButton;
    sU32 Key;
    sInt MouseX;
    sInt MouseY;
    sInt HardX;
    sInt HardY;
  };
  sArray<PostQueueEntry> PostQueue;
  PostQueueEntry AsyncEntry;
  sThreadLock AsyncLock;

  sArray<Event> EventQueue;
  sBool QueueEvents;              // stop processing inputs immediatly and start queueing them
  sBool DontQueueEvents;          // this is a bad moment to start queueing, because we are just processing WM_PAINT

  class sGuiApp *GuiApp;
  void RecCalcSize(sWindow *);
  void RecLayout(sWindow *);
  void RecPaint(sWindow *,const sRect &update);
  void RecPaint3d(sWindow *,const sRect &wr);
  sBool RecShortcut(sWindow *w,sU32 key);
  sWindow *RecHitWindow(sWindow *,sInt x,sInt y,sBool border) const;
  void SendMouse(const Event &ie);
  void OnPrepareFrame();
  void OnPaint(const sRect &client,const sRect &update);
  void OnPaint3d();
  void SendKey(sU32 key);
  void OnEvent(sInt event);
  void OnInput(const sInput2Event &ie);
  void DoInput(const Event &ie);
  void PositionWindow(sWindow *w,sInt x,sInt y);
  void CheckToolTip();
 
  void RecCommand(sWindow *,sInt cmd);
  void ProcessPost();
  void RecNotifyMake(sWindow *p);
  void RecNotify(sWindow *p,const void *start,const void *end);
public:
  sCLASSNAME(sGui_);
  sGui_();
  ~sGui_();
  void Tag();
  sBool HandleShiftEscape;        // this this to 0 if you want to handle SHIFT ESCAPE on your own
  sBool TabletMode;

  void SetTheme(const sGuiTheme &theme);

  // painting resources

  sFont2D *PropFont;
  sFont2D *FixedFont;
  void RectHL(const sRect &r,sInt colh,sInt coll) const;
  void RectHL(const sRect &r, sBool invert=sFALSE) const; // use default colors for thin 3d border
  void PaintHandle(sInt x,sInt y,sBool select) const;
  sBool HitHandle(sInt x,sInt y,sInt mx,sInt my) const;
  void PaintButtonBorder(sRect &r,sBool pressed) const;
  void PaintButton(const sRect &rect,const sChar *text,sInt flags,sInt len=-1,sU32 backcolor=0) const;
  void BeginBackBuffer(const sRect &rect);
  void EndBackBuffer();

  // window handling

  void SetRoot(sWindow *);        // you should not change the root window, it is provided automatically
  void SetFocus(sWindow *);       // force focus to another window
  sWindow *GetFocus() { return Focus; }
  void Layout();                  // force new layout and repaint all
  void Layout(const sRect &r);    // force new layout and repaint only a portion 
  void CalcSize(sWindow *w) {RecCalcSize(w);} // calculate required size of a window subtree, even before it gets connected to the whole thing
  sWindow *HitWindow(sInt x,sInt y);  // see what window you woudl hit
  void Post(sWindow *win,sInt cmd); // post a message in the message-queue. it will be handled at the end of the frame
  void Send(sWindow *win,sInt cmd); // send a message immediatly.
  void Update(const sRect &r);    // repaint windows in this rectangle. do not call sUpdateWindow(...) directly!
  void Update();
  void PostAsync(sWindow *win,sInt cmd); // post a message in the message-queue. it will be handled at the end of the frame
  void Notify(const void *ptr,sDInt n);
  template<typename Type> void Notify(const Type &Val) { Notify(&Val,sizeof(Val)); }
  void CommandToAll(sInt cmd,sWindow *w=0);
  void GetMousePos(sInt &x,sInt &y) { x=DragData.MouseX; y=DragData.MouseY; }

  // adding windows to the main window

  void AddWindow(sWindow *);      // low level add window
  void AddBackWindow(sWindow *);  // add a window that fills the whole screen
  void AddFloatingWindow(sWindow *,const sChar *title, sBool closeable=sFALSE); // add a floating window with a size border and title
  void AddPopupWindow(sWindow *); // add a popup window where the mouse is.
  void AddWindow(sWindow *,sInt x,sInt y); // add a popup window where the mouse is.
  void AddCenterWindow(sWindow *w);   // add window in center of screen
  void AddPulldownWindow(sWindow *);  // add a pulldown window under the focus window
  void AddPulldownWindow(sWindow *,const sRect &client);  // add a pulldown window, specify client of parent manually
};

extern sGui_ *sGui;
void sInitGui(class sGuiApp *newGuiApp=sNULL);
void sOBSOLETE sInitOldGui(class sGuiApp *newGuiApp=sNULL); // inits gui with old look

class sGuiApp : public sApp
{
  sFont2D *Font;
public:
  sGuiApp();
  ~sGuiApp();

  void OnPrepareFrame();
  void OnPaint2D(const sRect &client,const sRect &update);
  void OnPaint3D();
  void OnInput(const sInput2Event &ie);
  void OnEvent(sInt id);
};

/****************************************************************************/

#endif
