// This file is distributed under a BSD license. See LICENSE.txt for details.

#pragma once

#include "_types.hpp"

/****************************************************************************/

void Error(sChar *name);
void NewLine(sInt line=0);
void Out(sChar *text);
void OutF(sChar *format,...);

extern sInt Indent;
extern sInt Mode;
extern sInt RealMode;
extern sInt Modify;
extern sChar *LekktorName;
extern sU8 *ModArray;

/****************************************************************************/
