/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Tree.hpp"

using namespace Altona2;
using namespace Asl;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/


int sType::GetSize(sTree *tree)
{
    switch(Kind)
    {
    case sTK_Float:
    case sTK_Int:
    case sTK_Uint:
    case sTK_Bool:      
        return 1;

    case sTK_Double:    
        return 2;

    case sTK_Array:     
        return Base->GetSize(tree) * tree->ConstIntExpr(SizeArray,0);

    case sTK_Vector:    
        return Base->GetSize(tree) * SizeX;

    case sTK_Matrix:    
        return Base->GetSize(tree) * SizeX * SizeY;

    case sTK_Struct:
    case sTK_CBuffer:
    case sTK_TBuffer:
        return StructSize;

    default:
        return 0;
    }
}


int sType::GetVectorSize(sTree *tree)
{
    switch(Kind)
    {
    case sTK_Float:
    case sTK_Int:
    case sTK_Uint:
    case sTK_Bool:      
        return 1;

    case sTK_Double:    
        return 2;

    case sTK_Array:     
        return 4;                   // this is larger than a vector!

    case sTK_Vector:    
        return Base->GetSize(tree) * SizeX;

    case sTK_Matrix:    
        return Base->GetSize(tree) * SizeX * SizeY;

    case sTK_Struct:
    case sTK_CBuffer:
    case sTK_TBuffer:
        return StructSize;

    default:
        return 0;
    }
}


bool sType::IsBaseType()
{
    switch(Kind)
    {
    case sTK_Int:
    case sTK_Uint:
    case sTK_Float:
    case sTK_Double:
    case sTK_Bool:
        return true;
    default:
        return false;
    }
}

bool sType::CheckBaseType(int allowedTypes)
{
    auto t = this;
    while(t->Base)
        t = t->Base;
    if(t->Kind==sTK_Int     ) return (allowedTypes & sBT_Int)!=0;
    if(t->Kind==sTK_Uint    ) return (allowedTypes & sBT_Uint)!=0;
    if(t->Kind==sTK_Float   ) return (allowedTypes & sBT_Float)!=0;
    if(t->Kind==sTK_Double  ) return (allowedTypes & sBT_Double)!=0;
    if(t->Kind==sTK_Bool    ) return (allowedTypes & sBT_Bool)!=0;
    return false;
}

bool sType::Compatible(sType *b)
{
    sType *a = this;

    if(a==0 || b==0)
        return false;

    if(a==b)
        return true;
    if(a->IsBaseType() && a->Kind==b->Kind)
        return true;
    if(a->Kind==sTK_Vector || a->Kind==sTK_Matrix || a->Kind==sTK_Array)
        return a->Base->Compatible(b->Base) && a->SizeX==b->SizeX && a->SizeY==b->SizeY;
    return false;
}

bool sType::IsSame(sType *b)
{
    sType *a = this;
    if(a==0 || b==0)
        return false;
    if(a->Kind!=b->Kind)
        return false;
    if(a->SizeX!=b->SizeX)
        return false;
    if(a->SizeY!=b->SizeY)
        return false;
    if(a->Semantic!=b->Semantic)
        return false;

    // can't handle this:

    if(!a->Members.IsEmpty() || !b->Members.IsEmpty())
        return false;
    if(a->SizeArray || b->SizeArray)
        return false;

    // finish it

    if(a->Base || b->Base)
        return a->Base->IsSame(b->Base);
    return true;
}

void sType::AssignOffsets(sTree *tree,int startoffset)
{
    int offset = 0;

    for(auto m : Members)
    {
        if(m->Active)
        {
            int size = m->Type->GetSize(tree);

            if(m->Align)
            {
                offset = sAlign(offset,4);
            }
            else
            {
                int r0 = offset/4;
                int r1 = (offset+size-1)/4;
                if(r0!=r1)
                    offset = sAlign(offset,4);
            }
            m->Offset = offset + startoffset;
            offset += size;
        }
    }
    StructSize = sAlign(offset,4);
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sType *sTree::MakeType(sTypeKind kind)
{
    sType *x = Pool.Alloc<sType>();
    sClear(*x);
    if(Scan) x->Loc = Scan->GetLoc();
    x->Name.Clear();
    x->Members.SetPool(&Pool);
    x->Intrinsics.SetPool(&Pool);
    x->Kind = kind;
    x->Semantic = sSEM_None;
    return x;
}

sType *sTree::MakeObject(sTypeKind objtype,sType *basetype)
{
    sType *x = MakeType(objtype);
    x->Base = basetype;
    x->Semantic = sSEM_None;
    return x;
}

sType *sTree::MakeVector(sType *basetype,int count)
{
    sType *x = MakeType(sTK_Vector);
    x->Base = basetype;
    x->SizeX = count;
    x->Semantic = sSEM_None;
    return x;
}

sType *sTree::MakeMatrix(sType *basetype,int col,int row)
{
    sType *x = MakeType(sTK_Matrix);
    x->Base = basetype;
    x->SizeX = col;
    x->SizeY = row;
    x->Semantic = sSEM_None;
    return x;
}

sType *sTree::MakeArray(sType *basetype,sExpr *expr)
{
    sType *x = MakeType(sTK_Array);
    x->Base = basetype;
    x->SizeArray = expr;
    x->Semantic = sSEM_None;
    return x;
}

sType *sTree::MakeVector(sTypeKind kind,int count)
{
    sType *x = MakeType(sTK_Vector);
    x->Base = MakeType(kind);
    x->SizeX = count;
    x->Semantic = sSEM_None;
    return x;
}

sType *sTree::MakeMatrix(sTypeKind kind,int col,int row)
{
    sType *x = MakeType(sTK_Matrix);
    x->Base = MakeType(kind);
    x->SizeX = col;
    x->SizeY = row;
    x->Semantic = sSEM_None;
    return x;
}

sType *sTree::MakeArray(sTypeKind kind,int count)
{
    sType *x = MakeType(sTK_Array);
    x->Base = MakeType(kind);
    x->SizeX = count;
    x->Semantic = sSEM_None;
    return x;
}

sType *sTree::SwitchBaseType(sType *type,sTypeKind newkind)
{
    if(type->Kind==sTK_Matrix)
        return MakeMatrix(newkind,type->SizeX,type->SizeY);
    if(type->Kind==sTK_Vector)
        return MakeVector(newkind,type->SizeX);
    return MakeType(newkind);
}

sMember *sTree::AddMember(sType *str,sType *memtype,sPoolString name,const sStorage &store,sSemantic sem)
{

    if(str->Members.Find([&](const sMember *m){ return m->Name==name && (m->Module==CurrentModule || (m->Storage.Flags & sST_Export) || (int(store.Flags) & sST_Export)); }))
        Scan->Error("member %q declared twice",name);

    sMember *x = Pool.Alloc<sMember>();
    sClear(*x);
    if(Scan) x->Loc = Scan->GetLoc();
    x->Type = memtype;
    x->Name = name;
    x->Storage = store;
    x->Semantic = sem;

    if(memtype->Kind==sTK_Matrix && !(store.Flags & ~(sST_RowMajor|sST_ColumnMajor)))
        x->Storage.Flags |= sST_RowMajor;

    str->Members.Add(x);
    return x;
}

sMember *sTree::MakeMember(sType *memtype,sPoolString name,const sStorage &store,sSemantic sem)
{
    sMember *x = Pool.Alloc<sMember>();
    sClear(*x);
    if(Scan) x->Loc = Scan->GetLoc();
    x->Type = memtype;
    x->Name = name;
    x->Storage = store;
    x->Semantic = sem;
    x->Active = true;

    if(memtype->Kind==sTK_Matrix && !(store.Flags & ~(sST_RowMajor|sST_ColumnMajor)))
        x->Storage.Flags |= sST_RowMajor;

    return x;
}

sMember *sTree::MakeMember(sMember *oldmem,sPoolString name)
{
    sASSERT(oldmem->ReplaceExpr==0);
    sASSERT(oldmem->Vector[0]==0);

    sMember *x = Pool.Alloc<sMember>();
    *x = *oldmem;
    x->Name = name;
    return x;
}

sVariable *sTree::MakeVar(sType *type,sPoolString name,const sStorage &store,bool local,sSemantic sem)
{
    sVariable *x = Pool.Alloc<sVariable>();
    sClear(*x);
    if(Scan) x->Loc = Scan->GetLoc();
    x->Type = type;
    x->Name = name;
    x->Storage = store;
    x->Semantic = sem;
    x->Shaders = CurrentShaders;

    if(type->Kind==sTK_Matrix && !(store.Flags & ~(sST_RowMajor|sST_ColumnMajor)) && !local)
        x->Storage.Flags |= sST_RowMajor;

    return x;
}

sTypeDef *sTree::MakeTypeDef(sType *type)
{
    sTypeDef *x = Pool.Alloc<sTypeDef>();
    if(Scan) x->Loc = Scan->GetLoc();
    x->Type = type;
    x->Shaders = CurrentShaders;
    return x;
}

sLiteral *sTree::MakeLiteral(sType *type)
{
    sLiteral *x = Pool.Alloc<sLiteral>();
    sClear(*x);
    if(Scan) x->Loc = Scan->GetLoc();
    x->Type = type;
    x->Elements = type->GetSize(this);
    x->DataU = Pool.Alloc<uint>(x->Elements);
    return x;
}

sExpr *sTree::MakeExpr(sExprKind op,sExpr *a,sExpr *b)
{
    sExpr *x = Pool.Alloc<sExpr>();
    sClear(*x);
    if(Scan) x->Loc = Scan->GetLoc();
    x->Args.SetPool(&Pool);
    x->Kind = op;
    x->Type = a && a->Type ? a->Type : ErrorType;
    x->IntrinsicReplacementName.Clear();
    if(a) x->Args.Add(a);
    if(b) x->Args.Add(b);
    return x;
}

sExpr *sTree::MakeExprInt(int n)
{
    sExpr *x = MakeExpr(sEX_Literal);
    x->Type = FindType("int");
    x->Literal = MakeLiteral(x->Type);
    x->Literal->DataI[0] = n;
    return x;
}

sExpr *sTree::MakeExprFloat(float f)
{
    sExpr *x = MakeExpr(sEX_Literal);
    x->Type = FindType("float");
    x->Literal = MakeLiteral(x->Type);
    x->Literal->DataF[0] = f;
    return x;
}

void sTree::ExpandVector(sExpr *&s,sExpr *v)
{
    ExpandVector(s,v->Type);
    /*
    if(s->Type->IsBaseType() && v->Type->Kind==sTK_Vector)
    {
        int sw[5] = { 0,0,0x0101,0x010101,0x01010101 };
        auto e = MakeExpr(sEX_Swizzle,s);
        e->Swizzle = sw[v->Type->SizeX];
        e->Type = MakeVector(s->Type->Kind,v->Type->SizeX);
        s = e;
    }
    */
}

void sTree::ExpandVector(sExpr *&s,sType *t)
{
    if(s->Type->IsBaseType() && t->Kind==sTK_Vector)
    {
        ConvertIntLit(s,t->Base);
        int sw[5] = { 0,0,0x0101,0x010101,0x01010101 };
        auto e = MakeExpr(sEX_SwizzleExpand,s);
        e->Swizzle = sw[t->SizeX];
        e->Type = MakeVector(s->Type->Kind,t->SizeX);
        s = e;
    }
    else
    {
        ConvertIntLit(s,t);
    }
}

void sTree::ConvertIntLit(sExpr *expr,sType *type)
{
    if(expr->Kind == sEX_Literal && expr->Literal->Type->Kind==sTK_Int)
    {
        if(type->Kind == sTK_Uint)
        {
            uint v = expr->Literal->DataU[0];
            expr->Literal = MakeLiteral(type);
            expr->Literal->DataU[0] = v;
            expr->Type = type;
        }
        if(type->Kind == sTK_Float)
        {
            int v = expr->Literal->DataI[0];
            expr->Literal = MakeLiteral(type);
            expr->Literal->DataF[0] = float(v);
            expr->Type = type;
        }
    }
}

sStmt *sTree::MakeStmt(sStatementKind st)
{
    sStmt *x = Pool.Alloc<sStmt>();
    sClear(*x);
    if(Scan) x->Loc = Scan->GetLoc();
    x->Kind = st;
    x->Block.SetPool(&Pool);
    x->Vars.SetPool(&Pool);
    return x;
}

sFunction *sTree::MakeFunction()
{
    sFunction *x = Pool.Alloc<sFunction>();
    sClear(*x);
    if(Scan) x->Loc = Scan->GetLoc();
    x->Name = "";
    x->Shaders = CurrentShaders;
    return x;
}

sExpr *sTree::MakeTemp(sStmt *root,sStmt *before,sExpr *expr)
{
    Scan->SetLoc(before->Loc);
    // make declaration

    sString<64> name;
    name.PrintF("_%d",TempIndex++);
    auto *var = MakeVar(expr->Type,sPoolString(name),sStorage(),true,sSEM_None);
    auto *s = MakeStmt(sST_Decl);
    s->Vars.Add(var);
    s->Expr = expr;

    // link to block

    root->Vars.Add(var);
    int index = root->Block.FindEqualIndex(before);
    sASSERT(index>=0);
    root->Block.AddAt(s,index);

    // make result expression

    auto e = MakeExpr(sEX_Variable);
    e->Variable = var;
    e->Type = var->Type;
    return e;
}

uint sTree::MakeSwizzle(int start,int size)
{
    uint s = 0;
    for(int i=0;i<size;i++)
        s |= (start+i+1) << (i*8);
    return s;
}

sInject *sTree::MakeInject()
{
    sInject *x = Pool.Alloc<sInject>();
    sClear(*x);
    if(Scan) x->Loc = Scan->GetLoc();
    x->Members.SetPool(&Pool);
    return x;
}

sSniplet *sTree::MakeSniplet()
{
    sSniplet *x = Pool.Alloc<sSniplet>();
    sClear(*x);

    return x;
}

sModule *sTree::MakeModule()
{
    sModule *x = Pool.Alloc<sModule>();
    sClear(*x);
    if(Scan) x->Loc = Scan->GetLoc();
    x->Sniplets.SetPool(&Pool);
    x->Depends.SetPool(&Pool);
    x->Conditions.SetPool(&Pool);
    x->Vars.SetPool(&Pool);

    return x;
}

sVariant *sTree::MakeVariant()
{
    sVariant *x = Pool.Alloc<sVariant>();
    sClear(*x);
    if(Scan) x->Loc = Scan->GetLoc();
    x->Name.Clear();
    x->Members.SetPool(&Pool);

    return x;
}

sVariantMember *sTree::MakeVariantMember()
{
    sVariantMember *x = Pool.Alloc<sVariantMember>();
    sClear(*x);
    if(Scan) x->Loc = Scan->GetLoc();
    x->Name.Clear();

    return x;
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

sTree::sTree()
{
    tb = 0;
    Scan = 0;
    CppLine = true;

    Types.SetPool(&Pool);
    TypeDefs.SetPool(&Pool);
    Funcs.SetPool(&Pool);
    Intrinsics.SetPool(&Pool);
    Vars.SetPool(&Pool);
    Injects.SetPool(&Pool);
    Modules.SetPool(&Pool);
    Variants.SetPool(&Pool);

    CurrentShaders = 0;
    UsedShaders = 0;
    VariantBit = 0;
    DumpTarget = sTAR_Max;
    LineOffset = 0;

    Reset(0);

    sClear(DumpSemanticNames);

    // Asl  (my own shit)

    DumpSemanticNames[sTAR_Asl][sSEM_Position] = "Position";
    DumpSemanticNames[sTAR_Asl][sSEM_Normal] = "Normal";
    DumpSemanticNames[sTAR_Asl][sSEM_Tangent] = "Tangent";
    DumpSemanticNames[sTAR_Asl][sSEM_Binormal] = "Binormal";
    DumpSemanticNames[sTAR_Asl][sSEM_BlendIndex] = "BlendIndex";
    DumpSemanticNames[sTAR_Asl][sSEM_BlendWeight] = "BlendWeight";
    DumpSemanticNames[sTAR_Asl][sSEM_Color] = "Color%d";
    DumpSemanticNames[sTAR_Asl][sSEM_Tex] = "Tex%d";
    DumpSemanticNames[sTAR_Asl][sSEM_InstanceId] = "InstanceId";
    DumpSemanticNames[sTAR_Asl][sSEM_VertexId] = "VertexId";
    DumpSemanticNames[sTAR_Asl][sSEM_PositionT] = "PositionT";
    DumpSemanticNames[sTAR_Asl][sSEM_VFace] = "VFace";
    DumpSemanticNames[sTAR_Asl][sSEM_VPos] = "VPos";
    DumpSemanticNames[sTAR_Asl][sSEM_Target] = "Target%d";
    DumpSemanticNames[sTAR_Asl][sSEM_Depth] = "Depth";
    DumpSemanticNames[sTAR_Asl][sSEM_Coverage] = "Coverage";
    DumpSemanticNames[sTAR_Asl][sSEM_IsFrontFace] = "IsFrontFace";
    DumpSemanticNames[sTAR_Asl][sSEM_SampleIndex] = "SampleIndex";
    DumpSemanticNames[sTAR_Asl][sSEM_PrimitiveId] = "PrimitiveId";
    DumpSemanticNames[sTAR_Asl][sSEM_GSInstanceId] = "GSInstanceId";
    DumpSemanticNames[sTAR_Asl][sSEM_RenderTargetArrayIndex] = "RenderTargetArrayIndex";
    DumpSemanticNames[sTAR_Asl][sSEM_ViewportArrayIndex] = "ViewportArrayIndex";
    DumpSemanticNames[sTAR_Asl][sSEM_DomainLocation] = "DomainLocation";
    DumpSemanticNames[sTAR_Asl][sSEM_InsideTessFactor] = "InsideTessFactor";
    DumpSemanticNames[sTAR_Asl][sSEM_OutputControlPointId] = "OutputControlPointId";
    DumpSemanticNames[sTAR_Asl][sSEM_TessFactor] = "TessFactor";
    DumpSemanticNames[sTAR_Asl][sSEM_DispatchThreadId] = "DispatchThreadId";
    DumpSemanticNames[sTAR_Asl][sSEM_GroupId] = "GroupId";
    DumpSemanticNames[sTAR_Asl][sSEM_GroupIndex] = "GroupIndex";
    DumpSemanticNames[sTAR_Asl][sSEM_GroupThreadId] = "GroupThreadId";
    DumpSemanticNames[sTAR_Asl][sSEM_Vs] = "Vs%d";
    DumpSemanticNames[sTAR_Asl][sSEM_Hs] = "Hs%d";
    DumpSemanticNames[sTAR_Asl][sSEM_Ds] = "Ds%d";
    DumpSemanticNames[sTAR_Asl][sSEM_Gs] = "Gs%d";
    DumpSemanticNames[sTAR_Asl][sSEM_Ps] = "Ps%d";
    DumpSemanticNames[sTAR_Asl][sSEM_Cs] = "Cs%d";
    DumpSemanticNames[sTAR_Asl][sSEM_Slot] = "Slot%d";
    DumpSemanticNames[sTAR_Asl][sSEM_SlotAuto] = "SlotAuto";
    DumpSemanticNames[sTAR_Asl][sSEM_Uav] = "Uav";

    // HLSL5 (DX11)

    DumpSemanticNames[sTAR_Hlsl5][sSEM_Position] = "POSITION";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_Normal] = "NORMAL";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_Tangent] = "TANGENT";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_Binormal] = "BINORMAL";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_BlendIndex] = "BLENDINDICES";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_BlendWeight] = "BLENDWEIGHT";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_Color] = "COLOR%d";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_Tex] = "TEXCOORD%d";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_InstanceId] = "SV_InstanceId";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_VertexId] = "SV_VertexId";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_PositionT] = "SV_Position";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_VFace] = "SV_VFace";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_VPos] = "SV_VPos";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_Target] = "SV_Target%d";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_Depth] = "SV_Depth";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_Coverage] = "SV_Coverage";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_IsFrontFace] = "SV_IsFrontFace";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_SampleIndex] = "SV_SampleIndex";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_PrimitiveId] = "SV_PrimitiveId";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_GSInstanceId] = "SV_GSInstanceId";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_RenderTargetArrayIndex] = "SV_RenderTargetArrayIndex";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_ViewportArrayIndex] = "SV_ViewportArrayIndex";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_DomainLocation] = "SV_DomainLocation";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_InsideTessFactor] = "SV_InsideTessFactor";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_OutputControlPointId] = "SV_OutputControlPointId";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_TessFactor] = "SV_TessFactor";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_DispatchThreadId] = "SV_DispatchThreadId";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_GroupId] = "SV_GroupId";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_GroupIndex] = "SV_GroupIndex";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_GroupThreadId] = "SV_GroupThreadId";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_Vs] = "register (b%d)";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_Hs] = "register (b%d)";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_Ds] = "register (b%d)";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_Gs] = "register (b%d)";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_Ps] = "register (b%d)";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_Cs] = "register (b%d)";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_Sampler] = "register (s%d)";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_Texture] = "register (t%d)";
    DumpSemanticNames[sTAR_Hlsl5][sSEM_Uav] = "register (u%d)";


    // HLSL3 (DX9)

    DumpSemanticNames[sTAR_Hlsl3][sSEM_Position] = "POSITION";
    DumpSemanticNames[sTAR_Hlsl3][sSEM_Normal] = "NORMAL";
    DumpSemanticNames[sTAR_Hlsl3][sSEM_Tangent] = "TANGENT";
    DumpSemanticNames[sTAR_Hlsl3][sSEM_Binormal] = "BINORMAL";
    DumpSemanticNames[sTAR_Hlsl3][sSEM_BlendIndex] = "BLENDINDEX";
    DumpSemanticNames[sTAR_Hlsl3][sSEM_BlendWeight] = "BLENDWEIGHT";
    DumpSemanticNames[sTAR_Hlsl3][sSEM_Color] = "COLOR%d";
    DumpSemanticNames[sTAR_Hlsl3][sSEM_Tex] = "TEXCOORD%d";
    DumpSemanticNames[sTAR_Hlsl3][sSEM_PositionT] = "POSITION";
    DumpSemanticNames[sTAR_Hlsl3][sSEM_VFace] = "VFACE";
    DumpSemanticNames[sTAR_Hlsl3][sSEM_VPos] = "VPOS";
    DumpSemanticNames[sTAR_Hlsl3][sSEM_Target] = "COLOR%d";
    DumpSemanticNames[sTAR_Hlsl3][sSEM_Depth] = "DEPTH";
    DumpSemanticNames[sTAR_Hlsl3][sSEM_Vs] = "register (c%d)";
    DumpSemanticNames[sTAR_Hlsl3][sSEM_Hs] = "register (c%d)";
    DumpSemanticNames[sTAR_Hlsl3][sSEM_Ds] = "register (c%d)";
    DumpSemanticNames[sTAR_Hlsl3][sSEM_Gs] = "register (c%d)";
    DumpSemanticNames[sTAR_Hlsl3][sSEM_Ps] = "register (c%d)";
    DumpSemanticNames[sTAR_Hlsl3][sSEM_Cs] = "register (c%d)";
    DumpSemanticNames[sTAR_Hlsl3][sSEM_Sampler] = "register (s%d)";
    DumpSemanticNames[sTAR_Hlsl3][sSEM_Const] = "register (c%d)";
}

sTree::~sTree()
{
}

void sTree::Reset(bool defaults)
{
    Types.Reset();
    TypeDefs.Reset();
    Vars.Reset();
    Funcs.Reset();
    Intrinsics.Reset();
    Injects.Reset();
    Modules.Reset();
    Variants.Reset();
    ErrorExpr = 0;
    ErrorType = 0;

    Pool.Reset();
    CurrentShaders = 0;
    CurrentModule = 0;
    UsedShaders = 0;
    TempIndex = 0;

    if(!defaults)
        return;

    sType *type;

    ErrorType = MakeType(sTK_Error);
    ErrorExpr = MakeExpr(sEX_Error);

    AddBaseType(sTK_Int,"int");
    AddBaseType(sTK_Uint,"uint");
    AddBaseType(sTK_Float,"float");
    AddBaseType(sTK_Double,"double");
    AddBaseType(sTK_Bool,"bool");

    AddType(sTK_Void,"void");
    AddType(sTK_Sampler,"Sampler");

    // Asl

    type = AddType(sTK_TexSam1D,"TexSam1D");
    AddTexSamIntrinsics(type,"float","int");
    type = AddType(sTK_TexSam2D,"TexSam2D");
    AddTexSamIntrinsics(type,"float2","int2");
    type = AddType(sTK_TexSam3D,"TexSam3D");
    AddTexSamIntrinsics(type,"float3","int3");
    type = AddType(sTK_TexSamCube,"TexSamCube");
    AddTexSamIntrinsics(type,"float3","int3");
    type = AddType(sTK_TexCmp1D,"TexCmp1D");
    AddTexSamIntrinsics(type,"float","int");
    type = AddType(sTK_TexCmp2D,"TexCmp2D");
    AddTexSamIntrinsics(type,"float2","int2");
    type = AddType(sTK_TexCmp3D,"TexCmp3D");
    AddTexSamIntrinsics(type,"float3","int3");
    type = AddType(sTK_TexCmpCube,"TexCmpCube");
    AddTexSamIntrinsics(type,"float3","int3");

    type = AddType(sTK_Buffer,"Buffer"); type->HasTemplate = true;
    type = AddType(sTK_ByteBuffer,"RawBuffer");
    AddRawBufferIntrinsics(type);
    type = AddType(sTK_StructBuffer,"StructuredBuffer"); type->HasTemplate = true;
    type = AddType(sTK_RWBuffer,"RWBuffer"); type->HasTemplate = true;
    type = AddType(sTK_RWByteBuffer,"RWRawBuffer");
    AddRawBufferIntrinsics(type);
    type = AddType(sTK_RWStructBuffer,"RWStructuredBuffer"); type->HasTemplate = true;
    type = AddType(sTK_AppendBuffer,"AppendBuffer"); type->HasTemplate = true;
    type = AddType(sTK_ConsumeBuffer,"ConsumeBuffer"); type->HasTemplate = true;

    // simple match

    AddIntrinsic("abs"          ,1,sBT_Float|sBT_Int|sBT_All,0);
    AddIntrinsic("max"          ,2,sBT_Float|sBT_Int|sBT_All|sBT_VectorPromote,0);
    AddIntrinsic("min"          ,2,sBT_Float|sBT_Int|sBT_All|sBT_VectorPromote,0);
    AddIntrinsic("sign"         ,1,sBT_Float|sBT_Int|sBT_All,0);
    AddIntrinsic("clamp"        ,3,sBT_Float|sBT_Int|sBT_All|sBT_VectorPromote,0);

    AddIntrinsic("ceil"         ,1,sBT_Float|sBT_All,0);
    AddIntrinsic("clip"         ,1,sBT_Float|sBT_All,0);
    AddIntrinsic("floor"        ,1,sBT_Float|sBT_All,0);
    AddIntrinsic("frac"         ,1,sBT_Float|sBT_All,0);
    AddIntrinsic("trunc"        ,1,sBT_Float|sBT_All,0);

    AddIntrinsic("saturate"     ,1,sBT_Float|sBT_All,0);
    AddIntrinsic("smoothstep"   ,3,sBT_Float|sBT_All,0);
    AddIntrinsic("step"         ,2,sBT_Float|sBT_All,0);

    AddIntrinsic("modf"         ,2,sBT_Float|sBT_All,0);
    AddIntrinsic("fmod"         ,2,sBT_Float|sBT_All,0);

    // trigonometric

    AddIntrinsic("acos"         ,1,sBT_Float|sBT_All,0);
    AddIntrinsic("asin"         ,1,sBT_Float|sBT_All,0);
    AddIntrinsic("atan"         ,1,sBT_Float|sBT_All,0);
    AddIntrinsic("atan2"        ,2,sBT_Float|sBT_All,0);
    AddIntrinsic("cos"          ,1,sBT_Float|sBT_All,0);
    AddIntrinsic("cosh"         ,1,sBT_Float|sBT_All,0);
    AddIntrinsic("exp"          ,1,sBT_Float|sBT_All,0);
    AddIntrinsic("exp2"         ,1,sBT_Float|sBT_All,0);
    AddIntrinsic("log"          ,1,sBT_Float|sBT_All,0);
    AddIntrinsic("log10"        ,1,sBT_Float|sBT_All,0);
    AddIntrinsic("log2"         ,1,sBT_Float|sBT_All,0);
    AddIntrinsic("pow"          ,2,sBT_Float|sBT_All,0);
    AddIntrinsic("rcp"          ,1,sBT_Float|sBT_All,0);
    AddIntrinsic("rsqrt"        ,1,sBT_Float|sBT_All,0);
    AddIntrinsic("sin"          ,1,sBT_Float|sBT_All,0);
    AddIntrinsic("sincos"       ,1,sBT_Float|sBT_All,0);
    AddIntrinsic("sinh"         ,1,sBT_Float|sBT_All,0);
    AddIntrinsic("sqrt"         ,1,sBT_Float|sBT_All,0);
    AddIntrinsic("tan"          ,1,sBT_Float|sBT_All,0);
    AddIntrinsic("tanh"         ,1,sBT_Float|sBT_All,0);

    // vector

    AddIntrinsic("cross"        ,2,sBT_Float|sBT_Vector3,0);
    AddIntrinsic("distance"     ,2,sBT_Float|sBT_Vector,sTAF_ResultScalar);
    AddIntrinsic("dot"          ,2,sBT_Float|sBT_Vector,sTAF_ResultScalar);
    AddIntrinsic("length"       ,1,sBT_Float|sBT_Vector,sTAF_ResultScalar);
    AddIntrinsic("lerp"         ,3,sBT_Float|sBT_Vector,0);
    AddIntrinsic("normalize"    ,1,sBT_Float|sBT_Vector,0);
    AddIntrinsic("reflect"      ,2,sBT_Float|sBT_Vector,0);

    // matrix

    AddIntrinsic("determinant"  ,1,sBT_Float|sBT_SquareMatrix,sTAF_ResultScalar);
    AddIntrinsic("transpose"    ,1,sBT_Float|sBT_Int|sBT_Bool|sBT_SquareMatrix,0);

    // binary

    AddIntrinsic("countbits"    ,1,sBT_Uint|sBT_Scalar|sBT_Vector,sTAF_ResultScalar);
    AddIntrinsic("firstbithigh" ,1,sBT_Uint|sBT_All,0);
    AddIntrinsic("firstbitlow"  ,1,sBT_Uint|sBT_Int|sBT_Scalar|sBT_Vector,0);
    AddIntrinsic("reversebits"  ,1,sBT_Uint|sBT_Scalar|sBT_Vector,0);

    // conversion

    AddIntrinsic("asdouble"     ,1,sBT_Uint,sTAF_ArgAnyType);
    AddIntrinsic("asfloat"      ,1,sBT_Uint,sTAF_ArgAnyType);
    AddIntrinsic("asint"        ,1,sBT_Uint,sTAF_ArgAnyType);
    AddIntrinsic("asuint"       ,1,sBT_Uint,sTAF_ArgAnyType);



    // rendering

    AddIntrinsic("ddx"          ,1,sBT_Float|sBT_All,0);
    AddIntrinsic("ddx_coarse"   ,1,sBT_Float|sBT_Scalar|sBT_Vector,0);
    AddIntrinsic("ddx_fine"     ,1,sBT_Float|sBT_Scalar|sBT_Vector,0);
    AddIntrinsic("ddy"          ,1,sBT_Float|sBT_All,0);
    AddIntrinsic("ddy_coarse"   ,1,sBT_Float|sBT_Scalar|sBT_Vector,0);
    AddIntrinsic("ddy_fine"     ,1,sBT_Float|sBT_Scalar|sBT_Vector,0);
    AddIntrinsic("fwidth"       ,1,sBT_Float|sBT_Scalar|sBT_Vector,0);

    // compute

    AddIntrinsic("InterlockedAdd", 3,sBT_Uint,sTAF_ResultVoid);
    AddIntrinsic("InterlockedAnd", 3,sBT_Uint,sTAF_ResultVoid);
    AddIntrinsic("InterlockedMin", 3,sBT_Uint,sTAF_ResultVoid);
    AddIntrinsic("InterlockedMax", 3,sBT_Uint,sTAF_ResultVoid);
    AddIntrinsic("InterlockedOr",  3,sBT_Uint,sTAF_ResultVoid);
    AddIntrinsic("InterlockedXor", 3,sBT_Uint,sTAF_ResultVoid);

    AddIntrinsic("InterlockedCompareExchange", 4,sBT_Uint,sTAF_ResultVoid);
    AddIntrinsic("InterlockedCompareStore", 3,sBT_Uint,sTAF_ResultVoid);
    AddIntrinsic("InterlockedExchange", 4,sBT_Uint,sTAF_ResultVoid);

    AddIntrinsic("AllMemoryBarrier", 0,sBT_Uint,sTAF_ResultVoid);
    AddIntrinsic("GroupMemoryBarrier", 0,sBT_Uint,sTAF_ResultVoid);
    AddIntrinsic("DeviceMemoryBarrier", 0,sBT_Uint,sTAF_ResultVoid);
    AddIntrinsic("AllMemoryBarrierWithGroupSync", 0,sBT_Uint,sTAF_ResultVoid);
    AddIntrinsic("GroupMemoryBarrierWithGroupSync", 0,sBT_Uint,sTAF_ResultVoid);
    AddIntrinsic("DeviceMemoryBarrierWithGroupSync", 0,sBT_Uint,sTAF_ResultVoid);

    // other

    AddIntrinsic("mul"          ,2,sBT_Float|sBT_All,sTAF_MulSpecial);
}

void sTree::AddBaseType(sTypeKind tk,const char *name)
{
    auto type = MakeType(tk);
    AddType(tk,name);

    sString<128> str;
    for(int x=2;x<=4;x++)
    {
        str.PrintF("%s%d",name,x); 
        AddType(MakeVector(type,x),str);
        for(int y=2;y<=4;y++)
        {
            str.PrintF("%s%dx%d",name,y,x); 
            AddType(MakeMatrix(type,x,y),str);
        }
    }
}

sType *sTree::AddType(sTypeKind tk,const char *name)
{
    auto type = MakeType(tk);
    type->Name = name;
    Types.Add(type);
    return type;
}

sType *sTree::AddType(sType *type,const char *name)
{
    type->Name = name;
    Types.Add(type);
    return type;
}


sIntrinsic *sTree::MakeIntrinsic(const char *name,int argcount,int typemask,int flags)
{
    auto in = Pool.Alloc<sIntrinsic>();
    sClear(*in);
    if(Scan) in->Loc = Scan->GetLoc();
    in->Name = name;
    in->ArgCount = argcount;
    in->MinArgCount = argcount;
    in->TypeMask = typemask;
    in->Flags = flags;
    Intrinsics.Add(in);
    return in;
}

void sTree::AddIntrinsic(const char *name,int argcount,int typemask,int flags)
{
    auto in = MakeIntrinsic(name,argcount,typemask,flags);
    Intrinsics.Add(in);
}

void sTree::AddTexSamIntrinsics(sType *type,const char *floatdim,const char *intdim)
{
    sIntrinsic *in;

    in = MakeIntrinsic("Gather",2,0,sTAF_Hlsl5Sampler);
    in->MinArgCount = 1;
    in->ResultType = FindType("float4");
    in->ArgType[0] = FindType(floatdim);
    in->ArgType[1] = FindType(intdim);
    type->Intrinsics.Add(in);

    in = MakeIntrinsic("GatherRed",2,0,sTAF_Hlsl5Sampler);
    in->MinArgCount = 1;
    in->ResultType = FindType("float4");
    in->ArgType[0] = FindType(floatdim);
    in->ArgType[1] = FindType(intdim);
    type->Intrinsics.Add(in);

    in = MakeIntrinsic("GatherGreen",2,0,sTAF_Hlsl5Sampler);
    in->MinArgCount = 1;
    in->ResultType = FindType("float4");
    in->ArgType[0] = FindType(floatdim);
    in->ArgType[1] = FindType(intdim);
    type->Intrinsics.Add(in);

    in = MakeIntrinsic("GatherBlue",2,0,sTAF_Hlsl5Sampler);
    in->MinArgCount = 1;
    in->ResultType = FindType("float4");
    in->ArgType[0] = FindType(floatdim);
    in->ArgType[1] = FindType(intdim);
    type->Intrinsics.Add(in);

    in = MakeIntrinsic("GatherAlpha",2,0,sTAF_Hlsl5Sampler);
    in->MinArgCount = 1;
    in->ResultType = FindType("float4");
    in->ArgType[0] = FindType(floatdim);
    in->ArgType[1] = FindType(intdim);
    type->Intrinsics.Add(in);

    in = MakeIntrinsic("Load",3,0,sTAF_Hlsl5Sampler);
    in->MinArgCount = 1;
    in->ResultType = FindType("float4");
    in->ArgType[0] = FindType("int");
    in->ArgType[1] = FindType("int");
    in->ArgType[2] = FindType("int");
    type->Intrinsics.Add(in);

    in = MakeIntrinsic("Sample",2,0,sTAF_Hlsl5Sampler);
    in->MinArgCount = 1;
    in->ResultType = FindType("float4");
    in->ArgType[0] = FindType(floatdim);
    in->ArgType[1] = FindType(intdim);
    type->Intrinsics.Add(in);

    in = MakeIntrinsic("SampleBias",3,0,sTAF_Hlsl5Sampler);
    in->MinArgCount = 2;
    in->ResultType = FindType("float4");
    in->ArgType[0] = FindType(floatdim);
    in->ArgType[1] = FindType("float");
    in->ArgType[2] = FindType(intdim);
    type->Intrinsics.Add(in);
    
    in = MakeIntrinsic("SampleCmp",4,0,sTAF_Hlsl5Sampler);
    in->MinArgCount = 2;
    in->ResultType = FindType("float");
    in->ArgType[0] = FindType(floatdim);
    in->ArgType[1] = FindType("float");
    in->ArgType[2] = FindType(intdim);
    type->Intrinsics.Add(in);

    in = MakeIntrinsic("SampleCmpLevelZero",4,0,sTAF_Hlsl5Sampler);
    in->MinArgCount = 2;
    in->ResultType = FindType("float");
    in->ArgType[0] = FindType(floatdim);
    in->ArgType[1] = FindType("float");
    in->ArgType[2] = FindType(intdim);
    type->Intrinsics.Add(in);

    in = MakeIntrinsic("SampleGrad",4,0,sTAF_Hlsl5Sampler);
    in->MinArgCount = 3;
    in->ResultType = FindType("float4");
    in->ArgType[0] = FindType(floatdim);
    in->ArgType[1] = FindType("float");
    in->ArgType[2] = FindType("float");
    in->ArgType[3] = FindType(intdim);
    type->Intrinsics.Add(in);


    in = MakeIntrinsic("SampleLevel",3,0,sTAF_Hlsl5Sampler);
    in->MinArgCount = 2;
    in->ResultType = FindType("float4");
    in->ArgType[0] = FindType(floatdim);
    in->ArgType[2] = FindType("float");
    in->ArgType[3] = FindType(intdim);
    type->Intrinsics.Add(in);
}

void sTree::AddRawBufferIntrinsics(sType *type)
{
    sIntrinsic *in;

    in = MakeIntrinsic("Load"  ,1,sBT_Uint|sBT_Vector,0); type->Intrinsics.Add(in); in->ArgType[0] = FindType("uint");
    in = MakeIntrinsic("Load2" ,1,sBT_Uint|sBT_Vector,0); type->Intrinsics.Add(in); in->ArgType[0] = FindType("uint2");
    in = MakeIntrinsic("Load3" ,1,sBT_Uint|sBT_Vector,0); type->Intrinsics.Add(in); in->ArgType[0] = FindType("uint3");
    in = MakeIntrinsic("Load4" ,1,sBT_Uint|sBT_Vector,0); type->Intrinsics.Add(in); in->ArgType[0] = FindType("uint4");
    in = MakeIntrinsic("Store" ,2,sBT_Uint|sBT_Vector,0); type->Intrinsics.Add(in); in->ArgType[0] = FindType("uint"); in->ArgType[1] = FindType("uint");
    in = MakeIntrinsic("Store2",2,sBT_Uint|sBT_Vector,0); type->Intrinsics.Add(in); in->ArgType[0] = FindType("uint"); in->ArgType[1] = FindType("uint2");
    in = MakeIntrinsic("Store3",2,sBT_Uint|sBT_Vector,0); type->Intrinsics.Add(in); in->ArgType[0] = FindType("uint"); in->ArgType[1] = FindType("uint3");
    in = MakeIntrinsic("Store4",2,sBT_Uint|sBT_Vector,0); type->Intrinsics.Add(in); in->ArgType[0] = FindType("uint"); in->ArgType[1] = FindType("uint4");
}

sType *sTree::FindType(sPoolString name)
{
    return Types.Find([&](const sType *t) { return t->Name==name; });
}

sModule *sTree::FindModule(sPoolString name)
{
    return Modules.Find([&](const sModule *t) { return t->Name==name; });
}

sInject *sTree::FindInject(sPoolString name)
{
    return Injects.Find([&](const sInject *t) { return t->Name==name; });
}

sTypeDef *sTree::FindTypeDef(sPoolString name)
{
    return TypeDefs.Find([&](const sTypeDef *t) { return t->Type->Name==name; });
}

sMember *sTree::FindMember(sPoolString name,sType *type)
{
    sMember *modmem = 0;
    sMember *nulmem = 0;
    for(auto m : type->Members)
    {
        if(m->Name == name)
        {
            if(m->Module==0 || (m->Storage.Flags & sST_Export))
            {
                sASSERT(nulmem==0)
                nulmem = m;
            }
            if(CurrentModule && m->Module == CurrentModule && ! (m->Storage.Flags & sST_Export))
            {
                sASSERT(modmem==0)
                modmem = m;
            }
        }
    }

    if(modmem && nulmem)
        Scan->Error("ambiguous use of %s inside module %s and outside any module",name,CurrentModule->Name);
    return nulmem ? nulmem : modmem;
}

/****************************************************************************/
/***                                                                      ***/
/***   Const Expressions                                                  ***/
/***                                                                      ***/
/****************************************************************************/

int sTree::ConstIntExpr(sExpr *expr,sPoolArray<sCondition> *c)
{
    switch(expr->Kind)
    {
    case sEX_Literal:
        if(expr->Literal->Type->Kind==sTK_Int || expr->Literal->Type->Kind==sTK_Bool)
            return expr->Literal->DataI[0];
        break;

    case sEX_Condition:
        if(c)
        {
            for(auto &e : *c)
            {
                if(e.Name==expr->CondName && e.Member==expr->CondMember)
                {
                    return e.Value;
                }
            }
        }
        Scan->Error(expr->Loc,"unknown condition");
        return 0;

    case sEX_Not:
        return !ConstIntExpr(expr->Args[0],c);
    case sEX_Neg:
        return -ConstIntExpr(expr->Args[0],c);

    case sEX_Add:
        return ConstIntExpr(expr->Args[0],c) + ConstIntExpr(expr->Args[1],c);
    case sEX_Sub:
        return ConstIntExpr(expr->Args[0],c) - ConstIntExpr(expr->Args[1],c);
    case sEX_Mul:
        return ConstIntExpr(expr->Args[0],c) * ConstIntExpr(expr->Args[1],c);
    case sEX_BAnd:
        return ConstIntExpr(expr->Args[0],c) & ConstIntExpr(expr->Args[1],c);
    case sEX_BOr:
        return ConstIntExpr(expr->Args[0],c) | ConstIntExpr(expr->Args[1],c);
    case sEX_BEor:
        return ConstIntExpr(expr->Args[0],c) ^ ConstIntExpr(expr->Args[1],c);
    case sEX_LAnd:
        return ConstIntExpr(expr->Args[0],c) && ConstIntExpr(expr->Args[1],c);
    case sEX_LOr:
        return ConstIntExpr(expr->Args[0],c) || ConstIntExpr(expr->Args[1],c);
    case sEX_Cond:
        return ConstIntExpr(expr->Args[2],c) ? ConstIntExpr(expr->Args[0],c) :  ConstIntExpr(expr->Args[1],c);

    case sEX_EQ:
        return ConstIntExpr(expr->Args[0],c) == ConstIntExpr(expr->Args[1],c);
    case sEX_NE:
        return ConstIntExpr(expr->Args[0],c) != ConstIntExpr(expr->Args[1],c);
    case sEX_GT:
        return ConstIntExpr(expr->Args[0],c) > ConstIntExpr(expr->Args[1],c);
    case sEX_GE:
        return ConstIntExpr(expr->Args[0],c) >= ConstIntExpr(expr->Args[1],c);
    case sEX_LT:
        return ConstIntExpr(expr->Args[0],c) < ConstIntExpr(expr->Args[1],c);
    case sEX_LE:
        return ConstIntExpr(expr->Args[0],c) <= ConstIntExpr(expr->Args[1],c);
    }
    Scan->Error(expr->Loc,"not a const int expression");
    return 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   Dumping                                                            ***/
/***                                                                      ***/
/****************************************************************************/

void sTree::SyncLoc(const sScannerLoc &loc)
{
    if(CppLine)
    {
        int lines = tb->CountLines();
        if(loc.Line!=lines+1+LineOffset)
        {
            tb->PrintF("#line %d %q\n",loc.Line,loc.File);
            LineOffset = loc.Line - lines - 2;
        }
    }
}

void sTree::_DumpType(sType *t,bool full)
{
    bool name = false;
    switch(t->Kind)
    {
    default: tb->Print("???"); break;
    case sTK_Void: tb->Print("void"); break;

    case sTK_Float: tb->Print("float"); break;
    case sTK_Int: tb->Print("int"); break;
    case sTK_Uint:
        if(DumpTargetGl1)
            tb->Print("int");           // glsl1
        else
            tb->Print("uint");
        break;
    case sTK_Bool: tb->Print("bool"); break;
    case sTK_Double: tb->Print("double"); break;

    case sTK_Array:
        _DumpType(t->Base,full);
        if(full)
            tb->PrintF("[%d]",t->SizeX);
        break;
    case sTK_Vector:
        if(DumpTargetGl1)
        {
            switch(t->Base->Kind)
            {
            default: tb->Print("???"); break;
            case sTK_Float: tb->Print("vec"); break;
            case sTK_Int: tb->Print("vec"); break;
            case sTK_Uint: tb->Print("vec"); break;
            case sTK_Bool: tb->Print("vec"); break;
            case sTK_Double: tb->Print("vec"); break;
            }
        }
        else
        {
            _DumpType(t->Base,full);
        }
        tb->PrintF("%d",t->SizeX);
        break;
    case sTK_Matrix:
        if(DumpTargetGl)
        {
            switch(t->Base->Kind)
            {
            default: tb->Print("???"); break;
            case sTK_Float:  tb->Print("mat"); break;
            case sTK_Int:    tb->Print("imat"); break;
            case sTK_Uint:   tb->Print("umat"); break;
            case sTK_Bool:   tb->Print("bmat"); break;
            case sTK_Double: tb->Print("dmat"); break;
            }
        }
        else
        {
            _DumpType(t->Base,full);
        }
        tb->PrintF("%dx%d",t->SizeY,t->SizeX);
        break;


    case sTK_Sampler:             tb->Print("Sampler"); break;
    case sTK_SamplerState:        tb->Print("SamplerState"); break;
    case sTK_SamplerCmpState:     tb->Print("SamplerComparisonState"); break;
    case sTK_TexSam1D:            tb->Print("TexSam1D"); break;
    case sTK_TexSam2D:            tb->Print("TexSam2D"); break;
    case sTK_TexSam3D:            tb->Print("TexSam3D"); break;
    case sTK_Texture1D:           tb->Print("Texture1D"); break;
    case sTK_Texture2D:           tb->Print("Texture2D"); break;
    case sTK_Texture3D:           tb->Print("Texture3D"); break;
    case sTK_Sampler2D:           tb->Print("sampler2D"); break;
    case sTK_SamplerCube:         tb->Print("samplerCube"); break;

    case sTK_Struct:     if(full) tb->Print("struct ");    tb->Print(t->Name);      break;
    case sTK_CBuffer:    if(full) tb->Print("cbuffer ");   tb->Print(t->Name);      break;
    case sTK_TBuffer:    if(full) tb->Print("tbuffer ");   tb->Print(t->Name);      break;
    case sTK_Buffer:              tb->Print("Buffer");               break;
    case sTK_ByteBuffer:          tb->Print("ByteAddressBuffer");    break;
    case sTK_StructBuffer:        tb->Print("StructuredBuffer");     break;
    case sTK_RWBuffer:            tb->Print("RWBuffer");             break;
    case sTK_RWByteBuffer:        tb->Print("RWByteAddressBuffer");  break;
    case sTK_RWStructBuffer:      tb->Print("RWStructuredBuffer");   break;
    case sTK_AppendBuffer:        tb->Print("AppendBuffer");         break;
    case sTK_ConsumeBuffer:       tb->Print("ConsumeBuffer");        break;

    }

    if(t->TemplateType)
    {
        tb->Print("<");
        _DumpType(t->TemplateType,false);
        tb->Print(">");
    }
}

void sTree::_DumpValue(sExpr *e)
{
}

void sTree::_DumpExpr(sExpr *e)
{
//    tb->Print("([");
//    _DumpType(e->Type);
//    tb->Print("]");
    
    switch(e->Kind)
    {
    default: 
        tb->Print("???");
        break;
    case sEX_Literal:
        if(e->Type->Kind==sTK_Float) tb->PrintF("%f",e->Literal->DataF[0]);
        else if(e->Type->Kind==sTK_Uint) tb->PrintF("%d",e->Literal->DataU[0]);
        else if(e->Type->Kind==sTK_Int) tb->PrintF("%d",e->Literal->DataI[0]);
        else if(e->Type->Kind==sTK_Double) tb->PrintF("%f",e->Literal->DataD[0]);
        else if(e->Type->Kind==sTK_Bool) tb->PrintF("%s",e->Literal->DataI[0]?"true":"false");
        else tb->PrintF("???");
        break;

    case sEX_Variable:
        tb->Print(e->Variable->Name);
        break;

    case sEX_Member:
        if(!e->Args.IsEmpty())
        {
            _DumpExpr(e->Args[0]);
            tb->Print(".");
        }
        if(e->Args.GetCount()==2)
        {
            _DumpExpr(e->Args[1]);
        }
        else
        {
            if(e->Member->Module)
                tb->PrintF("%s_",e->Member->Module->Name);
            tb->Print(e->Member->Name);
        }
        break;

    case sEX_Condition:
        tb->Print(e->CondMember);
        if(!e->CondName.IsEmpty())
        {
            tb->Print(".");
            tb->Print(e->CondName);
        }
        break;

    case sEX_Cast:
        if(DumpTargetGl)
        {
            tb->Print("(");
            _DumpType(e->Type,false);
            tb->Print("(");
            _DumpExpr(e->Args[0]);
            tb->Print(")");
            tb->Print(")");
        }
        else
        {
            tb->Print("(");
            tb->Print("(");
            _DumpType(e->Type,false);
            tb->Print(")");
            _DumpExpr(e->Args[0]);
            tb->Print(")");
        }
        break;

    case sEX_Call:
        tb->Print(e->FuncName);
        tb->Print("(");
        for(int i=0;i<e->Args.GetCount();i++)
        {
            if(i!=0)
                tb->Print(",");
            _DumpExpr(e->Args[i]);
        }
        tb->Print(")");
        break;

    case sEX_Swizzle:
        _DumpExpr(e->Args[0]);
        tb->Print(".");
        for(int i=0;(e->Swizzle>>(i*8)&0xff) && i<4;i++)
            tb->PrintF("%c",((const char *)"?xyzw01")[(e->Swizzle>>(i*8)&0xff)]);
        break;
    case sEX_SwizzleExpand:
        if(DumpTarget == sTAR_Glsl1 || DumpTarget == sTAR_Glsl1Es || DumpTarget == sTAR_Glsl4)
        {
            _DumpType(e->Type,false);
            tb->Print("(");
            _DumpExpr(e->Args[0]);
            tb->Print(")");
        }
        else
        {
            _DumpExpr(e->Args[0]);
        }
        break;

    case sEX_Intrinsic:
        if(!e->IntrinsicReplacementName.IsEmpty())
            tb->Print(e->IntrinsicReplacementName);
        else
            tb->Print(e->Intrinsic->Name);
        tb->Print("(");
        for(int i=0;i<e->Args.GetCount();i++)
        {
            if(i>0)
                tb->Print(",");
            _DumpExpr(e->Args[i]);
        }
        tb->Print(")");
        break;
    case sEX_Construct:
        _DumpType(e->Type,false);
        tb->Print("(");
        for(int i=0;i<e->Args.GetCount();i++)
        {
            if(i>0)
                tb->Print(",");
            _DumpExpr(e->Args[i]);
        }
        tb->Print(")");
        break;

    case sEX_Index:
        _DumpExpr(e->Args[0]);
        tb->Print("[");
        _DumpExpr(e->Args[1]);
        tb->Print("]");
        break;

    case sEX_Neg:           tb->Print("-(");  _DumpExpr(e->Args[0]); tb->Print(")"); break;
    case sEX_Pos:           tb->Print("+(");  _DumpExpr(e->Args[0]); tb->Print(")"); break;
    case sEX_Not:           tb->Print("!(");  _DumpExpr(e->Args[0]); tb->Print(")"); break;
    case sEX_Complement:    tb->Print("~(");  _DumpExpr(e->Args[0]); tb->Print(")"); break;
    case sEX_PreInc:        tb->Print("++("); _DumpExpr(e->Args[0]); tb->Print(")"); break;
    case sEX_PreDec:        tb->Print("--("); _DumpExpr(e->Args[0]); tb->Print(")"); break;
    case sEX_PostInc:       tb->Print("(");   _DumpExpr(e->Args[0]); tb->Print("++"); tb->Print(")"); break;
    case sEX_PostDec:       tb->Print("(");   _DumpExpr(e->Args[0]); tb->Print("--"); tb->Print(")"); break;

    case sEX_Add:           tb->Print("(");   _DumpExpr(e->Args[0]); tb->Print(" + ");  _DumpExpr(e->Args[1]); tb->Print(")"); break;
    case sEX_Sub:           tb->Print("(");   _DumpExpr(e->Args[0]); tb->Print(" - ");  _DumpExpr(e->Args[1]); tb->Print(")"); break;
    case sEX_Mul:           tb->Print("(");   _DumpExpr(e->Args[0]); tb->Print(" * ");  _DumpExpr(e->Args[1]); tb->Print(")"); break;
    case sEX_Div:           tb->Print("(");   _DumpExpr(e->Args[0]); tb->Print(" / ");  _DumpExpr(e->Args[1]); tb->Print(")"); break;
    case sEX_Mod:           tb->Print("(");   _DumpExpr(e->Args[0]); tb->Print(" % ");  _DumpExpr(e->Args[1]); tb->Print(")"); break;
    case sEX_Dot:           tb->Print("(");   _DumpExpr(e->Args[0]); tb->Print(" ** "); _DumpExpr(e->Args[1]); tb->Print(")"); break;

    case sEX_ShiftL:        tb->Print("(");   _DumpExpr(e->Args[0]); tb->Print(" << "); _DumpExpr(e->Args[1]); tb->Print(")"); break;
    case sEX_ShiftR:        tb->Print("(");   _DumpExpr(e->Args[0]); tb->Print(" >> "); _DumpExpr(e->Args[1]); tb->Print(")"); break;
    case sEX_BAnd:          tb->Print("(");   _DumpExpr(e->Args[0]); tb->Print(" & ");  _DumpExpr(e->Args[1]); tb->Print(")"); break;
    case sEX_BOr:           tb->Print("(");   _DumpExpr(e->Args[0]); tb->Print(" | ");  _DumpExpr(e->Args[1]); tb->Print(")"); break;
    case sEX_BEor:          tb->Print("(");   _DumpExpr(e->Args[0]); tb->Print(" ^ ");  _DumpExpr(e->Args[1]); tb->Print(")"); break;

    case sEX_EQ:            tb->Print("(");   _DumpExpr(e->Args[0]); tb->Print(" == "); _DumpExpr(e->Args[1]); tb->Print(")"); break;
    case sEX_NE:            tb->Print("(");   _DumpExpr(e->Args[0]); tb->Print(" != "); _DumpExpr(e->Args[1]); tb->Print(")"); break;
    case sEX_LT:            tb->Print("(");   _DumpExpr(e->Args[0]); tb->Print(" < ");  _DumpExpr(e->Args[1]); tb->Print(")"); break;
    case sEX_GT:            tb->Print("(");   _DumpExpr(e->Args[0]); tb->Print(" > ");  _DumpExpr(e->Args[1]); tb->Print(")"); break;
    case sEX_LE:            tb->Print("(");   _DumpExpr(e->Args[0]); tb->Print(" <= "); _DumpExpr(e->Args[1]); tb->Print(")"); break;
    case sEX_GE:            tb->Print("(");   _DumpExpr(e->Args[0]); tb->Print(" >= "); _DumpExpr(e->Args[1]); tb->Print(")"); break;
            
    case sEX_LAnd:          tb->Print("(");   _DumpExpr(e->Args[0]); tb->Print(" && "); _DumpExpr(e->Args[1]); tb->Print(")"); break;
    case sEX_LOr:           tb->Print("(");   _DumpExpr(e->Args[0]); tb->Print(" || "); _DumpExpr(e->Args[1]); tb->Print(")"); break;
        /*
    case sEX_And:  _DumpExpr(e->Args[0]); tb->Print("-"); _DumpExpr(e->Args[1]); break;
    case sEX_Or:  _DumpExpr(e->Args[0]); tb->Print("-"); _DumpExpr(e->Args[1]); break;
    case sEX_Eor:  _DumpExpr(e->Args[0]); tb->Print("-"); _DumpExpr(e->Args[1]); break;
    */
    case sEX_Cond:          tb->Print("(");   _DumpExpr(e->Args[2]); tb->Print(" ? "); _DumpExpr(e->Args[0]); tb->Print(" : "); _DumpExpr(e->Args[1]); tb->Print(")");  break;

    case sEX_Assign:        _DumpExpr(e->Args[0]); tb->Print(" = ");   _DumpExpr(e->Args[1]); break;
    case sEX_AssignAdd:     _DumpExpr(e->Args[0]); tb->Print(" += ");  _DumpExpr(e->Args[1]); break;
    case sEX_AssignSub:     _DumpExpr(e->Args[0]); tb->Print(" -= ");  _DumpExpr(e->Args[1]); break;
    case sEX_AssignMul:     _DumpExpr(e->Args[0]); tb->Print(" *= ");  _DumpExpr(e->Args[1]); break;
    case sEX_AssignDiv:     _DumpExpr(e->Args[0]); tb->Print(" /= ");  _DumpExpr(e->Args[1]); break;
    case sEX_AssignMod:     _DumpExpr(e->Args[0]); tb->Print(" %= ");  _DumpExpr(e->Args[1]); break;
    case sEX_AssignShiftL:  _DumpExpr(e->Args[0]); tb->Print(" <<= "); _DumpExpr(e->Args[1]); break;
    case sEX_AssignShiftR:  _DumpExpr(e->Args[0]); tb->Print(" >>= "); _DumpExpr(e->Args[1]); break;
    case sEX_AssignBAnd:    _DumpExpr(e->Args[0]); tb->Print(" &= ");  _DumpExpr(e->Args[1]); break;
    case sEX_AssignBOr:     _DumpExpr(e->Args[0]); tb->Print(" |= ");  _DumpExpr(e->Args[1]); break;
    case sEX_AssignBEor:    _DumpExpr(e->Args[0]); tb->Print(" ^= ");  _DumpExpr(e->Args[1]); break;
    }
}

void sTree::_DumpStmt(sStmt *s,int indent)
{
    if(!(s->Kind==sST_Block || s->Kind==sST_BlockNoBrackets || s->Kind==sST_Inject))
        SyncLoc(s->Loc);

    if(s->StatementFlags & sSTF_Unroll)
        tb->PrintF("%_[unroll]\n",indent*4);
    if(s->StatementFlags & sSTF_Loop)
        tb->PrintF("%_[loop]\n",indent*4);


    switch(s->Kind)
    {
    default:
        tb->PrintF("%_???;\n",indent*4);
        break;
    case sST_Block:
        tb->PrintF("%_{\n",indent*4);
        for(auto ss : s->Block)
        {
            if(!(ss->Kind==sST_Block && ss->Block.GetCount()==0))
                _DumpStmt(ss,indent+1);
        }
        tb->PrintF("%_}\n",indent*4);
        break;
    case sST_Inject:
    case sST_BlockNoBrackets:
        for(auto ss : s->Block)
        {
            if(!(ss->Kind==sST_Block && ss->Block.GetCount()==0))
                _DumpStmt(ss,indent);
        }
        break;
    case sST_Nop:
        tb->PrintF("%_;\n",indent*4);
        break;
    case sST_If:
        tb->PrintF("%_if(",indent*4);
        _DumpExpr(s->Expr);
        tb->Print(")\n");
        _DumpStmt(s->Block[0],indent+(s->Block[0]->Kind!=sST_Block));
        if(s->Block.GetCount()==2)
        {
            tb->PrintF("%_else\n",indent*4);
            _DumpStmt(s->Block[1],indent+(s->Block[1]->Kind!=sST_Block));
        }
        break;
    case sST_Cif:
        tb->PrintF("%_cif(",indent*4);
        _DumpExpr(s->Expr);
        tb->Print(")\n");
        _DumpStmt(s->Block[0],indent+(s->Block[0]->Kind!=sST_Block));
        if(s->Block.GetCount()==2)
        {
            tb->PrintF("%_else\n",indent*4);
            _DumpStmt(s->Block[1],indent+(s->Block[1]->Kind!=sST_Block));
        }
        break;
    case sST_For:
        tb->PrintF("%_for(",indent*4);
        {
            auto t = s->Block[0];
            switch(t->Kind)
            {
            case sST_Decl:
                _DumpStorage(t->Vars[0]->Storage);
                _DumpType(t->Vars[0]->Type,false);
                tb->PrintF(" %s",t->Vars[0]->Name);
                if(t->Expr)
                {
                    tb->Print(" = ");
                    _DumpExpr(t->Expr);
                }
                tb->PrintF(";");
                break;
            case sST_Expr:
                _DumpExpr(t->Expr);
                tb->PrintF(";");
                break;
            default:
                _DumpStmt(s->Block[0],0);
                break;
            }
        }
        _DumpExpr(s->Expr);
        tb->PrintF(";");
        _DumpExpr(s->IncExpr);
        tb->PrintF(")\n");
        _DumpStmt(s->Block[1],indent+(s->Block[1]->Kind!=sST_Block));
        break;
    case sST_While:
        tb->PrintF("%_while(",indent*4);
        _DumpExpr(s->Expr);
        tb->Print(")\n");
        _DumpStmt(s->Block[0],indent+(s->Block[0]->Kind!=sST_Block));
        break;
    case sST_Switch:
        tb->PrintF("%_???;\n",indent*4);
        break;
    case sST_Do:
        tb->PrintF("%_???;\n",indent*4);
        break;
    case sST_Break:
        tb->PrintF("%_break;\n",indent*4);
        break;
    case sST_Continue:
        tb->PrintF("%_continue;\n",indent*4);
        break;
    case sST_Discard:
        tb->PrintF("%_discard;\n",indent*4);
        break;
    case sST_Return:
        tb->PrintF("%_return ",indent*4);
        if(s->Expr)
            _DumpExpr(s->Expr);
        tb->PrintF(";\n");
        break;
    case sST_Case:
        tb->PrintF("%_case ",indent*4-4);
        _DumpExpr(s->Expr);
        tb->PrintF(":\n");
        break;
    case sST_Default:
        tb->PrintF("%_default:\n",indent*4-4);
        break;
    case sST_Expr:
        tb->PrintF("%_",indent*4);
        _DumpExpr(s->Expr);
        tb->PrintF(";\n");
        break;
    case sST_Decl:
        tb->PrintF("%_",indent*4);
        _DumpDecl(s->Vars[0]->Storage,s->Vars[0]->Type,s->Vars[0]->Name);

        if(s->Expr)
        {
            tb->Print(" = ");
            _DumpExpr(s->Expr);
        }
        tb->PrintF(";\n");
        break;
    }
}

void sTree::_DumpStorage(const sStorage &storage)
{
    int store = storage.Flags;
    if(store & sST_RowMajor) tb->Print("row_major ");
    if(store & sST_ColumnMajor) tb->Print("column_major ");
    if(store & sST_Static) tb->Print("static ");
    if(store & sST_Const) tb->Print("const ");
    if(store & sST_GroupShared) tb->Print("groupshared ");
    if(store & sST_Precise) tb->Print("precise ");
    if(store & sST_In) tb->Print("in ");
    if(store & sST_Out) tb->Print("out ");
    if(store & sST_Import) tb->Print("/*import*/ ");
    if(store & sST_Export) tb->Print("/*export*/ ");
    if(store & sST_Linear) tb->Print("linear ");
    if(store & sST_NoInterpolation) tb->Print("nointerpolation ");
    if(store & sST_Centroid) tb->Print("centroid ");
    if(store & sST_NoPerspective) tb->Print("noperspective ");
    if(store & sST_Sample) tb->Print("sample ");

    if(store & sST_Uniform) tb->Print("uniform ");
    if(store & sST_Varying) tb->Print("varying ");
    if(store & sST_Attribute) tb->Print("attribute ");

    if(DumpTarget==sTAR_Glsl1Es)
    {
        if(store & sST_LowP) tb->Print("lowp ");
        if(store & sST_MedP) tb->Print("mediump ");
        if(store & sST_HighP) tb->Print("highp ");

        if (store & sST_Uniform) tb->Print("mediump ");
    }
}

void sTree::_DumpSemantic(sSemantic sem)
{
    bool forceindex = false;
    int semkind = sem & sSEM_Mask;
    int semindex = sem>>sSEM_IndexShift;

    if(semkind)
    {
        auto string = DumpSemanticNames[DumpTarget][semkind];
        if(string)
        {
            tb->Print(" : ");
            if(sFindFirstString(string,"%%"))
                tb->PrintF(string,semindex);
            else if(semindex==0)
                tb->Print(string);
            else
                tb->PrintF("%s%d",string,semindex);
        }
        else
        {
            tb->PrintF("???");
        }
    }

    /*
    switch(sem & sSEM_Mask)
    {
    case sSEM_None: return;
    case sSEM_Position: tb->Print(" : Position"); break;
    case sSEM_Normal: tb->Print(" : Normal"); break;
    case sSEM_Tangent: tb->Print(" : Tangent"); break;
    case sSEM_Binormal: tb->Print(" : Bitangent"); break;
    case sSEM_BlendIndex: tb->Print(" : BlendIndex"); break;
    case sSEM_BlendWeight: tb->Print(" : BlendWeight"); break;
    case sSEM_Color: tb->Print(" : Color"); forceindex=true; break;
    case sSEM_Tex: tb->Print(" : Tex"); forceindex=true; break;

    case sSEM_InstanceId: tb->Print(" : InstanceId"); break;
    case sSEM_VertexId: tb->Print(" : VertexId"); break;

    case sSEM_VFace: tb->Print(" : VFace"); break;
    case sSEM_VPos: tb->Print(" : VPos"); break;
    case sSEM_Target: tb->Print(" : Target"); forceindex=true; break;
    case sSEM_Depth: tb->Print(" : Depth"); break;
    case sSEM_Coverage: tb->Print(" : Coverage"); break;
    case sSEM_IsFrontFace: tb->Print(" : IsFrontFace"); break;
    case sSEM_SampleIndex: tb->Print(" : SampleIndex"); break;
    case sSEM_PrimitiveId: tb->Print(" : PrimitiveId"); break;

    case sSEM_Vs: tb->Print(" : Vs"); forceindex=true; break;
    case sSEM_Hs: tb->Print(" : Hs"); forceindex=true; break;
    case sSEM_Ds: tb->Print(" : Ds"); forceindex=true; break;
    case sSEM_Gs: tb->Print(" : Gs"); forceindex=true; break;
    case sSEM_Ps: tb->Print(" : Ps"); forceindex=true; break;
    case sSEM_Cs: tb->Print(" : Cs"); forceindex=true; break;
    case sSEM_Slot: tb->Print(" : Slot"); forceindex=true; break;

    default: tb->Print(" : ???"); break;
    }
    int index = sem>>sSEM_IndexShift;
    if(index>0 || forceindex)
        tb->PrintF("%d",index);
        */
}

void sTree::_DumpMember(sMember *m,int indent,sType *base)
{
    if(m->Active)
    {
        tb->PrintF("%_",indent*4);
        _DumpStorage(m->Storage);
        _DumpType(m->Type,false);
        tb->Print(" ");
        if(m->Module)
            tb->PrintF("%s_",m->Module->Name);
        tb->PrintF("%s",m->Name);

        auto type = m->Type;
        while(type->Kind==sTK_Array)
        {
            tb->PrintF("[%d]",ConstIntExpr(type->SizeArray,0));
            type = type->Base;
        }

        _DumpSemantic(m->Semantic);
        if(DumpTarget==sTAR_Hlsl5 && (base->Kind==sTK_CBuffer || base->Kind==sTK_TBuffer))
        {
            int start = m->Offset & 3;
            tb->PrintF(" : packoffset(c%d.%c)",m->Offset/4,((const char *)"xyzw")[start]);
        }


        tb->PrintF(";\n");
    }
}

void sTree::_DumpDecl(sStorage &store,sType *type,sPoolString name)
{
    _DumpStorage(store);
    _DumpType(type,false);
    tb->PrintF(" %s",name);
    
    while(type->Kind==sTK_Array)
    {
        tb->Print("[");
        _DumpExpr(type->SizeArray);
        tb->Print("]");
        type = type->Base;
    }

}

void sTree::_DumpVariable(sVariable *m)
{
    _DumpDecl(m->Storage,m->Type,m->Name);
    if(m->Initializer)
    {
        tb->Print(" = ");
        _DumpExpr(m->Initializer);
    }
    _DumpSemantic(m->Semantic);
}

void sTree::_DumpFunc(sFunction *f)
{
    if(f->NumThreadsX!=0)
    {
        tb->Print("[numthreads(");
        _DumpExpr(f->NumThreadsX);
        tb->Print(",");
        _DumpExpr(f->NumThreadsY);
        tb->Print(",");
        _DumpExpr(f->NumThreadsZ);
        tb->Print(")]\n");
    }
    SyncLoc(f->Loc);
    _DumpType(f->Result,false);
    tb->PrintF(" %s(",f->Name);
    bool first = true;
    for(auto m : f->Root->Vars)
    {
        if(first)
            first = false;
        else
            tb->Print(",");
        _DumpVariable(m);
    }
    tb->PrintF(")\n",f->Name);
    sASSERT((f->Root->Kind==sST_Block || f->Root->Kind==sST_Inject) && f->Root->Block.GetCount()==1);
    _DumpStmt(f->Root->Block[0],0);
}

void sTree::_DumpTypeDef(sTypeDef *td)
{
    SyncLoc(td->Loc);
    _DumpType(td->Type,true);
    _DumpSemantic(td->Type->Semantic);
    switch(td->Type->Kind)
    {
    case sTK_Struct:
    case sTK_CBuffer:
    case sTK_TBuffer:
        tb->PrintF("\n{\n");
        for(auto m : td->Type->Members)
            _DumpMember(m,1,td->Type);
        tb->PrintF("}");
        break;
    }
    tb->PrintF(";\n");
}

void sTree::Dump(sTextLog &tb_,int shaders,sTarget target)
{
    DumpTarget = target;
    LineOffset = 0;
    tb = &tb_;

    DumpTargetGl = false;
    DumpTargetGl1 = false;

    if(DumpTarget==sTAR_Glsl1)
    {
        tb->Print("#version 130\n");
        DumpTargetGl = true;
        DumpTargetGl1 = true;
    }
    if(DumpTarget==sTAR_Glsl1Es)
    {
        tb->Print("#version 100\n");
        DumpTargetGl = true;
        DumpTargetGl1 = true;
    }
    if(DumpTarget==sTAR_Glsl4)
    {
        tb->Print("#version 410\n");
        DumpTargetGl = true;
    }
    if(DumpTarget==sTAR_Hlsl3 || DumpTarget==sTAR_Hlsl5)
    {
        tb->Print("#pragma warning( disable : 3571) // exp sign\n");
        tb->Print("#pragma warning( disable : 3557) // force unroll\n");
    }

    for(auto nt : TypeDefs)
    {
        if(!nt->Type->IsBaseType() && nt->Type->Kind!=sTK_Vector && nt->Type->Kind!=sTK_Matrix)
        {
            if(nt->Shaders==0 || (nt->Shaders&shaders))
                _DumpTypeDef(nt);
        }
    }
    for(auto var : Vars)
    {
        if(var->Shaders==0 || (var->Shaders&shaders))
        {
            SyncLoc(var->Loc);
            _DumpVariable(var);
            tb->Print(";\n");
        }
    }

    // dump main function last

    for(auto f : Funcs)
    {
        if((f->Shaders==0 || (f->Shaders&shaders)) && f->Name!="main")
            _DumpFunc(f);
    }
    for(auto f : Funcs)
    {
        if((f->Shaders==0 || (f->Shaders&shaders)) && f->Name=="main")
            _DumpFunc(f);
    }

    // done

    tb = 0;
    DumpTarget = sTAR_Max;
}

/****************************************************************************/
/****************************************************************************/

sTree *sTree::CopyTo()
{
    auto newtree = new sTree();
    newtree->Scan = Scan;
    newtree->Reset(false);
    newtree->CopyFrom(this);
    return newtree;
}

void sTree::CopyFrom(sTree *x)
{
    sASSERT(Types.IsEmpty());
    sASSERT(TypeDefs.IsEmpty());
    sASSERT(Vars.IsEmpty());
    sASSERT(Funcs.IsEmpty());
    sASSERT(Intrinsics.IsEmpty());
    sASSERT(Injects.IsEmpty());
    sASSERT(Modules.IsEmpty());
    sASSERT(ErrorExpr==0);
    sASSERT(ErrorType==0);

    // simple copy

    CurrentShaders = x->CurrentShaders;
    DumpTarget = x->DumpTarget;
    DumpTargetGl = x->DumpTargetGl;
    UsedShaders = x->UsedShaders;
    TempIndex = x->TempIndex;
    VariantBit = x->VariantBit;

    // clear deep-copy fields

    DeepTag(x->ErrorExpr);
    DeepTag(x->ErrorType);
    for(auto t : x->Types)
        DeepTag(t);
    for(auto v : x->Vars)
        DeepTag(v);
    for(auto f : x->Funcs)
    {
        DeepTag(f->Root);
        DeepTag(f->NumThreadsX);
        DeepTag(f->NumThreadsY);
        DeepTag(f->NumThreadsZ);
    }
    for(auto m : x->Modules)
        DeepTag(m);
    for(auto i : x->Intrinsics)
        DeepTag(i);
    for(auto i : x->Injects)
    {
        for(auto m : i->Members)
            DeepTag(m);
        DeepTag(i->Scope);
        i->DeepCopy = 0;
    }
    for(auto m : x->Modules)
    {
        for(auto s : m->Sniplets)
            DeepTag(s->Code);
        for(auto v : m->Vars)
            DeepTag(v);
        m->DeepCopy = 0;
    }

    // deep copy

    ErrorExpr = DeepCopy(x->ErrorExpr);
    ErrorType = DeepCopy(x->ErrorType);

    for(auto t : x->Types)
        Types.Add(DeepCopy(t));
    for(auto td : x->TypeDefs)
    {
        auto ntd = MakeTypeDef(DeepCopy(td->Type));
        ntd->Loc = td->Loc;
        ntd->Shaders = td->Shaders;
        ntd->Temp = td->Temp;
        TypeDefs.Add(ntd);
    }
    for(auto v : x->Vars)
    {
        Vars.Add(DeepCopy(v));
    }
    for(auto f : x->Funcs)
    {
        auto nf = MakeFunction();
        nf->Result = DeepCopy(f->Result);
        nf->Name = f->Name;
        nf->Loc = f->Loc;
        nf->Root = DeepCopy(f->Root);
        nf->Shaders = f->Shaders;

        nf->NumThreadsX = DeepCopy(f->NumThreadsX);
        nf->NumThreadsY = DeepCopy(f->NumThreadsY);
        nf->NumThreadsZ = DeepCopy(f->NumThreadsZ);
        Funcs.Add(nf);
    }
    for(auto i : x->Intrinsics)
    {
        DeepCopy(i);
        // added automatically
    }
    for(auto i : x->Injects)
    {
        Injects.Add(DeepCopy(i));
    }
    for(auto m : x->Modules)
    {
        Modules.Add(DeepCopy(m));
    }
    for(auto v : x->Variants)
    {
        auto nv = MakeVariant();
        Variants.Add(nv);
        nv->Loc = v->Loc;
        nv->Name = v->Name;
        nv->Mask = v->Mask;
        nv->Bool = v->Bool;

        for(auto m : v->Members)
        {
            auto nm = MakeVariantMember();
            nv->Members.Add(m);
            nm->Loc = m->Loc;
            nm->Name = m->Name;
            nm->Module = m->Module;
            nm->Value = m->Value;
        }
    }
}

void sTree::DeepTag(sType *x)
{
    if(x==0)
        return;
    x->DeepCopy = 0;

    DeepTag(x->Base);
    DeepTag(x->TemplateType);
    DeepTag(x->SizeArray);
    for(auto m : x->Members)
        DeepTag(m);
    for(auto i : x->Intrinsics)
        i->DeepCopy = 0;
}

void sTree::DeepTag(sMember *x)
{
    if(x==0)
        return;
    x->DeepCopy = 0;

    DeepTag(x->Type);
    for(int i=0;i<4;i++)
        DeepTag(x->Vector[i]);
    DeepTag(x->ReplaceExpr);
    DeepTag(x->Condition);
    DeepTag(x->ReplaceMember);
}

void sTree::DeepTag(sVariable *x)
{
    if(x==0)
        return;
    x->DeepCopy = 0;

    DeepTag(x->Type);
    DeepTag(x->Initializer);
    DeepTag(x->TempReplace);
    DeepTag(x->Vector[0]);
    DeepTag(x->Vector[1]);
    DeepTag(x->Vector[2]);
    DeepTag(x->Vector[3]);
}

void sTree::DeepTag(sIntrinsic *x)
{
    if(x==0)
        return;
    x->DeepCopy = 0;

    for(int i=0;i<sCOUNTOF(x->ArgType);i++)
        DeepTag(x->ArgType[i]);
    DeepTag(x->ResultType);
}

void sTree::DeepTag(sModule *x)
{
    if(x==0)
        return;
    x->DeepCopy = 0;

    for(auto &m : x->Depends)
    {
        DeepTag(m.Module);
        DeepTag(m.Condition);
    }
    for(auto s : x->Sniplets)
        DeepTag(s->Code);
    for(auto v : x->Vars)
        DeepTag(v);
}

void sTree::DeepTag(sStmt *x)
{
    if(x==0)
        return;
    x->DeepCopy = 0;

    DeepTag(x->Expr);
    DeepTag(x->IncExpr);
    for(auto s : x->Block)
        DeepTag(s);
    for(auto v : x->Vars)
        DeepTag(v);
}

void sTree::DeepTag(sExpr *x)
{
    if(x==0)
        return;
    x->DeepCopy = 0;

    DeepTag(x->Type);
    for(auto a : x->Args)
        DeepTag(a);

    DeepTag(x->Variable);
    DeepTag(x->Member);
    DeepTag(x->Intrinsic);
    DeepTag(x->Literal);
}

void sTree::DeepTag(sLiteral *x)
{
    if(x==0)
        return;
    x->DeepCopy = 0;

    DeepTag(x->Type);
}

sType *sTree::DeepCopy(sType *x)
{
    if(x==0)
        return 0;
    if(x->DeepCopy) 
        return x->DeepCopy;

    auto nx = MakeType(x->Kind);
    x->DeepCopy = nx;

    nx->Loc = x->Loc;
    nx->Base = DeepCopy(x->Base);
    nx->Name = x->Name;
    nx->SizeX = x->SizeX;
    nx->SizeY = x->SizeY;
    for(auto m : x->Members)
        nx->Members.Add(DeepCopy(m));
    nx->Semantic = x->Semantic;
    for(auto i : x->Intrinsics)
        nx->Intrinsics.Add(DeepCopy(i));
    nx->StructSize = x->StructSize;
    nx->TemplateType = x->TemplateType;
    nx->HasTemplate = x->HasTemplate;
    nx->Temp = x->Temp;

    return nx;
}

sMember *sTree::DeepCopy(sMember *x)
{
    if(x==0)
        return 0;
    if(x->DeepCopy) 
        return x->DeepCopy;

    auto nx = MakeMember(DeepCopy(x->Type),x->Name,x->Storage,x->Semantic);
    x->DeepCopy = nx;

    nx->Loc = x->Loc;
    nx->Offset = x->Offset;
    nx->Active = x->Active;
    nx->Align = x->Align;
    nx->Module = DeepCopy(x->Module);
    nx->Vector[0] = DeepCopy(x->Vector[0]);
    nx->Vector[1] = DeepCopy(x->Vector[1]);
    nx->Vector[2] = DeepCopy(x->Vector[2]);
    nx->Vector[3] = DeepCopy(x->Vector[3]);
    nx->ReplaceExpr = DeepCopy(x->ReplaceExpr);
    nx->Condition = DeepCopy(x->Condition);
    nx->ReplaceMember = DeepCopy(x->ReplaceMember);
    nx->Module = DeepCopy(x->Module);

    return nx;
}

sVariable *sTree::DeepCopy(sVariable *x)
{
    if(x==0)
        return 0;
    if(x->DeepCopy) 
        return x->DeepCopy;

    auto nx = MakeVar(DeepCopy(x->Type),x->Name,x->Storage,true,x->Semantic);
    x->DeepCopy = nx;

    nx->Loc = x->Loc;
    nx->Shaders = x->Shaders;
    nx->TempReplace = DeepCopy(x->TempReplace);
    nx->PatchSlot = x->PatchSlot;
    nx->Initializer = DeepCopy(x->Initializer);
    nx->Vector[0] = DeepCopy(x->Vector[0]);
    nx->Vector[1] = DeepCopy(x->Vector[1]);
    nx->Vector[2] = DeepCopy(x->Vector[2]);
    nx->Vector[3] = DeepCopy(x->Vector[3]);

    return nx;
}

sIntrinsic *sTree::DeepCopy(sIntrinsic *x)
{
    if(x==0)
        return 0;
    if(x->DeepCopy) 
        return x->DeepCopy;

    auto nx = MakeIntrinsic(x->Name,x->ArgCount,x->TypeMask,x->Flags);
    x->DeepCopy = nx;

    nx->Loc = x->Loc;
    nx->MinArgCount = x->MinArgCount;
    for(int j=0;j<sCOUNTOF(x->ArgType);j++)
        nx->ArgType[j] = DeepCopy(x->ArgType[j]);
    nx->ResultType = DeepCopy(x->ResultType);

    return nx;
}

sInject *sTree::DeepCopy(sInject *x)
{
    if(x==0)
        return 0;
    if(x->DeepCopy) 
        return x->DeepCopy;

    auto nx = MakeInject();
    x->DeepCopy = nx;

    nx->Shaders = x->Shaders;
    nx->Loc = x->Loc;
    nx->Name = x->Name;
    nx->Kind = x->Kind;
    nx->Scope = DeepCopy(x->Scope);
    nx->Type = DeepCopy(x->Type);
    for(auto m : x->Members)
        nx->Members.Add(DeepCopy(m));

    return nx;
}

sModule *sTree::DeepCopy(sModule *x)
{
    if(x==0)
        return 0;
    if(x->DeepCopy) 
        return x->DeepCopy;

    auto nm = MakeModule();
    x->DeepCopy = nm;

    nm->Loc = x->Loc;
    nm->Name = x->Name;
//    nm->Index = x->Index;
    nm->Once = x->Once;
    nm->OnceTemp = x->OnceTemp;
    for(auto s : x->Sniplets)
    {
        auto ns = MakeSniplet();
        ns->Code = DeepCopy(s->Code);
        ns->Inject = DeepCopy(s->Inject);
        nm->Sniplets.Add(ns);
    }
    for(auto &d : x->Depends)
    {
        nm->Depends.Add(sModuleDependency(d.Module->DeepCopy,DeepCopy(d.Condition)));
    }
    for(auto v : x->Vars)
        nm->Vars.Add(DeepCopy(v));

    return nm;
}

sStmt *sTree::DeepCopy(sStmt *x)
{
    if(x==0)
        return 0;
    if(x->DeepCopy) 
        return x->DeepCopy;

    auto nx = MakeStmt(x->Kind);
    x->DeepCopy = nx;

    nx->Loc = x->Loc;
    nx->SwitchCase = x->SwitchCase;
    for(auto b : x->Block)
        nx->Block.Add(DeepCopy(b));
    for(auto v : x->Vars)
        nx->Vars.Add(DeepCopy(v));
    nx->Expr = DeepCopy(x->Expr);
    nx->IncExpr = DeepCopy(x->IncExpr);
    nx->ParentScope = DeepCopy(x->ParentScope);
    nx->Inject = DeepCopy(x->Inject);
    nx->StatementFlags = x->StatementFlags;

    return nx;
}

sExpr *sTree::DeepCopy(sExpr *x)
{
    if(x==0)
        return 0;
    if(x->DeepCopy) 
        return x->DeepCopy;

    auto nx = MakeExpr(x->Kind);
    x->DeepCopy = nx;

    nx->Loc = x->Loc;
    for(auto a : x->Args)
        nx->Args.Add(DeepCopy(a));
    nx->Type = DeepCopy(x->Type);
    nx->Literal = DeepCopy(x->Literal);
    nx->Swizzle = x->Swizzle;
    nx->Variable = DeepCopy(x->Variable);
    nx->Member = DeepCopy(x->Member);
    nx->Intrinsic = DeepCopy(x->Intrinsic);
    nx->IntrinsicReplacementName = x->IntrinsicReplacementName;
    nx->CondName = x->CondName;
    nx->CondMember = x->CondMember;
    nx->FuncName = x->FuncName;

    return nx;
}

sLiteral *sTree::DeepCopy(sLiteral *x)
{
    if(x==0)
        return 0;
    if(x->DeepCopy) 
        return x->DeepCopy;

    auto nx = MakeLiteral(DeepCopy(x->Type));
    x->DeepCopy = nx;

    nx->Loc = x->Loc;
    sASSERT(nx->Elements == x->Elements);
    for(int i=0;i<nx->Elements;i++)
        nx->DataU[i] = x->DataU[i];

    return nx;
}

/****************************************************************************/
/****************************************************************************/

void sTree::CloneTag(sStmt *x)
{
    if(x==0)
        return;
    x->DeepCopy = 0;
    if(x->ParentScope)
        x->ParentScope->DeepCopy = 0;

    for(auto y : x->Block)
        CloneTag(y);
    CloneTag(x->Expr);
    CloneTag(x->IncExpr);
}

void sTree::CloneTag(sExpr *x)
{
    if(x==0)
        return;
    x->DeepCopy = 0;

    for(auto y : x->Args)
        CloneTag(y);
}

sStmt *sTree::CloneCopy(sStmt *x)
{
    if(x==0)
        return 0;
    if(x->DeepCopy)
        return x->DeepCopy;

    auto nx = MakeStmt(x->Kind);
    x->DeepCopy = nx;
    for(auto y : x->Block)
    {
        auto ny = CloneCopy(y);
        nx->Block.Add(ny);
    }
    nx->Vars.CopyFrom(x->Vars);
    nx->Loc = x->Loc;
    nx->Kind = x->Kind;
    nx->SwitchCase = x->SwitchCase;
    nx->Expr = CloneCopy(x->Expr);
    nx->IncExpr = CloneCopy(x->IncExpr);
    if(x->ParentScope)
        nx->ParentScope = x->ParentScope->DeepCopy ? x->ParentScope->DeepCopy : x->ParentScope;
    else
        nx->ParentScope = 0;
    nx->Inject = x->Inject;
    nx->StatementFlags = x->StatementFlags;

    return nx;
}

sExpr *sTree::CloneCopy(sExpr *x)
{
    if(x==0)
        return 0;
    if(x->DeepCopy)
        return x->DeepCopy;

    auto nx = MakeExpr(x->Kind);
    x->DeepCopy = nx;
    for(auto y : x->Args)
        nx->Args.Add(CloneCopy(y));
    nx->Loc = x->Loc;
    nx->Kind = x->Kind;
    nx->Type = x->Type;
    nx->Literal = x->Literal;
    nx->Swizzle = x->Swizzle;
    nx->Variable = x->Variable;
    nx->Member = x->Member;
    nx->Intrinsic = x->Intrinsic;
    nx->IntrinsicReplacementName = x->IntrinsicReplacementName;
    nx->CondName = x->CondName;
    nx->CondMember = x->CondMember;
    nx->FuncName = x->FuncName;

    return nx;
}

sExpr *sTree::CloneCopyExpr(sExpr *x)
{
    if(x==0)
        return 0;

    auto nx = MakeExpr(x->Kind);
    for(auto y : x->Args)
        nx->Args.Add(CloneCopy(y));
    nx->Loc = x->Loc;
    nx->Kind = x->Kind;
    nx->Type = x->Type;
    nx->Literal = x->Literal;
    nx->Swizzle = x->Swizzle;
    nx->Variable = x->Variable;
    nx->Member = x->Member;
    nx->Intrinsic = x->Intrinsic;
    nx->IntrinsicReplacementName = x->IntrinsicReplacementName;
    nx->CondName = x->CondName;
    nx->CondMember = x->CondMember;
    nx->FuncName = x->FuncName;

    return nx;
}

/****************************************************************************/
/****************************************************************************/

