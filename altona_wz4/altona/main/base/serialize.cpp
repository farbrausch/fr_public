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

#include "base/serialize.hpp"
#include "base/system.hpp"

static const sInt sSerMaxBytes = 0x10000;
#define sSerMaxAlign 16        // just good alignment

/****************************************************************************/
/***                                                                      ***/
/***   Streaming                                                          ***/
/***                                                                      ***/
/****************************************************************************/

static const sInt sSerObjectMax = 0x10000;

struct sWriteLink
{
  const void *Ptr;
  sWriteLink *Next;
};

#define STATICMEM (sPLATFORM!=sPLAT_WINDOWS && sPLATFORM!=sPLAT_LINUX)

#if STATICMEM

static const sInt sSerObjectMaxW = sSerObjectMax/2;

static union
{
  sWriteLink WOL[sSerObjectMaxW]; // write object list
  void *ROL[sSerObjectMax]; // read object list
} sSerLink;

static const sInt sSerBufferSize = sSerMaxBytes*3+sSerMaxAlign;
sALIGNED(static sU8,sSerBuffer[sSerBufferSize],sSerMaxAlign);
static sInt sSerBufferUsed = 0;

#else

static const sInt sSerObjectMaxW = sSerObjectMax;

#endif 

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Serialisation, new endian-safe system                              ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

sWriter::sWriter()
{
  CheckEnd = 0;
  File = 0;
  Ok = 1;
  Data = 0;
  Buffer = 0;
  BufferSize = 0;

#if STATICMEM
  WOL = sSerLink.WOL;
#else
  WOL = new sWriteLink[sSerObjectMaxW];
#endif
}

sWriter::~sWriter()
{
#if !STATICMEM
  delete[] WOL;
#endif
}

void sWriter::Begin(sFile *file)
{
  File = file;
#if !STATICMEM
  BufferSize = sSerMaxBytes*2+sSerMaxAlign;
  Buffer = (sU8*) sAllocMem_(BufferSize,sSerMaxAlign,0);
#else
  sVERIFY(!sSerBufferUsed);   // multiple file reader/writers not supported with STATICMEM
  BufferSize = sSerBufferSize;
  Buffer = sSerBuffer;
  sSerBufferUsed = sTRUE;   
#endif

  Data = Buffer;
  CheckEnd = Buffer+sSerMaxBytes;
  if(!File)
    Ok = 0;
  sVERIFY((sDInt(Buffer)&(sSerMaxAlign-1))==0);

  WOCount = 1;
  sClear(WOH);
}

sBool sWriter::End()
{
  if(Ok && !File->Write(Buffer,Data-Buffer))
    Ok = 0;
#if !STATICMEM
  sFreeMem(Buffer);
  Buffer = 0;
#else
  sSerBufferUsed = sFALSE;
#endif
  return Ok;
}

void sWriter::Check()
{
  if(!Ok)
  {
    Data = Buffer;
  }
  else
  {
    if(Data>=CheckEnd)
    {
      sDInt bytes = (Data-Buffer) & ~(sSerMaxAlign-1);
      sDInt left = (Data-Buffer) - bytes;
      sVERIFY(bytes>0);
      File->Write(Buffer,bytes);
      sCopyMem(Buffer,Buffer+bytes,left);
      Data -= bytes;
    }
  }
}


sDInt sWriter::GetSize()
{
  sDInt bytes = (Data-Buffer);
  return File->GetOffset() + bytes;
}


/****************************************************************************/

sInt sWriter::Header(sU32 id,sInt currentversion)
{
  Align();
  Check();
  U32(sMAKE4('<','<','<','<'));
  U32(id);
  U32(currentversion);
  return currentversion;
}

void sWriter::Footer()
{
  Check();
  U32(sMAKE4('>','>','>','>'));
}

void sWriter::Skip(sInt bytes)
{
  sInt chunk;
  Check();
  while(bytes>0)
  {
    chunk = sMin(sSerMaxBytes/1,bytes);
    for(sInt i=0;i<chunk;i++)
      U8(0);
    Check();
    bytes -= chunk;
  }
}

void sWriter::Align(sInt alignment)
{
  while(sDInt(Data)&(alignment-1))
    U8(0);
}

// registering a null ptr is ok. but the implementation is not as expected:
// the first pointer registered is always a null ptr,
// therefore null-ptrs are always serialized as index 0.
// in addition to that, there is a special test that ensures that null-ptrs
// are always written as index 0, no matter whats in the hash table.

// things get a bit more complicated:
// there is no check for registering a pointer twice.
// a second record for the same pointer is created and inserted into the
// hash table BEFORE the old one. from now on, the new index is prefered.
// but null-ptrs have special check.

sInt sWriter::RegisterPtr(void *obj)
{
  sVERIFY(WOCount<sSerObjectMaxW);

  sDInt hash = sDInt(obj);
  hash = (hash & 0xffff) ^ ((hash>>16)&0xffff);
  hash = (hash & 0x00ff) ^ ((hash>> 8)&0x00ff);

  WOL[WOCount].Ptr = obj;
  WOL[WOCount].Next = WOH[hash];
  WOH[hash] = &WOL[WOCount];

  return WOCount++;
}

sBool sWriter::IsRegistered(void *obj)
{
  sWriteLink *e;

  if(obj==0)
  {
    U32(0);
    return 1;       // zero pointers are always registered!
  }

  sDInt hash = sDInt(obj);
  hash = (hash & 0xffff) ^ ((hash>>16)&0xffff);
  hash = (hash & 0x00ff) ^ ((hash>> 8)&0x00ff);

  e = WOH[hash];
  while(e)
  {
    if(e->Ptr == obj)
    {
      sInt i = sInt(e-WOL);
      sVERIFY(i>=1 && i<WOCount);
      return 1;
    }
    e = e->Next;
  }

  return 0;
}

void sWriter::VoidPtr(const void *obj)
{
  sWriteLink *e;

  if(obj==0)
  {
    U32(0);
    return;
  }

  sDInt hash = sDInt(obj);
  hash = (hash & 0xffff) ^ ((hash>>16)&0xffff);
  hash = (hash & 0x00ff) ^ ((hash>> 8)&0x00ff);

  e = WOH[hash];
  while(e)
  {
    if(e->Ptr == obj)
    {
      sInt i = sInt(e-WOL);
      sVERIFY(i>=1 && i<WOCount);
      U32(i);
      return;
    }
    e = e->Next;
  }
  sFatal(L"tried to write unregistered object %08x",(sDInt)obj);
}

void sWriter::Bits(sInt *a,sInt *b,sInt *c,sInt *d,sInt *e,sInt *f,sInt *g,sInt *h)
{
  sU8 bits = 0;
  if(a) bits |= (*a&0x01)<<0;
  if(b) bits |= (*b&0x01)<<1;
  if(c) bits |= (*c&0x01)<<2;
  if(d) bits |= (*d&0x01)<<3;
  if(e) bits |= (*e&0x01)<<4;
  if(f) bits |= (*f&0x01)<<5;
  if(g) bits |= (*g&0x01)<<6;
  if(h) bits |= (*h&0x01)<<7;
  U8(bits);
}

/****************************************************************************/

void sWriter::ArrayU8(const sU8 *ptr,sInt count)
{
  sInt chunk;
  Check();
  while(count>0)
  {
    chunk = sMin(sSerMaxBytes/1,count);
//    for(sInt i=0;i<chunk;i++)
//      U8(*ptr++);
    sCopyMem(Data,ptr,chunk);
    Data += chunk;
    ptr += chunk;
    Check();
    count -= chunk;
  }
}

void sWriter::ArrayU16(const sU16 *ptr,sInt count)
{
  sInt chunk;
  Check();
  while(count>0)
  {
    chunk = sMin(sSerMaxBytes/2,count);
    for(sInt i=0;i<chunk;i++)
      U16(*ptr++);
    Check();
    count -= chunk;
  }
}

void sWriter::ArrayU16Align4(const sU16 *ptr,sInt count)
{
  sInt chunk;
  Check();
  while(count>0)
  {
    chunk = sMin(sSerMaxBytes/2,count);
    for(sInt i=0;i<chunk;i++)
      U16(*ptr++);
    if(chunk & 1)
    {
      sU16 pad=0;
      U16(pad);
    }
    Check();
    count -= chunk;
  }
}

void sWriter::ArrayU32(const sU32 *ptr,sInt count)
{
  sInt chunk;
  Check();
  while(count>0)
  {
    chunk = sMin(sSerMaxBytes/4,count);
    for(sInt i=0;i<chunk;i++)
      U32(*ptr++);
    Check();
    count -= chunk;
  }
}

void sWriter::ArrayU64(const sU64 *ptr,sInt count)
{
  sInt chunk;
  Check();
  while(count>0)
  {
    chunk = sMin(sSerMaxBytes/8,count);
    for(sInt i=0;i<chunk;i++)
      U64(*ptr++);
    Check();
    count -= chunk;
  }
}

void sWriter::String(const sChar *v)
{
  sInt len = sGetStringLen(v);
  U32(len);
  ArrayU16((sU16 *)v,len);
  Align();
}

/****************************************************************************/
/****************************************************************************/

sReader::sReader()
{
  File = 0;
  Map = 0;
  Buffer = 0;
  BufferSize = 0;
  ReadLeft = 0;
  Ok = 0;
  Data = 0;
  CheckEnd = 0;
  LoadEnd = 0;
  DontMap = 1;
  LastId = 0;

  ROCount = 0;

#if STATICMEM
  ROL = sSerLink.ROL;
#else
  ROL = (void **)sAllocMem(sSerObjectMax*sizeof(void *),4,sAMF_ALT);
#endif
}

sReader::~sReader()
{
#if !STATICMEM
  sFreeMem(Buffer);
  sFreeMem(ROL);
#else
  sSerBufferUsed = sFALSE;
#endif
}

void sReader::Begin(sFile *file)
{
  File = file;
  ReadLeft = File->GetSize();
  Map = 0;
  Ok = 1;     // Ok = 1 before first check, otherwise check fails
  LastId = 0;
#if !STATICMEM
  if(!DontMap)
    Map = File->MapAll();
  if(Map==0)
  {
    BufferSize = sSerMaxBytes*3+sSerMaxAlign;
    Buffer = (sU8 *)sAllocMem(BufferSize,64,0);
    Data = CheckEnd = LoadEnd = Buffer;
    Check();
  }
  else
  {
    Data = Map;
  }
#else
  sVERIFY(!sSerBufferUsed);   // multiple file reader/writers not supported with STATICMEM
  sSerBufferUsed = sTRUE;
  BufferSize = sSerBufferSize;
  Buffer = sSerBuffer;
  Data = CheckEnd = LoadEnd = Buffer;
  Check();
#endif

  ROCount = 1;
  ROL[0] = 0;
}

sBool sReader::End()
{
#if !STATICMEM
  sFreeMem(Buffer);
  Buffer = 0;
#else
  sSerBufferUsed = sFALSE;
#endif
  File = 0;
  Map = 0;
  Buffer = 0;
  BufferSize = 0;
  ReadLeft = 0;

  return Ok;
}

#define FAKEMULTITHREADING 0
// fakemultithreading by bjoern 
// use this only if you know what you're doing
// (i am doing animated loading screens)
#if FAKEMULTITHREADING
void Render3D();
#endif 

void sReader::Check()
{
  if(Map)
  {
  }
  else if(!Ok)
  {
    Data = Buffer;
  }
  else if(Data>=CheckEnd)
  {
    if(Data>LoadEnd)
      sFatal(L"read past end of serialize buffer (last header read: %08x). call sReader::Check more often!",LastId);
    sDInt remove = (Data-Buffer) & ~(sSerMaxAlign-1);
    sDInt copy = (LoadEnd-Buffer) - remove;

    sCopyMem(Buffer,Buffer+remove,copy);
    Data -= remove;
    LoadEnd -= remove;
    sDInt load = Buffer+BufferSize-LoadEnd;
    if(load>ReadLeft) 
      load = ReadLeft;
    if(load>0)
    {
      if(!File->Read(LoadEnd,load))
        Ok = 0;
      LoadEnd += load;
      ReadLeft -= load;

#if FAKEMULTITHREADING
      sVERIFYSTATIC(!sSTRIPPED); // never ever enable this in production
      Render3D();
#endif
    }
    CheckEnd = Buffer+BufferSize-sSerMaxBytes-sSerMaxAlign;
    // checksumme erzeugen:
#if 0
    sU32 size = sU32(CheckEnd-Data);
    sU32 checksum = sChecksumCRC32(Data,size);
    sLogF(L"sReader",L"chunksize:%d checksum:%x\n",size,checksum);
#endif
  }
}

void sReader::DebugPeek(sInt count)
{
  const sU32 *ptr = (const sU32 *)Data;
  for(sInt i=0;i<count;i++)
    sDPrintF(L"%08x ",ptr[i]);
  sDPrintF(L"\n");
}

/****************************************************************************/

sInt sReader::Header(sU32 id,sInt currentversion)
{
  sU32 value;
  sInt version;
  Check();
  Align();
  U32(value); if(value!=sMAKE4('<','<','<','<')) Ok  = 0;
  U32(value);
  if(value!=id) Ok = 0;
  S32(version);
  if(version>currentversion)
  {
    Ok = 0;
    sLogF(L"file",L"warning: newer version number - updated sourcecode?\n");
  }
  LastId = Ok ? id : 0;
  return Ok ? version : 0;
}

sU32 sReader::PeekHeader()
{
  sU32 value;
  Check();
  const sU8 *tmp = Data;
  Align();
  U32(value); if(value!=sMAKE4('<','<','<','<')) Ok  = 0;
  U32(value);
  Data = tmp;
  return value;
}

sU32 sReader::PeekU32()
{
  sU32 value;
  Check();
  const sU8 *tmp = Data;
  Align();
  U32(value);
  Data = tmp;
  return value;
}

void sReader::Footer()
{
  sU32 v;
  Check();
  U32(v); if(v!=sMAKE4('>','>','>','>')) Ok = 0;
}

sBool sReader::PeekFooter()
{
  const sU8 *tmp = Data;
  sU32 v;
  Check();
  sBool result = sFALSE;
  U32(v);
  if(v==sMAKE4('>','>','>','>'))
    result = sTRUE;
  Data = tmp;
  return result;
}

void sReader::Skip(sInt bytes)
{
  sInt chunk;
  Check();
  while(bytes>0)
  {
    chunk = sMin(sSerMaxBytes/1,bytes);
    Data += chunk;
    Check();
    bytes -= chunk;
  }
}

const sU8 *sReader::GetPtr(sInt bytes)
{
  const sU8 *result;
  Check();
  result = Data;
  Data += bytes;
  sVERIFY(Data<CheckEnd);     // don't use this function, it is broken by design
  Check();
  return result;
}

void sReader::Align(sInt alignment)
{
  sDInt misalign = sDInt(Data)&(alignment-1);
  if(misalign>0)
    Data += alignment-misalign;
  sVERIFY((sDInt(Data) & (alignment-1)) == 0);
}

sInt sReader::RegisterPtr(void *obj)
{
  sVERIFY(ROCount<sSerObjectMax);
  ROL[ROCount] = obj;
  return ROCount++;
}

void sReader::VoidPtr(void *&obj)
{
  sInt i;
  S32(i);
  sVERIFY(i>=0 && i<ROCount);
  obj = ROL[i];
}

void sReader::Bits(sInt *a,sInt *b,sInt *c,sInt *d,sInt *e,sInt *f,sInt *g,sInt *h)
{
  sU8 bits;
  U8(bits);
  if(a) *a = (bits>>0)&0x01;
  if(b) *b = (bits>>1)&0x01;
  if(c) *c = (bits>>2)&0x01;
  if(d) *d = (bits>>3)&0x01;
  if(e) *e = (bits>>4)&0x01;
  if(f) *f = (bits>>5)&0x01;
  if(g) *g = (bits>>6)&0x01;
  if(h) *h = (bits>>7)&0x01;
}

/****************************************************************************/

void sReader::ArrayU8(sU8 *ptr,sInt count)
{
  sInt chunk;
  Check();
  while(count>0)
  {
    chunk = sMin(sSerMaxBytes/1,count);
//    for(sInt i=0;i<chunk;i++)
//      U8(*ptr++);
    sCopyMem(ptr,Data,chunk);
    Data += chunk;
    ptr += chunk;
    Check();
    count -= chunk;
  }
}

void sReader::ArrayU16(sU16 *ptr,sInt count)
{
  sInt chunk;
  Check();
  while(count>0)
  {
    chunk = sMin(sSerMaxBytes/2,count);
    for(sInt i=0;i<chunk;i++)
      U16(*ptr++);
    Check();
    count -= chunk;
  }
}

void sReader::ArrayU32(sU32 *ptr,sInt count)
{
  sInt chunk;
  Check();
  while(count>0)
  {
    chunk = sMin(sSerMaxBytes/4,count);
    //for(sInt i=0;i<chunk;i++)
    //  U32(*ptr++);
    const sU8 *data = Data;
    for(sInt i=0;i<chunk;i++)
    {
      sUnalignedLittleEndianLoad32(data,*ptr++);
      data += 4;
    }
    Data = data;
    Check();
    count -= chunk;
  }
}

void sReader::ArrayU64(sU64 *ptr,sInt count)
{
  sInt chunk;
  Check();
  while(count>0)
  {
    chunk = sMin(sSerMaxBytes/8,count);
    for(sInt i=0;i<chunk;i++)
      U64(*ptr++);
    Check();
    count -= chunk;
  }
}

void sReader::String(sChar *v,sInt maxsize)
{
  sInt len;
  maxsize--;
  S32(len);
  if(len<=maxsize)
  {
    ArrayU16((sU16 *)v,len);
    v[len] = 0;
  }
  else
  {
    ArrayU16((sU16 *)v,maxsize);
    v[maxsize] = 0;
    Skip((len-maxsize)*sizeof(sU16));
  }
  Align();
}

/****************************************************************************/
/****************************************************************************/
