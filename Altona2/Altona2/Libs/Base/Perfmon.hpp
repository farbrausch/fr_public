/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_BASE_PERFMON_HPP
#define FILE_ALTONA2_LIBS_BASE_PERFMON_HPP

#include "Altona2/Libs/Base/Types.hpp"
#include "Altona2/Libs/Base/System.hpp"


namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***   Perfmon                                                            ***/
/***                                                                      ***/
/****************************************************************************/

struct sPerfMonSection
{
    const char *Name;
    uint Color;

    sPerfMonSection() { Name="Frame"; Color=0xff000000; }
    sPerfMonSection(const char *n,uint c) { Name=n; Color=c; }
};

/****************************************************************************/
/****************************************************************************/

class sPerfMonBase
{
protected:
    const char *Name;
    sPerfMonSection MasterSection;
public:
    enum RecordKind
    {
        RK_Enter,
        RK_Leave,
    };
    struct Record
    {
        RecordKind Kind;
        const sPerfMonSection *Section;
        uint64 Time;

        Record() {}
        Record(RecordKind k,const sPerfMonSection *s,uint64 t) { Kind=k; Section=s; Time=t; }
    };

    sPerfMonBase(const char *name);
    virtual ~sPerfMonBase() {}
    virtual sArray<Record> *GetLastFrame(uint64 &freq) = 0;
    virtual sArray<Record> *GetFrame(int n,uint64 &freq) { return GetLastFrame(freq); }
    virtual void Flip() = 0;
    const char *GetName();
    int History;
};

/****************************************************************************/

class sPerfMonThread : public sPerfMonBase
{
    struct Frame
    {
        Frame() { Frequency = 0; }
        sArray<Record> Records;
        uint64 Frequency;
    };
    sArray<Frame *> Frames;
    int DoubleBuffer;
    Frame *Current;
public:
    sPerfMonThread(const char *name,int history = 32);
    ~sPerfMonThread();

    void Enter(const sPerfMonSection *s) { if(this && Current) Current->Records.AddTail(Record(RK_Enter,s,sGetTimeHR())); }
    void Leave(const sPerfMonSection *s) { if(this && Current) Current->Records.AddTail(Record(RK_Leave,s,sGetTimeHR())); }
    sArray<Record> *GetLastFrame(uint64 &freq);
    sArray<Record> *GetFrame(int n,uint64 &freq);
    void Flip();
};

/****************************************************************************/

class sPerfMonGpu : public sPerfMonBase
{
    struct RecordQuery
    {
        sGpuQuery *Query;
        int PatchTime;
    };
    class Frame
    {
    public:
        Frame(sAdapter *);
        ~Frame();

        uint64 Frequency;
        sArray<Record> Data;
        sArray<RecordQuery> Queries;
        sGpuQuery *FrequencyQuery;
    };

    sArray<sGpuQuery *> FreeQueries;
    sArray<sGpuQuery *> AllQueries;
    sArray<Frame *> AllFrames;
    sArray<Frame *> FreeFrames;
    sArray<Frame *> PendingFrames;
    Frame *Current;
    Frame *MostRecentFrame;
    sAdapter *Adapter;

    sGpuQuery *GetQuery();
    Frame *GetFrame();
    void RecycleFrame(Frame *);
    bool CheckFrame(Frame *f);
public:
    sPerfMonGpu(sAdapter *adapter,const char *name);
    ~sPerfMonGpu();

    void Enter(const sPerfMonSection *s);
    void Leave(const sPerfMonSection *s);
    sArray<Record> *GetLastFrame(uint64 &freq);
    void Flip();
};

/****************************************************************************/

const sArray<sPerfMonBase *> *sGetPerfMonThreads();
bool sPerfMonEnabled();
void sEnablePerfMon(bool);

extern sTHREADLOCALSTORAGE sPerfMonThread *sThreadPerfMon;
extern sPerfMonGpu *sGpuPerfMon;

/****************************************************************************/

class sPerfMonZone
{
    const sPerfMonSection *Section;
public:
    sPerfMonZone(const sPerfMonSection &s) { Section = &s; sThreadPerfMon->Enter(Section); }
    ~sPerfMonZone() { sThreadPerfMon->Leave(Section); }
};

class sPerfMonGpuZone
{
    const sPerfMonSection *Section;
public:
    sPerfMonGpuZone(const sPerfMonSection &s) { Section = &s; sGpuPerfMon->Enter(Section); }
    ~sPerfMonGpuZone() { sGpuPerfMon->Leave(Section); }
};

#if sConfigPlatform!=sConfigPlatformWin

#define sZONE(n,c) 
#define sGPU_ZONE(n,c)

#else

#define sZONE(n,c) static Altona2::sPerfMonSection _pmsx(n,c); Altona2::sPerfMonZone _pmxy(_pmsx);
#define sGPU_ZONE(n,c) static Altona2::sPerfMonSection _gpmsx(n,c); Altona2::sPerfMonGpuZone _gpmxy(_gpmsx);

#endif

/****************************************************************************/

} // namespace Altona2

#endif  // FILE_ALTONA2_LIBS_BASE_PERFMON_HPP

