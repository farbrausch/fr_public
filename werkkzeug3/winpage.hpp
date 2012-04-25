// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __WINPAGE_HPP__
#define __WINPAGE_HPP__

#include "_util.hpp"
#include "_gui.hpp"
#include "werkkzeug.hpp"

/****************************************************************************/

enum
{
  CMD_PAGE_POPUP=0x0100,
  CMD_PAGE_PULLDOWN,
  CMD_PAGE_POPMISC,
  CMD_PAGE_POPTEX,
  CMD_PAGE_POPADD,
  CMD_PAGE_SHOW,
  CMD_PAGE_SHOW2,
  CMD_PAGE_DELETE,
  CMD_PAGE_CUT,
  CMD_PAGE_COPY,
  CMD_PAGE_PASTE,
  CMD_PAGE_POPSCENE,
  CMD_PAGE_POPLEVEL,
  CMD_PAGE_POPMESH,
  CMD_PAGE_POPMTRL,
  CMD_PAGE_SHOWROOT,
  CMD_PAGE_BACKSPACE,
  CMD_PAGE_BACKSPACER,
  CMD_PAGE_BIRDTOGGLE,
  CMD_PAGE_RENAMEALL,
  CMD_PAGE_RENAMEALL2,
  CMD_PAGE_GOTO,
  CMD_PAGE_GOTO2,
  CMD_PAGE_BIRDON,
  CMD_PAGE_BIRDOFF,
  CMD_PAGE_TIMEDRAGON,
  CMD_PAGE_TIMEDRAGOFF,
  CMD_PAGE_BYPASS,
  CMD_PAGE_MAKESCRATCH,
  CMD_PAGE_KEEPSCRATCH,
  CMD_PAGE_DISCARDSCRATCH,
  CMD_PAGE_TOGGLESCRATCH,
  CMD_PAGE_POPIPP,
  CMD_PAGE_HIDE,
  CMD_PAGE_EXCHANGEOP,
  CMD_PAGE_GOTOLINK,
  CMD_PAGE_FINDREFS,
  CMD_PAGE_POPMINMESH,
  CMD_PAGE_EXCHANGEOP2,
  CMD_PAGE_FINDBUGS,
  CMD_PAGE_FINDMATERIALS,
  CMD_PAGE_DUMP,
  CMD_PAGE_DUMP2,
  CMD_PAGE_HELP,
  CMD_PAGE_RENAMEONE,
  CMD_PAGE_RENAMEONE2,
};

/****************************************************************************/
/***                                                                      ***/
/***   windows                                                            ***/
/***                                                                      ***/
/****************************************************************************/

#define WINPAGE_MAXBACK 256

class WinPage : public sGuiWindow
{
  void PaintButton(sRect r,sChar *name,sU32 color,sBool pressed,sInt style,sU32 col);
  WerkOp *FindOp(sInt mx,sInt my);    // by mouse
  WerkOp *FindOpPos(sInt px,sInt py); // by coordinates
  sBool CheckDest(sBool dup);
  void MoveDest(sBool dup);
  void AddOps(sU32 cid);
  void AddOp(WerkClass *cl);
  void ScrollToCursor();
  sBool CheckDest(WerkPage *source,sInt dx,sInt dy);
  sBool CheckDest(sInt dx,sInt dy,sInt dw);
  void MakeRect(WerkOp *,sRect &r);
  void Delete();
  void Copy();
  void Paste(sInt x,sInt y);
  void Extract(WerkOp *,const sChar *name);

  sChar RenameOld[KK_NAME];
  sChar RenameNew[KK_NAME];

  struct BackEntry
  {
    sInt ScrollX;
    sInt ScrollY;
    sInt CursorX;
    sInt CursorY;
    sInt Page;
  } BackList[WINPAGE_MAXBACK+1];
  sInt BackCount;                 // stackpointer, push increments
  sInt BackRev;                   // highest item on stack for undo
  sInt BackMode;                  // disable PushBackList while PopBackList

  sInt PageX;                     // size of one operator in page
  sInt PageY;                     // size of one operator in page
  sInt DragMode;
  sInt DragKey;
  sInt StickyKey;
  sInt SelectMode;  
  sInt DragStartX;
  sInt DragStartY;
  sInt DragMouseX;
  sInt DragMouseY;
  sInt DragMoveX;
  sInt DragMoveY;
  sInt DragWidth;
  sRect DragRect;
  sInt CursorX;
  sInt CursorY;
  sInt CursorWidth;
  sInt OpWindowX;
  sInt OpWindowY;
  sU32 AddOpClass;

  sInt TimeDrag;
  sInt Birdseye;
  sRect Bird;
  sRect BirdView;

  sInt Fullsize;
  sInt Patterns[4];

public:
  WinPage();
  ~WinPage();
  void Tag();
//  sU32 GetClass() { return sCID_TOOL_PAGEWIN; }
  void OnInit();
  void OnCalcSize();
  void OnPaint();
  void OnKey(sU32 key);
  void OnDrag(sDragData &);
  sBool OnCommand(sU32 cmd);
  void SetPage(WerkPage *page);
  void GotoOp(WerkOp *);
  void PushBackList();
  void PopBackList(sInt r);
  void ExchangeOp();
  void ExchangeOp2();
  void UpdateCusorPosInStatus();

  class WerkkzeugApp *App;
  class WerkPage *Page;
  class WerkOp *EditOp;                 // latest operator that was picked for editing
};

/****************************************************************************/
/****************************************************************************/

#endif

