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

void FixGUID(sPoolString &ps)
{
  if (!ps.IsEmpty())
  {
    sString<128> s = ps;
    sMakeUpper(s);
    ps = s;
  }
}

/****************************************************************************/

Option *Tool::GetOption(sPoolString name)
{
  Option *opt = sFind(Options,&Option::Name,name);
  if(opt==0)
  {
    opt = new Option;
    opt->Name = name;
    opt->Value = L"";
    Options.AddTail(opt);
  }
  return opt;
}

void Tool::SetOption(sPoolString name,sPoolString value)
{
  Option *opt = GetOption(name);
  opt->Value = value;
}


void Tool::CopyFrom(Tool *s)
{
  Option *os,*od;
  Name = s->Name;
  sFORALL(s->Options,os)
  {
    od = new Option;
    od->Condition = os->Condition;
    od->NegateCondition = os->NegateCondition;
    od->Name = os->Name;
    od->Value = os->Value;
    Options.AddTail(od);
  }
}

void Tool::AddFrom(Tool *s)
{
  Option *od,*os;

  sFORALL(s->Options,os)
  {
    od = GetOption(os->Name);
    od->Value = os->Value;
    if(od->Condition && os->Condition && (od->Condition!=os->Condition || od->NegateCondition!=os->NegateCondition))
      sFatal(L"mixed conditions in tool %s",os->Name);
    od->Condition = os->Condition;
    od->NegateCondition = os->NegateCondition;
  }
}

Tool *Config::GetTool(sPoolString name)
{
  Tool *tool = sFind(Tools,&Tool::Name,name);
  if(tool==0)
  {
    tool = new Tool;
    tool->Name = name;
    Tools.AddTail(tool);
  }
  return tool;
}

void Config::CopyFrom(Config *s)
{
  Tool *ts,*td;
  Name = s->Name;
//  Wildcard = s->Wildcard;
  Predicate = s->Predicate;
  Alias = s->Alias;
  VSPlatformMask = s->VSPlatformMask;
  ExcludeFromBuild = s->ExcludeFromBuild;
  sFORALL(s->Tools,ts)
  {
    td = new Tool;
    td->CopyFrom(ts);
    Tools.AddTail(td);
  }
  Defines.Add(s->Defines);
  Links.Add(s->Links);
  SourceExtensions.Add(s->SourceExtensions);
  CustomExtensions.Add(s->CustomExtensions);
  PropertySheets.Add(s->PropertySheets);
  TargetExeTemplate = s->TargetExeTemplate;
  TargetLibTemplate = s->TargetLibTemplate;
  MakefileExtension = s->MakefileExtension;
  ObjectExtension = s->ObjectExtension;
  MakeDefine = s->MakeDefine.Get();
  MakeTarget = s->MakeTarget.Get();
  LinkExe = s->LinkExe.Get();
  LinkLib = s->LinkLib.Get();
  FakeTargetOnce.Add(s->FakeTargetOnce);
  FakeTargetAll.Add(s->FakeTargetAll);
}

void Config::AddFrom(Config *s)
{
  Tool *ts,*td;
  ExcludeFromBuild |= s->ExcludeFromBuild;
  VSPlatformMask |= s->VSPlatformMask;
  RequiresSDK |= s->RequiresSDK;
  sFORALL(s->Tools,ts)
  {
    td = sFind(Tools,&Tool::Name,ts->Name);
    if(!td)
    {
      td = new Tool;
      td->Name = ts->Name;
      Tools.AddTail(td);
    }
    td->AddFrom(ts);
  }
  Defines.Add(s->Defines);
  Links.Add(s->Links);  
  SourceExtensions.Add(s->SourceExtensions);
  CustomExtensions.Add(s->CustomExtensions);
  PropertySheets.Add(s->PropertySheets);

  if (!s->TargetExeTemplate.IsEmpty())
    TargetExeTemplate = s->TargetExeTemplate;
  if (!s->TargetLibTemplate.IsEmpty())
    TargetLibTemplate = s->TargetLibTemplate;
  if (!s->MakefileExtension.IsEmpty())
    MakefileExtension = s->MakefileExtension;
  if (!s->ObjectExtension.IsEmpty())
    ObjectExtension = s->ObjectExtension;
  MakeDefine.Print(s->MakeDefine.Get());
  MakeTarget.Print(s->MakeTarget.Get());
  LinkExe.Print(s->LinkExe.Get());
  LinkLib.Print(s->LinkLib.Get());
  FakeTargetOnce.Add(s->FakeTargetOnce);
  FakeTargetAll.Add(s->FakeTargetAll);
}

File::File()
{
  CreateFile = 1;
  License = 0;
}

File::~File()
{
  sDeleteAll(Modifications);
}

Config *File::GetModification(const sChar *name,sU32 plat)
{
  Config *akku = sFind(Modifications,&Config::Name,name);
  if(!akku)
  {
    akku = new Config;
    akku->Name = name;
    akku->VSPlatformMask = plat;
    Modifications.AddTail(akku);
  }
  return akku;
}

ProjFolder::ProjFolder()
{
}

ProjFolder::~ProjFolder()
{
  sDeleteAll(Files);
  sDeleteAll(Folders);
}

void ProjFolder::Clear()
{
  sDeleteAll(Files);
  sDeleteAll(Folders);
}

void ProjFolder::GetAllFiles(sArray<File *> &out)
{
  ProjFolder *folder;
  sFORALL(Folders,folder)
    folder->GetAllFiles(out);
  out.Add(Files);
}

/****************************************************************************/

Document::Document()
{
  TestFlag=0;
  BriefFlag=0;
  WriteFlag=0;
  DebugFlag=0;
  CheckLicenseFlag = 0;
  Project = 0;
  TargetLinux = sFALSE;

  VS_Version          = 8;
  VS_ProjectVersion   = 8;
  VS_SolutionVersion  = 9; 
  VS_ProjExtension    = L".vcproj";
}

Document::~Document()
{
  sDeleteAll(Solutions);
  sDeleteAll(Projects);
  sDeleteAll(Licenses);
  sDeleteAll(Suffixes);
  sDeleteAll(Targets);
}
/*
void Document::SetVSVersion(sInt vsver)
{
  VS_Version = vsver;
  VS_ProjectVersion  = VS_Version;
  VS_SolutionVersion = VS_Version + 1; 
}
*/
void Document::_Tool(Config *conf)
{
  sPoolString option;
  sPoolString value;
  sPoolString name;


  Scan.ScanString(name);

  Tool *tool = conf->GetTool(name);

  Scan.Match('{');
  while(Scan.Errors==0 && Scan.Token!='}')
  {
    sBool ifok = 1;
    if(Scan.IfName(L"if"))
    {
      sPoolString cmpa,cmpb;
      Scan.Match('(');
      if(Scan.IfName(L"vsver")) cmpa = ConfigFile.VSVersion;
      else Scan.Error(L"config variable expected (vsver)");
      
      sInt cond = Scan.Token; Scan.Scan();
      if(!(cond==sTOK_EQ || cond==sTOK_NE || cond==sTOK_GE || cond==sTOK_LE || cond=='<' || cond=='>'))
        Scan.Error(L"compare operator expected");
      Scan.ScanString(cmpb);
      Scan.Match(')');

      switch(cond)
      {
        case sTOK_EQ:  ifok = (cmpa == cmpb); break;
        case sTOK_NE:  ifok = (cmpa != cmpb); break;
        case sTOK_GE:  ifok = (cmpa >= cmpb); break;
        case sTOK_LE:  ifok = (cmpa <= cmpb); break;
        case '>':      ifok = (cmpa >  cmpb); break;
        case '<':      ifok = (cmpa <  cmpb); break;
      }
    }
    Scan.ScanName(option);
    sInt appendmode = 0;
    if(Scan.IfToken('+'))
      appendmode = 1;
    Scan.Match('=');
    Scan.ScanString(value);

    if(ifok)
    {
      Option *opt = tool->GetOption(option);
      if(appendmode)
      {
        opt->Value.Add(opt->Value,L" ");
        opt->Value.Add(opt->Value,value);
      }
      else
      {
        opt->Value = value;
      }

      if(Scan.IfToken('!'))
        opt->NegateCondition = 1;
      if(Scan.IfName(L"iflibrary"))
      {
        if(opt->Condition!=OC_ALWAYS)
          Scan.Error(L"more than one condition on option");
        opt->Condition = OC_LIBRARY;
      }
    }

    Scan.Match(';');
  }

  Scan.Match('}');
}

void Document::_ConfigX()
{
  sPoolString cname;
  sPoolString bname;
  Config *conf,*aconf;

  // create config

  Scan.ScanString(cname);
  conf = sFind(BaseConfigs,&Config::Name,cname);
  if(conf!=0)
    Scan.Error(L"config <%s> specified twice",cname);
  conf = new Config;
  conf->Name = cname;

  // alias?

  if(Scan.IfName(L"alias"))
    Scan.ScanString(conf->Alias);

  // add builds

  while(!Scan.Errors)
  {
    Scan.ScanName(bname);
    aconf = sFind(Builds,&Config::Name,bname);
    if(aconf)
    {
      conf->AddFrom(aconf);
    }
    else
    {
      Scan.Error(L"build <%s> not found",bname);
    }

    if(!Scan.IfToken(',')) 
      break;
  }
  Scan.Match(';');

  conf->BaseConfValid = ((ConfigFile.SDK & conf->RequiresSDK)==conf->RequiresSDK);
  BaseConfigs.AddTail(conf);
}

void Document::_MakeTextBlock(sTextBuffer &tb)
{
  sPoolString line;
  const sChar *ptr;

  Scan.Match('{');
  while(!Scan.Errors && !Scan.IfToken('}'))
  {
    Scan.ScanString(line);
    ptr = line;
    if(*ptr==' ')
    {
      while(*ptr==' ')
        ptr++;
      tb.PrintChar('\t');
    }
    tb.Print(ptr);
    tb.PrintChar('\n');
  }
}

ConfigExpr *Document::_CEVal()
{
  if(Scan.IfToken('!'))
  {
    return MakeCE(CE_NOT,_CEVal());
  }
  else if(Scan.Token==sTOK_STRING)
  {
    sPoolString str;
    Scan.ScanString(str);
    const sChar *str2 = str;
    if(str2[0]=='!')
    {
      ConfigExpr *e0,*e1;

      // this is  an obsolete feature!
      e1 = MakeCE(CE_MATCH);
      e1->String = str2+1;
      e0 = MakeCE(CE_NOT,e1);
      return e0;
    }
    else
    {
      ConfigExpr *e1;

      e1 = MakeCE(CE_MATCH);
      e1->String = str2;
      return e1;
    }
  }
  else if(Scan.IfToken('('))
  {
    ConfigExpr *e = _CEExpr();
    Scan.Match(')');
    return e;
  }
  else
  {
    Scan.Error(L"illegal wildcard expression for configs");
    return MakeCE(CE_TRUE); 
  }
}

ConfigExpr *Document::_CEAnd()
{
  ConfigExpr *e;

  e = _CEVal();
  while(Scan.IfToken('&'))
    e = MakeCE(CE_AND,e,_CEVal());

  return e;
}

ConfigExpr *Document::_CEOr()
{
  ConfigExpr *e;

  e = _CEAnd();
  while(Scan.IfToken('|'))
    e = MakeCE(CE_OR,e,_CEAnd());

  return e;
}

ConfigExpr *Document::_CEExpr()
{
  return _CEOr();
}

sBool ConfigExpr::Eval(const sChar *name)
{
  if(this==0) return 1;
  switch(Op)
  {
  default:
  case CE_TRUE:
    return 1;
  case CE_AND:
    return Left->Eval(name) && Right->Eval(name);
  case CE_OR:
    return Left->Eval(name) || Right->Eval(name);
  case CE_NOT:
    return !Left->Eval(name);
  case CE_MATCH:
    return sMatchWildcard(String,name);
  }
}


ConfigExpr *Document::MakeCE(sInt op,ConfigExpr *l,ConfigExpr *r)
{
  ConfigExpr *e = new ConfigExpr;
  CEs.AddTail(e);
  e->Op = op;
  e->Left = l;
  e->Right = r;
  e->String = 0;
  return e;
}

void Document::_Config(sArray<Config *> &conflist,sBool ismodifier)
{
  sPoolString cmd;
  sPoolString base;
  sPoolString name;
  Config *conf;

  conf = new Config;
  if(ismodifier)
  {
    if(Scan.Token!='{')
      conf->Predicate = _CEExpr();
  }
  else
  {
    Scan.ScanString(name);

    if(sFind(conflist,&Config::Name,name))
      Scan.Error(L"config <%s> specified twice",name);

    conf->Name = name;

    if(Scan.IfName(L"base"))
    {
      Scan.ScanString(base);
      Config *bc = sFind(conflist,&Config::Name,base);
      if(bc==0)
        Scan.Error(L"base config <%s> not found",base);
      if(Scan.Errors==0)
      {
        conf->CopyFrom(bc);
        conf->Name = name;
      }
    }
  }
  conflist.AddTail(conf);

  Scan.Match('{');
  while(Scan.Errors==0 && Scan.Token!='}')
  {
    if(Scan.IfName(L"tool"))
    {
      _Tool(conf);
    }
    else if(Scan.IfName(L"exclude"))
    {
      conf->ExcludeFromBuild = 1;
      Scan.Match(';');
    }
    else if(Scan.IfName(L"define"))
    {
      sPoolString def;
      Scan.ScanString(def);
      conf->Defines.AddTail(def);
      Scan.Match(';');
    }
    else if(Scan.IfName(L"props"))
    {
      sPoolString prop;
      Scan.ScanString(prop);
      conf->PropertySheets.AddTail(prop);
      Scan.Match(';');
    }
    else if(Scan.IfName(L"link"))
    {
      sPoolString link;
      Scan.ScanString(link);
      conf->Links.AddTail(link);
      Scan.Match(';');
    }
    else if(Scan.IfName(L"platform"))
    {
      sPoolString plat;
      Scan.ScanString(plat);
      Scan.Match(';');
      if(plat==L"win32")
        conf->VSPlatformMask |= sVSPLAT_WIN32;
      else if(plat==L"win64")
        conf->VSPlatformMask |= sVSPLAT_WIN64;
      else if(plat==L"linux")
        conf->VSPlatformMask |= sVSPLAT_LINUX;
      else
        Scan.Error(L"unknown platform <%s>. Should be win32, linux or win64", plat);
    }
    else if(Scan.IfName(L"makedefine"))
    {
      _MakeTextBlock(conf->MakeDefine);
    }
    else if(Scan.IfName(L"maketarget"))
    {
      _MakeTextBlock(conf->MakeTarget);
    }
    else if(Scan.IfName(L"linkexe"))
    {
      _MakeTextBlock(conf->LinkExe);
    }
    else if(Scan.IfName(L"linklib"))
    {
      _MakeTextBlock(conf->LinkLib);
    }
    else if(Scan.IfName(L"faketarget"))
    {
      if(Scan.IfName(L"once"))
      {
        Scan.ScanString(name);
        conf->FakeTargetOnce.AddTail(name);
      }
      else if(Scan.IfName(L"all"))
      {
        Scan.ScanString(name);
        conf->FakeTargetAll.AddTail(name);
      }
      else
      {
        Scan.Error(L"expected 'once' or 'all' for faketarget definition");
      }
      Scan.Match(';');
    }
    else if(Scan.IfName(L"target"))
    {
      if(Scan.IfName(L"exe"))
      {
        Scan.Match('=');
        Scan.ScanString(conf->TargetExeTemplate);
      }
      else if(Scan.IfName(L"lib"))
      {
        Scan.Match('=');
        Scan.ScanString(conf->TargetLibTemplate);
      }
      else 
      {
        Scan.Error(L"expected 'exe' or 'lib' for target name template definition");
      }
      Scan.Match(';');
    }
    else if(Scan.IfName(L"requires"))
    {
      sPoolString sdk;
      Scan.ScanName(sdk);
      Scan.Match(';');
      if(!Scan.Errors)
      {
        sInt val = sParseSDK(sdk);
        if(val==0)
          Scan.Error(L"can't find sdk %q",sdk);
        conf->RequiresSDK |= val;
      }
    }
    else if(Scan.IfName(L"extension"))
    {
      if (Scan.IfName(L"src"))
      {
        Scan.Match('=');
        do
        {
          sPoolString srcext;
          Scan.ScanString(srcext);
          conf->SourceExtensions.AddTail(srcext);
        }
        while (Scan.IfToken(','));
      }
      else if (Scan.IfName(L"custom"))
      {
        Scan.Match('=');
        do
        {
          sPoolString srcext;
          Scan.ScanString(srcext);
          conf->CustomExtensions.AddTail(srcext);
        }
        while (Scan.IfToken(','));
      }
      else if (Scan.IfName(L"obj"))
      {
        Scan.Match('=');
        Scan.ScanString(conf->ObjectExtension);
      }
      else if (Scan.IfName(L"makefile"))
      {
        Scan.Match('=');
        Scan.ScanString(conf->MakefileExtension);
      }
      else 
      {
        Scan.Error(L"expected 'src', 'obj' or 'makefile' for extension definition");
      }
      Scan.Match(';');
    }
    else
    {
      Scan.Error(L"syntax error");
    }
  }
  Scan.Match('}');
}

void Document::_Global()
{
  sPoolString cmd;
  while(Scan.Errors==0 && Scan.Token!=sTOK_END)
  {
    if(Scan.IfName(L"config"))
    {
      _ConfigX();
    }
    else if(Scan.IfName(L"build"))
    { 
      _Config(Builds,sFALSE);
    }
    else if(Scan.IfName(L"definelicense"))
    {
      _DefineLicense();
    }
    else if(Scan.IfName(L"defineextension"))
    {
      _DefineSuffix();
    }
    else if(Scan.IfName(L"definetarget"))
    {
      _DefineTarget();
    }
    else
    {
      Scan.Error(L"syntax error at");
    }
  }
}

/****************************************************************************/

void Document::_File(ProjFolder *folder,sBool predicate)
{
  LicenseInfo *pushlic = CurrentLicense;

  sBool createFile = 1;

  while (Scan.Token==sTOK_NAME)
  {
    sPoolString option;
    Scan.ScanName(option);

    if (option==L"nonew")
    {
      createFile = sFALSE;
    }
    else
    {
      Scan.Error(L"Error: unknown file option \"%s\" in solution \"%s\"\n",option, Solution->Name);
    }
  }

  File *file = new File;
  file->CreateFile = createFile;
  sString<sMAXPATH> buffer;

  Scan.ScanString(file->Name);
  buffer.PrintF(L"%p",file->Name); // fix slashes etc
  file->Name = buffer;

  if(Scan.IfName(L"license"))
    _License();
  file->License = CurrentLicense;

//  sDPrintF(L"%s %s\n",file->Name,file->License?file->License->Name:L"default");

  if(predicate)
  {
    sInt pos = sFindLastChar(file->Name,'?');
    if(pos>=0)
    {
      sString<256> buffer;

      buffer = file->Name;
      buffer[pos] = 'c';
      file->Name = buffer;
      folder->Files.AddTail(file);

      File *file2 = new File;
      file2->CreateFile = createFile;
      file2->License = file->License;

      buffer[pos] = 'h';
      file2->Name = buffer;
      folder->Files.AddTail(file2);
    }
    else
    {
      folder->Files.AddTail(file);
    }
  }
  else
  {
    delete file;
    file = 0;
  }

  sAutoArray<Config *> FileConfigs;

  if(Scan.Token==';')
  {
    Scan.Match(';');
  }
  else
  {
    // get some configurations

    sPoolString cmd;
    Scan.Match('{');
    while(Scan.Errors==0 && Scan.Token!='}')
    {
      Scan.ScanName(cmd);
      if(cmd==L"config")
      {
        _Config(FileConfigs,sTRUE);
      }
      else if(cmd==L"depend")
      {
        sPoolString dep;
        Scan.ScanString(dep);
        file->AdditionalDependencies.AddTail(dep);
        Scan.Match(';');
      }
      else
      {
        Scan.Error(L"syntax error at <%s>",cmd);
      }
    }
    Scan.Match('}');
  }

  // find file extension. then modify parameters by file extension

  SuffixInfo *info = 0;
  Config *ExcludeConfig = 0;
  const sChar *ext = 0;
  if(file)
  {
    ext = sFindFileExtension(file->Name);
    if(ext)
    {
      info = sFind(Suffixes,&SuffixInfo::Suffix,ext);
    }
  }

  // exclude by platform and file extension

  if(info && info->ExcludePlatforms)
  {
    ExcludeConfig = new Config;
    ExcludeConfig->ExcludeFromBuild = 1;
    ExcludeConfig->Predicate = info->ExcludePlatforms;
  }

  // additional dependencies

  Config *DependConfig = 0;
  if(info && file->AdditionalDependencies.GetCount()>0)
  {
    Tool *tool = 0;
    if(info->Tool.IsEmpty())
    {
      Scan.Error(L"no tool defined for file extension %q",ext);
    }
    else 
    {
      DependConfig = new Config;
      tool = DependConfig->GetTool(info->Tool);
      sPoolString *str;
      sTextBuffer tb;
      sFORALL(file->AdditionalDependencies,str)
      {
        if(_i==0) tb.Print(L" ");
        tb.PrintF(L"&quot;%s&quot;",*str);
      }
      tool->SetOption(info->AdditionalDependencyTag,tb.Get());
    }
  }

  // merge all the modifications

  sArray<Config *> TempConfigs;
  TempConfigs.Add(folder->Modifications);
  TempConfigs.Add(FileConfigs);
  if(ExcludeConfig) TempConfigs.AddTail(ExcludeConfig);
  if(DependConfig) TempConfigs.AddTail(DependConfig);

  // resolve wildcards

  if(TempConfigs.GetCount()>0)
  {
    if(!Scan.Errors && file)
    {
      Config *wild,*check,*akku;
      sBool found;

      sFORALL(TempConfigs,wild)
      {
        found = 0;
        sFORALL(Solution->Configs,check)
        {
          if(!wild->Predicate || wild->Predicate->Eval(check->Name))
          {
            found = 1;
            akku = file->GetModification(check->Name,check->VSPlatformMask);
            akku->AddFrom(wild);
          }
        }
      }
    }
  }
  CurrentLicense = pushlic;

  delete ExcludeConfig;
  delete DependConfig;
}

void Document::_Folder(ProjFolder *parent,sBool predicate)
{
  sPoolString cmd;
  LicenseInfo *puhslic = CurrentLicense;
  ProjFolder *folder = new ProjFolder;
  if(predicate) 
    parent->Folders.AddTail(folder);
  Config *mod;
  sFORALL(parent->Modifications,mod)
  {
    Config *t = new Config;
    t->CopyFrom(mod);
    folder->Modifications.AddTail(t);
  }

  Scan.ScanString(folder->Name);
  Scan.Match('{');

  while(Scan.Errors==0 && Scan.Token!='}')
  {
    Scan.ScanName(cmd);
    if(cmd==L"folder")
    {
      _Folder(folder);
    }
    else if(cmd==L"file")
    {
      _File(folder);
    }
    else if(cmd==L"if")
    {
      sBool cond = _If();
      Scan.ScanName(cmd);
      if(cmd==L"file")
        _File(folder,cond);
      else
        Scan.Error(L"syntax error at <%s>",cmd);
    }
    else if(cmd==L"license")
    {
      _License();
      Scan.Match(';');
    }
    else if(cmd==L"config")
    {
      _Config(folder->Modifications,sTRUE);
    }
    else
    {
      Scan.Error(L"syntax error at <%s>",cmd);
    }
  }
  Scan.Match('}');
  CurrentLicense = puhslic;

  if(!predicate)
    delete folder;
}

void Document::_Guid()
{
  Scan.ScanString(Project->Guid);
  FixGUID(Project->Guid);
  Scan.Match(';');
}


void Document::_Everything()
{
  sPoolString path;
  Scan.ScanString(path);
  Scan.Match(';');
  sSPrintF(Project->EverythingPath,L"%s",path);
}


sBool Document::_IfVal()
{
  sBool result = 0;

  if(Scan.Token==sTOK_INT)
  {
    result = Scan.ScanInt();
  }
  else if(Scan.Token=='(')
  {
    Scan.Match('(');
    result = _IfOr();
    Scan.Match(')');
  }
  else if(Scan.Token=='!')
  {
    Scan.Match('!');
    result = !_IfVal();
  }
  else if(Scan.Token==sTOK_NAME)
  {
    if     (Scan.IfName(L"sSDK_DX9"))     result = (ConfigFile.SDK & sSDK_DX9)!=0;
    else if(Scan.IfName(L"sSDK_DX11"))    result = (ConfigFile.SDK & sSDK_DX11)!=0;
    else if(Scan.IfName(L"sSDK_CG"))      result = (ConfigFile.SDK & sSDK_CG)!=0;
    else if(Scan.IfName(L"sSDK_CHAOS"))   result = (ConfigFile.SDK & sSDK_CHAOS)!=0;
    else if(Scan.IfName(L"sSDK_XSI"))     result = (ConfigFile.SDK & sSDK_XSI)!=0;
    else if(Scan.IfName(L"sSDK_GECKO"))   result = (ConfigFile.SDK & sSDK_GECKO)!=0;
    else if(Scan.IfName(L"sSLN_WIN32"))   result = (ConfigFile.Solution & sSLN_WIN32)!=0;
    else if(Scan.IfName(L"sSLN_WIN64"))   result = (ConfigFile.Solution & sSLN_WIN64)!=0;
    else if(Scan.IfName(L"sMAKE_MINGW"))  result = (ConfigFile.Makefile & sMAKE_MINGW)!=0;
    else if(Scan.IfName(L"sMAKE_LINUX"))  result = (ConfigFile.Makefile & sMAKE_LINUX)!=0;

    else Scan.Error(L"unknown conditional <%s>",Scan.Name);
  }
  else
    Scan.Error(L"syntax error");

  return result;
}

sBool Document::_IfAnd()
{
  sBool result = _IfVal();
  while(Scan.IfToken('&'))
    result &= _IfVal();
  return result;
}

sBool Document::_IfOr()
{
  sBool result = _IfAnd();
  while(Scan.IfToken('|'))
    result |= _IfAnd();
  return result;
}

sBool Document::_If()
{
  Scan.Match('(');
  sBool result = _IfOr();
  Scan.Match(')');
  return result;
}

void Document::_Create(sBool predicate)
{
  sInt index;
  sPoolString name;
  Scan.ScanString(name);
  Scan.Match(';');

  sInt found = 0;
  if(Scan.Errors)
    found=1;
  if(!predicate)
    found=1;

  if(!found)
  {
    index = sFindIndex(Solution->Configs,&Config::Name,name);
    if(index>=0)
    {
      Solution->SetConfigBit(index);
      found = 1;
    }
  }
  if(!found)
  {
    index = sFindIndex(Solution->Configs,&Config::Alias,name);
    if(index>=0)
    {
      Solution->SetConfigBit(index);
      found = 1;
      sPrintF(L"%q: please replace old config name %q with new one (%q)\n",
        Scan.Stream->Filename,Solution->Configs[index]->Alias,Solution->Configs[index]->Name);
    }
  }

  // ignore platforms that have SDK's missing

  if(!found)
    if(sFind(BaseConfigs,&Config::Name,name))
      found = 1;
  if(!found)
  {
    if(sFind(BaseConfigs,&Config::Alias,name))
    {
      found = 1;
      sPrintF(L"%q: please replace old config name %q with new one\n",Scan.Stream->Filename,name);
    }
  }

  if(!found)
    Scan.Error(L"configuration <%s> not found",name);
}

void Document::_DependExtern(sBool predicate)
{
  sPoolString path;
  sPoolString guid;

  ConfigExpr *expr = 0;
  if(Scan.IfName(L"config"))
    expr = _CEExpr();

  Scan.ScanString(path);
  Scan.MatchName(L"guid");
  Scan.ScanString(guid);
  Scan.Match(';');
  if(!Scan.Errors && predicate)
  {
    DependExtern *dep = new DependExtern;

    sString<sMAXPATH> buffer;

    sInt i0 = sFindLastChar(path,'/');
    sInt i1 = sFindLastChar(path,'\\');
    sInt index = sMax(i0,i1);
    if(index<=0)
    {
      Scan.Error(L"could not name for extern dependency %q",path);
      delete dep;
    }
    else
    {
      dep->Name = path+index+1;
      dep->Path = RootPath;
      dep->Path.Path_Relative = path;

      FixGUID(guid);
      dep->Guid = guid;
      dep->IfConfig = expr;
      Project->DependExterns.AddTail(dep);
    }
  }
}

void Document::_Depend(sBool predicate)
{
  sPoolString path;

  Scan.ScanString(path);
  Scan.Match(';');
  if(!Scan.Errors && predicate)
  {
    Depend *dep = new Depend;

    sString<sMAXPATH> buffer;

    sInt i0 = sFindLastChar(path,'/');
    sInt i1 = sFindLastChar(path,'\\');
    sInt index = sMax(i0,i1);
    if(index<=0)
    {
      Scan.Error(L"could not find .mp.txt for <%s>",path);
    }
    else
    {
      dep->Name = path+index+1;
      dep->Path = RootPath;

      buffer = path;
      dep->Path.Path_Relative = buffer;
      buffer = dep->Path.Path(OmniPath::PT_SYSTEM);
      buffer.Add(path+index);
      
      buffer.Add(L".mp.txt");
      
      sScanner sub;
      sPoolString name;
      sub.Init();
      sub.AddTokens(L"{},.:;=!()");
      sub.StartFile(buffer);
      if(sub.Errors)
      {
        Scan.Error(L"could not find dependency project <%s>\n",buffer);
      }
      else
      {
        sub.ScanName(name);
        sub.ScanString(dep->Guid);
        FixGUID(dep->Guid);
        sub.Match(';');
        if(name!=L"guid")
          Scan.Error(L"for dependencies, in <%s> first command must be \"guid\"\n",buffer);
        if(sub.Errors)
          Scan.Error(L"syntax error in dependency project <%s>\n",buffer);
      }
    }

    if(Scan.Errors==0)
    {
      Project->Dependencies.AddTail(dep);
//      sDPrintF(L"  depend <%s> %s\n",dep->Name,dep->Guid);
    }
    else
    {
      delete dep;
    }
  }
}

void Document::_Include(sBool predicate)
{
  OmniPath path = RootPath;
  sPoolString buf;
  Scan.ScanString(buf);
  Scan.Match(';');

  path.Path_Relative = buf;
  if(predicate)
    Project->Includes.AddTail(path);
}

LicenseInfo *Document::_DefineLicense()
{
  LicenseInfo *lic = new LicenseInfo;
  lic->StartYear = 0;
  lic->EndYear = 0;
  lic->Status = sLS_UNKNOWN;
  lic->Name = L"#noname";

  if(Scan.Token==sTOK_NAME)
  {
    Scan.ScanName(lic->Name);
  }
  Scan.Match('{');
  while(!Scan.Errors && Scan.Token!='}')
  {
    if(Scan.IfName(L"owner"))
    {
      Scan.Match('=');
      Scan.ScanString(lic->Owner);
      Scan.Match(';');
    }
    else if(Scan.IfName(L"ownerurl"))
    {
      Scan.Match('=');
      Scan.ScanString(lic->OwnerURL);
      Scan.Match(';');
    }
    else if(Scan.IfName(L"license"))
    {
      Scan.Match('=');
      Scan.ScanString(lic->License);
      Scan.Match(';');
    }
    else if(Scan.IfName(L"licenseurl"))
    {
      Scan.Match('=');
      Scan.ScanString(lic->LicenseURL);
      Scan.Match(';');
    }
    else if(Scan.IfName(L"year"))
    {
      Scan.Match('=');
      lic->StartYear = Scan.ScanInt();
      if(Scan.IfToken(sTOK_ELLIPSES))
        lic->EndYear = Scan.ScanInt();
      else
        lic->EndYear = lic->StartYear;
      Scan.Match(';');
    }
    else if(Scan.IfName(L"status"))
    {
      Scan.Match('=');
      if(Scan.IfName(L"open"))
        lic->Status = sLS_OPEN;
      else if(Scan.IfName(L"closed"))
        lic->Status = sLS_CLOSED;
      else if(Scan.IfName(L"free"))
        lic->Status = sLS_FREE;
      else
        Scan.Error(L"unknown license status. should be open, closed or free");
      Scan.Match(';');
    }
    else if(Scan.IfName(L"text") || Scan.IfName(L"text1"))
    {
      Scan.Match('=');
      Scan.ScanString(lic->Text1);
      Scan.Match(';');
    }
    else if(Scan.IfName(L"text2"))
    {
      Scan.Match('=');
      Scan.ScanString(lic->Text2);
      Scan.Match(';');
    }
    else
    {
      Scan.Error(L"syntax error");
    }
  }
  Scan.Match('}');

  Licenses.AddTail(lic);

  // print license header

  sTextBuffer tb;
  sString<256> buffer;

  tb.PrintF(L"/*+**************************************************************************/\n");
  if(!lic->Text1.IsEmpty())
  {
    tb.PrintF(L"/***                                                                      ***/\n");
    tb.PrintF(L"/***   %-64s   ***/\n",lic->Text1);
    if(!lic->Text2.IsEmpty())
      tb.PrintF(L"/***   %-64s   ***/\n",lic->Text2);
  }
  else
  {
    if(!lic->Owner.IsEmpty())
    {
      tb.PrintF(L"/***                                                                      ***/\n");
      if(lic->EndYear)
        sSPrintF(buffer,L"Copyright (C) %d-%d by %s",lic->StartYear,lic->EndYear,lic->Owner);
      else if(lic->StartYear)
        sSPrintF(buffer,L"Copyright (C) %d by %s",lic->StartYear,lic->Owner);
      else
        sSPrintF(buffer,L"Copyright (C) by %s",lic->Owner);

      tb.PrintF(L"/***   %-64s   ***/\n",buffer);
      tb.PrintF(L"/***   all rights reserved                                                ***/\n");
    }
    if(!lic->OwnerURL.IsEmpty())
      tb.PrintF(L"/***   %-64s   ***/\n",lic->OwnerURL);

    if(!lic->License.IsEmpty())
    {
      tb.PrintF(L"/***                                                                      ***/\n");
      tb.PrintF(L"/***   Permission to use this software is granted by terms of the         ***/\n");
      tb.PrintF(L"/***   %s license. %_   ***/\n",lic->License,54-sGetStringLen(lic->License));
      if(!lic->LicenseURL.IsEmpty())
      {
        tb.PrintF(L"/***   A copy of this license can be retrieved at                         ***/\n");
        tb.PrintF(L"/***   %-64s   ***/\n",lic->LicenseURL);
      }
    }
    else
    {
      tb.PrintF(L"/***                                                                      ***/\n");
      tb.PrintF(L"/***   To license this software, please contact the copyright holder.     ***/\n");
    }
    switch(lic->Status)
    {
    case sLS_OPEN:
      tb.PrintF(L"/***                                                                      ***/\n");
      tb.PrintF(L"/***   This is open source software, but not free software. This          ***/\n");
      tb.PrintF(L"/***   software may be combined with closed source software.              ***/\n");
      break;
    case sLS_CLOSED:
      tb.PrintF(L"/***                                                                      ***/\n");
      tb.PrintF(L"/***   This is closed source software. It may not be combined with free   ***/\n");
      tb.PrintF(L"/***   software, like software under the GPL license. It may not be       ***/\n");
      tb.PrintF(L"/***   linked statically with LGPL software.                              ***/\n");
      break;
    case sLS_FREE:
      tb.PrintF(L"/***                                                                      ***/\n");
      tb.PrintF(L"/***   This is free software. It may not be combined with closed          ***/\n");
      tb.PrintF(L"/***   source software.                                                   ***/\n");
      break;
    default:
      break;
    }
  }
  tb.PrintF(L"/***                                                                      ***/\n");
  tb.PrintF(L"/**************************************************************************+*/\n\n");

  lic->Header = tb.Get();
  sDPrint(lic->Header);

  return lic;
}

void Document::_DefineSuffix()
{
  sPoolString name;
  sPoolString key;
  sPoolString value;
  SuffixInfo *info;

  Scan.ScanString(name);
  Scan.Match('{');

  info = sFind(Suffixes,&SuffixInfo::Suffix,name);
  if(info==0)
  {
    info = new SuffixInfo;
    info->Suffix = name;
    info->AdditionalDependencyTag = L"AdditionalDependencies";
    Suffixes.AddTail(info);
  }

  while(!Scan.Errors && !Scan.IfToken('}'))
  {
    Scan.ScanName(key);
    Scan.Match('=');
    if(key==L"tool")
    {
      Scan.ScanString(value);
      info->Tool = value;
    }
    else if(key==L"dependency")
    {
      Scan.ScanString(value);
      info->AdditionalDependencyTag = value;
    }
    else if(key==L"exclude")
    {
      sDelete(info->ExcludePlatforms);
      info->ExcludePlatforms = _CEExpr();
    }
    else
    {
      Scan.Error(L"unknown key %q in definesuffix",key);
    }

    Scan.Match(';');
  }
}

void Document::_DefineTarget()
{
  sPoolString name;
  Scan.ScanString(name);
  Scan.Match('{');

  TargetInfo *info = sFind(Targets,&TargetInfo::Name,name);
  if (!info)
  {
    info = new TargetInfo;
    info->Name = name;
    Targets.AddTail(info);
  }

  while(!Scan.Errors && !Scan.IfToken('}'))
  {
    sPoolString key;
    Scan.ScanName(key);
    Scan.Match('=');
    if (key==L"alias")
    {
      Scan.ScanString(info->Alias);
      Scan.Match(';');
    }
    else if (key==L"extensions")
    {
      Scan.Match('{');
      while (!Scan.Errors && !Scan.IfToken('}'))
      {
        if (Scan.Token==sTOK_STRING)
        {
          sPoolString ext;
          Scan.ScanString(ext);
          if (!sFind(info->Extensions,ext))
            info->Extensions.AddTail(ext);
        }
        Scan.IfToken(',');
      }
    }
  }

}

void Document::_License()
{
  sPoolString name;
  if(Scan.Token=='{')
  {
    CurrentLicense = _DefineLicense();
  }
  else
  {
    Scan.ScanName(name);
    if(name==L"default")
    {
      CurrentLicense = 0;
    }
    else
    {
      CurrentLicense = sFind(Licenses,&LicenseInfo::Name,name);
      if(CurrentLicense==0)
        Scan.Error(L"unknown license %s",name);
    }
  }
}

void Document::_Project(sBool projectKeyword)
{
  sPoolString cmd;

  if(!projectKeyword)
    CurrentLicense = 0;

  // project keyword found in file.. (not complete file mode)
  if (projectKeyword)
  {
    CreateNewProject();

    Scan.ScanString(Project->Name);

    if (!Scan.Errors)
    {
      Project->OutputFile = Solution->Path.Path(OmniPath::PT_SYSTEM);
      Project->OutputFile.Add(L"/");
      Project->OutputFile.Add(Project->Name);
      Project->OutputFile.Add(VS_ProjExtension);
      if(TestFlag)
        Project->OutputFile.Add(L".txt");
    }

    Scan.Match(L'{');
  }


  while(Scan.Errors==0 && Scan.Token!=sTOK_END && (!projectKeyword || Scan.Token!=L'}'))
  {
    Scan.ScanName(cmd);

    if(cmd==L"if")
    {
      sBool cond = _If();
      Scan.ScanName(cmd);

      if(cmd==L"folder")
      {
        _Folder(&Project->Files,cond);
      }
      else if(cmd==L"file")
      {
        _File(&Project->Files,cond);
      }
      else if(cmd==L"depend")
      {
        _Depend(cond);
      }
      else if(cmd==L"depend_extern")
      {
        _DependExtern(cond);
      }
      else if(cmd==L"create")
      {
        _Create(cond);
      }
      else if(cmd==L"include")
      {
        _Include(cond);
      }
    }
    else if(cmd==L"folder")
    {
      _Folder(&Project->Files);
    }
    else if(cmd==L"file")
    {
      _File(&Project->Files);
    }
    else if(cmd==L"guid")
    {
      _Guid();
    }
    else if(cmd==L"library")
    {
      Scan.Match(';');
      Project->ConfigurationType = 4;
    }
    else if(cmd==L"dllibrary")
    {
      Scan.Match(';');
      Project->ConfigurationType = 2;
    }
    else if(cmd==L"create_new_files")
    {
      Scan.Match(';');
      sPrintF(L"%s(%d): create_new_files is obsolete\n",Scan.GetFilename(),Scan.GetLine());
      sDPrintF(L"%s(%d): create_new_files is obsolete\n",Scan.GetFilename(),Scan.GetLine());
    }
    else if(cmd==L"depend")
    {
      _Depend();
    }
    else if(cmd==L"depend_extern")
    {
      _DependExtern();
    }
    else if(cmd==L"create")
    {
      _Create();
    }
    else if(cmd==L"include")
    {
      _Include();
    }
    else if(cmd==L"everything")
    {
      _Everything();
    }
    else if(cmd==L"project")
    {
      if (projectKeyword)
      {
        Scan.Error(L"error: project keyword in project",cmd);
      }
      else
      {
        _Project(sTRUE);
        Scan.Match(L'}');
      }
    }
    else if(cmd==L"license")
    {
      _License();
      Scan.Match(';');
    }
    else if(cmd==L"rule")
    {
      sPoolString rule;
      Scan.ScanString(rule);
      Scan.Match(';');
      Project->Rules.AddTail(rule);
    }
    else if(cmd==L"solutionconfig")
    {
      // parse it as if it was a normal file modifier
      sAutoArray<Config*> modConfig;
      _Config(modConfig,sTRUE);
      sVERIFY(modConfig.GetCount() == 1);

      // then add settings to all matching solution configurations
      Config *mod = modConfig[0];
      Config *sc;
      sFORALL(Solution->Configs,sc)
        if(mod->Predicate->Eval(sc->Name))
          sc->AddFrom(mod);
    }
    else
    {
      Scan.Error(L"syntax error at <%s>",cmd);
    }
  } // while

  if(CheckLicenseFlag && CurrentLicense==0 && !projectKeyword)
    sPrintF(L"%s: no project license defined\n",Project->Name);

  if (Scan.Errors==0 && projectKeyword)
  {
    // add dependency to subproject
    sString<2048> buffer;
    Depend *dep       = new Depend;

    dep->Name = Project->Name;
    dep->Path = Project->Path;
        
    dep->Path.Path_Relative.AddPath(Project->Name);
        
    dep->Guid = Project->Guid;
    dep->Project = sNULL;

    Solution->ProjectMain->Dependencies.AddTail(dep);
     
    // reset to main project
    Project = Solution->ProjectMain;
  }
}

/****************************************************************************/

void Document::MakeEverythingDependencies(ProjectData *eproject)
{
  // include all projects which contain the everythingpath
  ProjectData *proj;
  sFORALL(Projects,proj)
  {
    if (eproject!=proj)
    {
      sInt v = sFindString(proj->Path.Path_Relative, eproject->EverythingPath);

      if ((v==0 || (v==1 && (proj->Path.Path_Relative[0]=='/' || proj->Path.Path_Relative[0] =='\\'))) && proj->EverythingPath==L"")
      {
        // search if the dependency is already set
        Depend *dep;
        sBool depAlreadySet = sFALSE;
        
        sFORALL(eproject->Dependencies,dep)
        {
          depAlreadySet |= dep->Guid==proj->Guid;
        }

        if (!depAlreadySet)
        {
          sString<2048> buffer;

          dep       = new Depend;
          dep->Name = proj->Name;
          dep->Path = proj->Path;
          dep->Path.Path_Relative.AddPath(proj->Name);
          
          
          dep->Guid = proj->Guid;
          dep->Project = sNULL;
          eproject->Dependencies.AddTail(dep);
        }
      }
    }
  }
  
}

/****************************************************************************/

sBool Document::ParseGlobal(const sChar *filename)
{
  Scan.Init();
  Scan.DefaultTokens();
  Scan.StartFile(filename);

  _Global();

  if(ConfigFile.SDK & sSDK_CHAOS)
  {
    sString<1024> buffer;

    buffer = ConfigFile.CodeRoot_System;
    buffer.AddPath(L"chaos-code/makeproject.txt");       // this should be more configurable....
    Scan.Init();
    Scan.DefaultTokens();
    Scan.StartFile(buffer);

    _Global();
  }

  return Scan.Errors==0;
}

/****************************************************************************/

sBool Document::ScanForSolutions(OmniPath *path)
{
  sArray<sDirEntry> dir;
  sDirEntry *file;
  sLoadDir(dir,path->Path(OmniPath::PT_SYSTEM));
  sFORALL(dir,file)
  {
    if(file->Name[0]=='.')
      continue;

    //sDPrintF(L"%8d %s/'%s'\n",file->Size,path,file->Name);

    if(file->Flags & sDEF_DIR)
    {
      OmniPath newPath = *path;
      newPath.Path_Relative.AddPath(file->Name);
      if(!ScanForSolutions(&newPath))
        return 0;
    }

    sInt len = sGetStringLen(file->Name);
    if(len>7 && sCmpString(L".mp.txt",file->Name+len-7)==0)
    {
      Config *sc,*dc;
      Solution = new SolutionData;

      Solution->Name.Init(file->Name,sGetStringLen(file->Name)-7);

      Solution->Path = *path;

      Solution->InputFile = path->Path(OmniPath::PT_SYSTEM);
      Solution->InputFile.Add(L"/");
      Solution->InputFile.Add(file->Name);

      Solution->SolutionOutputFile = path->Path(OmniPath::PT_SYSTEM);
      Solution->SolutionOutputFile.Add(L"/");
      Solution->SolutionOutputFile.Add(Solution->Name);
      Solution->SolutionOutputFile.Add(L".sln");
      if(TestFlag)
        Solution->SolutionOutputFile.Add(L".txt");

      Solution->GNUMakeOutputfile  = path->Path(OmniPath::PT_SYSTEM);
      Solution->GNUMakeOutputfile.Add(L"/Makefile");
      if(TestFlag)
        Solution->GNUMakeOutputfile.Add(L".txt");

      
      sDeleteAll(Solution->Configs);
    
      Solution->ClearConfigBits();

      sFORALL(BaseConfigs,sc)
      {
        if(sc->BaseConfValid)
        {
          dc = new Config;
          dc->CopyFrom(sc);
          Solution->Configs.AddTail(dc);
        }
      }

      if(Solution->Configs.GetCount()<=Solution->ConfigMask.Size())
      {
        if(ParseSolution())
        {
          Solutions.AddTail(Solution);
          Solution = 0;
          Project  = 0;
        }
        else
        {
          delete Solution;
          Project  = 0;
          return 0;
        }
      }
      else
      {        
        sPrintF(L"%d configurations, which exceeds the internal limit (%d).\n",Solution->Configs.GetCount(),Solution->ConfigMask.Size());
        sPrint(L"please disable SDK's from the altona_config.hpp.\n");
      }
    }
  }
  return 1;
}

/****************************************************************************/

void Document::CreateNewProject()
{
  Project = new ProjectData;
  
  Projects.AddTail(Project);
  Solution->Projects.AddTail(Project);

  Project->Path               = Solution->Path;
  Project->ConfigurationType  = 1; 
  
  sDeleteAll(Project->Dependencies);
  Project->Files.Clear();
  Project->CycleDetect = sFALSE;
}

ProjectData *Document::DetectCyclesR(ProjectData *cur)
{
  static ProjectData * const DoneCycle = (ProjectData*) -1;

  if(cur->CycleDetect == 1) // found one!
  {
    sPrintF(L"Cycle in dependency graph: %s <%s>",cur->Name,cur->Path.Path(OmniPath::PT_SYSTEM));
    return cur;
  }

  if(cur->CycleDetect == 2) // already done
    return 0;

  cur->CycleDetect = 1;

  Depend *dep;
  sFORALL(cur->Dependencies,dep)
  {
    ProjectData *cycle = DetectCyclesR(dep->Project);
    if(cycle)
    {
      if(cycle != DoneCycle)
      {
        sPrintF(L" <- %s <%s>",cur->Name,cur->Path.Path(OmniPath::PT_SYSTEM));
        if(cycle == cur)
        {
          sPrintF(L"\n");
          cycle = DoneCycle; // just to make sure no further output occurs
        }
      }

      return cycle;
    }
  }

  cur->CycleDetect = 2;
  return 0;
}

void Document::WriteDependencyGraph(SolutionData *s)
{
  sString<sMAXPATH> filename = s->Path.Path(OmniPath::PT_SYSTEM);
  filename.Add(L"/");
  filename.Add(s->Name);
  filename.Add(L"_deps.dot");

  ProjectData *proj;
  sFORALL(Projects,proj)
    proj->CycleDetect = 0;

  sTextFileWriter wr(filename);
  wr.PrintF(L"digraph %s {\n",s->Name);
  wr.PrintF(L"  ratio=auto;\n");
  wr.PrintF(L"  rankdir=LR;\n");
  wr.PrintF(L"  node [ shape=box,fontsize=10,fontname=Sans ];\n");

  sFORALL(s->Projects,proj)
    WriteDepsForProjectR(wr,proj);

  wr.PrintF(L"}\n");
}

void Document::WriteDepsForProjectR(sTextFileWriter &wr,ProjectData *proj)
{
  if(proj->CycleDetect)
    return;

  proj->CycleDetect = 1;

  wr.PrintF(L"  subgraph cluster_%s {\n",proj->Name);
  wr.PrintF(L"    label = %q;\n",proj->Name);
  wr.PrintF(L"    color = blue;\n");

  WriteDepsForProjectFolderR(wr,proj,&proj->Files,sFALSE);

  wr.PrintF(L"  }\n");

  WriteDepsForProjectFolderR(wr,proj,&proj->Files,sTRUE);

  Depend *dep;
  sFORALL(proj->Dependencies,dep)
    WriteDepsForProjectR(wr,dep->Project);
}

void Document::WriteDepsForProjectFolderR(sTextFileWriter &wr,ProjectData *proj,ProjFolder *folder,sBool edges)
{
  File *f;

  sFORALL(folder->Files,f)
  {
    WriteDepsForFile(wr,proj,f,edges);
  }

  ProjFolder *fold;
  sFORALL(folder->Folders,fold)
    WriteDepsForProjectFolderR(wr,proj,fold,edges);
}

void Document::WriteDepsForFile(sTextFileWriter &wr,ProjectData *proj,File *file,sBool edges)
{
  sString<sMAXPATH> filename;

  // always write filename
  if(!edges)
  {
    filename = proj->Path.Path_Relative;
    filename.AddPath(file->Name);
    sReplaceChar(filename,'\\','/');

    wr.PrintF(L"    f%x [ label=%q ];\n",(sPtr)file,filename);
    return;
  }

  // is it a supported extension? if yes, also trace dependencies.
  static const sChar *extensions[] = { L"c",L"h",L"cpp",L"hpp",L"asc",L"ops" };
  sInt ext;
  sBool isHeader = sFALSE;

  for(ext=0;ext<sCOUNTOF(extensions);ext++)
  {
    if(sCheckFileExtension(file->Name,extensions[ext]))
    {
      if(extensions[ext][0] == 'h')
        isHeader = sTRUE;
      break;
    }
  }

  if(ext == sCOUNTOF(extensions)) // extension not found
    return;

  wr.PrintF(L"    f%x",(sPtr) file);

  // try loading the file to scan for includes
  filename = proj->Path.Path(OmniPath::PT_SYSTEM);
  filename.AddPath(file->Name);

  sBool foundDeps = sFALSE;

  sChar *text = sLoadText(filename);
  if(text)
  {
    // search for pattern "#include" in text
    // this is effectively the knuth-morris-pratt string search algorithm,
    // greatly simplified by the fact that no character occurs twice
    // in the pattern.
    const sChar pattern[] = L"#include";
    sInt i=0,j=0;

    while(text[i])
    {
      if(text[i++] != pattern[j]) // no match
        j = 0; // start at beginning
      else // match
      {
        j++;
        if(!pattern[j]) // complete match!
        {
          // skip whitespace
          while(sIsSpace(text[i]))
            i++;

          filename[0] = 0;

          if(text[i] == '<') // #include <blah>
          {
            i++;
            sInt outPos = 0;

            while(text[i] && text[i] != '>')
            {
              if(outPos < sMAXPATH-1) filename[outPos++] = text[i];
              i++;
            }

            filename[outPos++] = 0;
          }
          else if(text[i] == '"') // #include "blah"
          {
            i++;
            sInt outPos = 0;

            while(text[i] && text[i] != '"')
            {
              if(outPos < sMAXPATH-1) filename[outPos++] = text[i];
              i++;
            }

            filename[outPos++] = 0;
          }

          // it's an #include directive?
          if(filename[0])
          {
            File *incFile = FindMatchingFile(filename,proj,!isHeader);
            if(incFile) // yes, and we know the file
            {
              if(!foundDeps)
              {
                wr.Print(L" -> { ");
                foundDeps = sTRUE;
              }

              wr.PrintF(L"f%x ",(sPtr)incFile);
            }
          }
        }
      }
    }

    delete[] text;
  }

  wr.Print(foundDeps ? L"};\n" : L";\n");
}

File *Document::FindMatchingFile(const sChar *name,ProjectData *proj,sBool onlyThisProject)
{
  File *f;

  // try to find it in this project
  f = FindFileR(name,&proj->Files);
  if(f)
    return f;

  // then try to find it in include paths
  for(sInt i=0;i<proj->Includes.GetCount();i++)
  {
    sString<sMAXPATH> includePath = proj->Includes[i].Path_Relative;
    includePath.AddPath(name);

    ProjectData *proj2;
    sFORALL(Projects,proj2)
    {
      if(onlyThisProject && proj2 != proj)
        continue;

      const sChar *pr = proj2->Path.Path_Relative;

      sInt len = sGetStringLen(pr);
      if(sCmpStringPLen(includePath,pr,len) == 0) // match
      {
        if(includePath[len] == '/' || includePath[len] == '\\')
          len++;

        f = FindFileR(includePath+len,&proj2->Files);
        if(f)
          return f;
      }
    }
  }

  return 0;
}

File *Document::FindFileR(const sChar *name,ProjFolder *folder)
{
  File *f;
  sFORALL(folder->Files,f)
  {
    if(sCmpStringP(name,f->Name) == 0)
      return f;
  }

  ProjFolder *fold;
  sFORALL(folder->Folders,fold)
  {
    if((f = FindFileR(name,fold)) != 0)
      return f;
  }

  return 0;
}

/****************************************************************************/

sBool Document::ParseSolution()
{
  if(!BriefFlag)
  {
    sPrintF(L"%s\n",Solution->InputFile);
  }

  Scan.Init();
  Scan.DefaultTokens();
  Scan.StartFile(Solution->InputFile);


  CreateNewProject();

  Project->OutputFile = Solution->Path.Path(OmniPath::PT_SYSTEM);
  Project->OutputFile.Add(L"/");
  Project->OutputFile.Add(Solution->Name);
  Project->OutputFile.Add(VS_ProjExtension);
  if(TestFlag)
    Project->OutputFile.Add(L".txt");

  Project->Name = Solution->Name;
  Solution->ProjectMain = Project;

  _Project(sFALSE);

  if(Project->Guid.IsEmpty())
    Scan.Error(L"no Guid");


  return Scan.Errors==0;
}

void Document::WriteProjectsAndSolutions()
{
  sBool error;

  Depend *dep;
  ProjectData *proj;

  error = 0;
  sFORALL(Projects,Project)
  {
    sInt ind = _i;

    // create dependencies for "everything"-projects
    if (Project->EverythingPath!=L"")
    {
      MakeEverythingDependencies(Project);
    }

    // solve dependencies
    sFORALL(Project->Dependencies,dep)
    {
      dep->Project = sFind(Projects,&ProjectData::Name,dep->Name);
      if(!dep->Project)
      {
        sPrintF(L"in project <%s>, could not find dependency <%s>\n",Project->Name,dep->Name);
        error = 1;
      }
    }
    // check for double GUIDs
    for(sInt j=ind+1;j<Projects.GetCount();j++)
    {
      proj = Projects[j];
      if(Project->Guid==proj->Guid)
      {
        sPrintF(L"projects %s <%s> and %s <%s> have same GUID %s\n",Project->Name,Project->Path.Path(OmniPath::PT_SYSTEM),proj->Name,proj->Path.Path(OmniPath::PT_SYSTEM),proj->Guid);
        error = 1;
      }
    }

    // prepare for cycle detection
    Project->CycleDetect = 0;
  }

  // detect cycles
  sFORALL(Projects,Project)
  {
    if(!Project->CycleDetect && DetectCyclesR(Project))
    {
      error = sTRUE;
      break;
    }
  }

  // and clear cycle detect flags again
  sFORALL(Projects,Project)
    Project->CycleDetect = 0;

  if(!error)
  {
    if(ConfigFile.Makefile == sMAKE_LINUX || ConfigFile.Makefile == sMAKE_MINGW)
      OutputPrepareMakefile();
    
    sFORALL(Solutions,Solution)
    {
      TargetLinux = ConfigFile.Makefile == sMAKE_LINUX;
      
      OutputSolution();
      if(ConfigFile.Makefile == sMAKE_LINUX || ConfigFile.Makefile == sMAKE_MINGW)
        OutputSolutionMakefile();

      sFORALL(Solution->Projects,Project)
      {
        TargetLinux = ConfigFile.Makefile == sMAKE_LINUX;
        
        if (VS_Version>=10)
          OutputXProject();
        else
          OutputProject();
        if(ConfigFile.Makefile == sMAKE_LINUX || ConfigFile.Makefile == sMAKE_MINGW)
          OutputProjectMakefile();
        DoCreateNewFiles(&Project->Files);
        if(CheckLicenseFlag)
          DoUpdateLicense(&Project->Files);
      }
    }
  }
  else
    sPrintF(L"\nThere were errors.\n");
}

void Document::WriteDependencyGraphs(const sChar *pattern)
{
  SolutionData *s;
  sFORALL(Solutions,s)
    if(sMatchWildcard(pattern,s->Name))
      WriteDependencyGraph(s);
}

void Document::Cleanup()
{
  ProjectData *Project;

  sFORALL(Projects,Project)
  {
    Project->Files.Clear();
  }
}

sInt Document::GetConfigCount()
{
  sInt n=0;
  Config *c;
  sFORALL(BaseConfigs,c)
    if(c->BaseConfValid)
      n++;
  return n;
}

/****************************************************************************/

const sChar *OmniPath::Path( OmniPath::PathType p )
{
  TempPath = RootPath[p];
  TempPath.AddPath(Path_Relative);

  return TempPath;
}

/****************************************************************************/
