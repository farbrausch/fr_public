// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_types.hpp"
#include "exepacker.hpp"
#include "packer.hpp"
#include "debuginfo.hpp"
#include "dis.hpp"

/****************************************************************************/

extern "C" sU8 depacker[],depacker2[];
extern "C" sU32 depackerSize,depacker2Size;
//static const sU32 depackerWorkSize = 2048;
static const sU32 depackerWorkSize = 4096+20;

/****************************************************************************/

struct PEHeader
{
  sChar Magic[4];
  sU16 CPU;
  sU16 Sections;
  sU8 _[12];
  sU16 OptHeaderSize;
  sU16 Flags;
  sU8 __[4];
  sU32 CodeSize;
  sU32 DataSize;
  sU32 BSSSize;
  sU32 Entry;
  sU32 CodeStart;
  sU32 DataStart;
  sU32 ImageStart;
  sU32 ObjectAlign;
  sU32 FileAlign;
  sU8 ___[16];
  sU32 ImageSize;
  sU32 HeaderSize;
  sU32 CheckSum;
  sU16 SubSystem;
  sU16 DllFlags;
  sU8 ____[20];
  sU32 DataDirs;
};

struct PESection
{
  sChar Name[8];
  sU32 VirtSize;
  sU32 VirtAddr;
  sU32 Size;
  sU32 DataPtr;
  sU8 _[12];
  sU32 Flags;
};

struct RSRCDirEntry
{
  sU32 NameID;
  sU32 Offset;
};

struct RSRCDirectory
{
  sU32 Flags;
  sU8 _[8];
  sU16 NameEntries;
  sU16 IDEntries;
};

struct RSRCDataEntry
{
  sU32 RVA;
  sU32 Size;
  sU32 Codepage;
  sU32 Reserved;
};

/****************************************************************************/

KKBlobList::KKBlobList()
{
  Blobs.Init();
}

KKBlobList::~KKBlobList()
{
  Blobs.Exit();
}

void KKBlobList::AddBlob(void *data,sU32 size)
{
  KKBlob *blob;

  blob = Blobs.Add();
  blob->Data = data;
  blob->Size = size;
}

/****************************************************************************/

CodeFragment::CodeFragment()
{
  Code = 0;
  OriginalCode = 0;
  CodeSize = 0;
}

CodeFragment::~CodeFragment()
{
  delete[] OriginalCode;
  OriginalCode = 0;
}

void CodeFragment::Init(sPtr code,sInt codeSize)
{
  if(OriginalCode)
    delete[] OriginalCode;

  OriginalCode = new sU8[codeSize];
  Code = (sU8 *) code;
  CodeSize = codeSize;
  sCopyMem(OriginalCode,Code,CodeSize);
}

sBool CodeFragment::Patch(sChar *pattern,sU8 *patch,sInt size)
{
  sInt i,pos,len;

  len = sGetStringLen(pattern);
  pos = -1;
  for(i=0;i<CodeSize-len+1;i++)
  {
    if(!sCmpMem(OriginalCode+i,pattern,len))
    {
      if(pos == -1)
        pos = i;
      else // not unique!
        return sFALSE;
    }
  }

  if(pos != -1)
  {
    sCopyMem(Code+pos,patch,size);
    return sTRUE;
  }
  else
    return sFALSE;
}

sBool CodeFragment::PatchDWord(sChar *pattern,sU32 patch)
{
  return Patch(pattern,(sU8 *) &patch,sizeof(sU32));
}

sBool CodeFragment::PatchJump(sChar *pattern,sU32 offset,sU32 base)
{
  sInt i;
  sU32 *pt;
  sBool match;

  match = sFALSE;
  for(i=0;i<CodeSize-3;i++)
  {
    if(!sCmpMem(OriginalCode + i,pattern,4))
    {
      if(match)
        return sFALSE;

      match = sTRUE;
      pt = (sU32 *) (Code + i);
      *pt = offset - 4 - i - base;
    }
  }

  return match;
}

sBool CodeFragment::PatchOffsets(sU32 base)
{
  sInt i;
  sU32 *pt;

  for(i=0;i<CodeSize-3;i++)
  {
    pt = (sU32 *) (Code + i);
    if(OriginalCode[i+2] == 'P' && OriginalCode[i+3] == 'R')
      *pt = (OriginalCode[i+0]+OriginalCode[i+1]*256) + base;
  }

  return sTRUE;
}

/****************************************************************************/

sU32 EXEPacker::Align(sU32 size,sU32 align)
{
  return size + (align-1) & -(sInt) align;
}

sPtr EXEPacker::AppendImage(sPtr data,sU32 size)
{
  sCopyMem(OutImage+OutSize,data,size);
  OutSize += size;
  sVERIFY(OutSize < OutLimit);

  return OutImage+OutSize-size;
}

void EXEPacker::Padding(sU32 size)
{
  sSetMem(OutImage+OutSize,0,size);
  OutSize += size;
  sVERIFY(OutSize < OutLimit);
}

void EXEPacker::AlignmentPadding(sU32 align)
{
  Padding(Align(OutSize,align)-OutSize);
}

sU32 EXEPacker::SourceOffset(sU32 RVA)
{
  sInt i;
  sU32 relOffs;

  for(i=0;i<PH->Sections;i++)
  {
    relOffs = RVA - Section[i].VirtAddr;
    if(RVA>=Section[i].VirtAddr && relOffs<sMax(Section[i].VirtSize,Section[i].Size))
      return Section[i].DataPtr + relOffs;
  }

  sVERIFYFALSE;
  return 0;
}

void EXEPacker::ResizeImports()
{
  sU8 *ni;

  if(ImportSize+512>=ImportMax)
  {
    sVERIFY(ImportSize<ImportMax);
    ni = new sU8[ImportMax *= 2];
    sCopyMem(ni,Imports,ImportSize);
    delete[] Imports;
    Imports = ni;
  }
}

sBool EXEPacker::ProcessImports()
{
  sU32 i,j;
  sU32 *src,*imp,*iat;
  sU8 *str;

  ImportSize = 0;
  ImportMax = 1024;
  Imports = new sU8[ImportMax];
  Imports[ImportSize++] = 0; // depacker skips first byte

  if(DataDir[3])
  {
    src = (sU32 *) (Source + SourceOffset(DataDir[2]));

    for(i=0;src[i*5+4];i++)
    {
      ResizeImports();

      // copy iat offset
      *(sU32 *) (Imports+ImportSize) = src[i*5+4]+PH->ImageStart; ImportSize += 4;

      // copy dll name
      str = Source + SourceOffset(src[i*5+3]);
      while(*str)
        Imports[ImportSize++] = *str++;

      Imports[ImportSize++] = 0;

      // dll imports
      iat = (sU32 *) (Source + SourceOffset(src[i*5+4]));
      if(src[i*5+0])
        imp = (sU32 *) (Source + SourceOffset(src[i*5+0]));
      else
        imp = iat;

      for(j=0;imp[j];j++)
      {
        ResizeImports();

        if(imp[j] & 0x80000000)
        {
          if((imp[j] & 0x7fffffff) > 0xffff)
          {
            sSPrintF(Error,512,"invalid import ordinal");
            return sFALSE;
          }

          Imports[ImportSize++] = 0xff;
          Imports[ImportSize++] = imp[j] & 0xff;
          Imports[ImportSize++] = (imp[j] >> 8) & 0xff;
          Imports[ImportSize++] = 0;
        }
        else
        {
          str = Source + SourceOffset(imp[j]) + 2;

          while(*str)
          {
            if(*str >= 128)
            {
              sSPrintF(Error,512,"non-ascii character in imported name");
              return sFALSE;
            }

            Imports[ImportSize++] = *str++;
          }

          Imports[ImportSize++] = 0;
        }
      }

      Imports[ImportSize++] = 0;
    }
  }

  *(sU32 *) (Imports+ImportSize) = 0; ImportSize += 4;

  return sTRUE;
}

void EXEPacker::CleanupUnImage()
{
  sU32 i,j,*pt;

  // clear debug info
  if(DataDir[13])
  {
    for(i=0;i<DataDir[13];i+=28)
    {
      pt = (sU32 *) (UnImageVA+DataDir[12]+i);
      sSetMem(UnImageVA+pt[5],0,pt[4]);
    }
  }

  // clear import names
  if(DataDir[3])
  {
    sU32 *src = (sU32 *) (UnImageVA + DataDir[2]);
    sU8 *str;

    for(i=0;src[i*5+4];i++)
    {
      // clear dll name
      str = UnImageVA + src[i*5+3];
      while(*str)
        *str++ = 0;

      // imports
      sU32 *imp,*iat;
      iat = (sU32 *) (UnImageVA + src[i*5+4]);
      imp = src[i*5+0] ? ((sU32 *) (UnImageVA + src[i*5+0])) : iat;

      for(j=0;imp[j];j++)
      {
        if(!(imp[j] & 0x80000000)) // import by name
        {
          str = UnImageVA + imp[j];
          
          // overwrite hint with zero
          *str++ = 0;
          *str++ = 0;

          while(*str)
            *str++ = 0;
        }

        imp[j] = iat[j] = 0;
      }
    }
  }

  // zero out all data referred to in the data directory, except resources
  for(i=0;i<PH->DataDirs;i++)
  {
    if(i != 2 && DataDir[i*2+1])
      sSetMem(UnImageVA+DataDir[i*2+0],0,DataDir[i*2+1]);
  }
}

sBool EXEPacker::ProcessResources()
{
  if(ResourcesPresent)
  {
    // this should *always* be 0
    sU32 offs = ProcessResourcesR(Source + SourceOffset(DataDir[4]),0,0,sTRUE);
    return offs == 0;
  }
  else
    return sTRUE;
}

sU32 EXEPacker::ProcessResourcesR(sU8 *start,sU32 offs,sInt level,sBool compress)
{
  RSRCDirectory *dir;
  RSRCDataEntry *data;
  RSRCDirEntry *entry,*targetEntry;
  sU16 *str;
  sU32 destOffs,dataOffs,size;
  sInt i;
  sBool compFlag;
  
  // this function is called only for resource directories.
  dir = (RSRCDirectory *) (start + offs);

  // first, copy this resource directory into the output buffer
  destOffs = Resources.Count;
  size = sizeof(RSRCDirectory) + (dir->IDEntries + dir->NameEntries) * sizeof(RSRCDirEntry);
  Resources.AtLeast(destOffs + size);
  sCopyMem(&Resources[destOffs],dir,size);
  Resources.Count += size;

  // clear resource directory in unpacked image
  sSetMem(UnImageVA + DataDir[4] + offs,0,size);

  // then process each child node in turn
  entry = (RSRCDirEntry *) (start + offs + sizeof(RSRCDirectory));
  for(i=0;i<dir->NameEntries + dir->IDEntries;i++,entry++)
  {
    // decide whether to compress child nodes or not
    compFlag = compress;

    if(level == 0) // type level
    {
      if(entry->NameID == 3 || entry->NameID == 14) // compress everything but icons
        compFlag = sFALSE;
    }

    // process names
    if(entry->NameID & 0x80000000) // is name
    {
      // copy string
      dataOffs = ResourceStrings.Count;
      str = (sU16 *) (start + (entry->NameID & 0x7fffffff));
      size = (*str + 1) * 2;

      ResourceStrings.AtLeast(dataOffs + size);
      ResourceStrings.Count += size;
      sCopyMem(&ResourceStrings[dataOffs],str,size);

      // clear string in unpacked image
      sSetMem(UnImageVA + DataDir[4] + (entry->NameID & 0x7fffffff),0,size);

      // patch new offset into resource structure
      // (relative to strings, we adjust all this in a second pass)
      targetEntry = (RSRCDirEntry *) &Resources[destOffs + sizeof(RSRCDirectory)];
      targetEntry[i].NameID = dataOffs | 0x80000000;
    }

    if(entry->Offset & 0x80000000) // subdirectory
    {
      dataOffs = ProcessResourcesR(start,entry->Offset & 0x7fffffff,level + 1,compFlag);
      
      // patch new offset into copied resource structure
      targetEntry = (RSRCDirEntry *) &Resources[destOffs + sizeof(RSRCDirectory)];
      targetEntry[i].Offset = dataOffs | 0x80000000;
    }
    else // child is a leaf
    {
      // copy the data entry, then get a pointer to it
      dataOffs = Resources.Count;
      Resources.AtLeast(dataOffs + sizeof(RSRCDataEntry));
      sCopyMem(&Resources[dataOffs],start + entry->Offset,sizeof(RSRCDataEntry));
      Resources.Count += sizeof(RSRCDataEntry);
      data = (RSRCDataEntry *) &Resources[dataOffs];

      // adjust entry position
      targetEntry = (RSRCDirEntry *) &Resources[destOffs + sizeof(RSRCDirectory)];
      targetEntry[i].Offset = dataOffs;

      if(compress) // this will be compressed, so leave it where it is
      {
        // but all relative addresses will change, so make the address
        // absolute, then we can fix it later
        data->RVA += PH->ImageStart;
        data->Reserved = 1; // we use that field for our own sinister purposes here
      }
      else // this won't be compressed, move it to the unpacked data range
      {
        dataOffs = ResourceData.Count;

        // copy actual data
        ResourceData.AtLeast(dataOffs + ((data->Size + 3) & ~3));
        ResourceData.Count += (data->Size + 3) & ~3;
        sCopyMem(&ResourceData[dataOffs],Source + SourceOffset(data->RVA),data->Size);

        // clear data in unpacked image
        sSetMem(UnImageVA + data->RVA,0,data->Size);

        // update structures
        data->RVA = dataOffs; // relative to beginning of resource data here
        data->Reserved = 0;
      }
    }
  }

  return destOffs;
}

void EXEPacker::FinishResources(sU32 *dataDir)
{
  sU32 size,stringpos,datarva;
  sU8 *resdata;

  if(ResourcesPresent)
  {
    // calc size
    size = (Resources.Count + 3) & ~3;
    stringpos = size;
    size += (ResourceStrings.Count + 3) & ~3;
    size += (ResourceData.Count + 3) & ~3;

    // append data in turn and update data directory
    AlignmentPadding(4);

    dataDir[4] = OutSize;
    dataDir[5] = size;
    resdata = (sU8 *) AppendImage(&Resources[0],Resources.Count);
    AlignmentPadding(4);
    AppendImage(&ResourceStrings[0],ResourceStrings.Count);
    AlignmentPadding(4);
    datarva = OutSize;
    AppendImage(&ResourceData[0],ResourceData.Count);
    AlignmentPadding(4);

    // then adjust addresses
    AdjustResourcesR(resdata,0,stringpos,datarva);
  }
}

void EXEPacker::AdjustResourcesR(sU8 *start,sU32 offs,sU32 stringrel,sU32 datarel)
{
  RSRCDirectory *dir;
  RSRCDirEntry *entry;
  RSRCDataEntry *data;
  sInt i;

  // iterate through the entries...
  dir = (RSRCDirectory *) (start + offs);
  entry = (RSRCDirEntry *) (start + offs + sizeof(RSRCDirectory));
  for(i=0;i<dir->NameEntries + dir->IDEntries;i++,entry++)
  {
    if(entry->NameID & 0x80000000) // it's a name, adjust to point to strings
      entry->NameID += stringrel;

    if(entry->Offset & 0x80000000) // it's a subdirectory, recurse
      AdjustResourcesR(start,entry->Offset & 0x7fffffff,stringrel,datarel);
    else // leaf
    {
      data = (RSRCDataEntry *) (start + entry->Offset);

      if(data->Reserved)
      {
        data->RVA -= OPH->ImageStart; // make address RVA again
        data->Reserved = 0;
      }
      else
        data->RVA += datarel;
    }
  }
}

/****************************************************************************/

sU8 *EXEPacker::Pack(PackerFrontEnd *pfe,sU8 *source,sInt &outSize,DebugInfo *info,PackerCallback cb,KKBlobList *blobs)
{
  PESection *outSection;
  sU32 *outDir,imageEnd,dataStart,dataSize,*p,iltOffs,depSize,skip;
  sU32 depack2Base;
  sU8 *buffer;
  sInt i,j,inSize,ptrTableO;
  DisFilter filter;
  CodeFragment depack2;
  ReorderBuffer reorder;

  Error[0] = 0;
  ActualSize = 0;
  Source = source;
  WarningsPtr = Warnings;
  Resources.Init();
  ResourceStrings.Init();
  ResourceData.Init();

  // verify the file is actually a packable executable
  if(sCmpMem(source,"MZ",2))
  {
    sSPrintF(Error,sizeof(Error),"file is not executable");
    return 0;
  }

  PH = (PEHeader *) (source + *(sU32 *) (source + 0x3c));
  if(sCmpMem(PH->Magic,"PE\x00\x00",4))
  {
    sSPrintF(Error,sizeof(Error),"file is not a portable executable");
    return 0;
  }

  // setup some useful pointers
  DataDir = (sU32 *) &PH[1];
  Section = (PESection *) (DataDir + PH->DataDirs*2);

  // perform additional checks to reject files with features we don't support
  if(DataDir[1] || PH->DataDirs>=10 && DataDir[19])
  {
    sSPrintF(Error,sizeof(Error),"files with exports or tls data are not supported");
    return 0;
  }

  // allocate output image
  inSize = PH->HeaderSize;
  for(i=0;i<PH->Sections;i++)
    inSize += Align(Section[i].Size,PH->FileAlign);

  if(blobs)
  {
    for(i=0;i<blobs->Blobs.Count;i++)
    {
      inSize = (inSize + 15) & ~15;
      inSize += blobs->Blobs[i].Size;
    }
  }

  OutLimit = inSize + (inSize/8) + 4096 + 512;
  OutImage = new sU8[OutLimit+512];
  OutSize = 0;

  // rebase debug info
  if(info)
    info->Rebase(PH->ImageStart);

  // check for resources
  ResourcesPresent = DataDir[5] != 0;

  // build the image: headers
  AppendImage("MZfarbrausch",12); // dos stub
  OPH = (PEHeader *) AppendImage(PH,sizeof(PEHeader));
  OPH->FileAlign = 512;  // more is a waste of space
  OPH->DataStart = 12;   // position of pe header
  OPH->DataDirs = ResourcesPresent ? 16 : 5;
  OPH->Sections = 1;
  OPH->OptHeaderSize = sizeof(PEHeader)-24+OPH->DataDirs*8;

  outDir = (sU32 *) AppendImage(DataDir,OPH->DataDirs*8);
  outSection = (PESection *) AppendImage(Section,sizeof(PESection));

  OPH->HeaderSize = Section[0].VirtAddr;
  imageEnd = Section[PH->Sections-1].VirtAddr+Section[PH->Sections-1].VirtSize;

  // account for blobs
  if(blobs)
  {
    for(i=0;i<blobs->Blobs.Count;i++)
    {
      imageEnd = (imageEnd + 15) & ~15;
      imageEnd += blobs->Blobs[i].Size;
    }

    ptrTableO = -1;
    for(i=0;i<PH->Sections;i++)
    {
      buffer = Source + SourceOffset(Section[i].VirtAddr);

      for(j=0;j<(sInt) (Section[i].Size - 8);j++)
      {
        if(!sCmpMem(buffer+j,"PTRTABLE",8))
        {
          if(ptrTableO != -1)
          {
            sSPrintF(Error,sizeof(Error),"multiple candidate PTRTABLEs in file");
            delete[] OutImage;
            return 0;
          }
          else
            ptrTableO = j + Section[i].VirtAddr;
        }
      }
    }

    if(ptrTableO == -1)
    {
      sSPrintF(Error,sizeof(Error),"blobs specified but no PTRTABL present");
      delete[] OutImage;
      return 0;
    }
  }

	dataSize = Align(imageEnd-Section[0].VirtAddr,PH->FileAlign);

  // process import records
  if(!ProcessImports())
  {
    delete[] OutImage;
    return 0;
  }

  dataSize += ImportSize;

  // filter code section and clear original code section
  sU32 codeSize = PH->CodeSize;
  sU8 *codePtr = Source + SourceOffset(PH->CodeStart);
  /*while(codeSize && !codePtr[codeSize - 1])
    codeSize--;*/

  filter.Filter(codePtr,codeSize,PH->ImageStart + PH->CodeStart,info);
  dataSize += depacker2Size + filter.Output.Size;

  // test unfilter (and build reorder buffer)
  buffer = new sU8[codeSize + 16];
  DisUnFilter(filter.Output.Data,buffer,PH->ImageStart + PH->CodeStart,PH->ImageStart + Section[0].VirtAddr - filter.Output.Size,reorder);
  if(sCmpMem(buffer,codePtr,codeSize))
  {
    delete[] buffer;
    delete[] OutImage;
    sSPrintF(Error,512,"(internal) unfilter failed");
    return 0;
  }
  delete[] buffer;

  // build the (uncompressed) image (data chunk only)
  UnImage = new sU8[dataSize];
  sSetMem(UnImage,0,dataSize);
  
  sCopyMem(UnImage,depacker2,depacker2Size);
  depack2.Init(UnImage,depacker2Size);

  sCopyMem(UnImage + depacker2Size,Imports,ImportSize);
  delete[] Imports;
  
  sCopyMem(UnImage + depacker2Size + ImportSize,filter.Output.Data,filter.Output.Size);

  UnImageVA = UnImage + depacker2Size + ImportSize + filter.Output.Size - Section[0].VirtAddr;

  for(i=0;i<PH->Sections;i++)
    if(Section[i].DataPtr)
      sCopyMem(UnImageVA+Section[i].VirtAddr,source+Section[i].DataPtr,Section[i].Size);

  // process resources
  ProcessResources();

  // clean up
  CleanupUnImage();
  sSetMem(UnImageVA + PH->CodeStart,0,codeSize);

  // copy blobs (and write blob ptrs to ptrtable)
  if(blobs)
  {
    j = Section[PH->Sections-1].VirtAddr+Section[PH->Sections-1].VirtSize;
    for(i=0;i<blobs->Blobs.Count;i++)
    {
      j = (j + 15) & ~15;
      ((sU32 *) (UnImageVA + ptrTableO))[i] = j + PH->ImageStart;
      sCopyMem(UnImageVA + j,blobs->Blobs[i].Data,blobs->Blobs[i].Size);
      j += blobs->Blobs[i].Size;
    }
  }

  // set up (stage2) depacker params
  depack2Base = PH->ImageStart + Section[0].VirtAddr - depacker2Size - ImportSize - filter.Output.Size;
  depack2.PatchOffsets(depack2Base);
  depack2.Patch("DEPACKTABLE",filter.Table,sizeof(filter.Table));
  depack2.PatchDWord("DCOD",PH->ImageStart + PH->CodeStart);
  depack2.PatchJump("ETRY",PH->ImageStart + PH->Entry,depack2Base);

  skip = 0;
  while(skip<dataSize && !UnImage[dataSize-skip-1])
    skip++;

  // compress the image (right to its destination)
  dataStart = OutSize;
  OutSize += pfe->Pack(UnImage,dataSize-skip,OutImage+dataStart,cb,0);
  //sSystem->SaveFile("unpacked.dat",UnImage,dataSize-skip);
  //sSystem->SaveFile("packed.dat",OutImage+dataStart,OutSize-dataStart);

  // verify that packing was successful and clean up
  if(pfe->GetBackEnd()->GetDepacker())
  {
    buffer = new sU8[dataSize-skip];

    if(info)
    {
      info->StartAnalyze(depack2Base,&reorder);
      i = pfe->GetBackEnd()->GetDepacker()(buffer,OutImage+dataStart,&DebugInfo::TokenizeCallback,info);
      info->FinishAnalyze();
    }
    else 
      i = pfe->GetBackEnd()->GetDepacker()(buffer,OutImage+dataStart,0,0);

    if(sCmpMem(buffer,UnImage,dataSize-skip))
    {
      delete[] buffer;
      delete[] UnImage;
      delete[] OutImage;
      sSPrintF(Error,512,"depacker didn't successfully depack data");
      return 0;
    }

    delete[] buffer;
  }

  delete[] UnImage;
  if(OutSize<OPH->HeaderSize)
    Padding(OPH->HeaderSize-OutSize);

  // build the packed image
  Depacker.Init(AppendImage(depacker,depackerSize),depackerSize);

  // calc depacker size and move image start
  depSize = OutSize-OPH->HeaderSize+depackerWorkSize+depacker2Size+ImportSize+filter.Output.Size;
  OPH->ImageStart -= Align(depSize,65536);

  // resources
  if(ResourcesPresent)
  {
    FinishResources(outDir);
    sSPrintF(WarningsPtr,512,"Resources present, this decreases compression a bit");
    WarningsPtr += sGetStringLen(WarningsPtr) + 1;
  }

  // import loookup table
  iltOffs = OutSize;
  p = (sU32 *) (OutImage + OutSize);
  *p++ = iltOffs+24+12;
  *p++ = iltOffs+24+26;
  *p++ = 0;
  *p++ = iltOffs+24+12;
  *p++ = iltOffs+24+26;
  *p++ = 0;
  OutSize = ((sU8 *) p) - OutImage;

  // import names
  AppendImage("KERNEL32.DLL\x00\x00LoadLibraryA\x00\x00GetProcAddress\x00\x00",44);

  // import directory
  outDir[2] = OutSize;
  outDir[3] = 40;

  p = (sU32 *) (OutImage + OutSize);
  *p++ = iltOffs + 12;
  *p++ = 0;
  *p++ = 0;
  *p++ = iltOffs + 24;
  *p++ = iltOffs;
  OutSize = ((sU8 *) p) - OutImage;
  ActualSize = OutSize;
  AlignmentPadding(OPH->FileAlign);

  sCopyMem(&outSection->Name,"kkrunchy",8);
  outSection->VirtSize = Align(depSize,65536)+dataSize;
  outSection->VirtAddr = OPH->HeaderSize;
  outSection->Size = Align(OutSize-OPH->HeaderSize,OPH->FileAlign);
  outSection->DataPtr = OPH->HeaderSize;
  outSection->Flags = 0xe00000e0;
  OPH->ImageSize = Align(outSection->VirtSize+OPH->HeaderSize,OPH->ObjectAlign);
  OPH->Entry = Depacker.GetCode() - OutImage;

  if(OPH->ImageStart < 0x400000)
  {
    sSPrintF(WarningsPtr,512,"Out ImageBase 0x%x lower than 0x400000, won't run under Win9x",OPH->ImageStart);
    WarningsPtr += sGetStringLen(WarningsPtr)+1;
  }

  Depacker.PatchOffsets(OPH->ImageStart + OPH->Entry);
  Depacker.PatchDWord("ESI0",OPH->ImageStart + dataStart);
  Depacker.PatchDWord("EDI0",PH->ImageStart + Section[0].VirtAddr - depacker2Size - ImportSize - filter.Output.Size);
  //Depacker.PatchJump("ETRY",depack2Base,OPH->ImageStart + OPH->Entry);
  Depacker.PatchDWord("IMPO",PH->ImageStart + Section[0].VirtAddr - ImportSize - filter.Output.Size);
  Depacker.PatchDWord("LLIB",OPH->ImageStart + iltOffs + 0);
  Depacker.PatchDWord("WORK",OPH->ImageStart + OutSize + 8);

  // clean up
  *WarningsPtr++ = 0;
  Resources.Exit();
  ResourceStrings.Exit();
  ResourceData.Exit();

  outSize = OutSize;
  return OutImage;
}

sChar *EXEPacker::GetErrorMessage()
{
  return Error;
}

sChar *EXEPacker::GetWarningMessages()
{
  return Warnings;
}

sU32 EXEPacker::GetActualSize()
{
  return ActualSize;
}
