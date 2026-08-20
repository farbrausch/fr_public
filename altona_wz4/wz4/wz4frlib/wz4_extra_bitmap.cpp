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

}
