/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "fxparticle_mc.hpp"
#include "wz4frlib/fxparticle_ops.hpp"
#include "wz4frlib/fxparticle_shader.hpp"
#include "wz4frlib/wz4_bsp.hpp"
#include "base/graphics.hpp"
#include "util/taskscheduler.hpp"

/****************************************************************************/

sU32 RNMarchingCubesTemplate::PartType::c;
__m128 RNMarchingCubesTemplate::SimdType::cr;
__m128 RNMarchingCubesTemplate::SimdType::cg;
__m128 RNMarchingCubesTemplate::SimdType::cb;

/****************************************************************************/

static const sInt table[27][4] =
{
  { 0x07,-1,-1,-1 },
  { 0x06,-1,-1, 0 },
  { 0x16,-1,-1, 1 },

  { 0x05,-1, 0,-1 },
  { 0x04,-1, 0, 0 },
  { 0x14,-1, 0, 1 },

  { 0x25,-1, 1,-1 },
  { 0x24,-1, 1, 0 },
  { 0x34,-1, 1, 1 },


  { 0x03, 0,-1,-1 },
  { 0x02, 0,-1, 0 },
  { 0x12, 0,-1, 1 },

  { 0x01, 0, 0,-1 },
  { 0x00, 0, 0, 0 },
  { 0x10, 0, 0, 1 },

  { 0x21, 0, 1,-1 },
  { 0x20, 0, 1, 0 },
  { 0x30, 0, 1, 1 },


  { 0x43, 1,-1,-1 },
  { 0x42, 1,-1, 0 },
  { 0x52, 1,-1, 1 },

  { 0x41, 1, 0,-1 },
  { 0x40, 1, 0, 0 },
  { 0x50, 1, 0, 1 },

  { 0x61, 1, 1,-1 },
  { 0x60, 1, 1, 0 },
  { 0x70, 1, 1, 1 },
};


/****************************************************************************/
/***                                                                      ***/
/***   Render as Marching Cubes                                           ***/
/***                                                                      ***/
/****************************************************************************/


/****************************************************************************/

/*
static void sINLINE func(const sVector31 &v,sVector4 &pot,const funcinfo &fi)
{
  sF32 p = 0;
  sVector30 n(0,0,0);
  for(sInt i=0;i<fi.pn;i++)
  {
    sVector30 d(v-fi.parts[i]);
    sF32 pp = d^d;
    if(pp<=fi.treshf)
    {
      pp = 1.0f/(pp)-fi.tresh;
      p += pp;
      n += d*(pp*pp);
    }
  }
  if(p==0)
    n.Init(0,0,0);
  else
    n.UnitFast();
  pot.Init(n.x,n.y,n.z,p-fi.iso);
}
*/

template<class T>
void sINLINE RNMarchingCubesBase<T>::func(const sVector31 &v,typename T::FieldType &pot,const funcinfo &fi)
{
  __m128 vx = _mm_load_ps1(&v.x);
  __m128 vy = _mm_load_ps1(&v.y);
  __m128 vz = _mm_load_ps1(&v.z);
  __m128 po = _mm_setzero_ps();           // p
  __m128 nx = _mm_setzero_ps();
  __m128 ny = _mm_setzero_ps();
  __m128 nz = _mm_setzero_ps();
  __m128 akkur = _mm_setzero_ps();
  __m128 akkug = _mm_setzero_ps();
  __m128 akkub = _mm_setzero_ps();
  __m128 akkua = _mm_setzero_ps();
  __m128 s255 = _mm_set_ps1(255.0f);
  
  sBool good = 0;

  for(sInt i=0;i<fi.pn4;i++)
  {
    const T::SimdType *part = fi.parts4 + i;

    __m128 dx = _mm_sub_ps(vx,part->x);
    __m128 dy = _mm_sub_ps(vy,part->y);
    __m128 dz = _mm_sub_ps(vz,part->z);
    __m128 ddx = _mm_mul_ps(dx,dx);
    __m128 ddy = _mm_mul_ps(dy,dy);
    __m128 ddz = _mm_mul_ps(dz,dz);
    __m128 pp = _mm_add_ps(_mm_add_ps(ddx,ddy),ddz);

    if(_mm_movemask_ps(_mm_cmple_ps(pp,fi.treshf4))!=0)
    {
      __m128 pp2 = _mm_sub_ps(_mm_div_ps(fi.one,pp),fi.tresh4);
      __m128 pp3 = _mm_max_ps(pp2,_mm_setzero_ps());
      po = _mm_add_ps(po,pp3);                  // p = p+pp;
      __m128 pp4 = _mm_mul_ps(pp3,pp3);         // pp*pp
      nx = _mm_add_ps(nx,_mm_mul_ps(pp4,dx));   // n += d*(pp*pp)
      ny = _mm_add_ps(ny,_mm_mul_ps(pp4,dy));
      nz = _mm_add_ps(nz,_mm_mul_ps(pp4,dz));
      if(T::Color==1)
      {
        akkur = _mm_add_ps(akkur,_mm_mul_ps(pp3,part->cr));
        akkug = _mm_add_ps(akkug,_mm_mul_ps(pp3,part->cg));
        akkub = _mm_add_ps(akkub,_mm_mul_ps(pp3,part->cb));
        good = 1;
      }
    }
  }

  sF32 p = 0;
  sVector30 n;
  
  _MM_TRANSPOSE4_PS(po,nx,ny,nz);
  __m128 r = _mm_add_ps(_mm_add_ps(_mm_add_ps(nx,ny),nz),po);
  n.x = r.m128_f32[1];
  n.y = r.m128_f32[2];
  n.z = r.m128_f32[3];
  p = r.m128_f32[0];

  if(p==0)
    n.Init(0,0,0);
  else
    n.UnitFast();
  pot.x = n.x;
  pot.y = n.y;
  pot.z = n.z;
  pot.w = p-fi.iso;
  if(T::Color)
  {
    if(good)
    {
      r = _mm_mul_ss(s255,_mm_rcp_ss(r));
  //    r = _mm_rcp_ss(r);
      _MM_TRANSPOSE4_PS(akkub,akkug,akkur,akkua);
      __m128 r2 = _mm_add_ps(_mm_add_ps(_mm_add_ps(akkur,akkug),akkub),akkua);

      r2 = _mm_mul_ps(r2,_mm_shuffle_ps(r,r,0x00));
      __m128i r3 = _mm_cvtps_epi32(r2);
      r3 = _mm_packs_epi32(r3,r3);
      __m128i r4 = _mm_packus_epi16(r3,r3);
      pot.c = r4.m128i_u32[0]|0xff000000;
    }
    else
    {
      pot.c = 0;
    }
  }
}

/****************************************************************************/

template<class T>
RNMarchingCubesBase<T>::RNMarchingCubesBase() : MC(T::Color)
{
  Source = 0;
  Mtrl = 0;
  MaxThread = sSched->GetThreadCount();

  Time = 0;

  PotSize = 0;
  SimdCount = 0;
  PotData = new typename T::FieldType*[MaxThread];
  SimdParts = new typename T::SimdType*[MaxThread];
  for(sInt i=0;i<MaxThread;i++)
  {
    PotData[i] = 0;
    SimdParts[i] = 0;
  }

  Workload = 0;
}

template<class T>
RNMarchingCubesBase<T>::~RNMarchingCubesBase()
{
  if(Workload)
  {
    Workload->Sync();
    Workload->End();
    Workload = 0;
  }
  if(Para.Multithreading==2)
    MC.End();

  Source->Release();
  Mtrl->Release();
//  sDeleteAll(AllHashConts);
//  sDeleteAll(AllPartConts);
  sDeleteAll(FreeHashConts);
  sDeleteAll(NodeHashConts);
  sDeleteAll(ThreadHashConts);
  sDeleteAll(AllPartContBlocks);
  for(sInt i=0;i<MaxThread;i++)
  {
    delete[] PotData[i];
    delete[] SimdParts[i];
  }
  delete[] PotData;
  delete[] SimdParts;
}

template<class T>
void RNMarchingCubesBase<T>::Init()
{
  MC.Init(Para.Multithreading==2);
  PInfo.Init(Source->GetPartFlags(),Source->GetPartCount());
  if(Para.Multithreading==2)
    MC.Begin();
}

template<class T>
void RNMarchingCubesBase<T>::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);

  Source->Simulate(ctx);
  Time = ctx->GetTime();
  SimulateChilds(ctx);
}

template<class T>
void RNMarchingCubesBase<T>::Prepare(Wz4RenderContext *ctx)
{
  PInfo.Reset();
  Source->Func(PInfo,Time,0);


  Spatial();
  Render();
}

template<class T>
typename RNMarchingCubesBase<T>::HashContainer *RNMarchingCubesBase<T>::GetHashContainer()
{
  HashContainer *c = 0;
  if(FreeHashConts.IsEmpty())
  {
    c = new HashContainer;
  }
  else
  {
    c = FreeHashConts.RemTail();
  }
  NodeHashConts.AddTail(c);
  return c;
}

template<class T>
typename RNMarchingCubesBase<T>::PartContainer *RNMarchingCubesBase<T>::GetPartContainer()
{
  PartContainer *c = 0;
  if(FreePartConts.IsEmpty())
  {
    PartContainer *cc = new PartContainer[256];
    AllPartContBlocks.AddTail(cc);
    for(sInt i=0;i<256;i++)
      FreePartConts.AddTail(cc+i);
  }
  c = FreePartConts.RemTail();
  NodePartConts.AddTail(c);
  return c;
}

template<class T>
void RNMarchingCubesBase<T>::Spatial()
{
  const sF32 e = Para.Influence/Para.GridSize;
  sVector31 gspos(1.0f/Para.GridSize,1.0f/Para.GridSize,1.0f/Para.GridSize);
  sClear(HashTable);
  sInt ixyz[3];

  Wz4Particle *p;
  sInt max = PInfo.GetCount();
  sBool colored = T::Color && PInfo.Colors!=0;
  for(sInt i=0;i<max;i++)
  {
    p = PInfo.Parts+i;
    if(p->Time>=0)
    {
      sVector31 opos = p->Pos;
      sVector31 pos = opos*gspos;
      sInt ix = sRoundDown(pos.x);
      sInt iy = sRoundDown(pos.y);
      sInt iz = sRoundDown(pos.z);

      sF32 fx = pos.x-ix;
      sF32 fy = pos.y-iy;
      sF32 fz = pos.z-iz;
      sInt bits = 0;
      if(fx<e) bits |= 0x01;
      if(fy<e) bits |= 0x02;
      if(fz<e) bits |= 0x04;
      if(fx>1-e) bits |= 0x10;
      if(fy>1-e) bits |= 0x20;
      if(fz>1-e) bits |= 0x40;

      for(sInt t=0;t<27;t++)
      {
        if((bits & table[t][0])==table[t][0])
        {
          ixyz[0] = ix+table[t][3];
          ixyz[1] = iy+table[t][2];
          ixyz[2] = iz+table[t][1];

          sInt hash = sChecksumMurMur((sU32 *)ixyz,3) & HashMask;
          HashContainer *hc = HashTable[hash];
          while(hc)
          {
            if(hc->IX==ixyz[0] && hc->IY==ixyz[1] && hc->IZ==ixyz[2])
              goto FOUND;
            hc = hc->Next;
          }
          hc = GetHashContainer();
          hc->IX = ixyz[0];
          hc->IY = ixyz[1];
          hc->IZ = ixyz[2];
          hc->FirstPart = GetPartContainer();
          hc->FirstPart->Count = 0;
          hc->FirstPart->Next = 0;
          hc->Next = HashTable[hash];
          HashTable[hash] = hc;
    FOUND:
          PartContainer *pc = hc->FirstPart;
          if(pc->Count==ContainerSize)
          {
            pc = GetPartContainer();
            pc->Count = 0;
            pc->Next = hc->FirstPart;
            hc->FirstPart = pc;
          }
          pc->Parts[pc->Count].x = opos.x;
          pc->Parts[pc->Count].y = opos.y;
          pc->Parts[pc->Count].z = opos.z;
          if(colored)
            pc->Parts[pc->Count].c = PInfo.Colors[i];
          pc->Count++;
        }
      }
    } 
  }
}

/****************************************************************************/

template <class T,int base,int subdiv>
static void StaticRenderT(sStsManager *man,sStsThread *thread,sInt start,sInt count,void *data)
{
  sSchedMon->Begin(thread?thread->GetIndex():0,0xff00ff00);
  RNMarchingCubesBase<T> *_this = (RNMarchingCubesBase<T> *) data;
  _this->RenderT<base,subdiv>(start,count,thread?thread->GetIndex():0);
  sSchedMon->End(thread?thread->GetIndex():0);
}


template <class T>
template <int base,int subdiv>
void RNMarchingCubesBase<T>::RenderT(sInt start,sInt count,sInt thread)
{
  for(sInt i_=start;i_<start+count;i_++)
  {
    HashContainer *hc = ThreadHashConts[i_];
    PartContainer *con = hc->FirstPart;
    const sInt s = 1<<base;
    const sInt m = (s+1);
    const sInt mm = (s+1)*(s+1);
    sF32 S = Para.GridSize/s;
    sVector31 tpos(hc->IX*Para.GridSize,hc->IY*Para.GridSize,hc->IZ*Para.GridSize);

//    sInt size = (s+2)*(s+1)*(s+1);
    typename T::FieldType *pot = PotData[thread];

    funcinfo fi;

    // calculate potential and normal

    sClear(fi);
    fi.tresh = 1/(Para.Influence*Para.Influence);
    fi.treshf = 1.0f/fi.tresh-0.00001f;
    fi.iso = Para.IsoValue;

    // reorganize array for SIMD

    sInt pn4 = 0;
    PartContainer *cp = con;
    while(cp)
    {
      pn4 += (cp->Count+3)/4;
      cp = cp->Next;
    }

    fi.tresh4 = _mm_load_ps1(&fi.tresh);
    fi.treshf4 = _mm_load_ps1(&fi.treshf);
    fi.one = _mm_set_ps1(1.0f);
    fi.epsilon = _mm_set_ps1(0.01f);
    fi.pn4 = pn4;

    fi.parts4 = SimdParts[thread];
    sInt i4 = 0;

    typename T::PartType far;
    far.x = 1024*1024;
    far.y = 0;
    far.z = 0;
    cp = con;
    while(cp)
    {
      sInt pn = cp->Count;
      typename T::PartType *p = cp->Parts;

      switch(pn&3)
      {
        case 1: p[pn+2] = far;
        case 2: p[pn+1] = far;
        case 3: p[pn+0] = far;
        case 0: break;
      }

      for(sInt i=0;i<(pn+3)/4;i++)
      {
        fi.parts4[i4].x.m128_f32[0] = p[0].x;
        fi.parts4[i4].x.m128_f32[1] = p[1].x;
        fi.parts4[i4].x.m128_f32[2] = p[2].x;
        fi.parts4[i4].x.m128_f32[3] = p[3].x;

        fi.parts4[i4].y.m128_f32[0] = p[0].y;
        fi.parts4[i4].y.m128_f32[1] = p[1].y;
        fi.parts4[i4].y.m128_f32[2] = p[2].y;
        fi.parts4[i4].y.m128_f32[3] = p[3].y;

        fi.parts4[i4].z.m128_f32[0] = p[0].z;
        fi.parts4[i4].z.m128_f32[1] = p[1].z;
        fi.parts4[i4].z.m128_f32[2] = p[2].z;
        fi.parts4[i4].z.m128_f32[3] = p[3].z;

        if(T::Color)
        {
          fi.parts4[i4].cr.m128_f32[0] = ((p[0].c>>16)&255)/255.0f;
          fi.parts4[i4].cr.m128_f32[1] = ((p[1].c>>16)&255)/255.0f;
          fi.parts4[i4].cr.m128_f32[2] = ((p[2].c>>16)&255)/255.0f;
          fi.parts4[i4].cr.m128_f32[3] = ((p[3].c>>16)&255)/255.0f;

          fi.parts4[i4].cg.m128_f32[0] = ((p[0].c>> 8)&255)/255.0f;
          fi.parts4[i4].cg.m128_f32[1] = ((p[1].c>> 8)&255)/255.0f;
          fi.parts4[i4].cg.m128_f32[2] = ((p[2].c>> 8)&255)/255.0f;
          fi.parts4[i4].cg.m128_f32[3] = ((p[3].c>> 8)&255)/255.0f;

          fi.parts4[i4].cb.m128_f32[0] = ((p[0].c>> 0)&255)/255.0f;
          fi.parts4[i4].cb.m128_f32[1] = ((p[1].c>> 0)&255)/255.0f;
          fi.parts4[i4].cb.m128_f32[2] = ((p[2].c>> 0)&255)/255.0f;
          fi.parts4[i4].cb.m128_f32[3] = ((p[3].c>> 0)&255)/255.0f;
        }

        p+=4;
        i4++;
      }
      cp = cp->Next;
    }
    sVERIFY(i4==fi.pn4);

    // pass 1: skip every second vertex

    for(sInt z=0;z<s+1;z++)
    {
      for(sInt y=0;y<s+1;y++)
      {
        for(sInt x=0;x<s+1;x++)
        {
          sVector31 v = sVector30(x,y,z) * S + tpos;

          func(v,pot[z*mm+y*m+x],fi);
        }
      }
    }

    // subdivision schemes

    if(subdiv==0)                 // none
    {
      // i don't understand, but manually inlining this makes things a bit faster...
      //  MC.March(Para.BaseGrid,pot,S,tpos);

      switch(base)
      {
        case 0: MC.March_0_1(pot,S,tpos,thread); break;
        case 1: MC.March_1_1(pot,S,tpos,thread); break;
        case 2: MC.March_2_1(pot,S,tpos,thread); break;
        case 3: MC.March_3_1(pot,S,tpos,thread); break;
        case 4: MC.March_4_1(pot,S,tpos,thread); break;
        case 5: MC.March_5_1(pot,S,tpos,thread); break;
        default: sVERIFYFALSE;
      }  
    }
    else                          // subdiv once
    {
      typename T::FieldType pot2[4][3][3];
      sVector31 v;
      typename T::FieldType pot2y[s][4];
      sInt lastyz[s];
      for(sInt i=0;i<s;i++) lastyz[i] = -2;

      for(sInt z=0;z<s;z++)
      {
        sInt LastY = -2;
        for(sInt y=0;y<s;y++)
        {
          sInt LastX = -2;
          for(sInt x=0;x<s;x++)  
          {
            sU32 flo,ma,mo;
            flo = *(sU32 *)&pot[(z+0)*mm+(y+0)*m+(x+0)].w; ma  = flo; mo  = flo;
            flo = *(sU32 *)&pot[(z+0)*mm+(y+0)*m+(x+1)].w; ma &= flo; mo |= flo;
            flo = *(sU32 *)&pot[(z+0)*mm+(y+1)*m+(x+0)].w; ma &= flo; mo |= flo;
            flo = *(sU32 *)&pot[(z+0)*mm+(y+1)*m+(x+1)].w; ma &= flo; mo |= flo;
            flo = *(sU32 *)&pot[(z+1)*mm+(y+0)*m+(x+0)].w; ma &= flo; mo |= flo;
            flo = *(sU32 *)&pot[(z+1)*mm+(y+0)*m+(x+1)].w; ma &= flo; mo |= flo;
            flo = *(sU32 *)&pot[(z+1)*mm+(y+1)*m+(x+0)].w; ma &= flo; mo |= flo;
            flo = *(sU32 *)&pot[(z+1)*mm+(y+1)*m+(x+1)].w; ma &= flo; mo |= flo;
            if((ma&0x80000000)==0 && (mo&0x80000000)!=0)
            {
              
              // get the dots we already have

              pot2[0][0][0] = pot[(z+0)*mm+(y+0)*m+(x+0)];
              pot2[0][0][2] = pot[(z+0)*mm+(y+0)*m+(x+1)];
              pot2[0][2][0] = pot[(z+0)*mm+(y+1)*m+(x+0)];
              pot2[0][2][2] = pot[(z+0)*mm+(y+1)*m+(x+1)];
              pot2[2][0][0] = pot[(z+1)*mm+(y+0)*m+(x+0)];
              pot2[2][0][2] = pot[(z+1)*mm+(y+0)*m+(x+1)];
              pot2[2][2][0] = pot[(z+1)*mm+(y+1)*m+(x+0)];
              pot2[2][2][2] = pot[(z+1)*mm+(y+1)*m+(x+1)];

              // reuse last x2 for current x0

              if(LastX==x-1)
              {
                pot2[1][0][0] = pot2[1][0][2];
                pot2[0][1][0] = pot2[0][1][2];
                pot2[1][1][0] = pot2[1][1][2];
                pot2[2][1][0] = pot2[2][1][2];
                pot2[1][2][0] = pot2[1][2][2];
              }
              else
              {
                v = sVector30(x+0.0f,y+0.0f,z+0.5f) * S + tpos;  func(v,pot2[1][0][0],fi);
                v = sVector30(x+0.0f,y+0.5f,z+0.0f) * S + tpos;  func(v,pot2[0][1][0],fi);
                v = sVector30(x+0.0f,y+0.5f,z+0.5f) * S + tpos;  func(v,pot2[1][1][0],fi);
                v = sVector30(x+0.0f,y+0.5f,z+1.0f) * S + tpos;  func(v,pot2[2][1][0],fi);
                v = sVector30(x+0.0f,y+1.0f,z+0.5f) * S + tpos;  func(v,pot2[1][2][0],fi);
              }
              LastX = x;

              // resuse last y2 for current y0

              if(LastY==y-1 && lastyz[x]==z)
              {
                pot2[0][0][1] = pot2y[x][0];
                pot2[1][0][1] = pot2y[x][1];
                pot2[2][0][1] = pot2y[x][2];
                pot2[1][0][2] = pot2y[x][3];
              }
              else
              {
                v = sVector30(x+0.5f,y+0.0f,z+0.0f) * S + tpos;  func(v,pot2[0][0][1],fi);
                v = sVector30(x+0.5f,y+0.0f,z+0.5f) * S + tpos;  func(v,pot2[1][0][1],fi);
                v = sVector30(x+0.5f,y+0.0f,z+1.0f) * S + tpos;  func(v,pot2[2][0][1],fi);
                v = sVector30(x+1.0f,y+0.0f,z+0.5f) * S + tpos;  func(v,pot2[1][0][2],fi);
              }

              v = sVector30(x+0.5f,y+1.0f,z+0.0f) * S + tpos;  func(v,pot2[0][2][1],fi);  pot2y[x][0] = pot2[0][2][1];
              v = sVector30(x+0.5f,y+1.0f,z+0.5f) * S + tpos;  func(v,pot2[1][2][1],fi);  pot2y[x][1] = pot2[1][2][1];
              v = sVector30(x+0.5f,y+1.0f,z+1.0f) * S + tpos;  func(v,pot2[2][2][1],fi);  pot2y[x][2] = pot2[2][2][1];
              v = sVector30(x+1.0f,y+1.0f,z+0.5f) * S + tpos;  func(v,pot2[1][2][2],fi);  pot2y[x][3] = pot2[1][2][2];
              LastY = y;
              lastyz[x] = z;

              // do the rest, don't bother caching

              v = sVector30(x+0.5f,y+0.5f,z+0.0f) * S + tpos;  func(v,pot2[0][1][1],fi);
              v = sVector30(x+0.5f,y+0.5f,z+0.5f) * S + tpos;  func(v,pot2[1][1][1],fi);
              v = sVector30(x+0.5f,y+0.5f,z+1.0f) * S + tpos;  func(v,pot2[2][1][1],fi);

              v = sVector30(x+1.0f,y+0.5f,z+0.0f) * S + tpos;  func(v,pot2[0][1][2],fi);
              v = sVector30(x+1.0f,y+0.5f,z+0.5f) * S + tpos;  func(v,pot2[1][1][2],fi);
              v = sVector30(x+1.0f,y+0.5f,z+1.0f) * S + tpos;  func(v,pot2[2][1][2],fi);

              // render it

              MC.March_1_1(&pot2[0][0][0],S/2,tpos+sVector30(x*S,y*S,z*S),thread);
            }
          }
        }
      }
    }
  }
}

template<class T>
void RNMarchingCubesBase<T>::Render()
{
  sStsCode code = 0;

  if(Para.Subdivide)
  {
    switch(Para.BaseGrid)
    {
      case 0: code = StaticRenderT<T,0,1>; break;
      case 1: code = StaticRenderT<T,1,1>; break;
      case 2: code = StaticRenderT<T,2,1>; break;
      case 3: code = StaticRenderT<T,3,1>; break;
      case 4: code = StaticRenderT<T,4,1>; break;
      case 5: code = StaticRenderT<T,5,1>; break;
    }
  }
  else
  {
    switch(Para.BaseGrid)
    {
      case 0: code = StaticRenderT<T,0,0>; break;
      case 1: code = StaticRenderT<T,1,0>; break;
      case 2: code = StaticRenderT<T,2,0>; break;
      case 3: code = StaticRenderT<T,3,0>; break;
      case 4: code = StaticRenderT<T,4,0>; break;
      case 5: code = StaticRenderT<T,5,0>; break;
    }
  }
  sVERIFY(code);

  // finish work

  if(Workload)
  {
    Workload->Sync();
    Workload->End();
    Workload = 0;
  }

  if(Para.Multithreading==2)
    MC.End();

  // reorganize memory

  const sInt s = 1<<Para.BaseGrid;
  sInt size = (s+2)*(s+1)*(s+1);
  if(size>PotSize)
  {
    PotSize = size;
    for(sInt i=0;i<MaxThread;i++)
    {
      delete[] PotData[i];
      PotData[i] = (typename T::FieldType *) sAllocMem(PotSize*sizeof(typename T::FieldType),16,0);
    }
  }
  sInt maxparts = 0;
  HashContainer *hc;
  sFORALL(NodeHashConts,hc)
  {
    sInt hcp = 0;
    PartContainer *nc = hc->FirstPart;
    while(nc)
    {
      hcp += nc->Count;
      nc = nc->Next;
    }
    maxparts = sMax(maxparts,hcp);
  }
  if(maxparts>SimdCount)
  {
    SimdCount = maxparts;
    for(sInt i=0;i<MaxThread;i++)
    {
      delete[] SimdParts[i];
      SimdParts[i] = (typename T::SimdType *) sAllocMem(SimdCount*sizeof(typename T::SimdType),16,0);
    }
  }

  // do the work

  FreeHashConts.Add(ThreadHashConts);       // thread -> free
  FreePartConts.Add(ThreadPartConts);
  ThreadHashConts.Clear();
  ThreadPartConts.Clear();
  NodeHashConts.Swap(ThreadHashConts);      // node -> thread
  NodePartConts.Swap(ThreadPartConts);

  MC.Begin();
  if(Para.Multithreading==0)
  {
    for(sInt i=0;i<ThreadHashConts.GetCount();i++)
      (*code)(0,0,i,1,this);
  }
  else
  {
    Workload = sSched->BeginWorkload();
    Workload->AddTask(Workload->NewTask(code,this,ThreadHashConts.GetCount(),0));
    Workload->Start();
    if(Para.Multithreading!=2)
    {
      while(sSched->HelpWorkload(Workload))
        MC.ChargeGeo();
      Workload->Sync();
    }
  }
  if(Para.Multithreading!=2)
    MC.End();
}

/****************************************************************************/

template<class T>
void RNMarchingCubesBase<T>::Render(Wz4RenderContext *ctx)
{
  if(ctx->IsCommonRendermode() && PInfo.Used>0)
  {
    if(Mtrl->SkipPhase(ctx->RenderMode,Para.LightEnv&15)) return;
    sMatrix34CM *model;
    sFORALL(Matrices,model)
    {
      Mtrl->Set(ctx->RenderMode|sRF_MATRIX_ONE,Para.LightEnv&15,model,0,0,0);
      MC.Draw();
    }
  } 
}

/****************************************************************************/
/****************************************************************************/

template class RNMarchingCubesBase<RNMarchingCubesTemplate>;
template class RNMarchingCubesBase<RNMarchingCubesColorTemplate>;

/****************************************************************************/
