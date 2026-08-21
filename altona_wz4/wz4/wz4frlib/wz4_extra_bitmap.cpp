/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "wz4frlib/wz4_extra_bitmap.hpp"

namespace Wz4Extra
{

/****************************************************************************/
/***                                                                      ***/
/***   pixels                                                             ***/
/***                                                                      ***/
/****************************************************************************/

// A pixel unpacked into signed ints, so that intermediate results are allowed
// to leave the 0..0x7fff range and get clamped once, on the way back in.

struct Pixel
{
  sInt b,g,r,a;

  void Load(sU64 p)
  {
    b = sInt((p    ) & 0xffff);
    g = sInt((p>>16) & 0xffff);
    r = sInt((p>>32) & 0xffff);
    a = sInt((p>>48) & 0xffff);
  }

  sU64 Store() const
  {
    return (sU64(sClamp<sInt>(b,0,MaxChannel))    )
         | (sU64(sClamp<sInt>(g,0,MaxChannel))<<16)
         | (sU64(sClamp<sInt>(r,0,MaxChannel))<<32)
         | (sU64(sClamp<sInt>(a,0,MaxChannel))<<48);
  }

  sInt Channel(sInt sel) const
  {
    switch(sel)
    {
    default:
    case CHANNEL_LUMINANCE: return (r*77 + g*151 + b*28)>>8;
    case CHANNEL_RED:       return r;
    case CHANNEL_GREEN:     return g;
    case CHANNEL_BLUE:      return b;
    case CHANNEL_ALPHA:     return a;
    }
  }
};

sU64 Color64(sU32 c)
{
  sU64 col = c;
  col = ((col&0xff000000)<<24)
      | ((col&0x00ff0000)<<16)
      | ((col&0x0000ff00)<< 8)
      | ((col&0x000000ff)    );
  return ((col | (col<<8))>>1) & 0x7fff7fff7fff7fffULL;
}

// fade 0..1 between two packed colors
static sU64 Fade(sU64 a,sU64 b,sF32 f)
{
  Pixel pa,pb,pr;
  pa.Load(a);
  pb.Load(b);
  f = sClamp<sF32>(f,0,1);
  pr.b = pa.b + sInt((pb.b-pa.b)*f);
  pr.g = pa.g + sInt((pb.g-pa.g)*f);
  pr.r = pa.r + sInt((pb.r-pa.r)*f);
  pr.a = pa.a + sInt((pb.a-pa.a)*f);
  return pr.Store();
}

static sInt WrapCoord(sInt v,sInt max,sInt wrap)
{
  if(wrap==WRAP_TILE)
  {
    v %= max;
    if(v<0) v += max;
    return v;
  }
  return sClamp<sInt>(v,0,max-1);
}

static sU64 Fetch(const sU64 *src,sInt xs,sInt ys,sInt x,sInt y,sInt wrap)
{
  return src[WrapCoord(x,xs,wrap) + WrapCoord(y,ys,wrap)*xs];
}

// u,v in texture coordinates: 0..1 covers the image
static sU64 SampleBilinear(const sU64 *src,sInt xs,sInt ys,sF32 u,sF32 v,sInt wrap)
{
  sF32 fx = u*xs - 0.5f;
  sF32 fy = v*ys - 0.5f;
  sInt x0 = sInt(sRoundDown(fx));
  sInt y0 = sInt(sRoundDown(fy));
  sF32 tx = fx - x0;
  sF32 ty = fy - y0;

  Pixel p00,p01,p10,p11,pr;
  p00.Load(Fetch(src,xs,ys,x0  ,y0  ,wrap));
  p01.Load(Fetch(src,xs,ys,x0+1,y0  ,wrap));
  p10.Load(Fetch(src,xs,ys,x0  ,y0+1,wrap));
  p11.Load(Fetch(src,xs,ys,x0+1,y0+1,wrap));

  sF32 w00 = (1-tx)*(1-ty);
  sF32 w01 = (  tx)*(1-ty);
  sF32 w10 = (1-tx)*(  ty);
  sF32 w11 = (  tx)*(  ty);

  pr.b = sInt(p00.b*w00 + p01.b*w01 + p10.b*w10 + p11.b*w11);
  pr.g = sInt(p00.g*w00 + p01.g*w01 + p10.g*w10 + p11.g*w11);
  pr.r = sInt(p00.r*w00 + p01.r*w01 + p10.r*w10 + p11.r*w11);
  pr.a = sInt(p00.a*w00 + p01.a*w01 + p10.a*w10 + p11.a*w11);
  return pr.Store();
}

/****************************************************************************/
/***                                                                      ***/
/***   generators                                                         ***/
/***                                                                      ***/
/****************************************************************************/

void Checker(sU64 *dst,sInt xs,sInt ys,sInt countx,sInt county,sF32 phase,
             sU64 col0,sU64 col1)
{
  countx = sMax(1,countx);
  county = sMax(1,county);

  for(sInt y=0;y<ys;y++)
  {
    sInt iy = (y*county)/ys;

    // phase slides every other row of cells sideways, which is what turns a
    // checkerboard into a brick-ish or a half-drop pattern. It counts in
    // cells, so 0.5 is the half-brick offset a wall is laid with.
    sF32 shift = (iy&1) ? phase/countx : 0.0f;

    for(sInt x=0;x<xs;x++)
    {
      sF32 fx = sF32(x)/xs + shift;
      fx -= sRoundDown(fx);
      sInt ix = sInt(fx*countx);
      dst[x+y*xs] = ((ix+iy)&1) ? col1 : col0;
    }
  }
}

void Sine(sU64 *dst,sInt xs,sInt ys,sInt waves,sF32 freq,sF32 angle,sF32 spread,
          sF32 phase,sF32 amp,sF32 gamma,sU64 col0,sU64 col1)
{
  waves = sClamp<sInt>(waves,1,16);
  if(gamma<=0.0f) gamma = 1.0f;

  // precalculate one direction and one phase per wave
  sF32 dx[16],dy[16],ph[16];
  for(sInt i=0;i<waves;i++)
  {
    sF32 a = (angle + spread*i)*sPI2F;
    dx[i] = sFCos(a)*freq*sPI2F;
    dy[i] = sFSin(a)*freq*sPI2F;
    ph[i] = (phase*(i+1))*sPI2F;
  }

  sF32 scale = amp/waves;

  for(sInt y=0;y<ys;y++)
  {
    sF32 fy = sF32(y)/ys;
    for(sInt x=0;x<xs;x++)
    {
      sF32 fx = sF32(x)/xs;
      sF32 v = 0;
      for(sInt i=0;i<waves;i++)
        v += sFSin(fx*dx[i] + fy*dy[i] + ph[i]);

      v = v*scale*0.5f + 0.5f;
      v = sClamp<sF32>(v,0,1);
      if(gamma!=1.0f)
        v = sFPow(v,gamma);
      dst[x+y*xs] = Fade(col0,col1,v);
    }
  }
}

void Julia(sU64 *dst,sInt xs,sInt ys,sBool julia,sF32 cx,sF32 cy,
           sF32 centerx,sF32 centery,sF32 zoom,sInt iterations,sF32 cycle,
           sU64 colin,sU64 col0,sU64 col1)
{
  iterations = sClamp<sInt>(iterations,1,4096);
  if(zoom<=0.0001f) zoom = 0.0001f;
  if(cycle<=0.0f) cycle = 1.0f;

  sF32 aspect = sF32(xs)/sF32(ys);

  for(sInt y=0;y<ys;y++)
  {
    for(sInt x=0;x<xs;x++)
    {
      sF32 px = ((sF32(x)+0.5f)/xs - 0.5f)*2.0f*aspect/zoom + centerx;
      sF32 py = ((sF32(y)+0.5f)/ys - 0.5f)*2.0f        /zoom + centery;

      sF32 zx,zy,kx,ky;
      if(julia)
      {
        zx = px; zy = py;
        kx = cx; ky = cy;
      }
      else
      {
        zx = 0;  zy = 0;
        kx = px; ky = py;
      }

      sInt i;
      sF32 zx2 = zx*zx, zy2 = zy*zy;
      for(i=0;i<iterations && zx2+zy2<=256.0f;i++)
      {
        zy = 2*zx*zy + ky;
        zx = zx2-zy2 + kx;
        zx2 = zx*zx;
        zy2 = zy*zy;
      }

      if(i>=iterations)
      {
        dst[x+y*xs] = colin;
      }
      else
      {
        // smooth iteration count, so the bands do not stair-step
        sF32 mag = zx2+zy2;
        sF32 f = i + 1.0f;
        if(mag>1.0f)
          f -= sFLog(0.5f*sFLog(mag)/sFLog(2.0f))/sFLog(2.0f);

        f = f/cycle;
        f -= sRoundDown(f);
        // triangle, so that consecutive bands meet instead of jumping back
        f = f<0.5f ? f*2 : (1-f)*2;
        dst[x+y*xs] = Fade(col0,col1,f);
      }
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   filters                                                            ***/
/***                                                                      ***/
/****************************************************************************/

void Colorize(sU64 *dst,const sU64 *src,sInt xs,sInt ys,
              const sU64 *lut,sInt lutlen,sInt channel,sBool keepalpha)
{
  if(lutlen<1) return;

  for(sInt i=0;i<xs*ys;i++)
  {
    Pixel p;
    p.Load(src[i]);
    sF32 t = sClamp<sF32>(sF32(p.Channel(channel))/MaxChannel,0,1);

    // interpolate along the palette rather than picking the nearest entry,
    // otherwise a short gradient posterizes everything it colors
    sF32 f = t*(lutlen-1);
    sInt i0 = sClamp<sInt>(sInt(f),0,lutlen-1);
    sInt i1 = sMin(i0+1,lutlen-1);
    sU64 col = Fade(lut[i0],lut[i1],f-i0);

    if(keepalpha)
    {
      Pixel c;
      c.Load(col);
      c.a = p.a;
      col = c.Store();
    }
    dst[i] = col;
  }
}

/****************************************************************************/

// 8SSEDT: every cell remembers the offset to the nearest seed found so far,
// and two sweeps -- one down, one up -- propagate those offsets around.

namespace
{
  struct DistPt
  {
    sS16 dx,dy;
    sInt Dist2() const { return sInt(dx)*dx + sInt(dy)*dy; }
  };

  static const sInt DIST_FAR = 0x3fff;

  struct DistGrid
  {
    DistPt *P;
    sInt XS,YS,Wrap;

    void Compare(DistPt &p,sInt x,sInt y,sInt ox,sInt oy)
    {
      sInt nx = x+ox;
      sInt ny = y+oy;

      // Off the edge there is no neighbour to ask. Clamping to the border
      // cell instead would hand back that cell's own offset stepped by one,
      // which reads as "a seed one pixel further out" -- a lie that then
      // propagates inwards and eats holes into the field. Tiling is different:
      // there the neighbour really is on the other side.
      if(Wrap==WRAP_TILE)
      {
        nx = WrapCoord(nx,XS,WRAP_TILE);
        ny = WrapCoord(ny,YS,WRAP_TILE);
      }
      else if(nx<0 || nx>=XS || ny<0 || ny>=YS)
      {
        return;
      }

      // the offset of the neighbour, extended by the step we took to reach it
      DistPt o = P[nx+ny*XS];
      o.dx = sS16(o.dx + ox);
      o.dy = sS16(o.dy + oy);
      if(o.Dist2() < p.Dist2())
        p = o;
    }

    void Sweep()
    {
      for(sInt y=0;y<YS;y++)
      {
        for(sInt x=0;x<XS;x++)
        {
          DistPt &p = P[x+y*XS];
          Compare(p,x,y,-1, 0);
          Compare(p,x,y, 0,-1);
          Compare(p,x,y,-1,-1);
          Compare(p,x,y, 1,-1);
        }
        for(sInt x=XS-1;x>=0;x--)
          Compare(P[x+y*XS],x,y,1,0);
      }
      for(sInt y=YS-1;y>=0;y--)
      {
        for(sInt x=XS-1;x>=0;x--)
        {
          DistPt &p = P[x+y*XS];
          Compare(p,x,y, 1,0);
          Compare(p,x,y, 0,1);
          Compare(p,x,y,-1,1);
          Compare(p,x,y, 1,1);
        }
        for(sInt x=0;x<XS;x++)
          Compare(P[x+y*XS],x,y,-1,0);
      }
    }
  };
}

void DistanceField(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sInt channel,
                   sF32 threshold,sF32 range,sInt wrap,sU64 col0,sU64 col1)
{
  sInt size = xs*ys;
  sInt thr = sInt(sClamp<sF32>(threshold,0,1)*MaxChannel);
  if(range<=0.0f) range = 1.0f;

  DistPt *inside = new DistPt[size];
  DistPt *outside = new DistPt[size];

  for(sInt i=0;i<size;i++)
  {
    Pixel p;
    p.Load(src[i]);
    sBool set = p.Channel(channel) >= thr;

    // a cell that is part of the shape is at distance 0 from the shape, and
    // unknown-far from the background, and the other way around
    inside [i].dx = set ? 0 : DIST_FAR;
    inside [i].dy = 0;
    outside[i].dx = set ? DIST_FAR : 0;
    outside[i].dy = 0;
  }

  DistGrid gi; gi.P = inside;  gi.XS = xs; gi.YS = ys; gi.Wrap = wrap;
  DistGrid go; go.P = outside; go.XS = xs; go.YS = ys; go.Wrap = wrap;
  gi.Sweep();
  go.Sweep();

  for(sInt i=0;i<size;i++)
  {
    // Distance to the contour, negative inside the shape: the "outside" grid
    // was seeded with the background, so what it holds for a cell inside the
    // shape is how deep in that cell sits.
    sF32 d = sFSqrt(sF32(inside[i].Dist2())) - sFSqrt(sF32(outside[i].Dist2()));
    sF32 t = 0.5f + 0.5f*d/range;
    dst[i] = Fade(col0,col1,sClamp<sF32>(t,0,1));
  }

  delete[] inside;
  delete[] outside;
}

/****************************************************************************/

// one separable min/max pass, horizontal when stride is 1
static void MorphPass(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sInt radius,
                      sBool dilate,sBool vertical,sInt wrap)
{
  sInt outer = vertical ? xs : ys;
  sInt inner = vertical ? ys : xs;
  sInt step  = vertical ? xs : 1;

  for(sInt o=0;o<outer;o++)
  {
    const sU64 *s = src + (vertical ? o : o*xs);
    sU64 *d       = dst + (vertical ? o : o*xs);

    for(sInt i=0;i<inner;i++)
    {
      Pixel acc;
      acc.Load(s[i*step]);

      for(sInt k=-radius;k<=radius;k++)
      {
        if(k==0) continue;
        sInt j = i+k;
        if(wrap==WRAP_TILE)
        {
          j %= inner;
          if(j<0) j += inner;
        }
        else
        {
          if(j<0 || j>=inner) continue;
        }

        Pixel p;
        p.Load(s[j*step]);
        if(dilate)
        {
          acc.b = sMax(acc.b,p.b); acc.g = sMax(acc.g,p.g);
          acc.r = sMax(acc.r,p.r); acc.a = sMax(acc.a,p.a);
        }
        else
        {
          acc.b = sMin(acc.b,p.b); acc.g = sMin(acc.g,p.g);
          acc.r = sMin(acc.r,p.r); acc.a = sMin(acc.a,p.a);
        }
      }
      d[i*step] = acc.Store();
    }
  }
}

// the round kernel is not separable, so it pays for its shape by the square
static void MorphDisc(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sInt radius,
                      sBool dilate,sInt wrap)
{
  sInt r2 = radius*radius;
  for(sInt y=0;y<ys;y++)
  {
    for(sInt x=0;x<xs;x++)
    {
      Pixel acc;
      acc.Load(src[x+y*xs]);
      for(sInt dy=-radius;dy<=radius;dy++)
      {
        for(sInt dx=-radius;dx<=radius;dx++)
        {
          if(dx*dx+dy*dy > r2) continue;
          if(wrap!=WRAP_TILE && (x+dx<0 || x+dx>=xs || y+dy<0 || y+dy>=ys)) continue;

          Pixel p;
          p.Load(Fetch(src,xs,ys,x+dx,y+dy,wrap));
          if(dilate)
          {
            acc.b = sMax(acc.b,p.b); acc.g = sMax(acc.g,p.g);
            acc.r = sMax(acc.r,p.r); acc.a = sMax(acc.a,p.a);
          }
          else
          {
            acc.b = sMin(acc.b,p.b); acc.g = sMin(acc.g,p.g);
            acc.r = sMin(acc.r,p.r); acc.a = sMin(acc.a,p.a);
          }
        }
      }
      dst[x+y*xs] = acc.Store();
    }
  }
}

static void MorphOnce(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sInt radius,
                      sBool dilate,sBool circle,sInt wrap)
{
  if(circle)
  {
    MorphDisc(dst,src,xs,ys,radius,dilate,wrap);
  }
  else
  {
    sU64 *tmp = new sU64[xs*ys];
    MorphPass(tmp,src,xs,ys,radius,dilate,sFALSE,wrap);
    MorphPass(dst,tmp,xs,ys,radius,dilate,sTRUE ,wrap);
    delete[] tmp;
  }
}

void Morphology(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sInt mode,
                sInt radius,sBool circle,sInt wrap)
{
  radius = sClamp<sInt>(radius,0,circle ? 24 : 64);
  if(radius==0)
  {
    sCopyMem(dst,src,xs*ys*sizeof(sU64));
    return;
  }

  switch(mode)
  {
  case MORPH_ERODE:
    MorphOnce(dst,src,xs,ys,radius,sFALSE,circle,wrap);
    break;
  case MORPH_DILATE:
    MorphOnce(dst,src,xs,ys,radius,sTRUE,circle,wrap);
    break;
  case MORPH_OPEN:
  case MORPH_CLOSE:
    {
      sBool first = (mode==MORPH_CLOSE);
      sU64 *tmp = new sU64[xs*ys];
      MorphOnce(tmp,src,xs,ys,radius,first,circle,wrap);
      MorphOnce(dst,tmp,xs,ys,radius,!first,circle,wrap);
      delete[] tmp;
    }
    break;
  }
}

/****************************************************************************/

void Median(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sInt size,sInt wrap)
{
  sInt radius = sClamp<sInt>(size,1,3);
  sInt count = (radius*2+1)*(radius*2+1);
  sInt half = count/2;

  sInt *vb = new sInt[count];
  sInt *vg = new sInt[count];
  sInt *vr = new sInt[count];
  sInt *va = new sInt[count];

  for(sInt y=0;y<ys;y++)
  {
    for(sInt x=0;x<xs;x++)
    {
      sInt n=0;
      for(sInt dy=-radius;dy<=radius;dy++)
      {
        for(sInt dx=-radius;dx<=radius;dx++)
        {
          Pixel p;
          p.Load(Fetch(src,xs,ys,x+dx,y+dy,wrap));
          vb[n]=p.b; vg[n]=p.g; vr[n]=p.r; va[n]=p.a; n++;
        }
      }

      // insertion sort: n is 9 or 25 or 49, which is where it wins
      sInt *ch[4] = { vb,vg,vr,va };
      for(sInt c=0;c<4;c++)
      {
        sInt *v = ch[c];
        for(sInt i=1;i<n;i++)
        {
          sInt t = v[i];
          sInt j = i-1;
          while(j>=0 && v[j]>t) { v[j+1] = v[j]; j--; }
          v[j+1] = t;
        }
      }

      Pixel r;
      r.b = vb[half]; r.g = vg[half]; r.r = vr[half]; r.a = va[half];
      dst[x+y*xs] = r.Store();
    }
  }

  delete[] vb; delete[] vg; delete[] vr; delete[] va;
}

/****************************************************************************/

void Sobel(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sInt mode,sInt channel,
           sF32 angle,sF32 amp,sInt wrap)
{
  sF32 ca = sFCos(angle*sPI2F);
  sF32 sa = sFSin(angle*sPI2F);

  for(sInt y=0;y<ys;y++)
  {
    for(sInt x=0;x<xs;x++)
    {
      sInt v[3][3];
      for(sInt dy=-1;dy<=1;dy++)
      {
        for(sInt dx=-1;dx<=1;dx++)
        {
          Pixel p;
          p.Load(Fetch(src,xs,ys,x+dx,y+dy,wrap));
          v[dy+1][dx+1] = p.Channel(channel);
        }
      }

      sF32 gx = sF32((v[0][2]+2*v[1][2]+v[2][2]) - (v[0][0]+2*v[1][0]+v[2][0]));
      sF32 gy = sF32((v[2][0]+2*v[2][1]+v[2][2]) - (v[0][0]+2*v[0][1]+v[0][2]));
      gx /= 4*MaxChannel;
      gy /= 4*MaxChannel;

      sF32 t;
      if(mode==0)
        t = sFSqrt(gx*gx+gy*gy)*amp;                 // magnitude
      else
        t = 0.5f + (gx*ca + gy*sa)*amp*0.5f;         // directional emboss

      sInt c = sInt(sClamp<sF32>(t,0,1)*MaxChannel);
      Pixel p;
      p.b = p.g = p.r = c;
      p.a = MaxChannel;
      dst[x+y*xs] = p.Store();
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   samplers                                                           ***/
/***                                                                      ***/
/****************************************************************************/

void Kaleidoscope(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sInt sectors,
                  sF32 cx,sF32 cy,sF32 angle,sF32 zoom,sBool mirror,sInt wrap)
{
  sectors = sClamp<sInt>(sectors,1,256);
  if(zoom<=0.0001f) zoom = 0.0001f;

  sF32 sector = sPI2F/sectors;
  sF32 rot = angle*sPI2F;

  for(sInt y=0;y<ys;y++)
  {
    for(sInt x=0;x<xs;x++)
    {
      sF32 dx = (sF32(x)+0.5f)/xs - cx;
      sF32 dy = (sF32(y)+0.5f)/ys - cy;

      sF32 r = sFSqrt(dx*dx+dy*dy)/zoom;
      sF32 a = sFATan2(dy,dx) - rot;

      // fold the full circle down into one sector
      sF32 t = a/sector;
      sF32 fi = sRoundDown(t);
      sF32 f = t - fi;
      if(mirror && (sInt(fi)&1))
        f = 1.0f - f;
      a = f*sector + rot;

      sF32 u = cx + sFCos(a)*r;
      sF32 v = cy + sFSin(a)*r;
      dst[x+y*xs] = SampleBilinear(src,xs,ys,u,v,wrap);
    }
  }
}

void Scatter(sU64 *dst,const sU64 *src,sInt xs,sInt ys,
             const sU64 *stamp,sInt sxs,sInt sys,
             sInt count,sInt seed,sF32 scalemin,sF32 scalemax,
             sF32 anglemin,sF32 anglemax,sF32 alpha,sF32 tint,sInt mode,sInt wrap)
{
  sCopyMem(dst,src,xs*ys*sizeof(sU64));

  count = sClamp<sInt>(count,0,4096);
  if(count==0) return;

  sRandomMT rnd;
  rnd.Seed(seed);

  for(sInt i=0;i<count;i++)
  {
    sF32 px = rnd.Float(1.0f);
    sF32 py = rnd.Float(1.0f);
    sF32 scale = scalemin + rnd.Float(1.0f)*(scalemax-scalemin);
    sF32 ang = (anglemin + rnd.Float(1.0f)*(anglemax-anglemin))*sPI2F;
    sF32 shade = 1.0f - rnd.Float(1.0f)*sClamp<sF32>(tint,0,1);

    if(scale<=0.0001f) continue;

    // the stamp covers this much of the destination, in pixels
    sF32 w = sxs*scale;
    sF32 h = sys*scale;
    sF32 rad = sFSqrt(w*w+h*h)*0.5f + 1;
    sF32 dcx = px*xs;
    sF32 dcy = py*ys;

    sInt x0 = sInt(sRoundDown(dcx-rad));
    sInt x1 = sInt(sFCeil (dcx+rad));
    sInt y0 = sInt(sRoundDown(dcy-rad));
    sInt y1 = sInt(sFCeil (dcy+rad));

    if(wrap!=WRAP_TILE)
    {
      x0 = sMax(x0,0); y0 = sMax(y0,0);
      x1 = sMin(x1,xs); y1 = sMin(y1,ys);
    }

    sF32 ca = sFCos(-ang), sa = sFSin(-ang);

    for(sInt y=y0;y<y1;y++)
    {
      for(sInt x=x0;x<x1;x++)
      {
        // destination pixel -> stamp texture coordinates
        sF32 ox = (x+0.5f) - dcx;
        sF32 oy = (y+0.5f) - dcy;
        sF32 rx = (ox*ca - oy*sa)/w + 0.5f;
        sF32 ry = (ox*sa + oy*ca)/h + 0.5f;
        if(rx<0 || rx>=1 || ry<0 || ry>=1) continue;

        sInt dx = WrapCoord(x,xs,wrap);
        sInt dy = WrapCoord(y,ys,wrap);
        if(wrap!=WRAP_TILE && (dx!=x || dy!=y)) continue;

        Pixel s,d;
        s.Load(SampleBilinear(stamp,sxs,sys,rx,ry,WRAP_CLAMP));
        d.Load(dst[dx+dy*xs]);

        s.b = sInt(s.b*shade);
        s.g = sInt(s.g*shade);
        s.r = sInt(s.r*shade);

        sF32 a = sClamp<sF32>(alpha,0,1) * (sF32(s.a)/MaxChannel);

        switch(mode)
        {
        default:
        case SCATTER_OVER:
          d.b = sInt(d.b + (s.b-d.b)*a);
          d.g = sInt(d.g + (s.g-d.g)*a);
          d.r = sInt(d.r + (s.r-d.r)*a);
          d.a = sMax(d.a,sInt(s.a*sClamp<sF32>(alpha,0,1)));
          break;
        case SCATTER_ADD:
          d.b += sInt(s.b*a);
          d.g += sInt(s.g*a);
          d.r += sInt(s.r*a);
          d.a += sInt(s.a*a);
          break;
        case SCATTER_MAX:
          d.b = sMax(d.b,sInt(s.b*a));
          d.g = sMax(d.g,sInt(s.g*a));
          d.r = sMax(d.r,sInt(s.r*a));
          d.a = sMax(d.a,sInt(s.a*a));
          break;
        }

        dst[dx+dy*xs] = d.Store();
      }
    }
  }
}


/****************************************************************************/
/***                                                                      ***/
/***   value noise                                                        ***/
/***                                                                      ***/
/****************************************************************************/

// Wood and Marble need turbulence, and the engine's perlin lives behind
// xInitPerlin() and a table this file cannot see. A hashed lattice does the
// same job here and stays tileable, which the pattern generators want.

static sU32 NoiseHash(sInt x,sInt y,sInt seed)
{
  sU32 h = sU32(x)*374761393u + sU32(y)*668265263u + sU32(seed)*2246822519u;
  h = (h ^ (h>>13)) * 1274126177u;
  return h ^ (h>>16);
}

static sF32 NoiseAt(sInt x,sInt y,sInt period,sInt seed)
{
  if(period>0)
  {
    x %= period; if(x<0) x += period;
    y %= period; if(y<0) y += period;
  }
  return (NoiseHash(x,y,seed) & 0xffffff)/16777215.0f;
}

static sF32 ValueNoise(sF32 x,sF32 y,sInt period,sInt seed)
{
  sF32 fx = sRoundDown(x), fy = sRoundDown(y);
  sInt ix = sInt(fx), iy = sInt(fy);
  sF32 tx = x-fx, ty = y-fy;

  // smoothstep on the interpolant, or the lattice shows through as a grid
  tx = tx*tx*(3-2*tx);
  ty = ty*ty*(3-2*ty);

  sF32 a = NoiseAt(ix  ,iy  ,period,seed);
  sF32 b = NoiseAt(ix+1,iy  ,period,seed);
  sF32 c = NoiseAt(ix  ,iy+1,period,seed);
  sF32 d = NoiseAt(ix+1,iy+1,period,seed);

  return (a + (b-a)*tx) + ((c + (d-c)*tx) - (a + (b-a)*tx))*ty;
}

static sF32 Turbulence(sF32 x,sF32 y,sInt period,sInt octaves,sInt seed)
{
  octaves = sClamp<sInt>(octaves,1,10);
  sF32 sum = 0, amp = 1, norm = 0;
  for(sInt i=0;i<octaves;i++)
  {
    sum += sAbs(ValueNoise(x,y,period,seed+i*71)*2-1)*amp;
    norm += amp;
    amp *= 0.5f;
    x *= 2; y *= 2;
    period *= 2;
  }
  return sum/norm;
}

// 0..1 -> 0..1, pushed towards its ends. sharpness 0 leaves it alone.
static sF32 Sharpen01(sF32 v,sF32 sharpness)
{
  v = sClamp<sF32>(v,0,1);
  if(sharpness<=0.0f) return v;
  sF32 e = 1.0f + sharpness*8.0f;
  return sFPow(v,e);
}

/****************************************************************************/
/***                                                                      ***/
/***   generators, second batch                                           ***/
/***                                                                      ***/
/****************************************************************************/

void Noise(sU64 *dst,sInt xs,sInt ys,sInt seed,sBool colour,sF32 gamma,
           sU64 col0,sU64 col1)
{
  if(gamma<=0.0f) gamma = 1.0f;
  Pixel p0,p1;
  p0.Load(col0);
  p1.Load(col1);

  for(sInt y=0;y<ys;y++)
  {
    for(sInt x=0;x<xs;x++)
    {
      if(!colour)
      {
        sF32 v = NoiseAt(x,y,0,seed);
        if(gamma!=1.0f) v = sFPow(v,gamma);
        dst[x+y*xs] = Fade(col0,col1,v);
      }
      else
      {
        // one roll per channel: rgb static rather than grain
        Pixel p;
        sF32 vb = NoiseAt(x,y,0,seed);
        sF32 vg = NoiseAt(x,y,0,seed+7919);
        sF32 vr = NoiseAt(x,y,0,seed+104729);
        if(gamma!=1.0f)
        {
          vb = sFPow(vb,gamma); vg = sFPow(vg,gamma); vr = sFPow(vr,gamma);
        }
        p.b = p0.b + sInt((p1.b-p0.b)*vb);
        p.g = p0.g + sInt((p1.g-p0.g)*vg);
        p.r = p0.r + sInt((p1.r-p0.r)*vr);
        p.a = p1.a;
        dst[x+y*xs] = p.Store();
      }
    }
  }
}

void Spiral(sU64 *dst,sInt xs,sInt ys,sInt arms,sF32 twist,sF32 cx,sF32 cy,
            sF32 sharpness,sU64 col0,sU64 col1)
{
  arms = sClamp<sInt>(arms,1,64);

  for(sInt y=0;y<ys;y++)
  {
    for(sInt x=0;x<xs;x++)
    {
      sF32 dx = (sF32(x)+0.5f)/xs - cx;
      sF32 dy = (sF32(y)+0.5f)/ys - cy;
      sF32 r = sFSqrt(dx*dx+dy*dy);
      sF32 a = sFATan2(dy,dx)/sPI2F;

      // winding the angle with the radius is what turns spokes into a spiral
      sF32 t = (a + twist*r)*arms;
      t -= sRoundDown(t);

      sF32 v = t<0.5f ? t*2 : (1-t)*2;
      dst[x+y*xs] = Fade(col0,col1,Sharpen01(v,sharpness));
    }
  }
}

void Rays(sU64 *dst,sInt xs,sInt ys,sInt count,sF32 cx,sF32 cy,sF32 angle,
          sF32 sharpness,sF32 falloff,sU64 col0,sU64 col1)
{
  count = sClamp<sInt>(count,1,256);

  for(sInt y=0;y<ys;y++)
  {
    for(sInt x=0;x<xs;x++)
    {
      sF32 dx = (sF32(x)+0.5f)/xs - cx;
      sF32 dy = (sF32(y)+0.5f)/ys - cy;
      sF32 r = sFSqrt(dx*dx+dy*dy)*2;
      sF32 a = sFATan2(dy,dx)/sPI2F - angle;

      sF32 t = a*count;
      t -= sRoundDown(t);
      sF32 v = t<0.5f ? t*2 : (1-t)*2;
      v = Sharpen01(v,sharpness);

      // and fade the whole thing out with distance from the centre
      sF32 fade = sClamp<sF32>(1.0f-r,0,1);
      if(falloff!=1.0f)
        fade = sFPow(fade,sMax(0.01f,falloff));

      dst[x+y*xs] = Fade(col0,col1,v*fade);
    }
  }
}

void Hexagon(sU64 *dst,sInt xs,sInt ys,sInt countx,sInt county,sF32 width,
             sInt seed,sF32 variation,sU64 col0,sU64 col1)
{
  countx = sMax(1,countx);
  county = sMax(1,county);

  for(sInt y=0;y<ys;y++)
  {
    for(sInt x=0;x<xs;x++)
    {
      sF32 fx = sF32(x)/xs*countx;
      sF32 fy = sF32(y)/ys*county;

      // hex centres sit on two interleaved rectangular lattices; the nearest
      // of the nine candidates around a point is the cell it belongs to, and
      // the runner up says how close the border is
      sF32 best = 1e9f, second = 1e9f;
      sInt bx = 0,by = 0;
      for(sInt oy=-1;oy<=1;oy++)
      {
        for(sInt ox=-1;ox<=1;ox++)
        {
          sInt ix = sInt(sRoundDown(fx))+ox;
          sInt iy = sInt(sRoundDown(fy))+oy;
          sF32 px = ix + ((iy&1) ? 0.5f : 0.0f) + 0.5f;
          sF32 py = iy + 0.5f;
          sF32 dx = fx-px, dy = (fy-py)*1.1547f;   // 2/sqrt(3): make them regular
          sF32 d = sFSqrt(dx*dx+dy*dy);
          if(d<best) { second = best; best = d; bx = ix; by = iy; }
          else if(d<second) { second = d; }
        }
      }

      sF32 edge = second-best;                     // 0 exactly on a border
      sF32 v;
      if(width<=0.0f)
      {
        v = 1.0f;
      }
      else
      {
        v = sClamp<sF32>(edge/width,0,1);
      }

      // let the cells differ from each other, the way real tiling does
      if(variation>0)
      {
        sF32 r = NoiseAt(bx,by,0,seed);
        v *= 1.0f - variation*r;
      }

      dst[x+y*xs] = Fade(col0,col1,v);
    }
  }
}

void Truchet(sU64 *dst,sInt xs,sInt ys,sInt count,sInt seed,sF32 width,
             sBool diagonal,sU64 col0,sU64 col1)
{
  count = sClamp<sInt>(count,1,256);
  if(width<=0.0f) width = 0.1f;

  for(sInt y=0;y<ys;y++)
  {
    for(sInt x=0;x<xs;x++)
    {
      sF32 fx = sF32(x)/xs*count;
      sF32 fy = sF32(y)/ys*count;
      sInt ix = sInt(sRoundDown(fx));
      sInt iy = sInt(sRoundDown(fy));
      sF32 u = fx-ix, v = fy-iy;

      // each cell picks one of the two rotations, and because both of them
      // meet the cell border at the same four points, the tiles always join
      if(NoiseHash(ix,iy,seed)&1)
        u = 1.0f-u;

      sF32 d;
      if(diagonal)
      {
        d = sAbs(u-v)*0.70710678f;
      }
      else
      {
        // two quarter circles, around opposite corners
        sF32 d0 = sAbs(sFSqrt(u*u+v*v)-0.5f);
        sF32 d1 = sAbs(sFSqrt((1-u)*(1-u)+(1-v)*(1-v))-0.5f);
        d = sMin(d0,d1);
      }

      sF32 t = sClamp<sF32>(1.0f-d/width,0,1);
      dst[x+y*xs] = Fade(col0,col1,t);
    }
  }
}

void Weave(sU64 *dst,sInt xs,sInt ys,sInt countx,sInt county,sF32 width,
           sF32 shadow,sU64 col0,sU64 col1,sU64 colgap)
{
  countx = sMax(1,countx);
  county = sMax(1,county);
  width = sClamp<sF32>(width,0.01f,1.0f);

  for(sInt y=0;y<ys;y++)
  {
    for(sInt x=0;x<xs;x++)
    {
      sF32 fx = sF32(x)/xs*countx;
      sF32 fy = sF32(y)/ys*county;
      sInt ix = sInt(sRoundDown(fx));
      sInt iy = sInt(sRoundDown(fy));
      sF32 u = fx-ix, v = fy-iy;

      // in every other cell the horizontal strand passes over the vertical one
      sBool horizontal = ((ix+iy)&1)==0;
      sF32 across = horizontal ? v : u;

      sF32 half = width*0.5f;
      sF32 d = sAbs(across-0.5f);
      if(d>half)
      {
        dst[x+y*xs] = colgap;
        continue;
      }

      // round the strand off towards its edges
      sF32 shade = 1.0f - shadow*(d/half)*(d/half);
      sU64 col = horizontal ? col0 : col1;
      Pixel p;
      p.Load(col);
      p.b = sInt(p.b*shade);
      p.g = sInt(p.g*shade);
      p.r = sInt(p.r*shade);
      dst[x+y*xs] = p.Store();
    }
  }
}

void Wood(sU64 *dst,sInt xs,sInt ys,sF32 rings,sF32 turbulence,sInt octaves,
          sInt seed,sF32 cx,sF32 cy,sF32 aspect,sU64 col0,sU64 col1)
{
  if(aspect<=0.0f) aspect = 1.0f;

  for(sInt y=0;y<ys;y++)
  {
    for(sInt x=0;x<xs;x++)
    {
      sF32 dx = ((sF32(x)+0.5f)/xs - cx)*aspect;
      sF32 dy = (sF32(y)+0.5f)/ys - cy;
      sF32 r = sFSqrt(dx*dx+dy*dy);

      // the rings are circles; the turbulence is what stops them looking
      // like a target
      sF32 t = r*rings + turbulence*Turbulence(dx*8,dy*8,0,octaves,seed);
      t -= sRoundDown(t);
      sF32 v = t<0.5f ? t*2 : (1-t)*2;
      dst[x+y*xs] = Fade(col0,col1,v);
    }
  }
}

void Marble(sU64 *dst,sInt xs,sInt ys,sF32 freq,sF32 turbulence,sInt octaves,
            sInt seed,sF32 angle,sF32 gamma,sU64 col0,sU64 col1)
{
  if(gamma<=0.0f) gamma = 1.0f;
  sF32 ca = sFCos(angle*sPI2F);
  sF32 sa = sFSin(angle*sPI2F);

  for(sInt y=0;y<ys;y++)
  {
    for(sInt x=0;x<xs;x++)
    {
      sF32 fx = sF32(x)/xs;
      sF32 fy = sF32(y)/ys;
      sF32 p = fx*ca + fy*sa;

      sF32 t = p*freq + turbulence*Turbulence(fx*8,fy*8,0,octaves,seed);
      sF32 v = sFSin(t*sPI2F)*0.5f+0.5f;
      if(gamma!=1.0f) v = sFPow(v,gamma);
      dst[x+y*xs] = Fade(col0,col1,v);
    }
  }
}

// sample a list of (position,colour) stops, the way Gradient does
static sU64 SampleStops(const sF32 *pos,const sU64 *col,sInt count,sF32 t)
{
  if(count<=0) return 0;
  if(count==1) return col[0];
  if(t<=pos[0]) return col[0];
  if(t>=pos[count-1]) return col[count-1];
  for(sInt i=1;i<count;i++)
  {
    if(t<=pos[i])
    {
      sF32 span = pos[i]-pos[i-1];
      sF32 f = span>0.00001f ? (t-pos[i-1])/span : 0;
      return Fade(col[i-1],col[i],f);
    }
  }
  return col[count-1];
}

void ShapeGradient(sU64 *dst,sInt xs,sInt ys,sInt mode,sF32 cx,sF32 cy,
                   sF32 radius,sF32 angle,const sF32 *pos,const sU64 *col,
                   sInt count,sBool repeat)
{
  if(count<1) return;
  if(radius<=0.00001f) radius = 0.00001f;
  sF32 ca = sFCos(-angle*sPI2F);
  sF32 sa = sFSin(-angle*sPI2F);

  for(sInt y=0;y<ys;y++)
  {
    for(sInt x=0;x<xs;x++)
    {
      sF32 dx = (sF32(x)+0.5f)/xs - cx;
      sF32 dy = (sF32(y)+0.5f)/ys - cy;
      sF32 rx = dx*ca - dy*sa;
      sF32 ry = dx*sa + dy*ca;

      sF32 t;
      switch(mode)
      {
      default:
      case SHAPEGRAD_RADIAL:  t = sFSqrt(rx*rx+ry*ry)/radius; break;
      case SHAPEGRAD_ANGULAR: t = sFATan2(ry,rx)/sPI2F + 0.5f; break;
      case SHAPEGRAD_DIAMOND: t = (sAbs(rx)+sAbs(ry))/radius; break;
      case SHAPEGRAD_SQUARE:  t = sMax(sAbs(rx),sAbs(ry))/radius; break;
      }

      if(repeat)
      {
        t -= sRoundDown(t);
      }
      dst[x+y*xs] = SampleStops(pos,col,count,sClamp<sF32>(t,0,1));
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   filters, second batch                                              ***/
/***                                                                      ***/
/****************************************************************************/

void Threshold(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sInt channel,
               sF32 threshold,sF32 softness,sU64 col0,sU64 col1)
{
  sF32 lo = threshold - softness*0.5f;
  sF32 hi = threshold + softness*0.5f;
  if(hi-lo < 0.000001f) { lo = threshold-0.0000005f; hi = threshold+0.0000005f; }

  for(sInt i=0;i<xs*ys;i++)
  {
    Pixel p;
    p.Load(src[i]);
    sF32 c = sF32(p.Channel(channel))/MaxChannel;
    sF32 t = sClamp<sF32>((c-lo)/(hi-lo),0,1);
    t = t*t*(3-2*t);                       // smoothstep, so the edge is not a stair
    dst[i] = Fade(col0,col1,t);
  }
}

void Curve(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sInt channelmask,
           const sF32 *pos,const sF32 *value,sInt count)
{
  if(count<2) count = 0;

  // bake the curve into a table once, rather than walking the point list per
  // pixel per channel
  static const sInt LutSize = 1024;
  sInt *lut = new sInt[LutSize];
  for(sInt i=0;i<LutSize;i++)
  {
    sF32 t = sF32(i)/(LutSize-1);
    sF32 v = t;
    if(count>=2)
    {
      if(t<=pos[0]) v = value[0];
      else if(t>=pos[count-1]) v = value[count-1];
      else
      {
        for(sInt j=1;j<count;j++)
        {
          if(t<=pos[j])
          {
            sF32 span = pos[j]-pos[j-1];
            sF32 f = span>0.00001f ? (t-pos[j-1])/span : 0;
            v = value[j-1] + (value[j]-value[j-1])*f;
            break;
          }
        }
      }
    }
    lut[i] = sInt(sClamp<sF32>(v,0,1)*MaxChannel);
  }

  for(sInt i=0;i<xs*ys;i++)
  {
    Pixel p;
    p.Load(src[i]);
    sInt *ch[4] = { &p.r,&p.g,&p.b,&p.a };
    for(sInt c=0;c<4;c++)
      if(channelmask & (1<<c))
        *ch[c] = lut[sClamp<sInt>(*ch[c],0,MaxChannel)*(LutSize-1)/MaxChannel];
    dst[i] = p.Store();
  }

  delete[] lut;
}

void Swizzle(sU64 *dst,sInt xs,sInt ys,const sU64 *const *src,
             const sInt *channel,const sF32 *fill)
{
  for(sInt i=0;i<xs*ys;i++)
  {
    Pixel out;
    sInt *ch[4] = { &out.r,&out.g,&out.b,&out.a };
    for(sInt c=0;c<4;c++)
    {
      if(src[c])
      {
        Pixel p;
        p.Load(src[c][i]);
        *ch[c] = p.Channel(channel[c]);
      }
      else
      {
        *ch[c] = sInt(sClamp<sF32>(fill[c],0,1)*MaxChannel);
      }
    }
    dst[i] = out.Store();
  }
}

void Dither(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sInt levels,sInt mode,
            sF32 strength)
{
  levels = sClamp<sInt>(levels,2,256);
  sF32 step = sF32(MaxChannel)/(levels-1);
  strength = sClamp<sF32>(strength,0,1);

  static const sInt Bayer[8][8] =
  {
    { 0,32, 8,40, 2,34,10,42},{48,16,56,24,50,18,58,26},
    {12,44, 4,36,14,46, 6,38},{60,28,52,20,62,30,54,22},
    { 3,35,11,43, 1,33, 9,41},{51,19,59,27,49,17,57,25},
    {15,47, 7,39,13,45, 5,37},{63,31,55,23,61,29,53,21},
  };

  if(mode==DITHER_ORDERED)
  {
    for(sInt y=0;y<ys;y++)
    {
      for(sInt x=0;x<xs;x++)
      {
        // the threshold matrix decides which way each pixel rounds
        sF32 bias = (Bayer[y&7][x&7]/64.0f - 0.5f)*step*strength;
        Pixel p;
        p.Load(src[x+y*xs]);
        sInt *ch[4] = { &p.b,&p.g,&p.r,&p.a };
        for(sInt c=0;c<4;c++)
        {
          sF32 v = *ch[c] + bias;
          *ch[c] = sInt(sRoundDown(v/step+0.5f)*step);
        }
        dst[x+y*xs] = p.Store();
      }
    }
  }
  else
  {
    // floyd-steinberg: carry what was rounded away into the neighbours
    sF32 *err = new sF32[xs*ys*3];
    for(sInt i=0;i<xs*ys*3;i++) err[i] = 0;

    for(sInt y=0;y<ys;y++)
    {
      for(sInt x=0;x<xs;x++)
      {
        Pixel p;
        p.Load(src[x+y*xs]);
        sInt *ch[3] = { &p.b,&p.g,&p.r };
        for(sInt c=0;c<3;c++)
        {
          sF32 v = *ch[c] + err[(x+y*xs)*3+c]*strength;
          sF32 q = sRoundDown(v/step+0.5f)*step;
          sF32 e = v-q;
          *ch[c] = sInt(q);

          if(x+1<xs)              err[((x+1)+(y  )*xs)*3+c] += e*7/16;
          if(y+1<ys)
          {
            if(x>0)               err[((x-1)+(y+1)*xs)*3+c] += e*3/16;
                                  err[((x  )+(y+1)*xs)*3+c] += e*5/16;
            if(x+1<xs)            err[((x+1)+(y+1)*xs)*3+c] += e*1/16;
          }
        }
        dst[x+y*xs] = p.Store();
      }
    }
    delete[] err;
  }
}

void MotionBlur(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sF32 angle,
                sF32 length,sInt samples,sInt wrap)
{
  samples = sClamp<sInt>(samples,1,256);
  sF32 dx = sFCos(angle*sPI2F)*length;
  sF32 dy = sFSin(angle*sPI2F)*length;

  for(sInt y=0;y<ys;y++)
  {
    for(sInt x=0;x<xs;x++)
    {
      sF32 u = (sF32(x)+0.5f)/xs;
      sF32 v = (sF32(y)+0.5f)/ys;
      sInt ab=0,ag=0,ar=0,aa=0;

      for(sInt i=0;i<samples;i++)
      {
        sF32 f = samples>1 ? sF32(i)/(samples-1)-0.5f : 0;
        Pixel p;
        p.Load(SampleBilinear(src,xs,ys,u+dx*f,v+dy*f,wrap));
        ab += p.b; ag += p.g; ar += p.r; aa += p.a;
      }

      Pixel o;
      o.b = ab/samples; o.g = ag/samples; o.r = ar/samples; o.a = aa/samples;
      dst[x+y*xs] = o.Store();
    }
  }
}

void RadialBlur(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sF32 cx,sF32 cy,
                sF32 zoom,sF32 spin,sInt samples,sInt wrap)
{
  samples = sClamp<sInt>(samples,1,256);

  for(sInt y=0;y<ys;y++)
  {
    for(sInt x=0;x<xs;x++)
    {
      sF32 u = (sF32(x)+0.5f)/xs - cx;
      sF32 v = (sF32(y)+0.5f)/ys - cy;
      sInt ab=0,ag=0,ar=0,aa=0;

      for(sInt i=0;i<samples;i++)
      {
        sF32 f = samples>1 ? sF32(i)/(samples-1)-0.5f : 0;
        sF32 s = 1.0f + zoom*f;
        sF32 a = spin*f*sPI2F;
        sF32 ca = sFCos(a), sa = sFSin(a);

        sF32 su = (u*ca - v*sa)*s + cx;
        sF32 sv = (u*sa + v*ca)*s + cy;

        Pixel p;
        p.Load(SampleBilinear(src,xs,ys,su,sv,wrap));
        ab += p.b; ag += p.g; ar += p.r; aa += p.a;
      }

      Pixel o;
      o.b = ab/samples; o.g = ag/samples; o.r = ar/samples; o.a = aa/samples;
      dst[x+y*xs] = o.Store();
    }
  }
}

void ChromaticAberration(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sF32 cx,
                         sF32 cy,sF32 amount,sBool radial,sF32 angle,sInt wrap)
{
  sF32 ca = sFCos(angle*sPI2F);
  sF32 sa = sFSin(angle*sPI2F);

  for(sInt y=0;y<ys;y++)
  {
    for(sInt x=0;x<xs;x++)
    {
      sF32 u = (sF32(x)+0.5f)/xs;
      sF32 v = (sF32(y)+0.5f)/ys;

      sF32 ox,oy;
      if(radial)
      {
        // a lens spreads the channels along the line to the centre, and more
        // of it the further out you look
        ox = (u-cx)*amount;
        oy = (v-cy)*amount;
      }
      else
      {
        ox = ca*amount;
        oy = sa*amount;
      }

      Pixel pr,pg,pb,o;
      pr.Load(SampleBilinear(src,xs,ys,u+ox,v+oy,wrap));
      pg.Load(SampleBilinear(src,xs,ys,u,v,wrap));
      pb.Load(SampleBilinear(src,xs,ys,u-ox,v-oy,wrap));

      o.r = pr.r; o.g = pg.g; o.b = pb.b; o.a = pg.a;
      dst[x+y*xs] = o.Store();
    }
  }
}

void Seamless(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sF32 border,sBool mirror)
{
  border = sClamp<sF32>(border,0.001f,0.5f);

  for(sInt y=0;y<ys;y++)
  {
    for(sInt x=0;x<xs;x++)
    {
      // The trick is to blend the image with a copy of itself offset by half
      // its size. What lands on the edge then comes from the middle, which is
      // continuous across the wrap by construction.
      sInt sx = mirror ? (xs-1-x) : ((x+xs/2)%xs);
      sInt sy = mirror ? (ys-1-y) : ((y+ys/2)%ys);

      sF32 u = sF32(x)/(xs-1);
      sF32 v = sF32(y)/(ys-1);
      sF32 du = sMin(u,1.0f-u)/border;
      sF32 dv = sMin(v,1.0f-v)/border;
      sF32 wu = sClamp<sF32>(du,0,1);
      sF32 wv = sClamp<sF32>(dv,0,1);
      wu = wu*wu*(3-2*wu);
      wv = wv*wv*(3-2*wv);

      dst[x+y*xs] = Fade(src[sx+sy*xs],src[x+y*xs],wu*wv);
    }
  }
}

void Equalize(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sBool perchannel,
              sF32 amount)
{
  static const sInt Bins = 1024;
  sInt size = xs*ys;
  amount = sClamp<sF32>(amount,0,1);
  if(size<=0) return;

  sInt hist[4][Bins];
  sInt map[4][Bins];
  sInt channels = perchannel ? 3 : 1;

  for(sInt c=0;c<channels;c++)
    for(sInt i=0;i<Bins;i++)
      hist[c][i] = 0;

  for(sInt i=0;i<size;i++)
  {
    Pixel p;
    p.Load(src[i]);
    if(perchannel)
    {
      hist[0][p.r*(Bins-1)/MaxChannel]++;
      hist[1][p.g*(Bins-1)/MaxChannel]++;
      hist[2][p.b*(Bins-1)/MaxChannel]++;
    }
    else
    {
      hist[0][sClamp<sInt>(p.Channel(CHANNEL_LUMINANCE),0,MaxChannel)*(Bins-1)/MaxChannel]++;
    }
  }

  // the running total of the histogram is the mapping that flattens it
  for(sInt c=0;c<channels;c++)
  {
    sInt sum = 0;
    for(sInt i=0;i<Bins;i++)
    {
      sum += hist[c][i];
      map[c][i] = sInt(sF32(sum)/size*MaxChannel);
    }
  }

  for(sInt i=0;i<size;i++)
  {
    Pixel p,o;
    p.Load(src[i]);
    if(perchannel)
    {
      o.r = map[0][p.r*(Bins-1)/MaxChannel];
      o.g = map[1][p.g*(Bins-1)/MaxChannel];
      o.b = map[2][p.b*(Bins-1)/MaxChannel];
      o.a = p.a;
    }
    else
    {
      sInt lum = sClamp<sInt>(p.Channel(CHANNEL_LUMINANCE),1,MaxChannel);
      sInt eq = map[0][lum*(Bins-1)/MaxChannel];
      // scale the colour rather than replacing it, so hue survives
      o.r = p.r*eq/lum;
      o.g = p.g*eq/lum;
      o.b = p.b*eq/lum;
      o.a = p.a;
    }

    dst[i] = Fade(src[i],o.Store(),amount);
  }
}

void NormalCombine(sU64 *dst,const sU64 *a,const sU64 *b,sInt xs,sInt ys,
                   sF32 strength)
{
  for(sInt i=0;i<xs*ys;i++)
  {
    Pixel pa,pb,o;
    pa.Load(a[i]);
    pb.Load(b[i]);

    sF32 ax = sF32(pa.r)/MaxChannel*2-1;
    sF32 ay = sF32(pa.g)/MaxChannel*2-1;
    sF32 az = sF32(pa.b)/MaxChannel*2-1;
    sF32 bx = (sF32(pb.r)/MaxChannel*2-1)*strength;
    sF32 by = (sF32(pb.g)/MaxChannel*2-1)*strength;
    sF32 bz = sF32(pb.b)/MaxChannel*2-1;

    // whiteout blend: add the slopes, multiply what is left pointing up
    sF32 x = ax+bx;
    sF32 y = ay+by;
    sF32 z = az*bz;
    sF32 len = sFSqrt(x*x+y*y+z*z);
    if(len<0.00001f) { x = 0; y = 0; z = 1; len = 1; }
    x /= len; y /= len; z /= len;

    o.r = sInt((x*0.5f+0.5f)*MaxChannel);
    o.g = sInt((y*0.5f+0.5f)*MaxChannel);
    o.b = sInt((z*0.5f+0.5f)*MaxChannel);
    o.a = pa.a;
    dst[i] = o.Store();
  }
}

void AmbientOcclusion(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sF32 radius,
                      sInt samples,sF32 height,sF32 strength,sInt wrap)
{
  samples = sClamp<sInt>(samples,4,64);
  sInt steps = sClamp<sInt>(sInt(radius),1,64);
  if(radius<1.0f) radius = 1.0f;

  for(sInt y=0;y<ys;y++)
  {
    for(sInt x=0;x<xs;x++)
    {
      Pixel c;
      c.Load(src[x+y*xs]);
      sF32 h0 = sF32(c.Channel(CHANNEL_LUMINANCE))/MaxChannel*height;

      sF32 occ = 0;
      for(sInt s=0;s<samples;s++)
      {
        sF32 a = sF32(s)/samples*sPI2F;
        sF32 dx = sFCos(a), dy = sFSin(a);

        // walk outwards and remember the steepest thing in the way: that is
        // the horizon in this direction
        sF32 maxslope = 0;
        for(sInt t=1;t<=steps;t++)
        {
          sF32 dist = radius*t/steps;
          Pixel p;
          p.Load(Fetch(src,xs,ys,x+sInt(dx*dist),y+sInt(dy*dist),wrap));
          sF32 h = sF32(p.Channel(CHANNEL_LUMINANCE))/MaxChannel*height;
          sF32 slope = (h-h0)/dist;
          if(slope>maxslope) maxslope = slope;
        }
        occ += maxslope/sFSqrt(maxslope*maxslope+1);   // sin of the horizon angle
      }

      sF32 v = sClamp<sF32>(1.0f - occ/samples*strength,0,1);
      Pixel o;
      o.b = o.g = o.r = sInt(v*MaxChannel);
      o.a = MaxChannel;
      dst[x+y*xs] = o.Store();
    }
  }
}

/****************************************************************************/

}
