/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#define sPEDANTIC_WARN 1
#include "base/types2.hpp"
#include "util/scanner.hpp"

sScanner *Scan;


/****************************************************************************/

void _Global()
{
  while(!Scan->Errors && Scan->Token!=sTOK_END)
  {
    sPoolString name;

    Scan->ScanName(name);
    Scan->Match('=');
    if(Scan->Token==sTOK_STRING)
    {
      sPrintF(L"%s = \"%s\"\n",name,Scan->String);
      Scan->Scan();
    }   
    else if(Scan->Token==sTOK_INT)
    {
      sPrintF(L"%s = %d\n",name,Scan->ValI);
      Scan->Scan();
    }
    else if(Scan->Token==sTOK_FLOAT)
    {
      sPrintF(L"%s = %f\n",name,Scan->ValF);
      Scan->Scan();
    }
    else
    {
      Scan->Error(L"syntax");
    }
    Scan->Match(';');
  }
}

/****************************************************************************/

void sMain()
{
  Scan = new sScanner();
  Scan->Init();
  Scan->DefaultTokens();
  Scan->Flags = sSF_CPPCOMMENT|sSF_ESCAPECODES|sSF_MERGESTRINGS;
  Scan->StartFile(L"source.txt");
  _Global();
}

/****************************************************************************/

