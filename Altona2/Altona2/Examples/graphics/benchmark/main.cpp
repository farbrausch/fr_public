/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "main.hpp"
#include "shader.hpp"
#include "altona2/libs/util/graphicshelper.hpp"

using namespace Altona2;


#if sConfigPlatform==sConfigPlatformIOS
#define SLOW 1
#else
#define SLOW 0
#endif

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Altona2::Main()
{
  sInt flags = 0;
//  flags |= sSM_Fullscreen;
  sRunApp(new App,sScreenMode(flags,"benchmark",1280,720));
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

App::App()
{
  CubeMtrl = 0;
  Tex = 0;
  CubeGeo = 0;
  StatGeo = 0;
  DynGeo = 0;
  Benchmark = 0;
  BenchValue = 0;
  sEnablePerfMon(1);
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

  CubeMtrl = new sMaterial(Adapter);
  CubeMtrl->SetShaders(CubeShader.Get(0));
  CubeMtrl->SetTexturePS(0,Tex,sSamplerStatePara(sTF_Linear|sTF_Clamp,0));
  CubeMtrl->SetState(sRenderStatePara(sMTRL_DepthOn|sMTRL_CullOn,sMB_Off));
  CubeMtrl->Prepare(Adapter->FormatPCT);

  FlatMtrl = new sFixedMaterial(Adapter);
  FlatMtrl->SetTexturePS(0,Tex,sSamplerStatePara(sTF_Linear|sTF_Clamp,0));
  FlatMtrl->SetState(sRenderStatePara(sMTRL_DepthOn|sMTRL_CullOff,sMB_Off));
  FlatMtrl->Prepare(Adapter->FormatPCT);

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
  struct vert
  {
    sF32 px,py,pz;
    sF32 nx,ny,nz;
    sF32 u0,v0;
  };
  static const vert vb[vc] =
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

  CubeGeo = new sGeometry(Adapter);
  CubeGeo->SetIndex(sResBufferPara(sRBM_Index|sRU_Static,sizeof(sU16),ic),ib);
  CubeGeo->SetVertex(sResBufferPara(sRBM_Vertex|sRU_Static,sizeof(vert),vc),vb);
  CubeGeo->Prepare(Adapter->FormatPNT,sGMP_Tris|sGMF_Index16,ic,0,vc,0);

  cbv0 = new sCBuffer<CubeShader_cbvs>(Adapter,sST_Vertex,0);

  DPaint = new sDebugPainter(Adapter);
  sGpuPerfMon = new sPerfMonGpu(Adapter,"GPU");

  {
    sInt vc = 0x8000;
    sInt ic = 0x18000;

    DynGeo = new sGeometry();
    DynGeo->SetIndex(new sResource(Adapter,sResBufferPara(sRBM_Index|sRU_MapWrite,sizeof(sU16),ic),0,0),1);
    DynGeo->SetVertex(new sResource(Adapter,sResBufferPara(sRBM_Vertex|sRU_MapWrite,sizeof(sVertexPCT),vc),0,0),0,1);
    DynGeo->Prepare(Adapter->FormatPCT,sGMP_Tris|sGMF_Index16,ic,0,vc,0);

    StatGeoBenchmark = -1;
    StatGeoBenchValue = 0;
  }
}

void App::OnExit()
{
  delete DPaint;
  delete CubeMtrl;
  delete Tex;
  delete CubeGeo;
  delete cbv0;
  delete DynGeo;
  delete StatGeo;
  delete FlatMtrl;
}

void App::OnFrame()
{
}

void App::OnPaint()
{
  sZONE("OnPaint",0xff00ff00);
  sGPU_ZONE("OnPaint",0xff00ff00);
  sTargetPara tp(sTAR_ClearAll,0xff405060,Screen);

  sInt sx = tp.SizeX;
  sInt sy = tp.SizeY;
  const sInt h = 100;
  for(sInt i=0;i<MaxBench;i++)
    BenchRect[i].Init(sx*(i+0)/MaxBench,sy-h,sx*(i+1)/MaxBench,sy);
  for(sInt i=0;i<MaxValue;i++)
    ValueRect[i].Init(sx*(i+0)/MaxValue,0,sx*(i+1)/MaxValue,h);

  Context->BeginTarget(tp);

  {
    sGPU_ZONE("Benchmark",0xff008000);
    switch(Benchmark)
    {
    case 0: Bench1(tp); break;
    case 1: Bench2_3(tp,0); break;
    case 2: Bench2_3(tp,1); break;
    case 3: Bench4(tp); break;

    default: break;
    }
  }

  static const sChar *BenchmarkNames[] =
  {
    "F1 - Cubes",
    "F2 - Dynamic Quads",
    "F3 - Static Quads",
    "F4 - CPU Burning",
    "F5 - ???",
    "F6 - ???",
    "F7 - ???",
    "F8 - ???",
    "F9 - ???",
    "F10 - ???",
    "F11 - ???",
    "F12 - ???",
  };

  {
    sGPU_ZONE("sDebugPainter",0xff404040);
    DPaint->PrintPerfMon();
    DPaint->PrintFPS();
    DPaint->PrintStats();
    DPaint->FramePrintF("F1..F12 : Benchmark [%s]\n",BenchmarkNames[Benchmark]);
    DPaint->FramePrintF("0..9 : Value [%d]\n",BenchValue);
  }

  DPaint->Draw(tp);

  Context->EndTarget();
}

void App::OnKey(const sKeyData &kd)
{
  sU32 key = sCleanKeyCode(kd.Key);
  switch(key)
  {
  case sKEY_Escape:
  case sKEY_Escape | sKEYQ_Shift:
    sExit();

  case sKEY_F1: 
  case sKEY_F2:
  case sKEY_F3:
  case sKEY_F4:
  case sKEY_F5:
  case sKEY_F6:
  case sKEY_F7:
  case sKEY_F8:
  case sKEY_F9:
  case sKEY_F10:
  case sKEY_F11:
  case sKEY_F12:
    Benchmark = key-sKEY_F1;
    break;
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    BenchValue = key-'0';
    break;
  case sKEY_Pause:
    sEnablePerfMon(!sPerfMonEnabled());
    break;
  }
}

void App::OnDrag(const sDragData &dd)
{
  if(dd.Mode==sDM_Start)
  {
    for(sInt i=0;i<MaxBench;i++)
      if(BenchRect[i].Hit(dd.StartX,dd.StartY))
        Benchmark = i;
    for(sInt i=0;i<MaxValue;i++)
      if(ValueRect[i].Hit(dd.StartX,dd.StartY))
        BenchValue = i;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   The Benchmarks                                                     ***/
/***                                                                      ***/
/****************************************************************************/

void App::Bench1(const sTargetPara &tp)
{
  sU32 utime = sGetTimeMS();
  sF32 time = utime*0.001f;
  sViewport view;

  sInt max = (SLOW?BenchValue:BenchValue*4) + 1;
  sF32 s = 2.5f;
  sF32 b = - ((max-1)/2.0f*2.5f);

  for(sInt z=0;z<max;z++)
  {
    for(sInt y=0;y<max;y++)
    {
      for(sInt x=0;x<max;x++)
      {
        sVector41 pos;
        pos.Set(sF32(x)*s+b,sF32(y)*s+b,sF32(z)*s+b);
        view.Camera = sEulerXYZ(time*0.11f,time*0.13f,time*0.15f);
        sVector40 v = sBaseZ(view.Camera);
        view.Camera.SetTrans(sVector41(v * (-4.0f*max)));
        view.Model.SetTrans(pos);
        view.ZoomX = 1/tp.Aspect;
        view.ZoomY = 1;
        view.Prepare(tp);

        cbv0->Map();
        cbv0->Data->mat = view.MS2SS;
        cbv0->Data->ldir.Set(-view.MS2CS.k.x,-view.MS2CS.k.y,-view.MS2CS.k.z);
        cbv0->Unmap();

        Context->Draw(sDrawPara(CubeGeo,CubeMtrl,cbv0));
      }
    }
  }
}

/****************************************************************************/

void App::Bench2_3(const sTargetPara &tp,sInt stat)
{
  sBool update = (StatGeoBenchmark!=Benchmark || StatGeoBenchValue!=BenchValue);

  sInt max = sMax(8,BenchValue*8);


  sViewport view;

  view.Camera.k.w = -0.7f*max;
  view.ZoomX = 1/tp.Aspect;
  view.ZoomY = 1;
  view.Prepare(tp);

  cbv0->Map();
  cbv0->Data->mat = view.MS2SS;
  cbv0->Data->ldir.Set(-view.MS2CS.i.z,-view.MS2CS.j.z,-view.MS2CS.k.z);
  cbv0->Unmap();
/*
  for(sInt i=0;i<(SLOW ? 1 : 100);i++)
  {
    if(update || !stat)
    {
      sU16 *ip;
      sVertexPCT *vp;
      
      DynGeo->VB(0)->MapBuffer(&vp,0);
      sF32 ox = -sF32(max)/2.0f;
      sF32 oy = -sF32(max)/2.0f;
      sF32 sx = 1.0f;
      sF32 sy = 1.0f;
      for(sInt y=0;y<max;y++)
      {
        for(sInt x=0;x<max;x++)
        {
          sF32 u = ((x^y)&1) ? 1/16.0f : 3/16.0f;
          sF32 v = 1/16.0f;
          vp[0].Init((x+0)*sx+ox,(y+0)*sy+oy,0,~0U,u,v);
          vp[1].Init((x+0)*sx+ox,(y+1)*sy+oy,0,~0U,u,v);
          vp[2].Init((x+1)*sx+ox,(y+1)*sy+oy,0,~0U,u,v);
          vp[3].Init((x+1)*sx+ox,(y+0)*sy+oy,0,~0U,u,v);
          vp+=4;
        }
      }
      DynGeo->VB(0)->Unmap();

      DynGeo->IB()->MapBuffer(&ip,0);
      for(sInt y=0;y<max;y++)
        for(sInt x=0;x<max;x++)
          sQuad(ip,(y*max+x)*4,0,1,2,3);
      DynGeo->IB()->Unmap();
    }

    sDrawPara dp(DynGeo,FlatMtrl,cbv0);
    sDrawRange dr(0,max*max*6);
    dp.Ranges = &dr;
    dp.RangeCount = 1;
    dp.Flags |= sDF_Ranges;
    Context->Draw(dp);
  }
  */
}

/****************************************************************************/

void App::Bench4(const sTargetPara &tp)
{
  sU32 code;
 
  code = sGetTimeMS();
  for(sU64 i=0;i<((SLOW ? 0x100 : 0x4000ULL)<<BenchValue);i++)
  {
    code = sMulDiv(code,31,15);
  }

  DPaint->FramePrintF("\n\n%08x\n",code);
}

/****************************************************************************/

