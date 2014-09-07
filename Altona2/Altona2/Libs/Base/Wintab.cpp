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
#include "Wintab.h"
using namespace Altona2;
using namespace Altona2::Private;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

struct Packet
{
//    HCTX            Context;
//    UINT            Status;
//    DWORD           Time;
//    WTPKT           Changed;
//    UINT            SerialNumber;
    uint            Cursor;
    uint            Buttons;
    int             X;
    int             Y;
    LONG            Z;
    int             NormalPressure;
//    int             TangentPressure;
    ORIENTATION     Orientation;
//    ROTATION        Rotation; /* 1.1 */
} __TYPES ;


class sWacomSubsystem : public sSubsystem
{
    HINSTANCE Library;

    typedef UINT ( API * WTINFOA ) ( UINT, UINT, LPVOID );
    typedef HCTX ( API * WTOPENA )( HWND, LPLOGCONTEXTA, BOOL );
    typedef BOOL ( API * WTGETA ) ( HCTX, LPLOGCONTEXT );
    typedef BOOL ( API * WTSETA ) ( HCTX, LPLOGCONTEXT );
    typedef BOOL ( API * WTCLOSE ) ( HCTX );
    typedef BOOL ( API * WTENABLE ) ( HCTX, BOOL );
    typedef BOOL ( API * WTPACKET ) ( HCTX, UINT, LPVOID );
    typedef BOOL ( API * WTOVERLAP ) ( HCTX, BOOL );
    typedef BOOL ( API * WTSAVE ) ( HCTX, LPVOID );
    typedef BOOL ( API * WTCONFIG ) ( HCTX, HWND );
    typedef HCTX ( API * WTRESTORE ) ( HWND, LPVOID, BOOL );
    typedef BOOL ( API * WTEXTSET ) ( HCTX, UINT, LPVOID );
    typedef BOOL ( API * WTEXTGET ) ( HCTX, UINT, LPVOID );
    typedef BOOL ( API * WTQUEUESIZESET ) ( HCTX, int );
    typedef int  ( API * WTDATAPEEK ) ( HCTX, UINT, UINT, int, LPVOID, LPINT);
    typedef int  ( API * WTPACKETSGET ) (HCTX, int, LPVOID);

    WTINFOA WTInfoA;
    WTOPENA WTOpenA;
    WTGETA WTGetA;
    WTSETA WTSetA;
    WTCLOSE WTClose;
    WTPACKET WTPacket;
    WTENABLE WTEnable;
    WTOVERLAP WTOverlap;
    WTSAVE WTSave;
    WTCONFIG WTConfig;
    WTRESTORE WTRestore;
    WTEXTSET WTExtSet;
    WTEXTGET WTExtGet;
    WTQUEUESIZESET WTQueueSizeSet;
    WTDATAPEEK WTDataPeek;
    WTPACKETSGET WTPacketsGet;

    LOGCONTEXTA Context;

    int Buttons;
    sKeyData KeyData;
    sDragData DragData;
    int MaxNormalPressure;

public:
    sWacomSubsystem() : sSubsystem() {}

    void Init();
    void Exit();
    void MsgProc(const sWinMessage &msg);
} sWacomSubsystem_;
bool sWacomSubsystemRegistered;

void sWacomSubsystem::Init()
{
    sClear(KeyData);
    sClear(DragData);
    sClear(Context);
    MaxNormalPressure = 0;
    Library = 0;

    sWinMessageHook.Add(sDelegate1<void,const sWinMessage &>(this,&sWacomSubsystem::MsgProc));

    // we can not initialize without a screen and a window

    auto screen = sGetScreen(0);
    if(!screen) return;

    // load libary and get fpointers

    Library = LoadLibraryA("Wintab32.dll");
    if(!Library) return;

#define GETPROCADDRESS(type, func) func = (type)GetProcAddress(Library, #func); if(!func) return;

    GETPROCADDRESS( WTOPENA, WTOpenA );
    GETPROCADDRESS( WTINFOA, WTInfoA );
    GETPROCADDRESS( WTGETA, WTGetA );
    GETPROCADDRESS( WTSETA, WTSetA );
    GETPROCADDRESS( WTPACKET, WTPacket );
    GETPROCADDRESS( WTCLOSE, WTClose );
    GETPROCADDRESS( WTENABLE, WTEnable );
    GETPROCADDRESS( WTOVERLAP, WTOverlap );
    GETPROCADDRESS( WTSAVE, WTSave );
    GETPROCADDRESS( WTCONFIG, WTConfig );
    GETPROCADDRESS( WTRESTORE, WTRestore );
    GETPROCADDRESS( WTEXTSET, WTExtSet );
    GETPROCADDRESS( WTEXTGET, WTExtGet );
    GETPROCADDRESS( WTQUEUESIZESET, WTQueueSizeSet );
    GETPROCADDRESS( WTDATAPEEK, WTDataPeek );
    GETPROCADDRESS( WTPACKETSGET, WTPacketsGet );

#undef GETPROCADDRESS

    // Get context

    if(!WTInfoA(0,0,0)) return;

    if(!WTInfoA(WTI_DEFCONTEXT,0,&Context)) return;
    Context.lcOptions |= CXO_MESSAGES;
    Context.lcPktData = PK_X | PK_Y | PK_Z | PK_BUTTONS | PK_NORMAL_PRESSURE | PK_ORIENTATION | PK_CURSOR;
    Context.lcPktMode = 0;
    Context.lcMoveMask = Context.lcPktData;
    Context.lcBtnUpMask = Context.lcBtnDnMask;
    /*
    Context.lcInOrgX = 0;
    Context.lcInOrgY = 0;
    if(!WTInfoA(WTI_DEVICES,DVC_X,&Context.lcInExtX)) return;
    if(!WTInfoA(WTI_DEVICES,DVC_Y,&Context.lcInExtY)) return;
    if(!WTInfoA(WTI_DEVICES,DVC_Z,&Context.lcInExtY)) return;
    Context.lcOutOrgX = 0;
    Context.lcOutOrgY = 0;
    Context.lcOutExtX = Context.lcInExtX;
    Context.lcOutExtY = Context.lcInExtY;
    */

    AXIS axis;
    if(!WTInfoA(WTI_DEVICES,DVC_NPRESSURE,&axis)) return;
    MaxNormalPressure = axis.axMax;

    if(!WTOpenA(HWND(screen->ScreenWindow),&Context,TRUE)) return;
}

void sWacomSubsystem::MsgProc(const sWinMessage &msg)
{
    if(msg.Message==WT_PACKET)
    {
        auto screen = sGetScreen(0);
        if(!screen) return;

        Packet Packet;
        if(WTPacket((HCTX) msg.lparam,msg.wparam,&Packet))
        {
            if(0)
            {
                sDPrintF("%d %d %d - %d %d %d - %d %d %d\n"
                    ,Packet.X,Packet.Y,Packet.Z
                    ,Packet.Buttons,Packet.Cursor,Packet.NormalPressure
                    ,Packet.Orientation.orAltitude,Packet.Orientation.orAzimuth,Packet.Orientation.orTwist);
            }

            int ssx,ssy;
            screen->GetSize(ssx,ssy);

            float pfx = float(Packet.X)*float(ssx)/float(Context.lcOutExtX);
            float pfy = float(Context.lcOutExtY-1-Packet.Y)*float(ssy)/float(Context.lcOutExtY);
            float press = float(Packet.NormalPressure) / float(MaxNormalPressure);
            int pix = int(pfx+0.5f);
            int piy = int(pfy+0.5f);
            float pressex = sClamp<float>(press*4,0,2); pressex = pressex*pressex;
            DragData.RawDeltaX += pressex *(pfx-DragData.PosXF);
            DragData.RawDeltaY += pressex *(pfy-DragData.PosYF);
            DragData.PosX = pix;
            DragData.PosY = piy;
            DragData.PosXF = pfx;
            DragData.PosYF = pfy;
            DragData.DeltaX = DragData.PosX - DragData.StartX;
            DragData.DeltaY = DragData.PosY - DragData.StartY;
            DragData.Timestamp = (int)GetMessageTime();
            DragData.Pressure = press;
            DragData.Screen = screen;
            KeyData.Timestamp = DragData.Timestamp;
            KeyData.MouseX = pix;
            KeyData.MouseY = piy;

            WINDOWINFO info;
            sClear(info);
            info.cbSize = sizeof(WINDOWINFO);
            GetWindowInfo((HWND) screen->ScreenWindow,&info);
            SetCursorPos(pix+info.rcClient.left,piy+info.rcClient.top);

            if(Packet.Buttons && !Buttons && AquireMouseLock(sMLI_Wintab))
            {
                switch(Packet.Buttons | (Packet.Cursor<<16))
                {
                case 0x00010001: Buttons = sMB_Pen; KeyData.Key = sKEY_Pen; break;
                case 0x00010003: Buttons = sMB_PenF; KeyData.Key = sKEY_PenF; break;
                case 0x00010005: Buttons = sMB_PenB; KeyData.Key = sKEY_PenB; break;
                case 0x00020001: Buttons = sMB_Rubber; KeyData.Key = sKEY_Rubber; break;
                }
                KeyData.Key |= sGetKeyQualifier();

                DragData.Mode = sDM_Start;
                DragData.StartX = pix;
                DragData.StartY = piy;
                DragData.StartXF = pfx;
                DragData.StartYF = pfy;
                DragData.DeltaX = 0;
                DragData.DeltaY = 0;
                DragData.RawDeltaX = 0;
                DragData.RawDeltaY = 0;
                DragData.Buttons = Buttons;
                CurrentApp->OnKey(KeyData);
                CurrentApp->OnDrag(DragData);
                DragData.Mode = sDM_Drag;
            }
            else if(Packet.Buttons && Buttons && TestMouseLock(sMLI_Wintab))
            {
                CurrentApp->OnDrag(DragData);
            }
            if(!Packet.Buttons && Buttons && TestMouseLock(sMLI_Wintab))
            {
                DragData.Mode = sDM_Stop;
                CurrentApp->OnDrag(DragData);
                KeyData.Key |= sKEYQ_Break;
                CurrentApp->OnKey(KeyData);
                ReleaseMouseLock(sMLI_Wintab);
                Buttons = 0;
            }
        }
    }
}

void sWacomSubsystem::Exit()
{
    sWinMessageHook.Rem(sDelegate1<void,const sWinMessage &>(this,&sWacomSubsystem::MsgProc));
    if(Library)
        FreeLibrary(Library);
}

void Altona2::sInitWacom()
{
    if(!sWacomSubsystemRegistered)
    {
        sWacomSubsystemRegistered = true;
        sWacomSubsystem_.Register("Wintab (Wacom)",0xf0);
    }
}

/****************************************************************************/

