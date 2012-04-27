/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "main.hpp"
#include "doc.hpp"
#include "base/system.hpp"

#define VERSION 1
#define REVISION 40

/****************************************************************************/

void sMain()
{

  const sChar *name = sGetShellParameter(0,0);
  if(!name)
  {
    sPrintF(L"wz4ops %s %s\n",VERSION,REVISION);
    sPrint(L"usage: wz4ops name.ops\n");
    sPrint(L"this will read name.ops and write name.hpp and name.cpp\n");
    sSetErrorCode();
    return;
  }

  Doc = new Document;
  Doc->SetNames(name);

  if(!Doc->Parse(Doc->InputFileName))
  {
    sSetErrorCode();
  }
  else
  {
    if(!Doc->Output())
    {
      sSetErrorCode();
    }
    else
    {
      // CHAOS: IfDifferent is a bad idea. 
      if(!sSaveTextAnsi/*IfDifferent*/(Doc->HPPFileName,Doc->HPP.Get()))
      {
        sPrintF(L"failed to write output <%s>\n",Doc->HPPFileName);
        sSetErrorCode();
      }
      if(!sSaveTextAnsi/*IfDifferent*/(Doc->CPPFileName,Doc->CPP.Get()))
      {
        sPrintF(L"failed to write output <%s>\n",Doc->CPPFileName);
        sSetErrorCode();
      }
    }
  }

  delete Doc;
}

/****************************************************************************/

