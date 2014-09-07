/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Base/SystemPrivate.hpp"

namespace Altona2 {

using namespace Private;

/****************************************************************************/

namespace Private
{
    static sArray<sFileHandler *> *FileHandlers;
    sArray<sScreen *> OpenScreens;
    sMouseLockId MouseLock;
};


/****************************************************************************/

sScreenMode Private::CurrentMode;
uint Private::KeyQual;
sApp *Private::CurrentApp;
sDragData Private::DragData;
int Private::OldMouseButtons;
int Private::NewMouseButtons;
int Private::NewMouseX;
int Private::NewMouseY;
int Private::MouseWheelAkku;
float Private::RawMouseX;
float Private::RawMouseY;
float Private::RawMouseStartX;
float Private::RawMouseStartY;

/****************************************************************************/
/***                                                                      ***/
/***   Filehandler Manager                                                ***/
/***                                                                      ***/
/****************************************************************************/

class sFileHandlerSubsystem : public sSubsystem
{
public:
    sFileHandlerSubsystem() : sSubsystem("Filehandler Managment",0x10) {}


    void Init()
    {
        FileHandlers = new sArray<sFileHandler *>;
    }
    void Exit()
    {
        for(int i=0;i<FileHandlers->GetCount();i++)
            delete (*FileHandlers)[i];
        delete FileHandlers;
    }
} sFileHandlerSubsystem_;

/****************************************************************************/

sFileHandler::~sFileHandler()
{
}

sFile::~sFile()
{
}

/****************************************************************************/

void sAddFileHandler(sFileHandler *fh)
{
    FileHandlers->Add(fh);
}

sFileHandler *GetRootFileHandler()
{
    return (*FileHandlers)[0];
}

void sRemFileHandler(sFileHandler *fh)
{
    for(int i=0;i<FileHandlers->GetCount();i++)
    {
        if((*FileHandlers)[i]==fh)
        {
            FileHandlers->RemAtOrder(i);
            return;
        }
    }
    sASSERT0();
}

static sDelegate2<void, const char *, int> sFileLogHandler;

void sRegFileLogCb(sDelegate2<void, const char *, int> Handler)
{
    sFileLogHandler = Handler;
}

sFile *sOpenFile(const char *name,sFileAccess access)
{
    sFile *file;
    for(int i=FileHandlers->GetCount()-1;i>=0;i--)   // have to scan backwards!
    {
        file = (*FileHandlers)[i]->Create(name,access);
        if(file) 
        {
            if (!sFileLogHandler.IsEmpty()) sFileLogHandler.Call(name, 0);
            return file;
        }
    }
    sLogF("file","File not Found <%s>",name);
    if (!sFileLogHandler.IsEmpty()) sFileLogHandler.Call(name, 1);
    return 0;
}

bool sCheckFile(const char *name)
{
    for(int i=FileHandlers->GetCount()-1;i>=0;i--)   // have to scan backwards!
    {
        if ((*FileHandlers)[i]->Exists(name))
        {
            if (!sFileLogHandler.IsEmpty()) sFileLogHandler.Call(name, 2);
            return 1;
        }
    }
    if (sFileLogHandler!=0) sFileLogHandler.Call(name, 3);
    return 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   sLinkFileHanlder - symlinks and putting home folders to root       ***/
/***                                                                      ***/
/****************************************************************************/

sLinkFileHandler::sLinkFileHandler(const char *dest,const char *source)
{
    Dest = dest;
    Source = source;
}

bool sLinkFileHandler::BuildPath(const char *name)
{
    if(*name=='/')
        name++;

    if(Source)
    {
        uptr len = sGetStringLen(Source);
        if(sCmpStringILen(name,Source,len)!=0)
            return 0;
        name += len;
        if(*name++!='/')
            return 0;
    }

    Buffer.PrintF("%s/%s",Dest,name);
    return 1;
}

sFile *sLinkFileHandler::Create(const char *name,sFileAccess access)
{
    if(BuildPath(name))
        if(GetRootFileHandler()->Exists(Buffer))
            return GetRootFileHandler()->Create(Buffer,access);
    return 0;
}

bool sLinkFileHandler::Exists(const char *name)
{
    if(BuildPath(name))
        return GetRootFileHandler()->Exists(Buffer);
    return 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   Wrappers                                                           ***/
/***                                                                      ***/
/****************************************************************************/

uint8 *sLoadFile(const char *name)
{
    uptr size;
    return sLoadFile(name,size);
}

uint8 *sLoadFile(const char *name,uptr &size)
{
    size = 0;
    sFile *file = sOpenFile(name,sFA_Read);
    if(file==0)
        return 0;
    int64 size_ = file->GetSize();
    if(!sConfig64Bit)
        sASSERT(size_<=0x7fffffff);
    size = (uptr) size_;

    uint8 *mem = (uint8 *)sAllocMem(size,16,0);

    if(!file->Read(mem,size))
    {
        delete[] mem;
        mem = 0;
    }
    delete file;
    return mem;
}

bool sSaveFile(const char *name,const void *data,uptr bytes)
{
    sFile *file=sOpenFile(name,sFA_Write);
    if (!file) return 0;
    bool ret=file->Write(data,bytes);
    if(!file->Close()) ret = 0;
    delete file;
    return ret;
}

bool sSaveFileIfDifferent(const char *name,const void *data,uptr bytes)
{
    uptr oldsize = 0;
    uint8 *olddata = sLoadFile(name,oldsize);

    if(olddata)
    {
        if(bytes==oldsize && sCmpMem(olddata,data,oldsize)==0)
        {
            delete[] olddata;
            return true;
        }
        delete[] olddata;
    }

    return sSaveFile(name,data,bytes);
}

/****************************************************************************/
/****************************************************************************/

char *sLoadText(const char *name,bool forceutf8)
{
    uptr size = 0;

    sFile *file = sOpenFile(name,sFA_Read);
    if(file==0)
        return 0;
    int64 size_ = file->GetSize();
    if(!sConfig64Bit)
        sASSERT(size_<=0x7fffffff);
    size = (uptr) size_;

    uint8 *mem = (uint8 *)sAllocMem(size+1,16,0);

    if(!file->Read(mem,size))
    {
        delete[] mem;
        delete file;
        return 0;
    }
    delete file;

    if((mem[0]==0xfe && mem[1]==0xff) || (mem[0]==0xff && mem[1]==0xfe))
    {
        // UTF 16

        sLog("file","can not handle UTF16 files");
        mem = 0;
        return 0;
    }
    else if(mem[0]==0xef && mem[1]==0xbb && mem[2]==0xbf)
    {
        // UTF 8 (that's what we want anyhow)
        // just get rid to the header and append a zero

        for(uptr i=0;i<size-3;i++)
            mem[i] = mem[i+3];
        mem[size-3] = 0;
    }
    else if(forceutf8)
    {
        // append zero
        mem[size] = 0;
    }
    else
    {
        // convert from ansi to utf8
        // 0x00..0x7f   do nothing
        // 0x80..0x9f   convert to 2-3 byte sequence
        // 0xa0..0xff   convert to 2 byte sequence

        // first find out how much memory we acutally need (estimated):

        uptr osize = 1;
        for(uptr i=0;i<size;i++)
        {
            if(mem[i]<0x80) osize++;
            else if(mem[i]>=0xa0) osize+=2;
            else osize+=3;
        }

        uint8 *omem = new uint8[osize];
        char *d = (char *)omem;

        for(uptr i=0;i<size;i++)
        {
            if(mem[i]<0x80) 
            {
                *d++ = mem[i];
            }
            else if(mem[i]>=0xa0)
            {
                *d++ = 0xc0 | mem[i]>>6;
                *d++ = 0x80 | (mem[i]&0x3f);
            }
            else 
            {
                static const int table[0x20] =
                {
                    0x20ac,0x0081,0x201a,0x0192, 0x201e,0x2026,0x2020,0x2021,
                    0x02c6,0x2030,0x0160,0x2039, 0x0152,0x008d,0x017d,0x008f,
                    0x0090,0x2018,0x2019,0x201c, 0x201d,0x2020,0x2013,0x2014,
                    0x02dc,0x2122,0x0161,0x203a, 0x0153,0x009d,0x017e,0x0178,
                };

                sASSERT(mem[i]>=0x80 && mem[i]<0xa0);
                sWriteUTF8(d,table[mem[i]-0x80]);
            }
        }
        *d++ = 0;

        delete[] mem;
        mem = omem;
    }

    // convert CRLF -> LF

    {
        uint8 *s = mem;
        uint8 *d = mem;
        while(*s)
        {
            if(s[0]=='\r' && s[1]=='\n')
                s++;
            *d++ = *s++;
        }
        *d++ = 0;
    }

    // done

    return (char *)mem;
}

/****************************************************************************/

bool sSaveTextUTF8(const char *name,const char *data)
{
    static const uint8 header[3] = {0xef,0xbb,0xbf };
    uptr bytes = sGetStringLen(data);
    sFile *file=sOpenFile(name,sFA_Write);
    if (!file) return 0;
    bool ok = 1;
    if(!file->Write(header,3)) ok = 0;
    if(!file->Write(data,bytes)) ok = 0;
    if(!file->Close()) ok = 0;
    delete file;
    return ok;
}

bool sSaveTextUTF8IfDifferent(const char *name,const char *data)
{
    static const uint8 header[3] = {0xef,0xbb,0xbf };
    uptr oldsize = 0;
    uint8 *olddata = sLoadFile(name,oldsize);

    if(olddata)
    {
        int newsize = sGetStringLen(data)+3;
        if(newsize==oldsize)
        {
            if(sCmpMem(olddata,header,3)==0 && sCmpMem(olddata+3,data,oldsize-3)==0)
            {
                delete[] olddata;
                return true;
            }
        }
        delete[] olddata;
    }

    return sSaveTextUTF8(name,data);
}

bool sSaveTextAnsi(const char *filename,const char *text,bool doslf)
{
    int limit = 0;

    for(int i=0;text[i];i++)
    {
        if(doslf && text[i]=='\n')
            limit++;
        limit++;
    }

    uint8 *mem = new uint8[limit];

    uint8 *dest = mem;
    const char *s = text;

    while(*s)
    {
        int val = sReadUTF8(s);
        if(val>=0x100 || (val>=0x80 && val<0xa0))
        {
            *dest++ = '?';
        }
        else if(doslf && val=='\n')
        {
            *dest++ = '\r';
            *dest++ = '\n';
        }
        else
        {
            *dest++ = val;
        }
    }
    sASSERT(dest<=mem+limit);

    bool ok = false;
    ok = sSaveFile(filename,mem,dest-mem);
    delete mem;
    return ok;
}

/****************************************************************************/
/***                                                                      ***/
/***   Root filehandler                                                   ***/
/***                                                                      ***/
/****************************************************************************/

class sPackfileFile : public sFile
{
    uint64 DataOffset;
    int64 Offset;
    sFile *File;
    sPackfileEntry Entry;
    bool Ok;

public:
    sPackfileFile(sFile *file,uint64 offset,sPackfileEntry *de)
    {
        Entry = *de;
        File = file;
        DataOffset = offset;
        Offset = 0;
        Ok = 1;
    }

    ~sPackfileFile()
    {
    }

    bool Close()
    {
        File = 0;
        return Ok;
    }

    bool SetSize(int64)
    {
        Ok = 0;
        return 0;
    }

    int64 GetSize()
    {
        return Entry.Size;
    }

    // synchronious interface

    bool Read(void *data,uptr size)
    {
        if(int64(Offset+size) > int64(Entry.Size))
        {
            Ok = 0;
        }
        if(Ok)
        {
            if(!File->SetOffset(Offset+DataOffset+Entry.Offset) ||
                !File->Read(data,size))
                Ok = 0;

            Offset+=size;
        }
        return Ok;
    }

    bool Write(const void *data,uptr size)
    {
        Ok = 0;
        return 0;
    }

    bool SetOffset(int64 offset)
    {
        Offset = offset;
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

class sPackfileHandler : public sFileHandler
{
    int Count;
    uint64 DataOffset;
    sPackfileEntry *Dir;
    char *Strings;
    sFile *File;
    sString<sMaxPath> PackfileName;

public:
    sPackfileHandler(const char *name)
    {
        bool ok = 0;
        File = sOpenFile(name,sFA_Read);
        if(File)
        {
            sPackfileHeader hdr;

            PackfileName = name;

            if(File->Read(&hdr,sizeof(hdr)))
            {
                if(hdr.Magic==sPackfileMagic && hdr.Version==sPackfileVersion)
                {
                    Count = hdr.DirCount;
                    Strings = new char[hdr.StringBytes];
                    Dir = new sPackfileEntry[hdr.DirCount];
                    DataOffset = hdr.DataOffset;
                    if(File->Read(Dir,sizeof(sPackfileEntry)*Count) && 
                        File->Read(Strings,hdr.StringBytes))
                    {
                        ok = 1;
                    }
                }
            }
        }
        if(!ok)
            sFatalF("could not open packfile <%s>",name);
    }

    ~sPackfileHandler()
    {
        delete[] Dir;
        delete[] Strings;
        delete File;
    }

    sFile *Create(const char *name,sFileAccess access)
    {
        sPackfileFile *f = 0;
        if(access==sFA_Read || sFA_ReadRandom)
        {
            sPackfileEntry *de;
            for(int i=0;i<Count;i++)
            {
                de = &Dir[i];
                if(sCmpStringP(name,de->Name+Strings)==0)
                {
                    f = new sPackfileFile(File,DataOffset,de);
                    sLogF("file","open packfile at %08x <%s>:<%s> for read",de->Offset+DataOffset,PackfileName,name);
                }
            }
            if (!f)
            {
                const char *name2 = sExtractName(name);
                for(int i=0;i<Count;i++)
                {
                    de = &Dir[i];
                    if(sCmpStringP(name2,de->Name+Strings)==0)
                    {
                        f = new sPackfileFile(File,DataOffset,de);
                        sLogF("file","open packfile at %08x <%s>:<%s> for read",de->Offset+DataOffset,PackfileName,name);
                    }
                }
            }

        }
        return f;
    }

    bool Exists(const char *name)
    {
        sPackfileEntry *de;
        for(int i=0;i<Count;i++)
        {
            de = &Dir[i];
            if(sCmpStringP(name,de->Name+Strings)==0)
                return 1;
        }
        return 0;
    }
};

/****************************************************************************/

sFileHandler *sAddPackfile(const char *name)
{
    sPackfileHandler *pfh = new sPackfileHandler(name);
    sAddFileHandler(pfh);
    return pfh;
}



/****************************************************************************/
/***                                                                      ***/
/***   yasm / nasm                                                        ***/
/***                                                                      ***/
/****************************************************************************/

bool sAssemble(const char *asmfile,const char *objectfile)
{
    return sAssemble(asmfile,objectfile,sConfigPlatform,sConfig64Bit);
}

bool sAssemble(const char *asmfile,const char *objectfile,int platform,bool bit64)
{
    sString<sMaxPath*3> cmdline;

    const char *format = "???";
    const char *exe = "yasm -rnasm -pnasm";

    if(platform==sConfigPlatformLinux)
    {
        if(bit64)
            format = "elf64";
        else
            format = "elf32";
    }
    if(platform==sConfigPlatformOSX)
    {
        if(bit64)
            format = "macho64";
        else
            format = "macho64"; //by now
        exe = "/Users/Shared/bin/nasm";
    }
    if(platform==sConfigPlatformIOS)
    {
        if(bit64)
            format = "macho64";
        else
            format = "macho32";
        exe = "/Users/Shared/bin/nasm";
    }
    if(platform==sConfigPlatformWin)
    {
        if(bit64)
            format = "win64";
        else
            format = "win32";
    }

    cmdline.PrintF("%s -Xvc -f %s -o\"%s\" \"%s\"",exe,format,objectfile,asmfile);
    return sExecuteShell(cmdline);
}

/****************************************************************************/
/***                                                                      ***/
/***   input helpers                                                      ***/
/***                                                                      ***/
/****************************************************************************/

uint sCleanKeyCode(uint key)
{
    uint ukey  = key & (sKEYQ_Mask|sKEYQ_Break|sKEYQ_Alt);
    if(key & sKEYQ_Shift) ukey |= sKEYQ_Shift;
    if(key & sKEYQ_Ctrl) ukey |= sKEYQ_Ctrl;
    return ukey;
}

void sKeyName(const sStringDesc &buffer,uint key)
{
    sString<128> b;
    if(key & sKEYQ_Alt) b.Add("ALT+");
    if(key & sKEYQ_Ctrl) b.Add("CTRL+");
    if(key & sKEYQ_Shift) b.Add("SHIFT+");

    uint c = key & sKEYQ_Mask;

    switch(c)
    {
    default: 
        if(c>0x20 && c<0xe000)
            b.AddF("%c",c); 
        else
            b.Add("???"); 
        break;

    case sKEY_Backspace : b.Add("Backspace"); break;
    case sKEY_Tab : b.Add("Tab"); break;
    case sKEY_Enter : b.Add("Enter"); break;
    case sKEY_Escape : b.Add("Escape"); break;
    case sKEY_Space : b.Add("Space"); break;

    case sKEY_Up : b.Add("Up"); break;
    case sKEY_Down : b.Add("Down"); break;
    case sKEY_Left : b.Add("Left"); break;
    case sKEY_Right : b.Add("Right"); break;
    case sKEY_PageUp : b.Add("PageUp"); break;
    case sKEY_PageDown : b.Add("PageDown"); break;
    case sKEY_Home : b.Add("Home"); break;
    case sKEY_End : b.Add("End"); break;
    case sKEY_Insert : b.Add("Insert"); break;
    case sKEY_Delete : b.Add("Delete"); break;

    case sKEY_Pause : b.Add("Pause"); break;
    case sKEY_Scroll : b.Add("Scroll"); break;
    case sKEY_Numlock : b.Add("Numloc"); break;
    case sKEY_Print : b.Add("Print"); break;
    case sKEY_WinL : b.Add("LeftWin"); break;
    case sKEY_WinR : b.Add("RightWin"); break;
    case sKEY_WinM : b.Add("Menu"); break;

    case sKEY_F1 : b.Add("F1"); break;
    case sKEY_F2 : b.Add("F2"); break;
    case sKEY_F3 : b.Add("F3"); break;
    case sKEY_F4 : b.Add("F4"); break;
    case sKEY_F5 : b.Add("F5"); break;
    case sKEY_F6 : b.Add("F6"); break;
    case sKEY_F7 : b.Add("F7"); break;
    case sKEY_F8 : b.Add("F8"); break;
    case sKEY_F9 : b.Add("F9"); break;
    case sKEY_F10: b.Add("F10"); break;
    case sKEY_F11: b.Add("F11"); break;
    case sKEY_F12: b.Add("F12"); break;

    case sKEY_LMB : b.Add("LMB"); break;
    case sKEY_RMB : b.Add("RMB"); break;
    case sKEY_MMB : b.Add("MMB"); break;
    case sKEY_Extra1 : b.Add("X1MB"); break;
    case sKEY_Extra2 : b.Add("X2MB"); break;
    case sKEY_Pen : b.Add("Pen"); break;
    case sKEY_PenF : b.Add("Pen+Front"); break;
    case sKEY_PenB : b.Add("Pen+Back"); break;
    case sKEY_Rubber : b.Add("Rubber"); break;
    case sKEY_WheelUp : b.Add("WheelUp"); break;
    case sKEY_WheelDown : b.Add("WheelDown"); break;
    }

    sCopyString(buffer,b);
}


/****************************************************************************/

bool Private::AquireMouseLock(sMouseLockId mouselockid)
{
    if(MouseLock==sMLI_None)
        MouseLock = mouselockid;
    return MouseLock == mouselockid;
}

bool Private::TestMouseLock(sMouseLockId mouselockid)
{
    return MouseLock == mouselockid;
}

void Private::ReleaseMouseLock(sMouseLockId mouselockid)
{
    if(MouseLock==mouselockid)
        MouseLock = sMLI_None;
}

void sDragData::FakeTablet()
{
    StartXF = float(StartX);
    StartYF = float(StartY);
    PosXF = float(PosX);
    PosYF = float(PosY);
    Pressure = (Mode==sDM_Stop) ? 0.0f : 1.0f;
}

void Private::HandleMouse(int msgtime,int dbl,sScreen *scr)
{
    sASSERT(scr);

    int mo = OldMouseButtons;
    int mn = NewMouseButtons;
    bool drag = 0;
    DragData.Timestamp = msgtime;
    DragData.Screen = scr;
    if(NewMouseX!=DragData.PosX || NewMouseY!=DragData.PosY
        || RawMouseX-RawMouseStartX!=DragData.RawDeltaX
        || RawMouseY-RawMouseStartY!=DragData.RawDeltaY)
    {
        drag = 1;
        DragData.PosX = NewMouseX;
        DragData.PosY = NewMouseY;
        DragData.DeltaX = DragData.PosX - DragData.StartX;
        DragData.DeltaY = DragData.PosY - DragData.StartY;
        if(sConfigPlatform==sConfigPlatformWin)
        {
            DragData.RawDeltaX = RawMouseX-RawMouseStartX;
            DragData.RawDeltaY = RawMouseY-RawMouseStartY;
        }
        else
        {
            DragData.RawDeltaX = float(DragData.DeltaX);
            DragData.RawDeltaY = float(DragData.DeltaY);
        }
        DragData.FakeTablet();
    }

    if(!mo && mn)
    {
        drag = 1;
        DragData.Mode = sDM_Start;
        DragData.StartX = DragData.PosX;
        DragData.StartY = DragData.PosY;
        DragData.DeltaX = DragData.PosX - DragData.StartX;
        DragData.DeltaY = DragData.PosY - DragData.StartY;
        DragData.RawDeltaX = 0;
        DragData.RawDeltaY = 0;
        DragData.FakeTablet();
        RawMouseStartX = RawMouseX;
        RawMouseStartY = RawMouseY;

        if(NewMouseButtons & sMB_Left)   DragData.Buttons = sMB_Left;
        else if(NewMouseButtons & sMB_Right)  DragData.Buttons = sMB_Right;
        else if(NewMouseButtons & sMB_Middle) DragData.Buttons = sMB_Middle;
        else if(NewMouseButtons & sMB_Extra1) DragData.Buttons = sMB_Extra1;
        else if(NewMouseButtons & sMB_Extra2) DragData.Buttons = sMB_Extra2;

        DragData.Touches[0] = DragData;
        if(AquireMouseLock(sMLI_Mouse))
            CurrentApp->OnDrag(DragData);
        if(dbl & sMB_DoubleClick)
        {
            DragData.Buttons |= sMB_DoubleClick;
            if(TestMouseLock(sMLI_Mouse))
                CurrentApp->OnDrag(DragData);
            DragData.Buttons &= ~sMB_DoubleClick;
        }
        DragData.Mode = sDM_Drag;
    }

    if(drag==1)
    {
        DragData.Touches[0] = DragData;
        if(DragData.Mode==sDM_Hover || TestMouseLock(sMLI_Mouse))
            CurrentApp->OnDrag(DragData);
    }

    if(TestMouseLock(sMLI_Mouse))
    {
        sKeyData kd;
        uint qual = sGetKeyQualifier();
        if(dbl & sMB_DoubleClick)
            qual |= sKEYQ_Double;
        kd.Key = 0;
        kd.MouseX = DragData.PosX;
        kd.MouseY = DragData.PosY;
        kd.Timestamp = DragData.Timestamp;
        if(!(mo & sMB_Left  ) && (mn & sMB_Left  )) { kd.Key=sKEY_LMB   |qual; CurrentApp->OnKey(kd); }
        if(!(mo & sMB_Right ) && (mn & sMB_Right )) { kd.Key=sKEY_RMB   |qual; CurrentApp->OnKey(kd); }
        if(!(mo & sMB_Middle) && (mn & sMB_Middle)) { kd.Key=sKEY_MMB   |qual; CurrentApp->OnKey(kd); }
        if(!(mo & sMB_Extra1) && (mn & sMB_Extra1)) { kd.Key=sKEY_Extra1|qual; CurrentApp->OnKey(kd); }
        if(!(mo & sMB_Extra2) && (mn & sMB_Extra2)) { kd.Key=sKEY_Extra2|qual; CurrentApp->OnKey(kd); }
        qual |= sKEYQ_Break;
        if((mo & sMB_Left  ) && !(mn & sMB_Left  )) { kd.Key=sKEY_LMB   |qual; CurrentApp->OnKey(kd); }
        if((mo & sMB_Right ) && !(mn & sMB_Right )) { kd.Key=sKEY_RMB   |qual; CurrentApp->OnKey(kd); }
        if((mo & sMB_Middle) && !(mn & sMB_Middle)) { kd.Key=sKEY_MMB   |qual; CurrentApp->OnKey(kd); }
        if((mo & sMB_Extra1) && !(mn & sMB_Extra1)) { kd.Key=sKEY_Extra1|qual; CurrentApp->OnKey(kd); }
        if((mo & sMB_Extra2) && !(mn & sMB_Extra2)) { kd.Key=sKEY_Extra2|qual; CurrentApp->OnKey(kd); }
    }

    if(mo && !mn)
    {

        DragData.Mode = sDM_Stop;
        DragData.FakeTablet();
        DragData.Touches[0] = DragData;
        if(TestMouseLock(sMLI_Mouse))
        {
            CurrentApp->OnDrag(DragData);
            ReleaseMouseLock(sMLI_Mouse);
        }
        DragData.Mode = sDM_Hover;
        DragData.Buttons = 0;
    }
    OldMouseButtons = NewMouseButtons;
}

/****************************************************************************/
/***                                                                      ***/
/***   Clipboard                                                          ***/
/***                                                                      ***/
/****************************************************************************/

#if sConfigPlatform!=sConfigPlatformWin

class sClipboardSimple : public sClipboardBase
{
    int Id;
    uint8 *Data;
    uptr Size;

    void Clear()
    {
        delete[] Data;
        Data = 0;
        Size = 0;
        Id = 0;
    }

    void *MakeCopy()
    {
        void *d = new uint8[Size];
        sCopyMem(d,Data,Size);
        return d;
    }

public:
    sClipboardSimple()
    {
        Data = 0;
        Size = 0;
        Id = 0;
    }

    ~sClipboardSimple()
    {
        Clear();
    }


    void SetText(const char *str,uptr len=-1)
    {
        if(len==-1)
            len = sGetStringLen(str);
        Clear();

        Size = len+1;
        Data = new uint8[Size];
        Id = sSerId::sCharPtr;

        sCopyMem(Data,str,len);
        Data[len] = 0;
    }
    void SetBlob(const void *data,uptr size,int serid)
    {
        Clear();

        Size = size;
        Data = new uint8[Size];
        Id = serid;

        sCopyMem(Data,data,Size);
    }

    char *GetText()
    {
        if(Id==sSerId::sCharPtr)
        {
            return (char *)MakeCopy();
        }
        else
        {
            return 0;
        }
    }

    void *GetBlob(uptr &size,int &serid)
    {
        size = Size;
        serid = Id;
        return MakeCopy();
    }
};



class sClipboardSubsystem : public sSubsystem
{
public:
    sClipboardSubsystem() : sSubsystem("Clipboard",0x90) {}

    void Init()
    {
        sClipboard = new sClipboardSimple;
    }
    void Exit()
    {
        delete sClipboard;
    }
} sClipboardSubsystem_;

sClipboardBase *sClipboard;

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Render Font to Bitmap                                              ***/
/***                                                                      ***/
/****************************************************************************/

sSystemFontInfo::sSystemFontInfo()
{
    Name = 0;
    Flags = 0;
}

sSystemFontInfo::~sSystemFontInfo()
{
    delete[] Name;
}

/****************************************************************************/

sSystemFont::sSystemFont()
{
    SizeX = 0;
    SizeY = 0;
    Data = 0;
}

sSystemFont::~sSystemFont()
{
}


/****************************************************************************/

#if sConfigPlatform==sConfigPlatformLinux || sConfigPlatform==sConfigPlatformOSX || sConfigPlatform==sConfigPlatformIOS

const sSystemFontInfo *sGetSystemFont(sSystemFontId id)
{
    return 0;
}

bool sLoadSystemFontDir(sArray<const sSystemFontInfo *> &dir)
{
    return 0;
}

/****************************************************************************/

sSystemFont *sLoadSystemFont(const sSystemFontInfo *fi,int sx,int sy,int flags)
{
    return 0;
}

int sSystemFont::GetAdvance()
{
    return 0;
}

int sSystemFont::GetBaseline()
{
    return 0;
}

int sSystemFont::GetCellHeight()
{
    return 0;
}

bool sSystemFont::GetInfo(int code,int *abc)
{
    return 0;
}

bool sSystemFont::CodeExists(int code)
{
    return 0;
}

void sSystemFont::BeginRender(int sx,int sy,uint8 *data)
{
}

void sSystemFont::RenderChar(int code,int px,int py)
{
}

void sSystemFont::EndRender()
{
}

sOutline *sSystemFont::GetOutline(const char *text)
{
	return 0;
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Outlines                                                           ***/
/***                                                                      ***/
/****************************************************************************/

sOutline::sOutline()
{
}

sOutline::~sOutline()
{
    Vertices.DeleteAll();
    Segments.DeleteAll();
}

sOutline::Segment::Segment()
{
    P0 = P1 = C0 = C1 = 0;
    Next = Prev = 0;
    Temp = 0;
}

sOutline::Vertex::Vertex()
{
}

sOutline::Vertex::Vertex(float x,float y) : Pos(x,y)
{
}

sOutline::Segment *sOutline::AddSegment()
{
    Segment *s = new Segment;
    Segments.Add(s);
    return s;
}

void sOutline::Assert()
{
    for(auto first : Contours)
    {
        Segment *s = first;
        do
        {
            sASSERT(s->Next->Prev == s);
            sASSERT(s->Prev->Next == s);
            sASSERT(s->Next->P0 == s->P1);
            sASSERT(s->Prev->P1 == s->P0);
            s = s->Next;
        }
        while(s!=first);
    }
}

int sOutline::CountIntersection(sVector2 p0)
{
    int count = 0;
    for(auto s : ValidSegments)
    {
        sVector2 x;
        if(p0!=s->P0->Pos && p0!=s->P1->Pos)
        {
            sVector2 a = s->P0->Pos;
            sVector2 b = s->P1->Pos;
            if(p0.x>=sMin(a.x,b.x) && p0.x<sMax(a.x,b.x))
            {
                float y = (p0.x-a.x)*(b.y-a.y)/(b.x-a.x) + a.y;
                if(y<p0.y)
                {
                    if(a.x<b.x)
                        count++;
                    else
                        count--;
                }
            }
        }
    }  
    return count;
}

bool sOutline::CheckIntersection(sVector2 p0,sVector2 p1)
{
    for(auto s : ValidSegments)
    {
        sVector2 x;
        if(p0!=s->P0->Pos && p1!=s->P0->Pos && p0!=s->P1->Pos && p1!=s->P1->Pos)
            if(sIntersect(p0,p1,s->P0->Pos,s->P1->Pos,x))
                return 1;
    }  
    return 0;
}

void sOutline::Triangulate()
{
    ValidVertices.Clear();
    ValidSegments.Clear();
    Large.Clear();

    for(auto first : Contours)
    {
        Segment *s = first;
        int n = 0;
        do
        {
            ValidVertices.Add(s->P0);
            ValidSegments.Add(s);
            s = s->Next;
            n++;
        }
        while(s!=first);
        if(n>3)
            Large.Add(s);
    }

    while(Large.GetCount())
    {
        Segment *s0 = Large[0];
        for(auto first : Large)
        {
            Segment *s1 = first;
            do
            {
                if(s0!=s1)
                {
                    if(!CheckIntersection(s0->P0->Pos,s1->P0->Pos))
                    {
                        if(CountIntersection((s0->P0->Pos+s1->P0->Pos)*0.5f)==1)
                        {
                            Segment *n0 = AddSegment();
                            Segment *n1 = AddSegment();
                            n0->P0 = s1->P0;
                            n0->P1 = s0->P0;
                            n0->Prev = s1->Prev; n0->Prev->Next = n0;
                            n0->Next = s0;       n0->Next->Prev = n0;
                            n1->P0 = s0->P0;
                            n1->P1 = s1->P0;
                            n1->Prev = s0->Prev; n0->Prev->Next = n0;
                            n1->Next = s1;       n0->Next->Prev = n0;
                            Assert();
                            // hinzufügen des neuen loops
                            // entfernen von konturen die nur 3 groß sind
                        }
                    }
                }
                s1 = s1->Next;
            }
            while(s1!=first);
        }
    }

  /*
  int max = ValidVertices.GetCount();
  for(int i=0;i<max;i++)
  {
    for(int j=0;j<max;j++)
    {
      if(j!=i)
      {
        if(!CheckIntersection(ValidVertices[i]->Pos,ValidVertices[j]->Pos))
        {
          if(CountIntersection((ValidVertices[i]->Pos+ValidVertices[j]->Pos)*0.5f)==1)
          {
            Segment *s = AddSegment();
            s->P0 = ValidVertices[i];
            s->P1 = ValidVertices[j];
          }
        }
      }
    }
  }
  */
}



/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/

}
