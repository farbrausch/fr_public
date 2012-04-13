// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_types.hpp"
#if !sMOBILE
#include "_start.hpp"
#endif
#include <stdarg.h>
#if sLINK_INTMATH
#include "_intmath.hpp"
#endif

/****************************************************************************/

namespace Werkk3TexLib
{

/****************************************************************************/
/***                                                                      ***/
/***   Init/Exit                                                          ***/
/***                                                                      ***/
/****************************************************************************/

void InitIntMath();

void sInitTypes()
{
#if !sINTRO
  InitIntMath();
#endif
#if sLINK_INTMATH
  sInitIntMath2();
#endif
}

void sExitTypes()
{
}

/****************************************************************************/
/***                                                                      ***/
/***   Forwards                                                           ***/
/***                                                                      ***/
/****************************************************************************/

static sU32 sRandomSeed=0x74382381;

sU32 sGetRnd()
{
  sU32 i;

#if sMOBILE
  sU32 eax,ebx;
  eax = sRandomSeed;
  eax = eax*0x343fd+0x269ec3;
  ebx = eax;
  eax = eax*0x343fd+0x269ec3;
  sRandomSeed = eax;
  eax = (eax>>10)&0x0000ffff;
  ebx = (ebx<< 6)&0xffff0000;
  i = eax|ebx;
#else
  __asm
  {
    mov eax,sRandomSeed

    imul eax,eax,343fdh
    add eax,269ec3h
    mov ebx,eax

    imul eax,eax,343fdh
    add eax,269ec3h

    mov sRandomSeed,eax

    sar eax,10
    and eax,00000ffffh
    shl ebx,6
    and ebx,0ffff0000h
    or eax,ebx
    mov i,eax
  }
#endif
  return i;
}

/****************************************************************************/

sU32 sGetRnd(sU32 max)
{
  return sGetRnd()%max;
}

/****************************************************************************/

#if !sMOBILE
sF32 sFGetRnd()
{
  return ((sGetRnd()&0x3fffffff)*1.0f)/0x40000000;
}
#endif

/****************************************************************************/

#if !sMOBILE
sF32 sFGetRnd(sF32 max)
{
  return ((sGetRnd()&0x3fffffff)*max)/0x40000000;
}
#endif

/****************************************************************************/

void sSetRndSeed(sInt seed)
{
  sRandomSeed = seed+seed*17+seed*121+(seed*121/17);
  sGetRnd();  
  sRandomSeed ^= seed+seed*17+seed*121+(seed*121/17);
  sGetRnd();  
  sRandomSeed ^= seed+seed*17+seed*121+(seed*121/17);
  sGetRnd();  
  sRandomSeed ^= seed+seed*17+seed*121+(seed*121/17);
  sGetRnd();
}

/****************************************************************************/

sInt sMakePower2(sInt val)
{
  sInt p;

  p = 1;

  while(p<val)
    p = p*2;
  return p;
}

/****************************************************************************/

sInt sGetPower2(sInt val)
{
  sInt p;

  p = 1;

  while((1<<p)<val)
    p++;
  return p;
}

/****************************************************************************/

#if !sMOBILE
sU32 sDec3(sF32 x,sF32 y,sF32 z)
{
  sInt ix,iy,iz;
  ix = sRange<sInt>(x*511,511,-511)&0x3ff;
  iy = sRange<sInt>(y*511,511,-511)&0x3ff;
  iz = sRange<sInt>(z*511,511,-511)&0x3ff;
  return (ix)|(iy<<10)|(iz<<20);
}
#endif

/****************************************************************************/

#if !sMOBILE
void sHermite(sF32 *d,sF32 *p0,sF32 *p1,sF32 *p2,sF32 *p3,sInt count,sF32 fade,sF32 t,sF32 c,sF32 b,sBool ignoretime)
{
  sF32 f1,f2,f3,f4;
  sF32 t0,t1,t2,t3;
  sF32 a1,b1,a2,b2;
  sInt i;

  a1 = (1-t)*(1-c)*(1+b);
  b1 = (1-t)*(1+c)*(1-b);
  a2 = (1-t)*(1+c)*(1+b);
  b2 = (1-t)*(1-c)*(1-b);

  t1 = p1[-1];
  t2 = p2[-1];

  if(p0==0)
  {
    p0 = p2;
    a1 = -a1;
    t0 = t1+(t1-t2);
  }
  else
  {
    t0 = p0[-1];
  }
  if(p3==0)
  {
    p3 = p1;
    b2 = -b2;
    t3 = t2+(t2-t1);
  }
  else
  {
    t3 = p3[-1];
  }

  f1 =  2*fade*fade*fade - 3*fade*fade + 1;
  f2 = -2*fade*fade*fade + 3*fade*fade;
  f3 =    fade*fade*fade - 2*fade*fade + fade;
  f4 =    fade*fade*fade -   fade*fade;
 
  if(!ignoretime)
  {
    t0 = t1-t0;
    t1 = t2-t1;
    t2 = t3-t2;
    f3 = f3 * t1/(t1+t0);
    f4 = f4 * t1/(t1+t2);
  }
  else
  {
    f3 *= 0.5f;
    f4 *= 0.5f;
  }

  sVERIFY(count<=9);

  for(i=0;i<count;i++)
  {
    d[i] = f1 * p1[i]
         + f2 * p2[i]
         + f3 * (a1*(p1[i]-p0[i]) + b1*(p2[i]-p1[i]))
         + f4 * (a2*(p2[i]-p1[i]) + b2*(p3[i]-p2[i]));
  }
}


void sHermiteD(sF32 *d,sF32 *dd,sF32 *p0,sF32 *p1,sF32 *p2,sF32 *p3,sInt count,sF32 fade,sF32 t,sF32 c,sF32 b,sBool ignoretime)
{
  sF32 f1,f2,f3,f4;
  sF32 t0,t1,t2,t3;
  sF32 a1,b1,a2,b2;
  sF32 f1d,f2d,f3d,f4d;
  sInt i;

  a1 = (1-t)*(1-c)*(1+b);
  b1 = (1-t)*(1+c)*(1-b);
  a2 = (1-t)*(1+c)*(1+b);
  b2 = (1-t)*(1-c)*(1-b);

  t1 = p1[-1];
  t2 = p2[-1];

  if(p0==0)
  {
    p0 = p2;
    a1 = -a1;
    t0 = t1+(t1-t2);
  }
  else
  {
    t0 = p0[-1];
  }
  if(p3==0)
  {
    p3 = p1;
    b2 = -b2;
    t3 = t2+(t2-t1);
  }
  else
  {
    t3 = p3[-1];
  }

  f1 =  2*fade*fade*fade - 3*fade*fade + 1;
  f2 = -2*fade*fade*fade + 3*fade*fade;
  f3 =    fade*fade*fade - 2*fade*fade + fade;
  f4 =    fade*fade*fade -   fade*fade;

  f1d =  6*fade*fade - 6*fade;
  f2d = -6*fade*fade + 6*fade;
  f3d =  3*fade*fade - 4*fade + 1;
  f4d =  3*fade*fade - 2*fade;

  if(!ignoretime)
  {
    t0 = t1-t0;
    t1 = t2-t1;
    t2 = t3-t2;
    f3 = f3 * t1/(t1+t0);
    f4 = f4 * t1/(t1+t2);
    f3d = f3d * t1/(t1+t0);
    f4d = f4d * t1/(t1+t2);
  }
  else
  {
    f3 *= 0.5f;
    f4 *= 0.5f;
    f3d *= 0.5f;
    f4d *= 0.5f;
  }

  f1d *= 0.25f;
  f2d *= 0.25f;
  f3d *= 0.25f;
  f4d *= 0.25f;

  sVERIFY(count<=9);

  for(i=0;i<count;i++)
  {
    d[i] = f1 * p1[i]
         + f2 * p2[i]
         + f3 * (a1*(p1[i]-p0[i]) + b1*(p2[i]-p1[i]))
         + f4 * (a2*(p2[i]-p1[i]) + b2*(p3[i]-p2[i]));
    dd[i] = f1d * p1[i]
          + f2d * p2[i]
          + f3d * (a1*(p1[i]-p0[i]) + b1*(p2[i]-p1[i]))
          + f4d * (a2*(p2[i]-p1[i]) + b2*(p3[i]-p2[i]));
  }
}
#endif

/*
sInt sRangeInt(sInt a,sInt b,sInt c)
{
  return sRange(a,b,c);
}
sF32 sRangeF32(sF32 a,sF32 b,sF32 c)
{
  return sRange(a,b,c);
}
*/

#if !sMOBILE


/****************************************************************************/

sInt sQuadraticRoots(const sF32 *coeffs,sF32 *roots)
{
  if(sFAbs(coeffs[2]) > 1e-20f) // not degenerate
  {
    sF32 disc = sSquare(coeffs[1]) - 4.0f * coeffs[0] * coeffs[2];
    if(disc > 1e-20f) // positive, two roots
    {
      sF32 root = sFSqrt(disc);
      sF32 b = coeffs[1];
      sF32 q = (b > 0.0f) ? -0.5f * (b + root) : -0.5f * (b - root);

      roots[0] = q / coeffs[2];
      roots[1] = coeffs[0] / q;
      return 2;
    }
    else if(disc > -1e-20f) // zero, one root
    {
      roots[0] = -0.5f * coeffs[1] / coeffs[2];
      return 1;
    }
    else // negative, no root
      return 0;
  }
  else // degenerate, only a linear function
  {
    if(sFAbs(coeffs[1]) > 1e-20f)
    {
      roots[0] = -coeffs[0] / coeffs[1];
      return 1;
    }
    else
      return 0; // 0 or infinitely many solutions
  }
}

sBool sNormalFloat(sF32 value)
{
  const sU32 binval = (sU32 &) value;
  sInt exp = ((binval >> 23) & 0xff) - 127;

  if(exp == -127) // zero
    return (binval & 0x7fffffff) == 0;
  else
    return (exp >= -126 && exp <= 127);
}

#endif

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Int Math                                                           ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

#if !sINTRO

#define sSINUSFULL 0x4000               // 360 degrees for sAngle

#define FSIN_PTS 512
#define FSIN_MAX 0x10000
#define FSIN_STEP (FSIN_MAX/FSIN_PTS)

static sInt sSinus[sSINUSFULL/4+1];

/****************************************************************************/
/****************************************************************************/

void InitIntMath()
{
  sU32 t1,t2,val,v;
  sInt i;
 
  val=0x1921fc;                                 // Sinus
  t1=0;
  t2=0x80000000;
  for(i=1;i<sSINUSFULL/4;i++)
  {
    t1+=((sU64)t2*val)>>32;
    t2-=((sU64)t1*val)>>32;
    v=t1/(0x80000000/0x10000);
    sSinus[             i] =  v;
  }
  sSinus[sSINUSFULL/4*0]=0;
  sSinus[sSINUSFULL/4*1]=0x10000;
}

/****************************************************************************/

sInt sISinOld(sInt w)
{
  w = (w*sSINUSFULL>>16) & (sSINUSFULL-1);
  if(w>=sSINUSFULL/2)
  {
    w=w-sSINUSFULL/2;
    if(w<sSINUSFULL/4)
      return -sSinus[w];
    else
      return -sSinus[sSINUSFULL/2-w];
  }
  else
  {
    if(w<sSINUSFULL/4)
      return sSinus[w];
    else
      return sSinus[sSINUSFULL/2-w];
  }
}

/****************************************************************************/

sInt sICosOld(sInt w)
{
  return sISinOld(w+0x4000);
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Memory                                                             ***/
/***                                                                      ***/
/****************************************************************************/

void sCopyMemFast(void *d,const void *s,sInt c)
{
#if !sINTRO && !sMOBILE
  if(sSystem->CpuMask & sCPU_MMX2)
  {
    __asm
    {
      mov     esi, [s];
      mov     edi, [d];
      mov     ecx, [c];
      shr     ecx, 6;
      jz      tail;

copy:
      movq    mm0, [esi+ 0];
      movq    mm1, [esi+ 8];
      movq    mm2, [esi+16];
      movq    mm3, [esi+24];
      movq    mm4, [esi+32];
      movq    mm5, [esi+40];
      movq    mm6, [esi+48];
      movq    mm7, [esi+56];

      movntq  [edi+ 0], mm0;
      movntq  [edi+ 8], mm1;
      movntq  [edi+16], mm2;
      movntq  [edi+24], mm3;
      movntq  [edi+32], mm4;
      movntq  [edi+40], mm5;
      movntq  [edi+48], mm6;
      movntq  [edi+56], mm7;

      add     esi, 64;
      add     edi, 64;
      dec     ecx;
      jnz     copy;

      emms;

tail:
      mov     ecx, [c];
      and     ecx, 63;
      jz      end;
      rep     movsb;

end:
    }
  }
  else
#endif
    sCopyMem(d,s,c);
}

void sCopyMem4(sU32 *d,const sU32 *s,sInt c)
{
  sCopyMemFast(d,s,c*4);
  /*__asm
  {
    mov esi,[s]
    mov edi,[d]
    mov ecx,[c]
    rep movsd
  }*/
}

#if !sINTRO
void sCopyMem8(sU64 *d,const sU64 *s,sInt c)  
{
  sCopyMemFast(d,s,c*8);
  /*__asm
  {
    mov esi,[s]
    mov edi,[d]
    mov ecx,[c]
    add ecx,ecx
    rep movsd
  }*/
}
#endif

/****************************************************************************/
/***                                                                      ***/
/***   String                                                             ***/
/***                                                                      ***/
/****************************************************************************/

sChar *sDupString(sChar *s,sInt minsize)
{
  sInt size;
  sChar *d;

  if(s==0)
    return 0;

  size = sGetStringLen(s)+1;
  if(minsize>size)
    size = minsize;

  d = new sChar[size];
  sCopyString(d,s,size);
  return d;
}


void sCopyString(sChar *d,const sChar *s,sInt size)
{
  while(size>0 && *s)
  {
    size--;
    *d++ = *s++;
  }
  *d = 0;
}

/****************************************************************************/

void sAppendString(sChar *d,const sChar *s,sInt size)
{
  while(size>0 && *d)
  {
    size--;
    d++;
  }
  while(size>0 && *s)
  {
    size--;
    *d++ = *s++;
  }
  *d = 0;
}

/****************************************************************************/

void sParentString(sChar *path)
{
  sChar *s,*e;

  e = path;
  s = path;
  while(s[0] && s[1])
  {
    if(*s=='/' && (s!=path || s[-1]!=':'))
      e = s+1;
    s++;
  }

  *e = 0;
}

sChar *sFileFromPathString(sChar *path)
{
  sChar *s;
  s = path;
  while(*path)
  {
    if(*path=='\\' || *path=='/')
      s = path+1;
    path++;
  }
  return s;
}

sChar *sFileExtensionString(sChar *path)
{
  sChar *s = 0;
  while(*path)
  {
    if(*path=='.')
      s = path;
    path++;
  }
  return s;
}

/****************************************************************************/

sInt sCmpString(const sChar *a,const sChar *b)
{
  sInt aa,bb;
  do
  {
    aa = *a++;
    bb = *b++;
  }
  while(aa!=0 && aa==bb);
  return sSign(aa-bb);
}

sInt sCmpStringI(const sChar *a,const sChar *b)
{
  sInt aa,bb;
  do
  {
    aa = *a++;
    bb = *b++;
    if(aa>='A' && aa<='Z') aa=aa-'A'+'a';
    if(bb>='A' && bb<='Z') bb=bb-'A'+'a';
  }
  while(aa!=0 && aa==bb);
  return sSign(aa-bb);
}

sInt sCmpMemI(const sChar *a,const sChar *b,sInt len)
{
  sInt aa,bb;
  sInt i;
  for(i=0;i<len;i++)
  {
    aa = *a++;
    bb = *b++;
    if(aa>='A' && aa<='Z') aa=aa-'A'+'a';
    if(bb>='A' && bb<='Z') bb=bb-'A'+'a';
    if(aa!=bb)
      return sSign(aa-bb);
  }
  return 0;
}

const sChar *sFindString(const sChar *s,const sChar *f)
{
  sInt len,i,slen;

  len = sGetStringLen(f);
  slen = sGetStringLen(s);
  for(i=0;i<=slen-len;i++)
  {
    if(sCmpMem((const sPtr)(s+i),(const sPtr)f,len)==0)
      return s+i;
  }
  return 0;
}
/*
const sChar *sFindStringI(const sChar *s,const sChar *f)
{
  sInt len,i,slen;

  len = sGetStringLen(f);
  slen = sGetStringLen(s);
  for(i=0;i<=slen-len;i++)
  {
    if(sCmpMemI(s+i,f,len)==0)
      return s+i;
  }
  return 0;
}
*/

sInt sAtoi(const sChar *s)
{
  sInt val;
  val = 0;
  while(*s>='0' && *s<='9')
    val = val * 10 + (*s++) - '0';
  return val;
}

/****************************************************************************/
/****************************************************************************/

sInt sScanInt(const sChar *&scan)
{
  sInt val; 
  sInt sign;

  sign = 1;
  val = 0;

  if(*scan=='\'')
  {
    if(scan[1]!=0 && scan[2]=='\'')
    {
      val = scan[1];
      scan+=3;
      return val;
    }
    return 0;
  }

  if(*scan=='+')
    scan++;
  else if(*scan=='-')
  {
    sign = -1;
    scan++;
  }

  if(*scan=='$' || *scan=='#')
  {
    scan++;
    for(;;)
    {
      if(*scan>='0' && *scan<='9')
        val = val*16+(*scan)-'0';
      else if(*scan>='a' && *scan<='f')
        val = val*16+(*scan)-'a'+10;
      else if(*scan>='A' && *scan<='F')
        val = val*16+(*scan)-'A'+10;
      else
        break;
      scan++;
    }
  }
  else
  {
    while(*scan>='0' && *scan<='9')
      val = val*10+(*scan++)-'0';
  }

  return val*sign;
}


sInt sScanHex(const sChar *&scan)
{
  sInt val; 
  sInt sign;

  sign = 1;
  val = 0;

  if(*scan=='+')
    scan++;
  else if(*scan=='-')
  {
    sign = -1;
    scan++;
  }

  for(;;)
  {
    if(*scan>='0' && *scan<='9')
      val = val*16+(*scan)-'0';
    else if(*scan>='a' && *scan<='f')
      val = val*16+(*scan)-'a'+10;
    else if(*scan>='A' && *scan<='F')
      val = val*16+(*scan)-'A'+10;
    else
      break;
    scan++;
  }

  return val*sign;
}

/****************************************************************************/

#if !sMOBILE

sF32 sScanFloat(const sChar *&scan)
{
  sF64 val;
  sInt sign;
  sF64 dec;

  sign = 1;
  val = 0;

  if(*scan=='+')
    scan++;
  else if(*scan=='-')
  {
    sign = -1;
    scan++;
  }

  while(*scan>='0' && *scan<='9')
    val = val*10+(*scan++)-'0';
  if(*scan=='.')
  {
    scan++;
    dec = 1.0;
    while(*scan>='0' && *scan<='9')
    {
      dec = dec/10.0;
      val = val + ((*scan++)-'0')*dec;
    }
  }

  return val*sign;
}

#endif

/****************************************************************************/

sBool sScanString(const sChar *&scan,sChar *buffer,sInt size)
{
  if(*scan!='"')
    return sFALSE;
  scan++;
  while(*scan!='"' && *scan!=0 && *scan!='\r' && *scan!='\n' && size>1)
  {
    *buffer++ = *scan++;
    size--;
  }
  *buffer++ = 0;
  if(*scan=='"')
  {
    scan++;
    return sTRUE;
  }
  else
    return sFALSE;
}

/****************************************************************************/

sBool sScanName(const sChar *&scan,sChar *buffer,sInt size)
{
  sBool done;

  done = sFALSE;

  if(!((*scan>='a' && *scan<='z') || 
       (*scan>='A' && *scan<='Z') || *scan=='_' )) 
    return sFALSE;

  *buffer++ = *scan++;
  size--;

  while(size>1)
  {
    if(!((*scan>='a' && *scan<='z') || 
         (*scan>='A' && *scan<='Z') || 
         (*scan>='0' && *scan<='9') || *scan=='_' )) 
    {
      *buffer++=0;
      return sTRUE;
    }
    *buffer++ = *scan++;
    size--;
  }

  *buffer++=0;
  return sFALSE;
}

/****************************************************************************/

void sScanSpace(const sChar *&scan)
{
  while(*scan==' ' || *scan=='\t' || *scan=='\r' || *scan=='\n')
    scan++;
}

/****************************************************************************/

sBool sScanCycle(const sChar *s,sInt index,sInt &start,sInt &len)
{
  start = 0;
  len = 0;
  while(index>0)
  {
    while(s[start]!='|' && s[start]!=0)
      start++;
    if(s[start]!='|')
      return sFALSE;
    start++;
    index--;
  }
  while(s[start+len]!='|' && s[start+len]!=0)
    len++;
  return sTRUE;
}

/****************************************************************************/
/***                                                                      ***/
/***   Formatted Printing                                                 ***/
/***                                                                      ***/
/****************************************************************************/

#if !sMOBILE
extern "C" char * __cdecl _fcvt( double value, int count, int *dec, int *sign );
extern "C" char * __cdecl _ecvt( double value, int count, int *dec, int *sign );
#endif

sBool sFormatString(sChar *d,sInt left,const sChar *s,const sChar **fp)
{
  sInt c;
  sInt field0;
  sInt field1;
  sInt minus;
  sInt null;
  sInt len;
  sChar buffer[64];
  sChar *string;
  sInt val;
  sInt arg;
  sInt sign;
  sInt i;
#if !sMOBILE
  sF64 fval;
#endif
  static sChar hex[17] = sTXT("0123456789abcdef");
  static sChar HEX[17] = sTXT("0123456789ABCDEF");

  arg = 0;
  left--;

  c = *s++;
  while(c)
  {
    if(c=='%')
    {
      c = *s++;

      minus = 0;
      null = 0;
      field0 = 0;
      field1 = 4;

      if(c=='-')
      {
        minus = 1;
        c = *s++;
      }
      if(c=='0')
      {
        null = 1;
      }
      while(c>='0' && c<='9')
      {
        field0 = field0*10 + c - '0';
        c = *s++;
      }
      if(c=='.')
      {
        field1=0;
        c = *s++;
        while(c>='0' && c<='9')
        {
          field1 = field1*10 + c - '0';
          c = *s++;
        }
      }

      if(c=='%')
      {
        c = *s++;
        if(left>0)
        {
          *d++ = '%';
          left--;
        }
      }
      else if(c=='d' || c=='x' || c=='X' || c=='i' || c=='f' || c=='e')
      {
        len = 0;
        sign = 0;
        if(c=='f' || c=='e')
        {
#if !sMOBILE
          fval = sVARARGF(fp,arg);arg+=2;
#else
          arg+=2;
#endif
        }
        else
        {
          val = sVARARG(fp,arg);arg++;
        }
        if(c=='f')        // this is preliminary!!!!!!!
        {
#if sINTRO || sMOBILE
          string = sTXT("???");
#else
          if(fval<0)
            field1++;
          string = _fcvt(fval,field1,&i,&sign);
#endif
          if(i<0)
          {
            buffer[len++]='.';
            while(i<=-1)
            {
              i++;
              buffer[len++]='0';
            }
            i=-1;
          }
          while(*string)
          {
            if(i==0)
              buffer[len++]='.';
            i--;
            buffer[len++]=*string++;
          }
        }
        else if(c=='e')
        {
#if sINTRO || sMOBILE
          string = sTXT("???");
#else
          if(fval<0)
            field1++;
          string = _ecvt(fval,field1,&i,&sign);
#endif
          if(*string)
          {
            buffer[len++] = *string++;
            buffer[len++] = '.';
            while(*string)
              buffer[len++] = *string++;
            
            buffer[len++] = 'e';
            if(i<0)
            {
              buffer[len++] = '-';
              i = -i;
            }
            else
              buffer[len++] = '+';

            if(i>=100)
            {
              buffer[len++] = 'X';
              buffer[len++] = 'X';
            }
            else
            {
              buffer[len++] = (i/10)%10 + '0';
              buffer[len++] = i%10 + '0';
            }
          }
        }
        else if(c=='d' || c=='i')
        {          
          if(val==0x80000000)
          {
            val = val/10;
            buffer[len++] = '8';
          }
          if(val<0)
          {
            sign = 1;
            val = -val;
          }
          do
          {
            buffer[len++] = val%10+'0';
            val = val/10;
          }
          while(val!=0);
          if(sign)
            len++;

        }
        else if(c=='x' || c=='X')
        {
          do
          {
            if(c=='x')
              buffer[len] = hex[val&15];
            else
              buffer[len] = HEX[val&15];
            val = (val>>4)&0x0fffffff;
            len++;
          }
          while(val!=0);
        }

        if(!minus && !null)
        {
          while(field0>len && left>0)
          {
            *d++ = ' ';
            left--;
            field0--;
          }
        }
        if(sign && left>0)
        {
          *d++ = '-';
          left--;
          field0--;
          len--;
        }
        if(!minus && null)
        {
          while(field0>len && left>0)
          {
            *d++ = '0';
            left--;
            field0--;
          }
        }
        i = 0;
        while(len>0 && left>0)
        {
          len--;
          if(c=='f' || c=='e')
            *d++ = buffer[i++];
          else
            *d++ = buffer[len];
          left--;
          field0--;
        }
        if(!minus)
        {
          while(field0>len && left>0)
          {
            *d++ = ' ';
            left--;
            field0--;
          }
        }
        c = *s++;
      }
      else if(c=='c')
      {
        val = (sInt)sVARARG(fp,arg);arg++;
        if(left>0)
        {
          *d++ = val;
          left--;
        }
        c = *s++;
      }
      else if(c=='s')
      {
        string = (sChar * )(sDInt)sVARARG(fp,arg);arg++;
        len = sGetStringLen(string);
        if(field0<=len)
          field0=len;
        if(!minus)
        {
          while(field0>len && left>0)
          {
            *d++ = ' ';
            left--;
            field0--;
          }
        }
        while(*string && left>0)
        {
          *d++=*string++;
          left--;
        }
        if(minus)
        {
          while(field0>len && left>0)
          {
            *d++ = ' ';
            left--;
            field0--;
          }
        }
        c = *s++;
      }
      else if(c=='h' || c=='H') 
      {
        val = sVARARG(fp,arg);arg++;
        if(c=='H')
        {
          if(sAbs(val)>=0x00010000)
          {
            if(val>0x80000000)             
              val |= 0x000000ff;
            else
              val &= 0xffffff00;
          }
        }
        *d++ = '0';
        *d++ = 'x';
        *d++ = hex[(val>>28)&15];
        *d++ = hex[(val>>24)&15];
        *d++ = hex[(val>>20)&15];
        *d++ = hex[(val>>16)&15];
        *d++ = '.';
        *d++ = hex[(val>>12)&15];
        *d++ = hex[(val>> 8)&15];
        *d++ = hex[(val>> 4)&15];
        *d++ = hex[(val    )&15];
        c = *s++;
      }
      else if(c!=0)
      {
        c = *s++;
      }
    }
    else
    {
      if(left>0)
      {
        *d++ = c;
        left--;
      }
      c = *s++;
    }
  }
  *d=0;

  return left>0;  // actually, if text fit's exactly, a false truncation error is reported!
}


/****************************************************************************/

void __cdecl sSPrintF(sChar *buffer,sInt size,const sChar *format,...)
{
  sFormatString(buffer,size,format,&format);
}

/****************************************************************************/
/***                                                                      ***/
/***   Memory Stack Allocator                                             ***/
/***                                                                      ***/
/****************************************************************************/

#if !sMOBILE

void sMemStack::Init()
{
  Size = 0;
  Used = 0;
  Size = 0;
  Delete = sFALSE;
}

void sMemStack::Init(sInt size)
{
  Size = size;
  Used = 0;
  Mem = new sU8[Size];
  Delete = sTRUE;
}

void sMemStack::Init(sInt size,sU8 *mem)
{
  Size = size;
  Used = 0;
  Mem = mem;
  Delete = sFALSE;
}

void sMemStack::Exit()
{
  if(Delete)
    delete[] Mem;
  Init();
}

void sMemStack::Flush()
{
  Used = 0;
}

sU8 *sMemStack::Alloc(sInt size)
{
  sU8 *r;
  r = Mem+Used;
  Used += size;
  sVERIFY(Used<=Size);
  return r;
}

void sGrowableMemStack::AddNewBlock()
{
  Block *b = new Block;
  b->Mem = new sU8[BlockSize];
  b->Used = 0;
  b->Next = Blocks;
  Blocks = b;
}

void sGrowableMemStack::FreeAllBlocks()
{
  Block *b,*n;

  for(b=Blocks;b;b=n)
  {
    n = b->Next;
    delete[] b->Mem;
    delete b;
  }

  Blocks = 0;
}

void sGrowableMemStack::Init(sInt size)
{
  Blocks = 0;
  BlockSize = size;
  TotalUsed = 0;

  AddNewBlock();
}

void sGrowableMemStack::Exit()
{
  FreeAllBlocks();
}

void sGrowableMemStack::Flush()
{
  // do we need to resize?
  if(TotalUsed > BlockSize)
  {
    FreeAllBlocks();
    BlockSize = TotalUsed + (TotalUsed >> 3);
    AddNewBlock();
  }
  else
    Blocks->Used = 0;

  TotalUsed = 0;
}

sU8 *sGrowableMemStack::Alloc(sInt size)
{
  if(Blocks->Used + size <= BlockSize)
  {
    sU8 *r = Blocks->Mem + Blocks->Used;
    Blocks->Used += size;
    TotalUsed += size;
    return r;
  }
  else
  {
    if(size > BlockSize)
      BlockSize = size;
    
    AddNewBlock();
    Blocks->Used = size;
    TotalUsed += size;

    return Blocks->Mem;
  }
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Perlin Noise Util                                                  ***/
/***                                                                      ***/
/****************************************************************************/

#if !sMOBILE

sVector sPerlinGradient3D[16];
sF32 sPerlinRandom[256][2];
sU8 sPerlinPermute[512];

/****************************************************************************/

static const sChar GTable[16*4] =
{
  1,1,0,0, 2,1,0,0, 1,2,0,0, 2,2,0,0,
  1,0,1,0, 2,0,1,0, 1,0,2,0, 2,0,2,0,
  0,1,1,0, 0,2,1,0, 0,1,2,0, 0,2,2,0,
  1,1,0,0, 2,1,0,0, 0,2,1,0, 0,2,2,0
};
static const sF32 GValue[3] = { 0.0f, 1.0, -1.0f };

void sInitPerlin()
{
  sInt i,j,x,y;

  sSetRndSeed(1);

	// 3d gradients
	for(i=0;i<64;i++)
    (&sPerlinGradient3D[0].x)[i] = GValue[GTable[i]];

  // permutation
	for(i=0;i<256;i++)
	{
		sPerlinRandom[i][0]=sGetRnd(0x10000);
		sPerlinPermute[i]=i;
	}

	for(i=0;i<255;i++)
	{
    for(j=i+1;j<256;j++)
    {
		  if(sPerlinRandom[i][0]>sPerlinRandom[j][0])
		  {
			  sSwap(sPerlinRandom[i][0],sPerlinRandom[j][0]);
			  sSwap(sPerlinPermute[i],sPerlinPermute[j]);
		  }
    }
	}
  sCopyMem(sPerlinPermute+256,sPerlinPermute,256);

  // random
  for(i=0;i<256;)
  {
    x = sGetRnd(0x10000)-0x8000;
    y = sGetRnd(0x10000)-0x8000;
    if(x*x+y*y<0x8000*0x8000)
    {
      sPerlinRandom[i][0] = x/32768.0f;
      sPerlinRandom[i][1] = y/32768.0f;
      i++;
    }
  }
}


#define ix is[0]
#define iy is[1]
#define iz is[2]
#define P sPerlinPermute
#define G sPerlinGradient3D

void sPerlin3D(const sVector &pos,sVector &out)
{
  sVector t0,t1,t2;
  sF32 fs[3];
  sInt is[3];

  for(sInt j=0;j<3;j++)
  {
    is[j] = sFtol(pos[j]-0.5f);   // integer coordinate
    fs[j] = pos[j] - is[j];  // fractional part
    is[j] &= 255;           // integer grid wraps round 256
    fs[j] = sRange<sF32>(fs[j],1,0);
    fs[j] = fs[j]*fs[j]*fs[j]*(10.0f+fs[j]*(6.0f*fs[j]-15.0f));
  }

	// trilinear interpolation of grid points
	t0.Lin3(G[P[P[P[ix]+iy  ]+iz  ]&15],G[P[P[P[ix+1]+iy  ]+iz  ]&15],fs[0]);
	t1.Lin3(G[P[P[P[ix]+iy+1]+iz  ]&15],G[P[P[P[ix+1]+iy+1]+iz  ]&15],fs[0]);
	t0.Lin3(t0,t1,fs[1]);
	t1.Lin3(G[P[P[P[ix]+iy  ]+iz+1]&15],G[P[P[P[ix+1]+iy  ]+iz+1]&15],fs[0]);
	t2.Lin3(G[P[P[P[ix]+iy+1]+iz+1]&15],G[P[P[P[ix+1]+iy+1]+iz+1]&15],fs[0]);
	t1.Lin3(t1,t2,fs[1]);
	t0.Lin3(t0,t1,fs[2]);

  out = t0;
}
#undef ix
#undef iy
#undef iz
#undef P
#undef G

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Compact value encodings                                            ***/
/***                                                                      ***/
/****************************************************************************/

#if !sMOBILE

void sWriteShort(sU8 *&data,sInt value)
{
  sVERIFY(value >= 0 && value <= 32767);

  *data++ = (value & 127) | (value>127 ? 128 : 0);
  if(value>127)
    *data++ = value >> 7;
}

sInt sReadShort(const sU8 *&data)
{
  sInt v;

  v = *data++;
  if(v & 128)
    v = (v & 127) | (*data++ << 7);

  return v;
}

void sWriteX16(sU8 *&data,sF32 value)
{
  sInt v,vn;

  v = value * 4096.0f;
  vn = (v < -32768) ? -32768 : (v > 32767) ? 32767 : v;
  *((sS16 *) data) = vn;
  data += 2;
}

sF32 sReadX16(const sU8 *&data)
{
  sInt v;

  v = *((sS16 *) data);
  data += 2;
  return v / 4096.0f;
}

static sInt doShift(sU32 v,sInt shift)
{
  while(shift>0)
    v<<=1,shift--;

  while(shift<0)
    v>>=1,shift++;

  return v;
}

void sWriteF16(sU8 *&data,sF32 v)
{
  sU32 value;
  sU16 dest;
  sInt esrc,edst;

  value = *(sU32 *) &v;
  esrc = ((value>>23) & 255) - 128;
  edst = sRange(esrc,15,-16);

  if(edst == -16) // very near zero or zero
    *data++ = 0x00;
  else if(sFAbs(v - 1.0f) < 1.0f/1024.0f) // very near one
    *data++ = 0x80;
  else if(sFAbs(v - 0.5f) < 0.5f/1024.0f) // very near 0.5
    *data++ = 0x01;
  else if(sFAbs(v - 0.25f) < 0.25f/1024.0f) // very near 0.25
    *data++ = 0x81;
  else
  {
    dest = ((value>>16) & 32768) // sign
      | ((edst+16)<<10)
      | (doShift(value>>13,edst-esrc) & 1023);
    sVERIFY((dest >> 8) != 0x00 && (dest >> 8) != 0x01 && (dest >> 8) != 0x81);

    *data++ = dest >> 8;
    *data++ = dest & 0xff;
  }
}

sF32 sReadF16(const sU8 *&data)
{
  sInt v;
  sU32 vd;

  v = *data++;
  if(v == 0x00) // zero
    return 0.0f;
  else if(v == 0x80) // one
    return 1.0f;
  else if(v == 0x01) // 0.5
    return 0.5f;
  else if(v == 0x81) // minus one
    return 0.25f;
  else
  {
    v = (v << 8) | *data++;
    vd = (v & 32768) << 16 // sign
      | ((((v >> 10) & 31) + 128 - 16) << 23)
      | ((v & 1023) << 13);

    return *(sF32 *) &vd;
  }
}

void sWriteF24(sU8 *&data,sF32 value)
{
  sU32 fltv,exp;

  if(value == 0.0f)
    *data++ = 0;
  else if(value == 1.0f)
    *data++ = 1;
  else if(value == -1.0f)
    *data++ = 0xff;
  else
  {
    fltv = *((sU32 *) &value);
    exp = (fltv >> 23) & 0xff;
    if(exp<=1)
      *data++ = 0;
    else
    {
      sVERIFY(exp != 0xff);
    
      // order of bytes: exponent, mantissa 2, sign+mantissa 1
      *data++ = exp;
      *data++ = fltv >> 8;
      *data++ = ((fltv >> 24) & 128) | ((fltv >> 16) & 127);
    }
  }
}

sF32 sReadF24(const sU8 *&data)
{
  sU32 first,second,full;

  first = *data++;
  if(first == 0)
    return 0.0f;
  else if(first == 1)
    return 1.0f;
  else if(first == 0xff)
    return -1.0f;
  else
  {
    second = *((sU16 *) data); data += 2;
    full = (first << 23) | ((second & 32768) << 16) | ((second & 32767) << 8);
    return *((sF32 *) &full);
  }
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   File Reading and Writing                                           ***/
/***                                                                      ***/
/****************************************************************************/

void sWriteString(sU32 *&data,sChar *str)
{
  sInt size;

  size = sGetStringLen(str);
  *data++ = size;
  data[size/4] = 0;

  sCopyMem(data,str,size);
  data+=(size+3)/4;
}

sBool sReadString(sU32 *&data,sChar *str,sInt max)
{
  sInt size;

  size = *data++;
  if(size+1>max)
  {
    str[0] = 0;
    return sFALSE;
  }
  sCopyMem(str,data,size);
  str[size] = 0;
  data+=(size+3)/4;

  return sTRUE;
}

sU32 *sWriteBegin(sU32 *&data,sU32 cid,sInt version)
{
  *data++ = sMAGIC_BEGIN;
  *data++ = cid;
  *data++ = version;
  *data++ = 0;
  return data-1;
}

void sWriteEnd(sU32 *&data,sU32 *header)
{
  if(header)
    *header = data-(header+1);
  *data++ = sMAGIC_END;
}

sInt sReadBegin(sU32 *&data,sU32 cid)
{
  sInt version;
  sInt size;

  if(*data++!=sMAGIC_BEGIN) return 0;
  if(*data++!=cid) return 0;
  version = *data++;
  size = *data++;
  if(data[size]!=sMAGIC_END) return 0;
  return version;
}

sBool sReadEnd(sU32 *&data)
{
  if(*data++!=sMAGIC_END) return 0;
  return sTRUE;
}

/****************************************************************************/
/***                                                                      ***/
/***   Debugging                                                          ***/
/***                                                                      ***/
/****************************************************************************/

#if sMOBILE

void sTerminate(sChar *buffer);
void sOutputDebugString(sChar *buffer);

void sVerifyFalse(const sChar *file,sInt line)
{
  sFatal(sTXT("%s(%d) : assertion"),file,line);
}

void __cdecl sFatal(const sChar *format,...)
{
  static sChar buffer[1024];

//  sFatality = 1;
  sFormatString(buffer,sCOUNTOF(buffer),format,&format);

  sOutputDebugString(buffer);
  sOutputDebugString(sTXT("\n"));

  sTerminate(buffer);
}

void sDPrint(sChar *text)
{
  sOutputDebugString(text);
}

void __cdecl sDPrintF(const sChar *format,...)
{
  sChar buffer[1024];

  sFormatString(buffer,sCOUNTOF(buffer),format,&format);

  sOutputDebugString(buffer);
}  


/****************************************************************************/
#else
/****************************************************************************/

void sVerifyFalse(const sChar *file,sInt line)
{
#if sDEBUG
  __asm int 3;
#endif
}

/****************************************************************************/

#endif

/****************************************************************************/
/***                                                                       ***/
/***   Simple Structs                                                     ***/
/***                                                                      ***/
/****************************************************************************/

#pragma lekktor(off)

#if !sMOBILE
void sRect::Init(const struct sFRect &r) 
{
  x0=sFtol(r.x0); 
  y0=sFtol(r.y0);
  x1=sFtol(r.x1); 
  y1=sFtol(r.y1);
}
#endif

sBool sRect::Hit(sInt x,sInt y)
{
  return x>=x0 && x<x1 && y>=y0 && y<y1;
}

sBool sRect::Hit(const sRect &rin)
{
  sRect r;
  r = *this;
  r.And(rin);
  return r.x0<r.x1 && r.y0<r.y1;
}

sBool sRect::Inside(const sRect &r)
{
  if(x0  < r.x0) return sFALSE;
  if(y0  < r.y0) return sFALSE;
  if(x1  > r.x1) return sFALSE;
  if(y1  > r.y1) return sFALSE;
  return sTRUE;
}

void sRect::And(const sRect &r)
{
  if(r.x0 > x0) x0 = r.x0;
  if(r.y0 > y0) y0 = r.y0;
  if(r.x1 < x1) x1 = r.x1;
  if(r.y1 < y1) y1 = r.y1;
}

void sRect::Or(const sRect &r)
{
  if(r.x0 < x0) x0 = r.x0;
  if(r.y0 < y0) y0 = r.y0;
  if(r.x1 > x1) x1 = r.x1;
  if(r.y1 > y1) y1 = r.y1;
}

void sRect::Sort()
{
  if(x0>x1) sSwap(x0,x1);
  if(y0>y1) sSwap(y0,y1);
}

void sRect::Extend(sInt i)
{
  x0-=i;
  y0-=i;
  x1+=i;
  y1+=i;
}

/****************************************************************************/
/****************************************************************************/

#if !sMOBILE

void sFRect::Init(const struct sRect &r) 
{
  x0=r.x0; 
  y0=r.y0; 
  x1=r.x1;
  y1=r.y1;
}

sBool sFRect::Hit(sF32 x,sF32 y)
{
  return x>=x0 && x<x1 && y>=y0 && y<y1;
}

sBool sFRect::Hit(const sFRect &rin)
{
  sFRect r;
  r = *this;
  r.And(rin);
  return r.x0<r.x1 && r.y0<r.y1;
}

void sFRect::And(const sFRect &r)
{
  if(r.x0 > x0) x0 = r.x0;
  if(r.y0 > y0) y0 = r.y0;
  if(r.x1 < x1) x1 = r.x1;
  if(r.y1 < y1) y1 = r.y1;
}

void sFRect::Or(const sFRect &r)
{
  if(r.x0 < x0) x0 = r.x0;
  if(r.y0 < y0) y0 = r.y0;
  if(r.x1 > x1) x1 = r.x1;
  if(r.y1 > y1) y1 = r.y1;
}

void sFRect::Sort()
{
  if(x0>x1) sSwap(x0,x1);
  if(y0>y1) sSwap(y0,y1);
}

void sFRect::Extend(sF32 i)
{
  x0-=i;
  y0-=i;
  x1+=i;
  y1+=i;
}

#endif

/****************************************************************************/

#if !sINTRO

void sRandom::Init()
{
  Seed(0);
}

void sRandom::Seed(sInt seed)
{
  kern = seed+seed*17+seed*121+(seed*121/17);
  Int32();  
  kern ^= seed+seed*17+seed*121+(seed*121/17);
  Int32();  
  kern ^= seed+seed*17+seed*121+(seed*121/17);
  Int32();  
  kern ^= seed+seed*17+seed*121+(seed*121/17);
  Int32();  
}

sInt sRandom::Int16()
{
  sU32 r0;

  r0 = kern*0x343fd+0x269ec3;
  kern = r0;
  r0 = (r0>>10)&0x0000ffff;
  return r0;
}

sU32 sRandom::Int32()
{
  sU32 r0,r1;
  r0 = kern*0x343fd+0x269ec3;
  r1 = r0*0x343fd+0x269ec3;
  kern = r1;
  r1 = (r1>>10)&0x0000ffff;
  r0 = (r0<< 6)&0xffff0000;

  return r1|r0;
}

sInt sRandom::Int(sInt max)
{
  if(max<0x4000)
    return (Int16()*max)>>16;
  else
    return sInt((sU64(Int32())*max)>>32);
}

sF32 sRandom::Float(sF32 max)
{
  return Int32()*max/(65536.0f*65536.0f);
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Vector and Matrix                                                  ***/
/***                                                                      ***/
/****************************************************************************/

#if !sMOBILE

//void sVector::Init(sInt3 &v)                                 {x=v.x/65536.0f; y=v.y/65536.0f; z=v.z/65536.0f; w=0.0f;}
//void sVector::Init(sInt4 &v)																	{x=v.x/65536.0f; y=v.y/65536.0f; z=v.z/65536.0f; w=v.w/65536.0f;}
//void sVector::InitColor(sU32 col)                            {x=((col>>0)&0xff)/255.0f;y=((col>>8)&0xff)/255.0f;z=((col>>16)&0xff)/255.0f;w=((col>>24)&0xff)/255.0f;}
//sU32 sVector::GetColor()                                     {return sRange<sInt>(x*255,255,0)|(sRange<sInt>(y*255,255,0)<<8)|(sRange<sInt>(z*255,255,0)<<16)|(sRange<sInt>(w*255,255,0)<<24);}

void sVector::InitRnd()
{
  do
  {
    x = sFGetRnd()*2.0f-1.0f;
    y = sFGetRnd()*2.0f-1.0f;
    z = sFGetRnd()*2.0f-1.0f;
  }
  while(Dot3(*this)>1.0f);
}

void sVector::Lin3(const sVector &a,const sVector &b,sF32 t) {x=a.x+(b.x-a.x)*t; y=a.y+(b.y-a.y)*t; z=a.z+(b.z-a.z)*t;}
void sVector::Rotate3(const sMatrix &m,const sVector &v)     {Scale3(m.i,v.x); AddScale3(m.j,v.y); AddScale3(m.k,v.z);}
void sVector::Rotate3(const sMatrix &m)                      {sVector v; v=*this; Rotate3(m,v);}
void sVector::RotateT3(const sMatrix &m,const sVector &v)    {x=m.i.Dot3(v); y=m.j.Dot3(v); z=m.k.Dot3(v);}
void sVector::RotateT3(const sMatrix &m)                     {sVector v; v=*this; RotateT3(m,v);}
void sVector::Cross3(const sVector &a,const sVector &b)      {x=a.y*b.z-a.z*b.y; y=a.z*b.x-a.x*b.z; z=a.x*b.y-a.y*b.x;}
sF32 sVector::Abs3() const                                   {return sFSqrt(Dot3(*this));}
void sVector::Unit3()                                        {Scale3(sFInvSqrt(Dot3(*this)));}
sF32 sVector::UnitAbs3()                                     {sF32 e=sFSqrt(Dot3(*this)); if(e>1e-10f) Scale3(1.0f/e); else Init3(1,0,0); return e; }
void sVector::UnitSafe3()                                    {sF32 e=Dot3(*this); if(e>1e-20f) Scale3(sFInvSqrt(e)); else Init3(1,0,0);}

void sVector::Lin4(const sVector &a,const sVector &b,sF32 t) {x=a.x+(b.x-a.x)*t; y=a.y+(b.y-a.y)*t; z=a.z+(b.z-a.z)*t; w=a.w+(b.w-a.w)*t;}
void sVector::Rotate4(const sMatrix &m,const sVector &v)     {Scale4(m.i,v.x); AddScale4(m.j,v.y); AddScale4(m.k,v.z); AddScale4(m.l,v.w);}
void sVector::Rotate4(const sMatrix &m)                      {sVector v; v=*this; Rotate4(m,v);}
void sVector::Rotate34(const sMatrix &m,const sVector &v)    {Scale4(m.i,v.x); AddScale4(m.j,v.y); AddScale4(m.k,v.z); Add4(m.l);}
void sVector::Rotate34(const sMatrix &m)                     {sVector v; v=*this; Rotate34(m,v);}
void sVector::RotateT4(const sMatrix &m,const sVector &v)    {x=m.i.Dot4(v); y=m.j.Dot4(v); z=m.k.Dot4(v); w=m.l.Dot4(v);}
void sVector::RotateT4(const sMatrix &m)                     {sVector v; v=*this; RotateT4(m,v);}
void sVector::RotatePl(const sMatrix &m,const sVector &v)    {Rotate3(m,v); w=v.w-Dot3(m.l);}
void sVector::RotatePl(const sMatrix &m)                     {sVector v; v=*this; RotatePl(m,v);}
void sVector::Cross4(const sVector &a,const sVector &b)      {x=a.y*b.z-a.z*b.y; y=a.z*b.x-a.x*b.z; z=a.x*b.y-a.y*b.x; w=0;}
sF32 sVector::Abs4() const                                   {return Dot4(*this);}
void sVector::Unit4()                                        {Scale4(sFInvSqrt(Dot4(*this)));}
sF32 sVector::UnitAbs4()                                     {sF32 e=sFSqrt(Dot4(*this)); if(e>0.000001) Scale4(1.0f/e); else Init4(1,0,0,0); return e; }
void sVector::UnitSafe4()                                    {sF32 e=Dot4(*this); if(e>0.00001) Scale4(sFInvSqrt(e)); else Init4(1,0,0,0);}

sInt sVector::Classify()
{
  if(sFAbs(x) > sFAbs(y) && sFAbs(x) > sFAbs(z))
  {
    if(x>0) return 1;
    else return 0;
  }
  else if(sFAbs(y) > sFAbs(z))
  {
    if(y>0) return 3;
    else return 2;
  }
  else
  {
    if(z>0) return 5;
    else return 4;
  }  
}

void sVector::Plane(const sVector &a,const sVector &b,const sVector &c)
{
  sVector d1,d2;

  d1.Sub3(b,a);
  d2.Sub3(c,a);
  Cross3(d1,d2);
  w = -Dot3(a);
}

void sVector::Write(sU32 *&p)
{
  ((sF32 *)p)[0] = x;
  ((sF32 *)p)[1] = y;
  ((sF32 *)p)[2] = z;
  ((sF32 *)p)[3] = w;
  p+=4;
}


void sVector::Write3(sU32 *&p)
{
  ((sF32 *)p)[0] = x;
  ((sF32 *)p)[1] = y;
  ((sF32 *)p)[2] = z;
  p+=3;
}


void sVector::Write3U(sU32 *&p)
{
  ((sF32 *)p)[0] = x;
  ((sF32 *)p)[1] = y;
  p+=2;
}


void sVector::Read(sU32 *&p)
{
  x = ((sF32 *)p)[0];
  y = ((sF32 *)p)[1];
  z = ((sF32 *)p)[2];
  w = ((sF32 *)p)[3];
  p+=4;
}


void sVector::Read3(sU32 *&p)
{
  x = ((sF32 *)p)[0];
  y = ((sF32 *)p)[1];
  z = ((sF32 *)p)[2];
  w = 1.0f;
  p+=3;
}


void sVector::Read3U(sU32 *&p)
{
  x = ((sF32 *)p)[0];
  y = ((sF32 *)p)[1];
  z = sFSqrt(1.0f-x*x-y*y);
  w = 1.0f;
  p+=2;
}

/****************************************************************************/
/****************************************************************************/

void sMatrix::Write(sU32 *&f)
{
  i.Write(f);
  j.Write(f);
  k.Write(f);
  l.Write(f);
}

void sMatrix::WriteR(sU32 *&f)
{
  i.Write3U(f);
  j.Write3U(f);
  l.Write3(f);
}

void sMatrix::Write33(sU32 *&f)
{
  i.Write3(f);
  j.Write3(f);
  k.Write3(f);
}

void sMatrix::Write34(sU32 *&f)
{
  i.Write3(f);
  j.Write3(f);
  k.Write3(f);
  l.Write3(f);
}

void sMatrix::Write33R(sU32 *&f)
{
  i.Write3U(f);
  j.Write3U(f);
}

void sMatrix::Read(sU32 *&f)
{
  i.Read(f);
  j.Read(f);
  k.Read(f);
  l.Read(f);
}

void sMatrix::ReadR(sU32 *&f)
{
  i.Read3U(f); i.w=0.0f;
  j.Read3U(f); j.w=0.0f;
  k.Cross3(i,j); k.w=0.0f;
  l.Read3(f); l.w=1.0f;
}

void sMatrix::Read33(sU32 *&f)
{
  i.Read3(f); i.w = 0.0f;
  j.Read3(f); j.w = 0.0f;
  k.Read3(f); k.w = 0.0f;
  l.Init(0.0f,0.0f,0.0f,1.0f);
}

void sMatrix::Read34(sU32 *&f)
{
  i.Read3(f); i.w = 0.0f;
  j.Read3(f); j.w = 0.0f;
  k.Read3(f); k.w = 0.0f;
  l.Read3(f); k.w = 1.0f;
}

void sMatrix::Read33R(sU32 *&f)
{
  i.Read3U(f); i.w=0.0f;
  j.Read3U(f); j.w=0.0f;
  k.Cross3(i,j); k.w=0.0f;
  l.Init(0.0f,0.0f,0.0f,1.0f);
}

/****************************************************************************/
/****************************************************************************/

void sMatrix::Init()
{
  i.Init(1,0,0,0);
  j.Init(0,1,0,0);
  k.Init(0,0,1,0);
  l.Init(0,0,0,1);
}

/****************************************************************************/

void sMatrix::InitRot(const sVector &v)
{
  sF32 abs = v.Abs3();
  if(abs<0.00000001)
  {
    Init();
  }
  else
  {
    InitRot(v,abs);
  }
}

void sMatrix::InitRot(const sVector &v,sF32 angle)
{
  sMatrix c,s;
  sVector u;
  sF32 cc,ss;

  cc = sFCos(angle);
  ss = sFSin(angle);
  u = v;
  u.UnitSafe3();

  s.i.x = ss *  0;
  s.i.y = ss *  u.z; 
  s.i.z = ss * -u.y;
  s.j.x = ss * -u.z;
  s.j.y = ss *  0;
  s.j.z = ss *  u.x;
  s.k.x = ss *  u.y;
  s.k.y = ss * -u.x;
  s.k.z = ss *  0;

  c.i.x = cc * (1.0f-u.x*u.x);
  c.i.y = cc *      -u.x*u.y ;
  c.i.z = cc *      -u.x*u.z ;
  c.j.x = cc *      -u.y*u.x ;
  c.j.y = cc * (1.0f-u.y*u.y);
  c.j.z = cc *      -u.y*u.z ;
  c.k.x = cc *      -u.z*u.x ;
  c.k.y = cc *      -u.z*u.y ; 
  c.k.z = cc * (1.0f-u.z*u.z);

  i.x = u.x*u.x + c.i.x + s.i.x;
  i.y = u.x*u.y + c.i.y + s.i.y;
  i.z = u.x*u.z + c.i.z + s.i.z;
  i.w = 0.0f;
  j.x = u.y*u.x + c.j.x + s.j.x;
  j.y = u.y*u.y + c.j.y + s.j.y;
  j.z = u.y*u.z + c.j.z + s.j.z;
  j.w = 0.0f;
  k.x = u.z*u.x + c.k.x + s.k.x;
  k.y = u.z*u.y + c.k.y + s.k.y;
  k.z = u.z*u.z + c.k.z + s.k.z;
  k.w = 0.0f;
  l.Init4(0,0,0,1);
}

/****************************************************************************/

void sMatrix::InitDir(const sVector &v)
{
  j.Init3(0,1,0);
  k = v;
  k.UnitSafe3();
  i.Cross3(j,k);
  i.UnitSafe3();
  j.Cross3(k,i);
  j.UnitSafe3();
  i.w = 0.0f;
  j.w = 0.0f;
  k.w = 0.0f;
  l.Init4(0,0,0,1);
}

/****************************************************************************/

//#if !sINTRO
void sMatrix::InitEuler(sF32 a,sF32 b,sF32 c)
{
  sF32 sx,sy,sz;
  sF32 cx,cy,cz;

  sFSinCos(a,sx,cx);
  sFSinCos(b,sy,cy);
  sFSinCos(c,sz,cz);

  i.x =  cy*cz;
  i.y =  cy*sz;
  i.z = -sy;
  i.w = 0.0f;
  j.x = -cx*sz + sx*sy*cz;
  j.y =  cx*cz + sx*sy*sz;
  j.z =  sx*cy;
  j.w = 0.0f;
  k.x =  sx*sz + cx*sy*cz;
  k.y = -sx*cz + cx*sy*sz;
  k.z =  cx*cy;
  k.w = 0.0f;
  l.Init4(0,0,0,1);
}
//#endif

void sMatrix::InitEulerPI2(const sF32 *a)
{
  sF32 sx,sy,sz;
  sF32 cx,cy,cz;

  if(sFAbs(a[0]) < 1e-6f && sFAbs(a[1]) < 1e-6f && sFAbs(a[2]) < 1e-6f)
    Init();
  else
  {
    sFSinCos(a[0]*sPI2F,sx,cx);
    sFSinCos(a[1]*sPI2F,sy,cy);
    sFSinCos(a[2]*sPI2F,sz,cz);

    i.x =  cy*cz;
    i.y =  cy*sz;
    i.z = -sy;
    i.w = 0.0f;
    j.x = -cx*sz + sx*sy*cz;
    j.y =  cx*cz + sx*sy*sz;
    j.z =  sx*cy;
    j.w = 0.0f;
    k.x =  sx*sz + cx*sy*cz;
    k.y = -sx*cz + cx*sy*sz;
    k.z =  cx*cy;
    k.w = 0.0f;
    l.Init4(0,0,0,1);
  }
}


/****************************************************************************/

void sMatrix::InitSRT(const sF32 *srt)
{
  InitEulerPI2(srt+3);
  i.Scale3(srt[0]);
  j.Scale3(srt[1]);
  k.Scale3(srt[2]);
  l.Init3(srt[6],srt[7],srt[8]);
}

/****************************************************************************/

void sMatrix::InitRandomSRT(const sF32 *srt)
{
  sInt i;
  sF32 srtc[9];

  for(i=0;i<3;i++)
    srtc[i] = (srt[i] - 1.0f) * sFGetRnd() + 1.0f;

  for(i=3;i<9;i++)
    srtc[i] = srt[i] * (sFGetRnd() - 0.5f) * 2.0f;

  InitSRT(srtc);
}

/****************************************************************************/

void sMatrix::InitSRTInv(const sF32 *srt)
{
  sVector v;
  InitEulerPI2(srt+3);
  Trans3();
  i.Scale3(1.0f/srt[0]);
  j.Scale3(1.0f/srt[1]);
  k.Scale3(1.0f/srt[2]);
  v.Init3(-srt[6],-srt[7],-srt[8]);
  l.Rotate3(*this,v);
}

/****************************************************************************/

void sMatrix::InitClassify(sInt id)
{
  static const sVector dirs[6] = 
  {
    { -1, 0, 0 },
    {  1, 0, 0 },
    {  0,-1, 0 },
    {  0, 1, 0 },
    {  0, 0,-1 },
    {  0, 0, 1 },
  };

  Init();
  k = dirs[id];
  switch(id)
  {
  case 0:
  case 1:
  case 4:
  case 5:
    j.Init(0,1,0);
    break;
  case 2:
    j.Init(0,0,1);
    break;
  case 3:
    j.Init(0,0,1);
    break;
  }
  i.Cross3(j,k);
}

/****************************************************************************/

void sMatrix::PivotTransform(sF32 x,sF32 y,sF32 z)
{
  sMatrix t1,t2;

  t1.Init();
  t1.l.x = -x;
  t1.l.y = -y;
  t1.l.z = -z;
  t2 = *this;
  MulA(t1,t2);
  l.x += x;
  l.y += y;
  l.z += z;
}

/****************************************************************************/

void sMatrix::FindEuler(sF32 &a,sF32 &b,sF32 &c)
{
	sF32 cy;

	cy = sFSqrt(i.x*i.x+i.y*i.y);
	b = sFATan2(-i.z,cy);

	if(cy>1e-4f)
	{
		a = sFATan2(j.z,k.z);
		c = sFATan2(i.y,i.x);
	}
	else
	{
		a = sFATan2(-k.y,j.y);
		c = 0.0f;
	}
}

/****************************************************************************/
/****************************************************************************/

void sMatrix::Trans3()
{
  sSwap(i.y,j.x);
  sSwap(i.z,k.x);
  sSwap(j.z,k.y);
}

/****************************************************************************/

void sMatrix::Trans4()
{
  sSwap(i.y,j.x);
  sSwap(i.z,k.x);
  sSwap(i.w,l.x);
  sSwap(j.z,k.y);
  sSwap(j.w,l.y);
  sSwap(k.w,l.z);
}

/****************************************************************************/

void sMatrix::TransR()
{
  Trans3();
  l.Neg3();
  l.Rotate3(*this);
}

/****************************************************************************/
/****************************************************************************/

void sMatrix::InvCheap3()
{
  sF32 x,y,z;

  x = sFInvSqrt(i.Dot3(i));
  y = sFInvSqrt(j.Dot3(j));
  z = sFInvSqrt(k.Dot3(k));
  i.Scale3(x);
  j.Scale3(y);
  k.Scale3(z);
  Trans3();
  i.Scale3(x);
  j.Scale3(y);
  k.Scale3(z);
}

/****************************************************************************/

void sMatrix::Scale3(sF32 s)
{
  i.Scale3(s);
  j.Scale3(s);
  k.Scale3(s);
}

/****************************************************************************/
/****************************************************************************/

void sMatrix::InvCheap4()
{
  sF32 x,y,z;

  x = sFInvSqrt(i.Dot3(i));
  y = sFInvSqrt(j.Dot3(j));
  z = sFInvSqrt(k.Dot3(k));
  i.Scale3(x);
  j.Scale3(y);
  k.Scale3(z);
  Trans3();
  i.Scale3(x);
  j.Scale3(y);
  k.Scale3(z);
  l.Neg3();
  l.Rotate3(*this);
}

/****************************************************************************/

void sMatrix::Scale4(sF32 s)
{
  i.Scale4(s);
  j.Scale4(s);
  k.Scale4(s);
  l.Scale4(s);
}

/****************************************************************************/

void sMatrix::Invert(const sMatrix &a)
{
  sF32 s = 1.0f/(a.i.x*a.j.y*a.k.z+a.i.y*a.j.z*a.k.x+a.i.z*a.j.x*a.k.y
                -a.k.x*a.j.y*a.i.z-a.k.y*a.j.z*a.i.x-a.k.z*a.j.x*a.i.y);

  i.x = s*(a.j.y*a.k.z - a.j.z*a.k.y);
  i.y = s*(a.k.y*a.i.z - a.i.y*a.k.z);
  i.z = s*(a.i.y*a.j.z - a.j.y*a.i.z);
  i.w = 0.0f;
  j.x = s*(a.k.x*a.j.z - a.j.x*a.k.z);
  j.y = s*(a.i.x*a.k.z - a.i.z*a.k.x);
  j.z = s*(a.j.x*a.i.z - a.i.x*a.j.z);
  j.w = 0.0f;
  k.x = s*(a.j.x*a.k.y - a.k.x*a.j.y);
  k.y = s*(a.k.x*a.i.y - a.i.x*a.k.y);
  k.z = s*(a.i.x*a.j.y - a.j.x*a.i.y);
  k.w = 0.0f;
  l.x = -i.x*a.l.x - j.x*a.l.y - k.x*a.l.z;
  l.y = -i.y*a.l.x - j.y*a.l.y - k.y*a.l.z;
  l.z = -i.z*a.l.x - j.z*a.l.y - k.z*a.l.z;
  l.w = 1.0f;
}

/****************************************************************************/

void sMatrix::Lin4(sMatrix &m0,sMatrix &m1,sF32 fade)
{
  Init();
  i.Lin3(m0.i,m1.i,fade);
  j.Lin3(m0.j,m1.j,fade);
  k.Cross3(i,j);
  k.Unit3();
  i.Cross3(j,k);
  i.Unit3();
  j.Unit3();
  l.Lin3(m0.l,m1.l,fade);
}

/****************************************************************************/
/****************************************************************************/

void sMatrix::Mul3(const sMatrix &a,const sMatrix &b)
{
  i.Scale3(b.i,a.i.x); i.AddScale3(b.j,a.i.y); i.AddScale3(b.k,a.i.z);
  j.Scale3(b.i,a.j.x); j.AddScale3(b.j,a.j.y); j.AddScale3(b.k,a.j.z);
  k.Scale3(b.i,a.k.x); k.AddScale3(b.j,a.k.y); k.AddScale3(b.k,a.k.z);
/*
  i.x = a.i.x*b.i.x + a.i.y*b.j.x + a.i.z*b.k.x;
  i.y = a.i.x*b.i.y + a.i.y*b.j.y + a.i.z*b.k.y;
  i.z = a.i.x*b.i.z + a.i.y*b.j.z + a.i.z*b.k.z;
  j.x = a.j.x*b.i.x + a.j.y*b.j.x + a.j.z*b.k.x;
  j.y = a.j.x*b.i.y + a.j.y*b.j.y + a.j.z*b.k.y;
  j.z = a.j.x*b.i.z + a.j.y*b.j.z + a.j.z*b.k.z;
  k.x = a.k.x*b.i.x + a.k.y*b.j.x + a.k.z*b.k.x;
  k.y = a.k.x*b.i.y + a.k.y*b.j.y + a.k.z*b.k.y;
  k.z = a.k.x*b.i.z + a.k.y*b.j.z + a.k.z*b.k.z;
*/
}

/****************************************************************************/

void sMatrix::Mul4(const sMatrix &a,const sMatrix &b)
{
  // 64 muls, 48 adds
  sVector *out = &i;

  for(sInt n=0;n<4;n++)
  {
    sF32 ax,ay,az,aw;

    ax = a[n].x; ay = a[n].y; az = a[n].z; aw = a[n].w;
    out->x = b.i.x * ax + b.j.x * ay + b.k.x * az + b.l.x * aw;
    out->y = b.i.y * ax + b.j.y * ay + b.k.y * az + b.l.y * aw;
    out->z = b.i.z * ax + b.j.z * ay + b.k.z * az + b.l.z * aw;
    out->w = b.i.w * ax + b.j.w * ay + b.k.w * az + b.l.w * aw;
    out++;
  }
}

/****************************************************************************/

void sMatrix::MulA(const sMatrix &a,const sMatrix &b)
{
  // 36 muls, 27 adds => use when possible!
  sVector *out = &i;

  // rotational part first (assume a.w = b.w = 0)
  for(sInt n=0;n<4;n++)
  {
    sF32 ax,ay,az;

    ax = a[n].x; ay = a[n].y; az = a[n].z;
    out->x = b.i.x * ax + b.j.x * ay + b.k.x * az;
    out->y = b.i.y * ax + b.j.y * ay + b.k.y * az;
    out->z = b.i.z * ax + b.j.z * ay + b.k.z * az;
    out->w = 0.0f;
    out++;
  }

  // now add translations
  l.x += b.l.x;
  l.y += b.l.y;
  l.z += b.l.z;
  l.w  = 1.0f;
}

/****************************************************************************/
/****************************************************************************/

void sQuaternion::InitAxisAngle(const sVector &axis,sF32 angle)
{
  sF32 s,c;
  
  sFSinCos(angle * 0.5f,s,c);
  s /= axis.Abs3();

  w = c;
  x = s * axis.x;
  y = s * axis.y;
  z = s * axis.z;
}

void sQuaternion::Lin(const sQuaternion &a,const sQuaternion &b,sF32 t)
{
  sF32 dot = a.w*b.w + a.x*b.x + a.y*b.y + a.z*b.z;

  if(dot<0)
  {
    w = a.w + (-b.w-a.w)*t;
    x = a.x + (-b.x-a.x)*t;
    y = a.y + (-b.y-a.y)*t;
    z = a.z + (-b.z-a.z)*t;
  }
  else
  {
    w = a.w + (b.w-a.w)*t;
    x = a.x + (b.x-a.x)*t;
    y = a.y + (b.y-a.y)*t;
    z = a.z + (b.z-a.z)*t;
  }

  Unit();
}

sF32 sQuaternion::Abs() const
{
  return sFSqrt(w*w + x*x + y*y + z*z);
}

void sQuaternion::Unit()
{
  Scale(sFInvSqrt(w*w + x*x + y*y + z*z));
}

void sQuaternion::UnitSafe()
{
  sF32 len = Abs();
  if(len > 1e-6f)
    Scale(1.0f / len);
  else
    Init(1,0,0,0);
}

// Left operand may be *this, right may not.
void sQuaternion::Mul(const sQuaternion &a,const sQuaternion &b)
{
  sF32 aw = a.w, ax = a.x, ay = a.y, az = a.z;

  w = aw*b.w - ax*b.x - ay*b.y - az*b.z;
  x = ax*b.w + aw*b.x - az*b.y + ay*b.z;
  y = ay*b.w + az*b.x + aw*b.y - ax*b.z;
  z = az*b.w - ay*b.x + ax*b.y + aw*b.z;
}

void sQuaternion::MulL(const sQuaternion &a)
{
  sF32 bw = w, bx = x, by = y, bz = z;

  w = a.w*bw - a.x*bx - a.y*by - a.z*bz;
  x = a.x*bw + a.w*bx - a.z*by + a.y*bz;
  y = a.y*bw + a.z*bx + a.w*by - a.x*bz;
  z = a.z*bw - a.y*bx + a.x*by + a.w*bz;
}

// Only makes sense for unit quaternions.
void sQuaternion::ToMatrix(sMatrix &m) const
{
  sF32 xx = 2.0f*x*x, xy = 2.0f*x*y, xz = 2.0f*x*z;
  sF32 yy = 2.0f*y*y, yz = 2.0f*y*z, zz = 2.0f*z*z;
  sF32 xw = 2.0f*x*w, yw = 2.0f*y*w, zw = 2.0f*z*w;

  // maybe need to transpose this
  m.i.x = 1.0f - yy - zz;
  m.i.y =        xy - zw;
  m.i.z =        xz + yw;
  m.i.w = 0.0f;
  m.j.x =        xy + zw;
  m.j.y = 1.0f - xx - zz;
  m.j.z =        yz - xw;
  m.j.w = 0.0f;
  m.k.x =        xz - yw;
  m.k.y =        yz + xw;
  m.k.z = 1.0f - xx - yy;
  m.k.w = 0.0f;
  m.l.x = 0.0f;
  m.l.y = 0.0f;
  m.l.z = 0.0f;
  m.l.w = 1.0f;
}

void sQuaternion::FromMatrix(const sMatrix &mat)
{
  sF32 tr,s;
	
	tr = mat.i.x + mat.j.y + mat.k.z;

	if(tr>=0) 
  {
		s = (sF32)sFSqrt(tr + 1);
		w = s*0.5f;
		s = 0.5f / s;
		x = (mat.k.y - mat.j.z) * s;
		y = (mat.i.z - mat.k.x) * s;
		z = (mat.j.x - mat.i.y) * s;
	}
	else 
  {
		int index;
		if (mat.j.y > mat.i.x)
    {      
		  if (mat.k.z > mat.j.y) index=2; else index=1;
    }
    else
    {      
		  if (mat.k.z > mat.i.x) index=2; else index=0;
    }

		switch(index)
    {
		case 0:
			s = (sF32)sFSqrt((mat.i.x - (mat.j.y+mat.k.z)) + 1.0f );
			x = s*0.5f;
			s = 0.5f / s;
			y = (mat.i.y + mat.j.x) * s;
			z = (mat.k.x + mat.i.z) * s;
			w = (mat.k.y - mat.j.z) * s;
			break;
		case 1:
			s = (sF32)sFSqrt( (mat.j.y - (mat.k.z+mat.i.x)) + 1.0f );
			y = s*0.5f;
			s = 0.5f / s;
			z = (mat.j.z + mat.k.y) * s;
			x = (mat.i.y + mat.j.x) * s;
			w = (mat.i.z - mat.k.x) * s;
			break;
		case 2:
			s = (sF32)sFSqrt( (mat.k.z - (mat.i.x+mat.j.y)) + 1.0f );
			z = s*0.5f;
			s = 0.5f / s;
			x = (mat.k.x + mat.i.z) * s;
			y = (mat.j.z + mat.k.y) * s;
			w = (mat.j.x - mat.i.y) * s;
			break;
		}
	}
}

/****************************************************************************/
/****************************************************************************/

void sAABox::Rotate34(const sAABox &a,const sMatrix &m)
{
  sVector v[3];
  sInt i;

  v[0].Scale3(m.i,a.Max.x-a.Min.x);
  v[1].Scale3(m.j,a.Max.y-a.Min.y);
  v[2].Scale3(m.k,a.Max.z-a.Min.z);
  Min.Rotate34(m,a.Min);
  Max = Min;

  for(i=0;i<3;i++)
  {
    if(v[i].x<0) Min.x += v[i].x; else Max.x += v[i].x;
    if(v[i].y<0) Min.y += v[i].y; else Max.y += v[i].y;
    if(v[i].z<0) Min.z += v[i].z; else Max.z += v[i].z;
  }
}

void sAABox::Or(const sAABox &b)
{
  Min.x = sMin(Min.x,b.Min.x);
  Min.y = sMin(Min.y,b.Min.y);
  Min.z = sMin(Min.z,b.Min.z);
  Max.x = sMax(Max.x,b.Max.x);
  Max.y = sMax(Max.y,b.Max.y);
  Max.z = sMax(Max.z,b.Max.z);
}

void sAABox::And(const sAABox &b)
{
  Min.x = sMax(Min.x,b.Min.x);
  Min.y = sMax(Min.y,b.Min.y);
  Min.z = sMax(Min.z,b.Min.z);
  Max.x = sMin(Max.x,b.Max.x);
  Max.y = sMin(Max.y,b.Max.y);
  Max.z = sMin(Max.z,b.Max.z);
}

sBool sAABox::Intersects(const sAABox &b)
{
  return sMax(Min.x,b.Min.x) < sMin(Max.x,b.Max.x)
    && sMax(Min.y,b.Min.y) < sMin(Max.y,b.Max.y)
    && sMax(Min.z,b.Min.z) < sMin(Max.z,b.Max.z);
}

sBool sAABox::IntersectsSphere(const sVector &center,sF32 radius)
{
  sF32 d = 0.0f, dt;

  if((dt = center.z - Min.z) < 0.0f)
    d += dt * dt;
  else if((dt = center.z - Max.z) > 0.0f)
    d += dt * dt;
  
  if((dt = center.x - Min.x) < 0.0f)
    d += dt * dt;
  else if((dt = center.x - Max.x) > 0.0f)
    d += dt * dt;

  if((dt = center.y - Min.y) < 0.0f)
    d += dt * dt;
  else if((dt = center.y - Max.y) > 0.0f)
    d += dt * dt;

  return d <= radius * radius;
}

/****************************************************************************/

void sCullPlane::LinearComb(const sVector &a,sF32 wa,const sVector &b,sF32 wb)
{
  Plane.Scale4(a,wa);
  Plane.AddScale4(b,wb);
  CalcNVert();
}

void sCullPlane::From3Points(const sVector &a,const sVector &b,const sVector &c)
{
  Plane.Plane(a,b,c);
  CalcNVert();
}

void sCullPlane::CalcNVert()
{
  sInt ind;

  ind  = (Plane.x > 0.0f) ? 1 : 0;
  ind |= (Plane.y > 0.0f) ? 2 : 0;
  ind |= (Plane.z > 0.0f) ? 4 : 0;

  NVert = ind;
}

void sCullPlane::Normalize()
{
  Plane.Scale4(sFInvSqrt(Plane.Dot3(Plane)));
}

sF32 sCullPlane::Distance(const sVector &x) const
{
  return Plane.Dot3(x) + Plane.w;
}

sBool sCullBBox(const sAABox &box,const sCullPlane *planes,sInt planeCount)
{
  sVector v;
  sInt i,vi;

  for(i=0;i<planeCount;i++)
  {
    vi = planes[i].NVert;
    v.x = (vi & 1) ? box.Max.x : box.Min.x;
    v.y = (vi & 2) ? box.Max.y : box.Min.y;
    v.z = (vi & 4) ? box.Max.z : box.Min.z;

    if(planes[i].Distance(v) < -1e-6f) // near vert out
      return sTRUE;
  }

  return sFALSE;
}

/****************************************************************************/

void sFrustum::FromViewProject(const sMatrix &viewProject,const sFRect &viewport)
{
  sMatrix temp;

  // make transposed view/projection matrix
  temp = viewProject;
  temp.Trans4();

  // build frustum planes from linear combinations
  Planes[0].LinearComb(temp.l,-viewport.x0,temp.i, 1.0f); // left
  Planes[1].LinearComb(temp.l, viewport.x1,temp.i,-1.0f); // right
  Planes[2].LinearComb(temp.l,-viewport.y0,temp.j, 1.0f); // bottom
  Planes[3].LinearComb(temp.l, viewport.y1,temp.j,-1.0f); // top
  Planes[4].LinearComb(temp.l,        0.0f,temp.k, 1.0f); // near
  Planes[5].LinearComb(temp.l,        1.0f,temp.k,-1.0f); // far

  NPlanes = Planes[5].Plane.Dot3(Planes[5].Plane) > 1e-20f ? 6 : 5;
}

void sFrustum::ZFailVolume(const sVector &light,const sMatrix &view,sF32 znear,sF32 zoom[2],const sFRect &viewport)
{
  sVector points[4];
  static const sF32 onEpsilon = 1e-4f;

  // calc projection of frustum points onto znear plane in world space
  sF32 xs = znear / zoom[0];
  sF32 ys = znear / zoom[1];

  for(sInt i=0;i<4;i++)
  {
    points[i] = view.l;
    points[i].AddScale3(view.i,xs*((i&1) ? viewport.x1 : viewport.x0));
    points[i].AddScale3(view.j,ys*((i&2) ? viewport.y1 : viewport.y0));
    points[i].AddScale3(view.k,znear);
  }

  // build planes
  Planes[0].From3Points(points[0],points[1],points[2]);         // near
  sF32 znd = Planes[0].Distance(light);

  if(sFAbs(znd) >= onEpsilon) // before/behind znear
  {
    Planes[1].From3Points(light,points[1],points[0]);           // top
    Planes[2].From3Points(light,points[3],points[1]);           // right
    Planes[3].From3Points(light,points[2],points[3]);           // bottom
    Planes[4].From3Points(light,points[0],points[2]);           // left

    if(znd < 0.0f) // we were behind znear, so flip all planes
    {
      for(sInt i=0;i<5;i++)
      {
        Planes[i].Plane.Scale4(-1.0f);
        Planes[i].CalcNVert();
      }
    }

    // make additional plane to "close off" behind light, improving cull ratio
    Planes[5].Plane.Sub3(view.l,light);
    Planes[5].Plane.AddScale3(view.k,znear);
    Planes[5].Plane.w = -Planes[5].Plane.Dot3(light);
    Planes[5].CalcNVert();

    NPlanes = 6;
  }
  else // on znear (degenerate case)
  {
    Planes[1].Plane.Scale4(Planes[0].Plane,-1.0f);
    Planes[1].CalcNVert();

    // move planes to encompass epsilon range
    Planes[0].Plane.w += onEpsilon;
    Planes[1].Plane.w -= onEpsilon;

    NPlanes = 2;
  }
}

void sFrustum::EnlargeToInclude(const sVector &point)
{
  for(sInt i=0;i<NPlanes;i++)
  {
    sF32 d = Planes[i].Distance(point);
    if(d < 0.0f) // point behind plane => move plane backwards
      Planes[i].Plane.w -= d;
  }
}

void sFrustum::Normalize()
{
  for(sInt i=0;i<NPlanes;i++)
    Planes[i].Normalize();
}

#pragma lekktor(on)
#endif

/****************************************************************************/
/***                                                                      ***/
/***   Static Strings                                                     ***/
/***                                                                      ***/
/****************************************************************************/

void sSPrintF(const sStringDesc &desc,const sChar *format,...)
{
  sFormatString(desc.Buffer,desc.Size,format,&format);
}

/****************************************************************************/
/***                                                                      ***/
/***   Arrays                                                             ***/
/***                                                                      ***/
/****************************************************************************/

#pragma lekktor(off)

void sArrayInit(sArrayBase *base,sInt s,sInt c)
{
  if(c)
    base->Array = new sU8[s*c];
  else
    base->Array = 0;
  base->Count = 0;
  base->Alloc = c;
  base->TypeSize = s;
}

void sArraySetMax(sArrayBase *base,sInt c)
{
  sU8 *n;
  //sVERIFY(c > base->Alloc);
  sVERIFY(c >= base->Count);
  if(c != base->Alloc)
  {
    n=new sU8[c*base->TypeSize]; 
    if(base->Array)
    {
      sCopyMem(n,base->Array,base->TypeSize*base->Count); 
      delete[] base->Array; 
    }
    base->Count=sMin(base->Count,c); 
    base->Array=n; 
    base->Alloc=c; 
  }
}

void sArrayAtLeast(sArrayBase *base,sInt c)
{
  if(c>base->Alloc) 
    sArraySetMax(base,sMax(c,base->Alloc*2-base->Alloc/2));
}

void sArrayCopy(sArrayBase *dest,const sArrayBase *src)
{
  dest->Count = 0;
  if(src->Count > dest->Alloc)
    sArraySetMax(dest,src->Count);
  sCopyMem(dest->Array,src->Array,src->TypeSize*src->Count);
  dest->Count = src->Count;
}

sU8 *sArrayAdd(sArrayBase *base)
{
  sU8 *data;
  sArrayAtLeast(base,base->Count+1); 
  data = base->Array+base->Count*base->TypeSize;
  base->Count++;
  return data;
}

#pragma lekktor(on)

/****************************************************************************/
/***                                                                      ***/
/***   Matrix Stack                                                       ***/
/***                                                                      ***/
/****************************************************************************/

#if !sMOBILE

void sMatrixStack::Init()
{

  Stack.Init(16);
  PushIdentity();
}

void sMatrixStack::Exit()
{
  Stack.Exit();
}

#endif

/****************************************************************************/

#if !sMOBILE

#pragma lekktor(off)

void sVector::Init(sInt3 &v)                                 {x=v.x/65536.0f; y=v.y/65536.0f; z=v.z/65536.0f; w=0.0f;}
void sVector::Init(sInt4 &v)																 {x=v.x/65536.0f; y=v.y/65536.0f; z=v.z/65536.0f; w=v.w/65536.0f;}
void sVector::InitColor(sU32 col)                            {x=((col>>16)&0xff)/255.0f;y=((col>>8)&0xff)/255.0f;z=((col>>0)&0xff)/255.0f;w=((col>>24)&0xff)/255.0f;}
void sVector::InitColorI(sU32 col)                           {x=((col>>0)&0xff)/255.0f;y=((col>>8)&0xff)/255.0f;z=((col>>16)&0xff)/255.0f;w=((col>>24)&0xff)/255.0f;}
sU32 sVector::GetColor()                                     {return sRange<sInt>(z*255,255,0)|(sRange<sInt>(y*255,255,0)<<8)|(sRange<sInt>(x*255,255,0)<<16)|(sRange<sInt>(w*255,255,0)<<24);}
sU32 sVector::GetColorI()                                    {return sRange<sInt>(x*255,255,0)|(sRange<sInt>(y*255,255,0)<<8)|(sRange<sInt>(z*255,255,0)<<16)|(sRange<sInt>(w*255,255,0)<<24);}

void sVector::Add4(const sVector &a,const sVector &b)        {x=a.x+b.x; y=a.y+b.y; z=a.z+b.z; w=a.w+b.w;}
void sVector::Sub4(const sVector &a,const sVector &b)        {x=a.x-b.x; y=a.y-b.y; z=a.z-b.z; w=a.w-b.w;}
void sVector::Mul4(const sVector &a,const sVector &b)        {x=a.x*b.x; y=a.y*b.y; z=a.z*b.z; w=a.w*b.w;}
void sVector::Neg4(const sVector &a)                         {x=-a.x; y=-a.y; z=-a.z; w=-a.w;}
void sVector::Add4(const sVector &a)                         {x+=a.x; y+=a.y; z+=a.z; w+=a.w;}
void sVector::Sub4(const sVector &a)                         {x-=a.x; y-=a.y; z-=a.z; w-=a.w;}
void sVector::Mul4(const sVector &a)                         {x*=a.x; y*=a.y; z*=a.z; w*=a.w;}
void sVector::Neg4()                                         {x=-x; y=-y; z=-z; w=-w;}
void sVector::AddMul4(const sVector &a,const sVector &b)     {x+=a.x*b.x; y+=a.y*b.y; z+=a.z*b.z; w+=a.w*b.w;}
sF32 sVector::Dot4(const sVector &a) const                   {return x*a.x + y*a.y + z*a.z + w*a.w;}
void sVector::Scale4(sF32 s)                                 {x*=s; y*=s; z*=s; w*=s;}
void sVector::Scale4(const sVector &a,sF32 s)                {x=a.x*s; y=a.y*s; z=a.z*s; w=a.w*s;}
void sVector::AddScale4(const sVector &a,sF32 s)             {x+=a.x*s; y+=a.y*s; z+=a.z*s; w+=a.w*s;}

void sVector::Add3(const sVector &a,const sVector &b)        {x=a.x+b.x; y=a.y+b.y; z=a.z+b.z;}
void sVector::Sub3(const sVector &a,const sVector &b)        {x=a.x-b.x; y=a.y-b.y; z=a.z-b.z;}
void sVector::Mul3(const sVector &a,const sVector &b)        {x=a.x*b.x; y=a.y*b.y; z=a.z*b.z;}
void sVector::Neg3(const sVector &a)                         {x=-a.x; y=-a.y; z=-a.z;}
void sVector::Add3(const sVector &a)                         {x+=a.x; y+=a.y; z+=a.z;}
void sVector::Sub3(const sVector &a)                         {x-=a.x; y-=a.y; z-=a.z;}
void sVector::Mul3(const sVector &a)                         {x*=a.x; y*=a.y; z*=a.z;}
void sVector::Neg3()                                         {x=-x; y=-y; z=-z;}
sF32 sVector::Dot3(const sVector &a) const                   {return x*a.x+y*a.y+z*a.z;}
void sVector::AddMul3(const sVector &a,const sVector &b)     {x+=a.x*b.x; y+=a.y*b.y; z+=a.z*b.z;}
void sVector::Scale3(sF32 s)                                 {x*=s; y*=s; z*=s;}
void sVector::Scale3(const sVector &a,sF32 s)                {x=a.x*s; y=a.y*s; z=a.z*s;}
void sVector::AddScale3(const sVector &a,sF32 s)             {x+=a.x*s; y+=a.y*s; z+=a.z*s;}

#endif 

/****************************************************************************/

} // namespace

/****************************************************************************/
