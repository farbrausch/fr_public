// This file is distributed under a BSD license. See LICENSE.txt for details.


#include "gen_bitmap_class.hpp"

#if sMOBILE
#include "start_mobile.hpp"
#else
sInt sFontBegin(sInt pagex,sInt pagey,sU16 *name,sInt xs,sInt ys,sInt style);
void sFontPrint(sInt x,sInt y,sU16 *string,sInt len);
void sFontEnd();
sU32 *sFontBitmap();
#endif
#include "player_mobile/engine_soft.hpp"

#if !sPLAYER
#include "doc.hpp"
#include "win_para.hpp"
#include "_util.hpp"
#endif

#include "player_mobile/pa.hpp"

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Tile-Rendering                                                     ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

using namespace GenBitmapTile;

/****************************************************************************/
/***                                                                      ***/
/***   Fixed Point Arithmetic 1:16:15 | 0x8000 = unit                     ***/
/***                                                                      ***/
/****************************************************************************/

sInt PerlinRandom[256][2];
sU8 PerlinPermute[512];

// log2/exp2 tables

static unsigned short Log2Tab[257] = {
  0x0000,0x00b8,0x016f,0x0226,0x02dc,0x0392,0x0447,0x04fb,0x05ae,0x0661,
  0x0713,0x07c4,0x0875,0x0925,0x09d5,0x0a83,0x0b31,0x0bdf,0x0c8c,0x0d38,
  0x0de4,0x0e8f,0x0f39,0x0fe3,0x108c,0x1134,0x11dc,0x1284,0x132a,0x13d1,
  0x1476,0x151b,0x15c0,0x1663,0x1707,0x17a9,0x184c,0x18ed,0x198e,0x1a2f,
  0x1acf,0x1b6e,0x1c0d,0x1cac,0x1d49,0x1de7,0x1e84,0x1f20,0x1fbc,0x2057,
  0x20f2,0x218c,0x2226,0x22bf,0x2358,0x23f0,0x2488,0x251f,0x25b6,0x264c,
  0x26e2,0x2777,0x280c,0x28a0,0x2934,0x29c8,0x2a5b,0x2aee,0x2b80,0x2c11,
  0x2ca3,0x2d33,0x2dc4,0x2e54,0x2ee3,0x2f72,0x3001,0x308f,0x311d,0x31aa,
  0x3237,0x32c3,0x3350,0x33db,0x3466,0x34f1,0x357c,0x3606,0x368f,0x3719,
  0x37a1,0x382a,0x38b2,0x393a,0x39c1,0x3a48,0x3ace,0x3b54,0x3bda,0x3c5f,
  0x3ce4,0x3d69,0x3ded,0x3e71,0x3ef5,0x3f78,0x3ffa,0x407d,0x40ff,0x4181,
  0x4202,0x4283,0x4304,0x4384,0x4404,0x4483,0x4503,0x4582,0x4600,0x467e,
  0x46fc,0x477a,0x47f7,0x4874,0x48f1,0x496d,0x49e9,0x4a64,0x4ae0,0x4b5b,
  0x4bd5,0x4c4f,0x4cc9,0x4d43,0x4dbc,0x4e36,0x4eae,0x4f27,0x4f9f,0x5017,
  0x508e,0x5105,0x517c,0x51f3,0x5269,0x52df,0x5355,0x53cb,0x5440,0x54b5,
  0x5529,0x559e,0x5612,0x5685,0x56f9,0x576c,0x57df,0x5851,0x58c4,0x5936,
  0x59a8,0x5a19,0x5a8a,0x5afb,0x5b6c,0x5bdc,0x5c4c,0x5cbc,0x5d2c,0x5d9b,
  0x5e0a,0x5e79,0x5ee8,0x5f56,0x5fc4,0x6032,0x60a0,0x610d,0x617a,0x61e7,
  0x6253,0x62c0,0x632c,0x6398,0x6403,0x646e,0x64d9,0x6544,0x65af,0x6619,
  0x6683,0x66ed,0x6757,0x67c0,0x6829,0x6892,0x68fb,0x6964,0x69cc,0x6a34,
  0x6a9c,0x6b03,0x6b6b,0x6bd2,0x6c39,0x6c9f,0x6d06,0x6d6c,0x6dd2,0x6e38,
  0x6e9d,0x6f02,0x6f68,0x6fcd,0x7031,0x7096,0x70fa,0x715e,0x71c2,0x7225,
  0x7289,0x72ec,0x734f,0x73b2,0x7414,0x7477,0x74d9,0x753b,0x759d,0x75fe,
  0x7660,0x76c1,0x7722,0x7783,0x77e3,0x7844,0x78a4,0x7904,0x7964,0x79c3,
  0x7a23,0x7a82,0x7ae1,0x7b40,0x7b9e,0x7bfd,0x7c5b,0x7cb9,0x7d17,0x7d75,
  0x7dd2,0x7e30,0x7e8d,0x7eea,0x7f46,0x7fa3,0x7fff,
};

static unsigned short Exp2Tab[257] = {
  0x4000,0x402c,0x4058,0x4085,0x40b2,0x40df,0x410c,0x4139,0x4166,0x4194,
  0x41c1,0x41ef,0x421d,0x424a,0x4278,0x42a7,0x42d5,0x4303,0x4332,0x4360,
  0x438f,0x43be,0x43ed,0x441c,0x444c,0x447b,0x44aa,0x44da,0x450a,0x453a,
  0x456a,0x459a,0x45ca,0x45fb,0x462b,0x465c,0x468d,0x46be,0x46ef,0x4720,
  0x4752,0x4783,0x47b5,0x47e7,0x4818,0x484a,0x487d,0x48af,0x48e1,0x4914,
  0x4947,0x497a,0x49ad,0x49e0,0x4a13,0x4a46,0x4a7a,0x4aae,0x4ae1,0x4b15,
  0x4b4a,0x4b7e,0x4bb2,0x4be7,0x4c1b,0x4c50,0x4c85,0x4cba,0x4cf0,0x4d25,
  0x4d5b,0x4d90,0x4dc6,0x4dfc,0x4e32,0x4e69,0x4e9f,0x4ed5,0x4f0c,0x4f43,
  0x4f7a,0x4fb1,0x4fe9,0x5020,0x5058,0x508f,0x50c7,0x50ff,0x5138,0x5170,
  0x51a9,0x51e1,0x521a,0x5253,0x528c,0x52c5,0x52ff,0x5339,0x5372,0x53ac,
  0x53e6,0x5421,0x545b,0x5495,0x54d0,0x550b,0x5546,0x5581,0x55bd,0x55f8,
  0x5634,0x5670,0x56ac,0x56e8,0x5724,0x5761,0x579d,0x57da,0x5817,0x5854,
  0x5891,0x58cf,0x590d,0x594a,0x5988,0x59c7,0x5a05,0x5a43,0x5a82,0x5ac1,
  0x5b00,0x5b3f,0x5b7e,0x5bbe,0x5bfd,0x5c3d,0x5c7d,0x5cbe,0x5cfe,0x5d3e,
  0x5d7f,0x5dc0,0x5e01,0x5e42,0x5e84,0x5ec5,0x5f07,0x5f49,0x5f8b,0x5fce,
  0x6010,0x6053,0x6096,0x60d9,0x611c,0x615f,0x61a3,0x61e7,0x622b,0x626f,
  0x62b3,0x62f8,0x633c,0x6381,0x63c6,0x640b,0x6451,0x6497,0x64dc,0x6522,
  0x6569,0x65af,0x65f6,0x663c,0x6683,0x66ca,0x6712,0x6759,0x67a1,0x67e9,
  0x6831,0x6879,0x68c2,0x690b,0x6954,0x699d,0x69e6,0x6a2f,0x6a79,0x6ac3,
  0x6b0d,0x6b57,0x6ba2,0x6bed,0x6c38,0x6c83,0x6cce,0x6d1a,0x6d65,0x6db1,
  0x6dfd,0x6e4a,0x6e96,0x6ee3,0x6f30,0x6f7d,0x6fcb,0x7018,0x7066,0x70b4,
  0x7102,0x7151,0x719f,0x71ee,0x723d,0x728d,0x72dc,0x732c,0x737c,0x73cc,
  0x741c,0x746d,0x74be,0x750f,0x7560,0x75b1,0x7603,0x7655,0x76a7,0x76f9,
  0x774c,0x779f,0x77f2,0x7845,0x7899,0x78ec,0x7940,0x7994,0x79e9,0x7a3d,
  0x7a92,0x7ae7,0x7b3d,0x7b92,0x7be8,0x7c3e,0x7c94,0x7ceb,0x7d41,0x7d98,
  0x7def,0x7e47,0x7e9f,0x7ef6,0x7f4f,0x7fa7,0x8000,
};

static unsigned short SqrtTab[257] =
{
  0x0000,0x007f,0x00ff,0x017e,0x01fe,0x027c,0x02fb,0x0379,0x03f8,0x0476,0x04f3,0x0571,0x05ee,0x066b,0x06e8,0x0764,
  0x07e0,0x085d,0x08d8,0x0954,0x09cf,0x0a4b,0x0ac5,0x0b40,0x0bbb,0x0c35,0x0caf,0x0d29,0x0da3,0x0e1c,0x0e95,0x0f0e,
  0x0f87,0x1000,0x1078,0x10f0,0x1168,0x11e0,0x1257,0x12cf,0x1346,0x13bd,0x1433,0x14aa,0x1520,0x1596,0x160c,0x1682,
  0x16f8,0x176d,0x17e2,0x1857,0x18cc,0x1941,0x19b5,0x1a29,0x1a9d,0x1b11,0x1b85,0x1bf8,0x1c6c,0x1cdf,0x1d52,0x1dc4,
  0x1e37,0x1ea9,0x1f1c,0x1f8e,0x2000,0x2071,0x20e3,0x2154,0x21c5,0x2236,0x22a7,0x2318,0x2388,0x23f8,0x2469,0x24d9,
  0x2548,0x25b8,0x2628,0x2697,0x2706,0x2775,0x27e4,0x2852,0x28c1,0x292f,0x299e,0x2a0c,0x2a79,0x2ae7,0x2b55,0x2bc2,
  0x2c2f,0x2c9c,0x2d09,0x2d76,0x2de3,0x2e4f,0x2ebb,0x2f28,0x2f94,0x3000,0x306b,0x30d7,0x3142,0x31ad,0x3219,0x3284,
  0x32ee,0x3359,0x33c4,0x342e,0x3498,0x3502,0x356c,0x35d6,0x3640,0x36a9,0x3713,0x377c,0x37e5,0x384e,0x38b7,0x3920,
  0x3988,0x39f1,0x3a59,0x3ac1,0x3b29,0x3b91,0x3bf9,0x3c61,0x3cc8,0x3d30,0x3d97,0x3dfe,0x3e65,0x3ecc,0x3f32,0x3f99,
  0x4000,0x4066,0x40cc,0x4132,0x4198,0x41fe,0x4264,0x42c9,0x432f,0x4394,0x43f9,0x445e,0x44c3,0x4528,0x458d,0x45f1,
  0x4656,0x46ba,0x471e,0x4783,0x47e7,0x484a,0x48ae,0x4912,0x4975,0x49d9,0x4a3c,0x4a9f,0x4b02,0x4b65,0x4bc8,0x4c2b,
  0x4c8d,0x4cf0,0x4d52,0x4db4,0x4e16,0x4e79,0x4eda,0x4f3c,0x4f9e,0x5000,0x5061,0x50c2,0x5124,0x5185,0x51e6,0x5247,
  0x52a7,0x5308,0x5369,0x53c9,0x542a,0x548a,0x54ea,0x554a,0x55aa,0x560a,0x566a,0x56c9,0x5729,0x5788,0x57e8,0x5847,
  0x58a6,0x5905,0x5964,0x59c3,0x5a22,0x5a80,0x5adf,0x5b3d,0x5b9b,0x5bfa,0x5c58,0x5cb6,0x5d14,0x5d71,0x5dcf,0x5e2d,
  0x5e8a,0x5ee8,0x5f45,0x5fa2,0x6000,0x605d,0x60b9,0x6116,0x6173,0x61d0,0x622c,0x6289,0x62e5,0x6341,0x639e,0x63fa,
  0x6456,0x64b2,0x650d,0x6569,0x65c5,0x6620,0x667c,0x66d7,0x6732,0x678e,0x67e9,0x6844,0x689f,0x68f9,0x6954,0x69af,
  0x6a09,
};

static unsigned int ISqrtTab[257] =
{
  0x40000000,0x3fe017ec,0x3fc05f61,0x3fa0d5e9,0x3f817b11,0x3f624e65,0x3f434f76,0x3f247dd4,0x3f05d910,0x3ee760be,0x3ec91474,0x3eaaf3c7,0x3e8cfe50,0x3e6f33a7,0x3e519367,0x3e341d2b,
  0x3e16d091,0x3df9ad37,0x3ddcb2bd,0x3dbfe0c3,0x3da336eb,0x3d86b4d9,0x3d6a5a31,0x3d4e2698,0x3d3219b5,0x3d163330,0x3cfa72b2,0x3cded7e4,0x3cc36271,0x3ca81207,0x3c8ce650,0x3c71defd,
  0x3c56fbbb,0x3c3c3c3c,0x3c21a02f,0x3c072747,0x3becd136,0x3bd29db2,0x3bb88c6d,0x3b9e9d1f,0x3b84cf7d,0x3b6b233f,0x3b51981c,0x3b382dcf,0x3b1ee411,0x3b05ba9d,0x3aecb12e,0x3ad3c780,
  0x3abafd52,0x3aa2525f,0x3a89c668,0x3a71592b,0x3a590a69,0x3a40d9e3,0x3a28c759,0x3a10d28e,0x39f8fb46,0x39e14143,0x39c9a44a,0x39b22421,0x399ac08b,0x39837951,0x396c4e38,0x39553f09,
  0x393e4b8b,0x39277387,0x3910b6c7,0x38fa1514,0x38e38e38,0x38cd2200,0x38b6d037,0x38a098a8,0x388a7b21,0x38747770,0x385e8d61,0x3848bcc3,0x38330565,0x381d6717,0x3807e1a9,0x37f274eb,
  0x37dd20ad,0x37c7e4c2,0x37b2c0fc,0x379db52c,0x3788c125,0x3773e4bc,0x375f1fc3,0x374a720f,0x3735db74,0x37215bc8,0x370cf2e1,0x36f8a093,0x36e464b6,0x36d03f21,0x36bc2faa,0x36a8362a,
  0x36945278,0x3680846c,0x366ccbe1,0x365928ae,0x36459aad,0x363221b9,0x361ebdac,0x360b6e60,0x35f833b0,0x35e50d79,0x35d1fb95,0x35befde1,0x35ac143a,0x35993e7b,0x35867c83,0x3573ce2f,
  0x3561335d,0x354eabea,0x353c37b5,0x3529d69e,0x35178883,0x35054d43,0x34f324bf,0x34e10ed5,0x34cf0b68,0x34bd1a56,0x34ab3b82,0x34996ecc,0x3487b415,0x34760b40,0x3464742f,0x3452eec3,
  0x34417ae0,0x34301867,0x341ec73d,0x340d8745,0x33fc5862,0x33eb3a78,0x33da2d6c,0x33c93121,0x33b8457c,0x33a76a63,0x33969fb9,0x3385e566,0x33753b4d,0x3364a156,0x33541765,0x33439d62,
  0x33333333,0x3322d8be,0x33128deb,0x330252a0,0x32f226c6,0x32e20a43,0x32d1fcff,0x32c1fee3,0x32b20fd7,0x32a22fc2,0x32925e8e,0x32829c24,0x3272e86d,0x32634351,0x3253acba,0x32442492,
  0x3234aac2,0x32253f35,0x3215e1d5,0x3206928b,0x31f75143,0x31e81de7,0x31d8f862,0x31c9e0a0,0x31bad68a,0x31abda0e,0x319ceb15,0x318e098d,0x317f3560,0x31706e7c,0x3161b4cb,0x3153083c,
  0x314468b9,0x3135d630,0x3127508e,0x3118d7bf,0x310a6bb2,0x30fc0c52,0x30edb98e,0x30df7353,0x30d1398f,0x30c30c30,0x30b4eb24,0x30a6d659,0x3098cdbe,0x308ad140,0x307ce0cf,0x306efc59,
  0x306123cd,0x3053571a,0x3045962f,0x3037e0fc,0x302a3770,0x301c997b,0x300f070b,0x30018012,0x2ff4047e,0x2fe69440,0x2fd92f47,0x2fcbd586,0x2fbe86ea,0x2fb14366,0x2fa40aea,0x2f96dd66,
  0x2f89bacc,0x2f7ca30c,0x2f6f9617,0x2f6293df,0x2f559c55,0x2f48af6b,0x2f3bcd11,0x2f2ef53a,0x2f2227d7,0x2f1564da,0x2f08ac35,0x2efbfddb,0x2eef59bd,0x2ee2bfcd,0x2ed62ffe,0x2ec9aa42,
  0x2ebd2e8d,0x2eb0bcd0,0x2ea454fe,0x2e97f70b,0x2e8ba2e8,0x2e7f588a,0x2e7317e3,0x2e66e0e7,0x2e5ab388,0x2e4e8fbb,0x2e427572,0x2e3664a2,0x2e2a5d3e,0x2e1e5f3a,0x2e126a89,0x2e067f20,
  0x2dfa9cf2,0x2deec3f4,0x2de2f41a,0x2dd72d58,0x2dcb6fa2,0x2dbfbaee,0x2db40f2f,0x2da86c5a,0x2d9cd263,0x2d914140,0x2d85b8e6,0x2d7a3948,0x2d6ec25d,0x2d635419,0x2d57ee72,0x2d4c915c,
  0x2d413ccc
};

/****************************************************************************/

//#include <math.h>
void InitBitmaps()
{
  sInt i,j;
  sInt x,y;

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

  for(i=0;i<256;)
  {
    x = sGetRnd(0x4000)-0x2000;
    y = sGetRnd(0x4000)-0x2000;
    if(x*x+y*y<0x2000*0x2000)
    {
      PerlinRandom[i][0] = x;
      PerlinRandom[i][1] = y;
      i++;
    }
  }

// SQRT
/*
  for(i=0;i<=256;i++)
    SqrtTab[i] = (sqrt((i+256)/256.0f)*0x10000)-0x10000;

  for(i=0;i<=256;i++)
    ISqrtTab[i] = (1<<30) / sqrt((i+256)/256.0f);
*/


/*
  for(i=0;i<64;i++)
  {
    j = i;
    sDPrintF(sTXT("%08x -> %08x (float %08x old %08x)\n"),j,ISqrt15D(j),(sInt)((1<<30)/sqrt((sU32)j*1.0f)),ISqrt15D(j));
  }
  for(i=0;i<256;i++)
  {
    j = 0xffffffff-i*0x1000000;
    sDPrintF(sTXT("%08x -> %08x (float %08x old %08x)\n"),j,ISqrt15D(j),(sInt)((1<<30)/sqrt((sU32)j*1.0f)),ISqrt15D(j));
  }
  for(i=0;i<257;i++)
  {
    if((i&15)==0)
      sDPrintF(sTXT("\n  "));
    sDPrintF(sTXT("0x%04x,"),ISqrtTab[i]);
  }

  sInt e0=0,e1=0;
  for(sU64 u=0x00100000;u<0xffffffff;u+=67)
  {
    sInt a = ISqrt15D(u);
    sInt b = (sInt)((1<<30)/sqrt(u*1.0f));
    if(a!=0)
    {
      if(a-b<0) e0++;
      if(a-b>0) e1++;
      if(a-b<-2 || a-b>2)
        sDPrintF(sTXT("error %08x -> %04x | %04x\n"),(sU32)u,a,b);
    }
    if((u&0xffffff)==0)
    {
      sDPrintF(sTXT("%08x"),(sU32)u);
      sDPrintF(sTXT("  errors: %08x < 0 < %08x\n"),e0,e1);
    }
  }

  sDPrintF(sTXT("\n"));
*/
}

/****************************************************************************/

sInt IntPower(sInt value,sInt power)
{
  sInt v1;
  switch(power>>15)
  {
  default:
  case 16: value = (value * value)>>15;
  case 15: value = (value * value)>>15;
  case 14: value = (value * value)>>15;
  case 13: value = (value * value)>>15;
  case 12: value = (value * value)>>15;
  case 11: value = (value * value)>>15;
  case 10: value = (value * value)>>15;
  case 9: value = (value * value)>>15;
  case 8: value = (value * value)>>15;
  case 7: value = (value * value)>>15;
  case 6: value = (value * value)>>15;
  case 5: value = (value * value)>>15;
  case 4: value = (value * value)>>15;
  case 3: value = (value * value)>>15;
  case 2: value = (value * value)>>15;
  case 1: value = (value * value)>>15;
  case 0:
    v1 = (value * value)>>15;
    return value+(((v1-value)*(power&0x7fff))>>15);
  }
}

sInt GenBitmapTile::Sin15(sInt x)
{
  return sISinOld(x*2)/2;
}

sInt GenBitmapTile::Cos15(sInt x)
{
  return sICosOld(x*2)/2;
}

void GenBitmapTile::SinCos15(sInt x,sInt &s,sInt &c)
{
  s = sISinOld(x*2)/2;
  c = sICosOld(x*2)/2;
}

sInt GenBitmapTile::Sqrt15D(sU32 r)
{
  sU32 e = sCountLeadingZeroes(r);
  sU32 x = r<<(e+1);
  sU32 i = x>>24;
  sU32 f = (x&0x00ffffff);
  
  x =  SqrtTab[i] + Mul24(SqrtTab[i+1]-SqrtTab[i],f) + 0x10000;
  if(e&1) x=x>>1;
  else x=Mul24(x+1,11863283);
  return (x>>(e/2));
}

sInt GenBitmapTile::ISqrt15D(sU32 r)
{
  sU32 e = sCountLeadingZeroes(r);
  sU32 x = r<<(e+1);
  sU32 i = x>>24;
  sU32 f = (x&0x00ffffff);
  
  x =  ISqrtTab[i] + Mul24(ISqrtTab[i+1]-ISqrtTab[i],f);
  if(!(e&1) )
    x=Mul24(x+1,11863283);

  return (x>>(15-e/2));
}

sInt GenBitmapTile::Log15(sInt x) // base-2 logarithm (assumes a 32bit architecture)
{
	if(x <= 0)
		return -128 << 15; // "big"

	sInt lz = sCountLeadingZeroes(x);
	sU32 mantissa = x << (lz + 1);

  sInt index = mantissa >> (32-8);
  sInt fade = (mantissa >> (32-16)) & 255;
  sInt v0 = Log2Tab[index], v1 = Log2Tab[index+1];
  sInt v = (v0 << 8) + (v1 - v0) * fade;

  return (v >> 8) - ((lz - 16) << 15);
}

sInt GenBitmapTile::ExpClip15(sInt x) // base-2 power function, clips at 1
{
	if(x <= -(15 << 15))
		return 0;
	else if(x >= 0)
		return 32768;
	else
  {
    sInt index = (x >> 7) & 0xff;
    sInt fade = x & 0x7f;
    sInt v0 = Exp2Tab[index], v1 = Exp2Tab[index+1];
    sInt v = (v0 << 7) + (v1 - v0) * fade;

    return v >> ((7*32768 - x - 1) >> 15);
  }
}

sInt GenBitmapTile::Pow15(sInt x,sInt y)
{
  return x ? ExpClip15(Mul15(Log15(x),y)) : 0;
}
/*
sInt Sqrt15Fast(sInt x) // assumes x<1!!!!
{
  sInt temp, g=0;

  x <<= 15;

  if (x >= 0x10000000)
  {
    g = 0x4000; 
    x -= 0x10000000;
  }

#define INNER_SQRT(s)                         \
  temp = (g << (s)) + (1 << ((s) * 2 - 2));   \
  if (x >= temp) {                            \
    g += 1 << ((s)-1);                        \
    x -= temp;                                \
  }

  INNER_SQRT(14)
  INNER_SQRT(13)
  INNER_SQRT(12)
  INNER_SQRT(11)
  INNER_SQRT(10)
  INNER_SQRT( 9)
  INNER_SQRT( 8)
  INNER_SQRT( 7)
  INNER_SQRT( 6)
  INNER_SQRT( 5)
  INNER_SQRT( 4)
  INNER_SQRT( 3)
  INNER_SQRT( 2)
#undef INNER_SQRT

  if(x >= g+g+1)
    g++;

  return g;
}
*/
static sInt Perlin2(sInt x,sInt y,sInt mask,sInt seed)
{
  sInt v00,v01,v10,v11,vx,vy;
  sInt f00,f01,f10,f11;
  sInt tx,ty,fa,fb,f;
  sInt ttx,tty;

  mask &= 255;
  vx = (x>>15) & mask; tx=(x&0x7fff);
  vy = (y>>15) & mask; ty=(y&0x7fff);
  v00 = PerlinPermute[((vx+0)     )+PerlinPermute[((vy+0)     )^seed]];
  v01 = PerlinPermute[((vx+1)&mask)+PerlinPermute[((vy+0)     )^seed]];
  v10 = PerlinPermute[((vx+0)     )+PerlinPermute[((vy+1)&mask)^seed]];
  v11 = PerlinPermute[((vx+1)&mask)+PerlinPermute[((vy+1)&mask)^seed]];
  f00 = (PerlinRandom[v00][0]*(tx-0x0000)+PerlinRandom[v00][1]*(ty-0x0000))>>14;
  f01 = (PerlinRandom[v01][0]*(tx-0x8000)+PerlinRandom[v01][1]*(ty-0x0000))>>14;
  f10 = (PerlinRandom[v10][0]*(tx-0x0000)+PerlinRandom[v10][1]*(ty-0x8000))>>14;
  f11 = (PerlinRandom[v11][0]*(tx-0x8000)+PerlinRandom[v11][1]*(ty-0x8000))>>14;
  ttx = 10*0x8000+ Mul15(tx, (6*tx-15*0x8000));
  tty = 10*0x8000+ Mul15(ty, (6*ty-15*0x8000));
  tx = Mul15(((((tx*tx)>>15) *tx)>>15) ,ttx);
  ty = Mul15(((((ty*ty)>>15) *ty)>>15) ,tty);
//  tx = (((tx*tx)>>15)*(0x18000-2*tx))>>15;
//  ty = (((ty*ty)>>15)*(0x18000-2*ty))>>15;
  fa = f00+(((f01-f00)*tx)>>15);
  fb = f10+(((f11-f10)*tx)>>15);
  f = fa+(((fb-fa)*ty)>>15);
 // f = sRange(f,0x8000,-0x8000);

  return f*2;
}

/****************************************************************************/
/***                                                                      ***/
/***   Helpers                                                            ***/
/***                                                                      ***/
/****************************************************************************/

#if sPLAYER

template <class Format> void DoTileHandler(struct GenNode *node,Tile<Format> *tile)
{
  sVERIFY(node);
  sVERIFY(node->Handler);

  if(node)
  {
    void (*handler)(GenNode *,Tile<Format> *) = (void (*)(GenNode *,Tile<Format> *)) node->Handler;
    (*handler)(node,tile);
#if sMOBILE
    if(tile->Flags & TF_FIRST) sProgress(16);
#endif
  }
}

#else

GenBitmap *FindBitmapInCache(sAutoArray<GenObject> *cache,sInt xs,sInt ys,sInt var)
{
  GenObject *obj;
  GenBitmap *bm;
  sFORLIST(cache,obj)
  {
    if(obj->Variant==var)
    {
      bm = (GenBitmap *)obj;

      if(bm->XSize==xs && bm->YSize==ys)
        return bm;
    }
  }
  return 0;
}


GenBitmap *CalcBitmap(GenOp *op,sInt xs,sInt ys,sInt var)
{
  GenBitmap *bm;

  op = FollowOp(op);

  bm = FindBitmapInCache(&op->Cache,xs,ys,var);

  if(!bm)
  {
    bm = new GenBitmap;
    switch(var)
    {
    case GenBitmapTile::Pixel16C::Variant:
      bm->Init<GenBitmapTile::Pixel16C>(xs,ys);
      GenBitmapTile_Calc(op,bm);
      break;
    case GenBitmapTile::Pixel16I::Variant:
      bm->Init<GenBitmapTile::Pixel16I>(xs,ys);
      GenBitmapTile_Calc(op,bm);
      break;
    case GenBitmapTile::Pixel8C::Variant:
      bm->Init<GenBitmapTile::Pixel8C>(xs,ys);
      GenBitmapTile_Calc(op,bm);
      break;
    case GenBitmapTile::Pixel8I::Variant:
      bm->Init<GenBitmapTile::Pixel8I>(xs,ys);
      GenBitmapTile_Calc(op,bm);
      break;
    }
    delete bm;
  }

  bm = FindBitmapInCache(&op->Cache,xs,ys,var);

  return bm;
}

template <class Format> void DoTileHandler(struct GenNode *node,Tile<Format> *tile)
{
  GenBitmap *bm;
  sVERIFY(node);
  sVERIFY(node->Handler);

  sVERIFY(node->Variant==Format::Variant);
  sVERIFY(node->SizeX = tile->SizeX);
  sVERIFY(node->SizeY = tile->SizeY);

  if(node)
  {
    bm = 0;

    if(node->Cache)               
    {
      bm = FindBitmapInCache(node->Cache,tile->SizeX,tile->SizeY,Format::Variant);
  
      if(!bm)                     // create cache
      {
        bm = new GenBitmap;
        node->Cache->Add(bm);
        bm->Init<Format>(tile->SizeX,tile->SizeY);
      }
    }

    if(bm && bm->Valid)           // get cached version
    {
      sCopyMem(tile->Data,bm->GetAdr(tile->OffsetX,tile->OffsetY),BMTILE_COUNT*sizeof(Format));
    }
    else                          // no cache available
    {
      void (*handler)(GenNode *,Tile<Format> *) = (void (*)(GenNode *,Tile<Format> *)) node->Handler;
      (*handler)(node,tile);
      if(tile->Flags & TF_FIRST)
        node->Op->CalcCount++;

      if(bm)                      // store to cache
      {
        sCopyMem(bm->GetAdr(tile->OffsetX,tile->OffsetY),tile->Data,BMTILE_COUNT*sizeof(Format));
        if(tile->Flags & TF_LAST) // mark cache as valid when last tile is rendered
          bm->Valid = sTRUE;
      }
    }
  }
}

#endif

/****************************************************************************/

template <class Format> Bitmap<Format> *BeginFullHandler(struct GenNode *node,Tile<Format> *tile)
{
  Bitmap<Format> *bm;
  TileNode<Format> tn;

  bm = (Bitmap<Format> *) node->FullData;
  if(tile->Flags & TF_FIRST)
  {
    if(!bm)
    {
      bm = new Bitmap<Format>(tile->SizeX,tile->SizeY,tile->ShiftX,tile->ShiftY);
      bm->RefCount=0;
      tn.Buffer = bm;
      tn.Node = node;
      tn.Calc();
      node->FullData = bm;
#if !sPLAYER
      if(tile->Flags & TF_FIRST)
        node->Op->CalcCount++;
#endif
    }
    else
    {
//      sDPrintF(sTXT("what?\n"));
    }
    bm->RefCount++;
  }
  sVERIFY(bm->SizeX==tile->SizeX && bm->SizeY==tile->SizeY);
  return bm;
}

template <class Format> void EndFullHandler(struct GenNode *node,Tile<Format> *tile)
{
  Bitmap<Format> *bm;

  bm = (Bitmap<Format> *) node->FullData;
  sVERIFY(bm);
  if(tile->Flags & TF_LAST)
  {
    bm->RefCount--;
    if(bm->RefCount<=0)
    {
      node->FullData = 0;
      delete bm;
    }
  }
}

#define TILE_MAKE(x) \
void x##_8I (struct GenNode *node,Tile<Pixel8I>  *tile) { x<Pixel8I >(node,tile); } \
void x##_8C (struct GenNode *node,Tile<Pixel8C>  *tile) { x<Pixel8C >(node,tile); } \
void x##_16I(struct GenNode *node,Tile<Pixel16I> *tile) { x<Pixel16I>(node,tile); } \
void x##_16C(struct GenNode *node,Tile<Pixel16C> *tile) { x<Pixel16C>(node,tile); }

#define TILE_ADD(x) \
Handlers.Set(GVI_BITMAP_TILE8I ,x##_8I);  \
Handlers.Set(GVI_BITMAP_TILE8C ,x##_8C);  \
Handlers.Set(GVI_BITMAP_TILE16I,x##_16I); \
Handlers.Set(GVI_BITMAP_TILE16C,x##_16C);

#define TILE_ARRAY(y,x) \
{ x##_8I ,y+0 }, \
{ x##_8C ,y+1 }, \
{ x##_16I,y+2 }, \
{ x##_16C,y+3 },

/****************************************************************************/

#if sPLAYER

sDInt NodeMemIndex = 0;
const sDInt NodeMemMax = 0x40000;
static sU8 NodeMem[NodeMemMax];

sU8 *AllocNodeMemX(sInt size)
{
  if(NodeMemIndex+size>=NodeMemMax)
    sFatal(sTXT("out of nodemem"));
  sU8 *result = NodeMem+NodeMemIndex;
  NodeMemIndex = sAlign(NodeMemIndex+size,16);
  return result;
}

template <class Type> Type *AllocNodeMem(sInt count=1)
{
  return (Type *) AllocNodeMemX(sizeof(Type)*count);
}

#else

template <class Type> Type *AllocNodeMem(sInt count=1)
{
  return Doc->AllocNode<Type>(count);
}

#endif

/****************************************************************************/

template <class Format> struct TileNode
{
  Bitmap<Format> *Buffer;
  GenNode *Node;

  void Calc()
  {
    Tile<Format> tile;
    tile.Init(0,Buffer->SizeX,Buffer->SizeY);
    sInt i = tile.SizeX*tile.SizeY/(BMTILE_SIZE*BMTILE_SIZE);
    for(tile.OffsetY = 0; tile.OffsetY<tile.SizeY; tile.OffsetY+=BMTILE_SIZE)
    {
      for(tile.OffsetX = 0; tile.OffsetX<tile.SizeX; tile.OffsetX+=BMTILE_SIZE)
      {
        if(--i==0)
          tile.Flags |= TF_LAST;
        tile.Data = &Buffer->Adr(tile.OffsetX,tile.OffsetY);
        DoTileHandler<Format>(Node,&tile);
        tile.Flags = 0;
      }
    }
  }

#if !sPLAYER
  /*
  void CopyToBitmap(GenBitmap *bm)
  {
    sVERIFY(bm->XSize==Buffer->SizeX);
    sVERIFY(bm->YSize==Buffer->SizeY);
    sVERIFY((Buffer->SizeX&BMTILE_MASK)==0);
    sVERIFY((Buffer->SizeY&BMTILE_MASK)==0);
    for(sInt y=0;y<bm->YSize;y++)
    {
      for(sInt x=0;x<bm->XSize;x++)
      {
        sInt r,g,b,a;

        Buffer->Adr(x,y).Get(r,g,b,a);
        ((sU16*)bm->Data)[(y*bm->XSize+x)*4+0] = r;
        ((sU16*)bm->Data)[(y*bm->XSize+x)*4+1] = g;
        ((sU16*)bm->Data)[(y*bm->XSize+x)*4+2] = b;
        ((sU16*)bm->Data)[(y*bm->XSize+x)*4+3] = a;
      }
    }
  }
*/
  void Do(GenBitmap *bm,GenNode *node)
  {
    sVERIFY(bm->Variant == Format::Variant);
    Buffer = new Bitmap<Format>(bm->XSize,bm->YSize,sGetPower2(bm->XSize),sGetPower2(bm->YSize));
    Node = node;
    Calc();
    sCopyMem(bm->Data,Buffer->Data,bm->XSize*bm->YSize*sizeof(Format));
//    CopyToBitmap(bm);
    delete Buffer;
  }
#endif
/*
  void CopyToBitmap(sInt xs,sInt ys,sU32 *data)
  {
    sVERIFY(xs==Buffer->SizeX);
    sVERIFY(ys==Buffer->SizeY);
    sVERIFY((Buffer->SizeX&BMTILE_MASK)==0);
    sVERIFY((Buffer->SizeY&BMTILE_MASK)==0);
    for(sInt y=0;y<xs;y++)
    {
      for(sInt x=0;x<ys;x++)
      {
        data[y*xs+x] = Buffer->Adr(x,y).Get32();
      }
    }
  }
*/
  void Do(sInt xs,sInt ys,sU32 *data,GenNode *node)
  {
    Buffer = new Bitmap<Format>(xs,ys,sGetPower2(xs),sGetPower2(ys));
    Node = node;
    Calc();

    sVERIFY(xs==Buffer->SizeX);
    sVERIFY(ys==Buffer->SizeY);
    for(sInt y=0;y<xs;y++)
    {
      for(sInt x=0;x<ys;x++)
      {
        sInt r,g,b,a;

        Buffer->Adr(x,y).Get(r,g,b,a);
        ((sU8*)data)[(y*xs+x)*4+0] = r>>7;
        ((sU8*)data)[(y*xs+x)*4+1] = g>>7;
        ((sU8*)data)[(y*xs+x)*4+2] = b>>7;
        ((sU8*)data)[(y*xs+x)*4+3] = a>>7;
      }
    }

    delete Buffer;
  }
};

void GenBitmapTile_Calc(GenNode *node,sInt xs,sInt ys,sU32 *data)
{
  TileNode<Pixel8C> tn;
  tn.Do(xs,ys,data,node);
}

#if !sPLAYER
void GenBitmapTile_Calc(GenOp *op,GenBitmap *bm)
{
  Doc->MakeNodePrepare();
  GenNode *node = Doc->MakeNodeTree(op,bm->Variant,bm->XSize,bm->YSize);
  if(!node)
  {
    sSetMem(bm->Data,~0,bm->BytesTotal);
    return;
  }

  switch(bm->Variant)
  {
  case GVI_BITMAP_TILE8I:
    {
      TileNode<Pixel8I> tn;
      tn.Do(bm,node);
    }
    break;
  case GVI_BITMAP_TILE16I:
    {
      TileNode<Pixel16I> tn;
      tn.Do(bm,node);
    }
    break;
  case GVI_BITMAP_TILE8C:
    {
      TileNode<Pixel8C> tn;
      tn.Do(bm,node);
    }
    break;
  case GVI_BITMAP_TILE16C:
    {
      TileNode<Pixel16C> tn;
      tn.Do(bm,node);
    }
    break;
  }
}
#endif

/****************************************************************************/
/****************************************************************************/
/*
void BitmapCache::Flush()
{
  if(Object)
  {
    Bitmap<Pixel16C> *bm;
    bm = (Bitmap<Pixel16C> *) Object;
    delete bm;
    Object = 0;
    Variant = 0;
  }
}

template <class Format>Bitmap<Format> * BitmapCache::Get(Tile<Format> *tile)
{
  Bitmap<Format> *cache = (Bitmap<Format> *) Object;
  if(cache)
  {
    if(tile->Data[0].Variant!=Variant)
    {
      Flush();
      cache = 0;
    }
    else if(cache->SizeX!=tile->SizeX || cache->SizeY!=tile->SizeY)
    {
      Flush();
      cache = 0;
    }
  }

  return cache;
}

template <class Format> Bitmap<Format> * BitmapCache::Set(Tile<Format> *tile)
{
  Bitmap<Format> *cache = Get(tile);
  if(!cache)
  {
    cache = new Bitmap<Format>(tile->SizeX,tile->SizeY);
    Object = cache;
  }
  return cache;
}
*/

template <class Format> Bitmap<Format> *GetCacheBitmap(GenObject **cache,Tile<Format> *tile)
{
  /*

  Bitmap<Format> *cache = (Bitmap<Format> *) Object;
  if(cache)
  {
    if(tile->Data[0].Variant!=Variant)
    {
      Flush();
      cache = 0;
    }
    else if(cache->SizeX!=tile->SizeX || cache->SizeY!=tile->SizeY)
    {
      Flush();
      cache = 0;
    }
  }
*/
  return 0;
}


GenBitmap::GenBitmap()
{
  Data = 0;
  XSize = 0;
  YSize = 0;
  Pixels = 0;
  BytesPerPixel = 0;
  BytesTotal = 0;
  Variant = 0;
  Valid = 0;

#if !sMOBILE
  Format = sTF_A8R8G8B8;
  Texture = sINVALID;
  TexMipCount = 0;
  TexMipTresh = 0;
#endif

  Soft = 0;
}

GenBitmap::~GenBitmap()
{
  sDeleteArray(Data);
#if !sMOBILE
  if(Texture!=sINVALID) sSystem->RemTexture(Texture);
#endif

  delete Soft;
}

void GenBitmap::Copy(GenBitmap *ob)
{
  sVERIFY(ob->Data);
  sVERIFY(!Data);
#if !sMOBILE
  sVERIFY(Texture == sINVALID);
#endif

  XSize = ob->XSize;
  YSize = ob->YSize;
  Pixels = ob->Pixels;
  BytesPerPixel = ob->BytesPerPixel;
  BytesTotal = ob->BytesTotal;
  Variant = ob->Variant;
  Data = new sU8[XSize*YSize*4];
  sCopyMem4((sU32 *)Data,(sU32 *)ob->Data,BytesTotal/4);
}

GenBitmap *GenBitmap::Copy()
{
  GenBitmap *r;
  r = new GenBitmap;
  r->Copy(this);
  return r;
}

#if !sMOBILE
void GenBitmap::MakeTexture(sInt format)
{
  sInt x,y,z;
  if(Texture!=sINVALID)
    sSystem->RemTexture(Texture);
  if(format!=0)
    Format = format;

  sU16 *dest = new sU16[4*XSize*YSize];
  sU16 *p;
  sU16 *s16;
  sU8 *s8;

  switch(Variant)
  {
  default:
    sVERIFYFALSE;
    break;

  case Pixel16C::Variant:
    s16 = (sU16 *) Data;
    for(z=0;z<XSize;z+=BMTILE_SIZE)
    {
      for(y=0;y<YSize;y++)
      {
        for(x=0;x<BMTILE_SIZE;x++)
        {
          p = dest + 4*(x+z) + 4*XSize*y;
          p[0] = s16[0];
          p[1] = s16[1];
          p[2] = s16[2];
          p[3] = s16[3];
          s16+=4;
        }
      }
    }
    break;

  case Pixel16I::Variant:
    s16 = (sU16 *) Data;
    for(z=0;z<XSize;z+=BMTILE_SIZE)
    {
      for(y=0;y<YSize;y++)
      {
        for(x=0;x<BMTILE_SIZE;x++)
        {
          p = dest + 4*(x+z) + 4*XSize*y;
          p[0] = s16[0];
          p[1] = s16[0];
          p[2] = s16[0];
          p[3] = 0x7fff;
          s16+=1;
        }
      }
    }
    break;

  case Pixel8C::Variant:
    s8 = (sU8 *) Data;
    p = dest;
    for(y=0;y<YSize;y++)
    {
      for(x=0;x<XSize;x++)
      {
        s8 = GetAdr(x,y);
        p[0] = (s8[0]<<7)|(s8[0]>>1);
        p[1] = (s8[1]<<7)|(s8[1]>>1);
        p[2] = (s8[2]<<7)|(s8[2]>>1);
        p[3] = (s8[3]<<7)|(s8[3]>>1);
        p+=4;
      }
    }
    break;

  case Pixel8I::Variant:
    s8 = (sU8 *) Data;
    p = dest;
    for(y=0;y<YSize;y++)
    {
      for(x=0;x<XSize;x++)
      {
        s8 = GetAdr(x,y);
        p[0] = (s8[0]<<7)|(s8[0]>>1);
        p[1] = (s8[0]<<7)|(s8[0]>>1);
        p[2] = (s8[0]<<7)|(s8[0]>>1);
        p[3] = 0x7fff;
        p+=4;
      }
    }
    break;
  }

#if !sMOBILE && (DEMO_VERSION>=2)
  for(sInt y=0;y<YSize;y++) 
  {
    sInt dy = sAbs(y-YSize/2);
    for(sInt x=0;x<XSize;x++) 
    {
      sInt dx = sAbs(x-XSize/2);
      if((dx>dy?dx+dy*4:dy+dx*4)<(XSize+YSize)/8)
        ((sU64 *)dest)[y*XSize+x] = 0x7fff404040404040ULL; 
    }
  }
#endif

  Texture = sSystem->AddTexture(XSize,YSize,Format,dest,TexMipCount,TexMipTresh);

  delete[] dest;
}


sBitmap *GenBitmap::MakeBitmap()
{
  sInt x,y,z;
  sBitmap *bm = new sBitmap();
  bm->Init(XSize,YSize);

  sU8 *dest = (sU8*) bm->Data;
  sU8 *p;
  sU16 *s16;
  sU8 *s8;

  switch(Variant)
  {
  default:
    sVERIFYFALSE;
    break;

  case Pixel16C::Variant:
    s16 = (sU16 *) Data;
    for(z=0;z<XSize;z+=BMTILE_SIZE)
    {
      for(y=0;y<YSize;y++)
      {
        for(x=0;x<BMTILE_SIZE;x++)
        {
          p = dest + 4*(x+z) + 4*XSize*y;
          p[0] = s16[0]>>7;
          p[1] = s16[1]>>7;
          p[2] = s16[2]>>7;
          p[3] = s16[3]>>7;
          s16+=4;
        }
      }
    }
    break;

  case Pixel16I::Variant:
    s16 = (sU16 *) Data;
    for(z=0;z<XSize;z+=BMTILE_SIZE)
    {
      for(y=0;y<YSize;y++)
      {
        for(x=0;x<BMTILE_SIZE;x++)
        {
          p = dest + 4*(x+z) + 4*XSize*y;
          p[0] = s16[0]>>7;
          p[1] = s16[0]>>7;
          p[2] = s16[0]>>7;
          p[3] = 0xff;
          s16+=1;
        }
      }
    }
    break;

  case Pixel8C::Variant:
    s8 = (sU8 *) Data;
    p = dest;
    for(y=0;y<YSize;y++)
    {
      for(x=0;x<XSize;x++)
      {
        s8 = GetAdr(x,y);
        p[0] = s8[0];
        p[1] = s8[1];
        p[2] = s8[2];
        p[3] = s8[3];
        p+=4;
      }
    }
    break;

  case Pixel8I::Variant:
    s8 = (sU8 *) Data;
    p = dest;
    for(y=0;y<YSize;y++)
    {
      for(x=0;x<XSize;x++)
      {
        s8 = GetAdr(x,y);
        p[0] = s8[0];
        p[1] = s8[0];
        p[2] = s8[0];
        p[3] = 0xff;
        p+=4;
      }
    }
    break;
  }

  
#if !sMOBILE && (DEMO_VERSION>=2)
  for(sInt y=0;y<YSize;y++) 
  {
    sInt dy = sAbs(y-YSize/2);
    for(sInt x=0;x<XSize;x++) 
    {
      sInt dx = sAbs(x-XSize/2);
      if((dx>dy?dx+dy*4:dy+dx*4)<(XSize+YSize)/8)
        ((sU32 *)dest)[y*XSize+x] = 0xff808080UL; 
    }
  }
#endif

  return bm;
}
#endif

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Per Tile                                                           ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/


template <class Format> void Tile_NodeLoad(struct GenNode *node,Tile<Format> *tile)
{

  DoTileHandler<Format>(node->Links[0],tile);
/*
  Bitmap<Format> *bm;
  Format *s,*d;
  sInt i;


  sVERIFY(node->LinkCount==1);
  sVERIFY(node->Links[0]);

  bm = BeginFullHandler<Format>(node->Links[0],tile);
  
  d = tile->Data;
  s = &bm->Adr(tile->OffsetX,tile->OffsetY);
  for(i=0;i<BMTILE_COUNT;i++)
    *d++ = *s++;

  EndFullHandler(node->Inputs[0],tile);
  */
}

TILE_MAKE(Tile_NodeLoad)

template <class Format> void Tile_NodeStore(struct GenNode *node,Tile<Format> *tile)
{
  Bitmap<Format> *bm;
  Format *s,*d;
  sInt i;

  if(node->StoreCache==0)
  {
    bm = BeginFullHandler<Format>(node->Inputs[0],tile);
    bm->RefCount += node->Para[0]+9999;
    node->StoreCache = bm;
  }

  bm = (Bitmap<Format> *) node->StoreCache;

  d = tile->Data;
  s = &bm->Adr(tile->OffsetX,tile->OffsetY);
  for(i=0;i<BMTILE_COUNT;i++)
    *d++ = *s++;

  EndFullHandler(node->Inputs[0],tile);
}

TILE_MAKE(Tile_NodeStore)

/****************************************************************************/

template <class Format> void Tile_Flat(struct GenNode *node,Tile<Format> *tile)
{
  Format color;

  PA(L"Tile_Flat");

  color.Set32(node->Para[3]);

  for(sInt i=0;i<BMTILE_COUNT;i++)
    tile->Data[i] = color;
}

TILE_MAKE(Tile_Flat)

/****************************************************************************/

template <class Format> void Tile_Color(struct GenNode *node,Tile<Format> *tile)
{
  Format color;
  sInt i;

  DoTileHandler<Format>(node->Inputs[0],tile);
  color.Set32(node->Para[1]);

  PA(L"Tile_Color");

  // "mul|add|sub|gray|invert|scale"

  switch(node->Para[0])
  {
  case 0: // mul;
    for(i=0;i<BMTILE_COUNT;i++)
      tile->Data[i].Mul(tile->Data[i],color);
    break;
  case 1: // add;
    for(i=0;i<BMTILE_COUNT;i++)
      tile->Data[i].Add(tile->Data[i],color);
    break;
  case 2: // sub;
    for(i=0;i<BMTILE_COUNT;i++)
      tile->Data[i].Sub(tile->Data[i],color);
    break;
  case 3: // gray;
    for(i=0;i<BMTILE_COUNT;i++)
      tile->Data[i].Gray();
    break;
  case 4: // invert;
    {
      Format white; white.One();
      for(i=0;i<BMTILE_COUNT;i++)
        tile->Data[i].XOr(tile->Data[i],white);
    }
    break;
  case 5: // scale;
    for(i=0;i<BMTILE_COUNT;i++)
      tile->Data[i].MulScale(tile->Data[i],color,4);
    break;
  }
}

TILE_MAKE(Tile_Color)

/****************************************************************************/

template <class Format> void Tile_Add(struct GenNode *node,Tile<Format> *tile)
{
  Tile<Format> tileb;
  Format data[BMTILE_SIZE*BMTILE_SIZE];
  sInt i,input;


  DoTileHandler<Format>(node->Inputs[0],tile);
  for(input=1;input<node->InputCount && node->Inputs[input];input++)
  {
    tileb.Init(data,tile);
    DoTileHandler<Format>(node->Inputs[input],&tileb);

  // "add|sub|mul|diff|make alpha"

    switch(node->Para[0])
    {
    case 0: // add;
      for(i=0;i<BMTILE_COUNT;i++)
        tile->Data[i].Add(tile->Data[i],tileb.Data[i]);
      break;

    case 1: // sub;
      for(i=0;i<BMTILE_COUNT;i++)
        tile->Data[i].Sub(tile->Data[i],tileb.Data[i]);
      break;

    case 2: // mul;
      for(i=0;i<BMTILE_COUNT;i++)
        tile->Data[i].Mul(tile->Data[i],tileb.Data[i]);
      break;

    case 3: // diff;
      for(i=0;i<BMTILE_COUNT;i++)
        tile->Data[i].One();
      break;

    case 4: // makealpha;
      for(i=0;i<BMTILE_COUNT;i++)
        tile->Data[i].Alpha(tileb.Data[i]);
      break;
    }
  }
}

TILE_MAKE(Tile_Add)

/****************************************************************************/

template <class Format> struct Tile_GlowRect_TempData
{
  sInt cx;
  sInt cy;
  sInt rx;
  sInt ry;
  sInt sx;
  sInt sy;
  Format color;
  sInt alpha;
  sInt power;
};

template <class Format> void Tile_GlowRect(struct GenNode *node,Tile<Format> *tile)
{
  const sInt ONESHIFT = 4;
  const sInt ONE = 16;
  const sInt OOONE = 0x8000/ONE;
  const sInt OOONESHIFT = 15-4;
  Format *dest;
  Tile_GlowRect_TempData<Format> *td;

  if(node->UserData)
  {
    td = (Tile_GlowRect_TempData<Format> *) node->UserData;
  }
  else
  {
    node->UserData = td = AllocNodeMem<Tile_GlowRect_TempData<Format> >();

    td->cx = node->Para[0]>>-(tile->ShiftX-OOONESHIFT);
    td->cy = node->Para[1]>>-(tile->ShiftY-OOONESHIFT);
    td->rx = node->Para[2]>>-(tile->ShiftX-OOONESHIFT);
    td->ry = node->Para[3]>>-(tile->ShiftY-OOONESHIFT);
    td->sx = node->Para[4]>>-(tile->ShiftX-OOONESHIFT);
    td->sy = node->Para[5]>>-(tile->ShiftY-OOONESHIFT);
    td->color.Set32(node->Para[6]);
    td->alpha = node->Para[7];
    td->power = node->Para[8];

    td->rx=td->rx*td->rx; if(td->rx<=0x100) td->rx=0x100; 
    td->ry=td->ry*td->ry; if(td->ry<=0x100) td->ry=0x100; 

    // rx/ry are typically in the range 1/256..1, so we don't significantly
    // lose precision by multiplying with the reciprocal
    td->rx = (1<<30) / td->rx;
    td->ry = (1<<30) / td->ry;
  }

  sInt cx = td->cx-(tile->OffsetX<<ONESHIFT);
  sInt cy = td->cy-(tile->OffsetY<<ONESHIFT);

  if(node->Inputs[0])
    DoTileHandler<Format>(node->Inputs[0],tile);
  else
    sSetMem(tile->Data,0,Format::ElementSize*Format::ElementCount*BMTILE_COUNT);

  PA(L"Tile_GlowRect");

  dest = tile->Data;
  for(sInt y=0;y<BMTILE_SIZE;y++)
  {
    sInt fy = sAbs(y*ONE-cy)-td->sy;
    if(fy<0) fy = 0;
    fy = Mul15(fy*fy,td->ry);

    for(sInt x=0;x<BMTILE_SIZE;x++)
    {
      sInt fx = sAbs(x*ONE-cx)-td->sx;
      if(fx<0) fx = 0;
      
      sInt a = Mul15(fx*fx,td->rx)+fy;
      if(a<0x8000)
      {
        a = Sqrt15D(a<<15);//Sqrt15Fast(a);
        a = (Pow15(0x8000 - a,td->power) * td->alpha) >> 15;
        //a = ((0x8000-a)*td->alpha)>>15;
        dest->Fade(*dest,td->color,a);
      }
      dest++;
    }
  }
}

TILE_MAKE(Tile_GlowRect)

/****************************************************************************/

struct Tile_Rotate_TempData
{
  sInt m00;
  sInt m01;
  sInt m10;
  sInt m11;
  sInt m20;
  sInt m21;
};


template <class Format> void Tile_Rotate(struct GenNode *node,Tile<Format> *tile)
{
  Bitmap<Format> *in;
  Format *d;
  Tile_Rotate_TempData *td;
  
  if(node->UserData)
  {
    td = (Tile_Rotate_TempData *) node->UserData;
  }
  else
  {
	  sInt xs,ys;
    sInt txs,tys;
    sInt sx,sy;
    sInt angles,anglec;
    sInt tx,ty;

    node->UserData = td = AllocNodeMem<Tile_Rotate_TempData>();

    xs = txs = tile->SizeX;
	  ys = tys = tile->SizeY;
    SinCos15(node->Para[0],angles,anglec);
    sx = node->Para[1]; if(sx<=xs && sx>=-xs) sx=xs;
    sy = node->Para[2]; if(sy<=ys && sy>=-ys) sy=ys;
    tx = node->Para[3]*xs;
    ty = node->Para[4]*ys;

    td->m00 = Div15( anglec*xs,sx*txs);
    td->m01 = Div15( angles*ys,sx*txs);
    td->m10 = Div15(-angles*xs,sy*tys);
    td->m11 = Div15( anglec*ys,sy*tys);
    td->m20 = tx - ((txs)*td->m00+(tys)*td->m10)/2;
    td->m21 = ty - ((txs)*td->m01+(tys)*td->m11)/2;
  }

  in = BeginFullHandler<Format>(node->Inputs[0],tile);

  PA(L"Tile_Rotate");

  d = tile->Data;
  
  for(sInt y=0;y<BMTILE_SIZE;y++)
  {
    sInt u = (y+tile->OffsetY)*td->m10 + td->m20 + tile->OffsetX*td->m00;
    sInt v = (y+tile->OffsetY)*td->m11 + td->m21 + tile->OffsetX*td->m01;
 
    for(sInt x=0;x<BMTILE_SIZE;x++)
    {
      d->Filter(in,u,v);
      d++;
      u += td->m00;
      v += td->m01;
    }
  }

  EndFullHandler(node->Inputs[0],tile);
}

TILE_MAKE(Tile_Rotate)

/****************************************************************************/

#define BLURSUB 256
#define BLURSUBMASK (BLURSUB-1)

template <class Format> void Tile_BlurY(
  sInt flags,                     // unused yet
  sInt width,                     // width of blur in pixels
  sInt sub,                       // subpixel blur, 4 bit
  sInt amp,                       // amplification, 0x8000 = none
  Format *s,                      // source pointer
  Format *d,                      // destination pointer
  sInt pixels,                    // number of pixels per akku
  sInt akkus)                     // number of akkus (lines)
{
  Format::PixelAccu accu[BMTILE_SIZE];
  sInt s0,s1,s2,s3,dd;
  sInt max;
  sInt sub0,sub1;
  sInt x,y;
  
  // clear

  max = pixels*akkus;

  s0 = (pixels-width)*akkus;
  s1 = s0+akkus;
  if(s0>=max) s0-=max;
  if(s1>=max) s1-=max;
  s2 = s0;
  s3 = s1;
  dd = 0;
  sub0 = sub;
  sub1 = BLURSUB-sub;

  // pre

  for(x=0;x<akkus*Format::ElementCount;x++)
  {
//    accu[x].Clear();
//    s[s2+x].AddAccu(accu[x],sub0-sub1);
    accu[0].p[x] = s[s2].p[x]*(sub0-sub1);
  }

  for(y=0;y<width*2;y++)
  {
    for(x=0;x<akkus*Format::ElementCount;x++)
    {
      accu[0].p[x] += s[s0].p[x]*sub1 + s[s1].p[x]*sub0;
    }
    s0+=akkus;
    s1+=akkus;
    if(s0==max) s0=0;
    if(s1==max) s1=0;
  }

  // main

  sInt v;
  for(y=0;y<pixels;y++)
  {
    for(x=0;x<akkus*Format::ElementCount;x++)
    {
      v = accu[0].p[x];
      d[dd].p[x] = sMin<sInt>(Mul24(v,amp),Format::ElementMax);
      v += (s[s1].p[x]-s[s2].p[x])*sub0;
      v += (s[s0].p[x]-s[s3].p[x])*sub1;
      accu[0].p[x] = v;
    }
    dd+=akkus;
    s0+=akkus;
    s1+=akkus;
    s2+=akkus;
    s3+=akkus;
    if(s0==max) s0=0;
    if(s1==max) s1=0;
    if(s2==max) s2=0;
    if(s3==max) s3=0;
  }
}

template <class Format> void Tile_Blur(struct GenNode *node,Tile<Format> *tile)
{
  Bitmap<Format> *in;
  Format *d,*s;
	sInt xs,ys;
  sInt flags,order;
  sInt x,y,i;
  sInt amp;

  flags = node->Para[0]; 
  order = flags&7;
  amp = node->Para[3];
  xs = tile->SizeX;
	ys = tile->SizeY;

  if(order==0 || (node->Para[1]==0 && node->Para[2]==0))
  {
    sInt amp = node->Para[3];
    DoTileHandler<Format>(node->Inputs[0],tile);
    if(amp!=0x8000)
      for(i=0;i<BMTILE_COUNT;i++)
        tile->Data[i].Scale(tile->Data[i],amp);
  }
  else
  {
    in = BeginFullHandler<Format>(node->Inputs[0],tile);

    PA(L"Tile_Blur");

    // the blurring
   
    if(!(node->UserFlags & 1))
    {
      node->UserFlags |= 1;

      sInt sizex,sizey,subx,suby;
      sInt ampy,ampx;

      sizex = (node->Para[1]*xs*BLURSUB)>>15;
      sizey = (node->Para[2]*ys*BLURSUB)>>15;
      if(sizex>=xs*BLURSUB) sizex=xs*BLURSUB-1;
      if(sizey>=ys*BLURSUB) sizey=ys*BLURSUB-1;
      if(sizex<=2) sizex=2;
      if(sizey<=2) sizey=2;

      sizex &= ~1;
      ampx = 0x1000000/sizex;
      sizex = (sizex+BLURSUB)/2;
      subx = sizex&BLURSUBMASK; 
      sizex = sizex/BLURSUB;

      sizey &= ~1;
      ampy = 0x1000000/sizey;
      sizey = (sizey+BLURSUB)/2;
      suby = sizey&BLURSUBMASK; 
      sizey = sizey/BLURSUB;


      Format *b0,*b1,*bx;

      // y blur

      if(sizey>0)
      {
        b0 = new Format[BMTILE_SIZE*in->SizeY];
        b1 = new Format[BMTILE_SIZE*in->SizeY];

        sInt amp0 = (sizex==0)?Mul15(ampy,amp):ampy;

        for(x=0;x<xs;x+=BMTILE_SIZE)
        {
          bx = &in->Adr(x,0);
          if(order==1)
          {
            Tile_BlurY<Format>(flags,sizey,suby,amp0,bx,b0,ys,BMTILE_SIZE);
            sCopyMem(&in->Adr(x,0),b0,ys*BMTILE_SIZE*sizeof(Format));
          }
          else if(order>1)
          {
            Tile_BlurY<Format>(flags,sizey,suby,ampy,bx,b0,ys,BMTILE_SIZE);
            for(i=1;i<order;i++)
            {
              Tile_BlurY<Format>(flags,sizey,suby,(i==order-1)?amp0:ampy,b0,b1,ys,BMTILE_SIZE);
              sSwap(b0,b1);
            }
            Tile_BlurY<Format>(flags,sizey,suby,ampy,b0,bx,ys,BMTILE_SIZE);
          }
        }

        delete[] b0;
        delete[] b1;
      }

      // x blur

      if(sizex>0)
      {
      
        sInt lines= 8*1024 / (in->SizeX*sizeof(Format));   // fit into cache
        lines = sRange(sMakePower2(lines),BMTILE_SIZE,1);
        b0 = new Format[lines*in->SizeX];
        b1 = new Format[lines*in->SizeX];
        sInt amp0 = Mul15(ampx,amp);
        
        for(sInt l=0;l<ys;l+=lines)
        {
          bx = &in->Adr(0,l);

          for(y=0;y<lines;y++)
            for(sInt x0=0;x0<xs;x0+=BMTILE_SIZE)
              for(sInt x1=0;x1<BMTILE_SIZE;x1++)
                b0[y+(x0+x1)*lines] = bx[y*BMTILE_SIZE+x0*ys+x1];

          for(i=0;i<order;i++)
          {
            Tile_BlurY<Format>(flags,sizex,subx,(i==order-1)?amp0:ampx,b0,b1,xs,lines);
            sSwap(b0,b1);
          }

          for(y=0;y<lines;y++)
            for(sInt x0=0;x0<xs;x0+=BMTILE_SIZE)
              for(sInt x1=0;x1<BMTILE_SIZE;x1++)
                bx[y*BMTILE_SIZE+x0*ys+x1] = b0[y+(x0+x1)*lines];
        }
        
        delete[] b0;
        delete[] b1;
      }
    }

  // output to tiles

    d = tile->Data;
    s = &in->Adr(tile->OffsetX,tile->OffsetY);
    for(i=0;i<BMTILE_COUNT;i++)
      *d++ = *s++;
    
    EndFullHandler(node->Inputs[0],tile);
  }
}

TILE_MAKE(Tile_Blur)

/****************************************************************************/

template <class Format> void Tile_Normals(struct GenNode *node,Tile<Format> *tile)
{
  Bitmap<Pixel16I> *in;
  Format *d;
  sInt vx,vy,vz;
  sInt mode;
  
	sInt xs,ys;
  sInt txs,tys;
  sInt shiftx,shifty,dist;

  Tile<Pixel16I> tiles;
  Pixel16I data[BMTILE_COUNT];
  Pixel16I *s;
  tiles.Init(data,tile);

// prepare (xs=old, txs=new)

  xs = txs = tile->SizeX;
	ys = tys = tile->SizeY;
  dist = node->Para[0];
  shiftx = tile->ShiftX;
  shifty = tile->ShiftY;
  mode = node->Para[1];

  in = BeginFullHandler<Pixel16I>(node->Inputs[0],&tiles);

  PA(L"Tile_Normals");

// rotate

  d = tile->Data; 
  s = tiles.Data;
  
  for(sInt yy=0;yy<BMTILE_SIZE;yy++)
  {
    sInt y=yy+tile->OffsetY;
    for(sInt xx=0;xx<BMTILE_SIZE;xx++)
    {
      sInt x=xx+tile->OffsetX;
      vx = in->Adr(x-2,y+0).p[0]
         + in->Adr(x-1,y+0).p[0]*3
         - in->Adr(x+0,y+0).p[0]*3
         - in->Adr(x+1,y+0).p[0];
      vx = vx<<(15-Format::ElementShift);
      vx = sRange<sInt>(Mul15(-vx,dist),0x7fff,-0x7fff);

      vy = in->Adr(x+0,y-2).p[0]
         + in->Adr(x+0,y-1).p[0]*3
         - in->Adr(x+0,y+0).p[0]*3
         - in->Adr(x+0,y+1).p[0];
      vy = vy<<(15-Format::ElementShift);
      vy = sRange<sInt>(Mul15(-vy,dist),0x7fff,-0x7fff);

      sInt len = vx*vx+vy*vy;

      vz = (0x3fff*0x3fff)-len;
      if(vz>0)
      {
        vz = Sqrt15D(vz);
      }
      else
      {
        sInt e = ISqrt15D(len);
        vx = (vx*e)>>16;
        vy = (vy*e)>>16;
        vz = 0;
      }

      if(mode)
      {
        sSwap(vx,vy);
        vy=-vy;
      }

      d->Set(vx+0x4000,vy+0x4000,vz+0x4000,0x7fff);
      d++;
    }
  }

  EndFullHandler(node->Inputs[0],&tiles);
}

TILE_MAKE(Tile_Normals)

/****************************************************************************/

template <class Format> struct Tile_Bump_Tempdata
{
  sInt mode;
  sInt px;
  sInt py;
  sInt pz;
  sInt dx;
  sInt dy;
  sInt dz;
  Format diff;
  Format ambi;
  sInt outer;
  sInt falloff;
  sInt amp;
  Format spec;
  sInt spow;
  sInt samp;
};

template <class Format> void Tile_Bump(struct GenNode *node,Tile<Format> *tileback)
{
  Tile<Pixel16C> t__;
  Tile<Pixel16C> *tilebump=&t__;
  Pixel16C data[BMTILE_SIZE*BMTILE_SIZE];
  Tile_Bump_Tempdata<Format> *td;
  sInt fade;

  tilebump->Init(data,tileback);

  DoTileHandler<Format>(node->Inputs[0],tileback);
  DoTileHandler<Pixel16C>(node->Inputs[1],tilebump);

  PA(L"Tile_Bump");

	sInt xs = tileback->SizeX;
	sInt ys = tileback->SizeY;

  if(tileback->Flags & TF_FIRST)
  {
    td = AllocNodeMem<Tile_Bump_Tempdata<Format> >();
    node->UserData = (void *)td;

    td->mode = node->Para[0];
    td->px = (node->Para[1]*xs*2-(xs*0x4000))>>11;
    td->py = (node->Para[2]*ys*2-(ys*0x4000))>>11;
    td->pz = (node->Para[3]*xs)>>11;
    sInt da = node->Para[4];
    sInt db = node->Para[5];
    td->diff.Set32(node->Para[6]);
    td->ambi.Set32(node->Para[7]);
    td->outer = node->Para[8];
    td->falloff = node->Para[9];
    td->amp = node->Para[10];
    td->spec.Set32(node->Para[11]);
    td->spow = node->Para[12];
    td->samp = node->Para[13];

    td->dz = Sin15(db); db = Cos15(db);
    td->dx = Mul15(db,Sin15(da));
    td->dy = Mul15(db,Cos15(da));

    if(td->mode==0)
    {
      td->px = td->px-sMulDiv(td->dx,td->pz,td->dz);
      td->py = td->py-sMulDiv(td->dy,td->pz,td->dz);
    }
  }
  else
  {
    td = (Tile_Bump_Tempdata<Format> *)node->UserData;
  }

  sInt lf = 0x8000;
  sInt sf = 0x0000;
  sInt df = 0x8000;
  sInt nx = 0;
  sInt ny = 0;
  sInt nz = 1;
  sInt lx = td->dx;
  sInt ly = td->dy;
  sInt lz = td->dz;

  Pixel16C *b = tilebump->Data;
  if(tilebump->Data->ElementCount<3)
    b = 0;
  Format *d = tileback->Data;

  for(sInt yy=0;yy<BMTILE_SIZE;yy++)
  {
    sInt y=(yy+tileback->OffsetY)<<4;
    for(sInt xx=0;xx<BMTILE_SIZE;xx++)
    {
      sInt x=(xx+tileback->OffsetX)<<4;

      if(td->mode!=2)
      {
        lx = (x-td->px);
        ly = (y-td->py);
        lz = (  td->pz);
        sInt e = ISqrt15D(lx*lx + ly*ly + lz*lz);
        lx = (lx*e)>>15;
        ly = (ly*e)>>15;
        lz = (lz*e)>>15;
      }

      if(b)
      {
        nx = (b->p[0]<<(15-b->ElementShift))-0x4000;
        ny = (b->p[1]<<(15-b->ElementShift))-0x4000;
        nz = (b->p[2]<<(15-b->ElementShift))-0x4000;
        b++;
      }

      lf = ((lx*nx)>>14) + ((ly*ny)>>14) + ((lz*nz)>>14);

      if(td->samp)
      {
        sInt hx = lx/2;
        sInt hy = ly/2;
        sInt hz = lz/2+0x4000;
        sInt e = ISqrt15D(hx*hx + hy*hy + hz*hz);
        hx = (hx*e)>>15;
        hy = (hy*e)>>15;
        hz = (hz*e)>>15;
        sf = ((hx*nx + hy*ny + hz*nz)>>14);
        if(sf<0) sf=0;
        sf = IntPower(sf,td->spow);
      }

      if(td->mode==0)
      {
        df = ((lx*td->dx)>>15) + ((ly*td->dy)>>15) + ((lz*td->dz)>>15);
        if(df<td->outer)
        {
          df = 0;
        }
        else
        {
          df = sDivShift(df-td->outer,0x8000-td->outer)>>1;
          df = IntPower(df,td->falloff);
        }
      }
      sf = (((sf*df)>>15)*td->samp)>>15;

      fade = (df*lf)>>15;
      fade = Mul15(fade,td->amp);

      for(sInt i=0;i<Format::ElementCount;i++)
      {
        sInt j = ((d->p[i]*(td->ambi.p[i]+Mul15(td->diff.p[i],fade)))>>Format::ElementShift);
        sInt v = sRange<sInt>(j,Format::ElementMax,0)+((td->spec.p[i]*sf)>>15);
          //((d->p[i]*col.p[i])>>Format::ElementShift) + ((td->spec.p[i]*sf)>>15);
        d->p[i]=sMin<sInt>(v,Format::ElementMax); 
      }

      d++;
    }
  }

}

TILE_MAKE(Tile_Bump)

/****************************************************************************/

template <class Format> void Tile_Resize(struct GenNode *node,Tile<Format> *tile)
{
  Bitmap<Format> *in;
  Format *d;
  Tile<Format> tilein;
  
	sInt xs,ys;
  sInt txs,tys;
	sInt m00,m01,m10,m11,m20,m21;

// prepare 

  xs  = tile->SizeX;
	ys  = tile->SizeY;
  txs = 16<<node->Para[0]; 
  tys = 16<<node->Para[1]; 

  tilein.Init(tile->Data,txs,tys,tile->Flags);
  in = BeginFullHandler<Format>(node->Inputs[0],&tilein);

  PA(L"Tile_Resize");

// rotate

  m00 = (0x8000*txs)>>tile->ShiftX;
  m01 = 0;
  m10 = 0;
  m11 = (0x8000*tys)>>tile->ShiftY;
  m20 = ((0x4000*txs)>>tile->ShiftX)-0x4000;
  m21 = ((0x4000*tys)>>tile->ShiftY)-0x4000;

  d = tile->Data;
  
  for(sInt y=0;y<BMTILE_SIZE;y++)
  {
    sInt u = (y+tile->OffsetY)*m10+m20+tile->OffsetX*m00;
    sInt v = (y+tile->OffsetY)*m11+m21+tile->OffsetX*m01;

    for(sInt x=0;x<BMTILE_SIZE;x++)
    {
      d->Filter(in,u,v);
      d++;
      u += m00;
      v += m01;
    }
  }

  EndFullHandler(node->Inputs[0],&tilein);
}

TILE_MAKE(Tile_Resize)

/****************************************************************************/

template <class dest,class source> void Tile_Depth_Convert(Tile<dest> *tiled,GenNode *input)
{
  sInt v;
  Tile<source> tiles;
  source data[BMTILE_COUNT];
  tiles.Init(data,tiled);

  DoTileHandler<source>(input,&tiles);

  PA(L"Tile_DepthConvert");

  source *s = tiles.Data;
  dest *d = tiled->Data;
  for(sInt i=0;i<BMTILE_COUNT;i++)
  {
    for(sInt j=0;j<dest::ElementCount;j++)
    {
      v = s[i].p[j&(source::ElementCount-1)];
      
      if(dest::ElementShift<source::ElementShift)
        v = v>>(source::ElementShift-dest::ElementShift);
      if(dest::ElementShift>source::ElementShift)
        v = v<<(dest::ElementShift-source::ElementShift);
        
      d[i].p[j] = v;
    }
  }
}

template <class Format> void Tile_Depth(struct GenNode *node,Tile<Format> *tile)
{
  switch(node->Para[0])
  {
    case 0: Tile_Depth_Convert<Format,Pixel16C>(tile,node->Inputs[0]); break;
    case 1: Tile_Depth_Convert<Format,Pixel16I>(tile,node->Inputs[0]); break;
    case 2: Tile_Depth_Convert<Format,Pixel8C >(tile,node->Inputs[0]); break;
    case 3: Tile_Depth_Convert<Format,Pixel8I >(tile,node->Inputs[0]); break;
  }
}

TILE_MAKE(Tile_Depth)

/****************************************************************************/

template <class Format> void Tile_Perlin(struct GenNode *node,Tile<Format> *tile)
{
  sInt x,y,i;
  sInt px,py;
  Format *d;
  sInt nn,n,s;
  sInt bias;

  PA(L"Tile_Perlin");

  sInt freq = node->Para[0];
  sInt oct = node->Para[1];
  sInt fade = node->Para[2];
  sInt seed = node->Para[3];
  sInt mode = node->Para[4];
  sInt amp = node->Para[5];
  Format c0; c0.Set32(node->Para[6]);
  Format c1; c1.Set32(node->Para[7]);
  sInt shiftx = tile->ShiftX;
  sInt shifty = tile->ShiftY;
  if(mode&1)
  {
    bias = 0;
  }
  else
  {
    bias = 0x4000;
    amp = amp/2;
  }

  d = tile->Data;
  for(y=0;y<BMTILE_SIZE;y++)
  {
    for(x=0;x<BMTILE_SIZE;x++)
    {
      n = 0;
      s = 0x8000;
      for(i=freq;i<freq+oct;i++)
      {
        px = ((x+tile->OffsetX)<<(15-shiftx))<<i;
        py = ((y+tile->OffsetY)<<(15-shifty))<<i;
        nn = Perlin2(px,py,((1<<i)-1),seed);
        if(mode&1)
          nn = sAbs(nn);
        if(mode&2)
          nn = Sin15(nn)/2;
        n += Mul15(nn,s);
        s = Mul15(s,fade);
      }
      d->Fade(c0,c1,sRange(Mul15(n,amp)+bias,0x8000,0));
      d++;
    }
  }
}

TILE_MAKE(Tile_Perlin)

/****************************************************************************/

template <class Format> struct Tile_Gradient_TempData
{
  Format c0;
  Format c1;
  sInt cdy;
  sInt cdx;
  sInt cstart;
};

template <class Format> void Tile_Gradient(struct GenNode *node,Tile<Format> *tile)
{
  sInt mode;
  sInt c;
  Tile_Gradient_TempData<Format> *td;

  mode = node->Para[5];

  PA(L"Tile_Gradient");

  if(node->UserData)
  {
    td = (Tile_Gradient_TempData<Format> *) node->UserData;
  }
  else
  {
    node->UserData = td = AllocNodeMem<Tile_Gradient_TempData<Format> >();

    sInt len;
    sInt pos;
    sInt dx,dy;

    td->c0.Set32(node->Para[0]);
    td->c1.Set32(node->Para[1]);
    pos = node->Para[2]/2;
    SinCos15(node->Para[3],dy,dx);
    len = node->Para[4]; if(len==0) len=1;
    dx = Div15(dx,len);
    dy = Div15(dy,len);
    td->cstart = 0x4000-Mul15((dx/2+dy/2),(pos+0x8000));

    td->cdx = Mul15(dx,0x8000>>tile->ShiftX);
    td->cdy = Mul15(dy,0x8000>>tile->ShiftY) - BMTILE_SIZE*td->cdx;
  }

  sInt x,y;
  Format *d;
  sInt val;

  c = td->cstart;
  c += td->cdx*tile->OffsetX;
  c += (td->cdy+td->cdx*BMTILE_SIZE)*tile->OffsetY;

  d = tile->Data;
  for(y=0;y<BMTILE_SIZE;y++)
  {
    for(x=0;x<BMTILE_SIZE;x++)
    {
      switch(mode)
      {
      case 0:
        val = sRange(c,0x7fff,0); 
        break;
      case 1:
        val = 0x4000-Sin15(sRange(c,0x7fff,0)+0x2000)/2;
        break;
      case 2:
        val = Sin15(sRange(c,0x7fff,0)/2);
        break;
      }
      d->Fade(td->c0,td->c1,val);
      d++;

      c+=td->cdx;
    }
    c+=td->cdy;
  }
}

TILE_MAKE(Tile_Gradient)

/****************************************************************************/

struct Tile_Cell_TempData
{
  sInt cells[256][3];
  sInt cellt[256];
  sInt aspl,aspr;
  sInt max;
};

template <class Format> void Tile_Cell(struct GenNode *node,Tile<Format> *tile)
{
  Format c0,c1,cb;
  sInt max0;
  sInt mdist;
  sInt seed;
  sInt aspect;
  sInt mode;
  sInt percent;
  sInt amp;
  sInt gamma;
  Tile_Cell_TempData *td;

  PA(L"Tile_Cell");

  sInt x,y,i,j,dist,best,best2,besti;
  sInt dx,dy,px,py;
  sInt shiftx,shifty;
  sInt v0,v1;
  sBool cut;
  sInt val;
  Format *d;
  Format cc;

  c0.Set32(node->Para[0]);
  c1.Set32(node->Para[1]);
  max0 = node->Para[2];
  mdist = node->Para[3];
  seed = node->Para[4];
  aspect = node->Para[5];
  mode = node->Para[6];
  cb.Set32(node->Para[7]);
  percent = node->Para[8];
  amp = node->Para[9];
  gamma = node->Para[10];

  // distribution

  if(node->UserData)
  {
    td = (Tile_Cell_TempData *) node->UserData;
  }
  else
  {
    node->UserData = td = AllocNodeMem<Tile_Cell_TempData>();

    sSetRndSeed(seed);
    for(i=0;i<max0*3;i++)
      td->cells[0][i] = sGetRnd(0x4000);

    mdist = (mdist*mdist)/4;
    for(i=1;i<max0;)
    {
      if((mode&2) && (sInt)sGetRnd(255)<percent)
        td->cells[i][2] = 0xffff;
      px = ((td->cells[i][0])&0x3fff)-0x2000;
      py = ((td->cells[i][1])&0x3fff)-0x2000; 
      cut = sFALSE;
      for(j=0;j<i && !cut;j++)
      {
        dx = ((td->cells[j][0]-px)&0x3fff)-0x2000;
        dy = ((td->cells[j][1]-py)&0x3fff)-0x2000; 
        dist = dx*dx+dy*dy;
        if(dist<mdist)
        {
          cut = sTRUE;
        }
      }
      if(cut)
      {
        max0--;
        td->cells[i][0] = td->cells[max0][0];
        td->cells[i][1] = td->cells[max0][1];
        td->cells[i][2] = td->cells[max0][2];
      }
      else
      {
        i++;
      }
    }
    td->max = max0;

    if(aspect>0)
    {
      td->aspl = 0;
      td->aspr = aspect;
    }
    else
    {
      td->aspl = -aspect;
      td->aspr = 0;
    }
  }

  // loop


  shiftx = 14-tile->ShiftX;
  shifty = 14-tile->ShiftY;
  d = tile->Data;
  for(y=0;y<BMTILE_SIZE;y++)
  {
    py = (y+tile->OffsetY) << shifty;
    for(i=0;i<td->max;i++)
    {
      dy = ((td->cells[i][1]-py)&0x3fff)-0x2000;      
//      td->cellt[i] = (((dy*dy)<<aspl)>>aspr);
      td->cellt[i] = (dy*dy);
    }

    for(x=0;x<BMTILE_SIZE;x++)
    {
      best = 0x8000*0x8000;
      best2 = 0x8000*0x8000;
      besti = -1;
      px = (x+tile->OffsetX)<<(shiftx);
      for(i=0;i<td->max;i++)
      {
        dx = ((td->cells[i][0]-px)&0x3fff)-0x2000;
        dist = (dx*dx) + td->cellt[i];
        if(dist<best)
        {
          best2 = best;
          best = dist;
          besti = i;
        }
        else if(dist<best2)
        {
          best2 = dist;
        }
      }

      v0 = Sqrt15D(best)*2;
      if(mode&1)
      {
        v1 = Sqrt15D(best2)*2;
        if(v0+v1>1)
          v0 = Div15(v1-v0,v1+v0);
        else
          v0 = 0;
      }
/*
      v0 = sFSqrt(best) * aspdiv;
      if(mode&1)
      {
        v1 = sFSqrt(best2) * aspdiv;
        if(v0+v1>0.00001f)
          v0 = (v1-v0)/(v1+v0);
        else
          v0 = 0;
      }
      val = sRange7fff(sFPow(v0*amp,gamma)*0x8000)*2;
      if(mode&4)
        val = 0x10000-val;
      */

      val = sRange(Pow15(Mul15(v0,amp),gamma),0x8000,0);
      if(mode&4)
        val = 0x8000-val;
      if(mode&2)
      {
        if(td->cells[besti][2]==0xffff)
          cc=cb;
        else
          cc.Fade(c0,c1,td->cells[besti][2]*2);
        d->Fade(cb,cc,val);
      }
      else
      {
        d->Fade(c1,c0,val);
      }
      d++;
    }
  }
}

TILE_MAKE(Tile_Cell)

/****************************************************************************/

struct Tile_RotateMul_TempData
{
  sInt m00;
  sInt m01;
  sInt m10;
  sInt m11;
  sInt m20;
  sInt m21;
};


template <class Format> void Tile_RotateMul(struct GenNode *node,Tile<Format> *tile)
{
  Bitmap<Format> *in;
  Format *d;
  Tile_RotateMul_TempData *td;
  
  sInt i,j,x,y;

  sInt count = sRange(node->Para[5],256,1);
  sInt amp = node->Para[11];

  PA(L"Tile_RotateMul");


  if(node->UserData)
  {
    td = (Tile_RotateMul_TempData *) node->UserData;
  }
  else
  {
	  sInt xs,ys;
    sInt txs,tys;
    sInt sx,sy;
    sInt angles,anglec;
    sInt tx,ty;

    node->UserData = td = AllocNodeMem<Tile_RotateMul_TempData>(count);

    sx = node->Para[1];
    sy = node->Para[2];
    for(i=0;i<count;i++)
    {
      xs = txs = tile->SizeX;
	    ys = tys = tile->SizeY;
      SinCos15(node->Para[0]+node->Para[6]*i,angles,anglec);
      if(sx<=xs && sx>=-xs) sx=xs;
      if(sy<=ys && sy>=-ys) sy=ys;
      tx = (node->Para[3]+node->Para[9]*i)*xs;
      ty = (node->Para[4]+node->Para[10]*i)*ys;

      td[i].m00 = Div15( anglec*xs,sx*txs);
      td[i].m01 = Div15( angles*ys,sx*txs);
      td[i].m10 = Div15(-angles*xs,sy*tys);
      td[i].m11 = Div15( anglec*ys,sy*tys);
      td[i].m20 = tx - ((txs)*td[i].m00+(tys)*td[i].m10)/2;
      td[i].m21 = ty - ((txs)*td[i].m01+(tys)*td[i].m11)/2;

      sx = Mul15(sx,node->Para[7]);
      sy = Mul15(sy,node->Para[8]);
    }
  }

  in = BeginFullHandler<Format>(node->Inputs[0],tile);

  
  Format col0;
  col0.Set32(0x00000000);
  for(i=0;i<BMTILE_COUNT;i++)
    tile->Data[i] = col0;

  for(i=0;i<count;i++)
  {
    d = tile->Data;
    for(y=0;y<BMTILE_SIZE;y++)
    {
      sInt u = (y+tile->OffsetY)*td[i].m10 + td[i].m20 + tile->OffsetX*td[i].m00;
      sInt v = (y+tile->OffsetY)*td[i].m11 + td[i].m21 + tile->OffsetX*td[i].m01;
   
      for(x=0;x<BMTILE_SIZE;x++)
      {
        col0.Filter(in,u,v);

        for(j=0;j<Format::ElementCount;j++)
          d->p[j] = sMin<sInt>(Format::ElementMax,d->p[j] + ((col0.p[j]*amp)>>15));
        d++;
        u += td[i].m00;
        v += td[i].m01;
      }
    }
  }

  EndFullHandler(node->Inputs[0],tile);
}

TILE_MAKE(Tile_RotateMul)

/****************************************************************************/

template <class Format> void Tile_Distort(struct GenNode *node,Tile<Format> *tile)
{
  Bitmap<Format> *in;
  Format *d;
  Pixel16C *s;
  Tile<Pixel16C> tileb;
  Pixel16C data[BMTILE_SIZE*BMTILE_SIZE];


  sInt u,v;
  sInt sx,sy;

  in = BeginFullHandler<Format>(node->Inputs[0],tile);

  tileb.Init(data,tile);
  DoTileHandler<Pixel16C>(node->Inputs[1],&tileb);

  PA(L"Tile_Distort");

  sx = node->Para[0]*tile->SizeX;
  sy = node->Para[0]*tile->SizeY;

  d = tile->Data;
  s = tileb.Data;
  
  for(sInt y=0;y<BMTILE_SIZE;y++)
  {
    for(sInt x=0;x<BMTILE_SIZE;x++)
    {
      u = ((x+tile->OffsetX)<<15) + Mul15(s->p[0&(s->ElementCount-1)]<<(15-s->ElementShift),sx);
      v = ((y+tile->OffsetY)<<15) + Mul15(s->p[1&(s->ElementCount-1)]<<(15-s->ElementShift),sy);
      d->Filter(in,u,v);
      d++;
      s++;
    }
  }

  EndFullHandler(node->Inputs[0],tile);
}

TILE_MAKE(Tile_Distort)

/****************************************************************************/

template <class Format> void Tile_Mask(struct GenNode *node,Tile<Format> *tile)
{
  Tile<Format> tileb;
  Tile<Format> tilec;
  Format db[BMTILE_SIZE*BMTILE_SIZE];
  Format dc[BMTILE_SIZE*BMTILE_SIZE];
  sInt i;

  Format *x,*b,*c;

  DoTileHandler<Format>(node->Inputs[0],tile);

  tileb.Init(db,tile);
  DoTileHandler<Format>(node->Inputs[1],&tileb);
  tilec.Init(dc,tile);
  DoTileHandler<Format>(node->Inputs[2],&tilec);

  PA(L"Tile_Mask");

  x = tile->Data;
  b = db;
  c = dc;

  switch(node->Para[0])
  {
  case 0:
    for(i=0;i<BMTILE_COUNT;i++)
    {
      sInt f0 = x[i].p[0];
      sInt f1 = Format::ElementMax-f0;
      for(sInt j=0;j<Format::ElementCount;j++)
        x[i].p[j] = (f0*b[i].p[j] + f1*c[i].p[j])>>Format::ElementShift;
    }
    break;

  case 1:
    for(i=0;i<BMTILE_COUNT;i++)
    {
      sInt f0 = x[i].p[0];
      for(sInt j=0;j<Format::ElementCount;j++)
        x[i].p[j] = sMin<sInt>(Format::ElementMax,((f0*b[i].p[j])>>Format::ElementShift) + c[i].p[j]);
    }
    break;

  case 2:
    for(i=0;i<BMTILE_COUNT;i++)
    {
      sInt f0 = x[i].p[0];
      for(sInt j=0;j<Format::ElementCount;j++)
        x[i].p[j] = sMax<sInt>(0,c[i].p[j]-((f0*b[i].p[j])>>Format::ElementShift));
    }
    break;

  case 3:
    for(i=0;i<BMTILE_COUNT;i++)
    {
      sInt f0 = x[i].p[0];
      sInt f1 = Format::ElementMax-f0;
      for(sInt j=0;j<Format::ElementCount;j++)
        x[i].p[j] = (f0*((b[i].p[j]*c[i].p[j])>>Format::ElementShift) + f1*c[i].p[j])>>Format::ElementShift;
    }
    break;
  }
}

TILE_MAKE(Tile_Mask)

/****************************************************************************/

template <class Format> void Tile_ColorRange(struct GenNode *node,Tile<Format> *tile)
{
  Format c0,c1;
  sInt mode;
  sInt val;

  mode = node->Para[0];
  c0.Set32(node->Para[1]);
  c1.Set32(node->Para[2]);
  if(mode&2)
    sSwap(c0,c1);

  DoTileHandler<Format>(node->Inputs[0],tile);

  PA(L"Tile_ColorRange");

  if(mode&1)
  {
    for(sInt i=0;i<BMTILE_COUNT;i++)
    {
      val = tile->Data[i].Intensity();
      for(sInt j=0;j<Format::ElementCount;j++)
        tile->Data[i].p[j] = c0.p[j]+(((c1.p[j]-c0.p[j])*val)>>15);
    }
  }
  else
  {
    for(sInt i=0;i<BMTILE_COUNT;i++)
    {
      for(sInt j=0;j<Format::ElementCount;j++)
      {
        val = tile->Data[i].p[j];
        tile->Data[i].p[j] = c0.p[j]+(((c1.p[j]-c0.p[j])*val)>>Format::ElementShift);
      }
    }
  }
}

TILE_MAKE(Tile_ColorRange)

/****************************************************************************/

template <class Format> void Tile_ColorBalance(struct GenNode *node,Tile<Format> *tile)
{
  Format col;

  DoTileHandler<Format>(node->Inputs[0],tile);
  col.One();

  for(sInt i=0;i<BMTILE_COUNT;i++)
    tile->Data[i].XOr(tile->Data[i],col);
}

TILE_MAKE(Tile_ColorBalance)

/****************************************************************************/

template <class Format> void Tile_CSBOld(struct GenNode *node,Tile<Format> *tile)
{
  Format::ElementType *map;
  sInt i,j;

  DoTileHandler<Format>(node->Inputs[0],tile);

  PA(L"Tile_CSBOld");

  if(node->UserData)
  {
    map = (Format::ElementType *) node->UserData;
  }
  else
  {
    sInt p,dp,dq,q;
    sInt b,c,d;
    sInt x1,x2,x3,y;

    p  = node->Para[0];
    dp = node->Para[1];
    dq = node->Para[2];
    q  = node->Para[3];

    b = dp;
    c = 3*0x8000-2*dp-dq;
    d = -2*0x8000+dp+dq;

    node->UserData = map = AllocNodeMem<Format::ElementType>(257);

    for(i=0;i<256;i++)
    {
      x1 = i<<(15-8);
      x1 = ((x1*(0x8000-p))>>15)+p;
      if(x1<0) x1=0;
      x2 = (x1*x1)>>15;
      x3 = (x2*x1)>>15;

      y = Mul15(b,x1)+Mul15(c,x2)+Mul15(d,x3);
      y = sRange(Mul15(y,q),0x7fff,0);

      map[i] = y>>(15-Format::ElementShift);
    }
    map[256]=map[255];
  }
/*
  for(i=0;i<BMTILE_COUNT;i++)
  {
    for(j=0;j<Format::ElementCount;j++)
    {
      x1 = tile->Data[i].p[j]<<(15-Format::ElementShift);
      x1 = ((x1*(0x8000-p))>>15)+p;
      if(x1<0) x1=0;
      x2 = (x1*x1)>>15;
      x3 = (x2*x1)>>15;


      y = Mul15(b,x1)+Mul15(c,x2)+Mul15(d,x3);
      y = sRange(Mul15(y,q),0x7fff,0);

      tile->Data[i].p[j] = y>>(15-Format::ElementShift);
    }
  }
*/

  if(Format::ElementShift==8)
  {
    for(i=0;i<BMTILE_COUNT;i++)
    {
      for(j=0;j<Format::ElementCount;j++)
      {
        tile->Data[i].p[j] = map[tile->Data[i].p[j]];
      }
    }
  }
  else
  {
    for(i=0;i<BMTILE_COUNT;i++)
    {
      for(j=0;j<Format::ElementCount;j++)
      {
        sInt v = tile->Data[i].p[j]<<(15-Format::ElementShift);
        sInt f0 = v&0x7f;
        tile->Data[i].p[j] = ((map[v>>7]*(0x7f-f0))+(map[(v>>7)+1]*f0))>>7;
      }
    }
  }
}

TILE_MAKE(Tile_CSBOld)

/****************************************************************************/

template <class Format> void Tile_CSB(struct GenNode *node,Tile<Format> *tile)
{
  Format col;
  Format::ElementType *map;

  DoTileHandler<Format>(node->Inputs[0],tile);
  col.One();

  PA(L"Tile_CSB");

  if(node->UserData)
  {
    map = (Format::ElementType *) node->UserData;
  }
  else
  {
    sInt i,v;
    sInt c = node->Para[0];
    sInt s = node->Para[1];
    sInt b = node->Para[2];

    sInt s0 = 0x8000-s;
    sInt b0 = b<0?-b:0;

    node->UserData = map = AllocNodeMem<Format::ElementType>(257);

    for(i=0;i<256;i++)
    {
      v = i<<(15-8);
      v = Pow15(v,c);
      v = ((v+s)*(s0))>>15;
      v = (Mul15(v,b)+b0);
      
      map[i] = sMin(v,0x7fff)>>(15-Format::ElementShift);
    }
    map[256]=map[255];
  }

  if(Format::ElementShift==8)
  {
    for(sInt i=0;i<BMTILE_COUNT;i++)
    {
      for(sInt j=0;j<Format::ElementCount;j++)
      {
        tile->Data[i].p[j] = map[tile->Data[i].p[j]];
      }
    }
  }
  else
  {
    for(sInt i=0;i<BMTILE_COUNT;i++)
    {
      for(sInt j=0;j<Format::ElementCount;j++)
      {
        sInt v = tile->Data[i].p[j]<<(15-Format::ElementShift);
        sInt f0 = v&0x7f;
        tile->Data[i].p[j] = ((map[v>>7]*(0x7f-f0))+(map[(v>>7)+1]*f0))>>7;
      }
    }
  }
}

TILE_MAKE(Tile_CSB)

/****************************************************************************/

template <class Format> void Tile_HSB(struct GenNode *node,Tile<Format> *tile)
{
  Format col;

  DoTileHandler<Format>(node->Inputs[0],tile);
  col.One();

  for(sInt i=0;i<BMTILE_COUNT;i++)
    tile->Data[i].XOr(tile->Data[i],col);
}

TILE_MAKE(Tile_HSB)

/****************************************************************************/

struct Tile_Twirl_TempData
{
  sInt m00;
  sInt m01;
  sInt m10;
  sInt m11;
  sInt m20;
  sInt m21;
};


template <class Format> void Tile_Twirl(struct GenNode *node,Tile<Format> *tile)
{
  Bitmap<Format> *in;
  Format *d;
  Tile_Twirl_TempData *td;
  
  if(node->UserData)
  {
    td = (Tile_Twirl_TempData *) node->UserData;
  }
  else
  {
	  sInt xs,ys;
    sInt txs,tys;
    sInt sx,sy;
    sInt angles,anglec;
    sInt tx,ty;

    node->UserData = td = AllocNodeMem<Tile_Twirl_TempData>();

    xs = txs = tile->SizeX;
	  ys = tys = tile->SizeY;
    SinCos15(node->Para[0],angles,anglec);
    sx = node->Para[1]; if(sx<=xs && sx>=-xs) sx=xs;
    sy = node->Para[2]; if(sy<=ys && sy>=-ys) sy=ys;
    tx = node->Para[3]*xs;
    ty = node->Para[4]*ys;

    td->m00 = Div15( anglec*xs,sx*txs);
    td->m01 = Div15( angles*ys,sx*txs);
    td->m10 = Div15(-angles*xs,sy*tys);
    td->m11 = Div15( anglec*ys,sy*tys);
    td->m20 = tx - ((txs)*td->m00+(tys)*td->m10)/2;
    td->m21 = ty - ((txs)*td->m01+(tys)*td->m11)/2;
  }

  in = BeginFullHandler<Format>(node->Inputs[0],tile);

  d = tile->Data;
  
  for(sInt y=0;y<BMTILE_SIZE;y++)
  {
    sInt u = (y+tile->OffsetY)*td->m10 + td->m20 + tile->OffsetX*td->m00;
    sInt v = (y+tile->OffsetY)*td->m11 + td->m21 + tile->OffsetX*td->m01;
 
    for(sInt x=0;x<BMTILE_SIZE;x++)
    {
      d->Filter(in,u,v);
      d++;
      u += td->m00;
      v += td->m01;
    }
  }

  EndFullHandler(node->Inputs[0],tile);
}

TILE_MAKE(Tile_Twirl)

/****************************************************************************/

template <class Format> void Tile_Text(struct GenNode *node,Tile<Format> *tile)
{
  Bitmap<Format> *in;

  sInt x = node->Para[0];
  sInt y = node->Para[1];
  sInt height = node->Para[2];
  sInt ls = node->Para[3];
  sInt width = node->Para[4];
  Format col; col.Set32(node->Para[5]);
  sInt flags = node->Para[6];

  sInt *data = node->Para+7;
  const sU16 *font = (const sU16 *)(sDInt)(data+1); data += data[0]+1;
  const sU16 *text = (const sU16 *)(sDInt)(data+1); data += data[0]+1;

  sInt alias = 1<<((flags>>2)&3);
  sInt xs = tile->SizeX * alias;
  sInt ys = tile->SizeY * alias;

  in = BeginFullHandler<Format>(node->Inputs[0],tile);

  if(!(node->UserFlags & 1))
  {
    node->UserFlags |= 1;

    sInt yf = sFontBegin(xs,ys,font,(tile->SizeX*width*alias)>>15,(tile->SizeY*height*alias)>>15,4);
    yf = (yf*ls*alias)>>15;

    sFontPrint((x*xs)>>15,(y*ys)>>15,text,-1);


    sInt fade;
    sU32 *bitmem = sFontBitmap();
    for(sInt yi=0;yi<in->SizeY;yi++)
    {
      sU32 *s = bitmem+yi*xs*alias;
      for(sInt xi=0;xi<in->SizeX;xi++)
      {
        if(alias==1)
          fade = s[0]&255;
        if(alias==2)
          fade = ((s[0]&255)+(s[1]&255)+(s[0+xs]&255)+(s[1+xs]&255))>>2; 
        if(alias==4)
          fade = ((s[0+0*xs]&255)+(s[1+0*xs]&255)+(s[2+0*xs]&255)+(s[3+0*xs]&255)
                  +(s[0+1*xs]&255)+(s[1+1*xs]&255)+(s[2+1*xs]&255)+(s[3+1*xs]&255)
                  +(s[0+2*xs]&255)+(s[1+2*xs]&255)+(s[2+2*xs]&255)+(s[3+2*xs]&255)
                  +(s[0+3*xs]&255)+(s[1+3*xs]&255)+(s[2+3*xs]&255)+(s[3+3*xs]&255))>>4;


        fade = (fade>>1)|(fade<<7);
        Format &pixel = in->Adr(xi,yi);
        pixel.Fade(pixel,col,fade);
        s+=alias;
      }
    }
    sFontEnd();
  }

  {
    Format *d = tile->Data;
    Format *s = &in->Adr(tile->OffsetX,tile->OffsetY);
    for(sInt i=0;i<BMTILE_COUNT;i++)
      *d++ = *s++;
  }
  EndFullHandler(node->Inputs[0],tile);
}

TILE_MAKE(Tile_Text)

#if !sPLAYER

sInt sFontBegin(sInt pagex,sInt pagey,const sU16 *name,sInt xs,sInt ys,sInt style)
{
  sChar buffer[256];

  for(sInt i=0;i<sCOUNTOF(buffer)-1 && *name;)
    buffer[i++] = *name++;
  buffer[i++] = 0;

  return sSystem->FontBegin(pagex,pagey,buffer,xs,ys,style);
}

void sFontPrint(sInt x,sInt y,const sU16 *string,sInt len)
{
  sSystem->FontPrint(x,y,string,len);
}

void sFontEnd()
{
  sSystem->FontEnd();
}

sU32 *sFontBitmap()
{
  return sSystem->FontBitmap();
}

#endif

/****************************************************************************/

#if !sPLAYER

template <class Format> void Tile_Export(struct GenNode *node,Tile<Format> *tile)
{
  DoTileHandler<Format>(node->Inputs[0],tile);

  PA(L"Tile_Export");
}

TILE_MAKE(Tile_Export)

#endif

/****************************************************************************/

#if !sPLAYER
template <class Format> void Tile_Import(struct GenNode *node,Tile<Format> *tile)
{
  Format color;

  sU32 *Image;

  PA(L"Tile_Import");

  sInt *data = node->Para+0;

  if(tile->Flags & TF_FIRST)
  {
    const sU16 *filename = (const sU16 *)(sDInt)(data+1); data += data[0]+1;

    sBitmap *bm;
    sChar buffer[256];
    for(sInt i=0;i<255 && filename[i];i++)
      buffer[i] = filename[i];
    buffer[i] = 0;
    bm = sLoadBitmap(buffer);
    if(bm)
    {
      node->UserData = Image = AllocNodeMem<sU32>(tile->SizeX*tile->SizeY);
      sInt i=0;
      for(sInt y=0;y<tile->SizeY;y++)
      {
        sInt yt = (y*bm->YSize)>>tile->ShiftY;
        for(sInt x=0;x<tile->SizeX;x++)
        {
          sInt xt = (x*bm->YSize)>>tile->ShiftX;
          Image[i++] = bm->Data[yt*bm->XSize+xt];
        }
      }      
    }
  }

  Image = (sU32 *) node->UserData;
  if(Image)
  {
    Image += tile->OffsetX + tile->OffsetY*tile->SizeX;
    Format *d = tile->Data;
    for(sInt y=0;y<BMTILE_SIZE;y++)
    {
      for(sInt x=0;x<BMTILE_SIZE;x++)
      {
        d->Set32(Image[x+y*tile->SizeX]);
        d++;
      }
    }
  }
  else
  {
    color.Set32(0xffff0000);

    for(sInt i=0;i<BMTILE_COUNT;i++)
      tile->Data[i] = color;
  }
}

TILE_MAKE(Tile_Import)
#endif

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   The operators themselves                                           ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

GenMobMesh * MobMesh_NodeLoad(GenNode *node);
GenMobMesh * MobMesh_NodeStore(GenNode *node);

#if !sPLAYER

class Class_NodeLoad : public GenClass
{
public:
  Class_NodeLoad()
  {
    Flags = GCF_LOAD;
    ClassId = GCI_NODELOAD;
    Column = GCC_INVISIBLE;
    TILE_ADD(Tile_NodeLoad);    
    Handlers.Set(GVI_MESH_MOBMESH,MobMesh_NodeLoad);
  }
};
class Class_NodeStore : public GenClass
{
public:
  Class_NodeStore()
  {
    Flags = GCF_LOAD;
    ClassId = GCI_NODESTORE;
    Column = GCC_INVISIBLE;
    TILE_ADD(Tile_NodeStore);    
    Handlers.Set(GVI_MESH_MOBMESH,MobMesh_NodeStore);
  }
};

class Class_Bitmap_Load : public GenClass
{
public:
  Class_Bitmap_Load()
  {
    Flags = GCF_LOAD;
    Name = "load";
    ClassId = GCI_BITMAP_LOAD;
    Shortcut = 'l';
    Column = GCC_LINK;
    Output = Doc->FindType(GTI_BITMAP);
    Para.Add(GenPara::MakeLink(1,"load",Output,GPF_DEFAULT|GPF_LINK_REQUIRED));
  }
};

class Class_Bitmap_Store : public GenClass
{
public:
  Class_Bitmap_Store()
  {
    Flags = GCF_NOP|GCF_STORE;
    Name = "store";
    ClassId = GCI_BITMAP_STORE;
    Column = GCC_LINK;
    Shortcut = 's';
    Output = Doc->FindType(GTI_BITMAP);
    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
  }
};

class Class_Bitmap_Nop : public GenClass
{
public:
  Class_Bitmap_Nop()
  {
    Flags = GCF_NOP;
    Name = "nop";
    ClassId = GCI_BITMAP_NOP;
    Column = GCC_LINK;
    Output = Doc->FindType(GTI_BITMAP);
    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    Para.Add(GenPara::MakeString(2,"bla",32,"dumdidum"));
  }
};

/****************************************************************************/


class Class_Bitmap_Flat : public GenClass
{
public:
  Class_Bitmap_Flat()
  {
    Name = "flat";
    ClassId = GCI_BITMAP_FLAT;
    Column = GCC_GENERATOR;
    Shortcut = 'f';
    Output = Doc->FindType(GTI_BITMAP);

    TILE_ADD(Tile_Flat);
    Para.Add(GenPara::MakeLabel("Bitmap Size"));
    Para.Add(GenPara::MakeIntV(1,"Size",2,4096,0,256,16,GPF_INVISIBLE));
    Para.Add(GenPara::MakeChoice(3,"Variant","Mono8|Color8|Mono16|Color16",0,GPF_INVISIBLE));
    Para.Add(GenPara::MakeColor(4,"ClearColor",0xff000000,GPF_LABEL));
  }
};

/****************************************************************************/

class Class_Bitmap_Color : public GenClass
{
public:
  Class_Bitmap_Color()
  {
    Name = "color";
    ClassId = GCI_BITMAP_COLOR;
    Column = GCC_FILTER;
    Shortcut = 'c';
    Output = Doc->FindType(GTI_BITMAP);

    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    TILE_ADD(Tile_Color);
    Para.Add(GenPara::MakeChoice(1,"Variant","mul|add|sub|gray|invert|scale",0,GPF_FULLWIDTH|GPF_LABEL));
    Para.Add(GenPara::MakeColor(2,"Color",0xffffffff,GPF_LABEL));
  }
};

/****************************************************************************/

class Class_Bitmap_Add : public GenClass
{
public:
  Class_Bitmap_Add()
  {
    Name = "add";
    ClassId = GCI_BITMAP_ADD;
    Column = GCC_ADD;
    Shortcut = 'a';
    Output = Doc->FindType(GTI_BITMAP);

    TILE_ADD(Tile_Add);
    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    Inputs.Add(GenInput::Make(2,Output,0,0));
    Inputs.Add(GenInput::Make(3,Output,0,0));
    Inputs.Add(GenInput::Make(4,Output,0,0));
    Inputs.Add(GenInput::Make(5,Output,0,0));
    Inputs.Add(GenInput::Make(6,Output,0,0));
    Inputs.Add(GenInput::Make(7,Output,0,0));
    Inputs.Add(GenInput::Make(8,Output,0,0));
    Para.Add(GenPara::MakeChoice(1,"Variant","add|sub|mul|diff|make alpha",0));
  }
};

/****************************************************************************/

class Class_Bitmap_GlowRect : public GenClass
{
public:
  Class_Bitmap_GlowRect()
  {
    Name = "glowrect";
    ClassId = GCI_BITMAP_GLOWRECT;
    Column = GCC_GENERATOR;
    Shortcut = 'g';
    Output = Doc->FindType(GTI_BITMAP);

    TILE_ADD(Tile_GlowRect);
    Inputs.Add(GenInput::Make(1,Output,0,0));
    Para.Add(GenPara::MakeFloatV(1,"Center"   ,2,16,-16,0.50f,0.001f));
    Para.Add(GenPara::MakeFloatV(2,"Radius"   ,2,16,  0,0.25f,0.001f));
    Para.Add(GenPara::MakeFloatV(3,"Size"     ,2,16,  0,0.00f,0.001f));
    Para.Add(GenPara::MakeColor (4,"Color"    ,0xffffffff,GPF_LABEL));
    Para.Add(GenPara::MakeFloatV(5,"Alpha"    ,1, 1,  0,1.00f,0.01f));
    Para.Add(GenPara::MakeFloatV(6,"Power"    ,1,16,  0,1.00f,0.01f));
    Para.Add(GenPara::MakeChoice(7,"Wrap"     ,"off|x|y|on",0));
    Para.Add(GenPara::MakeChoice(8,"Bug-Mode" ,"off|on",0));
  }
};

/****************************************************************************/

class Class_Bitmap_Rotate : public GenClass
{
public:
  Class_Bitmap_Rotate()
  {
    Name = "rotate";
    ClassId = GCI_BITMAP_ROTATE;
    Column = GCC_FILTER;
    Shortcut = 'r';
    Output = Doc->FindType(GTI_BITMAP);

    TILE_ADD(Tile_Rotate);
    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    Para.Add(GenPara::MakeFloatV(1,"Angle"    ,1,16,-16,0.00f,0.001f));
    Para.Add(GenPara::MakeFloatV(2,"Zoom"   ,2,256,-256,1.00f,0.001f));
    Para.Add(GenPara::MakeFloatV(3,"Center"   ,2,16,-16,0.50f,0.001f));
    Para.Add(GenPara::MakeChoice(4,"Border"   ,"off|x|y|on",0));
    Para.Add(GenPara::MakeIntV  (5,"New Bitmap Size",2,4096,0,0,16));
  }
};

/****************************************************************************/

class Class_Bitmap_Blur : public GenClass
{
public:
  Class_Bitmap_Blur()
  {
    Name = "blur";
    ClassId = GCI_BITMAP_BLUR;
    Column = GCC_FILTER;
    Shortcut = 'b';
    Output = Doc->FindType(GTI_BITMAP);

    TILE_ADD(Tile_Blur);
    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    Para.Add(GenPara::MakeChoice(1,"Order"    ,"off|square|triangle|3|4|5",2,GPF_FULLWIDTH|GPF_LABEL));
    Para.Add(GenPara::MakeFloatV(2,"Size"     ,2,1,  0,0.00f,0.001f,GPF_FULLWIDTH|GPF_LABEL));
    Para.Add(GenPara::MakeFloatV(3,"Amplify"  ,1,64,  0,0.50f,0.01f ,GPF_FULLWIDTH|GPF_LABEL));
  }
};

/****************************************************************************/

class Class_Bitmap_Normals : public GenClass
{
public:
  Class_Bitmap_Normals()
  {
    Name = "normals";
    ClassId = GCI_BITMAP_NORMALS;
    Column = GCC_FILTER;
    Output = Doc->FindType(GTI_BITMAP);

    TILE_ADD(Tile_Normals);
    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    Para.Add(GenPara::MakeFloatV(1,"Strength",1,16,0,1.00f,0.01f,GPF_DEFAULT));
    Para.Add(GenPara::MakeChoice(2,"Mode","normals|tangent",0,GPF_DEFAULT));
  }
};

/****************************************************************************/

class Class_Bitmap_Bump : public GenClass
{
public:
  Class_Bitmap_Bump()
  {
    Name = "bump";
    ClassId = GCI_BITMAP_BUMP;
    Column = GCC_SPECIAL;
    Shortcut = 'B'|sKEYQ_SHIFT;
    Output = Doc->FindType(GTI_BITMAP);

    TILE_ADD(Tile_Bump);
    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    Inputs.Add(GenInput::Make(2,Output,0,GIF_REQUIRED));

    Para.Add(GenPara::MakeChoice(1,"Mode","spot|point|directional",2));
    Para.Add(GenPara::MakeFloatV(2,"Pos",3,16,-16,0.5f,0.02f));
    Para.Add(GenPara::MakeFloatV(3,"Dir",2,4,-4,0.125f,0.001f));
    Para.Add(GenPara::MakeColor(4,"Diffuse",0xffffffff));
    Para.Add(GenPara::MakeColor(5,"Ambient",0xff000000));
    Para.Add(GenPara::MakeFloatV(6,"Outer",1,4,0,0.75f,0.001f));
    Para.Add(GenPara::MakeFloatV(7,"Falloff",1,4,0,1.75f,0.001f));
    Para.Add(GenPara::MakeFloatV(8,"Amplify",1,4,-4,0.50f,0.001f));
    Para.Add(GenPara::MakeColor(9,"Specular",0xffffffff));
    Para.Add(GenPara::MakeFloatV(10,"Power",1,16,0,4,0.02f));
    Para.Add(GenPara::MakeFloatV(11,"Amplify",1,4,0,1.00f,0.001f));
  }
};

/****************************************************************************/

class Class_Bitmap_Resize : public GenClass
{
public:
  Class_Bitmap_Resize()
  {
    Name = "resize";
    ClassId = GCI_BITMAP_RESIZE;
    Column = GCC_SPECIAL;
    Shortcut = 'R'|sKEYQ_SHIFT;
    Output = Doc->FindType(GTI_BITMAP);

    TILE_ADD(Tile_Resize);
    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    Para.Add(GenPara::MakeChoice(1,"X","16|32|64|128|256|512|1024|2048",4));
    Para.Add(GenPara::MakeChoice(2,"Y","16|32|64|128|256|512|1024|2048",4));
  }
};

/****************************************************************************/

class Class_Bitmap_Depth : public GenClass
{
public:
  Class_Bitmap_Depth()
  {
    Name = "depth";
    ClassId = GCI_BITMAP_DEPTH;
    Column = GCC_SPECIAL;
    Shortcut = 'D'|sKEYQ_SHIFT;
    Output = Doc->FindType(GTI_BITMAP);

    TILE_ADD(Tile_Depth);
    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    Para.Add(GenPara::MakeChoice(1,"X","16C|16I|8C|8I",2));
  }
};

/****************************************************************************/

class Class_Bitmap_Perlin : public GenClass
{
public:
  Class_Bitmap_Perlin()
  {
    Name = "perlin";
    ClassId = GCI_BITMAP_PERLIN;
    Column = GCC_GENERATOR;
    Shortcut = 'p';
    Output = Doc->FindType(GTI_BITMAP);

    TILE_ADD(Tile_Perlin);

    Para.Add(GenPara::MakeInt(1,"Frequency",16,0,1));
    Para.Add(GenPara::MakeInt(2,"Octaves",16,0,2));
    Para.Add(GenPara::MakeFloat(3,"Fadeoff",8,-8,1,0.01f));
    Para.Add(GenPara::MakeInt(4,"Seed",255,0,0));
    Para.Add(GenPara::MakeChoice(5,"Mode","norm|abs|sin|abs+sin"));
    Para.Add(GenPara::MakeFloat(6,"Amplify",16,0,1,0.01f));
    Para.Add(GenPara::MakeColor(7,"Color 0",0xff000000));
    Para.Add(GenPara::MakeColor(8,"Color 1",0xffffffff));    
  }
};

/****************************************************************************/

class Class_Bitmap_Gradient : public GenClass
{
public:
  Class_Bitmap_Gradient()
  {
    Name = "gradient";
    ClassId = GCI_BITMAP_GRADIENT;
    Column = GCC_GENERATOR;
    Shortcut = 'G'|sKEYQ_SHIFT;
    Output = Doc->FindType(GTI_BITMAP);

    TILE_ADD(Tile_Gradient);

    Para.Add(GenPara::MakeColor(1,"Color 0",0xff000000));
    Para.Add(GenPara::MakeColor(2,"Color 1",0xffffffff));    
    Para.Add(GenPara::MakeFloat(3,"Position",16,-16,0,0.02f));
    Para.Add(GenPara::MakeFloat(4,"Angle",4,-4,1,0.001f));
    Para.Add(GenPara::MakeFloat(5,"Length",16,-16,1,0.001f));
    Para.Add(GenPara::MakeChoice(6,"Mode","gradient|guass|sin"));
  }
};

/****************************************************************************/

class Class_Bitmap_Cell : public GenClass
{
public:
  Class_Bitmap_Cell()
  {
    Name = "cell";
    ClassId = GCI_BITMAP_CELL;
    Column = GCC_GENERATOR;
    Shortcut = 'C'|sKEYQ_SHIFT;
    Output = Doc->FindType(GTI_BITMAP);

    TILE_ADD(Tile_Cell);

    Para.Add(GenPara::MakeColor(1,"Color 0",0xffff0000));
    Para.Add(GenPara::MakeColor(2,"Color 1",0xffffff00));    
    Para.Add(GenPara::MakeInt(3,"Max Count",255,0,0));
    Para.Add(GenPara::MakeFloat(4,"Min Distance",1,0,0.125f,0.001f));
    Para.Add(GenPara::MakeInt(5,"Seed",255,0,0));
    Para.Add(GenPara::MakeInt(6,"Aspect",8,-8,0,0.01f));
    Para.Add(GenPara::MakeFlags(7,"Flags","inner|outer:*1|cell-color:*2|invert"));
    Para.Add(GenPara::MakeColor(8,"Color 2",0xff000000));    
    Para.Add(GenPara::MakeInt(9,"Percentage",255,0,0));
    Para.Add(GenPara::MakeFloat(10,"Amplify",16,0,1,0.001f));
    Para.Add(GenPara::MakeFloat(11,"Gamma",16,0,1,0.001f));
  }
};

/****************************************************************************/

class Class_Bitmap_RotateMul : public GenClass
{
public:
  Class_Bitmap_RotateMul()
  {
    Name = "rotatemul";
    ClassId = GCI_BITMAP_ROTATEMUL;
    Column = GCC_FILTER;
    Output = Doc->FindType(GTI_BITMAP);

    TILE_ADD(Tile_RotateMul);
    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    Para.Add(GenPara::MakeFloatV(1,"Angle"    ,1,16,-16,0.00f,0.001f));
    Para.Add(GenPara::MakeFloatV(2,"Zoom"   ,2,256,-256,1.00f,0.001f));
    Para.Add(GenPara::MakeFloatV(3,"Center"   ,2,16,-16,0.50f,0.001f));
    Para.Add(GenPara::MakeInt(4,"Count",255,1,1));
    Para.Add(GenPara::MakeFloatV(5,"*Angle"    ,1,16,-16,0.00f,0.001f));
    Para.Add(GenPara::MakeFloatV(6,"*Zoom"     ,2,256,-256,1.00f,0.001f));
    Para.Add(GenPara::MakeFloatV(7,"*Center"   ,2,16,-16,0.00f,0.001f));
    Para.Add(GenPara::MakeFloatV(8,"Amplify"   ,1,16,-16,1.00f,0.001f));
  }
};

/****************************************************************************/

class Class_Bitmap_Distort : public GenClass
{
public:
  Class_Bitmap_Distort()
  {
    Name = "distort";
    ClassId = GCI_BITMAP_DISTORT;
    Column = GCC_SPECIAL;
    Shortcut = 'd';
    Output = Doc->FindType(GTI_BITMAP);

    TILE_ADD(Tile_Distort);
    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    Inputs.Add(GenInput::Make(2,Output,0,GIF_REQUIRED));

    Para.Add(GenPara::MakeFloat(1,"Strength",4,-4,0.02f,0.001f));
  }
};

/****************************************************************************/

class Class_Bitmap_Mask : public GenClass
{
public:
  Class_Bitmap_Mask()
  {
    Name = "mask";
    ClassId = GCI_BITMAP_MASK;
    Shortcut = 'm';
    Output = Doc->FindType(GTI_BITMAP);

    TILE_ADD(Tile_Mask);
    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    Inputs.Add(GenInput::Make(2,Output,0,GIF_REQUIRED));
    Inputs.Add(GenInput::Make(3,Output,0,GIF_REQUIRED));

    Para.Add(GenPara::MakeChoice(1,"Mode","mix|add|sub|mul"));
  }
};

/****************************************************************************/

class Class_Bitmap_ColorRange : public GenClass
{
public:
  Class_Bitmap_ColorRange()
  {
    Name = "colorrange";
    ClassId = GCI_BITMAP_COLORRANGE;
    Column = GCC_FILTER;
    Output = Doc->FindType(GTI_BITMAP);

    TILE_ADD(Tile_ColorRange);
    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));

    Para.Add(GenPara::MakeFlags(1,"Flags","adjust|range:*1normal|invert"));
    Para.Add(GenPara::MakeColor(2,"Color 0",0xff000000));
    Para.Add(GenPara::MakeColor(3,"Color 1",0xffffffff));
  }
};

/****************************************************************************/

class Class_Bitmap_ColorBalance : public GenClass
{
public:
  Class_Bitmap_ColorBalance()
  {
    Name = "colorbalance";
    ClassId = GCI_BITMAP_COLORBALANCE;
    Column = GCC_FILTER;
    Output = Doc->FindType(GTI_BITMAP);

    TILE_ADD(Tile_ColorBalance);
    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
  }
};

/****************************************************************************/

class Class_Bitmap_CSBOld : public GenClass
{
public:
  Class_Bitmap_CSBOld()
  {
    Name = "(csb old)";
    ClassId = GCI_BITMAP_CSBOLD;
    Column = GCC_INVISIBLE;
    Output = Doc->FindType(GTI_BITMAP);

    TILE_ADD(Tile_CSBOld);
    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));

    Para.Add(GenPara::MakeContrast(1));
  }
};

/****************************************************************************/

class Class_Bitmap_CSB : public GenClass
{
public:
  Class_Bitmap_CSB()
  {
    Name = "csb";
    ClassId = GCI_BITMAP_CSB;
    Column = GCC_FILTER;
    Output = Doc->FindType(GTI_BITMAP);

    TILE_ADD(Tile_CSB);
    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));

    Para.Add(GenPara::MakeFloat(1,"Contrast"  ,64,0,1,0.001f));
    Para.Add(GenPara::MakeFloat(2,"Saturation",1,0,0,0.001f));
    Para.Add(GenPara::MakeFloat(3,"Brightness",64,-64,1,0.001f));
  }
};

/****************************************************************************/

class Class_Bitmap_HSB : public GenClass
{
public:
  Class_Bitmap_HSB()
  {
    Name = "hsb";
    ClassId = GCI_BITMAP_HSB;
    Column = GCC_FILTER;
    Shortcut = 'h';
    Output = Doc->FindType(GTI_BITMAP);

    TILE_ADD(Tile_HSB);
    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
  }
};

/****************************************************************************/

class Class_Bitmap_Twirl : public GenClass
{
public:
  Class_Bitmap_Twirl()
  {
    Name = "twirl";
    ClassId = GCI_BITMAP_TWIRL;
    Column = GCC_FILTER;
    Shortcut = 'T'|sKEYQ_SHIFT;
    Output = Doc->FindType(GTI_BITMAP);

    TILE_ADD(Tile_Twirl);
    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    Para.Add(GenPara::MakeFloatV(1,"Angle"    ,1,16,-16,0.00f,0.001f));
    Para.Add(GenPara::MakeFloatV(2,"Zoom"   ,2,256,-256,1.00f,0.001f));
    Para.Add(GenPara::MakeFloatV(3,"Center"   ,2,16,-16,0.50f,0.001f));
    Para.Add(GenPara::MakeChoice(4,"Border"   ,"off|x|y|on",0));
    Para.Add(GenPara::MakeIntV  (5,"New Bitmap Size",2,4096,0,0,16));
  }
};

/****************************************************************************/

class Class_Bitmap_Text : public GenClass
{
public:
  Class_Bitmap_Text()
  {
    Name = "text";
    ClassId = GCI_BITMAP_TEXT;
    Column = GCC_FILTER;
    Shortcut = 't';
    Output = Doc->FindType(GTI_BITMAP);

    TILE_ADD(Tile_Text);
    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    Para.Add(GenPara::MakeFloatV(1,"Position"    ,2,4,-4,0.000f,0.001f));
    Para.Add(GenPara::MakeFloatV(2,"Height"      ,1,4, 0,0.125f,0.001f));
    Para.Add(GenPara::MakeFloatV(3,"Line Feed"   ,1,16,-16,1.0f,0.01f));
    Para.Add(GenPara::MakeFloatV(4,"Width"       ,1,4, 0,0.000f,0.001f));
    Para.Add(GenPara::MakeColor (5,"Color",0xffffffff));
    Para.Add(GenPara::MakeFlags (6,"Flags","center x:*1center y:*2normal|2*alias|4*alias"));
    Para.Add(GenPara::MakeString(7,"Font",32,"arial"));
    Para.Add(GenPara::MakeString(8,"Text",1024,"hund."));
  }
};

/****************************************************************************/

class Class_Bitmap_Export : public GenClass
{
public:
  Class_Bitmap_Export()
  {
    Name = "export";
    ClassId = GCI_BITMAP_EXPORT;
    Column = GCC_SPECIAL;
    Output = Doc->FindType(GTI_BITMAP);

    TILE_ADD(Tile_Export);
    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    Para.Add(GenPara::MakeFilename(1,"Filename","c:\\temp\\dummy.bmp"));
    Para.Add(GenPara::MakeChoice(2,"Size","16|32|64|128|256|512|1024|2048",4,GPF_LABEL));
    Para.Add(GenPara::MakeChoice(3,0,"16|32|64|128|256|512|1024|2048",4,0));
    Para.Add(GenPara::MakeChoice(4,"Variant","Mono8|Color8|Mono16|Color16",3,GPF_LABEL));
  }
};

/****************************************************************************/

class Class_Bitmap_Import : public GenClass
{
public:
  Class_Bitmap_Import()
  {
    Name = "import";
    ClassId = GCI_BITMAP_IMPORT;
    Column = GCC_GENERATOR;
    Output = Doc->FindType(GTI_BITMAP);

    TILE_ADD(Tile_Import);
    Para.Add(GenPara::MakeFilename(1,"Filename","c:\\temp\\dummy.bmp"));
  }
};

/****************************************************************************/

#endif

/****************************************************************************/
/****************************************************************************/

#if !sPLAYER
void AddBitmapClasses(GenDoc *doc)
{
  doc->Classes.Add(new Class_NodeLoad);
  doc->Classes.Add(new Class_NodeStore);
  doc->Classes.Add(new Class_Bitmap_Load);
  doc->Classes.Add(new Class_Bitmap_Store);
  doc->Classes.Add(new Class_Bitmap_Nop);

  doc->Classes.Add(new Class_Bitmap_Flat);
  doc->Classes.Add(new Class_Bitmap_Color);
  doc->Classes.Add(new Class_Bitmap_Add);
  doc->Classes.Add(new Class_Bitmap_GlowRect);
  doc->Classes.Add(new Class_Bitmap_Rotate);
  doc->Classes.Add(new Class_Bitmap_Blur);
  doc->Classes.Add(new Class_Bitmap_Normals);
  doc->Classes.Add(new Class_Bitmap_Bump);

  doc->Classes.Add(new Class_Bitmap_Resize);
  doc->Classes.Add(new Class_Bitmap_Depth);
  doc->Classes.Add(new Class_Bitmap_Perlin);
  doc->Classes.Add(new Class_Bitmap_Gradient);
  doc->Classes.Add(new Class_Bitmap_Cell);
  doc->Classes.Add(new Class_Bitmap_RotateMul);
  doc->Classes.Add(new Class_Bitmap_Distort);
  doc->Classes.Add(new Class_Bitmap_Mask);
  doc->Classes.Add(new Class_Bitmap_ColorRange);
  doc->Classes.Add(new Class_Bitmap_ColorBalance);
  doc->Classes.Add(new Class_Bitmap_CSBOld);
  doc->Classes.Add(new Class_Bitmap_CSB);
  doc->Classes.Add(new Class_Bitmap_HSB);
//  doc->Classes.Add(new Class_Bitmap_Twirl);
  doc->Classes.Add(new Class_Bitmap_Text);
  doc->Classes.Add(new Class_Bitmap_Export);
  doc->Classes.Add(new Class_Bitmap_Import);
}
#endif

GenHandlerArray GenBitmapHandlers[] =
{
  TILE_ARRAY(0x0010,Tile_NodeLoad)
  TILE_ARRAY(0x0020,Tile_NodeStore)
  { MobMesh_NodeLoad      ,0x0018 },
  { MobMesh_NodeStore     ,0x0028 },

  TILE_ARRAY(0x1010,Tile_Flat)
  TILE_ARRAY(0x1030,Tile_Color)
  TILE_ARRAY(0x1040,Tile_Add)
  TILE_ARRAY(0x1050,Tile_GlowRect)
  TILE_ARRAY(0x1060,Tile_Rotate)
  TILE_ARRAY(0x1070,Tile_Blur)

  TILE_ARRAY(0x10c0,Tile_Normals)
  TILE_ARRAY(0x10d0,Tile_Bump)
  TILE_ARRAY(0x10f0,Tile_Resize)
  TILE_ARRAY(0x1100,Tile_Depth)
  TILE_ARRAY(0x1110,Tile_Perlin)
  TILE_ARRAY(0x1120,Tile_Gradient)
  TILE_ARRAY(0x1130,Tile_Cell)
  TILE_ARRAY(0x1150,Tile_RotateMul)
  TILE_ARRAY(0x1160,Tile_Distort)
  TILE_ARRAY(0x1170,Tile_Mask)
  TILE_ARRAY(0x1180,Tile_ColorRange)
  TILE_ARRAY(0x1190,Tile_ColorBalance)
  TILE_ARRAY(0x11a0,Tile_CSBOld)
  TILE_ARRAY(0x11b0,Tile_CSB)
  TILE_ARRAY(0x11c0,Tile_HSB)
  //TILE_ARRAY(0x11d0,Tile_Twirl)
  TILE_ARRAY(0x11e0,Tile_Text)
#if !sPLAYER
  TILE_ARRAY(0x11f0,Tile_Export)
  TILE_ARRAY(0x1200,Tile_Import)
#endif
  { 0,0 }
};

/****************************************************************************/
