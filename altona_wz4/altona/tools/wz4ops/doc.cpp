/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "doc.hpp"

Document *Doc;

/****************************************************************************/

Document::Document()
{
}

Document::~Document()
{
  sDeleteAll(HCodes);
  sDeleteAll(HEndCodes);
  sDeleteAll(CCodes);
  sDeleteAll(Types);
  sDeleteAll(Ops);
}

void Document::SetNames(const sChar *name)
{
  InputFileName = name;

  if(name[0]=='.' && name[1]=='\\')
    name+=2;

  ProjectName = name;
  sChar *ext = sFindFileExtension(ProjectName);
  if(ext && ext[0])
    ext[-1] = 0;
  else
    InputFileName.Add(L".ops");

  sSPrintF(CPPFileName,L"%s.cpp",ProjectName);
  sSPrintF(HPPFileName,L"%s.hpp",ProjectName);
}

/****************************************************************************/

CodeBlock::CodeBlock(sPoolString code,sInt line)
{
  Code = code;
  Line = line;
}

/****************************************************************************/

External::External()
{
  Code = 0;
  Line = 0;
}

External::~External()
{
  delete Code;
}

/****************************************************************************/

Importer::Importer()
{
}

/****************************************************************************/

Type::Type()
{
  Parent = L"";
  Symbol = L"";
  Label = L"";
  Color = 0;
  Flags = 0;
  Code = 0;
  Header = 0;
  Virtual = 0;
  GuiSets = 0;
}

Type::~Type()
{
  delete Code;
  delete Header;
  sDeleteAll(Externals);
}

/****************************************************************************/

Parameter::Parameter()
{
  Type = 0;
  Offset = 0;
  Label = L"";
  Symbol = L"";
  Min = 0;
  Max = 255;
  Step = 0.125f;
  RStep = 0.125f;
  sClear(DefaultU);
  DefaultString = L"";
  Options = L"";
  Count = 1;
  XYZW = 0;
  LinkMethod = 0;
  DefaultStringRandom = 0;
  Condition = 0;
  LayoutFlag = 0;
  ContinueFlag = 0;
  FilterMode = -1;
  Format = L"";
  GridLines = 1;
  Flags = 0;
/*
  OverBoxFlag = 0;
  AnimFlag = 0;
  OverLabelFlag = 0;
  LineNumberFlag = 0;
  NarrowFlag = 0;
  */
}

sPoolString Parameter::ScriptName()
{
  sString<256> buffer;
  if(!Label.IsEmpty() && sIsName(Label)) buffer = Label;
  else buffer = Symbol;
  sMakeLower(buffer);

  if(buffer==L"int") buffer = L"int_";
  if(buffer==L"float") buffer = L"float_";
  if(buffer==L"color") buffer = L"color_";
  if(buffer==L"string") buffer = L"string_";
  // may be add more keywords...

  return sPoolString(buffer);
}


Input::Input()
{
  Type = L"";
  DefaultOpName = L"";
  DefaultOpType = L"";
  InputFlags = 0;
  Method = IE_INPUT;
}

Op::Op()
{
  Name = L"";
  OutputType = L"";
  TabType = L"";
  Code = 0;
  Flags = 0;
  HideArray = 0;
  ArrayNumbers = 0;
  GroupArray = 0;
  GridColumns = 0;
  DefaultArrayMode = 0;
  Shortcut = 0;
  Column = 0;
  MaxOffset = 0;
  MaxStrings = 0;
  MaxArrayOffset = 0;
  FileInMask = 0;
  FileOutMask = 0;
  Helper = 0;
  Handles = 0;
  SetDefaultsArray = 0;
  Actions = 0;
  SpecialDrag = 0;
  Description = 0;
}

Op::~Op()
{
  sDeleteAll(Ties);
  sDeleteAll(ArrayTies);
  sDeleteAll(Parameters);
  sDeleteAll(ArrayParam);
  sDeleteAll(Inputs);
  delete Helper;
  delete Code;
  delete Handles;
  delete SetDefaultsArray;
  delete Actions;
  delete SpecialDrag;
  delete Description;
}

/****************************************************************************/

