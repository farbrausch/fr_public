/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"

#if sConfigPlatform==sConfigPlatformLinux || sConfigPlatform==sConfigPlatformOSX || sConfigPlatform==sConfigPlatformIOS || sConfigPlatform==sConfigPlatformAndroid

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***   Root filehandler                                                   ***/
/***                                                                      ***/
/****************************************************************************/

class sRootFile : public sFile
{
  FILE *File;
  sFileAccess Access;
  bool IsWrite;
  bool IsRead;

  bool Ok;
  int64 Size;
  int64 Offset;
public:
  sRootFile(FILE *file,sFileAccess access)
  {
    File = file;
    Access = access;
    Ok = 1;
    IsWrite = Access==sFA_Write || Access==sFA_WriteAppend || Access==sFA_ReadWrite;
    IsRead = Access==sFA_Read || Access==sFA_ReadRandom || Access==sFA_ReadWrite;
    Offset = 0;

    if(fseek(File,0,SEEK_END))
      Ok = 0;
    Size = ftello(File);
    if(Size==-1)
    {
      Ok = 0;
      Size = 0;
    }
    if(Access==sFA_WriteAppend)
      Offset = Size;
    else
      fseek(File,0,SEEK_SET);
  }

  ~sRootFile()
  {
    Close();
  }

  bool Close()
  {
    if(File)
    {
      if(fclose(File))
        Ok = 0;
      File = 0;
    }
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
      uptr read = fread(data,size,1,File);
      if(read!=1) Ok = 0;
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
      uptr write = fwrite(data,size,1,File);
      if(write!=1) Ok = 0;
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
#if sConfigPlatform == sConfigPlatformOSX || sConfigPlatform == sConfigPlatformIOS
    if(fseek(File,int(offset),SEEK_SET)==-1)
      Ok = 0;
#else
    if(fseek(File,offset,SEEK_SET)==-1)
      Ok = 0;
#endif
    
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
    static const char *mode[] =  {  0,"read","readrandom","write","writeappend","readwrite"  };
    static const char *umode[] =  {  0,"rb","rb","wb","ab","rb+"  };
    sASSERT(access>=sFA_Read && access<=sFA_ReadWrite);

    FILE *file = fopen(name,umode[access]);
    
    if(file)
    {
      sLogF("file","open file <%s> for %s",name,mode[access]);
      sFile *f = new sRootFile(file,access);
      return f;
    }
    else 
    {
      sLogF("file","failed to open file <%s> for %s",name,mode[access]);
      return 0;
    }
  }

  bool Exists(const char *name)
  {
    FILE *file = fopen(name,"rb");
    if(file)
      fclose(file);
    return file!=0;
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
  void *ptr = 0;
//  if(posix_memalign(&ptr,size,align))
  ptr = malloc(size);
  if(!ptr)
    sFatalF("out of mem %d %d",size,align);
  if(ptr)
    sSetMem(ptr,0x11,size);
  return ptr;
}

void sFreeMemSystem(void *ptr)
{
  free(ptr);
}

uptr sMemSizeSystem(void *ptr)
{
  return (uptr) 1;//msize(ptr,0,0);
}

/****************************************************************************/
/***                                                                      ***/
/***   Shell                                                              ***/
/***                                                                      ***/
/****************************************************************************/

bool sExecuteShell(const char *cmdline,const char *dir)
{
  sASSERT(dir==0);
  sLogF("file","Execute <%s>",cmdline);
  if(system(cmdline)!=0)
  {
    sLogF("file","Execute failed");
    return 0;
  }
  return 1;
}

bool sExecuteShellDetached(const char *cmdline,const char *dir)
{
  sASSERT(dir==0);
  sLogF("file","Execute detached <%s>",cmdline);
  if(fork()==0)
  {
    int ignore = system(cmdline);
    _exit(127+ignore*0);
  }
  return 1;
}

/*
bool sExecuteOpen(const char *file)
{
  sLogF("sys","shell open <%s>",file);

  uint16 buffer[sMaxPath];
  if(!sUTF8toUTF16(file,buffer,sCOUNTOF(buffer),0))
    sFatal("sExecuteOpen: string too long");
   
  return uptr(ShellExecuteW(0,L"open",(LPCWSTR)buffer,0,L"",SW_SHOW))>=32;
}


// warning!
// this is exactly as the sample implementation from microsoft
// "Creating a Child Process with Redirected Input and Output"
// the slightest change will break the code in mysterious ways!

bool sExecuteShell(const char *cmdline,sTextLog *tb)
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
      tb->Add(uni,bytesread);
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
*/

    
/****************************************************************************/
/***                                                                      ***/
/***   System Specials                                                    ***/
/***                                                                      ***/
/****************************************************************************/

bool sGetSystemPath(sSystemPath sp,const sStringDesc &desc)
{
    switch(sp)
    {
    case sSP_User:
        sCopyString(desc,"~");
        return true;
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
  sFile *sf = sOpenFile(source,sFA_Read);
  sFile *df = sOpenFile(dest,sFA_Write);
  bool ok = 0;
  if(sf && df)
  {
    uptr size = uptr(sf->GetSize());
    uptr block = 1024*1024;
    uint8 *buffer = new uint8[block];
    
    while(size>0)
    {
      uptr chunk = sMin(block,size);
      sf->Read(buffer,chunk);
      df->Write(buffer,chunk);
      size -= chunk;
    }
  }
  if(sf) if(!sf->Close()) ok = 0;
  if(df) if(!df->Close()) ok = 0;
  
  if(!ok)
    sLogF("file","failed to copy file <%s> to <%s>",source,dest);
  else
    sLogF("file","copy file <%s> to <%s>",source,dest);
  return ok;
}

bool sRenameFile(const char *source,const char *dest, bool overwrite)
{
  if(rename(source,dest))
  {
    sLogF("file","failed rename file <%s> to <%s>",source,dest);
    return 0;;
  }
  else
  {
    sLogF("file","rename file <%s> to <%s>",source,dest);
    return 1;
  }
}

bool sDeleteFile(const char *name)
{
  if(remove(name))
  {
    sLogF("file","failed delete file <%s>",name);
    return 0;;
  }
  else
  {
    sLogF("file","delete file <%s>",name);
    return 1;
  }
}


bool sGetFileWriteProtect(const char *name,bool &prot)
{
  sASSERT0();
  prot = 0;
  return 0;
}

bool sSetFileWriteProtect(const char *name,bool prot)
{
  sASSERT0();
  return 0;
}

bool sGetFileInfo(const char *name,sDirEntry *de)
{
#if sConfigPlatform!=sConfigPlatformIOS
#if sConfigPlatform!=sConfigPlatformOSX
   	struct stat64 stats;
  	de->Flags = 0;
	de->Size = 0;
  	de->Name = name;
  	de->LastWriteTime = 0;
  	if(!stat64(name,&stats))
  	{
    	de->Flags |= sDEF_Exists;
    	if(S_ISDIR(stats.st_mode)) de->Flags |= sDEF_Dir;
    	de->Size = stats.st_size;
    	de->LastWriteTime = stats.st_mtime;
    	if(access(name,W_OK)!=0) de->Flags |= sDEF_WriteProtect;
    	return 1;
  	}
#else
   	struct stat stats;
  	de->Flags = 0;
	de->Size = 0;
  	de->Name = name;
  	de->LastWriteTime = 0;
  	if(!stat(name,&stats))
  	{
    	de->Flags |= sDEF_Exists;
    	if(S_ISDIR(stats.st_mode)) de->Flags |= sDEF_Dir;
    	de->Size = stats.st_size;
    	de->LastWriteTime = stats.st_mtime;
    	if(access(name,W_OK)!=0) de->Flags |= sDEF_WriteProtect;
    	return 1;
  	}
#endif
#endif
  return 0;
}

  /****************************************************************************/
  
bool sIsAbsolutePath(const char *file)
{
  return file[0]=='/';
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

bool sChangeDir(const char *name)
{
  if(chdir(name))
  {
    sLogF("file","failed change dir <%s>",name);
    return 0;;
  }
  else
  {
    sLogF("file","change dir <%s>",name);
    return 1;
  }
}

void sGetCurrentDir(const sStringDesc &str)
{
  if(getcwd(str.Buffer,str.Size)==0)
    sLogF("file","getcwd() failed");
}

/****************************************************************************/


bool sMakeDir(const char *name)
{
  if(mkdir(name,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
  {
    sLogF("file","failed make dir <%s>",name);
    return 0;;
  }
  else
  {
    sLogF("file","make dir <%s>",name);
    return 1;
  }
}

bool sDeleteDir(const char *name)
{
  if(rmdir(name))
  {
    sLogF("file","failed delete dir <%s>",name);
    return 0;;
  }
  else
  {
    sLogF("file","delete dir <%s>",name);
    return 1;
  }
}

bool sCheckDir(const char *name)
{
  DIR *dir;
  dir = opendir(name);
  if(dir)
    closedir(dir);
  return dir!=0;
}

/****************************************************************************/

bool sLoadDir(sArray<sDirEntry> &list,const char *path)
{
  DIR *dir;
  dirent *dp;
  sDirEntry *de;
  sString<sMaxPath> buffer;

  dir = opendir(path);
  if(!dir) 
  {
    sLogF("file","load dir failed <%s>",path);
    return 0;
  }
  else
  {
    sLogF("file","load dir <%s>",path);
  }

  bool ok = 1;
  list.Clear();  
  while(1)
  {
    dp = readdir(dir);
    if(!dp) 
      break;
    if(sCmpString(dp->d_name,".")==0 || sCmpString(dp->d_name,"..")==0)
      continue;
    buffer.PrintF("%s/%s",path,dp->d_name);
    de = list.AddMany(1);
    if(!sGetFileInfo(buffer,de))
      ok = 0;
    de->Name = dp->d_name;
  }
  
  closedir(dir);

  return ok;
}

bool sLoadVolumes(sArray<sDirEntry> &list)
{
    auto *de = list.AddMany(1);
    de->Flags = sDEF_Exists;
    de->Flags |= sDEF_Dir;
    de->Size = 0;
    de->LastWriteTime = 0;
    de->Name = "";      // the logic: the volume name is "", like "C:" on windows, and the "/" is appended to "" automatically :-)

    return true;
}

/****************************************************************************/
/***                                                                      ***/
/***   Time and Date                                                      ***/
/***                                                                      ***/
/****************************************************************************/

#if sConfigPlatform != sConfigPlatformOSX && sConfigPlatform != sConfigPlatformIOS

static int sStartSec = -1;
    
uint sGetTimeMS()
{
  timespec ts;
  clock_gettime(CLOCK_REALTIME,&ts);
  if(sStartSec==-1)
    sStartSec = ts.tv_sec;
  return (ts.tv_sec-sStartSec)*1000+ts.tv_nsec/1000000;
}

uint64 sGetTimeUS()
{
  timespec ts;
  clock_gettime(CLOCK_REALTIME,&ts);
  if(sStartSec==-1)
    sStartSec = ts.tv_sec;
  return (ts.tv_sec-sStartSec)*1000000UL+ts.tv_nsec/1000;
}
#endif

uint64 sGetTimeRandom()
{
  return 4;  // random number chosen by dice roll
}

/****************************************************************************/
/***                                                                      ***/
/***   Multithreading                                                     ***/
/***                                                                      ***/
/****************************************************************************/

sThreadLock::sThreadLock()
{
  lock = new pthread_mutex_t;
  if(pthread_mutex_init((pthread_mutex_t *)lock,0))
    sFatal("pthread_mutex_init failed");
}

sThreadLock::~sThreadLock()
{
  sASSERT(lock);
  if(pthread_mutex_destroy((pthread_mutex_t *)lock))
    sFatal("pthread_mutex_destroy failed");
  delete (pthread_mutex_t *) lock;
}

void sThreadLock::Lock()
{
  if(pthread_mutex_lock((pthread_mutex_t *)lock))
    sFatal("pthread_mutex_lock failed");
}

bool sThreadLock::TryLock()
{
  int err = pthread_mutex_lock((pthread_mutex_t *)lock);
  if(err==EBUSY)
    return 0;
  if(err)
    sFatal("pthread_mutex_lock failed");
  return 1;
}

void sThreadLock::Unlock()
{
  if(pthread_mutex_unlock((pthread_mutex_t *)lock))
    sFatal("pthread_mutex_unlock failed");
}

}

#else

int ALTONA2_LIBS_BASE_SYSTEM_POSIX_CPP_STUB;

#endif // linux || osx
