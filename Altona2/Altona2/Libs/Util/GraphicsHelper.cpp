/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Util/GraphicsHelper.hpp"
#include "Altona2/Libs/Util/Image.hpp"

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***   Subsystem Initializer                                              ***/
/***                                                                      ***/
/****************************************************************************/

class sGraphicsHelperSubsystem : public sSubsystem
{
public:
    sGraphicsHelperSubsystem() : sSubsystem("graphics helper",0xc1) {}
    void Init();
    void Exit();
} sGraphicsHelperSubsystem_;


void sGraphicsHelperSubsystem::Init()
{
}

void sGraphicsHelperSubsystem::Exit()
{
}

/****************************************************************************/
/***                                                                      ***/
/***   Queries                                                            ***/
/***                                                                      ***/
/****************************************************************************/

sQueryQueue::sQueryQueue(sAdapter *adapter,sGpuQueryKind kind)
{
    Adapter = adapter;
    Kind = kind;
    LastU32 = 0;
    LastU64 = 0;
    sClear(LastStats);
    Current = 0;
}

sQueryQueue::~sQueryQueue()
{
    AllQueries.DeleteAll();
}

void sQueryQueue::Begin()
{
    sASSERT(Current==0);

    if(FreeQueries.IsEmpty())
    {
        Current = new sGpuQuery(Adapter,Kind);
        AllQueries.Add(Current);
    }
    else
    {
        Current = FreeQueries.RemTail();
    }

    Current->Begin(Adapter->ImmediateContext);
}

void sQueryQueue::End()
{
    sASSERT(Current!=0);

    Current->End(Adapter->ImmediateContext);
    PendingQueries.AddTail(Current);
    Current = 0;
}

uint sQueryQueue::GetLastU32()
{
    while(!PendingQueries.IsEmpty())
    {
        sGpuQuery *q = PendingQueries[0];
        if(q->GetData(LastU32))
        {
            LastU64 = LastU32;
            PendingQueries.RemAtOrder(0);
            FreeQueries.Add(q);
        }
        else
        {
            break;
        }
    }

    return LastU32;
}

uint64 sQueryQueue::GetLastU64()
{
    while(!PendingQueries.IsEmpty())
    {
        sGpuQuery *q = PendingQueries[0];
        if(q->GetData(LastU64))
        {
            LastU32 = LastU64&0xffffffffULL;
            PendingQueries.RemAtOrder(0);
            FreeQueries.Add(q);
        }
        else
        {
            break;
        }
    }

    return LastU64;
}

void sQueryQueue::GetLastStats(sPipelineStats &stats)
{
    while(!PendingQueries.IsEmpty())
    {
        sGpuQuery *q = PendingQueries[0];
        if(q->GetData(LastStats))
        {
            PendingQueries.RemAtOrder(0);
            FreeQueries.Add(q);
        }
        else
        {
            break;
        }
    }

    stats = LastStats;
}

/****************************************************************************/
/***                                                                      ***/
/***   Loading                                                            ***/
/***                                                                      ***/
/****************************************************************************/

sResource *sLoadTexture(sAdapter *ada,const char *filename,int format)
{
    sImage img;
    if(!img.Load(filename))
        return 0;
    sPrepareTexture2D prep;
    prep.Load(&img);
    prep.Format = format;
    prep.Process();
    return prep.GetTexture(ada);
}

/****************************************************************************/

}
