// Written by Fabian "ryg" Giesen.
// I hereby place this code in the public domain.

#include "_types.hpp"
#include "_startconsole.hpp"
#include "rdf.hpp"

/****************************************************************************/

const RDFObject::Segment *RDFObject::GetSegment(sInt number) const
{
  for(sInt i=0;i<Segments.Count;i++)
    if(Segments[i].Number == number)
      return &Segments[i];

  return 0;
}

sU32 RDFObject::GetSegAddress(sInt number) const
{
  for(sInt i=0;i<Segments.Count;i++)
    if(Segments[i].Number == number)
      return Segments[i].BaseAddress;

  for(sInt i=0;i<Imports.Count;i++)
    if(Imports[i].Segment == number)
      return Imports[i].Address;

  sVERIFYFALSE;
  return ~0U;
}

RDFObject::RDFObject()
{
  Relocs.Init();
  Imports.Init();
  Exports.Init();
  Segments.Init();
}

RDFObject::RDFObject(const sU8 *rdfFile)
{
  Relocs.Init();
  Imports.Init();
  Exports.Init();
  Segments.Init();
  Read(rdfFile);
}

RDFObject::~RDFObject()
{
  Clear();
}

void RDFObject::Clear()
{
  for(sInt i=0;i<Segments.Count;i++)
    delete[] Segments[i].Data;

  Relocs.Count = 0;
  Imports.Count = 0;
  Exports.Count = 0;
  Segments.Count = 0;
}

sBool RDFObject::Read(const sU8 *rdfFile)
{
  Clear();

  // check id
  if(sCmpMem(rdfFile,"RDOFF2",6))
    return sFALSE;

  rdfFile += 6;

  // get data+header size
  DataSize = *((const sU32 *) rdfFile); rdfFile += 4;
  HeaderSize = *((const sU32 *) rdfFile); rdfFile += 4;
  BSSSize = 0;

  const sU8 *rdfEnd = rdfFile + DataSize - 4;

  // read header records
  const sU8 *headerEnd = rdfFile + HeaderSize;
  while(rdfFile < headerEnd)
  {
    sU8 recordId = *rdfFile++;
    sU8 recordLen = *rdfFile++;

    switch(recordId)
    {
    case 1: // relocation
      {
        if(recordLen != 8)
          return sFALSE;

        Reloc *r = Relocs.Add();
        r->Segment = *rdfFile++;
        r->Offset = *((const sU32 *) rdfFile); rdfFile += 4;
        r->Length = *rdfFile++;
        r->RefSegment = *((const sU16 *) rdfFile); rdfFile += 2;
      }
      break;

    case 2: // import
      {
        Import *i = Imports.Add();
        rdfFile++; // ???
        i->Segment = *((const sU16 *) rdfFile); rdfFile += 2;
        sCopyString(i->Label,(const sChar *) rdfFile,32);
        rdfFile += recordLen - 3;
      }
      break;

    case 3: // export
      {
        Export *e = Exports.Add();
        e->Flags = *rdfFile++;
        e->Segment = *rdfFile++;
        e->Offset = *((const sU32 *) rdfFile); rdfFile += 4;
        sCopyString(e->Label,(const sChar *) rdfFile,32);
        rdfFile += recordLen - 6;
      }
      break;

    case 5: // reserve BSS
      if(recordLen != 4)
        return sFALSE;

      BSSSize += *((const sU32 *) rdfFile); rdfFile += 4;
      break;

    default:
      sSystem->PrintF("unsupported header record type %02x\n",recordId);
      rdfFile += recordLen;
      break;
    }
  }

  if(rdfFile > headerEnd)
  {
    sSystem->PrintF("Header read error\n");
    return sFALSE;
  }

  // read segments
  while(rdfFile < rdfEnd)
  {
    Segment *s = Segments.Add();
    s->Type = *((const sU16 *) rdfFile); rdfFile += 2;
    s->Number = *((const sU16 *) rdfFile); rdfFile += 2;
    rdfFile += 2;
    s->Length = *((const sU32 *) rdfFile); rdfFile += 4;
    s->BaseAddress = 0;

    if(s->Length == 0) // empty segment, remove again
      Segments.Count--;
    else
    {
      // we only support .text+.data, currently
      if(s->Type != 1 && s->Type != 2)
        return sFALSE;

      s->Data = new sU8[s->Length];
      sCopyMem(s->Data,rdfFile,s->Length);
    }

    rdfFile += s->Length;
  }

  if(rdfFile > rdfEnd)
  {
    sSystem->PrintF("section read error\n");
    return sFALSE;
  }

  return sTRUE;
}

void RDFObject::Dump() const
{
  sSystem->PrintF("data size=%d, header size=%d\n",DataSize,HeaderSize);

  sSystem->PrintF("\nRelocs:\n");
  for(sInt i=0;i<Relocs.Count;i++)
  {
    const Reloc *rel = &Relocs[i];
    sSystem->PrintF("  Segment %d Ref %d Offset %08x Length %d\n",
      rel->Segment,rel->RefSegment,rel->Offset,rel->Length);
  }

  sSystem->PrintF("\nImports:\n");
  for(sInt i=0;i<Imports.Count;i++)
  {
    const Import *imp = &Imports[i];
    sSystem->PrintF("  \"%s\" (Segment %d)\n",imp->Label,imp->Segment);
  }

  sSystem->PrintF("\nExports:\n");
  for(sInt i=0;i<Exports.Count;i++)
  {
    const Export *exp = &Exports[i];
    sSystem->PrintF("  \"%s\" (%02x:%08x, Flags=%02x)\n",exp->Label,exp->Segment,exp->Offset,exp->Flags);
  }

  sSystem->PrintF("\nSegments:\n");
  for(sInt i=0;i<Segments.Count;i++)
  {
    const Segment *seg = &Segments[i];
    sSystem->PrintF("  Number %04x Type %d Length %d\n",seg->Number,seg->Type,seg->Length);
  }
}

/****************************************************************************/

RDFLinker::Symbol *RDFLinker::FindSymbol(const sChar *name,sInt stage)
{
  for(sInt i=0;i<Symbols.Count;i++)
    if(!sCmpString(Symbols[i].Name,name) && Symbols[i].Stage >= stage)
      return &Symbols[i];

  return 0;
}

RDFLinker::RDFLinker()
{
  Stages.Init();
  Symbols.Init();
  BSSBase = 0;
}

RDFLinker::~RDFLinker()
{
  Clear();
}

sInt RDFLinker::AddStage(sU32 baseAddress)
{
  sInt index = Stages.Count;
  Stage *stage = Stages.Add();

  stage->BaseAddress = baseAddress;
  stage->Objects.Init();
  stage->LinkedImage = 0;

  return index;
}

void RDFLinker::AddObject(sInt stage,RDFObject *object)
{
  sVERIFY(stage >= 0 && stage < Stages.Count);
  *Stages[stage].Objects.Add() = object;
}

void RDFLinker::AddSymbol(const sChar *name,sU32 address,sInt stage)
{
  Symbol *sym = Symbols.Add();
  sCopyString(sym->Name,name,sizeof(sym->Name));
  sym->Address = address;
  sym->Stage = stage;
}

void RDFLinker::SetStageBase(sInt stage,sU32 baseAddress)
{
  sVERIFY(stage >= 0 && stage < Stages.Count);
  Stages[stage].BaseAddress = baseAddress;
}

void RDFLinker::SetBSSBase(sU32 baseAddress)
{
  BSSBase = baseAddress;
}

sInt RDFLinker::GetResult(sInt stage,sU8 *&data)
{
  sVERIFY(stage >= 0 && stage < Stages.Count);
  data = Stages[stage].LinkedImage;

  return Stages[stage].CodeSize + Stages[stage].DataSize;
}

sU32 RDFLinker::GetSymAddress(const sChar *name,sInt stage)
{
  sVERIFY(stage >= 0 && stage < Stages.Count);

  Symbol *sym = FindSymbol(name,stage);
  return sym ? sym->Address : 0;
}

void RDFLinker::Clear()
{
  for(sInt i=0;i<Stages.Count;i++)
  {
    Stages[i].Objects.Exit();
    delete[] Stages[i].LinkedImage;
  }

  Stages.Count = 0;
  Symbols.Count = 0;
}

sBool RDFLinker::CalcSizes(sU32 &totalInit,sU32 &totalUninit)
{
  totalInit = 0;
  totalUninit = 0;

  // Determine size of individual sections (code,data,bss) per stage.
  for(sInt i=0;i<Stages.Count;i++)
  {
    Stage *stage = &Stages[i];

    stage->CodeSize = 0;
    stage->DataSize = 0;
    stage->BSSSize = 0;

    // Code+Data is additive because we need to keep it all.
    // OTOH, BSS gets reused between different modules (This means
    // you can only assume it to be zero in the very first one!)
    for(sInt j=0;j<stage->Objects.Count;j++)
    {
      RDFObject *obj = stage->Objects[j];

      for(sInt k=0;k<obj->Segments.Count;k++)
      {
        if(obj->Segments[k].Type == 1) // .text
          stage->CodeSize += obj->Segments[k].Length;
        else if(obj->Segments[k].Type == 2) // .data
          stage->DataSize += obj->Segments[k].Length;
        else
          return sFALSE;
      }

      if(obj->BSSSize > stage->BSSSize)
        stage->BSSSize = obj->BSSSize;
    }

    // Align BSS sizes up to multiples of 16 bytes
    stage->BSSSize = (stage->BSSSize + 15) & ~15;

    // Update totals
    totalInit += stage->CodeSize + stage->DataSize;
    totalUninit = sMax<sU32>(totalUninit,stage->BSSSize);
  }

  return sTRUE;
}

sBool RDFLinker::Link()
{
  // Copy together images and assign addresses for all stages
  for(sInt i=0;i<Stages.Count;i++)
  {
    Stage *stage = &Stages[i];

    stage->LinkedImage = new sU8[stage->CodeSize + stage->DataSize];
    sSetMem(stage->LinkedImage,0,stage->CodeSize + stage->DataSize);

    sU32 base = stage->BaseAddress;
    sU32 address = base;

    // code sections first
    for(sInt j=0;j<stage->Objects.Count;j++)
    {
      RDFObject *obj = stage->Objects[j];

      for(sInt k=0;k<obj->Segments.Count;k++)
      {
        RDFObject::Segment *seg = &obj->Segments[k];
        if(seg->Type != 1)
          continue;

        seg->BaseAddress = address;
        seg->LinkedData = stage->LinkedImage + address - base;

        sCopyMem(seg->LinkedData,seg->Data,seg->Length);
        address += seg->Length;
      }
    }

    // then data sections
    for(sInt j=0;j<stage->Objects.Count;j++)
    {
      RDFObject *obj = stage->Objects[j];

      for(sInt k=0;k<obj->Segments.Count;k++)
      {
        RDFObject::Segment *seg = &obj->Segments[k];
        if(seg->Type != 2)
          continue;

        seg->BaseAddress = address;
        seg->LinkedData = stage->LinkedImage + address - base;

        sCopyMem(seg->LinkedData,seg->Data,seg->Length);
        address += seg->Length;
      }
    }

    // check whether everything was done correctly
    sVERIFY((address - base) == stage->CodeSize + stage->DataSize);
  }

  // After addresses have been assigned, add exported symbols to symbol
  // table.
  for(sInt i=0;i<Stages.Count;i++)
  {
    Stage *stage = &Stages[i];

    for(sInt j=0;j<stage->Objects.Count;j++)
    {
      RDFObject *obj = stage->Objects[j];

      for(sInt k=0;k<obj->Exports.Count;k++)
      {
        RDFObject::Export *exp = &obj->Exports[k];
        const RDFObject::Segment *seg = obj->GetSegment(exp->Segment);
        if(!seg || !seg->BaseAddress)
          return sFALSE;

        AddSymbol(exp->Label,seg->BaseAddress + exp->Offset,i);
      }
    }
  }

  // Now process imports
  for(sInt i=0;i<Stages.Count;i++)
  {
    Stage *stage = &Stages[i];

    for(sInt j=0;j<stage->Objects.Count;j++)
    {
      RDFObject *obj = stage->Objects[j];

      for(sInt k=0;k<obj->Imports.Count;k++)
      {
        RDFObject::Import *imp = &obj->Imports[k];
        Symbol *sym = FindSymbol(imp->Label,i);
        if(!sym)
          return sFALSE;

        imp->Address = sym->Address;
      }
    }
  }

  // Finally, apply relocations (we now should have everything ready)
  for(sInt i=0;i<Stages.Count;i++)
  {
    Stage *stage = &Stages[i];

    for(sInt j=0;j<stage->Objects.Count;j++)
    {
      RDFObject *obj = stage->Objects[j];

      for(sInt k=0;k<obj->Relocs.Count;k++)
      {
        RDFObject::Reloc *rel = &obj->Relocs[k];
        const RDFObject::Segment *target = obj->GetSegment(rel->Segment & 63);
        if(!target)
          return sFALSE;

        sU32 reloc = rel->RefSegment == 2 ? BSSBase : obj->GetSegAddress(rel->RefSegment);
        if(rel->Segment & 64) // relative
          reloc -= target->BaseAddress;

        if(rel->Length != 4) // only 32bit relocs supported right now
          return sFALSE;

        sU32 *data = (sU32 *) (target->LinkedData + rel->Offset);
        *data += reloc;
      }
    }
  }

  return sTRUE;
}

/****************************************************************************/
