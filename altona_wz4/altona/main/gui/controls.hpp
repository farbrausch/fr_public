/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_GUI_CONTROLS_HPP
#define FILE_GUI_CONTROLS_HPP

#ifndef __GNUC__
#pragma once
#endif

#include "gui/window.hpp"
#include "gui/manager.hpp"

/****************************************************************************/

class sControl : public sWindow
{
public:
  sControl();
  ~sControl();
  void Tag();

  sInt Style;
  sU32 BackColor;
  sMessage DoneMsg;
  sMessage ChangeMsg;

  void SendChange();
  void SendDone();
  void PostChange();
  void PostDone();
};

/****************************************************************************/

class sButtonControl : public sControl
{
  sInt Pressed; 
  sInt RadioValue;                // set these two to implement radio button highlighting
  sInt *RadioPtr;                 // local, so you can't foret to update notify.
public:
  sCLASSNAME(sButtonControl);
  sButtonControl(const sChar *,const sMessage &msg,sInt style=0);
  sButtonControl();
  void InitRadio(sInt *ptr,sInt value);
  void InitToggle(sInt *ptr);
  void InitCheckmark(sInt *ptr,sInt val);
  void InitReadOnly(sInt *ptr,sInt val=1);
  const sChar *Label;
  sInt LabelLength;               // length of label string, set to -1
  sInt Width;                     // usually, the width is calculated automatic. set this != 0 to override!
  sU32 Shortcut;                  // shortcutkey, will be displayed inside button. this is just for display in sBCS_NOBORDER mode
  sMessage DoubleClickMsg;

  sInt GetRadioValue() { return RadioValue; }

  void OnCalcSize();
  void OnPaint2D();
  void OnDrag(const sWindowDrag &dd);
  sBool OnKey(sU32 key);
  sBool OnCommand(sInt cmd);

  static void MakeShortcut(sString<64> &buffer,sU32 shortcut);    // this is awfully useful and belongs to gui.hpp or types.hpp
};

enum sButtonControlStyle
{
  sBCS_NOBORDER   = 0x0001,       // don't draw any border
  sBCS_STATIC     = 0x0002,       // disable interaction
  sBCS_IMMEDIATE  = 0x0004,       // react on ButtonDown, not ButtonUp
  sBCS_LABEL      = 0x0008,       // no interaction
  sBCS_RADIO      = 0x0010,       // when pressed, actually set the *radioptr=radioval
  sBCS_TOGGLE     = 0x0020,       // when pressed, toggle *radioptr. radioval should be 1
  sBCS_CHECKMARK  = 0x0040,       // add a checkmark when *radioptr==radioval
  sBCS_READONLY   = 0x0080,       // just display *radioptr==radioval, don't change value when clicked
  sBCS_TOGGLEBIT  = 0x0100,       // when pressed, eor *radioptr with radioval, toggling a bit
  sBCS_SHOWSELECT = 0x0200,       // just display in pressed-down state. 
};

/****************************************************************************/

class sFileDialogControl : public sButtonControl
{
  void CmdStart();
  sStringDesc String;
  const sChar *Text;
  const sChar *Extensions;
  sInt Flags;
public:
  sFileDialogControl(const sStringDesc &string,const sChar *text,const sChar *ext,sInt flags=0);
  ~sFileDialogControl();

  sMessage OkMsg;
};

//  sSOF_LOAD = 1,
//  sSOF_SAVE = 2,
//  sSOF_DIR = 3,

/****************************************************************************/

class sChoiceControl : public sControl
{
//  sInt *Value;                    // ptr to value
public:
  struct ChoiceInfo
  {
    const sChar *Label;           // display string
    sInt Length;                  // string length
    sInt Value;                   // value to set, not shifted by ValueShift.
  };
  struct ChoiceMulti
  {
    sInt *Ptr;
    sInt Default;
  };
  enum ChoicesStyles
  {
    sCBS_CYCLE     = 0,           // cycle between choices
    sCBS_DROPDOWN  = 1,           // open a dropdown list 
  };
  sInt ValueMask;                 // mask of bits that may be affected
  sInt ValueShift;                // shift by this amount
  sInt Width;                     // usually, the width is calculated automatic. set this != 0 to override!
  sArray<ChoiceInfo> Choices;     // created by InitChoices();
  sInt Pressed;                   // visual feedback

  sCLASSNAME(sChoiceControl);
  sChoiceControl(const sChar *choices,sInt *val);
  sChoiceControl();
  void InitChoices(const sChar *&,sInt *val);  // parse the choice string. "*2x|y|3z" -> 3 values: 0:"x", 4:"y", 12:"z". s will be advanced, so that it points to after the string. for flags op.
  void AddMultiChoice(sInt *val); // same choice for multiple data: set *val for initchoices to 0
  template <class Type> void AddMultiChoice(sArray<Type *> &list,sInt Type::*offset) { Type *t; sFORALL(list,t) AddMultiChoice(&(t->*offset)); }


  void OnCalcSize();
  void OnPaint2D();
  void OnDrag(const sWindowDrag &dd);
  sBool OnKey(sU32 key);
  sBool OnCommand(sInt cmd);
  void FakeDropdown(sInt x,sInt y);
private:
  void Next();
  void Dropdown();
  void SetValue(sDInt newval);
  void SetDefaultValue();
  sArray<ChoiceMulti> Values;
};

/****************************************************************************/

enum sStringControlStyle
{
  sSCS_BACKCOLOR = 0x0001,
  sSCS_STATIC = 0x0002,
  sSCS_BACKBAR = 0x0004,
  sSCS_FIXEDWIDTH = 8,
};

// InitString needs some cleanup
// need multiline for Textbuffers

class sStringControl : public sControl
{
  sPrintInfo PrintInfo;
  sInt MarkMode;
  sInt MarkBegin;
  sInt MarkEnd;
  void DeleteMark();
  void InsertChar(sInt c);
  void ResetCursor();
  sChar *Buffer;
  sInt Size;
  sPoolString *PoolPtr;
  sTextBuffer *TextBuffer;

  const sChar *GetText();
protected:
  void SetCharPos(sInt x);
  sBool Changed;
public:
  sCLASSNAME(sStringControl);
  sStringControl(const sStringDesc &);
  sStringControl(sInt size,sChar *buffer);
  sStringControl(sPoolString *pool);
  sStringControl(sTextBuffer *tb);
  sStringControl();
  ~sStringControl();
  void Tag();
  void InitString(const sStringDesc &desc);
  void InitString(sPoolString *pool);
  void InitString(sTextBuffer *tb);

  virtual sBool MakeBuffer(sBool unconditional);   // if buffer needs to be updated, do that and return 1
  virtual sBool ParseBuffer();  // if buffer is valid, update value and return 1

  sU32 BackColor;
  sF32 Percent;

  void OnCalcSize();
  void OnPaint2D();
  sBool OnKey(sU32 key);
  void OnDrag(const sWindowDrag &dd);
  sBool OnCommand(sInt cmd);

  sStringDesc GetBuffer();
  sStringControl *DragTogether;      // only used for ValueControl
};

/****************************************************************************/

template <typename Type> 
class sValueControl : public sStringControl
{
  sString<64> String;
  Type *ColorPtr;
public:
  Type *Value;
  Type Min;
  Type Max;
  Type OldValue;
  Type Default;
  sF32 Step;
  sF32 RightStep;
  Type DragStart;
  const sChar *Format;
  sInt DisplayShift;      // for int only: shift before display
  sInt DisplayMask;       // for int only: mask before display

  // CLASSNAME..
  static const sChar *ClassName();
  void MoreInit();
  const sChar *GetClassName() { return ClassName(); }
  void InitValue(Type *val,Type min,Type max,sF32 step=0.0f,Type *colptr=0)
  {
    Min = min;  Max = max;  Value = val; Step = step;
    if(min<=0 && max>=0)
      Default = 0;
    else
      Default = min;
    OldValue = *Value;
    ColorPtr = colptr;
    ClearNotify();
    DisplayShift = 0;
    DisplayMask = ~0;
    DragTogether = 0;
    RightStep = 0.125f;
    AddNotify(Value,sizeof(Type));
    AddNotify(ColorPtr,sizeof(Type)*3);
    MoreInit();
  }
  sValueControl(Type *val,Type min,Type max,sF32 step=0.0f,Type *colptr=0)
  {
    InitString(String); InitValue(val,min,max,step,colptr);
    Format=L"%f"; MakeBuffer(1);
  }
  sValueControl()
  {
    InitString(String); InitValue(0,0,0,0,0);
    Format=L"%f"; MakeBuffer(1);
  }
  void OnCalcSize()
  {
    ReqSizeY = sGui->PropFont->GetHeight()+4;
  }
  sBool OnKey(sU32 key)
  {
    if(Style & sSCS_STATIC) return 0;
    sStringControl *tie;
    switch(key&(sKEYQ_BREAK|sKEYQ_MASK))
    {
    case sKEY_HOME:
      *Value = Default; if(MakeBuffer(0)) Update();
      tie = DragTogether;
      if((key&sKEYQ_CTRL) && tie)
      {
        while(tie!=this)
        { tie->OnKey(sKEY_HOME); tie = tie->DragTogether; }
      }
      PostChange();
      sGui->Notify(Value,sizeof(Type));
      PostDone();
      return 1;
    default:
      return sStringControl::OnKey(key);
    }
  }

  void OnPaint2D();
  void OnDrag(const sWindowDrag &dd)
  {
    if(Style & sSCS_STATIC) return;
    if(Step!=0)
    {
      sF32 step = (dd.Buttons&2)?RightStep:Step; 
      switch(dd.Mode)
      {
      case sDD_START:   
        SetCharPos(dd.StartX-Inner.x0);
        if(dd.Flags&sDDF_DOUBLECLICK)
          OnKey(sKEY_HOME|(sGetKeyQualifier()&sKEYQ_CTRL));
        DragStart = *Value;
        break;
      case sDD_DRAG:
        if(dd.DeltaX)
          SetCharPos(Inner.SizeX());
        else
          SetCharPos(dd.StartX-Inner.x0);
        if(!(dd.Flags&sDDF_DOUBLECLICK))
        { sF64 val = DragStart;
          if(step>=0)
            val = val + dd.HardDeltaX/2*step;
          else
            val = (val==0) ? 0.0001f : sExp(sLog(sF32(val))-dd.HardDeltaX/2*step);
          *Value = Type(sClamp<sF64>(val,Min,Max));
          if(MakeBuffer(0)) { Update(); PostChange(); Changed=1; sGui->Notify(Value,sizeof(Type)); }
        } 
        if((sGetKeyQualifier()&sKEYQ_CTRL))
          OnKey('a'|sKEYQ_CTRL);
        break;
      case sDD_STOP: 
        PostChange();
        PostDone(); 
        break;
      }
    }
    if(!(dd.Buttons&0x8000))
    {
      sStringControl *tie = DragTogether;
      sWindowDrag dd2 = dd;
      dd2.Buttons |= 0x8000;
      if((sGetKeyQualifier()&sKEYQ_CTRL) && tie)
      {
        while(tie!=this)
        { tie->OnDrag(dd2); tie = tie->DragTogether; }
      }
    }
  }

  sBool MakeBuffer(sBool unconditional);
  sBool ParseBuffer();
};

typedef sValueControl<sU8> sByteControl;
typedef sValueControl<sU16> sWordControl;
typedef sValueControl<sInt> sIntControl;
typedef sValueControl<sF32> sFloatControl;

/****************************************************************************/

class sFlagControl : public sControl
{
  sBool Toggle;
  sInt GetValue();
  sInt CountChoices();
  void SetValue(sInt n);
  void NextChoice(sInt delta);
  void MakeChoice(sInt n,const sStringDesc &desc);

public:
  sCLASSNAME(sFlagControl);
  sFlagControl();
  sFlagControl(sInt *val,const sChar *choices);
  sFlagControl(sInt *val,const sChar *choices,sInt mask,sInt shift);

  sInt *Val;
  sInt Max;
  sInt Mask;
  sInt Shift;
  const sChar *Choices;

  void OnCalcSize();
  void OnPaint2D();
  void OnDrag(const sWindowDrag &dd);
};

/****************************************************************************/

class sProgressBarControl : public sControl
{
  sF32 Percentage;

public:
  sCLASSNAME(sProgressBarControl);
  sProgressBarControl();

  void SetPercentage(sF32 percentage);

  void OnCalcSize();
  void OnPaint2D();
};

/****************************************************************************/

class sBitmaskControl : public sControl
{
  sInt X[9];
  sU8 *Val;
public:
  sCLASSNAME(sBitmaskControl);
  sBitmaskControl();
  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();
  void OnDrag(const sWindowDrag &dd);

  void Init(sU8 *val);
};

/****************************************************************************/

#endif
