v
#include "Altona2/Libs/Base/Base.hpp"
#include "Doc.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   output vs2012                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Document::OutputVS2012Options(mProject *pro,mFile *file,sPoolString tool)
{
    for(auto opt: file->Options)
    {
        for(auto con : pro->Configs)
        {
            sBool ok = 0;
            if(opt->Name[0]=='!')
                ok = !sMatchWildcard(opt->Name+1,con->Name,0,1);
            else
                ok = sMatchWildcard(opt->Name,con->Name,0,1);
            if(ok)
            {
                for(auto gr : opt->VSGroups)
                {
                    if(gr->Name==tool && gr->Compiler == OutputKind)
                    {
                        for(auto it : gr->VSItems)
                        {
                            const sChar *pre = 0;
                            if(it->Add)
                                pre = FindKey(pro, con->Name, tool, it->Key, OutputKind);
                            if(!pre) pre = "";

                            if(it->PlatformBits & 1)
                            {
                                vcx.PrintF("      <%s Condition=\"'$(Configuration)|$(Platform)'=='%s|Win32'\">%s%s</%s>\r\n",
                                    it->Key,con->Name,pre,it->Value,it->Key);
                            }
                            if(X64 && (it->PlatformBits & 2))
                            {
                                vcx.PrintF("      <%s Condition=\"'$(Configuration)|$(Platform)'=='%s|x64'\">%s%s</%s>\r\n",
                                    it->Key,con->Name,pre,it->Value,it->Key);
                            }
                        }
                    }
                }
            }
        }
    }
}

void Document::OutputVS2012Folder(mFolder *fol)
{
    _folders.Add(fol);
    for(auto folder : fol->Folders)
        OutputVS2012Folder(folder);
    for(auto file : fol->Files)
    {
        if(file->ToolId==mTI_cpp) _cpp.Add(file);
        else if(file->ToolId==mTI_c) _cpp.Add(file);
        else if(file->ToolId==mTI_hpp) _hpp.Add(file);
        else if(file->ToolId==mTI_none) _none.Add(file);
        else if(file->ToolId==mTI_lib) _lib.Add(file);
        else if(file->ToolId==mTI_rc) _rc.Add(file);
        else _tool.Add(file);
    }
}

void Document::OutputVS2012ItemGroup(sArray<mFile*> &arr,const sChar *tool,mFolder *root)
{
    vcxf.Print("  <ItemGroup>\r\n");
    for(auto file : arr)
    {
        if(file->Parent && file->Parent!=root)
        {
            vcxf.PrintF("    <%s Include=\"%s\">\r\n",tool,file->Name);
            vcxf.PrintF("      <Filter>%s</Filter>\r\n",file->Parent->NamePath);
            vcxf.PrintF("    </%s>\r\n",tool);
        }
        else
        {
            vcxf.PrintF("    <%s Include=\"%s\" />\r\n",tool,file->Name);
        }
    }
    vcxf.Print("  </ItemGroup>\r\n");
}

void Document::OutputVS2012Config(mProject *pro,bool x64,bool propertygroup,const sChar *x64name)
{
    sInt testbit = x64 ? 2 : 1;
    for(auto con : pro->Configs)
    {
        if(!propertygroup)
        {
            vcx.PrintF("  <ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\">\r\n",con->Name,x64name);

            cOptions *opt = con;

            for(auto gr : opt->VSGroups)
            {
                if (gr->Name[0] != '*' && gr->Compiler == OutputKind && gr->Name[0] >= 'A' && gr->Name[0] <= 'Z') // only capital names are official vs2010 name
                {
                    vcx.PrintF("    <%s>\r\n",gr->Name);
                    for(auto it : gr->VSItems)
                    {
                        if(it->PlatformBits & testbit)
                        {
                            Resolve(LargeBuffer,it->Value);
                            vcx.PrintF("      <%s>%s</%s>\r\n",it->Key,LargeBuffer,it->Key);
                        }
                    }
                    vcx.PrintF("    </%s>\r\n",gr->Name);
                }
            }
            vcx.Print ("  </ItemDefinitionGroup>\r\n");
        }
        else
        {
            cOptions *opt = con;

            for(auto gr : opt->VSGroups)
            {
                if (gr->Name[0] == '*' && gr->Compiler == OutputKind && gr->Name[1] >= 'A' && gr->Name[1] <= 'Z') // only capital names are official vs2010 name
                {
                    const sChar *name = (const sChar *)(gr->Name)+1;
                    vcx.PrintF("  <%s Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\">\r\n",name,con->Name,x64name);
                    for(auto it : gr->VSItems)
                    {
                        if(it->PlatformBits & testbit)
                        {
                            Resolve(LargeBuffer,it->Value);
                            vcx.PrintF("      <%s>%s</%s>\r\n",it->Key,LargeBuffer,it->Key);
                        }
                    }
                    vcx.PrintF("  </%s>\r\n",name);
                }
            }
        }
    }
}

sBool Document::OutputVS2012(mProject *pro)
{
    sASSERT(OutputKind == OK_VS2012 || OutputKind == OK_VS2013);

    sln.Clear();
    vcx.Clear();
    vcxf.Clear();
    sBool ok = 1;

    // set environment

    Environment.Clear();
    Set("rootpath",(const sChar *)TargetRootPath);
    Set("library",pro->Library ? "1" : "0");
    Set("compiler","vs2012");

    _cpp.Clear();
    _hpp.Clear();
    _none.Clear();
    _tool.Clear();
    _folders.Clear();
    _lib.Clear();
    _rc.Clear();

    // sort all files and folders

    OutputVS2012Folder(pro->Root);

    // solution

    sln.Print("\r\n");
    sln.Print("Microsoft Visual Studio Solution File, Format Version 12.00\r\n");
    if (OutputKind == OK_VS2013)
        sln.Print("# Visual Studio Express 2013 for Windows Desktop\r\n");
    else
        sln.Print("# Visual Studio Express 2012 for Windows Desktop\r\n");

    OutputVSSln(pro, OutputKind);

    // vcx

    vcx.Print("<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n");
    if (OutputKind == OK_VS2013)
        vcx.Print("<Project DefaultTargets=\"Build\" ToolsVersion=\"12.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\r\n");
    else
        vcx.Print("<Project DefaultTargets=\"Build\" ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\r\n");

    vcx.Print("  <ItemGroup Label=\"ProjectConfigurations\">\r\n");
    for(auto con : pro->Configs)
    {
        vcx.PrintF("    <ProjectConfiguration Include=\"%s|Win32\">\r\n",con->Name);
        vcx.PrintF("      <Configuration>%s</Configuration>\r\n",con->Name);
        vcx.Print("      <Platform>Win32</Platform>\r\n");
        vcx.Print("    </ProjectConfiguration>\r\n");
        if(X64)
        {
            vcx.PrintF("    <ProjectConfiguration Include=\"%s|x64\">\r\n",con->Name);
            vcx.PrintF("      <Configuration>%s</Configuration>\r\n",con->Name);
            vcx.Print("      <Platform>x64</Platform>\r\n");
            vcx.Print("    </ProjectConfiguration>\r\n");
        }
    }
    vcx.Print("  </ItemGroup>\r\n");

    if(_hpp.GetCount())
    {
        vcx.Print("  <ItemGroup>\r\n");
        for(auto file : _hpp)
        {
            vcx.PrintF("    <ClInclude Include=\"%s\" />\r\n",file->Name);
        }
        vcx.Print("  </ItemGroup>\r\n");
    }
    if(_cpp.GetCount())
    {
        vcx.Print("  <ItemGroup>\r\n");
        for(auto file : _cpp)
        {
            vcx.PrintF("    <ClCompile Include=\"%s\" >\r\n",file->Name);
            OutputVS2012Options(pro,file,"ClCompile");
            vcx.PrintF("    </ClCompile>\r\n");
        }
        vcx.Print("  </ItemGroup>\r\n");
    }
    if(pro->Depends.GetCount())
    {
        vcx.Print("  <ItemGroup>\r\n");
        for(auto dep : pro->Depends)
        {
            vcx.PrintF("    <ProjectReference Include=\"%s.vcxproj\" >\r\n",dep->Project->PathName);
            vcx.PrintF("      <Project>%s</Project>\r\n",dep->Project->Guid);
            vcx.PrintF("    </ProjectReference>\r\n");
        }
        vcx.Print("  </ItemGroup>\r\n");
    }
    if(_none.GetCount())
    {
        vcx.Print("  <ItemGroup>\r\n");
        for(auto file : _none)
            vcx.PrintF("    <None Include=\"%s\" />\r\n",file->Name);
        vcx.Print("  </ItemGroup>\r\n");
    }
    if(_lib.GetCount())
    {
        vcx.Print("  <ItemGroup>\r\n");
        for(auto file : _lib)
            vcx.PrintF("    <Library Include=\"%s\" />\r\n",file->Name);
        vcx.Print("  </ItemGroup>\r\n");
    }
    if(_rc.GetCount())
    {
        vcx.Print("  <ItemGroup>\r\n");
        for(auto file : _rc)
            vcx.PrintF("    <ResourceCompile Include=\"%s\" />\r\n",file->Name);
        vcx.Print("  </ItemGroup>\r\n");
    }

    if(_tool.GetCount())
    {
        sPoolString inc_name = "incbin";
        sPoolString asc_name = "asc";
        sPoolString ops_name = "ops";
        sPoolString para_name = "para";
        vcx.Print("  <ItemGroup>\r\n");
        for(auto file : _tool)
        {
            sPoolString toolname;
            if(file->ToolId==mTI_incbin) toolname = inc_name;
            if(file->ToolId==mTI_asc) toolname = asc_name;
            if(file->ToolId==mTI_ops) toolname = ops_name;
            if(file->ToolId==mTI_para) toolname = para_name;
            if(file->ToolId==mTI_packfile) toolname = "packfile";

            if(!toolname.IsEmpty())
            {
                vcx.PrintF("    <CustomBuild Include=\"%s\">\r\n",file->Name);
                vcx.PrintF("      <FileType>Document</FileType>\r\n");
                for(auto con : pro->Configs)
                {
                    for(auto gr: con->VSGroups)
                    {
                        if (gr->Name == toolname && gr->Compiler == OutputKind)
                        {
                            for(auto it : gr->VSItems)
                            {
                                Resolve(LargeBuffer,it->Value);
                                if((it->PlatformBits & 1))
                                {
                                    vcx.PrintF("      <%s Condition=\"'$(Configuration)|$(Platform)'=='%s|Win32'\">%s</%s>\r\n",
                                        it->Key,con->Name,LargeBuffer,it->Key);
                                }
                                if(X64 && (it->PlatformBits & 2))
                                {
                                    vcx.PrintF("      <%s Condition=\"'$(Configuration)|$(Platform)'=='%s|x64'\">%s</%s>\r\n",
                                        it->Key,con->Name,LargeBuffer,it->Key);
                                }
                            }
                        }
                    }

                    for(auto con2 : file->Options)
                    {
                        if(sMatchWildcard(con2->Name,con->Name,1,0))
                        {
                            for(auto gr : con2->VSGroups)
                            {
                                if (gr->Name == toolname && gr->Compiler == OutputKind)
                                {
                                    for(auto it : gr->VSItems)
                                    {
                                        Resolve(LargeBuffer,it->Value);
                                        if((it->PlatformBits & 1))
                                        {
                                            vcx.PrintF("      <%s Condition=\"'$(Configuration)|$(Platform)'=='%s|Win32'\">%s</%s>\r\n",
                                                it->Key,con->Name,LargeBuffer,it->Key);
                                        }
                                        if(X64 && (it->PlatformBits & 2))
                                        {
                                            vcx.PrintF("      <%s Condition=\"'$(Configuration)|$(Platform)'=='%s|x64'\">%s</%s>\r\n",
                                                it->Key,con->Name,LargeBuffer,it->Key);
                                        }
                                    }
                                }
                            }
                        }
                    }

                }
                if(!file->AdditionalDependencies.IsEmpty())
                {
                    vcx.PrintF("      <AdditionalInputs>%s</AdditionalInputs>\r\n",file->AdditionalDependencies);
                }
                vcx.Print ("    </CustomBuild>\r\n");
            }
        }
        vcx.Print("  </ItemGroup>\r\n");
    }


    vcx.PrintF("  <PropertyGroup Label=\"Globals\">\r\n");
    vcx.PrintF("    <ProjectGuid>%s</ProjectGuid>\r\n",pro->Guid);
    vcx.PrintF("    <Keyword>Win32Proj</Keyword>\r\n");
    vcx.PrintF("    <RootNamespace>%s</RootNamespace>\r\n",pro->Name);
    vcx.PrintF("  </PropertyGroup>\r\n");
    vcx.PrintF("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\r\n");
    /*
    vcx.PrintF("  <PropertyGroup>\r\n");
    vcx.PrintF("    <IncludePath>$(DXSDKDir)\\include;$(VCInstallDir)include;$(VCInstallDir)atlmfc\\include;$(WindowsSdkDir)include;$(FrameworkSDKDir)\\include;</IncludePath>\r\n");
    vcx.PrintF("  </PropertyGroup>\r\n");
    */

    for(auto con : pro->Configs)
    {
        vcx.PrintF("  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='%s|Win32'\" Label=\"Configuration\">\r\n",con->Name);
        vcx.PrintF("    <ConfigurationType>%s</ConfigurationType>\r\n",pro->Library ? "StaticLibrary" : "Application");
        vcx.PrintF("    <UseDebugLibraries>true</UseDebugLibraries>\r\n");
        //  vcx.PrintF("    <UseDebugLibraries>false</UseDebugLibraries>n");
        vcx.PrintF("    <CharacterSet>Unicode</CharacterSet>\r\n");
        if (OutputKind == OK_VS2013)
            vcx.PrintF("    <PlatformToolset>v120</PlatformToolset>\r\n");
        else
            vcx.PrintF("    <PlatformToolset>v110</PlatformToolset>\r\n");
        vcx.PrintF("  </PropertyGroup>\r\n");
        if(X64)
        {
            vcx.PrintF("  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='%s|x64'\" Label=\"Configuration\">\r\n",con->Name);
            vcx.PrintF("    <ConfigurationType>%s</ConfigurationType>\r\n",pro->Library ? "StaticLibrary" : "Application");
            vcx.PrintF("    <UseDebugLibraries>true</UseDebugLibraries>\r\n");
            //  vcx.PrintF("    <UseDebugLibraries>false</UseDebugLibraries>n");
            vcx.PrintF("    <CharacterSet>Unicode</CharacterSet>\r\n");
            if (OutputKind == OK_VS2013)
                vcx.PrintF("    <PlatformToolset>v120</PlatformToolset>\r\n");
            else
                vcx.PrintF("    <PlatformToolset>v110</PlatformToolset>\r\n");
            vcx.PrintF("  </PropertyGroup>\r\n");
        }
    }

    vcx.Print("    <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\r\n");
    vcx.Print("    <ImportGroup Label=\"ExtensionSettings\">\r\n");
    vcx.Print("    </ImportGroup>\r\n");

    for(auto con : pro->Configs)
    {
        vcx.PrintF("  <ImportGroup Label=\"PropertySheets\" Condition=\"'$(Configuration)|$(Platform)'=='%s|Win32'\">\r\n",con->Name);
        vcx.PrintF("    <Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />\r\n");
        vcx.PrintF("  </ImportGroup>\r\n");
        if(X64)
        {
            vcx.PrintF("  <ImportGroup Label=\"PropertySheets\" Condition=\"'$(Configuration)|$(Platform)'=='%s|x64'\">\r\n",con->Name);
            vcx.PrintF("    <Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />\r\n");
            vcx.PrintF("  </ImportGroup>\r\n");
        }
    }

    vcx.Print("  <PropertyGroup Label=\"UserMacros\" />\r\n");
    vcx.Print("  <PropertyGroup />\r\n");

    // these are the real options!

    OutputVS2012Config(pro,0,1,"Win32");
    if(X64)
        OutputVS2012Config(pro,1,1,"x64");
    OutputVS2012Config(pro,0,0,"Win32");
    if(X64)
        OutputVS2012Config(pro,1,0,"x64");

    vcx.Print("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />\r\n");
    vcx.Print("  <ImportGroup Label=\"ExtensionTargets\">\r\n");
    vcx.Print("  </ImportGroup>\r\n");
    vcx.Print("</Project>\r\n");

    // filters

    vcxf.Print("<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n");
    vcxf.Print("<Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\r\n");
    OutputVS2012ItemGroup(_cpp,"CLCompile",pro->Root);
    OutputVS2012ItemGroup(_hpp,"ClInclude",pro->Root);
    OutputVS2012ItemGroup(_tool,"CustomBuild",pro->Root);
    OutputVS2012ItemGroup(_lib,"Library",pro->Root);
    OutputVS2012ItemGroup(_rc,"ResourceCompile",pro->Root);
    OutputVS2012ItemGroup(_none,"None",pro->Root);
    vcxf.Print("  <ItemGroup>\r\n");
    for(auto folder : _folders)
    {
        if(folder!=pro->Root)
        {
            sU32 hash = sHashString(folder->Name);
            sString<64> guid;
            guid.PrintF("{%08X-0000-0000-0000-%08x0000}",pro->GuidHash,hash);

            vcxf.PrintF("    <Filter Include=\"%s\">\r\n",folder->NamePath);
            vcxf.PrintF("      <UniqueIdentifier>%d</UniqueIdentifier>\r\n",guid);
            if(folder->Parent && folder->Parent!=pro->Root)
                vcxf.PrintF("      <Filter>%s</Filter>\r\n",folder->Parent->NamePath);
            vcxf.Print("    </Filter>\r\n");
        }
    }
    vcxf.Print("  </ItemGroup>\r\n");
    vcxf.Print("</Project>\r\n");

    // write out

    if(!Pretend)
    {
        sString<sMaxPath> name;
        name.PrintF("%s.sln",pro->Name);
        if(!sSaveTextUTF8(name,sln.Get())) ok = 0;

        name.PrintF("%s.vcxproj",pro->Name);
        if(!sSaveTextUTF8(name,vcx.Get())) ok = 0;

        name.PrintF("%s.vcxproj.filters",pro->Name);
        if(!sSaveTextUTF8(name,vcxf.Get())) ok = 0;    
    }
    if(pro->Dump)
    {
        sDPrint("------------------------------------------------------------------------------\n");
        sDPrint(sln.Get());
        sDPrint("------------------------------------------------------------------------------\n");
        sDPrint(vcx.Get());
        sDPrint("------------------------------------------------------------------------------\n");
    }


    _folders.Clear();
    _cpp.Clear();
    _hpp.Clear();
    _none.Clear();
    _tool.Clear();
    _lib.Clear();
    _rc.Clear();

    return ok;
}

sBool Document::OutputVS2012()
{
    sBool ok = 1;
    for(auto sol : Solutions)
    {
        sChangeDir(sol->Path);
        for(auto pro : sol->Projects)
        {
        	for(auto dep : pro->Depends)
            {
              	for (auto opt : dep->Project->Root->Options)
                {
                    for(auto conf : pro->Configs)
                    {
                        if (sFindFirstString(conf->Name,opt->Name)!=-1)
          				{
           					AddOptions(conf, opt);
           				}
                    }
                }
            }
            
    		for (auto opt : pro->Root->Options)
            {
                for(auto conf : pro->Configs)
                {
                    if (sFindFirstString(conf->Name,opt->Name)!=-1)
          			{
                    	AddOptions(conf, opt);
                    }
                }
            }

            if (TargetProject.IsEmpty() || sCmpString(TargetProject.Get(),pro->Name.Get())==0)
                if(!OutputVS2012(pro))
                    ok = 0;
        }
    }
    return ok;
}


/****************************************************************************/

