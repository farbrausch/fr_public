/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "main.hpp"
#include "shader.hpp"

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Altona2::Main()
{
    sRunApp(new App,sScreenMode(0,"Create Vertex & Index Buffer in Compute Shader",1280,720));
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

App::App()
{
    DPaint = 0;
    cbv0 = 0;
    BigVB = 0;
    BigIB = 0;
    CountVC = 0;
    CountIC = 0;
    CShader = 0;
    CMtrl = 0;
    Csb0 = 0;
    IndirectShader = 0;
    IndirectBuffer = 0;
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

    // compute

    BigVB = Adapter->CreateRawRW(64*1024*1024,sRBM_Vertex);
    BigIB = Adapter->CreateRawRW(16*1024*1024,sRBM_Index);
    CountVC = Adapter->CreateCounterRW(1,4,0);
    CountIC = Adapter->CreateCounterRW(1,4,0);

    CShader = Adapter->CreateShader(ComputeShader.Get(0),sST_Compute);
    Csb0 = new sCBuffer<ComputeShader_csb0>(Adapter,sST_Compute,0);

    // geometry

    CGeo = new sGeometry(Adapter);
    CGeo->SetVertex(BigVB,0,0);
    CGeo->SetIndex(BigIB,0);
    CGeo->Prepare(Adapter->FormatP,sGMF_Index32|sGMP_Tris);

    CMtrl = new sFixedMaterial(Adapter);
    CMtrl->SetState(sRenderStatePara(sMTRL_DepthOn|sMTRL_CullOff,sMB_Off));
    CMtrl->Prepare(Adapter->FormatP);

    cbv0 = new sCBuffer<sFixedMaterialLightVC>(Adapter,sST_Vertex,0);

    // indirect compute

    IndirectBuffer = Adapter->CreateBufferRW(5,sRFB_32|sRFC_R|sRFT_UInt,sRM_DrawIndirect);
    IndirectShader = Adapter->CreateShader(WriteCount.Get(0),sST_Compute);

}

void App::OnExit()
{
    delete cbv0;
    delete DPaint;
    delete BigVB;
    delete BigIB;
    delete CountVC;
    delete CountIC;
    delete CGeo;
    delete CMtrl;
    delete Csb0;
    delete IndirectBuffer;
}

void App::OnFrame()
{
}

void App::OnPaint()
{
    // matrix

    sScreenMode mode;
    Screen->GetMode(mode);
    sTargetPara tp(sTAR_ClearAll,0xff405060,Screen);

    sF32 time = sGetTimeMS()*0.001f;
    sViewport view;
    view.Camera.k.w = -4;
    view.Model = sEulerXYZ(time*0.11f,time*0.13f,time*0.15f);
    view.ZoomX = 1/tp.Aspect;
    view.ZoomY = 1;
    view.Prepare(tp);

    // compute

    {
        sDispatchContext dc;
        dc.SetUav(0,BigVB);
        dc.SetUav(1,BigIB);
        dc.SetUav(2,CountVC,0);
        dc.SetUav(3,CountIC,0);
        dc.SetCBuffer(Csb0);


        Csb0->Map(Context);
        Csb0->Data->Matrix = sEulerXYZ(-time*0.21f,-time*0.23f,-time*0.25f);
        Csb0->Data->PosMul.x = 0.25f + (sSin(time*1.12f)+1)*0.125f;
        Csb0->Data->PosMul.y = 0.25f + (sSin(time*1.12f)+1)*0.125f;
        Csb0->Data->PosMul.z = 0.25f + (sSin(time*1.12f)+1)*0.125f;
        Csb0->Data->PosAdd.x = -(16-1)*0.5f;
        Csb0->Data->PosAdd.y = -(16-1)*0.5f;
        Csb0->Data->PosAdd.z = -(16-1)*0.5f;
        Csb0->Data->tx.Set(view.MS2CS.i.x,view.MS2CS.i.y,view.MS2CS.i.z);
        Csb0->Data->ty.Set(view.MS2CS.j.x,view.MS2CS.j.y,view.MS2CS.j.z);
        Csb0->Data->Size = 0.06125f;
        Csb0->Unmap();

        Context->BeginDispatch(&dc);
        Context->Dispatch(CShader,4,4,4);
        Context->EndDispatch(&dc);
    }

    // copy index count to indirect buffer

    {
        sDispatchContext dc;
        dc.SetUav(0,CountIC);
        dc.SetUav(1,IndirectBuffer);

        Context->BeginDispatch(&dc);
        Context->Dispatch(IndirectShader,1,1,1);
        Context->EndDispatch(&dc);
    }

    // render

    Context->BeginTarget(tp);

    sFixedMaterialLightPara lp;
    lp.LightDirWS[0].Set(0,0,-1);
    lp.LightColor[0] = 0xffffff;
    lp.AmbientColor = 0x202020;

    cbv0->Map();
    cbv0->Data->Set(view,lp);
    cbv0->Unmap();

    sDrawPara dp(CGeo,CMtrl,cbv0);
    dp.IndirectBuffer = IndirectBuffer;
    dp.IndirectBufferOffset = 0;
    dp.Flags |= sDF_Indirect2;
    Context->Draw(dp);

    // debug

    DPaint->PrintFPS();
    DPaint->PrintStats();
    DPaint->Draw(tp);

    Context->EndTarget();

}

void App::OnKey(const sKeyData &kd)
{
    if((kd.Key&(sKEYQ_Mask|sKEYQ_Break))==27)
        sExit();
}

void App::OnDrag(const sDragData &dd)
{
}

/****************************************************************************/

