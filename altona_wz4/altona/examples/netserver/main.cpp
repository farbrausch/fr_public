/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "main.hpp"
#include "network/http.hpp"

const sChar8 Stylesheet[] = 
  "body {"
    "margin-left:20px;"
    "margin-right:10px;"
    "margin-top:10px;"
    "margin-bottom:10px;"
    "font:normal 11pt Century Gothic, Arial, Helvetica;"
    "vertical-align:middle;"
    "color:#FFCC99;"
    "background-position:middle;"
    "border-width:0;"
    "background-color:#000000;"
  "}";


const sChar8 MainPage[] = 
  "<html>"
    "<head><title>jaja</title></head>"
    "<body><a href=\"lol/test.html\">Test!</a></body>"
  "</html>";


class TestHandler : public sHTTPServer::SimpleHandler
{
public:

  sHTTPServer::HandlerResult WriteDocument(const sChar *URL)
  {
    static int calls=0;
    WriteHTMLHeader(L"Testhandler",L"/test.css");

    PrintF(L"das hier ist ein Test von %s! (%d)",URL,calls++);

    WriteHTMLFooter();
    return sHTTPServer::HR_OK;
  }

  static Handler *Factory() { return new TestHandler; }
};



void sMain()
{
  sDPrintF(L"\n\n");

  sString<512> localhost;
  sGetLocalHostName(localhost);
  sDPrintF(L"hostname: %s\n",localhost);

  sHTTPServer server(8080,L"c:\\pouet");

  server.AddStaticPage(L"/test.css",Stylesheet,sizeof(Stylesheet));
  server.AddStaticPage(L"/",MainPage,sizeof(MainPage));

  server.AddHandler(L"*/test.html",TestHandler::Factory);

  server.Run(0);
};


/****************************************************************************/

