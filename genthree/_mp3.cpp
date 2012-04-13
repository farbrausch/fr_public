// This file is distributed under a BSD license. See LICENSE.txt for details.
#include "_mp3.hpp"

#if sLINK_MP3

#include <math.h>
#include <string.h>
/*
#include "ogg/os_types.h"
#include "ogg/ogg.h"
#include "ogg/codec.h"
*/
/****************************************************************************/
/***                                                                      ***/
/***   ampdec.h                                                           ***/
/***                                                                      ***/
/****************************************************************************/

class ampegdecoder 
{
  unsigned char *InBuffer;
  int InSize;
  int InIndex;

public:
// bitstream
  unsigned char *bitbuf;
  int *bitbufpos;

  int mpgetbit()
  {
    int val=(bitbuf[*bitbufpos>>3]>>(7-(*bitbufpos&7)))&1;
    (*bitbufpos)++;
    return val;
  }
  unsigned int convbe32(unsigned int x)
  {
  #ifdef BIGENDIAN
    return x;
  #elif defined(__WATCOMC__)||defined(GNUCI486)
    return __swap32(x);
  #else
    return ((x<<24)&0xFF000000)|((x<<8)&0xFF0000)|((x>>8)&0xFF00)|((x>>24)&0xFF);
  #endif
  }
  long mpgetbits(int n)
  {
    if (!n)
      return 0;
#ifdef FASTBITS
    unsigned long val=(convbe32(*(unsigned long*)&bitbuf[*bitbufpos>>3])>>(32-(*bitbufpos&7)-n))&((1<<n)-1);
#else
    unsigned long val=(((((unsigned char)(bitbuf[(*bitbufpos>>3)+0]))<<24)|(((unsigned char)(bitbuf[(*bitbufpos>>3)+1]))<<16)|(((unsigned char)(bitbuf[(*bitbufpos>>3)+2]))<<8))>>(32-(*bitbufpos&7)-n))&((1<<n)-1);
#endif
    *bitbufpos+=n;
    return val;
  }
  void getbytes(void *buf2, int n);

// mainbitstream
  enum
  {
    mainbufmin=2048,
    mainbufmax=16384
  };

  unsigned char *mainbuf;
  int mainbufsize;
  int mainbuflow;
  int mainbuflen;
  int mainbufpos;

  int sync7FF();

// decoder
  static int lsftab[4];
  static int freqtab[4];
  static int ratetab[2][3][16];

  int hdrlay;
  int hdrcrc;
  int hdrbitrate;
  int hdrfreq;
  int hdrpadding;
  int hdrmode;
  int hdrmodeext;
  int hdrlsf;

  int init;
  int orglay;
  int orgfreq;
  int orglsf;
  int orgstereo;

  int stream;
  int slotsize;
  int nslots;
  int fslots;
  int slotdiv;
  int seekinitframes;
  char framebuf[2*32*36*4];
  int curframe;
  int framepos;
  int nframes;
  int framesize;
  int atend;
  float fraction[2][36][32];

  int decodehdr(int);
  int decode(void *);

// synth
  static float dwin[1024];
  static float dwin2[512];
  static float dwin4[256];
  static float sectab[32];

  int synbufoffset;
  float synbuf[2048];

  inline float muladd16a(float *a, float *b);
  inline float muladd16b(float *a, float *b);

  int convle16(int x)
  {
  #ifdef BIGENDIAN
    return ((x>>8)&0xFF)|((x<<8)&0xFF00);
  #else
    return x;
  #endif
  }
  int cliptoshort(float x)
  {
  #ifdef __WATCOMC__
    long foo;
    __fistp(foo,x);
  #else
    int foo=(int)x;
  #endif
    return (foo<-32768)?convle16(-32768):(foo>32767)?convle16(32767):convle16(foo);
  }
  static void fdctb32(float *out1, float *out2, float *in);
  static void fdctb16(float *out1, float *out2, float *in);
  static void fdctb8(float *out1, float *out2, float *in);

  int tomono;
  int tostereo;
  int dstchan;
  int ratereduce;
  int usevoltab;
  int srcchan;
  int dctstereo;
  int samplesize;
  float stereotab[3][3];
  float equal[32];
  int equalon;
  float volume;

  int opensynth();
  void synth(void *, float (*)[32], float (*)[32]);
  void resetsynth();

  void setstereo(const float *);
  void setvol(float);
  void setequal(const float *);

//layer 1/2
  struct sballoc
  {
    unsigned int steps;
    unsigned int bits;
    int scaleadd;
    float scale;
  };

  static sballoc alloc[17];
  static sballoc *atab0[];
  static sballoc *atab1[];
  static sballoc *atab2[];
  static sballoc *atab3[];
  static sballoc *atab4[];
  static sballoc *atab5[];
  static const int alloctablens[5];
  static sballoc **alloctabs[3][32];
  static const int alloctabbits[3][32];
  static float multiple[64];
  static float rangefac[16];

  float scale1[2][32];
  unsigned int bitalloc1[2][32];
  float scale2[2][3][32];
  int scfsi[2][32];
  sballoc *bitalloc2[2][32];

  static void init12();
  void openlayer1(int);
  void decode1();
  void openlayer2(int);
  void decode2();
//  void decode2mc();

//layer 3
  struct grsistruct
  {
    int gr;
    int ch;

    int blocktype;
    int mixedblock;

    int grstart;
    int tabsel[4];
    int regionend[4];
    int grend;

    int subblockgain[3];
    int preflag;
    int sfshift;
    int globalgain;

    int sfcompress;
    int sfsi[4];

    int ktabsel;
  };

  static int htab00[],htab01[],htab02[],htab03[],htab04[],htab05[],htab06[],htab07[];
  static int htab08[],htab09[],htab10[],htab11[],htab12[],htab13[],htab14[],htab15[];
  static int htab16[],htab24[];
  static int htaba[],htabb[];
  static int *htabs[34];
  static int htablinbits[34];
  static int sfbtab[7][3][5];
  static int slentab[2][16];
  static int sfbands[3][3][14];
  static int sfbandl[3][3][23];
  static float citab[8];
  static float csatab[8][2];
  static float sqrt05;
  static float ktab[3][32][2];
  static float sec12[3];
  static float sec24wins[6];
  static float cos6[3];
  static float sec36[9];
  static float sec72winl[18];
  static float cos18[9];
  static float winsqs[3];
  static float winlql[9];
  static float winsql[12];
  static int pretab[22];
  static float pow2tab[65];
  static float pow43tab[8207];
  static float ggaintab[256];

  int rotab[3][576];
  float l3equall[576];
  float l3equals[192];
  int l3equalon;

  float prevblck[2][32][18];
  unsigned char huffbuf[4096];
  int huffbit;
  int huffoffset;

  int ispos[576];
  int scalefac0[2][39];
  float xr0[2][576];

  static void init3();
  inline int huffmandecoder(int *valt);
  static void imdct(float *out, float *in, float *prev, int blocktype);
  static void fdctd6(float *out, float *in);
  static void fdctd18(float *out, float *in);
  void readgrsi(grsistruct &si, int &bitpos);
  void jointstereo(grsistruct &si, float (*xr)[576], int *scalefacl);
  void readhuffman(grsistruct &si, float *xr);
  void doscale(grsistruct &si, float *xr, int *scalefacl);
  void readscalefac(grsistruct &si, int *scalefacl);
  void hybrid(grsistruct &si, float (*hout)[32], float (*prev)[18], float *xr);
  void readsfsi(grsistruct &si);
  void readmain(grsistruct (*si)[2]);

  void openlayer3(int);
  void decode3();
  void seekinit3(int);
  void setl3equal(const float *);

  int getheader(int &layer, int &lsf, int &freq, int &stereo, int &rate);

  int Start(unsigned char *buffer,int size);
  int DecodeFrame(void *out);
};

/****************************************************************************/
/***                                                                      ***/
/***   amp1dec.cpp                                                        ***/
/***                                                                      ***/
/****************************************************************************/


void ampegdecoder::openlayer1(int rate)
{
  if (rate)
  {
    slotsize=4;
    slotdiv=freqtab[orgfreq]>>orglsf;
    nslots=(36*rate)/(freqtab[orgfreq]>>orglsf);
    fslots=(36*rate)%slotdiv;
    seekinitframes=1;
  }
}

void ampegdecoder::decode1()
{
  int i,j,q,fr;
  for (fr=0; fr<3; fr++)
  {
    if (fr)
      decodehdr(0);
    if (!hdrbitrate)
    {
      for (q=0; q<12; q++)
        for (j=0; j<2; j++)
          for (i=0; i<32; i++)
            fraction[j][12*fr+q][i]=0;
      continue;
    }

    int bitend=mainbufpos-32-(hdrcrc?16:0)+(hdrpadding?32:0)+12*1000*ratetab[hdrlsf?1:0][0][hdrbitrate]/(freqtab[hdrfreq]>>hdrlsf)*32;

    int jsbound=(hdrmode==1)?(hdrmodeext+1)*4:(hdrmode==3)?0:32;
    int stereo=(hdrmode==3)?1:2;

    for (i=0;i<32;i++)
      for (j=0;j<((i<jsbound)?2:1);j++)
      {
        bitalloc1[j][i] = mpgetbits(4);
        if (i>=jsbound)
          bitalloc1[1][i] = bitalloc1[0][i];
      }

    for (i=0;i<32;i++)
      for (j=0;j<stereo;j++)
        if (bitalloc1[j][i])
          scale1[j][i]=multiple[mpgetbits(6)]*rangefac[bitalloc1[j][i]];

    for (q=0;q<12;q++)
      for (i=0;i<32;i++)
        for (j=0;j<((i<jsbound)?2:1);j++)
          if (bitalloc1[j][i])
          {
            int s=mpgetbits(bitalloc1[j][i]+1)-(1<<bitalloc1[j][i])+1;
            fraction[j][12*fr+q][i]=scale1[j][i]*s;
            if (i>=jsbound)
              fraction[1][12*fr+q][i]=scale1[1][i]*s;
          }
          else
          {
            fraction[j][12*fr+q][i]=0;
            if (i>=jsbound)
              fraction[1][12*fr+q][i]=0;
          }

    mpgetbits(bitend-mainbufpos);
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   amp2dec.cpp                                                        ***/
/***                                                                      ***/
/****************************************************************************/


ampegdecoder::sballoc ampegdecoder::alloc[17];

ampegdecoder::sballoc *ampegdecoder::atab0[]=
{
  0        , &alloc[0], &alloc[3], &alloc[4], &alloc[5], &alloc[6], &alloc[7], &alloc[8],
  &alloc[9], &alloc[10],&alloc[11],&alloc[12],&alloc[13],&alloc[14],&alloc[15],&alloc[16]
};

ampegdecoder::sballoc *ampegdecoder::atab1[]=
{
  0        , &alloc[0], &alloc[1], &alloc[3], &alloc[2], &alloc[4], &alloc[5], &alloc[6],
  &alloc[7], &alloc[8], &alloc[9], &alloc[10],&alloc[11],&alloc[12],&alloc[13],&alloc[16]
};

ampegdecoder::sballoc *ampegdecoder::atab2[]=
{
  0        , &alloc[0], &alloc[1], &alloc[3], &alloc[2], &alloc[4], &alloc[5], &alloc[16]
};

ampegdecoder::sballoc *ampegdecoder::atab3[]=
{
  0        , &alloc[0], &alloc[1], &alloc[16]
};

ampegdecoder::sballoc *ampegdecoder::atab4[]=
{
  0        , &alloc[0], &alloc[1], &alloc[2], &alloc[4], &alloc[5], &alloc[6], &alloc[7],
  &alloc[8], &alloc[9], &alloc[10],&alloc[11],&alloc[12],&alloc[13],&alloc[14],&alloc[15]
};

ampegdecoder::sballoc *ampegdecoder::atab5[]=
{
  0        , &alloc[0], &alloc[1], &alloc[3], &alloc[2], &alloc[4], &alloc[5], &alloc[6],
  &alloc[7], &alloc[8], &alloc[9], &alloc[10],&alloc[11],&alloc[12],&alloc[13],&alloc[14]
};

const int ampegdecoder::alloctablens[5]={27,30,8,12,30};

ampegdecoder::sballoc **ampegdecoder::alloctabs[3][32]=
{
  {
    atab0, atab0, atab0, atab1, atab1, atab1, atab1, atab1, atab1, atab1,
    atab1, atab2, atab2, atab2, atab2, atab2, atab2, atab2, atab2, atab2,
    atab2, atab2, atab2, atab3, atab3, atab3, atab3, atab3, atab3, atab3
  },
  {
    atab4, atab4, atab4, atab4, atab4, atab4, atab4, atab4, atab4, atab4,
    atab4, atab4
  },
  {
    atab5, atab5, atab5, atab5, atab4, atab4, atab4, atab4, atab4, atab4,
    atab4, atab4, atab4, atab4, atab4, atab4, atab4, atab4, atab4, atab4,
    atab4, atab4, atab4, atab4, atab4, atab4, atab4, atab4, atab4, atab4
  }
};

const int ampegdecoder::alloctabbits[3][32]=
{
  {4,4,4,4,4,4,4,4,4,4,4,3,3,3,3,3,3,3,3,3,3,3,3,2,2,2,2,2,2,2},
  {4,4,3,3,3,3,3,3,3,3,3,3},
  {4,4,4,4,3,3,3,3,3,3,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}
};

float ampegdecoder::multiple[64];
float ampegdecoder::rangefac[16];

void ampegdecoder::init12()
{
  int i;
  for (i=0; i<63; i++)
    multiple[i]=exp(log(2.0f)*(3-i)/3.0);
  multiple[63]=0;
  for (i=0; i<16; i++)
    rangefac[i]=2.0/((2<<i)-1);
  for (i=0; i<3; i++)
  {
    alloc[i].scaleadd=1<<i;
    alloc[i].steps=2*alloc[i].scaleadd+1;
    alloc[i].bits=(10+5*i)>>1;
    alloc[i].scale=2.0/(2*alloc[i].scaleadd+1);
  }
  for (i=3; i<17; i++)
  {
    alloc[i].steps=0;
    alloc[i].bits=i;
    alloc[i].scaleadd=(1<<(i-1))-1;
    alloc[i].scale=2.0/(2*alloc[i].scaleadd+1);
  }
}

void ampegdecoder::openlayer2(int rate)
{
  if (rate)
  {
    slotsize=1;
    slotdiv=freqtab[orgfreq]>>orglsf;
    nslots=(144*rate)/(freqtab[orgfreq]>>orglsf);
    fslots=(144*rate)%slotdiv;
    seekinitframes=1;
  }
}

void ampegdecoder::decode2()
{
  int i,j,k,q;

  if (!hdrbitrate)
  {
    for (q=0; q<36; q++)
      for (j=0; j<2; j++)
        for (i=0; i<32; i++)
          fraction[j][q][i]=0;
    return;
  }

  int bitend=mainbufpos-32-(hdrcrc?16:0)+144*1000*ratetab[hdrlsf?1:0][1][hdrbitrate]/(freqtab[hdrfreq]>>hdrlsf)*8+(hdrpadding?8:0);

  int stereo = (hdrmode==3)?1:2;
  int brpch = ratetab[0][1][hdrbitrate] / stereo;

  int tabnum;
  if (hdrlsf)
    tabnum=4;
  else
  if (((hdrfreq==1)&&(brpch>=56))||((brpch>=56)&&(brpch<=80)))
    tabnum=0;
  else
  if ((hdrfreq!=1)&&(brpch>=96))
    tabnum=1;
  else
  if ((hdrfreq!=2)&&(brpch<=48))
    tabnum=2;
  else
    tabnum=3;
  sballoc ***alloc=alloctabs[tabnum>>1];
  const int *allocbits=alloctabbits[tabnum>>1];
  int sblimit=alloctablens[tabnum];
  int jsbound;
  if (hdrmode==1)
    jsbound=(hdrmodeext+1)*4;
  else
  if (hdrmode==3)
    jsbound=0;
  else
    jsbound=sblimit;

  for (i=0; i<sblimit; i++)
    for (j=0; j<((i<jsbound)?2:1); j++)
    {
      bitalloc2[j][i]=alloc[i][mpgetbits(allocbits[i])];
      if (i>=jsbound)
        bitalloc2[1][i]=bitalloc2[0][i];
    }

  for (i=0; i<sblimit; i++)
    for (j=0; j<stereo; j++)
      if (bitalloc2[j][i])
        scfsi[j][i]=mpgetbits(2);

  for (i=0;i<sblimit;i++)
    for (j=0;j<stereo;j++)
      if (bitalloc2[j][i])
      {
        int si[3];
        switch (scfsi[j][i])
        {
        case 0:
          si[0]=mpgetbits(6);
          si[1]=mpgetbits(6);
          si[2]=mpgetbits(6);
          break;
        case 1:
          si[0]=si[1]=mpgetbits(6);
          si[2]=mpgetbits(6);
          break;
        case 2:
          si[0]=si[1]=si[2]=mpgetbits(6);
          break;
        case 3:
          si[0]=mpgetbits(6);
          si[1]=si[2]=mpgetbits(6);
          break;
        }
        for (k=0; k<3; k++)
          scale2[j][k][i]=multiple[si[k]]*bitalloc2[j][i]->scale;
      }

  for (q=0;q<12;q++)
  {
    for (i=0; i<sblimit; i++)
      for (j=0; j<((i<jsbound)?2:1); j++)
        if (bitalloc2[j][i])
        {
          int s[3];
          if (!bitalloc2[j][i]->steps)
            for (k=0; k<3; k++)
              s[k]=mpgetbits(bitalloc2[j][i]->bits)-bitalloc2[j][i]->scaleadd;
          else
          {
            int nlevels=bitalloc2[j][i]->steps;
            int c=mpgetbits(bitalloc2[j][i]->bits);
            for (k=0; k<3; k++)
            {
              s[k]=(c%nlevels)-bitalloc2[j][i]->scaleadd;
              c/=nlevels;
            }
          }

          for (k=0; k<3; k++)
            fraction[j][q*3+k][i]=s[k]*scale2[j][q>>2][i];
          if (i>=jsbound)
            for (k=0;k<3;k++)
              fraction[1][q*3+k][i]=s[k]*scale2[1][q>>2][i];
        }
        else
        {
          for (k=0;k<3;k++)
            fraction[j][q*3+k][i]=0;
          if (i>=jsbound)
            for (k=0;k<3;k++)
              fraction[1][q*3+k][i]=0;
        }

    for (i=sblimit; i<32; i++)
      for (k=0; k<3; k++)
        for (j=0; j<stereo; j++)
          fraction[j][q*3+k][i]=0;
  }
//  if ((bitend-mainbufpos)>=16)
//    decode2mc();
  mpgetbits(bitend-mainbufpos);
}

/****************************************************************************/
/***                                                                      ***/
/***   amp3dec.cpp                                                        ***/
/***                                                                      ***/
/****************************************************************************/

#define _PI 3.14159265358979323846
//#define _PI 3.14159265358979

#define HUFFMAXLONG 8
//#define HUFFMAXLONG (hdrlsf?6:8)

// some remarks
//
// huffman with blocktypes!=0:
// there are 2 regions for the huffman decoder. in version 1 the limit
// is always 36. in version 2 it seems to be 54 for blocktypes 1 and 3
// and 36 for blocktype 2. why those 54? i cannot understand that.
// the streams i found decode fine with HUFFMAXLONG==8, but not with
// the limit of 36 in all cases (HUFFMAXLONG==version?6:8).
// i suspect this might be a bug in L3ENC by Fraunhofer IIS.
// i have no proof, just a feeling...
//
// mixed blocks:
// in version 1 there are 8 long bands and 9*3 short bands. these values
// seem to be unchanged in version 2 in all the sources i could find.
// this means there are 35 bands in total and that does not match the value
// 33 of sfbtab, which means that there are 2 bands without a scalefactor.
// furthermore there would be bands with double scalefactors, because the
// long bands stop at index 54 and the short bands begin at 36.
// this is nonsense.
// therefore i changed the number of long bands to 6 in version 2, so that
// the limit is 36 for both and there are 35 bands in total. this is the only
// solution with all problems solved.
// i have no streams to test this, but i am very sure about this.
//
// huffman decoder:
// the size of one granule is given in the sideinfo, just why isn't it
// always exact???!!!

//  nbeisert


float ampegdecoder::csatab[8][2];

float ampegdecoder::ggaintab[256];
float ampegdecoder::ktab[3][32][2];
float ampegdecoder::pow2tab[65];
float ampegdecoder::pow43tab[8207];

float ampegdecoder::sec12[3];
float ampegdecoder::sec24wins[6];
float ampegdecoder::cos6[3];
float ampegdecoder::sec72winl[18];
float ampegdecoder::sec36[9];
float ampegdecoder::cos18[9];
float ampegdecoder::winsqs[3];
float ampegdecoder::winsql[12];
float ampegdecoder::winlql[9];
float ampegdecoder::sqrt05;

int ampegdecoder::sfbands[3][3][14] =
{
  {
    {  0, 12, 24, 36, 48, 66, 90,120,156,198,252,318,408,576},
    {  0, 12, 24, 36, 48, 66, 84,114,150,192,240,300,378,576},
    {  0, 12, 24, 36, 48, 66, 90,126,174,234,312,414,540,576}
  },
  {
    {  0, 12, 24, 36, 54, 72, 96,126,168,222,300,396,522,576},
    {  0, 12, 24, 36, 54, 78,108,144,186,240,312,408,540,576},
    {  0, 12, 24, 36, 54, 78,108,144,186,240,312,402,522,576}
  },
  {
    {  0, 12, 24, 36, 54, 78,108,144,186,240,312,402,522,576},
    {  0, 12, 24, 36, 54, 78,108,144,186,240,312,402,522,576},
    {  0, 24, 48, 72,108,156,216,288,372,480,486,492,498,576}
  }
};


int ampegdecoder::sfbandl[3][3][23] =
{
  {
    {  0,  4,  8, 12, 16, 20, 24, 30, 36, 44, 52, 62, 74, 90,110,134,162,196,238,288,342,418,576},
    {  0,  4,  8, 12, 16, 20, 24, 30, 36, 42, 50, 60, 72, 88,106,128,156,190,230,276,330,384,576},
    {  0,  4,  8, 12, 16, 20, 24, 30, 36, 44, 54, 66, 82,102,126,156,194,240,296,364,448,550,576}
  },
  {
    {  0,  6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96,116,140,168,200,238,284,336,396,464,522,576},
    {  0,  6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96,114,136,162,194,232,278,332,394,464,540,576},
    {  0,  6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96,116,140,168,200,238,284,336,396,464,522,576}
  },
  {
    {  0,  6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96,116,140,168,200,238,284,336,396,464,522,576},
    {  0,  6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96,116,140,168,200,238,284,336,396,464,522,576},
    {  0, 12, 24, 36, 48, 60, 72, 88,108,132,160,192,232,280,336,400,476,566,568,570,572,574,576}
  }
};

int ampegdecoder::pretab[22]={0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,2,2,3,3,3,2,0};
float ampegdecoder::citab[8]={-0.6f,-0.535f,-0.33f,-0.185f,-0.095f,-0.041f,-0.0142f,-0.0037f};

int ampegdecoder::slentab[2][16] = {{0,0,0,0,3,1,1,1,2,2,2,3,3,3,4,4},
                                      {0,1,2,3,0,1,2,3,1,2,3,1,2,3,2,3}};

int ampegdecoder::sfbtab[7][3][5]=
{
  {{ 0, 6,11,16,21}, { 0, 9,18,27,36}, { 0, 6,15,24,33}},
  {{ 0, 6,11,18,21}, { 0, 9,18,30,36}, { 0, 6,15,27,33}},
  {{ 0,11,21,21,21}, { 0,18,36,36,36}, { 0,15,33,33,33}},
  {{ 0, 7,14,21,21}, { 0,12,24,36,36}, { 0, 6,21,33,33}},
  {{ 0, 6,12,18,21}, { 0,12,21,30,36}, { 0, 6,18,27,33}},
  {{ 0, 8,16,21,21}, { 0,15,27,36,36}, { 0, 6,24,33,33}},
  {{ 0, 6,11,16,21}, { 0, 9,18,24,36}, { 0, 8,17,23,35}},
};

int ampegdecoder::htab00[] =
{
   0
};

int ampegdecoder::htab01[] =
{
  -5,  -3,  -1,  17,   1,  16,   0
};

int ampegdecoder::htab02[] =
{
 -15, -11,  -9,  -5,  -3,  -1,  34,   2,  18,  -1,  33,  32,  17,  -1,   1,
  16,   0
};

int ampegdecoder::htab03[] =
{
 -13, -11,  -9,  -5,  -3,  -1,  34,   2,  18,  -1,  33,  32,  16,  17,  -1,
   1,   0
};

int ampegdecoder::htab04[] =
{
   0
};

int ampegdecoder::htab05[] =
{
 -29, -25, -23, -15,  -7,  -5,  -3,  -1,  51,  35,  50,  49,  -3,  -1,  19,
   3,  -1,  48,  34,  -3,  -1,  18,  33,  -1,   2,  32,  17,  -1,   1,  16,
   0
};

int ampegdecoder::htab06[] =
{
 -25, -19, -13,  -9,  -5,  -3,  -1,  51,   3,  35,  -1,  50,  48,  -1,  19,
  49,  -3,  -1,  34,   2,  18,  -3,  -1,  33,  32,   1,  -1,  17,  -1,  16,
   0
};

int ampegdecoder::htab07[] =
{
 -69, -65, -57, -39, -29, -17, -11,  -7,  -3,  -1,  85,  69,  -1,  84,  83,
  -1,  53,  68,  -3,  -1,  37,  82,  21,  -5,  -1,  81,  -1,   5,  52,  -1,
  80,  -1,  67,  51,  -5,  -3,  -1,  36,  66,  20,  -1,  65,  64, -11,  -7,
  -3,  -1,   4,  35,  -1,  50,   3,  -1,  19,  49,  -3,  -1,  48,  34,  18,
  -5,  -1,  33,  -1,   2,  32,  17,  -1,   1,  16,   0
};

int ampegdecoder::htab08[] =
{
 -65, -63, -59, -45, -31, -19, -13,  -7,  -5,  -3,  -1,  85,  84,  69,  83,
  -3,  -1,  53,  68,  37,  -3,  -1,  82,   5,  21,  -5,  -1,  81,  -1,  52,
  67,  -3,  -1,  80,  51,  36,  -5,  -3,  -1,  66,  20,  65,  -3,  -1,   4,
  64,  -1,  35,  50,  -9,  -7,  -3,  -1,  19,  49,  -1,   3,  48,  34,  -1,
   2,  32,  -1,  18,  33,  17,  -3,  -1,   1,  16,   0
};

int ampegdecoder::htab09[] =
{
 -63, -53, -41, -29, -19, -11,  -5,  -3,  -1,  85,  69,  53,  -1,  83,  -1,
  84,   5,  -3,  -1,  68,  37,  -1,  82,  21,  -3,  -1,  81,  52,  -1,  67,
  -1,  80,   4,  -7,  -3,  -1,  36,  66,  -1,  51,  64,  -1,  20,  65,  -5,
  -3,  -1,  35,  50,  19,  -1,  49,  -1,   3,  48,  -5,  -3,  -1,  34,   2,
  18,  -1,  33,  32,  -3,  -1,  17,   1,  -1,  16,   0
};

int ampegdecoder::htab10[] =
{
-125,-121,-111, -83, -55, -35, -21, -13,  -7,  -3,  -1, 119, 103,  -1, 118,
  87,  -3,  -1, 117, 102,  71,  -3,  -1, 116,  86,  -1, 101,  55,  -9,  -3,
  -1, 115,  70,  -3,  -1,  85,  84,  99,  -1,  39, 114, -11,  -5,  -3,  -1,
 100,   7, 112,  -1,  98,  -1,  69,  53,  -5,  -1,   6,  -1,  83,  68,  23,
 -17,  -5,  -1, 113,  -1,  54,  38,  -5,  -3,  -1,  37,  82,  21,  -1,  81,
  -1,  52,  67,  -3,  -1,  22,  97,  -1,  96,  -1,   5,  80, -19, -11,  -7,
  -3,  -1,  36,  66,  -1,  51,   4,  -1,  20,  65,  -3,  -1,  64,  35,  -1,
  50,   3,  -3,  -1,  19,  49,  -1,  48,  34,  -7,  -3,  -1,  18,  33,  -1,
   2,  32,  17,  -1,   1,  16,   0
};

int ampegdecoder::htab11[] =
{
-121,-113, -89, -59, -43, -27, -17,  -7,  -3,  -1, 119, 103,  -1, 118, 117,
  -3,  -1, 102,  71,  -1, 116,  -1,  87,  85,  -5,  -3,  -1,  86, 101,  55,
  -1, 115,  70,  -9,  -7,  -3,  -1,  69,  84,  -1,  53,  83,  39,  -1, 114,
  -1, 100,   7,  -5,  -1, 113,  -1,  23, 112,  -3,  -1,  54,  99,  -1,  96,
  -1,  68,  37, -13,  -7,  -5,  -3,  -1,  82,   5,  21,  98,  -3,  -1,  38,
   6,  22,  -5,  -1,  97,  -1,  81,  52,  -5,  -1,  80,  -1,  67,  51,  -1,
  36,  66, -15, -11,  -7,  -3,  -1,  20,  65,  -1,   4,  64,  -1,  35,  50,
  -1,  19,  49,  -5,  -3,  -1,   3,  48,  34,  33,  -5,  -1,  18,  -1,   2,
  32,  17,  -3,  -1,   1,  16,   0
};

int ampegdecoder::htab12[] =
{
-115, -99, -73, -45, -27, -17,  -9,  -5,  -3,  -1, 119, 103, 118,  -1,  87,
 117,  -3,  -1, 102,  71,  -1, 116, 101,  -3,  -1,  86,  55,  -3,  -1, 115,
  85,  39,  -7,  -3,  -1, 114,  70,  -1, 100,  23,  -5,  -1, 113,  -1,   7,
 112,  -1,  54,  99, -13,  -9,  -3,  -1,  69,  84,  -1,  68,  -1,   6,   5,
  -1,  38,  98,  -5,  -1,  97,  -1,  22,  96,  -3,  -1,  53,  83,  -1,  37,
  82, -17,  -7,  -3,  -1,  21,  81,  -1,  52,  67,  -5,  -3,  -1,  80,   4,
  36,  -1,  66,  20,  -3,  -1,  51,  65,  -1,  35,  50, -11,  -7,  -5,  -3,
  -1,  64,   3,  48,  19,  -1,  49,  34,  -1,  18,  33,  -7,  -5,  -3,  -1,
   2,  32,   0,  17,  -1,   1,  16
};

int ampegdecoder::htab13[] =
{
-509,-503,-475,-405,-333,-265,-205,-153,-115, -83, -53, -35, -21, -13,  -9,
  -7,  -5,  -3,  -1, 254, 252, 253, 237, 255,  -1, 239, 223,  -3,  -1, 238,
 207,  -1, 222, 191,  -9,  -3,  -1, 251, 206,  -1, 220,  -1, 175, 233,  -1,
 236, 221,  -9,  -5,  -3,  -1, 250, 205, 190,  -1, 235, 159,  -3,  -1, 249,
 234,  -1, 189, 219, -17,  -9,  -3,  -1, 143, 248,  -1, 204,  -1, 174, 158,
  -5,  -1, 142,  -1, 127, 126, 247,  -5,  -1, 218,  -1, 173, 188,  -3,  -1,
 203, 246, 111, -15,  -7,  -3,  -1, 232,  95,  -1, 157, 217,  -3,  -1, 245,
 231,  -1, 172, 187,  -9,  -3,  -1,  79, 244,  -3,  -1, 202, 230, 243,  -1,
  63,  -1, 141, 216, -21,  -9,  -3,  -1,  47, 242,  -3,  -1, 110, 156,  15,
  -5,  -3,  -1, 201,  94, 171,  -3,  -1, 125, 215,  78, -11,  -5,  -3,  -1,
 200, 214,  62,  -1, 185,  -1, 155, 170,  -1,  31, 241, -23, -13,  -5,  -1,
 240,  -1, 186, 229,  -3,  -1, 228, 140,  -1, 109, 227,  -5,  -1, 226,  -1,
  46,  14,  -1,  30, 225, -15,  -7,  -3,  -1, 224,  93,  -1, 213, 124,  -3,
  -1, 199,  77,  -1, 139, 184,  -7,  -3,  -1, 212, 154,  -1, 169, 108,  -1,
 198,  61, -37, -21,  -9,  -5,  -3,  -1, 211, 123,  45,  -1, 210,  29,  -5,
  -1, 183,  -1,  92, 197,  -3,  -1, 153, 122, 195,  -7,  -5,  -3,  -1, 167,
 151,  75, 209,  -3,  -1,  13, 208,  -1, 138, 168, -11,  -7,  -3,  -1,  76,
 196,  -1, 107, 182,  -1,  60,  44,  -3,  -1, 194,  91,  -3,  -1, 181, 137,
  28, -43, -23, -11,  -5,  -1, 193,  -1, 152,  12,  -1, 192,  -1, 180, 106,
  -5,  -3,  -1, 166, 121,  59,  -1, 179,  -1, 136,  90, -11,  -5,  -1,  43,
  -1, 165, 105,  -1, 164,  -1, 120, 135,  -5,  -1, 148,  -1, 119, 118, 178,
 -11,  -3,  -1,  27, 177,  -3,  -1,  11, 176,  -1, 150,  74,  -7,  -3,  -1,
  58, 163,  -1,  89, 149,  -1,  42, 162, -47, -23,  -9,  -3,  -1,  26, 161,
  -3,  -1,  10, 104, 160,  -5,  -3,  -1, 134,  73, 147,  -3,  -1,  57,  88,
  -1, 133, 103,  -9,  -3,  -1,  41, 146,  -3,  -1,  87, 117,  56,  -5,  -1,
 131,  -1, 102,  71,  -3,  -1, 116,  86,  -1, 101, 115, -11,  -3,  -1,  25,
 145,  -3,  -1,   9, 144,  -1,  72, 132,  -7,  -5,  -1, 114,  -1,  70, 100,
  40,  -1, 130,  24, -41, -27, -11,  -5,  -3,  -1,  55,  39,  23,  -1, 113,
  -1,  85,   7,  -7,  -3,  -1, 112,  54,  -1,  99,  69,  -3,  -1,  84,  38,
  -1,  98,  53,  -5,  -1, 129,  -1,   8, 128,  -3,  -1,  22,  97,  -1,   6,
  96, -13,  -9,  -5,  -3,  -1,  83,  68,  37,  -1,  82,   5,  -1,  21,  81,
  -7,  -3,  -1,  52,  67,  -1,  80,  36,  -3,  -1,  66,  51,  20, -19, -11,
  -5,  -1,  65,  -1,   4,  64,  -3,  -1,  35,  50,  19,  -3,  -1,  49,   3,
  -1,  48,  34,  -3,  -1,  18,  33,  -1,   2,  32,  -3,  -1,  17,   1,  16,
   0
};

int ampegdecoder::htab14[] =
{
   0
};

int ampegdecoder::htab15[] =
{
-495,-445,-355,-263,-183,-115, -77, -43, -27, -13,  -7,  -3,  -1, 255, 239,
  -1, 254, 223,  -1, 238,  -1, 253, 207,  -7,  -3,  -1, 252, 222,  -1, 237,
 191,  -1, 251,  -1, 206, 236,  -7,  -3,  -1, 221, 175,  -1, 250, 190,  -3,
  -1, 235, 205,  -1, 220, 159, -15,  -7,  -3,  -1, 249, 234,  -1, 189, 219,
  -3,  -1, 143, 248,  -1, 204, 158,  -7,  -3,  -1, 233, 127,  -1, 247, 173,
  -3,  -1, 218, 188,  -1, 111,  -1, 174,  15, -19, -11,  -3,  -1, 203, 246,
  -3,  -1, 142, 232,  -1,  95, 157,  -3,  -1, 245, 126,  -1, 231, 172,  -9,
  -3,  -1, 202, 187,  -3,  -1, 217, 141,  79,  -3,  -1, 244,  63,  -1, 243,
 216, -33, -17,  -9,  -3,  -1, 230,  47,  -1, 242,  -1, 110, 240,  -3,  -1,
  31, 241,  -1, 156, 201,  -7,  -3,  -1,  94, 171,  -1, 186, 229,  -3,  -1,
 125, 215,  -1,  78, 228, -15,  -7,  -3,  -1, 140, 200,  -1,  62, 109,  -3,
  -1, 214, 227,  -1, 155, 185,  -7,  -3,  -1,  46, 170,  -1, 226,  30,  -5,
  -1, 225,  -1,  14, 224,  -1,  93, 213, -45, -25, -13,  -7,  -3,  -1, 124,
 199,  -1,  77, 139,  -1, 212,  -1, 184, 154,  -7,  -3,  -1, 169, 108,  -1,
 198,  61,  -1, 211, 210,  -9,  -5,  -3,  -1,  45,  13,  29,  -1, 123, 183,
  -5,  -1, 209,  -1,  92, 208,  -1, 197, 138, -17,  -7,  -3,  -1, 168,  76,
  -1, 196, 107,  -5,  -1, 182,  -1, 153,  12,  -1,  60, 195,  -9,  -3,  -1,
 122, 167,  -1, 166,  -1, 192,  11,  -1, 194,  -1,  44,  91, -55, -29, -15,
  -7,  -3,  -1, 181,  28,  -1, 137, 152,  -3,  -1, 193,  75,  -1, 180, 106,
  -5,  -3,  -1,  59, 121, 179,  -3,  -1, 151, 136,  -1,  43,  90, -11,  -5,
  -1, 178,  -1, 165,  27,  -1, 177,  -1, 176, 105,  -7,  -3,  -1, 150,  74,
  -1, 164, 120,  -3,  -1, 135,  58, 163, -17,  -7,  -3,  -1,  89, 149,  -1,
  42, 162,  -3,  -1,  26, 161,  -3,  -1,  10, 160, 104,  -7,  -3,  -1, 134,
  73,  -1, 148,  57,  -5,  -1, 147,  -1, 119,   9,  -1,  88, 133, -53, -29,
 -13,  -7,  -3,  -1,  41, 103,  -1, 118, 146,  -1, 145,  -1,  25, 144,  -7,
  -3,  -1,  72, 132,  -1,  87, 117,  -3,  -1,  56, 131,  -1, 102,  71,  -7,
  -3,  -1,  40, 130,  -1,  24, 129,  -7,  -3,  -1, 116,   8,  -1, 128,  86,
  -3,  -1, 101,  55,  -1, 115,  70, -17,  -7,  -3,  -1,  39, 114,  -1, 100,
  23,  -3,  -1,  85, 113,  -3,  -1,   7, 112,  54,  -7,  -3,  -1,  99,  69,
  -1,  84,  38,  -3,  -1,  98,  22,  -3,  -1,   6,  96,  53, -33, -19,  -9,
  -5,  -1,  97,  -1,  83,  68,  -1,  37,  82,  -3,  -1,  21,  81,  -3,  -1,
   5,  80,  52,  -7,  -3,  -1,  67,  36,  -1,  66,  51,  -1,  65,  -1,  20,
   4,  -9,  -3,  -1,  35,  50,  -3,  -1,  64,   3,  19,  -3,  -1,  49,  48,
  34,  -9,  -7,  -3,  -1,  18,  33,  -1,   2,  32,  17,  -3,  -1,   1,  16,
   0
};

int ampegdecoder::htab16[] =
{
-509,-503,-461,-323,-103, -37, -27, -15,  -7,  -3,  -1, 239, 254,  -1, 223,
 253,  -3,  -1, 207, 252,  -1, 191, 251,  -5,  -1, 175,  -1, 250, 159,  -3,
  -1, 249, 248, 143,  -7,  -3,  -1, 127, 247,  -1, 111, 246, 255,  -9,  -5,
  -3,  -1,  95, 245,  79,  -1, 244, 243, -53,  -1, 240,  -1,  63, -29, -19,
 -13,  -7,  -5,  -1, 206,  -1, 236, 221, 222,  -1, 233,  -1, 234, 217,  -1,
 238,  -1, 237, 235,  -3,  -1, 190, 205,  -3,  -1, 220, 219, 174, -11,  -5,
  -1, 204,  -1, 173, 218,  -3,  -1, 126, 172, 202,  -5,  -3,  -1, 201, 125,
  94, 189, 242, -93,  -5,  -3,  -1,  47,  15,  31,  -1, 241, -49, -25, -13,
  -5,  -1, 158,  -1, 188, 203,  -3,  -1, 142, 232,  -1, 157, 231,  -7,  -3,
  -1, 187, 141,  -1, 216, 110,  -1, 230, 156, -13,  -7,  -3,  -1, 171, 186,
  -1, 229, 215,  -1,  78,  -1, 228, 140,  -3,  -1, 200,  62,  -1, 109,  -1,
 214, 155, -19, -11,  -5,  -3,  -1, 185, 170, 225,  -1, 212,  -1, 184, 169,
  -5,  -1, 123,  -1, 183, 208, 227,  -7,  -3,  -1,  14, 224,  -1,  93, 213,
  -3,  -1, 124, 199,  -1,  77, 139, -75, -45, -27, -13,  -7,  -3,  -1, 154,
 108,  -1, 198,  61,  -3,  -1,  92, 197,  13,  -7,  -3,  -1, 138, 168,  -1,
 153,  76,  -3,  -1, 182, 122,  60, -11,  -5,  -3,  -1,  91, 137,  28,  -1,
 192,  -1, 152, 121,  -1, 226,  -1,  46,  30, -15,  -7,  -3,  -1, 211,  45,
  -1, 210, 209,  -5,  -1,  59,  -1, 151, 136,  29,  -7,  -3,  -1, 196, 107,
  -1, 195, 167,  -1,  44,  -1, 194, 181, -23, -13,  -7,  -3,  -1, 193,  12,
  -1,  75, 180,  -3,  -1, 106, 166, 179,  -5,  -3,  -1,  90, 165,  43,  -1,
 178,  27, -13,  -5,  -1, 177,  -1,  11, 176,  -3,  -1, 105, 150,  -1,  74,
 164,  -5,  -3,  -1, 120, 135, 163,  -3,  -1,  58,  89,  42, -97, -57, -33,
 -19, -11,  -5,  -3,  -1, 149, 104, 161,  -3,  -1, 134, 119, 148,  -5,  -3,
  -1,  73,  87, 103, 162,  -5,  -1,  26,  -1,  10, 160,  -3,  -1,  57, 147,
  -1,  88, 133,  -9,  -3,  -1,  41, 146,  -3,  -1, 118,   9,  25,  -5,  -1,
 145,  -1, 144,  72,  -3,  -1, 132, 117,  -1,  56, 131, -21, -11,  -5,  -3,
  -1, 102,  40, 130,  -3,  -1,  71, 116,  24,  -3,  -1, 129, 128,  -3,  -1,
   8,  86,  55,  -9,  -5,  -1, 115,  -1, 101,  70,  -1,  39, 114,  -5,  -3,
  -1, 100,  85,   7,  23, -23, -13,  -5,  -1, 113,  -1, 112,  54,  -3,  -1,
  99,  69,  -1,  84,  38,  -3,  -1,  98,  22,  -1,  97,  -1,   6,  96,  -9,
  -5,  -1,  83,  -1,  53,  68,  -1,  37,  82,  -1,  81,  -1,  21,   5, -33,
 -23, -13,  -7,  -3,  -1,  52,  67,  -1,  80,  36,  -3,  -1,  66,  51,  20,
  -5,  -1,  65,  -1,   4,  64,  -1,  35,  50,  -3,  -1,  19,  49,  -3,  -1,
   3,  48,  34,  -3,  -1,  18,  33,  -1,   2,  32,  -3,  -1,  17,   1,  16,
   0
};

int ampegdecoder::htab24[] =
{
-451,-117, -43, -25, -15,  -7,  -3,  -1, 239, 254,  -1, 223, 253,  -3,  -1,
 207, 252,  -1, 191, 251,  -5,  -1, 250,  -1, 175, 159,  -1, 249, 248,  -9,
  -5,  -3,  -1, 143, 127, 247,  -1, 111, 246,  -3,  -1,  95, 245,  -1,  79,
 244, -71,  -7,  -3,  -1,  63, 243,  -1,  47, 242,  -5,  -1, 241,  -1,  31,
 240, -25,  -9,  -1,  15,  -3,  -1, 238, 222,  -1, 237, 206,  -7,  -3,  -1,
 236, 221,  -1, 190, 235,  -3,  -1, 205, 220,  -1, 174, 234, -15,  -7,  -3,
  -1, 189, 219,  -1, 204, 158,  -3,  -1, 233, 173,  -1, 218, 188,  -7,  -3,
  -1, 203, 142,  -1, 232, 157,  -3,  -1, 217, 126,  -1, 231, 172, 255,-235,
-143, -77, -45, -25, -15,  -7,  -3,  -1, 202, 187,  -1, 141, 216,  -5,  -3,
  -1,  14, 224,  13, 230,  -5,  -3,  -1, 110, 156, 201,  -1,  94, 186,  -9,
  -5,  -1, 229,  -1, 171, 125,  -1, 215, 228,  -3,  -1, 140, 200,  -3,  -1,
  78,  46,  62, -15,  -7,  -3,  -1, 109, 214,  -1, 227, 155,  -3,  -1, 185,
 170,  -1, 226,  30,  -7,  -3,  -1, 225,  93,  -1, 213, 124,  -3,  -1, 199,
  77,  -1, 139, 184, -31, -15,  -7,  -3,  -1, 212, 154,  -1, 169, 108,  -3,
  -1, 198,  61,  -1, 211,  45,  -7,  -3,  -1, 210,  29,  -1, 123, 183,  -3,
  -1, 209,  92,  -1, 197, 138, -17,  -7,  -3,  -1, 168, 153,  -1,  76, 196,
  -3,  -1, 107, 182,  -3,  -1, 208,  12,  60,  -7,  -3,  -1, 195, 122,  -1,
 167,  44,  -3,  -1, 194,  91,  -1, 181,  28, -57, -35, -19,  -7,  -3,  -1,
 137, 152,  -1, 193,  75,  -5,  -3,  -1, 192,  11,  59,  -3,  -1, 176,  10,
  26,  -5,  -1, 180,  -1, 106, 166,  -3,  -1, 121, 151,  -3,  -1, 160,   9,
 144,  -9,  -3,  -1, 179, 136,  -3,  -1,  43,  90, 178,  -7,  -3,  -1, 165,
  27,  -1, 177, 105,  -1, 150, 164, -17,  -9,  -5,  -3,  -1,  74, 120, 135,
  -1,  58, 163,  -3,  -1,  89, 149,  -1,  42, 162,  -7,  -3,  -1, 161, 104,
  -1, 134, 119,  -3,  -1,  73, 148,  -1,  57, 147, -63, -31, -15,  -7,  -3,
  -1,  88, 133,  -1,  41, 103,  -3,  -1, 118, 146,  -1,  25, 145,  -7,  -3,
  -1,  72, 132,  -1,  87, 117,  -3,  -1,  56, 131,  -1, 102,  40, -17,  -7,
  -3,  -1, 130,  24,  -1,  71, 116,  -5,  -1, 129,  -1,   8, 128,  -1,  86,
 101,  -7,  -5,  -1,  23,  -1,   7, 112, 115,  -3,  -1,  55,  39, 114, -15,
  -7,  -3,  -1,  70, 100,  -1,  85, 113,  -3,  -1,  54,  99,  -1,  69,  84,
  -7,  -3,  -1,  38,  98,  -1,  22,  97,  -5,  -3,  -1,   6,  96,  53,  -1,
  83,  68, -51, -37, -23, -15,  -9,  -3,  -1,  37,  82,  -1,  21,  -1,   5,
  80,  -1,  81,  -1,  52,  67,  -3,  -1,  36,  66,  -1,  51,  20,  -9,  -5,
  -1,  65,  -1,   4,  64,  -1,  35,  50,  -1,  19,  49,  -7,  -5,  -3,  -1,
   3,  48,  34,  18,  -1,  33,  -1,   2,  32,  -3,  -1,  17,   1,  -1,  16,
   0
};

int ampegdecoder::htaba[] =
{
 -29, -21, -13,  -7,  -3,  -1,  11,  15,  -1,  13,  14,  -3,  -1,   7,   5,
   9,  -3,  -1,   6,   3,  -1,  10,  12,  -3,  -1,   2,   1,  -1,   4,   8,
   0
};

int ampegdecoder::htabb[] = // get 4 bits
{
 -15,  -7,  -3,  -1,  15,  14,  -1,  13,  12,  -3,  -1,  11,  10,  -1,   9,
   8,  -7,  -3,  -1,   7,   6,  -1,   5,   4,  -3,  -1,   3,   2,  -1,   1,
   0
};

int ampegdecoder::htablinbits[34]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,3,4,6,8,10,13,4,5,6,7,8,9,11,13,0,0};
int *ampegdecoder::htabs[34]={htab00,htab01,htab02,htab03,htab04,htab05,htab06,htab07,htab08,htab09,htab10,htab11,htab12,htab13,htab14,htab15,htab16,htab16,htab16,htab16,htab16,htab16,htab16,htab16,htab24,htab24,htab24,htab24,htab24,htab24,htab24,htab24,htaba,htabb};




// This uses Byeong Gi Lee's Fast Cosine Transform algorithm to
// decompose the 36 point and 12 point IDCT's into 9 point and 3
// point IDCT's, respectively. -- Jeff Tsay.

// 9 Point Inverse Discrete Cosine Transform
//
// This piece of code is Copyright 1997 Mikko Tommila and is freely usable
// by anybody. The algorithm itself is of course in the public domain.
//
// Again derived heuristically from the 9-point WFTA.
//
// The algorithm is optimized (?) for speed, not for small rounding errors or
// good readability.
//
// 36 additions, 11 multiplications
//
// Again this is very likely sub-optimal.
//
// The code is optimized to use a minimum number of temporary variables,
// so it should compile quite well even on 8-register Intel x86 processors.
// This makes the code quite obfuscated and very difficult to understand.
//
// References:
// [1] S. Winograd: "On Computing the Discrete Fourier Transform",
//     Mathematics of Computation, Volume 32, Number 141, January 1978,
//     Pages 175-199

#ifndef FDCTDEXT
void ampegdecoder::fdctd6(float *out, float *in)
{
  float t0,t1;
  out[1] = in[0*3] - (in[4*3]+in[3*3]);
  t0 = (in[1*3]+in[2*3]) * cos6[1];
  t1 = in[0*3] + (in[4*3]+in[3*3]) * cos6[2];
  out[0] = t1 + t0;
  out[2] = t1 - t0;

  in[1*3] += in[0*3];
  in[3*3] += in[2*3];
  in[5*3] += in[3*3]+in[4*3];
  out[4] = in[1*3] - in[5*3];
  t0 = (in[3*3]+in[1*3]) * cos6[1];
  t1 = in[1*3] + in[5*3] * cos6[2];
  out[5] = t1 + t0;
  out[3] = t1 - t0;

  float tmp;
  tmp = out[5] * sec12[0];
  out[5] = (tmp - out[0])*sec24wins[5];
  out[0] = (tmp + out[0])*sec24wins[0];
  tmp = out[4] * sec12[1];
  out[4] = (tmp - out[1])*sec24wins[4];
  out[1] = (tmp + out[1])*sec24wins[1];
  tmp = out[3] * sec12[2];
  out[3] = (tmp - out[2])*sec24wins[3];
  out[2] = (tmp + out[2])*sec24wins[2];
}

void ampegdecoder::fdctd18(float *out, float *in)
{
  in[17]+=in[16]; in[16]+=in[15]; in[15]+=in[14]; in[14]+=in[13];
  in[13]+=in[12]; in[12]+=in[11]; in[11]+=in[10]; in[10]+=in[9];
  in[9] +=in[8];  in[8] +=in[7];  in[7] +=in[6];  in[6] +=in[5];
  in[5] +=in[4];  in[4] +=in[3];  in[3] +=in[2];  in[2] +=in[1];
  in[1] +=in[0];
  in[17]+=in[15]; in[15]+=in[13]; in[13]+=in[11]; in[11]+=in[9];
  in[9] +=in[7];  in[7] +=in[5];  in[5] +=in[3];  in[3] +=in[1];

  float t0,t1,t2,t3;

  t0 = cos18[2] * (in[ 4] + in[ 8]);
  t1 = cos18[4] * (in[ 4] + in[16]);
  t2 = cos18[6] *  in[12] + in[ 0];
  t3 = cos18[8] * (in[16] - in[ 8]);
  out[0] = + t2 + t0 + t3;
  out[2] = + t2 + t1 - t0;
  out[3] = + t2 - t3 - t1;

  t0 = in[ 8] + in[16] - in[ 4];
  t1 = in[ 0] - in[12];
  out[1] = + t1 - t0 * cos18[6];
  out[4] = + t1 + t0;

  t0 = cos18[1] * (in[ 2] + in[10]);
  t1 = cos18[3] *  in[ 6];
  t2 = cos18[5] * (in[14] + in[ 2]);
  t3 = cos18[7] * (in[14] - in[10]);
  out[5] = - t1 - t2 + t0;
  out[6] = - t1 + t3 + t2;
  out[8] = + t1 + t0 + t3;

  t0 = in[ 2] - in[10] - in[14];
  out[7] = + t0 * cos18[3];

  float tmp;
  tmp=out[0];
  out[0] = tmp + out[8];
  out[8] = tmp - out[8];
  tmp=out[1];
  out[1] = tmp + out[7];
  out[7] = tmp - out[7];
  tmp=out[2];
  out[2] = tmp + out[6];
  out[6] = tmp - out[6];
  tmp=out[3];
  out[3] = tmp + out[5];
  out[5] = tmp - out[5];

  t0 = cos18[2] * (in[ 5] + in[ 9]);
  t1 = cos18[4] * (in[ 5] + in[17]);
  t2 = cos18[6] *  in[13] + in[ 1];
  t3 = cos18[8] * (in[17] - in[ 9]);
  out[15] = + t2 - t0 + t1;
  out[17] = + t2 + t3 + t0;
  out[14] = + t2 - t1 - t3;

  t0 = in[9] + in[17] - in[5];
  t1 = in[1] - in[13];
  out[16] = + t1 - t0 * cos18[6];
  out[13] = + t1 + t0;

  t0 = cos18[1] * (in[ 3] + in[11]);
  t1 = cos18[3] *  in[ 7];
  t2 = cos18[5] * (in[ 3] + in[15]);
  t3 = cos18[7] * (in[15] - in[11]);
  out[12] = - t1 - t2 + t0;
  out[11] = - t1 + t3 + t2;
  out[ 9] = + t1 + t0 + t3;

  t0 = in[ 3] - in[11] - in[15];
  out[10] = + t0 * cos18[3];

  tmp = out[17];
  out[17] = tmp + out[ 9];
  out[ 9] = tmp - out[ 9];
  tmp = out[16];
  out[16] = tmp + out[10];
  out[10] = tmp - out[10];
  tmp = out[15];
  out[15] = tmp + out[11];
  out[11] = tmp - out[11];
  tmp = out[14];
  out[14] = tmp + out[12];
  out[12] = tmp - out[12];

  tmp = out[17] * sec36[0];
  out[17] = (tmp-out[ 0])*sec72winl[17];
  out[ 0] = (tmp+out[ 0])*sec72winl[ 0];
  tmp = out[16] * sec36[1];
  out[16] = (tmp-out[ 1])*sec72winl[16];
  out[ 1] = (tmp+out[ 1])*sec72winl[ 1];
  tmp = out[15] * sec36[2];
  out[15] = (tmp-out[ 2])*sec72winl[15];
  out[ 2] = (tmp+out[ 2])*sec72winl[ 2];
  tmp = out[14] * sec36[3];
  out[14] = (tmp-out[ 3])*sec72winl[14];
  out[ 3] = (tmp+out[ 3])*sec72winl[ 3];
  tmp = out[13] * sec36[4];
  out[13] = (tmp-out[ 4])*sec72winl[13];
  out[ 4] = (tmp+out[ 4])*sec72winl[ 4];
  tmp = out[12] * sec36[5];
  out[12] = (tmp-out[ 5])*sec72winl[12];
  out[ 5] = (tmp+out[ 5])*sec72winl[ 5];
  tmp = out[11] * sec36[6];
  out[11] = (tmp-out[ 6])*sec72winl[11];
  out[ 6] = (tmp+out[ 6])*sec72winl[ 6];
  tmp = out[10] * sec36[7];
  out[10] = (tmp-out[ 7])*sec72winl[10];
  out[ 7] = (tmp+out[ 7])*sec72winl[ 7];
  tmp = out[ 9] * sec36[8];
  out[ 9] = (tmp-out[ 8])*sec72winl[ 9];
  out[ 8] = (tmp+out[ 8])*sec72winl[ 8];
}
#endif

void ampegdecoder::imdct(float *out, float *in, float *prv, int type)
{
  if (type!=2)
  {
    float tmp[18];

    fdctd18(tmp, in);

    if (type!=3)
    {
      out[ 0] = prv[ 0] - tmp[ 9];
      out[ 1] = prv[ 1] - tmp[10];
      out[ 2] = prv[ 2] - tmp[11];
      out[ 3] = prv[ 3] - tmp[12];
      out[ 4] = prv[ 4] - tmp[13];
      out[ 5] = prv[ 5] - tmp[14];
      out[ 6] = prv[ 6] - tmp[15];
      out[ 7] = prv[ 7] - tmp[16];
      out[ 8] = prv[ 8] - tmp[17];
      out[ 9] = prv[ 9] + tmp[17] * winlql[ 8];
      out[10] = prv[10] + tmp[16] * winlql[ 7];
      out[11] = prv[11] + tmp[15] * winlql[ 6];
      out[12] = prv[12] + tmp[14] * winlql[ 5];
      out[13] = prv[13] + tmp[13] * winlql[ 4];
      out[14] = prv[14] + tmp[12] * winlql[ 3];
      out[15] = prv[15] + tmp[11] * winlql[ 2];
      out[16] = prv[16] + tmp[10] * winlql[ 1];
      out[17] = prv[17] + tmp[ 9] * winlql[ 0];
    }
    else
    {
      out[ 0] = prv[ 0];
      out[ 1] = prv[ 1];
      out[ 2] = prv[ 2];
      out[ 3] = prv[ 3];
      out[ 4] = prv[ 4];
      out[ 5] = prv[ 5];
      out[ 6] = prv[ 6] - tmp[15] * winsql[11];
      out[ 7] = prv[ 7] - tmp[16] * winsql[10];
      out[ 8] = prv[ 8] - tmp[17] * winsql[ 9];
      out[ 9] = prv[ 9] + tmp[17] * winsql[ 8];
      out[10] = prv[10] + tmp[16] * winsql[ 7];
      out[11] = prv[11] + tmp[15] * winsql[ 6];
      out[12] = prv[12] + tmp[14] * winsql[ 5];
      out[13] = prv[13] + tmp[13] * winsql[ 4];
      out[14] = prv[14] + tmp[12] * winsql[ 3];
      out[15] = prv[15] + tmp[11] * winsql[ 2];
      out[16] = prv[16] + tmp[10] * winsql[ 1];
      out[17] = prv[17] + tmp[ 9] * winsql[ 0];
    }
    if (type!=1)
    {
      prv[ 0] =           tmp[ 8] * winlql[ 0];
      prv[ 1] =           tmp[ 7] * winlql[ 1];
      prv[ 2] =           tmp[ 6] * winlql[ 2];
      prv[ 3] =           tmp[ 5] * winlql[ 3];
      prv[ 4] =           tmp[ 4] * winlql[ 4];
      prv[ 5] =           tmp[ 3] * winlql[ 5];
      prv[ 6] =           tmp[ 2] * winlql[ 6];
      prv[ 7] =           tmp[ 1] * winlql[ 7];
      prv[ 8] =           tmp[ 0] * winlql[ 8];
      prv[ 9] =           tmp[ 0];
      prv[10] =           tmp[ 1];
      prv[11] =           tmp[ 2];
      prv[12] =           tmp[ 3];
      prv[13] =           tmp[ 4];
      prv[14] =           tmp[ 5];
      prv[15] =           tmp[ 6];
      prv[16] =           tmp[ 7];
      prv[17] =           tmp[ 8];
    }
    else
    {
      prv[ 0] =           tmp[ 8] * winsql[ 0];
      prv[ 1] =           tmp[ 7] * winsql[ 1];
      prv[ 2] =           tmp[ 6] * winsql[ 2];
      prv[ 3] =           tmp[ 5] * winsql[ 3];
      prv[ 4] =           tmp[ 4] * winsql[ 4];
      prv[ 5] =           tmp[ 3] * winsql[ 5];
      prv[ 6] =           tmp[ 2] * winsql[ 6];
      prv[ 7] =           tmp[ 1] * winsql[ 7];
      prv[ 8] =           tmp[ 0] * winsql[ 8];
      prv[ 9] =           tmp[ 0] * winsql[ 9];
      prv[10] =           tmp[ 1] * winsql[10];
      prv[11] =           tmp[ 2] * winsql[11];
      prv[12] = 0;
      prv[13] = 0;
      prv[14] = 0;
      prv[15] = 0;
      prv[16] = 0;
      prv[17] = 0;
    }
  }
  else
  {
    float tmp[3][6];

    fdctd6(tmp[0], in+0);
    fdctd6(tmp[1], in+1);
    fdctd6(tmp[2], in+2);

    out[ 0] = prv[ 0];
    out[ 1] = prv[ 1];
    out[ 2] = prv[ 2];
    out[ 3] = prv[ 3];
    out[ 4] = prv[ 4];
    out[ 5] = prv[ 5];
    out[ 6] = prv[ 6] - tmp[0][3];
    out[ 7] = prv[ 7] - tmp[0][4];
    out[ 8] = prv[ 8] - tmp[0][5];
    out[ 9] = prv[ 9] + tmp[0][5] *  winsqs[2];
    out[10] = prv[10] + tmp[0][4] *  winsqs[1];
    out[11] = prv[11] + tmp[0][3] *  winsqs[0];
    out[12] = prv[12] + tmp[0][2] *  winsqs[0] - tmp[1][3];
    out[13] = prv[13] + tmp[0][1] *  winsqs[1] - tmp[1][4];
    out[14] = prv[14] + tmp[0][0] *  winsqs[2] - tmp[1][5];
    out[15] = prv[15] + tmp[0][0]              + tmp[1][5] *  winsqs[2];
    out[16] = prv[16] + tmp[0][1]              + tmp[1][4] *  winsqs[1];
    out[17] = prv[17] + tmp[0][2]              + tmp[1][3] *  winsqs[0];

    prv[ 0] =         - tmp[2][3]              + tmp[1][2] *  winsqs[0];
    prv[ 1] =         - tmp[2][4]              + tmp[1][1] *  winsqs[1];
    prv[ 2] =         - tmp[2][5]              + tmp[1][0] *  winsqs[2];
    prv[ 3] =           tmp[2][5] *  winsqs[2] + tmp[1][0];
    prv[ 4] =           tmp[2][4] *  winsqs[1] + tmp[1][1];
    prv[ 5] =           tmp[2][3] *  winsqs[0] + tmp[1][2];
    prv[ 6] =           tmp[2][2] *  winsqs[0];
    prv[ 7] =           tmp[2][1] *  winsqs[1];
    prv[ 8] =           tmp[2][0] *  winsqs[2];
    prv[ 9] =           tmp[2][0];
    prv[10] =           tmp[2][1];
    prv[11] =           tmp[2][2];
    prv[12] = 0;
    prv[13] = 0;
    prv[14] = 0;
    prv[15] = 0;
    prv[16] = 0;
    prv[17] = 0;
  }
}



void ampegdecoder::readsfsi(grsistruct &si)
{
  int i;
  for (i=0; i<4; i++)
    si.sfsi[i]=si.gr?mpgetbit():0;
}

void ampegdecoder::readgrsi(grsistruct &si, int &bitpos)
{
  int i;
  si.grstart=bitpos;
  bitpos+=mpgetbits(12);
  si.grend=bitpos;
  si.regionend[2]=mpgetbits(9)*2;
  si.globalgain=mpgetbits(8);
  if (hdrlsf&&(hdrmode==1)&&(hdrmodeext&1)&&si.ch)
  {
    si.sfcompress=mpgetbits(8);
    si.ktabsel=mpgetbit();
  }
  else
  {
    si.sfcompress=mpgetbits(hdrlsf?9:4);
    si.ktabsel=2;
  }
  if (mpgetbit())
  {
    si.blocktype = mpgetbits(2);
    si.mixedblock = mpgetbit();
    for (i=0; i<2; i++)
      si.tabsel[i]=mpgetbits(5);
    si.tabsel[2]=0;
    for (i=0; i<3; i++)
      si.subblockgain[i]=4*mpgetbits(3);

    if (si.blocktype==2)
      si.regionend[0]=sfbands[hdrlsf][hdrfreq][3];
    else
      si.regionend[0]=sfbandl[hdrlsf][hdrfreq][HUFFMAXLONG];
    si.regionend[1]=576;
  }
  else
  {
    for (i=0; i<3; i++)
      si.tabsel[i]=mpgetbits(5);
    int region0count = mpgetbits(4)+1;
    int region1count = mpgetbits(3)+1+region0count;
    si.regionend[0]=sfbandl[hdrlsf][hdrfreq][region0count];
    si.regionend[1]=sfbandl[hdrlsf][hdrfreq][region1count];
    si.blocktype = 0;
    si.mixedblock = 0;
  }
  if (si.regionend[0]>si.regionend[2])
    si.regionend[0]=si.regionend[2];
  if (si.regionend[1]>si.regionend[2])
    si.regionend[1]=si.regionend[2];
  si.regionend[3]=576;
  si.preflag=hdrlsf?(si.sfcompress>=500)?1:0:mpgetbit();
  si.sfshift=mpgetbit();
  si.tabsel[3]=mpgetbit()?33:32;

  if (si.blocktype)
    for (i=0; i<4; i++)
      si.sfsi[i]=0;
}

int ampegdecoder::huffmandecoder(int *tab)
{
  while (1)
  {
    int v=*tab++;
    if (v>=0)
      return v;
    if (mpgetbit())
      tab-=v;
  }
}

void ampegdecoder::readscalefac(grsistruct &si, int *scalefacl)
{
  *bitbufpos=si.grstart;
  int newslen[4];
  int blocknumber;
  if (!hdrlsf)
  {
    newslen[0]=slentab[0][si.sfcompress];
    newslen[1]=slentab[0][si.sfcompress];
    newslen[2]=slentab[1][si.sfcompress];
    newslen[3]=slentab[1][si.sfcompress];
    blocknumber=6;
  }
  else
    if ((hdrmode!=1)||!(hdrmodeext&1)||!si.ch)
    {
      if (si.sfcompress>=500)
      {
        newslen[0] = ((si.sfcompress-500)/ 3)%4;
        newslen[1] = ((si.sfcompress-500)/ 1)%3;
        newslen[2] = ((si.sfcompress-500)/ 1)%1;
        newslen[3] = ((si.sfcompress-500)/ 1)%1;
        blocknumber = 2;
      }
      else
      if (si.sfcompress>=400)
      {
        newslen[0] = ((si.sfcompress-400)/20)%5;
        newslen[1] = ((si.sfcompress-400)/ 4)%5;
        newslen[2] = ((si.sfcompress-400)/ 1)%4;
        newslen[3] = ((si.sfcompress-400)/ 1)%1;
        blocknumber = 1;
      }
      else
      {
        newslen[0] = ((si.sfcompress-  0)/80)%5;
        newslen[1] = ((si.sfcompress-  0)/16)%5;
        newslen[2] = ((si.sfcompress-  0)/ 4)%4;
        newslen[3] = ((si.sfcompress-  0)/ 1)%4;
        blocknumber = 0;
      }
    }
    else
    {
      if (si.sfcompress>=244)
      {
        newslen[0] = ((si.sfcompress-244)/ 3)%4;
        newslen[1] = ((si.sfcompress-244)/ 1)%3;
        newslen[2] = ((si.sfcompress-244)/ 1)%1;
        newslen[3] = ((si.sfcompress-244)/ 1)%1;
        blocknumber = 5;
      }
      else
      if (si.sfcompress>=180)
      {
        newslen[0] = ((si.sfcompress-180)/16)%4;
        newslen[1] = ((si.sfcompress-180)/ 4)%4;
        newslen[2] = ((si.sfcompress-180)/ 1)%4;
        newslen[3] = ((si.sfcompress-180)/ 1)%1;
        blocknumber = 4;
      }
      else
      {
        newslen[0] = ((si.sfcompress-  0)/36)%5;
        newslen[1] = ((si.sfcompress-  0)/ 6)%6;
        newslen[2] = ((si.sfcompress-  0)/ 1)%6;
        newslen[3] = ((si.sfcompress-  0)/ 1)%1;
        blocknumber = 3;
      }
    }

  int i,k;

  int *sfb=sfbtab[blocknumber][(si.blocktype!=2)?0:si.mixedblock?2:1];
  int *sfp=scalefacl;
  for (i=0;i<4;i++)
    if (!si.sfsi[i])
      for (k=sfb[i]; k<sfb[i+1]; k++)
        *sfp++=mpgetbits(newslen[i]);
    else
      sfp+=sfb[i+1]-sfb[i];
  *sfp++=0;
  *sfp++=0;
  *sfp++=0;
}




void ampegdecoder::readhuffman(grsistruct &si, float *xr)
{
  int *ro=rotab[(si.blocktype!=2)?0:si.mixedblock?2:1];
  int i;
  for (i=0; i<si.regionend[2]; i+=2)
  {
    int t=(i<si.regionend[0])?0:(i<si.regionend[1])?1:2;
    int linbits=htablinbits[si.tabsel[t]];
    int val=huffmandecoder(htabs[si.tabsel[t]]);
    int x;
    double v;
    x=val>>4;
    if (x==15)
      x=15+mpgetbits(linbits);
    v=pow43tab[x];
    if (x)
      if (mpgetbit())
        v=-v;
    xr[ro[i+0]]=v;

    x=val&15;
    if (x==15)
      x=15+mpgetbits(linbits);
    v=pow43tab[x];
    if (x)
      if (mpgetbit())
        v=-v;
    xr[ro[i+1]]=v;
  }

  while ((*bitbufpos<si.grend)&&(i<576))
  {
    int val=huffmandecoder(htabs[si.tabsel[3]]);
    int x;
    x=(val>>3)&1;
    if (x)
      if (mpgetbit())
        x=-x;
    xr[ro[i+0]]=x;
    x=(val>>2)&1;
    if (x)
      if (mpgetbit())
        x=-x;
    xr[ro[i+1]]=x;
    x=(val>>1)&1;
    if (x)
      if (mpgetbit())
        x=-x;
    xr[ro[i+2]]=x;
    x=val&1;
    if (x)
      if (mpgetbit())
        x=-x;
    xr[ro[i+3]]=x;

    i+=4;
  }

  if (*bitbufpos>si.grend)
    i-=4;

  while (i<576)
    xr[ro[i++]]=0;
}


void ampegdecoder::doscale(grsistruct &si, float *xr, int *scalefacl)
{
  int *bil=sfbandl[hdrlsf][hdrfreq];
  int *bis=sfbands[hdrlsf][hdrfreq];
  int largemax=(si.blocktype==2)?si.mixedblock?(hdrlsf?6:8):0:22;
  int smallmin=(si.blocktype==2)?si.mixedblock?3:0:13;

  int j,k,i;
  int *sfp=scalefacl;
  float gain=ggaintab[si.globalgain];
  for (j=0; j<largemax; j++)
  {
    double f=gain*pow2tab[(*sfp++ +(si.preflag?pretab[j]:0))<<si.sfshift];
    for (i=bil[j]; i<bil[j+1]; i++)
      xr[i]*=f;
  }
  float sgain[3];
  if (smallmin!=13)
  {
    for (k=0; k<3; k++)
      sgain[k]=gain*pow2tab[si.subblockgain[k]];
  }
  for (j=smallmin; j<13; j++)
    for (k=0; k<3; k++)
    {
      double f=sgain[k]*pow2tab[*sfp++<<si.sfshift];
      for (i=bis[j]+k; i<bis[j+1]; i+=3)
        xr[i]*=f;
    }
}

void ampegdecoder::jointstereo(grsistruct &si, float (*xr)[576], int *scalefacl)
{
  int i,j;

  if (!hdrmodeext)
    return;
  int max=576>>ratereduce;
  if (hdrmodeext==2)
  {
    for (i=0; i<max; i++)
    {
      double a=xr[0][i];
      xr[0][i] = (a+xr[1][i])*sqrt05;
      xr[1][i] = (a-xr[1][i])*sqrt05;
    }
    return;
  }

  int *bil=sfbandl[hdrlsf][hdrfreq];
  int *bis=sfbands[hdrlsf][hdrfreq];

  for (i=0; i<max; i++)
    ispos[i]=7;

  float (*kt)[2]=ktab[si.ktabsel];

  int sfb;
  int sfbhigh=22;
  if (si.blocktype==2)
  {
    int sfblow=si.mixedblock?3:0;
    int *scalefacs=scalefacl+(si.mixedblock?hdrlsf?(6-3*3):(8-3*3):0);
    sfbhigh=si.mixedblock?(hdrlsf?6:8):0;

    for (j=0; j<3; j++)
    {
      for (i=bis[13]+j-3; i>=bis[sfblow]; i-=3)
        if (xr[1][i])
          break;
      for (sfb=sfblow; sfb<13; sfb++)
        if (i<bis[sfb])
          break;
      if (sfb>3)
        sfbhigh=0;
      int v=7;
      for (sfb=sfb; sfb<13; sfb++)
      {
        v=(sfb==12)?v:scalefacs[sfb*3+j];
        for (i=bis[sfb]+j; i<bis[sfb+1]; i+=3)
          ispos[i]=v;
      }
    }
  }
  for (i=bil[sfbhigh]-1; i>=0; i--)
    if (xr[1][i])
      break;
  for (sfb=0; sfb<sfbhigh; sfb++)
    if (i<bil[sfb])
      break;
  int v=7;
  for (sfb=sfb; sfb<sfbhigh; sfb++)
  {
    v=(sfb==21)?v:scalefacl[sfb];
    for (i=bil[sfb]; i<bil[sfb+1]; i++)
      ispos[i]=v;
  }

  int msstereo=hdrmodeext&2;
  for (i=0; i<max; i++)
  {
    if (ispos[i]==7)
    {
      if (msstereo)
      {
        double a=xr[0][i];
        xr[0][i] = (a+xr[1][i])*sqrt05;
        xr[1][i] = (a-xr[1][i])*sqrt05;
      }
    }
    else
    {
      xr[1][i] = xr[0][i] * kt[ispos[i]][1];
      xr[0][i] = xr[0][i] * kt[ispos[i]][0];
    }
  }
}

void ampegdecoder::hybrid(grsistruct &si, float (*hout)[32], float (*prev)[18], float *xr)
{
  int nbands=32>>ratereduce;
  int lim=(si.blocktype!=2)?(32>>ratereduce):si.mixedblock?(sfbands[hdrlsf][hdrfreq][3]/18):0;

  int sb,ss;
  for (sb=1;sb<lim;sb++)
    for (ss=0;ss<8;ss++)
    {
      float bu = xr[(sb-1)*18+17-ss];
      float bd = xr[sb*18+ss];
      xr[(sb-1)*18+17-ss] = (bu * csatab[ss][0]) - (bd * csatab[ss][1]);
      xr[sb*18+ss] = (bd * csatab[ss][0]) + (bu * csatab[ss][1]);
    }

  if (l3equalon)
  {
    int i;
    int shlim=lim*6;
    for (i=0; i<shlim; i++)
    {
      xr[3*i+0]*=l3equals[i];
      xr[3*i+1]*=l3equals[i];
      xr[3*i+2]*=l3equals[i];
    }
    int nbnd=nbands*18;
    for (i=shlim*3; i<nbnd; i++)
      xr[i]*=l3equall[i];
  }

  int mixlim=si.mixedblock?(sfbands[hdrlsf][hdrfreq][3]/18):0;
  for (sb=0; sb<nbands; sb++)
  {
    float rawout[18];
    imdct(rawout, xr+sb*18, prev[sb], (sb<mixlim)?0:si.blocktype);
    if (sb&1)
      for (ss=0; ss<18; ss+=2)
      {
        hout[ss][sb]=rawout[ss];
        hout[ss+1][sb]=-rawout[ss+1];
      }
    else
      for (ss=0; ss<18; ss++)
        hout[ss][sb]=rawout[ss];
  }
}

void ampegdecoder::readmain(grsistruct (*si)[2])
{
  int stereo=(hdrmode==3)?1:2;

  int maindatabegin = mpgetbits(hdrlsf?8:9);

  mpgetbits(hdrlsf?(stereo==1)?1:2:(stereo==1)?5:3);

  if (!si)
    mpgetbits(hdrlsf?(stereo==1)?64:128:(stereo==1)?127:247);
  else
  {
    int ngr=hdrlsf?1:2;
    int gr,ch;
    for (gr=0; gr<ngr; gr++)
      for (ch=0; ch<stereo; ch++)
      {
        si[ch][gr].ch=ch;
        si[ch][gr].gr=gr;
      }

    for (gr=0; gr<ngr; gr++)
      for (ch=0; ch<stereo; ch++)
        readsfsi(si[ch][gr]);

    int bitpos=0;
      for (gr=0; gr<ngr; gr++)
        for (ch=0; ch<stereo; ch++)
          readgrsi(si[ch][gr], bitpos);
  }

  int mainslots=(hdrlsf?72:144)*1000*ratetab[hdrlsf?1:0][2][hdrbitrate]/(freqtab[hdrfreq]>>hdrlsf)+(hdrpadding?1:0)-4-(hdrcrc?2:0)-(hdrlsf?(stereo==1)?9:17:(stereo==1)?17:32);

  if (huffoffset<maindatabegin)
    huffoffset=maindatabegin;
  memmove(huffbuf, huffbuf+huffoffset-maindatabegin, maindatabegin);
  getbytes(huffbuf+maindatabegin, mainslots);
  huffoffset=maindatabegin+mainslots;
  bitbuf=huffbuf;
  bitbufpos=&huffbit;
}


void ampegdecoder::decode3()
{
  int fr,gr,ch,sb,ss;
  for (fr=0; fr<(hdrlsf?2:1); fr++)
  {
    grsistruct si0[2][2];

    if (fr)
      decodehdr(0);

    if (!hdrbitrate)
    {
      for (gr=fr; gr<2; gr++)
        for (ch=0; ch<2; ch++)
          for (sb=0; sb<32; sb++)
            for (ss=0; ss<18; ss++)
            {
              fraction[ch][gr*18+ss][sb]=((sb&ss&1)?-1:1)*prevblck[ch][sb][ss];
              prevblck[ch][sb][ss]=0;
            }
      return;
    }

    readmain(si0);
    int stereo=(hdrmode==3)?1:2;
    int ngr=hdrlsf?1:2;
    for (gr=0;gr<ngr;gr++)
    {
      for (ch=0; ch<stereo; ch++)
      {
        readscalefac(si0[ch][gr], scalefac0[ch]);
        readhuffman(si0[ch][gr], xr0[ch]);
        doscale(si0[ch][gr], xr0[ch], scalefac0[ch]);
      }

      if (hdrmode==1)
        jointstereo(si0[1][gr], xr0, scalefac0[1]);

      for (ch=0; ch<stereo; ch++)
        hybrid(si0[ch][gr], fraction[ch]+(fr+gr)*18, prevblck[ch], xr0[ch]);
    }
  }
}

void ampegdecoder::seekinit3(int discard)
{
  int i,j,k;
  if (seekinitframes!=discard)
    for (i=0; i<2; i++)
      for (j=0; j<32; j++)
        for (k=0; k<18; k++)
          prevblck[i][j][k]=0;
  huffoffset=0;
  for (i=discard; i<(seekinitframes-1); i++)
    for (j=0; j<(hdrlsf?2:1); j++)
    {
      if (!decodehdr(0))
        return;
      readmain(0);
    }
}

void ampegdecoder::setl3equal(const float *buf)
{
  if (!buf)
  {
    l3equalon=0;
    return;
  }
  int i;
  for (i=0; i<576; i++)
    if (buf[i]!=1)
      break;
  if (i==576)
  {
    l3equalon=0;
    return;
  }
  if (ratereduce==0)
    for (i=0; i<576; i++)
      l3equall[i]=buf[i];
  else
  if (ratereduce==1)
    for (i=0; i<288; i++)
      l3equall[i]=(buf[2*i+0]+buf[2*i+1])/2;
  else
    for (i=0; i<144; i++)
      l3equall[i]=(buf[4*i+0]+buf[4*i+1]+buf[4*i+2]+buf[4*i+3])/4;
  for (i=0; i<(192>>ratereduce); i++)
    l3equals[i]=(l3equall[3*i+0]+l3equall[3*i+1]+l3equall[3*i+2])/3;
  l3equalon=1;
}

void ampegdecoder::init3()
{
  int i;
  for (i=0; i<32; i++)
  {
    ktab[0][i][0]=(i&1)?exp(-log(2.0f)*0.5*((i+1)>>1)):1;
    ktab[0][i][1]=(i&1)?1:exp(-log(2.0f)*0.5*(i>>1));
    ktab[1][i][0]=(i&1)?exp(-log(2.0f)*0.25*((i+1)>>1)):1;
    ktab[1][i][1]=(i&1)?1:exp(-log(2.0f)*0.25*(i>>1));
    ktab[2][i][0]=sin(_PI/12*i)/(sin(_PI/12*i)+cos(_PI/12*i));
    ktab[2][i][1]=cos(_PI/12*i)/(sin(_PI/12*i)+cos(_PI/12*i));
  }

  for (i=0; i<65; i++)
    pow2tab[i]=exp(log(0.5)*0.5*i);

  for (i=0;i<8;i++)
  {
    csatab[i][0] = 1.0/sqrt(1.0+citab[i]*citab[i]);
    csatab[i][1] = citab[i]*csatab[i][0];
  }

  for (i=0; i<3; i++)
    winsqs[i]=cos(_PI/24*(2*i+1))/sin(_PI/24*(2*i+1));
  for (i=0; i<9; i++)
    winlql[i]=cos(_PI/72*(2*i+1))/sin(_PI/72*(2*i+1));
  for (i=0; i<6; i++)
    winsql[i]=1/sin(_PI/72*(2*i+1));
  for (i=6; i<9; i++)
    winsql[i]=sin(_PI/24*(2*i+1))/sin(_PI/72*(2*i+1));
  for (i=9; i<12; i++)
    winsql[i]=sin(_PI/24*(2*i+1))/cos(_PI/72*(2*i+1));
  for (i=0; i<6; i++)
    sec24wins[i]=0.5/cos(_PI*(2*i+1)/24.0)*sin(_PI/24*(2*i+1)-_PI/4);
  for (i=0; i<18; i++)
    sec72winl[i]=0.5/cos(_PI*(2*i+1)/72.0)*sin(_PI/72*(2*i+1)-_PI/4);
  for (i=0; i<3; i++)
    sec12[i]=0.5/cos(_PI*(2*i+1)/12.0);
  for (i=0; i<9; i++)
    sec36[i]=0.5/cos(_PI*(2*i+1)/36.0);
  for (i=0;i<3;i++)
    cos6[i]=cos(_PI/6*i);
  for (i=0;i<9;i++)
    cos18[i]=cos(_PI/18*i);

  for (i=0;i<256;i++)
    ggaintab[i]=exp(log(0.5)*0.25*(210-i));

  pow43tab[0]=0;
  for (i=1; i<8207; i++)
    pow43tab[i]=exp(log(i*1.0f)*4/3);

  sqrt05=sqrt(0.5);
}

void ampegdecoder::openlayer3(int rate)
{
  if (rate)
  {
    slotsize=1;
    slotdiv=freqtab[orgfreq]>>orglsf;
    nslots=(144*rate)/(freqtab[orgfreq]>>orglsf);
    fslots=(144*rate)%slotdiv;
    seekinitframes=1+3+(orglsf?254:510)/(nslots-38);
  }

  int i,j,k;

  for (i=0; i<13; i++)
  {
    int sfbstart=sfbands[orglsf][orgfreq][i];
    int sfblines=(sfbands[orglsf][orgfreq][i+1]-sfbands[orglsf][orgfreq][i])/3;
    for (k=0; k<3; k++)
      for (j=0;j<sfblines;j++)
      {
        rotab[0][sfbstart+k*sfblines+j]=sfbstart+k*sfblines+j;
        rotab[1][sfbstart+k*sfblines+j]=sfbstart+k+j*3;
        rotab[2][sfbstart+k*sfblines+j]=(i<3)?(sfbstart+k*sfblines+j):(sfbstart+k+j*3);
      }
  }

  huffoffset=0;

  for (i=0; i<2; i++)
    for (j=0; j<32; j++)
      for (k=0; k<18; k++)
        prevblck[i][j][k]=0;

  l3equalon=0;
}

/****************************************************************************/
/***                                                                      ***/
/***   ampdec.cpp                                                         ***/
/***                                                                      ***/
/****************************************************************************/

extern unsigned char InBuffer[];
extern int InSize;
extern int InIndex;

#define MOVE32(p) (*((unsigned int *)(p)))

int ampegdecoder::lsftab[4]={2,3,1,0};
int ampegdecoder::freqtab[4]={44100,48000,32000};

int ampegdecoder::ratetab[2][3][16]=
{
  {
    {  0, 32, 64, 96,128,160,192,224,256,288,320,352,384,416,448,  0},
    {  0, 32, 48, 56, 64, 80, 96,112,128,160,192,224,256,320,384,  0},
    {  0, 32, 40, 48, 56, 64, 80, 96,112,128,160,192,224,256,320,  0},
  },
  {
    {  0, 32, 48, 56, 64, 80, 96,112,128,144,160,176,192,224,256,  0},
    {  0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160,  0},
    {  0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160,  0},
  },
};


void ampegdecoder::getbytes(void *buf2, int n)
{
  memcpy(buf2, bitbuf+(*bitbufpos>>3), n);
  *bitbufpos+=n*8;
}

int ampegdecoder::sync7FF()
{
  int index;
  int max;

  max = mainbufsize-1;

  index = (mainbufpos+7)>>3;
  
  while(!(mainbuf[index]==0xff && mainbuf[index+1]>=0xe0) && index<max)
    index++;

  while((mainbuf[index]==0xff && mainbuf[index+1]>=0xe0) && index<max)
    index++;

  if(index>=max)
	return 0;

  mainbufpos = (index<<3)+3;
  return 1;
/*
  mainbufpos=(mainbufpos+7)&~7;

  while (1)
  {
    if(mainbufpos+16>mainbuflen)
      return 0;
    while ((((mainbufpos>>3)+1)<mainbuflen)&&((mainbuf[mainbufpos>>3]!=0xFF)||(mainbuf[(mainbufpos>>3)+1]<0xE0)))
    {
      mainbufpos+=8;
    }
    while (1)
    {
      if (((mainbufpos>>3)+1)>=mainbuflen)
        break;
      if (mainbuf[mainbufpos>>3]!=0xFF)
        break;
      if (mainbuf[(mainbufpos>>3)+1]<0xE0)
        break;
      mainbufpos+=8;
    }
    if ((mainbufpos>>3)<mainbuflen)
    {
      mainbufpos+=3;
      return 1;
    }
  }
  */
}

int ampegdecoder::decodehdr(int init)
{
  while (1)
  {
    if (!sync7FF())
    {
      hdrbitrate=0;
      return 0;
    }

    bitbuf=mainbuf;
    bitbufpos=&mainbufpos;
    hdrlsf=lsftab[mpgetbits(2)];
    hdrlay=3-mpgetbits(2);
    hdrcrc=!mpgetbit();
    hdrbitrate = mpgetbits(4);
    hdrfreq = mpgetbits(2);
    hdrpadding = mpgetbit();
    mpgetbit(); // extension
    hdrmode = mpgetbits(2);
    hdrmodeext = mpgetbits(2);
    mpgetbit(); // copyright
    mpgetbit(); // original
    mpgetbits(2); // emphasis
    if (init)
    {
      orglsf=hdrlsf;
      orglay=hdrlay;
      orgfreq=hdrfreq;
      orgstereo=(hdrmode==1)?0:hdrmode;
    }
    if ((hdrlsf!=orglsf)||(hdrlay!=orglay)||(hdrbitrate==0)||(hdrbitrate==15)||(hdrfreq!=orgfreq)||(((hdrmode==1)?0:hdrmode)!=orgstereo))
    {
      *bitbufpos-=20;
      continue;
    }
    if (hdrcrc)
      mpgetbits(16);
    return 1;
  }
}


int ampegdecoder::getheader(int &layer, int &lsf, int &freq, int &stereo, int &rate)
{
  int totrate=0;
  int i;
  unsigned char *inp;


  InIndex = 0;
  inp = InBuffer;

  if (MOVE32(inp)==0x49443303)
  {
    {
      unsigned char c;
      do
      {
        do
        {
	      c=*inp++;
        }
        while(c!=0xff /*&& !in.eof()*/);
        c=*inp;
      }
      while(c<0xe0 /*&& !in.eof()*/);
    }
  }
  else
  {    
    unsigned char c;

	c = *inp++;
    if(c!=0xff)
      return 0;
  }

  for (i=0; i<8; i++)
  {
    unsigned char hdr[4];
	hdr[1] = inp[0];
	hdr[2] = inp[1];
	hdr[3] = inp[2];
	int nr = 3;
    if ((nr!=3)&&!i)
      return 0;
//    if (hdr[0]!=0xFF)
//      return 0;
    if (hdr[1]<0xE0)
      return 0;
    lsf=lsftab[((hdr[1]>>3)&3)];
    if (lsf==3)
      return 0;
    layer=3-((hdr[1]>>1)&3);
    if (layer==3)
      return 0;
    if ((lsf==2)&&(layer!=2))
      return 0;
    int pad=(hdr[2]>>1)&1;
    stereo=((hdr[3]>>6)&3)!=3;
    freq=freqtab[(hdr[2]>>2)&3]>>lsf;
    if (!freq)
      return 0;
    rate=ratetab[lsf?1:0][layer][(hdr[2]>>4)&15]*1000;
    if (!rate)
      return 0;

    if ((layer!=2))
      return 1;
    totrate+=rate;
    inp += ((layer==0)?(((12*rate)/freq+pad)*4):(((lsf&&(layer==2))?72:144)*rate/freq+pad));
  }
  rate=totrate/i;

  return 1;
}


int ampegdecoder::decode(void *outsamp)
{
  int rate;
  if (init)
  {
	stream = 0 ;
    int layer,lsf,freq,stereo;
    if (!getheader(layer, lsf, freq, stereo, rate))
      return 0;
    if (stream)
      rate=0;
    atend=0;
  }
  if (atend)
    return 0;
  if (!decodehdr(init))
    if (init)
      return 0;
    else
      atend=1;
  if (init)
  {
    if (orglay==0)
      openlayer1(rate);
    else
    if (orglay==1)
      openlayer2(rate);
    else
    if (orglay==2)
      openlayer3(rate);
    else
      return 0;
    if (rate)
      nframes=(long)floor((double)(InSize+slotsize)*slotdiv/((nslots*slotdiv+fslots)*slotsize)+0.5);
    else
      nframes=0;
  }
  if (orglay==0)
    decode1();
  else
  if (orglay==1)
    decode2();
  else
    decode3();

  if (init)
  {
    srcchan=(orgstereo==3)?1:2;
    opensynth();
    framepos=0;
    framesize=(36*32*samplesize*dstchan)>>ratereduce;
    curframe=1;
  }

  synth(outsamp, fraction[0], fraction[1]);
  return 1;
}


int ampegdecoder::Start(unsigned char *buffer,int size)
{
  if(buffer[0]=='I' && buffer[1]=='D' && buffer[2]=='3')
  {
    buffer+=3;
    size-=3;
    while(size>2 && !(buffer[0]==0xff && buffer[1]==0xfb))
    {
      buffer++;
      size--;
    }
  }

  InBuffer = buffer;
  InSize = size;
  InIndex = 0;

  mainbufsize=size;
  mainbuf = buffer;
  mainbuflow=-1;
  mainbufpos=0;
  mainbuflen=InSize*8;

  init12();
  init3();

  dstchan=2;
  ratereduce=0;
  samplesize=2;

  init=1;
  if (!decode(framebuf))
  {
    return 0;
  }
  init=0;

//  freq=freqtab[orgfreq]>>(orglsf+ratereduce);
//  stereo=(dstchan==2)?1:0;

  return -1;
}

int ampegdecoder::DecodeFrame(void *out)
{ 
  int res;

  res=decode(out);
  return res ? framesize : 0;
}
/****************************************************************************/
/***                                                                      ***/
/***   ampsynth.cpp                                                       ***/
/***                                                                      ***/
/****************************************************************************/

//#define _PI 3.14159265358979323846
#define BUFFEROFFSET (16*17)

float ampegdecoder::dwin[1024];
float ampegdecoder::dwin2[512];
float ampegdecoder::dwin4[256];
float ampegdecoder::sectab[32];

#ifdef __WATCOMC__
#define MULADDINLINE
float __muladd16a(float *a, float *b);
#pragma aux __muladd16a parm [ebx] [ecx] value [8087] modify exact [8087] = \
  "fld  dword ptr [ebx+ 0]" \
  "fmul dword ptr [ecx+ 0]" \
  "fld  dword ptr [ebx+ 4]" \
  "fmul dword ptr [ecx+ 4]" \
  "fxch st(1)" \
  "fld  dword ptr [ebx+ 8]" \
  "fmul dword ptr [ecx+ 8]" \
  "fxch st(2)" \
  "fsubp st(1),st" \
  "fld  dword ptr [ebx+12]" \
  "fmul dword ptr [ecx+12]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+16]" \
  "fmul dword ptr [ecx+16]" \
  "fxch st(2)" \
  "fsubp st(1),st" \
  "fld  dword ptr [ebx+20]" \
  "fmul dword ptr [ecx+20]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+24]" \
  "fmul dword ptr [ecx+24]" \
  "fxch st(2)" \
  "fsubp st(1),st" \
  "fld  dword ptr [ebx+28]" \
  "fmul dword ptr [ecx+28]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+32]" \
  "fmul dword ptr [ecx+32]" \
  "fxch st(2)" \
  "fsubp st(1),st" \
  "fld  dword ptr [ebx+36]" \
  "fmul dword ptr [ecx+36]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+40]" \
  "fmul dword ptr [ecx+40]" \
  "fxch st(2)" \
  "fsubp st(1),st" \
  "fld  dword ptr [ebx+44]" \
  "fmul dword ptr [ecx+44]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+48]" \
  "fmul dword ptr [ecx+48]" \
  "fxch st(2)" \
  "fsubp st(1),st" \
  "fld  dword ptr [ebx+52]" \
  "fmul dword ptr [ecx+52]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+56]" \
  "fmul dword ptr [ecx+56]" \
  "fxch st(2)" \
  "fsubp st(1),st" \
  "fld  dword ptr [ebx+60]" \
  "fmul dword ptr [ecx+60]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fsubrp st(1),st"

float __muladd16b(float *a, float *b);
#pragma aux __muladd16b parm [ebx] [ecx] value [8087] modify exact [8087] = \
  "fld  dword ptr [ebx+60]" \
  "fmul dword ptr [ecx+ 0]" \
  "fld  dword ptr [ebx+56]" \
  "fmul dword ptr [ecx+ 4]" \
  "fxch st(1)" \
  "fld  dword ptr [ebx+52]" \
  "fmul dword ptr [ecx+ 8]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+48]" \
  "fmul dword ptr [ecx+12]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+44]" \
  "fmul dword ptr [ecx+16]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+40]" \
  "fmul dword ptr [ecx+20]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+36]" \
  "fmul dword ptr [ecx+24]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+32]" \
  "fmul dword ptr [ecx+28]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+28]" \
  "fmul dword ptr [ecx+32]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+24]" \
  "fmul dword ptr [ecx+36]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+20]" \
  "fmul dword ptr [ecx+40]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+16]" \
  "fmul dword ptr [ecx+44]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+12]" \
  "fmul dword ptr [ecx+48]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+ 8]" \
  "fmul dword ptr [ecx+52]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+ 4]" \
  "fmul dword ptr [ecx+56]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+ 0]" \
  "fmul dword ptr [ecx+60]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "faddp st(1),st" \
  "fchs"
#endif

#ifdef GNUCI486
#define MULADDINLINE
static inline float __muladd16a(float *a, float *b)
{
  float res;
  asm
  (
    "flds   0(%1)\n"
    "fmuls  0(%2)\n"
    "flds   4(%1)\n"
    "fmuls  4(%2)\n"
    "fxch %%st(1)\n"
    "flds   8(%1)\n"
    "fmuls  8(%2)\n"
    "fxch %%st(2)\n"
    "fsubrp %%st,%%st(1)\n"
    "flds  12(%1)\n"
    "fmuls 12(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  16(%1)\n"
    "fmuls 16(%2)\n"
    "fxch %%st(2)\n"
    "fsubrp %%st,%%st(1)\n"
    "flds  20(%1)\n"
    "fmuls 20(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  24(%1)\n"
    "fmuls 24(%2)\n"
    "fxch %%st(2)\n"
    "fsubrp %%st,%%st(1)\n"
    "flds  28(%1)\n"
    "fmuls 28(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  32(%1)\n"
    "fmuls 32(%2)\n"
    "fxch %%st(2)\n"
    "fsubrp %%st,%%st(1)\n"
    "flds  36(%1)\n"
    "fmuls 36(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  40(%1)\n"
    "fmuls 40(%2)\n"
    "fxch %%st(2)\n"
    "fsubrp %%st,%%st(1)\n"
    "flds  44(%1)\n"
    "fmuls 44(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  48(%1)\n"
    "fmuls 48(%2)\n"
    "fxch %%st(2)\n"
    "fsubrp %%st,%%st(1)\n"
    "flds  52(%1)\n"
    "fmuls 52(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  56(%1)\n"
    "fmuls 56(%2)\n"
    "fxch %%st(2)\n"
    "fsubrp %%st,%%st(1)\n"
    "flds  60(%1)\n"
    "fmuls 60(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "fsubp %%st,%%st(1)"
    : "=st"(res) : "r"(a),"r"(b) : "st(7)","st(6)","st(5)"
  );
  return res;
}

static inline float __muladd16b(float *a, float *b)
{
  float res;
  asm
  (
    "flds  60(%1)\n"
    "fmuls  0(%2)\n"
    "flds  56(%1)\n"
    "fmuls  4(%2)\n"
    "fxch %%st(1)\n"
    "flds  52(%1)\n"
    "fmuls  8(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  48(%1)\n"
    "fmuls 12(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  44(%1)\n"
    "fmuls 16(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  40(%1)\n"
    "fmuls 20(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  36(%1)\n"
    "fmuls 24(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  32(%1)\n"
    "fmuls 28(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  28(%1)\n"
    "fmuls 32(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  24(%1)\n"
    "fmuls 36(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  20(%1)\n"
    "fmuls 40(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  16(%1)\n"
    "fmuls 44(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  12(%1)\n"
    "fmuls 48(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds   8(%1)\n"
    "fmuls 52(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds   4(%1)\n"
    "fmuls 56(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds   0(%1)\n"
    "fmuls 60(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "faddp %%st,%%st(1)\n"
    "fchs"
    : "=st"(res) : "r"(a),"r"(b) : "st(7)","st(6)","st(5)"
  );
  return res;
}
#endif

float ampegdecoder::muladd16a(float *a, float *b)
{
#ifdef MULADDINLINE
  return __muladd16a(a,b);
#else
  return +a[ 0]*b[ 0]-a[ 1]*b[ 1]+a[ 2]*b[ 2]-a[ 3]*b[ 3]
         +a[ 4]*b[ 4]-a[ 5]*b[ 5]+a[ 6]*b[ 6]-a[ 7]*b[ 7]
         +a[ 8]*b[ 8]-a[ 9]*b[ 9]+a[10]*b[10]-a[11]*b[11]
         +a[12]*b[12]-a[13]*b[13]+a[14]*b[14]-a[15]*b[15];
#endif
}

float ampegdecoder::muladd16b(float *a, float *b)
{
#ifdef MULADDINLINE
  return __muladd16b(a,b);
#else
  return -a[15]*b[ 0]-a[14]*b[ 1]-a[13]*b[ 2]-a[12]*b[ 3]
         -a[11]*b[ 4]-a[10]*b[ 5]-a[ 9]*b[ 6]-a[ 8]*b[ 7]
         -a[ 7]*b[ 8]-a[ 6]*b[ 9]-a[ 5]*b[10]-a[ 4]*b[11]
         -a[ 3]*b[12]-a[ 2]*b[13]-a[ 1]*b[14]-a[ 0]*b[15];
#endif
}

#ifndef FDCTBEXT
void ampegdecoder::fdctb8(float *out1, float *out2, float *in)
{
  float p1[8];
  float p2[8];

  p1[ 0] =               in[ 0] + in[ 7];
  p1[ 7] = sectab[ 4] * (in[ 0] - in[ 7]);
  p1[ 1] =               in[ 1] + in[ 6];
  p1[ 6] = sectab[ 5] * (in[ 1] - in[ 6]);
  p1[ 2] =               in[ 2] + in[ 5];
  p1[ 5] = sectab[ 6] * (in[ 2] - in[ 5]);
  p1[ 3] =               in[ 3] + in[ 4];
  p1[ 4] = sectab[ 7] * (in[ 3] - in[ 4]);

  p2[ 0] =               p1[ 0] + p1[ 3];
  p2[ 3] = sectab[ 2] * (p1[ 0] - p1[ 3]);
  p2[ 1] =               p1[ 1] + p1[ 2];
  p2[ 2] = sectab[ 3] * (p1[ 1] - p1[ 2]);
  p2[ 4] =               p1[ 4] + p1[ 7];
  p2[ 7] =-sectab[ 2] * (p1[ 4] - p1[ 7]);
  p2[ 5] =               p1[ 5] + p1[ 6];
  p2[ 6] =-sectab[ 3] * (p1[ 5] - p1[ 6]);

  p1[ 0] =               p2[ 0] + p2[ 1];
  p1[ 1] = sectab[ 1] * (p2[ 0] - p2[ 1]);
  p1[ 2] =               p2[ 2] + p2[ 3];
  p1[ 3] =-sectab[ 1] * (p2[ 2] - p2[ 3]);
  p1[ 2] += p1[ 3];
  p1[ 4] =               p2[ 4] + p2[ 5];
  p1[ 5] = sectab[ 1] * (p2[ 4] - p2[ 5]);
  p1[ 6] =               p2[ 6] + p2[ 7];
  p1[ 7] =-sectab[ 1] * (p2[ 6] - p2[ 7]);
  p1[ 6] += p1[ 7];
  p1[ 4] += p1[ 6];
  p1[ 6] += p1[ 5];
  p1[ 5] += p1[ 7];

  out1[16* 0]=p1[ 1];
  out1[16* 1]=p1[ 6];
  out1[16* 2]=p1[ 2];
  out1[16* 3]=p1[ 4];
  out1[16* 4]=p1[ 0];
  out2[16* 0]=p1[ 1];
  out2[16* 1]=p1[ 5];
  out2[16* 2]=p1[ 3];
  out2[16* 3]=p1[ 7];
}

void ampegdecoder::fdctb16(float *out1, float *out2, float *in)
{
  float p1[16];
  float p2[16];

  p2[ 0] =               in[ 0] + in[15];
  p2[15] = sectab[ 8] * (in[ 0] - in[15]);
  p2[ 1] =               in[ 1] + in[14];
  p2[14] = sectab[ 9] * (in[ 1] - in[14]);
  p2[ 2] =               in[ 2] + in[13];
  p2[13] = sectab[10] * (in[ 2] - in[13]);
  p2[ 3] =               in[ 3] + in[12];
  p2[12] = sectab[11] * (in[ 3] - in[12]);
  p2[ 4] =               in[ 4] + in[11];
  p2[11] = sectab[12] * (in[ 4] - in[11]);
  p2[ 5] =               in[ 5] + in[10];
  p2[10] = sectab[13] * (in[ 5] - in[10]);
  p2[ 6] =               in[ 6] + in[ 9];
  p2[ 9] = sectab[14] * (in[ 6] - in[ 9]);
  p2[ 7] =               in[ 7] + in[ 8];
  p2[ 8] = sectab[15] * (in[ 7] - in[ 8]);

  p1[ 0] =               p2[ 0] + p2[ 7];
  p1[ 7] = sectab[ 4] * (p2[ 0] - p2[ 7]);
  p1[ 1] =               p2[ 1] + p2[ 6];
  p1[ 6] = sectab[ 5] * (p2[ 1] - p2[ 6]);
  p1[ 2] =               p2[ 2] + p2[ 5];
  p1[ 5] = sectab[ 6] * (p2[ 2] - p2[ 5]);
  p1[ 3] =               p2[ 3] + p2[ 4];
  p1[ 4] = sectab[ 7] * (p2[ 3] - p2[ 4]);
  p1[ 8] =               p2[ 8] + p2[15];
  p1[15] =-sectab[ 4] * (p2[ 8] - p2[15]);
  p1[ 9] =               p2[ 9] + p2[14];
  p1[14] =-sectab[ 5] * (p2[ 9] - p2[14]);
  p1[10] =               p2[10] + p2[13];
  p1[13] =-sectab[ 6] * (p2[10] - p2[13]);
  p1[11] =               p2[11] + p2[12];
  p1[12] =-sectab[ 7] * (p2[11] - p2[12]);

  p2[ 0] =               p1[ 0] + p1[ 3];
  p2[ 3] = sectab[ 2] * (p1[ 0] - p1[ 3]);
  p2[ 1] =               p1[ 1] + p1[ 2];
  p2[ 2] = sectab[ 3] * (p1[ 1] - p1[ 2]);
  p2[ 4] =               p1[ 4] + p1[ 7];
  p2[ 7] =-sectab[ 2] * (p1[ 4] - p1[ 7]);
  p2[ 5] =               p1[ 5] + p1[ 6];
  p2[ 6] =-sectab[ 3] * (p1[ 5] - p1[ 6]);
  p2[ 8] =               p1[ 8] + p1[11];
  p2[11] = sectab[ 2] * (p1[ 8] - p1[11]);
  p2[ 9] =               p1[ 9] + p1[10];
  p2[10] = sectab[ 3] * (p1[ 9] - p1[10]);
  p2[12] =               p1[12] + p1[15];
  p2[15] =-sectab[ 2] * (p1[12] - p1[15]);
  p2[13] =               p1[13] + p1[14];
  p2[14] =-sectab[ 3] * (p1[13] - p1[14]);

  p1[ 0] =               p2[ 0] + p2[ 1];
  p1[ 1] = sectab[ 1] * (p2[ 0] - p2[ 1]);
  p1[ 2] =               p2[ 2] + p2[ 3];
  p1[ 3] =-sectab[ 1] * (p2[ 2] - p2[ 3]);
  p1[ 2] += p1[ 3];
  p1[ 4] =               p2[ 4] + p2[ 5];
  p1[ 5] = sectab[ 1] * (p2[ 4] - p2[ 5]);
  p1[ 6] =               p2[ 6] + p2[ 7];
  p1[ 7] =-sectab[ 1] * (p2[ 6] - p2[ 7]);
  p1[ 6] += p1[ 7];
  p1[ 4] += p1[ 6];
  p1[ 6] += p1[ 5];
  p1[ 5] += p1[ 7];
  p1[ 8] =               p2[ 8] + p2[ 9];
  p1[ 9] = sectab[ 1] * (p2[ 8] - p2[ 9]);
  p1[10] =               p2[10] + p2[11];
  p1[11] =-sectab[ 1] * (p2[10] - p2[11]);
  p1[10] += p1[11];
  p1[12] =               p2[12] + p2[13];
  p1[13] = sectab[ 1] * (p2[12] - p2[13]);
  p1[14] =               p2[14] + p2[15];
  p1[15] =-sectab[ 1] * (p2[14] - p2[15]);
  p1[14] += p1[15];
  p1[12] += p1[14];
  p1[14] += p1[13];
  p1[13] += p1[15];

  out1[16* 0]=p1[ 1];
  out1[16* 1]=p1[14]+p1[ 9];
  out1[16* 2]=p1[ 6];
  out1[16* 3]=p1[10]+p1[14];
  out1[16* 4]=p1[ 2];
  out1[16* 5]=p1[12]+p1[10];
  out1[16* 6]=p1[ 4];
  out1[16* 7]=p1[ 8]+p1[12];
  out1[16* 8]=p1[ 0];
  out2[16* 0]=p1[ 1];
  out2[16* 1]=p1[ 9]+p1[13];
  out2[16* 2]=p1[ 5];
  out2[16* 3]=p1[13]+p1[11];
  out2[16* 4]=p1[ 3];
  out2[16* 5]=p1[11]+p1[15];
  out2[16* 6]=p1[ 7];
  out2[16* 7]=p1[15];
}

void ampegdecoder::fdctb32(float *out1, float *out2, float *in)
{
  float p1[32];
  float p2[32];
  p1[ 0] =               in[0]  + in[31];
  p1[31] = sectab[16] * (in[0]  - in[31]);
  p1[ 1] =               in[1]  + in[30];
  p1[30] = sectab[17] * (in[1]  - in[30]);
  p1[ 2] =               in[2]  + in[29];
  p1[29] = sectab[18] * (in[2]  - in[29]);
  p1[ 3] =               in[3]  + in[28];
  p1[28] = sectab[19] * (in[3]  - in[28]);
  p1[ 4] =               in[4]  + in[27];
  p1[27] = sectab[20] * (in[4]  - in[27]);
  p1[ 5] =               in[5]  + in[26];
  p1[26] = sectab[21] * (in[5]  - in[26]);
  p1[ 6] =               in[6]  + in[25];
  p1[25] = sectab[22] * (in[6]  - in[25]);
  p1[ 7] =               in[7]  + in[24];
  p1[24] = sectab[23] * (in[7]  - in[24]);
  p1[ 8] =               in[8]  + in[23];
  p1[23] = sectab[24] * (in[8]  - in[23]);
  p1[ 9] =               in[9]  + in[22];
  p1[22] = sectab[25] * (in[9]  - in[22]);
  p1[10] =               in[10] + in[21];
  p1[21] = sectab[26] * (in[10] - in[21]);
  p1[11] =               in[11] + in[20];
  p1[20] = sectab[27] * (in[11] - in[20]);
  p1[12] =               in[12] + in[19];
  p1[19] = sectab[28] * (in[12] - in[19]);
  p1[13] =               in[13] + in[18];
  p1[18] = sectab[29] * (in[13] - in[18]);
  p1[14] =               in[14] + in[17];
  p1[17] = sectab[30] * (in[14] - in[17]);
  p1[15] =               in[15] + in[16];
  p1[16] = sectab[31] * (in[15] - in[16]);

  p2[ 0] =               p1[ 0] + p1[15];
  p2[15] = sectab[ 8] * (p1[ 0] - p1[15]);
  p2[ 1] =               p1[ 1] + p1[14];
  p2[14] = sectab[ 9] * (p1[ 1] - p1[14]);
  p2[ 2] =               p1[ 2] + p1[13];
  p2[13] = sectab[10] * (p1[ 2] - p1[13]);
  p2[ 3] =               p1[ 3] + p1[12];
  p2[12] = sectab[11] * (p1[ 3] - p1[12]);
  p2[ 4] =               p1[ 4] + p1[11];
  p2[11] = sectab[12] * (p1[ 4] - p1[11]);
  p2[ 5] =               p1[ 5] + p1[10];
  p2[10] = sectab[13] * (p1[ 5] - p1[10]);
  p2[ 6] =               p1[ 6] + p1[ 9];
  p2[ 9] = sectab[14] * (p1[ 6] - p1[ 9]);
  p2[ 7] =               p1[ 7] + p1[ 8];
  p2[ 8] = sectab[15] * (p1[ 7] - p1[ 8]);
  p2[16] =               p1[16] + p1[31];
  p2[31] =-sectab[ 8] * (p1[16] - p1[31]);
  p2[17] =               p1[17] + p1[30];
  p2[30] =-sectab[ 9] * (p1[17] - p1[30]);
  p2[18] =               p1[18] + p1[29];
  p2[29] =-sectab[10] * (p1[18] - p1[29]);
  p2[19] =               p1[19] + p1[28];
  p2[28] =-sectab[11] * (p1[19] - p1[28]);
  p2[20] =               p1[20] + p1[27];
  p2[27] =-sectab[12] * (p1[20] - p1[27]);
  p2[21] =               p1[21] + p1[26];
  p2[26] =-sectab[13] * (p1[21] - p1[26]);
  p2[22] =               p1[22] + p1[25];
  p2[25] =-sectab[14] * (p1[22] - p1[25]);
  p2[23] =               p1[23] + p1[24];
  p2[24] =-sectab[15] * (p1[23] - p1[24]);

  p1[ 0] =               p2[ 0] + p2[ 7];
  p1[ 7] = sectab[ 4] * (p2[ 0] - p2[ 7]);
  p1[ 1] =               p2[ 1] + p2[ 6];
  p1[ 6] = sectab[ 5] * (p2[ 1] - p2[ 6]);
  p1[ 2] =               p2[ 2] + p2[ 5];
  p1[ 5] = sectab[ 6] * (p2[ 2] - p2[ 5]);
  p1[ 3] =               p2[ 3] + p2[ 4];
  p1[ 4] = sectab[ 7] * (p2[ 3] - p2[ 4]);
  p1[ 8] =               p2[ 8] + p2[15];
  p1[15] =-sectab[ 4] * (p2[ 8] - p2[15]);
  p1[ 9] =               p2[ 9] + p2[14];
  p1[14] =-sectab[ 5] * (p2[ 9] - p2[14]);
  p1[10] =               p2[10] + p2[13];
  p1[13] =-sectab[ 6] * (p2[10] - p2[13]);
  p1[11] =               p2[11] + p2[12];
  p1[12] =-sectab[ 7] * (p2[11] - p2[12]);
  p1[16] =               p2[16] + p2[23];
  p1[23] = sectab[ 4] * (p2[16] - p2[23]);
  p1[17] =               p2[17] + p2[22];
  p1[22] = sectab[ 5] * (p2[17] - p2[22]);
  p1[18] =               p2[18] + p2[21];
  p1[21] = sectab[ 6] * (p2[18] - p2[21]);
  p1[19] =               p2[19] + p2[20];
  p1[20] = sectab[ 7] * (p2[19] - p2[20]);
  p1[24] =               p2[24] + p2[31];
  p1[31] =-sectab[ 4] * (p2[24] - p2[31]);
  p1[25] =               p2[25] + p2[30];
  p1[30] =-sectab[ 5] * (p2[25] - p2[30]);
  p1[26] =               p2[26] + p2[29];
  p1[29] =-sectab[ 6] * (p2[26] - p2[29]);
  p1[27] =               p2[27] + p2[28];
  p1[28] =-sectab[ 7] * (p2[27] - p2[28]);

  p2[ 0] =               p1[ 0] + p1[ 3];
  p2[ 3] = sectab[ 2] * (p1[ 0] - p1[ 3]);
  p2[ 1] =               p1[ 1] + p1[ 2];
  p2[ 2] = sectab[ 3] * (p1[ 1] - p1[ 2]);
  p2[ 4] =               p1[ 4] + p1[ 7];
  p2[ 7] =-sectab[ 2] * (p1[ 4] - p1[ 7]);
  p2[ 5] =               p1[ 5] + p1[ 6];
  p2[ 6] =-sectab[ 3] * (p1[ 5] - p1[ 6]);
  p2[ 8] =               p1[ 8] + p1[11];
  p2[11] = sectab[ 2] * (p1[ 8] - p1[11]);
  p2[ 9] =               p1[ 9] + p1[10];
  p2[10] = sectab[ 3] * (p1[ 9] - p1[10]);
  p2[12] =               p1[12] + p1[15];
  p2[15] =-sectab[ 2] * (p1[12] - p1[15]);
  p2[13] =               p1[13] + p1[14];
  p2[14] =-sectab[ 3] * (p1[13] - p1[14]);
  p2[16] =               p1[16] + p1[19];
  p2[19] = sectab[ 2] * (p1[16] - p1[19]);
  p2[17] =               p1[17] + p1[18];
  p2[18] = sectab[ 3] * (p1[17] - p1[18]);
  p2[20] =               p1[20] + p1[23];
  p2[23] =-sectab[ 2] * (p1[20] - p1[23]);
  p2[21] =               p1[21] + p1[22];
  p2[22] =-sectab[ 3] * (p1[21] - p1[22]);
  p2[24] =               p1[24] + p1[27];
  p2[27] = sectab[ 2] * (p1[24] - p1[27]);
  p2[25] =               p1[25] + p1[26];
  p2[26] = sectab[ 3] * (p1[25] - p1[26]);
  p2[28] =               p1[28] + p1[31];
  p2[31] =-sectab[ 2] * (p1[28] - p1[31]);
  p2[29] =               p1[29] + p1[30];
  p2[30] =-sectab[ 3] * (p1[29] - p1[30]);

  p1[ 0] =               p2[ 0] + p2[ 1];
  p1[ 1] = sectab[ 1] * (p2[ 0] - p2[ 1]);
  p1[ 2] =               p2[ 2] + p2[ 3];
  p1[ 3] =-sectab[ 1] * (p2[ 2] - p2[ 3]);
  p1[ 2] += p1[ 3];
  p1[ 4] =               p2[ 4] + p2[ 5];
  p1[ 5] = sectab[ 1] * (p2[ 4] - p2[ 5]);
  p1[ 6] =               p2[ 6] + p2[ 7];
  p1[ 7] =-sectab[ 1] * (p2[ 6] - p2[ 7]);
  p1[ 6] += p1[ 7];
  p1[ 4] += p1[ 6];
  p1[ 6] += p1[ 5];
  p1[ 5] += p1[ 7];
  p1[ 8] =               p2[ 8] + p2[ 9];
  p1[ 9] = sectab[ 1] * (p2[ 8] - p2[ 9]);
  p1[10] =               p2[10] + p2[11];
  p1[11] =-sectab[ 1] * (p2[10] - p2[11]);
  p1[10] += p1[11];
  p1[12] =               p2[12] + p2[13];
  p1[13] = sectab[ 1] * (p2[12] - p2[13]);
  p1[14] =               p2[14] + p2[15];
  p1[15] =-sectab[ 1] * (p2[14] - p2[15]);
  p1[14] += p1[15];
  p1[12] += p1[14];
  p1[14] += p1[13];
  p1[13] += p1[15];
  p1[16] =               p2[16] + p2[17];
  p1[17] = sectab[ 1] * (p2[16] - p2[17]);
  p1[18] =               p2[18] + p2[19];
  p1[19] =-sectab[ 1] * (p2[18] - p2[19]);
  p1[18] += p1[19];
  p1[20] =               p2[20] + p2[21];
  p1[21] = sectab[ 1] * (p2[20] - p2[21]);
  p1[22] =               p2[22] + p2[23];
  p1[23] =-sectab[ 1] * (p2[22] - p2[23]);
  p1[22] += p1[23];
  p1[20] += p1[22];
  p1[22] += p1[21];
  p1[21] += p1[23];
  p1[24] =               p2[24] + p2[25];
  p1[25] = sectab[ 1] * (p2[24] - p2[25]);
  p1[26] =               p2[26] + p2[27];
  p1[27] =-sectab[ 1] * (p2[26] - p2[27]);
  p1[26] += p1[27];
  p1[28] =               p2[28] + p2[29];
  p1[29] = sectab[ 1] * (p2[28] - p2[29]);
  p1[30] =               p2[30] + p2[31];
  p1[31] =-sectab[ 1] * (p2[30] - p2[31]);
  p1[30] += p1[31];
  p1[28] += p1[30];
  p1[30] += p1[29];
  p1[29] += p1[31];

  out1[16* 0] = p1[ 1];
  out1[16* 1] = p1[17] + p1[30] + p1[25];
  out1[16* 2] = p1[14] + p1[ 9];
  out1[16* 3] = p1[22] + p1[30] + p1[25];
  out1[16* 4] = p1[ 6];
  out1[16* 5] = p1[22] + p1[26] + p1[30];
  out1[16* 6] = p1[10] + p1[14];
  out1[16* 7] = p1[18] + p1[26] + p1[30];
  out1[16* 8] = p1[ 2];
  out1[16* 9] = p1[18] + p1[28] + p1[26];
  out1[16*10] = p1[12] + p1[10];
  out1[16*11] = p1[20] + p1[28] + p1[26];
  out1[16*12] = p1[ 4];
  out1[16*13] = p1[20] + p1[24] + p1[28];
  out1[16*14] = p1[ 8] + p1[12];
  out1[16*15] = p1[16] + p1[24] + p1[28];
  out1[16*16] = p1[ 0];
  out2[16* 0] = p1[ 1];
  out2[16* 1] = p1[17] + p1[25] + p1[29];
  out2[16* 2] = p1[ 9] + p1[13];
  out2[16* 3] = p1[21] + p1[25] + p1[29];
  out2[16* 4] = p1[ 5];
  out2[16* 5] = p1[21] + p1[29] + p1[27];
  out2[16* 6] = p1[13] + p1[11];
  out2[16* 7] = p1[19] + p1[29] + p1[27];
  out2[16* 8] = p1[ 3];
  out2[16* 9] = p1[19] + p1[27] + p1[31];
  out2[16*10] = p1[11] + p1[15];
  out2[16*11] = p1[23] + p1[27] + p1[31];
  out2[16*12] = p1[ 7];
  out2[16*13] = p1[23] + p1[31];
  out2[16*14] = p1[15];
  out2[16*15] = p1[31];
}
#endif

void ampegdecoder::synth(void *dest, float (*bandsl)[32], float (*bandsr)[32])
{
  int blk;
  for (blk=0; blk<36; blk++)
  {
    int i,j,k;

    int nsmp=32>>ratereduce;
    int nsmp2=16>>ratereduce;

    if (usevoltab)
    {
      if (tomono)
        for (i=0; i<nsmp; i++)
          bandsl[0][i]=bandsl[0][i]*stereotab[2][0]+bandsr[0][i]*stereotab[2][1];
      else
      if (srcchan==2)
        for (i=0; i<nsmp; i++)
        {
          float t=bandsl[0][i];
          bandsl[0][i]=bandsl[0][i]*stereotab[0][0]+bandsr[0][i]*stereotab[0][1];
          bandsr[0][i]=t*stereotab[1][0]+bandsr[0][i]*stereotab[1][1];
        }
      else
        for (i=0; i<nsmp; i++)
          bandsl[0][i]*=stereotab[2][2];
    }
    else
      if (tomono)
        for (i=0; i<nsmp; i++)
          bandsl[0][i]=0.5*(bandsl[0][i]+bandsr[0][i]);
    if (volume!=1)
    {
      for (i=0; i<nsmp; i++)
        bandsl[0][i]*=volume;
      if (!tomono&&(srcchan!=1))
        for (i=0; i<nsmp; i++)
          bandsr[0][i]*=volume;
    }
    if (equalon)
    {
      for (i=0; i<nsmp; i++)
        bandsl[0][i]*=equal[i];
      if (!tomono&&(srcchan!=1))
        for (i=0; i<nsmp; i++)
          bandsr[0][i]*=equal[i];
    }

    for (k=0; k<dctstereo; k++)
    {
      float *out1=synbuf+k*2*BUFFEROFFSET+(synbufoffset&1)*BUFFEROFFSET+((synbufoffset+1)&14);
      float *out2=synbuf+k*2*BUFFEROFFSET+((synbufoffset+1)&1)*BUFFEROFFSET+(synbufoffset|1);

      if (ratereduce==0)
        fdctb32(out1, out2, k?bandsr[0]:bandsl[0]);
      else
      if (ratereduce==1)
        fdctb16(out1, out2, k?bandsr[0]:bandsl[0]);
      else
        fdctb8(out1, out2, k?bandsr[0]:bandsl[0]);
    }

    bandsl++;
    bandsr++;

    float *in=synbuf+((synbufoffset+1)&1)*BUFFEROFFSET;
    float *dw1=((ratereduce==0)?dwin:(ratereduce==1)?dwin2:dwin4)+16-(synbufoffset|1);
    float *dw2=dw1-16+2*(synbufoffset|1);
    synbufoffset = (synbufoffset-1)&15;
    if (!dest)
      continue;

    if (samplesize==2)
    {
      short *samples=(short*)dest;
      if (!tostereo)
        if (dstchan==2)
          for (j=0; j<=nsmp2; j++, dw1+=32, dw2+=32, in+=16)
          {
            samples[2*j+0]=cliptoshort(muladd16a(dw1,in));
            samples[2*j+1]=cliptoshort(muladd16a(dw1,in+BUFFEROFFSET*2));
            if (!j||(j==nsmp2))
              continue;
            samples[2*2*nsmp2-2*j+0]=cliptoshort(muladd16b(dw2,in));
            samples[2*2*nsmp2-2*j+1]=cliptoshort(muladd16b(dw2,in+BUFFEROFFSET*2));
          }
        else
          for (j=0; j<=nsmp2; j++, dw1+=32, dw2+=32, in+=16)
          {
            samples[j]=cliptoshort(muladd16a(dw1,in));
            if (!j||(j==nsmp2))
              continue;
            samples[2*nsmp2-j]=cliptoshort(muladd16b(dw2,in));
          }
      else
        if (!usevoltab)
          for (j=0; j<=nsmp2; j++, dw1+=32, dw2+=32, in+=16)
          {
            samples[2*j+0]=samples[2*j+1]=cliptoshort(muladd16a(dw1,in));
            if (!j||(j==nsmp2))
              continue;
            samples[2*2*nsmp2-2*j+0]=samples[2*2*nsmp2-2*j+1]=cliptoshort(muladd16b(dw2,in));
          }
        else
          for (j=0; j<=nsmp2; j++, dw1+=32, dw2+=32, in+=16)
          {
            double sum=muladd16a(dw1,in);
            samples[2*j+0]=cliptoshort(sum*stereotab[0][2]);
            samples[2*j+1]=cliptoshort(sum*stereotab[1][2]);
            if (!j||(j==nsmp2))
              continue;
            sum=muladd16b(dw2,in);
            samples[2*2*nsmp2-2*j+0]=cliptoshort(sum*stereotab[0][2]);
            samples[2*2*nsmp2-2*j+1]=cliptoshort(sum*stereotab[1][2]);
          }
    }
    else
    {
      float *samples=(float*)dest;
      if (!tostereo)
        if (dstchan==2)
          for (j=0; j<=nsmp2; j++, dw1+=32, dw2+=32, in+=16)
          {
            samples[2*j+0]=muladd16a(dw1,in);
            samples[2*j+1]=muladd16a(dw1,in+BUFFEROFFSET*2);
            if (!j||(j==nsmp2))
              continue;
            samples[2*2*nsmp2-2*j+0]=muladd16b(dw2,in);
            samples[2*2*nsmp2-2*j+1]=muladd16b(dw2,in+BUFFEROFFSET*2);
          }
        else
          for (j=0; j<=nsmp2; j++, dw1+=32, dw2+=32, in+=16)
          {
            samples[j]=muladd16a(dw1,in);
            if (!j||(j==nsmp2))
              continue;
            samples[2*nsmp2-j]=muladd16b(dw2,in);
          }
      else
        if (!usevoltab)
          for (j=0; j<=nsmp2; j++, dw1+=32, dw2+=32, in+=16)
          {
            samples[2*j+0]=samples[2*j+1]=muladd16a(dw1,in);
            if (!j||(j==nsmp2))
              continue;
            samples[2*2*nsmp2-2*j+0]=samples[2*2*nsmp2-2*j+1]=muladd16b(dw2,in);
          }
        else
          for (j=0; j<=nsmp2; j++, dw1+=32, dw2+=32, in+=16)
          {
            double sum=muladd16a(dw1,in);
            samples[2*j+0]=sum*stereotab[0][2];
            samples[2*j+1]=sum*stereotab[1][2];
            if (!j||(j==nsmp2))
              continue;
            sum=muladd16b(dw2,in);
            samples[2*2*nsmp2-2*j+0]=sum*stereotab[0][2];
            samples[2*2*nsmp2-2*j+1]=sum*stereotab[1][2];
          }
    }
    dest=((char*)dest)+samplesize*dstchan*(32>>ratereduce);
  }
}

void ampegdecoder::resetsynth()
{
  int i;
  synbufoffset=0;
  for (i=0; i<(BUFFEROFFSET*4); i++)
    synbuf[i]=0;
}

int ampegdecoder::opensynth()
{
  int i,j;
  resetsynth();
  float dwincc[8];
//  dwincc[5]=-0.342943448;   //   2.739098988
//  dwincc[6]=0.086573376;    //  11.50752718
//  dwincc[7]=-0.00773018993; // 129.3590624
  dwincc[5]=-0.341712191984f;
  dwincc[6]=0.0866307578916f;
  dwincc[7]=-0.00849728985506f;
  dwincc[0]=0.5;
  dwincc[1]=-sqrt(1-dwincc[7]*dwincc[7]);
  dwincc[2]=sqrt(1-dwincc[6]*dwincc[6]);
  dwincc[3]=-sqrt(1-dwincc[5]*dwincc[5]);
  dwincc[4]=sqrt(0.5);
  for (i=0; i<1024; i++)
  {
    double v=0;
    for (j=0; j<8; j++)
      v+=cos(2*_PI/512*((i<<5)+(i>>5))*j)*dwincc[j]*8192;
    dwin[i]=(i&2)?-v:v;
  }
  for (i=0; i<512; i++)
    dwin2[i]=dwin[(i&31)+((i&~31)<<1)];
  for (i=0; i<256; i++)
    dwin4[i]=dwin[(i&31)+((i&~31)<<2)];

  for (j=0; j<5; j++)
    for (i=0; i<(1<<j); i++)
      sectab[i+(1<<j)]=0.5/cos(_PI*(2*i+1)/(4<<j));

  if (!dstchan)
    dstchan=srcchan;
  tomono=0;
  tostereo=0;
  if (dstchan==-2)
  {
    dstchan=2;
    tostereo=1;
    if (srcchan==2)
      tomono=1;
  }
  if ((srcchan==2)&&(dstchan==1))
    tomono=1;
  if ((srcchan==1)&&(dstchan==2))
    tostereo=1;
  dctstereo=(tomono||tostereo||(srcchan==1))?1:2;
  usevoltab=0;
  volume=1;
  equalon=0;

  return 1;
}

void ampegdecoder::setvol(float v)
{
  volume=v;
}

void ampegdecoder::setstereo(const float *v)
{
  if (!v)
  {
    usevoltab=0;
    return;
  }
  if ((v[0]==1)&&(v[1]==0)&&(v[2]==1)&&(v[3]==0)&&(v[4]==1)&&(v[5]==1)&&(v[6]==0.5)&&(v[7]==0.5)&&(v[8]==1))
  {
    usevoltab=0;
    return;
  }
  stereotab[0][0]=v[0];
  stereotab[0][1]=v[1];
  stereotab[0][2]=v[2];
  stereotab[1][0]=v[3];
  stereotab[1][1]=v[4];
  stereotab[1][2]=v[5];
  stereotab[2][0]=v[6];
  stereotab[2][1]=v[7];
  stereotab[2][2]=v[8];
  usevoltab=1;
}

void ampegdecoder::setequal(const float *buf)
{
  if (!buf)
  {
    equalon=0;
    return;
  }
  int i;
  for (i=0; i<32; i++)
    if (buf[i]!=1)
      break;
  if (i==32)
  {
    equalon=0;
    return;
  }
  if (ratereduce==0)
    for (i=0; i<32; i++)
      equal[i]=buf[i];
  else
  if (ratereduce==1)
    for (i=0; i<16; i++)
      equal[i]=(buf[2*i+0]+buf[2*i+1])/2;
  else
    for (i=0; i<8; i++)
      equal[i]=(buf[4*i+0]+buf[4*i+1]+buf[4*i+2]+buf[4*i+3])/4;
  equalon=1;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/


/****************************************************************************/
/***                                                                      ***/
/***   mp3-decoder                                                        ***/
/***                                                                      ***/
/****************************************************************************/


sMP3Decoder::sMP3Decoder()
{
  Buffer = 0;
  dec = 0;

  Buffer = new sU32[16384];
  dec = new ampegdecoder;
}

/****************************************************************************/

sMP3Decoder::~sMP3Decoder()
{
  if(Buffer)
    delete[] Buffer;
  if(dec)
    delete dec;
}

/****************************************************************************/

sBool sMP3Decoder::Init(sInt songnr)
{
  dec->Start(Stream,StreamSize);
  Size = 0;
  Index = 0;
  return sTRUE;
}


/****************************************************************************/

sInt sMP3Decoder::Render(sS16 *stream,sInt samples)
{
  sInt count,samples0;
  sU32 *d;

  samples0 = samples;
  d = (sU32 *) stream;

  while(samples>0)
  {
    if(Index==Size)
    {
      Index = 0;
      Size = dec->DecodeFrame(Buffer)/4;
    }
    if(Size==0)
      break;

    count = sMin(samples,Size-Index);

    sCopyMem4(d,Buffer+Index,count);
    d+=count;
    samples-=count;
    Index+=count;
  }
  if(samples>0)
    sSetMem4(d,0,samples);
  return samples0;
}

/****************************************************************************/
/****************************************************************************/

sInt sMP3Decoder::DecodeBlock()
{
  if(Stream)
    return dec->DecodeFrame(Buffer)/4;
  else
    return 0;
}


#endif

