/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/**************************************************************************+*/

#ifndef FILE_ASC_ASC_DOC_HPP
#define FILE_ASC_ASC_DOC_HPP

#include "base/types2.hpp"
#include "util/scanner.hpp"

/****************************************************************************/

enum ACTokens
{
  AC_TOK_LINE = 0x100,
  AC_TOK_ASSIGNADD,
  AC_TOK_ASSIGNSUB,
  AC_TOK_ASSIGNMUL,
  AC_TOK_ASSIGNDIV,
  AC_TOK_ANDAND,
  AC_TOK_OROR,
  AC_TOK_INC,
  AC_TOK_DEC,
  AC_TOK_TRUE,
  AC_TOK_FALSE,
  AC_TOK_IMPLIES,

  AC_TOK_IF,
  AC_TOK_ELSE,
  AC_TOK_FOR,
  AC_TOK_WHILE,
  AC_TOK_DO,
  AC_TOK_RETURN,
  AC_TOK_BREAK,
  AC_TOK_CONTINUE,
  AC_TOK_STRUCT,
  AC_TOK_CBUFFER,
  AC_TOK_PERMUTE,
  AC_TOK_USE,
  AC_TOK_EXTERN,
  AC_TOK_PIF,
  AC_TOK_PELSE,
  AC_TOK_ASSERT,
  AC_TOK_REGISTER,
  AC_TOK_ASM,
  AC_TOK_PRAGMA,

  AC_TOK_IN,
  AC_TOK_OUT,
  AC_TOK_INOUT,
  AC_TOK_UNIFORM,
  AC_TOK_ROWMAJOR,
  AC_TOK_COLMAJOR,
  AC_TOK_CONST,
  AC_TOK_INLINE,
  AC_TOK_STATIC,
  AC_TOK_GROUPSHARED,
};

enum ACTypeType
{
  ACT_VOID = 1,
  ACT_INT,
  ACT_UINT,
  ACT_BOOL,
  ACT_FLOAT,
  ACT_DOUBLE,
  ACT_HALF,
  ACT_STRUCT,
  ACT_ARRAY,
  ACT_FUNC,
  ACT_SAMPLER,
  ACT_TEX,
  ACT_SAMPLERSTATE,
  ACT_CBUFFER,
  ACT_STREAM,
};

enum ACTypeUsages
{
  ACF_IN       = 0x0001,
  ACF_OUT      = 0x0002,
  ACF_INOUT    = 0x0003,
  ACF_UNIFORM  = 0x0004,
  ACF_ROWMAJOR = 0x0008,          // column_major is default
  ACF_COLMAJOR = 0x0010,          // column_major was explicitly states
  ACF_CONST    = 0x0020,
  ACF_INLINE   = 0x0040,
  ACF_STATIC   = 0x0080,
  
  ACF_POINT    = 0x0100,
  ACF_LINE     = 0x0200,
  ACF_TRIANGLE = 0x0400,
  ACF_LINEADJ  = 0x0800,
  ACF_TRIANGLEADJ = 0x1000,
  ACF_GROUPSHARED = 0x2000,
};

class ACType                      // a variable type
{
public:
  ACType();
  ~ACType();

  sPoolString Name;               // name of the type (may be nameless)
  sInt Type;                      // ACT_??? : int, float, struct, array

  sInt Rows;                      // int or float: rows is number of components (xyzw)
  sInt Columns;                   // int or float: columns is number of registers ( column major default)
  ACType *BaseType;               // struct or array: link to base type
  struct ACExpression *ArrayCountExpr;
  sInt ArrayCount;                // array: count
  sArray<class ACVar *> Members;  // struct: list of members
  sArray<class ACExtern *> Externs;  // cbuffer: code fragments 
  sInt CRegStart,CRegCount;       // constant buffer range, when using auto-constants
  sInt CSlot;                     // constant buffer slot
  ACType *Template;               // one template parameter <x>
  sInt TemplateCount;             // second template parameter <x,n>
  sInt Temp;

  void SizeOf(sInt &columns,sInt &rows);
};

class ACVar                       // a variable (local to a function)
{
public:
  ACVar();
  ~ACVar();
  void CopyFrom(ACVar *);

  sPoolString Name;
  class ACType *Type;
  sPoolString Semantic;           // "POSITION", "TEXCOORD1"
  sInt RegisterType;              // 0: no explicit register binding, 'c' or 's' otherwise
  sInt RegisterNum;
  sInt Usages;
  sBool Function;                 // this is actually an ACFunc
  struct ACExpression *Permute;          // permutation dependent variable declarations

};

class ACExtern
{
public:
  ACExtern();
  sPoolString Result;
  sPoolString Name;
  sPoolString Para;
  sPoolString Body;
  const sChar *File;
  sInt ParaLine;
  sInt BodyLine;
};

struct ACPermuteMember
{
  sPoolString Name;
  sInt Value;             // value.
  sInt Max;               // group: max value (inclusive)
  sInt Mask;              // group only: mask to apply (after shift)
  sInt Shift;             // shift amount, for creating symbols
  sBool Group;            // 1:group, 0:element in group
};

class ACPermute
{
public:
  sPoolString Name;
  sInt MaxShift;
  sArray<ACPermuteMember> Members;
  sArray<struct ACExpression *> Asserts;
};

#define SCOPE_MAX 16
#define SCOPE_GLOBAL 0
#define SCOPE_PARA 1

class ACFunc : public ACVar
{
public:
  ACFunc();
  ~ACFunc();
  struct ACStatement *FuncBody;   // zero if intrinsic, otherwise at least ACS_EMPTY
  sArray<class ACVar *> Locals[SCOPE_MAX];   // global, para, local variables;
};

enum ACExpressionOpcode
{
  ACE_NOP = 0,
  ACE_ADD,  ACE_SUB,  ACE_MUL,  ACE_DIV,  ACE_MOD,  ACE_DOT,
  ACE_ASSIGN,  ACE_ASSIGN_ADD,  ACE_ASSIGN_SUB,  ACE_ASSIGN_MUL,  ACE_ASSIGN_DIV,
  ACE_AND,  ACE_OR,  ACE_COND1,  ACE_COND2,
  ACE_CAST,
  ACE_GT,  ACE_LT,  ACE_EQ,  ACE_NE,  ACE_GE,  ACE_LE,
  ACE_PREINC,  ACE_PREDEC,  ACE_POSTINC,  ACE_POSTDEC,
  ACE_MEMBER,
  ACE_NOT,  ACE_POSITIVE,  ACE_NEGATIVE, ACE_COMPLEMENT,

  ACE_BAND, ACE_BOR, ACE_BXOR, ACE_SHIFTL, ACE_SHIFTR, 

  ACE_TRUE,
  ACE_FALSE,
  ACE_CALL,
  ACE_VAR,
  ACE_LITERAL,
  ACE_INDEX,
  ACE_COMMA,
  ACE_CONSTRUCT,     // for building parameter lists. comma operator not supported
  ACE_PERMUTE,
  ACE_IMPLIES,        // converted int !a||b immediatly while parsing
  ACE_RENDER,         // pif(RENDER_DX9)
};

struct ACLiteral
{
  ACLiteral *Next;
  ACLiteral *Child;
  ACExpression *Expr;             // sometimes, literals are expressions.. used in {}
  sPoolString Value;              // lexical value, for exact float reproduction
  sF32 ValueF;
  sInt ValueI;
  sInt Token;                     // sTOK_INT, sTOK_FLOAT
};

struct ACExpression
{
  ACExpression *Left;
  ACExpression *Right;
  sInt Op;

  ACType *CastType;               // ACE_CAST
  ACVar *Variable;                // ACE_VAR, ACE_CALL
  sPoolString Member;             // ACE_MEMBER  (dot operator, not dot product!)
  ACPermuteMember *Permute;       // ACE_PERMUTE
  ACLiteral *Literal;             // literal
};

enum ACStatementOpcode
{
  ACS_EMPTY = 1,
  ACS_WHILE,                      // while(COND) BODY;
  ACS_FOR,                        // for(ELSE;COND;EXPR) BODY;
  ACS_IF,                         // if(COND) BODY; else ELSE;
  ACS_EXPRESSION,                 // Expr;
  ACS_RETURN,                     // return EXPR;
  ACS_BREAK,                      // break;
  ACS_CONTINUE,                   // continue;
  ACS_DO,                         // do BODY while(COND);
  ACS_BLOCK,                      // { BODY }
  ACS_TYPEDECL,                   // typedef TYPE;
  ACS_VARDECL,                    // VAR = EXPR;
  ACS_PIF,                        // pif(COND) BODY; pelse ELSE;
  ACS_ASM,                        // asm { ... }
  ACS_ATTRIBUTE,                  // [ ... ]
};

struct ACStatement
{
public:
  ACStatement *Next;              // link to next statement in block
  sInt Op;                        // ACS_???
  ACStatement *Body;              // all flow control
  ACStatement *Else;              // else body (if any), for-init
  ACExpression *Cond;             // for-cond, if, while
  ACExpression *Expr;             // expr, return, for-step
  ACType *Type;                   // ACS_TYPEDECL
  ACVar *Var;                     // ACS_VARDECL
  sInt SourceLine;
  const sChar *SourceFile;
};

enum ACGlobalType
{
  ACG_FUNC = 1,
  ACG_VARIABLE,
  ACG_TYPEDEF,
  ACG_USE,
  ACG_ATTRIBUTE,
  ACG_PRAGMA,
};

struct ACGlobal
{
  sInt Kind;
  ACVar *Var;
  ACType *Type;
  ACExpression *Init;
  ACExpression *Permute;          // ACG_ATTRIBUTE, ACG_PRAGMA
  sPoolString Attribute;          // ACG_ATTRIBUTE, ACG_PRAGMA
  sInt SourceLine;
  const sChar *SourceFile;
};

/****************************************************************************/

class ACDoc
{
  // input and output interface

  sMemoryPool *Pool;              // pool for ACStatement and ACExpression
  sScanner Scan;
  sInt CurrentLine;
  const sChar *CurrentFile;
  void Print(const sChar *str);
  void PrintLine(sInt line,const sChar *file);
  sPRINTING0(PrintF, sString<1024> tmp; sFormatStringBuffer buf=sFormatStringBase(tmp,format);buf,Print((const sChar*)tmp););

  // default types and functions. initialized only once!

  sArray<ACType *> DefaultTypes;  // predefined types
  sArray<ACFunc *> Intrinsics;    // predifined functions  

  // environment, persists between parse calls

public:
  sArray<ACType *> UserTypes;     // user defined types. usually cbuffers and structs
  sArray<ACPermute *> Permutes;   // pemutation declarators

  // parsed program

  sArray<ACFunc *> Functions;     // user defined functions
  sArray<ACVar *> Variables;      // global variables
  sArray<ACVar *> AllVariables;   // for memory management
  sArray<ACGlobal> Program;       // all global items
  ACFunc *Function;               // current function
  sInt Scope;                     // current variable scope level
  ACPermute *UsePermute;          // "use permute" clause

  // small helpers
private:
  ACType *FindType(sPoolString);  // search UserTypes and DefaultTypes
  ACFunc *FindFunc(sPoolString);  // search Functions and Intrinsics
  ACVar *FindVar(sPoolString);    // search local and global variables

  void AddDefTypes(const sChar *name,sInt type,sBool many);
  void AddDefType(const sChar *name,sInt type,sInt row,sInt col);
  void AddDefFunc(const sChar *name);
  ACExpression *NewExpr(sInt op,ACExpression *a,ACExpression *b);
  ACStatement *NewStat();
  sInt ConstFoldInt(ACExpression *expr);

  // parser

  void _Global();
  void _Line();
  sBool _IsDecl();
  ACVar *_Decl();
  ACVar *_DeclPost(ACType *type,sInt usages);
  void _Register(sInt &regtype,sInt &regnum);
  ACStatement *_Block();          // scan one or more statement, open new scope
  ACType *_Struct(sInt type);     // ACT_STRUCT or ACT_CBUFFER
  ACExtern *_Extern();
  void _Permute();

  ACStatement *_Statement();      // scan one or more statements without new scope
  ACExpression *_Expression();
  ACExpression *_Expression1(sInt level);
  ACExpression *_Value();
  ACLiteral *_Literal();
  void _ParameterList(ACExpression **patch);

  // output
  
  sInt OutputLanguage;
  sInt OutputRender;
  void OutStruct(ACType *type);
  void OutTypeDecoration(ACType *type,sPoolString name);
  void OutVar(ACVar *var,ACExpression *init);
  void OutBlock(ACStatement *stat,sInt indent);
  void OutStat(ACStatement *stat,sInt indent);
  void OutBinary(ACExpression *left,ACExpression *right,const sChar *name,sBool brackets);
  void OutExpr(ACExpression *expr,sBool brackets=0);
  void OutLiteral(ACLiteral *lit);
  void OutLitVal(ACLiteral *lit);

public:

  ACDoc();
  ~ACDoc();

  sTextBuffer Out;                  // hlsl put generated by Output()

  void NewFile();                   // clears cbuffers / permutations
  sBool Parse(const sChar *source); // parse a shader or cbuffer/permute source
  void Void();                      // invalidate ASC buffer. Output will fail.
  void Output(sInt shadertype,sInt renderer);     // output shader as sSTF_CG or sSTF_DX (hlsl), sSTF_VERTEX or sSTF_PIXEL. pif(renderer)
  sBool IsValidPermutation(ACPermute *perm,sInt n);
};

/****************************************************************************/

#endif // FILE_ASC_ASC_DOC_HPP

