// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __DIS_HPP__
#define __DIS_HPP__

#include "_types.hpp"

/****************************************************************************/

#define NBUFFERS  22

struct DataBuffer
{
  sInt Size,Max;
  sU8 *Data;

  DataBuffer();
  ~DataBuffer();
  void Clear();
  void Append(sU8 *data,sInt size);
  void PutValue(sU32 value,sInt size);
};

struct CoreI
{
  sU16 Index;
  sU8 Len;
  sU8 Bytes[5];
  sInt Freq;
  CoreI *Next;

  CoreI()                                   { Clear(); }
  void Clear();
  void AddByte(sU8 val)                     { sVERIFY(Len<5); Bytes[Len++] = val; }
};

struct CoreInstructionSet
{
  enum
  {
    BUCKET_COUNT = 1024,
    BUCKET_MASK  = BUCKET_COUNT - 1
  };

  CoreI *Bucket[BUCKET_COUNT];

  static sInt Hash(const CoreI &instr);
  static bool CompareInstrs(const CoreI *a,const CoreI *b);

public:
  CoreI **InstrList;
  sInt InstrCount;

  CoreInstructionSet();
  ~CoreInstructionSet();

  void Clear();
  CoreI *FindInstr(const CoreI &instr);
  void CountInstr(const CoreI &instr);

  void MakeInstrList();
};

class DisFilter
{
  DataBuffer Buffer[NBUFFERS];
  sInt FuncTablePos;
  sU32 FuncTable[255];
  sBool NextFunc;
  sU32 LastJump;
  class DebugInfo *Info;
  CoreInstructionSet Instrs;

  sInt CountInstr(sU8 *instr);
  sInt ProcessInstr(sU8 *instr,sU32 memory,sU32 VA);

public:
  DataBuffer Output;
  sU8 Table[256];

  void Filter(sU8 *code,sInt size,sU32 VA,DebugInfo *info=0);
};

void DisUnFilter(sU8 *packed,sU8 *dest,sU32 oldAddr,sU32 newAddr,class ReorderBuffer &reord);

/****************************************************************************/

#endif
