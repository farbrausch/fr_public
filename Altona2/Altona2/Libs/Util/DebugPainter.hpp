/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_UTIL_DEBUGPAINTER_HPP
#define FILE_ALTONA2_LIBS_UTIL_DEBUGPAINTER_HPP

#include "Altona2/Libs/Base/Base.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

enum sDebugPrintEnum
{
    sDPF_Frame  = 0x0001,
    sDPF_ConOut = 0x0002,
    sDPF_ConIn  = 0x0004,
    sDPF_Wires  = 0x0008,
};

class sDebugPainter
{
    enum cmdenum
    {
        CmdLine2D,
        CmdRect2D,
        CmdFrame2D,
        CmdPoint3D,
        CmdLine3D,
        CmdCircle3D,
        CmdAABox3D,
        CmdBox3D,
        CmdText3D,
    };

    struct cmd
    {
        cmdenum Kind;
        float Duration;
        uint Color;
        float Width;
    };
    struct cmd2D : cmd
    {
        sFRect Rect;
    };
    struct cmd3D : cmd
    {
        sVector41 p0;
        sVector41 p1;     // or radius
        const char *Text;
    };

    // -------

    sString<sFormatBuffer> PrintBuffer;
    sTextLog FrameBuffer;
    sTextLog ConsoleBuffer;
    uint TextColor;
    uint TextAltColor;
    float TextSize;

    int MaxIndex;
    int MaxVertex;
    sMemoryPool Pool;
    sArray<cmd> Cmds;

    sGeometry *Geo;
    sFixedMaterial *Mtrl;
    sResource *Tex;
    sVertexFormat *Format;
    sCBuffer<sFixedMaterialVC> *CBuffer;

    sArray<const sPerfMonBase::Record*> PerfmonRecordStack;

    bool PerfMonFlag;
    float DrawPerfMon(sContext *ctx,const sTargetPara &tp,sRect *clientrect);
    int FilterIndex;
    int FilterFramerate[24];
    sAdapter *Adapter;
    int PerfMonHistory;
    int PerfMonFramesTarget;

public:

    // control

    sDebugPainter(sAdapter *adapter);
    ~sDebugPainter();
    void Draw(sContext *ctx,const sTargetPara &,sRect *clientrect=0);
    void Draw(const sTargetPara &tp,sRect *clientrect=0)
    { Draw(Adapter->ImmediateContext,tp,clientrect); }
    int Flags;                       // sDPF_??? flags
    void OnInput(uint key);           // if you want to allow enabling the console from keyboard or console input. optional.

    // printing

    void SetPrintMode(uint color=0xc0ffffffU,uint altcolor=0xffffffffU,float size = 1.0f);
    void FramePrint(const char *);   // will be cleared every frame
    void ConsolePrint(const char *); // will log over multiple frames
    sPRINTING0(FramePrintF,sFormatStringBuffer buf=sFormatStringBase(PrintBuffer,format);buf,FramePrint(PrintBuffer););
    sPRINTING0(ConsolePrintF,sFormatStringBuffer buf=sFormatStringBase(PrintBuffer,format);buf,ConsolePrint(PrintBuffer););
    void PrintFPS();                  // print fps and ms
    void PrintStats();                // print sGfxStats
    void PrintStats(sPipelineStats &stats); // print more precize DX11 stats
    void PrintPerfMon(int history = 1,int frames=0); // print performance monitor

    // 2d drawing (in pixels)

    void SetDrawMode(float duration=0,uint color=0xffffffff,float width = 1.0f);
    void Line2D(float x0,float y0,float x1,float y1);
    void Rect2D(const sFRect &r);
    void Frame2D(const sFRect &r);

    // 3d drawing (using viewport, will also use SetDrawMode())

    void Point3D(const sVector41 &p);
    void Line3D(const sVector41 &p0,const sVector41 &p1);
    void Circle3D(const sVector41 &p,float r);
    void AABox3D(const sVector41 &p0,const sVector41 &p1);
    void Box3D(const sMatrix44A &center_and_diameter);
    void Text3D(const sVector41 &p0,const char *text);
    sPRINTING1(Text3DF,sFormatStringBuffer buf=sFormatStringBase(PrintBuffer,format);buf,Text3D(arg1,PrintBuffer);,const sVector41 &);
};

/****************************************************************************/

}

#endif  // FILE_ALTONA2_LIBS_UTIL_DEBUGPAINTER_HPP

