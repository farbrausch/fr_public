/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "base/types.hpp"
#include "base/system.hpp"
#include "base/math.hpp"

/****************************************************************************/

static void CalcInvVector(sVector30 &dst,const sVector30 &src)
{
  dst.x = src.x ? 1.0f / src.x : 0.0f;
  dst.y = src.y ? 1.0f / src.y : 0.0f;
  dst.z = src.z ? 1.0f / src.z : 0.0f;
}

void sMain()
{
  // bbox and bilinear patch to test against
  sAABBox box;
  box.Min.Init(0.0f,0.0f,0.0f);
  box.Max.Init(1.0f,1.0f,1.0f);

  sVector31 p00,p01,p10,p11;
  p00.Init(0.0f,0.0f,0.0f);
  p01.Init(1.0f,0.0f,0.0f);
  p10.Init(0.0f,0.0f,1.0f);
  p11.Init(1.0f,1.0f,1.0f);

  // ray direction and inverse ray direction is always constant
  sVector30 ird;
  sRay ray(sVector31(0.0f,2.0f,0.0f),sVector30(0.0f,-1.0f,0.0f));
  CalcInvVector(ird,ray.Dir);

  // do a bunch of tests
  static const sInt TestCount = 5000000;
  sRandomKISS rand;
  sF32 dist;
  rand.Init();

  sInt time1 = sGetTime();
  sInt nHits1 = 0;
  for(sInt i=0;i<TestCount;i++)
  {
    ray.Start.x = rand.Float(4.0f);
    ray.Start.z = rand.Float(4.0f);
    nHits1 += box.HitInvRay(dist,ray.Start,ird) ? 1 : 0;
  }
  time1 = sGetTime() - time1;

  sInt time2 = sGetTime();
  sInt nHits2 = 0;
  for(sInt i=0;i<TestCount;i++)
  {
    ray.Start.x = rand.Float(4.0f);
    ray.Start.z = rand.Float(4.0f);
    nHits2 += ray.HitBilinearPatch(dist,p00,p01,p10,p11,0,0) ? 1 : 0;
  }
  time2 = sGetTime() - time2;

  // print results
  sDPrintF(L"bbox: %d ms, %d runs, %d hits\n",time1,TestCount,nHits1);
  sDPrintF(L"patch: %d ms, %d runs, %d hits\n",time2,TestCount,nHits2);
}

/****************************************************************************/

