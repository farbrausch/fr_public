// Written by Fabian "ryg" Giesen.
// I hereby place this code in the public domain.

#include "_types.hpp"
#include "_startconsole.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Compiler                                                           ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/
/***                                                                      ***/
/***   Init/Exit                                                          ***/
/***                                                                      ***/
/****************************************************************************/

void sDLLSYSTEM sInitTypes();
void sDLLSYSTEM sExitTypes();
#if sINTRO
extern class ScriptRuntime *SR__;
#endif
/****************************************************************************/
/***                                                                      ***/
/***   Forwards                                                           ***/
/***                                                                      ***/
/****************************************************************************/

static sU32 sRandomSeed=0x74382381;

sU32 sGetRnd()
{
  sU32 i;

  _asm
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

  return i;
}

/****************************************************************************/

sU32 sGetRnd(sU32 max)
{
  return sGetRnd()%max;
}

/****************************************************************************/

sF32 sFGetRnd()
{
  return ((sGetRnd()&0x3fffffff)*1.0f)/0x40000000;
}

/****************************************************************************/

sF32 sFGetRnd(sF32 max)
{
  return ((sGetRnd()&0x3fffffff)*max)/0x40000000;
}

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

#if !sINTRO // this is inlined in spline class! 
void sHermite(sF32 *d,sF32 *p0,sF32 *p1,sF32 *p2,sF32 *p3,sInt count,sF32 fade,sF32 t,sF32 c,sF32 b)
{
  sF32 f1,f2,f3,f4;
  sF32 t0,t1,t2,t3;
  sF32 a1,b1,a2,b2;
  sInt i;

  a1 = (1-t)*(1-c)*(1+b)/2;
  b1 = (1-t)*(1+c)*(1-b)/2;
  a2 = (1-t)*(1+c)*(1+b)/2;
  b2 = (1-t)*(1-c)*(1-b)/2;

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
 
  t0 = t1-t0;
  t1 = t2-t1;
  t2 = t3-t2;
  f3 = f3 * 2*t1/(t1+t0);
  f4 = f4 * 2*t1/(t1+t2);

  sVERIFY(count<=9);

  for(i=0;i<count;i++)
  {
    d[i] = f1 * p1[i]
         + f2 * p2[i]
         + f3 * (a1*(p1[i]-p0[i]) + b1*(p2[i]-p1[i]))
         + f4 * (a2*(p2[i]-p1[i]) + b2*(p3[i]-p2[i]));
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
#if sINTRO

/****************************************************************************/

sF64 sFPow(sF64 a,sF64 b)
{
	__asm
	{
		fld		qword ptr [a];
		fld		qword ptr [b];

		fxch	st(1);
		ftst;
		fstsw	ax;
		sahf;
		jz		zero;

		fyl2x;
		fld1;
		fld		st(1);
		fprem;
		f2xm1;
		faddp	st(1), st;
		fscale;

zero:
		fstp	st(1);
		fstp	qword ptr [a];
	}
	
	return a;
}

sF64 sFExp(sF64 f)
{
	__asm
	{
		fld		qword ptr [f];
		fldl2e;
		fmulp	st(1), st;

		fld1;
		fld		st(1);
		fprem;
		f2xm1;
		faddp	st(1), st;
		fscale;

    fstp  st(1);
		fstp	qword ptr [f];
	}

	return f;
}


#endif


/****************************************************************************/
/***                                                                      ***/
/***   Memory and String                                                  ***/
/***                                                                      ***/
/****************************************************************************/

void sCopyMem4(sU32 *d,const sU32 *s,sInt c)
{
  __asm
  {
    mov esi,[s]
    mov edi,[d]
    mov ecx,[c]
    rep movsd
  }
}

#if !sINTRO
void sCopyMem8(sU64 *d,const sU64 *s,sInt c)  
{
  __asm
  {
    mov esi,[s]
    mov edi,[d]
    mov ecx,[c]
    add ecx,ecx
    rep movsd
  }
}
#endif

#if !sINTRO

sChar *sDupString(const sChar *s,sInt minsize)
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

#endif

/****************************************************************************/

#if !sINTRO || !sRELEASE
void sCopyString(sChar *d,const sChar *s,sInt size)
{
  while(size>0 && *s)
  {
    size--;
    *d++ = *s++;
  }
  *d = 0;
}
#endif


/****************************************************************************/

#if !sINTRO

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

const sChar *sFindString(const sChar *s,const sChar *f)
{
  sInt len,i,slen;

  len = sGetStringLen(f);
  slen = sGetStringLen(s);
  for(i=0;i<=slen-len;i++)
  {
    if(sCmpMem(s+i,f,len)==0)
      return s+i;
  }
  return 0;
}
/*
sChar *sFindStringI(sChar *s,sChar *f)
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
#endif
/****************************************************************************/
/****************************************************************************/

#if !sINTRO
sInt sScanInt(sChar *&scan)
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


sInt sScanHex(sChar *&scan)
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

sF32 sScanFloat(sChar *&scan)
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

/****************************************************************************/

sBool sScanString(sChar *&scan,sChar *buffer,sInt size)
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

sBool sScanName(sChar *&scan,sChar *buffer,sInt size)
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

void sScanSpace(sChar *&scan)
{
  while(*scan==' ' || *scan=='\t' || *scan=='\r' || *scan=='\n')
    scan++;
}

/****************************************************************************/

sBool sScanCycle(sChar *s,sInt index,sInt &start,sInt &len)
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

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Formatted Printing                                                 ***/
/***                                                                      ***/
/****************************************************************************/

#if !sINTRO || _DEBUG

extern "C" char * __cdecl _fcvt( double value, int count, int *dec, int *sign );

void sFormatString(sChar *d,sInt left,const sChar *s,sChar **fp)
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
  sF64 fval;
  static sChar hex[17] = "0123456789abcdef";
  static sChar HEX[17] = "0123456789ABCDEF";

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
      else if(c=='d' || c=='x' || c=='X' || c=='i' || c=='f')
      {
        len = 0;
        sign = 0;
        if(c=='f')
        {
          fval = sVARARGF(fp,arg);arg+=2;
        }
        else
        {
          val = sVARARG(fp,arg);arg++;
        }
        if(c=='f')        // this is preliminary!!!!!!!
        {
#if sINTRO
          string = "???";
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
          if(c=='f')
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
        string = (sChar *)sVARARG(fp,arg);arg++;
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
}


/****************************************************************************/

void __cdecl sSPrintF(sChar *buffer,sInt size,const sChar *format,...)
{
  sFormatString(buffer,size,format,(sChar **) &format);
}

#endif


/****************************************************************************/
/***                                                                      ***/
/***   File Reading and Writing                                           ***/
/***                                                                      ***/
/****************************************************************************/

#if !sINTRO
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
#endif

/****************************************************************************/
/***                                                                      ***/
/***   Debugging                                                          ***/
/***                                                                      ***/
/****************************************************************************/

void sVerifyFalse(sChar *file,sInt line)
{
#if !sINTRO
  sFatal("%s(%d) : assertion",file,line);
#else
  sFatal("assertion");
#endif
}

/****************************************************************************/

#if !sINTRO || _DEBUG 
void __cdecl sFatal(sChar *format,...)
{
  static sChar buffer[1024];

  sFormatString(buffer,sizeof(buffer),format,&format);
  sSystem->Log(buffer);
  sSystem->Log("\n");
  sSystem->Abort(buffer);
}
#endif

/****************************************************************************/

#if !sINTRO || !sRELEASE
void sDPrint(sChar *text)
{
  sSystem->Log(text);
}
#endif

/****************************************************************************/

#if !sINTRO || !sRELEASE
void __cdecl sDPrintF(sChar *format,...)
{
  sChar buffer[1024];

  sFormatString(buffer,sizeof(buffer),format,&format);
  sSystem->Log(buffer);
}  
#endif

/****************************************************************************/
/***                                                                       ***/
/***   Simple Structs                                                     ***/
/***                                                                      ***/
/****************************************************************************/

#if !sINTRO

void sRect::Init(struct sFRect &r) 
{
  x0=sFtol(r.x0); 
  y0=sFtol(r.y0);
  x1=sFtol(r.x1); 
  y1=sFtol(r.y1);
}

sBool sRect::Hit(sInt x,sInt y)
{
  return x>=x0 && x<x1 && y>=y0 && y<y1;
}

sBool sRect::Hit(sRect &rin)
{
  sRect r;
  r = *this;
  r.And(rin);
  return r.x0<r.x1 && r.y0<r.y1;
}

sBool sRect::Inside(sRect &r)
{
  if(x0  < r.x0) return sFALSE;
  if(y0  < r.y0) return sFALSE;
  if(x1 >= r.x1) return sFALSE;
  if(y1 >= r.y1) return sFALSE;
  return sTRUE;
}

void sRect::And(sRect &r)
{
  if(r.x0 > x0) x0 = r.x0;
  if(r.y0 > y0) y0 = r.y0;
  if(r.x1 < x1) x1 = r.x1;
  if(r.y1 < y1) y1 = r.y1;
}

void sRect::Or(sRect &r)
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

#endif

/****************************************************************************/
/****************************************************************************/

#if !sINTRO

void sFRect::Init(struct sRect &r) 
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

sBool sFRect::Hit(sFRect &rin)
{
  sFRect r;
  r = *this;
  r.And(rin);
  return r.x0<r.x1 && r.y0<r.y1;
}

void sFRect::And(sFRect &r)
{
  if(r.x0 > x0) x0 = r.x0;
  if(r.y0 > y0) y0 = r.y0;
  if(r.x1 < x1) x1 = r.x1;
  if(r.y1 < y1) y1 = r.y1;
}

void sFRect::Or(sFRect &r)
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
/****************************************************************************/

#if !sINTRO

static sInt sClamp256x(sInt a)
{
  if(a<0)
    return 0;
  if(a>255)
    return 255;
  return a;
}

void sColor::Fade(sF32 fade,sColor c0,sColor c1)
{
  sInt ff;

  ff = sFtol(fade*256);
  r = sClamp256x(c0.r + (((c1.r-c0.r)*ff)>>8));
  g = sClamp256x(c0.g + (((c1.g-c0.g)*ff)>>8));
  b = sClamp256x(c0.b + (((c1.b-c0.b)*ff)>>8));
  a = sClamp256x(c0.a + (((c1.a-c0.a)*ff)>>8));
}

void sColor::Fade(sInt fade,sColor c0,sColor c1)
{
  r = sClamp256x(c0.r + (((c1.r-c0.r)*fade)>>8));
  g = sClamp256x(c0.g + (((c1.g-c0.g)*fade)>>8));
  b = sClamp256x(c0.b + (((c1.b-c0.b)*fade)>>8));
  a = sClamp256x(c0.a + (((c1.a-c0.a)*fade)>>8));
}

#endif

/****************************************************************************/
/****************************************************************************/

#if !sINTRO

void sPlacer::Init(sRect &r,sInt xdiv,sInt ydiv)
{
  XPos = r.x0;
  YPos = r.y0;
  XSize = r.XSize();
  YSize = r.YSize();
  XDiv = xdiv;
  YDiv = ydiv;
}

void sPlacer::Div(sRect &r,sInt x,sInt y,sInt xs,sInt ys)
{
  xs+=x;
  ys+=y;
  r.x0 = XPos + x  * XSize / XDiv;
  r.y0 = YPos + y  * YSize / YDiv;
  r.x1 = XPos + xs * XSize / XDiv;
  r.y1 = YPos + ys * YSize / YDiv;
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Vector and Matrix                                                  ***/
/***                                                                      ***/
/****************************************************************************/

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
void sVector::UnitSafe3()                                    {sF32 e=Dot3(*this); if(e>0.00001) Scale3(sFInvSqrt(e)); else Init3(1,0,0);}

void sVector::Lin4(const sVector &a,const sVector &b,sF32 t) {x=a.x+(b.x-a.x)*t; y=a.y+(b.y-a.y)*t; z=a.z+(b.z-a.z)*t; w=a.w+(b.w-a.w)*t;}
void sVector::Rotate4(const sMatrix &m,const sVector &v)     {Scale4(m.i,v.x); AddScale4(m.j,v.y); AddScale4(m.k,v.z); AddScale4(m.l,v.w);}
void sVector::Rotate4(const sMatrix &m)                      {sVector v; v=*this; Rotate4(m,v);}
void sVector::Rotate34(const sMatrix &m,const sVector &v)    {Scale4(m.i,v.x); AddScale4(m.j,v.y); AddScale4(m.k,v.z); Add4(m.l);}
void sVector::Rotate34(const sMatrix &m)                     {sVector v; v=*this; Rotate34(m,v);}
void sVector::RotateT4(const sMatrix &m,const sVector &v)    {x=m.i.Dot4(v); y=m.j.Dot4(v); z=m.k.Dot4(v); w=m.l.Dot4(v);}
void sVector::RotateT4(const sMatrix &m)                     {sVector v; v=*this; RotateT4(m,v);}
sF32 sVector::Abs4() const                                   {return Dot4(*this);}
void sVector::Unit4()                                        {Scale4(sFInvSqrt(Dot4(*this)));}
void sVector::UnitSafe4()                                    {sF32 e=Dot4(*this); if(e>0.00001) Scale4(sFInvSqrt(e)); else Init4(1,0,0,0);}

#if !sINTRO

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

#endif

/****************************************************************************/
/****************************************************************************/

#if !sINTRO

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

#endif

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
  sF32 abs;
  sMatrix c,s;
  sVector u;
  sF32 cc,ss;

  abs = v.Abs3();
  if(abs>0.00000001)
  {
    Init();
  }
  else
  {
    cc = sFCos(abs);
    ss = sFSin(abs);
    u = v;
    u.Unit3();

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
    c.i.y = cc *       u.x*u.y ;
    c.i.z = cc *       u.x*u.z ;
    c.j.x = cc *       u.y*u.x ;
    c.j.y = cc * (1.0f-u.y*u.y);
    c.j.z = cc *       u.y*u.z ;
    c.k.x = cc *       u.z*u.x ;
    c.k.y = cc *       u.z*u.y ; 
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

#if !sINTRO
void sMatrix::InitEuler(sF32 a,sF32 b,sF32 c)
{
  sF32 sx,sy,sz;
  sF32 cx,cy,cz;

  sx = sFSin(a);
  cx = sFCos(a);
  sy = sFSin(b);
  cy = sFCos(b);
  sz = sFSin(c);
  cz = sFCos(c);

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
#endif

void sMatrix::InitEulerPI2(sF32 *a)
{
  sF32 sx,sy,sz;
  sF32 cx,cy,cz;
  sF32 v;
  v=a[0]*sPI2F;
  sx = sFSin(v);
  cx = sFCos(v);
  v=a[1]*sPI2F;
  sy = sFSin(v);
  cy = sFCos(v);
  v=a[2]*sPI2F;
  sz = sFSin(v);
  cz = sFCos(v);

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


/****************************************************************************/

void sMatrix::InitSRT(sF32 *srt)
{
  InitEulerPI2(srt+3);
  i.Scale3(srt[0]);
  j.Scale3(srt[1]);
  k.Scale3(srt[2]);
  l.Init3(srt[6],srt[7],srt[8]);
}

/****************************************************************************/

void sMatrix::InitSRTInv(sF32 *srt)
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

#if !sINTRO

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

#endif

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
  sVERIFYFALSE;
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
  sVERIFYFALSE;
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
#if sINTRO
  sInt i;
  sF32 *tt;
  const sF32 *aa,*bb;
  tt = &this->i.x;
  aa = &a.i.x;
  bb = &b.i.x;
  for(i=0;i<16;i++)
    tt[i] = aa[(i&12)+0]*bb[ 0+(i&3)]
          + aa[(i&12)+1]*bb[ 4+(i&3)]
          + aa[(i&12)+2]*bb[ 8+(i&3)]
          + aa[(i&12)+3]*bb[12+(i&3)];

#else
  i.Scale4(b.i,a.i.x); i.AddScale4(b.j,a.i.y); i.AddScale4(b.k,a.i.z); i.AddScale4(b.l,a.i.w);
  j.Scale4(b.i,a.j.x); j.AddScale4(b.j,a.j.y); j.AddScale4(b.k,a.j.z); j.AddScale4(b.l,a.j.w);
  k.Scale4(b.i,a.k.x); k.AddScale4(b.j,a.k.y); k.AddScale4(b.k,a.k.z); k.AddScale4(b.l,a.k.w);
  l.Scale4(b.i,a.l.x); l.AddScale4(b.j,a.l.y); l.AddScale4(b.k,a.l.z); l.AddScale4(b.l,a.l.w);
#endif
}

/****************************************************************************/

#if !sINTRO
void sMatrix::MulR(const sMatrix &a,const sMatrix &b)
{
  i.Scale3(b.i,a.i.x); i.AddScale3(b.j,a.i.y); i.AddScale3(b.k,a.i.z); i.w=0.0f;
  j.Scale3(b.i,a.j.x); j.AddScale3(b.j,a.j.y); j.AddScale3(b.k,a.j.z); j.w=0.0f;
  k.Scale3(b.i,a.k.x); k.AddScale3(b.j,a.k.y); k.AddScale3(b.k,a.k.z); k.w=0.0f;
  l.Rotate3(*this,b.l);
  l.Add3(a.l); l.w=1.0f;
}
#endif

/****************************************************************************/
/***                                                                      ***/
/***   Object Broker                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sBroker_::sBroker_()
{
  ObjectCount = 0;
  ObjectAlloc = 16*1024;
  Objects = new sObject*[ObjectAlloc];
#if !sINTRO
  RootCount = 0;
  RootAlloc = 4*1024;
  Roots = new sObject*[RootAlloc];
#endif
}

/****************************************************************************/

sBroker_::~sBroker_()
{
  delete[] Objects;
#if !sINTRO
  delete[] Roots;
#endif
}

/****************************************************************************/
/****************************************************************************/

void sBroker_::NewObject(sObject *obj)
{
  sVERIFY(ObjectCount<ObjectAlloc);
  Objects[ObjectCount] = obj;
  obj->TagVal = ObjectCount++;
  obj->_Label = 0;
}

/****************************************************************************/

void sBroker_::DeleteObject(sObject *obj)
{
  sInt i;

  if(obj)
  {
#if !sINTRO
    if(obj->TagVal&sTAGVAL_ROOT)
      RemRoot(obj);
#endif

    i = obj->TagVal&sTAGVAL_INDEX;
    sVERIFY(Objects[i] = obj);
    obj = Objects[--ObjectCount];
    Objects[i] = obj;
    obj->TagVal = (obj->TagVal & (~sTAGVAL_INDEX)) | i;
  }
}

/****************************************************************************/

#if !sINTRO

void sBroker_::AddRoot(sObject *root)
{
  sVERIFY(!(root->TagVal & sTAGVAL_ROOT));
  sVERIFY(RootCount<RootAlloc);
  Roots[RootCount++] = root;
  root->TagVal |= sTAGVAL_ROOT;
}

void sBroker_::RemRoot(sObject *root)
{
  sInt i;

  sVERIFY(root->TagVal & sTAGVAL_ROOT);
  root->TagVal &= ~sTAGVAL_ROOT;

  for(i=0;i<RootCount;i++)
  {
    if(Roots[i]==root)
    {
      Roots[i] = Roots[--RootCount];
      return;
    }
  }
  sFatal("root table mismatch");
}

#endif

/****************************************************************************/
/****************************************************************************/

void sBroker_::Need(sObject *obj)
{
  if(obj && !(obj->TagVal & sTAGVAL_USED))
  {
    obj->TagVal |= sTAGVAL_USED;
    obj->Tag();
  }
}

/****************************************************************************/

void sBroker_::Free()
{
  sInt i;

  for(i=0;i<ObjectCount;i++)
    Objects[i]->TagVal &= ~sTAGVAL_USED;

#if !sINTRO
  for(i=0;i<RootCount;i++)
    Need(Roots[i]);
  sSystem->Tag();
#else
  Need((sObject *)SR__);
#endif

  for(i=0;i<ObjectCount;)
  {
    if(!(Objects[i]->TagVal & sTAGVAL_USED))
      delete Objects[i];
    else
      i++;
  }
}

/****************************************************************************/

#if !sINTRO
void sBroker_::Dump()
{
  sInt i;
  if(RootCount>0 || ObjectCount>0)
  {
    sDPrintF("Broker Memory Leak!\n");
    sDPrintF("Roots:\n");
    for(i=0;i<RootCount;i++)
      sDPrintF("  %5d: %08x %08x\n",i,Roots[i],Roots[i]->GetClass());
    sDPrintF("All Objects:\n");
    for(i=0;i<ObjectCount;i++)
    {
      sDPrintF("  %5d: %08x %08x %c\n"
        ,i
        ,Objects[i]
        ,Objects[i]->GetClass()
        ,(Objects[i]->TagVal & sTAGVAL_ROOT) ? 'r':' ');
    }
  }
}
#endif

/****************************************************************************/
/****************************************************************************/

void sObject::Tag()
{
}

#if !sINTRO

sBool sObject::Write(sU32 *&)
{
  return sFALSE;
}

sBool sObject::Read(sU32 *&)
{
  return sFALSE;
}
#endif

#if !sINTRO
void sObject::Clear()
{
}

void sObject::Copy(sObject *)
{
#if !sINTRO
  sFatal("unimplemented copy called");
#endif
}
#endif

/****************************************************************************/
/***                                                                      ***/
/***   Arrays                                                             ***/
/***                                                                      ***/
/****************************************************************************/

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
  sVERIFY(c > base->Alloc);
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

void sArrayAtLeast(sArrayBase *base,sInt c)
{
  if(c>base->Alloc) 
    sArraySetMax(base,sMax(c,base->Alloc*2));
}

void sArrayCopy(sArrayBase *dest,sArrayBase *src)
{
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

/****************************************************************************/
/***                                                                      ***/
/***   Bitmap                                                             ***/
/***                                                                      ***/
/****************************************************************************/

sBitmap::sBitmap()
{
  Data = 0;
  XSize = 0;
  YSize = 0;
}

/****************************************************************************/

sBitmap::~sBitmap()
{
#if sINTRO
  if(Data)
    delete[] Data;
#else
  Clear();
#endif
}

/****************************************************************************/
/****************************************************************************/

void sBitmap::Init(sInt xs,sInt ys)
{
  if(xs!=XSize || ys!=YSize)
  {
    if(Data)
      delete[] Data;
    XSize = xs;
    YSize = ys;
    Data = new sU32[XSize*YSize];
  }
}

/****************************************************************************/

#if !sINTRO
void sBitmap::Clear()
{
  if(Data)
    delete[] Data;
  Data = 0;
  XSize = 0;
  YSize = 0;
}

/****************************************************************************/

void sBitmap::Copy(sObject *o)
{
  sBitmap *bm;

  sVERIFY(o->GetClass()==sCID_BITMAP);

  bm = (sBitmap *) o;
  Init(bm->XSize,bm->YSize);
  sCopyMem4(Data,bm->Data,XSize*YSize);
}
#endif

/****************************************************************************/

#if !sINTRO

sBool sBitmap::Write(sU32 *&p)
{
  *p++ = 1;
  *p++ = XSize;
  *p++ = YSize;
  sCopyMem4(p,Data,XSize*YSize);
  p+=XSize*YSize;
  return sTRUE;
}

/****************************************************************************/

sBool sBitmap::Read(sU32 *&p)
{
  sInt version;
  version = *p++;
  if(version<1 || version>1) return sFALSE;
  if(p[0]>0x2000 || p[1]>0x2000) return sFALSE;
  Init(p[0],p[1]);
  p+=2;
  sCopyMem4(Data,p,XSize*YSize);
  p+=XSize*YSize;
  return sTRUE;
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   StringSet                                                         ***/
/***                                                                      ***/
/******************** ********************************************************/

#if !sINTRO

sInt sStringSet::FindNr(sChar *string)
{
  sInt i;

  for(i=0;i<Set.Count;i++)
    if(sCmpString(Set[i].String,string)==0)
      return i;
  return -1;
}

void sStringSet::Init()
{
  Set.Init(16);
  Set.Count = 0;
}

void sStringSet::Exit()
{
  Set.Exit();
}

void sStringSet::Clear()
{
  Set.Count = 0;
}

sObject *sStringSet::Find(sChar *string)
{
  sInt i;

  i = FindNr(string);
  if(i!=-1)
    return Set[i].Object;
  else
    return 0;
}

sBool sStringSet::Add(sChar *string,sObject *obj)
{
  sInt i;

  i = FindNr(string);
  if(i!=-1)
  {
    Set[i].Object = obj;
    return sTRUE;
  }
  else
  {
    i = Set.Count++;
    Set.AtLeast(i+1);
    Set[i].String = string;
    Set[i].Object = obj;
    return sFALSE;
  }
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Text                                                               ***/
/***                                                                      ***/
/****************************************************************************/

#if !sINTRO
sChar *sText::DummyString = "";

/****************************************************************************/

sText::sText()
{
  Text = DummyString;
  Alloc = 0;
}

/****************************************************************************/

sText::~sText()
{
  if(Text!=DummyString)
    delete[] Text;
}

/****************************************************************************/

void sText::Init(sChar *text,sInt len)
{
  sInt size;
  if(len!=-1)
    size = len+1;
  else
    size = sGetStringLen(text)+1;

  if(size>Alloc)
  {
    Alloc = size;
    if(Text!=DummyString)
      delete[] Text;
    Text = new sChar[size];
  }
  sCopyMem(Text,text,size-1);
  Text[size-1] = 0;
}

/****************************************************************************/

void sText::Clear()
{
  if(Text[0]!=0)
    Text[0] = 0;
}

/****************************************************************************/

void sText::Copy(sObject *o)
{
  sVERIFY(o->GetClass()==sCID_TEXT);
  Init(((sText *)o)->Text);
}

/****************************************************************************/

sBool sText::Write(sU32 *&p)
{
  sInt size;
  size = sGetStringLen(Text)+1;
  p[size/4]=0;
  sCopyMem(p,Text,size);
  p+=sAlign(size,4);
  return sTRUE;
}

/****************************************************************************/

sBool sText::Read(sU32 *&p)
{
  Init((sChar *)p);
  while((*p++)&0xff000000) ;
  return sTRUE;
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   MusicPlayers                                                       ***/
/***                                                                      ***/
/****************************************************************************/

#if !sINTRO

sMusicPlayer::sMusicPlayer()
{
  Stream = 0;
  StreamSize = 0;
  StreamDelete = 0;
  Status = 0;
  RewindBuffer = 0;
  RewindSize = 0;
  RewindPos = 0;
  PlayPos = 0;
}

sMusicPlayer::~sMusicPlayer()
{
  if(StreamDelete)
    delete[] Stream;
  if(RewindBuffer)
    delete[] RewindBuffer;
}


sBool sMusicPlayer::Load(sChar *name)
{
  sInt size;

  if(StreamDelete)
    delete[] Stream;
  Stream = sSystem->LoadFile(name,size);
  StreamSize = size;
  StreamDelete = sFALSE;
  if(Stream)
  {
    StreamDelete = sTRUE;
    Status = 1;
    return sTRUE;
  }
  return sFALSE;
}

sBool sMusicPlayer::Load(sU8 *data,sInt size)
{
  if(StreamDelete)
    delete[] Stream;
  Stream = data;
  StreamSize = size;
  StreamDelete = sFALSE;
  Status = 1;
  return sTRUE;
}

void sMusicPlayer::AllocRewind(sInt bytes)
{
  RewindSize = bytes/4;
  RewindBuffer = new sS16[bytes/2];
  RewindPos = 0;
}

sBool sMusicPlayer::Start(sInt songnr)
{
  if(Status==1)
  {
    if(Init(songnr))
      Status = 3;
    else
      Status = 0;
  }
  return Status==3;
}

sBool sMusicPlayer::Handler(sS16 *buffer,sInt samples)
{
  sInt diff,size;
  sInt result;

  result = 1;
  if(Status==3)
  {
    if(RewindBuffer)
    {
      if(PlayPos+samples < RewindSize)
      {
        while(RewindPos<PlayPos+samples && result)
        {
          diff = PlayPos+samples-RewindPos;
          size = Render(RewindBuffer+RewindPos*2,diff);
          if(size==0) 
            result = 0;
          RewindPos+=size;
        }
        size = samples;
        if(RewindPos+size>RewindSize)
          size = RewindSize-RewindPos;
        if(size>0)
        {
          sCopyMem(buffer,RewindBuffer+PlayPos*2,size*4);
          buffer+=size*2;
          PlayPos += size;
          samples -= size;
        }
      }
      if(samples>0)
      {
        sSetMem(buffer,0,samples*4);
      }
    }
    else
    {
      while(samples>0 && result)
      {
        size = Render(buffer,samples);
        if(size==0)
          result = 0;
        buffer+=size*2;
        PlayPos += size;
        samples-=size;
      }
    }
  }
  else
  {
    sSetMem(buffer,0,samples*4);
  }
  return result;
}

#endif

/****************************************************************************/




