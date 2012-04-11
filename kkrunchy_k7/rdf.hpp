// Written by Fabian "ryg" Giesen.
// I hereby place this code in the public domain.

#ifndef __RDF_HPP__
#define __RDF_HPP__

#include "_types.hpp"

/****************************************************************************/

class RDFObject
{
  friend class RDFLinker;

  struct Reloc
  {
    sInt Segment,RefSegment;
    sInt Offset;
    sInt Length;
  };

  struct Import
  {
    sInt Segment;
    sChar Label[32];

    sU32 Address; // set by RDFLinker
  };

  struct Export
  {
    sInt Flags;
    sInt Segment;
    sInt Offset;
    sChar Label[32];
  };

  struct Segment
  {
    sInt Type;
    sInt Number;
    sInt Length;
    sU8 *Data;

    sU32 BaseAddress; // set by RDFLinker
    sU8 *LinkedData;  // set by RDFLinker
  };

  sArray<Reloc> Relocs;
  sArray<Import> Imports;
  sArray<Export> Exports;
  sArray<Segment> Segments;
  sInt DataSize,HeaderSize;
  sInt BSSSize;

  const Segment *GetSegment(sInt number) const;
  sU32 GetSegAddress(sInt number) const;

public:
  RDFObject();
  RDFObject(const sU8 *rdfFile);
  ~RDFObject();

  void Clear();
  sBool Read(const sU8 *rdfFile);
  void Dump() const;
};

/****************************************************************************/

class RDFLinker
{
  struct Stage
  {
    sU32 BaseAddress;
    sArray<RDFObject *> Objects;

    sInt CodeSize;
    sInt DataSize;
    sInt BSSSize;
    sU8 *LinkedImage;
  };

  struct Symbol
  {
    sChar Name[32];
    sU32 Address;
    sInt Stage;
  };

  sArray<Stage> Stages;
  sArray<Symbol> Symbols;
  sU32 BSSBase;

  Symbol *FindSymbol(const sChar *name,sInt stage);

public:
  RDFLinker();
  ~RDFLinker();

  sInt AddStage(sU32 baseAddress);
  void AddObject(sInt stage,RDFObject *object);
  void AddSymbol(const sChar *name,sU32 address,sInt stage);

  void SetStageBase(sInt stage,sU32 baseAddress);
  void SetBSSBase(sU32 baseAddress);

  sInt GetResult(sInt stage,sU8 *&data);
  sU32 GetSymAddress(const sChar *name,sInt stage);

  void Clear();
  sBool CalcSizes(sU32 &totalInit,sU32 &totalUninit);
  sBool Link(); // only after CalcSizes
};

/****************************************************************************/

#endif
