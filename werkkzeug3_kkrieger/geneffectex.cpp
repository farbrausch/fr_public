// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "geneffect.hpp"
#include "genmaterial.hpp"
#include "genbitmap.hpp"
#include "genmesh.hpp"
#include "genminmesh.hpp"
#include "kkriegergame.hpp"
#include "genoverlay.hpp"
#include "engine.hpp"
#include "material11.hpp"
#include "_util.hpp"
#include <xmmintrin.h>
#include "geneffectex.hpp"

extern sF32 GlobalFps;


/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Effekte ohne besonderen Grund...                                   ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/


void SetVert2(sF32 *fp,sF32 x,sF32 y,sF32 z,sF32 u,sF32 v,sU32 col=~0)
{
  fp[0] = x;
  fp[1] = y;
  fp[2] = z;
  fp[3] = 0;
  fp[4] = 0;
  *((sU32 *)(&fp[5])) = col;
  fp[6] = u;
  fp[7] = v;
}

/****************************************************************************/
/***                                                                      ***/
/***   Starter-Effekt.                                                    ***/
/***                                                                      ***/
/***   Für copy & paste                                                   ***/
/***                                                                      ***/
/****************************************************************************/

void Chaos0(KOp *op,KEnvironment *kenv,sF323 a,sF323 b,sF323 c,sInt flags,sInt effect,sInt seed,sInt count)
{
  GenMaterial *mtrl;
  sInt geo;
  sU16 *ip;
  sVertexDouble *vp;
  sInt i;

  mtrl = (GenMaterial *) op->GetLinkCache(0);
  if(!(mtrl && mtrl->Passes.Count>0)) return;

  geo = sSystem->GeoAdd(sFVF_DOUBLE,sGEO_TRI);
  mtrl->Passes[0].Mtrl->Set(kenv->CurrentCam);
  sSystem->GeoBegin(geo,4*count,6*count,(sF32 **)&vp,(void **)&ip);

  for(i=0;i<count;i++)
  {
    vp[0+i*4].Init(-1,-1,i*a.z,~0,0,0,0,0);
    vp[1+i*4].Init( 1,-1,i*a.z,~0,0,1,0,1);
    vp[2+i*4].Init(-1, 1,i*a.z,~0,1,0,1,0);
    vp[3+i*4].Init( 1, 1,i*a.z,~0,1,1,1,1);
  }
  for(i=0;i<count;i++)
  {
    sQuad(ip,i*4+0,i*4+1,i*4+3,i*4+2);
  }
  sSystem->GeoEnd(geo);
  sSystem->GeoDraw(geo);
  sSystem->GeoRem(geo);
}

/****************************************************************************/
/***                                                                      ***/
/***   Einige sich anziehende Gravitationsquellen.                        ***/
/***                                                                      ***/
/***   Die Simulation wird jeden Frame mit neuen Startwerten durchgeführt.***/
/***   Aus den Bewegungsbahnen werden Bänder dargestellt.                 ***/
/***                                                                      ***/
/***   Divisionen durch Null werden noch nicht abgefangen.                ***/
/***                                                                      ***/
/****************************************************************************/

void Chaos1(KOp *op,KEnvironment *kenv,sF323 a,sF323 b,sF323 c,sInt flags,sInt effect,sInt seed,sInt maxcount)
{
  const sInt pcount = 8;
  const sInt steps = 250;
  GenMaterial *mtrl;
  sInt geo;
  sF32 *fp;
  sU16 *ip;
  sInt i,j,k;
  sF32 s;
  sMatrix Pos[pcount],mat;
  sVector Speed[pcount];
  sVector d;
  sF32 dist,t;
  sU32 col;
  sInt invert;

  static sMatrix record[pcount][steps];

  mtrl = (GenMaterial *) op->GetLinkCache(0);
  if(!(mtrl && mtrl->Passes.Count>0)) return;

  geo = sSystem->GeoAdd(sFVF_TSPACE3,sGEO_TRI);
  s = 0.25f;

  for(invert = -1;invert<=1;invert+=2)
  {
    sSetRndSeed(seed);
    t = a.z+24;
    for(i=0;i<pcount;i++)
    {
      Pos[i].Init();
      Pos[i].l.x = sFSin(t*sFGetRnd())*c.x;
      Pos[i].l.y = sFSin(t*sFGetRnd())*c.y;
      Pos[i].l.z = sFSin(t*sFGetRnd())*c.z;
      Pos[i].l.w = 1.0f;
      Pos[i].k.InitRnd();
      Pos[i].k.x *= b.x*invert;
      Pos[i].k.y *= b.y*invert;
      Pos[i].k.z *= b.z*invert;
      Pos[i].j.Init(0,0,0,0);
    }
    Pos[0].Init();
    Speed[0].Init(0,0,0,0);

    for(i=0;i<steps;i++)
    {
      for(j=0;j<pcount-1;j++)
      {
        for(k=j+1;k<pcount;k++)
        {
          d.Sub3(Pos[j].l,Pos[k].l);
          dist = d.Dot3(d);
          d.Scale3(dist);

          Pos[j].j.Sub3(d);
          Pos[k].j.Add3(d);
        }
      }
      Pos[0].Init();
      Speed[0].Init(0,0,0,0);
      for(j=0;j<pcount;j++)
      {
        record[j][i] = Pos[j];
        Pos[j].k.AddScale3(Pos[j].j,a.x*0.0001f);
        Pos[j].l.AddScale3(Pos[j].k,a.y*0.01f);
        Pos[j].j.Init(0,0,0,0);
      }
    }

    for(j=1;j<pcount;j++)
    {
      mtrl->Passes[0].Mtrl->Set(kenv->CurrentCam);
      sSystem->GeoBegin(geo,steps*2,(steps-1)*6,&fp,(void **)&ip);
      for(i=0;i<steps;i++)
      {
        mat = record[j][i];
//        s = mat.j.Abs3()*0.00003f;//sRange<sF32>(2-mat.j.Abs3(),2,1)*0.25f;
        mat.k.Unit3();
        mat.i.Cross3(mat.j,mat.k);
        mat.i.Unit3();
        mat.j.Cross3(mat.k,mat.i);
        col = 255-(i*255/steps);
        col = 0xff000000|(col<<16)|(col<<8)|(col<<0);
        SetVert2(fp,mat.l.x+mat.i.x*s,mat.l.y+mat.i.y*s,mat.l.z+mat.i.z*s,0,i*1.0f*invert,col);
        fp += 8;
        SetVert2(fp,mat.l.x-mat.i.x*s,mat.l.y-mat.i.y*s,mat.l.z-mat.i.z*s,1,i*1.0f*invert,col);
        fp += 8;
      }
      for(i=0;i<steps-1;i++)
      {
        sQuad(ip,i*2+0,i*2+1,i*2+3,i*2+2);
      }
      sSystem->GeoEnd(geo);
      sSystem->GeoDraw(geo);
    }
  }

  sSystem->GeoRem(geo);
}

/****************************************************************************/
/***                                                                      ***/
/***   UV-Animator                                                        ***/
/***                                                                      ***/
/***   Ein Quad wird mehrfach gezeichnet.                                 ***/
/***   Dabei werden zwei paare UV-Koordinaten rotiert, verschoben und     ***/
/***   skaliert.                                                          ***/
/***   Man sollte damit zwei Texturen multiplizieren und additiv blenden. ***/
/***                                                                      ***/
/****************************************************************************/

void Chaos2(KOp *op,KEnvironment *kenv,sF323 a,sF323 b,sF323 c,sInt flags,sInt effect,sInt seed,sInt maxcount)
{
  GenMaterial *mtrl;

  sInt geo;
  sU16 *ip;
  sInt i;
  sInt steps;
  sVertexDouble *vp;
  sF32 s0,c0,s1,c1;
  sF32 r0,r1,z;

  mtrl = (GenMaterial *) op->GetLinkCache(0);
  if(!(mtrl && mtrl->Passes.Count>0)) return;

  steps = maxcount;
  geo = sSystem->GeoAdd(sFVF_DOUBLE,sGEO_TRI);
  mtrl->Passes[0].Mtrl->Set(kenv->CurrentCam);
  sSystem->GeoBegin(geo,steps*4,(steps)*6,(sF32 **)&vp,(void **)&ip);

  r0 = a.x;
  r1 = a.y;
  z  = a.z;

  for(i=0;i<steps;i++)
  {
    s0 = sFSin(r0)*z;
    c0 = sFCos(r0)*z;
    s1 = sFSin(-r0)*z;
    c1 = sFCos(-r0)*z;

    vp[0+i*4].Init(-1,-1,0,~0, s0+r1, c0, s1-r1, c1);
    vp[1+i*4].Init( 1,-1,0,~0, c0+r1,-s0, c1-r1,-s1);
    vp[2+i*4].Init(-1, 1,0,~0,-c0+r1, s0,-c1-r1, s1);
    vp[3+i*4].Init( 1, 1,0,~0,-s0+r1,-c0,-s1-r1,-c1);

    r0 += b.x/steps;
    r1 += b.y/steps;
    z  += b.z/steps;
  }
  for(i=0;i<steps;i++)
  {
    sQuad(ip,i*4+0,i*4+1,i*4+3,i*4+2);
  }
  sSystem->GeoEnd(geo);
  sSystem->GeoDraw(geo);
  sSystem->GeoRem(geo);
}

/****************************************************************************/
/***                                                                      ***/
/***   Plane-Partition                                                    ***/
/***                                                                      ***/
/***   Unterteile eine ebene immer wieder.                                ***/
/***                                                                      ***/
/****************************************************************************/

struct Chaos3Cell
{
  sVector Vertex[4];
  sInt Level;
  sInt Leaf;
  sU32 Color;
  sF32 Wx,Wy;
};


void Chaos3(KOp *op,KEnvironment *kenv,sF323 a,sF323 b,sF323 c,sInt flags,sInt effect,sInt seed,sInt count)
{
  GenMaterial *mtrl;
  sInt geo;
  sU16 *ip;
  sVertexDouble *vp;
  sInt i,j,cc,cd;
  const sInt maxcell = 1024;
  sInt max;
  Chaos3Cell *cp,*ca,*cb;
  static Chaos3Cell cells[maxcell];
  sVector v1,v2,v;
  sVector vd[4];
  sF32 f,fa,fb;
  sU32 col;

  mtrl = (GenMaterial *) op->GetLinkCache(0);
  if(!(mtrl && mtrl->Passes.Count>0)) return;
  sSetRndSeed(seed);

// generate cells

  cc = 0;
  cd = 0;
  cp = &cells[cc++];
  cp->Level = 0;
  cp->Leaf = 0;
  cp->Color = sGetRnd()|0xff808080;
  cp->Wx = 1.0f;
  cp->Wy = 1.0f;
  cp->Vertex[0].Init(-16,0,-16,1);
  cp->Vertex[1].Init( 16,0,-16,1);
  cp->Vertex[2].Init( 16,0, 16,1);
  cp->Vertex[3].Init(-16,0, 16,1);

  for(;cc<count && cc<maxcell-10 && cd<=cc;)
  {
    cp = &cells[cd++];       // divide this cell

    if(cp->Level>=2 && sFGetRnd()<a.y)  continue;    // don't divide this cell..

    ca = &cells[cc++];      // the two new cells..
    cb = &cells[cc++];

    *ca = *cp;
    *cb = *cp;
    cp->Leaf = 0;
    ca->Level++;
    ca->Leaf = 1;
    ca->Color = sGetRnd()|0xff808080;
    cb->Level++;
    cb->Leaf = 1;
      cb->Color = sGetRnd()|0xff808080;

    v1.Sub3(cp->Vertex[0],cp->Vertex[3]);
    v1.Add3(cp->Vertex[1]);
    v1.Sub3(cp->Vertex[2]);
    v2.Sub3(cp->Vertex[0],cp->Vertex[1]);
    v2.Add3(cp->Vertex[3]);
    v2.Sub3(cp->Vertex[2]);
    fa = v1.Abs3();
    fb = v2.Abs3();

    if(cp->Wx/cp->Wy>1.0f+(sFGetRnd(2)-1.0f)*a.z)         // by x or z axxis?
    {
      fa = sFGetRnd(b.x)+(1.0f-b.x)/2;
      fb = (sFGetRnd(1)-0.5f)*(b.y*2/(cp->Level+2));
      v1.Lin3(cp->Vertex[0],cp->Vertex[1],fa+fb);
      v2.Lin3(cp->Vertex[3],cp->Vertex[2],fa-fb);
      ca->Vertex[0] = v1;
      ca->Vertex[3] = v2;
      cb->Wx = cp->Wx*fa;
      cb->Vertex[1] = v1;
      cb->Vertex[2] = v2;
      ca->Wx = cp->Wx*(1-fa);
    }
    else
    {
      fa = sFGetRnd(b.x)+(1.0f-b.x)/2;
      fb = (sFGetRnd(1)-0.5f)*(b.y*2/(cp->Level+2));
      v1.Lin3(cp->Vertex[0],cp->Vertex[3],fa+fb);
      v2.Lin3(cp->Vertex[1],cp->Vertex[2],fa-fb);
      ca->Vertex[0] = v1;
      ca->Vertex[1] = v2;
      cb->Wy = cp->Wx*fa;
      cb->Vertex[3] = v1;
      cb->Vertex[2] = v2;
      ca->Wy = cp->Wx*(1-fa);
    }
  }

// draw cells

  max = cc;
  geo = sSystem->GeoAdd(sFVF_DOUBLE,sGEO_TRI);
  mtrl->Passes[0].Mtrl->Set(kenv->CurrentCam);
  sSystem->GeoBegin(geo,4*max,6*max,(sF32 **)&vp,(void **)&ip);

  for(i=0;i<max;i++)
  {
    cp = &cells[i];
    
    col = cp->Color;
    vp[0+i*4].Init(0,0,0,col,0,1,0,0);
    vp[1+i*4].Init(0,0,0,col,1,1,0,0);
    vp[2+i*4].Init(0,0,0,col,1,0,0,0);
    vp[3+i*4].Init(0,0,0,col,0,0,0,0);

//    f = cp->Level*a.x;
    f = a.x;
    for(j=0;j<4;j++)
    {
      v.Sub3(cp->Vertex[j],cp->Vertex[(j+1)&3]);  
      v.Unit3();
      vd[j].Init(v.z*f,0,-v.x*f,0);
    }

    for(j=0;j<4;j++)
    {
      vp[j+i*4].x = cp->Vertex[j].x+vd[(j+3)&3].x+vd[(j+0)&3].x;
      vp[j+i*4].y = cp->Level*0.1;
      vp[j+i*4].z = cp->Vertex[j].z+vd[(j+3)&3].z+vd[(j+0)&3].z;
    }
  }
  for(i=0;i<max;i++)
  {
    sQuad(ip,i*4+0,i*4+1,i*4+2,i*4+3);
  }
  sSystem->GeoEnd(geo);
  sSystem->GeoDraw(geo);
  sSystem->GeoRem(geo);
}

/****************************************************************************/
/***                                                                      ***/
/***   Der gute alte Timeslice-Effect (Rubber Cube)                       ***/
/***                                                                      ***/
/***   ... nur dass ich dies mal alle phasen jeden frame neu zeichne,     ***/
/***   in den selben framebuffer (additiv), und mit einer projezierten    ***/
/***   textur das ding in scheiben schneide - World Space                 ***/
/***                                                                      ***/
/***   Ich habe also volle Kontrolle über alle Bewegungen, kann das       ***/
/***   Objekt morphen und alles einfrieren und im 3d-Raum herumfliegen... ***/
/***                                                                      ***/
/****************************************************************************/

void Chaos4(KOp *op,KEnvironment *kenv,sF323 a,sF323 b,sF323 c,sInt flags,sInt effect,sInt seed,sInt count)
{
  GenMaterial *mtrl;
  sInt geo;
  sU16 *ip;
  volatile sVertexDouble *vp;
  sInt i;
  sInt tsx,tsy,ttt,tsl;
  sInt x,y,t,s;
  sF32 ri,ro,fx,fy,scale;
  sVector v;
  sMatrix mat;
  static sVector cache[4096];
  sVector *p;

  mtrl = (GenMaterial *) op->GetLinkCache(0);
  if(!(mtrl && mtrl->Passes.Count>0)) return;

  tsl = count;
  ttt = 4;
  tsx = 12;
  tsy = 24;

  sVERIFY(ttt*(tsx+1)*(tsy+1)<4096);
  p = cache;
  for(t=0;t<ttt;t++)
  {
    ro = 1.00f+t*0.50f;
    ri = 0.20f;

    for(y=0;y<tsy+1;y++)
    {
      for(x=0;x<tsx+1;x++)
      {
        fx = x*sPI2F/tsx;
        fy = y*sPI2F/tsy;

        v.x = sFSin(fy)*(ro+sFSin(fx)*ri);
        v.y = sFCos(fx)*ri;
        v.z = sFCos(fy)*(ro+sFSin(fx)*ri);
        v.w = 1.0f;

        *p++ = v;
      }
    }
  }

  geo = sSystem->GeoAdd(sFVF_DOUBLE,sGEO_TRI);

  for(s=0;s<tsl;s++)
  {
    scale = 2;
    sMaterial11 *mtrl11 = (sMaterial11 *) mtrl->Passes[0].Mtrl;
    mtrl11->SRT1[1] = 0.5f/(scale/(tsl-1.0f));
    mtrl11->SRT1[7] = mtrl11->SRT1[1]*(s/(tsl-1.0f)*scale-(scale/2))+0.5f;
    mtrl11->Set(kenv->CurrentCam);
    sSystem->GeoBegin(geo,(tsx+1)*(tsy+1)*ttt,6*tsx*tsy*ttt,(sF32 **)&vp,(void **)&ip);

    p = cache;
    for(t=0;t<ttt;t++)
    {
      ro = 1.00f+t*0.50f;
      ri = 0.20f;

      mat.InitEuler(
        a.x+b.x*t/ttt+c.x*s/tsl,
        a.y+b.y*t/ttt+c.y*s/tsl,
        a.z+b.z*t/ttt+c.z*s/tsl);

      for(y=0;y<tsy+1;y++)
      {
        for(x=0;x<tsx+1;x++)
        {
          v = *p++;
          v.Rotate34(mat);
          vp->x = v.x;
          vp->y = v.y;
          vp->z = v.z;
          vp->c0 = ~0;
          vp->u0 = x*1.0f/tsx;
          vp->v0 = y*1.0f/tsy;
          vp->u1 = 0;
          vp->v1 = 0;
          vp++;
        }
      }
    }
    i = 0;
    for(t=0;t<ttt;t++)
    {
      for(y=0;y<tsy;y++)
      {
        for(x=0;x<tsx;x++)
        {
          sQuad(ip,
            t*(tsx+1)*(tsy+1)+(y+0)*(tsx+1)+(x+0),
            t*(tsx+1)*(tsy+1)+(y+1)*(tsx+1)+(x+0),
            t*(tsx+1)*(tsy+1)+(y+1)*(tsx+1)+(x+1),
            t*(tsx+1)*(tsy+1)+(y+0)*(tsx+1)+(x+1));
        }
      }
    }
    sSystem->GeoEnd(geo);
    sSystem->GeoDraw(geo);
  }
  sSystem->GeoRem(geo);
}


/****************************************************************************/
/***                                                                      ***/
/***   Allgemeiner code für die Effekte                                   ***/
/***                                                                      ***/
/****************************************************************************/

KObject * __stdcall Init_Effect_Chaos1(class GenMaterial *mtrl,sF323 a,sF323 b,sF323 c,sInt flags,sInt effect,sInt seed,sInt maxcount)
{
  if(mtrl) mtrl->Release();
  return new GenEffect;
}

void __stdcall Exec_Effect_Chaos1(KOp *op,KEnvironment *kenv,sF323 a,sF323 b,sF323 c,sInt flags,sInt effect,sInt seed,sInt maxcount)
{
  switch(effect)
  {
  case 0:
    Chaos0(op,kenv,a,b,c,flags,effect,seed,maxcount);
    break;
  case 1:
    Chaos1(op,kenv,a,b,c,flags,effect,seed,maxcount);
    break;
  case 2:
    Chaos2(op,kenv,a,b,c,flags,effect,seed,maxcount);
    break;
  case 3:
    Chaos3(op,kenv,a,b,c,flags,effect,seed,maxcount);
    break;
  case 4:
    Chaos4(op,kenv,a,b,c,flags,effect,seed,maxcount);
    break;
  }
}


/****************************************************************************/
/***                                                                      ***/
/***   Tourque Effect                                                     ***/
/***                                                                      ***/
/****************************************************************************/

#if !sINTRO

struct sVector3
{
  sF32 x,y,z;

  void Init(const sVector &v)
  {
    x = v.x;
    y = v.y;
    z = v.z;
  }
};

sMAKEZONE(TourquePart,"TourquePart",0x400000);
sMAKEZONE(TourqueCons,"TourqueCons",0x600000);
sMAKEZONE(TourqueDraw,"TourqueDraw",0x800000);

class TourqueSimulator
{
  enum {
    FieldDim = 32,
    MaskField = FieldDim - 1,
    yStep = FieldDim,
    zStep = FieldDim*FieldDim,
  };

public:
  struct Constraint
  {
    sInt p0,p1;     // particle indices
    sF32 restLenSq; // squared rest length
  };

  sArray<Constraint> Constraints;

private:
  sVector3 *ForceField;
  sInt StepTimer;

  // ---- particles
  static sF32 __forceinline SmoothStep(sF32 x)
  {
    return x*x*(3.0f - 2.0f*x);
  }

  static void __forceinline xLerp(sVector3 &out,const sVector3 *base,sInt d,sF32 t)
  {
    out.x = base->x + t * (base[d].x - base->x);
    out.y = base->y + t * (base[d].y - base->y);
    out.z = base->z + t * (base[d].z - base->z);
  }

  static void __forceinline vLerp(sVector3 &out,const sVector3 &a,const sVector3 &b,sF32 t)
  {
    out.x = a.x + t * (b.x - a.x);
    out.y = a.y + t * (b.y - a.y);
    out.z = a.z + t * (b.z - a.z);
  }

  void MoveParticles(sF32 speed,sF32 damp)
  {
    sZONE(TourquePart);

    sVector3 *oldPos,*newPos;

    oldPos = ParticlePos[0];
    newPos = ParticlePos[1];

    sF32 scale = 4.0f, offset = scale * FieldDim / 2.0f;
    sF32 magic = 12582912.0f; // 1.5*2^23

    // process each particle
    for(sInt i=0;i<ParticleCount;i++,oldPos++,newPos++)
    {
      // position
      sF32 x = oldPos->x * scale + offset;
      sF32 y = oldPos->y * scale + offset;
      sF32 z = oldPos->z * scale + offset;

      // rounded version, fractional part
      sF32 rx = (magic + x) - magic, fx = SmoothStep(x - rx);
      sF32 ry = (magic + y) - magic, fy = SmoothStep(y - ry);
      sF32 rz = (magic + z) - magic, fz = SmoothStep(z - rz);

      // integer grid position
      sInt ix = sInt(rx) & MaskField, nx = (ix + 1) & MaskField;
      sInt iy = sInt(ry) & MaskField, ny = (iy + 1) & MaskField;
      sInt iz = sInt(rz) & MaskField, nz = (iz + 1) & MaskField;
      const sVector3 *field = ForceField + ix + iy * yStep + iz * zStep;
      
      sInt deltaX = nx - ix;
      sInt deltaY = (ny - iy) * yStep, deltaZ = (nz - iz) * zStep;

      // calc force
      sVector3 t0,t1,t2,force;

      xLerp(t0,field +      0 +      0,deltaX,fx);
      xLerp(t1,field +      0 + deltaY,deltaX,fx);
      vLerp(t0,t0,t1,fy);
      xLerp(t1,field + deltaZ +      0,deltaX,fx);
      xLerp(t2,field + deltaZ + deltaY,deltaX,fx);
      vLerp(t1,t1,t2,fy);
      vLerp(force,t0,t1,fz);

      // verlet integration
      newPos->x = oldPos->x + (oldPos->x - newPos->x)*damp + force.x*speed;
      newPos->y = oldPos->y + (oldPos->y - newPos->y)*damp + force.y*speed;
      newPos->z = oldPos->z + (oldPos->z - newPos->z)*damp + force.z*speed;
    }
  }

  void Constrain()
  {
    sZONE(TourqueCons);

    Constraint *cons = Constraints.Array;
    sVector3 *parts = ParticlePos[1];

    // enforce stick constraints
    for(sInt i=0;i<Constraints.Count;i++,cons++)
    {
      if(cons->p0 >= ParticleCount || cons->p1 >= ParticleCount)
        continue;

      // get particles
      sVector3 *p0 = parts + cons->p0;
      sVector3 *p1 = parts + cons->p1;

      // difference, squared distance
      sF32 dx = p1->x - p0->x, dy = p1->y - p0->y, dz = p1->z - p0->z;
      sF32 dsq = dx*dx + dy*dy + dz*dz;

      // approximately enforce constraint
      sF32 sc = 0.5f - cons->restLenSq / (dsq + cons->restLenSq);

      p0->x += dx * sc; p1->x -= dx * sc;
      p0->y += dy * sc; p1->y -= dy * sc;
      p0->z += dz * sc; p1->z -= dz * sc;
    }
  }

  void TimeStep(sF32 speed,sF32 damp)
  {
    // swap old and new particles
    sSwap(ParticlePos[0],ParticlePos[1]);

    MoveParticles(speed,damp);
    Constrain();
  }

  // ---- force field
  __forceinline sInt Index(sInt x,sInt y,sInt z)
  {
    return (x & MaskField) + (y & MaskField) * yStep + (z & MaskField) * zStep;
  }

  __forceinline sVector3 &Field(sInt x,sInt y,sInt z)
  {
    return ForceField[Index(x,y,z)];
  }

  void PreprocessField(sF32 postScale = 0.001f)
  {
    // alloc temp space
    sF32 *div = new sF32[FieldDim*FieldDim*FieldDim];
    sF32 *high = new sF32[FieldDim*FieldDim*FieldDim];

    // calc divergence field
    sF32 scale = 1.0f / FieldDim;
    sF32 invScale = 1.0f / scale;

    for(sInt z=0;z<FieldDim;z++)
    {
      for(sInt y=0;y<FieldDim;y++)
      {
        for(sInt x=0;x<FieldDim;x++)
        {
          div[Index(x,y,z)] = -0.5f * scale *
            (Field(x+1,y,z).x - Field(x-1,y,z).x
            +Field(x,y+1,z).y - Field(x,y-1,z).y
            +Field(x,y,z+1).z - Field(x,y,z-1).z);
          high[Index(x,y,z)] = 0.0f;
        }
      }
    }

    // gauss-seidel iteration to calc density field
    for(sInt step=0;step<40;step++)
    {
      for(sInt z=0;z<FieldDim;z++)
      {
        for(sInt y=0;y<FieldDim;y++)
        {
          for(sInt x=0;x<FieldDim;x++)
          {
            high[Index(x,y,z)] = (-6 * div[Index(x,y,z)]
              + high[Index(x-1,y,z)] + high[Index(x+1,y,z)]
              + high[Index(x,y-1,z)] + high[Index(x,y+1,z)]
              + high[Index(x,y,z-1)] + high[Index(x,y,z+1)]) / 6.0f;
          }
        }
      }
    }

    // remove gradients from vector field
    for(sInt z=0;z<FieldDim;z++)
    {
      for(sInt y=0;y<FieldDim;y++)
      {
        for(sInt x=0;x<FieldDim;x++)
        {
          Field(x,y,z).x -= 0.5f * invScale * (high[Index(x+1,y,z)] - high[Index(x-1,y,z)]);
          Field(x,y,z).y -= 0.5f * invScale * (high[Index(x,y+1,z)] - high[Index(x,y-1,z)]);
          Field(x,y,z).z -= 0.5f * invScale * (high[Index(x,y,z+1)] - high[Index(x,y,z-1)]);

          Field(x,y,z).x *= postScale;
          Field(x,y,z).y *= postScale;
          Field(x,y,z).z *= postScale;
        }
      }
    }

    // free temp space
    delete[] div;
    delete[] high;
  }

  void RandomField(sF32 strength = 1.0f)
  {
    // make some random vector field
    for(sInt i=0;i<FieldDim*FieldDim*FieldDim;i++)
    {
      sVector v;
      v.InitRnd();
      v.UnitSafe3();
      v.Scale3(strength);

      ForceField[i].x = v.x;
      ForceField[i].y = v.y;
      ForceField[i].z = v.z;
    }
  }

public:
  enum
  {
    MaxParticles = 0x40000,
  };

  sVector3 *ParticlePos[2];    // 0=old, 1=new (pointers get swapped)
  sInt ParticleCount;         // # of live particles
  sInt TourqueIndex;          // which particle to delete
  sInt TourqueCount;          // max particle count
  sInt TourquePCount;
  sInt TourqueMode;

  TourqueSimulator(sInt max)
  {
    Constraints.Init();
    TourqueCount = max;
    TourqueIndex = 0;
    TourqueMode = -1;
    ForceField = new sVector3[FieldDim*FieldDim*FieldDim];
    ResetField();

    ParticlePos[0] = new sVector3[MaxParticles];
    ParticlePos[1] = new sVector3[MaxParticles];
    ParticleCount = 0;
    StepTimer = 0;

    sSetMem(ParticlePos[0],0,MaxParticles*sizeof(sVector3));
    sSetMem(ParticlePos[1],0,MaxParticles*sizeof(sVector3));
  }

  ~TourqueSimulator()
  {
    Constraints.Exit();
    delete[] ForceField;
    delete[] ParticlePos[0];
    delete[] ParticlePos[1];
  }

  sF32 Advance(sInt ticks,sF32 speed,sF32 damp)
  {
    ticks = sMin(ticks,500); // to limit time spent in simulation
    StepTimer += ticks * 32;
    
    while(StepTimer >= 1000)
    {
      TimeStep(speed,damp);
      StepTimer -= 1000;
    }

    return StepTimer / 1000.0f;
  }

  void ClearConstraints()
  {
    Constraints.Count = 0;
  }

  void AddConstraint(sInt p0,sInt p1,sF32 len)
  {
    Constraint *cons = Constraints.Add();
    cons->p0 = p0;
    cons->p1 = p1;
    cons->restLenSq = len*len;
  }

  void ResetField()
  {
    RandomField();
    PreprocessField();
  }

  void MeshField(GenMesh *mesh,sF32 strength)
  {
    // clear force field
    RandomField(0.001f * strength / 40.0f);
    // twirlify
    PreprocessField(0.001f * strength / 80.0f);

    // calc density field
    sVector *vb = mesh->VertBuf;
    sInt step = mesh->VertSize();
    sInt count = mesh->Vert.Count;
    sF32 *dens = new sF32[FieldDim*FieldDim*FieldDim];

    for(sInt z=0;z<FieldDim;z++)
    {
      for(sInt y=0;y<FieldDim;y++)
      {
        for(sInt x=0;x<FieldDim;x++)
        {
          sVector pos;

          pos.x = (x - FieldDim / 2.0f) / 4.0f;
          pos.y = (y - FieldDim / 2.0f) / 4.0f;
          pos.z = (z - FieldDim / 2.0f) / 4.0f;

          sF32 minDSq = 1e+20f;
          sF32 density = 0.0f;

          for(sInt v=0;v<count;v++)
          {
            const sVector &vp = vb[v];
            sF32 dx = vp.x - pos.x;
            sF32 dy = vp.y - pos.y;
            sF32 dz = vp.z - pos.z;
            sF32 d = dx*dx + dy*dy + dz*dz;
            //minDSq = sMin(d,minDSq);

            density += strength * sMin(1.0f / (d + 0.01f),8.0f);
          }

          /*minDSq += pos.x*pos.x + pos.y*pos.y + pos.z*pos.z;
          dens[Index(x,y,z)] = strength * sMin(1.0f / (minDSq + 0.01f),8.0f);*/
          dens[Index(x,y,z)] = density / count;
        }
      }
    }

    // convert density field to forces
    for(sInt z=0;z<FieldDim;z++)
    {
      for(sInt y=0;y<FieldDim;y++)
      {
        for(sInt x=0;x<FieldDim;x++)
        {
          Field(x,y,z).x -= 0.5f * (dens[Index(x+1,y,z)] - dens[Index(x-1,y,z)]);
          Field(x,y,z).y -= 0.5f * (dens[Index(x,y+1,z)] - dens[Index(x,y-1,z)]);
          Field(x,y,z).z -= 0.5f * (dens[Index(x,y,z+1)] - dens[Index(x,y,z-1)]);
        }
      }
    }

    delete[] dens;
  }
};

struct TourquePart
{
  sVector Pos;
  sVector Speed;
  sVector Side;
};

static TourqueSimulator *TourqueSim[16];

KObject * __stdcall Init_Effect_Tourque(class GenMaterial *mtrl,
  class GenMesh *mesh,sInt Seed,sInt maxcount,sInt Rate,sInt Flags,
  sF32 SizeF,sF32 SizeS,sF32 Speed,sF32 Damp,sF323 Pos,sF323 Range,sInt Slot)
{
  if(mtrl) mtrl->Release();
  if(mesh) // count tris
  {
    sInt nTris = 0;

    mesh->CalcNormals();

    for(sInt i=0;i<mesh->Face.Count;i++)
    {
      if(!mesh->Face[i].Material)
        continue;

      sInt e = mesh->Face[i].Edge, ee = e, count = 0;

      do
      {
        e = mesh->NextFaceEdge(e);
        count++;
      }
      while(e != ee);

      nTris += count-2;
    }

    maxcount = nTris*3;
  }

  maxcount = sMin<sInt>(maxcount,TourqueSimulator::MaxParticles);
  if(TourqueSim[Slot]==0 || TourqueSim[Slot]->TourqueCount!=maxcount)
  {
    sSetRndSeed(Seed);
    if(TourqueSim[Slot])
      delete TourqueSim[Slot];

    sREGZONE(TourquePart);
    sREGZONE(TourqueCons);
    sREGZONE(TourqueDraw);

    TourqueSim[Slot] = new TourqueSimulator(maxcount);

  }

  if((Flags & 3) != TourqueSim[Slot]->TourqueMode)
  {
    TourqueSim[Slot]->ClearConstraints();

    if(mesh)
    {
      sInt nPart = 0;

      TourqueSim[Slot]->MeshField(mesh,0.04f * 16.0f);

      // setup particles to match vertices in mesh. also add constraints
      for(sInt i=0;i<mesh->Face.Count;i++)
      {
        if(!mesh->Face[i].Material)
          continue;

        // collect verts
        sInt verts[64];
        sInt e = mesh->Face[i].Edge, ee = e, count = 0;

        do
        {
          verts[count++] = mesh->GetVertId(e);
          e = mesh->NextFaceEdge(e);
        }
        while(e != ee);

        // build tris
        for(sInt j=2;j<count;j++)
        {
          sVector v0 = mesh->VertPos(verts[0]);
          sVector v1 = mesh->VertPos(verts[j-1]);
          sVector v2 = mesh->VertPos(verts[j]);
          //sF32 speed = sFExp(sFGetRnd() * 11.0f - 20.0f);
          sF32 speed = 0.0f;

          TourqueSim[Slot]->ParticlePos[0][nPart+0].Init(v0);
          TourqueSim[Slot]->ParticlePos[0][nPart+1].Init(v1);
          TourqueSim[Slot]->ParticlePos[0][nPart+2].Init(v2);
          v0.AddScale3(mesh->VertNorm(verts[0]),speed);
          v1.AddScale3(mesh->VertNorm(verts[j-1]),speed);
          v2.AddScale3(mesh->VertNorm(verts[j]),speed);
          TourqueSim[Slot]->ParticlePos[1][nPart+0].Init(v0);
          TourqueSim[Slot]->ParticlePos[1][nPart+1].Init(v1);
          TourqueSim[Slot]->ParticlePos[1][nPart+2].Init(v2);
          TourqueSim[Slot]->AddConstraint(nPart+0,nPart+1,v0.Distance(v1));
          TourqueSim[Slot]->AddConstraint(nPart+1,nPart+2,v1.Distance(v2));
          TourqueSim[Slot]->AddConstraint(nPart+2,nPart+0,v2.Distance(v0));
          nPart += 3;
        }
      }

      TourqueSim[Slot]->TourquePCount = nPart;
      TourqueSim[Slot]->TourqueIndex = nPart;
    }
    else
    {
      switch(Flags&3)
      {
      case 1:
        for(sInt i=0;i<maxcount;i++)
          TourqueSim[Slot]->AddConstraint(i,(i+1)%maxcount,0.05f);
        break;
      case 2:
        for(sInt i=0;i<maxcount;i+=16)
          for(sInt j=0;j<16;j++)
            TourqueSim[Slot]->AddConstraint(i+j,i+((j+1)&15),0.05f);
        break;
      }
    }

    TourqueSim[Slot]->TourqueMode = Flags & 3;
  }

  return new GenEffect;
}

void __stdcall Exec_Effect_Tourque(KOp *op,KEnvironment *kenv,
  sInt Seed,sInt maxcount,sInt Rate,sInt Flags,
  sF32 SizeF,sF32 SizeS,sF32 Speed,sF32 Damp,sF323 Pos,sF323 Range,sInt Slot)

{
  GenMaterial *mtrl;
  sInt geo;
  volatile sVertexTSpace3 *vp;
  sVector side;
  sVector av,ar;
  const sF32 minspeed=0.001f;

  mtrl = (GenMaterial *) op->GetLinkCache(0);
  if(!(mtrl && mtrl->Passes.Count>0)) return;

  sBool triMode = op->GetLink(1) != 0;
  if(triMode)
  {
    maxcount = TourqueSim[Slot]->TourquePCount;
    Flags |= 32;
  }

  sInt time = kenv->CurrentTime & 8191;
  static sBool init = sFALSE;

  static sInt deltaTick = 0;
  static sInt emitTick = 0;
  deltaTick += kenv->TimeDelta;

  av.Init(Pos.x,Pos.y,Pos.z,1);
  ar.Init(Range.x,Range.y,Range.z,1);

  while(deltaTick >= 10)
  {
    if(!(Flags & 32) || TourqueSim[Slot]->TourqueIndex < maxcount)
    {
      for(sInt i=0;i<Rate;i++)
      {
        sVector3 *tp = &TourqueSim[Slot]->ParticlePos[0][TourqueSim[Slot]->TourqueIndex%maxcount];
        sVector v;

        v.InitRnd();
        v.Mul3(ar);
        v.Add3(av);

        tp->x = v.x;
        tp->y = v.y;
        tp->z = v.z;

        TourqueSim[Slot]->ParticlePos[1][TourqueSim[Slot]->TourqueIndex%maxcount] = *tp;
        TourqueSim[Slot]->TourqueIndex++;
      }
    }
    /*for(sInt i=0;i<Rate;i++)
    {
      sVector v;

      v.InitRnd();
      v.Mul3(ar);
      v.Add3(av);
      
      for(sInt j=0;j<3;j++)
      {
        sVector3 *tp = &TourqueSim->ParticlePos[0][TourqueIndex%maxcount];
        tp->x = v.x + ((j == 1) ? 0.10f : 0.0f);
        tp->y = v.y;
        tp->z = v.z + ((j == 2) ? 0.10f : 0.0f);

        TourqueSim->ParticlePos[1][TourqueIndex%maxcount] = *tp;
        TourqueIndex++;
      }
    }*/

    while(TourqueSim[Slot]->TourqueIndex>2*maxcount)
      TourqueSim[Slot]->TourqueIndex-=maxcount;

    deltaTick -= 10;
  }

  maxcount = sMin<sInt>(maxcount,TourqueSimulator::MaxParticles);
  maxcount = sMin(maxcount,TourqueSim[Slot]->TourqueIndex);
  if(!maxcount)
    return;

  TourqueSim[Slot]->ParticleCount = maxcount;
  sF32 tFactor = TourqueSim[Slot]->Advance(kenv->TimeDelta,Speed,Damp);

  kenv->CurrentCam.ModelSpace = kenv->ExecStack.Top();
  geo = sSystem->GeoAdd(sFVF_TSPACE3,sGEO_TRI);
  mtrl->Passes[0].Mtrl->Set(kenv->CurrentCam);
  sSystem->GeoBegin(geo,(triMode?1:3)*maxcount,0,(sF32 **)&vp,0);

  sVector3 *pOld = TourqueSim[Slot]->ParticlePos[0];
  sVector3 *pNew = TourqueSim[Slot]->ParticlePos[1];

  sZONE(TourqueDraw);
  sInt ctr = 2;
  sU32 normCode;

  for(sInt i=0;i<maxcount;i++,pOld++,pNew++)
  {
    sVector pos,forw,upos;

    // speed
    forw.x = pNew->x - pOld->x;
    forw.y = pNew->y - pOld->y;
    forw.z = pNew->z - pOld->z;

    // position
    pos.x = pOld->x + tFactor * forw.x;
    pos.y = pOld->y + tFactor * forw.y;
    pos.z = pOld->z + tFactor * forw.z;

    if(triMode)
    {
      if(++ctr == 3)
      {
        sVector d1,d2,n;
        d1.x = pNew[1].x - pNew->x;
        d1.y = pNew[1].y - pNew->y;
        d1.z = pNew[1].z - pNew->z;
        d2.x = pNew[2].x - pNew->x;
        d2.y = pNew[2].y - pNew->y;
        d2.z = pNew[2].z - pNew->z;
        n.Cross3(d1,d2);
        n.Unit3();

        normCode = (sFtol(n.x*127.5f+127.5f)<<16)
          + (sFtol(n.y*127.5f+127.5f)<<8)
          + sFtol(n.z*127.5f+127.5f);
        ctr = 0;
      }

      vp->x = pos.x;
      vp->y = pos.y;
      vp->z = pos.z;
      vp->n = normCode;
      vp->s = 0x80ff80;
      vp->c = ~0;
      vp->u = 0.0f;
      vp->v = 0.0f;
      vp++;
    }
    else
    {
  // direction

      sF32 speed = forw.UnitAbs3();
      if(speed<minspeed)
      {
        upos.Init(pos.x,pos.y,pos.z);
        upos.Unit3();
        forw.AddScale3(upos,minspeed-speed);
        forw.Unit3();
      }

  // side

      /*upos.Cross3(forw,tp->Side);
      side.Cross3(upos,forw);
      side.Unit3();
      side.Scale3(0.125f);
      side.Add3(tp->Side);
      side.Unit3();
      tp->Side = side;*/
      side.Init(1,0,0,0);

  // draw

      forw.Scale3(SizeF);
      side.Scale3(SizeS);

      upos.Add3(pos,forw);
      ((sVertexTSpace3 *) vp)[0].Init(upos.x,upos.y,upos.z,~0,0,0);
      upos.Sub3(pos,forw);
      upos.Add3(side);
      ((sVertexTSpace3 *) vp)[1].Init(upos.x,upos.y,upos.z,~0,0,1);
      upos.Sub3(pos,forw);
      upos.Sub3(side);
      ((sVertexTSpace3 *) vp)[2].Init(upos.x,upos.y,upos.z,~0,1,0);

      vp+=3;
    }
  }
  sSystem->GeoEnd(geo);
  sSystem->GeoDraw(geo);
  sSystem->GeoRem(geo);
}

#endif

/****************************************************************************/
/****************************************************************************/

struct StreamPart
{
  sVector Rand[3];        // randomize vertices of triangle
  sVector Pos;            // x/z deviation on stream
  sF32 Fade;              // fading
};

static StreamPart StreamParts[0x10000];

static sInt StreamPartInit;

KObject * __stdcall Init_Effect_Stream(class GenMaterial *mtrl,StreamPara para)
{
  if(!StreamPartInit)
  {
    StreamPartInit = 1;

    for(sInt i=0;i<0x10000;i++)
    {
      StreamParts[i].Rand[0].InitRnd(); StreamParts[i].Rand[0].w = 0;
      StreamParts[i].Rand[1].InitRnd(); StreamParts[i].Rand[1].w = 0;
      StreamParts[i].Rand[2].InitRnd(); StreamParts[i].Rand[2].w = 0;
      StreamParts[i].Pos.InitRnd();     StreamParts[i].Pos.w = 1;
      StreamParts[i].Fade = sFGetRnd();
    }
  }
  return new GenEffect;
}

/****************************************************************************/

void __stdcall Exec_Effect_Stream(KOp *op,KEnvironment *kenv,StreamPara para)
{
  GenMaterial *mtrl;
  sInt geo;
  sVertexTSpace3 *vp;
  StreamPart *sp;
  sMatrix mat[4];

  mtrl = (GenMaterial *) op->GetLinkCache(0);
  if(!(mtrl && mtrl->Passes.Count>0)) return;


  sInt maxcount = para.Count;
  sInt maxvert = maxcount*3;

  kenv->CurrentCam.ModelSpace.Init();
  geo = sSystem->GeoAdd(sFVF_TSPACE3,sGEO_TRI);
  mtrl->Passes[0].Mtrl->Set(kenv->CurrentCam);
  sSystem->GeoBegin(geo,maxvert,0,(sF32 **)&vp,0);

  sSetRndSeed(para.Seed);
  sp = StreamParts;

  for(sInt i=0;i<4;i++)
  {
    mat[i].Init();
    mat[i].l = para.Pos[i];
    mat[i].l.w = 1;
    mat[i].i.x = para.Pos[i].w;
    mat[i].j.y = para.Pos[i].w;
    mat[i].k.z = para.Pos[i].w;
  }
  for(sInt i=0;i<maxcount;i++)
  {
    sVector pos,upos;
    sVector b[4];
    sF32 f[4];
    sF32 s;

    pos = sp->Pos;
    pos.w = 1;
    s = sFMod((sp->Fade+para.StuffX),1);
    f[0] =  (1-s)*(1-s)*(1-s);
    f[1] = 3*  s *(1-s)*(1-s);
    f[2] = 3*  s *   s *(1-s);
    f[3] =     s *   s *   s;

    for(sInt j=0;j<3;j++)
    {
      upos = pos;
      upos.AddScale3(sp->Rand[j],para.SizeF);
      b[0].Rotate34(mat[0],upos); 
      b[1].Rotate34(mat[1],upos);
      b[2].Rotate34(mat[2],upos);
      b[3].Rotate34(mat[3],upos);
      upos.   Scale3(b[0],f[0]);
      upos.AddScale3(b[1],f[1]);
      upos.AddScale3(b[2],f[2]);
      upos.AddScale3(b[3],f[3]);
      vp[j].Init(upos.x,upos.y,upos.z,~0,0,0);
    }
    vp+=3;
    sp++;
  }
  sSystem->GeoEnd(geo);
  sSystem->GeoDraw(geo);
  sSystem->GeoRem(geo);
}


/****************************************************************************/
/***                                                                      ***/
/***   Breakpoint 06 Invitation: Scene Spirit Effect                      ***/
/***                                                                      ***/
/****************************************************************************/

#if !sINTRO

KObject * __stdcall Init_Effect_BP06Spirit(class GenMaterial *mtrl,class GenMaterial *mtrl2,sF323 pos,sF32 radius,sF32 corerad,sF32 perlfreq,sF32 perlamp,sF32 perlanim,sU32 coli,sU32 colo,sU32 colc,sInt partcount,sF32 partrad,sF32 partspeed,sF32 partthick,sF32 partanim,sInt segments)
{
  sRelease(mtrl2);
  return MakeEffect(mtrl);
}


void __stdcall   Exec_Effect_BP06Spirit(KOp *op,KEnvironment *kenv,sF323 pos,sF32 radius,sF32 corerad,sF32 perlfreq,sF32 perlamp,sF32 perlanim,sU32 coli,sU32 colo,sU32 colc,sInt partcount,sF32 partrad,sF32 partspeed,sF32 partthick,sF32 partanim,sInt segments)
{
  GenMaterial *mtrl;
  GenMaterial *mtrl2;
  sInt geo;
  sF32 *fp;
  sU16 *ip;
  sMatrix mat;
  sVector p,d;
  static sVector Ring[256];

  // check material

  mtrl = (GenMaterial *) op->GetLinkCache(0);
  if(!(mtrl && mtrl->Passes.Count>0))
    return;
  mtrl2 = (GenMaterial *) op->GetLinkCache(1);
  if(!(mtrl2 && mtrl2->Passes.Count>0))
    mtrl2 = mtrl;
  kenv->CurrentCam.ModelSpace.Init();
  mtrl->Passes[0].Mtrl->Set(kenv->CurrentCam);
  geo = sSystem->GeoAdd(sFVF_TSPACE3,sGEO_TRI);

  // perpare shape

  sInt max = sRange(segments,255,3);
  sSystem->GetTransform(sGT_MODELVIEW,mat);
  for(sInt i=0;i<max;i++)
  {
    sF32 fs = sFSin(i*sPI2F/max);
    sF32 fc = sFCos(i*sPI2F/max);
    Ring[i].x = radius*(mat.i.x*fs + mat.i.y*fc)+pos.x;
    Ring[i].y = radius*(mat.j.x*fs + mat.j.y*fc)+pos.y;
    Ring[i].z = radius*(mat.k.x*fs + mat.k.y*fc)+pos.z;
    p.x = Ring[i].x * perlfreq + perlanim;
    p.y = Ring[i].y * perlfreq + perlanim;
    p.z = Ring[i].z * perlfreq + perlanim;
    sPerlin3D(p,p);
    Ring[i].x += (mat.i.x*p.x + mat.i.y*p.y)*perlamp;
    Ring[i].y += (mat.j.x*p.x + mat.j.y*p.y)*perlamp;
    Ring[i].z += (mat.k.x*p.x + mat.k.y*p.y)*perlamp;
  }

  // paint shape

  sSystem->GeoBegin(geo,max+1,max*3,&fp,(void **)&ip);
  for(sInt i=0;i<max;i++)
  {
    fp[0] = Ring[i].x;
    fp[1] = Ring[i].y;
    fp[2] = Ring[i].z;
    fp[3] = 0;
    fp[4] = 0;
    *((sU32 *)(&fp[5])) = coli;
    fp[6] = 0.5f;
    fp[7] = 0.5f;
    fp += 8;
  }
  fp[0] = pos.x;
  fp[1] = pos.y;
  fp[2] = pos.z;
  fp[3] = 0;
  fp[4] = 0;
  *((sU32 *)(&fp[5])) = coli;
  fp[6] = 0.5f;
  fp[7] = 0.5f;
  for(sInt i=0;i<max;i++)
  {
    *ip++ = i;
    *ip++ = (i+1)%max;
    *ip++ = max;
  }

  sSystem->GeoEnd(geo);
  sSystem->GeoDraw(geo);

  // paint particles

  sRandom rnd;
  sSystem->GeoBegin(geo,partcount*4,partcount*6,&fp,(void **)&ip);
  for(sInt i=0;i<partcount;i++)
  {
    sInt time = (rnd.Int(0x10000)+sInt(partanim*0x1000))&0xffff;

    sInt timernd = time >> 12;
    time = time&0xfff;

    sF32 side = rnd.Float(2)*partthick;
    sInt sidesign = rnd.Int(2)?-1:1;
    sInt pos0 = rnd.Int(max*1024);
    sF32 posf = (pos0&(1024-1))/1024.0f;
    pos0 = (pos0/1024+timernd*7)%max;
    sInt pos1 = (pos0+1)%max;


    p.x = Ring[pos0].x + (Ring[pos1].x-Ring[pos0].x)*posf;
    p.y = Ring[pos0].y + (Ring[pos1].y-Ring[pos0].y)*posf;
    p.z = Ring[pos0].z + (Ring[pos1].z-Ring[pos0].z)*posf;

    d.x = p.x-pos.x;
    d.y = p.y-pos.y;
    d.z = p.z-pos.z;
    d.Unit3();
    p.AddScale3(d,sidesign*(side+time*partspeed/4096));

    sF32 s = (1-time/4096.0f)*partrad;

    fp[0] = p.x + s*(- mat.i.x*s + mat.i.y*s);
    fp[1] = p.y + s*(- mat.j.x*s + mat.j.y*s);
    fp[2] = p.z + s*(- mat.k.x*s + mat.k.y*s);
    fp[3] = 0;  fp[4] = 0;   *((sU32 *)(&fp[5])) = colo;  fp[6] = 0;  fp[7] = 0;
    fp += 8;
    fp[0] = p.x + s*(+ mat.i.x*s + mat.i.y*s);
    fp[1] = p.y + s*(+ mat.j.x*s + mat.j.y*s);
    fp[2] = p.z + s*(+ mat.k.x*s + mat.k.y*s);
    fp[3] = 0;  fp[4] = 0;   *((sU32 *)(&fp[5])) = colo;  fp[6] = 1;  fp[7] = 0;
    fp += 8;
    fp[0] = p.x + s*(+ mat.i.x*s - mat.i.y*s);
    fp[1] = p.y + s*(+ mat.j.x*s - mat.j.y*s);
    fp[2] = p.z + s*(+ mat.k.x*s - mat.k.y*s);
    fp[3] = 0;  fp[4] = 0;   *((sU32 *)(&fp[5])) = colo;  fp[6] = 1;  fp[7] = 1;
    fp += 8;
    fp[0] = p.x + s*(- mat.i.x*s - mat.i.y*s);
    fp[1] = p.y + s*(- mat.j.x*s - mat.j.y*s);
    fp[2] = p.z + s*(- mat.k.x*s - mat.k.y*s);
    fp[3] = 0;  fp[4] = 0;   *((sU32 *)(&fp[5])) = colo;  fp[6] = 0;  fp[7] = 1;
    fp += 8;
  }

  for(sInt i=0;i<partcount;i++)
    sQuad(ip,0+i*4,1+i*4,2+i*4,3+i*4);

  sSystem->GeoEnd(geo);
  sSystem->GeoDraw(geo);

  // paint core

  mtrl2->Passes[0].Mtrl->Set(kenv->CurrentCam);
  sSystem->GeoBegin(geo,4,6,&fp,(void **)&ip);

  fp[0] = corerad*( - mat.i.x + mat.i.y)+pos.x;
  fp[1] = corerad*( - mat.j.x + mat.j.y)+pos.y;
  fp[2] = corerad*( - mat.k.x + mat.k.y)+pos.z;
  fp[3] = 0;  fp[4] = 0;  *((sU32 *)(&fp[5])) = colc;  fp[6] = 0.0f;  fp[7] = 0.0f;
  fp += 8;
  fp[0] = corerad*( + mat.i.x + mat.i.y)+pos.x;
  fp[1] = corerad*( + mat.j.x + mat.j.y)+pos.y;
  fp[2] = corerad*( + mat.k.x + mat.k.y)+pos.z;
  fp[3] = 0;  fp[4] = 0;  *((sU32 *)(&fp[5])) = colc;  fp[6] = 1.0f;  fp[7] = 0.0f;
  fp += 8;
  fp[0] = corerad*( + mat.i.x - mat.i.y)+pos.x;
  fp[1] = corerad*( + mat.j.x - mat.j.y)+pos.y;
  fp[2] = corerad*( + mat.k.x - mat.k.y)+pos.z;
  fp[3] = 0;  fp[4] = 0;  *((sU32 *)(&fp[5])) = colc;  fp[6] = 1.0f;  fp[7] = 1.0f;
  fp += 8;
  fp[0] = corerad*( - mat.i.x - mat.i.y)+pos.x;
  fp[1] = corerad*( - mat.j.x - mat.j.y)+pos.y;
  fp[2] = corerad*( - mat.k.x - mat.k.y)+pos.z;
  fp[3] = 0;  fp[4] = 0;  *((sU32 *)(&fp[5])) = colc;  fp[6] = 0.0f;  fp[7] = 1.0f;
  fp += 8;

  sQuad(ip,0,1,2,3);

  sSystem->GeoEnd(geo);
  sSystem->GeoDraw(geo);

  // done

  sSystem->GeoRem(geo);
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***  Breakpoint 06 Invitation: Dschungelgelöt                            ***/
/***                                                                      ***/
/****************************************************************************/

KObject * __stdcall Init_Effect_BP06Jungle(GenMaterial *mtrl,sInt segments,sInt slices,sF32 thickness,sF32 thickscale,sF32 length,sF32 lenscale,sF32 sangle,sF32 carfreq,sF32 caramp,sF32 modfreq,sF32 modamp,sF32 modphase)
{
  return MakeEffect(mtrl);
}

void __stdcall Exec_Effect_BP06Jungle(KOp *op,KEnvironment *kenv,sInt segments,sInt slices,sF32 thickness,sF32 thickscale,sF32 length,sF32 lenscale,sF32 sangle,sF32 carfreq,sF32 caramp,sF32 modfreq,sF32 modamp,sF32 modphase)
{
  GenMaterial *mtrl;
  volatile sVertexTSpace3 *vp;
  sU16 *ip;
  sInt geo;

  // check material

  mtrl = (GenMaterial *) op->GetLinkCache(0);
  if(!(mtrl && mtrl->Passes.Count>0))
    return;

  kenv->CurrentCam.ModelSpace = kenv->ExecStack.Top();
  mtrl->Passes[0].Mtrl->Set(kenv->CurrentCam);

  geo = sSystem->GeoAdd(sFVF_TSPACE3,sGEO_TRI|sGEO_DYNAMIC);

  // paint the effect

  sSystem->GeoBegin(geo,(segments+1)*(slices+1),segments*slices*6,(sF32 **) &vp,(void **) &ip);

  sF32 angle;
  sF32 posx,posy;

  angle = sangle;
  posx = posy = 0.0f;

  length /= segments;
  lenscale = sFPow(lenscale,1.0f / segments);
  thickscale = sFPow(thickscale,1.0f / segments);
  caramp /= segments;
  angle = sangle;

  for(sInt i=0;i<=segments;i++)
  {
    // calculate angles
    sF32 ca,sa,v;

    v = 1.0f * i / segments;
    angle += caramp * sFCos(sPI2F * v * (carfreq + modamp * sFSin(sPI2F * (v * modfreq + modphase))));
    sFSinCos(angle*sPI2F,sa,ca);

    // build the segments
    for(sInt j=0;j<=slices;j++)
    {
      sF32 u = 1.0f * j / slices;

      vp->x = posx + (u - 0.5f) * ca * thickness;
      vp->y = posy - (u - 0.5f) * sa * thickness;
      vp->z = 0.0f;
      vp->n = 0x808000;
      vp->s = 0x80ff80;
      vp->c = 0xffffffff;
      vp->u = u;
      vp->v = 1.0f - v;
      vp++;
    }

    // update angle and position
    posx += sa * length;
    posy += ca * length;
    length *= lenscale;
    thickness *= thickscale;
  }

  sInt slice1 = slices + 1;

  for(sInt i=0;i<segments;i++)
    for(sInt j=0;j<slices;j++)
      sQuad(ip,i*slice1+j,i*slice1+j+1,(i+1)*slice1+j+1,(i+1)*slice1+j);

  sSystem->GeoEnd(geo);
  sSystem->GeoDraw(geo);

  // done

  sSystem->GeoRem(geo);
}
