/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_TOOLS_MAKEPROJECT_DOC_HPP
#define FILE_ALTONA2_TOOLS_MAKEPROJECT_DOC_HPP

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Util/Scanner.hpp"

using namespace Altona2;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/
/*
enum cProjectTypes
{
  cPT_Project = 0,
  cPT_Library = 1,
  cPT_Max = 2,
};
*/

enum OutputKindEnum
{
    OK_Error = 0,
    OK_VS2010 = 1,
    OK_VS2008 = 2,
    OK_Make = 3,
    OK_XCode4 = 4,
    OK_VS2012 = 5,
    OK_NDK = 6,
    OK_VS2013 = 7,
};

class cVSItem : public sRCObject
{
public:
    cVSItem() { Add=0; PlatformBits=0xff; }
    ~cVSItem() {}	
    cVSItem *Copy();

    sInt PlatformBits;
    sInt Add;
    sPoolString Key;
    sPoolString Value;
};

class cVSGroup : public sRCObject
{
public:
    cVSGroup() {}
    ~cVSGroup() { VSItems.ReleaseAll(); }
    
    cVSGroup *Copy();

    cVSItem *AddItem(sPoolString name,int mask=3);
    sPoolString Name;
    OutputKindEnum Compiler;
    sArray<cVSItem *> VSItems;
};

class cOptions : public sRCObject
{
public:
    cOptions() { Excluded=0; }
    ~cOptions() { VSGroups.ReleaseAll(); }

    cOptions *Copy();

    cVSGroup *AddGroup(sPoolString name,OutputKindEnum compiler);
    sPoolString Name;
    sArray<cVSGroup *> VSGroups;
    sBool Excluded;
};


/****************************************************************************/

enum mToolId
{
    mTI_none = 1,
    mTI_cpp,
    mTI_c,
    mTI_hpp,    // and *.h
    mTI_incbin,
    mTI_asc,
    mTI_asm,
    mTI_m,
    mTI_mm,
    mTI_xib,
    mTI_packfile,
    mTI_pak,
    mTI_ops,
    mTI_lib,
    mTI_rc,
    mTI_dat,
    mTI_xml,
    mTI_para,
};

class mFile
{
public:
    mFile() { NoNew = 0; Parent = 0; }
    ~mFile() { Options.ReleaseAll(); }

    sPoolString Name;
    sPoolString OriginalName;
    sPoolString NameWithoutExtension;
    sPoolString AdditionalDependencies;
    sInt NoNew;
    sArray<cOptions *> Options;
    mToolId ToolId;
    sString<32> XCodeFileGuid;
    sString<32> XCodeRefGuid;
    sString<sMaxPath> FullPath;
    class mProject *Project;
    class mFolder *Parent;
};

class mFolder
{
public:
    mFolder() { Parent = 0; }
    ~mFolder() { Folders.DeleteAll(); Files.DeleteAll(); Options.ReleaseAll(); }

    sPoolString Name;
    sPoolString NamePath;
    sArray<cOptions *> Options;
    sArray<mFolder *> Folders;
    sArray<mFile *> Files;
    mFolder *Parent;
};

class mDepend
{
public:
    mDepend() { Project=0; }
    ~mDepend() {}

    sPoolString Name;
    sPoolString PathName;
    class mProject *Project;
};

class mProject
{
public:
    mProject() { Root = 0; Dump = 0; }
    ~mProject() { delete Root; Depends.DeleteAll(); Configs.DeleteAll(); }

    sBool Library;
    sBool Dump;
    sPoolString Name;
    sPoolString SolutionPath;
    sPoolString TargetSolutionPath;
    sPoolString PathName;
    sPoolString Alias;
    sString<64> Guid;
    sU32 GuidHash;
    mFolder *Root;
    sArray<cOptions *> Configs;
    sArray<mDepend *> Depends;
};

class mSolution
{
public:
    mSolution() {}
    ~mSolution() { Projects.DeleteAll(); }

    sArray<mProject *> Projects;
    sPoolString LicenseString;
    sPoolString Path;
};

class mPlatform
{
public:
    mPlatform() {}
    ~mPlatform() { Configs.DeleteAll(); }

    sPoolString Name;
    sArray<cOptions *> Configs;
};

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

struct Env
{
    sPoolString Key;
    sPoolString Value;
};

struct HeaderDependency
{
    sPoolString Filename;
    sBool Checked;
    sArray<HeaderDependency *> Dep;
};

/****************************************************************************/

class Document
{
    Altona2::sScanner Scan;
    sString<16*1024> LargeBuffer;

    // scan mp.txt files

    void AddOptions(cOptions *cop,cOptions *opt);
    cOptions *_C_Option();
    cOptions *_C_Combine();
    void _C_Global();

    void _MP_Folder(mProject *pro,mFolder *fol);
    void _MP_Project(mSolution *sol,sBool lib);
    void _MP_Global(mSolution *sol);
    sBool ScanFile(const sChar *dir);
    sBool ScanMPR(const sChar *dir);

    // output

    sTextLog sln;
    sTextLog vcx;
    sTextLog vcxf;

    sArray<mFolder *> _folders; // vs2012/13 only
    sArray<mFile *> _cpp;
    sArray<mFile *> _hpp;
    sArray<mFile *> _none;
    sArray<mFile *> _tool;
    sArray<mFile *> _lib;    // vs2012/13 only
    sArray<mFile *> _rc;    // vs2012/13 only

    void OutputVSSln(mProject *pro,OutputKindEnum compiler);
    void OutputFolder(mFolder *fol,sArray<mFile *> &files);

    void OutputVS2008Folder(mFolder *fol,mProject *pro);
    sBool OutputVS2008(mProject *pro);

    void OutputVS2010Options(mProject *pro,mFile *,sPoolString tool);
    void OutputVS2010Folder(mFolder *fol);
    sBool OutputVS2010(mProject *pro);

    void OutputVS2012ItemGroup(sArray<mFile*> &arr,const sChar *tool,mFolder *root);
    void OutputVS2012Options(mProject *pro,mFile *,sPoolString tool);
    void OutputVS2012Folder(mFolder *fol);
    void OutputVS2012Config(mProject *pro,bool x64,bool propertygroup,const sChar *x64name);
    sBool OutputVS2012(mProject *pro);

    void OutputMakeDependencies(mFile *file);
    sBool OutputMake(mProject *pro);

    sBool OutputXCode4(mProject *pro);

    sBool OutputNDK(mProject *pro);


    void Create(mSolution *sol,mProject *pro,mFolder *fol);
    HeaderDependency *CheckIncludes(sPoolString file);
    void PrintIncludes(HeaderDependency *dep,sTextLog &log);

public:
    Document();
    ~Document();

    // options

    sString<sMaxPath> TargetProject;
    sString<sMaxPath> RootPath;
    sString<sMaxPath> TargetRootPath;
    sDict<sPoolString,sPoolString> Licenses;
    sInt CreateNewFiles;
    sInt SvnNewFiles;
    sInt Pretend;
    sInt ForceInfo;
    OutputKindEnum OutputKind;
    mPlatform *Platform;
    sInt CygwinPath;   //only used for android

    sArray<Env> Environment;
    void Set(sPoolString key,sPoolString value);
    void Resolve(const sStringDesc &dest,const sChar *source);
    const sChar *FindKey(mProject *pro,sPoolString config,sPoolString toolname,sPoolString item,OutputKindEnum compiler);

    // process

    sBool ScanConfig();
    sBool ScanMP();
    sBool FindDepends();
    sBool OutputVS2012();
    sBool OutputVS2010();
    sBool OutputVS2008();
    sBool OutputMake();
    sBool OutputNDK();
    sBool OutputXCode4();
    void Create();

    sInt X64;
    sInt Manifest;

    sArray<mSolution *> Solutions;
    sArray<cOptions *> OptionFrags;
    sArray<mPlatform *> Platforms;
    sArray<cOptions *> AllConfigs;

    // header dependencies

    sArray<HeaderDependency *> HeaderDeps;

    void PrintIncludes(sPoolString file,sTextLog &log);

    // some outputs

    sTextLog Log;
    void OutputMpx(mProject *proj,mFolder *folder,const char *foldername);
    void OutputMpx(mProject *proj);
    void OutputMpx();
};


/****************************************************************************/


#endif  // FILE_ALTONA2_TOOLS_MAKEPROJECT_DOC_HPP
