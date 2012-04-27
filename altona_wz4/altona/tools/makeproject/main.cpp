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
#include "main.hpp"
#include "doc.hpp"
#include "base/system.hpp"

#define VERSION 2
#define REVISION 63

//sINITMEM(sIMF_DEBUG|sIMF_CLEAR|sIMF_NORTL,1024*1024*1024);

/****************************************************************************/

static void SetTildeExpand(sString<sMAXPATH> &dest,const sChar *path)
{
  if(path[0] == '~' && path[1]=='/') // expand home dir
  {
    dest = L"$(HOME)";
    dest.AddPath(path+2);
  }
  else
    dest = path;
}

void sMain()
{
  Document Doc;
  sString<sMAXPATH> path;
  const sChar *proj;
  sString<sMAXPATH> configbuffer;
  sString<sMAXPATH> projbuffer;

  // command line parameters

  Doc.WriteFlag = !(sGetShellSwitch(L"p")||sGetShellSwitch(L"-pretend"));
  Doc.CheckLicenseFlag = sGetShellSwitch(L"l")||sGetShellSwitch(L"-license");
  Doc.TestFlag = sGetShellSwitch(L"t")||sGetShellSwitch(L"-test");
  Doc.DebugFlag = sGetShellSwitch(L"d")||sGetShellSwitch(L"-debug");
  Doc.BriefFlag = !(sGetShellSwitch(L"v")||sGetShellSwitch(L"-verbose"));
  Doc.CreateNewFiles = sGetShellSwitch(L"c")||sGetShellSwitch(L"-create");

  if(sGetShellParameter(L"r",0))                    
    path.PrintF(L"%p",sGetShellParameter(L"r",0)); // this converts '\\' to '/'
  else
    path.PrintF(L"%p",sCONFIG_CODEROOT);
  

  const sChar *depPattern = sGetShellParameter(L"D",0);

  projbuffer = path;
  projbuffer.AddPath(sCONFIG_MP_TEMPLATES);
  proj = projbuffer;

  Doc.RootPath.RootPath[OmniPath::PT_SYSTEM]    = path;
  Doc.RootPath.RootPath[OmniPath::PT_SOLUTION]  = path; // was sCONFIG_CODEROOT_WINDOWS, which was sCONFIG_CODEROOT, but doesn't work -r param
  SetTildeExpand(Doc.RootPath.RootPath[OmniPath::PT_MAKEFILE],path);

  configbuffer = path;
  configbuffer.AddPath(sCONFIG_CONFIGFILE);
  Doc.ConfigFile.Scan(configbuffer);
  Doc.ConfigFile.DPrint();

  if(sCmpString(Doc.ConfigFile.VSVersion,L"2008")>=0)
  {
    Doc.VS_Version = 9;
    Doc.VS_ProjectVersion  = 9;
    Doc.VS_SolutionVersion = 10; 
  }

  if(sCmpString(Doc.ConfigFile.VSVersion,L"2010")>=0)
  {
    Doc.VS_ProjExtension = L".vcxproj";
    Doc.VS_Version = 10;
    Doc.VS_ProjectVersion  = 10;
    Doc.VS_SolutionVersion = 11; 
  }

  Doc.IntermediatePath.RootPath[OmniPath::PT_SYSTEM] = Doc.ConfigFile.IntermediateRoot;
  Doc.IntermediatePath.RootPath[OmniPath::PT_SOLUTION] = Doc.ConfigFile.IntermediateRoot;
  SetTildeExpand(Doc.IntermediatePath.RootPath[OmniPath::PT_MAKEFILE],Doc.ConfigFile.IntermediateRoot);

  Doc.OutputPath.RootPath[OmniPath::PT_SYSTEM] = Doc.ConfigFile.OutputRoot;
  Doc.OutputPath.RootPath[OmniPath::PT_SOLUTION] = Doc.ConfigFile.OutputRoot;
  SetTildeExpand(Doc.OutputPath.RootPath[OmniPath::PT_MAKEFILE],Doc.ConfigFile.OutputRoot);

  Doc.VSPlatformMask = 0;
  if(Doc.ConfigFile.Solution & sSLN_WIN32)    Doc.VSPlatformMask |= sVSPLAT_WIN32;
  if(Doc.ConfigFile.Solution & sSLN_WIN64)    Doc.VSPlatformMask |= sVSPLAT_WIN64;
  Doc.ConfMakefile = Document::MF_NONE;
  if(Doc.ConfigFile.Makefile==sMAKE_MINGW)    Doc.ConfMakefile = Document::MF_MINGW;
  if(Doc.ConfigFile.Makefile==sMAKE_LINUX)    Doc.ConfMakefile = Document::MF_LINUX;


  sPrintF(L"makeproject v%d.%d\n",VERSION,REVISION);

  if(sGetShellSwitch(L"vs"))
  {
    sPrint(L"the '-vs' option is obsolete. please use 'altona_config.hpp'\n");
  }
  else if(sGetShellSwitch(L"h")||sGetShellSwitch(L"-help"))
  {
    sPrint(L"makeproject [options]\n");
    sPrint(L"-r path       root (default c:/svn2)\n");
    sPrint(L"-h --help     print this text\n");
    sPrint(L"-p --pretend  don't write destination files\n");
    sPrint(L"-t --test     write to .vsproj.txt\n");
    sPrint(L"-d --debug    output parser tree\n");
    sPrint(L"-v --verbose  more output\n");
    sPrint(L"-l --license  check and fix license header in all sources\n");
    sPrint(L"-c --create   create new files.\n");
    sPrint(L"-D pattern    write dependency graph of matching solutions.\n");
  }
  else if(path)
  {
    if(Doc.ParseGlobal(proj))
    {
      sPrintF(L"Scan for projects...\n");
      if (Doc.ScanForSolutions(&Doc.RootPath))
      {
        if(Doc.WriteFlag)
          sPrintF(L"Write Projects...\n");

        Doc.WriteProjectsAndSolutions();

        if(depPattern)
        {
          sPrintF(L"Write dependency graphs....\n");
          Doc.WriteDependencyGraphs(depPattern);
        }
        
        sPrintF(L"%d Configurations.\n",Doc.GetConfigCount());
        Doc.Cleanup();
      }
    }
  }
}

/****************************************************************************/
