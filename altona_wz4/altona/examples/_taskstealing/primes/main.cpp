/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "main.hpp"
#include "util/taskscheduler.hpp"
#include "util/algorithms.hpp"

/****************************************************************************/

#define GROUP 1000;
struct datapacket
{
  sInt Start;
};

sInt primes[1000*1000];
sU32 primecount;

void task(sStsManager *man,sStsThread *thr,sInt start,sInt count,void *data_)
{
  datapacket *data = (datapacket *) data_;

  sInt n0 = (start+data->Start)*GROUP;
  sInt n1 = n0+count*GROUP;
  for(sInt i=n0;i<n1;i++)
  {
    sBool prime = 1;
    sInt max = sInt(sSqrt(i)+1);
    for(sInt j=2;prime && j<max;j++)
      if((i%j)==0)
        prime = 0;

    if(prime)
    {
      sInt index = sAtomicInc(&primecount)-1;
      primes[index] = i;
    }
  }
}

void sMain()
{
  sAddSched();
  sInit(0);

  // prepare

  const sInt max = 1000;
  datapacket datas[max];

  for(sInt xxx = 0;xxx<100;xxx++)
  {
    // simple test

    primecount = 0;
    datapacket data;
    data.Start = 0;
    sStsWorkload *wl = sSched->BeginWorkload();
    wl->AddTask(wl->NewTask(task,&data,max,0));
    wl->Start();
    wl->Sync();
    wl->End();
    sPrintF(L"primes (simple): %d\n",primecount);

    // hard test

    primecount = 0;
    sInt n = 0;
    sInt datai = 0;
    sRandom rnd;
    wl = sSched->BeginWorkload();
    while(n<max)
    {
      sInt c = rnd.Int(10)+2;
      if(c+n>max)
        c = max-n;

      datas[datai].Start = n;
      wl->AddTask(wl->NewTask(task,&datas[datai],c,0));
      datai++;
      n+=c;
      sVERIFY(datai<max);
    }
    wl->Start();
    wl->Sync();
    wl->End();

    // output hard

    sPrintF(L"primes (hard):   %d\n",primecount);
    if(primecount<1000)
    {
      sArrayRange<sInt> r(primes+0,primes+primecount);
      sHeapSort(r);
      for(sInt i=0;i<sInt(primecount);i++)
      {
        if(i%10==0)
          sPrint(L"\n");
        sPrintF(L"%10d ",primes[i]);
      }
      sPrint(L"\n");
    }
  }

  // done

}

/****************************************************************************/

