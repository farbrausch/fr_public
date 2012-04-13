// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "player_mobile/pa.hpp"
#include "player_mobile/engine_soft.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Performance Analysis                                               ***/
/***                                                                      ***/
/****************************************************************************/

#if sMOBILE

struct PAEntry *PAFirst;

sInt PATotal;
 
void InitPA()
{
  PATotal = -sGetTime();
}

PAEntry *list[1024];
sInt count;

void ExitPA()
{
  sInt i,j;
  PAEntry *pa = PAFirst;

  PATotal += sGetTime();
  count = 0;
  while(pa)
  {
    list[count++] = pa;
    pa = pa->Next;
  }

  for(i=0;i<count-1;i++)
    for(j=i+1;j<count;j++)
      if(list[i]->Time<list[j]->Time)
        sSwap(list[i],list[j]);

  for(i=0;i<count;i++)
    sDPrintF(L"%05d <%s>\n",list[i]->Time,list[i]->Name);
}

void PrintPA(class SoftEngine *soft)
{
  sInt sum;
  sInt i;
  sChar txt[128];

  sum = 0;
  for(i=0;i<count;i++)
    sum += list[i]->Time;
  sSPrintF(txt,sCOUNTOF(txt),L"Total: %dms, sum %dms",PATotal,sum);
  soft->Print(5,5,txt,0xf800);

  sum = 0;
  for(i=0;i<count;i++)
  {
    sum+=list[i]->Time;
    sSPrintF(txt,sCOUNTOF(txt),L"%05d (%05d) <%s>",list[i]->Time,sum,list[i]->Name);
    soft->Print(5,5+(i+1)*9,txt,0xf800);
  }
}

#endif

/****************************************************************************/
