// This file is distributed under a BSD license. See LICENSE.txt for details.


#include "geneffectdebris.hpp"
#include "genminmesh.hpp"
#include "genblobspline.hpp"
#include "genscene.hpp"
#include "genmaterial.hpp"
#include "engine.hpp"

#if !sPLAYER
extern sBool SceneWireframe;
extern sInt SceneWireFlags;
extern sU32 SceneWireMask;
#endif

/****************************************************************************/
/***                                                                      ***/
/***   Walk the spline                                                    ***/
/***                                                                      ***/
/****************************************************************************/

KObject * __stdcall Init_Effect_WalkTheSpline(class GenMinMesh *mesh,class GenSpline *spline,sF32 animpos,sF32 animwalk,sInt count,sInt seed,sF32 side,sF32 middle)
{
  EngMesh *engmesh;
  if(spline) spline->Release();
  if(mesh && mesh->ClassId==KC_MINMESH)
  {
#if !sPLAYER
    if(SceneWireframe)
    {
      mesh->PrepareWire(SceneWireFlags,SceneWireMask);
      engmesh = mesh->WireMesh;
    }
    else
    {
      mesh->UnPrepareWire();
      mesh->Prepare();
      engmesh = mesh->PreparedMesh;
    }
#else
    mesh->Prepare();
    engmesh = mesh->PreparedMesh;
#endif
#if sPLAYER
    engmesh->Preload();
#endif
  }
  if(mesh) mesh->Release();
  return new GenScene;
}

/****************************************************************************/

void __stdcall Exec_Effect_WalkTheSpline(KOp *op,KEnvironment *kenv,sF32 animpos,sF32 animwalk,sInt count,sInt seed,sF32 side,sF32 middle)
{
  GenMinMesh *mesh = (GenMinMesh *)op->GetInput(0)->Cache;
  GenSpline *spline = (GenSpline *) op->GetInput(1)->Cache;
  sVERIFY(mesh);
  sVERIFY(spline);

  sMatrix mat,mat0,mat1;
  sF32 dummy,a,s;

  if(mesh && mesh->ClassId==KC_MINMESH)
  {
    EngMesh *engmesh;
  #if !sPLAYER
    if(SceneWireframe)
      engmesh = mesh->WireMesh;
    else
      engmesh = mesh->PreparedMesh;
  #else
    engmesh = mesh->PreparedMesh;
  #endif
    if(engmesh)
    {
      sSetRndSeed(seed);
      animpos = sFMod(animpos,1);
      mat0 = kenv->ExecStack.Top();
      for(sInt i=0;i<count;i++)
      {
        s = sFGetRnd(2)-1;
        a = animpos+sFGetRnd();
        if(a>1.0f) a = a-1.0f;
        if(s<0)
        {
          spline->Eval(1-a,0,mat1,dummy);
          mat.MulA(mat1,mat0);
          mat.l.AddScale3(mat.i,side*s - middle);
          mat.i.Scale3(-1);
          mat.k.Scale3(-1);
          Engine->AddPaintJob(engmesh,mat,animwalk,0);
        }
        else
        {
          spline->Eval(a,0,mat1,dummy);

          mat.MulA(mat1,mat0);
          mat.l.AddScale3(mat.i,side*s + middle);
          Engine->AddPaintJob(engmesh,mat,animwalk,0);
        }
      }
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Chain Line                                                         ***/
/***                                                                      ***/
/****************************************************************************/

KObject * __stdcall Init_Effect_ChainLine(GenMaterial *mtrl,
                                          sF323 posa,sF323 posb,sInt marka,sInt markb,sF32 dist,sF32 thick,sF32 gravity,sF32 damp,sF32 spring,
                                          sF32 rip,sF32 windspeed,sF32 windforce,sInt flags,sF323 basewind,sInt rippoint)
{
  return MakeEffect(mtrl);
}

/****************************************************************************/

static const sInt CLMax = 32;

struct CLVerlet
{
  sVector OldPos;
  sVector Pos;
  sVector NewPos;
};

struct CLData : public KInstanceMem
{
  CLVerlet Lines[CLMax];
  sInt Ripped;
  sInt ColAxis;
  sInt TimeCounter;
  sInt FirstCycles;

  sVector OldA,OldB;
};

void __stdcall Exec_Effect_ChainLine(KOp *op,KEnvironment *kenv,
                                     sF323 _posa,sF323 _posb,sInt marka,sInt markb,sF32 dist,sF32 thick,sF32 gravity,sF32 damp,sF32 spring,
                                     sF32 rip,sF32 windspeed,sF32 windforce,sInt flags,sF323 basewind,sInt rippoint)
{
  GenMaterial *mtrl;
  sVector posa,posb;
  sVector d,tempv;
  sF32 len;
  sVector wind;
  sVector winddir;
  sVector NewA,NewB;
  sF32 ColMin;
  sF32 ColMax;

  // check and convert parameters


  sVERIFY(marka>=0 && marka<sCOUNTOF(kenv->Markers));
  sVERIFY(markb>=0 && markb<sCOUNTOF(kenv->Markers));

  NewA.Init3(_posa.x,_posa.y,_posa.z);
  NewB.Init3(_posb.x,_posb.y,_posb.z);
  NewA.Rotate34(kenv->Markers[marka]);
  NewB.Rotate34(kenv->Markers[markb]);

  dist = dist / (CLMax-1);
  gravity = gravity * gravity;

  // initialize instance data

  CLData *mem = kenv->GetInst<CLData>(op);
  if(mem->Reset)
  {
    d.Sub3(NewA,NewB);
    if(flags & 4)
    {
      for(sInt i=0;i<CLMax;i++)
      {
        mem->Lines[i].Pos = NewA;
        mem->Lines[i].Pos.y -= i*dist;
        mem->Lines[i].OldPos = mem->Lines[i].Pos;
      }
      mem->Ripped = CLMax-1;
    }
    else
    {
      sF32 h;
      sF32 delta;
      delta = d.Dot3(d);
      h = (dist*(CLMax-1)*dist*(CLMax-1))-delta;
      if(h<=0)
        h = 0;
      else
        h = sFSqrt(h)*0.4f;
      for(sInt i=0;i<CLMax;i++)
      {
        sF32 n = 1.0f*i/(CLMax-1);
        sF32 m = (i-(CLMax-1)/2.0f)/((CLMax-1)/2.0f);
        mem->Lines[i].Pos.Lin3(NewA,NewB,n);
        mem->Lines[i].Pos.y -= (1-m*m)*h;
        if(mem->Lines[i].Pos.y<0) mem->Lines[i].Pos.y = 0;
        mem->Lines[i].OldPos = mem->Lines[i].Pos;
      }
      mem->Ripped = -1;
    }
    
    if(sFAbs(d.x) > sFAbs(d.y))
    {
      if(sFAbs(d.x) > sFAbs(d.z))
        mem->ColAxis = 0;
      else
        mem->ColAxis = 2;
    }
    else
    {
      if(sFAbs(d.y) > sFAbs(d.z))
        mem->ColAxis = 1;
      else
        mem->ColAxis = 2;
    }
    mem->TimeCounter = 0;
    mem->FirstCycles = 250;
    mem->OldA = NewA;
    mem->OldB = NewB;
  }


  // physical simulation

  sInt timeslices = kenv->TimeSlices;
#if !sPLAYER
  if(timeslices > 50)
    timeslices = 50;
#endif

  if(!(flags & 8)) for(sInt t=0;t<timeslices*10;t++)
  {
    posa.Lin3(mem->OldA,NewA,(t+1)*1.0f/(timeslices*10));
    posb.Lin3(mem->OldB,NewB,(t+1)*1.0f/(timeslices*10));

    ColMin = posa[mem->ColAxis];
    ColMax = posb[mem->ColAxis];
    if(ColMin>ColMax)
      sSwap(ColMin,ColMax);

    sF32 wtimer = (mem->TimeCounter + t)*windspeed; // wind speed
    d.Init(wtimer*1.0f,wtimer*1.1f,wtimer*1.2f);
    sPerlin3D(d,wind);
    wind.Scale3(windforce);
    wind.y *= 0.5f;
    wind.x += basewind.x;
    wind.y += basewind.y;
    wind.z += basewind.z;
    winddir = wind;
    winddir.UnitSafe3();

    for(sInt i=0;i<CLMax;i++)                 // per element loop
    {
      CLVerlet *v = &mem->Lines[i];
      v->NewPos = v->Pos;                     // verlet step
      if(mem->FirstCycles==0)
      {
        v->NewPos.AddScale3(v->Pos,damp);
        v->NewPos.AddScale3(v->OldPos,-damp);
      }
      else
      {
        v->NewPos.AddScale3(v->Pos,0.975f);
        v->NewPos.AddScale3(v->OldPos,-0.975f);
      }

      v->NewPos.y -= gravity;                 // gravity
    }

    for(sInt i=0;i<CLMax-1;i++)               // between elements loop
    {
      CLVerlet *a = &mem->Lines[i];
      CLVerlet *b = &mem->Lines[i+1];
      d.Sub3(b->Pos,a->Pos);

      tempv.Cross3(d,winddir);
      sF32 wf = tempv.Abs3();                 // wind
      a->NewPos.AddScale3(wind,wf*0.0001f);

      if(i!=mem->Ripped)
      {
        len = d.UnitAbs3();                   // spring force
        if(len>dist)
        {
          a->NewPos.AddScale3(d,(len-dist)*spring);
          b->NewPos.AddScale3(d,-(len-dist)*spring);
        }
      }
    }

    for(sInt i=0;i<CLMax;i++)                 // constraint
    {
      CLVerlet *v = &mem->Lines[i];

      if(v->NewPos.y < 0)                     // ground
        v->NewPos.y = 0;

      sInt j = mem->ColAxis;
      switch(flags & 3)
      {
      case 0:
        break;
      case 1:
        if(v->NewPos[j]<ColMin)
        {
          v->Pos[j] -= v->NewPos[j]-ColMin;
          v->NewPos[j] = ColMin;
        }
        if(v->NewPos[j]>ColMax)
        {
          v->Pos[j] -= v->NewPos[j]-ColMax;
          v->NewPos[j] = ColMax;
        }
        break;
      case 2:
        if(v->NewPos[j]<ColMin)
          v->NewPos[j] = ColMin;
        if(v->NewPos[j]>ColMax)
          v->NewPos[j] = ColMax;
        break;
      }
    }

    mem->Lines[0].NewPos = posa;              // endpoint constraints
    if(!(flags & 4))
      mem->Lines[CLMax-1].NewPos = posb;

    if(mem->Ripped==-1)
    {
      d.Sub3(posa,posb);
      if(d.Dot3(d)>rip*rip)
        mem->Ripped = rippoint;
    }

    for(sInt i=0;i<CLMax;i++)                 // copy to next step
    {
      CLVerlet *v = &mem->Lines[i];
      v->OldPos = v->Pos;
      v->Pos = v->NewPos;
    }
    if(mem->FirstCycles>0)
      mem->FirstCycles--;
  }
  mem->TimeCounter += kenv->TimeSlices*10;
  mem->OldA = NewA;
  mem->OldB = NewB;

  // drawing

  mtrl = (GenMaterial *) op->GetLinkCache(0);
  if(mtrl && mtrl->Passes.Count>0)
  {
    sInt geo;
    sF32 *fp;
    sU16 *ip;
    sVector v,s,a,b,d;
    sMatrix view;

    sSystem->GetTransform(sGT_MODELVIEW,view);
    s.Init(view.i.z,view.j.z,view.k.z);
    s.Unit3();
    kenv->CurrentCam.CameraSpace.Init();
    mtrl->Passes[0].Mtrl->Set(kenv->CurrentCam);
    geo = sSystem->GeoAdd(sFVF_TSPACE3,sGEO_TRI);
    sSystem->GeoBegin(geo,CLMax*2,(CLMax-1)*6,&fp,(void **)&ip);
    for(sInt i=0;i<CLMax;i++)
    {
      a = mem->Lines[sMax(0,i-1)].Pos;
      b = mem->Lines[sMin(CLMax-1,i+1)].Pos;
      d.Sub3(a,b);
      d.UnitSafe3();
      a.Cross3(d,s);
      a.Scale3(thick/2);

      v = mem->Lines[i].Pos;
      fp[0] = v.x+a.x;
      fp[1] = v.y+a.y;
      fp[2] = v.z+a.z;
      fp[3] = 0;
      fp[4] = 0;
      *((sU32 *)(&fp[5])) = 0xff404040;
      fp[6] = i;
      fp[7] = 0;
      fp += 8;

      fp[0] = v.x-a.x;
      fp[1] = v.y-a.y;
      fp[2] = v.z-a.z;
      fp[3] = 0;
      fp[4] = 0;
      *((sU32 *)(&fp[5])) = 0xff404040;
      fp[6] = i;
      fp[7] = 1;
      fp += 8;
    }
    for(sInt i=0;i<CLMax-1;i++)
    {
      if(i!=mem->Ripped)
      {
        sQuad(ip,i*2+3,i*2+2,i*2+0,i*2+1);
      }
      else
      {
        sQuad(ip,i*2+0,i*2+1,i*2+0,i*2+1);
      }
    }
    sSystem->GeoEnd(geo);
    sSystem->GeoDraw(geo);
    sSystem->GeoRem(geo);
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   BillboardField                                                     ***/
/***                                                                      ***/
/****************************************************************************/

struct BFData : public KInstanceMem
{
  sInt Count;
  sInt Max;
  sInt Seed;
  sF32 MinDist;
  sF32 HeightRnd;
  sVector *Data;
};

KObject * __stdcall Init_Effect_BillboardField(class GenMaterial *mtrl,sF32 size,sF32 aspect,sF32 maxdist,sInt mode,sF323 r0,sF323 r1,sF32 anim,sInt count,sInt seed,sF32 heightrnd,sF32 mindist)
{
  sInt pass = 0;
  if(mtrl)
  {
    mtrl->Release();
    pass = mtrl->Passes[0].Pass;
  }
  GenEffect *eff = new GenEffect;
  eff->Pass = pass;
  return eff;
}

void __stdcall Exec_Effect_BillboardField(KOp *op,KEnvironment *kenv,sF32 size,sF32 aspect,sF32 maxdist,sInt mode,sF323 r0,sF323 r1,sF32 anim,sInt max,sInt seed,sF32 heightrnd,sF32 mindist)
{
  GenMaterial *mtrl;
  sVector v;
  sMatrix mat,mat0,mat1;
  sInt count;
  sF32 dist,dpos;

  static const sInt MAXBILL = 10240;
  static const sInt MAXOT = 1024;


  static BillboardOTElem Buffer[MAXBILL];
  static BillboardOTElem *OT[MAXOT];


  BFData *mem = kenv->GetInst<BFData>(op);
  if(mem->Reset || mem->MinDist!=mindist || mem->Seed!=seed || mem->Max!=max || mem->HeightRnd!=heightrnd)
  {
    if(!mem->Reset)
      delete[] mem->Data;
    mem->Max = max;
    mem->MinDist = mindist;
    mem->HeightRnd = heightrnd;
    mem->Seed = seed;
    mem->Count = 0;
    mem->Data = new sVector[max];
    mem->DeleteArray = mem->Data;

    sSetRndSeed(seed);

    for(sInt i=0;i<max;i++)
    {
      v.x = sFGetRnd();
      v.y = sFGetRnd();
      v.z = sFGetRnd();
      v.w = 1+sFGetRnd()*heightrnd;

      for(sInt j=0;j<mem->Max;j++)
      {
        sF32 x = (mem->Data[j].x-v.x)*(r1.x-r0.x);
        sF32 y = (mem->Data[j].y-v.y)*(r1.y-r0.y);
        sF32 z = (mem->Data[j].z-v.z)*(r1.z-r0.z);
        if(x*x+y*y+z*z<mindist*mindist)
          goto skip;
      }

      mem->Data[mem->Count++] = v;
skip:;
    }
  }
 
  mtrl = (GenMaterial *) op->GetInput(0)->Cache;
  if(mtrl && mtrl->Passes.Count>0)
  {
    mat0 = kenv->ExecStack.Top();               // model matrix
    mat1 = kenv->CurrentCam.CameraSpace;        // camera matrix
    sSystem->GetTransform(sGT_MODELVIEW,mat);   // align the billboards by this
    mat.Trans3();
    dpos = mat1.l.x*mat1.k.x + mat1.l.y*mat1.k.y + mat1.l.z*mat1.k.z;

    sSetRndSeed(17);

    if(anim>0)
      anim = sFMod(anim,1);

    count = 0;
    for(sInt i=0;i<MAXOT;i++)
      OT[i] = 0;
    for(sInt i=0;i<mem->Count && i<MAXBILL;i++)
    {
      BillboardOTElem *e = &Buffer[i];
      v = mem->Data[i];
      v.z += anim;
      if(v.z>0)
      {
        if(v.z>1)
          v.z -= 1;
        v.x = v.x * (r1.x-r0.x)+r0.x;
        v.y = v.y * (r1.y-r0.y)+r0.y;
        v.z = v.z * (r1.z-r0.z)+r0.z;
        e->size = v.w;
        v.Rotate34(mat0);
        e->x = v.x;
        e->y = v.y;
        e->z = v.z;
        e->uvval = i&7;

        dist = - dpos + (e->x*mat1.k.x + e->y*mat1.k.y + e->z*mat1.k.z);
        sInt id = sInt(dist * (MAXOT/maxdist));
        if(id>=0 && id<MAXOT)
        {
          e->Next = OT[id];
          OT[id] = e;
          count++;
        }
      }
    }

    if(count>0)
    {
      mtrl->Passes[0].Mtrl->Set(kenv->CurrentCam);
      PaintBillboards(kenv,count,OT,MAXOT,size,aspect,mode);
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/
