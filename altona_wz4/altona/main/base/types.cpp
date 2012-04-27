/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "base/types.hpp"
#include "base/types2.hpp" 
#include "base/system.hpp"
#include "base/graphics.hpp"

#if sPLATFORM==sPLAT_WINDOWS && sCONFIG_64BIT
// for _controlfp87 
#include <float.h>
#endif 

/****************************************************************************/

#define sCFG_MEMDBG_LOCKING 0   // enable for sMemDbgLock/sMemDbgUnlock debugging

/****************************************************************************/


/* this is most likely never used 
#if sCONFIG_PLATFORM_LINUX
void __debugbreak() {};
#endif 
*/

static sChar emptyString[] = L"";
static sInt  sLogPrefixLength = 8; // length of sLogF and sLog prefix indentation "[???]   ..."

/****************************************************************************/
/***                                                                      ***/
/***   Basic Types and Functions                                          ***/
/***                                                                      ***/
/****************************************************************************/

sInt sFindLowerPower(sInt x)
{
  sInt y = 0;
  while((2<<y)<=x) y++;

  return y;
};

sInt sFindHigherPower(sInt x)
{
  sInt y = 0;
  while((1<<y)<x) y++;

  return y;
};

sU32 sMakeMask(sU32 max)
{
  sU32 mask = max;
  mask |= mask>>1;
  mask |= mask>>2;
  mask |= mask>>4;
  mask |= mask>>8;
  mask |= mask>>16;
  return mask;
}

sU64 sMakeMask(sU64 max)
{
  sU64 mask = max;
  mask |= mask>>1;
  mask |= mask>>2;
  mask |= mask>>4;
  mask |= mask>>8;
  mask |= mask>>16;
  mask |= mask>>32;
  return mask;
}

sU32 sPart1By1(sU32 x) // "inserts" a 0 bit between each of the low 16 bits of x
{
                                    // x = ---- ---- ---- ---- fedc ba98 7654 3210 (bits)
  x = (x ^ (x <<  8)) & 0x00ff00ff; // x = ---- ---- fedc ba98 ---- ---- 7654 3210
  x = (x ^ (x <<  4)) & 0x0f0f0f0f; // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
  x = (x ^ (x <<  2)) & 0x33333333; // x = --fe --dc --ba --98 --76 --54 --32 --10
  x = (x ^ (x <<  1)) & 0x55555555; // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
  return x;
}

sU32 sPart1By2(sU32 x) // "inserts" two 0 bits between each of the low 10 bits of x
{
                                    // x = ---- ---- ---- ---- ---- --98 7654 3210 (bits)
  x = (x ^ (x << 16)) & 0xff0000ff; // x = ---- --98 ---- ---- ---- ---- 7654 3210
  x = (x ^ (x <<  8)) & 0x0300f00f; // x = ---- --98 ---- ---- 7654 ---- ---- 3210
  x = (x ^ (x <<  4)) & 0x030c30c3; // x = ---- --98 ---- 76-- --54 ---- 32-- --10
  x = (x ^ (x <<  2)) & 0x09249249; // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
  return x;
}

sU32 sCompact1By1(sU32 x) // inverse of sPart1By1 - "delete" all odd-indexed bits
{
  x &= 0x55555555;                  // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0 (bits)
  x = (x ^ (x >>  1)) & 0x33333333; // x = --fe --dc --ba --98 --76 --54 --32 --10
  x = (x ^ (x >>  2)) & 0x0f0f0f0f; // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
  x = (x ^ (x >>  4)) & 0x00ff00ff; // x = ---- ---- fedc ba98 ---- ---- 7654 3210
  x = (x ^ (x >>  8)) & 0x0000ffff; // x = ---- ---- ---- ---- fedc ba98 7654 3210
  return x;
}

sU32 sCompact1By2(sU32 x) // inverse of sPart1By2 - "delete" two bits after every bit
{
  x &= 0x09249249;                  // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0 (bits)
  x = (x ^ (x >>  2)) & 0x030c30c3; // x = ---- --98 ---- 76-- --54 ---- 32-- --10
  x = (x ^ (x >>  4)) & 0x0300f00f; // x = ---- --98 ---- ---- 7654 ---- ---- 3210
  x = (x ^ (x >>  8)) & 0xff0000ff; // x = ---- --98 ---- ---- ---- ---- 7654 3210
  x = (x ^ (x >> 16)) & 0x000003ff; // x = ---- ---- ---- ---- ---- --98 7654 3210
  return x;
}

/****************************************************************************/

sU64 sHashStringFNV(const sChar *text)
{
  sU64 hash = 0x84222325cbf29ce4ULL;
  const sU64 prime = 0x00000100000001b3ULL;

  while(*text)
  {
    sU16 tmp = (sU16) *text++;

    hash ^= (tmp >> 0) & 0xff;
    hash *= prime;
    hash ^= (tmp >> 8) & 0xff;
    hash *= prime;
  }
  return hash;
}

static const sInt Adler32_Base = 65521;
static sU32 Adler32_s1=1;
static sU32 Adler32_s2=0;

sU32 sChecksumAdler32(const sU8 *data,sInt size)
{
  // this can be optimised, see wikipedia...


  sU32 s1 = 1;
  sU32 s2 = 0;
  const sU8 *end = data+size;
  while(data<end)
  {
    s1 = (s1+(*data++)) % Adler32_Base; 
    s2 = (s2+s1) % Adler32_Base;
  }

  return (s2<<16) + s1;
}

void sChecksumAdler32Begin()
{
  Adler32_s1 = 1;
  Adler32_s2 = 0;
}

void sChecksumAdler32Add(const sU8 *data,sInt size)
{
  const sU8 *end = data+size;
  while(data<end)
  {
    Adler32_s1 = (Adler32_s1+(*data++)) % Adler32_Base;
    Adler32_s2 = (Adler32_s2+Adler32_s1) % Adler32_Base;
  }
}

sU32 sChecksumAdler32End()
{
  return (Adler32_s2<<16) + Adler32_s1;
}

/****************************************************************************/

static sU32 sCRCTable[256] =
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

sU32 sChecksumCRC32(const sU8 *data,sInt size)
{
  sU32 crc = 0xffffffff;

  const sU8 *end = data+size;
  while(data<end)
    crc = sCRCTable[(crc ^ *data++) & 0xff] ^ (crc>>8);

  return crc ^ 0xfffffffe;
}

sU32 sChecksumRandomByte(const sU8 *data,sInt size)
{
  sU32 csum = 0;

  const sU8 *end = data+size;
  while(data<end)
    csum = sCRCTable[*data++] ^ csum;

  return csum;
}

sU32 sChecksumMurMur(const sU32 *data,sInt words)
{
  const sU32 m = 0x5bd1e995;

	sU32 h = m;
	sU32 k;

	for(sInt i=0;i<words;i++)
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

/****************************************************************************/

void sChecksumMD5::Block(const sU8 *data)
{
  sU32 a = Hash[0];
  sU32 b = Hash[1];
  sU32 c = Hash[2];
  sU32 d = Hash[3];

  f( a, b, c, d,  0,  7, 0xd76aa478,(sU32 *)data  ); 
  f( d, a, b, c,  1, 12, 0xe8c7b756,(sU32 *)data  ); 
  f( c, d, a, b,  2, 17, 0x242070db,(sU32 *)data  ); 
  f( b, c, d, a,  3, 22, 0xc1bdceee,(sU32 *)data  ); 
  f( a, b, c, d,  4,  7, 0xf57c0faf,(sU32 *)data  ); 
  f( d, a, b, c,  5, 12, 0x4787c62a,(sU32 *)data  ); 
  f( c, d, a, b,  6, 17, 0xa8304613,(sU32 *)data  ); 
  f( b, c, d, a,  7, 22, 0xfd469501,(sU32 *)data  ); 
  f( a, b, c, d,  8,  7, 0x698098d8,(sU32 *)data  ); 
  f( d, a, b, c,  9, 12, 0x8b44f7af,(sU32 *)data  ); 
  f( c, d, a, b, 10, 17, 0xffff5bb1,(sU32 *)data  ); 
  f( b, c, d, a, 11, 22, 0x895cd7be,(sU32 *)data  ); 
  f( a, b, c, d, 12,  7, 0x6b901122,(sU32 *)data  ); 
  f( d, a, b, c, 13, 12, 0xfd987193,(sU32 *)data  ); 
  f( c, d, a, b, 14, 17, 0xa679438e,(sU32 *)data  ); 
  f( b, c, d, a, 15, 22, 0x49b40821,(sU32 *)data  ); 

  g( a, b, c, d,  1,  5, 0xf61e2562,(sU32 *)data  );
  g( d, a, b, c,  6,  9, 0xc040b340,(sU32 *)data  );
  g( c, d, a, b, 11, 14, 0x265e5a51,(sU32 *)data  );
  g( b, c, d, a,  0, 20, 0xe9b6c7aa,(sU32 *)data  );
  g( a, b, c, d,  5,  5, 0xd62f105d,(sU32 *)data  );
  g( d, a, b, c, 10,  9, 0x02441453,(sU32 *)data  );
  g( c, d, a, b, 15, 14, 0xd8a1e681,(sU32 *)data  );
  g( b, c, d, a,  4, 20, 0xe7d3fbc8,(sU32 *)data  );
  g( a, b, c, d,  9,  5, 0x21e1cde6,(sU32 *)data  );
  g( d, a, b, c, 14,  9, 0xc33707d6,(sU32 *)data  );
  g( c, d, a, b,  3, 14, 0xf4d50d87,(sU32 *)data  );
  g( b, c, d, a,  8, 20, 0x455a14ed,(sU32 *)data  );
  g( a, b, c, d, 13,  5, 0xa9e3e905,(sU32 *)data  );
  g( d, a, b, c,  2,  9, 0xfcefa3f8,(sU32 *)data  );
  g( c, d, a, b,  7, 14, 0x676f02d9,(sU32 *)data  );
  g( b, c, d, a, 12, 20, 0x8d2a4c8a,(sU32 *)data  );

  h( a, b, c, d,  5,  4, 0xfffa3942,(sU32 *)data  );
  h( d, a, b, c,  8, 11, 0x8771f681,(sU32 *)data  );
  h( c, d, a, b, 11, 16, 0x6d9d6122,(sU32 *)data  );
  h( b, c, d, a, 14, 23, 0xfde5380c,(sU32 *)data  );
  h( a, b, c, d,  1,  4, 0xa4beea44,(sU32 *)data  );
  h( d, a, b, c,  4, 11, 0x4bdecfa9,(sU32 *)data  );
  h( c, d, a, b,  7, 16, 0xf6bb4b60,(sU32 *)data  );
  h( b, c, d, a, 10, 23, 0xbebfbc70,(sU32 *)data  );
  h( a, b, c, d, 13,  4, 0x289b7ec6,(sU32 *)data  );
  h( d, a, b, c,  0, 11, 0xeaa127fa,(sU32 *)data  );
  h( c, d, a, b,  3, 16, 0xd4ef3085,(sU32 *)data  );
  h( b, c, d, a,  6, 23, 0x04881d05,(sU32 *)data  );
  h( a, b, c, d,  9,  4, 0xd9d4d039,(sU32 *)data  );
  h( d, a, b, c, 12, 11, 0xe6db99e5,(sU32 *)data  );
  h( c, d, a, b, 15, 16, 0x1fa27cf8,(sU32 *)data  );
  h( b, c, d, a,  2, 23, 0xc4ac5665,(sU32 *)data  );

  i( a, b, c, d,  0,  6, 0xf4292244,(sU32 *)data  );
  i( d, a, b, c,  7, 10, 0x432aff97,(sU32 *)data  );
  i( c, d, a, b, 14, 15, 0xab9423a7,(sU32 *)data  );
  i( b, c, d, a,  5, 21, 0xfc93a039,(sU32 *)data  );
  i( a, b, c, d, 12,  6, 0x655b59c3,(sU32 *)data  );
  i( d, a, b, c,  3, 10, 0x8f0ccc92,(sU32 *)data  );
  i( c, d, a, b, 10, 15, 0xffeff47d,(sU32 *)data  );
  i( b, c, d, a,  1, 21, 0x85845dd1,(sU32 *)data  );
  i( a, b, c, d,  8,  6, 0x6fa87e4f,(sU32 *)data  );
  i( d, a, b, c, 15, 10, 0xfe2ce6e0,(sU32 *)data  );
  i( c, d, a, b,  6, 15, 0xa3014314,(sU32 *)data  );
  i( b, c, d, a, 13, 21, 0x4e0811a1,(sU32 *)data  );
  i( a, b, c, d,  4,  6, 0xf7537e82,(sU32 *)data  );
  i( d, a, b, c, 11, 10, 0xbd3af235,(sU32 *)data  );
  i( c, d, a, b,  2, 15, 0x2ad7d2bb,(sU32 *)data  );
  i( b, c, d, a,  9, 21, 0xeb86d391,(sU32 *)data  );


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

sInt sChecksumMD5::CalcAdd(const sU8 *data,sInt size)
{
  const sU8* ptr = data;
  sInt left = size;
  while(left>=64)
  {
    Block(ptr);
    ptr+=64;
    left-=64;
  }
  return ptr-data;
}

void sChecksumMD5::CalcEnd(const sU8 *data, sInt size, sInt sizeall)
{
  sInt done = CalcAdd(data,size);
  data += done;
  size -= done;

  sU8 buffer[64];
  sClear(buffer);
  sCopyMem(buffer,data,size);
  
  buffer[size++] = 0x80;
  if(size>56)
  {
    Block(buffer);
    sClear(buffer);
  }
  *((sU64 *)(buffer+56)) = sizeall*8;
  Block(buffer);

  // correct md5 swapping
  sSwapEndianI(Hash[0]);
  sSwapEndianI(Hash[1]);
  sSwapEndianI(Hash[2]);
  sSwapEndianI(Hash[3]);
}

void sChecksumMD5::Calc(const sU8 *data,sInt size)
{
  Hash[0] = 0x67452301;
  Hash[1] = 0xEFCDAB89;
  Hash[2] = 0x98BADCFE;
  Hash[3] = 0x10325476;
  sU8 buffer[64];

  sInt left = size;
  while(left>=64)
  {
    Block(data);
    data+=64;
    left-=64;
  }
  sClear(buffer);
  sCopyMem(buffer,data,left);
  buffer[left++] = 0x80;
  if(left>56)
  {
    Block(buffer);
    left = 0;
    sClear(buffer);
  }
  *((sU64 *)(buffer+56)) = size*8;
  Block(buffer);

  // correct md5 swapping
  sSwapEndianI(Hash[0]);
  sSwapEndianI(Hash[1]);
  sSwapEndianI(Hash[2]);
  sSwapEndianI(Hash[3]);
}

sBool sChecksumMD5::Check(const sU8 *data,sInt size)
{
  sChecksumMD5 c;
  c.Calc(data,size);
  return c==*this;
}

sBool sChecksumMD5::operator== (const sChecksumMD5 &o)const
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

  f.PrintInt<sU32>(info,md5.Hash[0],0);
  f.PrintInt<sU32>(info,md5.Hash[1],0);
  f.PrintInt<sU32>(info,md5.Hash[2],0);
  f.PrintInt<sU32>(info,md5.Hash[3],0);
  f.Fill();

  return f;
}

/****************************************************************************/
/***                                                                      ***/
/***   Assembler                                                          ***/
/***                                                                      ***/
/****************************************************************************/

#if sINTRO

#if !sCONFIG_ASM_MSC_IA32 || !sCONFIG_ASM_87
sCOMPILEERROR("need msc inline assembly");
#endif


sF64 sFMod(sF64 a,sF64 b)
{
  __asm
  {
    fld   qword ptr [b];
    fld   qword ptr [a];
    fprem;

    fstp  st(1);
    fstp  qword ptr [a];
  }

  return a;
}

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

#endif //sPC_64

/****************************************************************************/
/***                                                                      ***/
/***   Float / Ascii Conversion                                           ***/
/***                                                                      ***/
/****************************************************************************/

void sFloatInfo::FloatToAscii(sU32 fu,sInt digits)
{
  sInt e2 = ((fu&0x7f800000)>>23);
  sU32 man = fu & 0x007fffff;
  sInt e10 = 0;
  const sInt shift=28;
  
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
      for(sInt i=0;i<digits;i++)
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
  for(sInt i=0;i<digits;i++)
  {
    Digits[i]= '0'+(man>>shift);
    man = (man&((1UL<<shift)-1));
    man = (man<<3) + (man<<1);
  }

  // done

  Digits[digits] = 0;
  Exponent = e10;
}

void sFloatInfo::DoubleToAscii(sU64 fu,sInt digits)
{
  sInt e2 = ((fu&0x7ff0000000000000ULL)>>52);
  sU64 man = fu &0x000fffffffffffffULL;
  sInt e10 = 0;
  const sInt shift=60;
  
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
      for(sInt i=0;i<digits;i++)
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
  for(sInt i=0;i<digits;i++)
  {
    Digits[i]= '0'+(man>>shift);
    man = (man&((1ULL<<shift)-1));
    man = man*10;
  }

  // done

  Digits[digits] = 0;
  Exponent = e10;
}

sU32 sFloatInfo::AsciiToFloat()
{
  sU64 man;
  sInt e10;
  sInt e2;
  const sInt shift=60;
  const sU64 highmask = ~((1ULL<<shift)-1);
  const sU64 highmask1 = ~((1ULL<<(shift-1))-1);
  const sU64 highmask0 = ~((1ULL<<(shift+1))-1);
  sU32 fu;

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
  for(sInt i=0;Digits[i];i++)
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

sU64 sFloatInfo::AsciiToDouble()
{
  sU64 man;
  sInt e10;
  sInt e2;
  const sInt shift=60;
  const sU64 highmask = ~((1ULL<<shift)-1);
  const sU64 highmask1 = ~((1ULL<<(shift-1))-1);
  const sU64 highmask0 = ~((1ULL<<(shift+1))-1);
  sU64 fu;

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
  for(sInt i=0;Digits[i];i++)
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

  fu |= 0x7ff0000000000000ULL & (sU64(e2)<<52);
  fu |= 0x000fffffffffffffULL & ((man+((1ULL<<(shift-53))-1))>>(shift-52));

ende:

  return fu;
}

/****************************************************************************/

void sFloatInfo::PrintE(const sStringDesc &desc)
{
  if(Infinite)
    sSPrintF(desc,L"%c#inf",Negative?'-':'+');
  else if(NaN)
    sSPrintF(desc,L"%c#nan%d",Negative?'-':'+',NaN);
  else
    sSPrintF(desc,L"%c%c.%se%d",Negative?'-':'+',Digits[0],Digits+1,Exponent);
}


void sFloatInfo::PrintF(const sStringDesc &desc,sInt fractions)
{
  if(Infinite || NaN || Exponent>50 || Exponent<-50)
  {
    PrintE(desc);
    return;
  }

  sChar *s = desc.Buffer;
  sChar *e = desc.Buffer+desc.Size-1;

  sInt exp = Exponent+1;
  sInt dig = 0;
  if(Negative)
    *s++ = '-';
  sChar *sx = s;

  

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
      sInt frac = fractions;
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
        for(sChar *d = s-1;d>=sx;d--)
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
        for(sChar *d = s-1;d>=sx;d--)
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
    sCopyString(desc,L"###");
}

/****************************************************************************/
/***                                                                      ***/
/***   Strings and Memory                                                 ***/
/***                                                                      ***/
/****************************************************************************/


void sCopyString(sChar *d,const sChar *s,sInt size)
{
  size--;
  while(size>0 && *s)
  {
    size--;
    *d++ = *s++;
  }
  *d = 0;
}

void sCopyString(sChar8* d,const sChar *s,sInt c)
{
  while(c>0 && ((*d++=(sChar8)*s++))!=0) c--;
  if(c==0)
    d[-1]=0;
}

void sCopyString(sChar *d, const sChar8* s,sInt c)
{
  while(c>0 && ((*d++=(*s++)&0xff))!=0) c--;
  if(c==0)
    d[-1]=0;
}

void sCopyString(sChar8* d, const sChar8* s,sInt c)
{
  while(c>0 && ((*d++=(*s++)&0xff))!=0) c--;
  if(c==0)
    d[-1]=0;
}

//Encodes the 2byte unicode string into UTF-8 byte array
void sCopyStringToUTF8(sChar8 *d, const sChar *s, sInt size)
{
  const sChar MASKBITS   = 0x3F;
  const sChar MASKBYTE   = 0x80;
  const sChar MASK2BYTES = 0xC0;
  const sChar MASK3BYTES = 0xE0;

  size--;
  while(size > 0 && *s)
  {        
    if(*s < 0x80)
    {
      // 0xxxxxx -> 1byte
      size--;
      *d++ = (sChar8)*s++;
    }    
    else if(*s < 0x800)
    {
      size -= 2;
      if (size < 0)
        break;

      // 110xxxxx 10xxxxxx -> 2byte
      *d++ = (sChar8)(MASK2BYTES | (*s >> 6));
      *d++ = (sChar8)(MASKBYTE   | (*s & MASKBITS));
      s++;
    }    
    else  //(*s < 0x10000)
    {
      size -= 3;
      if (size < 0)
        break;

      // 1110xxxx 10xxxxxx 10xxxxxx -> 3byte
      *d++ = (sChar8)(MASK3BYTES | (*s >> 12));
      *d++ = (sChar8)(MASKBYTE   | ((*s >> 6) & MASKBITS));
      *d++ = (sChar8)(MASKBYTE   | (*s & MASKBITS));
      s++;
    }

  }
  *d = 0;
}


//Decodes the 0 terminated UTF-8 byte array into an 2byte unicode string. 
//Supports up to 3 byte UTF8 sequences -> longer ones wouldn't fit into 2byte sChar.
//'size' is the size of 'd'
void sCopyStringFromUTF8(sChar *d, const sChar8 *s, sInt size)
{
  const sChar MASK4BITS  = 0x0F;
  const sChar MASK5BITS  = 0x1F;
  const sChar MASK6BITS  = 0x3F;
  const sChar MASKBYTE   = 0x80;
  const sChar MASK2BYTES = 0xC0;
  const sChar MASK3BYTES = 0xE0;

  sInt shiftBits = 0;

  size--;
  while(size > 0 && *s)
  {
    if (!(*s & MASKBYTE))                       // ASCII character -> direct copy
    {      
      size--;
      *d++ = *s++;
    }
    else if ((*s & MASK3BYTES) == MASK3BYTES)   // Start of a 3byte character sequence
    {
      shiftBits = 6;
      *d = ( ((*s & MASK4BITS) << 12) );      
      s++;
    }
    else if ((*s & MASK2BYTES) == MASK2BYTES)   // Start of a 2byte character sequence
    {
      shiftBits = 0;
      *d = ( ((*s & MASK5BITS) << 6) );
      s++;         
    }
    else if ((*s & MASKBYTE) == MASKBYTE)       // In between byte
    {
      // add the six bits to the destination character  
      *d |= ( (*s & MASK6BITS) << shiftBits );
      s++;

      shiftBits -= 6;
      if (shiftBits < 0)  // reached the end of the sequence (either 2 or 3 bytes), start new character
      {
        shiftBits = 0;
        size--;
        d++;
      }
    }               
    else
    {
      // unsupported sequence / character. this may produce aukward results...
      shiftBits = 0;
      size--;
      *d++ = '?';
      s++;
    }
  }
  *d = 0;
}

/****************************************************************************/

sChar sUpperChar(sChar c)
{
  if(c>=0xe0 && c<=0xfd && c!=247) return c-0x20;
  if(c>='a'  && c<='z'           ) return c-0x20;
  return c;
}

sChar sLowerChar(sChar c)
{
  if(c>=0xc0 && c<=0xdd && c!=215) return c+0x20;
  if(c>='A'  && c<='Z'           ) return c+0x20;
  return c;
}

sChar *sMakeUpper(sChar * s)
{
  sChar * c = s;
  while (*c > 0)
  {
    *c = sUpperChar(*c);
    c++;
  }
  return s;
}

sChar *sMakeLower(sChar * s)
{
  sChar * c = s;
  while (*c > 0) 
  {
    *c = sLowerChar(*c);
    c++;
  }
  return s;
}

/****************************************************************************/

void sAppendString(sChar *d,const sChar *s,sInt size)
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

void sAppendString2(sChar *d,const sChar *s,sInt size,sInt len)
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

void sAppendPath(sChar *d,const sChar *s,sInt size)
{
  // absolute path?
  if(sIsAbsolutePath(s))
  {
    sCopyString(d,s,size);
    return;
  }

  // delete trailing "/" from destination path
  sInt p0 = sMax(sFindLastChar(d,'/'),sFindLastChar(d,'\\'));
  if (p0>=0 && p0==sGetStringLen(d)-1)
    d[p0]=0;

  // eat up leading "../"
  while(sCmpMem(s,L"../",sizeof(sChar)*3)==0 ||sCmpMem(s,L"..\\",sizeof(sChar)*3)==0)
  {
    p0 = sMax(sFindLastChar(d,'/'),sFindLastChar(d,'\\'));
    if(p0>0)
      d[p0]=0;
    else
      *d=0;
    s+=3;
  }


  // relative path, append, and insert '/' if required

  size--;
  if(*d!=0)
  {
    while(size>0 && *d)
    {
      size--;
      d++;
    }
    if(d[-1]!='\\' && d[-1]!='/' && size>0)
    {
      *d++ = '/';
      size--;
    }
  }
  while(size>0 && *s)
  {
    size--;
    *d++ = *s++;
  }
  *d = 0;
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
    aa = *a++; if(aa>='a' && aa<='z') aa=aa-'a'+'A';
    bb = *b++; if(bb>='a' && bb<='z') bb=bb-'z'+'Z';
  }
  while(aa!=0 && aa==bb);
  return sSign(aa-bb);
}

sInt sCmpStringP(const sChar *a,const sChar *b)
{
  sInt aa,bb;
  do
  {
    aa = *a++; if(aa>='A' && aa<='Z') aa=aa-'A'+'a'; if(aa=='\\') aa='/';
    bb = *b++; if(bb>='A' && bb<='Z') bb=bb-'A'+'a'; if(bb=='\\') bb='/';
  }
  while(aa!=0 && aa==bb);
  return sSign(aa-bb);
}

sInt sCmpStringLen(const sChar *a,const sChar *b,sInt len)
{
  sInt aa,bb;
  do
  {
    if(len--==0) return 0;
    aa = *a++; 
    bb = *b++; 
  }
  while(aa!=0 && aa==bb);
  return sSign(aa-bb);
}

sInt sCmpStringILen(const sChar *a,const sChar *b,sInt len)
{
  sInt aa,bb;
  do
  {
    if(len--==0) return 0;
    aa = *a++; if(aa>='a' && aa<='z') aa=aa-'a'+'A';
    bb = *b++; if(bb>='a' && bb<='z') bb=bb-'z'+'Z';
  }
  while(aa!=0 && aa==bb);
  return sSign(aa-bb);
}

sInt sCmpStringPLen(const sChar *a,const sChar *b,sInt len)
{
  sInt aa,bb;
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

sInt sFindString(const sChar *f,const sChar *s)
{
  sInt testlen = sGetStringLen(f);
  sInt findlen = sGetStringLen(s);

  for (sInt i = 0; i <= testlen-findlen; i++)
  {
    if (sCmpMem(f+i,s,findlen*sizeof(sChar)) == 0)
      return i;
  }
  return -1;
}

/****************************************************************************/

sInt sFindStringI(const sChar *f,const sChar *s)
{
  sInt testlen = sGetStringLen(f);
  sInt findlen = sGetStringLen(s);

  for (sInt i = 0; i <= testlen-findlen; i++)
  {
    if (sCmpStringILen(f+i,s,findlen) == 0)
      return i;
  }
  return -1;
}

/****************************************************************************/

sInt sFindFirstChar(const sChar *f,sInt c)
{
  for(sInt i=0;f[i];i++)
    if(f[i]==sChar(c))
      return i;
  return -1;
}

/****************************************************************************/

sInt sFindLastChar(const sChar *f,sInt c)
{
  sInt best = -1;
  for(sInt i=0;f[i];i++)
    if(f[i]==sChar(c))
      best = i;
  return best;
}

/****************************************************************************/

sInt sFindNthChar(const sChar *f, sInt c, sInt n)
{
  for(sInt i=0;f[i];i++)
  {
    if(f[i]==sChar(c))
    {
      if (n==0) 
        return i;
      else 
        n--;
    }
  }
  return -1;
}

/****************************************************************************/

sInt sFindChar(const sChar * str, sChar character, sInt startPos)
{
  const sChar * startStr = &str[startPos];
  sInt charPos = sFindFirstChar(startStr, character);
  if (charPos >= 0) return startPos + charPos;
  return -1;
}

/****************************************************************************/

const sChar *sFindFileExtension(const sChar *a)
{
  const sChar *result=0;

  while(*a)
  {
    if(*a=='.')
      result = a+1;
    a++;
  }
  if(result)
    return result;
  else 
    return L"";
}

sBool sExtractFileExtension(const sStringDesc &d, const sChar *a, sInt nr/*=0*/)
{
  sInt count = 0;
  const sChar *ptr = a;
  while(*ptr)
  {
    if(*ptr++ == '.')
      count++;
  }

  if(nr>=count)
    return sFALSE;

  ptr = a;
  while(*ptr && count>nr)
  {
    if(*ptr++=='.')
      count--;
  }

  sChar *dest = d.Buffer;
  count = d.Size;

  while(*ptr && *ptr != '.' && count>1)
    *dest++ = *ptr++;
  *dest = 0;

  return !*ptr || *ptr == '.';
}


sBool sCheckFileExtension(const sChar *name,const sChar *ext)
{
  if(!name)
    return sFALSE;
  sInt len1 = sGetStringLen(name);
  sInt len2 = sGetStringLen(ext);
  if(len1<len2) return 0;
  return name[len1-len2-1]=='.' && sCmpStringI(name+len1-len2,ext)==0;
}

sChar *sFindFileExtension(sChar *a)
{
  sChar *result=0;

  while(*a)
  {
    if(*a=='.')
      result = a+1;
    a++;
  }
  if(result)
    return result;
  else 
    return emptyString;
}

const sChar *sFindFileWithoutPath(const sChar *a)
{
  const sChar *result=a;

  while(*a)
  {
    if(*a=='/' || *a=='\\' || *a==':')
      result = a+1;
    a++;
  }

  return result;
}

sBool sIsAbsolutePath(const sChar *a)
{
  if(a[0]!=0   && a[1]==':') return 1;      // does not handle "cdrom:"
  if(a[0]=='\\' /*&& a[1]=='\\'*/) return 1;
  if(a[0]=='/' /*&& a[1]=='/'*/) return 1;
  return 0;
}

sBool sExtractPathDrive(const sChar *path, const sStringDesc &drive)
{
  sVERIFY(drive.Size);
  const sChar *ptr = path;
  sInt count = 0;
  while(*ptr && *ptr!=':' && count<drive.Size-1)
    drive.Buffer[count++]=*ptr++;

  if(*ptr && *ptr==':')
  {
    drive.Buffer[count] = 0;
    return sTRUE;
  }
  drive.Buffer[0] = 0;
  return sFALSE;
}

void sExtractPath(const sChar *a, const sStringDesc &path)
{
  const sChar *b=sFindFileWithoutPath(a);
  sCopyString(path.Buffer,a,sMin<sDInt>(path.Size,b-a+1));
}

void sExtractPath(const sChar *a, const sStringDesc &path, const sStringDesc &filename)
{
  const sChar *b=sFindFileWithoutPath(a);
  sCopyString(path.Buffer,a,sMin<sDInt>(path.Size,b-a+1));
  sCopyString(filename,b);
}

void sMakeRelativePath(const sStringDesc &path, const sChar *relTo)
{
  // check drive/protocol letters
  sChar *p=path.Buffer;
  sInt pcp=sFindFirstChar(path.Buffer,':');
  sInt rcp=sFindFirstChar(relTo,':');
  if (rcp<0)
  {
    if (pcp>=0) return; // we're lost. we don't know on what drive the rel path is.
  }
  else
  {
    if (pcp>=0)
    {
      if (rcp!=pcp || sCmpStringILen(p,relTo,pcp-1)) return; // different drive or protocol
      sCopyString(path,p+pcp+1); // cut off everything
    }
    relTo+=rcp+1;
  }

  sInt plen=sGetStringLen(p);
  sInt rlen=sGetStringLen(relTo);

  // check for double slashes at beginning (URLs or network shares)
  if (plen>=2 && ((p[0]=='/' && p[1]=='/') || (p[0]=='\\' || p[1]=='\\')))
  {
    if (rlen>=2 && ((relTo[0]=='/' && relTo[1]=='/') || (relTo[0]=='\\' || relTo[1]=='\\')))
    {
      relTo+=2;
      rlen-=2;
      sCopyString(path,p+2);
      plen-=2;
    }
    else return; // we'll have to give up here.
  }

  // now simply cut off the common parts
  while (*p &&  *relTo)
  {
    sInt pc=sLowerChar(*p); if (pc=='\\') pc='/';
    sInt rc=sLowerChar(*relTo); if (pc=='\\') pc='/';
    if (pc!=rc) break;
    p++;
    relTo++;
  }

  sCopyString(path,p);
}


sInt sCountChar(const sChar *str,sChar c)
{
  sInt count = 0;
  while(*str)
    if(*str++ == c)
      count++;
  return count;
}

// find in a string like L"bli|bla|blub"

sInt sFindChoice(const sChar *str,const sChar *choices)
{
  sInt i = 0;
  sInt len;
  sInt len2 = sGetStringLen(str);
  while(*choices)
  {
    while(*choices=='|')
    {
      choices++;
      i++;
    }
    len = 0;
    while(choices[len]!=0 && choices[len]!='|')
      len++;
    if(len==len2 && sCmpMem(str,choices,len*sizeof(sChar))==0)
      return i;
    choices+=len;
  }
  return -1;
}

sBool sFindFlag(const sChar *str,const sChar *choices,sInt &mask_,sInt &value_)
{
  const sChar *s = choices;
  sInt len2 = sGetStringLen(str);

  while(*s)
  {
    sInt max = 0; 
    sInt shift = 0;
    sInt found = 0;
    sInt value = 0;

    if(*s=='*')
    {
      s++;
      sScanInt(s,shift);
    }
    for(;;)
    {
      while(*s=='|')
      {
        s++;
        value++;
      }
      if(*s==':' || *s==0)
        break;

      if(sIsDigit(*s))
        sScanInt(s,value);
      if(*s==' ')
        s++;

      sInt len = 0;
      while(s[len]!='|' && s[len]!=':' && s[len]!=0) len++;
      if(len2==len && sCmpMem(str,s,len*sizeof(sChar))==0)
      {
        value_ = value<<shift;
        found = 1;
      }
      s += len;

      max = sMax(max,value);
    }

    if(found)
    {
      mask_ = 1;
      while(mask_ < max)
        mask_ = mask_*2+1;
      mask_ <<= shift;
      return 1;
    }

    if(*s==':')
      s++;
  }
  return 0;
}

/*
sChar * sMakeChoice(sChar * choice, const sChar * choices, sInt index)
{
  choice[0] = 0;
  sInt charPos;
  if (index > 0)
  {
    charPos = sFindNthChar(choices, '|', index-1);
    if (charPos < 0) return sNULL;
    choices += charPos+1;
  }
  else
  {
    charPos = sFindFirstChar(choices, ':');
    if (charPos >= 0)
      choices += charPos+1;
  }
  charPos = sFindFirstChar(choices, '|');
  if (charPos < 0) charPos = sGetStringLen(choices);
  sCopyString(choice, choices, charPos+1);
  return choice;
}
*/
sInt sCountChoice(const sChar *choices)
{
  if(*choices==0)
  {
    return 0;
  }
  else
  {
    sInt i = 1;

    while(*choices)
      if(*choices++ == '|')
        i ++;

    return i;
  }
}

void sMakeChoice(const sStringDesc &desc,const sChar *choices,sInt index)
{
  while(index>0)
  {
    while(*choices!='|' && *choices!=0) choices++;
    if(*choices=='|') choices++;
    index--;
  }
  sInt n=0;
  while(choices[n]!='|' && choices[n]!=0) n++;
  
  sChar *d = desc.Buffer;
  for(sInt i=0;i<desc.Size-1 && i<n;i++)
    *d++ = choices[i];
  *d++ = 0;
}

sStringDesc sGetAppendDesc(const sStringDesc &sd)
{
  sStringDesc result = sd;
  sInt length = sGetStringLen(sd.Buffer);
  result.Buffer += length;
  result.Size   -= length;
  return result;
}

sBool sCheckPrefix(const sChar *string,const sChar *prefix)
{
  for(sInt i=0;prefix[i];i++)
    if(string[i]!=prefix[i])
      return 0;
  return 1;
}

sBool sCheckSuffix(const sChar *string,const sChar *suffix)
{
  sInt len1 = sGetStringLen(string);
  sInt len2 = sGetStringLen(suffix);

  if(len2>len1)
    return 0;
  for(sInt i=0;i<len2;i++)
    if(string[len1-len2+i]!=suffix[i])
      return 0;
  return 1;
}

sInt sReplaceChar(sChar *string, sChar from, sChar to)
{
  if (!string) return 0;
  sInt count=0;
  for (;*string;string++)
  {
    if (*string==from)
    {
      *string=to;
      count++;
    }
  }
  return count;
}

/****************************************************************************/

// ?=any one character,  *=zero or more characters
sBool sMatchWildcard(const sChar *wild,const sChar *text,sBool casesensitive,sBool pathsensitive)
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
      sInt a = *text;
      sInt b = *wild;

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

void sReadString(sU32 *&data,sChar *buffer,sInt size)
{
  sInt len = *data++;
  sInt chars = sMin(len,size-1);

  sCopyMem(buffer,data,chars*2);
  buffer[chars] = 0;

  data += (len+1)/2;
}

void sWriteString(sU32 *&data,const sChar *buffer)
{
  sInt len = sGetStringLen(buffer);
  sVERIFY(len<0x7ffe);

  *data++ = len;
  len = sAlign(len,2);
  sCopyMem(data,buffer,len*2);
  data += len/2;
}

/****************************************************************************/

sBool sIsName(const sChar *s)
{
  if(!sIsLetter(*s++)) return 0;
  sInt c;
  while((c = (*s++))!=0)
    if(!sIsLetter(c) && !sIsDigit(c)) 
      return 0;
  return 1;
}

sU32 sHashString(const sChar *string)
{
	sU32 crc = 0xffffffff;

  while(*string)
  {
    crc = sCRCTable[(crc ^ (*string++)) & 0xff] ^ (crc>>8);
  }

  return crc ^ 0xfffffffe;
}

sU32 sHashString(const sChar *string,sInt len)
{
	sU32 crc = 0xffffffff;

  while(len>0)
  {
    crc = sCRCTable[(crc ^ (*string++)) & 0xff] ^ (crc>>8);
    len--;
  }

  return crc ^ 0xfffffffe;
}

/****************************************************************************/

sBool sScanInt(const sChar *&s,sInt &result)
{
  sU32 val = 0;
  sBool sign = 0;
  sInt overflow = 0;
  sU32 digit;

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
    result = -(sInt)val;
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

#if sCONFIG_64BIT
sBool sScanInt(const sChar *&s,sDInt &result)
{
  sU64 val = 0;
  sBool sign = 0;
  sInt overflow = 0;
  sU64 digit;

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
    result = -(sInt)val;
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
#endif

sBool sScanFloat(const sChar *&s,sF32 &result)
{
  sF64 val = 0;
  sF64 dec = 1;
  sInt sign = 1;

  // scan sign

  if(*s=='-')
  {
    s++;
    sign = -1;
  }
  else if(*s=='+')
  {
    s++;
  }

  // here we must have either '.' or digit.

  if(!sIsDigit(*s) && *s!='.')
    return 0;

  // scan integer part

  while(sIsDigit(*s))
  {
    val = val * 10 + ((*s++)&15);
  }

  // optional fractional part

  if(*s=='.')
  {
    s++;
    while(sIsDigit(*s))
    {
      dec = dec * 10;
      val += ((*s++)&15) / dec;
    }
  }

  // optional exponent

  if(*s=='e' || *s=='E')
  {
    s++;

    // optional exponent sign
    sInt eSign = 1;
    if(*s == '-')
      s++, eSign = -1;
    else if(*s == '+')
      s++;

    // exponent itself
    if(!sIsDigit(*s))
      return 0;

    sInt eVal = 0;
    while(sIsDigit(*s))
      eVal = eVal * 10 + (*s++ - '0');

    val *= sFPow(10.0f,eSign * eVal);
  }

  // apply sign and write out

  result = sF32(val * sign);

  // done

  return 1;
}

sBool sScanHex(const sChar *&s,sInt &result, sInt maxlen)
{
  sU32 val;
  sInt c;

  if(!sIsHex(*s)) return 0;

  if(s[0]=='0' && (s[1]=='x' || s[1]=='X'))      // skip leading 0x
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

sBool sScanMatch(const sChar *&scan, const sChar *match)
{
  const sChar *sp=scan;
  while (*match)
    if (*sp++!=*match++) return sFALSE;
  scan=sp;
  return sTRUE;
}

sBool sScanGUID(const sChar *&str, sGUID &guid)
{
  const sChar *p=str;
  sInt temp;

  if (!sScanHex(p,temp,8)) return sFALSE;
  guid.Data32=temp;
  if (*p++!='-') return sFALSE;
  for (sInt i=0; i<3; i++)
  {
    if (!sScanHex(p,temp,4)) return sFALSE;
    guid.Data16[i]=temp;
    if (*p++!='-') return sFALSE;
  }
  for (sInt i=0; i<6; i++)
  {
    if (!sScanHex(p,temp,2)) return sFALSE;
    guid.Data8[i]=temp;
  }

  str=p;
  return sTRUE;
}

/****************************************************************************/
/***                                                                      ***/
/***   String Formatting, varargs                                         ***/
/***                                                                      ***/
/****************************************************************************/
/*
extern "C" char * __cdecl _fcvt( double value, int count, int *dec, int *sign );
extern "C" char * __cdecl _ecvt( double value, int count, int *dec, int *sign );

sBool sFormatString(const sStringDesc &desc,const sChar *s,const sChar **fp)
{
  sInt c;
  sInt field0;
  sInt field1;
  sInt minus;
  sInt null;
  sInt len;
  sChar buffer[64];
  char *convert;
  sChar *string;
  sInt val;
  sInt arg;
  sInt sign;
  sInt i;
  sF64 fval;

  sChar *d = desc.Buffer;
  sInt left = desc.Size;
  static sChar hex[17] = L"0123456789abcdef";
  static sChar HEX[17] = L"0123456789ABCDEF";

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
          fval = sVARARGF(fp,arg);arg+=2;
        }
        else
        {
          val = sVARARG(fp,arg);arg++;
        }
        if(c=='f')        // this is preliminary!!!!!!!
        {
#if sINTRO
          convert = "???";
#else
          if(fval<0)
            field1++;
          convert = _fcvt(fval,field1,&i,&sign);
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
          while(*convert)
          {
            if(i==0)
              buffer[len++]='.';
            i--;
            buffer[len++]=*convert++;
          }
        }
        else if(c=='e')
        {
#if sINTRO
          convert = "???";
#else
          if(fval<0)
            field1++;
          convert = _ecvt(fval,field1,&i,&sign);
#endif
          if(*convert)
          {
            buffer[len++] = *convert++;
            buffer[len++] = '.';
            while(*string)
              buffer[len++] = *convert++;
            
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
          if(sU32(val)==0x80000000)
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
*/
/****************************************************************************/
/*
void __cdecl sSPrintF(const sStringDesc &desc,const sChar *format,...)
{
  sFormatString(desc,format,&format);
}
*/

/****************************************************************************/

#if sENABLE_DPRINT

void (*sPrintScreenCB)(const sChar *text) = 0;
void sPrintScreen(const sChar *text)
{
  if(text)
    sDPrint(text);
  if(sPrintScreenCB)
    (*sPrintScreenCB)(text);
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   String Formatting, type-safe                                       ***/
/***                                                                      ***/
/****************************************************************************/

sBool sFormatStringBuffer::Fill()
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

  sVERIFY(Dest<End);

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
    Format = L"";
    return 0;
  }
}

void sFormatStringBuffer::GetInfo(sFormatStringInfo &info)
{
  sInt c;

  info.Format = 0;
  info.Field = 0;
  info.Fraction = -1;
  info.Minus = 0;
  info.Null = 0;
  info.Truncate = 0;

  if(!Fill())
    return;

  sVERIFY(*Format=='%');
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

  sVERIFY(info.Format);
}

void sFormatStringBuffer::Add(const sFormatStringInfo &info,const sChar *buffer,sBool sign)
{
  sInt len = sGetStringLen(buffer);
  sInt field = info.Field;

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

void sFormatStringBuffer::Print(const sChar *str)
{
  while(Dest<End-1 && *str)
    *Dest++ = *str++;
}

template<typename Type>
void sFormatStringBuffer::PrintInt(const sFormatStringInfo &info,Type val,sBool sign)
{
  sChar buf[32];
  static sChar hex[17] = L"0123456789abcdef";
  static sChar HEX[17] = L"0123456789ABCDEF";
  static sChar units[] = L" kmgtpe";
  static sChar UNITS[] = L" KMGTPE";
  sInt len=0,komma=0,unit=0;

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
    *Dest++ = sign ? -sInt(val) : val;
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
        sInt weeks = val / 40;
        val -= weeks*40;
        if(weeks>=10)
        {
          buf[len++] = weeks/10+'0';
          weeks = weeks % 10;
        }
        buf[len++] = weeks+'0';
        buf[len++] = 'w';
      }
      if(val>8)
      {
        sInt days = val/8;
        val -= days*8;
        buf[len++] = days+'0';
        buf[len++] = 'd';
      }
      if(val>0)
      {
        buf[len++] = val+'0';
        buf[len++] = 'h';
      }
    }
    buf[len++] = 0;
    Add(info,buf,sign);
    break;
  }
}

void sFormatStringBuffer::PrintFloat(const sFormatStringInfo &info,sF32 v)
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
      fi.FloatToAscii(sRawCast<sU32,sF32>(v));
      sInt sign = fi.Negative;
      fi.Negative = 0;

      sInt frac = info.Fraction;
      sInt field = info.Field;

      if(field==0)
        field = 7;
      if(frac==-1)
        frac = sClamp(field-2-fi.Exponent-sign,0,field-2);
      if(info.Format=='F' && frac>0)
        frac--;

      fi.PrintF(buf,frac);
      if(info.Format=='F')     // special format: add trailing 'f'
      {
        sBool addf = 0;
        for(sInt n=0;buf[n];n++)
          if(buf[n]=='.')
            addf = 1;
        if(addf==1)
          buf.Add(L"f");
      }

      Add(info,buf,sign); 
    }
    break;

  default:
    if(v>=0)
      PrintInt(info,sU64(v),0);
    else
      PrintInt(info,sU64(-v),1);
    break;
  }
}

void sFormatStringBuffer::PrintFloat(const sFormatStringInfo &info,sF64 v)
{
  sFloatInfo fi;
  sString<256> buf;

  fi.DoubleToAscii(sRawCast<sU64,sF64>(v));
  sInt sign = fi.Negative;
  fi.Negative = 0;


  sInt frac = info.Fraction;
  sInt field = info.Field;

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

// sNOINLINE for size and a VS2005 compiler error

sNOINLINE sFormatStringBuffer sFormatStringBase(const sStringDesc &buffer,const sChar *format)
{
  sFormatStringBuffer buf;
  buf.Start = buffer.Buffer;
  buf.Dest = buffer.Buffer;
  buf.End = buffer.Buffer + buffer.Size;
  buf.Format = format;

  buf.Fill();

  return buf;
}

sNOINLINE void sFormatStringBaseCtx(sFormatStringBuffer &buf,const sChar *format)
{
  sThreadContext *tx = sGetThreadContext();

  buf.Start = tx->PrintBuffer;
  buf.Dest = tx->PrintBuffer;
  buf.End = tx->PrintBuffer + sCOUNTOF(tx->PrintBuffer);
  buf.Format = format;

  buf.Fill();
}

sFormatStringBuffer& operator% (sFormatStringBuffer &f,sInt val)
{
  if (!*f.Format)
    return f;

  sBool sign=(val<0);
  if(sign) val = -val;

  sFormatStringInfo info;

  f.GetInfo(info);
  f.PrintInt<sU32>(info,sU32(val),sign);
  f.Fill();

  return f;
}

sFormatStringBuffer& operator% (sFormatStringBuffer &f,sU32 val)
{
  if (!*f.Format)
    return f;

  sFormatStringInfo info;

  f.GetInfo(info);
  f.PrintInt<sU32>(info,val,0);
  f.Fill();

  return f;
}

sFormatStringBuffer& operator% (sFormatStringBuffer &f,sU64 val)
{
  if (!*f.Format)
    return f;

  sFormatStringInfo info;

  f.GetInfo(info);
  f.PrintInt<sU64>(info,val,0);
  f.Fill();

  return f;
}

sFormatStringBuffer& operator% (sFormatStringBuffer &f,sS64 val)
{
  if (!*f.Format)
    return f;

  sBool sign=(val<0);
  if(sign) val = -val;

  sFormatStringInfo info;

  f.GetInfo(info);
  f.PrintInt<sU64>(info,sU64(val),sign);
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
  info.Field = sCONFIG_64BIT ? 16 : 8;
  f.PrintInt<sU64>(info,sU64(ptr),0);
  f.Fill();

  return f;
}

sFormatStringBuffer& operator% (sFormatStringBuffer &f,sF32 v)
{
  if (!*f.Format)
    return f;

  sFormatStringInfo info;

  f.GetInfo(info);
  f.PrintFloat(info,v);
  f.Fill();

  return f;
}

sFormatStringBuffer& operator% (sFormatStringBuffer &f,sF64 v)
{
  if (!*f.Format)
    return f;

  sFormatStringInfo info;

  f.GetInfo(info);
  f.PrintFloat(info,v);
  f.Fill();

  return f;
}

sFormatStringBuffer& operator% (sFormatStringBuffer &f,const sChar *str)
{
  sString<sMAXPATH> path;
  sInt i;
  sBool q=1;
  const sChar *s;
  if (!*f.Format)
    return f;

  if (!str) str=L"<NULL>";

  sFormatStringInfo info;
  f.GetInfo(info);
  if (str)
  {
    switch(info.Format)
    {
    case 'c':     // lower case
      i=0;
      while(i<sMAXPATH-2 && *str)
      {
        sChar c = *str++;
        if(c>='A' && c<='Z') c=c+'a'-'A';
        path[i++] = c;
      }
      path[i++] = 0;
      f.Add(info,path,0);  
      break;
    case 'C':     // upper case
      i=0;
      while(i<sMAXPATH-2 && *str)
      {
        sChar c = *str++;
        if(c>='a' && c<='z') c=c+'A'-'a';
        path[i++] = c;
      }
      path[i++] = 0;
      f.Add(info,path,0);  
      break;
    case 'p':
      i=0;
      while(i<sMAXPATH-2 && *str)
      {
        sChar c = *str++;
        if(c=='\\') c='/';
//          if(c=='"') path[i++] = '\\';
        path[i++] = c;
      }
      path[i++] = 0;
      f.Add(info,path,0);  
      break;
    case 'P':
      i=0;
      while(i<sMAXPATH-2 && *str)
      {
        sChar c = *str++;
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
        sChar *d = path;
        *d++ = '\"';
        while(d < path+sMAXPATH-3 && *str)
        {
          switch(*str)
          {
          case '"':
            *d++ = '\\';
            *d++ = '"';
            break;
          case '\\':
            *d++ = '\\';
            *d++ = '\\';
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

//sString<1024> sXDPrintFBuffer;

/****************************************************************************/
/***                                                                      ***/
/***   Small Structures                                                   ***/
/***                                                                      ***/
/****************************************************************************/

sFormatStringBuffer& operator% (sFormatStringBuffer &f, const sRect &r)
{
  if (!*f.Format)
    return f;

  sFormatStringInfo info;
  f.GetInfo(info);

  sBool sign;

  sign = (r.x0<0);
  f.PrintInt<sU32>(info,sign?-r.x0:r.x0,sign);
  f.Print(L" ");
  sign = (r.y0<0);
  f.PrintInt<sU32>(info,sign?-r.y0:r.y0,sign);
  f.Print(L"-");
  sign = (r.x1<0);
  f.PrintInt<sU32>(info,sign?-r.x1:r.x1,sign);
  f.Print(L" ");
  sign = (r.y1<0);
  f.PrintInt<sU32>(info,sign?-r.y1:r.y1,sign);
  f.Print(L" ");
  f.Fill();

  return f;
}

sFormatStringBuffer& operator% (sFormatStringBuffer &f, const sFRect &r)
{
  if (!*f.Format)
    return f;

  sFormatStringInfo info;
  f.GetInfo(info);

  f.PrintFloat(info,r.x0);
  f.Print(L" ");
  f.PrintFloat(info,r.y0);
  f.Print(L"-");
  f.PrintFloat(info,r.x1);
  f.Print(L" ");
  f.PrintFloat(info,r.y1);
  f.Print(L" ");
  f.Fill();

  return f;
}

/****************************************************************************/

sFRect sRectToFRect(const sRect &src)
{
  return sFRect(src.x0, src.y0, src.x1, src.y1);
}

/****************************************************************************/

sRect sFRectToRect(const sFRect &src)
{
  return sRect(sInt(src.x0), sInt(src.y0), sInt(src.x1), sInt(src.y1));
}

/****************************************************************************/

void sRandom::Seed(sU32 seed)
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

sU32 sRandom::Step()
{
  const sU32 a = 1103515245;
  const sU32 c = 12345;
  kern = a*kern+c;
  return kern & 0x7fffffff;
}

sU32 sRandom::Int32()
{
  sU32 r0,r1;
  r0 = Step();
  r1 = Step();

  return r0 ^ ((r1<<16) | (r1>>16));
}

sInt sRandom::Int(sInt max_)
{
  sVERIFY(max_>=1);

  sU32 max = sU32(max_-1);
  sU32 mask = sMakeMask(max);

  sU32 v;
  do
  {
    v = Int32()&mask;    
  }
  while(v>max);

  return v;
}

sF32 sRandom::Float(sF32 max)
{
  return (0x7fffffff & Int32())*max/(0x8000*65536.0f);
}

/****************************************************************************/

sU32 sRandomMT::Step()
{
  if(Index==Count)
  {
    Reload();
    Index = 0;
  }
  sU32 v = State[Index++];
  v ^= (v>>11);
  v ^= (v<< 7)&0x9d2c5680UL;
  v ^= (v<<15)&0xefc60000UL;
  v ^= (v>>18);
  return v;
}

static sU32 mtwist(sU32 m,sU32 s0,sU32 s1)
{
  return m ^ ((s0&0x80000000)|(s1&0x7fffffff)) ^ (sU32(-sInt(s1&1))&0x9908b0dfUL);
}

void sRandomMT::Reload()
{
  sU32 *p = State;
  for(sInt i=0;i<Count-Period;i++)
    p[i] = mtwist(p[i+Period],p[i+0],p[i+1]);
  for(sInt i=Count-Period;i<Count-1;i++)
    p[i] = mtwist(p[i+Period-Count],p[i+0],p[i+1]);
  p[Count-1] = mtwist(p[Period-1],p[Count-1],State[0]);
}

void sRandomMT::Seed(sU32 seed)
{
  for(sInt i=0;i<Count;i++)
  {
    State[i] = seed;
    seed = 1812433253UL*(seed^(seed>>30)+i);
  }
  Index = Count;
}

sInt sRandomMT::Int(sInt max_)
{
  sVERIFY(max_>=1);

  sU32 max = sU32(max_-1);
  sU32 mask = sMakeMask(max);

  sU32 v;
  do
  {
    v = Step()&mask;
  }
  while(v>max);

  return v;
}

sF32 sRandomMT::Float(sF32 max)
{
  return Step()*max/(65536.0f*65536.0f);
}

/****************************************************************************/

sU32 sRandomKISS::Step()
{
  sU64 t;
  sU64 a = 698769069ull;
  x = 69069*x+12345;
  y ^= y<<13; 
  y ^= y>>17; 
  y ^= y<<5;
  t = a*z+c; 
  c = t>>32;
  z = (sU32) t;

  return x+y+z; 
}

void sRandomKISS::Seed(sU32 seed)
{
  x = seed + 123456789;
  y = seed + 362436000; 
  if(y==0) y = 362436000;
  z = seed + 521288629;
  c = 7654321;
}

sF32 sRandomKISS::Float(sF32 max)
{
  return Step()*max/(65536.0f*65536.0f);
}


sInt sRandomKISS::Int(sInt max_)
{
  sVERIFY(max_>=1);

  sU32 max = sU32(max_-1);
  sU32 mask = sMakeMask(max);

  sU32 v;
  do
  {
    v = Step()&mask;
  }
  while(v>max);

  return v;
}

/****************************************************************************/

sTiming::sTiming()
{
  TotalTime = 0;
  LastTime = 0;
  FirstFrame = 1;
  Slices = 0;
  TimeSlice = 10;
  Delta = 0;
  Jitter = 0;
  Timestamp = 0;

  sClear(History);
  HistoryIndex = 0;
  HistoryCount = 0;
  HistorySkip = 4;
  HistoryMax = sCOUNTOF(History);
}

void sTiming::SetTimeSlice(sInt ts)
{
  TimeSlice = ts;
}

void sTiming::OnFrame(sInt time)
{
  StopMeasure(time); 
  StartMeasure(time);
}

void sTiming::StartMeasure(sInt time)
{
  LastTime = time;
}

void sTiming::StopMeasure(sInt time)
{
  if(FirstFrame)
  {
    LastTime = time;
    Timestamp = time;
    FirstFrame = 0;
  }

  Slices = time/TimeSlice - LastTime/TimeSlice;
  Jitter = time%TimeSlice;
  Delta = time - LastTime;
  TotalTime += Delta;

  //  sDPrintF(L"time %d - last %d delta %d total %d\n",time,LastTime,Delta,TotalTime);

  if(HistorySkip==0)
  {
    History[HistoryIndex] = Delta;
    HistoryIndex = (HistoryIndex+1)%HistoryMax;
    HistoryCount = sMin((sInt)HistoryCount+1,HistoryMax);
  }
  else
  {
    HistorySkip--;
  }
}

sF32 sTiming::GetAverageDelta() const
{
  sF32 avg = 0;

  if(HistoryCount>0)
  {
    for(sInt i=0;i<HistoryCount;i++)
      avg += sF32(History[i]);

    avg = avg / HistoryCount;
  }
  if(avg<0.001f) avg = 0.001f; // at least 1 microsec, to avoid division by zero!

  return avg;
}

/****************************************************************************/
/***                                                                      ***/
/***   Colors                                                             ***/
/***                                                                      ***/
/****************************************************************************/

sU32 sMulColor(sU32 a,sU32 b)
{
  return ((((a>> 0)&255)*((b>> 0)&255)>>8)<< 0)
       + ((((a>> 8)&255)*((b>> 8)&255)>>8)<< 8)
       + ((((a>>16)&255)*((b>>16)&255)>>8)<<16)
       + ((((a>>24)&255)*((b>>24)&255)>>8)<<24);
}

sU32 sScaleColor(sU32 a,sInt scale)
{
  return (((((a>> 0)&255)*scale)>>16)<< 0)
       + (((((a>> 8)&255)*scale)>>16)<< 8)
       + (((((a>>16)&255)*scale)>>16)<<16)
       + (((((a>>24)&255)*scale)>>16)<<24);
}

sU32 sScaleColorFast(sU32 a,sInt scale)
{
  sU32 ag = (a&0xff00ff00)>>8;
  sU32 rb = a&0x00ff00ff;
  sU32 col = ((ag*scale)&0xff00ff00) | (((rb*scale)>>8)&0x00ff00ff);
  return col;
}

sU32 sAddColor(sU32 a,sU32 b)
{
  return (sClamp<sInt>(((a>> 0)&255)+((b>> 0)&255),0,255)<< 0)
       + (sClamp<sInt>(((a>> 8)&255)+((b>> 8)&255),0,255)<< 8)
       + (sClamp<sInt>(((a>>16)&255)+((b>>16)&255),0,255)<<16)
       + (sClamp<sInt>(((a>>24)&255)+((b>>24)&255),0,255)<<24);
}

sU32 sSubColor(sU32 a,sU32 b)
{
  return (sClamp<sInt>(((a>> 0)&255)-((b>> 0)&255),0,255)<< 0)
       + (sClamp<sInt>(((a>> 8)&255)-((b>> 8)&255),0,255)<< 8)
       + (sClamp<sInt>(((a>>16)&255)-((b>>16)&255),0,255)<<16)
       + (sClamp<sInt>(((a>>24)&255)-((b>>24)&255),0,255)<<24);
}

sU32 sMadColor(sU32 mul1,sU32 mul2,sU32 add)
{
  return (sClamp<sInt>((((mul1>> 0)&255)*((mul2>> 0)&255)>>8)+((add>> 0)&255),0,255)<< 0)
       + (sClamp<sInt>((((mul1>> 8)&255)*((mul2>> 8)&255)>>8)+((add>> 8)&255),0,255)<< 8)
       + (sClamp<sInt>((((mul1>>16)&255)*((mul2>>16)&255)>>8)+((add>>16)&255),0,255)<<16)
       + (sClamp<sInt>((((mul1>>24)&255)*((mul2>>24)&255)>>8)+((add>>24)&255),0,255)<<24);
}

sU32 sFadeColor(sInt fade,sU32 a,sU32 b)
{
  sInt f1 = fade;
  sInt f0 = 0x10000-fade;
  return (( ((((a>> 0)&255)*f0)>>16)  + ((((b>> 0)&255)*f1)>>16) )<< 0)
       + (( ((((a>> 8)&255)*f0)>>16)  + ((((b>> 8)&255)*f1)>>16) )<< 8)
       + (( ((((a>>16)&255)*f0)>>16)  + ((((b>>16)&255)*f1)>>16) )<<16)
       + (( ((((a>>24)&255)*f0)>>16)  + ((((b>>24)&255)*f1)>>16) )<<24);
}

/****************************************************************************/

sU32 sColorFade (sU32 a, sU32 b, sF32 f)
{
  sU32 f1 = sU32(f*256);
  sU32 f0 = 256-sU32(f*256);

  return (sClamp((((a>>24)&0xff)*f0+((b>>24)&0xff)*f1)/256,sU32(0),sU32(255)) << 24) |
         (sClamp((((a>>16)&0xff)*f0+((b>>16)&0xff)*f1)/256,sU32(0),sU32(255)) << 16) |
         (sClamp((((a>> 8)&0xff)*f0+((b>> 8)&0xff)*f1)/256,sU32(0),sU32(255)) <<  8) |
         (sClamp((((a>> 0)&0xff)*f0+((b>> 0)&0xff)*f1)/256,sU32(0),sU32(255)) <<  0);
}

sU32 sPMAlphaFade (sU32 c, sF32 f)
{
  sU32 f0 = sU32(f*255);
  return (((((c>> 0)&255)*f0)>>8)<< 0)
       + (((((c>> 8)&255)*f0)>>8)<< 8)
       + (((((c>>16)&255)*f0)>>8)<<16)
       + (((((c>>24)&255)*f0)>>8)<<24);  
}

sU32 sAlphaFade (sU32 c, sF32 f)
{
  sU32 f0 = sU32(f*256);
  return (sClamp((((c >> 24) & 0xff)*f0)/256,sU32(0),sU32(255)) << 24)|(c&0xffffff);
}

sF32 sScaleFade(sF32 a, sF32 b, sF32 f)
{
  if (f <= 0.0f)
    return a;
  if (f >= 1.0f)
    return b;
  sF32 la = sLog(a);
  return sExp(la+(sLog(b) - la)*f);
}

sF32 sDegreeDiff(sF32 alpha, sF32 beta)
{
  return sAngleDiff(alpha/360.0f,beta/360.0f) * 360.0f;
}

sF32 sRadianDiff(sF32 alpha, sF32 beta)
{
  return sAngleDiff(alpha/sPI2F, beta/sPI2F) * sPI2F;
}

sF32 sAngleDiff(sF32 alpha, sF32 beta)
{
  sF32 a=sFrac(alpha); 
  sF32 b=sFrac(beta);
  sF32 r=a-b;
  if (r<-0.5f) r+=1;
  if (r>0.5f) r-=1;
  return r;
}

// a-b, angles=0..c, -count/2 > result > count/2
sInt sAngleDiff(sInt alpha, sInt beta, sInt count)
{
  sInt r = alpha - beta;
  if (r < -(count>>1)) r += count;
  if (r > (count>>1)) r -= count;
  return r;
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Basic Stuff                                                        ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

static sChar ShellParaBuffer[4096];
static sChar *ShellParaBufferEnd;
static sChar *ShellParaVal[64];
static sChar *ShellParaOpt[64];
static sInt ShellParaCount;

/****************************************************************************/
/***                                                                      ***/
/***   Commandline Parsing                                                ***/
/***                                                                      ***/
/****************************************************************************/

void sParseCmdLine(const sChar *cmd,sBool skipcmd)
{
  static sChar strOne[] = L"1";
  sChar *d;
  const sChar *s;
  sChar c;
  sInt mode;
  sInt i;

  // skip first (programm name)

  if(skipcmd)
  {
    while(sIsSpace(*cmd)) cmd++;
    if(*cmd=='"')
    {
      cmd++;
      while(*cmd!=0 && *cmd!='"') cmd++;
      if(*cmd=='"') cmd++;
    }
    else
    {
      while(*cmd && !sIsSpace(*cmd)) cmd++;
    }
  }

  // skip initial whitespace

  while(*cmd && sIsSpace(*cmd)) cmd++;

  // initialize

  ShellParaCount = 0;
  s = cmd;
  d = ShellParaBuffer;
  mode = 0;

  for(i=0;i<64;i++)
  {
    ShellParaVal[i]=0;
    ShellParaOpt[i]=0;
  }

  // scan

  while(*s)
  {
    c=*s++;
    switch(mode)
    {
    case 0:     // what's next?
      if(!(c==' ' || c=='\t' || c=='\r' || c=='\n'))
      {
        if(c=='-')   // read option word
        {
          if(ShellParaOpt[ShellParaCount])
          {
            ShellParaVal[ShellParaCount]=strOne;
            ShellParaCount++;
          }
          ShellParaOpt[ShellParaCount] = d;
          mode = 1;
        }
        else if(c=='"')   // read string (for unnamed options)
        {
          if(ShellParaCount>0 && ShellParaVal[ShellParaCount-1]==0)
            ShellParaCount--;
          ShellParaVal[ShellParaCount] = d;
          mode = 2;
        }
#if sPLATFORM != sPLAT_WINDOWS && sPLATFORM != sPLAT_LINUX 
        // this breaks network path parameters
        else if(c=='/' && s[0]=='/') // skip end-of-line comment (for command line files)
        {
          s++;
          while(*s && *s!='\n' && *s!='\r')
            s++;
        }
#endif
        else    // read word (for unnamed options)
        {
          if(ShellParaCount>0 && ShellParaVal[ShellParaCount-1]==0)
            ShellParaCount--;
          ShellParaVal[ShellParaCount] = d;
          *d++ = c;
          mode = 1;
        }
        ShellParaCount++;
      }
      break;
    case 1:     // read in word till next whitespace
      if(c==' ' || c=='\t' || c=='\r' || c=='\n')
      {
        *d++ = 0;
        mode = 0;
      }
      else
      {
        *d++ = c;
      }
      break;
    case 2:     // read in string
      if(c=='"')
      {
        *d++ = 0;
        mode = 0;
      }
      else
      {
        *d++ = c;
      }
      break;
    }
  }
  *d++ = 0;
  if(ShellParaOpt[ShellParaCount])
  {
    ShellParaVal[ShellParaCount]=strOne;
    ShellParaCount++;
  }
/*
#if sCOMMANDLINE
  if(ShellParaCount>0)
  {
    for(i=1;i<ShellParaCount;i++)
    {
      ShellParaVal[i-1] = ShellParaVal[i];
      ShellParaOpt[i-1] = ShellParaOpt[i];
    }
    ShellParaCount--;
  }
#endif
*/
  if(ShellParaCount>0)
  {
    sLogPrefixLength = sGetShellParameterInt(L"logprefix", 0, 8);

    sLogF(L"sys",L"Shell Parameters\n");
    for(i=0;i<ShellParaCount;i++)
    {
      if(ShellParaOpt[i])
        sLogF(L"sys",L"  -%s %s\n",ShellParaOpt[i],ShellParaVal[i]);
      else
        sLogF(L"sys",L"  %s\n",ShellParaVal[i]);
    }
  }

  // save the end of the used buffer
  ShellParaBufferEnd = d;

  sEnableLogFilter();
}

/****************************************************************************/

sBool sAddShellParameter(const sChar *opt, const sChar *val)
{
  sBool result = sTRUE;

  // get end of ShellParaBuffer
  sChar *d = ShellParaBufferEnd;
  
  sDInt spaceLeft    = sCOUNTOF(ShellParaBuffer) - (d-ShellParaBuffer);
  sDInt neededSpace  = sGetStringLen(opt) + sGetStringLen(val) + 2; // 2 trailing zeroes

  // is there enough space and entries left
  result = (spaceLeft>=neededSpace) && (ShellParaCount < sCOUNTOF(ShellParaVal));

  if (result)
  {
    sInt length;

    // copy opt
    sCopyString(d, opt, spaceLeft );
    ShellParaOpt[ShellParaCount] = d;
    length = sGetStringLen(opt) + 1;
    d         += length;
    spaceLeft -= length;
    
    // copy val
    sCopyString(d, val, spaceLeft );
    ShellParaVal[ShellParaCount] = d;
    length = sGetStringLen(val) + 1;
    d         += length;
    spaceLeft -= length;

    sLogF(L"sys",L"Additional Shell Parameter: ");
    if(ShellParaOpt[ShellParaCount])
      sLogF(L"sys",L"-%s %s\n",ShellParaOpt[ShellParaCount],ShellParaVal[ShellParaCount]);
    else
      sLogF(L"sys",L"%s\n",ShellParaVal[ShellParaCount]);

    ShellParaBufferEnd = d;
    ShellParaCount++;
  }

  return result;
}

/****************************************************************************/


const sChar *sGetShellParameter(const sChar *opt,sInt n)
{
  sInt i;

  for(i=0;i<ShellParaCount;i++)
  {
    if((opt==0 && ShellParaOpt[i]==0) || (opt && ShellParaOpt[i] && sCmpStringI(opt,ShellParaOpt[i])==0))
    {
      if(n==0)
        return ShellParaVal[i];
      n--;
    }
  }
  return 0;
}

/****************************************************************************/

sInt sGetShellParameterInt(const sChar *opt,sInt n,sInt def)
{
  const sChar *s = sGetShellParameter(opt,n);

  sInt val;
  if (s && sScanInt(s, val))
    return val;
  return def;
}

/****************************************************************************/

sF32 sGetShellParameterFloat(const sChar *opt,sInt n,sF32 def)
{
  const sChar *s = sGetShellParameter(opt,n);

  sF32 val;
  if (s && sScanFloat(s, val))
    return val;
  return def;
}

/****************************************************************************/

sU32 sGetShellParameterHex(const sChar *opt, sInt n, sU32 def)
{
  const sChar *s = sGetShellParameter(opt,n);

  sInt val;
  if (s && sScanHex(s, val))
    return (sU32)val;
  return def;
}

/****************************************************************************/

sBool sGetShellSwitch(const sChar *opt)
{
  sInt i;
  sBool result;

  result = sFALSE;

  for(i=0;i<ShellParaCount;i++)
  {
    if(ShellParaOpt[i] && sCmpStringI(opt,ShellParaOpt[i])==0)
    {
      result = sTRUE;
//      ShellParaOpt[i] = 0;
    }
  }
  return result;
}

/****************************************************************************/


const sChar *sGetShellString(const sChar *opt0,const sChar *opt1,const sChar *def)
{
  const sChar *str;

  if(opt0)
  {
    str = sGetShellParameter(opt0,0);
    if(str) return str;
  }
  if(opt1)
  {
    str = sGetShellParameter(opt1,0);
    if(str) return str;
  }
  return def;
}

/****************************************************************************/


sInt sGetShellInt(const sChar *opt0,const sChar *opt1,sInt def)
{
  const sChar *str;

  if(opt0)
  {
    str = sGetShellParameter(opt0,0);
    if(str)
    {
      sScanInt(str,def);
      return def;
    }
  }
  if(opt1)
  {
    str = sGetShellParameter(opt1,0);
    if(str)
    {
      sScanInt(str,def);
      return def;
    }
  }

  return def;
}

/****************************************************************************/

sFormatStringBuffer& operator% (sFormatStringBuffer &f, const sGUID &guid)
{
  if (!*f.Format)
    return f;

  sFormatStringInfo info;
  f.GetInfo(info);
  info.Null = 1;
  if(info.Format != 'x' && info.Format != 'X')
    info.Format = 'x';

  f.PrintInt<sU32>(info,guid.Data32,0);
  f.Print(L"-");
  for (sInt i=0; i<3; i++)
  {
    f.PrintInt<sU16>(info,guid.Data16[i],0);
    f.Print(L"-");
  }
  for (sInt i=0; i<6; i++)
    f.PrintInt<sU8>(info,guid.Data8[i],0);

  f.Fill();
  return f;
}

sGUID sGetGUIDFromString(const sChar *str)
{
  sChecksumMD5 md5;
  md5.Calc((const sU8 *)str,2*sGetStringLen(str));
  sGUID guid;
  guid.Data32=md5.Hash[0];
  guid.Data16[0]=md5.Hash[1]>>16;
  guid.Data16[1]=md5.Hash[1]&0xffff;
  guid.Data16[2]=md5.Hash[2]>>16;
  guid.Data8[0]=(md5.Hash[2]>>8)&0xff;
  guid.Data8[1]=md5.Hash[2]&0xff;
  guid.Data8[2]=md5.Hash[3]>>24;
  guid.Data8[3]=(md5.Hash[3]>>16)&0xff;
  guid.Data8[4]=(md5.Hash[3]>>8)&0xff;
  guid.Data8[5]=md5.Hash[3]&0xff;
  return guid;
}
/****************************************************************************/

void sHexDump(const void *data,sInt bytes)
{
  const sU32 *ptr = (sU32 *)(sDInt(data)&~3);
  for(sInt i=0;i<bytes;i+=32)
  {
    sDPrintF(L"%08x   %08x %08x %08x %08x  %08x %08x %08x %08x\n",
      sPtr(ptr),ptr[0],ptr[1],ptr[2],ptr[3],ptr[4],ptr[5],ptr[6],ptr[7]);
    ptr += 8;
  }
}

void sHexByteDump(const void *data,sInt bytes)
{
  const sU8 *ptr = (sU8*)data;
  for(sInt i=0;i<bytes;i+=8)
  {
    sDPrintF(L"%08x   %02x %02x %02x %02x  %02x %02x %02x %02x\n",
      sPtr(ptr),ptr[0],ptr[1],ptr[2],ptr[3],ptr[4],ptr[5],ptr[6],ptr[7]);
    ptr += 8;
  }
}

/****************************************************************************/
sChar sLogFilterPosList[1024];
sChar sLogFilterNegList[1024];
const sChar *sLastLogModule=L"";

void sEnableLogFilter()
{
  sInt n = 0;

  sChar *ps,*pe;
  sChar *ns,*ne;

  ps = sLogFilterPosList;
  pe = ps+sCOUNTOF(sLogFilterPosList)-2;
  ns = sLogFilterNegList;
  ne = ns+sCOUNTOF(sLogFilterNegList)-2;

  n = 0;
  for(;;)
  {
    const sChar *s = sGetShellParameter(L"log",n);
    if(!s) break;
    while(sIsSpace(*s)) s++;
    while(*s)
    {
      while(*s && !sIsSpace(*s) && ps<pe)
        *ps++ = *s++;
      *ps++ = 0;
      while(sIsSpace(*s)) s++;
    }
    n++;
  }
  *ps++ = 0;

  n = 0;
  for(;;)
  {
    const sChar *s = sGetShellParameter(L"dontlog",n);
    if(!s) break;
    while(sIsSpace(*s)) s++;
    while(*s)
    {
      while(*s && !sIsSpace(*s) && ns<ne)
        *ns++ = *s++;
      *ns++ = 0;
      while(sIsSpace(*s)) s++;
    }
    n++;
  }
  *ns++ = 0;
}

sBool sCheckLogFilter(const sChar *module)
{
  const sChar *ps = sLogFilterPosList;
  const sChar *ns = sLogFilterNegList;
  if(*ps)
  {
    while(*ps)
    {
      if(sCmpStringI(module,ps)==0) return 1;
      while(*ps) ps++;
      ps++;
    }
    return 0;
  }
  if(*ns)
  {
    while(*ns)
    {
      if(sCmpStringI(module,ns)==0) 
        return 0;
      while(*ns) ns++;
      ns++;
    }
  }
  return 1;
}

#if sPLATFORM!=sPLAT_WINDOWS
void sPrintError(const sChar *text)   { sPrint(text); }
void sPrintWarning(const sChar *text) { sPrint(text); }
#endif

#if sENABLE_DPRINT

static sBool enableLogConsole = sFALSE;

void sEnableLogConsole(sBool enable)
{
  enableLogConsole = enable;
}

void sLog(const sChar *module,const sChar *text)
{
  const sInt max = 1024;
  sString<max> buf;
  if(module==0)
    module = sLastLogModule;
  else
    sLastLogModule = module;

  if(sCheckLogFilter(module))
  {
    while(*text)
    {
      sInt i=0;
      if (sLogPrefixLength > 0)
      {
        buf[i++] = '[';
        const sChar *m = module;
        while(*m && i<max-4 && i<sLogPrefixLength+1)
          buf[i++] = *m++;
        buf[i++] = ']';
        buf[i++] = ' ';

        while (i < sLogPrefixLength + 3) buf[i++] = ' ';   // this line can't overflow!
      }
      
      while(*text!=0 && *text!='\n' && i<max-2)
        buf[i++] = *text++;
      if(*text=='\n')
        buf[i++]= *text++;
      buf[i++] = 0;
      sDPrint(buf);
      if (enableLogConsole)
        sPrint(buf);
    }
  }
}
#else

void sEnableLogConsole(sBool enable)
{
}

#if !sSTRIPPED
void sLog(const sChar *module,const sChar *text)
{
}
#endif

#endif


/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Memory Management                                                  ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

sPtr sMemoryUsed;
sPtr sMemoryUsedByMemSystem;
sInt sMemoryInitFlags;
sPtr sMemoryInitSize;
sPtr sMemoryInitDebugSize=16*1024*1024;

struct sMemoryMarkStruct
{
  sMemoryMarkStruct()
  {
    sClear(*this);
  }

  sU32 Hash;              // hash of the memorysituation
  sInt Taken;             // shows if a hash was taken
  sInt Reset;             // flags that a reset should be performed
  sInt AllocId;           // allocId when the first memmark was taken
  sInt Id;                // this int can be used to determine if this memmark is a special one
};

static sStackArray<sMemoryMarkStruct,3> sMemoryMarks;
static sMemoryMarkStruct sMemoryMark;     // the structure to work on
static sBool sMemoryMarkFlushVertexFormat = sFALSE;  // flag to flush vertexformats during memmark

static sInt sMemoryAllocId;
static sInt sMemoryBreakId;
static sHooks1<sBool> *sMemFlushHook;
static sMemoryLeakTracker *sMemoryLeaks;
static sBool sMemoryLeakCheck;
static sMemoryHandler *sMemoryHandlers[sAMF_MASK+1];
static sInt sMemoryHandlerMax;

sInt sOutOfMemory=0; // set to heap id upon out of memory condition

#if !sSTRIPPED
static sF32 sMemFailProbability = 0;
static sRandom sMemFailRandom;
void sSetAllocFailTest(sF32 prob) { sMemFailProbability=prob; }
sF32 sGetAllocFailTest() { return sMemFailProbability; }
#else
void sSetAllocFailTest(sF32) {}
sF32 sGetAllocFailTest() { return 0.0f; }
#endif

/****************************************************************************/
sMemoryHandler::sMemoryHandler()
{
  Start = 0;
  End = 0;
  Owner = 0;
  ThreadSafe = 0;
  IncludeInSnapshot = 0;
  MayFail = 0;
  NoLeaks = 0;
  Lock_ = 0;
  MinTotalFree = 0;
}

sMemoryHandler::~sMemoryHandler()
{
  sMemoryHandler::MakeThreadUnsafe(); 
}

void sMemoryHandler::MakeThreadSafe()
{
  if(!Lock_)
  {
    Lock_ = new sThreadLock;
    ThreadSafe = 1;
    Owner = 0;
  }
}

void sMemoryHandler::MakeThreadUnsafe()
{
  if(Lock_)
  {
    sThreadLock *l= Lock_;
    l->Lock();

    Owner = sGetThreadContext();
    Lock_ = 0;
    ThreadSafe = 0;

    l->Unlock();
    delete l;
  }
}


void sMemoryHandler::Lock()
{
  if(ThreadSafe)
    Lock_->Lock();
}

sBool sMemoryHandler::TryLock()
{
  if(ThreadSafe)
    return Lock_->TryLock();
  return sTRUE;
}

void sMemoryHandler::Unlock()
{
  if(ThreadSafe)
    Lock_->Unlock();
}

/****************************************************************************/

struct sDbgMemRange
{
  sDbgMemRange() { Start=0; End=0; }
  sDbgMemRange(sPtr start, sPtr end) { Start=start; End=end; }
  sPtr Start;
  sPtr End;
};
static sThreadLock *sDbgMemRangesLock=0;
static sStaticArray<sDbgMemRange> *sDbgMemRanges=0;

void sMemDbgLock(sPtr start, sPtr size)
{
#if sCFG_MEMDBG_LOCKING
  if(sDbgMemRangesLock)
  {
    sScopeLock sl(sDbgMemRangesLock);
    sDbgMemRanges->AddTail(sDbgMemRange(start,start+size));
  }
#endif
}

void sMemDbgUnlock(sPtr start, sPtr size)
{
#if sCFG_MEMDBG_LOCKING
  if(sDbgMemRangesLock)
  {
    sScopeLock sl(sDbgMemRangesLock);
    sDbgMemRange *range;
    sFORALL(*sDbgMemRanges,range)
    {
      if(range->Start==start&&range->End==start+size)
      {
        sDbgMemRanges->RemAt(_i);
        return;
      }
    }
  }
#endif
}

sBool sMemDbgCheckLock(void *ptr_)
{
#if sCFG_MEMDBG_LOCKING
  if(sDbgMemRangesLock)
  {
    sScopeLock sl(sDbgMemRangesLock);
    sDbgMemRange *range;
    sPtr ptr = (sPtr)ptr_;
    sFORALL(*sDbgMemRanges,range)
    {
      if(ptr>=range->Start&&ptr<range->End)
        return sTRUE;
    }
  }
#endif
  return sFALSE;
}


/****************************************************************************/

void sInitMem0()
{
  sMemoryLeakCheck = 0;
  sMemoryAllocId = 1;
  sMemoryBreakId = 0;
  sMemoryHandlerMax = 0;

  sInitMem1();

  sMemoryUsedByMemSystem = sMemoryUsed;

  if(sMemoryHandlers[sAMF_DEBUG])
    sMemoryHandlers[sAMF_DEBUG]->MakeThreadSafe();

  if(sCONFIG_DEBUGMEM && (sIsMemTypeAvailable(sAMF_DEBUG) || (sPLATFORM==sPLAT_WINDOWS || sPLATFORM==sPLAT_LINUX)))
  {
    if(!(sMemoryInitFlags & sIMF_NOLEAKTRACK))
    {
      sLogF(L"mem",L"initialize memory leak tracker\n");
      sInt memtype=sIsMemTypeAvailable(sAMF_DEBUG) ? sAMF_DEBUG : sAMF_HEAP;
      sPushMemType(memtype);
      sMemoryLeaks = new sMemoryLeakTracker;
      sMemoryLeaks->Init2(memtype);
      sPopMemType(memtype);
      sMemoryLeakCheck = 1;
    }
  }

  sMemFlushHook = new sHooks1<sBool>;

  sDumpMemoryMap();

#if sCFG_MEMDBG_LOCKING
  sDbgMemRangesLock = new sThreadLock;
  sDbgMemRanges = new sStaticArray<sDbgMemRange>;
  sDbgMemRanges->HintSize(1024);
#endif
}

void sDumpMemoryMap()
{
  sLogF(L"mem",L"MEMORY MAP:\n");
  for(sInt i=0;i<sAMF_MASK+1;i++)
  {
    sMemoryHandler *h = sMemoryHandlers[i];
    if(h)
      sLogF(L"mem",L"%02d: %08x..%08x (%Kb)\n",i,h->Start,h->End,(sU64)h->GetSize());
  }
}

void sExitMem0()
{
#if sCFG_MEMDBG_LOCKING
  sDelete(sDbgMemRangesLock);
  sDelete(sDbgMemRanges);
#endif

  sPartitionMemory(0,0,0);

  for(sInt i=1;i<=sMemoryHandlerMax;i++)
    if(sMemoryHandlers[i])
      sMemoryHandlers[i]->sMemoryHandler::MakeThreadUnsafe();

  delete sMemFlushHook;
  if(sMemoryLeakCheck && sMemoryLeaks->HasLeak())
  {
    sDPrintF(L"/****************************************************************************/\n");
    sDPrintF(L"/***                                                                      ***/\n");
    sDPrintF(L"/***   Memory Leak Detected.                                              ***/\n");
    sDPrintF(L"/***                                                                      ***/\n");
    sDPrintF(L"/****************************************************************************/\n");

    sMemoryLeaks->DumpLeaks(L"memory leaks",0);
    sMemoryLeakCheck = 0;
    sMemoryLeaks->Exit();
    sDelete(sMemoryLeaks);
    sLogF(L"mem",L"%K bytes leaked\n",sMemoryUsed-sMemoryUsedByMemSystem);
  }
  else
  {
    if(sMemoryLeakCheck)
    {
      sMemoryLeakCheck = 0;
      sMemoryLeaks->Exit();
      sDelete(sMemoryLeaks);
    }
    if(sMemoryUsed-sMemoryUsedByMemSystem!=0)
    {
      sDPrintF(L"/****************************************************************************/\n");
      sDPrintF(L"/***                                                                      ***/\n");
      sDPrintF(L"/***   Memory Leak Detected.                                              ***/\n");
      sDPrintF(L"/***                                                                      ***/\n");
      sDPrintF(L"/****************************************************************************/\n");
      sLogF(L"mem",L"%K bytes leaked\n",sMemoryUsed-sMemoryUsedByMemSystem);
    }
  }
  sExitMem1();
}

void sRegisterMemHandler(sInt slot,sMemoryHandler *h)
{
  sVERIFYRELEASE(sMemoryHandlers[slot]==0);
  if (h&&!h->IsThreadSafe())
    h->Owner = sGetThreadContext();     // threadsafe memory handlers don't have an owner
  sMemoryHandlers[slot] = h;
  sMemoryHandlerMax = sMax(sMemoryHandlerMax,slot);
}

void sUnregisterMemHandler(sInt slot)
{
  if (sMemoryHandlers[slot]) 
    sMemoryHandlers[slot]->Owner = 0;
  sMemoryHandlers[slot] = 0;
  for (sInt i=slot-1; i>=0; i--)
    if (sMemoryHandlers[i])
    {
      sMemoryHandlerMax=i;
      break;
    }
}

sMemoryHandler *sGetMemHandler(sInt slot)
{
  if(slot>=0&&slot<sCOUNTOF(sMemoryHandlers))
    return sMemoryHandlers[slot];
  return 0;
}


sBool sIsMemTypeAvailable(sInt t)
{
  return sMemoryHandlers[t&sAMF_MASK]!=0;
}

sCONFIG_SIZET sGetFreeMem(sInt type)
{
  type&=sAMF_MASK;
  if (type==sAMF_DEFAULT)
  {
    sThreadContext *tx = sGetThreadContext();
    type=tx->MemTypeStack[tx->MemTypeStackIndex];
  }

  sMemoryHandler *h=sMemoryHandlers[type&sAMF_MASK];
  if (!h) return 0;
  h->Lock();
  sCONFIG_SIZET free=h->GetFree();
  h->Unlock();
  return free;
}

void sPushMemType(sInt t)
{
  sThreadContext *tx = sGetThreadContext();
  tx->MemTypeStackIndex++;

  sVERIFY(tx->MemTypeStackIndex<sCOUNTOF(tx->MemTypeStack));
  tx->MemTypeStack[tx->MemTypeStackIndex] = t;
}

void sPopMemType(sInt t)
{
  sThreadContext *tx = sGetThreadContext();

  sVERIFY(tx->MemTypeStack[tx->MemTypeStackIndex] == t);
  tx->MemTypeStackIndex--;
  sVERIFY(tx->MemTypeStackIndex>=0);
}

void *sAllocMem_(sPtr size,sInt align,sInt flags)
{
  if(sMemoryBreakId == sMemoryAllocId)
    sDEBUGBREAK;

  sThreadContext *tx = sGetThreadContext();

#if sCONFIG_DEBUGMEM
  tx->TagLastFile = tx->TagMemFile;
#endif

  void *p = 0;
  if((flags & sAMF_MASK)==0)
  {
    flags |= tx->MemTypeStack[tx->MemTypeStackIndex];
  }

  sMemoryHandler *h = sMemoryHandlers[flags & sAMF_MASK];
  sVERIFY(h);
  sVERIFY(h->Owner==0 || h->Owner==tx);
  
#if !sSTRIPPED
  // random fail test
  if (h->MayFail && sMemFailRandom.Float(1.0f)<sMemFailProbability)
  {
#if sCONFIG_DEBUGMEM
    tx->TagMemFile = 0;
#endif
    return 0;
  }
#endif

  h->Lock();
  p = h->Alloc(size,align,flags);
  h->Unlock();
  if(!p)
  {
    if(h->MayFail || (flags & sAMF_MAYFAIL))
    {
#if sCONFIG_DEBUGMEM
      tx->TagMemFile = 0;
#endif
      return 0;
    }
    else
    {
      sOutOfMemory=flags&sAMF_MASK;     
#if sCONFIG_DEBUGMEM
      const char *tagfile=tx->TagMemFile;
      const sInt tagline=tx->TagMemLine;
      if (sMemoryLeakCheck && sOutOfMemory!=sAMF_DEBUG) sMemoryLeaks->DumpLeaks(L"",0,sOutOfMemory);     
#endif
      h->Lock();
      sCONFIG_SIZET hsize=h->GetSize();
      sCONFIG_SIZET free=h->GetFree();
      sCONFIG_SIZET largest=h->GetLargestFree();
      h->Unlock();

#if sCONFIG_DEBUGMEM
      if (tagfile)
      {
        static sChar oomfile[1024];
        sCopyString(oomfile,tagfile,1023);
        sFatal(L"%s(%d): out of mem: tried %K, align %d, flags %08x\n(free: %K of %K, largest: %K)",oomfile,tagline,size,align,flags,(sU64)free,(sU64)hsize,(sU64)largest);
      }
      else
#endif
        sFatal(L"out of mem - tried %K, align %d, flags %08x\n(free: %K of %K, largest: %K)",size,align,flags,(sU64)free,(sU64)hsize,(sU64)largest);
    }
  }
#if sCONFIG_DEBUGMEM
  if(sMemoryLeakCheck && !(flags & sAMF_NOLEAK) && !h->NoLeaks)
    sMemoryLeaks->AddLeak(p,size,tx->TagMemFile,tx->TagMemLine,sMemoryAllocId,flags&(sAMF_MASK|sAMF_ALT),tx->MemLeakDescBuffer2,tx->MLDBCRC);
  tx->TagMemFile = 0;
#endif
  sMemoryAllocId++;
  return p;
}


void sFreeMem(void *ptr)
{
  if(ptr)
  {
    sThreadContext *tx = sGetThreadContext();
    sMemoryHandler *h=0;

    sPtr p = sPtr(ptr);
    for(sInt i=sMemoryHandlerMax;i>=1;i--)
    {
      h = sMemoryHandlers[i];
      if(h)
      {
        if(p>=h->Start && p<h->End)
        {
          sVERIFY(h->Owner==0 || h->Owner==tx);
          h->Lock();
          sBool r= h->Free(ptr);
          h->Unlock();

          if (r) break;
        }
      }
      h=0;
    }

    if (!h) for(sInt i=sMemoryHandlerMax;i>=1;i--)
    {
      h = sMemoryHandlers[i];
      if(h && h->End==0)
      {
        sVERIFY(h->Owner==0 || h->Owner==tx);
        h->Lock();
        sBool r= h->Free(ptr);
        h->Unlock();

        if(r) break;
      }
      h=0;
    }

    if (!h) 
      sFatal(L"pointer %08x seems not to belong to any sMemoryHandler",p);

#if sCONFIG_DEBUGMEM
    if(sMemoryLeakCheck && !h->NoLeaks)
      sMemoryLeaks->RemLeak(ptr);
#endif
  }
}

sPtr sMemSize(void *ptr)
{
  if(ptr)
  {
    sThreadContext *tx = sGetThreadContext();

    sPtr p = sPtr(ptr);
    for(sInt i=sMemoryHandlerMax;i>=1;i--)
    {
      sMemoryHandler *h = sMemoryHandlers[i];
      if(h)
      {
        if(p>=h->Start && p<h->End)
        {
          sVERIFY(h->Owner==0 || h->Owner==tx);
          h->Lock();
          sPtr r= h->MemSize(ptr);
          h->Unlock();
          return r;
        }
      }
    }
    for(sInt i=sMemoryHandlerMax;i>=1;i--)
    {
      sMemoryHandler *h = sMemoryHandlers[i];
      if(h && h->End==0)
      {
        sVERIFY(h->Owner==0 || h->Owner==tx);
        h->Lock();
        sPtr r= h->MemSize(ptr);
        h->Unlock();
        return r;
      }
    }
    sFatal(L"pointer %08x seems not to belong to any sMemoryHandler",p);
  }
  return 0;
}

void sCheckMem_()
{
  sThreadContext *tx = sGetThreadContext();
  for(sInt i=1;i<=sMemoryHandlerMax;i++)
  {
    sMemoryHandler *h = sMemoryHandlers[i];
    if(h && (h->Owner==0 || h->Owner==tx))
    {
      h->Lock();
      h->Validate();
      h->Unlock();
    }
  }
}

void sResetMemChecksum()
{
  sLog(L"mem",L"WARNING! resetting memory checksum!\n");
  sMemoryMark.Reset = 1;
}


void sMemMark(sBool fatal/*=sTRUE*/)
{
  sU32 hash;

  sMemFlushHook->Call(sMemoryMarkFlushVertexFormat);
  sMemoryMarkFlushVertexFormat = sTRUE;

  // calculate hash over all handlers, excluding debug
  sThreadContext *tx = sGetThreadContext();
  hash = 0;
  for(sInt i=1;i<=sMemoryHandlerMax;i++)
  {
    if(i!=sAMF_DEBUG)
    {
      sMemoryHandler *h = sMemoryHandlers[i];
      if(h&&h->IncludeInSnapshot)
      {
        sVERIFY(h->Owner==0 || h->Owner==tx);
        h->Lock();
        hash ^= h->MakeSnapshot();
        h->Unlock();
      }
    }
  }
  sLogF(L"mem",L"Memory Hash is %08x\n",hash);

  // check

  if(sMemoryMark.Reset)
  {
    sMemoryMark.Taken = 0;
    sMemoryMark.Reset = 0;
  }
  if(sMemoryMark.Taken)
  {
    if(sMemoryMark.Hash!=hash)
    {
      sLogF(L"mem",L"Memory Hash has Changed %08x -> %08x\n",sMemoryMark.Hash,hash);
      sDPrintF(L"/****************************************************************************/\n");
      sDPrintF(L"/***                                                                      ***/\n");
      sDPrintF(L"/***   Memory Leak Detected.                                              ***/\n");
      sDPrintF(L"/***                                                                      ***/\n");
      sDPrintF(L"/****************************************************************************/\n");
#if sCONFIG_DEBUGMEM
      if (sMemoryLeakCheck) 
      {
        for(sInt i=1;i<=sMemoryHandlerMax;i++)
        {
          sMemoryHandler *h = sMemoryHandlers[i];
          if(h && h->IncludeInSnapshot && i!=sAMF_DEBUG)
          {
            sString<64> str;
            sSPrintF(str,L"heap 0x%2x, leaks since first sMemMark()",i);
            sMemoryLeaks->DumpLeaks(str,sMemoryMark.AllocId,i);
          }
        }
      }
#endif
      if(fatal) sFatal(L"memory leaks are fatal.\n");
    }
  }
  else  // or save state
  {
    sMemoryMark.Hash = hash;   // this may be called more than once
    if(sMemoryMark.AllocId==0) // but we set the markallocid only once, otherwise we mess up the MemMarkCallback
      sMemoryMark.AllocId = sMemoryAllocId;
  }
  sMemoryMark.Taken = 1;
}

sBool sIsMemMarkSet()         // this will return true even when we are in reset memmark mode!
{
  return sMemoryMark.AllocId>0;
}

void sAddMemMarkCallback(void (*cb)(sBool flush,void *user),void *user)
{
  //if (!sMemStackActive) return;
  sMemFlushHook->Add(cb,user);
}


void sBreakOnAllocation(sInt n)
{
  sMemoryBreakId = n;
}

sInt sGetMemoryAllocId()
{
  return sMemoryAllocId;
}

void sMemMarkPush(sInt id)
{
  sMemoryMarks.AddTail(sMemoryMark);
  if (id!=-1)
    sMemoryMark.Id = id;
}

void sMemMarkPop()
{
  if (sMemoryMarks.GetCount())
  {
    sMemoryMarkStruct stored = sMemoryMark;

    sMemoryMark = sMemoryMarks.RemTail();
    
    sMemoryMark.AllocId = stored.AllocId;
    sMemoryMark.Reset   = stored.Reset;
  }
}

sInt sMemMarkGetId()
{
  return sMemoryMark.Id;
}

#if sCONFIG_DEBUGMEM

void sTagMem(const char *file,sInt line)
{
  sThreadContext *tx=sGetThreadContext();
  if(!tx->TagMemFile) { tx->TagMemFile=file; tx->TagMemLine=line; } 
}

void sTagMemLast()
{
  sThreadContext *tx=sGetThreadContext();
  tx->TagMemFile = tx->TagLastFile;
}
#endif

sMemoryLeakTracker *sGetMemoryLeakTracker() 
{
  if (!sSTRIPPED && sMemoryLeakCheck)
    return sMemoryLeaks;
  else
    return 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   Memory Partitioning                                                ***/
/***                                                                      ***/
/****************************************************************************/

static sBool sFrameMemDoubleBuffered = sFALSE;

sPtr sMemFrameSize;
sPtr sMemFrameLastUsed;
static sPtr sMemFramePtr[2]={0,0};
static sPtr sMemFrameUsed;
static sInt sMemFrameToggle;
static sPtr sMemFrameWasted;
static sPtr sMemDmaSize;
static sPtr sMemDmaPtr[2]={0,0};
static sPtr sMemDmaUsed;
static sInt sMemDmaToggle;
static sPtr sMemDmaWasted;
static sU64 sMemFlipFrame;

sDInt sMemStatMaxFrameUsed=0;
sDInt sMemStatMaxDMAUsed=0;

void sRender3DFlush();

void sPartitionMemory(sPtr frame,sPtr dma,sPtr gfx)
{
  // delete old

  if(sMemFrameSize!=frame && frame!=1)
  {
    sFreeMem((void*)sMemFramePtr[0]);
    sMemFramePtr[0] = 0;
    sMemFrameSize = 0;
    if(sFrameMemDoubleBuffered)
    {
      sFreeMem((void*)sMemFramePtr[1]);
      sMemFramePtr[1] = 0;
    }
  }
  if(sMemDmaSize!=dma && dma!=1)
  {
    sRender3DFlush();
    sMemDmaSize = 0;
    sFreeMem((void*)sMemDmaPtr[0]); sMemDmaPtr[0] = 0;
    sFreeMem((void*)sMemDmaPtr[1]); sMemDmaPtr[1] = 0;
  }
  // alloc new

  sInitMem2(gfx);

  if(!sMemFramePtr[0] && frame>=2)
  {
    sPushMemLeakDesc(L"FrameMemory");
    sMemFrameSize = frame;
    sMemFramePtr[0] =  (sPtr)sAllocMem(sMemFrameSize,16,sAMF_HEAP);
    if(sFrameMemDoubleBuffered)
      sMemFramePtr[1] =  (sPtr)sAllocMem(sMemFrameSize,16,sAMF_HEAP);
    sMemFrameToggle = 0;
    sPopMemLeakDesc();
  }

  if(!sMemDmaPtr[0] && dma>=2)
  {
    sPushMemLeakDesc(L"DMAMemory");
    sMemDmaSize = dma;
    sMemDmaPtr[0] = (sPtr)sAllocMem(sMemDmaSize,4096,sAMF_GFX);
    sMemDmaPtr[1] = (sPtr)sAllocMem(sMemDmaSize,4096,sAMF_GFX);
    sMemDmaToggle = 0;
    sPopMemLeakDesc();
  }

  // set Used ptrs.  
  sFlipMem();
  sMemFrameLastUsed=0;

  sMemStatMaxFrameUsed=0;
  sMemStatMaxDMAUsed=0;
}

void sFrameMemDoubleBuffer(sBool enable)
{
  if(sFrameMemDoubleBuffered!=enable)
  {
    if(sFrameMemDoubleBuffered)
    {
      sFreeMem((void*)sMemFramePtr[1]);
      sMemFramePtr[1] = 0;
    }
    else
      sMemFramePtr[1] =  (sPtr)sAllocMem(sMemFrameSize,16,sAMF_HEAP);
    sMemFrameToggle = 0;
    sFrameMemDoubleBuffered=enable;
    sFlipMem();
  }
}

/****************************************************************************/

#if !sCONFIG_FRAMEMEM_MT

// single-threaded implementations

sBool sRender3DLockOwner();

void *sAllocFrame(sPtr size,sInt align)
{
  sVERIFY(sRender3DLockOwner());

  if(sMemFrameUsed==0)
    sFatal(L"please use sPartitionMemory() in sMain() to allocate frame memory");

  size = sAlign(size,align);
  sMemFrameUsed = sAlign(sMemFrameUsed,align);
  sMemFrameUsed += size;
  if(sMemFrameUsed > sMemFramePtr[sMemFrameToggle]+sMemFrameSize)
    sFatal(L"out of frame mem");
  return (void*)(sMemFrameUsed-size);
}

void *sAllocFrameBegin(sInt size,sInt align)
{
  sVERIFY(sRender3DLockOwner());
  if(sMemFrameUsed==0)
    sFatal(L"please use sPartitionMemory() in sMain() to allocate frame memory");

  size = sAlign(size,align);
  sMemFrameUsed = sAlign(sMemFrameUsed,align);

  if((sMemFrameUsed+size) > sMemFramePtr[sMemFrameToggle]+sMemFrameSize)
    sFatal(L"out of frame mem");
  return (void*)(sMemFrameUsed);
}

void sAllocFrameEnd(void *ptr)
{
  sVERIFY(sRender3DLockOwner());
  sPtr end = (sPtr) ptr;
  if (end<sMemFrameUsed || end>sMemFramePtr[sMemFrameToggle]+sMemFrameSize)
    sFatal(L"sAllocFrameEnd: illegal pointer");

  sMemFrameUsed = end;
}

#else

/****************************************************************************/
#define sCFG_ALLOCFRAME_FAST 0                            // multithread safe but may waste more memory then the slow version
                                                          // generally shouldn't be an issue for allocframe (most of the time small allocs with 16 byte alignment)

static void *sAllocFrameBeginImpl(sThreadContext *ctx,sInt size,sInt align)
{
retry:
  // we remember up to two borrowed memory segments, enough free memory in one of them?
  sBool fit = ctx->FrameCurrent ? sAlign(ctx->FrameCurrent,align)+size<=ctx->FrameEnd : 0;
  sBool fitalt = ctx->FrameAltCurrent ? sAlign(ctx->FrameAltCurrent,align)+size<=ctx->FrameAltEnd : 0;

  if(fit)
  {
    ctx->FrameCurrent = sAlign(ctx->FrameCurrent,align);
  }
  else if(fitalt)
  {
    sSwap(ctx->FrameCurrent,ctx->FrameAltCurrent);
    sSwap(ctx->FrameEnd,ctx->FrameAltEnd);
    ctx->FrameCurrent = sAlign(ctx->FrameCurrent,align);
  }
  else  // none fits
  {
    sPtr free = ctx->FrameCurrent ? ctx->FrameEnd-ctx->FrameCurrent :  0;
    sPtr freealt = ctx->FrameAltCurrent ? ctx->FrameAltEnd-ctx->FrameAltCurrent :  0;
    if(freealt<free)      // move larger chunk to alt memory
    {
      sSwap(ctx->FrameCurrent,ctx->FrameAltCurrent);
      sSwap(ctx->FrameEnd,ctx->FrameAltEnd);
    }

    // allocate new segment
    sPtr old_fc = 0;
    sPtr old_fe = 0;
    sSwap(old_fc,ctx->FrameCurrent);
    sSwap(old_fe,ctx->FrameEnd);
    sInt size2 = sAlign(sMax(size+align,sMin(256*1024,size*2+align)),16);    // try to borrow up to size*2+align memory but at least requested size+align
    ctx->FrameCurrent = (sPtr) sAllocFrame(size2,16);
    ctx->FrameEnd = ctx->FrameCurrent + size2;

     // if possible merged new segment with existing ones
    if(old_fe==ctx->FrameCurrent)
    {
      ctx->FrameCurrent = old_fc;
      goto retry;
    }
    else if(ctx->FrameAltEnd==ctx->FrameCurrent)
    {
      ctx->FrameAltEnd = ctx->FrameEnd;
      ctx->FrameCurrent = old_fc;
      ctx->FrameEnd = old_fe;
      goto retry;
    }

    // can not be merged, we have wasted some memory
    sAtomicAdd(&sMemFrameWasted,old_fe-old_fc);
    ctx->FrameCurrent = sAlign(ctx->FrameCurrent,align);
  }

  ctx->FrameBorrow = ctx->FrameCurrent + size;
  sVERIFY(ctx->FrameBorrow <= ctx->FrameEnd);
  return (void *) ctx->FrameCurrent;
}

static sINLINE void sAllocFrameEndImpl(sThreadContext *ctx,void *ptr)
{
  if(ctx->FrameBorrow==0) sFatal(L"sAllocFrameEnd without sAllocFrameBegin");
  sPtr end = (sPtr) ptr;

  sVERIFY(end <= ctx->FrameBorrow);
  ctx->FrameCurrent = end;
  ctx->FrameBorrow = 0;

  if(ctx->FrameCurrent==ctx->FrameEnd)
  {
    // borrowed memory completely consumed, reset segment and swap with alt
    ctx->FrameCurrent = 0;
    ctx->FrameEnd = 0;
    sSwap(ctx->FrameCurrent,ctx->FrameAltCurrent);
    sSwap(ctx->FrameEnd,ctx->FrameAltEnd);
  }
}

#if sCFG_ALLOCFRAME_FAST
sINLINE void *sAllocFrame(sPtr size,sInt align/*=16*/)
{
  // multithread safe but may waste more memory then the slow version
  // generally shouldn't be an issue for allocframe (most of the time small allocs with 16 byte alignment)

  if(sMemFrameUsed==0)
    sFatal(L"please use sPartitionMemory() in sMain() to allocate frame memory");

  size = sAlign(size,16);
  if(align>16)
    size += sAlign(align,16);
  sPtr end = sAtomicAdd(&sMemFrameUsed,size);
  sWriteBarrier();
  if(end > sMemFramePtr[sMemFrameToggle]+sMemFrameSize)
    sFatal(L"out of frame mem");
  sPtr result = sAlign(end-size,align);
  return (void *) result;
}

#else

void *sAllocFrame(sPtr size,sInt align)
{
  if(sMemFrameUsed==0)
    sFatal(L"please use sPartitionMemory() in sMain() to allocate frame memory");

  sPtr result = 0;
  sThreadContext *ctx = sGetThreadContext();
  if(ctx->FrameFrame!=sMemFlipFrame)
  {
    // first alloc frame this frame, reset state
    ctx->FrameFrame = sMemFlipFrame;
    ctx->FrameBorrow = 0;
    ctx->FrameCurrent = ctx->FrameEnd = 0;
    ctx->FrameAltCurrent = ctx->FrameAltEnd =0;
  }
  if(ctx->FrameBorrow) sFatal(L"can't call sAllocFrame() between sAllocFrameBegin() and sAllocFrameEnd()");
  if(ctx->FrameCurrent || align!=16)
  {
    if(align==16)
    {
      sBool fit = ctx->FrameCurrent ? sAlign(ctx->FrameCurrent,align)+size<=ctx->FrameEnd : 0;
      sBool fitalt = ctx->FrameAltCurrent ? sAlign(ctx->FrameAltCurrent,align)+size<=ctx->FrameAltEnd : 0;
      if(!fit&&!fitalt)
        goto nonborrowed;   // don't discard borrowed segments for non-fitting allocation with native alignment
    }

    // we have or want borrowed memory, use it
    result = (sPtr) sAllocFrameBeginImpl(ctx,size,align);
    sAllocFrameEndImpl(ctx,(void *)(result+size));
  }
  else
  {
nonborrowed:
    // no borrowed memory and native alignment
    size = sAlign(size,16);
    sPtr end = sAtomicAdd(&sMemFrameUsed,size);
    sWriteBarrier();
    if(end > sMemFramePtr[sMemFrameToggle]+sMemFrameSize)
      sFatal(L"out of frame mem");
    result = end-size;
  }
  return (void *) result;
}
#endif // sCFG_ALLOCFRAME_FAST

void *sAllocFrameBegin(sInt size,sInt align)
{
  sThreadContext *ctx = sGetThreadContext();
  if(ctx->FrameFrame!=sMemFlipFrame)
  {
    // first alloc frame this frame, reset state
    ctx->FrameFrame = sMemFlipFrame;
    ctx->FrameBorrow = 0;
    ctx->FrameCurrent = ctx->FrameEnd = 0;
    ctx->FrameAltCurrent = ctx->FrameAltEnd =0;
  }
  if(ctx->FrameBorrow) sFatal(L"can't call sAllocFrameBegin() between sAllocFrameBegin() and sAllocFrameEnd()");

  return sAllocFrameBeginImpl(ctx,size,align);
}

void sAllocFrameEnd(void *ptr)
{
  return sAllocFrameEndImpl(sGetThreadContext(),ptr);
}

#endif

/****************************************************************************/

sBool sIsFrameMem(void *ptr)
{
  sPtr p=(sPtr)ptr;
  return p>=sMemFramePtr[sMemFrameToggle] && p<(sMemFramePtr[sMemFrameToggle]+sMemFrameSize);
}
sBool sIsDmaMem(void *ptr)
{
  sPtr p=(sPtr)ptr;
  return p>=sMemDmaPtr[sMemDmaToggle] && p<(sMemDmaPtr[sMemFrameToggle]+sMemDmaSize);
}

/****************************************************************************/

void *sAllocDma(sPtr size,sInt align)
{
  if(sMemDmaUsed==0)
    sFatal(L"please use sPartitionMemory() in sMain() to allocate dma memory");

  sPtr result = 0;
  sThreadContext *ctx = sGetThreadContext();
  if(ctx->DmaFrame!=sMemFlipFrame)
  {
    // first alloc dma this frame, reset state
    ctx->DmaFrame = sMemFlipFrame;
    ctx->DmaBorrow = 0;
    ctx->DmaCurrent = ctx->DmaEnd = 0;
    ctx->DmaAltCurrent = ctx->DmaAltEnd =0;
  }
  if(ctx->DmaBorrow) sFatal(L"can't call sAllocDma() between sAllocDmaBegin() and sAllocDmaEnd()");
  if(ctx->DmaCurrent || align!=16)
  {
    if(align==16)
    {
      sBool fit = ctx->DmaCurrent ? sAlign(ctx->DmaCurrent,align)+size<=ctx->DmaEnd : 0;
      sBool fitalt = ctx->DmaAltCurrent ? sAlign(ctx->DmaAltCurrent,align)+size<=ctx->DmaAltEnd : 0;
      if(!fit&&!fitalt)
        goto nonborrowed;   // don't discard borrowed segments for non-fitting allocation with native alignment
    }
    // we have or want borrowed memory, use it
    result = (sPtr) sAllocDmaBegin(size,align);
    sAllocDmaEnd((void *)(result+size));
  }
  else
  {
nonborrowed:
    // no borrowed memory and native alignment
    size = sAlign(size,16);
    sPtr end = sAtomicAdd(&sMemDmaUsed,size);
    sWriteBarrier();
    if(end > sMemDmaPtr[sMemDmaToggle]+sMemDmaSize)
    {  
      sFatal(L"out of dma mem");
    }
    result = end-size;
  }
  return (void *) result;
}


sU32 sAllocDmaEstimateAvailable (void)
//////////////////////////////////////
// Returns an estimate of the available DMA memory. 
// This function underestimates in case some of the memory has been borrowed but will
// never overestimate. At least that's what Dierk said.
{
  if(sMemDmaUsed==0)
    sFatal(L"please use sPartitionMemory() in sMain() to allocate dma memory");
  return (sMemDmaPtr[sMemDmaToggle]+sMemDmaSize) - sMemDmaUsed;
}
sU32 sDMAMemSize(void)
{ return sMemDmaSize;
}


void *sAllocDmaBegin(sInt size, sInt align/*=16*/)
{
  sThreadContext *ctx = sGetThreadContext();

  if(ctx->DmaFrame!=sMemFlipFrame)
  {
    // first alloc frame this frame, reset state
    ctx->DmaFrame = sMemFlipFrame;
    ctx->DmaBorrow = 0;
    ctx->DmaCurrent = ctx->DmaEnd = 0;
    ctx->DmaAltCurrent = ctx->DmaAltEnd =0;
  }
  if(ctx->DmaBorrow) sFatal(L"can't call sAllocDmaBegin() between sAllocDmaBegin() and sAllocDmaEnd()");

retry:
  // we remember up to two borrowed memory segments, enough free memory in one of them?
  sBool fit = ctx->DmaCurrent ? sAlign(ctx->DmaCurrent,align)+size<=ctx->DmaEnd : 0;
  sBool fitalt = ctx->DmaAltCurrent ? sAlign(ctx->DmaAltCurrent,align)+size<=ctx->DmaAltEnd : 0;

  if(fit)
  {
    ctx->DmaCurrent = sAlign(ctx->DmaCurrent,align);
  }
  else if(fitalt)
  {
    sSwap(ctx->DmaCurrent,ctx->DmaAltCurrent);
    sSwap(ctx->DmaEnd,ctx->DmaAltEnd);
    ctx->DmaCurrent = sAlign(ctx->DmaCurrent,align);
  }
  else  // none fits
  {
    sPtr free = ctx->DmaCurrent ? ctx->DmaEnd-ctx->DmaCurrent :  0;
    sPtr freealt = ctx->DmaAltCurrent ? ctx->DmaAltEnd-ctx->DmaAltCurrent :  0;
    if(freealt<free)      // move larger chunk to alt memory
    {
      sSwap(ctx->DmaCurrent,ctx->DmaAltCurrent);
      sSwap(ctx->DmaEnd,ctx->DmaAltEnd);
    }
    // allocate new segment
    sPtr old_dc = 0;
    sPtr old_de = 0;
    sSwap(old_dc,ctx->DmaCurrent);
    sSwap(old_de,ctx->DmaEnd);
    sInt size2 = sAlign(sMax(size+align,sMin(256*1024,size*2+align)),16);    // try to borrow up to size*2+align memory but at least requested size+align
    ctx->DmaCurrent = (sPtr) sAllocDma(size2,16);
    ctx->DmaEnd = ctx->DmaCurrent + size2;

    // if possible merged new segment with existing ones
    if(old_de==ctx->DmaCurrent)
    {
      ctx->DmaCurrent = old_dc;
      goto retry;
    }
    else if(ctx->DmaAltEnd==ctx->DmaCurrent)
    {
      ctx->DmaAltEnd = ctx->DmaEnd;
      ctx->DmaCurrent = old_dc;
      ctx->DmaEnd = old_de;
      goto retry;
    }

    // can not be merged, we have wasted some memory
    sAtomicAdd(&sMemDmaWasted,old_de-old_dc);
    ctx->DmaCurrent = sAlign(ctx->DmaCurrent,align);
  }

  ctx->DmaBorrow = ctx->DmaCurrent + size;
  sVERIFY(ctx->DmaBorrow <= ctx->DmaEnd);
  sVERIFY((ctx->DmaCurrent & (align-1))==0);
  return (void *) ctx->DmaCurrent;
}

void sAllocDmaEnd(void *ptr)
{
  sThreadContext *ctx = sGetThreadContext();
  if(ctx->DmaBorrow==0) sFatal(L"sAllocDmaEnd without sAllocDmaBegin");
  sPtr end = (sPtr) ptr;

  sVERIFY(end <= ctx->DmaBorrow);
  ctx->DmaCurrent = end;
  ctx->DmaBorrow = 0;

  if(ctx->DmaCurrent==ctx->DmaEnd)
  {
    // borrowed memory completely consumed, reset segment and swap with alt
    ctx->DmaCurrent = 0;
    ctx->DmaEnd = 0;
    sSwap(ctx->DmaCurrent,ctx->DmaAltCurrent);
    sSwap(ctx->DmaEnd,ctx->DmaAltEnd);
  }
}


/****************************************************************************/

void sFlipMem()
{
  if (sMemFrameUsed)
  {
    sDInt used = sMemFrameUsed-sMemFramePtr[sMemFrameToggle];
    if(used>sMemStatMaxFrameUsed)
    {
      sMemStatMaxFrameUsed = used;
      sLogF(L"sys",L"max frame used %Kb\n",sMemStatMaxFrameUsed);
    }
  }
  if (sMemDmaUsed)
  {
    sDInt used = sMemDmaUsed-sMemDmaPtr[sMemDmaToggle];
    if(used>sMemStatMaxDMAUsed)
    {
      sMemStatMaxDMAUsed = used;
      sLogF(L"sys",L"max DMA Used %Kb\n",sMemStatMaxDMAUsed);
    }
  }

  sMemFrameLastUsed=sMemFrameUsed-sMemFramePtr[sMemFrameToggle];
  //sDPrintF(L"frame %K of %K\n",sMemFrameUsed-sMemFramePtr[sMemFrameToggle],sMemFrameSize);
  sMemDmaToggle = !sMemDmaToggle;
  sMemDmaUsed = sMemDmaPtr[sMemDmaToggle];
  if(sFrameMemDoubleBuffered) sMemFrameToggle = !sMemFrameToggle;
  sMemFrameUsed = sMemFramePtr[sMemFrameToggle];
  //if(sMemFrameWasted||sMemDmaWasted) sDPrintF(L"wasted %Kb frame mem, %Kb dma mem\n",sMemFrameWasted,sMemDmaWasted);
  sMemFrameWasted = 0;
  sMemDmaWasted = 0;
  sMemFlipFrame++;
}

/****************************************************************************/
/***                                                                      ***/
/***   MemoryPool                                                         ***/
/***                                                                      ***/
/****************************************************************************/

sMemoryPool::sMemoryPoolBuffer::sMemoryPoolBuffer(sPtr size,sInt flags)
{
  Start = (sU8 *)sAllocMem(size,16,flags);
  Current = Start;
  End = Start + size;
  Next = 0;
}

sMemoryPool::sMemoryPoolBuffer::~sMemoryPoolBuffer()
{
  sFreeMem(Start);
}

/****************************************************************************/

sMemoryPool::sMemoryPool(sPtr defaultsize,sInt allocflags, sInt maxstacksize)
{
  DefaultSize = defaultsize;
  AllocFlags = allocflags;
  sPushMemType(AllocFlags);
  First = Current = Last = new sMemoryPoolBuffer(DefaultSize,AllocFlags);
  Stack.HintSize(maxstacksize);
  sPopMemType(AllocFlags);
}

sMemoryPool::~sMemoryPool()
{
  sMemoryPoolBuffer *i,*n;

  i = First;
  while(i)
  {
    n = i->Next;
    delete i;
    i = n;
  }
}

void sMemoryPool::Reset()
{
  sMemoryPoolBuffer *i,*n;

  i = First;
  while(i)
  {
    n = i->Next;
    delete i;
    i = n;
  }

  Stack.Clear();
  sPushMemType(AllocFlags);
  First = Current = Last = new sMemoryPoolBuffer(DefaultSize,AllocFlags);
  sPopMemType(AllocFlags);
}

sU8 *sMemoryPool::Alloc(sPtr size,sInt align)
{
  sU8 *result;
  if(size>DefaultSize/2)          // special case: very large object.
  {
    sPushMemType(AllocFlags);
    sMemoryPoolBuffer *b = new sMemoryPoolBuffer(size+align,AllocFlags);
    sPopMemType(AllocFlags);
    b->Next = First;
    First = b;
    b->Current = sAlign(b->Current,align);
    result = b->Current;
    b->Current += size;
  }
  else                            // normal case: stuff it in!
  {
    Current->Current = sAlign(Current->Current,align);
    if(Current->Current+size>Current->End)
    {
      if(Current!=Last)
      {
        Current = Current->Next;
        Current->Current = sAlign(Current->Current,align);
      }
      else
      {
        sPushMemType(AllocFlags);
        Last->Next = new sMemoryPoolBuffer(DefaultSize,AllocFlags);
        sPopMemType(AllocFlags);
        Last = Last->Next;
        Last->Current = sAlign(Last->Current,align);    // if align>DefaultSize/2 we have a problem...
        Current = Last;
      }
    }
    result = Current->Current;
    Current->Current += size;
  }

  return result;
}

sChar *sMemoryPool::AllocString(const sChar *str)
{
  return AllocString(str,sGetStringLen(str));
}

sChar *sMemoryPool::AllocString(const sChar *str,sInt len)
{
  sVERIFY(len>=0);
  sChar *r = Alloc<sChar>(len+1);
  sCopyMem(r,str,len*sizeof(sChar));
  r[len] = 0;
  return r;
}


void sMemoryPool::Push()
{
  sStackItem *item = Stack.AddMany(1);
  item->CBuffer = Current;
  item->CPtr = Current->Current;
}

void sMemoryPool::Pop()
{
  sInt count = Stack.GetCount();
  sVERIFY(count);
  sStackItem *item = &Stack[count-1];
  Current = item->CBuffer;
  sMemoryPoolBuffer *buf = item->CBuffer;
  buf->Current = item->CPtr;
  buf = buf->Next;
  while(buf)
  {
    buf->Current = buf->Start;
    buf = buf->Next;
  }
  Stack.RemTail();
}

sPtr sMemoryPool::BytesAllocated() const
{
  const sMemoryPoolBuffer *buf = First;
  sPtr allocsize = buf->Current-buf->Start;
  while(buf!=Last)
  {
    buf = buf->Next;
    allocsize += buf->Current-buf->Start;
  }
  return allocsize;
}

sPtr sMemoryPool::BytesReserved() const
{
  const sMemoryPoolBuffer *buf = First;
  sPtr memsize = buf->End-buf->Start;
  while(buf!=Last)
  {
    buf = buf->Next;
    memsize += buf->End-buf->Start;
  }
  return memsize;
}

/****************************************************************************/
/***                                                                      ***/
/***   Memory Leak Counter                                                ***/
/***                                                                      ***/
/****************************************************************************/
/*
sMemoryLeakTracker1::sMemoryLeakTracker1()
{
  LeakAlloc = 0;
  LeakUsed = 0;
  LeakData = 0;
  Lock = 0;
}

void sMemoryLeakTracker1::Init(sInt listsize,sInt allocflags)
{
  Lock = new sThreadLock();
  AllocFlags = allocflags;
  LeakAlloc = listsize;
  LeakUsed = 0;
  if (sIsMemTypeAvailable(AllocFlags))
    LeakData = (LeakCheck *) sAllocMem(sizeof(LeakCheck)*LeakAlloc,4,AllocFlags);
  else
    LeakData=0;

  if(LeakData==0)
  {
    sLogF(L"mem",L"leak debugging disabled (no workmem)\n");
    LeakAlloc = 0;
  }
  else
    sLogF(L"mem",L"initializing leak debugging %08x..%08x\n",sPtr(LeakData),sPtr(LeakData)+(sPtr)sizeof(LeakCheck)*LeakAlloc);

}

void sMemoryLeakTracker1::Exit()
{
  LeakCheck *d = LeakData;
  LeakData = 0;
  LeakAlloc = 0;
  LeakUsed = 0;
  sFreeMem(d);
  delete Lock;
}


void sMemoryLeakTracker1::AddLeak(void *ptr,sPtr size,const char *file,sInt line,sInt allocid,sInt heapid)
{
  if (LeakAlloc)
  {
    sScopeLock l(Lock);
    if(LeakUsed<LeakAlloc)
    {
      sInt i = LeakUsed++;
      LeakData[i].Ptr = ptr;
      LeakData[i].Size = size;
      LeakData[i].File = file;
      LeakData[i].Line = line;
      LeakData[i].AllocId = allocid;
      LeakData[i].HeapId = heapid;
      LeakData[i].pad = 0;
      LeakData[i].Duplicates = 0;
      LeakData[i].TotalBytes = 0;
    }
    else
    {
      sFatal(L"sMemoryLeakTracker1 overflow");
    }
#if sCONFIG_DEBUGMEM
    sGetThreadContext()->TagMemFile = 0;
#endif
  }
}

void sMemoryLeakTracker1::RemLeak(void *ptr)
{
  if (LeakAlloc)
  {
    sScopeLock l(Lock);

    for(sInt i=LeakUsed-1;i>=0;i--)
    {
      if(LeakData[i].Ptr==ptr)
      {
        LeakData[i] = LeakData[--LeakUsed];
        return;
      }
    }
    sFatal(L"delete without new");
  }
}


void sMemoryLeakTracker1::DumpLeaks(const sChar *heapname,sInt minallocid)
{
  sScopeLock l(Lock);
  sInt count = 0;
  sPtr bytes = 0;
  sInt maxallocid = sMemoryAllocId;
  for(sInt i=0;i<LeakUsed;i++)
  {
    if(LeakData[i].AllocId>=minallocid)
    {
      count++;
      bytes += LeakData[i].Size;
    }
  }

  if(count>0)
  {
    sLogF(L"mem",L"%q: (%d leaks in %K Bytes total)\n",heapname,count,bytes);
    sPushMemType(AllocFlags);

    // merge leaks

    sHashTable<LeakCheck,LeakCheck> hash;
    LeakCheck *n;
    for(sInt i=0;i<LeakUsed;i++)
    {
      if(LeakData[i].AllocId>=minallocid && LeakData[i].AllocId<maxallocid)
      {
        n = hash.Find(&LeakData[i]);
        if(n)
        {
          n->TotalBytes += LeakData[i].Size;
          n->Duplicates ++;
        }
        else
        {
          n = &LeakData[i];
          n->TotalBytes = LeakData[i].Size;
          n->Duplicates = 1;
          hash.Add(&LeakData[i],&LeakData[i]);
        }
      }
    }

    // sort leaks
    
    sArray<LeakCheck *> Leaks;
    hash.GetAll(&Leaks);
    sSortDown(Leaks,&LeakCheck::Size);
    sFORALL(Leaks,n)
    {
      sString<128> s,b;
      if(n->File)
        sCopyString(s,n->File,128);
      else
        s = L"unknown";
      b.PrintF(L"%p(%d):",s,n->Line);
      sDPrintF(L"%s%_ %5k bytes,  %5k*%-5d, group %-2d, id:%d\n",b,60-sGetStringLen(b),n->TotalBytes,n->Size,n->Duplicates,n->HeapId,n->AllocId);
    }
    sPopMemType(AllocFlags);
  }
}


sInt sMemoryLeakTracker1::BeginGetLeaks(const sMemoryLeakTracker1::LeakCheck *&leaks)
{
  Lock->Lock();
  leaks=LeakData;
  return LeakUsed;
}

void sMemoryLeakTracker1::EndGetLeaks()
{
  Lock->Unlock();
}
*/
/****************************************************************************/
/***                                                                      ***/
/***   Track Memory Leaks, second try                                     ***/
/***                                                                      ***/
/****************************************************************************/
#if sCONFIG_DEBUGMEM

void sPushMemLeakDesc(const sChar *desc)
{
  sThreadContext *tc=sGetThreadContext();

  sInt dl=sMin(sGetStringLen(desc),sMemoryLeakDesc::STRINGLEN-tc->MLDBPos-1);

  if (dl<sGetStringLen(desc))
  {
    sLogF(L"mem",L"Warning: sPushMemLeakDesc overflow!\n");
    return;
  }

  if (tc->MLDBPos>0) tc->MemLeakDescBuffer2[tc->MLDBPos-1]=' ';

  for (sInt i=0; i<dl; i++, tc->MLDBPos++)
    tc->MemLeakDescBuffer[tc->MLDBPos]=tc->MemLeakDescBuffer2[tc->MLDBPos]=desc[i];

  tc->MemLeakDescBuffer[tc->MLDBPos]=0;
  tc->MemLeakDescBuffer2[tc->MLDBPos]=0;
  tc->MLDBPos++;
  tc->MLDBCRC=sChecksumRandomByte((const sU8*)tc->MemLeakDescBuffer2,2*tc->MLDBPos);
}

void sPopMemLeakDesc()
{
  sThreadContext *tc=sGetThreadContext();

  if (!tc->MLDBPos)
  {
    sLogF(L"mem",L"Warning: sPopMemLeakDesc underflow!\n");
    return;
  }
  tc->MLDBPos-=2;
  if (tc->MLDBPos<=0)
  {
    tc->MLDBPos=0;
    tc->MemLeakDescBuffer[tc->MLDBPos]=0;
    tc->MemLeakDescBuffer2[tc->MLDBPos]=0;
    tc->MLDBCRC=0;
  }
  for (;tc->MLDBPos>0 && tc->MemLeakDescBuffer[tc->MLDBPos]!=0;--tc->MLDBPos) {}
  tc->MemLeakDescBuffer[tc->MLDBPos]=0;
  tc->MemLeakDescBuffer2[tc->MLDBPos]=0;
  if (tc->MLDBPos) tc->MLDBPos++;
  tc->MLDBCRC=sChecksumRandomByte((const sU8*)tc->MemLeakDescBuffer2,2*tc->MLDBPos);
}

#endif

sMemoryLeakTracker2::sMemoryLeakTracker2()
{
  Pool = 0;
  sClear(LocHash);
  sClear(LeakHash);
  sClear(DescHash);
  LeakCount = 0;
  AllocFlags = 0;
  Lock = 0;
  InformationStrings = sNULL;
}

sMemoryLeakTracker2::~sMemoryLeakTracker2()
{
  Exit();
}

void sMemoryLeakTracker2::Init2(sInt allocflags)
{
  Lock = new sThreadLock();
  AllocFlags = allocflags|sAMF_NOLEAK;
  Pool = new sMemoryPool(256*1024,AllocFlags);
}

void sMemoryLeakTracker2::Exit()
{
  sClear(LocHash);
  sClear(LeakHash);
  FreeLeaks.Clear();
  Descs.Clear();
  FreeDescs.Clear();
  sDelete(Pool);
  sDelete(Lock);
}

void sMemoryLeakTracker2::DumpLeaks(const sChar *message,sInt minallocid,sInt heapid)
{
  if (!Pool) return;

  sPushMemType(AllocFlags);

  sMemoryLeakInfo *info;
  sStaticArray<sMemoryLeakInfo> locs;
  sStaticArray<sMemoryLeakDesc> descs;

  GetLeaks(locs,minallocid);

  if(locs.GetCount()>0)
  {
    sInt totalleak = 0;
    sInt totalmem = 0;

    sFORALL(locs,info) if (!heapid || (info->HeapId&sAMF_MASK)==(heapid&sAMF_MASK))
    {
      totalleak += info->Count;
      totalmem += info->TotalBytes;
    }

    sLogF(L"mem",L"%q: (%d leaks in %K Bytes total)\n",message,totalleak,totalmem);
    sSortDown(locs,&sMemoryLeakInfo::TotalBytes);
    sFORALL(locs,info) if (!heapid || (info->HeapId&sAMF_MASK)==(heapid&sAMF_MASK))
    {
      sString<128> s,b;
      if(info->File)
        sCopyString(s,info->File,128);
      else
        s = L"unknown";
      b.PrintF(L"%p(%d):",s,info->Line);
      sDPrintF(L"%s%_ %5K bytes in %5K chunks %5K avg group %-2d id:%d first bytes: %d (%08x)\n",b,
        60-sGetStringLen(b),info->TotalBytes,info->Count,info->TotalBytes/info->Count,info->HeapId,info->AllocId,info->FirstSize,info->FirstSize);
    }

    GetLeakDescriptions(descs, minallocid);
    if (descs.GetCount()>1 || minallocid>0)
    {
      sMemoryLeakDesc *d;

      sHeapSortUp(descs,&sMemoryLeakDesc::String);
      sDPrintF(L"\n");
      sLogF(L"mem",L"Memory usage by description:\n");
      if (minallocid>0)
        sLogF(L"mem",L"(warning: this list is incomplete)\n");
      sFORALL(descs,d)
      {
        sInt size=0; 
        sInt count=0;
        if (heapid)
        {
          size=d->Size[heapid&sAMF_MASK];
          count=d->Count[heapid&sAMF_MASK];
        }
        else for (sInt i=1; i<=sAMF_MASK; i++)
        {
          size+=d->Size[i];
          count+=d->Count[i];
        }
        if (count)
          sDPrintF(L"%s%_: %5K bytes in %5K chunks %5K avg\n",d->String,60-sGetStringLen(d->String),size,count,size/count);       
      }
    }

  }
  sPopMemType(AllocFlags);
}


static sBool cmp(sMemoryLeakTracker2::LeakLoc *loc,sInt line,sInt heapid,const char *file)
{
  if(loc->Line!=line) return 0;
  if(loc->HeapId!=heapid) return 0;
  if(loc->File==file) return 1;
  if(loc->File==0 || file==0) return 0;
  for(sInt i=0;loc->File[i] || file[i];i++) 
    if(loc->File[i]!=file[i]) return 0; 
  return 1;
}

static sInt sMemoryLeakTracker2Hash(void *ptr,sInt hashSize)
{
  sDInt ptrInt = (sDInt) ptr;
  sU64 nHashVal    = 0x84222325cbf29ce4ULL,
       nMagicPrime = 0x00000100000001b3ULL;

  nHashVal ^= ((ptrInt >>  0) & 0xffff);
  nHashVal *= nMagicPrime;
  nHashVal ^= ((ptrInt >> 16) & 0xffff);
  nHashVal *= nMagicPrime;
#if sCONFIG_64BIT
  nHashVal ^= ((ptrInt >> 32) & 0xffff);
  nHashVal *= nMagicPrime;
  nHashVal ^= ((ptrInt >> 48) & 0xffff);
  nHashVal *= nMagicPrime;
#endif

  return nHashVal & (hashSize-1);
}

void sMemoryLeakTracker2::AddLeak( void *ptr,sPtr size,const char *file,sInt line,sInt allocid,sInt heapid, const sChar *descstr, sU32 descCRC )
{
  if (!Pool) return;

  LeakLoc *loc;
  Leak *leak;
  sMemoryLeakDesc *desc;

  Lock->Lock();

  // find leak loc
  sInt hash = line & (LocHashSize-1);
  loc = LocHash[hash];
  while(loc)
  {
    if(cmp(loc,line,heapid,file))
      break;
    loc = loc->NextHash;
  }
  if(loc==0)
  {
    loc = Pool->Alloc<LeakLoc>();
    loc->File = file;
    loc->Line = line;
    loc->HeapId = heapid;
    loc->NextHash = LocHash[hash];
    loc->Leaks.Clear();
    LocHash[hash] = loc;
  }

  // find leak description
  if (!descstr || !*descstr) 
  {
    descstr=L"(unknown)";
    descCRC=0;
  }

  hash = descCRC & (DescHashSize-1);
  desc = DescHash[hash];
  while (desc)
  {
    if (!sCmpString(descstr,desc->String))
      break;
    desc=desc->NextHash;
  }
  if (desc==0)
  {
    if (FreeDescs.IsEmpty())
      desc = Pool->Alloc<sMemoryLeakDesc>();
    else
      desc = FreeDescs.RemTail();

    desc->String=descstr;
    desc->RefCount=0;
    sClear(desc->Size);
    sClear(desc->Count);
    desc->Hash=hash;
    desc->NextHash = DescHash[hash];
    desc->AllocId = allocid;
    DescHash[hash]=desc;
    Descs.AddTail(desc);
  }
  desc->RefCount++;
  desc->Count[heapid&sAMF_MASK]++;
  desc->Size[heapid&sAMF_MASK]+=size;

  if(FreeLeaks.IsEmpty())
    leak = Pool->Alloc<Leak>();
  else
    leak = FreeLeaks.RemTail();
  leak->Ptr = ptr;
  leak->Size = size;
  leak->AllocId = allocid;
  leak->HeapId = heapid;
  leak->Desc=desc;
  leak->NextHash = 0;

  loc->Leaks.AddTail(leak);

  hash = sMemoryLeakTracker2Hash(ptr,LeakHashSize);
  //hash = ((sDInt(ptr)>>4)|(sDInt(ptr)>>16)) & (LeakHashSize-1);
  leak->NextHash = LeakHash[hash];
  LeakHash[hash] = leak;
  
  LeakCount++;

  Lock->Unlock();
}

void sMemoryLeakTracker2::RemLeak(void *ptr)
{
  if (!Pool) return;

  Lock->Lock();

  sInt hash = sMemoryLeakTracker2Hash(ptr,LeakHashSize);
  //sInt hash = ((sDInt(ptr)>>4)|(sDInt(ptr)>>16)) & (LeakHashSize-1);
  Leak **leakp = &LeakHash[hash];
  Leak *leak = *leakp;
  while(leak)
  {
    if(leak->Ptr==ptr)
    {
      *leakp = leak->NextHash;
      goto found;
    }
    leakp = &leak->NextHash;
    leak = *leakp;
  }
  Lock->Unlock();
  sDPrintF(L"warning: delete without new: 0x%08x. may happen inside leaktracker\n",sPtr(ptr));
  return;
found:

  sMemoryLeakDesc *desc=leak->Desc;
  desc->Size[leak->HeapId&sAMF_MASK]-=leak->Size;
  desc->Count[leak->HeapId&sAMF_MASK]--;
  desc->RefCount--;
  if (!desc->RefCount)
  {
    desc->Node.Rem();
    FreeDescs.AddTail(leak->Desc);

    sMemoryLeakDesc **descp=&DescHash[desc->Hash];

    while (*descp)
    {
      if (*descp==desc)
      {
        *descp=desc->NextHash;
        break;
      }
      descp=&((*descp)->NextHash);
    }    
  }
  leak->Desc=0; // for safety

  leak->Node.Rem();
  FreeLeaks.AddTail(leak);
  LeakCount--;

  Lock->Unlock();
}

void sMemoryLeakTracker2::GetLeaks(sStaticArray<sMemoryLeakInfo> &locs,sInt minallocid)
{
  if (!Pool) return;

  LeakLoc *loc;
  Leak *leak;
  sMemoryLeakInfo *info;

  Lock->Lock();

  // count locations

  sInt count=0;
  for(sInt i=0;i<LocHashSize;i++)
  {
    loc = LocHash[i];
    while(loc)
    {
      if(!loc->Leaks.IsEmpty())
        count++;
      loc = loc->NextHash;
    }
  }
  
// do we leak?

  if(count>0)
  {
    locs.HintSize(count+1); // +1 because this very call might open a new location entry
    sInt maxallocid = sMemoryAllocId;

    for(sInt i=0;i<LocHashSize;i++)
    {
      loc = LocHash[i];
      while(loc)
      {
        if(!loc->Leaks.IsEmpty())
        {
          sInt totalleak=0;
          sInt totalmem=0;
          sInt allocid=maxallocid+1;
          sInt firstsize = 0;
          sFORALL_LIST(loc->Leaks,leak)
          {
            if(leak->AllocId>=minallocid && leak->AllocId<maxallocid)
            {
              if(leak->AllocId<allocid)
              {
                allocid = leak->AllocId;
                firstsize = leak->Size;
              }
              totalleak++;
              totalmem += leak->Size;
            }
          }
          if(totalleak>0)
          {
            info = locs.AddMany(1);
            info->File = loc->File;
            info->Line = loc->Line;
            info->HeapId = loc->HeapId;
            info->AllocId = allocid;
            info->Count = totalleak;
            info->TotalBytes = totalmem;
            info->FirstSize = firstsize;
          }
        }
        loc = loc->NextHash;
      }
    }
  }

  Lock->Unlock();
}


void sMemoryLeakTracker2::GetLeakDescriptions(sStaticArray<sMemoryLeakDesc> &descs, sInt minid)
{
  if (!Pool) return;

  Lock->Lock();

  // count descriptions

  sInt count=Descs.GetCount();

  // do we leak?
  if(count>0)
  {
    descs.HintSize(count);
//    sInt maxallocid = sMemoryAllocId;

    sMemoryLeakDesc *dp=Descs.GetHead();
    for(sInt i=0;i<count;i++, dp=Descs.GetNext(dp))
    {
      if (dp->AllocId>=minid)
       descs.AddTail(*dp);
    }

  }

  Lock->Unlock();
}


sInt sMemoryLeakTracker2::CmpString8(const sChar *a, const sChar8 *b)
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

sChar8 *sMemoryLeakTracker2::GetPersistentString8(const sChar *informationString)
{
  sChar8 *result = sNULL;
  
  if (Pool)
  {
    Lock->Lock();

    // find string in list and sort LRU-Style
    sMemoryLeakInformationStrings *infoStrings = InformationStrings;
    sMemoryLeakInformationStrings *infoStrings_prev = sNULL;
    sInt i=0;
    while (!result && infoStrings)
    {
      result = infoStrings->InformationString;
      if (CmpString8(informationString, result)==0)
      {
        // hit - result is valid
        // move node to the start of the list as new head
        if (infoStrings_prev)
        {
          infoStrings_prev->NextInfo = infoStrings->NextInfo;
          infoStrings->NextInfo = InformationStrings;
          InformationStrings = infoStrings;
        }
      }
      else
      {
        // miss - try the next one
        result = sNULL;
        infoStrings_prev = infoStrings;
        infoStrings = infoStrings->NextInfo;
      }
      
      i++;
    }

    // if no string is found make a copy and register it
    if (!result)
    {
      // create node
      infoStrings = Pool->Alloc<sMemoryLeakInformationStrings>();
    
      // fill data
      sInt len = sGetStringLen(informationString)+1;
      result = (sChar8*)Pool->Alloc(len * sizeof(sChar8), 1);
      sCopyString(result, informationString, len);
      infoStrings->InformationString = result;
      
      // add at head
      infoStrings->NextInfo = InformationStrings;
      InformationStrings = infoStrings;
    }

    Lock->Unlock();
  }

  return result;
}

/****************************************************************************/
/***                                                                      ***/
/***   Memory Heap (simple, slow, efficient)                              ***/
/***                                                                      ***/
/****************************************************************************/


#if sCONFIG_64BIT
#define HEADER 8
#define OFFSET 24
#define MASK 31
#define ALIGN 32
#else
#define HEADER 4      // HEADER == sizeof(void *)
#define OFFSET 12     // OFFSET == sizeof(sMemoryHeapFreeNode)
#define MASK 15       // MASK + 1 == ALIGN
#define ALIGN 16      // HEADER + OFFSET == ALIGN
#endif
#define MEMVERBOSE 0

sMemoryHeap::sMemoryHeap()
{
  Start = 0;
  End = 0;
  TotalFree = 0;
  LastFreeNode = 0;
  Clear = MEMVERBOSE;
}

void sMemoryHeap::Init(sU8 *start,sPtr size)
{
  sVERIFY(start);
  Start = sPtr(start);
  End = Start + size;

  Start = ((Start+HEADER+MASK)&(~MASK))-HEADER;
  End = ((End+HEADER)&(~MASK))-HEADER;

  TotalFree = End-Start;

  if(Clear) sSetMem((void *)Start,0xaa,End-Start);

  sMemoryHeapFreeNode *node = (sMemoryHeapFreeNode *) Start;
  node->Size = TotalFree;
  FreeList.AddHead(node);
  LastFreeNode = node;
  MinTotalFree = GetFree();
}

void sMemoryHeap::SetDebug(sBool clear,sInt memwall)
{
  Clear = clear;
}

/****************************************************************************/

sCONFIG_SIZET sMemoryHeap::GetFree()
{
  return TotalFree;
}

sCONFIG_SIZET sMemoryHeap::GetLargestFree()
{
  Lock();
  sPtr max=0;
  sMemoryHeapFreeNode *n;
  sFORALL_LIST(FreeList,n)
    max=sMax(max,n->Size);
  Unlock();
  return max;
}

sPtr sMemoryHeap::GetUsed()
{
  return End-Start-TotalFree;
}

void sMemoryHeap::DumpStats(sInt verbose)
{
  sInt count = 0;
  sInt fcount = 0;
  sPtr largest = 0;
  sPtr free = 0;
  sPtr smallest = 0x7fffffff;
  sPtr fragged = 0;
  sMemoryHeapFreeNode *node;

  sFORALL_LIST(FreeList,node)
  {
    count++;
    free += node->Size;
    if(node->Size<64*1024)
    {
      fragged += node->Size;
      fcount++;
    }
    if(node->Size>largest)
      largest = node->Size;
    if(node->Size<smallest)
      smallest = node->Size;
  }
  if(count==0)
    smallest = 0;

  sU32 hash = MakeSnapshot();
  sLogF(L"mem",L"HeapStats: %08x..%08x, HASH %08x\n",Start,End,hash);
  sLogF(L"mem",L"%08x(%5K) Free in %d nodes\n",free,free,count);
  sLogF(L"mem",L"%08x(%5K) Largest\n",largest,largest);
  sLogF(L"mem",L"%08x(%5K) Smallest\n",smallest,smallest);
  sLogF(L"mem",L"%08x(%5K) Fragged in %d nodes\n",fragged,fragged,fcount);
  sLogF(L"mem",L"%08x(%5K) Unfragged in %d nodes\n",free-fragged,free-fragged,count-fcount);
  if(free!=TotalFree)
    sLogF(L"mem",L"Heap Corrupted! (%08x free, %08x should)\n",free,TotalFree);
}

sU32 sMemoryHeap::MakeSnapshot()
{
  sMemoryHeapFreeNode *node;

  sInt freeNodeCount = 0;
  sChecksumAdler32Begin();
  sLogF(L"mem", L"starting snapshot\n");
  sFORALL_LIST(FreeList,node)
  {
    sChecksumAdler32Add((const sU8 *)node,sizeof(*node));
    // use this for verbose debugging
    //sLogF(L"mem", L"0x%08x : node = 0x%08x  size = %d\n", (sInt)node, (sInt)&node->Node, node->Size);
    freeNodeCount++;
  }
  sLogF(L"mem", L"Free mem nodes: %d\n", freeNodeCount);
  // use this for verbose debugging
  // sMemoryLeaks->DumpLeaks(L"Current leaks", 0,0);

  return sChecksumAdler32End();
}

sBool sMemoryHeap::IsFree(const void *ptr) const
{
  const sMemoryHeapFreeNode *node;

  sFORALL_LIST(FreeList,node)
  {
    sU8 *ns = (sU8 *)node;
    sU8 *ne = ns + node->Size;
    if(ptr>=ns && ptr<ne) 
      return 1;
  }
  return 0;
}

void sMemoryHeap::Validate()
{
  sPtr free = 0;
  sPtr last = 0;
  sPtr t = 0;
  sInt errors = 0;
  sMemoryHeapFreeNode *node,*prev,*next;

  sFORALL_LIST(FreeList,node)
  {
    prev = FreeList.GetPrev(node);
    next = FreeList.GetNext(node);
    if(FreeList.GetNext(prev)!=node || FreeList.GetPrev(next)!=node)
    {
      sLogF(L"mem",L"linked list broken\n");
      errors++;
      break;
    }
    if(node->Size<ALIGN || node->Size>(End-Start))
    {
      sLogF(L"mem",L"invalid free node size! %08x for %08x\n",sPtr(node),node->Size);
      errors++;
    }
    free += node->Size;
    t = sPtr(node);
    if(t==last)
    {
      sLogF(L"mem",L"nodes not merged! %08x..%08x\n",last,t);
      errors++;
    }
    if(t<last)
    {
      sLogF(L"mem",L"nodes not sorted! %08x..%08x\n",last,t);
      errors++;
    }
    last = t+node->Size;
    if(t<Start || last>End)
    {
      sLogF(L"mem",L"nodes outside heap! %08x..%08x\n",t,last);
      errors++;
    }
  }

  if(free!=TotalFree)
  {
    sLogF(L"mem",L"%08x free, %08x expected\n",free,TotalFree);
    errors++;
  }
  if(errors)
    sFatal(L"heap corrupted");
}

void sMemoryHeap::ClearFree()
{
  sMemoryHeapFreeNode *node;

  Validate();

  sU8 pattern=0x00;

  sLogF(L"mem",L"clearing mem with 0x%02x\n",pattern);

  sFORALL_LIST(FreeList,node)
    sSetMem(((sU8 *)node)+sizeof(sMemoryHeapFreeNode),pattern,node->Size-sizeof(sMemoryHeapFreeNode));
}

/****************************************************************************/

void *sMemoryHeap::Alloc(sPtr bytes,sInt align,sInt flags)
{
  sMemoryHeapFreeNode *n,*p;
  sPtr size = ((bytes+HEADER+MASK)&(~MASK));   // align size to 16'er, including header
  sVERIFY(bytes<size);                         // overflow check
  if(align<ALIGN) align = ALIGN;

  if(/*size<0 ||*/ size>TotalFree) return 0;       // no way we can do it

  if(MEMVERBOSE) Validate();

  sBool found = 0;
  sPtr fs,fe,as,ae;

  if(!(flags & sAMF_ALT))
  {
    sFORALL_LIST(FreeList,n)
    {
      fs = sPtr(n);                // free start
      fe = fs+n->Size;              // free end
      as = sAlign(fs+HEADER,align)-HEADER;   // allocated start, aligned and with header
      ae = as+size;                 // allocated end
      if(ae<=fe)                          // does it fit?
      {
        sVERIFY(as>=fs);
        sVERIFY(ae<=fe);
        found = 1;
        break;
      }
    }
  }
  else
  {
    sFORALL_LIST_REVERSE(FreeList,n)
    {
      fs = sPtr(n);                // free start
      fe = fs+n->Size;              // free end
      as = ((fe-size+HEADER)&~(align-1))-HEADER;   // allocated start, aligned and with header
      ae = as+size;                 // allocated end
      if(fs<=as)                          // does it fit?
      {
        sVERIFY(as>=fs);
        sVERIFY(ae<=fe);
        found = 1;
        break;
      }
    }
  }

  if(found)                          // does it fit?
  {
    if(as!=fs)                        // free space at start to pad misalignment
    {
      sVERIFY(((as-fs)&MASK)==0);     // free space should be multiple of 16
      n->Size = (as-fs);              // trim node to new size
      p = n;                          // link rest after this node
    }
    else                              // no free space at start (small alignemnt)
    {
      p = FreeList.GetPrev(n);
      if(!FreeList.IsLast(n))
        LastFreeNode = FreeList.GetNext(n);
      else if(!FreeList.IsFirst(n))
        LastFreeNode = FreeList.GetPrev(n);
      else
        LastFreeNode = 0;
      FreeList.Rem(n);
    }
    if(ae!=fe)                        // add node at end
    {
      sVERIFY(((fe-ae)&MASK)==0);
      sVERIFY(((fe+HEADER)&MASK)==0);
      n = (sMemoryHeapFreeNode *)(ae);
      n->Size = fe-ae;
      FreeList.AddAfter(n,p);
    }

    if(Clear) sSetMem((void *)as,0xcc,ae-as);
    *((sPtr *)as) = bytes;

    TotalFree -= size;
    MinTotalFree = sMin(TotalFree,MinTotalFree);

    sVERIFY(((as+HEADER)&MASK)==0);

    if(MEMVERBOSE) Validate();

    sAtomicAdd(&sMemoryUsed,size);
    
    return (sU8 *) (as+HEADER);
  }

  return 0;
}

sBool sMemoryHeap::Free(void *ptr)
{
#if sCFG_MEMDBG_LOCKING
  if(sMemDbgCheckLock(ptr))
    sFatal(L"called free an locked memory\n");
#endif

  sMemoryHeapFreeNode *n,*b;

  sPtr bytes = ((sPtr *)ptr)[-1];
  sPtr size = ((bytes+HEADER+MASK)&(~MASK));   // align size to 16'er, including header
  sPtr bs = sPtr(ptr)-HEADER;
  sPtr be = bs+size;

  if(MEMVERBOSE) Validate();
  if(MEMVERBOSE) sVERIFYRELEASE(!IsFree(ptr));

  sAtomicAdd(&sMemoryUsed,-(sDInt)size);

  if(Clear) sSetMem((void *)(((sU8 *)ptr)-HEADER),0xee,size);

  sVERIFY(bs>=Start && be<=End);

  TotalFree += size;

  b = (sMemoryHeapFreeNode *) bs;
  b->Size = size;
  
  if(FreeList.IsEmpty())
  {
    FreeList.AddHead(b);
    return 1;
  }

#if 1
  sMemoryHeapFreeNode *end = FreeList.GetTail();

  if(be<=sPtr(end))                         // freed block in free list memory range?
  {
    sMemoryHeapFreeNode *start = FreeList.GetHead();

    // use last freed node to divide search list
    if(LastFreeNode)
    {
      if(be<=sPtr(LastFreeNode))
        end = LastFreeNode;
      else
        start = LastFreeNode;
    }

    if(be<=sPtr(start))                     // new first element?
    {
      FreeList.AddBefore(b,start);
      goto found;
    }
    else if(bs-sPtr(start)<sPtr(end)-be)    // forward search ptr in free list
    {
      n = start;
      while(1)
      {
        sPtr sn = sPtr(n);
        if(be<=sn)
        {
          FreeList.AddBefore(b,n);
          goto found;
        }
        if(n==end)
          break;
        n = FreeList.GetNext(n);
      }
    }
    else                                    // backward search ptr in free list
    {
      n = end;
      while(1)
      {
        sPtr en = sPtr(n)+n->Size;
        if(bs>=en)
        {
          FreeList.AddAfter(b,n);
          goto found;
        }
        sVERIFY(n!=start);
        if(n==start)
          break;
        n = FreeList.GetPrev(n);
      }
    }
  }
#else
  sFORALL_LIST(FreeList,n)
  {
    sPtr sn = sPtr(n);
    if(be<=sn)
    {
      FreeList.AddBefore(b,n);
      goto found;
    }
  }
#endif
  FreeList.AddTail(b);
found:;

  if(!FreeList.IsFirst(b))
  {
    sMemoryHeapFreeNode *a = FreeList.GetPrev(b);
    sPtr ae = sPtr(a)+a->Size;
    if(ae == bs)
    {
      FreeList.Rem(b);
      a->Size += b->Size;
      b = a;
    }
  }
  if(!FreeList.IsLast(b))
  {
    sMemoryHeapFreeNode *c = FreeList.GetNext(b);
    sPtr cs = sPtr(c);
    if(be==cs)
    {
      FreeList.Rem(c);
      b->Size += c->Size;
    }
  }
  if(MEMVERBOSE) Validate();
  LastFreeNode = b;

  return 1;
}

sPtr sMemoryHeap::MemSize(void *ptr)
{
  sPtr bytes = ((sPtr *)ptr)[-1];
  return bytes;
}

/****************************************************************************/

#undef HEADER
#undef OFFSET
#undef MASK


/****************************************************************************/
/***                                                                      ***/
/***   memory heap, usefull for gpu-memory scenarious (slow read access)  ***/
/***                                                                      ***/
/****************************************************************************/

sGpuHeap::sGpuHeap()
{
  HashTable = 0;
  HashSize = 0;
  HashMask = 0;
  HashShift = 0;
  NodeMem = 0;
  TotalFree = 0;
  Clear = 0;
}

sGpuHeap::~sGpuHeap()
{
  delete[] HashTable;
  delete[] NodeMem;
}

void sGpuHeap::Init(sU8 *start,sPtr size,sInt maxallocations)
{
  Start = sPtr(start);
  End = Start+size;
  TotalFree = End - Start;

  // init hashtable

  HashShift = sFindLowerPower(maxallocations/8);
  HashSize = 1<<HashShift;
  HashMask = HashSize-1;
  HashTable = new sGpuHeapUsedNode *[HashSize];
  for(sInt i=0;i<HashSize;i++)
    HashTable[i] = 0;

  // allocate unused nodes

  NodeMem = new sGpuHeapFreeNode[maxallocations];
  for(sInt i=0;i<maxallocations;i++)
    UnusedNodes.AddTail(&NodeMem[i]);

  // initialize free list

  sGpuHeapFreeNode *n = UnusedNodes.RemTail();
  n->Data = sPtr(start);
  n->Size = size;
  FreeList.AddTail(n);
  MinTotalFree = GetFree();
}

void sGpuHeap::Exit()
{
  FreeList.Clear();
  UnusedNodes.Clear();
  sDeleteArray(HashTable);
  sDeleteArray(NodeMem);
  HashSize = 0;
  HashMask = 0;
  HashShift = 0;
  TotalFree = 0;
}

void sGpuHeap::AddNode(sGpuHeapUsedNode *l)
{
  sU32 hash = ((l->Data>>4) ^ (l->Data>>(4+HashShift))) & HashMask;
  l->Next = HashTable[hash];
  HashTable[hash] = l;
}

sGpuHeapUsedNode *sGpuHeap::FindNode(sPtr data,sBool remove)
{
  sGpuHeapUsedNode *l,**p;
  sU32 hash = ((data>>4) ^ (data>>(4+HashShift))) & HashMask;
  p = &HashTable[hash];
  while(*p)
  {
    l = *p;
    if(l->Data==data)
    {
      if(remove)
        *p = l->Next;
      return l;
    }
    p = &l->Next;
  }
  return 0;
}

/****************************************************************************/

sCONFIG_SIZET sGpuHeap::GetFree()
{
  return TotalFree;
}

sCONFIG_SIZET sGpuHeap::GetLargestFree()
{
  Lock();
  sPtr max=0;
  sGpuHeapFreeNode *n;
  sFORALL_LIST(FreeList,n)
    max=sMax(max,n->Size);
  Unlock();
  return max;
}


sPtr sGpuHeap::GetUsed()
{
  return End-Start-TotalFree;
}

void sGpuHeap::SetDebug(sBool clear,sInt memwall)
{
  Clear = clear;
}

sU32 sGpuHeap::MakeSnapshot()
{
  sGpuHeapFreeNode *node;

  sPtr data[2];

  sChecksumAdler32Begin();
  sFORALL_LIST(FreeList,node)
  {
    data[0] = node->Data;
    data[1] = node->Size;
    sChecksumAdler32Add((const sU8 *)data,sizeof(sPtr)*2);
  }
  return sChecksumAdler32End();
}



void sGpuHeap::Validate()
{
  sPtr free = 0;
  sPtr allocated = 0;
  sPtr last = 0;
  sInt errors = 0;
  sGpuHeapFreeNode *node,*prev,*next;

  // free list

  sFORALL_LIST(FreeList,node)
  {
    prev = FreeList.GetPrev(node);
    next = FreeList.GetNext(node);
    if(FreeList.GetNext(prev)!=node || FreeList.GetPrev(next)!=node)
    {
      sLogF(L"mem",L"linked list broken\n");
      errors++;
      break;
    }
    if(node->Size<16 || node->Size>(End-Start))
    {
      sLogF(L"mem",L"invalid free node size! %08x for %08x\n",sPtr(node),node->Size);
      errors++;
    }
    free += node->Size;

    if(node->Data==last)
    {
      sLogF(L"mem",L"nodes not merged! %08x..%08x\n",last,node->Data);
      errors++;
    }
    if(node->Data<last)
    {
      sLogF(L"mem",L"nodes not sorted! %08x..%08x\n",last,node->Data);
      errors++;
    }
    last = node->Data+node->Size;
    if(node->Data<Start || last>End)
    {
      sLogF(L"mem",L"nodes outside heap! %08x..%08x\n",node->Data,last);
      errors++;
    }
  }

  if(free!=TotalFree)
  {
    sLogF(L"mem",L"%08x free, %08x expected\n",free,TotalFree);
    errors++;
  }

  // allocated

  sGpuHeapUsedNode *h;

  for(sInt i=0;i<HashSize;i++)
  {
    h = HashTable[i];
    while(h)
    {
      allocated += sAlign(h->Size,16);
      if(h->Data<Start || h->Data+h->Size>End)
      {
        sLogF(L"mem",L"allocated memory outside heap! %08x..%08x\n",h->Data,h->Data+h->Size);
        errors++;
      }
      h = h->Next;
    }
  }

  if(allocated+free!=End-Start)
  {
    sLogF(L"mem",L"some memory leaked from the heap: %08x free + %08x allocated != %08x total\n",free,allocated,End-Start);
    errors++;
  }

  // final message

  if(errors)
    sFatal(L"heap corrupted");
}

void *sGpuHeap::Alloc(sPtr bytes,sInt align,sInt flags)
{
  sGpuHeapFreeNode *n,*p;
  sPtr size = sAlign(bytes,16);         // align size to 16'er, including header
  sVERIFY(bytes<=size);                 // overflow check
  if(align<16) align = 16;

  if(/*size<0 ||*/ size>TotalFree) return 0;       // no way we can do it

  if(MEMVERBOSE) Validate();

  sBool found = 0;
  sPtr fs,fe,as,ae;

  if((flags & sAMF_ALT))          // default is reverse allocation
  {
    sFORALL_LIST(FreeList,n)
    {
      fs = n->Data;                 // free start
      fe = fs+n->Size;              // free end
      as = sAlign(fs,align);        // allocated start, aligned
      ae = as+size;                 // allocated end
      if(ae<=fe)                    // does it fit?
      {
        sVERIFY(as>=fs);
        sVERIFY(ae<=fe);
        found = 1;
        break;
      }
    }
  }
  else
  {
    sFORALL_LIST_REVERSE(FreeList,n)
    {
      fs = n->Data;                 // free start
      fe = fs+n->Size;              // free end
      as = (fe-size)&~(align-1);    // allocated start, aligned
      ae = as+size;                 // allocated end
      if(fs<=as)                    // does it fit?
      {
        sVERIFY(as>=fs);
        sVERIFY(ae<=fe);
        found = 1;
        break;
      }
    }
  }

  if(found)                          // does it fit?
  {
    if(as!=fs)                        // free space at start to pad misalignment
    {
      sVERIFY(((as-fs)&15)==0);     // free space should be multiple of 16
      n->Size = (as-fs);              // trim node to new size
      p = n;                          // link rest after this node
    }
    else                              // no free space at start (small alignemnt)
    {
      p = FreeList.GetPrev(n);
      FreeList.Rem(n);
      UnusedNodes.AddTail(n);
    }
    if(ae!=fe)                        // add node at end
    {
      sVERIFY(((fe-ae)&15)==0);
      sVERIFY(((fe)&15)==0);
      n = UnusedNodes.RemTail();
      if(!n)  sVERIFY(0);
      n->Data = ae;
      n->Size = fe-ae;
      FreeList.AddAfter(n,p);
    }

    if(Clear) sSetMem((void *)as,0xcc,ae-as);
    sGpuHeapUsedNode *alloc = (sGpuHeapUsedNode *) UnusedNodes.RemTail();
    alloc->Data = as;
    alloc->Size = bytes;
    AddNode(alloc);

    TotalFree -= size;
    MinTotalFree = sMin(TotalFree,MinTotalFree);
    sVERIFY((as&15)==0);

    if(MEMVERBOSE) Validate();

    sAtomicAdd(&sMemoryUsed,size);

    return (sU8 *) (as);
  }

  return 0;
}

sBool sGpuHeap::Free(void *ptr)
{
#if sCFG_MEMDBG_LOCKING
  if(sMemDbgCheckLock(ptr))
    sFatal(L"called free an locked memory\n");
#endif

  sGpuHeapFreeNode *n,*b;
  sGpuHeapUsedNode *old;

  if(MEMVERBOSE) Validate();

  // find allocation node

  old = FindNode(sPtr(ptr),1);
  if(old==0)
    return 0;
  sVERIFY(old->Data==sPtr(ptr));

  sPtr bytes = old->Size;
  sPtr size = sAlign(bytes,16);              // align size to 16'er, including header
  sPtr bs = sPtr(ptr);
  sPtr be = bs+size;

  b = (sGpuHeapFreeNode *) old;
  old->Size = size;
  old->Data = sPtr(ptr);

  sAtomicAdd(&sMemoryUsed,-(sDInt)size);

  if(Clear) sSetMem((void *)ptr,0xee,size);

  sVERIFY(bs>=Start && be<=End);

  TotalFree += size;

  b->Size = size;
  
  if(FreeList.IsEmpty())
  {
    FreeList.AddHead(b);
    return 1;
  }

  sFORALL_LIST(FreeList,n)
  {
    sPtr sn = n->Data;
    if(be<=sn)
    {
      FreeList.AddBefore(b,n);
      goto found;
    }
  }
  FreeList.AddTail(b);
found:;

  if(!FreeList.IsFirst(b))
  {
    sGpuHeapFreeNode *a = FreeList.GetPrev(b);
    sPtr ae = a->Data+a->Size;
    if(ae == bs)
    {
      FreeList.Rem(b);
      UnusedNodes.AddTail(b);
      a->Size += b->Size;
      b = a;
    }
  }
  if(!FreeList.IsLast(b))
  {
    sGpuHeapFreeNode *c = FreeList.GetNext(b);
    sPtr cs = c->Data;
    if(be==cs)
    {
      FreeList.Rem(c);
      b->Size += c->Size;
      UnusedNodes.AddTail(c);
    }
  }
  if(MEMVERBOSE) Validate();

  return 1;
}

sPtr sGpuHeap::MemSize(void *ptr)
{
  sGpuHeapUsedNode *l = FindNode(sPtr(ptr),0);
  if(l)
    return l->Size;
  else
    return 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   sSimpleMemPool                                                     ***/
/***                                                                      ***/
/****************************************************************************/

sSimpleMemPool::sSimpleMemPool(sInt size)
{
  DeleteMe = 1;
  Start = Current = sPtr(new sU8[size]);
  End = Start+size;
}

sSimpleMemPool::sSimpleMemPool(sU8 *data,sInt size)
{
  DeleteMe = 0;
  Start = Current = sPtr(data);
  End = Start+size;
}

sSimpleMemPool::~sSimpleMemPool()
{
  if(DeleteMe)
    delete[] (sU8 *)Start;
}

sU8 *sSimpleMemPool::Alloc(sInt size,sInt align)
{
  Current = sAlign(Current,align);
  sU8 *result = (sU8 *) Current;
  Current+=size;
  sVERIFY(Current<=End);
  return result;
}

void sSimpleMemPool::Reset()
{
  Current = Start;
}
/****************************************************************************/
/***                                                                      ***/
/***   Compression                                                        ***/
/***                                                                      ***/
/****************************************************************************/
//
//  equal sequence compression format
//
//  00pppppp pppppppp ssssssss      copy dest[-p-s-4],s+4
//  01ssssss pppppppp               copy dest[-p-s-3],s+3
//  10sspppp                        copy dest[-p-s-2],s+2
//  1100ssss ...                    copy src[0],s+1
//  1101ssss dddddddd               fill d,s+2
//  1110ssss                        fill 0,s+1
//  11110sss ssssssss ...           copy src[0],s+1+15
//  11111sss ssssssss dddddddd      fill d,s+2+15
//
/****************************************************************************/

sBool sCompES(sU8 *s,sU8 *d,sInt ssize,sInt &dsize,sInt scan)
{
  sInt si;                        // source index  
  sInt di;                        // destination index
  sInt ri;                        // bytes read from source without pattern
  sInt i,j;                       // :-)
  sInt imax;                      // temporary limit for scanning
  sInt jmin;                      // temporary limit for scanning
  sInt mode;                      // 3 = run found, 1 = sequence found
  sInt data;                      // value for run 
  sInt size;                      // size for both modes
  sInt pos;                       // offset for equal sequence (absolute)
  sInt dpos;                      // offset for equal sequence (as written)
  sInt write;                     // bytes needed to write pattern (approx.)
  sU16 seq;                       // first two characters of sequence to look for


  si=0;                           // initialize (important)
  di=0;
  ri=0;

  data = 0;                       // initialize (unimportant, only to get rid of warnings)
  size = 0;
  pos = 0;
  dpos = 0;
  write = 0;

  while(si<ssize && di<dsize)
  {
    mode = 0;
    if(ri+si+1<ssize && s[ri+si]==s[ri+si+1])   // find run
    {
      size=2;
      data = s[ri+si];
      while(ri+si+size<ssize && data==s[ri+si+size])
        size++;
      mode = 3;
      write = 2;
      if(data==0)
        write = 1;
    }
    if(ri+si+1<ssize && mode==0)                 // find equal seq
    {
      size = 0;
      j = ri+si-2;
      jmin = ri+si-scan;
      if(jmin<0)
        jmin = 0;
      seq = ((sU16 *) (s+ri+si))[0];

      while(j>=jmin)
      {
        if(((sU16 *)(s+j))[0]==seq)
        {
          i=0;
          imax = 255+4;
          imax = sMin(imax,ssize-ri-si);
          imax = sMin(imax,ri+si-j);
  //        while(ri+si+i<ssize && s[j+i]==s[ri+si+i] && j+i<ri+si && i<255+4)
          sVERIFY(ri+si+imax<=ssize);
          while(i<imax && s[j+i]==s[ri+si+i])
            i++;
          if(i>size)
          {
            size = i;
            pos = j;
            mode = 1;
          }
        }
        j--;
      }
      if(mode==1)
      {
        write = 2;
        dpos = si+ri-pos-size;
        if(dpos > 15)
          write = 3;
        if(dpos > 255)
          write = 4;
        if(dpos > 0x3fff || write>size)
          mode = 0;
      }
    }
    if((mode!=0 && (ri==0 || size>write)) || ri+si==ssize) // is it worth writing?
    {
      while(ri>0)                                 // write uncompressed data
      {
        if(ri>15+1)
        {
          i = sMin(ri,0x7ff+1+15);
          d[di++] = 0xf0 | ((i-1-15)>>8);
          d[di++] = (i-1-15)&0xff;
        }
        else
        {
          i = ri;
          d[di++] = 0xc0|(ri-1);
        }
        ri-=i;
        while(i>0)
        {
          d[di++] = s[si++];
          i--;
        }
      }
      if(mode==1)                                 // write sequence
      {
        if(size<=3+2 && dpos<=15)
        {
          d[di++] = 0x80 | ((size-2)<<4) | (dpos);
        }
        else if(size<=63+3 && dpos<=255)
        {
          d[di++] = 0x40 | (size-3);
          d[di++] = dpos;
        }
        else if(size<=255+4 && dpos<=0x3fff)
        {
          d[di++] = 0x00 | (dpos>>8);
          d[di++] = (dpos)&0xff;
          d[di++] = size-4;
        }
        si+=size;
      }
      if(mode==3)                                // write fill  
      {
        si+=size;
        while(size>0)
        {
          if(data==0 && size<=15+1)
          {
            d[di++] = 0xe0 | (size-1);
            size=0;
          }
          else if(size<=15+2)
          {
            d[di++] = 0xd0 | (size-2);
            d[di++] = data;
            size=0;
          }
          else
          {
            i = sMin(size,0x7ff+2+15);
            d[di++] = 0xf8 | ((i-2-15)>>8);
            d[di++] = (i-2-15)&0xff;
            d[di++] = data;
            size-=i;
          }
        }
      }
    }
    else
      ri++;
  }

  dsize = di;

  return (si==ssize);
}

/****************************************************************************/

sBool sDeCompES(sU8 *s,sU8 *d,sInt ssize,sInt &dsize,sBool verify)
{
  sInt val;                       // code byte    
  sInt si;                        // source stream index
  sInt di;                        // destination stream index
  sInt mode;                      // mode of operation; 1=dest copy, 2=source copy, 3=fill
  sInt pos;                       // offset for dest copy mode
  sInt size;                      // number of bytes (all modes)
  sInt data;                      // data for fill mode
  sInt i;                         // :-)

  si=0;                           // initialize (important)
  di=0;

  data = 0;                       // initialize (unimportant, only to get rid of warnings)
  pos = 0;
  
  while(si<ssize)
  {
    val = s[si++];
    if(val&0x80)
    {
      if(val&0x40)
      {
        if(val&0x20)
        {
          if(val&0x10)
          {
            if(val&0x08)    // 11111xxx
            {
              mode = 3;
              size = ((val&0x07)<<8)+s[si++]+2+15;
              data = s[si++];
            }
            else            // 11110xxx
            {
              mode = 2;
              size = ((val&0x07)<<8)+s[si++]+1+15;
            }
          }
          else              // 1110xxxx
          {
            mode = 3;
            size = (val&0x0f)+1;
            data = 0;
          }
        }
        else
        {
          if(val&0x10)      // 1101xxxx
          {
            mode = 3;
            size = (val&0x0f)+2;
            data = s[si++];
          }
          else              // 1100xxxx
          {
            mode = 2;
            size = (val&0x0f)+1;
          }
        }
      }
      else                  // 10xxxxxx
      {
        mode = 1;
        size = ((val&0x30)>>4)+2;
        pos = ((val&0x0f)+size);
      }
    }
    else
    {
      if(val&0x40)          // 01xxxxxx
      {
        mode = 1;
        size = (val&0x3f)+3;
        pos = (s[si++]+size);
      }
      else                  // 00xxxxxx
      {
        mode = 1;
        pos = ((val&0x3f)<<8)+s[si++];
        size = s[si++]+4;
        pos = (pos+size);
      }
    }

    if(di+size>dsize)
      return sFALSE;

    switch(mode)
    {
    case 1: // copy dest
      if(verify)
      {
        if(sCmpMem(d+di,d+di-pos,size)!=0)
          return sFALSE;
      }
      else
      {
        sCopyMem(d+di,d+di-pos,size);
      }
      di+=size;
      break;
    case 2: // copy source
      if(verify)
      {
        if(sCmpMem(d+di,s+si,size)!=0)
          return sFALSE;
      }
      else
      {
        sCopyMem(d+di,s+si,size);
      }
      di+=size;
      si+=size;
      break;
    case 3: // fill
      if(verify)
      {
        for(i=0;i<size;i++)
          if(d[di+i]!=data)
            return sFALSE;
      }
      else
      {
        sSetMem(d+di,data,size);
      }
      di+=size;
      break;
    }
  }

  if(dsize==0)                    // return real decompressed size if not given
    dsize = di;

  return (si==ssize) && (di==dsize);  // return sFALSE if something is strange
}

/****************************************************************************/
/***                                                                      ***/
/***  Hooks                                                               ***/
/***                                                                      ***/
/****************************************************************************/

sHooksBase::sHooksBase(sInt max)
{
  Hooks = new sStaticArray<HookInfo>;
  Hooks->HintSize(max);
}

sHooksBase::~sHooksBase()
{
  if(this) delete Hooks;
}

void sHooksBase::_Add(void *h,void *user)
{
  if(this)
  {
    HookInfo nfo;
    nfo.Hook = h;
    nfo.User = user;
    Hooks->AddTail(nfo);
  }
}

void sHooksBase::_Rem(void *h,void *user)
{
  if(this)
  {
    HookInfo *hi;
    sFORALL(*Hooks,hi)
    {
      if(hi->Hook == h && (!user || hi->User == user))
      {
        Hooks->RemAtOrder(_i);
        break;
      }
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Arithmetic Functions                                               ***/
/***                                                                      ***/
/****************************************************************************/

#include <math.h>
#include <string.h>

#ifndef sHASINTRINSIC_SETMEM
void sSetMem(void *dd,sInt s,sInt c) { memset(dd,s,c); }
#endif

#ifndef sHASINTRINSIC_COPYMEM
void sCopyMem(void *dd,const void *ss,sInt c) { memcpy(dd,ss,c); }
#endif

void sMoveMem(void *dd,const void *ss,sInt c) { memmove(dd,ss,c); }

#ifndef sHASINTRINSIC_CMPMEM
sInt sCmpMem(const void *dd,const void *ss,sInt c) { return memcmp(dd,ss,c); }
#endif

// int

#ifndef sHASINTRINSIC_MULDIV
sInt sMulDiv(sInt a,sInt b,sInt c)  { return sS64(a)*b/c; }
#endif

#ifndef sHASINTRINSIC_MULDIVU
sU32 sMulDivU(sU32 a,sU32 b,sU32 c)  { return sU64(a)*b/c; }
#endif

#ifndef sHASINTRINSIC_MULSHIFT
sInt sMulShift(sInt a,sInt b)  { return (sS64(a)*b)>>16; }
#endif

#ifndef sHASINTRINSIC_MULSHIFTU
sU32 sMulShiftU(sU32 a,sU32 b)  { return (sU64(a)*b)>>16; }
#endif

#ifndef sHASINTRINSIC_DIVSHIFT
sInt sDivShift(sInt a,sInt b)  { return (sS64(a)<<16)/b; }
#endif

// float fiddling

#ifndef sHASINTRINSIC_ABSF
sF32 sAbs(sF32 f)               { return fabs(f); }
#endif

#ifndef sHASINTRINSIC_MINF
sF32 sMinF(sF32 a,sF32 b)       { return sMin(a,b); }
#endif

#ifndef sHASINTRINSIC_MAXF
sF32 sMaxF(sF32 a,sF32 b)       { return sMax(a,b); }
#endif

#ifndef sHASINTRINSIC_MOD
sF32 sMod(sF32 over,sF32 under) { return fmod(over,under); }
#endif

sInt sAbsMod(sInt over,sInt under) { return (over >= 0)?(over % under):(under-(-over % under)); }
sF32 sAbsMod(sF32 over,sF32 under) { return (over >= 0.0f)?sMod(over,under):(under-sMod(-over,under)); }

#ifndef sHASINTRINSIC_DIVMOD
void sDivMod(sF32 over,sF32 under,sF32 &div,sF32 &mod)  
{ 
  div = over/under; 
  mod = sMod(over,under); 
}
#endif

#ifndef sHASINTRINSIC_SETFLOATLP
void sSetFloat()        
{ 
#if !sCONFIG_OPTION_NPP
#if sPLATFORM==sPLAT_WINDOWS
#if sCONFIG_64BIT
#pragma warning(push)
#pragma warning(disable : 4996) 
  _clearfp();
  _controlfp(_RC_NEAR|_EM_INVALID|_EM_DENORMAL|_EM_ZERODIVIDE|_EM_OVERFLOW|_EM_UNDERFLOW|_EM_INEXACT,_MCW_RC|_MCW_EM);
#pragma warning(pop)
#else
  __asm
	{
    fclex;
		push		0103fh; // round to nearest even + single precision
		fldcw		[esp];
		pop			eax;
	}
#endif
#endif
#endif
}

#ifndef sHASINTRINSIC_FRAC_FF
void sFrac(sF32 val,sF32 &frac,sF32 &full)
{
  frac = modff(val,&full);
}
#endif

#ifndef sHASINTRINSIC_FRAC_FI
void sFrac(sF32 val,sF32 &frac,sInt &full)        
{
  sF32 f;
  frac = modff(val,&f);
  full = sInt(f);
}
#endif

#ifndef sHASINTRINSIC_FRAC
sF32 sFrac(sF32 val)
{ 
  sF32 f;
  return modff(val,&f);
}
#endif

#ifndef sHASINTRINSIC_ROUNDDOWN
sF32 sRoundDown(sF32 f)         { return floorf(f); }
#endif

#ifndef sHASINTRINSIC_ROUNDUP
sF32 sRoundUp(sF32 f)           { return ceilf(f); }
#endif

#ifndef sHASINTRINSIC_ROUNDZERO
sF32 sRoundZero(sF32 f)         { return (f>=0.0f)?sFFloor(f):sFCeil(f); }
#endif

#ifndef sHASINTRINSIC_ROUNDNEAR
sF32 sRoundNear(sF32 f)         { return floorf(f+0.5f); }
#endif

#ifndef sHASINTRINSIC_ROUNDDOWNI
sInt sRoundDownInt(sF32 f)      { return sInt(floorf(f)); }
#endif

#ifndef sHASINTRINSIC_ROUNDUPI
sInt sRoundUpInt(sF32 f)        { return sInt(ceilf(f)); }
#endif

#ifndef sHASINTRINSIC_ROUNDZEROI
sInt sRoundZeroInt(sF32 f)      { return sInt(f); }
#endif

#ifndef sHASINTRINSIC_ROUNDNEARI
sInt sRoundNearInt(sF32 f)      { return sInt(floorf(f+0.5f)); }
#endif

#ifndef sHASINTRINSIC_SELECT
#ifndef sHASINTRINSIC_SELECTEQ
sF32 sSelectEQ(sF32 a,sF32 b,sF32 t,sF32 f)   { return (a==b) ? t : f; }
#endif

#ifndef sHASINTRINSIC_SELECTNE
sF32 sSelectNE(sF32 a,sF32 b,sF32 t,sF32 f)   { return (a!=b) ? t : f; }
#endif

#ifndef sHASINTRINSIC_SELECTGT
sF32 sSelectGT(sF32 a,sF32 b,sF32 t,sF32 f)   { return (a>=b) ? t : f; }
#endif

#ifndef sHASINTRINSIC_SELECTGE
sF32 sSelectGE(sF32 a,sF32 b,sF32 t,sF32 f)   { return (a> b) ? t : f; }
#endif

#ifndef sHASINTRINSIC_SELECTLT
sF32 sSelectLT(sF32 a,sF32 b,sF32 t,sF32 f)   { return (a<=b) ? t : f; }
#endif

#ifndef sHASINTRINSIC_SELECTLE
sF32 sSelectLE(sF32 a,sF32 b,sF32 t,sF32 f)   { return (a< b) ? t : f; }
#endif
#endif

#ifndef sHASINTRINSIC_SELECT0
#ifndef sHASINTRINSIC_SELECTEQ0
sF32 sSelectEQ(sF32 a,sF32 b,sF32 t)          { return (a==b) ? t : 0; }
#endif

#ifndef sHASINTRINSIC_SELECTNE0
sF32 sSelectNE(sF32 a,sF32 b,sF32 t)          { return (a!=b) ? t : 0; }
#endif

#ifndef sHASINTRINSIC_SELECTGT0
sF32 sSelectGT(sF32 a,sF32 b,sF32 t)          { return (a>=b) ? t : 0; }
#endif

#ifndef sHASINTRINSIC_SELECTGE0
sF32 sSelectGE(sF32 a,sF32 b,sF32 t)          { return (a> b) ? t : 0; }
#endif

#ifndef sHASINTRINSIC_SELECTLT0
sF32 sSelectLT(sF32 a,sF32 b,sF32 t)          { return (a<=b) ? t : 0; }
#endif

#ifndef sHASINTRINSIC_SELECTLE0
sF32 sSelectLE(sF32 a,sF32 b,sF32 t)          { return (a< b) ? t : 0; }
#endif
#endif

// non-trivial float

#ifndef sHASINTRINSIC_SQRT
sF32 sSqrt(sF32 f)        { return sqrtf(f); }
#endif

#ifndef sHASINTRINSIC_RSQRT
sF32 sRSqrt(sF32 f)       { return 1.0f/sqrtf(f); }
#endif

#ifndef sHASINTRINSIC_LOG
sF32 sLog(sF32 f)         { return logf(f); }
#endif

#ifndef sHASINTRINSIC_LOG2
sF32 sLog2(sF32 f)        { return logf(f)/0.69314718055994530941723212145818f; }
#endif

#ifndef sHASINTRINSIC_LOG10
sF32 sLog10(sF32 f)       { return log10f(f); }
#endif

#ifndef sHASINTRINSIC_EXP
sF32 sExp(sF32 f)         { return expf(f); }
#endif

#ifndef sHASINTRINSIC_POW
sF32 sPow(sF32 a,sF32 b)  { return powf(a,b); }
#endif


#ifndef sHASINTRINSIC_SIN
sF32 sSin(sF32 f)         { return sinf(f); }

#endif

#ifndef sHASINTRINSIC_COS
sF32 sCos(sF32 f)         { return cosf(f); }
#endif

#ifndef sHASINTRINSIC_TAN
sF32 sTan(sF32 f)         { return tanf(f); }
#endif

#ifndef sHASINTRINSIC_SINCOS
void sSinCos(sF32 f,sF32 &s,sF32 &c)  { s=sinf(f); c=cosf(f); }
#endif

#ifndef sHASINTRINSIC_ASIN
sF32 sASin(sF32 f)        { return asinf(f); }
#endif

#ifndef sHASINTRINSIC_ACOS
sF32 sACos(sF32 f)        { return acosf(f); }
#endif

#ifndef sHASINTRINSIC_ATAN
sF32 sATan(sF32 f)        { return (sF32) atan(f); }
#endif

#ifndef sHASINTRINSIC_ATAN2
sF32 sATan2(sF32 a,sF32 b){ return (sF32) atan2(a,b); }
#endif

// non-trivial float, fast

#ifndef sHASINTRINSIC_FSQRT
sF32 sFSqrt(sF32 f)       { return sqrtf(f); }
#endif

#ifndef sHASINTRINSIC_FRSQRT
sF32 sFRSqrt(sF32 f)      { return 1.0f/sqrtf(f); }
#endif

#ifndef sHASINTRINSIC_FLOG
sF32 sFLog(sF32 f)        { return logf(f); }
#endif

#ifndef sHASINTRINSIC_FLOG2
sF32 sFLog2(sF32 f)       { return logf(f)/0.69314718055994530941723212145818f; }
#endif

#ifndef sHASINTRINSIC_FLOG10
sF32 sFLog10(sF32 f)      { return log10f(f); }
#endif

#ifndef sHASINTRINSIC_FEXP
sF32 sFExp(sF32 f)        { return expf(f); }
#endif

#ifndef sHASINTRINSIC_FPOW
sF32 sFPow(sF32 a,sF32 b) { return powf(a,b); }
#endif


#ifndef sHASINTRINSIC_FSIN
sF32 sFSin(sF32 f)        { return sinf(f); }
#endif

#ifndef sHASINTRINSIC_FCOS
sF32 sFCos(sF32 f)        { return cosf(f); }
#endif

#ifndef sHASINTRINSIC_FTAN
sF32 sFTan(sF32 f)        { return tanf(f); }
#endif

#ifndef sHASINTRINSIC_FSINCOS
void sFSinCos(sF32 f,sF32 &s,sF32 &c)  { s=sinf(f); c=cosf(f); }
#endif

#ifndef sHASINTRINSIC_FASIN
sF32 sFASin(sF32 f)       { return asinf(f); }
#endif

#ifndef sHASINTRINSIC_FACOS
sF32 sFACos(sF32 f)       { return acosf(f); }
#endif

#ifndef sHASINTRINSIC_FATAN
sF32 sFATan(sF32 f)       { return atanf(f); }
#endif

#ifndef sHASINTRINSIC_FATAN2
sF32 sFATan2(sF32 a,sF32 b){ return atan2f(a,b); }
#endif
#endif
/****************************************************************************/
