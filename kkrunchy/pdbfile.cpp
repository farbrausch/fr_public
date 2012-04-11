// Written by Fabian "ryg" Giesen.
// I hereby place this code in the public domain.

#include "_types.hpp"
#include "debuginfo.hpp"
#include "pdbfile.hpp"

#include <malloc.h>
#include "windows.h"
#include "ole2.h"

/****************************************************************************/

// I don't want to use the complete huge dia2.h headers (>350k), so here
// goes the minimal version...

enum SymTagEnum
{
  SymTagNull,
  SymTagExe,
  SymTagCompiland,
  SymTagCompilandDetails,
  SymTagCompilandEnv,
  SymTagFunction,
  SymTagBlock,
  SymTagData,
  SymTagAnnotation,
  SymTagLabel,
  SymTagPublicSymbol,
  SymTagUDT,
  SymTagEnum,
  SymTagFunctionType,
  SymTagPointerType,
  SymTagArrayType,
  SymTagBaseType,
  SymTagTypedef,
  SymTagBaseClass,
  SymTagFriend,
  SymTagFunctionArgType,
  SymTagFuncDebugStart,
  SymTagFuncDebugEnd,
  SymTagUsingNamespace,
  SymTagVTableShape,
  SymTagVTable,
  SymTagCustom,
  SymTagThunk,
  SymTagCustomType,
  SymTagManagedType,
  SymTagDimension,
  SymTagMax
};

class IDiaEnumSymbols;
class IDiaEnumSymbolsByAddr;
class IDiaEnumTables;

class IDiaDataSource;
class IDiaSession;

class IDiaSymbol;
class IDiaSectionContrib;

class IDiaTable;

// not transcribed here:
class IDiaSourceFile;

class IDiaEnumSourceFiles;
class IDiaEnumLineNumbers;
class IDiaEnumDebugStreams;
class IDiaEnumInjectedSources;

class IDiaEnumSymbols : public IUnknown
{
public:
  virtual HRESULT __stdcall get__NewEnum(IUnknown **ret) = 0;
  virtual HRESULT __stdcall get_Count(LONG *ret) = 0;

  virtual HRESULT __stdcall Item(DWORD index,IDiaSymbol **symbol) = 0;
  virtual HRESULT __stdcall Next(ULONG celt,IDiaSymbol **rgelt,ULONG *pceltFetched) = 0;
  virtual HRESULT __stdcall Skip(ULONG celt) = 0;
  virtual HRESULT __stdcall Reset() = 0;

  virtual HRESULT __stdcall Clone(IDiaEnumSymbols **penum) = 0;
};

class IDiaEnumSymbolsByAddr : public IUnknown
{
public:
  virtual HRESULT __stdcall symbolByAddr(DWORD isect,DWORD offset,IDiaSymbol** ppSymbol) = 0;
  virtual HRESULT __stdcall symbolByRVA(DWORD relativeVirtualAddress,IDiaSymbol** ppSymbol) = 0;
  virtual HRESULT __stdcall symbolByVA(ULONGLONG virtualAddress,IDiaSymbol** ppSymbol) = 0;

  virtual HRESULT __stdcall Next(ULONG celt,IDiaSymbol ** rgelt,ULONG* pceltFetched) = 0;
  virtual HRESULT __stdcall Prev(ULONG celt,IDiaSymbol ** rgelt,ULONG * pceltFetched) = 0;

  virtual HRESULT __stdcall Clone(IDiaEnumSymbolsByAddr **ppenum) = 0;
};

class IDiaEnumTables : public IUnknown
{
public:
  virtual HRESULT __stdcall get__NewEnum(IUnknown **ret) = 0;
  virtual HRESULT __stdcall get_Count(LONG *ret) = 0;

  virtual HRESULT __stdcall Item(VARIANT index,IDiaTable **table) = 0;
  virtual HRESULT __stdcall Next(ULONG celt,IDiaTable ** rgelt,ULONG *pceltFetched) = 0;
  virtual HRESULT __stdcall Skip(ULONG celt) = 0;
  virtual HRESULT __stdcall Reset() = 0;

  virtual HRESULT __stdcall Clone(IDiaEnumTables **ppenum) = 0;
};

class IDiaDataSource : public IUnknown
{
public:
  virtual HRESULT __stdcall get_lastError(BSTR *ret) = 0;

  virtual HRESULT __stdcall loadDataFromPdb(LPCOLESTR pdbPath) = 0;
  virtual HRESULT __stdcall loadAndValidateDataFromPdb(LPCOLESTR pdbPath,GUID *pcsig70,DWORD sig,DWORD age) = 0;
  virtual HRESULT __stdcall loadDataForExe(LPCOLESTR executable,LPCOLESTR searchPath,IUnknown *pCallback) = 0;
  virtual HRESULT __stdcall loadDataFromIStream(IStream *pIStream) = 0;

  virtual HRESULT __stdcall openSession(IDiaSession **ppSession) = 0;
};

class IDiaSession : public IUnknown
{
public:
  virtual HRESULT __stdcall get_loadAddress(ULONGLONG *ret) = 0;
  virtual HRESULT __stdcall put_loadAddress(ULONGLONG val) = 0;
  virtual HRESULT __stdcall get_globalScape(IDiaSymbol **sym) = 0;

  virtual HRESULT __stdcall getEnumTables(IDiaEnumTables** ppEnumTables) = 0;
  virtual HRESULT __stdcall getSymbolsByAddr(IDiaEnumSymbolsByAddr** ppEnumbyAddr) = 0;

  virtual HRESULT __stdcall findChildren(IDiaSymbol* parent,enum SymTagEnum symtag,LPCOLESTR name,DWORD compareFlags,IDiaEnumSymbols** ppResult) = 0;
  virtual HRESULT __stdcall findSymbolByAddr(DWORD isect,DWORD offset,enum SymTagEnum symtag,IDiaSymbol** ppSymbol) = 0;
  virtual HRESULT __stdcall findSymbolByRVA(DWORD rva,enum SymTagEnum symtag,IDiaSymbol** ppSymbol) = 0;
  virtual HRESULT __stdcall findSymbolByVA(ULONGLONG va,enum SymTagEnum symtag,IDiaSymbol** ppSymbol) = 0;
  virtual HRESULT __stdcall findSymbolByToken(ULONG token,enum SymTagEnum symtag,IDiaSymbol** ppSymbol) = 0;
  virtual HRESULT __stdcall symsAreEquiv(IDiaSymbol* symbolA,IDiaSymbol* symbolB) = 0;
  virtual HRESULT __stdcall symbolById(DWORD id,IDiaSymbol** ppSymbol) = 0;
  virtual HRESULT __stdcall findSymbolByRVAEx(DWORD rva,enum SymTagEnum symtag,IDiaSymbol** ppSymbol,long* displacement) = 0;
  virtual HRESULT __stdcall findSymbolByVAEx(ULONGLONG va,enum SymTagEnum symtag,IDiaSymbol** ppSymbol,long* displacement) = 0;

  virtual HRESULT __stdcall findFile(IDiaSymbol* pCompiland,LPCOLESTR name,DWORD compareFlags,IDiaEnumSourceFiles** ppResult) = 0;
  virtual HRESULT __stdcall findFileById(DWORD uniqueId,IDiaSourceFile** ppResult) = 0;

  virtual HRESULT __stdcall findLines(IDiaSymbol* compiland,IDiaSourceFile* file,IDiaEnumLineNumbers** ppResult) = 0;
  virtual HRESULT __stdcall findLinesByAddr(DWORD seg,DWORD offset,DWORD length,IDiaEnumLineNumbers** ppResult) = 0;
  virtual HRESULT __stdcall findLinesByRVA(DWORD rva,DWORD length,IDiaEnumLineNumbers** ppResult) = 0;
  virtual HRESULT __stdcall findLinesByVA(ULONGLONG va,DWORD length,IDiaEnumLineNumbers** ppResult) = 0;
  virtual HRESULT __stdcall findLinesByLinenum(IDiaSymbol* compiland,IDiaSourceFile* file,DWORD linenum,DWORD column,IDiaEnumLineNumbers** ppResult) = 0;

  virtual HRESULT __stdcall findInjectedSource(LPCOLESTR srcFile,IDiaEnumInjectedSources** ppResult) = 0;
  virtual HRESULT __stdcall getEnumDebugStreams(IDiaEnumDebugStreams** ppEnumDebugStreams) = 0;
};

class IDiaSymbol : public IUnknown
{
public:
  virtual HRESULT __stdcall get_symIndexId(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_symTag(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_name(BSTR *ret) = 0;
  virtual HRESULT __stdcall get_lexicalParent(IDiaSymbol **ret) = 0;
  virtual HRESULT __stdcall get_classParent(IDiaSymbol **ret) = 0;
  virtual HRESULT __stdcall get_type(IDiaSymbol **ret) = 0;
  virtual HRESULT __stdcall get_dataKind(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_locationType(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_addressSection(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_addressOffset(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_relativeVirtualAddress(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_virtualAddress(ULONGLONG *ret) = 0;
  virtual HRESULT __stdcall get_registerId(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_offset(LONG *ret) = 0;
  virtual HRESULT __stdcall get_length(ULONGLONG *ret) = 0;
  virtual HRESULT __stdcall get_slot(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_volatileType(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_constType(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_unalignedType(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_access(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_libraryName(BSTR *ret) = 0;
  virtual HRESULT __stdcall get_platform(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_language(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_editAndContinueEnabled(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_frontEndMajor(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_frontEndMinor(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_frontEndBuild(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_backEndMajor(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_backEndMinor(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_backEndBuild(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_sourceFileName(BSTR *ret) = 0;
  virtual HRESULT __stdcall get_unused(BSTR *ret) = 0;
  virtual HRESULT __stdcall get_thunkOrdinal(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_thisAdjust(LONG *ret) = 0;
  virtual HRESULT __stdcall get_virtualBaseOffset(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_virtual(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_intro(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_pure(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_callingConvention(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_value(VARIANT *ret) = 0;
  virtual HRESULT __stdcall get_baseType(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_token(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_timeStamp(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_guid(GUID *ret) = 0;
  virtual HRESULT __stdcall get_symbolsFileName(BSTR *ret) = 0;
  virtual HRESULT __stdcall get_reference(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_count(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_bitPosition(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_arrayIndexType(IDiaSymbol **ret) = 0;
  virtual HRESULT __stdcall get_packed(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_constructor(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_overloadedOperator(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_nested(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_hasNestedTypes(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_hasAssignmentOperator(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_hasCastOperator(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_scoped(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_virtualBaseClass(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_indirectVirtualBaseClass(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_virtualBasePointerOffset(LONG *ret) = 0;
  virtual HRESULT __stdcall get_virtualTableShape(IDiaSymbol **ret) = 0;
  virtual HRESULT __stdcall get_lexicalParentId(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_classParentId(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_typeId(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_arrayIndexTypeId(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_virtualTableShapeId(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_code(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_function(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_managed(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_msil(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_virtualBaseDispIndex(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_undecoratedName(BSTR *ret) = 0;
  virtual HRESULT __stdcall get_age(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_signature(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_compilerGenerated(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_addressTaken(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_rank(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_lowerBound(IDiaSymbol **ret) = 0;
  virtual HRESULT __stdcall get_upperBound(IDiaSymbol **ret) = 0;
  virtual HRESULT __stdcall get_lowerBoundId(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_upperBoundId(DWORD *ret) = 0;
  
  virtual HRESULT __stdcall get_dataBytes(DWORD cbData,DWORD *pcbData,BYTE data[]) = 0;
  virtual HRESULT __stdcall findChildren(enum SymTagEnum symtag,LPCOLESTR name,DWORD compareFlags,IDiaEnumSymbols** ppResult) = 0;

  virtual HRESULT __stdcall get_targetSection(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_targetOffset(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_targetRelativeVirtualAddress(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_targetVirtualAddress(ULONGLONG *ret) = 0;
  virtual HRESULT __stdcall get_machineType(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_oemId(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_oemSymbolId(DWORD *ret) = 0;

  virtual HRESULT __stdcall get_types(DWORD cTypes,DWORD *pcTypes,IDiaSymbol* types[]) = 0;
  virtual HRESULT __stdcall get_typeIds(DWORD cTypes,DWORD *pcTypeIds,DWORD typeIds[]) = 0;
  
  virtual HRESULT __stdcall get_objectPointerType(IDiaSymbol **ret) = 0;
  virtual HRESULT __stdcall get_udtKind(DWORD *ret) = 0;

  virtual HRESULT __stdcall get_undecoratedNameEx(DWORD undecorateOptions,BSTR *name) = 0;
};

class IDiaSectionContrib : public IUnknown
{
public:
  virtual HRESULT __stdcall get_compiland(IDiaSymbol **ret) = 0;
  virtual HRESULT __stdcall get_addressSection(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_addressOffset(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_relativeVirtualAddress(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_virtualAddress(ULONGLONG *ret) = 0;
  virtual HRESULT __stdcall get_length(DWORD *ret) = 0;

  virtual HRESULT __stdcall get_notPaged(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_code(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_initializedData(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_uninitializedData(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_remove(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_comdat(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_discardable(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_notCached(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_share(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_execute(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_read(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_write(BOOL *ret) = 0;
  virtual HRESULT __stdcall get_dataCrc(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_relocationsaCrc(DWORD *ret) = 0;
  virtual HRESULT __stdcall get_compilandId(DWORD *ret) = 0;
};

class IDiaTable : public IEnumUnknown
{
public:
  virtual HRESULT __stdcall get__NewEnum(IUnknown **ret) = 0;
  virtual HRESULT __stdcall get_name(BSTR *ret) = 0;
  virtual HRESULT __stdcall get_Count(LONG *ret) = 0;
  
  virtual HRESULT __stdcall Item(DWORD index,IUnknown **element) = 0;
};

class DECLSPEC_UUID("e60afbee-502d-46ae-858f-8272a09bd707") DiaSource;
class DECLSPEC_UUID("79f1bb5f-b66e-48e5-b6a9-1545c323ca3d") IDiaDataSource;

/****************************************************************************/

struct PDBFileReader::SectionContrib
{
  DWORD Section;
  DWORD Offset;
  DWORD Length;
  DWORD Compiland;
  BOOL CodeFlag;
};

sU32 PDBFileReader::CompilandFromSectionOffset(sU32 sec,sU32 offs,sBool &codeFlag)
{
  sInt l,r,x;

  l = 0;
  r = nContribs;
  
  while(l < r)
  {
    x = (l + r) / 2;
    const SectionContrib &cur = Contribs[x];

    if(sec < cur.Section || sec == cur.Section && offs < cur.Offset)
      r = x;
    else if(sec > cur.Section || sec == cur.Section && offs >= cur.Offset + cur.Length)
      l = x+1;
    else // we got a winner
    {
      codeFlag = cur.CodeFlag;
      return cur.Compiland;
    }
  }

  // normally, this shouldn't happen!
  return 0;
}

// helpers
static sChar *BStrToString(BSTR str,sChar *defString="")
{
  if(!str)
  {
    sInt len = sGetStringLen(defString);
    sChar *buffer = new sChar[len+1];
    sCopyString(buffer,defString,len+1);

    return buffer;
  }
  else
  {
    sInt len = SysStringLen(str);
    sChar *buffer = new sChar[len+1];

    for(sInt i=0;i<len;i++)
      buffer[i] = (str[i] >= 32 && str[i] < 128) ? str[i] : '?';

    buffer[len] = 0;

    return buffer;
  }
}

static sInt GetBStr(BSTR str,sChar *defString,DebugInfo &to)
{
  sChar *normalStr = BStrToString(str);
  sInt result = to.MakeString(normalStr);
  delete[] normalStr;

  return result;
}

void PDBFileReader::ProcessSymbol(IDiaSymbol *symbol,DebugInfo &to)
{
  DWORD section,offset,rva;
  enum SymTagEnum tag;
  ULONGLONG length = 0;
  sU32 compilandId;
  IDiaSymbol *compiland = 0;
  BSTR name = 0, sourceName = 0;
  sBool codeFlag;

  symbol->get_symTag((DWORD *) &tag);
  symbol->get_relativeVirtualAddress(&rva);
  symbol->get_length(&length);
  symbol->get_addressSection(&section);
  symbol->get_addressOffset(&offset);

  // is the previous symbol desperately looking for a length? we can help!
  if(DanglingLengthStart)
  {
    to.Symbols[to.Symbols.Count - 1].Size = rva - DanglingLengthStart;
    DanglingLengthStart = 0;
  }

  // get length from type for data
  if(tag == SymTagData)
  {
    IDiaSymbol *type;
    if(symbol->get_type(&type) == S_OK)
    {
      type->get_length(&length);
      type->Release();
    }

    // if length still zero, just guess and take number of bytes between
    // this symbol and the next one.
    if(!length)
      DanglingLengthStart = rva;
  }

  compilandId = CompilandFromSectionOffset(section,offset,codeFlag);
  Session->symbolById(compilandId,&compiland);

  if(compiland)
    compiland->get_name(&sourceName);

  symbol->get_name(&name);

  // fill out structure
  sChar *nameStr = BStrToString(name,"<no name>");
  sChar *sourceStr = BStrToString(sourceName,"<no source>");

  if(tag == SymTagPublicSymbol)
  {
    length = 0;
    DanglingLengthStart = rva;
  }

  DISymbol *outSym = to.Symbols.Add();
  outSym->Name.Index = outSym->MangledName.Index = to.MakeString(nameStr);
  outSym->FileNum = to.GetFileByName(sourceStr);
  outSym->VA = rva;
  outSym->Size = (sU32) length;
  outSym->Class = codeFlag ? DIC_CODE : DIC_DATA;
  outSym->NameSpNum = to.GetNameSpaceByName(nameStr);

  // clean up
  delete[] nameStr;
  delete[] sourceStr;

  if(compiland)   compiland->Release();
  if(sourceName)  SysFreeString(sourceName);
  if(name)        SysFreeString(name);
}

void PDBFileReader::ReadEverything(DebugInfo &to)
{
  ULONG celt;
  
  Contribs = 0;
  nContribs = 0;
  
  DanglingLengthStart = 0;

  // read section table
  IDiaEnumTables *enumTables;
  if(Session->getEnumTables(&enumTables) == S_OK)
  {
    VARIANT vIndex;
    vIndex.vt = VT_BSTR;
    vIndex.bstrVal = SysAllocString(L"Sections");

    IDiaTable *secTable;
    if(enumTables->Item(vIndex,&secTable) == S_OK)
    {
      LONG count;

      secTable->get_Count(&count);
      Contribs = new SectionContrib[count];
      nContribs = 0;

      IDiaSectionContrib *item;
      while(SUCCEEDED(secTable->Next(1,(IUnknown **)&item,&celt)) && celt == 1)
      {
        SectionContrib &contrib = Contribs[nContribs++];

        item->get_addressOffset(&contrib.Offset);
        item->get_addressSection(&contrib.Section);
        item->get_length(&contrib.Length);
        item->get_compilandId(&contrib.Compiland);
        item->get_execute(&contrib.CodeFlag);

        item->Release();
      }

      secTable->Release();
    }

    SysFreeString(vIndex.bstrVal);
    enumTables->Release();
  }

  // enumerate symbols by (virtual) address
  IDiaEnumSymbolsByAddr *enumByAddr;
  if(SUCCEEDED(Session->getSymbolsByAddr(&enumByAddr)))
  {
    IDiaSymbol *symbol;
    // get first symbol to get first RVA (argh)
    if(SUCCEEDED(enumByAddr->symbolByAddr(1,0,&symbol)))
    {
      DWORD rva;
      if(symbol->get_relativeVirtualAddress(&rva) == S_OK)
      {
        symbol->Release();

        // now, enumerate by rva.
        if(SUCCEEDED(enumByAddr->symbolByRVA(rva,&symbol)))
        {
          do
          {
            ProcessSymbol(symbol,to);
            symbol->Release();

            if(FAILED(enumByAddr->Next(1,&symbol,&celt)))
              break;
          }
          while(celt == 1);
        }
      }
      else
        symbol->Release();
    }

    enumByAddr->Release();
  }

  // clean up
  delete[] Contribs;
}

/****************************************************************************/

sBool PDBFileReader::ReadDebugInfo(sChar *fileName,DebugInfo &to)
{
  sBool readOk = sFALSE;

  if(FAILED(CoInitialize(0)))
    return sFALSE;

  IDiaDataSource *source = 0;
  CoCreateInstance(__uuidof(DiaSource),0,CLSCTX_INPROC_SERVER,
    __uuidof(IDiaDataSource),(void**) &source);

  if(source)
  {
    wchar_t wideFileName[260];
    mbstowcs(wideFileName,fileName,260);
    if(SUCCEEDED(source->loadDataForExe(wideFileName,0,0)))
    {
      if(SUCCEEDED(source->openSession(&Session)))
      {
        ReadEverything(to);

        readOk = sTRUE;
        Session->Release();
      }
    }

    source->Release();
  }

  CoUninitialize();

  return readOk;
}

/****************************************************************************/
