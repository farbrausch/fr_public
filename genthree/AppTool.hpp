// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef __APP_TOOL_HPP__
#define __APP_TOOL_HPP__

#include "_util.hpp"
#include "_gui.hpp"
#include "genplayer.hpp"
#include "gencode.hpp"

/****************************************************************************/

class ToolDoc;
class ToolObject;
class ToolWindow;
class sToolApp;

/****************************************************************************/
/***                                                                      ***/
/***   CIDS                                                               ***/
/***                                                                      ***/
/****************************************************************************/

#define sCID_TOOL_PARAWIN         0x01000040
#define sCID_TOOL_PAGEWIN         0x01000041
#define sCID_TOOL_PROJECTWIN      0x01000042
#define sCID_TOOL_VIEWWIN         0x01000043
#define sCID_TOOL_OBJECTWIN       0x01000044
#define sCID_TOOL_PERFMONWIN      0x01000045
#define sCID_TOOL_EDITWIN         0x01000046
#define sCID_TOOL_TIMEWIN         0x01000048
#define sCID_TOOL_ANIMWIN         0x01000049

#define sCID_TOOL_DOC             0x01000050
#define sCID_TOOL_PAGEDOC         0x01000051
#define sCID_TOOL_EDITDOC         0x01000052
#define sCID_TOOL_TIMEDOC         0x01000054
#define sCID_TOOL_ANIMDOC         0x01000055

#define sCID_TOOL_PAGEOP          0x01000060
#define sCID_TOOL_TIMEOP          0x01000061

/****************************************************************************/
/***                                                                      ***/
/***   docs                                                               ***/
/***                                                                      ***/
/****************************************************************************/


class ToolObject : public sObject
{
public:
  ToolObject();                   // clear all fields
  void Tag();                     // needs doc
//  void Init(sU32 cid,sChar *name,ToolDoc *doc);

  void Calc();                    // start calc recursion

                                  // please set these!
  sChar Name[32];                 // name
  ToolDoc *Doc;                   // document that created the object
  sU32 CID;                       // cid for this object
  sInt Temp;

  sBool NeedsCache;               // someone requested this ToolObject to have a cached object
  sU32 ChangeCount;               // object was last changed at this GetChange() call
  sU32 CalcCount;                 // object was up to date at this CheckChange() call
  sObject *Cache;                 // cached object

  sInt GlobalLoad;                // during code generation, global from where to load
  sInt GlobalStore;               // during code generation, global where to store finally

  ToolCodeSnip *Snip;             // link in tree for codesnipplets
};

class ToolDoc : public sObject
{
public:
  ToolDoc();                      // clear fields
  sToolApp *App;                  // please set this!

//  virtual void Update(ToolObject *)=0;     // update object in this doc
  sChar Name[32];                 // name of document
};

/****************************************************************************/
/***                                                                      ***/
/***   objects                                                            ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/
/***                                                                      ***/
/***   windows                                                            ***/
/***                                                                      ***/
/****************************************************************************/

class ToolWindow : public sGuiWindow
{
public:
  ToolWindow();
  virtual void OnInit() {}
  virtual void SetDoc(ToolDoc *) {}

  sToolApp *App;
  sInt WinId;
  sBool CanUseDoc(ToolDoc *);

  sToolBorder *ToolBorder;
  sChar *ShortName;
};

/****************************************************************************/

struct WindowSetEntry
{
  sInt Id;                      // -1 = hsplit, -2 = vsplit
  sInt Pos;                     // relative position of split
  sInt Count;                   // number of childs
  sGuiWindow *Window;           // if this is the active set, yet another ptr to that window
  void Init(sInt i,sInt p,sInt c) { Id=i; Pos=p; Count=c; }
};

/****************************************************************************/

#define sDW_MAXSET      4
#define sDW_MAXPERSET   32
#define sDW_MAXWINDOW   32


#define sDW_MAXCOPY 64

struct DockerEntry
{
  sGuiWindow *Window;
  sInt ChildIndex;
  sRect Rect;
};

class DockerWindow : public sGuiWindow
{
  sInt PressedRect;
  sInt LastRect;

  void PaintR(sGuiWindow *win,sInt ci);
public:
  DockerWindow();
  void OnPaint();
  void OnDrag(sDragData &dd);
  void OnKey(sU32 key);
  sBool OnCommand(sU32 cmd);

  sToolApp *App;
  sGuiWindow *Root;
  sGuiWindow **Choices;
  sInt ChoiceCount;
  DockerEntry Windows[sDW_MAXCOPY];
  sInt WinCount;
};

/****************************************************************************/
/***                                                                      ***/
/***   application                                                        ***/
/***                                                                      ***/
/****************************************************************************/

class sToolApp : public sDummyFrame
{
  sStatusBorder *Status;
  GenTimeBorder *TimeBorder;
  sChar Stat[5][256];
  class sFileWindow *FileWindow;
  DockerWindow *Docker;

  sInt LogMode;
  sU32 ChangeCount;
  sInt ChangeMode;
  sInt DocChangeCount;
  sInt TextureSize;
  sInt ScreenRes;
  sInt TurboMode;
private:

  void MakeClassList();
  void DeleteClassList();

public:
  sToolApp();
  ~sToolApp();
  void Tag();
  sBool OnDebugPrint(sChar *text);
//  void OnKey(sU32 key);
  sBool OnShortcut(sU32 key);
  sBool OnCommand(sU32);
  void OnDrag(sDragData &);
  void OnFrame();
  void OnPaint() {}
  sBool Write(sU32 *&);
  sBool Read(sU32 *&);

  ToolWindow *FindActiveWindow(sU32 cid);
  void SetActiveWindow(ToolWindow *win);
  void SetWindowSet(ToolWindow *win);
  void SetWindowSet(sInt nr);
  void SetWindowSetR(WindowSetEntry *&e,sGuiWindow *p);
  void RemWindowSetR(WindowSetEntry *&e,sGuiWindow *p,sGuiWindow *win);
  sU32 GetChange();
  sU32 CheckChange();
  void SetStat(sInt num,sChar *text);
  struct PageOpClass *FindPageOpClass(sInt i);
  sBool DemoPrev(sBool doexport=0);

  WindowSetEntry WinSetData[sDW_MAXSET][2][sDW_MAXPERSET];
  ToolWindow *WinSetStorage[sDW_MAXWINDOW];
  sInt WindowSet;

  sGuiWindow *SecondWindow;
  sList<ToolWindow> *Windows;
  sList<ToolDoc> *Docs;
  sList<ToolObject> *Objects;
  sList<ToolObject> *CurrentLoadStore;
  GenPlayer *Player;
  GenPlayer *PlayerCalc;
  ToolCodeGen CodeGen;

  sBool Export(sChar *name,sInt binary);

  ToolObject *FindObject(sChar *name);
  sU32 ClassColor(sU32 cid);
  void UpdateObjectList();
  ToolDoc *FindDoc(sChar *name);
  void UpdateDocList();
  void UpdateAllWindows();

  struct PageOpClass *ClassList;
  sInt ClassCount;
};

/****************************************************************************/
/***                                                                      ***/
/***   application                                                        ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/

#endif
