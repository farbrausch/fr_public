/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#pragma once

#include "gui/window.hpp"
#include "gui/manager.hpp"
#include "gui/wire.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Text Window                                                        ***/
/***                                                                      ***/
/****************************************************************************/

class sTextWindow : public sWindow
{
  sPrintInfo PrintInfo;
  sInt MarkMode;
  sInt MarkBegin;
  sInt MarkEnd;
  sInt WindowHeight;
  sInt DragCursorStart;
  sInt DisableDrag;
  sInt GoodCursorX;
  class sFont2D *Font;
  sRect TextRect;

  sTextBuffer *Text;              // textbuffer to edit

  void GetCursorPos(sInt &x,sInt &y);
  sInt FindCursorPos(sInt x,sInt y);
  sBool BeginMoveCursor(sBool selmode);
  void EndMoveCursor(sBool selmode);
  void Delete(sInt pos,sInt len);
  void Insert(sInt pos,const sChar *s,sInt len=-1);
  void Mark(sInt start,sInt end);
  void MarkOff();

  sInt CursorTimer;
  void CmdCursorToggle();

  sMessageTimer *Timer;

  struct UndoStep
  {
    sBool Delete;
    sInt Pos;
    sInt Count;
    sChar *Text;
  };
  UndoStep *UndoBuffer;
  sInt UndoAlloc;
  sInt UndoValid;
  sInt UndoIndex;
  sString<256> UndoCollector;
  sInt UndoCollectorIndex;
  sInt UndoCollectorValid;
  sInt UndoCollectorStart;

  UndoStep *UndoGetStep();
  void UndoFlushCollector();
  void UndoInsert(sInt pos,const sChar *string,sInt len);
  void UndoDelete(sInt pos,sInt size);
  void Undo();
  void Redo();

public:
  sCLASSNAME(sTextWindow);
  sTextWindow();
  ~sTextWindow();
  void InitCursorFlash();
  void Tag();
  void SetText(sTextBuffer *);
  sTextBuffer *GetText() { return Text; }
  sBool HasSelection() { return MarkMode!=0; }

  void OnPaint2D();
  sBool OnKey(sU32 key);
  void OnDrag(const sWindowDrag &dd);
  void OnCalcSize();

  void ResetCursorFlash();
  sBool GetCursorFlash();

  sObject *TextTag;               // this tag will be hold should be object in which the textbuffer is embedded

  sInt TextFlags;                 // sF2P_???
  sInt EditFlags;                 // sTEF_???
  sInt EnterCmd;                  // post this when enter is pressed
  sInt BackColor;                 // sGC_???
  sInt HintLine;                  // fine vertical line that hints the right border of the page
  sU32 HintLineColor;             // sGC_???
  sInt TabSize;                   // default 8. currently, TAB insert spaces
  sMessage EnterMsg;
  sMessage ChangeMsg;
  sMessage CursorMsg;             // when cursor changes
  sMessage CursorFlashMsg;        // synchronise to cursor :-)
  sBool ShowCursorAlways;         // usually the cursor is hidden when focus is lost

  void DeleteChar();
  void DeleteBlock();
  void InsertChar(sInt c);
  void InsertString(const sChar *s);
  void IndentBlock(sInt indent);
  void OverwriteChar(sInt c);
  void TextChanged();             // react to the fact that the text changed.
  void SetCursor(sInt p);
  void SetCursor(const sChar *p);
  void SetFont(sFont2D *newFont);
  void ScrollToCursor();
  sFont2D *GetFont();
  sInt GetCursorPos() { return PrintInfo.CursorPos; }
  sInt GetCursorColumn() const;
  void Find(const sChar *string,sBool dir,sBool next);
  void GetMark(sInt &start,sInt &end);

  void UndoClear();
  void UndoGetStats(sInt &undos,sInt &redos);

};

enum sTextEditFlags
{
//  sTEF_EDIT       = 0x0001,
  sTEF_MARK       = 0x0002,
  sTEF_STATIC     = 0x0004,
  sTEF_LINENUMBER = 0x0008,
};

class sWireTextWindow : public sTextWindow
{
public:
  void InitWire(const sChar *name) { sWire->AddWindow(name,this); }
  sBool OnKey(sU32 key) { if(sWire->HandleKey(this,key)) return 1; else return sTextWindow::OnKey(key); }
  sBool OnShortcut(sU32 key) { return sWire->HandleShortcut(this,key); }
};

/****************************************************************************/
