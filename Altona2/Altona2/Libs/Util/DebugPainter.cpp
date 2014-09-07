/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Base/Perfmon.hpp"
#include "Altona2/Libs/Util/GraphicsHelper.hpp"
#include "DebugPainter.hpp"

namespace Altona2 {

extern const uint8 sFontData[];

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/


sDebugPainter::sDebugPainter(sAdapter *adapter)
{
    Adapter = adapter;
    Format = 0;
    Tex = 0;
    Mtrl = 0;
    Geo = 0;
    Pool = 0;
    MaxIndex = 0x18000;
    MaxVertex = 0x10000;
    PerfMonFlag = 0;
    SetPrintMode();
    FilterIndex = 0;
    for(int i=0;i<sCOUNTOF(FilterFramerate);i++)
        FilterFramerate[i] = 1;
    PerfMonHistory = 1;
    PerfMonFramesTarget = 0;

    // create vf

    Format = Adapter->FormatPCT;

    // create texture

    uint *data = new uint[128*128];
    for(int y=0;y<16;y++)
    {
        for(int x=0;x<16;x++)
        {
            for(int i=0;i<8;i++)
            {
                int bits = sFontData[(y*16+x)*8+i];
                for(int j=0;j<8;j++)
                {
                    data[(y*8+i)*128+x*8+j] = (bits&0x80) ? 0xffffffff : 0x00ffffff;
                    bits = bits*2;
                }
            }
        }
    }
    Tex = new sResource(Adapter,sResTexPara(sRF_BGRA8888,128,128,1),data,128*128*4);
    delete[] data;

    // material

    Mtrl = new sFixedMaterial();
    Mtrl->SetState(Adapter->CreateRenderState(sRenderStatePara(sMTRL_CullOff|sMTRL_DepthRead,sMB_Alpha)));
    Mtrl->SetTexturePS(0,Tex,Adapter->CreateSamplerState(sSamplerStatePara(sTF_Point|sTF_Tile,0)));
    Mtrl->Prepare(Adapter,Format);

    // Geometry

    uint16 *ib = new uint16[MaxIndex+5];
    for(int i=0;i<MaxIndex/6;i++)
    {
        ib[i*6+0] = 0+i*4;
        ib[i*6+1] = 1+i*4;
        ib[i*6+2] = 2+i*4;
        ib[i*6+3] = 0+i*4;
        ib[i*6+4] = 2+i*4;
        ib[i*6+5] = 3+i*4;
    }

    Geo = new sGeometry();
    Geo->SetIndex(new sResource(Adapter,sResBufferPara(sRBM_Index|sRU_Static,sizeof(uint16),MaxIndex),ib,MaxIndex*sizeof(uint16)),1);
    Geo->SetVertex(new sResource(Adapter,sResBufferPara(sRBM_Vertex|sRU_MapWrite,sizeof(sVertexPCT),MaxVertex)),0,1);
    Geo->Prepare(Format,sGMP_Tris|sGMF_Index16,MaxIndex,0,MaxVertex,0);

    delete[] ib;

    // Constant Buffer

    CBuffer = new sCBuffer<sFixedMaterialVC>(Adapter,sST_Vertex,0);
}

sDebugPainter::~sDebugPainter()
{
    delete CBuffer;
    delete Geo;
    delete Mtrl;
    delete Tex;
}

void sDebugPainter::Draw(sContext *ctx,const sTargetPara &para,sRect *clientrect)
{
    sZONE("sDebugPainter",0xff404040);

    bool oldenable = sEnableGfxStats(0);

    // define geometry

    float startx = 10/TextSize;
    float starty = 10/TextSize;
    float cursorx = startx;
    float cursory = starty;
    float sizex = 6;
    float sizey = 8;

    float endx = (para.SizeX-10)/(TextSize);
    int lines = int((para.SizeY-20)/(TextSize*(sizey+1)));

    // constants

    sMatrix44 mat;
    mat.i.x = TextSize*2/para.SizeX;
    mat.j.y = TextSize*2/para.SizeY;
    mat.i.w = -1+0.0f/para.SizeX;
    mat.j.w =  1-0.0f/para.SizeY;
    CBuffer->Map(ctx);
    CBuffer->Data->MS2SS = mat;
    CBuffer->Unmap(ctx);

    // perfmon

    if(PerfMonFlag)
    {
        cursory = DrawPerfMon(ctx,para,clientrect) + 10;
        PerfMonFlag = 0;
    }

    // shorten print buffer

    ConsoleBuffer.TrimToLines(sMax(5,lines-1-FrameBuffer.CountLines()));

    // print into vertexbuffer

    sVertexPCT *vp;
    uint col = TextColor;
    bool alt = 0;
    Geo->VB(0)->MapBuffer(ctx,&vp,sRMM_Discard);
    sVertexPCT *vpend = vp+(MaxVertex-3);
    int count = 0;

    for(int source=0;source<2;source++)
    {
        const char *s;
        if(source==0) 
            s = FrameBuffer.Get();
        else
            s = ConsoleBuffer.Get();

        while(*s && vp<vpend)
        {
            if(*s=='\'')
            {
                if(s[1]=='#')
                {
                    int col_;
                    s+=2;
                    if(sScanHex(s,col_))
                        col = uint(col_);
                }
                else if(s[1]=='\'')
                {
                    s++;
                }
                else
                {
                    alt = !alt;
                    s++;
                }
            }
            else if(*s=='\n')
            {
                alt = 0;
                cursory += sizey+1;
                cursorx = startx;
                col = TextColor;
                s++;
            }
            else
            {
                if(cursorx+sizex > endx)
                {
                    cursory += sizey+1;
                    cursorx = startx;
                }
                sFRect uv;
                sFRect p;
                int code = sReadUTF8(s);
                if(code<0 || code>255) code = '?';
                int u = ((code) & 15)*8;
                int v = ((code) / 16)*8;
                uv.x0 = u/128.0f;
                uv.y0 = v/128.0f;
                uv.x1 = (u+sizex)/128.0f;
                uv.y1 = (v+sizey)/128.0f;
                p.x0 = cursorx;
                p.y0 = cursory;
                p.x1 = cursorx+sizex;
                p.y1 = cursory+sizey;
                if(sConfigRender==sConfigRenderDX9)
                {
                    p.x0 -= 0.5f;
                    p.y0 -= 0.5f;
                    p.x1 -= 0.5f;
                    p.y1 -= 0.5f;
                }
                vp[0].Init(p.x0,-p.y0,0,alt?TextAltColor:col,uv.x0,uv.y0);
                vp[1].Init(p.x1,-p.y0,0,alt?TextAltColor:col,uv.x1,uv.y0);
                vp[2].Init(p.x1,-p.y1,0,alt?TextAltColor:col,uv.x1,uv.y1);
                vp[3].Init(p.x0,-p.y1,0,alt?TextAltColor:col,uv.x0,uv.y1);

                cursorx += sizex;
                count += 4;
                vp += 4;
            }
        }
        cursory += sizey;
        cursorx = startx;
    }

    Geo->VB()->Unmap(ctx);

    // put on screen

    if(count>0)
    {
        sDrawRange r;
        sDrawPara dp(Geo,Mtrl,CBuffer);
        dp.RangeCount = 1;
        dp.Flags |= sDF_Ranges;
        dp.Ranges = &r;
        dp.Ranges->Start = 0;
        dp.Ranges->End = count/4*6;
        ctx->Draw(dp);
    }

    FrameBuffer.Clear();

    sEnableGfxStats(oldenable);
}

void sDebugPainter::OnInput(uint key)
{
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void sDebugPainter::SetPrintMode(uint color,uint altcolor,float size)
{
    TextColor = color;
    TextAltColor = altcolor;
    TextSize = size;
}

void sDebugPainter::FramePrint(const char *s)
{
    FrameBuffer.Print(s);
}

void sDebugPainter::ConsolePrint(const char *s)
{
    ConsoleBuffer.Print(s);
}

/****************************************************************************/

void sDebugPainter::PrintFPS()
{
    sGfxStats stat;
    sGetGfxStats(stat);
    FramePrintF("'%.2f' ms %.2f fps\n",stat.FilteredFrameDuration*1000.0f,1.0f/stat.FilteredFrameDuration);
}

void sDebugPainter::PrintStats()
{
    sGfxStats stat;
    sGetGfxStats(stat);
    FramePrintF("'%d' batches, %d instances, %d ranges, %d primitives, %d indices\n",stat.Batches,stat.Instances,stat.Ranges,stat.Primitives,stat.Indices);
}

void sDebugPainter::PrintStats(sPipelineStats &stat)
{
    FramePrintF("shaders: %d vs, %d hs, %d ds, %d gs, %d ps, %d cs\n",stat.VS,stat.HS,stat.DS,stat.GS,stat.PS,stat.CS);
    FramePrintF("primitives: %d vs, %d gs, %d ras, %d c&c ; vertices %d\n",
        stat.IAPrimitives,stat.GSPrimitives,stat.Primitives,stat.ClippedPrimitives,stat.IAVertices);
}

void sDebugPainter::PrintPerfMon(int history,int frames)
{
    PerfMonHistory = history ? history : 9999;
    PerfMonFramesTarget = frames;
    PerfMonFlag = 1;
}

/****************************************************************************/
/***                                                                      ***/
/***   PerfMon                                                            ***/
/***                                                                      ***/
/****************************************************************************/

float sDebugPainter::DrawPerfMon(sContext *ctx,const sTargetPara &tp,sRect *clientrect)
{
#if sConfigPlatform==sConfigPlatformWin

    if(sGetPerfMonThreads()==0)
        return 10;

    float sum = 0.5f;
    for(int i=0;i<sCOUNTOF(FilterFramerate);i++)
        sum += float(FilterFramerate[i]);
    int PerfMonFrames = int(sum/sCOUNTOF(FilterFramerate));
    if(PerfMonFramesTarget)
        PerfMonFrames = PerfMonFramesTarget;

    const sArray<sPerfMonBase *> *mons = sGetPerfMonThreads();

    int count = 0;
    sVertexPCT *vp;
    Geo->VB(0)->MapBuffer(ctx,&vp,sRMM_Discard);
    sVertexPCT *vpend = vp+(MaxVertex-3);

    float x = 10;
    float y = 10;
    float yh = 8;
    int mx,my,mb;
    sGetMouse(mx,my,mb);
    if(clientrect)
    {
        mx -= clientrect->x0;
        my -= clientrect->y0;
    }
    else
    {
        mx -= tp.Window.x0;
        my -= tp.Window.y0;
    }
    sString<256> buffer;
    const sPerfMonThread::Record *MouseOver = 0;

    buffer = "Perfmon - hover over bars for more info\n";

    int maxx = 0;

    float tmax = 0;
    for(auto th : *mons)
        for(int frame=1;frame<th->History && frame<=PerfMonHistory;frame++)
        {
            uint64 freq;
            const sArray<sPerfMonThread::Record> *rec = th->GetFrame(frame,freq);
            if(rec->IsEmpty())
                continue;
            float scale = ((tp.SizeX-20)*60.0f/PerfMonFrames)/float(freq);
            PerfmonRecordStack.Clear();
            uint64 t0 = (*rec)[0].Time;
            const sPerfMonThread::Record *rpaint = 0;
            const sPerfMonThread::Record *rpop = 0;

            sPerfMonThread::Record dummy;
            sPerfMonSection dummysect("error",0xff000000);
            dummy.Kind = sPerfMonThread::RK_Enter;
            dummy.Section = &dummysect;
            dummy.Time = 0;

            float lastx = x;
            for(auto &rv_ : *rec)
            {
                auto r = &rv_;
                if(r->Kind==sPerfMonBase::RK_Enter)
                {
                    rpaint = 0;
                    if(!PerfmonRecordStack.IsEmpty())
                    {
                        rpaint = PerfmonRecordStack[PerfmonRecordStack.GetCount()-1];
                    }
                    else
                    {
                        rpaint = &dummy;
                    }
                    PerfmonRecordStack.AddTail(r);
                }
                else
                {
                    if(PerfmonRecordStack.GetCount()!=0)
                    {
                        rpaint = r;
                        rpop = PerfmonRecordStack.RemTail();
                    }
                    else
                    {
                        rpaint = &dummy;
                    }
                }
                if(rpaint && vp<vpend)
                {
                    sFRect p;
                    p.x0 = lastx;
                    p.x1 = x+float(r->Time-t0)*scale;

                    uint col = sVERTEXCOLOR(rpaint->Section->Color);
                    lastx = p.x1;
                    tmax = sMax<float>(tmax,float(r->Time-t0)/freq);
                    p.y0 = y;
                    p.y1 = y+yh;
                    maxx = sMax(maxx,int(p.x1));
                    if(sConfigRender==sConfigRenderDX9)
                    {
                        p.x0 -= 0.5f;
                        p.y0 -= 0.5f;
                        p.x1 -= 0.5f;
                        p.y1 -= 0.5f;
                    }

                    vp[0].Init(p.x0,-p.y0,0,col,0,0);
                    vp[1].Init(p.x1,-p.y0,0,col,0,0);
                    vp[2].Init(p.x1,-p.y1,0,col,0,0);
                    vp[3].Init(p.x0,-p.y1,0,col,0,0);
                    count+=4;
                    vp+=4;
                    if(p.Hit(float(mx),float(my)))
                        MouseOver = rpaint;

                    if(MouseOver && r->Section==MouseOver->Section && rpop && rpop->Section==MouseOver->Section && r->Kind==sPerfMonBase::RK_Leave)
                    {
                        p.x0 = x+float(rpop->Time-t0)*scale;
                        p.x1 = x+float(r->Time-t0)*scale;
                        p.y0 = y;
                        p.y1 = y+yh;
                        if(sConfigRender==sConfigRenderDX9)
                        {
                            p.x0 -= 0.5f;
                            p.y0 -= 0.5f;
                            p.x1 -= 0.5f;
                            p.y1 -= 0.5f;
                        }
                        uint col = 0xffffffff;

                        vp[0].Init(p.x0,-(p.y0),0,col,0,0);
                        vp[1].Init(p.x1,-(p.y0),0,col,0,0);
                        vp[2].Init(p.x1,-(p.y0+1),0,col,0,0);
                        vp[3].Init(p.x0,-(p.y0+1),0,col,0,0);
                        count+=4;
                        vp+=4;
                        vp[0].Init(p.x0,-(p.y1),0,col,0,0);
                        vp[1].Init(p.x1,-(p.y1),0,col,0,0);
                        vp[2].Init(p.x1,-(p.y1-1),0,col,0,0);
                        vp[3].Init(p.x0,-(p.y1-1),0,col,0,0);
                        count+=4;
                        vp+=4;
                        vp[0].Init(p.x0,-(p.y0),0,col,0,0);
                        vp[1].Init(p.x0+1,-(p.y0),0,col,0,0);
                        vp[2].Init(p.x0+1,-(p.y1),0,col,0,0);
                        vp[3].Init(p.x0,-(p.y1),0,col,0,0);
                        count+=4;
                        vp+=4;
                        vp[0].Init(p.x1,-(p.y0),0,col,0,0);
                        vp[1].Init(p.x1-1,-(p.y0),0,col,0,0);
                        vp[2].Init(p.x1-1,-(p.y1),0,col,0,0);
                        vp[3].Init(p.x1,-(p.y1),0,col,0,0);
                        count+=4;
                        vp+=4;
                        buffer.PrintF("Perfmon: %s - ",th->GetName());
                        for(auto rr : PerfmonRecordStack)
                            buffer.AddF("%s.",rr->Section->Name);
                        buffer.AddF("%s",r->Section->Name);
                        buffer.AddF(" - %fms\n",float(1000*double(r->Time-rpop->Time)/double(freq)));
                        MouseOver = 0;
                    }
                }
            }
            y+=10;
        }
        for(int i=0;i<=PerfMonFrames && vp<vpend;i++)
        {
            float px = x + (tp.SizeX-20)*float(i)/PerfMonFrames;
            uint col = 0xffffffff;
            vp[0].Init(px,-8,0,col,0,0);
            vp[1].Init(px+1,-8,0,col,0,0);
            vp[2].Init(px+1,-y,0,col,0,0);
            vp[3].Init(px,-y,0,col,0,0);
            count+=4;
            vp+=4;
        }

        FrameBuffer.Prepend(buffer,-1);

        Geo->VB(0)->Unmap(ctx);
        if(count>0)
        {
            sDrawRange r;
            sDrawPara dp(Geo,Mtrl,CBuffer);
            dp.RangeCount = 1;
            dp.Flags |= sDF_Ranges;
            dp.Ranges = &r;
            dp.Ranges->Start = 0;
            dp.Ranges->End = count/4*6;
            ctx->Draw(dp);
        }

        FilterFramerate[(FilterIndex++)%sCOUNTOF(FilterFramerate)] = sClamp(int(0.95f+tmax*60.0f),1,10);

        PerfmonRecordStack.Clear();

        return y;
#else
    return 10;
#endif
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

const uint8 sFontData[] = 
    //ChrSet generated by ChrsetEditor
{
    255,255,255,255,255,255,255,255,
    0,0,0,0,255,0,0,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    0,0,0,0,0,0,0,0,
    32,32,32,32,32,0,32,0,
    80,80,0,0,0,0,0,0,
    80,80,248,80,248,80,80,0,
    32,120,160,112,40,240,32,0,
    192,200,16,32,64,152,24,0,
    96,144,144,96,168,144,104,0,
    32,32,64,0,0,0,0,0,
    16,32,64,64,64,32,16,0,
    64,32,16,16,16,32,64,0,
    0,80,112,248,112,80,0,0,
    0,32,32,248,32,32,0,0,
    0,0,0,0,0,32,32,64,
    0,0,0,248,0,0,0,0,
    0,0,0,0,0,0,32,0,
    8,16,16,32,32,64,64,128,
    112,136,152,168,200,136,112,0,
    48,16,16,16,16,16,16,0,
    240,8,8,16,32,64,248,0,
    240,8,8,48,8,8,240,0,
    16,48,80,144,248,16,16,0,
    248,128,128,240,8,8,240,0,
    112,128,128,240,136,136,112,0,
    248,8,16,16,32,32,32,0,
    112,136,136,112,136,136,112,0,
    112,136,136,120,8,8,112,0,
    0,0,32,0,0,32,0,0,
    0,0,32,0,0,32,32,64,
    0,16,32,64,32,16,0,0,
    0,0,248,0,248,0,0,0,
    0,64,32,16,32,64,0,0,
    240,8,8,16,32,0,32,0,
    112,136,184,168,184,128,112,0,
    112,136,136,248,136,136,136,0,
    240,136,136,240,136,136,240,0,
    120,128,128,128,128,128,120,0,
    224,144,136,136,136,144,224,0,
    248,128,128,224,128,128,248,0,
    248,128,128,224,128,128,128,0,
    120,128,128,152,136,136,120,0,
    136,136,136,248,136,136,136,0,
    112,32,32,32,32,32,112,0,
    8,8,8,8,136,136,112,0,
    136,144,160,192,160,144,136,0,
    128,128,128,128,128,128,248,0,
    136,216,168,136,136,136,136,0,
    136,200,168,152,136,136,136,0,
    112,136,136,136,136,136,112,0,
    240,136,136,240,128,128,128,0,
    112,136,136,136,168,144,104,0,
    240,136,136,240,160,144,136,0,
    120,128,128,112,8,8,240,0,
    248,32,32,32,32,32,32,0,
    136,136,136,136,136,136,112,0,
    136,136,136,136,136,80,32,0,
    136,136,136,168,168,216,136,0,
    136,136,80,32,80,136,136,0,
    136,136,136,80,32,32,32,0,
    248,8,16,32,64,128,248,0,
    48,32,32,32,32,32,48,0,
    128,64,64,32,32,16,16,8,
    96,32,32,32,32,32,96,0,
    32,80,136,0,0,0,0,0,
    0,0,0,0,0,0,0,248,
    32,32,16,0,0,0,0,0,
    0,0,112,8,120,136,120,0,
    128,128,240,136,136,136,240,0,
    0,0,120,128,128,128,120,0,
    8,8,120,136,136,136,120,0,
    0,0,112,136,248,128,120,0,
    24,32,112,32,32,32,32,0,
    0,0,120,136,136,120,8,240,
    128,128,240,136,136,136,136,0,
    32,0,32,32,32,32,32,0,
    16,0,16,16,16,16,144,96,
    64,64,72,80,96,80,72,0,
    32,32,32,32,32,32,24,0,
    0,0,240,168,168,168,136,0,
    0,0,176,200,136,136,136,0,
    0,0,112,136,136,136,112,0,
    0,0,240,136,136,240,128,128,
    0,0,120,136,136,120,8,8,
    0,0,88,96,64,64,64,0,
    0,0,120,128,112,8,240,0,
    32,32,112,32,32,32,24,0,
    0,0,136,136,136,136,112,0,
    0,0,136,136,80,80,32,0,
    0,0,136,136,168,168,80,0,
    0,0,136,80,32,80,136,0,
    0,0,136,136,80,32,32,64,
    0,0,248,16,32,64,248,0,
    24,32,32,64,32,32,24,0,
    32,32,32,32,32,32,32,0,
    192,32,32,16,32,32,192,0,
    104,176,0,0,0,0,0,0,
    168,80,168,80,168,80,168,80,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    248,136,136,136,136,136,248,0,
    0,0,0,0,0,0,0,0,
    0,32,0,32,32,32,32,32,
    32,120,160,160,120,32,0,0,
    48,72,64,224,64,64,248,0,
    136,112,136,112,136,0,0,0,
    136,80,32,112,32,32,32,0,
    32,32,32,0,32,32,32,0,
    112,136,96,144,72,48,136,112,
    80,80,0,0,0,0,0,0,
    120,132,180,164,180,132,120,0,
    56,72,88,104,0,120,0,0,
    0,40,80,160,80,40,0,0,
    248,8,0,0,0,0,0,0,
    0,0,0,248,0,0,0,0,
    120,132,180,172,180,172,132,120,
    248,0,0,0,0,0,0,0,
    64,160,64,0,0,0,0,0,
    32,32,248,32,32,0,248,0,
    96,144,32,64,240,0,0,0,
    224,16,96,16,224,0,0,0,
    16,32,0,0,0,0,0,0,
    0,0,136,136,136,216,168,128,
    120,232,232,104,40,40,40,0,
    0,0,48,48,0,0,0,0,
    0,0,0,0,0,0,32,96,
    32,96,32,32,32,0,0,0,
    48,72,72,48,0,120,0,0,
    0,160,80,40,80,160,0,0,
    64,200,80,40,80,184,8,0,
    64,200,80,56,72,144,24,0,
    192,72,48,232,80,184,8,0,
    32,0,32,64,128,136,112,0,
    64,32,112,136,248,136,136,0,
    16,32,112,136,248,136,136,0,
    32,80,112,136,248,136,136,0,
    104,176,112,136,248,136,136,0,
    80,112,136,136,248,136,136,0,
    48,80,112,136,248,136,136,0,
    60,48,80,124,144,144,156,0,
    120,128,128,128,128,128,120,32,
    64,32,248,128,224,128,248,0,
    16,32,248,128,224,128,248,0,
    32,80,248,128,224,128,248,0,
    80,0,248,128,224,128,248,0,
    64,32,112,32,32,32,112,0,
    16,32,112,32,32,32,112,0,
    32,80,112,32,32,32,112,0,
    80,0,112,32,32,32,112,0,
    96,80,72,232,72,80,96,0,
    104,176,200,168,152,136,136,0,
    64,32,112,136,136,136,112,0,
    16,32,112,136,136,136,112,0,
    32,80,112,136,136,136,112,0,
    104,176,112,136,136,136,112,0,
    80,112,136,136,136,136,112,0,
    0,136,80,32,80,136,0,0,
    120,152,152,168,200,200,240,0,
    64,32,136,136,136,136,112,0,
    16,32,136,136,136,136,112,0,
    32,80,136,136,136,136,112,0,
    80,136,136,136,136,136,112,0,
    16,32,136,80,32,32,32,0,
    128,128,240,136,136,240,128,128,
    96,144,144,176,136,136,176,0,
    64,32,112,8,120,136,120,0,
    16,32,112,8,120,136,120,0,
    32,80,112,8,120,136,120,0,
    104,176,112,8,120,136,120,0,
    80,0,112,8,120,136,120,0,
    48,80,112,8,120,136,120,0,
    0,0,216,36,124,176,236,0,
    0,0,120,128,128,128,120,32,
    64,32,112,136,248,128,120,0,
    16,32,112,136,248,128,120,0,
    32,80,112,136,248,128,120,0,
    80,0,112,136,248,128,120,0,
    64,32,0,32,32,32,32,0,
    16,32,0,32,32,32,32,0,
    32,80,0,32,32,32,32,0,
    80,0,32,32,32,32,32,0,
    80,96,16,120,136,136,112,0,
    104,176,0,176,200,136,136,0,
    64,32,0,112,136,136,112,0,
    16,32,0,112,136,136,112,0,
    32,80,0,112,136,136,112,0,
    104,176,0,112,136,136,112,0,
    80,0,112,136,136,136,112,0,
    0,32,0,248,0,32,0,0,
    0,4,120,152,168,200,112,128,
    64,32,136,136,136,136,112,0,
    16,32,136,136,136,136,112,0,
    32,80,136,136,136,136,112,0,
    80,0,136,136,136,136,112,0,
    16,32,136,136,80,32,32,64,
    128,176,200,136,136,200,176,128,
    80,0,136,136,80,32,32,64
};


/****************************************************************************/
}
