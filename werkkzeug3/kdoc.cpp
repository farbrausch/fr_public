// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "kdoc.hpp"

#include "genbitmap.hpp"
#include "genmaterial.hpp"
#include "genmesh.hpp"
#include "genminmesh.hpp"
#include "genoverlay.hpp"
#include "genscene.hpp"
#include "geneffect.hpp"
#include "kkriegergame.hpp"

#if !sPLAYER
KOp *KOpCurrent = 0;
#include "werkkzeug.hpp"
#endif

#define sUSE_TIME_OF_YEAR 1
#define sOP_PERFORMANCE 0

#if sUSE_TIME_OF_YEAR
struct SYSTEMTIME
{
  sU16 wYear;
  sU16 wMonth;
  sU16 wDayOfWeek;
  sU16 wDay;
  sU16 wHour;
  sU16 wMinute;
  sU16 wSecond;
  sU16 wMilliseconds;
};

typedef sU64 FILETIME;

extern "C" void __stdcall GetSystemTime(SYSTEMTIME *lpSystemTime);
extern "C" sBool __stdcall SystemTimeToFileTime(const SYSTEMTIME *lpSystemTime,FILETIME *lpFileTime);
#endif

#if sOP_PERFORMANCE
extern "C" int __cdecl wsprintfA(char *dest,const char *format,...);
extern "C" void __stdcall OutputDebugStringA(const char *str);
#endif

/****************************************************************************/
/***                                                                      ***/
/***   Helpers                                                            ***/
/***                                                                      ***/
/****************************************************************************/

static KOp *SkipNops(KOp *op)
{
  while(op)
  {
    if(op->Command == 0x01) // load
      op = op->GetLink(0);
    else if(op->Command >= 0x02 && op->Command <= 0x05 ) // store/nop/unknown/time
      op = op->GetInput(0);
    else if(op->Command == 0x06) // event
      op = op->GetInput(0);
    else
      break;
  }

  return op;
}

static sBool WouldSkip(KOp *op)
{
  return !op || (op->Command >= 0x01 && op->Command <= 0x06);
}

/****************************************************************************/
/***                                                                      ***/
/***   Objects                                                            ***/
/***                                                                      ***/
/****************************************************************************/

KObject::KObject() 
{ 
  ClassId=KC_NULL; 
  RefCount = 1;
}

KObject::~KObject() 
{
}

void KObject::Copy(KObject *o) 
{
  ClassId=o->ClassId;
}

KObject *KObject::Copy()
{
  KObject *r;
  r = new KObject;
  r->Copy(this);
  return r;
}

sBool KObject::Strip()
{
  return sFALSE;
}

sBool KObject::IsStripped()
{
  return sFALSE;
}

/****************************************************************************/
/***                                                                      ***/
/***   Execution                                                          ***/
/***                                                                      ***/
/****************************************************************************/

static sInt PrecalcCount,PrecalcMax;

#if sOP_PERFORMANCE
static sInt OpPerf[256];
#endif

#if !sPLAYER
void BlockOp(KOp *op)
{
  op = SkipNops(op);
  if(op)
    op->Blocked++;
}

void UnblockOp(KOp *op)
{
  op = SkipNops(op);
  if(op)
    op->Blocked--;
}
#endif

sInt KOp::Calc(KEnvironment *env,sInt flags)
{
  sInt i;
  sInt pops;
  sInt changed;

// early break recursion
  if(SkipCalc && !Changed && Cache && !Cache->IsStripped())
    return 0;

  sFloatFix();

// cycle check start

#if !sPLAYER
  if(CycleFlag)
    return 0;
  if(env->CalcAbort)
    return 0;
  CycleFlag = 1;
#endif

// calculate animation

  if(Changed||!Cache)
    sCopyMem(DataAnim,DataEdit,DataMax*4);

  SkipCalc = sTRUE;
  pops = env->ExecuteAnim(this,AnimCode);

// find changed

  changed = Changed;
  if(flags&KCF_NEED)
  {
    if(!Cache)
      changed = sTRUE;
    else if(!(flags & KCF_STRIPPEDOK) && Cache->IsStripped())
      changed = sTRUE;
  }
  /*if(Cache && !changed && ((flags & KCF_STRIPPEDOK) || !Cache->IsStripped()))
    flags &= ~KCF_NEED;*/
  if(Cache && !changed)
  {
    if((flags & KCF_STRIPPEDOK) || !Cache->IsStripped())
      flags &= ~KCF_NEED;
    else
      flags = flags;
  }

// skip exec calculation

  SkipExec = sFALSE;
  if(Convention&OPC_SKIPEXEC)
    SkipExec = sTRUE;
  if(AnimCode[0]!=KA_END)
    SkipExec = sFALSE;

// recursion
  if(Convention&OPC_STRIPPEDIN)
    flags|=KCF_STRIPPEDOK;
  else
    flags&=~KCF_STRIPPEDOK;

  for(i=0;i<InputCount;i++)
  {
    if(Input[i])
    {
      changed |= Input[i]->Calc(env,flags&~KCF_ROOT);
#if !sPLAYER
      BlockOp(Input[i]);
      //Input[i]->Blocked++;
#endif
      UpdateVar(Input[i]->VarStart,Input[i]->VarCount);
      SkipExec &= Input[i]->SkipExec;
      SkipCalc &= Input[i]->SkipCalc;
    }
  }
  if(!(Convention & OPC_DONTCALLLINK))
  {
    for(i=0;i<LinkMax;i++)
    {
      if(Link[i])
      {
        changed |= Link[i]->Calc(env,flags&~KCF_ROOT);
#if !sPLAYER
        BlockOp(Link[i]);
        //Link[i]->Blocked++;
#endif
        UpdateVar(Link[i]->VarStart,Link[i]->VarCount);
        SkipExec &= Link[i]->SkipExec;
        SkipCalc &= Link[i]->SkipCalc;
      }
    }
  }

  if(changed)
    sVERIFY(flags & KCF_NEED);

// calculate

  if(changed)
  {
    if(Cache)
    {
#if !sPLAYER
      KMemoryManager->Remove(this);
#endif
      Cache->Release();
      Cache = 0;
    }

#if sOP_PERFORMANCE
    sU32 stime = sSystem->GetTime();
#endif

    Cache = Call(env);
#if !sPLAYER
    if(Tag&4)
    {
      env->CalcAbort |= sSystem->ProgressGDI(1);
      Tag &= ~4;
    }
#endif

#if sOP_PERFORMANCE
    OpPerf[Command] += sSystem->GetTime() - stime;
#endif

#if !sINTRO // in the intro, we just let things run and compact later
    if(Cache && Cache->ClassId == KC_MESH)
      ((GenMesh *) Cache)->Compact();
#endif

    CalcError = (Cache==0);
    CacheCount = OutputCount;
    Changed = 0;
  }

// done

  env->Pop(pops);

#if !sPLAYER
  WheelSpeed = sMax(WheelSpeed,96);
  CycleFlag = 0;
  for(i=0;i<InputCount;i++)
    UnblockOp(Input[i]);
    /*if(Input[i])
      Input[i]->Blocked--;*/
  if(!(Convention & OPC_DONTCALLLINK))
    for(i=0;i<LinkMax;i++)
      UnblockOp(Link[i]);
      /*if(Link[i])
        Link[i]->Blocked--;*/

  if((changed || (flags&KCF_ROOT)) && Cache && !Cache->IsStripped())
    KMemoryManager->Use(this);
  if(changed)
  {
    BlockOp(this);
    //Blocked++;
    KMemoryManager->Manage();
    UnblockOp(this);
    //Blocked--;
  }
#endif
#if sPLAYER
  if(flags & KCF_PRECALC)
  {
    for(sInt j=0;j<GetInputCount();j++)
      GetInput(j)->CalcUsed();

    for(sInt j=0;j<GetLinkMax();j++)
      GetLink(j)->CalcUsed();

    sSystem->Progress(PrecalcCount,PrecalcMax);
    if(changed)
      PrecalcCount++;
  }
#endif
  return changed;
}

/****************************************************************************/

#pragma warning(disable : 4731)   // EPB CHANGED WARNING

static sInt CallCode(sInt code,sInt *para,sInt count)
{
  sInt result;
  __asm
  {
    mov eax,code
    mov esi,para
    mov ecx,count
    push ebp
    mov ebp,esp
    sub esp,ecx
    sub esp,ecx
    sub esp,ecx
    sub esp,ecx
    mov edi,esp
    rep movsd

    call eax

    mov esp,ebp
    pop ebp
    mov result,eax
  }

  return result; 
}

#pragma warning(default : 4731)

/****************************************************************************/

KObject *KOp::Call(KEnvironment *kenv)
{
  sU32 data[KK_MAXINPUT+128];
  sInt p,pNoPara;
  sInt i,max;
  KObject *o;
  KOp *op;
  KObject *result;
  sInt error;

  p = 0;
#if !sINTRO
  error = 0;
#endif

// update

  if(Convention&(OPC_KOP|OPC_ALTINIT))
  {
    data[p++] = (sU32) this;
  }
  if(Convention&OPC_KENV)
  {
    data[p++] = (sU32) kenv;
  }

  pNoPara = p;

// inputs
  max = OPC_GETINPUT(Convention);
  for(i=0;i<max;i++)
  {
    op = GetInput(i);

    if(op)
    {
      o = op->Cache;
      if(o==0)
        error = 1;
      else
        o->AddRef();
      data[p++] = (sU32) o;
    }
    else
    {
      data[p++] = 0;
    }
  }

// links

  max = OPC_GETLINK(Convention);
  for(i=0;i<max;i++)
  {
    if(Convention & OPC_DONTCALLLINK)
    {
      data[p++] = (sU32) Link[i];
    }
    else
    {
      o = (Link[i] ? Link[i]->Cache : 0);
      if(o)
        o->AddRef();
      data[p++] = (sU32) o;
    }
  }

// normal parameters

  max = OPC_GETDATA(Convention);
  for(i=0;i<max;i++)
    data[p++] = *GetAnimPtrU(i);

// strings

  max = OPC_GETSTRING(Convention);
  for(i=0;i<max;i++)
    data[p++] = (sU32) GetString(i);

// splines

  max = OPC_GETSPLINE(Convention);
  for(i=0;i<max;i++)
    data[p++] = (sU32) GetSpline(i);

// unlimited inputs

  if(Convention&OPC_VARIABLEINPUT)
  {
    data[p++] = InputCount;
    for(i=0;i<InputCount;i++)
    {
      if(Input[i] && Input[i]->Cache && p<KK_MAXINPUT+128)
      {
        Input[i]->Cache->AddRef();
        data[p++] = (sU32) Input[i]->Cache;
      }
      else
        error = 1;
    }
  }

// call!

#if !sPLAYER
  sVERIFY(Werk);
  if(GetInputCount()<Werk->Class->MinInput && GetInputCount()>Werk->Class->MaxInput)
    error = 1;
  for(i=0;i<Werk->Class->MinInput;i++)
    if(GetInput(i)==0) 
      error = 1;
  for(i=0;i<GetInputCount();i++)
    if(GetInput(i) && !GetInput(i)->Cache)
      error = 1;
  if(!(Convention & OPC_DONTCALLLINK))
    for(i=0;i<GetLinkMax();i++)
      if(GetLink(i) && !GetLink(i)->Cache)
        error = 1;
#endif

#if !sINTRO
  result = 0;
  if(!error)
  {
    if(Convention&OPC_ALTINIT)
      p = pNoPara;

    result = (KObject *)CallCode((sInt)InitHandler,(sInt *)data,p);
  }
#else
  {
    if(Convention&OPC_ALTINIT)
      p = pNoPara;

    result = (KObject *)CallCode((sInt)InitHandler,(sInt *)data,p);
  }
#endif

// cleanup in case of failure

#if !sINTRO
  if(result==0)
  {
    max = OPC_GETINPUT(Convention);
    for(i=0;i<max;i++)
    {
      op = GetInput(i);
      if(op)
      {
        o = (op->Cache ? op->Cache : 0);
        if(o) o->Release();
      }
    }
    if(Convention&OPC_VARIABLEINPUT)
    {
      for(i=0;i<InputCount;i++)
        if(Input[i] && Input[i]->Cache)
          Input[i]->Cache->Release();
    }
    if(!(Convention & OPC_DONTCALLLINK))
    {
      max = OPC_GETLINK(Convention);
      for(i=0;i<max;i++)
      {
        o = (Link[i] ? Link[i]->Cache : 0);
        if(o)
          o->Release();
      }
    }
  }
#endif

#if sPLAYER && sDEBUG
  sVERIFY(result);
#endif

  return result;
}

/****************************************************************************/

void KOp::ExecWithNewMem(KEnvironment *kenv,KInstanceMem **link)
{
  KInstanceMem *oldnext,**oldlink;

  oldnext = kenv->NextInstance;
  oldlink = kenv->LinkInstance;

  if(*link)
  {
    kenv->NextInstance = *link;
    kenv->LinkInstance = 0;
  }
  else
  {
    kenv->NextInstance = 0;
    kenv->LinkInstance = link;
  }

  Exec(kenv);

  kenv->NextInstance = oldnext;
  kenv->LinkInstance = oldlink;
}

void KOp::Exec(KEnvironment *kenv)
{
  sU32 data[KK_MAXINPUT+128];
  sInt p;
  sInt i,max;
  sInt pops;

  p = 0;
#if !sPLAYER
  if(CycleFlag)
    return;

  if(GetInputCount()<Werk->Class->MinInput && GetInputCount()>Werk->Class->MaxInput)
    return;
  for(i=0;i<Werk->Class->MinInput;i++)
    if(GetInput(i)==0) 
      return;
  if(Result!=KC_MESH)
  {
    for(i=0;i<GetInputCount();i++)
      if(GetInput(i) && !GetInput(i)->Cache)
        return;
  }
  for(i=0;i<GetLinkMax();i++)
    if(GetLink(i) && !GetLink(i)->Cache)
      return;
#endif

  if(Result==KC_BITMAP)       // bitmaps haben kein Exec, weil sie nicht animierbar sind
    return;

#if !sPLAYER
  CycleFlag = 1;                            // be careful with cyclecheck: one error and the item will NEVER be updated again.
#endif

// do animation

  pops = kenv->ExecuteAnim(this,AnimCode);

// the simple case...

  if(Convention&OPC_ALTEXEC)
  {
    ((void (__stdcall *)(KOp *,KEnvironment *))ExecHandler)(this,kenv);
  }
  else
  {

// parameter-case

    data[p++] = (sU32) this;              // obligatorische parameter
    data[p++] = (sU32) kenv;              

    max = OPC_GETDATA(Convention);        // normal parameters
    sCopyMem(data+p,GetAnimPtrU(0),max*sizeof(sU32));
    p += max;

    max = OPC_GETSTRING(Convention);      // strings
    for(i=0;i<max;i++)
      data[p++] = (sU32) GetString(i);

    max = OPC_GETSPLINE(Convention);      // splines
    for(i=0;i<max;i++)
      data[p++] = (sU32) GetSpline(i);

    CallCode((sInt)ExecHandler,(sInt *)data,p);   // call
  }

// end animation

  kenv->Pop(pops);
  WheelSpeed = sMax(WheelSpeed,96);
#if !sPLAYER
  CycleFlag = 0;
#endif
}

/****************************************************************************/
/****************************************************************************/

void KOp::ExecInputs(KEnvironment *kenv)
{
  sInt i,max;
  KOp *op;
  
  max = GetInputCount();
  for(i=0;i<max;i++)
  {
    op = GetInput(i);
    sVERIFY(op);
    op->Exec(kenv);
  }
  if(!(Convention & OPC_DONTCALLLINK))
  {
    max = GetLinkMax();
    for(i=0;i<max;i++)
    {
      op = GetLink(i);
      if(op)
        op->Exec(kenv);
    }
  }
}

void KOp::ExecInput(KEnvironment *kenv,sInt i)
{
  KOp *op;

  op = GetInput(i);
  if(op)
    op->Exec(kenv);
}


void KOp::ExecEvent(KEnvironment *kenv,KEvent *event,sBool)
{
  sVector save[8];
  KEvent *ce;
  sInt i;
  sF32 time;

  KSpline *oldspline;
  struct KKriegerMonster *oldmon;

  for(i=0;i<8;i++)                          // save all
    save[i] = kenv->Var[i];

  oldmon = kenv->EventMonster;
  oldspline = kenv->EventSpline;
  //oldsector = kenv->CurrentSector;
  ce = kenv->CurrentEvent;

  time = 0;                                 // calculate time
  if(event->Start<event->End)
    time = 1.0f * (kenv->BeatTime - event->Start) / (event->End-event->Start);

  time = sRange<sF32>(time,1,0);
  time = event->StartInterval + time * (event->EndInterval - event->StartInterval);

//  sDPrintF("%08x:%08x / %08x / %08x -> %f\n",this,event->Start,event->End,kenv->BeatTime,time);

  if(!(event->Flags&KEF_NOTIME))
    kenv->Var[0].InitS(time); // set variables
  kenv->Var[1].InitS(event->Velocity);
  kenv->Var[2].InitS(event->Modulation);
  kenv->Var[3].InitS(sFtol(event->Select));
  kenv->Var[4] = event->Scale;
  kenv->Var[5] = event->Rotate;
  kenv->Var[6] = event->Translate;
  kenv->Var[7].InitColor(event->Color);

  // time of year computation
#if sUSE_TIME_OF_YEAR
  SYSTEMTIME systemTime;
  sU64 timeYear,startYear,endYear;

  GetSystemTime(&systemTime);
  SystemTimeToFileTime(&systemTime,(FILETIME *) &timeYear);

  systemTime.wMonth = 1;
  systemTime.wDay = 1;
  systemTime.wHour = systemTime.wMinute = systemTime.wSecond = 0;
  systemTime.wMilliseconds = 0;
  SystemTimeToFileTime(&systemTime,(FILETIME *) &startYear);

  systemTime.wYear++;
  SystemTimeToFileTime(&systemTime,(FILETIME *) &endYear);

  kenv->Var[3].w = 1.0 * (timeYear - startYear) / (endYear - startYear);
#endif

  if(event->Spline)                         // set spline
    kenv->EventSpline = event->Spline;
  kenv->EventMonster = event->Monster;
  kenv->CurrentEvent = event;               // set event

  kenv->ExecStack.PushMul(event->Matrix);
  ExecWithNewMem(kenv,&event->FirstInstance);
  kenv->ExecStack.Pop();

  kenv->EventSpline = oldspline;
  kenv->EventMonster = oldmon;
  kenv->CurrentEvent = ce;
  for(i=0;i<8;i++)
    kenv->Var[i] = save[i];
}

void KOp::Change(sInt input)
{
  sInt i;

  if(Changed) return;
#if !sPLAYER
  if(CycleChangeFlag)
    return;
  CycleChangeFlag = 1;
#endif
/*
  if(Update && input>=0 && input<32 && (ChangeMask & (1<<input)))
  {
    sCopyMem(Update->Data,GetAnimPtr(0),sMax(4*DataMax,128));
    return;
  }
*/
  WheelSpeed = 255;
  Changed = 1;
  for(i=0;i<OutputCount;i++)
    if(Output[i])
      Output[i]->Change(-1);
#if !sPLAYER
  CycleChangeFlag = 0;
#endif
}

sBool KOp::CheckOutput(sInt kc)
{
  if(!this) return sFALSE;
#if sPLAYER
  if(!Cache || CacheFreed)
  {
    if(!Result)
      return sFALSE;
    else if(kc!=KC_ANY && Result!=kc)
      return sFALSE;
  }
  else
    if(kc!=KC_ANY && Cache->ClassId!=kc)
      return sFALSE;
#else
  if(!Cache) return sFALSE;
  if(kc!=KC_ANY && Cache->ClassId!=kc) return sFALSE;
#endif
  return sTRUE;
}

void KOp::UpdateVar(sInt start,sInt count)
{
  sInt min,max;
  if(count)
  {
    if(VarCount==0)
    {
      VarStart = start;
      VarCount = count;
    }
    else
    {
      min = VarStart;
      max = VarStart+VarCount;

      if(start<min)
        min = start;
      if(start+count>max)
        max = start+count;

      VarStart = min;
      VarCount = max-min;
    }
  }
}

sBool AnimCodeWritesTo(sU8 *code,sInt offset)
{
  sU8 mask,val;

  while(*code!=KA_END)
  {
    if(*code>=0x80)
    {
      mask = code[0];
      val = code[1];
      if((mask&1) && val+0==offset) return sTRUE;
      if((mask&2) && val+1==offset) return sTRUE;
      if((mask&4) && val+2==offset) return sTRUE;
      if((mask&8) && val+3==offset) return sTRUE;
    }
    code += CalcCmdSize(code);
  }
  return sFALSE;
}

#if sPLAYER
void KOp::CalcUsed()
{
  if(this && !--CacheCount && Cache && !CacheFreed)
  {
    if(Cache->ClassId == KC_BITMAP || Cache->ClassId == KC_MESH || Cache->ClassId == KC_MINMESH)
    {
      Result = Cache->ClassId;
      Cache->Release();
      CacheFreed = sTRUE;
      // sollte die variable nicht auch auf 0 gesetzt werden?
      // nein, sollte sie nicht! -ryg
    }
  }
}
#endif

/****************************************************************************/
/***                                                                      ***/
/***   Environment                                                        ***/
/***                                                                      ***/
/****************************************************************************/

void KInstanceMem::DeleteChain()
{
  KInstanceMem *next,*mem;

  next = this;
  while(next)
  {
    mem = next;
    next = mem->Next;
    if(mem->DeleteArray)
      delete[] mem->DeleteArray;
    if(mem->DeleteChild && *(mem->DeleteChild))
    {
      (*(mem->DeleteChild))->DeleteChain();
      (*(mem->DeleteChild)) = 0;
    }
    if(mem->DeleteChild2 && *(mem->DeleteChild2))
    {
      (*(mem->DeleteChild2))->DeleteChain();
      (*(mem->DeleteChild2)) = 0;
    }
    delete mem;
  }
}

KEnvironment::KEnvironment()
{
  sInt i;

  EventSpline = 0;
  EventMonster = 0;
  Splines = 0;
  SplineCount = 0;
  //Mem.Init(256*1024);
  Mem.Init(256*1024);
  LastTime = 0;
  TimeReset = 1;
  DefaultInstance = 0;
  NextInstance = 0;
  LinkInstance = 0;
  DynamicEvents.Init();
  EventOpsCleanup.Init();
  InitFrame(0,0);
  ClearInstanceMem = 0;
  sSetMem(Letters,0,sizeof(Letters));
  for(i=0;i<64;i++)
  {
    Letters[0][i+0x40].UV.x0 = (i&7)*0.125f;
    Letters[0][i+0x40].UV.y0 = (i/8)*0.125f;
    Letters[0][i+0x40].UV.x1 = (i&7)*0.125f+0.125f;
    Letters[0][i+0x40].UV.y1 = (i/8)*0.125f+0.125f;
  }
  Game = 0;
  ExecStack.Init();
#if !PLAYER
  DebugOpByte = 0;
  DebugOpOutput.Init(0,0,0,0);
#endif
}

KEnvironment::~KEnvironment()
{
  InitView();
  InitFrame(0,0);
  ExitFrame();
  Mem.Exit();
  DynamicEvents.Exit();
  EventOpsCleanup.Exit();
  ExecStack.Exit();
}

void KEnvironment::InitView()
{
  sInt i;

  for(i=0;i<DynamicEvents.Count;i++)
  {
    DynamicEvents[i]->FirstInstance->DeleteChain();
    delete DynamicEvents[i];
  }
  DynamicEvents.Count = 0;
  ClearInstanceMem = 1;
  DefaultInstance->DeleteChain();
  DefaultInstance = 0;
  NextInstance = 0;
  LinkInstance = 0;
  Aspect = 1.0f;

  BeatTime = 0;
  CurrentTime = 0;
  LastTime = 0;
  TimeDelta = 0;
  TimeSlices = 0;
  TimeJitter = 0;
  TimeReset = 1;

  for(sInt i=0;i<sCOUNTOF(Markers);i++)
    Markers[i].Init();

  if(Game)
    Game->Monsters.Count=0;
}

void KEnvironment::InitFrame(sInt timebeat,sInt timems)
{
  sInt i;

  sSetMem(Var,0,sizeof(Var));
  Var[0].Init(timebeat/65536.0f,timebeat/65536.0f,timebeat/65536.0f,timebeat/65536.0f);
  Var[4].Init(1,1,1,0);
  sSetMem(Stack,0,sizeof(Stack));
  sSetMem(StackAdr,0,sizeof(StackAdr));
  Index = 0;
  GameCam.Init();
  CurrentCam.Init();
  ExecStack.PopAll();
  EventSpline = 0;
  EventMonster = 0;
  CurrentEvent = 0;
#if !sPLAYER
  CalcAbort = 0;
#endif

  BeatTime = timebeat;
  LastTime = CurrentTime;
  CurrentTime = timems;
  TimeDelta = CurrentTime-LastTime;
  if(TimeDelta<0)
  {
    TimeReset = 1;
  }
  else
  {
    TimeSlices = (CurrentTime/10)-(LastTime/10);
    TimeJitter = (CurrentTime%10);
    TimeReset =0;
  }
  if(TimeReset)
  {
    TimeDelta = 0;
    TimeSlices = 0;
    TimeJitter = 0;
  }

  // add dynamic events

  sVERIFY(EventOpsCleanup.Count == 0);
  for(i=0;i<DynamicEvents.Count;i++)
    AddStaticEvent(DynamicEvents[i]);

  // set up instance memory

  NextInstance = DefaultInstance;
  LinkInstance = 0;
  if(!DefaultInstance)
    LinkInstance = &DefaultInstance;

#if sLINK_KKRIEGER
  NextMonsterOverride=0;
  NextMonsterKillSwitch=0;
  NextMonsterSpawnSwitch=0;
  NextMonsterFlags=0;
#endif
}

void KEnvironment::ExitFrame()
{
  sInt i;
  KEvent *ev;

  for(i=0;i<EventOpsCleanup.Count;i++)
    EventOpsCleanup[i]->FirstEvent = 0;

  for(i=0;i<DynamicEvents.Count;)
  {
    ev = DynamicEvents[i];
    if(ev->End!=ev->Start && BeatTime >= ev->End)
    {
      ev->FirstInstance->DeleteChain();
      DynamicEvents[i]= DynamicEvents[--DynamicEvents.Count];
      delete ev;
    }
    else
    {
      i++;
    }
  }
  EventOpsCleanup.Count = 0;
  ClearInstanceMem = 0;
}

void KEnvironment::AddDynamicEvent(KEvent *event)
{
  *DynamicEvents.Add() = event;
}

void KEnvironment::AddStaticEvent(KEvent *event)
{
  if(event->Start==event->End || (BeatTime>=event->Start && BeatTime<event->End))
  {
    if(ClearInstanceMem)
    {
      event->FirstInstance->DeleteChain();
      event->FirstInstance = 0;
    }
    if(event->Op->FirstEvent==0)
      *EventOpsCleanup.Add() = event->Op;
    event->NextOp = event->Op->FirstEvent;
    event->Op->FirstEvent = event;
  }  
}

/****************************************************************************/
/***                                                                      ***/
/***   Animation                                                          ***/
/***                                                                      ***/
/****************************************************************************/

#define STACKMAX 256

sInt KEnvironment::ExecuteAnim(struct KOp *op,sU8 *bytecode)
{
  sInt pop;
  sInt cmd,cmd2;
  sInt index;
  static sVector stack[STACKMAX+4];
  sVector *p;
  sS32 *destint;
  sU8 *destbyte;
  sU8 bval;
  sF32 *destfloat;
  sF32 fval;
  sBool changed,cp;
  sInt i;
  sMatrix *mat;

  pop=0;
  changed = 0;
  p = &stack[2];

  for(;;)
  {
#if !PLAYER
    sBool debug = (bytecode==DebugOpByte);
#endif
    cmd2 = cmd = *bytecode++;
    if(cmd2>=0x80)
      cmd2 &= 0xf0;
    // if you change something here, remember to also change KDoc::Init

    switch(cmd2)
    {
    case KA_NOP:
      break;
    case KA_END:
      goto ende;
    case KA_LOADVAR:
      index = *bytecode++;
      sVERIFY(index<KK_MAXVAR);
      *p = Var[index];
      p++;
      op->UpdateVar(index,1);
      break;
    case KA_LOADPARA1:
      index = *bytecode++;
      sVERIFY(index<op->GetDataWords());
      fval = ((sF32 *)op->DataEdit)[index];
      p->x = fval;
      p->y = fval;
      p->z = fval;
      p->w = fval;
      p++;
      break;
    case KA_LOADPARA2:
      index = *bytecode++;
      sVERIFY(index+1<op->GetDataWords());
      p->x = ((sF32 *)op->DataEdit)[index+0];
      p->y = ((sF32 *)op->DataEdit)[index+1];
      p->z = 0;
      p->w = 0;
      p++;
      break;
    case KA_LOADPARA3:
      index = *bytecode++;
      sVERIFY(index+2<op->GetDataWords());
      p->x = ((sF32 *)op->DataEdit)[index+0];
      p->y = ((sF32 *)op->DataEdit)[index+1];
      p->z = ((sF32 *)op->DataEdit)[index+2];
      p->w = 0;
      p++;
      break;
    case KA_LOADPARA4:
      index = *bytecode++;
      sVERIFY(index+3<op->GetDataWords());
      p->x = ((sF32 *)op->DataEdit)[index+0];
      p->y = ((sF32 *)op->DataEdit)[index+1];
      p->z = ((sF32 *)op->DataEdit)[index+2];
      p->w = ((sF32 *)op->DataEdit)[index+3];
      p++;
      break;
    case KA_SWIZZLEX:
      p[-1].x = p[-1].x;
      p[-1].y = p[-1].x;
      p[-1].z = p[-1].x;
      p[-1].w = p[-1].x;
      break;
    case KA_SWIZZLEY:
      p[-1].x = p[-1].y;
      p[-1].y = p[-1].y;
      p[-1].z = p[-1].y;
      p[-1].w = p[-1].y;
      break;
    case KA_SWIZZLEZ:
      p[-1].x = p[-1].z;
      p[-1].y = p[-1].z;
      p[-1].z = p[-1].z;
      p[-1].w = p[-1].z;
      break;
    case KA_SWIZZLEW:
      p[-1].x = p[-1].w;
      p[-1].y = p[-1].w;
      p[-1].z = p[-1].w;
      p[-1].w = p[-1].w;
      break;
    case KA_ADD:
      p--;
      p[-1].Add4(*p);
      break;
    case KA_SUB:
      p--;
      p[-1].Sub4(*p);
      break;
    case KA_MUL:
      p--;
      p[-1].Mul4(*p);
      break;
    case KA_DIV:
      p--;
      p[-1].x /= p->x;
      p[-1].y /= p->y;
      p[-1].z /= p->z;
      p[-1].w /= p->w;
      break;
    case KA_MOD:
      p--;
      p[-1].x = sFMod(p[-1].x,p->x);
      p[-1].y = sFMod(p[-1].y,p->y);
      p[-1].z = sFMod(p[-1].z,p->z);
      p[-1].w = sFMod(p[-1].w,p->w);
      break;
    case KA_NEG:
      p[-1].x = -p[-1].x;
      p[-1].y = -p[-1].y;
      p[-1].z = -p[-1].z;
      p[-1].w = -p[-1].w;
      break;
    case KA_INVERT:
      p[-1].x = 1-p[-1].x;
      p[-1].y = 1-p[-1].y;
      p[-1].z = 1-p[-1].z;
      p[-1].w = 1-p[-1].w;
      break;
    case KA_SIN:
      p[-1].x = p[-1].y = p[-1].z = p[-1].w = sFSin(p[-1].x*sPI2F);
      break;
    case KA_COS:
      p[-1].x = p[-1].y = p[-1].z = p[-1].w = sFCos(p[-1].x*sPI2F);
      break;
    case KA_PULSE:
      fval = sFMod(p[-1].x,1.0);
      if(fval < 0.0f)
        fval += 1.0f;
      p[-1].x = p[-1].y = p[-1].z = p[-1].w = fval<0.5f ? 1.0f : 0.0f;
      break;
    case KA_RAMP:
      fval = sFMod(p[-1].x,1.0f);
      if(fval < 0.0f)
        fval += 1.0f;
      p[-1].x = p[-1].y = p[-1].z = p[-1].w = fval;
      break;
    case KA_CONSTV:
      *p = *(sVector *)bytecode;
      bytecode+=16;
      p++;
      break;
    case KA_CONSTC:
      p->x = (*bytecode++)/255.0f;
      p->y = (*bytecode++)/255.0f;
      p->z = (*bytecode++)/255.0f;
      p->w = (*bytecode++)/255.0f;
      p++;
      break;
    case KA_CONSTS:
      fval = *(sF32 *)bytecode;
      bytecode+=4;
      p->x = fval;
      p->y = fval;
      p->z = fval;
      p->w = fval;
      p++;
      break;
    case KA_SPLINE:
      i = *(sU16 *)bytecode;
      bytecode+=2;
      fval = p[-1].x;
      p[-1].Init();
#if !sPLAYER
      Splines[i]->Eval(fval,p[-1]);
#else
      (*Splines)[i].Eval(fval,p[-1]);
#endif
      break;
    case KA_EVENTSPLINE:
      fval = p[-1].x;
      p[-1].Init();
      if(EventSpline)
        EventSpline->Eval(fval,p[-1]);
      break;
    case KA_MATRIX:
      mat = (sMatrix *) &Var[KV_MATRIX_I];
      p[-1].Rotate4(*mat);
      break;
    case KA_NOISE:
      fval = sFMod(p[-1].x,1.0);
      if(fval < 0.0f)
        fval += 1.0f;
      i = fval * 65536;
      i = sPerlinPermute[(i >> 8) & 0xff] | (sPerlinPermute[i & 0xff] << 8);
      p[-1].x = p[-1].y = p[-1].z = p[-1].w = i / 65535.0f;
      break;
    case KA_EASE:
      {
        sF32 ei,eo,d;
        ei = *(sF32 *)bytecode; bytecode+=4;
        eo = *(sF32 *)bytecode; bytecode+=4;

        if(ei>0.5f) ei = 0.5f;
        if(eo>0.5f) eo = 0.5f;
        eo = 1-eo;
        d = eo-ei;
        if(d<0.1f)
        {
          eo += (0.1-d)/2;
          ei -= (0.1-d)/2;
          d = eo-ei;
        }

        sF32 a,c,s;
        s = 1/d;

        for(sInt i=0;i<4;i++)
        {
          fval = (&p[-1].x)[i];
          if(fval<0)                        // too small
          {
            fval = -ei*s/2;
          }
          else if(fval<ei)                  // ease in
          {
            a = -ei*s/2;
            c = s/(2*ei);
            fval = a+fval*fval*c;
          }
          else if(fval<eo)                  // linear
          {
            fval = (fval-ei)*s;
          }
          else if(fval<1)                   // ease out
          {
            a = -(1-eo)*s/2;
            c = s/(2*(1-eo));
            fval = 1-a-(1-fval)*(1-fval)*c;
          }
          else                              // too large
          {
            fval = 1+(1-eo)*s/2;
          }

          fval = (fval+(ei*s/2))/(1+ei*s/2+(1-eo)*s/2);

          (&p[-1].x)[i] = fval;
        }
      }
      break;
    case KA_TIMESHIFT:
      {
        sF32 t0 = (*bytecode++)/256.0f;
        sF32 t1 = (*bytecode++)/256.0f;
        for(sInt i=0;i<4;i++)
        {
          fval = (&p[-1].x)[i];
          if(fval<=t0) fval = 0;
          else if(fval>=t1) fval = 1;
          else fval = (fval-t0)/(t1-t0);
          (&p[-1].x)[i] = fval;
        }
      }
      break;
    case KA_POW:
      p--;
      p[-1].x = sFPow(p[-1].x,p->x);
      p[-1].y = sFPow(p[-1].y,p->y);
      p[-1].z = sFPow(p[-1].z,p->z);
      p[-1].w = sFPow(p[-1].w,p->w);
      break;
    case KA_LOG2:
      p[-1].x = sFLog(p[-1].x)*0.69314718055994530941723212145818f;
      p[-1].y = sFLog(p[-1].y)*0.69314718055994530941723212145818f;
      p[-1].z = sFLog(p[-1].z)*0.69314718055994530941723212145818f;
      p[-1].w = sFLog(p[-1].w)*0.69314718055994530941723212145818f;
      break;
    case KA_POW2:
      p[-1].x = sFPow(2,p[-1].x);
      p[-1].y = sFPow(2,p[-1].y);
      p[-1].z = sFPow(2,p[-1].z);
      p[-1].w = sFPow(2,p[-1].w);
      break;

    case KA_STOREVAR:
      index = *bytecode++;
      sVERIFY(index<KK_MAXVAR);
      p--;
      sVERIFY(Index<KK_MAXVARSAVE);
      Stack[Index] = Var[index];
      StackAdr[Index] = index;
      Index++;
      pop++;
      if(cmd&0x01) Var[index].x = p->x;
      if(cmd&0x02) Var[index].y = p->y;
      if(cmd&0x04) Var[index].z = p->z;
      if(cmd&0x08) Var[index].w = p->w;
      break;

    case KA_STOREPARAFLOAT:
    case KA_CHANGEPARAFLOAT:
      index = *bytecode++;
      sVERIFY(index<op->GetDataWords());
      p--;
      destfloat = (sF32 *)(op->DataAnim+index);
      cp = 0;

      if(cmd&0x01) { if(destfloat[0]!=p->x) cp=1; destfloat[0]=p->x; }
      if(cmd&0x02) { if(destfloat[1]!=p->y) cp=1; destfloat[1]=p->y; }
      if(cmd&0x04) { if(destfloat[2]!=p->z) cp=1; destfloat[2]=p->z; }
      if(cmd&0x08) { if(destfloat[3]!=p->w) cp=1; destfloat[3]=p->w; }
      if(cmd2==KA_CHANGEPARAFLOAT) changed|=cp;
      break;
    case KA_STOREPARAINT:
    case KA_CHANGEPARAINT:
      index = *bytecode++;
      sVERIFY(index<op->GetDataWords());
      p--;
      destint = (sS32 *)(op->DataAnim+index);
      cp = 0;
      if(cmd&0x01) { if(destint[0]!=p->x) cp=1; destint[0]=p->x; }
      if(cmd&0x02) { if(destint[1]!=p->y) cp=1; destint[1]=p->y; }
      if(cmd&0x04) { if(destint[2]!=p->z) cp=1; destint[2]=p->z; }
      if(cmd&0x08) { if(destint[3]!=p->w) cp=1; destint[3]=p->w; }
      if(cmd2==KA_CHANGEPARAINT) changed|=cp;
      break;
    case KA_STOREPARABYTE:
    case KA_CHANGEPARABYTE:
      index = *bytecode++;
      sVERIFY(index<op->GetDataWords());
      p--;
      destbyte = (sU8 *)(op->DataAnim+index);
      cp = 0;

      if(cmd&0x01) { bval=sRange<sInt>(p->x*255,255,0); if(destbyte[0]!=bval) cp=1; destbyte[0]=bval; }
      if(cmd&0x02) { bval=sRange<sInt>(p->y*255,255,0); if(destbyte[1]!=bval) cp=1; destbyte[1]=bval; }
      if(cmd&0x04) { bval=sRange<sInt>(p->z*255,255,0); if(destbyte[2]!=bval) cp=1; destbyte[2]=bval; }
      if(cmd&0x08) { bval=sRange<sInt>(p->w*255,255,0); if(destbyte[3]!=bval) cp=1; destbyte[3]=bval; }
      if(cmd2==KA_CHANGEPARABYTE) changed|=cp;
      break;


    default:
      sFatal("unknown opcode in animation bytecode");
      break;
    }

    sVERIFY(p>=stack+2);
    sVERIFY(p<stack+2+STACKMAX);

#if !sPLAYER
    if(debug)
      DebugOpOutput = p[-1];
#endif
  }
ende:

  sVERIFY(p==stack+2);
  if(changed)
  {
    op->WheelSpeed = 255;
    op->SkipCalc = 0;
#if !sPLAYER
    op->Change();
#endif
//    op->Changed = 1;
  }
  return pop;
}

sInt CalcCmdSize(sU8 *bytecode)
{
  static sU8 CmdSize[] =
  {
      0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x00-0x0f
      0, 0, 0, 0, 0, 0,16, 4, 4, 2, 0, 0, 0, 8, 2, 0, // 0x10-0x1f
  };
  sU8 j;

  j = *bytecode;
  if(j >= 0x80)
    return 1+1;
  else
    return 1+CmdSize[j];
}

sInt CalcCodeSize(sU8 *bytecode)
{
  sU8 *data,j;

  data = bytecode;
  do
  {
    j = *data;
    data += CalcCmdSize(data);
  }
  while(j!=KA_END);

  return data - bytecode;
}

void KEnvironment::Pop(sInt count)
{
  while(count>0)
  {
    Index--;
    count--;
    sVERIFY(Index>=0);
    Var[StackAdr[Index]] = Stack[Index];
  }
}

void *KEnvironment::GetInstImpl(KOp *me,sInt size)
{
  KInstanceMem *mem;
  if(NextInstance)
  {
    mem = NextInstance;
    sVERIFY(mem->Operator == me);
    sVERIFY(LinkInstance==0);
    mem->Reset = 0;
    NextInstance = mem->Next;
  }
  else
  {
    sVERIFY(LinkInstance);
    mem = (KInstanceMem *) new sU8[size];
    sSetMem(mem,0,size);
    *LinkInstance = mem;
    LinkInstance = &mem->Next;
    mem->Operator = me;
    mem->Reset = 1;
  }
  return mem;
}

void KEnvironment::SetMatrix(sMatrix &mat,sMatrix &save)
{
  sMatrix *var;

  var = (sMatrix *) &Var[KV_MATRIX_I];
  save = *var;
  *var = mat;
}

void KEnvironment::RestoreMatrix(sMatrix &save)
{
  sMatrix *var;

  var = (sMatrix *) &Var[KV_MATRIX_I];
  *var = save;
}

/****************************************************************************/
/***                                                                      ***/
/***   Splines                                                            ***/
/***                                                                      ***/
/****************************************************************************/

sF32 KSplineChannel::Eval(sF32 time,sInt inter)
{
  //sF32 val;
  sInt k;
  //sF32 *keyp[4];
  sF32 t,tdif,dif;
  sF32 d0,d1;

  if(!KeyCount)
    return 0;

  if(time<=Keys[0].Time)
    return Keys[0].Value;
  if(time>=Keys[KeyCount-1].Time)
    return Keys[KeyCount-1].Value;

  for(k=0;k<KeyCount-1;k++)
  {
    if(Keys[k+1].Time>time)
    {
      KSplineKey *key = &Keys[k];
      dif = key[1].Value - key->Value;
      tdif = key[1].Time - key->Time;
      t = (time - key->Time) / tdif;

      switch(inter)
      {
      case KSI_CUBIC:
        // this is equivalent to the sHermite call used before...
        // only about 180 bytes shorter ;)
        if(k == 0) // first key
          d0 = dif;
        else
          d0 = tdif * (key[1].Value - key[-1].Value) / (key[1].Time - key[-1].Time);

        if(k == KeyCount - 2) // last key
          d1 = dif;
        else
          d1 = tdif * (key[2].Value - key[0].Value) / (key[2].Time - key[0].Time);

        return (((d0 + d1 - 2*dif) * t + 3*dif - 2*d0 - d1)*t + d0)*t + key->Value;

        /*keyp[0] = (k == 0) ? 0 : &key[-1].Value;
        keyp[1] = &key[0].Value;
        keyp[2] = &key[1].Value;
        keyp[3] = (k == KeyCount - 2) ? 0 : &key[2].Value;

        sHermite(&val,keyp[0],keyp[1],keyp[2],keyp[3],1,t,0,0,0);
        return val;*/

      case KSI_LINEAR:
        return t * dif + key->Value;

      case KSI_STEP:
        return key->Value;
      }
    }
  }
  return 0;
}

void KSpline::Eval(sF32 time,sVector &v)
{
  sInt i;

  for(i=0;i<Count;i++)
    (&v.x)[i] = Channel[i].Eval(time,Interpolation);
}

/****************************************************************************/

void KEvent::Init()
{
  sSetMem(this,0,sizeof(*this));
  Matrix.Init();
  EndInterval = 1.0f;
}

void KEvent::Exit()
{
  if(FirstInstance)
  {
    FirstInstance->DeleteChain();
    FirstInstance = (KInstanceMem*)0xffffffff;
  }
}

void KEvent::Stop()
{
  Start = 0;
  End = 1;
}

/****************************************************************************/
/***                                                                      ***/
/***   Import                                                             ***/
/***                                                                      ***/
/****************************************************************************/

static KClass KClasses[256];

static sBool DistributeAnimR(KOp *start)
{
  sInt i;

  sBool flag = start->GetAnimCode()[0] != KA_END;

  for(i=0;i<start->GetInputCount();i++)
    if(start->GetInput(i))
      flag |= DistributeAnimR(start->GetInput(i));

  start->AnimFlag = flag;
  return flag;
}

#if sPLAYER

void KDoc::Init(const sU8 *&dataPtr)
{
  const sU8 *data;
  sChar *pack;
  sInt i,j,k,in,nOps,nSplines,typeByte,delta,nClasses,max;
  sInt eventCount;
  sU32 conv;
  KClass *cls;
  KEvent *event;
  KSpline *spline;
  KOp *op,*dest;
  sInt rootindex[MAX_OP_ROOT];
  sInt flags;

  data = dataPtr;

  flags = *(sU32 *)data; data+=4;
  BuzzTiming = (flags&4);
  if(flags&1) data+=32;
  SongSize = *(sU32 *)data; data+=4;
  SongData = (sU8 *)data; data+=sAlign(SongSize,4);
  if(flags&2)
  {
    SampleSize = *(sU32 *)data; data+=4;
    SampleData = (sU8 *)data; data+=sAlign(SampleSize,4);
  }
  SongBPM = *(sU32 *)data; data+=4;
  SongLength = *(sU32 *)data; data+=4;
  nOps = sReadShort(data);
  nSplines = sReadShort(data);
  for(i=0;i<MAX_OP_ROOT;i++)
    rootindex[i] = sReadShort(data);  

  Ops.Init(nOps); Ops.Count = nOps;
  Splines.Init(nSplines); Splines.Count = nSplines;

  cls = KClasses;
  nClasses = 0;

  while((conv = *((sU32 *) data)))
  {
    data += 4;
    cls->Convention = conv;
    i = *((sU16 *)data); data+=2;
    cls->Packing = (sChar *) data;
    while(*data++);

    KHandler *handler = KHandlers;
    while(handler->Id && handler->Id != i)
      handler++;

    if(!handler->Id)
      sFatal("Unknown operator class %02x in class list.",i);

#if sOP_PERFORMANCE
    sChar buffer[64];
    wsprintfA(buffer,"%02x->[%02x]\n",nClasses,i);
    OutputDebugStringA(buffer);
#endif
 
    cls->InitHandler = handler->InitHandler;
    cls->ExecHandler = handler->ExecHandler;
    cls++;
    nClasses++;
  }

  data += 4;

  // read op types+connections, build the tree
  for(i=0;i<nOps;i++)
  {
    typeByte = *data++;
    cls = KClasses + (typeByte & 0x7f);

    op = &Ops[i];
    sSetMem(op,0,sizeof(KOp));
    op->Init(cls->Convention);
    op->Command = typeByte & 0x7f;
    op->Convention = cls->Convention;
    op->InitHandler = cls->InitHandler;
    op->ExecHandler = cls->ExecHandler;
    op->WheelSpeed = (sInt) cls->Packing; // abuse, yes.
    op->OpId = i;
    op->AnimFlag = sTRUE; // first, assume that ops are all animated

    if(typeByte & 0x80)
      max = 1;
    else
      max = (cls->Convention & OPC_FLEXINPUT) ? *data++ : OPC_GETINPUT(cls->Convention);

    for(in=0;in<max;in++)
    {
      delta = (typeByte & 0x80) ? 0 : sReadShort(data);
      dest = &Ops[i - 1 - delta];

      op->AddInput(dest);
      dest->AddOutput(op);
    }
  }

  for(i=0;i<nOps;i++)
  {
    op = &Ops[i];
    for(in=0;in<OPC_GETLINK(op->Convention);in++)
    {
      delta = sReadShort(data);
      dest = delta ? &Ops[delta - 1] : 0;
      op->SetLink(in,dest);
      if(dest)
        dest->AddOutput(op);
    }
  }

  // read ops sorted by type
  for(i=0;i<nClasses;i++)
  {
    for(in=0;in<nOps;in++)
    {
      op = &Ops[in];
      if(op->Command == i)
      {
        // read data
        pack = (sChar *) op->WheelSpeed;

        for(j=0;pack[j];j++)
        {
          typeByte = pack[j];
          if(typeByte>='A' && typeByte<='Z')
            typeByte += 0x20;

          switch(typeByte)
          {
          case 'g': *op->GetEditPtrF(j) = sReadF16(data);                          break;
          case 'f': *op->GetEditPtrF(j) = sReadF24(data);                          break;
          case 'e': *op->GetEditPtrF(j) = sReadX16(data);                          break;
          case 'i': *op->GetEditPtrS(j) = *((sS32 *) data); data += 4;             break;
          case 's': *op->GetEditPtrS(j) = *((sS16 *) data); data += 2;             break;
          case 'b': *op->GetEditPtrU(j) = *data++;                                 break;
          case 'c': *op->GetEditPtrU(j) = *((sU32 *) data); data += 4;             break;
          case 'm': *op->GetEditPtrU(j) = *((sU32 *) data) & 0xffffff; data += 3;  break;
          }
        }

        for(j=0;j<OPC_GETSTRING(op->Convention);j++)
        {
          op->SetString(j,(sChar *)data);
          while(*data)
            data++;
          data++;
        }

        for(j=0;j<OPC_GETSPLINE(op->Convention);j++)
        {
          k = *((sU16 *) data); data += 2;
          op->SetSpline(j,k ? &Splines[k-1] : 0);
        }

        if(op->Convention&OPC_BLOB)
        {
          k = *((sInt *) data); data += 4;
          op->SetBlob(0,k);
        }
      }
    }
  }

  // read anim codes
  for(i=0;i<nOps;i++)
  {
    j = CalcCodeSize((sU8*)data);
    Ops[i].SetAnimCode((sU8*)data,j);
    data += j;
  }

  // read events
  Events.Init();
  eventCount = sReadShort(data);
  Events.AtLeast(eventCount);
  while(eventCount--)
  {
    event = Events.Add();
    event->Init();
    
    // read op
    event->Op = &Ops[sReadShort(data)];

    // read data
    event->Start = *((sInt *) data); data += 4;
    event->End = *((sInt *) data); data += 4;
    event->Velocity = sReadF24(data);
    event->Modulation = sReadF24(data);
    event->Select = *data++;

    event->Scale.x = sReadF24(data);
    event->Scale.y = sReadF24(data);
    event->Scale.z = sReadF24(data);
    event->Rotate.x = sReadF24(data);
    event->Rotate.y = sReadF24(data);
    event->Rotate.z = sReadF24(data);
    event->Translate.x = sReadF24(data);
    event->Translate.y = sReadF24(data);
    event->Translate.z = sReadF24(data);
    event->Color = *((sU32 *) data); data += 4;
    i = sReadShort(data);
    if(i)
      event->Spline = &Splines[i-1];
    else
      event->Spline = 0;
//    event->ReturnMatrix.Init();
    event->StartInterval = sReadF16(data);
    event->EndInterval = sReadF16(data);
    event->Flags = *data++;
  }

  // read splines
  for(i=0;i<nSplines;i++)
  {
    spline = &Splines[i];
    j = *data++;
    spline->Count = j & 7;
    spline->Interpolation = j >> 3;

    for(j=0;j<spline->Count;j++)
    {
      spline->Channel[j].KeyCount = k = sReadShort(data);
      if(k)
        spline->Channel[j].Keys = new KSplineKey[k];
      else
        spline->Channel[j].Keys = 0;
      for(k=0;k<spline->Channel[j].KeyCount;k++)
      {
        spline->Channel[j].Keys[k].Time = *data++ / 255.0f;
        spline->Channel[j].Keys[k].Value = sReadF16(data);
      }
    }
  }

  // read blobs
  for(i=0;i<nOps;i++)
  {
    j = Ops[i].GetBlobSize();
    if(j)
    {
      Ops[i].SetBlobData(data);
      data += j;
    }
  }

  // distribute anim flag
  DistributeAnimR(&Ops[Ops.Count-1]);

  for(i=0;i<MAX_OP_ROOT;i++)
  {
    if(rootindex[i]<nOps)
      RootOps[i] = &Ops[rootindex[i]];
    else
      RootOps[i] = 0;
  }
  CurrentRoot = 0;

  dataPtr = data;
}

#endif

void KDoc::Exit()
{
  sInt i,j;
  KOp *op;

  for(i=0;i<Ops.Count;i++)
  {
    op = &Ops[i];
#if sPLAYER
    if(op->Cache && !op->CacheFreed)
      op->Cache->Release();
#else
    if(op->Cache)
      op->Cache->Release();
#endif

    op->Exit();
//    if(op->Update)
//      op->Update->Release();
  }

  for(i=0;i<Splines.Count;i++)
  {
    for(j=0;j<Splines[i].Count;j++)
      delete[] Splines[i].Channel[j].Keys;
  }

  Ops.Exit();
  Events.Exit();
  Splines.Exit();
}

void KDoc::Precalc(KEnvironment *kenv)
{
//  KOp *op;

#if sOP_PERFORMANCE
  for(sInt i=0;i<256;i++)
    OpPerf[i] = 0;
#endif

/*
  for(i=0;i<Ops.Count;i++)
  {
    op = &Ops[i];

#if sOP_PERFORMANCE
    sU32 stime = sSystem->GetTime();
#endif
    op->Calc(kenv,KCF_ROOT|KCF_NEED);
#if sOP_PERFORMANCE
    perf[op->Command] += sSystem->GetTime() - stime;
#endif

#if sPLAYER
    for(sInt j=0;j<op->GetInputCount();j++)
      op->GetInput(j)->CalcUsed();

    for(sInt j=0;j<op->GetLinkMax();j++)
      op->GetLink(j)->CalcUsed();

    sSystem->Progress(i,Ops.Count);
#endif
  }
*/

  PrecalcCount = 0;
  PrecalcMax = CountOps(RootOps[CurrentRoot]);
  RootOps[CurrentRoot]->Calc(kenv,KCF_ROOT|KCF_NEED|KCF_PRECALC);

#if sOP_PERFORMANCE
  sInt perft = 0;

  for(sInt i=0;i<256;i++)
  {
    if(OpPerf[i])
    {
      sChar buffer[64];
      wsprintfA(buffer,"%02x: %d ms\n",i,OpPerf[i]);
      OutputDebugStringA(buffer);
      perft += OpPerf[i];
    }
  }
  sChar buffer[64];
  wsprintfA(buffer,"%d ms total (ops)\n",perft);
  OutputDebugStringA(buffer);
#endif
}

void KDoc::AddEvents(KEnvironment *kenv)
{
  sInt i;

  for(i=0;i<Events.Count;i++)
    kenv->AddStaticEvent(&Events[i]);
}

static sInt CountOpsR(KOp *op)
{
  sInt count = 1;
  op->Tag = 1;

  for(sInt i=0;i<op->GetInputCount();i++)
  {
    KOp *in = op->GetInput(i);
    if(in && !in->Tag)
      count += CountOpsR(in);
  }

  for(sInt i=0;i<op->GetLinkMax();i++)
  {
    KOp *link = op->GetLink(i);
    if(link && !link->Tag)
      count += CountOpsR(link);
  }
  
  return count;
}

sInt KDoc::CountOps(KOp *start)
{
  for(sInt i=0;i<Ops.Count;i++)
    Ops[i].Tag = Ops[i].Cache && !Ops[i].Changed;

  return CountOpsR(start);
}

/****************************************************************************/
/***                                                                      ***/
/***   Operator. Mostly used by Tool                                      ***/
/***                                                                      ***/
/****************************************************************************/

void KOp::Init(sU32 convention)
{
  sInt size;
  sU32 *mem;
  sInt i;

  DataMax   = OPC_GETDATA(convention);
  InputMax  = OPC_GETINPUT(convention);
  LinkMax   = OPC_GETLINK(convention);
  StringMax = OPC_GETSTRING(convention);
  SplineMax = OPC_GETSPLINE(convention);
  AnimMax = 1;

#if sPLAYER
  if(convention & OPC_VARIABLEINPUT)
    InputMax = 255;
  OutputMax = 0;
#else
  OutputMax = 4;
#endif

  size = DataMax*2+InputMax+LinkMax+OutputMax+StringMax*2+SplineMax+AnimMax;
  mem = Memory = new sU32[size];
  sSetMem(mem,0,size*4);

  Input = (KOp**) mem; mem+=InputMax;
  Link = (KOp**) mem; mem+=LinkMax;
  Output = (KOp**) mem; mem+=OutputMax;
  String = (sChar**)mem; mem+=StringMax;
  Spline = (KSpline**)mem; mem+=SplineMax;
  DataEdit = mem; mem+=DataMax;
  DataAnim = mem; mem+=DataMax;
  for(i=0;i<StringMax;i++)
  {
    String[i] = (sChar *)mem;
    String[i][0] = 0;
    mem++;
  }
  AnimCode = (sU8 *)mem; mem+=AnimMax; *AnimCode = KA_END;
  InputCount = 0;
  OutputCount = 0;
  BlobData = 0;
  BlobSize = 0;
#if !sPLAYER
  Blocked = 0;
#endif
}

void KOp::Exit()
{
#if !sPLAYER
  KMemoryManager->Remove(this);
  if(BlobData)
    delete[] BlobData;
#endif

  if(Memory)
  {
    delete[] Memory;
    Memory = 0;
  }
}

/****************************************************************************/

#if !sPLAYER

void KOp::ReAlloc(sInt inmax,sInt outmax)
{
  sInt size,i;
  sU32 *mem;
  sU32 *oldmem,*olddata,*oldanim;
  KOp **oldin,**oldlink,**oldout;
  sChar **oldstring;
  KSpline **oldspline;
  sU8 *oldcode;
  
// remember old pointers

  oldmem = Memory;
  olddata = DataEdit;
  oldanim = DataAnim;
  oldin = Input;
  oldlink = Link;
  oldout = Output;
  oldstring = String;
  oldspline = Spline;
  oldcode = AnimCode;

// set new structure

  sVERIFY(inmax>=InputCount);
  sVERIFY(outmax>=OutputCount);
  InputMax = inmax;
  OutputMax = outmax;

// calculate size

  size = DataMax*2+InputMax+LinkMax+OutputMax+StringMax+SplineMax+AnimMax;
  for(i=0;i<StringMax;i++)
    size += sAlign<sInt>(sGetStringLen(oldstring[i])+1,4)/4;

// distribute new memory

  mem = Memory = new sU32[size];
  Input = (KOp**) mem; mem+=InputMax;
  Link = (KOp**) mem; mem+=LinkMax;
  Output = (KOp**) mem; mem+=OutputMax;
  String = (sChar**)mem; mem+=StringMax;
  Spline = (KSpline**)mem; mem+=SplineMax;
  DataEdit = mem; mem+=DataMax;
  DataAnim = mem; mem+=DataMax;
  AnimCode = (sU8*)mem; mem+=AnimMax;

// copy old data

  sCopyMem(Input,oldin,InputCount*4);
  sCopyMem(Link,oldlink,LinkMax*4);
  sCopyMem(Output,oldout,OutputCount*4);
  sCopyMem(Spline,oldspline,SplineMax*4);
  sCopyMem(DataEdit,olddata,DataMax*4);
  sCopyMem(DataAnim,oldanim,DataMax*4);
  sCopyMem(AnimCode,oldcode,AnimMax*4);

  for(i=0;i<StringMax;i++)
  {
    String[i] = (sChar *)mem;
    sCopyMem(String[i],oldstring[i],sAlign(sGetStringLen(oldstring[i])+1,4));
    mem += sAlign(sGetStringLen(String[i])+1,4)/4;
  }

// done

  sVERIFY(Memory+size==mem);
  delete[] oldmem;
}

void KOp::SetString(sInt i,sChar *c)
{
  sVERIFY(i<StringMax); 
  String[i] = c;
  ReAlloc(InputMax,OutputMax);
}

void KOp::SetSpline(sInt i,KSpline *spline)
{
  sVERIFY(i<SplineMax);
  Spline[i] = spline;
}

void KOp::SetAnimCode(sU8 *code,sInt size)
{
  sInt max;
  max = (size+3)/4;
  if(AnimMax==max)
  {
    sCopyMem(AnimCode,code,max*4);
  }
  else
  {
    AnimCode = code;
    AnimMax = max;
    ReAlloc(InputMax,OutputMax);
  }
}

void KOp::AddInput(KOp *op) 
{
  if(InputCount>=InputMax)
    ReAlloc(InputMax+8,OutputMax);
  Input[InputCount++]=op;
}

void KOp::AddOutput(KOp *op) 
{
  if(OutputCount>=OutputMax)
    ReAlloc(InputMax,OutputMax+8);
  Output[OutputCount++]=op;
}

void KOp::CopyEditToAnim()
{
  sCopyMem(DataAnim,DataEdit,DataMax*4);
}

static void RecursiveRelease(KOp *op)
{
  KObject *cache = op->Cache;

  // go through all output ops
  sInt count = op->GetOutputCount();
  for(sInt i=0;i<count;i++)
  {
    KOp *other = op->GetOutput(i);
    if(other->Cache == cache)
      RecursiveRelease(other);
  }

  // actually release
  sRelease(op->Cache);
}

void KOp::Uncache()
{
  /*sInt i,max;
  KOp *op;*/

  KMemoryManager->Remove(this);
  if(Cache && (Cache->ClassId==KC_BITMAP || Cache->ClassId==KC_MESH || Cache->ClassId==KC_MINMESH))
  {
//    if(Cache->ClassId==KC_MESH && ((GenMesh*)Cache)->StoreMode)
//      return;

    /*max = GetOutputCount();
    for(i=0;i<max;i++)
    {
      op = GetOutput(i);
      if(op->Result!=KC_ANY && op->Result!=Cache->ClassId)
        return;
    }

    Cache->Release();
    Cache = 0;*/
    if(!Cache->Strip())
    {
      // release the cache
      RecursiveRelease(this);

      //sRelease(Cache);
    }
  }
}

#endif

void KOp::SetBlob(const sU8 *data,sInt size)
{
#if !sPLAYER
  if(BlobData)
    delete[] BlobData;
#endif
  BlobData = data;
  BlobSize = size;
}


/****************************************************************************/
/***                                                                      ***/
/***  Memory Management (Tool only)                                       ***/
/***                                                                      ***/
/****************************************************************************/

#if !sPLAYER

static sInt GetObjectSize(KObject *obj)
{
  GenMesh *mesh;
  GenMinMesh *mmesh;
  sInt size;
  switch(obj->ClassId)
  {
  case KC_BITMAP:
    return ((GenBitmap *)obj)->Size * 8;
  case KC_MESH:
    mesh = (GenMesh *)obj;
    size = 0;
    size += mesh->Vert.Alloc * sizeof(GenMeshVert);
    size += mesh->Edge.Alloc * sizeof(GenMeshEdge);
    size += mesh->Face.Alloc * sizeof(GenMeshFace);
    size += mesh->Mtrl.Alloc * sizeof(GenMeshMtrl);
    size += mesh->Coll.Alloc * sizeof(GenMeshColl);

    size += mesh->VertAlloc * mesh->VertSize() * 16;
    size += mesh->KeyCount * mesh->CurveCount * 4;

    size += mesh->BoneMatrix.Alloc * sizeof(GenMeshMatrix);
    size += mesh->BoneCurve.Alloc * sizeof(GenMeshCurve);

    // it is not possible to count data in "GenMeshJobs" because
    // those are not initialized when this is executed.

    return size;
  case KC_MINMESH:
    mmesh = (GenMinMesh *)obj;
    size = 0;
    size += mmesh->Vertices.Alloc * sizeof(GenMinVert);
    size += mmesh->Faces.Alloc * sizeof(GenMinFace);
    size += mmesh->Clusters.Alloc * sizeof(GenMinCluster);
    // this is missing animation...
    return size;

  default:
    return 0;
  }
}

KMemoryManager_::KMemoryManager_()
{
  sInt i;

  for(i=0;i<KC_COUNT;i++)
  {
    Objects[i].Size = 0;
    Objects[i].Budget = 0;
    Objects[i].Ops.Init();
  }
}

KMemoryManager_::~KMemoryManager_()
{
  sInt i;

  for(i=0;i<KC_COUNT;i++)
    Objects[i].Ops.Exit();
}

sInt KMemoryManager_::GetBudget(sInt type)
{
  sVERIFY(type < KC_COUNT);
  return Objects[type].Budget;
}

void KMemoryManager_::SetBudget(sInt type,sInt size)
{
  sVERIFY(type < KC_COUNT);
  Objects[type].Budget = size;
}

sInt KMemoryManager_::GetSize(sInt type)
{
  sVERIFY(type < KC_COUNT);
  return Objects[type].Size;
}

sInt KMemoryManager_::GetCount(sInt type)
{
  sVERIFY(type < KC_COUNT);
  return Objects[type].Ops.Count;
}

static sInt MapClassId(sInt id)
{
  if(id == KC_MINMESH)
    return KC_MESH;

  return id;
}

void KMemoryManager_::Use(KOp *op)
{
  KObjectList *obj;
  sInt i,j,size,oldSize;

  op = SkipNops(op);
  if(!op)
    return;

  if(op->Cache && !op->Cache->IsStripped())
  {
    sVERIFY(op->Cache->ClassId >= 0 && op->Cache->ClassId < KC_COUNT);
    obj = &Objects[MapClassId(op->Cache->ClassId)];

    // find this op in the op list
    for(i=0;i<obj->Ops.Count && obj->Ops[i]!=op;i++);

    // add an item if necessary
    if(i == obj->Ops.Count)
    {
      obj->Ops.Add();
      oldSize = 0;
    }
    else
      oldSize = op->CacheSize;

    // move to front
    for(j=i-1;j>=0;j--)
      obj->Ops[j+1] = obj->Ops[j];

    obj->Ops[0] = op;
    size = GetObjectSize(op->Cache);
    op->CacheSize = size;
    obj->Size += size - oldSize;
  }
}

void KMemoryManager_::Remove(KOp *op)
{
  KObjectList *obj;
  sInt i;

  if(WouldSkip(op))
    return;

  if(op->Cache && !op->Cache->IsStripped())
  {
    sVERIFY(op->Cache->ClassId >= 0 && op->Cache->ClassId < KC_COUNT);
    obj = &Objects[MapClassId(op->Cache->ClassId)];

    // find op
    i = obj->Ops.Count - 1;
    while(i>=0)
    {
      if(obj->Ops[i] == op)
        break;
      else
        i--;
    }
    
    // remove
    if(i >= 0)
    {
      sVERIFY(obj->Ops[i] == op);

      obj->Size -= op->CacheSize;
      while(++i < obj->Ops.Count)
        obj->Ops[i-1] = obj->Ops[i];

      obj->Ops.Count--;
    }
  }
}

void KMemoryManager_::Manage()
{
  sInt cls,i;
  KObjectList *obj;
  static sU8 validateCounter = 0;
  if(!++validateCounter)
    Validate();

  for(cls=0;cls<KC_COUNT;cls++)
  {
    obj = &Objects[cls];
    while(obj->Size > obj->Budget)
    {
      // find first non-blocked object
      i = obj->Ops.Count-1;
      while(i>=0)
      {
        sVERIFY(obj->Ops[i]->Cache && !obj->Ops[i]->Cache->IsStripped());

        if(!obj->Ops[i]->Blocked)
          break;
        else
          i--;
      }

      if(i >= 0) // uncache if available
        obj->Ops[i]->Uncache();
      else // no non-blocked ops to free, we can't do much
      {
        sDPrintF("KMemoryManager: too small budget in class %d\n",cls);
        break;
      }
    }
  }
}

void KMemoryManager_::Validate()
{
  for(sInt cls=0;cls<KC_COUNT;cls++)
  {
    KObjectList *obj = &Objects[cls];
    for(sInt i=0;i<obj->Ops.Count;i++)
    {
      KOp *op = obj->Ops[i];
      
      sVERIFY(op->Cache && op->Cache->ClassId >= 0 && op->Cache->ClassId < KC_COUNT);
      sVERIFY(!op->Cache->IsStripped());
    }
  }
}

KMemoryManager_ *KMemoryManager = 0;

void MemManagerInit()
{
  KMemoryManager = new KMemoryManager_;
  KMemoryManager->SetBudget(KC_BITMAP,192*1024*1024); // just a default value
  KMemoryManager->SetBudget(KC_MESH  ,96*1024*1024); // just a default value
}

void MemManagerExit()
{
  delete KMemoryManager;
  KMemoryManager = 0;
}

/****************************************************************************/

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Classless Operators                                                ***/
/***                                                                      ***/
/****************************************************************************/

#pragma lekktor(off)

KObject * __stdcall Init_Misc_Load(KObject *o)
{
  return o;
}

KObject * __stdcall Init_Misc_Nop(KOp *op)
{
  return op->GetInput(0)->Cache;
}

KObject * __stdcall Init_Misc_If(KOp *op)
{
  if(op->GetInput(1) && op->GetInput(1)->Cache)
    op->GetInput(1)->Cache->Release();
  return op->GetInput(0)->Cache;
}

KObject * __stdcall Init_Misc_Event(KObject *o,sF32 duration)
{
  return o;
}

KObject * __stdcall Init_Misc_EventTime(KObject *o)
{
  return o;
}

KObject * __stdcall Init_Misc_Trigger(KObject *o,KOp *event,
  sF32 trigger,sF32 tresh,sF32 delay,sF32 duration,sInt flags,
  sF32 mod,sF32 vel,sInt sel,sF323 s,sF323 r,sF323 t,sU32 col,KSpline *spline)
{
  return o;
}

void __stdcall Exec_Misc_Nop(KOp *op,KEnvironment *kenv)
{
  op->ExecInputs(kenv);
}

void __stdcall Exec_Misc_Load(KOp *op,KEnvironment *kenv)
{
  sVERIFY(op->GetLink(0));

  op->GetLink(0)->Exec(kenv);
}

/****************************************************************************/

void __stdcall Exec_Misc_Time(KOp *op,KEnvironment *kenv)
{
  sVERIFY(op->GetInput(0));
  if(op->FirstEvent)
    op->GetInput(0)->ExecEvent(kenv,op->FirstEvent,0);
}

void __stdcall Exec_Misc_Event(KOp *op,KEnvironment *kenv,sF32 duration)
{
  KEvent *ev;
  
  sVERIFY(op->GetInput(0));
  ev = op->FirstEvent;
  while(ev)
  {
    if(ev->Start == ev->End)
    {
      ev->Start = kenv->BeatTime;
      ev->End = ev->Start + sFtol(duration * 0x10000);
    }
    op->GetInput(0)->ExecEvent(kenv,ev,0);
    ev = ev->NextOp;
  }
}

void __stdcall Exec_Misc_EventTime(KOp *op,KEnvironment *kenv)
{
  struct EventTimeMem : KInstanceMem
  {
    sInt EndTime;
  };
  KEvent *ev;
  EventTimeMem *mem;
  
  sVERIFY(op->GetInput(0));
  mem = kenv->GetInst<EventTimeMem>(op);
  if(mem->Reset)
    mem->EndTime = 0x7fffffff;
  ev = op->FirstEvent;
  if(ev)
  {
    op->GetInput(0)->ExecEvent(kenv,ev,0);
    mem->EndTime = ev->End;
  }
  else
  {
    sF32 time = kenv->BeatTime > mem->EndTime ? 1 : 0;
    sVector save = kenv->Var[KV_TIME];
    kenv->Var[KV_TIME].Init(time,time,time,time);
    op->ExecInputs(kenv);
    kenv->Var[KV_TIME] = save;
  }
}

/****************************************************************************/

struct TriggerMem : KInstanceMem
{
  sInt Active;
  sInt Time;
};

void __stdcall Exec_Misc_Trigger(KOp *op,KEnvironment *kenv,
  sF32 trigger,sF32 tresh,sF32 delay,sF32 duration,sInt flags,
  sF32 vel,sF32 mod,sInt sel,sF323 s,sF323 r,sF323 t,sU32 col,KSpline *spline)
{
  KEvent *ev;
  KOp *eop;
  TriggerMem *mem;
  sBool doit;

  eop = op->GetLink(0);

  if(eop && eop->ExecHandler == Exec_Misc_Event)
  {
    mem = kenv->GetInst<TriggerMem>(op);
    if(mem->Reset)
    {
      mem->Active = 1;
      mem->Time = kenv->BeatTime;
    }

//    sDPrintF("%f %f %08x %08x (%d)\n",trigger,tresh,kenv->BeatTime,mem->Time,flags);

    doit = (trigger>tresh);
    if(flags&1)
      doit = !doit;
    if(!doit)
      mem->Time = kenv->BeatTime;
    if((flags&2) && mem->Active && kenv->BeatTime>=mem->Time)
      mem->Active = 0;

    if(doit && !mem->Active)
    {
      ev = new KEvent;
      ev->Init();

      ev->Op = eop;
      ev->Start = mem->Time;
      ev->End = ev->Start+duration*0x10000;
      ev->Dynamic = 1;

      ev->Velocity = vel;
      ev->Modulation = mod;
      ev->Select = sel;
      ev->Scale.Init(s.x,s.y,s.z,0);
      ev->Rotate.Init(r.x,r.y,r.z,0);
      ev->Translate.Init(t.x,t.y,t.z,0);
      if(flags&4)
      {
        ev->Translate = kenv->ExecStack.Top().l;
        ev->Translate.w = 0;
      }
      kenv->AddDynamicEvent(ev);
//      sVERIFY(ev->Active == 1);

//      sDPrintF("-> %08x %08x\n",ev->Start,ev->End);

      mem->Active = 1;
      mem->Time += delay*0x10000;
    }
    if(!doit)
      mem->Active = 0;
  }

  op->ExecInput(kenv,0);
}

/****************************************************************************/

struct DelayMem : KInstanceMem
{
  sInt Phase;     // 0 = off, 1=rise, 2=hold, 3=decline
  sF32 Time;
};

void __stdcall Exec_Misc_Delay(KOp *op,KEnvironment *kenv,sInt flags,sInt sw,sF32 tresh,sF32 hyst,sF32 time0,sF32 time1,sF32 time2)
{
  sF32 val;
  sF32 delta;
  DelayMem *mem;

  mem = kenv->GetInst<DelayMem>(op);
  if(mem->Reset)
  {
    mem->Phase = 0;
    mem->Time = 0;
  }

  if(flags&0xc0)
  {
    val = kenv->Var[(flags&0xc0)>>6].x;
  }
  else
  {
    val = 0;
    tresh = 0.5f;
    hyst = 0.0f;
    if(kenv->Game->Switches) 
      val = kenv->Game->Switches[sw];
  }

  if(mem->Phase==2 && val<tresh-hyst)
  {
    mem->Phase = 3;
    mem->Time = 0;
  }
  if(mem->Phase==0 && val>tresh+hyst)
  {
    mem->Phase = 1;
    mem->Time = 0;
  }

  delta = (kenv->CurrentTime-kenv->LastTime)/1000.0f;
  if(delta<0) delta =0;
  switch(mem->Phase)
  {
  case 1:
    if(time0>0)
      mem->Time += delta/time0;
    else
      mem->Time = 1.0f;
    if(mem->Time>=1.0f)
    {
      mem->Phase = 2;
      mem->Time = 0;
    }
    break;
  case 2:
    if(time1>0)
    {
      mem->Time += delta/time1;
      if(mem->Time>1.0f)
        mem->Time = mem->Time -1;
      if(mem->Time>1.0f)
        mem->Time = 0;
    }
    break;
  case 3:
    if(time2>0)
      mem->Time += delta/time2;
    else
      mem->Time = 1.0f;
    if(mem->Time>=1.0f)
    {
      mem->Phase = 0;
      mem->Time = 0;
    }
    break;
  }

  val = 0;
  if(mem->Phase==1) val = mem->Time;
  if(mem->Phase==2) val = 1;
  switch(flags&0x0f)
  {
  case 0:
    if(mem->Phase==3) val = 1-mem->Time;
    break;
  case 1:
    if(mem->Phase==3) val = 1+mem->Time;
    val = val*0.5f;
    break;
  case 2:
    if(mem->Phase==2) val = 1+mem->Time*2;
    if(mem->Phase==3) val = 3+mem->Time;
    val = val*0.25f;
    break;
  }

  kenv->Var[(flags&0x30)>>4].Init(val,val,val,val);

  op->ExecInput(kenv,0);
}

/****************************************************************************/

void __stdcall Exec_Misc_If(KOp *op,KEnvironment *kenv,sInt sw,sInt val)
{
  KOp *child;

  sVERIFY(op->GetInputCount()>=1);
  if(kenv->Game->Switches[sw]==val)
  {
    child = op->GetInput(0);
  }
  else
  {
    if(op->GetInputCount()<2)
      return;
    child = op->GetInput(1);
  }
  child->ExecWithNewMem(kenv,&child->SceneMemLink);
}

/****************************************************************************/

void __stdcall Exec_Misc_SetIf(KOp *op,KEnvironment *kenv,sInt sw,sInt val,sInt flags,sInt osw)
{
  sVector v;
  sInt var;
  sInt compare;

  var = flags&15;

  compare = kenv->Game->Switches[sw]-val;
  if(flags&0x20)
    compare = compare<0;
  compare = compare?0:1;
  if(flags&0x10)
    compare = 1-compare;

  switch(var)
  {
  case 0:
  case 1:
  case 2:
  case 3:
    v = kenv->Var[var];
    kenv->Var[var].Init(compare,compare,compare,compare);
    op->ExecInputs(kenv);
    kenv->Var[var] = v;
    break;
  case 4:
    kenv->Game->Switches[osw] = compare;
    op->ExecInputs(kenv);
    break;
  case 5:
  case 6:
    if(compare)
      kenv->Game->Switches[osw] = 6-var;
    op->ExecInputs(kenv);
    break;
  }
}

/****************************************************************************/

void __stdcall Exec_Misc_State(KOp *op,KEnvironment *kenv,sInt sw,sInt val,sInt cond,sInt condsw,sInt condval)
{
  sInt ok;

  ok = 0;
  switch(cond)
  {
  case 4:
    if(kenv->Game->LastKey==sKEY_UP)
      ok = 1;
    break;
  case 5:
    if(kenv->Game->LastKey==sKEY_DOWN)
      ok = 1;
    break;
  case 6:
    if(kenv->Game->LastKey==sKEY_LEFT)
      ok = 1;
    break;
  case 7:
    if(kenv->Game->LastKey==sKEY_RIGHT)
      ok = 1;
    break;
  case 0:
    ok = 1;
    break;
  case 1:
    if(kenv->Game->LastKey==' ')
      ok = 1;
    break;
  case 2:
    if(kenv->Game->LastKey==sKEY_ENTER || kenv->Game->LastKey==sKEY_MOUSEL)
      ok = 1;
    break;
  case 3:
    if(kenv->Game->LastKey==sKEY_BACKSPACE || kenv->Game->LastKey==sKEY_ESCAPE)
      ok = 1;
    break;
  }

  if(ok && kenv->Game->Switches[condsw]==condval && sw!=0)
  {
    kenv->Game->Switches[sw] = val;
    if(cond!=0)
      kenv->Game->LastKey = 0;
  }

  op->ExecInputs(kenv);
}

void __stdcall Exec_Misc_Menu(KOp *op,KEnvironment *kenv,sInt sw,sInt max,sInt flags)
{
  sInt i;

  i = 0;
  if( (flags&1) && kenv->Game->LastKey==sKEY_UP   ) i = -1;
  if( (flags&1) && kenv->Game->LastKey==sKEY_DOWN ) i = 1;
  if(!(flags&1) && kenv->Game->LastKey==sKEY_LEFT ) i = -1;
  if(!(flags&1) && kenv->Game->LastKey==sKEY_RIGHT) i = 1;

  if(i!=0)
  {
    kenv->Game->Switches[sw] = sRange<sInt>(kenv->Game->Switches[sw]+i,max-1,0);
    kenv->Game->LastKey = 0;
  }

  op->ExecInputs(kenv);
}

struct PlaySampleMem : public KInstanceMem
{
  sInt On;
  sInt Retrigger;
  sInt OldValue;
  sVector OldPos;
  sInt Buffer3D;
};

#if sLINK_KKRIEGER

void __stdcall Exec_Misc_PlaySample(KOp *op,KEnvironment *kenv,sInt sw,sInt val,sF32 tresh,sInt sample,sF32 volume,sF32 retrigger,sF32 halfrange)
{
  PlaySampleMem *mem;
  sInt ok,buf;
  sVector v;

  mem = kenv->GetInst<PlaySampleMem>(op);
  if(mem->Reset)
  {
    mem->Retrigger = -1;
    mem->OldValue = kenv->Game->Switches[sw];
    mem->OldPos.Init4(0,0,0,0);
    mem->Buffer3D = -1;
  }

  ok = (kenv->Game->Switches[sw]==val && tresh>0);
  if(val==-1)
    ok = kenv->Game->Switches[sw]!=mem->OldValue;
  mem->OldValue = kenv->Game->Switches[sw];
  if(mem->Retrigger>=0)
  {
    mem->Retrigger -= kenv->TimeDelta;
    if(mem->Retrigger<0)
      mem->On = 0;
  }
  if(ok && !mem->On)
  {
#if !sPLAYER
    if(GenOverlayManager->SoundEnable)
#endif
    {
      buf = sSystem->SamplePlay(sample,sFPow(10.0f,-volume/20.0f),0);
      if(retrigger>0)
        mem->Retrigger = sFtol(retrigger*1000);

      if(halfrange)
        mem->Buffer3D = buf;
    }
  }
  
  // update 3d parameters
  if(mem->Buffer3D != -1)
  {
    const sMatrix &matrix = kenv->ExecStack.Top();

    v.Sub3(matrix.l,mem->OldPos);
    v.Scale3(1000.0f / (kenv->TimeDelta + 1.0f));
    sSystem->Sample3DParam(sample,mem->Buffer3D,matrix.l,v,1.0f,halfrange*8.0f);
    mem->OldPos = matrix.l;
  }

  mem->On = ok;
  op->ExecInputs(kenv);
}

#endif

#pragma lekktor(on)

/****************************************************************************/


GenDemo::GenDemo()
{
  ClassId = KC_DEMO;
}

GenDemo::~GenDemo()
{
}

KObject * __stdcall Init_Misc_Demo(KOp *op)
{
  sInt i;
  KObject *obj;

  for(i=0;i<op->GetInputCount();i++)
  {
    obj = op->GetInput(i)->Cache;
    if(obj->ClassId!=KC_DEMO && obj->ClassId!=KC_IPP)
      return 0;
  }

  for(i=0;i<op->GetInputCount();i++)
  {
    obj = op->GetInput(i)->Cache;
    obj->Release();
  }
  return new GenDemo;
}

void __stdcall Exec_Misc_Demo(KOp *op,KEnvironment *kenv)
{
  sInt i;
  KOp *in;

  for(i=0;i<op->GetInputCount();i++)
  {
    in = op->GetInput(i);
    if(in->Cache->ClassId==KC_DEMO)
    {
      op->ExecInput(kenv,i);
    }
    else if(in->Cache->ClassId==KC_IPP)
    {
      GenOverlayManager->Reset(kenv);

      op->ExecInput(kenv,i);
      if(GenOverlayManager->LastOutput!=0)
      {
        GenOverlayManager->Copy(GenOverlayManager->LastOutput,
          GenOverlayManager->Alloc(0,GENOVER_RTSIZES,1),0,0xffffffff,1);    // owner is 0 because noone should find this!
      }
    }
  }
}

/****************************************************************************/

KObject * __stdcall Init_Misc_MasterCam(sF323 rot,sF323 pos,sF32 farclip,sF32 nearclip,sF32 centerx,sF32 centery,sF32 zoomx,sF32 zoomy)
{
  return new GenDemo;
}

void __stdcall Exec_Misc_MasterCam(KOp *op,KEnvironment *kenv,sF323 rot,sF323 pos,sF32 farclip,sF32 nearclip,sF32 centerx,sF32 centery,sF32 zoomx,sF32 zoomy)
{
  sMaterialEnv env;

  // CurrentCam is the way to tell camera details
  env.Init();

#if !sPLAYER
  if(!GenOverlayManager->FreeCamera)
#endif
  {
    env.FarClip = farclip;
    env.NearClip = nearclip;
    env.CenterX = centerx;
    env.CenterY = centery;
    env.ZoomX = zoomx;
    env.ZoomY = zoomy;
    env.CameraSpace.InitEulerPI2(&rot.x);
    env.CameraSpace.l.Init3(pos.x,pos.y,pos.z);

    kenv->GameCam = env;
  }
}

/****************************************************************************/
/****************************************************************************/


