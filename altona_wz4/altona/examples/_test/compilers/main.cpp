/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/
#define sPEDANTIC_WARN 1
#include "main.hpp" 
#include "base/math.hpp"
#include "base/serialize.hpp"
 
/****************************************************************************/

#define CHECK(x) if(!(x)) sCheck(sTXT(__FILE__),__LINE__);

static sInt ErrorCount;

void sCheck(const sChar *file,sInt line)
{
  sPrintF(L"%s(%d) failed!\n",file,line);
  sDPrintF(L"%s(%d) failed!\n",file,line);
  ErrorCount++; 
}

void sCheckModule(const sChar *mod)
{
  sPrintF(L"Testing %s...\n",mod);
}

/****************************************************************************/
/***                                                                      ***/
/***  Arrays with size zero                                               ***/
/***                                                                      ***/
/****************************************************************************/

void TestZeroArray()
{
  sCheckModule(L"zero array");

  struct bla
  {
    sU32 x;
    sU32 y[0];
  };

  sU32 *flat = new sU32[4];
  bla *stru = (bla *) flat;
  stru->x = 1;
  stru->y[0] = 2;
  stru->y[1] = 3;
  stru->y[2] = 4;

  CHECK(flat[0]==1);
  CHECK(flat[1]==2);
  CHECK(flat[2]==3);
  CHECK(flat[3]==4);

  delete[] flat;
}

sInt TestCastDown(sInt num)
{
  sU32 u32 = num;
  sU16 u16 = u32;
  return u16;
} 


/****************************************************************************/
/***                                                                      ***/
/***   sMain                                                              ***/
/***                                                                      ***/
/****************************************************************************/

sINITMEM(sIMF_DEBUG|sIMF_NORTL,512*1024*1024);

void sMain()
{
  ErrorCount = 0;

  sPrintF(L"/****************************************************************************/\n");
  sPrintF(L"/***                                                                      ***/\n");
  sPrintF(L"/***   Starting Test                                                      ***/\n");
  sPrintF(L"/***                                                                      ***/\n");
  sPrintF(L"/****************************************************************************/\n");

  sPrintF(L"\n");
 
  TestZeroArray();
  TestCastDown(0x3fffffff);

  sPrintF(L"\n");

  if(ErrorCount!=0)
  {
    sPrintF(L"/****************************************************************************/\n");
    sPrintF(L"/***                                                                      ***/\n");
    sPrintF(L"/***   there were %4d errors!                                            ***/\n",ErrorCount);
    sPrintF(L"/***                                                                      ***/\n");
    sPrintF(L"/****************************************************************************/\n");
  }
  else
  {
    sPrintF(L"/****************************************************************************/\n");
    sPrintF(L"/***                                                                      ***/\n");
    sPrintF(L"/***   all ok                                                             ***/\n");
    sPrintF(L"/***                                                                      ***/\n");
    sPrintF(L"/****************************************************************************/\n");
  }
}

/****************************************************************************/

