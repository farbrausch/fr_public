// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "genbitmap.hpp"
#include "rtmanager.hpp"
#include "_start.hpp"
#include "mmintrin.h"
#include "xmmintrin.h"

#define SCRIPTVERIFY(x) {if(!(x)) return 0;}
#define SCRIPTVERIFYVOID(x) {return 0; }


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

#define BI_HARDLIGHT  16
#define BI_OVER       17
#define BI_ADDSMOOTH  18

#define BI_RANGE      19

/****************************************************************************/
/***                                                                      ***/
/***   Helpers                                                            ***/
/***                                                                      ***/
/****************************************************************************/

static GenBitmap *NewBitmap(sInt xs,sInt ys)
{
  GenBitmap *bm;
  xs = sRange(xs+GenBitmapTextureSizeOffset,12,0);
  ys = sRange(ys+GenBitmapTextureSizeOffset,12,0);
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

void Fade64(sU64 &r,sU64 &c0,sU64 &c1,sInt fade)
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
  ctx->XSize1 = (w-1)<<3;
  ctx->YSize1 = h-1;
  ctx->XAm = ((b&1) ? 0xffffffff : w-1) << 3;
  ctx->YAm = (b&2) ? 0xffffffff : h-1;
}

void BilinearFilter(BilinearContext *ctx,sU64 *r,sInt u,sInt v)
{
  static const sU64 adjust = 0x8000800080008000;
  
  __asm
  {
    mov       esi, [ctx];
    mov       edi, [esi]BilinearContext.Src;

    movd      mm1, [v];

    // calc s0
    mov       eax, [v];
    lea       ebx, [eax-0x80000000];
    sar       eax, 16;
    sar       ebx, 31;
    mov       edx, eax;
    mov       ecx, [esi]BilinearContext.XShift;

    and       eax, [esi]BilinearContext.YAm;
    and       ebx, [esi]BilinearContext.YSize1;
    cmp       eax, [esi]BilinearContext.YSize1;
    jbe       ok1;
    mov       eax, ebx;

ok1:
    shl       eax, cl;
    add       edi, eax;

    movd      mm0, [u];

    // calc s1
    lea       eax, [edx+1];
    lea       ebx, [edx+0x7fffffff];
    and       eax, [esi]BilinearContext.YAm;
    sar       ebx, 31;
    and       ebx, [esi]BilinearContext.YSize1;
    cmp       eax, [esi]BilinearContext.YSize1;
    jbe       ok2;
    mov       eax, ebx;

ok2:
    shl       eax, cl;
    add       eax, [esi]BilinearContext.Src;
    mov       ecx, eax;

    // u+0
    mov       eax, [u];
    lea       ebx, [eax-0x80000000];
    sar       eax, 13;
    sar       ebx, 31;
    mov       edx, eax;

    and       eax, [esi]BilinearContext.XAm;
    and       ebx, [esi]BilinearContext.XSize1;

    cmp       eax, [esi]BilinearContext.XSize1;
    jbe       ok3;
    mov       eax, ebx;

ok3:
    movq      mm2, [edi+eax];
    movq      mm4, [ecx+eax];

    // u+1
    lea       eax, [edx+8];
    lea       ebx, [edx+0x7ffffff8];
    and       eax, [esi]BilinearContext.XAm;
    sar       ebx, 31;
    and       ebx, [esi]BilinearContext.XSize1;
    cmp       eax, [esi]BilinearContext.XSize1;
    jbe       ok4;
    mov       eax, ebx;

ok4:
    movq      mm3, [edi+eax];
    movq      mm5, [ecx+eax];

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

    movq      mm7,[esi+ecx+8]             ; mm7 = data[1]
    psubsw    mm7,mm2
    pxor      mm1,mm1
    lea       eax,loop13

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

loop13:                      // color range
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
  SCRIPTVERIFY(mode>=0 && mode<9);
  if(mode==5) mode=BI_BRIGHTNESS;  // ***SIZE
  if(mode>=6 && mode <=8) mode += BI_HARDLIGHT-6;
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

  sInt icx=cx,icy=cy,isx=sx,isy=sy;

  tresh = 1.0f/65536.0f;
  if(rx<tresh) rx=tresh;
  rx = 1.0f/(rx*rx);

  if(ry<tresh) ry=tresh;
  ry = 1.0f/(ry*ry);

  alpha *= 32768.0f;
  col = GetColor64(color);
  fm = alpha;

  for(x=0;x<1025;x++)
  {
    if(bug)
      GammaTable[x] = sRange7fff(sFPow(1.0f-x/1024.0f,power)*alpha)*2;
    else
      GammaTable[x] = sRange7fff((1.0f-sFPow(x/1024.0f,power*2.0f))*alpha)*2;
  }

  // there are very steep slopes around 0, so don't try approximating them
  for(x=0;x<32;x++)
  {
    if(bug)
      LowTable[x] = sRange7fff(sFPow(1.0f-x/32768.0f,power)*alpha)*2;
    else
      LowTable[x] = sRange7fff((1.0f-sFPow(x/32768.0f,power*2.0f))*alpha)*2;
  }

  d = bm->Data;
  for(y=0;y<bm->YSize;y++)
  {
    fy = sFAbs(y-cy)-sy;
    if(fy<0) fy = 0;
    fy *= fy*ry;

    for(x=0;x<bm->XSize;x++)
    {
      fx = sFAbs(x-cx)-sx;
      if(fx<0) fx = 0;
      
      a = fx*fx*rx+fy;
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
    while(x<bm->XSize);
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

// registers are tight here, we don't want a frame pointer
#pragma optimize("y",on)

GenBitmap * __stdcall Bitmap_Blur(GenBitmap *bm,sInt flags,sF32 sx,sF32 sy,sF32 _amp)
{
  NOTEXTURES1(bm);

  if(CheckBitmap(bm)) return 0;
  
  sInt x,y;
  sInt size,size2;
  sU32 mask;
  sInt s1,s2,d;
  __m64 akku0,akku1,f1mf0,f0f1,out;
  __m64 pix1,pix2,mix1,mix2;
  __m64 amplo,amplos,amphi,aklo,akhi;
  __m64 ampclip,add;
  static const __int64 addc = 0x8000800080008000;
  sU16 *p,*q,*pp,*qq,*po,*qo;
  sInt ordercount;
  sInt amp,ampc,famp;
  sInt f0,f1;
  sInt order;

// prepare

  pp = (sU16 *)bm->Data;
  qq = (sU16 *) new sU64[bm->Size];
  order = flags & 15;
  if(order==0) return bm;

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
    f1mf0 = _mm_set_pi16(0,f1-f0,0,f1-f0);
    f0f1 = _mm_set_pi16(f1,f0,f1,f0);
    amplo = _mm_set_pi16(amp,amp,amp,amp);
    amplos = _mm_srli_pi16(amplo,1);
    amphi = _mm_set_pi16(amp>>16,amp>>16,amp>>16,amp>>16);
    ampclip = _mm_set_pi32(ampc,ampc);
    *((__int64 *) &add) = addc;

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

        s2 = s1 = -((size+1)/2)*4;
        d = 0;

        pix1 = *((__m64 *) (p + ((s2+0) & mask)));
        akku0 = _mm_madd_pi16(_mm_unpacklo_pi16(pix1,pix1),f1mf0);
        akku1 = _mm_madd_pi16(_mm_unpackhi_pi16(pix1,pix1),f1mf0);
        for(x=0;x<size;x++)
        {
          pix1 = *((__m64 *) (p + ((s1+0) & mask)));
          pix2 = *((__m64 *) (p + ((s1+4) & mask)));
          mix1 = _mm_unpacklo_pi16(pix1,pix2);
          mix2 = _mm_unpackhi_pi16(pix1,pix2);
          akku0 = _mm_add_pi32(akku0,_mm_madd_pi16(mix1,f0f1));
          akku1 = _mm_add_pi32(akku1,_mm_madd_pi16(mix2,f0f1));
          s1 += 4;
        }

        for(x=0;x<bm->XSize;x++)
        {
          pix1 = *((__m64 *) (p + ((s1+0) & mask)));
          pix2 = *((__m64 *) (p + ((s1+4) & mask)));
          mix1 = _mm_unpacklo_pi16(pix1,pix2);
          mix2 = _mm_unpackhi_pi16(pix1,pix2);
          akku0 = _mm_add_pi32(akku0,_mm_madd_pi16(mix1,f0f1));
          akku1 = _mm_add_pi32(akku1,_mm_madd_pi16(mix2,f0f1));
          s1 += 4;

          akhi = _mm_packs_pi32(_mm_srli_pi32(akku0,22),_mm_srli_pi32(akku1,22));
          aklo = _mm_packs_pi32(_mm_srai_pi32(_mm_slli_pi32(akku0,10),16),_mm_srai_pi32(_mm_slli_pi32(akku1,10),16));
          out = _mm_packs_pi32(_mm_cmpgt_pi32(akku0,ampclip),_mm_cmpgt_pi32(akku1,ampclip));
          out = _mm_adds_pu16(out,_mm_mullo_pi16(akhi,amplo));
          out = _mm_adds_pu16(out,_mm_mullo_pi16(aklo,amphi));
          out = _mm_adds_pu16(out,_mm_mulhi_pu16(aklo,amplo)); // needs p3
          out = _mm_add_pi16(out,add);
          out = _mm_subs_pi16(out,add);

          *((__m64 *) (q+d)) = out;
          d += 4;

          pix1 = *((__m64 *) (p + ((s2+0) & mask)));
          pix2 = *((__m64 *) (p + ((s2+4) & mask)));
          mix1 = _mm_unpacklo_pi16(pix2,pix1);
          mix2 = _mm_unpackhi_pi16(pix2,pix1);
          akku0 = _mm_sub_pi32(akku0,_mm_madd_pi16(mix1,f0f1));
          akku1 = _mm_sub_pi32(akku1,_mm_madd_pi16(mix2,f0f1));
          s2 += 4;
        }
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

  SCRIPTVERIFY(sx!=0 && sy!=0);
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
  m20 = sFtol( tx*xs*0x10000 - ((txs-1)*m00+(tys-1)*m10)/2);
  m21 = sFtol( ty*ys*0x10000 - ((txs-1)*m01+(tys-1)*m11)/2);
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

  SCRIPTVERIFY(rx!=0 && ry!=0);
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
      bm = Bitmap_Merge(mode&15,2,bm,bb);
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
      bm->AddRef();
      bx = Bitmap_Rotate(bm,angle*i,(sx-1)*i+1,(sy-1)*i+1,(tx-0.5)*i+0.5,(ty-0.5)*i+0.5,border,0,0);
      bb = Bitmap_Merge(mode&15,2,bb,bx);
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

  for(y=0;y<xs;y++)
  {
    for(x=0;x<ys;x++)
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
      vx = filterbump(sx,x*4,(xs-1)*4,4);
      vy = filterbump(sy,y*xs*4,(ys-1)*xs*4,xs*4);
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
#if !sINTRO
  sInt w0,w1,w2;
  sInt count,chr;
  sInt page;
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
#if !sINTRO
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
#endif
  {
    op->SetBlob(0,0);

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
        if(text[i]=='\n')
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
  sInt x,y,px,py,noffs;
  sInt shiftx,shifty;
  sU64 c0,c1;
  sInt val;
  sF32 n,nn,s;
  sU64 *tile;

  bm = NewBitmap(xs,ys);

  c0 = GetColor64(col0);
  c1 = GetColor64(col1);

  shiftx = sGetPower2(bm->XSize);
  shifty = sGetPower2(bm->YSize);
  tile = bm->Data;
  seed &= 255;

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

  for(y=0;y<bm->YSize;y++)
  {
    for(x=0;x<bm->XSize;x++)
    {
      n = 0;
      s = 1.0f;
      for(i=freq;i<freq+oct;i++)
      {
        px = ((x)<<(16-shiftx))<<i;
        py = ((y)<<(16-shifty))<<i;

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
      val = GetGamma(val);
      Fade64(*tile,c0,c1,val);
      tile++;
    }
  }

  return bm;
}

/****************************************************************************/

GenBitmap * __stdcall Bitmap_Cell(sInt xs,sInt ys,sU32 col0,sU32 col1,sU32 col2,sInt max,sInt seed,sF32 amp,sF32 gamma,sInt mode,sF32 mindistf,sInt percent,sF32 aspect)
{
  NOTEXTURES0();

  GenBitmap *bm;
  static sInt cells[256][3];
  static sInt cellt[256];
  sInt x,y,i,j,dist,best,best2,besti;
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
  sF32 aspsquare = aspect * aspect;
  sF32 aspdiv = 1.0f / (16384.0f * aspect);

  for(y=0;y<bm->XSize;y++)
  {
    py = (y) << shifty;
    for(i=0;i<max;i++)
    {
      dy = ((cells[i][1]-py)&0x3fff)-0x2000;
      cellt[i] = dy*dy*aspsquare;
    }

    for(x=0;x<bm->YSize;x++)
    {
      best = 0x8000*0x8000;
      best2 = 0x8000*0x8000;
      besti = -1;
      px = (x)<<(shiftx);
      for(i=0;i<max;i++)
      {
        dx = ((cells[i][0]-px)&0x3fff)-0x2000;
        dist = dx*dx + cellt[i];
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
    sideakku = (by*sidestep)&0x3fff;
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
/****************************************************************************/

#include "genoverlay.hpp"
#include "genminmesh.hpp"

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

  view.InitTex(tex);
  sSystem->SetViewport(view);
  sSystem->Clear(sVCF_ALL,0xffff8080);

  // taken vom   Exec_IPP_RenderTarget
  {
    // just tweak the master viewport
    sViewport saved,newvp;

    GenOverlayManager->GetMasterViewport(saved);
    newvp.InitTex(tex);
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
  newvp.InitTex(tex);
  GenOverlayManager->SetMasterViewport(newvp);
  RenderTargetManager->SetMasterViewport(newvp);

  for(sInt i=0;i<6;i++)
  {
    if(flags & (1<<i))
    {
      env.Init();
      light.Position.Init(0,0,-1024);
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

      view.InitTex(tex);
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
  Texture = sINVALID;
  ClassId = KC_BITMAP;
  Format = sTF_A8R8G8B8;
  TexMipCount = 0;
  TexMipTresh = 0;
}

GenBitmap::~GenBitmap()
{
  if(Data) delete[] Data;
  if(Texture!=sINVALID) sSystem->RemTexture(Texture);
}

void GenBitmap::Copy(KObject *o)
{
  GenBitmap *ob;

  sVERIFY(o->ClassId==KC_BITMAP);

  ob = (GenBitmap *) o;
  sVERIFY(ob->Data);

  Texture = sINVALID;
  Init(ob->XSize,ob->YSize);
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


void GenBitmap::MakeTexture(sInt format)
{
  if(Texture!=sINVALID)
    sSystem->RemTexture(Texture);
  if(format!=0)
    Format = format;
  Texture = sSystem->AddTexture(XSize,YSize,Format,(sU16 *)Data,TexMipCount,TexMipTresh);
}

/****************************************************************************/
/****************************************************************************/
