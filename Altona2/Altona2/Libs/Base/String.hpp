/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_BASE_STRING_HPP
#define FILE_ALTONA2_LIBS_BASE_STRING_HPP

#include "Altona2/Libs/Base/Machine.hpp"
#include "Altona2/Libs/Base/Types.hpp"
#include "Altona2/Libs/Base/Serialize.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***   Basic String Operations                                            ***/
/***                                                                      ***/
/****************************************************************************/

struct sStringDesc                // this is always used as CONST, otherwise it won't work correctly.
{
    char *Buffer;
    uptr Size;
    sStringDesc() : Buffer(0),Size(0) {}
    sStringDesc(char *b,uptr s) : Buffer(b),Size(s) {}
    sStringDesc(const sStringDesc &s) : Buffer(s.Buffer),Size(s.Size) {}
    sStringDesc& operator=(const sStringDesc &s) { Buffer=s.Buffer; Size=s.Size; return*this;}
};

void sCopyString(char *d,const char *s,uptr size);
void sCopyString(const sStringDesc &desc,const char *s);
void sCopyStringLen(const sStringDesc &desc,const char *s,uptr n);

int sCmpString(const char *a,const char *b);               // normal string compare
int sCmpStringI(const char *a,const char *b);              // case insensitive 
int sCmpStringP(const char *a,const char *b);              // filename paths ('/'=='\', case insensitive)
int sCmpStringLen(const char *a,const char *b,uptr len);   // first characters match
int sCmpStringILen(const char *a,const char *b,uptr len);
int sCmpStringPLen(const char *a,const char *b,uptr len);

void sAppendString(char *d,const char *s,uptr size);
void sAppendStringLen(char *d,const char *s,uptr size,uptr len);

inline bool sIsDigit(int i) { return (i>='0' && i<='9'); }
inline bool sIsLetter(int i) { return (i>='a' && i<='z') || (i>='A' && i<='Z') || i=='_'; }
inline bool sIsSpace(int i) { return i==' ' || i=='\t' || i=='\r' || i=='\n'; }
inline bool sIsHex(int i) { return (i>='a' && i<='f') || (i>='A' && i<='F') || (i>='0' && i<='9'); }

inline int sGetDigit(int i) { return i-'0'; }
inline int sGetHex(int i) { if(i>='0' && i<'9') return i&15; else return (i-'a'+10)&15; }

char sUpperChar(char c);
char sLowerChar(char c);

const char *sAllocString(const char *);

int sFindFirstChar(const char *str,char c);
int sFindFirstString(const char *str,const char *find);

/****************************************************************************/
/***                                                                      ***/
/***   String Operations for working with file system paths               ***/
/***                                                                      ***/
/****************************************************************************/

void sCleanPath(const sStringDesc &str);    // clean up '\' -> '/', '//' -> '/', '../' , ..
const char *sExtractLastSuffix(const char *); // xxx.x.y -> y
const char *sExtractAllSuffix(const char *);  // xxx.x.y -> x.y
const char *sExtractName(const char *);       // xxx/y/z -> z
void sRemoveLastSuffix(const sStringDesc &str); // xxx.x.y -> xxx.x
void sRemoveAllSuffix(const sStringDesc &str);  // xxx.x.y -> xxx
void sRemoveName(const sStringDesc &str);       // xxx/y/z -> xxx/y
bool sMakeRelativePath(const sStringDesc &out,const char *fromdir,const char *tofile);

/****************************************************************************/
/***                                                                      ***/
/***   UTF-8 Parsing and Synthesis                                        ***/
/***                                                                      ***/
/****************************************************************************/

int sReadUTF8(const char *&s);
void sWriteUTF8(char *&d,uint c);

bool sUTF8toUTF16(const char *s,uint16 *outbuffer,int buffersize,uptr *bufferused=0);
bool sUTF8toUTF16Scan(const char *&s,uint16 *outbuffer,int buffersize,uptr *bufferused=0);
char *sUTF16toUTF8(const uint16 *s);
bool sUTF16toUTF8Scan(const uint16 *s,const sStringDesc &dest);
bool sUTF16toUTF8(const uint16 *s_,const sStringDesc &dest);

/****************************************************************************/
/***                                                                      ***/
/***   Numeric Conversions                                                ***/
/***                                                                      ***/
/****************************************************************************/

struct sFloatInfo
{
    char Digits[20];               // zero-terminated digits, without decimal 
    int Exponent;                  // 1.0e0 -> Exponent = 0;
    uint64 NaN;                       // if NaN, this holds all the NaN bits
    bool Negative;                 // sign (1 or 0)
    bool Denormal;                 // denormal detected during float->ascii conversion
    bool Infinite;                 // infinite (1 or 0)

    void PrintE(const sStringDesc &desc,int fractions); // print number in 1.2345e6 notation
    void PrintF(const sStringDesc &desc,int fractions); // print number in 0.0001234 notation
    bool Scan(const char *&scan);  // fill structure 

    // float -> ascii -> float roundtrips reproduce bitpattern 100% (at min 9 digits)

    void FloatToAscii(uint f,int digits=9);   
    uint AsciiToFloat();

    // double -> ascii -> double roundtips have a maximum error of 2ulp's (at min 17 digits)

    void DoubleToAscii(uint64 f,int digits=17); 
    uint64 AsciiToDouble();
};

bool sScanHex(const char *&scan,int &val, int maxlen=0);


bool sScanValue(const char *&s,uint8 &result);
bool sScanValue(const char *&s,int8 &result);
bool sScanValue(const char *&s,uint16 &result);
bool sScanValue(const char *&s,int16 &result);

bool sScanValue(const char *&scan,int &val);
bool sScanValue(const char *&scan,int64 &val);
bool sScanValue(const char *&scan,uint &val);
bool sScanValue(const char *&scan,uint64 &val);
bool sScanValue(const char *&scan,float &val);
bool sScanValue(const char *&scan,double &val);
bool sScanHexColor(const char *&scan,uint &val);

/****************************************************************************/
/***                                                                      ***/
/***   Formatted Printing                                                 ***/
/***                                                                      ***/
/****************************************************************************/


struct sFormatStringInfo      // parses %n.mf, like %12.6f 
{
    int Format;                // f, f
    int Field;                 // n, 12
    int Fraction;              // m, 6 (-1 if not specified)
    int Minus;                 // starts with negative sign -> left aligned
    int Null;                  // starts with null character -> pad with zero
    int Truncate;              // starts with exclamation mark -> truncate output to field length if bigger
};

struct sFormatStringBuffer
{
    char *Start;
    char *Dest;
    char *End;
    const char *Format;

    bool Fill();
    void GetInfo(sFormatStringInfo &);
    void Add(const sFormatStringInfo &info,const char *buffer,bool sign);
    void Print(const char *str);
    template<typename Type> void PrintInt(const sFormatStringInfo &info,Type v,bool sign);
    void PrintFloat(const sFormatStringInfo &info,float v);
    void PrintFloat(const sFormatStringInfo &info,double v);
    const char *Get() { return Start; }
};

/****************************************************************************/

sFormatStringBuffer sFormatStringBase(const sStringDesc &buffer,const char *format);
sFormatStringBuffer& operator% (sFormatStringBuffer &,int);
sFormatStringBuffer& operator% (sFormatStringBuffer &,uint);
sFormatStringBuffer& operator% (sFormatStringBuffer &,uint64);
sFormatStringBuffer& operator% (sFormatStringBuffer &,int64);
sFormatStringBuffer& operator% (sFormatStringBuffer &,float);
sFormatStringBuffer& operator% (sFormatStringBuffer &,double);
sFormatStringBuffer& operator% (sFormatStringBuffer &,void *);
sFormatStringBuffer& operator% (sFormatStringBuffer &,const char *);

// we want to get rid of the variable arg thing and have a typesafe solution
// three examples:
//   sDPrintF(format,...)
//   sSPrintF(buffer,format,...)
//   sTextBuffer::PrintF(format,...)
// this has to be extended to templates (up to ten variable args):
//   template<typename A> sDPrintF(format,A a);
//   template<typename A,typename B> sDPrintF(format,A a,B b);
//   template<typename A,typename B,typename C> sDPrintF(format,A a,B b,C c);
// we don't want to type this everytime, especially since the template syntax is so lengthy
// that's what the sPRINTING macros are for:
//   sDPrintF(format,...) -> sPRINTING0
//   sSPrintF(buffer,format,...) -> sPRINTING1
//   sTextBuffer::PrintF(format,...) -> sPRINTING0
// how do we implement that?
//   #define sPRINTING0(Symbol,PreImpl,PostImpl)
//   #define sPRINTING1(Symbol,PreImpl,PostImpl,ArgType1)
//   #define sPRINTING2(Symbol,PreImpl,PostImpl,ArgType1,ArgType2)

#define sPRINTING0(Symbol,PreImpl,PostImpl) \
    template<typename A,typename B,typename C,typename D,typename E,typename F,typename G,typename H,typename I,typename J> inline void Symbol(const char *format,A a,B b,C c,D d,E e,F f,G g,H h,I i,J j) { PreImpl%a%b%c%d%e%f%g%h%i%j; PostImpl } \
    template<typename A,typename B,typename C,typename D,typename E,typename F,typename G,typename H,typename I           > inline void Symbol(const char *format,A a,B b,C c,D d,E e,F f,G g,H h,I i    ) { PreImpl%a%b%c%d%e%f%g%h%i  ; PostImpl } \
    template<typename A,typename B,typename C,typename D,typename E,typename F,typename G,typename H                      > inline void Symbol(const char *format,A a,B b,C c,D d,E e,F f,G g,H h        ) { PreImpl%a%b%c%d%e%f%g%h    ; PostImpl } \
    template<typename A,typename B,typename C,typename D,typename E,typename F,typename G                                 > inline void Symbol(const char *format,A a,B b,C c,D d,E e,F f,G g            ) { PreImpl%a%b%c%d%e%f%g      ; PostImpl } \
    template<typename A,typename B,typename C,typename D,typename E,typename F                                            > inline void Symbol(const char *format,A a,B b,C c,D d,E e,F f                ) { PreImpl%a%b%c%d%e%f        ; PostImpl } \
    template<typename A,typename B,typename C,typename D,typename E                                                       > inline void Symbol(const char *format,A a,B b,C c,D d,E e                    ) { PreImpl%a%b%c%d%e          ; PostImpl } \
    template<typename A,typename B,typename C,typename D                                                                  > inline void Symbol(const char *format,A a,B b,C c,D d                        ) { PreImpl%a%b%c%d            ; PostImpl } \
    template<typename A,typename B,typename C                                                                             > inline void Symbol(const char *format,A a,B b,C c                            ) { PreImpl%a%b%c              ; PostImpl } \
    template<typename A,typename B                                                                                        > inline void Symbol(const char *format,A a,B b                                ) { PreImpl%a%b                ; PostImpl } \
    template<typename A                                                                                                   > inline void Symbol(const char *format,A a                                    ) { PreImpl%a                  ; PostImpl } \
    /*                                                                                                                   */ inline void Symbol(const char *format                                        ) { PreImpl%0                  ; PostImpl }

#define sPRINTING0R(rtype,Symbol,PreImpl,PostImpl) \
    template<typename A,typename B,typename C,typename D,typename E,typename F,typename G,typename H,typename I,typename J> inline rtype Symbol(const char *format,A a,B b,C c,D d,E e,F f,G g,H h,I i,J j) { PreImpl%a%b%c%d%e%f%g%h%i%j; PostImpl } \
    template<typename A,typename B,typename C,typename D,typename E,typename F,typename G,typename H,typename I           > inline rtype Symbol(const char *format,A a,B b,C c,D d,E e,F f,G g,H h,I i    ) { PreImpl%a%b%c%d%e%f%g%h%i  ; PostImpl } \
    template<typename A,typename B,typename C,typename D,typename E,typename F,typename G,typename H                      > inline rtype Symbol(const char *format,A a,B b,C c,D d,E e,F f,G g,H h        ) { PreImpl%a%b%c%d%e%f%g%h    ; PostImpl } \
    template<typename A,typename B,typename C,typename D,typename E,typename F,typename G                                 > inline rtype Symbol(const char *format,A a,B b,C c,D d,E e,F f,G g            ) { PreImpl%a%b%c%d%e%f%g      ; PostImpl } \
    template<typename A,typename B,typename C,typename D,typename E,typename F                                            > inline rtype Symbol(const char *format,A a,B b,C c,D d,E e,F f                ) { PreImpl%a%b%c%d%e%f        ; PostImpl } \
    template<typename A,typename B,typename C,typename D,typename E                                                       > inline rtype Symbol(const char *format,A a,B b,C c,D d,E e                    ) { PreImpl%a%b%c%d%e          ; PostImpl } \
    template<typename A,typename B,typename C,typename D                                                                  > inline rtype Symbol(const char *format,A a,B b,C c,D d                        ) { PreImpl%a%b%c%d            ; PostImpl } \
    template<typename A,typename B,typename C                                                                             > inline rtype Symbol(const char *format,A a,B b,C c                            ) { PreImpl%a%b%c              ; PostImpl } \
    template<typename A,typename B                                                                                        > inline rtype Symbol(const char *format,A a,B b                                ) { PreImpl%a%b                ; PostImpl } \
    template<typename A                                                                                                   > inline rtype Symbol(const char *format,A a                                    ) { PreImpl%a                  ; PostImpl } \
    /*                                                                                                                   */ inline rtype Symbol(const char *format                                        ) { PreImpl%0                  ; PostImpl }

#define sPRINTING1(Symbol,PreImpl,PostImpl,ArgType1) \
    template<typename A,typename B,typename C,typename D,typename E,typename F,typename G,typename H,typename I,typename J> inline void Symbol(ArgType1 arg1,const char *format,A a,B b,C c,D d,E e,F f,G g,H h,I i,J j) { PreImpl%a%b%c%d%e%f%g%h%i%j; PostImpl } \
    template<typename A,typename B,typename C,typename D,typename E,typename F,typename G,typename H,typename I           > inline void Symbol(ArgType1 arg1,const char *format,A a,B b,C c,D d,E e,F f,G g,H h,I i    ) { PreImpl%a%b%c%d%e%f%g%h%i  ; PostImpl } \
    template<typename A,typename B,typename C,typename D,typename E,typename F,typename G,typename H                      > inline void Symbol(ArgType1 arg1,const char *format,A a,B b,C c,D d,E e,F f,G g,H h        ) { PreImpl%a%b%c%d%e%f%g%h    ; PostImpl } \
    template<typename A,typename B,typename C,typename D,typename E,typename F,typename G                                 > inline void Symbol(ArgType1 arg1,const char *format,A a,B b,C c,D d,E e,F f,G g            ) { PreImpl%a%b%c%d%e%f%g      ; PostImpl } \
    template<typename A,typename B,typename C,typename D,typename E,typename F                                            > inline void Symbol(ArgType1 arg1,const char *format,A a,B b,C c,D d,E e,F f                ) { PreImpl%a%b%c%d%e%f        ; PostImpl } \
    template<typename A,typename B,typename C,typename D,typename E                                                       > inline void Symbol(ArgType1 arg1,const char *format,A a,B b,C c,D d,E e                    ) { PreImpl%a%b%c%d%e          ; PostImpl } \
    template<typename A,typename B,typename C,typename D                                                                  > inline void Symbol(ArgType1 arg1,const char *format,A a,B b,C c,D d                        ) { PreImpl%a%b%c%d            ; PostImpl } \
    template<typename A,typename B,typename C                                                                             > inline void Symbol(ArgType1 arg1,const char *format,A a,B b,C c                            ) { PreImpl%a%b%c              ; PostImpl } \
    template<typename A,typename B                                                                                        > inline void Symbol(ArgType1 arg1,const char *format,A a,B b                                ) { PreImpl%a%b                ; PostImpl } \
    template<typename A                                                                                                   > inline void Symbol(ArgType1 arg1,const char *format,A a                                    ) { PreImpl%a                  ; PostImpl } \
    /*                                                                                                                   */ inline void Symbol(ArgType1 arg1,const char *format                                        ) { PreImpl%0                 ; PostImpl }

#define sPRINTING2(Symbol,PreImpl,PostImpl,ArgType1,ArgType2) \
    template<typename A,typename B,typename C,typename D,typename E,typename F,typename G,typename H,typename I,typename J> inline void Symbol(ArgType1 arg1,ArgType2 arg2,const char *format,A a,B b,C c,D d,E e,F f,G g,H h,I i,J j) { PreImpl%a%b%c%d%e%f%g%h%i%j; PostImpl } \
    template<typename A,typename B,typename C,typename D,typename E,typename F,typename G,typename H,typename I           > inline void Symbol(ArgType1 arg1,ArgType2 arg2,const char *format,A a,B b,C c,D d,E e,F f,G g,H h,I i    ) { PreImpl%a%b%c%d%e%f%g%h%i  ; PostImpl } \
    template<typename A,typename B,typename C,typename D,typename E,typename F,typename G,typename H                      > inline void Symbol(ArgType1 arg1,ArgType2 arg2,const char *format,A a,B b,C c,D d,E e,F f,G g,H h        ) { PreImpl%a%b%c%d%e%f%g%h    ; PostImpl } \
    template<typename A,typename B,typename C,typename D,typename E,typename F,typename G                                 > inline void Symbol(ArgType1 arg1,ArgType2 arg2,const char *format,A a,B b,C c,D d,E e,F f,G g            ) { PreImpl%a%b%c%d%e%f%g      ; PostImpl } \
    template<typename A,typename B,typename C,typename D,typename E,typename F                                            > inline void Symbol(ArgType1 arg1,ArgType2 arg2,const char *format,A a,B b,C c,D d,E e,F f                ) { PreImpl%a%b%c%d%e%f        ; PostImpl } \
    template<typename A,typename B,typename C,typename D,typename E                                                       > inline void Symbol(ArgType1 arg1,ArgType2 arg2,const char *format,A a,B b,C c,D d,E e                    ) { PreImpl%a%b%c%d%e          ; PostImpl } \
    template<typename A,typename B,typename C,typename D                                                                  > inline void Symbol(ArgType1 arg1,ArgType2 arg2,const char *format,A a,B b,C c,D d                        ) { PreImpl%a%b%c%d            ; PostImpl } \
    template<typename A,typename B,typename C                                                                             > inline void Symbol(ArgType1 arg1,ArgType2 arg2,const char *format,A a,B b,C c                            ) { PreImpl%a%b%c              ; PostImpl } \
    template<typename A,typename B                                                                                        > inline void Symbol(ArgType1 arg1,ArgType2 arg2,const char *format,A a,B b                                ) { PreImpl%a%b                ; PostImpl } \
    template<typename A                                                                                                   > inline void Symbol(ArgType1 arg1,ArgType2 arg2,const char *format,A a                                    ) { PreImpl%a                  ; PostImpl } \
    /*                                                                                                                   */ inline void Symbol(ArgType1 arg1,ArgType2 arg2,const char *format                                        ) { PreImpl%0                  ; PostImpl }

/****************************************************************************/
/***                                                                      ***/
/***   simple strings (no memory management)                              ***/
/***                                                                      ***/
/****************************************************************************/

template<int size> struct sString
{
protected:
    char Buffer[size];
public:
    sString()                               { Buffer[0] = 0; }
    ~sString()                              { Buffer[0] = 1; }
    sString(const char *s)                  { if (s) sCopyString(Buffer,s,size); else Buffer[0]=0; }
    sString(const char *s,uptr len)         { if (s) sCopyString(Buffer,s,sMin(uptr(size),len+1)); else Buffer[0]=0; }
    sString(const sString &s)               { sCopyString(Buffer,s.Buffer,size); }
    template<int n> sString(const sString<n> &s) { sCopyString(Buffer,s,size); }
    sString &operator=(const char *s)       { if (s) sCopyString(Buffer,s,size); else Buffer[0]=0; return *this; }
    template<int n> sString &operator=(const sString<n> &s) { if(s) sCopyString(Buffer,s,size); else Buffer[0]=0; return *this; }

    operator char*()                        { return Buffer; }
    operator const char*() const            { return Buffer; }
    operator sStringDesc()                  { return sStringDesc(Buffer,size); }
    char *GetBuffer()                       { return Buffer; }
    const char *Get() const                 { return Buffer; }
    const char &operator[](int i) const     { return Buffer[i]; }
    char &operator[](int i)                 { return Buffer[i]; }
    const char &operator[](uptr i) const    { return Buffer[i]; }
    char &operator[](uptr i)                { return Buffer[i]; }
    int Count() const                       { return (int) sGetStringLen(Buffer); }
    bool IsEmpty() const                    { return Buffer[0]==0; }
    int Size() const                        { return size; }
    void Clear()                            { Buffer[0]=0; }

    void Init(const char *a)                { uptr n = sMin(uptr(size-1),sGetStringLen(a)); sCopyMem(Buffer,a,n*sizeof(char)); Buffer[n]=0; }
    void Init(const char *a,uptr len)       { uptr n = sMin(uptr(size-1),len); sCopyMem(Buffer,a,n*sizeof(char)); Buffer[n]=0; }
    void Add(const char *a)                 { sAppendString(Buffer,a,size); }
    void Add(const char *a,uptr len)        { sAppendStringLen(Buffer,a,size,len); }
    void AddComma(const char *a)            { if(!IsEmpty()) Add(a); }

    sPRINTING0(PrintF,sFormatStringBuffer buf=sFormatStringBase(sStringDesc(Buffer,size),format);buf,;);
    sPRINTING0(AddF,sFormatStringBuffer buf=sFormatStringBase(sStringDesc(Buffer+sGetStringLen(Buffer),size-sGetStringLen(Buffer)),format);buf,;);
};

template<int na,int nb>
inline bool operator ==(const sString<na> &a,const sString<nb> &b) { return sCmpString(a.Get(),b.Get())==0; }
template<int na,int nb>
inline bool operator !=(const sString<na> &a,const sString<nb> &b) { return sCmpString(a.Get(),b.Get())!=0; }
template<int na,int nb>
inline bool operator < (const sString<na> &a,const sString<nb> &b) { return sCmpString(a.Get(),b.Get())< 0; }
template<int na,int nb>
inline bool operator > (const sString<na> &a,const sString<nb> &b) { return sCmpString(a.Get(),b.Get())> 0; }

template<int n>
inline bool operator ==(const char *a,const sString<n> &b) { return sCmpString(a,b.Get())==0; }
template<int n>
inline bool operator !=(const char *a,const sString<n> &b) { return sCmpString(a,b.Get())!=0; }
template<int n>
inline bool operator < (const char *a,const sString<n> &b) { return sCmpString(a,b.Get())< 0; }
template<int n>
inline bool operator > (const char *a,const sString<n> &b) { return sCmpString(a,b.Get())> 0; }

template<int n>
inline bool operator ==(const sString<n> &a,const char *b) { return sCmpString(a.Get(),b)==0; }
template<int n>
inline bool operator !=(const sString<n> &a,const char *b) { return sCmpString(a.Get(),b)!=0; }
template<int n>
inline bool operator < (const sString<n> &a,const char *b) { return sCmpString(a.Get(),b)< 0; }
template<int n>
inline bool operator > (const sString<n> &a,const char *b) { return sCmpString(a.Get(),b)> 0; }

template<int n>
inline char *begin(sString<n> &s)       { return s.GetBuffer(); }
template<int n> 
inline char *end(sString<n> &s)         { return s.GetBuffer()+s.Count(); }
template<int n> 
const char *begin(const sString<n> &s)  { return s.GetBuffer(); }
template<int n> 
const char *end(const sString<n> &s)    { return s.GetBuffer()+s.Count(); }

/****************************************************************************/
/***                                                                      ***/
/***   pooled strings. create on write / delete never                     ***/
/***                                                                      ***/
/****************************************************************************/

class sPoolStringPool
{
    sMemoryPool *EPool;
    sMemoryPool *SPool;
    static const int HashSize = 4096;
    struct Entry
    {
        char *Data;
        Entry *Next;
    };
    Entry *Hash[HashSize];

public:
    sPoolStringPool();
    ~sPoolStringPool();
    const char *Add(const char *,uptr len);
    void Reset();

    const char *EmptyString;
};

extern sPoolStringPool *sDefaultStringPool;

/****************************************************************************/

struct sPoolString              // a string from the pool
{
private:
    const char *Buffer;
public:
    sPoolString()                               { Buffer = sDefaultStringPool->EmptyString; }
    sPoolString(const char *s)                  { Buffer = sDefaultStringPool->Add(s,sGetStringLen(s)); }
    sPoolString(const char *s,uptr len)         { Buffer = sDefaultStringPool->Add(s,len); }
    void Init(const char *s)                    { Buffer = sDefaultStringPool->Add(s,sGetStringLen(s)); }
    void Init(const char *s,uptr len)           { Buffer = sDefaultStringPool->Add(s,len); }

    sPoolString(sPoolStringPool *p)                             { Buffer = p->EmptyString; }
    sPoolString(const char *s,sPoolStringPool *p)               { Buffer = p->Add(s,sGetStringLen(s)); }
    sPoolString(const char *s,uptr len,sPoolStringPool *p)      { Buffer = p->Add(s,len); }
    void Init(const char *s,sPoolStringPool *p)                 { Buffer = p->Add(s,sGetStringLen(s)); }
    void Init(const char *s,uptr len,sPoolStringPool *p)        { Buffer = p->Add(s,len); }

    //  sPoolString &operator=(const sPoolString &s){ Buffer = s.Buffer; return *this; }
    sPoolString &operator=(const char *s)       { Buffer = sDefaultStringPool->Add(s,sGetStringLen(s)); return *this; }

    operator const char*() const                { return Buffer; }

    const char *Get() const                     { return Buffer; }

    char operator[](int i) const                { return Buffer[i]; }
    uptr Count() const                          { return sGetStringLen(Buffer); }
    bool IsEmpty() const                        { return Buffer[0]==0; }
    void Clear()                                { Buffer = sDefaultStringPool->EmptyString; }
};


inline bool operator ==(const sPoolString &a,const sPoolString &b) { return a.Get()==b.Get(); }
inline bool operator !=(const sPoolString &a,const sPoolString &b) { return a.Get()!=b.Get(); }
inline bool operator < (const sPoolString &a,const sPoolString &b) { return sCmpString(a.Get(),b.Get())< 0; }
inline bool operator > (const sPoolString &a,const sPoolString &b) { return sCmpString(a.Get(),b.Get())> 0; }

inline bool operator ==(const char *a,const sPoolString &b) { return sCmpString(a,b.Get())==0; }
inline bool operator !=(const char *a,const sPoolString &b) { return sCmpString(a,b.Get())!=0; }
inline bool operator < (const char *a,const sPoolString &b) { return sCmpString(a,b.Get())< 0; }
inline bool operator > (const char *a,const sPoolString &b) { return sCmpString(a,b.Get())> 0; }

inline bool operator ==(const sPoolString &a,const char *b) { return sCmpString(a.Get(),b)==0; }
inline bool operator !=(const sPoolString &a,const char *b) { return sCmpString(a.Get(),b)!=0; }
inline bool operator < (const sPoolString &a,const char *b) { return sCmpString(a.Get(),b)< 0; }
inline bool operator > (const sPoolString &a,const char *b) { return sCmpString(a.Get(),b)> 0; }

/****************************************************************************/
/***                                                                      ***/
/***   reference counted strings (dynamic memory management, slow)        ***/
/***                                                                      ***/
/****************************************************************************/

class sRCString : public sRCObject
{
    uptr Size;
    char *Buffer;
protected:
    ~sRCString()                                { delete[] Buffer; }
public:
    sRCString()                                 { Size = 0; Buffer = 0; }
    sRCString(const char *str)                  { Size = 0; Buffer = 0; Set(str,sGetStringLen(str)); }
    sRCString(const char *str,uptr len)         { Size = 0; Buffer = 0; Set(str,len); }
    void Set(const char *str)                   { Set(str,sGetStringLen(str)); }
    void Set(const char *str,uptr len);
    void SetSize(uptr size);
    sRCString &operator=(const char *str)       { Set(str); return *this; }

    operator char*()                            { return Buffer; }
    operator const char*() const                { return Buffer; }
    operator sStringDesc()                      { return sStringDesc(Buffer,Size); }
    char *GetBuffer()                           { return Buffer; }
    const char *Get() const                     { return Buffer; }
    const char &operator[](int i) const         { return Buffer[i]; }
    char &operator[](int i)                     { return Buffer[i]; }
    const char &operator[](uptr i) const        { return Buffer[i]; }
    char &operator[](uptr i)                    { return Buffer[i]; }
    void Clear()                                { Set("",0); }

    sPRINTING0(PrintF,sString<sFormatBuffer> s; sFormatStringBuffer buf=sFormatStringBase(s,format);buf,Set(s););
};

inline bool operator ==(const sRCString &a,const sRCString &b) { return sCmpString(a.Get(),b.Get())==0; }
inline bool operator !=(const sRCString &a,const sRCString &b) { return sCmpString(a.Get(),b.Get())!=0; }
inline bool operator < (const sRCString &a,const sRCString &b) { return sCmpString(a.Get(),b.Get())< 0; }
inline bool operator > (const sRCString &a,const sRCString &b) { return sCmpString(a.Get(),b.Get())> 0; }

inline bool operator ==(const char *a,const sRCString &b) { return sCmpString(a,b.Get())==0; }
inline bool operator !=(const char *a,const sRCString &b) { return sCmpString(a,b.Get())!=0; }
inline bool operator < (const char *a,const sRCString &b) { return sCmpString(a,b.Get())< 0; }
inline bool operator > (const char *a,const sRCString &b) { return sCmpString(a,b.Get())> 0; }

inline bool operator ==(const sRCString &a,const char *b) { return sCmpString(a.Get(),b)==0; }
inline bool operator !=(const sRCString &a,const char *b) { return sCmpString(a.Get(),b)!=0; }
inline bool operator < (const sRCString &a,const char *b) { return sCmpString(a.Get(),b)< 0; }
inline bool operator > (const sRCString &a,const char *b) { return sCmpString(a.Get(),b)> 0; }

/****************************************************************************/
/***                                                                      ***/
/***   text log. for appending to a big buffer.                           ***/
/***                                                                      ***/
/****************************************************************************/

class sTextLog
{
protected:
    uptr Alloc;
    uptr Used;
    char *Buffer;

    sString<sFormatBuffer> PrintBuffer;
    void Grow(uptr size);
public:
    sTextLog();
    ~sTextLog();

    void Clear();
    void Set(const char *);
    const char *Get() const;
    sStringDesc sTextLog::GetStringDesc(uptr size);
    uptr GetCount() { return Used; }
    void Print(const char *);
    void Add(const char *,uptr len);
    void Prepend(const char *,uptr len);
    void PrintChar(char c);
    sPRINTING0(PrintF,sFormatStringBuffer buf=sFormatStringBase(PrintBuffer,format);buf,Print(PrintBuffer););

    void HexDump32(uptr offset,const uint *data,uptr bytes);
    void PrintHeader(const char *s);
    int CountLines();
    void TrimToLines(int n);
    void ExtendTabs(int tabsize);
    void DumpWithLineNumbers() const;
    void PrintWithLineNumbers(const char *s);

    void Serialize(sReader &s);
    void Serialize(sWriter &s);
};

class sTextBuffer : public sTextLog
{
public:
    void Delete(uptr pos);
    void Delete(uptr start,uptr end);
    bool Insert(uptr pos,int c);
    bool Insert(uptr pos,const char *text,uptr len=-1);
};

/****************************************************************************/
/***                                                                      ***/
/***   dynamic string. does what you want without being any fast          ***/
/***                                                                      ***/
/****************************************************************************/

// any good ideas?

/****************************************************************************/
/***                                                                      ***/
/***   sCommandline Parsing                                               ***/
/***                                                                      ***/
/****************************************************************************/

class sCommandlineParser
{
    struct key
    {
        const char *Name;
        sStringDesc RefStr;
        int *RefInt;
        int *WasSet;
        bool File;
        bool Optional;
        bool WasSet_;
        const char *Help;
    };
    struct limstr
    {
        const char *Data;
        uptr Len;
        bool Cmp(const char *zl);
        limstr() { Data = 0; Len=0; }
    };
    struct cmd
    {
        limstr Name;
        sArray<limstr> Values;
    };
    sArray<key> Keys;
    sArray<cmd *> Cmds;
    cmd *NoCmds;

    void ScanSpace(const char *&s);
    void ScanName(const char *&s,limstr &name);
    bool Error;
    int HelpFlag;
public:
    sCommandlineParser();
    ~sCommandlineParser();

    void AddHelp(const char *name);
    void AddSwitch(const char *name,int &ref,const char *help=0);
    void AddInt(const char *name,int &ref,int *wasset=0,const char *help=0);
    void AddString(const char *name,const sStringDesc &,int *wasset=0,const char *help=0);
    //void sAddStrings(const char *name,sBaseArray<const sStringDesc &> &,int *wasset=0);
    void AddFile(const char *name,const sStringDesc &,int *wasset=0,const char *help=0);
    //void sAddFiles(const char *name,sBaseArray<const sStringDesc &> &,int *wasset=0);
    bool Parse(bool print=1);
    bool Parse(const char *cmdline,bool print=1);
    void Help();
};


/****************************************************************************/
/***                                                                      ***/
/***   Pattern Shit                                                       ***/
/***                                                                      ***/
/****************************************************************************/

bool sMatchWildcard(const char *wild,const char *text,bool casesensitive,bool pathsensitive);

/****************************************************************************/
/***                                                                      ***/
/***   printf macros                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sPRINTING1(sSPrintF,sFormatStringBuffer buf=sFormatStringBase(arg1,format);buf,;,const sStringDesc &)
sPRINTING0(sPrintF,sString<sFormatBuffer> s; sFormatStringBuffer buf=sFormatStringBase(s,format);buf,sPrint(buf.Get());)
sPRINTING0(sDPrintF,sString<sFormatBuffer> s; sFormatStringBuffer buf=sFormatStringBase(s,format);buf,sDPrint(buf.Get());)
sPRINTING0(sFatalF,sString<sFormatBuffer> s; sFormatStringBuffer buf=sFormatStringBase(s,format);buf,sFatal(buf.Get());)
sPRINTING1(sLogF,sString<sFormatBuffer> s; sFormatStringBuffer buf=sFormatStringBase(s,format);buf,sLog(arg1,buf.Get());,const char *)

/****************************************************************************/
/***                                                                      ***/
/***   string serializaton                                                ***/
/***                                                                      ***/
/****************************************************************************/

template <int n>
inline sReader& operator| (sReader &s,sString<n> &str) { s.String(str.GetBuffer(),n); return s; }
template <int n>
inline sWriter& operator| (sWriter &s,sString<n> &str) { s.String(str.GetBuffer(),n); return s; }

inline sReader& operator| (sReader &s,const sStringDesc &desc) { s.String(desc.Buffer,desc.Size); return s; }
inline sWriter& operator| (sWriter &s,const sStringDesc &desc) { s.String(desc.Buffer,desc.Size); return s; }

inline sReader& operator| (sReader &s,sPoolString &str) { auto data = s.ReadString(); str=data; delete[] data; return s; }
inline sWriter& operator| (sWriter &s,sPoolString str) { s.WriteString(str); return s; }

inline sReader& operator| (sReader &s,sRCString *str) { auto data = s.ReadString(); str->Set(data); delete[] data; return s; }
inline sWriter& operator| (sWriter &s,sRCString *str) { s.WriteString((const char *)str); return s; }

inline sReader& operator| (sReader &s,sTextLog &log) { log.Serialize(s); return s; }
inline sWriter& operator| (sWriter &s,sTextLog &log) { log.Serialize(s); return s; }

/****************************************************************************/
} // namespace Altona2

#endif  // FILE_ALTONA2_LIBS_BASE_STRING_HPP
