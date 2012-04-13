// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef __WINPAGE_HPP__
#define __WINPAGE_HPP__

#include "_util.hpp"
#include "_gui.hpp"
#include "apptool.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   doc                                                                ***/
/***                                                                      ***/
/****************************************************************************/

#define MAX_PAGEOPPARA    64
#define MAX_PAGEOPINPUTS  16
#define MAX_ANIMSTRING    128

struct PageOpClass
{
  sChar *Name;
  sInt FuncId;
  sU32 Flags;
  sU32 Color;
  sInt InputCount;
  sU32 InputCID[MAX_PAGEOPINPUTS];
  sU32 OutputCID;
  sControlTemplate Para[MAX_PAGEOPPARA];
  sInt ParaCount;
  sU32 Shortcut;
  sU32 InputFlags;

  void Init(sChar *name,sInt funcid);
  sInt AddPara(sChar *name,sInt type);
};

#define POCF_LOAD     0x0001
#define POCF_STORE    0x0002
#define POCF_LABEL    0x0004
#define POCF_SL       (POCF_STORE|POCF_LABEL)

struct PageOpPara
{
  sInt Animated;
  sInt Data[4];
  sChar *Anim;
};

class PageOp : public ToolObject
{
public:
  PageOp();
  ~PageOp();
  sU32 GetClass() { return sCID_TOOL_PAGEOP; }
  void Copy(sObject *o);
  void Tag();
  void MakeRect(sRect &out,sRect &in);
  void Init(PageOpClass *cl,sInt x,sInt y,sInt w,class PageDoc *doc);

  PageOpClass *Class;                       // operator class
  sInt PosX;                                // page position
  sInt PosY;
  sInt Width;
  sBool Selected;                           // selected
  sBool Showed;                             // one of the viewports is showing this
  sBool Error;                              // error on connecting, variable, ...
  sBool Edited;                             // one of the parawin is editing this

  sInt InputCount;                          // number of inputs used
  PageOp *Inputs[MAX_PAGEOPINPUTS];         // input ptrs. please keep unused ptrs 0
  PageOpPara Data[MAX_PAGEOPPARA];          // parameters
};

class PageDoc : public ToolDoc
{
public:
  PageDoc();
  ~PageDoc();
  void Clear();
  void Tag();
  sU32 GetClass() {return sCID_TOOL_PAGEDOC;}

  sBool Write(sU32 *&);
  sBool Read(sU32 *&);
  sBool CheckDest(PageDoc *source,sInt dx,sInt dy);
  sBool CheckDest(sInt x,sInt y,sInt w);
  void Delete();
  void Copy();
  void Paste(sInt x,sInt y);

  sList<PageOp> *Ops;

  void UpdatePage();
};

/****************************************************************************/
/***                                                                      ***/
/***   windows                                                            ***/
/***                                                                      ***/
/****************************************************************************/

class PageWin : public ToolWindow
{
  void PaintButton(sRect r,sChar *name,sU32 color,sBool pressed,sInt style);
  PageOp *FindOp(sInt mx,sInt my);
  sBool CheckDest(sBool dup);
  void MoveDest(sBool dup);
  void AddOps(sU32 cid);
  void AddOp(PageOpClass *cl);
  void ScrollToCursor();

  sInt DragMode;
  sInt DragKey;
  sInt StickyKey;
  sInt SelectMode;  
  sInt DragStartX;
  sInt DragStartY;
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
public:
  PageWin();
  ~PageWin();
  void Tag();
  sU32 GetClass() { return sCID_TOOL_PAGEWIN; }
  void OnInit();
  void OnCalcSize();
  void OnPaint();
  void OnKey(sU32 key);
  void OnDrag(sDragData &);
  sBool OnCommand(sU32 cmd);
  void SetDoc(ToolDoc *doc);

  PageOp *EditOp;                 // latest operator that was picked for editing
  PageDoc *Doc;
};

/****************************************************************************/
/****************************************************************************/

#endif
