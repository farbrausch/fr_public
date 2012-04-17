// This code is in the public domain. See LICENSE for details.

#include "stdafx.h"
#include "texbase.h"
#include "debug.h"
#include "exporter.h"
#include "tstream.h"
#include <math.h>

typedef sF32 (*getHeightProc)(const sU16 *ptr);

static sF32 getGrayscale(const sU16 *src)
{
  return (src[0] * 0.11f + src[1] * 0.59f + src[2] * 0.30f) / 32767.0f;
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

// ---- mk alpha

class frTCAlpha: public frTexturePlugin
{
  sBool doProcess()
  {
    sU16 *in1, *in2, *out;
    sInt i, post;

    post=params[0].selectv.sel;

    in1=getInputData(0)->data;
    in2=getInputData(1)->data;
    out=data->data;

    for (i=0; i<data->w*data->h*4; i+=4)
    {
      sU32 alpha = (7209 * in2[i + 0] + 38666 * in2[i + 1] + 19661 * in2[i + 2]) >> 16;

      if (post==1) // invert
        alpha^=32767;

      out[i+0]=in1[i+0];
      out[i+1]=in1[i+1];
      out[i+2]=in1[i+2];
      out[i+3]=alpha;
    }

    return sTRUE;
  }

public:
  frTCAlpha(const frPluginDef* d)
    : frTexturePlugin(d)
  {
    addSelectParam("Post process", "None|Invert", 0);
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver = 2;

    strm << ver;

    if (ver >= 1 && ver <= 2)
    {
      if (ver == 1)
      {
        sInt adet;

        strm << adet;
        if (adet != 0)
          fr::debugOut("Set Alpha: Non-grayscale 'Alpha detection' isn't supported anymore.\n");
      }

      strm << params[0].selectv.sel;
    }
  }
  
  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    f.putsU8(params[0].selectv.sel);
  }
};

TG2_DECLARE_PLUGIN(frTCAlpha, "Set Alpha", 0, 2, 2, sTRUE, sFALSE, 17);

// ---- blend

class frTCBlend: public frTexturePlugin
{
  sBool doProcess()
  {
    sU16 *in1, *in2, *in3, *out;
    sInt i, post;
    
    post=params[0].selectv.sel;
    
    in1=getInputData(0)->data;
    in2=getInputData(1)->data;
    in3=getInputData(2)->data;
    out=data->data;
    
    for (i=0; i<data->w*data->h*4; i+=4)
    {
      sU32 alpha = (7209 * in3[i + 0] + 38666 * in3[i + 1] + 19661 * in3[i + 2]) >> 16;
      if (post==1) // invert
        alpha^=32767;
      
      out[i+0]=in1[i+0]+((((sS32) in2[i+0]-in1[i+0])*alpha)>>15);
      out[i+1]=in1[i+1]+((((sS32) in2[i+1]-in1[i+1])*alpha)>>15);
      out[i+2]=in1[i+2]+((((sS32) in2[i+2]-in1[i+2])*alpha)>>15);
      out[i+3]=in1[i+3]+((((sS32) in2[i+3]-in1[i+3])*alpha)>>15);
    }
    
    return sTRUE;
  }
  
public:
  frTCBlend(const frPluginDef* d)
    : frTexturePlugin(d)
  {
    addSelectParam("Alpha process", "None|Invert", 0);
  }
  
  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver = 2;
    
    strm << ver;
    
    if (ver >= 1 && ver <= 2)
    {
      if (ver == 1)
      {
        sInt adet;

        strm << adet;

        if (adet)
          fr::debugOut("Blend: Non-grayscale 'Alpha detection' is not supported anymore\n");
      }

      strm << params[0].selectv.sel;
    }
  }
  
  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    f.putsU8(params[0].selectv.sel);
  }
};

TG2_DECLARE_PLUGIN(frTCBlend, "Blend", 0, 3, 3, sTRUE, sFALSE, 18);

// ---- frTCAdd

class frTCAdd: public frTexturePlugin
{
  sF32    gains[255];
  sChar   texDescs[255][8];
  
  sBool onConnectUpdate()
  {
    sInt i, cnt;

    for (i=0, cnt=0; i<def->nInputs; i++)
    {
      if (getInID(i))
        cnt++;
    }

    clearParams();

    for (i=0; i<cnt; i++)
    {
      sprintf(texDescs[i], "Gain %d", i+1);
      addFloatParam(texDescs[i], gains[i], -1.0f, 1.0f, 0.01f, 3);
    }

    return sTRUE;
  }

  sBool onParamChanged(sInt param)
  {
    if (param<255)
      gains[param]=params[param].floatv;

    return sFALSE;
  }

  sBool doProcess()
  {
    sU16  *ptd, *pt;
    sInt  i, cnt;
    sInt  gain;

    cnt=data->w*data->h-1;
    ptd=data->data;

    memset(ptd, 0, (cnt+1)*sizeof(sU16)*4);

    for (i=0; i<def->nInputs; i++)
    {
      if (!getInputData(i))
        continue;
      
      pt=getInputData(i)->data;
      gain=gains[i]*32767.0f;
      
      if (gain>=0)
      {
        __asm {
          mov       esi, [pt];
          mov       edi, [ptd];
          mov       ecx, [cnt];
          movd      mm2, [gain];
          punpcklwd mm2, mm2;
          punpcklwd mm2, mm2;
        lp1:
          movq      mm0, [edi+ecx*8];
          movq      mm1, [esi+ecx*8];
          pmulhw    mm1, mm2;
          psllw     mm1, 1;
          paddsw    mm0, mm1;
          movq      [edi+ecx*8], mm0;
          dec       ecx;
          jns       lp1;
          emms;
        }
      }
      else
      {
        gain=-gain;
        __asm {
          mov       esi, [pt];
          mov       edi, [ptd];
          mov       ecx, [cnt];
          movd      mm2, [gain];
          punpcklwd mm2, mm2;
          punpcklwd mm2, mm2;
        lp2:
          movq      mm0, [edi+ecx*8];
          movq      mm1, [esi+ecx*8];
          pmulhw    mm1, mm2;
          psllw     mm1, 1;
          psubusw   mm0, mm1;
          movq      [edi+ecx*8], mm0;
          dec       ecx;
          jns       lp2;
          emms;
        }
      }
    }

    return sTRUE;
  }

public:
  frTCAdd(const frPluginDef* d)
    : frTexturePlugin(d)
  {
    for (sInt i=0; i<255; i++)
      gains[i]=1.0f;
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver=2;

    strm << ver;

    if (ver==1)
    {
      for (sInt i=0; i<16; i++)
        strm << gains[i];
    }
    else if (ver==2)
    {
      sInt count=0;
      while (getInID(count))
        count++;

      strm << count;

      for (sInt i=0; i<count; i++)
        strm << gains[i];

      if (!strm.isWrite())
      {
        for (sInt i=count; i<255; i++)
          gains[i]=1.0f;
      }
    }
  }

  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    for (sInt i=0; i<255; i++)
    {
      if (getInID(i))
        f.putsS8(gains[i]*127);
    }

    // n bytes data+1 byte id+1 byte src cnt+n bytes src+1 byte dst=2n+3 bytes
    // (min 7, max 513)
  }
};

TG2_DECLARE_PLUGIN(frTCAdd, "Add", 0, 255, 2, sTRUE, sFALSE, 10);

// ---- frTCBump (depends on makeNormalMap)

struct vec
{
  sF32  x, y, z;
};

static void vecNorm(vec &x)
{
  sF32 l=sqrt(x.x*x.x+x.y*x.y+x.z*x.z);
  if (l)
    l=1.0f/l;
  x.x*=l;
  x.y*=l;
  x.z*=l;
}

class frTCBump: public frTexturePlugin
{
  sBool onParamChanged(sInt param)
  {
    return param == 1; // update parameter list if the type parameter was changed
  }

  sBool doProcess()
  {
    frTexture *inHeight=getInputData(1);
    sF32      escale;
    sF32      ica, oca, ocs;
    vec       l, lr, h, c, d;

    makeNormalMap(data->data, inHeight->data, inHeight->w, inHeight->h, params[0].floatv);

    if (params[1].selectv.sel == 0) // directional light
    {
      sF32 lat=params[4].floatv * 3.1415926535 / 180.0f;
      sF32 lon=params[5].floatv * 3.1415926535 / 180.0f;
      
      l.x=sin(lon);
      l.y=sin(lat)*cos(lon);
      l.z=cos(lat)*cos(lon);
      h.x=h.y=h.z=0.0f;
    }
    else
    {
      lr.x=params[2].pointv.x * 2.0f - 1.0f;
      lr.y=params[2].pointv.y * 2.0f - 1.0f;
      lr.z=params[3].floatv;
      escale=pow(2.0f, params[12].floatv);

      if (params[1].selectv.sel==2) // spot light
      {
        sF32 lat=params[4].floatv * 3.1415926535 / 180.0f;
        sF32 lon=params[5].floatv * 3.1415926535 / 180.0f;

        d.x=sin(lon);
        d.y=sin(lat)*cos(lon);
        d.z=cos(lat)*cos(lon);

        lr.x+=lr.z*d.x;
        lr.y+=lr.z*d.y;

        oca=cos(params[6].floatv * 3.1415926535 / 360.0f);
        ica=cos(params[7].floatv * 3.1415926535 / 360.0f);
        ocs=ica-oca;
        if (ocs)
          ocs=1.0f/ocs;
      }
    }
      
    sU16 *ptr=data->data, *iptr=getInputData(0)->data;
    for (sInt y=0; y<data->h; y++)
    {
      for (sInt x=0; x<data->w; x++)
      {
        if (params[1].selectv.sel!=0) // point/spot light
        {
          c.x=-(2.0f*x/data->w-1.0f);
          c.y=-(2.0f*y/data->h-1.0f);
          c.z=1.0f;

          l.x=lr.x+c.x;
          l.y=lr.y+c.y;
          l.z=lr.z;
          vecNorm(l);
          vecNorm(c);

          h.x=l.x+c.x;
          h.y=l.y+c.y;
          h.z=l.z+c.z;
          vecNorm(h);
        }

        sF32 ndotl=(((sInt) ptr[2]-16384)*l.x+((sInt) ptr[1]-16384)*l.y+((sInt) ptr[0]-16384)*l.z)/16384.0f;
        sF32 ndoth=(((sInt) ptr[2]-16384)*h.x+((sInt) ptr[1]-16384)*h.y+((sInt) ptr[0]-16384)*h.z)/16384.0f;
        
        if (ndotl<0.0f)
          ndotl=0.0f;

        if (ndoth<=0.0f)
          ndoth=0.0f;
        else
          ndoth=pow(ndoth, escale);

        if (params[1].selectv.sel==2) // spot light
        {
          sF32 ca=l.x*d.x+l.y*d.y+l.z*d.z; // cos alpha
          sF32 scale=1.0f;
          
          if (ca<=oca)
            scale=0.0f;
          else if (ca<=ica)
            scale=pow((ca-oca)*ocs, params[8].floatv);

          ndotl*=scale;
          ndoth*=scale;
        }
        
        for (sInt i=0; i<3; i++)
        {
          sF32 scale=(params[9].colorv.v[i]+params[10].colorv.v[i]*ndotl)/255.0f;
          sInt v=iptr[i]*scale+params[11].colorv.v[i]*ndoth*256.0f;
          if (v>32767)
            v=32767;

          ptr[i]=v;
        }

        ptr[3]=iptr[3];

        ptr+=4;
        iptr+=4;
      }
    }

    return sTRUE;
  }

public:
  frTCBump(const frPluginDef* d)
    : frTexturePlugin(d)
  {
    addFloatParam("Bumpyness", 1.0f, 0.01f, 63.0f, 0.01f, 2);
    addSelectParam("Light type", "Directional Light|Point Light|Spot Light", 1);
    addPointParam("Position XY", 0.5f, 0.5f, sFALSE);
    addFloatParam("Position Z", 1.0f, 0.0f, 25.0f, 0.01f, 2);
    addFloatParam("Direction latitude", 0.0f, -90.0f, 90.0f, 0.5f, 2);
    addFloatParam("Direction longitude", 0.0f, -90.0f, 90.0f, 0.5f, 2);
    addFloatParam("Outer cone", 45.0f, 0.0f, 180.0f, 1.0f, 2);
    addFloatParam("Inner cone", 30.0f, 0.0f, 180.0f, 1.0f, 2);
    addFloatParam("Falloff", 1.0f, 0.0f, 10.0f, 0.01f, 2);
    addColorParam("Ambient color", 0);
    addColorParam("Diffuse color", 0xe0e0e0);
    addColorParam("Specular color", 0x101010);
    addFloatParam("Specular exponent", 4.0f, 0.0f, 13.0f, 0.01f, 2);
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver = 3;
    
    strm << ver;

    // old version (import)
    if (ver >= 1 && ver <= 2)
    {
      if (ver == 1)
      {
        sInt hdet;
        strm << hdet;

        if (hdet)
          fr::debugOut("Bump: Non-grayscale 'Height detection' isn't supported anymore\n");
      }

      strm << params[0].floatv << params[1].selectv;
      strm << params[9].colorv << params[10].colorv;
      strm << params[2].pointv << params[3].floatv;
      strm << params[11].colorv << params[12].floatv;
      strm << params[4].floatv << params[5].floatv;
      strm << params[6].floatv << params[7].floatv << params[8].floatv;
    }

    // new version
    if (ver == 3)
    {
      strm << params[0].floatv << params[1].selectv;
      strm << params[2].pointv << params[3].floatv << params[4].floatv << params[5].floatv;
      strm << params[6].floatv << params[7].floatv << params[8].floatv;
      strm << params[9].colorv << params[10].colorv << params[11].colorv << params[12].floatv;
    }
  }

  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    f.putsU16(params[0].floatv * 1024);
    f.putsU8(params[1].selectv.sel);
    f.putsU8(params[9].colorv.b);
    f.putsU8(params[9].colorv.g);
    f.putsU8(params[9].colorv.r);
    f.putsU8(params[10].colorv.b);
    f.putsU8(params[10].colorv.g);
    f.putsU8(params[10].colorv.r);

    if (params[1].selectv.sel != 0)
    {
      f.putsU8(params[11].colorv.b);
      f.putsU8(params[11].colorv.g);
      f.putsU8(params[11].colorv.r);

      f.putsS16((params[2].pointv.x - 0.5f) * 1024);
      f.putsS16((params[2].pointv.y - 0.5f) * 1024);
      f.putsS16(params[3].floatv * 512);
      f.putsU8(params[12].floatv * 255.0f / 13.0f);
    }

    if (params[1].selectv.sel != 1)
    {
      f.putsS8(params[4].floatv * 127.0f / 90.0f);
      f.putsS8(params[5].floatv * 127.0f / 90.0f);
    }

    if (params[1].selectv.sel == 2)
    {
      f.putsU8(params[6].floatv * 255.0f / 180.0f);
      f.putsU8(params[7].floatv * 255.0f / 180.0f);
      f.putsU8(params[8].floatv * 255.0f / 10.0f);
    }
  }

  sBool displayParameter(sU32 index)
  {
    sInt type = params[1].selectv.sel;

    switch (index)
    {
    case 0: case 1:
      return sTRUE;

    case 2: case 3: // position xy/z
      return type != 0; // display if not directional light

    case 4: case 5: // latitude/longitude
      return type != 1; // display if not pointlight

    case 6: case 7: case 8: // inner/outer cone / falloff
      return type == 2; // display if spotlight

    case 9: case 10: // ambient/diffuse color
      return sTRUE;

    case 11: case 12: // specular color/exponent
      return type != 0; // display if not directional light
    }

    return sFALSE; // we shouldn't get here.
  }
};

TG2_DECLARE_PLUGIN(frTCBump, "Bump", 0, 2, 2, sTRUE, sFALSE, 11);

// ---- frTCDisplace

class frTCDisplace: public frTexturePlugin
{
  sBool doProcess()
  {
    sU16  *output, *source, *height;
    sInt  cnt, xo, yo, x, y;
    sS32  xa, ya;

    cnt=data->w*data->h;
    output=data->data;
    source=getInputData(0)->data;
    height=getInputData(1)->data;

    xo=params[0].tfloatv.a;
    yo=params[0].tfloatv.b;
    xa=params[1].tfloatv.a*65536.0f*64.0f;
    ya=params[1].tfloatv.b*65536.0f*64.0f;
    
    for (y=0; y<data->h; y++)
    {
      for (x=0; x<data->w; x++)
      {
        sInt nx=(x+xo+data->w) & (data->w-1);
        sInt ny=(y+yo+data->h) & (data->h-1);
        
        sF32 myh=getGrayscale(height+(y*data->w+x)*4);
        sF32 hx=getGrayscale(height+(y*data->w+nx)*4)-myh;
        sF32 hy=getGrayscale(height+(ny*data->w+x)*4)-myh;

        sU32 xp=(x<<16)+hx*xa;
        sU32 yp=(y<<16)+hy*ya;
        
        sU32 rxp=(xp>>16) & (data->w-1), rxpn=(rxp+1) & (data->w-1);
        sU32 ryp=(yp>>16) & (data->h-1);
        sU16 *pt1=source+((ryp*data->w)+rxp)*4;
        sU16 *pt2=source+((ryp*data->w)+rxpn)*4;
        ryp=(ryp+1)&(data->h-1);
        sU16 *pt3=source+((ryp*data->w)+rxp)*4;
        sU16 *pt4=source+((ryp*data->w)+rxpn)*4;
        
        for (sInt k=0; k<4; k++)
        {
          sInt xw=(xp>>8)&0xff;
          sU32 t1=((sU32) pt1[k]<<8)+xw*(pt2[k]-pt1[k]);
          sU32 t2=((sU32) pt3[k]<<8)+xw*(pt4[k]-pt3[k]);
          sU32 t3=(t1<<8)+((yp&0xff00)>>8)*((sS32) (t2-t1));
          
          output[k]=t3>>16;
        }

        output+=4;
      }
    }
    
    return sTRUE;
  }
  
public:
  frTCDisplace(const frPluginDef* d)
    : frTexturePlugin(d)
  {
    addTwoFloatParam("Offset", 2, 2, -128, 127, 1, 0);
    addTwoFloatParam("Amplify", 1.0f, 1.0f, 0.0f, 16.0f, 0.01f, 3);
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
          fr::debugOut("Displace: Non-grayscale 'height detection' isn't supported anymore\n");
      }

      strm << params[0].tfloatv;
      strm << params[1].tfloatv;
    }
  }
  
  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    f.putsS8(params[0].tfloatv.a);
    f.putsS8(params[0].tfloatv.b);
    f.putsS16(params[1].tfloatv.a*32767.0f/16.0f);
    f.putsS16(params[1].tfloatv.b*32767.0f/16.0f);

    // 7 bytes data+1 byte id+2 bytes src+1 byte dst=11 bytes
  }
};

TG2_DECLARE_PLUGIN(frTCDisplace, "Displacement", 0, 2, 2, sTRUE, sFALSE, 16);

// ---- frTCMul

class frTCMul: public frTexturePlugin
{
  sBool doProcess()
  {
    sU16  *ptd, *pts;
    sInt  cnt, i;
    
    ptd=data->data;
    cnt=data->w*data->h;
    
    frTexture *in=getInputData(0);
    
    memcpy(ptd, getInputData(0)->data, cnt*sizeof(sU16)*4);
    for (i=1; i<def->nInputs; i++)
    {
      if (!getInputData(i))
        break;
      
      pts=getInputData(i)->data;
      
      __asm
      {
        mov     edi, [ptd];
        mov     esi, [pts];
        mov     ecx, [cnt];
        dec     ecx;
lp:
        movq    mm0, [edi+ecx*8];
        pmulhw  mm0, [esi+ecx*8];
        psllw   mm0, 1;
        movq    [edi+ecx*8], mm0;
        dec     ecx;
        jns     lp;
        emms;
      }
    }
    
    return sTRUE;
  }
  
public:
  frTCMul(const frPluginDef* d)
    : frTexturePlugin(d)
  {
  }
  
  void serialize(fr::streamWrapper &strm)
  {
    if (def->ID!=12)
    {
      sU16 ver=1;
      
      strm << ver;
    }
  }
};

TG2_DECLARE_PLUGIN(frTCMul, "Multiply", 0, 255, 2, sTRUE, sFALSE, 20);
typedef frTCMul frTCMulOld;
TG2_DECLARE_PLUGIN(frTCMulOld, "Multiply (Old!)", 0, 2, 2, sTRUE, sFALSE, -12);
