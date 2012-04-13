

// ronan heiﬂt in wirklichkeit lisa. ich war nur zu faul zum renamen.


#ifdef RONAN

#include "types.h"
#include "ronan.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

#include <math.h>
#pragma intrinsic (sin, cos, exp, log, atan, atan2, pow)

#define PI (4.0f*(sF32)atan(1.0f))
#define FRAMESIZE_MSEC 100

#include "phonemtab.h"

static const char *sagdas=
//"2EYb2IHl2IH2AXnw2ERdz2AEg4OW DH2AX s4EYl2ERzd2IHz2EYp4IYrd    "
//"2EYst2AOr2IYf2AOrDH2AXCH2IHldr2EHnt2UWr4AAk DH2EHmb2AEkt2UWsl4IYp    "
//"2EYm2IHl2IH2AXnb2ERn2IHNGb4UHks l2AYkt4AOrCH2IHz2IHn2AWrh4AEndz    "
//"2EYf2AEbr2IHk2AXv2AYd4IYlz t2UWd6AEk2AOr2EYt2AWrh4OWmz   "
//" !wIHTH!yUW!AY!stAEn!dIHn!hOW_p "
"!y4UW_!4AY_!st2AE_ndd!IH_n!h4OW_p"
"!DHAE_t!gAA_d!wIH_l!s4EY_v!AH_s!frAA_m!AW_r!2s4EH_lf2s "
"!EH!vRIY!t4AY_m!EY_!w4EY_s!tIH_d!m4OW_m!EH_nt "
"!ah_n!t2IH_l!AE_n!2AH!DHER_!d4EY_!IH_z!l4AO_st "
"!4IY!vEH_n!l2AE_ndz!w2IY_!w2AH_ns!kAO_ld!h4OW_m "
"!l2AY_!AH_n!dIH_s!kAH!v4EH_rd!AE_nd!AH_n!n4OW_n "
"!2OW_n!lIY_!h2AE!vEH_nz!s4AY!l2EH_ns!fAO_r!AE_n!AE_ns!w2ER_ "
//"2w4IY    4AA2r    b4AEk     ";
//  "5AW    T5OW    B5AAn   "
;

static struct 
{
	sU32 samplerate;
	sF32 fcminuspi_sr;
	sF32 fc2pi_sr;
} g;


struct syldef
{
	char syl[4];
	sS8  ptab[4];
} syls[] = {
	{"sil",{50,-1,-1,-1}},
	{"ng",{38,-1,-1,-1}},
	{"th",{57,-1,-1,-1}},
	{"sh",{55,-1,-1,-1}},
	{"dh",{12,51,13,-1}},
	{"zh",{67,51,67,-1}},
	{"ch",{ 9,10,-1,-1}},
	{"jh",{29,30,51,30}},
	{"ih",{25,-1,-1,-1}},
	{"eh",{16,-1,-1,-1}},
	{"ae",{ 1,-1,-1,-1}},
	{"ah",{60,-1,-1,-1}},
	{"oh",{39,-1,-1,-1}},
	{"uh",{42,-1,-1,-1}},
	{"ax",{ 0,-1,-1,-1}},
	{"iy",{17,-1,-1,-1}},
	{"er",{19,-1,-1,-1}},
	{"aa",{ 4,-1,-1,-1}},
	{"ao",{ 5,-1,-1,-1}},
	{"uw",{61,-1,-1,-1}},
	{"ey",{ 2,25,-1,-1}},
	{"ay",{28,25,-1,-1}},
	{"oy",{41,25,-1,-1}},
	{"aw",{45,46,-1,-1}},
	{"ow",{40,46,-1,-1}},
	{"ia",{26,27,-1,-1}},
	{"ea",{ 3,27,-1,-1}},
	{"ua",{43,27,-1,-1}},
	{"ll",{35,-1,-1,-1}},
	{"wh",{63,-1,-1,-1}},
	{"ix",{ 0,-1,-1,-1}},
	{"el",{34,-1,-1,-1}},
	{"rx",{53,-1,-1,-1}},
	{"h",{24,-1,-1,-1}},
	{"p",{47,48,49,-1}},
	{"t",{56,58,59,-1}},
	{"k",{31,32,33,-1}},
	{"b",{ 6, 7, 8,-1}},
	{"d",{11,14,15,-1}},
	{"g",{21,22,23,-1}},
	{"m",{36,-1,-1,-1}},
	{"n",{37,-1,-1,-1}},
	{"f",{20,-1,-1,-1}},
	{"s",{54,-1,-1,-1}},
	{"v",{62,51,62,-1}},
	{"z",{66,51,68,-1}},
	{"l",{34,-1,-1,-1}},
	{"r",{52,-1,-1,-1}},
	{"w",{63,-1,-1,-1}},
	{"q",{51,-1,-1,-1}},
	{"y",{65,-1,-1,-1}},
	{" ",{18,-1,-1,-1}},
};
#define NSYLS (sizeof(syls)/sizeof(syldef))



// filter type 1: 2-pole resonator
struct ResDef
{
	sF32 a,b,c;  // coefficients

	void set(sF32 f, sF32 bw, sF32 gain)
	{
		sF32 r=(sF32)exp(g.fcminuspi_sr*bw);
		c=-(r*r);
		b=r*(sF32)cos(g.fc2pi_sr*f)*2.0f;
		a=gain*(1.0f-b-c);
	}
};

struct Resonator
{
	const ResDef &def;
	sF32 p1, p2; // delay buffers

	Resonator(const ResDef &a_def) : def(a_def) { }

	void reset() { p1=p2=0; }

	sF32 tick(sF32 in)
	{
		sF32 x=def.a*in+def.b*p1+def.c*p2;
		p2=p1;
		p1=x;
		return x;
	}
};

/*
// filter type 2: 2-zero antiresonator
struct Antiresonator : Resonator
{
public:

	void set(sF32 f, sF32 bw, sF32 gain)
	{
		Resonator::set(f,bw,1.0f);
		a=1.0f/a;
		b*=-a;
		c*=-a;
		a*=gain;
	}

	sF32 tick(sF32 in)
	{
		sF32 x=a*in+b*p1+c*p2;
		p2=p1;
		p1=in;
		return x;
	}
};
*/

static struct syVRonan
{
	ResDef f1,f2,f3,f4,nas;
	sF32 a_voicing;
	sF32 a_aspiration;
	sF32 a_frication;
	sF32 a_bypass;
	sF32 a_breath;
	sF32 d_breath;
} v;

static struct syWRonan : syVRonan
{
	// synth
	syVRonan newframe;

	Resonator rf1,rf2,rf3,rf4,rnas;

	sF32 lastin;
	sF32 lastin2;
	sF32 nval;

	// phonem seq
	sInt framecount;  // frame rate divider
	sInt spos;        // pos within syl definition (0..3)
	sInt scounter;    // syl duration divider
	sInt cursyl;      // current syl
	sInt durfactor;   // duration modifier
	sF32 invdur;      // 1.0 / current duration
	const char *ptr;  // pointer to text

	sInt curp1, curp2;

	// sync
	sInt noteoncnt;
	sInt noteoffcnt;
	sInt offphase;

	syWRonan(): rf1(f1), rf2(f2), rf3(f3), rf4(f4), rnas(nas) {};

} w;




static sF32 db2lin(sF32 db)
{
	return (sF32)pow(2,(db-70)/6.0f);
}

static sF32 flerp(const sF32 a,const sF32 b,const sF32 x) { return a+x*(b-a); }

static void SetFrame(const Phoneme &p1, const Phoneme &p2, const sF32 x, syVRonan &dest)
{
	dest.f1.set(flerp(p1.f1f,p2.f1f,x),flerp(p1.f1b,p2.f1b,x),db2lin(flerp(p1.a_1,p2.a_1,x)));
	dest.f2.set(flerp(p1.f2f,p2.f2f,x),flerp(p1.f2b,p2.f2b,x),db2lin(flerp(p1.a_2,p2.a_2,x)));
	dest.f3.set(flerp(p1.f3f,p2.f3f,x),flerp(p1.f3b,p2.f3b,x),db2lin(flerp(p1.a_3,p2.a_3,x)));
	dest.f4.set(3200,200,db2lin(flerp(p1.a_4,p2.a_4,x)));
	dest.nas.set(flerp(p1.fnf,p2.fnf,x),100.0f,db2lin(flerp(p1.a_n,p2.a_n,x)));
	dest.a_voicing=db2lin(flerp(p1.a_voicing,p2.a_voicing,x));
	dest.a_aspiration=db2lin(flerp(p1.a_aspiration,p2.a_aspiration,x))*0.5f;
	dest.a_breath=db2lin(flerp(p1.a_breath,p2.a_breath,x));
	dest.a_frication=db2lin(flerp(p1.a_frication,p2.a_frication,x));
	dest.d_breath=flerp(p1.d_breath,p2.d_breath,x);
	dest.a_bypass=db2lin(flerp(p1.a_bypass,p2.a_bypass,x))*0.1f;
}


//-----------------------------------------------------------------------

#define NOISEGAIN 0.25f

static sF32 noise()
{
	static sF32 nout=0;
	static sU32 seed=0xdeadbeef;
	static union { sU32 i; sF32 f; } val;

	// random...
	seed=(seed*196314165)+907633515;

	// convert to float between 2.0 and 4.0
	val.i=(seed>>9)|0x40000000;

	// slight low pass filter...
	nout=(val.f-3.0f)+0.75f*nout;
	return nout*NOISEGAIN;
}



// -----------------------------------------------------------------------

extern "C" void __stdcall ronanCBSetSR(sU32 sr)
{
	g.samplerate=sr;
	g.fc2pi_sr=2.0f*PI/(sF32)sr;
	g.fcminuspi_sr=-g.fc2pi_sr*0.5f;
}


extern "C" void __stdcall ronanCBInit()
{
	w.rf1.reset();
	w.rf2.reset();
	w.rf3.reset();
	w.rf4.reset();
	w.rnas.reset();

	SetFrame(phonemes[18],phonemes[18],0,w); // off
	SetFrame(phonemes[18],phonemes[18],0,w.newframe); // off
	w.curp1=w.curp2=18;

	w.framecount=0;
	w.spos=4;
	w.scounter=0;
	w.ptr=sagdas;
	w.lastin=w.lastin2=w.nval=0;

	w.noteoncnt=0;
	w.noteoffcnt=0;
	w.offphase=0;
}


const sInt framerate=2;

extern "C" void __stdcall ronanCBTick()
{
	if (w.framecount<=0)
	{
		w.framecount=framerate;
		// let current phoneme expire
		if (w.scounter<=0)
		{
			// set to next phoneme
			w.spos++;
			if (w.spos >=4 || syls[w.cursyl].ptab[w.spos]==-1)
			{
				// go to next syllable

				// fake: cycle text
				if (!(*w.ptr)) w.ptr=sagdas;

				if (*w.ptr=='!' && !w.noteoncnt)
				{
					w.durfactor=1;			
					w.framecount=1;
					w.spos--;
					if (w.offphase)
					{
						w.cursyl=NSYLS-1;
						w.spos=0;
					}
				}
				else if (*w.ptr=='!')
				{
					w.noteoncnt=0;
					w.noteoffcnt=0;
					w.offphase=0;
					w.ptr++;
				}
				else if (*w.ptr=='_' && !w.noteoffcnt)
				{
					w.durfactor=1;			
					w.framecount=1;
					w.spos--;
				}
				else if (*w.ptr=='_')
				{
					w.ptr++;
					w.noteoffcnt=0;
					w.offphase=1;
				}

				if (*w.ptr && *w.ptr!='!' && *w.ptr!='_')
				{
					w.durfactor=0;
					while (*w.ptr>='0' && *w.ptr<='9') w.durfactor=10*w.durfactor+(*w.ptr++ - '0');
					if (!w.durfactor) w.durfactor=1;

					sInt fs;
					for (fs=0; fs<NSYLS; fs++)
					{
						const syldef &s=syls[fs];
						if (!strnicmp(s.syl,w.ptr,strlen(s.syl)))
							break;
					}
					if (fs==NSYLS) fs--; // make fake space char of all unknown chars
					w.cursyl=fs;
					w.spos=0;
					w.ptr+=strlen(syls[fs].syl);
				}
			}
			
			w.curp1=w.curp2;
			w.curp2=syls[w.cursyl].ptab[w.spos];
			//if (!w.spos) w.curp1=w.curp2;
/*
			if (w.spos)
				w.curp1=syls[w.cursyl].ptab[w.spos-1];
			else
				w.curp1=w.curp2;
*/
			w.scounter=phonemes[w.curp2].duration*w.durfactor;
			if (!w.scounter) w.scounter=1;
			w.invdur=1.0f/((sF32)w.scounter*framerate);
		}
		w.scounter--;
	}
	w.framecount--;

	sF32 x=(sF32)(w.scounter*framerate+w.framecount)*w.invdur;

//	SetFrame(phonemes[w.curp2],phonemes[w.curp1],x,w);
	const Phoneme &p1=phonemes[w.curp1];
	const Phoneme &p2=phonemes[w.curp2];
	x=(sF32)pow(x,(sF32)p1.rank/(sF32)p2.rank);
//	if (*w.ptr=='_' || *w.ptr=='!')
		//SetFrame(p2,p2,0,w.newframe);
//	else
		SetFrame(p2,(abs(p2.rank-p1.rank)>8)?p2:p1,x,w.newframe);
//	SetFrame(p2,p1,x,w.newframe);
	  //SetFrame(phonemes[w.curp2],phonemes[w.curp1],0,w.newframe);
//	char bla[256];
//	sprintf(bla,"%d %d %3.2f\n",w.curp2,w.curp1,x);
//	OutputDebugString(bla);

}

extern "C" void __stdcall ronanCBNoteOn()
{
	w.noteoncnt++;
}

extern "C" void __stdcall ronanCBNoteOff()
{
	w.noteoffcnt++;
}

extern "C" void __stdcall ronanCBSetCtl(sU32 ctl, sU32 val)
{
}


static float ndecay=0.95f;


static sF32 hp1=0, hp2=0;

extern "C" void __stdcall ronanCBProcess(sF32 *buf, sU32 len)
{
	static syVRonan deltaframe;

	// prepare interpolation
	{
		sF32 *src1=(sF32*)&w;
		sF32 *src2=(sF32*)&w.newframe;
		sF32 *dest=(sF32*)&deltaframe;
		sF32 mul  =1.0f/(sF32)len;
		for (sU32 i=0; i<(sizeof(syVRonan)/sizeof(sF32)); i++)
			dest[i]=(src2[i]-src1[i])*mul;
	}

	for (sU32 i=0; i<len; i++)
	{
		// interpolate all values
		{
			sF32 *src=(sF32*)&deltaframe;
			sF32 *dest=(sF32*)&w;
			for (sU32 i=0; i<(sizeof(syVRonan)/sizeof(sF32)); i++)
				dest[i]+=src[i];
		}

		sF32 in=buf[2*i];
		sF32 din=2*in-w.lastin;
		w.lastin=in;		
		if (din>0) w.nval+=din;
		if (w.nval>1.0f) w.nval=1.0f;
		w.nval*=w.d_breath;

		// add breath noise
		in+=w.nval*noise()*w.a_breath;
		sF32 nse=noise();

		// add aspiration noise
		in=in*w.a_voicing+nse*w.a_aspiration;

		// process complete input signal with f1/nasal filters
		sF32 out=w.rf1.tick(in);
		out+=w.rnas.tick(in);

		// differentiate input signal, add frication noise
		in=(nse*w.a_frication)+in-w.lastin2;
		w.lastin2=in;

		// process diff/fric input signal with f2, f3, f4 and bypass (phase inverted)
		out=w.rf2.tick(in)-out;
		out=w.rf3.tick(in)-out;
		out=w.rf4.tick(in)-out;
		out=in*w.a_bypass-out;

		// high pass filter
		hp1+=0.007f*(out=out-hp1);
		hp2+=0.007f*(out=out-hp2);

		buf[2*i]=buf[2*i+1]=out;
	}
}



#endif
