/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Doc.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   output vs2010                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Document::OutputVS2010Options(mProject *pro,mFile *file,sPoolString tool)
{
    for(auto opt : file->Options)
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
                    if(gr->Name==tool && gr->Compiler == OK_VS2010)
                    {
                        for(auto it : gr->VSItems)
                        {
                            vcx.PrintF("      <%s Condition=\"'$(Configuration)|$(Platform)'=='%s|Win32'\">%s</%s>\r\n",
                                it->Key,con->Name,it->Value,it->Key);
                        }
                    }
                }
            }
        }
    }
}

void Document::OutputVS2010Folder(mFolder *fol)
{
    for(auto folder : fol->Folders)
        OutputVS2010Folder(folder);
    for(auto file : fol->Files)
    {
        if(file->ToolId==mTI_cpp) _cpp.Add(file);
        else if(file->ToolId==mTI_c) _hpp.Add(file);
        else if(file->ToolId==mTI_hpp) _hpp.Add(file);
        else if(file->ToolId==mTI_none) _none.Add(file);
        else _tool.Add(file);
    }
}

sBool Document::OutputVS2010(mProject *pro)
{
    sln.Clear();
    vcx.Clear();
    vcxf.Clear();
    sBool ok = 1;

    // set environment

    Environment.Clear();
    Set("rootpath",(const sChar *)TargetRootPath);
    Set("library",pro->Library ? "1" : "0");
    Set("compiler","vs2010");

    // sort all files and folders

    _cpp.Clear();
    _hpp.Clear();
    _none.Clear();
    _tool.Clear();

    OutputVS2010Folder(pro->Root);

    // solution

    sln.Print("\r\n");
    sln.Print("Microsoft Visual Studio Solution File, Format Version 11.00\r\n");
    sln.Print("# Visual C++ Express 2010\r\n");

    OutputVSSln(pro,OK_VS2010);

    // vcx

    vcx.Print("<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n");
    vcx.Print("<Project DefaultTargets=\"Build\" ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\r\n");

    vcx.Print("  <ItemGroup Label=\"ProjectConfigurations\">\r\n");
    for(auto con : pro->Configs)
    {
        vcx.PrintF("    <ProjectConfiguration Include=\"%s|Win32\">\r\n",con->Name);
        vcx.PrintF("      <Configuration>%s</Configuration>\r\n",con->Name);
        vcx.Print("      <Platform>Win32</Platform>\r\n");
        vcx.Print("    </ProjectConfiguration>\r\n");
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
            OutputVS2010Options(pro,file,"ClCompile");
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
                if(!file->AdditionalDependencies.IsEmpty())
                {
                    vcx.PrintF("      <AdditionalDependencies>%s</AdditionalDependencies>\r\n",file->AdditionalDependencies);
                    vcx.PrintF("      <AdditionalInputs>%s</AdditionalInputs>\r\n",file->AdditionalDependencies);
                }
                for(auto con : pro->Configs)
                {
                    for(auto gr : con->VSGroups)
                    {
                        if(gr->Name==toolname && gr->Compiler == OK_VS2010)
                        {
                            for(auto it : gr->VSItems)
                            {
                                Resolve(LargeBuffer,it->Value);
                                vcx.PrintF("      <%s Condition=\"'$(Configuration)|$(Platform)'=='%s|Win32'\">%s</%s>\r\n",
                                    it->Key,con->Name,LargeBuffer,it->Key);
                            }
                        }
                    }
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
        vcx.PrintF("  </PropertyGroup>\r\n");
    }

    vcx.Print("    <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\r\n");
    vcx.Print("    <ImportGroup Label=\"ExtensionSettings\">\r\n");
    vcx.Print("    </ImportGroup>\r\n");

    for(auto con : pro->Configs)
    {
        vcx.PrintF("  <ImportGroup Label=\"PropertySheets\" Condition=\"'$(Configuration)|$(Platform)'=='%s|Win32'\">\r\n",con->Name);
        vcx.PrintF("    <Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />\r\n");
        vcx.PrintF("  </ImportGroup>\r\n");
    }

    vcx.Print("  <PropertyGroup Label=\"UserMacros\" />\r\n");
    vcx.Print("  <PropertyGroup />\r\n");

    // these are the real options!

    for(auto con : pro->Configs)
    {
        vcx.PrintF("  <ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='%s|Win32'\">\r\n",con->Name);

        cOptions *opt = con;

        for(auto gr : opt->VSGroups)
        {
            if(gr->Compiler == OK_VS2010 && gr->Name[0]>='A' && gr->Name[0]<='Z') // only capital names are official vs2010 name
            {
                vcx.PrintF("    <%s>\r\n",gr->Name);
                for(auto it : gr->VSItems)
                {
                    Resolve(LargeBuffer,it->Value);
                    vcx.PrintF("      <%s>%s</%s>\r\n",it->Key,LargeBuffer,it->Key);
                }
                vcx.PrintF("    </%s>\r\n",gr->Name);
            }
        }
        vcx.Print ("  </ItemDefinitionGroup>\r\n");
    }

    vcx.Print("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />\r\n");
    vcx.Print("  <ImportGroup Label=\"ExtensionTargets\">\r\n");
    vcx.Print("  </ImportGroup>\r\n");
    vcx.Print("</Project>\r\n");

    // write out

    if(!Pretend)
    {
        sString<sMaxPath> name;
        name.PrintF("%s.sln",pro->Name);
        if(!sSaveTextUTF8(name,sln.Get())) ok = 0;

        name.PrintF("%s.vcxproj",pro->Name);
        if(!sSaveTextUTF8(name,vcx.Get())) ok = 0;
        /*
        name.PrintF("%s.vcxproj.filters",pro->Name);
        if(!sSaveTextUTF8(name,vcxf.Get())) ok = 0;
        */
    }
    if(pro->Dump)
    {
        sDPrint("------------------------------------------------------------------------------\n");
        sDPrint(sln.Get());
        sDPrint("------------------------------------------------------------------------------\n");
        sDPrint(vcx.Get());
        sDPrint("------------------------------------------------------------------------------\n");
    }


    _cpp.Clear();
    _hpp.Clear();
    _none.Clear();
    _tool.Clear();

    return ok;
}

sBool Document::OutputVS2010()
{
    sBool ok = 1;
    for(auto sol : Solutions)
    {
        sChangeDir(sol->Path);
        for(auto pro : sol->Projects)
            if(!OutputVS2010(pro))
                ok = 0;
    }
    return ok;
}


/****************************************************************************/

