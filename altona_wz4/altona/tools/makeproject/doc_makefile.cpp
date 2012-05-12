/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#define sPEDANTIC_WARN 1
#include "doc.hpp"

/****************************************************************************/

static const sChar *MakePlatforms[] =
{
  L"???",
  L"MinGW",
  L"Linux",
};

/****************************************************************************/
/****************************************************************************/

static sBool ConfigMatchesPlatform(Config *config,sInt platform)
{
  switch(platform)
  {
  case sMAKE_LINUX: return (config->VSPlatformMask & sVSPLAT_LINUX) != 0; break;
  default:          return sFALSE;
  }
}

Config *FindConfig(sArray<Config *>*list,sInt platform)
{
  Config *config;
  
  sFORALL(*list,config)
  {
    if(ConfigMatchesPlatform(config,platform))
      return config;
  }
  return 0;
}

static void CheckConfig(Config *config,sInt platform)
{
  if(!config)
    sPrintF(L"configuration for platform <%s> not found\n",MakePlatforms[platform]);
  else if(config->MakeTarget.GetCount()==0 || config->MakeDefine.GetCount()==0 || config->TargetExeTemplate.IsEmpty() ||
    config->LinkExe.GetCount()==0 || config->ObjectExtension.IsEmpty())
    sPrintF(L"configuration for platform <%s> is incomplete\n",MakePlatforms[platform]);
}

Config *FindConfigCheck(sArray<Config *>*list,sInt platform)
{
  Config *config = FindConfig(list,platform);
  CheckConfig(config,platform);
  return config;
}

/****************************************************************************/

void Document::OutputPrepareMakefile()
{
  ProjectData *proj;
  Config *config;

  config = FindConfigCheck(&BaseConfigs,ConfMakefile);
  if(config)
  {
    sFORALL(Projects,proj)
    {
      if(proj->ConfigurationType == 1)
        sSPrintF(proj->MakeTarget,config->TargetExeTemplate,proj->Name);
      else
        sSPrintF(proj->MakeTarget,config->TargetLibTemplate,proj->Name);
    }
  }
}

/****************************************************************************/

void Document::OutputSolutionMakefile()
{
  ProjectData *proj;
  Depend *depend;
  sPoolString *nameptr;
  sArray<Depend *> deps;
  Config *config;

  config = FindConfigCheck(&BaseConfigs,ConfMakefile);
  if(!config) return;
  
  sString<256> mext;
  if (!config->MakefileExtension.IsEmpty())
  {
    mext=L".";
    mext.Add(config->MakefileExtension);
  }

  OutBuffer.Clear();
  OutBuffer.PrintF(L"#\n# Automatic Generated Makefile for %s\n#\n\n",MakePlatforms[ConfigFile.Makefile]);

  OutBuffer.Print (L"all:\t");
  sFORALL(Solution->Projects, proj)
    OutBuffer.PrintF(L" %s", proj->Name);
  OutBuffer.Print (L"\n");
  OutBuffer.Print (L"\n");


  sFORALL(Solution->Projects, proj)
  {
    OutBuffer.PrintF(L"%s:", proj->Name);
    sFORALL(proj->Dependencies,depend)
    {
      OutBuffer.PrintF(L" %s",depend->Project->Name);
	    if(!sFind(deps,&Depend::Project,depend->Project) && !sFindPtr(Solution->Projects,depend->Project))
        deps.AddTail(depend);
    }
    OutBuffer.PrintF(L"\n\t@echo --- %s\n",proj->Name);
    OutBuffer.PrintF(L"\t+@make -s --no-print-directory -C %s -f Makefile.%s%s\n",proj->Path.Path(OmniPath::PT_MAKEFILE),proj->Name,mext);
    OutBuffer.PrintF(L"\t@echo Done.\n\n");
  }
	
	OutBuffer.PrintF(L"\n# deps\n\n");
	                 
  sFORALL(deps,depend)
  {
    proj = depend->Project;
    OutBuffer.PrintF(L"%s:",proj->Name);
    sFORALL(proj->Dependencies,depend)
    {
      OutBuffer.PrintF(L" %s",depend->Project->Name);
      if(!sFind(deps,&Depend::Project,depend->Project) && !sFindPtr(Solution->Projects,depend->Project))
        deps.AddTail(depend);
    }
    OutBuffer.PrintF(L"\n\t@echo --- %s\n",proj->Name);
    OutBuffer.PrintF(L"\t+@make -s --no-print-directory -C %s -f Makefile.%s%s\n\n",proj->Path.Path(OmniPath::PT_MAKEFILE),proj->Name,mext);
  }

	OutBuffer.PrintF(L"\n# faketargetall\n\n");
	
	sFORALL(config->FakeTargetAll,nameptr)
  {
    OutBuffer.PrintF(L"%s:\n",*nameptr);

    sFORALL(Solution->Projects, proj)
      OutBuffer.PrintF(L"\t@make -s --no-print-directory -C %s -f Makefile.%s%s %s\n",proj->Path.Path(OmniPath::PT_MAKEFILE),proj->Name,mext,*nameptr );
    sFORALL(deps,depend)
    {
      proj = depend->Project;
      OutBuffer.PrintF(L"\t@make -s --no-print-directory -C %s -f Makefile.%s%s %s\n",proj->Path.Path(OmniPath::PT_MAKEFILE),proj->Name,mext,*nameptr );
    }
    OutBuffer.PrintF(L"\t@echo Done.\n");
    OutBuffer.PrintF(L"\n");
  }

	OutBuffer.PrintF(L"\n# faketargetonce\n\n");
	
	sFORALL(config->FakeTargetOnce,nameptr)
  {
    OutBuffer.PrintF(L"%s:",*nameptr);
    sFORALL(deps,depend)
      OutBuffer.PrintF(L" %s",depend->Project->Name);
    OutBuffer.PrintF(L"\n");

    sFORALL(Solution->Projects, proj)
      OutBuffer.PrintF(L"\t@make -s --no-print-directory -C %s -f Makefile.%s%s %s\n",proj->Path.Path(OmniPath::PT_MAKEFILE),proj->Name,mext,*nameptr );
    OutBuffer.PrintF(L"\t@echo Done.\n");
    OutBuffer.PrintF(L"\n");
  }

  if(WriteFlag)
  {
    sString<4096> makefilename=Solution->GNUMakeOutputfile;
    if (!config->MakefileExtension.IsEmpty())
    {
      makefilename.Add(L".");
      makefilename.Add(config->MakefileExtension);
    }
    if(!BriefFlag)
      sPrintF(L"  write <%s>\n",makefilename);
    sSaveTextAnsiIfDifferent(makefilename,OutBuffer.Get());
  }
}

/****************************************************************************/

static void PrintList(sTextBuffer &out,const sChar *macro,sInt &n)
{
  if(n%8==0)
  {
    if(n==0)
      out.PrintF(L"\n%-11s =",macro);
    else
      out.PrintF(L"\n%-11s+=",macro);
  }
  n++;
}


void Document::AddMakefileFolder(ProjFolder *parent,sInt &n,Config *projcfg)
{
  File *file;
  ProjFolder *folder;
  Config *config;
  sFORALL(parent->Files,file)
  {
    const sChar *ext=sFindFileExtension(file->Name);
    if (sFind(projcfg->SourceExtensions,sPoolString(ext)))
    {
      config = sFind(file->Modifications,&Config::Name,projcfg->Name);
      if(!(config && config->ExcludeFromBuild))
      {
        PrintList(OutBuffer,L"OBJS",n);
        sString<256> buffer;
        sCopyString(buffer,file->Name,ext-file->Name);
        
        for(sInt i=0;buffer[i];i++)
        {
          if(buffer[i]=='\\')
            buffer[i] = '/';
        }
        
        OutBuffer.PrintF(L" $(CONFIG)/%s.%s",buffer,projcfg->ObjectExtension);
      }
    }
  }
  sFORALL(parent->Folders,folder)
    AddMakefileFolder(folder,n,projcfg);
}

sBool Document::AddMakefileCustomSteps(ProjFolder *parent,sInt &n,Config *projcfg)
{
  File *file;
  ProjFolder *folder;
  Config *config;
  sBool result=sFALSE;

  sFORALL(parent->Files,file)
  {
    const sChar *ext=sFindFileExtension(file->Name);
    if (sFind(projcfg->CustomExtensions,sPoolString(ext)))
    {
      config = sFind(file->Modifications,&Config::Name,projcfg->Name);
      if(!(config && config->ExcludeFromBuild))
      {
        PrintList(OutBuffer,L"CUSTOMDEPS",n);
        sString<256> buffer;
        sCopyString(buffer,file->Name,ext-file->Name);
        for(sInt i=0;buffer[i];i++)
          if(buffer[i]=='\\')
            buffer[i] = '/';
        OutBuffer.PrintF(L" %s.hpp",buffer);
        result = sTRUE;
      }
    }
  }
  sFORALL(parent->Folders,folder)
    result|=AddMakefileCustomSteps(folder,n,projcfg);

  return result;
}

static void BuildCachedLinksR(ProjFolder *parent,Config *projcfg,CachedLink *cache)
{
  File *file;
  ProjFolder *folder;
  sFORALL(parent->Files,file)
  {
    const sChar *ext=sFindFileExtension(file->Name);
    if(sFind(projcfg->SourceExtensions,sPoolString(ext)))
    {
      Config *config = sFind(file->Modifications,&Config::Name,projcfg->Name);
      if(config && !config->ExcludeFromBuild)
      {
        // add libraries required by any of the source files to the links
        sPoolString *link;
        sFORALL(config->Links,link)
        {
          if(!sFind(cache->Links,*link))
            cache->Links.AddTail(*link);
        }
      }
    }
  }
  sFORALL(parent->Folders,folder)
    BuildCachedLinksR(folder,projcfg,cache);
}

CachedLink *Document::GetMakefileCachedLinks(ProjectData *proj,Config *projcfg)
{
  CachedLink *cache;

  sFORALL(proj->CachedLinks,cache)
  {
    if(cache->ProjectConfig == projcfg)
      return cache;
  }

  // First time around, need to create the cache first
  cache = new CachedLink;
  cache->ProjectConfig = projcfg;
  cache->Links = projcfg->Links;
  BuildCachedLinksR(&proj->Files,projcfg,cache);
  return cache;
}

static void AddLinks(sArray<sPoolString> &current,sArray<sPoolString> &more)
{
  sPoolString *link;
  sFORALL(more,link)
  {
    if(!sFind(current,*link))
      current.AddTail(*link);
  }
}

static void AddDepsR(ProjectData *proj,SolutionData *solution,sArray<ProjectData*> &deps)
{
  if(proj->CycleDetect)
  {
    sPrintF(L"Cyclic dependency between libraries detected! This won't (easily) work on Linux.\n");
    return;
  }
  
  proj->CycleDetect = 1;
  
  // add all my dependencies first
  Depend *dep;
  sFORALL(proj->Dependencies,dep)
    AddDepsR(dep->Project,solution,deps);

  // add this project if it's not already in and if it's not part of the solution anyway
  if(!sFindPtr(deps,proj) && !sFindPtr(solution->Projects,proj))
    deps.AddTail(proj);
  
  proj->CycleDetect = 0;
}

void Document::OutputProjectMakefile()
{
  sPoolString *name;
  Config *defconfig;

  defconfig = FindConfig(&Solution->Configs,ConfMakefile);
  if(!defconfig) return;

  OutBuffer.Clear();
  OutBuffer.PrintF(L"#\n# Automatic Generated Makefile for %s\n#\n",MakePlatforms[ConfMakefile]);

  // configuration selector
  OutBuffer.PrintF(L"ifndef CONFIG\nCONFIG      = %s\nendif\n",defconfig->Name);
  
  // --- configuration-dependent section
  
  // object files
  Config *config;
  sBool first = sTRUE;
  sInt numendifs=0;
  sFORALL(Solution->Configs,config)
  {
    if(!ConfigMatchesPlatform(config,ConfMakefile))
      continue;
    
    CheckConfig(config,ConfMakefile);
    OutBuffer.PrintF(L"\n%sifeq ($(CONFIG),%s)\n\n",first ? L"" : L"else\n",config->Name);
    numendifs++;
    first = sFALSE;
    
    // deep copy is necessary here (we might modify it later)
    sArray<sPoolString> links = GetMakefileCachedLinks(Project,config)->Links;

    sInt n=0;
    AddMakefileFolder(&Project->Files,n,config);
    sInt nObjFiles=n;
    if(n==0)
      PrintList(OutBuffer,L"OBJS",n);
      
    // asc generated files (if necessary)
    const sChar *customString = L"";
    n = 0;
    if (AddMakefileCustomSteps(&Project->Files,n,config))
    {
      if(n==0)
        PrintList(OutBuffer,L"CUSTOMDEPS",n);       
      customString = L"$(CUSTOMDEPS) ";
    }
    
    // depending libraries: topological sort so we don't need to do it manually anymore.
    sArray<ProjectData *> deps;
    AddDepsR(Project,Solution,deps);
    
    // output deps
    n = 0;
    ProjectData *depend;
    sFORALLREVERSE(deps,depend)
    {
      PrintList(OutBuffer,L"DEPS",n);
      OutBuffer.PrintF(L" %s/$(CONFIG)/%s",depend->Path.Path(OmniPath::PT_MAKEFILE),depend->MakeTarget);

      // if any of our deps needs a library, we need it to
      AddLinks(links,GetMakefileCachedLinks(depend,config)->Links);
    }
    if(n==0)
      PrintList(OutBuffer,L"DEPS",n);

    // defines
    n=0;
    PrintList(OutBuffer,L"DEFINES",n);
    OutBuffer.PrintF(L" -DsCONFIG_GUID=");
    const sChar *g=Project->Guid;
    if(ConfigFile.Makefile != sMAKE_LINUX)
    {
      OutBuffer.PrintF(L"{0x%!8s\\\\\\,{0x%!4s\\\\\\,0x%!4s\\\\\\,0x%!4s}\\\\\\,{0x%!2s\\\\\\,0x%!2s\\\\\\,0x%!2s\\\\\\,0x%!2s\\\\\\,0x%!2s\\\\\\,0x%!2s}}",g+1,g+10,g+15,g+20,g+25,g+27,g+29,g+31,g+33,g+35);
    }
    else
    {
      OutBuffer.PrintF(L"{0x%!8s\\,{0x%!4s\\,0x%!4s\\,0x%!4s}\\,{0x%!2s\\,0x%!2s\\,0x%!2s\\,0x%!2s\\,0x%!2s\\,0x%!2s}}",g+1,g+10,g+15,g+20,g+25,g+27,g+29,g+31,g+33,g+35);
    }
    
    sFORALL(config->Defines,name)
    {
      PrintList(OutBuffer,L"DEFINES",n);
      OutBuffer.PrintF(L" -D%s",*name);
    }

    // libs
    n=0;
    sFORALL(links,name)
    {
      PrintList(OutBuffer,L"LIBS",n);
      if((*name)[0]=='$')
        OutBuffer.PrintF(L" %s",*name);
      else
        OutBuffer.PrintF(L" -l%s",*name);
    }
    if(n==0)
      PrintList(OutBuffer,L"LIBS",n);

    // includes
    n=0;
    OmniPath *path;
    sFORALL(Project->Includes,path)
    {
      PrintList(OutBuffer,L"INCDIR",n);
      OutBuffer.PrintF(L" -I%s",path->Path(OmniPath::PT_MAKEFILE));
    }
    if (n==0)
      PrintList(OutBuffer,L"INCDIR",n);

    // misc

    OutBuffer.PrintChar('\n');
    OutBuffer.PrintF(L"TARGET      = $(CONFIG)/%s\n",Project->MakeTarget);
  
    // the text

    OutBuffer.Print(L"\n#\n# defines\n#\n\n");
    OutBuffer.PrintF(L"ALTONAPATH = \"%s\"\n",RootPath.Path(OmniPath::PT_MAKEFILE));
    OutBuffer.Print(config->MakeDefine.Get());

    OutBuffer.Print(L"\n#\n# the main target\n#\n\n");
    if(nObjFiles)
    {
      if(Project->ConfigurationType == 1)
      {
        OutBuffer.PrintF(L"$(TARGET): $(OBJS) $(DEPS)\n");
        OutBuffer.Print(L"\t@echo \"LD  $(TARGET)\"\n");
        OutBuffer.Print(config->LinkExe.Get());
      }
      else
      {
        OutBuffer.PrintF(L"$(TARGET): $(OBJS)\n");
        OutBuffer.Print(L"\t@echo \"AR  $(TARGET)\"\n");
        OutBuffer.Print(config->LinkLib.Get());
      }
      if (customString[0]!=0)
      {
        OutBuffer.Print(L"\n#\n# custom build step dependencies\n#\n\n");
        OutBuffer.PrintF(L"$(OBJS): | %s\n",customString);
      }
    }
    else
      OutBuffer.Print(L"$(TARGET):\n\n");
    
    OutBuffer.Print(L"\n#\n# other targets\n#\n\n");
    OutBuffer.Print(config->MakeTarget.Get());
  }

  OutBuffer.PrintF(L"\nelse\n$(error CONFIG set to an invalid value)\n");
  for (sInt i=0; i<numendifs; i++)
    OutBuffer.PrintF(L"endif\n");

  if(WriteFlag)
  {
    sString<sMAXPATH> filename;

    filename = Project->Path.Path(OmniPath::PT_SYSTEM);
    filename.Add(L"/Makefile.");
    filename.Add(Project->Name);
    if (!defconfig->MakefileExtension.IsEmpty())
    {
      filename.Add(L".");
      filename.Add(defconfig->MakefileExtension);
    }
    if(TestFlag)
      filename.Add(L".txt");

    if(!BriefFlag)
      sPrintF(L"  write <%s>\n",filename);
    
    sSaveTextAnsiIfDifferent(filename,OutBuffer.Get());    
  }
}


/****************************************************************************/

