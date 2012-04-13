// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __W3TEXMAIN_HPP__
#define __W3TEXMAIN_HPP__

#include "_types.hpp"
#include "w3texlib.hpp"

/****************************************************************************/

struct W3Image;

namespace Werkk3TexLib
{

/****************************************************************************/

#define KK_MAXLINK    16          // linked inputs
#define KK_MAXSPLINE  8           // spline inpuits
#define KK_MAXINPUT   256         // conected inputs
#define KK_MAXKEYS    128         // keys per spline, each dimension has it's own
#define KK_NAME       32          // editor-name (not stored here)
#define KK_MAXVAR     32          // variables for animation
#define KK_MAXVARSAVE 256         // space for saving variables 

                                  // KObject Class ID's
#define KC_NULL       0           // no input allowed
#define KC_BITMAP     1           // bitmap
#define KC_COUNT      2           // number of distinct object classes
#define KC_ANY        255         // any class allowed as input

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

// root flags

#define RF_NOALPHA       0x00000001 // fill alpha with 255

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

struct KClass                     // Operator class
{
  sInt Id;                        // The operator ID
  sU32 Convention;                // calling convention +flag 0x10000000 (varinputs)
  sChar *Packing;                 // packing string
  void *InitHandler;              // initialize operator
}; // =16 bytes

/****************************************************************************/

#define KCF_NEED          0x0001  // result is really needed. this flag is cleared during recursion when no changes are found.
#define KCF_ROOT          0x0002  // tool-memory-management: do not flush root
#define KCF_PRECALC       0x0004  // progress bar and memory management
#define KCF_STRIPPEDOK    0x0008  // accept stripped ops as valid

struct KOp                        // Operator
{
  friend class KEnvironment;
  sInt Command;                   // class of operator
  KObject *Cache;                 // result pointers, depending on class
  sInt CacheCount;                // on creation set to output-count, then reduced with each link
  void *InitHandler;              // the actual code
  void *ExecHandler;              // the actual code
  sU32 Convention;                // calling conventions for code
  sChar *Packing;                 // the packing string
  sInt OpId;                      // operator id (used to locate blobs+shaders)

private:
  sInt InputMax;                  // allocated per configuration                 
  sInt LinkMax;
  sInt OutputMax;
  sInt DataMax;
  sInt StringMax;                 // number of strings, size of strings not included

  sInt InputCount;                // inputs used
  sInt OutputCount;               // outputs used

  sU32 *Memory;                   // a block of memory, for everything to do

  KOp **Input;                    // inputs
  KOp **Link;                     // links
  KOp **Output;                   // outputs (not set in player!)
  sChar **String;                 // dynamic strings
  sU32 *DataEdit;                 // dynamic parameter array 

  sInt BlobSize;                  // binary large object. Set OPC_BLOB to use!
  const sU8 *BlobData;                  // in Player, this points directly into export data. in Werkkzeug, this gets deleted[] on Exit().

public:

// interface
  sBool CalcNoRec(sBool decreaseInRef); // non-recursive calc, returns sFALSE on error
  sInt Calc(sInt flags);                // recursivly calculate, returns sTRUE when changed
  KObject *Call();                      // InitHandler Recursion just call this op
  void Purge();                         // free all calculated data

  KOp *GetInput(sInt i) {return i<InputCount?Input[i]:0;}
  KOp *GetLink(sInt i) {return i<LinkMax?Link[i]:0;}
  KObject *GetLinkCache(sInt i) {return i<LinkMax?(Link[i]?Link[i]->Cache:0):0;}
  KOp *GetOutput(sInt i) {return Output[i];}
  sChar *GetString(sInt i) {return String[i];}
  sInt GetInputCount() {return InputCount;}
  sInt GetOutputCount() {return OutputCount;}
  sInt GetLinkMax() {return LinkMax;}
  sInt GetDataWords() {return DataMax;}
  sInt GetStringMax() {return StringMax;}

  const sU8 *GetBlob(sInt &size) {size=BlobSize; return BlobData;}  // get blob (if there)
  sInt GetBlobSize() {return BlobSize;}
  const sU8 *GetBlobData() {return BlobData;}

  void Init(sU32 convention);     // allocate buffers to fit this convention
  void Exit();                    // free buffers
  void AddInput(KOp *op)                {Input[InputCount++] = op;}
  void AddOutput(KOp *op)               {OutputCount++;}
  void SetLink(sInt i,KOp *op)          {Link[i]=op;}
  void SetString(sInt i,sChar *s)       {String[i]=s;}
  void SetBlobSize(sInt size)           {BlobSize = size;}
  void SetBlobData(const sU8 *data)     {BlobData = data;}
  void CalcUsed();

  sU8  *GetEditPtr (sInt i) {return ((sU8 *)DataEdit)+i;}
  sU32 *GetEditPtrU(sInt i) {return ((sU32*)DataEdit)+i;}
  sS32 *GetEditPtrS(sInt i) {return ((sS32*)DataEdit)+i;}
  sF32 *GetEditPtrF(sInt i) {return ((sF32*)DataEdit)+i;}

  friend struct KDoc;
};

/****************************************************************************/

struct KDoc
{
  sBool Init(const sU8 *&data);
  void Exit();
  
  sArray<KOp> Ops;
  sArray<sInt> Roots;
  sArray<sInt> RootFlags;
  sArray<sChar *> RootNames;
  sArray<W3Image *> RootImages;

  sBool Calculate(W3ProgressCallback cb);
};

/****************************************************************************/

}

/****************************************************************************/

#endif
