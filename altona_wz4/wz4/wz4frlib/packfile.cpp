/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "packfile.hpp"

#include "base/system.hpp"
#include "base/windows.hpp"
#include "base/types2.hpp"

#include "base/graphics.hpp"
#include "util/shaders.hpp"

/****************************************************************************/

struct sDemoPackFile::PackHeader
{
  sInt NameOffset;
  sInt FileOffset;
  sInt PackedSize;
  sInt OriginalSize;
  sInt dummy;
};

class DPFUnpacked : public sFile
{
  sFile *BaseFile;
  sS64 BaseOffset;
  sS64 BaseSize;
  sS64 Offset;
  sS64 Size;
public:
  DPFUnpacked(sFile *base,sS64 offset,sS64 size);
  ~DPFUnpacked();
  sBool Read(void *data,sDInt size);
  sBool SetOffset(sS64 offset);       // seek to offset
  sS64 GetOffset();                   // get offset
  sS64 GetSize();                     // get size
};


class DPFPacked : public sFile
{
  sInt SeekSize;
  sInt ChunkSize;

  sFile *BaseFile;
  sS64 BaseOffset;
  sS64 BaseSize;
  sS64 ReadOffset;
  sInt DepackStart;
  sS64 Size;
  sS64 Offset;

  sU8 *DestBuffer;
  sU8 *DestEnd;
  sU8 *DestPtr;
public:
  DPFPacked(sFile *base,sS64 offset,sS64 size);
  ~DPFPacked();
  sBool Read(void *data,sDInt size);
  sBool SetOffset(sS64 offset);       // seek to offset
  sS64 GetOffset();                   // get offset
  sS64 GetSize();                     // get size

  void LoadChunk(sU8 *,sInt size);
};

/****************************************************************************/


sU32 CCADepacker(sU8 *dst,const sU8 *src);

/****************************************************************************/

sDemoPackFile::sDemoPackFile(const sChar *name)
{
  sU32 Header[4];
  sClear(Header);
  File = sCreateFile(name,sFA_READ);
  File->Read(Header,16);

  sU32 *ptr = (sU32 *) Header;
#if sCONFIG_BE
  for(sInt i=0;i<4;i++)
    ptr[i] = sSwapEndian(ptr[i]);
#endif

  sVERIFY(ptr[0] == 0xdeadbeef);
  Count = ptr[1];

  Data = new sU8[Header[2]];
  sCopyMem(Data,Header,16);
  sVERIFY(Header[2]>32);
  File->Read(Data+16,Header[2]-16);
  Dir = (PackHeader *)(Data+16);
  
#if sCONFIG_BE
  sU32 *dir0 = (sU32 *)(Data+16);
  sU32 *dir1 = (sU32 *)(Data+16+Count*sizeof(PackHeader));
  for(sU32 *ptr=dir0;ptr<dir1;ptr++)
    *ptr = sSwapEndian(*ptr);

  sU16 *string0 = (sU16 *)(Data+16+Count*sizeof(PackHeader));
  sU16 *string1 = (sU16 *)(Data+Dir[0].FileOffset);
  for(sU16 *ptr=string0;ptr<string1;ptr++)
    *ptr = (*ptr>>8)|(*ptr<<8);
#endif
}

sDemoPackFile::~sDemoPackFile()
{
  delete[] Data;
  delete File;
}

/****************************************************************************/

sFile *sDemoPackFile::Create(const sChar *name,sFileAccess access)
{
  PackHeader *file;

  if(access!=sFA_READ && access!=sFA_READRANDOM) return 0;

  name = sFindFileWithoutPath(name);
  for(sInt i=0;i<Count;i++)
  {
    file = &Dir[i];
    if(sCmpStringI(name,(const sChar *)(Data+file->NameOffset))==0)
    {
      sLogF(L"file",L"open packfile offset %08x:%08x file <%s>\n",file->FileOffset,file->OriginalSize,name);
      if(file->OriginalSize==file->PackedSize)
        return new DPFUnpacked(File,file->FileOffset,file->OriginalSize);
      else
        return new DPFPacked(File,file->FileOffset,file->OriginalSize);
    }
  }
  return 0;
}

/****************************************************************************/
/****************************************************************************/

static sGeometry *sProgressGeo;
static sMaterial *sProgressMtrl;
sBool sProgressDoCallBeginEnd=1;

void sProgress(sInt done,sInt max)
{
//  sDPrintF(L"%d/%d\n",done,max);
  if(sProgressMtrl==0) return;      // sometimes we load ouseide of progress bar...
  static sInt lasttime = -1000;

  sInt time = sGetTime();
  sBool hasfocus = 0;
  if((time>lasttime+75 || max==1 || hasfocus || !sProgressDoCallBeginEnd) && sProgressMtrl)
  {
    lasttime = time;

    if(sProgressDoCallBeginEnd)
      sRender3DBegin();

    sInt xs,ys;
    sGetScreenSize(xs,ys);


    sRect r(xs/10,ys/2-16,xs*9/10,ys/2+16);
    sInt w = r.SizeX();
    sInt x = sClamp(sMulDiv(done,w,max),0,w);

    sSetTarget(sTargetPara(sCLEAR_COLOR,0x000000,0));

    sViewport view;
    sVertexBasic *vp;
    view.Orthogonal = sVO_PIXELS;
    view.SetTargetCurrent(0);
    view.Prepare();
    sCBuffer<sSimpleMaterialPara> cb;
    cb.Data->mvp = view.ModelScreen;
    sProgressMtrl->Set(&cb);
    sProgressGeo->BeginLoadVB(3*4,sGD_STREAM,&vp);

    sU32 col1 = 0xffffffff;
    sU32 col0 = 0xff000000;
    vp[0].Init(r.x1+8+0.5f,r.y0-8+0.5f,0,col1);
    vp[1].Init(r.x1+8+0.5f,r.y1+8+0.5f,0,col1);
    vp[2].Init(r.x0-8+0.5f,r.y1+8+0.5f,0,col1);
    vp[3].Init(r.x0-8+0.5f,r.y0-8+0.5f,0,col1);
    vp+=4;
    vp[0].Init(r.x1+4+0.5f,r.y0-4+0.5f,0,col0);
    vp[1].Init(r.x1+4+0.5f,r.y1+4+0.5f,0,col0);
    vp[2].Init(r.x0-4+0.5f,r.y1+4+0.5f,0,col0);
    vp[3].Init(r.x0-4+0.5f,r.y0-4+0.5f,0,col0);
    vp+=4;
    vp[0].Init(r.x0+x+0.5f,r.y0  +0.5f,0,col1);
    vp[1].Init(r.x0+x+0.5f,r.y1  +0.5f,0,col1);
    vp[2].Init(r.x0  +0.5f,r.y1  +0.5f,0,col1);
    vp[3].Init(r.x0  +0.5f,r.y0  +0.5f,0,col1);
    vp+=4;

    sProgressGeo->EndLoadVB();
    sProgressGeo->Draw();
    if(sProgressDoCallBeginEnd)
      sRender3DEnd();
  }
}

void sProgressBegin()
{
  sProgressGeo = new sGeometry;
  sProgressGeo->Init(sGF_QUADLIST,sVertexFormatBasic);
  sProgressMtrl = new sSimpleMaterial;
  sProgressMtrl->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  sProgressMtrl->Prepare(sVertexFormatBasic);

  sProgress(0,1);
  sProgress(0,1);
}

void sProgressEnd()
{
  sProgress(1,1);
  sProgress(1,1);
  sDelete(sProgressMtrl);
  sDelete(sProgressGeo);
}


/****************************************************************************/
/****************************************************************************/

DPFUnpacked::DPFUnpacked(sFile *base,sS64 offset,sS64 size)
{
  BaseFile = base;
  BaseOffset = offset;
  BaseSize = BaseFile->GetSize();
  Size = size;
  Offset = 0;
}

DPFUnpacked::~DPFUnpacked()
{
}

sBool DPFUnpacked::Read(void *data,sDInt size)
{
  sVERIFY(Offset+size<=Size);
  BaseFile->SetOffset(Offset+BaseOffset);
  Offset += size;

//  sProgress(Offset+BaseOffset,BaseSize);
  
  return BaseFile->Read(data,size);
}

sBool DPFUnpacked::SetOffset(sS64 offset)
{
  sVERIFY(offset>=0 && offset<=Size);
  Offset = offset;
  return sTRUE;
}

sS64 DPFUnpacked::GetOffset()
{
  return Offset;
}

sS64 DPFUnpacked::GetSize()
{
  return Size;
}

/****************************************************************************/
/****************************************************************************/

/****************************************************************************/


static const sInt CodeModel = 0;
static const sInt PrevMatchModel = 2;
static const sInt MatchLowModel = 3; // +(pos>=16)*16
static const sInt LiteralModel = 35;
static const sInt Gamma0Model = 291;
static const sInt Gamma1Model = 547;
static const sInt SizeModels = 803;

static const sInt SrcBufferSize = 16*1024;

struct DepackState
{
  // ari
  const sU8 *Src;
  const sU8 *End;
  sU32 Code,Range;
  DPFPacked *pf;

  // continue

  sU8 *DstO;       // original dest
  sInt ContOfs;
  sU32 ContLen;
    
  sInt _code,_LWM,_compSize,_R0;

  
  // probabilities
  sU32 Model[SizeModels];
  sU8 SourceBuffer[SrcBufferSize];
};

static DepackState stx;

static void DecodeLoadSrcBuffer(DepackState &st)
{
  st.pf->LoadChunk(st.SourceBuffer,SrcBufferSize);
  st.Src = st.SourceBuffer;
  st.End = st.Src + SrcBufferSize;
}

sINLINE static sBool DecodeBit(DepackState &st,sInt index,sInt move,sU32 &Code,sU32 &Range)
{
  sU32 bound;
  sBool result;

  // decode
  bound = (Range >> 11) * st.Model[index];
  if(Code < bound)
  {
    Range = bound;
    st.Model[index] += (2048 - st.Model[index]) >> move;
    result = sFALSE;
  }
  else
  {
    Code -= bound;
    Range -= bound;
    st.Model[index] -= st.Model[index] >> move;
    result = sTRUE;
  }

  // renormalize
  if(Range < 0x01000000U) // no while, all our codes take <8bit
  {
    if(st.Src==st.End)
      DecodeLoadSrcBuffer(st);
    Code = (Code << 8) | *st.Src++;
    Range <<= 8;
  }

  return result;
}

sINLINE static sInt DecodeTree(DepackState &st,sInt model,sInt maxb,sInt move,sU32 &Code,sU32 &Range)
{
  sInt ctx;

  ctx = 1;
  while(ctx < maxb)
    ctx = (ctx * 2) + DecodeBit(st,model + ctx,move,Code,Range);

  return ctx - maxb;
}

sINLINE static sInt DecodeGamma(DepackState &st,sInt model,sU32 &Code,sU32 &Range)
{
  sInt value = 1;
  sU8 ctx = 1;

  do
  {
    ctx = ctx * 2 + DecodeBit(st,model + ctx,5,Code,Range);
    value = (value * 2) + DecodeBit(st,model + ctx,5,Code,Range);
    ctx = ctx * 2 + (value & 1);
  }
  while(ctx & 2);

  return value;
}

sU32 DecodeChunk(DepackState * __restrict st,sU8 *dst_,sU8 *dstend,sInt start,DPFPacked *pf)
{
  sInt i,offs,len;
  sInt code,LWM,compSize,R0;
  sU8 * __restrict dst = dst_;
  sU32 Code,Range;

  if(start)
  {
    st->pf = pf;

    DecodeLoadSrcBuffer(*st);

    st->ContLen = 0;
    st->DstO = dst;
    Code = 0;
    for(i=0;i<4;i++)
      Code = (Code<<8) | *st->Src++;

    Range = ~0;
    for(i=0;i<SizeModels;i++)
      st->Model[i] = 1024;

    code = 0;
    LWM = 0;
    R0 = 0;
    compSize = 0;
  }
  else 
  {
    code = st->_code;
    LWM = st->_LWM;
    compSize = st->_compSize;
    R0 = st->_R0;
    Code = st->Code;
    Range = st->Range;

    if(st->ContLen>0)
    {
      sInt chunk = sMin<sInt>(st->ContLen,dstend-dst);
      for(sInt i=0;i<chunk;i++)
        dst[i] = dst[i-st->ContOfs];
      dst += chunk;
      st->ContLen -= chunk;

      if(st->ContLen>0)
        return ~0;

      code = DecodeBit(*st,CodeModel + LWM,5,Code,Range);
    }
  }


  while(dst<dstend)
  {
    switch(code)
    {
    case 0: // literal
      *dst++ = DecodeTree(*st,LiteralModel,256,4,Code,Range);
      compSize = 1;
      LWM = 0;
      break;
      
    case 1: // match
      len = 0;

      if(!LWM && DecodeBit(*st,PrevMatchModel,5,Code,Range)) // prev match
        offs = R0;
      else
      {
        offs = DecodeGamma(*st,Gamma0Model,Code,Range);
        if(!offs)
          return dst - st->DstO;

        offs -= 2;
        offs = (offs << 4) + DecodeTree(*st,MatchLowModel + (offs ? 16 : 0),16,5,Code,Range) + 1;
        
        if(offs>=2048)  len++;
        if(offs>=96)    len++;
      }
      
      R0 = offs;
      LWM = 1;      
      len += DecodeGamma(*st,Gamma1Model,Code,Range);
      compSize = len;

      sInt chunk = sMin<sInt>(len,dstend-dst);
      sVERIFY(offs<640*1024);
      for(sInt i=0;i<chunk;i++)
        dst[i] = dst[i-offs];
      dst += chunk;
      len -= chunk;
      if(len>0)
      {
        st->ContOfs = offs;
        st->ContLen = len;
        goto ende;
      }
      break;
    }

    code = DecodeBit(*st,CodeModel + LWM,5,Code,Range);
  }

ende:

  st->_code = code;
  st->_LWM = LWM;
  st->_compSize = compSize;
  st->_R0 = R0;
  st->Code = Code;
  st->Range = Range;

  return ~0;
}
/*
sU32 DecodeChunkAll(sU8 *dst)
{
  sInt start = 1;
  sU32 r;
  do
  {
    r = DecodeChunk(dst,dst+0x100,start);
    dst += 0x100;
    start = 0;
  }
  while(r==~0);
  return r;
}
*/
/****************************************************************************/
/****************************************************************************/

DPFPacked::DPFPacked(sFile *base,sS64 offset,sS64 size)
{
  SeekSize = 640*1024;
  ChunkSize = 128*1024;
  DestBuffer = new sU8[SeekSize+ChunkSize];
  BaseFile = base;
  BaseOffset = offset;
  BaseSize = base->GetSize();
  Size = size;
  ReadOffset = 0;
  DestPtr = DestEnd = DestBuffer + SeekSize + ChunkSize;
  DepackStart = 1;
  Offset = 0;
}

DPFPacked::~DPFPacked()
{
  delete DestBuffer;
}

sBool DPFPacked::Read(void *datav,sDInt size)
{
  Offset += size;
  sU8 *data = (sU8 *) datav;
  while(size>0)
  {
    sVERIFY(DestPtr<=DestEnd);
    sInt chunk = sMin(size,DestEnd-DestPtr);
    if(chunk==0)
    {
      sCopyMem(DestBuffer,DestBuffer+ChunkSize,SeekSize);
      DecodeChunk(&stx,DestBuffer+SeekSize,DestEnd,DepackStart,this);
      DepackStart = 0;
      DestPtr = DestBuffer+SeekSize;
    }
    else
    {
      sCopyMem(data,DestPtr,chunk);
      data += chunk;
      DestPtr += chunk;
      sVERIFY(DestPtr<=DestEnd);
      size -= chunk;
    }
  } 
//  sProgress(ReadOffset+BaseOffset,BaseSize);
  return 1;
}

sBool DPFPacked::SetOffset(sS64 offset)
{
  sFatal(L"can't seek compressed files");
  return 0;
}

sS64 DPFPacked::GetOffset()
{
  return Offset;
}

sS64 DPFPacked::GetSize()
{
  return Size;
}

void DPFPacked::LoadChunk(sU8 *ptr,sInt size)
{
  BaseFile->SetOffset(ReadOffset+BaseOffset);
  BaseFile->Read(ptr,size);
  ReadOffset += size;
}

/****************************************************************************/


/****************************************************************************/

