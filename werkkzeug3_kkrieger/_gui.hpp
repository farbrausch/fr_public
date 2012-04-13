// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __GUI_HPP__
#define __GUI_HPP__

#include "_util.hpp"

/****************************************************************************/

class sGuiWindow;
class sGuiManager;
extern sGuiManager *sGui;
class sClipboard;

class sControl;

#define sGUI_MAXROOTS 4           // multiscreen systems have multiple root windows!

/****************************************************************************/
/***                                                                      ***/
/***   Helpers                                                            ***/
/***                                                                      ***/
/****************************************************************************/

struct sDragData
{
  sInt Mode;                      // sDD_???
  sU32 Buttons;                   // sDDM_??? (you may hardcode these numbers)
  sInt Flags;                     // sDDF_???
  sInt MouseX,MouseY;             // current position relative to screen
  sInt DeltaX,DeltaY;             // amount of mouseticks dragged. ticks are not pixels
};

#define sDD_HOVER     1           // mouse is hovering without any button pressed
#define sDD_START     2           // the "press" event
#define sDD_DRAG      3           // mouse is dragged 
#define sDD_STOP      4           // the "release" event.

#define sDDB_LEFT     1           // left button
#define sDDB_RIGHT    2           // right button
#define sDDB_MIDDLE   4           // middle button

#define sDDF_DOUBLE   0x0001      // this as a double-click!

/****************************************************************************/

#define sGC_NONE        0         // unused palette entry
#define sGC_BACK        1         // client area clear color
#define sGC_TEXT        2         // normal text color
#define sGC_DRAW        3         // color for lines etc.
#define sGC_HIGH        4         // emboss box high color (outer)
#define sGC_LOW         5         // emboss box low color (outer)
#define sGC_HIGH2       6         // emboss box high color (inner)
#define sGC_LOW2        7         // emboss box low color (inner)
#define sGC_BUTTON      8         // button face
#define sGC_BARBACK     9         // scrollbar background
#define sGC_SELECT      10        // selected item text color
#define sGC_SELBACK     11        // selected item background color
#define sGC_MAX         12        // max color

/****************************************************************************/

#define sGS_SCROLLBARS  0x0001    // set to 0 to disable scrollbars
#define sGS_MAXTITLE    0x0002    // show window title when maximised (usefull for fullscreen apps)
#define sGS_SMALLFONTS  0x0004    // use the smallest readable fonts
#define sGS_NOCHANGE    0x0008    // don't send change cmd's (for sControl)
#define sGS_SKIN05      0x0010    // 2005 skin style

/****************************************************************************/
/***                                                                      ***/
/***   The main class                                                     ***/
/***                                                                      ***/
/****************************************************************************/

class sGuiManager : public sObject
{
//  sU32 FlatMatStates[256];
//  sU32 AlphaMatStates[256];
//  sU32 AddMatStates[256];

  sGuiWindow *Root[sGUI_MAXROOTS];
  sInt CurrentRoot;
  sU32 KeyBuffer[256];
  sInt KeyIndex;
  sU32 PostBuffer[256];
  sGuiWindow *PostBufferWin[256];
  sInt PostIndex;
  sInt NewAppPosToggle;
  sViewport Viewport;
  sInt ViewportValid;
  sGuiWindow *OnWindow;
  sList<sObject> *Clipboard;

  sInt StartGCFlag;
  sInt LastGCCount;

  sGuiWindow *Focus;
  sGuiWindow *NewFocus;
  sGuiWindow *DragWindow;
  sInt MouseX;
  sInt MouseY;
  sInt MouseStartX;
  sInt MouseStartY;
  sInt MouseAStartX;
  sInt MouseAStartY;
  sInt MouseButtons;
  sInt MouseDragButtons;
  sInt MouseTrigger;
  sInt MouseLastTime;
  sU32 GuiStyle;

  void PaintR(sGuiWindow *,sRect clip,sInt sx,sInt sy);
  sBool KeyR(sGuiWindow *win,sU32 key);
  void FrameR(sGuiWindow *win);
  void CalcSizeR(sGuiWindow *win);
  void LayoutR(sGuiWindow *win);
  sGuiWindow *FocusR(sGuiWindow *win,sInt mx,sInt my,sInt flagmask);
  void SendDrag(sDragData &dd);
public:
  sGuiManager();
  ~sGuiManager();
  sU32 GetClass() { return sCID_GUIMANAGER; }
  void Tag();

  void Bevel(sRect &r,sBool down);
  void RectHL(sRect &r,sInt w,sU32 colh,sU32 coll);
  void Button(sRect r,sBool down,const sChar *text,sInt align=0,sU32 col=0);
  void CheckBox(sRect r,sBool checked,sU32 col=0);
  void Group(sRect r,const sChar *label);

  void OnPaint();
  void OnPaint3d();
  void OnKey(sU32 key);
  void OnFrame();
  void OnTick(sInt value);
  sBool OnDebugPrint(sChar *);

  void SetRoot(sGuiWindow *root,sInt screen=0);
  sGuiWindow *GetRoot(sInt screen=0);
  sGuiWindow *GetFocus();
  void SetFocus(sGuiWindow *win);
  void SetFocus(sInt mx,sInt my);

  sBool Send(sU32 cmd,sGuiWindow *win);
  void Post(sU32 cmd,sGuiWindow *win);
  void RemWindow(sGuiWindow *win);
  void Clip(sRect &r);                      // this is clipping, not clipboard
  void ClearClip();
  void GarbageCollection() {StartGCFlag = sTRUE; }
  sU32 GetStyle();
  void SetStyle(sU32 mask);
  void GetMouse(sInt &x,sInt &y) { x=MouseX;y=MouseY; }

  void AddPulldown(sGuiWindow *win);
  void AddPopup(sGuiWindow *win);
  void AddApp(sGuiWindow *win,sInt screen=-1);
  void AddWindow(sGuiWindow *win,sInt x,sInt y,sInt screen=-1,sInt center=0);

  void ClipboardClear();
  void ClipboardAdd(sObject *);
  void ClipboardAddText(sChar *,sInt len=-1);
  sObject *ClipboardFind(sU32 cid);
  sChar *ClipboardFindText();

  sInt PropFonts[2];
  sInt FixedFonts[2];
  sInt FatFonts[2];

  sInt PropFont;
  sInt FixedFont;
  sInt FatFont;
  sInt FlatMat;
  sInt AlphaMat;
  sInt AddMat;
  sInt CursorMat;
  sInt SkinMat;
  sU32 Palette[sGC_MAX];
//  sClipboard *Clipboard;
  sRect CurrentClip;

};

/****************************************************************************/
/*
class sClipboard : public sObject
{
  sInt TextAlloc;
  sChar *Text;
public:
  sClipboard();
  ~sClipboard();
  void Tag();

  void Clear();
  void SetText(sChar *,sInt len=-1);
  sChar *GetText();
};
*/
/****************************************************************************/
/***                                                                      ***/
/***   window class                                                       ***/
/***                                                                      ***/
/****************************************************************************/

class sGuiWindow : public sObject
{
public:
  sGuiWindow();
  ~sGuiWindow();
  sU32 GetClass() { return sCID_GUIWINDOW; }
  void Tag();
  void KillMe();

  sGuiWindow *Parent;             // backlink to parent, 0 for root, parent of border is always the borders window!
  sGuiWindow *Childs;             // first child
  sGuiWindow *Borders;            // first border
  sGuiWindow *Next;               // next child / border
  sGuiWindow *Modal;              // when this window is clicked, activate modal window instead

  sU32 Flags;                     // sGWF_????
  sRect Client;                   // inner size (without border)
  sRect ClientSave;               // client without scrolling
  sRect ClientClip;               // portion of the client on-screen
  sRect Position;                 // outer size (with border)
  sRect LayoutInfo;               // used by parent for layouting
  sInt SizeX,SizeY;               // requested size of the client area
  sInt RealX,RealY;               // real size of the client area
  sInt ScrollX,ScrollY;           // amount of scrolling to add. no releayout is needed when this changes.

  void AddBorder(sGuiWindow *);   // add window as border
  void AddBorderHead(sGuiWindow *);   // add window as border
  void AddChild(sGuiWindow *);    // add window as child
  void AddChild(sInt pos,sGuiWindow *);    // add window as child, at specified position
  void RemChild(sGuiWindow *);    // remove one of it's childs;
  void RemChilds();               // remove all childs
  sInt GetChildCount();           // get number of childs (slow)
  sGuiWindow *GetChild(sInt i);   // get specific child (slow)
  sGuiWindow *SetChild(sInt i,sGuiWindow *);   // set/exchange specific child (slow), returns old one
  sGuiWindow *SetChild(sGuiWindow *old,sGuiWindow *);   // set/exchange specific child (slow), returns old one
  sBool Send(sU32 cmd);           // send command
  void Post(sU32 cmd);            // send command
  void ScrollTo(sRect vis,sInt mode=1); // scroll so that rect gets visible
  void ScrollTo(sInt x,sInt y);         // scroll upper left corner to that position
  void AddTitle(sChar *title,sU32 flags=0,sU32 closecmd=0);  // add sSizeBorder and set title. flags = 1 -> no sizeing
  void AddScrolling(sInt x=1,sInt y=1); // add sScrollBorder and set EnableX/Y
  sBool MMBScrolling(sDragData &dd,sInt &sx,sInt &sy); // implement left mousebutton scrolling
//  void WheelScrolling(sU32 key,sInt step=0); // implement mouse-wheel scrolling
  sGuiWindow *FindBorder(sU32 cid);     // find a certain border window
  class sSizeBorder *FindTitle();       // find the border window that contains the titlebar
  void SetFullsize(sBool);              // recurse to make this window fullscreen (on/off)

  virtual void OnCalcSize();            // calculate XSize and YSize
  virtual void OnSubBorder();           // (border only) subtract bordersize from parent
  virtual void OnLayout();              // (window only) set childs Position from own Client
  virtual void OnPaint();               // paint the window
  virtual void OnPaint3d(sViewport &);  // paint the window
  virtual void OnKey(sU32 key);         // process keyboard input
  virtual sBool OnShortcut(sU32 key);   // intercept keyboard input 
  virtual void OnDrag(sDragData &);     // process mouse input
  virtual void OnFrame();               // called each frame. lot's of gui stuff is done here
  virtual void OnTick(sInt ticks);      // called each frame with specified ticks. do this for simulation-timing
  virtual sBool OnCommand(sU32);        // command processing
  virtual sBool OnDebugPrint(sChar *);  // sDPrintF processing

  // a whole bunch of new painting routines for increased usability

  sU32 Pal(sU32 col);

  void Paint(sInt x,sInt y,sInt xs,sInt ys,sU32 col=sGC_BACK);
  void Paint(const sRect &r,sU32 col=sGC_BACK);
  void Paint(const sRect &pos,const sFRect &uv,sU32 col=sGC_BACK);

  void PaintAlpha(sInt x,sInt y,sInt xs,sInt ys,sU32 col=sGC_BACK);
  void PaintAlpha(const sRect &r,sU32 col=sGC_BACK);
  void PaintAlpha(const sRect &pos,const sFRect &uv,sU32 col=sGC_BACK);

  void Print(sInt x,sInt y,const sChar *text,sU32 col=sGC_TEXT,sInt len=-1);
  sInt Print(sRect &r,sInt align,const sChar *text,sU32 col=sGC_TEXT,sInt len=-1);
  sInt PrintWidth(const sChar *text,sInt len=-1);
  sInt PrintHeight();

  void PrintFixed(sInt x,sInt y,const sChar *text,sU32 col=sGC_TEXT,sInt len=-1);
  sInt PrintFixed(sRect &r,sInt align,const sChar *text,sU32 col=sGC_TEXT,sInt len=-1);
  sInt PrintFixedWidth(const sChar *text,sInt len=-1);
  sInt PrintFixedHeight();

  void PaintBevel(sRect &r,sBool down);
  void PaintRectHL(sRect &r,sInt w,sU32 colh,sU32 coll);
  void PaintButton(sRect r,sBool down,const sChar *text,sInt align=0,sU32 col=sGC_BACK);
  void PaintCheckBox(sRect r,sBool checked,sU32 col=sGC_BACK);
  void PaintGroup(sRect r,const sChar *label);

  // new skinning stuff

  void PaintGH(const sRect &pos,sU32 col0,sU32 col1);
  void PaintGV(const sRect &pos,sU32 col0,sU32 col1);

  void ClearBack(sInt colindex=sGC_BACK);
};

                                        // flags set by user
#define sGWF_DISABLED     0x00000001    // ignored for all processing
#define sGWF_UPDATE       0x00000002    // this window needs to be redrawn
#define sGWF_LAYOUT       0x00000004    // this window and it's childs needs to be layouted
#define sGWF_TOPMOST      0x00000008    // never put non-topmost in front of this
#define sGWF_BORDER       0x00000010    // this is a border, automatically set by AddBorder
#define sGWF_FLUSH        0x00000020    // flush painter before painting window and after painting childs
#define sGWF_SETSIZE      0x00000040    // set size of window after OnCalcSize is done
#define sGWF_PAINT3D      0x00000080    // call Paint3d
#define sGWF_PASSRMB      0x00000100    // clicks with the right mousebutton are passed to parent
#define sGWF_PASSKEY      0x00000200    // give OnKey to parent. usefull for borders! can still do OnShortcut()
#define sGWF_SQUEEZE      0x00000400    // don't scroll, squeeze!
#define sGWF_PASSMMB      0x00000800    // clicks with the middle mousebutton are passed to parent
#define sGWF_AUTOKILL     0x00001000    // kills automatically when focus lost (usefull for popups)
#define sGWF_PASSLMB      0x00002000    // clicks with the left mousebutton are passed to parent

                                        // flags set by system
#define sGWF_HOVER        0x00010000    // mouse is hovering over this window
#define sGWF_FOCUS        0x00020000    // this is the focus window
#define sGWF_CHILDFOCUS   0x00040000    // this window or one of it's parents has the focus
#define sGWF_SCROLLX      0x00080000    // can scroll in x-direction. set by sScrollBorder
#define sGWF_SCROLLY      0x00100000    // can scroll in y-direction. set by sScrollBorder

                                        // composite
#define sGWF_PASSMB       (sGWF_PASSLMB|sGWF_PASSRMB|sGWF_PASSMMB)
  

#define sGWS_RECT         1             // ScrollTo() so that rect is completly visible
#define sGWS_SAFE         2             // ScrollTo() with additional safety of 50 pixels
#define sGWS_SLOW         3             // Scrolling might take a second or so...


/****************************************************************************/
/***                                                                      ***/
/***   windows                                                            ***/
/***                                                                      ***/
/****************************************************************************/

class sTestWindow : public sGuiWindow
{
  sU32 LastKey;
  sU32 LastX,LastY;
public:
  sTestWindow();
  void OnCalcSize();
  void OnSubBorder();
  void OnLayout();
  void OnPaint();
  void OnKey(sU32 key);
  void OnDrag(sDragData &);
  void OnFrame();
  sBool OnCommand(sU32 cmd);
};

/****************************************************************************/

// simple to use: (new sDialogWindow)->InitOkCancecl(..); is all you need!

class sDialogWindow : public sGuiWindow
{
  sU32 CmdOk;
  sU32 CmdCancel;
  sChar *Text;
  sControl *OkButton;
  sControl *CanButton;

  sChar *EditBuffer;
  sChar *Edit;
  sInt EditSize;
  sControl *EditControl;
public:
  sDialogWindow();
  ~sDialogWindow();
  sU32 GetClass() { return sCID_DIALOGWINDOW; }
  void Tag();
  void OnPaint();
  void OnLayout();
  sBool OnCommand(sU32 cmd);
  void OnKey(sU32 key);
  void SetFocus();

  void InitString(sChar *buffer,sInt size); // use this before InitOkCancen!

  void InitAB(sGuiWindow *sendto,sChar *title,sChar *message,sU32 cmdok,sU32 cmdcan,sChar *ok,sChar *can);
  void InitOk(sGuiWindow *sendto,sChar *title,sChar *message,sU32 cmdok);
  void InitOkCancel(sGuiWindow *sendto,sChar *title,sChar *message,sU32 cmdok,sU32 cmdcan);
  void InitYesNo(sGuiWindow *sendto,sChar *title,sChar *message,sU32 cmdok,sU32 cmdcan);

  sGuiWindow *SendTo;
};

/****************************************************************************/
/***                                                                      ***/
/***   frames                                                             ***/
/***                                                                      ***/
/****************************************************************************/

class sOverlappedFrame : public sGuiWindow
{
public:
  sOverlappedFrame();
  void OnCalcSize();
  void OnLayout();
  void OnPaint();
  void OnFrame();
  void OnDrag(sDragData &dd);

  sU32 RightClickCmd;
  sGuiWindow *SendTo;
};

/****************************************************************************/

class sDummyFrame : public sGuiWindow
{
public:
  void OnCalcSize();
  void OnLayout();
  void OnPaint();
};

/****************************************************************************/

class sMenuFrame : public sGuiWindow
{
  sInt ColumnCount;
  sInt ColumnIndex[16];
  sInt ColumnWidth[16];
  sInt SortStart;
public:
  sMenuFrame();
  void Tag();
  void OnCalcSize();
  void OnLayout();
  void OnPaint();
  void OnKey(sU32 key);
  sBool OnShortcut(sU32 key);
  sBool OnCommand(sU32 cmd);

  sGuiWindow *SendTo;
  sU32 ExitKey;                   // additional key for exit, ESCAPE always works.

  sControl *AddMenu(const sChar *name,sU32 cmd,sU32 shortcut);
  sControl *AddMenuSort(const sChar *name,sU32 cmd,sU32 shortcut);
  sControl *AddCheck(const sChar *name,sU32 cmd,sU32 shortcut,sInt state);
  void AddSpacer();
  void AddColumn();
  void Sort();              // note: sort each column seperatly
};

/****************************************************************************/

class sGridFrame : public sGuiWindow
{
  sInt GridX,GridY;
  sInt MinX,MinY;
  sInt DragStartX,DragStartY;
public:
  sGridFrame();
  void OnCalcSize();
  void OnDrag(sDragData &);
  void OnLayout();
  void OnPaint();

  void SetGrid(sInt x,sInt y,sInt xs=0,sInt ys=0);
  void Add(sGuiWindow *win,sInt x0,sInt y0,sInt xs,sInt ys);
  void Add(sGuiWindow *win,sInt x0,sInt x1,sInt y);
  void Add(sGuiWindow *win,sRect r);
  void AddLabel(const sChar *name,sInt x0,sInt x1,sInt y);

  sControl *AddCon(sInt x0,sInt y0,sInt xs,sInt ys,sInt zones=0);
};

/****************************************************************************/

class sVSplitFrame : public sGuiWindow
{
  sInt Width;
  sInt Count;
  sInt DragMode;
  sInt DragStart;
public:
  sVSplitFrame();
  sU32 GetClass() { return sCID_VSPLITFRAME; }
  void OnCalcSize();
  void OnLayout();
  void OnPaint();
  void OnDrag(sDragData &dd);
  void SplitChild(sInt pos,sGuiWindow *win);
  void RemoveSplit(sGuiWindow *win);

  sInt Pos[16];
  sInt Fullscreen;        // -1 = off, 0..n = only one window
};

/****************************************************************************/

class sHSplitFrame : public sGuiWindow
{
  sInt Width;
  sInt Count;
  sInt DragMode;
  sInt DragStart;
public:
  sHSplitFrame();
  sU32 GetClass() { return sCID_HSPLITFRAME; }
  void OnCalcSize();
  void OnLayout();
  void OnPaint();
  void OnDrag(sDragData &dd);
  void SplitChild(sInt pos,sGuiWindow *win);
  void RemoveSplit(sGuiWindow *win);

  sInt Pos[16];
  sInt Fullscreen;        // -1 = off, 0..n = only one window
};


/****************************************************************************/

class sSwitchFrame : public sGuiWindow
{
  sInt Current;
public:
  sSwitchFrame();
  sU32 GetClass() { return sCID_SWITCHFRAME; }
  void OnCalcSize();
  void OnLayout();
  void OnPaint();

  void Set(sInt win);
};

/****************************************************************************/
/***                                                                      ***/
/***   borders                                                            ***/
/***                                                                      ***/
/****************************************************************************/

class sFocusBorder : public sGuiWindow
{
public:
  void OnCalcSize();
  void OnSubBorder();
  void OnPaint();
};

/****************************************************************************/

class sNiceBorder : public sGuiWindow
{
public:
  void OnCalcSize();
  void OnSubBorder();
  void OnPaint();
};

/****************************************************************************/

class sThinBorder : public sGuiWindow
{
public:
  void OnCalcSize();
  void OnSubBorder();
  void OnPaint();
};

/****************************************************************************/

class sSizeBorder : public sGuiWindow
{
  sRect DragStart;
  sRect TitleRect;
  sRect CloseRect;
  sInt DragMode;
  sInt DragClose;
  sRect MinPos;
  sInt OldMouseX;
  sInt OldMouseY;
public:
  sSizeBorder();
  sU32 GetClass() { return sCID_SIZEBORDER; }
  void OnCalcSize();
  void OnSubBorder();
  void OnPaint();
  void OnDrag(sDragData &);

  sChar *Title;
  sBool CloseCmd;
  sInt Maximised;
  sInt DontResize;
  void Maximise(sBool max);
};

/****************************************************************************/

class sToolBorder : public sGuiWindow
{
  sInt Height;
  sInt DragFakeX;
  sInt DragFakeY;
  sInt DragFakeMode;
  sInt DragMode;
  sInt DragX,DragY;
public:
  sToolBorder();
  void OnCalcSize();
  void OnSubBorder();
  void OnLayout();
  void OnPaint();
  void OnDrag(sDragData &);

  sBool MenuStyle;

  sControl *AddButton(const sChar *name,sU32 cmd);
  sControl *AddMenu(const sChar *name,sU32 cmd);
  sControl *AddLabel(const sChar *name);
  void AddContextMenu(sU32 cmd);
};

/****************************************************************************/

class sScrollBorder : public sGuiWindow
{
  sInt Width;
  sRect BodyX;
  sRect BodyY;
  sRect BodyS;
  sRect KnopX;
  sRect KnopY;

  sInt DragMode;
  sInt DragStart;
  sInt KnopXSize;
  sInt KnopYSize;
  sBool EnableKnopX;
  sBool EnableKnopY;
public:
  sScrollBorder();
  void OnCalcSize();
  void OnSubBorder();
  void OnPaint();
  void OnDrag(sDragData &dd);

  sBool EnableX;
  sBool EnableY;
};

/****************************************************************************/

struct sTabEntry
{
  sInt Width;
  const sChar *Name;
};

class sTabBorder : public sGuiWindow
{
  sInt DragMode;
  sInt DragStart;
  sInt Count;
  sInt Height;
  sTabEntry Tabs[32];
public:
  sTabBorder();
  void OnCalcSize();
  void OnSubBorder();
  void OnPaint();
  void OnDrag(sDragData &dd);

  void SetTab(sInt nr,const sChar *name,sInt width);
  sInt GetTab(sInt nr);
  sInt GetTotalWidth();
};


/****************************************************************************/

class sStatusBorder : public sGuiWindow
{
  sInt Count;
  sInt Height;
  sTabEntry Tabs[32];
public:
  sStatusBorder();
  void OnCalcSize();
  void OnSubBorder();
  void OnPaint();

  void SetTab(sInt nr,const sChar *name,sInt width);
  sInt GetTotalWidth();
};

/****************************************************************************/
/***                                                                      ***/
/***   controls and buttons                                               ***/
/***                                                                      ***/
/****************************************************************************/

class sMenuSpacerControl : public sGuiWindow
{
  const sChar *Label;
public:
  explicit sMenuSpacerControl(const sChar *name=0);
  void OnCalcSize();
  void OnPaint();
};

/****************************************************************************/

struct sControlTemplate
{
  sInt XPos,YPos,XSize,YSize;     // you may use this for layouting

  sInt Type;                      // sCT_???
  const sChar *Name;              // label for the control
  sInt Zones;                     // for vector controls, and sCT_MASK
  sF32 Max[4],Min[4],Step,Default[4];      // numerics
  const sChar *Cycle;             // choices for sCT_CYCLE
  sInt Size;                      // max size for sCT_STRING, also used als CycleOffset
  sU8 CycleShift;
  sU8 CycleBits;

  void Init();
  void Init(sInt type,sChar *name);
  void InitNum(sF32 min,sF32 max,sF32 step,sF32 def);
  void InitNum(sF32 *min,sF32 *max,sF32 step,sF32 *def,sInt zones);
  sInt AddFlags(sGuiWindow *win,sU32 cmd,sPtr data,sU32 style=0);
};

class sControl : public sGuiWindow
{
  sInt DragMode;
  sInt DragZone;
  sInt DragChanged;
  sInt LastDX;
  union
  {
    sU32 DragStartU[4];
    sS32 DragStartS[4];
    sF32 DragStartF[4];
  };
  sChar NumEditBuf[32];
  void FormatValue(sChar *buffer,sInt zone);
  sInt GetCycle();
  void SetCycle(sInt);
public:
  sControl();
  ~sControl();
  sU32 GetClass() { return sCID_CONTROL; }
  void OnCalcSize();
  void OnPaint();
  void OnKey(sU32 key);
  void OnDrag(sDragData &);
  sBool OnCommand(sU32 cmd);

  sChar *GetEditBuffer() { return NumEditBuf; }

  void Init(sInt type,sU32 flags,const sChar *label,sU32 cmd,sPtr data);
  void InitNum(sF32 min,sF32 max,sF32 step,sF32 def);
  void InitNum(sF32 *min,sF32 *max,sF32 step,sF32 *def,sInt zones);
  void InitCycle(const sChar *choices);
  void InitTemplate(sControlTemplate *temp,sU32 cmd,sPtr data);

  void Button(const sChar *label,sU32 cmd);
  void Menu(const sChar *label,sU32 cmd,sU32 shortcut);
  void Label(const sChar *label);
  void MenuCheck(const sChar *label,sU32 cmd,sU32 shortcut,sInt state);

  void EditBool(sU32 cmd,sInt *ptr,const sChar *label);
  void EditCycle(sU32 cmd,sInt *ptr,const sChar *label,const sChar *choices="off|on");
  void EditChoice(sU32 cmd,sInt *ptr,const sChar *label,const sChar *choices="off|on");
  void EditMask(sU32 cmd,sInt *ptr,const sChar *label,sInt max,const sChar *choices=0);

  void EditString(sU32 cmd,sChar *edit,const sChar *label,sInt size);
  void EditInt(sU32 cmd,sInt *ptr,const sChar *label);
  void EditFloat(sU32 cmd,sF32 *ptr,const sChar *label);
  void EditHex(sU32 cmd,sU32 *ptr,const sChar *label);
  void EditFixed(sU32 cmd,sInt *ptr,const sChar *label);
  void EditRGB(sU32 cmd,sInt *ptr,const sChar *label);
  void EditRGBA(sU32 cmd,sInt *ptr,const sChar *label);
  void EditURGB(sU32 cmd,sInt *ptr,const sChar *label);
  void EditURGBA(sU32 cmd,sInt *ptr,const sChar *label);
  void EditFRGB(sU32 cmd,sF32 *ptr,const sChar *label);
  void EditFRGBA(sU32 cmd,sF32 *ptr,const sChar *label);

  sInt Type;                      // control type: bool or string or...
  sInt Style;                     // flags
  const sChar *Text;              // text for this control
  sInt TextSize;                  // -1 for ignore (just like all Print()
  sU32 TextColor;                 // if(TextColor!=0) use this as color, don't forget alpha! (0xffff0000 = red)
  sU32 BackColor;                 // if(BackColor!=0) use this as color, don't forget alpha! (0xffff0000 = red)
  sU32 ChangeCmd;                 // command send when data changed
  sU32 DoneCmd;                   // command send when done editing
  sU32 EnterCmd;                  // command send when sKEY_ENTER is pressed
  sU32 EscapeCmd;                 // command send when sKEY_ESCAPE is pressed
  sU32 RightCmd;                  // when this is set, RMB clicks are catched!
  sU32 Shortcut;                  // keyboard shortcut
  sInt State;                     // for Checkmark
  sInt LabelWidth;                // width for the label-field for those controls with external labels. (ignored for buttons and the like)

  sF32 Min[4],Max[4],Step,Default[4];  // parameters for numbers
  sF32 DragStep;                  // step after applying all modifiers
  sInt Zones;                     // for vector edits, and bitmasks

  const sChar *Cycle;             // choices for cycle, '|' seperated
  sU8 CycleSize;                  // number of choices
  sU8 CycleShift;                 // shift cycle by this
  sU8 CycleBits;                  // number of bits. default = 0 = all bits
  sU8 CycleOffset;                // start with this value. 

  sChar *Edit;                    // edit buffer
  sInt EditSize;                  // size parameter
  sInt EditMode;                  // editing is active
  sInt EditZone;                  // when editing number
  sInt Cursor;                    // cursor position for editing
  sInt Overwrite;                 // cursor mode
  sInt EditScroll;                // scrolling for edit control
  sInt SetCursor;                 // x-position where to set cursor, or -65536
  sU8 Swizzle[4];                 // change order of zones (for rgba)

  union                           // Data Ptr
  {
    sPtr Data;
    sU8 *DataU8;
    sU32 *DataU;
    sS32 *DataS;
    sF32 *DataF;
  };
};

#define sCT_NONE            0     // uninitialised
#define sCT_LABEL           1     // static text display
#define sCT_BUTTON          2     // send action when pressed
#define sCT_TOGGLE          3     // toggle on/off
#define sCT_CYCLE           4     // cycle through choices
#define sCT_CHOICE          12    // display a popup of choices
#define sCT_MASK            13    // display a grid of checkboxes for bitmasks
#define sCT_CHECKMARK       14    // button with checkmark

#define sCT_STRING          5     // single line string editing
#define sCT_INT             6     // integer 
#define sCT_FLOAT           7     // float
#define sCT_FIXED           8     // 16:16 fixed point 
#define sCT_HEX             9     // hex editing

#define sCT_RGB             10    // RGB 32 bits per gun
#define sCT_RGBA            11    // RGBA 32 bits per gun
#define sCT_URGB            15    // RGB 8 bits per gun
#define sCT_URGBA           16    // RGBA 8 bits per gun
#define sCT_FRGB            17    // RGB float
#define sCT_FRGBA           18    // RGBA float


#define sCS_THINBORDER    0x0000  // one pixel black border
#define sCS_NOBORDER      0x0001  // don't display the border
#define sCS_FATBORDER     0x0002  // button style border
#define sCS_BORDERSEL     0x0004  // when selected, change border
#define sCS_FACESEL       0x0008  // when selected, change backcolor
#define sCS_HOVER         0x0010  // change color when mouse hovering
#define sCS_LEFT          0x0020  // align text left
#define sCS_EDIT          0x0040  // display edit-string 
#define sCS_EDITNUM       0x0080  // display number editing
#define sCS_EDITCOL       0x0100  // display color (rgb/rgba)
#define sCS_ZONES         0x0200  // display in zones.
#define sCS_GLOW          0x0400  // print "glowing" label
#define sCS_SIDELABEL     0x0800  // display label beside control
#define sCS_RIGHTLABEL    0x1000  // label will be algned right 
#define sCS_SMALLWIDTH    0x2000  // half width of fixed-width-controls
#define sCS_STATIC        0x4000  // static control, can't edit
#define sCS_GRAY          0x8000  // gray out controls, as if they are static.
#define sCS_DONTCLEARBACK 0x10000 // dont clear background. usefull for Skin05
#define sCS_FAT           0x20000 // 

/****************************************************************************/

class sImageButton : public sGuiWindow
{
  sInt Hover;
public:
  sImageButton();
  void OnCalcSize();
  void OnPaint();
  void OnDrag(sDragData &);

  void InitContext();

  sInt Cmd;

  sInt ImgSizeX;
  sInt ImgSizeY;
  sInt Material;
  sFRect OffUV;
  sFRect HoverUV;
};

/****************************************************************************/

struct sListControlEntry
{
  sChar *Name;
  sInt Select;
  sInt Level;
  sInt Hidden;
  sInt Button;                    // '+', '-', ' '
  sInt Cmd;
  sU32 Color;
};

#define sLISTCON_TABMAX   16

class sListControl : public sGuiWindow
{
  sArray<sListControlEntry> List;
  sInt LastSelected;
  sInt Height;
  sInt DragStartX;
  sInt DragStartY;
  sDialogWindow *Dialog;
  sInt DragMode;
  sInt DragLineIndex;
public:
  sListControl();
  ~sListControl();
  void OnCalcSize();
  void OnPaint();
  void OnDrag(sDragData &dd);
  void OnKey(sU32 key);
  sBool OnCommand(sU32 cmd);

  void Set(sInt nr,sChar *name,sInt cmd=-1,sU32 color=0);
  void Add(sChar *name,sInt cmd=-1,sU32 color=0);
  void Rem(sInt nr);
  void ClearList();
  void SetName(sInt nr,sChar *name);
  sChar *GetName(sInt nr);
  sInt GetCmd(sInt cmd);
  sBool GetSelect(sInt nr);
  sInt GetCount();
  sInt GetSelect();
  void ClearSelect();
  void SetSelect(sInt nr,sBool Select);
  sInt FindName(sChar *name);
  void ScrollTo(sInt nr);
  void GetTree(sInt nr,sInt &level,sInt &button);
  void SetTree(sInt nr,sInt level,sInt button);
  void CalcLevel();
  void OpenFolder(sInt nr);
  void AverageLevel(sInt nr);

  sU32 Style;
  sInt LeftCmd;
  sInt MenuCmd;
  sInt DoubleCmd;
  sInt TreeCmd;


  sInt TabStops[sLISTCON_TABMAX+1];

  // for handling
  void SetRename(sChar *buffer,sInt buffersize);
  sInt HandleIndex;     // delete, swap, xchange
  sInt InsertIndex;     // xchange
  sChar NameBuffer[64]; // add name buffer

};

#define sLCS_ALLOCNAME    0x0001  // store copies of strings
#define sLCS_MULTISELECT  0x0002  // allow multiple selection
#define sLCS_TREE         0x0004  // tree-like view
#define sLCS_HANDLING     0x0008  // hanlde add, delete, deltree, rename and move up/down
#define sLCS_DRAG         0x0010  // drag-handling exchange, duplicate
 
#define sLCS_CMD_RENAME   0x0080  // call SetRename for HandleIndex
#define sLCS_CMD_DEL      0x0081  // delete HandleIndex
#define sLCS_CMD_ADD      0x0082  // add NameBuffer at HandleIndex
#define sLCS_CMD_SWAP     0x0083  // swap HandleIndex & HandleIndex+1
#define sLCS_CMD_UPDATE   0x0084  // update list
#define sLCS_CMD_EXCHANGE 0x0085  // remove HandleIndex and insert at InsertIndex
#define sLCS_CMD_DUP      0x0086  // duplicate HandleIndex to InsertIndex

// send these to list control to start some actions
#define sCMD_LIST_ADD       0x0101
#define sCMD_LIST_DELETE    0x0102
#define sCMD_LIST_DELETE2   0x0103
#define sCMD_LIST_RENAME    0x0104
#define sCMD_LIST_RENAME2   0x0105
#define sCMD_LIST_MOVEUP    0x0106
#define sCMD_LIST_MOVEDOWN  0x0107
#define sCMD_LIST_CANCEL    0x0108

class sListHeader : public sGuiWindow
{
  sListControl *List;
  sInt TabCount;
  sChar *TabName[sLISTCON_TABMAX];

  sInt Height;
  sInt DragTab;
  sInt DragStart;

public:
  sListHeader(sListControl *list);

  void OnCalcSize();
  void OnSubBorder();
  void OnPaint();
  void OnDrag(sDragData &dd);

  void AddTab(sChar *name);
};

/****************************************************************************/
/***                                                                      ***/
/***   Operator Window                                                    ***/
/***                                                                      ***/
/****************************************************************************/

class sOpWindow : public sGuiWindow
{
protected:
  sInt PageX;                     // size of box in pixel
  sInt PageY;
  sInt PageMaxX;                  // size of page in op-slots
  sInt PageMaxY;
  sInt CursorX;                   // cursor on page in op-slots
  sInt CursorY;
  sInt CursorWidth;

  sInt Birdseye;                  // birdseye 
  sRect BirdView;
  sRect Bird;

  sInt DragClickX;                // drag information
  sInt DragClickY;
  sInt DragStartX;
  sInt DragStartY;
  sInt DragMode;
  sInt DragMouseX;
  sInt DragMouseY;
  sInt DragMoveX;
  sInt DragMoveY;
  sInt DragWidth;
  sRect DragRect;
  sInt SelectMode;

  sInt OpWindowX;                 // the operator window position
  sInt OpWindowY;
public:
  struct sOpInfo
  {
    sInt PosX;
    sInt PosY;
    sInt Width;
    sU32 Style;
    sU32 Color;
    sInt Wheel;
    sChar Name[64];
    void SetName(const sChar *str) {sCopyString(Name,str,sizeof(Name));}
    void AppendName(const sChar *str) {sAppendString(Name,str,sizeof(Name));}
    void PrefixName(const sChar *str);
  };
  sOpWindow();
  ~sOpWindow();

  void Tag();
  void OnPaint();
  void OnCalcSize();
  void OnKey(sU32 key);
  void OnDrag(sDragData &);

  void MakeRect(const sOpInfo &oi,sRect &r);
  sBool FindOp(sInt x,sInt y,sOpInfo &oi);
  sBool CheckDest(sInt x,sInt y,sInt w);
  sBool CheckDest(sBool dup);
  void AddOperatorMenu(sMenuFrame *mf);

  virtual sInt GetOpCount();
  virtual void GetOpInfo(sInt i,sOpInfo &);
  virtual void SelectRect(sRect cells,sInt mode);
  virtual void MoveDest(sBool dup);
};

#define sOIS_LOAD       0x0001
#define sOIS_STORE      0x0002
#define sOIS_SELECT     0x0004
#define sOIS_GRAYOUT    0x0008
#define sOIS_LED1       0x0010
#define sOIS_LED2       0x0020
#define sOIS_LED3       0x0040
#define sOIS_ERROR      0x0080

#define sOPSEL_TOGGLE   0x0001
#define sOPSEL_SET      0x0002
#define sOPSEL_CLEAR    0x0003
#define sOPSEL_ADD      0x0004

#define sOIC_CHANGED    0x0100
#define sOIC_MENU       0x0101
#define sOIC_SHOW       0x0102
#define sOIC_SETEDIT    0x0103
#define sOIC_ADD        0x0104

/****************************************************************************/
/***                                                                      ***/
/***   Report Window                                                      ***/
/***                                                                      ***/
/****************************************************************************/


class sReportWindow : public sGuiWindow
{
  sInt DragX,DragY;
  sInt Height,Lines;
  sInt Line;
protected:
  void PrintLine(const sChar *format,...);
  void PrintGroup(sChar *label);
public: 
  sReportWindow();
  void OnPaint();
  void OnCalcSize();
  void OnDrag(sDragData &dd);
  virtual void Print();
};

#define sLW_ROWMAX 512
class sLogWindow : public sGuiWindow
{
  sChar **Line;
  sChar *Buffer;
  sInt LineUsed;
  sInt LineMax;
  sInt BufferUsed;
  sInt BufferMax;

  sInt DragX,DragY;
  sInt Height;

  void RemLine();
public:
  sLogWindow(sInt buffermax=16*1024,sInt linemax=1024);
  ~sLogWindow();
  void PrintFLine(const sChar *format,...);
  void PrintLine(const sChar *);

  void OnPaint();
  void OnCalcSize();
  void OnDrag(sDragData &dd);
};


/****************************************************************************/
/***                                                                      ***/
/***   text editor                                                        ***/
/***                                                                      ***/
/****************************************************************************/

class sTextControl : public sGuiWindow
{
  sInt Cursor;
  sInt Overwrite;
  sInt SelPos;
  sInt SelMode;                   // 0 = off, 1 = normal, 2 = rect    
  sInt RecalcSize;
  sInt MMBX,MMBY;

  void ScrollToCursor();
  sInt ClickToPos(sInt x,sInt y);
  void Realloc(sInt size);
public:
  sTextControl();
  ~sTextControl();
  void OnCalcSize();
  void OnPaint();
  void OnKey(sU32 key);
  void OnDrag(sDragData &);
  void Tag();

  void SetText(class sText *);
  void SetText(sChar *);
  sChar *GetText();
  void Engine(sInt pos,sInt len,sChar *insert);
  void DelSel();
  sInt GetCursorX();
  sInt GetCursorY();
  void SetCursor(sInt x,sInt y);
  void SetSelection(sInt cursor,sInt sel);
  void Log(sChar *);              // append string to editbuffer, usefull for log-windows          
  void ClearText();               // clears all text

  void CmdCut();
  void CmdCopy();
  void CmdPaste();
  void CmdBlock();                // mark block start


  sInt CursorWish;                // imagine you scroll down into an empty line and scroll one more down into a full one. in which column should the cursor jump?
  sInt DoneCmd;
  sInt ChangeCmd;
  sInt CursorCmd;
  sInt ReallocCmd;
  sInt MenuCmd;
  class sText *TextBuffer;
  sInt Changed;
  sInt Static;                    // dont allow changing, usefull for log-windows
};

/****************************************************************************/

#endif
