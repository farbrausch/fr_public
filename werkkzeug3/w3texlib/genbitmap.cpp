// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "genbitmap.hpp"
#include "_start.hpp"
#include "mmintrin.h"
#include "xmmintrin.h"

#if sLINK_ENGINE
#include "genoverlay.hpp"
#include "genminmesh.hpp"
#include "rtmanager.hpp"
#endif

#define SCRIPTVERIFY(x) {if(!(x)) return 0;}
#define SCRIPTVERIFYVOID(x) {return 0; }

namespace Werkk3TexLib
{

#if 0
#define NOTEXTURES0() return NewBitmap(4,4);
#define NOTEXTURES1(a) if(a) a->Release(); return NewBitmap(4,4);
#define NOTEXTURES2(a,b) if(a) a->Release(); if(b) b->Release(); return NewBitmap(4,4);
#define NOTEXTURES3(a,b,c) if(a) a->Release(); if(b) b->Release(); if(c) c->Release(); return NewBitmap(4,4);
#else
#define NOTEXTURES0()
#define NOTEXTURES1(a)
#define NOTEXTURES2(a,b)
#define NOTEXTURES3(a,b,c)
#endif

// rules:
// - first all SCRIPTVERIFY
// - then CheckBitmap()
// - after CheckBitmap you may not return 0
//
// if return 0, then caller calls Release() for every argument.
// if return !=0, then routine calls Release() for every argument
// (except what it may return) 

/****************************************************************************/

sInt GenBitmapTextureSizeOffset;         // 0 = normal, -1 = smaller, 1 = large

#define BI_ADD        0
#define BI_SUB        1
#define BI_MUL        2
#define BI_DIFF       3
#define BI_ALPHA      4
#define BI_MULCOL     5
#define BI_ADDCOL     6
#define BI_SUBCOL     7
#define BI_GRAY       8
#define BI_INVERT     9
#define BI_SCALECOL   10
#define BI_MERGE      11
#define BI_BRIGHTNESS 12
#define BI_SUBR       13
#define BI_MULMERGE   14
#define BI_SHARPEN    15

#define BI_HARDLIGHT  0x10
#define BI_OVER       0x11
#define BI_ADDSMOOTH  0x12
#define BI_MIN        0x13
#define BI_MAX        0x14

#define BI_RANGE      0x15    // alway last

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
  xs = sRange(xs,12,0);
  ys = sRange(ys,12,0);
  bm = new GenBitmap;
  bm->Init(1<<xs,1<<ys);
  return bm;
}

static sBool CheckBitmap(GenBitmap *&bm,GenBitmap **inbm=0)
{
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
  return 0;
}

sInt sRange7fff(sInt a)
{
  if((unsigned) a < 0x7fff)
    return a;
  else if(a<0)
    return 0;
  else
    return 0x7fff;
}

sU64 GetColor64(const sInt *i)
{
  sU64 col;
  col = ((sU64)sRange7fff(i[2]/2))<<0 
      | ((sU64)sRange7fff(i[1]/2))<<16 
      | ((sU64)sRange7fff(i[0]/2))<<32 
      | ((sU64)sRange7fff(i[3]/2))<<48;

  return col;
}

sU64 GetColor64(sU32 c)
{
  sU64 col;
  col = c;
  col = ((col&0xff000000)<<24) 
      | ((col&0x00ff0000)<<16) 
      | ((col&0x0000ff00)<< 8) 
      | ((col&0x000000ff)    );
  col = ((col | (col<<8))>>1)&0x7fff7fff7fff7fff;
  
  return col;
}

sU32 GetColor32(const sInt4 &colx)
{
  sU32 col;
  col = (((sU32)sRange<sInt>(colx.z>>8,0xff,0))<<0 )
      | (((sU32)sRange<sInt>(colx.y>>8,0xff,0))<<8 )
      | (((sU32)sRange<sInt>(colx.x>>8,0xff,0))<<16) 
      | (((sU32)sRange<sInt>(colx.w>>8,0xff,0))<<24);

  return col;
}

void __forceinline Fade64(sU64 &r,sU64 &c0,sU64 &c1,sInt fade)
{
  static const sU64 xor0 = 0xffffffff00000000;
  static const sU64 add0 = 0x0000800100000000;

  __asm
  {
    mov       eax, [c0];
    movq      mm2, [eax];
    mov       eax, [c1];
    movq      mm3, [eax];

    movd      mm0, [fade];
    punpckldq mm0, mm0;
    psrad     mm0, 1;
    pxor      mm0, [xor0];
    paddd     mm0, [add0];
    packssdw  mm0, mm0;
    punpcklwd mm0, mm0;
    movq      mm1, mm0;
    punpcklwd mm0, mm0;
    punpckhwd mm1, mm1;
    
    mov       eax, [r];
    pmulhw    mm3, mm0;
    pmulhw    mm2, mm1;
    paddw     mm2, mm3;
    psllw     mm2, 1;
    movq      [eax], mm2;

    emms;
  }
}

void AddScale64(sU64 &r,sU64 &c0,sU64 &c1,sInt fade)
{
  __asm
  {
    mov       eax, [c0];
    movq      mm2, [eax];
    mov       eax, [c1];
    movq      mm3, [eax];

    movd      mm0, [fade];
    psrad     mm0, 1;
    packssdw  mm0, mm0;
    punpcklwd mm0, mm0;
    punpcklwd mm0, mm0;

    mov       eax, [r];
    pmulhw    mm3, mm0;
    psllw     mm3, 1;
    paddsw    mm2, mm3;
    movq      [eax], mm2;

    emms;
  }
}

void BilinearSetup(BilinearContext *ctx,sU64 *src,sInt w,sInt h,sInt b)
{
  ctx->Src = src;
  ctx->XShift = sGetPower2(w)+3;
  ctx->XSize1 = (w-1) << 3;
  ctx->YSize1 = h-1;
  ctx->XMax1 = (w << 16) - 1;
  ctx->YMax1 = (h << 16) - 1;
  ctx->XAm = (b&1) ? 0xffffffff : ctx->XMax1;
  ctx->YAm = (b&2) ? 0xffffffff : ctx->YMax1;
}

void __forceinline BilinearFilter(BilinearContext *ctx,sU64 *r,sInt u,sInt v)
{
  static const sU64 adjust = 0x8000800080008000;

  __asm
  {
    mov       esi, [ctx];
    mov       edi, [esi]BilinearContext.Src;

    // clamp u, v
    mov       eax, [u];
    mov       ebx, [v];
    and       eax, [esi]BilinearContext.XAm;
    and       ebx, [esi]BilinearContext.YAm;
    cmp       eax, [esi]BilinearContext.XMax1;
    jbe       okx;
    sar       eax, 31;
    not       eax;
    and       eax, [esi]BilinearContext.XMax1;

okx:
    cmp       ebx, [esi]BilinearContext.YMax1;
    jbe       oky;
    sar       ebx, 31;
    not       ebx;
    and       ebx, [esi]BilinearContext.YMax1;

oky:
    movd      mm0, eax;
    movd      mm1, ebx;

    // calc s0, s1, u0, u1
    mov       ecx, [esi]BilinearContext.XShift;
    shr       eax, 13;      // u0 (pix offset)
    shr       ebx, 16;      // v0 (line count)
    and       eax, not 7;   // eax=u0
    lea       edx, [ebx+1];
    shl       ebx, cl;
    and       edx, [esi]BilinearContext.YSize1;
    add       ebx, edi; // ebx=s0
    shl       edx, cl;
    lea       ecx, [eax+8];
    add       edi, edx; // edi=s1
    and       ecx, [esi]BilinearContext.XSize1; // ecx=u1

    // load pixels
    movq      mm2, [ebx+eax];
    movq      mm3, [ebx+ecx];
    movq      mm4, [edi+eax];
    movq      mm5, [edi+ecx];

    // actual sample
    movq      mm6, [adjust];
    punpcklwd mm0, mm0;
    punpcklwd mm1, mm1;
    mov       eax, [r];
    punpcklwd mm0, mm0;
    punpcklwd mm1, mm1;
    psrlw     mm0, 1;
    psrlw     mm1, 1;

    // horiz
    psubw     mm3, mm2;
    psubw     mm5, mm4;
    pmulhw    mm3, mm0;
    pmulhw    mm5, mm0;
    psllw     mm3, 1;
    psllw     mm5, 1;
    paddw     mm2, mm3;
    paddw     mm4, mm5;

    // vert+write
    psubw     mm4, mm2;
    paddw     mm2, mm6;
    pmulhw    mm4, mm1;
    psllw     mm4, 1;
    paddw     mm2, mm4;
    psubusw   mm2, mm6;
    movntq    [eax], mm2;
    
    emms;
  }
}

static sInt GammaTable[1025];

static sInt GetGamma(sInt value)
{
  sInt vi = value >> 5;
  return GammaTable[vi] + (((GammaTable[vi+1]-GammaTable[vi]) * (value & 31)) >> 5);
}

/****************************************************************************/
/***                                                                      ***/
/***   Operators                                                          ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/

GenBitmap * __stdcall Bitmap_Flat(sInt xs,sInt ys,sU32 color)
{
  NOTEXTURES0();

  GenBitmap *bm;

  bm = NewBitmap(xs,ys);
  sSetMem8(bm->Data,GetColor64(color),bm->Size);

  return bm;
}

GenBitmap * __stdcall Bitmap_Format(GenBitmap *bm,sInt format,sInt count,sInt tresh)
{
  NOTEXTURES1(bm);

  SCRIPTVERIFY(format>0);
  if(CheckBitmap(bm)) return 0;
  bm->Format = format;
  bm->TexMipCount = count;
  bm->TexMipTresh = tresh;
  return bm;
}

#if sLINK_ENGINE
GenBitmap * __stdcall Bitmap_RenderTarget(sInt xs,sInt ys,sU32 format)
{
  GenBitmap *bm;
  bm = new GenBitmap;
  bm->Format = format;
  bm->XSize = 1<<xs;
  bm->YSize = 1<<ys;
  bm->Texture = sSystem->AddTexture(1<<xs,1<<ys,format,0);
  return bm;
}
#endif

/****************************************************************************/

void __stdcall Bitmap_Inner(sU64 *d,sU64 *s,sInt count,sInt mode,sU64 *x=0)
{
  static const sU64 mask1 = 0x8000800080008000;

  __asm
  {
    emms

    mov       esi,[s]
    mov       edi,[d]
    mov       ebx,[x]
    mov       ecx,[count]
    mov       edx,[mode]

    pcmpeqb	  mm3,mm3;
		psrlw	    mm3,1;              ; mm3 = 0x7fff7fff7fff7fff
    pcmpeqb	  mm4,mm4
    psrlq		  mm4,16              ; mm4 = 0x00007fff7fff7fff
    pcmpeqb	  mm5,mm5
    pxor		  mm5,mm4             ; mm5 = 0x7fff000000000000
    psrlw     mm4,1
    psrlw     mm5,1
    movq      mm2,[esi]           ; mm2 = data[0]
    pcmpeqb	  mm7,mm7;
		psrlw	    mm7,2;              ; mm7 = 0x3fff3fff3fff3fff
    pxor      mm6,mm6             ; mm6 = 0x0000000000000000

    shl       ecx,3;              ; ecx = number of bytes
    add       esi,ecx;            ; esi = end of source
    add       edi,ecx;            ; edi = end of dest
    add       ebx,ecx;            ; ebx = end of source2
    neg       ecx

    lea       eax,loop0
    dec       edx
    js        loop_start

    lea       eax,loop1
    dec       edx
    js        loop_start

    lea       eax,loop2
    dec       edx
    js        loop_start

    lea       eax,loop3
    dec       edx
    js        loop_start

    lea       eax,loop4
    dec       edx
    js        loop_start

    lea       eax,loop5
    dec       edx
    js        loop_start

    lea       eax,loop6
    dec       edx
    js        loop_start

    lea       eax,loop7
    dec       edx
    js        loop_start

    lea       eax,loop8
    dec       edx
    js        loop_start

    lea       eax,loop9
    dec       edx
    js        loop_start

    lea       eax,loopa
    dec       edx
    js        loop_start

    lea       eax,loopb
    dec       edx
    js        loop_start

    lea       eax,loopc
    dec       edx
    js        loop_start

    lea       eax,loopd
    dec       edx
    js        loop_start

    lea       eax,loop0e
    dec       edx
    js        loop_start

    lea       eax,loop0f
    dec       edx
    js        loop_start

    lea       eax,loop10
    dec       edx
    js        loop_start

    lea       eax,loop11
    dec       edx
    js        loop_start

    lea       eax,loop12
    dec       edx
    js        loop_start

    lea       eax,loop13
    dec       edx
    js        loop_start

    lea       eax,loop14
    dec       edx
    js        loop_start

    movq      mm7,[esi+ecx+8]             ; mm7 = data[1]
    psubsw    mm7,mm2
    pxor      mm1,mm1
    lea       eax,loop15

loop_start:
    jmp       eax
loop0:                        // add
    movq      mm0,[ebx+ecx];
    paddsw    mm0,[esi+ecx];
    jmp       loopo;

loop1:                        // sub
    movq      mm0,[ebx+ecx];
    psubusw   mm0,[esi+ecx];
    jmp       loopo

loop2:                        // mul
    movq      mm0,[ebx+ecx];
    pmulhw    mm0,[esi+ecx];
		psllw		  mm0,1;
    jmp       loopo

loop3:                        // diff
    movq      mm0,[ebx+ecx];
    psubw     mm0,[esi+ecx];
    paddw     mm0,mm3
    psrlw     mm0,1
    jmp       loopo

loop4:                        // alpha
    movq      mm0,[esi+ecx]
    movq      mm1,[ebx+ecx]
		movq		  mm6,mm0
		movq		  mm7,mm0

		psrlq		  mm6,16
		psrlq		  mm7,32
		paddw		  mm0,mm6
		paddw		  mm7,mm6
		psrlw		  mm0,1
		psrlw		  mm7,1
		paddw		  mm0,mm7
		pand		  mm1,mm4
		psllq		  mm0,47
		pand		  mm0,mm5
		por			  mm0,mm1
    jmp       loopo


loop5:                      // mul color
    movq      mm0,[ebx+ecx]
	  pmulhw	  mm0,mm2
	  psllw	    mm0,1
    jmp       loopo

loop6:                      // add color
    movq      mm0,[ebx+ecx]
    paddsw    mm0,mm2
    jmp       loopo

loop7:                      // sub color
    movq      mm0,[ebx+ecx]
    psubusw   mm0,mm2
    jmp       loopo

loop8:                      // gray color
    movq      mm0,[ebx+ecx]
		movq			mm6, mm0;
		movq			mm7, mm0;

		psrlq			mm6, 16;
		psrlq			mm7, 32;
		paddw			mm0, mm6;
		paddw			mm7, mm6;
		psrlw			mm0, 1;
		psrlw			mm7, 1;
		paddw			mm0, mm7;
		psrlw			mm0, 1;

		punpcklwd	mm0, mm0;
		punpcklwd	mm0, mm0;
		por				mm0, mm5;
    jmp       loopo

loop9:                      // invert color
    movq      mm0,[ebx+ecx]
    pxor      mm0,mm3
    jmp       loopo

loopa:                      // scale color
		movq			mm0, [ebx+ecx];
		movq			mm1, mm0;
		pmullw		mm0, mm2;
		pmulhw		mm1, mm2;
		movq			mm6, mm0;
		punpcklwd	mm0, mm1;
		punpckhwd	mm6, mm1;
		psrld			mm0, 11;
		psrld			mm6, 11;
		packssdw	mm0, mm6;
    jmp       loopo

loopb:                      // merge
    movq      mm0,[edi+ecx];
    movq      mm4,[esi+ecx];
    movq      mm5,[ebx+ecx];
    movq      mm1,mm3;
    psubsw    mm1,mm0;
    pmulhw    mm0,mm4;
    pmulhw    mm1,mm5;
    paddsw    mm0,mm1;
    psllw     mm0,1
    jmp       loopo

loopc:                      // brigtness
    movq      mm0,[esi+ecx];
    movq      mm1,[ebx+ecx];
    movq      mm2,mm0
    pcmpgtw   mm2,mm7
    psrlw     mm2,1

    pxor      mm0,mm2
    pxor      mm1,mm2
	  pmulhw	  mm0,mm1
	  psllw	    mm0,2
    pxor      mm0,mm2
    jmp       loopo


loopd:                        // subr
    movq      mm0,[esi+ecx];
    psubusw   mm0,[ebx+ecx];
    jmp       loopo

loop0e:                      // mulmerge
    movq      mm0,[edi+ecx];
    movq      mm4,[esi+ecx];
    movq      mm5,[ebx+ecx];
    movq      mm1,mm3
    pmulhw    mm4,mm5
    psllw     mm4,1
    psubsw    mm1,mm0
    pmulhw    mm0,mm4
    pmulhw    mm1,mm5
    paddsw    mm0,mm1
    psllw     mm0,1
    jmp       loopo

loop0f:                     // sharpen
    movq      mm0,[ebx+ecx]
    movq      mm3,mm0
    psubusw   mm0,[edi+ecx]

		movq			mm1,mm0
		pmullw		mm0,mm2
		pmulhw		mm1,mm2

    movq			mm7,mm0
		punpcklwd	mm0,mm1
		punpckhwd	mm7,mm1
		psrad			mm0,11
		psrad			mm7,11
		packssdw	mm0,mm7

    paddsw    mm0,mm3
    pmaxsw    mm0,mm6
    jmp       loopo

loop10:                      // hardlight
    movq      mm1,[esi+ecx];
    movq      mm0,[ebx+ecx];
    psllw     mm1,1;
    movq      mm2,mm1;
    pand      mm1,mm3;
    psraw     mm2,15;
    movq      mm4,mm0;
    pmulhw    mm0,mm1;
    psllw     mm0,1;
    paddw     mm1,mm4;
    pxor      mm0,mm2;
    pand      mm1,mm2;
    paddw     mm0,mm1;
    jmp       loopo

loop11:                      // over
    movq      mm0,[ebx+ecx];
    movq      mm1,[esi+ecx];
    movq      mm2,mm1;
    psubsw    mm1,mm0;
    punpckhwd mm2,mm2;
    punpckhwd mm2,mm2;
    pmulhw    mm1,mm2;
    psllw     mm1,1;
    paddsw    mm0,mm1;
    pmaxsw    mm0,mm6;
    /*paddw     mm0,[mask1];
    psubsw    mm0,[mask1];*/
    jmp       loopo

loop12:                      // addsmooth (screen)
    movq      mm0,[ebx+ecx];
    movq      mm1,[esi+ecx];
    pxor      mm0,mm3;
    pxor      mm1,mm3;
    pmulhw    mm0,mm1;
    psllw     mm0,1;
    pxor      mm0,mm3;

    jmp       loopo;

loop13:                       // min
    movq      mm0,[ebx+ecx];
    pminsw    mm0,[esi+ecx];
    jmp       loopo;

loop14:                       // max
    movq      mm0,[ebx+ecx];
    pmaxsw    mm0,[esi+ecx];
    jmp       loopo;

loop15:                      // color range
    movq      mm0,[ebx+ecx]
    pmulhw    mm0,mm7
    psllw     mm0,1
    paddsw    mm0,mm2
    pmaxsw    mm0,mm1
    /*paddw     mm0,mm6
    psubusw   mm0,mm6*/

loopo:
    movq      [edi+ecx],mm0;
    add       ecx, 8;
    je        ende
    jmp       eax

ende:
    emms
  }
}

GenBitmap * __stdcall Bitmap_Merge(sInt mode,sInt count,GenBitmap *b0,...)
{
  sInt i;
  GenBitmap *bi,*in;
  SCRIPTVERIFY(b0);
  SCRIPTVERIFY(b0->ClassId==KC_BITMAP);
  SCRIPTVERIFY(mode>=0 && mode<11);
  static sU8 modes[] = 
  {
    BI_ADD,BI_SUB,BI_MUL,BI_DIFF,BI_ALPHA,
    BI_BRIGHTNESS,
    BI_HARDLIGHT,BI_OVER,BI_ADDSMOOTH,
    BI_MIN,BI_MAX
  };
  mode = modes[mode];
  for(i=1;i<count && (&b0)[i];i++)
  {
    bi = ((&b0)[i]);
    SCRIPTVERIFY(bi->ClassId==KC_BITMAP);
    if(b0->XSize!=bi->XSize || b0->YSize!=bi->YSize)
      return 0;
  }

  if(count == 1)
    return CheckBitmap(b0) ? 0 : b0;

  if(CheckBitmap(b0,&in)) return 0;

  i = 1;
  while(i<count && (&b0)[i])
  {
    bi = ((&b0)[i]);
    sVERIFY(b0->XSize==bi->XSize && b0->YSize==bi->YSize);
    Bitmap_Inner(b0->Data,bi->Data,b0->Size,mode,in->Data);
    bi->Release();
    in = b0;
    i++;
  }

  return b0;
}

GenBitmap * __stdcall Bitmap_Color(GenBitmap *bm,sInt mode,sU32 col)
{
  NOTEXTURES1(bm);
  sU64 color;
  GenBitmap *in;

  SCRIPTVERIFY(mode>=0 && mode<6);
  if(CheckBitmap(bm,&in)) return 0;

  color = GetColor64(col);
  Bitmap_Inner(bm->Data,&color,bm->Size,mode+BI_MULCOL,in->Data);

  return bm;
}

GenBitmap * __stdcall Bitmap_Range(GenBitmap *bm,sInt mode,sU32 col0,sU32 col1)
{
  NOTEXTURES1(bm);
  sU64 cx[2];
  GenBitmap *in;

  SCRIPTVERIFY(mode>=0 && mode<6);
  if(CheckBitmap(bm,&in)) return 0;

  cx[0] = GetColor64(col0);
  cx[1] = GetColor64(col1);
  if(mode&1)
  {
    Bitmap_Inner(bm->Data,cx,bm->Size,BI_GRAY,in->Data);
    in = bm;
  }
  if(mode&2)
  {
    Bitmap_Inner(bm->Data,cx,bm->Size,BI_INVERT,in->Data);
    in = bm;
  }
  Bitmap_Inner(bm->Data,cx,bm->Size,BI_RANGE,in->Data);

  return bm;
}

/****************************************************************************/

GenBitmap * __stdcall Bitmap_GlowRect(GenBitmap *bm,sF32 cx,sF32 cy,sF32 rx,sF32 ry,sF32 sx,sF32 sy,sU32 color,sF32 alpha,sF32 power,sU32 wrap,sU32 bug)
{
  NOTEXTURES1(bm);

  sU64 *d;
  sInt x,y;
  sF32 a;
  sInt f,fm;
  sU64 col;
  sF32 fx,fy,tresh;
  sInt LowTable[32];
  sBool circular = (bug & 2) == 0;

  if(wrap==1)
  {
    if(cx+rx+sx> 1.0f) bm = Bitmap_GlowRect(bm,cx-1.0f,cy,rx,ry,sx,sy,color,alpha,power,2,bug);
    if(cx-rx-sx<-0.0f) bm = Bitmap_GlowRect(bm,cx+1.0f,cy,rx,ry,sx,sy,color,alpha,power,2,bug);
  }
  if(wrap==1 || wrap==2)
  {
    if(cy+ry+sy> 1.0f) bm = Bitmap_GlowRect(bm,cx,cy-1.0f,rx,ry,sx,sy,color,alpha,power,0,bug);
    if(cy-ry-sy<-0.0f) bm = Bitmap_GlowRect(bm,cx,cy+1.0f,rx,ry,sx,sy,color,alpha,power,0,bug);
  }

  if(CheckBitmap(bm)) return 0;

  if(power==0) power=(1.0f/65536.0f);
  power = 0.25/power;

  cx*=bm->XSize;
  cy*=bm->YSize;
  rx*=bm->XSize;
  ry*=bm->YSize;
  sx*=bm->XSize;
  sy*=bm->YSize;

  // bounding box
  sInt x0 = sMax<sInt>(cx-rx-sx-1,0);
  sInt x1 = sMin<sInt>(cx+rx+sx+1,bm->XSize);
  sInt y0 = sMax<sInt>(cy-ry-sy-1,0);
  sInt y1 = sMin<sInt>(cy+ry+sy+1,bm->YSize);

  sInt icx=cx,icy=cy,isx=sx,isy=sy;

  tresh = 1.0f/65536.0f;
  if(rx<tresh) rx=tresh;
  rx = circular ? 1.0f/(rx*rx) : 1.0f / rx;

  if(ry<tresh) ry=tresh;
  ry = circular ? 1.0f/(ry*ry) : 1.0f / ry;

  alpha *= 32768.0f;
  col = GetColor64(color);
  fm = alpha;

  for(x=0;x<1025;x++)
  {
    if(bug & 1)
      GammaTable[x] = sRange7fff(sFPow(1.0f-x/1024.0f,power)*alpha)*2;
    else
      GammaTable[x] = sRange7fff((1.0f-sFPow(x/1024.0f,power*2.0f))*alpha)*2;
  }

  // there are very steep slopes around 0, so don't try approximating them
  for(x=0;x<32;x++)
  {
    if(bug & 1)
      LowTable[x] = sRange7fff(sFPow(1.0f-x/32768.0f,power)*alpha)*2;
    else
      LowTable[x] = sRange7fff((1.0f-sFPow(x/32768.0f,power*2.0f))*alpha)*2;
  }

  for(y=y0;y<y1;y++)
  {
    fy = sFAbs(y-cy)-sy;
    if(fy<0) fy = 0;
    fy *= circular ? fy*ry : ry;

    d = bm->Data + y * bm->XSize + x0;

    for(x=x0;x<x1;x++)
    {
      fx = sFAbs(x-cx)-sx;
      if(fx<0) fx = 0;
      
      a = circular ? fx*fx*rx+fy : sMax(fx*rx,fy);
      if(a<1.0f-1.0f/32768.0f) // to cull a few more pixels...
      {
        f = sFtol(a*32768);
        if(f<32)
          f = LowTable[f];
        else
          f = GetGamma(f);

        Fade64(*d,*d,col,f);
      }

      d++;
    }
  }

  return bm;
}

/****************************************************************************/

GenBitmap * __stdcall Bitmap_Dots(GenBitmap *bm,sU32 color0,sU32 color1,sInt count,sInt seed)
{
  NOTEXTURES1(bm);

  sU64 *d;
  sInt f;
  sInt x;
  sU64 c0,c1;


  if(CheckBitmap(bm)) return 0;

  c0 = GetColor64(color0);
  c1 = GetColor64(color1);
  count = (bm->Size*count)>>12;

  sSetRndSeed(seed);
 
  d = bm->Data;

  do
  {
    f = sGetRnd(0x10000);
    x = sGetRnd(bm->Size);
    Fade64(d[x],c0,c1,f);
    count--;
  }
  while(count>0);
  return bm;
}

/****************************************************************************/
/***                                                                      ***/
/***   HSCB                                                               ***/
/***                                                                      ***/
/****************************************************************************/

GenBitmap * __stdcall Bitmap_HSCB(GenBitmap *bm,sF32 fh,sF32 fs,sF32 fc,sF32 fb)
{
  NOTEXTURES1(bm);
  GenBitmap *in;

  if(CheckBitmap(bm,&in)) return 0;

  sU16 *d,*s;
  sInt i;
  sInt ch,ffh,ffs;
  sInt cr,cg,cb,min,max,mm;

  d = (sU16 *) bm->Data;
  s = (sU16 *) in->Data;
  
  fc = fc*fc;
  for(i=0;i<1025;i++)
    GammaTable[i] = sFPow((i*32+0.01)/32768.0f,fc)*32768.0f*fb;

  ffh = sInt(fh * 6 * 65536) % (6*65536);
  ffs = fs * 65536;

  for(i=0;i<bm->Size;i++)
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

    cr = GetGamma(s[2]);
    cg = GetGamma(s[1]);
    cb = GetGamma(s[0]);

// convert to hsv
    if(ffh || ffs!=65536)
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
  return bm;
}

/****************************************************************************/

GenBitmap * __stdcall Bitmap_Wavelet(GenBitmap *bm,sInt mode,sInt count)
{
  NOTEXTURES1(bm);

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
          
          d[x*8+0] = sRange<sInt>(s[x*4+0+xs*2]-(s[x*4+0]-0x4000),0x7fff,0);
          d[x*8+1] = sRange<sInt>(s[x*4+1+xs*2]-(s[x*4+1]-0x4000),0x7fff,0);
          d[x*8+2] = sRange<sInt>(s[x*4+2+xs*2]-(s[x*4+2]-0x4000),0x7fff,0);
          d[x*8+3] = sRange<sInt>(s[x*4+3+xs*2]-(s[x*4+3]-0x4000),0x7fff,0);
          d[x*8+4] = sRange<sInt>(s[x*4+0+xs*2]+(s[x*4+0]-0x4000),0x7fff,0);
          d[x*8+5] = sRange<sInt>(s[x*4+1+xs*2]+(s[x*4+1]-0x4000),0x7fff,0);
          d[x*8+6] = sRange<sInt>(s[x*4+2+xs*2]+(s[x*4+2]-0x4000),0x7fff,0);
          d[x*8+7] = sRange<sInt>(s[x*4+3+xs*2]+(s[x*4+3]-0x4000),0x7fff,0);
          
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
          d[y*g+0  ] = sRange<sInt>(s[y*f+0+ys*f/2]-(s[y*f+0]-0x4000),0x7fff,0);          
          d[y*g+1  ] = sRange<sInt>(s[y*f+1+ys*f/2]-(s[y*f+1]-0x4000),0x7fff,0);
          d[y*g+2  ] = sRange<sInt>(s[y*f+2+ys*f/2]-(s[y*f+2]-0x4000),0x7fff,0);
          d[y*g+3  ] = sRange<sInt>(s[y*f+3+ys*f/2]-(s[y*f+3]-0x4000),0x7fff,0);
          d[y*g+0+f] = sRange<sInt>(s[y*f+0+ys*f/2]+(s[y*f+0]-0x4000),0x7fff,0);
          d[y*g+1+f] = sRange<sInt>(s[y*f+1+ys*f/2]+(s[y*f+1]-0x4000),0x7fff,0);
          d[y*g+2+f] = sRange<sInt>(s[y*f+2+ys*f/2]+(s[y*f+2]-0x4000),0x7fff,0);
          d[y*g+3+f] = sRange<sInt>(s[y*f+3+ys*f/2]+(s[y*f+3]-0x4000),0x7fff,0);
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

static void BlurCore(sU16 *p,sU16 *q,sInt sizek,sInt xRes,sInt f0,sInt f1,sInt amp,sInt ampc)
{
  __m64 f0f1,amplo,amphi,ampclip;
  sInt mask;

  __asm
  {
    movd      mm0, [f0];
    punpcklwd mm0, [f1];
    punpckldq mm0, mm0;
    movq      [f0f1], mm0;

    movd      mm0, [ampc];
    movd      mm1, [amp];
    punpcklwd mm1, mm1;
    punpckldq mm0, mm0;
    movq      mm2, mm1;
    movq      [ampclip], mm0;
    punpcklwd mm1, mm1;
    punpckhwd mm2, mm2;
    movq      [amplo], mm1;
    movq      [amphi], mm2;

    mov       eax, [f1];
    sub       eax, [f0];
    movzx     eax, ax;
    movd      mm2, eax;
    punpckldq mm2, mm2;

    mov       esi, [p];
    mov       edi, [q];
    mov       edx, [xRes];
    dec       edx;
    shl       edx, 3;
    mov       [mask], edx;

    mov       eax, [sizek];
    inc       eax;
    sar       eax, 1;
    neg       eax;
    shl       eax, 3;
    and       eax, edx;
    mov       edx, eax;


    // mm0 = akku0
    // mm1 = akku1
    // eax = s1*2 & mask*2
    // edx = s2*2 & mask*2
    // ecx = counter
    // esi = p
    // edi = q+d

    // seed first pixel
    movq      mm0, [esi+edx]
    movq      mm1, mm0
    punpcklwd mm0, mm0
    punpckhwd mm1, mm1
    pmaddwd   mm0, mm2
    pmaddwd   mm1, mm2
    
    // init loop
    mov       ecx, [sizek]
    test      ecx, ecx
    jz        xstartmain
    movq      mm5, [f0f1]
  
xinitloop:
    movq      mm2, [esi+eax]
    add       eax, 8
    and       eax, [mask];
    movq      mm3, [esi+eax]
    movq      mm4, mm2
    punpcklwd mm2, mm3
    punpckhwd mm4, mm3
    pmaddwd   mm2, mm5
    pmaddwd   mm4, mm5
    paddd     mm0, mm2
    paddd     mm1, mm4
    sub       ecx, 1
    jnz       xinitloop

xstartmain:
    mov       ecx, [xRes]
  
xmainloop:
    movq      mm2, [esi+eax]
    movq      mm5, [f0f1]
    add       eax, 8
    and       eax, [mask]
    movq      mm3, [esi+eax]
    movq      mm4, mm2
    punpcklwd mm2, mm3        // mix1
    punpckhwd mm4, mm3        // mix2
    pmaddwd   mm2, mm5
    pmaddwd   mm4, mm5
    paddd     mm0, mm2
    paddd     mm1, mm4

    movq      mm2, mm0
    movq      mm3, mm1
    movq      mm5, [ampclip]
    psrad     mm2, 6
    psrad     mm3, 6
    movq      mm6, mm0
    movq      mm7, mm1
    movq      mm4, mm2
    punpcklwd mm2, mm3
    punpckhwd mm4, mm3
    pcmpgtd   mm6, mm5
    pcmpgtd   mm7, mm5
    movq      mm5, [amplo]
    movq      mm3, mm2
    punpcklwd mm2, mm4        // mm2=aklo
    punpckhwd mm3, mm4        // mm3=akhi
    movq      mm4, [amphi]
    packssdw  mm6, mm7        // mm6=out
    pcmpeqb   mm7, mm7
    pmullw    mm3, mm5        // mm3=akhi*amplo
    pmullw    mm4, mm2        // mm4=aklo*amphi
    pmulhuw   mm2, mm5        // mm2=aklo*amplo
    psllw     mm7, 15
    paddusw   mm6, mm3        // out+=akhi*amplo
    movq      mm3, [esi+edx]  // pix1
    add       edx, 8
    paddusw   mm6, mm4        // out+=aklo*amphi
    and       edx, [mask]
    movq      mm5, [f0f1]
    paddusw   mm6, mm2        // out+=aklo*amplo
    movq      mm2, [esi+edx]  // pix2
    paddw     mm6, mm7        // out+=add
    movq      mm4, mm2
    psubsw    mm6, mm7        // out-=add
    punpcklwd mm2, mm3        // mix1
    punpckhwd mm4, mm3        // mix2
    pmaddwd   mm2, mm5
    pmaddwd   mm4, mm5
    movq      [edi], mm6      // *(q+d) = out
    add       edi, 8
    psubd     mm0, mm2
    psubd     mm1, mm4
    sub       ecx, 1
    jnz       xmainloop

    emms
  }
}

// registers are tight here, we don't want a frame pointer
#pragma optimize("y",on)

GenBitmap * __stdcall Bitmap_Blur(GenBitmap *bm,sInt flags,sF32 sx,sF32 sy,sF32 _amp)
{
  NOTEXTURES1(bm);

  if(CheckBitmap(bm)) return 0;
  
  sInt x,y;
  sInt size,size2;
  sU32 mask;
  sInt s1,s2;
  sU16 *p,*q,*pp,*qq,*po,*qo;
  sInt ordercount;
  sInt amp,ampc,famp;
  sInt f0,f1;
  sInt order;

// prepare

  order = flags & 15;
  if(order==0) return bm;

  pp = (sU16 *)bm->Data;
  qq = (sU16 *) new sU64[bm->Size];

// blur x

  size = sFtol(128*sx*bm->XSize);
  size2 = sFtol(128*sy*bm->YSize);
  famp = _amp * 65536.0f * 64;

  sInt repeat = 1;
  do
  {
    f1 = (size&127)/2;
    f0 = 64-f1;
    size = (size/128)*2;
    if(flags&0x10)
      size++;
    
    amp = famp / sMax<sInt>(1,size*64+f1*2);
    if(amp > 128)
      ampc = sDivShift(65536*64,amp) - 1;
    else
      ampc = 0x7fffffff;

    mask = bm->XSize * 4 - 1;
    po = pp;
    qo = qq;
    y = 0;

    do
    {
      pp = po + y*bm->XSize*4;
      qq = qo + y*bm->XSize*4;
      ordercount = order;

      do
      {
        p = pp;
        q = qq;
        pp = q;
        qq = p;

        BlurCore(p,q,size,bm->XSize,f0,f1,amp,ampc);
        ordercount--;
      }
      while(ordercount);
      y++;
    }
    while(y<bm->YSize);
    pp = po;
    qq = qo;
    if(order&1)
      sSwap(pp,qq);

    p = pp;
    q = qq;
    pp = q;
    qq = p;
    s1 = bm->YSize * 4;
    s2 = bm->XSize * 4;
    for(x=0;x<bm->XSize;x+=8)
    {
      p = qq + x*4;
      for(y=0;y<bm->YSize;y++)
      {
        __m64 m0,m1,m2,m3,m4,m5,m6,m7;
        m0 = *((__m64 *)(p+ 0));
        m1 = *((__m64 *)(p+ 4));
        m2 = *((__m64 *)(p+ 8));
        m3 = *((__m64 *)(p+12));
        m4 = *((__m64 *)(p+16));
        m5 = *((__m64 *)(p+20));
        m6 = *((__m64 *)(p+24));
        m7 = *((__m64 *)(p+28));
        *((__m64 *)(q+0*s1)) = m0;
        *((__m64 *)(q+1*s1)) = m1;
        *((__m64 *)(q+2*s1)) = m2;
        *((__m64 *)(q+3*s1)) = m3;
        *((__m64 *)(q+4*s1)) = m4;
        *((__m64 *)(q+5*s1)) = m5;
        *((__m64 *)(q+6*s1)) = m6;
        *((__m64 *)(q+7*s1)) = m7;
        q += 4;
        p += s2;
      }
      q += 7*s1;
    }
    sSwap(bm->XSize,bm->YSize);
    size = size2;
  }
  while(repeat--);
  
  __asm { emms };
  sVERIFY(qq!=(sU16 *)bm->Data);
  delete[] qq;

  return bm;
}

#pragma optimize("",on)

/****************************************************************************/
/***                                                                      ***/
/***   Mask                                                               ***/
/***                                                                      ***/
/****************************************************************************/

GenBitmap * __stdcall Bitmap_Mask(GenBitmap *bm,GenBitmap *bb,GenBitmap *bc,sInt mode)
{
  NOTEXTURES1(bm);

  sInt size;
  GenBitmap *in;

  SCRIPTVERIFY(bb);
  SCRIPTVERIFY(bc);
  SCRIPTVERIFY(bb->ClassId==KC_BITMAP);
  SCRIPTVERIFY(bc->ClassId==KC_BITMAP);
  SCRIPTVERIFY(bm->Size==bb->Size);
  SCRIPTVERIFY(bm->Size==bc->Size);
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

GenBitmap * __stdcall Bitmap_Rotate(GenBitmap *bm,sF32 angle,sF32 sx,sF32 sy,sF32 tx,sF32 ty,sInt border,sInt newWidth,sInt newHeight)
{
  NOTEXTURES1(bm);

  GenBitmap *in;
  sInt x,y;
  sU16 *d;
  sU64 *mem;
	sInt xs,ys;
  sInt txs,tys;
	sInt m00,m01,m10,m11,m20,m21;
  sF32 rotangle;
  BilinearContext ctx;

  if(sFAbs(sx) < 1.0f/32768.0f) sx = 1.0f/32768.0f;
  if(sFAbs(sy) < 1.0f/32768.0f) sy = 1.0f/32768.0f;

  if(CheckBitmap(bm,&in)) return 0;

// prepare
	xs = bm->XSize;
	ys = bm->YSize;
  txs = newWidth ? 1 << (newWidth - 1 + GenBitmapTextureSizeOffset) : xs;
  tys = newHeight ? 1 << (newHeight - 1 + GenBitmapTextureSizeOffset) : ys;
  mem = new sU64[txs * tys];
  d = (sU16 *)mem;

// rotate

  rotangle = angle*sPI2F;

  m00 = sFtol( sFCos(rotangle)*0x10000*xs/(sx*txs));
  m01 = sFtol( sFSin(rotangle)*0x10000*ys/(sx*txs));
  m10 = sFtol(-sFSin(rotangle)*0x10000*xs/(sy*tys));
  m11 = sFtol( sFCos(rotangle)*0x10000*ys/(sy*tys));
  m20 = sFtol( tx*xs*0x10000 - (txs*m00+tys*m10)/2);
  m21 = sFtol( ty*ys*0x10000 - (txs*m01+tys*m11)/2);
  BilinearSetup(&ctx,in->Data,xs,ys,border);

  for(y=0;y<tys;y++)
  {
    sInt u = y*m10+m20;
    sInt v = y*m11+m21;

    for(x=0;x<txs;x++)
    {
      BilinearFilter(&ctx,(sU64 *)d,u,v);
      u += m00;
      v += m01;
      d += 4;
    }
  }

  delete[] bm->Data;
  bm->XSize = txs;
  bm->YSize = tys;
  bm->Size = txs*tys;
  bm->Data = mem;

  return bm;
}

static sInt CSTable[2][1025];

static sInt CSLookup(const sInt *table,sInt value)
{
  sInt ind = (value >> 6);
  return table[ind] + (((table[ind+1] - table[ind]) * (value & 63)) >> 6);
}

GenBitmap * __stdcall Bitmap_Twirl(GenBitmap *bm,sF32 strength,sF32 gamma,sF32 rx,sF32 ry,sF32 cx,sF32 cy,sInt border)
{
  NOTEXTURES1(bm);

  GenBitmap *in;
  sInt x,y;
  sU16 *s,*d;
  sU64 *mem;
	sInt xs,ys;
  sInt px,py,dx,dy,u,v;
  BilinearContext ctx;

  if(rx < 1.0f/32768.0f) rx = 1.0f/32768.0f;
  if(ry < 1.0f/32768.0f) ry = 1.0f/32768.0f;

  if(CheckBitmap(bm,&in)) return 0;

// prepare

  mem = new sU64[in->Size];
  d = (sU16 *)mem;
  s = (sU16 *)in->Data;
	xs = in->XSize;
	ys = in->YSize;
  rx*=0.5f;
  ry*=0.5f;
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
      
			//BilinearFilter((sU64*)d,(sU64*)s,xs,ys,sFtol(u*0x10000*xs),sFtol(v*0x10000*ys),border);
      //BilinearFilter(&ctx,(sU64 *)d,sFtol(u*0x10000*xs),sFtol(v*0x10000*ys));
      BilinearFilter(&ctx,(sU64 *)d,u*xs,v*ys);
      d+=4;

      px += xstep;
    }

    py += ystep;
  }

  delete[] bm->Data;
  bm->Data = mem;

  return bm;
}

GenBitmap * __stdcall Bitmap_RotateMul(GenBitmap *bm,sF32 angle,sF32 sx,sF32 sy,sF32 tx,sF32 ty,sInt border,sU32 color,sInt mode,sInt count,sU32 fade)
{
  NOTEXTURES1(bm);

  GenBitmap *bb,*bx;
  sInt cr,cg,cb,ca,i;
  SCRIPTVERIFY(count>0);
  SCRIPTVERIFY(bm);
  SCRIPTVERIFY(bm->ClassId==KC_BITMAP);
  bm = Bitmap_Color(bm,0,color);
  bb = (GenBitmap *)bm->Copy();

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
        bm = Bitmap_Color(bm,0,fade);
        ca = (ca*ca)>>8;
        cr = (cr*cr)>>8;
        cg = (cg*cg)>>8;
        cb = (cb*cb)>>8;
        fade = (ca<<24)|(cr<<16)|(cg<<8)|(cb);
      }
      bm = Bitmap_Rotate(bm,angle,sx,sy,tx,ty,border,0,0);
      if(!bm) { bb->Release(); return 0; }
      bm = Bitmap_Merge(mode&15,2,bm,bb);
      if(!bm) { bb->Release(); return 0; }
      bb = (GenBitmap *) bm->Copy();
      angle = angle*2;
      tx = (tx-0.5f)*2+0.5f;
      ty = (ty-0.5f)*2+0.5f;
      sx = sx*sx;
      sy = sy*sy;
//      bm->AddRef();
//      bb = Bitmap_Merge(,mode&15,2,bb,bm);
    }
    else
    {
      if(fade!=0xffffffff)
        bm = Bitmap_Color(bm,0,fade);
      if(!bm) { bb->Release(); return 0; }
      bm->AddRef();
      bx = Bitmap_Rotate(bm,angle*i,(sx-1)*i+1,(sy-1)*i+1,(tx-0.5)*i+0.5,(ty-0.5)*i+0.5,border,0,0);
      if(!bx) { bm->Release(); bb->Release(); return 0; }
      bb = Bitmap_Merge(mode&15,2,bb,bx);
      if(!bb) return 0;
      i++;
    }
  }

  bm->Release();
  return bb;
}

/****************************************************************************/

GenBitmap * __stdcall Bitmap_Distort(GenBitmap *bb,GenBitmap *bm,sF32 dist,sInt border)
{
  NOTEXTURES2(bb,bm);

  GenBitmap *in;
  sInt x,y;
  sU16 *t,*d,*a;
	sInt xs,ys;
  sInt u,v;
  sInt bumpx,bumpy;
  BilinearContext ctx;

  SCRIPTVERIFY(bb);
  SCRIPTVERIFY(bm);
  SCRIPTVERIFY(bb->ClassId==KC_BITMAP);
  SCRIPTVERIFY(bm->ClassId==KC_BITMAP);
  SCRIPTVERIFY(bm->Size==bb->Size);
  if(CheckBitmap(bm,&in)) return 0;

  bb->Release();

// prepare

  t = (sU16 *)bm->Data;
  d = (sU16 *)bb->Data;
  a = (sU16 *)in->Data;
	xs = in->XSize;
	ys = in->YSize;
  bumpx = (dist*xs)*4;
  bumpy = (dist*ys)*4;
  BilinearSetup(&ctx,(sU64 *)d,xs,ys,border);

// rotate 

  for(y=0;y<ys;y++)
  {
    for(x=0;x<xs;x++)
    {
      u = ((x)<<16) + ((a[0]-0x4000)*bumpx);
      v = ((y)<<16) + ((a[1]-0x4000)*bumpy);
      BilinearFilter(&ctx,(sU64 *)t,u,v);
			//BilinearFilter((sU64*)t,(sU64*)d,xs,ys,u,v,border);
      t+=4;
      a+=4;
    }
  }

  return bm;
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

GenBitmap * __stdcall Bitmap_Normals(GenBitmap *bm,sF32 _dist,sInt mode)
{
  NOTEXTURES1(bm);
  GenBitmap *in;
  sInt shiftx,shifty;
  sInt xs,ys;
  sInt x,y;
  //sU64 *mem;
  sU16 *sx,*sy,*s,*d;
  sInt vx,vy,vz;
  sF32 e;
  sInt dist;

  if(CheckBitmap(bm,&in)) return 0;

  dist = sFtol(_dist*65536.0f);
  d = (sU16 *) bm->Data;
  sx = sy = s = (sU16*) in->Data;
	xs = in->XSize;
	ys = in->YSize; 
  shiftx = sGetPower2(in->XSize);
  shifty = sGetPower2(in->YSize);
  
//  Bitmap_Inner(bm->Data,0,bm->Size,8);      // gray bitmap

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

      d[0] = vx+0x4000;
      d[1] = vy+0x4000;
      d[2] = vz+0x4000;
      d[3] = 0xffff;

      d += 4;
      sy += 4;
    }
    sx += xs*4;
  }

  /*delete[] bm->Data;
  bm->Data = mem;*/
  return bm;
}

/****************************************************************************/

void BumpLight(GenBitmap *bm,GenBitmap *bb,sS32 *para);

GenBitmap * __stdcall Bitmap_Light(GenBitmap *bm,sInt subcode,sF32 px,sF32 py,sF32 pz,sF32 da,sF32 db,
                                   sU32 diff,sU32 ambi,sF32 outer,sF32 falloff,sF32 amp)
{
  return Bitmap_Bump(bm,0,subcode,px,py,pz,da,db,diff,ambi,outer,falloff,amp,ambi,0,0);
}


GenBitmap * __stdcall Bitmap_Bump(GenBitmap *bm,GenBitmap *bb,sInt subcode,sF32 px,sF32 py,sF32 pz,sF32 da,sF32 db,
                                  sU32 _diff,sU32 _ambi,sF32 outer,sF32 falloff,sF32 amp,
                                  sU32 _spec,sF32 spow,sF32 samp)
{
  NOTEXTURES2(bm,bb);

  GenBitmap *in;

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

  SCRIPTVERIFY(outer!=0);
#if !sINTRO
  if(bm && bb)
  {
    SCRIPTVERIFY(bm->ClassId==KC_BITMAP);
    SCRIPTVERIFY(bb->ClassId==KC_BITMAP);
    SCRIPTVERIFY(bm->XSize==bb->XSize && bm->YSize==bb->YSize);
  }
#endif
  if(CheckBitmap(bm,&in)) return 0;

	xs = in->XSize;
	ys = in->YSize;

  *(sU64 *)diff = GetColor64(_diff);
  *(sU64 *)ambi = GetColor64(_ambi);
  *(sU64 *)spec = GetColor64(_spec);

  px = px*xs*2-(xs/2);
  py = py*ys*2-(ys/2);
  pz = pz*xs;
  da *= sPI2F;
  db *= sPIF;

  dx = dy = sFCos(db);
  dx *= sFSin(da);
  dy *= sFCos(da);
  dz = sFSin(db);


  s = (sU16 *)in->Data;
  d = (sU16 *)bm->Data;
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

  for(y=0;y<bm->YSize;y++)
  {
    for(x=0;x<bm->XSize;x++)
    {

      if(subcode!=2)
      {
        lx = x-px;
        ly = y-py;
        lz = pz;
        e = 1/sFSqrt(lx*lx+ly*ly+lz*lz);
        lx*=e;
        ly*=e;
        lz*=e;
      }

      if(b)
      {
        nx = (b[0]-0x4000)/16384.0f;
        ny = (b[1]-0x4000)/16384.0f;
        nz = (b[2]-0x4000)/16384.0f;
        b+=4;
      }

      lf = lx*nx+ly*ny+lz*nz;

      if(samp)
      {
        hx = lx;
        hy = ly;
        hz = lz+1;
        e = 1/sFSqrt(hx*hx+hy*hy+hz*hz);
        sf = hx*nx+hy*ny+hz*nz;
        if(sf<0) sf=0;
        sf = sFPow(sf*e,spow);
      }

      if(subcode==0)
      {
        df = (lx*dx+ly*dy+lz*dz);
        if(df<outer)
          df = 0;
        else
          df = sFPow((df-outer)/(1-outer),falloff);
      }

      f0 = df*lf*amp;
      for(i=0;i<4;i++)
        buff[i] = sRange7fff(sFtol(s[i]*(ambi[i]+diff[i]*f0)/0x8000));
      AddScale64(*(sU64 *)d,*(sU64 *)buff,*(sU64 *)spec,sFtol(df*sf*samp));
      s+=4;
      d+=4;
    }
  }

  if(bb) bb->Release();

  return bm;
}

/****************************************************************************/

struct letterz
{
  sU16 xp,yp;
  sS16 w0,w1,w2;
};

GenBitmap * __stdcall Bitmap_Text(KOp *op,KEnvironment *kenv,GenBitmap *bm,sF32 x,sF32 y,sF32 width,sF32 height,sU32 col,sU32 flags,sInt extspace,sF32 inspace,sF32 lineskip,sChar *text,sChar *font)
{
  NOTEXTURES1(bm);

  sU64 col64;
  col64 = GetColor64(col);
  sU32 *bitmem;
  sInt size;
  sU8 *data;
  static letterz let[256];

  GenBitmap *in;
  sInt xi,yi,i;
  sU32 fade;
  sInt xs,ys,xp,yp,yf,is,es,xp0,lsk;
  sU32 *s;
  sInt page;
#if !sINTRO
  sInt w0,w1,w2;
  sInt count,chr;
  sInt widths[3];
#endif
  //const sInt alias = 2;
  const sInt alias = 4;

  if(CheckBitmap(bm,&in)) return 0;

  sSetMem(let,0,sizeof(let));

// write let for normal character generation
// read let after all is done
// eventually write blob 
// maybe it's better to write blob everytime, just not always store it. (shorter cleaner code)

  xs = bm->XSize*alias;
  ys = bm->YSize*alias;
  es = extspace*alias;

  es += 2*alias;

  is = inspace*xs;
  xp = x*xs+es;
  yp = y*ys+es;

  data = (sU8*)op->GetBlob(size);
  page = (flags&0x70)>>4;
  if((flags&0x80) && data && data[0]==3 && data[1]==(page!=0) && 
    *((sU16 *)(data+2))==bm->XSize && *((sU16 *)(data+4))==bm->YSize)
  {
    yf = *((sU16 *)(data+6));
    data+=8;

    if(page)
    {
      for(i=0;i<256;i++)
      {
        let[i].xp = *((sU16 *)data); data+=2;
        let[i].yp = *((sU16 *)data); data+=2;
        let[i].w0 = *((sS16 *)data); data+=2;
        let[i].w1 = *((sS16 *)data); data+=2;
        let[i].w2 = *((sS16 *)data); data+=2;
      }
    }

    for(i=0;i<bm->Size;i++)
    {
      fade = *data++;

      fade = (fade<<5) | (fade<<2) | (fade>>1);
      fade = (fade)|(fade<<8);
      Fade64(bm->Data[i],in->Data[i],col64,fade);
    }
  }
  else
  {
#if !sINTRO
    op->SetBlob(0,0);
#endif

    yf = sSystem->FontBegin(xs,ys,font,xs*height,ys*width,4);
    lsk = lineskip * yf;
#if !sINTRO
    if(page)                            // font mode
    {
      count = 0;
      chr = 'a';
      sSetMem(kenv->Letters[page],0,sizeof(KLetterMetric)*256);

      while(*text || chr<count)
      {
        if(text[0]=='-' && text[1]!='-' && text[1]!=0)
        {
          text++;
          count = (*text++)&255;
        }
        if(text[0]=='-' && text[1]=='-')
        {
          text++;
        }
        if(chr<count)
        {
          chr++;
        }
        else
        {
          count = 0;
          chr = (*text++)&255;
        }
        sSystem->FontCharWidth(chr,widths);
        w1 = widths[1];
        //xf = sAbs(widths[0]) + sAbs(widths[1]) + sAbs(widths[2]);

        if(xp+w1+es+2*is>=xs)
        {
          yp = sAlign(yp+yf+2*is,alias);
          yp += es;
          xp = x*xs+es;
          if(yp+yf+es+2*is>=ys)
            break;
        }

        let[chr].xp = xp;
        let[chr].yp = yp;
        let[chr].w0 = widths[0];
        let[chr].w1 = widths[1];
        let[chr].w2 = widths[2];

        sChar c=chr;
        sSystem->FontPrint(xp+is-widths[0],yp+is,&c,1);
        xp = sAlign(xp+w1+2*is,alias);
        xp += es;
      }
    }
    else                                      // normal mode
#endif
    {
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
            xp0 = xp-sSystem->FontWidth((sChar *)text,i)/2;
          else
            xp0 = xp;
          sSystem->FontPrint(xp0,yp,(sChar *)text,i);
        }

        text+=i;
        if(*text=='\n')
        {
          yp += lsk;
          text++;
        }
      }
    }

    // write to bitmap

    bitmem = sSystem->FontBitmap();
    i = 0;
    for(yi=0;yi<bm->YSize;yi++)
    {
      s = bitmem+yi*xs*alias;
      for(xi=0;xi<bm->XSize;xi++)
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
        Fade64(bm->Data[i],in->Data[i],col64,fade);
        i++;
        s+=alias;
      }
    }

    // write to blob

#if !sPLAYER
    if(flags&0x80)
    {
      sU8 *mem;
      size = 8 + bm->XSize*bm->YSize;
      if(page)
        size += 256*10;
      mem = data = new sU8[size];

      data[0] = 3;
      data[1] = page != 0;
      *((sU16 *)(data+2)) = bm->XSize;
      *((sU16 *)(data+4)) = bm->YSize;
      *((sU16 *)(data+6)) = yf;
      data+=8;

      if(page)
      {
        for(i=0;i<256;i++)
        {
          *((sU16 *)data) = let[i].xp; data+=2;
          *((sU16 *)data) = let[i].yp; data+=2;
          *((sS16 *)data) = let[i].w0; data+=2;
          *((sS16 *)data) = let[i].w1; data+=2;
          *((sS16 *)data) = let[i].w2; data+=2;
        }
      }

      for(yi=0;yi<bm->YSize;yi++)
      {
        s = bitmem+yi*xs*alias;
        for(xi=0;xi<bm->XSize;xi++)
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


          *data++ = fade>>5;
          s+=alias;
        }
      }

      op->SetBlob(mem,size);
    }
#endif

    sSystem->FontEnd();
  }

#if !sINTRO
  if(page)
  {
    for(i=0;i<256;i++)
    {
      xp = let[i].xp;
      yp = let[i].yp;
      w0 = let[i].w0;
      w1 = let[i].w1;
      w2 = let[i].w2;

//      if(w0|w1|w2) sDPrintF("'%c' %08x %08x %08x %08x %08x\n",i,xp,yp,w0,w1,w2);

      kenv->Letters[page][i].UV.x0 = xp*1.0f/(xs);
      kenv->Letters[page][i].UV.y0 = yp*1.0f/(ys);
      kenv->Letters[page][i].UV.x1 = (xp+w1+is*2)*1.0f/(xs);
      kenv->Letters[page][i].UV.y1 = (yp+yf+is*2)*1.0f/(ys);
      kenv->Letters[page][i].PreSpace = (w0-is)*1.0f/xs;
      kenv->Letters[page][i].Width = (w0+w1+w2+is*2)*1.0f/xs;
    }
    sDPrintF("-----------------------------------------\n");
  }
#endif
  return bm;
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
  v00 = sPerlinPermute[((vx+0)     )+sPerlinPermute[((vy+0)     )^seed]];
  v01 = sPerlinPermute[((vx+1)&mask)+sPerlinPermute[((vy+0)     )^seed]];
  v10 = sPerlinPermute[((vx+0)     )+sPerlinPermute[((vy+1)&mask)^seed]];
  v11 = sPerlinPermute[((vx+1)&mask)+sPerlinPermute[((vy+1)&mask)^seed]];
  f00 = sPerlinRandom[v00][0]*(tx-0)+sPerlinRandom[v00][1]*(ty-0);
  f01 = sPerlinRandom[v01][0]*(tx-1)+sPerlinRandom[v01][1]*(ty-0);
  f10 = sPerlinRandom[v10][0]*(tx-0)+sPerlinRandom[v10][1]*(ty-1);
  f11 = sPerlinRandom[v11][0]*(tx-1)+sPerlinRandom[v11][1]*(ty-1);
  tx = tx*tx*tx*(10+tx*(6*tx-15));
  ty = ty*ty*ty*(10+ty*(6*ty-15));
  //tx = tx*tx*(3-2*tx);
  //ty = ty*ty*(3-2*ty);
  fa = f00+(f01-f00)*tx;
  fb = f10+(f11-f10)*tx;
  f = fa+(fb-fa)*ty;

  return f;
}

GenBitmap * __stdcall Bitmap_Perlin(sInt xs,sInt ys,sInt freq,sInt oct,sF32 fadeoff,sInt seed,sInt mode,sF32 amp,sF32 gamma,sU32 col0,sU32 col1)
{
  NOTEXTURES0();

  GenBitmap *bm;
  sInt i;
  sInt x,y,noffs;
  sInt shiftx,shifty;
  sU64 c0,c1;
  sU64 *tile;

  bm = NewBitmap(xs,ys);

  c0 = GetColor64(col0);
  c1 = GetColor64(col1);

  shiftx = 16-sGetPower2(bm->XSize);
  shifty = 16-sGetPower2(bm->YSize);
  tile = bm->Data;
  seed &= 255;
  mode &= 3;

  for(i=0;i<1025;i++)
    GammaTable[i] = sRange7fff(sFPow(i/1024.0f,gamma)*0x8000)*2;

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

  sInt ampi = sFtol(amp);

  sInt sinTab[257];
  if(mode & 2)
  {
    for(x=0;x<257;x++)
      sinTab[x] = sFtol(sFSin(sPI2F * x / 256.0f) * 0.5f * 65536.0f);
  }

  sInt *nrow = new sInt[bm->XSize];
  sInt *poly = new sInt[bm->XSize>>freq];

  for(x=0;x<(bm->XSize>>freq);x++)
  {
    sF32 f = 1.0f * x / (bm->XSize>>freq);
    poly[x] = sFtol(f*f*f*(10+f*(6*f-15))*16384.0f);
  }

  for(y=0;y<bm->YSize;y++)
  {
    sSetMem(nrow,0,sizeof(sInt)*bm->XSize);
    sF32 s = 1.0f;

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
      sInt vy0 = sPerlinPermute[((vy+0)     )^seed];
      sInt vy1 = sPerlinPermute[((vy+1)&mask)^seed];
      sInt shf = i-freq;
      //sInt si = sFtol(s * ampi);
      sInt si = sFtol(s * 16384.0f);

      if(shiftx+i < 16 || (py & 0xffff)) // otherwise, the contribution is always zero
      {
        sInt *rowp = nrow;
        for(sInt vx=0;vx<groups;vx++)
        {
          sInt v00 = sPerlinPermute[((vx+0)&mask)+vy0];
          sInt v01 = sPerlinPermute[((vx+1)&mask)+vy0];
          sInt v10 = sPerlinPermute[((vx+0)&mask)+vy1];
          sInt v11 = sPerlinPermute[((vx+1)&mask)+vy1];

          sF32 f_0h = sPerlinRandom[v00][0] + (sPerlinRandom[v10][0] - sPerlinRandom[v00][0]) * tyf;
          sF32 f_1h = sPerlinRandom[v01][0] + (sPerlinRandom[v11][0] - sPerlinRandom[v01][0]) * tyf;
          sF32 f_0v = sPerlinRandom[v00][1]*ty0f + sPerlinRandom[v10][1]*ty1f;
          sF32 f_1v = sPerlinRandom[v01][1]*ty0f + sPerlinRandom[v11][1]*ty1f;

          sInt fa = sFtol(f_0v * 65536.0f);
          sInt fb = sFtol((f_1v - f_1h) * 65536.0f);
          sInt fad = sFtol(f_0h * dtx);
          sInt fbd = sFtol(f_1h * dtx);

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
            *rowp++ += (nni*si)>>14;
            fa += fad;
            fb += fbd;
          }
        }
      }

      s *= fadeoff;
    }

    // resolve
    for(x=0;x<bm->XSize;x++)
      Fade64(*tile++,c0,c1,GetGamma(sRange7fff(sMulShift(nrow[x],ampi)+noffs)));
  }

  delete[] nrow;
  delete[] poly;

  return bm;
}

/****************************************************************************/

GenBitmap * __stdcall Bitmap_Cell(sInt xs,sInt ys,sU32 col0,sU32 col1,sU32 col2,sInt max,sInt seed,sF32 amp,sF32 gamma,sInt mode,sF32 mindistf,sInt percent,sF32 aspect)
{
  NOTEXTURES0();

  GenBitmap *bm;
  static sInt cells[256][3];
  static sInt cellt[256];
  sInt x,y,i,j,dist,best,best2,besti,best2i;
  sInt dx,dy,px,py;
  sInt shiftx,shifty;
  sF32 v0,v1;
  sInt val,mdist;
  sU64 c0,c1,cc,cb;
  sU64 *tile;
  sBool cut;

  SCRIPTVERIFY(max>=1 && max<=256);

  bm = NewBitmap(xs,ys);

  sSetRndSeed(seed);
  for(i=0;i<max*3;i++)
    cells[0][i] = sGetRnd(0x4000);

  mdist = sFtol(mindistf*0x4000);
  mdist = mdist*mdist;
  for(i=1;i<max;)
  {
    if((mode&2) && (sInt)sGetRnd(255)<percent)
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


  shiftx = 14-sGetPower2(bm->XSize);
  shifty = 14-sGetPower2(bm->YSize);
  c0 = GetColor64(col1);
  c1 = GetColor64(col0);
  cb = GetColor64(col2);
  tile = bm->Data;

  aspect = sFPow(2,aspect);
  sInt aspx,aspy;
  sF32 aspdiv;

  if(aspect >= 1.0f)
  {
    aspx = 65536 / (aspect * aspect);
    aspy = 65536;
    aspdiv = aspect / 16384.0f;
  }
  else
  {
    aspx = 65536;
    aspy = aspect * aspect * 65536;
    aspdiv = 1.0f / (16384.0f * aspect);
  }

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
  for(sInt by=0;by<bm->YSize;by+=tileSize)
  {
    for(sInt bx=0;bx<bm->XSize;bx+=tileSize)
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
          cellt[i] = 0;
        else
        {
          dx = sMin(sAbs(dx),sAbs(dy));
          cellt[i] = sMulShift(dx*dx,aspf);
        }

        dx = ((cells[i][1]-py0)&0x3fff)-0x2000;
        dy = ((cells[i][1]-py1)&0x3fff)-0x2000;
        if((dx ^ dy) > 0)
        {
          dy = sMin(sAbs(dx),sAbs(dy));
          cellt[i] += dy*dy;
        }
      }

      // (insertion) sort by it
      for(i=1;i<max;i++)
      {
        sInt x = cells[i][0], y = cells[i][1], c = cells[i][2];
        sInt dy = cellt[i], j = i;

        while(j && cellt[j-1] > dy)
        {
          cells[j][0] = cells[j-1][0];
          cells[j][1] = cells[j-1][1];
          cells[j][2] = cells[j-1][2];
          cellt[j] = cellt[j-1];
          j--;
        }

        cells[j][0] = x;
        cells[j][1] = y;
        cells[j][2] = c;
        cellt[j] = dy;
      }

      // render tile
      tile = bm->Data + by*bm->XSize + bx;

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
          for(sInt i=0;i<max && best2 > cellt[i];i++)
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
          val = sRange7fff(sFPow(v0*amp,gamma)*0x8000)*2;
          if(mode&4)
            val = 0x10000-val;

          if(mode&2)
          {
            if(cells[besti][2]==0xffff)
              cc=cb;
            else
              Fade64(cc,c0,c1,cells[besti][2]*4);
            Fade64(*tile,cc,cb,val);
          }
          else
          {
            Fade64(*tile,c0,c1,val);
          }
          tile++;
        }

        tile += bm->XSize-tileSize;
      }
    }
  }

  return bm;
}

/****************************************************************************/

GenBitmap * __stdcall Bitmap_Gradient(sInt xs,sInt ys,sU32 col0,sU32 col1,sF32 posf,sF32 a,sF32 length,sInt mode)
{
  NOTEXTURES0();

	GenBitmap *bm;
  sU64 c0,c1;
	sInt c,cdx,cdy,x,y;
	sInt dx,dy,pos;
	sF32 l;
  sInt val;
  sU64 *tile;

  bm = NewBitmap(xs,ys);

	c0 = GetColor64(col0);
	c1 = GetColor64(col1);

	l = 32768.0f / length;
  pos = posf*32768.0f;
	dx = sFtol(sFCos(a * sPI2) * l);
	dy = sFtol(sFSin(a * sPI2) * l);
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
        val = sFtol(sFSin(sRange7fff(c)*sPI2/0x8000+0x2000)*0x7fff+0x7fff);
        break;
      case 2:
        val = sFtol(sFSin(sRange7fff(c)*sPI2/0x10000)*0xffff);
        break;
      }
			Fade64(*tile,c0,c1,val);
			c += cdx;
			tile++;
		}

		c += cdy;
	}

	return bm;
}

/****************************************************************************/

GenBitmap * __stdcall Bitmap_Sharpen(GenBitmap *bm,sInt order,sF32 sx,sF32 sy,sF32 amp)
{
  NOTEXTURES1(bm);

  GenBitmap *bx;
  sU64 color;

  SCRIPTVERIFY(bm);
  bx = bm;
  bx->AddRef();
  if(CheckBitmap(bm)) return 0;

  color = sRange<sInt>(amp*0x800,0x7fff,-0x7fff)&0xffff;
  color |= color<<16;
  color |= color<<32;

  bm = Bitmap_Blur(bm,order|0x10,sx,sy,1.0f);
  Bitmap_Inner(bm->Data,&color,bm->Size,BI_SHARPEN,bx->Data);

  bx->Release();
  return bm;
}

/****************************************************************************/
/****************************************************************************/

GenBitmap * __stdcall Bitmap_Import(KOp *op,sChar *filename)
{
  NOTEXTURES0();

  GenBitmap *bm;
  const sU8 *data;
  sU32 *s;
  sU64 *d;
  sInt size;
  sInt i;
  sInt xs,ys;
  sU8 *pic;

  bm = 0;

  data = op->GetBlob(size);
#if !sPLAYER
  if(data==0)
  {
    data = sSystem->LoadFile(filename,size);
    op->SetBlob(data,size);
  }
#endif
  if(data)
  {
    if(sSystem->LoadBitmapCore(data,size,xs,ys,pic))
    {
      bm = new GenBitmap;
      bm->Init(xs,ys);
      s = (sU32 *)pic;
      d = (sU64 *)bm->Data;
      
      for(i=0;i<xs*ys;i++)
        *d++ = GetColor64(*s++);     

      delete[] pic;
    }
  }

  return bm;
}

/****************************************************************************/
/****************************************************************************/

static sInt CBTable[3][257];

static sInt CBLookup(const sInt *table,sInt value)
{
  sInt ind = (value >> 7);
  return table[ind] + (((table[ind+1] - table[ind]) * (value & 127)) >> 7);
}

GenBitmap * __stdcall Bitmap_ColorBalance(GenBitmap *bm,sF323 shadows,sF323 midtones,sF323 highlights)
{
  sInt i,j;
  sF32 x;
  sF32 vsha,vmid,vhil;
  sF32 p;
  sF32 min,max,msc;
  GenBitmap *in;
  static const sF32 sc = 100.0f / 255.0f;

  NOTEXTURES1(bm);

  if(CheckBitmap(bm,&in)) return 0;

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
      x = sFPow((sRange(i/256.0f,max,min) - min) * msc,p);
      CBTable[j][i] = sRange7fff(x * 32768.0f);
    }
  }

  // now just apply lookup tables
  sU16 *d = (sU16 *) bm->Data;
  sU16 *s = (sU16 *) in->Data;

  for(i=0;i<bm->Size;i++)
  {
    d[0] = CBLookup(CBTable[2],s[0]);
    d[1] = CBLookup(CBTable[1],s[1]);
    d[2] = CBLookup(CBTable[0],s[2]);
    d[3] = s[3];

    d += 4;
    s += 4;
  }

  return bm;
}

/****************************************************************************/

GenBitmap * __stdcall Bitmap_Unwrap(GenBitmap *bm,sInt mode)
{
  GenBitmap *in;
  BilinearContext ctx;
  sU64 *mem;

  NOTEXTURES1(bm);
  if(CheckBitmap(bm,&in)) return 0;

  BilinearSetup(&ctx,in->Data,in->XSize,in->YSize,(mode >> 4) & 3);
  sInt wrapMode = mode & 15;

  mem = new sU64[in->Size];
  sU16 *d = (sU16 *) mem;
  sF32 invX = 1.0f / bm->XSize;
  sF32 invY = 1.0f / bm->YSize;
  sInt usc = bm->XSize << 16;
  sInt vsc = bm->YSize << 16;

  for(sInt y=0;y<bm->YSize;y++)
  {
    sF32 fy = y * invY,fyc = 0.5f - fy;

    for(sInt x=0;x<bm->XSize;x++)
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

  delete[] bm->Data;
  bm->Data = mem;

  return bm;
}

GenBitmap * __stdcall Bitmap_Bulge(GenBitmap *bm,sF32 warp)
{
  GenBitmap *in;
  BilinearContext ctx;
  sU64 *mem;

  NOTEXTURES1(bm);
  if(CheckBitmap(bm,&in)) return 0;

  BilinearSetup(&ctx,in->Data,in->XSize,in->YSize,0);
  mem = new sU64[bm->Size];
  sU16 *d = (sU16 *) mem;
  sF32 invX = 1.0f / bm->XSize;
  sF32 invY = 1.0f / bm->YSize;
  sInt usc = bm->XSize << 16;
  sInt vsc = bm->YSize << 16;

  for(sInt y=0;y<bm->YSize;y++)
  {
    sF32 fy = y * invY,fyc = (0.5f - fy) * 2.0f;

    for(sInt x=0;x<bm->XSize;x++)
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

  delete[] bm->Data;
  bm->Data = mem;

  return bm;
}

/****************************************************************************/
/****************************************************************************/

GenBitmap * __stdcall Bitmap_Bricks(sInt bmxs,sInt bmys,sInt color0,sInt color1,sInt colorf,sF32 ffugex,sF32 ffugey,sInt tx,sInt ty,sInt seed,sInt heads,sInt flags,sF32 side,sF32 colorbalance)
{
  GenBitmap *bm;
  sU64 *d;
  sInt fx,fy;
  sInt fugex,fugey;
  sInt bx,by;
  sU64 c0,c1,cf,cb;
  sInt f;
  sInt kopf,kopftrigger;
  sInt sidestep,sideakku;
  sInt multiply;
  struct Cell
  {
    sU64 Color;
    sInt Kopf;
  } *cells;

  bm = NewBitmap(bmxs,bmys);

  sSetRndSeed(seed);

  d = bm->Data;
  c0 = GetColor64(color0);
  c1 = GetColor64(color1);
  cf = GetColor64(colorf);
  fugex = ffugex*0x2000; 
  fugey = ffugey*0x2000; 
  sidestep = side*0x4000;
  multiply = 1<<((flags>>4)&7);

  cells = new Cell[tx*multiply*ty*multiply];
  kopf = 0;
//  Fade64(cb,c0,c1,sFPow(sFGetRnd(),colorbalance)*0x10000);

// distribute heads

  for(sInt y=0;y<ty;y++)
  {
    kopftrigger = 0;
    for(sInt x=0;x<tx;x++)
    {
      kopftrigger--;
      cells[y*tx*multiply+x].Color = 0;
      cells[y*tx*multiply+x].Kopf = kopf;
      if(kopf==0)
      {
        kopf = 1;
      }
      else
      {
        if((sInt)sGetRnd(255)>heads || kopftrigger>=0)
        {
          kopf = 0;
        }
        else
          if(flags&4) kopftrigger=2;
      }
//      if(kopf)
//        Fade64(cb,c0,c1,sFPow(sFGetRnd(),colorbalance)*0x10000);
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

  Fade64(cb,c0,c1,sFPow(sFGetRnd(),colorbalance)*0x10000);
  for(sInt y=0;y<ty;y++)
  {
    for(sInt x=0;x<tx;x++)
    {
      if(cells[y*tx+x].Kopf)
        Fade64(cb,c0,c1,sFPow(sFGetRnd(),colorbalance)*0x10000);
      cells[y*tx+x].Color = cb;
    }
    if(!cells[y*tx].Kopf)
      cells[y*tx].Color = cells[y*tx+tx-1].Color;
  }

// do the painting

  sInt bmx = 0x4000/bm->XSize*tx;
  sInt bmy = 0x4000/bm->YSize*ty;
  sideakku = 0;
  for(sInt y=0;y<bm->YSize;y++)
  {
    fy = 0x4000/bm->YSize*y*ty;
    by = fy>>14;
    sVERIFY(by<ty);
    fy = fy&0x3fff;
    sideakku = (by*sidestep)&0x7fff;
    for(sInt x=0;x<bm->XSize;x++)
    {
      fx = 0x4000/bm->XSize*x*tx;
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
      Fade64(*d,cf,cells[by*tx+bx%tx].Color,f*4);
      d++;
    }
  }

  delete[] cells;

  return bm;
}

/****************************************************************************/

#if sLINK_ENGINE

/****************************************************************************/

void _stdcall Exec_Bitmap_Render(sInt xs,sInt ys)
{
}

void _stdcall Exec_Bitmap_RenderAuto(sInt xs,sInt ys,sInt flags)
{
}

/****************************************************************************/

GenBitmap * __stdcall Bitmap_Render(KOp *op,class GenIPP *ipp,sInt xs,sInt ys)
{
  NOTEXTURES0();

  sViewport view;
  sTexInfo tif;
  GenBitmap *bm;

  bm = NewBitmap(xs,ys);

  tif.InitRT(bm->XSize,bm->YSize);
  sInt tex = sSystem->AddTexture(tif);

  view.InitTexMS(tex);
  sSystem->SetViewport(view);
  sSystem->Clear(sVCF_ALL,0xffff8080);

  // taken vom   Exec_IPP_RenderTarget
  {
    // just tweak the master viewport
    sViewport saved,newvp;

    GenOverlayManager->GetMasterViewport(saved);
    newvp.InitTexMS(tex);
    GenOverlayManager->SetMasterViewport(newvp);

    // render
    KEnvironment *kenv=new KEnvironment;
    kenv->InitView();
    kenv->InitFrame(0,0);
    op->ExecInputs(kenv);
    kenv->ExitFrame();
    delete kenv;

    // restore
    GenOverlayManager->SetMasterViewport(saved);
  }

  sSystem->ReadTexture(tex,(sU16 *)bm->Data);
  sSystem->RemTexture(tex);

  sRelease(ipp);
  return bm;
}

/****************************************************************************/

GenBitmap * __stdcall Bitmap_RenderAuto(KOp *op,class GenMinMesh *mesh,sInt xs,sInt ys,sInt flags)
{
  NOTEXTURES0();
  sViewport view;
  sTexInfo tif;
  GenBitmap *bm;
  sMatrix mat;
  EngLight light;
  sMaterialEnv env;
  sAABox box;
  sInt fieldcount;
  sInt fieldmax;
  sViewport saved,newvp;
  sVector boxmin,boxmax;



  bm = NewBitmap(xs,ys);
  sSetMem8(bm->Data,0xffff0000ffff0000ULL,bm->Size);

  tif.InitRT(bm->XSize,bm->YSize);
  sInt tex = sSystem->AddTexture(tif);

  mesh->CalcBBox(box);

  fieldcount = 0;
  fieldmax = 0;
  for(sInt i=0;i<6;i++)
    if(flags & (1<<i))
      fieldmax++;

  GenOverlayManager->GetMasterViewport(saved);
  newvp.InitTexMS(tex);
  GenOverlayManager->SetMasterViewport(newvp);
  RenderTargetManager->SetMasterViewport(newvp);

  for(sInt i=0;i<6;i++)
  {
    if(flags & (1<<i))
    {
      env.Init();
      light.Type = sLT_POINT;
      light.Position.Init(0,0,-1024);
      light.Direction.Init(0,0,1);
      light.Flags = 0;
      light.Color = 0xffffffff;
      light.Amplify = 1.0f;
      light.Range = 2048;
      light.Event = 0;
      light.Id = 0;

      mat.InitClassify(i);
      mat.Trans3();

      boxmin.Rotate34(mat,box.Min);
      boxmax.Rotate34(mat,box.Max);

      env.CameraSpace.l.x = (boxmin.x + boxmax.x)/2;
      env.CameraSpace.l.y = (boxmin.y + boxmax.y)/2;
      env.CameraSpace.l.z = -256;
      env.ZoomX = 2.0f/sFAbs(boxmax.x-boxmin.x);
      env.ZoomY = 2.0f/sFAbs(boxmax.y-boxmin.y);
      env.Orthogonal = 1;

      KEnvironment *kenv = new KEnvironment;
      kenv->InitView();
      kenv->InitFrame(0,0);

      view.InitTexMS(tex);
      view.Window.x0 = bm->XSize *  fieldcount    / fieldmax;
      view.Window.x1 = bm->XSize * (fieldcount+1) / fieldmax;


      sSystem->SetViewport(view);
      sSystem->Clear(sVCF_ALL,0x00000000);
//      sSystem->SetViewProject(env);

      mesh->Prepare();

      Engine->StartFrame();
      Engine->SetViewProject(env);

      Engine->AddPaintJob(mesh->PreparedMesh,mat,0.0f);
      Engine->AddLightJob(light);

      Engine->ProcessPortals(kenv,0);
      Engine->Paint(kenv);

      kenv->ExitFrame();
      delete kenv;

      fieldcount++;
    }
  }

  GenOverlayManager->SetMasterViewport(saved);
  RenderTargetManager->SetMasterViewport(saved);

  sSystem->ReadTexture(tex,(sU16 *)bm->Data);
  sSystem->RemTexture(tex);

  sRelease(mesh);
  
  return bm;
}

#endif // sLINK_ENGINE

GenBitmap * __stdcall Bitmap_Export(GenBitmap *bm,sChar *filename)
{
  NOTEXTURES1(bm);
  if(CheckBitmap(bm)) return 0;

  return bm;
}

GenBitmap * __stdcall Bitmap_Paste(GenBitmap *bm,GenBitmap *bs,sF322 pre,sF322 pos,sF322 size,sF32 angle,sInt mode)
{
  NOTEXTURES2(bm,bs);
  if(CheckBitmap(bm)) return 0;

  // sign stuff
  if(pre.x < 0.0f)
    pre.x = -pre.x, size.x = -size.x;

  if(pre.y < 0.0f)
    pre.y = -pre.y, size.y = -size.y;

  pre.x *= bs->XSize;
  pre.y *= bs->YSize;

  // calc transformed quad points
  sF32 s,c;
  sFSinCos(angle*sPI2F,s,c);

  sF32 ux = size.x * c;
  sF32 uy = size.x * -s;
  sF32 vx = size.y * s;
  sF32 vy = size.y * c;
  sF32 orgx = pos.x - 0.5f * (ux + vx);
  sF32 orgy = pos.y - 0.5f * (uy + vy);

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
  sInt u0 = (rmx*vy - rmy*vx) * invM * pre.x;
  sInt v0 = (ux*rmy - uy*rmx) * invM * pre.y;
  sInt dudx = vy * invM * pre.x / XRes;
  sInt dvdx = -uy * invM * pre.y / XRes;
  sInt dudy = -vx * invM * pre.x / YRes;
  sInt dvdy = ux * invM * pre.y / YRes;
  sU32 maxU = pre.x * 65536;
  sU32 maxV = pre.y * 65536;

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

        if(mode)
          Fade64(out[x],out[x],pix,(pix >> 48) << 1);
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
/***   Generator                                                          ***/
/***                                                                      ***/
/****************************************************************************/

GenBitmap::GenBitmap()
{
  Data = 0;
  XSize = 0;
  YSize = 0;
  Size = 0;
  ClassId = KC_BITMAP;
  Format = sTF_A8R8G8B8;
  //Format = sTF_DXT1;
  TexMipCount = 0;
  TexMipTresh = 0;
}

GenBitmap::~GenBitmap()
{
  if(Data) delete[] Data;
}

void GenBitmap::Copy(KObject *o)
{
  GenBitmap *ob;

  sVERIFY(o->ClassId==KC_BITMAP);

  ob = (GenBitmap *) o;
  sVERIFY(ob->Data);

  Init(ob->XSize,ob->YSize);

  Format = ob->Format;
  TexMipCount = ob->TexMipCount;
  TexMipTresh = ob->TexMipTresh;

  sCopyMem4((sU32 *)Data,(sU32 *)ob->Data,Size*2);
}

KObject *GenBitmap::Copy()
{
  GenBitmap *r;
  r = new GenBitmap;
  r->Copy(this);
  return r;
}

void GenBitmap::Init(sInt x,sInt y)
{
  XSize = x;
  YSize = y;
  Size = x*y;
  Data = new sU64[Size];
}

} // namespace

/****************************************************************************/
/****************************************************************************/
