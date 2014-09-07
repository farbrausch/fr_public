/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_GUI_3D_HPP
#define FILE_ALTONA2_LIBS_GUI_3D_HPP

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Gui/Window.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class s3dWindow : public sWindow
{
protected:
    int RtSizeX;
    int RtSizeY;
    int RtMsCount;
    int RtMsQuality;
    sResource *RtColor;
    sResource *RtDepth;

    // 

    bool LayeredBlit;
    sPainterImage *BlitImage;
    sMaterial *BlitMtrl;

public:
    s3dWindow(bool layeredblit = false);
    ~s3dWindow();
    void OnInit();

    void OnPaint3d();
    void OnPaint(int layer);

    class sDebugPainter *DPaint;
    uint BackColor;
    bool Fps;
    int MsCount;
    int MsQuality;

    virtual bool PreRender(const sTargetPara &tp) { return 1; }
    virtual void Render(const sTargetPara &tp) {}
    virtual void PostRender(const sTargetPara &tp) { }
    virtual void DebugLog() {}
    int GetMsQuality(int count);
};

/****************************************************************************/

class s3dNavWindow : public s3dWindow
{
public:
    s3dNavWindow();
    void OnEnterFocus() { QuakeMask = 0; }
    void OnLeaveFocus() { QuakeMask = 0; }
    void DebugLog();

    // camera

    sVector41 Pos;
    sVector3 Rot;
    float Zoom;

    // nav

    sVector40 QuakeDir;
    int QuakeMask;
    int QuakeSpeedFactor;
    uint QuakeLastTime;
    float DragStartX;
    float DragStartY;
    float DragDist;
    uint MouseWheelTime;
    bool PerfmonHistory;
    bool FirstDragInFrame;
    bool Freecam;

    // cmd

    void CmdReset();
    void CmdFps() { Fps = !Fps; }
    void CmdPerfmonHistory() { PerfmonHistory = !PerfmonHistory; }
    void CmdStopPerfmon() { sEnablePerfMon(!sPerfMonEnabled()); }
    void CmdFreecam() { Freecam = !Freecam; }

    void Simulate();
    void DragRotate(const sDragData &dd);
    void DragOrbit(const sDragData &dd);
    void DragPan(const sDragData &dd);
    void CmdQuake(int code);
    void CmdQuakeSpeed(int n);
};

/****************************************************************************/
}
#endif  // FILE_ALTONA2_LIBS_GUI_3D_HPP

