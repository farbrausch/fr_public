/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "base/types.hpp"

#if sCONFIG_SYSTEM_LINUX

/****************************************************************************/

#include "base/system.hpp"
#include "base/types2.hpp"
#include "base/windows.hpp"
#include "base/input2.hpp"

#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>
#include <dirent.h>
#include <fnmatch.h>

#include <time.h>

#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>
#include <locale.h>
#include <poll.h>

/****************************************************************************/

extern void sCollector(sBool exit=sFALSE);

#if !sCOMMANDLINE
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#undef Status // goddamnit!

XVisualInfo *sXVisualInfo;
Visual *sXVisual;
sInt sXScreen;
Colormap sXColMap;
Window sXWnd;
GC sXGC;

static Display *sXMainDisplay;
static sPtr sXDisplayTls;
#else
struct Display;
#endif

extern sInt sSystemFlags;
extern sInt sExitFlag;
extern sApp *sAppPtr;

sU32 sKeyQual;

/****************************************************************************/
/***                                                                      ***/
/***   External Modules                                                   ***/
/***                                                                      ***/
/****************************************************************************/

void Render3D();

void PreInitGFX(sInt &flags,sInt &xs,sInt &ys);
void InitGFX(sInt flags,sInt xs,sInt ys);
void ExitGFX();
void ResizeGFX(sInt x,sInt y);

/****************************************************************************/

static sInt ErrorCode = 0;

/****************************************************************************/
/***                                                                      ***/
/***   Unicode helper functions                                           ***/
/***                                                                      ***/
/****************************************************************************/

void sLinuxFromWide(char *dest,const sChar *src,int size)
{
  sInt len = sGetStringLen(src);
  sU32 *convBuffer = sALLOCSTACK(sU32,len+1);
  for(sInt i=0;i<=len;i++) // fake-wchar16-to-wchar32 (argh!)
    convBuffer[i] = src[i];
  
  wcstombs(dest,(wchar_t *)convBuffer,size);
  dest[size-1] = 0;
}

template<class T> static inline void sLinuxFromWide(T &dest,const sChar *str)
{
  sLinuxFromWide(dest,str,sizeof(dest));
}

// Careful with this, result is overwritten on next call, sDPrintF etc!!!
char *sLinuxFromWide(const sChar *str)
{
  sThreadContext *ctx = sGetThreadContext();
  char *buffer = (char *) &ctx->PrintBuffer[0];
  sLinuxFromWide(buffer,str,sizeof(ctx->PrintBuffer));
  
  return buffer;
}

// Includes home directory matching
static void FromWideFileName(char *dest,const sChar *src,int size)
{
  if(src[0] == L'~' && src[1] == L'/') // home directory expansion
  {
    // could do this using wordexp or glob; but I only want to do a minimum amount of
    // postprocessing on filenames so there's no need to start using escape codes...
    char *homedir = getenv("HOME");
    if(homedir)
    {
      int len = sMin<int>(strlen(homedir),size);
      memcpy(dest,homedir,len);
      dest += len;
      src++; // skip the tilde
      size -= len;
    }
    else
      sDPrintF(L"warning: $HOME not set during filename tilde expansion, not sure what to do!\n");
  }
  
  sLinuxFromWide(dest,src,size); // transform the rest
  
  // if the filename contains backslashes, replace them by slashes
  mbstate_t state;
  memset(&state,0,sizeof(state));
  
  int len = 0;
  while((len = mbrlen(dest,MB_CUR_MAX,&state)) > 0)
  {
    if(len==1 && *dest=='\\')
      *dest = '/';
    
    dest += len;
  }
}

template<class T> static inline void FromWideFileName(T &dest,const sChar *str)
{
  FromWideFileName(dest,str,sizeof(dest));
}

// Careful with this, result is overwritten on next call, sDPrintF etc!!!
static char *FromWideFileName(const sChar *str)
{
  sThreadContext *ctx = sGetThreadContext();
  char *buffer = (char *) &ctx->PrintBuffer[0];
  FromWideFileName(buffer,str,sizeof(ctx->PrintBuffer));
  
  return buffer;
}

void sLinuxToWide(sChar *dest,const char *src,int size)
{
  sU32 *convBuffer = sALLOCSTACK(sU32,size);
  size_t nconv = mbstowcs((wchar_t *)convBuffer,src,size);
  if(nconv == (size_t) -1) nconv = 0;
  
  for(size_t i=0;i!=nconv;i++) // fake-wchar32-to-wchar16
    dest[i]=convBuffer[i];
  dest[sMin<sInt>(nconv,size-1)] = 0;
}

template<class T> static inline void sLinuxToWide(T &dest,const char *src)
{
  sLinuxToWide(dest,src,sCOUNTOF(dest));
}

static sBool NormalizePathR(const sStringDesc &desc,sInt &inPos,sInt outPos,sInt count)
{
  // copy first part up to and including slash/null
  for(sInt i=0;i<count;i++)
    desc.Buffer[outPos++] = desc.Buffer[inPos++];
  
  while(desc.Buffer[inPos-1])
  {
    const sChar *cur = &desc.Buffer[inPos];
    
    if(*cur == L'/') // "//" = nop
      inPos++;
    else if(sCmpStringLen(cur,L"./",2) == 0
            || sCmpString(cur,L".") == 0) // "./" = nop
      inPos += 1 + (cur[1] != 0);
    else if(sCmpStringLen(cur,L"../",3) == 0
            || sCmpString(cur,L"..") == 0) // "../" = pop
    {
      inPos += 2 + (cur[2] != 0);
      return sFALSE;
    }
    else
    {
      sInt count = 0;
      while(cur[count] && cur[count]!=L'/')
        count++;
      
      NormalizePathR(desc,inPos,outPos,count+1);
    }
  }
  
  return sTRUE;
}

static void NormalizePath(const sStringDesc &desc)
{
  sInt slashPos = sFindFirstChar(desc.Buffer,L'/');
  if(slashPos == -1)
  {
    desc.Buffer[0] = 0;
    return; // nothing to do if no slashes
  }
  
  sInt inPos=0;
  if(!NormalizePathR(desc,inPos,0,slashPos+1))
    desc.Buffer[0] = 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   Debugging (declared in types.hpp!)                                 ***/
/***                                                                      ***/
/****************************************************************************/

void sCDECL sFatalImpl(const sChar *str)
{
  sPrint(L"FATAL ERROR: ");
  sPrint(str);
  sPrint(sTXT("\n"));

  exit(1);
}

static int sDebugFile = -1;
static sBool sDPrintFlag=sFALSE;

void sDPrint(const sChar *text)
{
  sThreadContext *tc=sGetThreadContext();
  if (tc->Flags & sTHF_NODEBUG) return;
#if !sSTRIPPED
  if (sDebugOutHook && !(tc->Flags&sTHF_NONETDEBUG))
    sDebugOutHook->Call(text);
#endif

  sInt size = sGetStringLen(text)*2+1;
  sChar8 *buffer = sALLOCSTACK(sChar8,size);
  sLinuxFromWide(buffer,text,size);
  
#if sCOMMANDLINE
  if(sDebugFile!=-1)
  {
    ssize_t cnt = write(sDebugFile,buffer,strlen(buffer));
    cnt = cnt; // unused
  }
#else
  fputs(buffer,stderr);
#endif
  
  sDPrintFlag=sTRUE;
}


void sPrint(const sChar* text)
{
  sChar8 buffer[2048];
  sLinuxFromWide(buffer,text);
  fputs(buffer,stdout);
}

static volatile sBool sCtrlCFlag = sFALSE;

static void sCtrlCHandler(int signal)
{
  sCtrlCFlag = sTRUE;
}

void sCatchCtrlC(sBool enable)
{
  struct sigaction action;
  
  action.sa_handler = enable ? sCtrlCHandler : SIG_DFL;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  sigaction(SIGINT,&action,0);
  sigaction(SIGTERM,&action,0);
}

sBool sGotCtrlC()
{
  return sCtrlCFlag;
}

void sSetErrorCode(sInt code)
{
  ErrorCode = code;
}

sInt sStartTime = 0;

void sInitGetTime()
{
  timespec currentTime;
  clock_gettime(CLOCK_REALTIME, &currentTime);

  sStartTime = currentTime.tv_sec * 1000 + currentTime.tv_nsec/1000000;
}

sInt sGetTime()
{
  timespec currentTime;
  clock_gettime(CLOCK_REALTIME, &currentTime);

  return currentTime.tv_sec * 1000 + currentTime.tv_nsec/1000000 - sStartTime;
}

sU64 sGetTimeUS()
{
  timespec currentTime;
  clock_gettime(CLOCK_REALTIME, &currentTime);
  return sU64(currentTime.tv_sec) * 1000000UL + currentTime.tv_nsec/1000;
}

static sDateAndTime TimeFromSystemTime(const struct tm *t)
{
  sDateAndTime r;

  r.Year = t->tm_year + 1900;
  r.Month = t->tm_mon + 1;
  r.Day = t->tm_mday;
  r.DayOfWeek = t->tm_wday;
  r.Hour = t->tm_hour;
  r.Minute = t->tm_min;
  r.Second = t->tm_sec;
  return r;
}

static void TimeToSystemTime(struct tm *dest,const sDateAndTime &t)
{
  dest->tm_year = t.Year - 1900;
  dest->tm_mon = t.Month - 1;
  dest->tm_mday = t.Day;
  dest->tm_hour = t.Hour;
  dest->tm_min = t.Minute;
  dest->tm_sec = t.Second;
  dest->tm_isdst = -1;
}

sDateAndTime sGetDateAndTime()
{
  time_t result = time(0);
  struct tm *t = localtime(&result);
  return TimeFromSystemTime(t);
}

sDateAndTime sAddLocalTime(sDateAndTime origin,sInt seconds)
{
  struct tm t;
  TimeToSystemTime(&t,origin);
  t.tm_sec += seconds; // mktime supports this properly! (phew.)
  time_t result = mktime(&t);
  struct tm *t2 = localtime(&result);
  return TimeFromSystemTime(t2);
}

sS64 sDiffLocalTime(sDateAndTime a,sDateAndTime b)
{
  struct tm ta,tb;
  TimeToSystemTime(&ta,a);
  TimeToSystemTime(&tb,b);
  return (sS64) difftime(mktime(&ta),mktime(&tb));
}

sDateAndTime sFromFileTime(sU64 lastWriteTime)
{
  time_t time = lastWriteTime;
  struct tm *t = localtime(&time);
  return TimeFromSystemTime(t);
}

sU64 sToFileTime(sDateAndTime time)
{
  struct tm t;
  TimeToSystemTime(&t,time);
  return mktime(&t);
}

sBool sStackTrace(const sStringDesc &tgt,sInt skipCount,sInt maxCount)
{
  return sFALSE; // no stack traces on linux yet.
}

static char sLogAppName[256];
static int sLogFacility=LOG_USER;

void sSysLogInit(const sChar *appname)
{
  strncpy(sLogAppName,sLinuxFromWide(appname),256);
  sLogAppName[255] = 0; // apparently, openlog stores the *pointer* to the name.
  sLogFacility = LOG_LOCAL0;
  openlog(sLogAppName,LOG_CONS,sLogFacility);
}

void sSysLog(const sChar *module,const sChar *text)
{
  char mod[64];
  char txt[1024];
  
  sLinuxFromWide(mod,module ? module : L"");
  sLinuxFromWide(txt,text);

  if(module)
    syslog(sLogFacility|LOG_NOTICE,"[%s] %s",mod,txt);
  else
    syslog(sLogFacility|LOG_NOTICE,"%s",txt);
}

sBool sDaemonize()
{
  return daemon(1,1) == 0;
}

sInt sGetRandomSeed()
{
  sChecksumMD5 check;
  time_t tm = time(0);
  int bytes = sizeof(tm);
  
  check.CalcBegin();
  
  FILE *f = fopen("/dev/random","r");
  if(f)
  {
    sU8 buffer[4] = { 0 };
    if(fread(&buffer,1,4,f) == 4)
    {
      check.CalcAdd(buffer,4);
      bytes += 4;
    }
    
    fclose(f);
  }

  check.CalcEnd((const sU8 *)&tm,sizeof(tm),bytes);
  return check.Hash[0];
}

sBool sExecuteShell(const sChar *cmdline)
{
  sPrintF(L"sExecuteShell(\"%s\")\n",cmdline);
  return sFALSE;
}

sBool sExecuteShellDetached(const sChar *cmdline)
{
  sPrintF(L"sExecuteShellDetached(\"%s\")\n",cmdline);
  return sFALSE;
}

sBool sExecuteShell(const sChar *cmdline,sTextBuffer *tb)
{
  sPrintF(L"sExecuteShell(\"%s\",tb)\n",cmdline);
  return sFALSE;
}

sBool sExecuteOpen(const sChar *file)
{
  sPrintF(L"Implement sExecuteOpen!\n");
  return sFALSE;
}

/****************************************************************************/
/***                                                                      ***/
/***   File Operation                                                     ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/

class sRootFileHandler : public sFileHandler
{
  sFile *Create(const sChar *name,sFileAccess access);
  sBool Exists(const sChar *name);
};

class sRootFile : public sFile
{
  int File;
  sFileAccess Access;
  sS64 Size;
  sS64 Offset;
  sBool Ok;
  
  sS64 MapOffset;
  sDInt MapSize;
  sU8 *MapPtr;             // 0 = mapping not active
  sBool MapFailed;          // 1 = we tryed once to map and it didn't work, do don't try again
  sRootFileHandler *Handler;
  
public:
  sRootFile(int file,sFileAccess access,sRootFileHandler *h);
  ~sRootFile();
  sBool Close();
  sBool Read(void *data,sDInt size); 
  sBool Write(const void *data,sDInt size);
  sU8 *Map(sS64 offset,sDInt size);
  sBool SetOffset(sS64 offset);
  sS64 GetOffset();
  sBool SetSize(sS64);
  sS64 GetSize();
};

static void sAddRootFilesystem()
{
  sAddFileHandler(new sRootFileHandler);
}

sADDSUBSYSTEM(RootFileHandler,0x28,sAddRootFilesystem,0);

/****************************************************************************/

sBool sRootFileHandler::Exists(const sChar *name)
{
  struct stat st;
  return stat(FromWideFileName(name),&st) == 0;
}

sFile *sRootFileHandler::Create(const sChar *name,sFileAccess access)
{
  int fd = -1;
  static const sChar *mode[] =  {  0,L"read",L"readrandom",L"write",L"writeappend",L"readwrite"  };
  
  const char *u8name = FromWideFileName(name);
  
  switch(access)
  {
  case sFA_READ:
  case sFA_READRANDOM:
    fd = open(u8name,O_RDONLY);
    break;
  case sFA_WRITE:
    fd = open(u8name,O_WRONLY|O_CREAT|O_TRUNC,0644);
    break;
  case sFA_WRITEAPPEND:
    fd = open(u8name,O_WRONLY|O_CREAT,0644);
    break;
  case sFA_READWRITE:
    fd = open(u8name,O_RDWR|O_CREAT,0644);
    break;
  default:
    sVERIFYFALSE;
    return 0;
  }
  
  if(fd!=-1)
  {
    sLogF(L"file",L"open file <%s> for %s\n",name,mode[access]);
    sFile *file = new sRootFile(fd,access,this);
    return file;
  }
  else 
  {
    sLogF(L"file",L"failed to open file <%s> for %s\n",name,mode[access]);
    return 0;
  }
}

/****************************************************************************/

sRootFile::sRootFile(int file,sFileAccess access,sRootFileHandler *h)
{
  File = file;
  Access = access;
  Handler = h;
  Offset = 0;
  Ok = 1;
  MapOffset = 0;
  MapSize = 0;
  MapPtr = 0;
  MapFailed = 0;

  // get file size
  Size = lseek64(File,0,SEEK_END);
  Ok = (Size != -1);
  lseek64(File,0,SEEK_SET);
}

sRootFile::~sRootFile()
{
  if(File!=-1)
    Close();
}

sBool sRootFile::Close()
{
  sVERIFY(File!=-1);
  
  if(MapPtr && munmap(MapPtr,MapSize) != 0)
    Ok = 0;
  if(close(File) != 0)
    Ok = 0;
  File = -1;
  return Ok;
}

sBool sRootFile::Read(void *data,sDInt size)
{
  sVERIFY(File!=-1)
  sVERIFY(size<=0x7fffffff);

  sBool result = sTRUE;
  ssize_t rd = read(File,data,size);
  if(rd == -1 || rd != size)
    result = sFALSE;
  
  if(!result) Ok = 0;
  Offset += (rd != -1) ? rd : 0;
  return result;
}

sBool sRootFile::Write(const void *data,sDInt size)
{
  sVERIFY(File!=-1);
  sVERIFY(size<=0x7fffffff);
  
  sBool result = sTRUE;
  ssize_t wr = write(File,data,size);
  if(wr == -1 || wr != size)
    result = sFALSE;
  
  if(!result) Ok = 0;
  Offset += (wr != -1) ? wr : 0;
  return result;
}

sU8 *sRootFile::Map(sS64 offset,sDInt size)
{
  sVERIFY(File != -1);
  
  // try mapping only once
  if(MapFailed) return 0;
  
  // prepare mapping
  if(Access!=sFA_READ && Access!=sFA_READRANDOM) // only supported for reading
  {
    MapFailed = 1;
    return 0;
  }
  
  // unmap current view
  if(MapPtr)
    munmap(MapPtr,MapSize);
  MapPtr = 0;
  
  // map new view
  if(size>=0x7fffffff) return 0;
  MapPtr = (sU8*) mmap64(0,size,PROT_READ,MAP_PRIVATE|MAP_FILE,File,offset);
  if(MapPtr != (sU8*) MAP_FAILED)
  {
    MapOffset = offset;
    MapSize = size;
  }
  else
  {
    MapPtr = 0;
    MapOffset = 0;
    MapSize = 0;
  }
  
  return MapPtr;
}

sBool sRootFile::SetOffset(sS64 offset)
{
  sVERIFY(File!=-1)
  
  Offset = offset;
  sBool result = (lseek64(File,offset,SEEK_SET) == offset);
  if(!result) Ok = 0;
  return result;
}

sS64 sRootFile::GetOffset()
{
  return Offset;
}

sBool sRootFile::SetSize(sS64 size)
{
  sVERIFY(File!=-1);
  if(Ok)
  {
    Size = size;
    Ok &= (ftruncate64(File,Size) == 0);
    Ok &= SetOffset(sMin(Size,Offset));
  }
  return Ok;
}

sS64 sRootFile::GetSize()
{
  return Size;
}

/****************************************************************************/

sBool sLoadDir(sArray<sDirEntry> &list,const sChar *path,const sChar *pattern)
{
  if(!pattern) pattern = L"*";
  
  char u8Pattern[1024];
  FromWideFileName(u8Pattern,pattern);
  
  const char *pathconv = strdupa(FromWideFileName(path));
  int pathclen = strlen(pathconv);
  char *fnbuf = sALLOCSTACK(char,pathclen+256+2);
  
  DIR *dir = opendir(pathconv);
  if(dir)
  {
    strcpy(fnbuf,pathconv);
    if(pathclen && fnbuf[pathclen-1] != '/')
    {
      fnbuf[pathclen] = '/';
      pathclen++;
    }
    
    dirent *entry;
    while((entry = readdir(dir)) != 0)
    {
      if(strcmp(entry->d_name,".") == 0 || strcmp(entry->d_name,"..") == 0)
        continue;
      
      if(fnmatch(u8Pattern,entry->d_name,0) == 0) // matches
      {
        struct stat64 st;
        
        strncpy(fnbuf+pathclen,entry->d_name,256);
        fnbuf[pathclen+256] = 0;
        
        if(stat64(fnbuf,&st) == 0)
        {
          sDirEntry *de = list.AddMany(1);
          de->Flags = sDEF_EXISTS;
          if(S_ISDIR(st.st_mode)) de->Flags |= sDEF_DIR;
          if(access(fnbuf,W_OK) != 0) de->Flags |= sDEF_WRITEPROTECT;
          
          de->Size = st.st_size;
          de->LastWriteTime = st.st_mtime;
          sLinuxToWide(de->Name,entry->d_name);
          
          if(!de->Name[0]) // if conversion failed, don't add the file! far better than reporting bullshit.
            list.RemTail();
        }
      }
    }
    
    closedir(dir);
    return sTRUE;
  }
  else
    return sFALSE;
}

sBool sChangeDir(const sChar *name)
{
  const char *dir = FromWideFileName(name);
  if(dir[0] == '~' && !dir[1]) // home dir
    dir = getenv("HOME");
  
  return chdir(dir) == 0;
}

void sGetCurrentDir(const sStringDesc &str)
{
  char *dir = getcwd(0,0);
  sLinuxToWide(str.Buffer,dir,str.Size);
  free(dir);
}

void sGetTempDir(const sStringDesc &str)
{
  sCopyString(str,"/tmp");
}

sBool sMakeDir(const sChar *name)
{
  return mkdir(FromWideFileName(name),0733) == 0;
}

sBool sCheckDir(const sChar *name)
{
  DIR *dir = opendir(FromWideFileName(name));
  if(dir)
    closedir(dir);
  
  return dir != 0;
}

sBool sCopyFile(const sChar *source,const sChar *dest,sBool failifexists)
{
  sLogF(L"file",L"copy from <%s>\n",source);
  sLogF(L"file",L"copy to   <%s>\n",dest);
  
  static const sInt bufSize=65536;
  sU8 *buf = new sU8[bufSize];
  if(!buf)
  {
    sLogF(L"file",L"copy failed: couldn't allocate temp buffer!\n");
    return sFALSE;
  }
  
  sBool ok = sFALSE;  
  int fdin = open(FromWideFileName(source),O_RDONLY);
  if(fdin >= 0)
  {
    int flags = O_WRONLY|O_CREAT|O_TRUNC;
    if(failifexists)
      flags |= O_EXCL;
  
    int fdout = open(FromWideFileName(dest),flags,0644);
    if(fdout >= 0)
    {
      int nread;
      while((nread = read(fdin,buf,bufSize)) > 0)
      {
        if(write(fdout,buf,bufSize) != nread)
        {
          // Use nread of -1 to signal error
          nread = -1;
          break;
        }
      }
    
      close(fdout);
      
      if(nread < 0) // error copying?
      {
        sLogF(L"file",L"error copying file!\n");        
        // Try to remove the partially copied output file if possible
        unlink(FromWideFileName(dest));
      }
      else
        ok = sTRUE;
    }
    else
      sLogF(L"file",L"copy failed: error opening output file!\n");
    
    close(fdin);
  }
  else
    sLogF(L"file",L"copy failed: error opening input file!\n");
  
  delete[] buf;
  return ok;
}

sBool sRenameFile(const sChar *source,const sChar *dest, sBool overwrite/*=sFALSE*/)
{
  sLogF(L"file",L"rename from <%s>\n",source);
  sLogF(L"file",L"rename to   <%s>\n",dest);

  const char *srcconv = strdupa(FromWideFileName(source));
  const char *destconv = strdupa(FromWideFileName(dest));

  if(!overwrite && sCheckFile(source) && sCheckFile(dest)) // this is not entirely safe (non-atomic!)
    return sFALSE;

  if(rename(srcconv,destconv) == 0)
    return sTRUE;
  else
  {
    sLogF(L"file",L"rename failed to <%s>\n",dest);
    return sFALSE;
  }
}

sBool sDeleteFile(const sChar *name)
{
  return unlink(FromWideFileName(name)) == 0;
}

sBool sGetFileWriteProtect(const sChar *filename,sBool &prot)
{
  int ret = access(FromWideFileName(filename),W_OK);
  
  if(ret == 0)
  {
    prot = sFALSE;
    return sTRUE;
  }
  else if(ret == -1 && errno == EACCES)
  {
    prot = sTRUE;
    return sTRUE;
  }
  else
    return sFALSE;
}

sBool sSetFileWriteProtect(const sChar *filename,sBool prot)
{
  return chmod(FromWideFileName(filename),prot ? 0444 : 0644) == 0;
}

sBool sIsBelowCurrentDir(const sChar *relativePath)
{
  sString<4096> here,there;
  
  sGetCurrentDir(here);
  there = here;
  there.AddPath(relativePath);
  NormalizePath(there);
  
  return sCheckPrefix(there,here);
}

sBool sGetFileInfo(const sChar *name,sDirEntry *de)
{
  struct stat64 st;

  de->Name = sFindFileWithoutPath(name);
  de->Flags = 0;
  de->Size = 0;
  de->LastWriteTime = 0;
  
  const char *cname = FromWideFileName(name); 
  if(stat64(cname,&st) != 0)
    return sFALSE;
  
  de->Flags = sDEF_EXISTS;
  if(S_ISDIR(st.st_mode)) de->Flags |= sDEF_DIR;
  if(access(cname,W_OK) != 0) de->Flags |= sDEF_WRITEPROTECT;
    
  de->Size = st.st_size;
  de->LastWriteTime = st.st_mtime;
    
  return sTRUE;
}

/****************************************************************************/
/***                                                                      ***/
/***   Multithreading                                                     ***/
/***                                                                      ***/
/****************************************************************************/

static sBool sThreadInitialized = sFALSE;
static pthread_key_t sThreadKey, sContextKey;
static sThreadContext sEmergencyThreadContext;

/****************************************************************************/

sThread *sGetCurrentThread()
{
  return (sThread *) pthread_getspecific(sThreadKey);
}

sInt sGetCurrentThreadId()
{
  return pthread_self();
}

/****************************************************************************/


sThreadContext *sGetThreadContext()
{
  sThreadContext *ctx = 0;
  if(sThreadInitialized)
    ctx = (sThreadContext*) pthread_getspecific(sContextKey);
  if(!ctx)
    ctx = &sEmergencyThreadContext;
  
  return ctx;
}

/****************************************************************************/

void sSleep(sInt ms)
{
  usleep(ms*1000); //uSleep need microseconds
}

sInt sGetCPUCount()
{
  return sysconf(_SC_NPROCESSORS_CONF);
}

/****************************************************************************/


void * sSTDCALL sThreadTrunk_pthread(void *ptr)
{
  sThread *th = (sThread *) ptr;
  
  pthread_setspecific(sThreadKey, th);
  pthread_setspecific(sContextKey, th->Context);

  // wait for the ThreadId to be set in the mainthread
  while (!th->ThreadId)
   sSleep(10);

  sU64 self = pthread_self();
  sLogF(L"sys",L"New sThread started. 0x%x, id is 0x%x\n", self, th->ThreadId);

  sVERIFY( pthread_equal( self, th->ThreadId ) );

  th->Code(th,th->Userdata);

#if !sCOMMANDLINE
  // clean up X display handle if we hold one
  if(sXDisplayTls)
  {
    Display *dpy = *sGetTls<Display*>(sXDisplayTls);
    if(dpy)
      XCloseDisplay(dpy);
  }
#endif
  
  return 0;
}

/****************************************************************************/

sThread::sThread(void (*code)(sThread *,void *),sInt pri,sInt stacksize,void *userdata, sInt flags/*=0*/)
{
  sVERIFY(sizeof(pthread_t)==sCONFIG_PTHREAD_T_SIZE);

  
  TerminateFlag = 0;
  Code = code;
  Userdata = userdata;
  Stack = 0;
  Context = sCreateThreadContext(this);

  ThreadHandle = new pthread_t;
  ThreadId     = 0;

  // windows: ThreadHandle = CreateThread(0,stacksize,sThreadTrunk,this,0,(ULONG *)&ThreadId);

  int result = pthread_create((pthread_t*)ThreadHandle, sNULL, sThreadTrunk_pthread, this);
  sVERIFY(result==0);

  ThreadId = *(pthread_t*)ThreadHandle;

// clone(sThreadTrunk, StackMemory + stacksize - 1,  CLONE_FS|CLONE_FILES|CLONE_SIGHAND|CLONE_VM|CLONE_THREAD ,this);

//  if(pri>0)
//  {
//    SetPriorityClass(ThreadHandle,HIGH_PRIORITY_CLASS);
//    SetThreadPriority(ThreadHandle,THREAD_PRIORITY_TIME_CRITICAL);
//  }
//  if(pri<0)
//  {
////    SetPriorityClass(ThreadHandle,BELOW_NORMAL_PRIORITY_CLASS);
//    SetThreadPriority(ThreadHandle,THREAD_PRIORITY_BELOW_NORMAL);
//  }
}

/****************************************************************************/

sThread::~sThread()
{
  if(TerminateFlag==0)
    Terminate();
  if(TerminateFlag==1)
    pthread_join(*(pthread_t*)ThreadHandle, sNULL);

  delete (pthread_t*)ThreadHandle;
  delete Context;
}

/****************************************************************************/

void sInitThread()
{
  pthread_key_create(&sThreadKey, 0);
  pthread_key_create(&sContextKey, 0);

  sClear(sEmergencyThreadContext);
  sEmergencyThreadContext.ThreadName = L"MainThread";
  sEmergencyThreadContext.MemTypeStack[0] = sAMF_HEAP;
  pthread_setspecific(sContextKey, &sEmergencyThreadContext);
  
  sThreadInitialized = sTRUE;
}

/****************************************************************************/

void sExitThread()
{
  pthread_key_delete(sThreadKey);
  pthread_key_delete(sContextKey);
}

/****************************************************************************/

sThreadLock::sThreadLock()
{
  CriticalSection = new pthread_mutex_t;

  pthread_mutexattr_t mattr;
  pthread_mutexattr_init(&mattr);
  pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init((pthread_mutex_t*) CriticalSection, &mattr);  
}

/****************************************************************************/

sThreadLock::~sThreadLock()
{
  delete (pthread_mutex_t*)CriticalSection;
}

/****************************************************************************/

void sThreadLock::Lock()
{
  pthread_mutex_lock((pthread_mutex_t*) CriticalSection);
}

/****************************************************************************/

sBool sThreadLock::TryLock()
{
  return pthread_mutex_trylock((pthread_mutex_t*) CriticalSection) == 0;
}

/****************************************************************************/

void sThreadLock::Unlock()
{
  pthread_mutex_unlock((pthread_mutex_t*) CriticalSection);

}

/****************************************************************************/

// THIS IS UNTESTED, MAY CONTAIN BUGS AND IS DEFINITELY INEFFICIENT.
// It's also outright dangerous on anything that's not x86!

sThreadEvent::sThreadEvent(sBool manual)
{
  Signaled = 0;
  ManualReset = manual;
  
  sLogF(L"system",L"WARNING! This program uses sThreadEvents on Linux. They are currently UNTESTED, probably DANGEROUS and most likely also INEFFICIENT.\n");
}

sThreadEvent::~sThreadEvent()
{
}

sBool sThreadEvent::Wait(sInt timeout)
{
  if(ManualReset) // manual reset
  {
    if(timeout == -1) // okay, just wait forever
    {
      while(!Signaled)
        pthread_yield();
        
      return sTRUE;
    }
    
    sInt start = sGetTime();
    sU32 tDiff;
    sBool okay = sFALSE;
    do
    {
      okay = Signaled;
      tDiff = sU32(sGetTime()-start);
      if(!okay && sU32(timeout) > tDiff) // not signaled, not yet timed out
        pthread_yield();
    }
    while(!okay && sU32(timeout) > tDiff);
    
    return okay;
  }
  else // automatic reset
  {
    if(timeout == -1) // okay, just wait forever
    {
      sU32 gotit;
      while((gotit = sAtomicSwap(&Signaled,0)) == 0)
        pthread_yield();
        
      return sTRUE;
    }
    
    sInt start = sGetTime();
    sU32 tDiff,gotit;
    do
    {
      gotit = sAtomicSwap(&Signaled,0);
      tDiff = sU32(sGetTime()-start);
      if(!gotit && sU32(timeout) > tDiff) // haven't got it, not timed out
        pthread_yield();
    }
    while(!gotit && sU32(timeout) > tDiff);
    
    return gotit == 1;
  }
}

void sThreadEvent::Signal()
{
  Signaled = 1;
}

void sThreadEvent::Reset()
{
  Signaled = 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   X Windows support (disabled in commandline builds)                 ***/
/***                                                                      ***/
/****************************************************************************/

static sThreadLock *InputThreadLock;

class sKeyboardData
{
public:
  sKeyboardData(sInt num)
    : Device(sINPUT2_TYPE_KEYBOARD, num)
  {
    keys.HintSize(256);
    keys.AddManyInit(256, sFALSE);
    sInput2RegisterDevice(&Device);
  }

  ~sKeyboardData()
  {
    sInput2RemoveDevice(&Device);
  }

  void Poll()
  {
    InputThreadLock->Lock();
    sInput2DeviceImpl<sINPUT2_KEYBOARD_MAX>::Value_ v;
    for (sInt i=0; i < 255; i++)
      v.Value[i] = keys[i] ? 1.0f : 0.0f;
    v.Timestamp = sGetTime();
    v.Status = 0;
    Device.addValues(v);
    InputThreadLock->Unlock();
  }

  sStaticArray<sBool> keys;

private:
  sInput2DeviceImpl<sINPUT2_KEYBOARD_MAX> Device;
};

class sMouseData
{
public:
  enum
  {
    sMB_LEFT = 1,                   // left mouse button
    sMB_RIGHT = 2,                  // right mouse button
    sMB_MIDDLE = 4,                 // middle mouse button
    sMB_X1 = 8,                     // 4th mouse button
    sMB_X2 = 16,                    // 5th mouse button
  };

  sMouseData(sInt num) 
   : Device(sINPUT2_TYPE_MOUSE, num) 
  {
    X = Y = Z = RawX = RawY = Buttons = 0;
    sInput2RegisterDevice(&Device);
  }

  ~sMouseData()
  {
    sInput2RemoveDevice(&Device);
  }

  void Poll() 
  {
    InputThreadLock->Lock();
    sInput2DeviceImpl<sINPUT2_MOUSE_MAX>::Value_ v;
    v.Value[sINPUT2_MOUSE_X] = X;
    v.Value[sINPUT2_MOUSE_Y] = Y;
    v.Value[sINPUT2_MOUSE_RAW_X] = RawX;
    v.Value[sINPUT2_MOUSE_RAW_Y] = RawY;
    v.Value[sINPUT2_MOUSE_WHEEL] = Z;
    v.Value[sINPUT2_MOUSE_LMB] = Buttons & sMB_LEFT;
    v.Value[sINPUT2_MOUSE_RMB] = Buttons & sMB_RIGHT;
    v.Value[sINPUT2_MOUSE_MMB] = Buttons & sMB_MIDDLE;
    v.Value[sINPUT2_MOUSE_X1] = Buttons & sMB_X1;
    v.Value[sINPUT2_MOUSE_X2] = Buttons & sMB_X2;
    v.Value[sINPUT2_MOUSE_VALID] = 1.0f;
    v.Timestamp = sGetTime();
    v.Status = 0;
    Device.addValues(v);
    InputThreadLock->Unlock();
  }

  sInt X;
  sInt Y;
  sInt Z;
  sInt RawX;
  sInt RawY;
  sInt Buttons;

private:
  sInput2DeviceImpl<sINPUT2_MOUSE_MAX> Device;
};

static sThread *InputThread;
static sKeyboardData *Keyboard;
static sMouseData *Mouse;

static void sPollInput(sThread *thread, void *data)
{
  while(thread->CheckTerminate())
  {
    Mouse->Poll();
    Keyboard->Poll();
    sSleep(5);
  }
}

static void sInitInput()
{
#if !sCOMMANDLINE
  Keyboard = new sKeyboardData(0);
  Mouse = new sMouseData(0);
  InputThreadLock = new sThreadLock;
  InputThread = new sThread(sPollInput);
#endif
}

static void sExitInput()
{
#if !sCOMMANDLINE
  sDelete(InputThread);
  sDelete(Keyboard);
  sDelete(Mouse);
  sDelete(InputThreadLock);
#endif
}
sADDSUBSYSTEM(Input, 0x30, sInitInput, sExitInput);

Display *sXDisplay()
{
#if !sCOMMANDLINE
  Display **dpy = sGetTls<Display*>(sXDisplayTls);
  if(!dpy) // main thread
    dpy = &sXMainDisplay;
  
  if(!*dpy)
  {
    *dpy = XOpenDisplay(0);
    if(!*dpy)
      sFatal(L"Cannot open display!");
  }
  
  return *dpy;
#else
  return 0;
#endif
}

void sTriggerEvent(sInt event)
{
#if !sCOMMANDLINE
  XClientMessageEvent ev;
  sClear(ev);
  ev.type = ClientMessage;
  ev.message_type = None;
  ev.format = 32;
  ev.data.l[0] = event;
  
  XSendEvent(sXDisplay(),sXWnd,False,NoEventMask,(XEvent *)&ev);
  XFlush(sXDisplay());
#endif
}

void sSetMouse(sInt x,sInt y)
{
#if !sCOMMANDLINE
  XWarpPointer(sXDisplay(),None,sXWnd,0,0,0,0,x,y);
#endif
}

void sSetMouseCenter()
{
#if !sCOMMANDLINE
  Window root;
  int xp,yp;
  unsigned int width,height,border,depth;
  
  XGetGeometry(sXDisplay(),sXWnd,&root,&xp,&yp,&width,&height,&border,&depth);
  sSetMouse(width/2,height/2);
#endif
}

sU32 sGetKeyQualifier()
{
  return sKeyQual;
}

sInt sMakeUnshiftedKey(sInt ascii)
{
  ascii &= sKEYQ_MASK;
  if(ascii>='A' && ascii<='Z') return ascii-'A'+'a';
  
  // this is for german keyboard! please do some linux kung fu here!
  if(ascii=='!') return '1';
  if(ascii=='"') return '2';
  if(ascii==0xa7) return '3';
  if(ascii=='$') return '4';
  if(ascii=='%') return '5';
  if(ascii=='&') return '6';
  if(ascii=='/') return '7';
  if(ascii=='(') return '8';
  if(ascii==')') return '9';
  if(ascii=='=') return '0';
  return ascii;
}

void sInit(sInt flags,sInt xs,sInt ys)
{
#if !sCOMMANDLINE
  const sChar *caption = sGetWindowName();
  
  // X11 initialization
  sXDisplayTls = sAllocTls(sizeof(Display*),sizeof(Display*));
  if(!sXDisplayTls)
    sFatal(L"Failed to allocate X display TLS\n");
  
  Display *dpy = sXDisplay(); // lazily creates the display
  XSynchronize(dpy,True);
  
  sXScreen = DefaultScreen(dpy);
  int depth = DefaultDepth(dpy,sXScreen);

  if(flags & sISF_3D)
    PreInitGFX(flags,xs,ys);
  
  if(!sXVisualInfo)
  {
    sXVisual = DefaultVisual(dpy,sXScreen);
    depth = DefaultDepth(dpy,sXScreen);
  }
  else
  {
    sXVisual = sXVisualInfo->visual;
    depth = sXVisualInfo->depth;
    
    sLogF(L"win",L"sXVisualInfo set, parameters:\n");
    sLogF(L"win",L"  depth=%d\n",depth);
    sLogF(L"win",L"  class=%d\n",sXVisualInfo->c_class);
    sLogF(L"win",L"  r_mask=%06x\n",(sU32)sXVisualInfo->red_mask);
    sLogF(L"win",L"  g_mask=%06x\n",(sU32)sXVisualInfo->green_mask);
    sLogF(L"win",L"  b_mask=%06x\n",(sU32)sXVisualInfo->blue_mask);
    sLogF(L"win",L"  colormap_size=%d\n",sXVisualInfo->colormap_size);
    sLogF(L"win",L"  bits_per_rgb=%d\n",sXVisualInfo->bits_per_rgb);
  }
  
  if(sXVisual->c_class < TrueColor)
    sFatal(L"True color or direct color visual required\n");
  
  Window root = RootWindow(dpy,sXScreen);
  sXColMap = XCreateColormap(dpy,root,sXVisual,AllocNone);
  
  // window and graphics context
  unsigned long black = (sXVisual == DefaultVisual(dpy,sXScreen)) ? BlackPixel(dpy,sXScreen) : 0;
  
  XSetWindowAttributes attr;
  attr.background_pixel = black;
  attr.border_pixel = black;
  attr.colormap = sXColMap;
  sXWnd = XCreateWindow(dpy, root, 10,10, xs,ys, 1,depth, InputOutput,
    sXVisual, CWBackPixel|CWBorderPixel|CWColormap, &attr);
  if(!sXWnd)
    sFatal(L"XCreateWindow failed\n");
  
  XSync(dpy,False); // ...so we get errors now
  
  sXGC = XCreateGC(dpy,sXWnd,0,0);
  
  XSelectInput(dpy,sXWnd,ExposureMask|KeyPressMask|KeyReleaseMask
    |ButtonPressMask|ButtonReleaseMask|StructureNotifyMask|PointerMotionMask);
  XStoreName(dpy,sXWnd,sLinuxFromWide(caption));
  XMapWindow(dpy,sXWnd);
  
  sSystemFlags = flags;
  if(flags & sISF_3D)
    InitGFX(flags,xs,ys);
  else
    ResizeGFX(xs,ys);
  
  XSynchronize(dpy,False);

  // run remaining initializers
  sSetRunlevel(0x100);  
  sUpdateWindow();
#endif
}

extern void sXClipPushUpdate();
extern void sXClearUpdate();
extern void sXGetUpdateBoundingRect(sRect &r);
extern sBool sXUpdateEmpty();

#if !sCOMMANDLINE
static sInt sXLookupKeySym(KeySym sym)
{
  struct Mapping
  {
    KeySym Sym;
    sInt Key;
  };
  static const Mapping keyMap[] = // sorted by X keysym
  {
    { XK_Tab,           sKEY_TAB,       },
    { XK_Return,        sKEY_ENTER,     },
    { XK_Pause,         sKEY_PAUSE,     },
    { XK_Scroll_Lock,   sKEY_SCROLL,    },
    { XK_Escape,        sKEY_ESCAPE,    },

    { XK_Home,          sKEY_HOME,      },
    { XK_Left,          sKEY_LEFT,      },
    { XK_Up,            sKEY_UP,        },
    { XK_Right,         sKEY_RIGHT,     },
    { XK_Down,          sKEY_DOWN,      },
    { XK_Page_Up,       sKEY_PAGEUP,    },
    { XK_Page_Down,     sKEY_PAGEDOWN,  },
    { XK_End,           sKEY_END,       },
    
    { XK_Print,         sKEY_PRINT,     },
    { XK_Insert,        sKEY_INSERT,    },
    { XK_Menu,          sKEY_WINM,      },
    { XK_Num_Lock,      sKEY_NUMLOCK,   },
    
    { XK_F1,            sKEY_F1,        },
    { XK_F2,            sKEY_F2,        },
    { XK_F3,            sKEY_F3,        },
    { XK_F4,            sKEY_F4,        },
    { XK_F5,            sKEY_F5,        },
    { XK_F6,            sKEY_F6,        },
    { XK_F7,            sKEY_F7,        },
    { XK_F8,            sKEY_F8,        },
    { XK_F9,            sKEY_F9,        },
    { XK_F10,           sKEY_F10,       },
    { XK_F11,           sKEY_F11,       },
    { XK_F12,           sKEY_F12,       },
    
    { XK_Shift_L,       sKEY_SHIFT,     },
    { XK_Shift_R,       sKEY_SHIFT,     },
    { XK_Control_L,     sKEY_CTRL,      },
    { XK_Control_R,     sKEY_CTRL,      },
    { XK_Super_L,       sKEY_WINL,      },
    { XK_Super_R,       sKEY_WINR,      },

    { XK_Delete,        sKEY_DELETE,    },
  };
  
  sInt l=0,r=sCOUNTOF(keyMap);
  while(l<r) // binary search
  {
    sInt x = (l+r)>>1;
    if(sym < keyMap[x].Sym)
      r = x;
    else if(sym > keyMap[x].Sym)
      l = x+1;
    else
      return keyMap[x].Key;
  }
  
  return 0;
}
#endif

#if !sCOMMANDLINE
static void sendMouseMove(sInt x, sInt y)
{
  // (x<<16)|y? *Really*? (Sigh.)
  sInput2SendEvent(sInput2Event(sKEY_MOUSEMOVE, 0, ((x & 0xffff) << 16) | (y & 0xffff)));
}
#endif

static void sXMessageLoop()
{
#if !sCOMMANDLINE
  Display *dpy = sXDisplay();
  
  if(sAppPtr && sXWnd)
  {
    sBool done = sFALSE;
    static const sInt buttonKey[10] = { 0,sKEY_LMB,sKEY_MMB,sKEY_RMB,sKEY_WHEELUP,sKEY_WHEELDOWN,0,0,sKEY_X1MB,sKEY_X2MB };
    static const sInt buttonMask[10] = { 0,sMouseData::sMB_LEFT,sMouseData::sMB_MIDDLE,sMouseData::sMB_RIGHT,0,0,0,0,sMouseData::sMB_X1,sMouseData::sMB_X2 };
    
    while(!done)
    {
      XEvent e;
      
      if(!XPending(dpy) && !sXUpdateEmpty())
      {
        sFrameHook->Call();

        sBool app_fullpaint = sFALSE;
        
        if(sGetApp())
        {
          app_fullpaint = sGetApp()->OnPaint();
          if(!app_fullpaint)
            sGetApp()->OnPrepareFrame();
        }
        
        if((sSystemFlags & sISF_2D) && !sExitFlag)
        {
          Window root;
          int xp,yp;
          unsigned int width,height,border,depth;
          
          XGetGeometry(dpy,sXWnd,&root,&xp,&yp,&width,&height,&border,&depth);
          sRect client,update;
          client.Init(0,0,width,height);
          sXGetUpdateBoundingRect(update);
          sXClipPushUpdate();
          if(sAppPtr)
            sAppPtr->OnPaint2D(client,update);
          sClipPop();
          sXClearUpdate();
          sCollector();
        }
        
        if(!app_fullpaint && (sSystemFlags & sISF_3D))
        {
          Render3D();
        }
        
        if(sSystemFlags & sISF_CONTINUOUS)
          sUpdateWindow();
      }
      
      while(XPending(dpy))
      {
        XNextEvent(dpy,&e);
        
        switch(e.type)
        {
        case DestroyNotify:
          sDelete(sAppPtr);
          done = sTRUE;
          break;
        
        case Expose:
          {
            sRect r;
            r.Init(e.xexpose.x,e.xexpose.y,e.xexpose.x+e.xexpose.width,e.xexpose.y+e.xexpose.height);
            sUpdateWindow(r);
          }
          break;

        case MotionNotify:
          sendMouseMove(e.xmotion.x, e.xmotion.y);
          break;
        
        case ButtonPress:
        case ButtonRelease:
          {
            sInt b = e.xbutton.button;
            sU32 orm = (e.type == ButtonRelease) ? 0 : ~0u;
            
            sendMouseMove(e.xbutton.x, e.xbutton.y);
            if(b < sCOUNTOF(buttonMask))
            {
              sU32 mask = buttonMask[b];
              Mouse[0].Buttons = (Mouse[0].Buttons & ~mask) | (mask & orm);
              sInput2SendEvent(sInput2Event(buttonKey[b] | (sKEYQ_BREAK & ~orm)));
            }
          }
          break;
        
        case KeyPress:
        case KeyRelease:
          {
            static XComposeStatus compose[2];
            sInt isRelease = (e.type == KeyRelease);
            sU32 orm = isRelease ? 0 : ~0u;
            char str[8];
            sChar wch[8];
            KeySym sym;
            
            e.xkey.state &= ~ControlMask; // we handle this ourselves

            int nch = XLookupString(&e.xkey,str,sizeof(str),&sym,&compose[isRelease]);
            str[nch] = 0; // todo: latin1 to utf
            sLinuxToWide(wch,str);
            
            switch(sym)
            {
            case XK_Shift_L:    sKeyQual = (sKeyQual & ~sKEYQ_SHIFTL) | (sKEYQ_SHIFTL & orm); break;
            case XK_Shift_R:    sKeyQual = (sKeyQual & ~sKEYQ_SHIFTR) | (sKEYQ_SHIFTR & orm); break;
            case XK_Control_L:  sKeyQual = (sKeyQual & ~sKEYQ_CTRLL)  | (sKEYQ_CTRLL  & orm); break;
            case XK_Control_R:  sKeyQual = (sKeyQual & ~sKEYQ_CTRLR)  | (sKEYQ_CTRLR  & orm); break;
            case XK_Alt_L:      sKeyQual = (sKeyQual & ~sKEYQ_ALT)    | (sKEYQ_ALT    & orm); break;
            case XK_Alt_R:      sKeyQual = (sKeyQual & ~sKEYQ_ALTGR)  | (sKEYQ_ALTGR  & orm); break;
            case XK_Delete:     wch[0]=0; break; // don't generate printable char for delete
            }
            
            if(nch==1 && wch[0]==13) // CR to LF
              wch[0] = sKEY_ENTER;
            
            for(sInt i=0;i<wch[i];i++)
              sInput2SendEvent(sInput2Event(wch[i] | (sKEYQ_BREAK & ~orm)));
            
            if(!wch[0]) // nonprintable
            {
              sInt k = sXLookupKeySym(sym);
              if(k)
                sInput2SendEvent(sInput2Event(k | (sKEYQ_BREAK & ~orm)));
            }
          }
          break;
          
        case ClientMessage:
          if(e.xclient.format == 32)
            sAppPtr->OnEvent(e.xclient.data.l[0]);
          break;

        default:
          break;
        }
        
        if(sExitFlag)
        {
          sAppPtr->OnEvent(sAE_EXIT);
          sCollect();
          sCollector(sTRUE);
          
          if(sSystemFlags & sISF_3D)
            ExitGFX();
          
          XDestroyWindow(dpy,sXWnd);
          done = sTRUE;
        }
      }
    }
    
    sDelete(sAppPtr);
      
    sSetRunlevel(0x80);
    sCollect();
    sCollector(sTRUE);
    
    if(sSystemFlags & sISF_3D)
      ExitGFX();
  }
  else // something went wrong, clean up
  {
    sSetRunlevel(0x80);
    sCollect();
    sCollector(sTRUE);
    
    if(sSystemFlags & sISF_3D)
      ExitGFX();
  }
  
  XCloseDisplay(dpy);
#endif
}

#if 0 // this was written for old input interface and needs updating

/****************************************************************************/
/***                                                                      ***/
/***   Linux Joypads                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class sLinuxJoypad : public sJoypad
{
  sInt TimestampOffset;
  sU32 LastTimestamp;
  sString<256> Path;
  
  int fd;
  sU8 AxisMapping[ABS_MAX+1];
  sBool FirstEvent;
  
  sJoypadData State;
  
  sInt TranslateTimestamp(sU32 inTimestamp);
  void Open(const sChar *filename);
  void Close();
  
  void Event(const js_event &ev);
  
public:
  sLinuxJoypad(const sChar *filename);
  ~sLinuxJoypad();
  
  sBool IsConnected();
  void GetData(sJoypadData &data);
  void GetName(const sStringDesc &name); 
  void SetMotor(sInt slow,sInt fast);

  void Poll();
};

sInt sLinuxJoypad::TranslateTimestamp(sU32 inTimestamp)
{
  if(FirstEvent)
  {
    TimestampOffset = sGetTime() - inTimestamp;
    FirstEvent = sFALSE;
  }
  
  return inTimestamp + TimestampOffset;
}

void sLinuxJoypad::Open(const sChar *filename)
{
  sCopyString(Path,filename);
  
  fd = open(FromWideFileName(filename),O_RDONLY);
  if(fd < 0)
    return;
  
  // get joypad interface version
  sU32 version;
  if(ioctl(fd,JSIOCGVERSION,&version) < 0 || version < 0x010000) // only tested with ver >=2.0
  {
    sLogF(L"inp",L"joypad(%q): need a 1.0 or higher device interface\n",filename);
    Close();
    return;
  }
  
  // get device name
  static const sInt nameLen = 128;
  char devname[nameLen];
  if(ioctl(fd,JSIOCGNAME(nameLen-1),devname) < 0)
  {
    sLogF(L"inp",L"joypad(%q): couldn't get device name!\n",filename);
    Close();
    return;
  }
  
  devname[nameLen-1] = 0;
  sCopyString(Name,devname);
  
  // get number of axes and buttons
  sU8 nAxis,nButton;
  if(ioctl(fd,JSIOCGAXES,&nAxis) < 0
    || ioctl(fd,JSIOCGBUTTONS,&nButton) < 0)
  {
    sLogF(L"inp",L"joypad(%q): couldn't get number of axes and buttons!\n",filename);
    Close();
    return;
  }
  
  // get axis mapping
  if(ioctl(fd,JSIOCGAXMAP,AxisMapping) < 0)
  {
    sLogF(L"inp",L"joypad(%q): couldn't get axis mapping!\n",filename);
    Close();
    return;
  }
  
  // we have everything, fill out State struct
  FirstEvent = sTRUE;
  State.ButtonMask = (1<<nButton)-1; // we just take the native button numbering
  State.AnalogMask = 0;
  State.PovMask = 0;
  
  for(sInt i=0;i<nAxis;i++)
  {
    sInt ind = AxisMapping[i];
    
    if(ind >= ABS_X && ind < ABS_HAT0X) // analog axis with index ind (max 16)
    {
      sVERIFY(ind < 16);
      State.AnalogMask |= 1<<ind;
    }
    else if(ind >= ABS_HAT0X && ind <= ABS_HAT3Y) // POV hats
    {
      sInt povIndex = (ind - ABS_HAT0X) / 2; // one POV hat = 2 axes
      State.PovMask |= 1<<povIndex;
    }
    else
    {
      // if it's neither of these, we don't know what to do with this
      sLogF(L"inp",L"joypad(%q): ignoring axis %d (mapping %d)\n",filename,i,ind);
    }
  }
}

void sLinuxJoypad::Close()
{  
  if(fd >= 0)
  {
    sLogF(L"inp",L"closing joypad <%s> (%q).\n",Name,Path);
    
    close(fd);
    fd = -1;
    Path = L"";
  }
}

void sLinuxJoypad::Event(const js_event &ev)
{
  sInt timestamp = TranslateTimestamp(ev.time);
  
  switch(ev.type & ~JS_EVENT_INIT)
  {
  case JS_EVENT_BUTTON: // button pressed/released
    {
      sInt index = ev.number;
      if(index < sCOUNTOF(State.Pressure))
      {
        sU32 mask = 1u << index;
        
        if(ev.value) // pressed
        {
          State.Pressure[index] = 255;
          State.Buttons |= mask;
          sQueueInput(sIED_JOYPAD,GetId(),sPAD_BUTTON|index,timestamp);
        }
        else // released
        {
          State.Pressure[index] = 0;
          State.Buttons &= ~mask;
          sQueueInput(sIED_JOYPAD,GetId(),sPAD_BUTTON|index|sKEYQ_BREAK,timestamp);
        }
      }
    }
    break;
    
  case JS_EVENT_AXIS: // axis moved
    {
      sInt map = AxisMapping[ev.number];
      
      if(map >= ABS_X && map < ABS_HAT0X) // proper analog axis
        State.Analog[map] = sClamp(ev.value+0x8000,0,0xffff);
      else if(map >= ABS_HAT0X && map <= ABS_HAT3Y) // POV hats
      {
        sInt povIndex = (map - ABS_HAT0X) / 2;
        sInt axisShift = ~map & 1;
        sInt val = axisShift ? -ev.value : ev.value;
        
        sInt oldbits = State.Povs >> (4*povIndex);
        sInt bits = oldbits;

        bits &= ~(5 << axisShift); // bit 0+2
        if(val < 0) bits |= 1 << axisShift;
        if(val > 0) bits |= 4 << axisShift;        
        
        sInt shift = povIndex*4;
        State.Povs = (State.Povs & ~(0xf << shift)) | (bits << shift);
        
        sInt change = bits ^ oldbits;
        for(sInt j=0;j<4;j++)
        {
          sInt mask = 1<<j;
          if(change & mask)
            sQueueInput(sIED_JOYPAD,GetId(),sPAD_POV|(povIndex*4)|j|((bits & mask ) ? 0 : sKEYQ_BREAK),timestamp);
        }
      }
    }
    break;
    
  default:
    // just ignore unknown event types
    break;
  }
}

sLinuxJoypad::sLinuxJoypad(const sChar *filename)
{
  fd = -1;

  sClear(State);
  State.Analog[0] = 0x8000;
  State.Analog[1] = 0x8000;
  State.Analog[2] = 0x8000;
  State.Analog[5] = 0x8000;
  
  Open(filename);
  
  if(IsConnected())
  {
    sLogF(L"inp", L"joypad: buttons %08x axes %04x povs %02x name <%s>\n",State.ButtonMask,State.AnalogMask,State.PovMask,Name);
    Poll(); // read initial state of device
  }
}

sLinuxJoypad::~sLinuxJoypad()
{
  Close();
}

sBool sLinuxJoypad::IsConnected()
{
  return fd >= 0;
}

void sLinuxJoypad::GetData(sJoypadData &data)
{
  data = State;
}

void sLinuxJoypad::GetName(const sStringDesc &name)
{
  sCopyString(name,Name);
}

void sLinuxJoypad::SetMotor(sInt slow,sInt fast)
{
  // don't do anything
}

void sLinuxJoypad::Poll()
{
  pollfd pfd;
  pfd.fd = fd;
  pfd.events = POLLIN;
  pfd.revents = 0;
  
  while(poll(&pfd,1,0) >= 0)
  {
    if(pfd.revents & POLLIN)
    {
      js_event ev;
      if(read(fd,&ev,sizeof(ev)) != sizeof(ev))
      {
        sLogF(L"inp",L"joypad(%q): read unexpected event size from joypad <%s>, closing.\n",Path,Name);
        Close();
        break;
      }
      else
        Event(ev);
    }
    else
      break;
  }
  
  State.Connected = fd >= 0;
}

static void sInitLinuxJoypad()
{
  // "enumerate" joypads
  sArray<sDirEntry> joypads;
  sString<64> base;
  sString<256> filename;
  
  base = L"/dev/input";
  
  if(sLoadDir(joypads,base,L"js*"))
  {
    for(sInt i=0;i<joypads.GetCount();i++)
    {
      filename = base;
      filename.AddPath(joypads[i].Name);
      sLogF(L"inp",L"adding joypad %q\n",filename);
      sAddJoypad(new sLinuxJoypad(filename));
      sResetMemChecksum();
    }
  }
}

static void sExitLinuxJoypad()
{
  // nothing to do
}

sADDSUBSYSTEM(JoypadManager,0xa0,sInitJoypadManager,sExitJoypadManager);
#if !sCONFIG_OPTION_XSI && !sCOMMANDLINE
sADDSUBSYSTEM(LinuxJoypad,0xa1,sInitLinuxJoypad,sExitLinuxJoypad);
#endif

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Platform Dependend Memory Code                                     ***/
/***                                                                      ***/
/****************************************************************************/

sU8 *sMainHeapBase;
sU8 *sDebugHeapBase;
class sMemoryHeap sMainHeap;
class sMemoryHeap sDebugHeap;

class sLibcHeap_ : public sMemoryHandler
{
public:
  void *Alloc(sPtr size,sInt align,sInt flags)
  {
//    sAtomicAdd(&sMemoryUsed, (sDInt)size);
    void *ptr;
    align = sMax<sInt>(align,sizeof(void*));
    if(posix_memalign(&ptr,align,size))
      ptr = 0;
    
    return ptr;
  }
  sBool Free(void *ptr)
  {
//    sAtomicAdd(&sMemoryUsed, -(sDInt)size);
    free(ptr);
    return 1;
  }
} sLibcHeap;

static const sInt DebugHeapSize = 16*1024*1024;

void sInitMem2(sPtr gfx)
{
}

void sInitMem1()
{
  sMainHeapBase = 0;
  sDebugHeapBase = 0;
  
  sInt flags = sMemoryInitFlags;
  
  if(flags & sIMF_DEBUG)
  {
    if (flags & sIMF_NORTL)
    {
      sInt size = DebugHeapSize;
      sDebugHeapBase = (sU8 *)mmap(0,size,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,0,0);
      sVERIFY(sDebugHeapBase != (sU8*) MAP_FAILED);
      sDebugHeap.Init(sDebugHeapBase,size);
      sRegisterMemHandler(sAMF_DEBUG,&sDebugHeap);
    }
    else
      sRegisterMemHandler(sAMF_DEBUG,&sLibcHeap);
  }
  if((flags & sIMF_NORTL) && sMemoryInitSize>0)
  {
    sMainHeapBase = (sU8 *)mmap(0,sMemoryInitSize,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,0,0);
    sVERIFY(sMainHeapBase != (sU8*) MAP_FAILED);
    if(flags & sIMF_CLEAR)
      sSetMem(sMainHeapBase,0x77,sMemoryInitSize);
    sMainHeap.Init(sMainHeapBase,sMemoryInitSize);
    sMainHeap.SetDebug((flags & sIMF_CLEAR)!=0,0);
    sRegisterMemHandler(sAMF_HEAP,&sMainHeap);
  }
  else
  {
    sRegisterMemHandler(sAMF_HEAP,&sLibcHeap);
  }
}

void sExitMem1()
{
  if(sDebugHeapBase)  munmap(sDebugHeapBase,DebugHeapSize);
  if(sMainHeapBase)   munmap(sMainHeapBase,sMemoryInitSize);
  
  sUnregisterMemHandler(sAMF_DEBUG);
  sUnregisterMemHandler(sAMF_HEAP);
}

/****************************************************************************/

int main(int argc, char **argv)
{ 
  setlocale(LC_ALL,"");
  setbuf(stdout,0); // don't buffer stdout
  
#if sCOMMANDLINE
  const char *home = getenv("HOME");
  if(home)
  {
    char fnbuf[1024];
    strncpy(fnbuf,home,1024);
    fnbuf[1023] = 0;
    strncat(fnbuf,"/altonadebug",1024);
    fnbuf[1023] = 0;
    sDebugFile = open(fnbuf,O_WRONLY|O_NDELAY);
  }
#endif
  
  sInitThread();
  sInitMem0();
  sInitGetTime();

  sChar commandLine[2048];
  
  // create a commandline
  commandLine[0] = 0;
  for (sInt i=0; i<argc; i++)
  {
    sChar buffer[2048];
    sLinuxToWide(buffer,argv[i]);
    //sCopyString(buffer,argv[i],sCOUNTOF(buffer));
    sAppendString(commandLine,buffer,sCOUNTOF(commandLine));
    sAppendString(commandLine,L" ",sCOUNTOF(commandLine));
  }
  
  sParseCmdLine(commandLine);

  // ignore brogen pipe signal
  signal(SIGPIPE, SIG_IGN);
  
  sFrameHook = new sHooks;
  sNewDeviceHook = new sHooks;
  sActivateHook = new sHooks1<sBool>;
#if !sSTRIPPED
  sInputHook = new sHooks2<const sInput2Event &,sBool &>;
  sDebugOutHook = new sHooks1<const sChar*>;
#endif
 
  sSetRunlevel(0x80);
  sMain();
  
  sXMessageLoop();
  
  sSetRunlevel(0x00);
  sDelete(sFrameHook);
  sDelete(sNewDeviceHook);
  sDelete(sActivateHook);
#if !sSTRIPPED
  sDelete(sInputHook);
  sDelete(sDebugOutHook);
#endif
  sExitThread();
  sExitMem0();
  if(sDebugFile!=-1) close(sDebugFile);
  return ErrorCode;
} 

/****************************************************************************/

sVideoWriter *sCreateVideoWriter(const sChar *filename,const sChar *codec,sF32 fps,sInt xRes,sInt yRes)
{
  sDPrintF(L"sCreateVideoWriter not implemented on Linux!\n");
  return 0;
}

/****************************************************************************/

void sEnableKeyboard(sBool v) {}

sInt sGetNumInstances()
{
  sFatal(L"sGetNumInstances not implemented");
  return 1;
}

#else

sInt sDummyLink_system_linux;

#endif    // linux

