/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "base/system.hpp"

/****************************************************************************/

struct UltraSuper
{
  static void TurboCrash()
  {
    sFatal(L"AUA!");
  }
};

void TestStackDump()
{
  UltraSuper::TurboCrash();
}

void sMain()
{
  TestStackDump();
}

/****************************************************************************/

