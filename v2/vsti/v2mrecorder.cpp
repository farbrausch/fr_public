
#pragma warning (disable: 4244)

#define _CRT_SECURE_NO_DEPRECATE

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include "windows.h"
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>

#include <list>
#include "../tool/file.h"
#include "../types.h"

#include "../sounddef.h"

#include "v2mrecorder.h"

using namespace std;

//----------------------------------------------------------------------------------------
// Daten
//----------------------------------------------------------------------------------------
namespace
{

	
	struct _mev1 { 
		sU32 time; 
		sU8 v1; 
		bool operator < (_mev1 b) const { return time<b.time; }
	};

	struct _mev2 { 
		sU32 time; 
		sU8 v1, v2; 
		bool operator < (_mev2 b) const { return time<b.time || (time==b.time && v2<b.v2); }
	};

	struct _timeev 
	{ 
		sU32 time; 
		sU32 usecs; 
		sU8 num, den, tpq; 
		bool operator < (_timeev b) const { return time<b.time; }
	};

	typedef list<_timeev> timelist;
	typedef list<_mev1> m1list;
	typedef list<_mev2> m2list;

	typedef struct {
		m2list notes;
		m1list pgmch;
		m2list pitch;
		m1list controls[7];
	} _chan;

	struct v2mdata
	{
		timelist globevs;
		_chan    chanevs[16];
	};
};



struct CV2MRecorder::PrivateData
{
	v2mdata vd;
	sU32 curtime;
	sU32 firsttime;
	const sU32 framesize;
	const sU32 samplerate;

	PrivateData(const sU32 fs, const sU32 sr) : curtime(0), firsttime(0xdeadbeef), framesize(fs), samplerate(sr) {};
};



CV2MRecorder::CV2MRecorder(sU32 framesize, sU32 samplerate)
{
	printf("v2mrec on\n");
	p = new PrivateData(framesize,samplerate);

	_timeev acttime={0, (sU32)(1000000.0f*128.0f*(sF32)p->framesize/(sF32)p->samplerate), 4, 4, 8};
	p->vd.globevs.push_back(acttime);

}


CV2MRecorder::~CV2MRecorder()
{
	printf("v2mrec off\n");
	delete p;
}


void CV2MRecorder::AddEvent(long sampletime, sU8 b0, sU8 b1, sU8 b2)
{
  
	p->curtime=sampletime;

	_mev1 m1;
	_mev2 m2;

	int realtime = p->curtime/p->framesize;
	int cmd=(b0&0x70)>>4;
	int chn=(b0&0x0f);

	_chan &che=p->vd.chanevs[chn];

	m1.time=m2.time=realtime;

	printf("addev %6d, %02x %02x %02x\n",realtime,b0,b1,b2);

	switch (cmd)
	{
	case 1: // 9x: note on
		if (p->firsttime==0xdeadbeef)
			p->firsttime=realtime;
		printf("setting firsttime: %08x\n",p->firsttime);
	case 0: // 8x: note off
		m2.v1=b1;
		if (cmd && b2)
		{
			m2.v2=b2;
			che.notes.push_back(m2);
		}
		else
		{
			m2.v2=0;
			m2list::iterator it=che.notes.end();
			it--;
			while (it != che.notes.end() && it->time==m2.time) it--;
			it++;
			che.notes.insert(it,m2);
		}
		break;

	case 3: // ctlchange
		m1.v1=b2;
		if (b1>0 && b1<8) che.controls[b1-1].push_back(m1);
		break;

	case 4: // pgmchange
		m1.v1=b1;
		che.pgmch.push_back(m1);
		break;

	case 6: // pitchben
		m2.v1=b1;
		m2.v2=b2;
		che.pitch.push_back(m2);
		break;

	case 2: // polypressure
	case 5: // chanpressure
	case 7: // realtime
		break;
	}
		
}


int CV2MRecorder::Export(file &f)
{
	printf("export!\n",p->firsttime);

	if (p->firsttime==0xdeadbeef)
		return 0;

	if (p->firsttime) p->firsttime--;

	// postprocess event lists
	sU32 maxtime=0;
	sU32 pgmno=0;
	sU8      pgmmap[256];

	int  time;

	// globals...
	p->vd.globevs.sort();
	timelist::iterator gi,g2;
	
	gi=p->vd.globevs.begin();
	time=0;
	while (gi!=p->vd.globevs.end())
	{
    g2=gi;
		gi++;
		if (gi != p->vd.globevs.end() && gi->time==g2->time)
			p->vd.globevs.erase(g2);
		else
		{
			if (g2->time>p->firsttime) g2->time-=p->firsttime; else g2->time=0;
			if (g2->time>maxtime)
				maxtime=g2->time;
      g2->time-=time;
			time+=g2->time;
		}
	}


	printf("global events: %d\n",p->vd.globevs.size());

	for (int i=0; i<16; i++)
	{
		_chan &ch=p->vd.chanevs[i];
		ch.notes.sort();
		m2list::iterator ni=ch.notes.begin();
		time=0;
		while (ni!=ch.notes.end())
		{
			if (ni->time>p->firsttime) ni->time-=p->firsttime; else ni->time=0;
			if (ni->time>maxtime)
				maxtime=ni->time;
			ni->time-=time;
			time+=ni->time;
		
			ni++;
		}
		ch.pgmch.sort();
		m1list::iterator pci=ch.pgmch.begin(), pc2;
		time=0;
		while (pci!=ch.pgmch.end())
		{
			pc2=pci;
			pci++;
			if (pci != ch.pgmch.end() && pci->time==pc2->time)
				ch.pgmch.erase(pc2);
			else
			{
				if (pc2->time>p->firsttime) pc2->time-=p->firsttime; else pc2->time=0;

				if (pc2->time>maxtime)
					maxtime=pc2->time;
				pc2->time-=time;
				time+=pc2->time;
			
				// remapping
				sU32 tp;
				for (tp=0; tp<pgmno; tp++)
					if (pc2->v1 == pgmmap[tp])
						break;
				if (tp==pgmno)
				{
//					printf("Remapping PGM %d to %d\n",pc2->v1,tp);
					pgmmap[tp]=pc2->v1;
					pgmno++;
				}
				pc2->v1=tp;
			}
		}
		ch.pitch.sort();
		m2list::iterator pbi=ch.pitch.begin(), pb2;
		time=0;
		while (pbi!=ch.pitch.end())
		{
			pb2=pbi;
			pbi++;
			if (pbi != ch.pitch.end() && pbi->time==pb2->time)
				ch.pitch.erase(pb2);
			else
			{
				if (pb2->time>p->firsttime) pb2->time-=p->firsttime; else pb2->time=0;

				if (pb2->time>maxtime)
					maxtime=pb2->time;
				pb2->time-=time;
				time+=pb2->time;
				
			}
		}
		int ccn=0;
		for (int j=0; j<7; j++)
		{
			m1list &cc=ch.controls[j];
			cc.sort();
			m1list::iterator cci=cc.begin(), cc2;
			time=0;
			while (cci!=cc.end())
			{
				cc2=cci;
				cci++;
				if (i!=15 && cci != cc.end() && cci->time==cc2->time)
					cc.erase(cc2);
				else
				{
					if (cc2->time>p->firsttime) cc2->time-=p->firsttime; else cc2->time=0;

					if (j==6) printf("vol cmd: ch %d, val %d\n",i,cc2->v1);

					if (cc2->time>maxtime)
						maxtime=cc2->time;
					cc2->time-=time;
					time+=cc2->time;
					
				}
			}
			ccn+=cc.size();
		}

		if (ch.notes.size() && ch.pgmch.size())
			printf("channel %2d: %4d notes, %4d pgmch, %4d pitchb, %4d ctlch\n",i+1,
							ch.notes.size(),ch.pgmch.size(),ch.pitch.size(),ccn);
		if (ch.notes.size() && !ch.pgmch.size())
			printf("WARNING: channel %2d contains notes but no program changes! This won't work!\n",i+1);
	}

	// write out v2m
	f.putsU32(128); // time division
	f.putsU32(maxtime);

	timelist &globevs=p->vd.globevs;
  f.putsU32(globevs.size());
	for (gi=globevs.begin(); gi!=globevs.end(); ++gi)
		f.putsU8((gi->time) & 0xff);
	for (gi=globevs.begin(); gi!=globevs.end(); ++gi)
		f.putsU8((gi->time)>>8);
	for (gi=globevs.begin(); gi!=globevs.end(); ++gi)
		f.putsU8((gi->time)>>16);
	for (gi=globevs.begin(); gi!=globevs.end(); ++gi)
		f.putsU32(gi->usecs);
	for (gi=globevs.begin(); gi!=globevs.end(); ++gi)
		f.putsU8(gi->num);
	for (gi=globevs.begin(); gi!=globevs.end(); ++gi)
		f.putsU8(gi->den);
	for (gi=globevs.begin(); gi!=globevs.end(); ++gi)
		f.putsU8(gi->tpq);

	sU8 v1, v2;

  for (int ch=0; ch<16; ch++)
	{
		m1list::iterator m1;
		m2list::iterator m2;
    _chan &c=p->vd.chanevs[ch];
		f.putsU32(c.notes.size());
		if (!c.notes.size())
			continue;

		v1=v2=0;
		for (m2=c.notes.begin(); m2!=c.notes.end(); m2++)
			f.putsU8((m2->time) & 0xff);
		for (m2=c.notes.begin(); m2!=c.notes.end(); m2++)
			f.putsU8((m2->time)>>8);
		for (m2=c.notes.begin(); m2!=c.notes.end(); m2++)
			f.putsU8((m2->time)>>16);
		for (m2=c.notes.begin(); m2!=c.notes.end(); m2++)
		{
			f.putsU8(m2->v1-v1);
			v1=m2->v1;
		}
		for (m2=c.notes.begin(); m2!=c.notes.end(); m2++)
		{
			f.putsU8(m2->v2-v2);
			v2=m2->v2;
		}

		f.putsU32(c.pgmch.size());
		v1=v2=0;
		for (m1=c.pgmch.begin(); m1!=c.pgmch.end(); m1++)
			f.putsU8((m1->time) & 0xff);
		for (m1=c.pgmch.begin(); m1!=c.pgmch.end(); m1++)
			f.putsU8((m1->time)>>8);
		for (m1=c.pgmch.begin(); m1!=c.pgmch.end(); m1++)
			f.putsU8((m1->time)>>16);
		for (m1=c.pgmch.begin(); m1!=c.pgmch.end(); m1++)
		{
			sU8 gaga=m1->v1-v1;
			f.putsU8(gaga);
			v1=m1->v1;
		}

		f.putsU32(c.pitch.size());
		v1=v2=0;
		for (m2=c.pitch.begin(); m2!=c.pitch.end(); m2++)
			f.putsU8((m2->time) & 0xff);
		for (m2=c.pitch.begin(); m2!=c.pitch.end(); m2++)
			f.putsU8((m2->time)>>8);
		for (m2=c.pitch.begin(); m2!=c.pitch.end(); m2++)
			f.putsU8((m2->time)>>16);
		for (m2=c.pitch.begin(); m2!=c.pitch.end(); m2++)
		{
			f.putsU8(m2->v1-v1);
			v1=m2->v1;
		}
		for (m2=c.pitch.begin(); m2!=c.pitch.end(); m2++)
		{
			f.putsU8(m2->v2-v2);
			v2=m2->v2;
		}

		for (int ccn=0; ccn<7; ccn++)
		{
			m1list &cc=c.controls[ccn];
			f.putsU32(cc.size());
			v1=v2=0;
			for (m1=cc.begin(); m1!=cc.end(); m1++)
				f.putsU8((m1->time) & 0xff);
			for (m1=cc.begin(); m1!=cc.end(); m1++)
				f.putsU8((m1->time)>>8);
			for (m1=cc.begin(); m1!=cc.end(); m1++)
				f.putsU8((m1->time)>>16);
			for (m1=cc.begin(); m1!=cc.end(); m1++)
			{
				f.putsU8(m1->v1-v1);
				v1=m1->v1;
			}
		}

	}
	
	
	// dahinter globals...
	f.putsU32(v2ngparms);
	f.write(globals,v2ngparms);

	// pack patch data
	{
		sU8 *smblock=new sU8[pgmno*(4+v2soundsize)];
		sU32 *offsets=(sU32*)smblock;
		sU8  *smptr=smblock+4*pgmno;

		for (sU32 i=0; i<pgmno; i++)
		{
			offsets[i]=smptr-smblock;
			sU8 *mem=soundmem+patchoffsets[pgmmap[i]];
			int nMods=mem[v2nparms];
			printf("patch %d: %d at offs %d, %d mods\n",i,pgmmap[i],offsets[i],nMods);
			memcpy(smptr,mem,v2nparms+1);
			smptr+=v2nparms+1;
			memcpy(smptr,mem+v2nparms+1,3*nMods);
			smptr+=3*nMods;
		}

		// ... and write them
		f.putsU32(smptr-smblock);
		f.write(smblock,smptr-smblock);
		delete[] smblock;
	}

#ifdef RONAN
	// pack speech data
	{
		// count used speech segs
		sU32 n=0;
		for (sU32 i=0; i<64; i++)
			if (strlen(speechptrs[i])) n=i+1;

		// pack into block
		sU8 *spblock=new sU8[n*(256+4)+4];
		sU32 *nsp=(sU32*)spblock;
		*nsp=n;
		sU32 *offsets=nsp+1;
		sU8  *spptr=(sU8*)(offsets+n);
		for (sU32 i=0; i<n; i++)
		{
			offsets[i]=spptr-spblock;
			strcpy((char*)spptr,speechptrs[i]);
			spptr+=strlen((char*)spptr)+1;
		}

		// ... and write
		f.putsU32(spptr-spblock);
		f.write(spblock,spptr-spblock);
		delete[] spblock;
	}
#else
		f.putsU32(0);
#endif

	printf("ficken1: %d, %d\n",f.tell(),f.size());
	/*
	// speech
	f.putsU32(spsize);
	f.write(speechblock,spsize);
*/
  return 1;
}
