/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) 2005-2006 by Dierk Ohlerich                          ***/
/***   all rights reserverd                                               ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_BASE_SYSTEM_IOS_HPP
#define FILE_BASE_SYSTEM_IOS_HPP

//#include "base/types.hpp"

/****************************************************************************/

void IOSInit1();
void IOSInit2();
void IOSInit3();
void IOSResize1();
void IOSResize2();
void IOSRender(int ms,unsigned long long us);
void IOSExit();

enum 
{
  IOS_MaxTouches = 11,
};

struct IOS_TouchData
{
  int x,y,c;
};

extern IOS_TouchData IOS_Touches[IOS_MaxTouches];
extern int IOS_TouchCount;

/****************************************************************************/

#endif