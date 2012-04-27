/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "base/system.hpp"
#include "network/miniftp.hpp"

/****************************************************************************/

sFile *OpenFileWrite(const sChar *name)
{
  sFile *file = sCreateFile(name,sFA_READWRITE);
  if(file)
    return file;

  // if not succesful, it might be because the target directory doesn't exist.
  // try creating the directory, then retry.
  sString<4096> path;
  sExtractPath(name,path);
  sMakeDirAll(path);
  return sCreateFile(name,sFA_READWRITE);
}

sBool MakeDirListing(sArray<sChar> &listing,const sChar *dirName)
{
  sArray<sDirEntry> entries;
  if(sLoadDir(entries,dirName))
  {
    for(sInt i=0;i<entries.GetCount();i++)
    {
      sInt len = sGetStringLen(entries[i].Name);
      sCopyString(listing.AddMany(len+1),entries[i].Name,len+1);
    }

    listing.AddTail(0);
    return sTRUE;
  }
  else
    return sFALSE;
}

sBool RequestHandler(sMiniFTPServer::RequestInfo &info)
{
  const sChar *filename = info.Filename;
  if(!sIsBelowCurrentDir(filename))
    return sFALSE;

  switch(info.Command)
  {
  case sMFC_EXISTS: return sCheckFile(filename);
  case sMFC_GET:    info.File = sCreateFile(filename,sFA_READ); return info.File != 0;
  case sMFC_PUT:    info.File = OpenFileWrite(filename); return info.File != 0;
  case sMFC_LIST:   return MakeDirListing(info.DirListing,filename);
  case sMFC_DELETE: return sDeleteFile(filename);
  }

  return sFALSE;
}

void sMain()
{
  sGetMemHandler(sAMF_HEAP)->MakeThreadSafe();

  const sChar *rootDir = sGetShellString(L"r",L"root",L"c:/nxn/temp/fileserver");
  sInt port = sGetShellInt(L"p",L"port",4901);

  sPrintF(L"Server root directory: \"%s\".\n",rootDir);
  if(sChangeDir(rootDir))
  {
    sMiniFTPServer server(port); // port 4901 is "unassigned" in IANA list
    server.SetRequestHandler(RequestHandler);
    server.SetOnlyFullFiles(sTRUE);
    sPrintF(L"Starting fileserver...\n");
    server.Run(0);
  }
  else
    sPrintF(L"Server root not found!\n");

  // yep, that's it already.
}

/****************************************************************************/

