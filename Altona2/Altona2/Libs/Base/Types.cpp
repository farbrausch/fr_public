/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"  

namespace Altona2 {

int RttiAtomic = 0;

/****************************************************************************/
/***                                                                      ***/
/***   Simple Functions                                                   ***/
/***                                                                      ***/
/****************************************************************************/

int sFindLowerPower(int x)
{
    int y = 0;
    while((2<<y)<=x) y++;

    return y;
}

int sFindHigherPower(int x)
{
    int y = 0;
    while((1<<y)<x) y++;

    return y;
}


uint sMakeMask(uint max)
{
    uint mask = max;
    mask |= mask>>1;
    mask |= mask>>2;
    mask |= mask>>4;
    mask |= mask>>8;
    mask |= mask>>16;
    return mask;
}

uint64 sMakeMask(uint64 max)
{
    uint64 mask = max;
    mask |= mask>>1;
    mask |= mask>>2;
    mask |= mask>>4;
    mask |= mask>>8;
    mask |= mask>>16;
    mask |= mask>>32;
    return mask;
}

/****************************************************************************/
/***                                                                      ***/
/***   Hash Functions                                                     ***/
/***                                                                      ***/
/****************************************************************************/

static uint sCRCTable[256] =
{
    0x00000000L, 0x77073096L, 0xEE0E612CL, 0x990951BAL,
    0x076DC419L, 0x706AF48FL, 0xE963A535L, 0x9E6495A3L,
    0x0EDB8832L, 0x79DCB8A4L, 0xE0D5E91EL, 0x97D2D988L,
    0x09B64C2BL, 0x7EB17CBDL, 0xE7B82D07L, 0x90BF1D91L,
    0x1DB71064L, 0x6AB020F2L, 0xF3B97148L, 0x84BE41DEL,
    0x1ADAD47DL, 0x6DDDE4EBL, 0xF4D4B551L, 0x83D385C7L,
    0x136C9856L, 0x646BA8C0L, 0xFD62F97AL, 0x8A65C9ECL,
    0x14015C4FL, 0x63066CD9L, 0xFA0F3D63L, 0x8D080DF5L,
    0x3B6E20C8L, 0x4C69105EL, 0xD56041E4L, 0xA2677172L,
    0x3C03E4D1L, 0x4B04D447L, 0xD20D85FDL, 0xA50AB56BL,
    0x35B5A8FAL, 0x42B2986CL, 0xDBBBC9D6L, 0xACBCF940L,
    0x32D86CE3L, 0x45DF5C75L, 0xDCD60DCFL, 0xABD13D59L,
    0x26D930ACL, 0x51DE003AL, 0xC8D75180L, 0xBFD06116L,
    0x21B4F4B5L, 0x56B3C423L, 0xCFBA9599L, 0xB8BDA50FL,
    0x2802B89EL, 0x5F058808L, 0xC60CD9B2L, 0xB10BE924L,
    0x2F6F7C87L, 0x58684C11L, 0xC1611DABL, 0xB6662D3DL,
    0x76DC4190L, 0x01DB7106L, 0x98D220BCL, 0xEFD5102AL,
    0x71B18589L, 0x06B6B51FL, 0x9FBFE4A5L, 0xE8B8D433L,
    0x7807C9A2L, 0x0F00F934L, 0x9609A88EL, 0xE10E9818L,
    0x7F6A0DBBL, 0x086D3D2DL, 0x91646C97L, 0xE6635C01L,
    0x6B6B51F4L, 0x1C6C6162L, 0x856530D8L, 0xF262004EL,
    0x6C0695EDL, 0x1B01A57BL, 0x8208F4C1L, 0xF50FC457L,
    0x65B0D9C6L, 0x12B7E950L, 0x8BBEB8EAL, 0xFCB9887CL,
    0x62DD1DDFL, 0x15DA2D49L, 0x8CD37CF3L, 0xFBD44C65L,
    0x4DB26158L, 0x3AB551CEL, 0xA3BC0074L, 0xD4BB30E2L,
    0x4ADFA541L, 0x3DD895D7L, 0xA4D1C46DL, 0xD3D6F4FBL,
    0x4369E96AL, 0x346ED9FCL, 0xAD678846L, 0xDA60B8D0L,
    0x44042D73L, 0x33031DE5L, 0xAA0A4C5FL, 0xDD0D7CC9L,
    0x5005713CL, 0x270241AAL, 0xBE0B1010L, 0xC90C2086L,
    0x5768B525L, 0x206F85B3L, 0xB966D409L, 0xCE61E49FL,
    0x5EDEF90EL, 0x29D9C998L, 0xB0D09822L, 0xC7D7A8B4L,
    0x59B33D17L, 0x2EB40D81L, 0xB7BD5C3BL, 0xC0BA6CADL,
    0xEDB88320L, 0x9ABFB3B6L, 0x03B6E20CL, 0x74B1D29AL,
    0xEAD54739L, 0x9DD277AFL, 0x04DB2615L, 0x73DC1683L,
    0xE3630B12L, 0x94643B84L, 0x0D6D6A3EL, 0x7A6A5AA8L,
    0xE40ECF0BL, 0x9309FF9DL, 0x0A00AE27L, 0x7D079EB1L,
    0xF00F9344L, 0x8708A3D2L, 0x1E01F268L, 0x6906C2FEL,
    0xF762575DL, 0x806567CBL, 0x196C3671L, 0x6E6B06E7L,
    0xFED41B76L, 0x89D32BE0L, 0x10DA7A5AL, 0x67DD4ACCL,
    0xF9B9DF6FL, 0x8EBEEFF9L, 0x17B7BE43L, 0x60B08ED5L,
    0xD6D6A3E8L, 0xA1D1937EL, 0x38D8C2C4L, 0x4FDFF252L,
    0xD1BB67F1L, 0xA6BC5767L, 0x3FB506DDL, 0x48B2364BL,
    0xD80D2BDAL, 0xAF0A1B4CL, 0x36034AF6L, 0x41047A60L,
    0xDF60EFC3L, 0xA867DF55L, 0x316E8EEFL, 0x4669BE79L,
    0xCB61B38CL, 0xBC66831AL, 0x256FD2A0L, 0x5268E236L,
    0xCC0C7795L, 0xBB0B4703L, 0x220216B9L, 0x5505262FL,
    0xC5BA3BBEL, 0xB2BD0B28L, 0x2BB45A92L, 0x5CB36A04L,
    0xC2D7FFA7L, 0xB5D0CF31L, 0x2CD99E8BL, 0x5BDEAE1DL,
    0x9B64C2B0L, 0xEC63F226L, 0x756AA39CL, 0x026D930AL,
    0x9C0906A9L, 0xEB0E363FL, 0x72076785L, 0x05005713L,
    0x95BF4A82L, 0xE2B87A14L, 0x7BB12BAEL, 0x0CB61B38L,
    0x92D28E9BL, 0xE5D5BE0DL, 0x7CDCEFB7L, 0x0BDBDF21L,
    0x86D3D2D4L, 0xF1D4E242L, 0x68DDB3F8L, 0x1FDA836EL,
    0x81BE16CDL, 0xF6B9265BL, 0x6FB077E1L, 0x18B74777L,
    0x88085AE6L, 0xFF0F6A70L, 0x66063BCAL, 0x11010B5CL,
    0x8F659EFFL, 0xF862AE69L, 0x616BFFD3L, 0x166CCF45L,
    0xA00AE278L, 0xD70DD2EEL, 0x4E048354L, 0x3903B3C2L,
    0xA7672661L, 0xD06016F7L, 0x4969474DL, 0x3E6E77DBL,
    0xAED16A4AL, 0xD9D65ADCL, 0x40DF0B66L, 0x37D83BF0L,
    0xA9BCAE53L, 0xDEBB9EC5L, 0x47B2CF7FL, 0x30B5FFE9L,
    0xBDBDF21CL, 0xCABAC28AL, 0x53B39330L, 0x24B4A3A6L,
    0xBAD03605L, 0xCDD70693L, 0x54DE5729L, 0x23D967BFL,
    0xB3667A2EL, 0xC4614AB8L, 0x5D681B02L, 0x2A6F2B94L,
    0xB40BBE37L, 0xC30C8EA1L, 0x5A05DF1BL, 0x2D02EF8DL
};

uint sChecksumCRC32(const uint8 *data,uptr size)
{
    uint crc = 0xffffffff;

    const uint8 *end = data+size;
    while(data<end)
        crc = sCRCTable[(crc ^ *data++) & 0xff] ^ (crc>>8);

    return crc ^ 0xfffffffe;
}


uint sHashString(const char *string)
{
    uint crc = 0xffffffff;

    while(*string)
        crc = sCRCTable[(crc ^ (*string++)) & 0xff] ^ (crc>>8);

    return crc ^ 0xfffffffe;
}

uint sHashString(const char *string,uptr len)
{
    uint crc = 0xffffffff;

    const char *end = string+len;
    while(string<end)
        crc = sCRCTable[(crc ^ (*string++)) & 0xff] ^ (crc>>8);

    return crc ^ 0xfffffffe;
}

/****************************************************************************/

uint sChecksumMurMur(const uint *data,int words)
{
    const uint m = 0x5bd1e995;

    uint h = m;
    uint k;

    for(int i=0;i<words;i++)
    {
        k = data[i];
        k *= m;
        k ^= k>>24;
        k *= m;

        h *= m;
        h ^= k;
    }

    return h;
}

void sMurMur::Begin()
{
    h = 0x5bd1e995;
}

void sMurMur::Add(const uint *data,int words)
{
    const uint m = 0x5bd1e995;
    for(int i=0;i<words;i++)
    {
        uint k = data[i];
        k *= m;
        k ^= k>>24;
        k *= m;

        h *= m;
        h ^= k;
    }
}

uint sMurMur::End()
{
    return h;
}

/****************************************************************************/

inline void sSwapEndianI(sU32 &a) {a= ((a&0xff000000)>>24)|((a&0x00ff0000)>>8)|((a&0x0000ff00)<<8)|((a&0x000000ff)<<24); }


void sChecksumMD5::Block(const uint8 *data)
{
    uint a = Hash[0];
    uint b = Hash[1];
    uint c = Hash[2];
    uint d = Hash[3];

    f( a, b, c, d,  0,  7, 0xd76aa478,(uint *)data  ); 
    f( d, a, b, c,  1, 12, 0xe8c7b756,(uint *)data  ); 
    f( c, d, a, b,  2, 17, 0x242070db,(uint *)data  ); 
    f( b, c, d, a,  3, 22, 0xc1bdceee,(uint *)data  ); 
    f( a, b, c, d,  4,  7, 0xf57c0faf,(uint *)data  ); 
    f( d, a, b, c,  5, 12, 0x4787c62a,(uint *)data  ); 
    f( c, d, a, b,  6, 17, 0xa8304613,(uint *)data  ); 
    f( b, c, d, a,  7, 22, 0xfd469501,(uint *)data  ); 
    f( a, b, c, d,  8,  7, 0x698098d8,(uint *)data  ); 
    f( d, a, b, c,  9, 12, 0x8b44f7af,(uint *)data  ); 
    f( c, d, a, b, 10, 17, 0xffff5bb1,(uint *)data  ); 
    f( b, c, d, a, 11, 22, 0x895cd7be,(uint *)data  ); 
    f( a, b, c, d, 12,  7, 0x6b901122,(uint *)data  ); 
    f( d, a, b, c, 13, 12, 0xfd987193,(uint *)data  ); 
    f( c, d, a, b, 14, 17, 0xa679438e,(uint *)data  ); 
    f( b, c, d, a, 15, 22, 0x49b40821,(uint *)data  ); 

    g( a, b, c, d,  1,  5, 0xf61e2562,(uint *)data  );
    g( d, a, b, c,  6,  9, 0xc040b340,(uint *)data  );
    g( c, d, a, b, 11, 14, 0x265e5a51,(uint *)data  );
    g( b, c, d, a,  0, 20, 0xe9b6c7aa,(uint *)data  );
    g( a, b, c, d,  5,  5, 0xd62f105d,(uint *)data  );
    g( d, a, b, c, 10,  9, 0x02441453,(uint *)data  );
    g( c, d, a, b, 15, 14, 0xd8a1e681,(uint *)data  );
    g( b, c, d, a,  4, 20, 0xe7d3fbc8,(uint *)data  );
    g( a, b, c, d,  9,  5, 0x21e1cde6,(uint *)data  );
    g( d, a, b, c, 14,  9, 0xc33707d6,(uint *)data  );
    g( c, d, a, b,  3, 14, 0xf4d50d87,(uint *)data  );
    g( b, c, d, a,  8, 20, 0x455a14ed,(uint *)data  );
    g( a, b, c, d, 13,  5, 0xa9e3e905,(uint *)data  );
    g( d, a, b, c,  2,  9, 0xfcefa3f8,(uint *)data  );
    g( c, d, a, b,  7, 14, 0x676f02d9,(uint *)data  );
    g( b, c, d, a, 12, 20, 0x8d2a4c8a,(uint *)data  );

    h( a, b, c, d,  5,  4, 0xfffa3942,(uint *)data  );
    h( d, a, b, c,  8, 11, 0x8771f681,(uint *)data  );
    h( c, d, a, b, 11, 16, 0x6d9d6122,(uint *)data  );
    h( b, c, d, a, 14, 23, 0xfde5380c,(uint *)data  );
    h( a, b, c, d,  1,  4, 0xa4beea44,(uint *)data  );
    h( d, a, b, c,  4, 11, 0x4bdecfa9,(uint *)data  );
    h( c, d, a, b,  7, 16, 0xf6bb4b60,(uint *)data  );
    h( b, c, d, a, 10, 23, 0xbebfbc70,(uint *)data  );
    h( a, b, c, d, 13,  4, 0x289b7ec6,(uint *)data  );
    h( d, a, b, c,  0, 11, 0xeaa127fa,(uint *)data  );
    h( c, d, a, b,  3, 16, 0xd4ef3085,(uint *)data  );
    h( b, c, d, a,  6, 23, 0x04881d05,(uint *)data  );
    h( a, b, c, d,  9,  4, 0xd9d4d039,(uint *)data  );
    h( d, a, b, c, 12, 11, 0xe6db99e5,(uint *)data  );
    h( c, d, a, b, 15, 16, 0x1fa27cf8,(uint *)data  );
    h( b, c, d, a,  2, 23, 0xc4ac5665,(uint *)data  );

    i( a, b, c, d,  0,  6, 0xf4292244,(uint *)data  );
    i( d, a, b, c,  7, 10, 0x432aff97,(uint *)data  );
    i( c, d, a, b, 14, 15, 0xab9423a7,(uint *)data  );
    i( b, c, d, a,  5, 21, 0xfc93a039,(uint *)data  );
    i( a, b, c, d, 12,  6, 0x655b59c3,(uint *)data  );
    i( d, a, b, c,  3, 10, 0x8f0ccc92,(uint *)data  );
    i( c, d, a, b, 10, 15, 0xffeff47d,(uint *)data  );
    i( b, c, d, a,  1, 21, 0x85845dd1,(uint *)data  );
    i( a, b, c, d,  8,  6, 0x6fa87e4f,(uint *)data  );
    i( d, a, b, c, 15, 10, 0xfe2ce6e0,(uint *)data  );
    i( c, d, a, b,  6, 15, 0xa3014314,(uint *)data  );
    i( b, c, d, a, 13, 21, 0x4e0811a1,(uint *)data  );
    i( a, b, c, d,  4,  6, 0xf7537e82,(uint *)data  );
    i( d, a, b, c, 11, 10, 0xbd3af235,(uint *)data  );
    i( c, d, a, b,  2, 15, 0x2ad7d2bb,(uint *)data  );
    i( b, c, d, a,  9, 21, 0xeb86d391,(uint *)data  );


    Hash[0] += a;
    Hash[1] += b;
    Hash[2] += c;
    Hash[3] += d;
}


void sChecksumMD5::CalcBegin()
{
    Hash[0] = 0x67452301;
    Hash[1] = 0xEFCDAB89;
    Hash[2] = 0x98BADCFE;
    Hash[3] = 0x10325476;
}

uptr sChecksumMD5::CalcAdd(const uint8 *data,uptr size)
{
    const uint8* ptr = data;
    uptr left = size;
    while(left>=64)
    {
        Block(ptr);
        ptr+=64;
        left-=64;
    }
    return ptr-data;
}

void sChecksumMD5::CalcEnd(const uint8 *data, uptr size, uptr sizeall)
{
    uptr done = CalcAdd(data,size);
    data += done;
    size -= done;

    uint8 buffer[64];
    sClear(buffer);
    sCopyMem(buffer,data,size);

    buffer[size++] = 0x80;
    if(size>56)
    {
        Block(buffer);
        sClear(buffer);
    }
    *((uint64 *)(buffer+56)) = sizeall*8;
    Block(buffer);

    // correct md5 swapping    
    sSwapEndianI(Hash[0]);
    sSwapEndianI(Hash[1]);
    sSwapEndianI(Hash[2]);
    sSwapEndianI(Hash[3]);    
}

void sChecksumMD5::Calc(const uint8 *data,uptr size)
{
    Hash[0] = 0x67452301;
    Hash[1] = 0xEFCDAB89;
    Hash[2] = 0x98BADCFE;
    Hash[3] = 0x10325476;
    uint buffer[64];

    uptr left = size;
    while(left>=64)
    {
        if(sConfigStrictAlign && (uptr(data)&3)!=0)
        {
            sCopyMem(buffer,data,64);
            Block((uint8 *)buffer);
        }
        else
        {
            Block(data);
        }
        data+=64;
        left-=64;
    }
    sClear(buffer);
    sCopyMem(buffer,data,left);
    buffer[left++] = 0x80;
    if(left>56)
    {
        Block((uint8 *)buffer);
        left = 0;
        sClear(buffer);
    }
    *((uint64 *)(((uint8 *)buffer)+56)) = size*8;
    Block((uint8 *)buffer);

    // correct md5 swapping    
    sSwapEndianI(Hash[0]);
    sSwapEndianI(Hash[1]);
    sSwapEndianI(Hash[2]);
    sSwapEndianI(Hash[3]);
    
}

bool sChecksumMD5::Check(const uint8 *data,uptr size)
{
    sChecksumMD5 c;
    c.Calc(data,size);
    return c==*this;
}

bool sChecksumMD5::operator== (const sChecksumMD5 &o)const
{
    if(o.Hash[0]!=Hash[0]) return 0;
    if(o.Hash[1]!=Hash[1]) return 0;
    if(o.Hash[2]!=Hash[2]) return 0;
    if(o.Hash[3]!=Hash[3]) return 0;
    return 1;
}

sFormatStringBuffer& operator% (sFormatStringBuffer &f, const sChecksumMD5 &md5)
{
    if (!*f.Format)
        return f;

    sFormatStringInfo info;
    f.GetInfo(info);
    info.Null = 1;
    if(info.Format != 'x' && info.Format != 'X')
        info.Format = 'x';

    f.PrintInt<uint>(info,md5.Hash[0],0);
    f.PrintInt<uint>(info,md5.Hash[1],0);
    f.PrintInt<uint>(info,md5.Hash[2],0);
    f.PrintInt<uint>(info,md5.Hash[3],0);
    f.Fill();

    return f;
}

/****************************************************************************/
/***                                                                      ***/
/***   Random Generators                                                  ***/
/***                                                                      ***/
/****************************************************************************/


void sRandom::Seed(uint seed)
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

uint sRandom::Step()
{
    const uint a = 1103515245;
    const uint c = 12345;
    kern = a*kern+c;
    return kern & 0x7fffffff;
}

uint sRandom::Int32()
{
    uint r0,r1;
    r0 = Step();
    r1 = Step();

    return r0 ^ ((r1<<16) | (r1>>16));
}

int sRandom::Int(int max_)
{
    sASSERT(max_>=1);

    uint max = uint(max_-1);
    uint mask = sMakeMask(max);

    uint v;
    do
    {
        v = Int32()&mask;    
    }
    while(v>max);

    return v;
}

float sRandom::Float(float max)
{
    return (0x7fffffff & Int32())*max/(0x8000*65536.0f);
}

/****************************************************************************/

uint sRandomMT::Step()
{
    if(Index==Count)
    {
        Reload();
        Index = 0;
    }
    uint v = State[Index++];
    v ^= (v>>11);
    v ^= (v<< 7)&0x9d2c5680UL;
    v ^= (v<<15)&0xefc60000UL;
    v ^= (v>>18);
    return v;
}

static uint mtwist(uint m,uint s0,uint s1)
{
    return m ^ ((s0&0x80000000)|(s1&0x7fffffff)) ^ (uint(-int(s1&1))&0x9908b0dfUL);
}

void sRandomMT::Reload()
{
    uint *p = State;
    for(int i=0;i<Count-Period;i++)
        p[i] = mtwist(p[i+Period],p[i+0],p[i+1]);
    for(int i=Count-Period;i<Count-1;i++)
        p[i] = mtwist(p[i+Period-Count],p[i+0],p[i+1]);
    p[Count-1] = mtwist(p[Period-1],p[Count-1],State[0]);
}

void sRandomMT::Seed(uint seed)
{
    for(int i=0;i<Count;i++)
    {
        State[i] = seed;
        seed = 1812433253UL*(seed^(seed>>30)+i);
    }
    Index = Count;
}

void sRandomMT::Seed64(uint64 seed)
{
    uint seed0 = seed&0xffffffff;
    uint seed1 = seed>>32;
    for(int i=0;i<Count;i++)
    {
        State[i] = seed0 ^ seed1;
        seed0 = 1812433253UL*(seed0^(seed0>>30)+i);
        seed1 = 1812433253UL*(seed1^(seed1>>30)+i);
    }
    Index = Count;
}

int sRandomMT::Int(int max_)
{
    sASSERT(max_>=1);

    uint max = uint(max_-1);
    uint mask = sMakeMask(max);

    uint v;
    do
    {
        v = Step()&mask;
    }
    while(v>max);

    return v;
}

float sRandomMT::Float(float max)
{
    return Step()*max/(65536.0f*65536.0f);
}

/****************************************************************************/

uint sRandomKISS::Step()
{
    uint64 t;
    uint64 a = 698769069ull;
    x = 69069*x+12345;
    y ^= y<<13; 
    y ^= y>>17; 
    y ^= y<<5;
    t = a*z+c; 
    c = t>>32;
    z = (uint) t;

    return x+y+z; 
}

void sRandomKISS::Seed(uint seed)
{
    x = seed + 123456789;
    y = seed + 362436000; 
    if(y==0) y = 362436000;
    z = seed + 521288629;
    c = 7654321;
}

float sRandomKISS::Float(float max)
{
    return Step()*max/(65536.0f*65536.0f);
}


int sRandomKISS::Int(int max_)
{
    sASSERT(max_>=1);

    uint max = uint(max_-1);
    uint mask = sMakeMask(max);

    uint v;
    do
    {
        v = Step()&mask;
    }
    while(v>max);

    return v;
}

/****************************************************************************/
/***                                                                      ***/
/***   Hashtable                                                          ***/
/***                                                                      ***/
/****************************************************************************/

sHashTableBase::sHashTableBase(int size,int nodesperblock)
{
    HashSize = size;
    HashMask = size-1;
    HashTable = new Node*[HashSize];
    for(int i=0;i<HashSize;i++)
        HashTable[i] = 0;

    NodesPerBlock = nodesperblock;
    CurrentNodeBlock = 0;
    CurrentNode = NodesPerBlock;
    FreeNodes = 0;
}

sHashTableBase::~sHashTableBase()
{
    Clear();
    delete HashTable;
}

void sHashTableBase::Clear()
{
    for(int i=0;i<HashSize;i++)
        HashTable[i] = 0;
    NodeBlocks.DeleteAll();
    FreeNodes = 0;
    CurrentNode = NodesPerBlock;
    CurrentNodeBlock = 0;
}

sHashTableBase::Node *sHashTableBase::AllocNode()
{
    // (a): use free list

    if(FreeNodes)
    {
        Node *n = FreeNodes;
        FreeNodes = n->Next;
        return n;
    }

    // (b): check blocks

    if(CurrentNode==NodesPerBlock)
    {
        CurrentNodeBlock = new Node[NodesPerBlock];
        NodeBlocks.AddTail(CurrentNodeBlock);
        CurrentNode = 0;
    }

    // (c): get next node from block

    return &CurrentNodeBlock[CurrentNode++];
}

void sHashTableBase::Add(const void *key,void *value)
{
    Node *n = AllocNode();
    n->Value = value;
    n->Key = key;
    uint hash = HashKey(key) & HashMask;
    n->Next = HashTable[hash];
    HashTable[hash] = n;
}

void *sHashTableBase::Find(const void *key)
{
    uint hash = HashKey(key) & HashMask;
    Node *n = HashTable[hash];
    while(n)
    {
        if(CompareKey(n->Key,key))
            return n->Value;
        n = n->Next;
    }
    return 0;
}

void *sHashTableBase::Rem(const void *key)
{
    uint hash = HashKey(key) & HashMask;
    Node *n = HashTable[hash];
    Node **l = &HashTable[hash];
    while(n)
    {
        if(CompareKey(n->Key,key))
        {
            *l = n->Next;         // remove from hashtable
            n->Next = FreeNodes;  // insert in free nodes list
            FreeNodes = n;
            return n->Value;      // return value
        }
        l = &n->Next;           // maintain last ptr
        n = n->Next;            // next element
    }
    return 0;
}

void sHashTableBase::ClearAndDeleteValues()
{
    Node *n;
    for(int i=0;i<HashSize;i++)
    {
        n = HashTable[i];
        while(n)
        {
            delete (uint8 *)n->Value;
            n = n->Next;
        }
    }
    Clear();
}

void sHashTableBase::ClearAndDeleteKeys()
{
    Node *n;
    for(int i=0;i<HashSize;i++)
    {
        n = HashTable[i];
        while(n)
        {
            delete (uint8 *)n->Key;
            n = n->Next;
        }
    }
    Clear();
}

void sHashTableBase::ClearAndDelete()
{
    Node *n;
    for(int i=0;i<HashSize;i++)
    {
        n = HashTable[i];
        while(n)
        {
            delete (uint8 *)n->Value;
            delete (uint8 *)n->Key;
            n = n->Next;
        }
    }
    Clear();
}

void sHashTableBase::GetAll(sArray<void *> *a)
{
    for(int i=0;i<HashSize;i++)
    {
        Node *n = HashTable[i];
        while(n)
        {
            a->AddTail(n->Value);
            n = n->Next;
        }
    }
}

/****************************************************************************/
/***                                                                      ***/
/***   sMemoryPool                                                        ***/
/***                                                                      ***/
/****************************************************************************/

sMemoryPool::sMemoryPool(uptr defaultsize)
{
    BlockSize = defaultsize;
    Start = Ptr = End = 0;
    TotalUsed = 0;
}

sMemoryPool::~sMemoryPool()
{
    Reset();
    for(int i=0;i<FreePool.GetCount();i++)
        delete[] (uint8 *)FreePool[i];
}

void sMemoryPool::Reset()
{ 
    TotalUsed = 0;
    FreePool.AddTail(UsedPool);
    UsedPool.Clear();
    if(Start)
    {
        FreePool.Add(Start);
        Start = Ptr = End = 0;
    }
}

uint8 *sMemoryPool::Alloc(uptr size,int align)
{
    sASSERT(size+align<=BlockSize);

    Ptr = sAlign(Ptr,align);
    if(Ptr+size>End)
    {
        if(Start)
            UsedPool.Add(Start);
        if(FreePool.IsEmpty())
        {
            uptr mem = uptr(new uint8[BlockSize]);
            sASSERT(mem);
            FreePool.Add(mem);
        }
        Start = FreePool.RemTail();
        sASSERT(Start);
        End = Start+BlockSize;
        Ptr = Start;
        Ptr = sAlign(Ptr,align);
    }
    uint8 *result = (uint8 *)Ptr;
    Ptr += size;
    return result;
}

uptr sMemoryPool::GetUsed()
{
    return TotalUsed;
}


char *sMemoryPool::AllocString(const char *str)
{
    if(str==0)
        return 0;
    return AllocString(str,sGetStringLen(str));
}

char *sMemoryPool::AllocString(const char *str,uptr len)
{
    if(str==0)
        return 0;
    sASSERT(len>=0);
    char *r = Alloc<char>(len+1);
    sCopyMem(r,str,len*sizeof(char));
    r[len] = 0;
    return r;
}

void sMemoryPool::Serialize(sReader &s,const char *&str)
{
    uint n = 0;
    s.Align(4);
    s.Value(n);
    if(n==0xffffffff)
    {
        str = 0;
    }
    else
    {
        char *data = Alloc<char>(n+1);
        s.SmallData(n,data);
        data[n] = 0;
        s.Align(4);
        str = data;
    }
}

void sMemoryPool::Serialize(sWriter &s,const char *&str)
{
    if(str==0)
    {
        s.Align(4);
        s.Value<uint>(0xffffffff);
        s.Align(4);
    }
    else
    {
        uptr n = sGetStringLen(str);
        sASSERT(n<0x8000000ULL);

        s.Align(4);
        s.Value(uint(n));
        s.SmallData(int(n),str);
        s.Align(4);
    }
}

/****************************************************************************/
/***                                                                      ***/
/***   Colors                                                             ***/
/***                                                                      ***/
/****************************************************************************/

uint sMulColor(uint a,uint b)
{
    return ((((a>> 0)&255)*((b>> 0)&255)>>8)<< 0)
        + ((((a>> 8)&255)*((b>> 8)&255)>>8)<< 8)
        + ((((a>>16)&255)*((b>>16)&255)>>8)<<16)
        + ((((a>>24)&255)*((b>>24)&255)>>8)<<24);
}

uint sScaleColor(uint a,int scale)
{
    return (((((a>> 0)&255)*scale)>>16)<< 0)
        + (((((a>> 8)&255)*scale)>>16)<< 8)
        + (((((a>>16)&255)*scale)>>16)<<16)
        + (((((a>>24)&255)*scale)>>16)<<24);
}

uint sScaleColorFast(uint a,int scale)
{
    uint ag = (a&0xff00ff00)>>8;
    uint rb = a&0x00ff00ff;
    uint col = ((ag*scale)&0xff00ff00) | (((rb*scale)>>8)&0x00ff00ff);
    return col;
}

uint sAddColor(uint a,uint b)
{
    return (sClamp<int>(((a>> 0)&255)+((b>> 0)&255),0,255)<< 0)
        + (sClamp<int>(((a>> 8)&255)+((b>> 8)&255),0,255)<< 8)
        + (sClamp<int>(((a>>16)&255)+((b>>16)&255),0,255)<<16)
        + (sClamp<int>(((a>>24)&255)+((b>>24)&255),0,255)<<24);
}

uint sSubColor(uint a,uint b)
{
    return (sClamp<int>(((a>> 0)&255)-((b>> 0)&255),0,255)<< 0)
        + (sClamp<int>(((a>> 8)&255)-((b>> 8)&255),0,255)<< 8)
        + (sClamp<int>(((a>>16)&255)-((b>>16)&255),0,255)<<16)
        + (sClamp<int>(((a>>24)&255)-((b>>24)&255),0,255)<<24);
}

uint sMadColor(uint mul1,uint mul2,uint add)
{
    return (sClamp<int>((((mul1>> 0)&255)*((mul2>> 0)&255)>>8)+((add>> 0)&255),0,255)<< 0)
        + (sClamp<int>((((mul1>> 8)&255)*((mul2>> 8)&255)>>8)+((add>> 8)&255),0,255)<< 8)
        + (sClamp<int>((((mul1>>16)&255)*((mul2>>16)&255)>>8)+((add>>16)&255),0,255)<<16)
        + (sClamp<int>((((mul1>>24)&255)*((mul2>>24)&255)>>8)+((add>>24)&255),0,255)<<24);
}

uint sFadeColor(int fade,uint a,uint b)
{
    int f1 = fade;
    int f0 = 0x10000-fade;
    return (( ((((a>> 0)&255)*f0)>>16)  + ((((b>> 0)&255)*f1)>>16) )<< 0)
        + (( ((((a>> 8)&255)*f0)>>16)  + ((((b>> 8)&255)*f1)>>16) )<< 8)
        + (( ((((a>>16)&255)*f0)>>16)  + ((((b>>16)&255)*f1)>>16) )<<16)
        + (( ((((a>>24)&255)*f0)>>16)  + ((((b>>24)&255)*f1)>>16) )<<24);
}

/****************************************************************************/

uint sColorFade (uint a, uint b, float f)
{
    uint f1 = uint(f*256);
    uint f0 = 256-uint(f*256);

    return (sClamp((((a>>24)&0xff)*f0+((b>>24)&0xff)*f1)/256,uint(0),uint(255)) << 24) |
        (sClamp((((a>>16)&0xff)*f0+((b>>16)&0xff)*f1)/256,uint(0),uint(255)) << 16) |
        (sClamp((((a>> 8)&0xff)*f0+((b>> 8)&0xff)*f1)/256,uint(0),uint(255)) <<  8) |
        (sClamp((((a>> 0)&0xff)*f0+((b>> 0)&0xff)*f1)/256,uint(0),uint(255)) <<  0);
}

uint sPMAlphaFade (uint c, float f)
{
    uint f0 = uint(f*255);
    return (((((c>> 0)&255)*f0)>>8)<< 0)
        + (((((c>> 8)&255)*f0)>>8)<< 8)
        + (((((c>>16)&255)*f0)>>8)<<16)
        + (((((c>>24)&255)*f0)>>8)<<24);  
}

uint sAlphaFade (uint c, float f)
{
    uint f0 = uint(f*256);
    return (sClamp((((c >> 24) & 0xff)*f0)/256,uint(0),uint(255)) << 24)|(c&0xffffff);
}

/****************************************************************************/
/***                                                                      ***/
/***   Garbage Collection                                                 ***/
/***                                                                      ***/
/****************************************************************************/

int sGCObject::TotalCount;
int sRCObject::TotalCount;

sGCObject::sGCObject()
{
    TagFlag = 1;
    sGC->AddObject(this);
    TotalCount++;
}

sGCObject::~sGCObject()
{
    TotalCount--;
}

void sGCObject::Need()
{
    if(this && TagFlag)
    {
        TagFlag = 0;
        sGC->ObjectStack[0].Add(this);
    }
}

/****************************************************************************/

sGarbageCollector::sGarbageCollector()
{
    TriggerTime = sGetTimeMS()+5*60*1000;
}

sGarbageCollector::~sGarbageCollector()
{
    sASSERT(Roots.GetCount()==0);
    CollectNow();
}

void sGarbageCollector::AddObject(sGCObject *o)
{
    Objects.Add(o);
}

void sGarbageCollector::AddRoot(sGCObject *o)
{
    Roots.Add(o);
}

void sGarbageCollector::RemRoot(sGCObject *o)
{
    Roots.Rem(o);
}


void sGarbageCollector::CollectRecursion()
{
    ObjectStack[0].Clear();
    ObjectStack[1].Clear();

    for(auto o : Roots)
        o->Need();
    TagHook.Call();

    while(!ObjectStack[0].IsEmpty())
    {
        ObjectStack[0].Exchange(ObjectStack[1]);
        for(auto o : ObjectStack[1])
            o->Tag();
        ObjectStack[1].Clear();
    }
}

void sGarbageCollector::CollectNow()
{
    sZONE("Garbage Collector",0xffff0000);
    if(Objects.GetCount()>0)
    {
        for(auto o : Objects)
            o->TagFlag = 1;

        CollectRecursion();

        // Objects.DeleteOrderIf(&sGCObject::TagFlag); // don't have access to destructor!
        int d=0; 
        sGCObject **Data = Objects.GetData();
        for(int s=0;s<Objects.GetCount();s++) 
        { 
            if(!(Data[s]->TagFlag)) 
                Data[d++] = Data[s]; 
            else 
                ObjectsToDelete.Add(Data[s]);
        } 
        int n = Objects.GetCount()-d;
        if(n>0)
            sLogF("sys","garbage collector deleted %d objects",n);
        Objects.SetSize(d);

        for(auto o : ObjectsToDelete)
            o->Finalize();
        for(auto o : ObjectsToDelete)
            delete o;
        ObjectsToDelete.Clear();
    }

    TriggerTime = sGetTimeMS()+5*60*1000;   // every 5 minutes
}

void sGarbageCollector::Trigger()
{
    TriggerTime = sMin(TriggerTime,sGetTimeMS()+2*1000);    // 2 seconds, or sooner
}

void sGarbageCollector::TriggerNow()
{
    TriggerTime = sGetTimeMS()-1;           // really, NOW!
}

void sGarbageCollector::CollectIfTriggered()
{
    if(sGetTimeMS()>TriggerTime)
        CollectNow();
}

/****************************************************************************/

class sGarbageCollectorSubsystem : public sSubsystem
{
public:
    sGarbageCollectorSubsystem() : sSubsystem("Garbage Collection",0x08) {}

    void Init()
    {
        sGC = new sGarbageCollector;
    }
    void Exit()
    {
        delete sGC;
    }
} sGarbageCollectorSubsystem_;

sGarbageCollector *sGC = 0;

/****************************************************************************/

}
