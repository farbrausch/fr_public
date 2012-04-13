// This file is distributed under a BSD license. See LICENSE.txt for details.

#pragma once

#include "_types.hpp"

#if !sLINK_KKRIEGER || !sPLAYER

/****************************************************************************/
/****************************************************************************/

#define sST_SCALAR  1             // full types
#define sST_VECTOR  2
#define sST_MATRIX  3
#define sST_OBJECT  4

#define sST_STRING  5             // internal types
#define sST_CODE    6
#define sST_ERROR   7

#define sSCRIPTNAME 32

enum KBytecode2
{

// generic commands for all four types...

  // variables. dont change order because of TOK_ASSIGN!
  KB_LOADS=0,                     // offset ( - scalar ) 
  KB_LOADV,                       // offset ( - vector )
  KB_LOADM,                       // offset ( - matrix )
  KB_LOADO,                       // offset ( - object )
  KB_ELOADS,                      // extern offset ( - scalar ) 
  KB_ELOADV,                      // extern offset ( - vector )
  KB_ELOADM,                      // extern offset ( - matrix )
  KB_ELOADO,                      // extern offset ( - object )
  KB_STORES,                      // offset ( scalar - )
  KB_STOREV,                      // offset ( vector - )
  KB_STOREM,                      // offset ( matrix - )
  KB_STOREO,                      // offset ( object - )
  KB_ESTORES,                     // extern offset ( scalar - )
  KB_ESTOREV,                     // extern offset ( vector - )
  KB_ESTOREM,                     // extern offset ( matrix - )
  KB_ESTOREO,                     // extern offset ( object - )
  // arg
  KB_ALOADS,                      // offset ( - scalar ) 
  KB_ALOADV,                      // offset ( - vector )
  KB_ALOADM,                      // offset ( - matrix )
  KB_ALOADO,                      // offset ( - object )
  // stack. scalar and spline do the internally the same
  KB_DUPS,                        // ( scalar - scalar scalar ) 
  KB_DUPV,                        // ( vector - vector vector )
  KB_DUPM,                        // ( matrix - matrix matrix )
  KB_DUPO,                        // ( object - object object )
  KB_DROPS,                       // ( scalar - )
  KB_DROPV,                       // ( vector - )
  KB_DROPM,                       // ( matrix - )
  KB_DROPO,                       // ( spline - )
  // conditional
  KB_CONDS,                       // ( scalar scalar scalar - scalar )
  KB_CONDV,                       // ( scalar vector vector - vector )
  KB_CONDM,                       // ( scalar matrix matrix - matrix )
  KB_CONDO,                       // ( scalar object object - object )
  // return from subroutine
  KB_RETS,                        // ( scalar -> scalar )
  KB_RETV,                        // ( vector -> scalar )
  KB_RETM,                        // ( matrix -> scalar )
  KB_RETO,                        // ( spline -> scalar )
  // swizzle
  KB_SELECTX,                     // ( vector - scalar )
  KB_SELECTY,                     // ( vector - scalar )
  KB_SELECTZ,                     // ( vector - scalar )
  KB_SELECTW,                     // ( vector - scalar )
  KB_SELECTI,                     // ( matrix - vector )
  KB_SELECTJ,                     // ( matrix - vector )
  KB_SELECTK,                     // ( matrix - vector )
  KB_SELECTL,                     // ( matrix - vector )
  // literals
  KB_LITS,                        // data ( - scalar )
  KB_LITV,                        // data ( - vector )
  KB_LITM,                        // data ( - matrix )  [this does not work]
  KB_LITO,                        // data ( - spline )  [this does not work]

// special functions

  // combine. actually, these are nops...
  KB_COMBV,                       // ( scalar scalar scalar scalar - vector )
  KB_COMBM,                       // ( vector vector vector vector - matrix )

// arithmetic

  // scalar
  KB_ADDS,                        // ( scalar scalar - scalar )
  KB_SUBS,                        // ( scalar scalar - scalar )
  KB_MULS,                        // ( scalar scalar - scalar )
  KB_DIVS,                        // ( scalar scalar - scalar )
  KB_MODS,                        // ( scalar scalar - scalar )
  // vector
  KB_ADDV,                        // ( vector vector - vector )
  KB_SUBV,                        // ( vector vector - vector )
  KB_MULV,                        // ( vector vector - vector )
  KB_CROSS,                       // ( vector vector - vector )
  // matrix
  KB_MULM,                        // ( matrix matrix - matrix )
  // special
  KB_NEG,                         // ( scalar - scalar )
  KB_SIN,                         // ( scalar - scalar ) [full circle is 0..1]
  KB_COS,                         // ( scalar - scalar ) [full circle is 0..1]
  KB_RAMP,                        // ( scalar - scalar )
  KB_DP3,                         // ( vector vector - scalar )
  KB_DP4,                         // ( vector vector - scalar )
  // more special
  KB_EULER,                       // ( vector - matrix)
  KB_LOOKAT,                      // ( vector - matrix)
  KB_AXIS,                        // ( vector - matrix)
  KB_QUART,                       // ( vector - matrix)
  KB_SCALE,                       // ( vector - matrix)

// structure

  // compare and logic
  KB_EQ,                          // ( scalar - scalar ) [true if zero]
  KB_NE,                          // ( scalar - scalar ) [true if not zero]
  KB_GT,                          // ( scalar - scalar ) [true if greater than zero]
  KB_GE,                          // ( scalar - scalar ) [true if greater or equal zero]
  // jumps
  KB_JUMP,                        // position
  KB_JUMPT,                       // position (scalar - )
  KB_JUMPF,                       // position (scalar - )
  // calls
  KB_CALL,                        // position argcount ( ??? - ??? )
  KB_OBJECT,                      // position argcount ( ??? object - ??? )
  KB_RET,                         // ( -- )

// stuff...

  KB_PRINT,                       // text ( -- )
  KB_PRINTS,                      // ( scalar - )
  KB_PRINTV,                      // ( vector - )
  KB_PRINTM,                      // ( matrix - )
  KB_PRINTO,                      // ( spline - )

// end
 
  KB_MAX,

// special code for codegeneration

  KB_NEXT = 0x100,
  KB_ERROR,
  KB_STRING,
  KB_LOADSYM,
  KB_STORESYM,
};

/****************************************************************************/
/****************************************************************************/

struct sScript
{
public:
  void Init();
  void Exit();

  sF32 *Var;
  sInt VarCount;
  sU8 *Code;
  sInt CodeCount;
  sInt ErrorCount;
};

/****************************************************************************/

class sScriptVM
{
  sF32 Stack[1024];
  sInt Index;

  sU8 *ip;
  sInt GetByte()                { return *ip++; }
  sInt GetOffset();

  sChar *OutBuffer;
  sInt OutAlloc;
  sInt OutUsed;
  void Print(const sChar *);

public:
  sScriptVM();
  ~sScriptVM();

  void Push(const sF32 *p,sInt count) { while(count--) Stack[Index++]=*p++; }
  void Pop(sF32 *p,sInt count)  { Index-=count; for(sInt i=0;i<count;i++) *p++ = Stack[Index+i]; }

  void PushS(sF32 s)            { Stack[Index++] = s; }
  void PushV(const sVector &v)  { Push(&v.x,4); }
  void PushM(const sMatrix &m)  { Push(&m.i.x,16); }
  void PushO(void *o)           { ((void **)Stack)[Index++] = o; }

  sF32 PopS()                   { return Stack[--Index]; }
  void PopV(sVector &v)         { Pop(&v.x,4); }
  void PopM(sMatrix &m)         { Pop(&m.i.x,16); }
  void *PopO()                  { return ((void **)Stack)[--Index]; }

  sBool Execute(sScript *,sInt offset=0);
  sChar *GetOutput()            { return OutBuffer; }

  virtual sBool Extern(sInt id,sInt offset,sInt type,sF32 *data,sBool store);
  virtual sBool UseObject(sInt id,void *object);
};

/****************************************************************************/

struct sScriptSymbol
{
  sChar Name[sSCRIPTNAME];
  sInt Type;
  sInt Flags;
  sInt Offset;
  sInt Extern;
};

#define sSS_EXTERN  0x0001
#define sSS_CONST   0x0002

struct sScriptNode
{
  sInt Code;
  sInt Type;
  sScriptNode *Left;
  sScriptNode *Right;
  sF32 Val[4];                    // KB_LIT
  sU32 Select;                    // KB_LOADSYM / KB_STORESYM
  sScriptSymbol *Symbol;          // KB_LOADSYM / KB_STORESYM
  sScriptNode *BackPatch;         // copy after-ip to this node
  sScriptNode *ForePatch;         // get front-ip from this node
  sInt CodeOffset;                // ip of instruction
};

class sScriptCompiler
{
  // scanner

  const sChar *ScanPtr;
  sInt Token;
  sF32 Value;
  sChar Name[sSCRIPTNAME+1];
  sChar String[4096];

  sInt Scan();
  sBool Match(sInt token);

  // error messages

  sText *Errors;
  sInt ErrorCount;
  sInt ErrorLine;
  sInt Line;

  void Error(const sChar *error,...);

  // node factory

  sScriptNode *Nodes;
  sInt NodeAlloc;
  sInt NodeUsed;

  sScriptNode *GetNode(sInt code,sInt type);

  // code buffer

  sU8 *Code;
  sInt CodeAlloc;
  sInt CodeUsed;

  void EmitByte(sInt code);
  void EmitOffset(sU32 offset);
  void EmitOffset2(sInt offset);
  void EmitOffset2(sInt offset,sU8 *ptr);
  void EmitFloat(sF32 val);
  void EmitString(const sChar *string);

  // variable offset counter

  sInt VarUsed;
  
  // parser

  sScriptNode *_Block();
  sScriptNode *_Statement();
  sScriptNode *_Expression();
  sScriptNode *_Sum();
  sScriptNode *_Term();
  sScriptNode *_Compare();
  sScriptNode *_Value();
  sScriptNode *_Intrinsic(const struct Intrinsic *intr);
  sScriptNode *_If();
  sScriptNode *_While();
  sScriptNode *_DoWhile();
  void _Define(sInt type,sInt size,sInt flags);

  // code generator

  void Generate(sScriptNode *node);
  void GenStatement(sScriptNode *node);

public:
  sScriptCompiler();
  ~sScriptCompiler();

  // compiler

  sBool Compiler(sScript *,const sChar *source,sText *Errors);

  // diagnostics

  void PrintToken();
  sBool Disassemble(sScript *script,sText *out,sBool verbose=1);
  void PrintSymbols();

  // symbol table

  sArray<sScriptSymbol> Locals;                   // symbols defined by outside for use in this source
  sArray<sScriptSymbol> Symbols;                  // symbols defined by this source
  sScriptSymbol *FindSymbol(const sChar *name);
  virtual sBool ExternSymbol(sChar *group,sChar *name,sU32 &groupid,sU32 &offset);
  void ClearLocals();
  void AddLocal(const sChar *name,sInt type,sInt flags,sInt global,sInt offset);
};

/****************************************************************************/
/****************************************************************************/

#endif
