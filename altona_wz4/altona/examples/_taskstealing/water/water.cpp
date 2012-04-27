/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "water.hpp"
#include "util/taskscheduler.hpp"

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
  if(n==1)
  {
    Func(0,0);
  }
  else if(n>0)
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
//  sF32 e = 1.0e-15;
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


    // find near particles

    sInt near[MAXNEAR];
    sInt nearcount = 0;
    for(sInt range=0;range<ranges;range++)
    {
      sInt j0 = rangestart[range];
      sInt j1 = j0 + rangecount[range];
      for(sInt j=j0;j<j1;j++)
      {
        sVERIFY(j>=0 && j<max);
        if(j>=n) continue;
        q = parts+j;

        d = p->OldPos - q->OldPos;
        sF32 lsq = d^d;                 // square distance
        if(lsq<r*r)
        {
          sVERIFY(nearcount<MAXNEAR);
          near[nearcount++] = j;
        }
      }
    }

    // viscosity

    for(sInt j=0;j<nearcount;j++)
    {
      q = parts+near[j];
/*
      sVector30 vp = p->OldPos - p->NewPos;
      sVector30 vq = q->OldPos - q->NewPos;
      d = p->OldPos - q->OldPos;
      sF32 lsq = d^d;                 // square distance
      sF32 ln = sSqrt(lsq);
      sF32 li = 1-ln*rr;
      d *= 1/ln;
      sF32 u = (vp-vq) ^ d;

      d *= (li*u*u*1.1f);
      q->OldPos += d;
      p->OldPos -= d;
*/
      sVector30 vp = p->OldPos - p->NewPos;
      sVector30 vq = q->OldPos - q->NewPos;

      d = p->OldPos - q->OldPos;
      sF32 lsq = d^d;                 // square distance
      sF32 ln = sSqrt(lsq);
      sF32 li = 1-ln*rr;

      d = (vp-vq)*li*li*0.5f*0.05f;
      p->OldPos -= d;
      q->OldPos += d;
    }

    // gravity

    p->NewPos.y += GravityY;
    d = sVector30(p->NewPos);
    p->NewPos += d*CentralGravity;

    // double density function

    sF32 pfar = 0;
    sF32 pnear = 0;

    for(sInt j=0;j<nearcount;j++)
    {
      q = parts+near[j];
      d = p->OldPos - q->OldPos;
      sF32 lsq = d^d;
      sF32 ln = sSqrt(lsq);
      sF32 li = 1-ln*rr;

      pfar += li*li;
      pnear += li*li*li;
    }

    pnear = pnear * InnerForce;
    pfar = pfar * OuterForce - 0.0002f;

    sVector30 dx;
    for(sInt j=0;j<nearcount;j++)
    {
      q = parts+near[j];
      d = p->OldPos - q->OldPos;
      sF32 lsq = d^d;
      sF32 ln = sSqrt(lsq);
      sF32 li = 1-ln*rr;

      d *= 1/ln;
      d *= pfar*li + pnear*li*li;

      p->NewPos += d;
      q->NewPos -= d;
    }


/*
    for(sInt range=0;range<ranges;range++)
    {
      sInt j0 = rangestart[range];
      sInt j1 = j0 + rangecount[range];
      for(sInt j=j0;j<j1;j++)
      {
        sVERIFY(j>=0 && j<max);
        if(j>=n) continue;
        q = parts+j;

        d = p->OldPos - q->OldPos;
        sF32 lsq = d^d;                 // square distance
        if(lsq<r*r)
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
*/
    // collide with environment
/*
    if(p->NewPos.x < -1) p->NewPos.x = -1;
    if(p->NewPos.y < -1) p->NewPos.y = -1;
    if(p->NewPos.z < -1) p->NewPos.z = -1;
    if(p->NewPos.x >  1) p->NewPos.x =  1;
//    if(p->NewPos.y >  1) p->NewPos.y =  1;
    if(p->NewPos.z >  1) p->NewPos.z =  1;
*/
  
    if(0)
    {
      const sF32 rc = 2.7f;
      const sF32 yc = 0;
      d = sVector30(p->NewPos.x,p->NewPos.y+yc,p->NewPos.z);
      sF32 lsq = d^d;
      if(lsq>rc*rc)
      {
        d.Unit();
        d *= rc;//*0.9990f;
        p->NewPos = sVector31(d);
      }
    }
    

    if(1)
    {
      for(sInt i=0;i<4;i++)
      {
        sF32 d = -(Planes[i]^p->NewPos);
        if(d>0)
        {
          sVector30 f = sVector30(Planes[i])*(d);
          p->NewPos += f;
//          p->OldPos -= f;
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

