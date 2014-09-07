/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "main.hpp"

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Altona2::Main()
{
    sRunApp(new App,sScreenMode(sSM_Fullscreen*0,"dragdata",1280,720));
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

App::App()
{
    Mtrl = 0;
    Geo = 0;
    DPaint = 0;
    cbv0 = 0;

    DragActive = 0;
    StartX = StartY = 0;
    EndX = EndY = 0;

    MaxVertex = 0x4000;
}

App::~App()
{
}

void App::OnInit()
{
    Screen = sGetScreen();
    Adapter = Screen->Adapter;
    Context = Adapter->ImmediateContext;

    DPaint = new sDebugPainter(Adapter);

    static const sU32 vfdesc[] =
    {
        sVF_Position,
        sVF_Color0,
        sVF_End,
    };

    Format = Adapter->CreateVertexFormat(vfdesc);

    Mtrl = new sFixedMaterial(Adapter);
    Mtrl->SetState(sRenderStatePara(sMTRL_DepthOn|sMTRL_CullOff,sMB_Alpha));
    Mtrl->Prepare(Format);

    Geo = new sGeometry(Adapter);
    Geo->SetVertex(new sResource(Adapter,sResBufferPara(sRBM_Vertex|sRU_MapWrite,sizeof(Vertex),MaxVertex)),0);
    Geo->Prepare(Format,sGMP_Tris,0,0,MaxVertex,0);

    cbv0 = new sCBuffer<sFixedMaterialVC>(Adapter,sST_Vertex,0);
}

void App::OnExit()
{
    delete Mtrl;
    delete Geo;
    delete cbv0;
    delete DPaint;
}

void App::OnFrame()
{
}

void App::OnPaint()
{
    sInt time = sGetTimeMS();
    sTargetPara tp(sTAR_ClearAll,0xff405060,Screen);
    sViewport view;

    Context->BeginTarget(tp);

    view.Mode = sVM_Pixels;
    view.Prepare(tp);

    cbv0->Map();
    cbv0->Data->Set(view);
    cbv0->Unmap();

    // draw

    sInt vc = 0;
    Vertex *vp;

    Geo->VB(0)->MapBuffer(&vp,sRMM_Discard);
    for(auto &cl : Clicks)
    {
        sF32 f = 1.0f-(time-cl.Time)*0.002f;
        sF32 s = 50.0f*(1-f);
        sF32 x = sF32(cl.ScreenX);
        sF32 y = sF32(cl.ScreenY);
        if(vc+3<=MaxVertex && f>0)
        {
            if(!cl.Make) s *= -1;
            sU32 col = cl.Color;
            col += (sU32(f*255)<<24);
            vp[vc+0].Init(x-s,y-s*0.5f,0.5f,col);
            vp[vc+1].Init(x+s,y-s*0.5f,0.5f,col);
            vp[vc+2].Init(x  ,y+s     ,0.5f,col);

            vc += 3;
        }
        else
        {
            cl.Color = 0;
        }
    }
    if(DragActive)
    {
        sU32 col = 0xffffffff;

        sFRect r;
        r.Init(sF32(StartX),sF32(StartY),sF32(EndX),sF32(EndY));
        sVector2 tn = sNormalize(sVector2(r.SizeY(),-r.SizeX()));

        vp[vc+0].Init(r.x0+tn.x,r.y0+tn.y,0.5f,col);  // 00
        vp[vc+1].Init(r.x0-tn.x,r.y0-tn.y,0.5f,col);  // 01
        vp[vc+2].Init(r.x1-tn.x,r.y1-tn.y,0.5f,col);  // 11
        vp[vc+3].Init(r.x0+tn.x,r.y0+tn.y,0.5f,col);  // 00
        vp[vc+4].Init(r.x1-tn.x,r.y1-tn.y,0.5f,col);  // 11
        vp[vc+5].Init(r.x1+tn.x,r.y1+tn.y,0.5f,col);  // 10
        vc+=6;
    }
    Geo->VB(0)->Unmap();

    if(vc>0)
    {
        sDrawPara dp(Geo,Mtrl,cbv0);
        sDrawRange dr;
        dr.Start = 0;
        dr.End = vc;
        dp.Flags |= sDF_Ranges;
        dp.RangeCount = 1;
        dp.Ranges = &dr;

        Context->Draw(dp);
    }

    // remove unused clicks

    sInt max = Clicks.GetCount();
    for(sInt i=0;i<max;)
    {
        if(Clicks[i].Color == 0)
            Clicks[i] = Clicks[--max];
        else
            i++;
    }
    Clicks.SetSize(max);

    // debug

    DPaint->PrintFPS();
    DPaint->PrintStats();
    DPaint->Draw(tp);

    Context->EndTarget();
}

void App::OnKey(const sKeyData &kd)
{
    sU32 mkey = kd.Key & sKEYQ_Mask;
    if(mkey==27)
        sExit();
}

void App::OnDrag(const sDragData &dd)
{
    sU32 col = 0xffffff;
    if(dd.Buttons==sMB_Left) col = 0x00ff00;
    if(dd.Buttons==sMB_Right) col = 0xff0000;
    if(dd.Buttons==sMB_Middle) col = 0x0000ff;
    if(dd.Mode==sDM_Start)
    {
        Click *c = Clicks.Add();
        c->Make = 1;
        c->ScreenX = dd.PosX;
        c->ScreenY = dd.PosY;
        c->Time = sGetTimeMS();
        c->Color = col;
    }
    if(dd.Mode==sDM_Stop)
    {
        Click *c = Clicks.Add();
        c->Make = 0;
        c->ScreenX = dd.PosX;
        c->ScreenY = dd.PosY;
        c->Time = sGetTimeMS();
        c->Color = col;
    }
    if(dd.Mode==sDM_Drag)
    {
        DragActive = 1;
        StartX = dd.StartX;
        StartY = dd.StartY;
        EndX = dd.PosX;
        EndY = dd.PosY;
    }
    else
    {
        DragActive = 0;
    }
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/

