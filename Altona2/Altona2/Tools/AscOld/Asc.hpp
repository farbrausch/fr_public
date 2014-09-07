/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_TOOLS_ASC_ASC_HPP
#define FILE_ALTONA2_TOOLS_ASC_ASC_HPP

#include "altona2/libs/base/base.hpp"
#include "altona2/libs/util/scanner.hpp"

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

namespace AltonaShaderLanguage {
using namespace Altona2;

/****************************************************************************/

enum wTokens
{
  // asc

  TokMtrl = 0x100,
  TokPermute,
  TokPif,
  TokPelse,
  TokVS,TokHS,TokDS,TokGS,TokPS,TokCS,
  TokIA2VS,TokVS2RS,TokVS2PS,TokPS2OM,
  TokVS2GS,TokGS2PS,TokGS2RS,

  // hlsl

  TokInc,
  TokDec,
  TokAssignAdd,
  TokAssignSub,
  TokAssignMul,
  TokAssignDiv,
  TokAssignMod,
  TokLAnd,
  TokLOr,

//  TokBlendState,
  TokBool,
  TokBreak,
  TokBuffer,
  TokCBuffer,
  TokClass,
//  TokCompile,
  TokColumnMajor,
  TokConst,
  TokContinue,
//  TokDepthStencilState,
//  TokDepthStencilView,
  TokDiscard,
  TokDo,
  TokDouble,
  TokElse,
  TokExtern,
  TokFalse,
  TokFloat,
  TokFor,
//  TokGeometryShader
  TokGroupShared,
  TokHalf,
  TokIf,
  TokIn,
  TokInline,
  TokInout,
  TokInt,
//  TokInterface,
  TokMatrix,
  TokNamespace,
  TokNointerpolation,
  TokOut,
  TokPass,
//  TokPixelShader,
  TokPrecise,
//  TokRasterizerState,
//  TokRenderTargetView,
  TokReturn,
  TokRegister,
  TokRowMajor,
  TokSampler,TokSampler1D,TokSampler2D,TokSampler3D,TokSamplerCube,
  TokSamplerState,TokSamplercomparisonState,    // see IsSamplerType()
//  TokStateBlock,
//  TokStateBlockState,
  TokSlot,
  TokStatic,
//  TokString,
  TokStruct,
  TokSwitch,
//  TokTBuffer,
//  TokTechnique,
//  TokTechnique10,
  TokTexture,TokTexture1D,TokTexture1DArray,TokTexture2D,TokTexture2DArray,
  TokTexture2DMS,TokTexture2DMSArray,
  TokTexture3D,TokTextureCube,TokTextureCubeArray,  // see IsTextureType()
  TokTrue,
  TokTypedef,
  TokUInt,
  TokUniform,
  TokVector,
//  TokVertexShader,
  TokVoid,
  TokVolatile,
  TokWhile,

  TokLowP,TokMediumP,TokHighP,
};

/****************************************************************************/

enum wLitKind
{
  LitError = 0,
  LitBool,
  LitInt,
  LitFloat,
  LitCompound,
};

enum wSymbolKind
{
  SymError = 0,
  SymType,
  SymObject,
//  SymMember,
  SymPermute,
};

enum wTypeKind
{
  TypeError = 0,
  TypeVoid,
  TypeBase,                       // int,uint,half,float,double
//  TypeSampler,                    // sampler, sampler1d, sampler2d, ...
//  TypeTexture,                    // texture, texture1d, texture2d, ...

  TypeVector,
  TypeMatrix,
  TypeArray,
  TypeScope,
};

enum wExprKind
{
  ExprError = 0,
  ExprMul,ExprDiv,ExprMod,
  ExprAdd,ExprSub,
  ExprShiftL,ExprShiftR,
  ExprGT,ExprLT,ExprGE,ExprLE,
  ExprEQ,ExprNE,
  ExprAnd,ExprOr,ExprEor,
  ExprLAnd,ExprLOr,
  ExprCond,
  ExprAssign,ExprAssignAdd,ExprAssignSub,ExprAssignMul,ExprAssignDiv,ExprAssignMod,

  ExprNot,ExprNeg,ExprComplement,
  ExprPreInc,ExprPreDec,ExprPostInc,ExprPostDec,
  ExprLit,
  ExprIndex,
  ExprSymbol,
  ExprCast,
  ExprCall,
  ExprMember,

};

enum wStmtKind
{
  StmtError = 0,
  StmtNop,
  StmtExpr,
  StmtBlock,
  StmtObject,

  StmtBreak,
  StmtContinue,
  StmtDiscard,
  StmtDo,
  StmtFor,
  StmtIf,
  StmtPif,
  StmtReturn,
  StmtTypedef,
  StmtWhile,
};

enum wInvariantKind
{
  InvIA2VS  = 0x0001,             // for simple shaders
  InvVS2RS  = 0x0002,
  InvVS2PS  = 0x0004,
  InvPS2OM  = 0x0008,

  InvVS2GS  = 0x0010,             // geometry shaders
  InvGS2PS  = 0x0020,
  InvGS2RS  = 0x0040,

  InvVS     = 0x0100,
  InvHS     = 0x0200,
  InvDS     = 0x0400,
  InvGS     = 0x0800,
  InvPS     = 0x1000,
  InvCS     = 0x2000,
};

enum wMemberFlags
{
  MemExtern           = 0x00000001,
  MemNointerpolation  = 0x00000002,
  MemPrecise          = 0x00000004,
  MemGroupShared      = 0x00000008,
  MemStatic           = 0x00000010,
  MemUniform          = 0x00000020,
  MemVolatile         = 0x00000040,

  MemConst            = 0x00010000,
  MemColumnMajor      = 0x00020000,
  MemRowMajor         = 0x00040000,

  MemLowP             = 0x00100000,
  MemMediumP          = 0x00200000,
  MemHighP            = 0x00400000,
  MemPrecisionMask    = 0x00700000,
};

/****************************************************************************/

struct wLit;
struct wScope;
struct wSymbol;
struct wType;
struct wExpr;
struct wStmt;
struct wMember;
struct wPermute;
struct wPermuteOption;
struct wInvariant;
struct wIntrinsic;

struct wLit
{
  wLitKind Kind;
  union
  {
    sS32 Int;
    sU32 UInt;
    sF32 Float;
    sF64 Double;
  };
  sSList<wLit> Members;
  wLit *Next;
};

struct wType
{
  wTypeKind Kind;
  sInt Token;
  wType *Base;
  sScannerLoc Loc;
  sPoolString DebugName;

  wScope *Scope;
  wExpr *Count;
  sInt Rows;
  sInt Columns;

  sSList<wSymbol> Members;

  sBool IsTextureType() { return Kind==TypeBase && Token>=TokTexture && Token<=TokTextureCubeArray; }
  sBool IsSamplerType() { return Kind==TypeBase && Token>=TokSampler && Token<=TokSamplercomparisonState; }
};

struct wScope
{
  wScope *Parent;
  sSList<wScope> Childs;
  sSList<wSymbol> Symbols;

  wScope *Next;
  void Add(wSymbol *sym) { Symbols.Add(sym); }
};

struct wSymbol
{
  wSymbolKind Kind;
  wType *Type;
  sPoolString Name;
  sScannerLoc Loc;
  wPermute *Permute;    // SymPermute
  sPoolString Surrogate;

  wSymbol *Next;
};

struct wExpr
{
  wExprKind Kind;
  wExpr *a,*b,*c;
  wLit *Literal;
  sScannerLoc Loc;

  wScope *Scope;        // always set
  wType *Type;          // ExprCast, ExprError for typedef
  sPoolString Name;     // ExprMember
  sSList<wExpr> Args;   // ExprCall, ExprCast

  wExpr *Next;          // for argument list
  wType *ResultType;    // for type checking
  wIntrinsic *Intrinsic;
};

struct wStmt
{
  wStmtKind Kind;
  wStmt *Childs;
  wStmt *Alt;
  wExpr *Expr;
  sScannerLoc Loc;
  wStmt *Next;
  wMember *Object;
  wScope *Scope;
};

struct wPermuteOption
{
  sPoolString Name;
  sInt Value;                     // value is shifted

  wPermuteOption *Next;
};

struct wPermute
{
  sPoolString Name;
  sSList<wPermuteOption> Options;
  sInt Shift;
  sInt Mask;                      // mask is before shifting.
  sInt Count;                     // how many options are there?

  wPermute *Next;
};

struct wInvariant
{
  sInt KindMask;
  sInt Slot;
  sInt MemberFlags;
  sPoolString Name;
  sPoolString Semantic;
  sPoolString InSurrogate;        // set with the GL2 surrogete for input (for in out arguments)
  wType *Type;
  wExpr *Condition;
  wInvariant *Next;
};

struct wMember
{
  sPoolString Name;
  sInt Flags;
  wType *Type;
  wExpr *Initializer;
  sPoolString Semantics;
  wMember *Next;

  // constant buffer packing

  sInt Register;                  // this member is mapped to a xyzw constant register directly. Use this when outputting shader coder
  sInt AssignedScalar;            // use the Asssigned fields when outputting constant buffer structure to header . Register*4 + field
  sPoolString Surrogate;      

};

struct wCBuffer
{
  sShaderTypeEnum Shader;
  sPoolString Name;
  sInt Slot;
  sInt Register;
  sInt RegSize;
  sSList<wMember> Members;

  wCBuffer *Next;
};

struct wMtrl
{
  wMtrl() { GeoMaxVert=0; GeoType=0; }
  sPoolString Name;
  wStmt *Shaders[sST_Max];
  sSList<wPermute> Permutes;
  sSList<wInvariant> Invariants;
  sSList<wCBuffer> CBuffers;

  // geometry shader specials

  sInt GeoMaxVert;
  sInt GeoType;           // 1..5: point line trianlge lineadj trianlgeadj
};

struct wIntrinsic
{
  wType *Class;
  sPoolString Name;
  wType *Result;
  wType *Arg[4];
  sInt ArgCount;

  const sChar *HandleDX9;
  const sChar *HandleDX11;
  const sChar *HandleGL2;
};

class wMaterialOut
{
public:
  wMaterialOut(sInt permmax,sPoolString name);
  ~wMaterialOut();

  sPoolString Name;
  sInt PermMax;
  
  sShaderBinary **VS;
  sShaderBinary **HS;
  sShaderBinary **DS;
  sShaderBinary **GS;
  sShaderBinary **PS;
  sShaderBinary **CS;
};

/****************************************************************************/

class Document
{
  sScanner Scan;
  sMemoryPool *Pool;

  // helpers

  wScope *MakeScope();
  wSymbol *MakeSym(wSymbolKind kind,sPoolString name,wType *type=0);
  wLit *MakeLit(wLitKind kind);
  wType *MakeType(wTypeKind kind,wType *base);
  wType *MakeTypeBase(sInt Token);
  wExpr *MakeExpr(wExprKind kind,wExpr *a=0,wExpr *b=0,wExpr *c=0);
  wStmt *MakeStmt(wStmtKind kind);

  void PushScope();
  void PopScope();
  sInt GetRegCount(wMember *mem);
  // hlsl-similar

  void _Struct();
  wType *_Type();
  sInt _MemberFlags();
  wMember *_Decl();
  void _CBuffer();
  void _Args(wExpr *);
  wLit *_Lit();
  wExpr *_Value();
  wExpr *_ExprR(sInt level);
  wExpr *_Expr();
  wStmt **_Stmt(wStmt **n);
  wStmt *_Block(sBool noscope = 0);

  // asc language

  wStmt *_Shader();
  void _A2B(wExpr *cond);
  void _Permute();
  void _MtrlBlock(wExpr *cond);
  void _MtrlStmt(wExpr *cond);
  void _MtrlMakeTypes(wType *base,const sChar *name);
  void _Mtrl();
  void _Global();

  // dumping

  void DumpLit(wLit *);
  void DumpType(wType *);
  void DumpExprBin(const sChar *op,wExpr *a,wExpr *b);
  void DumpExpr(wExpr *,sBool brace);
  void DumpStmt(wStmt *,sInt indent);
  void DumpMtrl();

  // output for real

  void OutMemberFlags(sInt flags);
  void OutMember(wMember *mem,sInt indent,sInt flags);
  void OutLit(wLit *);
  void OutType(wType *);
  void OutExprPrintBin(const sChar *op,wExpr *e,sInt typemode);
  void OutExprPrint(wExpr *,sBool brace);
  void OutExprTypeBin(const sChar *op,wExpr *e,sInt typemode);
  void OutExprType(wExpr *);
  void OutExpr(wExpr *);
  void OutStmt(wStmt *,sInt indent,sBool pif=0);
  sShaderBinary *OutShader(sShaderTypeEnum skind,wStmt *root);

  // some resoning

  void AssignPermute();
  wSymbol *FindSymbol(sPoolString name);
  wSymbol *FindSymbol(wScope *scope,sPoolString name);
  sBool IsSymbolUnique(sPoolString name);
  sBool IsPermutationValid(sInt perm);
  sInt ConstInt(wExpr *);
  void ClearObjectSymbols(wScope *scope);
  wType *FuncType(const sChar *name);
  wIntrinsic *Func(const sChar *rt,const sChar *cl,sPoolString name,const sChar *arg0=0,const sChar *arg1=0,const sChar *arg2=0,const sChar *arg3=0);
  wType *GetLiteralType(wLit *);
  sBool Same(const sScannerLoc &loc,wType *a,wType *b);
  wType *Compatible(const sScannerLoc &loc,wType *from,wType *to,sBool noerror=0);
  wType *MakeCompatible(const sScannerLoc &loc,wExpr **from_,wType *to_,sBool noerror=0);
  void WriteAsm();

  // global state

  sInt Flags;                     // 1 = dump
  sInt Platform;                  // sConfigRender???
  sInt MaxPerm;                   // max value of permutation
  sShaderTypeEnum ShaderType;     // pixel or vertex? valid in OutShader().
  sTextLog Dump;                  // debug dump
  sTextLog Asm;                   // generated asm file
  sTextLog Hpp;                   // generated hpp file
  sTextLog Cpp;                   // generated cpp file (alternative to asm file)
  sArrayPtr<wIntrinsic *> Intrinsics;

  // material state

  wMtrl *Material;
  wScope *CScope;
  wScope *GScope;
  sInt PifCount;
  wType *ErrorType;
  sInt TempCount;                 // for naming temp vars

  sArrayPtr<wType *> BaseTypes;

  // output state

  sTextLog Out;                   // output to shader compiler
  sInt Perm;                      // current permutation value
  sArrayPtr<sShaderBinary *> AllShaders;  // all shaders, unique
  sArrayPtr<wMaterialOut *> MtrlOuts;  // permutation patterns
public:
  Document();
  ~Document();

  sBool Parse(const sChar *filename,sInt platform,sInt flags);
  const sChar *GetDump() { return Dump.Get(); }
  const sChar *GetAsm() { return Asm.Get(); }
  const sChar *GetCpp() { return Cpp.Get(); }
  const sChar *GetHpp() { return Hpp.Get(); }
};

/****************************************************************************/

};  // namespace AltonaShaderLanguage

/****************************************************************************/

#endif  // FILE_ALTONA2_TOOLS_ASC_ASC_HPP

