/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Main.hpp"
#include "ShaderOld.hpp"
#include "Altona2/Libs/Asl/Asl.hpp"
#include "Altona2/Libs/Asl/Tree.hpp"
#include "Altona2/Libs/Asl/Parser.hpp"
#include "Altona2/Libs/Asl/Transform.hpp"
#include "Altona2/Libs/Asl/Module.hpp"
#include "Altona2/Libs/Base/DxShaderCompiler.hpp"

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Altona2::Main()
{
    sInt flags = 0;
    //  flags |= sSM_Fullscreen;
    sRunApp(new App,sScreenMode(flags,"cube",1280,720));
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

App::App()
{
    Mtrl = 0;
    Tex = 0;
    Geo = 0;

    DPaint = 0;
    Mtrl = 0;
    Tex = 0;
    Geo = 0;
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

    const sInt sx=64;
    const sInt sy=64;
    sU32 tex[sy][sx];
    for(sInt y=0;y<sy;y++)
        for(sInt x=0;x<sx;x++)
            tex[y][x] = ((x^y)&8) ? 0xffff8080 : 0xff80ff80;
    Tex = new sResource(Adapter,sResTexPara(sRF_BGRA8888,sx,sy,1),&tex[0][0],sizeof(tex));

    Mtrl = TestAsl(Adapter,Asl::Document::GetDefaultTarget());

    if(!Mtrl)
    {
        sExit();
        return;
    }

//    Mtrl = new sMaterial(Adapter);
//    Mtrl->SetShaders(CubeShader.Get(0));
    Mtrl->SetTexturePS(0,Tex,sSamplerStatePara(sTF_Linear|sTF_Clamp,0));
    Mtrl->SetState(sRenderStatePara(sMTRL_DepthOn|sMTRL_CullOn,sMB_Off));
    Mtrl->Prepare(Adapter->FormatPNT);


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

    cbv0 = new sCBuffer<CubeShader_cbvs>(Adapter,sST_Vertex,0);

    DPaint = new sDebugPainter(Adapter);
}

void App::OnExit()
{
    delete DPaint;
    delete Mtrl;
    delete Tex;
    delete Geo;
    delete cbv0;
}

void App::OnFrame()
{
}

void App::OnPaint()
{
    sU32 utime = sGetTimeMS();
    sF32 time = utime*0.01f;
    sTargetPara tp(sTAR_ClearAll,0xff405060,Screen);
    sViewport view;

    Context->BeginTarget(tp);

    view.Camera.k.w = -4;
    view.Model = sEulerXYZ(time*0.11f,time*0.13f,time*0.15f);
    view.ZoomX = 1/tp.Aspect;
    view.ZoomY = 1;
    view.Prepare(tp);

    cbv0->Map();
    cbv0->Data->mat = view.MS2SS;
    cbv0->Data->ldir.Set(-view.MS2CS.k.x,-view.MS2CS.k.y,-view.MS2CS.k.z,0);
    cbv0->Unmap();

    Context->Draw(sDrawPara(Geo,Mtrl,cbv0));

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

sMaterial *App::TestAsl(sAdapter *ada,sTarget tar)
{
    sTextBuffer log;

    Asl::Document doc;
    doc.ParseFile("Shader.txt");
    auto *aslm = doc.FindMaterial("TestShader");
    if(!aslm->ConfigureModules(0,tar))
        return 0;
    if(!aslm->Transform())
        return 0;
    aslm->Tree->CppLine = false;
    aslm->Tree->Dump(log,1<<sST_Vertex,tar);
    aslm->Tree->Dump(log,1<<sST_Pixel,tar);
    auto mtrl = aslm->CompileMaterial(ada);
    if(!aslm->WriteHeader(&log))
        return 0;

    aslm = doc.FindMaterial("sFixedShaderFlat");
    if(!aslm->ConfigureModules(0,tar))
        return 0;
    sDPrint(log.Get());

    return mtrl;
}


/****************************************************************************/

