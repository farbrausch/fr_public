#include "types.h"
#include "v2mconv.h"
#include "sounddef.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "debug.h"

// nicht drüber nachdenken.
static struct _ssbase {
	const sU8   *patchmap;
	const sU8   *globals;
	sU32	timediv;
	sU32	timediv2;
	sU32	maxtime;
	const sU8   *gptr;
	sU32  gdnum;
  struct _basech {
		sU32	notenum;
		const sU8		*noteptr;
		sU32	pcnum;
		const sU8		*pcptr;
		sU32	pbnum;
		const sU8		*pbptr;
		struct _bcctl {
			sU32	ccnum;
			const sU8		*ccptr;
		} ctl[7];
	} chan[16];

	const sU8 *mididata;
	sInt midisize;
	sInt patchsize;
	sInt globsize;
	sInt maxp;

} base;


static void readfile(const unsigned char *inptr, const int inlen)
{
	const sU8 *d=inptr;
	memset(&base,0,sizeof(base));

	base.timediv=(*((sU32*)(d)));
	base.timediv2=10000*base.timediv;
	base.maxtime=*((sU32*)(d+4));
	base.gdnum=*((sU32*)(d+8));
	d+=12;
	base.gptr=d;
  d+=10*base.gdnum;
	for (sInt ch=0; ch<16; ch++)
	{
		_ssbase::_basech &c=base.chan[ch];
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
	base.midisize=d-inptr;
	base.globsize=*((sU32*)d);
	d+=4;
	base.globals=d;
	d+=base.globsize;
	base.patchsize=*((sU32*)d);
	d+=4;
	base.patchmap=d;
	d+=base.patchsize;
}


// gives version deltas
int CheckV2MVersion(const unsigned char *inptr, const int inlen)
{
	int i;
	readfile(inptr,inlen);

	// determine highest used patch from progchange commands
	// (midiconv remaps the patches so that they're a 
	//  continuous block from 0 to x)
	base.maxp=0;
	for (int ch=0; ch<16; ch++)
	{
		for (int i=0; i<(int)base.chan[ch].pcnum; i++)
		{
			sInt p=base.chan[ch].pcptr[3*base.chan[ch].pcnum+i];
			// WORKAROUND: omit illegal patch numbers
			// (bug in midiconv? bug in logic's .mid export?)
			if (p<0x80)
				if (p>=base.maxp) base.maxp=p+1;
		}
	}

	if (!base.maxp)
		return -1;

	// offset table to ptaches
	sInt *poffsets=(sInt *)base.patchmap;

	sInt matches=0, best=-1;
	// for each version check...
	for (i=0; i<=v2version; i++)
	{
		// ... if globals size is correct...
		if (base.globsize==v2gsizes[i])
		{
			// ... and if the size of all patches makes sense
      int p;
			for (p=0; p<base.maxp-1; p++)
			{
				sInt d=(poffsets[p+1]-poffsets[p])-(v2vsizes[i]-3*255);
				if (d%3) break;
				d/=3;
				if ( d != base.patchmap[poffsets[p]+v2vsizes[i]-3*255-1])
					break;
			}

		  if (p==base.maxp-1)
			{
				best=i;
				matches++;
			}
		}
	}

	// if we've got exactly one match, we're quite sure
  sInt ret=(matches==1)?v2version-best:-1;
  return ret;
}

sInt transEnv(sInt enVal)
{
  sF32 dv=1.0f-pow(2.0f, -enVal/128.0f*11.0f);
  dv=sqrt(dv); // square root to correct value
  sInt nEnVal=-log(1.0f-dv)/log(2.0f)*128.0f/11.0f;
  if (nEnVal<0)
    nEnVal=0;
  else if (nEnVal>127)
    nEnVal=127;

  return nEnVal;
}

void ConvertV2M(const unsigned char *inptr, const int inlen, unsigned char **outptr, int *outlen)
{
	int i,p;
	// check version
	sInt vdelta=CheckV2MVersion(inptr,inlen);
	if (!vdelta) // if same, simply clone
	{
		*outptr=new sU8[inlen];
		*outlen=inlen;
		memcpy(*outptr,inptr,*outlen);
		return;
	}
	else if (vdelta<0) // if invalid...
	{
		*outptr=0;
		*outlen=0;
		return;
	}
	vdelta=v2version-vdelta;

	// calc new size
	int newsize=inlen+v2gsizes[v2version]-v2gsizes[vdelta]+base.maxp*(v2vsizes[v2version]-v2vsizes[vdelta]);

	// init new v2m
	*outlen=newsize;
	sU8 *newptr=*outptr=new sU8[newsize];

	// copy midi data
	memcpy(newptr,inptr,base.midisize);
	
	// new globals length...
	newptr+=base.midisize;
	*(sInt *)newptr=v2ngparms;
	newptr+=4;

	// copy/remap globals
	memcpy(newptr,v2initglobs,v2ngparms);
	const sU8 *oldgptr=base.globals;
	for (i=0; i<v2ngparms; i++)
	{
		if (v2gparms[i].version<=vdelta)
			newptr[i]=*oldgptr++;
	}
	newptr+=v2ngparms;

	// new patch data length
	*(sInt *)newptr=base.patchsize+base.maxp*(v2vsizes[v2version]-v2vsizes[vdelta]);
	newptr+=4;

	sInt *poffsets=(sInt *)base.patchmap;
	sInt *noffsets=(sInt *)newptr;
	const int ros=v2vsizes[vdelta]-255*3-1;

	// copy patch table...
	for (p=0; p<base.maxp; p++)
		noffsets[p]=poffsets[p]+p*(v2vsizes[v2version]-v2vsizes[vdelta]);

	newptr+=4*base.maxp;
	
	// for each patch...
	for (p=0; p<base.maxp; p++)
	{
		const sU8 *src=base.patchmap+poffsets[p];

		// fill patch with default values
		memcpy(newptr,v2initsnd,v2nparms);

		// copy/remap patch data
		for (i=0; i<v2nparms; i++)
		{
          if (v2parms[i].version<=vdelta)
          {
  			*newptr=*src++;
            if (vdelta<2 && (i==33 || i==36 || i==39 || i==42)) // fix envelopes
              *newptr=transEnv(*newptr);
          }
		  newptr++;
		}

		// copy mod number
		const sInt modnum=*newptr++=*src++;

		// copy/remap modulations
		for (i=0; i<modnum; i++)
		{
			newptr[0]=src[0];
			newptr[1]=src[1];
			newptr[2]=src[2];
			for (int k=0; k<=newptr[2]; k++)
				if (v2parms[k].version>vdelta) newptr[2]++;
			newptr+=3;
			src+=3;
		}
	}
}

