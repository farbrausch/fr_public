// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __LEKTOR_HPP__
#include "_types.hpp"

/****************************************************************************/

class sLekktor
{
public:
  sChar *Name;
  sU8 Modify[0x10000];
  void Init(sChar *name);
  void Exit();
  void Set(sInt);
};

void sLekktorInit();
void sLekktorExit();

/****************************************************************************/

#endif
