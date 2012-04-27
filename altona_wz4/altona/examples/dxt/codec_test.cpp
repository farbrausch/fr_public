/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "codec_test.hpp"
#include "base/graphics.hpp"

/****************************************************************************/
/****************************************************************************/

CodecTest::CodecTest() 
{
  /* // this shows that A and B are different!
  sInt a,b;
  for(sInt i=0;i<32;i++)
  {
    a = (i*255+15)/31;
    b = (i<<3)+((i<<3)>>5);
    sDPrintF(L"%2d -> %3d %3d | %d\n",i,a,b,a-b);
  }
  */
}

CodecTest::~CodecTest() 
{
}

/****************************************************************************/

const sChar *CodecTest::GetName()
{
  return L"test";
}

static sU16 make16(sU32 col)
{
  sInt r,g,b;
  r = sClamp<sInt>((((col>>16)&255)+4)>>3,0,31);
  g = sClamp<sInt>((((col>> 8)&255)+2)>>2,0,63);
  b = sClamp<sInt>((((col>> 0)&255)+4)>>3,0,31);

  return (r<<11)|   // 0xf800
         (g<< 5)|   // 0x07e0
         (b<< 0);   // 0x001f
}

static sU32 make32(sU16 col)
{
  sInt r,g,b;

  r = (col&0xf800)>>11;  r = (r*255+15)/31;
  g = (col&0x07e0)>>5;   g = (g*255+31)/63;
  b = (col&0x001f);      b = (b*255+15)/31;

  return 0xff000000|(r<<16)|(g<<8)|(b<<0);
}


void CodecTest::Pack(sImage *bmp,sImageData *dxt,sInt level)      // dummy!
{
  sU16 *d16;
  sU16 c0,c1;
  sU32 map;
  sU32 *s32;

  s32 = bmp->Data;
  d16 = (sU16 *) dxt->Data;
  for(sInt yy=0;yy<bmp->SizeY;yy+=4)
  {
    for(sInt xx=0;xx<bmp->SizeX;xx+=4)
    {
      c0 = make16(s32[xx]);
      c1 = c0;
      map = 0;

      if(level==sTEX_DXT3 || level==sTEX_DXT5)
      {
        *d16++ = ~0;
        *d16++ = ~0;
      }
      
      *d16++ = c0;
      *d16++ = c1;
      *(sU32 *)d16 = map; d16+=2;
    }

    s32 += bmp->SizeX*4;
  }
}

/****************************************************************************/


static void makecol(sU32 *c32,sU16 c0,sU16 c1,sBool opaque)
{
  sU8 *c8 = (sU8 *) c32;

  sInt r0,g0,b0;
  sInt r1,g1,b1;

  r0 = (c0&0xf800)>>11;
  g0 = (c0&0x07e0)>>5; 
  b0 = (c0&0x001f);    
  r1 = (c1&0xf800)>>11;
  g1 = (c1&0x07e0)>>5; 
  b1 = (c1&0x001f);    

  c8[ 0] = (b0*255+15)/31;
  c8[ 1] = (g0*255+31)/63;
  c8[ 2] = (r0*255+15)/31;
  c8[ 3] = 255;
  c8[ 4] = (b1*255+15)/31;
  c8[ 5] = (g1*255+31)/63;
  c8[ 6] = (r1*255+15)/31;
  c8[ 7] = 255;
  if(opaque)
  {
    c8[ 8] = ((b0+b0+b1)*255+45)/(31*3);
    c8[ 9] = ((g0+g0+g1)*255+93)/(63*3);
    c8[10] = ((r0+r0+r1)*255+45)/(31*3);
    c8[11] = 255;
    c8[12] = ((b0+b1+b1)*255+45)/(31*3);
    c8[13] = ((g0+g1+g1)*255+93)/(63*3);
    c8[14] = ((r0+r1+r1)*255+45)/(31*3);
    c8[15] = 255;
  }
  else
  {
    c8[ 8] = ((b0+b1)*255+30)/(31*2);     // INACURATE! (compared to ms)
    c8[ 9] = ((g0+g1)*255+62)/(63*2);     // INACURATE! (compared to ms)
    c8[10] = ((r0+r1)*255+30)/(31*2);     // INACURATE! (compared to ms)
    c8[11] = 255;
    c8[12] = 0;
    c8[13] = 0;
    c8[14] = 0;
    c8[15] = 0;
  }
}

void CodecTest::Unpack(sImage *bmp,sImageData *dxt,sInt level)
{
  sU8 *s;
  sU16 c0,c1;
  sU32 c[4];
  sU32 map;
  sU32 *d32;
  sU64 a64;
  sInt a0,a1;
  sU8 a[8];

  d32 = bmp->Data;
  s = (sU8 *)dxt->Data;
  for(sInt yy=0;yy<bmp->SizeY;yy+=4)
  {
    for(sInt xx=0;xx<bmp->SizeX;xx+=4)
    {
      switch(level)
      {
      case sTEX_DXT1:
      case sTEX_DXT1A:
        c0 = *(sU16 *) (s+0);
        c1 = *(sU16 *) (s+2);
        map = *(sU32 *) (s+4);
        s+=8;
        makecol(&c[0],c0,c1,c0>c1);
        for(sInt y=0;y<4;y++)
        {
          for(sInt x=0;x<4;x++)
          {
            d32[y*bmp->SizeX+xx+x] = c[map&3];
            map = map>>2;
          }
        }
        break;
      case sTEX_DXT3:
        a64 = *(sU64 *) (s+0);
        c0 = *(sU16 *) (s+8);
        c1 = *(sU16 *) (s+10);
        map = *(sU32 *) (s+12);
        s+=16;
        makecol(&c[0],c0,c1,1);
        for(sInt y=0;y<4;y++)
        {
          for(sInt x=0;x<4;x++)
          {
            a0 = (a64&15); a0=a0*255/15;
            d32[y*bmp->SizeX+xx+x] = (c[map&3]&0x00ffffff) | (a0<<24);
            map = map>>2;
            a64 = a64>>4;
          }
        }
        break;
      case sTEX_DXT5:
        a0 = *(sU8 *) (s+0);
        a1 = *(sU8 *) (s+1);
        a64 = (*(sU64 *) (s+0))>>16;
        c0 = *(sU16 *) (s+8);
        c1 = *(sU16 *) (s+10);
        map = *(sU32 *) (s+12);
        s+=16;
        if(a0>a1)
        {
          a[0] = a0;
          a[1] = a1;
          a[2]= (6*a0+1*a1+3)/7;
          a[3]= (5*a0+2*a1+3)/7;
          a[4]= (4*a0+3*a1+3)/7;
          a[5]= (3*a0+4*a1+3)/7;
          a[6]= (2*a0+5*a1+3)/7;
          a[7]= (1*a0+6*a1+3)/7;
        }
        else
        {
          a[0] = a0;
          a[1] = a1;
          a[2]= (4*a0+1*a1+2)/5;
          a[3]= (3*a0+2*a1+2)/5;
          a[4]= (2*a0+3*a1+2)/5;
          a[5]= (1*a0+4*a1+2)/5;
          a[6]= 0;
          a[7]= 255;
        }
        makecol(&c[0],c0,c1,1);
        for(sInt y=0;y<4;y++)
        {
          for(sInt x=0;x<4;x++)
          {
            d32[y*bmp->SizeX+xx+x] = (c[map&3]&0x00ffffff)|(a[a64&7]<<24);
            map = map>>2;
            a64 = a64>>3;
          }
        }
        break;
      }
    }

    d32 += bmp->SizeX*4;
  }
}

/****************************************************************************/
/****************************************************************************/

