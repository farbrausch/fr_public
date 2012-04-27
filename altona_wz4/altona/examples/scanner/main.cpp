/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "main.hpp"
#include "util/scanner.hpp"


/****************************************************************************/
/****************************************************************************/

enum
{
  TOK_LINEFEED = 0x100,
  TOK_INCLUDE,
};

void sMain()
{
  sScanner scan;

  scan.Init();
  scan.Flags = sSF_ESCAPECODES|sSF_CPPCOMMENT|sSF_NUMBERCOMMENT|sSF_TYPEDNUMBERS|sSF_SEMICOMMENT;
  scan.DefaultTokens();
  scan.AddToken(L"linefeed",TOK_LINEFEED);
  scan.AddToken(L"include",TOK_INCLUDE);
  scan.StartFile(L"test_source.txt");

  sTextBuffer tb;  
  while(scan.Token!=sTOK_END && scan.Errors==0)
  {
    switch(scan.Token)
    {
    case TOK_LINEFEED:
      scan.Scan();
      tb.PrintChar('\n');
      break;
    case TOK_INCLUDE:
      {
        scan.Scan();
        sPoolString name;
        scan.ScanString(name);
        if(scan.IncludeFile(name))
          tb.PrintF(L"[include %q] ",name);
        else
          tb.PrintF(L"[include %q failed] ",name);
      }
      break;
    case '{':
      {
        sPoolString skip;
        scan.ScanRaw(skip,'{','}');
        tb.PrintF(L"[raw %q] ",skip);
      }
      break;
    default:
      scan.Print(tb);
      scan.Scan();
      break;
    }
  }

  sDPrint(tb.Get());
  sPrint(tb.Get());
}

/****************************************************************************/

