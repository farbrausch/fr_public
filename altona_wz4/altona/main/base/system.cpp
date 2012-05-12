/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "base/types.hpp"
#include "base/system.hpp"
#include "base/input2.hpp"

/****************************************************************************/

sInt sSystemFlags = 0;
sInt sExitFlag = 0;

sApp *sAppPtr = 0;
sHooks *sFrameHook = 0;
sHooks *sNewDeviceHook = 0;
sHooks1<sBool &> *sAltF4Hook = 0;
sHooks1<sBool> *sActivateHook = 0;
sHooks1<sStringDesc> *sCrashHook = 0;
sHooks *sCheckCapsHook = 0;

#if !sSTRIPPED
sHooks2<const sInput2Event &,sBool &> *sInputHook = 0;
sHooks1<const sChar*> *sDebugOutHook = 0;
#endif


void (*sRedirectStdOut)(const sChar *text) = sNULL;

sBool sAllocMemEnabled = sTRUE;

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Basic Stuff                                                        ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

sApp::~sApp()
{
}

void sApp::OnInput(const sInput2Event &)
{
}

void sApp::OnPaint2D(const sRect &,const sRect &)
{
}

void sApp::OnPaint3D()
{
}

void sApp::OnPrepareFrame()
{
}

sBool sApp::OnPaint()
{
  return sFALSE;
}

void sApp::OnEvent(sInt)
{
}

void sApp::OnInit()
{
}

/****************************************************************************/

void sVerifyFalse(const sChar *file,sInt line)
{
  sFatal(sTXT("%s(%d) : assertion"),file,line);
}

void sVerifyFalse2(const sChar *file,sInt line,const sChar *expr,const sChar *desc)
{
  sFatal(L"%s(%d): Assertion failed%s%s%s%s",file,line,(expr[0]?L"\nExpression: ":L""),expr,(desc[0]?L"\nDescription: ":L""),desc);
}

void sExit()
{
  if(!sExitFlag)
    sExitFlag = 1;
}

void sRestart()
{
  sExitFlag = 2;
}

sInt sGetSystemFlags()
{
  return sSystemFlags;
}

void sSetApp(sApp *app)
{
  sAppPtr = app;
  if (sAppPtr)
    sAppPtr->OnInit();
}

sApp *sGetApp()
{
  return sAppPtr;
}

/****************************************************************************/

static struct sSubsystem
{
  sInt Priority;
  const sChar *Name;
  void (*Init)();
  void (*Exit)();
  sBool Running;
} Subsystems[256];
static sInt SubsystemCount;
static sInt SubsystemPriority;

sBool sIsSubsystemRunning(const sChar *name)
{
  sBool result = sFALSE;

  sInt i = SubsystemCount;
  while (i-- && !result)
  {
    sSubsystem *subsystem = Subsystems + i;
    result = sCmpStringI(name, subsystem->Name)==0 && subsystem->Running;
  }

  return result;
}

void sAddSubsystem(const sChar *name,sInt priority,void (*init)(),void (*exit)())
{
  sVERIFY(SubsystemCount<sCOUNTOF(Subsystems));
  Subsystems[SubsystemCount].Name = name;
  Subsystems[SubsystemCount].Priority = priority;
  Subsystems[SubsystemCount].Init = init;
  Subsystems[SubsystemCount].Exit = exit;
  Subsystems[SubsystemCount].Running = 0;
  if(Subsystems[SubsystemCount].Priority < SubsystemPriority)
  {
    if(Subsystems[SubsystemCount].Init)
      (*Subsystems[SubsystemCount].Init)();
    Subsystems[SubsystemCount].Running = 1;
  }
  SubsystemCount++;
}

void sSetRunlevel(sInt priority)
{
  // sort runlevels

  for(sInt i=0;i<SubsystemCount-1;i++)
    for(sInt j=i+1;j<SubsystemCount;j++)
      if(Subsystems[i].Priority > Subsystems[j].Priority)
        sSwap(Subsystems[i],Subsystems[j]);

  // switch on what needs switching on

  for(sInt i=0;i<SubsystemCount;i++)
  {
    if(Subsystems[i].Priority<priority && !Subsystems[i].Running)
    {
      sLogF(L"sys",L"init 0x%02x %s\n",Subsystems[i].Priority,Subsystems[i].Name);
      sPushMemLeakDesc(Subsystems[i].Name);
      if(Subsystems[i].Init)
        (*Subsystems[i].Init)();
      Subsystems[i].Running = 1;
      sPopMemLeakDesc();
    }
  }

  // switch off what needs switching off

  for(sInt i=SubsystemCount-1;i>=0;i--)
  {
    if(Subsystems[i].Priority>=priority && Subsystems[i].Running)
    {
      sLogF(L"sys",L"exit 0x%02x %s\n",Subsystems[i].Priority,Subsystems[i].Name);
      if(Subsystems[i].Exit)
        (*Subsystems[i].Exit)();
      Subsystems[i].Running = 0;
    }
  }

  SubsystemPriority = priority;
}

sInt sGetRunlevel()
{
  return SubsystemPriority;
}

/****************************************************************************/
/***                                                                      ***/
/***   Misc                                                               ***/
/***                                                                      ***/
/****************************************************************************/

#if sDEBUG

const sF32 *sCheckFloatArray(const sF32 *ptr, sInt count)
{
  for (sInt i=0; i<count; i++)
  {
    sU32 v=*(sU32*)ptr;
    sInt exp=(v&0x7f800000)>>23;
    if (exp==0xff)
      return ptr;
    else if (exp==0xf0)
      return ptr;
	  ptr++;
  }

  return sNULL;
}

#endif

sDateAndTime::sDateAndTime()
{
  Year = 0;
  Month = 0;
  Day = 0;
  DayOfWeek = 0;
  Hour = 0;
  Minute = 0;
  Second = 0;
}

sInt sDateAndTime::Compare(const sDateAndTime &x) const
{
  sInt d;

  if((d = Year - x.Year) != 0)      return d;
  if((d = Month - x.Month) != 0)    return d;
  if((d = Day - x.Day) != 0)        return d;
  if((d = Hour - x.Hour) != 0)      return d;
  if((d = Minute - x.Minute) != 0)  return d;
  if((d = Second - x.Second) != 0)  return d;
  return 0;
}

#if sPLATFORM != sPLAT_WINDOWS && sPLATFORM != sPLAT_LINUX
sU8 sGetFirstDayOfWeek()
{
  const sU8 SUNDAY=0;
  const sU8 MONDAY=1;

  sInt region = sGetRegionCodes();
  switch(sGetLanguage())
  {
  case sLANG_PT:
  case sLANG_JP:
    return SUNDAY;

  case sLANG_ES:
    return (region & sREGION_EUROPE) ? MONDAY : SUNDAY; // spain, middle america

  case sLANG_EN:
    return (region & sREGION_EUROPE) ? MONDAY : SUNDAY; // britain, usa

  case sLANG_FR:
    return (region & sREGION_EUROPE) ? MONDAY : SUNDAY; // france, canada

  case sLANG_DE:
  case sLANG_IT:
  case sLANG_NL:
  case sLANG_RU:
  default:
    return MONDAY;
  }
}
#endif

sBool sIsLeapYear(sU16 year)
{
  // Every 4th year is a leap year.
  // Years that are multiple of 100 are special. Only those that are multiple of 400 are leap years.
  sVERIFY(year > 0 && year <= sMAX_U16);
  return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

sInt sGetDaysInMonth(sU16 year, sU8 month)
{
  sVERIFY((year >= 1 || year <= sMAX_U16) && (month >= 1 || month <= 12));
  if (month == 2 && !sIsLeapYear(year))
    return 28;

  static const sU8 daysInMonth[] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  return daysInMonth[month - 1];
}

sBool sIsDateValid(sU16 year, sU8 month, sU8 day)
{
  if (year < 1 || year > sMAX_U16) return sFALSE;
  if (month < 1 || month > 12) return sFALSE;
  if (day < 1) return sFALSE;
  return day <= sGetDaysInMonth(year, month);
}

sBool sIsDateValid(const sDateAndTime &date)
{
  return sIsDateValid(date.Year, date.Month, date.Day);
}

sU8 sGetDayOfWeek(const sDateAndTime &date)
{
  // Calculates the day of the week using Zeller's congruence (http://www.ietf.org/rfc/rfc3339.txt).
  sU16 year = date.Year;
  sU8 month = date.Month;
  sU8 day = date.Day;
  sVERIFY(sIsDateValid(year, month, day));
  sU8 a = (14 - month) / 12;
  year -= a;
  return (day + year + 31 * (month + 12 * a - 2) / 12 + year / 4 - year / 100 + year / 400) % 7;
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   File abstraction                                                   ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

sFile::sFile()
{
  MapFakeMem = 0;
}

sFile::~sFile()
{
  delete[] MapFakeMem;
}

sBool sFile::Close()
{
  return 0;
}

sBool sFile::Read(void *data,sDInt size)
{
  return 0;
}

sBool sFile::Write(const void *data,sDInt size)
{
  return 0;
}

sU8 *sFile::Map(sS64 offset,sDInt size)
{
  return 0;
}

sBool sFile::SetOffset(sS64 offset)
{
  return 0;
}

sS64 sFile::GetOffset()
{
  return 0;
}

sBool sFile::SetSize(sS64)
{
  return 0;
}

sS64 sFile::GetSize()
{
  return 0;
}

sFileReadHandle sFile::BeginRead(sS64 offset,sDInt size, void *destbuffer, sFilePriorityFlags prio)
{
  sFatal(L"asynchronous io not supported");
  return 0;
}

void sFile::EndRead(sFileReadHandle handle)
{
  sFatal(L"asynchronous io not supported");
}

sBool sFile::DataAvailable(sFileReadHandle handle)
{
  sFatal(L"asynchronous io not supported");
  return 0;
}

void *sFile::GetData(sFileReadHandle handle)
{
  sFatal(L"asynchronous io not supported");
  return 0;
}

sU64 sFile::GetPhysicalPosition(sS64 offset)
{
  sFatal(L"physical positions not supported");
  return 0;
}

sU8 *sFile::MapAll()
{
  if(MapFakeMem) 
    return MapFakeMem;

  sDInt size = GetSize();
  if(size>0x7fffffff)
    return 0;

  sU8 *map = Map(0,size);

  if(map==0)
  {
#if sPLATFORM==sPLAT_WINDOWS
    sLogF(L"sys",L"file mapping failed\n");
#endif
    map = MapFakeMem = (sU8 *)sAllocMem(size,16,sAMF_ALT);
    if(!Read(MapFakeMem,sDInt(size)))
    {
      sDeleteArray(MapFakeMem);
      return 0;
    }
  }

  return map;
}


sInt sFile::CopyFrom(sFile *f)
{
  if (!f) return 0;
  return CopyFrom(f,~0ull >> 1);
}

sInt sFile::CopyFrom(sFile *f, sS64 max)
{
  if (!f) return 0;
  max=sMin(max,f->GetSize()-f->GetOffset());

#if sPLATFORM==sPLAT_WINDOWS || sPLATFORM==sPLAT_LINUX
  static const sS64 CHUNK=4*1024*1024;
#else
  static const sS64 CHUNK=65536;
#endif
  sU8 *buffer=new sU8[CHUNK];
  sInt done=0;

  while (max)
  {
    sInt todo=sMin(max,CHUNK);
    if (!f->Read(buffer,todo)) break;
    if (!Write(buffer,todo)) break;
    max-=todo;
    done+=todo;
  }

  sDeleteArray(buffer);
  return done;
}

/****************************************************************************/

sFailsafeFile::sFailsafeFile(sFile *host,const sChar *name,const sChar *tgt)
{
  Host = host;
  sVERIFY(Host != 0);
  CurrentName = name;
  TargetName = tgt;
}

sFailsafeFile::~sFailsafeFile()
{
  Close();
}

sBool sFailsafeFile::Close()
{
  if(!Host || !Host->Close())
    return sFALSE;

  sDelete(Host);

  return sRenameFile(CurrentName,TargetName,sTRUE);
  // you'd probably want to fsync here on Linux.
}

/****************************************************************************/

sFileHandler::~sFileHandler()
{
}

sFile *sFileHandler::Create(const sChar *name,sFileAccess access)
{
  return 0;
};

sBool sFileHandler::Exists(const sChar *name)
{
  sFile *f=Create(name,sFA_READ);
  delete f;
  return f!=0;
}

/****************************************************************************/

static sFileHandler *FileHandlers[16];
static sInt FileHandlerCount;

void sInitFileHandlers()
{
  FileHandlerCount = 0;
}

void sExitFileHandlers()
{
  for(sInt i=0;i<FileHandlerCount;i++)
    sDelete(FileHandlers[i]);
}

sFile *sCreateFile(const sChar *name,sFileAccess access)
{
  sFile *file;
  for(sInt i=FileHandlerCount-1;i>=0;i--)   // have to scan backwards!
  {
    file = FileHandlers[i]->Create(name,access);
    if(file) 
      return file;
  }
  sLogF(L"file",L"File not Found <%s>\n",name);
  return 0;
}

sFile *sCreateFailsafeFile(const sChar *name,sFileAccess access)
{
  if(access == sFA_WRITE)
  {
    sString<2048> tempName;
    tempName.PrintF(L"%s_%016x.fail.tmp",name,sGetTimeUS());
    sFile *host = sCreateFile(tempName,access);
    if(!host)
    {
      sLogF(L"file",L"Error creating temp file for failsafe file");
      return 0;
    }
    return new sFailsafeFile(host,tempName,name);
  }
  else
  {
    sLogF(L"file",L"Can't create failsafe file for other access modes than write, returning a conventional file instead.");
    return sCreateFile(name,access);
  }
}

sBool sCheckFile(const sChar *name)
{
  for(sInt i=FileHandlerCount-1;i>=0;i--)
    if (FileHandlers[i]->Exists(name))
      return sTRUE;

  return sFALSE;
}


void sAddFileHandler(sFileHandler *h)
{
  sVERIFY(FileHandlerCount<sCOUNTOF(FileHandlers));
  FileHandlers[FileHandlerCount++] = h;
}

void sRemFileHandler(sFileHandler *h)
{
  for (sInt i=0; i<FileHandlerCount; i++)
    if (h==FileHandlers[i])
    {
      FileHandlerCount--;
      for (sInt j=i; j<FileHandlerCount; j++)
        FileHandlers[i]=FileHandlers[j+1];
      return;
    }
}

sADDSUBSYSTEM(FileHandlers,0x20,sInitFileHandlers,sExitFileHandlers);

/****************************************************************************/
/****************************************************************************/

class sMemFile : public sFile
{
  sU8 *Data;
  sDInt Size;
  sBool Ownage;
  sDInt Used;
  sDInt Offset;
  sInt BlockSize;
  sFileAccess Access;
public:
  sMemFile(const void *data,sDInt size,sBool owndata,sInt bsize=1);
  sMemFile(void *data,sDInt size,sBool owndata,sInt bsize=1,sFileAccess access=sFA_READ);
  ~sMemFile();
  sBool Read(void *data,sDInt size);  // read bytes
  sBool Write(const void *data,sDInt size); // write bytes, may change mapping.
  sU8 *Map(sS64 offset,sDInt size);  // map file (from offset) (may fail)  
  sBool SetOffset(sS64 offset);       // seek to offset
  sS64 GetOffset();                   // get offset
  sS64 GetSize();                     // get size

  sFileReadHandle BeginRead(sS64 offset,sDInt size,void *destbuffer, sFilePriorityFlags prio/*=0*/);
  sBool DataAvailable(sFileReadHandle handle);
  void EndRead(sFileReadHandle handle);
};

sMemFile::sMemFile(const void *data,sDInt size,sBool owndata,sInt bsize/*=1*/)
{
  Data = (sU8*)data;  // loose const, but only allow reading
  Size = size;
  Ownage = owndata;
  Used = 0;
  Offset = 0;
  BlockSize = bsize;
  Access = sFA_READ;
}

sMemFile::sMemFile(void *data,sDInt size,sBool owndata,sInt bsize/*=1*/,sFileAccess access/*=sFA_READ*/)
{
  Data = (sU8*)data;
  Size = size;
  Ownage = owndata;
  Used = 0;
  Offset = 0;
  BlockSize = bsize;
  Access = access;
}

sMemFile::~sMemFile()
{
  if(Ownage)
    delete[] Data;
}

sBool sMemFile::Read(void *data,sDInt size)
{
  if(Offset+size<=Size && (Access==sFA_READ || Access==sFA_READRANDOM || Access==sFA_READWRITE))
  {
    sCopyMem(data,Data+Offset,size);
    Offset += size;
    return 1;
  }

  return 0;
}

sBool sMemFile::Write(const void *data,sDInt size)
{
  if(Offset+size <= Size && (Access==sFA_WRITE || Access==sFA_WRITEAPPEND || Access==sFA_READWRITE))
  {
    // fill uninitialized file with zeros
    if(Offset > Used)
    {
      sSetMem(Data+Used,0,Offset-Used);
      Used = Offset;
    }

    // write
    sCopyMem(Data+Offset,data,size);
    Offset += size;
    Used = Offset;
    return 1;
  }

  return 0;
}

sU8 *sMemFile::Map(sS64 offset,sDInt size)
{
  if(offset>=0 && offset+size<=Size)
    return (sU8 *)(Data+offset);   // loose const!
  else
    return 0;
}

sBool sMemFile::SetOffset(sS64 offset)
{
  if(offset>=0 && offset<=Size)
  {
    Offset = sDInt(offset);
    return 1;
  }
  else
  {
    Offset = 0;
    return 0;
  }
}

sS64 sMemFile::GetOffset()
{
  return Offset;
}


sS64 sMemFile::GetSize()
{
  return Size;
}

sFileReadHandle sMemFile::BeginRead(sS64 offset,sDInt size,void *destbuffer, sFilePriorityFlags prio)
{
  sVERIFY(destbuffer);

  sInt alignedsize = sAlign(Size,BlockSize);
  if(offset+size>Size&&offset+size<=alignedsize)      // blockwise async read, return dummy 0 data if last block goes
  {                                                   // over size border
    sInt size2 = Size-offset;
    sCopyMem(destbuffer,Data+offset,size2);
    sSetMem(((sU8*)destbuffer)+size2,0,size-size2);
    return 1;
  }

  sVERIFY(offset+size<=Size);

  sCopyMem(destbuffer,Data+offset,size);
  return 1;
}

sBool sMemFile::DataAvailable(sFileReadHandle handle)
{
  return handle==1;
}

void sMemFile::EndRead(sFileReadHandle handle)
{
}

/****************************************************************************/

sFile *sCreateMemFile(const void *data,sDInt size,sBool owndata,sInt blocksize/*=1*/)
{
  return new sMemFile(data,size,owndata,blocksize);
}

sFile *sCreateMemFile(void *data,sDInt size,sBool owndata,sInt blocksize/*=1*/,sFileAccess access/*=sFA_READ*/)
{
  return new sMemFile(data,size,owndata,blocksize,access);
}

sFile *sCreateMemFile(const sChar *name)
{
  sFile *file = sCreateFile(name);

  if(!file) return 0;

  sDInt size = file->GetSize();
  sU8* data = new sU8[size];
  if(!file->Read(data,size))
  {
    sDelete(file);
    sDeleteArray(data);
    return 0;
  }
  sDelete(file);
  return new sMemFile(data,size,sTRUE);
}

/****************************************************************************/
/****************************************************************************/

class sGrowMemFile : public sFile
{
  sU8 *Data;
  sDInt Used;
  sDInt Alloc;
  sDInt Offset;
public:
  sGrowMemFile();
  ~sGrowMemFile();
  sBool Read(void *data,sDInt size);
  sBool Write(const void *data,sDInt size);
  sU8 *Map(sS64 offset,sDInt size);
  sBool SetOffset(sS64 offset);
  sS64 GetOffset();
  sS64 GetSize();
};

sGrowMemFile::sGrowMemFile()
{
  Alloc = 0x10000;
  Used = 0;
  Data = new sU8[Alloc];
  Offset = 0;
}

sGrowMemFile::~sGrowMemFile()
{
  delete[] Data;
}

sBool sGrowMemFile::Read(void *data,sDInt size)
{
  if(Offset+size<=Used)
  {
    sCopyMem(data,Data+Offset,size);
    Offset += size;
    return 1;
  }
  else
  {
    return 0;
  }
}

sBool sGrowMemFile::Write(const void *data,sDInt size)
{
  // enlarge buffer

  if(Offset+size > Alloc)
  {
    sInt newsize = sMax(Alloc*2,Offset+size);
    sU8 *newdata = new sU8[newsize];
    sCopyMem(newdata,Data,Used);
    delete[] Data;
    Data = newdata;
    Alloc = newsize;
  }

  // fill uninitialized file with zeros

  if(Offset > Used)
  {
    sSetMem(Data+Used,0,Offset-Used);
    Used = Offset;
  }

  // write

  sCopyMem(Data+Offset,data,size);
  Offset += size;
  Used = Offset;
  return 1;
}

sU8 *sGrowMemFile::Map(sS64 offset,sDInt size)
{
  if(offset>=0 && offset+size<=Used)
    return Data+offset;
  else
    return 0;
}

sBool sGrowMemFile::SetOffset(sS64 offset)
{
  Offset = sDInt(offset);
  return 1;
}

sS64 sGrowMemFile::GetOffset()
{
  return Offset;
}

sS64 sGrowMemFile::GetSize()
{
  return Used;
}

/****************************************************************************/

class sFile *sCreateGrowMemFile()
{
  return new sGrowMemFile;
}


/****************************************************************************/

sBool sCopyFileFailsafe(const sChar *source,const sChar *dest,sBool failifexists/*=0*/)
{
  sString<sMAXPATH> temp;
  temp.PrintF(L"%s_%016x.fail.tmp",dest,sGetTimeUS());
  sBool result = sCopyFile(source,temp,failifexists);
  if(result)
    return sRenameFile(temp,dest,!failifexists);
  return result;
}

/****************************************************************************/
/***                                                                      ***/
/***   sFileCalcMD5                                                       ***/
/***                                                                      ***/
/****************************************************************************/

sCalcMD5File::sCalcMD5File()
{
  TempCount = 0;
  TotalCount = 0;
  Checksum.CalcBegin();
}

sCalcMD5File::~sCalcMD5File()
{
}

sBool sCalcMD5File::Close()
{
  TotalCount += TempCount;
  Checksum.CalcEnd(TempBuf,TempCount,TotalCount);
  return sTRUE;
}

sBool sCalcMD5File::Read(void *data,sDInt size)
{
  sFatal(L"sFileMakeMD5::Read not supported");
  return sFALSE;
}

sBool sCalcMD5File::Write(const void *data,sDInt size)
{
  const sU8 *ptr = (const sU8*) data;
  sDInt count = size;
  sInt done = 0;

  if(TempCount)
  {
    // insert temp data from last write
    sInt add = sMin(size,sCOUNTOF(TempBuf)-TempCount);
    sCopyMem(TempBuf+TempCount,ptr,add);

    ptr += add;
    TempCount += add;
    count -= add;

    done = Checksum.CalcAdd(TempBuf,TempCount);
    TotalCount += done;

    if(done<TempCount)
    {
      sVERIFY(add==size);
      // size was smaller than tempbuffer, we are done
      sCopyMem(TempBuf,TempBuf+done,TempCount-done);
      TempCount -= done;
      return sTRUE;
    }

    TempCount = 0;
  }

  // calc checksum
  done = Checksum.CalcAdd(ptr,count);
  ptr += done;
  TotalCount += done;
  count -= done;

  // any temp data
  if(count>0)
  {
    TempCount = count;
    sVERIFY(TempCount>0 && TempCount<sCOUNTOF(TempBuf));
    sCopyMem(TempBuf,ptr,TempCount);
  }

  return sTRUE;
}

sU8 *sCalcMD5File::Map(sS64 offset,sDInt size)
{
  return 0;
}

sBool sCalcMD5File::SetOffset(sS64 offset)
{
  sFatal(L"sFileMakeMD5::SetOffset not supported\n");
  return sFALSE;
}

sS64 sCalcMD5File::GetOffset()
{
  return TotalCount;
}

sBool sCalcMD5File::SetSize(sS64)
{
  sFatal(L"sFileMakeMD5::SetOffset not supported\n");
  return sFALSE;
}

sS64 sCalcMD5File::GetSize()
{
  return TotalCount;
}

/****************************************************************************/
/***                                                                      ***/
/***   File System Wrappers                                               ***/
/***                                                                      ***/
/****************************************************************************/

sU8 *sLoadFile(const sChar *name)
{
  sDInt size;
  return sLoadFile(name,size);
}

sU8 *sLoadFile(const sChar *name,sDInt &size)
{
  sFile *file = sCreateFile(name,sFA_READ);
  if(file==0)
    return 0;
  size = file->GetSize();
  sVERIFY(size<=0x7fffffff);

  const sInt align = 16;
  sU8 *mem = (sU8 *)sAllocMem(size,align,0);

  if(!file->Read(mem,size))
    sDeleteArray(mem);
  delete file;
  return mem;
}

sChar *sLoadText(const sChar *name)
{
  sDInt size;
  sChar *mem,*d,*s16;
  sInt count;
  sU8 *s8;

  sU8 *data = sLoadFile(name,size);
  s16 = (sChar *) data;

  if(!data) return 0;

  // handles little and big endian cpu
  // handles little and big endian file

  if(s16[0]==0xfeff)    // cpu endian == file endian
  {
    d = (sChar *) data;
    s16 = d+1;
    count = size/2-1;
    for(sInt i=0;i<count;i++)
    {
      if(*s16!='\r')
        *d++ = *s16;
      s16++;
    }
    *d++ = 0;
    return (sChar *)data;
  }
  else if(s16[0]==0xfffe)    // cpu endian != file endian
  {
    d = (sChar *) data;
    s16 = d+1;
    count = size/2-1;
    for(sInt i=0;i<count;i++)
    {
      sChar v = *s16; v = ((v>>8)|(v<<8))&0xffff;
      if(v!='\r')
        *d++ = v;
      s16++;
    }
    *d++ = 0;
    return (sChar *)data;
  }
  else if(data[0]==0xef && data[1]==0xbb && data[2]==0xbf)  // UTF8
  {
    d = mem = (sChar *) sAllocMem((size-3+1)*sizeof(sChar),2,sAMF_ALT);
    s8 = data+3;
    const sU8 *e = data+size;

    // this does not check for overlong encodings, but checks for other invalid utf8

    while(s8<e)
    {
      if((*s8 & 0x80)==0x00)
      {
        if(*s8 != '\r')   // no fucking dos CR!
          *d++ = s8[0];
        s8+=1;
      }
      else if((*s8 & 0xc0)==0x80)
      {
        *d++ = '?';       // continuation byte without prefix code
        s8+=1;
      }
      else if((*s8 & 0xe0)==0xc0)
      {
        if(s8+2<=e && (s8[1]&0xc0)==0x80)
        {
          *d++ = ((s8[0]&0x1f)<<6)|(s8[1]&0x3f);
        }
        else
        {
          *d++= '?';      // eof or invalid continuation
        }
        s8+=2;
      }
      else if((*s8 & 0xf0)==0xe0)
      {
        if(s8+3<=e && (s8[1]&0xc0)==0x80 && (s8[2]&0xc0)==0x80)
        {
          *d++ = ((s8[0]&0x0f)<<12)|((s8[1]&0x3f)<<6)|(s8[2]&0x3f);
        }
        else
        {
          *d++= '?';      // eof or invalid continuation
        }
        s8+=3;
      }
      else if((*s8 & 0xf8)==0xf0)
      {
        *d++ = '?';       // valid utf8, but can't decode them into 16 bit
        s8+=4;
      }
      else
      {
        *d++ = '?';       // invalid prefix code
        s8+=1;
      }
    }
    *d++ = 0;
    delete[] data;
    return mem;
  }
  else    // 8 bit -> 16 bit
  {
    count = size;
    d = mem = (sChar *) sAllocMem((count+1)*sizeof(sChar),2,sAMF_ALT);
    s8 = data;

    for(sInt i=0;i<count;i++)
    {
      if(*s8!='\r')
        *d++ = *s8;
      s8++;
    }
    *d++ = 0;
    delete[] data;
    return mem;
  }
}


sBool sSaveFile(const sChar *name,const void *data,sDInt bytes)
{
  sFile *file=sCreateFile(name,sFA_WRITE);
  if (!file) return 0;
  sBool ret=file->Write(data,bytes);
  delete file;
  return ret;
}

sBool sSaveFileFailsafe(const sChar *name,const void *data,sDInt bytes)
{
  sFile *file = sCreateFailsafeFile(name,sFA_WRITE);
  if(!file) return 0;
  sBool ret = file->Write(data,bytes);
  ret &= file->Close();
  delete file;
  return ret;
}

sBool sSaveTextUnicode(const sChar *name,const sChar *data)
{
  sFile *file=sCreateFile(name,sFA_WRITE);
  if (!file) return 0;
  sU16 code = 0xfeff;
  file->Write(&code,2);
  sBool ret=file->Write(data,sGetStringLen(data)*2);
  delete file;
  return ret;
}

sBool sSaveTextUTF8(const sChar *name,const sChar *data)
{
  sInt len = sGetStringLen(data);
  sU8 *buffer = new sU8[len*4+4];

  sU8 *d = buffer;
  *d++ = 0xef;
  *d++ = 0xbb;
  *d++ = 0xbf;
  for(sInt i=0;i<len;i++)
  {
    sInt c = data[i]&0xffff;
    if(c<0x80)
    {
      *d++ = c;
    }
    else if(c<0x0800)
    {
      *d++ = 0xc0 | ((c>> 6)&0x1f);
      *d++ = 0x80 | ( c     &0x3f);
    }
    else
    {
      *d++ = 0xe0 | ((c>>12)&0x1f);
      *d++ = 0x80 | ((c>> 6)&0x3f);
      *d++ = 0x80 | ( c     &0x3f);
    }
  }
  *d++ = 0x00;
  sBool ok = sSaveFile(name,buffer,d-buffer);
  delete[] buffer;
  return ok;
}

sBool sSaveTextAnsi(const sChar *name,const sChar *data)
{
  sFile *file = sCreateFile(name,sFA_WRITE);
  if(!file)
    return 0;

  const sInt BUFFER_SIZE = 2048;
  sU8 buffer[BUFFER_SIZE];
  sBool lastWasCR = sFALSE;

  // convert in small chunks
  while(*data)
  {
    sInt pos = 0;
    while(*data && pos < BUFFER_SIZE - 2)
    {
      if(sCONFIG_SYSTEM_WINDOWS)
      {
        if(*data == '\n' && !lastWasCR) // convert LF to CRLF on windows
          buffer[pos++] = '\r';

        lastWasCR = *data == '\r';
      }
      buffer[pos++] = *data++ & 0xff;
    }

    if(!file->Write(buffer,pos))
    {
      delete file;
      return 0;
    }
  }

  delete file;
  return 1;
}

sBool sSaveTextAnsiIfDifferent(const sChar *name,const sChar *data)
{
  sString<sMAXPATH> origname;
  sString<sMAXPATH> tempname;
  
  // save new file
  origname = name;
  tempname=origname;
  tempname.Add(L"_new");
  if(!sSaveTextAnsi(tempname,data))
    return sFALSE;

  if (sCheckFile(origname))
  {
    // identical to current version?
    if(sFilesEqual(tempname,origname))
    {
      sDPrintF(L"new version of <%s> identical to old one, not saving.\n",name);
      sDeleteFile(tempname);
      return sTRUE;
    }

    // not identical, delete original file
//    sPrintF(L"!!! WRITE %q\n",name);
    if(!sDeleteFile(origname))
      return sFALSE;
  }

  // then rename temp to target filename
  if(!sRenameFile(tempname,origname))
  {
    sDeleteFile(tempname);
    return sFALSE;
  }

  return sTRUE;
}

sBool sFilesEqual(const sChar *name1,const sChar *name2)
{
  sFile *file1 = sCreateFile(name1);
  sFile *file2 = sCreateFile(name2);
  if(!file1 || !file2 || file1->GetSize() != file2->GetSize())
  {
    sDelete(file1);
    sDelete(file2);
    return sFALSE;
  }

  static const sInt bufferSize = 1024;
  sS64 remainingSize = file1->GetSize();
  sU8 buffer1[bufferSize],buffer2[bufferSize];

  while(remainingSize)
  {
    sInt chunkSize = (sInt) sMin<sS64>(remainingSize,bufferSize);
    if(!file1->Read(buffer1,chunkSize) || !file2->Read(buffer2,chunkSize))
      break;

    if(sCmpMem(buffer1,buffer2,chunkSize) != 0)
      break;

    remainingSize -= chunkSize;
  }

  sDelete(file1);
  sDelete(file2);
  return remainingSize == 0;
}

#if sPLATFORM==sPLAT_WINDOWS && sPLATFORM==sPLAT_LINUX // this is obvisously stupid...

sBool sBackupFile(const sChar *name)
{
  sArray<sDirEntry> dir;
  sDirEntry *ent;
  sInt len;
  sInt best,val;
  sString<sMAXPATH> buffer;

  if(!sLoadDir(dir,L"backup"))
    if(!sMakeDir(L"backup")) return 0;

  buffer = sFindFileWithoutPath(name);
  buffer.Add(L"#");
  len = sGetStringLen(buffer);

  best = 0;

  sFORALL(dir,ent)
  {
    if(sGetStringLen(ent->Name)>len && sCmpStringILen(ent->Name,(const sChar *)buffer,len)==0)
    {
      const sChar *scan = ent->Name+len;
      if(sScanInt(scan,val) && val>best)
        best = val;
    }
  }
  sSPrintF(buffer,L"backup/%s#%04d",sFindFileWithoutPath(name),best+1);
  return sCopyFile(name,buffer);
}

#endif

sBool sFileCalcMD5(const sChar *name, sChecksumMD5 &md5)
{
  const sInt BufSize = 65536;
  sU8 buffer[BufSize];

  sFile *file = sCreateFile(name);
  if(file)
  {
    md5.CalcBegin();
    sS64 size = file->GetSize();
    sInt left = size;
    while(left>BufSize)
    {
      if(!file->Read(buffer,BufSize))
        goto failed;
      md5.CalcAdd(buffer,BufSize);
      left -= BufSize;
    }
    if(left)
      if(!file->Read(buffer,left))
        goto failed;

    md5.CalcEnd(buffer,left,sInt(size));
    sDelete(file);
    return sTRUE;
  }
failed:
  md5.Hash[0] = 0;
  md5.Hash[1] = 0;
  md5.Hash[2] = 0;
  md5.Hash[3] = 0;
  sDelete(file);
  return sFALSE;
}

/****************************************************************************/

// this can parse strings in the form:
// "aa/bb/cc"        relative paths
// "aa\bb\cc"        both kinds of slashes
// "aa/bb/cc/"       trailing '/' optional
// "aa:/bb/cc"       windows absolute paths
// "//aa/bb/cc"      windows network paths
// "/aa/bb/cc"       unix absolute paths

sBool sMakeDirAll(const sChar *path)
{
  sString<sMAXPATH> canon;
  sString<sMAXPATH> test;
  sInt len = sGetStringLen(path);
  sInt pos;

  // remove trailing '/'

  if(len>0 && (path[len-1]=='/' || path[len-1]=='\\'))
    len--;

  // too long or empty strings

  if(len>=sMAXPATH-1 && len<1)    
    return 0;

  // convert slashes

  for(sInt i=0;i<len;i++)
  {
    if(path[i]=='\\')
      canon[i] = '/';
    else
      canon[i] = path[i];
  }
  canon[len] = 0;

  // special case: network paths

  sInt skip = 0;
  if(canon[0]=='/' && canon[1]=='/')
    skip = 1;

  // handle "xx:/", "//" and "/"

  pos = sFindLastChar(canon,':');
  if(pos==-1)
    pos = 0;
  else
    pos++;
  while(canon[pos]=='/')
    pos++;

  // now the remainder is in the form "aa/bb/cc\0"

  sBool ok = 1;
  while(canon[pos] && ok)
  {
    // find next dir

    while(canon[pos]!='/' && canon[pos]!=0)
      pos++;
    test = canon;
    test[pos] = 0;

    // check and create
    
    if(skip==0)
    {
      if(!sCheckDir(test))
        ok = sMakeDir(test);
    }
    else
    {
      skip--;
    }

    // next

    if(canon[pos]!=0)
      pos++;
  }
  
  // done

  return ok;
}

/****************************************************************************/
/***                                                                      ***/
/***   input2 device management                                           ***/
/***                                                                      ***/
/****************************************************************************/

#define INPUT2_EVENT_QUEUE_SIZE 64
#define INPUT2_MAX_DEVICES 16
sDList<sInput2Device, &sInput2Device::DNode> sInputDevice;
sLockQueue<sInput2Event, INPUT2_EVENT_QUEUE_SIZE>* sInput2EventQueue = 0;

sInput2Device* sFindInput2Device(sU32 type, sInt id)
{
  sVERIFY2((type!=sINPUT2_TYPE_CUSTOM),L"nope, you can't find custom evaluators");
  if (type==sINPUT2_TYPE_CUSTOM) return sNULL;

  sInput2Device *dev;
#if !sSTRIPPED
  if (sGetShellSwitch(L"fourctrls"))
  {
    sFORALL_LIST(sInputDevice,dev)
      if (dev->GetType() == type && dev->GetId() == id && dev->GetStatusWord()==0)
        return dev;
      
    sFORALL_LIST(sInputDevice,dev)
      if (dev->GetType() == type && dev->GetStatusWord()==0)
        return dev;

    sFORALL_LIST(sInputDevice,dev)
      if (dev->GetType() == type)
        return dev;
  }
#endif

  sFORALL_LIST(sInputDevice,dev)
    if (dev->GetType() == type && dev->GetId() == id)
      return dev;

  return sNULL;
}

sInt sInput2NumDevices(sU32 type)
{
  sInt num = 0;
  sInput2Device *dev;
  sFORALL_LIST(sInputDevice,dev)
    if (dev->GetType() == type)
      num++;
  return num;
}

static sBool Input2Mute = sFALSE;
void sInput2SetMute(sBool mute)
{
  Input2Mute = mute;
}

void sInput2Update(sInt time, sBool ignoreTimestamp)
{
  sInput2Device *dev;
  sFORALL_LIST(sInputDevice,dev)
    dev->OnGameStep(time, ignoreTimestamp, Input2Mute);
}

void sInput2RegisterDevice(sInput2Device* device)
{
  // sort custom evaluators to the back and the rest to the front (order of OnGameStep calls)
  if (device->GetType() == sINPUT2_TYPE_CUSTOM)
    sInputDevice.AddTail(device);
  else
    sInputDevice.AddHead(device);
}

void sInput2RemoveDevice(sInput2Device* device)
{
  sInputDevice.Rem(device);
}

sInt sInput2NumDevicesConnected(sInt input2type) 
{
  sInt result = 0;

  sInt count = sInput2NumDevices(input2type);
  for(sInt i=0; i<count; ++i)
  {
    sInput2Device* device = sFindInput2Device(input2type, i);
    if(device)
    {
      if(!(device->GetStatusWord() & sINPUT2_STATUS_NOT_CONNECTED))
        result++;
    }
  }

  return result;

}

void sInput2SendEvent(const sInput2Event& event)
{
  if (sSystemFlags & sISF_CONTINUOUS) {
    // queue events when in game-mode
    sInput2EventQueue->AddTail(event);
  } else {
    // send events instantly when in tool-mode
#if !sSTRIPPED
    sBool skip = sFALSE;
    sInputHook->Call(event, skip);
    if (!skip)
#endif
    if (sAppPtr)
      sAppPtr->OnInput(event);
  }
}

sBool sInput2PopEvent(sInput2Event& event, sInt time)
{
  if(!sInput2EventQueue->IsEmpty() && time >= sInput2EventQueue->Front().Timestamp) {
    sInput2EventQueue->RemHead(event);
    return sTRUE;
  }
  return sFALSE;
}

sBool sInput2PeekEvent(sInput2Event& event, sInt time)
{
  if(!sInput2EventQueue->IsEmpty() && time >= sInput2EventQueue->Front().Timestamp) {
    event = sInput2EventQueue->Front();
    return sTRUE;
  }
  return sFALSE;
}

extern sU32 sKeyQual;

sInt sInput2CECounter;

void sInput2ClearQueue()
{
  sKeyQual = 0;
  while (!sInput2EventQueue->IsEmpty()) {
    sInput2Event e;
    sInput2EventQueue->RemHead(e);
  }
}

void sInitEventQueue()
{
  sInput2EventQueue = new sLockQueue<sInput2Event, INPUT2_EVENT_QUEUE_SIZE>;
}

void sExitEventQueue()
{
  sDelete(sInput2EventQueue);
}

sADDSUBSYSTEM(EventQueue, 0x01, sInitEventQueue, sExitEventQueue)

/****************************************************************************/

sBool sInput2IsKeyboardKey(sInt key)
{
  sInt pureKey = key & sKEYQ_MASK;
         // not in page 0xe000          or   key below keyboard_border
  return ((pureKey & 0xf000) != 0xe000) || (pureKey<=(sInt)sKEY_KEYBOARD_MAX);
}

/****************************************************************************/
/***                                                                      ***/
/***   Multithreading                                                     ***/
/***                                                                      ***/
/****************************************************************************/

sPtr sThreadContext::TlsOffset = sOFFSET(sThreadContext,Mem);
static const sPtr sTLsMaxSize = sizeof(sThreadContext);

sThreadContext *sCreateThreadContext(sThread *thread)
{
  sThreadContext *ctx = (sThreadContext *) new sU8[sTLsMaxSize];
  sSetMem(ctx,0,sTLsMaxSize);
  ctx->Thread = thread;
  ctx->ThreadName = L"unnamed";
  ctx->MemTypeStack[0] = sAMF_HEAP;
  return ctx;
}

sPtr sAllocTls(sPtr bytes,sInt align)
{
  sThreadContext::TlsOffset = sAlign(sThreadContext::TlsOffset,align);
  sPtr result = sThreadContext::TlsOffset;
  sThreadContext::TlsOffset += bytes;
  sVERIFY(sThreadContext::TlsOffset<=sTLsMaxSize);   // increase sMAX_APP_TLS
  return result;
}

/****************************************************************************/

sVideoWriter::~sVideoWriter()
{
}

/****************************************************************************/
