// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __WERKKZEUG_HPP__
#define __WERKKZEUG_HPP__

#include "_util.hpp"
#include "_gui.hpp"
#include "appbrowser.hpp"
#include "kdoc.hpp"
#include "novaplayer.hpp"
#include "script.hpp"

#define APPNAME "werkkzeug"
#define VERSION "3.365"

#if sLINK_GUI

#define sCID_WERKKZEUG      0x01000100
#define sCID_WERKANIM       0x01000101
#define sCID_WERKOP         0x01000102
#define sCID_WERKPAGE       0x01000103
#define sCID_WERKEVENT      0x01000104
#define sCID_WERKDOC        0x01000105
#define sCID_WERKSPLINE     0x01000106
#define sCID_WERKOPANIM     0x01000107

#define sCID_WERKDOC2       0x01000110
#define sCID_WERKEVENT2     0x01000111
#define sCID_WERKSCENE2     0x01000112
#define sCID_WERKSCENENODE2 0x01000113

class WerkDoc;

/****************************************************************************/
/***                                                                      ***/
/***   Document                                                           ***/
/***                                                                      ***/
/****************************************************************************/

typedef void (* WerkClassEdit)(class WerkkzeugApp *,class WerkOp *);
typedef void (* WerkClassCalc)(struct KOp *);

struct WerkClass
{
  sChar *Name;                              // name of operator
  sInt Id;                                  // id for load/save (please keep if 8 bit)
  sInt Output;                              // KC_?? of object generated
  sInt MinInput,MaxInput;                   // input count for 
  sU32 Convention;                          // calling convention: 0xm00slipp (modify,0,strings,links,inputs,para)
  sInt Column;                              // column in editor
  sU32 Shortcut;                            // shortcut in palette
  sChar *Packing;                           // "iiffbc" type information for packing
  WerkClassEdit EditHandler;                // set up for editing this operator
  void *InitHandler;                        // initialize Object
  void *ExecHandler;                        // execute Object
  sInt Inputs[KK_MAXINPUT];                 // KC_?? of inputs
  sInt Links[KK_MAXLINK];                   // KC_?? of links
};

extern WerkClass WerkClasses[];
extern sU32 WerkColors[16];
WerkClass *FindWerkClass(sInt id);

// packing string (large letters are animateble)
// f = float
// e = small range float, may be stored as 1:3:12
// i = full range integer
// s = 16 bit signed
// b = 8 bit unsigned (or 8 bit mask)
// c = 32 bit RGBA color
// m = 24 bit mask
// - = gap. 

/****************************************************************************/

#define WS_MAXCHANNEL 4

#define WSK_ANY       0                     // Kind, used for finetuning editor behavour
#define WSK_SCALE     1                   
#define WSK_ROTATE    2
#define WSK_TRANSLATE 3
#define WSK_COLOR     4

class WerkSpline : public sObject
{
public:
  struct WerkSplineChannel                  // holds a spline channel
  {
    sArray<KSplineKey> Keys;                // SplineKeys
    sInt Selected;                          // selected for editing
  };
  KSpline Spline;                           // Spline Info
  WerkSplineChannel Channel[WS_MAXCHANNEL]; // the chnnels of the spline. valid if Channel[x].Spline is initialized
  sChar Name[KK_NAME];                      // name of the spline
  sInt Kind;                                // WSK_??
  sInt Index;                               // global index for spline

  WerkSpline();
  ~WerkSpline();
  sU32 GetClass() { return sCID_WERKSPLINE; }
  void Tag();
  void Copy(sObject *);
  void Init(sInt count,sChar *name,sInt kind,sBool addkeys,sInt inter);
  void InitDef(sInt count,sChar *name,sInt kind,const sF32 *def,sInt inter);
  void Clear();

  KSplineKey *AddKey(sInt channel,sF32 time,sF32 value);
  void Select(sInt value);
  void RemSelection();
  void MoveSelection(sF32 time,sF32 value);
  sBool Sort();
};


#define WOA_MAXINPUT 4                      // WerkOpAnims don't have more than 2 or 3 inputs
struct WerkOpAnimInfo
{
  const sChar *Name;
  sInt Code;
  sInt Inputs;
  sU32 Flags;
  sInt UnionKind;
};

#define AOIF_ADDOP        1
#define AOIF_LOADPARA     2
#define AOIF_STOREVAR     4
#define AOIF_STOREPARA    8
#define AOIF_LOADVAR      16

#define AOU_OFF           0                 // no additional data
#define AOU_VECTOR        1                 // ConstVector
#define AOU_COLOR         2                 // ConstColor
#define AOU_SCALAR        3                 // ConstVector.x
#define AOU_NAME          4                 // SplineName
#define AOU_MASK          5                 // writemask, stored in op
#define AOU_EASE          6                 // parameters for ease function 2x(0..1)

extern const WerkOpAnimInfo AnimOpCmds[];

class WerkOpAnim : public sObject           // animations have operators, too!
{
public:
  sInt PosX;                                // position for editing
  sInt PosY;
  sInt Width;
  sInt Bytecode;                            // KA_??? bytecode of that op
  sInt Parameter;                           // bytecode-parameter (variable index,...)
  sInt AutoOp;                              // 0=mul, 1=add, for operators with usually zero inputs
  sInt Select;                              // selected in gui
  sInt Error;                               // wrong connection
  sInt MaxMask;                             // maximum allowed mask (store only)
  sInt UnionKind;         
  sInt UsedInCode;                          // used by codegenerator
  union
  {
    sVector ConstVector;
    sU32 ConstColor;
    sChar SplineName[KK_NAME];
  };

  sInt OutputCount;                         // there is no list of outputs, but we want to know the count
  sInt InputCount;                          // list of inputs
  WerkOpAnim *Inputs[WOA_MAXINPUT];

  const WerkOpAnimInfo *Info;               // link to info

  WerkOpAnim();
  ~WerkOpAnim();
  void Tag();
  void Init(sInt code,sInt para);           // init all except PosX, PosY, Width
  sU32 GetClass() { return sCID_WERKOPANIM; }
  void Copy(sObject *);
};
/*
class WerkAnim : public sObject             // obsolete
{
public:
  WerkAnim();
  ~WerkAnim();
  sU32 GetClass() { return sCID_WERKANIM; }
  void Tag();
  void Rename(sChar *o,sChar *n);

  KAnim Anim;                               // spline as in player
  class WerkOp *Target;                     // target operator
  sChar Name[KK_NAME];                      // name for editor
  sInt Select[4];                           // select each dimensin in gui
};
*/
class WerkOp : public sObject
{
public:
  WerkOp();
  WerkOp(WerkkzeugApp *app,sInt id,sInt x,sInt y,sInt w);
  ~WerkOp();
  sU32 GetClass() { return sCID_WERKOP; }
  void Tag();
  void Copy(sObject *ob);

  KOp Op;                                   // operator as in player
  sInt PosX;                                // position on page
  sInt PosY;
  sInt Width;
  sChar Name[KK_NAME];                      // name for linking
  sChar LinkName[KK_MAXLINK][KK_NAME];      // name of links
  sChar SplineName[KK_MAXSPLINE][KK_NAME];  // name of splines
//  sList<WerkAnim> *Anims;                   // obsolete: animation splines
  sList<WerkOpAnim> *AnimOps;               // animation operators

//  WerkOp *Inputs[KK_MAXINPUT];              // duplicated from KOp, use KOp.InputCount
//  WerkOp *Links[KK_MAXLINK];                // duplicated from KOp!!
//  WerkOp *Outputs[KK_MAXOUTPUT];            // duplicated from KOp, use KOp.OutputCount
  class WerkPage *Page;                     // backlink to page
  sInt Error;                               // something wrong
  sInt Selected;                            // selected in page
  WerkClass *Class;                         // class information
  sInt SetDefaults;                         // special mode for EditHandler that does not generate gui
  sInt SaveId;
  sInt WheelPhase;                          // phase for spinning wheel animation
  sInt Bypass;                              // true = skip op during connection
  sInt Hide;                                // true = ignore op during connection
  sInt BlinkTime;                           // blink this op until the time is reached. 0=off
  sInt CycleFind;                           // cycle-breaker count for DumpOpByClassR

  void Init(WerkkzeugApp *app,sInt id,sInt x,sInt y,sInt w,sBool inConstruct=sFALSE);  // get something fast
  void AddOutput(WerkOp *oo);
  void Rename(sChar *o,sChar *n);           // rename every occurence of a KK_NAME string
  void Change(sBool reconnect,sInt offset=-1);      // this operator has changed in some way. updated KOp and WerkPage
  sBool ConnectAnim(WerkDoc *doc);
  sBool GenCodeAnim(WerkOpAnim *oa,WerkDoc *doc,sU8 *stored);
  sInt GetResultClass();                    // finds the class of result without ever evaluating, even thorugh load's & stores

  WerkOp *GetInput(sInt i) {KOp *op; op=Op.GetInput(i); return op?op->Werk:0;}
  sInt GetInputCount() {return Op.GetInputCount();}
  WerkOp *GetOutput(sInt i) {KOp *op; op=Op.GetOutput(i); return op?op->Werk:0;}
  sInt GetOutputCount() {return Op.GetOutputCount();}
  WerkOp *GetLink(sInt i) {KOp *op; op=Op.GetLink(i); return op?op->Werk:0;}
  sInt GetLinkMax() {return Op.GetLinkMax();}

};


/****************************************************************************/

class WerkPage : public sObject             // a page of operators
{
public:
  WerkPage();
  ~WerkPage();
  sU32 GetClass() { return sCID_WERKPAGE; }
  void Tag();
  void Clear();

// page content

  sChar Name[KK_NAME];                      // name of page
  sChar ListName[256];                      // name as diplayed on list "name|user|count"
  sList<WerkOp> *Ops;                       // operator list to work with. either original or scratchpad
  sList<WerkOp> *Orig;                      // operator list, original
  sList<WerkOp> *Scratch;                   // operator list, scratchpad
  sList<WerkSpline> *Splines;               // splines are stored per page, so that we can use owner-rights on splines
  sInt Changed;                             // operator has changed (reconnect or not)
  sInt Reconnect;                           // changed, need to reconnect
  class WerkDoc *Doc;                       // backlink
  sInt ScrollX;                             // save scroll position when changing 
  sInt ScrollY;
  sInt TreeLevel;                           // level in the Pagelist-tree
  sInt TreeButton;

// multi-user ownership

  sChar Owner[KK_NAME];                     // username of owner
  sInt OwnerCount;                          // usagecount of owner
  sChar NextOwner[KK_NAME];                  // pass rights from one owner to another
  sInt FakeOwner;                           // edit this although you shouldn't
  sInt MergeError;                          // if a merge-error happened 

  void Change(sBool reconnect=sFALSE);      // this page has changed in some way.
  void Connect1();                          // pass1: connect operators and collect stores
  void Connect2();                          // pass2: follow links
  void RenameOps(sChar *oldname,sChar *newname);
  sBool Access();                           // check if page may be modified

  void MakeScratch();                       // create scratch,copy from orig. switch to scratch
  void KeepScratch();                       // delete orig. copy scratch to orig, switch to orig
  void DiscardScratch();                    // delete scratch, swithc to orig
  void ToggleScratch();                     // switch between scratch & orig.
};

sList<WerkOp> *CopyOpList(sList<WerkOp> *src,WerkPage *page);
    
#define ME_NOTHING  0
#define ME_KEEP     1
#define ME_UPDATE   2
#define ME_NEW      3

/****************************************************************************/

class WerkEvent : public sObject            // an event in the timeline
{
public:
  WerkEvent();
  ~WerkEvent();
  sU32 GetClass() { return sCID_WERKEVENT; }
  void Tag();
  void Copy(sObject *);
  void Rename(sChar *o,sChar *n);

  KEvent Event;                             // event as in player
  WerkOp *Op;                               // operator for this event, may be 0
  WerkDoc *Doc;                             // backlink to doc, to remove event from KEnv
  WerkSpline *Spline;                       // spline for event
  sChar Name[KK_NAME];                      // name for editor
  sChar SplineName[KK_NAME];                // name of spline for event
  sInt Line;                                // line in timeline
  sInt Select;                              // selection in Gui (don't mix with Op.Select, which is something like Midi-Note)

};

/****************************************************************************/

class WerkDoc : public sObject              // the whole document
{
public:
  WerkDoc();
  ~WerkDoc();
  sU32 GetClass() { return sCID_WERKDOC; }
  void Tag();
  void Clear();

  sList<WerkPage> *Pages;                   // pages in document
  sList<WerkEvent> *Events;                 // events in timeline
  sList<WerkOp> *Stores;                    // temporary list of stored objects here

  sArray<WerkSpline *> EnvWerkSplines;         // splines for KEnvironment. 
  sArray<KSpline *> EnvKSplines;            // splines for KEnvironment. 
  KEnvironment *KEnv;                       // KEnvironment himself

  sChar DemoName[32];                       // Demo Info
  sChar SongName[64];
  sChar SampleName[64];
  sF32 SoundBPM;
  sInt BuzzTiming;
  sInt SoundOffset;
  sInt SoundEnd;                            // in 16:16 beats
  sInt FrCode;
  sF32 Aspect;
  sInt ResX,ResY;
  sF32 GlobalGamma;
  sInt PlayerType;
  sInt OutdatedOps;

  class WerkkzeugApp *App;

  WerkPage *AddCopyOfPage(WerkPage *);
  WerkPage *AddPage(sChar *name,sInt pos=-1);
  void RemPage(WerkPage *);

  sBool FindNameIndex(sChar *name,sInt &index);
  void AddStore(WerkOp *store);
  WerkOp *FindName(sChar *name);
  WerkOp *FindRoot(sInt index);             // es gibt jetzt MAX_OP_ROOT roots!

  void AddSpline(WerkSpline *);
  void RemSpline(WerkSpline *);

  void OwnAll();
  void ClearInstanceMem();
  void Connect();
  void ConnectAnim();
  void Flush(sInt kind=KC_ANY);
  void RenameOps(sChar *oldname,sChar *newname);
  WerkPage *FindPage(WerkOp *op);
  void ChangeClass(sInt classid);
  WerkSpline *FindSpline(sChar *name);

  void ClearCycleFlag();
  void DumpOpByClassR(sChar *name,WerkOp *op);
  void DumpOpByClass(sChar *name);
  void DumpUsedSwitchesR(WerkOp *op);
  void DumpUsedSwitches();
  void DumpReferencesR(WerkOp *ref,WerkOp *op);
  void DumpReferences(WerkOp *ref);
  void DumpBugsR(WerkOp *op);
  void DumpBugs();
  void DumpBugsRelative(WerkOp *base);
  void DumpMaterialsR(WerkOp *op);
  void DumpMaterials(WerkOp *base);

  sBool Read(sU32 *&data);
  sBool Write(sU32 *&data);
};

/****************************************************************************/

struct WerkExportOpSize
{
  sInt Graph;
  sInt Data;
  sInt Count;
  WerkOp *LastOp;
  sU8 *DStart;
};

#define MAXOPID 512

class WerkExport
{
public:
  WerkDoc *Doc;
  sList<WerkOp> *ExportSet;
  sArray<KSpline *> *SplineSet;
  sInt OpTypeRemap[MAXOPID];
  sInt OpTypes[256];
  WerkExportOpSize OpSize[MAXOPID];
  sInt OpTypeCount;

  WerkOp *SkipNops(WerkOp *op);

  void ExportSpline(KSpline *spline);
  void BuildExportSetR(WerkOp *op);

  sBool Export(WerkDoc *doc,sU8 *&data);
};

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   New Document Objects                                               ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

struct sMatrixEdit 
{
  sInt EditMode;
  union
  {
    sF32 Rotate[3];
    sF32 Target[3];
  };
  sF32 Pos[3];
  sF32 Scale[3];
  sF32 Tilt;

  void Init();
};

/****************************************************************************/


struct WerkEvent2Anim : public sObject
{
public:
  WerkEvent2Anim();
  ~WerkEvent2Anim();
  void Tag();

  sInt Offset;
  sInt AnimType;
  WerkSpline *Splines[4];
  sChar SceneNode[KK_NAME];
};

class WerkEvent2 : public sObject
{
public:
  WerkEvent2();
  ~WerkEvent2();
  sU32 GetClass() { return sCID_WERKEVENT2; }
  void Tag();
  void Clear();
  void Copy(sObject *o);
  void UpdateLinks(class WerkDoc2 *doc2,WerkDoc *doc1);
  sBool CheckError();
  void Calc(KEnvironment *kenv1);

  void AddSymbol(const sChar *name,sInt id,sInt type);
  void AddSceneExports(class WerkScene2 *scene);
  void AddAnimCode();
  void AddAnim(sInt offset,sInt type,const sF32 *data,const sChar *name);
  WerkEvent2Anim *FindAnim(sInt offset);
  WerkEvent2Anim *FindAnim(const sChar *name);


  KEvent2 Event;
  sInt Line;
  sInt LoopFlag;                  // enable timeline looping
  sBool Select;
  sChar Name[KK_NAME];
  sChar LinkName[K2MAX_LINK][KK_NAME];
  sChar AliasName[KK_NAME];                 // name to alias another event
  WerkEvent2 *AliasEvent;
  WerkOp *LinkOps[K2MAX_LINK];
  sText *Source;                            // animation script
  sText *CompilerErrors;
  sArray<KEffect2Symbol> Exports;           // symbols defined by effect and splines
  sList<WerkSpline> *Splines;               // splines. use the old ones.
  sList<WerkEvent2Anim> *Anims;             // anims that are not in code
  class WerkScene2 *LinkScenes[K2MAX_LINK];
  class WerkDoc2 *Doc;
  sBool Recompile;
};

class WerkSceneNode2 : public sObject
{
public:
  WerkSceneNode2();
  ~WerkSceneNode2();
  sU32 GetClass() { return sCID_WERKSCENENODE2; }
  void Tag();
  void Copy(sObject *o);
  void UpdateLinks(WerkDoc *doc1);

  // data for editor

  sInt HirarchicError;
  sInt TreeLevel;                 
  sInt TreeButton;
  sInt Selected;
  sInt Visible;
  sChar Name[KK_NAME];
  sChar ListName[KK_NAME+16];
  
  // data for engine

  sMatrixEdit MatrixEdit;
  sChar MeshName[KK_NAME];
  KSceneNode2 *Node;
  WerkOp *MeshOp;
};

class WerkScene2 : public sObject
{
public:
  WerkScene2();
  ~WerkScene2();
  sU32 GetClass() { return sCID_WERKSCENE2; }
  void Tag();
  void Copy(sObject *);
  void UpdateLinks();                       // link childs-KSceneNodes
  void CalcScene(KEnvironment *env1,sInt index=-1); // call "calc" for mesh ops
  void CalcSceneR(KEnvironment *env1,KSceneNode2 *n);

  sInt TreeLevel;
  sInt TreeButton;
  sChar Name[KK_NAME];
  sChar ListName[KK_NAME+16];
  sList<WerkSceneNode2> *Nodes;

  KSceneNode2 *Root;                         // root of all scene-nodes
  sBool Recompile;
};

class WerkScriptVM : public sScriptVM
{
  sF32 Data[64];
public:
  WerkScriptVM(WerkDoc2 *doc);
  WerkDoc2 *Doc;

  void InitFrame();
  void InitEvent(KEvent2 *event);
  sF32 GetTime() { return Data[25]; }
  void SetTime(sF32 time) { Data[25] = time; }
  sBool Extern(sInt id,sInt offset,sInt type,sF32 *data,sBool store);
  sBool UseObject(sInt id,void *object);
};

class WerkScriptCompiler : public sScriptCompiler
{
public:
  WerkScriptCompiler(WerkDoc2 *doc);
  WerkDoc2 *Doc;
  sBool ExternSymbol(sChar *group,sChar *name,sU32 &groupid,sU32 &offset);
};

class WerkDoc2 : public sObject             // the document, NOVA version. Needs support from WerkDoc(1).
{
public:
  WerkDoc2();
  ~WerkDoc2();
  sU32 GetClass() { return sCID_WERKDOC2; }
  void Tag();
  void Clear();
  sBool Read(sU32 *&data);
  sBool Write(sU32 *&data);

  sList<WerkEvent2> *Events;                 // events in timeline
  sList<WerkScene2> *Scenes;
  void SortEvents();
  void CompileEvents();
  void CompileEvent(WerkEvent2 *event,sInt global);
  void Build();
  void UpdateLinks(WerkDoc *doc1);
  void CalcEvents(sInt time);
  struct KEnvironment2 Env2;
  class WerkkzeugApp *App;

  WerkEvent2 *FindEvent(const sChar *name);
  WerkScene2 *FindScene(const sChar *name);
  KEffect2 *FindEffect(const sChar *name);
  KEffect2 *FindEffect(sInt id);
  WerkEvent2 *MakeEvent(KEffect2 *ef);

  WerkScriptVM *VM;
  WerkScriptCompiler *Compiler;
  sBool Recompile;                          // set to trigger recompile
};


/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   windows                                                            ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

class WinView;
class WinPage;
class WinTimeline;
class WinTimeline2;
class WinSpline;
class WinSplineList;
class WinSplinePara;
class WinPara;
class WinAnimPage;
class WinEvent;
class WinPerfMon;
class WinCalc;
class WinEventPara2;
class WinEventScript2;
class WinSceneList;
class WinSceneNode;
class WinSceneAnim;
class WinScenePara;
class WinNovaView;

/****************************************************************************/

class WinPagelist : public sGuiWindow
{
  sListControl *DocList;
  sChar DeligateName[KK_NAME];      // for edit-requester
public:
  class WerkkzeugApp *App;
  WinPagelist();
  ~WinPagelist();
  void OnInit();
  void OnCalcSize();
  sBool OnShortcut(sU32 key);
  void OnLayout();
  void OnPaint() {}
  sBool OnCommand(sU32 cmd);
  void UpdateList();
  void SetPage(WerkPage *page);

  sInt FullInfo;
};

/****************************************************************************/

class WinStatus : public sGuiWindow
{
public:
  class WerkkzeugApp *App;
  void OnPaint() { sPainter->Paint(sGui->FlatMat,Client,sGui->Palette[sGC_BACK]); sPainter->PrintC(sGui->PropFont,Client,0,
    "status"); }
};

/****************************************************************************/
/*
class WinAnim : public sGuiWindow
{
public:
  class WerkkzeugApp *App;
  void OnPaint() { sPainter->Paint(sGui->FlatMat,Client,sGui->Palette[sGC_BACK]); sPainter->PrintC(sGui->PropFont,Client,0,
    "anim"); }
};
*/
/****************************************************************************/

class WinEditPara : public sGuiWindow
{
  sGridFrame *Grid;
  sInt GSTitle;
  sInt GSFonts;
  sInt GSScroll;
  sInt GSSkin05;
  sU32 Color[4];
  sInt MaxLines;
  sInt TextureMem;
  sInt MeshMem;
  WerkkzeugApp *App;
public:
  WinEditPara();
  ~WinEditPara();
  void Tag();
  void OnLayout();
  void OnCalcSize();
  void OnPaint();
  sBool OnCommand(sU32 cmd);

  void SetApp(WerkkzeugApp *app);
};

/****************************************************************************/

class WinDemoPara : public sGuiWindow
{
  sGridFrame *Grid;
  WerkkzeugApp *App;
  sInt SongLengthBeat;
  sInt SongLengthFine;
public:
  WinDemoPara();
  ~WinDemoPara();
  void Tag();
  void OnLayout();
  void OnCalcSize();
  void OnPaint();
  sBool OnCommand(sU32 cmd);

  void SetApp(WerkkzeugApp *app);
};

/****************************************************************************/

class WinTimeBorder : public sGuiWindow
{
  sInt Height;
  sInt DragMode;
  sInt DragStart;
public:
  WerkkzeugApp *App;
  WinTimeBorder();
  void Tag();
  void OnCalcSize();
  void OnSubBorder();
  void OnPaint();
  void OnDrag(sDragData &dd);
};

/****************************************************************************/

class WinStatistics : public sReportWindow
{
  WerkkzeugApp *App;
public: 
  WinStatistics(WerkkzeugApp *);
  void Print();
};

class WinGameInfo : public sReportWindow
{
  WerkkzeugApp *App;
public: 
  WinGameInfo(WerkkzeugApp *);
  void Print();
  void OnDrag(sDragData &dd);
};

/****************************************************************************/

struct FindResult
{
  sChar TextBuf[256];
  sChar ExtraText[128];
  WerkOp *Op;
};

class WinFindResults : public sGuiWindow
{
  sChar TitleBuffer[256];
  WerkkzeugApp *App;
  sControl *Title;
  sArray<FindResult> Results;
  sListControl *ResultsList;

public:
  WinFindResults(WerkkzeugApp *);
  ~WinFindResults();

  void Tag();
  void OnLayout();
  void OnFrame();
  sBool OnCommand(sU32 cmd);

  void Clear();
  void Add(WerkOp *op,sChar *text = 0);
  void Update();
};

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Application                                                        ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

class WerkkzeugApp : public sDummyFrame
{
  sChar Stat[8][256];
  sInt BeatNow;                   // current beat
  sInt TimeNow;                   // current time
  sInt TimeSpeed;                 // Value calculated from BPM
  sInt TotalSample;               // total samples calculated
  sInt LastTime;                  // save last time during dragging
  sInt Switch0Mode;

  sInt RadioPage,RadioTime,RadioSpline,RadioScene;
  sInt NovaMode;

  sBool ShowFindResults;

public:
  WinView *ViewWin;
  WinPage *PageWin;
  WinPagelist *PagelistWin;
  WinStatus *StatusWin;
  WinTimeline *TimelineWin;
  WinTimeline2 *Timeline2Win;
  WinEventPara2 *EventPara2Win;
  WinEventScript2 *EventScript2Win;
  WinPara *ParaWin;
  WinEvent *EventWin;
  WinSpline *SplineWin;
  WinSplinePara *SplineParaWin;
  WinSplineList *SplineListWin;
//  WinAnim *AnimWin;
  WinAnimPage *AnimPageWin;
  WinTimeBorder *TimeBorderWin;
  WinStatistics *StatisticsWin;
  WinGameInfo *GameInfoWin;
  WinPerfMon *PerfMonWin;
  WinFindResults *FindResultsWin;
  WinCalc *CalcWin;

  WinSceneList *SceneListWin;
  WinSceneNode *SceneNodeWin;
  WinSceneAnim *SceneAnimWin;
  WinScenePara *SceneParaWin;
  WinNovaView *ViewNovaWin;


  sLogWindow *LogWin;
  sFileWindow *FileWindow;
  sSwitchFrame *Switch0;          // para window & stuff
  sSwitchFrame *Switch1;          // main window (page, spline, timeline,...)
  sSwitchFrame *Switch2;          // list window (pagelist, scenelist)
  sSwitchFrame *SwitchView;       // normal view / nova view
  sVSplitFrame *TopSplit;
  sHSplitFrame *ParaSplit;
  sStatusBorder *Status;
  sBrowserApp *OpBrowserWin;
  sChar OpBrowserPath[sDI_PATHSIZE];
  sChar WinTitle[256];

  // public variables

  WerkDoc *Doc;                   // the main document
  WerkDoc2 *Doc2;                 // .. the new main document, still incomplete
  class sMusicPlayer *Music;
  //class sViruz2 *Music;           // music player
  sInt MusicPlay;                 // 0=pause 1=play 2=drag
  sInt MusicLoop;                 // 0=cont. 1=loop
  sInt MusicLoopTaken;            // set after the loop was taken for the first time after beeing set. This is used to adjust the graphics-time to play the full loop.
  sInt MusicLoop0;                // 16:16 fixpoint loop start
  sInt MusicLoop1;                // 16:16 fixpoint loop end
  sInt AutoSaveMax;               // autosave timer minutes to wait
  sInt AutoSaveTimer;             // time of last save
  sChar UserName[KK_NAME];        // user name for multiuser sync
  sInt UserCount;                 // count for user usage 
  sInt MusicVolume;               // 0..256

  // window control

  WerkkzeugApp();
  ~WerkkzeugApp();
  void Tag();
  void OnPaint();
  void OnCalcSize();
  void OnKey(sU32 key);
  void OnDrag(sDragData &);
  void OnFrame();
  sBool OnDebugPrint(sChar *t);
  sBool OnShortcut(sU32 key);
  sBool OnCommand(sU32 cmd);

  // load and save

  sBool Read(sU32 *&);
  sBool Write(sU32 *&);
  sBool Merge(sU32 *&);
  sBool LoadConfig();
  sBool SaveConfig();
  void Clear();
  void Export(sChar *name);
  void MakeDemo(sChar *name);

  // tools

  void SetView(sInt nr);                    // nova/normal view
  void SetStat(sInt nr,sChar *string);      // change status bar
  void SetMode(sInt mode);                  // MODE_???
  sInt GetMode();
  void MusicReset();
  void MusicLoad();                         // load the tune, as in Doc
  void MusicStart(sInt mode);               // start and stop music
  void MusicSeek(sInt beat);                // set music to certain position
  void MusicNow(sInt &beat,sInt &ms);       // get current beat and time
  sBool MusicIsPlaying();
  void SoundHandler(sS16 *buffer,sInt samples); // internal sound handler
  void OpBrowser(sGuiWindow *sendto=0,sU32 cmd=0);
  void OpBrowserOff();

  void FindResults(sBool toggle);           // turn find results window on/off
  sBool GetNovaMode() { return NovaMode; }
}; 

#define STAT_DRAG       0
#define STAT_VIEW       1
#define STAT_FPS        2
#define STAT_MESSAGE    3

#define MODE_NORMAL     -1        // switch back from gamemode
#define MODE_PAGE       0
#define MODE_SPLINE     1
#define MODE_EVENT      2
#define MODE_SCENE      3
#define MODE_STATISTIC  4
#define MODE_GAMEINFO   5
#define MODE_PERFMON    6
#define MODE_LOG        7
#define MODE_EVENT2     8

/****************************************************************************/

#endif
#endif
