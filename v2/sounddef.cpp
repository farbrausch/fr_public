//#include "stdafx.h"

#include "types.h"
#include "tool/file.h"

#include <string.h>
#include <stdio.h>

#include "sounddef.h"
#include "v2mconv.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// lr: in case you miss the arrays: look in sounddef.h

static unsigned char v2clipboard[v2soundsize];
static char v2clipname[256];

unsigned char *soundmem;
long          *patchoffsets;
unsigned char *editmem;
char					patchnames [128][32];
char          globals[v2ngparms];
int           v2version;
int           *v2vsizes;
int           *v2gsizes;
int           *v2topics2;
int           *v2gtopics2;
int           v2curpatch;

#ifdef RONAN
char					speech[64][256];
char          *speechptrs[64];
#endif

void sdInit()
{
  soundmem = new unsigned char [smsize+v2soundsize];
	patchoffsets = (long *)soundmem;
	unsigned char *sptr=soundmem+128*4;

	printf("sound size: %d\n",v2nparms);

	char s[256];

	for (int i=0; i<129; i++)
	{
		if (i<128)
		{
			patchoffsets[i]=(long)(sptr-soundmem);
			sprintf(s,"Init Patch #%03d",i);
			strcpy(patchnames[i],s);
		}
		else 
			editmem=sptr;
		memcpy(sptr,v2initsnd,v2soundsize);
		sptr+=v2soundsize;
	}
	for (int i=0; i<v2ngparms; i++)
		globals[i]=v2initglobs[i];

	memcpy(v2clipboard,v2initsnd,v2soundsize);
	sprintf(v2clipname,"Init Patch");

	// init version control
	v2version=0;
	for (int i=0; i<v2nparms; i++)
		if (v2parms[i].version>v2version) v2version=v2parms[i].version;
	for (int i=0; i<v2ngparms; i++)
		if (v2gparms[i].version>v2version) v2version=v2gparms[i].version;

	v2vsizes = new int[v2version+1];
	v2gsizes = new int[v2version+1];
	memset(v2vsizes,0,(v2version+1)*sizeof(int));
	memset(v2gsizes,0,(v2version+1)*sizeof(int));
	for (int i=0; i<v2nparms; i++)
	{
		const V2PARAM &p=v2parms[i];
//		ATLASSERT(p.version<=v2version);
		for (int j=v2version; j>=p.version; j--)
			v2vsizes[j]++;
	}
	for (int i=0; i<v2ngparms; i++)
	{
		const V2PARAM &p=v2gparms[i];
		//ATLASSERT(p.version<=v2version);
		for (int j=v2version; j>=p.version; j--)
			v2gsizes[j]++;
	}
//ATLASSERT(v2vsizes[v2version]==v2nparms);

	for (int i=0; i<=v2version; i++)
	{
		printf("size of version %d sound bank: %d params, %d globals\n",i,v2vsizes[i],v2gsizes[i]);
		v2vsizes[i]+=1+255*3;
	}

	v2topics2=new int[v2ntopics];
	int p=0;
	for (int i=0; i<v2ntopics; i++)
	{
		v2topics2[i]=p;
		p+=v2topics[i].no;
	}

	v2gtopics2=new int[v2ngtopics];
	p=0;
	for (int i=0; i<v2ngtopics; i++)
	{
		v2gtopics2[i]=p;
		p+=v2gtopics[i].no;
	}

#ifdef RONAN	
	ZeroMemory(speech,64*256);
	for (int i=0; i<64; i++)
		speechptrs[i]=speech[i];

	strcpy(speech[0],"!DHAX_ !kwIH_k !br4AH_UHn !fAA_ks !jAH_mps !OW!vER_ !DHAX_ !lEY!zIY_ !dAA_g ");
#endif
}

void sdClose()
{
	delete soundmem;
	delete v2vsizes;
	delete v2topics2;
	delete v2gtopics2;
}



static sBool sdLoadPatch(file &in, sInt pn, sInt fver=-1)
{
	if (fver==-1)
	{
		if (in.read(patchnames[pn],32)<32) return 0;
		if (in.read(&fver,4)<4) return 0;
	}

	int rss=v2vsizes[fver]-255*3-1;
	sU8 *patch=soundmem+128*4+v2soundsize*pn;

	// fill patch with default values
	memcpy(patch,v2initsnd,v2soundsize);

	// load patch
	for (int j=0; j<v2nparms; j++)
		if (v2parms[j].version<=fver)
			patch[j]=in.getsU8();
  patch+=v2nparms;
	in.read(patch,1);
	
	// remap mods
	for (int j=0; j<*patch; j++)
	{
		sU8 *mod=patch+3*j+1;
		if (in.read(mod,3)<3) return 0;
		for (int k=0; k<=mod[2]; k++)
			if (v2parms[k].version>fver) mod[2]++;
	}

	in.seekcur(3*(255-*patch));
//	if (in.read(patch+v2nparms,3*(255-*patch))<3*(255-*patch)) return 0;
	return 1;
}



static sBool sdLoadBank(file &in)
{
	int   i,fver=-1;
	int  sml, pw;

	if (in.read(patchnames,128*32)<128*32) return 0;
	sml=in.getsU32();
	pw=sml/128;

	sU32 globsize;
	in.seekcur(sml);
	in.read(globsize);
	in.seekcur(-(sml+4));

	for (i=0; i<=v2version; i++)
		if (pw==v2vsizes[i] && globsize==v2gsizes[i]) fver=i;

	if (fver==-1)
	{
//		MessageBox("Sound defs in file have unknown size... aborting!",
//				  "Farbrausch ViruZ II",MB_ICONERROR);
		return 0;
	}

	printf("Found version %d\n",fver);

	for (i=0; i<128; i++)
		sdLoadPatch(in,i,fver);

	if (!in.eof())
	{
		memcpy(globals,v2initglobs,v2ngparms);
		sml=in.getsU32();
		if (sml<v2ngparms)
		{
			for (int j=0; j<v2ngparms; j++)
				if (v2gparms[j].version<=fver)
					globals[j]=in.getsU8();
		}
		else
		{
			if (in.read(globals,v2ngparms)<v2ngparms) return 0;
			in.seekcur(sml-v2ngparms);
		}
	}


#ifdef RONAN
	ZeroMemory(speech,64*256);
#endif

	if (!in.eof())
	{
		sml=in.getsU32();
#ifdef RONAN
		if (sml<=64*256)
			in.read(speech,sml);
		else
		{
			in.read(speech,64*256);
			in.seekcur(sml-64*256);
		}
#else
#ifndef SINGLECHN
		if (sml) MessageBox(0,"Warning: Not loading speech synth texts\nIf you overwrite the file later, the texts will be lost!","Farbrausch V2",MB_ICONEXCLAMATION);
#endif
		in.seekcur(sml);
#endif
	}

	return 1;
}



sBool sdLoad(file &in)
{
	char id[5];
	id[4]=0;
	if (in.read(id,4)<4) return 0;
	if (!strcmp(id,"v2p0")) return sdLoadBank(in);
	if (!strcmp(id,"v2p1")) return sdLoadPatch(in,v2curpatch,-1);
	return 0;
}


sBool sdSaveBank(file &out)
{
	if (!out.puts("v2p0")) return 0;

	// 1: patchnamen
	if (out.write(patchnames,128*32)<128*32) return 0;

	// 2: patchdaten
	if (!out.putsU32(smsize-128*4)) return 0;
	if (out.write(soundmem+128*4,smsize-128*4)<smsize-128*4) return 0;

	// 3: globals
	if (!out.putsU32(v2ngparms)) return 0;
	if (out.write(globals,v2ngparms)<v2ngparms) return 0;

#ifdef RONAN
	// 4: speech synth
	if (!out.putsU32(64*256)) return 0;
	if (out.write(speech,64*256)<64*256) return 0;
#else
	if (!out.putsU32(0)) return 0;
#endif

	return 1;
}


sBool sdSavePatch(file &out)
{
	if (!out.puts("v2p1")) return 0;

	if (out.write(patchnames[v2curpatch],32)<32) return 0;

	// 2: patchdaten
	if (!out.putsU32(v2version)) return 0;
	if (out.write(soundmem+128*4+v2soundsize*v2curpatch,v2soundsize)<v2soundsize) return 0;

	return 1;
}


sBool sdImportV2MPatches(file &in, const char *prefix)
{
	fileM mem;
	mem.open(in);
	int len=mem.size();
	sU8 *ptr=(sU8*)mem.detach();

	const sU8 *v2mpatches[128];
	sU8 *newmem;
	sU32 np=GetV2MPatchData(ptr,len,&newmem,v2mpatches);

	for (sU32 i=0; i<np; i++)
	{
		sInt p=(v2curpatch+i)%128;
		memcpy(soundmem+128*4+v2soundsize*p,v2mpatches[i],v2soundsize);

		char buf[256];
		sprintf(buf,"%s %03d",prefix,i);
		buf[31]=0;
		strcpy(patchnames[p],buf);
	}
	
	return np?1:0;
}


void sdCopyPatch()
{
	memcpy(v2clipboard,soundmem+128*4+v2curpatch*v2soundsize,v2soundsize);
	strcpy(v2clipname,patchnames[v2curpatch]);
}

void sdPastePatch()
{
	memcpy(soundmem+128*4+v2curpatch*v2soundsize,v2clipboard,v2soundsize);
	strcpy(patchnames[v2curpatch],v2clipname);
}

void sdInitPatch()
{
	memcpy(soundmem+128*4+v2curpatch*v2soundsize,v2initsnd,v2soundsize);
	sprintf(patchnames[v2curpatch],"Init Patch #%03d",v2curpatch);
}