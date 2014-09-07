/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Module.hpp"

using namespace Altona2;
using namespace Asl;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sModules::sModules()
{
    Tree = 0;
    Scan = 0;
    CurrentUse = 0;
}

sModules::~sModules()
{
}

void sModules::Load(sTree *t,sScanner *s)
{
    Tree = t;
    Scan = s;
    Used.Clear();
    Used.SetPool(&Tree->Pool);
    for(auto mod : Tree->Modules)
        mod->OnceTemp = false;
    Index = 0;
}

void sModules::UseModule(sPoolString name)
{
    auto mod = Tree->FindModule(name);
    if(!mod)
        Scan->Error("unknown module %q",name);
    else
        UseModule(mod);
}

sModuleUse *sModules::NewModule(sModule *mod)
{
    auto use = Tree->Pool.Alloc<sModuleUse>();
    sClear(*use);
    use->Module = mod;
    use->Patches.SetPool(&Tree->Pool);
    use->Conditions.SetPool(&Tree->Pool);
    use->Conditions.CopyFrom(mod->Conditions);
    for(auto &c : mod->Conditions)
        c.Value = false;
    Used.Add(use);
    return use;
}

void sModules::UseModule(sModule *mod)
{
    if(CurrentUse)
    {
        AddDepends(CurrentUse->Module);
        CurrentUse = 0;
    }

    if(mod)
    {
        if(mod->Once && mod->OnceTemp)
        {
            // allow adding twice, but don't allow to configure the second time...
        }
        else 
        {
            CurrentUse = NewModule(mod);
            mod->OnceTemp = true;
            CurrentUse->Index = ++Index;
        }
    }
}

void sModules::CBPatch(sPoolString name,int *patch)
{
    sInject *inject = Tree->FindInject(name);
    if(!inject)
        Scan->Error("inject %q not found",name);
    else
        CBPatch(inject,patch);
}

void sModules::CBPatch(sInject *inject,int *patch)
{
    if(CurrentUse)
    {
        auto c = Tree->Pool.Alloc<sModulePatch>();
        sClear(*c);
        c->Kind = sMPK_ConstOffset;
        c->Inject = inject;
        c->FirstMember = 0;
        c->Patch = patch;
        c->Variable = 0;
        CurrentUse->Patches.Add(c);
    }
}

void sModules::VarPatch(sPoolString name,int *patch)
{
    if(CurrentUse)
    {
        for(auto var : CurrentUse->Module->Vars)
        {
            VarPatch(var,patch);
            return;
        }
    }
    Scan->Error("var %q not found",name);
}

void sModules::VarPatch(sVariable *var,int *patch)
{
    if(CurrentUse)
    {
        auto c = Tree->Pool.Alloc<sModulePatch>();
        sClear(*c);
        c->Kind = sMPK_VarSlot;
        c->Inject = 0;
        c->FirstMember = 0;
        c->Variable = var;
        c->Patch = patch;
        CurrentUse->Patches.Add(c);
    }
}

void sModules::Import(sPoolString name,int index)
{
    if(CurrentUse)
    {
        auto c = Tree->Pool.Alloc<sModulePatch>();
        sClear(*c);
        c->Kind = sMPK_Import;
        c->Name = name;
        c->Index = index;
        CurrentUse->Patches.Add(c);
    }
}

void sModules::SetCondition(sPoolString name)
{
    if(CurrentUse)
    {
        for(auto &c : CurrentUse->Conditions)
        {
            if(c.Name==name && c.Member.IsEmpty())
            {
                c.Value = 1;
                return;
            }
        }
    }
    Scan->Error("unknown condition %q",name);
}

void sModules::SetCondition(sPoolString name,int value)
{
    if(CurrentUse)
    {
        for(auto &c : CurrentUse->Conditions)
        {
            if(c.Name==name && c.Member.IsEmpty())
            {
                c.Value = value;
                return;
            }
        }
    }
    Scan->Error("unknown condition %q",name);
}

void sModules::SetCondition(sPoolString name,sPoolString member)
{
    if(CurrentUse)
    {
        for(auto &c : CurrentUse->Conditions)
        {
            if(c.Name==name && c.Member==member)
            {
                c.Value = 1;
                return;
            }
        }
    }
    Scan->Error("unknown condition %q.%q",name,member);
}

void sModules::AddDepends(sModule *mod)
{
    for(auto &dep : mod->Depends)
    {
        if(!dep.Module->OnceTemp)
        {
            if(!dep.Condition || Tree->ConstIntExpr(dep.Condition,&CurrentUse->Conditions))
            {
                NewModule(dep.Module);

                dep.Module->OnceTemp = true;
                AddDepends(dep.Module);
            }
        }
    }
}

void sModules::Unload()
{
    Tree = 0;
    Scan = 0;
    Used.Reset();
    Used.SetPool(0);
}

/****************************************************************************/

bool sModules::Transform()
{
    if(CurrentUse)
    {
        AddDepends(CurrentUse->Module);
        CurrentUse = 0;
    }

    int n = 1;
    sString<256> name;

    for(auto use : Used)
    {
        auto mod = use->Module;
        sASSERT(mod->Conditions.GetCount()==use->Conditions.GetCount())

        // insert sniplets

        for(auto snip : mod->Sniplets)
        {
            if(snip->Inject->Kind==sIK_Const || snip->Inject->Kind==sIK_Struct)
            {
                sModulePatch *patch = use->Patches.Find([=](const sModulePatch *c){ return c->Inject==snip->Inject && c->Kind==sMPK_ConstOffset; });
                auto td = Tree->TypeDefs.Find([=](const sTypeDef *i)
                    { return snip->Inject->Type==i->Type; });
                sASSERT(td);
                for(auto mem : snip->Inject->Members)
                {
                    if(mem->Module==mod)
                    {
                        if(mod->Once)
                        {
                            if(mem->Condition)
                                Scan->Error("once-module may not have conditional members");
                            mem->Active = true;
                            if(patch && !patch->FirstMember)
                                patch->FirstMember = mem;
                        }
                        else
                        {
                            if(mem->Condition==0 || Tree->ConstIntExpr(mem->Condition,&use->Conditions))
                            {
                                name.PrintF("_%d_%s",use->Index,mem->Name);
                                auto newmem = Tree->MakeMember(mem,sPoolString(name));
                                newmem->Active = true;
                                mem->ReplaceMember = newmem;
                                td->Type->Members.Add(newmem);

                                if(patch && !patch->FirstMember)
                                    patch->FirstMember = newmem;
                            }
                        }
                    }
                }
            }
        }

        // insert variables

        for(auto var : mod->Vars)
        {
            if(mod->Once)
            {
                Tree->Vars.Add(var);
            }
            else
            {
                name.PrintF("_%d_%s",use->Index,var->Name);
                auto newvar = Tree->MakeVar(var->Type,sPoolString(name),var->Storage,true,var->Semantic);
                newvar->Shaders = var->Shaders;
                var->TempReplace = newvar;
                Tree->Vars.Add(newvar);

                for(auto patch : use->Patches)
                {
                    if(patch->Kind== sMPK_VarSlot && patch->Variable == var)
                        newvar->PatchSlot = patch->Patch;
                }
            }
        }

        // fix conditionals in constant buffers

        for(auto type : Tree->Types)
        {
            for(auto m : type->Members)
            {
                if(m->Module==mod && m->Type->SizeArray)
                {
                    Tree->TraverseExpr(&m->Type->SizeArray,[&](sExpr **exprp)
                    {
                        sExpr *expr = *exprp;
                        if(expr->Kind==sEX_Condition)
                        {
                            for(auto &c : use->Conditions)
                            {
                                if(c.Name==expr->CondName && c.Member==expr->CondMember)
                                {
                                    expr->Kind = sEX_Literal;
                                    expr->Literal = Tree->MakeLiteral(Tree->FindType("int"));
                                    *expr->Literal->DataI = c.Value;
                                    break;
                                }
                            }
                            if(expr->Kind != sEX_Literal)
                                Scan->Error(expr->Loc,"unknown condition %q",expr->CondName);
                        }
                    });
                }
            }
        }

        // rename & replace

        sPoolArray<sSniplet *> snips;
        snips.SetPool(&Tree->Pool);
        Tree->TraverseStmt([&](sStmt *stmt)
        {
            if(stmt->Kind==sST_Inject)
            {
                for(auto snip : mod->Sniplets)
                {
                    if(snip->Inject==stmt->Inject)
                    {
                        Tree->CloneTag(snip->Code);
                        auto copy = Tree->CloneCopy(snip->Code);

                        // import & export

                        Tree->TraverseStmt(copy,[&](sStmt *stmt)
                        {
                            if(stmt->Kind==sST_Decl)
                            {
                                auto var = stmt->Vars[0];
                                int index = -1;
                                if(var->Storage.Flags & sST_Export)
                                {
                                    index = use->Index;
                                }
                                if(var->Storage.Flags & sST_Import)
                                {
                                    auto patch = use->Patches.Find([&](const sModulePatch *p) { return p->Kind==sMPK_Import && p->Name==var->Name; });
                                    if(patch)
                                        index = patch->Index;
                                    stmt->Kind = sST_Nop;
                                }
                                if(index!=-1)
                                {
                                    name.PrintF("_%d_%s",index,var->Name);
                                    var->TempReplace = Tree->MakeVar(var->Type,sPoolString(name),var->Storage,true,var->Semantic);
                                    var->TempReplace->Shaders = var->Shaders;
                                    var->TempReplace->Storage.Flags &= ~(sST_Import | sST_Export);
                                    if(var->Storage.Flags & sST_Import)
                                    {
                                        stmt->Vars.Clear();
                                        stmt->Kind = sST_Nop;
                                    }
                                    else
                                    {
                                        stmt->Vars[0] = var->TempReplace;
                                    }
                                }
                            }
                        });

                        // replace members, replace conditionals

                        Tree->TraverseExpr(copy,[&](sExpr **exprp)
                        {
                            sExpr *expr = *exprp;
                            if(expr->Member)
                            {
                                if(expr->Member->ReplaceMember)
                                    expr->Member = expr->Member->ReplaceMember;
                            }
                            if(expr->Kind==sEX_Condition)
                            {
                                for(auto &c : use->Conditions)
                                {
                                    if(c.Name==expr->CondName && c.Member==expr->CondMember)
                                    {
                                        expr->Kind = sEX_Literal;
                                        expr->Literal = Tree->MakeLiteral(Tree->FindType("int"));
                                        *expr->Literal->DataI = c.Value;
                                        break;
                                    }
                                }
                                if(expr->Kind != sEX_Literal)
                                    Scan->Error(expr->Loc,"unknown condition %q",expr->CondName);
                            }
                            if(expr->Kind==sEX_Variable && expr->Variable->TempReplace)
                            {
                                expr->Variable = expr->Variable->TempReplace;
                            }
                        });

                        // resolve cif

                        Tree->TraverseStmt(copy,[&](sStmt *stmt)
                        {
                            if(stmt->Kind==sST_Cif)
                            {
                                int value = Tree->ConstIntExpr(stmt->Expr,0);
                                sStmt *x = 0;
                                if(value)
                                    x = stmt->Block[0];
                                else if(stmt->Block.IsIndex(1))
                                    x = stmt->Block[1];
                                stmt->Block.Clear();
                                if(x)
                                {
                                    stmt->Block.Add(x);
                                    if(x->Kind==sST_Block)
                                        x->Kind = sST_BlockNoBrackets;
                                }
                                stmt->Kind = sST_BlockNoBrackets;
                            }
                        });

                        if(copy->Kind==sST_Block)
                            copy->Kind = sST_BlockNoBrackets;
                        Tree->TraverseStmt(copy,[&](sStmt *stmt) 
                            { for(auto v : stmt->Vars) v->TempReplace = 0; });
                        stmt->Block.Add(copy);
                    }
                }
            }
        });

        for(auto snip : mod->Sniplets)
        {
            if(snip->Inject->Kind==sIK_Const || snip->Inject->Kind==sIK_Struct)
            {
                auto td = Tree->TypeDefs.Find([=](const sTypeDef *i)
                    { return snip->Inject->Type==i->Type; });
                sASSERT(td);
                for(auto mem : snip->Inject->Members)
                    mem->ReplaceMember = 0;
            }
        }

        for(auto var : mod->Vars)
            var->TempReplace = 0;
    }

    for(auto td : Tree->TypeDefs)
        td->Type->AssignOffsets(Tree);

    for(auto use : Used)
    {
        for(auto patch : use->Patches)
        {
            if(patch->Kind==sMPK_ConstOffset && patch->FirstMember && patch->Patch)
                *patch->Patch = patch->FirstMember->Offset/4;
        }
    }

    return Scan->Errors==0;
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sAslHelper::sAslHelper(sAdapter *ada)
{
    Adapter = ada;
    Ok = true;
    Aslm = 0;
    Compiled = 0;
}

sAslHelper::~sAslHelper()
{
}


void sAslHelper::AddSource(const char *source,const char *filename)
{
    Source.PrintF("#line 1 \"%s\"\n",filename);
    Source.Print(source);
}

void sAslHelper::Parse(const char *material)
{
    Doc.ParseText(Source.Get(),0);
    if(Doc.Scan->Errors>0)
    {
        Ok = false;
        return;
    }
    Aslm = Doc.FindMaterial(material);
    if(!Aslm)
        Ok = false;
    if(Ok)
        Mod.Load(Aslm->Tree,Doc.Scan);
}


void sAslHelper::UseModule(sPoolString name)
{
    if(Ok)
        Mod.UseModule(name);
}

void sAslHelper::CBPatch(sPoolString name,int *patch)
{
    if(Ok)
        Mod.CBPatch(name,patch);
}

void sAslHelper::VarPatch(sPoolString name,int *patch)
{
    if(Ok)
        Mod.VarPatch(name,patch);
}

void sAslHelper::SetCondition(sPoolString name)
{
    if(Ok)
        Mod.SetCondition(name);
}

void sAslHelper::SetCondition(sPoolString name,sPoolString member)
{
    if(Ok)
        Mod.SetCondition(name,member);
}

void sAslHelper::SetCondition(sPoolString name,int value)
{
    if(Ok)
        Mod.SetCondition(name,value);
}

void sAslHelper::Import(sPoolString name,int index)
{
    if(Ok)
        Mod.Import(name,index);
}


void sAslHelper::Transform()
{
    if(Ok)
        Ok = Mod.Transform();
    if(Ok)
        Mod.Unload();
}

void sAslHelper::GetTypeVectorSize(sPoolString name,int &size)
{
    if(Ok)
        Aslm->GetTypeVectorSize(name,size);
}

void sAslHelper::Compile(int shadermask)
{
    if(!Compiled && Ok)
    {
        Compiled = true;
        if(Ok)
            Ok = Aslm->ConfigureModules(0,Asl::Document::GetDefaultTarget());
        if(Ok)
            Aslm->WriteHeader(&HeaderLog);
        if(Ok)
            Ok = Aslm->Transform();
    }
    ErrorLog.PrintHeader("AslErrors");
    ErrorLog.Print(Doc.Scan->ErrorLog.Get());
}

sShader *sAslHelper::CompileShader(sShaderTypeEnum shader)
{
    Compile(1<<shader);
    if(!Ok)
        return false;
    ErrorLog.PrintHeader("Compiler Errors");
    auto s = Aslm->Compile(Adapter,&ErrorLog,shader);
    if(!s)
        Ok = false;

    static const char *ShaderNames[6] = { "Vertex","Hull","Domain","Geometry","Pixel","Compute" };
    sTextLog tl;
    sString<64> str; str.PrintF("%s Shader",ShaderNames[shader]);
    ErrorLog.PrintHeader(str);
    Aslm->Tree->Dump(tl,1<<shader,Asl::Document::GetDefaultTarget());
    ErrorLog.PrintWithLineNumbers(tl.Get());

    if(!Ok)
        sDPrint(ErrorLog.Get());


    return s;
}

sMaterial *sAslHelper::CompileMtrl(int shadermask)
{
    Compile(shadermask);
    sMaterial *mtrl = 0;
    if(Ok)
    {
        mtrl = new sMaterial(Adapter);
        ErrorLog.PrintHeader("Compiler Errors");
        for(int i=0;i<sST_Max;i++)
        {
            if((1<<i) & shadermask)
            {
                auto shader = CompileShader(sShaderTypeEnum(i));
                if(shader)
                    mtrl->SetShader(shader,sShaderTypeEnum(i));
            }
        }
    }

    static const char *ShaderNames[6] = { "Vertex","Hull","Domain","Geometry","Pixel","Compute" };
    for(int i=0;i<sST_Max;i++)
    {
        if((1<<i) & shadermask)
        {
            sTextLog tl;
            sString<64> str; str.PrintF("%s Shader",ShaderNames[i]);
            ErrorLog.PrintHeader(str);
            Aslm->Tree->Dump(tl,1<<i,Asl::Document::GetDefaultTarget());
            ErrorLog.PrintWithLineNumbers(tl.Get());
        }
    }

    if(!Ok)
        sDPrint(ErrorLog.Get());

    return mtrl;
}

const char *sAslHelper::GetLog()
{
    if(HeaderLog.GetCount()>0)
    {
        ErrorLog.PrintHeader("Header");
        ErrorLog.Print(HeaderLog.Get());
        HeaderLog.Clear();
    }

    return ErrorLog.Get();
}


/****************************************************************************/

