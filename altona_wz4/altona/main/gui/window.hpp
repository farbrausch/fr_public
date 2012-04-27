/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_GUI_WINDOW_HPP
#define FILE_GUI_WINDOW_HPP

#ifndef __GNUC__
#pragma once
#endif

#include "base/types2.hpp"
#include "base/windows.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Helper Structures                                                  ***/
/***                                                                      ***/
/****************************************************************************/

class sWindow;
struct sWindowDrag;

typedef void(sWindow::*sCommandFunc)(sDInt);
typedef void(sWindow::*sDragFunc)(const sWindowDrag &dd,sDInt);

/****************************************************************************/

struct sWindowDrag
{
  sInt Mode;                      // sDD_xxx
  sInt Buttons;                   // LMB | RMB | MMB - 0x8000 is reserved for faking (sValueControl::OnDrag())
  sInt Flags;                     // double clicked?
  sInt MouseX;                    // current position
  sInt MouseY;
  sInt StartX;                    // position at start of drag
  sInt StartY;
  sInt DeltaX;                    // delta since start of drag
  sInt DeltaY;
  sInt HardDeltaX;                // delta since start of drag, using hardware mouse without clipping and acceleration
  sInt HardDeltaY;
};


/****************************************************************************/

struct sWindowNotify              // update when something in range changes
{
  const void *Start;
  const void *End;

  sBool Hit(const void *s,const void *e) const { return (e>Start && s<End); }
  void Clear() { Start=End=0; }
  void Add(const sWindowNotify &n) { if(End==0) { *this=n; } else { if(Start>n.Start) Start=n.Start; if(End<n.End) End=n.End; } }
};

/****************************************************************************/

struct sWindowMessage
{
  sCommandFunc Func;              // if func is set, invoke it
  sDInt Code;                     // otherwise send the command

  sWindowMessage() { Func = 0; Code = 0; }
  sWindowMessage(sDInt c) { Func = 0; Code = c; }
  template <class T> sWindowMessage(void(T::*f)(sDInt),sDInt c) { Func = (sCommandFunc) f; Code = c; }
  template <class T> sWindowMessage(void(T::*f)(sDInt)) { Func = (sCommandFunc) f; Code = 0; }

  void Send(sWindow *);           // send message or invoke func immediatly
  void Post(sWindow *);           // post message or invoke func immediatly
};

/****************************************************************************/

enum sGuiColor
{
  sGC_BACK = 1,                   // standard background color
  sGC_DOC,                        // document background color (brighter)
  sGC_BUTTON,                     // button background color (darker)
  sGC_TEXT,                       // text color
  sGC_DRAW,                       // color for drawing, usually black
  sGC_SELECT,                     // selected text
  sGC_HIGH,                       // high edge, outer
  sGC_LOW,                        // low edge, outer
  sGC_HIGH2,                      // hight edge, inner
  sGC_LOW2,                       // low edge, inner

  sGC_RED,                        // the color, with contrast to sGC_TEXT and sGC_DRAW
  sGC_YELLOW,                     // the color, with contrast to sGC_TEXT and sGC_DRAW
  sGC_GREEN,                      // the color, with contrast to sGC_TEXT and sGC_DRAW
  sGC_BLUE,                       // the color, with contrast to sGC_TEXT and sGC_DRAW
  sGC_BLACK,                      // 0x000000
  sGC_WHITE,                      // 0xffffff
  sGC_DARKGRAY,                   // 0x404040
  sGC_GRAY,                       // 0x808080
  sGC_LTGRAY,                     // 0xc0c0c0
  sGC_PINK,                       // better contrast than red to sGC_TEXT

  sGC_MAX,
};

enum sWindowDragMode
{
  sDD_HOVER = 0,
  sDD_START,
  sDD_DRAG,
  sDD_STOP,
};

enum sWindowDragFlags
{
  sDDF_DOUBLECLICK = 0x0001,      // this drag was started from a doubleclick 
};

enum sWindowCommands              // only user-commands and sCMD_DUMMY are send to parents
{
  sCMD_NULL             = 0x0000, // null messages are immediatly ignored!
  sCMD_DUMMY            = 0x0001, // dummy messages are handled normally, but should have no meaning
  sCMD_TRIGGER          = 0x0002, // used by the gui locally in different ways
  sCMD_ENTERFOCUS       = 0x0010, // focus or childfocus was set
  sCMD_LEAVEFOCUS,                // focus or childfocus was reset
  sCMD_ENTERHOVER,                // mouse hovers in (no childs)
  sCMD_LEAVEHOVER,                // mouse hovers out (no childs)
  sCMD_SHUTDOWN,                  // applcation shutdown, only send to windows which are still linked during shutdown
  sCMD_TIMER,                     // use sSetTimer() to set timer to send to ALL windows.inefficient but usefull
  sCMD_USER             = 0x1000, // here user defined CMD's may start
};

/****************************************************************************/
/***                                                                      ***/
/***   The Window Class                                                   ***/
/***                                                                      ***/
/****************************************************************************/

class sWindow : public sObject
{
  sInt MMBScrollStartX;
  sInt MMBScrollStartY;
  sInt MMBScrollMode;
public:
  sRect Outer;                    // area of window on screen
  sRect Inner;                    // area of window on screen minus border decoration
  sRect Client;                   // area to paint to, due to scrolling it may be larger than screen area or have negative coordinates
  sInt ReqSizeX;                  // requested size of paint area for OnCalcSize()
  sInt ReqSizeY;
  sInt DecoratedSizeX;            // ReqSizeX plus all borders
  sInt DecoratedSizeY;
  sInt ScrollX;                   // scrolling of paint area inside screen area
  sInt ScrollY;
  sInt MousePointer;              // sMP_???: current mouse pointer image
  const sChar *ToolTip;           // optional tooltip text for hovering
  sInt ToolTipLength;             // character count of tooltip, or -1 for null termination
  
  sInt Flags;                     // sWF_???: important behavour flags
  sInt Temp;                      // used by whoever needs something temporarly
  sInt WireSets;                  // used by wire to switch command sets in a window

  sWindow *Parent;
  sWindow *PopupParent;           // popup windows have a second parent, that is used for focus. the "real" parent is the overlapped window frame.
  sArray<sWindow *> Childs;
  sArray<sWindow *> Borders;
  sArray<sWindowNotify> NotifyList; // automatically call update when sGui::Notify is called for registered items
  sWindowNotify NotifyBounds;     // outer bounds of notify range

  void Tag();
  sCLASSNAME(sWindow);

  sWindow();
  virtual ~sWindow();
  virtual void OnBeforePaint();
  virtual void OnPaint2D();
  virtual void OnPaint3D();
  virtual void OnLayout();
  virtual void OnCalcSize();
  virtual void OnNotify(const void *ptr, sInt size);
  virtual sBool OnKey(sU32 key);
  virtual sBool OnShortcut(sU32 key);
  virtual sBool OnCommand(sInt cmd);
  virtual void OnDrag(const sWindowDrag &dd);


  void AddChild(sWindow *);
  void AddBorder(sWindow *);
  void AddBorderHead(sWindow *);
  class sButtonControl *AddButton(const sChar *label,const sMessage &cmd,sInt style=0);
  class sButtonControl *AddRadioButton(const sChar *label,sInt *ptr,sInt val,const sMessage &cmd,sInt style=0);
  class sButtonControl *AddToggleButton(const sChar *label,sInt *ptr,const sMessage &cmd,sInt style=0);
  void AddNotify(const void *,sDInt);
  template<typename Type> void AddNotify(const Type &Val) { AddNotify(&Val,sizeof(Val)); }
  void ClearNotify();
  void UpdateNotify();
  void Update();
  void Layout();
  void Close();

  void SetToolTipIfNarrow(const sChar *string,sInt len=-1,sInt space=4);

  void Post(sInt cmd);
  sBool MMBScroll(const sWindowDrag &dd);
  class sScrollBorder *AddScrolling(sBool x,sBool y);
  void ScrollTo(const sRect &vis,sBool save);
  void ScrollTo(sInt x,sInt y);
};

enum sWindowFlag
{
  sWF_FOCUS              =0x0001, // has focus
  sWF_CHILDFOCUS         =0x0002, // this or child has focus

  sWF_ZORDER_BACK        =0x0004, // always fullscreen backdrop window
  sWF_ZORDER_TOP         =0x0008, // always on top of normal windows
  sWF_OVERLAPPEDCHILDS   =0x0010, // correct clipping for overlapping childs
  sWF_CLIENTCLIPPING     =0x0020, // clip out childs before painting client

  sWF_HOVER              =0x0040, // window the mouse is hovering over
  sWF_SCROLLX            =0x0080,
  sWF_SCROLLY            =0x0100,

  sWF_3D                 =0x0200,
  sWF_NOTIFYVALID        =0x0400, // the notify range is correctly updated
  sWF_AUTOKILL           =0x0800, // window kills itself when it looses focus
  sWF_BEFOREPAINT       = 0x1000, // call OnBeforePaint()

  sWF_USER1              =0x10000000,
  sWF_USER2              =0x20000000,
  sWF_USER3              =0x40000000,
  sWF_USER4              =0x80000000,
};

/****************************************************************************/

#endif
