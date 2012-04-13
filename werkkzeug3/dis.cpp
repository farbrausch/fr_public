// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_types.hpp"
#include "dis.hpp"
#include "mapfile.hpp"
#include <stdlib.h>
#include <algorithm>

/****************************************************************************/
/****************************************************************************/

// formats

#define fNM       0x0     // no modrm
#define fAM       0x1     // no modrm, address mode
#define fMR       0x2     // modrm
#define fMO       0x3     // modrm+xtra opcode
#define fMODE     0x3     // mode mask

#define fNI       0x0     // no immediate
#define fBI       0x4     // byte immediate
#define fDI       0x8     // dword immediate
#define fWI       0xc     // word immediate
#define fTYPE     0xc     // type mask

#define fAD       0x0     // address
#define fBR       0x4     // byte relative
#define fDR       0xc     // dword relative

#define fERR      0x9     // error!

/****************************************************************************/

static sU8 Table0[256] =
{
  // 0       1       2       3       4       5       6       7       8       9       a       b       c       d       e       f
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fNM|fBI,fNM|fDI,fNM|fNI,fNM|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fNM|fBI,fNM|fDI,fNM|fNI,fNM|fNI, // 0
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fNM|fBI,fNM|fDI,fNM|fNI,fNM|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fNM|fBI,fNM|fDI,fNM|fNI,fNM|fNI, // 1
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fNM|fBI,fNM|fDI,fNM|fNI,fNM|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fNM|fBI,fNM|fDI,fNM|fNI,fNM|fNI, // 2
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fNM|fBI,fNM|fDI,fNM|fNI,fNM|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fNM|fBI,fNM|fDI,fNM|fNI,fNM|fNI, // 3

  fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI, // 4
  fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI, // 5
  fNM|fNI,fNM|fNI,fMR|fNI,fMR|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fDI,fMR|fDI,fNM|fBI,fMR|fBI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI, // 6
  fAM|fBR,fAM|fBR,fAM|fBR,fAM|fBR,fAM|fBR,fAM|fBR,fAM|fBR,fAM|fBR,fAM|fBR,fAM|fBR,fAM|fBR,fAM|fBR,fAM|fBR,fAM|fBR,fAM|fBR,fAM|fBR, // 7

  fMR|fBI,fMR|fDI,fMR|fBI,fMR|fBI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI, // 8
  fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fERR   ,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI, // 9
  fAM|fAD,fAM|fAD,fAM|fAD,fAM|fAD,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fBI,fNM|fDI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI, // a
  fNM|fBI,fNM|fBI,fNM|fBI,fNM|fBI,fNM|fBI,fNM|fBI,fNM|fBI,fNM|fBI,fNM|fDI,fNM|fDI,fNM|fDI,fNM|fDI,fNM|fDI,fNM|fDI,fNM|fDI,fNM|fDI, // b

  fMR|fBI,fMR|fBI,fNM|fWI,fNM|fNI,fMR|fNI,fMR|fNI,fMR|fBI,fMR|fDI,fERR   ,fNM|fNI,fNM|fWI,fNM|fNI,fNM|fNI,fNM|fBI,fERR   ,fNM|fNI, // c
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fNM|fBI,fNM|fBI,fNM|fNI,fNM|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI, // d
  fAM|fBR,fAM|fBR,fAM|fBR,fAM|fBR,fNM|fBI,fNM|fBI,fNM|fBI,fNM|fBI,fAM|fDR,fAM|fDR,fAM|fAD,fAM|fBR,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI, // e
  fNM|fNI,fERR   ,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fMO|fNI,fMO|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fMO|fNI,fMO|fNI, // f
};

/****************************************************************************/

static sU8 Table0f[256] =
{
  // 0       1       2       3       4       5       6       7       8       9       a       b       c       d       e       f
  fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // 0
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // 1
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // 2
  fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // 3

  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI, // 4
  fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // 5
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI, // 6
  fMR|fBI,fMR|fBI,fMR|fBI,fMR|fBI,fMR|fNI,fMR|fNI,fMR|fNI,fNM|fNI,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fMR|fNI,fMR|fNI, // 7

  fAM|fDR,fAM|fDR,fAM|fDR,fAM|fDR,fAM|fDR,fAM|fDR,fAM|fDR,fAM|fDR,fAM|fDR,fAM|fDR,fAM|fDR,fAM|fDR,fAM|fDR,fAM|fDR,fAM|fDR,fAM|fDR, // 8
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI, // 9
  fNM|fNI,fNM|fNI,fNM|fNI,fMR|fNI,fMR|fBI,fMR|fNI,fMR|fNI,fMR|fNI,fERR   ,fERR   ,fERR   ,fMR|fNI,fMR|fBI,fMR|fNI,fERR   ,fMR|fNI, // a
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fERR   ,fERR   ,fERR   ,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI, // b

  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI, // c
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI, // d
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI, // e
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fERR   , // f
};

/****************************************************************************/

static sU8 Tablefx[32] =
{
  // 0       1       2       3       4       5       6       7       8       9       a       b       c       d       e       f
  fMR|fBI,fERR   ,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fBI,fERR   ,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI, // 0
  fMR|fNI,fMR|fNI,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fMR|fNI,fMR|fNI,fMR|fNI,fERR   ,fMR|fNI,fERR   ,fMR|fNI,fERR   , // 1
};

// caaaaareful with this table (match disdeck!)

/****************************************************************************/
/****************************************************************************/

DataBuffer::DataBuffer()
{
  Size = 0;
  Max = 16;
  Data = (sU8 *) malloc(Max);
}

DataBuffer::~DataBuffer()
{
  free(Data);
}

void DataBuffer::Clear()
{
  Size = 0;
}

void DataBuffer::Append(sU8 *data,sInt size)
{
  if(Size+size>Max)
  {
    Max = (Max*2 < Size+size) ? Size+size : Max*2;
    Data = (sU8 *) realloc(Data,Max);
  }

  sCopyMem(Data+Size,data,size);
  Size += size;
}

void DataBuffer::PutValue(sU32 value,sInt size)
{
  Append((sU8*) &value,size);
}

/****************************************************************************/

void CoreI::Clear()
{
  Len = 0;
  Index = 255;
  sSetMem(Bytes,0,sizeof(Bytes));
  Freq = 0;
  Next = 0;
}

/****************************************************************************/

CoreInstructionSet::CoreInstructionSet()
{
  for(sInt i=0;i<BUCKET_COUNT;i++)
    Bucket[i] = 0;

  InstrCount = 0;
  InstrList = 0;
}

CoreInstructionSet::~CoreInstructionSet()
{
  Clear();
}

void CoreInstructionSet::Clear()
{
  // delete all hash chains
  for(sInt i=0;i<BUCKET_COUNT;i++)
  {
    CoreI *current,*next;

    for(current=Bucket[i];current;current=next)
    {
      next = current->Next;
      delete current;
    }

    Bucket[i] = 0;
  }

  InstrCount = 0;
  if(InstrList)
  {
    delete[] InstrList;
    InstrList = 0;
  }
}

CoreI *CoreInstructionSet::FindInstr(const CoreI &instr)
{
  // determine hash chain and try to find a match
  sInt bucket = Hash(instr);
  CoreI *match;

  for(match=Bucket[bucket];match;match=match->Next)
    if(match->Len == instr.Len && !sCmpMem(match->Bytes,instr.Bytes,instr.Len))
      break;

  return match;
}

void CoreInstructionSet::CountInstr(const CoreI &instr)
{
  CoreI *match = FindInstr(instr);

  if(match) // got one
    match->Freq++;
  else // no match, add a new instr
  {
    CoreI *instCopy = new CoreI(instr);
    sInt bucket = Hash(instr);

    instCopy->Freq = 1;
    instCopy->Index = 65535;
    instCopy->Next = Bucket[bucket];
    Bucket[bucket] = instCopy;

    InstrCount++;
  }
}

void CoreInstructionSet::MakeInstrList()
{
  // Make a list of instructions
  InstrList = new CoreI *[InstrCount];
  sInt n = 0;
  for(sInt i=0;i<BUCKET_COUNT;i++)
    for(CoreI *current=Bucket[i];current;current=current->Next)
      InstrList[n++] = current;

  sVERIFY(n == InstrCount);

  // Sort + assign indices
  std::sort(InstrList,InstrList + InstrCount,CompareInstrs);
  for(sInt i=0;i<254+256 && i<InstrCount;i++)
    InstrList[i]->Index = i;
}

sInt CoreInstructionSet::Hash(const CoreI &instr)
{
  // FNV-style hash.
  sU32 hash = 0x9dc5 + instr.Len;
  hash = (hash ^ instr.Bytes[0]) * 0x0193;
  hash = (hash ^ instr.Bytes[1]) * 0x0193;
  hash = (hash ^ instr.Bytes[2]) * 0x0193;

  return hash & BUCKET_MASK;
}

bool CoreInstructionSet::CompareInstrs(const CoreI *a,const CoreI *b)
{
  if(a->Freq != b->Freq)
    return a->Freq > b->Freq;

  if(a->Len != b->Len)
    return a->Len < b->Len;

  return sCmpMem(a->Bytes,b->Bytes,a->Len) < 0;
}

/****************************************************************************/

sInt DisFilter::CountInstr(sU8 *instr)
{
  sInt code,code2,modrm,sib,flags;
  sBool o16;
  CoreI corei;
  
  sU8 *start = instr;
  code = *instr++;
  if(code == 0x66) // operand size prefix
  {
    o16 = sTRUE;
    code = *instr++;
  }
  else
    o16 = sFALSE;

  if(code == 0x0f) // two-byte opcode
  {
    code2 = *instr++;
    flags = Table0f[code2];
  }
  else
    flags = Table0[code];

  if((flags & fMODE) == fMO)
    flags = Tablefx[((*instr & 0x38) >> 3) | ((code & 0x01) << 3) | ((code & 0x08) << 1)];

  if(flags != fERR)
  {
    if(o16)
      corei.AddByte(0x66);

    corei.AddByte(code);
    if(code == 0x0f)
      corei.AddByte(code2);

    if(flags & fMR) // modrm
    {
      modrm = *instr++;
      corei.AddByte(modrm);

      // skip everything that may come after
      if((modrm & 0x07) == 4 && (modrm & 0xc0) != 0xc0)
        sib = *instr++;
      else
        sib = 0;

      if((modrm & 0xc0) == 0x40) // byte displacement
        instr++;

      if((modrm & 0xc0) == 0x80 || (modrm & 0xc7) == 0x05 || ((modrm & 0xc0) == 0 && (sib & 0x07) == 5))
        instr += 4;
    }

    Instrs.CountInstr(corei);
 
    // instruction counted, skip immediates that may follow
    if((flags & fMODE) == fAM)
    {
      switch(flags & fTYPE)
      {
      case fAD: instr += 4; break;
      case fBR: instr += 1; break;
      case fDR: instr += 4; break;
      }
    }
    else
    {
      switch(flags & fTYPE)
      {
      case fBI: instr += 1; break;
      case fDI: instr += o16 ? 2 : 4; break;
      case fWI: instr += 2; break;
      }
    }

    // return size
    return instr - start;
  }
  else
    return 1; // error: just skip one byte and try again
}

sInt DisFilter::ProcessInstr(sU8 *instr,sU32 memory,sU32 VA)
{
  sU8 *start;
  sU8 code,code2,modrm,sib,flags;
  sU32 val,tmp;
  sBool o16;
  sInt i;
  CoreI corei;

  o16 = sFALSE;
  modrm = 0;
  sib = 0;

  start = instr;
  code = *instr++;
  if(NextFunc && code != 0xcc)
  {
    FuncTable[FuncTablePos] = memory;
    if(++FuncTablePos == 255)
      FuncTablePos = 0;

    NextFunc = sFALSE;
  }

  if(code == 0x66) // operand size prefix
  {
    o16 = sTRUE;
    code = *instr++;
  }

  if(code == 0x0f)
  {
    code2 = *instr++;
    flags = Table0f[code2];
  }
  else
    flags = Table0[code];

  if(code == 0xc2 || code == 0xc3 || code == 0xcc) // return/int3
    NextFunc = sTRUE;

  if((flags & fMODE) == fMO)
    flags = Tablefx[((*instr & 0x38) >> 3) | ((code & 0x01) << 3) | ((code & 0x08) << 1)];

  if(flags != fERR)
  {
    if(o16)
      corei.AddByte(0x66);

    corei.AddByte(code);
    if(code == 0x0f)
      corei.AddByte(code2);
  }

  if((flags & fMODE) == fMR)
  {
    modrm = *instr++;
    corei.AddByte(modrm);

    if((modrm & 0x07) == 4 && (modrm & 0xc0) != 0xc0)
    {
      sib = *instr++;
      Buffer[21].PutValue(sib,1);
    }

    if((modrm & 0xc0) == 0x40)
      Buffer[(modrm & 0x07)+1].PutValue(*instr++,1);

    if((modrm & 0xc0) == 0x80 || (modrm & 0xc7) == 0x05 || ((modrm & 0xc0) == 0 && (sib & 0x07) == 5))
    {
      val = *(sU32 *) instr; instr += 4;
      Buffer[(modrm & 0xc7) == 5 ? 14 : 13].PutValue(val & 0xffffff,3);
      Buffer[18].PutValue(val >> 24,1);
    }
  }

  // core instruction is completed by here, so find and encode
  if(flags != fERR)
  {
    CoreI *InstCode = Instrs.FindInstr(corei);
    sVERIFY(InstCode != 0);

    if(InstCode->Index < 255)
      Buffer[0].PutValue(InstCode->Index,1);
    else
    {
      Buffer[0].PutValue(255,1); // not in top 255, escape
      Buffer[0].Append(corei.Bytes,corei.Len);
    }
  }

  if((flags & fMODE) == fAM)
  {
    switch(flags & fTYPE)
    {
    case fAD:
      val = *(sU32 *) instr; instr += 4;
      Buffer[15].PutValue(val & 0xffffff,3);
      Buffer[19].PutValue(val >> 24,1);
      break;

    case fBR:
      Buffer[9].PutValue(*instr++,1);
      break;

    case fDR:
      val = *(sU32 *) instr; instr += 4;
      val += (instr - start) + memory;
      if(code != 0xe8)
      {
        i = val - LastJump;
        tmp = (i < 0) ? -i * 2 - 1 : i * 2;
        Buffer[17].PutValue(tmp & 0xffff,2);
        Buffer[20].PutValue(tmp >> 16,2);
        LastJump = val;
      }
      else
      {
        for(i=0;i<255;i++)
          if(FuncTable[i] == val)
            break;

        Buffer[16].PutValue(i+1,1);
        if(i == 255)
        {
          Buffer[16].PutValue(val & 0xffff,2);
          Buffer[20].PutValue(val >> 16,2);

          FuncTable[FuncTablePos] = val;
          if(++FuncTablePos == 255)
            FuncTablePos = 0;
        }
      }
      break;
    }
  }
  else
  {
    switch(flags & fTYPE)
    {
    case fBI:
      Buffer[10].PutValue(*instr++,1);
      break;

    case fDI:
      if(!o16)
      {
        Buffer[12].Append(instr,4);
        instr += 4;
        break;
      }
      // fall-through

    case fWI:
      Buffer[11].Append(instr,2);
      instr += 2;
      break;
    }
  }

  if(flags == fERR)
  {
    Buffer[0].PutValue(0xff,1); // corei escape
    Buffer[0].PutValue(0xce,1); // escape
    Buffer[0].PutValue(*start,1); // byte to encode
    return 1;
  }
  else
    return instr - start;
}

void DisFilter::Filter(sU8 *code,sInt size,sU32 VA,DebugInfo *info)
{
  sU32 memory;
  sInt lastSize,i,processed,v;

  Instrs.Clear();

  // first pass: count instructions and instruction types
  sU8 *curCode = code;
  sInt curSize = size;

  while(curSize > 0)
  {
    lastSize = curSize;
    processed = CountInstr(curCode);
    curCode += processed;
    curSize -= processed;
  }

  Instrs.MakeInstrList();

  // second pass: actual encoding
  for(i=0;i<NBUFFERS;i++)
    Buffer[i].Clear();

  for(i=0;i<255;i++)
    FuncTable[i] = ~0U;

  FuncTablePos = 0;
  NextFunc = sTRUE;
  LastJump = 0;
  Info = info;
  memory = 0;

  sInt rest = lastSize;
  curSize = size - rest;
  curCode = code;

  // before encoding the real code, first encode corei list
  sInt nOps = sMin(Instrs.InstrCount,255);
  Buffer[0].PutValue(nOps,1);
  for(i=0;i<nOps;i++)
    Buffer[0].Append(Instrs.InstrList[i]->Bytes,Instrs.InstrList[i]->Len);

  while(curSize > 0)
  {
    processed = ProcessInstr(curCode,memory,VA);
    memory += processed;
    curCode += processed;
    curSize -= processed;
  }
  sVERIFY(curSize == 0);

  // encode the rest as escapes
  while(rest--)
  {
    Buffer[0].PutValue(0xff,1);       // corei escape
    Buffer[0].PutValue(0xce,1);       // escape
    Buffer[0].PutValue(*curCode++,1); // value
  }

  Output.Clear();
  for(i=0;i<NBUFFERS;i++)
    Output.PutValue(Buffer[i].Size,4);

  for(i=0;i<NBUFFERS;i++)
    Output.Append(Buffer[i].Data,Buffer[i].Size);

  for(i=0;i<512;i+=2)
  {
    if(i<256)
      v = Table0[i] | (Table0[i+1] << 4);
    else
      v = Table0f[i-256] | (Table0f[i-255] << 4);

    if((v & 0x0f) == fERR)  v &= 0xf0;
    if((v >> 4)   == fERR)  v &= 0x0f;

    Table[i/2] = v;
  }

  Instrs.Clear();
}

/****************************************************************************/

void DisUnFilter(sU8 *packed,sU8 *dest,sU32 oldAddr,sU32 newAddr,ReorderBuffer &reord)
{
  sU8 *buffer[NBUFFERS],*finish,*start,*opacked,*corei;
  sInt i,flags;
  sU8 code,code2,modrm,sib,coreind,corelen;
  sBool o16,nextFunc;
  sU32 val,lastJump,memory;
  sU32 funcTable[256];
  sU8 *instTable[255];
  sInt funcTablePos;
  sU8 *oldDest;

  oldDest = dest;
  opacked = packed;
  start = packed + NBUFFERS*4;
  for(i=0;i<NBUFFERS;i++)
  {
    val = *(sU32 *) packed; packed += 4;
    buffer[i] = start; start += val;
  }

  finish = buffer[1];
  funcTablePos = 1;
  lastJump = 0;
  memory = 0;
  nextFunc = sTRUE;

  // read instr table
  sInt nInstr = *buffer[0]++;
  sU8 *bptr = buffer[0];

  for(i=0;i<nInstr;i++)
  {
    sInt flags,code;

    instTable[i] = buffer[0];
    code = *buffer[0]++;

    if(code == 0x66) // operand size prefix?
      code = *buffer[0]++; // skip

    if(code == 0x0f) // two-byte instruction?
      flags = Table0f[*buffer[0]++];
    else
      flags = Table0[code];

    if(flags & fMR) // modrm byte?
      buffer[0]++;
  }
  
  while(buffer[0]<finish)
  {
    start = dest;

    coreind = *buffer[0]++;
    corei = (coreind == 255) ? buffer[0] : instTable[coreind];

    code = *corei++;
    corelen = 1;

    if(nextFunc && code != 0xcc)
    {
      funcTable[funcTablePos] = memory;
      if(++funcTablePos == 256)
        funcTablePos = 1;

      nextFunc = sFALSE;
    }

    if(code == 0xce) // escape
    {
      reord.Add(newAddr + buffer[0] - 1 - opacked,3,dest - oldDest + oldAddr,1);
      *dest++ = *corei++; // copy one byte
    }
    else
    {
      *dest++ = code;

      o16 = sFALSE;
      modrm = sib = 0;

      if(code == 0x66) // operand size prefix
      {
        o16 = sTRUE;
        code = *corei++;
        *dest++ = code;
        corelen++;
      }

      if(code == 0xc2 || code == 0xc3 || code == 0xcc) // return/int3
        nextFunc = sTRUE; // next opcode (probably) starts a new function

      if(code == 0x0f) // two-byte instruction
      {
        code2 = *corei++;
        *dest++ = code2;
        flags = Table0f[code2];
        corelen++;
      }
      else
        flags = Table0[code];

      if(flags & fMR)
      {
        modrm = *corei++;
        *dest++ = modrm;
        corelen++;

        if((flags & fMODE) == fMO)
        {
          flags = fMR|fNI;
          if(!(modrm & 0x38) && !(code & 8))
            flags += fBI;
        }

        if((modrm & 0x07) == 4 && (modrm & 0xc0) != 0xc0)
        {
          reord.Add(newAddr + buffer[21] - opacked,1,dest - oldDest + oldAddr,1);
          sib = *buffer[21]++;
          *dest++ = sib;
        }

        if((modrm & 0xc0) == 0x40)
        {
          reord.Add(newAddr + buffer[(modrm & 0x07) + 1] - opacked,1,dest - oldDest + oldAddr,1);
          *dest++ = *buffer[(modrm & 0x07) + 1]++;
        }

        if((modrm & 0xc0) == 0x80 || (modrm & 0xc7) == 0x05 || ((modrm & 0xc0) == 0x00 && (sib & 0x07) == 0x05))
        {
          i = (modrm & 0xc7) == 5 ? 14 : 13;

          reord.Add(newAddr + buffer[18] - opacked,1,dest - oldDest + oldAddr + 3,1);
          reord.Add(newAddr + buffer[i] - opacked,3,dest - oldDest + oldAddr,3);

          val = (*buffer[18]++) << 24;
          val |= (*(sU32 *) buffer[i]) & 0xffffff; buffer[i] += 3;
          *(sU32 *) dest = val; dest += 4;
        }
      }

      if(coreind != 255)
        reord.Add(newAddr + buffer[0] - 1 - opacked,1,start - oldDest + oldAddr,corelen);
      else
        reord.Add(newAddr + buffer[0] - 1 - opacked,corelen + 1,start - oldDest + oldAddr,corelen);

      if((flags & fMODE) == fAM)
      {
        switch(flags & fTYPE)
        {
        case fAD:
          reord.Add(newAddr + buffer[19] - opacked,1,dest - oldDest + oldAddr + 3,1);
          reord.Add(newAddr + buffer[15] - opacked,3,dest - oldDest + oldAddr,3);
          val = (*buffer[19]++) << 24;
          val |= (*(sU32 *) buffer[15]) & 0xffffff; buffer[15] += 3;
          *(sU32 *) dest = val; dest += 4;
          break;

        case fBR:
          reord.Add(newAddr + buffer[9] - opacked,1,dest - oldDest + oldAddr,1);
          *dest++ = *buffer[9]++;
          break;

        case fDR:
          if(code == 0xe8)
          {
            i = *buffer[16]++;
            if(i)
            {
              reord.Add(newAddr + buffer[16] - 1 - opacked,1,dest - oldDest + oldAddr,4);
              val = funcTable[i];
            }
            else
            {
              reord.Add(newAddr + buffer[16] - 1 - opacked,3,dest - oldDest + oldAddr,2);
              reord.Add(newAddr + buffer[20] - opacked,2,dest - oldDest + oldAddr + 2,2);
              val = *(sU16 *) buffer[16]; buffer[16] += 2;
              val |= (*(sU16 *) buffer[20]) << 16; buffer[20] += 2;

              funcTable[funcTablePos] = val;
              if(++funcTablePos == 256)
                funcTablePos = 1;
            }
          }
          else
          {
            reord.Add(newAddr + buffer[17] - opacked,2,dest - oldDest + oldAddr,2);
            reord.Add(newAddr + buffer[20] - opacked,2,dest - oldDest + oldAddr + 2,2);
            val = *(sU16 *) buffer[17]; buffer[17] += 2;
            val |= (*(sU16 *) buffer[20]) << 16; buffer[20] += 2;
            if(val & 1)
              val = ~(val >> 1);
            else
              val >>= 1;

            lastJump += val;
            val = lastJump;
          }

          val -= dest + 4 - start + memory;
          *(sU32 *) dest = val; dest += 4;
          break;
        }
      }
      else
      {
        switch(flags & fTYPE)
        {
        case fBI:
          reord.Add(newAddr + buffer[10] - opacked,1,dest - oldDest + oldAddr,1);
          *dest++ = *buffer[10]++;
          break;
          
        case fDI:
          if(!o16)
          {
            reord.Add(newAddr + buffer[12] - opacked,4,dest - oldDest + oldAddr,4);
            val = *(sU32 *) buffer[12];
            *(sU32 *) dest = val;
            dest += 4;
            buffer[12] += 4;
            break;
          }
          // fall-through

        case fWI:
          reord.Add(newAddr + buffer[11] - opacked,2,dest - oldDest + oldAddr,2);
          *(sU16 *) dest = *(sU16 *) buffer[11];
          dest += 2;
          buffer[11] += 2;
          break;
        }
      }
    }

    if(coreind == 255)
      buffer[0] = corei;

    memory += dest - start;
  }
}
