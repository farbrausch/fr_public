/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "main.hpp"
#include "shader.hpp"
#include "altona2/libs/util/image.hpp"

using namespace Altona2;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Altona2::Main()
{
  sInt flags = 0;
//  flags |= sSM_Fullscreen;
  sRunApp(new App,sScreenMode(flags,"texture compression",1280,720));
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

  if(sConfigPlatform!=sConfigPlatformIOS)
  {
    sImage img(sx,sy);
    for(sInt y=0;y<sy;y++)
    {
      for(sInt x=0;x<sx;x++)
      {
        sInt b = x*255/sx;
        sInt g = y*255/sy;
        sInt r = (((x*255/sx)-128) * ((y*255/sy)-128))/129+128;
        img.GetData()[y*sx+x] = 0xff000000 | (r<<16) | (g<<8) | b;
      }
    }

    sPrepareTexture2D prep;
    prep.Load(&img);
    prep.Mipmaps = 1;
    prep.Format = sRF_BC1;
    prep.Process();
    sSaveFile("texture_bc1",prep.GetData(),prep.GetSize());

    prep.Load(&img);
    prep.Mipmaps = 1;
    prep.Format = sRF_PVR4;
    prep.Process();
    sSaveFile("texture_pvr4",prep.GetData(),prep.GetSize());
  }

  if(0)
  {
    sAddPackfile("packfile.pak");
  }

  if(sConfigPlatform==sConfigPlatformIOS)
  {
    sPtr texsize;
    sU8 *texdata = sLoadFile("texture_pvr4",texsize);
    Tex = new sResource(Adapter,sResTexPara(sRF_PVR4,sx,sy,1),texdata,texsize);
    delete[] texdata;
  }
  else
  {
    sPtr texsize;
    sU8 *texdata = sLoadFile("texture_bc1",texsize);
    Tex = new sResource(Adapter,sResTexPara(sRF_BC1,sx,sy,1),texdata,texsize);
    delete[] texdata;
  }
  
  Mtrl = new sMaterial(Adapter);
  Mtrl->SetShaders(CubeShader.Get(0));
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
  Geo->Prepare(Adapter->FormatPNT,sGMP_Tris|sGMF_Index16,ic,0,vc,0);

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
  cbv0->Data->ldir.Set(-view.MS2CS.k.x,-view.MS2CS.k.y,-view.MS2CS.k.z);
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

/****************************************************************************/

