/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_NETWORK_MINIFTP_HPP
#define FILE_NETWORK_MINIFTP_HPP

#include "base/types.hpp"
#include "base/types2.hpp"
#include "network/sockets.hpp"

class sFile;
class sThread;

/****************************************************************************/

// This is *a* file transfer protocol, not *the* file transfer protocol (FTP).
// Just a quick and simple protocol for bulk data transfers.
// Oh, and unlike the real FTP, it supports resume on upload.

enum sMiniFTPCommand
{
  sMFC_EXISTS = 0,    // does a certain file exist?
  sMFC_GET = 1,       // receive a file from the server
  sMFC_PUT = 2,       // write a file to the server
  sMFC_LIST = 3,      // return a directory listing
  sMFC_DELETE = 4,    // delete a file on the server
  
  sMFC_LAST = sMFC_DELETE, // not a command!
};

enum sMiniFTPError
{
  sMFE_OK = 0,            // everything just fine!
  sMFE_NOSUCHFILE = 1,    // file doesn't exist or couldn't be created
  sMFE_NOCONNECT = 256,   // couldn't (re)connect
};

/****************************************************************************/
/***                                                                      ***/
/*** MiniFTP Client                                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

class sMiniFTPClient
{
public:
  enum ConnStatus
  {
    CS_NOTARGET = 0,
    CS_NOTCONNECTED,
    CS_CONNECTED,
  };

  sMiniFTPClient();
  sMiniFTPClient(const sChar *host,sIPPort port);
  ~sMiniFTPClient();

  typedef sBool (*ProgressCallback)(const sChar *file,sSize current,sSize total,void *user);

  sBool Connect(const sChar *host,sIPPort port); // URL support? maybe later.
  void Disconnect();

  void SetRetryPolicy(sInt count,sInt initialDelay,sInt reconnectDelay,sBool continueUploads=sTRUE);

  void SetProgressCallback(ProgressCallback progress,void *user=0);
  
  sBool FileExists(const sChar *filename);
  // also returns sFALSE on error. GetLastError() if you want to be sure.

  sBool GetFile(const sChar *filename,sFile *writeTo);
  // may have written part of the file even if sFALSE is returned.

  sBool PutFile(const sChar *filename,sFile *readFrom);
  // may have read part of the file even if sFALSE is returned.

  sBool ListFiles(const sChar *basepath,sArray<sChar> &listing);
  // Returns "name1\0name2\0[..]nameN\0\0"

  sBool DeleteFile(const sChar *filename);

  ConnStatus GetCurrentState() const;
  sMiniFTPError GetLastError() const;
  const sChar *GetLastErrorMessage() const;

protected:
  sBool SendCommand(sInt command,const sChar *filename,sSize extra);
  sBool TryReconnect(sInt attempts,sInt startDelay=-1);
  sBool MaybeNextTime();

  sBool ReadAll(sU8 *buffer,sInt size);
  sBool WriteAll(const sU8 *buffer,sInt size);

  sTCPClientSocket Socket;
  sInt RetryCount;
  sInt InitialTimeout;
  sInt ReconnectTimeout;
  sBool ContinueUploads;

  ConnStatus State;
  sMiniFTPError Error;
  sInt RetryLastTime;
  sString<256> TargetHost; // only for display
  sIPAddress TargetAddress;
  sIPPort TargetPort;
  ProgressCallback Progress;
  void *ProgressUser;
};

/****************************************************************************/
/***                                                                      ***/
/*** MiniFTP Server                                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

class sMiniFTPServer
{
public:
  sMiniFTPServer(sIPPort port);
  ~sMiniFTPServer();

  struct RequestInfo
  {
    // input
    sMiniFTPCommand Command;
    const sChar *Filename;

    // output
    sFile *File;
    sArray<sChar> DirListing;
  };
  
  typedef sBool (*RequestHandler)(RequestInfo &info);

  void SetRequestHandler(RequestHandler handler);
  void SetOnlyFullFiles(sBool enable);
  sBool Run(sThread *t);

private:
  static const sInt MAXCONN = 128;
  static const sInt MAXREQUEST = 1024;

  struct Connection
  {
    sTCPSocket *Socket;
    sThread *Thread;
    RequestHandler Handler;
    sDNode Link;
    sBool OnlyFullFiles;
  };

  static void ClientThreadFunc(sThread *t,void *user);

  sTCPHostSocket HostSocket;
  RequestHandler Handler;
  sBool OnlyFullFiles;
  Connection ConnStore[MAXCONN];
  sDList<Connection,&Connection::Link> ConnList;
  sDList<Connection,&Connection::Link> FreeList;
};

/****************************************************************************/

#endif // FILE_NETWORK_MINIFTP_HPP

