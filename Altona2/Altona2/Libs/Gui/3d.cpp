/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Util/graphicshelper.hpp"
#include "Altona2/Libs/Util/debugpainter.hpp"
#include "3d.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/


s3dWindow::s3dWindow(bool layeredblit)
{
    Flags |= sWF_Paint3d;
    RtSizeX = 0;
    RtSizeY = 0;
    RtMsCount = 0;
    RtMsQuality = 0;
    RtColor = 0;
    RtDepth = 0;
    DPaint = 0;

    LayeredBlit = layeredblit;
    BlitImage = 0;
    BlitMtrl = 0;

    BackColor = 0xff204060;
    Fps = 1;
    MsCount = 0;
    MsQuality = 0;
}

void s3dWindow::OnInit()
{
    DPaint = new sDebugPainter(Adapter());

    if(LayeredBlit)
    {
        BlitMtrl = new sFixedMaterial(Adapter());
        BlitMtrl->SetState(sRenderStatePara(sMTRL_CullOff|sMTRL_DepthOff,sMB_Off));
        BlitMtrl->SetTexturePS(0,0,sSamplerStatePara(sTF_Clamp|sTF_Linear));
        BlitMtrl->Prepare(Adapter()->FormatPCT);

        BlitImage = new sPainterImage(BlitMtrl);
    }
}

s3dWindow::~s3dWindow()
{
    delete DPaint;

    delete RtColor;
    delete RtDepth;
    delete BlitImage;
}


void s3dWindow::OnPaint3d()
{
    int sx = Client.SizeX();
    int sy = Client.SizeY();
    if(sx*sy==0)
        return;
    if(sx!=RtSizeX || sy!=RtSizeY || MsCount!=RtMsCount || MsQuality!=RtMsQuality)
    {
        sScreenMode mode;
        sGetScreen()->GetMode(mode);
        sDelete(RtColor);
        sDelete(RtDepth);
        RtSizeX = sx;
        RtSizeY = sy;
        RtMsCount = MsCount;
        RtMsQuality = MsQuality;
        sResPara rpc(sRBM_ColorTarget|sRBM_Shader|sRU_Gpu|sRM_Texture,mode.ColorFormat,sRES_NoMipmaps,sx,sy);
        sResPara rpd(sRBM_DepthTarget|sRBM_Shader|sRU_Gpu|sRM_Texture,mode.DepthFormat,sRES_NoMipmaps,sx,sy);
        if(RtMsQuality<Adapter()->GetMultisampleQuality(rpc,RtMsCount))
        {
            rpc.MSCount = rpd.MSCount = RtMsCount;
            rpc.MSQuality = rpd.MSQuality = RtMsQuality;
        }
        RtColor = new sResource(Adapter(),rpc);
        RtDepth = new sResource(Adapter(),rpd);

        if(BlitMtrl && BlitImage)
        {
            BlitMtrl->UpdateTexturePS(0,RtColor);
            BlitImage->SizeX = RtSizeX;
            BlitImage->SizeY = RtSizeY;
        }
    }

    sTargetPara tp(sTAR_ClearAll,BackColor,RtColor,RtDepth);
    sViewport view;

    if(PreRender(tp))
    {
        Screen->GuiAdapter->Context->BeginTarget(tp);

        if(Fps)
        {
            DebugLog();
            DPaint->PrintFPS();
            DPaint->PrintStats();
        }

        Render(tp);
        DPaint->Draw(tp,&Client);

        Screen->GuiAdapter->Context->EndTarget();
        PostRender(tp);
    }
}

int s3dWindow::GetMsQuality(int count)
{
    sScreenMode mode;
    sGetScreen()->GetMode(mode);
    sResPara rpc(sRBM_ColorTarget|sRU_Gpu|sRM_Texture,mode.ColorFormat,sRES_NoMipmaps,Client.SizeX(),Client.SizeY());
    return Adapter()->GetMultisampleQuality(rpc,count);
}


void s3dWindow::OnPaint(int layer)
{
    if(RtColor)
    {
        if(BlitImage)
        {
            Painter()->SetLayer(layer);
            Painter()->Image(BlitImage,0xffffffff,sRect(0,0,RtSizeX,RtSizeY),Client);
        }
        else
        {
            sCopyTexturePara cp(0,sGetScreen()->GetScreenColor(),RtColor);
            cp.DestRect = Client;
            cp.SourceRect.Set(0,0,RtSizeX,RtSizeY);
            Screen->GuiAdapter->Context->Copy(cp);
        }
    }
}


/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

s3dNavWindow::s3dNavWindow()
{
    Flags |= sWF_Paint3d;
    DPaint = 0;
    FirstDragInFrame = true;

    Pos.Set(0,0,-4);

    CmdReset();
    QuakeLastTime = sGetTimeMS();
    QuakeDir.Set(0,0,0);
    DragStartX = 0;
    DragStartY = 0;
    DragDist = 0;
    MouseWheelTime = 0;
    PerfmonHistory = 0;
    FirstDragInFrame = 0;
    Freecam = 0;

    // interface

    AddKey("Reset",'r',sGuiMsg(this,&s3dNavWindow::CmdReset));
    AddKey("Display FPS",'F'|sKEYQ_Shift,sGuiMsg(this,&s3dNavWindow::CmdFps));
    AddKey("Perfmon History",'H'|sKEYQ_Shift,sGuiMsg(this,&s3dNavWindow::CmdPerfmonHistory));
    AddKey("Stop Perfmon",sKEY_Pause|sKEYQ_Shift,sGuiMsg(this,&s3dNavWindow::CmdStopPerfmon));
    AddKey("Freecam",'f',sGuiMsg(this,&s3dNavWindow::CmdFreecam));
    AddKey("Move Backward",'s'            ,sGuiMsg(this,&s3dNavWindow::CmdQuake,0x00));
    AddKey("Move Backward",'s'|sKEYQ_Break,sGuiMsg(this,&s3dNavWindow::CmdQuake,0x10));
    AddKey("Move Forward" ,'w'            ,sGuiMsg(this,&s3dNavWindow::CmdQuake,0x01));
    AddKey("Move Forward" ,'w'|sKEYQ_Break,sGuiMsg(this,&s3dNavWindow::CmdQuake,0x11));
    AddKey("Move Left"    ,'a'            ,sGuiMsg(this,&s3dNavWindow::CmdQuake,0x02));
    AddKey("Move Left"    ,'a'|sKEYQ_Break,sGuiMsg(this,&s3dNavWindow::CmdQuake,0x12));
    AddKey("Move Right"   ,'d'            ,sGuiMsg(this,&s3dNavWindow::CmdQuake,0x03));
    AddKey("Move Right"   ,'d'|sKEYQ_Break,sGuiMsg(this,&s3dNavWindow::CmdQuake,0x13));
    AddKey("Faster (hold)", sKEY_Shift|sKEYQ_Shift, sGuiMsg(this, &s3dNavWindow::CmdQuake, 0x04));
    AddKey("Faster (hold)", sKEY_Shift|sKEYQ_Break, sGuiMsg(this, &s3dNavWindow::CmdQuake, 0x14));
    AddKey("Slower (hold)", sKEY_Ctrl|sKEYQ_Ctrl, sGuiMsg(this, &s3dNavWindow::CmdQuake, 0x05));
    AddKey("Slower (hold)", sKEY_Ctrl|sKEYQ_Break, sGuiMsg(this, &s3dNavWindow::CmdQuake, 0x15));
    AddKey("Faster", sKEY_WheelUp, sGuiMsg(this, &s3dNavWindow::CmdQuakeSpeed, 1));
    AddKey("Slower",sKEY_WheelDown,sGuiMsg(this,&s3dNavWindow::CmdQuakeSpeed,-1));

    AddDrag("Rotate",sKEY_LMB,sGuiMsg(this,&s3dNavWindow::DragRotate));
    AddDrag("Orbit",sKEY_LMB|sKEYQ_Alt,sGuiMsg(this,&s3dNavWindow::DragOrbit));
    AddDrag("Pan", sKEY_MMB, sGuiMsg(this, &s3dNavWindow::DragPan));

    AddHelp();
}

void s3dNavWindow::CmdReset()
{
    Pos.Set(0,0,-4);
    Rot.Set(0,0,0);
    QuakeMask = 0;
    QuakeSpeedFactor = 0;
}

void s3dNavWindow::DebugLog()
{
    DPaint->PrintPerfMon(PerfmonHistory ? 10 : 1,3);
}

void s3dNavWindow::Simulate()
{
    sVector40 qs;

    if(QuakeMask & 0x01) qs.z -= 1;
    if(QuakeMask & 0x02) qs.z += 1;
    if(QuakeMask & 0x04) qs.x -= 1;
    if(QuakeMask & 0x08) qs.x += 1;

    float speed = sPow(2.0f,QuakeSpeedFactor*0.5f);
    if (QuakeMask & 0x10) speed *= 5.0f;
    if (QuakeMask & 0x20) speed /= 5.0f;

    uint time = sGetTimeMS();
    float dt = (time-QuakeLastTime)*0.001f;
    QuakeLastTime = time;

    sMatrix44A mat = sEulerXYZ(Rot);
    qs *= speed*dt;
    Pos += mat * qs;
}

void s3dNavWindow::DragRotate(const sDragData &dd)
{
    switch(dd.Mode)
    {
    case sDM_Start:
        DragStartX = Rot.x;
        DragStartY = Rot.y;
        break;
    case sDM_Drag:
        Rot.x = DragStartX + dd.DeltaY*0.01f;
        Rot.y = DragStartY + dd.DeltaX*0.01f;
        break;
    }
}

void s3dNavWindow::DragOrbit(const sDragData &dd)
{
    sVector40 v;
    sMatrix44A mat;
    switch(dd.Mode)
    {
    case sDM_Start:
        DragStartX = Rot.x;
        DragStartY = Rot.y;
        v.Set(Pos);
        DragDist = sLength(v);
        break;
    case sDM_Drag:
        Rot.x = DragStartX + dd.DeltaY*0.01f;
        Rot.y = DragStartY + dd.DeltaX*0.01f;
        mat = sEulerXYZ(Rot);
        v = sBaseZ(mat);
        v *= -DragDist;
        Pos.Set(v);
        break;
    }
}

void s3dNavWindow::DragPan(const sDragData &dd)
{
    sVector41 v;
    float speed;
    switch (dd.Mode)
    {
    case sDM_Start:
        DragStartX = Pos.x;
        DragStartY = Pos.y;
        DragDist = Pos.z;
        break;
    case sDM_Drag:
        speed = 0.01f * sPow(2.0f, QuakeSpeedFactor*0.5f);
        v = sVector41(DragStartX, DragStartY, DragDist);
        v += sEulerXYZ(Rot) * sVector40(dd.DeltaX * -speed, dd.DeltaY * speed, 0);
        Pos.Set(v);
        break;
    }
}


void s3dNavWindow::CmdQuake(int code)
{
    if(code & 0x10)
        QuakeMask &= ~(1<<(code & 0x0f));
    else
        QuakeMask |= (1<<(code & 0x0f));
}

void s3dNavWindow::CmdQuakeSpeed(int n)
{
    QuakeSpeedFactor = sClamp(QuakeSpeedFactor+n,-40,40);
    MouseWheelTime = sGetTimeMS()+2000;
}

/****************************************************************************/
}
