/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#define sPEDANTIC_WARN 1
#include "doc.hpp"
#include "base/system.hpp"

/****************************************************************************/

void Document::OutputTool(Tool *otool,sInt indent,const sChar *plat,Config *conf)
{
  Option *opt;
  OmniPath *path;
  sString<4096> buffer;

  // create temporary options

  Tool *tool = new Tool;
  tool->CopyFrom(otool);

  // modify options for 64bit

  if(!sCmpString(plat,L"x64"))
  {
    if(tool->Name==L"VCLinkerTool")
    {
      if(!sCmpString(plat,L"x64"))
        tool->SetOption(L"TargetMachine",L"17");
      else
        tool->SetOption(L"TargetMachine",L"1");
    }
    opt = sFind(tool->Options,&Option::Name,L"DebugInformationFormat");
    if(opt && opt->Value==L"4") opt->Value=L"3";
    opt = sFind(tool->Options,&Option::Name,L"MinimalRebuild");
    if(opt && opt->Value==L"true") opt->Value=L"false";    
  }

  // include directories

  if(tool->Name==L"VCCLCompilerTool")
  {
    buffer.Clear();
    Option *aid = tool->GetOption(L"AdditionalIncludeDirectories");
    if(aid && !aid->Value.IsEmpty())
    {
      buffer = aid->Value;
      buffer.Add(';');
    }
    else
    {
      buffer=L"./;";
    }
    sFORALL(Project->Includes,path)
    {
      buffer.Add(path->Path(OmniPath::PT_SOLUTION));
      buffer.Add(L";");
    }
    
    tool->SetOption(L"AdditionalIncludeDirectories",(sChar *)buffer);

    const sChar *g=Project->Guid;
    buffer.PrintF(L"&quot;sCONFIG_GUID={0x%!8s,{0x%!4s,0x%!4s,0x%!4s},{0x%!2s,0x%!2s,0x%!2s,0x%!2s,0x%!2s,0x%!2s}}&quot;",
                  g+1,g+10,g+15,g+20,g+25,g+27,g+29,g+31,g+33,g+35);

    if(conf->Defines.GetCount()>0)
    {
      sPoolString *strp;
      sFORALL(conf->Defines,strp)
      {
        buffer.Add(L";");
        buffer.Add(*strp);
      }
    }
    tool->SetOption(L"PreprocessorDefinitions",(sChar *)buffer);
  }

  if(tool->Name==L"VCLinkerTool")
  {
    if(conf->Links.GetCount()>0)
    {
      buffer=L"";
      sPoolString *strp;
      sFORALL(conf->Links,strp)
      {
        if(_i!=0)
          buffer.Add(L" ");
        buffer.Add(L"&quot;");
        buffer.Add(*strp);
        buffer.Add(L"&quot;");
      }
      tool->SetOption(L"AdditionalDependencies",(sChar *)buffer);
    }
  }

  // add postfix to name of executable
  if(tool->Name==L"VCLinkerTool")
  {
    if (ConfigFile.ExecutablePostfix & sEXEPOSTFIX_CONFIG)
    {
      sString<128> s;
      s.PrintF(L"$(OutDir)\\$(ProjectName)_%s.exe", conf->Name);
      tool->SetOption(L"OutputFile", (sChar*)s);
    }
  }

/*
  OutBuffer.PrintF(L"%t<Tool Name=\"%s\"\r\n",indent,tool->Name);
  if(tool->Name==L"VCCLCompilerTool")
  {
    OutBuffer.PrintF(L"%tAdditionalIncludeDirectories=\"./;",indent+1);
    sFORALL(Project->Includes,path)
      OutBuffer.PrintF(L"%s;",*path);
    OutBuffer.PrintF(L"\"\r\n");
  }
*/

  // write out options

  OutBuffer.PrintF(L"%t<Tool Name=\"%s\"\r\n",indent,tool->Name);
  sFORALL(tool->Options,opt)
  {
    sBool ok = 1;
    if(opt->Condition==OC_LIBRARY)
    {
      ok = (Project->ConfigurationType!=1);
      if(opt->NegateCondition) ok = !ok;
    }

    if(ok)
      OutBuffer.PrintF(L"%t%s=\"%s\"\r\n",indent+1,opt->Name,opt->Value);
  }
  OutBuffer.PrintF(L"%t/>\r\n",indent);

  // delete temporary options

  delete tool;
}

void Document::OutputConfig(Config *conf,const sChar *plat)
{
  Tool *tool;

  OutBuffer.Print(L"\t\t<Configuration\r\n");
  OutBuffer.PrintF(L"\t\t\tName=\"%s|%s\"\r\n",conf->Name,plat);

  if (*OutputPath.Path(OmniPath::PT_SOLUTION))
    OutBuffer.PrintF(L"\t\t\tOutputDirectory=\"%s\\$(ProjectName)_$(ConfigurationName)_%s\"\r\n",OutputPath.Path(OmniPath::PT_SOLUTION),plat);
  else
    OutBuffer.PrintF(L"\t\t\tOutputDirectory=\"$(SolutionDir)$(ConfigurationName)_%s\"\r\n",plat);
  
  if (*IntermediatePath.Path(OmniPath::PT_SOLUTION))
    OutBuffer.PrintF(L"\t\t\tIntermediateDirectory=\"%s\\$(ProjectName)_$(ConfigurationName)_%s\"\r\n",IntermediatePath.Path(OmniPath::PT_SOLUTION),plat);
  else
    OutBuffer.PrintF(L"\t\t\tIntermediateDirectory=\"$(ConfigurationName)_%s\"\r\n",plat);
  OutBuffer.PrintF(L"\t\t\tConfigurationType=\"%d\"\r\n",Project->ConfigurationType);
  OutBuffer.Print(L"\t\t\tCharacterSet=\"1\"\r\n");
  OutBuffer.Print(L"\t\t\t>\r\n");
 
  sFORALL(conf->Tools,tool)
    OutputTool(tool,3,plat,conf);

  OutBuffer.Print(L"\t\t</Configuration>\r\n");
}

void Document::OutputFolder(ProjFolder *parent,sInt indent)
{
  File *file;
  ProjFolder *folder;

  OutBuffer.PrintF(L"%t<Filter Name=\"%s\" Filter=\"\">\r\n",indent,parent->Name);
  
  sFORALL(parent->Folders,folder)
    OutputFolder(folder,indent+1);
  sFORALL(parent->Files,file)
    OutputFile(file,indent+1);
  OutBuffer.PrintF(L"%t</Filter>\r\n",indent);
}

void Document::OutputFile(File *file,sInt indent)
{
  static sInt pbits[2] = 
  { sVSPLAT_WIN32,sVSPLAT_WIN64 };
  static const sChar *pname[2] =
  { L"Win32",L"x64" }; 

  Config *conf;
  Tool *tool;
  OutBuffer.PrintF(L"%t<File RelativePath=\".\\%P\">\r\n",indent,file->Name);

  sFORALL(file->Modifications,conf)
  {
    for(sInt i=0;i<sCOUNTOF(pbits);i++)
    {
      if(VSPlatformMask & conf->VSPlatformMask & pbits[i])
      {
        OutBuffer.PrintF(L"%t<FileConfiguration Name=\"%s|%s\"%s>\r\n",indent+1,conf->Name,pname[i],conf->ExcludeFromBuild?L" ExcludedFromBuild=\"true\"":L"");
        sFORALL(conf->Tools,tool)
          OutputTool(tool,indent+2,pname[i],conf);
        OutBuffer.PrintF(L"%t</FileConfiguration>\r\n",indent+1);
      }
    }
  }
  OutBuffer.PrintF(L"%t</File>\r\n",indent);
}

void Global(sTextBuffer &tb,const sChar *name,const sChar *value)
{
  tb.PrintF(L"\t\t<Global\r\n");
  tb.PrintF(L"\t\t\tName=%q\r\n",name);
  tb.PrintF(L"\t\t\tValue=%q\r\n",value);
  tb.PrintF(L"\t\t/>\r\n");
}

void Document::OutputProject()
{
  Config *conf;
  File *file;
  ProjFolder *folder;
  sPoolString *strp;

  if(sCmpString(L"makeproject",Solution->Name)==0)
    sDPrintF(L"..\n");

  OutBuffer.Clear();


  OutBuffer.Print(L"<?xml version=\"1.0\" encoding=\"Windows-1252\"?>\r\n");
  OutBuffer.Print(L"<VisualStudioProject\r\n");
  OutBuffer.Print(L"\tProjectType=\"Visual C++\"\r\n");
  OutBuffer.PrintF(L"\tVersion=\"%d,00\"\r\n", VS_ProjectVersion);
  OutBuffer.PrintF(L"\tName=\"%s\"\r\n",Project->Name);
  OutBuffer.PrintF(L"\tProjectGUID=\"%s\"\r\n",Project->Guid);
  OutBuffer.PrintF(L"\tRootNamespace=\"%s\"\r\n",Project->Name);
  OutBuffer.Print(L"\tKeyword=\"Win32Proj\"\r\n");
  OutBuffer.Print(L"\t>\r\n");


  sU32 allmask = 0;
  sFORALL(Solution->Configs,conf)
    if(Solution->GetConfigBit(_i))
      allmask |= conf->VSPlatformMask;
  allmask &= VSPlatformMask;

  OutBuffer.Print(L"\t<Platforms>\r\n");
  if(allmask & (sVSPLAT_WIN32))
    OutBuffer.Print(L"\t\t<Platform Name=\"Win32\"/>\r\n");
  if(allmask & sVSPLAT_WIN64)
    OutBuffer.Print(L"\t\t<Platform Name=\"x64\"/>\r\n");
  OutBuffer.Print(L"\t</Platforms>\r\n\r\n");

  OutBuffer.Print(L"\t<ToolFiles>\r\n");  
  OutBuffer.PrintF(L"\t\t<ToolFile RelativePath=\"%s\\altona\\doc\\altona.rules\"/>\r\n",RootPath.Path(OmniPath::PT_SOLUTION));  
  OutBuffer.PrintF(L"\t\t<ToolFile RelativePath=\"%s\\altona\\doc\\nasm.rules\"/>\r\n",RootPath.Path(OmniPath::PT_SOLUTION));
  OutBuffer.PrintF(L"\t\t<ToolFile RelativePath=\"%s\\altona\\doc\\yasm.rules\"/>\r\n",RootPath.Path(OmniPath::PT_SOLUTION));
  if(ConfigFile.SDK&sSDK_CHAOS)
    OutBuffer.PrintF(L"\t\t<ToolFile RelativePath=\"%s\\chaos-code\\chaos.rules\"/>\r\n",RootPath.Path(OmniPath::PT_SOLUTION));
  sFORALL(Solution->ProjectMain->Rules,strp)
    OutBuffer.PrintF(L"\t\t<ToolFile RelativePath=\"%s\\%P\"/>\r\n",RootPath.Path(OmniPath::PT_SOLUTION),(const sChar *)*strp);
  OutBuffer.Print(L"\t</ToolFiles>\r\n\r\n");

  OutBuffer.Print(L"\t<Configurations>\r\n");

  sFORALL(Solution->Configs,conf)
  {
    if(Solution->GetConfigBit(_i))
    {
      if(allmask & conf->VSPlatformMask & sVSPLAT_WIN32)
        OutputConfig(conf,L"Win32");
      if(allmask & conf->VSPlatformMask & sVSPLAT_WIN64)
        OutputConfig(conf,L"x64");
    }
  }
  OutBuffer.Print(L"\t</Configurations>\r\n\r\n");

  OutBuffer.Print(L"\t<References>\r\n");
  OutBuffer.Print(L"\t</References>\r\n\r\n");

  OutBuffer.Print(L"\t<Files>\r\n");
  sFORALL(Project->Files.Folders,folder)
    OutputFolder(folder,2);
  sFORALL(Project->Files.Files,file)
    OutputFile(file,2);

  // finish it

  OutBuffer.Print(L"\t</Files>\r\n\r\n");

  OutBuffer.Print(L"\t<Globals>\r\n");
  OutBuffer.Print(L"\t</Globals>\r\n\r\n");

  OutBuffer.Print(L"</VisualStudioProject>\r\n");

  // write file

  if(WriteFlag)
  {
    if(!BriefFlag)
      sPrintF(L"  write <%s>\n",Project->OutputFile);
    sSaveTextAnsiIfDifferent(Project->OutputFile,OutBuffer.Get());
  }
}

/****************************************************************************/

void Document::OutputFilters(ProjFolder &folder, const sChar *path)
{
  sString<256> name=path;
  if (!name.IsEmpty()) name.Add(L"\\");
  name.Add(folder.Name);

  if (!name.IsEmpty())
  {
    OutBuffer.PrintF(L"\t\t<Filter Include=\"%s\">\r\n",name);

    sGUID guid;
    sChecksumMD5 md5p, md5f;
    md5p.Calc((const sU8*)(const sChar*)Project->Guid,2*sGetStringLen(Project->Guid));
    md5f.Calc((const sU8*)(const sChar*)name,2*sGetStringLen(name));
    sU32 *g32 = (sU32*)&guid;
    g32[0]=md5f.Hash[0]^md5p.Hash[0];
    g32[1]=md5f.Hash[1]^md5p.Hash[1];
    g32[2]=md5f.Hash[2]^md5p.Hash[2];
    g32[3]=md5f.Hash[3]^md5p.Hash[3];
    OutBuffer.PrintF(L"\t\t\t<UniqueIdentifier>{%x}</UniqueIdentifier>\r\n",guid);
    OutBuffer.PrintF(L"\t\t</Filter>\r\n");
  }

  ProjFolder *f;
  sFORALL(folder.Folders,f) OutputFilters(*f,name);
}

void Document::OutputFilterFiles(ProjFolder &folder, const sChar *path, sInt group)
{
  sString<256> name=path;
  if (!name.IsEmpty()) name.Add(L"\\");
  name.Add(folder.Name);

  sString<64> gname=L"None";
  if (group>0) gname=Targets[group-1]->Name;

  File *file;
  sFORALL(folder.Files,file) if (file->Temp==group)
  {
    if (!name.IsEmpty())
    {
      OutBuffer.PrintF(L"\t\t<%s Include=\"%s\">\r\n",gname,file->Name);
      OutBuffer.PrintF(L"\t\t\t<Filter>%s</Filter>\r\n",name);
      OutBuffer.PrintF(L"\t\t</%s>\r\n",gname);
    }
    else
      OutBuffer.PrintF(L"\t\t<%s Include=\"%s\" />\r\n",gname,file->Name);
  }
 
  ProjFolder *f;
  sFORALL(folder.Folders,f) OutputFilterFiles(*f,name,group);
}

void Document::OutputDependencies(ProjectData *project,sArray<ProjectData*> &done)
{
  Depend *dep;
  sFORALL(project->Dependencies,dep)
  {
    sBool found=sFALSE;
    for (sInt i=0; i<done.GetCount(); i++) if (done[i]==dep->Project) found=sTRUE;

    if (!found)
    {
      OutBuffer.PrintF(L"\t\t<ProjectReference Include=\"%s\">\r\n",dep->Project->OutputFile);
      OutBuffer.PrintF(L"\t\t\t<Project>%s</Project>\r\n",dep->Guid);
      OutBuffer.Print(L"\t\t</ProjectReference>\r\n");
      done.AddTail(dep->Project);
    }
    OutputDependencies(dep->Project,done);
  }
}

#define FORALLCONFIG(conf) sFORALL(Solution->Configs,conf) if(Solution->GetConfigBit(_i)) for (sInt p=0; p<sCOUNTOF(platflags); p++) if (allmask & conf->VSPlatformMask & platflags[p])

void Document::OutputXProject() // for VS2010 and higher
{
  Config *conf;
  File *file;

  if(sCmpString(L"makeproject",Solution->Name)==0)
    sDPrintF(L"..\n");

  OutBuffer.Clear();

  OutBuffer.Print(L"<?xml version=\"1.0\" encoding=\"Windows-1252\"?>\r\n");
  OutBuffer.Print(L"<Project DefaultTargets=\"Build\" ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\r\n");

  sU32 allmask = 0;
  sFORALL(Solution->Configs,conf)
    if(Solution->GetConfigBit(_i))
      allmask |= conf->VSPlatformMask;
  allmask &= VSPlatformMask;

  static const sU32 platflags[] = {sVSPLAT_WIN32, sVSPLAT_WIN64 };
  static const sChar* platnames[] = {L"Win32", L"x64", };

  OutBuffer.Print(L"\t<ItemGroup Label=\"ProjectConfigurations\">\r\n");
  FORALLCONFIG(conf)
  {
    OutBuffer.PrintF(L"\t\t<ProjectConfiguration Include=\"%s|%s\">\r\n",conf->Name,platnames[p]);
    OutBuffer.PrintF(L"\t\t\t<Configuration>%s</Configuration>\r\n",conf->Name);
    OutBuffer.PrintF(L"\t\t\t<Platform>%s</Platform>\r\n",platnames[p]);
    OutBuffer.PrintF(L"\t\t</ProjectConfiguration>\r\n");
  }
  OutBuffer.Print(L"\t</ItemGroup>\r\n");
  OutBuffer.Print(L"\r\n");

  OutBuffer.Print(L"\t<PropertyGroup Label=\"Globals\">\r\n");
  OutBuffer.PrintF(L"\t\t<ProjectGuid>%s</ProjectGuid>\r\n",Project->Guid);
  OutBuffer.PrintF(L"\t\t<RootNamespace>%s</RootNamespace>\r\n",Project->Name);
  OutBuffer.Print(L"\t\t<Keyword>Win32Proj</Keyword>\r\n");
  OutBuffer.PrintF(L"\t\t<AltonaRoot>%s</AltonaRoot>\r\n",RootPath.Path(OmniPath::PT_SOLUTION));
  OutBuffer.Print(L"\t</PropertyGroup>\r\n");
  OutBuffer.Print(L"\r\n");

  OutBuffer.Print(L"\t<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\r\n");
  OutBuffer.Print(L"\r\n");

  FORALLCONFIG(conf)
  {
    static const sChar *conftypes[]={L"",L"Application",L"DynamicLibrary",L"",L"StaticLibrary"};
    OutBuffer.PrintF(L"\t<PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\" Label=\"Configuration\">\r\n",conf->Name,platnames[p]);
    OutBuffer.PrintF(L"\t\t<ConfigurationType>%s</ConfigurationType>\r\n",conftypes[Project->ConfigurationType]);
    OutBuffer.Print(L"\t\t<CharacterSet>Unicode</CharacterSet>\r\n");
   
    if (*OutputPath.Path(OmniPath::PT_SOLUTION))
      OutBuffer.PrintF(L"\t\t<OutDir>%s\\$(ProjectName)_$(Configuration)_%s\\</OutDir>\r\n",OutputPath.Path(OmniPath::PT_SOLUTION),platnames[p]);
    else
      OutBuffer.PrintF(L"\t\t<OutDir>$(SolutionDir)$(Configuration)_%s\\</OutDir>\r\n",platnames[p]);
  
    if (*IntermediatePath.Path(OmniPath::PT_SOLUTION))
      OutBuffer.PrintF(L"\t\t<IntDir>%s\\$(ProjectName)_$(Configuration)_%s\\</IntDir>\r\n",IntermediatePath.Path(OmniPath::PT_SOLUTION),platnames[p]);
    else
      OutBuffer.PrintF(L"\t\t<IntDir>$(Configuration)_%s\\</IntDir>\r\n",platnames[p]);

	  if (VS_Version==11) OutBuffer.PrintF(L"\t\t<PlatformToolset>v110</PlatformToolset>\r\n");
    OutBuffer.Print(L"\t</PropertyGroup>\r\n");
  }
  OutBuffer.Print(L"\r\n");

  OutBuffer.Print(L"\t<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\r\n");
  OutBuffer.Print(L"\t<ImportGroup Label=\"ExtensionSettings\">\r\n");
  OutBuffer.Print(L"\t\t<Import Project=\"$(AltonaRoot)\\altona\\doc\\altona.props\"/>\r\n");
  OutBuffer.Print(L"\t\t<Import Project=\"$(AltonaRoot)\\altona\\doc\\yasm.props\"/>\r\n");
  OutBuffer.Print(L"\t</ImportGroup>\r\n");
  
  FORALLCONFIG(conf)
  {
    OutBuffer.PrintF(L"\t<ImportGroup Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\" Label=\"PropertySheets\">\r\n",conf->Name,platnames[p]);
    OutBuffer.Print(L"\t\t<Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />\r\n");
    sPoolString *prop;
    sFORALL(conf->PropertySheets,prop)
      OutBuffer.PrintF(L"\t\t<Import Project=\"$(AltonaRoot)\\%P\" />\r\n",*prop);
    OutBuffer.Print(L"\t</ImportGroup>\r\n");
  }
  
  OutBuffer.Print(L"\t<PropertyGroup Label=\"UserMacros\" />\r\n");
  OutBuffer.Print(L"\r\n");
  
  FORALLCONFIG(conf)
  {
    OutBuffer.PrintF(L"\t<ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\">\r\n",conf->Name,platnames[p]);

    OutBuffer.Print(L"\t\t<ClCompile>\r\n");
    OutBuffer.Print(L"\t\t\t<AdditionalIncludeDirectories>");
    OmniPath *op;
    sFORALL(Project->Includes,op)
      OutBuffer.PrintF(L"%s;",op->Path(OmniPath::PT_SOLUTION));
    OutBuffer.Print(L"%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>\r\n");
    OutBuffer.Print(L"\t\t\t<PreprocessorDefinitions>");
    const sChar *g=Project->Guid;
    OutBuffer.PrintF(L"sCONFIG_GUID={0x%!8s,{0x%!4s,0x%!4s,0x%!4s},{0x%!2s,0x%!2s,0x%!2s,0x%!2s,0x%!2s,0x%!2s}};",
                  g+1,g+10,g+15,g+20,g+25,g+27,g+29,g+31,g+33,g+35);
    sPoolString *def;
    sFORALL(conf->Defines,def)
      OutBuffer.PrintF(L"%s;",*def);
    OutBuffer.Print(L"%(PreprocessorDefinitions)</PreprocessorDefinitions>\r\n");
    OutBuffer.Print(L"\t\t</ClCompile>\r\n");

    OutBuffer.Print(L"\t\t<Link>\r\n");
    OutBuffer.Print(L"\t\t\t<AdditionalDependencies>");
    sFORALL(conf->Links,def)
      OutBuffer.PrintF(L"%s;",*def);
    OutBuffer.Print(L"%(AdditionalDependencies)</AdditionalDependencies>\r\n");
    OutBuffer.Print(L"\t\t</Link>\r\n");

    OutBuffer.PrintF(L"\t</ItemDefinitionGroup>\r\n");
  }
  OutBuffer.Print(L"\r\n");


  sArray<File *> allfiles;
  sStaticArray<sInt> groups;
  groups.Resize(Targets.GetCount()+1);
  for (sInt i=0; i<=Targets.GetCount(); i++) groups[i]=0;

  Project->Files.GetAllFiles(allfiles);
  sFORALL(allfiles,file)
  {
    file->Temp=0;
    const sChar *fext=sFindFileExtension(file->Name);
    TargetInfo *tgt;
    sFORALL(Targets,tgt)
    {
      sPoolString ext;
      for (sInt i=0; i<tgt->Extensions.GetCount(); i++)
        if (!sCmpStringI(fext,tgt->Extensions[i]))
          file->Temp = _i+1;
      if (file->Temp>0) break;
    }
    groups[file->Temp]++;
  }

  for (sInt g=Targets.GetCount(); g>=0; g--) if (groups[g])
  {
    sPoolString name=L"None";
    if (g>0) name=Targets[g-1]->Name;

    OutBuffer.Print(L"\t<ItemGroup>\r\n");
    sFORALL(allfiles,file) if (file->Temp==g)
    {
      if (!file->Modifications.IsEmpty())
      {
        OutBuffer.PrintF(L"\t\t<%s Include=\"%s\">\r\n",name,file->Name);
        sFORALL(file->Modifications,conf)
          if(conf->ExcludeFromBuild && Solution->GetConfigBit(_i)) 
            for (sInt p=0; p<sCOUNTOF(platflags); p++) 
              if (allmask & conf->VSPlatformMask & platflags[p])
                OutBuffer.PrintF(L"\t\t\t<ExcludedFromBuild Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\">true</ExcludedFromBuild>\r\n",conf->Name,platnames[p]);
        OutBuffer.PrintF(L"\t\t</%s>\r\n",name);
      }
      else
        OutBuffer.PrintF(L"\t\t<%s Include=\"%s\" />\r\n",name,file->Name);
    }
    OutBuffer.Print(L"\t</ItemGroup>\r\n");
  }
  OutBuffer.Print(L"\r\n");

  if (!Project->Dependencies.IsEmpty())
  {
    OutBuffer.Print(L"\t<ItemGroup>\r\n");
    sArray<ProjectData *> done;
    OutputDependencies(Project,done);
    OutBuffer.Print(L"\t</ItemGroup>\r\n\r\n");
  }

  OutBuffer.Print(L"\t<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />\r\n");
  OutBuffer.Print(L"\t<ImportGroup Label=\"ExtensionTargets\">\r\n");
  OutBuffer.PrintF(L"\t\t<Import Project=\"%s\\altona\\doc\\altona.targets\"/>\r\n",RootPath.Path(OmniPath::PT_SOLUTION));  
  OutBuffer.PrintF(L"\t\t<Import Project=\"%s\\altona\\doc\\yasm.targets\"/>\r\n",RootPath.Path(OmniPath::PT_SOLUTION));
  OutBuffer.Print(L"\t</ImportGroup>\r\n");

  OutBuffer.Print(L"</Project>\r\n");

  // write file

  if(WriteFlag)
  {
    if(!BriefFlag)
      sPrintF(L"  write <%s>\n",Project->OutputFile);
    sSaveTextAnsiIfDifferent(Project->OutputFile,OutBuffer.Get());
  }

  // write filters file
  OutBuffer.Clear();
  OutBuffer.Print(L"<?xml version=\"1.0\" encoding=\"Windows-1252\"?>\r\n");
  OutBuffer.Print(L"<Project DefaultTargets=\"Build\" ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\r\n");

  for (sInt g=Targets.GetCount(); g>=0; g--) if (groups[g])
  {
    OutBuffer.Print(L"\t<ItemGroup>\r\n");
    OutputFilterFiles(Project->Files, L"", g);
    OutBuffer.Print(L"\t</ItemGroup>\r\n");
  }

  OutBuffer.Print(L"\t<ItemGroup>\r\n");
  OutputFilters(Project->Files, L"");
  OutBuffer.Print(L"\t</ItemGroup>\r\n");

  OutBuffer.Print(L"</Project>\r\n");

  if(WriteFlag)
  {
    sString<sMAXPATH> fn=Project->OutputFile;
    fn.Add(L".filters");
    if(!BriefFlag)
      sPrintF(L"  write <%s>\n",fn);
    sSaveTextAnsiIfDifferent(fn,OutBuffer.Get());
  }

}

#undef FORALLCONFIG

/****************************************************************************/

void Document::OutputMatrix(const sChar *guid,const sChar *name,ConfigExpr *p)
{
  Config *conf;
  sFORALL(Solution->Configs,conf)
  {
    sBool build = p->Eval(conf->Name);
    if(Solution->GetConfigBit(_i))
    {
      if(conf->VSPlatformMask & VSPlatformMask & sVSPLAT_WIN32)
      {
        OutBuffer.PrintF(L"\t\t%s.%s|Win32.ActiveCfg = %s|Win32\r\n",guid,conf->Name,conf->Name);
        if(build)
          OutBuffer.PrintF(L"\t\t%s.%s|Win32.Build.0 = %s|Win32\r\n",guid,conf->Name,conf->Name);
      }
      if(conf->VSPlatformMask & VSPlatformMask & sVSPLAT_WIN64)
      {
        OutBuffer.PrintF(L"\t\t%s.%s|x64.ActiveCfg = %s|x64\r\n",guid,conf->Name,conf->Name);
        if(build)
          OutBuffer.PrintF(L"\t\t%s.%s|x64.Build.0 = %s|x64\r\n",guid,conf->Name,conf->Name);
      }
    }
  }
}

// this routine is a candidate for types.hpp!

void MakeRelPath(const sStringDesc &dest,const sChar *path,const sChar *cdir)
{
  sInt plen,clen;
  sChar *d = dest.Buffer;
  
  // we want to use '/', not '\\'

  plen = sFindFirstChar(path,'\\');
  clen = sFindFirstChar(cdir,'\\');
  sVERIFY(plen==-1 && clen==-1);

  // different drive letters -> no way to get to a common path

  plen = sFindLastChar(path,':');
  clen = sFindLastChar(cdir,':');
  if(plen!=clen || sCmpStringLen(path,cdir,plen)!=0)
  {
    sCopyString(dest,path);
    return;
  }

  // skip drive letters

  path += plen+1;
  cdir += plen+1;

  // skip common directories

  for(;;)
  {
    plen = sFindFirstChar(path,'/');
    clen = sFindFirstChar(cdir,'/');
    if(plen!=clen || sCmpStringLen(path,cdir,plen)!=0)
      break;
    path += plen+1;
    cdir += plen+1;
  }

  // trace back for every 'wrong' directory in cdir.

  for(;;)
  {
    clen = sFindFirstChar(cdir,'/');
    if(clen==-1) 
      break;
    *d++ = '.'; *d++ = '.'; *d++ = '/';
    cdir += clen+1;
  }
  if(*cdir!=0)
  {
    *d++ = '.'; *d++ = '.'; *d++ = '/';
  }

  // append path

  while((*d++ = *path++)!=0);

  sVERIFY(d-dest.Buffer <= dest.Size);
}

void Document::OutputProjectWithDependencies(ProjectData *eproject, sArray<ProjectData *> &alreadyInSolution)
{
  Depend *dep;
  DependExtern *depex;
  sString<sMAXPATH> path;
  
  // do nothing if it is already added
  sInt index = sFindIndex(alreadyInSolution, eproject);
  if (index!=-1)
   return;


  // mark as inserted
  alreadyInSolution.AddTail(eproject);

  // write project
  sString<sMAXPATH> solpath,projpath;
  sExtractPath(Solution->SolutionOutputFile,solpath);
  projpath = eproject->Path.Path(OmniPath::PT_SOLUTION);
  projpath.AddPath(eproject->Name);
  projpath.Add(VS_ProjExtension);
  MakeRelPath(path,projpath,solpath);
  OutBuffer.PrintF(L"Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"%s\", \"%P\", \"%s\"\r\n",
    eproject->Name,path,eproject->Guid);
  
  // write dependencies
  if(eproject->Dependencies.GetCount()+eproject->DependExterns.GetCount()>0)
  {
    OutBuffer.Print(L"\tProjectSection(ProjectDependencies) = postProject\r\n");
    sFORALL(eproject->Dependencies,dep)
      OutBuffer.PrintF(L"\t\t%s = %s\r\n",dep->Guid,dep->Guid);
    sFORALL(eproject->DependExterns,depex)
      OutBuffer.PrintF(L"\t\t%s = %s\r\n",depex->Guid,depex->Guid);
    OutBuffer.PrintF(L"\tEndProjectSection\r\n");
  }
  OutBuffer.Print(L"EndProject\r\n");

  // add projects of dependencies
  sFORALL(eproject->Dependencies,dep)
  {
    OutputProjectWithDependencies(dep->Project,alreadyInSolution);
  }
  sFORALL(eproject->DependExterns,depex)
  {
    sExtractPath(Solution->SolutionOutputFile,solpath);
    projpath = depex->Path.Path(OmniPath::PT_SOLUTION);
    projpath.AddPath(depex->Name);
    projpath.Add(VS_ProjExtension);
    MakeRelPath(path,projpath,solpath);
    OutBuffer.PrintF(L"Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"%s\", \"%P\", \"%s\"\r\n",
      depex->Name,path,depex->Guid);
    OutBuffer.PrintF(L"EndProject\n");
  }
}

void Document::OutputSolution()
{
  Config *conf;

  OutBuffer.Clear();

  // determine what name to print in the solution file (the comment seems to get parsed by the VC version selector)
  const sChar *yearString = L"2005";
  if(VS_SolutionVersion == 10) // 2008
    yearString = L"2008";
  if(VS_SolutionVersion == 11) // 2010
	  yearString = L"2010";

  OutBuffer.PrintF(L"Microsoft Visual Studio Solution File, Format Version %d.00\r\n", VS_SolutionVersion);
  OutBuffer.PrintF(L"# Visual C++ Express %s\r\n",yearString);

  sArray<ProjectData *> alreadyInSolution;

  sFORALL(Solution->Projects, Project)
  {
    OutputProjectWithDependencies(Project, alreadyInSolution);
  }

  OutBuffer.Print(L"Global\r\n");


  OutBuffer.Print(L"\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\r\n");
  sFORALL(Solution->Configs,conf)
  {
    if(Solution->GetConfigBit(_i))
    {
      if(conf->VSPlatformMask & VSPlatformMask & sVSPLAT_WIN32)
        OutBuffer.PrintF(L"\t\t%s|Win32 = %s|Win32\r\n",conf->Name,conf->Name);
      if(conf->VSPlatformMask & VSPlatformMask & sVSPLAT_WIN64)
        OutBuffer.PrintF(L"\t\t%s|x64 = %s|x64\r\n",conf->Name,conf->Name);
    }
  }
  OutBuffer.Print(L"\tEndGlobalSection\r\n");

  OutBuffer.Print(L"\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\r\n");
  OutputMatrix(Project->Guid,Project->Name,0);
  ProjectData *proda;
  sFORALL(alreadyInSolution,proda)
  {
    if(proda->Guid != Project->Guid)
    {
      DependExtern *dex;
      OutputMatrix(proda->Guid,proda->Name,0);
      sFORALL(proda->DependExterns,dex)
      {
        OutputMatrix(dex->Guid,dex->Name,dex->IfConfig);  
      }
    }
  }

  OutBuffer.Print(L"\tEndGlobalSection\r\n");


  OutBuffer.Print(L"\tGlobalSection(SolutionProperties) = preSolution\r\n");
  OutBuffer.Print(L"\t\tHideSolutionNode = FALSE\r\n");
  OutBuffer.Print(L"\tEndGlobalSection\r\n");
  OutBuffer.Print(L"EndGlobal\r\n");

  if(WriteFlag)
  {
    if(!BriefFlag)
      sPrintF(L"  write <%s>\n",Solution->SolutionOutputFile);
    sSaveTextAnsiIfDifferent(Solution->SolutionOutputFile,OutBuffer.Get());
  }
}

/****************************************************************************/

void Document::DoCreateNewFiles(ProjFolder *parent)
{
  File *file;
  ProjFolder *folder;
  sString<sMAXPATH> buffer;
  sString<4096> print;
  sString<128> noext;

  static const sChar lic_none[] =
  L"/****************************************************************************/\n"
  L"\n";

  static const sChar text_any[] =
  L"%s";

  static const sChar text_hpp[] =
  L"%s"
  L"#ifndef FILE_%s_%s\n"
  L"#define FILE_%s_%s\n"
  L"\n"
  L"#include \"base/types.hpp\"\n"
  L"\n"
  L"/****************************************************************************/\n"
  L"\n"
  L"\n"
  L"\n"
  L"/****************************************************************************/\n"
  L"\n"
  L"#endif // FILE_%s_%s\n"
  L"\n";

  static const sChar text_cpp[] =
  L"%s"
  L"#define sPEDANTIC_WARN 1\n"
  L"#include \"%shpp\"\n"
  L"\n"
  L"/****************************************************************************/\n"
  L"\n"
  L"\n"
  L"\n"
  L"/****************************************************************************/\n"
  L"\n";

  static const sChar text_asm[] =
  L"%s"
  L"			section .text\n"
  L"			bits    32\n"
  L"\n"
  L";			global	_label\n"
  L"\n"
  L";****************************************************************************/\n"
  L"\n"
  L";_label:	incbin	\"file.txt\"\n"
  L";			dw		0\n"
  L"\n"
  L";****************************************************************************/\n"
  L"\n"
  L"\n"
  L"\n"
  L";****************************************************************************/\n"
  L"\n"
  L"			.end\n"
  L"\n";

  static const sChar text_rc[] =
  L"%s"
  L"100 ICON \"icon.ico\"\n"
  L"\n"
  L";****************************************************************************/\n"
  L"\n";

  // this ommits folders!

  sString<256> up;    // upper case project name
  sString<256> uf;    // upper case file name

  up = Project->Name;
  sMakeUpper(up);

  sFORALL(parent->Files,file)
  {
    if (file->CreateFile)
    {
      buffer = Project->Path.Path(OmniPath::PT_SYSTEM);
      buffer.Add(L"/");
      buffer.Add(file->Name);
      if(!sCheckFile(buffer))
      {
        if(CreateNewFiles)
        {
          const sChar *lic = file->License ? file->License->Header : lic_none;

          if(sCmpStringI(sFindFileExtension(buffer),L"hpp")==0)
          {
            uf = file->Name;
            sInt pos = sFindFirstChar(uf,'.');
            if(pos>=0) uf[pos] = 0;
            sReplaceChar(uf,' ','_');
            sReplaceChar(uf,'/','_');
            uf.Add(L"_HPP");
            sMakeUpper(uf);

            sSPrintF(print,text_hpp,lic,up,uf,up,uf,up,uf);
          }
          else if(sCmpStringI(sFindFileExtension(buffer),L"cpp")==0)
          {            
            noext = sFindFileWithoutPath(file->Name);
            *sFindFileExtension(noext) = 0;
            sSPrintF(print,text_cpp,lic,noext);
          }
          else if(sCmpStringI(sFindFileExtension(buffer),L"rc")==0)
          {
            noext = sFindFileWithoutPath(file->Name);
            *sFindFileExtension(noext) = 0;
            sSPrintF(print,text_rc,lic,noext);
          }
          else if(sCmpStringI(sFindFileExtension(buffer),L"asm")==0)
          {
            sTextBuffer tb;
            tb.Print(lic);
            sChar *s = tb.Get();
            while(*s=='/')
            {
              *s=';';
              while(*s && *s!='\n')
                *s++;
              if(*s=='\n')
                s++;
            }
            sSPrintF(print,text_asm,tb.Get());
          }
          else
          {
            sSPrintF(print,text_any,lic);
          }

          sPrintF(L"create %q\n",buffer);
          if(WriteFlag)
          {
            sString<sMAXPATH> path;
            sExtractPath(buffer,path);
            sMakeDirAll(path);
            sSaveTextAnsi(buffer,print);
          }
        }
        else
        {
          sPrintF(L"missing file %q\n",buffer);
        }
      }
    }
  }
  sFORALL(parent->Folders,folder)
  {
    DoCreateNewFiles(folder);
  }
}

/****************************************************************************/

static sBool specialcompare(const sChar *a,const sChar *b,sInt blen)
{
  const sChar *bend = b+blen;
  for(;;)
  {
    if(sIsSpace(*a) && sIsSpace(*b))
    {
      while(sIsSpace(*a)) a++;
      while(sIsSpace(*b)) b++;
    }
    if(b>=bend) return 1;
    if(*a!=*b) return 0;
    a++;
    b++;
  }
}

void Document::DoUpdateLicense(ProjFolder *parent)
{
  File *file;
  ProjFolder *folder;
  sBool fix;
  sString<sMAXPATH> buffer;
  sFORALL(parent->Files,file)
  {
    buffer = Project->Path.Path(OmniPath::PT_SYSTEM);
    buffer.Add(L"/");
    buffer.Add(file->Name);

    fix = 0;
    const sChar *text = sLoadText(buffer);
    const sChar *mem = text;
    if(text)
    {
      // skip whitespace before possible license

      while(sIsSpace(*text)) 
        text++;

      // check for license

      if(sCmpMem(text,L"/*+*",8)==0)      
      {
        if(file->License)
        {
          if(!specialcompare(text,file->License->Header,sGetStringLen(file->License->Header)))
          {
            sPrintF(L"%s: wrong license\n",buffer);
            fix = 1;
          }
        }
        else
        {
          sPrintF(L"%s: found makeproject generated license where none should have been!\n",buffer);
          fix = 1;
        }

      }
      else                                // no license
      {
        if(file->License)
        {
//          sPrintF(L"%s: license missing!\n",buffer);
          fix = 1;
        }
      }

      if(fix && WriteFlag)
      {
        if(sCmpMem(text,L"/*+*",8)==0)    // skip old license
        {
          while(*text!=0 && sCmpMem(text,L"*+*/",8)!=0)
            text++;
          if(*text==0)
            sPrintF(L"%s: malformed license string\n",buffer);
          else
            text += 4;     // skip "*+*/\n"
          while(sIsSpace(*text)) text++;
        }
        sTextBuffer tb;
        if(file->License)
          tb.Print(file->License->Header);
        tb.Print(text);
        sSaveTextAnsi(buffer,tb.Get());
      }
      delete[] mem;
    }


    sFORALL(parent->Folders,folder)
    {
      DoUpdateLicense(folder);
    }
  }
}

/****************************************************************************/

