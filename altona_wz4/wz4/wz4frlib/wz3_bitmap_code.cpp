/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "wz4frlib/wz3_bitmap_code.hpp"
#include "wz4frlib/wz3_bitmap_ops.hpp"
#include "genvector.hpp"
#include <emmintrin.h>

/****************************************************************************/

sInt GenBitmapTextureSizeOffset;         // 0 = normal, -1 = smaller, 1 = large
sInt GenBitmapDefaultFormat = sTEX_ARGB8888|sTEX_2D;
void __stdcall Bitmap_Inner(sU64 *d,sU64 *s,sInt count,sInt mode,sU64 *x=0);

/****************************************************************************/

struct BilinearContext
{
  sU64 *Src;                      // image data
  sInt XShift;                    // xsize = 1 << XShift
  sInt XSize1,YSize1;             // xsize-1,ysize-1
  sU32 XMax1,YMax1;               // max. valid x-1,y-1
  sU32 XAm,YAm;                   // x-andmask, y-andmask
};

/****************************************************************************/
/****************************************************************************/

sVector30 xPerlinGradient3D[16];
sF32 xPerlinRandom[256][2];
sU8 xPerlinPermute[512];

/****************************************************************************/

static const sChar GTable[16*3] =
{
  1,1,0, 2,1,0, 1,2,0, 2,2,0,
  1,0,1, 2,0,1, 1,0,2, 2,0,2,
  0,1,1, 0,2,1, 0,1,2, 0,2,2,
  1,1,0, 2,1,0, 0,2,1, 0,2,2,
};
static const sF32 GValue[3] = { 0.0f, 1.0, -1.0f };

void xInitPerlin()
{
  sInt i,j,x,y;

  sRandom rnd;

  // 3d gradients
  for(i=0;i<16*3;i++)
    (&xPerlinGradient3D[0].x)[i] = GValue[GTable[i]];

  // permutation
  for(i=0;i<256;i++)
  {
    xPerlinRandom[i][0]=rnd.Int16();
    xPerlinPermute[i]=i;
  }

  for(i=0;i<255;i++)
  {
    for(j=i+1;j<256;j++)
    {
      if(xPerlinRandom[i][0]>xPerlinRandom[j][0])
      {
        sSwap(xPerlinRandom[i][0],xPerlinRandom[j][0]);
        sSwap(xPerlinPermute[i],xPerlinPermute[j]);
      }
    }
  }
  sCopyMem(xPerlinPermute+256,xPerlinPermute,256);

  // random
  for(i=0;i<256;)
  {
    x = rnd.Int16()-0x8000;
    y = rnd.Int16()-0x8000;
    if(x*x+y*y<0x8000*0x8000)
    {
      xPerlinRandom[i][0] = x/32768.0f;
      xPerlinRandom[i][1] = y/32768.0f;
      i++;
    }
  }
}


#define ix is[0]
#define iy is[1]
#define iz is[2]
#define P xPerlinPermute
#define G xPerlinGradient3D

void xPerlin3D(const sVector30 &pos,sVector30 &out)
{
  sVector30 t0,t1,t2;
  sF32 fs[3];
  sInt is[3];

  for(sInt j=0;j<3;j++)
  {
    is[j] = sInt(pos[j]-0.5f);   // integer coordinate
    fs[j] = pos[j] - is[j];  // fractional part
    is[j] &= 255;           // integer grid wraps round 256
    fs[j] = sClamp<sF32>(fs[j],0,1);
    fs[j] = fs[j]*fs[j]*fs[j]*(10.0f+fs[j]*(6.0f*fs[j]-15.0f));
  }

  // trilinear interpolation of grid points
  t0.Fade(fs[0],G[P[P[P[ix]+iy  ]+iz  ]&15],G[P[P[P[ix+1]+iy  ]+iz  ]&15]);
  t1.Fade(fs[0],G[P[P[P[ix]+iy+1]+iz  ]&15],G[P[P[P[ix+1]+iy+1]+iz  ]&15]);
  t0.Fade(fs[1],t0,t1);
  t1.Fade(fs[0],G[P[P[P[ix]+iy  ]+iz+1]&15],G[P[P[P[ix+1]+iy  ]+iz+1]&15]);
  t2.Fade(fs[0],G[P[P[P[ix]+iy+1]+iz+1]&15],G[P[P[P[ix+1]+iy+1]+iz+1]&15]);
  t1.Fade(fs[1],t1,t2);
  t0.Fade(fs[2],t0,t1);

  out = t0;
}
#undef ix
#undef iy
#undef iz
#undef P
#undef G


/****************************************************************************/
/***                                                                      ***/
/***   Helpers                                                            ***/
/***                                                                      ***/
/****************************************************************************/

static GenBitmap *NewBitmap(sInt xs,sInt ys)
{
  GenBitmap *bm;
  sBool dontscale = xs & 0x80;
  xs &= 0x7f;
  if(!dontscale)
  {
    xs += GenBitmapTextureSizeOffset;
    ys += GenBitmapTextureSizeOffset;
  }
  xs = sClamp(xs,0,12);
  ys = sClamp(ys,0,12);
  bm = new GenBitmap;
  bm->Init(1<<xs,1<<ys);
  return bm;
}

static sBool CheckBitmap(GenBitmap *&bm,GenBitmap **inbm=0)
{
  sFatal(L"i hate CheckBitmap");
  /*
  GenBitmap *oldbm = bm;
  
  if(inbm)
    *inbm = bm;

  if(bm==0)
    return 1;
  if(bm->ClassId!=KC_BITMAP)
    return 1;
  if(bm->Data==0)
    return 1;
  if(bm->RefCount>1) 
  {
    if(inbm)
    {
      bm->Release();
      bm = new GenBitmap;
      bm->Init(oldbm->XSize,oldbm->YSize);
    }
    else
    {
      oldbm = bm;
      bm = (GenBitmap *)oldbm->Copy();
      oldbm->Release();
    }
  }
  */
  return 0;
}

static sInt sRange7fff(sInt a)
{
  if((unsigned) a < 0x7fff)
    return a;
  else if(a<0)
    return 0;
  else
    return 0x7fff;
}

static sU64 GetColor64(const sInt *i)
{
  sU64 col;
  col = ((sU64)sRange7fff(i[2]/2))<<0 
      | ((sU64)sRange7fff(i[1]/2))<<16 
      | ((sU64)sRange7fff(i[0]/2))<<32 
      | ((sU64)sRange7fff(i[3]/2))<<48;

  return col;
}

static sU64 GetColor64(sU32 c)
{
  sU64 col;
  col = c;
  col = ((col&0xff000000)<<24) 
      | ((col&0x00ff0000)<<16) 
      | ((col&0x0000ff00)<< 8) 
      | ((col&0x000000ff)    );
  col = ((col | (col<<8))>>1)&0x7fff7fff7fff7fffULL;
  
  return col;
}

static sU64 GetColor64(const sVector4 &c)
{
  sU64 col;
  col = ((sU64)sRange7fff(c.z*0x7fff))<<0 
      | ((sU64)sRange7fff(c.y*0x7fff))<<16 
      | ((sU64)sRange7fff(c.x*0x7fff))<<32 
      | ((sU64)sRange7fff(c.w*0x7fff))<<48;

  return col;
}

static __m128i GetColor128(sU32 c)
{
  sU64 col64 = GetColor64(c);
  return _mm_loadl_epi64((const __m128i *) &col64);
}

static sINLINE __m128i FadeCol(__m128i col0,__m128i col1,sInt fade)
{
  __m128i fadei   = _mm_cvtsi32_si128(-(fade >> 1));
  __m128i fadem   = _mm_shufflelo_epi16(fadei,0x00);

  __m128i diff    = _mm_sub_epi16(col1,col0);
  __m128i diffm   = _mm_mulhi_epi16(diff,fadem);
  __m128i diffms  = _mm_slli_epi16(diffm,1);
  __m128i res     = _mm_subs_epi16(col0,diffms);

  return res;
}

static sINLINE void FadeColStore(sU64 &r,__m128i col0,__m128i col1,sInt fade)
{
  _mm_storel_epi64((__m128i *) &r,FadeCol(col0,col1,fade));
}

static sINLINE void FadeOver(sU64 &r,__m128i col,sInt fade)
{
  __m128i cur     = _mm_loadl_epi64((const __m128i *) &r);
  __m128i faded   = FadeCol(cur,col,fade);
  _mm_storel_epi64((__m128i *) &r,faded);
}

static void AddScalePix(sU64 &r,sU64 &c0,sU64 &c1,sInt fade)
{
  __m128i col0  = _mm_loadl_epi64((const __m128i *) &c0);
  __m128i col1  = _mm_loadl_epi64((const __m128i *) &c1);

  __m128i fadei = _mm_cvtsi32_si128(sMin(fade >> 1,32768));
  __m128i fadem = _mm_shufflelo_epi16(fadei,0x00);

  __m128i mul   = _mm_mulhi_epi16(col1,fadem);
  __m128i muls  = _mm_slli_epi16(mul,1);

  __m128i sum   = _mm_adds_epi16(col0,muls);

  _mm_storel_epi64((__m128i *) &r,sum);
}

static void BilinearSetup(BilinearContext *ctx,sU64 *src,sInt w,sInt h,sInt b)
{
  ctx->Src = src;
  ctx->XShift = sFindLowerPower(w);
  ctx->XSize1 = w-1;
  ctx->YSize1 = h-1;
  ctx->XMax1 = (w << 16) - 1;
  ctx->YMax1 = (h << 16) - 1;
  ctx->XAm = (b&1) ? ~0u : ctx->XMax1;
  ctx->YAm = (b&2) ? ~0u : ctx->YMax1;
}

static void PointFilter(BilinearContext *ctx,sU64 *r,sInt u,sInt v)
{
  // clamp/wrap u,v
  u += 0x8000; // pixel offset for nearest neighbor
  v += 0x8000;

  sU32 uWrap      = u & ctx->XAm;
  sU32 vWrap      = v & ctx->YAm;
  sU32 uClamp     = (uWrap <= ctx->XMax1) ? uWrap : ~(sInt(uWrap) >> 31) & ctx->XMax1;
  sU32 vClamp     = (vWrap <= ctx->YMax1) ? vWrap : ~(sInt(vWrap) >> 31) & ctx->YMax1;

  // sample
  sInt x = uClamp >> 16;
  sInt y = vClamp >> 16;
  *r = ctx->Src[(y << ctx->XShift) + x];
}

static void BilinearFilter(BilinearContext *ctx,sU64 *r,sInt u,sInt v)
{
  // setup, clamp u,v
  const sU64 *src = ctx->Src;
  sU32 uWrap      = u & ctx->XAm;
  sU32 vWrap      = v & ctx->YAm;
  sU32 uClamp     = (uWrap <= ctx->XMax1) ? uWrap : ~(sInt(uWrap) >> 31) & ctx->XMax1;
  sU32 vClamp     = (vWrap <= ctx->YMax1) ? vWrap : ~(sInt(vWrap) >> 31) & ctx->YMax1;

  // preload fade factors
  __m128i fadeu   = _mm_cvtsi32_si128(uClamp);
  __m128i fadev   = _mm_cvtsi32_si128(vClamp);
  __m128i adjust  = _mm_set1_epi16(-0x8000);
  fadeu           = _mm_shufflelo_epi16(fadeu,0x00);
  fadev           = _mm_shufflelo_epi16(fadev,0x00);
  fadeu           = _mm_srli_epi16(fadeu,1);
  fadev           = _mm_srli_epi16(fadev,1);

  // calc u0,u1,v0,v1,s0,s1
  sInt u0         = uClamp >> 16;
  sInt v0         = vClamp >> 16;
  sInt s0         = v0 << ctx->XShift;
  sInt u1         = (u0 + 1) & ctx->XSize1;
  sInt v1         = (v0 + 1) & ctx->YSize1;
  sInt s1         = v1 << ctx->XShift;

  // fetch the pixels
  __m128i pix00   = _mm_loadl_epi64((const __m128i *) (src + s0 + u0));
  __m128i pix01   = _mm_loadl_epi64((const __m128i *) (src + s0 + u1));
  __m128i pix10   = _mm_loadl_epi64((const __m128i *) (src + s1 + u0));
  __m128i pix11   = _mm_loadl_epi64((const __m128i *) (src + s1 + u1));

  // horizontal lerp
  __m128i dhor0   = _mm_sub_epi16(pix01,pix00);
  __m128i dhor1   = _mm_sub_epi16(pix11,pix10);
  __m128i df0     = _mm_mulhi_epi16(dhor0,fadeu);
  __m128i df1     = _mm_mulhi_epi16(dhor1,fadeu);
  __m128i dfs0    = _mm_slli_epi16(df0,1);
  __m128i dfs1    = _mm_slli_epi16(df1,1);
  __m128i row0    = _mm_add_epi16(pix00,dfs0);
  __m128i row1    = _mm_add_epi16(pix10,dfs1);

  // vertical lerp
  __m128i row0a   = _mm_add_epi16(row0,adjust);
  __m128i dvert   = _mm_sub_epi16(row1,row0);
  __m128i dfvert  = _mm_mulhi_epi16(dvert,fadev);
  __m128i dfverts = _mm_slli_epi16(dfvert,1);
  __m128i final   = _mm_add_epi16(row0a,dfverts);
  __m128i clamped = _mm_subs_epu16(final,adjust);

  // store
  _mm_storel_epi64((__m128i *) r,clamped);
}

static sInt GetGamma(const sInt *table,sInt value)
{
  sInt vi = value >> 5;
  return table[vi] + (((table[vi+1]-table[vi]) * (value & 31)) >> 5);
}

/****************************************************************************/
/***                                                                      ***/
/***   Generator                                                          ***/
/***                                                                      ***/
/****************************************************************************/

GenBitmap::GenBitmap()
{
  Type = GenBitmapType;
  Data = 0;
  XSize = 0;
  YSize = 0;
  Size = 0;
}

GenBitmap::~GenBitmap()
{
  if(Data) delete[] Data;
}

void GenBitmap::CopyFrom(wObject *o)
{
  GenBitmap *ob;

  sVERIFY(o->IsType(GenBitmapType));

  ob = (GenBitmap *) o;
  sVERIFY(ob->Data);

//  Texture = sINVALID;
  Init(ob->XSize,ob->YSize);

  sCopyMem(Data,ob->Data,Size*8);
  Atlas=ob->Atlas;
}

void GenBitmap::CopyTo(sImage *img)
{
  img->Init(XSize,YSize);
  sU8 *d = (sU8 *)img->Data;
  const sU16 *s = (const sU16 *) Data;
  for(sInt i=0;i<Size;i++)
  {
    d[0] = ((s[0]>>7)&0x7fff);
    d[1] = ((s[1]>>7)&0x7fff);
    d[2] = ((s[2]>>7)&0x7fff);
    d[3] = ((s[3]>>7)&0x7fff);
    d+=4;
    s+=4;
  }
}

void GenBitmap::CopyTo(sImageI16 *dest)
{
  dest->Init(XSize,YSize);
  const sU16 *s = (const sU16 *) Data;
  for(sInt i=0;i<Size;i++)
  {
    sInt n = sMax(s[0]&0x7fff,sMax(s[1]&0x7fff,s[2]&0x7fff));
    dest->Data[i] = (n<<1)|(n>>14);
    s+=4;
  }
}

void GenBitmap::Init(sInt x,sInt y)
{
  XSize = x;
  YSize = y;
  if(Size!=x*y)
  {
    delete[] Data;
    Size = x*y;
    Data = new sU64[Size];
  }
}

void GenBitmap::Init(const sImage *img)
{
  Init(img->SizeX,img->SizeY);
  sU16 *d = (sU16 *) Data;
  sU8 *s = (sU8 *) img->Data;

  for(sInt i=0;i<Size*4;i++)
    d[i] = (s[i]<<7) | (s[i]>>1);
}

void GenBitmap::Blit(sInt x,sInt y,GenBitmap *src)
{
  if(x<XSize && y<YSize)
  {
    sRect r(x,y,0,0);

    r.x1 = sMin(XSize,x+src->XSize);
    r.y1 = sMin(YSize,y+src->YSize);

    sU64 *s = src->Data;
    sU64 *d = Data + r.y0*XSize + r.x0;
    for(sInt y=0;y<r.SizeY();y++)
    {
      for(sInt x=0;x<r.SizeX();x++)
      {
        d[x]=s[x];
      }
      d+=XSize;
      s+=src->XSize;
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Operators                                                          ***/
/***                                                                      ***/
/****************************************************************************/

void GenBitmap::Flat(sU32 color)
{
  sU64 col = GetColor64(color);
  for(sInt i=0;i<Size;i++)
    Data[i] = col;
}

/****************************************************************************/

static sF32 noise2(sInt x,sInt y,sInt mask,sInt seed)
{
  sInt v00,v01,v10,v11,vx,vy;
  sF32 f00,f01,f10,f11;
  sF32 tx,ty,fa,fb,f;

  mask &= 255;
  vx = (x>>16) & mask; tx=(x&0xffff)/65536.0f;
  vy = (y>>16) & mask; ty=(y&0xffff)/65536.0f;
  v00 = xPerlinPermute[((vx+0)     )+xPerlinPermute[((vy+0)     )^seed]];
  v01 = xPerlinPermute[((vx+1)&mask)+xPerlinPermute[((vy+0)     )^seed]];
  v10 = xPerlinPermute[((vx+0)     )+xPerlinPermute[((vy+1)&mask)^seed]];
  v11 = xPerlinPermute[((vx+1)&mask)+xPerlinPermute[((vy+1)&mask)^seed]];
  f00 = xPerlinRandom[v00][0]*(tx-0)+xPerlinRandom[v00][1]*(ty-0);
  f01 = xPerlinRandom[v01][0]*(tx-1)+xPerlinRandom[v01][1]*(ty-0);
  f10 = xPerlinRandom[v10][0]*(tx-0)+xPerlinRandom[v10][1]*(ty-1);
  f11 = xPerlinRandom[v11][0]*(tx-1)+xPerlinRandom[v11][1]*(ty-1);
  tx = tx*tx*tx*(10+tx*(6*tx-15));
  ty = ty*ty*ty*(10+ty*(6*ty-15));
  //tx = tx*tx*(3-2*tx);
  //ty = ty*ty*(3-2*ty);
  fa = f00+(f01-f00)*tx;
  fb = f10+(f11-f10)*tx;
  f = fa+(fb-fa)*ty;

  return f;
}

void GenBitmap::Perlin(sInt freq,sInt oct,sF32 fadeoff,sInt seed,sInt mode,sF32 amp,sF32 gamma,sU32 col0,sU32 col1)
{
  sInt i;
  sInt x,y,noffs;
  sInt shiftx,shifty;
  sF32 s;
  sU64 *tile;
  sInt gammaTable[1025];

  GenBitmap *bm = this;

  __m128i c0 = GetColor128(col0);
  __m128i c1 = GetColor128(col1);

  shiftx = 16-sFindLowerPower(bm->XSize);
  shifty = 16-sFindLowerPower(bm->YSize);
  tile = bm->Data;
  seed &= 255;
  mode &= 3;

  for(i=0;i<1025;i++)
    gammaTable[i] = sRange7fff(sFPow(i/1024.0f,gamma)*0x8000)*2;

  if(mode & 1)
  {
    amp *= 0x8000;
    noffs = 0;
  }
  else
  {
    amp *= 0x4000;
    noffs = 0x4000;
  }

  sInt ampi = sInt(amp);

  sInt sinTab[257];
  if(mode & 2)
  {
    for(x=0;x<257;x++)
      sinTab[x] = sInt(sFSin(sPI2F * x / 256.0f) * 0.5f * 65536.0f);
  }
#if 1
  sInt *nrow = new sInt[bm->XSize];
  sInt *poly = new sInt[bm->XSize>>freq];

  for(x=0;x<(bm->XSize>>freq);x++)
  {
    sF32 f = 1.0f * x / (bm->XSize>>freq);
    poly[x] = sInt(f*f*f*(10+f*(6*f-15))*16384.0f);
  }

  for(y=0;y<bm->YSize;y++)
  {
    sSetMem(nrow,0,sizeof(sInt)*bm->XSize);
    s = 1.0f;

    // make some noise
    for(i=freq;i<freq+oct;i++)
    {
      sInt xGrpSize = (shiftx+i < 16) ? sMin(bm->XSize,1<<(16-shiftx-i)) : 1;
      sInt groups = (shiftx+i < 16) ? bm->XSize>>(16-shiftx-i) : bm->XSize;
      sInt mask = ((1<<i)-1) & 255;
      sInt py = y<<(shifty+i);

      sInt vy = (py >> 16) & mask;
      sInt dtx = 1 << (shiftx+i);
      sF32 ty = (py & 0xffff) / 65536.0f;
      sF32 tyf = ty*ty*ty*(10+ty*(6*ty-15));
      sF32 ty0f = ty*(1-tyf);
      sF32 ty1f = (ty-1)*tyf;
      sInt vy0 = xPerlinPermute[((vy+0)     )^seed];
      sInt vy1 = xPerlinPermute[((vy+1)&mask)^seed];
      sInt shf = i-freq;
      //sInt si = sFtol(s * ampi);
      sInt si = sInt(s * 65536.0f);

      if(shiftx+i < 16 || (py & 0xffff)) // otherwise, the contribution is always zero
      {
        sInt *rowp = nrow;
        for(sInt vx=0;vx<groups;vx++)
        {
          sInt v00 = xPerlinPermute[((vx+0)&mask)+vy0];
          sInt v01 = xPerlinPermute[((vx+1)&mask)+vy0];
          sInt v10 = xPerlinPermute[((vx+0)&mask)+vy1];
          sInt v11 = xPerlinPermute[((vx+1)&mask)+vy1];

          sF32 f_0h = xPerlinRandom[v00][0] + (xPerlinRandom[v10][0] - xPerlinRandom[v00][0]) * tyf;
          sF32 f_1h = xPerlinRandom[v01][0] + (xPerlinRandom[v11][0] - xPerlinRandom[v01][0]) * tyf;
          sF32 f_0v = xPerlinRandom[v00][1]*ty0f + xPerlinRandom[v10][1]*ty1f;
          sF32 f_1v = xPerlinRandom[v01][1]*ty0f + xPerlinRandom[v11][1]*ty1f;

          sInt fa = sInt(f_0v * 65536.0f);
          sInt fb = sInt((f_1v - f_1h) * 65536.0f);
          sInt fad = sInt(f_0h * dtx);
          sInt fbd = sInt(f_1h * dtx);

          for(sInt xg=0;xg<xGrpSize;xg++)
          {
            sInt nni = fa + (((fb-fa)*poly[xg<<shf])>>14);
            switch(mode)
            {
            case 0:   break;
            case 1:   nni = sAbs(nni); break;
            case 3:   nni &= 0x7fff;
            case 2:
              {
                sInt ind = (nni>>8) & 0xff;
                nni = sinTab[ind] + (((sinTab[ind+1] - sinTab[ind]) * (nni & 0xff)) >> 8);
              }
              break;
            default:  __assume(false); break;
            }
            *rowp++ += sMulShift(nni,si);
            fa += fad;
            fb += fbd;
          }
        }
      }

      s *= fadeoff;
    }

    // resolve
    for(x=0;x<bm->XSize;x++)
      FadeColStore(*tile++,c0,c1,GetGamma(gammaTable,sRange7fff(sMulShift(nrow[x],ampi)+noffs)));
  }

  delete[] nrow;
  delete[] poly;
#else
  for(y=0;y<bm->YSize;y++)
  {
    for(x=0;x<bm->XSize;x++)
    {
      n = 0;
      s = 1.0f;
      for(i=freq;i<freq+oct;i++)
      {
        sInt px = ((x)<<(16-shiftx))<<i;
        sInt py = ((y)<<(16-shifty))<<i;

        nn = noise2(px,py,((1<<i)-1),seed);
        if(mode&1)
          nn = sFAbs(nn);
        if(mode&2)
          nn = sFSin(nn*sPI2F)*0.5f;
        n += nn*s;
        s*=fadeoff;
      }
      /*if(mode&1)
        n = sFPow(n*amp,gamma);
      else
        n = sFPow(n*amp*0.5f+0.5f,gamma);
      val = sRange7fff(n*0x8000)*2;*/
      val = sRange7fff(sFtol(n*amp)+noffs);
      val = GetGamma(gammaTable,val);
      FadeColStore(*tile++,c0,c1,val);
    }
  }
#endif
}

/****************************************************************************/

void GenBitmap::Loop(sInt mode,GenBitmap *srca,GenBitmap *srcb)
{
  sVERIFY(Size==srca->Size);
  sVERIFY(srcb==0 || Size==srcb->Size);
  Bitmap_Inner(Data,srca->Data,Size,mode,srcb?srcb->Data:0);
}

void GenBitmap::Loop(sInt mode,sU64 *srca,GenBitmap *srcb)
{
  sVERIFY(srcb==0 || Size==srcb->Size);
  Bitmap_Inner(Data,srca,Size,mode,srcb?srcb->Data:0);
}

void GenBitmap::PreMulAlpha()
{
  sU16 *data = (sU16 *) Data;
  for(sInt i=0;i<Size;i++)
  {
    sInt a = data[3];
    data[0] = (data[0] * a)>>15; 
    data[1] = (data[1] * a)>>15; 
    data[2] = (data[2] * a)>>15; 
    data+=4;
  }
}

/****************************************************************************/
/****************************************************************************/

// The code gets kinda repetitive without this.
#define LOAD_A \
  __m128i a01 = _mm_load_si128(xtr + i + 0); \
  __m128i a23 = _mm_load_si128(xtr + i + 1)

#define LOAD_AB \
  LOAD_A; \
  __m128i b01 = _mm_load_si128(src + i + 0); \
  __m128i b23 = _mm_load_si128(src + i + 1)

#define LOAD_ABC \
  LOAD_AB; \
  __m128i c01 = _mm_load_si128(dst + i + 0); \
  __m128i c23 = _mm_load_si128(dst + i + 1)

#define STORE_R \
  _mm_store_si128(dst + i + 0,r01); \
  _mm_store_si128(dst + i + 1,r23)

void __stdcall Bitmap_Inner(sU64 *d,sU64 *s,sInt count,sInt mode,sU64 *x)
{
  sVERIFY(count && (count & 3) == 0); // always at least 4 pixels. shouldn't be a problem.

  __m128i *dst        = (__m128i *) d;
  const __m128i *src  = (const __m128i *) s;
  __m128i *xtr        = (__m128i *) x;
  sInt numWords       = count >> 1;

  switch(mode)
  {
  case BI_ADD:
    for(sInt i=0;i<numWords;i+=2)
    {
      LOAD_AB;
      __m128i r01 = _mm_adds_epi16(a01,b01);
      __m128i r23 = _mm_adds_epi16(a23,b23);
      STORE_R;
    }
    break;

  case BI_SUB:
    for(sInt i=0;i<numWords;i+=2)
    {
      LOAD_AB;
      __m128i r01 = _mm_subs_epu16(a01,b01);
      __m128i r23 = _mm_subs_epu16(a23,b23);
      STORE_R;
    }
    break;

  case BI_MUL:
    for(sInt i=0;i<numWords;i+=2)
    {
      LOAD_AB;
      // exact (a*b)/32767 would be nice, but we'll survive this cheap approximation.
      __m128i r01 = _mm_mulhi_epu16(_mm_slli_epi16(a01,1),b01);
      __m128i r23 = _mm_mulhi_epu16(_mm_slli_epi16(a23,1),b23);
      STORE_R;
    }
    break;

  case BI_DIFF:
    {
      __m128i bias = _mm_set1_epi16(0x7fff);
      for(sInt i=0;i<numWords;i+=2)
      {
        LOAD_AB;
        __m128i d01   = _mm_sub_epi16(a01,b01);
        __m128i d23   = _mm_sub_epi16(a23,b23);
        __m128i db01  = _mm_add_epi16(d01,bias);
        __m128i db23  = _mm_add_epi16(d23,bias);
        __m128i r01   = _mm_srli_epi16(db01,1);
        __m128i r23   = _mm_srli_epi16(db23,1);
        STORE_R;
      }
    }
    break;

  case BI_ALPHA:
    {
      static const sALIGNED(sU16,maskc[8],16) = { 0xffff,0xffff,0xffff,0,0xffff,0xffff,0xffff,0 };
      __m128i mask = _mm_load_si128((__m128i *) maskc);

      for(sInt i=0;i<numWords;i+=2)
      {
        LOAD_AB;

        // extract components and shift them into the alpha channel
        __m128i rd01    = _mm_slli_epi64(b01,16);
        __m128i rd23    = _mm_slli_epi64(b23,16);
        __m128i gr01    = _mm_slli_epi64(b01,32);
        __m128i gr23    = _mm_slli_epi64(b23,32);
        __m128i bl01    = _mm_slli_epi64(b01,48);
        __m128i bl23    = _mm_slli_epi64(b23,48);

        // 50:50 mix of red with blue...
        __m128i rb01    = _mm_avg_epu16(bl01,rd01);
        __m128i rb23    = _mm_avg_epu16(bl23,rd23);

        // then 50:50 mix of result with green to get alpha
        __m128i al01    = _mm_avg_epu16(rb01,gr01);
        __m128i al23    = _mm_avg_epu16(rb23,gr23);
        __m128i mal01   = _mm_andnot_si128(mask,al01);
        __m128i mal23   = _mm_andnot_si128(mask,al23);

        // insert into target alpha
        __m128i am01    = _mm_and_si128(a01,mask);
        __m128i am23    = _mm_and_si128(a23,mask);
        __m128i r01     = _mm_or_si128(am01,mal01);
        __m128i r23     = _mm_or_si128(am23,mal23);

        STORE_R;
      }
    }
    break;

  case BI_MULCOL:
    {
      __m128i col = _mm_slli_epi16(_mm_loadl_epi64(src),1);
      col = _mm_shuffle_epi32(col,0x44);
      
      for(sInt i=0;i<numWords;i+=2)
      {
        LOAD_A;
        __m128i r01 = _mm_mulhi_epu16(a01,col);
        __m128i r23 = _mm_mulhi_epu16(a23,col);
        STORE_R;
      }
    }
    break;

  case BI_ADDCOL:
    {
      __m128i col = _mm_loadl_epi64(src);
      col = _mm_shuffle_epi32(col,0x44);
      
      for(sInt i=0;i<numWords;i+=2)
      {
        LOAD_A;
        __m128i r01 = _mm_adds_epi16(a01,col);
        __m128i r23 = _mm_adds_epi16(a23,col);
        STORE_R;
      }
    }
    break;

  case BI_SUBCOL:
    {
      __m128i col = _mm_loadl_epi64(src);
      col = _mm_shuffle_epi32(col,0x44);

      for(sInt i=0;i<numWords;i+=2)
      {
        LOAD_A;
        __m128i r01 = _mm_subs_epu16(a01,col);
        __m128i r23 = _mm_subs_epu16(a23,col);
        STORE_R;
      }
    }
    break;

  case BI_GRAY:
    {
      static const sALIGNED(sU16,maskc[8],16) = { 0,0,0,0xffff,0,0,0,0xffff };
      __m128i mask = _mm_load_si128((__m128i *) maskc);
      
      for(sInt i=0;i<numWords;i+=2)
      {
        LOAD_A;
        
        // extract components
        __m128i rd01    = _mm_srli_epi64(a01,32);
        __m128i rd23    = _mm_srli_epi64(a23,32);
        __m128i gr01    = _mm_srli_epi64(a01,16);
        __m128i gr23    = _mm_srli_epi64(a23,16);

        // 50:50 mix of red with blue...
        __m128i rb01    = _mm_avg_epu16(a01,rd01);
        __m128i rb23    = _mm_avg_epu16(a23,rd23);

        // then 50:50 mix of result with green to get gray
        __m128i gray01  = _mm_avg_epu16(rb01,gr01);
        __m128i gray23  = _mm_avg_epu16(rb23,gr23);

        // expand
        gray01          = _mm_shufflelo_epi16(gray01,0x00);
        gray23          = _mm_shufflelo_epi16(gray23,0x00);
        gray01          = _mm_shufflehi_epi16(gray01,0x00);
        gray23          = _mm_shufflehi_epi16(gray23,0x00);

        // set alpha to opaque
        __m128i r01     = _mm_or_si128(gray01,mask);
        __m128i r23     = _mm_or_si128(gray23,mask);

        STORE_R;
      }
    }
    break;

  case BI_INVERT:
    {
      __m128i xorm = _mm_set1_epi16(0x7fff);

      for(sInt i=0;i<numWords;i+=2)
      {
        LOAD_A;

        __m128i r01     = _mm_xor_si128(a01,xorm);
        __m128i r23     = _mm_xor_si128(a23,xorm);

        STORE_R;
      }
    }
    break;

  case BI_SCALECOL:
    {
      __m128i col = _mm_loadl_epi64(src);
      col = _mm_shuffle_epi32(col,0x44);

      for(sInt i=0;i<numWords;i+=2)
      {
        LOAD_A;

        __m128i lo01    = _mm_mullo_epi16(a01,col);
        __m128i lo23    = _mm_mullo_epi16(a23,col);
        __m128i hi01    = _mm_mulhi_epu16(a01,col);
        __m128i hi23    = _mm_mulhi_epu16(a23,col);

        // swizzle together
        __m128i lohi0   = _mm_unpacklo_epi16(lo01,hi01);
        __m128i lohi1   = _mm_unpackhi_epi16(lo01,hi01);
        __m128i lohi2   = _mm_unpacklo_epi16(lo23,hi23);
        __m128i lohi3   = _mm_unpackhi_epi16(lo23,hi23);

        // descale
        __m128i desc0   = _mm_srai_epi32(lohi0,11);
        __m128i desc1   = _mm_srai_epi32(lohi1,11);
        __m128i desc2   = _mm_srai_epi32(lohi2,11);
        __m128i desc3   = _mm_srai_epi32(lohi3,11);

        // merge back
        __m128i r01     = _mm_packs_epi32(desc0,desc1);
        __m128i r23     = _mm_packs_epi32(desc2,desc3);

        STORE_R;
      }
    }
    break;

  case BI_MERGE:
    {
      __m128i invt = _mm_set1_epi16(0x7fff);

      for(sInt i=0;i<numWords;i+=2)
      {
        LOAD_ABC;

        // do the blend
        __m128i ic01      = _mm_xor_si128(c01,invt);
        __m128i ic23      = _mm_xor_si128(c23,invt);
        __m128i bc01      = _mm_mulhi_epi16(b01,c01);
        __m128i bc23      = _mm_mulhi_epi16(b23,c23);
        __m128i aic01     = _mm_mulhi_epi16(a01,ic01);
        __m128i aic23     = _mm_mulhi_epi16(a23,ic23);
        __m128i sum01     = _mm_add_epi16(aic01,bc01);
        __m128i sum23     = _mm_add_epi16(aic23,bc23);
        __m128i r01       = _mm_slli_epi16(sum01,1);
        __m128i r23       = _mm_slli_epi16(sum23,1);

        STORE_R;
      }
    }
    break;

  case BI_BRIGHTNESS:
    {
      __m128i cmpc = _mm_set1_epi16(0x3fff);

      for(sInt i=0;i<numWords;i+=2)
      {
        LOAD_AB;

        // xor mask
        __m128i mask01a = _mm_cmpgt_epi16(b01,cmpc);
        __m128i mask23a = _mm_cmpgt_epi16(b23,cmpc);
        __m128i mask01  = _mm_srli_epi16(mask01a,1);
        __m128i mask23  = _mm_srli_epi16(mask23a,1);

        // xor inputs
        __m128i ax01    = _mm_xor_si128(a01,mask01);
        __m128i ax23    = _mm_xor_si128(a23,mask23);
        __m128i bx01    = _mm_xor_si128(b01,mask01);
        __m128i bx23    = _mm_xor_si128(b23,mask23);

        // multiply
        __m128i m01     = _mm_mulhi_epi16(ax01,bx01);
        __m128i m23     = _mm_mulhi_epi16(ax23,bx23);
        __m128i ms01    = _mm_slli_epi16(m01,2);
        __m128i ms23    = _mm_slli_epi16(m23,2);

        // xor outputs
        __m128i r01     = _mm_xor_si128(ms01,mask01);
        __m128i r23     = _mm_xor_si128(ms23,mask23);

        STORE_R;
      }
    }
    break;

  case BI_SUBR:
    for(sInt i=0;i<numWords;i+=2)
    {
      LOAD_AB;

      __m128i r01       = _mm_subs_epu16(b01,a01);
      __m128i r23       = _mm_subs_epu16(b23,a23);

      STORE_R;
    }
    break;

  case BI_MULMERGE:
    {
      __m128i invt = _mm_set1_epi16(0x7fff);

      for(sInt i=0;i<numWords;i+=2)
      {
        LOAD_ABC;

        // mulmerge: lerp(a,a*b,c) = a*(1-c) + a*b*c

        // a*b and inv blend factor
        __m128i ab01    = _mm_mulhi_epi16(a01,b01);
        __m128i ab23    = _mm_mulhi_epi16(a23,b23);
        __m128i ic01    = _mm_xor_si128(c01,invt);
        __m128i ic23    = _mm_xor_si128(c23,invt);
        __m128i abs01   = _mm_slli_epi16(ab01,1);
        __m128i abs23   = _mm_slli_epi16(ab23,1);

        // merge it together
        __m128i aic01   = _mm_mulhi_epi16(a01,ic01);
        __m128i aic23   = _mm_mulhi_epi16(a23,ic23);
        __m128i abc01   = _mm_mulhi_epi16(abs01,c01);
        __m128i abc23   = _mm_mulhi_epi16(abs23,c23);
        __m128i sum01   = _mm_add_epi16(aic01,abc01);
        __m128i sum23   = _mm_add_epi16(aic23,abc23);
        __m128i r01     = _mm_slli_epi16(sum01,1);
        __m128i r23     = _mm_slli_epi16(sum23,1);

        STORE_R;
      }
    }
    break;

  case BI_SHARPEN:
    {
      __m128i scale = _mm_loadl_epi64(src);
      __m128i zero = _mm_setzero_si128();
      scale = _mm_shuffle_epi32(scale,0x44);

      for(sInt i=0;i<numWords;i+=2)
      {
        LOAD_A;
        __m128i b01     = _mm_load_si128(dst + i + 0);
        __m128i b23     = _mm_load_si128(dst + i + 1);

        __m128i d01     = _mm_sub_epi16(a01,b01);
        __m128i d23     = _mm_sub_epi16(a23,b23);

        __m128i mlo01   = _mm_mullo_epi16(d01,scale);
        __m128i mlo23   = _mm_mullo_epi16(d23,scale);
        __m128i mhi01   = _mm_mulhi_epi16(d01,scale);
        __m128i mhi23   = _mm_mulhi_epi16(d23,scale);

        // scale it
        __m128i m0      = _mm_unpacklo_epi16(mlo01,mhi01);
        __m128i m1      = _mm_unpackhi_epi16(mlo01,mhi01);
        __m128i m2      = _mm_unpacklo_epi16(mlo23,mhi23);
        __m128i m3      = _mm_unpackhi_epi16(mlo23,mhi23);

        __m128i ms0     = _mm_srai_epi32(m0,11);
        __m128i ms1     = _mm_srai_epi32(m1,11);
        __m128i ms2     = _mm_srai_epi32(m2,11);
        __m128i ms3     = _mm_srai_epi32(m3,11);

        __m128i mp01    = _mm_packs_epi32(ms0,ms1);
        __m128i mp23    = _mm_packs_epi32(ms2,ms3);

        // add onto original image and saturate
        __m128i sum01   = _mm_adds_epi16(a01,mp01);
        __m128i sum23   = _mm_adds_epi16(a23,mp23);
        __m128i r01     = _mm_max_epi16(sum01,zero);
        __m128i r23     = _mm_max_epi16(sum23,zero);

        STORE_R;
      }
    }
    break;

  case BI_HARDLIGHT:
    {
      __m128i andm = _mm_set1_epi16(0x7fff);
      __m128i half = _mm_set1_epi16(0x3fff);

      for(sInt i=0;i<numWords;i+=2)
      {
        LOAD_AB;

        // hard light: (b >= 0.5) ? 1 - (2 * (1 - a) * (1 - b)) : a * b * 2

        __m128i bg01    = _mm_cmpgt_epi16(b01,half);
        __m128i bg23    = _mm_cmpgt_epi16(b23,half);
        __m128i mask01  = _mm_and_si128(bg01,andm);
        __m128i mask23  = _mm_and_si128(bg23,andm);

        __m128i am01    = _mm_xor_si128(a01,mask01);
        __m128i am23    = _mm_xor_si128(a23,mask23);
        __m128i bm01    = _mm_xor_si128(b01,mask01);
        __m128i bm23    = _mm_xor_si128(b23,mask23);

        __m128i ab01    = _mm_mulhi_epi16(am01,bm01);
        __m128i ab23    = _mm_mulhi_epi16(am23,bm23);
        __m128i abs01   = _mm_slli_epi16(ab01,2);
        __m128i abs23   = _mm_slli_epi16(ab23,2);
        
        __m128i r01     = _mm_xor_si128(abs01,mask01);
        __m128i r23     = _mm_xor_si128(abs23,mask23);

        STORE_R;
      }
    }
    break;

  case BI_OVER:
    {
      __m128i zero = _mm_setzero_si128();

      for(sInt i=0;i<numWords;i+=2)
      {
        LOAD_AB;

        __m128i alt01   = _mm_shufflelo_epi16(b01,0xff);
        __m128i alt23   = _mm_shufflelo_epi16(b23,0xff);
        __m128i al01    = _mm_shufflehi_epi16(alt01,0xff);
        __m128i al23    = _mm_shufflehi_epi16(alt23,0xff);
        __m128i d01     = _mm_sub_epi16(b01,a01);
        __m128i d23     = _mm_sub_epi16(b23,a23);
        __m128i dal01   = _mm_mulhi_epi16(d01,al01);
        __m128i dal23   = _mm_mulhi_epi16(d23,al23);
        __m128i dals01  = _mm_slli_epi16(dal01,1);
        __m128i dals23  = _mm_slli_epi16(dal23,1);
        __m128i sum01   = _mm_adds_epi16(a01,dals01);
        __m128i sum23   = _mm_adds_epi16(a23,dals23);
        __m128i r01     = _mm_max_epi16(sum01,zero);
        __m128i r23     = _mm_max_epi16(sum23,zero);

        STORE_R;
      }
    }
    break;

  case BI_ADDSMOOTH:
    {
      __m128i mask = _mm_set1_epi16(0x7fff);

      for(sInt i=0;i<numWords;i+=2)
      {
        LOAD_AB;

        __m128i ai01    = _mm_xor_si128(a01,mask);
        __m128i ai23    = _mm_xor_si128(a23,mask);
        __m128i bi01    = _mm_xor_si128(b01,mask);
        __m128i bi23    = _mm_xor_si128(b23,mask);
        
        __m128i m01     = _mm_mulhi_epi16(ai01,bi01);
        __m128i m23     = _mm_mulhi_epi16(ai23,bi23);
        __m128i ms01    = _mm_slli_epi16(m01,1);
        __m128i ms23    = _mm_slli_epi16(m23,1);
        __m128i r01     = _mm_xor_si128(ms01,mask);
        __m128i r23     = _mm_xor_si128(ms23,mask);

        STORE_R;
      }
    }
    break;

  case BI_MIN:
    for(sInt i=0;i<numWords;i+=2)
    {
      LOAD_AB;

      __m128i r01       = _mm_min_epi16(a01,b01);
      __m128i r23       = _mm_min_epi16(a23,b23);

      STORE_R;
    }
    break;

  case BI_MAX:
    for(sInt i=0;i<numWords;i+=2)
    {
      LOAD_AB;

      __m128i r01       = _mm_max_epi16(a01,b01);
      __m128i r23       = _mm_max_epi16(a23,b23);

      STORE_R;
    }
    break;

  case BI_RANGE:
    {
      __m128i add       = _mm_loadl_epi64(src + 0);
      __m128i mul       = _mm_loadl_epi64((const __m128i *) ((const sU64*) src + 1));
      __m128i zero      = _mm_setzero_si128();

      add = _mm_shuffle_epi32(add,0x44);
      mul = _mm_shuffle_epi32(mul,0x44);
      mul = _mm_sub_epi16(mul,add);

      for(sInt i=0;i<numWords;i+=2)
      {
        LOAD_A;

        __m128i m01     = _mm_mulhi_epi16(a01,mul);
        __m128i m23     = _mm_mulhi_epi16(a23,mul);
        __m128i ms01    = _mm_slli_epi16(m01,1);
        __m128i ms23    = _mm_slli_epi16(m23,1);
        __m128i sum01   = _mm_adds_epi16(ms01,add);
        __m128i sum23   = _mm_adds_epi16(ms23,add);
        __m128i r01     = _mm_max_epi16(sum01,zero);
        __m128i r23     = _mm_max_epi16(sum23,zero);

        STORE_R;
      }
    }
    break;
  }
}

#undef LOAD_A
#undef LOAD_AB
#undef LOAD_ABC
#undef STORE_R

void GenBitmap::Merge(sInt mode,GenBitmap *other)
{
  static sU8 modes[] = 
  {
    BI_ADD,BI_SUB,BI_MUL,BI_DIFF,BI_ALPHA,
    BI_BRIGHTNESS,
    BI_HARDLIGHT,BI_OVER,BI_ADDSMOOTH,
    BI_MIN,BI_MAX
  };

  Bitmap_Inner(Data,other->Data,Size,modes[mode],Data);
}

void GenBitmap::Color(sInt mode,sU32 col)
{
  sU64 color;
  
  color = GetColor64(col);
  Bitmap_Inner(Data,&color,Size,mode+BI_MULCOL,Data);
}

/****************************************************************************/

void GenBitmap::GlowRect(sF32 cx,sF32 cy,sF32 rx,sF32 ry,sF32 sx,sF32 sy,sU32 color,sF32 alpha,sF32 power,sInt wrap,sInt bug)
{
  sU64 *d;
  sInt x,y;
  sF32 a;
  sInt f,fm;
  sF32 fx,fy,tresh;
  sInt gammaTable[1025];
  sInt lowTable[32];
  sBool circular = (bug & 2) == 0;

  if(wrap==1)
  {
    if(cx+rx+sx> 1.0f) GlowRect(cx-1.0f,cy,rx,ry,sx,sy,color,alpha,power,2,bug);
    if(cx-rx-sx<-0.0f) GlowRect(cx+1.0f,cy,rx,ry,sx,sy,color,alpha,power,2,bug);
  }
  if(wrap==1 || wrap==2)
  {
    if(cy+ry+sy> 1.0f) GlowRect(cx,cy-1.0f,rx,ry,sx,sy,color,alpha,power,0,bug);
    if(cy-ry-sy<-0.0f) GlowRect(cx,cy+1.0f,rx,ry,sx,sy,color,alpha,power,0,bug);
  }

  if(power==0) power=(1.0f/65536.0f);
  power = 0.25/power;

  cx*=XSize;
  cy*=YSize;
  rx*=XSize;
  ry*=YSize;
  sx*=XSize;
  sy*=YSize;


  tresh = 1.0f/65536.0f;
  if(rx<tresh) rx=tresh;
  rx = circular ? 1.0f/(rx*rx) : 1.0f / rx;

  if(ry<tresh) ry=tresh;
  ry = circular ? 1.0f/(ry*ry) : 1.0f / ry;

  alpha *= 32768.0f;
  __m128i col = GetColor128(color);
  fm = alpha;

  for(x=0;x<1025;x++)
  {
    if(bug & 1)
      gammaTable[x] = sRange7fff(sFPow(1.0f-x/1024.0f,power)*alpha)*2;
    else
      gammaTable[x] = sRange7fff((1.0f-sFPow(x/1024.0f,power*2.0f))*alpha)*2;
  }

  // there are very steep slopes around 0, so don't try approximating them
  for(x=0;x<32;x++)
  {
    if(bug & 1)
      lowTable[x] = sRange7fff(sFPow(1.0f-x/32768.0f,power)*alpha)*2;
    else
      lowTable[x] = sRange7fff((1.0f-sFPow(x/32768.0f,power*2.0f))*alpha)*2;
  }

  d = Data;
  for(y=0;y<YSize;y++)
  {
    fy = sFAbs(y-cy)-sy;
    if(fy<0) fy = 0;
    fy *= circular ? fy*ry : ry;

    for(x=0;x<XSize;x++)
    {
      fx = sFAbs(x-cx)-sx;
      if(fx<0) fx = 0;
      
      a = circular ? fx*fx*rx+fy : sMax(fx*rx,fy);
      if(a<1.0f-1.0f/32768.0f) // to cull a few more pixels...
      {
        f = sInt(a*32768);
        if(f<32)
          f = lowTable[f];
        else
          f = GetGamma(gammaTable,f);

        FadeOver(*d,col,f);
      }

      d++;
    }
    while(x<XSize);
  }
}

/****************************************************************************/

void GenBitmap::Dots(sU32 color0,sU32 color1,sInt count,sInt seed)
{
  sU64 *d;
  sInt f;
  sInt x;

  __m128i c0 = GetColor128(color0);
  __m128i c1 = GetColor128(color1);
  count = sMulDiv(Size,count,1<<12);

  sRandomMT rnd;
  rnd.Seed(seed);
 
  d = Data;

  do
  {
    f = rnd.Int16();
    x = rnd.Int(Size);
    FadeColStore(d[x],c0,c1,f);
    count--;
  }
  while(count>0);
}

/****************************************************************************/
/***                                                                      ***/
/***   HSCB                                                               ***/
/***                                                                      ***/
/****************************************************************************/

void GenBitmap::HSCB(sF32 fh,sF32 fs,sF32 fc,sF32 fb)
{
  sU16 *d,*s;
  sInt i;
  sInt ch,ffh,ffs;  
  sInt cr,cg,cb,min,max,mm;
  sInt gammaTable[1025];
  sBool adjustHSV;

  d = (sU16 *) Data;
  s = (sU16 *) Data;
  
  fc = fc*fc;
  for(i=0;i<1025;i++)
    gammaTable[i] = sFPow((i*32+0.01)/32768.0f,fc)*32768.0f*fb;

  ffh = sInt(fh * 6 * 65536) % (6*65536);
  if(ffh<-0) ffh+=(6*65536);
  ffs = fs * 65536;
  adjustHSV = ffh != 0 || ffs != 65536;

  for(i=0;i<Size;i++)
  {

// read, gamma, brightness

//    cr = sGamma[d[0]>>8]*fb;
//    cg = Gamma[d[1]>>8]*fb;
//    cb = Gamma[d[2]>>8]*fb;
//    if(fb<0)
//    {
//      cr = -65535*fb+cr;
//      cg = -65535*fb+cg;
//      cb = -65535*fb+cb;
//    }

    cr = GetGamma(gammaTable,s[2]);
    cg = GetGamma(gammaTable,s[1]);
    cb = GetGamma(gammaTable,s[0]);

// convert to hsv
    if(adjustHSV)
    {
      max = sMax(cr,sMax(cg,cb));
      min = sMin(cr,sMin(cg,cb));
      mm = max - min;
      
      if(!mm)
        ch = 0;
      else
      {
        if(cr==max)
          ch = 1*65536 + sDivShift(cg-cb,mm);
        else if(cg==max)
          ch = 3*65536 + sDivShift(cb-cr,mm);
        else
          ch = 5*65536 + sDivShift(cr-cg,mm);
      }

// adjust hue and saturation
      ch += ffh;
      mm = sMulShift(mm,ffs);
      min = max - mm;

// convert to rgb
      if(ch>6*65536) ch-=6*65536;
      
      sInt rch = ch & 131071, m1, m2;
      m1 = min + ((rch >= 65536) ? sMulShift(rch - 65536,mm) : 0);
      m2 = min + ((rch  < 65536) ? sMulShift(65536 - rch,mm) : 0);

      if(ch < 2*65536)
      {
        cr = max;
        cg = m1;
        cb = m2;
      }
      else if(ch < 4*65536)
      {
        cr = m2;
        cg = max;
        cb = m1;
      }
      else
      {
        cr = m1;
        cg = m2;
        cb = max;
      }
    }

    d[2] = sRange7fff(cr);
    d[1] = sRange7fff(cg);
    d[0] = sRange7fff(cb);
    d[3] = s[3];
    d+=4;
    s+=4;
  }
}

/****************************************************************************/

GenBitmap * __stdcall Bitmap_Wavelet(GenBitmap *bm,sInt mode,sInt count)
{
  sU64 *mem;
  sU16 *s,*d;
  sInt steps;
  sInt x,y,xs,ys,f,g;  

  if(CheckBitmap(bm)) return 0;

  xs = bm->XSize;
  ys = bm->YSize;
  f = xs*4;
  g = xs*8;

  mem = new sU64[xs*ys];

  for(steps = 0;steps<count;steps++)
  {
    d = (sU16 *) mem;
    s = (sU16 *) bm->Data;
    for(y=0;y<ys;y++)
    {
      for(x=0;x<xs/2;x++)
      {
        if(mode)
        {
          // c = b-a/2;
          // d = b+a/2;
          
          d[x*8+0] = sClamp<sInt>(s[x*4+0+xs*2]-(s[x*4+0]-0x4000),0,0x7fff);
          d[x*8+1] = sClamp<sInt>(s[x*4+1+xs*2]-(s[x*4+1]-0x4000),0,0x7fff);
          d[x*8+2] = sClamp<sInt>(s[x*4+2+xs*2]-(s[x*4+2]-0x4000),0,0x7fff);
          d[x*8+3] = sClamp<sInt>(s[x*4+3+xs*2]-(s[x*4+3]-0x4000),0,0x7fff);
          d[x*8+4] = sClamp<sInt>(s[x*4+0+xs*2]+(s[x*4+0]-0x4000),0,0x7fff);
          d[x*8+5] = sClamp<sInt>(s[x*4+1+xs*2]+(s[x*4+1]-0x4000),0,0x7fff);
          d[x*8+6] = sClamp<sInt>(s[x*4+2+xs*2]+(s[x*4+2]-0x4000),0,0x7fff);
          d[x*8+7] = sClamp<sInt>(s[x*4+3+xs*2]+(s[x*4+3]-0x4000),0,0x7fff);
          
        }
        else
        {
          // a = (d-c)/2
          // b = (d+c)/2
          d[x*4+0     ] = (s[x*8+4]-s[x*8+0]+0x8000L)/2;
          d[x*4+1     ] = (s[x*8+5]-s[x*8+1]+0x8000L)/2;
          d[x*4+2     ] = (s[x*8+6]-s[x*8+2]+0x8000L)/2;
          d[x*4+3     ] = (s[x*8+7]-s[x*8+3]+0x8000L)/2;
          d[x*4+0+xs*2] = (s[x*8+4]+s[x*8+0])/2;
          d[x*4+1+xs*2] = (s[x*8+5]+s[x*8+1])/2;
          d[x*4+2+xs*2] = (s[x*8+6]+s[x*8+2])/2;
          d[x*4+3+xs*2] = (s[x*8+7]+s[x*8+3])/2;
        }
      }
      s+=xs*4;
      d+=xs*4;
    }

    d = (sU16 *) bm->Data;
    s = (sU16 *) mem;
    for(x=0;x<xs;x++)
    {
      for(y=0;y<ys/2;y++)
      {
        if(mode)
        {
          d[y*g+0  ] = sClamp<sInt>(s[y*f+0+ys*f/2]-(s[y*f+0]-0x4000),0,0x7fff);          
          d[y*g+1  ] = sClamp<sInt>(s[y*f+1+ys*f/2]-(s[y*f+1]-0x4000),0,0x7fff);
          d[y*g+2  ] = sClamp<sInt>(s[y*f+2+ys*f/2]-(s[y*f+2]-0x4000),0,0x7fff);
          d[y*g+3  ] = sClamp<sInt>(s[y*f+3+ys*f/2]-(s[y*f+3]-0x4000),0,0x7fff);
          d[y*g+0+f] = sClamp<sInt>(s[y*f+0+ys*f/2]+(s[y*f+0]-0x4000),0,0x7fff);
          d[y*g+1+f] = sClamp<sInt>(s[y*f+1+ys*f/2]+(s[y*f+1]-0x4000),0,0x7fff);
          d[y*g+2+f] = sClamp<sInt>(s[y*f+2+ys*f/2]+(s[y*f+2]-0x4000),0,0x7fff);
          d[y*g+3+f] = sClamp<sInt>(s[y*f+3+ys*f/2]+(s[y*f+3]-0x4000),0,0x7fff);
        }
        else
        {
          d[y*f+0       ] = (s[y*g+0+f]-s[y*g+0]+0x8000L)/2;
          d[y*f+1       ] = (s[y*g+1+f]-s[y*g+1]+0x8000L)/2;
          d[y*f+2       ] = (s[y*g+2+f]-s[y*g+2]+0x8000L)/2;
          d[y*f+3       ] = (s[y*g+3+f]-s[y*g+3]+0x8000L)/2;
          d[y*f+0+ys*f/2] = (s[y*g+0+f]+s[y*g+0])/2;
          d[y*f+1+ys*f/2] = (s[y*g+1+f]+s[y*g+1])/2;
          d[y*f+2+ys*f/2] = (s[y*g+2+f]+s[y*g+2])/2;
          d[y*f+3+ys*f/2] = (s[y*g+3+f]+s[y*g+3])/2;
        }
      }
      s+=4;
      d+=4;
    }
  }

  s = (sU16*)bm->Data;
  for(steps=0;steps<xs*ys+4;steps++)
  {
    *s = (*s)&0x7fe0;
    s++;
  }

  delete[] mem;

  return bm;
}

/****************************************************************************/
/***                                                                      ***/
/***   Blur                                                               ***/
/***                                                                      ***/
/****************************************************************************/

void GenBitmap::Blur(sInt flags,sF32 sx,sF32 sy,sF32 _amp)
{
 
  sInt x,y;
  sInt size,size2;
  sU32 mask;
  sInt s1,s2,d;
  sU16 *p,*q,*pp,*qq,*po,*qo;
  sInt ordercount;
  sInt amp,ampc,famp;
  sInt f0,f1;
  sInt order;

// prepare

  order = flags & 15;
  if(order==0) return;

  pp = (sU16 *)Data;
  qq = (sU16 *) new sU64[Size];

// blur x

  size = sInt(128*sx*XSize);
  size2 = sInt(128*sy*YSize);
  famp = _amp * 65536.0f * 64;

  sInt repeat = 1;
  do
  {
    f1 = (size&127)/2;
    f0 = 64-f1;
    size = (size/128)*2+1;
    
    amp = famp / sMax<sInt>(1,size*64+f1*2);
    if(amp > 128)
      ampc = sDivShift(65536*64,amp) - 1;
    else
      ampc = 0x7fffffff;

    mask = XSize * 4 - 1;
    po = pp;
    qo = qq;
    y = 0;
    __m128i f1mf0   = _mm_set_epi16(0,f1-f0,0,f1-f0,0,f1-f0,0,f1-f0);
    __m128i f0f1    = _mm_set_epi16(f1,f0,f1,f0,f1,f0,f1,f0);
    __m128i amplo   = _mm_set1_epi16(amp);
    __m128i amphi   = _mm_set1_epi16(amp>>16);
    __m128i ampclip = _mm_set1_epi32(ampc);
    __m128i add     = _mm_set1_epi16(-0x8000);

    do
    {
      pp = po + y*XSize*4;
      qq = qo + y*XSize*4;
      ordercount = order;

      do
      {
        p = pp;
        q = qq;
        pp = q;
        qq = p;

        s2 = s1 = -((size+1)/2)*4;
        d = 0;

        __m128i pix1    = _mm_loadl_epi64((const __m128i *) (p + ((s2+0) & mask)));
        __m128i pix1u   = _mm_unpacklo_epi16(pix1,pix1);
        __m128i akku    = _mm_madd_epi16(pix1u,f1mf0);
        for(sInt x=0;x<size;x++)
        {
          __m128i pix1    = _mm_loadl_epi64((const __m128i *) (p + ((s1+0) & mask)));
          __m128i pix2    = _mm_loadl_epi64((const __m128i *) (p + ((s1+4) & mask)));
          __m128i mixed   = _mm_unpacklo_epi16(pix1,pix2);
          __m128i mult    = _mm_madd_epi16(mixed,f0f1);
          akku            = _mm_add_epi32(akku,mult);
          s1              = s1 + 4;
        }

        for(sInt x=0;x<XSize;x++)
        {
          __m128i pix1    = _mm_loadl_epi64((const __m128i *) (p + ((s1+0) & mask)));
          __m128i pix2    = _mm_loadl_epi64((const __m128i *) (p + ((s1+4) & mask)));
          __m128i mixed   = _mm_unpacklo_epi16(pix1,pix2);
          __m128i mult    = _mm_madd_epi16(mixed,f0f1);
          akku            = _mm_add_epi32(akku,mult);
          s1              = s1 + 4;

          __m128i akkugt  = _mm_cmpgt_epi32(akku,ampclip);
          __m128i akkuhi  = _mm_srli_epi32(akku,22);
          __m128i akkulo  = _mm_srai_epi32(_mm_slli_epi32(akku,10),16);
          __m128i out     = _mm_packs_epi32(akkugt,akkugt);
          __m128i akhi    = _mm_packs_epi32(akkuhi,akkuhi);
          __m128i aklo    = _mm_packs_epi32(akkulo,akkulo);
          out             = _mm_adds_epu16(out,_mm_mullo_epi16(akhi,amplo));
          out             = _mm_adds_epu16(out,_mm_mullo_epi16(aklo,amphi));
          out             = _mm_adds_epu16(out,_mm_mulhi_epu16(aklo,amplo));
          out             = _mm_add_epi16(out,add);
          out             = _mm_subs_epi16(out,add);

          _mm_storel_epi64((__m128i *) (q+d),out);
          d               = d + 4;

          pix1            = _mm_loadl_epi64((const __m128i *) (p + ((s2+0) & mask)));
          pix2            = _mm_loadl_epi64((const __m128i *) (p + ((s2+4) & mask)));
          mixed           = _mm_unpacklo_epi16(pix2,pix1);
          mult            = _mm_madd_epi16(mixed,f0f1);
          akku            = _mm_sub_epi32(akku,mult);
          s2              = s2 + 4;
        }

        ordercount--;
      }
      while(ordercount);
      y++;
    }
    while(y<YSize);
    pp = po;
    qq = qo;
    if(order&1)
      sSwap(pp,qq);

    p = pp;
    q = qq;
    pp = q;
    qq = p;
    s1 = YSize * 4;
    s2 = XSize * 4;
    for(x=0;x<XSize;x+=8)
    {
      p = qq + x*4;
      for(y=0;y<YSize;y++)
      {
        // no point using aligned 128-bit loads here, we scatter on store.
        __m128i m0    = _mm_loadl_epi64((const __m128i *) (p+ 0));
        __m128i m1    = _mm_loadl_epi64((const __m128i *) (p+ 4));
        __m128i m2    = _mm_loadl_epi64((const __m128i *) (p+ 8));
        __m128i m3    = _mm_loadl_epi64((const __m128i *) (p+12));
        __m128i m4    = _mm_loadl_epi64((const __m128i *) (p+16));
        __m128i m5    = _mm_loadl_epi64((const __m128i *) (p+20));
        __m128i m6    = _mm_loadl_epi64((const __m128i *) (p+24));
        __m128i m7    = _mm_loadl_epi64((const __m128i *) (p+28));
        _mm_storel_epi64((__m128i *) (q + 0*s1),m0);
        _mm_storel_epi64((__m128i *) (q + 1*s1),m1);
        _mm_storel_epi64((__m128i *) (q + 2*s1),m2);
        _mm_storel_epi64((__m128i *) (q + 3*s1),m3);
        _mm_storel_epi64((__m128i *) (q + 4*s1),m4);
        _mm_storel_epi64((__m128i *) (q + 5*s1),m5);
        _mm_storel_epi64((__m128i *) (q + 6*s1),m6);
        _mm_storel_epi64((__m128i *) (q + 7*s1),m7);

        q += 4;
        p += s2;
      }
      q += 7*s1;
    }
    sSwap(XSize,YSize);
    size = size2;
  }
  while(repeat--);
  
  sVERIFY(qq!=(sU16 *)Data);
  delete[] qq;
}

/****************************************************************************/
/***                                                                      ***/
/***   Mask                                                               ***/
/***                                                                      ***/
/****************************************************************************/

GenBitmap * __stdcall Bitmap_Mask(GenBitmap *bm,GenBitmap *bb,GenBitmap *bc,sInt mode)
{
  sInt size;
  GenBitmap *in;

  if(CheckBitmap(bm,&in)) return 0;

  size = bm->Size;
  Bitmap_Inner(bm->Data,bm->Data,size,BI_GRAY,in->Data);
  switch(mode)
  {
  case 0:
    Bitmap_Inner(bm->Data,bb->Data,size,BI_MERGE,bc->Data);
    break;
  case 1:
    Bitmap_Inner(bm->Data,bb->Data,size,BI_MUL,bm->Data);
    Bitmap_Inner(bm->Data,bc->Data,size,BI_ADD,bm->Data);
    break;
  case 2:
    Bitmap_Inner(bm->Data,bb->Data,size,BI_MUL,bm->Data);
    Bitmap_Inner(bm->Data,bc->Data,size,BI_SUBR,bm->Data);
    break;
  case 3:
    Bitmap_Inner(bm->Data,bb->Data,size,BI_MULMERGE,bc->Data);
    break;
  }

  bb->Release();
  bc->Release();

  return bm;
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void GenBitmap::Rotate(GenBitmap *in,sF32 cx,sF32 cy,sF32 angle,sF32 sx,sF32 sy,sF32 tx,sF32 ty,sInt border)
{
  sInt x,y;
  sU16 *d;
  sU64 *s;
  sInt xs,ys;
  sInt txs,tys;
  sInt m00,m01,m10,m11,m20,m21;
  sF32 rotangle;
  BilinearContext ctx;

// prepare
  xs = in->XSize;
  ys = in->YSize;
  txs = XSize;
  tys = YSize;
  d = (sU16 *)Data;
  s = in->Data;

  if(in==this)
  {
    s = new sU64[in->Size];
    sCopyMem(s,in->Data,in->Size*8);
  }

// rotate

  rotangle = angle*sPI2F;

  m00 = sInt( sFCos(rotangle)*0x10000*xs/(sx*txs));
  m01 = sInt( sFSin(rotangle)*0x10000*ys/(sx*txs));
  m10 = sInt(-sFSin(rotangle)*0x10000*xs/(sy*tys));
  m11 = sInt( sFCos(rotangle)*0x10000*ys/(sy*tys));
  m20 = sInt( tx*xs*0x10000 - (txs*m00*cx+tys*m10*cy));
  m21 = sInt( ty*ys*0x10000 - (txs*m01*cx+tys*m11*cy));
//  m20 = sInt( tx*xs*0x10000 - ((txs-1)*m00+(tys-1)*m10)/2);
//  m21 = sInt( ty*ys*0x10000 - ((txs-1)*m01+(tys-1)*m11)/2);
  BilinearSetup(&ctx,s,xs,ys,border);
  
  for(y=0;y<tys;y++)
  {
    sInt u = y*m10+m20;
    sInt v = y*m11+m21;

    if(border & 4)
    {
      for(x=0;x<txs;x++)
      {
        PointFilter(&ctx,(sU64 *)d,u,v);
        u += m00;
        v += m01;
        d += 4;
      }
    }
    else
    {
      for(x=0;x<txs;x++)
      {
        BilinearFilter(&ctx,(sU64 *)d,u,v);
        u += m00;
        v += m01;
        d += 4;
      }
    }
  }

  if(s!=in->Data)
    delete[] s;
}

static sInt CSLookup(const sInt *table,sInt value)
{
  sInt ind = (value >> 6);
  return table[ind] + (((table[ind+1] - table[ind]) * (value & 63)) >> 6);
}

void GenBitmap::Twirl(GenBitmap *src,sF32 strength,sF32 gamma,sF32 rx,sF32 ry,sF32 cx,sF32 cy,sInt border)
{
  sInt CSTable[2][1025];
  sInt x,y;
  sU16 *s,*d;
  sInt xs,ys;
  sInt px,py,dx,dy,u,v;
  BilinearContext ctx;

  sVERIFY(Size==src->Size);

// prepare

  if(rx!=0 && ry!=0)
  {
    d = (sU16 *)Data;
    s = (sU16 *)src->Data;

    xs = XSize;
    ys = YSize;
    rx = rx*rx;
    ry = ry*ry;
//    rx*=0.25f;
//    ry*=0.25f;
    strength*=sPI2F;
    BilinearSetup(&ctx,(sU64 *)s,xs,ys,border);

  // calc table
    for(x=0;x<=1024;x++)
    {
      sF32 dist = sFPow(x / 1024.0f,gamma)*strength;
      sF32 dsin,dcos;

      sFSinCos(dist,dsin,dcos);
      CSTable[0][x] = 65536.0f * dsin;
      CSTable[1][x] = 65536.0f * dcos;
    }

    sInt fcx = cx * 65536.0f;
    sInt fcy = cy * 65536.0f;
    sInt frx = rx * 65536.0f;
    sInt fry = ry * 65536.0f;
    sInt xstep = 0x10000 / xs;
    sInt ystep = 0x10000 / ys;

  // rotate

    py = 0;

    for(y=0;y<ys;y++)
    {
      dy = py - fcy;
      sInt distb = 0x10000 - sMulDiv(dy,dy,fry);

      px = 0;

      for(x=0;x<xs;x++)
      {
        dx = px - fcx;
        sInt dist = distb - sMulDiv(dx,dx,frx);
        
        if(dist>0)
        {
          sInt fsin = CSLookup(CSTable[0],dist);
          sInt fcos = CSLookup(CSTable[1],dist);

          u = fcx + sMulShift(dx,fcos) + sMulShift(dy,fsin);
          v = fcy - sMulShift(dx,fsin) + sMulShift(dy,fcos);
        }
        else
        {
          u = px;
          v = py;
        }
        
        //BilinearFilter((sU64*)d,(sU64*)s,xs,ys,sInt(u*0x10000*xs),sInt(v*0x10000*ys),border);
        //BilinearFilter(&ctx,(sU64 *)d,sInt(u*0x10000*xs),sInt(v*0x10000*ys));
        BilinearFilter(&ctx,(sU64 *)d,u*xs,v*ys);
        d+=4;

        px += xstep;
      }

      py += ystep;
    }
  }
  else
  {
    CopyFrom(src);
  }
}

void GenBitmap::RotateMul(sF32 cx,sF32 cy,sF32 angle,sF32 sx,sF32 sy,sF32 tx,sF32 ty,sInt border,sU32 color,sInt mode,sInt count,sU32 fade)
{
  GenBitmap *bb,*bo;
  sInt cr,cg,cb,ca,i;
  Color(0,color);
  bb = new GenBitmap;
  bb->InitSize(this);
  bo = new GenBitmap;
  bo->CopyFrom(this);

  sF32 sgnx = sx < 0.0f ? -1.0f : 1.0f;
  sF32 sgny = sy < 0.0f ? -1.0f : 1.0f;
  sx *= sgnx;
  sy *= sgny;

  if(mode&16)
  {
    angle /= 1<<count;
    tx = (tx-0.5)/((1<<count)-1)+0.5f;
    ty = (ty-0.5)/((1<<count)-1)+0.5f;

    sx = sFPow(sx,1.0f/((1<<count)-1));
    sy = sFPow(sy,1.0f/((1<<count)-1));

    ca = (fade>>24)&0xff;
    cr = (fade>>16)&0xff;
    cg = (fade>> 8)&0xff;
    cb = (fade    )&0xff;
  }
  else
  {
    i = 1;
    angle /= count;
    tx = (tx-0.5)/(count)+0.5f;
    ty = (ty-0.5)/(count)+0.5f;
    sx = sFPow(sx,1.0f/count);
    sy = sFPow(sy,1.0f/count);
  }

  sx *= sgnx;
  sy *= sgny;

  while(count>0)
  {
    count--;
    if(mode&16)
    {
      if(fade!=0xffffffff)
      {
        Color(0,fade);
        ca = (ca*ca)>>8;
        cr = (cr*cr)>>8;
        cg = (cg*cg)>>8;
        cb = (cb*cb)>>8;
        fade = (ca<<24)|(cr<<16)|(cg<<8)|(cb);
      }
      bb->Rotate(this,cx,cy,angle,sx,sy,tx,ty,border);
      Merge(mode&15,bb);

      angle = angle*2;
      tx = (tx-0.5f)*2+0.5f;
      ty = (ty-0.5f)*2+0.5f;
      sx = sx*sx;
      sy = sy*sy;
    }
    else
    {
      if(fade!=0xffffffff)
        Color(0,fade);
      bb->Rotate(bo,cx,cy,angle*i,(sx-1)*i+1,(sy-1)*i+1,(tx-0.5)*i+0.5f,(ty-0.5)*i+0.5f,border);
      Merge(mode&15,bb);
      i++;
    }
  }
  bb->Release();
  bo->Release();
}

/****************************************************************************/

void GenBitmap::Distort(GenBitmap *src,GenBitmap *map,sF32 dist,sInt border)
{
  sInt x,y;
  sU16 *t,*d,*a;
  sInt xs,ys;
  sInt u,v;
  sInt bumpx,bumpy;
  BilinearContext ctx;

  sVERIFY(Size==src->Size);
  sVERIFY(Size==map->Size);

// prepare

  t = (sU16 *)Data;
  d = (sU16 *)src->Data;
  a = (sU16 *)map->Data;
  xs = XSize;
  ys = YSize;
  bumpx = (dist*xs)*4;
  bumpy = (dist*ys)*4;
  BilinearSetup(&ctx,(sU64 *)d,xs,ys,border);

// rotate 

  for(y=0;y<ys;y++)
  {
    for(x=0;x<xs;x++)
    {
      u = ((x)<<16) + ((a[2]-0x4000)*bumpx);
      v = ((y)<<16) + ((a[1]-0x4000)*bumpy);
      BilinearFilter(&ctx,(sU64 *)t,u,v);
      //BilinearFilter((sU64*)t,(sU64*)d,xs,ys,u,v,border);
      t+=4;
      a+=4;
    }
  }
}

/****************************************************************************/

static sInt filterbump(sU16 *s,sInt pos,sInt mask,sInt step)
{
  return s[(pos-step-step)&mask]*1 
       + s[(pos-step     )&mask]*3 
       - s[(pos          )&mask]*3 
       - s[(pos+step     )&mask]*1;
}

static sInt filterbumpsharp(sU16 *s,sInt pos,sInt mask,sInt step)
{
  return 4*(s[(pos-step)&mask] - s[pos&mask]);
}

void GenBitmap::Normals(GenBitmap *src,sF32 _dist,sInt mode)
{
  sInt shiftx,shifty;
  sInt xs,ys;
  sInt x,y;
  //sU64 *mem;
  sU16 *sx,*sy,*s,*d;
  sInt vx,vy,vz;
  sF32 e;
  sInt dist;

  sVERIFY(Size==src->Size);

  dist = sInt(_dist*65536.0f);
  d = (sU16 *) Data;
  sx = sy = s = (sU16*) src->Data;
  xs = src->XSize;
  ys = src->YSize; 
  shiftx = sFindLowerPower(src->XSize);
  shifty = sFindLowerPower(src->YSize);

  for(y=0;y<ys;y++)
  {
    sy = s;
    for(x=0;x<xs;x++)
    {
      if(mode&4)
      {
        vx = filterbumpsharp(sx,x*4,(xs-1)*4,4);
        vy = filterbumpsharp(sy,y*xs*4,(ys-1)*xs*4,xs*4);
      }
      else
      {
        vx = filterbump(sx,x*4,(xs-1)*4,4);
        vy = filterbump(sy,y*xs*4,(ys-1)*xs*4,xs*4);
      }
/*
      vx = sx[((x-2)&(xs-1))*4]*1
         + sx[((x-1)&(xs-1))*4]*3
         - sx[((x  )&(xs-1))*4]*3
         - sx[((x+1)&(xs-1))*4]*1;      

      vy = sy[((y-2)&(ys-1))*xs*4]*1
         + sy[((y-1)&(ys-1))*xs*4]*3
         - sy[((y  )&(ys-1))*xs*4]*3
         - sy[((y+1)&(ys-1))*xs*4]*1;
*/
      vx = sRange7fff((((vx) * (dist>>4))>>(20-shiftx))+0x4000)-0x4000;
      vy = sRange7fff((((vy) * (dist>>4))>>(20-shifty))+0x4000)-0x4000;
      vz = 0;

      if(mode&1)
      {
        vz = (0x3fff*0x3fff)-vx*vx-vy*vy;
        if(vz>0)
        {
//          vz = sFSqrt(vz/16384.0f/16384.0f)*0x3fff;
          vz = sFSqrt(vz);
        }
        else
        {
          e = sFInvSqrt(vx*vx+vy*vy)*0x3fff;
          vx *= e;
          vy *= e;
          vz = 0;
        }
      }
      if(mode&2)
      {
        sSwap(vx,vy);
        vy=-vy;
      }

      d[0] = vz+0x4000;
      d[1] = vy+0x4000;
      d[2] = vx+0x4000;
      d[3] = 0xffff;

      d += 4;
      sy += 4;
    }
    sx += xs*4;
  }
}

/****************************************************************************/

void GenBitmap::Unwrap(GenBitmap *src,sInt mode)
{
  BilinearContext ctx;

  BilinearSetup(&ctx,src->Data,src->XSize,src->YSize,(mode >> 4) & 3);

  Init(src->XSize,src->YSize);

  sU16 *d = (sU16 *) Data;
  sF32 invX = 1.0f / XSize;
  sF32 invY = 1.0f / YSize;
  sInt usc = XSize << 16;
  sInt vsc = YSize << 16;

  for(sInt y=0;y<YSize;y++)
  {
    sF32 fy = y * invY,fyc = 0.5f - fy;

    for(sInt x=0;x<XSize;x++)
    {
      sF32 fx = x * invX,fxc = fx - 0.5f;
      sF32 u,v;

      switch(mode & 3)
      {
      case 0: // polar2normal
        sFSinCos(fx * sPI2F,v,u);
        u = (1.0f + u * fy) * 0.5f;
        v = (1.0f - v * fy) * 0.5f;
        break;

      case 1: // normal2polar
        u = sFATan2(fyc,fxc) / sPI2F;
        if(u<0.0f) u += 1.0f;
        v = sFSqrt(fxc*fxc + fyc*fyc) * 2.0f;
        break;
        
      case 2: // rect2normal
        if(sFAbs(fyc) <= sFAbs(fxc)) // 1st or 3rd quadrant
        {
          v = sFAbs(fxc) * 2.0f;
          u = 0.25f - fy / (v * 2.0f);
          if(fxc < 0.0f)
            u = 0.75f - u;
        }
        else // 2nd or 4th quadrant
        {
          v = sFAbs(fyc) * 2.0f;
          u = 0.5f - fx / (v * 2.0f);
          if(fyc < 0.0f)
            u = 1.0f - u;
        }
        break;
      }

      BilinearFilter(&ctx,(sU64 *) d,u*usc,v*vsc);
      d += 4;
    }
  }
}

/****************************************************************************/

void GenBitmap::Bulge(GenBitmap *src,sF32 warp)
{
  BilinearContext ctx;

  BilinearSetup(&ctx,src->Data,src->XSize,src->YSize,0);
  Init(src->XSize,src->YSize);

  sU16 *d = (sU16 *) Data;
  sF32 invX = 1.0f / XSize;
  sF32 invY = 1.0f / YSize;
  sInt usc = XSize << 16;
  sInt vsc = YSize << 16;

  for(sInt y=0;y<YSize;y++)
  {
    sF32 fy = y * invY,fyc = (0.5f - fy) * 2.0f;

    for(sInt x=0;x<XSize;x++)
    {
      sF32 fx = x * invX,fxc = (fx - 0.5f) * 2.0f;
      
      sF32 u = fxc;
      sF32 v = fyc;

      sF32 rsq = u*u + v*v;
      if(rsq <= 1.0f)
      {
        sF32 iz = 1.0f / (1.0f + warp * sFSqrt(1.0f - rsq));
        u *= iz;
        v *= iz;
      }

      u = (1.0f + u) * 0.5f;
      v = (1.0f - v) * 0.5f;

      BilinearFilter(&ctx,(sU64 *) d,u*usc,v*vsc);
      d += 4;
    }
  }
}

/****************************************************************************/
/****************************************************************************/

void GenBitmap::Bump(GenBitmap *bb,sInt subcode,sF32 px,sF32 py,sF32 pz,sF32 da,sF32 db,
                     sU32 _diff,sU32 _ambi,sF32 outer,sF32 falloff,sF32 amp,
                     sU32 _spec,sF32 spow,sF32 samp)
{
  sInt xs,ys;
  sInt x,y;
  sU16 *d,*b,*s;
  sInt i;

  sU16 diff[4];
  sU16 ambi[4];
  sU16 spec[4];
  sU16 buff[4];
  sF32 dx,dy,dz;                  // spot direction
  sF32 e;
  sF32 f0;

  sF32 lx,ly,lz;                  // light -> material
  sF32 nx,ny,nz;                  // material normal
  sF32 hx,hy,hz;                  // halfway vector (specular)
  sF32 df;                        // spot direction factor
  sF32 lf;                        // light factor
  sF32 sf;                        // specular factor

  xs = XSize;
  ys = YSize;

  *(sU64 *)diff = GetColor64(_diff);
  *(sU64 *)ambi = GetColor64(_ambi);
  *(sU64 *)spec = GetColor64(_spec);

  px = px*xs;
  py = py*ys;
  pz = pz*xs;
  da *= sPI2F;
  db *= sPIF;

  dx = dy = sFCos(db);
  dx *= sFSin(da);
  dy *= sFCos(da);
  dz = sFSin(db);


  s = (sU16 *)Data;
  d = (sU16 *)Data;
  if(bb)
  {
    b = (sU16 *)bb->Data;
  }
  else
  {
    b = 0;
  }


  lf = 1.0f;
  sf = 0.0f;
  df = 1.0f;
  nx = 0;
  ny = 0;
  nz = 1;
  lx = dx;
  ly = dy;
  lz = dz;

  if(subcode==0)
  {
    px = px-dx*pz/dz;
    py = py-dy*pz/dz;
  }

  samp *= 65536.0f;

  for(y=0;y<YSize;y++)
  {
    for(x=0;x<XSize;x++)
    {

      if(subcode!=2)
      {
        lx = x-px;
        ly = y-py;
        lz = pz;
        e = sFRSqrt(lx*lx+ly*ly+lz*lz);
        lx*=e;
        ly*=e;
        lz*=e;
      }

      if(b)
      {
        nx = (b[2]-0x4000)/16384.0f;
        ny = (b[1]-0x4000)/16384.0f;
        nz = (b[0]-0x4000)/16384.0f;
        b+=4;
      }

      lf = lx*nx+ly*ny+lz*nz;

      if(samp)
      {
        hx = lx;
        hy = ly;
        hz = lz+1;
        e = sFRSqrt(hx*hx+hy*hy+hz*hz);
        sf = hx*nx+hy*ny+hz*nz;
        if(sf<0) sf=0;
        sf = sPow(sf*e,spow);
      }

      if(subcode==0)
      {
        df = (lx*dx+ly*dy+lz*dz);
        if(df<outer)
          df = 0;
        else
          df = sPow((df-outer)/(1-outer),falloff);
      }

      f0 = df*lf*amp;
      for(i=0;i<4;i++)
        buff[i] = sRange7fff(sInt(s[i]*(ambi[i]+diff[i]*f0)/0x8000));
      AddScalePix(*(sU64 *)d,*(sU64 *)buff,*(sU64 *)spec,sInt(df*sf*samp));
      s+=4;
      d+=4;
    }
  }
}

/****************************************************************************/

void GenBitmap::Downsample(GenBitmap *in,sInt flags)
{
  sInt stepx = 1;
  sInt stepy = 1;
  if(flags==1)
  {
    stepx = sMax(1,in->XSize/XSize);
    stepy = sMax(1,in->YSize/YSize);
  }
  sInt div = stepx*stepy;
  sU16 *s = (sU16 *)in->Data;
  sInt spitch = in->XSize*4;
  sU16 *d = (sU16 *)Data;
  for(sInt y=0;y<YSize;y++)
  {
    for(sInt x=0;x<XSize;x++)
    {
      sInt srcx = x*in->XSize/XSize;
      sInt srcy = y*in->YSize/YSize;
      sInt cr=0,cg=0,cb=0,ca=0;
      for(sInt yy=0;yy<stepy;yy++)
      {
        sU16 *ss = s+(srcy+yy)*spitch+srcx*4;
        for(sInt xx=0;xx<stepx;xx++)
        {
          cr += ss[0];
          cg += ss[1];
          cb += ss[2];
          ca += ss[3];
          ss+=4;
        }
      }
      d[0] = cr/div;
      d[1] = cg/div;
      d[2] = cb/div;
      d[3] = ca/div;
      d+=4;
    }
  }
}

/****************************************************************************/

void GenBitmap::Text(sF32 x,sF32 y,sF32 width,sF32 height,sU32 col,sU32 flags,sF32 lineskip,const sChar *text,const sChar *fontname)
{
  sU32 *bitmem;

  sInt xi,yi,i;
  sU32 fade;
  sInt xs,ys,xp,yp,yf,xp0,lsk;
  sU32 *s;

  sInt page;
  const sInt alias = 4;

  xs = XSize*alias;
  ys = YSize*alias;
  xp = x*xs;
  yp = y*ys;

  page = (flags & 0x70)>>4;
  __m128i color = GetColor128(col);

  sInt createflags = 0;
  if(flags & 0x100) createflags |= sF2C_ITALICS;
  if(flags & 0x200) createflags |= sF2C_BOLD;
  if(flags & 0x400) createflags |= sF2C_SYMBOLS;

  sRender2DBegin(xs,ys);
  sRect2D(0,0,xs,ys,sGC_BLACK);
  sFont2D *font = new sFont2D(fontname,ys*height,createflags,xs*width);
  yf = font->GetHeight();
  sSetColor2D(sGC_MAX+0,0x000000);
  sSetColor2D(sGC_MAX+1,0xffffff);
  font->SetColor(sGC_MAX+1,sGC_MAX+0);
  lsk = lineskip * yf;

  sInt lf = 0;
  for(i=0;text[i];i++)
    if(text[i]=='\n') lf++;
  if(flags&2)
    yp -= yf*(lf+1)/2;
  while(*text)
  {
    for(i=0;text[i] && text[i]!='\n';i++) ;

    if(i>0)
    {
      if(flags&1)
        xp0 = xp-font->GetWidth(text,i)/2;
      else
        xp0 = xp;
      font->Print(0,xp0,yp,text,i);
    }

    text+=i;
    if(*text=='\n')
    {
      yp += lsk;
      text++;
    }
  }
   
  // write to bitmap

  bitmem = new sU32[xs*ys];
  sRender2DGet(bitmem);
  i = 0;
  for(yi=0;yi<YSize;yi++)
  {
    s = bitmem+yi*xs*alias;
    for(xi=0;xi<XSize;xi++)
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


      fade = (fade)|(fade<<8);
      FadeOver(Data[i],color,fade);
      i++;
      s+=alias;
    }
  }
  delete[] bitmem;

  delete font;
  sRender2DEnd();
}

/****************************************************************************/


/****************************************************************************/

void GenBitmap::Cell(sU32 col0,sU32 col1,sU32 col2,sInt max,sInt seed,sF32 amp,sF32 gamma,sInt mode,sF32 mindistf,sInt percent,sF32 aspect)
{
  sInt cells[256][4];
  sInt x,y,i,j,dist,best,best2,besti,best2i;
  sInt dx,dy,px,py;
  sInt shiftx,shifty;
  sF32 v0,v1;
  sInt val,mdist;
  sU64 *tile;
  sBool cut;


  sRandomMT rnd;
  rnd.Seed(seed);
  for(i=0;i<max;i++)
  {
    cells[i][0] = rnd.Int(0x4000);
    cells[i][1] = rnd.Int(0x4000);
    cells[i][2] = rnd.Int(0x4000);
    cells[i][3] = 0;
  }

  mdist = sInt(mindistf*0x4000);
  mdist = mdist*mdist;
  for(i=1;i<max;)
  {
    if((mode&2) && (sInt)rnd.Int(255)<percent)
      cells[i][2] = 0xffff;
    px = ((cells[i][0])&0x3fff)-0x2000;
    py = ((cells[i][1])&0x3fff)-0x2000; 
    cut = sFALSE;
    for(j=0;j<i && !cut;j++)
    {
      dx = ((cells[j][0]-px)&0x3fff)-0x2000;
      dy = ((cells[j][1]-py)&0x3fff)-0x2000; 
      dist = dx*dx+dy*dy;
      if(dist<mdist)
      {
        cut = sTRUE;
      }
    }
    if(cut)
    {
      max--;
      cells[i][0] = cells[max][0];
      cells[i][1] = cells[max][1];
      cells[i][2] = cells[max][2];
    }
    else
    {
      i++;
    }
  }

  shiftx = 14-sFindLowerPower(XSize);
  shifty = 14-sFindLowerPower(YSize);
  __m128i c0 = GetColor128(col1);
  __m128i c1 = GetColor128(col0);
  __m128i cb = GetColor128(col2);
  tile = Data;
  aspect = sFPow(2,aspect);

#if 1 // optimized cells code
  sF32 aspdiv;
  sInt aspf;
  sBool flipxy;

  if(aspect >= 1.0f)
  {
    aspf = 65536 / (aspect * aspect);
    aspdiv = aspect / 16384.0f;
    flipxy = sFALSE;
  }
  else
  {
    aspf = aspect * aspect * 65536;
    aspdiv = 1.0f / (16384.0f * aspect);
    flipxy = sTRUE;
  }

  if(flipxy)
  {
    for(i=0;i<max;i++)
      sSwap(cells[i][0],cells[i][1]);
  }

  static const int tileSize = 16;
  for(sInt by=0;by<YSize;by+=tileSize)
  {
    for(sInt bx=0;bx<XSize;bx+=tileSize)
    {
      // for all cells, calc distance lower bound
      sInt px0 = bx << shiftx, px1 = (bx+tileSize-1) << shiftx;
      sInt py0 = by << shifty, py1 = (by+tileSize-1) << shifty;

      if(flipxy)
      {
        sSwap(px0,py0);
        sSwap(px1,py1);
      }

      for(i=0;i<max;i++)
      {
        dx = ((cells[i][0]-px0)&0x3fff)-0x2000;
        dy = ((cells[i][0]-px1)&0x3fff)-0x2000;
        if((dx ^ dy) <= 0)
          cells[i][3] = 0;
        else
        {
          dx = sMin(sAbs(dx),sAbs(dy));
          cells[i][3] = sMulShift(dx*dx,aspf);
        }

        dx = ((cells[i][1]-py0)&0x3fff)-0x2000;
        dy = ((cells[i][1]-py1)&0x3fff)-0x2000;
        if((dx ^ dy) > 0)
        {
          dy = sMin(sAbs(dx),sAbs(dy));
          cells[i][3] += dy*dy;
        }
      }

      // (insertion) sort by it
      for(i=1;i<max;i++)
      {
        sInt x = cells[i][0], y = cells[i][1], c = cells[i][2];
        sInt dy = cells[i][3], j = i;

        while(j && cells[j-1][3] > dy)
        {
          cells[j][0] = cells[j-1][0];
          cells[j][1] = cells[j-1][1];
          cells[j][2] = cells[j-1][2];
          cells[j][3] = cells[j-1][3];
          j--;
        }

        cells[j][0] = x;
        cells[j][1] = y;
        cells[j][2] = c;
        cells[j][3] = dy;
      }

      // render tile
      tile = Data + by*XSize + bx;

      for(sInt ty=0;ty<tileSize;ty++)
      {
        py = (by+ty) << shifty;

        for(sInt tx=0;tx<tileSize;tx++)
        {
          px = (bx+tx) << shiftx;
          
          if(flipxy)
            x = py, y = px;
          else
            x = px, y = py;

          best = 0x8000*0x8000;
          best2 = 0x8000*0x8000;
          besti = best2i = -1;

          // search for points
          for(sInt i=0;i<max && best2 > cells[i][3];i++)
          {
            dx = ((cells[i][0]-x)&0x3fff)-0x2000;
            dy = ((cells[i][1]-y)&0x3fff)-0x2000;
            dist = sMulShift(dx*dx,aspf) + dy*dy;
            if(dist<best)
            {
              best2 = best;
              best2i = besti;
              best = dist;
              besti = i;
            }
            else if(dist>best && dist<best2)
            {
              best2 = dist;
              best2i = i;
            }
          }

          v0 = sFSqrt(best) * aspdiv;
          if(mode&1)
          {
            v1 = sFSqrt(best2) * aspdiv;
            if(v0+v1>0.00001f)
              v0 = (v1-v0)/(v1+v0);
            else
              v0 = 0;
          }
          val = sRange7fff(sFPow(v0*amp,gamma)*0x8000)*2; // the sFPow is the biggest individual CPU hog here
          if(mode&4)
            val = 0x10000-val;

          if(mode&2)
          {
            __m128i cc = FadeCol(c0,c1,cells[besti][2]*4);
            if(cells[besti][2]==0xffff)
              cc=cb;
            FadeColStore(*tile,cc,cb,val);
          }
          else
            FadeColStore(*tile,c0,c1,val);
          tile++;
        }

        tile += XSize-tileSize;
      }
    }
  }
#else
  sF32 aspsquare = aspect * aspect;
  sF32 aspdiv = 1.0f / (16384.0f * aspect);

  for(y=0;y<YSize;y++)
  {
    py = (y) << shifty;
    for(i=0;i<max;i++)
    {
      dy = ((cells[i][1]-py)&0x3fff)-0x2000;
      cells[i][3] = dy*dy*aspsquare;
    }

    for(x=0;x<XSize;x++)
    {
      best = 0x8000*0x8000;
      best2 = 0x8000*0x8000;
      besti = -1;
      px = (x)<<(shiftx);
      for(i=0;i<max;i++)
      {
        dx = ((cells[i][0]-px)&0x3fff)-0x2000;
        dist = dx*dx + cells[i][3];
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

      if(mode&2)
      {
        __m128i cc = FadeCol(c0,c1,cells[besti][2]*4);
        if(cells[besti][2]==0xffff)
          cc=cb;
        FadeColStore(*tile,cc,cb,val);
      }
      else
        FadeColStore(*tile,c0,c1,val);
      tile++;
    }
  }
#endif
}

/****************************************************************************/

GenBitmap * __stdcall Bitmap_Gradient(sInt xs,sInt ys,sU32 col0,sU32 col1,sF32 posf,sF32 a,sF32 length,sInt mode)
{
  GenBitmap *bm;
  sInt c,cdx,cdy,x,y;
  sInt dx,dy,pos;
  sF32 l;
  sInt val;
  sU64 *tile;

  bm = NewBitmap(xs,ys);

  __m128i c0 = GetColor128(col0);
  __m128i c1 = GetColor128(col1);

  l = 32768.0f / length;
  pos = posf*32768.0f;
  dx = sInt(sFCos(a * sPI2) * l);
  dy = sInt(sFSin(a * sPI2) * l);
  cdx = sMulShift(dx,0x10000/bm->XSize);
  cdy = sMulShift(dy,0x10000/bm->YSize)-bm->XSize*cdx;
  
  c = 0x4000-(dx/2+dy/2)*(posf+1);

  tile = bm->Data;
  for(y=0;y<bm->YSize;y++)
  {
    for(x=0;x<bm->XSize;x++)
    {
      switch(mode)
      {
      case 0:
        val = sRange7fff(c)*2;
        break;
      case 1:
        val = sInt(sFSin(sRange7fff(c)*sPI2/0x8000+0x2000)*0x7fff+0x7fff);
        break;
      case 2:
        val = sInt(sFSin(sRange7fff(c)*sPI2/0x10000)*0xffff);
        break;
      }
      FadeColStore(*tile,c0,c1,val);
      c += cdx;
      tile++;
    }

    c += cdy;
  }

  return bm;
}

/****************************************************************************/

void GenBitmap::Sharpen(GenBitmap *in,sInt order,sF32 sx,sF32 sy,sF32 amp)
{
  sU64 color;

  color = sClamp<sInt>(amp*0x800,-0x7fff,0x7fff)&0xffff;
  color |= color<<16;
  color |= color<<32;

  CopyFrom(in);
  Blur(order,sx,sy,1.0f);
  Bitmap_Inner(Data,&color,Size,BI_SHARPEN,in->Data);
}

/****************************************************************************/
/****************************************************************************/

static sInt CBLookup(const sInt *table,sInt value)
{
  sInt ind = (value >> 7);
  return table[ind] + (((table[ind+1] - table[ind]) * (value & 127)) >> 7);
}

void GenBitmap::ColorBalance(sVector30 shadows,sVector30 midtones,sVector30 highlights)
{
  sInt i,j;
  sF32 x;
  sF32 vsha,vmid,vhil;
  sF32 p;
  sF32 min,max,msc;
  sInt table[3][257];

  static const sF32 sc = 100.0f / 255.0f;

  // lookup tables
  for(j=0;j<3;j++)
  {
    vsha = (&shadows.x)[j];
    vmid = (&midtones.x)[j];
    vhil = (&highlights.x)[j];

    p = sFPow(0.5f,vsha * 0.5f + vmid + vhil * 0.5f);
    min = -sMin(vsha,0.0f) * sc;
    max = 1.0f - sMax(vhil,0.0f) * sc;
    msc = 1.0f / (max - min);

    for(i=0;i<=256;i++)
    {
      x = sFPow((sClamp(i/256.0f,min,max) - min) * msc,p);
      table[j][i] = sRange7fff(x * 32768.0f);
    }
  }

  // now just apply lookup tables
  sU16 *d = (sU16 *) Data;
  sU16 *s = (sU16 *) Data;

  for(i=0;i<Size;i++)
  {
    d[0] = CBLookup(table[2],s[0]);
    d[1] = CBLookup(table[1],s[1]);
    d[2] = CBLookup(table[0],s[2]);
    d[3] = s[3];

    d += 4;
    s += 4;
  }
}

/****************************************************************************/
/****************************************************************************/

void GenBitmap::Bricks(
  sInt bmxs,sInt bmys,
  sInt color0,sInt color1,sInt colorf,
  sF32 ffugex,sF32 ffugey,sInt tx,sInt ty,
  sInt seed,sInt heads,sInt flags,
  sF32 side,sF32 colorbalance)
{
  sU64 *d;
  sInt fx,fy;
  sInt fugex,fugey;
  sInt bx,by;
  sInt f;
  sInt kopf,kopftrigger;
  sInt sidestep,sideakku;
  sInt multiply;
  struct Cell
  {
    __m128i Color;
    sInt Kopf;
  } *cells;

  Init(bmxs,bmys);
  
  sRandom rnd;
  rnd.Seed(seed);

  d = Data;
  __m128i c0 = GetColor128(color0);
  __m128i c1 = GetColor128(color1);
  __m128i cf = GetColor128(colorf);
  __m128i cb;
  fugex = ffugex*0x2000; 
  fugey = ffugey*0x2000; 
  sidestep = side*0x4000;
  multiply = 1<<((flags>>4)&7);

  cells = new Cell[tx*multiply*ty*multiply];
  kopf = 0;
//  cb = FadeCol(c0,c1,sFPow(sFGetRnd(),colorbalance)*0x10000);

// distribute heads

  for(sInt y=0;y<ty;y++)
  {
    kopftrigger = 0;
    for(sInt x=0;x<tx;x++)
    {
      kopftrigger--;
      cells[y*tx*multiply+x].Color = _mm_setzero_si128();
      cells[y*tx*multiply+x].Kopf = kopf;
      if(kopf==0)
      {
        kopf = 1;
      }
      else
      {
        if((sInt)rnd.Int(255)>heads || kopftrigger>=0)
        {
          kopf = 0;
        }
        else
          if(flags&4) kopftrigger=2;
      }
//      if(kopf)
//        cb = FadeCol(c0,c1,sFPow(sFGetRnd(),colorbalance)*0x10000);
    }
    if(!cells[y*tx*multiply].Kopf && !cells[y*tx*multiply+tx-1].Kopf)
      cells[y*tx*multiply].Kopf = 1;
    if(flags & 4)
    {
      if(cells[y*tx*multiply].Kopf && cells[y*tx*multiply+1].Kopf && cells[y*tx*multiply+2].Kopf)
        cells[y*tx*multiply+1].Kopf = 0;
      if(cells[y*tx*multiply+tx-1].Kopf && cells[y*tx*multiply].Kopf && cells[y*tx*multiply+1].Kopf)
        cells[y*tx*multiply].Kopf = 0;
      if(cells[y*tx*multiply+tx-2].Kopf && cells[y*tx*multiply+tx-1].Kopf && cells[y*tx*multiply].Kopf)
        cells[y*tx*multiply+tx-1].Kopf = 0;
    }
  }

// multiply head pattern

  for(sInt yy=0;yy<multiply;yy++)
  {
    for(sInt y=0;y<ty;y++)
    {
      for(sInt xx=0;xx<multiply;xx++)
      {
        for(sInt x=0;x<tx;x++)
        {
          cells[(yy*ty+y)*(multiply*tx)+xx*tx+x] = cells[(y)*(multiply*tx)+x];
        }
      }
    }
  }

// distribute colors

  tx *= multiply;
  ty *= multiply;


  cb = FadeCol(c0,c1,sFPow(rnd.Float(1),colorbalance)*0x10000);
  for(sInt y=0;y<ty;y++)
  {
    for(sInt x=0;x<tx;x++)
    {
      if(cells[y*tx+x].Kopf)
        cb = FadeCol(c0,c1,sFPow(rnd.Float(1),colorbalance)*0x10000);
      cells[y*tx+x].Color = cb;
    }
    if(!cells[y*tx].Kopf)
      cells[y*tx].Color = cells[y*tx+tx-1].Color;
  }

// do the painting

  sInt bmx = 0x4000/XSize*tx;
  sInt bmy = 0x4000/YSize*ty;
  sideakku = 0;
  for(sInt y=0;y<YSize;y++)
  {
    fy = 0x4000/YSize*y*ty;
    by = fy>>14;
    sVERIFY(by<ty);
    fy = fy&0x3fff;
    sideakku = (by*sidestep)&0x7fff;
    for(sInt x=0;x<XSize;x++)
    {
      fx = 0x4000/XSize*x*tx;
      fx += sideakku;
      bx = fx>>14;
      if(bx>=tx) bx-=tx;
      fx = fx&0x3fff;

      if(flags&8)
      {
        f = 0x4000;
        if(cells[by*tx+(bx+0)%tx].Kopf)
          if(       fx<fugex) f = 0x4000*sMax(0,((       fx)-(fugex-bmx)))/bmx;
        if(cells[by*tx+(bx+1)%tx].Kopf)
          if(0x4000-fx<fugex) f = 0x4000*sMax(0,((0x4000-fx)-(fugex-bmx)))/bmx;
        if(       fy<fugey) f = sMin(f,0x4000*sMax(0,((       fy)-(fugey-bmy)))/bmy);
        if(0x4000-fy<fugey) f = sMin(f,0x4000*sMax(0,((0x4000-fy)-(fugey-bmy)))/bmy);
      }
      else
      {
        f = 0x4000;
        if(cells[by*tx+(bx+0)%tx].Kopf)
          if(       fx<fugex) f = 0x4000*        fx /fugex;
        if(cells[by*tx+(bx+1)%tx].Kopf)
          if(0x4000-fx<fugex) f = 0x4000*(0x4000-fx)/fugex;
        if(       fy<fugey) f = sMin(f,0x4000*        fy /fugey);
        if(0x4000-fy<fugey) f = sMin(f,0x4000*(0x4000-fy)/fugey);
      }
      FadeColStore(*d,cf,cells[by*tx+bx%tx].Color,f*4);
      d++;
    }
  }

  delete[] cells;
}

/****************************************************************************/

void CalcGenBitmapVectorLoop(struct GenBitmapArrayVector *e,sInt count,sArray<GenBitmapVectorLoop> &a)
{
  sInt start = 0;
  while(start<count)
  {
    sInt end = count;
    for(sInt i=start+1;i<count;i++)
    {
      if(e[i].restart&15)
      {
        end = i;
        break;
      }
    }
    GenBitmapVectorLoop l;
    l.mode = e[start].restart;
    l.start = start;
    l.end = end;

    if((l.mode&7)==2)
    {
      sInt d = (end-start)/3;
      l.end = start+d*3;
      if(d>=2)
        a.AddTail(l);
    }
    else if((l.mode&7)==1)
    {
      if(end-start>=3)
        a.AddTail(l);
    }
    start = end; 
  }
}


void GenBitmap::Vector(sU32 color,GenBitmapArrayVector *arr,sInt count)
{
  Vectorizer::VectorRasterizer vec(this);

  sInt xs = XSize;
  sInt ys = YSize;
  sInt start = 0;
  sInt raster = -1;
  sInt mode = 0;
  while(start<count)
  {
    sInt end = count;
    for(sInt i=start+1;i<count;i++)
    {
      if(arr[i].restart&15)
      {
        end = i;
        break;
      }
    }
    if(arr[start].restart & 7)
      mode = arr[start].restart & 7;

    if(mode==1 && end-start>=3)
    {
      if(raster==-1)
        raster = start;
      sVector2 p1(arr[end-1].x*xs,arr[end-1].y*ys);
      for(sInt i=start;i<end;i++)
      {
        sVector2 p0(arr[i].x*xs,arr[i].y*ys);
        vec.line(p1,p0);
        p1 = p0;
      }
    }
    else if(mode==2 && end-start>=6)
    {
      if(raster==-1)
        raster = start;
      for(sInt i=start;i<end-2;i+=3)
      {
        sInt ie = i+3;
        if(ie==end) ie=start;
        sVector2 a(arr[i+0].x*xs,arr[i+0].y*ys);
        sVector2 b(arr[i+1].x*xs,arr[i+1].y*ys);
        sVector2 c(arr[i+2].x*xs,arr[i+2].y*ys);
        sVector2 d(arr[ie ].x*xs,arr[ie ].y*ys);

        vec.cubic(a,b,c,d);
      }
    }
    start = end;
    if(raster>=0 && (start==count || (arr[start].restart&8)==0))
    {
      vec.rasterizeAll(arr[raster].col,(arr[raster].restart & 16)?1:0);
      raster = -1;
    }
  }
}

/****************************************************************************/

GenBitmap * __stdcall Bitmap_Paste(GenBitmap *bm,GenBitmap *bs,sF32 prex,sF32 prey,sF32 posx,sF32 posy,sF32 sizex,sF32 sizey,sF32 angle,sInt mode)
{
  if(CheckBitmap(bm)) return 0;

  // sign stuff
  if(prex < 0.0f)
    prex = -prex, sizex = -sizex;

  if(prey < 0.0f)
    prey = -prey, sizey = -sizey;

  prex *= bs->XSize;
  prey *= bs->YSize;

  // calc transformed quad points
  sF32 s,c;
  sFSinCos(angle*sPI2F,s,c);

  sF32 ux = sizex * c;
  sF32 uy = sizex * -s;
  sF32 vx = sizey * s;
  sF32 vy = sizey * c;
  sF32 orgx = posx - 0.5f * (ux + vx);
  sF32 orgy = posy - 0.5f * (uy + vy);

  // calculate bounding rect
  sInt XRes = bm->XSize, YRes = bm->YSize;
  sInt minX = sMax<sInt>(0,((orgx + sMin(ux,0.0f) + sMin(vx,0.0f)) * XRes) - 1);
  sInt minY = sMax<sInt>(0,((orgy + sMin(uy,0.0f) + sMin(vy,0.0f)) * YRes) - 1);
  sInt maxX = sMin<sInt>(XRes-1,((orgx + sMax(ux,0.0f) + sMax(vx,0.0f)) * XRes) + 1);
  sInt maxY = sMin<sInt>(YRes-1,((orgy + sMax(uy,0.0f) + sMax(vy,0.0f)) * YRes) + 1);

  // solve for u0,v0 and deltas (Cramer's rule)
  sF32 detM = ux*vy - uy*vx;
  if(fabs(detM) * XRes * YRes < 0.25f) // smaller than a pixel? skip it.
    return bm;

  sF32 invM = (1 << 16) / detM;
  sF32 rmx = (minX + 0.5f) / XRes - orgx;
  sF32 rmy = (minY + 0.5f) / YRes - orgy;
  sInt u0 = (rmx*vy - rmy*vx) * invM * prex;
  sInt v0 = (ux*rmy - uy*rmx) * invM * prey;
  sInt dudx = vy * invM * prex / XRes;
  sInt dvdx = -uy * invM * prey / XRes;
  sInt dudy = -vx * invM * prex / YRes;
  sInt dvdy = ux * invM * prey / YRes;
  sU32 maxU = prex * 65536;
  sU32 maxV = prey * 65536;

  sU32 mskUV = ~0u;
  sInt uvOff = 0;
  if(mode & 128) // nearest neighbor
  {
    mskUV = ~0xffff;
    uvOff = 0x8000;
  }

  mode &= 127;

  // render
  BilinearContext ctx;
  BilinearSetup(&ctx,bs->Data,bs->XSize,bs->YSize,0);

  for(sInt y=minY;y<=maxY;y++)
  {
    sU64 *out = &bm->Data[y*XRes];
    sInt u = u0;
    sInt v = v0;

    for(sInt x=minX;x<=maxX;x++)
    {
      if((unsigned) u < maxU && (unsigned) v < maxV)
      {
        sU64 pix;
        BilinearFilter(&ctx,&pix,(u & mskUV) + uvOff,(v & mskUV) + uvOff);

        __m128i pixr = _mm_loadl_epi64((const __m128i *) &pix);

        if(mode)
          FadeOver(out[x],pixr,(pix >> 48) << 1);
        else
          out[x] = pix;
      }

      u += dudx;
      v += dvdx;
    }

    u0 += dudy;
    v0 += dvdy;
  }

  return bm;
}

/****************************************************************************/
/***                                                                      ***/
/***   New Ops                                                            ***/
/***                                                                      ***/
/****************************************************************************/

void GenBitmap::Gradient(GenBitmapGradientPoint *g,sInt count,sInt flags)
{
  sU64 *row = new sU64[XSize];
  sVector4 c;

  if(count==0)
  {
    for(sInt i=0;i<XSize;i++)
      row[i] = 0;
  }
  else if(count==1)
  {
    c = g[0].Color;
    if(flags & 0x10)
    {
      c.x *= c.w;
      c.y *= c.w;
      c.z *= c.w;
    }
    sU64 col = ::GetColor64(c);

    for(sInt i=0;i<XSize;i++)
      row[i] = col;
  }
  else
  {
    // sort input

    for(sInt i=0;i<count-1;i++)
      for(sInt j=i+1;j<count;j++)
        if(g[i].Pos>g[j].Pos)
          sSwap(g[i],g[j]);

    // write colors

    for(sInt x=0;x<XSize;x++)
    {
      sF32 p = x/sF32(XSize-1);
      sU64 col;

      if(p<=g[0].Pos)
      {
        c = g[0].Color;
      }
      else if(p>=g[count-1].Pos)
      {
        c = g[count-1].Color;
      }
      else
      {
        sInt i=1;
        for(;i<count-1;i++)
          if(p<g[i].Pos)
            break;
        i--;
        GenBitmapGradientPoint p0,p1,p2,p3;

        p1 = g[i];
        p2 = g[i+1];
        if(i-1>=0)
        {
          p0 = g[i-1];
        }
        else
        {
          p0.Color = p1.Color-2*p1.Color;
          p0.Pos   = p1.Pos  -2*p1.Pos;
        }
        if(i+2<count)
        {
          p3 = g[i+2];
        }
        else
        {
          p3.Color = p2.Color-p1.Color*2;
          p3.Pos   = p2.Pos  -p1.Pos *2;
        }
        sF32 c0,c1,c2,c3;
        sF32 t = (p-p1.Pos)/(p2.Pos-p1.Pos);

        switch(flags & 15)
        {
        case 0:
        default:
          c0 = c2 = c3 = 0;
          c1 = 1;
          break;
        case 1:
          c0 = c3 = 0;
          c1 = 1-t;
          c2 = t;
          break;
        case 2:
          sHermite(t,c0,c1,c2,c3);
          break;
        case 3:
          c0 = 0;
          c2 = 3*t*t-2*t*t*t;
          c1 = 1-c2;
          c3 = 0;
          break;
        }

        c = p0.Color*c0 + p1.Color*c1 + p2.Color*c2 + p3.Color*c3;
      }
      if(flags & 0x10)
      {
        c.x *= c.w;
        c.y *= c.w;
        c.z *= c.w;
      }
      col = ::GetColor64(c);
      row[x] = col;
    }
  }

  // copy result

  for(sInt y=0;y<YSize;y++)
    sCopyMem(Data+y*XSize,row,XSize*8);

  delete[] row;
}


/****************************************************************************/
/****************************************************************************/

sU64 GenBitmap::GetColor64(sU32 c)
{
  return ::GetColor64(c);
}

void GenBitmap::Fade64(sU64 &r,sU64 &c0,sU64 &c1,sInt fade)
{
  __m128i col0    = _mm_loadl_epi64((const __m128i *) &c0);
  __m128i col1    = _mm_loadl_epi64((const __m128i *) &c1);
  FadeColStore(r,col0,col1,fade);
}

/****************************************************************************/
/****************************************************************************/


sInt LoadAtlas(const sChar *name, GenBitmap *bmp)
{
  sInt w,h;
  sInt cnt=0;
  sInt pos=0;
  sImage *ret=0;
  sString<1024> str(name);
  sImage *images[2048];
    
  pos=str.Count()-1;

  while (pos>=0&&(str[pos]<'0'||str[pos]>'9')) pos--;

  while (sCheckFile(str) && cnt<2048)
  {
    sInt p=pos;
    images[cnt]=new sImage();
    images[cnt]->Load(str);    
    str[p]++;
    while (str[p]==58)
    {
      str[p]='0';
      p--;		
      str[p]++;	  	  
    }	
    cnt++;
  }

  if (cnt)
  {
    w=images[0]->SizeX;
    h=images[0]->SizeY;

    sF32 fw=w;
    sF32 fh=h;

    sInt dx= sFCeil(sFSqrt(fh*cnt/fw));    
    sInt tw=1<<sFindHigherPower(dx*w);        
    dx=(tw+w-1)/w;        
    sInt dy= (cnt+dx-1)/dx;
    sInt th=1<<sFindHigherPower(dy*h);
    sF32 ftw=tw;
    sF32 fth=th;
        
    ret=new sImage(tw,th);

    ret->Fill(0);
    bmp->Atlas.Entries.Resize(cnt);

    for (int i=0;i<cnt;i++)
    {
      sInt x=i%dx;
      sInt y=i/dx;
      ret->BlitFrom(images[i],0,0,w*x,h*y,w,h);
      bmp->Atlas.Entries[i].Pixels.x0=w*x;
      bmp->Atlas.Entries[i].Pixels.y0=h*y;
      bmp->Atlas.Entries[i].Pixels.x1=w*(x+1);
      bmp->Atlas.Entries[i].Pixels.y1=h*(y+1);
      bmp->Atlas.Entries[i].UVs.x0=bmp->Atlas.Entries[i].Pixels.x0/ftw;
      bmp->Atlas.Entries[i].UVs.y0=bmp->Atlas.Entries[i].Pixels.y0/fth;
      bmp->Atlas.Entries[i].UVs.x1=bmp->Atlas.Entries[i].Pixels.x1/ftw;
      bmp->Atlas.Entries[i].UVs.y1=bmp->Atlas.Entries[i].Pixels.y1/fth;
      delete images[i];
    }
    bmp->Init(ret);
    delete ret;
    return 0;
  } 
  return 1;
}

/****************************************************************************/
