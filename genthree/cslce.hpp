// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef __CSLCE_HPP__
#define __CSLCE_HPP__

#include "_types.hpp"
#include "cslrt.hpp"

/****************************************************************************/
/****************************************************************************/

#define SC_MAXBYTECODE    8192    // max size of bytecode
#define SC_MAXSTRINGCOUNT 1024    
#define SC_MAXSTRINGSIZE  (32*1024)
#define SC_MAXWORKSPACE   (4*1024*1024)
#define SC_MAXCODE        (128*1024)
#define SC_MAXDATA        (64*1024)

#define SC_MAXOPPARA      4096
#define SC_MAXOPSTATEMENT 1024
#define SC_MAXOPFUNCTION  512

/****************************************************************************/

#define TOK_ERROR       0x01
#define TOK_INTLIT      0x02
#define TOK_STRINGLIT   0x03
#define TOK_NAME        0x04
#define TOK_END         0x05
#define TOK_COLORLIT    0x06

#define TOK_OPEN        0x10
#define TOK_CLOSE       0x11
#define TOK_COPEN       0x12
#define TOK_CCLOSE      0x13
#define TOK_SOPEN       0x14
#define TOK_SCLOSE      0x15
#define TOK_DOTDOT      0x16
#define TOK_PLUS        0x17
#define TOK_MINUS       0x18
#define TOK_STAR        0x19
#define TOK_SLASH       0x1a
#define TOK_PERCENT     0x1b
#define TOK_DOT         0x1c
#define TOK_COLON       0x1d
#define TOK_SEMI        0x1e
#define TOK_QUESTION    0x1f
#define TOK_COMMA       0x20

#define TOK_GE          0x21
#define TOK_LE          0x22
#define TOK_GT          0x23
#define TOK_LT          0x24
#define TOK_EQ          0x25
#define TOK_NE          0x26
#define TOK_EQUAL       0x27

#define TOK_CONST       0x30
#define TOK_CLASS       0x31
//#define TOK_OSTACK      0x32
//#define TOK_TO          0x33
//#define TOK_VAR         0x34
#define TOK_IF          0x35
#define TOK_ELSE        0x36
#define TOK_WHILE       0x37
#define TOK_DO          0x38
#define TOK_RETURN      0x39
#define TOK_LOAD        0x3a
#define TOK_STORE       0x3b
#define TOK_DROP        0x3c
#define TOK_AND         0x3d
#define TOK_OR          0x3e
#define TOK_INT         0x3f
#define TOK_INT2        0x40
#define TOK_INT3        0x41
#define TOK_INT4        0x42
#define TOK_OBJECT      0x43
#define TOK_VOID        0x44

#define OP_VAR          0x100
#define OP_CALL         0x102
#define OP_ARG          0x103
#define OP_FINISH       0x104
#define OP_GLUE         0x105
#define OP_COMPOUND     0x106
#define OP_SCALE        0x107
#define OP_F2I          0x108
#define OP_I2F          0x109

/****************************************************************************/
/****************************************************************************/

struct SCType;
struct SCFuncArg;
struct SCFunction;
struct SCStatement;

/****************************************************************************/

struct SCSymbol
{
  SCSymbol *Next;
  sChar *Name;
  sInt Kind;
};

#define SCS_TYPE      1           // symbol kind
#define SCS_VAR       2
#define SCS_FUNC      3
#define SCS_CONST     4

/****************************************************************************/

struct SCObject : public SCSymbol
{
  sU32 CID;
};

/****************************************************************************/

struct SCType : public SCSymbol
{
  sInt Base;
  sInt Count;
  sU32 Cid;
  struct SCFuncArg *Members;
};

#define SCT_VOID    0             // base
#define SCT_INT     1
#define SCT_OBJECT  2
#define SCT_FLOAT   3

/****************************************************************************/

struct SCVar : public SCSymbol
{
  SCType *Type;
  sInt Index;
  sInt Scope;
};

#define SCS_LOCAL   0             // variable scope
#define SCS_GLOBAL  1

/****************************************************************************/

struct SCConst : public SCSymbol
{
  SCType *Type;
  sInt Value[4];
};

/****************************************************************************/

struct SCFuncArg                  // also used for memeber-variables!
{
  SCFuncArg *Next;                // linked list of arguments
  sChar *Name;                    // name in prototype
  SCType *Type;                   // type in prototype
  SCVar *Var;                     // corresponding local var
  sInt Offset;                    // offset to structure (if member not argument)
  

  sInt Default[4];                // default value
  sInt GuiMin[4];                 // minimum value for gui
  sInt GuiMax[4];                 // maximum value for gui
  sChar *GuiSpecial;              // "special" string for gui
  sInt GuiSpeed;                  // mouse speed for gui
  sInt Flags;                     // SCFA_???
};

#define SCFA_DEFAULT      1       // Default valid
#define SCFA_MIN          2       // GuiMin valid
#define SCFA_MAX          4       // GuiMax valid
#define SCFA_SPECIAL      8       // GuiSpecial valid
#define SCFA_FLOAT       16       // use float, not int
#define SCFA_SPEED       32       // GuiSpeed valid

/****************************************************************************/

struct SCFunction : public SCSymbol
{
  sInt Index;
  SCFuncArg *Args;
  SCType *Result;
  SCStatement *Block;             // the code that implements the function
  SCStatement *Clause;            // the complete clause, for marking in source
  SCSymbol *Locals;
  sInt MaxLocal;                  // last local used
  sU32 OStackIn[4];               // OStack input CIDs
  sU32 OStackOut[4];              // OStack output CIDs

  sU32 Shortcut;                  // keyboard shortcut
  sU32 Flags;                     // object input attributes
};

#define SCFU_COPY     0x0001      // input is copied, object may be modified
#define SCFU_LINK     0x0002      // input is linked, do never modified object
#define SCFU_MOD      0x0004      // input is modified
#define SCFU_NOP      0x0008      // input is passed to next stage, do not modify until next stage is done
#define SCFU_COPY2    0x0010      // second and other inputs differ from first
#define SCFU_LINK2    0x0020      // second and other inputs differ from first
#define SCFU_MOD2     0x0040      // second and other inputs differ from first
#define SCFU_NOP2     0x0080      // second and other inputs differ from first
#define SCFU_MULTI    0x0100      // multiple inputs (add operator)
#define SCFU_ROWA     0x0200      // special row #1 
#define SCFU_ROWB     0x0400      // special row #2 
#define SCFU_ROWC     0x0800      // special row #3 
#define SCFU_FXALLOC  0x1000      // allocate new buffer in fxchain. breaks -1

/****************************************************************************/

struct SCStatement
{
  SCStatement *Left;              // left part of tree
  SCStatement *Right;             // right part of tree
  sInt Token;                     // operation for this statement
  sInt Value;                     // integer value, use as needed
  sChar *Source0,*Source1;        // location in source (we ignore the fact that there might be multiple sources...)
  SCType *Type;                   // type of result in expressions
  SCVar *Var;                     // variable, use as needed
};

/****************************************************************************/
/****************************************************************************/

// for the operator view of source

struct SCOpPara
{
  sInt Type;                      // 0=unused, 1=simple int, 2=source
  sInt Count;                     // 1..4 for vectors, never 0
  sChar *Source0,*Source1;        // cursor range
  sU32 *Code0[4];                 // emitted bytecode
  sU32 *Code1[4];
};

struct SCOpStatement
{
  sInt Function;                  // function value (<0 is CODE, >0 is USER)
  sChar *Source0,*Source1;        // cursor range
  sU32 *Code0,*Code1;             // emitted bytecode
  sInt ParaCount;                 // number of parameters
  sInt ParaStart;                 // first parameter in array
};

struct SCOpFunction
{
  sChar *Name;                    // name of the function
  sChar *Source0,*Source1;        // start and end of function in source
  sU32 *Code0,*Code1;             // emitted bytecode
  sInt StatementCount;            // number of statements
  sInt StatementStart;            // first statement in array
};

/****************************************************************************/
/****************************************************************************/

class ScriptCompiler : public sObject
{
  sChar **Strings;
  sChar *StringBuffer;
  sU8 *MemBuffer;
  sU8 *DataBuffer;
  sInt StringCount;
  sInt StringNext;
  sInt MemNext;
  sInt DataNext;
  sU32 *CodeBuffer;
  sU32 *CodePtr;

  sChar *MakeString(sChar *);
  sU8 *Alloc(sInt size);

  SCType      *NewType()      { return (SCType *)      Alloc(sizeof(SCType));      }
  SCConst     *NewConst()     { return (SCConst *)     Alloc(sizeof(SCConst));     }
  SCObject    *NewObject()    { return (SCObject *)    Alloc(sizeof(SCObject));    }
  SCVar       *NewVar()       { return (SCVar *)       Alloc(sizeof(SCVar));       }
  SCFuncArg   *NewFuncArg()   { return (SCFuncArg *)   Alloc(sizeof(SCFuncArg));   }
  SCFunction  *NewFunction()  { return (SCFunction *)  Alloc(sizeof(SCFunction));  }
  SCStatement *NewStatement() { return (SCStatement *) Alloc(sizeof(SCStatement)); }

  void Error(sChar *text);
  void AddToken(sInt tok,sChar *string);
  void ScanX();
  void Scan();
  void Match(sInt tok);
  sChar *MatchName();
  sInt MatchInt();
  void PrintToken();
  sChar *Text;
  sChar *TextStart;
  sInt Token;                     // current token
  sInt TokValue;                  // value (if int)
  sInt TokFloatUsed;              // if TOK_INT, the value used dot notation: 1.0  1.35 0x0000.ffff
  sChar *Token0,*Token1;          // where in source is that token?
  sChar *TokName;
  sChar *Tokens[256];
  sInt IsError;
  sInt Line;
  sChar *File;
  sChar ErrorBuf[256];

  SCType *AddType(sChar *name,sInt type,sInt count=1);
//  void AddObject(sChar *name,sU32 cid);

  void AddSymbol(SCSymbol *sym,SCSymbol **list=0);
  SCSymbol *FindSymbol(sChar *name,SCSymbol **list=0);

  void ConvTypeLeft(SCStatement *st,SCType *type);
  void ConvTypeRight(SCStatement *st,SCType *type);
  void ConvType(SCStatement *&st,SCType *type);

  SCSymbol *Symbols;
  SCFunction *CurrentFunc;
//  sInt NextGlobalI;
//  sInt NextGlobalO;
  sInt NextGlobal;
  sInt NextFunction;
//  sInt NextLocalI;
//  sInt NextLocalO;
  sInt NextLocal;
  SCType *IntTypes[5];            // inttypes[0] = "void", inttypes[4] = "int4"      
  SCType *FloatTypes[5];
  SCType *ObjectType;
  SCType *StringType;

  void _Programm();
//  void _GStatement();
//  void _Global();
  void _Func(SCType *type,sChar *name,SCStatement *clause);
  sU32 ParseModifierString(sChar *scan);
  void _VarList(SCFunction *func);
  SCFuncArg *_VarEntry(sInt &offset);
  void _OStack(SCFunction *func);
  SCStatement *_Block();
  SCStatement *_Statement();
  void _Class();
  void _Arg();
  SCStatement *_Expr();
  SCStatement *_CExpr();
  SCStatement *_BExpr(sInt level);
  SCStatement *_UExpr(); 
  SCType *_Type();
  SCStatement *_Var();
  sInt _ConstExpr(sBool *floatused=0);
  sBool _ConstExprVector(sInt *vector,SCType *type);
  SCStatement *_Call();

  void Emit(sU32 code);
  void PrintArgs(SCFuncArg *arg);
  void PrintVar(SCVar *var,sU32 code,sInt loop);
  void PrintVar(SCStatement *var,sU32 code,sInt loop);
  void PrintStatement(SCStatement *st,sInt loop);
  void Optimise(SCStatement *st);
public:
  ScriptCompiler();
  ~ScriptCompiler();
  void InitOps();
  void Reset();
  sU32 *Compile(sChar *text,sChar *filename);
  sBool Parse(sChar *text,sChar *filename);
  void Optimise();
  sU32 *Generate();
  sInt GetCodeSize() { return (CodePtr-CodeBuffer)*4; }
  sChar *GetErrorPos();
  void PrintParseTrees();

  sChar ErrorText[64];
  sChar ErrorFile[64];
  sInt ErrorLine;
  sInt ErrorSymbolPos;
  sInt ErrorSymbolWidth;

  SCOpPara *OpParas;
  SCOpStatement *OpStatements;
  SCOpFunction *OpFunctions;
  sInt OpParaCount;
  sInt OpStatementCount;
  sInt OpFunctionCount;
  SCSymbol *GetSymbols() { return Symbols; }

// get information about classes and other things

  sChar *GetClassName(sU32 cid);

};


/****************************************************************************/
/****************************************************************************/

#endif
