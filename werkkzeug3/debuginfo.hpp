// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __DEBUGINFO_HPP__
#define __DEBUGINFO_HPP__

#include "_types.hpp"
#include "packer.hpp"

/****************************************************************************/

#define DIC_END     0
#define DIC_CODE    1
#define DIC_DATA    2
#define DIC_UNKNOWN 3

union DIString
{
  sInt Index;
  sChar *String;
};

struct DISymFile
{
  DIString Name;
  sF32 PackedSize;
  sU32 Size;
};

struct DISymNameSp // Namespace
{
  DIString Name;
  sF32 PackedSize;
  sU32 Size;
};

struct DISymbol
{
  DIString Name;
  DIString MangledName;
  sInt NameSpNum;
  sInt FileNum;
  sU32 VA;
  sU32 Size;
  sInt Class;
  sF64 PackedSize;
};

class DebugInfo
{
  sArray<sInt> Strings;
  sArray<sChar> StringData;
  sU32 Address;
  sU32 BaseAddress;
  class ReorderBuffer *Reorder;

public:
  sArray<DISymbol> Symbols;
  sArray<DISymFile> Files;
  sArray<DISymNameSp> NameSps;

  void Init();
  void Exit();

  // only use those before reading is finished!!
  sInt MakeString(sChar *string);
  sChar* GetStringPrep(sInt index)          { return &StringData[index]; }
  void SetBaseAddress(sU32 base)            { BaseAddress = base; }

  void FinishedReading();

  void Rebase(sU32 newBase);
  
  sInt GetFile(sInt name);
  sInt GetFileByName(sChar *name);

  sInt GetNameSpace(sInt name);
  sInt GetNameSpaceByName(sChar *name);
  
  void StartAnalyze(sU32 startAddress,class ReorderBuffer *reorder=0);
  static void TokenizeCallback(void *user,sInt uncompSize,sF32 compSize);
  void FinishAnalyze();
  sBool FindSymbol(sU32 VA,DISymbol **sym);

  sChar *WriteReport();
};

class DebugInfoReader
{
public:
  virtual sBool ReadDebugInfo(sChar *fileName,DebugInfo &to) = 0;
};

struct ReorderItem
{
  sU32 NewVA,NewSize;
  sU32 OldVA,OldSize;
};

class ReorderBuffer
{
public:
  ReorderBuffer();
  ~ReorderBuffer();

  sArray<ReorderItem> Reorder;

  void Add(sU32 newVA,sU32 newSize,sU32 oldVA,sU32 oldSize);
  void Finish();
  sBool Find(sU32 addr,ReorderItem **reord);
};

/****************************************************************************/

#endif
