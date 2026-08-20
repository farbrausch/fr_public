/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_SCREENMODE_SELECTOR_WIN_HPP
#define FILE_SCREENMODE_SELECTOR_WIN_HPP

#include "base/types.hpp"
#include "base/graphics.hpp"

// The dialog itself is win32, but its two structs are the player's only way to
// say "run at this resolution", so they have to exist on every platform.

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

// Fills in `result` and returns sTRUE to start the demo, sFALSE to quit.
sBool bOpenSelector(const bSelectorSetup &setup, bSelectorResult &result);

/****************************************************************************/

#endif // FILE_SCREENMODE_SELECTOR_WIN_HPP

