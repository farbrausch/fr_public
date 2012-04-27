/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "base/system.hpp"
#include "network/miniftp.hpp"

/****************************************************************************/

struct UpdateInfo
{
  sInt startTime;
  sInt lastTime;
};

sFile *OpenFileWrite(const sChar *name)
{
  sFile *file = sCreateFile(name,sFA_WRITE);
  if(file)
    return file;

  // if not succesful, it might be because the target directory doesn't exist.
  // try creating the directory, then retry.
  sString<4096> path;
  sExtractPath(name,path);
  sMakeDirAll(path);
  return sCreateFile(name,sFA_WRITE);
}

sBool Progress(const sChar *filename,sSize current,sSize total,void *user)
{
  UpdateInfo *update = (UpdateInfo *) user;
  sInt now = sGetTime();

  if(!update->startTime)
    update->startTime = now;

  if(current==0 || current==total || now-update->lastTime >= 50)
  {
    sInt timeElapsed = sGetTime() - update->startTime;
    sF64 percent = 100.0 * current / total;

    if(timeElapsed < 1000)
      sPrintF(L"\r%s  %4k/%k bytes (%6.2f%%)",filename,current,total,percent);
    else
    {
      sF64 mbPerS = (current * 1000.0) / (1048576.0 * timeElapsed);
      sPrintF(L"\r%s  %4k/%k bytes (%6.2f%%)  %6.2f MB/s",filename,current,total,percent,mbPerS);
    }

    update->lastTime = now;
  }

  return sTRUE;
}

void Syntax()
{
  sPrintF(L"Syntax: fileclient -s <server>[:<port>] <command>\n"
          L"  <command> can be:\n"
          L"  --exists/-e <filename>  Check whether the file exists on the server.\n"
          L"  --get/-g <filename>     Download the given file from the server.\n"
          L"  --put/-p <filename>     Upload the file to the server.\n"
          L"  --list/-l <dirname>     Print a directory listing.\n"
          L"  --delete/-d <filename>  Delete the given file on the server.\n");
}

void sMain()
{
  const sChar *server = sGetShellString(L"s",L"server");
  if(!server)
  {
    Syntax();
    return;
  }

  sMiniFTPClient client;
  client.SetRetryPolicy(5,250,250,sFALSE);

  if(!client.Connect(server,4901))
  {
    sPrintF(L"Error: Couldn't connect to server!\n");
    return;
  }

  UpdateInfo update;
  update.startTime = update.lastTime = 0;
  client.SetProgressCallback(Progress,&update);

  if(const sChar *existName = sGetShellString(L"e",L"exists"))
  {
    if(client.FileExists(existName))
      sPrintF(L"File \"%s\" exists.\n",existName);
    else if(client.GetLastError() == sMFE_NOSUCHFILE)
      sPrintF(L"File \"%s\" doesn't exist.\n",existName);
    else
      sPrintF(L"Error querying file \"%s\": %s\n",existName,client.GetLastErrorMessage());
  }
  else if(const sChar *getName = sGetShellString(L"g",L"get"))
  {
    sFile *target = OpenFileWrite(getName);
    if(!target)
    {
      sPrintF(L"Error opening file \"%s\" for writing.\n",getName);
      return;
    }

    if(client.GetFile(getName,target))
      sPrintF(L"\nDone.\n");
    else
      sPrintF(L"\nError getting file \"%s\": %s\n",getName,client.GetLastErrorMessage());
  }
  else if(const sChar *putName = sGetShellString(L"p",L"put"))
  {
    sFile *source = sCreateFile(putName,sFA_READ);
    if(!source)
    {
      sPrintF(L"Error opening file \"%s\" for reading.\n",putName);
      return;
    }

    if(client.PutFile(putName,source))
      sPrintF(L"\nDone.\n");
    else
      sPrintF(L"\nError putting file \"%s\": %s\n",putName,client.GetLastErrorMessage());
  }
  else if(const sChar *dirName = sGetShellString(L"l",L"list"))
  {
    sArray<sChar> listing;
    if(client.ListFiles(dirName,listing))
    {
      sPrintF(L"\nList:\n");
      sInt i=1,pos=0;
      while(listing[pos])
      {
        sPrintF(L"%4d. %s\n",i,&listing[pos]);
        i++;
        pos += sGetStringLen(&listing[pos])+1;
      }
    }
    else
      sPrintF(L"\nError listing directory \"%s\": %s\n",dirName,client.GetLastErrorMessage());
  }
  else if(const sChar *delName = sGetShellString(L"d",L"delete"))
  {
    if(client.DeleteFile(delName))
      sPrintF(L"File \"%s\" successfuly deleted.\n",delName);
    else if(client.GetLastError() == sMFE_NOSUCHFILE)
      sPrintF(L"File \"%s\" doesn't exist.\n",delName);
    else
      sPrintF(L"Error deleting file \"%s\": %s\n",delName,client.GetLastErrorMessage());
  }
  else
    Syntax();
}

/****************************************************************************/

