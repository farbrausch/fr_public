// This file is distributed under a BSD license. See LICENSE.txt for details.

#pragma once


/****************************************************************************/
/***                                                                      ***/
/***   basic stuff                                                        ***/
/***                                                                      ***/
/****************************************************************************/

#include "_types.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   application interface                                              ***/
/***                                                                      ***/
/****************************************************************************/

void sAppInit();
void sAppExit();

void sAppOnMouse(sInt dd,sInt mx,sInt my);
void sAppOnKey(sInt key);
void sAppOnPaint(sInt dx,sInt dy,sU16 *buffer);
void sAppOnPrint();

#define sMKEY_ESCAPE 0x1b
#define sMKEY_UP    0x26
#define sMKEY_RIGHT 0x27
#define sMKEY_DOWN  0x28
#define sMKEY_LEFT  0x25
#define sMKEY_ENTER 0x0d
#define sMKEY_APP0  0xc1
#define sMKEY_APP1  0xc2
#define sMKEY_APP2  0xc3
#define sMKEY_APP3  0xc4
#define sMKEY_APP4  0xc5


#define sMKEY_MOB0  0x70
#define sMKEY_MOB1  0x5b
#define sMKEY_MOB2  0x1b
#define sMKEY_MOB3  0x71

#define sMKEY_PHONEUP  0x73
#define sMKEY_PHONEDOWN  0x74


#define sMKEYQ_BREAK  0x80000000

#define sMDD_START  1
#define sMDD_DRAG   2
#define sMDD_STOP   3

/****************************************************************************/
/***                                                                      ***/
/***   system                                                             ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/
/***                                                                      ***/
/***   stuff                                                              ***/
/***                                                                      ***/
/****************************************************************************/

struct sDirEntryCE
{
  sChar Name[32];
  sInt Size;
};

void sProgressStart(sInt max);
void sProgress(sInt count);
void sProgressEnd();
sInt sProgressGet();
void sExit();
void sTerminate(sChar *message);
sInt sGetTime();
void sText(const sChar *format,...);
void sKeepRunning();
const sChar *sGetShellParameter(sInt i);

sU8 *sLoadFile(const sChar *name);
sInt sLoadDirCE(const sChar *pattern,sDirEntryCE *buffer,sInt max);


sInt sFontBegin(sInt pagex,sInt pagey,const sU16 *name,sInt xs,sInt ys,sInt style);
void sFontPrint(sInt x,sInt y,const sU16 *string,sInt len);
void sFontEnd();
sU32 *sFontBitmap();

/****************************************************************************/

inline sU16 sMakeColor(sInt r,sInt g,sInt b) { return ((r&31)<<11)|((g&31)<<6)|((b&31)<<0); }

/****************************************************************************/
/****************************************************************************/
