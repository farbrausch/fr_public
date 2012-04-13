// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef __APP_TEXT_HPP__
#define __APP_TEXT_HPP__

#include "_util.hpp"
#include "_gui.hpp"
#include "appbrowser.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   windows                                                            ***/
/***                                                                      ***/
/****************************************************************************/

#define sTCC_COPY     0x0101
#define sTCC_PASTE    0x0102
#define sTCC_CUT      0x0103
#define sTCC_BLOCK    0x0104

#define sTCC_CLEAR    0x0105
#define sTCC_OPEN     0x0106
#define sTCC_SAVE     0x0107
#define sTCC_SAVEAS   0x0108

#define sTCC_EXIT     0x0109

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

  sFileWindow *File;
public:
  sTextControl();
  ~sTextControl();
  void OnCalcSize();
  void OnPaint();
  void OnKey(sU32 key);
  void OnDrag(sDragData &);
  sBool OnCommand(sU32 cmd);

  void SetText(sChar *);
  sChar *GetText();
  void Engine(sInt pos,sInt len,sChar *insert);
  void DelSel();
  sInt GetCursorX();
  sInt GetCursorY();
  void SetCursor(sInt x,sInt y);
  void SetSelection(sInt cursor,sInt sel);
  sBool LoadFile(sChar *name);
  void Log(sChar *);              // append string to editbuffer, usefull for log-windows          
  void ClearText();               // clears all text


  sInt CursorWish;                // imagine you scroll down into an empty line and scroll one more down into a full one. in which column should the cursor jump?
  sInt DoneCmd;
  sInt CursorCmd;
  sInt ReallocCmd;
  sChar *Text;
  sInt TextAlloc;
  sChar PathName[sDI_PATHSIZE];
  sInt Changed;
  sInt Static;                    // dont allow changing, usefull for log-windows
};

class sTextApp : public sDummyFrame
{
  sTextControl *Edit;
  sStatusBorder *Status;
  sInt Changed;

  sChar StatusName[256];
  sChar StatusLine[32];
  sChar StatusColumn[32];
  sChar StatusChanged[32];
public:
  sTextApp();
  sBool OnCommand(sU32 cmd);
  void OnKey(sU32 key);
  void UpdateStatus();
};

/****************************************************************************/

#endif
