/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_GUI_FRAMES_HPP
#define FILE_GUI_FRAMES_HPP

#ifndef __GNUC__
#pragma once
#endif

#include "base/math.hpp"
#include "gui/window.hpp"
#include "gui/manager.hpp"
#include "gui/controls.hpp"

/****************************************************************************/

class sSplitFrame : public sWindow
{
protected:
  struct ChildDataStruct
  {
    sInt StartPos;
    sInt RelPos;
    sInt Pos;
    sInt Align;
  };
  sArray<ChildDataStruct> ChildData;
  sInt Initialized;
  sInt Drag;
  sInt DragStart;
  sInt Count;
  sInt OldT;
  sInt RelT;
  void MakeChildData();
public:
  sCLASSNAME(sSplitFrame);
  sSplitFrame();
  void SplitLayout(sInt w);
  sInt SplitDrag(const sWindowDrag &dd,sInt mousedelta,sInt mousepos);
  void Preset(sInt splitter,sInt value,sBool align=0);
  void PresetPos(sInt splitter,sInt value);
  void PresetAlign(sInt splitter,sBool align);
  sInt GetPos(sInt splitter);
  sInt Knop;
  sBool Proportional;
};

class sHSplitFrame : public sSplitFrame
{
public:
  sCLASSNAME(sHSplitFrame);
  sHSplitFrame();
//  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();
  void OnDrag(const sWindowDrag &dd);
};

class sVSplitFrame : public sSplitFrame
{
public:
  sCLASSNAME(sVSplitFrame);
  sVSplitFrame();
//  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();
  void OnDrag(const sWindowDrag &dd);
};

/****************************************************************************/

class sMenuFrame : public sWindow
{
  struct Item
  {
    sMessage Message;
    sU32 Shortcut;
    sInt Column;
    sWindow *Window;
  };
  const static sInt MaxColumn = 34;
  sArray<Item> Items;
  void CmdPressed(sDInt);
  void CmdPressedNoKill(sDInt);
  void Kill();

  sInt ColumnWidth[MaxColumn];
  sInt ColumnHeight[MaxColumn];
public:
  sCLASSNAME(sMenuFrame);
  sMenuFrame();
  sMenuFrame(sWindow *sendto);    // add thickborder, make "sendto" focus after menu
  ~sMenuFrame();
  void Tag();
  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();
  sBool OnShortcut(sU32 key);
//  sBool OnCommand(sInt cmd);

  void AddItem(const sChar *name,const sMessage &cmd,sU32 Shortcut,sInt len=-1,sInt column=0,sU32 backcol=0);
  void AddCheckmark(const sChar *name,const sMessage &cmd,sU32 Shortcut,sInt *refptr,sInt value,sInt len=-1,sInt column=0,sU32 backcol=0,sInt buttonstyle=0);
  void AddSpacer(sInt column=0);
  void AddHeader(sPoolString name,sInt column=0);
  void AddChoices(const sChar *choices,const sMessage &msg);

  sWindow *SendTo;          // only used to set focus
};


void sPopupChoices(const sChar *choices,const sMessage &msg); // make an sMenuFrame with choices from popup

/****************************************************************************/

enum sLayoutFrameWindowMode
{
  sLFWM_WINDOW,
  sLFWM_HORIZONTAL,
  sLFWM_VERTICAL,
  sLFWM_SWITCH,
  sLFWM_BORDER,
  sLFWM_BORDERPRE,
};

struct sLayoutFrameWindow
{
  sLayoutFrameWindowMode Mode;
  sInt Pos;                       // default position, negative is bottom/left aligned
  sInt Align;
  sInt Temp;                      // this is not used. but sWire need it!
  sWindow *Window;                // if sLFWM_WINDOW
  sInt Switch;                    // if sLFWM_SWITCH
  sPoolString Name;               // optional name, required to switch switch windows
  sArray<sLayoutFrameWindow *> Childs;
  sBool Proportional;

  sMessage OnSwitch;

  sLayoutFrameWindow(sLayoutFrameWindowMode mode,sWindow *window=0,sInt pos=0);
  ~sLayoutFrameWindow();
  void Add(sLayoutFrameWindow *w);
  void Layout(sWindow *win,class sLayoutFrame *root);
  void Cleanup(sWindow *win,class sLayoutFrame *root);
};

class sLayoutFrame : public sWindow
{
  sInt CurrentScreen;
  void SetSubSwitchR(sLayoutFrameWindow *p,sPoolString name,sInt nr);
  void GetSubSwitchR(sLayoutFrameWindow *p,sPoolString name,sInt &nr);
public:
  sCLASSNAME(sLayoutFrame);
  sLayoutFrame();
  ~sLayoutFrame();
  void Tag();
/*
  void OnCalcSize();
  void OnLayout();
*/
  void Switch(sInt screen);
  sInt GetSwitch() { return CurrentScreen; }
  void SetSubSwitch(sPoolString name,sInt nr);    // switch window deeper in hirarchy
  sInt GetSubSwitch(sPoolString name);

  sArray<sLayoutFrameWindow *> Screens;
  sArray<sWindow *> Windows;
};

class sSwitchFrame : public sWindow
{
  sInt CurrentScreen;
public:
  sCLASSNAME(sSwitchFrame);
  sSwitchFrame();
  ~sSwitchFrame();
  void Tag();
  void Switch(sInt screen);
  sInt GetSwitch() { return CurrentScreen; }

  sArray<sWindow *> Windows;
};

/****************************************************************************/

enum sGridFrameLayoutFlags
{
  sGFLF_GROUP = 1,                // display label as group. has different layout.
  sGFLF_NARROWGROUP = 2,          // add this to group for a more narrow display
  sGFLF_HALFUP = 4,               // moves the box up by half a gridheight
  sGFLF_CENTER = 8,               // center horizontally
  sGFLF_LEAD = 16,                // draw leading - a line that makes tables to read easier
};

struct sGridFrameLayout           // discribe the content of the grid window!
{
  sRect GridRect;                 // a gridcell
  sWindow *Window;                // is either a window
  const sChar *Label;             // or a static label string
  sInt Flags;                     // sGFLF
};                                // every child has to be in this array!

class sGridFrame : public sWindow
{
public:
  sCLASSNAME(sGridFrame);
  sGridFrame();
  ~sGridFrame();
  void Tag();

  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();
  void OnDrag(const sWindowDrag &dd);
  sBool OnKey(sU32 key);

  sArray<sGridFrameLayout> Layout;
  sInt Columns;
  sInt Height;

  void Reset();
  void AddGrid(sWindow *,sInt x,sInt y,sInt xs,sInt ys=1,sInt flags=0);
  void AddLabel(const sChar *,sInt x,sInt y,sInt xs,sInt ys=1,sInt flags=0);
};

struct sGridFrameHelper
{
  // configuration
  sGridFrame *Grid;               // link to grid, should be 12 wide
  sInt LabelWidth;                // width of a label, usually 3
  sInt ControlWidth;              // width of average control, usually 2
  sInt WideWidth;                 // width of wide controls, like strings.
  sInt BoxWidth;                  // width of a box, usually 1
  sMessage DoneMsg;               // message to add for "done" event
  sMessage ChangeMsg;             // message to add for "change" event
  sBool Static;                   // create controls as static (if possible)
  
  sF32 ScaleRange;
  sF32 RotateRange;
  sF32 TranslateRange;
  sF32 ScaleStep;
  sF32 RotateStep;
  sF32 TranslateStep;

  // private layut counters
  sInt Line;                      // current line
  sInt Left;                      // controls are layouted from left to right
  sInt Right;                     // boxes are layouted from right to left
  sInt EmptyLine;                 // a new line has already been started.
  sInt TieMode;                   // 0=off, 1=first, 2=cont
  sStringControl *TiePrev,*TieFirst;

  // general
  sGridFrameHelper(sGridFrame *); // initialize and connect to frame
  ~sGridFrameHelper();            // delete DoneMsg and ChangeMsg
  void Reset();                   // reset frame.
  void NextLine();                // advance to next line
  void InitControl(class sControl *con); // used internally to copy messaging to control
  void BeginTied();               // tie together controls so thay can be dragged together with CTRL
  void EndTied();
  void Tie(sStringControl *);      // tie this control, automatically called for INT, FLOAT and BYTE.
  void SetColumns(sInt left,sInt middle,sInt right);
  void MaxColumns(sInt left,sInt middle,sInt right);

  // special controls
  void Label(const sChar *label); // start new line and add label at the left
  void LabelC(const sChar *label); // label withouzt new line
  void Group(const sChar *label=0); // begin a new group in new line
  void GroupCont(const sChar *label=0); // begin a new group in current line (rarely used)
  void Textline(const sChar *text=0); // one line of text
  class sButtonControl *PushButton(const sChar *label,const sMessage &done,sInt layoutflags=0);    // add pushbutton, large one from the left
  class sButtonControl *Box(const sChar *label,const sMessage &done,sInt layoutflags=0);           // add pushbutton, small one from the right
  class sButtonControl *BoxToggle(const sChar *label,sInt *x,const sMessage &done,sInt layoutflags=0);           // add pushbutton, small one from the right
  void BoxFileDialog(const sStringDesc &string,const sChar *text,const sChar *ext,sInt flags=0);

  // value controls
  void Radio(sInt *val,const sChar *choices=L"off|on",sInt width=-1);                 // create radiobuttons from left to right. you may overwrite the width
  sChoiceControl *Choice(sInt *val,const sChar *choices=L"off|on");             // dropdownlist with choices, or toggle if only two choices
  sControl *Flags(sInt *val,const sChar *choices=L"off|on");                         // build multiple buttons separated by colon: "*0on|off:*1a|b|c|d:*3bli|bla"
  sButtonControl *Toggle(sInt *val,const sChar *label);                              // create toggle button
  sButtonControl *Button(const sChar *label,const sMessage &msg);                          // create pushbutton
  void Flags(sInt *val,const sChar *choices,const sMessage &msg);
  sStringControl *String(const sStringDesc &string,sInt width=0);
  sStringControl *String(sPoolString *pool,sInt width=0);
  sStringControl *String(sTextBuffer *tb,sInt width=0);
  class sTextWindow *Text(sTextBuffer *tb,sInt lines,sInt width=0);
  sFloatControl *Float(sF32 *val,sF32 min,sF32 max,sF32 step=0.25f,sF32 *colptr=0);
  sIntControl *Int(sInt *val,sInt min,sInt max,sF32 step=0.25f,sInt *colptr=0,const sChar *format=0);
  sByteControl *Byte(sU8 *val,sInt min,sInt max,sF32 step=0.25f,sU8 *colptr=0);
  sWordControl *Word(sU16 *val,sInt min,sInt max,sF32 step=0.25f,sU16 *colptr=0);
  void Color(sU32 *,const sChar *config);
  void ColorF(sF32 *,const sChar *config);
  void ColorPick(sU32 *,const sChar *config,sObject *tagref);
  void ColorPickF(sF32 *,const sChar *config,sObject *tagref);
  class sColorGradientControl *Gradient(class sColorGradient *,sBool alpha);
  void Bitmask(sU8 *x,sInt width = 1);

  // complex composites

  void Scale(sVector31 &);
  void Rotate(sVector30 &);
  void Translate(sVector31 &);
  void SRT(sSRT &);

  // add your own control

  void Control(sControl *con,sInt width=-1);
  void Custom(sWindow *con,sInt width=-1,sInt height=1);
};


struct sGridFrameTemplate
{
  sInt Type;                      // sGFT_???
  sInt Flags;                     // sGFF_???
  const sChar *Label;             // Label 
  const sChar *Choices;           // RADIO,CHOICE,FLAGS: choices ; COLOR,COLORF: rgb or rgba
  sInt Offset;                    // byte-offset to value 
  sInt Count;                     // INT,FLOAT,BYTE: number of controls ; STRING: number of chars
  sF32 Min;                       // INT,FLOAT,BYTE: minimum value
  sF32 Max;                       // INT,FLOAT,BYTE: maximum value
  sF32 Step;                      // INT,FLOAT,BYTE: step for dragging
  sInt ConditionOffset;           // if((obj[offset] & mask) == value)
  sInt ConditionMask;
  sInt ConditionValue;
  sMessage Message;               // BOX,PUSHBUTTON: message to send when pressed

  void Init();
  sBool Condition(void *obj_);
  void Add(sGridFrameHelper &gh,void *obj_,const sMessage &changemsg,const sMessage &relayoutmsg);
  sGridFrameTemplate *HideCond(sInt offset,sInt mask,sInt value);
  sGridFrameTemplate *HideCondNot(sInt offset,sInt mask,sInt value);

  // special controls
  sGridFrameTemplate *InitLabel (const sChar *label);
  sGridFrameTemplate *InitGroup (const sChar *label=0);
  sGridFrameTemplate *InitPushButton(const sChar *label,const sMessage &done);
  sGridFrameTemplate *InitBox   (const sChar *label,const sMessage &done);

  // value controls
  sGridFrameTemplate *InitRadio (const sChar *label,sInt offset,const sChar *choices=L"off|on");
  sGridFrameTemplate *InitChoice(const sChar *label,sInt offset,const sChar *choices=L"off|on");
  sGridFrameTemplate *InitFlags (const sChar *label,sInt offset,const sChar *choices=L"off|on");
  sGridFrameTemplate *InitString(const sChar *label,sInt offset,sInt count);
  sGridFrameTemplate *InitFloat (const sChar *label,sInt offset,sInt count,sF32 min,sF32 max,sF32 step=0.25f);
  sGridFrameTemplate *InitInt   (const sChar *label,sInt offset,sInt count,sInt min,sInt max,sF32 step=0.25f);
  sGridFrameTemplate *InitByte  (const sChar *label,sInt offset,sInt count,sInt min,sInt max,sF32 step=0.25f);
  sGridFrameTemplate *InitColor (const sChar *label,sInt offset,const sChar *config);
  sGridFrameTemplate *InitColorF(const sChar *label,sInt offset,const sChar *config);
};

enum sGridFrameTemplateFlags
{
  sGFF_NOLABEL      = 0x00000001, // even if label is specified, do not use it
  sGFF_WIDERADIO    = 0x00000002, // use wide radiobuttons
  sGFF_RELAYOUT     = 0x00000004, // redo layout when value changes      
  
  sGFF_CONDGRAY     = 0x00000100, // gray out when condition is met
  sGFF_CONDHIDE     = 0x00000200, // hide when condition is met
  sGFF_CONDNEGATE   = 0x00000800, // negate condition
  sGFF_USEDFLAGS    = 0xffff0000,
};

enum sGridFrameTemplateTypes
{
  sGFT_NOP = 0,
  sGFT_LABEL,
  sGFT_GROUP,
  sGFT_PUSHBUTTON,
  sGFT_BOX,

  sGFT_RADIO,
  sGFT_CHOICE,
  sGFT_FLAGS,
  sGFT_STRING,
  sGFT_INT,
  sGFT_FLOAT,
  sGFT_BYTE,
  sGFT_COLOR,
  sGFT_COLORF,

  sGFT_USER,
};

/****************************************************************************/

#endif
