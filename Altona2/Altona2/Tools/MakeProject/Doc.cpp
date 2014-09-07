/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Tools/MakeProject/Doc.hpp"

extern "C" sChar config_txt[];

using namespace Altona2;

/****************************************************************************/
/***                                                                      ***/
/***   helper classes (object model)                                      ***/
/***                                                                      ***/
/****************************************************************************/

cVSItem *cVSItem::Copy()
{
    cVSItem	*ret = new cVSItem();
    
    ret->PlatformBits = PlatformBits;
    ret->Add = Add;
    ret->Key = Key;
    ret->Value = Value;
    
    return ret;
}

cVSGroup *cVSGroup::Copy()
{
    cVSGroup *ret = new cVSGroup();
    
    ret->Name = Name;
    ret->Compiler = Compiler;
    
    for(auto item : VSItems)
        ret->VSItems.Add(item->Copy());
    
    return ret;
}

cVSItem *cVSGroup::AddItem(sPoolString name,int mask)
{
    for(auto it : VSItems)
    {
        if(it->Key==name && it->PlatformBits==mask)
            return it;
    }

    cVSItem *it = new cVSItem;
    it->Key = name;
    it->Value = "";
    it->PlatformBits = mask;
    VSItems.Add(it);

    return it;
}

cVSGroup *cOptions::AddGroup(sPoolString name,OutputKindEnum compiler)
{
    cVSGroup *gr;
    for(auto gr : VSGroups)
        if(gr->Name==name && gr->Compiler==compiler)
            return gr;

    gr = new cVSGroup;
    gr->Name = name;
    gr->Compiler = compiler;
    VSGroups.Add(gr);
    return gr;
}

cOptions *cOptions::Copy()
{
    cOptions *ret = new cOptions();
    
    ret->Name = Name;
    ret->Excluded = Excluded;

    for(auto g : VSGroups)
        ret->VSGroups.Add(g->Copy()) ;

    return ret;
}


/****************************************************************************/
/***                                                                      ***/
/***   document functions                                                 ***/
/***                                                                      ***/
/****************************************************************************/

Document::Document()
{
    CreateNewFiles = 0;
    SvnNewFiles = 0;
    Pretend = 0;
    OutputKind = OK_VS2010;
    Platform = 0;
    X64 = 0;
    CygwinPath = 0;
}

Document::~Document()
{
    Solutions.DeleteAll();
    OptionFrags.DeleteAll();
    Platforms.DeleteAll();
    HeaderDeps.DeleteAll();
}

void Document::Set(sPoolString key,sPoolString value)
{
    Env *v = Environment.FindValue([=](Env &i){return i.Key==key;});
    if(!v)
    {
        v = Environment.Add();
        v->Key = key;
    }
    v->Value = value;
}

void Document::Resolve(const sStringDesc &dest,const sChar *source)
{
    sChar *d = dest.Buffer;
    sChar *end = dest.Buffer+dest.Size-1;
    const sChar *s = source;

    while(d<end && *s)
    {
        if(s[0]=='\\' && s[1]!=0)
        {
            *d++ = s[1];
            s+=2;
        }
        else if(s[0]=='$' && s[1]=='(')
        {
            const sChar *t0,*t1,*a0,*a1,*b0,*b1;

            t0=t1=a0=a1=b0=b1=0;

            s+=2;
            t0 = s;
            while(*s!=0 && *s!=')' && *s!='?')
            {
                if(s[0]=='\\' && s[1]!=0)
                    s+=2;
                else
                    s++;
            }
            t1 = s;
            if(*s=='?')
            {
                s++;
                a0 = s;
                while(*s!=0 && *s!=':')
                {
                    if(s[0]=='\\' && s[1]!=0)
                        s+=2;
                    else
                        s++;
                }
                a1 = s;
            }
            if(*s==':')
            {
                s++;
                b0 = s;
                while(*s!=0 && *s!=')')
                {
                    if(s[0]=='\\' && s[1]!=0)
                        s+=2;
                    else
                        s++;
                }
                b1 = s;
            }

            if(*s!=')') 
            {
                if(Scan.Errors==0)
                    Scan.Error("unmatched environment variable in <%s>",source);
                dest.Buffer[0]=0;
                return;
            }
            else
            {
                s++;
            }

            sPoolString key(t0,t1-t0);
            Env *v = Environment.FindValue([=](Env &i){return i.Key==key;});
            if(v)
            {
                if(a0 && b0)
                {
                    if(v->Value=="1")
                    {
                        while(d<end && a0<a1)
                            *d++ = *a0++;
                    }
                    else
                    {
                        while(d<end && b0<b1)
                            *d++ = *b0++;
                    }
                }
                else
                {
                    const sChar *ss = v->Value;
                    while(d<end && *ss)
                        *d++ = *ss++;
                }
            }
            else if(Scan.Errors==0)
            {
                Scan.Error("could not find environment variable <%s>",key);
            }

        }
        else
        {
            *d++ = *s++;
        }
    }
    *d++ = 0;
}

const sChar *Document::FindKey(mProject *pro,sPoolString config,sPoolString toolname,sPoolString item,OutputKindEnum compiler)
{
    auto con = pro->Configs.Find([&](cOptions *i){ return i->Name==config; });
    if(!con) return 0;
    auto gr = con->VSGroups.Find([&](cVSGroup *i){ return i->Name==toolname && i->Compiler==compiler; });
    if(!gr) return 0;
    auto it = gr->VSItems.Find([&](cVSItem *i){ return i->Key==item; });
    if(!it) return 0;
    Resolve(LargeBuffer,it->Value);
    return LargeBuffer;
}

/****************************************************************************/

void Document::AddOptions(cOptions *cop,cOptions *opt)
{
    cVSGroup *cgr;
    cVSItem *cit;
    for(auto gr : opt->VSGroups)
    {
        cgr = cop->AddGroup(gr->Name,gr->Compiler);
        for(auto it : gr->VSItems)
        {
            cit = cgr->AddItem(it->Key,it->PlatformBits);
            if(it->Add && !cit->Value.IsEmpty())
            {
                LargeBuffer = cit->Value;
                LargeBuffer.Add(it->Value);
                cit->Value = LargeBuffer;
            }
            else
            {
                cit->Value = it->Value;
            }
        }
    }
}

/****************************************************************************/
/***                                                                      ***/
/***   Config Scanner                                                     ***/
/***                                                                      ***/
/****************************************************************************/


cOptions *Document::_C_Option()
{
    cOptions *opt = new cOptions;
    Scan.ScanNameOrString(opt->Name);
    Scan.Match('{');
    while(!Scan.Errors && !Scan.IfToken('}'))
    {
        if(Scan.IfName("group"))
        {
            sPoolString name,comp;
            Scan.ScanName(comp);      
            Scan.ScanString(name);

            OutputKindEnum compiler=OK_VS2010;
            if(comp=="vs2010")
                compiler = OK_VS2010;
            else if(comp=="vs2008")
                compiler = OK_VS2008;
            else if(comp=="make")
                compiler = OK_Make;
            else if(comp=="xcode4")
                compiler = OK_XCode4;
            else if(comp=="vs2012")
                compiler = OK_VS2012;
            else if (comp == "vs2013")
                compiler = OK_VS2013;
            else if (comp == "ndk")
                compiler = OK_NDK;      
            else
                Scan.Error("unknown compiler (like vs2010, vs2008, ..)");

            cVSGroup *gr = opt->AddGroup(name,compiler);

            Scan.Match('{');
            while(!Scan.Errors && !Scan.IfToken('}'))
            {
                if(Scan.IfName("item"))
                {
                    int bitmask = 3;
                    if(Scan.IfName("x86"))
                        bitmask = 1;
                    else if(Scan.IfName("x64"))
                        bitmask = 2;
                    Scan.ScanString(name);
                    cVSItem *it = gr->AddItem(name,bitmask);
                    it->Add = Scan.IfToken('+');
                    Scan.Match('=');
                    Scan.ScanString(it->Value);
                    Scan.Match(';');
                }
                else
                {
                    Scan.Error("keyword expected");
                }
            }
        }
        else if(Scan.IfName("exclude"))
        {
            opt->Excluded = 1;
            cVSGroup *gr = 0;
            cVSItem *it = 0;

            gr = opt->AddGroup("ClCompile", OK_VS2013);
            it = gr->AddItem("ExcludedFromBuild");
            it->Value = "true";

            gr = opt->AddGroup("CustomBuild", OK_VS2013);
            it = gr->AddItem("ExcludedFromBuild");
            it->Value = "true";

            gr = opt->AddGroup("ops", OK_VS2013);
            it = gr->AddItem("ExcludedFromBuild");
            it->Value = "true";

            gr = opt->AddGroup("para", OK_VS2013);
            it = gr->AddItem("ExcludedFromBuild");
            it->Value = "true";

            gr = opt->AddGroup("asc", OK_VS2013);
            it = gr->AddItem("ExcludedFromBuild");
            it->Value = "true";

            gr = opt->AddGroup("incbin", OK_VS2013);
            it = gr->AddItem("ExcludedFromBuild");
            it->Value = "true";

            gr = opt->AddGroup("ClCompile", OK_VS2012);
            it = gr->AddItem("ExcludedFromBuild");
            it->Value = "true";

            gr = opt->AddGroup("CustomBuild",OK_VS2012);
            it = gr->AddItem("ExcludedFromBuild");
            it->Value = "true";

            gr = opt->AddGroup("ops",OK_VS2012);
            it = gr->AddItem("ExcludedFromBuild");
            it->Value = "true";

            gr = opt->AddGroup("para",OK_VS2012);
            it = gr->AddItem("ExcludedFromBuild");
            it->Value = "true";

            gr = opt->AddGroup("asc",OK_VS2012);
            it = gr->AddItem("ExcludedFromBuild");
            it->Value = "true";

            gr = opt->AddGroup("incbin",OK_VS2012);
            it = gr->AddItem("ExcludedFromBuild");
            it->Value = "true";

            gr = opt->AddGroup("ClCompile",OK_VS2010);
            it = gr->AddItem("ExcludedFromBuild");
            it->Value = "true";

            gr = opt->AddGroup("CustomBuild",OK_VS2010);
            it = gr->AddItem("ExcludedFromBuild");
            it->Value = "true";

            gr = opt->AddGroup("",OK_VS2008);
            it = gr->AddItem("ExcludedFromBuild");
            it->Value = "true";

            Scan.Match(';');
        }
        else if(Scan.IfName("include"))
        {
            opt->Excluded = 1;
            cVSGroup *gr = 0;
            cVSItem *it = 0;

            bool add = Scan.IfToken('+');
            Scan.Match('=');
            sPoolString value = Scan.ScanString();

            gr = opt->AddGroup("ClCompile", OK_VS2013);
            it = gr->AddItem("AdditionalIncludeDirectories");
            it->Add = add;
            it->Value = value;

            gr = opt->AddGroup("ClCompile",OK_VS2012);
            it = gr->AddItem("AdditionalIncludeDirectories");
            it->Add = add;
            it->Value = value;

            Scan.Match(';');
        }
        else if(Scan.IfName("define"))
        {
            opt->Excluded = 1;
            cVSGroup *gr = 0;
            cVSItem *it = 0;

            bool add = Scan.IfToken('+');
            Scan.Match('=');
            sPoolString value = Scan.ScanString();

            gr = opt->AddGroup("ClCompile", OK_VS2013);
            it = gr->AddItem("PreprocessorDefinitions");
            it->Add = add;
            it->Value = value;

            gr = opt->AddGroup("ClCompile",OK_VS2012);
            it = gr->AddItem("PreprocessorDefinitions");
            it->Add = add;
            it->Value = value;

            Scan.Match(';');
        }
        else
        {
            Scan.Error("keyword expected");
        }
    }

    return opt;
}

cOptions *Document::_C_Combine()
{
    cOptions *cop = new cOptions;
    Scan.ScanString(cop->Name);
    if(AllConfigs.Find([=](cOptions *i){return i->Name==cop->Name;}))
        Scan.Error("duplicate config <%s>",cop->Name);
    AllConfigs.Add(cop);
    Scan.Match('=');
    do
    {
        sPoolString name;
        Scan.ScanName(name);
        cOptions *opt = OptionFrags.Find([=](cOptions *i){return i->Name==name;});
        if(opt)
        {
            AddOptions(cop,opt);
        }
        else
        {
            Scan.Error("option <%s> not found",name);
        }
    }
    while(Scan.IfToken('+'));
    Scan.Match(';');
    return cop;
}

void Document::_C_Global()
{
    while(!Scan.Errors && Scan.Token!=sTOK_End)
    {
        if(Scan.IfName("options"))
        {
            OptionFrags.Add(_C_Option());
        }
        else if(Scan.IfName("platform"))
        {
            mPlatform *p = new mPlatform;
            Platforms.Add(p);
            Scan.ScanName(p->Name);

            Scan.Match('{');
            while(!Scan.Errors && !Scan.IfToken('}'))
            {
                if(Scan.IfName("combine"))
                {
                    p->Configs.Add(_C_Combine());
                }
                else
                {
                    Scan.Error("keyword expected");
                }
            }
        }
        else if(Scan.IfName("license"))
        {
            sPoolString name = Scan.ScanName();
            sPoolString text;
            Scan.Match('{');
            while(!Scan.Errors && !Scan.IfToken('}'))
            {
                if(Scan.IfName("text"))
                {
                    Scan.Match('=');
                    text = Scan.ScanString();
                    Scan.Match(';');
                }
                else
                {
                    Scan.Error("keyword expected");
                }
            }
            Licenses.Add(name,text);
        }
        else
        {
            Scan.Error("keyword expected");
        }
    }
}

sBool Document::ScanConfig()
{
    Scan.Init(sSF_CppComment|sSF_EscapeCodes);
    Scan.AddDefaultTokens();
    //  Scan.StartFile("config.txt");
    Scan.Start(config_txt);
    Scan.SetFilename("config.txt");

    _C_Global();

    sDPrint(Scan.ErrorLog.Get());

    return Scan.Errors==0;
}

/****************************************************************************/
/***                                                                      ***/
/***   mp.txt scanner                                                     ***/
/***                                                                      ***/
/****************************************************************************/

void Document::_MP_Folder(mProject *pro,mFolder *fol)
{
    Scan.Match('{');
    while(!Scan.Errors && !Scan.IfToken('}'))
    {
        if(Scan.IfName("file"))
        {
            sBool nonew = 0;
            if(Scan.IfName("nonew"))
                nonew = 1;
            sString<sFormatBuffer> name;
            sTextLog Depends;
            Scan.ScanString(name);

            sArray<cOptions *> options;
            for(auto i : fol->Options)
            {
                i->AddRef();
                options.Add(i);
            }
            for(;;)
            {
                if(Scan.IfName("depend"))
                {
                    if(Depends.GetCount()>0)
                        Depends.Print(";");                 // this is correct for VS2012, don't know about other build platforms
                    Depends.Print(Scan.ScanString());
                }
                else if(Scan.IfName("options"))
                {
                    options.Add(_C_Option());
                }
                else
                {
                    break;
                }
            }
            Scan.Match(';');
            sPtr len = sGetStringLen(name);
            mToolId id = mTI_none;
            sPtr slen = 0;

            struct toolidinfo
            {
                mToolId tid;
                const sChar *name;
            };
            static const toolidinfo tifs[] = 
            {
                { mTI_cpp     ,".cpp" },
                { mTI_cpp     ,".cc" },
                { mTI_c       ,".c" },
                { mTI_hpp     ,".hpp" },
                { mTI_hpp     ,".h" },
                { mTI_incbin  ,".incbin" },
                { mTI_asc     ,".asc" },
                { mTI_asm     ,".asm" },
                { mTI_m       ,".m" },
                { mTI_mm      ,".mm" },
                { mTI_xib     ,".xib" },
                { mTI_packfile,".packfile" },
                { mTI_ops     ,".ops" },
                { mTI_lib     ,".lib" },
                { mTI_rc      ,".rc" },
                { mTI_xml     ,".xml" },
                { mTI_dat     ,".dat" },
                { mTI_para    ,".para" },
            };

            for(sInt i=0;i<sCOUNTOF(tifs);i++)
            {
                sPtr tl = sGetStringLen(tifs[i].name);
                if(len>tl && sCmpString(tifs[i].name,name+len-tl)==0)
                {
                    slen = tl;
                    id = tifs[i].tid;
                    break;
                }
            }

            if(len>4 && sCmpString(".?pp",name+len-4)==0)
            {
                slen = 4;
                name[len-3] = 'c';
                mFile *file = new mFile;
                file->Name = name;
                file->OriginalName = name;
                file->NameWithoutExtension.Init(name,len-slen);
                file->ToolId = mTI_cpp;
                file->NoNew = nonew;
                file->AdditionalDependencies = Depends.Get();
                file->Options.AddTail(options);
                file->FullPath.PrintF("%s/%s",pro->SolutionPath,file->Name);
                sCleanPath(file->FullPath);
                file->Project = pro;
                fol->Files.Add(file);
                file->Parent = fol;

                name[len-3] = 'h';
                file = new mFile;
                file->Name = name;
                file->OriginalName = name;
                file->NameWithoutExtension.Init(name,len-slen);
                file->ToolId = mTI_hpp;
                file->NoNew = nonew;
                file->AdditionalDependencies = Depends.Get();
                file->FullPath.PrintF("%s/%s",pro->SolutionPath,file->Name);
                sCleanPath(file->FullPath);
                
                file->Project = pro;
                fol->Files.Add(file);
                file->Parent = fol;

                if(nonew)   // these files are skipped in dependency scans
                {
                    sString<sMaxPath> buffer;
                    buffer.PrintF("%s/%s",pro->SolutionPath,name);
                    HeaderDependency *dep = new HeaderDependency;
                    dep->Filename = buffer;
                    dep->Checked = 0;
                    HeaderDeps.Add(dep);
                }
            }
            else
            {
                mFile *file = new mFile;
                file->Name = name;
                file->OriginalName = name;
                file->NameWithoutExtension.Init(name,len-slen);
                file->ToolId = id;
                file->NoNew = nonew;
                file->AdditionalDependencies = Depends.Get();
                file->Options.AddTail(options);
                file->FullPath.PrintF("%s/%s",pro->SolutionPath,file->Name);
                sCleanPath(file->FullPath);
                
                file->Project = pro;
                fol->Files.Add(file);
                file->Parent = fol;

                if(nonew && id==mTI_hpp)   // these files are skipped in dependency scans
                {
                    sString<sMaxPath> buffer;
                    buffer.PrintF("%s/%s",pro->SolutionPath,name);
                    HeaderDependency *dep = new HeaderDependency;
                    dep->Filename = buffer;
                    dep->Checked = 0;
                    HeaderDeps.Add(dep);
                }
            }
        }
        else if(Scan.IfName("folder"))
        {
            mFolder *folder = new mFolder;
            fol->Folders.Add(folder);
            folder->Parent = fol;
            for(auto i : fol->Options)
            {
                i->AddRef();
                folder->Options.Add(i);
            }

            Scan.ScanString(folder->Name);
            folder->NamePath = folder->Name;
            if(folder->Parent != pro->Root)
            {
                sString<sMaxPath> buffer;
                buffer.PrintF("%s\\%s",folder->Parent->NamePath,folder->Name);
                folder->NamePath = buffer;
            }

            _MP_Folder(pro,folder);
        }
        else if(Scan.IfName("options"))
        {
            cOptions *opt = _C_Option();
            fol->Options.Add(opt);
        }
        else if(Scan.IfName("config") && pro->Root==fol)
        {
            sPoolString name;
            Scan.ScanString(name);
            Scan.Match(';');
            cOptions *cop = AllConfigs.Find([=](cOptions *i){return i->Name==name;});
            if(cop)
            {
                if(Platform->Configs.Find([=](cOptions *i){return i->Name==name;}))   // only remember configs for current platform
                    pro->Configs.Add(cop->Copy());
            }
            else
            {
                Scan.Error("config <%s> not found",name);
            }
        }
        else if(Scan.IfName("depend") && pro->Root==fol)
        {
            mDepend *dep = new mDepend;
            Scan.ScanString(dep->Name);
            Scan.Match(';');

            sString<sMaxPath> name;
            name.PrintF("%s/%s",RootPath,dep->Name);
            dep->PathName = name;
            
            bool found = false;
            for(auto d : pro->Depends)
            {
                if (sCmpString(dep->Name, d->Name)==0)
                {
                    found = true;
                    break;
                }
            }
            
            if (found)
              delete dep;
            else
              pro->Depends.Add(dep);
            
        }
        else if(Scan.IfName("dump"))
        {
            pro->Dump = 1;
            Scan.Match(';');
        }
        else if(Scan.IfName("alias"))
        {
            Scan.ScanString(pro->Alias);
            Scan.Match(';');
        }
        else
        {
            Scan.Error("keyword expected");
        }
    }
}

void Document::_MP_Project(mSolution *sol,sBool lib)
{
    mProject *pro = new mProject;
    sol->Projects.Add(pro);
    pro->Library = lib;
    Scan.ScanString(pro->Name);

    sString<256> name;
    pro->SolutionPath = sol->Path;
    pro->TargetSolutionPath = sol->Path;
    sPtr len = sGetStringLen(RootPath);
    if(sCmpStringPLen(pro->SolutionPath,RootPath,len)==0)
    {
        name.PrintF("%s%s",TargetRootPath,pro->SolutionPath+len);
        pro->TargetSolutionPath = name;
    }

    name.PrintF("%s/%s",sol->Path,pro->Name);
    pro->PathName = name;
    sU32 hash = sHashString(name);
    pro->Guid.PrintF("{%08X-0000-0000-0000-000000000000}",hash);
    pro->GuidHash = hash;

    pro->Root = new mFolder;
    _MP_Folder(pro,pro->Root);

    if((Platform->Name!="ios") && (Platform->Name!="osx") && pro->Library)
    {
        if(pro->Configs.GetCount()==0)
        {
            for(auto cop : Platform->Configs)
            {
                pro->Configs.Add(cop->Copy());
            }
        }
    }    
}

void Document::_MP_Global(mSolution *sol)
{
    while(!Scan.Errors && Scan.Token!=sTOK_End)
    {
        if(Scan.IfName("library"))
        {
            _MP_Project(sol,1);
        }
        else if(Scan.IfName("project"))
        {
            _MP_Project(sol,0);
        }
        else if(Scan.IfName("license"))
        {
            sPoolString name = Scan.ScanName();
            sPoolString *license = Licenses.Get(name);
            if(license)
                sol->LicenseString = *license;
            else
                Scan.Error("unknown license %q",name);
            Scan.Match(';');
        }
        else
        {
            Scan.Error("keyword expected");
        }
    }
}

sBool Document::ScanFile(const sChar *path)
{
    sString<sMaxPath> newpath;
    newpath.PrintF("%s/mp.txt",path);

    mSolution *sol = new mSolution;
    Solutions.Add(sol);
    sol->Path = path;

    Scan.Init(sSF_CppComment|sSF_EscapeCodes);
    Scan.AddDefaultTokens();
    Scan.StartFile(newpath);

    _MP_Global(sol);

    sDPrint(Scan.ErrorLog.Get());

    return Scan.Errors==0;
}

sBool Document::ScanMPR(const sChar *path)
{
    sArray<sDirEntry> dir;

    if(!sLoadDir(dir,path))
        return 0;

    sBool ok = 1;

    for(auto &de : dir)
    {
        if(de.Flags & sDEF_Dir)
        {
            if(de.Name[0]!='.')
            {
                sString<sMaxPath> newpath;
                newpath.PrintF("%s/%s",path,de.Name);
                if(!ScanMPR(newpath)) ok = 0;
            }
        }
        else
        {
            if(de.Name=="mp.txt")
            {
                if(!ScanFile(path)) ok = 0;
            }
        }
    }

    return ok;
}

sBool Document::ScanMP()
{
    return ScanMPR(RootPath);
}

/****************************************************************************/
/***                                                                      ***/
/***   header dependencies                                                ***/
/***                                                                      ***/
/****************************************************************************/

HeaderDependency *Document::CheckIncludes(sPoolString filename)
{
    HeaderDependency *dthis;
    for(auto dc : HeaderDeps)
        if(sCmpStringP(filename,dc->Filename)==0)
            return dc;

    //  sPrintF("scan %s\n",filename);

    const sChar *text = sLoadText(filename);
    if(text==0)
        return 0;
    const sChar *s = text;

    dthis = new HeaderDependency;
    dthis->Filename = filename;
    dthis->Checked = 0;
    HeaderDeps.Add(dthis);

    // scan

    while(*s)
    {
        if(sCmpMem(s,"include",7)==0)
        {
            const sChar *p = s;
            while(p>=text && (p[-1]==' ' || p[-1]=='\t')) p--;
            if(p[-1]=='#')
            {
                s += 7;
                while(*s==' ' || *s=='\t') s++;
                if(*s=='"')
                {
                    HeaderDependency *dadd = 0;
                    s++;
                    sString<sMaxPath> buffer;
                    sChar *d = buffer;
                    while(*s!='"' && *s!=0 && d<buffer.Get()+sMaxPath-2)
                        *d++ = *s++;
                    *d++ = 0;
                    if(*s=='"')
                    {
                        sBool ok = 0;
                        sString<sMaxPath> path1;
                        sString<sMaxPath> path2;
                        path1.PrintF("%s/%s",RootPath,buffer);
                        dadd = CheckIncludes(sPoolString(path1));
                        if(dadd)
                            ok = 1;
                        if(!ok)
                        {
                            path2 = filename;
                            sChar *slash = 0;
                            sChar *ss = path2;
                            while(*ss)
                            {
                                if(*ss=='/')
                                    slash = ss;
                                ss++;
                            }
                            if(slash)
                            {
                                *slash = 0;
                                path2.AddF("/%s",buffer);
                                //              if(sCheckFile(path2))
                                {
                                    dadd = CheckIncludes(sPoolString(path2));
                                    if(dadd)
                                        ok = 1;
                                }
                            }
                        }
                        if(!ok)
                            sPrintF("could not scan %s for includes\n",buffer);
                        if(dadd)
                            dthis->Dep.Add(dadd);
                    }
                }
            }
            while(*s!=0 && *s!='\n')
                s++;
        }
        else
        {
            s+=1;
        }
    }

    // continue

    delete[] text;

    return dthis;
}

/****************************************************************************/

void Document::PrintIncludes(sPoolString filename,sTextLog &log)
{
    CheckIncludes(filename);

    for(auto d : HeaderDeps)
        d->Checked = 0;

    for(auto d : HeaderDeps)
    {
        if(sCmpStringP(filename,d->Filename)==0)
        {
            PrintIncludes(d,log);
            return;
        }
    }
}

void Document::PrintIncludes(HeaderDependency *dep,sTextLog &log)
{
    if(!dep->Checked)
    {
        dep->Checked = 1;

        sPtr len = sGetStringLen(RootPath);
        if(sCmpStringPLen(dep->Filename,RootPath,len)==0)
        {
            log.PrintF(" %s%s",TargetRootPath,dep->Filename+len);
        }
        else
        {
            log.PrintF(" %s",dep->Filename);
        }

        for(auto d : dep->Dep)
            PrintIncludes(d,log);
    }
}

/****************************************************************************/
/***                                                                      ***/
/***   find dependencies                                                  ***/
/***                                                                      ***/
/****************************************************************************/

sBool Document::FindDepends()
{
    sArray<mProject *> allp;

    sBool ok = 1;

    for(auto sol : Solutions)
        allp.AddTail(sol->Projects);

    for(auto pro : allp)
    {
        for(auto dep : pro->Depends)
        {
            dep->Project = allp.Find([=](mProject *i){return sCmpStringI(i->PathName,dep->PathName)==0;});
            if(!dep->Project)
                dep->Project = allp.Find([=](mProject *i){return sCmpStringI(i->Alias,dep->Name)==0;});
            if(!dep->Project)
            {
                sPrintF("could not find dependency <%s> in project <%s>\n",dep->PathName,pro->PathName);
                ok = 0;
            }
        }
    }

    return ok;
}

/****************************************************************************/
/***                                                                      ***/
/***   output vs, general                                                 ***/
/***                                                                      ***/
/****************************************************************************/

void Document::OutputVSSln(mProject *pro,OutputKindEnum compiler)
{
    const sChar *vcxproj = "vcxproj";
    if(compiler == OK_VS2008)
        vcxproj = "vcproj";


    sln.Print("Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = ");
    sln.PrintF("\"%s\", \"%s.%s\", \"%s\"\r\n",pro->Name,pro->Name,vcxproj,pro->Guid);
    if(pro->Depends.GetCount()>0)
    {
        sln.Print("\tProjectSection(ProjectDependencies) = postProject\r\n");
        for(auto dep : pro->Depends)
        {
            sASSERT(dep->Project);
            sln.PrintF("\t\t%s = %s\r\n",dep->Project->Guid,dep->Project->Guid);
        }
        sln.Print("\tEndProjectSection\r\n");
    }
    sln.Print("EndProject\r\n");

    for(auto dep : pro->Depends)
    {
        sASSERT(dep->Project);
        sln.Print("Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = ");
        sln.PrintF("\"%s\", \"%P.%s\", \"%s\"\r\n",dep->Project->Name,dep->Project->PathName,vcxproj,dep->Project->Guid);
        sln.Print("EndProject\r\n");
    }

    sln.Print("Global\r\n");

    sArray<cOptions *> dupe;

    sln.Print("\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\r\n");
    for(auto con : pro->Configs)
    {
        if(!dupe.FindEqual(con))
        {
            dupe.Add(con);
            sln.PrintF("\t\t%s|Win32 = %s|Win32\r\n",con->Name,con->Name);
            if(X64)
                sln.PrintF("\t\t%s|x64 = %s|x64\r\n",con->Name,con->Name);
        }
    }
    /*
    sFORALL(pro->Depends,dep)
    {
    sFORALL(dep->Project->Configs,con)
    {
    if(!dupe.FindPtr(con))
    {
    dupe.Add(con);
    sln.PrintF("\t\t%s|Win32 = %s|Win32\r\n",con->Name,con->Name);
    if(X64)
    sln.PrintF("\t\t%s|x64 = %s|x64\r\n",con->Name,con->Name);
    }
    }
    }
    */
    sln.Print("\tEndGlobalSection\r\n");

    dupe.Clear();
    sln.Print("\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\r\n");
    for(auto con : pro->Configs)
    {
        if(!dupe.FindEqual(con))
        {
            dupe.Add(con);
            sln.PrintF("\t\t%s.%s|Win32.ActiveCfg = %s|Win32\r\n",pro->Guid,con->Name,con->Name);
            sln.PrintF("\t\t%s.%s|Win32.Build.0 = %s|Win32\r\n",pro->Guid,con->Name,con->Name);
            if(X64)
            {
                sln.PrintF("\t\t%s.%s|x64.ActiveCfg = %s|x64\r\n",pro->Guid,con->Name,con->Name);
                sln.PrintF("\t\t%s.%s|x64.Build.0 = %s|x64\r\n",pro->Guid,con->Name,con->Name);
            }
        }
    }
    for(auto dep : pro->Depends)
    {
        for(auto con : dep->Project->Configs)
        {
            if(!dupe.FindEqual(con))
            {
                dupe.Add(con);
                sln.PrintF("\t\t%s.%s|x64.ActiveCfg = %s|x64\r\n",dep->Project->Guid,con->Name,con->Name);
                sln.PrintF("\t\t%s.%s|x64.Build.0 = %s|x64\r\n",dep->Project->Guid,con->Name,con->Name);
                if(X64)
                {
                    sln.PrintF("\t\t%s.%s|Win32.ActiveCfg = %s|Win32\r\n",dep->Project->Guid,con->Name,con->Name);
                    sln.PrintF("\t\t%s.%s|Win32.Build.0 = %s|Win32\r\n",dep->Project->Guid,con->Name,con->Name);
                }
            }
        }
    }
    sln.Print("\tEndGlobalSection\r\n");
    sln.Print("\tGlobalSection(SolutionProperties) = preSolution\r\n");
    sln.Print("\t\tHideSolutionNode = FALSE\r\n");
    sln.Print("\tEndGlobalSection\r\n");
    sln.Print("EndGlobal\r\n");
}

/****************************************************************************/
/***                                                                      ***/
/***   Other helpers for output phase                                     ***/
/***                                                                      ***/
/****************************************************************************/

void Document::OutputFolder(mFolder *fol,sArray<mFile *> &files)
{
    for(auto folder : fol->Folders)
        OutputFolder(folder,files);
    for(auto file : fol->Files)
        files.Add(file);
}

/****************************************************************************/
/***                                                                      ***/
/***   create new file                                                    ***/
/***                                                                      ***/
/****************************************************************************/

void Document::Create(mSolution *sol,mProject *pro,mFolder *fol)
{
    for(auto folder : fol->Folders)
        Create(sol,pro,folder);

    for(auto file : fol->Files)
    {
        sString<sMaxPath> path;
        path.PrintF("%s/%s",sol->Path,file->Name);
        const sChar *smallpath = path+sGetStringLen(RootPath)+1;
        sString<sMaxPath> name;
        name = file->Name;
        sChar *s = name;
        sChar *ss = 0;
        while(*s)
            if(*s++=='.') ss = s;
        if(ss)
            *ss = 0;

        if(!file->NoNew && !sCheckFile(path))
        {
            sPrintF("missing file %s\n",path);
            sTextLog txt;

            const sChar *license = sol->LicenseString.IsEmpty() ? *Licenses.Get("chaos") : sol->LicenseString;

            txt.Print("/****************************************************************************/\r\n");
            txt.Print("/***                                                                      ***/\r\n");
            while(*license)
            {
                const char *start = license;
                while(*license!=0 && *license!='\n')
                    license++;
            
                sString<256> buffer;
                buffer.Init(start,license-start);
                txt.PrintF("/***   %-64s   ***/\r\n",buffer);

                if(*license=='\n')
                    license++;
            }
            txt.Print("/***                                                                      ***/\r\n");
            txt.Print("/****************************************************************************/\r\n");

            switch(file->ToolId)
            {
            case mTI_hpp:
                {
                    sString<sMaxPath+16> label;
                    sChar *d = label;
                    *d++ = 'F';
                    *d++ = 'I';
                    *d++ = 'L';
                    *d++ = 'E';
                    *d++ = '_';
                    const sChar *s = smallpath;
                    while(*s)
                    {
                        if(*s=='/' || *s=='.' || *s=='\\')
                            *d++ = '_';
                        else if(*s>='a' && *s<='z')
                            *d++ = *s+'A'-'a';
                        else
                            *d++ = *s;
                        s++;
                    }
                    *d++ = 0;

                    txt.PrintF("\r\n");
                    txt.PrintF("#ifndef %s\r\n",label);
                    txt.PrintF("#define %s\r\n",label);
                    txt.PrintF("\r\n");
                    txt.PrintF("#include \"Altona2/Libs/Base/Base.hpp\"\r\n");
                    txt.PrintF("\r\n");
                    txt.PrintF("namespace %s {\r\n",pro->Name);
                    txt.PrintF("using namespace Altona2;\r\n");
                    txt.PrintF("\r\n");
                    txt.Print("/****************************************************************************/\r\n");
                    txt.Print("/***                                                                      ***/\r\n");
                    txt.Print("/***                                                                      ***/\r\n");
                    txt.Print("/***                                                                      ***/\r\n");
                    txt.Print("/****************************************************************************/\r\n");
                    txt.PrintF("\r\n");
                    txt.PrintF("\r\n");
                    txt.PrintF("\r\n");
                    txt.Print("/****************************************************************************/\r\n");
                    txt.PrintF("} // namespace %s\r\n",pro->Name);
                    txt.PrintF("#endif  // %s\r\n",label);
                    txt.PrintF("\r\n");
                }
                break;
            case mTI_cpp:
            case mTI_c:
            case mTI_m:
            case mTI_mm:
                {
                    txt.PrintF("\r\n");
                    txt.PrintF("#include \"Altona2/Libs/Base/Base.hpp\"\r\n");
                    txt.PrintF("#include \"%shpp\"\r\n",name);
                    txt.PrintF("\r\n");
                    txt.PrintF("using namespace Altona2;\r\n");
                    txt.PrintF("using namespace %s;\r\n",pro->Name);
                    txt.PrintF("\r\n");
                    txt.Print("/****************************************************************************/\r\n");
                    txt.Print("/***                                                                      ***/\r\n");
                    txt.Print("/***                                                                      ***/\r\n");
                    txt.Print("/***                                                                      ***/\r\n");
                    txt.Print("/****************************************************************************/\r\n");
                    txt.PrintF("\r\n");
                    txt.PrintF("\r\n");
                    txt.PrintF("\r\n");
                    txt.Print("/****************************************************************************/\r\n");
                    txt.PrintF("\r\n");
                }
                break;
            default:
                break;
            }

            sString<sMaxPath> svn;

            svn.PrintF("svn add %s",path);


            if(Pretend)
            {
                sDPrint("------------------------------------------------------------------------------\n");
                if(SvnNewFiles)
                    sDPrintF("\"%s\"\n",svn.Get());
                sDPrintF("save <%s>\n",path);
                sDPrint(txt.Get());
                sDPrint("------------------------------------------------------------------------------\n");
            }
            else
            {
                sSaveFile(path,txt.Get(),sGetStringLen(txt.Get()));
                if(SvnNewFiles)
                    sExecuteShell(svn.Get());
            }
        }
    }
}

void Document::Create()
{
    for(auto sol : Solutions)
    {
        for(auto pro : sol->Projects)
        {
            Create(sol,pro,pro->Root);
        }
    }
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Document::OutputMpx(mProject *proj,mFolder *folder,const char *foldername)
{
    Log.PrintF("    folder %q\n    {\n",foldername);
    for(auto f : folder->Files)
    {
        Log.PrintF("        file \"%s/%s\";\n",proj->SolutionPath,f->Name);
    }
    Log.PrintF("    }\n");

    for(int i=0;i<2;i++)
    {
        for(auto f : folder->Folders)
        {
            if((sCmpStringI(f->Name,"generated")==0) == i)
            {
                sString<sMaxPath> name;
                if(foldername[0])
                    name.PrintF("%s/%s",foldername,f->Name);
                else
                    name = f->Name;
                OutputMpx(proj,f,name);
            }
        }
    }
}

void Document::OutputMpx(mProject *proj)
{
    sString<sMaxPath> str;
    str.PrintF("%s/%s.sln",proj->SolutionPath,proj->Name);
    Log.PrintF("project %q %q\n{\n",proj->Name,str);
    OutputMpx(proj,proj->Root,"");
    Log.PrintF("}\n");
}

void Document::OutputMpx()
{
    for(auto sol : Solutions)
    {
        if(!sol->Projects.IsEmpty())
        {
            Log.Clear();
            for(auto proj : sol->Projects)
            {
                OutputMpx(proj);
                for(auto dep : proj->Depends)
                    OutputMpx(dep->Project);
            }
            sString<sMaxPath> name;
            name.PrintF("%s/%s.mpx.txt",sol->Projects[0]->SolutionPath,sol->Projects[0]->Name);
            sSaveTextAnsi(name,Log.Get(),1);
        }
    }
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/


/****************************************************************************/
