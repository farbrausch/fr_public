/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "base/types.hpp"
#include "util/taskscheduler.hpp"
#include "base/graphics.hpp"
#include "util/shaders.hpp"

static sU32 StatSpin;
static sU32 StatLock;
static sInt SpinDummy=1;

/****************************************************************************/
/***                                                                      ***/
/***   The Singleton                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sStsManager *sSched;
sStsPerfMon *sSchedMon;

static void sInitSts()
{
  sVERIFY(sSched==0)
  sSched = new sStsManager(128*1024,512);
}

static void sExitSts()
{
  sDelete(sSched);
}

void sAddSched()
{
  static sInt once=1;
  if(once)
  {
    once = 0;
    sAddSubsystem(L"StealingTaskScheduler",0x80,sInitSts,sExitSts);
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   A spin-blocking lock. Non-recursive (same thread can't pass twice) ***/
/***                                                                      ***/
/****************************************************************************/

void sSpin()
{
  // just make sure the compiler is not smart enough to optimize it away.
  // and don't do to many memory accesses!

  volatile int SpinDummy2 = SpinDummy;
  sInt x = SpinDummy2;

  for(sInt i=0;i<100;i++)
    x = (x>>31 | x<<1) ^ i;

  SpinDummy2 = x;
  sAtomicInc(&StatSpin);
}

sStsLock::sStsLock()
{
  Count = 0;
}

void sStsLock::Lock()
{
  if(sAtomicInc(&Count)==1)
    return;
  sAtomicInc(&StatLock);

  sInt n = 0;
  for(;;)
  {
    sAtomicDec(&Count);
    sSpin();
    n++;
    if(n>256)
    {
      n=0;
      sSleep(1);
    }
    if(sAtomicInc(&Count)==1)
      return;
  }
}

void sStsLock::Unlock()
{
  sAtomicDec(&Count);
}

/****************************************************************************/
/***                                                                      ***/
/***   A thread that can work tasks. Thread index 0 is on main thread     ***/
/***                                                                      ***/
/****************************************************************************/

void sStsThreadFunc(class sThread *thread, void *_user)
{
  sStsThread *user = (sStsThread *) _user;

  while(thread->CheckTerminate())
  {

//    if(sSchedMon) sSchedMon->Begin(user->GetIndex(),0xffffff);
    user->Event->Wait(5);
    if(sSchedMon) sSchedMon->Begin(user->GetIndex(),0xff0000);
/*
    user->Lock->Lock();
    user->Running = 1;
    user->Lock->Unlock();
*/
    sInt fails = 0;
    while(user->Manager->Running==1 && user->Manager->ActiveWorkloadCount>0)
    {
      if(user->Execute())
      {
        fails++;
        if(fails>10)
        {
          sSleep(0);
          fails = 0;
        }
      }
      else
      {
        fails = 0;
      }
    }
/*
    user->Lock->Lock();
    user->Running = 2;
    user->Lock->Unlock();
*/
    if(sSchedMon) sSchedMon->End(user->GetIndex());
 //   if(sSchedMon) sSchedMon->End(user->GetIndex());
  }
}

/****************************************************************************/

sStsThread::sStsThread(sStsManager *m,sInt index,sInt taskcount,sBool thread)
{
  Manager = m;
  Index = index;
  Thread = 0;
  Lock = new sStsLock;
  Event = new sThreadEvent;

//  Running = 0;

  if(thread)
  {
    Thread = new sThread(sStsThreadFunc,0,0x4000,this,0);
    Thread->SetHomeCore(index);
  }
  else
  {
    sVERIFY(index==0);
    sGetThreadContext()->Thread->SetHomeCore(index);
  }
}

sStsThread::~sStsThread()
{
  if(Thread)
  {
    Thread->Terminate();
    Event->Signal();
    delete Thread;
  }

  delete Lock;
  delete Event;
}

void sStsThread::AddTask(sStsTask *task)
{
  Lock->Lock();
  sStsWorkload *wl = task->Workload;
  sStsQueue *qu = wl->Queues[Index];
  if(qu->TaskCount==qu->TaskMax)          // task queue full, immediate execution
  {
    sAtomicInc(&wl->TasksRunning);
    Lock->Unlock();
//    sDPrintF(L"queue full\n");
    for(sInt i=task->Start;i<task->End;i++)
      (*task->Code)(Manager,this,i,1,task->Data);
    sAtomicDec(&wl->TasksRunning);
  }
  else                            // enqueue task
  {
    sAtomicInc(&wl->TasksLeft);
    sAtomicInc(&Manager->TotalTasksLeft);
    qu->Tasks[qu->TaskCount++] = task;
    qu->SubTaskCount += task->End-task->Start;
    Lock->Unlock();
  }
}

void sStsThread::DecreaseSync(sStsTask *t)
{
  for(sInt i=0;i<t->SyncCount;i++)
  {
    sStsSync *s = t->Syncs[i];
    if(s)
    {
      sInt n = sAtomicDec(&s->Count);
      if(n==0 && s->ContinueTask)
        AddTask(s->ContinueTask);
    }
  }
}

sBool sStsThread::Execute()
{
  sStsWorkload *wl;

  // grab next task

  sBool fail = 1;
  sInt start=0;
  sInt end=0;
  void *data=0;
  sStsCode code=0;
  sStsTask *killtask = 0;
  sInt count = 0;
  sStsQueue *qu = 0;
  sBool TryDeleteWorkload = 0;
  WorkloadReadLock.Lock();
  sFORALL_LIST(Manager->ActiveWorkloads,wl)
  {
    qu = wl->Queues[Index];
    Lock->Lock();

    // sometimes we have empty tasks in pipe. flush them!

    while(qu->TaskCount>0)
    {
      sStsTask *task;
      task = qu->Tasks[qu->TaskCount-1];
      if(task->Start<task->End)
        break;
      sVERIFY(task->Start==task->End);
      qu->TaskCount--;
      sAtomicDec(&wl->TasksLeft);
      sAtomicDec(&Manager->TotalTasksLeft);
    }
    if(wl->TasksLeft==0 && wl->TasksRunning==0)
      TryDeleteWorkload = 1;

    // get next subtasks

    if(qu->TaskCount>0)
    {
      sStsTask *t = qu->Tasks[qu->TaskCount-1];
      start = t->Start;
      end = t->End;
      data = t->Data;
      code = t->Code;
      count = sMin(end-start,t->Granularity);
      sVERIFY(count>0);
      t->Start+=count;
      if(t->Start==t->End)
        killtask = t;
      qu->SubTaskCount-=count;
      qu->DontSteal = (qu->TaskCount==1) && (t->End-t->Start <= t->EndGame);
      sAtomicInc(&wl->TasksRunning);
    }

    Lock->Unlock();
    if(code)
      break;
  }
  WorkloadReadLock.Unlock();
  if(TryDeleteWorkload)
  {
    sStsWorkload *wl0;
    Manager->WorkloadWriteLock();
retry:
    sFORALL_LIST(Manager->ActiveWorkloads,wl0)
    {
      if(wl0->TasksLeft==0 && wl0->TasksRunning==0)
      {
        Manager->ActiveWorkloads.Rem(wl0);
        sVERIFY(wl0->Mode==sSWM_RUNNING);
        wl0->Mode = sSWM_FINISHED;
        wl0->SpinCount = StatSpin; StatSpin = 0;
        wl0->FailedLockCount = StatLock; StatLock = 0;
        for(sInt i=0;i<wl0->ThreadCount;i++)
          wl0->ExeCount += wl0->Queues[i]->ExeCount;
        goto retry;
      }
    }
    Manager->WorkloadWriteUnlock();
  }
  if(code)                        // execute subtask
  {
    (*code)(Manager,this,start,count,data);
    if(killtask)
      DecreaseSync(killtask);
    qu->ExeCount++;
    sAtomicDec(&wl->TasksRunning);
    fail = 0;
  }
  else                            // nothing found. Steal some!
  {
    if(Manager->StealTasks(Index)==0)
    {
      sSpin();                    // still nothing found. spin a bit
    }
  }
  return fail;
}

/****************************************************************************/
/***                                                                      ***/
/***   Workloads                                                          ***/
/***                                                                      ***/
/****************************************************************************/

sStsWorkload::sStsWorkload(sStsManager *mng)
{
  Manager = mng;

  const sInt memory = mng->ConfigPoolMem;
  Mem = new sU8[memory];
  MemUsed = sPtr(Mem);
  MemEnd = MemUsed+memory;

  ThreadCount = mng->GetThreadCount();
  Queues = new sStsQueue *[ThreadCount];
  for(sInt i=0;i<ThreadCount;i++)
  {
    Queues[i] = new sStsQueue;
    Queues[i]->Tasks = new sStsTask*[mng->ConfigMaxTasks];
    Queues[i]->TaskMax = mng->ConfigMaxTasks;
    Queues[i]->TaskCount = 0;
    Queues[i]->SubTaskCount = 0;
    Queues[i]->DontSteal = 0;
    Queues[i]->ExeCount = 0;
  }
  Tasks.HintSize(4096);
  TasksLeft = 0;
  TasksRunning = 0;
}

sStsWorkload::~sStsWorkload()
{
  for(sInt i=0;i<ThreadCount;i++)
  {
    delete[] Queues[i]->Tasks;
    delete Queues[i];
  }
  delete[] Queues;
  delete[] Mem;
}

sU8 *sStsWorkload::AllocBytes(sInt bytes)
{
  bytes = sAlign(bytes,4);
  sPtr r = sAtomicAdd(&MemUsed,bytes);
  if(r>MemEnd)
    sFatal(L"out of sts memory");
  sVERIFY(((r-bytes)&3)==0);
  return (sU8 *)sPtr(r-bytes);
}

/****************************************************************************/

sStsTask *sStsWorkload::NewTask(sStsCode code,void* data,sInt subtasks,sInt syncs)
{
  sStsTask *task = Alloc<sStsTask>(1);
  task->Code = code;
  task->Data = data;
  task->Start = 0;
  task->End = subtasks;
  task->Granularity = 1;
  task->EndGame = 1;
  task->Workload = this;
  task->SyncCount = syncs;
  task->Syncs = 0;
  if(syncs>0)
  {
    task->Syncs = Alloc<sStsSync *>(syncs);
    for(sInt i=0;i<syncs;i++)
      task->Syncs[i] = 0;
  }
  return task;
}

void sStsWorkload::AddTask(sStsTask *task)
{
  Manager->Threads[0]->AddTask(task);
}

/****************************************************************************/

const sChar *sStsWorkload::PrintStat()
{
  StatBuffer.PrintF(L"steals %3d failed steals %3d failed locks %3d spins %5d exe %06d\n",StealCount,FailedStealCount,FailedLockCount,SpinCount,ExeCount);
  return StatBuffer;
}

/****************************************************************************/
/***                                                                      ***/
/***   Manage Tasks and Threads.                                          ***/
/***                                                                      ***/
/****************************************************************************/

sStsManager::sStsManager(sInt memory,sInt taskqueuelength,sInt maxcore)
{
  Running = 0;
  TotalTasksLeft = 0;
  ActiveWorkloadCount = 0;

  ConfigPoolMem = memory;
  ConfigMaxTasks = taskqueuelength;

//  Mem = new sU8[memory];
//  MemUsed = sPtr(Mem);
//  MemEnd = MemUsed+memory;

  ThreadCount = sGetCPUCount();
  if(maxcore>0)
    ThreadCount = sMin(maxcore,ThreadCount);
  if(maxcore<0)
    ThreadCount = sMax(1,ThreadCount+maxcore);

  ThreadBits = sFindHigherPower(ThreadCount);
  Threads = new sStsThread *[ThreadCount];
  for(sInt i=0;i<ThreadCount;i++)
    Threads[i] = new sStsThread(this,i,taskqueuelength,i>0);

  Start();
}

sStsManager::~sStsManager()
{
  Finish();

  sVERIFY(ActiveWorkloads.IsEmpty());
  sDeleteAll(FreeWorkloads);
//  delete[] Mem;
  for(sInt i=0;i<ThreadCount;i++)
    delete Threads[i];
  delete[] Threads;
  delete sSchedMon;
}

/****************************************************************************/

sStsWorkload *sStsManager::BeginWorkload()
{
  if(FreeWorkloads.IsEmpty())
  {
    FreeWorkloads.AddTail(new sStsWorkload(this));
  }
  sStsWorkload *wl = FreeWorkloads.RemTail();
  wl->TasksLeft = 0;
  wl->TasksRunning = 0;
  wl->Mode = sSWM_READY;
  wl->StealCount = 0;
  wl->SpinCount = 0;
  wl->ExeCount = 0;
  wl->FailedLockCount = 0;
  wl->FailedStealCount = 0;

  wl->MemUsed = sPtr(wl->Mem);

  return wl;
}

void sStsManager::StartWorkload(sStsWorkload *wl)
{
  for(sInt i=0;i<wl->ThreadCount;i++)
  {
    sStsQueue *qu = wl->Queues[i];

    qu->DontSteal = 0;
    qu->ExeCount = 0;
  }
  sVERIFY(wl->Mode==sSWM_READY);
  wl->Mode = sSWM_RUNNING;

  WorkloadWriteLock();
  ActiveWorkloads.AddTail(wl);
  WorkloadWriteUnlock();
  for(sInt i=1;i<ThreadCount;i++)
    Threads[i]->Event->Signal();
}

void sStsManager::SyncWorkload(sStsWorkload *wl)
{
  while(wl->Mode==sSWM_RUNNING)
  {
    sInt failcount = 0;
    if(Threads[0]->Execute())
    {
      failcount++;
      if(failcount>10)
      {
        failcount = 0;
        sSleep(0);
      }
    }
    else
    {
      failcount = 0;
    }
  }
}

sBool sStsManager::HelpWorkload(sStsWorkload *wl)
{
  if(wl->Mode==sSWM_RUNNING)
  {
    if(Threads[0]->Execute())
      sSleep(0);
    return 1;
  }
  else
  {
    return 0;
  }
}
void sStsManager::EndWorkload(sStsWorkload *wl)
{
  sVERIFY(wl->Mode==sSWM_FINISHED);
  wl->Mode = sSWM_IDLE;
  sVERIFY(wl->TasksLeft==0);
  sVERIFY(wl->TasksRunning==0);
  FreeWorkloads.AddTail(wl);
}

/****************************************************************************/
/*
sStsSync *sStsManager::NewSync()
{
  sStsSync *sync = Alloc<sStsSync>(1);
  sync->Count = 0;
  sync->ContinueTask = 0;
  return sync;
}
*/
void sStsManager::AddSync(sStsTask *t,sStsSync *s)
{
  for(sInt i=0;i<t->SyncCount;i++)
  {
    if(t->Syncs[i]==0)
    {
      t->Syncs[i] = s;
      sAtomicInc(&s->Count);
      return;
    }
  }
  sVERIFYFALSE;
}


/****************************************************************************/

void sStsManager::Start()
{
  // is it really off?

  sVERIFY(!Running);
/*
  for(sInt i=0;i<ThreadCount;i++)
  {
    Threads[i]->Lock->Lock();
    sVERIFY(Threads[i]->Running==0);
    Threads[i]->Lock->Unlock();
  }
*/

  // distribute initial load for a smooth start

//  for(sInt i=1;i<ThreadCount;i++)
//    StealTasks(i);

  // run! but not thread[0]

  Running = 1;
  for(sInt i=1;i<ThreadCount;i++)
    Threads[i]->Event->Signal();
}


void sStsManager::StartSingle()
{
  // is it really off?

  sVERIFY(!Running);
/*
  for(sInt i=0;i<ThreadCount;i++)
  {
    Threads[i]->Lock->Lock();
    sVERIFY(Threads[i]->Running==0);
    Threads[i]->Lock->Unlock();
  }
*/
  // do as if all other threads have finished
/*
  for(sInt i=0;i<ThreadCount;i++)
  {
    Threads[i]->Lock->Lock();
    Threads[i]->Running=2;
    Threads[i]->Lock->Unlock();
  }
  */
}


void sStsManager::Finish()
{
  // make thread[0] join the team

//  Threads[0]->Running=1;
  sBool x=0;
  for(;;)
  {
    Threads[0]->WorkloadReadLock.Lock();
    x = ActiveWorkloads.IsEmpty();
    Threads[0]->WorkloadReadLock.Unlock();
    if(x) break;
    Threads[0]->Execute();
  }

  // check if we have finished successfully

//  Threads[0]->Running=2;

  Running = 2; // signal threads to go to sleep

  // wait till all tasks sleep savely.
/*
  sBool ok;
  for(;;)
  {
    ok = 1;
    for(sInt i=1;i<ThreadCount;i++)
    {
      if(Threads[i]->Running!=2)
        ok = 0;
    }
    if(ok)
      break;

    sSpin();
  }
  sVERIFY(TotalTasksLeft==0);
*/

  while(TotalTasksLeft>0)
    sSpin();

  // check if it's really safe
/*
  for(sInt i=0;i<ThreadCount;i++)
  {
    Threads[i]->Lock->Lock();
    sVERIFY(Threads[i]->Running==2);
    Threads[i]->Lock->Unlock();
  }
*/
  sVERIFY(TotalTasksLeft == 0);

  // release threads
/*
  for(sInt i=0;i<ThreadCount;i++)
  {
    Threads[i]->Running=0;
  }
*/
  // reset memory pool

  Running = 0;
//  MemUsed = sPtr(Mem);
}

void sStsManager::Sync(sStsSync *sync)
{
  while(sync->Count>0)
    Threads[0]->Execute();
}

/****************************************************************************/
/*
sU8 *sStsManager::AllocBytes(sInt bytes)
{
  bytes = sAlign(bytes,4);
  sPtr r = sAtomicAdd(&MemUsed,bytes);
  if(r>MemEnd)
    sFatal(L"out of sts memory");
  sVERIFY(((r-bytes)&3)==0);
  return (sU8 *)sPtr(r-bytes);
}
*/
sBool sStsManager::StealTasks(sInt to)
{
  sInt bestt=-1;
  sInt bestn=0;
  sStsWorkload *bestwl;

  sStsWorkload *wl;
  Threads[to]->WorkloadReadLock.Lock();
  sFORALL_LIST(ActiveWorkloads,wl)
  {
    for(sInt i=0;i<ThreadBits;i++)  // test a few carefully selected threads, find the best
    {
      sInt t = to^(1<<i);
      if(t<ThreadCount)
      {
        sStsQueue *qu = wl->Queues[t];
        sInt n = qu->SubTaskCount;
        sVERIFY(n>=0);
        if(n>bestn && qu->DontSteal==0)
        {
          bestn = n;
          bestt = t;
          bestwl = wl;
        }
      }
    }

    if(bestn==0)                    // nothing found. try harder, check everyone!
    {
      for(sInt i=0;i<ThreadCount;i++)
      {
        sStsQueue *qu = wl->Queues[i];
        sInt n = qu->SubTaskCount;
        sVERIFY(n>=0);
        if(n>bestn && qu->DontSteal==0)
        {
          bestn = n;
          bestt = i;
          bestwl = wl;
        }
      }
    }
    if(bestn>0)
      break;
  }
  Threads[to]->WorkloadReadLock.Unlock();

  if(bestn==0)                    // still nothing found. could be someoneelse is stealing just now
    return 0;

  if(bestt==to)                   // don't steal from ourself
    return 0;

  wl = bestwl;

  // found something. maybe it's already gone, whe have not locked!

  // now we lock and steal

  sStsThread *th = Threads[bestt];
  sStsQueue *qu = wl->Queues[bestt];
  sStsQueue *qt = wl->Queues[to];
  th->Lock->Lock();

  // sometimes we have empty tasks in pipe. flush them!
/*
  while(qu->TaskCount>0)
  {
    sStsTask *task;
    task = qu->Tasks[qu->TaskCount-1];
    if(task->Start<task->End)
      break;
    sVERIFY(task->Start==task->End);
    qu->TaskCount--;
    th->DecreaseSync(task);
    sAtomicDec(&wl->TasksLeft);
    sAtomicDec(&TotalTasksLeft);
  }
*/
  // get a task for real

  sInt n = qu->TaskCount;
  sAtomicInc(&wl->StealCount);

  if(n>3 && qu->DontSteal==0)               // lots of tasks, be careless
  {
    // remove tasks, copy them to buffer

    n = sMin(n/2,qt->TaskMax - qt->TaskCount);
    sInt subtasks = 0;
    sStsTask **list = sALLOCSTACK(sStsTask *,n);
    for(sInt i=0;i<n;i++)
    {
      list[i] = qu->Tasks[qu->TaskCount-n+i];
      subtasks += list[i]->End - list[i]->Start;
    }

    qu->SubTaskCount -= subtasks;
    qu->TaskCount -= n;
    th->Lock->Unlock();

    // add tasks to new thread.
    // make sure not to lock two threads at the same time (deadlocks)

    Threads[to]->Lock->Lock();
    sVERIFY(qt->TaskCount + n <= qt->TaskMax);
    for(sInt i=0;i<n;i++)
      qt->Tasks[qt->TaskCount + i] = list[i];
    qt->TaskCount += n;
    qt->SubTaskCount += subtasks;
    Threads[to]->Lock->Unlock();
    return 1;
  }

  if(n>=1 && qu->DontSteal==0)                   // few tasks, split subtasks
  {
    sStsTask  *task = qu->Tasks[n-1];
    if(task->End-task->Start>1)   // we have subtasks! split range
    {
      sInt d = (task->End-task->Start)/2;
      sInt m = task->End - d;
      sStsTask *nt = wl->NewTask(task->Code,task->Data,0,task->SyncCount);
      nt->Granularity = task->Granularity;
      nt->EndGame = task->EndGame;
      nt->Start = m;
      nt->End = task->End;
      task->End = m;
      if(n==1) qu->DontSteal = task->End-task->Start <= task->EndGame;

      for(sInt i=0;i<task->SyncCount;i++)
      {
        nt->Syncs[i] = task->Syncs[i];
        sAtomicInc(&nt->Syncs[i]->Count);
      }
      qu->SubTaskCount -= d;
      sVERIFY(wl->TasksLeft>0);
      sAtomicInc(&wl->TasksLeft);  // count the new tasks before someone can finish old task (in case it't the last task)
      sAtomicInc(&TotalTasksLeft);
      th->Lock->Unlock();

      // add task

      Threads[to]->Lock->Lock();
      sVERIFY(qt->TaskCount<qt->TaskMax);
      qt->Tasks[qt->TaskCount++] = nt;
      qt->SubTaskCount += d;
      qt->DontSteal = nt->End-nt->Start <= nt->EndGame;
      Threads[to]->Lock->Unlock();
      return 1;
    }
    if(task->End-task->Start==1)    // only one subtask, move task
    {
      qu->TaskCount--;
      qu->SubTaskCount--;
      th->Lock->Unlock();

      // add task

      Threads[to]->Lock->Lock();
      sVERIFY(qt->TaskCount<qt->TaskMax);
      qt->Tasks[qt->TaskCount++] = task;
      qt->SubTaskCount++;
      qt->DontSteal = 1;          // protect this one subtask from getting stolen
      Threads[to]->Lock->Unlock();
      return 1;
    }
  }

  // all paths above do "return 1";

  th->Lock->Unlock();
  sAtomicInc(&wl->FailedStealCount);
  sAtomicDec(&wl->StealCount);
  return 0;
}

void sStsManager::WorkloadWriteLock()
{
  for(sInt i=0;i<ThreadCount;i++)
    Threads[i]->WorkloadReadLock.Lock();
}
void sStsManager::WorkloadWriteUnlock()
{
  ActiveWorkloadCount = ActiveWorkloads.GetCount();
  for(sInt i=ThreadCount-1;i>=0;i--)
    Threads[i]->WorkloadReadLock.Unlock();
}

/****************************************************************************/
/***                                                                      ***/
/***   Cool Performance Meter                                             ***/
/***                                                                      ***/
/****************************************************************************/

sStsPerfMon::sStsPerfMon()
{
  ThreadCount = sSched->GetThreadCount();
  DataCount = 0x40000;
  CountMask = DataCount-1;

  Counters = new sInt *[ThreadCount];
  OldCounters = new sInt *[ThreadCount];
  Datas = new Entry *[ThreadCount];
  OldDatas = new Entry *[ThreadCount];
  Rects = new sRect[ThreadCount];
  for(sInt i=0;i<ThreadCount;i++)
  {
    Counters[i] = new sInt;
    OldCounters[i] = new sInt;
    Datas[i] = new Entry[DataCount];
    OldDatas[i] = new Entry[DataCount];
  }
  Enable = 1;

  Geo = new sGeometry(sGF_QUADLIST,sVertexFormatBasic);
  Mtrl = new sSimpleMaterial;
  Mtrl->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  Mtrl->Prepare(sVertexFormatBasic);

  GeoPtr = 0;
  GeoUsed = 0;
  GeoAlloc = 0;

  Scale = 16;
}

sStsPerfMon::~sStsPerfMon()
{
  for(sInt i=0;i<ThreadCount;i++)
  {
    delete Counters[i];
    delete OldCounters[i];
    delete[] Datas[i];
    delete[] OldDatas[i];
  }
  delete[] Counters;
  delete[] OldCounters;
  delete[] Datas;
  delete[] OldDatas;

  delete[] Rects;
  delete Geo;
  delete Mtrl;
}

/****************************************************************************/

void sStsPerfMon::FlipFrame()
{
  sInt **c = Counters;
  Entry **d = Datas;

  Enable = 0;
  sWriteBarrier();

  if(sGetKeyQualifier() & sKEYQ_CTRL)
  {
    TimeStart = sGetTimeStamp();
    for(sInt i=0;i<ThreadCount;i++)
      *(Counters[i]) = 0;
  }
  else
  {
    for(sInt i=0;i<ThreadCount;i++)
      *(OldCounters[i]) = 0;

    sReadBarrier();
    Counters = OldCounters;
    Datas = OldDatas;
    OldCounters = c;
    OldDatas = d;
    sWriteBarrier();
    OldStart = TimeStart;
    OldEnd = TimeStart = sGetTimeStamp();
  }

  sWriteBarrier();
  Enable = 1;
}

/****************************************************************************/

void sStsPerfMon::Box(sF32 x0,sF32 y0,sF32 x1,sF32 y1,sU32 col)
{
  if(GeoUsed+4>=GeoAlloc)
  {
    BoxEnd();
    GeoAlloc = 0x4000;
    GeoUsed = 0;
    Geo->BeginLoadVB(GeoAlloc,sGD_STREAM,&GeoPtr);
  }

  GeoPtr[GeoUsed+0].Init(x0,y0,0,col);
  GeoPtr[GeoUsed+1].Init(x1,y0,0,0);
  GeoPtr[GeoUsed+2].Init(x1,y1,0,0);
  GeoPtr[GeoUsed+3].Init(x0,y1,0,col);
  GeoUsed+=4;
}

void sStsPerfMon::BoxEnd()
{
  if(GeoUsed>0)
  {
    Geo->EndLoadVB(GeoUsed);
    Geo->Draw();
  }
  GeoAlloc = 0;
  GeoUsed = 0;
  GeoPtr = 0;
}

void sStsPerfMon::Paint(const sTargetSpec &ts)
{
  sInt sx,sy;
  sx = ts.Window.SizeX();
  sy = ts.Window.SizeY();
  sU32 colorstack[16];
  sInt colorindex;
  sViewport View;

  // prepare

  GeoAlloc = 0;
  GeoUsed = 0;
  GeoPtr = 0;
  
  View.SetTarget(ts);
  View.Orthogonal = sVO_PIXELS;
  View.Prepare();
  sCBuffer<sSimpleMaterialPara> cb;
  cb.Data->Set(View);
  Mtrl->Set(&cb);

  // paint

  for(sInt i=0;i<ThreadCount;i++)
  {
    Rects[i].x0 = 10;
    Rects[i].x1 = sx-10;
    Rects[i].y1 = sy - (ThreadCount-i-1)*12-10-9 - (ThreadCount-i-1)/4*6;
    Rects[i].y0 = Rects[i].y1 - 9;
  }

  for(sInt i=0;i<ThreadCount;i++)
  {
    sClear(colorstack);
    colorindex = 0;
    Box(Rects[i].x0-1,Rects[i].y0-1,Rects[i].x0+((OldEnd-OldStart)>>Scale)+1,Rects[i].y1+1,0xff202020);
    sInt max = *OldCounters[i];
    if(max<DataCount)
    {
      Entry *d = OldDatas[i];
      sInt pos0 = Rects[i].x0;
      sInt col0 = 0;
      colorstack[colorindex&15] = 0;
      sF32 y0 = Rects[i].y0;
      sF32 y1 = Rects[i].y1;

      for(sInt j=0;j<max;j++)
      {
        sInt pos1 = (d[j].Timestamp>>(Scale))+Rects[i].x0;
        sU32 col1 = d[j].Color;
        if(col1)
          colorstack[(++colorindex)&15] = col1;
        else
          col1 = colorstack[(--colorindex)&15];
        if(pos1>pos0)
        {
          if(col0!=0)
            Box(pos0,y0,pos1,y1,col0);
        }
        col0 = col1;
        pos0 = pos1;
      }
    }
  }

  // done

  BoxEnd();
}

/****************************************************************************/

