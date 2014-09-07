/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Base/Containers.hpp"

using namespace Altona2;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Altona2::Main()
{
    sTextLog log,log1;

    sArray<sU32> unsorted;
    sArray<sU32> sorted;
    sInt min = 1024;
    sInt max = 1024*1024;
    sArray<sU32> newa,newb;

    // create random numbers

    unsorted.SetSize(max);
    sRandomKISS rnd;
    for(sInt i=0;i<max;i++)
        unsorted[i] = rnd.Int32();

    // create sorted numbers

    sorted.CopyFrom(unsorted);
    sorted.Sort([](sU32 a,sU32 b){ return a<b; });

    // sort tests

    const sChar *algostring[] = { "none","myinsert","mymerge","myquick" };
    const sChar *passstring[] = { "unsorted","sorted","reverse" };
    for(sInt pass=0;pass<3;pass++)
    {
        log.PrintF("\ndata: %s\n%10s ",passstring[pass],"Time");
        for(sInt algo=0;algo<sCOUNTOF(algostring);algo++)
            log.PrintF("%7s ",algostring[algo]);
        log.PrintF("\n");
        for(sInt size = min;size<=max;size = size*2)
        {
            log.PrintF("%10d ",size);
            //      sort.SetSize(size);
            newa.SetSize(size);
            newb.SetSize(size);
            for(sInt algo=0;algo<sCOUNTOF(algostring);algo++)
            {
                sBool sortedflag = 0;
                sU32 time0 = sGetTimeMS();
                for(sInt chunk =0;chunk<max;chunk += size)
                {
                    switch(pass)
                    {
                    case 0:
                        for(sInt i=0;i<size;i++)
                            newa[i] = unsorted[i+chunk];
                        break;
                    case 1:
                        for(sInt i=0;i<size;i++)
                            newa[i] = sorted[i+chunk];
                        break;
                    case 2:
                        for(sInt i=0;i<size;i++)
                            newa[i] = sorted[size-1-i+chunk];
                        break;
                    }
                    switch(algo)
                    {
                    case 0:
                        sortedflag = 0;
                        break;
                    case 1:
                        if(size<1024*4)
                        {
                            newa.InsertionSort([](sU32 a,sU32 b){ return a<b; });
                            for(sInt i=0;i<size-1;i++)
                                sASSERT(newa[i]<=newa[i+1]);
                            sortedflag = 1;
                        }
                        break;
                    case 2:
                        newa.MergeSort([](sU32 a,sU32 b){ return a<b; },newb);
                        for(sInt i=0;i<size-1;i++)
                            sASSERT(newa[i]<=newa[i+1]);
                        sortedflag = 1;
                        break;
                    case 3:
                        newa.QuickSort([](sU32 a,sU32 b){ return a<b; });
                        for(sInt i=0;i<size-1;i++)
                            sASSERT(newa[i]<=newa[i+1]);
                        sortedflag = 1;
                        break;
                    }
                }
                sU32 time1 = sGetTimeMS();
                if(algo==0 || sortedflag)
                    log.PrintF("%7d ",time1-time0);
                else
                    log.PrintF("%7s ","-");
            }
            log.Print("\n");

            log1.Print(log.Get());
            sDPrint(log.Get());
            log.Clear();
        }
    }
    sPrintF(log.Get());
}

/****************************************************************************/

