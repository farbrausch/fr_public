#ifndef _SOUNDDEF_H_
#define _SOUNDDEF_H_




enum V2CTLTYPES { VCTL_SKIP, VCTL_SLIDER, VCTL_MB, };

typedef struct {
	int		no;
	char  *name;
} V2TOPIC;

typedef struct {

	int   version;
	char  *name;
	V2CTLTYPES ctltype;
	int		offset, max;
	int   isdest;
	char  *ctlstr;

} V2PARAM;


extern const V2TOPIC v2topics[];
extern const int v2ntopics;
extern const char *v2sources[];
extern const int v2nsources;
extern const V2PARAM v2parms[];
extern const int v2nparms;
extern const unsigned char v2initsnd[];
extern const int v2soundsize;
extern int	 *v2topics2;

extern const V2TOPIC v2gtopics[];
extern const int v2ngtopics;
extern const V2PARAM v2gparms[];
extern const int v2ngparms;
extern const unsigned char v2initglobs[];
extern int	 *v2gtopics2;

extern unsigned char *soundmem;
extern long          *patchoffsets;
extern unsigned char *editmem;
extern const int     smsize;
extern char					 patchnames [128][32];
extern char					 globals[];

extern int           v2version;
extern int           *v2vsizes;
extern int           *v2gsizes;

extern void sdInit();
extern void sdClose();

#endif
