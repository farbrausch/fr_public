// Written by Fabian "ryg" Giesen.
// I hereby place this code in the public domain.

#include "_types.hpp"
#include "_startconsole.hpp"
#include "mapfile.hpp"
#include "debuginfo.hpp"
#include <algorithm>

typedef sU32 (__stdcall *PUnDecorateSymbolName)(sChar *name,sChar *buffer,sInt bufferLen,sU32 flasgs);

extern "C" void * __stdcall LoadLibraryA(sChar *name);
extern "C" void * __stdcall GetProcAddress(void *module,sChar *name);

// HACK: report buffer is 1mb fixed size

/****************************************************************************/

struct MAPFileReader::Section
{
  sInt Num;
  sU32 Start;
  sU32 Length;
  DIString Name;
  sBool Seen;
  sInt Class;
};

sInt MAPFileReader::ScanString(sChar *&string,DebugInfo &to)
{
  sChar buffer[3072];
  sInt i;

  i = 0;
  while(*string && *string!=' ' && *string!='\r' && *string!='\n' && i<3071)
    buffer[i++] = *string++;

  buffer[i++] = 0;
  return to.MakeString(buffer);
}

sBool MAPFileReader::IsHexString(const sChar *str,sInt count)
{
  while(count--)
  {
    if(!(*str>='0' && *str<='9' || *str>='A' && *str<='F' ||
      *str>='a' && *str<='f'))
      return sFALSE;

    str++;
  }

  return sTRUE;
}

MAPFileReader::Section *MAPFileReader::GetSection(sInt num,sU32 offs)
{
  sInt i;

  for(i=0;i<Sections.Count;i++)
  {
    if(Sections[i].Num==num && offs>=Sections[i].Start &&
      offs<Sections[i].Start+Sections[i].Length)
      return &Sections[i];
  }

  return 0;
}

/****************************************************************************/

sBool MAPFileReader::ReadDebugInfo(sChar *fileName,DebugInfo &to)
{
  PUnDecorateSymbolName UnDecorateSymbolName = 0;

  // determine map file name
  sChar fileBuf[260];
  sChar *text;
  sInt i;

  sCopyString(fileBuf,fileName,256);
  for(i=sGetStringLen(fileBuf)-1;i>=0 && fileBuf[i] != '.';i--);
  if(i > 0)
    sCopyString(fileBuf + i,".map",260-i);
  else
    sAppendString(fileBuf,".map",260);

  text = (sChar *) sSystem->LoadFile(fileBuf);
  if(!text)
    return sFALSE;

  sChar *orig_text = text;

  // load dbghelp.dll to resolve symbol names if available
  void *module = LoadLibraryA("dbghelp.dll");
  if(module)
    UnDecorateSymbolName = (PUnDecorateSymbolName) GetProcAddress(module,"UnDecorateSymbolName");

  // actual reading code
  sChar *line,buffer[2048];
  sInt j,code,data;
  sInt snum,offs,type,name,VA,fname;
  sInt symStart = to.Symbols.Count;
  Section *sec;
  DISymbol *sym;

  Sections.Init();

  code = to.MakeString("CODE");
  data = to.MakeString("DATA");

  while(*text)
  {
    // find end of line
    line = text;
    while(*text && *text != '\n')
      text++;

    if(text[-1] == '\r' && text[0] == '\n')
      text[-1] = 0;

    if(*text)
      *text++ = 0;

    // parse this line of text
    if(line[0]==' ' && IsHexString(line+1,4) && line[5]==':'
      && IsHexString(line+6,8) && line[14]==' ')
    {
      if(IsHexString(line+15,8)) // section definition
      {
        sec = Sections.Add();
        line += 1; sec->Num = sScanHex(line);
        line += 1; sec->Start = sScanHex(line);
        line += 1; sec->Length = sScanHex(line);
        line += 2; sec->Name.Index = ScanString(line,to);
        sScanSpace(line);
        type = ScanString(line,to);

        if(type == code)
          sec->Class = DIC_CODE;
        else if(type == data)
          sec->Class = DIC_DATA;
        else
          sec->Class = DIC_UNKNOWN;

        sec->Seen = sFALSE;
      }
      else // assume name definition
      {
        line += 1; snum = sScanHex(line);
        line += 1; offs = sScanHex(line);

        sec = GetSection(snum,offs);

        if(sec)
        {
          sScanSpace(line);
          name = ScanString(line,to);
          sScanSpace(line);
          VA = sScanHex(line);
          line += 5; fname = ScanString(line,to);

          if(!sec->Seen)
          {
            sym = to.Symbols.Add();
            sSPrintF(buffer,2048,"__end%s",to.GetStringPrep(sec->Name.Index));
            sym->Name.Index = to.MakeString(buffer);
            sym->FileNum = -1;
            sym->VA = VA-offs+sec->Start+sec->Length;
            sym->Size = 0;
            sym->Class = DIC_END;
            
            sec->Seen = sTRUE;
          }

          if(UnDecorateSymbolName)
            UnDecorateSymbolName(to.GetStringPrep(name),buffer,2048,0x1800);
          else
            sCopyString(buffer,to.GetStringPrep(name),2048);

          // add symbol
          sym = to.Symbols.Add();
          sym->Name.Index = to.MakeString(buffer);
          sym->MangledName.Index = name;
          sym->FileNum = to.GetFile(fname);
          sym->VA = VA;
          sym->Size = 0;
          sym->Class = sec->Class;
          sym->NameSpNum = to.GetNameSpaceByName(buffer);
        }
      }
    }
    else if(!sCmpMem(line," Preferred load address is ",28) && IsHexString(line+28,8))
    {
      line += 28;
      sU32 base = sScanHex(line);
      to.SetBaseAddress(base);
    }
  }

  // sort symbols by virtual address
  for(i=symStart+1;i<to.Symbols.Count;i++)
    for(j=i;j>symStart;j--)
      if(to.Symbols[j].VA<to.Symbols[j-1].VA)
        sSwap(to.Symbols[j],to.Symbols[j-1]);

  // calc sizes
  for(i=symStart;i<to.Symbols.Count;i++)
  {
    sym = &to.Symbols[i];

    if(sym->Class!=DIC_END)
    {
      sVERIFY(i != to.Symbols.Count-1);
      sym->Size = sym[1].VA - sym->VA;
    }
  }

  // cleanup
  Sections.Exit();
  delete[] orig_text;

  return sTRUE;
}

/****************************************************************************/
