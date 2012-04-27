/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "fr063_sph.hpp"
#include "wz4frlib/wz4_bsp.hpp"

/****************************************************************************/

SphGenerator::SphGenerator()
{
  Type = SphGeneratorType;
}

SphGenerator::~SphGenerator()
{
}

/****************************************************************************/

SphCollision::SphCollision()
{
  Type = SphCollisionType;
}

SphCollision::~SphCollision()
{
}

/****************************************************************************/
/***                                                                      ***/
/***   Generators                                                         ***/
/***                                                                      ***/
/****************************************************************************/

SphGenObject::SphGenObject()
{
  Bsp = 0;
}

SphGenObject::~SphGenObject()
{
  Bsp->Release();
}

void SphGenObject::Reset()
{
}

void SphGenObject::Init(class Wz4Mesh *mesh)
{
  if(mesh)
  {
    sAABBox box;
    mesh->CalcBBox(box);

    Bsp = new Wz4BSP;
    Bsp->FromMesh(mesh,0.001f);

    Para.Center = box.Center();
    Para.Radius = (box.Max-box.Min)*0.5f;
  }
}

void SphGenObject::SimPart(RPSPH *sph)
{
  if(sph->SimStep==Para.Time0)
  {
    sF32 d = Para.Raster;
    sF32 ds = 1.0f/Para.Raster;

    sInt x0 = sRoundDown((Para.Center.x - Para.Radius.x)*ds);
    sInt y0 = sRoundDown((Para.Center.y - Para.Radius.y)*ds);
    sInt z0 = sRoundDown((Para.Center.z - Para.Radius.z)*ds);
    sInt x1 = sRoundUp((Para.Center.x + Para.Radius.x)*ds);
    sInt y1 = sRoundUp((Para.Center.y + Para.Radius.y)*ds);
    sInt z1 = sRoundUp((Para.Center.z + Para.Radius.z)*ds);

    sRandomKISS rnd;

    sF32 f1 = Para.Randomness;
    sF32 f0 = (1-f1)*0.5f;

    sVector31 pmin = Para.Center - Para.Radius;
    sVector31 pmax = Para.Center + Para.Radius;

    sInt zc = z1-z0;
    sInt yc = y1-y0;
    sInt xc = x1-x0;
    sInt count = zc*yc*xc;

    if(count+sph->Parts[0]->GetCount() <= sph->Para.MaxPart)
    {
      sInt p0 = sph->Parts[0]->GetCount();
      RPSPH::Particle *p = sph->Parts[0]->AddMany(count);
      for(sInt z=z0;z<z1;z++)
      {
        for(sInt y=y0;y<y1;y++)
        {
          for(sInt x=x0;x<x1;x++)
          {
            p->NewPos.x = (x+rnd.Float(f1)+f0)*d;
            p->NewPos.y = (y+rnd.Float(f1)+f0)*d;
            p->NewPos.z = (z+rnd.Float(f1)+f0)*d;
            p->OldPos = p->NewPos - Para.StartSpeed;
            p->Color = Para.Color | 0xff000000;
            p->Hash = 0;

            if(Bsp)
            {
              if(!Bsp->IsInside(p->NewPos))
                p->Color = 0;
            }
            else
            {
              if((Para.Shape&0xf)==0)
              {
                if(!(p->NewPos.x > pmin.x &&
                     p->NewPos.y > pmin.y &&
                     p->NewPos.z > pmin.z &&
                     p->NewPos.x < pmax.x &&
                     p->NewPos.y < pmax.y &&
                     p->NewPos.z < pmax.z))
                {
                  p->Color = 0;
                }
              }
              else
              {
                sVector30 d = (p->NewPos - Para.Center);
                d.x /= Para.Radius.x;
                d.y /= Para.Radius.y;
                d.z /= Para.Radius.z;
                
                if((d^d)>=1.0f)
                  p->Color = 0;
              }
            }
            

            p++;
          }
        }
      }
      if(Para.Shape&0x10)
      {
        sInt m = xc;
        sInt mm = xc*yc;
        RPSPH::Spring *s;
        sVector30 d;

        s = sph->Springs.AddMany(zc*yc*(xc-1));
        for(sInt z=0;z<zc;z++)
        {
          for(sInt y=0;y<yc;y++)
          {
            for(sInt x=0;x<xc-1;x++)
            {
              sInt a = p0 + z*mm + y*m + (x+0);
              sInt b = p0 + z*mm + y*m + (x+1);
              d = (*(sph->Parts[0]))[a].NewPos - (*(sph->Parts[0]))[b].NewPos;

              s->Part0 = a;
              s->Part1 = b;
              s->RestLength = d.Length();

              s++;
            }
          }
        }
        s = sph->Springs.AddMany(zc*(yc-1)*xc);
        for(sInt z=0;z<zc;z++)
        {
          for(sInt y=0;y<yc-1;y++)
          {
            for(sInt x=0;x<xc;x++)
            {
              sInt a = p0 + z*mm + (y+0)*m + x;
              sInt b = p0 + z*mm + (y+1)*m + x;
              d = (*(sph->Parts[0]))[a].NewPos - (*(sph->Parts[0]))[b].NewPos;

              s->Part0 = a;
              s->Part1 = b;
              s->RestLength = d.Length();

              s++;
            }
          }
        }
        s = sph->Springs.AddMany((zc-1)*yc*xc);
        for(sInt z=0;z<zc-1;z++)
        {
          for(sInt y=0;y<yc;y++)
          {
            for(sInt x=0;x<xc;x++)
            {
              sInt a = p0 + (z+0)*mm + y*m + x;
              sInt b = p0 + (z+1)*mm + y*m + x;
              d = (*(sph->Parts[0]))[a].NewPos - (*(sph->Parts[0]))[b].NewPos;

              s->Part0 = a;
              s->Part1 = b;
              s->RestLength = d.Length();

              s++;
            }
          }
        }
        // diagonal
        s = sph->Springs.AddMany((zc-1)*(yc-1)*(xc-1)*4);
        for(sInt z=0;z<zc-1;z++)
        {
          for(sInt y=0;y<yc-1;y++)
          {
            for(sInt x=0;x<xc-1;x++)
            {
              sInt q0 = p0 + (z+0)*mm + (y+0)*m + (x+0);
              sInt q1 = p0 + (z+0)*mm + (y+0)*m + (x+1);
              sInt q2 = p0 + (z+0)*mm + (y+1)*m + (x+0);
              sInt q3 = p0 + (z+0)*mm + (y+1)*m + (x+1);
              sInt q4 = p0 + (z+1)*mm + (y+0)*m + (x+0);
              sInt q5 = p0 + (z+1)*mm + (y+0)*m + (x+1);
              sInt q6 = p0 + (z+1)*mm + (y+1)*m + (x+0);
              sInt q7 = p0 + (z+1)*mm + (y+1)*m + (x+1);

              d = (*(sph->Parts[0]))[q0].NewPos - (*(sph->Parts[0]))[q7].NewPos;
              s->Part0 = q0;
              s->Part1 = q7;
              s->RestLength = d.Length();
              s++;

              d = (*(sph->Parts[0]))[q1].NewPos - (*(sph->Parts[0]))[q6].NewPos;
              s->Part0 = q1;
              s->Part1 = q6;
              s->RestLength = d.Length();
              s++;

              d = (*(sph->Parts[0]))[q2].NewPos - (*(sph->Parts[0]))[q5].NewPos;
              s->Part0 = q2;
              s->Part1 = q5;
              s->RestLength = d.Length();
              s++;

              d = (*(sph->Parts[0]))[q3].NewPos - (*(sph->Parts[0]))[q4].NewPos;
              s->Part0 = q3;
              s->Part1 = q4;
              s->RestLength = d.Length();
              s++;
            }
          }
        }


      }
    }
  }
}

/****************************************************************************/

SphGenSource::SphGenSource()
{
}

void SphGenSource::Init()
{
  Matrix.Look(Para.Speed);
  Matrix.l = Para.Center;
  Rnd.Seed(1);
}

void SphGenSource::Reset()
{
}

void SphGenSource::SimPart(RPSPH *sph)
{
  if(Para.StartTime >= Para.EndTime ||(sph->SimStep >= Para.StartTime && sph->SimStep<Para.EndTime))
  {
    sInt t0 = (sph->SimStep+0)*Para.Density;
    sInt t1 = (sph->SimStep+1)*Para.Density;
    
    for(sInt i=t0;i<t1;i++)
    {
      if(sph->Parts[0]->GetCount()>=sph->Para.MaxPart)
        break;

      sF32 sx = (Rnd.Float(2)-1);
      sF32 sy = (Rnd.Float(2)-1);
      if(Para.Shape & 1)
        if(sx*sx+sy*sy>1.0f)
          continue;

      sVector31 pos(sx*Para.Radius,sy*Para.Radius,0);
      RPSPH::Particle p;

      p.NewPos = pos*Matrix;
      p.OldPos = p.NewPos-Para.Speed;
      p.Color = Para.Color | 0xff000000;
      p.Hash = 0;

      *sph->Parts[0]->AddMany(1) = p;
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Colliders                                                          ***/
/***                                                                      ***/
/****************************************************************************/

void SphCollSimple::Init()
{
}

void SphCollSimple::CollPart(RPSPH *sph)
{
  if(Para.StartTime >= Para.EndTime ||(sph->SimStep >= Para.StartTime && sph->SimStep<Para.EndTime))
  {
    if(Para.Radius.x*Para.Radius.y*Para.Radius.z>0)
    {
      sInt max = sph->Parts[0]->GetCount();
      RPSPH::Particle *p = sph->Parts[0]->GetData();

      if(Para.Shape==0)
      {
        sVector31 pmin = Para.Center-Para.Radius;
        sVector31 pmax = Para.Center+Para.Radius;
        if(Para.Flags & 1)
        {
          for(sInt i=0;i<max;i++)
          {
            if(p[i].NewPos.x < pmin.x ||
               p[i].NewPos.y < pmin.y ||
               p[i].NewPos.z < pmin.z ||
               p[i].NewPos.x > pmax.x ||
               p[i].NewPos.y > pmax.y ||
               p[i].NewPos.z > pmax.z)

              p[i].Color = 0;
          }
        }
        else
        {
          for(sInt i=0;i<max;i++)
          {
            if(p[i].NewPos.x < pmin.x) p[i].NewPos.x = pmin.x;
            if(p[i].NewPos.y < pmin.y) p[i].NewPos.y = pmin.y;
            if(p[i].NewPos.z < pmin.z) p[i].NewPos.z = pmin.z;

            if(p[i].NewPos.x > pmax.x) p[i].NewPos.x = pmax.x;
            if(p[i].NewPos.y > pmax.y) p[i].NewPos.y = pmax.y;
            if(p[i].NewPos.z > pmax.z) p[i].NewPos.z = pmax.z;
          }
        }
      }
      else
      {
        sVector30 s,rs,d;
        s = Para.Radius;
        rs.x = 1.0f/Para.Radius.x;
        rs.y = 1.0f/Para.Radius.y;
        rs.z = 1.0f/Para.Radius.z;

        if(Para.Flags & 1)
        {
          for(sInt i=0;i<max;i++)
          {
            d = (p[i].NewPos-Para.Center)*s;
            if((d^d)>1.0f)
              p[i].Color = 0;
          }
        }
        else
        {
          for(sInt i=0;i<max;i++)
          {
            d = (p[i].NewPos-Para.Center)*rs;
            sF32 rdist = sRSqrt(d^d);
            if(rdist<1.0f)
            {
              d*=rdist;
              p[i].NewPos = d*s+Para.Center;
            }
          }
        }
      }
    }
  }
}

/****************************************************************************/

void SphCollPlane::Init()
{
}

void SphCollPlane::CollPart(RPSPH *sph)
{
  if(Para.StartTime >= Para.EndTime ||(sph->SimStep >= Para.StartTime && sph->SimStep<Para.EndTime))
  {
    sInt max = sph->Parts[0]->GetCount();
    RPSPH::Particle *p = sph->Parts[0]->GetData();

    if(Para.Flags & 1)
    {
      for(sInt i=0;i<max;i++)
      {
        if(p[i].NewPos.y < Para.y)      
          p[i].Color = 0;
      }
    }
    else
    {
      for(sInt i=0;i<max;i++)
      {
        if(p[i].NewPos.y < Para.y)
          p[i].NewPos.y = Para.y;
      }
    }
  }
}

/****************************************************************************/

void SphCollShock::Init()
{
}

void SphCollShock::CollPart(RPSPH *sph)
{
  if(sph->SimStep==Para.Time || (Para.Flags & 1))
  {
    RPSPH::Particle *p;
    sVector30 d;
    sF32 r = Para.Radius;
    sF32 rr = 1.0f/r;
    sFORALL(*sph->Parts[0],p)
    {
      d = p->NewPos - Para.Center;
      sF32 len = sSqrt(d^d);
      if(len<r)
      {
        d *= 1.0f/len * (1-(len*rr)) * Para.Force;
        p->NewPos += d;
      }
      p->NewPos += Para.Speed;
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   The Real Simulator                                                 ***/
/***                                                                      ***/
/****************************************************************************/

RPSPH::RPSPH()
{
  Parts[0] = new sArray<Particle>;
  Parts[1] = new sArray<Particle>;
}

RPSPH::~RPSPH()
{
  sReleaseAll(Gens);
  sReleaseAll(Colls);
  SimStep = 0;
  delete Parts[0];
  delete Parts[1];
}

void RPSPH::Init()
{
  Reset();

  for(sInt i=0;i<Para.StartSteps;i++)
    SimPart();
}


void RPSPH::Reset()
{
  Parts[0]->HintSize(Para.MaxPart);
  Parts[0]->Clear();
  Springs.Clear();

  SphGenerator *gen;
  sFORALL(Gens,gen)
    gen->Reset();

  SphCollision *coll;
  sFORALL(Colls,coll)
    coll->Init();

  SimStep = 0;
  LastTimeF = 0;
}

void RPSPH::Hash()
{
  sVector30 d;

  sF32 r = Para.InteractRadius;
  sF32 rr = 1.0f/r;

  sInt max = Parts[0]->GetCount();
  Particle *parts = Parts[0]->GetData();

  sU32 *hashes = new sU32[max];
  sInt *remap = new sInt[max];
  for(sInt i=0;i<max;i++)
    remap[i] = 0x10000000;

  // get rid of springs when the particle died

  sInt sm = Springs.GetCount();
  Spring *s = Springs.GetData();
  for(sInt i=0;i<sm;)
  {
    if(parts[s[i].Part0].Color==0 || parts[s[i].Part1].Color==0)
      s[i] = s[--sm];
    else
      i++;
  }
  Springs.Resize(sm);

  // reset table

  for(sInt i=0;i<HASHSIZE;i++)      
  {
    HashTable[i].Count = 0;
    HashTable[i].Start = 0;
  }

  // calculate hash and figure out allocation

  for(sInt i=0;i<max;i++)           
  {
    if(parts[i].Color!=0)           // elements with color==0 will be killed!
    {
      sInt x = sInt((parts[i].NewPos.x+1024.0f)*rr);
      sInt y = sInt((parts[i].NewPos.y+1024.0f)*rr);
      sInt z = sInt((parts[i].NewPos.z+1024.0f)*rr);
      sU32 hash = (x*STEPX + y*STEPY + z*STEPZ) & HASHMASK;
      hashes[i] = hash;
      HashTable[hash].Count++;
    }
  }

  // allocate space for new layout

  sInt nmax = 0;
  for(sInt i=0;i<HASHSIZE;i++)  
  {
    HashTable[i].Start = nmax;
    nmax += HashTable[i].Count;
  }

  Parts[1]->Resize(nmax);
  Particle *parts1 = Parts[1]->GetData();

   // copy to new layout

  for(sInt i=0;i<max;i++)          
  {
    if(parts[i].Color!=0)           // elements with color==0 will be killed!
    {
      sU32 hash = hashes[i];
      sInt n = HashTable[hash].Start++;
      sVERIFY(n<nmax);
      parts1[n] = parts[i];
      parts1[n].Hash = hash;
      remap[i] = n;
    }
  }

  // we changed the start index. undo that!

  for(sInt i=0;i<HASHSIZE;i++)      
  {
    HashTable[i].Start -= HashTable[i].Count;
  }

  // fix springs

  sFORALL(Springs,s)
  {
    sVERIFY(s->Part0<max);
    sVERIFY(s->Part1<max);
    sVERIFY(remap[s->Part0]<nmax);
    sVERIFY(remap[s->Part1]<nmax);
    s->Part0 = remap[s->Part0];
    s->Part1 = remap[s->Part1];
  }

  // done;

  delete[] hashes;
  delete[] remap;

  sSwap(Parts[0],Parts[1]);
}

void RPSPH::Inter(sInt n0,sInt n1)
{
  Particle *p,*q,*parts;
  sF32 r = Para.InteractRadius;
  sF32 rr = 1.0f/r;
  sVector30 d;

  sInt max = Parts[0]->GetCount();
  parts = Parts[0]->GetData();
  sU32 oldhash = ~0UL;
  sInt rangestart[10];
  sInt rangecount[10];
  sInt ranges=0;

  sBool color = Para.ColorSmooth>0.00001f;

  for(sInt n=n0;n<n1;n++)
  {
    p = parts+n;

    // check everyone with everyone (optimized with spatial hashing)

    sU32 hash = p->Hash;
    if(hash!=oldhash)
    {
      sU32 h0;
      oldhash = hash;

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

      ranges = 9;
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

    // now we have half the neighbourhood 

    // viscosity

    for(sInt j=0;j<nearcount;j++)
    {
      q = parts+near[j];

      sVector30 vp = p->OldPos - p->NewPos;
      sVector30 vq = q->OldPos - q->NewPos;

      d = p->OldPos - q->OldPos;
      sF32 lsq = d^d;                 // square distance
      sF32 ln = sSqrt(lsq);
      sF32 li = 1-ln*rr;

      d = (vp-vq)*li*li*0.5f*Para.Viscosity;
      p->OldPos -= d;
      q->OldPos += d;
    }

    // gravity
/*
    p->NewPos.y += GravityY;
    d = sVector30(p->NewPos);
    p->NewPos += d*CentralGravity;
*/
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

    pnear = pnear * Para.InnerForce;
    pfar = pfar * Para.OuterForce - Para.BasePressure;

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

      if(color)
      {
        sF32 f = Para.ColorSmooth*0.5f*li;
        sInt r0 = ((p->Color>>16)&255);
        sInt g0 = ((p->Color>> 8)&255);
        sInt b0 = ((p->Color>> 0)&255);
        sInt r1 = ((q->Color>>16)&255);
        sInt g1 = ((q->Color>> 8)&255);
        sInt b1 = ((q->Color>> 0)&255);
        sInt dr = (r0 - r1) * f;
        sInt dg = (g0 - g1) * f;
        sInt db = (b0 - b1) * f;
        r0 -= dr;
        g0 -= dg;
        b0 -= db;
        r1 += dr;
        g1 += dg;
        b1 += db;
        p->Color = 0xff000000|(r0<<16)|(g0<<8)|(b0);
        q->Color = 0xff000000|(r1<<16)|(g1<<8)|(b1);
      }
    }
  }
}
 /*
void RPSPH::PhysicsSprings(sInt n0,sInt n1)
{
}
*/
void RPSPH::Physics()
{
  sInt max = Parts[0]->GetCount();
  Particle * const p = Parts[0]->GetData();
  sF32 pi = 1.0f/Para.PhysicsCycles;

//  sHeapSortUp(Springs,&Spring::Part0);

  for(sInt pl=0;pl<Para.PhysicsCycles;pl++)
  {
    for(sInt i=0;i<max;i++)
    {
      p[i].OldPos.y += Para.Gravity*pi;
      p[i].OldPos += sVector30(p[i].NewPos)*Para.CentralGravity*pi;
      sVector31 newpos = p[i].NewPos + (p[i].NewPos-p[i].OldPos)*Para.Friction;
      p[i].OldPos = p[i].NewPos;
      p[i].NewPos = newpos;
    }

    if(Para.SpringForce>0.000000001f)
    {
      if(Para.Flags&8)            // no break
      {
        Particle *p0,*p1;
        sVector30 d;

        sInt sm = Springs.GetCount();
        Spring *s = Springs.GetData();
        for(sInt i=0;i<sm;)
        {
          p0 = p+s[i].Part0;
          p1 = p+s[i].Part1;
          d = p0->NewPos-p1->NewPos;
          sF32 rest = s[i].RestLength;
          sF32 len = sSqrt(d^d);
          d *= 1.0f/len;
          sF32 force = (len-rest)*Para.SpringForce*pi;

          d *= force;
          p0->OldPos += d;
          p1->OldPos -= d;
          i++;
        }
      }
      else                        // can break
      {
        Particle *p0,*p1;
        sVector30 d;

        sInt sm = Springs.GetCount();
        Spring *s = Springs.GetData();
        for(sInt i=0;i<sm;)
        {
          p0 = p+s[i].Part0;
          p1 = p+s[i].Part1;
          d = p0->NewPos-p1->NewPos;
          sF32 rest = s[i].RestLength;
          sF32 len = sSqrt(d^d);
          d *= 1.0f/len;
          sF32 force = (len-rest)*Para.SpringForce*pi;

          // try something new: break if difference in speed orthogonal to spring direction
      
          sF32 forcedot = 0;
          {
            sVector30 d0(p0->NewPos-p0->OldPos);
            sVector30 d1(p1->NewPos-p1->OldPos);
            sVector30 dd = d0-d1;
            sVector30 c;
            c.Cross(dd,d);
            forcedot = c.Length();
          }

          // check for break

          if(force > Para.SpringBreakForce || forcedot > Para.SpringBreakForceDot)
          {
            s[i] = s[--sm];
          }
          else
          {
            d *= force;
            p0->OldPos += d;
            p1->OldPos -= d;
            i++;
          }
        }
        Springs.Resize(sm);
      }
    }
  }
}

static void InterT(sStsManager *,sStsThread *thread,sInt start,sInt count,void *data)
{
  RPSPH *_this = (RPSPH *) data;
  sSchedMon->Begin(thread->GetIndex(),0xffffff00);
  _this->Inter(start,start+count);
  sSchedMon->End(thread->GetIndex());
}

void RPSPH::SimPart()
{
  Hash();

  sInt max = Parts[0]->GetCount();
  if(Para.Multithreading)
  {
    sStsWorkload *wl = sSched->BeginWorkload();
    sStsTask *task = wl->NewTask(InterT,this,max,0);
    task->Granularity = sMax(1,max/256);
    wl->AddTask(task);
    wl->Start();
    wl->Sync();
    wl->End();
  }
  else
  {
    Inter(0,max);
  }
  Physics();

  SphGenerator *gen;
  sFORALL(Gens,gen)
    gen->SimPart(this);

  SphCollision *coll;
  sFORALL(Colls,coll)
    coll->CollPart(this);

  SimStep++;
}

void RPSPH::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
  SimTimeStep = Para.TimeStep*((Para.Flags&4)?0.001f:1);

  sInt n = 0;
  if(Para.Flags & 1)      // as fast as possible
  {
    for(sInt i=0;i<Para.TicksPerFrame;i++)
    {
      SimPart();
      n++;
    }
  }
  else
  {
    sF32 t = ctx->GetTime();
    if(ctx->PaintInfo->CacheWarmup)
      t = 0;
    CurrentTime = t;
    if(LastTimeF-SimTimeStep > t+0.01f)
    {
      LastTimeF = 0;
      Reset();
    }
    while(SimStep<Para.StartSteps && n<Para.MaxSteps)
    {
      SimPart();
      n++;
    }
    while(LastTimeF < t && n<Para.MaxSteps)
    {
      SimPart();
      LastTimeF += SimTimeStep;
      n++;
    }
  }
  ViewPrintF(L"SPH_Time: %5d, %d steps\n",SimStep,n);
}

sInt RPSPH::GetPartCount()
{
  return Para.MaxPart;
}

sInt RPSPH::GetPartFlags()
{
  if(Para.Flags & 2)
    return wPNF_Color;
  else
    return 0;
}

void RPSPH::Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt)
{
  sInt used = Parts[0]->GetCount();
  Particle *p = Parts[0]->GetData();

  sF32 f = 0;
  if(!(Para.Flags & 1) && SimTimeStep>0)
  {
    f = (LastTimeF-CurrentTime)/SimTimeStep;
    f = sClamp<sF32>(f,0,1);
  }

  Wz4Particle *part = pinfo.Parts;
  for(sInt i=0;i<used;i++)
  {
    part->Pos.Fade(f,p->NewPos,p->OldPos);
    part->Time = time+dt;
    part++; p++;
  }
  for(sInt i=used;i<Para.MaxPart;i++)
  {
    part->Pos.Init(0,0,0);
    part->Time = -1;
    part++;
  }
  pinfo.Used = used;
  pinfo.Compact = 1;

  if(pinfo.Colors)
  {
    p = Parts[0]->GetData();
    for(sInt i=0;i<used;i++)
      pinfo.Colors[i] = p[i].Color;
    pinfo.Flags = wPNF_Color;
  }
}
  
/****************************************************************************/
