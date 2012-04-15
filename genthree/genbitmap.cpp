// This file is distributed under a BSD license. See LICENSE.txt for details.
#include "genbitmap.hpp"
#include "_start.hpp"
#include "cslrt.hpp"

/****************************************************************************/

#if sINTRO_X
const sInt BitmapXSize=256;
const sInt BitmapYSize=256;
#else
static sInt BitmapXSize=256;
static sInt BitmapYSize=256;
#endif

/****************************************************************************/
/***                                                                      ***/
/***   Helpers                                                            ***/
/***                                                                      ***/
/****************************************************************************/

sInt sRange7fff(sInt a)
{
  if(a<0) return 0;
  if(a>0x7fff) return 0x7fff;
  return a;
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

sU32 GetColor32(const sInt4 &colx)
{
  sU32 col;
  col = ((sU32)sRange<sInt>(colx.z>>8,0xff,0))<<0 
      | ((sU32)sRange<sInt>(colx.y>>8,0xff,0))<<8 
      | ((sU32)sRange<sInt>(colx.x>>8,0xff,0))<<16 
      | ((sU32)sRange<sInt>(colx.w>>8,0xff,0))<<24;

  return col;
}

void Fade64(sU64 &r,sU64 &c0,sU64 &c1,sInt fade)
{
  _asm
  {
    emms

    mov eax,[fade]
    sar eax,1

    mov ebx,[c0]
    movq mm2,[ebx]
    mov ebx,[c1]
    movq mm3,[ebx]
    movd mm0,eax
    packssdw mm0,mm0
    punpcklwd mm0,mm0
    punpcklwd mm0,mm0
    pmulhw mm3,mm0
    neg eax
    add eax,0x8000
    movd mm0,eax
    packssdw mm0,mm0
    punpcklwd mm0,mm0
    punpcklwd mm0,mm0
    pmulhw mm2,mm0
    paddw mm2,mm3
    psllw mm2,1
    mov ebx,[r]
    movq [ebx],mm2

    emms
  }
}

sInt border(sInt v,sInt m,sInt b)
{
  if(b)
  {
    if(v<0) return 0;
    if(v>m) return m;
    return v;
  }
  else
  {
    return v&m;
  }
}

void BilinearFilter(sU64 *r,sU64 *src,sInt w,sInt h,sInt u,sInt v,sInt b)
{
  sU64 c0,c1;
  sInt fx,fy,uu;
  sU64 *s0,*s1;

  fx = (u&0xffff); u>>=16; 
  fy = (v&0xffff); v>>=16; 

  s0 = src+(border(v,h-1,b&12)*w);
  s1 = src+(border(v+1,h-1,b&12)*w);
  uu = border(u,w-1,b&3);
  Fade64(c0,s0[uu],s1[uu],fy);
  uu = border(u+1,w-1,b&3);
  Fade64(c1,s0[uu],s1[uu],fy);
  
  Fade64(*r,c0,c1,fx);
}
/*
#pragma warning (push)
#pragma warning (disable: 4799) // no EMMS at end of function

// you need to do emms yourself! u,v 16.15 fixpoint
void BilinearFilter(sU16 *r,sU16 *src,sInt w,sInt h,sInt u,sInt v,sInt border)
{
	sU16* smp[4];
	sInt rc[2];
	sInt i,j,c,d,b;

	// calc coord (not as much as it looks like)
	for(i=0;i<4;i++)
	{
		rc[0] = (u>>15)+(i&1);
		rc[1] = (v>>15)+(i>>1);
		b = border;
		
		for(j=0;j<2;j++)
		{
			c = rc[j];
			d = j ? h : w;

			switch(b&3)
			{
			case 0: // wrap
				c &= d-1;
				break;

			case 1: // clamp
				if(((sU32) c)>=(sU32) d) c = (c >= d) ? d-1 : 0;
				break;

			case 2: // mirror
				c &= d*2-1;
				if(c>=d) c=d*2-1-c;
				break;
			}

			rc[j] = c;
			b >>= 2;
		}

		smp[i] = src + (rc[1] * w + rc[0]) * 4;
	}

	u = u & 0x7fff;
	v = v & 0x7fff;

	__asm
	{
		// setup+read
		lea			esi, [smp];
		mov			eax, [esi];

		pcmpeqb	mm0, mm0;
		psrlw		mm0, 1;

		movd		mm4, [u];
		punpcklwd	mm4, mm4;
		punpcklwd	mm4, mm4;
		movq		mm5, mm0;
		pxor		mm5, mm4;

		movd		mm6, [v];
		punpcklwd	mm6, mm6;
		punpcklwd	mm6, mm6;
		movq		mm7, mm0;
		pxor		mm7, mm6;

		movq		mm0, [eax];
		mov			ebx, [esi + 4];
		movq		mm1, [ebx];
		mov			ecx, [esi + 8];
		movq		mm2, [ecx];
		mov			edx, [esi + 12];
		movq		mm3, [edx];
		mov			eax, [r];

		// horizontal
		pmulhw	mm0, mm5;
		pmulhw	mm1, mm4;
		pmulhw	mm2, mm5;
		pmulhw	mm3, mm4;
		paddw		mm0, mm1;
		paddw		mm2, mm3;
		psllw		mm0, 1;
		psllw		mm2, 1;

		// vertical+write
		pmulhw	mm0, mm7;
		pmulhw	mm2, mm6;
		paddw		mm0, mm2;
		psllw		mm0, 1;

		movq		[eax], mm0;
	}
}

#pragma warning (pop)
*/
/*
void Mult16(sU16 *r,sU16 *c0,sU16 *c1)
{
  r[0] = sRange7fff((c0[0]*c1[0])>>14);
  r[1] = sRange7fff((c0[1]*c1[1])>>14);
  r[2] = sRange7fff((c0[2]*c1[2])>>14);
  r[3] = sRange7fff((c0[3]*c1[3])>>14);
}
*/
/****************************************************************************/
/***                                                                      ***/
/***   Operators                                                          ***/
/***                                                                      ***/
/****************************************************************************/

#if !sINTRO_X
void __stdcall Bitmap_SetSize(sInt x,sInt y)
{
  BitmapYSize = 1<<(x>>16);
  BitmapXSize = 1<<(y>>16);
}
#endif

/****************************************************************************/

GenBitmap * __stdcall Bitmap_MakeTexture(GenBitmap *bm)
{
  sInt i,c;
  sU16 *s;
  sU8 *d;
  sBitmap *dbm;
  sTexInfo ti;

  dbm = new sBitmap;
  dbm->Init(bm->XSize,bm->YSize);

  s = (sU16 *)bm->Data;
  d = (sU8 *)dbm->Data;
	c = bm->Size;
  for(i=0;i<c*4;i++)
    d[i]=s[i]>>7;


  sVERIFY(bm->Texture==sINVALID);
  
#if sINTRO
  sVERIFY(bm->Texture==sINVALID);
#else 
  if(bm->Texture!=sINVALID)
    sSystem->RemTexture(bm->Texture);
#endif
  ti.Init(dbm);
  bm->Texture = sSystem->AddTexture(ti);

  return bm;
}

/****************************************************************************/

GenBitmap * __stdcall Bitmap_Flat(sInt4 color)
{
  GenBitmap *bm;
  sU64 col;

  bm = new GenBitmap;
  bm->Init(BitmapXSize,BitmapYSize);
  col = GetColor64(&color.x);
  sSetMem8(bm->Data,col,bm->Size);

  return bm;
}

/****************************************************************************/

void __stdcall Bitmap_Inner(sU64 *d,sU64 *s,sInt count,sInt mode)
{
  __asm
  {
    emms

    mov     esi,[s]
    mov     edi,[d]
    mov     ecx,[count]
    mov     edx,[mode]

    pcmpeqb	mm3,mm3;
		psrlw	  mm3,1;
    pcmpeqb	mm4,mm4
    psrlq		mm4,16
    pcmpeqb	mm5,mm5
    pxor		mm5,mm4
    movq    mm6,mm5
    psrlw   mm6,1
    movq    mm2,[esi]

    lea     eax,loop0
    sub     edx,1
    js      loop_start

    lea     eax,loop1
    sub     edx,1
    js      loop_start

    lea     eax,loop2
    sub     edx,1
    js      loop_start

    lea     eax,loop3
    sub     edx,1
    js      loop_start

    lea     eax,loop4
    sub     edx,1
    js      loop_start

    lea     eax,loop5
    sub     edx,1
    js      loop_start

    lea     eax,loop6
    sub     edx,1
    js      loop_start

    lea     eax,loop7
    sub     edx,1
    js      loop_start

    lea     eax,loop8
    sub     edx,1
    js      loop_start

    lea     eax,loop9
    sub     edx,1
    js      loop_start

    lea     eax,loopa

loop_start:
    jmp     eax
loop0:                        // add
		movq    mm0,[edi];
		paddsw  mm0,[esi];
    jmp     loopo

loop1:                        // sub
		movq    mm0,[edi];
		psubusw mm0,[esi];
    jmp     loopo

loop2:                        // mul
    movq    mm0,[edi]
		pmulhw	mm0,[esi];
		psllw		mm0,1;
    jmp     loopo

loop3:                        // diff
    movq    mm0,[edi]
    psubw   mm0,[esi]
    paddw   mm0,mm3
    psrlw   mm0,1
    jmp     loopo

loop4:                        // alpha
		movq		mm0,[esi]
		movq		mm1,[edi]
		movq		mm6,mm0
		movq		mm7,mm0

		psrlq		mm6,16
		psrlq		mm7,32
		paddw		mm0,mm6
		paddw		mm7,mm6
		psrlw		mm0,1
		psrlw		mm7,1
		paddw		mm0,mm7
		pand		mm1,mm4
		psllq		mm0,47
		pand		mm0,mm5
		por			mm0,mm1
    jmp     loopo


loop5:                      // mul color
    movq    mm0,[edi]
	  pmulhw	mm0,mm2
	  psllw	  mm0,1
    jmp     loopo

loop6:                      // add color
    movq    mm0,[edi]
    paddsw  mm0,mm2
    jmp     loopo

loop7:                      // sub color
    movq    mm0,[edi]
    psubusw mm0,mm2
    jmp     loopo

loop8:                      // gray color
		movq			mm0, [edi];
		movq			mm5, mm0;
		movq			mm7, mm0;

		psrlq			mm5, 16;
		psrlq			mm7, 32;
		paddw			mm0, mm5;
		paddw			mm7, mm5;
		psrlw			mm0, 1;
		psrlw			mm7, 1;
		paddw			mm0, mm7;
		psrlw			mm0, 1;

		punpcklwd	mm0, mm0;
		punpcklwd	mm0, mm0;
		por				mm0, mm6;
    jmp     loopo

loop9:                      // invert color
    movq    mm0,[edi]
    pxor    mm0,mm3
    jmp     loopo

loopa:                      // scale color
		movq			mm0, [edi];
		movq			mm1, mm0;
		pmullw		mm0, mm2;
		pmulhw		mm1, mm2;
		movq			mm6, mm0;
		punpcklwd	mm0, mm1;
		punpckhwd	mm6, mm1;
		psrld			mm0, 11;
		psrld			mm6, 11;
		packssdw	mm0, mm6;

loopo:
    movq    [edi],mm0
    add     esi,8
    add     edi,8
    sub     ecx,1
    je      ende
    jmp     eax

ende:
   	__asm emms;
  }
}

GenBitmap * __stdcall Bitmap_Merge(GenBitmap *bm,GenBitmap *bb,sInt mode)
{
  SCRIPTVERIFY(bm);
  SCRIPTVERIFY(bb);
  mode = mode>>16;
  SCRIPTVERIFY(mode>=0 && mode<5);

  Bitmap_Inner(bm->Data,bb->Data,bm->Size,mode);

  return bm;
}

GenBitmap * __stdcall Bitmap_Color(GenBitmap *bm,sInt mode,sInt4 col)
{
  sU64 color;

  SCRIPTVERIFY(bm);
  mode = mode>>16;
  SCRIPTVERIFY(mode>=0 && mode<6);
	color = GetColor64(&col.x);

  Bitmap_Inner(bm->Data,&color,bm->Size,mode+5);

  return bm;
}

/*
GenBitmap * __stdcall Bitmap_Merge(GenBitmap *bm,GenBitmap *bb,sInt mode)
{
  sU16 *d,*s;
  sInt _size;

  SCRIPTVERIFY(bm);
  SCRIPTVERIFY(bb);

  s = (sU16 *)bb->Data;
  d = (sU16 *)bm->Data;
  _size = bm->Size;

	__asm emms;

  switch(mode>>16)
  {
  case 0: // add
		__asm
		{
			mov				esi, [s];
			mov				edi, [d];
			mov				ecx, [_size];
			lea				esi, [esi + ecx * 8];
			lea       edi, [edi + ecx * 8];
			neg				ecx;

addlp:
			movq			mm0, [edi + ecx * 8 + 0];
			movq			mm1, [edi + ecx * 8 + 8];
			movq			mm2, [edi + ecx * 8 + 16];
			movq			mm3, [edi + ecx * 8 + 24];
			paddsw		mm0, [esi + ecx * 8 + 0];
			paddsw		mm1, [esi + ecx * 8 + 8];
			paddsw		mm2, [esi + ecx * 8 + 16];
			paddsw		mm3, [esi + ecx * 8 + 24];
			movq			[edi + ecx * 8 + 0], mm0;
			movq			[edi + ecx * 8 + 8], mm1;
			movq			[edi + ecx * 8 + 16], mm2;
			movq			[edi + ecx * 8 + 24], mm3;
			add				ecx, 4;
			jnz				addlp;
		}
    break;
  case 1: // sub
		__asm
		{
			mov				esi, [s];
			mov				edi, [d];
			mov				ecx, [_size];
			lea				esi, [esi + ecx * 8];
			lea       edi, [edi + ecx * 8];
			neg				ecx;

sublp:
			movq			mm0, [edi + ecx * 8 + 0];
			movq			mm1, [edi + ecx * 8 + 8];
			movq			mm2, [edi + ecx * 8 + 16];
			movq			mm3, [edi + ecx * 8 + 24];
			psubusw		mm0, [esi + ecx * 8 + 0];
			psubusw		mm1, [esi + ecx * 8 + 8];
			psubusw		mm2, [esi + ecx * 8 + 16];
			psubusw		mm3, [esi + ecx * 8 + 24];
			movq			[edi + ecx * 8 + 0], mm0;
			movq			[edi + ecx * 8 + 8], mm1;
			movq			[edi + ecx * 8 + 16], mm2;
			movq			[edi + ecx * 8 + 24], mm3;
			add				ecx, 4;
			jnz				sublp;
		}
    break;
  case 2: // mul (verliert 1bit; 1 * 1 != 1.. fix?)
		__asm
		{
			mov				esi, [s];
			mov				edi, [d];
			mov				ecx, [_size];
			lea				esi, [esi + ecx * 8];
			lea       edi, [edi + ecx * 8];
			neg				ecx;

mullp:
			movq			mm0, [edi + ecx * 8 + 0];
			movq			mm1, [edi + ecx * 8 + 8];
			movq			mm2, [edi + ecx * 8 + 16];
			movq			mm3, [edi + ecx * 8 + 24];
			pmulhw		mm0, [esi + ecx * 8 + 0];
			pmulhw		mm1, [esi + ecx * 8 + 8];
			pmulhw		mm2, [esi + ecx * 8 + 16];
			pmulhw		mm3, [esi + ecx * 8 + 24];
			psllw			mm0, 1;
			psllw			mm1, 1;
			psllw			mm2, 1;
			psllw			mm3, 1;
			movq			[edi + ecx * 8 + 0], mm0;
			movq			[edi + ecx * 8 + 8], mm1;
			movq			[edi + ecx * 8 + 16], mm2;
			movq			[edi + ecx * 8 + 24], mm3;
			add				ecx, 4;
			jnz				mullp;
		}
    break;
  case 3: // diff
		__asm
		{
			mov				esi, [s];
			mov				edi, [d];
			mov				ecx, [_size];
			lea				esi, [esi + ecx * 8];
			lea       edi, [edi + ecx * 8];
			neg				ecx;
			pcmpeqb		mm4, mm4;
			psrlw			mm4, 1;

difflp:
			movq			mm0, [edi + ecx * 8 + 0];
			movq			mm1, [edi + ecx * 8 + 8];
			psubw			mm0, [esi + ecx * 8 + 0];
			psubw			mm1, [esi + ecx * 8 + 8];
			paddw			mm0, mm4;
			paddw			mm1, mm4;
			psrlw			mm0, 1;
			psrlw			mm1, 1;

			movq			[edi + ecx * 8 + 0], mm0;
			movq			[edi + ecx * 8 + 8], mm1;
			add				ecx, 2;
			jnz				difflp;
		}
    break;
  case 4: // make alpha (veliert 1bit)
		// ((s[0] + s[1]) / 2 + (s[1] + s[2]) / 2) / 2
		__asm
		{
			mov				esi, [s];
			mov				edi, [d];
			mov				ecx, [_size];
			lea				esi, [esi + ecx * 8];
			lea       edi, [edi + ecx * 8];
			neg				ecx;
			pcmpeqb		mm4, mm4;
			psrlq			mm4, 16;
			pcmpeqb		mm5, mm5;
			pxor			mm5, mm4;

malphlp:
			movq			mm0, [esi + ecx * 8];
			movq			mm1, [edi + ecx * 8];
			movq			mm6, mm0;
			movq			mm7, mm0;

			psrlq			mm6, 16;
			psrlq			mm7, 32;
			paddw			mm0, mm6;
			paddw			mm7, mm6;
			psrlw			mm0, 1;
			psrlw			mm7, 1;
			paddw			mm0, mm7;
			pand			mm1, mm4;
			psllq			mm0, 47;
			pand			mm0, mm5;
			por				mm0, mm1;

			movq			[edi + ecx * 8], mm0;

			inc       ecx;
			jnz				malphlp;
		}
		break;
  }

	__asm emms;

  return bm;
}
*/
/****************************************************************************/
/*
GenBitmap * __stdcall Bitmap_Color(GenBitmap *bm,sInt mode,sInt4 col)
{
  sU16 *d;
	sU64 color;
  sInt _size;

  SCRIPTVERIFY(bm);

  d = (sU16 *)bm->Data;
  _size = bm->Size;
	color = GetColor64(&col.x);

	_asm emms;

	static const sU64 grayconst = 0x7fff000000000000;

  switch(mode>>16)
  {
  case 0: // mul (veliert 1bit; 1*1!=1... fix?)
		__asm
		{
			mov				edi, [d];
			mov				ecx, [_size];
			lea       edi, [edi + ecx * 8];
			neg				ecx;

			movq			mm4, [color];

mulclp:
			movq			mm0, [edi + ecx * 8 + 0];
			movq			mm1, [edi + ecx * 8 + 8];
			movq			mm2, [edi + ecx * 8 + 16];
			movq			mm3, [edi + ecx * 8 + 24];
			pmulhw		mm0, mm4;
			pmulhw		mm1, mm4;
			pmulhw		mm2, mm4;
			pmulhw		mm3, mm4;
			psllw			mm0, 1;
			psllw			mm1, 1;
			psllw			mm2, 1;
			psllw			mm3, 1;
			movq			[edi + ecx * 8 + 0], mm0;
			movq			[edi + ecx * 8 + 8], mm1;
			movq			[edi + ecx * 8 + 16], mm2;
			movq			[edi + ecx * 8 + 24], mm3;
			add				ecx, 4;
			jnz				mulclp;
		}
		break;
  case 1: // add
		__asm
		{
			mov				edi, [d];
			mov				ecx, [_size];
			lea       edi, [edi + ecx * 8];
			neg				ecx;

			movq			mm4, [color];

addclp:
			movq			mm0, [edi + ecx * 8 + 0];
			movq			mm1, [edi + ecx * 8 + 8];
			movq			mm2, [edi + ecx * 8 + 16];
			movq			mm3, [edi + ecx * 8 + 24];
			paddsw		mm0, mm4;
			paddsw		mm1, mm4;
			paddsw		mm2, mm4;
			paddsw		mm3, mm4;
			movq			[edi + ecx * 8 + 0], mm0;
			movq			[edi + ecx * 8 + 8], mm1;
			movq			[edi + ecx * 8 + 16], mm2;
			movq			[edi + ecx * 8 + 24], mm3;
			add				ecx, 4;
			jnz				addclp;
		}
		break;
  case 2: // sub
		__asm
		{
			mov				edi, [d];
			mov				ecx, [_size];
			lea       edi, [edi + ecx * 8];
			neg				ecx;

			movq			mm4, [color];

subclp:
			movq			mm0, [edi + ecx * 8 + 0];
			movq			mm1, [edi + ecx * 8 + 8];
			movq			mm2, [edi + ecx * 8 + 16];
			movq			mm3, [edi + ecx * 8 + 24];
			psubusw		mm0, mm4;
			psubusw		mm1, mm4;
			psubusw		mm2, mm4;
			psubusw		mm3, mm4;
			movq			[edi + ecx * 8 + 0], mm0;
			movq			[edi + ecx * 8 + 8], mm1;
			movq			[edi + ecx * 8 + 16], mm2;
			movq			[edi + ecx * 8 + 24], mm3;
			add				ecx, 4;
			jnz				subclp;
		}
		break;
  case 3: // gray (verliert 1bit)
		// ((s[0] + s[1]) / 2 + (s[1] + s[2]) / 2) / 2
		__asm
		{
			mov				edi, [d];
			mov				ecx, [_size];
			lea       edi, [edi + ecx * 8];
			neg				ecx;
			movq			mm4, [grayconst];

grayclp:
			movq			mm0, [edi + ecx * 8];
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
			por				mm0, mm4;

			movq			[edi + ecx * 8], mm0;

			inc       ecx;
			jnz				grayclp;
		}
		break;
  case 4: // invert
		__asm
		{
			mov				edi, [d];
			mov				ecx, [_size];
			lea       edi, [edi + ecx * 8];
			neg				ecx;

			pcmpeqb		mm4, mm4;
			psrlw			mm4, 1;

invclp:
			movq			mm0, [edi + ecx * 8 + 0];
			movq			mm1, [edi + ecx * 8 + 8];
			movq			mm2, [edi + ecx * 8 + 16];
			movq			mm3, [edi + ecx * 8 + 24];
			pxor			mm0, mm4;
			pxor			mm1, mm4;
			pxor			mm2, mm4;
			pxor			mm3, mm4;
			movq			[edi + ecx * 8 + 0], mm0;
			movq			[edi + ecx * 8 + 8], mm1;
			movq			[edi + ecx * 8 + 16], mm2;
			movq			[edi + ecx * 8 + 24], mm3;
			add				ecx, 4;
			jnz				invclp;
		}
		break;
  case 5: // scale
		__asm
		{
			mov				edi, [d];
			mov				ecx, [_size];
			lea       edi, [edi + ecx * 8];
			neg				ecx;

			movq			mm4, [color];

sclclp:
			movq			mm0, [edi + ecx * 8 + 0];
			movq			mm1, [edi + ecx * 8 + 8];
			movq			mm2, mm0;
			movq			mm3, mm1;

			pmullw		mm0, mm4;
			pmullw		mm1, mm4;
			pmulhw		mm2, mm4;
			pmulhw		mm3, mm4;

			movq			mm6, mm0;
			movq			mm7, mm1;
			punpcklwd	mm0, mm2;
			punpcklwd	mm1, mm3;
			punpckhwd	mm6, mm2;
			punpckhwd	mm7, mm2;
			psrld			mm0, 11;
			psrld			mm1, 11;
			psrld			mm6, 11;
			psrld			mm7, 11;

			packssdw	mm0, mm6;
			packssdw	mm1, mm7;

			movq			[edi + ecx * 8 + 0], mm0;
			movq			[edi + ecx * 8 + 8], mm1;
			add				ecx, 2;
			jnz				sclclp;
		}
		break;
  }

	__asm emms;

  return bm;
}
*/
/****************************************************************************/

GenBitmap * __stdcall Bitmap_GlowRect(GenBitmap *bm,sF32 cx,sF32 cy,sF32 rx,sF32 ry,sF32 sx,sF32 sy,sInt4 color,sF32 alpha,sF32 power)
{
  sU64 *d;
  sInt x,y;
  sF32 a;
  sInt f;
  sU64 col;
  sF32 fx,fy,tresh;

  SCRIPTVERIFY(bm);

  if(power==0) power=(1.0f/65536.0f);
  power = 0.25/power;

  cx*=bm->XSize;
  cy*=bm->YSize;
  rx*=bm->XSize;
  ry*=bm->YSize;
  sx*=bm->XSize;
  sy*=bm->YSize;

  tresh = 1.0f/65536.0f;
  if(rx<tresh) rx=tresh;
  rx = rx*rx; 

  if(ry<tresh) ry=tresh;
  ry = ry*ry; 

  alpha *= 65536.0f;
  col = GetColor64(&color.x);

  d = bm->Data;
  y=0; do
  {
    fy = y-cy;
    
    if(fy<-sy) fy=fy+sy;
    else if(fy>sy) fy=fy-sy;
    else fy = 0;

    x=0; do
    {
      fx = x-cx;
      
      if(fx<-sx) fx=fx+sx;
      else if(fx>sx) fx=fx-sx;
      else fx = 0;

      a = 1-(fx*fx/rx+fy*fy/ry);
      if(a>0)
      {
        f = sFtol(sFPow(a,power)*alpha);
        Fade64(*d,*d,col,f);
      }

      d++;
      x++;
    }
    while(x<bm->XSize);
    y++;
  }
  while(y<bm->YSize);

  return bm;
}

/****************************************************************************/

#if !sINTRO
void __stdcall Bitmap_SetTexture(GenBitmap *bm,sInt stage)
{
  SCRIPTVERIFYVOID(bm);

  if(bm->Texture!=sINVALID)
    sSystem->SetTexture(stage>>16,bm->Texture,0);
}
#endif

/****************************************************************************/

#if !sINTRO
GenBitmap * __stdcall Bitmap_Dots(GenBitmap *bm,sInt4 color0,sInt4 color1,sInt count,sInt seed)
{
  sU64 *d;
  sInt f;
  sInt x;
  sU64 c0,c1;


  SCRIPTVERIFY(bm);

  c0 = GetColor64(&color0.x);
  c1 = GetColor64(&color1.x);
  count = sMulShift(bm->Size,count)>>8;

  sSetRndSeed(seed>>16);
 
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
#endif


/****************************************************************************/
/***                                                                      ***/
/***   HSCB                                                               ***/
/***                                                                      ***/
/****************************************************************************/

//#if !sINTRO
GenBitmap * __stdcall Bitmap_HSCB(GenBitmap *bm,sInt h,sInt s,sInt c,sInt b)
{
  SCRIPTVERIFY(bm);

  sU16 *d;
  sInt i;

  sF32 fh,fs,fc,fb;
  sF32 ch,cs,cv,cr,cg,cb,min,max;

  d = (sU16 *) bm->Data;

  fh = h/65536.0f;
  fs = s/65536.0f;
  fc = c/65536.0f;
  fb = b/65536.0f;

  fc = fc*fc;

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

    cr = sFPow((d[2]+0.01)/32768.0f,fc)*32768.0f*fb;
    cg = sFPow((d[1]+0.01)/32768.0f,fc)*32768.0f*fb;
    cb = sFPow((d[0]+0.01)/32768.0f,fc)*32768.0f*fb;

// convert to hsv

    max = sMax(cr,sMax(cg,cb));
    min = sMin(cr,sMin(cg,cb));
    
    cv = max;
    if(max==min)
    {
      cs = 0;
      ch = 0;
    }
    else
    {
      cs = (max-min)/max;
      if(cr==max)
        ch = 0+(cg-cb)/(max-min);
      else if(cg==max)
        ch = 2+(cb-cr)/(max-min);
      else if(cb==max)
        ch = 4+(cr-cg)/(max-min);
    }

// hue and saturation

    ch += fh*6;
    cs *= fs;

// convert to rgb

    while(ch<-1) ch+=6;
    while(ch>5) ch-=6;

    max = cv;
    min = max-(cs*max);
    
    if(ch<0)
    {
      cr = max;
      cg = min;
      cb = min-ch*(max-min);
    }
    else if(ch<1)
    {
      cr = max;
      cb = min;
      cg = min+ch*(max-min);
    }
    else if(ch<2)
    {
      cg = max;
      cb = min;
      cr = min-(ch-2)*(max-min);
    }
    else if(ch<3)
    {
      cg = max;
      cr = min;
      cb = min+(ch-2)*(max-min);
    }
    else if(ch<4)
    {
      cb = max;
      cr = min;
      cg = min-(ch-4)*(max-min);
    }
    else if(ch<5)
    {
      cb = max;
      cg = min;
      cr = min+(ch-4)*(max-min);
    }

    d[2] = sRange7fff(sFtol(cr));
    d[1] = sRange7fff(sFtol(cg));
    d[0] = sRange7fff(sFtol(cb));
    d[3] = d[3];
    d+=4;
  }
  return bm;
}
//#endif


/****************************************************************************/

#if !sINTRO
GenBitmap * __stdcall Bitmap_Wavelet(GenBitmap *bm,sInt mode,sInt count)
{
  sU64 *mem;
  sU16 *s,*d;
  sInt steps;
  sInt x,y,xs,ys,f,g;  

  SCRIPTVERIFY(bm);

  xs = bm->XSize;
  ys = bm->YSize;
  f = xs*4;
  g = xs*8;

  mem = new sU64[xs*ys];

  for(steps = 0;steps<(count>>16);steps++)
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
//    sCopyMem8(bm->Data,mem,xs*ys);
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
#endif

/****************************************************************************/
/***                                                                      ***/
/***   Blur                                                               ***/
/***                                                                      ***/
/****************************************************************************/

#define SMALLBLUR 1
GenBitmap * __stdcall Bitmap_Blur(GenBitmap *bm,sInt order,sInt sx,sInt sy,sInt _amp)
{
  SCRIPTVERIFY(bm);
  
  sInt x,y;
  sInt size;
  sU32 mask;
#if !SMALLBLUR
  sInt xx;
  sInt shift;
#endif
  sInt s1,s2,d;
  sInt akku[4*32];
  sU16 *p,*q,*pp,*qq;
  sInt ordercount;
  sInt amp;

// prepare

  pp = (sU16 *)bm->Data;
  qq = (sU16 *) new sU64[bm->Size];
  order = order>>16;
  if(order==0) return bm;

// blur x

  size = (sx*bm->XSize)>>16;

#if SMALLBLUR
  sInt repeat = 1;
repeatlabel:
#endif
  mask = ((bm->XSize-1)*4)|3;
  amp = (_amp)/sMax(1,size);
  ordercount = order;
  do
  {
    p = pp;
    q = qq;
    pp = q;
    qq = p;
    y=bm->YSize;
    do
    {
      s2 = s1 = -((size+1)/2)*4;
      d = 0;
      akku[0] = 0;
      akku[1] = 0;
      akku[2] = 0;
      akku[3] = 0;
      for(x=0;x<size;x++)
      {
        akku[0] += p[(s1++)&mask];
        akku[1] += p[(s1++)&mask];
        akku[2] += p[(s1++)&mask];
        akku[3] += p[(s1++)&mask];
      }
      for(x=0;x<bm->XSize*4;x++)
      {
        q[d++] = sRange7fff(sMulShift(akku[x&3],amp));
        akku[x&3] += p[(s1++)&mask] - p[(s2++)&mask];
      }
      p+=bm->XSize*4;
      q+=bm->XSize*4;
      y--;
    }
    while(y);
    ordercount--;
  }
  while(ordercount);

#if SMALLBLUR
  p = pp;
  q = qq;
  pp = q;
  qq = p;
  for(y=0;y<bm->YSize;y++)
    for(x=0;x<bm->XSize;x++)
      ((sU64 *)q)[x*bm->YSize+y] = ((sU64 *)p)[y*bm->XSize+x];
#if !sINTRO_X
  sSwap(bm->XSize,bm->YSize);
#endif
  if(repeat)
  {
    repeat = 0;
    size = (sy*bm->XSize)>>16;
    goto repeatlabel;
  }
#else
// blur y

  amp = _amp/sMax(1,size);
  mask = (bm->YSize-1)*bm->XSize*4;
  shift = bm->XSize*4;

  ordercount = order;
  do
  {
    p = pp;
    q = qq;

    for(xx=0;xx<bm->XSize;xx+=32)
    {
      s2 = s1 = -((size+1)/2)*shift;
      d = 0; 
      p = pp+xx*4;
      q = qq+xx*4;
      x=32*4;
      do
      {
        x--;
        akku[x] = 0;
      }
      while(x);
      for(y=0;y<size;y++)
      {
        x=32*4;
        do
        {
          x--;
          akku[x] += p[x+(s1&mask)];
        }
        while(x);
        s1+=shift;
      }
      y = bm->YSize;
      do
      {
        x=32*4;
        do
        {
          x--;
          q[x] = sRange7fff(sMulShift(akku[x],amp));
          akku[x] += p[x+(s1&mask)] - p[x+(s2&mask)];
        }
        while(x);
        s1+=shift;
        s2+=shift;
        q+=shift;
        y--;
      }
      while(y);
    }
    p = pp;
    q = qq;
    pp = q;
    qq = p;
    ordercount--;
  }
  while(ordercount);
#endif
  sVERIFY(qq!=(sU16 *)bm->Data);
  delete[] qq;

  return bm;
}

/****************************************************************************/
/***                                                                      ***/
/***   Mask                                                               ***/
/***                                                                      ***/
/****************************************************************************/

GenBitmap * __stdcall Bitmap_Mask(GenBitmap *bm,GenBitmap *bb,GenBitmap *bc)
{
  sInt size;

  SCRIPTVERIFY(bm);
  SCRIPTVERIFY(bb);
  SCRIPTVERIFY(bc);
  SCRIPTVERIFY(bm->Size==bb->Size);
  SCRIPTVERIFY(bm->Size==bc->Size);

  size = bm->Size;
  Bitmap_Inner(bm->Data,bm->Data,size,3+5);     // gray
  Bitmap_Inner(bc->Data,bm->Data,size,2);       // mul
  Bitmap_Inner(bm->Data,bm->Data,size,4+5);     // invert
  Bitmap_Inner(bm->Data,bb->Data,size,2);       // mul;
  Bitmap_Inner(bm->Data,bc->Data,size,0);       // add

  return bm;
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

GenBitmap * __stdcall Bitmap_Rotate(GenBitmap *bm,sF32 angle,sF32 sx,sF32 sy,sInt tx,sInt ty,sInt border)
{
  sInt x,y;
  sU16 *s,*d;
  sU64 *mem;
	sInt xs,ys;
	sInt m00,m01,m10,m11,m20,m21;
  sF32 rotangle;

  SCRIPTVERIFY(bm);

// prepare


  mem = new sU64[bm->Size];
  d = (sU16 *)mem;
  s = (sU16 *)bm->Data;
	xs = bm->XSize;
	ys = bm->YSize;

// rotate

  SCRIPTVERIFY(sx!=0 && sy!=0)

  rotangle = angle*sPI2F;

  m00 = sFtol( sFCos(rotangle)*0x10000/sx);
  m01 = sFtol( sFSin(rotangle)*0x10000/sx);
  m10 = sFtol(-sFSin(rotangle)*0x10000/sy);
  m11 = sFtol( sFCos(rotangle)*0x10000/sy);
  m20 = sFtol( tx*xs - (xs*m00+ys*m10)/2);
  m21 = sFtol( ty*ys - (xs*m01+ys*m11)/2);
	border = border>>16;

  for(y=0;y<ys;y++)
  {
    for(x=0;x<xs;x++)
    {
			BilinearFilter((sU64*)d,(sU64*)s,xs,ys,x*m00+y*m10+m20,x*m01+y*m11+m21,border);
      d+=4;
    }
  }

  sCopyMem4((sU32 *)bm->Data,(sU32 *)mem,bm->Size*2);
  delete[] mem;

  return bm;
}

/****************************************************************************/

GenBitmap * __stdcall Bitmap_Distort(GenBitmap *bm,GenBitmap *bb,sInt dist,sInt border)
{
  sInt x,y;
  sU16 *t,*d;
	sInt xs,ys;
  sInt u,v;
  sInt bumpx,bumpy;

  SCRIPTVERIFY(bm);
  SCRIPTVERIFY(bb);
  SCRIPTVERIFY(bm->Size==bb->Size);

// prepare

  t = (sU16 *)bm->Data;   // vertauscht? mach sinn wegen tilerendering, aber tilerendering ist nicht mehr...
  d = (sU16 *)bb->Data;
	xs = bm->XSize;
	ys = bm->YSize;
  bumpx = (dist*xs)>>14;
  bumpy = (dist*ys)>>14;
	border = border>>16;

// rotate 

  for(y=0;y<xs;y++)
  {
    for(x=0;x<ys;x++)
    {
      u = ((x)<<16) + ((t[0]-0x4000)*bumpx);
      v = ((y)<<16) + ((t[1]-0x4000)*bumpy);
			BilinearFilter((sU64*)t,(sU64*)d,xs,ys,u,v,border);
      t+=4;
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

GenBitmap * __stdcall Bitmap_Normals(GenBitmap *bm,sInt dist,sInt mode)
{
  sInt shiftx,shifty;
  sInt xs,ys;
  sInt x,y;
  sU64 *mem;
  sU16 *sx,*sy,*s,*d;
  sInt vx,vy,vz;
  sF32 e;


  SCRIPTVERIFY(bm);

  mem = new sU64[bm->Size];
  d = (sU16 *)mem;
  sx = sy = s = (sU16*) bm->Data;
	xs = bm->XSize;
	ys = bm->YSize; 
  shiftx = sGetPower2(bm->XSize);
  shifty = sGetPower2(bm->YSize);
  
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

      if(mode&0x10000)
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
      if(mode&0x20000)
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

  sCopyMem4((sU32 *)bm->Data,(sU32 *)mem,bm->Size*2);
  delete[] mem;
  return bm;
}

/****************************************************************************/

#if !sINTRO_X

void BumpLight(GenBitmap *bm,GenBitmap *bb,sS32 *para);

GenBitmap * __stdcall Bitmap_Light(GenBitmap *bm,sInt subcode,sF32 px,sF32 py,sF32 pz,sF32 da,sF32 db,
                                   sInt4 diff,sInt4 ambi,sF32 outer,sF32 falloff,sF32 amp)
{
  return Bitmap_Bump(bm,0,subcode,px,py,pz,da,db,diff,ambi,outer,falloff,amp,ambi,0,0);
}


GenBitmap * __stdcall Bitmap_Bump(GenBitmap *bm,GenBitmap *bb,sInt subcode,sF32 px,sF32 py,sF32 pz,sF32 da,sF32 db,
                                  sInt4 _diff,sInt4 _ambi,sF32 outer,sF32 falloff,sF32 amp,
                                  sInt4 _spec,sF32 spow,sInt samp)
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

	xs = bm->XSize;
	ys = bm->YSize;

  SCRIPTVERIFY(outer!=0);

  subcode = subcode>>16;
  *(sU64 *)diff = GetColor64(&_diff.x);
  *(sU64 *)ambi = GetColor64(&_ambi.x);
  *(sU64 *)spec = GetColor64(&_spec.x);

  px = px*xs*2-(xs/2);
  py = py*ys*2-(ys/2);
  pz = pz*xs;
  da *= sPI2F;
  db *= sPIF;

  dx = dy = sFCos(db);
  dx *= sFSin(da);
  dy *= sFCos(da);
  dz = sFSin(db);


  s = d = (sU16 *)bm->Data;
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

  for(y=0;y<bm->XSize;y++)
  {
    for(x=0;x<bm->YSize;x++)
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
        hx*=e;
        hy*=e;
        hz*=e;
        sf = hx*nx+hy*ny+hz*nz;
        if(sf<0) sf=0;
        sf = sFPow(sf,spow);
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
      Fade64(*(sU64 *)d,*(sU64 *)buff,*(sU64 *)spec,sFtol(df*sf*samp));
      s+=4;
      d+=4;
    }
  }

  return bm;
}

#endif

/****************************************************************************/

GenBitmap * __stdcall Bitmap_Text(GenBitmap *bm,sInt x,sInt y,sInt width,sInt height,sInt4 col,sChar *text)
{
#if sINTRO
  sU32 *data;
  sInt i;
  sU64 col64;

  col64 = GetColor64(&col.x);
  data = sSystem->RenderFont(bm->XSize,bm->YSize,"arial",text,x*bm->XSize/0x10000,y*bm->YSize/0x10000,height*bm->XSize/0x10000,width*bm->YSize/0x10000);

  for(i=0;i<bm->Size;i++)
  {
    Fade64(bm->Data[i],bm->Data[i],col64,(data[i]&255)*(65536/255));
  }

  delete[] data;
  
#else
  sBitmap *tb;
  sHostFont *hf;
  sInt i;
  sInt xs,ys;
	sU64 color;
  sU32 *s;
  sU16 *d;
  sInt para[9];

  SCRIPTVERIFY(bm);

  sCopyMem(para,&x,4*9);
  

  xs = bm->XSize;
  ys = bm->YSize;
  tb = new sBitmap;
  tb->Init(xs,ys);
  sSetMem4(tb->Data,0,xs*ys);
  hf = new sHostFont;
  hf->Init("arial",ys*para[2]/0x10000,xs*para[3]/0x10000,0);
  hf->Print(tb,(sChar *)para[8],xs*(sInt)para[0]/0x10000,ys*(sInt)para[1]/0x10000);

	color = GetColor64(&col.x);
	s = tb->Data;
	d = (sU16 *)bm->Data;
	i = xs * ys;

	// not really optimized!
	static const sU64 mask1 = 0x4000400040004000;
	static const sU64 mask2 = 0x00ff00ff00ff00ff;
	__asm
	{
		emms;
		mov				esi, [s];
		mov				edi, [d];
		mov				ecx, [i];
		lea				esi, [esi + ecx * 4];
		lea				edi, [edi + ecx * 8];
		neg				ecx;
		movq			mm2, [mask1];
		pxor			mm3, mm3;
		movq			mm4, [color];
		movq			mm6, [mask2];

textlp:
		movd			mm0, [esi + ecx * 4];
		movq			mm1, [edi + ecx * 8];

		movq			mm7, mm2;
		punpcklbw	mm0, mm0;
		movq			mm5, mm4;
		punpcklbw	mm0, mm3;
		psubw			mm5, mm1;
		psllw			mm0, 7;
		paddw			mm0, mm6;
		pcmpgtw		mm7, mm0;
		pand			mm7, mm6;
		psubw			mm0, mm7;
		pmulhw		mm5, mm0;
		psllw			mm5, 1;
		paddw			mm5, mm1;
		movq			[edi + ecx * 8], mm5;

		inc				ecx;
		jnz				textlp;
		emms;
	}

  delete hf;
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
  seed &= 255;
  vx = (x>>16); tx=(x&0xffff)/65536.0f;
  vy = (y>>16); ty=(y&0xffff)/65536.0f;
  v00 = sPerlinPermute[((vx+0)&mask)+sPerlinPermute[((vy+0)&mask)^seed]];
  v01 = sPerlinPermute[((vx+1)&mask)+sPerlinPermute[((vy+0)&mask)^seed]];
  v10 = sPerlinPermute[((vx+0)&mask)+sPerlinPermute[((vy+1)&mask)^seed]];
  v11 = sPerlinPermute[((vx+1)&mask)+sPerlinPermute[((vy+1)&mask)^seed]];
  f00 = sPerlinRandom[v00][0]*(tx-0)+sPerlinRandom[v00][1]*(ty-0);
  f01 = sPerlinRandom[v01][0]*(tx-1)+sPerlinRandom[v01][1]*(ty-0);
  f10 = sPerlinRandom[v10][0]*(tx-0)+sPerlinRandom[v10][1]*(ty-1);
  f11 = sPerlinRandom[v11][0]*(tx-1)+sPerlinRandom[v11][1]*(ty-1);
  tx = tx*tx*(3-2*tx);
  ty = ty*ty*(3-2*ty);
  fa = f00+(f01-f00)*tx;
  fb = f10+(f11-f10)*tx;
  f = fa+(fb-fa)*ty;

  return f;
}

GenBitmap * __stdcall Bitmap_Perlin(sInt freq,sInt oct,sF32 fadeoff,sInt seed,sInt mode,sF32 amp,sF32 gamma,sInt4 col0,sInt4 col1)
{
  GenBitmap *bm;
  sInt i;
  sInt x,y,px,py;
  sInt shiftx,shifty;
  sU64 c0,c1;
  sInt val;
  sF32 n,nn,s;
  sU64 *tile;

  bm = new GenBitmap;
  bm->Init(BitmapXSize,BitmapYSize);

  freq = freq>>16;
  oct = oct>>16;
  seed = seed>>16;
  mode = mode>>16;
  c0 = GetColor64(&col0.x);
  c1 = GetColor64(&col1.x);

  shiftx = sGetPower2(bm->XSize);
  shifty = sGetPower2(bm->YSize);
  tile = bm->Data;

  for(y=0;y<bm->XSize;y++)
  {
    for(x=0;x<bm->YSize;x++)
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
      if(mode&1)
        n = sFPow(n*amp,gamma);
      else
        n = sFPow(n*amp*0.5f+0.5f,gamma);
      val = sRange7fff(n*0x8000)*2;
      Fade64(*tile,c0,c1,val);
      tile++;
    }
  }

  return bm;
}

/****************************************************************************/

GenBitmap * __stdcall Bitmap_Cell(sInt4 col0,sInt4 col1,sInt4 col2,sInt count,sInt seed,sF32 amp,sF32 gamma,sInt mode)
{
  GenBitmap *bm;
  sInt cells[256][3];
  sInt x,y,i,max,dist,best,best2,besti;
  sInt dx,dy,px,py;
  sInt shiftx,shifty;
  sF32 v0,v1;
  sInt val;
  sU64 c0,c1,cc,cb;
  sU64 *tile;


  bm = new GenBitmap;
  bm->Init(BitmapXSize,BitmapYSize);

  max = count>>16;
  SCRIPTVERIFY(max>=1 && max<=256);

  sSetRndSeed(seed>>16);
  for(i=0;i<max*3;i++)
    cells[0][i] = sGetRnd(0x4000);
 
  shiftx = 14-sGetPower2(bm->XSize);
  shifty = 14-sGetPower2(bm->YSize);
  mode = mode>>16;
  c0 = GetColor64(&col1.x);
  c1 = GetColor64(&col0.x);
  cb = GetColor64(&col2.x);
  tile = bm->Data;

  for(y=0;y<bm->XSize;y++)
  {
    for(x=0;x<bm->YSize;x++)
    {
      best = 0x8000*0x8000;
      best2 = 0x8000*0x8000;
      besti = 0;
      px = (x)<<(shiftx);
      py = (y)<<(shifty);
      for(i=0;i<max;i++)
      {
        dx = ((cells[i][0]-px)&0x3fff)-0x2000;
        dy = ((cells[i][1]-py)&0x3fff)-0x2000; 

        dist = dx*dx+dy*dy;
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

      v0 = sFSqrt(best)/16384.0f;
      if(mode&1)
      {
        v1 = sFSqrt(best2)/16384.0f;
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

GenBitmap * __stdcall Bitmap_Gradient(sInt4 col0,sInt4 col1,sInt pos,sF32 a,sF32 length)
{
	GenBitmap *bm;
  sU64 c0,c1;
	sInt c,cdx,cdy,x,y;
	sInt dx,dy;
	sF32 l;
  sU64 *tile;

  bm = new GenBitmap;
  bm->Init(BitmapXSize,BitmapYSize);

	c0 = GetColor64(&col0.x);
	c1 = GetColor64(&col1.x);

	l = 32768.0f / length;
	dx = sFtol(sFCos(a * sPI2) * l);
	dy = sFtol(sFSin(a * sPI2) * l);
	cdx = sMulShift(dx,bm->XSize);
	cdy = sMulShift(dy,bm->YSize)-bm->XSize*cdx;
  
  c = 0x4000-dx/2-dy/2-pos;

  tile = bm->Data;
	for(y=0;y<bm->XSize;y++)
	{
		for(x=0;x<bm->XSize;x++)
		{
			Fade64(*tile,c0,c1,sRange7fff(c)*2);
			c += cdx;
			tile++;
		}

		c += cdy;
	}

	return bm;
}

/****************************************************************************/
/****************************************************************************/


/****************************************************************************/
/***                                                                      ***/
/***   Generator                                                          ***/
/***                                                                      ***/
/****************************************************************************/

GenBitmap::GenBitmap()
{
  Data = 0;
#if !sINTRO_X
  XSize = 0;
  YSize = 0;
  Size = 0;
#endif
  Texture = sINVALID;
}

GenBitmap::~GenBitmap()
{
  if(Data) delete[] Data;
  if(Texture!=sINVALID) sSystem->RemTexture(Texture);
}

void GenBitmap::Copy(sObject *o)
{
  GenBitmap *ob;

  sVERIFY(o->GetClass()==sCID_GENBITMAP);

  ob = (GenBitmap *) o;
  sVERIFY(ob->Data);

  Texture = sINVALID;
  Init(ob->XSize,ob->YSize);
  sCopyMem4((sU32 *)Data,(sU32 *)ob->Data,Size*2);
}

void GenBitmap::Init(sInt x,sInt y)
{
#if !sINTRO_X
  XSize = x;
  YSize = y;
  Size = x*y;
#endif
  Data = new sU64[Size];
}

/****************************************************************************/
/****************************************************************************/
