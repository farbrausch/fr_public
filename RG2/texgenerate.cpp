// This code is in the public domain. See LICENSE for details.

#include "stdafx.h"
#include "texbase.h"
#include "debug.h"
#include "tstream.h"
#include "exportTool.h"
#include <math.h>

// -------- ops

static sU32 frColorConv(frColor &r)
{
  return ((sU32) r.r<<16)|((sU32) r.g<<8)|(sU32) r.b;
}

// ---- crystal

static sInt fixMul16(sInt a,sInt b)
{
  __asm
  {
    mov   eax, [a];
    imul  [b];
    shrd  eax, edx, 16;
  }
}

class frTGCrystal: public frTexturePlugin
{
  sBool doProcess()
  {
    sInt cells[256][3],maxdist=0;
    sU32 seed=params[0].intv;
    sU32 count=params[1].intv;
    sU32 width=data->w, height=data->h;
    sInt stepx,stepy;
    sU16 *dptr=data->data;

    for(stepx=1;width*stepx<1024;stepx*=2);
    for(stepy=1;height*stepy<1024;stepy*=2);

    seed = seed * 0x8af6f2ac + 0x1757286;

    // randomize cell positions
    for(sInt i=0;i<count*3;i++)
    {
      cells[0][i] = (seed >> 10) & 1023;
      seed = (seed * 0x15a4e35) + 1;
    }

    // calc nearest points
    for(sInt y=0;y<1024;y+=stepy)
    {
      /* if you want to trade speed for size, use this instead of the next loop
         and remove the disty[i]<best check in the "find closest point" loop
      // calc squared y distances for this row
      for(sInt i=0;i<count;i++)
      {
        sInt dy = ((cells[i][1] - y) & 1023) - 512;
        cells[i][2] = dy*dy;
      }*/

      // if you want it even faster, do this:
      for(sInt i=0;i<count;i++)
      {
        sInt px = cells[i][0];
        sInt py = cells[i][1];
        sInt dy = ((py - y) & 1023) - 512;
        sInt dist = dy*dy;

        // insertion sort (this is a good idea because the list is always "almost sorted")
        sInt j=i;
        while(j>0 && cells[j-1][2] > dist)
        {
          cells[j][0] = cells[j-1][0];
          cells[j][1] = cells[j-1][1];
          cells[j][2] = cells[j-1][2];
          j--;
        }

        cells[j][0] = px;
        cells[j][1] = py;
        cells[j][2] = dist;
      }

      for(sInt x=0;x<1024;x+=stepx)
      {
        sInt best = 2048 * 2048, besti = -1;

        // find closest point
        for(sInt i=0;i<count && cells[i][2]<best;i++)
        {
          sInt dx = ((cells[i][0] - x) & 1023) - 512;
          sInt dist = dx*dx + cells[i][2];

          if(dist<best)
          {
            best = dist;
            besti = i;
          }
        }

        // update max distance, store
        if(best > maxdist)
          maxdist = best;

        *((sU32 *) dptr) = best;
        dptr += 4;
      }
    }

    // contrast adjust, color ramp (you could easily do this with mmx if you wanted)
    sInt sr = params[2].colorv.r << 7;
    sInt sg = params[2].colorv.g << 7;
    sInt sb = params[2].colorv.b << 7;
    sInt dr = params[3].colorv.r - params[2].colorv.r;
    sInt dg = params[3].colorv.g - params[2].colorv.g;
    sInt db = params[3].colorv.b - params[2].colorv.b;
    sU32 rescale = (maxdist > 1) ? (1U << 31) / maxdist : (1U << 31) - 1;

    sU32 *sptr = (sU32 *) (data->data + width*height*2);
    dptr = data->data;
    for(sInt s=0;s<width*height;s++)
    {
      sInt fade = fixMul16(*((sU32 *) dptr),rescale);

      dptr[0] = sb + ((db * fade) >> 8);
      dptr[1] = sg + ((dg * fade) >> 8);
      dptr[2] = sr + ((dr * fade) >> 8);
      dptr[3] = 0;
      dptr += 4;
    }

    /*sU8   randBuf[512];
    sU32  seed=params[0].intv;
    sU32  count=params[1].intv;
    sU32  width=data->w, height=data->h;
    sU16  *dptr=data->data;

    static const sU32 _MMMax=0x7fffffff;
    const sU32 _MM256=(height<<16)|width, temp=(_MM256>>1) & (0x7fff7fff);
    const sU64 _MM128=(temp<<32)|temp;
    const sU64 _MMMask=((height-1)<<16)|(width-1);

    const sU32 col1=*((sU32 *) &params[2].colorv.b) & 0xffffff;
    const sU32 col2=*((sU32 *) &params[3].colorv.b) & 0xffffff;

    __asm
    {
      mov       edx, [seed];
      mov       esi, [dptr];

      mov       ecx, 511;
randomloop:
      imul      edx, 015a4e35h;
      inc       edx;
      mov       eax, edx;
      shr       eax, 16;
      mov       randBuf[ecx], al;
      dec       ecx;
      jns       randomloop;

      emms;
      pxor      mm7, mm7;
      xor       ecx, ecx;
      xor       edx, edx;

yloop:
      and       ecx, 0ffff0000h;

xloop:
      mov       edi, [count];
      dec       edi
      movd      mm2, ecx;
      movd      mm0, [_MMMax];
      movd      mm6, [_MM256];
      movq      mm5, mm2;
      xor       ebx, ebx;
      dec       ebx;

      push      esi;

tryloop:
      movd      mm1, randBuf[edi*2];
      movq      mm2, mm5;
      pand      mm1, [_MMMask];
      psubw     mm2, mm1;
      movq      mm1, mm0;
      movq      mm3, mm2;
      psraw     mm3, 15;
      pxor      mm2, mm3;
      psubsw    mm2, mm3;
      movq      mm3, mm2;
      pcmpgtw   mm3, [_MM128];
      movq      mm4, mm6;
      psubw     mm4, mm2;
      pand      mm4, mm3;
      pandn     mm3, mm2;
      por       mm3, mm4;
      pmaddwd   mm3, mm3;
      pcmpgtd   mm0, mm3;
      movd      eax, mm3;
      pand      mm3, mm0;
      pandn     mm0, mm1;
      por       mm0, mm3;
      cmp       eax, ebx;
      cmovb     ebx, eax;
      cmovb     esi, edi;
      dec       edi;
      jns       tryloop;

      mov       ebx, esi;
      pop       esi;

      movd      eax, mm0;
      cmp       eax, edx;
      cmova     edx, eax;
      mov       [esi], eax;
      add       esi, 8;
      inc       ecx;
      movzx     eax, cx;
      cmp       eax, [width];
      jne       xloop;

      add       ecx, 010000h;
      mov       eax, ecx;
      shr       eax, 16;
      cmp       eax, [height];
      jne       yloop;

      mov       ecx, [width];
      imul      ecx, [height];
      mov       [width], ecx;
      mov       ecx, 0ffffh;
      
      mov       edi, edx;
      inc       edi;

      movd      mm0, [col1];
      movd      mm1, [col2];
      punpcklbw mm0, mm7;
      punpcklbw mm1, mm7;
      psllw     mm0, 7;
      psllw     mm1, 7;
      psubw     mm1, mm0;

contrastloop:
      sub       esi, 8;
      mov       eax, [esi];
      mov       edx, eax;
      shl       eax, 16;
      shr       edx, 16;
      div       edi;
      xor       eax, -1;
      add       eax, ecx;
      mul       eax;
      shrd      eax, edx, 16;
      xor       eax, -1;
      add       eax, ecx;
      shr       eax, 1;
      movd      mm2, eax;
      punpcklwd mm2, mm2;
      punpcklwd mm2, mm2;
      pmulhw    mm2, mm1;
      psllw     mm2, 1;
      paddw     mm2, mm0;
      movq      [esi], mm2;
      
      mov       [esi+6], ax;
      dec       [width];
      jnz       contrastloop;
      
      emms;
    }*/
    
    return sTRUE;
  }

public:
  frTGCrystal(const frPluginDef* d)
    : frTexturePlugin(d)
  {
    addIntParam("Seed", 3456, 0, 65535, 1);
    addIntParam("Count", 42, 1, 255, 1);    
    addColorParam("Color 1", 0x000000);
    addColorParam("Color 2", 0xffffff);
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver=1;

    strm << ver;

    if (ver>=1)
    {
      strm << params[0].intv << params[1].intv;
      strm << params[2].colorv << params[3].colorv;
    }
  }
  
  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    f.putsU16(params[0].intv);
    f.putsU8(params[1].intv);
    f.putsU8(params[2].colorv.b);
    f.putsU8(params[2].colorv.g);
    f.putsU8(params[2].colorv.r);
    f.putsU8(params[3].colorv.b);
    f.putsU8(params[3].colorv.g);
    f.putsU8(params[3].colorv.r);
  }
};

TG2_DECLARE_PLUGIN(frTGCrystal, "Crystal", 0, 0, 0, sTRUE, sFALSE, 21);

// ---- gradient

class frTGGradient: public frTexturePlugin
{
  sBool doProcess()
  {
    const sInt type = params[0].selectv.sel;
    sF32 sc, expon, isc, xc, yc, xd, yd;

    sS32 r1 = params[1].colorv.r, g1 = params[1].colorv.g, b1 = params[1].colorv.b;
    sS32 r2 = params[2].colorv.r - r1, g2 = params[2].colorv.g - g1, b2 = params[2].colorv.b - b1;

    if (type==0) // linear
    {
      xc = params[3].pointv.x;      yc = params[3].pointv.y;
      xd = params[4].pointv.x - xc; yd = params[4].pointv.y - yc;
      sc = 1.0f/(xd*xd+yd*yd);
      xd *= sc;
      yd *= sc;
    }
    else if (type==1) // glow
    {
      sc = (2.0f / params[5].floatv) * (2.0f / params[5].floatv);
      expon = params[6].floatv;
      isc = params[7].floatv;
      xc = params[8].pointv.x;
      yc = params[8].pointv.y;
    }

    sU16 *ptr = data->data;
    const sF32 iw = 1.0f / data->w, ih = 1.0f / data->h;

    for (sInt y = 0; y < data->h; y++)
    {
      const sF32 p = y * 1.0f * ih - yc, pp = p * p;

      for (sInt x = 0; x < data->w; x++)
      {
        sF32 f;
        sF32 n = x * 1.0f * iw - xc;
        
        if (type == 0) // linear
          f = n * xd + p * yd;
        else if (type == 1) // glow
        {
          f = 1.0f - sqrt((n * n + pp) * sc);
          f = pow(f < 0.0f ? 0.0f : f > 1.0f ? 1.0f : f, expon) * isc;
        }

        f = f < 0.0f ? 0.0f : f > 1.0f ? 1.0f : f;
        sS32 fd = f * 65536;

        *ptr++ = ((b1 << 16) + b2 * fd) >> 9;
        *ptr++ = ((g1 << 16) + g2 * fd) >> 9;
        *ptr++ = ((r1 << 16) + r2 * fd) >> 9;
        *ptr++ = 0;
      }
    }
    
    return sTRUE;
  }

  sBool onParamChanged(sInt param)
  {
    return param == 0; // update if type was changed
  }

public:
  frTGGradient(const frPluginDef* d)
    : frTexturePlugin(d)
  {
    addSelectParam("Type", "Linear|Glow", 0);
    addColorParam("Background", 0x000000);
    addColorParam("Foreground", 0xffffff);
    addPointParam("Point 1", 0.2f, 0.2f, sFALSE);
    addPointParam("Point 2", 0.8f, 0.8f, sFALSE);
    addFloatParam("Scale", 1.0f, 0.01f, 5.1f, 0.02f, 2);
    addFloatParam("Exponent", 1.0f, 0.01f, 8191.0f, 0.01f, 3);
    addFloatParam("Intensity", 1.0f, 0.0f, 255.0f, 0.02f, 3);
    addPointParam("Center", 0.5f, 0.5f, sFALSE);

		params[3].animIndex = 1; // point 1
		params[4].animIndex = 2; // point 2
		params[8].animIndex = 3; // center
  }

  ~frTGGradient()
  {
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver = 2;

    strm << ver;

    // old version (import)
    if (ver == 1)
    {
      strm << params[0].selectv;
      strm << params[2].colorv << params[1].colorv;
      strm << params[5].floatv << params[6].floatv << params[7].floatv;
      strm << params[8].pointv;
      strm << params[3].pointv << params[4].pointv;
    }

    // new version
    if (ver == 2)
    {
      strm << params[0].selectv;
      strm << params[1].colorv << params[2].colorv;
      strm << params[3].pointv << params[4].pointv;
      strm << params[5].floatv << params[6].floatv << params[7].floatv;
      strm << params[8].pointv;
    }
  }

  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    sInt type = params[0].selectv.sel | (colorClass(params[1].colorv) << 1) | (colorClass(params[2].colorv) << 3);
    f.putsU8(type);

    putColor(f, params[1].colorv);
    putColor(f, params[2].colorv);
    
    if (params[0].selectv.sel == 0) // linear
    {
      f.putsS16(params[3].pointv.x * 256);
      f.putsS16(params[3].pointv.y * 256);
      f.putsS16(params[4].pointv.x * 256);
      f.putsS16(params[4].pointv.y * 256);
    }
    else // glow
    {
      f.putsU8(params[5].floatv * 50.0f);
      putSmallInt(f,params[6].floatv * 8.0f);
      f.putsU16(params[7].floatv * 256.0f);
      f.putsS16(params[8].pointv.x * 256);
      f.putsS16(params[8].pointv.y * 256);
    }
  }

  sBool displayParameter(sU32 index)
  {
    switch (index)
    {
    case 0: case 1: case 2:
      return sTRUE;

    case 3: case 4:
      return params[0].selectv.sel == 0;

    case 5: case 6: case 7: case 8:
      return params[0].selectv.sel == 1;
    }

    return sFALSE;
  }

	void setAnim(sInt index, const sF32* vals)
	{
		switch (index)
		{
		case 1:
			params[3].pointv.x = vals[0];
			params[3].pointv.y = vals[1];
			break;

		case 2:
			params[4].pointv.x = vals[0];
			params[4].pointv.y = vals[1];
			break;

		case 3:
			params[8].pointv.x = vals[0];
			params[8].pointv.y = vals[1];
			break;
		}

		dirty = sTRUE;
	}
};

TG2_DECLARE_PLUGIN(frTGGradient, "Gradient/Glow", 0, 0, 0, sTRUE, sFALSE, 19);

// ---- perlin noise

static sU32 srandom(sU32 n)
{
  return (n*(n*n*15731+789221)+1376312589) & 0x7fffffff;
}

class frTGPerlinNoise: public frTexturePlugin
{
  sU32  seed;
  sU16  wtab[512];

  static sU32 __forceinline srandom2(sU32 n)
  {
    __asm
    {
      mov   ecx, [n];
      mov   ebx, ecx;
      shl   ebx, 13;
      xor   ecx, ebx;
      mov   ebx, ecx;

      mov   eax, 15731;
      imul  ebx, ebx;
      imul  ebx, eax;
      add   ebx, 789221;
      imul  ecx, ebx;
      add   ecx, 1376312589;
      mov   eax, 2048;
      and   ecx, 07fffffffh;
      shr   ecx, 19;
      sub   eax, ecx;
      mov   [n], eax;
    }

    return n;
  }

  sS32 __forceinline inoise(sU32 x, sU32 y, sInt msk)
  {
    sInt ix=x>>10, iy=y>>10;
    sInt inx=(ix+1) & msk, iny=(iy+1) & msk;
    
    ix+=seed;
    inx+=seed;
    iy*=31337;
    iny*=31337;
    
    sInt fx=wtab[(x>>1) & 511];

    sS32 t1=srandom2(ix+iy);
    t1=(t1<<10)+(srandom2(inx+iy)-t1)*fx;
    sS32 t2=srandom2(ix+iny);
    t2=(t2<<10)+(srandom2(inx+iny)-t2)*fx;

    return ((t1<<10)+(t2-t1)*wtab[(y>>1) & 511])>>20;
  }

  static sS32 imul16(sS32 a, sS32 b)
  {
    __asm
    {
      mov   eax, [a];
      imul  [b];
      shrd  eax, edx, 16;
      mov   [a], eax;
    }

    return a;
  }

  sBool doProcess()
  {
    sU16  *out=data->data;
    sInt  x, y, o, oct, f, frq;
    sU32  amp, sc;
    sS32  sum, xv, yv;
    sS32  fx, fy, prs;

    const sInt so=params[7].intv;
    const sInt oseed=(params[3].intv+1)*0xb37fa184+303*31337+so*0xdeadbeef;

    frq=1<<params[2].intv;
    prs=params[1].floatv*65536;
    fx=1024*frq/data->w;
    fy=1024*frq/data->h;

    oct=params[0].intv;

    sum=0;
    amp=32768;
    
    sU32 samp;
    
    for (o=0; o<oct; o++)
    {
      if (o==so)
      {
        sum+=amp;
        samp=amp;
      }
      else if (o>so)
        sum+=amp;
      
      amp=(amp*prs)>>16;
    }

    sc=params[4].floatv*65536.0f*16.0f/sum;

    sInt r1=params[5].colorv.r<<7, g1=params[5].colorv.g<<7, b1=params[5].colorv.b<<7;
    sInt r2=(params[6].colorv.r<<7)-r1, g2=(params[6].colorv.g<<7)-g1, b2=(params[6].colorv.b<<7)-b1;

    fx<<=so;
    fy<<=so;
    sInt fb=frq<<so;
    
    for (y=0; y<data->h; y++)
    {
      for (x=0; x<data->w; x++)
      {
        amp=samp;
        sum=0;
        xv=x*fx;
        yv=y*fy;
        f=fb;
        seed=oseed;        

        for (o=so; o<oct; o++)
        {
          sum+=amp*inoise(xv, yv, f-1);
          xv*=2;
          yv*=2;
          f<<=1;
          seed+=0xdeadbeef;
          amp=(amp*prs)>>16;
        }

        sS32 sm=imul16(sum, sc)+32768;
        sm=(sm<0)?0:(sm>65536)?65536:sm;

        *out++=b1+((sm*b2)>>16);
        *out++=g1+((sm*g2)>>16);
        *out++=r1+((sm*r2)>>16);
        *out++=0;
      }
    }

    return sTRUE;
  }

  void calcWTab()
  {
    for (sInt i=0; i<512; i++)
    {
      const sF32 val=i/512.0f;
      wtab[i]=1024.0f*val*val*val*(10.0f+val*(val*6.0f-15.0f));
    }
  }

public:
  frTGPerlinNoise(const frPluginDef* d)
    : frTexturePlugin(d)
  {
    addIntParam("Octaves", 7, 1, 12, 1);
    addFloatParam("Persistence", 0.5f, 0.0f, 1.0f, 0.01f, 3);
    addIntParam("Scale", 2, 0, 10, 1);
    addIntParam("Random seed", 2584, 0, 65535, 1);
    addFloatParam("Mix contrast", 1.0f, 0.0f, 63.0f, 0.01f, 3);
    addColorParam("Color 1", 0x000000);
    addColorParam("Color 2", 0xffffff);
    addIntParam("Start Octave", 0, 0, 12, 1);

    calcWTab();
  }
  
  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver=2; // ver 2

    strm << ver;
    strm << params[0].intv << params[1].floatv;
    strm << params[2].intv << params[3].intv;
    strm << params[4].floatv;
    strm << params[5].colorv;
    strm << params[6].colorv;
    
    if (ver==1)
      params[7].intv=0;
    else if (ver==2)
      strm << params[7].intv;
  }

  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    f.putsU8(params[0].intv | (params[2].intv << 4));
    f.putsU8(params[1].floatv*255.0f);
    f.putsU16(params[3].intv);
    f.putsU8(params[7].intv | (colorClass(params[5].colorv) << 4) | (colorClass(params[6].colorv) << 6));
    putSmallInt(f,params[4].floatv*127.0f);
    putColor(f,params[5].colorv);
    putColor(f,params[6].colorv);

    // 15 bytes data+1 byte id+1 byte dest buffer=17 bytes
  }
};

TG2_DECLARE_PLUGIN(frTGPerlinNoise, "Perlin Noise", 0, 0, 0, sTRUE, sFALSE, 15);

// ---- frTGRandomDots

static void frColorFill(sU16 *pt, sU32 clr, sU32 cnt)
{
  __asm {
    mov       edi, [pt]
    mov       ecx, [cnt]
    dec       ecx
    mov       eax, [clr]
    movd      mm0, eax
    pxor      mm1, mm1
    punpcklbw mm0, mm1
    psllw     mm0, 7
lp:
    movq      [edi+ecx*8], mm0
    dec       ecx
    jns       lp
    emms
  }
}

class frTGRandomDots: public frTexturePlugin
{
  sBool doProcess()
  {
    sU16  *ptr, *o;
    sInt  i, x, y;
    sU32  seed;

    ptr=data->data;
    frColorFill(ptr, frColorConv(params[2].colorv), data->w*data->h);
    
    seed=(params[1].intv+303)*0x31337303;
    seed^=seed>>5;
    
    for (i=0; i<params[0].intv; i++)
    {
      x=(seed>>16)&(data->w-1); seed=(seed*375375621+1);
      y=(seed>>16)&(data->h-1); seed=(seed*375375621+1);

      o=ptr+(y*data->w+x)*4;
      o[0]=(sU16) params[3+(i&1)].colorv.b<<7;
      o[1]=(sU16) params[3+(i&1)].colorv.g<<7;
      o[2]=(sU16) params[3+(i&1)].colorv.r<<7;
    }

    return sTRUE;
  }

public:
  frTGRandomDots(const frPluginDef* d)
    : frTexturePlugin(d)
  {
    addIntParam("Dots", 100, 1, 65000, 10);
    addIntParam("Random seed", 1234, 0, 65535, 13);
    addColorParam("Background color", 0x000000);
    addColorParam("Dot color 1", 0xffffff);
    addColorParam("Dot color 2", 0xffffff);
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver=1;

    strm << ver;

    if (ver==1)
    {
      strm << params[0].intv << params[1].intv;
      strm << params[2].colorv << params[3].colorv << params[4].colorv;
    }
  }

  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    f.putsU16(params[0].intv);
    f.putsU16(params[1].intv);
    f.putsU8(params[2].colorv.b);
    f.putsU8(params[2].colorv.g);
    f.putsU8(params[2].colorv.r);
    f.putsU8(params[3].colorv.b);
    f.putsU8(params[3].colorv.g);
    f.putsU8(params[3].colorv.r);
    f.putsU8(params[4].colorv.b);
    f.putsU8(params[4].colorv.g);
    f.putsU8(params[4].colorv.r);

    // 22 bytes data+1 byte id+1 byte dest buffer=24 bytes
  }
};

TG2_DECLARE_PLUGIN(frTGRandomDots, "Random dots", 0, 0, 0, sTRUE, sFALSE, 0);

// ---- frTGRectangle

class frTGRectangle: public frTexturePlugin
{
  sBool doProcess()
  {
    sU16  *ptr;
    sInt  y;

    ptr=data->data;
    frColorFill(ptr, frColorConv(params[2].colorv), data->w*data->h);

    if (params[0].pointv.x > params[1].pointv.x)
      return sTRUE;

    sInt x1 = params[0].pointv.x * data->w;
    sInt y1 = params[0].pointv.y * data->h;
    sInt x2 = params[1].pointv.x * data->w;
    sInt y2 = params[1].pointv.y * data->h;
    
    for (y=y1; y<y2; y++)
      frColorFill(ptr+(y*data->w+x1)*4, frColorConv(params[3].colorv),
        x2-x1);

    return sTRUE;
  }

public:
  frTGRectangle(const frPluginDef* d)
    : frTexturePlugin(d)
  {
    addPointParam("Left/Top", 0.0f, 0.0f, sTRUE);
    addPointParam("Right/Bottom", 0.5f, 0.5f, sTRUE);
    addColorParam("Background color", 0x000000);
    addColorParam("Fill color", 0xffffff);
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver=1;

    strm << ver;

    if (ver==1)
    {
      strm << params[0].pointv << params[1].pointv;
      strm << params[2].colorv << params[3].colorv;
    }
  }

  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    f.putsU8(params[0].pointv.x*255);
    f.putsU8(params[1].pointv.x*255);
    f.putsU8(params[0].pointv.y*255);
    f.putsU8(params[1].pointv.y*255);
    f.putsU8(colorClass(params[2].colorv) | (colorClass(params[3].colorv) << 2));
    putColor(f,params[2].colorv);
    putColor(f,params[3].colorv);
    
    // 14 bytes data+1 byte id+1 byte dest buffer=16 bytes
  }
};

TG2_DECLARE_PLUGIN(frTGRectangle, "Rectangle", 0, 0, 0, sTRUE, sFALSE, 1);

// ---- frTGSolid

class frTGSolid: public frTexturePlugin
{
  sBool doProcess()
  {
    frColorFill(data->data, frColorConv(params[0].colorv), data->w*data->h);
    
    return sTRUE;
  }
  
public:
  frTGSolid(const frPluginDef* d)
    : frTexturePlugin(d)
  {
    addColorParam("Fill color", 0xffffff);
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver=1;

    strm << ver;

    if (ver==1)
    {
      strm << params[0].colorv;
    }
  }

  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    f.putsU8(params[0].colorv.b);
    f.putsU8(params[0].colorv.g);
    f.putsU8(params[0].colorv.r);
    // 3 bytes data+1 byte id
  }
};

TG2_DECLARE_PLUGIN(frTGSolid, "Solid", 0, 0, 0, sTRUE, sFALSE, 2);

// ---- frTGText

class frTGText: public frTexturePlugin
{
  sBool doProcess()
  {
    HWND              hWndDesktop;
    HDC               hDC, tempDC;
    HFONT             hFont, oldFont, oldFont2;
    HBITMAP           hBitmap, oldBitmap;
    HBRUSH            hBrush;
    sU8               *buf;

    hWndDesktop=GetDesktopWindow();
    tempDC=GetDC(hWndDesktop);
    hDC=CreateCompatibleDC(tempDC);
    
    BITMAPINFOHEADER bih={ sizeof(BITMAPINFOHEADER), 0, 0, 1, 32, 0, 0, 0, 0, 0, 0 };
    bih.biWidth=data->w;
    bih.biHeight=-data->h;
    
    hBitmap=CreateDIBSection(tempDC, (BITMAPINFO *) &bih, DIB_RGB_COLORS, (void **) &buf, 0, 0);

    sInt height=params[1].floatv*data->h;
    const sInt style=params[6].selectv.sel;

    hFont=CreateFont(height, 0, 0, 0, (style&1)?FW_BOLD:FW_NORMAL, style&2, 0, 0, DEFAULT_CHARSET,
      OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH|FF_DONTCARE,
      params[3].stringv);
    
    hBrush=CreateSolidBrush(params[5].colorv.r|(params[5].colorv.g<<8)|(params[5].colorv.b<<16));

    oldBitmap=(HBITMAP) SelectObject(hDC, hBitmap);
    oldFont2=(HFONT) SelectObject(tempDC, hFont);
    oldFont=(HFONT) SelectObject(hDC, hFont);

    RECT rc;
    rc.left=rc.top=0;
    rc.right=data->w;
    rc.bottom=data->h;
    FillRect(hDC, &rc, hBrush);
    
    SetTextColor(hDC, params[4].colorv.r|(params[4].colorv.g<<8)|(params[4].colorv.b<<16));
    SetBkMode(hDC, TRANSPARENT);

    sInt xp = params[0].pointv.x * data->w, yp = params[0].pointv.y * data->h, sp = 0, sl, sa;

    const sChar *sptr=params[2].stringv;

    while (sptr[sp])
    {
      sa=sl=0;
      while (1)
      {
        if (sptr[sp+sl]=='\\')
        {
          sa=1;
          break;
        }
        else
          if (!sptr[sp+sl])
            break;

        sl++;
      }

      TextOut(hDC, xp, yp, sptr+sp, sl);
      sp+=sl+sa;
      yp+=height;
    }

    SelectObject(hDC, oldBitmap);
    SelectObject(hDC, oldFont);
    
    sU16 *pt=data->data;
    sU32 cnt=data->w*data->h;

    static sU64 mask=0xffffff00ffffff00;

    GdiFlush();

    __asm {
      emms;
      mov       esi, [buf];
      mov       edi, [pt];
      mov       ecx, [cnt];
      sub       ecx, 4;
      pxor      mm1, mm1;
      movq      mm3, [mask];
      pxor      mm5, mm5;
      pxor      mm7, mm7;
lp:
      movd      mm0, [esi+ecx*4];
      movd      mm2, [esi+ecx*4+4];
      punpcklbw mm0, mm1;
      movd      mm4, [esi+ecx*4+8];
      punpcklbw mm2, mm5;
      movd      mm6, [esi+ecx*4+12];
      pxor      mm1, mm1;
      punpcklbw mm4, mm7;
      psllw     mm0, 7;
      punpcklbw mm6, mm1;
      pand      mm0, mm3;
      psllw     mm2, 7;
      movq      [edi+ecx*8], mm0;
      pand      mm2, mm3;
      psllw     mm4, 7;
      movq      [edi+ecx*8+8], mm2;
      pand      mm4, mm3;
      psllw     mm6, 7;
      movq      [edi+ecx*8+16], mm4;
      pand      mm6, mm3;
      movq      [edi+ecx*8+24], mm6;
      sub       ecx, 4;
      jns       lp;
      emms;
    };

    DeleteObject(hBitmap);
    DeleteObject(hFont);
    DeleteObject(hBrush);
    DeleteDC(hDC);
    
    SelectObject(tempDC, oldFont2);
    ReleaseDC(hWndDesktop, tempDC);
    
    return sTRUE;
  }
  
public:
  frTGText(const frPluginDef* d)
    : frTexturePlugin(d)
  {
    addPointParam("Position", 0, 0, sTRUE);
    addFloatParam("Size", 0.1f, 0, 0.99f, 0.005f, 3);
    addStringParam("Text", 511, "Blah");
    addStringParam("Font", 31, "Arial");
    addColorParam("Text color", 0xffffff);
    addColorParam("Background color", 0x000000);
    addSelectParam("Style", "Regular|Bold|Italic|Bold+Italic", 0);
  }
  
  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver=4;
    
    strm << ver;
    
    if (ver>=1 && ver<=4)
    {
      if (ver <= 3)
      {
        sInt x, y;
        strm << x << y;

        params[0].pointv.x = 1.0f * x / data->w;
        params[0].pointv.y = 1.0f * y / data->h;
      }
      else
        strm << params[0].pointv;

      strm << params[1].floatv;
      strm << params[2].stringv << params[3].stringv;
      strm << params[4].colorv << params[5].colorv;
    }
    
    if (ver<=2)
      params[6].selectv.sel=0;
    else
      strm << params[6].selectv;
  }
  
  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    f.putsU16(params[0].pointv.x * data->w);
    f.putsU16(params[0].pointv.y * data->h);
    f.putsU8(params[1].floatv*256);
    f.putsU8(params[6].selectv.sel);
    f.puts(params[2].stringv); f.putsChar(0);
    f.puts(params[3].stringv); f.putsChar(0);
    f.putsU8(params[4].colorv.r);
    f.putsU8(params[4].colorv.g);
    f.putsU8(params[4].colorv.b);
    f.putsU8(params[5].colorv.r);
    f.putsU8(params[5].colorv.g);
    f.putsU8(params[5].colorv.b);
    
    // 14+m+n bytes data, 1 byte id, 1 byte dest buffer=16+m+n bytes
  }
};

TG2_DECLARE_PLUGIN(frTGText, "Text", 0, 0, 0, sTRUE, sFALSE, 3);

// ---- Font

class frTGFont: public frTexturePlugin
{
  sBool doProcess()
  {
    HWND hWndDesktop = GetDesktopWindow();
    HDC tempDC = GetDC(hWndDesktop);
    HDC hDC = CreateCompatibleDC(tempDC);
    
    static BITMAPINFOHEADER bih = { sizeof(BITMAPINFOHEADER), 0, 0, 1, 32, 0, 0, 0, 0, 0, 0 };
    bih.biWidth = data->w;
    bih.biHeight = -data->h;

		sU8* buf;
    HBITMAP hBitmap = CreateDIBSection(tempDC, (BITMAPINFO *) &bih, DIB_RGB_COLORS, (void **) &buf, 0, 0);
    const sInt style = params[3].selectv.sel;

		HFONT hFont = CreateFont(params[0].floatv, 0, 0, 0, (style & 1) ? FW_BOLD : FW_NORMAL,
			style & 2, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, params[2].stringv);
    
    HGDIOBJ oldBitmap = SelectObject(hDC, hBitmap);
    HGDIOBJ oldFont2 = SelectObject(tempDC, hFont);
    HGDIOBJ oldFont = SelectObject(hDC, hFont);

    SetTextColor(hDC, 0xffffff);
    SetBkMode(hDC, TRANSPARENT);

		const sInt border = params[1].floatv;

		for (sInt i = 0; i < 96; i++)
		{
			sInt ch = i + 32;

			ABC abc;
			GetCharABCWidths(hDC, ch, ch, &abc);

			const sInt x = (i % 10) * (data->w / 10) + border - abc.abcA;
			const sInt y = (i / 10) * (data->h / 10) + border;

			TextOut(hDC, x, y, (LPCSTR) &ch, 1);
		}

    SelectObject(hDC, oldBitmap);
    SelectObject(hDC, oldFont);
    
    sU16* pt = data->data;
    sU32 cnt = data->w * data->h;

    static const sU64 mask = 0xffffff00ffffff00;

    GdiFlush();

    __asm {
      emms;
      mov       esi, [buf];
      mov       edi, [pt];
      mov       ecx, [cnt];
      sub       ecx, 4;
      pxor      mm1, mm1;
      movq      mm3, [mask];
      pxor      mm5, mm5;
      pxor      mm7, mm7;
lp:
      movd      mm0, [esi+ecx*4];
      movd      mm2, [esi+ecx*4+4];
      punpcklbw mm0, mm1;
      movd      mm4, [esi+ecx*4+8];
      punpcklbw mm2, mm5;
      movd      mm6, [esi+ecx*4+12];
      pxor      mm1, mm1;
      punpcklbw mm4, mm7;
      psllw     mm0, 7;
      punpcklbw mm6, mm1;
      pand      mm0, mm3;
      psllw     mm2, 7;
      movq      [edi+ecx*8], mm0;
      pand      mm2, mm3;
      psllw     mm4, 7;
      movq      [edi+ecx*8+8], mm2;
      pand      mm4, mm3;
      psllw     mm6, 7;
      movq      [edi+ecx*8+16], mm4;
      pand      mm6, mm3;
      movq      [edi+ecx*8+24], mm6;
      sub       ecx, 4;
      jns       lp;
      emms;
    };

    DeleteObject(hBitmap);
    DeleteObject(hFont);
    DeleteDC(hDC);
    
    SelectObject(tempDC, oldFont2);
    ReleaseDC(hWndDesktop, tempDC);
    
    return sTRUE;
  }
  
public:
  frTGFont(const frPluginDef* d)
    : frTexturePlugin(d)
  {
		addFloatParam("Size", 16.0f, 0.0f, 1024.0f, 0.1f, 0);
		addFloatParam("Border", 0.0f, 0.0f, 1024.0f, 0.1f, 0);
		addStringParam("Font", 31, "Arial");
		addSelectParam("Style", "Regular|Bold|Italic|Bold+Italic", 0);
		addFloatParam("Font page", 0, 0, 3, 0.1f, 0);
  }
  
  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver = 1;

		strm << ver;

		if (ver == 1)
		{
			strm << params[0].floatv << params[1].floatv;
			strm << params[2].stringv;
			strm << params[3].selectv.sel;
		}
  }
  
  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
		// ficken.
  }
};

TG2_DECLARE_PLUGIN(frTGFont, "Font", 0, 0, 0, sTRUE, sFALSE, 23);
