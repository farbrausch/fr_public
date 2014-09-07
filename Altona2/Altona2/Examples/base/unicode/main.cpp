/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "altona2/libs/base/base.hpp"

void Altona2::Main()
{
  sPrint ("print umlaut a, umlaut o, umlaut u and euro in different ways.\n");
  sDPrint ("print umlaut a, umlaut o, umlaut u and euro in different ways.\n");

  // if this fails, printf does not produce unicode
  sPrintF("from %%c with hex literals    <%c%c%c%c>\n",0x00e4,0x00f6,0x00fc,0x20ac);
  sDPrintF("from %%c with hex literals    <%c%c%c%c>\n",0x00e4,0x00f6,0x00fc,0x20ac);

  // if the euro fails, then the source is in ansi
  // if all fails, we have a signed / unsigned problem
  sPrintF("from %%c with char literals   <%c%c%c%c>\n",'ä','ö','ü','€');
  sDPrintF("from %%c with char literals   <%c%c%c%c>\n",'ä','ö','ü','€');

  // if this fails, then the compiler is not set to utf8
  sPrint ("from proper escape codes:    <\u00e4\u00f6\u00fc\u20ac>\n");
  sDPrint ("from proper escape codes:    <\u00e4\u00f6\u00fc\u20ac>\n");

  // if this fails, then the source is not in utf8
//  sPrint ("from string literal:         <äöü€>\n");
//  sDPrint ("from string literal:         <äöü€>\n");
}

/****************************************************************************/
