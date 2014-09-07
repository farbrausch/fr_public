/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Base/SystemPrivate.hpp"

#if sConfigPlatform == sConfigPlatformWin
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

using namespace Altona2;

namespace Altona2 {

uint sQuietNan_ = 0xffffffff;

/****************************************************************************/

sSubsystem *sSubsystem::First = 0;
int sSubsystem::Runlevel = 0;

sSubsystem::sSubsystem(const char *name,int pri)
{
    Name = 0;
    Priority = 0;
    Next = 0;
    Register(name,pri);
}

sSubsystem::sSubsystem()
{
    Name = 0;
    Priority = 0;
    Next = 0;
}

sSubsystem::~sSubsystem()
{
}
void sSubsystem::Register(const char *name,int pri)
{
    if(Name==0)
    {
        Name = name;
        Priority = pri;
        Next = First;
        First = this;
        sASSERT(pri>0);

        if(Runlevel>=Priority)
        {
            sLogF("sys","init %02x: %s",pri,name);
            Init();
        }
    }
}

void sSubsystem::Init()
{
}

void sSubsystem::Exit()
{
}

/****************************************************************************/

void sSubsystem::SetRunlevel(int newlevel)
{
    while(Runlevel<newlevel)
    {
        Runlevel++;
        sSubsystem *s = First;
        while(s)
        {
            if(s->Priority==Runlevel)
            {
                sLogF("sys","init %02x: %s",Runlevel,s->Name);
                s->Init();
            }
            s = s->Next;
        }
    }
    while(Runlevel>newlevel)
    {
        sSubsystem *s = First;
        while(s)
        {
            if(s->Priority==Runlevel)
            {
                sLogF("sys","exit %02x: %s",Runlevel,s->Name);
                s->Exit();
            }
            s = s->Next;
        }
        Runlevel--;
    }
}

/****************************************************************************/
/****************************************************************************/

static int sExitCode = 0;
extern "C"
{
const char *sCmdLine = 0;
};

/****************************************************************************/

void sAltonaInit()
{
    sDPrint("/****************************************************************************/\n");
    sSubsystem::SetRunlevel(0x7f);
    sDPrint("/****************************************************************************/\n");
}
    
int sAltonaExit()
{
#if sConfigShell && sConfigDebug
    sPrint("*** press any key ***\n");
    getchar();
#endif
    sGC->CollectNow();
    sDPrint("/****************************************************************************/\n");
    sSubsystem::SetRunlevel(0x00);
    sDPrint("/****************************************************************************/\n");

    Private::ExitGfx();

    sLog("sys","Altona2 is done");
    return sExitCode;
}

int sAltonaMain()
{
	sAltonaInit();
    sLog("sys","calling Main()");
    Main();
    sLog("sys","Main() returned");
    return sAltonaExit();
}
    

/****************************************************************************/

}

#if sConfigPlatform == sConfigPlatformWin
#if sConfigShell

int main()
{
  return WinMain(GetModuleHandle(0),0,GetCommandLineA(),0);
} 

#endif

INT WINAPI WinMain(HINSTANCE inst,HINSTANCE,LPSTR,INT)
{
    LPWSTR cmd = GetCommandLineW();
    if(cmd)
    {
        sCmdLine = sUTF16toUTF8((const uint16 *)cmd);
    }
    else
    {
        char *cmd =new char[1];
        cmd[0] = 0;
        sCmdLine = cmd; 
    }

    int result = sAltonaMain();

    delete[] sCmdLine;

    return result;
}

#endif

#if sConfigPlatform == sConfigPlatformLinux

int main(int argc,const char *argv[])
{
    sTextLog log;
    for(int i=0;i<argc;i++)
    {
        if(i>0)
            log.Print(" ");
        log.Print(argv[i]);
    }
    sCmdLine = log.Get();

    return sAltonaMain();
}

#endif

#if ((sConfigPlatform == sConfigPlatformOSX) && sConfigShell)

int main(int argc,const char *argv[])
{
    //Private::sArgC = argc;
    //Private::sArgV = argv;

    sTextLog log;
    for(int i=0;i<argc;i++)
    {
        if(i>0)
            log.Print(" ");
        log.Print(argv[i]);
    }
    sCmdLine = log.Get();

    bool result = sAltonaMain();

    return result;
}

#endif

#if sConfigPlatform == sConfigPlatformAndroid

extern "C"
{
#include <android_native_app_glue.h>

struct android_app* gState = 0;
struct AAssetManager* gAssetManager = 0;

void android_main(struct android_app* state) 
{  
    app_dummy();
    gState = state;
	gAssetManager = gState->activity->assetManager;

    Altona2::sDPrint("enter sAltonaMain");
    sAltonaMain();

    while (1) 
    {
        int ident;
        int events;
        struct android_poll_source* source;
        while ((ident=ALooper_pollAll(0, NULL, &events, (void**)&source)) >= 0) 
        {
            //Altona2::sDPrint("HelloWorld");
            if (source != NULL) 
            {
                source->process(gState, source);
            }
            //Render();
        }
    }

    Altona2::sDPrint("exit sAltonaMain");
}

}

#endif


/****************************************************************************/

namespace Altona2 {

/****************************************************************************/

void sSetExitCode(int c)
{
    sExitCode = c;
}

/****************************************************************************/

const char *sGetCommandline()
{
    return sCmdLine; 
}

/****************************************************************************/

void sWaitForKey()
{
#if sConfigShell && !sConfigDebug
    sPrint("*** press any key ***\n");
    getchar();
#endif
}

/****************************************************************************/
/****************************************************************************/

#if sConfigPlatform == sConfigPlatformWin

class sConsoleSubsystem : public sSubsystem
{
private:
    HANDLE WConOut;
    HANDLE WConIn;
    bool OutToFile;
public:
    sConsoleSubsystem() : sSubsystem("console",0x01) {}

    void Init()
    {
        WConOut = GetStdHandle(STD_OUTPUT_HANDLE);
        WConIn  = GetStdHandle(STD_INPUT_HANDLE);

        DWORD mode;
        OutToFile = !GetConsoleMode(WConOut,&mode);
        sLogF("sys","console is %s",OutToFile ? "file" : "terminal");
    }

    void Exit()
    {
        CloseHandle(WConOut);
        CloseHandle(WConIn);
    }

    void SetPrintColor(uint col)
    {
    }

    void Print(const char *s)
    {
        sDPrint(s);
        if(OutToFile)
        {
            char buffer[2048];
            DWORD dummy;
            do
            {
                int n = 0;
                while(n<sCOUNTOF(buffer)-2 && *s)
                {
                    int val = sReadUTF8(s);
                    if(val<0x100)
                        buffer[n] = val;
                    else 
                        buffer[n] = '?';
                    n++;
                }
                WriteFile(WConOut,buffer,n,&dummy,0);
            }
            while(*s);
        }
        else
        {
            uint16 buffer[2048];
            bool done;
            DWORD dummy;
            do
            {
                uptr n;
                done = sUTF8toUTF16Scan(s,buffer,sCOUNTOF(buffer),&n);
                WriteConsoleW(WConOut,buffer,uint(n),&dummy,0);
            }
            while(!done);
        }
    }

    void WaitForKey()
    {
    }

} sConsoleSubsystem_;

void sSetPrintColor(uint col)
{
    sConsoleSubsystem_.SetPrintColor(col);
}

void sPrint(const char *s)
{
    sConsoleSubsystem_.Print(s);
}

#else

void sSetPrintColor(uint col)
{

}

#if sConfigPlatform == sConfigPlatformAndroid

void sPrint(const char *s)
{
    sDPrint(s);
    //  fputs(s,stdout);
}


#else
void sPrint(const char *s)
{
    fputs(s,stdout);
}
#endif


#endif


/****************************************************************************/
/****************************************************************************/

#if sConfigLogging
#if sConfigPlatform == sConfigPlatformWin

void sDPrint(const char *s)
{
    uint16 buffer[2048];
    bool done;
    do
    {
        done = sUTF8toUTF16Scan(s,buffer,sCOUNTOF(buffer));
        OutputDebugStringW((LPCWSTR) buffer);
    }
    while(!done);
}

#elif sConfigPlatform == sConfigPlatformAndroid

#include <errno.h>

#include <android/log.h>
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "altona2", __VA_ARGS__))

void sDPrint(const char *s)
{
    LOGI(s);
}


#else

void sDPrint(const char *s)
{
    fputs(s,stderr);
}

#endif

void sLog(const char *system,const char *msg)
{
    sDPrintF("[%s]%_%s\n",system,10-sGetStringLen(system),msg);
}

#endif

void sFatal(const char *s)
{
    sPrint("fatal error:");
    sPrint(s);
    sPrint("\n");
#if sConfigPlatform == sConfigPlatformWin
    sDPrint("fatal error:");
    sDPrint(s);
    sDPrint("\n");
#endif
#if sConfigDebug
    sDebugBreak();
#else
#if sConfigPlatform == sConfigPlatformWin
    sSystemMessageBox("fatal error",s);
#endif
#endif

    exit(1);
}

void sAssertImpl(const char *file,int line)
{
    sFatalF("%s(%d): assertion\n",file,line);
}

/****************************************************************************/
/***                                                                      ***/
/***   Float Control                                                      ***/
/***                                                                      ***/
/****************************************************************************/

#if sConfigPlatform == sConfigPlatformWin

void sFpuControl(int input)
{
    uint mask = _MCW_DN | _MCW_EM;
    uint flags = _EM_OVERFLOW | _EM_UNDERFLOW;

    if(input & sFC_DenormalDisable)
        flags |= _DN_FLUSH;
    if(!(input & sFC_DenormalException))
        flags |= _EM_DENORMAL;
    if(!(input & sFC_GeneralException))
        flags |= _EM_INVALID | _EM_ZERODIVIDE;
    if(!(input & sFC_InexcactException))
        flags |= _EM_INEXACT;

    switch(flags & sFC_PrecisionMask)
    {
    case sFC_DefaultPrecision:
        break;
    case sFC_SinglePrecision:
        mask |= _MCW_PC;
        flags |= _PC_24;
        break;
    case sFC_DoublePrecision:
        mask |= _MCW_PC;
        flags |= _PC_53;
        break;
    case sFC_ExtendedPrecision:
        mask |= _MCW_PC;
        flags |= _PC_64;
        break;
    }

    uint old;
    _clearfp();
    _controlfp_s(&old,flags,mask);
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Leaktracking                                                       ***/
/***                                                                      ***/
/****************************************************************************/


void *sAllocMemSystem(uptr size,int align,int flags);
void sFreeMemSystem(void *ptr);
uptr sMemSizeSystem(void *ptr);

namespace Leaks {


struct Leak
{
    const char *File;
    void *Ptr;
    int Line;
    uptr Size;
    int Id;
    int Count;
    Leak *Next;
};
struct LeakBlock
{
    Leak Nodes[32];
    LeakBlock *Next;
};

const int HashSize = 4096;
Leak *Hash[HashSize];
Leak *Free = 0;
LeakBlock *FirstBlock = 0;

int Active = 0;
int LeakCount = 0;
sThreadLock *MemLeakLock = 0;


void Exit();
void Init();

void Init()
{
    sASSERT(Active==0);
    Active = 2;
    MemLeakLock = new sThreadLock();
    Free = 0;
    LeakCount = 0;
    sClear(Hash);
    FirstBlock = 0;
    atexit(Exit);
    Active = 1;
    sLog("sys","leak tracking activated");
}

void Add(void *ptr,uptr size,const char *file,int line,int id)
{
    if(Active==0) Init();
    if(Active==2) return;

    MemLeakLock->Lock();

    // are there free nodes?

    if(!Free)
    {
        LeakBlock *b = (LeakBlock *) sAllocMemSystem(sizeof(LeakBlock),4,0);
        b->Next = FirstBlock;
        FirstBlock = b;

        for(int i=0;i<sCOUNTOF(b->Nodes);i++)
        {
            b->Nodes[i].Next = Free;
            Free = &b->Nodes[i];
        }
    }

    // build node

    Leak *n = Free;
    Free = n->Next;
    n->Ptr = ptr;
    n->Size = size;
    n->File = file;
    n->Line = line;
    n->Count = 1;
    n->Id = id;

    // insert node

    uint hash = sChecksumCRC32((const uint8 *)&ptr,sizeof(void *)) & (HashSize-1);
    n->Next = Hash[hash];
    Hash[hash] = n;
    LeakCount++;

    MemLeakLock->Unlock();
}

void Rem(void *ptr)
{
    if(Active==0) Init();
    if(Active==2) return;

    // find node

    uint hash = sChecksumCRC32((const uint8 *)&ptr,sizeof(void *)) & (HashSize-1);

    Leak **np;
    Leak *n;
    MemLeakLock->Lock();
    np = &Hash[hash];
    for(;;)
    {
        n = *np;

        if(!n)
        {
            sFatal("tried to free() memory that was not allocated!");
        }


        if(n->Ptr == ptr)
        {
            // remove from list
            *np = n->Next;
            LeakCount--;

            // add to free list
            n->Next = Free;
            Free = n;

            // done
            break;
        }

        np = &n->Next;
    }
    MemLeakLock->Unlock();
}

struct cmp
{ 
    inline bool operator()(const Leak &a,const Leak &b) const 
    { int r=sCmpString(a.File,b.File);  if(r!=0) return r<0;  if(a.Line!=b.Line) return a.Line<b.Line; return a.Size<b.Size; }
};
struct cmp2
{ 
    inline bool operator()(const Leak &a,const Leak &b) const 
    { if(a.Size!=b.Size) return a.Size>b.Size; int r=sCmpString(a.File,b.File); if(r!=0) return r<0; return a.Line<b.Line; }
};

bool DumpMemory()
{
    // whats up?

    if(Active==0) return 0;
    if(Active==2) return 0;
    if(LeakCount==0)
    {
        sLog("sys","no memory leaks detected\n\n");
        return 1;
    }
    if(LeakCount<0)
    {
        sDPrint("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        sDPrint("negative number of leaks - this can't be\n");
        return 0;
    }

    // make linear array of all leaks

    sArray<Leak> Leaks;
    int count = LeakCount;
    Leak *data = (Leak *) sAllocMemSystem(count*sizeof(Leak),4,0);
    Leaks.OverrideStorage(data,count);
    for(int i=0;i<HashSize;i++)
    {
        Leak *n = Hash[i];
        while(n)
        {
            Leaks.Add(*n);
            n = n->Next;
        }
    }
    sASSERT(count==Leaks.GetCount());
    Leaks.QuickSort(cmp());

    // merge

    {
        sptr count = Leaks.GetCount();
        Leak *data = Leaks.GetData();
        int d = 0;
        int s = 0;
        while(s<count)
        {
            if(d>0 && data[d-1].Line==data[s].Line && sCmpString(data[d-1].File,data[s].File)==0)
            {
                data[d-1].Count++;
                data[d-1].Size += data[s].Size;
                data[d-1].Id = sMin(data[d].Id,data[s].Id);
                s++;
            }
            else
            {
                data[d++] = data[s++];
            }
        }
        Leaks.SetSize(d);
    }

    // sort for output

    Leaks.QuickSort(cmp2());

    // output (combined)

    sDPrint("/****************************************************************************/\n");
    sDPrint("/***                                                                      ***/\n");
    sDPrint("/***   memory leaks!                                                      ***/\n");
    sDPrint("/***                                                                      ***/\n");
    sDPrint("/****************************************************************************/\n");
    sDPrint("\n");
    sDPrintF("file(line):                                                   size count    id\n");
    sDPrint("\n");
    for(auto &n : Leaks)
    {
        sString<256> buffer;
        buffer.PrintF("%s(%d):",n.File,n.Line);
        sDPrintF("%-60s%6K %5d %5d\n",buffer,n.Size,n.Count,n.Id);
    }
    sDPrint("\n");
    sDPrint("/****************************************************************************/\n\n");

    // done

    Leaks.OverrideStorage(0,0);

    sFreeMemSystem(data);
    return 0;
}

void Exit()
{
    sLog("sys","leak tracking deactivated");
    sASSERT(Active==1);
    sDumpMemory();
    Active=2;
    LeakBlock *b = FirstBlock;
    LeakBlock *n;
    sDelete(MemLeakLock);

    while(b)
    {
        n = b->Next;
        sFreeMemSystem(b);
        b = n;
    }
}

}; // namespace

void sInitLeak();
void sAddLeak(void *ptr,uptr size);
void sRemLeak(void *ptr);
bool sDumpMemory()
{
    return Leaks::DumpMemory();
}


/****************************************************************************/
/***                                                                      ***/
/***   Basic Alloc / Free                                                 ***/
/***                                                                      ***/
/****************************************************************************/

static uptr sMemoryUsed = 0;
static int sAllocId = 1;
static int sBreakAllocId = 0;

void *sAllocMem_(uptr size,int align,int flags)
{
    if(size==0) return 0;
    if(sAllocId==sBreakAllocId)
        sDebugBreak();
    sAllocId++;
    void *mem = sAllocMemSystem(size,align,flags);
    if(mem==0) return 0;
    uptr msize = sMemSizeSystem(mem);
    sAtomicAdd(&sMemoryUsed, msize);
    if(sConfigDebugMem)
        Leaks::Add(mem,size,"unknown",0,sAllocId-1);
    return mem;
}

void *sAllocMem_(uptr size,int align,int flags,const char *file,int line)
{
    if(size==0) return 0;
    if(sAllocId==sBreakAllocId)
        sDebugBreak();
    sAllocId++;
    void *mem = sAllocMemSystem(size,align,flags);
    if(mem==0) return 0;
    uptr msize = sMemSizeSystem(mem);
    sAtomicAdd(&sMemoryUsed, msize);
    if(sConfigDebugMem)
        Leaks::Add(mem,size,file,line,sAllocId-1);
    return mem;
}

void sFreeMem(void *ptr)
{
    if(ptr)
    {
        if(sConfigDebugMem)
            Leaks::Rem(ptr);
        uptr msize = sMemSizeSystem(ptr);
        sAtomicAdd(&sMemoryUsed,0-msize);
        sFreeMemSystem(ptr);
    }
}

uptr sMemSize(void *ptr)
{
    if(ptr)
        return sMemSizeSystem(ptr);
    else
        return 0;
}

uptr sGetTotalMemUsed()
{
    return sMemoryUsed;
}

void sBreakOnAllocId(int id)
{
    sBreakAllocId = id;
}

int sGetAllocId()
{
    return sAllocId;
}

/****************************************************************************/
/****************************************************************************/

void sMoveMem(void *d,const void *s,uptr c)
{
    memmove(d,s,c);
}

/****************************************************************************/

}

/****************************************************************************/

