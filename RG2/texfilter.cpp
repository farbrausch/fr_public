// This code is in the public domain. See LICENSE for details.

#include "stdafx.h"
#include "texbase.h"
#include <math.h>
#include "exporter.h"
#include "debug.h"
#include "tstream.h"
#include "exportTool.h"

// ---- blur helper functions

static sInt gaussKernel[1027];

typedef sInt *(*filterKernelProc)(sF32 radius, sInt &n, sF32 sc);

static sInt *calcDummyKernel(sInt &n, sF32 sc=1.0f)
{
  n=3;
  gaussKernel[511]=0;
  gaussKernel[512]=sc*512.0f;
  gaussKernel[513]=0;
  gaussKernel[1026]=512;

  return gaussKernel+512;
}

static sInt *calcBoxKernel(sF32 radius, sInt &n, sF32 sc=1.0f)
{
  sInt  i, len, *out, div;

  if (radius<=1.0f)
    return calcDummyKernel(n, sc);

  n=ceil(radius)*2+1;
  len=n>>1;

  out=gaussKernel+512;
  out[0]=div=512;

  for (i=1; i<len-1; i++)
    div+=2*(out[-i]=out[i]=512);

  if (len>1)
    if (radius!=floor(radius))
      div+=2*(out[-len+1]=out[len-1]=512*(radius-floor(radius)));
    else
      div+=2*(out[-len+1]=out[len-1]=512);

  gaussKernel[1026]=div/sc;
  
  return out;
}

static sInt *calcTriKernel(sF32 radius, sInt &n, sF32 sc=1.0f)
{
  sInt  i, len, *out, div;
  
  if (radius<=1.0f)
    return calcDummyKernel(n, sc);
  
  n=ceil(radius)*2+1;
  len=n>>1;
  
  out=gaussKernel+512;
  out[0]=div=512;

  for (i=1; i<len; i++)
    div+=2*(out[-i]=out[i]=512.0f*(1.0f-i/radius));

  gaussKernel[1026]=div/sc;
  
  return out;
}

static sInt *calcGaussKernel(sF32 radius, sInt &n, sF32 sc=1.0f)
{
  sF32  sigma;
  sInt  i, len, *out, div;

  if (radius<=1.0f)
    return calcDummyKernel(n, sc);
  
  sigma=-2.0f/(radius*radius);
  n=ceil(radius)*2+1;
  len=n>>1;

  out=gaussKernel+512;
  out[0]=div=512;

  for (i=1; i<len; i++)
    div+=2*(out[-i]=out[i]=512.0f*exp(i*i*sigma));

  gaussKernel[1026]=div/sc;

  return out;
}

static filterKernelProc filters[]={calcBoxKernel, calcTriKernel, calcGaussKernel};

// ---- macht entweder einen horizontalen oder vertikalen blur

static void blur(frTexture *out, const frTexture *in, sInt len, sInt wrap, sInt dir)
{
  sInt  x, y, o, so;
  sInt  j, lh=len/2, dvi=(1<<24)/gaussKernel[1026];
  
  const sInt  rd = dir ? in->h    : in->w;
  const sInt  f  = dir ? in->w*4  : 4;
  
  __asm emms;
 
  for (y=0, o=0; y<in->h; y++)
  {
    for (x=0; x<in->w; x++, o+=4)
    {
      const sInt rc=dir?y:x;

      __asm pxor mm0, mm0;
      __asm pxor mm1, mm1;
      
      for (j=-lh+1; j<lh; j++)
      {

        if (((sUInt) (rc+j))<rd)
          so=j*f;

        else if (wrap)
        {
          switch (wrap)
          {
          case 1: // clamp
            if (rc+j<0)
              so=-rc*f;
            else
              so=(rd-1-rc)*f;
            break;
            
          case 2: // wrap
            so=(((rc+j+rd)&(rd-1))-rc)*f;
            break;
          }
        }
        else // black
          continue;
        
        so+=o;

        void  *data2  = &in->data[so];
        
        __asm
        {
          mov       eax, [j];
          mov       ebx, [data2];
          movd      mm2, gaussKernel[eax*4+512*4];
          punpcklwd mm2, mm2;
          punpcklwd mm2, mm2;
          movq      mm3, [ebx];
          movq      mm4, [ebx];
          pmullw    mm3, mm2;
          pmulhw    mm4, mm2;
          movq      mm5, mm3;
          movq      mm6, mm4;
          punpckhwd mm5, mm6;
          punpcklwd mm3, mm4;
          paddd     mm0, mm5;
          paddd     mm1, mm3;
        }
      }

      void  *out2=&out->data[o];
      __asm
      {
        mov   esi, 07fffh;
        mov   edi, [out2];
        mov   ebx, [dvi];
        movd  eax, mm1;
        xor   edx, edx;
        mul   ebx;
        shrd  eax, edx, 24;
        psrlq mm1, 32;
        cmp   eax, esi;
        cmova eax, esi;
        mov   ecx, eax;
        movd  eax, mm1;
        xor   edx, edx;
        mul   ebx;
        shrd  eax, edx, 24;
        cmp   eax, esi;
        cmova eax, esi;
        shl   eax, 16;
        xor   edx, edx;
        or    ecx, eax;
        movd  eax, mm0;
        psrlq mm0, 32;
        mul   ebx;
        shrd  eax, edx, 24;
        mov   [edi], ecx;
        cmp   eax, esi;
        cmova eax, esi;
        mov   ecx, eax;
        xor   edx, edx;
        movd  eax, mm0;
        mul   ebx;
        shrd  eax, edx, 24;
        cmp   eax, esi;
        cmova eax, esi;
        shl   eax, 16;
        or    ecx, eax;
        mov   [edi+4], ecx;
      }
    }
  }

  __asm emms;
}

// ---- blur filter

class frTFBlur: public frTexturePlugin
{
  sBool doProcess()
  {
    sInt  *kern, len;
    frTexture *src2, *dst1;
    sBool blurred;
    
    frTexture *temp=new frTexture(data->w, data->h);
    temp->lock();

    if (params[0].tfloatv.a<=1.0f && params[1].floatv==1.0f)
      src2=getInputData(0);
    else
      src2=temp;

    if (params[0].tfloatv.b<=1.0f && params[1].floatv==1.0f)
      dst1=data;
    else
      dst1=temp;

    blurred=sFALSE;

    if (params[1].floatv>0.0f)
    {
      if (params[0].tfloatv.a>1.0f || params[1].floatv!=1.0f)
      {
        kern=filters[params[2].selectv.sel](params[0].tfloatv.a, len, params[1].floatv);
        blur(dst1, getInputData(0), len, params[3].selectv.sel, 0);
        blurred=sTRUE;
      }

      if (params[0].tfloatv.b>1.0f || params[1].floatv!=1.0f)
      {
        kern=filters[params[2].selectv.sel](params[0].tfloatv.b, len, params[1].floatv);
        blur(data, src2, len, params[3].selectv.sel, 1);
        blurred=sTRUE;
      }

      if (!blurred)
        data->copy(*getInputData(0));
    }
    else
      memset(data->data, 0, data->w*data->h*4*sizeof(sU16));

    temp->unlock();
    delete temp;
    
    return sTRUE;
  }
  
public:
  frTFBlur(const frPluginDef* d)
    : frTexturePlugin(d)
  {
    addTwoFloatParam("Blur Radius", 3.0f, 3.0f, 1.0f, 128.0f, 0.01f, 3);
    addFloatParam("Intensity", 1.0f, 0.0f, 255.0f, 0.025f, 2);
    addSelectParam("Filter", "box|triangle|gaussian", 2);
    addSelectParam("Border Pixels", "black|clamp|wrap", 0);
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver=2;

    strm << ver;

    if (ver==1)
    {
      strm << params[0].tfloatv.a << params[1].floatv;
      strm << params[2].selectv << params[3].selectv;
      params[0].tfloatv.b=1.0f; // böser bug(-fix)
    }
    else
    {
      strm << params[0].tfloatv << params[1].floatv;
      strm << params[2].selectv << params[3].selectv;
    }
  }
  
  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    f.putsU8(params[2].selectv.sel);
    f.putsU16(params[1].floatv*256);
    f.putsU8(params[3].selectv.sel);
    f.putsU16((params[0].tfloatv.a-1)*512);
    f.putsU16((params[0].tfloatv.b-1)*512);
    
    // 8 bytes data+1 byte id+1 byte src+1 byte dest=11 bytes
  }
};

TG2_DECLARE_PLUGIN(frTFBlur, "Blur", 0, 1, 1, sTRUE, sFALSE, 6);

// ---- make normal map filter

static inline sF32 getGrayscale(const sU16 *ptr)
{
	const sS16* pt = (const sS16*) ptr;
	return pt[0] * 0.11f / 32767.0f + pt[1] * 0.59f / 32767.0f + pt[2] * 0.30f / 32767.0f;
}

static void makeNormalMap(sU16 *dst, const sU16 *src, sInt w, sInt h, sF32 hScale)
{
  sInt  x, y, xn, yn;
  sF32  nx, ny, nz;

  for (y=0; y<h; y++)
  {
    yn=y+1;
    if (yn>=h)
      yn=0;

    for (x=0; x<w; x++)
    {
      xn=x+1;
      if (xn>=w)
        xn=0;

      nz=getGrayscale(src+(y*w+x)*4);
      dst[3]=nz*32767.0f;
      nx=(nz - getGrayscale(src+(y*w+xn)*4))*hScale;
      ny=(nz - getGrayscale(src+(yn*w+x)*4))*hScale;
      nz=16383.0f/sqrt(nx*nx+ny*ny+1.0f);
      dst[0]=nz+16384;
      dst[1]=ny*nz+16384;
      dst[2]=nx*nz+16384;
      dst+=4;
    }
  }
}

class frTFNormalMap: public frTexturePlugin
{
  sBool doProcess()
  {
    makeNormalMap(data->data, getInputData(0)->data, data->w, data->h, params[0].floatv);
    
    return sTRUE;
  }
  
public:
  frTFNormalMap(const frPluginDef* d)
    : frTexturePlugin(d)
  {
    addFloatParam("Bumpyness", 1.0f, 0.01f, 63.0f, 0.01f, 2);
  }
  
  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver = 2;
    
    strm << ver;
    
    if (ver >= 1 && ver <= 2)
    {
      if (ver == 1)
      {
        sInt hdet;

        strm << hdet;

        if (hdet)
          fr::debugOut("Make Normal Map: Non-grayscale 'Height detection' isn't supported anymore\n");
      }

      strm << params[0].floatv;
    }
  }
  
  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    f.putsU16(params[0].floatv*1024);
    
    // 3 bytes data+1 byte id+1 byte src+1 byte dst=6 bytes
  }
};

TG2_DECLARE_PLUGIN(frTFNormalMap, "Mk Normal Map", 0, 1, 1, sTRUE, sFALSE, 7);

// ---- rotozoomer

class frTFRotoZoom: public frTexturePlugin
{
  sBool doProcess()
  {
    sU16  *srcPtr=getInputData(0)->data;
    sU16  *outPtr=data->data;
    sS32  xp, yp, yxp, yyp, xsx, ysx, xsy, ysy, scx, scy;
    sF32  ang;

    ang=params[0].floatv*3.1415926535f/180.0f;
    scx=65536.0f*params[1].tfloatv.a;
    scy=65536.0f*params[1].tfloatv.b;

    xsx= scx*cos(ang); ysx=scy*sin(ang); 
    xsy=-scx*sin(ang); ysy=scy*cos(ang);

    xp=-(params[2].tfloatv.a+params[3].pointv.x)*data->w*xsx-(params[2].tfloatv.b+params[3].pointv.y)*data->h*xsy;
    yp=-(params[2].tfloatv.a+params[3].pointv.x)*data->w*ysx-(params[2].tfloatv.b+params[3].pointv.y)*data->h*ysy;

    xp+=params[3].pointv.x*data->w*65536.0f;
    yp+=params[3].pointv.y*data->h*65536.0f;
      
    for (sInt i=0; i<data->h; i++)
    {
      yxp=xp;
      yyp=yp;

      for (sInt j=0; j<data->w; j++)
      {
        const sU32 rxp=(xp>>16) & (data->w-1), rxpn=(rxp+1) & (data->w-1);
        sU32 ryp=(yp>>16) & (data->h-1);
        const sU16 *pt1=srcPtr+((ryp*data->w)+rxp)*4;
        const sU16 *pt2=srcPtr+((ryp*data->w)+rxpn)*4;
        ryp=(ryp+1)&(data->h-1);
        const sU16 *pt3=srcPtr+((ryp*data->w)+rxp)*4;
        const sU16 *pt4=srcPtr+((ryp*data->w)+rxpn)*4;

        for (sInt k=0; k<4; k++)
        {
          const sInt xw=(xp>>8)&0xff;
          const sU32 t1=((sU32) pt1[k]<<8)+xw*(pt2[k]-pt1[k]);
          const sU32 t2=((sU32) pt3[k]<<8)+xw*(pt4[k]-pt3[k]);
          const sU32 t3=(t1<<8)+((yp&0xff00)>>8)*((sS32) (t2-t1));

          outPtr[k]=t3>>16;
        }

        xp+=xsx;
        yp+=ysx;
        outPtr+=4;
      }

      xp=yxp+xsy;
      yp=yyp+ysy;
    }
    
    return sTRUE;
  }
  
public:
  frTFRotoZoom(const frPluginDef* d)
    : frTexturePlugin(d)
  {
    addFloatParam("Angle", 0.0f, -10000.0f, 10000.0f, 1.0f, 2);
    addTwoFloatParam("Zoom", 1.0f, 1.0f, -255.0f, 255.0f, 0.01f, 6);
    addTwoFloatParam("Scroll", 0.0f, 0.0f, -100.0f, 100.0f, 0.01f, 6);
    addPointParam("Center", 0.5f, 0.5f, sFALSE);
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver = 3;

    strm << ver;

    strm << params[0].floatv;
    
    if (ver < 3)
    {
      sF32 x;
      strm << x;
      params[1].tfloatv.a = params[1].tfloatv.b = x;
    }
    else
      strm << params[1].tfloatv;

    if (ver == 1)
    {
      strm << params[2].tfloatv.a << params[2].tfloatv.b;
      params[3].pointv.x = params[3].pointv.y = 0.0f;
    }
    else if (ver >= 2)
      strm << params[2].tfloatv << params[3].pointv;
  }

  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    sU16 angtemp = fmod(params[0].floatv + 36000.0f, 360.0f) * 65536.0f / 360.0f;

    f.putsU16(angtemp);

    sF32 ang = angtemp * 3.1415926535f / 32768.0f;

    sF32 zoomtemp1 = params[1].tfloatv.a * 65536.0f, zoomtemp2 = params[1].tfloatv.b * 65536.0f;
    sF32 scrollx = (params[2].tfloatv.a + params[3].pointv.x) * data->w;
    sF32 scrolly = (params[2].tfloatv.b + params[3].pointv.y) * data->h;
    sF32 centerx = params[3].pointv.x * data->w * 65536.0f;
    sF32 centery = params[3].pointv.y * data->h * 65536.0f;

    sF32 xsx =  zoomtemp1 * cos(ang), ysx = zoomtemp2 * sin(ang);
    sF32 xsy = -zoomtemp1 * sin(ang), ysy = zoomtemp2 * cos(ang);

    putPackedFloat(f, zoomtemp1);
    putPackedFloat(f, zoomtemp2);

    f.putsS32(centerx -scrollx * xsx - scrolly * xsy);
    f.putsS32(centery -scrollx * ysx - scrolly * ysy);

    // 13 bytes data+1 byte id+1 byte src+1 byte dst=16 bytes
  }
};

TG2_DECLARE_PLUGIN(frTFRotoZoom, "Rotozoomer", 0, 1, 1, sTRUE, sFALSE, 8);

// ---- color adjust filter

class frTFAdjust: public frTexturePlugin
{
  static inline sInt conBright(sInt v, sS32 cont, sS32 bright)
  {
    v=((v*cont)>>12)+bright;
    v=(v<0)?0:(v>32767)?32767:v;

    return v;
  }

  static inline sInt imd16(sInt a, sInt b)
  {
    __asm
    {
      mov   eax, [a];
      mov   edx, [a];
      shl   eax, 16;
      sar   edx, 16;
      idiv  [b];
      mov   [a], eax;
    }

    return a;
  }

  sBool doProcess()
  {
    sU16  *pts, *ptd;
    sU32  cnt;
    sS32  cont, brgt;

    pts=getInputData(0)->data;
    ptd=data->data;
    cnt=data->w*data->h;

    cont=params[0].floatv*4096;
    brgt=params[1].floatv*32767+16384-4*cont;

    sS32 hue=fmod(params[2].floatv+1440.0f, 360.0f)*6.0f*65536.0f/360.0f;
    sS32 sat=params[3].floatv*32767;

    while (cnt--)
    {
      // rgb->hsb
      sS32 max=pts[0], min=pts[0];

      if (pts[1]<min)
        min=pts[1];
      
      if (pts[1]>max)
        max=pts[1];

      if (pts[2]<min)
        min=pts[2];

      if (pts[2]>max)
        max=pts[2];

      // calculate and adjust saturation
      sInt s=max-min;

      if (sat<0)
        s=(s*(sat+32768))>>15;
      else
        s+=((max-s)*sat)>>15;

      // calculate and shift hue, then convert back
      if (s) // not really correct - should be (max-min)
      {
        sS32 h, d;

        if (pts[2]==max)
          h=3, d=pts[1]-pts[0];
        else if (pts[1]==max)
          h=1, d=pts[0]-pts[2];
        else if (pts[0]==max)
          h=2, d=pts[2]-pts[1];

        h=(h<<17)+hue+imd16(d, max-min+1);

        const sInt ih=h>>16;
        const sS32 fh=h & 0xffff;

        sS32 vals[4];
        vals[0]=0;
        vals[1]=s;
        vals[2]=(s*fh)>>16;
        vals[3]=(s*(65536-fh))>>16;

        static const sU8 st[]={ 1, 1, 3, 0, 0, 2, 1, 1, 3, 0, 0, 2, 1, 1, 3, 0, 0, 2 };

        ptd[0]=conBright(max-vals[st[ih]], cont, brgt);
        ptd[1]=conBright(max-vals[st[ih+2]], cont, brgt);
        ptd[2]=conBright(max-vals[st[ih+4]], cont, brgt);
      }
      else // just stupid grayscale convert back to rgb.
      {
        const sInt x=conBright(max, cont, brgt);

        ptd[0]=x;
        ptd[1]=x;
        ptd[2]=x;
      }
      
      ptd[3]=conBright(pts[3], cont, brgt); // contrast+brightness alpha (but no color shift)
      
      pts+=4;
      ptd+=4;
    }
    
    return sTRUE;
  }
  
public:
  frTFAdjust(const frPluginDef* d)
    : frTexturePlugin(d)
  {
    addFloatParam("Contrast", 1.0f, -15.0f, 15.0f, 0.01f, 3);
    addFloatParam("Brightness", 0.0f, -1.0f, 1.0f, 0.005f, 4);
    addFloatParam("Hue", 0.0f, -1440.0f, 1440.0f, 1.0f, 2);
    addFloatParam("Saturation", 0.0f, -1.0f, 1.0f, 0.01f, 3);
  }
  
  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver=1;
    
    strm << ver;
    
    if (ver==1)
    {
      strm << params[0].floatv << params[1].floatv << params[2].floatv << params[3].floatv;
    }
  }
  
  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    f.putsS16(params[0].floatv*2048);
    f.putsS16(params[1].floatv*32767);
    f.putsU16(fmod(params[2].floatv+1440.0f, 360.0f)*65536.0f/360.0f);
    f.putsS16(params[3].floatv*32767);
    // 8 bytes data+1 byte id+1 byte src+1 byte dst=11 bytes
  }
};

TG2_DECLARE_PLUGIN(frTFAdjust, "Adjust", 0, 1, 1, sTRUE, sFALSE, 9);

// blur v2

class frTFBlurV2: public frTexturePlugin
{
  static void inplaceBlur(sU16* dest, const sU16* srci, sU32 w, sU32 h, sInt radius, sInt wrap, sInt steps, sBool vert)
  {
    static sU16 pbf[(1024 + 256) * 4 * 2];

    if (2 * radius >= w)
      radius = (w - 1) / 2;

    sU32 sc = 65536 / (2 * radius + 1);
		sU16* pbuf = pbf;
		sU16* pbuf2 = pbf + (1024 + 256) * 4;

    if (wrap == 0)
    {
      memset(pbuf, 0, radius * 4 * sizeof(sU16));
      memset(pbuf + (w + radius) * 4, 0, radius * 4 * sizeof(sU16));
			memset(pbuf2, 0, radius * 4 * sizeof(sU16));
			memset(pbuf2 + (w + radius) * 4, 0, radius * 4 * sizeof(sU16));
    }

    __asm emms;

		sInt hc = h;

    while (hc--)
    {
			{
				sU16* dst = pbuf + radius * 4;
				const sU32 delta = vert ? h * 8 : 8;

				__asm
				{
					mov			esi, [srci];
					mov			edi, [dst];
					mov			ecx, [w];
					mov			edx, [delta];
					lea     edi, [edi + ecx * 8];
					neg			ecx;
					imul		ebx, edx, 11;

copylpin:
					prefetchnta	[esi + ebx];
					movq		mm0, [esi];
					movq		[edi + ecx * 8], mm0;
					add			esi, edx;
					inc			ecx;
					jnz			copylpin;
				}
			}

			for (sInt j = steps; radius && j >= 0; --j)
			{
				if (wrap == 1) // clamp
				{
					for (sInt i = 0; i < radius * 4; i++)
					{
						const sInt ia = i & 3;

						pbuf[i] = pbuf[radius * 4 + ia];
						pbuf[i + (w + radius) * 4] = pbuf[(w + radius) * 4 - 4 + ia];
					}
				}
				else if (wrap == 2) // wrap
				{
					memcpy(pbuf, pbuf + w * 4, radius * 4 * sizeof(sU16));
					memcpy(pbuf + (w + radius) * 4, pbuf + radius*4, radius * 4 * sizeof(sU16));
				}

				const sU16* src = pbuf + radius * 4;
				const sU16* dst = pbuf2 + radius * 4;

				__asm
				{
					// pre-sum loop setup
					mov       esi, [pbuf];
					pxor      mm0, mm0;
					pxor      mm1, mm1;
					pxor      mm3, mm3;
					mov       ecx, [radius];
					shl       ecx, 1;

					// pre-sum loop
sumlp:
					movq      mm4, [esi];
					movq      mm5, mm4;
					punpcklwd mm4, mm3;
					punpckhwd mm5, mm3;
					add       esi, 8;
					paddd     mm0, mm4;
					dec       ecx;
					paddd     mm1, mm5;
					jns       sumlp;

					// inner loop setup
					mov       esi, [src];
					mov       edi, [dst];
					mov       ecx, [w];
					mov       ebx, [radius];
					shl       ebx, 3;
					lea       esi, [esi + ecx * 8];
					lea       edi, [edi + ecx * 8];
					lea       edx, [esi + ebx + 8];
					sub       esi, ebx;
					neg       ecx;

					movd      mm2, [sc]; // load scale reg
					punpcklwd mm2, mm2;
					punpcklwd mm2, mm2;

					// inner loop
pixlp:
					pcmpeqb		mm6, mm6;
					pcmpeqb		mm7, mm7;
					movq      mm4, mm0;
					movq      mm5, mm1;
					psrld			mm6, 17;
					psrld			mm7, 17;

					psrad     mm4, 15;
					psrad     mm5, 15;
					pand			mm6, mm0;
					pand			mm7, mm1;
					packssdw	mm4, mm5;
					packssdw	mm6, mm7;

					pmullw		mm4, mm2;
					movq      mm5, [edx + ecx * 8]; // add val
					pmulhw		mm6, mm2;
					psrlw			mm4, 1;
					movq			mm7, mm5;
					paddw			mm4, mm6;
					
					punpcklwd	mm5, mm3;
					movq      mm6, [esi + ecx * 8]; // sub val
					punpckhwd	mm7, mm3;

					movq      [edi + ecx * 8], mm4;
					movq			mm4, mm6;
					
					paddd			mm0, mm5;
					punpcklwd	mm6, mm3;
					paddd			mm1, mm7;
					punpckhwd	mm4, mm3;
					psubd			mm0, mm6;
					psubd			mm1, mm4;

					inc       ecx;
					jnz       pixlp;
				}

				// swap source and destination buffers
				sU16* tmp = pbuf;
				pbuf = pbuf2;
				pbuf2 = tmp;
			}

			const sU16* src = pbuf + radius * 4;
			const sU32 delta = vert ? h * 8 : 8;

			__asm
			{
				mov					esi, [src];
				mov					edi, [dest];
				mov					ecx, [w];
				mov					edx, [delta];
				lea					esi, [esi + ecx * 8];
				neg					ecx;

copylp:
				prefetchnta	[esi + ecx * 8 + 128];
				movq				mm0, [esi + ecx * 8];
				movntq			[edi], mm0;
				add					edi, edx;
				inc					ecx;
				jnz					copylp;
			}

			const sU32 step = vert ? 4 : w * 4;

			dest += step;
			srci += step;
    }

    __asm emms;
  }

  static void scaleIntensity(sU16* data, const sU16* srci, sUInt count, sU16 sf)
  {
    while (count--)
    {
      sU32 v = (*srci++ * sf) >> 8;
      if (v > 32767)
        v = 32767;

      *data++ = v;
    }
  }

  sBool doProcess()
  {
    const sInt rx = params[0].tfloatv.a - 1, ry = params[0].tfloatv.b - 1;
    const sF32 intens = params[1].floatv;
    const sInt steps = params[2].selectv.sel;
    const sInt wrap = params[3].selectv.sel;
    sU16 *dataPtr = data->data;
		const sU16* src = getInputData(0)->data;

    if (rx)
		{
      inplaceBlur(dataPtr, src, data->w, data->h, rx, wrap, steps, sFALSE);
			src = dataPtr;
		}

    if (ry)
		{
			inplaceBlur(dataPtr, src, data->h, data->w, ry, wrap, steps, sTRUE);
			src = dataPtr;
		}

    if (intens != 1.0f)
      scaleIntensity(dataPtr, src, data->w * data->h * 4, intens * 256);
		else if (src != dataPtr)
			memcpy(dataPtr, src, data->w * data->h * 8);

    return sTRUE;
  }

public:
  frTFBlurV2(const frPluginDef* d)
    : frTexturePlugin(d)
  {
    addTwoFloatParam("Radius", 3.0f, 3.0f, 1.0f, 128.0f, 0.2f, 0);
    addFloatParam("Intensity", 1.0f, 0.01f, 255.0f, 0.01f, 3);
    addSelectParam("Mode", "box|tri|gauss", 1);
    addSelectParam("Border Pixels", "black|clamp|wrap", 2);

		params[0].animIndex = 1; // radius
		params[1].animIndex = 2; // intensity
  }

  void serialize(fr::streamWrapper& strm)
  {
    sU16 ver = 3;
    strm << ver;

    if (ver >= 1 && ver <= 3)
    {
      strm << params[0].tfloatv;
      strm << params[2].selectv;

      if (ver >= 2)
        strm << params[3].selectv;
      else
        params[3].selectv.sel = 0; // black

      if (ver >= 3)
        strm << params[1].floatv;
      else
        params[1].floatv = 1.0f;
    }
  }

  void exportTo(fr::stream& f, const frGraphExporter& exp)
  {
    f.putsU8(params[0].tfloatv.a - 1);
    f.putsU8(params[0].tfloatv.b - 1);
    f.putsU8(params[2].selectv.sel | (params[3].selectv.sel << 2));
    f.putsU16(params[1].floatv * 256.0f);
  }

	void setAnim(sInt index, const sF32* vals)
	{
		switch (index)
		{
		case 1: // radius
			params[0].tfloatv.a = vals[0] + 0.5f;
			params[0].tfloatv.b = vals[1] + 0.5f;
			break;

		case 2: // intensity
			params[1].floatv = vals[0];
			break;
		}

		dirty = sTRUE;
	}
};

TG2_DECLARE_PLUGIN(frTFBlurV2, "Blur v2", 0, 1, 1, sTRUE, sFALSE, 22);

class frTFInstallFont: public frTexturePlugin
{
	fr::string fontFile;

	sBool doProcess()
	{
		if (fontFile != params[0].stringv)
		{
			if (fontFile.getLength())
				RemoveFontResource(fontFile);

			fontFile = params[0].stringv;
			if (!AddFontResource(fontFile))
				fontFile.empty();
		}

		memcpy(data->data, getInputData(0)->data, data->w*data->h*4*sizeof(sU16));

		return sTRUE;
	}

public:
	frTFInstallFont(const frPluginDef* d)
		: frTexturePlugin(d)
	{
		addStringParam("File name", 256, "");
	}

	~frTFInstallFont()
	{
		if (fontFile.getLength())
			RemoveFontResource(fontFile);
	}

	void serialize(fr::streamWrapper& strm)
	{
		sU16 ver = 1;
		
		strm << ver;

		if (ver == 1)
			strm << params[0].stringv;
	}

	void exportTo(fr::stream& f, const frGraphExporter& exp)
	{
		fr::streamF input;
		
		if (input.open(params[0].stringv))
		{
			putSmallInt(f, input.size());
			f.copy(input);
		}
		else
			putSmallInt(f, 0);
	}
};

TG2_DECLARE_PLUGIN(frTFInstallFont, "Install Font", 0, 1, 1, sTRUE, sTRUE, 24);
