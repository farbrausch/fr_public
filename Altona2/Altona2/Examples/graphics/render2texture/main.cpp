/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "main.hpp"
#include "altona2/libs/util/graphicshelper.hpp"

using namespace Altona2;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Altona2::Main()
{
    sRunApp(new App,sScreenMode(sSM_Fullscreen*0,"sFixedMaterial",1280,720));
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

App::App()
{
    Mtrl = 0;
    MtrlRt = 0;
    Tex = 0;
    TexRt = 0;
    Geo = 0;
    DPaint = 0;
    cbv0 = 0;
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

    const sInt sx=64;
    const sInt sy=64;
    sU32 tex[sy][sx];
    for(sInt y=0;y<sy;y++)
        for(sInt x=0;x<sx;x++)
            tex[y][x] = ((x^y)&8) ? 0xffffffff : 0xff808080;
    Tex = new sResource(Adapter,sResTexPara(sRF_BGRA8888,sx,sy,1),&tex[0][0],sizeof(tex));

    Mtrl = new sFixedMaterial(Adapter);
    Mtrl->SetTexturePS(0,Tex,sSamplerStatePara(sTF_Linear|sTF_Clamp,0));
    Mtrl->SetState(sRenderStatePara(sMTRL_DepthOn|sMTRL_CullOn,sMB_Off));
    Mtrl->Prepare(Adapter->FormatPNT);

    sResTexPara tpc(Adapter->GetDefaultRTColorFormat(),192,192,1);
    tpc.Mode = sRBM_ColorTarget|sRBM_Shader|sRU_Gpu|sRM_Texture;
    TexRt = new sResource(Adapter,tpc,0,0);
    
    sResTexPara tpd(Adapter->GetDefaultRTDepthFormat(),192,192,1);
    tpd.Mode = sRBM_DepthTarget|sRU_Gpu;
    if (Adapter->IsDefaultRTDepthUsableInShader())  //Check if we could use DepthTexture as normal texture within Shader to read from
        tpd.Mode |= sRBM_Shader|sRM_Texture;

    TexDepth = new sResource(Adapter,tpd,0,0);

    MtrlRt = new sFixedMaterial(Adapter);
    MtrlRt->SetTexturePS(0,TexRt,sSamplerStatePara(sTF_Point|sTF_Clamp,0));
    MtrlRt->SetState(sRenderStatePara(sMTRL_DepthOn|sMTRL_CullOn,sMB_Off));
    MtrlRt->Prepare(Adapter->FormatPNT);

    MtrlDepth = new sFixedMaterial(Adapter);
    if (Adapter->IsDefaultRTDepthUsableInShader())
        MtrlDepth->SetTexturePS(0,TexDepth,sSamplerStatePara(sTF_Point|sTF_Clamp,0));
    else
        MtrlDepth->SetTexturePS(0,TexRt,sSamplerStatePara(sTF_Point|sTF_Clamp,0));

    MtrlDepth->SetState(sRenderStatePara(sMTRL_DepthOn|sMTRL_CullOn,sMB_Off));
    MtrlDepth->Prepare(Adapter->FormatPNT);


    const sInt ic = 6*6;
    const sInt vc = 24;
    static const sU16 ib[ic] =
    {
        0, 1, 2, 0, 2, 3,
        4, 5, 6, 4, 6, 7,
        8, 9,10, 8,10,11,

        12,13,14,12,14,15,
        16,17,18,16,18,19,
        20,21,22,20,22,23,
    };
    static const sVertexPNT vb[vc] =
    {
        { -1, 1,-1,   0, 0,-1, 0,0, },
        {  1, 1,-1,   0, 0,-1, 1,0, },
        {  1,-1,-1,   0, 0,-1, 1,1, },
        { -1,-1,-1,   0, 0,-1, 0,1, },

        { -1,-1, 1,   0, 0, 1, 0,0, },
        {  1,-1, 1,   0, 0, 1, 1,0, },
        {  1, 1, 1,   0, 0, 1, 1,1, },
        { -1, 1, 1,   0, 0, 1, 0,1, },

        { -1,-1,-1,   0,-1, 0, 0,0, },
        {  1,-1,-1,   0,-1, 0, 1,0, },
        {  1,-1, 1,   0,-1, 0, 1,1, },
        { -1,-1, 1,   0,-1, 0, 0,1, },

        {  1,-1,-1,   1, 0, 0, 0,0, },
        {  1, 1,-1,   1, 0, 0, 1,0, },
        {  1, 1, 1,   1, 0, 0, 1,1, },
        {  1,-1, 1,   1, 0, 0, 0,1, },

        {  1, 1,-1,   0, 1, 0, 0,0, },
        { -1, 1,-1,   0, 1, 0, 1,0, },
        { -1, 1, 1,   0, 1, 0, 1,1, },
        {  1, 1, 1,   0, 1, 0, 0,1, },

        { -1, 1,-1,  -1, 0, 0, 0,0, },
        { -1,-1,-1,  -1, 0, 0, 1,0, },
        { -1,-1, 1,  -1, 0, 0, 1,1, },
        { -1, 1, 1,  -1, 0, 0, 0,1, },
    };

    Geo = new sGeometry(Adapter);
    Geo->SetIndex(sResBufferPara(sRBM_Index|sRU_Static,sizeof(sU16),ic),ib);
    Geo->SetVertex(sResBufferPara(sRBM_Vertex|sRU_Static,sizeof(sVertexPNT),vc),vb);
    Geo->Prepare(Adapter->FormatPNT,sGMP_Tris|sGMF_Index16);

    cbv0 = new sCBuffer<sFixedMaterialLightVC>(Adapter,sST_Vertex,0);
}

void App::OnExit()
{
    delete Mtrl;
    delete Tex;
    delete MtrlRt;
    delete MtrlDepth;
    delete TexRt;
    delete TexDepth;
    delete Geo;
    delete cbv0;
    delete DPaint;
}

void App::OnFrame()
{
}

void App::OnPaint()
{
    sF32 time = sGetTimeMS()*0.001f;
    sTargetPara tp(sTAR_ClearAll,0xff405060,Screen);
    sViewport view;
    sFixedMaterialLightPara lp;

    // render to texture

    view.Camera.k.w = -6;
    view.ZFar = 12.0f;
    view.ZNear = 3.0f;
  
    view.Model = sEulerXYZ(time*0.11f,time*0.13f,time*0.15f);
    view.Model.SetTrans(sEulerXYZ(time*0.61f,0.0f,0.0f) * sVector41(0.0f, 0.0f, 2.0f));    
    view.ZoomX = 2;
    view.ZoomY = 2;
    view.Prepare(tp);
    lp.LightDirWS[0].Set(0,0,-1);
    lp.LightColor[0] = 0xff8080;
    lp.AmbientColor = 0x202020;

    Context->BeginTarget(sTargetPara(sTAR_ClearAll,0xff605040,TexRt,TexDepth));

    cbv0->Map();
    cbv0->Data->Set(view,lp);
    cbv0->Unmap();

    Context->Draw(sDrawPara(Geo,Mtrl,cbv0));

    view.Model = sEulerXYZ(time*0.21f,-time*0.13f,-time*0.05f);
    view.Model.SetTrans(sEulerXYZ(time*0.61f,0.0f,0.0f) * sVector41(0.0f, 0.0f, -2.0f));    
    view.Prepare(tp);
    lp.LightColor[0] = 0x80ff80;

    cbv0->Map();
    cbv0->Data->Set(view,lp);
    cbv0->Unmap();

    Context->Draw(sDrawPara(Geo,Mtrl,cbv0));


    Context->EndTarget();

    // render cube with rendertarget color

    Context->BeginTarget(tp);

    view.Camera.k.w = -6;    
    view.Model = sEulerXYZ(time*0.11f,time*0.13f,time*0.15f);
    view.Model.SetTrans(sVector41(2.0f, 0.0f, 0.0f));    
    view.ZoomX = 2.0f/tp.Aspect;
    view.ZoomY = 2.0f;
    view.Prepare(tp);
    lp.LightDirWS[0].Set(0,0,-1);
    lp.LightColor[0] = 0xffffff;
    lp.AmbientColor = 0x202020;

    cbv0->Map();
    cbv0->Data->Set(view,lp);
    cbv0->Unmap();

    Context->Draw(sDrawPara(Geo,MtrlRt,cbv0));


    // render cube with rendertarget color

  
    view.Camera.k.w = -6;    
    view.Model = sEulerXYZ(time*0.11f,time*0.13f,time*0.15f);
    view.Model.SetTrans(sVector41(-2.0f, 0.0f, 0.0f));    
    view.Prepare(tp);
    cbv0->Map();
    cbv0->Data->Set(view,lp);
    cbv0->Unmap();

    Context->Draw(sDrawPara(Geo,MtrlDepth,cbv0));


    // done

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
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/

