// This file is distributed under a BSD license. See LICENSE.txt for details.

// ronan heiﬂt in wirklichkeit lisa. ich war nur zu faul zum renamen.

// !DHAX_ !kwIH_k !br4AH_UHn !fAA_ks !jAH_mps !OW!vE_R !DHAX_ !lEY!zIY_ !dAA_g 

#include "_types.hpp"

#define RONAN 1
#ifdef RONAN

//
#ifdef _DEBUG
extern void __cdecl printf2(const char *format, ...);
#else
#define printf2
#endif


#pragma pack (push, 1)

struct Phoneme
{
  sF32 f1f, f1b;
  sF32 f2f, f2b;
  sF32 f3f, f3b;
  sF32 fnf;
  sF32  a_voicing, a_aspiration, a_frication, a_bypass;
  sF32  a_1, a_2, a_3, a_4, a_n, a_56;
  sF32  duration, rank;
}; 

#pragma pack (pop)

//  {  490,   60, 1480,   90, 2500,  150,  270,  0,  0,  0,-16,-16,-16,-16,-16,-16,-16,  0,  5, 31}, //  18: end
//  {  280,   60, 1720,   90, 2560,  150,  270, 62,  0,  0,-16, 43, 38, 38, 50,-16,-16,  0,  4, 20}, //  68: zz

// f1f: /10
// f1b: /10
// f2f: /20
// f2b: /10
// f3f: /20
// f3b: /10
// fnf: /10

#define NPHONEMES 69
#define PTABSIZE  1311

static const sU8 multipliers[PTABSIZE/NPHONEMES] = { 10,10,20,10,20,10,10,1,1,1,1,1,1,1,1,1,1,1,1};


// CHANGES:  a_f56 + 15 (-> duration -15)
//           a_bypass -3 (-> a_f1 +3)

static const sU8 rawphonemes[PTABSIZE] =
{
//   0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32  33  34  35  36  37  38  39  40  41  42  43  44  45  46  47  48  49  50  51  52  53  54  55  56  57  58  59  60  61  62  63  64  65  66  67  68 
// f1f (Hz*1/10)
    49, 30,241,  0, 15,226,226,  0,  0,  0, 21,235,  9,  0,247,  0, 45,217, 24,  9,238,235,  0,  0, 30,247,247, 18, 30,196,  9,247,  0,  0, 27,  0,  2,  0,  0, 13,244,  0,244,  0, 12, 30,214,238,  0,  0, 30,235, 21,  0,247,  0,235, 21,235,  0, 51,211,  3,247,  0,  6,  3,  0,  0,
// f1b (Hz*1/10)
   234,  7,249,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,254,  0, 12,246,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 14,242,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// f2f (Hz*1/20)
    68, 15,247, 21,199,253,253,  0,  0, 51, 12,244,247,  0,  9,  0, 12, 15,214,253,  0,  3,  0,  0,  0, 30,  6,220,226, 45, 12,229,  0,  0,  0,229,  3, 39,208,  3, 30,223,  9,  0,247, 24,241,244,  0,  0, 36,253,244,  0, 27, 24,235,  0,  0,  0,235,232, 27,223, 36, 51,217, 15,241,
// f2b (Hz*1/10)
   179,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8, 13,241,250,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// f3f (Hz*1/20)
   116,  0,  0,  0,  0,  0,  0,  0,  0,  9,250,  6,250,  0,  6,  0,247, 35,221,  0,  3,  3,  0,  0,250,  3, 18,235,  0,  9,250,  3,  0,  0,250,  0,241, 21,  9,241,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  3,208,  0, 51,253,  6,  0,  0,  0,247,241, 18,229, 30, 18,235,  0,  0,
// f3b (Hz*1/10)
   143,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,253, 14,240,  5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,248, 15,249,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// fnf (Hz*1/10)
    12,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  9,  9,247,247,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// a_voicing (dB)
    35,  0,  0,  0,  0,  0,  0,  0,  0,194,  0, 62,230, 26,  0,  0,  0,  0,194, 62,194, 62,  0,  0,194, 62,  0,  0,  0,  0,  0,194,  0,  0, 62,  0,  0,  0,246, 10,  0,  0,  0,  0,  0,  0,  0,194,  0,  0,  0, 62,  0,244,206,  0,  0,  0,  0,  0, 62,  0,  0,  0,194, 62,234, 22,  0,
// a_aspiration (dB)
   194,  0,  0,  0,  0,  0,  0,  0,  0, 60,  0,196,  0,  0,  0,  0,  0,  0,  0,  0, 32,224,  0,  0, 60,196,  0,  0,  0,  0,  0, 60,  0,  0,196,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 60,  0,  0,196,  0,  0,  0, 32, 28,  0,  0,  0,  0,196,  0,  0,  0, 60,196, 20,236,  0,
// a_frication (dB)
     0,  0,  0,  0,  0,  0,  0,  0,  0, 60,  0,196, 60,196,  0,  0,  0,  0,  0,  0, 54,202,  0,  0, 60,196,  0,  0,  0,  0,  0, 60,  0,  0,196,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 60,  0,  0,196,  0,  0,  0, 60,  0,  0,  0,  0,  0,196,  0,  0,  0, 60,196, 60,196,  0,
// a_bypass (dB)
   237,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 70,186,  0,  0,  0,  0,  0,  0, 70,186,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// a_f1 (dB)
    83,  0,  0,  0,  0,  0,230,  0,202,  0,  0, 61,254,  0,  9,  0, 12,  0,176, 80,176, 65,  0,  0,  0, 15,  0,  0,  0,237,254,197,  0,  0, 66,  0,246,  9,241, 30,  0,  0,  0,  0,  0,  0,  0,176, 54,  0,202, 59, 13,  0,184,  0,  0,  0,  0,  0, 80,  0,235, 14,183, 80,235,  0,  0,
// a_f2 (dB)
    21,253,254,253,  7,252,181, 79,177,  0, 61,195, 61,  0,  7,246, 14,247,193, 75,211,226, 75,246,  1,  0,255, 15,255,177, 56,200, 80,246,242,  0,  4,  5,251, 17,  3,251,253,  0,  3,  2,251,184, 79,245,188, 70,251,  0,249,  3,195, 56,200,  0, 73,251,  2,244, 12,249,247,  2,254,
// a_f3 (dB)
     9,  5,253,  3,247,249,204, 73,183,  0, 72,184, 56,  0,  9,245, 14,254,190, 63,223,226, 70,245,253,  9,  1,253,252,197, 66,190, 80,246,242,  0,  7,  2,  0,243, 11,245,  6,  0,250, 13,249,198, 73,246,193, 66,255,  0,249, 14,184, 58, 10,246,  3,242, 19,241, 19,254,242, 12,244,
// a_f4 (dB)
     2,  5,254,  2,247,251,209, 68,188,  0, 61,195, 58,  0, 17,246,252,  0,195, 56,200,  0, 54,246,  8,  7,  2,251,252,204, 56,200, 59,246,  2,  0,205, 50,236, 15, 11,247,  5,  0,251, 11,250,204, 68,246,198, 63,193,  0, 70,247,195, 52, 28,246,240,242, 23,193, 49, 12,  5,246, 10,
// a_fn (dB)
   190,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 72,  0,  0,184,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 56,200,  0, 66,246, 10,246,  0,246,210,  0, 56,200, 46,210, 66,236,  0,
// a_f56 (dB)
   225,  0,  0,  0,  0,  0,  0,  0,  0, 46,246,  0, 10,236,246,240,  0,  0,  0,  0, 46,210,  0,  0, 26,230,  0,  0,  0, 26,246, 10,  0,246,240,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 46,236, 10,246,230,  0,  0,  0,  0,  0,  0,  0,  0, 26,  0,
// duration (frames)
   235,  1,  1,  0,  9,251,  2,245,255,  4,  4,  0,252,  0,253,  0,  3,  3,254, 11,252,  0,245,  1,  3,  1,  0,  0,  0,254,255,  5,249,  3,  4,  0,  0,  0,  0,254,  0,  0,254,  2,  0,  0,  0,  2,249,  1,  4,250, 11,255,  2,  0,250,  9,242,  1,  4,  3,251,  4,  4,251,253,  0,  0,
// rank
   254,  0,  0,  0,  0,  0, 24,  3,253,253,251,  8,250,  0,  9,253,232,  0, 29,227, 16,  8,  3,253,239,249,  0,  0,  0, 24,250,  3,  6,250,244,  0,  0,  0,  0,247,  0,  0,  0,  0,  0,  0,  0, 21,  6,250,  6,  1,236,  0,  8,  0,  5,251, 11,250,235,  0, 18,246,  8,248, 10,  0,  0,
}; 

// ronan heiﬂt in wirklichkeit lisa. ich war nur zu faul zum renamen.

// !DHAX_ !kwIH_k !br4AH_UHn !fAA_ks !jAH_mps !OW!vE_R !DHAX_ !lEY!zIY_ !dAA_g 


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

#include <math.h>
#pragma intrinsic (sin, cos, exp, log, atan, atan2, pow)

__forceinline mystrnicmp1(const char *a, const char *b)
{
	sInt l=0;
	while (*a && *b)
		if ((*a++ | 0x20)!=(*b++ | 0x20))
			return 0;
		else
			l++;
	return *a?0:l;
}

#define PI (4.0f*(sF32)atan(1.0f))

static Phoneme phonemes[NPHONEMES];

static struct 
{
	sU32 samplerate;
	sF32 fcminuspi_sr;
	sF32 fc2pi_sr;
} g;


static const char *texts[64]={0};
static const char *nix="";
static sF32  framerate, pitch;

static const struct syldef
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
	{"j",{29,30,51,30}},
	{" ",{18,-1,-1,-1}},
};
#define NSYLS (sizeof(syls)/sizeof(syldef))

// filter type 1: 2-pole resonator
struct ResDef
{
	sF32 a,b,c;  // coefficients

	void set(sF32 f, sF32 bw, sF32 gain)
	{		
		sF32 r=(sF32)sFExp(g.fcminuspi_sr*bw);
		c=-(r*r);
		b=r*(sF32)cos(g.fc2pi_sr*f)*2.0f;
		a=gain*(1.0f-b-c);
	}
};

struct Resonator
{
	ResDef *def;
	sF32 p1, p2; // delay buffers

	inline void setdef(ResDef &a_def) { def=&a_def; }

	sF32 tick(sF32 in)
	{
		sF32 x=def->a*in+def->b*p1+def->c*p2;
		p2=p1;
		p1=x;
		return x;
	}
};


static ResDef d_peq1;

struct syVRonan
{
	ResDef rdef[7]; // f1,f2,f3,f4,nas;
//		ResDef f5,f6;
	sF32 a_voicing;
	sF32 a_aspiration;
	sF32 a_frication;
	sF32 a_bypass;
};

static struct syWRonan : syVRonan
{
	syVRonan newframe;

//		Resonator rf1,rf2,rf3,rf4,rnas,rf5,rf6;
	Resonator res[7];  // 0:nas, 1..6: 1..6

	sF32 lastin2;

	// noise
	sU32 nseed;
	sF32 nout;

	// phonem seq
	sInt framecount;  // frame rate divider
	sInt spos;        // pos within syl definition (0..3)
	sInt scounter;    // syl duration divider
	sInt cursyl;      // current syl
	sInt durfactor;   // duration modifier
	sF32 invdur;      // 1.0 / current duration
	const char *baseptr; // pointer to start of text
	const char *ptr;  // pointer to text
	sInt curp1, curp2;  // current/last phonemes

	// sync
	sInt wait4on;
	sInt wait4off;

	// post EQ
	sF32 hpb1, hpb2;
	Resonator peq1;

	
} workspace;

static syWRonan *wsptr;



static sF32 flerp(const sF32 a,const sF32 b,const sF32 x) { return a+x*(b-a); }
static sF32 db2lin(sF32 db1, sF32 db2, sF32 x) { return (sF32)sFPow(2,(flerp(db1,db2,x)-70)/6.0f); }


static const sF32 f4=3200;
static const sF32 f5=4000;
static const sF32 f6=6000;
static const sF32 bn=100;
static const sF32 b4=200;
static const sF32 b5=500;
static const sF32 b6=800;




static void SetFrame(const Phoneme &p1s, const Phoneme &p2s, const sF32 x, syVRonan &dest)
{
	register syWRonan &w=*wsptr;

  static Phoneme p1,p2;
	static const sF32 * const p1f[]={&p1.fnf,&p1.f1f,&p1.f2f,&p1.f3f,&f4    ,&f5     ,&f6};
	static const sF32 * const p1b[]={&bn    ,&p1.f1b,&p1.f2b,&p1.f3b,&b4    ,&b5     ,&b6};
	static const sF32 * const p1a[]={&p1.a_n,&p1.a_1,&p1.a_2,&p1.a_3,&p1.a_4,&p1.a_56,&p1.a_56};
	static const sF32 * const p2f[]={&p2.fnf,&p2.f1f,&p2.f2f,&p2.f3f,&f4    ,&f5     ,&f6};
	static const sF32 * const p2b[]={&bn    ,&p2.f1b,&p2.f2b,&p2.f3b,&b4    ,&b5     ,&b6};
	static const sF32 * const p2a[]={&p2.a_n,&p2.a_1,&p2.a_2,&p2.a_3,&p2.a_4,&p2.a_56,&p2.a_56};

  sCopyMem(&p1,(sPtr)&p1s,sizeof(Phoneme));
  sCopyMem(&p2,(sPtr)&p2s,sizeof(Phoneme));

	for (sInt i=0; i<7; i++)
		dest.rdef[i].set(flerp(*p1f[i],*p2f[i],x)*pitch,flerp(*p1b[i],*p2b[i],x),db2lin(*p1a[i],*p2a[i],x));

	dest.a_voicing=db2lin(p1.a_voicing,p2.a_voicing,x);
	dest.a_aspiration=db2lin(p1.a_aspiration,p2.a_aspiration,x);
	dest.a_frication=db2lin(p1.a_frication,p2.a_frication,x);
	dest.a_bypass=db2lin(p1.a_bypass,p2.a_bypass,x);
}


//-----------------------------------------------------------------------

#define NOISEGAIN 0.25f

static sF32 noise()
{
	register syWRonan &w=*wsptr;

	union { sU32 i; sF32 f; } val;

	// random...
	w.nseed=(w.nseed*196314165)+907633515;

	// convert to float between 2.0 and 4.0
	val.i=(w.nseed>>9)|0x40000000;

	// slight low pass filter...
	w.nout=(val.f-3.0f)+0.75f*w.nout;
	return w.nout*NOISEGAIN;
}



// -----------------------------------------------------------------------

static void reset()
{
	register syWRonan &w=*wsptr;

	memset(wsptr,0,sizeof(syWRonan));
	for (sInt i=0; i<7; i++) w.res[i].setdef(w.rdef[i]);
	w.peq1.setdef(d_peq1);
	SetFrame(phonemes[18],phonemes[18],0,w); // off
	SetFrame(phonemes[18],phonemes[18],0,w.newframe); // off
	w.curp1=w.curp2=18;
	w.spos=4;
}

extern "C" void __stdcall ronanCBSetSR(sU32 sr)
{
	g.samplerate=sr;
	g.fc2pi_sr=2.0f*PI/(sF32)sr;
	g.fcminuspi_sr=-g.fc2pi_sr*0.5f;
}


extern "C" void __stdcall ronanCBInit()
{

	// convert phoneme table to a usable format
	register const sS8 *ptr=(const sS8*)rawphonemes;
	register sS32 val=0;
	for (sInt f=0; f<(PTABSIZE/NPHONEMES); f++)
	{
		sF32 *dest=((sF32*)phonemes)+f;
		for (sInt p=0; p<NPHONEMES; p++)
		{
			*dest=multipliers[f]*(sF32)(val+=*ptr++);
			dest+=PTABSIZE/NPHONEMES;
		}
	}


	wsptr=&workspace;

	register syWRonan &w=*wsptr;
	reset();

	framerate=3;
	pitch=1.0f;

	if (!texts[0])
		w.baseptr=w.ptr=nix;
	else
		w.baseptr=w.ptr=texts[0];

	/*w.lastin=*/w.lastin2=/*w.nval=*/0;

	d_peq1.set(12000,4000,2.0f);
	
}



extern "C" void __stdcall ronanCBTick()
{
	register syWRonan &w=*wsptr;
	if (w.wait4off || w.wait4on) return;

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

				if (!(*w.ptr)) // empty text: silence!
				{
					w.durfactor=1;			
					w.framecount=1;
					w.cursyl=NSYLS-1;
					w.spos=0;
					w.ptr=w.baseptr;
				}
				else if (*w.ptr=='!') // wait for noteon
				{
					w.framecount=0;
					w.scounter=0;
					w.wait4on=1;			
					w.ptr++;
					return;
				}
				else if (*w.ptr=='_') // noteoff
				{
					w.framecount=0;
					w.scounter=0;
					w.wait4off=1;
					w.ptr++;
					return;
				}

				if (*w.ptr && *w.ptr!='!' && *w.ptr!='_')
				{
					w.durfactor=0;
					while (*w.ptr>='0' && *w.ptr<='9') w.durfactor=10*w.durfactor+(*w.ptr++ - '0');
					if (!w.durfactor) w.durfactor=1;

					sInt fs,len=1,len2;
					for (fs=0; fs<NSYLS-1; fs++)
					{
						const syldef &s=syls[fs];
						if (len2=mystrnicmp1(s.syl,w.ptr))
						{
							len=len2;
							break;
						}
					}
					w.cursyl=fs;
					w.spos=0;
					w.ptr+=len;
				}
			}

			w.curp1=w.curp2;
			w.curp2=syls[w.cursyl].ptab[w.spos];
			w.scounter=phonemes[w.curp2].duration*w.durfactor;
			if (!w.scounter) w.scounter=1;
			w.invdur=1.0f/((sF32)w.scounter*framerate);
		}
		w.scounter--;
	}

	w.framecount--;
	sF32 x=(sF32)(w.scounter*framerate+w.framecount)*w.invdur;
	const Phoneme &p1=phonemes[w.curp1];
	const Phoneme &p2=phonemes[w.curp2];
	x=(sF32)sFPow(x,(sF32)p1.rank/(sF32)p2.rank);
	SetFrame(p2,(abs(p2.rank-p1.rank)>8)?p2:p1,x,w.newframe);

}

extern "C" void __stdcall ronanCBNoteOn()
{
	workspace.wait4on=0;
}

extern "C" void __stdcall ronanCBNoteOff()
{
	workspace.wait4off=0;
}


extern "C" void __stdcall ronanCBSetCtl(sU32 ctl, sU32 val)
{
	// controller 4, 0-63			: set text #
	// controller 4, 64-127		: set frame rate
	// controller 5					: set pitch
	register syWRonan &w=*wsptr;

	
	switch (ctl)
	{
	case 4:
		if (val<63)
		{
			reset();

			if (texts[val])
				w.ptr=w.baseptr=texts[val];
			else
				w.ptr=w.baseptr=nix;
		}
		else
			framerate=val-63;
		break;
	case 5:
		pitch=(sF32)sFPow(2.0f,(val-64.0f)/128.0f);
		break;

	}
}


extern "C" void __stdcall ronanCBProcess(sF32 *buf, sU32 len)
{
	register syWRonan &w=*wsptr;
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

		// add aspiration noise
		in=in*w.a_voicing+noise()*w.a_aspiration;

		// process complete input signal with f1/nasal filters
		sF32 out=w.res[0].tick(in)+w.res[1].tick(in);


		// differentiate input signal, add frication noise
		sF32 lin=in;
		in=(noise()*w.a_frication)+in-w.lastin2;
		w.lastin2=lin;

		// process diff/fric input signal with f2..f6 and bypass (phase inverted)
		for (sInt r=2; r<7; r++)
			out=w.res[r].tick(in)-out;

		out=in*w.a_bypass-out;

		// high pass filter
		w.hpb1+=0.012f*(out=out-w.hpb1);
		w.hpb2+=0.012f*(out=out-w.hpb2);

		// EQ
		out=w.peq1.tick(out)-out;

		buf[2*i]=buf[2*i+1]=out;
	}

}

void __stdcall synthSetLyrics(const char **a_ptr)
{
	register syWRonan &w=*wsptr;
	memcpy(texts,a_ptr,64*sizeof(char *));
	w.baseptr=w.ptr=texts[0];
}

#endif
