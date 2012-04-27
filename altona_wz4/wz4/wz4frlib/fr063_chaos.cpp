/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "wz4frlib/fr063_chaos.hpp"
#include "wz4frlib/fr063_chaos_ops.hpp"
#include "base/graphics.hpp"
#include "util/taskscheduler.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Water Simulation Class                                             ***/
/***                                                                      ***/
/****************************************************************************/

static void TaskFunc(sStsManager *,sStsThread *thread,sInt start,sInt count,void *data)
{
  WaterFX *fx = (WaterFX *) data;
  for(sInt i=0;i<count;i++)
    fx->Func(start+i,thread->GetIndex());
}

/****************************************************************************/

WaterFX::WaterFX()
{
  GravityY   = -0.0002f;
  CentralGravity = 0;
  OuterForce = -0.008f;    // anziehung
  InnerForce =  0.01f;    // abstﬂung
  InteractRadius = 0.1f;
  Friction = 0.994f;

  Parts[0] = new sArray<WaterParticle>;
  Parts[1] = new sArray<WaterParticle>;

  sVector30 n;
  n.Init( 0.5f,1.0f, 0.0f); n.Unit(); Planes[0].InitPlane(sVector31(0,-2,0),n);
  n.Init(-0.5f,1.0f, 0.0f); n.Unit(); Planes[1].InitPlane(sVector31(0,-2,0),n);
  n.Init( 0.0f,1.0f, 0.5f); n.Unit(); Planes[2].InitPlane(sVector31(0,-2,0),n);
  n.Init( 0.0f,1.0f,-0.5f); n.Unit(); Planes[3].InitPlane(sVector31(0,-2,0),n);
}

WaterFX::~WaterFX()
{
  delete Parts[0];
  delete Parts[1];
}

/****************************************************************************/

void WaterFX::Reset()
{
  Parts[0]->Clear();
}

void WaterFX::AddRain(sInt count,sU32 color)
{
  WaterParticle *p = Parts[0]->AddMany(count);

  for(sInt i=0;i<count;i++)
  {
    p[i].NewPos.x = Rnd.Float(2)-1;
    p[i].NewPos.y = Rnd.Float(2)-1;
    p[i].NewPos.z = Rnd.Float(2)-1;
    p[i].OldPos = p[i].NewPos;
    p[i].Color = color;
    p[i].Hash = 0;
  }
}

void WaterFX::AddDrop(sInt count,sU32 color,sF32 radius)
{
  WaterParticle *p = Parts[0]->AddMany(count);

  for(sInt i=0;i<count;i++)
  {
    sVector30 v;
    v.InitRandom(Rnd);
    p[i].NewPos = sVector31(v*radius);
    p[i].OldPos = p[i].NewPos;
    p[i].Color = color;
    p[i].Hash = 0;
  }
}

void WaterFX::Nudge(const sVector30 &speed)
{
  WaterParticle *p;

  sFORALL(*Parts[0],p)
    p->OldPos += speed;
}

void WaterFX::Step(sStsManager *sched,sStsWorkload *wl)
{
  sVector30 d;

  sF32 r = InteractRadius;
  sF32 rr = 1.0f/r;

  sInt max = Parts[0]->GetCount();
  Parts[1]->Resize(max);
  WaterParticle *parts = Parts[0]->GetData();
  WaterParticle *parts1 = Parts[1]->GetData();

  // spatial hashing

  sU32 *hashes = new sU32[max];

  for(sInt i=0;i<HASHSIZE;i++)      // reset table
  {
    HashTable[i].Count = 0;
    HashTable[i].Start = 0;
  }

  for(sInt i=0;i<max;i++)           // calculate hash and figure out allocation
  {
    sInt x = sInt((parts[i].NewPos.x+1024.0f)*rr);
    sInt y = sInt((parts[i].NewPos.y+1024.0f)*rr);
    sInt z = sInt((parts[i].NewPos.z+1024.0f)*rr);
    sU32 hash = (x*STEPX + y*STEPY + z*STEPZ) & HASHMASK;
    hashes[i] = hash;
    HashTable[hash].Count++;
  }

  for(sInt i=0,n=0;i<HASHSIZE;i++)  // allocate space for new layout
  {
    HashTable[i].Start = n;
    n += HashTable[i].Count;
  }

  for(sInt i=0;i<max;i++)           // copy to new layout
  {
    sU32 hash = hashes[i];
    sInt n = HashTable[hash].Start++;
    parts1[n] = parts[i];
    parts1[n].Hash = hash;
  }

  for(sInt i=0;i<HASHSIZE;i++)      // we changed the start index. undo that!
  {
    HashTable[i].Start -= HashTable[i].Count;
  }

  delete[] hashes;

  // swap array

  sSwap(Parts[0],Parts[1]);
  sSwap(parts,parts1);

  // apply last frames counterforce
/*
  counterforcevector *cf;
  LastCounterForce.Init(0,0,0);
  sFORALL(CounterForce,cf)
    LastCounterForce += cf->f;

  LastCounterForce *= 1.0f/max;

  // physics in parallel

  CounterForce.Resize(sched->GetThreadCount());
  sFORALL(CounterForce,cf)
    cf->f.Init(0,0,0);
*/
  sInt n = (max+BATCH-1)/BATCH;
  if(n>0)
  {
    sStsTask *task = wl->NewTask(TaskFunc,this,n,0);
    wl->AddTask(task);
  }
}


void WaterFX::Func(sInt n,sInt threadid)
{
  WaterParticle *p,*q,*parts;
  sF32 r = InteractRadius;
  sF32 rr = 1.0f/r;
  sF32 e = 1.0e-15;
  sVector30 d;

  sInt max = Parts[0]->GetCount();
  parts = Parts[0]->GetData();

  sInt n0 = n*BATCH;
  sInt n1 = sMin(max,(n+1)*BATCH);

  for(sInt n=n0;n<n1;n++)
  {
    sVERIFY(n<max);
    p = parts+n;


    // check everyone with everyone (optimized with spatial hashing)

    sInt rangestart[10];
    sInt rangecount[10];
    sU32 hash = p->Hash;
    sU32 h0;

    h0 = (hash - STEPX - STEPY - STEPZ)&HASHMASK;
    rangestart[0] = HashTable[h0].Start;
    rangecount[0] = HashTable[h0].Count+HashTable[(h0+1)&HASHMASK].Count+HashTable[(h0+2)&HASHMASK].Count;
    h0 = (hash - STEPX         - STEPZ)&HASHMASK;
    rangestart[1] = HashTable[h0].Start;
    rangecount[1] = HashTable[h0].Count+HashTable[(h0+1)&HASHMASK].Count+HashTable[(h0+2)&HASHMASK].Count;
    h0 = (hash - STEPX + STEPY - STEPZ)&HASHMASK;
    rangestart[2] = HashTable[h0].Start;
    rangecount[2] = HashTable[h0].Count+HashTable[(h0+1)&HASHMASK].Count+HashTable[(h0+2)&HASHMASK].Count;

    h0 = (hash - STEPX - STEPY        )&HASHMASK;
    rangestart[3] = HashTable[h0].Start;
    rangecount[3] = HashTable[h0].Count+HashTable[(h0+1)&HASHMASK].Count+HashTable[(h0+2)&HASHMASK].Count;
    h0 = (hash - STEPX                )&HASHMASK;
    rangestart[4] = HashTable[h0].Start;
    rangecount[4] = HashTable[h0].Count+HashTable[(h0+1)&HASHMASK].Count+HashTable[(h0+2)&HASHMASK].Count;
    h0 = (hash - STEPX + STEPY        )&HASHMASK;
    rangestart[5] = HashTable[h0].Start;
    rangecount[5] = HashTable[h0].Count+HashTable[(h0+1)&HASHMASK].Count+HashTable[(h0+2)&HASHMASK].Count;

    h0 = (hash - STEPX - STEPY + STEPZ)&HASHMASK;
    rangestart[6] = HashTable[h0].Start;
    rangecount[6] = HashTable[h0].Count+HashTable[(h0+1)&HASHMASK].Count+HashTable[(h0+2)&HASHMASK].Count;
    h0 = (hash - STEPX         + STEPZ)&HASHMASK;
    rangestart[7] = HashTable[h0].Start;
    rangecount[7] = HashTable[h0].Count+HashTable[(h0+1)&HASHMASK].Count+HashTable[(h0+2)&HASHMASK].Count;
    h0 = (hash - STEPX + STEPY + STEPZ)&HASHMASK;
    rangestart[8] = HashTable[h0].Start;
    rangecount[8] = HashTable[h0].Count+HashTable[(h0+1)&HASHMASK].Count+HashTable[(h0+2)&HASHMASK].Count;

    // one range may unfortunatly wrap from the end of the array to the start!

    sInt ranges = 9;
    for(sInt i=0;i<9;i++)
    {
      if(rangestart[i]+rangecount[i]>max)
      {
        sVERIFY(ranges<10);   // this should only happen once per particle!
        rangestart[ranges] = 0;
        rangecount[ranges] = rangestart[i]+rangecount[i] - max;
        rangecount[i] = max-rangestart[i];
        ranges++;
      }
    }

    // double density function

    for(sInt range=0;range<ranges;range++)
    {
      sInt j0 = rangestart[range];
      sInt j1 = j0 + rangecount[range];
      for(sInt j=j0;j<j1 /* && j<n*/;j++)
      {
        sVERIFY(j>=0 && j<max);
        if(j>=n) continue;
        q = parts+j;

        d = p->OldPos - q->OldPos;
        sF32 lsq = d^d;                 // square distance
        if(lsq<r*r && lsq>e*e)
        {
          sF32 l = sFSqrt(lsq);          // distance
          sF32 li = 1.0f-l*rr;           // 1-normalized distance
          sF32 f1 = li*li*OuterForce;    // anziehung
          sF32 f2 = li*li*li*InnerForce; // abstoﬂung
          
          d*= (f1+f2)/l;

          p->NewPos += d;
          q->NewPos -= d;
        }
      }
    }

    // simulate

  //  p->NewPos -= LastCounterForce;
    p->NewPos.y += GravityY;
    d = sVector30(p->NewPos);
    p->NewPos += d*CentralGravity;

    // collide with environment
/*
    if(p->NewPos.x < -1) p->NewPos.x = -1;
    if(p->NewPos.y < -1) p->NewPos.y = -1;
    if(p->NewPos.z < -1) p->NewPos.z = -1;
    if(p->NewPos.x >  1) p->NewPos.x =  1;
//    if(p->NewPos.y >  1) p->NewPos.y =  1;
    if(p->NewPos.z >  1) p->NewPos.z =  1;
*/
  
    if(1)
    {
      const sF32 rc = 2;
      const sF32 yc = 0;
      d = sVector30(p->NewPos.x,p->NewPos.y+yc,p->NewPos.z);
      sF32 lsq = d^d;
      if(lsq>rc*rc)
      {
        d.Unit();
        d *= rc;//*0.9990f;
        p->NewPos.Init(d.x,d.y-yc,d.z);
      }
    }
    

    if(0)
    {
      for(sInt i=0;i<4;i++)
      {
        sF32 d = -(Planes[i]^p->NewPos);
        if(d>0)
        {
          sVector30 f = sVector30(Planes[i])*(d*0.999);
          p->NewPos += f;
          p->OldPos -= f;
        }
      }
    }
    // simulatoin step
  
    sVector30 delta = p->NewPos - p->OldPos;
    p->OldPos = p->NewPos;
    p->NewPos += delta*Friction;

  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Water Simulation Effect                                            ***/
/***                                                                      ***/
/****************************************************************************/

RNFR063_Water::RNFR063_Water()
{
  Mtrl = 0;
  Step = 0;
  Anim.Init(Wz4RenderType->Script);   

  MC = 0;
  Water = 0;
  Drop = 0;
}

void RNFR063_Water::Init()
{
  MC = new MarchingCubes;

  Water = new WaterFX;
  Water->AddRain(Para.BowlCount,0xffffffff);
  Drop = new WaterFX;
  Drop->AddDrop(Para.DropCount,0xffff0000,Para.DropSize);
  Drop->GravityY = 0;
  Water->CentralGravity = 0;

  Water->GravityY       = Drop->CentralGravity  =  Para.Gravity;//-0.00005f;
  Water->OuterForce     = Drop->OuterForce      = -Para.OuterForce;//-0.0001f;    // anziehung
  Water->InnerForce     = Drop->InnerForce      =  Para.InnerForce;//0.01f;     // abstﬂung
  Water->InteractRadius = Drop->InteractRadius  =  Para.InteractRadius;//0.1f;
  Water->Friction       = Drop->Friction        =  Para.Friction;//0.995f;
}

RNFR063_Water::~RNFR063_Water()
{
  Mtrl->Release();
  delete MC;
  delete Drop;
  delete Water;
}


void RNFR063_Water::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
  SimulateChilds(ctx);

  WaterParticle *p;
  sStsWorkload *wl = sSched->BeginWorkload();


  MCParts.Clear();
  MCParts.Resize(Drop->GetCount()+Water->GetCount());
  sInt n=0;
  sFORALL(Drop->GetArray(),p)
    MCParts[n++] = sVector31(sVector30(p->OldPos)*10.0f);
  sFORALL(Water->GetArray(),p)
    MCParts[n++] = sVector31(sVector30(p->OldPos)*10.0f);
  MC->Begin(sSched,wl);

  // simulate

  Step++;

  if(Step==1000)
  {
    sFORALL(Drop->GetArray(),p)
    {
      p->NewPos.y += 0.5f;
      p->OldPos.y += 0.5f;
    }
    Water->GetArray().Add(Drop->GetArray());
    Drop->Reset();
  }

  if(Step%2000==0)
    Water->Nudge(sVector30(0.01f,0,0));

  if(Step<1000)
    Drop->Step(sSched,wl);

  Water->Step(sSched,wl);

  // multithread

  wl->Start();
  MC->Render(MCParts.GetData(),MCParts.GetCount());
  wl->Sync();
  wl->End();
  MC->End();
}


void RNFR063_Water::Prepare(Wz4RenderContext *ctx)
{
  if(Mtrl) Mtrl->BeforeFrame(Para.LightEnv);
}


void RNFR063_Water::Render(Wz4RenderContext *ctx)
{
  if(ctx->IsCommonRendermode())
  {
    if(Mtrl->SkipPhase(ctx->RenderMode,Para.LightEnv)) return;

    sMatrix34CM *mat;
    sFORALL(Matrices,mat)
    {
      Mtrl->Set(ctx->RenderMode|sRF_MATRIX_ONE,Para.LightEnv,mat,0,0,0);
      MC->Draw();
    }
  }
}


/****************************************************************************/
/***                                                                      ***/
/***   Fake Multithreaded Progress Bars                                   ***/
/***                                                                      ***/
/****************************************************************************/

RNFR063_MultiProgress::RNFR063_MultiProgress()
{
  Mtrl = new sSimpleMaterial;
  Mtrl->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  Mtrl->Prepare(sVertexFormatBasic);
  Geo = new sGeometry(sGF_QUADLIST,sVertexFormatBasic);

  Anim.Init(Wz4RenderType->Script);
}

void RNFR063_MultiProgress::Init()
{
}

RNFR063_MultiProgress::~RNFR063_MultiProgress()
{
  delete Geo;
  delete Mtrl;
}


void RNFR063_MultiProgress::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
  SimulateChilds(ctx);
}

static void rect(sVertexBasic *&vp,const sRect &r,sU32 col)
{
  vp[0].Init(r.x0,r.y0,0,col);
  vp[1].Init(r.x1,r.y0,0,col);
  vp[2].Init(r.x1,r.y1,0,col);
  vp[3].Init(r.x0,r.y1,0,col);
  vp+=4;
}

void RNFR063_MultiProgress::Prepare(Wz4RenderContext *ctx)
{
  sVertexBasic *vp;
  sInt xs = ctx->ScreenX;
  sInt ys = ctx->ScreenY;


  sInt count = Para.Cores;
  if(count==0) count = sSched->GetThreadCount();
  count = sClamp(count,1,64);

  sInt h0 = 15;
  if(count>2)
    h0 = 8;
  if(count>8)
    h0 = 6;
  if(count>16)
    h0 = 3;
  sInt w = 3;

  sInt h = h0*count+4*w;
  sF32 fade = Para.Animate;

  sInt n = 0;
  Geo->BeginLoadVB(8+4*count,sGD_STREAM,&vp);
  sRect r;

  r.x0 = 10;
  r.x1 = xs-10;
  r.y0 = ys/2-h/2;
  r.y1 = r.y0 + h;
  rect(vp,r,0xffffffff); n++;
  r.Extend(-w);
  rect(vp,r,0xff000000); n++;
  r.Extend(-w);


  sRandomKISS rnd;

  for(sInt i=0;i<count;i++)
  {
    // randomize

    sF32 f = sPow(fade,rnd.Float(1)+0.5);

    // render

    sRect rr=r;
    rr.y0 = r.y0 + h0*i;
    rr.y1 = r.y0 + h0*(i+1);
    rr.x1 = r.x0 + sInt(f*r.SizeX());
    rect(vp,rr,0xffffffff); n++;
  }

  Geo->EndLoadVB(n*4);
}


void RNFR063_MultiProgress::Render(Wz4RenderContext *ctx)
{
  if(ctx->IsCommonRendermode())
  {
//    if(Mtrl->SkipPhase(ctx->RenderMode,Para.LightEnv)) return;

    sViewport view;
    view.Orthogonal = sVO_PIXELS;
    view.Prepare();

    sCBuffer<sSimpleMaterialPara> cb;
    cb.Data->Set(view);

    Mtrl->Set(&cb);
    Geo->Draw();
  }
}



/****************************************************************************/

