/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Base/SystemPrivate.hpp"

#include <windows.h>
#include <audioclient.h>
#include <audiopolicy.h>
#include <mmdeviceapi.h>
#include <crtdbg.h>

#include <shobjidl.h>
#include <shlguid.h>
#include <strsafe.h>
#include <shlobj.h>

namespace Altona2 {

using namespace Private;

HWND Private::Window;
HCURSOR Private::MouseCursor;
WNDCLASSEXW Private::WindowClass;
bool Private::WindowActive;
sHook1<const Private::sWinMessage &> Private::sWinMessageHook;


#if sConfigLogging
void Private::DXError(uint err,const char *file,int line)
{
  if(FAILED(err))
  {
    sString<1024> buffer;

    sDPrintF("%s(%d): error %08x (%d)\n",file,line,err,err&0x3fff);
    sFatal("dx");
  }
}
#else
void Private::DXError(uint err)
{
  if(FAILED(err))
  {
    sString<256> buffer;

    sDPrintF("dx error %08x (%d)\n",err,err&0x3fff);
    sFatal("dx");
  }
}
#endif

namespace Private
{
  bool sExitFlag;
  uint64 HRFrequency;
  uint64 PTFrequency;
  uint64 LastPTTick;
  uint64 LastHRTick;
  uint64 StartPTTick;
  int StartTimeTick;

  bool IsFullyInitialized;
}

struct HotKey
{
    uint Key;
    sDelegate<void> Delegate;
    int Id;
    int VKey;
    int VMod;
};

namespace Private
{
    sThread *AudioThread;
    sThreadLock *AudioLock;
//    sThreadEvent *AudioEvent;
    sDelegate2<void,float *,int> AOHandler;

    bool AOEnable = 0;
    IAudioClient *AOClient = 0;
    IAudioRenderClient *AORender = 0;
    IMMDevice *AODev = 0;
    uint AOBufferSize = 0;
    int AOTimeout = 0;
    int64 AOTotalSamples = 0;

    sArray<HotKey *> HotKeys;
    int HotKeyId;
};

#pragma comment (lib,"winmm.lib")


static uint sKeyTable[3][256];    // norm/shift/alt
void sInitKeys();

/****************************************************************************/
/***                                                                      ***/
/***   Helpers                                                            ***/
/***                                                                      ***/
/****************************************************************************/

uptr sUTF8ToWide(uint16 *d,const char *s,int size,bool nocr=0)
{
  uint16 *start = d;
  uint16 *end = d+size;

  while(*s && d<end)
  {
    if((*s & 0x80)==0x00)
    {
      if(!nocr || *s != '\r')   // no fucking dos CR!
        *d++ = s[0];
      s+=1;
    }
    else if((*s & 0xc0)==0x80)
    {
      *d++ = '?';       // continuation byte without prefix code
      s+=1;
    }
    else if((*s & 0xe0)==0xc0)
    {
      if(s[1]!=0 && (s[1]&0xc0)==0x80)
      {
        *d++ = ((s[0]&0x1f)<<6)|(s[1]&0x3f);
      }
      else
      {
        *d++= '?';      // eof or invalid continuation
      }
      s+=2;
    }
    else if((*s & 0xf0)==0xe0)
    {
      if(s[1]!=0 && s[2]!=0 && (s[1]&0xc0)==0x80 && (s[2]&0xc0)==0x80)
      {
        *d++ = ((s[0]&0x0f)<<12)|((s[1]&0x3f)<<6)|(s[2]&0x3f);
      }
      else
      {
        *d++= '?';      // eof or invalid continuation
      }
      s+=3;
    }
    else if((*s & 0xf8)==0xf0 && s[1]!=0 && s[2]!=0 && s[3]!=0)
    {
      *d++ = '?';       // valid utf8, but can't decode them into 16 bit
      s+=4;
    }
    else
    {
      *d++ = '?';       // invalid prefix code
      s+=1;
    }
  }
  *d = 0;
  return d-start;
}

static void copywstring(WCHAR *d,const wchar_t *s)
{
  while(*s) *d++ = *s++;
  *d++ = 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   Com                                                                ***/
/***                                                                      ***/
/****************************************************************************/

class sComSubsystem : public sSubsystem
{
public:
    sComSubsystem() : sSubsystem("Windows Com",0x01) {}

    void Init()
    {
        CoInitializeEx(0,COINIT_MULTITHREADED);
    }
    void Exit()
    {
        CoUninitialize(); 
    }

} sComSubsystem_;

/****************************************************************************/
/***                                                                      ***/
/***   Root filehandler                                                   ***/
/***                                                                      ***/
/****************************************************************************/

class sRootFile : public sFile
{
  HANDLE File;
  sFileAccess Access;
  bool IsWrite;
  bool IsRead;

  bool Ok;
  int64 Size;
  int64 Offset;
public:
  sRootFile(HANDLE file,sFileAccess access)
  {
    File = file;
    Access = access;
    Ok = 1;
    IsWrite = Access==sFA_Write || Access==sFA_WriteAppend || Access==sFA_ReadWrite;
    IsRead = Access==sFA_Read || Access==sFA_ReadRandom || Access==sFA_ReadWrite;
    Offset = 0;

    LARGE_INTEGER s;
    if(!GetFileSizeEx(File,&s)) Ok = 0;
    Size = s.QuadPart;
    if(Access==sFA_WriteAppend)
      Offset = s.QuadPart;    // not tested...
  }

  ~sRootFile()
  {
    Close();
  }

  bool Close()
  {
    if(!CloseHandle(File))
      Ok = 0;
    File = INVALID_HANDLE_VALUE;
    return Ok;
  }

  bool SetSize(int64)
  {
    if(IsWrite)
    {
    }
    else
    {
      Ok = 0;
    }
    return Ok;
  }

  int64 GetSize()
  {
    return Size;
  }

  // use only one of these interfaces!

  // synchronious interface

  bool Read(void *data,uptr size)
  {
    if(IsRead)
    {
      sASSERT(size<0x7fffffff);
      DWORD read=0;
      if(!ReadFile(File,data,(uint)size,&read,0)) Ok = 0;
      if(read!=size) Ok = 0;
      Offset+=read;
    }
    else 
    {
      Ok = 0;
    }
    return Ok;
  }

  bool Write(const void *data,uptr size)
  {
    if(IsWrite)
    {
      sASSERT(size<0x7fffffff);
      DWORD write=0;
      if(!WriteFile(File,data,(uint)size,&write,0)) Ok = 0;
      if(write!=size) Ok = 0;
      Offset+=write;
    }
    else 
    {
      Ok = 0;
    }
    return Ok;  
  }

  bool SetOffset(int64 offset)
  {
    Offset = offset;

    LARGE_INTEGER o;
    o.QuadPart = offset;
    if(!SetFilePointerEx(File,o,0,FILE_BEGIN)) Ok = 0;

    return Ok;
  }

  int64 GetOffset()
  {
    return Offset;
  }

  // file mapping. may be unimplemented, returning 0

  uint8 *Map(int64 offset,uptr size)
  {
    return 0;
  }

  // asynchronous interface. may be unimplemented, returning 0

  bool ReadAsync(int64 offset,uptr size,void *mem,sFileASyncHandle *hnd,sFilePriorityFlags pri)
  {
    *hnd = 0;
    return 0;
  }

  bool Write(int64 offset,uptr size,void *mem,sFileASyncHandle *hnd,sFilePriorityFlags pri)
  {
    return 0;
  }

  bool CheckAsync(sFileASyncHandle)
  {
    return 0;
  }

  bool EndAsync(sFileASyncHandle)
  {
    return 0;
  }
};

/****************************************************************************/

class sRootFileHandler : public sFileHandler
{
public:
  sRootFileHandler()
  {
  }

  ~sRootFileHandler()
  {
  }

  sFile *Create(const char *name,sFileAccess access)
  {
    HANDLE handle = INVALID_HANDLE_VALUE;
    static const char *mode[] =  {  0,"read","readrandom","write","writeappend","readwrite"  };
    uint16 wname[sMaxPath];
    sUTF8ToWide(wname,name,sMaxPath);
    for(int i=0;wname[i];i++)
      if(wname[i]=='/')
        wname[i]='\\';

    switch(access)
    {
    case sFA_Read:
      handle = CreateFileW((LPCWSTR)wname,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN/*|FILE_FLAG_OVERLAPPED*/,0);
      break;
    case sFA_ReadRandom:
      handle = CreateFileW((LPCWSTR)wname,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,/*FILE_FLAG_OVERLAPPED*/0,0);
      break;
    case sFA_Write:
      handle = CreateFileW((LPCWSTR)wname,GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_FLAG_SEQUENTIAL_SCAN,0);
      break;
    case sFA_WriteAppend:
      handle = CreateFileW((LPCWSTR)wname,GENERIC_WRITE,0,0,OPEN_ALWAYS,FILE_FLAG_SEQUENTIAL_SCAN,0);
      break;
    case sFA_ReadWrite:
      handle = CreateFileW((LPCWSTR)wname,GENERIC_WRITE|GENERIC_READ,0,0,OPEN_ALWAYS,0,0);
      break;
    default:
      sASSERT0();
      break;
    }

    if(handle!=INVALID_HANDLE_VALUE)
    {
      sLogF("file","open file <%s> for %s",name,mode[access]);
      sFile *file = new sRootFile(handle,access);
      return file;
    }
    else 
    {
      sLogF("file","failed to open file <%s> for %s",name,mode[access]);
      return 0;
    }
  }

  bool Exists(const char *name)
  {
    WIN32_FIND_DATA wfd;
    HANDLE hndl;
    uint16 wname[sMaxPath];

    sUTF8ToWide(wname,name,sMaxPath);
    for(int i=0;wname[i];i++)
      if(wname[i]=='/')
        wname[i]='\\';

    hndl=FindFirstFileW((LPCWSTR)wname,&wfd);
    if (hndl==INVALID_HANDLE_VALUE)
      return 0;
    FindClose(hndl);
    return 1;
  }
};

/****************************************************************************/
/****************************************************************************/

class sRootFileHandlerSubsystem : public sSubsystem
{
public:
  sRootFileHandlerSubsystem() : sSubsystem("Root Filehandler",0x11) {}

  void Init()
  {
    sAddFileHandler(new sRootFileHandler);
  }
} sRootFileHandlerSubsystem_;

/****************************************************************************/
/***                                                                      ***/
/***   Basic Alloc / Free                                                 ***/
/***                                                                      ***/
/****************************************************************************/

void *sAllocMemSystem(uptr size,int align,int flags)
{
  void *ptr = _aligned_malloc(size,align);
  if(ptr==0)
    sFatal("out of virtual memory");
  if(ptr)
    sSetMem(ptr,0x11,size);
  return ptr;
}

void sFreeMemSystem(void *ptr)
{
  _aligned_free(ptr);
}

uptr sMemSizeSystem(void *ptr)
{
  return (uptr) _aligned_msize(ptr,0,0);
}

void sCheckMemory()
{
    sASSERT(_CrtCheckMemory());
}

void sDebugMemory(bool enable)
{
    if(enable)
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_CHECK_ALWAYS_DF);
    else
        _CrtSetDbgFlag(0);
}

/****************************************************************************/
/***                                                                      ***/
/***   Shell                                                              ***/
/***                                                                      ***/
/****************************************************************************/

bool sExecuteOpen(const char *file)
{
  sLogF("sys","shell open <%s>",file);

  uint16 buffer[sMaxPath];
  if(!sUTF8toUTF16(file,buffer,sCOUNTOF(buffer),0))
    sFatal("sExecuteOpen: string too long");
   
  return uptr(ShellExecuteW(0,L"open",(LPCWSTR)buffer,0,L"",SW_SHOW))>=32;
}

bool sExecuteShell(const char *cmdline,const char *dir)
{
  uint16 buffer[sMaxPath];
  uint16 bufferdir[sMaxPath];

  sLogF("sys","shell <%s>",cmdline);

  if(!sUTF8toUTF16(cmdline,buffer,sCOUNTOF(buffer),0))
    sFatal("sExecuteShell: string too long");

  if(dir)
    if(!sUTF8toUTF16(dir,bufferdir,sCOUNTOF(bufferdir),0))
      sFatal("sExecuteShell: string too long");

  bool result = 0;
  DWORD code = 1;
  PROCESS_INFORMATION pi;
  STARTUPINFOW si;
  
  sClear(pi);
  sClear(si);
  si.cb = sizeof(si);
  if(!CreateProcessW(0,(LPWSTR)buffer,0,0,0,0,0,dir?(LPWSTR)bufferdir:(LPWSTR)0,&si,&pi))
    return 0;
  
  WaitForSingleObject(pi.hProcess,INFINITE);
  if(GetExitCodeProcess(pi.hProcess,&code))
  {
    if(code==0)
      result = 1;
  }
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return result;
}


bool sExecuteShellDetached(const char *cmdline,const char *dir)
{
  sLogF("sys","shell detached <%s>",cmdline);

  uint16 buffer[sMaxPath];
  uint16 bufferdir[sMaxPath];

  if(!sUTF8toUTF16(cmdline,buffer,sCOUNTOF(buffer),0))
    sFatal("sExecuteShellDetached: string too long");
  
  if(dir)
    if(!sUTF8toUTF16(dir,bufferdir,sCOUNTOF(bufferdir),0))
      sFatal("sExecuteShell: string too long");

  PROCESS_INFORMATION pi;
  STARTUPINFOW si;
  
  sClear(pi);
  sClear(si);
  si.cb = sizeof(si);
  if(!CreateProcessW(0,(LPWSTR)buffer,0,0,0,CREATE_NEW_PROCESS_GROUP,0,dir?(LPWSTR)bufferdir:(LPWSTR)0,&si,&pi))
    return 0;
  
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return 1;
}


// warning!
// this is exactly as the sample implementation from microsoft
// "Creating a Child Process with Redirected Input and Output"
// the slightest change will break the code in mysterious ways!

bool sExecuteShell(const char *cmdline,sTextLog &tb)
{
  sLogF("sys","shell logging <%s>",cmdline);

  uint16 buffer[sMaxPath];
  if(!sUTF8toUTF16(cmdline,buffer,sCOUNTOF(buffer),0))
    sFatal("sExecuteShell with sTextLog: string too long");

  bool result = 0;
  DWORD code = 1;
  PROCESS_INFORMATION pi;
  STARTUPINFOW si;
  HANDLE readpipe;
  HANDLE writepipe;
  DWORD bytesread;
  SECURITY_ATTRIBUTES sec; 

  // prepare the pipe for stdout
  // read: not inhereted
  // write: inhrereted

  sClear(sec);
  sec.nLength = sizeof(SECURITY_ATTRIBUTES); 
  sec.bInheritHandle = TRUE; 
   
  HANDLE readpipe_1;
  if(!CreatePipe(&readpipe_1,&writepipe,&sec,0))
    sASSERT0();
  if(!DuplicateHandle(GetCurrentProcess(),readpipe_1,GetCurrentProcess(),&readpipe,0,0,DUPLICATE_SAME_ACCESS))
    sASSERT0();
  CloseHandle(readpipe_1);

  // create process with redirected pipe

  sClear(pi);
  sClear(si);
  si.cb = sizeof(si);

  si.hStdOutput = writepipe;
  si.hStdError = GetStdHandle(STD_ERROR_HANDLE); 
  si.hStdInput = GetStdHandle(STD_INPUT_HANDLE); 
  si.dwFlags = STARTF_USESTDHANDLES;

  uint16 env[2048];
  GetEnvironmentVariableW(L"PATH",(LPWSTR)&env[0],sCOUNTOF(env));

  // [kb 09-09-07] added CREATE_NO_WINDOW flag to suppress the empty console window
  if(CreateProcessW(0,(LPWSTR)buffer,0,0,1,CREATE_NO_WINDOW,0,0,&si,&pi))
  {
    // close write port. the child process has the last handle, and
    // that will be closed soon. 
    // this allows read to terminate when no more input can come.

    if(!CloseHandle(writepipe))
      sASSERT0();

    // read and convert to unicode in chunks

    const int block = 4096;
    uint8 ascii[block];
    char uni[block];

    for(;;)
    {
      if(!ReadFile(readpipe,ascii,block,&bytesread,0) || bytesread==0)
        break;
      for(uint i=0;i<bytesread;i++)
        uni[i] = ascii[i];
      tb.Add(uni,bytesread);
    }

    CloseHandle(readpipe);

    // terminate process and get termination code

    WaitForSingleObject(pi.hProcess,INFINITE);
    if(GetExitCodeProcess(pi.hProcess,&code))
    {
      if(code==0)
        result = 1;
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
  }
  else  // failed to create process
  {
    CloseHandle(writepipe);
    CloseHandle(readpipe);
    result = 1;
  }

  return result;
}

/****************************************************************************/
/***                                                                      ***/
/***   System Specials                                                    ***/
/***                                                                      ***/
/****************************************************************************/

void sSystemMessageBox(const char *title,const char *text)
{
  MessageBoxA(0,text,title,MB_OK);
}

bool sGetSystemPath(sSystemPath sp,const sStringDesc &desc)
{
    WCHAR buffer[MAX_PATH];
    sString<sMaxPath> b0,b1;
    switch(sp)
    {
    case sSP_User:
        if(SUCCEEDED(SHGetFolderPathW(0,CSIDL_PROFILE,0,0,buffer)))
        {
            sUTF16toUTF8((uint16 *)buffer,b0);
            b1.PrintF("%p",b0);
            sCopyString(desc,b1);
            return true;
        }
        break;
    }
    return false;
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

bool sCopyFile(const char *source,const char *dest,bool failifexists)
{
  sLogF("file","copy from <%s> to <%s>",source,dest);

  uint16 sb[sMaxPath];
  uint16 db[sMaxPath];
  if(!sUTF8toUTF16(source,sb,sCOUNTOF(sb),0)) sASSERT0();
  if(!sUTF8toUTF16(dest  ,db,sCOUNTOF(db),0)) sASSERT0();

  if (CopyFileW((LPCWSTR)sb,(LPCWSTR)db,failifexists))
  {
    SetFileAttributes((LPCWSTR)db,GetFileAttributes((LPCWSTR)db)&~FILE_ATTRIBUTE_READONLY);
    return 1;
  }
  else
  {
    sLogF("file","copy failed to <%s>",dest);
  }
  return 0;
}

bool sRenameFile(const char *source,const char *dest, bool overwrite)
{
  sLogF("file","rename from <%s> to <%s>",source,dest);

  uint16 sb[sMaxPath];
  uint16 db[sMaxPath];
  if(!sUTF8toUTF16(source,sb,sCOUNTOF(sb),0)) sASSERT0();
  if(!sUTF8toUTF16(dest  ,db,sCOUNTOF(db),0)) sASSERT0();

  if(MoveFileExW((LPCWSTR)sb,(LPCWSTR)db,overwrite?MOVEFILE_REPLACE_EXISTING:0))
  {
    return 1;
  }
  else
  {
    sLogF("file","rename failed to <%s>",dest);
    return 0;;
  }
}

bool sDeleteFile(const char *name)
{
  uint16 n[sMaxPath];
  if(!sUTF8toUTF16(name,n,sCOUNTOF(n),0)) sASSERT0();
  if(DeleteFileW((LPCWSTR)n))
  {
    sLogF("file","delete file <%s>",name);
    return 1;
  }
  else
  {
    sLogF("file","failed to delete file <%s>",name);
    return 1;
  }
}


bool sGetFileWriteProtect(const char *name,bool &prot)
{
  uint16 n[sMaxPath];
  if(!sUTF8toUTF16(name,n,sCOUNTOF(n),0)) sASSERT0();

  prot = 0;
  DWORD attr = GetFileAttributes((LPCWSTR)n);
  if(attr==INVALID_FILE_ATTRIBUTES)
    return 0;
  if(attr & FILE_ATTRIBUTE_READONLY)
    prot = 1;
  sLogF("file","get write protect <%s> %d",name,prot);
  return 1;
}

bool sSetFileWriteProtect(const char *name,bool prot)
{
  uint16 n[sMaxPath];
  if(!sUTF8toUTF16(name,n,sCOUNTOF(n),0)) sASSERT0();

  DWORD attr = GetFileAttributes((LPCWSTR)n);
  if(attr==INVALID_FILE_ATTRIBUTES)
    return 0;
  if(prot)
    attr |= FILE_ATTRIBUTE_READONLY;
  else
    attr &= ~FILE_ATTRIBUTE_READONLY;
  sLogF("file","set write protect <%s> %d",name,prot);
  return SetFileAttributes((LPCWSTR)n,attr)!=0;
}


bool sGetFileInfo(const char *name,sDirEntry *de)
{
  HANDLE hnd;
  FILETIME time;
  DWORD sizehigh;
  DWORD attr;
  bool ok = 0;

  uint16 n[sMaxPath];
  if(!sUTF8toUTF16(name,n,sCOUNTOF(n),0)) sASSERT0();

  // clear infgo

  const char *s,*s0;
  s0 = s = name;
  while(*s)
  {
    if(*s=='\\' || *s=='/')
      s0 = s+1;
    s++;
  }

  de->Name = s0;
  de->Flags = 0;
  de->Size = 0;
  de->LastWriteTime = 0;

  // open file

  hnd = CreateFileW((LPCWSTR)n,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
  if(hnd!=INVALID_HANDLE_VALUE)
  {
    de->Flags = sDEF_Exists;

    // size

    de->Size = GetFileSize(hnd,&sizehigh);
    if(sizehigh!=0 || de->Size<0)
      de->Size = 0x7fffffff;

    // time

    GetFileTime(hnd,0,0,&time);
    de->LastWriteTime = time.dwLowDateTime + (((uint64) time.dwHighDateTime)<<32);

    // attributes

    attr = GetFileAttributesW((LPCWSTR)n);
    if(attr!=INVALID_FILE_ATTRIBUTES)
    {
      if(attr & FILE_ATTRIBUTE_DIRECTORY)
        de->Flags |= sDEF_Dir;
      if(attr & FILE_ATTRIBUTE_READONLY)
        de->Flags |= sDEF_WriteProtect;

      ok = 1;
    }
  }

  // done

  CloseHandle(hnd);
  
  return ok;
}

/****************************************************************************/

bool sChangeDir(const char *name)
{
  sLogF("file","sChangeDir <%s>",name);
  uint16 n[sMaxPath];
  if(!sUTF8toUTF16(name,n,sCOUNTOF(n),0)) sASSERT0();

  bool ok = SetCurrentDirectoryW((LPCWSTR)n)!=0;
  if(!ok)  sLogF("file","sChangeDir failed");
  return ok;
}

void sGetCurrentDir(const sStringDesc &str)
{
  uint16 n[sMaxPath];

  GetCurrentDirectoryW(sMaxPath,(LPWSTR)n);

  const uint16 *s = n;
  sUTF16toUTF8(s,str);
}

/****************************************************************************/


bool sMakeDir(const char *name)
{
  uint16 n[sMaxPath];
  if(!sUTF8toUTF16(name,n,sCOUNTOF(n),0)) sASSERT0();
  sLogF("sys","makedir %q",name);
  if(!CreateDirectoryW((LPCWSTR)n,0))
  {
    sLogF("sys","makedir failed");
    return 0;
  }
  return 1;
}

bool sDeleteDir(const char *name)
{
  uint16 n[sMaxPath];
  if(!sUTF8toUTF16(name,n,sCOUNTOF(n),0)) sASSERT0();
  return RemoveDirectoryW((LPCWSTR)n)?1:0;
}

bool sCheckDir(const char *name)
{
  uint16 n[sMaxPath];
  if(!sUTF8toUTF16(name,n,sCOUNTOF(n),0)) sASSERT0();
  HANDLE file = CreateFileW((LPCWSTR)n,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,0);
  if(file!=INVALID_HANDLE_VALUE)
    CloseHandle(file);
  return file!=INVALID_HANDLE_VALUE;
}

/****************************************************************************/

bool sLoadDir(sArray<sDirEntry> &list,const char *path)
{
  sString<sMaxPath> buffer;
  HANDLE handle;
  WIN32_FIND_DATAW dir;
  sDirEntry *de;

  list.Clear();

  sLogF("file","load dir <%s>",path);
  buffer.PrintF("%s\\*.*",path);

  uint16 n[sMaxPath];
  if(!sUTF8toUTF16(buffer,n,sCOUNTOF(n),0)) sASSERT0();

  handle = FindFirstFileW((LPCWSTR)n,&dir);
  if(handle==INVALID_HANDLE_VALUE)
    return 0;

  do
  {
    if(dir.cFileName[0]=='.' && dir.cFileName[1]==0)
      continue;
    if(dir.cFileName[0]=='.' && dir.cFileName[1]=='.' && dir.cFileName[2]==0)
      continue;

    de = list.AddMany(1);
    de->Flags = sDEF_Exists;
    if(dir.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      de->Flags |= sDEF_Dir;
    if(dir.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
      de->Flags |= sDEF_WriteProtect;

    de->Size = dir.nFileSizeLow;
    if(de->Size<0 || dir.nFileSizeHigh)
      de->Size = 0x7fffffff;
    const uint16 *n16 = (const uint16 *) dir.cFileName;
    sUTF16toUTF8(n16,de->Name);
    de->LastWriteTime = dir.ftLastWriteTime.dwLowDateTime + (((uint64) dir.ftLastWriteTime.dwHighDateTime)<<32);
  }
  while(FindNextFileW(handle,&dir));

  FindClose(handle);

  return 1;
}

bool sLoadVolumes(sArray<sDirEntry> &list)
{
    sString<sMaxPath> buffer;
    HANDLE handle;

    list.Clear();

    uint16 volume[sMaxPath];
    uint16 letters[sMaxPath];

    handle = FindFirstVolumeW((LPWSTR)volume,sMaxPath);
    if(handle==INVALID_HANDLE_VALUE)
        return 0;

    do
    {
        DWORD used=0;
        if(GetVolumePathNamesForVolumeNameW((LPWSTR)volume,(LPWSTR)letters,sMaxPath,&used))
        {
            uint16 *w = letters;
            while(*w)
            {
                sDirEntry de;
                de.Flags = sDEF_Exists;
                de.Flags |= sDEF_Dir;
                de.Size = 0;
                de.LastWriteTime = 0;
                sUTF16toUTF8(letters,de.Name);
                for(int i=0;de.Name[i];i++)
                    if(de.Name[i]=='\\')
                        de.Name[i]='/';
                uptr len = sGetStringLen(de.Name);
                while(len>0 && de.Name[len-1]=='/')
                {
                    len--;
                    de.Name[len] = 0;
                }
                while(*w)
                    w++;
                w++;

                if(de.Name!="A:" && de.Name!="B:")      // please don't add floppies :-)
                    list.AddTail(de);
            }
        }
    }
    while(FindNextVolumeW(handle,(LPWSTR)volume,sMaxPath));

    FindVolumeClose(handle);

    list.Sort([](const sDirEntry &a,const sDirEntry &b){ return a.Name<b.Name; });
    return 1;
}

/****************************************************************************/

bool sIsAbsolutePath(const char *file)
{
  if(file[0]=='\\' && file[1]=='\\')
    return 1;
  for(int i=0;file[i];i++)
    if(file[i]==':')
      return 1;
  return 0;
}

void sMakeAbsolutePath(const sStringDesc &buffer,const char *file)
{
  if(sIsAbsolutePath(file))
  {
    sCopyString(buffer,file);
  }
  else
  {
    sString<sMaxPath> b;
    sGetCurrentDir(b);
    b.Add("/");
    b.Add(file);
    sCopyString(buffer,b);
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   dynamic link libraries                                             ***/
/***                                                                      ***/
/****************************************************************************/

sDynamicLibrary::sDynamicLibrary()
{
  Handle = 0;
}

sDynamicLibrary::~sDynamicLibrary()
{
  HMODULE mod = (HMODULE) Handle;
  FreeLibrary(mod);
}

bool sDynamicLibrary::Load(const char *filename)
{
  sASSERT(Handle==0);
  HMODULE mod = LoadLibraryA(filename);
  Handle = (uptr) mod;
  return Handle!=0;
}

void *sDynamicLibrary::GetSymbol(const char *symbol)
{
  HMODULE mod = (HMODULE) Handle;
  return GetProcAddress(mod,symbol);
}

/****************************************************************************/
/***                                                                      ***/
/***   File Manipulations                                                 ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/

/****************************************************************************/
/***                                                                      ***/
/***   Initialization and message loop and stuff                          ***/
/***                                                                      ***/
/****************************************************************************/

LRESULT WINAPI MsgProc(HWND win,UINT msg,WPARAM wparam,LPARAM lparam);

void sRunApp(sApp *app,const sScreenMode &sm_)
{
  CurrentApp = app;
  CurrentMode = sm_;

  // Register the window class.

  WNDCLASSEXW WindowClass;
  sClear(WindowClass);
  WindowClass.cbSize = sizeof(WNDCLASSEX);
  WindowClass.style = CS_CLASSDC|CS_DBLCLKS;
  WindowClass.lpfnWndProc = MsgProc;
  WindowClass.hInstance = GetModuleHandle(NULL);
  WindowClass.hIcon = LoadIcon(WindowClass.hInstance,MAKEINTRESOURCE(100));
  WindowClass.lpszClassName = L"Altona2";

  RegisterClassExW(&WindowClass);

  // raw input

    RAWINPUTDEVICE rid[1];
    rid[0].usUsagePage = 1;
    rid[0].usUsage = 2;
    rid[0].dwFlags = 0;
    rid[0].hwndTarget = 0;

    RegisterRawInputDevices(rid,1,sizeof(RAWINPUTDEVICE));

  // Create the application's window.

  sExitFlag = 0;
  Private::IsFullyInitialized = 0;

  MouseCursor = LoadCursor(0,MAKEINTRESOURCE(IDC_ARROW));

  // application loop

  Window = 0;
  sCreateWindow(sm_);
  sSubsystem::SetRunlevel(0xff);

  if(!(sm_.Flags & sSM_NoInitialScreen))
    new sScreen(sm_);

  if(Window && !(sm_.Flags & sSM_Headless))
    ShowWindow(Window,SW_SHOWDEFAULT);

  sInitKeys();
  CurrentApp->OnInit();
  Private::IsFullyInitialized = 1;

  sLog("sys","start main loop");
  sDPrint("/****************************************************************************/\n");
  while(1)
  {
    if(sExitFlag)
    {
      if(Window)
        DestroyWindow(Window);
#if sConfigRender==sConfigRenderDX11 || sConfigRender==sConfigRenderDX9
      DestroyAllWindows();
#endif
    }
    MSG msg;
    while(PeekMessageW(&msg,0,0,0,PM_REMOVE))
    {
        if(msg.message==WM_HOTKEY)
        {
          for(auto hk : HotKeys)
          {
              if(msg.wParam==hk->Id)
              {
                  hk->Delegate.Call();
                  break;
              }
          }
        }
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
      if(msg.message==WM_QUIT)
        goto out;
    }

    if((WindowActive || (CurrentMode.Flags & sSM_BackgroundRender)) && !sExitFlag)
    {
      Render();
    }
    else
    {
      if(GetMessageW(&msg,0,0,0))
      {
        if(msg.message==WM_HOTKEY)
        {
          for(auto hk : HotKeys)
          {
              if(msg.wParam==hk->Id)
              {
                  hk->Delegate.Call();
                  break;
              }
          }
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        if(msg.message==WM_QUIT)
          goto out;
      }
    }
  }
  out:;
  sDPrint("/****************************************************************************/\n");
  sLog("sys","stop main loop");
  CurrentApp->OnExit();
  sGC->CollectNow();

#if sConfigRender!=sConfigRenderGL2
  sSubsystem::SetRunlevel(0x7f);
#endif
  sWinMessageHook.FreeMemory();
  UnregisterClass(L"Altona2",WindowClass.hInstance);

  delete CurrentApp;
  CurrentApp = 0;
}

void sUpdateScreen(const sRect &r)
{
}

uint sGetKeyQualifier()
{
  return KeyQual & 0xffff0000;
}

void sGetMouse(int &x,int &y,int &b)
{
  x = DragData.PosX;
  y = DragData.PosY;
  b = NewMouseButtons;
}

void sExit()
{
  sLog("sys","sExit()");
  sExitFlag = 1;
}

/****************************************************************************/

class sHotkeySubsystem : public sSubsystem
{
public:
    sHotkeySubsystem() : sSubsystem("HotKeys",0x01) {}


    void Init()
    {
        Private::HotKeyId = 1;
    }
    void Exit()
    {
        Private::HotKeys.DeleteAll();
        Private::HotKeys.FreeMemory();
    }
} sHotkeySubsystemSubsystem_;

bool sRegisterHotkey(uint key,sDelegate<void> &del)
{
    int vk = VkKeyScanW(key&sKEYQ_Mask)&0xff;
    int mod = MOD_NOREPEAT;
    if(key & sKEYQ_Shift)
        mod |= MOD_SHIFT;
    if(key & sKEYQ_Ctrl)
        mod |= MOD_CONTROL;
    if(key & sKEYQ_Alt)
        mod |= MOD_ALT;
    if(key & sKEYQ_Meta)
        mod |= MOD_WIN;

    auto hk = new HotKey;
    hk->Key = key;
    hk->Delegate = del;
    hk->Id = HotKeyId++;
    hk->VKey = vk;
    hk->VMod = mod;


    if(RegisterHotKey(0,hk->Id,hk->VMod,hk->VKey))
    {
        Private::HotKeys.Add(hk);
        return 1;
    }
    else
    {
        delete hk;
        return 0;
    }
}

void sUnregisterHotkey(uint key)
{
    for(auto hk : Private::HotKeys)
    {
        if(hk->Key==key)
        {
            UnregisterHotKey(0,hk->Id);
            HotKeys.Rem(hk);
            delete hk;
            break;
        }
    }
}

/****************************************************************************/
/****************************************************************************/

void sInitKeys()
{
  static const uint keys[][2] =
  {
    { sKEY_Enter    , VK_RETURN   },
    { sKEY_Up       , VK_UP       },
    { sKEY_Down     , VK_DOWN     },
    { sKEY_Left     , VK_LEFT     },
    { sKEY_Right    , VK_RIGHT    },
    { sKEY_PageUp   , VK_PRIOR    },
    { sKEY_PageDown , VK_NEXT     },
    { sKEY_Home     , VK_HOME     },
    { sKEY_End      , VK_END      },
    { sKEY_Insert   , VK_INSERT   },
    { sKEY_Delete   , VK_DELETE   },
    { sKEY_Backspace, VK_BACK     },
    { sKEY_Pause    , VK_PAUSE    },
    { sKEY_Scroll   , VK_SCROLL   },
    { sKEY_Numlock  , VK_NUMLOCK  },
    { sKEY_Print    , VK_SNAPSHOT },
    { sKEY_WinL     , VK_LWIN     },
    { sKEY_WinR     , VK_RWIN     },
    { sKEY_WinM     , VK_APPS     },
    { sKEY_Shift    , VK_SHIFT    },
    { sKEY_Ctrl     , VK_CONTROL  },
    { sKEY_Caps     , VK_CAPITAL  },
    { sKEY_F1       , VK_F1       },
    { sKEY_F2       , VK_F2       },
    { sKEY_F3       , VK_F3       },
    { sKEY_F4       , VK_F4       },
    { sKEY_F5       , VK_F5       },
    { sKEY_F6       , VK_F6       },
    { sKEY_F7       , VK_F7       },
    { sKEY_F8       , VK_F8       },
    { sKEY_F9       , VK_F9       },
    { sKEY_F10      , VK_F10      },
    { sKEY_F11      , VK_F11      },
    { sKEY_F12      , VK_F12      },

    { '0'|sKEYQ_Numeric, VK_NUMPAD0  },
    { '1'|sKEYQ_Numeric, VK_NUMPAD1  },
    { '2'|sKEYQ_Numeric, VK_NUMPAD2  },
    { '3'|sKEYQ_Numeric, VK_NUMPAD3  },
    { '4'|sKEYQ_Numeric, VK_NUMPAD4  },
    { '5'|sKEYQ_Numeric, VK_NUMPAD5  },
    { '6'|sKEYQ_Numeric, VK_NUMPAD6  },
    { '7'|sKEYQ_Numeric, VK_NUMPAD7  },
    { '8'|sKEYQ_Numeric, VK_NUMPAD8  },
    { '9'|sKEYQ_Numeric, VK_NUMPAD9  },
    { '*'|sKEYQ_Numeric, VK_MULTIPLY },
    { '+'|sKEYQ_Numeric, VK_ADD      },
    { ','|sKEYQ_Numeric, VK_DECIMAL  },
    { '/'|sKEYQ_Numeric, VK_DIVIDE   },
    { '-'|sKEYQ_Numeric, VK_SUBTRACT },
    { 0 }
  };
//  static const int16 *symbols = L"€";

  // ranges for unicode codepages (end is -EXCLUSIVE-!)
  // see http://www.unicode.org/charts
  static const int ranges[][2] =
  {
    { 0x000, 0x0ff},       // asciitable
    { 0x400, 0x500},       // cyrillic
    { 0x2000, 0x2100 },
  };

  sClear(sKeyTable);

  for (int k=0; k<sCOUNTOF(ranges); k++)
  {
    for(int i=ranges[k][0];i<ranges[k][1];i++)
    {
      int r = VkKeyScanW(i);
      if((r&255)!=255)
      {
        int t = 0;
        if(r&0x100)
          t = 1;
        if(r&0x200)
          t = -1;
        if(r&0x400)
          t = 2;

        if(t>=0)
          sKeyTable[t][r&255] = i;
      }
    }
  }
/*
  for(int j=0;symbols[j];j++)
  {
    int i = symbols[j];
    int r = VkKeyScanW(i);
    if((r&255)!=255)
    {
      int t = 0;
      if(r&0x100)
        t = 1;
      if(r&0x400)
        t = 2;

      sKeyTable[t][r&255] = i;
    }
  }
*/ 
  for(int i=0;keys[i][0];i++)
  {
    sKeyTable[0][keys[i][1]] = keys[i][0];
    sKeyTable[1][keys[i][1]] = keys[i][0];
    sKeyTable[2][keys[i][1]] = keys[i][0];
  }
}

/****************************************************************************/

void sScreen::SetDragDropCallback(sDelegate1<void,const sDragDropInfo *> del)
{
    DragAcceptFiles((HWND)ScreenWindow,1);
    DragDropDelegate = del;
}

void sScreen::ClearDragDropCallback()
{
    DragAcceptFiles((HWND)ScreenWindow,0);
    DragDropDelegate = sDelegate1<void,const sDragDropInfo *>();
}

/****************************************************************************/

LRESULT WINAPI MsgProc(HWND win,UINT msg,WPARAM wparam,LPARAM lparam)
{
  if(0)
    sDPrintF("%08x %08x %08x\n",msg,wparam,lparam);

  sScreen *scr = 0;

  if(sConfigRender==sConfigRenderDX11 || sConfigRender==sConfigRenderDX9)
  {
    for(auto i : OpenScreens)
    {
      if(i->ScreenWindow==win)
      {
        scr = i;
        break;
      }
    }
  }
  else
  {
    if(!OpenScreens.IsEmpty())
      scr = OpenScreens[0];
  }


  switch(msg)
  {
  case WM_CREATE:
    WindowActive = 1;
    return DefWindowProc(win,msg,wparam,lparam);

  case WM_DESTROY:
    WindowActive = 0;
    PostQuitMessage(0);
    break;

  case WM_CLOSE:
    if(CurrentApp && !CurrentApp->OnClose())
      return 0;
    return DefWindowProc(win,msg,wparam,lparam);

  case WM_PAINT:
    PAINTSTRUCT ps;
    BeginPaint(win,&ps);
    FillRect(ps.hdc,&ps.rcPaint,(HBRUSH) GetStockObject(BLACK_BRUSH));
    EndPaint(win,&ps);    
    return 1;

  case WM_SETCURSOR:
    SetCursor(MouseCursor);
    return DefWindowProc(win,msg,wparam,lparam);

  case WM_MOVE:
    if(Private::IsFullyInitialized)
      Render();
    return DefWindowProc(win,msg,wparam,lparam);

  case WM_SIZE:
    if(!(CurrentMode.Flags & sSM_Fullscreen) && wparam!=SIZE_MINIMIZED && Private::IsFullyInitialized)
    {
      int x = int(lparam&0xffff);
      int y = int(lparam>>16);
      if(CurrentMode.SizeX!=x || CurrentMode.SizeY!=y)
      {
        CurrentMode.SizeX = x;
        CurrentMode.SizeY = y;
        if(win)
        {
          sRestartGfx();
          if(1)
          {
            Render();
            sSleepMs(16);
          }
        }
      }
    }
    break;
  case WM_ACTIVATE:
    {
      bool oldactive = WindowActive;
//      if(CurrentMode.Flags & sSM_Fullscreen)
        WindowActive = ((wparam&0xffff)!=WA_INACTIVE);
//      else
//        WindowActive = (!(wparam&0xffff0000));
      if(WindowActive && !oldactive && sConfigRender==sConfigRenderDX11)
        sRestartGfx();
      KeyQual = 0;
      if(GetKeyState(VK_LSHIFT)  &0x8000) KeyQual |= sKEYQ_ShiftL;
      if(GetKeyState(VK_RSHIFT)  &0x8000) KeyQual |= sKEYQ_ShiftR;
      if(GetKeyState(VK_LCONTROL)&0x8000) KeyQual |= sKEYQ_CtrlL;
      if(GetKeyState(VK_RCONTROL)&0x8000) KeyQual |= sKEYQ_CtrlR;
      if(GetKeyState(VK_LMENU)   &0x8000) KeyQual |= sKEYQ_Alt;
      if(GetKeyState(VK_RMENU)   &0x8000) KeyQual |= sKEYQ_AltGr;
      if(GetKeyState(VK_CAPITAL) &0x0001) KeyQual |= sKEYQ_Caps;

      if(!WindowActive)
      {
          sKeyData kd;
          kd.Key = WindowActive ? sKEY_Activated : sKEY_Deactivated;
          kd.MouseX = NewMouseX;
          kd.MouseY = NewMouseY;
          kd.Timestamp = (int)GetMessageTime();
          CurrentApp->OnKey(kd);
      }
    }

    return DefWindowProc(win,msg,wparam,lparam);

// input

  case WM_KEYUP:
  case WM_SYSKEYUP:
    {
      uint lr = uint(lparam & 0x01000000);
      if(wparam==VK_SHIFT)
      {
        if((lparam & 0x00ff0000) == 0x002a0000)
          KeyQual &= ~sKEYQ_ShiftL;
        else
          KeyQual &= ~sKEYQ_ShiftR;
      }
      if(wparam==VK_CONTROL && !lr)  KeyQual &= ~sKEYQ_CtrlL;
      if(wparam==VK_CONTROL &&  lr)  KeyQual &= ~sKEYQ_CtrlR;
      if(wparam==VK_MENU && !lr)     KeyQual &= ~sKEYQ_Alt;
      if(wparam==VK_MENU &&  lr)     KeyQual &= ~sKEYQ_AltGr;

      // special handling for Alt-F4
      if (wparam == VK_F4 && KeyQual == sKEYQ_Alt)
          return DefWindowProc(win, msg, wparam, lparam);

      int t=0;
      if((KeyQual&sKEYQ_Shift) || (KeyQual&sKEYQ_Caps))
        t = 1;
      if(KeyQual&sKEYQ_AltGr)
      {
        t = 2;
        KeyQual&=~sKEYQ_CtrlL;
      }
      uint i = sKeyTable[t][wparam&255];
      if(i==0)
        i = sKeyTable[0][wparam&255];
      if(wparam==VK_RETURN && lr) 
        i |= sKEYQ_Numeric;

      if(i)
      {
        sKeyData kd;
        kd.Key = i | KeyQual | sKEYQ_Break;
        kd.MouseX = NewMouseX;
        kd.MouseY = NewMouseY;
        kd.Timestamp = (int)GetMessageTime();
        CurrentApp->OnKey(kd);
      }
    }
    break;

  case WM_KEYDOWN:
  case WM_SYSKEYDOWN:
    {
      uint lr = uint(lparam & 0x01000000);

      if(wparam==VK_SHIFT)
      {
        if((lparam & 0x00ff0000) == 0x002a0000)
          KeyQual |= sKEYQ_ShiftL;
        else
          KeyQual |= sKEYQ_ShiftR;
      }
      if(wparam==VK_SHIFT &&  lr)    KeyQual |= sKEYQ_ShiftR;
      if(wparam==VK_CONTROL && !lr)  KeyQual |= sKEYQ_CtrlL;
      if(wparam==VK_CONTROL &&  lr)  KeyQual |= sKEYQ_CtrlR;
      if(wparam==VK_MENU && !lr)     KeyQual |= sKEYQ_Alt;
      if(wparam==VK_MENU &&  lr)     KeyQual |= sKEYQ_AltGr;

      // special handling for Alt-F4
      if (wparam == VK_F4 && KeyQual == sKEYQ_Alt)
          return DefWindowProc(win, msg, wparam, lparam);

      int repeat = (lparam & 0x40000000)?int(sKEYQ_Repeat) : 0;
      if(wparam==VK_CAPITAL && !lr && !repeat)  KeyQual ^= sKEYQ_Caps;

      int t=0;
      if((KeyQual&sKEYQ_Shift) || (KeyQual&sKEYQ_Caps))
        t = 1;
      if(KeyQual&sKEYQ_AltGr)
      {
        t = 2; 
        KeyQual&=~sKEYQ_CtrlL;
      }

      uint i = sKeyTable[t][wparam];
      if(i==0)
        i = sKeyTable[0][wparam];
      if(wparam==VK_RETURN && lr) 
        i |= sKEYQ_Numeric;
      if(i)
      {
        sKeyData kd;
        kd.Key = i | KeyQual | repeat;
        kd.MouseX = NewMouseX;
        kd.MouseY = NewMouseY;
        kd.Timestamp = (int)GetMessageTime();
        CurrentApp->OnKey(kd);
      }
    }
    break;


  case WM_MOUSEWHEEL:
    MouseWheelAkku += int16((wparam>>16)&0xffff);
    while(MouseWheelAkku>=120)
    {
      MouseWheelAkku-=120;
      sKeyData kd;
      kd.Key = sKEY_WheelUp | KeyQual;
      kd.MouseX = NewMouseX;
      kd.MouseY = NewMouseY;
      kd.Timestamp = (int)GetMessageTime();
      CurrentApp->OnKey(kd);
    }
    while(MouseWheelAkku<=-120)
    {
      MouseWheelAkku+=120;
      sKeyData kd;
      kd.Key = sKEY_WheelDown | KeyQual;
      kd.MouseX = NewMouseX;
      kd.MouseY = NewMouseY;
      kd.Timestamp = (int)GetMessageTime();
      CurrentApp->OnKey(kd);
    }
    break;

  case WM_MOUSEMOVE:
    NewMouseX = int16(lparam&0xffff);
    NewMouseY = int16(lparam>>16);
    HandleMouse((int)GetMessageTime(),0,scr);
    break;

  case WM_LBUTTONDOWN:
    NewMouseButtons |= sMB_Left;
    HandleMouse((int)GetMessageTime(),0,scr);
    SetCapture(win);
    break;
  case WM_RBUTTONDOWN:
    NewMouseButtons |= sMB_Right;
    HandleMouse((int)GetMessageTime(),0,scr);
    SetCapture(win);
    break;
  case WM_MBUTTONDOWN:
    NewMouseButtons |= sMB_Middle;
    HandleMouse((int)GetMessageTime(),0,scr);
    SetCapture(win);
    break;
  case WM_XBUTTONDOWN:
    switch(HIWORD(wparam))
    {
    case XBUTTON1:
      NewMouseButtons |= sMB_Extra1;
      HandleMouse((int)GetMessageTime(),0,scr);
      SetCapture(win);
      break;
    case XBUTTON2:
      NewMouseButtons |= sMB_Extra2;
      HandleMouse((int)GetMessageTime(),0,scr);
      SetCapture(win);
      break;
    }
    break;

  case WM_LBUTTONDBLCLK:
    NewMouseButtons |= sMB_Left;
    HandleMouse((int)GetMessageTime(),sMB_DoubleClick|sMB_Left,scr);
    SetCapture(win);
    break;
  case WM_RBUTTONDBLCLK:
    NewMouseButtons |= sMB_Right;
    HandleMouse((int)GetMessageTime(),sMB_DoubleClick|sMB_Right,scr);
    SetCapture(win);
    break;
  case WM_MBUTTONDBLCLK:
    NewMouseButtons |= sMB_Middle;
    HandleMouse((int)GetMessageTime(),sMB_DoubleClick|sMB_Middle,scr);
    SetCapture(win);
    break;
  case WM_XBUTTONDBLCLK:
    switch(HIWORD(wparam))
    {
    case XBUTTON1:
      NewMouseButtons |= sMB_Extra1;
      HandleMouse((int)GetMessageTime(),sMB_DoubleClick|sMB_Extra1,scr);
      SetCapture(win);
      break;
    case XBUTTON2:
      NewMouseButtons |= sMB_Extra2;
      HandleMouse((int)GetMessageTime(),sMB_DoubleClick|sMB_Extra2,scr);
      SetCapture(win);
      break;
    }
    break;


  case WM_LBUTTONUP:
    NewMouseButtons &=~sMB_Left;
    HandleMouse((int)GetMessageTime(),0,scr);
    if(NewMouseButtons==0) ReleaseCapture();
    break;
  case WM_RBUTTONUP:
    NewMouseButtons &=~sMB_Right;
    HandleMouse((int)GetMessageTime(),0,scr);
    if(NewMouseButtons==0) ReleaseCapture();
    break;
  case WM_MBUTTONUP:
    NewMouseButtons &=~sMB_Middle;
    HandleMouse((int)GetMessageTime(),0,scr);
    if(NewMouseButtons==0) ReleaseCapture();
    break;
  case WM_XBUTTONUP:
    switch(HIWORD(wparam))
    {
    case XBUTTON1:
      NewMouseButtons &=~sMB_Extra1;
      HandleMouse((int)GetMessageTime(),0,scr);
      if(NewMouseButtons==0) ReleaseCapture();
      break;
    case XBUTTON2:
      NewMouseButtons &=~sMB_Extra2;
      HandleMouse((int)GetMessageTime(),0,scr);
      if(NewMouseButtons==0) ReleaseCapture();
      break;
    }
    break;
    case WM_DROPFILES:
        {
            HDROP drop = (HDROP) wparam;
            struct sDragDropInfo ddi;

            POINT p;
            DragQueryPoint(drop,&p);
            ddi.PosX = p.x;
            ddi.PosY = p.y;
            wchar Buffer[sMaxPath];

            int count = DragQueryFileW(drop,~0U,Buffer,sMaxPath);
            for(int i=0;i<count;i++)
            {
                int r = DragQueryFileW(drop,i,Buffer,sMaxPath);
                if(r>0)
                    ddi.Names.Add(sUTF16toUTF8((const uint16 *)Buffer));
            }

            DragFinish(drop);

            scr->CallDragDropCallback(&ddi);

            ddi.Names.DeleteAll();
        }
        break;

    case WM_INPUT:
        {
            uint buffer[256];
            UINT size = sizeof(buffer);

            if(GetRawInputData((HRAWINPUT)lparam,RID_INPUT,buffer,&size,sizeof(RAWINPUTHEADER))>0 && size<=sizeof(buffer))
            {
                RAWINPUT *raw = (RAWINPUT *) buffer;
                if(raw->header.dwType==RIM_TYPEMOUSE)
                {
                    if(raw->data.mouse.lLastX!=0 || raw->data.mouse.lLastY!=0)
                    {
                        RawMouseX += float(raw->data.mouse.lLastX);
                        RawMouseY += float(raw->data.mouse.lLastY);
                        HandleMouse((int)GetMessageTime(),0,scr);
                    }
                }

                /*
                sDPrintF("%08x %1x %1x %1x %1x %08x %08x %08x\n"
                    ,(int)raw->data.mouse.usFlags
                    ,(int)raw->data.mouse.ulButtons,(int)raw->data.mouse.usButtonFlags,(int)raw->data.mouse.usButtonData,(int)raw->data.mouse.ulRawButtons
                    ,(int)raw->data.mouse.lLastX,(int)raw->data.mouse.lLastY,(int)raw->data.mouse.ulExtraInformation);
                    */
            }
        }
        break;

// done

  default:
    {
        sWinMessage hmsg;
        hmsg.Window = win;
        hmsg.Message = msg;
        hmsg.lparam = lparam;
        hmsg.wparam = wparam;
        sWinMessageHook.Call(hmsg);
        return DefWindowProc(win,msg,wparam,lparam);
    }
  }
  return 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   Time and Date                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void sGetAppId(sString<sMaxPath> &udid)
{
    sCopyString(udid.GetBuffer(), "Windows", sizeof("Windows"));
}


uint sGetTimeMS()
{
  return timeGetTime()-StartTimeTick;
}

uint64 sGetTimeUS()
{
  LARGE_INTEGER pc, pf;
  QueryPerformanceCounter(&pc);
  QueryPerformanceFrequency(&pf);
  return pc.QuadPart*1000000UL/pf.QuadPart;   // this is WRONG WRONG WRONG
}

uint64 sGetTimeRandom()
{
  LARGE_INTEGER pc;
  QueryPerformanceCounter(&pc);
  return pc.QuadPart-StartPTTick;
}

void sGetTimeAndDate(sTimeAndDate &date)
{
  SYSTEMTIME st;

  GetSystemTime(&st);

  date.Year = st.wYear;
  date.Month = st.wMonth-1;
  date.Day = st.wDay-1;
  date.Second = st.wSecond + st.wMinute*60 + st.wHour*60*60;
}

double sGetTimeSince2001()
{
  sFatal("not implemented");
}

/****************************************************************************/

void sUpdateHRFrequency()
{
  LARGE_INTEGER out;
  QueryPerformanceCounter(&out);
  uint64 pttick = out.QuadPart;
  uint64 hrtick = sGetTimeHR();
  if(PTFrequency==0)
  {
    QueryPerformanceFrequency(&out);
    PTFrequency = out.QuadPart;
    HRFrequency = 1000*1000;
  }
  else
  {
    double time = double(pttick-LastPTTick)/double(PTFrequency);
    HRFrequency = uint64((hrtick-LastHRTick)/time);
  }
  LastPTTick = pttick;
  LastHRTick = hrtick;
}

uint64 sGetHRFrequency()
{
  return HRFrequency;
}

class sHRTimerSubsystem : public sSubsystem
{
public:
    sHRTimerSubsystem() : sSubsystem("High Resolution Timer",0x02) {}

    void Init()
    {
        timeBeginPeriod(1);
        sUpdateHRFrequency();
        sPreFrameHook.Add(sDelegate<void>(sUpdateHRFrequency));

        LARGE_INTEGER out;
        QueryPerformanceCounter(&out);
        StartPTTick = out.QuadPart;
        StartTimeTick = timeGetTime();
    }
    void Edit()
    {
        timeEndPeriod(1);
    }
} sHRTimerSubsystem_;

/****************************************************************************/
/***                                                                      ***/
/***   Hardware Mouse Cursor                                              ***/
/***                                                                      ***/
/****************************************************************************/

class sHardwareCursorSubsystem : public sSubsystem
{
public:
  sHardwareCursorSubsystem() : sSubsystem("Hardware Mousecursor",0x50) {}

  HCURSOR Cursors[sHCI_Max];
  sHardwareCursorImage Current;
  bool Enabled;

  void Init()
  {
    Enabled = 1;
    Current = sHCI_Max;
    sClear(Cursors);
  }
  void Exit()
  {
    for(int i=0;i<sHCI_Max;i++)
      if(Cursors[i])
        DestroyCursor(Cursors[i]);
    sClear(Cursors);
  }

  void Set(sHardwareCursorImage img)
  {
    const static LPWSTR WinName[sHCI_Max] =
    {
      IDC_ARROW,
      IDC_ARROW,
      IDC_CROSS,
      IDC_HAND,
      IDC_IBEAM,

      IDC_NO,
      IDC_WAIT,
      IDC_SIZEALL,
      IDC_SIZEWE,
      IDC_SIZENS,
    };

    if(img!=Current)
    {
      Current = img;
      if(Cursors[Current]==0 && Current!=sHCI_Off)
        Cursors[Current] = LoadCursorW(0,WinName[Current]);
      SetCursor(Cursors[Current]);
      MouseCursor = Cursors[Current];
    }
  }
  void Enable(bool enable)
  {
    if(enable && !Enabled)
      ShowCursor(1);
    if(!enable && Enabled)
      ShowCursor(0);
    Enabled = enable;
  }

} sHardwareCursorSubsystem_;

void sSetHardwareCursor(sHardwareCursorImage img)
{
  sHardwareCursorSubsystem_.Set(img);
}

void sEnableHardwareCursor(bool enable)
{
  sHardwareCursorSubsystem_.Enable(enable);
}

/****************************************************************************/
/***                                                                      ***/
/***   Ios Features                                                       ***/
/***                                                                      ***/
/****************************************************************************/

void sEnableVirtualKeyboard(bool enable)
{
}

float sGetDisplayScaleFactor()
{
  return 1.0f;
}

const char *sGetDeviceName()
{
  return "Windows";
}

const char *sGetUserName()
{
  static sString<sFormatBuffer> buffer;

  if(buffer[0]==0)
  {
    uint16 utf16[sFormatBuffer];
    DWORD len = sFormatBuffer-1;
    GetUserNameW((LPWSTR)utf16,&len);
    utf16[sFormatBuffer-1];

    sUTF16toUTF8(utf16,buffer);
  }

  return buffer;
}

/****************************************************************************/
/***                                                                      ***/
/***   Render Font to Bitmap                                              ***/
/***                                                                      ***/
/****************************************************************************/

sSystemFontInfoPrivate::sSystemFontInfoPrivate()
{
  lf = 0;
}

sSystemFontInfoPrivate::~sSystemFontInfoPrivate()
{
  delete lf;
}

static sArray<const sSystemFontInfo *> *sLoadSystemFontDirArray;

static int CALLBACK sLoadSystemFontDirEnum(
  const LOGFONT *lf,
  const TEXTMETRIC *tm,
  DWORD FontType,
  LPARAM lParam)
{
  sSystemFontInfo *sfi = new sSystemFontInfo;
  LOGFONTW *lfcopy = new LOGFONTW;
  *lfcopy = *lf;
  sfi->lf = lfcopy;
  sString<sFormatBuffer> buffer;
  const uint16 *str = (const uint16 *)((ENUMLOGFONTEX *)lf)->elfFullName;
  sUTF16toUTF8(str,buffer);
  sfi->Name = sAllocString(buffer);
  sfi->Flags = sSFF_Italics|sSFF_Bold;
  if((lf->lfPitchAndFamily & 0x0f)!=1)
    sfi->Flags |= sSFF_Proportional;
  if(0)
  {
    sDPrintF("enum font %2dx%2d [bius %03d %1d %1d %1d] ",
      lf->lfWidth,lf->lfHeight,lf->lfWeight,lf->lfItalic,lf->lfUnderline,lf->lfStrikeOut);
    sDPrintF("charset %02x prec %d clip %d qual %d pitchandfamily %08x <%s>\n",
      lf->lfCharSet,lf->lfOutPrecision,lf->lfClipPrecision,lf->lfQuality,lf->lfPitchAndFamily,buffer);
  }
  sLoadSystemFontDirArray->Add(sfi);
  return 1;
}

const sSystemFontInfo *sGetSystemFont(sSystemFontId id)
{
  sSystemFontInfo *sfi = new sSystemFontInfo;
  LOGFONTW *lf = new LOGFONT;
  sClear(*lf);
  lf->lfCharSet = ANSI_CHARSET;
  sfi->lf = lf;
  switch(id)
  {
  case sSFI_Console:
    copywstring(lf->lfFaceName,L"Courier New");
    lf->lfPitchAndFamily = FIXED_PITCH|FF_DONTCARE;
    break;
  case sSFI_Serif:
    copywstring(lf->lfFaceName,L"Times New Roman");
    lf->lfPitchAndFamily = VARIABLE_PITCH|FF_ROMAN;
    break;
  case sSFI_SansSerif:
    copywstring(lf->lfFaceName,L"Verdana");
    lf->lfPitchAndFamily = VARIABLE_PITCH|FF_SWISS;
    break;
  case sSFI_Complete:
    copywstring(lf->lfFaceName,L"Arial Unicode MS");
    lf->lfPitchAndFamily = DEFAULT_PITCH|FF_DONTCARE;
    break;
  default:
    sDelete(sfi);
    break;
  }
  return sfi;
}

bool sLoadSystemFontDir(sArray<const sSystemFontInfo *> &dir)
{
  LOGFONTW lf;

  sClear(lf);
  lf.lfCharSet = ANSI_CHARSET;

  sLoadSystemFontDirArray = &dir;

  HDC screendc = GetDC(0);
  EnumFontFamiliesExW(screendc,&lf,sLoadSystemFontDirEnum,0,0);
  ReleaseDC(0,screendc);

  return 1;
}

/****************************************************************************/

struct sSystemFontPrivate_
{
  HFONT Font;
  HDC DC;
  HDC CDC;
  HBITMAP Bitmap;
  TEXTMETRIC TextMetric;
};

sSystemFont *sLoadSystemFont(const sSystemFontInfo *fi,int sx,int sy,int flags)
{
  sSystemFont *font = new sSystemFont();
  font->AA = (flags & sSFF_Antialias) ? 8 : 1;
  LOGFONTW lf = *(LOGFONTW *)fi->lf;
  lf.lfHeight = sMax(1,sy*font->AA);
  lf.lfWidth = sx*font->AA;
  if(flags & sSFF_Italics)
    lf.lfItalic = 1;
  if(flags & sSFF_Bold)
    lf.lfWeight = FW_BOLD;
  lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
  lf.lfClipPrecision = OUT_DEFAULT_PRECIS;
  lf.lfQuality = ANTIALIASED_QUALITY;
  HFONT hfont = CreateFontIndirectW(&lf);
  font->p->Font = hfont;
  font->p->DC = GetDC(0);
  font->p->CDC = CreateCompatibleDC(font->p->DC);

  SelectObject(font->p->CDC,font->p->Font);
  GetTextMetricsW(font->p->CDC,&font->p->TextMetric);

  // get kern pairs
  KERNINGPAIR *kerns = 0;
  int kernused = (int)GetKerningPairsW(font->p->CDC, 0, 0);
  kerns = new KERNINGPAIR[kernused];
  kernused = (int) GetKerningPairsW(font->p->CDC,kernused,kerns);

  font->KernPairs.SetSize(kernused);
  for(int i=0;i<kernused;i++)
  {
      font->KernPairs[i].CodeA = kerns[i].wFirst;
      font->KernPairs[i].CodeB = kerns[i].wSecond;
      font->KernPairs[i].Kern = kerns[i].iKernAmount;
  }
  delete[] kerns;

  // done

  return font;
}

sSystemFontPrivate::sSystemFontPrivate()
{
  p = new sSystemFontPrivate_;
  p->Font = 0;
  p->DC = 0;
  p->Bitmap = 0;
  sClear(p->TextMetric);
}

sSystemFontPrivate::~sSystemFontPrivate()
{
  if(p->Font)
    DeleteObject(p->Font);
  if(p->Bitmap)
    DeleteObject(p->Bitmap);
  if(p->DC)
    ReleaseDC(0,p->DC);
  if(p->CDC)
    DeleteDC(p->CDC);
  delete p;
}
int sSystemFont::GetAdvance()
{
  return (p->TextMetric.tmHeight + p->TextMetric.tmExternalLeading)/AA;
}

int sSystemFont::GetBaseline()
{
  return (p->TextMetric.tmHeight - p->TextMetric.tmInternalLeading)/AA;
}

int sSystemFont::GetCellHeight()
{
  return (p->TextMetric.tmHeight+AA-1)/AA;
}

bool sSystemFont::GetInfo(int code,int *abc)
{
  ABC wabc;
  if(GetCharABCWidths(p->CDC,code,code,&wabc))
  {

    int x0 = 0;
    int x1 = x0+wabc.abcA;
    int x2 = x1+wabc.abcB;
    int x3 = x2+wabc.abcC;

    x1 =  x1      /AA;
    x2 = (x2+AA-1)/AA;
    x3 = (x3+AA-1)/AA;

    abc[0] = x1;
    abc[1] = x2-x1;
    abc[2] = x3-x2;
  }
  else
  {
    INT w;
    GetCharWidth(p->CDC,code,code,&w);
    abc[0] = 0;
    abc[1] = w;
    abc[2] = 0;
  }
  return 1;
}

bool sSystemFont::CodeExists(int code)
{
    if(code>=0x10000)
        return 0;
    uint16 buffer[2];
    WORD out[1];
    buffer[0] = code;
    buffer[1] = 0;

    DWORD glyph = GetGlyphIndicesW(p->CDC,(LPCTSTR) buffer,1,&out[0],GGI_MARK_NONEXISTING_GLYPHS);

    return out[0]!=0xffff;
}

sOutline *sSystemFont::GetOutline(const char *text)
{
  sOutline *out = new sOutline;

  // render

  BeginPath(p->CDC);
  uint16 buffer[2048];
  int charcount = (int)sUTF8ToWide(buffer,text,2048);
  TextOutW(p->CDC,0,0,(WCHAR*)buffer,charcount);
  EndPath(p->CDC);
  
  // get path

  int n = GetPath(p->CDC,0,0,0);
  if(n==0)
    return out;
  POINT *v = new POINT[n];
  BYTE *t = new BYTE[n];
  GetPath(p->CDC,v,t,n);

  // create all vertices (even though some are not used)

  out->Vertices.SetSize(n);
  for(int j=0;j<n;j++)
    out->Vertices[j] = new sOutline::Vertex((float)v[j].x,(float)v[j].y);

  // create segments and contours

  int i = 4;
  while(i<n)
  {
    sASSERT(t[i]==PT_MOVETO);
    sOutline::Vertex *prevvert = out->Vertices[i];
    sOutline::Vertex *firstvert = prevvert;
    sOutline::Segment *firstseg = 0;
    sOutline::Segment **link = &firstseg;

    i++;

    for(;;)
    {
      sASSERT(i<n);
      if(t[i]==PT_LINETO)
      {
        sOutline::Segment *s = out->AddSegment();
        s->P0 = prevvert;
        prevvert = out->Vertices[i];
        s->P1 = prevvert;
        *link = s;
        link = &s->Next;
        i++;
      }
      else if(t[i]==(PT_LINETO|PT_CLOSEFIGURE))
      {
        sOutline::Segment *s = out->AddSegment();
        s->P0 = prevvert;
        prevvert = out->Vertices[i];
        s->P1 = prevvert;
        *link = s;
        link = &s->Next;

        s = out->AddSegment();
        s->P0 = prevvert;
        s->P1 = firstvert;
        *link = s;
        s->Next = firstseg;
        i++;
        out->Contours.Add(firstseg);
        break;
      }
      else if(t[i]==PT_BEZIERTO/* || t[i]==(PT_BEZIERTO|PT_CLOSEFIGURE)*/)
      {
        sOutline::Segment *s = out->AddSegment();
        s->P0 = prevvert;
        prevvert = out->Vertices[i+2];
        s->P1 = prevvert;
        s->C0 = out->Vertices[i+0];
        s->C1 = out->Vertices[i+1];
        *link = s;
        link = &s->Next;
        i+=3;
      }
      else
      {
        sASSERT0();
      }
    }
  }

  // make list double linked

  for(auto s : out->Segments)
    s->Next->Prev = s;

  if(sConfigDebug)
    out->Assert();

  // remove null-segments

  for(auto &start : out->Contours)
  {
      auto s = start;
      do
      {
          if(s->C0==0 && s->P0->Pos==s->P1->Pos)
          {
              s->Prev->Next = s->Next;
              s->Next->Prev = s->Prev;
              s->Next->P0 = s->Prev->P1;

              if(s==start)
                  start = s->Prev;
          }
          s = s->Next;
      }
      while(s!=start);
  }
  // done

  if(sConfigDebug)
    out->Assert();

  delete[] t;
  delete[] v;
  return out;
}

void sSystemFont::BeginRender(int sx,int sy,uint8 *data)
{
  sASSERT(!p->Bitmap);
  SizeX = sx*AA;
  SizeY = sy*AA;
  Data = data;

  p->Bitmap = CreateCompatibleBitmap(p->DC,SizeX,SizeY);
  SelectObject(p->CDC,p->Bitmap);

  SetBkMode(p->CDC,TRANSPARENT);
  SetTextColor(p->CDC,0xffffff);
  BitBlt(p->CDC,0,0,SizeX,SizeY,p->CDC,0,0,BLACKNESS);
  SetTextAlign(p->CDC,TA_TOP|TA_LEFT);
}

void sSystemFont::RenderChar(int code,int px,int py)
{
  sASSERT(p->Bitmap);
  uint16 text[2];
  text[0] = code;
  text[1] = 0;
  ExtTextOutW(p->CDC,px*AA,py*AA,0,0,(LPCWSTR)text,1,0);
}

void sSystemFont::EndRender()
{
  sASSERT(p->Bitmap);

  // copy bitmap to temporary buffer

  BITMAPINFO bmi;
  uint *bgra = new uint[SizeX*SizeY];

  sClear(bmi);
  bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
  bmi.bmiHeader.biWidth = SizeX;
  bmi.bmiHeader.biHeight = -SizeY;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;
  GetDIBits(p->CDC,p->Bitmap,0,SizeY,bgra,&bmi,DIB_RGB_COLORS);

  // copy temporary bgra-buffer to final a-buffer
  // use blue channel only

  if(AA==1)
  {
    for(int i=0;i<SizeX*SizeY;i++)
      Data[i] = bgra[i]&255;
  }
  else
  {
    int sx = SizeX/AA;
    int sy = SizeY/AA;
    for(int y=0;y<sy;y++)
    {
      for(int x=0;x<sx;x++)
      {
        int akku = 0;
        for(int yy=0;yy<AA;yy++)
          for(int xx=0;xx<AA;xx++)
            akku += bgra[(y*AA+yy)*SizeX+x*AA+xx]&255;
        Data[y*sx+x] = akku/(AA*AA);
      }
    }
  }

  // done

  delete[] bgra;
  DeleteObject(p->Bitmap);
  p->Bitmap = 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   Clipboard                                                          ***/
/***                                                                      ***/
/****************************************************************************/

class sClipboardWindows : public sClipboardBase
{
  uint ClipboardFormat;
public:
  sClipboardWindows()
  {
    ClipboardFormat = RegisterClipboardFormatW(L"Altona2 Clipboard Object");
    if(ClipboardFormat==0)
      sFatal("failed to register clipboard format");
  }

  ~sClipboardWindows()
  {
  }


  void SetText(const char *str,uptr len=-1)
  {
    OpenClipboard(0);
    EmptyClipboard();
    if(len==-1)
      len = sGetStringLen(str);
    const char *end = str+len;
    const char *scan = str;

    uptr size = 1;
    while(scan<end)
    {
      int c = sReadUTF8(scan);
      size++;
      if(c=='\n')
        size++;
    }

    HANDLE hmem = GlobalAlloc(GMEM_MOVEABLE,size*2);

    uint16 *d = (uint16 *) GlobalLock(hmem);
    uint16 *dstart = d;
    scan = str;
    while(scan<end)
    {
      int c = sReadUTF8(scan);
      if(c=='\n')
        *d++ = '\r';
      *d++ = c;      // ignore surrogates. windows can't handle them anyway.
    }
    *d++ = 0;
    sASSERT((d-dstart)==size);
    GlobalUnlock(hmem);

    SetClipboardData(CF_UNICODETEXT,hmem);
    CloseClipboard();
  }

  void SetBlob(const void *data,uptr size,int serid)
  {
    OpenClipboard(0);
    EmptyClipboard();

    HANDLE hmem = GlobalAlloc(GMEM_MOVEABLE,size+sizeof(uint64)*2);

    uint8 *d = (uint8 *) GlobalLock(hmem);
    ((uint64 *)d)[0] = size;
    ((uint64 *)d)[1] = serid;
    sCopyMem(d+sizeof(uint64)*2,data,size);

    GlobalUnlock(hmem);
    SetClipboardData(ClipboardFormat,hmem);
    CloseClipboard();
  }

  char *GetText()
  {
    char *text=0;

    OpenClipboard(0);

    HANDLE hmem = GetClipboardData(CF_UNICODETEXT);
    if(hmem)
    {
      const uint16 *data = (const uint16 *)GlobalLock(hmem);
      text = sUTF16toUTF8(data);

      char *s = text;
      char *d = text;
      while(*s)
      {
        if(*s!='\r')
          *d++ = *s;
        s++;
      }
      *d++ = 0;

      GlobalUnlock(hmem);
      CloseClipboard();
    }
    else
    {
      char *text = new char [1];
      text[0] = 0;
    }

    CloseClipboard();
    return text;
  }

  void *GetBlob(uptr &size,int &serid)
  {
    uint8 *data = 0;
    size = 0;

    OpenClipboard(0);

    HANDLE hmem = GetClipboardData(ClipboardFormat);
    if(hmem)
    {
      uint8 *d = (uint8 *) GlobalLock(hmem);
      size = uptr(((uint64 *)d)[0]);
      if(((uint64 *)d)[1] == serid)
      {
        data = new uint8[size];
        sCopyMem(data,d+sizeof(uint64)*2,size);
      }

      GlobalUnlock(hmem);
    }

    CloseClipboard();

    return data;
  }
};



class sClipboardSubsystem : public sSubsystem
{
public:
  sClipboardSubsystem() : sSubsystem("Clipboard",0x90) {}

  void Init()
  {
    sClipboard = new sClipboardWindows;
  }
  void Exit()
  {
    delete sClipboard;
  }
} sClipboardSubsystem_;

sClipboardBase *sClipboard;


/****************************************************************************/
/***                                                                      ***/
/***   Resolve Link File                                                  ***/
/***                                                                      ***/
/****************************************************************************/

bool sResolveWindowsLink(const char *filename,const sStringDesc &buffer) 
{ 
    HRESULT hres; 
    IShellLink* psl; 
    WCHAR szGotPath[MAX_PATH]; 
    WCHAR szDescription[MAX_PATH]; 
    WIN32_FIND_DATA wfd; 

    bool ok = 0;
 
    // Get a pointer to the IShellLink interface. It is assumed that CoInitialize
    // has already been called. 
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl); 
    if (SUCCEEDED(hres)) 
    { 
        IPersistFile* ppf; 
 
        // Get a pointer to the IPersistFile interface. 
        hres = psl->QueryInterface(IID_IPersistFile, (void**)&ppf); 
        
        if (SUCCEEDED(hres)) 
        { 
            WCHAR wsz[MAX_PATH]; 
 
            sUTF8toUTF16(filename,(uint16 *)wsz,MAX_PATH-1);
  
            // Load the shortcut. 
            hres = ppf->Load(wsz, STGM_READ); 
            
            if (SUCCEEDED(hres)) 
            { 
                // Resolve the link. 
                hres = psl->Resolve(0, 0); 

                if (SUCCEEDED(hres)) 
                { 
                    // Get the path to the link target. 
                    hres = psl->GetPath(szGotPath, MAX_PATH, (WIN32_FIND_DATA*)&wfd, SLGP_SHORTPATH); 

                    if (SUCCEEDED(hres)) 
                    { 
                        // Get the description of the target. 
                        hres = psl->GetDescription(szDescription, MAX_PATH); 

                        if (SUCCEEDED(hres)) 
                        {
                            sUTF16toUTF8((const uint16 *)szGotPath,buffer);
                            ok = 1;
                        }
                    }
                } 
            } 

            // Release the pointer to the IPersistFile interface. 
            ppf->Release(); 
        } 

        // Release the pointer to the IShellLink interface. 
        psl->Release(); 
    } 
    return ok; 
}

/****************************************************************************/
/***                                                                      ***/
/***   Multithreading                                                     ***/
/***                                                                      ***/
/****************************************************************************/

void sSleepMs(int ms)
{
  Sleep(ms);
}

int sGetCPUCount()
{
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  return info.dwNumberOfProcessors;
}

/****************************************************************************/

void sStartThread(sThread *th)
{
  th->Code.Call(th);
}

unsigned long __stdcall sThreadTrunk(void *ptr)
{
  sStartThread((sThread *) ptr);
  return 0;
}

sThread::sThread(const sDelegate1<void,sThread *> &code,int pri,int stacksize, int flags)
{
  TerminateFlag = 0;
  Code = code;

  ULONG ThreadId;
  ThreadHandle = CreateThread(0,stacksize,sThreadTrunk,this,0,(ULONG *)&ThreadId);
  if(pri>0)
  {
    SetPriorityClass(ThreadHandle,HIGH_PRIORITY_CLASS);
    SetThreadPriority(ThreadHandle,THREAD_PRIORITY_TIME_CRITICAL);
  }
  if(pri==0)
  {
//    SetThreadPriorityBoost(ThreadHandle,TRUE);
  }
  if(pri<0)
  {
    SetThreadPriority(ThreadHandle,THREAD_PRIORITY_BELOW_NORMAL);
  }
}

sThread::~sThread()
{
  if(TerminateFlag==0)
    Terminate();
  if(TerminateFlag==1)
    WaitForSingleObject(ThreadHandle,INFINITE);
  CloseHandle(ThreadHandle);
}

void sThread::Terminate()
{ 
  TerminateFlag = 1; 
}

bool sThread::CheckTerminate()
{ 
  return !TerminateFlag; 
}

/****************************************************************************/

sThreadLock::sThreadLock()
{
  CriticalSection = new CRITICAL_SECTION;
  InitializeCriticalSection((CRITICAL_SECTION*)CriticalSection);
}

sThreadLock::~sThreadLock()
{
  DeleteCriticalSection((CRITICAL_SECTION*)CriticalSection);
  delete CriticalSection;
}


void sThreadLock::Lock()
{
  EnterCriticalSection((CRITICAL_SECTION*)CriticalSection);
}

bool sThreadLock::TryLock()
{
  return TryEnterCriticalSection((CRITICAL_SECTION*)CriticalSection)?1:0;
}

void sThreadLock::Unlock()
{
  LeaveCriticalSection((CRITICAL_SECTION*)CriticalSection);
}

/****************************************************************************/

sThreadEvent::sThreadEvent(bool manual)
{
  EventHandle = CreateEvent(0,manual?TRUE:0,0,0);
}

sThreadEvent::~sThreadEvent()
{
  CloseHandle(EventHandle);
}

bool sThreadEvent::Wait(int timeout)
{
  return WaitForSingleObject(EventHandle,timeout)==WAIT_OBJECT_0;
}

void sThreadEvent::Signal()
{
  SetEvent(EventHandle);
}

void sThreadEvent::Reset()
{
  ResetEvent(EventHandle);
}

/****************************************************************************/
/***                                                                      ***/
/***   Inter Process Communication                                        ***/
/***                                                                      ***/
/****************************************************************************/

sSharedMemory::sSharedMemory()
{
  FileMapping = INVALID_HANDLE_VALUE;
  Memory = 0;
  Initialized = 0;
}

sSharedMemory::~sSharedMemory()
{
  Exit();
}

void *sSharedMemory::Init(const char *name,uptr size,bool master,int sections)
{
  sASSERT(!Initialized);
  Master = master;

  sString<sFormatBuffer> name8;
  uint16 name16[sFormatBuffer];

  name8.PrintF("Local\\%s_mapping",name);
  sUTF8toUTF16(name8,name16,sFormatBuffer);

#if sConfig64Bit
  uint high = DWORD(size>>32);
#else
  uint high = 0;
#endif

  if(master)
    FileMapping = CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,high,DWORD(size),LPCWSTR(name16));
  else
    FileMapping = OpenFileMappingW(FILE_MAP_READ|FILE_MAP_WRITE,0,LPCWSTR(name16));

  if(FileMapping==INVALID_HANDLE_VALUE)
    goto fail;

  Memory = MapViewOfFile(FileMapping,FILE_MAP_WRITE,0,0,size);

  // the sections

  for(int i=0;i<sections;i++)
  {
    // mutex

    name8.PrintF("Local\\%s_mutex_%d",name,i);
    sUTF8toUTF16(name8,name16,sFormatBuffer);
    void *handle = INVALID_HANDLE_VALUE;

    if(master)
      handle = CreateMutexW(NULL,0,LPCWSTR(name16));
    else
      handle = OpenMutexW(SYNCHRONIZE,0,LPCWSTR(name16));

    if(handle==INVALID_HANDLE_VALUE)
      goto fail;
    SectionMutex.Add(handle);

    // master to slave

    name8.PrintF("Local\\%s_mtos_%d",name,i);
    sUTF8toUTF16(name8,name16,sFormatBuffer);
    handle = INVALID_HANDLE_VALUE;

    if(master)
      handle = CreateEventW(NULL,0,0,LPCWSTR(name16));
    else
      handle = OpenEventW(SYNCHRONIZE|EVENT_MODIFY_STATE,0,LPCWSTR(name16));

    if(handle==INVALID_HANDLE_VALUE)
      goto fail;
    MasterToSlave.Add(handle);

    // slave to master

    name8.PrintF("Local\\%s_stom_%d",name,i);
    sUTF8toUTF16(name8,name16,sFormatBuffer);
    handle = INVALID_HANDLE_VALUE;

    if(master)
      handle = CreateEventW(NULL,0,0,LPCWSTR(name16));
    else
      handle = OpenEventW(SYNCHRONIZE|EVENT_MODIFY_STATE,0,LPCWSTR(name16));

    if(handle==INVALID_HANDLE_VALUE)
      goto fail;
    SlaveToMaster.Add(handle);
  }

  // done

  Initialized = 1;
  return Memory;

fail:
  Exit();
  return 0;
}

void sSharedMemory::Exit()
{
  if(FileMapping!=INVALID_HANDLE_VALUE)
    CloseHandle(FileMapping);

  if(Memory)
    UnmapViewOfFile(Memory);
  for(auto &handle : MasterToSlave)
    CloseHandle(handle);
  for(auto &handle : SlaveToMaster)
    CloseHandle(handle);
  for(auto &handle : SectionMutex)
    CloseHandle(handle);

  MasterToSlave.Clear();
  SlaveToMaster.Clear();
  SectionMutex.Clear();
  FileMapping = INVALID_HANDLE_VALUE;
  Memory = 0;
  Initialized = 0;
}

void sSharedMemory::Lock(int section)
{
  WaitForSingleObject(SectionMutex[section],INFINITE);
}

void sSharedMemory::Unlock(int section)
{
  ReleaseMutex(SectionMutex[section]);
}

void sSharedMemory::Signal(int section)
{
  if(Master)
    SetEvent(MasterToSlave[section]);
  else
    SetEvent(SlaveToMaster[section]);
}

void sSharedMemory::Wait(int section,int timeout)
{
  if(Master)
    WaitForSingleObject(SlaveToMaster[section],timeout==-1 ? INFINITE : timeout);
  else
    WaitForSingleObject(MasterToSlave[section],timeout==-1 ? INFINITE : timeout);
}

/****************************************************************************/
/***                                                                      ***/
/***   Audio                                                              ***/
/***                                                                      ***/
/****************************************************************************/

// Room for improvements
// * do not always render to all available space to reduce latency
// * use the WASAPI event system 

static void sAudioCode()
{
  while(AudioThread->CheckTerminate())
  {
//    AudioEvent->Wait(AOTimeout);
    sSleepMs(AOTimeout);
    if(0)
    {
      static int last = 0;
      int time = sGetTimeMS();
      sDPrintF("%d (%d)\n",time,time-last);
      last = time;
    }
    AudioLock->Lock();
    if(AOEnable)
    {
      HRESULT hr;

      uint pad;
      hr = AOClient->GetCurrentPadding(&pad);
      if(SUCCEEDED(hr))
      {
        uint chunk = AOBufferSize - pad;
        sASSERT(chunk>=0);
        float *data;
        hr = AORender->GetBuffer(chunk,(BYTE **) &data);
        if(SUCCEEDED(hr))
        {
          AOHandler.Call(data,chunk);
          hr = AORender->ReleaseBuffer(chunk,0);
        }
        AOTotalSamples += chunk;
      }
    }
    AudioLock->Unlock();
  }
}

int sStartAudioOut(int freq,const sDelegate2<void,float *,int> &handler,int flags)
{
  IMMDeviceEnumerator *AOEnum = 0;

  WAVEFORMATEXTENSIBLE fmt;
  WAVEFORMATEXTENSIBLE *mixform = 0;
 
  AOHandler = handler;

  int lat = 0;
  int chan = 0;
  int bits = 32;
  uint mask = 0;

  switch(flags & sAOF_LatencyMask)
  {
  case sAOF_LatencyHigh:  lat = 500; break;
  case sAOF_LatencyMed:   lat = 150; break;
  case sAOF_LatencyLow:   lat = 0; break;
  default:                sASSERT0(); break;
  }

  switch(flags & sAOF_FormatMask)
  {
  case sAOF_FormatStereo:  chan = 2; mask = KSAUDIO_SPEAKER_STEREO; break;
  case sAOF_Format5Point1: chan = 6; mask = KSAUDIO_SPEAKER_5POINT1; break;
  default:                 sASSERT0(); break;
  }

  // disable if already enabled...

  if(AOEnable)
    sStopAudioOut();

  // start audio

  HRESULT hr;
   
  if(FAILED(hr = CoCreateInstance(__uuidof(MMDeviceEnumerator),0,CLSCTX_ALL,__uuidof(IMMDeviceEnumerator),(void **)&AOEnum)))
    goto error;

  if(FAILED(hr = AOEnum->GetDefaultAudioEndpoint(eRender,eConsole,&AODev)))
    goto error;

  if(FAILED(hr = AODev->Activate(__uuidof(IAudioClient),CLSCTX_ALL,0,(void **)&AOClient)))
    goto error;

  AOClient->GetMixFormat((WAVEFORMATEX **)&mixform);
  freq = mixform->Format.nSamplesPerSec;
  CoTaskMemFree(mixform);

  sClear(fmt);
  fmt.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
  fmt.Format.nChannels = chan;
  fmt.Format.nSamplesPerSec = freq;
  fmt.Format.nAvgBytesPerSec = freq*chan*bits/8;
  fmt.Format.nBlockAlign = chan*bits/8;
  fmt.Format.wBitsPerSample = bits;
  fmt.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
  fmt.Samples.wReserved = bits;
  fmt.dwChannelMask = mask;
  fmt.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;


  if(FAILED(AOClient->Initialize(AUDCLNT_SHAREMODE_SHARED,0,10000*lat,0,&fmt.Format,0)))
    goto error;

  if(FAILED(AOClient->GetBufferSize(&AOBufferSize)))
    goto error;

  if(FAILED(AOClient->GetService(__uuidof(IAudioRenderClient),(void **)&AORender)))
    goto error;

  float *data;
  if(FAILED(AORender->GetBuffer(AOBufferSize,(BYTE **)&data)))
    goto error;

  sSetMem(data,0,(chan*bits/8)*AOBufferSize);

  if(FAILED(AORender->ReleaseBuffer(AOBufferSize,0)))
    goto error;

  if(FAILED(AOClient->Start()))
    goto error;

  // prepare thread

  AOTimeout = int(500.0f/(freq)*AOBufferSize);
  AudioLock = new sThreadLock();
//  AudioEvent = new sThreadEvent();
  AudioThread = new sThread(sDelegate1<void,sThread *>(sAudioCode));

  sLogF("sys","audio out started, ringbuffer %d samples, %f ms, timeout %d ms,%d hz",AOBufferSize,1000.0f/(freq)*AOBufferSize,AOTimeout,freq);
  AudioLock->Lock();
  AOEnable = 1;
  AudioLock->Unlock();

error:

  if(!AOEnable)
  {
    sRelease(AORender);
    sRelease(AOClient);
    sRelease(AODev);
  }
  sRelease(AOEnum);

  return AOEnable ? freq : 0;
}

void sUpdateAudioOut(int flags)
{
}

void sStopAudioOut()
{
  if(AOEnable)
  {
    sLogF("sys","audio out stoped");
    AOClient->Stop();
    AudioLock->Lock();
    AOEnable = 0;
    AudioLock->Unlock();
    delete AudioThread;
//    delete AudioEvent;
    delete AudioLock;
    sRelease(AORender);
    sRelease(AOClient);
    sRelease(AODev);
  }
}

int64 sGetSamplesOut()
{
//    AudioLock->Lock();
    UINT32 padding = 0;
    AOClient->GetCurrentPadding(&padding);
//    AudioLock->Unlock();
    return AOTotalSamples - padding;
}


int sGetSamplesQueuedOut()
{
//    AudioLock->Lock();
    UINT32 padding = 0;
    AOClient->GetCurrentPadding(&padding);
//    AudioLock->Unlock();
    return padding;
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

}

/****************************************************************************/
