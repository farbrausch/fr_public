/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) 2005-2006 by Dierk Ohlerich                          ***/
/***   all rights reserved                                                ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_UTIL_BITIO_HPP
#define FILE_UTIL_BITIO_HPP

#include "base/types.hpp"

/****************************************************************************/

class sFile;

class sBitWriter
{
  sFile *File;

  sU8 *Buffer;
  sU8 *BufferPtr,*BufferEnd;
  sU32 BitBuffer;
  sInt BitsLeft;

  sDInt Written;
  sBool Error;

  void FlushBuffer(sBool finish=sFALSE);

  sINLINE void PutByte(sU8 byte)
  {
    if(BufferPtr == BufferEnd)
      FlushBuffer();

    *BufferPtr++ = byte;
    Written++;
  }

public:
  sBitWriter();
  ~sBitWriter();

  void Start(sU8 *outBuffer,sDInt bufferSize);  // write to memory
  void Start(sFile *outFile);                   // write to file
  sDInt Finish(); // returns number of bytes written

  sBool IsOk() const { return !Error; }

  sINLINE void PutBits(sU32 bits,sInt count) // never write more than 24 bits at once!
  {
#if sCONFIG_BUILD_DEBUG // yes, not sDEBUG - only check in actual debug builds.
    sVERIFY(count <= 24);
    sVERIFY(bits < (1u << count));
#endif

    BitsLeft -= count;
    BitBuffer |= bits << BitsLeft;

    while(BitsLeft <= 24)
    {
      PutByte((sU8) (BitBuffer >> 24));
      BitBuffer <<= 8;
      BitsLeft += 8;
    }
  }
};

/****************************************************************************/

class sBitReader
{
  friend class sLocalBitReader;

  sFile *File;
  sS64 FileSize;

  sU8 *Buffer;
  sU8 *BufferPtr,*BufferEnd;
  sU32 BitBuffer;
  sInt BitsLeft;

  sInt ExtraBytes;
  sBool Error;

  void RefillBuffer();

  sINLINE sU8 GetByte()
  {
    if(BufferPtr == BufferEnd)
      RefillBuffer();

    return *BufferPtr++;
  }

public:
  sBitReader();
  ~sBitReader();

  void Start(const sU8 *buffer,sDInt size);   // read from memory
  void Start(sFile *file);                    // read from file
  sBool Finish();

  sBool IsOk() { return !Error && (ExtraBytes < 4 || BitsLeft == 32); }

  sINLINE void SkipBits(sInt count)
  {
#if sCONFIG_BUILD_DEBUG
    sVERIFY(count <= 24);
#endif

    BitBuffer <<= count;
    BitsLeft -= count;

    while(BitsLeft <= 24)
    {
      BitsLeft += 8;
      BitBuffer |= GetByte() << (32 - BitsLeft);
    }
  }

  // In case of error, PeekBits/GetBits always pad with zero bits - design your codes accordingly!
  sINLINE sU32 PeekBits(sInt count)   { return BitBuffer >> (32 - count); }
  sINLINE sS32 PeekBitsS(sInt count)  { return sS32(BitBuffer) >> (32 - count); }

  sINLINE sU32 GetBits(sInt count)    { sU32 r = PeekBits(count); SkipBits(count); return r; }
  sINLINE sS32 GetBitsS(sInt count)   { sS32 r = PeekBitsS(count); SkipBits(count); return r; }
};

// sLocalBitReader is the same as sBitReader; use for "local" copies in inner loops, only
// create instances as local variables, and never take any pointers or references to them.
// this should help a lot on platforms with high load-store forwarding penalties (e.g. powerpc).
class sLocalBitReader
{
  sBitReader &Parent;
  sU8 *BufferPtr,*BufferEnd;
  sU32 BitBuffer;
  sInt BitsLeft;

  sINLINE sU8 GetByte()
  {
    if(BufferPtr == BufferEnd)
    {
      Parent.BufferPtr = BufferPtr;
      Parent.RefillBuffer();
      BufferPtr = Parent.BufferPtr;
      BufferEnd = Parent.BufferEnd;
    }

    return *BufferPtr++;
  }

public:
  sLocalBitReader(sBitReader &dad) : Parent(dad)
  {
    BufferPtr = dad.BufferPtr;
    BufferEnd = dad.BufferEnd;
    BitBuffer = dad.BitBuffer;
    BitsLeft = dad.BitsLeft;
  }

  ~sLocalBitReader()
  {
    Parent.BufferPtr = BufferPtr;
    Parent.BitBuffer = BitBuffer;
    Parent.BitsLeft = BitsLeft;
  }

  sINLINE void SkipBits(sInt count)
  {
#if sCONFIG_BUILD_DEBUG
    sVERIFY(count <= 24);
#endif

    BitBuffer <<= count;
    BitsLeft -= count;

    while(BitsLeft <= 24)
    {
      BitsLeft += 8;
      BitBuffer |= GetByte() << (32 - BitsLeft);
    }
  }

  // In case of error, PeekBits/GetBits always pad with zero bits - design your codes accordingly!
  sINLINE sU32 PeekBits(sInt count)   { return BitBuffer >> (32 - count); }
  sINLINE sS32 PeekBitsS(sInt count)  { return sS32(BitBuffer) >> (32 - count); }

  sINLINE sU32 GetBits(sInt count)    { sU32 r = PeekBits(count); SkipBits(count); return r; }
  sINLINE sS32 GetBitsS(sInt count)   { sS32 r = PeekBitsS(count); SkipBits(count); return r; }
};

/****************************************************************************/
/***                                                                      ***/
/***   Huffman coding                                                     ***/
/***                                                                      ***/
/****************************************************************************/

// Input:   List of frequencies for <count> symbols (0=unused).
// Output:  Length of huffman code for each symbol; all lengths are <=maxLen.
void sBuildHuffmanCodeLens(sInt *lens,const sU32 *freq,sInt count,sInt maxLen);

// Turns the lengths from sBuildHuffmanCodeLens into the actual codes (for encoding).
void sBuildHuffmanCodeValues(sU32 *codes,const sInt *lens,sInt count);

// Full service
void sBuildHuffmanCodes(sU32 *codes,sInt *lens,const sU32 *freq,sInt count,sInt maxLen);

// Encode huffman code lengths (when you need to store them)
sBool sWriteHuffmanCodeLens(sBitWriter &writer,const sInt *lens,sInt count);

// Decode huffman code lenghts written with above function
sBool sReadHuffmanCodeLens(sBitReader &reader,sInt *lens,sInt count);

class sFastHuffmanDecoder
{
  static const sInt FastBits = 8; // MUST be <16!

  sU16 FastPath[1<<FastBits];
  sU32 MaxCode[26];
  sInt Delta[25];
  sU16 *CodeMap;

public:
  sFastHuffmanDecoder();
  ~sFastHuffmanDecoder();

  sBool Init(const sInt *lens,sInt count);

  // DecodeSymbol for sBitReader and sLocalBitReader do exactly the same thing.
  // Use the second variant where speed is critical (and where you're presumably using
  // sLocalBitReader anyway) and the second otherwise.
  sInt DecodeSymbol(sBitReader &reader);

  sINLINE sInt DecodeSymbol(sLocalBitReader &reader)
  {
    sU32 peek = reader.PeekBits(FastBits);
    sU16 fast = FastPath[peek];

    if(fast & 0xf000) // valid entry
    {
      reader.SkipBits(fast >> 12);
      return fast & 0xfff;
    }

    // can't use fast path (code is too long); first, find actual code length
    peek = reader.PeekBits(24);
    sInt len = FastBits+1;
    while(peek >= MaxCode[len])
      len++;

    if(len == 25) // code not found
      return -1;

    // return the coded symbol
    return CodeMap[reader.GetBits(len) + Delta[len]];
  }
};

/****************************************************************************/

#endif // FILE_UTIL_BITIO_HPP

