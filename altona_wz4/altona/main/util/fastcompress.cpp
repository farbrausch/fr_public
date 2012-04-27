/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) 2005-2006 by Dierk Ohlerich                          ***/
/***   all rights reserverd                                               ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "fastcompress.hpp"
#include "base/system.hpp"

/****************************************************************************/

// Hash table size. Influences both compression and decompression. Changing this
// *will* break existing compressed files.
static const sU32 HashSize = 65536;

static const sU32 ChunkBits = 15;
static const sU32 ChunkSize = 1<<ChunkBits;
static const sU32 MaxOutSize = (ChunkSize*9 + 7) / 8 + 8 + 3; // 9bits/byte+header+align+safety margin
static const sInt LenBytes = 3;

static const sU32 PeriodicUpdate = 1<<24;
static const sU32 WrapMask = 2*ChunkSize-1;

// Length of first few exponential golomb codes (for fast encoding)
static const sU8 EGLen[31] =
{
  1,
  3,3,
  5,5,5,5,
  7,7,7,7, 7,7,7,7,
  9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9,
};

// Number of leading zeroes for first few binary values (for fast decoding)
static const sU8 LZCount[32] =
{
  5,                                  // 00000
  4,                                  // 00001
  3,3,                                // 00010/0011
  2,2,2,2,                            // 00100/0101/0110/0111
  1,1,1,1, 1,1,1,1,                   // 01***
  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // 1****
};

// Direct decode from first 5 bits
static const sU8 DirectV[32][2] =
{
  {0,0}, {0,0}, {0,0}, {0,0},         // 00000-00011 (invalid)
  {3,5}, {4,5}, {5,5}, {6,5},         // 00100-00111 (3-6)
  {1,3}, {1,3}, {1,3}, {1,3},         // 01000-01011 (1)
  {2,3}, {2,3}, {2,3}, {2,3},         // 01100-01111 (2)
  {0,1}, {0,1}, {0,1}, {0,1},         // rest is 0
  {0,1}, {0,1}, {0,1}, {0,1},
  {0,1}, {0,1}, {0,1}, {0,1},
  {0,1}, {0,1}, {0,1}, {0,1},
};

/****************************************************************************/

void sFastLzpCompressor::StartWrite(sU8 *ptr)
{
  BitPos1 = ptr+2; // \ these two lines make a lot more sense
  BitPos2 = ptr;   // / if you look at the decompressor.
  RawPos = ptr+4;
  BitBuffer = 0;
  BitShift = 32;
}

void sFastLzpCompressor::EndWrite()
{
  sVERIFY(BitShift >= 16);
  BitBuffer |= ~0u >> (32-BitShift); // lots of 1 bits (=0 as exp-golomb)
  sUnalignedLittleEndianStore16(BitPos1,BitBuffer>>16);
  sUnalignedLittleEndianStore16(BitPos2,BitBuffer&0xffff);
}

void sFastLzpCompressor::Shift()
{
  sUnalignedLittleEndianStore16(BitPos1,BitBuffer>>16);
  BitBuffer <<= 16;
  BitShift += 16;
  BitPos1 = BitPos2;
  BitPos2 = RawPos;
  RawPos += 2;
}

sINLINE void sFastLzpCompressor::PutExpGolomb(sU32 value)
{
  if(value < sCOUNTOF(EGLen)) // fast path for short (=most common) values
    PutBits(value+1,EGLen[value]);
  else // general version
  {
    value++;

    sU32 v = value;
    sInt nBits = 0;
    if(v & 0xffff0000)  v>>=16, nBits+=16;
    if(v & 0x0000ff00)  v>>= 8, nBits+= 8;
    if(v & 0x000000f0)  v>>= 4, nBits+= 4;
    if(v & 0x0000000c)  v>>= 2, nBits+= 2;
    nBits += v >> 1;

    if(nBits>1) PutBitsLong(0,nBits);
    PutBitsLong(value,nBits+1);
  }
}

sInt sFastLzpCompressor::CompressChunk(sU32 nBytes,sBool isLast)
{
  sVERIFYSTATIC(WrapMask>0 && (WrapMask & (WrapMask+1)) == 0); // power of 2
  sVERIFYSTATIC(ChunkSize < (1<<24));
  sVERIFY(nBytes > 0 && nBytes <= ChunkSize);
  sVERIFY((CurrentPos & (ChunkSize-1)) == 0);

  StartWrite(OutBuffer+LenBytes);
  if(!isLast)
    PutBits(0,2); // 00=normal, compressed
  else
  {
    PutBits(1,2); // 01=last, compressed
    PutExpGolomb(nBytes); // nBytes can be 0!
  }

  sInt pos = CurrentPos;
  sInt endPos = CurrentPos+nBytes;
  while(pos<endPos)
  {
    sInt hash = (ChunkBuffer[(pos-2)&WrapMask]<<8) + ChunkBuffer[(pos-1)&WrapMask];
    sU32 matchPos = HashTable[hash];
    HashTable[hash] = pos;

    if(sU32(pos - matchPos) < sU32(ChunkSize)) // match, not too distant
    {
      // determine match len
      sInt len = 0, maxLen = endPos-pos-1;
      while(len<maxLen && ChunkBuffer[(matchPos+len) & WrapMask] == ChunkBuffer[(pos+len) & WrapMask])
        len++;

      // code match len
      PutExpGolomb(len);
      pos += len;
    }

    // code a literal
    PutByte(ChunkBuffer[pos&WrapMask]);
    pos++;
  }

  // did that actually compress? if not, just use an uncompressed block instead.
  if(sU32(RawPos-OutBuffer) > nBytes)
  {
    StartWrite(OutBuffer + LenBytes); // start over
    if(!isLast)
      PutBits(2,2); // 10=normal, uncompressed
    else
    {
      PutBits(3,2); // 11=last, uncompressed
      PutExpGolomb(nBytes);
    }

    sCopyMem(RawPos,&ChunkBuffer[CurrentPos&WrapMask],nBytes);

    // need to update hash table as well
    sInt hash = (ChunkBuffer[(CurrentPos-2)&WrapMask]<<8) + ChunkBuffer[(CurrentPos-1)&WrapMask];
    for(sInt pos=CurrentPos;pos<endPos;pos++)
    {
      HashTable[hash] = pos;
      hash = ((hash<<8) + ChunkBuffer[pos&WrapMask]) & 0xffff;
    }

    RawPos += nBytes;
  }

  EndWrite();

  // write size of block
  sVERIFY(RawPos - OutBuffer <= (sDInt) MaxOutSize);
  sU32 size = RawPos - (OutBuffer+LenBytes);
  OutBuffer[0] = (size >>  0) & 0xff;
  OutBuffer[1] = (size >>  8) & 0xff;
  OutBuffer[2] = (size >> 16) & 0xff;

  CurrentPos += nBytes;
  
  // clean up the hash table periodically
  if(!isLast && CurrentPos >= PeriodicUpdate+ChunkSize)
  {
    for(sU32 i=0;i<HashSize;i++)
      HashTable[i] = (sU32) sMin<sInt>(HashTable[i]-PeriodicUpdate,-sInt(ChunkSize));

    CurrentPos -= PeriodicUpdate;
  }

  return RawPos - OutBuffer;
}

void sFastLzpCompressor::Reset()
{
  CurrentPos = 0;
  sSetMem(ChunkBuffer,0,ChunkSize*2);
  for(sU32 i=0;i<HashSize;i++)
    HashTable[i] = (sU32) -sInt(ChunkSize);
}

sFastLzpCompressor::sFastLzpCompressor()
{
  HashTable = new sU32[HashSize];
  ChunkBuffer = new sU8[ChunkSize*2];
  OutBuffer = new sU8[MaxOutSize];

  PWritePos = ~0u;
}

sFastLzpCompressor::~sFastLzpCompressor()
{
  sVERIFY(PWritePos == ~0u);

  delete[] HashTable;
  delete[] ChunkBuffer;
  delete[] OutBuffer;
}

sBool sFastLzpCompressor::Compress(sFile *dest,sFile *src)
{
  sVERIFY(PWritePos == ~0u); // no piecewise I/O in progress

  sS64 size = src->GetSize();
  Reset();

  while(size)
  {
    sInt nBytes = sMin<sS64>(size,ChunkSize);
    if(!src->Read(&ChunkBuffer[(CurrentPos & WrapMask)],nBytes))
      return sFALSE;

    sInt outBytes = CompressChunk(nBytes,size==nBytes);
    if(!dest->Write(OutBuffer,outBytes))
      return sFALSE;

    size -= nBytes;
  }

  return sTRUE;
}

void sFastLzpCompressor::StartPiecewise()
{
  sVERIFY(PWritePos == ~0u); // no piecewise I/O in progress
  
  Reset();
  PWritePos = 0;
  PFirstBlock = sTRUE;
}

sBool sFastLzpCompressor::WritePiecewise(sFile *dest,const void *buffer,sDInt size)
{
  sVERIFY(PWritePos != ~0u);

  const sU8 *bufPtr = (const sU8 *) buffer;
  while(size > 0)
  {
    if(!PFirstBlock && (PWritePos & (ChunkSize-1)) == 0) // this chunk is full
    {
      sInt outBytes = CompressChunk(ChunkSize,sFALSE);
      if(!dest->Write(OutBuffer,outBytes))
        return sFALSE;
    }

    sInt nextBoundary = (PWritePos < ChunkSize) ? ChunkSize : 2*ChunkSize;
    sInt blockSize = sMin<sDInt>(size,nextBoundary-PWritePos);

    sCopyMem(&ChunkBuffer[PWritePos],bufPtr,blockSize);
    bufPtr += blockSize;
    size -= blockSize;
    PWritePos = (PWritePos + blockSize) & WrapMask;
    PFirstBlock = sFALSE;
  }

  return sTRUE;
}

sBool sFastLzpCompressor::EndPiecewise(sFile *dest)
{
  if(PWritePos == 0 && PFirstBlock) // nothing was written
  {
    PWritePos = ~0u;
    return sTRUE;
  }

  sU32 cpm = CurrentPos & WrapMask;
  sU32 size = PWritePos > cpm ? (PWritePos - cpm) : (PWritePos - cpm + 2*ChunkSize);
  sVERIFY(size <= ChunkSize);
  
  sInt outBytes = CompressChunk(size,sTRUE);
  PWritePos = ~0u;
  return dest->Write(OutBuffer,outBytes);
}

/****************************************************************************/

void sFastLzpDecompressor::StartRead(const sU8 *ptr)
{
  sUnalignedLittleEndianLoad32(ptr,BitBuffer);
  BitFill = 32;
  RawPos = ptr + 4;
}

void sFastLzpDecompressor::Shift()
{
  sU16 v;
  sUnalignedLittleEndianLoad16(RawPos,v);
  RawPos += 2;
  BitBuffer |= v << (16 - BitFill);
  BitFill += 16;
}

sINLINE sU32 sFastLzpDecompressor::GetExpGolomb()
{
  sInt first5 = PeekBits(5);
  if(first5 >= 4) // really fast path (4=00100)
  {
    sU32 val = DirectV[first5][0];
    SkipBits(DirectV[first5][1]);
    return val;
  }
  else if(first5) // fast path
  {
    sInt nZero = LZCount[first5];
    SkipBits(nZero);
    return GetBits(nZero+1)-1;
  }
  else
  {
    sVERIFYSTATIC(ChunkBits <= 20);
    SkipBits(5);

    sInt nZero = 0;
    sInt next16 = PeekBits(16);
    if(!next16)
      return ~0u;

    if(next16 & 0xff00) next16 >>= 8; else nZero += 8;
    if(next16 & 0x00f0) next16 >>= 4; else nZero += 4;
    nZero += LZCount[next16]-1;
    SkipBits(nZero);
    return GetBits(5+nZero+1)-1;
  }
}

sInt sFastLzpDecompressor::DecompressChunk(sU32 blockLen,sBool &last,sU8 *&ptr)
{
  // call after the chunk has been read+block length consumed
  // read header.
  StartRead(InBuffer);

  sInt flags = GetBits(2);
  last = (flags & 1) == 1;
  sU32 nBytes = last ? GetExpGolomb() : ChunkSize;

  if(nBytes > ChunkSize || nBytes && (CurrentPos & (ChunkSize-1)) != 0)
    return -1; // error

  ptr = &ChunkBuffer[CurrentPos&WrapMask];

  if(flags & 2) // uncompressed
  {
    // uncompressed block; copy, but keep hash table updated
    const sU8 *raw = RawPos;
    sInt hash = (ChunkBuffer[(CurrentPos-2)&WrapMask]<<8) + ChunkBuffer[(CurrentPos-1)&WrapMask];
    for(sU32 pos=CurrentPos;pos<CurrentPos+nBytes;pos++)
    {
      HashTable[hash] = pos;
      sU8 byte = *raw++;
      ChunkBuffer[pos&WrapMask] = byte;
      hash = ((hash<<8) + byte) & 0xffff;
    }

    RawPos = raw;
  }
  else // compressed
  {
    const sU8 *endRaw = InBuffer + blockLen;

    sInt pos = CurrentPos;
    sInt endPos = CurrentPos+nBytes;
    while(pos<endPos)
    {
      sInt hash = (ChunkBuffer[(pos-2)&WrapMask]<<8) + ChunkBuffer[(pos-1)&WrapMask];
      sU32 matchPos = HashTable[hash];
      HashTable[hash] = pos;

      if(pos - matchPos < ChunkSize) // match, not too distant
      {
        sU32 len = GetExpGolomb();
        if(len >= ChunkSize || sInt(pos+len+1) > endPos)
          return -1;

        // copy loop (could probably be done faster. check average iteration count!)
        for(sU32 i=0;i!=len;i++)
          ChunkBuffer[(pos+i)&WrapMask] = ChunkBuffer[(matchPos+i)&WrapMask];

        pos += len;
      }

      // get literal
      ChunkBuffer[pos&WrapMask] = GetByte();
      pos++;

      if(RawPos > endRaw) // todo:check how many bits we can consume per iteration
        return -1;
    }
  }

  CurrentPos += nBytes;
  sVERIFY(RawPos - InBuffer <= (sDInt) blockLen);
  if(RawPos - InBuffer != (sDInt) blockLen) // wrong number of bytes depacked
    return -1;

  // clean up the hash table periodically
  if(!last && CurrentPos >= PeriodicUpdate+ChunkSize)
  {
    for(sU32 i=0;i<HashSize;i++)
      HashTable[i] = (sU32) sMin<sInt>(HashTable[i]-PeriodicUpdate,-sInt(ChunkSize));

    CurrentPos -= PeriodicUpdate;
  }

  return nBytes;
}

void sFastLzpDecompressor::Reset()
{
  CurrentPos = 0;
  sSetMem(ChunkBuffer,0,ChunkSize*2);
  for(sU32 i=0;i<HashSize;i++)
    HashTable[i] = (sU32) -sInt(ChunkSize);
}

sU32 sFastLzpDecompressor::ReadLen(const sU8 *ptr)
{
  return ptr[0] + (ptr[1] << 8) + (ptr[2] << 16);
}

sFastLzpDecompressor::sFastLzpDecompressor()
{
  HashTable = new sU32[HashSize];
  ChunkBuffer = new sU8[ChunkSize*2];
  InBuffer = new sU8[MaxOutSize];

  PReadPos = ~0u;
}

sFastLzpDecompressor::~sFastLzpDecompressor()
{
  delete[] HashTable;
  delete[] ChunkBuffer;
  delete[] InBuffer;
}

sBool sFastLzpDecompressor::Decompress(sFile *dest,sFile *src)
{
  sVERIFY(PReadPos == ~0u);
  sBool last = sFALSE;
  sU8 *ptr;

  Reset();

  while(!last)
  {
    // read block size
    if(!src->Read(InBuffer,3))
      return sFALSE;

    sU32 len = ReadLen(InBuffer);
    if(len > ChunkSize+8 || !src->Read(InBuffer,len))
      return sFALSE;

    sInt deLen = DecompressChunk(len,last,ptr);
    if(deLen == -1)
      return sFALSE;

    if(deLen && !dest->Write(ptr,deLen))
      return sFALSE;
  }

  return sTRUE;
}

void sFastLzpDecompressor::StartPiecewise()
{
  sVERIFY(PReadPos == ~0u);
  Reset();

  PReadPos = 0;
  PBlockEnd = 0;
  PIsLast = sFALSE;
}

sBool sFastLzpDecompressor::ReadPiecewise(sFile *src,void *buffer,sDInt size)
{
  sVERIFY(PReadPos != ~0u);
  sU8 *bufPtr = (sU8 *) buffer;

  while(size > 0)
  {
    if(PReadPos == PBlockEnd) // need to read new block
    {
      if(PIsLast) // was the last block, nothing more to read!
        return sFALSE;

      sVERIFY(PReadPos == (CurrentPos & WrapMask));

      // read block size
      if(!src->Read(InBuffer,3))
        return sFALSE;

      sU32 compLen = ReadLen(InBuffer);
      if(compLen > ChunkSize+8 || !src->Read(InBuffer,compLen))
        return sFALSE;

      // try to decompress
      sU8 *ptr = 0;
      sInt deLen = DecompressChunk(compLen,PIsLast,ptr);
      if(deLen == -1)
        return sFALSE;

      PBlockEnd = (PReadPos + deLen) & WrapMask;
    }

    sInt nextBoundary = (PReadPos < ChunkSize) ? ChunkSize : 2*ChunkSize;
    sInt blockSize = sMin<sDInt>(size,nextBoundary-PReadPos);

    sCopyMem(bufPtr,&ChunkBuffer[PReadPos],blockSize);
    bufPtr += blockSize;
    size -= blockSize;
    PReadPos = (PReadPos + blockSize) & WrapMask;
  }

  return sTRUE;
}

void sFastLzpDecompressor::EndPiecewise()
{
  sVERIFY(PReadPos != ~0u);
  PReadPos = ~0u;
}

/****************************************************************************/

sFastLzpFile::sFastLzpFile()
{
  Comp = 0;
  Decomp = 0;
  Host = 0;
  Size = 0;
}

sFastLzpFile::~sFastLzpFile()
{
  Close();
}

sBool sFastLzpFile::Open(sFile *host,sBool writing)
{
  Close();
  if(host == 0)
    return sFALSE;

  Host = host;
  if(writing)
  {
    // store magic and null size tag in front
    sU8 buffer[16];
    sClear(buffer);
    sCopyMem(buffer,"FastLZP",8);
    if(!Host->Write(buffer,16))
    {
      sDelete(Host);
      return sFALSE;
    }

    Size = 0;
    Comp = new sFastLzpCompressor;
    Comp->StartPiecewise();
  }
  else
  {
    // read magic and size tag
    sU8 buffer[16];
    if(!Host->Read(buffer,16) || sCmpMem(buffer,"FastLZP",8)!=0)
    {
      sDelete(Host);
      return sFALSE;
    }

    sUnalignedLittleEndianLoad64(buffer+8,(sU64&)Size);
    Decomp = new sFastLzpDecompressor;
    Decomp->StartPiecewise();
  }

  return sTRUE;
}

sFile *sFastLzpFile::OpenRead(sFile *host)
{
  sFastLzpFile *lzp = new sFastLzpFile;
  if(!lzp->Open(host,sFALSE))
    sDelete(lzp);

  return lzp;
}

sFile *sFastLzpFile::OpenWrite(sFile *host)
{
  sFastLzpFile *lzp = new sFastLzpFile;
  if(!lzp->Open(host,sTRUE))
    sDelete(lzp);

  return lzp;
}

sBool sFastLzpFile::Close()
{
  sBool ret = sTRUE;

  if(Comp) // if compressing, store size tag
  {
    Comp->EndPiecewise(Host);

    sU8 buffer[8];
    sUnalignedLittleEndianStore64(buffer,Size);
    if(!Host || !Host->SetOffset(8) || !Host->Write(buffer,8))
      ret = sFALSE;

    delete Comp;
    Comp = 0;
  }

  if(Decomp)
  {
    Decomp->EndPiecewise();
    delete Decomp;
    Decomp = 0;
  }
  
  sDelete(Host);
  Size = 0;

  return ret;
}

sBool sFastLzpFile::Read(void *data,sDInt size)
{
  sVERIFY(Host && Decomp);
  return Decomp->ReadPiecewise(Host,data,size);
}

sBool sFastLzpFile::Write(const void *data,sDInt size)
{
  sVERIFY(Host && Comp);
  sBool ret = Comp->WritePiecewise(Host,data,size);
  Size += ret ? size : 0;
  return ret;
}

sS64 sFastLzpFile::GetSize()
{
  return Size;
}

/****************************************************************************/

