// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __EXEPACKER_HPP_
#define __EXEPACKER_HPP_

#include "_types.hpp"
#include "packer.hpp"

/****************************************************************************/

struct KKBlob
{
  void *Data;
  sU32 Size;
};

class KKBlobList
{
public:
  KKBlobList();
  ~KKBlobList();

  void AddBlob(void *data,sU32 size);

  sArray<KKBlob> Blobs;
};

class CodeFragment
{
  sU8 *Code;
  sU8 *OriginalCode;
  sInt CodeSize;

public:
  CodeFragment();
  ~CodeFragment();

  void Init(sPtr code,sInt codeSize);

  sBool Patch(sChar *pattern,sU8 *patch,sInt size);
  sBool PatchDWord(sChar *pattern,sU32 patch);
  sBool PatchJump(sChar *pattern,sU32 offset,sU32 base);
  sBool PatchOffsets(sU32 base);

  sU8 *GetCode()        { return Code; }
};

class EXEPacker
{
  sChar Error[512];
  sChar Warnings[4096];
  sChar *WarningsPtr;

  sU8 *Source;
  struct PEHeader *PH,*OPH;
  struct PESection *Section;
  sU32 *DataDir;
  
  sU8 *OutImage;
  sU32 OutLimit,OutSize;

  sU8 *Imports;
  sU32 ImportSize,ImportMax;

  sBool ResourcesPresent;
  sArray<sU8> Resources;
  sArray<sU8> ResourceStrings;
  sArray<sU8> ResourceData;

  /*sU8 *Resources;
  sU32 ResourceSize;*/

  sU8 *UnImage,*UnImageVA;
  CodeFragment Depacker;

  sU32 ActualSize;

  sU32 Align(sU32 size,sU32 align);
  sPtr AppendImage(sPtr data,sU32 size);
  void Padding(sU32 size);
  void AlignmentPadding(sU32 align);
  sU32 SourceOffset(sU32 RVA);

  void ResizeImports();
  sBool ProcessImports();

  void CleanupUnImage();
  
  sBool ProcessResources();
  sU32 ProcessResourcesR(sU8 *start,sU32 offs,sInt level,sBool compress);

  void FinishResources(sU32 *dataDir);
  void AdjustResourcesR(sU8 *start,sU32 offs,sU32 stringrel,sU32 datarel);

public:
  sU8 *Pack(class PackerFrontEnd *pfe,sU8 *source,sInt &outSize,class DebugInfo *info,PackerCallback cb,KKBlobList *blobs);
  sChar *GetErrorMessage();
  sChar *GetWarningMessages();
  sU32 GetActualSize();
};

/****************************************************************************/

#endif
