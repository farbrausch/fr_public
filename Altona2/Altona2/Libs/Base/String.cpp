/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***   Basic String Operations                                            ***/
/***                                                                      ***/
/****************************************************************************/

void sCopyString(char *d,const char *s,uptr size)
{
    size--;
    while(size>0 && *s)
    {
        size--;
        *d++ = *s++;
    }
    *d = 0;
}

void sCopyString(const sStringDesc &desc,const char *s)
{
    uptr size = desc.Size;
    char *d = desc.Buffer;

    size--;
    while(size>0 && *s)
    {
        size--;
        *d++ = *s++;
    }
    *d = 0;
}

void sCopyStringLen(const sStringDesc &desc,const char *s,uptr len)
{
    uptr size = sMin(desc.Size,len+1);
    char *d = desc.Buffer;

    size--;
    while(size>0 && *s)
    {
        size--;
        *d++ = *s++;
    }
    *d = 0;
}

/****************************************************************************/

int sCmpString(const char *a,const char *b)
{
    int aa,bb;
    do
    {
        aa = *a++;
        bb = *b++;
    }
    while(aa!=0 && aa==bb);
    return sSign(aa-bb);
}

int sCmpStringI(const char *a,const char *b)
{
    int aa,bb;
    do
    {
        aa = *a++; if(aa>='a' && aa<='z') aa=aa-'a'+'A';
        bb = *b++; if(bb>='a' && bb<='z') bb=bb-'z'+'Z';
    }
    while(aa!=0 && aa==bb);
    return sSign(aa-bb);
}

int sCmpStringP(const char *a,const char *b)
{
    int aa,bb;
    do
    {
        aa = *a++; if(aa>='A' && aa<='Z') aa=aa-'A'+'a'; if(aa=='\\') aa='/';
        bb = *b++; if(bb>='A' && bb<='Z') bb=bb-'A'+'a'; if(bb=='\\') bb='/';
    }
    while(aa!=0 && aa==bb);
    return sSign(aa-bb);
}

int sCmpStringLen(const char *a,const char *b,uptr len)
{
    int aa,bb;
    do
    {
        if(len--==0) return 0;
        aa = *a++; 
        bb = *b++; 
    }
    while(aa!=0 && aa==bb);
    return sSign(aa-bb);
}

int sCmpStringILen(const char *a,const char *b,uptr len)
{
    int aa,bb;
    do
    {
        if(len--==0) return 0;
        aa = *a++; if(aa>='a' && aa<='z') aa=aa-'a'+'A';
        bb = *b++; if(bb>='a' && bb<='z') bb=bb-'z'+'Z';
    }
    while(aa!=0 && aa==bb);
    return sSign(aa-bb);
}

int sCmpStringPLen(const char *a,const char *b,uptr len)
{
    int aa,bb;
    do
    {
        if(len--==0) return 0;
        aa = *a++; if(aa>='A' && aa<='Z') aa=aa-'A'+'a'; if(aa=='\\') aa='/';
        bb = *b++; if(bb>='A' && bb<='Z') bb=bb-'A'+'a'; if(bb=='\\') bb='/';
    }
    while(aa!=0 && aa==bb);
    return sSign(aa-bb);
}

/****************************************************************************/

void sAppendString(char *d,const char *s,uptr size)
{
    size--;
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

void sAppendStringLen(char *d,const char *s,uptr size,uptr len)
{
    size--;
    while(size>0 && *d)
    {
        size--;
        d++;
    }
    while(size>0 && len>0 && *s)
    {
        size--;
        len--;
        *d++ = *s++;
    }
    *d = 0;
}

/****************************************************************************/

char sUpperChar(char c)
{
    if(c>=0xe0 && c<=0xfd && c!=247) return c-0x20;
    if(c>='a'  && c<='z'           ) return c-0x20;
    return c;
}

char sLowerChar(char c)
{
    if(c>=0xc0 && c<=0xdd && c!=215) return c+0x20;
    if(c>='A'  && c<='Z'           ) return c+0x20;
    return c;
}

/****************************************************************************/

const char *sAllocString(const char *s)
{
    uptr size = sGetStringLen(s)+1;
    char *d = new char[size];
    sCopyMem(d,s,size*sizeof(char));
    return d;
}

/****************************************************************************/

int sFindFirstChar(const char *str,char c)
{
    for(int i=0;str[i];i++)
        if(str[i]==c)
            return i;
    return -1;
}

/****************************************************************************/

int sFindFirstString(const char *str,const char *find)
{
	int ret = -1;
	int i = 0;
    int j = 0;
    while (str[i] && find[j])
    {
        bool e = str[i]==find[j];
        if (e)
        {
        	if (j==0) ret = i;
            j++;
        }
        else
        {
            j = 0;
            ret = -1;
        }
		i++;
    }
    if (find[j]!=0) ret = -1;
    return ret;    
}

    
/****************************************************************************/
/***                                                                      ***/
/***   String Operations for working with file system paths               ***/
/***                                                                      ***/
/****************************************************************************/

void sCleanPath(const sStringDesc &str)
{
    char *s,*d;

    // convert all '\' to '/'

    int i = 0;
    s = str.Buffer;
    while(s[i])
    {
        if(s[i]=='\\')
            s[i] = '/';
        i++;
    }

    // convert multiple '/' to a singe '/', in case strings got wrongly concatenated

    s = d = str.Buffer;
    while(*s)
    {
        if(s[0]=='/' && s[1]=='/') 
            s++;
        else
            *d++ = *s++;
    }
    *d++ = 0;

    // convert '/./' to '/'

    s = d = str.Buffer;
    while(*s)
    {
        if(s[0]=='/' && s[1]=='.' && s[2]=='/') 
            s+=2;
        else
            *d++ = *s++;
    }
    *d++ = 0;

    // convert '/../' into a form that removes a previous directory

    s = d = str.Buffer;
    while(*s)
    {
        if(s[0]=='/' && s[1]=='.' && s[2]=='.' && s[3]=='/') 
        {
            s+=3;
            while(d-1>=str.Buffer && d[-1]!='/')
                d--;
        }
        else
        {
            *d++ = *s++;
        }
    }
    *d++ = 0;
}

const char *sExtractLastSuffix(const char *s)
{
    const char *r = 0;

    while(*s)
    {
        if(*s=='.')
            r = s+1;
        s++;
    }
    if(r==0) r = s;

    return r;
}

const char *sExtractAllSuffix(const char *s)
{
    while(*s)
    {
        if(*s=='.') return s+1;
        s++;
    }
    return s;
}

const char *sExtractName(const char *s)
{
    const char *r = s;

    while(*s)
    {
        if(*s=='/' || *s=='\\')
            r = s+1;
        s++;
    }

    return r;
}

void sRemoveLastSuffix(const sStringDesc &str)
{
    char *r = 0;
    char *s = str.Buffer;

    while(*s)
    {
        if(*s=='.')
            r = s;
        s++;
    }
    if(r) *r = 0;
}

void sRemoveAllSuffix(const sStringDesc &str)
{
    char *s = str.Buffer;

    while(*s)
    {
        if(*s=='.')
        {
            *s = 0;
            break;
        }
        s++;
    }
}

void sRemoveName(const sStringDesc &str)
{
    char *r = 0;
    char *s = str.Buffer;

    while(*s)
    {
        if(*s=='/')
            r = s;
        s++;
    }
    if(r) *r = 0;
    else if (str.Buffer) str.Buffer[0]=0;
}

bool sMakeRelativePath(const sStringDesc &out,const char *fromdir,const char *tofile)
{
    sString<sMaxPath> fb,tb,o;
    fb = fromdir;
    tb = tofile;
    sCleanPath(fb);
    sCleanPath(tb);
    const char *f = fb;
    const char *t = tb;

    o = "";
    sCopyString(out,o);

    if(*f!=*t)
        return 0;
    while(*f && *t && *f==*t) 
    {
        f++;
        t++;
    }
    if(*t=='/') t++;
    if(*t==0)
        return 0;
    while(*f)
    {
        while(*f!='/' && *f!=0)
            f++;
        if(*f=='/')
            f++;
        o.Add("../");
    }
    o.Add(t);
    sCopyString(out,o);
    return 1;
}


/****************************************************************************/
/***                                                                      ***/
/***   UTF-8 Parsing and Synthesis                                        ***/
/***                                                                      ***/
/****************************************************************************/

int sReadUTF8(const char *&s_)
{
    int val = 0;
    const char *s = s_;
    if((*s & 0x80)==0x00)
    {
        val = s[0];
        s+=1;
    }
    else if((*s & 0xc0)==0x80)
    {
        val = '?';       // continuation byte without prefix code
        s+=1;
    }
    else if((*s & 0xe0)==0xc0)
    {
        if((s[1]&0xc0)==0x80)
        {
            val = ((s[0]&0x1f)<<6)|(s[1]&0x3f);
            s+=2;
        }
        else
        {
            val = '?';      // eof or invalid continuation
            s+=1;
        }
    }
    else if((*s & 0xf0)==0xe0)
    {
        if((s[1]&0xc0)==0x80 && (s[2]&0xc0)==0x80)
        {
            val = ((s[0]&0x0f)<<12)|((s[1]&0x3f)<<6)|(s[2]&0x3f);
            s+=3;
        }
        else
        {
            val= '?';      // eof or invalid continuation
            s+=1;
        }
    }
    else if((*s & 0xf8)==0xf0)
    {
        if((s[1]&0xc0)==0x80 && (s[2]&0xc0)==0x80 && (s[3]&0xc0)==0x80)
        {
            val = ((s[0]&0x07)<<18)|((s[1]&0x3f)<<12)|((s[2]&0x3f)<<6)|(s[3]&0x3f);
            s+=4;
        }
        else
        {
            val= '?';      // eof or invalid continuation
            s+=1;
        }
    }
    else if((*s & 0xfc)==0xf8)
    {
        if((s[1]&0xc0)==0x80 && (s[2]&0xc0)==0x80 && (s[3]&0xc0)==0x80 && (s[4]&0xc0)==0x80)
        {
            val = ((s[0]&0x03)<<24)|((s[1]&0x3f)<<18)|((s[2]&0x3f)<<12)|((s[3]&0x3f)<<6)|(s[4]&0x3f);
            s+=5;
        }
        else
        {
            val= '?';      // eof or invalid continuation
            s+=1;
        }
    }
    else if((*s & 0xfe)==0xfc)
    {
        if((s[1]&0xc0)==0x80 && (s[2]&0xc0)==0x80 && (s[3]&0xc0)==0x80 && (s[4]&0xc0)==0x80 && (s[5]&0xc0)==0x80)
        {
            val = ((s[0]&0x01)<<30)|((s[1]&0x3f)<<24)|((s[1]&0x3f)<<18)|((s[2]&0x3f)<<12)|((s[3]&0x3f)<<6)|(s[4]&0x3f);
            s+=6;
        }
        else
        {
            val= '?';      // eof or invalid continuation
            s+=1;
        }
    }
    else
    {
        val = '?';       // invalid prefix code
        s+=1;
    }

    s_ = s;

    return val;
}

void sWriteUTF8(char *&d_,uint c)
{
    char *d = d_;

    if(c<0x80)
    {
        *d++ = c;
    }
    else if(c<0x00000800)
    {
        *d++ = 0xc0 | ((c>> 6)&0x1f);
        *d++ = 0x80 | ( c     &0x3f);
    } 
    else if(c<0x00010000)
    {
        *d++ = 0xe0 | ((c>>12)&0x0f);
        *d++ = 0x80 | ((c>> 6)&0x3f);
        *d++ = 0x80 | ( c     &0x3f);
    }
    else if(c<0x00200000)
    {
        *d++ = 0xf0 | ((c>>18)&0x07);
        *d++ = 0x80 | ((c>>12)&0x3f);
        *d++ = 0x80 | ((c>> 6)&0x3f);
        *d++ = 0x80 | ( c     &0x3f);
    }
    else if(c<0x04000000)
    {
        *d++ = 0xf8 | ((c>>24)&0x03);
        *d++ = 0x80 | ((c>>18)&0x3f);
        *d++ = 0x80 | ((c>>12)&0x3f);
        *d++ = 0x80 | ((c>> 6)&0x3f);
        *d++ = 0x80 | ( c     &0x3f);
    }
    else if(c<0x80000000)
    {
        *d++ = 0xfc | ((c>>30)&0x01);
        *d++ = 0x80 | ((c>>24)&0x3f);
        *d++ = 0x80 | ((c>>18)&0x3f);
        *d++ = 0x80 | ((c>>12)&0x3f);
        *d++ = 0x80 | ((c>> 6)&0x3f);
        *d++ = 0x80 | ( c     &0x3f);
    }
    else
    {
        *d++ = '?';
    }

    d_ = d;
}

/****************************************************************************/

bool sUTF8toUTF16(const char *s_,uint16 *d,int buffersize,uptr *bufferused)
{
    const char *s = s_;
    return sUTF8toUTF16Scan(s,d,buffersize,bufferused);
}

bool sUTF8toUTF16Scan(const char *&s_,uint16 *d,int buffersize,uptr *bufferused)
{
    const char *s = s_;
    uint16 *start = d;
    uint16 *end = d+buffersize-3;

    while(*s && d<end)
    {
        int val = sReadUTF8(s);
        if(val<0x10000)
        {
            if(val>=0xd800 && val<0xe000)
                *d++ = '?';
            else
                *d++ = val;
        }
        else
        {
            val -= 0x10000;
            *d++ = 0xd800 | ((val>>10) & 0x3ff);
            *d++ = 0xdc00 | ( val      & 0x3ff);
        }
    }

    if(bufferused)
        *bufferused = d-start;
    *d++ = 0;

    s_ = s;
    return *s==0;
}


char *sUTF16toUTF8(const uint16 *s)
{
    int len = 0;
    for(int i=0;s[i];i++)
    {
        if(s[i]<0x80)
        {
            len+=1;
        }
        else if(s[i]<0x800)
        {
            len+=2;
        }
        else
        {
            if(s[i]>=0xd800 && s[i]<0xe000)
            {
                len+=1;     // lame! just add '?'
            }
            else
            {
                len+=3;
            }
        }
    }

    char *start = new char[len+1];
    char *d = start;

    for(int i=0;s[i];i++)
        sWriteUTF8(d,s[i]);

    sASSERT(start+len==d);
    *d++ = 0;

    return start;
}

bool sUTF16toUTF8(const uint16 *s,const sStringDesc &dest)
{
    char *d = dest.Buffer;
    char *end = d+dest.Size-7;

    while(*s && d<end)
        sWriteUTF8(d,*s++);

    sASSERT(d<end);
    *d++ = 0;

    return *s==0;
}

/****************************************************************************/
/***                                                                      ***/
/***   Numeric Conversions                                                ***/
/***                                                                      ***/
/****************************************************************************/

void sFloatInfo::FloatToAscii(uint fu,int digits)
{
    int e2 = ((fu&0x7f800000)>>23);
    uint man = fu & 0x007fffff;
    int e10 = 0;
    const int shift=28;

    Digits[0] = 0;
    Negative = (fu&0x80000000) ? 1 : 0;
    NaN = 0;
    Exponent = 0;
    Infinite = 0;
    Denormal = 0;
    if(e2==0)
    {
        if(man==0)
        {
            for(int i=0;i<digits;i++)
                Digits[i]='0';
            Digits[digits] = 0;
            return;
        }
        Denormal = 1;
    }
    else if(e2==255)
    {
        if(man==0)
            Infinite = 1;
        else
            NaN = man;
        return;
    }

    if(!Denormal) man |= 0x00800000;
    man = man<<(shift-23); 
    man += 1UL<<(shift-24); 
    e2 = e2 - 127;


    // keep mantissa from 0x10000000 .. 0x9fffffff

    // <1

    while(e2<=-10)
    {
        man = sMulShiftU(man,1000*(1<<6));   // x*1000/1024 = x*1000*(65536/1024)/65536
        e10 -= 3;
        e2 += 10;
        if(man<(1ULL<<(shift)))
        {
            man = man*10;
            e10--;
        }
    }

    while(e2<=-1)
    {
        if(man<(1ULL<<(shift+1)))
        {
            man *= 5;
            e10--;
        }
        else
        {
            man = man>>1;
        }
        e2++;
    }

    // >1

    while(e2>=10)
    {
        man = sMulDivU(man,1024,1000);
        e10+=3;
        e2-=10;
        if(man>=(10UL<<shift))
        {
            man = man/10;
            e10++;
        }
    }

    while(e2>=1)
    {
        if(man>=(5UL<<shift))
        {
            man = man/5;
            e10++;
        }
        else
        {
            man = man<<1;
        }
        e2--;
    }

    // output digits

    //man += 1UL<<(shift-25);
    for(int i=0;i<digits;i++)
    {
        Digits[i]= '0'+(man>>shift);
        man = (man&((1UL<<shift)-1));
        man = (man<<3) + (man<<1);
    }

    // done

    Digits[digits] = 0;
    Exponent = e10;
}

void sFloatInfo::DoubleToAscii(uint64 fu,int digits)
{
    int e2 = ((fu&0x7ff0000000000000ULL)>>52);
    uint64 man = fu &0x000fffffffffffffULL;
    int e10 = 0;
    const int shift=60;

    Digits[0] = 0;
    Negative = (fu&0x8000000000000000ULL) ? 1 : 0;
    NaN = 0;
    Exponent = 0;
    Infinite = 0;
    Denormal = 0;
    if(e2==0)
    {
        if(man==0)
        {
            for(int i=0;i<digits;i++)
                Digits[i]='0';
            Digits[digits] = 0;
            return;
        }
        Denormal = 1;
    }
    else if(e2==2047)
    {
        if(man==0)
            Infinite = 1;
        else
            NaN = man;
        return;
    }

    if(!Denormal) man |= 0x0010000000000000ULL;
    man = man<<(shift-52); 
    man += 1ULL<<(shift-53); 
    e2 = e2 - 1023;


    // keep mantissa from 0x10000000 .. 0x9fffffff

    // <1

    while(e2<=-1)
    {
        if(man<(1ULL<<(shift+1)))
        {
            man *= 5;
            e10--;
        }
        else
        {
            man = man>>1;
        }
        e2++;
    }

    // >1

    while(e2>=1)
    {
        if(man>=(5ULL<<shift))
        {
            man = man/5;
            e10++;
        }
        else
        {
            man = man<<1;
        }
        e2--;
    }

    // output digits

    //man += 1UL<<(shift-54);
    for(int i=0;i<digits;i++)
    {
        Digits[i]= '0'+(man>>shift);
        man = (man&((1ULL<<shift)-1));
        man = man*10;
    }

    // done

    Digits[digits] = 0;
    Exponent = e10;
}

uint sFloatInfo::AsciiToFloat()
{
    uint64 man;
    int e10;
    int e2;
    const int shift=60;
    const uint64 highmask = ~((1ULL<<shift)-1);
    const uint64 highmask1 = ~((1ULL<<(shift-1))-1);
    const uint64 highmask0 = ~((1ULL<<(shift+1))-1);
    uint fu;

    // simple cases

    fu = Negative ? 0x80000000 : 0x00000000;
    if(NaN)
    {
        fu |= 0x7f800000;
        fu |= 0x007fffff & NaN;
        goto ende;
    }
    if(Infinite)
    {
        fu |= 0x7f800000;
        goto ende;
    }

    // scan digits

    man = 0;
    e10 = Exponent+1;
    e2 = shift;
    for(int i=0;Digits[i];i++)
    {
        if(man>0x1000000000000000ULL)   // we don't really need more digits
            break;
        man = man*10;
        man += Digits[i]&15;
        e10--;
    }
    if(man==0)
    {
        e2 = 0;
        goto ende;
    }

    // normalize for max precision during e10 passes

    while((man & highmask1)==0)
    {
        man = man<<1;
        e2--;
    }

    // get rid of e10

    while(e10<-6)
    {
        man = (man+500000)/1000000;
        while((man & highmask)==0)
        {
            man = man<<1;
            e2--;
        }
        e10+=6;
    }
    while(e10<0)
    {
        man = (man+5)/10;
        while((man & highmask)==0)
        {
            man = man<<1;
            e2--;
        }
        e10++;
    }

    if(e10>40)
    {
        e2 = 999;
    }
    else
    {
        while(e10>0)
        {
            man = man*10;
            while((man & highmask)!=0)
            {
                man = man>>1;
                e2++;
            }
            e10--;
        }
    }

    // normalize for 23 bits

    if(man!=0)
    {
        while(highmask0 & man)
        {
            man = man>>1;
            e2++;
        }
        while(!(highmask & man))
        {
            man = man<<1;
            e2--;
        }
    }

    // denormals

    e2 += 127;
    if(e2<-23)
    {
        man = 0;
        e2 = 0;
    }
    while(e2<0)
    {
        e2++;
        man = man>>1;
    }


    // infinity

    if(e2>255)
    {
        e2 = 255;
        man = 0;
    }

    // puzzle

    fu |= 0x7f800000 & (e2<<23);
    fu |= 0x007fffff & ((man+((1ULL<<(shift-24))-1))>>(shift-23));

ende:

    return fu;
}

uint64 sFloatInfo::AsciiToDouble()
{
    uint64 man;
    int e10;
    int e2;
    const int shift=60;
    const uint64 highmask = ~((1ULL<<shift)-1);
    const uint64 highmask1 = ~((1ULL<<(shift-1))-1);
    const uint64 highmask0 = ~((1ULL<<(shift+1))-1);
    uint64 fu;

    // simple cases

    fu = Negative ? 0x8000000000000000ULL : 0;
    if(NaN)
    {
        fu |= 0x7ff0000000000000ULL;
        fu |= 0x000fffffffffffffULL & NaN;
        goto ende;
    }
    if(Infinite)
    {
        fu |= 0x7ff0000000000000ULL;
        goto ende;
    }

    // scan digits

    man = 0;
    e10 = Exponent+1;
    e2 = shift;
    for(int i=0;Digits[i];i++)
    {
        if(man>(0x1000000000000000ULL/10))   // we don't really need more digits
            break;
        man = man*10;
        man += Digits[i]&15;
        e10--;
    }
    if(man==0)
    {
        e2 = 0;
        goto ende;
    }

    // normalize for max precision during e10 passes

    while((man & highmask1)==0)
    {
        man = man<<1;
        e2--;
    }

    // get rid of e10

    while(e10<0)
    {
        man = (man+5)/10;
        while((man & highmask)==0)
        {
            man = man<<1;
            e2--;
        }
        e10++;
    }

    while(e10>0)
    {
        man = man*10;
        while((man & highmask)!=0)
        {
            man = man>>1;
            e2++;
        }
        e10--;
    }

    // normalize for 23 bits

    if(man!=0)
    {
        while(highmask0 & man)
        {
            man = man>>1;
            e2++;
        }
        while(!(highmask & man))
        {
            man = man<<1;
            e2--;
        }
    }

    // denormals

    e2 += 1023;
    if(e2<-52)
    {
        man = 0;
        e2 = 0;
    }
    while(e2<0)
    {
        e2++;
        man = man>>1;
    }


    // infinity

    if(e2>2047)
    {
        e2 = 2047;
        man = 0;
    }

    // puzzle

    fu |= 0x7ff0000000000000ULL & (uint64(e2)<<52);
    fu |= 0x000fffffffffffffULL & ((man+((1ULL<<(shift-53))-1))>>(shift-52));

ende:

    return fu;
}

/****************************************************************************/

void sFloatInfo::PrintE(const sStringDesc &desc,int fractions)
{
    if(Infinite)
    {
        sSPrintF(desc,"%s#inf",Negative?"-":"");
    }
    else if(NaN)
    {
        sSPrintF(desc,"%s#nan%d",Negative?"-":"",NaN);
    }
    else
    {
        sFloatInfo fi = *this;
        fi.Exponent = 0;
        sString<64> buffer;
        fi.PrintF(buffer,fractions);
        sSPrintF(desc,"%s%se%d",Negative?"-":"",buffer,Exponent);
    }
}

void sFloatInfo::PrintF(const sStringDesc &desc,int fractions)
{
    if(Infinite || NaN /*|| Exponent>fractions*/ || Exponent<-fractions)
    {
        PrintE(desc,fractions);
        return;
    }

    char *s = desc.Buffer;
    char *e = desc.Buffer+desc.Size-1;

    int exp = Exponent+1;
    int dig = 0;
    if(Negative)
        *s++ = '-';
    char *sx = s;



    while(exp>0 && Digits[dig] && s<e)
    {
        *s++ = Digits[dig++];
        exp--;
    }
    while(exp>0 && s<e)
    {
        *s++ = '0';
        exp--;
    }
    if(fractions>0 && s<e)
    {    
        if(s==sx)
            *s++ = '0';
        if(s<e)
        {
            *s++ = '.';
            int frac = fractions;
            while(exp<0 && s<e && frac>0)
            {
                *s++ = '0';
                exp++;
                frac--;
            }
            while(frac>0 && Digits[dig] && s<e)
            {
                *s++ = Digits[dig++];
                frac--;
            }
            if(frac==0 && Digits[dig]>='5')  //aufrunden. gefährlich...
            {
                for(char *d = s-1;d>=sx;d--)
                {
                    if(*d!='.')
                    {
                        *d = *d + 1;
                        if(*d == '0'+10)
                            *d = '0';
                        else
                            goto round_done;
                    }
                }
                for(char *d = s-1;d>=sx;d--)
                    d[1] = d[0];
                *sx = '1';
                s++;
round_done:;
            }
            while(frac>0 && s<e)
            {
                *s++ = '0';
                frac--;
            }
        }
    }
    if(s<e)
        *s++ = 0;
    else
        sCopyString(desc,"###");
}

/****************************************************************************/

bool sFloatInfo::Scan(const char *&scan_)
{
    const char *scan = scan_;

    int n = 0;
    Digits[0] = 0;
    Exponent = -1;
    NaN = 0;
    Negative = 0;
    Denormal = 0;
    Infinite = 0;
    bool ok = 1;

    if(*scan=='-')
    {
        Negative = 1;
        scan++;
    }
    while(sIsDigit(*scan) && n<sCOUNTOF(Digits))
    {
        Digits[n++] = *scan;
        scan++;
        Exponent++;
    }
    if(*scan=='.')
    {
        scan++;
        while(sIsDigit(*scan) && n<sCOUNTOF(Digits))
        {
            Digits[n++] = *scan;
            scan++;
        }
    }
    if(*scan=='e' || *scan=='E')
    {
        scan++;
        int val = 0;
        if(!sScanValue(scan,val))
            ok = 0;
        Exponent += val;
    }

    if(n>=sCOUNTOF(Digits))
        n = 0;
    Digits[n] = 0;
    if(n==0)
        ok = 0;

    scan_ = scan;
    return ok;
}

/****************************************************************************/
/***                                                                      ***/
/***   Integer Scanning                                                   ***/
/***                                                                      ***/
/****************************************************************************/

bool sScanValue(const char *&s,int &result)
{
    uint val = 0;
    bool sign = 0;
    int overflow = 0;
    uint digit;

    // scan sign

    if(*s=='-')
    {
        s++;
        sign = 1;
    }
    else if(*s=='+')
    {
        s++;
    }

    // scan value

    if(!sIsDigit(*s))
        return 0;
    while(sIsDigit(*s))
    {
        digit = ((*s++)&15);
        if(val>(0x80000000-digit)/10)
            overflow = 1;
        val = val * 10 + digit;
    }
    if(overflow)
        return 0;

    // apply sign and write out

    if(sign)
    {
        result = -(int)val;
    }
    else
    {
        if(val>0x7fffffff)
            return 0;
        result = val;
    }

    // done

    return 1;
}

bool sScanValue(const char *&s,uint8 &result)
{
    int val = result;
    bool ok = sScanValue(s,val);
    result = val;
    return ok;
}
bool sScanValue(const char *&s,int8 &result)
{
    int val = result;
    bool ok = sScanValue(s,val);
    result = val;
    return ok;
}
bool sScanValue(const char *&s,uint16 &result)
{
    int val = result;
    bool ok = sScanValue(s,val);
    result = val;
    return ok;
}
bool sScanValue(const char *&s,int16 &result)
{
    int val = result;
    bool ok = sScanValue(s,val);
    result = val;
    return ok;
}


bool sScanValue(const char *&s,int64 &result)
{
    uint64 val = 0;
    bool sign = 0;
    int overflow = 0;
    uint64 digit;

    // scan sign

    if(*s=='-')
    {
        s++;
        sign = 1;
    }
    else if(*s=='+')
    {
        s++;
    }

    // scan value

    if(!sIsDigit(*s))
        return 0;
    while(sIsDigit(*s))
    {
        digit = ((*s++)&15);
        if(val>(0x8000000000000000LL-digit)/10)
            overflow = 1;
        val = val * 10 + digit;
    }
    if(overflow)
        return 0;

    // apply sign and write out

    if(sign)
    {
        result = -(int)val;
    }
    else
    {
        if(val>0x7fffffffffffffff)
            return 0;
        result = val;
    }

    // done

    return 1;
}


bool sScanValue(const char *&s,uint &result)
{
    uint val = 0;
    int overflow = 0;
    uint digit;

    // scan value

    if(!sIsDigit(*s))
        return 0;
    while(sIsDigit(*s))
    {
        digit = ((*s++)&15);
        if(val>(0xffffffffU-digit)/10)
            overflow = 1;
        val = val * 10 + digit;
    }
    if(overflow)
        return 0;

    result = val;

    // done

    return 1;
}

bool sScanValue(const char *&s,uint64 &result)
{
    uint64 val = 0;
    int overflow = 0;
    uint64 digit;

    // scan value

    if(!sIsDigit(*s))
        return 0;
    while(sIsDigit(*s))
    {
        digit = ((*s++)&15);
        if(val>(0xffffffffffffffffULL-digit)/10)
            overflow = 1;
        val = val * 10 + digit;
    }
    if(overflow)
        return 0;

    result = val;

    // done

    return 1;
}

/****************************************************************************/

bool sScanHex(const char *&s,int &result, int maxlen)
{
    uint val;
    int c;

    if(!sIsHex(*s)) return 0;

    if(s[0]=='0' && s[1]=='x')      // skip leading 0x
        s+=2;

    val = 0;
    while(sIsHex(*s))
    {
        c = *s++;
        val = val * 16;

        if(c>='0' && c<='9') val += c-'0';
        if(c>='a' && c<='f') val += c-'a'+10;
        if(c>='A' && c<='F') val += c-'A'+10;

        if (!--maxlen) break;
    }

    result = val;
    return 1;
}

bool sScanValue(const char *&scan,float &val)
{
    sFloatInfo fi;
    bool r=fi.Scan(scan);
    *(uint *)(&val) = fi.AsciiToFloat();
    return r;
}

bool sScanValue(const char *&scan,double &val)
{
    sFloatInfo fi;
    bool r=fi.Scan(scan);
    *(uint64 *)(&val) = fi.AsciiToDouble();
    return r;
}

bool sScanHexColor(const char *&scan,uint &val)
{
    const char *start = scan;

    uint hex = 0;
    int digits = 0;
    while(sIsHex(*scan))
    {
        hex = hex*16 + sGetHex(*scan++);
        digits ++;
    }
    if(digits<=4)
    {
        hex = ((hex&0x000f)>> 0)
            | ((hex&0x00f0)<< 4)
            | ((hex&0x0f00)<< 8)
            | ((hex&0xf000)<<12);
        hex = hex | (hex<<4);
        digits*=2;
    }
    switch(digits)
    {
    case 2: val = hex|(hex<<8)|(hex<<16)|0xff000000; break;
    case 6: val = hex|0xff000000; break;
    case 8: val = hex; break;
    default: scan = start; return false;
    }
    return true;
}

/****************************************************************************/
/***                                                                      ***/
/***   Formatted Printing                                                 ***/
/***                                                                      ***/
/****************************************************************************/

bool sFormatStringBuffer::Fill()
{
loop:
    while(*Format!='%' && *Format!=0 && Dest<End-1)
        *Dest++ = *Format++;
    if(Format[0]=='%' && Format[1]=='%' && Dest<End-1)
    {
        *Dest++ = '%';
        Format+=2;
        goto loop;
    }

    sASSERT(Dest<End);

    if(*Format==0 || *Format=='%')
    {
        Dest[0] = 0;
        return 1;
    }
    else
    {
        if(Dest==End-1)
            Dest--;
        *Dest++ = '?';          // the format string has not yet been consumed!
        *Dest = 0;
        Format = "";
        return 0;
    }
}

void sFormatStringBuffer::GetInfo(sFormatStringInfo &info)
{
    int c;

    info.Format = 0;
    info.Field = 0;
    info.Fraction = -1;
    info.Minus = 0;
    info.Null = 0;
    info.Truncate = 0;

    if(!Fill())
        return;

    sASSERT(*Format=='%');
    Format++;


    c = *Format++;
    if (c=='!')
    {
        info.Truncate = 1;
        c = *Format++;
    }
    if(c=='-')
    {
        info.Minus = 1;
        c = *Format++;
    }
    if(c=='0')
    {
        info.Null = 1;
    }
    while(c>='0' && c<='9')
    {
        info.Field = info.Field*10 + c - '0';
        c = *Format++;
    }
    if(c=='.')
    {
        info.Fraction=0;
        c = *Format++;
        while(c>='0' && c<='9')
        {
            info.Fraction = info.Fraction*10 + c - '0';
            c = *Format++;
        }
    }
    if(c!=0)
        info.Format = c;

    sASSERT(info.Format);
}

void sFormatStringBuffer::Add(const sFormatStringInfo &info,const char *buffer,bool sign)
{
    sptr len = sGetStringLen(buffer);
    sptr field = info.Field;

    if(!info.Minus && !info.Null)
    {
        while(field>(sign?len+1:len) && Dest<End-1)
        {
            *Dest++ = ' ';
            field--;
        }
    }
    if(sign && Dest<End-1)
    {
        *Dest++ = '-';
        field--;
    }
    if(info.Null)
    {
        while(field>len && Dest<End-1)
        {
            *Dest++ = '0';
            field--;
        }
    }
    if (info.Truncate)
    {
        while(len>0 && field>0 && Dest<End-1)
        {
            *Dest++ = *buffer++;
            len--;
            field--;
        }
    }
    else
    {
        while(len>0 && Dest<End-1)
        {
            *Dest++ = *buffer++;
            len--;
            field--;
        }
    }
    if(info.Minus)
    {
        while(field>len && Dest<End-1)
        {
            *Dest++ = ' ';
            field--;
        }
    }
}

void sFormatStringBuffer::Print(const char *str)
{
    while(Dest<End-1 && *str)
        *Dest++ = *str++;
}

template<typename Type>
void sFormatStringBuffer::PrintInt(const sFormatStringInfo &info,Type val,bool sign)
{
    char buf[32];
    static char hex[17] = "0123456789abcdef";
    static char HEX[17] = "0123456789ABCDEF";
    static char units[] = " kmgtpe";
    static char UNITS[] = " KMGTPE";
    int len=0,komma=0,unit=0;

    switch(info.Format)
    {
    case '_':
        if(!sign)
        {
            while(val>0 && Dest<End-1)
            {
                *Dest++ = ' ';
                val--;
            }
        }
        break;
    case 't':
        if(!sign)
        {
            while(val>0 && Dest<End-1)
            {
                *Dest++ = '\t';
                val--;
            }
        }
        break;

    case 'c':
        {
            char *bd = buf;
            sWriteUTF8(bd,uint(val));
            *bd=0;
            Add(info,buf,sign);
        } 
        break;

    case 'k':   // kilo mega tera
    case 'K':
        len = sCOUNTOF(buf);
        buf[--len] = 0;

        if (info.Format=='k')
        {
            // GCC warning: comparison is always false due to limited range of data type
            while (val>=1000*10 && unit<(sCOUNTOF(units)-1))
            {
                unit++;
                val=(val)/1000;
            }
            if (units[unit]!=' ') buf[--len] = units[unit];
        }
        else
        {
            // GCC warning: comparison is always false due to limited range of data type
            while (val>=1024*10 && unit<(sCOUNTOF(units)-1))
            {
                unit++;
                val=(val)/1024;
            }
            if (UNITS[unit]!=' ') buf[--len] = UNITS[unit];
        }

        do
        {
            buf[--len] = val%10+'0';
            val = val/10;
        }
        while(val!=0);
        Add(info,buf+len,sign);
        break;

    case 'x':
    case 'X':
        len = sCOUNTOF(buf);
        buf[--len] = 0;
        do
        {
            if(info.Format=='x')
                buf[--len] = hex[val&15];
            else
                buf[--len] = HEX[val&15];
            val = (val>>4);
        }
        while(val!=0);
        Add(info,buf+len,sign);
        break;

    case 'r':   // radis "12.34"
        len = sCOUNTOF(buf);
        buf[--len] = 0;
        komma = 0;

        do
        {
            buf[--len] = val%10+'0';
            val = val/10;
            komma++;
            if(komma == info.Fraction)
                buf[--len] = '.';
        }
        while(val!=0 || komma<info.Fraction+1);
        Add(info,buf+len,sign);
        break;

    case 'f':
    case 'e':
    case 'F':
    case 'E':
    case 'i':
    case 'd':
    default:
        len = sCOUNTOF(buf);
        buf[--len] = 0;

        do
        {
            buf[--len] = val%10+'0';
            val = val/10;
        }
        while(val!=0);
        Add(info,buf+len,sign);
        break;

    case 'h':     // 4h3d1w , a weak is 5 days, a day is 8 hours
        len = 0;
        if(val==0)
        {
            buf[len++] = '0';
        }
        else
        {
            if(val>39)
            {
                Type weeks = val / 40;
                val -= weeks*40;
                if(weeks>=10)
                {
                    buf[len++] = char(weeks/10+'0');
                    weeks = weeks % 10;
                }
                buf[len++] = char(weeks+'0');
                buf[len++] = 'w';
            }
            if(val>8)
            {
                Type days = val/8;
                val -= days*8;
                buf[len++] = char(days+'0');
                buf[len++] = 'd';
            }
            if(val>0)
            {
                buf[len++] = char(val+'0');
                buf[len++] = 'h';
            }
        }
        buf[len++] = 0;
        Add(info,buf,sign);
        break;
    }
}

void sFormatStringBuffer::PrintFloat(const sFormatStringInfo &info,float v)
{
    sFloatInfo fi;
    sString<256> buf;

    switch(info.Format)
    {
    case 'f':
    case 'e':
    case 'F':
    case 'E':
        {
            fi.FloatToAscii(sRawCast<uint,float>(v));
            bool sign = fi.Negative;
            fi.Negative = 0;

            int frac = info.Fraction;
            int field = info.Field;

            if(field==0)
                field = 7;
            if(frac==-1)
                frac = sClamp(field-2-fi.Exponent-sign,0,field-2);
            if(info.Format=='F' && frac>0)
                frac--;

            fi.PrintF(buf,frac);

            if(info.Format=='F' || info.Format=='G')     // special format: add trailing 'f'
            {
                bool dot = false;
                for(int n=0;buf[n];n++)
                    if(buf[n]=='.')
                        dot = true;
                if(!dot)
                    buf.Add(".0");
                buf.Add("f");
            }

            Add(info,buf,sign); 
        }
        break;

    default:
        if(v>=0)
            PrintInt(info,uint64(v),0);
        else
            PrintInt(info,uint64(-v),1);
        break;
    }
}

void sFormatStringBuffer::PrintFloat(const sFormatStringInfo &info,double v)
{
    sFloatInfo fi;
    sString<256> buf;

    fi.DoubleToAscii(sRawCast<uint64,double>(v));
    bool sign = fi.Negative;
    fi.Negative = 0;


    int frac = info.Fraction;
    int field = info.Field;

    if(field==0)
    {
        field = 7;
        frac = 3;
    }
    else
    {
        if(frac==-1) frac = sMax(0,field-2-fi.Exponent-sign);
    }

    fi.PrintF(buf,frac);

    Add(info,buf,sign);   
}

/****************************************************************************/
/****************************************************************************/

sNOINLINE sFormatStringBuffer sFormatStringBase(const sStringDesc &buffer,const char *format)
{
    sFormatStringBuffer buf;
    buf.Start = buffer.Buffer;
    buf.Dest = buffer.Buffer;
    buf.End = buffer.Buffer + buffer.Size;
    buf.Format = format;

    buf.Fill();

    return buf;
}

sFormatStringBuffer& operator% (sFormatStringBuffer &f,int val)
{
    if (!*f.Format)
        return f;

    bool sign=(val<0);
    if(sign) val = -val;

    sFormatStringInfo info;

    f.GetInfo(info);
    f.PrintInt<uint>(info,uint(val),sign);
    f.Fill();

    return f;
}

sFormatStringBuffer& operator% (sFormatStringBuffer &f,uint val)
{
    if (!*f.Format)
        return f;

    sFormatStringInfo info;

    f.GetInfo(info);
    f.PrintInt<uint>(info,val,0);
    f.Fill();

    return f;
}

sFormatStringBuffer& operator% (sFormatStringBuffer &f,uint64 val)
{
    if (!*f.Format)
        return f;

    sFormatStringInfo info;

    f.GetInfo(info);
    f.PrintInt<uint64>(info,val,0);
    f.Fill();

    return f;
}

sFormatStringBuffer& operator% (sFormatStringBuffer &f,int64 val)
{
    if (!*f.Format)
        return f;

    bool sign=(val<0);
    if(sign) val = -val;

    sFormatStringInfo info;

    f.GetInfo(info);
    f.PrintInt<uint64>(info,uint64(val),sign);
    f.Fill();

    return f;
}

sFormatStringBuffer& operator% (sFormatStringBuffer &f,void *ptr)
{
    if (!*f.Format)
        return f;

    sFormatStringInfo info;

    f.GetInfo(info);
    info.Null = 1;
    info.Field = sConfig64Bit ? 16 : 8;
    f.PrintInt<uint64>(info,uint64(ptr),0);
    f.Fill();

    return f;
}

sFormatStringBuffer& operator% (sFormatStringBuffer &f,float v)
{
    if (!*f.Format)
        return f;

    sFormatStringInfo info;

    f.GetInfo(info);
    f.PrintFloat(info,v);
    f.Fill();

    return f;
}

sFormatStringBuffer& operator% (sFormatStringBuffer &f,double v)
{
    if (!*f.Format)
        return f;

    sFormatStringInfo info;

    f.GetInfo(info);
    f.PrintFloat(info,v);
    f.Fill();

    return f;
}

sFormatStringBuffer& operator% (sFormatStringBuffer &f,const char *str)
{
    sString<sMaxPath> path;
    int i;
    bool q=1;
    const char *s;
    if (!*f.Format)
        return f;

    sFormatStringInfo info;
    f.GetInfo(info);
    if (str)
    {
        switch(info.Format)
        {
        case 'c':     // lower case
            i=0;
            while(i<sMaxPath-2 && *str)
            {
                char c = *str++;
                if(c>='A' && c<='Z') c=c+'a'-'A';
                path[i++] = c;
            }
            path[i++] = 0;
            f.Add(info,path,0);  
            break;
        case 'C':     // upper case
            i=0;
            while(i<sMaxPath-2 && *str)
            {
                char c = *str++;
                if(c>='a' && c<='z') c=c+'A'-'a';
                path[i++] = c;
            }
            path[i++] = 0;
            f.Add(info,path,0);  
            break;
        case 'p':
            i=0;
            while(i<sMaxPath-2 && *str)
            {
                char c = *str++;
                if(c=='\\') c='/';
                //          if(c=='"') path[i++] = '\\';
                path[i++] = c;
            }
            path[i++] = 0;
            f.Add(info,path,0);  
            break;
        case 'P':
            i=0;
            while(i<sMaxPath-2 && *str)
            {
                char c = *str++;
                if(c=='/') c='\\';
                //          if(c=='"') path[i++] = '\\';
                path[i++] = c;
            }
            path[i++] = 0;
            f.Add(info,path,0);  
            break;
        case 'Q':   // quote if required
            q = 1;
            s = str;
            if(sIsLetter(*s))
            {
                q = 0;
                while(*s && !q)
                {
                    if(!sIsLetter(*s) && !sIsDigit(*s))
                        q = 1;
                    s++;
                }
            }
            // nobreak
        case 'q':   // quote always
            if(q)
            {
                char *d = path;
                *d++ = '\"';
                while(d < path+sMaxPath-3 && *str)
                {
                    switch(*str)
                    {
                    case '"':
                        *d++ = '\\';
                        *d++ = '"';
                        break;
                    case '\n':
                        *d++ = '\\';
                        *d++ = 'n';
                        break;
                    case '\t':
                        *d++ = '\\';
                        *d++ = 't';
                        break;
                    case '\r':
                        *d++ = '\\';
                        *d++ = 'r';
                        break;
                    case '\f':
                        *d++ = '\\';
                        *d++ = 'f';
                        break;
                    default:
                        *d++ = *str;
                        break;
                    }
                    str++;
                }
                *d++ = '\"';
                *d++ = 0;
                f.Add(info,path,0);
            }
            else
            {
                f.Add(info,str,0);
            }
            break;
        default:    // 's'
            f.Add(info,str,0);
            break;
        }
    }
    f.Fill();

    return f;
}

/****************************************************************************/
/***                                                                      ***/
/***   pooled strings. create on write / delete never                     ***/
/***                                                                      ***/
/****************************************************************************/

sPoolStringPool::sPoolStringPool()
{
    sClear(Hash);
    EPool = new sMemoryPool();
    SPool = new sMemoryPool();
    EmptyString = "";
}

sPoolStringPool::~sPoolStringPool()
{
    delete EPool;
    delete SPool;
}

const char *sPoolStringPool::Add(const char *s,uptr len)
{
    uint hash = sHashString(s,len) & (HashSize-1);

    Entry *e = Hash[hash];
    while(e)
    {
        if(sCmpMem(s,e->Data,len)==0 && e->Data[len]==0)
            return e->Data;
        e = e->Next;
    }

    e = EPool->Alloc<Entry>();
    e->Data = SPool->Alloc<char>(len+1);
    sCopyMem(e->Data,s,len);
    e->Data[len]=0;
    e->Next = Hash[hash];
    Hash[hash] = e;

    return e->Data;
}

void sPoolStringPool::Reset()
{
    sClear(Hash);
    EPool->Reset();
    SPool->Reset();
}

/****************************************************************************/

sPoolStringPool *sDefaultStringPool;

class sPoolStringPoolSubsystem : public sSubsystem
{
public:
    sPoolStringPoolSubsystem() : sSubsystem("sPoolString",0x02) {}
    void Init()
    {
        sDefaultStringPool = new sPoolStringPool;
    }
    void Exit()
    {
        delete sDefaultStringPool;
    }
} sPoolStringPoolSubsystem_;

/****************************************************************************/
/***                                                                      ***/
/***   reference counted strings (dynamic memory management, slow)        ***/
/***                                                                      ***/
/****************************************************************************/

void sRCString::SetSize(uptr size)
{
    if(size!=Size)
    {
        sDelete(Buffer);
        if(size>0)
            Buffer = new char[size];
        Size = size;
    }
}

void sRCString::Set(const char *str,uptr len)
{ 
    if(Size<len+1) 
        SetSize(len+1);
    sCopyMem(Buffer,str,len);
    Buffer[len] = 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   text log. for appending to a big buffer.                           ***/
/***                                                                      ***/
/****************************************************************************/

sTextLog::sTextLog()
{
    Alloc = 0;
    Used = 0;
    Buffer = 0;//new char[Alloc];
    //  Buffer[0] = 0;
}

sTextLog::~sTextLog()
{
    delete Buffer;
}

void sTextLog::Clear()
{
    Used = 0;
    if(Buffer)
        Buffer[0] = 0;
}

void sTextLog::Set(const char *s)
{
    Clear();
    Print(s);
}

const char *sTextLog::Get() const
{
    return Buffer ? Buffer : "";
}

void sTextLog::Print(const char *s)
{
    Add(s,sGetStringLen(s));
}

void sTextLog::PrintHeader(const char *s)
{
    Print ("/****************************************************************************/\n");
    Print ("/***                                                                      ***/\n");
    PrintF("/***   %-64s   ***/\n",s);
    Print ("/***                                                                      ***/\n");
    Print ("/****************************************************************************/\n");
}

void sTextLog::Grow(uptr size)
{
    if(size+1>Alloc)
    {
        uptr na = sMax(sMax<uptr>(64,Alloc*2),size+1);
        char *nb = new char[na];
        sCopyMem(nb,Get(),Used*sizeof(char)+1);
        delete[] Buffer;
        Buffer = nb;
        Alloc = na;
    }
}

sStringDesc sTextLog::GetStringDesc(uptr n)
{
    Grow(n-1);
    return sStringDesc(Buffer,n);
}

void sTextLog::Add(const char *s,uptr len)
{
    len = sMin(len,sGetStringLen(s));    // in case there is a 0 in the string
    Grow(Used+len+1);
    sASSERT(Buffer);
    sCopyMem(Buffer+Used,s,len*sizeof(char));
    Used += len;
    Buffer[Used] = 0;
}

void sTextLog::Prepend(const char *s,uptr len)
{
    if(len==-1)
        len = sGetStringLen(s);
    Grow(Used+len+1);
    sASSERT(Buffer);
    for(sptr i=Used;i>=0;i--)
        Buffer[i+len] = Buffer[i];
    for(uptr i=0;i<len;i++)
        Buffer[i] = s[i];
    Used += len;
    Buffer[Used] = 0;  
}


void sTextLog::PrintChar(char c)
{
    Grow(Used+1);
    sASSERT(Buffer);
    Buffer[Used++] = c;
    Buffer[Used] = 0;
}

void sTextLog::HexDump32(uptr offset,const uint *data,uptr bytes)
{
    int n = 0;
    while(bytes>3)
    {
        if((n&7)==0)
        {
            if(n!=0)
                Print("\n");
            PrintF("%08x ",offset);
        }
        PrintF(" %08x",*data++);
        bytes -= 4;
        offset += 4;
        n++;
    }
}

int sTextLog::CountLines()
{
    int n=0;
    const char *t = Get();
    while(*t)
    {
        if(*t=='\n') n++;
        t++;
    }
    return n;
}

void sTextLog::TrimToLines(int n1)
{
    if(!Buffer)
        return;
    int n0 = CountLines();
    const char *s = Buffer;
    char *d = Buffer;
    for(int i=n1;i<n0;i++)
    {
        while(*s!=0 && *s!='\n') s++;
        if(*s=='\n') s++;
    }

    while(*s)
        *d++ = *s++;
    Used = d-Buffer;
    *d++ = 0;
}

void sTextLog::ExtendTabs(int tabsize)
{
    if(!Buffer)
        return;
    uptr tabcount = 0;
    for(uptr i=0;i<Used;i++)
        tabcount += (Buffer[i] == '\t');
    uptr newsize = Used + tabcount*(tabsize-1);
    char *newbuffer = new char[newsize];
    char *d = newbuffer;
    int pos = 0;
    for(uptr i=0;i<Used;i++)
    {
        char c = Buffer[i];
        if(c=='\t')
        {
            do
            {
                *d++ = ' ';
                pos++;
            }
            while(pos % tabsize != 0);
        }
        if(c=='\n')
        {
            *d++ = c;
            pos = 0;
        }
        else
        {
            *d++ = c;
            pos++;
        }
    }
    sASSERT(d <= newbuffer+newsize);

    Clear();
    Add(newbuffer,d-newbuffer);
    delete[] newbuffer;
}

void sTextLog::Serialize(sReader &s)
{
    int n = 0;
    Used = 0;
    s.Align(4);
    s.Value(n);
    Grow(Used+n+1);
    sASSERT(Buffer);
    Used = n;
    s.SmallData(Used,Buffer);
    Buffer[Used] = 0;
    s.Align(4);
}

void sTextLog::Serialize(sWriter &s)
{
    s.String(Get(),sMax<uptr>(1,Alloc));
}

void sTextLog::DumpWithLineNumbers() const
{
    const char *s = Get();
    int line = 1;
    sString<sFormatBuffer> buffer;
    while(*s)
    {
        int n = 0;
        while(s[n] && s[n]!='\n')
            n++;
        buffer.Init(s,n);
        sDPrintF("%04d %s\n",line++,buffer);
        s += n;
        if(*s=='\n')
            s++;
    }
}

void sTextLog::PrintWithLineNumbers(const char *s)
{
    int line = 1;
    sString<sFormatBuffer> buffer;
    while(*s)
    {
        int n = 0;
        while(s[n] && s[n]!='\n')
            n++;
        buffer.Init(s,n);
        PrintF("%04d %s\n",line++,buffer);
        s += n;
        if(*s=='\n')
            s++;
    }
}

/****************************************************************************/

void sTextBuffer::Delete(uptr pos)
{
    if(pos==Used)
        return;
    sASSERT(pos>=0 && pos<Used);
    
    uptr n = 1;
    if(Buffer[pos])
    {
        while((Buffer[pos+n]&0xc0)==0x80) n++;
        Delete(pos,pos+n);
    }
}

void sTextBuffer::Delete(uptr start,uptr end)
{
    uptr len = Used;
    uptr n = end-start;
    sASSERT(n>=0);
    sASSERT(start<=len);
    sASSERT(end<=len);

    for(uptr i=start;i<len-n+1;i++)
        Buffer[i] = Buffer[i+n];
    Used -= n;
}

bool sTextBuffer::Insert(uptr pos,int c)
{
    PrintBuffer.PrintF("%c",c);
    return Insert(pos,PrintBuffer);
}

bool sTextBuffer::Insert(uptr pos,const char *text,uptr n)
{
    if(n==-1)
        n = sGetStringLen(text);
    Grow(Used+n+1);
    sASSERT(Buffer);
    for(uptr i=Used+n;i>=pos+n;i--)
        Buffer[i] = Buffer[i-n];

    for(uptr i=0;i<n;i++)
        Buffer[pos+i] = text[i];

    Used += n;
    sASSERT(Used<Alloc);
    Buffer[Used]=0;

    return 1;
}

/****************************************************************************/
/***                                                                      ***/
/***   sCommandline Parsing                                               ***/
/***                                                                      ***/
/****************************************************************************/

sCommandlineParser::sCommandlineParser()
{
    Error = 0;
    NoCmds = 0;
    HelpFlag = 0;
}

sCommandlineParser::~sCommandlineParser()
{
}

/****************************************************************************/

void sCommandlineParser::AddSwitch(const char *name,int &ref,const char *help)
{
    key *k = Keys.Add();
    sClear(*k);
    sASSERT(name);
    k->Name = name;
    k->WasSet = &ref;
    k->Help = help;
    k->Optional = 1;
}

void sCommandlineParser::AddHelp(const char *name)
{
    AddSwitch("?",HelpFlag,"display help");
}

void sCommandlineParser::AddInt(const char *name,int &ref,int *wasset,const char *help)
{
    key *k = Keys.Add();
    sClear(*k);
    sASSERT(name);
    k->Optional = 1;
    if(name[0]=='!')
    {
        k->Optional = 0;
        name++;
    }
    k->Name = name;
    k->RefInt = &ref;
    k->WasSet = &ref;
    k->Help = help;
}

void sCommandlineParser::AddString(const char *name,const sStringDesc &ref,int *wasset,const char *help)
{
    key *k = Keys.Add();
    sClear(*k);
    k->Optional = 1;
    if(name[0]=='!')
    {
        k->Optional = 0;
        name++;
    }
    k->Name = name ? name : "";
    k->RefStr = ref;
    k->WasSet = wasset;
    k->Help = help;
}

void sCommandlineParser::AddFile(const char *name,const sStringDesc &ref,int *wasset,const char *help)
{
    key *k = Keys.Add();
    sClear(*k);
    k->Optional = 1;
    if(name && name[0]=='!')
    {
        k->Optional = 0;
        name++;
    }
    k->Name = name ? name : "";
    k->RefStr = ref;
    k->WasSet = wasset;
    k->File = 1;
    k->Help = help;
}

bool sCommandlineParser::limstr::Cmp(const char *zl)
{
    for(uptr i=0;i<Len;i++)
        if(Data[i]!=zl[i]) return 0;
    if(zl[Len]!=0) return 0;
    return 1;
}

void sCommandlineParser::ScanSpace(const char *&s)
{
    while(sIsSpace(*s)) s++;
}

void sCommandlineParser::ScanName(const char *&s_,limstr &name)
{
    const char *s = s_;

    if(*s=='"')
    {
        s++;
        name.Data = s;
        while(*s!='"' && *s!=0)
        {
            if(s[0]=='\\' && s[1]!=0)
                s+=2;
            else
                s++;
        }
        name.Len = s-name.Data;
        if(*s=='"')
            s++;
        else
            Error = 1;
    }
    else
    {
        if(sIsSpace(*s) || *s==0 || *s==',' || *s=='=')
        {
            Error = 1;
            s++;
        }
        else
        {
            name.Data = s;
            while(!sIsSpace(*s) && *s!=0 && *s!=',' && *s!='=')
                s++;
            name.Len = s-name.Data;
        }
    }

    s_ = s;
}

bool sCommandlineParser::Parse(bool print)
{
    return Parse(sGetCommandline(),print);
}

bool sCommandlineParser::Parse(const char *cmdline,bool print)
{
    Error = 0;
    sString<sFormatBuffer> buffer;

    // scan the commandline

    const char *s = cmdline;

    limstr exe,value;
    ScanName(s,exe);
    ScanSpace(s);

    while(*s)
    {
        if(*s=='-')
        {
            s++;

            cmd *c = new cmd;
            Cmds.Add(c);

            ScanName(s,c->Name);
            if(*s=='=')
            {
                s++;
                for(;;)
                {
                    ScanName(s,value);
                    c->Values.Add(value);
                    if(*s!=',')
                        break;
                }
            }
        }
        else
        {
            if(!NoCmds)
            {
                NoCmds = new cmd;
                NoCmds->Name.Data = "";
                NoCmds->Name.Len = 0;
                Cmds.Add(NoCmds);
            }
            ScanName(s,value);
            NoCmds->Values.Add(value);
        }
        ScanSpace(s);
    }

    if(Error && print)
    {
        sPrint("Commandline is malformed.\n");
        sPrint("It should look something like that:\n");
        sPrint("text.exe xxxx -bla -blub=yyyy -foo=\"hello world\",zzzz\n");
    }
    if(1)
    {
        sLogF("types","commandline switches:");
        sString<sMaxPath> log;
        for(auto c : Cmds)
        {
            if(c->Name.Len==0)
            {
                log = "  (none)";
            }
            else
            {
                buffer.Init(c->Name.Data,c->Name.Len);
                log = "  -";
                log.Add(buffer);
            }
            int first = 1;
            for(auto &k : c->Values)
            {
                buffer.Init(k.Data,k.Len);
                if(first)
                {
                    log.Add("=");
                    first = 0;
                }
                else
                {
                    log.Add(",");
                }

                log.Add("\"");
                log.Add(buffer);
                log.Add("\"");
            }
            sLog("types",log);
        }
    }

    // interpret the commandline

    if(!Error)
    {
        for(auto c : Cmds)
        {
            key *k = 0;
            for(auto &ki : Keys) 
            {
                if(c->Name.Cmp(ki.Name))
                {
                    k = &ki;
                    break;
                }
            }
            if(k)
            {
                k->WasSet_ = 1;
                if(k->WasSet)
                {
                    *k->WasSet = 1;
                }
                if(k->RefInt)
                {
                    if(c->Values.GetCount()!=1)
                    {
                        Error = 1;
                        if(print)
                        {
                            buffer.Init(c->Name.Data,c->Name.Len);
                            sPrintF("option -%s expects an integer",buffer);
                        }
                    }
                    else
                    {
                        buffer.Init(c->Values[0].Data,c->Values[0].Len);
                        const char *scan = buffer;
                        if(!sScanValue(scan,*k->RefInt) || *scan!=0)
                        {
                            Error = 1;
                            if(print)
                            {
                                buffer.Init(c->Name.Data,c->Name.Len);
                                sPrintF("option -%s expects an integer\n",buffer);
                            }
                        }
                    }
                }
                if(k->RefStr.Buffer)
                {
                    if(c->Values.GetCount()!=1)
                    {
                        Error = 1;
                        if(print)
                        {
                            buffer.Init(c->Name.Data,c->Name.Len);
                            sPrintF("option -%s expects an string\n",buffer);
                        }
                    }
                    else
                    {
                        buffer.Init(c->Values[0].Data,c->Values[0].Len);
                        sCopyString(k->RefStr,buffer);

                        if(k->File)
                        {
                            for(int i=0;k->RefStr.Buffer[i];i++)
                                if(k->RefStr.Buffer[i]=='\\')
                                    k->RefStr.Buffer[i]='/';
                        }
                    }
                }
            }
            else
            {
                Error = 1;
                if(print)
                {
                    buffer.Init(c->Name.Data,c->Name.Len);
                    sPrintF("unknown option -%s\n",buffer);
                }
            }
        }

        // all non-optional parameters found?

        if(!HelpFlag)
        {
            for(auto &k : Keys) 
            {
                if(!k.Optional && !k.WasSet_)
                {
                    sPrintF("required option -%s not set\n",k.Name);
                    Error = 1;
                }
            }
        }
        else
        {
            Error = 1;
            Help();
        }
    }

    // ...

    Cmds.DeleteAll();
    NoCmds = 0;
    sLogF("types","commandline switches parsing: %s",Error?"failure":"ok");
    return !Error;
}

/****************************************************************************/

void sCommandlineParser::Help()
{
    sPrint("Options ");
    for(auto &k : Keys)
    {
        if(k.Optional)
            sPrint("[");
        sPrintF("-%s",k.Name);
        if(k.RefInt)
            sPrintF("=number");
        else if(k.File)
            sPrintF("=\"file\"");
        else if(k.RefStr.Buffer)
            sPrintF("=\"string\"");
        if(k.Optional)
            sPrint("]");
        sPrint(" ");
    }
    sPrint("\n\n");
    for(auto &k : Keys)
    {
        sPrintF("-%-14s %s\n",k.Name,k.Help?k.Help:"");
    }
    sPrint("\n"); 
}

/****************************************************************************/
/***                                                                      ***/
/***   Pattern Shit                                                       ***/
/***                                                                      ***/
/****************************************************************************/

// ?=any one character,  *=zero or more characters
bool sMatchWildcard(const char *wild,const char *text,bool casesensitive,bool pathsensitive)
{
    for(;;)
    {
        if(*wild=='*')
        {
            wild++;
            while(*text)
            {
                if(sMatchWildcard(wild,text,casesensitive,pathsensitive)) return 1;
                text++;
            }
        }
        else if(*wild=='?')
        {
            if(*text==0) return 0;
            wild++;
            text++;
        }
        else if(*wild==0)
        {
            return (*text==0);
        }
        else 
        {
            int a = *text;
            int b = *wild;

            if(!casesensitive)
            {
                a = sUpperChar(a);
                b = sUpperChar(b);
            }
            if(!pathsensitive)
            {
                if(a=='\\') a = '/';
                if(b=='\\') b = '/';
            }

            if(a!=b) return 0;

            wild++;
            text++;
        }
    }
}


/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

}

