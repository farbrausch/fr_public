// This code is in the public domain. See LICENSE for details.

#include "types.h"
#include "opsys.h"
#include "texture.h"
#include "opdata.h"
#include "rtlib.h"
#include "math3d_2.h"

// for text stuff
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// ---- tool functions

static sF32 getGrayscale(const sU16* ptr)
{
  return (ptr[0] * 0.11f + ptr[1] * 0.59f + ptr[2] * 0.30f) / 32767.0f;
}

static sF32 calcHeights(const sU16* hmap, sInt x, sInt y, sInt w, sInt h, sInt xo, sInt yo, sF32& hx, sF32& hy)
{
  const sF32 hgt = getGrayscale(hmap + (y * w + x) * 4);
  hx = getGrayscale(hmap + (y * w + ((x + xo) & (w - 1))) * 4) - hgt;
  hy = getGrayscale(hmap + (((y + yo) & (h - 1)) * w + x) * 4) - hgt;

  return hgt;
}

static void makeNormalMap(sU16* dst, const sU16* src, sInt w, sInt h, sF32 hScale)
{
  sInt  x, y;
  sF32  nx, ny, nz;

  for (y = 0; y < h; y++)
  {
    for (x = 0; x < w; x++)
    {
      dst[3] = calcHeights(src, x, y, w, h, 1, 1, nx, ny) * 32767.0f;
      nx = -nx * hScale;
      ny = -ny * hScale;
      nz = 16383.0f / sqrt(nx * nx + ny * ny + 1.0f);
      dst[0] = nz + 16384;
      dst[1] = ny * nz + 16384;
      dst[2] = nx * nz + 16384;
      dst += 4;
    }
  }
}

static void sampleBilinear(sU16* outPtr, const sU16* srcPtr, sS32 xp, sS32 yp, sInt w, sInt h)
{
  sU32 rxp = (xp >> 16) & (w - 1), rxpn=(rxp + 1) & (w - 1);
  sU32 ryp = (yp >> 16) & (h - 1);
  const sU16 *pt1 = srcPtr + ((ryp * w) + rxp) * 4;
  const sU16 *pt2 = srcPtr + ((ryp * w) + rxpn) * 4;
  ryp = (ryp + 1) & (h - 1);
  const sU16 *pt3 = srcPtr + ((ryp * w) + rxp) * 4;
  const sU16 *pt4 = srcPtr + ((ryp * w) + rxpn) * 4;

  for (sInt k = 0; k < 4; k++)
  {
    sInt xw = (xp >> 8) & 0xff;
    sU32 t1 = ((sU32) pt1[k] << 8) + xw * (pt2[k] - pt1[k]);
    sU32 t2 = ((sU32) pt3[k] << 8) + xw * (pt4[k] - pt3[k]);
    sU32 t3 = (t1 << 8) + ((yp & 0xff00) >> 8) * ((sS32) (t2 - t1));

    outPtr[k] = t3 >> 16;
  }
}

static const sU8* getColor(sU8* dest,const sU8* src,sInt code)
{
  switch(code & 3)
  {
  case 0: // other
    dest[0] = *src++;
    dest[1] = *src++;
    dest[2] = *src++;
    break;

  case 1: // gray
    dest[0] = dest[1] = dest[2] = *src++;
    break;

  case 2: // black
    dest[0] = dest[1] = dest[2] = 0;
    break;

  case 3: // white
    dest[0] = dest[1] = dest[2] = 255;
    break;
  }

  return src;
}

// ---- random dots

class frOpTGRandomDots: public frTextureOperator
{
  sU16  dotCount;
  sU32  randomSeed;
  sU16  color[9];

  void doProcess()
  {
    frBitmap* data = getData();
    sU16* outData = data->data;

    for (sInt i = 0; i < data->size; i++)
    {
      outData[0] = color[0];
      outData[1] = color[1];
      outData[2] = color[2];
      outData[3] = 0;
      outData += 4;
    }

    sU32 seed = randomSeed;

    for (sInt i = 0; i < dotCount; i++)
    {
      const sU16* pColor = color + (i & 1) * 3 + 3;

      sInt x = (seed >> 16) & (data->w - 1); seed = seed * 375375621 + 1;
      sInt y = (seed >> 16) & (data->h - 1); seed = seed * 375375621 + 1;

      sU16* out = data->data + (y * data->w + x) * 4;
      out[0] = pColor[0];
      out[1] = pColor[1];
      out[2] = pColor[2];
    }
  }

public:
  const sU8* readData(const sU8* strm)
  {
    dotCount = ((sU16 *) strm)[0];
    randomSeed = (((sU16 *) strm)[1] + 303) * 0x31337303;
    randomSeed ^= randomSeed >> 5;
    strm += 4;

    for (sInt i = 0; i < 9; i++)
      color[i] = *strm++ << 7;

    return strm;
  }
};

frOperator* create_TGRandomDots() { return new frOpTGRandomDots; }

// ---- trivial ops

class frOpTOTrivial: public frTextureOperator
{
  sU8 color[6];

  void doProcess()
  {
    sU16* dst = data->data;

    switch(def->id)
    {
    case 0x01: // rectangle
      {
        for (sInt y = 0; y < data->h; y++)
        {
          for (sInt x = 0; x < data->w; x++)
          {
            const sInt tx = (x << 8) / data->w;
            const sInt ty = (y << 8) / data->h;
            const sU8* clr = color;

            if (tx >= param[0] && tx <= param[1] && ty >= param[2] && ty <= param[3])
              clr += 3;

            *dst++ = clr[0] << 7;
            *dst++ = clr[1] << 7;
            *dst++ = clr[2] << 7;
            *dst++ = 0;
          }
        }
      }
      break;

    case 0x02: // solid
      {
        for (sInt i = 0; i < data->size; i++)
        {
          *dst++ = param[0] << 7;
          *dst++ = param[1] << 7;
          *dst++ = param[2] << 7;
          *dst++ = 0;
        }
      }
      break;

    case 0x07: // make normal map
      makeNormalMap(dst, getInputData(0)->data, data->w, data->h, *reinterpret_cast<const sU16*>(param) / 1024.0f);
      break;

    case 0x10: // displace
      {
        const sU16* source = getInputData(0)->data;
        const sU16* height = getInputData(1)->data;
        
        sS32 xa = reinterpret_cast<const sS16*>(param)[1] * 16.0f / 32767.0f * 65536.0f * 64.0f;
        sS32 ya = reinterpret_cast<const sS16*>(param)[2] * 16.0f / 32767.0f * 65536.0f * 64.0f;

        for (sInt y = 0; y < data->h; y++)
        {
          for (sInt x = 0; x < data->w; x++)
          {
            sF32 hx, hy;
            calcHeights(height, x, y, data->w, data->h, param[0], param[1], hx, hy);
            sampleBilinear(dst, source, (x << 16) + hx * xa, (y << 16) + hy * ya, data->w, data->h);
            dst += 4;
          }
        }
      }
      break;

    case 0x11: // set alpha
      {
        const sU16* src1 = getInputData(0)->data;
        const sU16* src2 = getInputData(1)->data;

        for (sInt i = 0; i < data->size; i++)
        {
          sU32 alpha = (src2[0] + src2[1] + src2[1] + src2[2]) >> 2;
          if (param[0] & 1)
            alpha ^= 32767;

          dst[0] = src1[0];
          dst[1] = src1[1];
          dst[2] = src1[2];
          dst[3] = alpha;

          src1 += 4;
          src2 += 4;
          dst += 4;
        }
      }
      break;

    case 0x12: // blend
      {
        const sU16* src1 = getInputData(0)->data;
        const sU16* src2 = getInputData(1)->data;
        const sU16* src3 = getInputData(2)->data;

        for (sInt i = 0; i < data->size; i++)
        {
          sU32 alpha = (src3[0] + src3[1] + src3[1] + src3[2]) >> 2;
          if (param[0] & 1)
            alpha ^= 32767;

          for (sInt c = 0; c < 4; c++)
            dst[c] = src1[c] + ((((sS32) src2[c] - src1[c]) * alpha) >> 15);

          src1 += 4;
          src2 += 4;
          src3 += 4;
          dst += 4;
        }
      }
      break;
    }
  }

public:
  const sU8* readData(const sU8* strm)
  {
    if(def->id == 0x01) // rectangle
    {
      param = strm;
      strm += 4;
      sU8 type = *strm++;

      strm = getColor(color + 0, strm, type >> 0);
      strm = getColor(color + 3, strm, type >> 2);

      return strm;
    }
    else
      return frOperator::readData(strm);
  }
};

frOperator* create_TOTrivial() { return new frOpTOTrivial; }

// ---- text

class frOpTGText: public frTextureOperator
{
  const sChar*  strs[2];
  sInt          xp, yp, height, style;
  sU32          color[2];

  void doProcess()
  {
    sU16*   dst = data->data;
    HDC     hDC, hTempDC;
    HFONT   hFont, hOldFont;
    HBITMAP hBitmap, hOldBitmap;
    HBRUSH  hBrush;
    sU8*    buf;

    // initialize & allocate resources
    hTempDC = GetDC(0);
    hDC = CreateCompatibleDC(hTempDC);

    static BITMAPINFOHEADER bih={ sizeof(BITMAPINFOHEADER), 0, 0, 1, 32, 0, 0, 0, 0, 0, 0 };
    bih.biWidth = data->w;
    bih.biHeight = -data->h;

    sInt realHeight = (height * data->h) >> 8;

    hBitmap = CreateDIBSection(hTempDC, (BITMAPINFO *) &bih, DIB_RGB_COLORS, (void **) &buf, 0, 0);
    hFont = CreateFont(realHeight, 0, 0, 0, (style & 1) ? FW_BOLD : FW_NORMAL, style & 2, 0, 0, DEFAULT_CHARSET,
      OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, strs[1]);
    hBrush = CreateSolidBrush(color[1]);

    hOldBitmap=(HBITMAP) SelectObject(hDC, hBitmap);
    hOldFont=(HFONT) SelectObject(hDC, hFont);

    // fill bitmap with background color
    RECT rc;
    rc.left = rc.top = 0;
    rc.right = data->w;
    rc.bottom = data->h;
    FillRect(hDC, &rc, hBrush);

    // write the text
    SetTextColor(hDC, color[0]);
    SetBkMode(hDC, TRANSPARENT);

    sInt sp = 0, sl, sa;
    const sChar* text = strs[0];

    while (text[sp])
    {
      sa = sl = 0;

      while (1)
      {
        if (text[sp + sl] == '\\')
        {
          sa = 1;
          break;
        }
        else
          if (!text[sp + sl])
            break;

        sl++;
      }

      TextOut(hDC, xp, yp, text + sp, sl);
      sp += sl + sa;
      yp += realHeight;
    }

    SelectObject(hDC, hOldBitmap);
    SelectObject(hDC, hOldFont);

    for (sInt i = 0; i < data->size; i++)
    {
      dst[0] = buf[0] << 7;
      dst[1] = buf[1] << 7;
      dst[2] = buf[2] << 7;
      dst[3] = 0;
      dst += 4;
      buf += 4;
    }

    DeleteObject(hBitmap);
    DeleteObject(hFont);
    DeleteObject(hBrush);
    DeleteDC(hDC);
  }

public:
  const sU8* readData(const sU8* strm)
  {
    xp = reinterpret_cast<const sU16*>(strm)[0];
    yp = reinterpret_cast<const sU16*>(strm)[1];
    height = strm[4];
    style = strm[5];
    strm += 6;

    for (sInt i = 0; i < 2; i++)
    {
      strs[i] = reinterpret_cast<const sChar*>(strm);
      strm += strlen(strs[i]) + 1;
    }

    for (sInt i = 0; i < 2; i++)
    {
      color[i] = *reinterpret_cast<const sU32*>(strm) & 0xffffff;
      strm += 3;
    }

    return strm;
  }
};

frOperator* create_TGText() { return new frOpTGText; }

// ---- rotozoom

class frOpTFRotoZoom: public frTextureOperator
{
  sF32  angle, zoom1, zoom2;
  sS32  stx, sty;

  void doProcess()
  {
    const sU16* src = getInputData(0)->data;
    sU16*       dst = data->data;
    sS32        xp, yp, yxp, yyp, xsx, ysx, xsy, ysy;

    xsx =  zoom1 * cos(angle);  ysx = zoom2 * sin(angle);
    xsy = -zoom1 * sin(angle);  ysy = zoom2 * cos(angle);

    yxp = stx;
    yyp = sty;

    for (sInt y = 0; y < data->h; y++)
    {
      xp = yxp;
      yp = yyp;

      for (sInt x = 0; x < data->w; x++)
      {
        sampleBilinear(dst, src, xp, yp, data->w, data->h);

        xp += xsx;
        yp += ysx;
        dst += 4;
      }

      yxp += xsy;
      yyp += ysy;
    }
  }

public:
  const sU8* readData(const sU8* strm)
  {
    angle = *reinterpret_cast<const sU16*>(strm) * 3.1415926535f / 32768.0f;
    strm += 2;

    zoom1 = getPackedFloat(strm);
    zoom2 = getPackedFloat(strm);

    stx = reinterpret_cast<const sS32*>(strm)[0];
    sty = reinterpret_cast<const sS32*>(strm)[1];
    strm += 8;

    return strm;
  }
};

frOperator* create_TFRotoZoom() { return new frOpTFRotoZoom; }

// ---- adjust

class frOpTFAdjust: public frTextureOperator
{
  static sInt __fastcall conBright(sInt v, sS32 cont, sS32 bright)
  {
    v = ((v * cont) >> 11) + bright;
    v = (v < 0) ? 0 : (v > 32767) ? 32767 : v;
    
    return v;
  }

  void doProcess()
  {
    sU16* pts = getInputData(0)->data;
    sU16* ptd = data->data;
    sInt cont, brgt, hue, sat;

    cont = reinterpret_cast<const sS16*>(param)[0];
    brgt = reinterpret_cast<const sS16*>(param)[1] + 16384 - 8 * cont;
    hue = reinterpret_cast<const sU16*>(param)[2] * 6;
    sat = reinterpret_cast<const sS16*>(param)[3];

    sU32 cnt = data->size;
    do
    {
      // rgb->hsb

      sU32 max = pts[0], min = pts[0];

      min = (pts[1] < min) ? pts[1] : min;
      max = (pts[1] > max) ? pts[1] : max;
      min = (pts[2] < min) ? pts[2] : min;
      max = (pts[2] > max) ? pts[2] : max;

      // calculate and adjust saturation
      sS32 s = max - min;

      if (sat < 0)
        s = (s * (sat + 32768)) >> 15;
      else
        s += ((max - s) * sat) >> 15;

      // calculate and shift hue, then convert back to rgb
      if (s)
      {
        sS32 h, d;

        if (pts[2] == max)
          h = 3, d = pts[1] - pts[0];
        else if (pts[1] == max)
          h = 1, d = pts[0] - pts[2];
        else if (pts[0] == max)
          h = 2, d = pts[2] - pts[1];

        sS32 hv = hue;

        __asm
        {
          mov     eax, [d];
          mov     edx, eax;
          shl     eax, 16;
          sar     edx, 16;
          mov     ebx, [max];
          sub     ebx, [min];
          inc     ebx;
          idiv    ebx;
          mov     ebx, [h];
          shl     ebx, 17;
          add     eax, [hv];
          add     eax, ebx;
          mov     [h], eax;
        }

        const sInt ih = h >> 16;
        const sS32 fh = h & 0xffff;

        sS32 vals[4];
        vals[0] = 0;
        vals[1] = s;
        vals[2] = (s * fh) >> 16;
        vals[3] = (s * (65536 - fh)) >> 16;

        static const sU8 st[] = { 1, 1, 3, 0, 0, 2, 1, 1, 3, 0, 0, 2, 1, 1, 3, 0, 0, 2 };

        ptd[0] = conBright(max - vals[st[ih]], cont, brgt);
        ptd[1] = conBright(max - vals[st[ih + 2]], cont, brgt);
        ptd[2] = conBright(max - vals[st[ih + 4]], cont, brgt);
      }
      else // plain grayscale
      {
        sInt x = conBright(max, cont, brgt);

        ptd[0] = x;
        ptd[1] = x;
        ptd[2] = x;
      }

      ptd[3] = conBright(pts[3], cont, brgt); // contrast+brightness alpha (but no color shift)

      pts += 4;
      ptd += 4;
    }
    while(--cnt);
  }
};

frOperator* create_TFAdjust() { return new frOpTFAdjust; }

// ---- Add

class frOpTCAdd: public frTextureOperator
{
  const sS8* gains;

  void doProcess()
  {
    sU16* dst = data->data;
    const sInt cnt = data->size;

    memset(dst, 0, data->size * 4 * sizeof(sU16));

    for (sInt i = 0; i < nInputs; i++)
    {
      const sU16* src = getInputData(i)->data;
      sInt gain = gains[i] * 32767 / 127;

      if (gain > 0)
      {
        __asm
        {
          mov       esi, [src];
          mov       edi, [dst];
          mov       ecx, [cnt];
          lea       esi, [esi + ecx * 8];
          lea       edi, [edi + ecx * 8];
          neg       ecx;
          movd      mm2, [gain];
          punpcklwd mm2, mm2;
          punpcklwd mm2, mm2;

lp1:
          movq      mm0, [edi + ecx * 8];
          movq      mm1, [esi + ecx * 8];
          pmulhw    mm1, mm2;
          psllw     mm1, 1;
          paddsw    mm0, mm1;
          movq      [edi + ecx * 8], mm0;
          inc       ecx;
          jnz       lp1;
        }
      }
      else if (gain < 0)
      {
        gain = -gain;

        __asm
        {
          mov       esi, [src];
          mov       edi, [dst];
          mov       ecx, [cnt];
          lea       esi, [esi + ecx * 8];
          lea       edi, [edi + ecx * 8];
          neg       ecx;
          movd      mm2, [gain];
          punpcklwd mm2, mm2;
          punpcklwd mm2, mm2;

lp2:
          movq      mm0, [edi + ecx * 8];
          movq      mm1, [esi + ecx * 8];
          pmulhw    mm1, mm2;
          psllw     mm1, 1;
          psubusw   mm0, mm1;
          movq      [edi + ecx * 8], mm0;
          dec       ecx;
          jns       lp2;
        }
      }
    }

    __asm emms;
  }

public:
  const sU8* readData(const sU8* strm)
  {
    gains = reinterpret_cast<const sS8*>(strm);

    return strm + nInputs;
  }
};

frOperator* create_TCAdd() { return new frOpTCAdd; }

// ---- bump mapping

class frOpTCBump: public frTextureOperator
{
  sF32        bump;
  sU8         color[9];
  sInt        lType;
  fr::vector  olr;
  sF32        escale, falloff;
  sF32        oca, ica;
  sF32        lat, lon;

  void doProcess()
  {
    fr::vector c, d, lr, l[2];
    sF32 ocs;

    if (lType != 1)
    {
      l[0].x = sin(lon);
      l[0].y = sin(lat) * cos(lon);
      l[0].z = cos(lat) * cos(lon);
      l[1].x = 0.0f;
      l[1].y = 0.0f;
      l[1].z = 0.0f;
    }

    lr = olr;

    if (lType == 2)
    {
      d = l[0];
      lr.x += lr.z * d.x;
      lr.y += lr.z * d.y;

      ocs = ica - oca;
      if (ocs)
        ocs = 1.0f / ocs;
    }

    sU16* dst = data->data;
    makeNormalMap(dst, getInputData(1)->data, data->w, data->h, bump);

    const sU16* src = getInputData(0)->data;

    for (sInt y = 0; y < data->h; y++)
    {
      for (sInt x = 0; x < data->w; x++)
      {
        if (lType != 0) // point/spot light
        {
          c.x = 1.0f - 2.0f * x / data->w;
          c.y = 1.0f - 2.0f * y / data->h;
          c.z = 1.0f;

          l[0].x = lr.x + c.x;
          l[0].y = lr.y + c.y;
          l[0].z = lr.z;
          l[0].norm();

          c.norm();

          l[1].add(l[0], c);
          l[1].norm();
        }

        sF32 dots[2];
        fr::vector normal(dst[2] - 16384, dst[1] - 16384, dst[0] - 16384);

        for (sInt i = 0; i < 2; i++)
        {
          sF32 dot = l[i].dot(normal) / 16384.0f;
          if (dot < 0.0f)
            dot = 0.0f;

          dots[i]=dot;
        }

        dots[1] = pow(dots[1], escale);
        sF32 spotTerm = 1.0f;

        if (lType == 2) // spot light
        {
          sF32 ca = l[0].dot(d); // cos alpha

          if (ca <= oca)
            spotTerm = 0.0f;
          else if (ca <= ica)
            spotTerm = pow((ca - oca) * ocs, falloff);
        }

        for (sInt i = 0; i < 3; i++)
        {
          const sF32 scale = (color[i] + color[i + 3] * dots[0]) / 255.0f;
          sInt v = (src[i] * scale + color[i + 6] * dots[1] * 256.0f) * spotTerm;
          if (v > 32767)
            v = 32767;

          dst[i] = v;
        }

        dst[3] = src[3];

        dst += 4;
        src += 4;
      }
    }
  }

public:
  const sU8* readData(const sU8* strm)
  {
    bump = *reinterpret_cast<const sU16*>(strm) / 1024.0f; 
    lType = strm[2];
    strm += 3;

    for (sInt i = 0; i < ((lType != 0) ? 9 : 6); i++)
      color[i] = *strm++;

    if (lType != 0) // point&spot
    {
      for (sInt i = 0; i < 3; i++)
      {
        (&olr.x)[i] = *reinterpret_cast<const sS16*>(strm) / 512.0f;
        strm += 2;
      }

      escale = pow(2.0f, *strm++ * 13.0f / 255.0f);
    }

    if (lType != 1) // dir&spot
    {
      lat = reinterpret_cast<const sS8*>(strm)[0] * 3.1415926535f / 254.0f;
      lon = reinterpret_cast<const sS8*>(strm)[1] * 3.1415926535f / 254.0f;
      strm += 2;
    }

    if (lType == 2) // spot
    {
      oca = cos(strm[0] * 3.1415926535f / 510.0f);
      ica = cos(strm[1] * 3.1415926535f / 510.0f);
      falloff = strm[2] * 10.0f / 255.0f;
      strm += 3;
    }

    return strm;
  }
};

frOperator* create_TCBump() { return new frOpTCBump; }

// ---- Perlin noise

class frOpTGPerlin: public frTextureOperator
{
  sInt nOctaves;
  sInt persistence;
  sInt freq;
  sU32 oSeed;
  sInt startOctave;
  sU16 scale;
  sInt color[6];

  static sU32 seed;
  static sU16 wtab[512];
  static sBool wTabThere;

  static sS32 __forceinline inoise(sU32 x, sU32 y, sInt msk)
  {
    sInt ix = x >> 10, iy = y >> 10;
    sInt inx = (ix + 1) & msk, iny = (iy + 1) & msk;

    ix += seed;
    inx += seed;
    iy *= 31337;
    iny *= 31337;

    sInt fx = wtab[(x >> 1) & 511];

    sS32 t1 = srandom2(ix + iy);
    t1 = (t1 << 10) + (srandom2(inx + iy) - t1) * fx;
    sS32 t2 = srandom2(ix + iny);
    t2 = (t2 << 10) + (srandom2(inx + iny) - t2) * fx;

    return ((t1 << 10) + (t2 - t1) * wtab[(y >> 1) & 511]) >> 20;
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

  static void calcWTab()
  {
    for (sInt i = 0; i < 512; i++)
    {
      const sF32 val = i / 512.0f;
      wtab[i] = 1024.0f * val * val * val * (10.0f + val * (val * 6.0f - 15.0f));
    }
  }

  void doProcess()
  {
    sU16* out = data->data;
    sInt  x, y, o, f;
    sS32  amp, sc, sum, xv, yv, fx, fy;

    if (!wTabThere)
      calcWTab();

    sU32 oseed = oSeed + startOctave * 0xdeadbeef;
    const sInt fbase = (1024 << startOctave) * freq;

    fx = fbase / data->w;
    fy = fbase / data->h;

    sS32 samp;
    sum = 0;
    amp = 32768;

    for (o = 0; o < nOctaves; o++)
    {
      if (o >= startOctave)
        sum+=amp;

      if (o == startOctave)
        samp = amp;

      amp = (amp * persistence) >> 8;
    }

    sc = scale * 8 * 64 * 16 / sum;

    sInt fb = freq << startOctave;

    for (y = 0; y < data->h; y++)
    {
      for (x = 0; x < data->w; x++)
      {
        amp = samp;
        sum = 0;
        xv = x * fx;
        yv = y * fy;
        f = fb;
        seed = oseed;

        for (o = startOctave; o < nOctaves; o++)
        {
          sum += amp * inoise(xv, yv, f - 1);
          xv *= 2;
          yv *= 2;
          f *= 2;
          seed += 0xdeadbeef;
          amp = (amp * persistence) >> 8;
        }

        sS32 sm = imul16(sum, sc) + 32768;
        sm = (sm < 0) ? 0 : (sm > 65536) ? 65536 : sm;

        *out++ = color[0] + ((sm * ((sInt) color[3] - (sInt) color[0])) >> 16);
        *out++ = color[1] + ((sm * ((sInt) color[4] - (sInt) color[1])) >> 16);
        *out++ = color[2] + ((sm * ((sInt) color[5] - (sInt) color[2])) >> 16);
        *out++ = 0;
      }
    }
  }

public:
  const sU8* readData(const sU8* strm)
  {
    sInt temp = *strm++;

    nOctaves = temp & 15;
    persistence = *strm++;

    freq = 1 << (temp >> 4);
    oSeed = (*reinterpret_cast<const sU16*>(strm) + 1) * 0xb37fa184 + 303 * 31337;
    strm += 2;

    temp = *strm++;
    startOctave = temp & 15;
    scale = getSmallInt(strm);

    sU8 kolor[6];
    strm = getColor(kolor + 0,strm,temp >> 4);
    strm = getColor(kolor + 3,strm,temp >> 6);
    for(sInt i=0;i<6;i++)
      color[i] = kolor[i]<<7;

    return strm;
  }

  static sU32 srandom2(sU32 n)
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
};

sU32 frOpTGPerlin::seed;
sU16 frOpTGPerlin::wtab[512];
sBool frOpTGPerlin::wTabThere=sFALSE;

frOperator* create_TGPerlin() { return new frOpTGPerlin; }

// ---- Gradient

class frOpTGGradient: public frTextureOperator
{
  sInt  type;
  sU8   color[6];
  sF32  iw, ih;
  sF32  xc, yc, x2, y2, expon, isc;
  sInt  scv;

  void doProcess()
  {
    sF32 sc;
		sF32 xd = x2 - xc, yd = y2 - yc;

    if (type == 0)
      sc = 1.0f / (xd * xd + yd * yd);
    else
    {
      sc = 100.0f / scv;
      sc *= sc;
    }

    sU16* dst = data->data;

    for (sInt y = 0; y < data->h; y++)
    {
      const sF32 p = y * ih - yc, pp = p * p;

      for (sInt x = 0; x < data->w; x++)
      {
        sF32 f;
        sF32 n = x * iw - xc;

        if (type == 0) // linear
          f = (n * xd + p * yd) * sc;
        else if (type == 1) // glow
        {
          f = 1.0f - sqrt((n * n + pp) * sc);
          f = pow(f < 0.0f ? 0.0f : f > 1.0f ? 1.0f : f, expon) * isc;
        }

        f=f < 0.0f ? 0.0f : f > 1.0f ? 1.0f : f;
        sS32 fd = f * 65536;

        *dst++ = ((color[0] << 16) + (color[3] - color[0]) * fd) >> 9;
        *dst++ = ((color[1] << 16) + (color[4] - color[1]) * fd) >> 9;
        *dst++ = ((color[2] << 16) + (color[5] - color[2]) * fd) >> 9;
        *dst++ = 0;
      }
    }
  }

public:
  const sU8* readData(const sU8* strm)
  {
    sU8 typeByte = *strm++;
    type = typeByte & 1;

    strm = getColor(color + 0, strm, typeByte >> 1);
    strm = getColor(color + 3, strm, typeByte >> 3);
    iw = 1.0f / data->w;
    ih = 1.0f / data->h;

    if (type == 0)
    {
      xc = reinterpret_cast<const sS16*>(strm)[0] / 256.0f;
      yc = reinterpret_cast<const sS16*>(strm)[1] / 256.0f;
      x2 = reinterpret_cast<const sS16*>(strm)[2] / 256.0f;
      y2 = reinterpret_cast<const sS16*>(strm)[3] / 256.0f;
      strm += 8;
    }
    else
    {
      scv = *strm++;
      expon = getSmallInt(strm) / 8.0f;
      isc = reinterpret_cast<const sU16*>(strm)[0] / 256.0f;
      xc = reinterpret_cast<const sS16*>(strm)[1] / 256.0f;
      yc = reinterpret_cast<const sS16*>(strm)[2] / 256.0f;
      strm += 6;
    }

    return strm;
  }

	void setAnim(sInt parm, const sF32* val)
	{
		switch (parm)
		{
		case 1: // point 1/center
		case 3:
			xc = val[0];
			yc = val[1];
			break;

		case 2: // point 2
			xc = val[0];
			yc = val[1];
			break;
		}
	}
};

frOperator* create_TGGradient() { return new frOpTGGradient; }

// ---- Multiply

class frOpTCMultiply: public frTextureOperator
{
  void doProcess()
  {
    sU16* dst = data->data;
    sInt cnt = data->w * data->h;

    memcpy(dst, getInputData(0)->data, cnt * 4 * 2);

    for (sInt i = 1; i < nInputs; i++)
    {
      const sU16* src = getInputData(i)->data;

      __asm
      {
        mov     esi, [src];
        mov     edi, [dst];
        mov     ecx, [cnt];
        lea     esi, [esi + ecx * 8];
        lea     edi, [edi + ecx * 8];
        neg     ecx;

lp:
        movq    mm0, [esi + ecx * 8];
        pmulhw  mm0, [edi + ecx * 8];
        psllw   mm0, 1;
        movq    [edi + ecx * 8], mm0;
        inc     ecx;
        jnz     lp;
      }
    }

    __asm emms;
  }
};

frOperator* create_TCMultiply() { return new frOpTCMultiply; }

// ---- Crystal

class frOpTGCrystal: public frTextureOperator
{
  sU32  seed, count;
  sU32  col[2];

  void doProcess()
  {
    sU8   randBuf[512];
    sU16* dptr = data->data;
    sInt  width = data->w;
    sInt  height = data->h;

    static const sU32 _MMMax = 0x7fffffff;
    const sU32 _MM256 = (height << 16) | width, temp = (_MM256 >> 1) & 0x7fff7fff;
    const sU64 _MM128 = ((sU64)temp << 32) | temp;
    const sU64 _MMMask = ((height - 1) << 16) | (width - 1);

    const sU32 col1 = col[0];
    const sU32 col2 = col[1];
    const sU32 cnt = count;
    const sU32 sed = seed;

    __asm
    {
      mov       edx, [sed];
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
      mov       edi, [cnt];
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
      sar       edx, 16;
      idiv      edi;
      xor       eax, -1;
      add       eax, ecx;
      imul      eax;
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
    };
  }

public:
  const sU8* readData(const sU8* strm)
  {
    seed = reinterpret_cast<const sU16*>(strm)[0];
    count = strm[2];
    strm += 3;

    for (sInt i = 0; i < 2; i++)
    {
      col[i] = *reinterpret_cast<const sU32*>(strm) & 0xffffff;
      strm += 3;
    }

    return strm;
  }
};

frOperator* create_TGCrystal() { return new frOpTGCrystal; }

// ---- blur v2

class frOpTFBlurV2: public frTextureOperator
{
  sInt rx, ry, parm, intens;

public:
  static void inplaceBlur(sU16* dst, sU32 w, sU32 h, sUInt radius, sInt wrap)
  {
    static sU16 pbuf[(1024+256)*4];

    if (2*radius>=w)
      radius=(w-1)/2;

    sU32 sc=65536/(2*radius+1);

    if (wrap==0)
    {
      memset(pbuf, 0, radius*4*sizeof(sU16));
      memset(pbuf+(w+radius)*4, 0, radius*4*sizeof(sU16));
    }

    __asm emms;

    while (h--)
    {
      memcpy(pbuf+radius*4, dst, w*4*sizeof(sU16));

      if (wrap==1) // clamp
      {
        for (sUInt i=0; i<radius*4; i++)
        {
          const sUInt ia=i&3;

          pbuf[i]=dst[ia];
          pbuf[i+(w+radius)*4]=dst[w*4-4+ia];
        }
      }
      else if (wrap==2) // wrap
      {
        memcpy(pbuf, pbuf+w*4, radius*4*sizeof(sU16));
        memcpy(pbuf+(w+radius)*4, pbuf+radius*4, radius*4*sizeof(sU16));
      }

      const sU16 *src=pbuf+radius*4;
      static const sU64 msk1=0x0000ffff0000ffff;

      __asm
      {
        mov       esi, offset pbuf;
        pxor      mm0, mm0;
        pxor      mm1, mm1;
        pxor      mm3, mm3;
        mov       ecx, [radius];
        shl       ecx, 1;

sumlp:
        movq      mm4, [esi];
        add       esi, 8;
        movq      mm5, mm4;
        punpcklwd mm4, mm3;
        dec       ecx;
        punpckhwd mm5, mm3;
        paddd     mm0, mm4;
        paddd     mm1, mm5;
        jns       sumlp;

        mov       esi, [src];
        mov       edi, [dst];
        mov       ecx, [w];
        mov       ebx, [radius];
        shl       ebx, 3;
        lea       esi, [esi+ecx*8];
        lea       edi, [edi+ecx*8];
        lea       edx, [esi+ebx+8];
        sub       esi, ebx;
        neg       ecx;

        movd      mm2, [sc]; // load scale reg
        punpcklwd mm2, mm2;
        punpcklwd mm2, mm2;

pixlp:
        // calc color & store
        movq      mm4, mm0; // copy values
        movq      mm5, mm1;
        movq      mm6, mm0;
        movq      mm7, mm1;

        // perform scaling
        psrld     mm4, 16;
        psrld     mm5, 16;
        psrlw     mm6, 1;
        psrlw     mm7, 1;
        pmulhw    mm6, mm2;
        pmulhw    mm7, mm2;
        pmullw    mm4, mm2;
        pmullw    mm5, mm2;
        pand      mm6, [msk1];
        pand      mm7, [msk1];
        psllw     mm6, 1;
        psllw     mm7, 1;
        paddd     mm4, mm6;
        paddd     mm5, mm7;

        // re-pack & store
        packssdw  mm4, mm5;
        movq      [edi+ecx*8], mm4;

        // update state:
        movq      mm4, [edx+ecx*8]; // add val
        movq      mm6, [esi+ecx*8]; // sub val
        movq      mm5, mm4;
        movq      mm7, mm6;

        punpcklwd mm4, mm3; // splice values
        punpckhwd mm5, mm3;
        punpcklwd mm6, mm3;
        punpckhwd mm7, mm3;

        paddd     mm0, mm4; // add to state
        paddd     mm1, mm5;
        psubd     mm0, mm6; // sub from state
        psubd     mm1, mm7;

        // loop
        inc       ecx;
        jnz       pixlp;

        mov       [dst], edi;
      }
    }

    __asm emms;
  }

  static void flipAxes(const sU16 *src, sU16 *dst, sInt w, sInt h)
  {
    sInt ws=2, wt=w;

    while (wt>>=1)
      ws++;

    for (sInt x=0; x<w; x++)
    {
      const sU16 *s0=src+(x<<2);

      for (sInt y=0; y<h; y++)
      {
        const sU16 *so=s0+(y<<ws);

        ((sU32 *) dst)[0]=((const sU32 *) so)[0];
        ((sU32 *) dst)[1]=((const sU32 *) so)[1];
        dst+=4;
      }
    }
  }

  static void scaleIntensity(sU16 *data, sUInt count, sU16 sf)
  {
    while (count--)
    {
      sU32 v=(data[0]*sf)>>8;
      if (v>32767)
        v=32767;

      *data++=v;
    }
  }

  void doProcess()
  {
    const sInt wrap=parm>>2, steps=parm & 3;

    const sInt dw = data->w, dh = data->h;
    sU16* dptr = data->data;
    
    memcpy(dptr, getInputData(0)->data, dw * dh * 8); // first, copy over the data.

    if (rx)
    {
      for (sInt i=0; i<=steps; i++)
        inplaceBlur(dptr, dw, dh, rx, wrap);
    }

    if (ry)
    {
      sU16 *temp=new sU16[dw*dh*4];
      flipAxes(dptr, temp, dw, dh);

      for (sInt i=0; i<=steps; i++)
        inplaceBlur(temp, dh, dw, ry, wrap);

      flipAxes(temp, dptr, dh, dw);
      delete[] temp;
    }

    if (intens != 256)
      scaleIntensity(dptr, dw*dh*4, intens);
  }

public:
  const sU8* readData(const sU8* strm)
  {
    rx = *strm++;
    ry = *strm++;
    parm = *strm++;
    intens = *reinterpret_cast<const sU16*>(strm);
    strm += 2;

    return strm;
  }

	void setAnim(sInt index, const sF32* val)
	{
		switch (index)
		{
		case 1: // radius
			rx = val[0] - 0.5f;
			ry = val[1] - 0.5f;
			break;

		case 2: // intensity
			intens = val[0] * 256.0f;
			break;
		}
	}
};

frOperator* create_TFBlurV2() { return new frOpTFBlurV2; }

// ---- install font

class frOpTFInstallFont: public frTextureOperator
{
	sChar fontFile[256];

  void doProcess()
  {
		memcpy(data->data, getInputData(0)->data, data->w*data->h*4*sizeof(sU16));
  }

public:
	~frOpTFInstallFont()
	{
		RemoveFontResource(fontFile);
		DeleteFile(fontFile);
	}

	const sU8* readData(const sU8* strm)
	{
		sInt len = getSmallInt(strm);
		lstrcpy(fontFile, "c:\\temp.ttf");

		DWORD dwt;
		HANDLE hfl = CreateFile(fontFile, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
		WriteFile(hfl, strm, len, &dwt, 0);
		CloseHandle(hfl);	
		AddFontResource(fontFile);

		strm += len;

		return strm;
	}
};

frOperator* create_TFInstallFont() { return new frOpTFInstallFont; }
