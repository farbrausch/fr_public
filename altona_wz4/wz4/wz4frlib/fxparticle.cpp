/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "wz4frlib/fxparticle.hpp"
#include "wz4frlib/fxparticle_ops.hpp"
#include "wz4frlib/fxparticle_shader.hpp"
#include "wz4frlib/wz4_bsp.hpp"
#include "base/graphics.hpp"
#include "util/algorithms.hpp"

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Particles Effect                                                   ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

static const sInt SinTableLen=512;
static sF32 SinTable[SinTableLen+SinTableLen/2+1];

static void InitSinTable()
{
  static sBool SinTableInited=sFALSE;
  if (!SinTableInited)
  {
    for (sInt i=0; i<SinTableLen+SinTableLen/2+1; i++)
    {
      sF32 x=sPI2F*sF32(i)/sF32(SinTableLen);
      SinTable[i]=sFSin(x);
    }
    SinTableInited=sTRUE;
  }
}

static sBool logic(sInt selflag,sF32 select)
{
  switch(selflag)
  {
  default:
    return 1;
  case 1:
    return 0;
  case 2:
    return select>=0.5f;
  case 3:
    return select<=0.5f;
  }
}

static inline sF32 FastRSqrt(sF32 x)
{
  // TODO: use rsqrtss opcode
  sF32 x2=x*0.5f;
  sInt i;
  i = 0x5f3759df - ( (*(sU32*)&x) >> 1 );
  x = *(sF32*)&i;
  x *= 1.5f-(x2*x*x);
  return x;
}

// "up" must be normalized!
static inline void FastLook(sMatrix34 &m, const sVector30 &dir, const sVector30 &up)
{
}

static inline void FastEulerXYZ(sMatrix34 &m, sF32 x,sF32 y,sF32 z) // normalized rotations!
{
  x*=SinTableLen;
  y*=SinTableLen;
  z*=SinTableLen;
  sInt xi=(sInt)x; x-=xi;
  sInt yi=(sInt)y; y-=yi;
  sInt zi=(sInt)z; z-=zi;

  sF32 sx=sFade(x,SinTable[xi&(SinTableLen-1)],SinTable[(xi+1)&(SinTableLen-1)]);
  sF32 sy=sFade(y,SinTable[yi&(SinTableLen-1)],SinTable[(yi+1)&(SinTableLen-1)]);
  sF32 sz=sFade(z,SinTable[zi&(SinTableLen-1)],SinTable[(zi+1)&(SinTableLen-1)]);
  sF32 cx=sFade(x,SinTable[(xi+SinTableLen/4)&(SinTableLen-1)],SinTable[(xi+SinTableLen/4+1)&(SinTableLen-1)]);
  sF32 cy=sFade(y,SinTable[(yi+SinTableLen/4)&(SinTableLen-1)],SinTable[(yi+SinTableLen/4+1)&(SinTableLen-1)]);
  sF32 cz=sFade(z,SinTable[(zi+SinTableLen/4)&(SinTableLen-1)],SinTable[(zi+SinTableLen/4+1)&(SinTableLen-1)]);

  m.i.x = cy*cz;
  m.i.y = cy*sz;
  m.i.z = -sy;
  m.j.x = sx*cz*sy - cx*sz;
  m.j.y = sx*sz*sy + cx*cz;
  m.j.z = sx*cy;
  m.k.x = cx*cz*sy + sx*sz;
  m.k.y = cx*sz*sy - sx*cz;
  m.k.z = cx*cy;
}

static inline sF32 FastSin(sF32 x) // normalized angle!
{
  x*=SinTableLen;
  sInt xi=(sInt)x; x-=xi;
  return sFade(x,SinTable[xi&(SinTableLen-1)],SinTable[(xi+1)&(SinTableLen-1)]);
}


/****************************************************************************/
/****************************************************************************/

static sArray<sMatrix34CM> FinalBoneMats;
static sInt FBMRefCount=0;
static void FBMAddRef() { ++FBMRefCount; }
static void FBMRelease() { if (!--FBMRefCount) FinalBoneMats.Reset(); }
static sMatrix34CM *FBMUse(sInt count) { FinalBoneMats.Clear(); return FinalBoneMats.AddMany(count); }

/****************************************************************************/
/***                                                                      ***/
/***   Cloud (boring gathering of wobbling particles)                     ***/
/***                                                                      ***/
/****************************************************************************/


RPCloud::RPCloud()
{
  InitSinTable();
  Anim.Init(Wz4RenderType->Script);
}

RPCloud::~RPCloud()
{
}

void RPCloud::Init()
{
  Para = ParaBase;

  Particles.AddMany(Para.Count);
  Particle *p;
  sRandomMT rnd;
  sFORALL(Particles,p)
  {
    p->Pos0.InitRandom(rnd);
    p->Pos1.InitRandom(rnd);
  }
}

/****************************************************************************/

void RPCloud::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);
}

sInt RPCloud::GetPartCount()
{
  return Particles.GetCount();
}
sInt RPCloud::GetPartFlags()
{
  return 0;
}

void RPCloud::Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt)
{
  sVector30 dx,dy;
  sMatrix34 mat;
  sVector31 p,p0,p1;
  Particle *part;
  sMatrix34 mat0,mat1;

  // calculate particle positions

  FastEulerXYZ(mat0,0,(time+dt)*Para.CloudFreq[0],0);
  mat0.Scale(Para.CloudSize[0]);
  FastEulerXYZ(mat1,0,(time+dt)*Para.CloudFreq[1],0);
  mat1.Scale(Para.CloudSize[1]);

  sFORALL(Particles,part)
  {
    p = part->Pos1*mat1;
    p = (sVector30(p)+part->Pos0)*mat0+Para.CloudPos;
    pinfo.Parts[_i].Init(p,time);
  }

  pinfo.Used = pinfo.Alloc;
}

/****************************************************************************/
/***                                                                      ***/
/***   Ballistic (Parabel curve)                                          ***/
/***                                                                      ***/
/****************************************************************************/


RPBallistic::RPBallistic()
{
  InitSinTable();
  Anim.Init(Wz4RenderType->Script);
  Source = 0;
}

RPBallistic::~RPBallistic()
{
  Source->Release();
}

void RPBallistic::Init()
{
  Para = ParaBase;

  Particle *p;
  sVector30 r;
  sRandomMT rnd;
  rnd.Seed(Para.Seed);

  if(Source)
    Particles.AddMany(Source->GetPartCount());
  else
    Particles.AddMany(Para.Count);

  sFORALL(Particles,p)
  {
    switch(Para.CreateFlags & 3)
    {
    case 0:
      r.InitRandom(rnd);
      break;
    case 1:
      r.x = rnd.Float(2)-1;
      r.y = rnd.Float(2)-1;
      r.z = rnd.Float(2)-1;
      break;
    case 2:
      r.InitRandom(rnd);
      r.Unit();
      break;
    }
    p->Pos = (sVector31)r;
    r.InitRandom(rnd);
    p->Speed = r;
    p->Time = rnd.Float(1);
  }
}

/****************************************************************************/

void RPBallistic::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);
}

sInt RPBallistic::GetPartCount()
{
  return Particles.GetCount();
}

sInt RPBallistic::GetPartFlags()
{
  return Source ? Source->GetPartFlags() : 0;
}

void RPBallistic::Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt)
{
  sVector30 dx,dy;
  sMatrix34 mat;
  sVector31 p,p0,p1;
  Particle *part;
  
  if(Source)
    Source->Func(pinfo,time,dt);

  // calculate particle positions

  if(Para.LifeTime>0.01f)
  {
    time = time/Para.LifeTime;
    dt = dt/Para.LifeTime;
  }
  time = time - Para.Delay;
  sInt used = 0;
  sFORALL(Particles,part)
  {
    if(Source)
      p = pinfo.Parts[_i].Pos;
    else
      p = part->Pos*Para.PosRand+Para.PosStart;
    sF32 t = time - part->Time*Para.BurstPercent;
    switch(Para.TimeFlags)
    {
    default:
    case 0:
      if(t<0 || t>1)
        t = -1;
      break;
    case 1:
      t = sAbsMod(t,1.0f);
      break;
    case 2:
      if(t<=0) t=0;
      if(t>1) t=-1;
      break;
    }
    if(t>=0)
      used++;
    sF32 tt = t+dt;
    p = p + (part->Speed*Para.SpeedRand+Para.SpeedStart)*tt + Para.Gravity*(tt*tt);
    pinfo.Parts[_i].Init(p,t);
  }
  pinfo.Used = used;
}

/****************************************************************************/
/***                                                                      ***/
/***   ???                                                                ***/
/***                                                                      ***/
/****************************************************************************/

RPMesh::RPMesh()
{
}

RPMesh::~RPMesh()
{
  sRelease(Mesh);
}

void RPMesh::Init(Wz4Mesh *mesh)
{
  sVERIFY(mesh != 0);
  Mesh = mesh;
  Mesh->AddRef();
}

void RPMesh::Simulate(Wz4RenderContext *ctx)
{
  SimulateCalc(ctx);
}

sInt RPMesh::GetPartCount()
{
  return Mesh->Chunks.GetCount();
}

sInt RPMesh::GetPartFlags()
{
  return wPNF_Orientation;
}

void RPMesh::Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt)
{
  sInt count = Mesh->Chunks.GetCount();
  sRandomMT mt;
  mt.Seed(0);

  for(sInt i=0;i<count;i++)
  {
    pinfo.Quats[i] = sQuaternion();
    pinfo.Parts[i].Init(Mesh->Chunks[i].COM,mt.Float(1.0f));
  }

  pinfo.Flags = wPNF_Orientation;
  pinfo.Used = count;
}

/****************************************************************************/

Wz4Explosion::Wz4Explosion()
{
  Type = Wz4ExplosionType;
}

Wz4Explosion::~Wz4Explosion()
{
}

/****************************************************************************/

RPExploder::RPExploder()
{
  InitSinTable();
  Mesh = 0;
}

RPExploder::~RPExploder()
{
  Mesh->Release();
}

void RPExploder::Init(Wz4Mesh *mesh,wCommand *cmd)
{
  Mesh = mesh;
  Mesh->AddRef();

  sInt count = Mesh->Chunks.GetCount();
  sRandomMT rand;

  rand.Seed(Para.Seed);
  Particles.Resize(count);  

  sF32 invDensity = 1.0f / Para.Density;
  sF32 invAngVel = -1.0f / Para.MaxAngularVelocity;

  for(sInt i=0;i<count;i++)
  {
    sVector31 pos = mesh->Chunks[i].COM;
    sMatrix34 inert;
    mesh->Chunks[i].GetInertiaTensor(inert);

    // invert inertia tensor (inlined since default altona routine has waaaay too large epsilon)
    sMatrix34 invInert;
    invInert.i.x = inert.j.y*inert.k.z - inert.j.z*inert.k.y;
    invInert.i.y = inert.i.z*inert.k.y - inert.i.y*inert.k.z;
    invInert.i.z = inert.i.y*inert.j.z - inert.i.z*inert.j.y;
    invInert.j.x = inert.j.z*inert.k.x - inert.j.x*inert.k.z;
    invInert.j.y = inert.i.x*inert.k.z - inert.i.z*inert.k.x;
    invInert.j.z = inert.i.z*inert.j.x - inert.i.x*inert.j.z;
    invInert.k.x = inert.j.x*inert.k.y - inert.j.y*inert.k.x;
    invInert.k.y = inert.i.y*inert.k.x - inert.i.x*inert.k.y;
    invInert.k.z = inert.i.x*inert.j.y - inert.i.y*inert.j.x;

    sF32 det = inert.i.x*invInert.i.x + inert.i.y*invInert.j.x + inert.i.z*invInert.k.x;
    if(det != 0.0f)
    {
      det=1.0f/det;
      invInert.i *= det;
      invInert.j *= det;
      invInert.k *= det;
    }
    else
      sClear(invInert);

    Particles[i].Pos = pos;
    Particles[i].Time = 1e+20f;
    Particles[i].Speed.Init(0.0f,0.0f,0.0f);
    Particles[i].RotAxis.Init(1.0f,0.0f,0.0f);
    Particles[i].AngularVelocity = 0.0f;

    for(sInt j=1;j<cmd->InputCount;j++)
    {
      const Wz4Explosion *explosion = cmd->GetInput<Wz4Explosion*>(j);
      if(!explosion)
        continue;

      const Wz4ExplosionParaExplosion &explo = explosion->Para;

      // orientation
      sMatrix34 mat;
      mat.Init();
      FastEulerXYZ(mat,explo.Rotation.x,explo.Rotation.y,explo.Rotation.z);

      // calc time
      sVector31 IgnitePos = (explo.TimingFlags & 1) ? explo.IgnitionPos : explo.Position;
      sVector30 d = (pos - IgnitePos) * mat;
      d.x /= explo.Shape.x;
      d.y /= explo.Shape.y;
      d.z /= explo.Shape.z;

      sF32 time = rand.Float(explo.IgnitionRand);
      time += sFPow(d.Length(),explo.DistancePower) / explo.IgnitionSpeed;
      if((explo.TimingFlags & 2) && (time < explo.TimeMin || time > explo.TimeMax))
        continue;

      time = sClamp(time,explo.TimeMin,explo.TimeMax) + explo.IgnitionTime;
      if(time < Particles[i].Time)
      {
        Particles[i].Time = time;

        // calculate direction/attack point
        sVector30 d = explo.Position - pos;

        sF32 len = d.Length();
        d.Unit();
        
        sVector30 force = -sFPow(len+0.0001f,explo.Falloff) * d * explo.SpeedExplode;
        Particles[i].Speed.InitRandom(rand);
        Particles[i].Speed = Particles[i].Speed*explo.SpeedRandom + explo.SpeedConstant + force;

        // ladies and gentlemen: rotational movement!
        // project attack point into mesh volume
        sVector30 attackPoint = pos - explo.Position;
        sInt first = mesh->Chunks[i].FirstVert;
        if(first != -1 && mesh->Vertices.GetCount()>0)
        {
          sInt idx = mesh->Vertices[first].Index[0];
          for(sInt v=first;v<mesh->Vertices.GetCount() && mesh->Vertices[v].Index[0]==idx;v++)
          {
            sF32 proj = (attackPoint + pos) ^ mesh->Vertices[v].Normal;
            if(proj > 0.0f)
              attackPoint -= proj * mesh->Vertices[v].Normal;
          }
        }

        // calc rotation
        sVector30 torque = attackPoint % force; // drehmoment (auch drehimpuls weil instantan)
        sVector30 omega = invDensity * (torque * invInert);

        sF32 angVel = omega.Length();
        angVel = Para.MaxAngularVelocity * (1.0f - sFExp(angVel * invAngVel)); // velocity limiter
        Particles[i].AngularVelocity = angVel;

        omega.Unit();
        Particles[i].RotAxis = omega;
      }
    }
  }
}

sInt RPExploder::GetPartCount()
{
  return Mesh->Chunks.GetCount();
}

sInt RPExploder::GetPartFlags()
{
  return wPNF_Orientation;
}

void RPExploder::Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt)
{
  Particle *part;

  sF32 invDrag = 1.0f / Para.AirDrag;
  sF32 invRotDrag = 1.0f / Para.RotationAirDrag;

  sFORALL(Particles,part)
  {
    sF32 t = time - part->Time;
    if(t<=0) t=0;

    sF32 tt = t+dt;
    sVector30 dr;
    sF32 dragTime = tt;
    if(Para.AirDrag)
      dragTime = invDrag * (1.0f - sFExp(-Para.AirDrag * tt));

    pinfo.Parts[_i].Init(part->Pos + dragTime*part->Speed + tt*tt*Para.Gravity,t);

    sF32 time = tt;
    if(Para.RotationAirDrag)
      time = invRotDrag * (1.0f - sFExp(-Para.RotationAirDrag * tt));
    pinfo.Quats[_i].Init(part->RotAxis,time*part->AngularVelocity);
  }
  
  pinfo.Flags = wPNF_Orientation;
  pinfo.Used = pinfo.Alloc;
}

/****************************************************************************/
/***                                                                      ***/
/***   Render as Sprites                                                  ***/
/***                                                                      ***/
/****************************************************************************/


RNSprites::RNSprites()
{
  InitSinTable();
  Format = 0;
  Geo = 0;
  Mtrl = 0;
  TextureDiff = 0;
  TextureFade = 0;
  Time = 0;

  Source = 0;

  Anim.Init(Wz4RenderType->Script);
}

RNSprites::~RNSprites()
{
  delete Geo;
  delete Mtrl;
  TextureDiff->Release();
  TextureFade->Release();
  Source->Release();
}

void RNSprites::Init()
{
  Para = ParaBase;

  static const sU32 desc[] = 
  {
    sVF_STREAM0|sVF_UV1|sVF_F3,
    sVF_STREAM1|sVF_INSTANCEDATA|sVF_POSITION|sVF_F4,
    sVF_STREAM1|sVF_INSTANCEDATA|sVF_UV0|sVF_F4,
    sVF_STREAM1|sVF_INSTANCEDATA|sVF_UV2|sVF_F4,
    sVF_STREAM1|sVF_INSTANCEDATA|sVF_UV3|sVF_F1,
    sVF_STREAM1|sVF_INSTANCEDATA|sVF_COLOR0|sVF_C4,
    0,
  };

  Format = sCreateVertexFormat(desc);
  Geo = new sGeometry;
  Geo->Init(sGF_TRILIST|sGF_INDEX16,Format);

  Mtrl = new StaticParticleShader;    
  Mtrl->Flags = sMTRL_CULLOFF;
  Mtrl->AlphaRef = 0;
  if (Para.Mode & 0x10000000)
  {
#if sRENDERER == sRENDER_DX11
    ((StaticParticleShader *)Mtrl)->Extra = 2;
#else
    Mtrl->FuncFlags[1] = sMFF_GREATER;
#endif    
  }

  Mtrl->Texture[0] = TextureDiff->Texture;
  Mtrl->TFlags[0] = sMTF_LEVEL2|sMTF_CLAMP;
  if(TextureFade)
  {
    Mtrl->Texture[1] = TextureFade->Texture;
    Mtrl->TFlags[1] = sMTF_LEVEL2|sMTF_CLAMP;
  }

  switch(Para.Mode & 0x000f)
  {
    default    : Mtrl->BlendColor = sMB_ADD; break;
    case 0x0001: Mtrl->BlendColor = sMB_PMALPHA; break;
    case 0x0002: Mtrl->BlendColor = sMB_MUL; break;
    case 0x0003: Mtrl->BlendColor = sMB_MUL2; break;
    case 0x0004: Mtrl->BlendColor = sMB_ADDSMOOTH; break;
    case 0x0005: Mtrl->BlendColor = sMB_ALPHA; break;
  }
  switch(Para.Mode & 0x0030)
  {
    default    : Mtrl->Flags |= sMTRL_ZOFF; break;
    case 0x0010: Mtrl->Flags |= sMTRL_ZREAD; break;
    case 0x0020: Mtrl->Flags |= sMTRL_ZWRITE; break;
    case 0x0030: Mtrl->Flags |= sMTRL_ZON; break;
  }
  switch(Para.Mode & 0x30000)
  {
    default     : Mtrl->BlendAlpha = sMBS_1|sMBO_ADD|sMBD_0; break;
    case 0x10000: Mtrl->BlendAlpha = sMBS_0|sMBO_ADD|sMBD_1; break;
    case 0x20000: Mtrl->BlendAlpha = sMBS_0|sMBO_ADD|sMBD_0; break;
    case 0x30000: Mtrl->BlendAlpha = sMBS_1|sMBO_ADD|sMBD_SAI; break;
  }

  Mtrl->Prepare(Format);

  PInfo.Init(Source->GetPartFlags(),Source->GetPartCount());

  Particles.AddMany(PInfo.Alloc);
  Particle *p;
  sRandom rnd;

  sF32 maxrowrandom=1.0f;
  sF32 rowoffs=0.0f;
  if (TextureFade) 
  {
    rowoffs=1.0f/TextureFade->Texture->SizeY;
    maxrowrandom-=2*rowoffs*Para.GroupCount;
  }

  sFORALL(Particles,p)
  {
    p->Group=rnd.Int(Para.GroupCount);
    p->Pos.Init(0,0,0);
    p->RotStart = rnd.Float(1)-0.5f;
    p->RotRand = rnd.Float(2)-1;
    p->FadeRow = (rnd.Float(maxrowrandom)+sF32(p->Group))/sF32(Para.GroupCount)+rowoffs;
    p->SizeRand = 1 + ((rnd.Float(2)-1)*Para.SizeRand);
    p->TexAnimRand = rnd.Float(1)*Para.TexAnimRand;
    p->Dist = 0;
  }
  PartOrder.HintSize(PInfo.Alloc);
}


void RNSprites::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  Source->Simulate(ctx);
  Time = ctx->GetTime();
  SimulateChilds(ctx);
}

void RNSprites::Prepare(Wz4RenderContext *ctx)
{
  sRandom rnd;
  sVector31 p;
  sF32 t;
  sF32 dist;
  PartVert0 *vp0;
  PartVert1 *vp1;
  Particle *part;
  sVector4 plane;
  sU16 *ip0;

  if(PInfo.Alloc==0) return;

  PartOrder.Clear();

  // texture might have changed, recalculate atlas uv

  if(TextureDiff->Atlas.Entries.IsEmpty())
  {
    UVRects.Resize(1);
    PartUVRect &r=UVRects[0];
    r.u=0;
    r.v=0;
    r.du=1;
    r.dv=1;
  }
  else
  {
    UVRects.Resize(TextureDiff->Atlas.Entries.GetCount());
    BitmapAtlasEntry *e;

    sFORALL(TextureDiff->Atlas.Entries,e)
    {
      PartUVRect &r=UVRects[_i];
      r.u = e->UVs.x0;
      r.v = e->UVs.y0;
      r.du = e->UVs.x1-e->UVs.x0;
      r.dv = e->UVs.y1-e->UVs.y0;
    }
  }

  // calculate particle positions

  sViewport view = ctx->View;
  view.UpdateModelMatrix(sMatrix34(Matrices[0]));
  sMatrix34 mat;
  mat = view.Camera;
  plane.InitPlane(mat.l,mat.k);

  dist = (mat.l-(Para.Trans*view.Model)).Length();

  PInfo.Reset();
  Source->Func(PInfo,Time,0);

  sInt usecolor = 0;
  if((Para.Mode & 0x2000) && (PInfo.Flags & wPNF_Color))
    usecolor = 1;

  if(Para.Mode & 0x800)
  {
    sInt max = PInfo.GetCount();
    for(sInt i=0;i<max;i++)
    {
      part = &Particles[i];
      PInfo.Parts[i].Get(p,t);
      if (t<0) continue;

      p+=sVector30(Para.Trans);
      dist=-(plane ^ (p*view.Model));

      part->Time = t;
      part->Pos = p;
      part->Dist = dist;
      part->DistFade = 1;
      if(usecolor)
        part->Color = PInfo.Colors[i];
      else
        part->Color = 0xffffffff;

      PartOrder.AddTail(part);
    }
  }
  else
  {
    if (dist>Para.FarFadeDistance) return;

    sF32 distfade=1.0f/Para.NearFadeDistance;
    sF32 globalfade=1.0f;
    if (dist>=(Para.FarFadeDistance-Para.FarFadeRange))
      globalfade=sClamp((Para.FarFadeDistance-dist)/Para.FarFadeRange,0.0f,1.0f);

    sInt max = PInfo.GetCount();
    for(sInt i=0;i<max;i++)
    {
      part = &Particles[i];
      PInfo.Parts[i].Get(p,t);
      if (t<0) continue;

      p+=sVector30(Para.Trans);
      dist=-(plane ^ (p*view.Model));

      if (dist<=-view.ClipNear)
      {
        part->Time = t;
        part->Pos = p;
        part->Dist = dist;

        sF32 df=sClamp(-(part->Dist+view.ClipNear)*distfade,0.0f,1.0f)*globalfade;

        part->DistFade = sSmooth(df*df);
        if(usecolor)
          part->Color = PInfo.Colors[i];
        else
          part->Color = 0xffffffff;

        PartOrder.AddTail(part);
      }
    }
  }

  if (PartOrder.IsEmpty())
    return;

  if(Para.Mode & 0x0100)
    sHeapSortUp(PartOrder,&Particle::Dist);


  // prepare instance data

  Geo->BeginLoadVB(4,sGD_FRAME,&vp0,0);
  vp0[0].Init(1,0,sPI2F*0.25f*0);
  vp0[1].Init(1,1,sPI2F*0.25f*1);
  vp0[2].Init(0,1,sPI2F*0.25f*2);
  vp0[3].Init(0,0,sPI2F*0.25f*3);
  Geo->EndLoadVB(-1,0);
  Geo->BeginLoadIB(6,sGD_FRAME,&ip0);    // yes we need an index buffer, or D3DDebug is unhappy
  sQuad(ip0,0,0,1,2,3);
  Geo->EndLoadIB();

  // prepare particle data

  sF32 sx = Para.Size*sSqrt(sPow(2,Para.Aspect));
  sF32 sy = Para.Size/sSqrt(sPow(2,Para.Aspect));

  const sInt uvcounti = UVRects.GetCount()/Para.GroupCount;
  const sF32 uvcountf = uvcounti;

  Geo->BeginLoadVB(PartOrder.GetCount(),sGD_FRAME,&vp1,1);

  sF32 ga,gb,gc,gd;
  switch(Para.GrowMode)
  {
  default:
  case 0:    ga = 1; gb = 0; gc = 0; gd = 0;    break;
  case 1:    ga = 0; gb = 1; gc = 0; gd = 0;    break;
  case 2:    ga = 0; gb = 2; gc =-1; gd = 0;    break;
  case 3:    ga = 0; gb = 0; gc = 3; gd =-2;    break;
  }
  sF32 fi = Para.FadeIn;
  sF32 fo = Para.FadeOut;  
  if(fi+fo>1.0f)
    fo = 1-fi;
  sF32 fii = 0;
  sF32 foi = 0;
  if(fi>0.0001f)
    fii = 1/fi;
  else 
    fi = 0; 
  if(fo>0.0001f)
    foi = 1/fo;
  else 
    fo = 0;
  if(Para.GrowMode==0)
    fi = fo = 0;

  sFORALL(PartOrder,part)
  {
    vp1->px = part->Pos.x;
    vp1->py = part->Pos.y;
    vp1->pz = part->Pos.z;
    vp1->rot = sMod(Para.RotStart+part->RotStart*Para.RotSpread+Time*(Para.RotSpeed+Para.RotRand*part->RotRand),1)*sPI2F;
  
    t = sMod(part->Time,1);
    sF32 tt = 1;
    sF32 s = part->SizeRand;
    sF32 fade = 1;
    if(t<fi)
    {
      tt = t*fii;
      fade = (ga+tt*gb+tt*tt*gc+tt*tt*tt*gd);
    }
    else if(1-t<fo)
    {
      tt = (1-t)*foi;
      fade  = (ga+tt*gb+tt*tt*gc+tt*tt*tt*gd);
    }
    if (Para.FadeType == 0)
      s*=fade;
    vp1->sx = sx*s;
    vp1->sy = sy*s;
    vp1->u1 = t;
    vp1->v1 = part->FadeRow;

    sInt texanim = 0;
    if(Para.Mode & 0x200)
    {
      texanim = sInt(part->TexAnimRand*32) % UVRects.GetCount();
    }
    else
    {
      texanim=sInt(uvcountf*sMod(t*Para.TexAnimSpeed+part->TexAnimRand,1));
      if (texanim<0) texanim+=uvcounti;
    }
    vp1->uvrect = UVRects[texanim+part->Group*uvcounti];

    vp1->fade = part->DistFade;
    if (Para.FadeType == 1)
      vp1->Color = sColorFade(0,part->Color,fade);
    else
      vp1->Color = part->Color;

    vp1++;
  }
  Geo->EndLoadVB(-1,1);
}

void RNSprites::Render(Wz4RenderContext *ctx)
{
  if (PartOrder.IsEmpty())
    return;

  sBool zwrite;
  switch(Para.Mode & 0x0030)
  {
    case 0x0020: case 0x0030: zwrite=sTRUE; break;
    default: zwrite=sFALSE; break;
  }

  if(ctx->IsCommonRendermode(zwrite))
  {
    sMatrix34CM *cm;
    sMatrix34 mat;
    sCBuffer<StaticParticlePSPara> cb1;
    cb1.Data->col.InitColor(Para.Color);

    sU32 fade0col;
    switch(Para.Mode & 0x000f)
    {
      default    : fade0col=0x00000000; break;
      case 0x0001: fade0col=0x00000000; break;
      case 0x0002: fade0col=0x00ffffff; break;
      case 0x0003: fade0col=0x00808080; break;
      case 0x0004: fade0col=0x00000000; break;
      case 0x0005: fade0col=Para.Color&0x00ffffff; break;
    }
    cb1.Data->fade0col.InitColor(fade0col);

    sCBuffer<StaticParticleVSPara> cb0;
    sFORALL(Matrices,cm)
    {
      sViewport view = ctx->View;
      view.UpdateModelMatrix(sMatrix34(*cm));
      mat = view.ModelView;
      mat.Invert3();
      sF32 tscale = 1;
      if(Para.Mode & 0x4000)
      {
        sF32 d = cm->x.x*cm->x.x + cm->y.x*cm->y.x + cm->z.x*cm->z.x;
        tscale = sSqrt(d);
      }

      cb0.Data->mvp = view.ModelScreen;
      if(Para.Mode & 0x1000)
      {
        sMatrix34 mata;
        mata.Look(Para.AlignDir);
        mat=mata;
      }

      if (Para.Mode&0x1000000)
      {
        mat.j.x=0.0f;
        mat.j.y=1.0f;
        mat.j.z=0.0f;
        mat.i.Cross(mat.j,mat.k);
        mat.i.Unit();
      }

      cb0.Data->di = mat.i*tscale;
      cb0.Data->dj = mat.j*tscale;

      if (Para.Mode&0x100000)
        cb0.Data->trans.Init(0.0f,1.0f,0.0f,0.0f);
      else
        cb0.Data->trans.Init(0.0f,0.0f,0.0f,0.0f);
      cb0.Modify();

      Mtrl->Set(&cb0,&cb1);
      Geo->Draw(0,0,PartOrder.GetCount(),0);
    }
  } 
}

/****************************************************************************/
/***                                                                      ***/
/***   Render as small objects                                            ***/
/***                                                                      ***/
/****************************************************************************/

RNChunks::RNChunks()
{
  InitSinTable();
  Source = 0;
  Samples = 0;
  Time = 0;

  Anim.Init(Wz4RenderType->Script);

  FBMAddRef();
}

RNChunks::~RNChunks()
{
  sRelease(Source);
  sReleaseAll(Meshes);

  FBMRelease();
}

sBool RNChunks::Init()
{
  Para = ParaBase;
  Mode = Para.Direction;

  sInt _count = Source->GetPartCount();
  sInt _flags = Source->GetPartFlags();

  switch(Mode & 15)
  {
  case 0:
    Samples = 1;
    break;
  case 1:
    Samples = 2;
    break;
  case 2:
    Samples = 3;
    break;
  }

  BoneCount = 0;
  if(Meshes.GetCount()>0 && Meshes[0]->Chunks.GetCount()>0)
    BoneCount = Meshes[0]->Chunks.GetCount();

  if(Para.Direction & 0x20)
    _flags |= wPNF_Color;

  for(sInt i=0;i<Samples;i++)
    PInfo[i].Init(_flags,_count);

  Parts.Resize(_count);
  Part *p;

  if (Para.Direction & 0x40)
  {
    sFORALL(Parts,p)
    {
      p->Index = _i % Meshes.GetCount();
      p->Rand.Init(1,1,1);
    }
  }
  else
  {
    sRandomMT rnd;
    rnd.Seed(Para.Seed);
    sFORALL(Parts,p)
    {
      p->Index = rnd.Int(Meshes.GetCount());
      p->Rand.x = rnd.Float(1)-0.5f;
      p->Rand.y = rnd.Float(1)-0.5f;
      p->Rand.z = rnd.Float(1)-0.5f;
    }
  }


  if(BoneCount>0 && Meshes.GetCount()!=1)
    return 0;
  return 1;
}

/****************************************************************************/


static inline void FastThreePoint(sMatrix34 &m,const sVector31 &p0,const sVector31 &p1,const sVector31 &p2,const sVector30 &tweak)
{
  sVector30 db;

  m.k = p2-p0;
  m.k*=FastRSqrt(m.k.LengthSq());
  db = p1-p0;

  db*=FastRSqrt(db.LengthSq());

  m.i.Cross(db,m.k); 
  m.i = m.i + tweak;
  m.j.Cross(m.k,m.i); 

  m.i*=FastRSqrt(m.i.LengthSq());
  m.j*=FastRSqrt(m.j.LengthSq());
}

void RNChunks::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  Time = ctx->GetTime();
  Source->Simulate(ctx);
  SimulateChilds(ctx);
}

void RNChunks::Prepare(Wz4RenderContext *ctx)
{
  Part *p;
  sVector31 v0,v1,v2;
  sVector30 d,up,r;
  sMatrix34 mat0,mat;
  sF32 t;

  Wz4Mesh *mesh;
  sFORALL(Meshes,mesh)
    if(mesh) mesh->BeforeFrame(Para.LightEnv);

  PInfo[0].Reset();
  Source->Func(PInfo[0],Time,0);
  for(sInt i=1;i<Samples;i++)
  {
    PInfo[i].Reset();
    Source->Func(PInfo[i],Time,Para.LookAhead*i);
  }

  switch(Para.UpVector)
  {
  case 0:
    up.Init(1,0,0);
    break;
  case 1:
    up.Init(0,1,0);
    break;
  case 2:
    up.Init(0,0,1);
    break;
  }

  sInt mode = Mode & 15;
  sInt animmode = Para.Direction & 16;
  sVERIFY(mode==0 || mode==1 || mode==2);

  sBool dorot=!(Para.RotStart.LengthSq()==0.0f && Para.RotSpeed.LengthSq()==0.0f && Para.RotRand.LengthSq()==0.0f);
  sBool dospiral=!(Para.SpiralRand==0.0f && Para.SpiralSpeed==0.0f && Para.SpiralRandSpeed==0.0f);

  sFORALL(Parts,p)
  {
    PInfo[0].Parts[_i].Get(v0,t);

    if(mode==0)
    {
      if (dorot)
      {
        r = (p->Rand*Para.RotStart+t*(Para.RotSpeed+p->Rand*Para.RotRand));
        FastEulerXYZ(mat,r.x,r.y,r.z);
      }
      else
        mat.Init();
    }
    else
    {
      if(mode==1)
      {
        PInfo[1].Parts[_i].Get(v1);
        sVector30 d = v1-v0;

        mat.k = d*FastRSqrt(d.LengthSq());
        mat.j = up;
        mat.i.Cross(mat.j,mat.k);
        mat.i *= FastRSqrt(mat.i.LengthSq());
        mat.l.Init(0,0,0);
        mat.j.Cross(mat.k,mat.i);
        mat.j *= FastRSqrt(mat.j.LengthSq());

        if (dospiral)
        {
          FastEulerXYZ(mat0,0,0,(p->Rand.x*Para.SpiralRand+t*(Para.SpiralSpeed+p->Rand.y*Para.SpiralRandSpeed)));
          mat *= mat0;
        }
      }
      else
      {
        PInfo[1].Parts[_i].Get(v1);
        PInfo[2].Parts[_i].Get(v2);
        sVector30 da(v2-v0);
        sVector30 db(v1-v0);
        mat.i.Cross(db,da);    
        mat.j.Cross(da,mat.i);
        mat.k = da;
        mat.i*=FastRSqrt(mat.i.LengthSq());
        mat.j*=FastRSqrt(mat.j.LengthSq());
        mat.k*=FastRSqrt(mat.k.LengthSq());
      }

      if (dorot)
      {
        r.x = FastSin((t*Para.RotSpeed.x+p->Rand.x));
        r.y = FastSin((t*Para.RotSpeed.y+p->Rand.y));
        r.z = FastSin((t*Para.RotSpeed.z+p->Rand.z));
        r = (r*p->Rand*Para.RotRand+p->Rand*Para.RotStart);
        FastEulerXYZ(mat0,r.x,r.y,r.z);
        mat = mat * mat0;
      }
    }

    sF32 scale = (1 + Para.ScaleRand*p->Rand.z*2)*Para.Scale;
    p->Mat.i=mat.i * scale;
    p->Mat.j=mat.j * scale;
    p->Mat.k=mat.k * scale;
    p->Mat.l = v0 + sVector30(Para.Trans);
    p->Time = t;

    if(animmode)
    {
      sF32 a = p->Rand.z*Para.AnimRand;
      a += t*(Para.AnimSpeed + p->Rand.y*Para.AnimSpeedRand);
      p->Anim = sMod(a,1.0f);
    }
    else
    {
      p->Anim = 0;
    }
  }
}

void RNChunks::Render(Wz4RenderContext *ctx)
{
  Part *p;
//  Wz4ShaderEnv env;


  if(Para.Direction & 16)
  {
    sMatrix34 mat;
    sMatrix34CM *matp;
    sMatrix34CM *imat = new sMatrix34CM[Parts.GetCount()];

    sInt animdifferent = sMax(1,Parts.GetCount()/Para.AnimDifferent);

    for(sInt i=0;i<Meshes.GetCount();i++)
    {
      if(Meshes[i]->Skeleton)
      {
        sInt bc = Meshes[i]->Skeleton->Joints.GetCount();
        sMatrix34 *bonemat = new sMatrix34[bc];
        sMatrix34CM *basemat = new sMatrix34CM[bc];
        
        sFORALL(Matrices,matp)
        {
          sInt n = 0;
          sFORALL(Parts,p)
          {
            if(_i%animdifferent==0)
            {
              if(n>0)
                Meshes[i]->RenderBoneInst(ctx->RenderMode,Para.LightEnv,bc,basemat,n,imat);
              n = 0;
              Meshes[i]->Skeleton->EvaluateCM(p->Anim,bonemat,basemat);
            }
            if(p->Time>=0 && p->Index==i)
            {
              imat[n++] = p->Mat*sMatrix34(*matp);
            }
          }
          if(n>0)
            Meshes[i]->RenderBoneInst(ctx->RenderMode,Para.LightEnv,bc,basemat,n,imat);
        }
        delete[] bonemat;
        delete[] basemat;
      }
      else
      {
        sFORALL(Parts,p)
        {
          if(p->Time>=0 && p->Index==i)
          {
            sFORALL(Matrices,matp)
            {
              mat = p->Mat*sMatrix34(*matp);
              Meshes[i]->Render(ctx->RenderMode,Para.LightEnv,&sMatrix34CM(mat),p->Anim,ctx->Frustum);
            }
          }
        }
      }
    }

    delete[] imat;
  }
  else if(BoneCount)
  {
    sInt Count = PInfo[0].Alloc;
    sVERIFY(Count == Parts.GetCount());
    sInt max = (Count*Matrices.GetCount()+BoneCount-1)/BoneCount*BoneCount;

    sMatrix34CM *mats = FBMUse(max);
    sMatrix34CM *matp;
    sInt n = 0;
    sInt nc = 0;
    sFORALL(Matrices,matp)
    {
      sFORALL(Parts,p)
      {
        sMatrix34 mat;
        mat = p->Mat;
        mat.l += (-sVector30(Meshes[0]->Chunks[nc++].COM))*mat;
        mats[n++] = mat * sMatrix34(*matp);
        if(nc==BoneCount) nc=0;
      }
    }
    for(sInt i=Count;i<max;i++)
      mats[n++].Init();

    sInt left = Count*Matrices.GetCount();
    sInt done = 0;
    while(left>0)
    {
      sInt batch = sMin(BoneCount,left);
      Meshes[0]->RenderBone(ctx->RenderMode,Para.LightEnv,BoneCount,mats+done,batch);
      done += batch;
      left -= batch;
    }
  }
  else
  {
    sMatrix34CM *mats = FBMUse(PInfo[0].Used*Matrices.GetCount());
    sMatrix34CM *matp;

    if(mats>0)
    {
      for(sInt i=0;i<Meshes.GetCount();i++)
      {
        sInt n = 0;
        sFORALL(Parts,p)
          if(p->Time>=0 && p->Index==i)
            sFORALL(Matrices,matp)
              mats[n++] = p->Mat*sMatrix34(*matp);
        sVERIFY(n <= PInfo[0].Used*Matrices.GetCount());

        Meshes[i]->RenderInst(ctx->RenderMode,Para.LightEnv,n,mats,(Para.Direction & 0x20) ? PInfo[0].Colors : 0);
      }
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   This op is now very similar to Chunks. please rework!              ***/
/***                                                                      ***/
/****************************************************************************/

RNDebris::RNDebris()
{
  InitSinTable();

  Anim.Init(Wz4RenderType->Script);
  Mesh = 0;
  Source = 0;

  FBMAddRef();
}

RNDebris::~RNDebris()
{
  sRelease(Mesh);
  sRelease(Source);

  FBMRelease();
}

void RNDebris::Init()
{
  Para = ParaBase;

  PInfo.Init(Source->GetPartFlags(),Source->GetPartCount());
  Parts.Resize(PInfo.Alloc);

  for(sInt i=0;i<Parts.GetCount();i++)
    Parts[i].Init();
}

void RNDebris::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  Time = ctx->GetTime();
  Source->Simulate(ctx);
  SimulateChilds(ctx);
}

void RNDebris::Prepare(Wz4RenderContext *ctx)
{
  if(PInfo.Alloc)
  {
    if(Mesh) Mesh->BeforeFrame(Para.LightEnv);

    PInfo.Reset();
    Source->Func(PInfo,Time,0);
    sMatrix34 *p;
    sF32 t;
    sInt nChunks = Mesh ? Mesh->Chunks.GetCount() : 0;

    sFORALL(Parts,p)
    {
      if(_i >= nChunks)
        break;

      const sVector31 &center = Mesh->Chunks[_i].COM;
      sVector30 offset(center);

      if(PInfo.Quats)
      {
        p->Init(PInfo.Quats[_i]);
        offset = offset * *p;
      }

      PInfo.Parts[_i].Get(p->l,t);
      p->l -= offset;
    }
  }
}

void RNDebris::Render(Wz4RenderContext *ctx)
{
  if(Mesh)
  {
    sMatrix34CM *mats = FBMUse(PInfo.Used*Matrices.GetCount());
    sMatrix34CM *model;

    sFORALL(Matrices,model)
    {
      sInt count = sMin(PInfo.Used,sMin(Parts.GetCount(),Mesh->Chunks.GetCount()));
      for(sInt i=0;i<count;i++)
        mats[i] = Parts[i]*sMatrix34(*model);

      Mesh->RenderBone(ctx->RenderMode,Para.LightEnv,count,mats);
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Render as Metaballs                                                ***/
/***                                                                      ***/
/****************************************************************************/

struct RNMetaballsPartFormat
{
  sF32 px,py,pz;
  sF32 nx,ny,nz;
  sF32 u0,v0;
  void Init(const sVector31 &pos,const sVector31 &part,sF32 u,sF32 v) 
  { px=pos.x; py=pos.y; pz=pos.z; nx=part.x; ny=part.y; nz=part.z; u0=u; v0=v; }
};

RNMetaballs::RNMetaballs()
{
  InitSinTable();

  static const sU32 desc[] = 
  {
    sVF_POSITION|sVF_F3,
    sVF_NORMAL|sVF_F3,
    sVF_UV0|sVF_F2,
    0,
  };

  PartFormat = sCreateVertexFormat(desc);
  PartGeo = new sGeometry(sGF_QUADLIST,PartFormat);
  sClear(PartMtrl);

  BlitFormat = sVertexFormatStandard;
  BlitGeo = new sGeometry(sGF_QUADLIST,BlitFormat);
  BlitMtrl = 0;

  DebugMtrl = 0;

  Source = 0;
  sClear(PeelTex);

  Anim.Init(Wz4RenderType->Script);
}

RNMetaballs::~RNMetaballs()
{
  delete PartGeo;
  Source->Release();
  for(sInt i=0;i<MaxPeel;i++)
  {
    delete PeelTex[i];
    delete PartMtrl[i];
  }
  delete BlitGeo;
  delete BlitMtrl;
  delete DebugMtrl;
}

void RNMetaballs::Init()
{
  Para = ParaBase;
  sVERIFY(Para.Peels<=MaxPeel);

  switch(Para.Flags & 3)
  {
  default:
  case 0:
    TexSizeX = 256;
    TexSizeY = 128;
    break;
  case 1:
    TexSizeX = 512;
    TexSizeY = 256;
    break;
  case 2:
    TexSizeX = 1024;
    TexSizeY = 512;
    break;
  }

  for(sInt i=0;i<Para.Peels;i++)
    PeelTex[i] = new sTexture2D(TexSizeX,TexSizeY,sTEX_ARGB32F|sTEX_2D|sTEX_RENDERTARGET|sTEX_NOMIPMAPS,1);

  sEnlargeZBufferRT(TexSizeX,TexSizeY);

  for(sInt i=0;i<Para.Peels;i++)
  {
    PartMtrl[i] = new MetaballsPartMtrl;
    PartMtrl[i]->Flags = sMTRL_ZON|sMTRL_CULLOFF;
    if(i>0)
    {
      PartMtrl[i]->Texture[0] = PeelTex[i-1];
      PartMtrl[i]->TFlags[0] = sMTF_CLAMP|sMTF_LEVEL0;
    }
    PartMtrl[i]->AlphaRef = 254;
    PartMtrl[i]->FuncFlags[sMFT_ALPHA] = sMFF_LESS;   // allow low alpha (low z)
    PartMtrl[i]->Prepare(PartFormat);
  }

  BlitMtrl = new MetaballsTraceMtrl;
  BlitMtrl->Flags = sMTRL_ZON|sMTRL_CULLOFF;
  for(sInt i=0;i<MaxPeel;i++)
  {
    BlitMtrl->Texture[i] = PeelTex[i];
    BlitMtrl->TFlags[i] = sMTF_CLAMP|sMTF_LEVEL0;
  }
  BlitMtrl->Prepare(BlitFormat);

  DebugMtrl = new sSimpleMaterial;
  DebugMtrl->Flags = sMTRL_ZON|sMTRL_CULLOFF;
  DebugMtrl->Texture[0] = PeelTex[Para.Peels ? Para.Peels-1 : 0];
  DebugMtrl->TFlags[0] = sMTF_CLAMP|sMTF_LEVEL0;
  DebugMtrl->Prepare(BlitFormat);

  PInfo.Init(Source->GetPartFlags(),Source->GetPartCount());
  
  sVertexStandard *vp;
  sF32 u =0.5f/TexSizeX;
  sF32 v =0.5f/TexSizeY;
  BlitGeo->BeginLoadVB(4,sGD_STATIC,&vp);
  vp[0].Init(0,0,0,  1,-1,0, 0+u,0+v);
  vp[1].Init(0,1,0,  1, 1,0, 0+u,1+v);
  vp[2].Init(1,1,0, -1, 1,0, 1+u,1+v);
  vp[3].Init(1,0,0, -1,-1,0, 1+u,0+v);
  BlitGeo->EndLoadVB();
}


void RNMetaballs::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);

  Source->Simulate(ctx);
  Time = ctx->GetTime();
  SimulateChilds(ctx);
}

void RNMetaballs::Prepare(Wz4RenderContext *ctx)
{
  RNMetaballsPartFormat *vp;

  PInfo.Reset();
  Source->Func(PInfo,Time,0);

  if(PInfo.Used)
  {
    sVector30 off[4];
    sVector31 v;
    sMatrix34 mat = ctx->View.ModelView;
    mat.Trans3();
    sF32 s = Para.Size;
    off[0] = ( mat.i+mat.j)*s;
    off[1] = ( mat.i-mat.j)*s;
    off[2] = (-mat.i-mat.j)*s;
    off[3] = (-mat.i+mat.j)*s;

    sInt vc=0;
    PartGeo->BeginLoadVB(PInfo.Used,sGD_FRAME,&vp);
    for(sInt i=0;i<PInfo.Alloc;i++)
    {
      if(PInfo.Parts[i].Time>=0)
      {
        v = PInfo.Parts[i].Pos;
        vp[0].Init(v+off[0],v,-1,-1);
        vp[1].Init(v+off[1],v,-1, 1);
        vp[2].Init(v+off[2],v, 1, 1);
        vp[3].Init(v+off[3],v, 1,-1);
        vp+=4;
        vc++;
      }   
    }

    PartGeo->EndLoadVB();
    sVERIFY(vc==PInfo.Used);
  }

  // peel


  sRay ray[4];
  ctx->View.MakeRay(-1, 1,ray[0]);
  ctx->View.MakeRay(-1,-1,ray[1]);
  ctx->View.MakeRay( 1,-1,ray[2]);
  ctx->View.MakeRay( 1, 1,ray[3]);
  
  sCBuffer<MetaballsPartVSPara> cb;
  cb.Data->mvp = ctx->View.ModelScreen;
  cb.Data->uvoffset.Init(0.5f/TexSizeX,0.5f/TexSizeY,0,0);
  cb.Data->ray00 = ray[0].Start;
  cb.Data->ray10 = ray[1].Start;
  cb.Data->ray01 = ray[3].Start;
  cb.Data->ray11 = ray[2].Start;
  cb.Data->CamPos.x = ctx->View.Camera.l.x;
  cb.Data->CamPos.y = ctx->View.Camera.l.y;
  cb.Data->CamPos.z = ctx->View.Camera.l.z;
  cb.Data->CamPos.w = 0.2f;
  cb.Modify();

  for(sInt i=0;i<Para.Peels;i++)
  {
    sSetTarget(sTargetPara(sCLEAR_ALL,0xff000000,0,PeelTex[i],sGetRTDepthBuffer()));
    
    PartMtrl[i]->Set(&cb);
    PartGeo->Draw();
  }
}

void RNMetaballs::Render(Wz4RenderContext *ctx)
{
  if(ctx->IsCommonRendermode() && PInfo.Used>0)
  {
    if(Para.Debug&1)
    {
      if(Para.Peels>0)
      {
        sViewport view;
        view.Orthogonal = sVO_SCREEN;
        view.Prepare();

        sCBuffer<sSimpleMaterialPara> cb;
        cb.Data->mvp = view.ModelScreen;
        cb.Modify();

        DebugMtrl->Set(&cb);
        BlitGeo->Draw();
      }
      else
      {
        sCBuffer<MetaballsPartVSPara> cb;
        cb.Data->mvp = ctx->View.ModelScreen;
        cb.Modify();

        PartMtrl[0]->Set(&cb);
        PartGeo->Draw();
      }
    }
    else
    {
      sViewport view;
      view.Orthogonal = sVO_SCREEN;
      view.Prepare();

      sRay ray[4];
      ctx->View.MakeRay(-1, 1,ray[0]);
      ctx->View.MakeRay(-1,-1,ray[1]);
      ctx->View.MakeRay( 1,-1,ray[2]);
      ctx->View.MakeRay( 1, 1,ray[3]);
      

      sCBuffer<MetaballsTraceVSPara> cb;
      cb.Data->mvp = view.ModelScreen;
      cb.Data->ray00 = ray[0].Start;
      cb.Data->ray10 = ray[1].Start;
      cb.Data->ray01 = ray[3].Start;
      cb.Data->ray11 = ray[2].Start;
      cb.Data->CamPos.x = ctx->View.Camera.l.x;
      cb.Data->CamPos.y = ctx->View.Camera.l.y;
      cb.Data->CamPos.z = ctx->View.Camera.l.z;
      cb.Data->CamPos.w = 0.2f;
      cb.Modify();

      BlitMtrl->Set(&cb);
      BlitGeo->Draw();
    }
  } 
}

/****************************************************************************/
/***                                                                      ***/
/***   Another booring particle cloud                                     ***/
/***                                                                      ***/
/****************************************************************************/

RPCloud2::RPCloud2()
{
  InitSinTable();
}

RPCloud2::~RPCloud2()
{
}

void RPCloud2::Init(Wz4ParticlesArrayCloud2 *Array,sInt ArrayCount)
{
  sInt max = 0;
  Part *p;
  sRandomMT rnd;
  sVector30 v;
  sMatrix34 mat;
  sInt ac = ArrayCount;

  for(sInt i=0;i<ac;i++)
    max += Array[i].Count;

  Para = ParaBase;

  Parts.HintSize(max);
  Clusters.Resize(ac);
  for(sInt i=0;i<ac;i++)
  {
    mat.Init();
    FastEulerXYZ(mat,0,Array[i].Rot,0);
    mat.Scale(Array[i].Scale.x,Array[i].Scale.y,Array[i].Scale.z);
    mat.l += Array[i].Pos;
    mat.i *= 0.5f;
    mat.j *= 0.5f;
    mat.k *= 0.5f;
    Clusters[i].Matrix = mat;
    Clusters[i].Speed = Array[i].Speed/sMax<sF32>(sAbs(Array[i].Scale.z),0.125f);
  }

  for(sInt i=0;i<ac;i++)
  {
    p = Parts.AddMany(Array[i].Count);
    for(sInt j=0;j<Array[i].Count;j++)
    {
      p->ClusterId = i;
      p++;
    }
  }

  sFORALL(Parts,p)
  {
    sF32 x,y;
    do
    {
      x = rnd.Float(2)-1;
      y = rnd.Float(2)-1;
    }
    while(x*x+y*y>1);
    p->StartX = x;
    p->StartY = y;
    p->Phase = rnd.Float(1);
    p->Speed = Para.Speed + Clusters[p->ClusterId].Speed;
    p->Speed *= 1+((rnd.Float(2)-1.0f)*Para.SpeedSpread);
  }
}


sInt RPCloud2::GetPartCount()
{
  return Parts.GetCount();
}
sInt RPCloud2::GetPartFlags()
{
  return 0;
}

void RPCloud2::Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt)
{
  sInt life = 0;
  Part *src;
  sVector31 pos;
  Cluster *cl;
  sF32 ptime,dist;

  sFORALL(Parts,src)
  {
    cl = &Clusters[src->ClusterId];
    ptime = sAbsMod(src->Phase+time*src->Speed,1.0f);
    pos.x = (ptime+dt*src->Speed)*2-1;
    pos.y = src->StartY;
    pos.z = src->StartX;
    dist = sSqrt(pos.x*pos.x+pos.y*pos.y+pos.z*pos.z);
    pos = pos * cl->Matrix;

    if(dist<1)
    {
      ptime = 1-dist;
      life++;
    }
    else
    {
      ptime = -1;
    }

    pinfo.Parts[_i].Init(pos,ptime);
  }
  pinfo.Used = life;
}

/****************************************************************************/
/***                                                                      ***/
/***   add noise to particles                                             ***/
/***                                                                      ***/
/****************************************************************************/

RPWobble::RPWobble()
{
  Source = 0;
  Anim.Init(Wz4RenderType->Script);
}

RPWobble::~RPWobble()
{
  Source->Release();
}

void RPWobble::Init()
{
  Para = ParaBase;

  Random.Resize(Source->GetPartCount());

  sF32 *fp;
  sRandomMT rnd;
  sFORALL(Random,fp)
    *fp = rnd.Float(1);
}

sInt RPWobble::GetPartCount()
{
  return Source->GetPartCount();
}

sInt RPWobble::GetPartFlags()
{
  return Source->GetPartCount();
}

void RPWobble::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  Source->Simulate(ctx);
}

void RPWobble::Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt)
{
  Source->Func(pinfo,time,(Para.Function&16)?1.0f:dt*Para.DeltaFactor);

  sVector31 src;
  sVector30 d;
  sF32 t;

  Wz4Particle *part = pinfo.Parts;
  for(sInt i=0;i<pinfo.Alloc;i++)
  {
    if(part->Time>=0)
    {
      switch(Para.Source)
      {
      default:
      case 0:
        t = part->Time*Para.TimeFactor+Random[i]*Para.Spread+dt;
        src.Init(t,t,t);
        break;
      case 1:
        src = part->Pos;
        break;
      case 2:
        t = time*Para.TimeFactor+Random[i]*Para.Spread+dt + sF32(i)/pinfo.Alloc;
        src.Init(t,t,t);
        break;
      }
      src = src * Para.Freq + Para.Phase;
      switch(Para.Function & 0x0f)
      {
      default:
      case 0x0:
        d.x = sPerlin3D(src.x*0x10000,src.y*0x10000,src.z*0x10000,255,Para.Seed+0);
        d.y = sPerlin3D(src.x*0x10000,src.y*0x10000,src.z*0x10000,255,Para.Seed+1);
        d.z = sPerlin3D(src.x*0x10000,src.y*0x10000,src.z*0x10000,255,Para.Seed+2);
        break;
      case 0x1:
        d.x = sFSin((src.x)*sPI2F)*0.5f;
        d.y = sFSin((src.y)*sPI2F)*0.5f;
        d.z = sFSin((src.z)*sPI2F)*0.5f;
        break;
      }
      part->Pos.x += d.x*Para.Amp.x;
      part->Pos.y += d.y*Para.Amp.y;
      part->Pos.z += d.z*Para.Amp.z;
    }
    part++;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Add trails to particles                                            ***/
/***                                                                      ***/
/****************************************************************************/

RPTrails::RPTrails()
{
  Source = 0;
}

RPTrails::~RPTrails()
{
  Source->Release();
}

sInt RPTrails::GetPartCount()
{
  return Source->GetPartCount()*Para.Count;
}

sInt RPTrails::GetPartFlags()
{
  return 0;
}

void RPTrails::Simulate(Wz4RenderContext *ctx)
{
  Source->Simulate(ctx);

  SimulateCalc(ctx);
}

void RPTrails::Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt)
{
  Para = ParaBase;

  sF32 d = 0;
  if(Para.Count>=2)
    d = Para.Delta/(Para.Count-1);

  Wz4PartInfo::SaveInfo save;
  pinfo.Save(save);
  sInt used = 0;

  for(sInt i=0;i<Para.Count;i++)
  {
    pinfo.Alloc = Source->GetPartCount();
    pinfo.Used = 0;
    Source->Func(pinfo,time,d*i+dt);
    used += pinfo.Used;
    pinfo.Inc(pinfo.Alloc);
  }

  pinfo.Load(save);
  pinfo.Used = used;

  sF32 ic = 1.0f/(Para.Count);

  Wz4Particle *part = pinfo.Parts;
  if(Para.Lifetime)
  {
    for(sInt i=0;i<Para.Count;i++)
    {
      for(sInt j=0;j<Count;j++)
      {
        if(Para.Lifetime==1)
          part->Time = i*ic;
        else if(Para.Lifetime==2)
          part->Time = part->Time * (i*d);

        part++;
      }
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   StaticParticles                                                    ***/
/***                                                                      ***/
/****************************************************************************/

RPStaticParticles::RPStaticParticles()
{
}

RPStaticParticles::~RPStaticParticles()
{
}

void RPStaticParticles::Init(Wz4ParticlesArrayStaticParticles *ar,sInt count)
{
  Parts.Resize(count);

  sMatrix34 mat;
  for(sInt i=0;i<count;i++)
  {
    Parts[i].Time = ar->Time;
    Parts[i].Pos = ar->Pos;

    mat.EulerXYZ(ar->Rot.x*sPI2F,ar->Rot.y*sPI2F,ar->Rot.z*sPI2F);
    Parts[i].Quat.Init(mat);
    Parts[i].Color = ar->Color;

    ar++;
  }

  Flags = 0;
  if(Para.Flags & 1) Flags |= wPNF_Orientation;
  if(Para.Flags & 2) Flags |= wPNF_Color;
}

sInt RPStaticParticles::GetPartCount()
{
  return Parts.GetCount();
}

sInt RPStaticParticles::GetPartFlags()
{
  return Flags;
}

void RPStaticParticles::Simulate(Wz4RenderContext *ctx)
{
  SimulateCalc(ctx);
}

void RPStaticParticles::Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt)
{
  Para = ParaBase;

  Part *p = Parts.GetData();
  for(sInt i=0;i<pinfo.Alloc;i++)
  {
    pinfo.Parts[i].Pos = p[i].Pos;
    pinfo.Parts[i].Time = p[i].Time;
    if(pinfo.Quats)
      pinfo.Quats[i] = p[i].Quat;
    if(pinfo.Colors)
      pinfo.Colors[i] = p[i].Color;
  }
  pinfo.Used = pinfo.Alloc;
  pinfo.Flags = Flags;
}

/****************************************************************************/
/***                                                                      ***/
/***   Add particles                                                      ***/
/***                                                                      ***/
/****************************************************************************/

RPAdd::RPAdd()
{
  Count = 0;
}

RPAdd::~RPAdd()
{
  sReleaseAll(Sources);
}

void RPAdd::Init()
{
  Wz4ParticleNode *src;

  Count = 0;
  sFORALL(Sources,src)
    Count += src->GetPartCount();
}

void RPAdd::Simulate(Wz4RenderContext *ctx)
{
  Wz4ParticleNode *src;

  sFORALL(Sources,src)
    src->Simulate(ctx);

  SimulateCalc(ctx);
}


sInt RPAdd::GetPartCount()
{
  return Count;
}

sInt RPAdd::GetPartFlags()
{
  Wz4ParticleNode *src;

  sInt flags = 0;
  sFORALL(Sources,src)
    flags |= src->GetPartFlags();

  return flags;
}

void RPAdd::Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt)
{
  Wz4ParticleNode *src;

  sInt used = 0;
  Wz4PartInfo::SaveInfo save;

  pinfo.Save(save);
  sFORALL(Sources,src)
  {
    pinfo.Alloc = src->GetPartCount();
    pinfo.Used = 0;
    src->Func(pinfo,time,dt);
    used += pinfo.Used;
    pinfo.Inc(pinfo.Alloc);
  }
  pinfo.Load(save);

  pinfo.Used = used;
  pinfo.Compact = 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   Render particles as ribbon                                         ***/
/***                                                                      ***/
/****************************************************************************/


RNTrails::RNTrails()
{
  InitSinTable();
  Format = sVertexFormatStandard;
  Geo = new sGeometry;
  Geo->Init(sGF_TRILIST|sGF_INDEX32,Format);
  Mtrl = 0;
  Time = 0;

  Source = 0;
  PInfos = 0;

  Anim.Init(Wz4RenderType->Script);
}

RNTrails::~RNTrails()
{
  delete Geo;
  Mtrl->Release();
  Source->Release();
  delete[] PInfos;
}

void RNTrails::Init()
{
  ParaBase.Count = sMax(3,ParaBase.Count);
  Para = ParaBase;

  sInt _count = Source->GetPartCount();
  sInt _flags = Source->GetPartFlags();

  TrailCount = Para.Count;

  PInfos = new Wz4PartInfo [TrailCount];
  for(sInt i=0;i<TrailCount;i++)
    PInfos[i].Init(_flags,_count);
}


void RNTrails::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  Source->Simulate(ctx);
  Time = ctx->GetTime();
  SimulateChilds(ctx);
}

void RNTrails::Prepare(Wz4RenderContext *ctx)
{
  sVertexStandard *vp;
  Wz4Particle *part,*pt;

  if(PInfos[0].Alloc==0 || TrailCount<2) return;

  if(Mtrl) Mtrl->BeforeFrame(Para.LightEnv);

  sF32 delta = Para.Delta/(TrailCount-1);
  PInfos[0].Reset();
  Source->Func(PInfos[0],Time,0);
  for(sInt i=1;i<TrailCount;i++)
  {
    PInfos[i].Reset();
    Source->Func(PInfos[i],Time,delta*i);
  }

  // prepare particle data

  sInt faces = 2;
  sInt extra = 0;
  if((Para.Orientation &0x30)==0x10)
    faces = 4;
  if((Para.Orientation &0x30)==0x20)
  {
    faces = 8;
    extra = 8;
  }

  sInt Current = PInfos[0].Used;
  Geo->BeginLoadVB(Current*TrailCount*faces+Current*extra,sGD_FRAME,&vp);
  sInt test = 0;
  sF32 du = 1.0f/(TrailCount-1);
  sVector30 dir,k,side,wide,da,db;
  sVector31 p0,p1,p2;
  sMatrix34 mat;
  switch(Para.Orientation & 0x0f)
  {
  case 0:
  default:
    {
      sMatrix34 mat = sMatrix34(Matrices[0]) * Wz4MtrlType->View;
      k.Init(mat.i.z,mat.j.z,mat.k.z);
    }
    break;
  case 1:
    k.Init(1,0,0);
    break;
  case 2:
    k.Init(0,1,0);
    break;
  case 3:
    k.Init(0,0,1);
    break;
  case 4:
    k.Init(0,0,0);
  }
  sInt step = sMin(Para.TrailStep,Para.Count/2-1);
  if(step<1) step = 1;

  sF32 width = Para.Width;
  sInt max = PInfos[0].GetCount();
  for(sInt i=0;i<max;i++)
  {
    part = &PInfos[0].Parts[i];
    if(part->Time>=0)
    {
      for(sInt j=0;j<TrailCount;j++)
      {
        if((Para.Orientation&0x0f)==4)
        {
          sInt j1 = sClamp(j,step,TrailCount-1-step);
          sInt j0 = j1-step;
          sInt j2 = j1+step;

          p0 = PInfos[j0].Parts[i].Pos;
          p1 = PInfos[j1].Parts[i].Pos;
          p2 = PInfos[j2].Parts[i].Pos;
          mat.ThreePoint(p0,p1,p2,Para.Tweak);

          dir = mat.k;
          side = mat.i;
          wide = mat.j;
        }
        else
        {
          sInt j0 = sMax(j-1,0);
          sInt j1 = sMin(j+1,TrailCount-1);

          dir = PInfos[j0].Parts[i].Pos - PInfos[j1].Parts[i].Pos;
          side.Cross(dir,k);
          side.Unit();
          wide.Cross(side,dir);
          wide.Unit();
        }
        side *= width;
        wide *= width;

        if(faces==2)
        {
          pt = &PInfos[j].Parts[i];
          vp->px = pt->Pos.x+side.x;
          vp->py = pt->Pos.y+side.y;
          vp->pz = pt->Pos.z+side.z;
          vp->nx = wide.x;
          vp->ny = wide.y;
          vp->nz = wide.z;
          vp->u0 = j*du;
          vp->v0 = 0;
          vp++;
          vp->px = pt->Pos.x-side.x;
          vp->py = pt->Pos.y-side.y;
          vp->pz = pt->Pos.z-side.z;
          vp->nx = wide.x;
          vp->ny = wide.y;
          vp->nz = wide.z;
          vp->u0 = j*du;
          vp->v0 = 1;
          vp++;
        }
        else if(faces==4)
        {
          pt = &PInfos[j].Parts[i];
          vp->px = pt->Pos.x+side.x+wide.x;
          vp->py = pt->Pos.y+side.y+wide.y;
          vp->pz = pt->Pos.z+side.z+wide.z;
          vp->nx =  side.x+wide.x;
          vp->ny =  side.y+wide.y;
          vp->nz =  side.z+wide.z;
          vp->u0 = j*du;
          vp->v0 = 0;
          vp++;
          vp->px = pt->Pos.x-side.x+wide.x;
          vp->py = pt->Pos.y-side.y+wide.y;
          vp->pz = pt->Pos.z-side.z+wide.z;
          vp->nx = -side.x+wide.x;
          vp->ny = -side.y+wide.y;
          vp->nz = -side.z+wide.z;
          vp->u0 = j*du;
          vp->v0 = 1;
          vp++;
          vp->px = pt->Pos.x-side.x-wide.x;
          vp->py = pt->Pos.y-side.y-wide.y;
          vp->pz = pt->Pos.z-side.z-wide.z;
          vp->nx = -side.x-wide.x;
          vp->ny = -side.y-wide.y;
          vp->nz = -side.z-wide.z;
          vp->u0 = j*du;
          vp->v0 = 1;
          vp++;
          vp->px = pt->Pos.x+side.x-wide.x;
          vp->py = pt->Pos.y+side.y-wide.y;
          vp->pz = pt->Pos.z+side.z-wide.z;
          vp->nx =  side.x-wide.x;
          vp->ny =  side.y-wide.y;
          vp->nz =  side.z-wide.z;
          vp->u0 = j*du;
          vp->v0 = 0;
          vp++;
        }
        else
        {
          if(j==0)
          {
            dir.Unit();
            dir  = -dir;

            pt = &PInfos[j].Parts[i];
            vp->px = pt->Pos.x+side.x+wide.x;
            vp->py = pt->Pos.y+side.y+wide.y;
            vp->pz = pt->Pos.z+side.z+wide.z;
            vp->nx = dir.x;
            vp->ny = dir.y;
            vp->nz = dir.z;
            vp->u0 = j*du;
            vp->v0 = 0;
            vp++;

            vp->px = pt->Pos.x-side.x+wide.x;
            vp->py = pt->Pos.y-side.y+wide.y;
            vp->pz = pt->Pos.z-side.z+wide.z;
            vp->nx = dir.x;
            vp->ny = dir.y;
            vp->nz = dir.z;
            vp->u0 = j*du;
            vp->v0 = 1;
            vp++;

            vp->px = pt->Pos.x-side.x-wide.x;
            vp->py = pt->Pos.y-side.y-wide.y;
            vp->pz = pt->Pos.z-side.z-wide.z;
            vp->nx = dir.x;
            vp->ny = dir.y;
            vp->nz = dir.z;
            vp->u0 = j*du;
            vp->v0 = 1;
            vp++;

            vp->px = pt->Pos.x+side.x-wide.x;
            vp->py = pt->Pos.y+side.y-wide.y;
            vp->pz = pt->Pos.z+side.z-wide.z;
            vp->nx = dir.x;
            vp->ny = dir.y;
            vp->nz = dir.z;
            vp->u0 = j*du;
            vp->v0 = 0;
            vp++;
          }

          pt = &PInfos[j].Parts[i];
          vp->px = pt->Pos.x+side.x+wide.x;
          vp->py = pt->Pos.y+side.y+wide.y;
          vp->pz = pt->Pos.z+side.z+wide.z;
          vp->nx = wide.x;
          vp->ny = wide.y;
          vp->nz = wide.z;
          vp->u0 = j*du;
          vp->v0 = 0;
          vp++;
          vp->px = pt->Pos.x-side.x+wide.x;
          vp->py = pt->Pos.y-side.y+wide.y;
          vp->pz = pt->Pos.z-side.z+wide.z;
          vp->nx = wide.x;
          vp->ny = wide.y;
          vp->nz = wide.z;
          vp->u0 = j*du;
          vp->v0 = 1;
          vp++;

          vp->px = pt->Pos.x-side.x+wide.x;
          vp->py = pt->Pos.y-side.y+wide.y;
          vp->pz = pt->Pos.z-side.z+wide.z;
          vp->nx = -side.x;
          vp->ny = -side.y;
          vp->nz = -side.z;
          vp->u0 = j*du;
          vp->v0 = 1;
          vp++;
          vp->px = pt->Pos.x-side.x-wide.x;
          vp->py = pt->Pos.y-side.y-wide.y;
          vp->pz = pt->Pos.z-side.z-wide.z;
          vp->nx = -side.x;
          vp->ny = -side.y;
          vp->nz = -side.z;
          vp->u0 = j*du;
          vp->v0 = 1;
          vp++;

          vp->px = pt->Pos.x-side.x-wide.x;
          vp->py = pt->Pos.y-side.y-wide.y;
          vp->pz = pt->Pos.z-side.z-wide.z;
          vp->nx = -wide.x;
          vp->ny = -wide.y;
          vp->nz = -wide.z;
          vp->u0 = j*du;
          vp->v0 = 1;
          vp++;
          vp->px = pt->Pos.x+side.x-wide.x;
          vp->py = pt->Pos.y+side.y-wide.y;
          vp->pz = pt->Pos.z+side.z-wide.z;
          vp->nx = -wide.x;
          vp->ny = -wide.y;
          vp->nz = -wide.z;
          vp->u0 = j*du;
          vp->v0 = 0;
          vp++;

          vp->px = pt->Pos.x+side.x-wide.x;
          vp->py = pt->Pos.y+side.y-wide.y;
          vp->pz = pt->Pos.z+side.z-wide.z;
          vp->nx = side.x;
          vp->ny = side.y;
          vp->nz = side.z;
          vp->u0 = j*du;
          vp->v0 = 0;
          vp++;
          vp->px = pt->Pos.x+side.x+wide.x;
          vp->py = pt->Pos.y+side.y+wide.y;
          vp->pz = pt->Pos.z+side.z+wide.z;
          vp->nx = side.x;
          vp->ny = side.y;
          vp->nz = side.z;
          vp->u0 = j*du;
          vp->v0 = 0;
          vp++;

          if(j==TrailCount-1)
          {
            dir.Unit();
//            dir = -dir;

            pt = &PInfos[j].Parts[i];
            vp->px = pt->Pos.x+side.x+wide.x;
            vp->py = pt->Pos.y+side.y+wide.y;
            vp->pz = pt->Pos.z+side.z+wide.z;
            vp->nx = dir.x;
            vp->ny = dir.y;
            vp->nz = dir.z;
            vp->u0 = j*du;
            vp->v0 = 0;
            vp++;

            vp->px = pt->Pos.x-side.x+wide.x;
            vp->py = pt->Pos.y-side.y+wide.y;
            vp->pz = pt->Pos.z-side.z+wide.z;
            vp->nx = dir.x;
            vp->ny = dir.y;
            vp->nz = dir.z;
            vp->u0 = j*du;
            vp->v0 = 1;
            vp++;

            vp->px = pt->Pos.x-side.x-wide.x;
            vp->py = pt->Pos.y-side.y-wide.y;
            vp->pz = pt->Pos.z-side.z-wide.z;
            vp->nx = dir.x;
            vp->ny = dir.y;
            vp->nz = dir.z;
            vp->u0 = j*du;
            vp->v0 = 1;
            vp++;

            vp->px = pt->Pos.x+side.x-wide.x;
            vp->py = pt->Pos.y+side.y-wide.y;
            vp->pz = pt->Pos.z+side.z-wide.z;
            vp->nx = dir.x;
            vp->ny = dir.y;
            vp->nz = dir.z;
            vp->u0 = j*du;
            vp->v0 = 0;
            vp++;
          }
        }
      }
      test++;
    }
  }
  Geo->EndLoadVB();
  sVERIFY(test==Current);

  if(faces==2)
  {
    sU32 *ip;
    sInt n = 0;
    Geo->BeginLoadIB(Current*(TrailCount-1)*6,sGD_FRAME,&ip);
    for(sInt i=0;i<Current;i++)
    {
      for(sInt j=0;j<TrailCount-1;j++)
      {
        sQuad(ip,n,2,3,1,0);
        n+=2;
      }
      n+=2;
    }
    Geo->EndLoadIB();
  }
  else if(faces==4)
  {
    sU32 *ip;
    sInt n = 0;
    Geo->BeginLoadIB(Current*((TrailCount)*4*6+2*6),sGD_FRAME,&ip);
    for(sInt i=0;i<Current;i++)
    {
      sQuad(ip,n,3,2,1,0);
      for(sInt j=0;j<TrailCount-1;j++)
      {
        sQuad(ip,n,0,1,5,4);
        sQuad(ip,n,1,2,6,5);
        sQuad(ip,n,2,3,7,6);
        sQuad(ip,n,3,0,4,7);
        n+=4;
      }
      sQuad(ip,n,0,1,2,3);
      n+=4;
    }
    Geo->EndLoadIB();
  }
  else
  {
    sU32 *ip;
    sInt n = 0;
    Geo->BeginLoadIB(Current*((TrailCount-1)*4*6+2*6),sGD_FRAME,&ip);
    for(sInt i=0;i<Current;i++)
    {
      sQuad(ip,n,3,2,1,0);
      n+=4;
      for(sInt j=0;j<TrailCount-1;j++)
      {
        sQuad(ip,n,0,1, 9, 8);
        sQuad(ip,n,2,3,11,10);
        sQuad(ip,n,4,5,13,12);
        sQuad(ip,n,6,7,15,14);
        n+=8;
      }
      n+=8;
      sQuad(ip,n,0,1,2,3);
      n+=4;
    }
    Geo->EndLoadIB();
  }
}

void RNTrails::Render(Wz4RenderContext *ctx)
{
  if(ctx->IsCommonRendermode() && PInfos[0].Used>0)
  {
    if(Mtrl->SkipPhase(ctx->RenderMode,Para.LightEnv&15)) return;
    sMatrix34CM *model;
    sFORALL(Matrices,model)
    {
      Mtrl->Set(ctx->RenderMode|sRF_MATRIX_ONE,Para.LightEnv&15,model,0,0,0);
      Geo->Draw();
    }
  } 
}

/****************************************************************************/
/***                                                                      ***/
/***   lissajous particle generator (very cool)                           ***/
/***                                                                      ***/
/****************************************************************************/

RPLissajous::RPLissajous()
{
  _Phase  = AddSymbol(L"Phase");
  _Freq   = AddSymbol(L"Freq");
  _Amp    = AddSymbol(L"Amp");
  Anim.Init(Wz4RenderType->Script);
  Source = 0;
  InitSinTable();
}

RPLissajous::~RPLissajous()
{
  Source->Release();
}

void RPLissajous::Init()
{
  Part *p;
  sRandomMT rnd;

  Para = ParaBase;

  rnd.Seed(Para.Seed);
  if(Source)
    Parts.Resize(Source->GetPartCount());
  else
    Parts.Resize(Para.Count);
  sFORALL(Parts,p)
  {
    if(Para.Flags&4)
      p->Start = sFMod(((rnd.Float(1)-0.5f)*Para.Randomness+_i)/sF32(Parts.GetCount()),1.0f);
    else
      p->Start = rnd.Float(1);
    p->Speed = 1+(rnd.Float(2)-1)*Para.SpeedSpread;
    if(Para.Flags & 2)
    {
      sVector30 v,u;
      v.InitRandom(rnd);
      v.z = 0;
      v.Unit();

      p->Pos = sVector31(v*Para.Scale + v*(rnd.Float(1)-0.5)*Para.TubeRandom);
    }
    else
    {
      p->Pos.InitRandom(rnd);
      p->Pos = p->Pos*Para.Scale;
    }
  }

  sInt n = Curves.GetCount();
  Phases.Resize(n);
  Freqs.Resize(n);
  Amps.Resize(n);
  for(sInt i=0;i<n;i++)
  {
    Phases[i] = Curves[i].Phase;
    Freqs [i] = Curves[i].Freq;
    Amps  [i] = Curves[i].Amp;
  }
  Para.Lifetime = sMax<sF32>(0.01f,Para.Lifetime);
  Para.Spread = sMax<sF32>(0,Para.Spread);
}

sInt RPLissajous::GetPartCount()
{
  return Parts.GetCount();
}

sInt RPLissajous::GetPartFlags()
{
  return Source ? Source->GetPartFlags() : 0;
}

/****************************************************************************/

void RPLissajous::Simulate(Wz4RenderContext *ctx)
{
  sInt n = Curves.GetCount();
  for(sInt i=0;i<n;i++)
  {
    Phases[i] = Curves[i].Phase;
    Freqs [i] = Curves[i].Freq;
    Amps  [i] = Curves[i].Amp;
  }
  Para = ParaBase;

  ctx->Script->BindLocalFloat(_Phase ,n,Phases.GetData());
  ctx->Script->BindLocalFloat(_Freq  ,n,Freqs.GetData());
  ctx->Script->BindLocalFloat(_Amp   ,n,Amps.GetData());

  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  Para.Lifetime = sMax<sF32>(0.01f,Para.Lifetime);
  Para.Spread = sMax<sF32>(0,Para.Spread);
}


void RPLissajous::Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt)
{
  if(Source)
    Source->Func(pinfo,time,dt);

  sInt n = Curves.GetCount();

  time /= Para.Lifetime;
  //dt /= Para.Lifetime;
  Part *p;
  //sVector31 p0,p1,p2;
  sVector30 dx,dy;
  sF32 f0[5],f1[5],f2[5];
  sMatrix34 mat;

  const sF32 e=0.01f;

  Wz4Particle *part = pinfo.Parts;
  sFORALL(Parts,p)
  {
    sF32 t = time*p->Speed+p->Start*Para.Spread+Para.MasterPhase;
    if(Para.Flags & 1)
      t = sMax<sF32>(t,0);
    t = sAbsMod(t,1.0f);

    sClear(f0);
    sClear(f1);
    sClear(f2);
    f1[4] = Para.MasterScale;

    for(sInt i=0;i<n;i++)
    {
      sF32 freq = Freqs[i]*SinTableLen;
      sF32 phase = Phases[i];
      sF32 amp = Amps[i];
      sInt axis = Curves[i].Axis;

      sF32 xf=(t+dt+phase)*freq;
      sInt xi=sInt(xf);
      xf-=sF32(xi);
      sInt ei=(e*freq);
      xi-=ei;

      f0[axis] += amp*sFade(xf,SinTable[xi&(SinTableLen-1)],SinTable[(xi+1)&(SinTableLen-1)]);
      xi+=ei;
      f1[axis] += amp*sFade(xf,SinTable[xi&(SinTableLen-1)],SinTable[(xi+1)&(SinTableLen-1)]);
      xi+=ei;
      f2[axis] += amp*sFade(xf,SinTable[xi&(SinTableLen-1)],SinTable[(xi+1)&(SinTableLen-1)]);
    }

    // fake fast ThreePoint impl
    sF32 dbx,dby,dbz,kx, ky, kz, il2;
    kx=f2[0]-f0[0];
    ky=f2[1]-f0[1];
    kz=f2[2]-f0[2];
    il2 = FastRSqrt(kx*kx+ky*ky+kz*kz);
    mat.k.Init(kx*il2,ky*il2,kz*il2);

    dbx=f1[0]-f0[0];
    dby=f1[1]-f0[1];
    dbz=f1[2]-f0[2];
    il2 = FastRSqrt(dbx*dbx+dby*dby+dbz*dbz);
    dbx*=il2; dby*=il2; dbz*=il2;

    mat.i.x=dby*mat.k.z-dbz*mat.k.y+Para.Tweak.x; 
    mat.i.y=dbz*mat.k.x-dbx*mat.k.z+Para.Tweak.y; 
    mat.i.z=dbx*mat.k.y-dby*mat.k.x+Para.Tweak.z;
    mat.j.Cross(mat.k,mat.i); 
    mat.i*=FastRSqrt(mat.i.LengthSq());
    mat.j*=FastRSqrt(mat.j.LengthSq());

    sF32 xf=(f1[3]+(t+dt)*Para.LinearTwirl+Para.LinearTwist)*SinTableLen;
    sInt xi=sInt(xf);
    xf-=sF32(xi);
    
    sF32 s=sFade(xf,SinTable[xi&(SinTableLen-1)],SinTable[(xi+1)&(SinTableLen-1)]);
    sF32 c=sFade(xf,SinTable[(xi+SinTableLen/4)&(SinTableLen-1)],SinTable[(xi+SinTableLen/4+1)&(SinTableLen-1)]);
    dx = mat.i*s + mat.j*c;
    dy = mat.i*c - mat.j*s;

    sVector31 pos(f1[0],f1[1],f1[2]);
    pos += Para.LinearStretch * (t+dt); 
    pos += (dx*p->Pos.x + dy*p->Pos.y + mat.k*p->Pos.z)*f1[4];
    if(Source)
      pos += sVector30(part->Pos);

    part->Init(pos+sVector30(Para.Translate),t);
    part++;
  }
  pinfo.Used = pinfo.Alloc;
  pinfo.Compact = 1;
}


/****************************************************************************/
/***                                                                      ***/
/***   Particles along spline                                             ***/
/***                                                                      ***/
/****************************************************************************/

RPSplinedParticles::RPSplinedParticles()
{
  Anim.Init(Wz4RenderType->Script);
  Source = 0;
  Spline = AltSpline = 0;
}

RPSplinedParticles::~RPSplinedParticles()
{
  Source->Release();
}

void RPSplinedParticles::Init()
{
  Part *p;
  sRandomMT rnd;

  Para = ParaBase;

  rnd.Seed(Para.Seed);
  if(Source)
    Parts.Resize(Source->GetPartCount());
  else
    Parts.Resize(Para.Count);
  sFORALL(Parts,p)
  {
    p->Start = rnd.Float(1);
    if(Para.Flags & 2)
    {
      sVector30 v,u;
      v.InitRandom(rnd);
      v.z = 0;
      v.Unit();

      p->Pos = sVector31(v*Para.Scale + v*(rnd.Float(1)-0.5)*Para.TubeRandom);
    }
    else
    {
      p->Pos.InitRandom(rnd);
      p->Pos = p->Pos*Para.Scale;
    }
  }

  Symbol = Wz4RenderType->Script->AddSymbol(Name);
  AltSymbol = Wz4RenderType->Script->AddSymbol(AltName);
}

/****************************************************************************/

void RPSplinedParticles::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;

  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  Spline = AltSpline = 0;
  if(Symbol && Symbol->Value && Symbol->Value->Spline) 
    Spline = Symbol->Value->Spline;
  if(AltSymbol && AltSymbol->Value && AltSymbol->Value->Spline) 
    AltSpline = AltSymbol->Value->Spline;

  Para.Lifetime = sMax<sF32>(0.01f,Para.Lifetime);
  Para.Spread = sMax<sF32>(0,Para.Spread);
}

sInt RPSplinedParticles::GetPartCount()
{
  return Parts.GetCount();
}

sInt RPSplinedParticles::GetPartFlags()
{
  return 0;
}

void RPSplinedParticles::Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt)
{
  time /= Para.Lifetime;
  //dt /= Para.Lifetime;
  Part *p;
  sVector31 p0,p1,p2;
  sVector30 dx,dy;
  sF32 val[3][5];
  sF32 vala[3][5];
  sMatrix34 mat;

  if(Source)
    Source->Func(pinfo,time,dt);

  if(!Spline)
  {
    for(sInt i=0;i<pinfo.Alloc;i++)
      pinfo.Parts[i].Init(sVector31(0,0,0),-1);
    pinfo.Used = 0;
    return;
  }
  ScriptSpline *spl = Spline;
  ScriptSpline *alt = AltSpline;
  sInt altchannels;
  sF32 altlength;
  sInt channels = sMin(spl->Count,5);
  sF32 length = spl->Length();

  if(alt)
  {
    altchannels = sMin(alt->Count,5);
    altlength = alt->Length();     
  }

  sClear(val);
  sClear(vala);
  val[0][4] = 1;
  val[1][4] = 1;
  val[2][4] = 1;
  vala[0][4] = 1;
  vala[1][4] = 1;
  vala[2][4] = 1;

  sF32 e=Para.Epsilon;

  sFORALL(Parts,p)
  {
    sF32 t;
    if(Para.Flags & 4)
      t = time+((p->Start-0.5f)*Para.Randomness+_i)/sF32(Parts.GetCount())*Para.Spread;
    else
      t = time+p->Start*Para.Spread;
    t += Para.MasterPhase;
    if(Para.Flags & 1)
      t = sMax<sF32>(t,0);
    if(!(Para.Flags & 16))
      t = sAbsMod(t,1.0f);

    for(sInt i=0;i<3;i++)
    {
      spl->Eval((t + dt + e*(i-1))*length,val[i],channels);
      if(alt)
      {
        alt->Eval((t + dt + e*(i-1))*altlength+Para.AltShift,vala[i],altchannels);
      }
    }
    
    p0.Init(val[0][0]+vala[0][0],val[0][1]+vala[0][1],val[0][2]+vala[0][2]);
    p1.Init(val[1][0]+vala[1][0],val[1][1]+vala[1][1],val[1][2]+vala[1][2]);
    p2.Init(val[2][0]+vala[2][0],val[2][1]+vala[2][1],val[2][2]+vala[2][2]);
    if(Para.Flags & 0x08)
    {
      mat.Look_(p2-p0,Para.Tweak);
    }
    else
    {
      mat.ThreePoint_(p0,p1,p2,Para.Tweak);
    }

    sF32 twirl = val[1][3]+vala[1][3] + (t+dt)*Para.LinearTwirl;
    sF32 s,c;
    sFSinCos(twirl*sPI2F,s,c);
    dx = mat.i*c + mat.j*s;
    dy = mat.i*s - mat.j*c;

    p1 = p1 + (dx*p->Pos.x + dy*p->Pos.y + mat.k*p->Pos.z)*(val[1][4]+vala[1][4]);
    if(Source)
    {
      p1 += sVector30(pinfo.Parts[_i].Pos);
    }
    pinfo.Parts[_i].Init(p1,t);
  }
  pinfo.Used = pinfo.Alloc;
}

/****************************************************************************/
/***                                                                      ***/
/***   particle force field                                               ***/
/***                                                                      ***/
/****************************************************************************/

RPBulge::RPBulge()
{
  Source = 0;
  Anim.Init(Wz4RenderType->Script);
}

RPBulge::~RPBulge()
{
  Source->Release();
}

void RPBulge::Init()
{
  Para = ParaBase;
}

sInt RPBulge::GetPartCount()
{
  return Source->GetPartCount();
}
sInt RPBulge::GetPartFlags()
{
  return Source->GetPartFlags();
}

void RPBulge::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
  Source->Simulate(ctx);
}

void RPBulge::Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt)
{
  Source->Func(pinfo,time,dt);
  sVector31 center = Para.Center;
  sVector30 size = Para.Size, invSize;
  sF32 warp = Para.Warp;

  invSize.x = 1.0f / size.x;
  invSize.y = 1.0f / size.y;
  invSize.z = 1.0f / size.z;

  size = size * Para.Stretch;

  Wz4Particle *part = pinfo.Parts;
  for(sInt i=0;i<pinfo.Alloc;i++)
  {
    if(part->Time >= 0)
    {
      sVector30 d;
      d = (part->Pos - center) * invSize;
      sF32 lenSq = d.LengthSq();

      if(lenSq <= 1.0f)
      {
        sF32 len = sFSqrt(lenSq);
        sF32 smoothRevLen = sSmoothStep(1.0f - len);
        sF32 sc = 1.0f;

        switch(Para.Function)
        {
        case 0: // 3D Bulge
          sc = 1.0f / (1.0f + warp * sFSqrt(1.0f - lenSq));
          break;

        case 1: // S-Bulge
          sc = 1.0f / (1.0f + warp * smoothRevLen);
          break;
        }

        sF32 fadeF = sFPow(smoothRevLen,Para.FadeGamma);
        part->Pos.x = sFade(fadeF,part->Pos.x,center.x + size.x * sc * d.x);
        part->Pos.y = sFade(fadeF,part->Pos.y,center.y + size.y * sc * d.y);
        part->Pos.z = sFade(fadeF,part->Pos.z,center.z + size.z * sc * d.z);

        // TODO: calc jacobian to transform orientations?
      }
    }

    part++;
  }
  // no need to change Used
}

/****************************************************************************/
/***                                                                      ***/
/***   Add Sparcles to particles                                          ***/
/***                                                                      ***/
/****************************************************************************/

RPSparcle::RPSparcle()
{
  Source = 0;
  MaxSparks = 0;
  NeedInit = sTRUE;
}

RPSparcle::~RPSparcle()
{
  Source->Release();
}

void RPSparcle::Init()
{
  Para = ParaBase;
  MaxSparks = Source->GetPartCount() * Para.SamplePoints * Para.Percentage;
  NeedInit = sTRUE;
}

void RPSparcle::DelayedInit()
{
  sRandom rnd;
  rnd.Seed(Para.RandomSeed);
  sInt maxsrc = Source->GetPartCount();

  Wz4PartInfo part[2];
  sInt db = 0;
  
  part[0].Init(0,maxsrc);
  part[1].Init(0,maxsrc);

  part[db].Reset();
  Source->Func(part[db],-1.0f,0);

  Sparcs.Clear();
  for(sInt i=0;i<Para.SamplePoints;i++)
  {
    db = !db;
    sF32 time = sF32(i)/Para.SamplePoints;
    part[db].Reset();
    Source->Func(part[db],time,0);

    for(sInt j=0;j<maxsrc;j++)
    {
      if(part[db].Parts[j].Time>=0 && rnd.Float(1)<Para.Percentage && Sparcs.GetCount()<MaxSparks)
      {
        sVector30 speed;
        Sparc *s = Sparcs.AddMany(1);

        s->Time0 = time;
        s->Pos = part[db].Parts[j].Pos;
        speed.InitRandom(rnd);
        s->Speed = speed*Para.RandomSpeed;
        speed = part[db].Parts[j].Pos-part[!db].Parts[j].Pos;
        speed.Unit();
        s->Speed += speed*Para.DirectionSpeed;
      }
    }
  }

  NeedInit = sFALSE;
}

sInt RPSparcle::GetPartCount()
{
  return MaxSparks;
}

sInt RPSparcle::GetPartFlags()
{
  return Source->GetPartFlags();
}

void RPSparcle::Simulate(Wz4RenderContext *ctx)
{
  Source->Simulate(ctx);
  SimulateCalc(ctx);

  if (NeedInit)
    DelayedInit();
}

void RPSparcle::Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt)
{
  Sparc *s;
  Para = ParaBase;
  sVector30 g;
  
  g.Init(0,Para.Gravity,0);

  sInt used = 0;
  sF32 il = 1/Para.Lifetime;

  sFORALL(Sparcs,s)
  {
    Wz4Particle *p = pinfo.Parts+_i;
    sF32 t = (time-s->Time0)*il;
    sF32 tt = t;
    if(t<0 || t>1)
      tt = -1;
    else 
      used++;

    p->Init(s->Pos+s->Speed*t+g*t*t,tt);
  }
  pinfo.Used = used;
  pinfo.Flags = 0;
}

/****************************************************************************/

RPFromMesh::RPFromMesh()
{
}

RPFromMesh::~RPFromMesh()
{
}

void RPFromMesh::Init(Wz4Mesh *mesh)
{
  sAABBox box;
  mesh->CalcBBox(box);

  Wz4BSP *bsp = new Wz4BSP;
  bsp->FromMesh(mesh,0.001f);

  sF32 s = Para.Raster;
  sF32 rs = 1.0f/Para.Raster;
  sF32 xo = Para.Offset.x;
  sF32 yo = Para.Offset.y;
  sF32 zo = Para.Offset.z;
  
  sInt x0 = sRoundDown(box.Min.x*rs);
  sInt y0 = sRoundDown(box.Min.y*rs);
  sInt z0 = sRoundDown(box.Min.z*rs);
  sInt x1 = sRoundUp(box.Max.x*rs);
  sInt y1 = sRoundUp(box.Max.y*rs);
  sInt z1 = sRoundUp(box.Max.z*rs);
  sInt xm = x1-x0;
  sInt ym = y1-y0;
  sInt zm = z1-z0;

  if(zm*ym*xm==0 || zm*ym*xm > 0x1000000) return;

  sU8 *map = new sU8[zm*ym*xm];

  for(sInt z=z0;z<z1;z++)
  {
    for(sInt y=y0;y<y1;y++)
    {
      for(sInt x=x0;x<x1;x++)
      {
        sInt ok = bsp->IsInside(sVector31(x*s+xo,y*s+yo,z*s+zo));
        map[(z-z0)*ym*xm + (y-y0)*xm + (x-x0)] = ok;
      }
    }
  }

  sInt offset[26] =
  {
    0 - 1*ym*xm - 1*xm - 1,
    0 - 1*ym*xm - 1*xm + 0,
    0 - 1*ym*xm - 1*xm + 1,
    0 - 1*ym*xm + 0*xm - 1,
    0 - 1*ym*xm + 0*xm + 0,
    0 - 1*ym*xm + 0*xm + 1,
    0 - 1*ym*xm + 1*xm - 1,
    0 - 1*ym*xm + 1*xm + 0,
    0 - 1*ym*xm + 1*xm + 1,

    0 - 0*ym*xm - 1*xm - 1,
    0 - 0*ym*xm - 1*xm + 0,
    0 - 0*ym*xm - 1*xm + 1,
    0 - 0*ym*xm + 0*xm - 1,
//    0 - 0*ym*xm + 0*xm + 0,
    0 - 0*ym*xm + 0*xm + 1,
    0 - 0*ym*xm + 1*xm - 1,
    0 - 0*ym*xm + 1*xm + 0,
    0 - 0*ym*xm + 1*xm + 1,

    0 + 1*ym*xm - 1*xm - 1,
    0 + 1*ym*xm - 1*xm + 0,
    0 + 1*ym*xm - 1*xm + 1,
    0 + 1*ym*xm + 0*xm - 1,
    0 + 1*ym*xm + 0*xm + 0,
    0 + 1*ym*xm + 0*xm + 1,
    0 + 1*ym*xm + 1*xm - 1,
    0 + 1*ym*xm + 1*xm + 0,
    0 + 1*ym*xm + 1*xm + 1,
  };

  if(Para.Flags & 1)
  {
    for(sInt z=z0+1;z<z1-1;z++)
    {
      for(sInt y=y0+1;y<y1-1;y++)
      {
        for(sInt x=x0+1;x<x1-1;x++)
        {
          sInt bit = map[(z-z0)*ym*xm + (y-y0)*xm + (x-x0)];
          if(bit)
          {
            sInt kill = 1;
            for(sInt i=0;i<26;i++)
            {
              if(map[(z-z0)*ym*xm + (y-y0)*xm + (x-x0) + offset[i]]==0)
              {
                kill = 0;
                break;
              }
            }
            if(kill)
              map[(z-z0)*ym*xm + (y-y0)*xm + (x-x0)] = 2;
          }
        }
      }
    }
  }

  sRandom rnd;
  rnd.Seed(Para.RandomSeed);

  for(sInt z=z0;z<z1;z++)
  {
    for(sInt y=y0;y<y1;y++)
    {
      for(sInt x=x0;x<x1;x++)
      {
        if(map[(z-z0)*ym*xm + (y-y0)*xm + (x-x0)]==1)
        {
          if (rnd.Float(1)<=Para.Random)
          {
            Part *p = Parts.AddMany(1);
            p->Pos.Init(x*s+xo,y*s+yo,z*s+zo);
            p->Size = s;
          }
        }
      }
    }
  }

  delete[] map;
  bsp->Release();
}


void RPFromMesh::Simulate(Wz4RenderContext *ctx)
{
}

sInt RPFromMesh::GetPartCount()
{
  return Parts.GetCount();
}

sInt RPFromMesh::GetPartFlags()
{
  return 0;
}

void RPFromMesh::Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt)
{
  sInt max = pinfo.Alloc;
  for(sInt i=0;i<max;i++)
  {
    pinfo.Parts[i].Init(Parts[i].Pos,time+dt);
  }
  pinfo.Used = pinfo.Alloc;
}

/****************************************************************************/
/***                                                                      ***/
/***   FromVertex (generate node particles from each vertex in the mesh)  ***/
/***                                                                      ***/
/****************************************************************************/

RPFromVertex::RPFromVertex()
{
}

RPFromVertex::~RPFromVertex()
{
}

// For sorting (to identify unique verts)
static inline bool operator <(const sVector31 &a, const sVector31 &b)
{
  if (a.x != b.x) return a.x < b.x;
  if (a.y != b.y) return a.y < b.y;
  return a.z < b.z;
}

void RPFromVertex::Init(Wz4Mesh *mesh)
{
  sArray<sVector31> positions;
  Wz4MeshVertex * vp;
  Para = ParaBase;
  sRandom rnd;
  rnd.Seed(Para.RandomSeed);

  sVERIFY(mesh != 0);

  // build list of all positions (including duplicates)
  sFORALL(mesh->Vertices, vp)
  {
    if(logic(Para.Selection, vp->Select))
      positions.AddTail(vp->Pos);
  }

  // sort to identify unique particles
  sIntroSort(sAll(positions));
  for(sInt i=0; i < positions.GetCount(); i++)
  {
    if((i == 0 || positions[i] != positions[i-1]) && // haven't seen this one before
      rnd.Float(1) <= Para.Random)
    {
      Part *p = Parts.AddMany(1);
      p->Pos = positions[i];
    }
  }
}


void RPFromVertex::Simulate(Wz4RenderContext *ctx)
{
  //SimulateCalc(ctx);
}

sInt RPFromVertex::GetPartCount()
{
  return Parts.GetCount();
}

sInt RPFromVertex::GetPartFlags()
{
  return 0;
}

void RPFromVertex::Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt)
{
  sInt count = Parts.GetCount();

  for(sInt i=0;i<count;i++)
  {
    pinfo.Parts[i].Init(Parts[i].Pos,1.0f);
  }
  pinfo.Used = count;
}

/****************************************************************************/

RPMorph::RPMorph()
{
  Anim.Init(Wz4RenderType->Script);
  max = 0;
}

RPMorph::~RPMorph()
{
}

void RPMorph::Init(sArray<Wz4Mesh*> meshArray)
{
  sArray<sVector31> positions;
  Wz4MeshVertex * vp;
  Wz4Mesh* meshTmp;
  sInt meshIndex = 0;

  sFORALL(meshArray, meshTmp)
  {
    sVERIFY(meshTmp != 0);

    sFORALL(meshTmp->Vertices, vp)
    {
      Part *p = shapeParts[meshIndex].AddMany(1);
      p->Pos.Init(vp->Pos.x, vp->Pos.y, vp->Pos.z);
    }

    max = sMax(meshTmp->Vertices.GetCount(),max);

    meshIndex++;
  }
}

sInt RPMorph::GetPartCount()
{
  return max;
}

sInt RPMorph::GetPartFlags()
{
  return 0;
}

void RPMorph::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
}

void RPMorph::Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt)
{
  sRandomMT mt;
  mt.Seed(time);

  sInt rIndex = 0;
  sVector31 v;

  for(sInt i=0; i<max; i++)
  {
    sVector31 pos;

    if(Para.Transition < 1.0f)
    {
      // transition is not yet achieved

      if(i<shapeParts[0].GetCount() && i<shapeParts[1].GetCount())
      {
        v = shapeParts[0][i].Pos;

        v.x += mt.Float(Para.DirFactor.x * Para.Transition);
        v.y += mt.Float(Para.DirFactor.y * Para.Transition);
        v.z += mt.Float(Para.DirFactor.z * Para.Transition);

        pos.Fade(Para.Transition, v, shapeParts[1][i].Pos);
        pinfo.Parts[i].Init(pos,1.0f);
      }
      else if(i<shapeParts[0].GetCount())
      {
        switch(Para.NewVertexPos)
        {
        case 0: // random index
          rIndex = mt.Int(shapeParts[1].GetCount()-1);
          v = shapeParts[1][rIndex].Pos;
          break;

        case 1: // first index
          v = shapeParts[1][0].Pos;
          break;

        case 2: // manual position
          v = Para.BigBangOrigin;
          v.x += mt.Float(Para.BigBangDirFactor.x * Para.Transition);
          v.y += mt.Float(Para.BigBangDirFactor.y * Para.Transition);
          v.z += mt.Float(Para.BigBangDirFactor.z * Para.Transition);
          break;
        }

        pos.Fade(Para.Transition, shapeParts[0][i].Pos, v);
        pinfo.Parts[i].Init(pos,1.0f);
      }
      else if(Para.Transition > 0 && i<shapeParts[1].GetCount() && (shapeParts[0].GetCount() < shapeParts[1].GetCount()) )
      {
        switch(Para.NewVertexPos)
        {
        case 0: // random index
          rIndex = mt.Int(shapeParts[0].GetCount()-1);
          v = shapeParts[0][rIndex].Pos;
          break;

        case 1: // first index
          v = shapeParts[0][0].Pos;
          break;

        case 2: // manual position
          v = Para.BigBangOrigin;
          v.x += mt.Float(Para.BigBangDirFactor.x * Para.Transition);
          v.y += mt.Float(Para.BigBangDirFactor.y * Para.Transition);
          v.z += mt.Float(Para.BigBangDirFactor.z * Para.Transition);
          break;
        }

        pos.Fade(Para.Transition, v, shapeParts[1][i].Pos);
        pinfo.Parts[i].Init(pos,1.0f);
      }
      else
      {
        pinfo.Parts[i].Init(sVector31(0),-1.0f);
      }
    }
    else
    {
      // transition is achieved

      if(i<shapeParts[1].GetCount())
      {
        pinfo.Parts[i].Init(shapeParts[1][i].Pos,1.0f);
      }
      else
      {
        pinfo.Parts[i].Init(sVector31(0),-1.0f);
      }
    }
  }

  pinfo.Used = max;
}
