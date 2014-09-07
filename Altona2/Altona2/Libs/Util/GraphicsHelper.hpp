/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_UTIL_GRAPHICSHELPER_HPP
#define FILE_ALTONA2_LIBS_UTIL_GRAPHICSHELPER_HPP

#include "Altona2/Libs/Base/Base.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***   Queries                                                            ***/
/***                                                                      ***/
/****************************************************************************/

class sQueryQueue
{
    sGpuQueryKind Kind;
    uint LastU32;
    uint64 LastU64;
    sPipelineStats LastStats;
    sArray<sGpuQuery *> AllQueries;
    sArray<sGpuQuery *> FreeQueries;
    sArray<sGpuQuery *> PendingQueries;
    sGpuQuery *Current;
    sAdapter *Adapter;
public:
    sQueryQueue(sAdapter *adapter,sGpuQueryKind kind);
    ~sQueryQueue();

    void Begin();
    void End();
    uint GetLastU32();
    uint64 GetLastU64();
    void GetLastStats(sPipelineStats &stats);
};

/****************************************************************************/
/***                                                                      ***/
/***   Loading                                                            ***/
/***                                                                      ***/
/****************************************************************************/

sResource *sLoadTexture(sAdapter *ada,const char *filename,int format=sRF_BGRA8888);

/****************************************************************************/
}
#endif  // FILE_ALTONA2_LIBS_UTIL_GRAPHICSHELPER_HPP

