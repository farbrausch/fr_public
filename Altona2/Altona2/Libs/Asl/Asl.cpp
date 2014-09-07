/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Base/DxShaderCompiler.hpp"
#include "Asl.hpp"
#include "Tree.hpp"
#include "Module.hpp"
#include "Transform.hpp"

using namespace Altona2;
using namespace Asl;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

Material::Material(sScanner *scan)
{
    Scan = scan;
    Virtual = false;
    Tree = 0;
    Target = sTAR_Max;
}

Material::~Material()
{
    delete Tree;
}

Material *Material::Clone()
{
    auto nm = new Material(Scan);
    nm->Name = Name;
    nm->Virtual = Virtual;
    nm->Tree = Tree->CopyTo();

    return nm;
}

int Material::GetShaderMask()
{
    return Tree->UsedShaders;
}

bool Material::ConfigureModules(int value,sTarget tar)
{
    Target = tar;

    sModules mod;
    mod.Load(Tree,Scan);
    for(auto var : Tree->Variants)
    {
        for(auto mem : var->Members)
        {
            if(var->Mask==0)
            {
                mod.UseModule(mem->Module);
            }
            else
            {
                if((value & var->Mask) == mem->Value)
                    mod.UseModule(mem->Module);
            }
        }
    }
    bool ok = mod.Transform();
    mod.Unload();

    
    return ok;
}

bool Material::Transform()
{
    sTransform trans;
    trans.Load(Tree,Scan);
    bool ok = trans.Transform(Target);
    trans.Unload();
    return ok;
}

sShaderBinary *Material::CompileBinary(sShaderTypeEnum shader,sTextLog *log,sTarget tar)
{
    sTextBuffer source;
    Tree->Dump(source,1<<int(shader),tar);
    static const char *profiles[] = { "???","%cs_5_0","%cs_3_0","???","???" };
    sString<64> profile;
    profile.PrintF(profiles[tar],((const char *)"vhdgpc")[(int)shader]);
    auto bin = sCompileShader(shader,profile,source.Get(),log);
#if 0
    sTextLog log;
    sDisassembleShaderDX(bin,&log);
    sDPrint(log.Get());
#endif

    return bin;
}

sMaterial *Material::CompileMaterial(sAdapter *ada)
{
    sTarget tar = Document::GetDefaultTarget();

    auto mtrl = new sMaterial(ada);
    for(int i=0;i<sST_Max;i++)
    {
        if((1<<i) & Tree->UsedShaders)
        {
            auto bin = CompileBinary(sShaderTypeEnum(i),0,tar);
            if(!bin)
            {
                delete mtrl;
                return 0;
            }

            auto shader = ada->CreateShader(bin);
            delete bin;
            if(!shader)
            {
                delete mtrl;
                return 0;
            }
            mtrl->SetShader(shader,sShaderTypeEnum(i));
        }
    }
    return mtrl;
}

sShader *Material::Compile(sAdapter *ada,sTextLog *log,sShaderTypeEnum shader)
{
    auto bin = CompileBinary(shader,log,Document::GetDefaultTarget());
    if(!bin)
        return 0;
    auto s = ada->CreateShader(bin);
    delete bin;
    return s;
}

sShader *Material::CompileVS(sAdapter *ada,sTextLog *log)
{
    auto bin = CompileBinary(sST_Vertex,log,Document::GetDefaultTarget());
    if(!bin)
        return 0;
    auto shader = ada->CreateShader(bin);
    delete bin;
    return shader;
}

sShader *Material::CompilePS(sAdapter *ada,sTextLog *log)
{
    auto bin = CompileBinary(sST_Pixel,log,Document::GetDefaultTarget());
    if(!bin)
        return 0;
    auto shader = ada->CreateShader(bin);
    delete bin;
    return shader;
}

sShader *Material::CompileCS(sAdapter *ada,sTextLog *log)
{
    auto bin = CompileBinary(sST_Compute,log,Document::GetDefaultTarget());
    if(!bin)
        return 0;
    auto shader = ada->CreateShader(bin);
    delete bin;
    return shader;
}

const char *Material::BaseTypeInC(sType *type)
{
    switch(type->Kind)
    {
        case sTK_Float: return "float";
        case sTK_Int:   return "int";
        case sTK_Bool:  return "bool";
        case sTK_Uint:  return "uint";
        default: Scan->Error(type->Loc,"illegal Type for constant buffer"); return "XXX";
    }
}

bool Material::GetTypeVectorSize(sPoolString name,int &size)
{
    auto td = Tree->FindTypeDef(name);
    if(!td)
        return false;
    size = td->Type->StructSize/4;
    return true;
}

bool Material::WriteHeader(sTextLog *log)
{
    log->PrintF("\n");
    log->PrintF("/****************************************************************************/\n");
    log->PrintF("\n");

    for(auto td : Tree->TypeDefs)
    {
        if(td->Type->Kind==sTK_CBuffer || td->Type->Kind==sTK_TBuffer)
        {
            int offset = 0;
            int pad = 0;

            log->PrintF("struct %s_%s\n",Name,td->Type->Name);
            log->PrintF("{\n");
            for(auto mem : td->Type->Members)
            {
                while(mem->Offset > offset)
                {
                    int delta = 1;
                    while((offset+delta)<mem->Offset && ((offset+delta)&3)!=0)
                        delta++;
                    if(delta==1)
                        log->PrintF("    float pad%d;\n",pad);
                    else
                        log->PrintF("    float%d pad%d;\n",delta,pad);
                    pad++;
                    offset += delta;
                }
                if(mem->Active)
                {
                    auto type = mem->Type;
                    while(type->Kind==sTK_Array)
                        type = type->Base;

                    switch(type->Kind)
                    {
                    case sTK_Float:
                    case sTK_Int:
                    case sTK_Bool:
                    case sTK_Uint:
                        log->PrintF("    %s %s",BaseTypeInC(type),mem->Name);
                        offset += 1;
                        break;

                    case sTK_Vector:
                        if(type->Base->Kind == sTK_Float)
                            log->PrintF("    Altona2::sVector%d %s",type->SizeX,mem->Name);
                        else
                            log->PrintF("    %s[%d] %s",BaseTypeInC(type->Base),type->SizeX,mem->Name);
                        offset += type->SizeX;
                        break;

                    case sTK_Matrix:
                        if(type->Base->Kind == sTK_Float && type->SizeX==4 && type->SizeY==4)
                            log->PrintF("    Altona2::sMatrix44 %s",mem->Name);
                        else if(type->Base->Kind == sTK_Float && type->SizeX==3 && type->SizeY==4)
                            log->PrintF("    Altona2::sMatrix44A %s",mem->Name);
                        else
                            log->PrintF("    %s[%d][%d] %s",BaseTypeInC(type->Base),type->SizeY,type->SizeX,mem->Name);
                        offset += type->SizeY*4;
                        break;
                    default:
                        Scan->Error(mem->Loc,"illegal Type for constant buffer");
                        break;
                    }

                    type = mem->Type;
                    while(type->Kind==sTK_Array)
                    {
                        log->PrintF("[%d]",Tree->ConstIntExpr(type->SizeArray,0));
                        type = type->Base;
                    }
                    log->Print(";\n");

                }
            }
            log->PrintF("};\n");
            log->PrintF("\n");
        }
    }

    log->PrintF("enum %s_PermutateEnum\n",Name);
    log->PrintF("{\n");
    for(auto v : Tree->Variants)
    {
        if(v->Mask!=0)
        {
            if(!v->Bool)
            {
                log->PrintF("    %s_%s = %08x,\n",Name,v->Name,v->Mask);
                for(auto m : v->Members)
                    log->PrintF("    %s_%s_%s = %08x,\n",Name,v->Name,m->Name,m->Value);
            }
            else
            {
                log->PrintF("    %s_%s = %08x,\n",Name,v->Name,v->Mask);
            }
        }
    }
    log->PrintF("};\n");
    log->PrintF("\n");

    return true;
}

/****************************************************************************/

Document::Document()
{
    Scan = new sScanner;
    Parser = new sParse();
}

Document::~Document()
{
    delete Scan;
    delete Parser;
    Materials.DeleteAll();
}

sTarget Document::GetDefaultTarget()
{
#if sConfigRender==sConfigRenderDX11
    return sTAR_Hlsl5;
#elif sConfigRender==sConfigRenderDX9
    return sTAR_Hlsl3;
#elif sConfigRender==sConfigRenderGL2
    return sTAR_Glsl1;
#elif sConfigRender==sConfigRenderGLES2
    return sTAR_Glsl1Es;
#endif
    return sTAR_Hlsl3;
}

void Document::ParseText(const char *source,const char *file,int line)
{
    Parser->InitScanner(Scan);
    Scan->Start(source);
    if(file)
    {
        Scan->SetFilename(file);
        if(line!=0)
            Scan->SetLine(line);
    }
    Parse();
}

void Document::ParseFile(const char *filename)
{
    Parser->InitScanner(Scan);
    Scan->StartFile(filename);
    Parse();
}

void Document::Parse()
{
    while(Scan->Token!=sTOK_End && Scan->Errors==0)
    {
        bool virt = false;
        if(Scan->IfName("virtual"))
        {
            virt = true;
        }

        if(Scan->IfName("material"))
        {
            auto mtrl = new Material(Scan);
            mtrl->Virtual = virt;
            mtrl->Name = Scan->ScanName();

            if(Scan->IfToken(':'))
            {
                auto name = Scan->ScanName();
                auto basemtrl = FindMaterial(name);
                if(basemtrl==0)
                {
                    Scan->Error("unknwon material %s",name);
                    mtrl->Tree = new sTree;
                }
                else
                {
                    mtrl->Tree = basemtrl->Tree->CopyTo();
                }
            }
            else
            {
                mtrl->Tree = new sTree;
                mtrl->Tree->Reset();
            }
            mtrl->Tree->SetScan(Scan);

            Scan->Match('{');
            Parser->Parse(Scan,false,mtrl->Tree);
            Scan->Match('}');
            virt = false;

            Materials.Add(mtrl);
        }
        else
        {
            Scan->Error("material expected");
        }
        if(virt)
            Scan->Error("virtual not allowed here");
    }
}

Material *Document::FindMaterial(sPoolString name)
{
    return Materials.Find([=](const Material *mtrl){ return mtrl->Name==name; });
}

/****************************************************************************/
