// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __KDOC_HPP__
#define __KDOC_HPP__

#include "_types.hpp"
#include "_start.hpp"

/****************************************************************************/

struct KLogic;                    // game logic operation
struct KOp;                       // Operator
struct KEvent;                    // Event in timeline
struct KSpline;                   // Spline
struct KInstanceMem;              // per event and operator memory
class KEnvironment;               // glue everything together

/****************************************************************************/
                                  // program limits
#define KK_MAXLINK    16          // linked inputs
#define KK_MAXSPLINE  8           // spline inpuits
#define KK_MAXINPUT   256         // conected inputs
#define KK_MAXKEYS    128         // keys per spline, each dimension has it's own
#define KK_NAME       32          // editor-name (not stored here)
#define KK_MAXVAR     32          // variables for animation
#define KK_MAXVARSAVE 256         // space for saving variables 

#define KK_MAXJOBS    1024        // max paintjobs
#define MAX_OP_ROOT   16          // there are multiple roots nowadays

                                  // animation options
#define KAF_INT       0           // integral numbers
#define KAF_FIXED     1           // 16 bit fixed point
#define KAF_FLOAT     2           // 32 bit float
#define KAF_UBYTE     3           // ubyte, like for colors

#define KAM_MUL       0x0001      // multiply instead of adding
#define KAM_VELOCIFY  0x0002      // multiply spline with velocity
#define KAM_MODULATE  0x0004      // multiply spline with modulation
#define KAM_SELECT    0x0010      // replace time with select
#define KAM_MOUSEX    0x0020      // replace time with mousex
#define KAM_MOUSEY    0x0030      // replace time with mousex

                                  // KObject Class ID's
#define KC_NULL       0           // no input allowed
#define KC_BITMAP     1           // bitmap
#define KC_MINMESH    2           // minimal mesh
#define KC_SCENE      3           // list of meshes
#define KC_KKRIEGER   4           // the game itself
#define KC_MATERIAL   5           // material
#define KC_MESH       6           // a real mesh
#define KC_IPP        8           // image postprocessing
#define KC_EFFECT     9           // special effect, painted in scene like meshes but with own code
#define KC_SMESH      10          // simple mesh (but with bsp)
#define KC_DEMO       11          // image postprocessing
#define KC_SCENE2     12          // new scene. this is no OP!
#define KC_SPLINE     13          // a real mesh
#define KC_COUNT      14          // number of distinct object classes
//#define KC_UPDATE     254         // an update object, that's no real object
#define KC_ANY        255         // any class allowed as input

/****************************************************************************/

#define KA_NOP        0x00        // no operation
#define KA_END        0x01        // end of program
#define KA_LOADVAR    0x02        // load variable
#define KA_LOADPARA1  0x03        // load original operator parameter
#define KA_LOADPARA2  0x04        // load original operator parameter
#define KA_LOADPARA3  0x05        // load original operator parameter
#define KA_LOADPARA4  0x06        // load original operator parameter
#define KA_SWIZZLEX   0x07        // x -> xyzw
#define KA_SWIZZLEY   0x08        // y -> xyzw
#define KA_SWIZZLEZ   0x09        // z -> xyzw
#define KA_SWIZZLEW   0x0a        // w -> xyzw
#define KA_ADD        0x0b        // a+b
#define KA_SUB        0x0c        // a-b
#define KA_MUL        0x0d        // a*b
#define KA_DIV        0x0e        // a/b
#define KA_MOD        0x0f        // a%1
#define KA_INVERT     0x10        // 1-a
#define KA_NEG        0x11        // -a
#define KA_SIN        0x12        // sin(t)
#define KA_COS        0x13        // cos(t)
#define KA_PULSE      0x14        // pulse(t,phase)
#define KA_RAMP       0x15        // ramp(t)
#define KA_CONSTV     0x16        // const float vector
#define KA_CONSTC     0x17        // const color
#define KA_CONSTS     0x18        // const scalar
#define KA_SPLINE     0x19        // spline(t,"name")
#define KA_EVENTSPLINE      0x1a  // eventspline(t) -> use spline from event
#define KA_MATRIX           0x1b  // apply matrix
#define KA_NOISE      0x1c        // noise
#define KA_EASE       0x1d        // ease in / out

#define KA_STOREVAR   0x80        // store variable, with writemask
#define KA_STOREPARAFLOAT   0x90  // store parameter, with writemask
#define KA_STOREPARAINT     0xa0  // store parameter, with writemask
#define KA_STOREPARABYTE    0xb0  // store parameter, with writemask
#define KA_CHANGEPARAFLOAT  0xd0  // store parameter, with writemask, changing
#define KA_CHANGEPARAINT    0xe0  // store parameter, with writemask, changing
#define KA_CHANGEPARABYTE   0xf0  // store parameter, with writemask, changing

sInt CalcCmdSize(sU8 *bytecode);         // calc size of one single bytecode command
sInt CalcCodeSize(sU8 *bytecode);        // returns size of bytecode fragment
sBool AnimCodeWritesTo(sU8 *code,sInt offset);  // find if offset is changed by code

/****************************************************************************/

#define KV_TIME       0x00        // basic event
#define KV_VELOCITY   0x01
#define KV_MODULATION 0x02
#define KV_SELECT     0x03
#define KV_EVENT_S    0x04        // event matrix
#define KV_EVENT_R    0x05
#define KV_EVENT_T    0x06
#define KV_EVENT_C    0x07        // event color
#define KV_CAM_S      0x08        // camera matrix
#define KV_CAM_R      0x09
#define KV_CAM_T      0x0a
#define KV_0b         0x0b        // unused
#define KV_PLAYER_S   0x0c        // player matrix
#define KV_PLAYER_R   0x0d
#define KV_PLAYER_T   0x0e
#define KV_0f         0x0f        // unused
#define KV_LEG_L0     0x10        // walker-leg position
#define KV_LEG_R0     0x11
#define KV_LEG_L1     0x12
#define KV_LEG_R1     0x13
#define KV_LEG_L2     0x14
#define KV_LEG_R2     0x15
#define KV_LEG_L3     0x16
#define KV_LEG_R3     0x17
#define KV_18         0x18      // unused
#define KV_19         0x19      // unused
#define KV_1a         0x1a      
#define KV_LEG_TIMES  0x1b      // walker timing { time, 0, 0, 0 }
#define KV_MATRIX_I   0x1c      // general purpose matrix (set by Limb)
#define KV_MATRIX_J   0x1d
#define KV_MATRIX_K   0x1e
#define KV_MATRIX_L   0x1f

// variable misuse for monster

// KV_EVENT_S                   // attack timer { melee, ranged, life-percent, 0 }

/****************************************************************************/

// calling convention bitmasks, in the order of parameters

sInt inline OPC_GETDATA(sU32 x)   {return (x&0x000000ff);}
sInt inline OPC_GETINPUT(sU32 x)  {return (x&0x00000f00)>>8;}
sInt inline OPC_GETLINK(sU32 x)   {return (x&0x0000f000)>>12;}
sInt inline OPC_GETSTRING(sU32 x) {return (x&0x00070000)>>16;}
sInt inline OPC_GETSPLINE(sU32 x) {return (x&0x00700000)>>20;} // i assume that 2 bit are enough, tho

#define OPC_BLOB          0x00080000  // blob is stored in op. this does not change the actual calling convention
#define OPC_KENV          0x00800000  // push KENV in INIT
#define OPC_VARIABLEINPUT 0x01000000  // use Call(x,y,...); convention (for add)
#define OPC_SKIPEXEC      0x02000000  // nothing special happens during the exec phase
#define OPC_ALTEXEC       0x04000000  // alternative calling convention for exec (no parameter)
#define OPC_OUTPUTCOUNT   0x08000000  // add outputcount as parameter
#define OPC_DONTCALLLINK  0x10000000  // don't call link. used to pass events.
#define OPC_ALTINIT       0x20000000  // alternative calling convention for ínit (no parameter except op)
#define OPC_KOP           0x40000000  // push KOp in INIT (op is before env)
#define OPC_FLEXINPUT     0x80000000  // non-fixed number of input parameters (export)

// - the inputcount will be in the specified range
// - all inputs within inputcount are valid
// - links may be failty
// - if there is is an op as input or link, it will have a cached object
// this is true for Init and Exec
// in the player, this is explicitly assumed
// in the werkkzeug, this is checked in Init / Exec
// if DONTCALLLINK is set, links are not checked in Init.

/****************************************************************************/

struct KLogic                               // effect connected to cell
{
  sU8 Code;                                 // KKEC_??? KKEE_???
  sU8 Condition;                            // additional switch condition for switch and value groups. use switch 0 which is always on
  sU8 Output;                               // output: event or switch
  sU8 pad;
  sInt Value;                               // value for value-group, switch for logic group
  KOp *Event[2];                            // (possible) event to issue
};
                                            // what to do
#define KKEC_NOP                  0x00      // nothing
                                            // switches
#define KKEC_ENABLESWITCH         0x01      // set switch on event
#define KKEC_DISABLESWITCH        0x02      // clear switch on event
#define KKEC_TOGGLESWITCH         0x03      // toggle switch on event
#define KKEC_SETSWITCH            0x04      // set event (0/1) to switch
                                            // collectables
#define KKEC_SETVALUE             0x05      // output = value
#define KKEC_MAXVALUE             0x06      // output = max(output,value)
#define KKEC_MINVALUE             0x07      // output = min(output,value)
#define KKEC_INCVALUE             0x08      // output = output + value
#define KKEC_DECVALUE             0x09      // output = output - value
                                            // game physics
#define KKEC_JUMP                 0x0a      // accelerate up by value*0.001f
#define KKEC_RESPAWN              0x0b      // set respawn zone for player, value=0..9
/*
                                            // switch logic
#define KKEC_ANDLOGIC             0x0a      // output = condition & value
#define KKEC_ORLOGIC              0x0b      // output = condition | value
#define KKEC_NOTLOGIC             0x0c      // output = !condition
*/
                                            // when to do
#define KKEE_NOP                  0x00      // never
#define KKEE_ENTER                0x10      // when entering a zone
#define KKEE_LEAVE                0x20      // when leaving a zone
#define KKEE_COLLECT              0x30      // when entering a zone, once
#define KKEE_RESPAWN0             0x40      // when entering a zone, will respawn soon
#define KKEE_RESPAWN1             0x50      // when entering a zone, will respawn 
#define KKEE_RESPAWN2             0x60      // when entering a zone, will respawn 
#define KKEE_RESPAWN3             0x70      // when entering a zone, will respawn late
#define KKEE_USE                  0x80      // when pressing the use button inside a zone
#define KKEE_HIT                  0x90      // when a shot "enters" a zone
#define KKEE_ALWAYS               0xa0      // always, with respect to the condition
#define KKEE_INSIDE               0xb0      // when inside
#define KKEE_HITORENTER           0xc0      // when hit or entered

/****************************************************************************/

class KObject                     // base cass
{
public:
  sInt ClassId;                   // KC_??
  sInt RefCount;                  // 

  KObject();
protected: virtual ~KObject();
public:
  virtual void Copy(KObject *o);  // copy from object
  virtual KObject *Copy();        // duplicate object
  void AddRef() {RefCount++;}
  void Release() {RefCount--;if(RefCount==0) delete this;}
};                                // 12 bytes (vtable+8)

/****************************************************************************/

struct KInstanceMem               // used for instance-storage. derive from this
{
  KOp *Operator;                  // operator requesting storage
  KInstanceMem *Next;             // next instance mem
  KInstanceMem **DeleteChild;     // recurse to this child when DeleteChain(). 
  KInstanceMem **DeleteChild2;    // recurse to this child when DeleteChain(). 
  void *DeleteArray;              // call delete[] on this when DeleteChain(). 
  sBool Reset;                    // set true on first encounter, automatically reset
  void DeleteChain();             // delete a list of KInstanceMem, beginning with *this. can be called on 0-pointers
};

struct KEvent                     // timeline event
{
  void Init();                    // clear all
  void Exit();
  void Stop();                    // stop event by changing time. do this only to dynamic events!

  sMatrix Matrix;                 // event base matrix
  KOp *Op;                        // execute this operator
  KEvent *NextOp;                 // linked list of active events per op
  sInt Start;                     // time start
  sInt End;                       // time end
  sInt Mark;                      // connect KEvent and KSpline
  sInt ShotTime;                  // only used for reflect shots
  sInt Active;                    // obsolete
  KInstanceMem *FirstInstance;    // first instance in chain
  sInt Dynamic;                   // dynamic events must be deleted when removed
  sInt RemoveMe;                  // terminate this event

  sF32 Velocity;                  // event variable velocity
  sF32 Modulation;                // event variable modulation
  sInt Select;                    // event variable selection, 0..16 maps to time 0..1
  sVector Scale;                  // event variable
  sVector Rotate;                 // event variable
  sVector Translate;              // event variable
  sU32 Color;                     // event variable
  KSpline *Spline;                // spline for event
  sF32 StartInterval;             // start interval time (default: 0)
  sF32 EndInterval;               // end interval time (default 1)

  // obscure special hardcoded shit...
//  sMatrix ReturnMatrix;           // pass back a matrix to system, used by WALK
  struct KKriegerMonster *Monster;// connect collision to monster, used by PHYSIC
  sInt CullDist;                  // set if you want distance-culling
};

/****************************************************************************/
/****************************************************************************/

struct KLetterMetric
{
  sFRect UV;
  sF32 PreSpace;
  sF32 Width;
};

/****************************************************************************/
 
                                  // calc-flags
#define KCF_NEED          0x0001  // result is really needed. this flag is cleared during recursion when no changes are found.
#define KCF_ROOT          0x0002  // tool-memory-management: do not flush root
#define KCF_PRECALC       0x0004  // progress bar and memory management

class KEnvironment                // variables for 
{
public:
  KEnvironment();
  ~KEnvironment();

  void InitView();                                // prepare for viewing. Set after Init and before Exec, or whenever the viewed op was changed in the editor
  void InitFrame(sInt beattime,sInt timems);      // prepare for frame and time
  void ExitFrame();                                // clean up afterwards
//  void Calc(KOp *op);                             // call Init_???()
  void AddDynamicEvent(KEvent *event);            // call this to register event. event gets deleted by KEnv. this is used by TRIGGER
  void AddStaticEvent(KEvent *event);             // add static events. memory is managed by caller. call this directly after InitFrame(). Events come from Document and Game

  sInt ExecuteAnim(KOp *,sU8 *bytecode);          // calculate animation
  void Pop(sInt count);                           // always call this with the result of Calc to rollback changed variables
  void *GetInstImpl(KOp *me,sInt size);           // get next instance-mem (implementation)
  template <class t> t *GetInst(KOp *me);         // get next instance-mem (or alloc new)
  void SetMatrix(sMatrix &mat,sMatrix &save);     // set matrix in variable,
  void RestoreMatrix(sMatrix &save);              // clean it up;

  // internal state
  sMemStack Mem;                  // memory for all kinds of structures
  sVector Stack[KK_MAXVARSAVE];   // save modified variables here for rollback
  sU8 StackAdr[KK_MAXVARSAVE];    // save it here. 
  sInt Index;                     // for stack
  sArray<KEvent *> DynamicEvents; // use AddDynamicEvent() to add here
  sArray<KOp *> EventOpsCleanup;  // reset event-chain in these ops

  // current event environment

  sVector Var[KK_MAXVAR];         // current set of variables
  KSpline *EventSpline;           // spline attached to current event (or 0)
  struct KKriegerMonster *EventMonster;// monster from event
  KEvent *CurrentEvent;
 
  // instance memory
  KInstanceMem *NextInstance;     // if there is instance storage, this is the next chunk
  KInstanceMem **LinkInstance;    // if there isn't, allocate new and store here
  KInstanceMem *DefaultInstance;  // instance-list for "no event" state
  sInt ClearInstanceMem;          // clear instance-mem for all static events added. this is set by InitView

  // set by caller
  KSpline **Splines;              // link to splines
  sInt SplineCount;
  struct KKriegerGame *Game;      // link to game
#if sLINK_KKRIEGER
  sInt NextMonsterOverride;       // use the following values to override next monster created
  sInt NextMonsterKillSwitch;
  sInt NextMonsterSpawnSwitch;
  sInt NextMonsterFlags;
#endif

  sMaterialEnv GameCam;           // camera from game or user-interface.
  sMaterialEnv CurrentCam;        // camera from viewport op.
  sF32 Aspect;
  sInt BeatTime;                  // current time in 16:16 beats
  sInt CurrentTime;               // current time in ms
  sInt LastTime;                  // last frames time
  sInt TimeDelta;                 // current-last
  sInt TimeSlices;                // timeslices since last frame (10ms)
  sInt TimeJitter;                // fraction of time since last frame
  sInt TimeReset;                 // noticed backwards movement in time

  sMatrix Markers[32];            // use Scene_Marker to set. useful hack for Effect_ChainLine

  struct BlobCameraInfo
  {
    sMatrix Matrix;
    sF32 ZoomX;
    sF32 ZoomY;
  } Camera;


  // fonts
  KLetterMetric Letters[8][256];  // set by TEXT, used by PRINT

  sMatrixStack ExecStack;         // Matrix stack for scene transforms
};

/****************************************************************************/

struct KClass                     // Operator class
{
  sU32 Convention;                // calling convention +flag 0x10000000 (varinputs)
  sChar *Packing;                 // packing string
  void *InitHandler;              // initialize operator
  void *ExecHandler;              // execute operator
}; // =16 bytes

struct KHandler                   // Operator handler
{
  sInt Id;                        // operator id
  void *InitHandler;              // initialize operator
  void *ExecHandler;              // execute operator
};

extern KHandler KHandlers[];

/****************************************************************************/

struct KOp                        // Operator
{
  friend class KEnvironment;
  sInt Command;                   // class of operator
  sInt Result;                    // class of result
  KObject *Cache;                 // result pointers, depending on class
#if sPLAYER
  sBool CacheFreed;               // cache is freed
#endif
  sInt Changed;                   // cache is dirty
  sInt DataInvalid;               // this operator has been animated, please copy DataCopy to Data
  sInt CacheCount;                // on creation set to output-count, then reduced with each link
  sInt WheelSpeed;                // used only for spinning wheels in the gui...
  KEvent *FirstEvent;             // list of active events for this operator
  void *InitHandler;              // the actual code
  void *ExecHandler;              // the actual code
  sInt CalcError;                 // could not calculate cache
  sU32 Convention;                // calling conventions for code
  sU32 ChangeMask;                // which inputs don't update the "change" bit?
  sInt OpId;                      // operator id (used to locate blobs+shaders)
  sInt SkipExec;                  // dont' call exec phase when (a) all ops are not animated and (b) all ops don't need exec
  sInt SkipCalc;                  // if the op is not changed, you may stop recursion here...
  sInt Tag;                       // for counting
#if !sPLAYER
  sInt Highlight;                 // this operator is above the edit operator in the op-tree
  sInt CycleFlag;                 // for cyclic check
  sInt CycleChangeFlag;           // for cyclic check
  sInt CacheSize;                 // for memory management
  sBool Blocked;                  // for memory management
#endif
  sBool AnimFlag;                 // op is (somehow) animated
  sInt VarStart;                  // animated variable range of this op
  sInt VarCount;                  // (generated during Init_, used during Exec_)
  KInstanceMem *SceneMemLink;     // linked list of instance memory used for scenejobs

private:
  sInt InputMax;                  // allocated per configuration                 
  sInt LinkMax;
  sInt OutputMax;
  sInt DataMax;
  sInt StringMax;                 // number of strings, size of strings not included
  sInt SplineMax;                 // number of splines
  sInt AnimMax;                   // size of command-string, 4-byte aligned

  sInt InputCount;                // inputs used
  sInt OutputCount;               // outputs used

  sU32 *Memory;                   // a block of memory, for everything to do

  KOp **Input;                    // inputs
  KOp **Link;                     // links
  KOp **Output;                   // outputs (not set in player!)
  sChar **String;                 // dynamic strings
  KSpline **Spline;               // array of spline-pointers
  sU8 *AnimCode;                  // animation code
  sU32 *DataEdit;                 // dynamic parameter array 
  sU32 *DataAnim;                 // dynamic parameter array 

  sInt BlobSize;                  // binary large object. Set OPC_BLOB to use!
  const sU8 *BlobData;                  // in Player, this points directly into export data. in Werkkzeug, this gets deleted[] on Exit().

public:

// interface

  sInt Calc(KEnvironment *,sInt flags);   // recursivly calculate, returns sTRUE when changed
  KObject *Call(KEnvironment *);  // InitHandler Recursion just call this op

  void ExecWithNewMem(KEnvironment *,KInstanceMem **link);
  void ExecEvent(KEnvironment *,KEvent *,sBool newmem);// ExecHandler Recursion, set and reset variables for event
  void Exec(KEnvironment *);      // (*ExecHandler)(op,kenv,...)

  void ExecInputs(KEnvironment *);// ExecHandler Recursion
  void ExecInput(KEnvironment *,sInt i);// ExecHandler Recursion
  void Change(sInt input=-1);     // do all stuff needed to update change status
  sBool CheckOutput(sInt kc);     // check that this op outputs a valid object of the specified type. this also checks the THIS pointer for 0, so you may call it on zero pointers
  void UpdateVar(sInt start,sInt count);
  sBool IsWrittenTo(sInt offset); // checks if offset is written to in bytecode
/*
  sU32 GetU(sInt i) {return DataU[i];}            // getter for ops
  sS32 GetS(sInt i) {return DataS[i];}
  sF32 GetF(sInt i) {return DataF[i];}
  */
  KOp *GetInput(sInt i) {return i<InputCount?Input[i]:0;}
  KOp *GetLink(sInt i) {return i<LinkMax?Link[i]:0;}
  KObject *GetLinkCache(sInt i) {return i<LinkMax?(Link[i]?Link[i]->Cache:0):0;}
  KOp *GetOutput(sInt i) {return Output[i];}
  sChar *GetString(sInt i) {return String[i];}
  KSpline *GetSpline(sInt i) {return i<SplineMax?Spline[i]:0;}
  sInt GetInputCount() {return InputCount;}
  sInt GetOutputCount() {return OutputCount;}
  sInt GetLinkMax() {return LinkMax;}
  sInt GetDataWords() {return DataMax;}
  sInt GetStringMax() {return StringMax;}
  sInt GetSplineMax() {return SplineMax;}
  sU8 *GetAnimCode() {return AnimCode;}

  const sU8 *GetBlob(sInt &size) {size=BlobSize; return BlobData;}  // get blob (if there)
  sInt GetBlobSize() {return BlobSize;}
  const sU8 *GetBlobData() {return BlobData;}
  void SetBlob(const sU8 *data,sInt size); // set blob, no copy is made, so please do not delete memory.

  void Init(sU32 convention);     // allocate buffers to fit this convention
  void Exit();                    // free buffers
#if !sPLAYER                      // interface for player to manage dynamic array
  void ReAlloc(sInt newin,sInt newout); // realloc, eventually changing a string
  void SetString(sInt i,sChar *c);
  void SetSpline(sInt i,KSpline *s);
  void ClearInput() {InputCount=0;}
  void ClearOutput() {OutputCount=0;}
  void AddInput(KOp *op);
  void AddOutput(KOp *op);
  void SetLink(sInt i,KOp *op) {if(i<LinkMax) Link[i]=op; else sVERIFY(op==0);}
  void SetAnimCode(sU8 *AnimCode,sInt size);
  void CopyEditToAnim();
  void Uncache();
  sBool HasAnim()              {return GetAnimCode()[0] != KA_END;}

  sU8  *GetEditPtr (sInt i) {return ((sU8 *)DataEdit)+i;}
  sU32 *GetEditPtrU(sInt i) {return ((sU32*)DataEdit)+i;}
  sS32 *GetEditPtrS(sInt i) {return ((sS32*)DataEdit)+i;}
  sF32 *GetEditPtrF(sInt i) {return ((sF32*)DataEdit)+i;}
  sU8  *GetAnimPtr (sInt i) {return ((sU8 *)DataAnim)+i;}
  sU32 *GetAnimPtrU(sInt i) {return ((sU32*)DataAnim)+i;}
  sS32 *GetAnimPtrS(sInt i) {return ((sS32*)DataAnim)+i;}
  sF32 *GetAnimPtrF(sInt i) {return ((sF32*)DataAnim)+i;}

  class WerkOp *Werk;             // backlink to werkop. no refcount, no nothing. just do it right
#else
  void AddInput(KOp *op)                {Input[InputCount++] = op;}
  void AddOutput(KOp *op)               {OutputCount++;}
  void SetLink(sInt i,KOp *op)          {Link[i]=op;}
  void SetString(sInt i,sChar *s)       {String[i]=s;}
  void SetSpline(sInt i,KSpline *s)     {Spline[i]=s;}
  void SetAnimCode(sU8 *code,sInt size) {AnimCode=code; AnimMax=size;}
  void SetBlobData(const sU8 *data)     {BlobData = data;}
  void CalcUsed();

  sU8  *GetEditPtr (sInt i) {return ((sU8 *)DataEdit)+i;}
  sU32 *GetEditPtrU(sInt i) {return ((sU32*)DataEdit)+i;}
  sS32 *GetEditPtrS(sInt i) {return ((sS32*)DataEdit)+i;}
  sF32 *GetEditPtrF(sInt i) {return ((sF32*)DataEdit)+i;}
  sU8  *GetAnimPtr (sInt i) {return ((sU8 *)DataAnim)+i;}
  sU32 *GetAnimPtrU(sInt i) {return ((sU32*)DataAnim)+i;}
  sS32 *GetAnimPtrS(sInt i) {return ((sS32*)DataAnim)+i;}
  sF32 *GetAnimPtrF(sInt i) {return ((sF32*)DataAnim)+i;}
#endif
};

/****************************************************************************/

struct KSplineKey                 // spline key !! don't change order for sHermite
{
  sF32 Time;                      // time   
  sF32 Value;                     // value
#if sLINK_GUI
  sF32 OldTime;                   // used for dragging
  sInt Select;                    // (only gui)
#endif
};

#define KSI_CUBIC     0
#define KSI_LINEAR    1
#define KSI_STEP      2

struct KSplineChannel             // spline
{
  sInt KeyCount;                  // number of keys (may be 0)
  KSplineKey *Keys;               // sorted array of spline keys
  sF32 Eval(sF32 time,sInt inter);// calculate spline pos
};

struct KSpline
{
  KSplineChannel Channel[4];      // spline channels
  sInt Count;                     // number of channels
  sInt Interpolation;             // KSI_??? kind of interpolation
  void Eval(sF32 time,sVector &v);// eval vector. fill with zero
#if !sPLAYER
  sInt SaveId;                    // number in export
#endif
};

struct KDoc
{
  void Init(const sU8 *&data);
  void Exit();

  sU8 *SongData;                  // only set in player
  sInt SongSize;
  sU8 *SampleData;
  sInt SampleSize;
  sInt BuzzTiming;
  sInt SongBPM;                   // not used in editor, only for player
  sInt SongLength;

  sArray<KOp> Ops;
  sArray<KEvent> Events;
  sArray<KSpline> Splines;
  KOp *RootOps[MAX_OP_ROOT];
  sInt CurrentRoot;

  void Reset();
  void Precalc(KEnvironment *kenv);
  void AddEvents(KEnvironment *kenv);

  sInt CountOps(KOp *start);
};

/****************************************************************************/

#if !sPLAYER

struct KObjectList                // object list for KMemoryManager
{
  sInt Size;                      // memory size currently spent in that class
  sInt Budget;                    // memory budget available
  sArray<KOp *> Ops;              // op pointers (LRU order)
};

class KMemoryManager_
{
  KObjectList Objects[KC_COUNT];  // one for each class

public:
  KMemoryManager_();
  ~KMemoryManager_();

  sInt GetBudget(sInt type);
  void SetBudget(sInt type,sInt size);  // set budget for that object type
  sInt GetSize(sInt type);        // size currently spent for that type
  sInt GetCount(sInt type);       // # of cached ops in that type
  void Use(KOp *op);              // mark op as used
  void Remove(KOp *op);           // remove op from list (and from memory)
  void Manage();                  // make room if necessary

  void Validate();                // consistency check
};

extern KMemoryManager_ *KMemoryManager;

void MemManagerInit();
void MemManagerExit();

#endif

/****************************************************************************/
/****************************************************************************/

class GenScene;

KObject * __stdcall Init_Misc_Nop(KOp *op);
KObject * __stdcall Init_Misc_If(KOp *op);
KObject * __stdcall Init_Misc_Load(KObject *o);
KObject * __stdcall Init_Misc_Event(KObject *o,sF32 duration);
KObject * __stdcall Init_Misc_Trigger(KObject *o,KOp *event,
  sF32 trigger,sF32 tresh,sF32 delay,sF32 duration,sInt flags,
  sF32 mod,sF32 vel,sInt sel,sF323 s,sF323 r,sF323 t,sU32 col,KSpline *spline);
GenScene * __stdcall Init_Misc_PlaySample(GenScene *scene,sInt sw,sInt val,sF32 tresh,sInt sample,sF32 volume,sF32 retrigger,sF32 halfrange);
KObject * __stdcall Init_Misc_Demo(KOp *op);
KObject * __stdcall Init_Misc_MasterCam(sF323 rot,sF323 pos,sF32 farclip,sF32 nearclip,sF32 centerx,sF32 centery,sF32 zoomx,sF32 zoomy);
void __stdcall Exec_Misc_Nop(KOp *op,KEnvironment *kenv);
void __stdcall Exec_Misc_Load(KOp *op,KEnvironment *kenv);
void __stdcall Exec_Misc_Time(KOp *op,KEnvironment *kenv);
void __stdcall Exec_Misc_Event(KOp *op,KEnvironment *kenv,sF32 duration);
void __stdcall Exec_Misc_Trigger(KOp *op,KEnvironment *kenv,
  sF32 trigger,sF32 tresh,sF32 delay,sF32 duration,sInt flags,
  sF32 mod,sF32 vel,sInt sel,sF323 s,sF323 r,sF323 t,sU32 col,KSpline *spline);
void __stdcall Exec_Misc_Delay(KOp *op,KEnvironment *kenv,sInt flags,sInt sw,sF32 tresh,sF32 hyst,sF32 time0,sF32 time1,sF32 time2);
void __stdcall Exec_Misc_State(KOp *op,KEnvironment *kenv,sInt sw,sInt val,sInt cond,sInt condsw,sInt condval);
void __stdcall Exec_Misc_SetIf(KOp *op,KEnvironment *kenv,sInt sw,sInt val,sInt var,sInt osw);
void __stdcall Exec_Misc_Menu(KOp *op,KEnvironment *kenv,sInt sw,sInt max,sInt flags);
void __stdcall Exec_Misc_If(KOp *op,KEnvironment *kenv,sInt sw,sInt val);
void __stdcall Exec_Misc_PlaySample(KOp *op,KEnvironment *kenv,sInt sw,sInt val,sF32 tresh,sInt sample,sF32 volume,sF32 retrigger,sF32 halfrange);
void __stdcall Exec_Misc_Demo(KOp *op,KEnvironment *kenv);
void __stdcall Exec_Misc_Menu(KOp *op,KEnvironment *kenv,sInt sw,sInt max,sInt flags);
void __stdcall Exec_Misc_MasterCam(KOp *op,KEnvironment *kenv,sF323 rot,sF323 pos,sF32 farclip,sF32 nearclip,sF32 centerx,sF32 centery,sF32 zoomx,sF32 zoomy);

class GenDemo : public KObject
{
public:
  GenDemo();
  ~GenDemo();
};

/****************************************************************************/
/****************************************************************************/

template <class t> __forceinline t * KEnvironment::GetInst(KOp *me)
{
  return (t *)GetInstImpl(me,sizeof(t));
}

/****************************************************************************/
/****************************************************************************/

#endif  
