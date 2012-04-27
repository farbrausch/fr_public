/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "server.hpp"
#include "base/system.hpp"
#include "network/miniftp.hpp"

/****************************************************************************/

sBool RequestHandler(sMiniFTPServer::RequestInfo &info)
{
  // CAUTION: You should *never ever* pass filenames from network
  // directly to sCreateFile. You at least want to clean the filename
  // beforehand to make sure it doesn't reference any directory above
  // the server root! (sIsBelowCurrentDir is your friend)
  const sChar *filename = info.Filename;
  sBool fnOk = sCmpString(filename,L"test1.txt") == 0 || sCmpString(filename,L"test2.txt") == 0;
  static const sChar listing[] = L"test1.txt\0test2.txt\0";
  sInt nListing = sCOUNTOF(listing);
  
  switch(info.Command)
  {
  case sMFC_EXISTS: return fnOk && sCheckFile(filename);
  case sMFC_GET:    info.File = fnOk ? sCreateFile(filename,sFA_READ) : 0;      return info.File != 0;
  case sMFC_PUT:    info.File = fnOk ? sCreateFile(filename,sFA_READWRITE) : 0; return info.File != 0;
  case sMFC_LIST:   sCopyMem(info.DirListing.AddMany(nListing),listing,nListing*2); return sTRUE;
  }
  
  return sFALSE;
}

void ServerThread(sThread *t,void *user)
{
  sMiniFTPServer server(12345);

  sPrintF(L"Starting server, Ctrl+C to exit.\n");
  server.SetRequestHandler(RequestHandler);
  server.Run(t);
}

void sMain()
{
  sGetMemHandler(sAMF_HEAP)->MakeThreadSafe(); // WTF?!

  {
    sThread serve(ServerThread);
    
    sCatchCtrlC();
    while(!sGotCtrlC())
      sSleep(100);
  }

  sPrintF(L"Server quit.\n");
}

/****************************************************************************/

