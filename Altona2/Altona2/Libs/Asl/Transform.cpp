/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Transform.hpp"

using namespace Altona2;
using namespace Asl;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sTransform::sTransform()
{
    Unload();
}

sTransform::~sTransform()
{
}


void sTransform::Load(sTree *tree,sScanner *scan)
{
    Tree = tree;
    Scan = scan;
    InMul = Tree->MakeIntrinsic("mul",2,0,0);
    InDot = Tree->MakeIntrinsic("dot",2,0,0);
}

bool sTransform::Transform(sTarget tar)
{
    Common();

    bool isGl = false;
    switch(tar)
    {
    case sTAR_Hlsl5: Hlsl5(); break;
    case sTAR_Hlsl3: Matrix(); Hlsl3(); break;
    case sTAR_Glsl1: Matrix(); Glsl1(); isGl = true; break;
    case sTAR_Glsl1Es: Matrix(); Glsl1(); isGl = true; break;
    }

    // handle replacement expressions

    Tree->TraverseExpr([=](sExpr **exprp)
    {
        sExpr *expr = *exprp;

        if(isGl && expr->Kind==sEX_Index && expr->Args[0]->Kind==sEX_Member && expr->Args[0]->Member && expr->Args[0]->Member->ReplaceExpr) // special case for indexing
        {
            auto mem = expr->Args[0]->Member;
            if(mem->Type->Kind==sTK_Array)
            {
                auto rex = Tree->CloneCopyExpr(mem->ReplaceExpr);
                auto rind = rex;
                if(rind->Kind==sEX_Swizzle)
                    rind = rind->Args[0];
                if(rind->Kind==sEX_Index)
                {
                    auto radd = Tree->MakeExpr(sEX_Add,rind->Args[1],expr->Args[1]);
                    radd->Type = rind->Args[1]->Type;
                    rind->Args[1] = radd;
                    *exprp = rex;
                }
            }
        }


        if(expr->Kind==sEX_Member && expr->Member && expr->Member->ReplaceExpr)
        {
            *exprp = expr->Member->ReplaceExpr;
        }
    });

    // done

    return Scan->Errors==0;
}

void sTransform::Unload()
{
    Tree = 0;
    Scan = 0;
    InMul = 0;
    InDot = 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   Common Transformation that remove ASL specific features            ***/
/***                                                                      ***/
/****************************************************************************/

int shaderbit(int s)
{
    switch(s)
    {
    case 1<<0: return 0;
    case 1<<1: return 1;
    case 1<<2: return 2;
    case 1<<3: return 3;
    case 1<<4: return 4;
    case 1<<5: return 5;
    }
    return sST_Max;
}

bool istexsam(sTypeKind tk)
{
    if(tk==sTK_TexSam1D) return true;
    if(tk==sTK_TexSam2D) return true;
    if(tk==sTK_TexSam3D) return true;
    if(tk==sTK_TexSamCube) return true;
    if(tk==sTK_TexCmp1D) return true;
    if(tk==sTK_TexCmp2D) return true;
    if(tk==sTK_TexCmp3D) return true;
    if(tk==sTK_TexCmpCube) return true;
    return false;
}

void sTransform::Common()
{
    // fix expressions

    Tree->TraverseExpr([=](sExpr **exprp)
    {
        auto expr = *exprp;
        if(expr->Kind==sEX_Dot)
        {
            expr->Kind = sEX_Intrinsic;
            expr->Intrinsic = InDot;
        }
        if(expr->Kind==sEX_Mul && (expr->Args[0]->Type->Kind==sTK_Matrix || expr->Args[1]->Type->Kind==sTK_Matrix))
        {
            expr->Kind = sEX_Intrinsic;
            expr->Intrinsic = InMul;
        }
        if(expr->Kind==sEX_Swizzle && expr->Type->Kind==sTK_Vector)
        {
            int sw[5];
            sw[0] = (expr->Swizzle>>0)&0xff;
            sw[1] = (expr->Swizzle>>8)&0xff;
            sw[2] = (expr->Swizzle>>16)&0xff;
            sw[3] = (expr->Swizzle>>24)&0xff;
            sw[4] = 0;

            int len = 0;
            while(sw[len]) len++;
            int num = 0;
            while(num<len && sw[len-num-1]>=5)
                num++;

            for(int i=0;i<len-num;i++)
                if(sw[i]>=5)
                    Scan->Error(expr->Loc,"swizzles 0 and 1 only allowed at the end of a swizzle");

            if(num>0)
            {
                auto e = Tree->MakeExpr(sEX_Construct);
                e->Type = expr->Type;
                if(num!=len)
                    e->Args.Add(expr);
                for(int i=len-num;i<len;i++)
                {
                    auto lit = Tree->MakeLiteral(expr->Type->Base);
                    switch(expr->Type->Base->Kind)
                    {
                    case sTK_Uint:
                    case sTK_Int:
                    case sTK_Bool:
                        lit->DataI[0] = sw[i]-5;
                        break;
                    case sTK_Float:
                        lit->DataF[0] = float(sw[i]-5);
                        break;
                    case sTK_Double:
                        lit->DataD[0] = double(sw[i]-5);
                        break;
                    }
                    auto elit = Tree->MakeExpr(sEX_Literal);
                    elit->Literal = lit;
                    elit->Type = lit->Type;
                    e->Args.Add(elit);
                    expr->Swizzle = expr->Swizzle & ~(0xff<<(i*8));
                }
                *exprp = e;
            }
        }
    });

    // all structures part of shader input/output must have semantics!

    for(auto td : Tree->TypeDefs)
        td->Type->Temp = false;

    for(auto func : Tree->Funcs)
    {
        if(func->Shaders!=0)
        {
            for(auto var : func->Root->Vars)
                var->Type->Temp = true;
        }
    }
    
    for(auto td : Tree->TypeDefs)
    {
        if(td->Type->Temp && td->Type->Kind==sTK_Struct)
        {
            int index = 0;
            for(auto mem : td->Type->Members)
            {
                if(mem->Semantic==sSEM_None && mem->Active)
                    mem->Semantic = sSemantic(sSEM_Tex | ((index++)<<16));
            }
        }
    }

    // SlotAuto

    int slots[sST_Max+1];
    sClear(slots);

    for(auto var : Tree->Vars)  // find first free slot per shader
    {
        if((var->Semantic&sSEM_Mask) == sSEM_Slot && istexsam(var->Type->Kind))
        {
            int index = var->Semantic >> sSEM_IndexShift;
            slots[shaderbit(var->Shaders)] = sMax(slots[shaderbit(var->Shaders)],index+1);
        }
    }

    for(auto var : Tree->Vars)  // assign
    {
        if((var->Semantic&sSEM_Mask) == sSEM_SlotAuto && istexsam(var->Type->Kind))
        {
            int index = slots[shaderbit(var->Shaders)]++;
            var->Semantic = sSemantic(sSEM_Slot | (index<<sSEM_IndexShift));
        }
        if(var->PatchSlot)
            *var->PatchSlot = var->Semantic>>sSEM_IndexShift;
    }
}

/****************************************************************************/
/***                                                                      ***/
/***   Convert Matrix Types to Vectors                                    ***/
/***                                                                      ***/
/****************************************************************************/

void sTransform::Matrix()
{
    // for variables
    /*
    Tree->TraverseVar([=](sPoolArray<sVariable *> &ar,sVariable *var)
    {
        if(var->Type->Kind==sTK_Matrix)
        {
            if(var->Storage & sST_RowMajor)
            {
                int index = ar.FindEqualIndex(var);
                sASSERT(index>=0);
                ar.Rem(var);
                auto type = Tree->MakeObject(sTK_Vector,var->Type->Base);
                type->SizeX = var->Type->SizeX;
                for(int i=0;i<var->Type->SizeY;i++)
                {
                    sString<256> buffer;
                    buffer.PrintF("_%d_%s",i,var->Name);
                    auto vvar = Tree->MakeVar(type,sPoolString(buffer),var->Storage & ~(sST_ColumnMajor|sST_RowMajor),var->Semantic);
                    var->Vector[i] = vvar;
                    ar.AddAt(vvar,index++);
                }
            }
            else
            {
                Scan->Error(var->Loc,"matrix must be row_major for matrix to vector transform");
            }
        }
    });
    */
    // for members

    sPoolArray<sMember *> hitlist; hitlist.SetPool(&Tree->Pool);
    for(auto var : Tree->TypeDefs)
    {
        hitlist.Clear();
        for(auto mem : var->Type->Members)
        {
            if(mem->Type->Kind==sTK_Matrix)
            {
                if(mem->Storage.Flags & sST_RowMajor)
                {
                    hitlist.Add(mem);
                }
                else
                {
                    Scan->Error(mem->Loc,"matrix must be row_major for matrix to vector transform");
                }
            }
        }

        for(auto mem : hitlist)
        {
            int index = var->Type->Members.FindEqualIndex(mem);
            sASSERT(index>=0);
            var->Type->Members.RemOrder(mem);
            auto type = Tree->MakeObject(sTK_Vector,mem->Type->Base);
            type->SizeX = mem->Type->SizeX;
            for(int i=0;i<mem->Type->SizeY;i++)
            {
                sString<256> buffer;
                buffer.PrintF("_%d_%s",i,mem->Name);
                auto vmem = Tree->MakeMember(type,sPoolString(buffer),sStorage(mem->Storage.Flags & ~(sST_ColumnMajor|sST_RowMajor)),mem->Semantic);
                vmem->Offset = mem->Offset + i*4;
                mem->Vector[i] = vmem;
                var->Type->Members.AddAt(vmem,index++);
            }
        }
    }

    // find multiplications

    Tree->TraverseExprWithStmt([=](sExpr **exprp,sStmt *root,sStmt *stmt)
    {
        auto expr = *exprp;

        if(expr->Kind==sEX_Intrinsic && expr->Intrinsic->Name=="mul" && expr->Args.GetCount()==2)
        {
            auto ea = expr->Args[0];
            auto eb = expr->Args[1];
            if(ea->Type->Kind==sTK_Matrix && 
                (ea->Kind==sEX_Variable && ea->Variable->Vector[0]) || 
                (ea->Kind==sEX_Member && ea->Member->Vector[0]))
            {
                auto temp = Tree->MakeTemp(root,stmt,eb);
                auto e = Tree->MakeExpr(sEX_Construct);
                e->Type = expr->Type;
                for(int i=0;i<ea->Type->SizeY;i++)
                {
                    auto ev = Tree->MakeExpr(ea->Kind);
                    if(ea->Kind==sEX_Variable)
                        ev->Variable = ea->Variable->Vector[i];
                    if(ea->Kind==sEX_Member)
                        ev->Member = ea->Member->Vector[i];
                    auto ed = Tree->MakeExpr(sEX_Intrinsic,temp,ev);
                    ed->Intrinsic = InDot;
                    e->Args.Add(ed);
                }
                *exprp = e;
                return;
            }

            if(eb->Type->Kind==sTK_Matrix && 
                (eb->Kind==sEX_Variable && eb->Variable->Vector[0]) || 
                (eb->Kind==sEX_Member && eb->Member->Vector[0]))
            {
                auto temp = Tree->MakeTemp(root,stmt,ea);
                sExpr *ex[4];
                int swizzlemask[5] = { 0,0,0xffff,0xffffff,(int)(0xffffffff) };
                sASSERT(eb->Type->SizeY>=2);

                for(int i=0;i<eb->Type->SizeY;i++)
                {
                    auto ev = Tree->MakeExpr(eb->Kind);
                    if(eb->Kind==sEX_Variable)
                        ev->Variable = eb->Variable->Vector[i];
                    if(eb->Kind==sEX_Member)
                        ev->Member = eb->Member->Vector[i];
                    auto es  = Tree->MakeExpr(sEX_Swizzle,temp);
                    es->Swizzle = (0x01010101 * (i+1)) & swizzlemask[eb->Type->SizeY];
                    ex[i] = Tree->MakeExpr(sEX_Mul,es,ev);
                }

                auto e = Tree->MakeExpr(sEX_Add,ex[0],ex[1]);
                for(int i=2;i<eb->Type->SizeY;i++)
                    e = Tree->MakeExpr(sEX_Add,e,ex[i]);

                *exprp = e;
                return;
            }
        }
    });
}

/****************************************************************************/
/***                                                                      ***/
/***   Shader Specific Transformations                                    ***/
/***                                                                      ***/
/****************************************************************************/

void sTransform::Hlsl5()
{
    for(auto var : Tree->Vars)
    {
        auto tex = sTK_Error;
        int stex = 0;
        sTypeKind sk = sTK_Error;
        switch(var->Type->Kind)
        {
        case sTK_TexSam1D:   tex = sTK_Texture1D; sk = sTK_SamplerState; break;
        case sTK_TexSam2D:   tex = sTK_Texture2D; sk = sTK_SamplerState; break;
        case sTK_TexSam3D:   tex = sTK_Texture3D; sk = sTK_SamplerState; break;
        case sTK_TexCmp1D:   tex = sTK_Texture1D; sk = sTK_SamplerCmpState; break;
        case sTK_TexCmp2D:   tex = sTK_Texture2D; sk = sTK_SamplerCmpState; break;
        case sTK_TexCmp3D:   tex = sTK_Texture3D; sk = sTK_SamplerCmpState; break;
//        case sTK_TexSamCube: tex = sTK_TextureCube; break;
        case sTK_Buffer:            stex = sSEM_Texture; break;
        case sTK_ByteBuffer:        stex = sSEM_Texture; break;
        case sTK_StructBuffer:      stex = sSEM_Texture; break;
        case sTK_RWBuffer:          stex = sSEM_Uav; break;
        case sTK_RWByteBuffer:      stex = sSEM_Uav; break;
        case sTK_RWStructBuffer:    stex = sSEM_Uav; break;
        case sTK_AppendBuffer:      stex = sSEM_Uav; break;
        case sTK_ConsumeBuffer:     stex = sSEM_Uav; break;
        }
        if(sk != sTK_Error)
        {
            sString<256> str; str.PrintF("_tex_%s",var->Name);
            int index = var->Semantic & 0xffff0000;
            var->Type = Tree->MakeType(tex);
            var->Semantic = sSemantic(index | sSEM_Texture);
            auto sam = Tree->MakeVar(Tree->MakeType(sk),(const char *)str,0,true,sSemantic(index | sSEM_Sampler));
            var->TempReplace = sam;
            sam->Shaders = var->Shaders;
            Tree->Vars.Add(sam);
        }
        if(stex)
        {
            var->Semantic = sSemantic( (var->Semantic & ~sSEM_Mask) | stex );
        }
    }

    Tree->TraverseExpr([=](sExpr **exprp)
    {
        auto expr = *exprp;
        if(expr->Kind == sEX_Member && expr->Args.GetCount()==2 && expr->Args[1]->Kind==sEX_Intrinsic)
        {
            if(expr->Args[1]->Intrinsic->Flags & sTAF_Hlsl5Sampler)
            {
                if(expr->Args[0]->Kind==sEX_Variable && expr->Args[0]->Variable->TempReplace)
                {
                    auto e = Tree->MakeExpr(sEX_Variable);
                    e->Variable = expr->Args[0]->Variable->TempReplace;
                    expr->Args[1]->Args.AddAt(e,0);
                }
            }
        }
    });

    for(auto var : Tree->Vars)
        var->TempReplace = 0;

    Tree->TraverseExpr([=](sExpr **exprp)
    {
        auto expr = *exprp;
        auto tex = sTK_Error;
        switch(expr->Type->Kind)
        {
        case sTK_TexSam1D: tex = sTK_Texture1D; break;
        case sTK_TexSam2D: tex = sTK_Texture2D; break;
        case sTK_TexSam3D: tex = sTK_Texture3D; break;
        case sTK_TexCmp1D: tex = sTK_Texture1D; break;
        case sTK_TexCmp2D: tex = sTK_Texture2D; break;
        case sTK_TexCmp3D: tex = sTK_Texture3D; break;
        }
        if(tex != sTK_Error)
        {
            expr->Type = Tree->MakeType(tex);
        }

    });
}

/****************************************************************************/

void sTransform::Hlsl3()
{
    // texure sampling

    Tree->TraverseExpr([=](sExpr **exprp)
    {
        auto expr = *exprp;
        if(expr->Kind == sEX_Member && expr->Args.GetCount()==2 && expr->Args[1]->Kind==sEX_Intrinsic)
        {
            if(expr->Args[1]->Intrinsic->Flags & sTAF_Hlsl5Sampler)
            {
                sString<64> replacement;
                int swizzle = 0;
                bool mergeargs = false;
                if(expr->Args[0]->Type->Kind == sTK_TexSam1D)
                {
                    replacement = "tex1D";
                    swizzle = 0x010101;
                }
                else if(expr->Args[0]->Type->Kind == sTK_TexSam2D)
                {
                    replacement = "tex2D";
                    swizzle = 0x020201;
                }
                else if(expr->Args[0]->Type->Kind == sTK_TexSamCube)
                {
                    replacement = "texCUBE";
                    swizzle = 0x030201;
                }
                else if(expr->Args[0]->Type->Kind == sTK_TexSam3D)
                {
                    replacement = "tex3D";
                    swizzle = 0x030201;
                }
                if(expr->Args[0]->Type->Kind == sTK_TexCmp1D)
                {
                    replacement = "tex1D";
                    swizzle = 0x010101;
                }
                else if(expr->Args[0]->Type->Kind == sTK_TexCmp2D)
                {
                    replacement = "tex2D";
                    swizzle = 0x020201;
                }
                else if(expr->Args[0]->Type->Kind == sTK_TexCmpCube)
                {
                    replacement = "texCUBE";
                    swizzle = 0x030201;
                }
                else if(expr->Args[0]->Type->Kind == sTK_TexCmp3D)
                {
                    replacement = "tex3D";
                    swizzle = 0x030201;
                }
                else
                {
                    Scan->Error(expr->Loc,"unrecognised texture sample operation");
                    return;
                }

                if(expr->Args[1]->Intrinsic->Name=="Sample")
                {
                    ;
                }
                else if(expr->Args[1]->Intrinsic->Name=="SampleLevel")
                {
                    mergeargs = true;
                }
                else if(expr->Args[1]->Intrinsic->Name=="SampleBias")
                {
                    replacement.Add("bias");
                    mergeargs = true;
                }
                else if(expr->Args[1]->Intrinsic->Name=="SampleGrad")
                {
                    replacement.Add("grad");
                }
                else
                {
                    Scan->Error(expr->Loc,"unrecognised texture sample operation");
                    return;
                }

                if(mergeargs && expr->Args[1]->Args.GetCount()==2)
                {
                    auto es = Tree->MakeExpr(sEX_Swizzle,expr->Args[1]->Args[0]);
                    es->Swizzle = swizzle;
                    auto ec = Tree->MakeExpr(sEX_Construct,es,expr->Args[1]->Args[1]);
                    expr->Args[1]->Args.Clear();
                    expr->Args[1]->Args.Add(ec);
                }

                expr->Args[1]->IntrinsicReplacementName = replacement;
                expr->Args[1]->Args.AddAt(expr->Args[0],0);
                *exprp = expr->Args[1];
            }
        }
    });

    for(auto var : Tree->Vars)
    {
        auto tex = sTK_Error;
        switch(var->Type->Kind)
        {
        case sTK_TexSam1D:   tex = sTK_SamplerState; break;
        case sTK_TexSam2D:   tex = sTK_SamplerState; break;
        case sTK_TexSamCube: tex = sTK_SamplerState; break;
        case sTK_TexSam3D:   tex = sTK_SamplerState; break;
        case sTK_TexCmp1D:   tex = sTK_SamplerCmpState; break;
        case sTK_TexCmp2D:   tex = sTK_SamplerCmpState; break;
        case sTK_TexCmpCube: tex = sTK_SamplerCmpState; break;
        case sTK_TexCmp3D:   tex = sTK_SamplerCmpState; break;
        }
        if(tex != sTK_Error)
        {
            int index = var->Semantic & 0xffff0000;
            var->Type = Tree->MakeType(tex);
            var->Semantic = sSemantic(index | sSEM_Sampler);
        }
    }

    Tree->TraverseExpr([=](sExpr **exprp)
    {
        auto expr = *exprp;
        auto tex = sTK_Error;
        switch(expr->Type->Kind)
        {
        case sTK_TexSam1D:   tex = sTK_SamplerState; break;
        case sTK_TexSam2D:   tex = sTK_SamplerState; break;
        case sTK_TexSamCube: tex = sTK_SamplerState; break;
        case sTK_TexSam3D:   tex = sTK_SamplerState; break;
        case sTK_TexCmp1D:   tex = sTK_SamplerCmpState; break;
        case sTK_TexCmp2D:   tex = sTK_SamplerCmpState; break;
        case sTK_TexCmpCube: tex = sTK_SamplerCmpState; break;
        case sTK_TexCmp3D:   tex = sTK_SamplerCmpState; break;
        }
        if(tex != sTK_Error)
        {
            expr->Type = Tree->MakeType(tex);
        }
    });

    // discard

    Tree->TraverseStmt([=](sStmt *stmt)
    {
        if(stmt->Kind == sST_Discard)
        {
            stmt->Kind = sST_Expr;
            stmt->Expr = Tree->MakeExpr(sEX_Intrinsic);
            stmt->Expr->Intrinsic = Tree->MakeIntrinsic("clip",1,0,0);
            auto ex = Tree->MakeExprFloat(-1);
            stmt->Expr->Args.Add(ex);
        }
    });

    // constant buffer packing

    sPoolArray<sMember *> hitlist; hitlist.SetPool(&Tree->Pool);
    
    for(auto var : Tree->TypeDefs)          // find fields that need packing and replace
    {
        if(var->Type->Kind==sTK_CBuffer)
        {
            struct BadOffset
            {
                int Index;
                int Fields;
                sMember *Replacement;
                int Offset;
            };
            sArray<BadOffset> BadOffsets;
            BadOffset bo;
            bo.Index = -1;

            for(auto m : var->Type->Members)
            {
                if(m->Offset & 3)
                {
                    int index = m->Offset/4;
                    if(bo.Index!=index && bo.Index>=0)
                        BadOffsets.Add(bo);
                    bo.Index = index;
                    bo.Fields = (m->Offset & 3)+m->Type->GetSize(Tree);
                    bo.Replacement = 0;
                    bo.Offset = m->Offset & ~3;
                    sASSERT(bo.Fields<=4);
                    BadOffsets.Add(bo);
                }
            }

            if(BadOffsets.IsEmpty())
                continue;

            hitlist.Clear();
            int boi = 0;
            int max = BadOffsets.GetCount();
            for(auto m : var->Type->Members)
            {
                while(boi<max && m->Offset/4>BadOffsets[boi].Index)
                    boi++;
                if(boi>=max)
                    break;

                if(m->Offset/4==BadOffsets[boi].Index)
                    hitlist.Add(m);
            }

            boi = 0;
            for(auto m : hitlist)
            {
                while(boi<max && m->Offset/4>BadOffsets[boi].Index)
                    boi++;
                if(boi>=max)
                    break;

                if(!BadOffsets[boi].Replacement)
                {
                    sString<64> tempname;
                    tempname.PrintF("_%d",Tree->TempIndex++);
                    auto nm = Tree->MakeMember(Tree->MakeVector(sTK_Float,BadOffsets[0].Fields),sPoolString(tempname),sStorage());
                    nm->Offset = BadOffsets[boi].Offset;
                    BadOffsets[boi].Replacement = nm;
                    var->Type->Members.AddAt(nm,var->Type->Members.FindEqualIndex(m));
                }

                auto em = Tree->MakeExpr(sEX_Member);
                em->Type = BadOffsets[boi].Replacement->Type;
                em->Member = BadOffsets[boi].Replacement;
                em->Loc = m->Loc;
                auto es = Tree->MakeExpr(sEX_Swizzle,em);
                es->Type = m->Type;
                es->Swizzle = sTree::MakeSwizzle(m->Offset & 3,m->Type->GetSize(Tree));
                es->Loc = m->Loc;
                m->ReplaceExpr = es;
            }

            for(auto m : hitlist)
                var->Type->Members.RemOrder(m);
        }
    }

    // CBuffer to variables

    for(auto root : Tree->TypeDefs)
    {
        root->Temp = false;
        if(root->Type->Kind==sTK_CBuffer)
        {
            root->Temp = true;
            /*
            auto atype = Tree->MakeArray(Tree->FindType("float4"),root->Type->StructSize/4);

            sString<64> name;
            name.PrintF("%cc%d",root->Shaders==(1<<sST_Vertex) ? 'v' : 'p',root->Type->Semantic>>sSEM_IndexShift);
            auto var = Tree->MakeVar(atype,sPoolString(name),sST_Uniform);
            var->Type = atype;
            Tree->Vars.Add(var);

            auto ev = Tree->MakeExpr(sEX_Variable);
            ev->Variable = var;
            ev->Loc = root->Loc;
            ev->Type = atype;
            */
            for(auto mem : root->Type->Members)
            {
                if(mem->Active)
                {
                    auto var = Tree->MakeVar(mem->Type,mem->Name,mem->Storage,false,sSemantic(sSEM_Const|((mem->Offset/4)<<sSEM_IndexShift)));
                    var->Loc = mem->Loc;
                    var->Shaders = root->Shaders;
                    Tree->Vars.Add(var);
                    auto ev = Tree->MakeExpr(sEX_Variable);
                    ev->Variable = var;
                    ev->Loc = mem->Loc;
                    ev->Type = var->Type;                    
                    mem->ReplaceExpr = ev;
                }
            }
        }
    }
    Tree->TypeDefs.RemIfOrder([](const sTypeDef *td){ return td->Temp; });
}

/****************************************************************************/

void sTransform::Glsl1()
{
    // find main and extract attributes etc.

    sPoolArray<sVariable *> hitlist; hitlist.SetPool(&Tree->Pool);
    for(int shader=0;shader<sST_Max;shader++)
    {
        auto func = Tree->Funcs.Find([=](const sFunction *f){ return f->Name=="main" && (f->Shaders & (1<<shader)); });
        if(func)
        {
            hitlist.Clear();
            for(auto v : func->Root->Vars)
            {
                // attributes

                if((v->Storage.Flags & sST_In) && shader == sST_Vertex)
                {
                    Glsl1Attr(v,sST_Attribute);
                    hitlist.Add(v);
                }

                // varying

                if( ((v->Storage.Flags & sST_Out) && shader == sST_Vertex) ||
                    ((v->Storage.Flags & sST_In) && shader == sST_Pixel))
                {
                    Glsl1Attr(v,sST_Varying);
                    hitlist.Add(v);
                }

                // ps output

                if((v->Storage.Flags & sST_Out) && shader == sST_Pixel)
                {
                    Glsl1Attr(v,0);
                    hitlist.Add(v);
                }

            }

            for(auto v : hitlist)
            {
                func->Root->Vars.RemOrder(v);
                Tree->TypeDefs.RemIfOrder([=](const sTypeDef *td){ return td->Type==v->Type; });
            }
        }
    }

    // CBuffer to Uniform

    for(auto root : Tree->TypeDefs)
    {
        root->Temp = false;
        if(root->Type->Kind==sTK_CBuffer)
        {
            root->Temp = true;

            auto atype = Tree->MakeArray(Tree->FindType("float4"),Tree->MakeExprInt(root->Type->StructSize/4));

            sString<64> name;
            name.PrintF("%cc%d",root->Shaders==(1<<sST_Vertex) ? 'v' : 'p',root->Type->Semantic>>sSEM_IndexShift);
            auto var = Tree->MakeVar(atype,sPoolString(name),sST_Uniform,false,sSEM_None);
            var->Type = atype;
            var->Shaders = root->Shaders;
            Tree->Vars.Add(var);

            auto ev = Tree->MakeExpr(sEX_Variable);
            ev->Variable = var;
            ev->Loc = root->Loc;
            ev->Type = atype;
            for(auto mem : root->Type->Members)
            {
                auto expr = Tree->MakeExpr(sEX_Index,ev,Tree->MakeExprInt(mem->Offset/4));
                expr->Loc = mem->Loc;
                expr->Type = atype->Base;
                int len = mem->Type->GetVectorSize(Tree);
                if(len!=4)
                {
                    expr = Tree->MakeExpr(sEX_Swizzle,expr);
                    expr->Swizzle = sTree::MakeSwizzle(mem->Offset&3,len);
                    expr->Loc = mem->Loc;
                    expr->Type = mem->Type;
                }
                mem->ReplaceExpr = expr;
            }
        }
    }
    Tree->TypeDefs.RemIfOrder([](const sTypeDef *td){ return td->Temp; });

    // scalar swizzle

    Tree->TraverseExpr([=](sExpr **exprp)
    {
        auto expr = *exprp;

        if(expr->Kind==sEX_Swizzle && expr->Args[0]->Type->IsBaseType())
        {
            expr->Kind = sEX_Construct;
            expr->Swizzle = 0;
        }
    });

    // intrinsics

    Tree->TraverseExpr([=](sExpr **exprp)
    {
        auto expr = *exprp;
        if(expr->Kind==sEX_Intrinsic && expr->Intrinsic->Name=="saturate")
        {
            auto arg = expr->Args[0];
            auto type = arg->Type;
            expr->Args.Clear();
            expr->Intrinsic = Tree->MakeIntrinsic("max",2,0,0);
            auto exmin = Tree->MakeExpr(sEX_Intrinsic);
            exmin->Intrinsic = Tree->MakeIntrinsic("min",2,0,0);
            bool isfloat = expr->Type->CheckBaseType(sBT_Float|sBT_Double);
            auto ex0 = isfloat ? Tree->MakeExprFloat(0) : Tree->MakeExprInt(0);
            auto ex1 = isfloat ? Tree->MakeExprFloat(1) : Tree->MakeExprInt(1);
            exmin->Type = type;

            if(expr->Type->Kind==sTK_Vector)
            {
                ex0 = Tree->MakeExpr(sEX_Construct,ex0);
                ex0->Type = type;
                ex1 = Tree->MakeExpr(sEX_Construct,ex1);
                ex1->Type = type;
            }

            expr->Args.Add(exmin);
            expr->Args.Add(ex0);
            exmin->Args.Add(arg);
            exmin->Args.Add(ex1);
        }
        if(expr->Kind==sEX_Intrinsic && expr->Intrinsic->Name=="lerp")
        {
            expr->Intrinsic = Tree->MakeIntrinsic("mix",3,0,0);
        }
    });


    // samplers

    // texure sampling

    Tree->TraverseExpr([=](sExpr **exprp)
    {
        auto expr = *exprp;
        if(expr->Kind == sEX_Member && expr->Args.GetCount()==2 && expr->Args[1]->Kind==sEX_Intrinsic)
        {
            if(expr->Args[1]->Intrinsic->Flags & sTAF_Hlsl5Sampler)
            {
                sString<64> replacement;
                int swizzle = 0;
                bool mergeargs = false;
                if(expr->Args[0]->Type->Kind == sTK_TexSam1D || expr->Args[0]->Type->Kind == sTK_TexCmp1D)
                {
                    replacement = "texture1D";
                    swizzle = 0x010101;
                }
                else if(expr->Args[0]->Type->Kind == sTK_TexSam2D || expr->Args[0]->Type->Kind == sTK_TexCmp2D)
                {
                    replacement = "texture2D";
                    swizzle = 0x020201;
                }
                else if(expr->Args[0]->Type->Kind == sTK_TexSam3D || expr->Args[0]->Type->Kind == sTK_TexCmp3D)
                {
                    replacement = "texture3D";
                    swizzle = 0x030201;
                }
                else if(expr->Args[0]->Type->Kind == sTK_TexSamCube || expr->Args[0]->Type->Kind == sTK_TexCmpCube)
                {
                    replacement = "textureCube";
                    swizzle = 0x030201;
                }
                else
                {
                    Scan->Error(expr->Loc,"unrecognised texture sample operation");
                    return;
                }

                if(expr->Args[1]->Intrinsic->Name=="Sample")
                {
                    ;
                }
                else if(expr->Args[1]->Intrinsic->Name=="SampleLevel")
                {
                    ;
                }
                else
                {
                    Scan->Error(expr->Loc,"unrecognised texture sample operation");
                    return;
                }

                if(mergeargs && expr->Args[1]->Args.GetCount()==2)
                {
                    auto es = Tree->MakeExpr(sEX_Swizzle,expr->Args[1]->Args[0]);
                    es->Swizzle = swizzle;
                    auto ec = Tree->MakeExpr(sEX_Construct,es,expr->Args[1]->Args[1]);
                    expr->Args[1]->Args.Clear();
                    expr->Args[1]->Args.Add(ec);
                }

                expr->Args[1]->IntrinsicReplacementName = replacement;
                expr->Args[1]->Args.AddAt(expr->Args[0],0);
                *exprp = expr->Args[1];
            }
        }
    });

    for(auto var : Tree->Vars)
    {
        auto tex = sTK_Error;
        switch(var->Type->Kind)
        {
        case sTK_TexSam2D: tex = sTK_Sampler2D; break;
        case sTK_TexSamCube: tex = sTK_SamplerCube; break;
        }
        if(tex != sTK_Error)
        {
            sString<256> name; name.PrintF("%s_%d",var->Name,var->Semantic>>sSEM_IndexShift);
            var->Name = name;
            var->Type = Tree->MakeType(tex);
            var->Semantic = sSEM_None;
            var->Storage.Flags |= sST_Uniform;
        }
    }

    Tree->TraverseExpr([=](sExpr **exprp)
    {
        auto expr = *exprp;
        auto tex = sTK_Error;
        switch(expr->Type->Kind)
        {
        case sTK_TexSam2D: tex = sTK_Sampler2D; break;
        case sTK_TexSamCube: tex = sTK_SamplerCube; break;
        }
        if(tex != sTK_Error)
        {
            expr->Type = Tree->MakeType(tex);
        }
    });
}

void sTransform::Glsl1Attr(sVariable *root,int storage)
{
    int index = 0;
    for(auto mem : root->Type->Members)
    {
        sString<256> name;
        name = mem->Name;
        bool predef = false;
        if(mem->Active)
        {
            if(storage==sST_Attribute)
            {
                name.AddF("_%d",index);
            }
            else
            {
                switch(mem->Semantic&sSEM_Mask)
                {
                case sSEM_Color:
                    name.PrintF("COLOR%d",mem->Semantic>>sSEM_IndexShift);
                    break;
                case sSEM_Tex:
                    name.PrintF("TEXCOORD%d",mem->Semantic>>sSEM_IndexShift);
                    break;
                case sSEM_PositionT:
                    name.PrintF("gl_Position");
                    predef = true;
                    break;
                case sSEM_Target:
                    name.PrintF("gl_FragColor");
                    predef = true;
                    break;
                default:
                    name.AddF("_%d",index);
                    break;
                }
            }

            auto var = Tree->MakeVar(mem->Type,sPoolString(name),sStorage(mem->Storage.Flags | storage),false,sSEM_None);
            var->Shaders = root->Shaders;
            if(!predef)
                Tree->Vars.Add(var);

            auto expr = Tree->MakeExpr(sEX_Variable);
            expr->Variable = var;
            expr->Type = var->Type;
            expr->Loc = mem->Loc;
            mem->ReplaceExpr = expr;
            index++;
        }
    }
}

/****************************************************************************/

