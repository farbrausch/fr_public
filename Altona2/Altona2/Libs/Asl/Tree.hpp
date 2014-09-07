/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_ASL_TREE_HPP
#define FILE_ALTONA2_LIBS_ASL_TREE_HPP

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Util/Scanner.hpp"

namespace Altona2 {
class sTree;
namespace Asl {
using namespace Altona2;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

struct sType;
struct sExpr;
struct sMember;
struct sVariable;
struct sIntrinsic;
struct sInject;
struct sModule;

/****************************************************************************/

enum sTarget
{
    sTAR_Asl,
    sTAR_Hlsl5,
    sTAR_Hlsl3,
    sTAR_Glsl1,
    sTAR_Glsl1Es,
    sTAR_Glsl4,
    sTAR_Max,
};

enum sBaseTypeMask
{
    sBT_Float           = 0x0001,
    sBT_Int             = 0x0002,
    sBT_Uint            = 0x0004,
    sBT_Bool            = 0x0008,
    sBT_Double          = 0x0010,
    sBT_NonBool         = sBT_Float|sBT_Int|sBT_Uint|sBT_Double,

    sBT_Scalar          = 0x0100,
    sBT_Vector          = 0x0200,
    sBT_Matrix          = 0x0400,
    sBT_All             = sBT_Scalar|sBT_Vector|sBT_Matrix,
    sBT_Vector3         = 0x0800,
    sBT_SquareMatrix    = 0x1000,
    sBT_VectorPromote   = 0x2000,
};

enum sTemplateArgFlags
{
    sTAF_ResultBool     = 0x00000001,
    sTAF_ResultScalar   = 0x00000002,
    sTAF_MulSpecial     = 0x00000004,
    sTAF_ResultVoid     = 0x00000008,
    sTAF_Hlsl5Sampler   = 0x00000010,
    sTAF_ArgAnyType     = 0x00000020,       // for conversion operators
};

enum sTypeKind
{
    sTK_Error,
    sTK_Void,

    sTK_Float,                              // base types
    sTK_Int,
    sTK_Uint,
    sTK_Bool,
    sTK_Double,

    sTK_Array,                              // composite types
    sTK_Vector,
    sTK_Matrix,
    sTK_Struct,

    sTK_CBuffer,                            // buffers
    sTK_TBuffer,
    sTK_Buffer,
    sTK_ByteBuffer,
    sTK_StructBuffer,

    sTK_Sampler,
    sTK_SamplerState,
    sTK_SamplerCmpState,

    sTK_TexSam1D,
    sTK_TexSam2D,
    sTK_TexSam3D,
    sTK_TexSamCube,
    sTK_TexCmp1D,
    sTK_TexCmp2D,
    sTK_TexCmp3D,
    sTK_TexCmpCube,

    sTK_Texture1D,                          // textures
    sTK_Texture2D,
    sTK_Texture2DMS,
    sTK_Texture3D,

    sTK_TextureArray1D,
    sTK_TextureArray2D,
    sTK_TextureArray2DMS,

    sTK_RWBuffer,                           // UAV
    sTK_RWByteBuffer,
    sTK_RWStructBuffer,

    sTK_RWTexture1D,
    sTK_RWTexture2D,
    sTK_RWTextureArray1D,
    sTK_RWTextureArray2D,
    sTK_RWTexture3D,

    sTK_AppendBuffer,
    sTK_ConsumeBuffer,

    sTK_InputPatch,                         // Tesselation
    sTK_OutputPatch,

    sTK_PointStream,                        // Stream Out
    sTK_LineStream,
    sTK_TriangleStream,

    sTK_GeoPoint,                           // Geometry
    sTK_GeoLine,
    sTK_GeoTriangle,
    sTK_GeoLineAdj,
    sTK_GeoTriangleAdj,

    // Glsl1

    sTK_Sampler2D,
    sTK_SamplerCube,
};

enum sSemantic
{
    sSEM_None,

    // asl semantics

    sSEM_Position,                          // Input Assembly
    sSEM_Normal,
    sSEM_Tangent,
    sSEM_Binormal,
    sSEM_BlendIndex,
    sSEM_BlendWeight,
    sSEM_Color,
    sSEM_Tex,

    sSEM_InstanceId,                        // vertex shader input (and later)
    sSEM_VertexId,

    sSEM_PositionT,                         // Rasterizer

    sSEM_VFace,                             // Pixel Shader
    sSEM_VPos,
    sSEM_Target,
    sSEM_Depth,
    sSEM_Coverage,
    sSEM_IsFrontFace,
    sSEM_SampleIndex,
    sSEM_PrimitiveId,

    sSEM_GSInstanceId,                      // Geometry Shader
    sSEM_RenderTargetArrayIndex,
    sSEM_ViewportArrayIndex,

    sSEM_DomainLocation,                    // Tesselation
    sSEM_InsideTessFactor,
    sSEM_OutputControlPointId,
    sSEM_TessFactor,

    sSEM_DispatchThreadId,                  // Compute
    sSEM_GroupId,
    sSEM_GroupIndex,
    sSEM_GroupThreadId,

    sSEM_Vs,
    sSEM_Hs,
    sSEM_Ds,
    sSEM_Gs,
    sSEM_Ps,
    sSEM_Cs,
    sSEM_Slot,
    sSEM_SlotAuto,
    sSEM_Sampler,
    sSEM_Texture,
    sSEM_Const,
    sSEM_Uav,

    // special codes

    sSEM_Max,

    sSEM_Mask = 0xffff,
    sSEM_IndexMask = 0xff0000,
    sSEM_IndexShift = 16,
};

enum sStorageFlags
{
    // ASL 

    sST_RowMajor        = 0x00000001,
    sST_ColumnMajor     = 0x00000002,
    sST_LowP            = 0x00000004,
    sST_MedP            = 0x00000008,
    sST_HighP           = 0x00000010,
    sST_Const           = 0x00000020,
    sST_GroupShared     = 0x00000040,
    sST_Static          = 0x00000080,
    sST_Precise         = 0x00000100,

    sST_In              = 0x00000200,
    sST_Out             = 0x00000400,
    sST_Import          = 0x00000800,
    sST_Export          = 0x00001000,

    // sampler 

    sST_Linear          = 0x00010000,
    sST_NoInterpolation = 0x00020000,
    sST_Centroid        = 0x00040000,
    sST_NoPerspective   = 0x00080000,
    sST_Sample          = 0x00100000,

    // GLSL conversiomn

    sST_Uniform         = 0x10000000,
    sST_Varying         = 0x20000000,
    sST_Attribute       = 0x40000000,
};

struct sStorage
{
    sStorage() { sClear(*this); }
    sStorage(int flags) { sClear(*this); Flags = flags; }
    int Flags;
};

struct sMember
{
    sScannerLoc Loc;
    sType *Type;
    sPoolString Name;
    sStorage Storage;
    sSemantic Semantic;
    int Offset;                         // 32 bit offset
    bool Active;                        // modules might be inactive
    bool Align;                         // force alignment, used by inject
    sModule *Module;                    // if injected, the module that injected
    sExpr *Condition;                   // if injected, optional condition

    sMember *Vector[4];                 // for matrix -> vector transform
    sExpr *ReplaceExpr;                 // replace member acces with this expression
    sMember *ReplaceMember;             // used by sModules::Transform
    sMember *DeepCopy;
};

struct sType
{
    sScannerLoc Loc;
    sTypeKind Kind;
    sType *Base;

    sPoolString Name;                   // for struct, ...
    int SizeX;      // columns
    int SizeY;      // rows
    sExpr *SizeArray;
    sPoolArray<sMember *> Members;
    sSemantic Semantic;
    sPoolArray<sIntrinsic *> Intrinsics;
    int StructSize;
    sType *DeepCopy;
    bool HasTemplate;
    sType *TemplateType;

    int Temp;

    int GetSize(sTree *tree);                          // size in 32 bit words
    int GetVectorSize(sTree *tree);                    // size in 32 bit words, 1..4 (4 for arrays)
    bool IsBaseType();
    bool CheckBaseType(int allowedTypes);
    bool Compatible(sType *);
    bool IsSame(sType *);
    void AssignOffsets(sTree *tree,int startoffset = 0);
};


/****************************************************************************/

struct sTypeDef
{
    sScannerLoc Loc;
    sType *Type;
    int Shaders;
    int Temp;
};

struct sVariable
{
    sScannerLoc Loc;
    sType *Type;
    sPoolString Name;
    sStorage Storage;
    sSemantic Semantic;
    int Shaders;
    sExpr *Initializer;
    sVariable *DeepCopy;
    int *PatchSlot;

    sVariable *TempReplace;
    sVariable *Vector[4];                   // for matrix -> vector transform
};

/****************************************************************************/

struct sLiteral
{
    sScannerLoc Loc;
    sType *Type;
    union
    {
        uint *DataU;
        int *DataI;
        float *DataF;
        double *DataD;
    };
    int Elements;                           // in 4 byte units, so doubles take two elements
    sLiteral *DeepCopy;
};

/****************************************************************************/
/****************************************************************************/


enum sExprKind
{
    sEX_Error,

    sEX_Literal,
    sEX_Variable,
    sEX_Member,
    sEX_Swizzle,
    sEX_SwizzleExpand,
    sEX_Intrinsic,
    sEX_Index,
    sEX_Construct,          // float4(xxxx), float4x4(xxxx)
    sEX_Condition,          // for cif/celse
    sEX_Cast,               // (float)x
    sEX_Call,

    sEX_Neg,
    sEX_Pos,
    sEX_Not,
    sEX_Complement,
    sEX_PreInc,
    sEX_PreDec,
    sEX_PostInc,
    sEX_PostDec,

    sEX_Add,
    sEX_Sub,
    sEX_Mul,
    sEX_Div,
    sEX_Mod,
    sEX_Dot,

    sEX_ShiftL,
    sEX_ShiftR,
    sEX_BAnd,
    sEX_BOr,
    sEX_BEor,

    sEX_EQ,
    sEX_NE,
    sEX_LT,
    sEX_GT,
    sEX_LE,
    sEX_GE,

    sEX_LAnd,
    sEX_LOr,

    sEX_Cond,

    sEX_Assign,
    sEX_AssignAdd,
    sEX_AssignSub,
    sEX_AssignMul,
    sEX_AssignDiv,
    sEX_AssignMod,
    sEX_AssignShiftL,
    sEX_AssignShiftR,
    sEX_AssignBAnd,
    sEX_AssignBOr,
    sEX_AssignBEor,

};

struct sExpr
{
    sScannerLoc Loc;
    sExprKind Kind;
    sPoolArray<sExpr *> Args;
    sType *Type;

    sLiteral *Literal;
    uint Swizzle;
    sVariable *Variable;
    sMember *Member;
    sIntrinsic *Intrinsic;
    sPoolString CondName;
    sPoolString CondMember;
    sPoolString FuncName;
    
    sPoolString IntrinsicReplacementName;
    sExpr *DeepCopy;
};

/****************************************************************************/
/****************************************************************************/

enum sStatementKind
{
    sST_Error,
    sST_Block,
    sST_BlockNoBrackets,
    sST_Nop,
    sST_Decl,
    sST_If,
    sST_Cif,
    sST_For,
    sST_While,
    sST_Switch,
    sST_Do,
    sST_Break,
    sST_Continue,
    sST_Discard,
    sST_Return,
    sST_Case,
    sST_Default,
    sST_Expr,
    sST_Inject,
};

enum sStatementFlags
{
    sSTF_Unroll              = 0x0001,
    sSTF_Loop                = 0x0002,
    sSTF_FastOpt             = 0x0004,
    sSTF_AllocUavCondition   = 0x0008,
};

struct sStmt
{
    sScannerLoc Loc;
    sStatementKind Kind;
    int SwitchCase;
    sPoolArray<sStmt *> Block;
    sPoolArray<sVariable *> Vars;
    sExpr *Expr;
    sExpr *IncExpr;
    sStmt *ParentScope;
    sInject *Inject;
    sStmt *DeepCopy;
    int StatementFlags;

    void CopyFrom(sStmt *src)
    {
        Loc = src->Loc;
        Kind = src->Kind;
        SwitchCase = src->SwitchCase;
        Expr = src->Expr;
        IncExpr = src->IncExpr;
        ParentScope = src->ParentScope;
        Inject = src->Inject;
        DeepCopy = src->DeepCopy;

        Block.CopyFrom(src->Block);
        Vars.CopyFrom(src->Vars);
    }
};

/****************************************************************************/

struct sFunction
{
    sScannerLoc Loc;
    sPoolString Name;
    sStmt *Root;
    sType *Result;
    int Shaders;

    // hlsl specials

    sExpr *NumThreadsX;
    sExpr *NumThreadsY;
    sExpr *NumThreadsZ;
};

/****************************************************************************/

struct sIntrinsic
{
    sScannerLoc Loc;
    sPoolString Name;
    int ArgCount;
    int MinArgCount;                // in case of optional parameters
    int TypeMask;
    int Flags;
    sType *ArgType[5];
    sType *ResultType;
    sIntrinsic *DeepCopy;
};

/****************************************************************************/

enum sInjectKind
{
    sIK_Code = 1,
    sIK_Const,
    sIK_Struct,
};

struct sInject                             // Point where to inject
{
    sScannerLoc Loc;
    sPoolString Name;
    sInjectKind Kind;
    sStmt *Scope;
    sType *Type;                            // sIK_Const
    sPoolArray<sMember *> Members;          // sIK_Const
    sInject *DeepCopy;
    int Shaders;
};

struct sSniplet
{
    sStmt *Code;                            // sIK_Code
    sInject *Inject;
};

struct sCondition
{
    sPoolString Name;
    sPoolString Member;
    sTypeKind TypeKind;
    int Value;

    sCondition() { Value = 0; Name.Clear(); Member.Clear(); TypeKind = sTK_Bool; }
    void Set(sTypeKind tk,sPoolString n) { Value = 0; Name = n; Member.Clear(); TypeKind = tk; }
    void Set(sTypeKind tk,sPoolString n,sPoolString m) { Value = 0; Name=n; Member=m; TypeKind = tk; }
};

struct sModuleDependency
{
    sModule *Module;
    sExpr *Condition;

    sModuleDependency() { Module = 0; Condition = 0; }
    sModuleDependency(sModule *m,sExpr *c) { Module = m; Condition = c; }
};

struct sModule
{
    sScannerLoc Loc;
    sPoolString Name;
    bool Once;
    bool OnceTemp;
    sPoolArray<sSniplet *> Sniplets;
    sPoolArray<sModuleDependency> Depends;           // additional injections to draw in
    sModule *DeepCopy;
    sPoolArray<sCondition> Conditions;
    sPoolArray<sVariable *> Vars;
};

struct sVariantMember
{
    sScannerLoc Loc;
    sPoolString Name;
    sModule *Module;
    int Value;
};

struct sVariant
{
    sScannerLoc Loc;
    sPoolString Name;
    int Mask;
    bool Bool;
    sPoolArray<sVariantMember *> Members;
};

/****************************************************************************/

} // namespace Asl

/****************************************************************************/
/***                                                                      ***/
/***   Interface                                                          ***/
/***                                                                      ***/
/****************************************************************************/

using namespace Asl;

class sTree
{
public:
    sMemoryPool Pool;
private:
    sTextLog *tb;
    sScanner *Scan;

    void AddBaseType(sTypeKind tk,const char *name);
    sType *AddType(sTypeKind tk,const char *name);
    sType *AddType(sType *type,const char *name);
    void AddIntrinsic(const char *name,int argcount,int typemask,int flags);
    void AddTexSamIntrinsics(sType *type,const char *floatdim,const char *intdim);
    void AddRawBufferIntrinsics(sType *type);
    
    void SyncLoc(const sScannerLoc &loc);
    void _DumpType(sType *,bool full);
    void _DumpValue(sExpr *);
    void _DumpExpr(sExpr *);
    void _DumpStmt(sStmt *,int indent);
    void _DumpStorage(const sStorage &store);
    void _DumpSemantic(sSemantic sem);
    void _DumpMember(sMember *,int indent,sType *base);
    void _DumpDecl(sStorage &store,sType *type,sPoolString name);
    void _DumpVariable(sVariable *);
    void _DumpFunc(sFunction *);
    void _DumpTypeDef(sTypeDef *);

    const char *DumpSemanticNames[sTAR_Max][sSEM_Max];

public:
    sTree();
    ~sTree();

    void Reset(bool defaults = true);
    void SetScan(sScanner *scan) { Scan = scan; }
    bool CppLine;

    // Construct objects

    sType *MakeType(sTypeKind kind);
    sType *MakeObject(sTypeKind objtype,sType *basetype);
    sType *MakeVector(sType *,int count);
    sType *MakeMatrix(sType *,int col,int row);
    sType *MakeArray(sType *,sExpr *expr);

    sType *MakeVector(sTypeKind kind,int count);
    sType *MakeMatrix(sTypeKind kind,int col,int row);
    sType *MakeArray(sTypeKind kind,int count);

    sType *SwitchBaseType(sType *type,sTypeKind newkind);

    sMember *MakeMember(sType *memtype,sPoolString name,const sStorage &storage,sSemantic sem=sSEM_None);
    sMember *MakeMember(sMember *memtype,sPoolString name);
    sMember *AddMember(sType *str,sType *memtype,sPoolString name,const sStorage &storage,sSemantic sem=sSEM_None);

    sVariable *MakeVar(sType *type,sPoolString name,const sStorage &storage,bool local,sSemantic sem);
    sTypeDef *MakeTypeDef(sType *type);

    sLiteral *MakeLiteral(sType *type);

    sIntrinsic *MakeIntrinsic(const char *name,int argcount,int typemask,int flags);
    sExpr *MakeExpr(sExprKind op,sExpr *a=0,sExpr *b=0);
    sExpr *MakeExprInt(int n);
    sExpr *MakeExprFloat(float f);
    void ExpandVector(sExpr *&s,sExpr *v);
    void ExpandVector(sExpr *&s,sType *t);
    void ConvertIntLit(sExpr *expr,sType *type);

    sStmt *MakeStmt(sStatementKind st);
    sFunction *MakeFunction();

    sExpr *MakeTemp(sStmt *Root,sStmt *before,sExpr *expr);
    static uint MakeSwizzle(int start,int size);

    sInject *MakeInject();
    sSniplet *MakeSniplet();
    sModule *MakeModule();
    sVariant *MakeVariant();
    sVariantMember *MakeVariantMember();

    // State

    sPoolArray<sType *> Types;              // all types, internal & external
    sPoolArray<sTypeDef *> TypeDefs;        // types that are declared by user
    sPoolArray<sVariable *> Vars;           // global variables declared by user
    sPoolArray<sFunction *> Funcs;          // global functions declared by user
    sPoolArray<sIntrinsic *> Intrinsics;    // intrinsic functions
    sPoolArray<sInject *> Injects;
    sPoolArray<sModule *> Modules;
    sPoolArray<sVariant *> Variants;
    sModule *CurrentModule;                 // set while compiling a module
    sExpr *ErrorExpr;
    sType *ErrorType;
    int CurrentShaders;
    sTarget DumpTarget;
    bool DumpTargetGl;
    bool DumpTargetGl1;
    int UsedShaders;                        // bitmask of shaders defined here
    int TempIndex;                          // used to create temporary variables
    int VariantBit;
    int LineOffset;

    // 

    void BasicTypes();
    sType *FindType(sPoolString name);
    sModule *FindModule(sPoolString name);
    sInject *FindInject(sPoolString name);
    sTypeDef *FindTypeDef(sPoolString name);
    sMember *FindMember(sPoolString name,sType *type);
    sTree *CopyTo();
    void CopyFrom(sTree *x);

    int ConstIntExpr(sExpr *expr,sPoolArray<sCondition> *conditions);

    void DeepTag(sType *x);
    void DeepTag(sMember *x);
    void DeepTag(sVariable *x);
    void DeepTag(sIntrinsic *x);
    void DeepTag(sStmt *x);
    void DeepTag(sExpr *x);
    void DeepTag(sLiteral *x);
    void DeepTag(sModule *x);

    sType *DeepCopy(sType *x);
    sMember *DeepCopy(sMember *x);
    sVariable *DeepCopy(sVariable *x);
    sIntrinsic *DeepCopy(sIntrinsic *x);
    sInject *DeepCopy(sInject *x);
    sStmt *DeepCopy(sStmt *x);
    sExpr *DeepCopy(sExpr *x);
    sLiteral *DeepCopy(sLiteral *x);
    sModule *DeepCopy(sModule *x);

    void CloneTag(sStmt *x);
    void CloneTag(sExpr *x);
    sStmt *CloneCopy(sStmt *x);
    sExpr *CloneCopy(sExpr *x);
    sExpr *CloneCopyExpr(sExpr *x);

    void Dump(sTextLog &tb,int shaders,sTarget target);

    // traverse expression

    template <typename Pred> void TraverseExpr(sExpr **e,Pred pred) 
    { 
        pred(e);
        for(int i=0;i<(*e)->Args.GetCount();i++) TraverseExpr(&(*e)->Args[i],pred);
    }
    template <typename Pred> void TraverseExpr(sStmt *s,Pred pred) 
    { 
        if(s->Expr) TraverseExpr(&s->Expr,pred);
        if(s->IncExpr) TraverseExpr(&s->IncExpr,pred);
        for(auto c : s->Block) TraverseExpr(c,pred);
        for(auto v : s->Vars) if(v->Initializer) TraverseExpr(&v->Initializer,pred);
    }
    template <typename Pred> void TraverseExpr(Pred pred) 
    { 
        for(auto f : Funcs) if(f->Root) TraverseExpr(f->Root,pred); 
        for(auto v : Vars) if(v->Initializer) TraverseExpr(&v->Initializer,pred);
    }

    // traverse expression with statement 

    template <typename Pred> void TraverseExprWithStmt(sStmt *root,sStmt *s,sExpr **e,Pred pred) 
    { 
        pred(e,root,s);
        for(int i=0;i<(*e)->Args.GetCount();i++)
            TraverseExprWithStmt(root,s,&(*e)->Args[i],pred);
    }
    template <typename Pred> void TraverseExprWithStmt(sStmt *root,sStmt *s,Pred pred) 
    { 
        if(s->Expr) TraverseExprWithStmt(root,s,&s->Expr,pred);
        if(s->IncExpr) TraverseExprWithStmt(root,s,&s->IncExpr,pred);
        for(auto c : s->Block) TraverseExprWithStmt(s,c,pred);
    }
    template <typename Pred> void TraverseExprWithStmt(Pred pred) 
    { 
        for(auto f : Funcs) 
        {
            if(f->Root)
                for(auto s : f->Root->Block)
                    TraverseExprWithStmt(f->Root,s,pred); 
        }
    }

    // Traverse Statements

    template <typename Pred> void TraverseStmt(sStmt *s,Pred pred) 
    { 
        pred(s);
        for(auto c : s->Block) TraverseStmt(c,pred);
    }
    template <typename Pred> void TraverseStmt(Pred pred) 
    { 
        for(auto f : Funcs) if(f->Root) TraverseStmt(f->Root,pred);
    }

    // Traverse Variable Declarations

    template <typename Pred> void TraverseVar(sStmt *s,Pred pred) 
    { 
        for(auto v : s->Vars) pred(s->Vars,v);
        for(auto c : s->Block) TraverseVar(c,pred);
    }
    template <typename Pred> void TraverseVar(Pred pred) 
    { 
        for(auto v : Vars) pred(Vars,v);
        for(auto f : Funcs) if(f->Root) TraverseVar(f->Root,pred); 
    }

    // Traverse Member Declarations

    template <typename Pred> void TraverseMember(Pred pred) 
    { 
        for(auto t : Types) 
            for(auto m : t->Members)
                pred(t->Members,m);
    }
};



/****************************************************************************/
} // namespace Altona2
#endif  // FILE_ALTONA2_LIBS_ASL_TREE_HPP

