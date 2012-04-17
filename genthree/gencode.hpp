// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef __GENCODE_HPP
#define __GENCODE_HPP

#include "_util.hpp"
#include "_gui.hpp"
#include "genplayer.hpp"

class ToolDoc;
class ToolObject;
class ToolWindow;
class sToolApp;

#define FIRSTGLOBAL    8                     // magic constant from system.txt

/****************************************************************************/

sObject *FindLabel(sObject *,sInt label);
sChar *OffsetName(sU32 cid,sInt offset);
sInt AdressName(sU32 cid,sInt offset);
sBool DontCache(sU32 cid);
sInt OffsetSize(sU32 cid,sInt offset);

/****************************************************************************/


#define TCS_LOAD    1
#define TCS_STORE   2
#define TCS_ROOT    4             // root of recursion (maybe)
#define TCS_GENERATED 8           // during code generation, mark stores that are already generated.
#define TCS_VALID   16            // node is valid. this is cleared when note gets deleted!
#define TCS_CACHED  32            // this is a cached. Implies TCS_LOAD
#define TCS_ADD     64            // special code for handling varibale arguments "add" style functions
#define TCS_FXALLOC 128

#define TCG_DISCARD 1             // you may modify this object
#define TCG_KEEP    2             // you may not modify this object
#define TCG_POINTER 3             // you may not even copy this object

#define TC_INOUTMAX   32

struct ToolCodeSnip
{
  sU32 Flags;                     // TCS_???
  class ToolObject *CodeObject;   // backlink to object
  class ToolObject *StoreObject;  // backlink to object
  sInt OutputCID;                 // type for output
  sInt InputCount;                // inputs
  sInt OutputCount;               // outputs
  sInt OriginalOutputCount;       // outputs before split
  ToolCodeSnip *Inputs[TC_INOUTMAX];
  ToolCodeSnip *Outputs[TC_INOUTMAX];
  sChar Name[32];                 // name for load/store
  sInt Index;                     // variable index
  sInt NeedsCache;                // needs cache
  sBool FXViewport;               // true = negative outputcount
};

struct ToolCodeLabel
{
  sChar *Name;
  sObject *Root;
  sChar *RootName;
};

struct ToolCodeLabelName
{
  sChar Name[32];
};

struct ToolCodeGen
{
private:
  ToolCodeSnip *NewSnip();        // get new snip from array
  void Emit(sChar *format,...);   // add to source
  void Emit(ToolCodeSnip *);      // emit code for this snip
  void EmitR(ToolCodeSnip *);    // emit code for this snip. return TCG_??
  void AddR(ToolObject *to,ToolCodeSnip *root);  // trivial tree generation
  sInt ChangeR(ToolCodeSnip *);   // check changes 

public:
  void Init();
  void Exit();
  sInt MakeLabel(sChar *name);    // get id of label. create new one if not found

  sChar *System;                  // system.txt
  sArray<ToolCodeSnip*> Snips;    // add a snip to every operator
  sInt SnipsAllocated;            // how many snips are already allocated by NewSnip
  sArray<ToolCodeLabel> Labels;   // animation targets, make shure nothing is generated twice
  sArray<ToolCodeLabelName> LabelNames; // global list of label names. this list is never reset and gives you unique mappings
  sU32 *Bytecode;                 // save bytecode-ptr after compiling. 
                                  // please don't delete ScriptCompiler, this is not a copy!
  sInt State;                     // Begin=1 / End=2 / Execute=3 / Cleanup=0
  sToolApp *App;                  // link to app for FindObject();
  sInt Error;                     // if error, do nothing
  sInt Index;                     // global variable index
  sInt Temp;                      // temporary variable name

  sChar *Source;                  // the code ends up here
  sInt SourceUsed;                
  sInt SourceAlloc;

  class TimeDoc *Timeline[16];          // timelines to add to this 
  sInt TimelineCount;

  void Begin();                   // initialize array
  void AddResult(ToolObject *);   // add ToolObject as result. Cache will be set for this one
  void AddTimeline(class TimeDoc *);    // add timeline_OnInit() and timeline_OnFrmae();
  sBool End(ScriptCompiler *,sBool addsource=0,sBool doexport=0);    // resolve graph and generate code
  sBool Execute(ScriptRuntime *); // (optional) execute code and set Cache 
  void Cleanup();                 // cleans resources
};


/****************************************************************************/


#endif
