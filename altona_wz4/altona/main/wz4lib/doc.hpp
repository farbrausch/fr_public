/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) by Dierk Ohlerich                                    ***/
/***   all rights reserverd                                               ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WERKKZEUG4_DOC_HPP
#define FILE_WERKKZEUG4_DOC_HPP

#ifndef __GNUC__
#pragma once
#endif

#include "base/types2.hpp"
#include "gui/gui.hpp"
#include "gui/listwindow.hpp"
#include "base/serialize.hpp"
#include "util/shaders.hpp"

class sTexture2D;
class sMaterial;
class sViewport;
class WinView;

class ScriptContext;

/****************************************************************************/
/****************************************************************************/

enum wWerkkzeugConsts
{
  wMAXINPUTS = 16,                // max # of named inputs
  wNAMECOUNT = 64,                // max name count
  wPAGEXS = 192,
  wPAGEYS = 128,
  wSWITCHES = 10,                 // for switch op
};

typedef sString<wNAMECOUNT> wDocName;

struct wCommand;
class wObject;
class wExecutive;
class wOp;
class wDocument;
class wDocInclude;

#define sREGOPS(t,s) \
void AddTypes_##t##_ops(sBool); \
void AddOps_##t##_ops(sBool); \
if(i==0) AddTypes_##t##_ops(s); \
else AddOps_##t##_ops(s);

/****************************************************************************/
/****************************************************************************/

enum wTypeFlags
{
  wTF_NOTAB         = 0x01,
  wTF_RENDER3D      = 0x02,
  wTF_UNCACHE       = 0x04,       // use memorymanagement on this class
};

enum wTypeGuiSets
{
  wTG_2D            = 0x01,
  wTG_3D            = 0x02,
  wTG_MODE          = 0x04,
};

enum wHandleMode
{
  wHM_STATIC = 0,
  wHM_TEX2D,
  wHM_RAY,
  wHM_PLANE,                      // xz-plane
  wHM_PLANE_XY,                   // xy-plane
};

struct wHandle                    // this structure will be rebuild every frame
{
  sInt Mode;                      // wHM_???
  sInt *t;                        // value of handle: time and position
  sF32 *x;
  sF32 *y;
  sF32 *z;
  sRect HitBox;                   // for clicking
  wOp *Op;                        // identify by Op and Id
  sInt Id;
  sInt Index;                     // index in Handles[] 
  sInt SelectIndex;               // index in SelectedHandlesTag[], or -1
  sMatrix34 Local;                // transformation

  sInt ArrayLine;                 // link to gui to display array line
  sBool Selected;                 // this handle is selected

  void Clear() { sClear(*this); ArrayLine=-1; SelectIndex=-1; }
};

struct wHandleSelectTag           // this will persist as long as selection is active
{
  wOp *Op;
  sInt Id;
  sVector4 Drag;

  wHandleSelectTag() { Op=0; Id=0; }
  wHandleSelectTag(wOp *op,sInt id) { Id=id; Op=op; } 
};

class wPaintInfo
{
  sGeometry *TexGeo;              // sGD_QUADLIST,sVertexFormatSingle
  sMaterial *TexMtrl;
  sMaterial *TexAMtrl;
  sTexture2D *TexDummy;

  sVertexSingle *TexGeoVP;
  sInt TexGeoVC;

  sGeometry *LineGeo2;
  sVertexBasic *LineGeoVP[2];
  sInt LineGeoVC[2];

  void PaintHandle(sInt x,sInt y,sRect &r,sBool select);
  void PaintHandlesR(wOp *parent,sBool paint);

public:
  wPaintInfo();
  ~wPaintInfo();
  // general info

  WinView *Window;                // you SHOULD not use this, but it might be usefull anyway
  wOp *Op;                        // read only (for you)
  sRect Client;                   // window client area
  sBool ClearFirst;               // does the client area need to be cleared? (for Group3D chaining)
  sInt Enable3D;                  // true:graphics.hpp / false:windows.hpp
  sTargetSpec Spec;               // sSetTarget(sTargetPara(flags,color,pi.Spec));
  sInt TimeMS;
  sInt TimeBeat;
  sBool MTMFlag;
  sTextBuffer *ViewLog;
  sInt Lod;                       // 0..3 - low / med / high / extra
  sBool CacheWarmup;              // set by player during cache warmup
  sInt CacheWarmupAgain;          // an operator indicates, that he wants the same beat warmed up again. this is used for the mandelbulb op, which needs multiple frames of warmup

  // Handles

  sBool ShowHandles;              // enable handles
  sBool Dragging;                 // dragging mode
  sBool HandleEnable;             // paint this handle. is set automatically by handle recursion 
  sBool DeleteSelectedHandles;    // if this is true, please delete everything associated with a selected handle, from the handles() code
  sArray<wOp *> DeleteHandlesList;  // if DeleteSelectedHandles wants to delete an op, put it in this list for save, deferred deletion
  sInt DontPaintHandles;          // counter

  sArray<sInt> SelectedHandles;   // index into Handles[]
  sInt HandleMode;
  sU32 HandleColor;               // ARGB color used for painting.
  sInt HandleColorIndex;          // 2d color index for painting
  sRect SelectFrame;              // draw a rect as feedback for selection. just a gui service
  sBool SelectMode;

  sArray<wHandle> Handles;
  void AddHandle(wOp *op,sInt id,const sRect &r,sInt mode,sInt *t,sF32 *x,sF32 *y=0,sF32 *z=0,sInt arrayline=-1);
  void PaintHandles(const sMatrix34 *mat=0);
  void SelectHandle(wHandle *hnd,sInt mode=0);    // 0=clear all, 1=add, 2=rem
  sBool SelectionNotEmpty();
  void ClearHandleSelection();
  sBool IsSelected(wOp *op,sInt id) const;
  sInt FirstSelectedId(wOp *op) const;

  // base2d interface

  sInt PosX;                      // offset in client area
  sInt PosY;
  sInt Zoom2D;                    // 8 = normal size
  sBool Tile;
  sBool Alpha;

  // base3d interface

  sViewport *View;                // viewport. this will get manipulated by letterboxing
  sF32 Zoom3D;                    // zoom.
  sSimpleMaterial *FlatMtrl;            // sVertexFormatSingle ZON
  sSimpleMaterial *ShadedMtrl;          // sVertexFormatStandard
  sSimpleMaterial *DrawMtrl;            // sVertexFormatSingle, ZOFF
  sMaterialEnv *Env;                    // env is obsolete
  sViewport HandleView;           // viewport for painting handles
  sGeometry *LineGeo;             // sVertexFormatBasic;
  sBool Grid;
  sBool Wireframe;
  sBool CamOverride;
  sRay HitRay;
  sMatrix34 HandleTrans;          // transform handles by this
  sMatrix34 HandleTransI;          // transform handles by this
  sU32 BackColor;                 // for clearing the screen
  sU32 GridColor;
  sInt SplineMode;
  sBool SetCam;                   // overwrite interactive camera
  sMatrix34 SetCamMatrix;
  sF32 SetCamZoom;

  // misc parameters. use to pass information from "handles"/"drag" handlers to paint routine (e.g. cursor pos)
  
  sInt ParaI[8];
  sF32 ParaF[8];

  void ClearMisc();

  // special texture helpers

  class sImage *Image;            // use this as cache, to avoid memory allocations
  class sImage *AlphaImage;       // use this as cache, to avoid memory allocations
  sRect Rect;                     // texture rect after SetSize()


  void SetSizeTex2D(sInt xs,sInt ys); // set size of texture to calculate
  void MapTex2D(sF32 xin,sF32 yin,sInt &xout,sInt &yout);
  void PaintTex2D(sImage *img);      // do everything automatically.
  void PaintTex2D(sTexture2D *tex);
  void LineTex2D(sF32 x0,sF32 y0,sF32 x1,sF32 y1);
  void HandleTex2D(wOp *op,sInt id,sF32 &x,sF32 &y,sInt arrayline=-1);

  // special 3d helpers

  sBool Map3D(const sVector31 &pos,sF32 &x,sF32 &y,sF32 *zp);
  void Handle3D(wOp *op,sInt id,sVector31 &pos,sInt mode,sInt arrayline=-1);
  void Handle3D(wOp *op,sInt id,sF32 *x,sF32 *y,sF32 *z,sInt mode,sInt arrayline=-1);
  void Line3D(const sVector31 &a,const sVector31 &b,sU32 col=0,sBool zoff=0);
  void Transform3D(const sMatrix34 &mat);       // apply matrix to all child handles. matrix will be inverted

  // special anim helpers
/*
  sInt TimeToX(sInt time);
  sInt ValueToY(sF32 val);
  sInt XToTime(sInt x);
  sF32 YToValue(sInt y);
  void HandleAnim(wOp *op,sInt id,sInt &t,sF32 &v,sInt *sel=0,sInt arrayline=-1);
  void HandleAnim(wOp *op,sInt id,sF32 &t,sF32 &v,sInt *sel=0,sInt arrayline=-1);
*/
  // special material helpers

  void PaintMtrl();      // do everything automatically. Material must be set.
  void PaintAddRect2D(const sRect &rr,sU32 col);
  void PaintFlushRect();

  void PaintAddLine(sF32 x0,sF32 y0,sF32 z0,sF32 x1,sF32 y1,sF32 z1,sU32 col,sBool zoff);
  void PaintFlushLine(sBool zoff);
};

struct wHitInfo
{
  sBool Hit;                      // is this a hit?
  sVector31 Pos;                  // collision point
  sVector30 Normal;               // at collision point
  sF32 Dist;                      // ray start to collision point
  wObject *Mesh;                  // mesh at collision point (or 0)
  wObject *Material;              // material at collision point (or 0)
  sInt KDTriId;                   // tri id from kdtree
  sF32 HitU;                      // barycentric coordinate p = (1-u-v)*p0+u*p1+v*p2
  sF32 HitV;                      // barycentric coordinate p = (1-u-v)*p0+u*p1+v*p2
};

sOBSOLETE typedef wPaintInfo wPaintInfo3D;

class wType : public sObject
{
public:
  sCLASSNAME_NONEW(wType);
  wType();

  wType *Parent;                  // types can build hirarchies
  sInt Flags;
  sInt GuiSets;
  sU32 Color;
  sInt Secondary;                 // hide type in secondary menu
  sInt Order;                     // sorting order. set to 1..9 to assign type keyboard shortcuts 1..9
  const sChar *Label;             // user friendly name
  const sChar *Symbol;            // name of the class
  sPoolString ColumnHeaders[32];  // headers for columns in addop menu. 
  wType *EquivalentType;

  sBool IsType(wType *type);      // parameter type is parent of object type (or same)
  sBool IsTypeOrConversion(wType *type); // .. includes possible conversions from object to parameter type

  virtual void Init() {}
  virtual void Exit() {}
  virtual void BeforeShow(wObject *obj,wPaintInfo &pi) {}  // a hack used only for screenshots
private:                                          // please use Doc->Show(), don't call this directly
  friend class wDocument;
  virtual void BeginShow(wPaintInfo &pi) {}       // called for all classes before Show()
  virtual void EndShow(wPaintInfo &pi) {}         // called for all classes after Show()
  virtual void Show(wObject *obj,wPaintInfo &pi); // called for the class of the object only
public:
  virtual void ListExtractions(wObject *obj,void (* cb)(const sChar *name,wType *type),const sChar *storename) {}
  virtual sBool OverrideCamera(wObject *obj,sViewport &view,sF32 &zoom,sF32 time) { return 0; }
};


/****************************************************************************/

struct wGridFrameHelper : public sGridFrameHelper
{
  wGridFrameHelper(sGridFrame *frame) : sGridFrameHelper(frame) {}
  sMessage ConnectMsg;
  sMessage LayoutMsg;
  sMessage ConnectLayoutMsg;    // both: connect and layout
  sMessage LinkBrowserMsg;          // message that opens the link browser
  sMessage LinkGotoMsg;
  sMessage LinkPopupMsg;
  sMessage LinkAnimMsg;
  sMessage AddArrayMsg;
  sMessage RemArrayMsg;
  sMessage RemArrayGroupMsg;
  sMessage FileLoadDialogMsg;
  sMessage FileSaveDialogMsg;
  sMessage ActionMsg;             // 
  sMessage ArrayClearAllMsg;
  sMessage FileReloadMsg;
};

/****************************************************************************/

class wCustomEditor : public sObject
{
public:
  wCustomEditor();
  ~wCustomEditor();

  virtual void OnCalcSize(sInt &xs,sInt &ys);
  virtual void OnLayout(const sRect &Client);
  virtual void OnPaint2D(const sRect &Client);
  virtual sBool OnKey(sU32 key);
  virtual void OnDrag(const sWindowDrag &dd,const sRect &Client);
  virtual void OnChangeOp();
  virtual void OnTime(sInt time);

  void ChangeOp(wOp *op);
  void Update();
  void ScrollTo(const sRect &r,sBool safe);
};

/****************************************************************************/

enum wClassFlags
{
  wCF_VARARGS         = 0x00000001, // last input can be assigned multiple times (break hardcoded limit for add-style ops)
//  wCF_CANINPLACE      = 0x0002,   // can work in place (input[0]==output), and prefers that
  wCF_LOAD            = 0x00000004, // load-style operator body
  wCF_STORE           = 0x00000008, // style-style operator body
  wCF_HIDE            = 0x00000040, // hide in op palette
  wCF_CONVERSION      = 0x00000080, // this op can be used as conversion
  wCF_LOGGING         = 0x00000100, // enable sDPrintF logging window for this op.
  wCF_SLOW            = 0x00000200, // slow ops don't get calculated on change, only on request
  wCF_BLOCKHANDLES    = 0x00000400, // handles are not searched beyond this op. usually used with a nop.
  wCF_PASSINPUT       = 0x00000800, // steal input#0 from cache
  wCF_PASSOUTPUT      = 0x00001000, // allow stealing from cache
  wCF_CURVE           = 0x00002000, // special editing for animations (alternative view/para windows)
  wCF_CLIP            = 0x00004000, // special editing for animations (alternative view/para windows)
  sCF_OBSOLETE        = 0x00008000, // obsolete ops have a special mark in pagewin
  wCF_VERTICALRESIZE  = 0x00010000, // op can be resized vertically as well as horizontally (for nops, mainly)
  wCF_COMMENT         = 0x00020000, // this is a comment op. can't connect, and different rendering
  wCF_CALL            = 0x00040000, // modify build: call a subroutine
  wCF_INPUT           = 0x00080000, // modify build: inject subroutine argument
  wCF_LOOP            = 0x00100000, // modify build: loop input
  wCF_ENDLOOP         = 0x00200000, // modify build: stop loop processing. (not implemented)
  wCF_SHELLSWITCH     = 0x00400000, // modify build: depending on shell switch, use either input
  wCF_TYPEFROMINPUT   = 0x00800000, // op is tagged as AnyType, but actual type is same as input#0
  wCF_BLOCKCHANGE     = 0x01000000, // do not propagate changes to childs

  wCIF_METHODMASK     = 0x0007,   // method: link, input or optional=
  wCIF_METHODINPUT    = 0x0000,   // always input
  wCIF_METHODLINK     = 0x0001,   // always link
  wCIF_METHODBOTH     = 0x0002,   // choose between next input and link
  wCIF_METHODCHOOSE   = 0x0003,   // choose between specific input and link
  wCIF_METHODANIM     = 0x0004,
  wCIF_OPTIONAL       = 0x0010,
  wCIF_WEAK           = 0x0020,   // changing the input does not update the op, the op only has a reference of the object
};

struct wClassInputInfo            // information for every input of an operator
{
  sInt Flags;                     // wCIF_???
  class wType *Type;              // expected type
  class wClass *DefaultClass;     // possible default operator, if no input is given
  sPoolString Name;
};

struct wClassActionInfo
{
  sPoolString Name;
  sInt Id;

  void Init(sPoolString n,sInt i) { Name = n; Id = i; }
};

class wClass : public sObject     // information about a class of operators
{
public:
  sCLASSNAME(wClass);
  wClass();
  void Tag();

  sPoolString Name;               // internal name
  sPoolString Label;              // real name
  sInt Column;
  sInt Shortcut;
  sInt GridColumns;

  sInt Flags;                     // sCF_??
  sInt ParaWords;                 // # of 32 bit parameter words
  sInt ParaStrings;               // # of string parameters
  sInt HelperWords;               // # of 32 bit words in helper structure
  sInt ArrayCount;                // # of bytes in a line for an parameter array, or 0 if no parameter array (splines)
  sString<16> Extract;            // if this is an extraction op, this is the extraction prefix
  sU32 FileInMask;                // for each string, if the bit is set the string is a file load parameter
  sU32 FileOutMask;               // for each string, if the bit is set the string is a file save parameter
  sPoolString FileInFilter;       // file requester filter, like "bmp|png|pic|jpg"

  wType *OutputType;              // type of output
  wType *TabType;                 // type used for sorting op in tabs. usually output type
  sArray<wClassInputInfo> Inputs; // input info 
  sArray<wClassActionInfo> ActionIds; // names for actions. used for wDoc::GlobalAction()
  
  void (*MakeGui)(wGridFrameHelper &,wOp *op);
  sBool (*Command)(wExecutive *,wCommand *);   // provisorial: this should be more flexible...
  void (*SetDefaults)(wOp *op);
  void (*Handles)(wPaintInfo &pi,wOp *op);
  void (*SetDefaultsArray)(wOp *op,sInt pos,void *mem);
  sInt (*Actions)(wOp *op,sInt code,sInt pos);
  void (*SpecialDrag)(const sWindowDrag &dd,sDInt mode,wOp *op,const sViewport &,wPaintInfo &);
  void (*BindPara)(wCommand *cmd,ScriptContext *ctx);
  void (*Bind2Para)(wCommand *cmd,ScriptContext *ctx);
  void (*Bind3Para)(wOp *op,sTextBuffer &tb);
  const sChar *(*GetDescription)(wOp *op);
  wCustomEditor *(*CustomEd)(wOp *op);   // create custom editor for this op (may be 0)
  const sChar *WikiText;

  sInt Temp;
};

/****************************************************************************/

struct wOpInputInfo
{
  wDocName LinkName;              // link name for editing
  wOp *Link;                      // resolved link
  wOp *Default;                   // complete living copy of default op
  sBool DefaultUsed;              // indication to gui
  sInt Select;                    // 0=next ; 1=link ; 2=empty ; 3..n+2=select input-2
};

struct wScriptParaInfo
{
  sPoolString Name;
  sInt Type;                      // ScriptTypeInt or ScriptTypeFloat
  sInt Count;                     // dimension (max 4);
  sF32 Step;
  sF32 RStep;
  sBool Global;                   // make a global variable
  sF32 Min;                       // min (for all)
  sF32 Max;                       // max (for all)
  sF32 Default;                   // default (int and float)
  sInt GuiExtras;                 // string, color, 
  sInt GuiFlags;                  // 1:nolabel  2:flags
  sPoolString GuiChoices;
  union
  {
    sS32 IntVal[4];
    sF32 FloatVal[4];
    sU32 ColorVal;
  };
  sPoolString StringVal;
};

struct wScriptVar
{
  sPoolString Name;
  sInt Count;
  sInt Type;
  union
  {
    sS32 IntVal[4];
    sF32 FloatVal[4];
    const sChar *StringVal[4];
  };
};


class wOp : public sObject
{
public:
  sCLASSNAME_NONEW(wOp);
  wOp();
  ~wOp();
  void Finalize();
  void Tag();
  void CopyFrom(wOp *src);
  void Init(wClass *);
  wType *OutputType();            // finds out the output type of this op in its context and returns it
  wOp *MakeConversionTo(wType *type,sInt callid);
  wOp *MakeExtractionTo(const sChar *);
  void FileDialog(sDInt offset);
  void UpdateScript();

  template <class streamer> void Serialize_(streamer &stream);
  void Serialize(sWriter &);
  void Serialize(sReader &);

  // general info

  wDocName Name;
  sBool Bypass;                   // bypass is only allowed for stack ops
  wClass *Class;
  class wPage *Page;              // backlink to page, updated while connecting
  sBool CycleCheck;
  sBool CheckedByBuild;           // set by build when it has performed all checks in this op. also used to calc --depend
  const sChar *CalcErrorString;       // if set, a connection or execution error occurred
  const sChar *ConnectErrorString;       // if set, a connection or execution error occurred
  sBool ConnectError;             // first phase of connection (in wDocument) sets this flag to cancel further connection checking
  sBool SlowSkipFlag;             // this op was skipped due to a slow op
  sBool ConnectedToRoot;          // this op is an (indirect) child of the store op called "root"
  sBool NoError() { return CalcErrorString==0 && ConnectErrorString==0 && ConnectError==0; }
  sBool BlockedChange;            // a change was blocked! see wCF_BlockChange
  sBool ConversionOrExtractionUsed; // mark all used conversions and extractions, so we can delete those we do not need.

  sInt Select;                    // used by GUI
  sBool ImportantOp;              // this op requires caching for the gui.
  sInt HighlightArrayLine;        // used by GUI for parameter array
  sInt BlinkTimer;                // blink if sGetTime() < BlinkTimer
  sEndlessArray<sInt> SelectedHandles;  // Indices to pi.Handles[], or -1
  sInt Temp;

  void *EditData;                 // pure parameter data. use helpers below
  void *HelperData;               // temporary storage for Handle-Painting. not saved!
  sInt EditStringCount;
  sTextBuffer **EditString;       // string paramters
  sArray<wOpInputInfo> Links;     // link parameters
  sArray<wOp *> Inputs;           // input ops, as specified by user. needs to be mapped to op inputs
  sArray<wOp *> Outputs;          // output ops, may be more than one. including WeakOutputs
  sArray<wOp *> WeakOutputs;      // repeat the outputs that do not trigger update in gui.
  sArray<void *> ArrayData;       // parameter array data.
  sArray<wOp *> Conversions;      // conversion ops: store intermediate, so there is only one copy. does not need to be saved in document file
  sArray<wOp *> Extractions;      // conversion ops: store intermediate, so there is only one copy. does not need to be saved in document file

  sInt Strobe;                    // strobe parameter: will be reset after successful execution
  sU32 ConnectionMask;            // report from conection builder which input slots are connected. used for hiding buttons in gui
  sInt ArrayGroupMode;            // hiding and grouping of array elements

  // scripting

  sInt ScriptShow;                // button to show scripting source
  sTextBuffer ScriptSource;       // sourcecode, edited by user
  sArray<wScriptParaInfo> ScriptParas;  // slider parameters
  sInt ScriptSourceOffset;        // add this to ScriptSource to skip the slider definition
  sInt ScriptSourceValid;         // script source (After gui) has non-whitespace
  sInt ScriptSourceLine;
  ScriptContext *Script;

  void MakeSource(sTextBuffer &tb);
  ScriptContext *GetScript();

  // calc

  struct wNode *BuilderNode;      // see build.hpp. must be 0 before calling wBuilder::parse()
  sInt BuilderNodeCallId;         // if this was created during a call, this was the caller id
  sInt BuilderNodeCallerId;       // if this is a call, when this was called this id was used
  wObject *Cache;                 // permanently cached copy of data
  sU32 CacheLRU;
  sArray<wScriptVar> CacheVars;   // script vars associated to Cached Object
  wObject *WeakCache;             // if this op has weak outputs, cache the pointer here for reuse
  wCommand *CalcTemp;             // command with result during calculation
  sArray<wOp *> OldInputs;        // compare old and new inputs to check for reconnection change
  sObject *FeedbackObject;        // memory managed object set by calc as feedback to Gui

  // provide some random types for memory management. used by handles

  sObject *GCParent;
  sObject *GCObj;
  wObject *RefParent;
  wObject *RefObj;

  // helpers

  sU32 *EditU() { return (sU32 *) EditData; }   // EditData helpers
  sS32 *EditS() { return (sS32 *) EditData; }
  sF32 *EditF() { return (sF32 *) EditData; }
  void *AddArray(sInt pos=-1);      // -1 = at the end
  void RemArray(sInt pos);
  template <class T> T* GetArray(sInt index) { return (T*) ArrayData[index]; }
  sInt GetArrayCount() { return ArrayData.GetCount(); }
  wOp *FirstInputOrLink();
  sBool CheckShellSwitch();
};

class wTreeOp : public wOp
{
public:
  sCLASSNAME_NONEW(wTreeOp);
  wTreeOp();
  void Tag();

  // this serialization is not used during document serialisation, only for clipboard
  template <class streamer> void Serialize_(streamer &stream);
  void Serialize(sWriter &);
  void Serialize(sReader &);

  sListWindowTreeInfo<wTreeOp *> TreeInfo;
};

class wStackOp : public wOp
{
public:
  sCLASSNAME_NONEW(wStackOp);
  wStackOp();
  void Tag();
  void CopyFrom(wStackOp *src);

  // this serialization is not used during document serialisation, only for clipboard
  template <class streamer> void Serialize_(streamer &stream);
  void Serialize(sWriter &);
  void Serialize(sReader &);

  sInt PosX;
  sInt PosY;
  sInt SizeX;
  sInt SizeY;
  sBool Hide;
};

/****************************************************************************/

class wOpData     // store all data of an op, for undo / redo
{
  sArray<sU32> Data;
  sArray<sPoolString> Strings;
  sArray<sPoolString> LinkNames;
  sArray<sInt> LinkFlags;
  sArray<void *> ArrayData;       // parameter array data.
  sInt ElementBytes;
  wOp *Valid;
public:
  wOpData();
  ~wOpData();
  void CopyFrom(wOp *);
  void CopyTo(wOp *);
  sBool IsSame(wOp *);
  sBool IsValid(wOp *);
  void Clear();
};

/****************************************************************************/

class wDocInclude : public sObject
{
public:
  sString<256> Filename;
  sBool Active;
  sBool Protected;

  wDocInclude();
  ~wDocInclude();
  void Tag();
  void Clear();                   // remove pages from Doc
  void New();                     // create empty page in Doc
  sBool Load();                    // load pages from file
  sBool Save();                    // save pages to file

  template <class streamer> void Serialize_(streamer &s);
  void Serialize(sWriter &);
  void Serialize(sReader &);
};

/****************************************************************************/

class wPage : public sObject
{
public:
  sCLASSNAME(wPage);
  wPage();
  void Tag();
  sBool CheckDest(wOp *op,sInt x,sInt y,sInt w,sInt h,sBool move);
  sBool CheckMove(sInt dx,sInt dy,sInt dw,sInt dh,sBool move);
  sBool IsProtected() { return this==0 || ManualWriteProtect || (Include && Include->Protected); }
  void Rem(wOp *);

  // this serialization is not used during document serialisation, only for clipboard
  template <class streamer> void Serialize_(streamer &stream);
  void Serialize(sWriter &);
  void Serialize(sReader &);

  wDocName Name;
  wDocInclude *Include;           // 0 = not an include
  void DefaultName();
  sBool ManualWriteProtect;

  sBool IsTree;                   // tree or stack?
  sInt ScrollX;                   // stack page scrolling
  sInt ScrollY;
  sArray<wStackOp *> Ops;
  sArray<wTreeOp *> Tree;
  sInt Temp;

  sListWindowTreeInfo<wPage *> TreeInfo;
};

/****************************************************************************/

enum wEditOptionsFlags
{
  wEOF_IGNORESLOW  = 1,           // always calculate slow commands
  wEOF_GRAYUNCONNECTED = 0x0002,  // gray out ops that are not connected to root
};

struct wEditOptions 
{
  // startup 
  sPoolString File;               // last opened file
  sInt Screen;                    // switch to this wire-screen

  // misc options
  sU32 BackColor;                 // 3d view background color
  sU32 GridColor;
  sU32 AmbientColor;              // default ambient color
  sInt SplineMode;                // not written
  sInt Flags;         
  sInt Volume;                    // 0..100
  sF32 ClipNear;
  sF32 ClipFar;
  sInt MoveSpeed;                 // 0..3 slow|normal|fast|ultra
  sInt AutosavePeriod;            // seconds till autosave
  sInt MultiSample;               // multisample level.   0 = off  ;  1..n = level 0..n-1
  sInt ZoomFont;                  // enlarge debug font
  sInt MemLimit;                  // in megabyte
  sInt ExpensiveIPPQuality;       // 0=low 1=medium 2=high
  sInt DefaultCamSpeed;           // mousewheel factor -20..20 -> 2^n speed 

  enum
  {
    TH_DEFAULT,
    TH_DARKER,
    TH_CUSTOM,
  };
  sInt Theme;
  sGuiTheme CustomTheme;
 
  // timeline options

  void Init();
  template <class streamer> void Serialize_(streamer &stream);
  void Serialize(sWriter &);
  void Serialize(sReader &);
  void ApplyTheme();
};

enum wDocOptionsDialogFlags
{
  wDODF_Benchmark       = 0x0001, // display benchmark button
  wDODF_Multithreading  = 0x0002, // display "reserve core" button
  wDODF_SetResolution   = 0x0004, // default to ScreenX/ScreenY resolution instead of desktop resolution
  wDODF_LowQuality      = 0x0008, // quality low (1) or high (0) (default high)
  wDODF_NoLoop          = 0x0010, // hide loop button
  wDODF_HiddenParts     = 0x0020, // menu for hidden parts

  wDODH_Infinite        = 0x0001, // demo loops and time does not restart when song restarts
  wDODH_Dialog          = 0x0002, // Include in hidden parts box
  wDODH_FreeFlight      = 0x0004, // start with freeflight on
};


struct wDocOptions
{
  sPoolString ProjectPath;        // "c:/nxn/test"
  sPoolString ProjectName;        // "test"
  sInt ProjectId;                 // numerical id, eg. FR number
  sInt SiteId;                    // numerical id for eg. website in player (prod database id)
  sInt TextureQuality;            // full - dxt - low
  sInt LevelOfDetail;             // low - med - high - extra
  sInt BeatsPerSecond;            // beats per second * 0x10000. set to 0x10000 for a 60 BPM = seconds based timeline
  sInt BeatStart;                 // at time 0 it is already BeatStart. may be negative
  sInt Beats;                     // max time in beats
  sInt Infinite;                  // play initinitely (for hidden parts)
  sPoolString MusicFile;          // an ogg, please
  sInt ScreenX,ScreenY;           // requested resolution
  sPoolString PageName;           // last openend page
  sPoolString Packfiles;          // semicolon seperated list of packfiles to load on startup
  sInt SampleRate;                // of songs
  sInt DialogFlags;               // customize the player dialog

  sInt VariableBpm;
  struct BpmSegment 
  {
    sInt Beats;                   // not multiplied by 0x10000
    sF32 Bpm;                     // pure float

    // calculated
    sInt Bps;                     // multiplied by 0x10000
    sInt Milliseconds;            // pure ms
    sInt Samples;                 // pure samples
  };
  sArray<BpmSegment> BpmSegments;

  struct HiddenPart
  {
    sPoolString Code;             // what you have to type (or select)
    sPoolString Store;            // store with demo
    sPoolString Song;             // filenamne of song (can not be heard in wz4, only used by player)
    sF32 Bpm;                     // beats per minute (may not be variable)
    sInt LastBeat;                // total length of song
    sInt Flags;                   // wDODH_???
  };
  sArray<HiddenPart> HiddenParts;

  void Init();
  template <class streamer> void Serialize_(streamer &stream);
  void Serialize(sWriter &);
  void Serialize(sReader &);

  void UpdateTiming();
};

enum wTestOptionEnum
{
  wTOM_Compare = 0,
  wTOM_Write = 1,
  wTOM_CrossCompare = 2,

  wTOP_Unknown = 0,
  wTOP_DirectX9 = 1,
  wTOP_DirectX11 = 2,

  wTOF_NoCompare = 0x0001,
  wTOF_NOCrossCompare = 0x0002,
};

struct wTestOptions
{
  sInt Mode;
  sInt Compare;
  sBool FailRed;

  void Init();
};

struct wScriptDefine
{
  sInt Mode;                      // 1=string, 2=int, 3=float
  sPoolString Name;
  sPoolString StringValue;
  sInt IntValue;
  sF32 FloatValue;
};

class wDocument : public sObject
{
  static const sInt VERSION=12;

//  sBool ReconnectFlag;
  void ConnectTree(sArray<wTreeOp *> &);
  void ConnectStack(wPage *page);
  sRandom Rnd;                    // for creating random strings;
public:
  sCLASSNAME(wDocument);
  wDocument();
  ~wDocument();
  void Finalize();
  void Tag();
  void New();
  sBool Load(const sChar *filename);
  sBool Save(const sChar *filename);
  void DefaultDoc();
  void RemoveType(wType *);       // remove types that accidentally registered. usefull for stripping down a special version of the wz4
//  void Reconnect();               // connection has changed
  void ConnectError(wOp *op,const sChar *text);
  void Connect();                 // perform connection if flagged
  void Change(wOp *op,sBool ignoreweak=0,sBool dontnotify=sFALSE);
  void ChangeR(wOp *op,sBool ignoreweak,sBool dontnotify,wOp *from);
  void ChangeDefaults(wOp *op);
  void FlushCaches();
  void ChargeCaches();
  void UnblockChange();
  const sChar *CreateRandomString();
  wObject *CalcOp(wOp *op,sBool honorslow=0);
  void Show(wObject *obj,wPaintInfo &pi);
  void PropagateCalcError(wOp *op);
  void CalcDirtyWeakOps();
  void ClearSlowFlags();
  sBool RenameAllOps(const sChar *from,const sChar *to);
  sBool UnCacheLRU();
  sU32 CacheLRU;
  void GlobalAction(const sChar *name);

  sInt SecondsToBeats(sF32 t);
  sInt MilliSecondsToBeats(sInt t);
  sInt BeatsToMilliseconds(sInt b);
  sInt BeatsToSample(sInt b);
  sInt SampleToBeats(sInt s);

  static void SerializeOptions(sReader &s,wDocOptions &options);
  template <class streamer> void Serialize_(streamer &stream);
  void Serialize(sWriter &);
  void Serialize(sReader &);
  sInt UnknownOps;                // during loading, counts unknown ops

  wType *FindType(const sChar *name);
  wClass *FindClass(const sChar *name,const sChar *tname);
  wOp *FindStore(const sChar *name);
  wOp *FindStoreNoExtr(const sChar *name);
  sBool IsOp(wOp *op);

  sString<2048> Filename;
  sBool DocChanged;
  sViewport LastView;             // copy last view of a selected viewport here, so we can use its position for editing ops.
  sArray<wDocInclude *> Includes; // included documents
  sTargetSpec Spec;               // target spec of the screenmode
  sBool IsPlayer;
  sInt LowQuality;
  sBool IsCacheWarmup;

  wDocOptions DocOptions;
  wEditOptions EditOptions;
  wTestOptions TestOptions;

  sArray<wPage *> Pages;
  sArray<wType *> Types;
  sArray<wClass *> Classes;
  sArray<wClass *> Conversions;
  sArray<wClass *> Extractions;
  wDocName LastName;              // name of last op / load link. used to create new loads and stores
  sArray<sPoolString> PageNames;  // used to sort pages after loading includes
  sArray<wHandleSelectTag> SelectedHandleTags;
  
  sArray<wOp *> AllOps;
  sArray<wOp *> Stores;
  sArray<wOp *> DirtyWeakOps;     // when a weakly linked op is changed, it must be updated at the next possible situation

  wPage *CurrentPage;             // stack or view window, as currently shown on screen

  class wExecutive *Exe;
  class wBuilder *Builder;
  sTextBuffer *ViewLog;           // log on screen (sPainter) during ShowOp()

  sMessage AppChangeFromCustomMsg;
  sMessage AppScrollToArrayLineMsg;

  sArray<sInt> CacheWarmupBeat;   // used for demo player, set by clip.

  // command line switches for switch operator

  sInt ShellSwitches;           // for switch operator
  sString<64> ShellSwitchOptions[wSWITCHES];
  sString<wSWITCHES*64+1> ShellSwitchChoice;
  void CheckShellSwitches();
  sBool BlockedChanges;           // true if there are changes that got blocked.
  sArray<wScriptDefine> ScriptDefines;

  // these "actions" allow werkkzeug customization. you may use the global "Doc" ptr.

  void (*PostLoadAction)();       // called directly after a new doc was loaded
  void (*FinalizeAction)();       // called just before the document is destructed finally. (at application exit)
};

extern class wDocument *Doc;

/****************************************************************************/
/****************************************************************************/

class wObject
{
protected:
  virtual ~wObject();
public:
  wObject();
  wType *Type;
  sInt RefCount;

  sInt CallId;


  void AddRef()    { if(this) RefCount++; }
  void Release()   { if(this) { if(--RefCount<=0) delete this; } }
  sBool IsType(wType *type) { return Type->IsType(type); }   // output->IsType(input). obj type is of type, or type is parent of obj type. 
  virtual void Reuse()  { sFatal(L"this class can not be used for weak linking."); }
  virtual wObject *Copy()  { return 0; }
};

struct wCommand
{
public:
  sBool (* Code)(wExecutive *,wCommand *);
  sInt DataCount;                 // value parameters
  sInt StringCount;               // string parameters
  sInt InputCount;                // inputs
  sInt FakeInputCount;            // inputs including fake inputs. fake inputs are only for scripting and come after regular inputs
  sInt OutputRefs;                // count of references to output
  sInt ArrayCount;                // array value parameters, count of entries
  sInt PassInput;                 // -1=off, otherwise pass input# into output
  sInt CallId;                    // write this to object
  sInt ErrorFlag;                 // used for error propagation
  sInt LoopFlag;                  // called through subroutine or loop

  sU32 *Data;                     // value parmeters
  const sChar **Strings;          // string parameters
  wCommand **Inputs;              // references the Output of the command
  void *Array;                    // array parameters, flattened out as an array of whatever structure you used.

  wObject *Output;                // the output object
  sInt OutputVarCount;
  wScriptVar *OutputVars;         // scriptvars after execution
  wOp *StoreCacheOp;              // ???

  void (*ScriptBind2)(wCommand *,class ScriptContext *);
  sPoolString LoopName;
  sF32 LoopValue;
  const sChar *ScriptSource;
  ScriptContext *Script;

  template <class Type> Type GetInput(sInt n) { return (n<InputCount && Inputs[n]) ? (Type(Inputs[n]->Output)) : 0; }  // save way to access input
  wOp *Op;                        // this is not present in player.

  void Init() { sClear(*this); PassInput = -1; }
  sInt GetStrobe() { return Op ? Op->Strobe : 0; }
  void SetError(const sChar *str) { ErrorFlag=1; if(Op && Op->CalcErrorString==0) Op->CalcErrorString = str; }
  void AddOutputVar(wScriptVar &var);
};

class wExecutive
{
  void BeginLogging();
  void EndLogging();
public:
  wExecutive();
  ~wExecutive();
  sArray<wCommand *> Commands;
  sMemoryPool *MemPool;
  sArray<wType *> Outputs;

  wObject *Execute(sBool progress,sBool depend=0);
};

void OpPrint(const sChar *text);
sPRINTING0(OpPrintF,sFormatStringBuffer buf; sFormatStringBaseCtx(buf,format);buf,OpPrint(buf.Get());)
//sPRINTING0(OpPrintF,sString<1024> &sXDPrintFBufferT = sGetCurrentThreadContext().PrintBuffer; sFormatStringBuffer buf=sFormatStringBase(sXDPrintFBufferT,format);buf,OpPrint(sXDPrintFBufferT);)

void ViewPrint(const sChar *text);
sPRINTING0(ViewPrintF,sFormatStringBuffer buf; sFormatStringBaseCtx(buf,format);buf,ViewPrint(buf.Get());)

void NXNCheckout(const sChar* filename);

/****************************************************************************/
/****************************************************************************/

extern void (*ProgressPaintFunc)(sInt count, sInt max);

/****************************************************************************/

#endif // FILE_WERKKZEUG4_DOC_HPP

