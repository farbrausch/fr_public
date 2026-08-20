/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   The win32 startup dialog, as a command line.                       ***/
/***                                                                      ***/
/***   selector_win.cpp puts up a window with resolution, fullscreen and   ***/
/***   "reserve one core" controls before the demo starts. There is no     ***/
/***   equivalent here, so the same choices are read off the command line  ***/
/***   and everything else falls back to what the project asked for.       ***/
/***                                                                      ***/
/****************************************************************************/

#include "selector_win.hpp"
#include "wz4lib/doc.hpp"

#if sPLATFORM!=sPLAT_WINDOWS

/****************************************************************************/

sBool bOpenSelector(const bSelectorSetup &setup, bSelectorResult &result)
{
  sClear(result);
  result.HiddenPart = -1;

  // resolution: -res <x>x<y>, else what the project stored, else 1280x720

  sInt sx = setup.DialogScreenX;
  sInt sy = setup.DialogScreenY;
  if(sx<=0 || sy<=0) { sx = 1280; sy = 720; }

  const sChar *res = sGetShellString(L"res",L"-resolution");
  if(res)
  {
    const sChar *scan = res;
    sInt rx=0,ry=0;
    sBool ok = sScanInt(scan,rx) && (*scan=='x' || *scan=='X');
    if(ok) { scan++; ok = sScanInt(scan,ry); }
    if(ok && rx>0 && ry>0)
    {
      sx = rx;
      sy = ry;
    }
    else
    {
      sPrintF(L"could not read -res %s, expected something like 1920x1080\n",res);
    }
  }

  result.Mode.Display = -1;
  result.Mode.ScreenX = sx;
  result.Mode.ScreenY = sy;
  result.Mode.Aspect = sF32(sx)/sF32(sy);
  result.Mode.MultiLevel = -1;
  result.Mode.OverMultiLevel = -1;

  if(sGetShellSwitch(L"fullscreen"))
    result.Mode.Flags |= sSM_FULLSCREEN;

  result.Loop = sGetShellSwitch(L"loop");
  result.Benchmark = sGetShellSwitch(L"benchmark");
  result.OneCoreForOS = sGetShellSwitch(L"onecore");
  result.LowQuality = sGetShellSwitch(L"lowquality") ||
                      (setup.DialogFlags & wDODF_LowQuality);

  // -part <n> picks one of the hidden parts the project declared

  const sChar *part = sGetShellString(L"part",L"-part");
  if(part)
  {
    sInt n=0;
    if(sScanInt(part,n))
      result.HiddenPart = n;
  }

  return sTRUE;
}

/****************************************************************************/

#endif // !sPLAT_WINDOWS
