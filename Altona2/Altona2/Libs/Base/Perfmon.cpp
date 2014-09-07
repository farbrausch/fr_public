/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Base/Perfmon.hpp"


namespace Altona2 {
    
/****************************************************************************/
/***                                                                      ***/
/***   Perfmon                                                            ***/
/***                                                                      ***/
/****************************************************************************/

sTHREADLOCALSTORAGE sPerfMonThread *sThreadPerfMon;
sPerfMonGpu *sGpuPerfMon;
bool sPerfMonEnable = 0;
sArray<sPerfMonBase *> *sPerfMonThreads;

class sPerfMonSubsystem : public sSubsystem
{
public:
    sThreadLock Lock;
    sPerfMonSubsystem() : sSubsystem("Performance Monitor",0x02) {}

    void Init()
    {
        sPerfMonThreads = new sArray<sPerfMonBase *>();
        sThreadPerfMon = new sPerfMonThread("CPU (Main Thread)",32);
        sPreFrameHook.Add(sDelegate<void>(this,&sPerfMonSubsystem::Flip));
    }
    void Exit()
    {
        if(sPerfMonThreads)
        {
            sPerfMonThreads->DeleteAll();
            delete sPerfMonThreads;
        }
    }
    void Add(sPerfMonBase *th)
    {
        Lock.Lock();
        if(sCmpString(th->GetName(),"GPU")==0)
            sPerfMonThreads->AddHead(th);
        else
            sPerfMonThreads->Add(th);
        Lock.Unlock();
    }
    void Flip()
    {
        Lock.Lock();
        for(auto t : *sPerfMonThreads)
            t->Flip();
        Lock.Unlock();
    }
} sPerfMonSubsystem_;

#if sConfigRender == sConfigRenderDX9 || sConfigRender==sConfigRenderDX11
class sPerfMonSubsystemGpu : public sSubsystem
{
public:
    sThreadLock Lock;
    sPerfMonSubsystemGpu() : sSubsystem("Performance Monitor (GPU)",0xff) {}

    void Init()
    {
        if(sPerfMonThreads)
        {
            //      sGpuPerfMon = new sPerfMonGpu(sGetCurrentAdapter(),"GPU");
        }
    }
    void Exit()
    {
        if(sPerfMonThreads)
        {
            sPerfMonThreads->RemOrder(sGpuPerfMon);
            sDelete(sGpuPerfMon);
        }
    }
} sPerfMonSubsystemGpu_;
#endif

const sArray<sPerfMonBase *> *sGetPerfMonThreads()
{
    return sPerfMonThreads;
}

bool sPerfMonEnabled()
{
    return sPerfMonEnable;
}

void sEnablePerfMon(bool e)
{
    sPerfMonEnable = e;
}

/****************************************************************************/

sPerfMonBase::sPerfMonBase(const char *name)
{
    Name = name;
    MasterSection.Color = 0xff000000;
    MasterSection.Name = "Frame";
    History = 2;
}

const char *sPerfMonBase::GetName()
{
    return Name;
}

/****************************************************************************/
/***                                                                      ***/
/***   Cpu                                                                ***/
/***                                                                      ***/
/****************************************************************************/

sPerfMonThread::sPerfMonThread(const char *name,int history) : sPerfMonBase(name)
{
    sASSERT(history>=2);
    History = history;
    for(int i=0;i<history;i++)
        Frames.Add(new Frame);
    for(int i=0;i<history;i++)
        Frames[i]->Records.HintSize(4096);
    DoubleBuffer = 0;
    Current = Frames[0];
    Flip();
    Flip();
    sPerfMonSubsystem_.Add(this);
}

sPerfMonThread::~sPerfMonThread()
{
    Frames.DeleteAll();
}

void sPerfMonThread::Flip()
{
    Leave(&MasterSection);
    if(Current)
        Current->Frequency = sGetHRFrequency();
    if(sPerfMonEnabled())
    {
        DoubleBuffer = (DoubleBuffer+1)%History;
        Current = Frames[DoubleBuffer];
        Current->Records.Clear();
    }
    else
    {
        Current = 0;
    }
    Enter(&MasterSection);
}

sArray<sPerfMonThread::Record> *sPerfMonThread::GetLastFrame(uint64 &freq) 
{ 
    freq = Frames[(DoubleBuffer+History-1)%History]->Frequency; 
    return &Frames[(DoubleBuffer+History-1)%History]->Records; 
}

sArray<sPerfMonThread::Record> *sPerfMonThread::GetFrame(int n,uint64 &freq)
{
    freq = Frames[(DoubleBuffer+History-n)%History]->Frequency; 
    n = sClamp(n,1,History-1);
    return &Frames[(DoubleBuffer+History-n)%History]->Records; 
}


/****************************************************************************/
/***                                                                      ***/
/***   Gpu                                                                ***/
/***                                                                      ***/
/****************************************************************************/

sPerfMonGpu::Frame::Frame(sAdapter *ada)
{
    FrequencyQuery = new sGpuQuery(ada,sGQK_Frequency);
}

sPerfMonGpu::Frame::~Frame()
{
    delete FrequencyQuery;
    sASSERT(Queries.GetCount()==0);
}

/****************************************************************************/

sPerfMonGpu::sPerfMonGpu(sAdapter *adapter,const char *name) : sPerfMonBase(name)
{
    Adapter = adapter;
    MostRecentFrame = GetFrame();
    MostRecentFrame->Frequency = 1000*1000;
    Current = 0;
    Flip();
    sPerfMonSubsystem_.Add(this);
}

sPerfMonGpu::~sPerfMonGpu()
{
    for(auto f : PendingFrames)
        RecycleFrame(f);
    if(Current)
        RecycleFrame(Current);
    AllQueries.DeleteAll();
    AllFrames.DeleteAll();
}

sGpuQuery *sPerfMonGpu::GetQuery()
{
    if(FreeQueries.GetCount()>0)
        return FreeQueries.RemTail();
    sGpuQuery *q = new sGpuQuery(Adapter,sGQK_Timestamp);
    AllQueries.AddTail(q);
    return q;
}

sPerfMonGpu::Frame *sPerfMonGpu::GetFrame()
{
    if(FreeFrames.GetCount()>0)
        return FreeFrames.RemTail();
    Frame *f = new Frame(Adapter);
    AllFrames.AddTail(f);
    return f;
}

bool sPerfMonGpu::CheckFrame(Frame *f)
{
    for(auto &q : f->Queries)
    {
        if(q.Query->GetData(f->Data[q.PatchTime].Time))
        {
            FreeQueries.AddTail(q.Query);
            q.Query = 0;
        }
    }
    f->Queries.RemIf([](RecordQuery &q){return !q.Query;});
    if(f->Queries.GetCount()==0)
    {
        if(f->FrequencyQuery->GetData(f->Frequency))
            return 1;
    }
    return 0;
}

void sPerfMonGpu::RecycleFrame(sPerfMonGpu::Frame *f)
{
    FreeFrames.Add(f);
    for(auto &q : f->Queries)
        FreeQueries.Add(q.Query);
    f->Queries.Clear();
}

/****************************************************************************/

void sPerfMonGpu::Enter(const sPerfMonSection *s)
{
    if(this && Current)
    {
        RecordQuery *q = Current->Queries.AddMany(1);
        q->PatchTime = int(Current->Data.GetCount());
        q->Query = GetQuery();
        q->Query->Issue(Adapter->ImmediateContext);
        Record *r = Current->Data.AddMany(1);
        r->Kind = RK_Enter;
        r->Section = s;
        r->Time = 0;
    }
}

void sPerfMonGpu::Leave(const sPerfMonSection *s)
{
    if(this && Current)
    {
        RecordQuery *q = Current->Queries.AddMany(1);
        q->PatchTime = int(Current->Data.GetCount());
        q->Query = GetQuery();
        q->Query->Issue(Adapter->ImmediateContext);
        Record *r = Current->Data.AddMany(1);
        r->Kind = RK_Leave;
        r->Section = s;
        r->Time = 0;
    }
}

sArray<sPerfMonBase::Record> *sPerfMonGpu::GetLastFrame(uint64 &freq)
{
    while(PendingFrames.GetCount()>0)
    {
        Frame *f = PendingFrames[0];
        if(CheckFrame(f))
        {
            PendingFrames.RemAtOrder(0);
            if(sPerfMonEnable)
            {
                FreeFrames.Add(MostRecentFrame);
                MostRecentFrame = f;
            }
            else
            {
                FreeFrames.Add(f);
            }
        }
        else
        {
            break;
        }
    }

    freq = MostRecentFrame->Frequency;
    return &MostRecentFrame->Data;
}

void sPerfMonGpu::Flip()
{
    if(Current)
    {
        //    Leave(&MasterSection);
        Current->FrequencyQuery->End(Adapter->ImmediateContext);
        PendingFrames.AddTail(Current);
        Current = 0;
        uint64 freq;
        while(PendingFrames.GetCount()>History)
            GetLastFrame(freq);
    }
    if(sPerfMonEnable)
    {
        Current = GetFrame();
        Current->Data.Clear();
        sASSERT(Current->Queries.IsEmpty());
        Current->FrequencyQuery->Begin(Adapter->ImmediateContext);
        //  Enter(&MasterSection);
    }
}
  
/****************************************************************************/
}
