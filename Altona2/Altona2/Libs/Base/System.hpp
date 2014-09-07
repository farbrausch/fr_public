/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_BASE_SYSTEM_HPP
#define FILE_ALTONA2_LIBS_BASE_SYSTEM_HPP

#include "Altona2/Libs/Base/Machine.hpp"
#include "Altona2/Libs/Base/Types.hpp"
#include "Altona2/Libs/Base/String.hpp"
#include "Altona2/Libs/Base/Math.hpp"

#if sConfigPlatform==sConfigPlatformWin

#include "Altona2/Libs/Base/SystemWin.hpp"

#elif sConfigPlatform==sConfigPlatformLinux || sConfigPlatform==sConfigPlatformOSX || sConfigPlatform==sConfigPlatformIOS || sConfigPlatform==sConfigPlatformAndroid

#include "Altona2/Libs/Base/SystemPosix.hpp"

#else

namespace Altona2 
{
    class sThreadPrivate{};
    class sThreadLockPrivate{};
    class sThreadEventPrivate{};
}

#endif

namespace Altona2 {

class sScreen;
struct sScreenMode;

/****************************************************************************/
/***                                                                      ***/
/***   Low Level File Operation. Can usually be ignored.                  ***/
/***                                                                      ***/
/****************************************************************************/

enum sFileAccess
{
    sFA_Read = 1,                   // read mostly sequential
    sFA_ReadRandom,                 // read in random order (OS may optimize for this)
    sFA_Write,                      // create new file, possibly overwriting old data
    sFA_WriteAppend,                // append to existing file or create new file
    sFA_ReadWrite,                  // open file for read and write
};

enum sFilePriorityFlags
{
    sFP_BACKGROUND = 0,
    sFP_NORMAL = 1,
    sFP_REALTIME = 2,
};

typedef int sFileASyncHandle;

class sFile
{
public:
    virtual ~sFile()=0;                       // virtual destructor. implies Close()
    virtual bool Close()=0;                  // close manually to get error
    virtual bool SetSize(int64)=0;            // change size of file on disk (for write files)
    virtual int64 GetSize()=0;                 // get size

    // use only one of these interfaces!

    // synchronious interface

    virtual bool Read(void *data,uptr size)=0;        // read bytes. may change mapping.
    virtual bool Write(const void *data,uptr size)=0; // write bytes, may change mapping.
    virtual bool SetOffset(int64 offset)=0;   // seek to offset
    virtual int64 GetOffset()=0;               // get offset

    // file mapping. may be unimplemented, returning 0

    virtual uint8 *Map(int64 offset,uptr size)=0;  // map file 

    // asynchronous interface. may be unimplemented, returning 0

    virtual bool ReadAsync(int64 offset,uptr size,void *mem,sFileASyncHandle *hnd,sFilePriorityFlags prio=sFP_NORMAL)=0;  // begin reading
    virtual bool Write(int64 offset,uptr size,void *mem,sFileASyncHandle *hnd,sFilePriorityFlags prio=sFP_NORMAL)=0;  // begin writing
    virtual bool CheckAsync(sFileASyncHandle)=0;
    virtual bool EndAsync(sFileASyncHandle)=0;
};

/****************************************************************************/

class sFileHandler
{
public:
    virtual ~sFileHandler();
    virtual sFile *Create(const char *name,sFileAccess access)=0;
    virtual bool Exists(const char *name)=0;
};

void sAddFileHandler(sFileHandler *);
void sRemFileHandler(sFileHandler *);
sFileHandler *GetRootFileHandler();

void sRegFileLogCb(sDelegate2<void, const char *, int> Handler);

class sFile *sOpenFile(const char *name,sFileAccess access);
bool sCheckFile(const char *name);

/****************************************************************************/

class sLinkFileHandler : public sFileHandler
{
    const char *Source;
    const char *Dest;
    sString<sMaxPath> Buffer;
    bool BuildPath(const char *name);
public:
    sLinkFileHandler(const char *source,const char *dest=0);
    sFile *Create(const char *name,sFileAccess access);
    bool Exists(const char *name);
};

/****************************************************************************/
/***                                                                      ***/
/***   High Level File Operation. Use this if you can.                    ***/
/***                                                                      ***/
/****************************************************************************/

uint8 *sLoadFile(const char *name);
uint8 *sLoadFile(const char *name,uptr &size);
bool sSaveFile(const char *name,const void *data,uptr bytes);
bool sSaveFileIfDifferent(const char *name,const void *data,uptr bytes);

/****************************************************************************/

char *sLoadText(const char *name,bool forceutf8=0);

bool sSaveTextUTF8(const char *filename,const char *text);
bool sSaveTextAnsi(const char *filename,const char *text,bool doslf);
bool sSaveTextUTF8IfDifferent(const char *filename,const char *text);

/*
bool sSaveTextAnsi(const char *name,const char *data);
bool sSaveTextUnicode(const char *name,const char *data);
bool sSaveTextAnsiIfDifferent(const char *name,const char *data); // for ASC/makeproject etc.
*/

/****************************************************************************/
/***                                                                      ***/
/***   file and directory management                                      ***/
/***                                                                      ***/
/****************************************************************************/

struct sDirEntry
{
    sString<256> Name;
    int64 Size;
    int Flags;
    uint64 LastWriteTime;
};

enum sDirEntryFlags
{
    sDEF_Dir          = 0x0001,
    sDEF_WriteProtect = 0x0002,
    sDEF_Exists       = 0x0004,
};

bool sCopyFile(const char *source,const char *dest,bool failifexists=0);
bool sRenameFile(const char *source,const char *dest, bool overwrite=0);
bool sDeleteFile(const char *);
bool sGetFileWriteProtect(const char *,bool &prot);
bool sSetFileWriteProtect(const char *,bool prot);
bool sGetFileInfo(const char *name,sDirEntry *);

bool sChangeDir(const char *name);
void sGetCurrentDir(const sStringDesc &str);
bool sMakeDir(const char *);          // make one directory
bool sCheckDir(const char *);
bool sDeleteDir(const char *name);

bool sLoadDir(sArray<sDirEntry> &list,const char *path);
bool sLoadVolumes(sArray<sDirEntry> &list);

bool sIsAbsolutePath(const char *file);
void sMakeAbsolutePath(const sStringDesc &buffer,const char *file);

void sGetAppId(sString<sMaxPath> &udid);



/****************************************************************************/
/***                                                                      ***/
/***   dynamic link libraries                                             ***/
/***                                                                      ***/
/****************************************************************************/

class sDynamicLibrary
{
    uptr Handle;
public:
    sDynamicLibrary();
    ~sDynamicLibrary();
    bool Load(const char *filename);
    void *GetSymbol(const char *symbol);
};

/****************************************************************************/
/***                                                                      ***/
/***   Packfiles                                                          ***/
/***                                                                      ***/
/****************************************************************************/

// layout:
// * sPackfileHeader (automatically aligned)
// * sPackfileEntry[] (automatically aligned)
// * string table (aligned)
// * DataOffset: all the files (each one aligned)

// DataOffset <= 0x7fffffff: info part must fit into 2GB limit
// total packfile may exceed 2GB limit
// 32 bit packfile tool can not handle single files > 2GB

enum sPackfileEnums
{
    sPackfileAlign = 16,
    sPackfileMagic = ('p' | ('a'<<8) | ('c'<<16) | ('k'<<24)),
    sPackfileVersion = 1,
};

struct sPackfileHeader            // headerm 32 bytes
{
    uint Magic;                     // sPackfileMagic
    uint Version;                   // sPackfileVersion
    uint DirCount;                  // number of entries
    uint StringBytes;               // size of string table, aligned
    uint DataOffset;                // offset to first file, aligned
    uint pad[3];                    // so this automatically aligns.
};

struct sPackfileEntry             // entry in dir, 32 bytes
{
    uint64 Size;                      // size of unpacked data
    uint64 PackedSize;                // unused 
    uint64 Offset;                    // Relative to DataOffset
    uint Name;                      // relative to start of stringtable
    int Flags;                     // unused
};

sFileHandler *sAddPackfile(const char *name);

/****************************************************************************/
/***                                                                      ***/
/***   Shell                                                              ***/
/***                                                                      ***/
/****************************************************************************/

bool sExecuteOpen(const char *file);
bool sExecuteShell(const char *cmdline,const char *dir=0);
bool sExecuteShellDetached(const char *cmdline,const char *dir=0);
bool sExecuteShell(const char *cmdline,sTextLog &tb);

bool sAssemble(const char *asmfile,const char *objectfile);
bool sAssemble(const char *asmfile,const char *objectfile,int platform,bool bit64);

/****************************************************************************/
/***                                                                      ***/
/***   System Specials                                                    ***/
/***                                                                      ***/
/****************************************************************************/

void sSystemMessageBox(const char *title,const char *text);

enum sSystemPath
{
    sSP_User = 1,
};

bool sGetSystemPath(sSystemPath sp,const sStringDesc &desc);

/****************************************************************************/
/***                                                                      ***/
/***   Input                                                              ***/
/***                                                                      ***/
/****************************************************************************/

enum sMouseButtons
{
    sMB_Left            = 0x00000001,
    sMB_Right           = 0x00000002,
    sMB_Middle          = 0x00000004,
    sMB_Extra1          = 0x00000008,
    sMB_Extra2          = 0x00000010,
    sMB_Pen             = 0x00000020,       // wacom tablet pen
    sMB_PenF            = 0x00000040,       // pen while pressing front button
    sMB_PenB            = 0x00000080,       // pen while pressing back button
    sMB_Rubber          = 0x00000100,       // rubber side of the pen
    sMB_Mask            = 0x0000ffff,

    sMB_DoubleClick     = 0x00010000,
};

enum sDragMode
{
    sDM_Hover = 0,
    sDM_Start,
    sDM_Drag,
    sDM_Stop,
};

struct sTouchData                   // a single touch in a multi-touch environment
{
    sDragMode Mode;                 // sDM_???
    int PosX;                       // current position
    int PosY;
    int StartX;                     // where it started
    int StartY;
};

struct sDragData : public sTouchData     // mouse events. mouse is embedded
{
    sScreen *Screen;                // screen this came from
    int Timestamp;                  // in milliseconds
    int Buttons;                    // sMB_???
    int DeltaX;                     // PosXY - StartXY (no special handling)
    int DeltaY;
    float RawDeltaX;                // movement delta beyond screen coordinates, not pixel-precise with mouse movement.
    float RawDeltaY;

    // tablet data

    float StartXF;
    float StartYF;
    float PosXF;
    float PosYF;
    float Pressure;
    void FakeTablet();

    // touch data

    int TouchCount;                 // all touches. this repeats the embedded touch.
    sTouchData Touches[10];
};

struct sKeyData
{
    int Timestamp;                  // in milliseconds
    uint Key;                       // with qualifiers
    int MouseX;                     // mouse position when key was pressed
    int MouseY;
};

/****************************************************************************/

enum sKeyQualifiers
{
    sKEYQ_Mask        = 0x0000ffff,
    sKEYQ_ShiftL      = 0x00010000,
    sKEYQ_ShiftR      = 0x00020000,
    sKEYQ_CtrlL       = 0x00040000,
    sKEYQ_CtrlR       = 0x00080000,
    sKEYQ_Alt         = 0x00100000,
    sKEYQ_AltGr       = 0x00200000,
    sKEYQ_Caps        = 0x00400000,
    sKEYQ_Double      = 0x00800000,     // used only for mouse clicks
    sKEYQ_Meta        = 0x01000000,       // the windows key
    sKEYQ_Numeric     = 0x20000000,
    sKEYQ_Repeat      = 0x40000000,
    sKEYQ_Break       = 0x80000000,

    sKEYQ_Shift       = sKEYQ_ShiftL | sKEYQ_ShiftR,
    sKEYQ_Ctrl        = sKEYQ_CtrlL | sKEYQ_CtrlR,
};

enum sKeyCodes
{
    sKEY_Backspace    = 8,              // traditional ascii
    sKEY_Tab          = 9,
    sKEY_Enter        = 10,
    sKEY_Escape       = 27,
    sKEY_Space        = 32,

    sKEY_Up           = 0x0000e000,     // cursor block
    sKEY_Down         = 0x0000e001,
    sKEY_Left         = 0x0000e002,
    sKEY_Right        = 0x0000e003,
    sKEY_PageUp       = 0x0000e004,
    sKEY_PageDown     = 0x0000e005,
    sKEY_Home         = 0x0000e006,
    sKEY_End          = 0x0000e007,
    sKEY_Insert       = 0x0000e008,
    sKEY_Delete       = 0x0000e009,

    sKEY_Pause        = 0x0000e010,      // Pause/Break key
    sKEY_Scroll       = 0x0000e011,      // ScrollLock key
    sKEY_Numlock      = 0x0000e012,      // NumLock key
    sKEY_Print        = 0x0000e013,
    sKEY_WinL         = 0x0000e014,      // left windows key
    sKEY_WinR         = 0x0000e015,      // right windows key
    sKEY_WinM         = 0x0000e016,      // windows menu key
    sKEY_Shift        = 0x0000e017,      // a shift key was pressed
    sKEY_Ctrl         = 0x0000e018,      // a ctrl key was pressed
    sKEY_Caps         = 0x0000e019,      // caps lock key was pressed

    sKEY_F1           = 0x0000e020,      // function keys
    sKEY_F2           = 0x0000e021,
    sKEY_F3           = 0x0000e022,
    sKEY_F4           = 0x0000e023,
    sKEY_F5           = 0x0000e024,
    sKEY_F6           = 0x0000e025,
    sKEY_F7           = 0x0000e026,
    sKEY_F8           = 0x0000e027,
    sKEY_F9           = 0x0000e028,
    sKEY_F10          = 0x0000e029,
    sKEY_F11          = 0x0000e02a,
    sKEY_F12          = 0x0000e02b,

    // mouse. will also be trasmitted as keycodes

    sKEY_LMB          = 0x0000e100,
    sKEY_RMB          = 0x0000e101,
    sKEY_MMB          = 0x0000e102,
    sKEY_Extra1       = 0x0000e103,
    sKEY_Extra2       = 0x0000e104,

    sKEY_Pen          = 0x0000e105,       // wacom tablet pen
    sKEY_PenF         = 0x0000e106,       // pen while pressing front button
    sKEY_PenB         = 0x0000e107,       // pen while pressing back button
    sKEY_Rubber       = 0x0000e108,       // rubber side of the pen

    sKEY_WheelUp      = 0x0000e181,       // no break code
    sKEY_WheelDown    = 0x0000e182,       // no break code

    // special keys

    sKEY_Activated    = 0x0000e200,       // the application got keyboard focus
    sKEY_Deactivated  = 0x0000e201,       // the application lost keyboard focus
};

/****************************************************************************/
/***                                                                      ***/
/***   App                                                                ***/
/***                                                                      ***/
/****************************************************************************/

class sApp
{
public:
    sApp() {}
    virtual ~sApp() {}

    virtual void OnInit() {}
    virtual void OnExit() {}
    virtual void OnFrame() {}
    virtual void OnPaint() {}
    virtual void OnKey(const sKeyData &kd) {}
    virtual void OnDrag(const sDragData &dd) {}
    virtual bool OnClose() { return 1; }
};

void sRunApp(sApp *app,const Altona2::sScreenMode &sm);
void sUpdateScreen(const sRect &r);
uint sGetKeyQualifier();
void sGetMouse(int &x,int &y,int &b);
void sKeyName(const sStringDesc &buffer,uint key);
uint sCleanKeyCode(uint key);
void sExit();
bool sRegisterHotkey(uint key,sDelegate<void> &del);
void sUnregisterHotkey(uint key);
float *sGetGyroMatrix();

/****************************************************************************/
/***                                                                      ***/
/***   Time and Date                                                      ***/
/***                                                                      ***/
/****************************************************************************/

uint sGetTimeMS();
uint64 sGetTimeUS();
uint64 sGetTimeRandom();

#if sConfigCompiler == sConfigCompilerMsc
inline uint64 sGetTimeHR() { return __rdtsc(); }
#else
uint64 sGetTimeHR();
#endif
uint64 sGetHRFrequency();


struct sTimeAndDate
{
    int Year;                      // like 2001
    int Month;                     // 0..11
    int Day;                       // 0..30
    int Second;                    // 
};

void sGetTimeAndDate(sTimeAndDate &date);
double sGetTimeSince2001();

/****************************************************************************/
/***                                                                      ***/
/***   Clipboard                                                          ***/
/***                                                                      ***/
/****************************************************************************/

class sClipboardBase
{
public:
    virtual ~sClipboardBase() {}

    // set clipboard

    virtual void SetText(const char *,uptr len=-1) = 0;
    virtual void SetBlob(const void *data,uptr size,int serid) = 0;

    // get a copy of clipboard contents, please delete[] the returned pointer

    virtual char *GetText() = 0;
    virtual void *GetBlob(uptr &size,int &serid) = 0;
};

extern sClipboardBase *sClipboard;

/****************************************************************************/
/***                                                                      ***/
/***   Hardware Mouse Cursor                                              ***/
/***                                                                      ***/
/****************************************************************************/

enum sHardwareCursorImage
{
    sHCI_Off = 0,
    sHCI_Arrow,
    sHCI_Crosshair,
    sHCI_Hand,
    sHCI_Text,

    sHCI_No,
    sHCI_Wait,
    sHCI_SizeAll,
    sHCI_SizeH,
    sHCI_SizeV,

    sHCI_Max,
};

void sSetHardwareCursor(sHardwareCursorImage img);
void sEnableHardwareCursor(bool enable);

/****************************************************************************/
/***                                                                      ***/
/***   Ios Features                                                       ***/
/***                                                                      ***/
/****************************************************************************/

void sEnableVirtualKeyboard(bool enable);
float sGetDisplayScaleFactor();
const char *sGetDeviceName();
const char *sGetUserName();

/****************************************************************************/
/***                                                                      ***/
/***   Windows Features                                                   ***/
/***                                                                      ***/
/****************************************************************************/

bool sResolveWindowsLink(const char *filename,const sStringDesc &buffer);
void sInitWacom();

/****************************************************************************/
/***                                                                      ***/
/***   Render Font to Bitmap                                              ***/
/***                                                                      ***/
/****************************************************************************/

// this is private!!!
#if sConfigPlatform == sConfigPlatformWin

class sSystemFontInfoPrivate
{
    friend const class sSystemFontInfo *sGetSystemFont(enum sSystemFontId id);
    friend class sSystemFont *sLoadSystemFont(const class sSystemFontInfo *fi,int sx,int sy,int flags);
protected:
    sSystemFontInfoPrivate();
    ~sSystemFontInfoPrivate();
public:
    void *lf;
};

class sSystemFontPrivate
{
protected:
    struct sSystemFontPrivate_ *p;
    sSystemFontPrivate();
    ~sSystemFontPrivate();
};

#else

class sSystemFontInfoPrivate
{
};

class sSystemFontPrivate
{
};

#endif

/****************************************************************************/

class sOutline
{
public:
    struct Vertex;
    struct Segment;
private:
    sArray<Vertex *> ValidVertices;           // all vertices
    sArray<Segment *> ValidSegments;           // all segments to check for
    sArray<Segment *> Large;                  // all contours with more than 3 segments -> the stuff to triangulate
    bool CheckIntersection(sVector2 p0,sVector2 p1);
    int CountIntersection(sVector2 p0);
public:
    sOutline();
    ~sOutline();

    // segments inside a contour form a cyclic list, all contours are closed
    // the end vertex will alway be the same as the start vertex of the next segment
    struct Segment
    {
        Segment();

        Vertex *P0;                   // start vertex for this segment
        Vertex *P1;                   // end vertex for this segment
        Vertex *C0;                   // control point for quadrics, or -1
        Vertex *C1;                   // control point for cubics, or -1
        Segment *Next;                // next segment
        Segment *Prev;                // previous segment
        int Temp;                    // unused
    };

    // each start/end vertex is used exactly twice
    // each control vertex is used exactly once
    struct Vertex
    {
        Vertex();
        Vertex(float x,float y);

        sVector2 Pos;                 // actual position
        int Temp;                    // unused
        //    Segment *S0;                  // link to segment where this vertex is start
        //    Segment *S1;                  // link to segment where this vertex is end
    };

    // some vertices and segments might be unused, always iterate through contours
    sArray<Vertex *> Vertices;      // all vertices
    sArray<Segment *> Segments;     // all segments
    sArray<Segment *> Contours;     // for each contour, one segment
    Segment *AddSegment();
    void Assert();
    void Triangulate();
};

/****************************************************************************/

class sSystemFontInfo : public sSystemFontInfoPrivate
{
public:
    sSystemFontInfo();
    ~sSystemFontInfo();
    const char *Name;
    int Flags;
};

enum sSystemFontId
{
    sSFI_Console,                   // fixed with font (console)
    sSFI_Serif,                     // proportional font with serifs (times)
    sSFI_SansSerif,                 // proportional font without serifs (helvetica)
    sSFI_Complete,                  // font with most unicode characters
};

enum sSystemFontFlags
{
    // flags to modify font
    sSFF_Bold           = 0x000001,
    sSFF_Italics        = 0x000002,
    sSFF_Antialias      = 0x000004, // renders font up to 16 times larger to get antialiasing

    // flags with info
    sSFF_Proportional   = 0x000100,

    // flags used by sPainter from util
    sSFF_DistanceField  = 0x010000,
    sSFF_Multisample    = 0x020000,
};

struct sSystemFontKernPair
{
    int CodeA;
    int CodeB;
    int Kern;
};

class sSystemFont : public sSystemFontPrivate
{
    friend class sSystemFont *sLoadSystemFont(const class sSystemFontInfo *fi,int sx,int sy,int flags);
    int SizeX;		// size of the bitmap to use for rendering
    int SizeY;
    int AA;
    uint8 *Data;
protected:
    sSystemFont();
public:
    ~sSystemFont();
    int GetAdvance();
    int GetBaseline();
    int GetCellHeight();
    bool GetInfo(int code,int *abc);
    bool CodeExists(int code);
    void BeginRender(int sx,int sy,uint8 *data);
    void RenderChar(int code,int px,int py);
    void EndRender();
    sOutline *GetOutline(const char *text);

    sArray<sSystemFontKernPair> KernPairs;
};

const sSystemFontInfo *sGetSystemFont(sSystemFontId id);
bool sLoadSystemFontDir(sArray<const sSystemFontInfo *> &dir);
sSystemFont *sLoadSystemFont(const sSystemFontInfo *fi,int sx,int sy,int flags);

/****************************************************************************/
/***                                                                      ***/
/***   Multithreading                                                     ***/
/***                                                                      ***/
/****************************************************************************/

void sSleepMs(int ms);
int sGetCPUCount();

/****************************************************************************/

class sThread : public sThreadPrivate
{
    friend void sStartThread(sThread *);
    volatile bool TerminateFlag;
    sDelegate1<void,sThread *> Code;
public:
    sThread(const sDelegate1<void,sThread *> &code,int pri=0,int stacksize=0x4000, int flags=0);
    ~sThread();

    void Terminate();
    bool CheckTerminate();
};

/****************************************************************************/

class sThreadLock : public sThreadLockPrivate
{
public:
    sThreadLock();
    ~sThreadLock();

    void Lock();
    bool TryLock();
    void Unlock();
};

class sBlockLock
{
    sThreadLock &Lock;
public:
    sBlockLock(sThreadLock &lock) : Lock(lock) { Lock.Lock(); }
    ~sBlockLock() { Lock.Unlock(); }
};

/****************************************************************************/

class sThreadEvent : public sThreadEventPrivate
{
public:
    sThreadEvent(bool manual=0);    // with manual == 0 the signal gets deactivated be first waiting thread
    // use manual == sTRUE to disable event only by Reset
    // (can be usefull to synchronising multiple threads)
    ~sThreadEvent();

    bool Wait(int timeout=-1);    // 0 for immediate return, -1 for infinite wait
    void Signal();                  // the signal is consumed by the first thread for automatic events
    void Reset();                   // go into nonsignaled state
};

/****************************************************************************/
/***                                                                      ***/
/***   Inter Process Communication                                        ***/
/***                                                                      ***/
/****************************************************************************/

class sSharedMemory: public sSharedMemoryPrivate
{
public:
    sSharedMemory();
    ~sSharedMemory();
    void *Init(const char *name,uptr size,bool master,int sections);
    void Exit();
    void Lock(int section);
    void Unlock(int section);
    void Signal(int section);
    void Wait(int section,int timeout);
};

/****************************************************************************/
/***                                                                      ***/
/***   Audio                                                              ***/
/***                                                                      ***/
/****************************************************************************/

enum sAudioOutputFlags
{
    sAOF_Background       = 0x0001, // continue playing when minimized
    sAOF_AllowBackgroundMusic = 0x0002, // IOS: allow itune or other players to play music. Default: mute all other apps

    sAOF_LatencyMask      = 0x0f00,
    sAOF_LatencyHigh      = 0x0000, // no risk of dropouts, good for demos and audo players
    sAOF_LatencyMed       = 0x0100, // good for games
    sAOF_LatencyLow       = 0x0200, // good for interactive audio tools, pull all the strings

    sAOF_FormatMask       = 0xf000,
    sAOF_FormatStereo     = 0x0000, // 2 channel
    sAOF_Format5Point1    = 0x1000, // 6 channel
};

int sStartAudioOut(int freq,const sDelegate2<void,float *,int> &handler,int flags);
int64 sGetSamplesOut();
int sGetSamplesQueuedOut();
void sUpdateAudioOut(int flags);
void sStopAudioOut();

/****************************************************************************/

} // namespace Altona2

#endif  // FILE_ALTONA2_LIBS_BASE_SYSTEM_HPP
