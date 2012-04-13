// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_start.hpp"
#include "genscene.hpp"
#include "genmesh.hpp"
#include "genminmesh.hpp"
#include "genoverlay.hpp"
#include "genmaterial.hpp"
#include "geneffect.hpp"
#include "genblobspline.hpp"
#include "kkriegergame.hpp"
#include "engine.hpp"

#if !sPLAYER
#include "winview.hpp"
#endif

#if sPROFILE
#include "_util.hpp"

sMAKEZONE(ExecWalk    ,"ExecWalk "  ,0xff8000d0);
sMAKEZONE(PaintScene  ,"PaintScene" ,0xffb040ff);
sMAKEZONE(ExecXForm   ,"ExecXForm"  ,0xff593d33);
#endif

/****************************************************************************/

#if !sPLAYER
KOp *ShowPortalOp;
sVector ShowPortalCube[8];
sBool ShowPortalOpProcessed;

sBool SceneWireframe = sFALSE;
sInt SceneWireFlags = 0;
sU32 SceneWireMask = 0;
#endif

sInt GenScenePasses = ~0;
static sInt RenderPassAdjust = 0;

/****************************************************************************/

#define SCRIPTVERIFY(x) {if(!(x)) return 0;}

/****************************************************************************/

GenScene::GenScene()
{
  ClassId = KC_SCENE;
  Childs.Init();
  DrawMesh = 0;
#if sLINK_KKRIEGER
  CollMesh = 0;
#endif
  Effect = 0;
  sSetMem(SRT,0,sizeof(SRT));
  SRT[0] = 1;
  SRT[1] = 1;
  SRT[2] = 1;
  Count = 0;
  Next = 0;
  IsSector = sFALSE;
}

GenScene::~GenScene()
{
  sInt i;

  for(i=0;i<Childs.Count;i++)
    Childs[i]->Release();
  Childs.Exit();

  if(DrawMesh) DrawMesh->Release();
#if sLINK_KKRIEGER
  if(CollMesh) CollMesh->Release();
#endif
  if(Effect) Effect->Release();
}

void GenScene::Copy(KObject *o)
{
  sInt i;
  GenScene *scene;

  sVERIFY(o->ClassId==KC_SCENE);
  scene = (GenScene *) o;

  Childs.Copy(scene->Childs);
  for(i=0;i<Childs.Count;i++)
    Childs[i]->AddRef();

  DrawMesh = scene->DrawMesh;
  if(DrawMesh) DrawMesh->AddRef();

#if sLINK_KKRIEGER
  CollMesh = scene->CollMesh;
  if(CollMesh) CollMesh->AddRef();
#endif

  Effect = scene->Effect;
  if(Effect) Effect->AddRef();

  sCopyMem(SRT,scene->SRT,sizeof(SRT));
  Count = scene->Count;
  IsSector = scene->IsSector;
  Next = 0;
}

/****************************************************************************/
/****************************************************************************/

GenScene *MakeScene(KObject *in)
{
  GenScene *scene;
  EngMesh *mesh;

  scene = 0;
  if(in)
  {
    if(in->ClassId==KC_SCENE)
    {
      scene = (GenScene *)in;
    }
    else if(in->ClassId==KC_MESH)
    {
      GenMesh *inMesh;
      scene = new GenScene;
      inMesh = (GenMesh *)in;
      if(inMesh)
      {
        inMesh->Compact();

#if !sPLAYER
        if(SceneWireframe)
        {
          inMesh->PrepareWire(SceneWireFlags,SceneWireMask);
          mesh = inMesh->WireMesh;
        }
        else
        {
          inMesh->UnPrepareWire();
          inMesh->Prepare();
          mesh = inMesh->PreparedMesh;
        }
#else
        inMesh->Prepare();
        mesh = inMesh->PreparedMesh;
#endif

        if(mesh)
        {
          scene->DrawMesh = mesh;
          scene->DrawMesh->AddRef();

#if sPLAYER
//          mesh->Preload();
#endif
        }

#if sLINK_KKRIEGER
        scene->CollMesh = new KKriegerMesh(inMesh);
#endif
        inMesh->Release();
      }
    }
    else if(in->ClassId==KC_EFFECT)
    {
      scene = new GenScene;
      scene->Effect = (GenEffect *)in;
    }
#if sLINK_MINMESH
    else if(in->ClassId==KC_MINMESH)
    {
      GenMinMesh *minMesh;
      scene = new GenScene;
      minMesh = (GenMinMesh *)in;
      if(minMesh)
      {

#if !sPLAYER
        if(SceneWireframe)
        {
          minMesh->PrepareWire(SceneWireFlags,SceneWireMask);
          mesh = minMesh->WireMesh;
        }
        else
        {
          minMesh->UnPrepareWire();
          minMesh->Prepare();
          mesh = minMesh->PreparedMesh;
        }
#else
        minMesh->Prepare();
        mesh = minMesh->PreparedMesh;
#endif

        if(mesh)
        {
          scene->DrawMesh = mesh;
          scene->DrawMesh->AddRef();

#if sPLAYER
//          mesh->Preload();
#endif
        }

        minMesh->Release();
      }
    }
#endif
  }

  return scene;
}

/****************************************************************************/
/***                                                                      ***/
/***   The Operators                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void ExecSceneInput(KOp *parent,KEnvironment *kenv,sInt i)
{
  KOp *op;
  GenScene *scene;
  EngMesh *drawMesh;
  sInt classId;

  op = parent->GetInput(i);

  classId = KC_NULL;
#if sPLAYER
  if(op->CacheFreed)
    classId = op->Result;
  else
#endif
  if(op->Cache)
    classId = op->Cache->ClassId;

  if(classId == KC_MESH || classId == KC_MINMESH || classId == KC_EFFECT)
  {
    scene = (GenScene *)parent->Cache;
    if(scene->Childs.Count)
    {
      sVERIFY(i < scene->Childs.Count);
      drawMesh = scene->Childs[i]->DrawMesh;
    }
    else
      drawMesh = scene->DrawMesh;

    if(classId == KC_MESH || classId==KC_MINMESH)
    {
#if sLINK_KKRIEGER
      // this monster culling HACK should be removed.
      if(kenv->CurrentEvent && kenv->CurrentEvent->CullDist>0)
      {
        sVector v;
        v.Sub3(kenv->ExecStack.Top().l,kenv->GameCam.CameraSpace.l);
        if(kenv->CurrentEvent->CullDist*kenv->CurrentEvent->CullDist<v.Dot3(v))
          return;
      }
#endif
      Engine->AddPaintJob(drawMesh,kenv->ExecStack.Top(),kenv->Var[KV_TIME].x,RenderPassAdjust);
    }
    else
      Engine->AddPaintJob(op,kenv->ExecStack.Top(),kenv,RenderPassAdjust);
  }
  else
  {
    op->Exec(kenv);
  }
}

/****************************************************************************/

void ExecSceneInputs(KOp *parent,KEnvironment *kenv,sF32 *srt)
{
  sInt i,max;
  GenScene *scene;
  sMatrix mat;

  scene = (GenScene *) parent->Cache;

  if(srt)
  {
    mat.InitSRT(srt);
    kenv->ExecStack.PushMul(mat);
  }
  
  max = parent->GetInputCount();
  for(i=0;i<max;i++)
    ExecSceneInput(parent,kenv,i);

  if(srt)
    kenv->ExecStack.Pop();
}

/****************************************************************************/

GenScene * __stdcall Init_Scene_Scene(GenMesh *mesh,sF323 s,sF323 r,sF323 t,sBool lightmap)
{
  GenScene *scene;

  if(mesh==0)
    return new GenScene;

  scene = MakeScene(mesh);
  if(scene)
    sCopyMem(scene->SRT,&s.x,9*4);

  return scene;
}

void __stdcall Exec_Scene_Scene(KOp *op,KEnvironment *kenv,sF323 s,sF323 r,sF323 t,sBool lightmap)
{
  ExecSceneInputs(op,kenv,&s.x);
}

/****************************************************************************/

GenScene * __stdcall Init_Scene_Add(sInt count,GenScene *s0,...)
{
  GenScene *scene,*add;
  sInt i;

  scene = new GenScene;
  for(i=0;i<count;i++)
  {
    add = MakeScene((&s0)[i]);
    if(add)
      *scene->Childs.Add() = add;
  }

  return scene;
}

void __stdcall Exec_Scene_Add(KOp *op,KEnvironment *kenv)
{
  ExecSceneInputs(op,kenv,0);
}

/****************************************************************************/

GenScene * __stdcall Init_Scene_Multiply(GenScene *add,sF323 s,sF323 r,sF323 t,sInt count)
{
  GenScene *scene;

  add = MakeScene(add);
  if(!add) return 0;

  scene = new GenScene;
  *scene->Childs.Add() = add;
  scene->Count = count;
  sCopyMem(scene->SRT,&s.x,9*4);

  return scene;
}

void __stdcall Exec_Scene_Multiply(KOp *op,KEnvironment *kenv,sF323 s,sF323 r,sF323 t,sInt count)
{
  sMatrix mat,mat2;
  sInt i;
  sVector save;

  kenv->ExecStack.Dup();
  mat.InitSRT(&s.x);
  save = kenv->Var[KV_SELECT];
  for(i=0;i<count;i++)
  {
    kenv->Var[KV_SELECT].Init(i,i,i,i);
    ExecSceneInputs(op,kenv,0);
    mat2.MulA(mat,kenv->ExecStack.Top());
    kenv->ExecStack.Pop();
    kenv->ExecStack.Push(mat2);
  }
  kenv->ExecStack.Pop();
  kenv->Var[KV_SELECT] = save;
}

/****************************************************************************/

GenScene * __stdcall Init_Scene_Transform(GenScene *add,sF323 s,sF323 r,sF323 t)
{
  return Init_Scene_Multiply(add,s,r,t,0);
}

void __stdcall Exec_Scene_Transform(KOp *op,KEnvironment *kenv,sF323 s,sF323 r,sF323 t)
{
  ExecSceneInputs(op,kenv,&s.x);
}

/****************************************************************************/

GenScene * __stdcall Init_Scene_Light(sF323 r,sF323 t,sU32 flags,sF32 zoom,sU32 color,sF32 amplify,sF32 range)
{
  return new GenScene;
}

void __stdcall Exec_Scene_Light(KOp *op,KEnvironment *kenv,sF323 r,sF323 t,sU32 flags,sF32 zoom,sU32 color,sF32 amplify,sF32 range)
{
  //KSceneJob *job;
  sF32 srt[9];
  sMatrix mat;//,mat2;

//  sVERIFY(op->GetInputCount()==0);

  srt[0] = 1;
  srt[1] = 1;
  srt[2] = 1;
  srt[3] = r.x;
  srt[4] = r.y;
  srt[5] = r.z;
  srt[6] = t.x;
  srt[7] = t.y;
  srt[8] = t.z;
  mat.InitSRT(srt);
  kenv->ExecStack.PushMul(mat);
  //mat2.Mul4(mat,kenv->ExecMatrix);

#if !sPLAYER
  if(ShowPortalOp == op)
  {
    ShowPortalCube[0] = kenv->ExecStack.Top().l;
    ShowPortalOpProcessed = sTRUE;
  }
#endif

  EngLight light;
  light.Position = kenv->ExecStack.Top().l;
  light.Flags = flags;
  // ignore zoom
  light.Color = color;
  light.Amplify = amplify;
  light.Range = range;
  light.Event = kenv->CurrentEvent;
  light.Id = (sInt) op;

  Engine->AddLightJob(light);

  kenv->ExecStack.Pop();
}


/****************************************************************************/

GenScene * __stdcall Init_Scene_Camera(sF323 s,sF323 r,sF323 t)
{
  GenScene *scene;

  scene = new GenScene;
  sCopyMem(scene->SRT,&s.x,9*4);

  return scene;
}

void __stdcall Exec_Scene_Camera(KOp *op,KEnvironment *kenv,sF323 s,sF323 r,sF323 t)
{
  sMatrix mat;
  
#if !sPLAYER
  if(!GenOverlayManager->LinkEdit)
  {
    mat.InitSRT(&s.x);
    kenv->GameCam.CameraSpace.MulA(mat,kenv->ExecStack.Top());
  }
#else
  {
    mat.InitSRT(&s.x);
    kenv->GameCam.CameraSpace.MulA(mat,kenv->ExecStack.Top());
  }
#endif
}

/****************************************************************************/

GenScene * __stdcall Init_Scene_Limb(GenScene *scene0,GenScene *scene1,sF323 pos,sF323 dir,sF32 l0,sF32 l1,sU32 flags)
{
  GenScene *scene;
  GenScene *add;

  scene = new GenScene();

  add =  MakeScene(scene0);
  if(add) *scene->Childs.Add() = add;
  add =  MakeScene(scene1);
  if(add) *scene->Childs.Add() = add;

  return scene;
}

void __stdcall Exec_Scene_Limb(KOp *op,KEnvironment *kenv,sF323 pos,sF323 dirv,sF32 a,sF32 b,sU32 flags)
{
  sMatrix mat0,mat1,var;
  sF32 c,s,t;
  sVector i,j,k;
  
//  flags &= ~2; // relative mode ist kaputt
  if(op->GetInputCount()==2)
  {
    const sMatrix &matrix = kenv->ExecStack.Top();
    if(!(flags&2))
    {
      pos.x -= matrix.l.x;
      pos.y -= matrix.l.y;
      pos.z -= matrix.l.z;
    }

    j.Init(-pos.x,-pos.y,-pos.z,0);    // t-direction
    i.Init(dirv.x,dirv.y,dirv.z,0);
    if(!(flags&2))
      i.Rotate3(matrix);
    k.Cross4(i,j);                  // cross vector
    i.Cross4(j,k);                  // s-direction
    c = j.Abs3();                   // length
    i.Unit3();
    j.Unit3();
    k.Unit3();

    t = (c*c+a*a-b*b)/(2*c);       // b²=c²+a²-2ca cos(ß);  t=a cos(ß);
    /*if(a<=t)
      s=0;
    else
      s = sFSqrt(a*a-t*t);*/
    s = (a-t) * (a+t);
    if(s <= 0.0f)
      s = 0.0f;
    else
      s = sFSqrt(s);

    mat0.k = k;
    mat0.j.Scale4(j,t);
    mat0.j.AddScale3(i,s);
    mat0.j.Unit3();
    mat0.i.Cross4(mat0.j,mat0.k);
    mat0.l.Init4(0,0,0,1);


    if(a<=t && !(flags&1))
    {
      mat1 = mat0;
      mat1.l.AddScale3(j,-a-b);
    }
    else
    {
      mat1.k = k;
      mat1.j.Scale4(j,c-t);
      mat1.j.AddScale3(i,-s);
      mat1.j.Unit3();
      mat1.i.Cross4(mat1.j,mat1.k);
      mat1.l.Init4(pos.x,pos.y,pos.z,1);
    }

    if(flags&2)
      kenv->ExecStack.PushMul(mat0);
    else
    {
      kenv->ExecStack.Push(mat0);
      kenv->ExecStack.Top().l.Add3(matrix.l);
    }
    kenv->SetMatrix(mat1,var);
    ExecSceneInput(op,kenv,0);
    //op->ExecInput(kenv,0);
    kenv->RestoreMatrix(var);
    kenv->ExecStack.Pop();

    if(flags&2)
      kenv->ExecStack.PushMul(mat1);
    else
    {
      kenv->ExecStack.Push(mat1);
      kenv->ExecStack.Top().l.Add3(matrix.l);
    }
    kenv->SetMatrix(mat0,var);
    ExecSceneInput(op,kenv,1);
    //op->ExecInput(kenv,1);
    kenv->RestoreMatrix(var);
    kenv->ExecStack.Pop();
  }
}


/****************************************************************************/

GenScene * __stdcall Init_Scene_Walk(GenScene *scene0,
  sU32 Flags,sInt FootCount,sF32 StepTresh,sF32 RunLowTresh,sF32 RunHighTresh,
  sInt2 ft,sF322 sl,sF322 ss,sF322 sn,
  sF323 l0,sF323 l1,sF323 l2,sF323 l3,sF323 l4,sF323 l5,sF323 l6,sF323 l7,
  sF32 scanup,sF32 scandown,
  KSpline *stepspline)
{
  GenScene *scene;
  GenScene *add;

  sREGZONE(ExecWalk);

  scene = new GenScene();

  add =  MakeScene(scene0);
  if(add) *scene->Childs.Add() = add;

  return scene;
}

#pragma lekktor(off)
struct WalkMem : public KInstanceMem
{
//  KKriegerParticle FootPart[8];
//  KKriegerParticle BodyPart;
  sVector FootOld[8];
  sVector FootNew[8];
  sInt StepTime[8];
  sInt LastTime;
  sVector OldPos,NewPos,FootCenter;
  sInt State;                   // state 0=walk, 1=run
  sInt LastLeg;                 // 0 = left; 1 = right
  sInt NextLeg;                 // (LastLeg+1)%FootGroup
  sInt Idle;                    // no new steps issued last frame
  sVector Deviation;            // avoid walls
  sInt DeviationFlag;
  sInt InitSteps;
  sVector FootDelta[8];         // position of the foot-contact point relative to body
} *mem;

void __stdcall Exec_Scene_Walk(KOp *op,KEnvironment *kenv,
  sU32 Flags,sInt FootCount,sF32 StepTresh,sF32 RunLowTresh,sF32 RunHighTresh,
  sInt2 fg,sF322 sl,sF322 ss,sF322 sn,
  sF323 l0,sF323 l1,sF323 l2,sF323 l3,sF323 l4,sF323 l5,sF323 l6,sF323 l7,
  sF32 scanup,sF32 scandown,
  KSpline *stepspline)
{
  sMatrix mat;//,save;               // local vars
  sMatrix dir;                    // desired orientation of walker
  sVector savevar[8],savetime;
  sVector v,sum,foot[8],spline[8],stepdir;
//  sVector plane;
  sInt i,j;
  sInt bestleg;
  sInt time;
  sF32 dist,f,t;
  sInt changestate;
//  KKriegerCellAdd *cell;

  sZONE(ExecWalk);

  // copy of instance-vars

  sInt State;
  sInt LastLeg;
  sInt NextLeg;

  // parameters

  sInt FootGroup[2];              // groups of feet.
  sF32 StepLength[2];             // max. length of a step
  sInt StepSpeed[2];              // duration of a step in ms.
  sInt StepNext[2];               // when to start the next step in ms. steps may overlap

  // init parameters

  FootGroup[0] = fg.x;
  FootGroup[1] = fg.y;
  StepLength[0] = sl.x;
  StepLength[1] = sl.y;
  StepSpeed[0] = sFtol(ss.x*1000);
  StepSpeed[1] = sFtol(ss.y*1000);
  StepNext[0] = sMin(sFtol(sn.x*1000),StepSpeed[0]);
  StepNext[1] = sMin(sFtol(sn.y*1000),StepSpeed[1]);

// init instance storage
  sMatrix &matrix = kenv->ExecStack.Top();

  mem = kenv->GetInst<WalkMem>(op);
  if(mem->Reset || (Flags&0x100) || kenv->TimeReset)
  {
//    cell = 0;
//    if(Flags&2)
//      cell = kenv->Game->FindCell(matrix.l);
    sum.Init();
    mem->FootCenter.Init();
    mem->InitSteps = FootCount*2;
    for(i=0;i<FootCount;i++)
    {
      mem->FootDelta[i].Init((&l0)[i].x,(&l0)[i].y,(&l0)[i].z,0);
      v.Add3(matrix.l,mem->FootDelta[i]);
      v.w = 0;
      mem->FootOld[i] = mem->FootNew[i] = v;
      mem->StepTime[i] = StepSpeed[0];
//      mem->FootPart[i].Init(v,sum,cell);
      mem->FootCenter.x += mem->FootDelta[i].x/FootCount;
      mem->FootCenter.z += mem->FootDelta[i].z/FootCount;
    }
//    mem->FootPart[FootCount-1].EndMarker = 1;
//    mem->BodyPart.Init(matrix.l,sum,cell);
//    mem->BodyPart.EndMarker = 1;
    mem->LastTime = kenv->CurrentTime;
    mem->NewPos = matrix.l;
    mem->State = 0;
    mem->LastLeg = 0;
    for(i=0;i<FootCount;i++)
      mem->FootDelta[i].Sub3(mem->FootCenter);
  }

  State = mem->State;
  LastLeg = mem->LastLeg;
  NextLeg = mem->NextLeg;

  // fake for animation

#if !sINTRO
  if(Flags & 0x30)
  {
    sInt k;
    State = ((Flags&0x30)==0x20);
    j = FootGroup[State];
    f = (StepTresh+RunLowTresh)/2;
    if(j)
      f = (RunLowTresh+RunHighTresh)/2;
    f = f/j;
    if(Flags&0x40)
      f *= 2;

    time = kenv->CurrentTime % (StepNext[State]*j);
    k = kenv->CurrentTime / (StepNext[State]*j);
    for(i=0;i<FootCount;i++)
    {
      mem->StepTime[i] = time - (StepNext[State]*(i%j));
      if(Flags&0x80)
        mem->FootOld[i].Init(0,0,f*(i%j-(time*1.0f/StepNext[State])-(j-1)*0.5f));
      else
        mem->FootOld[i].Init(0,0,f*(k*j+(i%j)));
      mem->FootOld[i].Add3(mem->FootDelta[i]);
      mem->FootNew[i] = mem->FootOld[i];
      mem->FootNew[i].z += f*j;
      if(mem->StepTime[i]<StepSpeed[State]-StepNext[State]*j)
      {
        mem->StepTime[i] += StepNext[State]*j;
        mem->FootOld[i].z -= f*j;
        mem->FootNew[i].z -= f*j;
      }
      if(mem->StepTime[i]<0)
      {
        mem->FootNew[i] = mem->FootOld[i];
        mem->StepTime[i] = StepSpeed[State];
      }
      if(mem->StepTime[i]>StepSpeed[State])
        mem->StepTime[i] = StepSpeed[State];
    }
  }
#endif

  // animate

  sum.Init();
  f = 0;
  for(i=0;i<FootCount;i++)          
  {
    t = sRange(1.0f*mem->StepTime[i]/StepSpeed[State],1.0f,0.0f);
    if(i<FootGroup[State])
    {
      f += t;
    }
#pragma lekktor(on)
    if(stepspline)
    {
      stepspline->Eval((t+State)*0.5f,spline[i]);
      if(mem->FootDelta[i].x<0) spline[i].x=-spline[i].x;
      if(mem->FootDelta[i].z<0) spline[i].z=-spline[i].z;
      spline[i].w = spline[i].w-State;
    }
    else
    {
      spline[i].x = 0;
      spline[i].y = sFSin(t*sPI)*0.25f; 
      spline[i].z = 0;
      spline[i].w = t;
    }
#pragma lekktor(off)

    foot[i].Lin3(mem->FootOld[i],mem->FootNew[i],spline[i].w);
    foot[i].w = 1.0f;
    /*
    if(Flags&0x02)
    {
      foot[i].y += spline[i].y;
      mem->FootPart[i].Control(foot[i]);
      foot[i].y -= spline[i].y;
    }
    */
    sum.Add3(foot[i]);
  }
  sum.Scale3(1.0f/FootCount);
  mat.i.Init(0,0,0,0);
  mat.j.Init(0,0,0,0); // ryg 040820
  mat.k.Init(0,0,0,0);
  mat.l.Init(sum.x,sum.y,sum.z,1);
  for(i=0;i<FootCount;i++)
  {
    v.Sub3(foot[i],sum);
    mat.i.AddScale3(v,mem->FootDelta[i].x);
    mat.j.AddScale3(v,mem->FootDelta[i].y);
    mat.k.AddScale3(v,mem->FootDelta[i].z);
  }
#pragma lekktor(on)
  switch((Flags>>2)&3)
  {
  case 0:   // straight, copy from input
    mat.i = matrix.i; 
    mat.j = matrix.j; 
    mat.k = matrix.k; 
    break;
  case 2:   // use xy
    mat.i.Unit3();
    mat.j.Init(0,1,0,0);
    mat.k.Cross4(mat.i,mat.j);
    mat.k.Unit3();
    mat.j.Cross4(mat.k,mat.i);
    mat.j.Unit3();
    break;
  case 1:   // use y
    mat.k = matrix.k;
  case 3:   // use xz
    mat.k.Unit3();
    mat.j.Cross4(mat.k,mat.i);
    mat.j.Unit3();
    mat.i.Cross4(mat.j,mat.k);
    mat.i.Unit3();
    break;
  }
#pragma lekktor(off)

  t = (f+LastLeg)/FootGroup[State];
  if(t>=2.0f) t-=2.0f;
  if(t>=1.0f) t-=1.0f;
  t = (t+State)*0.5f;
  savetime = kenv->Var[KV_LEG_TIMES];
  kenv->Var[KV_LEG_TIMES].Init(t,0,0,0);

  // advance

  time = kenv->CurrentTime-mem->LastTime;
  mem->LastTime = kenv->CurrentTime;
  if(time<0)
    time = 0;
  dir = matrix;
  if(mem->DeviationFlag>0)
  {
    dir.l.Add3(mat.l,mem->Deviation);
  }
/*
  if(mem->BodyPart.Cell)
  {
    dir.l.y += 0.5f;
    if(kenv->CurrentEvent && kenv->CurrentEvent->Monster)
      mem->BodyPart.SkipCell = &kenv->CurrentEvent->Monster->Cell;
    mem->BodyPart.Control(dir.l);
    dir.l.y -= 0.5f;
  }
  */
  dir.k.y = 0;
  dir.k.Unit3();
  dir.j.Init(0,1,0,0);
  dir.i.Cross4(dir.j,dir.k);
  for(i=0;i<FootCount;i++)
  {
    if(mem->StepTime[i]<StepSpeed[State])
      mem->StepTime[i] += time;
  }

  // find distance

  dist = 0;
  stepdir.Init();
  NextLeg = (LastLeg+1)%FootGroup[State];
  bestleg = NextLeg;
  for(i=0;i<FootCount;i++)
  {
    v.Rotate34(dir,mem->FootDelta[i]);
    v.Sub3(foot[i]);
    v.y = 0;
    f = v.Abs3();
    if(f>dist && mem->StepTime[i]>=StepSpeed[State])
    {
      dist = f;
      stepdir = v;
      bestleg = i;
    }
    savevar[i] = kenv->Var[KV_LEG_L0+i];
    v.Rotate3(dir,spline[i]);
    kenv->Var[KV_LEG_L0+i].Add3(foot[i],v);
    kenv->Var[KV_LEG_L0+i].w = spline[i].w;
  }

  // change walking/running

  changestate = 1;
  for(i=0;i<FootCount;i++)
    if(mem->StepTime[i]<StepSpeed[State])
      changestate = 0;
  if((State==0 && dist>RunHighTresh) || (State==1 && dist<RunLowTresh))
    changestate +=2;

  // changestate = 0 -> in animation                  ->
  // changestate = 1 -> animation done.               -> issue bestleg when idle
  // changestate = 2 -> in animation, change state    -> don't issue new steps
  // changestate = 3 -> animation done, change state  -> change state really


//  sDPrintF("%08x:state %d change %d dist %f | %4d %4d %4d %4d %4d %4d\n",mem,State,changestate,dist,mem->StepTime[0],mem->StepTime[1],mem->StepTime[2],mem->StepTime[3],mem->StepTime[4],mem->StepTime[5]);
  if(changestate==3)
  {
    if((State==0 && dist>RunHighTresh) || (State==1 && dist<RunLowTresh))
    {
      State=1-State;
      changestate = 0;
      LastLeg = LastLeg%FootGroup[State];
      for(i=0;i<FootCount;i++)
        mem->StepTime[i]=StepSpeed[State];
      NextLeg = bestleg%FootGroup[State];
    }
  }

  bestleg = bestleg%FootGroup[State];
  if(mem->Idle && changestate==1)
    NextLeg = bestleg;

  // do the next step

  mem->Idle = 0;
  if(changestate!=2 && mem->StepTime[LastLeg]>=StepNext[State] && mem->StepTime[NextLeg]>=StepSpeed[State])
  {
    if(mem->InitSteps>0)
    {
      mem->InitSteps--;
      dist = 1024;
    }
    if(mem->DeviationFlag>0)
      mem->DeviationFlag--;
    if(dist>StepTresh)
    {
      v.Sub3(dir.l,mat.l);
      f = v.Abs3();
      if((Flags & 1) && mem->InitSteps==0)
        NextLeg = bestleg;

      if(f>StepLength[State])
      {
        mem->NewPos = mat.l;
        mem->NewPos.AddScale3(v,StepLength[State]/f);
      }
      else
      {
        mem->NewPos = dir.l;
      }
//      mem->NewPos.y = 0;
      dir.l = mem->NewPos;
      j = mem->StepTime[LastLeg]-StepNext[State];
      for(i=NextLeg;i<FootCount;i+=FootGroup[State])
      {
        mem->FootOld[i] = foot[i];//mem->FootNew[i];
//        sDPrintF("soll: %06.3f %06.3f %06.3f --\n ist: %06.3f %06.3f %06.3f\n",mem->FootOld[i].x,mem->FootOld[i].y,mem->FootOld[i].z,dir.l.x,dir.l.y,dir.l.z);
        v = mem->FootDelta[i];
        v.y = 0;
        v.Rotate34(dir);
        /*
        if(mem->FootPart[i].Cell) 
        {
          KKriegerCellAdd *cell;
          sVector p0,p1;

          mem->FootNew[i] = v;        // do the step in any case...
          mem->StepTime[i] = j;

          kenv->Game->CollisionForMonsterMode = 1;      // and then find a better point--

          p0 = v; p0.y += scanup;
          cell = kenv->Game->FindCell(p0);

          if(cell)
          {
            p1 = v; p1.y -= scandown;
            if(kenv->Game->FindFirstIntersect(p0,p1,cell,&plane))
            {
              if(plane.y>0.75 && p1.y<p0.y)
              {
                mem->FootNew[i] = p1;
                mem->StepTime[i] = j;
              }
            }
          }

          kenv->Game->CollisionForMonsterMode = 0;
        }
        else*/
        {
          mem->FootNew[i] = v;
          mem->StepTime[i] = j;
        }
      }
      LastLeg = NextLeg;
    }
    else
    {
      mem->Idle = 1;
    }
  }

  // paint & cleanup
/*
  if(mem->BodyPart.Cell)
  {
//    kenv->Game->AddPart(&mem->FootPart[0]);
    kenv->Game->AddPart(&mem->BodyPart);
  }
*/

  kenv->ExecStack.Push(mat);

  //save = kenv->ExecMatrix;
  //kenv->ExecMatrix = mat;
  op->ExecInputs(kenv);
  kenv->ExecStack.Pop();
  //kenv->ExecMatrix = save;

  mem->State = State;
  mem->LastLeg = LastLeg;
  mem->NextLeg = NextLeg;
  for(i=0;i<FootCount;i++)
  {
    kenv->Var[KV_LEG_L0+i] = savevar[i];
  }
  kenv->Var[KV_LEG_TIMES] = savetime;
//  if(kenv->CurrentEvent)
//    kenv->CurrentEvent->ReturnMatrix = mat;
}
#pragma lekktor(on)

/****************************************************************************/

GenScene * __stdcall Init_Scene_Rotate(GenScene *scene0,sF323 dir,sInt axxis)
{
  GenScene *scene;
  GenScene *add;

  scene = new GenScene();

  add =  MakeScene(scene0);
  if(add) *scene->Childs.Add() = add;

  return scene;
}

void __stdcall Exec_Scene_Rotate(KOp *op,KEnvironment *kenv,sF323 dir,sInt axxis)
{
  sMatrix mat;//,save;
  sVector v;
  
  v.Init(dir.x,dir.y,dir.z,0);
  v.UnitSafe3();

  const sMatrix &matrix = kenv->ExecStack.Top();
  //save = kenv->ExecMatrix;

  switch(axxis)
  {
  case 0:
    mat.i = v;
    mat.k.Cross4(mat.i,matrix.j);
    mat.k.Unit3();
    mat.j.Cross4(mat.k,mat.i);
    mat.j.Unit3();
    break;
  case 1:
    mat.j = v;
    mat.i.Cross4(mat.j,matrix.k);
    mat.i.Unit3();
    mat.k.Cross4(mat.i,mat.j);
    mat.k.Unit3();
    break;
  case 2:
    mat.k = v;
    mat.j.Cross4(mat.k,matrix.i);
    mat.j.Unit3();
    mat.i.Cross4(mat.j,mat.k);
    mat.i.Unit3();
    break;
  default:
    mat = kenv->CurrentCam.CameraSpace;
    break;
  }
  mat.l = matrix.l;

  kenv->ExecStack.Push(mat);
  op->ExecInputs(kenv);
  kenv->ExecStack.Pop();
}

/****************************************************************************/

GenScene * __stdcall Init_Scene_Forward(GenScene *scene0,sF32 tresh)
{
  GenScene *scene;
  GenScene *add;

  scene = new GenScene();

  add =  MakeScene(scene0);
  if(add) *scene->Childs.Add() = add;

  return scene;
}

struct ForwardMem : KInstanceMem
{
  sVector oldpos;
};

void __stdcall Exec_Scene_Forward(KOp *op,KEnvironment *kenv,sF32 tresh)
{
  sMatrix mat;//,save;
  sVector v;
  sF32 f;
  ForwardMem *mem;

  //save = kenv->ExecMatrix;
  const sMatrix &matrix = kenv->ExecStack.Top();

  mem = kenv->GetInst<ForwardMem>(op);
#pragma lekktor(off)
  if(mem->Reset)
  {
    mem->oldpos = matrix.l;
    mem->oldpos.AddScale3(matrix.k,-tresh);
  }
#pragma lekktor(on)

  v.Sub3(matrix.l,mem->oldpos);
  f = v.UnitAbs3();
  mem->oldpos = matrix.l;
  mem->oldpos.AddScale3(v,-tresh);

  mat.j.Init(0,1,0,0);
  mat.k = v;
  mat.k.w = 0;
  mat.i.Cross4(mat.j,mat.k);
  mat.j.Cross4(mat.k,mat.i);
  mat.l = matrix.l;

  kenv->ExecStack.Push(mat);
  //kenv->ExecMatrix = mat;
  op->ExecInputs(kenv);
  kenv->ExecStack.Pop();
  //kenv->ExecMatrix = save;
}

/****************************************************************************/

#if sLINK_KKRIEGER

GenScene * __stdcall Init_Scene_Physic(GenScene *scene0,sInt flags,sF323 speed,sF323 scale,sF323 rmassf,sF32 mass,sInt partkind)
{
  GenScene *scene;
  GenScene *add;

  scene = new GenScene();
  if(scene0)
  {
    add =  MakeScene(scene0);
    if(add) *scene->Childs.Add() = add;
  }

  return scene;
}

struct PhysicMem : KInstanceMem
{
  sInt Flags;
  KKriegerCellDynamic Cell;
//  KKriegerParticle Part;
//  KKriegerPartBox Box;
};

void __stdcall Exec_Scene_Physic(KOp *op,KEnvironment *kenv,sInt flags,sF323 speedf,sF323 scalef,sF323 rmassf,sF32 mass,sInt partkind)
{
  sMatrix mat;//,save;
  PhysicMem *mem;
  sVector speed;
  sVector scale;
  KEvent *monsterevent;
  sMatrix m1;
//  sInt i;
  KKriegerCellDynamic *monstercell;

  speed.Init(speedf.x,speedf.y,speedf.z);
  scale.Init(scalef.x,scalef.y,scalef.z);

  monsterevent = 0;
  monstercell =0 ;
  if(flags & 0x20)
  {
    monsterevent = kenv->CurrentEvent;
    if(monsterevent && monsterevent->Monster==0)
      monstercell = &kenv->Game->Player.HitCell;
  }
  //save = kenv->ExecMatrix;
  //mat = save;
  mat = kenv->ExecStack.Top();
  speed.Rotate3(mat);
  mem = kenv->GetInst<PhysicMem>(op);
  m1.Init();
#pragma lekktor(off)
  if(mem->Reset)
  {
#pragma lekktor(on)
    mem->Flags = flags;
    switch(flags&3)
    {
    case 0:
    default:
      /*
      mem->Part.Init(mat.l,speed,kenv->Game->PlayerCell);
      mem->Part.EndMarker = 1;
      mem->Part.Mass = mass;
      mem->Part.Kind = partkind;
      mem->Part.ShotEvent = monsterevent;
      mem->Part.SkipCell = monstercell;
      */
      break;
    case 1:
      /*
      mem->Box.Init(mat,scale,speed,kenv->Game->PlayerCell);
      for(i=0;i<8;i++)
      {
        mem->Box.Part[i].Mass = mass;
        mem->Box.Part[i].Kind = partkind;
        mem->Box.Part[i].ShotEvent = monsterevent;
        mem->Box.Part[i].SkipCell = monstercell;
      }
      */
      break;
    case 2:
      mem->Cell.Init(m1,scale,KCM_SUB);
      mem->Cell.PrepareDynamic(mat);
      break;
    case 3:
      /*
      mem->Cell.Init(m1,scale,KCM_SUB);
      mem->Cell.PrepareDynamic(mat);
      mem->Box.Init(mat,scale,speed,kenv->Game->PlayerCell);
      for(i=0;i<8;i++)
      {
        mem->Box.Part[i].SkipCell = &mem->Cell;
        mem->Box.Part[i].Kind = partkind;
        mem->Box.Part[i].ShotEvent = monsterevent;
      }
      mem->Cell.PartBox = &mem->Box;
*/
      break;
    }
  }

  if(flags & 0x10)
    mem->Cell.Monster = kenv->EventMonster;

  switch(mem->Flags&3)
  {
  case 0:
  default:
/*
    kenv->Game->AddPart(&mem->Part);
    mat.l = mem->Part.Pos;
    mat.l.w = 1.0f;
*/
    break;
  case 1:
/*
    mem->Box.GetMatrix(mat);
    mem->Box.Register(kenv->Game);
*/
    break;
  case 2:
    mem->Cell.Matrix1 = mat;
    kenv->Game->AddDCell(&mem->Cell);
    break;
  case 3:
/*
    mat = mem->Cell.ConfirmedMatrix;
//    mem->Box.GetMatrix(mat);
//    mem->Cell.Matrix1 = mat;
    mem->Box.Register(kenv->Game);
    kenv->Game->AddDCell(&mem->Cell);
*/
    break;
  }

  kenv->ExecStack.Push(mat);
  op->ExecInputs(kenv);
  kenv->ExecStack.Pop();
//  if(op->GetInput(0))
//    op->GetInput(0)->Exec(kenv);      
}

#endif

/****************************************************************************/

GenScene * __stdcall Init_Scene_ForceLights(GenScene *in,sInt mode,sInt sw)
{
  return in;
}

void __stdcall Exec_Scene_ForceLights(KOp *op,KEnvironment *kenv,sInt mode,sInt sw)
{
  sInt inc;

  inc = (!mode || (kenv->Game && kenv->Game->Switches[sw])) ? 1 : 0;
  //kenv->ForceLights += inc;
  op->ExecInputs(kenv);
  //kenv->ForceLights -= inc;
}

/****************************************************************************/

GenScene * __stdcall Init_Scene_MatHack(GenMaterial *)
{
  return new GenScene;
}

void __stdcall Exec_Scene_MatHack(KOp *op,KEnvironment *kenv)
{
  op->ExecInputs(kenv);
}

/****************************************************************************/

GenScene * __stdcall Init_Scene_Sector(GenScene *scene0)
{
  GenScene *scene,*add;
  
  scene = new GenScene;
  add = MakeScene(scene0);
  if(add) *scene->Childs.Add() = add;
  scene->IsSector = sTRUE;

  return scene;
}

void __stdcall Exec_Scene_Sector(KOp *op,KEnvironment *kenv)
{
  GenScene *scene;

  scene = (GenScene *)op->Cache;
  sVERIFY(scene && scene->ClassId == KC_SCENE);

  if(op->GetInput(0))
    Engine->AddSectorJob(scene,op->GetInput(0),kenv->ExecStack.Top());

  /*{
    sVERIFY(!kenv->CurrentSector);

    scene->Next = kenv->ExecSectorList;
    scene->Sector = op->GetInput(0);
    scene->SectorMatrix = kenv->ExecStack.Top();
    scene->SectorPaintThisFrame = sFALSE;
    kenv->ExecSectorList = scene;
  }*/
}

/****************************************************************************/

GenScene * __stdcall Init_Scene_Portal(GenScene *in,GenScene *s0,GenScene *s1,GenScene *si,sF322 x,sF322 y,sF322 z,sInt cost,sF32 door)
{
  if(s0) s0->Release();
  if(s1) s1->Release();
  if(si) si->Release();
  return in ? in : new GenScene;
}

void __stdcall Exec_Scene_Portal(KOp *op,KEnvironment *kenv,sF322 x,sF322 y,sF322 z,sInt cost,sF32 door)
{
  //KPortalJob *job;
  KOp *s[3];
  GenScene *sectors[3];
  
  s[0] = op->GetLink(0);
  s[1] = op->GetLink(1);
  s[2] = op->GetLink(2);

  if(s[2] && !s[2]->CheckOutput(KC_SCENE))
    s[2] = 0;

  if(s[0] && s[1] && s[0]->CheckOutput(KC_SCENE) && s[1]->CheckOutput(KC_SCENE))
  {
    for(sInt i=0;i<3;i++)
      sectors[i] = (s[i] && s[i]->Cache) ? (GenScene *)s[i]->Cache : 0;

    sAABox box;
    box.Min.Init(x.x,y.x,z.x,1.0f);
    box.Max.Init(x.y,y.y,z.y,1.0f);

    Engine->AddPortalJob(sectors,box,kenv->ExecStack.Top(),(door >= 0.02f) ? cost : 256);

#if !sPLAYER && 0 // FIXME!
    if(ShowPortalOp == op)
      ShowPortalJob = job;
#endif
  }
#if !sPLAYER // FIXME!
  if(ShowPortalOp == op)
  {
    for(sInt i=0;i<8;i++)
    {
      sVector p;
      p.Init((i&1) ? x.y : x.x,(i&2) ? y.y : y.x,(i&4) ? z.y : z.x,1);
      ShowPortalCube[i].Rotate34(kenv->ExecStack.Top(),p);
    }
    ShowPortalOpProcessed = sTRUE;
  }
#endif

  op->ExecInput(kenv,0);
}

/****************************************************************************/

GenScene * __stdcall Init_Scene_AdjustPass(GenScene *in,sInt adjust)
{
  return in;
}

void __stdcall Exec_Scene_AdjustPass(KOp *op,KEnvironment *kenv,sInt adjustPass)
{
  RenderPassAdjust += adjustPass;
  op->ExecInput(kenv,0);
  RenderPassAdjust -= adjustPass;
}

/****************************************************************************/

struct ScenePartMem : public KInstanceMem
{
  sInt Count;
  sInt OldCount;
  sInt Seed;
  sInt Mode;
  sF323 Rand;
  sVector *Pos;
};

GenScene * __stdcall Init_Scene_Particles(GenScene *scene0,
  sInt mode,sInt count,sInt seed,
  sF323 rand,sF323 rot,sF323 rotspeed,sF32 anim,
  sF323 line,KSpline *spline)
{
  GenScene *scene;
  GenScene *add;

  scene = new GenScene();

  add =  MakeScene(scene0);
  if(add) *scene->Childs.Add() = add;

  return scene;
}

/****************************************************************************/

void __stdcall Exec_Scene_Particles(KOp *op,KEnvironment *kenv,
  sInt mode,sInt count,sInt seed,
  sF323 rand,sF323 rot,sF323 rotspeed,sF32 anim,
  sF323 line,KSpline *spline)
{
  sInt i;
  sMatrix mat;
  ScenePartMem *mem;
  sF32 f;
  sVector v,v0;


  //spline = op->GetSpline(0);

  mem = kenv->GetInst<ScenePartMem>(op);
  if(mem->Reset || mem->Mode!=mode || mem->Count!=count || mem->Seed!=seed
     || mem->Rand.x!=rand.x || mem->Rand.y!=rand.y || mem->Rand.z!=rand.z)
  {
    mem->Count = count;
    mem->Mode = mode;
    mem->Seed = seed;
    mem->Rand = rand;
    mem->Pos = new sVector[count];
    mem->DeleteArray = mem->Pos;
    sSetRndSeed(seed);
    for(i=0;i<count;i++)
    {
      do
      {
        mem->Pos[i].x = (sFGetRnd(2.0f)-1.0f);
        mem->Pos[i].y = (sFGetRnd(2.0f)-1.0f);
        mem->Pos[i].z = (sFGetRnd(2.0f)-1.0f);
        if((mode&12)==4)
          mem->Pos[i].Unit3();
      }
      while(((mode&12)==8) && mem->Pos[i].Dot3(mem->Pos[i])>1.0);

      if(mode&0x20)
        mem->Pos[i].w = 1.0f*i/count;
      else
        mem->Pos[i].w = sFGetRnd(1.0f);
      mem->Pos[i].x *= rand.x*0.5f;
      mem->Pos[i].y *= rand.y*0.5f;
      mem->Pos[i].z *= rand.z*0.5f;
    }
  }

  anim = sFMod(anim,1.0f);
  if(anim<0) anim+=1;

  for(i=0;i<mem->Count;i++)
  {
    mat.Init();
    f = anim+mem->Pos[i].w;
    if(f>=1.0f) f -= 1.0f;

    if((mode&1) && spline)             // spline mode
    {
      spline->Eval(f,v);
      if(mode&0x10)
      {
        spline->Eval(f+0.01f,v0);
        v0.Sub3(v);
        mat.InitDir(v0);
      }
      mat.l.x += mem->Pos[i].x + v.x;
      mat.l.y += mem->Pos[i].y + v.y;
      mat.l.z += mem->Pos[i].z + v.z;
      mat.l.w = 1;
    }
    else                                // line mode
    {
      mat.l.x += mem->Pos[i].x + line.x*f;
      mat.l.y += mem->Pos[i].y + line.y*f;
      mat.l.z += mem->Pos[i].z + line.z*f;
      mat.l.w = 1;
    }
    
    kenv->ExecStack.PushMul(mat);
    ExecSceneInputs(op,kenv,0);
    kenv->ExecStack.Pop();
  }
}

/****************************************************************************/

GenScene * __stdcall Init_Scene_ApplySpline(GenScene *add,GenSpline *sp)
{
  GenScene *scene;

  sp->Release();
  add = MakeScene(add);
  if(!add) return 0;

  scene = new GenScene;
  *scene->Childs.Add() = add;

  return scene;
}
 
void __stdcall Exec_Scene_ApplySpline(KOp *op,KEnvironment *kenv,sF32 time)
{
  sMatrix mat;
  GenSpline *sp;
  sF32 zoom;

  sp = (GenSpline *)op->GetInput(1)->Cache;
  sVERIFY(sp);

  sp->Eval(time,0,mat,zoom);

  kenv->ExecStack.PushMul(mat);
  ExecSceneInputs(op,kenv,0);
  kenv->ExecStack.Pop();
}

/****************************************************************************/

GenScene * __stdcall Init_Scene_Marker(GenScene *add,sInt marker)
{
  GenScene *scene = new GenScene;
  *scene->Childs.Add() = MakeScene(add);
  return scene;
}

void __stdcall Exec_Scene_Marker(KOp *op,KEnvironment *kenv,sInt marker)
{
  sVERIFY(marker>=0 && marker<sCOUNTOF(kenv->Markers));
  kenv->Markers[marker] = kenv->ExecStack.Top();
  ExecSceneInputs(op,kenv,0);
}

/****************************************************************************/

GenScene * __stdcall Init_Scene_LOD(GenScene *high,GenScene *low,sF32 lod)
{
  GenScene *scene = new GenScene;
  *scene->Childs.Add() = MakeScene(high);
  if(low)
    *scene->Childs.Add() = MakeScene(low);
  return scene;
}

void __stdcall Exec_Scene_LOD(KOp *op,KEnvironment *kenv,sF32 lod)
{
  sF32 dist;
  sVector v;
  sMatrix mat0;
  sMatrix mat1;

  mat0 = kenv->ExecStack.Top();
  mat1 = kenv->CurrentCam.CameraSpace;

  v.Sub3(mat1.l,mat0.l);
  dist = -v.Dot3(mat1.k);

  if(dist<lod)
    ExecSceneInput(op,kenv,0);
  else if(op->GetInputCount()==2)
    ExecSceneInput(op,kenv,1);
}

/****************************************************************************/

GenScene * __stdcall Init_Scene_Ambient(sU32 color)
{
  return new GenScene;
}

void __stdcall Exec_Scene_Ambient(KOp *op,KEnvironment *kenv,sU32 color)
{
  Engine->AddAmbientLight(color);
}

/****************************************************************************/
