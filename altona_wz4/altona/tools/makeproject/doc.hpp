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

#ifndef HEADER_ALTONA_TOOLS_MAKEPROJECT_DOC
#define HEADER_ALTONA_TOOLS_MAKEPROJECT_DOC

#ifndef __GNUC__
#pragma once
#endif

#include "base/types2.hpp"
#include "util/scanner.hpp"
#include "util/scanconfig.hpp"
#include "base/system.hpp"

/****************************************************************************/

void FixGUID(sPoolString &ps);

/****************************************************************************/

#define sVSPLAT_WIN32   0x00000001
#define sVSPLAT_WIN64   0x00000002
#define sVSPLAT_LINUX   0x00020000

// class for storing an absolute path working on different platforms
class OmniPath
{
protected:
  sString<sMAXPATH> TempPath;
public:
  enum PathType
  {
    PT_SYSTEM,    // for system makeproject runs on
    PT_SOLUTION,  // for visual studio solution
    PT_MAKEFILE,  // for makefiles
    PT_MAX,
  };

  const sChar *Path(PathType p);

  sString<sMAXPATH> RootPath[PT_MAX];  // rootpath for makefiles
  sString<sMAXPATH> Path_Relative;      // path concatenated with one of the rootpaths
};


enum OptionCond
{
  OC_ALWAYS = 0,
  OC_LIBRARY = 1,
};

struct Option
{
  sPoolString Name;
  sPoolString Value;
  OptionCond Condition;
  sInt NegateCondition;

  Option() { Condition = OC_ALWAYS; NegateCondition=0; }
};

class Tool
{
public:
  sPoolString Name;
  sAutoArray<Option *> Options;

  Option *GetOption(sPoolString name);
  void SetOption(sPoolString name,sPoolString value);
  void CopyFrom(Tool *s);
  void AddFrom(Tool *s);
};

enum ConfigExprEnum
{
  CE_TRUE = 0,
  CE_AND = 1,
  CE_OR = 2,
  CE_NOT = 3,
  CE_MATCH = 4,
};

struct ConfigExpr
{
  sInt Op;
  ConfigExpr *Left,*Right;
  const sChar *String;
  sBool Eval(const sChar *name);
};

class Config
{
public:
  sPoolString Name;               // name of the config. used for base configs
//  sPoolString Wildcard;           // pattern matching for config. used for file & folder configs
  ConfigExpr *Predicate;          // pattern matching for config. used for file & folder configs
  sPoolString Alias;
  sAutoArray<Tool *> Tools;
  sBool ExcludeFromBuild;
  sArray<sPoolString> Defines;
  sArray<sPoolString> Links;
  sArray<sPoolString> Incdirs;
  sArray<sPoolString> FakeTargetOnce;
  sArray<sPoolString> FakeTargetAll;
  sArray<sPoolString> SourceExtensions;
  sArray<sPoolString> CustomExtensions;
  sArray<sPoolString> PropertySheets;
  sInt RequiresSDK;
  sU32 VSPlatformMask;
  sPoolString TargetExeTemplate;
  sPoolString TargetLibTemplate;
  sPoolString ObjectExtension;
  sPoolString MakefileExtension;
  sTextBuffer MakeTarget;
  sTextBuffer MakeDefine;
  sTextBuffer LinkExe;
  sTextBuffer LinkLib;

  Tool *GetTool(sPoolString name);
  void CopyFrom(Config *s);
  void AddFrom(Config *s);
  sInt BaseConfValid;

  Config(){ ExcludeFromBuild=0;  VSPlatformMask = 0; Predicate=0; RequiresSDK=0; BaseConfValid=0; } 
};

enum LicenseStatus
{
  sLS_UNKNOWN = 0,
  sLS_OPEN,
  sLS_CLOSED,
  sLS_FREE,
};

class LicenseInfo
{
public:
  sPoolString Name;
  sPoolString Owner;
  sPoolString OwnerURL;
  sPoolString License;
  sPoolString LicenseURL;
  sPoolString Text1;
  sPoolString Text2;
  sInt StartYear;
  sInt EndYear;
  LicenseStatus Status;
  sPoolString Header;
};

class SuffixInfo
{
public:
  sPoolString Suffix;
  sPoolString Tool;
  sPoolString AdditionalDependencyTag;
  ConfigExpr *ExcludePlatforms;

  SuffixInfo() { ExcludePlatforms = 0; }
};

class TargetInfo
{
public:
  sPoolString Name;
  sPoolString Alias;
  sArray<sPoolString> Extensions;
};

class File
{
public:
  File();
  ~File();
  Config*GetModification(const sChar *name,sU32 plat);
 
  sPoolString Name;
  sAutoArray<Config *> Modifications;
  sArray<sPoolString> AdditionalDependencies;
  sBool CreateFile;
  LicenseInfo *License;
  sInt Temp;
};

class ProjFolder
{
public:
  ProjFolder();
  ~ProjFolder();
  void Clear();

  void GetAllFiles(sArray<File *> &out);

  sPoolString Name;
  sAutoArray<File *> Files;
  sAutoArray<ProjFolder *> Folders;
  sAutoArray<Config *> Modifications;
};

class Depend
{
public:
  sPoolString Name;
  sPoolString Guid;
  class OmniPath     Path;
  class ProjectData *Project;     // this is set after the last input is parsed and the first output is written!
};

class DependExtern
{
public:
  DependExtern() { IfConfig = 0; }
  sPoolString Name;
  sPoolString Guid;
  class OmniPath Path;
  ConfigExpr *IfConfig;               // build only for some configurations
};

class SolutionData
{
  public:
    sString<sMAXPATH> InputFile;
    sString<sMAXPATH> SolutionOutputFile;
    sString<sMAXPATH> GNUMakeOutputfile;
    OmniPath          Path;

    sAutoArray<Config *> Configs;
    sStaticBitArray<128> ConfigMask;                    // bitmask of configurations to create, -1 = all
    sBool ConfigAll;

    sBool GetConfigBit(sInt bit) { return ConfigAll || ConfigMask.Get(bit); }
    void ClearConfigBits() { ConfigMask.ClearAll(); ConfigAll = 1; }
    void SetConfigBit(sInt bit) { ConfigAll = 0; ConfigMask.Set(bit); }

    sArray<ProjectData *> Projects;
    ProjectData          *ProjectMain;
    
    sPoolString Name;
};

class CachedLink
{
public:
  Config *ProjectConfig;
  sArray<sPoolString> Links;
};

class ProjectData
{
public:
  sPoolString Guid;

  OmniPath          Path;             // path of the project
  sString<sMAXPATH> OutputFile;       // filename for project file  
  sString<sMAXPATH> MakeTarget;       // target for makefile, without path
  sString<sMAXPATH> EverythingPath;   // path supplied by the everything-keyword

  sArray<OmniPath> Includes;
  sPoolString Name;
  ProjFolder Files;
  
  sAutoArray<Depend *> Dependencies;  // project dependencies
  sAutoArray<DependExtern *> DependExterns; 
  sArray<sPoolString> Rules;          // additional rules
  sInt ConfigurationType;            // 1 == exe 2 == dll  4 == lib

  sAutoArray<CachedLink *> CachedLinks;

  sInt CycleDetect;                   // used during dependency sorting. 0=unknown, 1=being visited, 2=done
};

class Document
{
  sScanner Scan;

  sArray<SolutionData *> Solutions;
  SolutionData *Solution;
  sArray<ProjectData *> Projects;
  ProjectData  *Project;
  sArray<LicenseInfo *> Licenses;
  sArray<SuffixInfo *> Suffixes;
  sArray<TargetInfo *> Targets;
  LicenseInfo *CurrentLicense;

  sAutoArray<Config *> BaseConfigs;
  sAutoArray<Config *> BaseConfigsOld;
  sAutoArray<Config *> Builds;
  sAutoArray<ConfigExpr *> CEs;

//  Config *GetConfig(sPoolString name);

  void _Tool(Config *);
  void _ConfigX();
  void _Build();
  void _MakeTextBlock(sTextBuffer &tb);
  void _Config(sArray<Config *> &conflist,sBool ismodifier);
  void _Include(sBool predicate=1);  
  LicenseInfo *_DefineLicense();
  void _DefineSuffix();
  void _DefineTarget();
  void _Global();
  void _License();
  void _File(ProjFolder *folder,sBool predicate=1);
  void _Folder(ProjFolder *parent,sBool predicate=1);
  void _Guid();
  void _Create(sBool predicate=1);
  void _DependExtern(sBool predicate=1);
  void _Depend(sBool predicate=1);
  void _Project(sBool projectKeyword); 
  void _Everything();
  sBool _IfVal();
  sBool _IfAnd();
  sBool _IfOr();
  sBool _If();
  ConfigExpr *_CEVal();
  ConfigExpr *_CEAnd();
  ConfigExpr *_CEOr();
  ConfigExpr *_CEExpr();
  ConfigExpr *MakeCE(sInt op,ConfigExpr *l=0,ConfigExpr *r=0);

  sTextBuffer OutBuffer;

  void MakeEverythingDependencies(ProjectData *project);

  void OutputConfig(Config *conf,const sChar *plat);
  void OutputFolder(ProjFolder *folder,sInt indent);
  void OutputFile(File *file,sInt indent);
  void OutputMatrix(const sChar *guid,const sChar *name,ConfigExpr *p);
  void OutputTool(Tool *tool,sInt indent,const sChar *plat,Config *conf);
  void DoCreateNewFiles(ProjFolder *parent);
  void DoUpdateLicense(ProjFolder *parent);
  void OutputProject();

  void OutputFilters(ProjFolder &folder,const sChar *path=L"");
  void OutputFilterFiles(ProjFolder &folder,const sChar *path, sInt group);
  void OutputDependencies(ProjectData *project,sArray<ProjectData*> &done);
  void OutputXProject();

  void OutputSolution();
  void OutputProjectWithDependencies(ProjectData *eproject, sArray<ProjectData *> &alreadyInSolution);
  void OutputPrepareMakefile();
  void OutputSolutionMakefile();
  void OutputProjectMakefile();
  void AddMakefileFolder(ProjFolder *parent,sInt &n,Config *projcfg);
  sBool AddMakefileCustomSteps(ProjFolder *parent,sInt &n,Config *projcfg);
  CachedLink *GetMakefileCachedLinks(ProjectData *proj,Config *projcfg);

  void CreateNewProject();
  
  ProjectData *DetectCyclesR(ProjectData *cur);

  void WriteDependencyGraph(SolutionData *s);
  void WriteDepsForProjectR(sTextFileWriter &wr,ProjectData *proj);
  void WriteDepsForProjectFolderR(sTextFileWriter &wr,ProjectData *proj,ProjFolder *folder,sBool edges);
  void WriteDepsForFile(sTextFileWriter &wr,ProjectData *proj,File *file,sBool edges);
  File *FindMatchingFile(const sChar *name,ProjectData *proj,sBool onlyThisProject);
  File *FindFileR(const sChar *name,ProjFolder *folder);

public:
  Document();
  ~Document();

  sBool ParseGlobal(const sChar *filename);
  sBool ScanForSolutions(OmniPath *path);
  sBool ParseSolution();
  
  void  WriteProjectsAndSolutions();
  void  WriteDependencyGraphs(const sChar *pattern);
  void  Cleanup();
  sInt GetConfigCount();

  sBool TestFlag;
  sBool BriefFlag;
  sBool WriteFlag;
  sBool DebugFlag;
  sBool CheckLicenseFlag;
  sBool CreateNewFiles;

  sBool TargetLinux;

  sU32 VSPlatformMask;
  enum ConfMakefileEnum
  {
    MF_NONE = 0,
    MF_MINGW,
    MF_LINUX,
  } ConfMakefile;

  sScanConfig ConfigFile;         // this is parsed, but not yet used!

  
  sInt  VS_Version;
  sInt  VS_ProjectVersion;
  sInt  VS_SolutionVersion;
  sString<16> VS_ProjExtension;

  OmniPath RootPath;
  OmniPath IntermediatePath;     // where the intermediate data will be stored (obj, etc..)
  OmniPath OutputPath;           // where the libraries and executables will be stored 
};

/****************************************************************************/

// HEADER_ALTONA_TOOLS_MAKEPROJECT_DOC
#endif
