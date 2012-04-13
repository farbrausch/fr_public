// This file is distributed under a BSD license. See LICENSE.txt for details.
#include "_types.hpp"
#include "plrutil.hpp"

/****************************************************************************/

sVector PerlinGradient3D[16];
sF32 PerlinRandom[256][2];
sU8 PerlinPermute[512];

/****************************************************************************/

static const sChar GTable[16*4] =
{
  1,1,0,0, 2,1,0,0, 1,2,0,0, 2,2,0,0,
  1,0,1,0, 2,0,1,0, 1,0,2,0, 2,0,2,0,
  0,1,1,0, 0,2,1,0, 0,1,2,0, 0,2,2,0,
  1,1,0,0, 2,1,0,0, 0,2,1,0, 0,2,2,0
};
static const sF32 GValue[3] = { 0.0f, 1.0, -1.0f };

void InitPerlin()
{
  sInt i,j,x,y;

  sSetRndSeed(1);

	// 3d gradients
	for(i=0;i<64;i++)
    (&PerlinGradient3D[0].x)[i] = GValue[GTable[i]];

  // permutation
	for(i=0;i<256;i++)
	{
		PerlinRandom[i][0]=sGetRnd(0x10000);
		PerlinPermute[i]=i;
	}

	for(i=0;i<255;i++)
	{
    for(j=i+1;j<256;j++)
    {
		  if(PerlinRandom[i][0]>PerlinRandom[j][0])
		  {
			  sSwap(PerlinRandom[i][0],PerlinRandom[j][0]);
			  sSwap(PerlinPermute[i],PerlinPermute[j]);
		  }
    }
	}
  sCopyMem(PerlinPermute+256,PerlinPermute,256);

  // random
  for(i=0;i<256;)
  {
    x = sGetRnd(0x10000)-0x8000;
    y = sGetRnd(0x10000)-0x8000;
    if(x*x+y*y<0x8000*0x8000)
    {
      PerlinRandom[i][0] = x/32768.0f;
      PerlinRandom[i][1] = y/32768.0f;
      i++;
    }
  }
}
