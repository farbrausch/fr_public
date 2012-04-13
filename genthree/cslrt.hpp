// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef __CSLRT_HPP__
#define __CSLRT_HPP__

#include "_types.hpp"

/****************************************************************************/
/****************************************************************************/

typedef sBool (* ScriptHandler )(class ScriptRuntime *);

extern sChar *ScriptRuntimeError;
extern class ScriptRuntime *ScriptRuntimeInterpreter;
#if !sINTRO
#define SCRIPTSTRINGIZE2(x)  #x
#define SCRIPTSTRINGIZE(x) SCRIPTSTRINGIZE2(x)
#define SCRIPTVERIFY(x) { if(!(x)) { if(!ScriptRuntimeError) ScriptRuntimeError = "Verify " __FILE__ "(" SCRIPTSTRINGIZE(__LINE__) ")";  return 0; } }
#define SCRIPTVERIFYVOID(x) { if(!(x)) { if(!ScriptRuntimeError) ScriptRuntimeError = "Verify " __FILE__ "(" SCRIPTSTRINGIZE(__LINE__) ")"; return; } }
#else
#define SCRIPTVERIFY(x)
#define SCRIPTVERIFYVOID(x)
#endif
#define SCRIPTCALL __stdcall

#define RT_MAXGLOBAL  2048        // max global variable
#define RT_MAXLOCAL   512        // max local variables per call
#define RT_MAXCALLS   16          // max call-depth
#define RT_MAXUSER    1024        // max user words
#define RT_MAXCODE    512         // max code words
#define RT_MAXSTACK   256         // max stack size


struct ScriptRuntimeCode
{
  void *Code;                     // Code ptr, stdcall convention
  sU8 Ints;                       // number of int arguments
  sU8 Objects;                    // number of object arguments
  sU8 Return;                     // 0 = void, 1 = int, 2 = object
  sU8 pad;                        // 0
};

class ScriptRuntime : public sObject
{
  sInt *XVar;                     // local and global integer variables           
  sInt XVarIndex;
//  sObject **OVar;                 // local and global object variables
//  sInt OVarIndex;
  sInt IStack[RT_MAXSTACK];       // integer stack
  sInt IIndex;
  sObject *OStack[RT_MAXSTACK];   // object stack
  sInt OIndex;
  sInt RStack[RT_MAXCALLS];       // return stack
  sInt RIndex;

  sU32 *Bytecode;                 // bytecode. first word is unused
  sU32 *BrokerRoots;
  sInt BytecodeSize;              // size of bytecode in words
  sChar *BytecodeStrings;
  sInt *UserWords;                // start of subroutine indices. 0 = unused
//  ScriptHandler *CodeWords;       // scripthandlers
  ScriptRuntimeCode *CodeCalls;   // direct called code-words

  sInt Error;                     // identify error condition
  sInt PC;                        // programm counter
  void Interpret();               // the interpreter core

#if !sINTRO || !sRELEASE
  sChar *ErrorMsg;                // error message for execution errors
#endif
public:
  ScriptRuntime();                // init
  ~ScriptRuntime();               // exit
  void Tag();                     // garbage collector tagging
  void Load(sU32 *code);          // load code and initialize 
  void AddCode(sInt word,ScriptHandler);
  void AddCode(sInt word,sU32 flags,void *code);
  sBool Execute(sInt word);

  sU8* PackedExport(sInt &size);
  void PackedImport(sU8 *);

#if !sINTRO
  void PushI(sInt);               // push integer to stack
  void PushO(sObject *);          // push object to stack
  sInt PopI();                    // pop integer from stack
  sF32 PopX();                    // pop fixed-point from stack
  sChar *PopS();                  // pop string
  sObject *PopO();                // pop object from stack
  sObject *PopO(sU32 cid);        // pop object of specified type from stack
#else
  __forceinline void PushI(sInt v)        { IStack[++IIndex] = v; }
  __forceinline void PushO(sObject *o)    { OStack[++OIndex] = o; }
  __forceinline sInt PopI()               { return IStack[IIndex--]; }
  __forceinline sF32 PopX()               { return IStack[IIndex--]/65536.0f; }
  __forceinline sChar *PopS()             { return (sChar *) IStack[IIndex--]; }
  __forceinline sObject *PopO()           { return OStack[OIndex--]; }
  __forceinline sObject *PopO(sU32 cid)   { return OStack[OIndex--]; }
#endif
/*
  void PushIV(sInt *v,sInt count);// push multiple integers on stack
  void PopIV(sInt *v,sInt count); // pop multiple integers from stack
  void PopXV(sF32 *v,sInt count); // pop multiple fixed-points from stack
  void PopOV(sObject **v,sInt count); // pop multiple integers from stack
  */
  __forceinline sInt GetGlobal(sInt nr);        // read global var
  __forceinline void SetGlobal(sInt nr,sInt v); // write global var
  __forceinline sObject *GetGlobalObject(sInt nr);       // read global var
  __forceinline void SetGlobalObject(sInt nr,sObject *); // write global var
  sU32 *GetUserWord(sInt nr)      { return (nr>=0 && nr<RT_MAXUSER) ? Bytecode+UserWords[nr] : 0; }

#if !sINTRO
  sChar *GetErrorMessage()        { return ErrorMsg; }
#endif

  class GenPlayer *Player;        // hmmm, this is kinda GenPlayer specific. 
  sInt ShowProgress;              // show progressbar
};


__forceinline sInt ScriptRuntime::GetGlobal(sInt nr)
{
  sVERIFY(nr>=0 && nr<RT_MAXGLOBAL);
  return XVar[nr];
}

__forceinline void ScriptRuntime::SetGlobal(sInt nr,sInt v)
{
  sVERIFY(nr>=0 && nr<RT_MAXGLOBAL);
  XVar[nr] = v;
}

__forceinline sObject *ScriptRuntime::GetGlobalObject(sInt nr)
{
  sVERIFY(nr>=0 && nr<RT_MAXGLOBAL);
  return (sObject *)XVar[nr];
}

__forceinline void ScriptRuntime::SetGlobalObject(sInt nr,sObject *v)
{
  sVERIFY(nr>=0 && nr<RT_MAXGLOBAL);
  XVar[nr] = (sInt)v;
}

#define RTC_NOP       0x00000000
#define RTC_EOF       0x01000000
#define RTC_RTS       0x02000000
#define RTC_I31       0x03000000
#define RTC_ADD       0x04000000
#define RTC_SUB       0x05000000
#define RTC_MUL       0x06000000
#define RTC_DIV       0x07000000
#define RTC_MOD       0x08000000
#define RTC_MIN       0x09000000
#define RTC_MAX       0x0a000000
#define RTC_AND       0x0b000000
#define RTC_OR        0x0c000000
#define RTC_SIN       0x0d000000
#define RTC_COS       0x0e000000
#define RTC_GT        0x0f000000
#define RTC_GE        0x10000000
#define RTC_LT        0x12000000
#define RTC_LE        0x13000000
#define RTC_EQ        0x14000000
#define RTC_NE        0x15000000
#define RTC_DUP       0x16000000
#define RTC_F2I       0x17000000
#define RTC_I2F       0x18000000

#define RTC_CODE      0x20000000  // call a code-word
#define RTC_USER      0x21000000  // call a user-word
#define RTC_FUNC      0x22000000  // define a code-word
#define RTC_STRING    0x23000000  // push string address
#define RTC_IF        0x24000000
#define RTC_THEN      0x25000000
#define RTC_ELSE      0x26000000
#define RTC_DO        0x27000000
#define RTC_WHILE     0x28000000
#define RTC_REPEAT    0x29000000

#define RTC_STORE     0x30000000
#define RTC_FETCH     0x32000000

#define RTC_STORELOCI 0x30000000  // store variable
#define RTC_STORELOCO 0x31000000  // store variable
#define RTC_FETCHLOCI 0x32000000  // load variable
#define RTC_FETCHLOCO 0x33000000  // load variable
#define RTC_STOREGLOI 0x34000000  // store variable
#define RTC_STOREGLOO 0x35000000  // store variable
#define RTC_FETCHGLOI 0x36000000  // load variable
#define RTC_FETCHGLOO 0x37000000  // load variable
#define RTC_STOREMEMI 0x38000000  // store objects member variable
#define RTC_STOREMEMO 0x39000000  // store objects member variable
#define RTC_FETCHMEMI 0x3a000000  // load objecs member variable
#define RTC_FETCHMEMO 0x3b000000  // load objecs member variable

//#define RTC_WORDMASK  0xffff0000
#define RTC_CMDMASK  0xff000000
#define RTC_IMMMASK   0x80000000
//#define RTC_VARMASK   0x00000100
//#define RTC_SCOPEMASK 0x00000600
/*
#define RTCVAR_OBJECT     0x0000
#define RTCVAR_INT        0x0100
#define RTCSCOPE_GLOBAL   0x0000
#define RTCSCOPE_LOCAL    0x0200

*/
/****************************************************************************/
/****************************************************************************/

#endif
