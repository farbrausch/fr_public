/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) by Dierk Ohlerich                                    ***/
/***   all rights reserved                                                ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_SCRIPT_HPP
#define FILE_WZ4FRLIB_SCRIPT_HPP

#include "base/types2.hpp"
#include "util/scanner.hpp"

/****************************************************************************/

struct ScriptSymbol;
struct ScriptValue;
enum ScriptType
{
  ScriptTypeInt = 1,
  ScriptTypeFloat = 2,
  ScriptTypeString = 3,
  ScriptTypeColor = 4,
};

/****************************************************************************/

struct ScriptSplineKey
{
  sF32 Time;
  sF32 Value;
};

enum ScriptSplineEnum
{
  SSM_Step = 0,
  SSM_Linear,
  SSM_UniformHermite,
  SSM_Hermite,
  SSM_UniformBSpline,

  SSF_BoundMask        = 0x0003,
  SSF_BoundClamp       = 0x0000,
  SSF_BoundWrap        = 0x0001,
  SSF_BoundZero        = 0x0002,
  SSF_BoundExtrapolate = 0x0003,
};

class ScriptSpline                // owned by rendernodes
{
public:
  ScriptSpline();
  ~ScriptSpline();

  sInt Count;
  sInt Mode;
  sInt Flags;
  sInt Bindings;                  // used by wz4fr: how should the camera interpret the spline?
  sF32 MaxTime;
  sInt RotClampMask;              // for interpolating angles in the 0..1 range
  sArray<ScriptSplineKey> **Curves;

  void Init(sInt count);
  void Eval(sF32 time,sF32 *result,sInt count);
  void EvalD(sF32 time,sF32 *result,sInt count);
  sF32 Length();
  sF32 ArcLength(sF32 startTime,sF32 endTime) const;

private:
  static sF64 ArcLengthIntegrand(sF64 x,void *user);
};

/****************************************************************************/

struct ScriptSymbol               // symbols can be bound to values, depending on scope
{
  sPoolString Name;               // name of the symbol
  ScriptValue *Value;             // storage currently bound to the symbol
  sInt CType;                     // compiler type information (0==uninitialized)
  sInt CCount;                    // compiler type information
};

struct ScriptFuncArg
{
  sU8 Type;                       // 1=int 2=float
  sU8 VarCount;                   // 0-> use absolute count. 1..n-> variable count index
  sU8 Count;                      // 0-> use variable count. 1..n-> absolute count
}; 

class ScriptFunc
{
public:
  ScriptFunc();
  ~ScriptFunc();

  sPoolString Name;
  ScriptFuncArg Result;           // result type
  ScriptFuncArg *Args;            // argument string
  sInt ArgCount;
  sInt Primitive;                 // 0-> use code. SC_?? -> use primitive
  sU32 *Code;                     // 0-> use primitive. code string otherwise
};

struct ScriptValue                // a value that might be bound to a symbol
{
  sInt Type;                      // ScriptTypeInt / ...
  sInt Count;                     // 1 = scalar, 1..n = array
  ScriptSymbol *Symbol;           // backlink to symbol
  ScriptValue *ScopeLink;         // linked list of all bindings
  ScriptValue *OldValue;          // value that was linked to the symbol in previous scope
  ScriptFunc *Func;               // this is actually a function..
  ScriptSpline *Spline;           // this is actually a spline..
  union                           // pointer to actual data
  {
    sInt *IntPtr;
    sF32 *FloatPtr;
    sPoolString *StringPtr;
    sU32 *ColorPtr;
  };
};

struct ScriptImport               // late binding with import statement
{
  sPoolString Name;
  sInt Type;
  sInt Count;
  union
  {
    sS32 *IntPtr;
    sF32 *FloatPtr;
    sPoolString *StringPtr;
    sU32 *ColorPtr;
  };
};

class ScriptContext               // a symbol table
{
  sArray<ScriptSymbol *> Symbols; // all symbols
  sArray<ScriptValue *> Scopes;   // first values of the many scopes
  sArray<ScriptFunc *> Funcs;     // all functions
  sArray<ScriptImport *> Imports; // imports
  ScriptValue **Scope;            // current scope
  ScriptValue *Locals;            // first local
  sMemoryPool *Pool;

  sU32 *Code;
  sString<1024> ErrorMsg;
  sArray<ScriptValue *> TVA;      // temporary array for values

public:
  ScriptContext();
  ~ScriptContext();

  ScriptSymbol *AddSymbol(sPoolString name);          // register a symbol. eternal lifetime
  void UpdateCType();                                 // update compiler type information
  ScriptValue *MakeValue(sInt type,sInt dim);         // create a value. execute lifetime
  ScriptValue *MakeValue(ScriptFunc *func);
  ScriptValue *MakeValue(ScriptSpline *spline);
  ScriptValue *CopyValue(ScriptValue *src);
  void BindLocal(ScriptSymbol *,ScriptValue *);       // bind a value to a symbol
  void BindGlobal(ScriptSymbol *,ScriptValue *);      // bind a value to a symbol
  ScriptFunc *AddFunc(sPoolString name,const sChar *proto);    // proto = L"i3:if3ff3" -> int3 func(int float3 float float3) 

  void ClearImports();
  void AddImport(sPoolString name,sInt type,sInt count,void *ptr);
  ScriptImport *FindImport(sPoolString name);

  void PushGlobal();              // create new scope for globals
  void PopGlobal();               // pop scope for globals
  void FlushLocal();              // locals are a special scope
  void ProtectGlobal();           // create a new copy of all globals in current scope. this makes changes to globals local.

  void BeginExe();                // at beginning of a group of executions, delete all values and unbind all symbols
  void EndExe();                  // mostly does some checking

  void Compile(const sChar *);    // new interface: compile into scriptbuffer
  const sChar *Run();             // and run later. could also use class ScriptCode

  // helpers

  ScriptValue *MakeFloat(sInt dim);
  ScriptValue *MakeInt(sInt dim);
  ScriptValue *MakeString(sInt dim);
  ScriptValue *MakeColor(sInt dim);
  void BindLocalFloat(ScriptSymbol *sym,sInt dim,sF32 *ptr);
  void BindLocalInt(ScriptSymbol *sym,sInt dim,sInt *ptr);
  void BindLocalColor(ScriptSymbol *sym,sU32 *ptr);
  ScriptValue *GetFirstFromScope() { if(Scope) return *Scope; else return 0; }
//  void UnbindLocalColor(ScriptSymbol *sym,sU32 *ptr);
};

/****************************************************************************/

class ScriptCode
{
  sU32 *Code;
  sChar *Source;
  sInt SourceLine;
  sBool Exe(ScriptContext *ctx);
public:
  ScriptCode(const sChar *txt,sInt line=0);
  ~ScriptCode();

  sBool Execute(ScriptContext *ctx);
  sString<1024> ErrorMsg;
};

sBool ScriptExecute(const sU32 *Data,ScriptContext *ctx,sString<1024> &ErrorMsg);

/****************************************************************************/

class ScriptCompiler
{
  struct Expression
  {
    sInt Kind;
    sInt Type;
    sInt Count;
    sInt Range0;
    sInt Range1;
    Expression *a,*b,*c,*d;
    union
    {
      sU32 *Literal;
      sInt *LiteralInt;
      sF32 *LiteralFloat;
      sU32 *LiteralColor;
      sPoolString *LiteralString;
      ScriptSymbol *Symbol;
      sU32 *Code;
    };
  };
  struct Statement
  {
    sInt Kind;
    Statement *Next;
    Statement *Child;
    Statement *Alt;
    Expression *Expr;
  };

  sScanner Scan;
  ScriptContext *ctx;
  sMemoryPool *Pool;
  sU32 *Code;
  sInt Index;

  void InitScanner();
  Statement *NewStat(sInt code);
  Expression *NewExpr(sInt code,Expression *a=0,Expression *b=0,Expression *c=0,sInt lit=0);
  Expression *NewExprCT(sInt code,Expression *a=0,Expression *b=0,Expression *c=0,sInt lit=0);
  void PromoteType(Expression **);
  void PromoteCount(Expression **,sInt count);
  void CheckFunc(Expression *expr,ScriptFunc *func);

  void _Range(sInt &r0,sInt &r1,Expression *&ei);
  Expression *_Value();
  Expression *_ExprR(sInt level);
  Expression *_Expr();
  Expression *_Name(sInt cmd);

  Statement *_VarDef(sInt mode);    // 0=local, 1=global, 2=import
  Statement *_Assign();
  Expression *_AssignTo(Expression *a);
  Expression *ScriptCompiler::_AssignTo(Expression *a,Expression *b);      // b = a
  Statement *_If();
  Statement *_Do();
  Statement *_While();
  Statement *_Block();
  Statement *_Statement();
  Statement *_Global();

  void PrintFunc(const sChar *name,Expression *a,Expression *b=0);
  void PrintFunc2(const sChar *name,Expression *e);
  void PrintType(sInt type,sInt count);
  void PrintStat(Statement *,sInt indent);
  void PrintExpr(Expression *);

  void ConstFold(Expression *);
  void ConstFold(Statement *);

  void PatchCmd(sInt cmdindex,sInt target);
  void OutputStat(Statement *);
  void OutputExpr(Expression *);

public:
  ScriptCompiler();
  ~ScriptCompiler();

  sU32 *Parse(ScriptContext *ctx,const sChar *src,sInt line);
  sString<1024> ErrorMsg;
};

/****************************************************************************/

#endif // FILE_WZ4FRLIB_SCRIPT_HPP

