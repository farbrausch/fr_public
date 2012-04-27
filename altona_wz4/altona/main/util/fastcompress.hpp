/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) 2005-2006 by Dierk Ohlerich                          ***/
/***   all rights reserverd                                               ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_UTIL_FASTCOMPRESS_HPP
#define FILE_UTIL_FASTCOMPRESS_HPP

#include "base/types.hpp"
#include "base/types2.hpp"
#include "base/system.hpp"

/****************************************************************************/

// Very fast LZP-style compressor. Pack ratio is relatively low, but it's
// *really* quick. Right now, intended for PC only: uses a few hundred
// kilobytes of memory.
// Primary for large files (at least a few hundred kilobytes) because of
// relatively high setup overhead (both compressor+decompressor).
//
// "Very fast" means: 100-200 MB/sec (depending on how compressible the data
// is) on a Core2 2.83GHz for both compression and decompression.
class sFastLzpCompressor
{
  sU8 *ChunkBuffer;
  sU8 *OutBuffer;
  sU32 *HashTable;
  sU32 CurrentPos;

  sU8 *RawPos;
  sU8 *BitPos1,*BitPos2;
  sU32 BitBuffer;
  sInt BitShift;

  sU32 PWritePos;
  sBool PFirstBlock;

  void StartWrite(sU8 *ptr);
  void EndWrite();

  void Shift();

  // The BitPos1/BitPos2 stuff looks strange, I know. But it's all too make the
  // depacker very simple+fast.
  sINLINE void PutBits(sU32 value,sInt nBits) // nBits must be <=16!!
  {
    BitShift -= nBits;
    BitBuffer |= value << BitShift;

    if(BitShift <= 16)
      Shift();
  }

  sINLINE void PutBitsLong(sU32 value,sInt nBits) // nBits may be up to 32
  {
    if(nBits <= 16)
      PutBits(value,nBits);
    else
    {
      PutBits(value>>16,nBits-16);
      PutBits(value&0xffff,16);
    }
  }

  sINLINE void PutByte(sU8 value) { *RawPos++ = value; }
  void PutExpGolomb(sU32 value);

  sInt CompressChunk(sU32 nBytes,sBool isLast);
  void Reset();

public:
  sFastLzpCompressor();
  ~sFastLzpCompressor();

  // EITHER: Everything at once
  sBool Compress(sFile *dest,sFile *src);

  // OR: Piecewise I/O (normal write operations)
  void StartPiecewise();
  sBool WritePiecewise(sFile *dest,const void *buffer,sDInt size);
  sBool EndPiecewise(sFile *dest);
};

// The corresponding decompressor.
class sFastLzpDecompressor
{
  sU8 *ChunkBuffer;
  sU8 *InBuffer;
  sU32 *HashTable;
  sU32 CurrentPos;

  sU32 PReadPos;
  sU32 PBlockEnd;
  sBool PIsLast;

  const sU8 *RawPos;
  sU32 BitBuffer;
  sInt BitFill;

  void StartRead(const sU8 *ptr);

  void Shift();

  sINLINE sU32 PeekBits(sInt nBits) // nBits must be <=16!!
  {
    return BitBuffer >> (32 - nBits);
  }

  sINLINE void SkipBits(sInt nBits) // nBist must be <=16!!
  {
    BitBuffer <<= nBits;
    BitFill -= nBits;
    if(BitFill <= 16)
      Shift();
  }

  sINLINE sU32 GetBits(sInt nBits) // nBits must be <=16!!
  {
    sU32 v = PeekBits(nBits);
    SkipBits(nBits);
    return v;
  }

  sINLINE sU8 GetByte()
  {
    return *RawPos++;
  }

  sU32 GetExpGolomb();

  sInt DecompressChunk(sU32 blockLen,sBool &last,sU8 *&ptr);
  void Reset();

  sU32 ReadLen(const sU8* ptr);

public:
  sFastLzpDecompressor();
  ~sFastLzpDecompressor();

  // EITHER: Everything at once
  sBool Decompress(sFile *dest,sFile *src);

  // OR: Piecewise I/O (normal read operations)
  void StartPiecewise();
  sBool ReadPiecewise(sFile *src,void *buffer,sDInt size);
  void EndPiecewise();
};

/****************************************************************************/

// File wrappers (intended to be used with serialization)
class sFastLzpFile : public sFile
{
  sFastLzpCompressor *Comp;
  sFastLzpDecompressor *Decomp;
  sFile *Host;
  sS64 Size;

public:
  sFastLzpFile();
  virtual ~sFastLzpFile();

  // either open for reading or writing, not both. sFastLzpFile owns host.
  // it's freed immediately if Open fails!
  sBool Open(sFile *host,sBool writing); 

  static sFile *OpenRead(sFile *host);
  static sFile *OpenWrite(sFile *host);

  virtual sBool Close();
  virtual sBool Read(void *data,sDInt size);
  virtual sBool Write(const void *data,sDInt size);
  virtual sS64 GetSize();
};

/****************************************************************************/

#endif // FILE_UTIL_FASTCOMPRESS_HPP

