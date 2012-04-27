/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "main.hpp"
#include "base/windows.hpp"
#include "util/image.hpp"
#include "util/shaders.hpp"

#include "util/taskscheduler.hpp"

/****************************************************************************/
/****************************************************************************/

// initialize resources
 
MyApp::MyApp()
{
  // debug output

  Painter = new sPainter;
  Timer.SetHistoryMax(4);

  // multitasking

  sSched = new sStsManager(1024*1024,1024*16);
  sSchedMon = new sStsPerfMon;

  // geometry 

  Geo = new sGeometry();
  Geo->Init(sGF_TRILIST|sGF_INDEX16,sVertexFormatStandard);

  // load vertices and indices

  Geo->LoadCube(0xffffffff,1,1,1,sGD_STATIC);

  // texture

  sImage img(64,64);
  img.Checker(0xffff8080,0xff80ff80,8,8);
  img.Glow(0.5f,0.5f,0.25f,0.25f,0xffffffff,1.0f,4.0f);
  Tex = sLoadTexture2D(&img,sTEX_2D|sTEX_ARGB8888|sTEX_DYNAMIC|sTEX_NOMIPMAPS);

  // material

  Mtrl = new sSimpleMaterial;
  Mtrl->Flags = sMTRL_ZON | sMTRL_CULLON;
  Mtrl->Flags |= sMTRL_LIGHTING;
  Mtrl->Texture[0] = Tex;
  Mtrl->TFlags[0] = sMTF_LEVEL2|sMTF_CLAMP;
  Mtrl->Prepare(Geo->GetFormat());

  // light

  sClear(Env);
  Env.AmbientColor  = 0x00202020;
  Env.LightColor[0] = 0x00ffffff;
  Env.LightDir[0].Init(0,0,1);
  Env.Fix();
}

// free resources (memory leaks are evil)

MyApp::~MyApp()
{
  delete Painter;
  delete Geo;
  delete Mtrl;
  delete Tex;
  delete sSched;
}

struct ms
{
  sU32 *s;
  sU32 *b;
  sInt i0;
  sInt i2;
  sStsWorkload *wl;
  sStsSync *parent;
};

void MergeSort(sU32 *s,sU32 *b,const sInt i0,const sInt i2)
{
  sInt n = i2-i0;
  sVERIFY(n>0);

  if(n==1)
  {
    return;
  }
  else if(n<10)
  {
    for(sInt i=i0;i<i2-1;i++)
      for(sInt j=i+1;j<i2;j++)
        if(s[i] > s[j])
          sSwap(s[i],s[j]);
  }
  else
  {
    sInt i1 = (i0+i2)/2;
    MergeSort(b,s,i0,i1);
    MergeSort(b,s,i1,i2);

//    for(sInt i=i0+1;i<i1;i++)  sVERIFY(b[i-1]<=b[i]);
//    for(sInt i=i1+1;i<i2;i++)  sVERIFY(b[i-1]<=b[i]);

    sInt s0 = i0;
    sInt s1 = i1;
    sInt d;
    for(d=i0;d<i2;)
    {
      if(b[s0]<b[s1])
      {
        s[d++] = b[s0++];
        if(s0==i1)
          while(d<i2)
            s[d++] = b[s1++];
      }
      else
      {
        s[d++] = b[s1++];
        if(s1==i2)
          while(d<i2)
            s[d++] = b[s0++];
      }
    }
    sVERIFY(s0==i1 && s1==i2 && d ==i2);
  }
//  for(sInt i=i0+1;i<i2;i++)  sVERIFY(s[i-1]<=s[i]);
}

void MergeSortMergeTask(sStsManager *m,sStsThread *th,sInt start,sInt count,void *data_)
{
  sSchedMon->Begin(th->GetIndex(),0xff00ff);
  ms *data = (ms *) data_;

  sU32 *s = data->s;
  sU32 *b = data->b;
  const sInt i0 = data->i0;
  const sInt i2 = data->i2;
  const sInt i1 = (i0+i2)/2;

//  sDPrintF(L"%08x <- %08x : %08x - %08x - %08x\n",s,b,i0,i1,i2);

//  for(sInt i=i0+1;i<i1;i++)  sVERIFY(b[i-1]<=b[i]);
//  for(sInt i=i1+1;i<i2;i++)  sVERIFY(b[i-1]<=b[i]);

  sInt s0 = i0;
  sInt s1 = i1;
  sInt d;
  for(d=i0;d<i2;)
  {
    if(b[s0]<b[s1])
    {
      s[d++] = b[s0++];
      if(s0==i1)
        while(d<i2)
          s[d++] = b[s1++];
    }
    else
    {
      s[d++] = b[s1++];
      if(s1==i2)
        while(d<i2)
          s[d++] = b[s0++];
    }
  }
  sVERIFY(s0==i1 && s1==i2 && d ==i2);
//  for(sInt i=i0+1;i<i2;i++)  sVERIFY(s[i-1]<=s[i]);

  sSchedMon->End(th->GetIndex());
}

void MergeSortTask(sStsManager *m,sStsThread *th,sInt start,sInt count,void *data_)
{
  sSchedMon->Begin(th->GetIndex(),0x0000ff);
  ms *data = (ms *) data_;

  sU32 *s = data->s;
  sU32 *b = data->b;
  const sInt i0 = data->i0;
  const sInt i2 = data->i2;
  sStsWorkload *wl = data->wl;

  sInt n = i2-i0;
  sVERIFY(n>0);
  if(n==1)
  {
    return;
  }
  else if(n<1024*16)
  {
//    sDPrintF(L"%08x <- %08x : %08x - %08x start\n",s,b,i0,i2);
    MergeSort(s,b,i0,i2);
//    sDPrintF(L"%08x <- %08x : %08x - %08x\n",s,b,i0,i2);
//    for(sInt i=i0+1;i<i2;i++)  sVERIFY(s[i-1]<=s[i]);
  }
  else
  {
    sInt i1 = (i0+i2)/2;

    ms *data0 = wl->Alloc<ms>();
    ms *data1 = wl->Alloc<ms>();
    *data0 = *data1 = *data;
    sSwap(data0->s,data0->b);
    sSwap(data1->s,data1->b);
    data0->i2 = data1->i0 = i1;

    sStsTask *t0 = wl->NewTask(MergeSortTask,data0,1,1);
    sStsTask *t1 = wl->NewTask(MergeSortTask,data1,1,1);
    sStsTask *tm = wl->NewTask(MergeSortMergeTask,data,1,1);


    sStsSync *sync = wl->Alloc<sStsSync>();
    sVERIFY(sync);
    data0->parent = data1->parent = sync;
    sync->Count = 0;
    sync->ContinueTask = tm;
    m->AddSync(t0,sync);
    m->AddSync(t1,sync);
    if(data->parent)
    {
      m->AddSync(tm,data->parent);
      sVERIFY(data->parent->Count>1);
    }
    wl->AddTask(t0);
    wl->AddTask(t1);
  }

  sSchedMon->End(th->GetIndex());
}



/****************************************************************************/

// paint a frame

void MyApp::OnPaint3D()
{ 
  // set rendertarget

  sSchedMon->Begin(0,0xffffff);
  sSetRendertarget(0,sCLEAR_ALL,0xff405060);
  // get timing

  Timer.OnFrame(sGetTime());
  static sInt time;
  if(sHasWindowFocus())
    time = Timer.GetTime();

  sF32 avg = Timer.GetAverageDelta();
  tb.Clear();
  tb.PrintF(L"%5.2ffps %5.3fms\n",1000/avg,avg);

  // prepare random data

  sSchedMon->Begin(0,0xffffffff);
  sRandom rnd;
  rnd.Seed(time);
  SortSource.Resize(SortCount);
  for(sInt i=0;i<SortCount;i++)
    SortSource[i] = rnd.Int32();
  SortDestHS.Resize(SortCount);
  SortDestST.Resize(SortCount);
  SortDestMT.Resize(SortCount);
  sSchedMon->End(0);

  sBool single = sGetKeyQualifier() & sKEYQ_SHIFT;

  // mergesort MT

//  sDPrintF(L"----------------\n");
  if(!single)
  {
    SortDestHS = SortSource;
    SortDestMT = SortSource;
    sStsWorkload *wl = sSched->BeginWorkload();
    ms data;
    data.s = SortDestMT.GetData();
    data.b = SortDestHS.GetData();
    data.i0 = 0;
    data.i2 = SortCount;
    data.parent = 0;
    data.wl = wl;
    data.wl->AddTask(data.wl->NewTask(MergeSortTask,&data,1,0));
    sSchedMon->Begin(0,0xffff0000);
    wl->Start();
    wl->Sync();
    wl->End();
    sSchedMon->End(0);
  }

  // mergesort ST

  if(single)
  {
    SortDestHS = SortSource;
    SortDestST = SortSource;
    sSchedMon->Begin(0,0xff00ff00);
    MergeSort(SortDestST.GetData(),SortDestHS.GetData(),0,SortCount);
    sSchedMon->End(0);
  }

  // heapsort
/*
  SortDestHS = SortSource;
  sSchedMon->Begin(0,0xffffff00);
  sHeapSortUp(SortDestHS);
  sSchedMon->End(0);
*/
  // did it work?
  
  sInt e = 0;
  sSchedMon->Begin(0,0xffffffff);
  for(sInt i=1;i<SortCount;i++)
  {
    if(SortDestMT[i-1]>SortDestMT[i])
    {
      sDPrintF(L"%08x\n",i);
      e = 1;
    }
  }
  sVERIFY(!e);
  
  /*
  sVERIFY(sCmpMem(SortDestMT.GetData(),SortDestST.GetData(),sizeof(sU32)*SortCount)==0);
  sVERIFY(sCmpMem(SortDestHS.GetData(),SortDestST.GetData(),sizeof(sU32)*SortCount)==0);
  sVERIFY(sCmpMem(SortDestHS.GetData(),SortDestMT.GetData(),sizeof(sU32)*SortCount)==0);
  */
  sSchedMon->End(0);

  // set camera

  View.SetTargetCurrent();
  View.SetZoom(2.0f);
  View.Model.EulerXYZ(time*0.0011f,time*0.0013f,time*0.0015f);
  View.Model.l.Init(0,0,0);
  View.Camera.l.Init(0,0,-3.2f);
  View.Prepare();
 
  // set material

  sCBuffer<sSimpleMaterialEnvPara> cb;
  cb.Data->Set(View,Env);
  Mtrl->Set(&cb);

  // draw

  Geo->Draw();

  // debug output

  Painter->SetTarget();
  Painter->Begin();
  Painter->SetPrint(0,0xff000000,2);
  Painter->SetPrint(0,~0,2);
  Painter->Print(10,10,tb.Get());
  Painter->End();

  sSchedMon->Paint(sTargetSpec());
  sSchedMon->Scale = 18;
  sSchedMon->End(0);
  sSchedMon->FlipFrame();
}

/****************************************************************************/

// abort program when escape is pressed

void MyApp::OnInput(const sInput2Event &ie)
{
  sU32 key = ie.Key;
  if(key & sKEYQ_SHIFT) key |= sKEYQ_SHIFT;
  if(key & sKEYQ_CTRL ) key |= sKEYQ_CTRL;
  switch(ie.Key)
  {
  case sKEY_ESCAPE:
  case sKEY_ESCAPE|sKEYQ_SHIFT:
    sExit();
    break;
  }
}

/****************************************************************************/

// register application class

#if sPLATFORM==sPLAT_WINDOWS
sINITMEM(0,0);
#endif

void sMain()
{
  sSetWindowName(L"Multithreaded Mandelbrot");

  sInit(sISF_3D|sISF_CONTINUOUS|sISF_FSAA/*|sISF_NOVSYNC*//*|sISF_FULLSCREEN*/,1280,720);
  sSetApp(new MyApp());
}

/****************************************************************************/


/****************************************************************************/

