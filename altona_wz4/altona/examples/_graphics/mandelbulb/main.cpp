/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#define sPEDANTIC_OBSOLETE 1
#define sPEDANTIC_WARN 1

#define LOGGING 1
#define FREEFLIGHT 1
#define FULLSCREEN 0

#include "main.hpp"
#include "base/windows.hpp"
#include "util/image.hpp"
#include "mandel.hpp"
#include "mandelshader.hpp"
#include "util/taskscheduler.hpp"

/****************************************************************************/

// initialize resources
 
MyApp::MyApp()
{
//  sGetMemHandler(sAMF_HEAP)->MakeThreadSafe();
  sAddSched();
  Root = 0;
  Workload = 0;
  Painter = new sPainter;
  Cam.SetPos(sVector31(0,0,-0.75f));
  Cam.SetSpeed(-12);
  Cam.SpaceshipMode(1);
  Timer.SetHistoryMax(10);
  ToggleNew = 1;
  FogFactor = -1;

  OctMan = new OctManager;

  // texture

  const sInt dim = 512;
  sImage img(dim,dim);

  img.Checker(0xff706050,0xff806040,256,256);
  Tex = sLoadTexture2D(&img,sTEX_2D|sTEX_ARGB8888);

  // material

  Mtrl = new MaterialMandel;
  Mtrl->Flags = sMTRL_ZON | sMTRL_CULLINV;
  Mtrl->Flags |= sMTRL_LIGHTING;
  Mtrl->Texture[0] = Tex;
  Mtrl->TFlags[0] = sMTF_LEVEL2|sMTF_TILE;
  Mtrl->Prepare(OctMan->MeshFormat);

}

// free resources (memory leaks are evil)

MyApp::~MyApp()
{
  if(Workload)
  {
    Workload->Sync();
    Workload->End();
  }
  OctMan->EndGame = 1;
  OctMan->FreeNode(Root);
  delete Painter;
  delete Mtrl;
  delete Tex;
  delete OctMan;
}


void MyApp::MakeSpace()
{
  Root = OctMan->AllocNode();
  Root->Splitting = 1;
  for(sInt i=0;i<8;i++)
  {
    Root->NewChilds[i] = OctMan->AllocNode();
    Root->NewChilds[i]->Parent = Root;
    Root->Center.Init(0,0,0);
    Root->Box.Min.Init(-1,-1,-1);
    Root->Box.Max.Init(1,1,1);
    sInt x0 = (i&1)?1:0;
    sInt y0 = (i&2)?1:0;
    sInt z0 = (i&4)?1:0;
    Root->NewChilds[i]->Init0(-1+x0,-1+y0,-1+z0,1);
  }
  OctMan->StartRender();
  OctMan->ReadbackRender();
  OctMan->StartRender();
  OctMan->ReadbackRender();
  OctMan->StartRender();
  OctMan->ReadbackRender();
  Root->MakeChilds1(0);
  Root->MakeChilds2();
}

/****************************************************************************/

// paint a frame

void MyApp::OnPaint3D()
{ 
  tb.Clear();
  OctMan->NewFrame();

  if(!Root)
    MakeSpace();

  // get timing

  Timer.OnFrame(sGetTime());
  static sInt time;
  time = Timer.GetTime();
  Cam.OnFrame(Timer.GetSlices(),0);

  // set camera

  View.SetTargetCurrent();
  View.SetZoom(1.7f);
  View.ClipNear = 0.0001f;
  View.ClipFar = 16;
  View.Model.Init();
  View.Camera.Init();

  if(FREEFLIGHT)
  {
    Cam.MakeViewport(View);
  }
  else
  {
    View.Camera.EulerXYZ(time*0.00011f,time*0.00013f,time*0.00017f);
    View.Camera.l = sVector31(View.Camera.k*-((sGetKeyQualifier() & sKEYQ_SHIFT) ? 4.0f : 0.75f));
  }

  View.Prepare();


  // manage subdivision

  Root->PrepareDraw(View);

  sInt droppednodes = 0;
  for(;;)
  {
    OctNode *n = Root->FindWorst();
    if(!n) break;
    sVERIFY(n->Evictable);
    if(n->Area>=MANDEL_DROP) break;

    droppednodes++;
    n->DeleteChilds();
  }

#if LOGGING
  if(droppednodes>0)
    sDPrintF(L"dropped %d\n",droppednodes);
#endif

  static sInt firsttime = 0;
  static sInt timerdelay = 2;
  sInt done = 0;
  OctNode *n;

  sInt newnodes = 0;
  if(ToggleNew)
  {
    for(sInt i=0;i<16;i++)
    {
      n = Root->FindBest();
      if(n && n->Area>MANDEL_SPLIT)
      {
        if(firsttime==0)
          firsttime = sGetTime();
        n->MakeChilds0();
        newnodes ++;
      }
    }
  }

  // MakeChilds0() has queued lots of renders, start them as fast as possible!

  OctMan->StartRender();

  if(Workload)
  {
    Workload->Sync();
    Workload->End();
    Workload = 0;
  }
  // Draw for real. put this after StartRender()

  sCBuffer<MaterialMandelPara> cb;
  cb.Data->mvp = View.ModelScreen;
  cb.Data->ldir = View.Camera.k;
  cb.Data->Color.InitColor(0xff806040);
  cb.Data->FogColor.InitColor(0xffa0b0c0);
  cb.Data->Cam.Init(View.Camera.l.x,View.Camera.l.y,View.Camera.l.z,sPow(2,sF32(FogFactor)));
  Mtrl->Set(&cb);
  sSetTarget(sTargetPara(sST_CLEARALL,0xffa0b0c0));
  Root->Draw(Mtrl,View);
  OctMan->Draw();


  // use pipeline


  
  if(1)   // enable / disable threading
  {
    if(!OctMan->Pipeline1.IsEmpty())
      Workload = sSched->BeginWorkload();
  }

  sFORALL(OctMan->Pipeline2,n)
    n->MakeChilds2();

  OctMan->ReadbackRender();  // do this after giving enough work to the gpu.


  sFORALL(OctMan->Pipeline1,n)
    n->MakeChilds1(Workload);


  if(LOGGING)
  {
    sInt p0 = OctMan->Pipeline0.GetCount();
    sInt p0b = OctMan->Pipeline0b.GetCount();
    sInt p1 = OctMan->Pipeline1.GetCount();
    sInt p2 = OctMan->Pipeline2.GetCount();
    if(done==0 && firsttime>0 && p0+p0b+p1+p2==0)
    {
      timerdelay--;
      if(timerdelay==0)
        sDPrintF(L"done: %d\n",sGetTime()-firsttime);
    }
    if(p0+p1+p2>0)
      sDPrintF(L"pipeline %3d %3d %3d %3d\n",p0,p0b,p1,p2);
  }

  OctMan->Pipeline2 = OctMan->Pipeline1;
  OctMan->Pipeline1 = OctMan->Pipeline0b;
  OctMan->Pipeline0b = OctMan->Pipeline0;
  OctMan->Pipeline0.Clear();

  if(Workload)
  {
    Workload->Start();
  }

  // debug output


  sGraphicsStats stat;
  sGetGraphicsStats(stat);
  sF32 avg = Timer.GetAverageDelta();

  static sInt oldmalloc = 0;
  sInt newmalloc = sGetMemoryAllocId();
  sInt mallocdiff = newmalloc-oldmalloc;
  oldmalloc = newmalloc;
  sPtr blobfree;
  sInt blobfrag;
  OctMan->BlobHeap->GetStats(blobfree,blobfrag);

  tb.PrintF(L"%5.2ffps %5.3fms\n",1000.0f/avg,avg);
  tb.PrintF(L"Prim %k Splitters %d Batches %d\n",stat.Primitives,stat.Splitter,stat.Batches);
  tb.PrintF(L"Vertices Drawn %k; Life %k; Deepest Drawn: %d; Nodes Drawn: %d;\n",OctMan->VerticesDrawn,OctMan->VerticesLife,sFindLowerPower(OctMan->DeepestDrawn),OctMan->NodesDrawn);
  tb.PrintF(L"Nodes Used %5d Max %5d; MemBundles Used %3d Max %3d;\n",OctMan->GetUsedNodes(),OctMan->NodesTotal,OctMan->GetUsedMemBundles(),OctMan->MemBundlesTotal);
  tb.PrintF(L"New Nodes %3d; Dropped Nodes %3d; Memory Allocation %4d; RoundRobin %d\n",newnodes,droppednodes,mallocdiff,OctMan->GeoRoundRobin);
  tb.PrintF(L"Blobmemory: free %k, fragments %d\n",blobfree,blobfrag);
  tb.PrintF(L"Cam Speed %d  Fog %d  AllowNewTiles %d\n",Cam.GetSpeed(),FogFactor,ToggleNew);

  sEnableGraphicsStats(0);
  sSetTarget(sTargetPara(0,0));
  Painter->SetTarget();
  Painter->Begin();
  Painter->SetPrint(0,0xff000000,2);
  Painter->SetPrint(0,~0,2);
  Painter->Print(10,10,tb.Get());
  Painter->End();
  sEnableGraphicsStats(1);
}

/****************************************************************************/

// abort program when escape is pressed

void MyApp::OnInput(const sInput2Event &ie)
{
  sU32 key = ie.Key&(sKEYQ_MASK|sKEYQ_BREAK);
  switch(key)
  {
  case sKEY_ESCAPE:
    sExit();
    break;
  case sKEY_ENTER:
  case ' ':
    ToggleNew = !ToggleNew;
    break;
  case '+':
    FogFactor = sClamp(FogFactor+1,-20,20);
    break;
  case '-':
    FogFactor = sClamp(FogFactor-1,-20,20);
    break;
  }
  Cam.OnInput(ie);
}

/****************************************************************************/
/****************************************************************************/

// register application class

void sMain()
{
  sSetWindowName(L"Mandelbulb Marching Cubes");

  if(FULLSCREEN)
    sInit(sISF_3D|sISF_CONTINUOUS|sISF_FULLSCREEN,1920,1200);
  else
    sInit(sISF_3D|sISF_CONTINUOUS,1280,768);
  sSetApp(new MyApp());
}

/****************************************************************************/
/****************************************************************************/
