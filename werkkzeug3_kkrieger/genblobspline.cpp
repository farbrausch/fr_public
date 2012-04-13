// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "genblobspline.hpp"
#include "werkkzeug.hpp"
#include "genoverlay.hpp"

struct InterpolElem
{
  sF32 Time;
  sF32 px,py,pz;
  sQuaternion quat;
  sF32 zoom;
};

/****************************************************************************/
/***                                                                      ***/
/***   new spline code                                                    ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/
/***                                                                      ***/
/***   The Spline Data                                                    ***/
/***                                                                      ***/
/****************************************************************************/

BlobSpline *MakeBlobSpline(sInt *out_bytes,sInt keys)
{
  sInt bytes = sizeof(BlobSpline)+sizeof(BlobSplineKey)*keys;

  BlobSpline *Spline = (BlobSpline *) new sU8[bytes];
  sSetMem(Spline,0,bytes);
  Spline->Count = keys;
  Spline->Version = 3;
  Spline->Select = -1;
  Spline->Mode = 0;
  Spline->Tension = 0;
  Spline->Continuity = 0;
  Spline->Uniform = 0;
  Spline->Target.Init(0,0,0,1);
  sSetMem(Spline->pad,0,sizeof(Spline->pad));

  if(out_bytes)
    *out_bytes = bytes;

  return Spline;
}


BlobSpline *MakeDummyBlobSpline(sInt *out_bytes)
{
  BlobSpline *Spline = MakeBlobSpline(out_bytes,2);
  Spline->Keys[0].Init();
  Spline->Keys[1].Init();
  Spline->Keys[1].Time = 1.0f;

  return Spline;
}

/****************************************************************************/

void BlobSpline::Sort()
{
  for(sInt i=0;i<Count-1;i++)
    for(sInt j=i;j<Count;j++)
      if(Keys[i].Time>Keys[j].Time)
        sSwap(Keys[i],Keys[j]);
}

sF32 BlobSpline::Normalize()
{
  sMatrix mat;
  sF32 zoom;
  sF32 dist;
  sVector v0,v1,delta;
  sF32 *dp;

  dp = new sF32[Count];

  Uniform = 1;

  Sort();
  dist = 0;
  Calc(Keys[0].Time,mat,zoom);
  v1 = mat.l;

  for(sInt i=0;i<Count-1;i++)
  {
    for(sInt j=1;j<=16;j++)
    {
      v0 = v1; 
      Calc(sFade(Keys[i].Time,Keys[i+1].Time,j/16.0f),mat,zoom);
      v1 = mat.l;

      delta.Sub3(v1,v0);
      dist += delta.Abs3();
    }
    dp[i] = dist;
  }

  if(dist==0) dist = 1;
  Keys[0].Time = 0;
  for(sInt i=1;i<Count;i++)
    Keys[i].Time = dp[i-1] / dist;

  delete[] dp;

  return dist;
}



BlobSpline *BlobSpline::Copy()
{
  BlobSpline *ns = (BlobSpline *) new sU8[sizeof(BlobSpline)+sizeof(BlobSplineKey)*Count];

  ns->Count = Count;
  ns->Version = Version;
  ns->CopyPara(this);
  for(sInt i=0;i<Count;i++)
    ns->Keys[i] = Keys[i];

  return ns;
}

void BlobSpline::CopyPara(BlobSpline *from)
{
  Select = from->Select;
  Mode = from->Mode;
  Target = from->Target;
  Continuity = from->Continuity;
  Tension = from->Tension;
  Uniform = from->Uniform;
}


void BlobSpline::SetSelect(sInt key)
{
  Select = key;
  for(sInt i=0;i<Count;i++)
    Keys[i].Select = (i==key);
}

void BlobSpline::SwapTargetCam()
{
  for(sInt i=0;i<Count;i++)
  {
    sSwap(Keys[i].px,Keys[i].rx);
    sSwap(Keys[i].py,Keys[i].ry);
    sSwap(Keys[i].pz,Keys[i].rz);
  }
}

void BlobSpline::SetTarget()
{
  for(sInt i=0;i<Count;i++)
  {
    Keys[i].rx = Target.x;
    Keys[i].ry = Target.y;
    Keys[i].rz = Target.z;
  }
}

/****************************************************************************/

template <typename T> 
static void sFindSplineTime(sF32 time,T *Keys,sInt Count ,sF32 *&p0,sF32 *&p1,sF32 *&p2,sF32 *&p3,sF32 &t)
{
  sInt i;

  if(time<=Keys[0].Time)
  {
    i = 1;
    time = 0;
  }
  else if(time>=Keys[Count-1].Time)
  {
    i = Count-1;
    time = Keys[Count-1].Time;
  }
  else
  {
    for(i=1;i<Count;i++)
      if(Keys[i].Time>time)
        break;
  }
  sVERIFY(i<Count);

  p0 = p3 = 0;
  if(i-2>=0)
    p0 = &Keys[i-2].Time+1;
  p1 = &Keys[i-1].Time+1;
  p2 = &Keys[i  ].Time+1;
  if(i+1<Count)
    p3 = &Keys[i+1].Time+1;

  if(p2[-1]==p1[-1])
    t = p2[-1];
  else
    t = (time - p1[-1]) / (p2[-1] - p1[-1]);
}

void BlobSpline::Calc(sF32 time,sMatrix &mat,sF32 &zoom)
{
  sF32 t;
  sF32 *p0,*p1,*p2,*p3;
  BlobSplineKey k;
  BlobSplineKey kd;
  sVector v;

  sFindSplineTime(time,Keys,Count,p0,p1,p2,p3,t);

  switch(Mode)
  {
  case 3:
    sHermiteD(&k.Time+1,&kd.Time+1,p0,p1,p2,p3,7,t,Tension,Continuity,0,Uniform);
    break;
  case 4:
    {
      sMatrix mat1,mat2;
      mat1.InitEulerPI2(p1+3);
      mat2.InitEulerPI2(p2+3);
      sF32 f1,f2,f3,f4;

      f1 =  2*t*t*t - 3*t*t + 1;
      f2 = -2*t*t*t + 3*t*t;
      f3 = (  t*t*t - 2*t*t + t)*(1-Tension);
      f4 = (  t*t*t -   t*t    )*(1-Tension);

 
      sQuaternion quat,q1,q2;

      q1.Init(p1[6],p1[3],p1[4],p1[5]);
      q2.Init(p2[6],p2[3],p2[4],p2[5]);
      quat.Lin(q1,q2,t);

      q1.ToMatrix(mat1);
      q2.ToMatrix(mat2);
      quat.ToMatrix(mat);

      mat.l.x = p1[0]*f1 + p2[0]*f2 + mat1.k.x*f3 + mat2.k.x*f4;
      mat.l.y = p1[1]*f1 + p2[1]*f2 + mat1.k.y*f3 + mat2.k.y*f4;
      mat.l.z = p1[2]*f1 + p2[2]*f2 + mat1.k.z*f3 + mat2.k.z*f4;

      k.Zoom = 1.0f;
    }
    break;
  default:
    sHermite(&k.Time+1,p0,p1,p2,p3,7,t,Tension,Continuity,0,Uniform);
  }

  switch(Mode)
  {
  case 4:
    break;
  case 0:
    mat.InitEulerPI2(&k.rx);
    mat.l.Init(k.px,k.py,k.pz,1);
    break;
  case 1:
    v.Init(Target.x-k.px,Target.y-k.py,Target.z-k.pz,0);
    mat.InitDir(v);
    mat.l.Init(k.px,k.py,k.pz,1);
    break;
  case 2:
    v.Init(k.rx-k.px,k.ry-k.py,k.rz-k.pz,0);
    mat.InitDir(v);
    mat.l.Init(k.px,k.py,k.pz,1);
    break;
  case 3:
    v.Init(kd.px,kd.py,kd.pz);
    mat.InitDir(v);
    mat.l.Init(k.px,k.py,k.pz);
    break;
  }

  zoom = k.Zoom;
}

#if !sPLAYER
void BlobSpline::CalcKey(sF32 time,BlobSplineKey &k)
{
  sF32 t;
  sF32 *p0,*p1,*p2,*p3;
  sInt i;

  if(time<=Keys[0].Time)
  {
    i = 1;
    time = 0;
  }
  else if(time>=Keys[Count-1].Time)
  {
    i = Count-1;
    time = Keys[Count-1].Time;
  }
  else
  {
    for(i=1;i<Count;i++)
      if(Keys[i].Time>time)
        break;
  }
  sVERIFY(i<Count);

  p0 = p3 = 0;
  if(i-2>=0)
    p0 = &Keys[i-2].Time+1;
  p1 = &Keys[i-1].Time+1;
  p2 = &Keys[i  ].Time+1;
  if(i+1<Count)
    p3 = &Keys[i+1].Time+1;

  t = (time - p1[-1]) / (p2[-1] - p1[-1]);
  sHermite(&k.Time+1,p0,p1,p2,p3,7,t,Tension,Continuity,0,Uniform);
}
#endif

/****************************************************************************/
/***                                                                      ***/
/***   BlobPipe - special blob for creating pipes for splines             ***/
/***                                                                      ***/
/****************************************************************************/

BlobPipe *MakeBlobPipe(sInt *out_bytes,sInt keys)
{
  sInt bytes = sizeof(BlobPipe)+sizeof(BlobPipeKey)*keys;

  BlobPipe *pipe = (BlobPipe *) new sU8[bytes];
  sSetMem(pipe,0,bytes);
  pipe->Count = keys;
  pipe->Version = 1;
  pipe->Mode = 0;

  if(out_bytes)
    *out_bytes = bytes;

  return pipe;
}

BlobPipe *MakeDummyBlobPipe(sInt *out_bytes)
{
  BlobPipe *pipe;

  pipe = MakeBlobPipe(out_bytes,2);
  pipe->StartX = 0;
  pipe->StartY = 0;
  pipe->StartZ = 0;
  pipe->Keys[0].PosX = 0;
  pipe->Keys[0].PosY = 0;
  pipe->Keys[0].PosZ = 1;
  pipe->Keys[0].Radius = 0.125f;
  pipe->Keys[0].Flags = 0;
  pipe->Keys[1].PosX = 0;
  pipe->Keys[1].PosY = 1;
  pipe->Keys[1].PosZ = 1;
  pipe->Keys[1].Radius = 0.125f;
  pipe->Keys[1].Flags = 2;

  return pipe;
}

void BlobPipe::CopyPara(BlobPipe *from)
{
  StartX = from->StartX;
  StartY = from->StartY;
  StartZ = from->StartZ;
  Tension = from->Tension;
}

BlobPipe *BlobPipe::Copy()
{
  sInt bytes = sizeof(BlobPipe)+sizeof(BlobPipeKey)*Count;

  BlobPipe *pipe = (BlobPipe *) new sU8[bytes];
  sCopyMem(pipe,this,bytes);

  return pipe;
}

/****************************************************************************/
/***                                                                      ***/
/***   GenSpline Object                                                   ***/
/***                                                                      ***/
/****************************************************************************/

GenSpline::GenSpline()
{
  ClassId = KC_SPLINE;
}

/****************************************************************************/

GenSplineSpline::GenSplineSpline()
{
  Spline = 0;
  Pipe = 0;
}

GenSplineSpline::~GenSplineSpline()
{
  if(Spline) delete[] (sU8 *) Spline;
  if(Pipe) delete[] (sU8 *) Pipe;
}

void GenSplineSpline::Eval(sF32 time,sF32 phase,sMatrix &mat,sF32 &zoom)
{
  Spline->Calc(time,mat,zoom);
}

void GenSplineSpline::Init(BlobSpline *data)
{
  sDelete(Spline);
  if(data)
    Spline = data->Copy();
}

/****************************************************************************/


GenSplineShaker::GenSplineShaker()
{
  Parent = 0;
  TranslateAmp.Init();
  RotateAmp.Init();
  TranslateFreq.Init();
  RotateFreq.Init();
  Center.Init();
  OpLink = 0;
}

GenSplineShaker::~GenSplineShaker()
{
  sRelease(Parent);
}

void GenSplineShaker::Eval(sF32 time2,sF32 phase,sMatrix &mat,sF32 &zoom)
{
  sMatrix mat1,mat0;
  sF32 tx,ty,tz,rx,ry,rz;
  sF32 time,ramp;

  if(Parent)
  {
    Parent->Eval(time2,phase,mat,zoom);
  }
  else
  {
    mat.Init();
    zoom = 1;
  }

  KEvent *event = OpLink->FirstEvent;
  while(event)
  {
    time = 1.0f*(event->End-GenOverlayManager->KEnv->BeatTime) / (event->End-event->Start);
    if(time>0 && time<1)
    {
      switch(Mode&15)
      {
      default:
      case 0:
        ramp = 1-time;
        break;
      case 1:
        ramp = time;
        time = 1-time;
        break;
      case 2:
        ramp = sFSin(time*sPIF);
        break;
      }

      if(Mode & 32)
      {
        sVector v;
        v = TranslateFreq;
        v.Scale3(time);
        sPerlin3D(v,v);
        tx = v.x;
        ty = v.y;
        tz = v.z;
        v = RotateFreq;
        v.Scale3(time);
        sPerlin3D(v,v);
        rx = v.x;
        ry = v.y;
        rz = v.z;
      }
      else
      {
        tx = sFSin(time*TranslateFreq.x*sPI2F)*TranslateAmp.x*ramp;
        ty = sFSin(time*TranslateFreq.y*sPI2F)*TranslateAmp.y*ramp;
        tz = sFSin(time*TranslateFreq.z*sPI2F)*TranslateAmp.z*ramp;
        rx = sFSin(time*RotateFreq.x*sPI2F)*RotateAmp.x*ramp;
        ry = sFSin(time*RotateFreq.y*sPI2F)*RotateAmp.y*ramp;
        rz = sFSin(time*RotateFreq.z*sPI2F)*RotateAmp.z*ramp;
      }

      tx = tx*TranslateAmp.x*ramp;
      ty = ty*TranslateAmp.y*ramp;
      tz = tz*TranslateAmp.y*ramp;
      rx = rx*RotateAmp.x*ramp;
      ry = ry*RotateAmp.y*ramp;
      rz = rz*RotateAmp.z*ramp;

      mat1.InitEuler(rx,ry,rz);
      mat1.l.Init(tx,ty,tz,1);

      if(Mode&16)
      {
        mat.l.x -= Center.x;
        mat.l.y -= Center.y;
        mat.l.z -= Center.z;
        mat0 = mat;
        mat.MulA(mat0,mat1);
        mat.l.x += Center.x;
        mat.l.y += Center.y;
        mat.l.z += Center.z;
      }
      else
      {
        mat0 = mat;
        mat.MulA(mat1,mat0);
      }
    }
    event = event->NextOp;
  }
}

/****************************************************************************/

GenSplineMeta::GenSplineMeta()
{
  sVERIFY(sCOUNTOF(Times)==sCOUNTOF(Splines));

  for(sInt i=0;i<sCOUNTOF(Splines);i++)
  {
    Times[i] = 0;
    Splines[i] = 0;
  }
}

GenSplineMeta::~GenSplineMeta()
{
  for(sInt i=0;i<sCOUNTOF(Splines);i++)
    sRelease(Splines[i]);
}

void GenSplineMeta::Sort()
{
  Count = 0;
  for(sInt i=0;i<sCOUNTOF(Splines);i++)
  {
    Times[i] = sRange<sF32>(Times[i],1,0);
    if(Splines[i]==0)
      Times[i] = 100;
    else
      Count++;
  }

  for(sInt i=0;i<sCOUNTOF(Splines)-1;i++)
  {
    for(sInt j=i+1;j<sCOUNTOF(Splines);j++)
    {
      if(Times[i]>Times[j])
      {
        sSwap(Times[i],Times[j]);
        sSwap(Splines[i],Splines[j]);
      }
    }
  }
}

void GenSplineMeta::Eval(sF32 time,sF32 phase,sMatrix &mat,sF32 &zoom)
{
  InterpolElem Meta[sCOUNTOF(Splines)];
  InterpolElem result;

  sF32 *p0,*p1,*p2,*p3;
  sF32 t;
  sMatrix mat1;
  sF32 zoom1;

  for(sInt i=0;i<Count;i++)
  {
    sVERIFY(Splines[i]);
    Splines[i]->Eval(time,phase,mat1,zoom1);

    Meta[i].Time = Times[i];
    Meta[i].px = mat1.l.x;
    Meta[i].py = mat1.l.y;
    Meta[i].pz = mat1.l.z;
    Meta[i].quat.FromMatrix(mat1);
    Meta[i].zoom = zoom1;
  }

  sFindSplineTime(phase,Meta,Count,p0,p1,p2,p3,t);
  sHermite(&result.px,p0,p1,p2,p3,8,t,0,0,0,0);

  result.quat.ToMatrix(mat);
  mat.l.Init(result.px,result.py,result.pz);
  zoom = result.zoom;
}

/****************************************************************************/
/***                                                                      ***/
/***   operators                                                          ***/
/***                                                                      ***/
/****************************************************************************/

KObject * __stdcall Init_Misc_Spline(KOp *op)
{
  GenSplineSpline *gs = new GenSplineSpline;
  gs->Init((BlobSpline *)op->GetBlobData());

  if(gs->Spline->Version<3)
  {
    gs->Spline->Uniform = 0;
    gs->Spline->Tension = 0;
    gs->Spline->Continuity = 0;
  }
  gs->Spline->Version = 3;
  return gs;
}

/****************************************************************************/

KObject * __stdcall Init_Misc_Shaker(KOp *op,GenSpline *parent,sInt mode,sF323 ta,sF323 ra,sF323 tf,sF323 rf,sF32 time,sF323 center)
{
  GenSplineShaker *gs = new GenSplineShaker;

  gs->Mode = mode;
  gs->TranslateAmp.x = ta.x;
  gs->TranslateAmp.y = ta.y;
  gs->TranslateAmp.z = ta.z;
  gs->RotateAmp.x = ra.x;
  gs->RotateAmp.y = ra.y;
  gs->RotateAmp.z = ra.z;
  gs->TranslateFreq.x = tf.x;
  gs->TranslateFreq.y = tf.y;
  gs->TranslateFreq.z = tf.z;
  gs->RotateFreq.x = rf.x;
  gs->RotateFreq.y = rf.y;
  gs->RotateFreq.z = rf.z;
  gs->Center.x = center.x;
  gs->Center.y = center.y;
  gs->Center.z = center.z;
  gs->Parent = parent;
  gs->OpLink = op;   

  return gs;
}

/****************************************************************************/

KObject * __stdcall Init_Misc_PipeSpline(KOp *op)
{
  sQuaternion quat;
  sVector v,d,p,ax;
  sF32 oldrad;
  sMatrix mat,mat0,mat1;

  BlobPipe *pipe = (BlobPipe *) op->GetBlobData();
  if(!pipe)
  {
    sInt size;
    pipe = MakeDummyBlobPipe(&size);
    op->SetBlob((sU8 *)pipe,size);
  }
  sVERIFY(pipe)

  GenSplineSpline *gs = new GenSplineSpline;
  gs->Spline = MakeBlobSpline(0,pipe->Count*2);
  gs->Spline->Mode = 4;
  gs->Pipe = pipe->Copy();

  mat.Init();
  oldrad = 0;
  v.Init(pipe->StartX,pipe->StartY,pipe->StartZ);

  for(sInt i=0;i<pipe->Count;i++)
  {
    // get point

    p = v;                                  // p = old point
    v.x = pipe->Keys[i].PosX;               // v = new point
    v.y = pipe->Keys[i].PosY;
    v.z = pipe->Keys[i].PosZ;
    d.x = v.x - p.x;                        // d = direction vector
    d.y = v.y - p.y;
    d.z = v.z - p.z;
    d.UnitSafe3();

    // calculate old point position

    sF32 angle = sFACos(mat.k.Dot3(d));
    ax.Cross3(mat.k,d);
    mat1.InitRot(ax,angle);

    mat0 = mat;
    mat.Mul3(mat0,mat1);
    quat.FromMatrix(mat);
    quat.ToMatrix(mat1);

    // old point

    p.AddScale3(d,oldrad);
    gs->Spline->Keys[i*2+0].Time = i+0.0f;
    gs->Spline->Keys[i*2+0].px = p.x;
    gs->Spline->Keys[i*2+0].py = p.y;
    gs->Spline->Keys[i*2+0].pz = p.z;
    gs->Spline->Keys[i*2+0].rx = quat.x;
    gs->Spline->Keys[i*2+0].ry = quat.y;
    gs->Spline->Keys[i*2+0].rz = quat.z;
    gs->Spline->Keys[i*2+0].Zoom = quat.w;

    // new point

    oldrad = pipe->Keys[i].Radius;
    p = v;
    p.AddScale3(d,-oldrad);
    gs->Spline->Keys[i*2+1].Time = i+0.5f;
    gs->Spline->Keys[i*2+1].px = p.x;
    gs->Spline->Keys[i*2+1].py = p.y;
    gs->Spline->Keys[i*2+1].pz = p.z;
    gs->Spline->Keys[i*2+1].rx = quat.x;
    gs->Spline->Keys[i*2+1].ry = quat.y;
    gs->Spline->Keys[i*2+1].rz = quat.z;
    gs->Spline->Keys[i*2+1].Zoom = quat.w;
  }

  gs->Spline->Tension = pipe->Tension;
  gs->Spline->Normalize();

  return gs;
}

/****************************************************************************/

KObject * __stdcall Init_Misc_MetaSpline(Init_Misc_MetaSplinePara para)
{
  GenSplineMeta *gs = new GenSplineMeta;

  for(sInt i=0;i<8;i++)
  {
    gs->Splines[i] = para.sp[i];
    gs->Times[i] = para.time[i];
  }
  gs->Sort();

  return gs;
}

/****************************************************************************/
