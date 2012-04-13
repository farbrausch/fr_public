// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_ogg.hpp"

#if sLINK_OGG
/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   standard ogg stuff                                                 ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

#ifdef _DEBUG
#pragma comment(lib,sLIBPATH"_ogg1d.lib")
#pragma comment(lib,sLIBPATH"_ogg2d.lib")
#else
#pragma comment(lib,sLIBPATH"_ogg1r.lib")
#pragma comment(lib,sLIBPATH"_ogg2r.lib")
#endif

#ifndef _OS_TYPES_H
#define _OS_TYPES_H


#define _ogg_malloc  malloc
#define _ogg_calloc  calloc
#define _ogg_realloc realloc
#define _ogg_free    free


typedef __int64 ogg_int64_t;
typedef __int32 ogg_int32_t;
typedef unsigned __int32 ogg_uint32_t;
typedef __int16 ogg_int16_t;

#endif

/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

  function: packing variable sized words into an octet stream
  last mod: $Id: bitwise.c,v 1.10 2001/12/21 14:41:15 segher Exp $

 ********************************************************************/

/* We're 'LSb' endian; if we write a word but read individual bits,
   then we'll read the lsb first */


#ifndef _OGG_H
#define _OGG_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  long endbyte;
  int  endbit;

  unsigned char *buffer;
  unsigned char *ptr;
  long storage;
} oggpack_buffer;

/* ogg_page is used to encapsulate the data in one Ogg bitstream page *****/

typedef struct {
  unsigned char *header;
  long header_len;
  unsigned char *body;
  long body_len;
} ogg_page;

/* ogg_stream_state contains the current encode/decode state of a logical
   Ogg bitstream **********************************************************/

typedef struct {
  unsigned char   *body_data;    /* bytes from packet bodies */
  long    body_storage;          /* storage elements allocated */
  long    body_fill;             /* elements stored; fill mark */
  long    body_returned;         /* elements of fill returned */


  int     *lacing_vals;      /* The values that will go to the segment table */
  ogg_int64_t *granule_vals; /* granulepos values for headers. Not compact
				this way, but it is simple coupled to the
				lacing fifo */
  long    lacing_storage;
  long    lacing_fill;
  long    lacing_packet;
  long    lacing_returned;

  unsigned char    header[282];      /* working space for header encode */
  int              header_fill;

  int     e_o_s;          /* set when we have buffered the last packet in the
                             logical bitstream */
  int     b_o_s;          /* set after we've written the initial page
                             of a logical bitstream */
  long    serialno;
  long    pageno;
  ogg_int64_t  packetno;      /* sequence number for decode; the framing
                             knows where there's a hole in the data,
                             but we need coupling so that the codec
                             (which is in a seperate abstraction
                             layer) also knows about the gap */
  ogg_int64_t   granulepos;

} ogg_stream_state;

/* ogg_packet is used to encapsulate the data and metadata belonging
   to a single raw Ogg/Vorbis packet *************************************/

typedef struct {
  unsigned char *packet;
  long  bytes;
  long  b_o_s;
  long  e_o_s;

  ogg_int64_t  granulepos;
  
  ogg_int64_t  packetno;     /* sequence number for decode; the framing
				knows where there's a hole in the data,
				but we need coupling so that the codec
				(which is in a seperate abstraction
				layer) also knows about the gap */
} ogg_packet;

typedef struct {
  unsigned char *data;
  int storage;
  int fill;
  int returned;

  int unsynced;
  int headerbytes;
  int bodybytes;
} ogg_sync_state;

/* Ogg BITSTREAM PRIMITIVES: bitstream ************************/

extern void  __cdecl oggpack_writeinit(oggpack_buffer *b);
extern void  __cdecl oggpack_reset(oggpack_buffer *b);
extern void  __cdecl oggpack_writeclear(oggpack_buffer *b);
extern void  __cdecl oggpack_readinit(oggpack_buffer *b,unsigned char *buf,int bytes);
extern void  __cdecl oggpack_write(oggpack_buffer *b,unsigned long value,int bits);
extern long  __cdecl oggpack_look(oggpack_buffer *b,int bits);
extern long  __cdecl oggpack_look_huff(oggpack_buffer *b,int bits);
extern long  __cdecl oggpack_look1(oggpack_buffer *b);
extern void  __cdecl oggpack_adv(oggpack_buffer *b,int bits);
extern int   __cdecl oggpack_adv_huff(oggpack_buffer *b,int bits);
extern void  __cdecl oggpack_adv1(oggpack_buffer *b);
extern long  __cdecl oggpack_read(oggpack_buffer *b,int bits);
extern long  __cdecl oggpack_read1(oggpack_buffer *b);
extern long  __cdecl oggpack_bytes(oggpack_buffer *b);
extern long  __cdecl oggpack_bits(oggpack_buffer *b);
extern unsigned char *__cdecl oggpack_get_buffer(oggpack_buffer *b);

/* Ogg BITSTREAM PRIMITIVES: encoding **************************/

extern int      __cdecl ogg_stream_packetin(ogg_stream_state *os, ogg_packet *op);
extern int      __cdecl ogg_stream_pageout(ogg_stream_state *os, ogg_page *og);
extern int      __cdecl ogg_stream_flush(ogg_stream_state *os, ogg_page *og);

/* Ogg BITSTREAM PRIMITIVES: decoding **************************/

extern int      __cdecl ogg_sync_init(ogg_sync_state *oy);
extern int      __cdecl ogg_sync_clear(ogg_sync_state *oy);
extern int      __cdecl ogg_sync_reset(ogg_sync_state *oy);
extern int	    __cdecl ogg_sync_destroy(ogg_sync_state *oy);

extern char    *__cdecl ogg_sync_buffer(ogg_sync_state *oy, long size);
extern int      __cdecl ogg_sync_wrote(ogg_sync_state *oy, long bytes);
extern long     __cdecl ogg_sync_pageseek(ogg_sync_state *oy,ogg_page *og);
extern int      __cdecl ogg_sync_pageout(ogg_sync_state *oy, ogg_page *og);
extern int      __cdecl ogg_stream_pagein(ogg_stream_state *os, ogg_page *og);
extern int      __cdecl ogg_stream_packetout(ogg_stream_state *os,ogg_packet *op);
extern int      __cdecl ogg_stream_packetpeek(ogg_stream_state *os,ogg_packet *op);

/* Ogg BITSTREAM PRIMITIVES: general ***************************/

extern int      __cdecl ogg_stream_init(ogg_stream_state *os,int serialno);
extern int      __cdecl ogg_stream_clear(ogg_stream_state *os);
extern int      __cdecl ogg_stream_reset(ogg_stream_state *os);
extern int      __cdecl ogg_stream_destroy(ogg_stream_state *os);
extern int      __cdecl ogg_stream_eos(ogg_stream_state *os);

extern void     __cdecl ogg_page_checksum_set(ogg_page *og);

extern int      __cdecl ogg_page_version(ogg_page *og);
extern int      __cdecl ogg_page_continued(ogg_page *og);
extern int      __cdecl ogg_page_bos(ogg_page *og);
extern int      __cdecl ogg_page_eos(ogg_page *og);
extern ogg_int64_t  __cdecl ogg_page_granulepos(ogg_page *og);
extern int      __cdecl ogg_page_serialno(ogg_page *og);
extern long     __cdecl ogg_page_pageno(ogg_page *og);
extern int      __cdecl ogg_page_packets(ogg_page *og);

extern void     __cdecl ogg_packet_clear(ogg_packet *op);


#ifdef __cplusplus
}
#endif

#endif  /* _OGG_H */

/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *

 ********************************************************************

 function: libvorbis codec headers
 last mod: $Id: codec.h,v 1.39 2001/12/12 09:45:23 xiphmont Exp $

 ********************************************************************/

#ifndef _vorbis_codec_h_
#define _vorbis_codec_h_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


typedef struct vorbis_info{
  int version;
  int channels;
  long rate;

  /* The below bitrate declarations are *hints*.
     Combinations of the three values carry the following implications:
     
     all three set to the same value: 
       implies a fixed rate bitstream
     only nominal set: 
       implies a VBR stream that averages the nominal bitrate.  No hard 
       upper/lower limit
     upper and or lower set: 
       implies a VBR bitstream that obeys the bitrate limits. nominal 
       may also be set to give a nominal rate.
     none set:
       the coder does not care to speculate.
  */

  long bitrate_upper;
  long bitrate_nominal;
  long bitrate_lower;
  long bitrate_window;

  void *codec_setup;
} vorbis_info;

/* vorbis_dsp_state buffers the current vorbis audio
   analysis/synthesis state.  The DSP state belongs to a specific
   logical bitstream ****************************************************/
typedef struct vorbis_dsp_state{
  int analysisp;
  vorbis_info *vi;

  float **pcm;
  float **pcmret;
  int      pcm_storage;
  int      pcm_current;
  int      pcm_returned;

  int  preextrapolate;
  int  eofflag;

  long lW;
  long W;
  long nW;
  long centerW;

  ogg_int64_t granulepos;
  ogg_int64_t sequence;

  ogg_int64_t glue_bits;
  ogg_int64_t time_bits;
  ogg_int64_t floor_bits;
  ogg_int64_t res_bits;

  void       *backend_state;
} vorbis_dsp_state;

typedef struct vorbis_block{
  /* necessary stream state for linking to the framing abstraction */
  float  **pcm;       /* this is a pointer into local storage */ 
  oggpack_buffer opb;
  
  long  lW;
  long  W;
  long  nW;
  int   pcmend;
  int   mode;

  int         eofflag;
  ogg_int64_t granulepos;
  ogg_int64_t sequence;
  vorbis_dsp_state *vd; /* For read-only access of configuration */

  /* local storage to avoid remallocing; it's up to the mapping to
     structure it */
  void               *localstore;
  long                localtop;
  long                localalloc;
  long                totaluse;
  struct alloc_chain *reap;

  /* bitmetrics for the frame */
  long glue_bits;
  long time_bits;
  long floor_bits;
  long res_bits;

  void *internal;

} vorbis_block;

/* vorbis_block is a single block of data to be processed as part of
the analysis/synthesis stream; it belongs to a specific logical
bitstream, but is independant from other vorbis_blocks belonging to
that logical bitstream. *************************************************/

struct alloc_chain{
  void *ptr;
  struct alloc_chain *next;
};

/* vorbis_info contains all the setup information specific to the
   specific compression/decompression mode in progress (eg,
   psychoacoustic settings, channel setup, options, codebook
   etc). vorbis_info and substructures are in backends.h.
*********************************************************************/

/* the comments are not part of vorbis_info so that vorbis_info can be
   static storage */
typedef struct vorbis_comment{
  /* unlimited user comment fields.  libvorbis writes 'libvorbis'
     whatever vendor is set to in encode */
  char **user_comments;
  int   *comment_lengths;
  int    comments;
  char  *vendor;

} vorbis_comment;


/* libvorbis encodes in two abstraction layers; first we perform DSP
   and produce a packet (see docs/analysis.txt).  The packet is then
   coded into a framed OggSquish bitstream by the second layer (see
   docs/framing.txt).  Decode is the reverse process; we sync/frame
   the bitstream and extract individual packets, then decode the
   packet back into PCM audio.

   The extra framing/packetizing is used in streaming formats, such as
   files.  Over the net (such as with UDP), the framing and
   packetization aren't necessary as they're provided by the transport
   and the streaming layer is not used */

/* Vorbis PRIMITIVES: general ***************************************/

extern void     __cdecl vorbis_info_init(vorbis_info *vi);
extern void     __cdecl vorbis_info_clear(vorbis_info *vi);
extern int      __cdecl vorbis_info_blocksize(vorbis_info *vi,int zo);
extern void     __cdecl vorbis_comment_init(vorbis_comment *vc);
extern void     __cdecl vorbis_comment_add(vorbis_comment *vc, char *comment); 
extern void     __cdecl vorbis_comment_add_tag(vorbis_comment *vc, 
				       char *tag, char *contents);
extern char    *__cdecl vorbis_comment_query(vorbis_comment *vc, char *tag, int count);
extern int      __cdecl vorbis_comment_query_count(vorbis_comment *vc, char *tag);
extern void     __cdecl vorbis_comment_clear(vorbis_comment *vc);

extern int      __cdecl vorbis_block_init(vorbis_dsp_state *v, vorbis_block *vb);
extern int      __cdecl vorbis_block_clear(vorbis_block *vb);
extern void     __cdecl vorbis_dsp_clear(vorbis_dsp_state *v);

/* Vorbis PRIMITIVES: analysis/DSP layer ****************************/

extern int      __cdecl vorbis_analysis_init(vorbis_dsp_state *v,vorbis_info *vi);
extern int      __cdecl vorbis_commentheader_out(vorbis_comment *vc, ogg_packet *op);
extern int      __cdecl vorbis_analysis_headerout(vorbis_dsp_state *v,
					  vorbis_comment *vc,
					  ogg_packet *op,
					  ogg_packet *op_comm,
					  ogg_packet *op_code);
extern float  **__cdecl vorbis_analysis_buffer(vorbis_dsp_state *v,int vals);
extern int      __cdecl vorbis_analysis_wrote(vorbis_dsp_state *v,int vals);
extern int      __cdecl vorbis_analysis_blockout(vorbis_dsp_state *v,vorbis_block *vb);
extern int      __cdecl vorbis_analysis(vorbis_block *vb,ogg_packet *op);

extern int      __cdecl vorbis_bitrate_addblock(vorbis_block *vb);
extern int      __cdecl vorbis_bitrate_flushpacket(vorbis_dsp_state *vd,
					   ogg_packet *op);

/* Vorbis PRIMITIVES: synthesis layer *******************************/
extern int      __cdecl vorbis_synthesis_headerin(vorbis_info *vi,vorbis_comment *vc,
					  ogg_packet *op);

extern int      __cdecl vorbis_synthesis_init(vorbis_dsp_state *v,vorbis_info *vi);
extern int      __cdecl vorbis_synthesis(vorbis_block *vb,ogg_packet *op);
extern int      __cdecl vorbis_synthesis_blockin(vorbis_dsp_state *v,vorbis_block *vb);
extern int      __cdecl vorbis_synthesis_pcmout(vorbis_dsp_state *v,float ***pcm);
extern int      __cdecl vorbis_synthesis_read(vorbis_dsp_state *v,int samples);
extern long     __cdecl vorbis_packet_blocksize(vorbis_info *vi,ogg_packet *op);

/* Vorbis ERRORS and return codes ***********************************/

#define OV_FALSE      -1  
#define OV_EOF        -2
#define OV_HOLE       -3

#define OV_EREAD      -128
#define OV_EFAULT     -129
#define OV_EIMPL      -130
#define OV_EINVAL     -131
#define OV_ENOTVORBIS -132
#define OV_EBADHEADER -133
#define OV_EVERSION   -134
#define OV_ENOTAUDIO  -135
#define OV_EBADPACKET -136
#define OV_EBADLINK   -137
#define OV_ENOSEEK    -138

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif


/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   standard chaos stuff                                               ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

struct sOGGStub
{
public:
  sInt DecStart();
  sInt DecBlock();
  void DecEnd();
//  sInt Decoder();
  void Handler(void *stream,sInt samples,sU32 time);

  sInt GetPacket();
  sInt GetPage();

  sU8 *Coded;
//  sU8 *Data;
  sS16 *Buffer;

  sInt Size;
  sInt Current;
  sInt Read,Write;
  
  ogg_sync_state   oy; /* sync and verify incoming physical bitstream */
  ogg_stream_state os; /* take physical pages, weld into a logical stream of packets */
  ogg_page         og; /* one Ogg bitstream page.  Vorbis packets are inside */
  ogg_packet       op; /* one raw packet of data for decode */
  
  vorbis_info      vi; /* struct that stores all the static vorbis bitstream settings */
  vorbis_comment   vc; /* struct that stores all the bitstream user comments */
  vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
  vorbis_block     vb; /* local working space for packet->PCM decode */
};

/****************************************************************************/
/*
void MySound::Load(sChar *name)
{  
  Coded = (sU8 *)sSystem->LoadFile(name,Size);
  Data = new sU8[1024*1024*64]; 
  Buffer = new sS16[16*1024];
  Current = Read = Write = 0;
  if(!Decoder())
    sFatal("decode error");
}
*/
/****************************************************************************/
/*
void MySound::Handler(void *stream,sInt samples,sU32 time)
{
  if(Data && Write-Current>=samples)
  {
    sCopyMem(stream,Data+Current*4,samples*4);
    Current+=samples;
  }
}
*/
/****************************************************************************/
/*
sInt MySound::Decoder()
{
  sInt size;
  sInt result;

  result = 0;
  if(DecStart())
  {
    do
    {
      size = DecBlock();
      if(size>0)
      {
        sCopyMem(Data+Write,Buffer,size);
        Write+=size;
      }
    }
    while(size>0);
    if(size==-1)
      result = 1;
    DecEnd();
  }

  return result;
}
*/
/****************************************************************************/

sInt sOGGStub::GetPage()
{
  sInt result;
  int bytes;
  char *buffer;
  while((result=ogg_sync_pageout(&oy,&og))==0)
  {
	  buffer=ogg_sync_buffer(&oy,4096);
    bytes=sMin(4096,Size-Read); sCopyMem(buffer,Coded+Read,bytes); Read+=bytes;
	  ogg_sync_wrote(&oy,bytes);
	  if(bytes==0) return 0;
  }
  if(result<0) return 0;
  return 1;
}

/****************************************************************************/

sInt sOGGStub::GetPacket()
{
  sInt result;
  while((result=ogg_stream_packetout(&os,&op))==0)
  {
    GetPage();
    ogg_stream_pagein(&os,&og);
  }
  if(result<0)
    return 0;
  return 1;
}

/****************************************************************************/

sInt sOGGStub::DecStart()
{
  ogg_sync_init(&oy);
  vorbis_info_init(&vi);
  vorbis_comment_init(&vc);

  if(!GetPage()) return 0;
  ogg_stream_init(&os,ogg_page_serialno(&og));
  if(ogg_stream_pagein(&os,&og)<0) return 0;
  if(ogg_stream_packetout(&os,&op)!=1) return 0;
  if(vorbis_synthesis_headerin(&vi,&vc,&op)<0) return 0;

  if(!GetPacket()) return 0;
  vorbis_synthesis_headerin(&vi,&vc,&op);

  if(!GetPacket()) return 0;
  vorbis_synthesis_headerin(&vi,&vc,&op);

  vorbis_synthesis_init(&vd,&vi);
  vorbis_block_init(&vd,&vb);    

//  if(vi.channels!=2) return 0;

  return 1;
}

/****************************************************************************/

sInt sOGGStub::DecBlock()
{
  sInt write;
  float **pcm;
	int samples;
	int i,j;
	int clipflag=0;
	int bout;

  write = 0;
  while(write==0)
  {
    if(ogg_page_eos(&og))
      return -1;

    if(!GetPacket()) return 0;

    if(vorbis_synthesis(&vb,&op)==0)
	    vorbis_synthesis_blockin(&vd,&vb);

    while((samples=vorbis_synthesis_pcmout(&vd,&pcm))>0)
    {
	    bout=samples;
	    
	    for(i=0;i<2;i++)
      {
		    sS16 *ptr=Buffer+i+write*2;
		    float  *mono=pcm[i];
		    for(j=0;j<bout;j++)
        {
  	      int val=mono[j]*32767.f;
		      if(val>32767)
		        val=32767;
		      if(val<-32768)
		        val=-32768;
		      *ptr=val;
		      ptr+=2;
		    }
	    }
	    
      write+=bout;
	    vorbis_synthesis_read(&vd,bout); 
//      sDPrintF("(%d)",bout);
    }
  }
//  sDPrintF("write %d\n",write);
  return write*4;
}

/****************************************************************************/

void sOGGStub::DecEnd()
{
  ogg_stream_clear(&os);
  vorbis_block_clear(&vb);
  vorbis_dsp_clear(&vd);
  vorbis_comment_clear(&vc);
  vorbis_info_clear(&vi); 
  ogg_sync_clear(&oy);  
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#include "_ogg.hpp"

sOGGDecoder::sOGGDecoder()
{
  ZeroStart = 0;
  Index = 0;
  Size = 0;

  Stub = new sOGGStub;
  Stub->Coded = 0;
  Stub->Size = 0;
  Stub->Current = 0;
  Stub->Read = 0;
  Stub->Write = 0;
  Stub->Buffer = new sS16[16*1024];
}

/****************************************************************************/

sOGGDecoder::~sOGGDecoder()
{
  if(Stub)
  {
    if(Stub->Size)
      Stub->DecEnd();

    if(Stub->Buffer)
      delete[] Stub->Buffer;
    delete Stub;
    Stub = 0;
  }
}

/****************************************************************************/

sBool sOGGDecoder::Init(sInt songnr)
{
  Stub->Coded = Stream;
  Stub->Size = StreamSize;

  ZeroStart = 0;
  Size = 0;
  Index = 0;

  return Stub->DecStart();
}

/****************************************************************************/

sInt sOGGDecoder::Render(sS16 *stream,sInt samples)
{
  sInt count;
  sU32 *d;
  sInt rendered = 0;

  d = (sU32 *) stream;

  while(samples && ZeroStart)
  {
    count = sMin(samples,ZeroStart);
    sSetMem4(d,0,count);
    ZeroStart -= count;
    samples -= count;
    rendered += count;
  }

  while(samples>0)
  {
    if(Index==Size)
    {
      Index = 0;
      Size = Stub->DecBlock()/4;
    }
    if(Size==0)
      break;

    count = sMin(samples,Size-Index);

    sCopyMem4(d,((sU32 *)Stub->Buffer)+Index,count);
    d += count;
    samples -= count;
    Index += count;
    rendered += count;
  }
  if(samples>0)
  {
    sSetMem4(d,0,samples);
    rendered += samples;
  }

  return rendered;
}

/****************************************************************************/

sInt sOGGDecoder::GetTuneLength()
{
  sInt seconds = (StreamSize * 8) / Stub->vi.bitrate_nominal;
  return (seconds * 105 / 100 + 2) * 44100;
}

/****************************************************************************/
/****************************************************************************/

#endif

