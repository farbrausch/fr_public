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
#define sDDB_DOUBLE   0x100       // this as a double-click!

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

/****************************************************************************/
/***                                                                      ***/
/***   The main class                                                     ***/
/***                                                                      ***/
/****************************************************************************/

class sGuiManager : public sObject
{
  sU32 FlatMatStates[256];
  sU32 AlphaMatStates[256];
  sU32 AddMatStates[256];
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
  void Button(sRect r,sBool down,sChar *text,sInt align=0,sU32 col=0);

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
  void AddWindow(sGuiWindow *win,sInt x,sInt y,sInt screen=-1);

  void ClipboardClear();
  void ClipboardAdd(sObject *);
  void ClipboardAddText(sChar *,sInt len=-1);
  sObject *ClipboardFind(sU32 cid);
  sChar *ClipboardFindText();

  sInt PropFonts[2];
  sInt FixedFonts[2];

  sInt PropFont;
  sInt FixedFont;
  sInt FlatMat;
  sInt AlphaMat;
  sInt AddMat;
  sInt CursorMat;
  sColor Palette[sGC_MAX];
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
  void AddTitle(sChar *title,sU32 flags=0);  // add sSizeBorder and set title. flags = 1 -> no sizeing
  void AddScrolling(sInt x=1,sInt y=1); // add sScrollBorder and set EnableX/Y
  sBool MMBScrolling(sDragData &dd,sInt &sx,sInt &sy); // implement left mousebutton scrolling
  sGuiWindow *FindBorder(sU32 cid);     // find a certain border window
  class sSizeBorder *FindTitle();       // find the border window that contains the titlebar

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

                                        // flags set by system
#define sGWF_HOVER        0x00010000    // mouse is hovering over this window
#define sGWF_FOCUS        0x00020000    // this is the focus window
#define sGWF_CHILDFOCUS   0x00040000    // this window or one of it's parents has the focus
#define sGWF_SCROLLX      0x00080000    // can scroll in x-direction. set by sScrollBorder
#define sGWF_SCROLLY      0x00100000    // can scroll in y-direction. set by sScrollBorder

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
  void Tag();
  void OnPaint();
  void OnLayout();
  sBool OnCommand(sU32 cmd);

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
public:
  sMenuFrame();
  void Tag();
  void OnCalcSize();
  void OnLayout();
  void OnPaint();
  void OnKey(sU32 key);
  sBool OnCommand(sU32 cmd);

  sGuiWindow *SendTo;

  sControl *AddMenu(sChar *name,sU32 cmd,sU32 shortcut);
  sControl *AddCheck(sChar *name,sU32 cmd,sU32 shortcut,sInt state);
  void AddSpacer();
  void AddColumn();
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
  void AddLabel(sChar *name,sInt x0,sInt x1,sInt y);

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

  sInt Pos[16];
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

  sInt Pos[16];
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
  sInt Maximised;
  sInt DontResize;
  void Maximise(sBool max);
};

/****************************************************************************/

class sToolBorder : public sGuiWindow
{
  sInt Height;
public:
  sToolBorder();
  void OnCalcSize();
  void OnSubBorder();
  void OnLayout();
  void OnPaint();

  sControl *AddButton(sChar *name,sU32 cmd);
  sControl *AddMenu(sChar *name,sU32 cmd);
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
  sChar *Name;
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

  void SetTab(sInt nr,sChar *name,sInt width);
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

  void SetTab(sInt nr,sChar *name,sInt width);
  sInt GetTotalWidth();
};

/****************************************************************************/
/***                                                                      ***/
/***   controls and buttons                                               ***/
/***                                                                      ***/
/****************************************************************************/

class sMenuSpacerControl : public sGuiWindow
{
public:
  void OnCalcSize();
  void OnPaint();
};

/****************************************************************************/

struct sControlTemplate
{
  sInt XPos,YPos,XSize,YSize;     // you may use this for layouting

  sInt Type;                      // sCT_???
  sChar *Name;                    // label for the control
  sInt Zones;                     // for vector controls, and sCT_MASK
  sF32 Max[4],Min[4],Step,Default[4];      // numerics
  sChar *Cycle;                   // choices for sCT_CYCLE
  sInt Size;                      // max size for sCT_STRING, also used als CycleOffset
  sU8 CycleShift;
  sU8 CycleBits;

  void Init();
  void Init(sInt type,sChar *name);
  void InitNum(sF32 min,sF32 max,sF32 step,sF32 def);
  void InitNum(sF32 *min,sF32 *max,sF32 step,sF32 *def,sInt zones);
  sInt AddFlags(sGuiWindow *win,sU32 cmd,sPtr data);
};

class sControl : public sGuiWindow
{
  sInt DragMode;
  sInt DragZone;
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


  void Init(sInt type,sU32 flags,sChar *label,sU32 cmd,sPtr data);
  void InitNum(sF32 min,sF32 max,sF32 step,sF32 def);
  void InitNum(sF32 *min,sF32 *max,sF32 step,sF32 *def,sInt zones);
  void InitCycle(sChar *choices);
  void InitTemplate(sControlTemplate *temp,sU32 cmd,sPtr data);

  void Button(sChar *label,sU32 cmd);
  void Menu(sChar *label,sU32 cmd,sU32 shortcut);
  void Label(sChar *label);
  void MenuCheck(sChar *label,sU32 cmd,sU32 shortcut,sInt state);

  void EditBool(sU32 cmd,sInt *ptr,sChar *label);
  void EditCycle(sU32 cmd,sInt *ptr,sChar *label,sChar *choices="off|on");
  void EditChoice(sU32 cmd,sInt *ptr,sChar *label,sChar *choices="off|on");
  void EditMask(sU32 cmd,sInt *ptr,sChar *label,sInt max,sChar *choices=0);

  void EditString(sU32 cmd,sChar *edit,sChar *label,sInt size);
  void EditInt(sU32 cmd,sInt *ptr,sChar *label);
  void EditFloat(sU32 cmd,sF32 *ptr,sChar *label);
  void EditHex(sU32 cmd,sU32 *ptr,sChar *label);
  void EditFixed(sU32 cmd,sInt *ptr,sChar *label);
  void EditRGB(sU32 cmd,sInt *ptr,sChar *label);
  void EditRGBA(sU32 cmd,sInt *ptr,sChar *label);
  void EditURGB(sU32 cmd,sInt *ptr,sChar *label);
  void EditURGBA(sU32 cmd,sInt *ptr,sChar *label);

  sInt Type;                      // control type: bool or string or...
  sInt Style;                     // flags
  sChar *Text;                    // text for this control
  sInt TextSize;                  // -1 for ignore (just like all Print()
  sU32 TextColor;                 // if(TextColor!=0) use this as color, don't forget alpha! (0xffff0000 = red)
  sU32 BackColor;                 // if(BackColor!=0) use this as color, don't forget alpha! (0xffff0000 = red)
  sU32 ChangeCmd;                 // command send when data changed
  sU32 DoneCmd;                   // command send when done editing
  sU32 Shortcut;                  // keyboard shortcut
  sInt State;                     // for Checkmark
  sInt LabelWidth;                // width for the label-field for those controls with external labels. (ignored for buttons and the like)

  sF32 Min[4],Max[4],Step,Default[4];  // parameters for numbers
  sInt Zones;                     // for vector edits, and bitmasks

  sChar *Cycle;                   // choices for cycle, '|' seperated
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

/****************************************************************************/

struct sListControlEntry
{
  sChar *Name;
  sInt Select;
  sInt Cmd;
};

class sListControl : public sGuiWindow
{
  sArray<sListControlEntry> List;
  sInt LastSelected;
  sInt Height;
  sInt DragStartX;
  sInt DragStartY;
public:
  sListControl();
  ~sListControl();
  void OnCalcSize();
  void OnPaint();
  void OnDrag(sDragData &dd);

  void Set(sInt nr,sChar *name,sInt cmd=-1);
  void Add(sChar *name,sInt cmd=-1);
  void Rem(sInt nr);
  void ClearList();
  sChar *GetName(sInt nr);
  sInt GetCmd(sInt cmd);
  sBool GetSelect(sInt nr);
  sInt GetCount();
  sInt GetSelect();
  void ClearSelect();
  void SetSelect(sInt nr,sBool Select);
  sInt FindName(sChar *name);


  sU32 Flags;
  sInt LeftCmd;
  sInt RightCmd;
  sInt DoubleCmd;
};

#define sLCF_ALLOCNAME    0x0001  // store copies of strings
#define sLCF_MULTISELECT  0x0002  // allow multiple selection

/****************************************************************************/
/****************************************************************************/

#endif
