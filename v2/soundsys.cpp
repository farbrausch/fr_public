
//#define HDRECORD

#include "types.h"
#include "soundsys.h"
#include "synth.h"
#include "dsio.h"
#include <stdio.h>
#include <windows.h>

#ifdef HDRECORD
#include "wtlbla/file.h"
static fileS rawout;
#endif


#ifdef _DEBUG
   int ssCPUUsage;
#endif

	 /*

Format:

+ d fraction
+ d maxtime
+ d anzahl globevs
  - b * globevs tdlo
  - b * globevs tdhi
  - b * globevs tdhi2
  - d * globevs usecs
  - b * globevs num
  - b * globevs den
  - b * globevs tpq
* for channels 0-15:
  + d anzahl notes 
	  (alles folgende nur wenn  != 0 !!!)
	  - b * notes tdlo
		- b * notes tdhi
		- b * notes tdhi2
		- b * notes v0
		- b * notes v1
  + d anzahl pgmchange
		- b * pgmch tdlo	  
		- b * pgmch tdhi
		- b * pgmch tdhi2
		- b * pgmch v0
  + d anzahl pitchbends
		- b * pitch tdlo	  
		- b * pitch tdhi
		- b * pitch tdhi2
		- b * pitch v0
		- b * pitch v1
	* for controllers 1-7
		+ d anzahl ctlchanges
			- b * ctlch tdlo	  
			- b * ctlch tdhi
			- b * ctlch tdhi2
			-	b * ctlch v0
*/


#ifdef RULESYNC

static struct rsEvent
{
	sU32 cursmp;
	sU32 pos;
} rsqueue[256];

static sU8 rsrpos;
static sU8 rswpos;
static sS32 rscurpos;
static sS32 rstimeoffs;
#endif

// nicht drüber nachdenken.

sU8 synth[3*1024*1024]; // FIXME: make size somewhat dynamic

static struct _ssbase {
	sU8   *patchmap;
	sU8   *globals;
	sU32	timediv;
	sU32	timediv2;
	sU32	maxtime;
	sU8   *gptr;
	sU32  gdnum;
  struct _basech {
		sU32	notenum;
		sU8		*noteptr;
		sU32	pcnum;
		sU8		*pcptr;
		sU32	pbnum;
		sU8		*pbptr;
		struct _bcctl {
			sU32	ccnum;
			sU8		*ccptr;
		} ctl[7];
	} chan[16];
#ifdef RONAN
	const char  *speechdata;
	const char  *speechptrs[256];
#endif
} base;

static struct _ssstat
{
	sBool running;
	sBool silent;
  sU32	time;
  sU32	nexttime;
	sU8		*gptr;
	sU32	gnt;
	sU32	gnr;
	sU32  usecs;
	sU32  num;
	sU32  den;
	sU32	tpq;
  sU32  bar;
	sU32  beat;
	sU32  tick;
	struct _statch {
		sU8		*noteptr;
		sU32  notenr;
		sU32	notent;
		sU8		lastnte;
		sU8   lastvel;
		sU8   *pcptr;
		sU32  pcnr;
		sU32  pcnt;
		sU8		lastpc;
		sU8		*pbptr;
		sU32  pbnr;
		sU32  pbnt;
		sU8		lastpb0;
		sU8		lastpb1;
		struct _scctl {
			sU8		*ccptr;
			sU32  ccnt;
			sU32  ccnr;
			sU8   lastcc;
		} ctl[7];
	} chan[16];
	sU32 cursmpl;
	sU32 smpldelta;
	sU32 smplrem;
	sU32 tdif;
} state;


int ssInitBase(sU8 *d, _ssbase &dest)
{
	dest.timediv=(*((sU32*)(d)));
	dest.timediv2=10000*dest.timediv;
	dest.maxtime=*((sU32*)(d+4));
	dest.gdnum=*((sU32*)(d+8));
	d+=12;
	dest.gptr=d;
  d+=10*dest.gdnum;
	for (sInt ch=0; ch<16; ch++)
	{
		_ssbase::_basech &c=dest.chan[ch];
		c.notenum=*((sU32*)d);
		d+=4;
		if (c.notenum)
		{
			c.noteptr=d;
			d+=5*c.notenum;
			c.pcnum=*((sU32*)d);
			d+=4;
			c.pcptr=d;
			d+=4*c.pcnum;
			c.pbnum=*((sU32*)d);
			d+=4;
			c.pbptr=d;
			d+=5*c.pbnum;
			for (sInt cn=0; cn<7; cn++)
			{
				_ssbase::_basech::_bcctl &cc=c.ctl[cn];
				cc.ccnum=*((sU32*)d);
				d+=4;
				cc.ccptr=d;
				d+=4*cc.ccnum;
			}						
		}		
	}
	sInt size=*((sU32*)d);
	if (size>16384 || size<0) return 0;
	d+=4;
	dest.globals=d;
	d+=size;
	size=*((sU32*)d);
	if (size>1048576 || size<0) return 0;
	d+=4;
	dest.patchmap=d;
	d+=size;
#ifdef RONAN
	sU32 spsize=*((sU32*)d);
	d+=4;
	if (!spsize || spsize>=8192)
	{
		for (sU32 i=0; i<256; i++)
			dest.speechptrs[i]=" ";
	}
	else
	{
		dest.speechdata=(const char *)d;
		d+=spsize;
		sU32 *p32=(sU32*)dest.speechdata;
		sU32 n=*(p32++);
		for (sU32 i=0; i<n; i++)
		{
			dest.speechptrs[i]=dest.speechdata+*(p32++);
		}
	}
#endif
	return 1;
}





static sU8  midibuf[4096];

#define GETDELTA(p, w) ((p)[0]+((p)[w]<<8)+((p)[2*w]<<16))
#define UPDATENT(n, v, p, w) if ((n)<(w)) { (v)=state.time+GETDELTA((p), (w)); if ((v)<state.nexttime) state.nexttime=(v); }
#define UPDATENT2(n, v, p, w) if ((n)<(w) && GETDELTA((p), (w))) { (v)=state.time+GETDELTA((p), (w)); }
#define UPDATENT3(n, v, p, w) if ((n)<(w) && (v)<state.nexttime) state.nexttime=(v); 
#define PUTSTAT(s) { sU8 bla=(s); if (laststat!=bla) { laststat=bla; *mptr++=(sU8)laststat; }};


void ssReset()
{
	state.time=0;
	state.nexttime=(sU32)-1;

	state.gptr=base.gptr;
	state.gnr=0;
	UPDATENT(state.gnr, state.gnt, state.gptr, base.gdnum);
	for (sInt ch=0; ch<16; ch++)
	{
		_ssbase::_basech &bc=base.chan[ch];
		_ssstat::_statch &sc=state.chan[ch];

		if (!bc.notenum) continue;
		sc.noteptr=bc.noteptr;
		sc.notenr=sc.lastnte=sc.lastvel=0;
		UPDATENT(sc.notenr,sc.notent, sc.noteptr, bc.notenum);
		sc.pcptr=bc.pcptr;
		sc.pcnr=sc.lastpc=0;
		UPDATENT(sc.pcnr,sc.pcnt, sc.pcptr, bc.pcnum);
		sc.pbptr=bc.pbptr;
		sc.pbnr=sc.lastpb0=sc.lastpb1=0;
		UPDATENT(sc.pbnr,sc.pbnt, sc.pbptr, bc.pcnum);
		for (sInt cn=0; cn<7; cn++)
		{
				_ssbase::_basech::_bcctl &bcc=bc.ctl[cn];
				_ssstat::_statch::_scctl &scc=sc.ctl[cn];
				scc.ccptr=bcc.ccptr;
				scc.ccnr=scc.lastcc=0;
				UPDATENT(scc.ccnr,scc.ccnt,scc.ccptr,bcc.ccnum);
		}
	}
	state.usecs=500000*441;
	state.num=4;
	state.den=4;
	state.tpq=8;
	state.bar=0;
	state.beat=0;
	state.tick=0;
	state.smplrem=0;

#ifdef RULESYNC
  rsrpos=rswpos=0;
  rscurpos=-1;
#endif

	synthInit(synth,base.patchmap);
	synthSetGlobals(synth,base.globals);
#ifdef RONAN
	synthSetLyrics(synth,base.speechptrs);
#endif
}


void ssTick()
{
	if (!state.running)
		return;

	state.tick+=state.nexttime-state.time;
	while (state.tick>=base.timediv)
	{
		state.tick-=base.timediv;
		state.beat++;
	}
	sU32 qpb=(state.num*4/state.den);
	while (state.beat>=qpb)
	{
		state.beat-=qpb;
		state.bar++;
	}


	state.time=state.nexttime;
	state.nexttime=(sU32)-1;
	sU8 *mptr=midibuf;
	sU32 laststat=-1;

	if (state.gnr<base.gdnum && state.time==state.gnt) // neues global-event?
	{
		state.usecs=(*(sU32 *)(state.gptr+3*base.gdnum+4*state.gnr))*441;
		state.num=state.gptr[7*base.gdnum+state.gnr];
		state.den=state.gptr[8*base.gdnum+state.gnr];
		state.tpq=state.gptr[9*base.gdnum+state.gnr];
		state.gnr++;
		UPDATENT2(state.gnr, state.gnt, state.gptr+state.gnr, base.gdnum);
	}
	UPDATENT3(state.gnr, state.gnt, state.gptr+state.gnr, base.gdnum);

	for (sInt ch=0; ch<16; ch++) 
	{
		_ssbase::_basech &bc=base.chan[ch];
		_ssstat::_statch &sc=state.chan[ch];
		if (!bc.notenum)
			continue;
		// 1. process pgm change events
		if (sc.pcnr<bc.pcnum && state.time==sc.pcnt)
		{
			PUTSTAT(0xc0|ch)
			*mptr++=(sc.lastpc+=sc.pcptr[3*bc.pcnum]);
			sc.pcnr++;
			sc.pcptr++;
			UPDATENT2(sc.pcnr,sc.pcnt,sc.pcptr,bc.pcnum);

      if (ch==10)
      {
        char buffer[128]; sprintf(buffer,"pc %d\n",sc.lastpc); OutputDebugString(buffer);
      }


		}
		UPDATENT3(sc.pcnr,sc.pcnt,sc.pcptr,bc.pcnum);

		// 2. process control changes
 		for (sInt cn=0; cn<7; cn++)
		{
				_ssbase::_basech::_bcctl &bcc=bc.ctl[cn];
				_ssstat::_statch::_scctl &scc=sc.ctl[cn];
				if (scc.ccnr<bcc.ccnum && state.time==scc.ccnt)
				{
					PUTSTAT(0xb0|ch)
					*mptr++=cn+1;
					*mptr++=(scc.lastcc+=scc.ccptr[3*bcc.ccnum]);
					scc.ccnr++;
					scc.ccptr++;
					UPDATENT2(scc.ccnr,scc.ccnt,scc.ccptr,bcc.ccnum);
				}
				UPDATENT3(scc.ccnr,scc.ccnt,scc.ccptr,bcc.ccnum);
		}

		// 3. process pitch bends
		if (sc.pbnr<bc.pbnum && state.time==sc.pbnt)
		{
			PUTSTAT(0xe0|ch)
			*mptr++=(sc.lastpb0+=sc.pbptr[3*bc.pcnum]);
			*mptr++=(sc.lastpb1+=sc.pbptr[4*bc.pcnum]);
			sc.pbnr++;
			sc.pbptr++;
			UPDATENT2(sc.pbnr,sc.pbnt,sc.pbptr,bc.pbnum);
		}
		UPDATENT3(sc.pbnr,sc.pbnt,sc.pbptr,bc.pbnum);

		// 4. process notes
		while (sc.notenr<bc.notenum && state.time==sc.notent)
		{
			PUTSTAT(0x90|ch)
			*mptr++=(sc.lastnte+=sc.noteptr[3*bc.notenum]);
			*mptr++=(sc.lastvel+=sc.noteptr[4*bc.notenum]);

			//			if (ch==14) printf2("(%d of %d) %d %d\n",sc.notenr,bc.notenum,mptr[-2],mptr[-1]);
			sc.notenr++;
			sc.noteptr++;
			UPDATENT2(sc.notenr,sc.notent,sc.noteptr,bc.notenum);
		}
		UPDATENT3(sc.notenr,sc.notent,sc.noteptr,bc.notenum);
	}

	*mptr++=0xfd;

	synthProcessMIDI(synth,midibuf);
	
	if (state.nexttime==(sU32)-1)
		state.running=0;


}




void __stdcall ssRender(void *, sF32 *outbuf, sU32 len)
{
#ifdef _DEBUG
	int oldsmpl=dsGetCurSmp();
	int oldlen=len;
#endif
#ifdef HDRECORD
	int hdlen=len;
	sF32 *hdbuf=outbuf;
#endif
	if (state.running && !state.silent)
	{
		while (len)
		{
			sInt torender=(len>state.smpldelta)?state.smpldelta:len;
			{
				if (torender)
					synthRender(synth,outbuf,torender);
			}
			outbuf+=2*torender;
			len-=torender;
			state.smpldelta-=torender;
			state.cursmpl+=torender;
			if (!state.smpldelta)
			{
				ssTick();
				if (state.running)
				{
					__asm {
						mov eax, [state.nexttime]
						sub eax, [state.time]
						mov ebx, [state.usecs]
						mul ebx
						mov ebx, [base.timediv2]
						div ebx
						add [state.smplrem], edx
						adc eax, 0
						mov [state.smpldelta], eax
					}
				}
				else
					state.smpldelta=-1;

				#ifdef RULESYNC
					sU32 pb32=(state.num*32/state.den);
					sU32 tb32=8*state.beat+8*state.tick/base.timediv;
					sU32 curpos=(state.bar<<16)|(tb32<<8)|(pb32);
					if (rsrpos==(sU8)(rswpos+1)) rsrpos++;
					rsqueue[rswpos].cursmp=state.cursmpl;
					rsqueue[rswpos].pos=curpos;
					rswpos++;
				#endif
			}
		}
	}
	else
	{
	  if (state.silent)
			__asm {
				mov edi, [outbuf]
				mov ecx, [len]
				shl ecx, 1
				xor eax, eax
				rep stosd
			}
		else
			synthRender(synth,outbuf,len);
		state.cursmpl+=len;
	}
#ifdef _DEBUG
	oldsmpl=((dsGetCurSmp()-oldsmpl+0x10000)&0xffff)/4;
	ssCPUUsage=100*oldsmpl/oldlen;
#endif

#ifdef HDRECORD
	if (!state.silent)
	{
		sS16 bla[2*65536];
		for (int hs=0; hs<2*hdlen; hs++)
			bla[hs]=(sS16)(32768.0f*hdbuf[hs]);
			rawout.write(bla,4*hdlen);
	}
#endif
}


void __stdcall ssInit(sU8 *d, void *hWnd)
{

	ssInitBase(d,base);

#ifdef HDRECORD
	rawout.open("out.raw",fileS::wr|fileS::cr);
#endif


	ssReset();
	state.cursmpl=state.smpldelta=0;
	state.running=0;
	state.silent=1;
	dsInit(ssRender,0,hWnd);
	dsSetVolume(1.0f);
}


void __stdcall ssFadeOut(sU32 time)
{
	//dsFadeOut(time*44100/TIMER_TICKS_PER_SEC);
}


void __stdcall ssClose()
{
  dsClose();

#ifdef HDRECORD
	rawout.close();
#endif

}


sS32 __stdcall ssGetTime()
{
	sS32 bla=dsGetCurSmp()-state.tdif;
	__asm
	{
		mov	 eax, [bla]
		mov  ebx, TIMER_TICKS_PER_SEC
		imul ebx
		mov  ebx, 176400
		idiv ebx
		mov  [bla], eax
	}
	return bla;
}


void __stdcall ssPlay()
{
	ssStop();
	ssReset();
	state.silent=0;
	state.running=1;
	state.tdif=4*state.cursmpl;
#ifdef RULESYNC
	rstimeoffs=-(sS32)4*state.cursmpl;
#endif
}


void __stdcall ssStop()
{
	state.running=0;
	state.silent=1;
	// MIDI-Reset schicken!
}

void __stdcall ssDoTick()
{
  dsTick();
}


#ifdef RULESYNC

sS32 __stdcall ssGetPos()
{
	if (!state.running)
		return rscurpos;
	sS32 realsmp=dsGetCurSmp();
	while (rsrpos!=rswpos && (sS32)rsqueue[rsrpos].cursmp<=realsmp)
		rscurpos=rsqueue[rsrpos++].pos;
	return rscurpos;
}

sU32 __stdcall ssCalcPositions(sS32 **dest, sU8 *otherv2m)
{
	_ssbase mybase;
	if (otherv2m)
		ssInitBase(otherv2m,mybase);
	else
		mybase=base;

	// step 1: ende finden
	sS32 *&dp=*dest;
	sU32 gnr=0;
	sU8* gp=mybase.gptr;
	sU32 curbar=0;
	sU32 cur32th=0;
	sU32 lastevtime=0;
	sU32 pb32=32;
	sU32 usecs=500000;

	sU32 posnum=0;
	sU32 ttime, td, this32;
	sF64 curtimer=0;
	
	while (gnr<mybase.gdnum)
	{
		ttime=lastevtime+(gp[2*mybase.gdnum]<<16)+(gp[mybase.gdnum]<<8)+gp[0];
		td=ttime-lastevtime;
		this32=(td*8/mybase.timediv);
		posnum+=this32;
		lastevtime=ttime;
		pb32=gp[7*mybase.gdnum]*32/gp[8*mybase.gdnum];
		gnr++;
		gp++;
	}
	td=mybase.maxtime-lastevtime;
	this32=(td*8/mybase.timediv);
	posnum+=this32+1;
	dp=new sS32[2*posnum];
	gnr=0;
	gp=mybase.gptr;
	lastevtime=0;
	pb32=32;
	sU32 pn;
	for (pn=0; pn<posnum; pn++)
	{
		sU32 curtime=pn*mybase.timediv/8;
		if (gnr<mybase.gdnum)
		{
			ttime=lastevtime+(gp[2*mybase.gdnum+gnr]<<16)+(gp[mybase.gdnum+gnr]<<8)+gp[gnr];
			if (curtime>=ttime)
			{
				pb32=gp[7*mybase.gdnum+gnr]*32/gp[8*mybase.gdnum+gnr];
				usecs=*(sU32 *)(gp+3*mybase.gdnum+4*gnr);
				gnr++;
				lastevtime=ttime;
			}
		}
		dp[2*pn]=(sU32)curtimer;
		dp[2*pn+1]=(curbar<<16)|(cur32th<<8)|(pb32);

		cur32th++;
		if (cur32th==pb32)
		{
			cur32th=0;
			curbar++;
		}
		curtimer+=TIMER_TICKS_PER_SEC*usecs/8000000.0;
	}
	return pn;
}



void __stdcall ssPlayFrom(sS32 time)
{
	state.silent=1;
	ssStop();
	ssReset();

	sU32 destsmpl, cursmpl=0;
	__asm
	{
		mov	 eax, [time]
		mov  ebx, 44100
		imul ebx
		mov  ebx, TIMER_TICKS_PER_SEC
		idiv ebx
		mov  [destsmpl], eax
	}

	state.running=1;
	state.smpldelta=0;
	state.smplrem=0;
  while ((cursmpl+state.smpldelta)<destsmpl && state.running)
	{
		cursmpl+=state.smpldelta;
		ssTick();
		if (state.running)
		{
			__asm {
				mov eax, [state.nexttime]
				sub eax, [state.time]
				mov ebx, [state.usecs]
				mul ebx
				mov ebx, [base.timediv2]
				div ebx
				add [state.smplrem], edx
				adc eax, 0
				mov [state.smpldelta], eax
			}
		}
		else
			state.smpldelta=-1;
	}
	rstimeoffs=4*cursmpl-4*state.cursmpl;
	state.silent=0;
}


sS32 __stdcall ssGetSongTime()
{
	sS32 bla=dsGetCurSmp();
  bla+=rstimeoffs;
	__asm
	{
		mov	 eax, [bla]
		mov  ebx, TIMER_TICKS_PER_SEC
		imul ebx
		mov  ebx, 176400
		idiv ebx
		mov  [bla], eax
	}
	return bla;
}


#endif