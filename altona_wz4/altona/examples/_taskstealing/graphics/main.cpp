/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "main.hpp"

/****************************************************************************/

#include "main.hpp"
#include "base/windows.hpp"
#include "shaders.hpp"
#include "util/image.hpp"
#include "util/shaders.hpp"
#include "util/taskscheduler.hpp"

/****************************************************************************/

// constructor

MyApp::MyApp()
{

  // painter: debug out in 2d

  Painter = new sPainter;
  Timer.SetHistoryMax(16);

  // scheduler

  sAddSched();
  ThreadCount = sSched->GetThreadCount();
  ThreadDatas = new ThreadDataType[ThreadCount];
  for(sInt i=0;i<ThreadCount;i++)
    ThreadDatas[i].Ctx = new sGfxThreadContext;
  sSchedMon = new sStsPerfMon;
  sSchedMon->Scale = 17;

  Mode = 2;
  EndGame = 0;
  Granularity = 0;

  // format

  static const sU32 desc[] =
  {
    sVF_STREAM0 | sVF_POSITION,
    sVF_STREAM0 | sVF_NORMAL,
    sVF_STREAM0 | sVF_UV0,

    sVF_STREAM1 | sVF_UV4 | sVF_F4 | sVF_INSTANCEDATA,
    sVF_STREAM1 | sVF_UV5 | sVF_F4 | sVF_INSTANCEDATA,
    sVF_STREAM1 | sVF_UV6 | sVF_F4 | sVF_INSTANCEDATA,
    sVF_END,
  };

  Format = sCreateVertexFormat(desc);


  // create dummy texture and material

  sImage img;
  img.Init(32,32);
  img.Fill(0xffc0c0c0);
//  img.Checker(0xffff4040,0xff40ff40,4,4);
  img.Glow(0.5f,0.5f,0.25f,0.25f,0xffc0c0c0,1.0f,4.0f);

  Tex = sLoadTexture2D(&img,sTEX_ARGB8888);

  Mtrl = new MaterialFlat;
  Mtrl->Texture[0] = Tex;
  Mtrl->Flags = sMTRL_ZON|sMTRL_CULLON;
  Mtrl->TFlags[0] = sMTF_CLAMP|sMTF_LEVEL2;
  Mtrl->Prepare(Format);

  // the cube geometry

  Geo = new sGeometry(sGF_TRILIST|sGF_INDEX16,Format);
  const sF32 s = 0.2f;
  Geo->LoadCube(-1,s,s,s);

  // Elements;

  sInt n = 32;
  Elements.AddMany(n*n*n);
  Element *e;
  sRandom rnd;
  sFORALL(Elements,e)
  {
    e->Pos.x = (rnd.Float(1)-0.5f)*7;
    e->Pos.y = (rnd.Float(1)-0.5f)*7;
    e->Pos.z = (rnd.Float(1)-0.5f)*7;

    e->Pos.x = (_i%n-(n*0.5-0.5))*0.5f;
    e->Pos.y = ((_i/n)%n-(n*0.5-0.5))*0.5f;
    e->Pos.z = ((_i/n/n)%n-(n*0.5-0.5))*0.5f;
    e->Rot.x = rnd.Float(1);
    e->Rot.y = rnd.Float(1);
    e->Rot.z = rnd.Float(1);
    e->RotSpeed.x = rnd.Float(1);
    e->RotSpeed.y = rnd.Float(1);
    e->RotSpeed.z = rnd.Float(1);
    e->Matrix.Init();
    e->CubeTex = 0;
  }
/*
  Elements[0].Pos.Init(0,0,0);
  Elements[0].Rot.Init(0,0,0);
  Elements[0].RotSpeed.Init(0,0,0);
  */
}

/****************************************************************************/

// destructor

MyApp::~MyApp()
{
  delete Painter;
  delete Geo;
  delete Mtrl;
  delete Tex;
  for(sInt i=0;i<ThreadCount;i++)
    delete ThreadDatas[i].Ctx;
  delete[] ThreadDatas;
}

/****************************************************************************/

void ThreadCode(sStsManager *man,sStsThread *th,sInt start,sInt count,void *data)
{
  ((MyApp *) data)->ThreadCode(man,th,start,count);
}

void MyApp::ThreadCode(sStsManager *man,sStsThread *th,sInt start,sInt count)
{
  sSchedMon->Begin(th->GetIndex(),0x0000ff);
  ThreadDatas[th->GetIndex()].Ctx->BeginUse();
  for(sInt i=0;i<count;i++)
  {
    Mtrl->Set(&MtrlPara);
//    Mtrl->Set(&MtrlPara);
    sVertexOffset off;
    sClear(off);
    off.VOff[1] = i+start;
    Geo->Draw(0,0,1,&off);
  }
  ThreadDatas[th->GetIndex()].Ctx->EndUse();
  sSchedMon->End(th->GetIndex());
}

// painting

void MyApp::OnPaint3D()
{
  Element *e;
  sMatrix34CM *vp;

  // timing

  sSchedMon->FlipFrame();
  sSchedMon->Begin(0,0x800000);

  Timer.OnFrame(sGetTime());
  Cam.OnFrame(Timer.GetSlices(),Timer.GetJitter());
//  sInt time = Timer.GetTime();
  sInt slices = Timer.GetSlices();

  // Physics

  sSchedMon->Begin(0,0x804000);
  sFORALL(Elements,e)
  {
    e->Rot += e->RotSpeed * (slices*0.0001f);
//    e->Matrix.EulerXYZ(e->Rot.x*sPI2F,e->Rot.y*sPI2F,e->Rot.z*sPI2F);
    e->Matrix.Init();
    e->Matrix.l = e->Pos;
  }
  sSchedMon->End(0);

  // render scene

  sSetTarget(sTargetPara(sCLEAR_ALL,0xff405060));

  View.SetTargetCurrent();
  View.SetZoom(2.0f);
  Cam.MakeViewport(View);
  View.Prepare();
  MtrlPara.Modify();
  MtrlPara.Data->Set(View);
  Mtrl->Set(&MtrlPara);

  sSchedMon->Begin(0,0x800040);
  Geo->BeginLoadVB(Elements.GetCount(),sGD_STREAM,&vp,1);
  for(sInt j=0;j<Elements.GetCount();j++)
    vp[j] = Elements[j].Matrix;
  Geo->EndLoadVB(-1,1);
  sSchedMon->End(0);

  sStsWorkload *wl = 0;
  if(Mode==0)
  {
    sSchedMon->Begin(0,0x008000);
    sInt batchcount = Elements.GetCount();
    sInt batchsize = 0x40000000;
    batchsize = 1;
    for(sInt batch = 0;batch<batchcount;batch+=batchsize)
    {
      sInt max = sMin(batchcount-batch,batchsize);
      Mtrl->Set(&MtrlPara);
      sVertexOffset off;
      sClear(off);
      off.VOff[1] = batch;
      Geo->Draw(0,0,max,&off);
    }
    sSchedMon->End(0);
  }
  else
  {
    sSchedMon->Begin(0,0x808000);
    wl = sSched->BeginWorkload();
    sStsTask *task = wl->NewTask(::ThreadCode,this,Elements.GetCount(),0);
    task->EndGame = 1<<sMax(Granularity,EndGame);
    task->Granularity = 1<<Granularity;
    wl->AddTask(task);

    for(sInt i=0;i<ThreadCount;i++)
    {
      ThreadDatas[i].Ctx->BeginRecording();
      ThreadDatas[i].Ctx->BeginUse();
      sSetTarget(sTargetPara(sCLEAR_NONE,0));
      Mtrl->Set(&MtrlPara);
      ThreadDatas[i].Ctx->EndUse();
    }

    sSched->StartWorkload(wl);
    sSchedMon->Begin(0,0x008000);
/*
    if(Mode==1)
      sSched->StartSingle();
    else
      sSched->Start();
    sSched->Finish();
      */
    sSched->SyncWorkload(wl);
    sSchedMon->End(0);
    
    sSchedMon->Begin(0,0xa0a000);
    for(sInt i=0;i<ThreadCount;i++)
      ThreadDatas[i].Ctx->EndRecording();
    sSchedMon->End(0);
    for(sInt i=0;i<ThreadCount;i++)
      ThreadDatas[i].Ctx->Draw();
    sSetTarget(sTargetPara(sCLEAR_NONE,0));
    sSchedMon->End(0);
  }

  // debug out

  sTextBuffer tb;

  const sChar *modestring[] = { L"no threading",L"one thread",L"all threads" };

  sF32 avg = Timer.GetAverageDelta();
  tb.PrintF(L"%5.2ffps %5.3fms\n",1000/avg,avg);
  tb.PrintF(L"camspeed %d\n",Cam.GetSpeed());
  tb.PrintF(L"%s",wl->PrintStat());
  tb.PrintF(L"(m) Mode: %s\n",modestring[Mode]);
  tb.PrintF(L"(g) Granularitye: %d\n",1<<Granularity);
  tb.PrintF(L"(e) Endgame: %d\n",1<<EndGame);

  sSched->EndWorkload(wl);
  Painter->Begin();
  Painter->SetTarget();
  Painter->SetPrint(0,~0,2);
  Painter->Print(10,10,tb.Get());
  Painter->End();
  sTargetSpec ts;
  sSchedMon->Paint(ts);
  sSchedMon->End(0);
}

/****************************************************************************/

// onkey: check for escape

void MyApp::OnInput(const sInput2Event &ie)
{
  Cam.OnInput(ie);
  if((ie.Key&sKEYQ_MASK)==sKEY_ESCAPE) sExit();
  switch(ie.Key & (sKEYQ_MASK|sKEYQ_BREAK))
  {
//  case 'm':
//    Mode = (Mode+1) % 3;
//    break;
  case 'g':
    Granularity = (Granularity+1) % 12;
    break;
  case 'e':
    EndGame = (EndGame+1) % 12;
    break;  
  case 'G':
    Granularity = (Granularity+12-1) % 12;
    break;
  case 'E':
    EndGame = (EndGame+12-1) % 12;
    break;
  case 'p':
    sSchedMon->Scale = sClamp(sSchedMon->Scale+1,12,20);
    break;
  case 'P':
    sSchedMon->Scale = sClamp(sSchedMon->Scale-1,12,20);
    break;
  }
}

/****************************************************************************/

// main: initialize screen and app

void sMain()
{
  sInit(sISF_3D|sISF_CONTINUOUS|sISF_NOVSYNC,1280,720);
  sSetApp(new MyApp());
  sSetWindowName(L"shader");
}

/****************************************************************************/
