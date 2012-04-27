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

MyApp *App;

volatile sMatrix34 SlowMat;

/****************************************************************************/

GeoBuffer::GeoBuffer()
{
  Geo = new sGeometry(sGF_TRILIST,App->Format);
  Alloc = 0;
  Used = 0;
  vp = 0;
}

GeoBuffer::~GeoBuffer()
{
  delete Geo;
}

void GeoBuffer::Begin()
{
  Alloc = 0x10000;
  Used = 0;
 
  App->BufferLock.Lock();
  Geo->BeginLoadVB(Alloc,sGD_FRAME,&vp,1);
  App->BufferLock.Unlock();
}

void GeoBuffer::End()
{
  App->BufferLock.Lock();
  Geo->EndLoadVB(Used,1);
  App->BufferLock.Unlock();
}

void GeoBuffer::Draw(sGeometry *geo)
{
  if(Used>0)
  {
    geo->MergeVertexStream(1,Geo,1);
    geo->Draw(0,0,Used,0);
  }
}

/****************************************************************************/

// constructor

MyApp::MyApp()
{
  App = this;

  // painter: debug out in 2d

  Painter = new sPainter;
  Timer.SetHistoryMax(16);

  // scheduler

  sAddSched();
  ThreadCount = sSched->GetThreadCount();
  ThreadDatas = new ThreadDataType[ThreadCount];
  sSchedMon = new sStsPerfMon;
  sSchedMon->Scale = 17;
  FrameSync = 0;
  WLA = 0;
  WLB = 0;

  // gui

  EndGame = 0;
  Mode = 0;
  Granularity = 8;

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

  sInt n = 64;
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
}

/****************************************************************************/

// destructor

MyApp::~MyApp()
{
  if(WLB)
  {
//    sSched->Finish();
    sSched->SyncWorkload(WLB);
    sSched->EndWorkload(WLB);
  }

  delete Painter;
  delete Geo;
  delete Mtrl;
  delete Tex;
  delete[] ThreadDatas;
  sDeleteAll(BufferFree);
  sDeleteAll(BufferFull);
}

GeoBuffer *MyApp::GetBuffer()
{
  BufferLock.Lock();

  if(BufferFree.IsEmpty())
    BufferFree.AddTail(new GeoBuffer);

  GeoBuffer *b = BufferFree.RemHead();

  BufferLock.Unlock();
  return b;
}

/****************************************************************************/

void ThreadCode0(sStsManager *man,sStsThread *th,sInt start,sInt count,void *data)
{
  ((MyApp *) data)->ThreadCode0(man,th,start,count);
}

void ThreadCode1(sStsManager *man,sStsThread *th,sInt start,sInt count,void *data)
{
  ((MyApp *) data)->ThreadCode1(man,th,start,count);
}

void MyApp::ThreadCode1(sStsManager *man,sStsThread *th,sInt start,sInt count)
{
  sInt id = th->GetIndex();
  sSchedMon->Begin(id,0xffff00);

  GeoBuffer *gb = ThreadDatas[id].GB;
  if(gb->Used+count>gb->Alloc)
  {
    gb->End();
    BufferLock.Lock();
    BufferFull.AddTail(gb);
    BufferLock.Unlock();
    gb = GetBuffer();
    ThreadDatas[id].GB = gb;
    gb->Begin();
  }
  sVERIFY(gb->Used + count <= gb->Alloc);
    
  sMatrix34 mat;
  for(sInt i=0;i<count;i++)
  {
    Element *e = &Elements[i+start];
    mat.EulerXYZ(e->Rot.x*sPI2F,e->Rot.y*sPI2F,e->Rot.z*sPI2F);
    mat.l = e->Pos;
    gb->vp[gb->Used++] = mat;
  }
  sSchedMon->End(id);
}

void MyApp::ThreadCode0(sStsManager *man,sStsThread *th,sInt start,sInt count)
{
  sInt id = th->GetIndex();
  sSchedMon->Begin(id,0x00ffff);
  sInt slices = Timer.GetSlices();

  for(sInt i=start;i<start+count;i++)
  {
    Element *e = &Elements[i];
    e->Rot += e->RotSpeed * (slices*0.0001f);
    e->Matrix.EulerXYZ(e->Rot.x*sPI2F,e->Rot.y*sPI2F,e->Rot.z*sPI2F);
    e->Matrix.l = e->Pos;  
  }
  
  sSchedMon->End(id);
}

// painting

void MyApp::OnPaint3D()
{
  // timing

  sSchedMon->FlipFrame();
  sSchedMon->Begin(0,0xff0000);

  Timer.OnFrame(sGetTime());
  Cam.OnFrame(Timer.GetSlices(),Timer.GetJitter());
//  sInt time = Timer.GetTime();
  sInt max = Elements.GetCount();

  sTextBuffer tb;

  sF32 avg = Timer.GetAverageDelta();
  tb.PrintF(L"%5.2ffps %5.3fms\n",1000/avg,avg);
  tb.PrintF(L"camspeed %d\n",Cam.GetSpeed());
  tb.PrintF(L"(g) Granularitye: %d\n",1<<Granularity);
  tb.PrintF(L"(e) Endgame: %d\n",1<<EndGame);
  tb.PrintF(L"(s) Graph Scale: %d\n",sSchedMon->Scale);

  // render scene

  sSetTarget(sTargetPara(sCLEAR_ALL,0xff405060));

  View.SetTargetCurrent();
  View.SetZoom(2.0f);
  Cam.MakeViewport(View);
  View.Prepare();
  MtrlPara.Modify();
  MtrlPara.Data->Set(View);
  Mtrl->Set(&MtrlPara);

  for(sInt i=0;i<ThreadCount;i++)
  {
    ThreadDatas[i].GB = GetBuffer();
    ThreadDatas[i].GB->Begin();
  }
  if(Mode==1 && WLB)
    sSched->SyncWorkload(WLB);

  sStsWorkload *wl = 0;
  if(Mode==2)
    wl = WLB;
  else 
    wl = WLA = sSched->BeginWorkload();
  sStsTask *task = wl->NewTask(::ThreadCode1,this,max,0);
  task->EndGame = 1<<sMax(Granularity,EndGame);
  task->Granularity = 1<<Granularity;
  wl->AddTask(task);
  if(Mode!=2)
    sSched->StartWorkload(wl);

  if(WLB)
  {
    sSched->SyncWorkload(WLB);
    tb.PrintF(L"Cyan: %s",WLB->PrintStat());
    sSched->EndWorkload(WLB);
    WLB = 0;
  }

  sSchedMon->Begin(0,0x0000ff);
  for(sInt i=0;i<20000;i++)
  {
    sMatrix34 mat;
    mat.EulerXYZ(i,i*2,i*3);
  }
  sSchedMon->End(0);

  // Physics

  if(Mode==1)
    sSched->SyncWorkload(WLA);

  WLB = sSched->BeginWorkload();
  sStsTask *task2 = WLB->NewTask(::ThreadCode0,this,max,0);
  task2->EndGame = 1<<sMax(Granularity,EndGame);
  task2->Granularity = 1<<Granularity;
  WLB->AddTask(task2);
  sSched->StartWorkload(WLB);

  // finish WLA

  if(WLA)
  {
    sSched->SyncWorkload(WLA);
    tb.PrintF(L"Blue: %s",WLA->PrintStat());
    sSched->EndWorkload(WLA);
    WLA = 0;
  }
  FrameSync = 0;

  // actual drawing

  BufferLock.Lock();
  for(sInt i=0;i<ThreadCount;i++)
  {
    ThreadDatas[i].GB->End();
    BufferFull.AddTail(ThreadDatas[i].GB);
  }
  BufferLock.Unlock();

  GeoBuffer *gb;
  BufferLock.Lock();
  sInt count = 0;
  sFORALL_LIST(BufferFull,gb)
  {
    count += gb->Used;
    gb->Draw(Geo);
  }
  BufferFree.AddTail(BufferFull);
  BufferLock.Unlock();

  // debug out

  switch(Mode)
  {
  case 0: tb.Print(L"Mode: real priorities\n"); break;
  case 1: tb.Print(L"Mode: Synced"); break;
  case 2: tb.Print(L"Mode: no priorities"); break;
  }

  Painter->Begin();
  Painter->SetTarget();
  Painter->SetPrint(0,~0,1);
  Painter->Print(10,10,tb.Get());
  Painter->End();

  sSchedMon->Begin(0,0x00ff00);
  sTargetSpec ts;
  sSchedMon->Paint(ts);
  for(sInt i=0;i<20000;i++)
  {
    sMatrix34 mat;
    mat.EulerXYZ(i,i*2,i*3);
  }
  sSchedMon->End(0);

  // done

  sSchedMon->End(0);
}

/****************************************************************************/

// onkey: check for escape

void MyApp::OnInput(const sInput2Event &ie)
{
  Cam.OnInput(ie);
  if((ie.Key&sKEYQ_MASK)==sKEY_ESCAPE) sExit();
  sU32 key = ie.Key & (sKEYQ_MASK|sKEYQ_BREAK);
  if(ie.Key & sKEYQ_SHIFT) key |= sKEYQ_SHIFT;
  switch(key)
  {
  case 'g':
    Granularity = sClamp(Granularity+1,0,15);
    break;
  case 'G'|sKEYQ_SHIFT:
    Granularity = sClamp(Granularity-1,0,15);
    break;
  case 'e':
    EndGame = sClamp(EndGame+1,0,15);
    break;
  case 'E'|sKEYQ_SHIFT:
    EndGame = sClamp(EndGame-1,0,15);
    break;
  case 'p':
    sSchedMon->Scale = sClamp(sSchedMon->Scale+1,8,23);
    break;
  case 'P'|sKEYQ_SHIFT:
    sSchedMon->Scale = sClamp(sSchedMon->Scale-1,8,23);
    break;
  case 'm':
    Mode = (Mode+1) % 3;
    break;
  }
}

/****************************************************************************/

// main: initialize screen and app

void sMain()
{
  sGetMemHandler(sAMF_HEAP)->MakeThreadSafe();
  sInit(sISF_3D|sISF_CONTINUOUS|sISF_NOVSYNC,1280,720);
  sSetApp(new MyApp());
  sSetWindowName(L"Pipelining");
}

/****************************************************************************/
