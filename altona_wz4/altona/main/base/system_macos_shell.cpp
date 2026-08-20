/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   The macOS platform layer, for builds that have no platform.        ***/
/***                                                                      ***/
/***   A command line tool (wz4ops, asc, ...) links none of Cocoa: it does ***/
/***   its work in sMain and main() returns straight after. These two are  ***/
/***   all that system_linux.cpp and types2.cpp still reach for, and the   ***/
/***   window they would talk to does not exist here.                     ***/
/***                                                                      ***/
/****************************************************************************/

#include "base/types.hpp"

#if defined(__APPLE__) && sCOMMANDLINE

// main() calls this after sMain to hand control to the message loop. There is
// nothing to hand it to, and returning is exactly right: it lets main() run
// its shutdown and exit.
void sRunMacApp()
{
}

// wakes the message loop up from another thread
void sTriggerEvent(sInt event)
{
}

#endif
