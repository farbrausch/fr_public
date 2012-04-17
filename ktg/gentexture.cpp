/****************************************************************************/
/***                                                                      ***/
/***   Written by Fabian Giesen.                                          ***/
/***   I hereby place this code in the public domain.                     ***/
/***                                                                      ***/
/****************************************************************************/

#include "gentexture.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Helpers                                                            ***/
/***                                                                      ***/
/****************************************************************************/

// Return sTRUE if x is a power of 2, sFALSE otherwise
static sBool IsPowerOf2(sInt x)
{
  return (x & (x-1)) == 0;
}

// Returns floor(log2(x))
static sInt FloorLog2(sInt x)
{
  sInt res = 0;

  if(x & 0xffff0000)  x >>= 16, res += 16;
  if(x & 0x0000ff00)  x >>=  8, res +=  8;
  if(x & 0x000000f0)  x >>=  4, res +=  4;
  if(x & 0x0000000c)  x >>=  2, res +=  2;
  if(x & 0x00000002)  res++;

  return res;
}

// Multiply intensities.
// Returns the result of round(a*b/65535.0)
static sU32 MulIntens(sU32 a,sU32 b)
{
  sU32 x = a*b + 0x8000;
  return (x + (x >> 16)) >> 16;
}

// Returns the result of round(a*b/65536)
static sInt MulShift16(sInt a,sInt b)
{
  return (sS64(a) * sS64(b) + 0x8000) >> 16;
}

// Returns the result of round(a*b/256)
static sU32 UMulShift8(sU32 a,sU32 b)
{
  return (sU64(a) * sU64(b) + 0x80) >> 8;
}

// Linearly interpolate between a and b with t=0..65536 [0,1]
// 0<=a,b<65536.
static sInt Lerp(sInt t,sInt a,sInt b)
{
  return a + ((t * (b-a)) >> 16);
}

static sF32 LerpF(sF32 t,sF32 a,sF32 b)
{
  return a + t * (b - a);
}

// Perlin permutation table
static sU16 Ptable[4096];
static sU32 *Ptemp;

static sInt P(sInt i)
{
  return Ptable[i&4095];
}

// Initialize perlin
static int InitPerlinCompare(const void *e1, const void *e2)
{
  unsigned i1 = Ptemp[*((sU16 *) e1)];
  unsigned i2 = Ptemp[*((sU16 *) e2)];

  return i1-i2;
}

static void InitPerlin()
{
  sU32 seed = 0x93638245u;
  Ptemp = new sU32[4096];

  // generate 4096 pseudorandom numbers using LFSR
  for(sInt i=0;i<4096;i++)
  {
    Ptemp[i] = seed;
    seed = (seed << 1) ^ ((seed & 0x80000000u) ? 0xc0000401u : 0);
  }

  for(sInt i=0;i<4096;i++)
    Ptable[i] = i;

  qsort(Ptable,4096,sizeof(*Ptable),InitPerlinCompare);

  delete[] Ptemp;
  Ptemp = 0;
}

// Perlin gradient function
static sF32 PGradient2(sInt hash,sF32 x,sF32 y)
{
  hash &= 7;
  sF32 u = hash<4 ? x : y;
  sF32 v = hash<4 ? y : x;
 
  return ((hash&1) ? -u : u) + ((hash&2) ? -2.0f*v : 2.0f*v);
}

// Perlin smoothstep function
static sF32 SmoothStep(sF32 x)
{
  return x*x*x*(10+x*(6*x-15));
}

// 2D non-bandlimited noise function
static sF32 Noise2(sInt x,sInt y,sInt maskx,sInt masky,sInt seed)
{
  static const sInt M = 0x10000;
  
  sInt X = x >> 16,Y = y >> 16;
  sF32 fx = (x & (M-1)) / 65536.0f;
  sF32 fy = (y & (M-1)) / 65536.0f;
  sF32 u = SmoothStep(fx);
  sF32 v = SmoothStep(fy);
  maskx &= 4095;
  masky &= 4095;

  return
    LerpF(v,
      LerpF(u,
        (P(((X+0)&maskx)+P(((Y+0)&masky))+seed)) / 2047.5f - 1.0f,
        (P(((X+1)&maskx)+P(((Y+0)&masky))+seed)) / 2047.5f - 1.0f),
      LerpF(u,
        (P(((X+0)&maskx)+P(((Y+1)&masky))+seed)) / 2047.5f - 1.0f,
        (P(((X+1)&maskx)+P(((Y+1)&masky))+seed)) / 2047.5f - 1.0f));
}

// 2D Perlin noise function
static sF32 PNoise2(sInt x,sInt y,sInt maskx,sInt masky,sInt seed)
{
  static const sInt M = 0x10000;
  static const sF32 S = sFInvSqrt(5.0f);
  
  sInt X = x >> 16,Y = y >> 16;
  sF32 fx = (x & (M-1)) / 65536.0f;
  sF32 fy = (y & (M-1)) / 65536.0f;
  sF32 u = SmoothStep(fx);
  sF32 v = SmoothStep(fy);
  maskx &= 4095;
  masky &= 4095;

  return S *
    LerpF(v,
      LerpF(u,
        PGradient2((P(((X+0)&maskx)+P(((Y+0)&masky))+seed)),fx     ,fy),
        PGradient2((P(((X+1)&maskx)+P(((Y+0)&masky))+seed)),fx-1.0f,fy)),
      LerpF(u,
        PGradient2((P(((X+0)&maskx)+P(((Y+1)&masky))+seed)),fx     ,fy-1.0f),
        PGradient2((P(((X+1)&maskx)+P(((Y+1)&masky))+seed)),fx-1.0f,fy-1.0f)));
}

static sInt GShuffle(sInt x,sInt y,sInt z)
{
  /*sU32 seed = ((x & 0x3ff) << 20) | ((y & 0x3ff) << 10) | (z & 0x3ff);

  seed ^= seed << 3;
  seed += seed >> 5;
  seed ^= seed << 4;
  seed += seed >> 17;
  seed ^= seed << 25;
  seed += seed >> 6;

  return seed;*/

  return P(P(P(x)+y)+z);
}

// 2D grid noise function (tiling)
static sF32 GNoise2(sInt x,sInt y,sInt maskx,sInt masky,sInt seed)
{
  // input coordinates
  sInt i = x >> 16;
  sInt j = y >> 16;
  sF32 xp = (x & 0xffff) / 65536.0f;
  sF32 yp = (y & 0xffff) / 65536.0f;
  sF32 sum = 0.0f;

  // sum over grid vertices
  for(sInt oy=0;oy<=1;oy++)
  {
    for(sInt ox=0;ox<=1;ox++)
    {
      sF32 xr = xp - ox;
      sF32 yr = yp - oy;

      sF32 t = xr*xr + yr*yr;
      if(t < 1.0f)
      {
        t = 1.0f - t;
        t *= t;
        t *= t;
        sum += t*PGradient2(GShuffle((i+ox)&maskx,(j+oy)&masky,seed),xr,yr);
      }
    }
  }

  return sum;
}

/****************************************************************************/
/***                                                                      ***/
/***   Pixel                                                              ***/
/***                                                                      ***/
/****************************************************************************/

void Pixel::Init(sU8 _r,sU8 _g,sU8 _b,sU8 _a)
{
  r = (_r << 8) | _r;
  g = (_g << 8) | _g;
  b = (_b << 8) | _b;
  a = (_a << 8) | _a;
}

void Pixel::Init(sU32 rgba)
{
  sU8 rv,gv,bv,av;

  rv = (rgba >> 16) & 0xff;
  gv = (rgba >>  8) & 0xff;
  bv = (rgba >>  0) & 0xff;
  av = (rgba >> 24) & 0xff;

  a = (av << 8) | av;
  r = MulIntens((rv << 8) | rv,a);
  g = MulIntens((gv << 8) | gv,a);
  b = MulIntens((bv << 8) | bv,a);
}

void Pixel::Lerp(sInt t,const Pixel &x,const Pixel &y)
{
  r = ::Lerp(t,x.r,y.r);
  g = ::Lerp(t,x.g,y.g);
  b = ::Lerp(t,x.b,y.b);
  a = ::Lerp(t,x.a,y.a);
}

void Pixel::CompositeAdd(const Pixel &x)
{
  r = sClamp<sInt>(r + x.r,0,65535);
  g = sClamp<sInt>(g + x.g,0,65535);
  b = sClamp<sInt>(b + x.b,0,65535);
  a = sClamp<sInt>(a + x.a,0,65535);
}

void Pixel::CompositeMulC(const Pixel &x)
{
  r = MulIntens(r,x.r);
  g = MulIntens(g,x.g);
  b = MulIntens(b,x.b);
  a = MulIntens(a,x.a);
}

void Pixel::CompositeROver(const Pixel &x)
{
  sInt transIn = 65535 - x.a;
  r = MulIntens(transIn,r) + x.r;
  g = MulIntens(transIn,g) + x.g;
  b = MulIntens(transIn,b) + x.b;
  a = MulIntens(transIn,a) + x.a;
}

void Pixel::CompositeScreen(const Pixel &x)
{
  r += MulIntens(x.r,65535-r);
  g += MulIntens(x.g,65535-g);
  b += MulIntens(x.b,65535-b);
  a += MulIntens(x.a,65535-a);
}

/****************************************************************************/
/***                                                                      ***/
/***   GenTexture                                                         ***/
/***                                                                      ***/
/****************************************************************************/

GenTexture::GenTexture()
{
  Data = 0;
  XRes = 0;
  YRes = 0;

  UpdateSize();
}

GenTexture::GenTexture(sInt xres,sInt yres)
{
  Data = 0;
  XRes = 0;
  YRes = 0;

  Init(xres,yres);
}

GenTexture::GenTexture(const GenTexture &x)
{
  XRes = x.XRes;
  YRes = x.YRes;
  UpdateSize();

  Data = new Pixel[NPixels];
  sCopyMem(Data,x.Data,NPixels * sizeof(Pixel));
}

GenTexture::~GenTexture()
{
  delete[] Data;
}

void GenTexture::Init(sInt xres,sInt yres)
{
  if(XRes != xres || YRes != yres)
  {
    delete[] Data;

    sVERIFY(IsPowerOf2(xres));
    sVERIFY(IsPowerOf2(yres));

    XRes = xres;
    YRes = yres;
    UpdateSize();

    Data = new Pixel[NPixels];
  }
}

void GenTexture::UpdateSize()
{
  NPixels = XRes * YRes;
  ShiftX = FloorLog2(XRes);
  ShiftY = FloorLog2(YRes);

  MinX = 1 << (24 - 1 - ShiftX);
  MinY = 1 << (24 - 1 - ShiftY);
}

void GenTexture::Swap(GenTexture &x)
{
  sSwap(Data,x.Data);
  sSwap(XRes,x.XRes);
  sSwap(YRes,x.YRes);
  sSwap(NPixels,x.NPixels);
  sSwap(ShiftX,x.ShiftX);
  sSwap(ShiftY,x.ShiftY);
  sSwap(MinX,x.MinX);
  sSwap(MinY,x.MinY);
}

GenTexture &GenTexture::operator =(const GenTexture &x)
{
  GenTexture t=x;
  
  Swap(t);
  return *this;
}

sBool GenTexture::SizeMatchesWith(const GenTexture &x) const
{
  return XRes == x.XRes && YRes == x.YRes;
}

// ---- Sampling helpers
void GenTexture::SampleNearest(Pixel &result,sInt x,sInt y,sInt wrapMode) const
{
  if(wrapMode & 1)  x = sClamp(x,MinX,0x1000000-MinX);
  if(wrapMode & 2)  y = sClamp(y,MinY,0x1000000-MinY);

  x &= 0xffffff;
  y &= 0xffffff;

  sInt ix = x >> (24 - ShiftX);
  sInt iy = y >> (24 - ShiftY);

  result = Data[(iy << ShiftX) + ix];
}

void GenTexture::SampleBilinear(Pixel &result,sInt x,sInt y,sInt wrapMode) const
{
  if(wrapMode & 1)  x = sClamp(x,MinX,0x1000000-MinX);
  if(wrapMode & 2)  y = sClamp(y,MinY,0x1000000-MinY);

  x = (x - MinX) & 0xffffff;
  y = (y - MinY) & 0xffffff;

  sInt x0 = x >> (24 - ShiftX);
  sInt x1 = (x0 + 1) & (XRes - 1);
  sInt y0 = y >> (24 - ShiftY);
  sInt y1 = (y0 + 1) & (YRes - 1);
  sInt fx = sU32(x << (ShiftX + 8)) >> 16;
  sInt fy = sU32(y << (ShiftY + 8)) >> 16;

  Pixel t0,t1;
  t0.Lerp(fx,Data[(y0 << ShiftX) + x0],Data[(y0 << ShiftX) + x1]);
  t1.Lerp(fx,Data[(y1 << ShiftX) + x0],Data[(y1 << ShiftX) + x1]);
  result.Lerp(fy,t0,t1);
}

void GenTexture::SampleFiltered(Pixel &result,sInt x,sInt y,sInt filterMode) const
{
  if(filterMode & FilterBilinear)
    SampleBilinear(result,x,y,filterMode);
  else
    SampleNearest(result,x,y,filterMode);
}

void GenTexture::SampleGradient(Pixel &result,sInt x) const
{
  x = sClamp(x,0,1<<24);
  x -= x >> ShiftX; // x=(1<<24) -> Take rightmost pixel

  sInt x0 = x >> (24 - ShiftX);
  sInt x1 = (x0 + 1) & (XRes - 1);
  sInt fx = sU32(x << (ShiftX + 8)) >> 16;

  result.Lerp(fx,Data[x0],Data[x1]);
}

// ---- The operators themselves

void GenTexture::Noise(const GenTexture &grad,sInt freqX,sInt freqY,sInt oct,sF32 fadeoff,sInt seed,sInt mode)
{
  sVERIFY(oct > 0);

  seed = P(seed);

  sInt offset;
  sF32 scaling;
  
  if(mode & NoiseNormalize)
    scaling = (fadeoff - 1.0f) / (sFPow(fadeoff,oct) - 1.0f);
  else
    scaling = sMin(1.0f,1.0f / fadeoff);

  if(mode & NoiseAbs) // absolute mode
  {
    offset = 0;
    scaling *= (1 << 24);
  }
  else
  {
    offset = 1 << 23;
    scaling *= (1 << 23);
  }

  sInt offsX = (1 << (16 - ShiftX + freqX)) >> 1;
  sInt offsY = (1 << (16 - ShiftY + freqY)) >> 1;

  Pixel *out = Data;
  for(sInt y=0;y<YRes;y++)
  {
    for(sInt x=0;x<XRes;x++)
    {
      sInt n = offset;
      sF32 s = scaling;

      sInt px = (x << (16 - ShiftX + freqX)) + offsX;
      sInt py = (y << (16 - ShiftY + freqY)) + offsY;
      sInt mx = (1 << freqX) - 1;
      sInt my = (1 << freqY) - 1;

      for(sInt i=0;i<oct;i++)
      {
        sF32 nv = (mode & NoiseBandlimit) ? Noise2(px,py,mx,my,seed) : GNoise2(px,py,mx,my,seed);
        if(mode & NoiseAbs)
          nv = sFAbs(nv);

        n += nv * s;
        s *= fadeoff;

        px += px;
        py += py;
        mx += mx + 1;
        my += my + 1;
      }

      grad.SampleGradient(*out,n);
      out++;
    }
  }
}

void GenTexture::GlowRect(const GenTexture &bgTex,const GenTexture &grad,sF32 orgx,sF32 orgy,sF32 ux,sF32 uy,sF32 vx,sF32 vy,sF32 rectu,sF32 rectv)
{
  sVERIFY(SizeMatchesWith(bgTex));

  // copy background over (if we're not the background texture already)
  if(this != &bgTex)
    *this = bgTex;

  // calculate bounding rect
  sInt minX = sMax<sInt>(0,floor((orgx - sFAbs(ux) - sFAbs(vx)) * XRes));
  sInt minY = sMax<sInt>(0,floor((orgy - sFAbs(uy) - sFAbs(vy)) * YRes));
  sInt maxX = sMin<sInt>(XRes-1,ceil((orgx + sFAbs(ux) + sFAbs(vx)) * XRes));
  sInt maxY = sMin<sInt>(YRes-1,ceil((orgy + sFAbs(uy) + sFAbs(vy)) * YRes));

  // solve for u0,v0 and deltas (cramer's rule)
  sF32 detM = ux*vy - uy*vx;
  if(fabs(detM) * XRes * YRes < 0.25f) // smaller than a pixel? skip it.
    return;

  sF32 invM = (1 << 16) / detM;
  sF32 rmx = (minX + 0.5f) / XRes - orgx;
  sF32 rmy = (minY + 0.5f) / YRes - orgy;
  sInt u0 = (rmx*vy - rmy*vx) * invM;
  sInt v0 = (ux*rmy - uy*rmx) * invM;
  sInt dudx = vy * invM / XRes;
  sInt dvdx = -uy * invM / XRes;
  sInt dudy = -vx * invM / YRes;
  sInt dvdy = ux * invM / YRes;
  sInt ruf = sMin<sInt>(rectu * 65536.0f,65535);
  sInt rvf = sMin<sInt>(rectv * 65536.0f,65535);
  sF32 gus = 1.0f / (65536.0f - ruf);
  sF32 gvs = 1.0f / (65536.0f - rvf);

  for(sInt y=minY;y<=maxY;y++)
  {
    Pixel *out = &Data[y*XRes + minX];
    sInt u = u0;
    sInt v = v0;

    for(sInt x=minX;x<=maxX;x++)
    {
      if(u>-65536 && u<65536 && v>-65536 && v<65536)
      {
        Pixel col;

        sInt du = sMax(sAbs(u) - ruf,0);
        sInt dv = sMax(sAbs(v) - rvf,0);

        if(!du && !dv)
        {
          grad.SampleGradient(col,0);
          out->CompositeROver(col);
        }
        else
        {
          sF32 dus = du * gus;
          sF32 dvs = dv * gvs;
          sF32 dist = dus*dus + dvs*dvs;

          if(dist < 1.0f)
          {
            grad.SampleGradient(col,(1 << 24) * sFSqrt(dist));
            out->CompositeROver(col);
          }
        }
      }

      u += dudx;
      v += dvdx;
      out++;
    }

    u0 += dudy;
    v0 += dvdy;
  }
}

struct CellPoint
{
  sInt x;
  sInt y;
  sInt distY;
  sInt node;
};

void GenTexture::Cells(const GenTexture &grad,const CellCenter *centers,sInt nCenters,sF32 amp,sInt mode)
{
  sVERIFY(((mode & 1) == 0) ? nCenters >= 1 : nCenters >= 2);

  Pixel *out = Data;
  CellPoint *points = NULL;

  points = new CellPoint[nCenters];

  // convert cell center coordinates to fixed point
  static const sInt scaleF = 14; // should be <=14 for 32-bit ints.
  static const sInt scale = 1<<scaleF;

  for(sInt i=0;i<nCenters;i++)
  {
    points[i].x = sInt(centers[i].x * scale + 0.5f) & (scale - 1);
    points[i].y = sInt(centers[i].y * scale + 0.5f) & (scale - 1);
    points[i].distY = -1;
    points[i].node = i;
  }

  sInt stepX = 1 << (scaleF - ShiftX);
  sInt stepY = 1 << (scaleF - ShiftY);
  sInt yc = stepY >> 1;
  
  amp = amp * (1 << 24);

  for(sInt y=0;y<YRes;y++)
  {
    sInt xc = stepX >> 1;

    // calculate new y distances
    for(sInt i=0;i<nCenters;i++)
    {
      sInt dy = (yc - points[i].y) & (scale - 1);
      points[i].distY = sSquare(sMin(dy,scale-dy));
    }

    // (insertion) sort by y-distance
    for(sInt i=1;i<nCenters;i++)
    {
      CellPoint v = points[i];
      sInt j = i;

      while(j && points[j-1].distY > v.distY)
      {
        points[j] = points[j-1];
        j--;
      }

      points[j] = v;
    }

    sInt best,best2;
    sInt besti,best2i;

    best = best2 = sSquare(scale);
    besti = best2i = -1;

    for(sInt x=0;x<XRes;x++)
    {
      sInt t,dx;

      // update "best point" stats
      if(besti != -1 && best2i != -1)
      {
        dx = (xc - points[besti].x) & (scale - 1);
        best = sSquare(sMin(dx,scale-dx)) + points[besti].distY;

        dx = (xc - points[best2i].x) & (scale - 1);
        best2 = sSquare(sMin(dx,scale-dx)) + points[best2i].distY;
        if(best2 < best)
        {
          sSwap(best,best2);
          sSwap(besti,best2i);
        }
      }

      // search for better points
      for(sInt i=0;i<nCenters && best2 > points[i].distY;i++)
      {
        sInt dx = (xc - points[i].x) & (scale - 1);
        dx = sSquare(sMin(dx,scale-dx));

        sInt dist = dx + points[i].distY;
        if(dist < best)
        {
          best2 = best;
          best2i = besti;
          best = dist;
          besti = i;
        }
        else if(dist > best && dist < best2)
        {
          best2 = dist;
          best2i = i;
        }
      }

      // color the pixel accordingly
      sF32 d0 = sFSqrt(best) / scale;

      if((mode & 1) == CellInner) // inner
        t = sClamp<sInt>(d0*amp,0,1<<24);
      else // outer
      {
        sF32 d1 = sFSqrt(best2) / scale;

        if(d0+d1 > 0.0f)
          t = sClamp<sInt>(d0 / (d1 + d0) * 2 * amp,0,1<<24);
        else
          t = 0;
      }

      grad.SampleGradient(*out,t);
      out[0].CompositeMulC(centers[points[besti].node].color);

      out++;
      xc += stepX;
    }

    yc += stepY;
  }

  delete[] points;
}

void GenTexture::ColorMatrixTransform(const GenTexture &x,const Matrix44 &matrix,sBool clampPremult)
{
  sInt m[4][4];

  sVERIFY(SizeMatchesWith(x));

  for(sInt i=0;i<4;i++)
  {
    for(sInt j=0;j<4;j++)
    {
      sVERIFY(matrix[i][j] >= -127.0f && matrix[i][j] <= 127.0f);
      m[i][j] = matrix[i][j] * 65536.0f;
    }
  }

  for(sInt i=0;i<NPixels;i++)
  {
    Pixel &out = Data[i];
    const Pixel &in = x.Data[i];

    sInt r = MulShift16(m[0][0],in.r) + MulShift16(m[0][1],in.g) + MulShift16(m[0][2],in.b) + MulShift16(m[0][3],in.a);
    sInt g = MulShift16(m[1][0],in.r) + MulShift16(m[1][1],in.g) + MulShift16(m[1][2],in.b) + MulShift16(m[1][3],in.a);
    sInt b = MulShift16(m[2][0],in.r) + MulShift16(m[2][1],in.g) + MulShift16(m[2][2],in.b) + MulShift16(m[2][3],in.a);
    sInt a = MulShift16(m[3][0],in.r) + MulShift16(m[3][1],in.g) + MulShift16(m[3][2],in.b) + MulShift16(m[3][3],in.a);

    if(clampPremult)
    {
      out.a = sClamp<sInt>(a,0,65535);
      out.r = sClamp<sInt>(r,0,out.a);
      out.g = sClamp<sInt>(g,0,out.a);
      out.b = sClamp<sInt>(b,0,out.a);
    }
    else
    {
      out.r = sClamp<sInt>(r,0,65535);
      out.g = sClamp<sInt>(g,0,65535);
      out.b = sClamp<sInt>(b,0,65535);
      out.a = sClamp<sInt>(a,0,65535);
    }
  }
}

void GenTexture::CoordMatrixTransform(const GenTexture &in,const Matrix44 &matrix,sInt mode)
{
  sInt scaleX = 1 << (24 - ShiftX);
  sInt scaleY = 1 << (24 - ShiftY);

  sInt dudx = matrix[0][0] * scaleX;
  sInt dudy = matrix[0][1] * scaleY;
  sInt dvdx = matrix[1][0] * scaleX;
  sInt dvdy = matrix[1][1] * scaleY;

  sInt u0 = matrix[0][3] * (1 << 24) + ((dudx + dudy) >> 1);
  sInt v0 = matrix[1][3] * (1 << 24) + ((dvdx + dvdy) >> 1);
  Pixel *out = Data;

  for(sInt y=0;y<YRes;y++)
  {
    sInt u = u0;
    sInt v = v0;

    for(sInt x=0;x<XRes;x++)
    {
      in.SampleFiltered(*out,u,v,mode);

      u += dudx;
      v += dvdx;
      out++;
    }

    u0 += dudy;
    v0 += dvdy;
  }
}

void GenTexture::ColorRemap(const GenTexture &inTex,const GenTexture &mapR,const GenTexture &mapG,const GenTexture &mapB)
{
  sVERIFY(SizeMatchesWith(inTex));

  for(sInt i=0;i<NPixels;i++)
  {
    const Pixel &in = inTex.Data[i];
    Pixel &out = Data[i];

    if(in.a == 65535) // alpha==1, everything easy.
    {
      Pixel colR,colG,colB;

      mapR.SampleGradient(colR,(in.r << 8) + ((in.r + 128) >> 8));
      mapG.SampleGradient(colG,(in.g << 8) + ((in.g + 128) >> 8));
      mapB.SampleGradient(colB,(in.b << 8) + ((in.b + 128) >> 8));

      out.r = sMin(colR.r+colG.r+colB.r,65535);
      out.g = sMin(colR.g+colG.g+colB.g,65535);
      out.b = sMin(colR.b+colG.b+colB.b,65535);
      out.a = in.a;
    }
    else if(in.a) // alpha!=0
    {
      Pixel colR,colG,colB;
      sU32 invA = (65535U << 16) / in.a;

      mapR.SampleGradient(colR,UMulShift8(sMin(in.r,in.a),invA));
      mapG.SampleGradient(colG,UMulShift8(sMin(in.g,in.a),invA));
      mapB.SampleGradient(colB,UMulShift8(sMin(in.b,in.a),invA));

      out.r = MulIntens(sMin(colR.r+colG.r+colB.r,65535),in.a);
      out.g = MulIntens(sMin(colR.g+colG.g+colB.g,65535),in.a);
      out.b = MulIntens(sMin(colR.b+colG.b+colB.b,65535),in.a);
      out.a = in.a;
    }
    else // alpha==0
      out = in;
  }
}

void GenTexture::CoordRemap(const GenTexture &in,const GenTexture &remapTex,sF32 strengthU,sF32 strengthV,sInt mode)
{
  sVERIFY(SizeMatchesWith(remapTex));

  const Pixel *remap = remapTex.Data;
  Pixel *out = Data;

  sInt u0 = MinX;
  sInt v0 = MinY;
  sInt scaleU = (1 << 24) * strengthU;
  sInt scaleV = (1 << 24) * strengthV;
  sInt stepU = 1 << (24 - ShiftX);
  sInt stepV = 1 << (24 - ShiftY);

  for(sInt y=0;y<YRes;y++)
  {
    sInt u = u0;
    sInt v = v0;

    for(sInt x=0;x<XRes;x++)
    {
      sInt dispU = u + MulShift16(scaleU,(remap->r - 32768) * 2);
      sInt dispV = v + MulShift16(scaleV,(remap->g - 32768) * 2);
      in.SampleFiltered(*out,dispU,dispV,mode);

      u += stepU;
      remap++;
      out++;
    }

    v0 += stepV;
  }
}

void GenTexture::Derive(const GenTexture &in,DeriveOp op,sF32 strength)
{
  sVERIFY(SizeMatchesWith(in));

  Pixel *out = Data;

  for(sInt y=0;y<YRes;y++)
  {
    for(sInt x=0;x<XRes;x++)
    {
      sInt dx2 = in.Data[y*XRes + ((x+1) & (XRes-1))].r - in.Data[y*XRes + ((x-1) & (XRes-1))].r;
      sInt dy2 = in.Data[x + ((y+1) & (YRes - 1)) * XRes].r - in.Data[x + ((y-1) & (YRes - 1)) * XRes].r;
      sF32 dx = dx2 * strength / (2 * 65535.0f);
      sF32 dy = dy2 * strength / (2 * 65535.0f);

      switch(op)
      {
      case DeriveGradient:
        out->r = sClamp<sInt>(dx*32768.0f + 32768.0f,0,65535);
        out->g = sClamp<sInt>(dy*32768.0f + 32768.0f,0,65535);
        out->b = 0;
        out->a = 65535;
        break;

      case DeriveNormals:
        {
          // (1 0 dx)^T x (0 1 dy)^T = (-dx -dy 1)
          sF32 scale = 32768.0f * sFInvSqrt(1.0f + dx*dx + dy*dy);

          out->r = sClamp<sInt>(-dx*scale + 32768.0f,0,65535);
          out->g = sClamp<sInt>(-dy*scale + 32768.0f,0,65535);
          out->b = sClamp<sInt>(    scale + 32768.0f,0,65535);
          out->a = 65535;
        }
        break;
      }

      out++;
    }
  }
}

// Wrap computation on pixel coordinates
static sInt WrapCoord(sInt x,sInt width,sInt mode)
{
  if(mode == 0) // wrap
    return x & (width - 1);
  else
    return sClamp(x,0,width-1);
}

// Size is half of edge length in pixels, 26.6 fixed point
static void Blur1DBuffer(Pixel *dst,const Pixel *src,sInt width,sInt sizeFixed,sInt wrapMode)
{
  sVERIFY(sizeFixed > 32); // kernel should be wider than one pixel
  sInt frac = (sizeFixed - 32) & 63;
  sInt offset = (sizeFixed + 32) >> 6;

  sVERIFY(((offset - 1) * 64 + frac + 32) == sizeFixed);
  sU32 denom = sizeFixed * 2;
  sU32 bias = denom / 2;

  // initialize accumulators
  sU32 accu[4];
  if(wrapMode == 0) // wrap around
  {
    // leftmost and rightmost pixels (the partially covered ones)
    sInt xl = WrapCoord(-offset,width,wrapMode);
    sInt xr = WrapCoord(offset,width,wrapMode);
    accu[0] = frac * (src[xl].r + src[xr].r) + bias;
    accu[1] = frac * (src[xl].g + src[xr].g) + bias;
    accu[2] = frac * (src[xl].b + src[xr].b) + bias;
    accu[3] = frac * (src[xl].a + src[xr].a) + bias;

    // inner part of filter kernel
    for(sInt x=-offset+1;x<=offset-1;x++)
    {
      sInt xc = WrapCoord(x,width,wrapMode);
      
      accu[0] += src[xc].r << 6;
      accu[1] += src[xc].g << 6;
      accu[2] += src[xc].b << 6;
      accu[3] += src[xc].a << 6;
    }
  }
  else // clamp on edge
  {
    // on the left edge, the first pixel is repeated over and over
    accu[0] = src[0].r * (sizeFixed + 32) + bias;
    accu[1] = src[0].g * (sizeFixed + 32) + bias;
    accu[2] = src[0].b * (sizeFixed + 32) + bias;
    accu[3] = src[0].a * (sizeFixed + 32) + bias;

    // rightmost pixel
    sInt xr = WrapCoord(offset,width,wrapMode);
    accu[0] += frac * src[xr].r;
    accu[1] += frac * src[xr].g;
    accu[2] += frac * src[xr].b;
    accu[3] += frac * src[xr].a;

    // inner part of filter kernel (the right half)
    for(sInt x=1;x<=offset-1;x++)
    {
      sInt xc = WrapCoord(x,width,wrapMode);

      accu[0] += src[xc].r << 6;
      accu[1] += src[xc].g << 6;
      accu[2] += src[xc].b << 6;
      accu[3] += src[xc].a << 6;
    }
  }

  // generate output pixels
  for(sInt x=0;x<width;x++)
  {
    // write out state of accumulator
    dst[x].r = accu[0] / denom;
    dst[x].g = accu[1] / denom;
    dst[x].b = accu[2] / denom;
    dst[x].a = accu[3] / denom;

    // update accumulator
    sInt xl0 = WrapCoord(x-offset+0,width,wrapMode);
    sInt xl1 = WrapCoord(x-offset+1,width,wrapMode);
    sInt xr0 = WrapCoord(x+offset+0,width,wrapMode);
    sInt xr1 = WrapCoord(x+offset+1,width,wrapMode);

    accu[0] += 64 * (src[xr0].r - src[xl1].r) + frac * (src[xr1].r - src[xr0].r - src[xl0].r + src[xl1].r);
    accu[1] += 64 * (src[xr0].g - src[xl1].g) + frac * (src[xr1].g - src[xr0].g - src[xl0].g + src[xl1].g);
    accu[2] += 64 * (src[xr0].b - src[xl1].b) + frac * (src[xr1].b - src[xr0].b - src[xl0].b + src[xl1].b);
    accu[3] += 64 * (src[xr0].a - src[xl1].a) + frac * (src[xr1].a - src[xr0].a - src[xl0].a + src[xl1].a);
  }
}

void GenTexture::Blur(const GenTexture &inImg,sF32 sizex,sF32 sizey,sInt order,sInt wrapMode)
{
  sVERIFY(SizeMatchesWith(inImg));

  sInt sizePixX = sClamp(sizex,0.0f,1.0f) * 64 * inImg.XRes / 2;
  sInt sizePixY = sClamp(sizey,0.0f,1.0f) * 64 * inImg.YRes / 2;

  // no blur at all? just copy!
  if(order < 1 || (sizePixX <= 32 && sizePixY <= 32))
    *this = inImg;
  else
  {
    // allocate pixel buffers
    sInt bufSize = sMax(XRes,YRes);
    Pixel *buf1 = new Pixel[bufSize];
    Pixel *buf2 = new Pixel[bufSize];
    const GenTexture *in = &inImg;

    // horizontal blur
    if(sizePixX > 32)
    {
      // go through image row by row
      for(sInt y=0;y<YRes;y++)
      {
        // copy pixels into buffer 1
        sCopyMem(buf1,&in->Data[y*XRes],XRes * sizeof(Pixel));

        // blur order times, ping-ponging between buffers
        for(sInt i=0;i<order;i++)
        {
          Blur1DBuffer(buf2,buf1,XRes,sizePixX,(wrapMode & ClampU) ? 1 : 0);
          sSwap(buf1,buf2);
        }

        // copy pixels back
        sCopyMem(&Data[y*XRes],buf1,XRes * sizeof(Pixel));
      }

      in = this;
    }

    // vertical blur
    if(sizePixY > 32)
    {
      // go through image column by column
      for(sInt x=0;x<XRes;x++)
      {
        // copy pixels into buffer 1
        const Pixel *src = &in->Data[x];
        Pixel *dst = buf1;

        for(sInt y=0;y<YRes;y++)
        {
          *dst++ = *src;
          src += XRes;
        }

        // blur order times, ping-ponging between buffers
        for(sInt i=0;i<order;i++)
        {
          Blur1DBuffer(buf2,buf1,YRes,sizePixY,(wrapMode & ClampV) ? 1 : 0);
          sSwap(buf1,buf2);
        }

        // copy pixels back
        src = buf1;
        dst = &Data[x];

        for(sInt y=0;y<YRes;y++)
        {
          *dst = *src++;
          dst += XRes;
        }
      }
    }

    // clean up
    delete[] buf1;
    delete[] buf2;
  }
}

void GenTexture::Ternary(const GenTexture &in1Tex,const GenTexture &in2Tex,const GenTexture &in3Tex,TernaryOp op)
{
  sVERIFY(SizeMatchesWith(in1Tex) && SizeMatchesWith(in2Tex) && SizeMatchesWith(in3Tex));

  for(sInt i=0;i<NPixels;i++)
  {
    Pixel &out = Data[i];
    const Pixel &in1 = in1Tex.Data[i];
    const Pixel &in2 = in2Tex.Data[i];
    const Pixel &in3 = in3Tex.Data[i];

    switch(op)
    {
    case TernaryLerp:
      out.r = MulIntens(65535-in3.r,in1.r) + MulIntens(in3.r,in2.r);
      out.g = MulIntens(65535-in3.r,in1.g) + MulIntens(in3.r,in2.g);
      out.b = MulIntens(65535-in3.r,in1.b) + MulIntens(in3.r,in2.b);
      out.a = MulIntens(65535-in3.r,in1.a) + MulIntens(in3.r,in2.a);
      break;

    case TernarySelect:
      out = (in3.r >= 32768) ? in2 : in1;
      break;
    }
  }
}

void GenTexture::Paste(const GenTexture &bgTex,const GenTexture &inTex,sF32 orgx,sF32 orgy,sF32 ux,sF32 uy,sF32 vx,sF32 vy,CombineOp op,sInt mode)
{
  sVERIFY(SizeMatchesWith(bgTex));

  // copy background over (if this image is not the background already)
  if(this != &bgTex)
    *this = bgTex;

  // calculate bounding rect
  sInt minX = sMax<sInt>(0,floor((orgx + sMin(ux,0.0f) + sMin(vx,0.0f)) * XRes));
  sInt minY = sMax<sInt>(0,floor((orgy + sMin(uy,0.0f) + sMin(vy,0.0f)) * YRes));
  sInt maxX = sMin<sInt>(XRes-1,ceil((orgx + sMax(ux,0.0f) + sMax(vx,0.0f)) * XRes));
  sInt maxY = sMin<sInt>(YRes-1,ceil((orgy + sMax(uy,0.0f) + sMax(vy,0.0f)) * YRes));

  // solve for u0,v0 and deltas (Cramer's rule)
  sF32 detM = ux*vy - uy*vx;
  if(fabs(detM) * XRes * YRes < 0.25f) // smaller than a pixel? skip it.
    return;

  sF32 invM = (1 << 24) / detM;
  sF32 rmx = (minX + 0.5f) / XRes - orgx;
  sF32 rmy = (minY + 0.5f) / YRes - orgy;
  sInt u0 = (rmx*vy - rmy*vx) * invM;
  sInt v0 = (ux*rmy - uy*rmx) * invM;
  sInt dudx = vy * invM / XRes;
  sInt dvdx = -uy * invM / XRes;
  sInt dudy = -vx * invM / YRes;
  sInt dvdy = ux * invM / YRes;

  for(sInt y=minY;y<=maxY;y++)
  {
    Pixel *out = &Data[y*XRes + minX];
    sInt u = u0;
    sInt v = v0;

    for(sInt x=minX;x<=maxX;x++)
    {
      if(u >= 0 && u < 0x1000000 && v >= 0 && v < 0x1000000)
      {
        Pixel in;
        sInt transIn,transOut;

        inTex.SampleFiltered(in,u,v,ClampU|ClampV|((mode & 1) ? FilterBilinear : FilterNearest));

        switch(op)
        {
        case CombineAdd:
          out->r = sMin(out->r + in.r,65535);
          out->g = sMin(out->g + in.g,65535);
          out->b = sMin(out->b + in.b,65535);
          out->a = sMin(out->a + in.a,65535);
          break;

        case CombineSub:
          out->r = sMax<sInt>(out->r - in.r,0);
          out->g = sMax<sInt>(out->g - in.g,0);
          out->b = sMax<sInt>(out->b - in.b,0);
          out->a = sMax<sInt>(out->a - in.a,0);
          break;

        case CombineMulC:
          out->r = MulIntens(out->r,in.r);
          out->g = MulIntens(out->g,in.g);
          out->b = MulIntens(out->b,in.b);
          out->a = MulIntens(out->a,in.a);
          break;

        case CombineMin:
          out->r = sMin(out->r,in.r);
          out->g = sMin(out->g,in.g);
          out->b = sMin(out->b,in.b);
          out->a = sMin(out->a,in.a);
          break;

        case CombineMax:
          out->r = sMax(out->r,in.r);
          out->g = sMax(out->g,in.g);
          out->b = sMax(out->b,in.b);
          out->a = sMax(out->a,in.a);
          break;

        case CombineSetAlpha:
          out->a = in.r;
          break;

        case CombinePreAlpha:
          out->r = MulIntens(out->r,in.r);
          out->g = MulIntens(out->g,in.r);
          out->b = MulIntens(out->b,in.r);
          out->a = in.g;
          break;

        case CombineOver:
          transIn = 65535 - in.a;

          out->r = MulIntens(transIn,out->r) + in.r;
          out->g = MulIntens(transIn,out->g) + in.g;
          out->b = MulIntens(transIn,out->b) + in.b;
          out->a += MulIntens(in.a,65535-out->a);
          break;

        case CombineMultiply:
          transIn = 65535 - in.a;
          transOut = 65535 - out->a;

          out->r = MulIntens(transIn,out->r) + MulIntens(transOut,in.r) + MulIntens(in.r,out->r);
          out->g = MulIntens(transIn,out->g) + MulIntens(transOut,in.g) + MulIntens(in.g,out->g);
          out->b = MulIntens(transIn,out->b) + MulIntens(transOut,in.b) + MulIntens(in.b,out->b);
          out->a += MulIntens(in.a,transOut);
          break;

        case CombineScreen:
          out->r += MulIntens(in.r,65535-out->r);
          out->g += MulIntens(in.g,65535-out->g);
          out->b += MulIntens(in.b,65535-out->b);
          out->a += MulIntens(in.a,65535-out->a);
          break;

        case CombineDarken:
          out->r += in.r - sMax(MulIntens(in.r,out->a),MulIntens(out->r,in.a));
          out->g += in.g - sMax(MulIntens(in.g,out->a),MulIntens(out->g,in.a));
          out->b += in.b - sMax(MulIntens(in.b,out->a),MulIntens(out->b,in.a));
          out->a += MulIntens(in.a,65535-out->a);
          break;

        case CombineLighten:
          out->r += in.r - sMin(MulIntens(in.r,out->a),MulIntens(out->r,in.a));
          out->g += in.g - sMin(MulIntens(in.g,out->a),MulIntens(out->g,in.a));
          out->b += in.b - sMin(MulIntens(in.b,out->a),MulIntens(out->b,in.a));
          out->a += MulIntens(in.a,65535-out->a);
          break;
        }
      }

      u += dudx;
      v += dvdx;
      out++;
    }

    u0 += dudy;
    v0 += dvdy;
  }
}

void GenTexture::Bump(const GenTexture &surface,const GenTexture &normals,const GenTexture *specular,const GenTexture *falloffMap,sF32 px,sF32 py,sF32 pz,sF32 dx,sF32 dy,sF32 dz,const Pixel &ambient,const Pixel &diffuse,sBool directional)
{
  sVERIFY(SizeMatchesWith(surface) && SizeMatchesWith(normals));

  sF32 L[3],H[3]; // light/halfway vector
  sF32 invX,invY;

  sF32 scale = sFInvSqrt(dx*dx + dy*dy + dz*dz);
  dx *= scale;
  dy *= scale;
  dz *= scale;

  if(directional)
  {
    L[0] = -dx;
    L[1] = -dy;
    L[2] = -dz;

    scale = sFInvSqrt(2.0f + 2.0f * L[2]); // 1/sqrt((L + <0,0,1>)^2)
    H[0] = L[0] * scale;
    H[1] = L[1] * scale;
    H[2] = (L[2] + 1.0f) * scale;
  }

  invX = 1.0f / XRes;
  invY = 1.0f / YRes;
  Pixel *out = Data;
  const Pixel *surf = surface.Data;
  const Pixel *normal = normals.Data;

  for(sInt y=0;y<YRes;y++)
  {
    for(sInt x=0;x<XRes;x++)
    {
      // determine vectors to light
      if(!directional)
      {
        L[0] = px - (x + 0.5f) * invX;
        L[1] = py - (y + 0.5f) * invY;
        L[2] = pz;

        sF32 scale = sFInvSqrt(L[0]*L[0] + L[1]*L[1] + L[2]*L[2]);
        L[0] *= scale;
        L[1] *= scale;
        L[2] *= scale;

        // determine halfway vector
        if(specular)
        {
          sF32 scale = sFInvSqrt(2.0f + 2.0f * L[2]); // 1/sqrt((L + <0,0,1>)^2)
          H[0] = L[0] * scale;
          H[1] = L[1] * scale;
          H[2] = (L[2] + 1.0f) * scale;
        }
      }

      // fetch normal
      sF32 N[3];
      N[0] = (normal->r - 0x8000) / 32768.0f;
      N[1] = (normal->g - 0x8000) / 32768.0f;
      N[2] = (normal->b - 0x8000) / 32768.0f;

      // get falloff term if specified
      Pixel falloff;
      if(falloffMap)
      {
        sF32 spotTerm = sMax<sF32>(dx*L[0] + dy*L[1] + dz*L[2],0.0f);
        falloffMap->SampleGradient(falloff,spotTerm * (1<<24));
      }

      // lighting calculation
      sF32 NdotL = sMax<sF32>(N[0]*L[0] + N[1]*L[1] + N[2]*L[2],0.0f);
      Pixel ambDiffuse;

      ambDiffuse.r = NdotL * diffuse.r;
      ambDiffuse.g = NdotL * diffuse.g;
      ambDiffuse.b = NdotL * diffuse.b;
      ambDiffuse.a = NdotL * diffuse.a;
      if(falloffMap)
        ambDiffuse.CompositeMulC(falloff);

      ambDiffuse.CompositeAdd(ambient);
      out->r = MulIntens(surf->r,ambDiffuse.r);
      out->g = MulIntens(surf->g,ambDiffuse.g);
      out->b = MulIntens(surf->b,ambDiffuse.b);
      out->a = MulIntens(surf->a,ambDiffuse.a);

      if(specular)
      {
        Pixel addTerm;
        sF32 NdotH = sMax<sF32>(N[0]*H[0] + N[1]*H[1] + N[2]*H[2],0.0f);
        specular->SampleGradient(addTerm,NdotH * (1<<24));
        if(falloffMap)
          addTerm.CompositeMulC(falloff);

        out->r = sClamp<sInt>(out->r+addTerm.r,0,out->a);
        out->g = sClamp<sInt>(out->g+addTerm.g,0,out->a);
        out->b = sClamp<sInt>(out->b+addTerm.b,0,out->a);
      }

      out++;
      surf++;
      normal++;
    }
  }
}

void GenTexture::LinearCombine(const Pixel &color,sF32 constWeight,const LinearInput *inputs,sInt nInputs)
{
  sInt w[256],uo[256],vo[256];
  
  sVERIFY(nInputs <= 255);
  sVERIFY(constWeight >= -127.0f && constWeight <= 127.0f);

  // convert weights and offsets to fixed point
  for(sInt i=0;i<nInputs;i++)
  {
    sVERIFY(inputs[i].Weight >= -127.0f && inputs[i].Weight <= 127.0f);
    sVERIFY(inputs[i].UShift >= -127.0f && inputs[i].UShift <= 127.0f);
    sVERIFY(inputs[i].VShift >= -127.0f && inputs[i].VShift <= 127.0f);

    w[i] = inputs[i].Weight * 65536.0f;
    uo[i] = inputs[i].UShift * (1 << 24);
    vo[i] = inputs[i].VShift * (1 << 24);
  }

  // compute preweighted constant color
  sInt c_r,c_g,c_b,c_a,t;

  t = constWeight * 65536.0f;
  c_r = MulShift16(t,color.r);
  c_g = MulShift16(t,color.g);
  c_b = MulShift16(t,color.b);
  c_a = MulShift16(t,color.a);

  // calculate output image
  sInt u0 = MinX;
  sInt v0 = MinY;
  sInt stepU = 1 << (24 - ShiftX);
  sInt stepV = 1 << (24 - ShiftY);
  Pixel *out = Data;

  for(sInt y=0;y<YRes;y++)
  {
    sInt u = u0;
    sInt v = v0;

    for(sInt x=0;x<XRes;x++)
    {
      sInt acc_r,acc_g,acc_b,acc_a;

      // initialize accumulator with start value
      acc_r = c_r;
      acc_g = c_g;
      acc_b = c_b;
      acc_a = c_a;

      // accumulate inputs
      for(sInt j=0;j<nInputs;j++)
      {
        const LinearInput &in = inputs[j];
        Pixel inPix;

        in.Tex->SampleFiltered(inPix,u + uo[j],v + vo[j],in.FilterMode);

        acc_r += MulShift16(w[j],inPix.r);
        acc_g += MulShift16(w[j],inPix.g);
        acc_b += MulShift16(w[j],inPix.b);
        acc_a += MulShift16(w[j],inPix.a);
      }

      // store (with clamping)
      out->r = sClamp(acc_r,0,65535);
      out->g = sClamp(acc_g,0,65535);
      out->b = sClamp(acc_b,0,65535);
      out->a = sClamp(acc_a,0,65535);

      // advance to next pixel
      u += stepU;
      out++;
    }

    v0 += stepV;
  }
}

void InitTexgen()
{
  InitPerlin();
}
