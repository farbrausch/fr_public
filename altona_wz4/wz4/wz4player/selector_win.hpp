/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_SCREENMODE_SELECTOR_WIN_HPP
#define FILE_SCREENMODE_SELECTOR_WIN_HPP

#include "base/types.hpp"

#if sPLATFORM==sPLAT_WINDOWS

#include "base/graphics.hpp"

/****************************************************************************/

struct bSelectorSetup
{
  sString<1024> Title;
  sInt IconInt; // resource # (0: none)
  sString<1024> IconURL;
  sString<32> Caption;
  sString<64> SubCaption;

  sInt DialogFlags;   // wDODF_???
  sInt DialogScreenX;
  sInt DialogScreenY;
  const sChar *HiddenPartChoices;
//  sBool BenchmarkButton;

  struct ShareSite
  {
    sInt IconInt;
    sString<1024> URL;
  } Sites[8];

  bSelectorSetup() { sClear(*this); }

};

struct bSelectorResult
{
  sScreenMode Mode;

  sBool Loop;
  sBool Benchmark;
  sBool OneCoreForOS;
  sBool LowQuality;
  sInt HiddenPart;      // -1 = none, 0..n
};

sBool bOpenSelector(const bSelectorSetup &setup, bSelectorResult &result);

#endif

/****************************************************************************/

#endif // FILE_SCREENMODE_SELECTOR_WIN_HPP

