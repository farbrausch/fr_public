/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4OPS_DOC_HPP
#define FILE_WZ4OPS_DOC_HPP

#include "base/types2.hpp"
#include "util/scanner.hpp"

/****************************************************************************/

enum DocTypes
{
  TYPE_LABEL = 1,
  TYPE_INT,
  TYPE_FLOAT,
  TYPE_COLOR,
  TYPE_FLAGS,
  TYPE_RADIO,
  TYPE_STRING,
  TYPE_FILEIN,
  TYPE_FILEOUT,
  TYPE_LINK,
  TYPE_GROUP,
  TYPE_STROBE,
  TYPE_ACTION,                    // trigger a code during editing that was defined with the op, that has access to the operator itself
  TYPE_CUSTOM,                    // Add a custom control
  TYPE_BITMASK,
  TYPE_CHARARRAY,
};

enum CustomTokens
{
  TOK_ANDAND = 0x100,
  TOK_OROR,
  TOK_IF,
  TOK_ELSE,
  TOK_ARRAY,
};

class CodeBlock
{
public:
  CodeBlock(sPoolString code,sInt line);
  void Output(sTextBuffer &,const sChar *file,sBool semi=0);

  sPoolString Code;
  sInt Line;
};


class External
{
public:
  External();
  ~External();

  sInt Line;
  sPoolString Type;
  sPoolString PureType;
  sPoolString Name;
  sPoolString Para;
  CodeBlock *Code;
};

class Importer
{
public:
  Importer();
  sPoolString Name;
  sPoolString Extension;
  sPoolString Code;
};

class Type
{
public:
  Type();
  ~Type();

  sPoolString Parent;
  sPoolString Symbol;
  sPoolString Label;
  sU32 Color;
  sInt Flags;
  sInt GuiSets;
  sBool Virtual;
  CodeBlock *Code;
  CodeBlock *Header;
  sPoolString ColumnHeaders[31];
  sArray<External *> Externals;
};

enum ExprOp
{
  EOP_NOP,
  // binary
  EOP_GT,
  EOP_LT,
  EOP_GE,
  EOP_LE,
  EOP_EQ,
  EOP_NE,
  EOP_AND,
  EOP_OR,
  EOP_BITAND,
  EOP_BITOR,
  // unary
  EOP_NOT,
  EOP_BITNOT, 
  // special
  EOP_INT,
  EOP_SYMBOL,
  EOP_INPUT,
};

struct ExprNode
{
  sInt Op;
  ExprNode *Left;
  ExprNode *Right;

  sInt Value;
  sPoolString Symbol;
};

enum ParaFlag
{
  PF_Anim       = 0x00000001,
  PF_LineNumber = 0x00000002,
  PF_Narrow     = 0x00000004,
  PF_OverLabel  = 0x00000008,
  PF_OverBox    = 0x00000010,
  PF_Static     = 0x00000020,
};

class Parameter
{
public:
  Parameter();

  sInt Type;
  sInt Offset;
  sPoolString Label;
  sPoolString Symbol;
  sPoolString CType;
  sPoolString Format;
  sF32 Min,Max,Step,RStep;
  sInt Count;
  sBool XYZW;
  sInt LinkMethod;
  union
  {
    sS32 DefaultS[16];
    sU32 DefaultU[16];
    sF32 DefaultF[16];
  };
  sBool DefaultStringRandom;
  sPoolString DefaultString;
  sPoolString Options;
  ExprNode *Condition;
  sInt LayoutFlag;      // use LayoutMsg, not ChangeMsg, only for Flags
  sInt ContinueFlag;    // do not create new variable, just new gui. for continuing Flags
  sInt FilterMode;
  sPoolString CustomName;   // name of custom control class

  sInt GridLines;           // lines in grid (for text edit thingies
  sInt Flags;
/*
  sInt OverBoxFlag;         // width in grid
  sInt OverLabelFlag;       // width in grid
  sBool AnimFlag;           // this parameter is animatable
  sBool LineNumberFlag;     // line numbers in printf's
  sBool NarrowFlag;
*/
  sPoolString ScriptName();
};

enum InputLinkMethod
{
  IE_INPUT,
  IE_LINK,
  IE_BOTH,
  IE_CHOOSE,
  IE_ANIM,
};

enum InputFlagsEnum
{
  IF_OPTIONAL = 0x0001,
  IF_WEAK = 0x0002,
};

class Input
{
public:
  Input();

  sPoolString Type;
  sPoolString DefaultOpName,DefaultOpType;
  sBool InputFlags;
  sInt Method;
  sPoolString GuiSymbol;
};

struct StructMember
{
  sPoolString CType;
  sPoolString Name;
  sInt PointerCount;
  sInt ArrayCount;
  sInt Line;
};
struct Struct
{
  sArray<StructMember> Members;
};
struct AnimInfo
{
  sPoolString Name;
  sInt Flags;
  AnimInfo() { Flags = 0; }
};
struct ActionInfo
{
  sPoolString Name;
  sInt Id;
  void Init(sPoolString n,sInt i) { Name=n; Id=i; }
};

struct Tie
{
  sArray<sPoolString> Paras;
};

class Op
{
public:
  Op();
  ~Op();
  
  sPoolString Name;
  sPoolString OutputType;     // wz4 class name of output
  sPoolString TabType;
  sPoolString OutputClass;    // c class name of output wObject derived class
  sPoolString Label;          // screen name of the op
  sInt ArrayNumbers;          // add number labeling
  sArray<Parameter *> Parameters;
  sArray<Parameter *> ArrayParam;
  sArray<AnimInfo> AnimInfos;
  sArray<ActionInfo> ActionInfos;
  sArray<Input *> Inputs;
  sArray<Tie *> Ties;
  sArray<Tie *> ArrayTies;
    
  CodeBlock *Code;
  sInt Shortcut;
  sInt Column;
  sInt MaxOffset;
  sInt MaxStrings;
  sInt MaxArrayOffset;
  sU32 FileInMask;
  sU32 FileOutMask;
  sPoolString FileInFilter;
  sInt Flags;
  sInt HideArray;
  sInt GroupArray;
  sInt GridColumns;
  sInt DefaultArrayMode;
  sPoolString Extract;
  sPoolString CustomEd;

  CodeBlock *Handles;
  CodeBlock *SetDefaultsArray;
  CodeBlock *Actions;
  CodeBlock *SpecialDrag;
  CodeBlock *Description;
  Struct *Helper;
};


class Document
{
  // parser

  sMemoryPool Pool;
  ExprNode *NewExpr(sInt op,ExprNode *left,ExprNode *right);
  ExprNode *NewExprInt(sInt value);

  sBool FindFlag(sPoolString para,const sChar *choice,sInt &mask,sInt &value);

  void _Global(); 
  void _Include(sArray<CodeBlock *> &codes);
  CodeBlock *_CodeBlock();
  void _Type();
  sInt _Choice(const sChar *match);
  sInt _Flag(const sChar *match);
  void _Operator();
  void _Parameter(Op *op,ExprNode *cond,sInt &offset,sInt &stringoffset,sInt inarray,sInt &linkoffset);
  void _Parameters(Op *op,ExprNode *cond,sInt &offset,sInt &stringoffset,sInt inarray,sInt &linkoffset);
  void _External(sArray<External *> &list);
  Struct *_Struct();

  ExprNode *_Literal();
  ExprNode *_Value();
  ExprNode *_Expr(sInt level=6);

  void HPPLine(sInt line=-1);
  void CPPLine(sInt line=-1);
  void OutputCodeblocks();
  void OutputExt(External *ext,const sChar *classname);
  void OutputTypes1();
  void OutputTypes2();
  void OutputParaStruct(const sChar *label,sArray<Parameter *> &pa,Op *op);
  void OutputIf(ExprNode *cond,ExprNode **condref,sInt indent);
  void OutputPara(sArray<Parameter *> &para,Op *op,sBool inarray);
  void OutputTies(sArray<Tie *> &ties);
  void OutputOps();
  void OutputMain();
  void OutputExpr(ExprNode *);
  void OutputStruct(sTextBuffer &tb,Struct *,const sChar *name);
  void OutputAnim();

  sArray<CodeBlock *> HCodes;
  sArray<CodeBlock *> HEndCodes;
  sArray<CodeBlock *> CCodes;
  sArray<Type *> Types;
  sArray<Op *> Ops;
  Op *CurrentOp;

public:
  Document();
  ~Document();
  sScanner Scan;

  sBool Parse(const sChar *filename);
  sBool Output();
  sTextBuffer CPP;
  sTextBuffer HPP;

  sInt Priority;

  void SetNames(const sChar *name);
  sString<2048> InputFileName;
  sString<2048> CPPFileName;
  sString<2048> HPPFileName;
  sString<2048> ProjectName;
};
 
extern Document *Doc;

/****************************************************************************/

#endif // FILE_WZ4OPS_DOC_HPP
