/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#define sPEDANTIC_WARN 1

#include "miniftp.hpp"
#include "base/system.hpp"
#include "base/serialize.hpp"

/****************************************************************************/

static const sInt FileIOBufferSize = 16384;
//static const sInt MetaIOBufferSize = 4096;
static const sInt ServerBufferSize = 65536;

static sBool DummyProgress(const sChar *file,sSize current,sSize total,void *user)
{
  return sTRUE;
}

/****************************************************************************/

sMiniFTPClient::sMiniFTPClient()
{
  State = CS_NOTARGET;
  Error = sMFE_OK;
  RetryCount = 5;
  InitialTimeout = 250;
  ReconnectTimeout = 0;
  ContinueUploads = sTRUE;

  RetryLastTime = 0;
  Progress = DummyProgress;
  ProgressUser = 0;
}

sMiniFTPClient::sMiniFTPClient(const sChar *host,sIPPort port)
{
  Connect(host,port);
}

sMiniFTPClient::~sMiniFTPClient()
{
}

sBool sMiniFTPClient::Connect(const sChar *host,sIPPort port)
{
  Disconnect();

  sCopyString(TargetHost,host);
  TargetPort = port;
  if(!sResolveNameAndPort(host,TargetAddress,TargetPort))
  {
    sLogF(L"MiniFTP",L"Could not resolve host '%s'.\n",host);
    return sFALSE;
  }

  State = CS_NOTCONNECTED;
  return TryReconnect(1);
}

void sMiniFTPClient::Disconnect()
{
  Socket.Disconnect();
  State = CS_NOTARGET;
}

void sMiniFTPClient::SetRetryPolicy(sInt count,sInt initialDelay,sInt reconnectDelay,sBool continueUploads)
{
  RetryCount = count;
  InitialTimeout = initialDelay;
  ReconnectTimeout = reconnectDelay;
  ContinueUploads = continueUploads;
}

void sMiniFTPClient::SetProgressCallback(ProgressCallback progress,void *user)
{
  if(!progress)
    progress = DummyProgress;
  Progress = progress;
  ProgressUser = user;
}

sBool sMiniFTPClient::FileExists(const sChar *filename)
{
  return SendCommand(sMFC_EXISTS,filename,0)
    && Error == sMFE_OK;
}

sBool sMiniFTPClient::GetFile(const sChar *filename,sFile *writeTo)
{
  sSize currentPos = 0;
  if(State == CS_NOTARGET)
    return sFALSE;

  for(;;)
  {
    if(State != CS_CONNECTED && !TryReconnect(RetryCount))
      break;

    sU8 sizeBuf[8];
    if(SendCommand(sMFC_GET,filename,currentPos)
      && ReadAll(sizeBuf,8))
    {
      sSize totalSize;
      sUnalignedLittleEndianLoad64(sizeBuf,(sU64&) totalSize);

      if(!Progress(filename,currentPos,totalSize,ProgressUser))
        return MaybeNextTime();

      sFixedArray<sU8> ioBuf(FileIOBufferSize);
      while(currentPos < totalSize)
      {
        sDInt size = (sDInt) sMin<sS64>(totalSize-currentPos,ioBuf.GetSize());
        sDInt read;

        if(!Socket.Read(&ioBuf[0],size,read) || !read)
        {
          MaybeNextTime();
          break;
        }

        if(!writeTo->Write(&ioBuf[0],read))
          return sFALSE;

        currentPos += read;
        if(!Progress(filename,currentPos,totalSize,ProgressUser))
          return MaybeNextTime();
      }

      if(currentPos == totalSize)
        return sTRUE;
    }
    else if(Error == sMFE_OK)
      MaybeNextTime();
    else
      break;
  }

  return sFALSE;
}

sBool sMiniFTPClient::PutFile(const sChar *filename,sFile *readFrom)
{
  sSize currentPos = 0;
  sSize totalSize = readFrom->GetSize();
  if(State == CS_NOTARGET)
    return sFALSE;

  for(;;)
  {
    if(State != CS_CONNECTED && !TryReconnect(RetryCount))
      break;

    if(!ContinueUploads)
      currentPos = 0;

    if(SendCommand(sMFC_PUT,filename,currentPos))
    {
      sU8 sizeBuf[8];
      sUnalignedLittleEndianStore64(sizeBuf,totalSize);
      if(!WriteAll(sizeBuf,8))
        continue;

      if(!Progress(filename,currentPos,totalSize,ProgressUser))
        return MaybeNextTime();

      sFixedArray<sU8> ioBuf(FileIOBufferSize);

      while(currentPos < totalSize)
      {
        sInt size = (sInt) sMin<sS64>(totalSize-currentPos,ioBuf.GetSize());
        if(!readFrom->Read(&ioBuf[0],size))
          return MaybeNextTime();

        if(!WriteAll(&ioBuf[0],size))
          break;

        currentPos += size;
        if(!Progress(filename,currentPos,totalSize,ProgressUser))
          return sFALSE;
      }

      if(currentPos == totalSize)
        return sTRUE;
    }
    else if(Error == sMFE_OK)
      MaybeNextTime();
    else
      break;
  }

  return sFALSE;
}

sBool sMiniFTPClient::ListFiles(const sChar *basepath,sArray<sChar> &listing)
{
  listing.Clear();
  if(State == CS_NOTARGET)
    return sFALSE;

  for(;;)
  {
    if(State != CS_CONNECTED && !TryReconnect(RetryCount))
      break;
  
    sU8 sizeBuf[8];
    if(SendCommand(sMFC_LIST,basepath,0) && ReadAll(sizeBuf,8))
    {
      sSize totalSize;
      sUnalignedLittleEndianLoad64(sizeBuf,(sU64&) totalSize);
      if((totalSize & 1)              // we expect 16bit chars
        || totalSize > 16*1024*1024)  // 16MB limit for directory listings (for now)
        return sFALSE;

      if(!Progress(basepath,0,totalSize,ProgressUser))
        return MaybeNextTime();

      sFixedArray<sU8> ioBuf((sInt) totalSize);
      sSize currentPos = 0;
      while(currentPos < totalSize)
      {
        sDInt size = (sDInt) (totalSize-currentPos);
        sDInt read;

        if(!Socket.Read(&ioBuf[(sInt) currentPos],size,read) || !read)
        {
          MaybeNextTime();
          break;
        }

        currentPos += read;
        if(!Progress(basepath,currentPos,totalSize,ProgressUser))
          return MaybeNextTime();
      }

      if(currentPos == totalSize)
      {
        listing.HintSize(sU32(totalSize/2));
        listing.AddMany(sU32(totalSize/2));

        for(sInt i=0;i<totalSize/2;i++)
        {
          sU16 v;
          sUnalignedLittleEndianLoad16(&ioBuf[i*2],v);
          listing[i] = v;
        }

        return sTRUE;
      }
    }
    else if(Error == sMFE_OK)
      MaybeNextTime();
    else
      break;
  }

  return sFALSE;
}

sBool sMiniFTPClient::DeleteFile(const sChar *filename)
{
  return SendCommand(sMFC_DELETE,filename,0)
    && Error == sMFE_OK;
}

sMiniFTPClient::ConnStatus sMiniFTPClient::GetCurrentState() const
{
  return State;
}

sMiniFTPError sMiniFTPClient::GetLastError() const
{
  return Error;
}

const sChar *sMiniFTPClient::GetLastErrorMessage() const
{
  const sChar *message = L"Unknown error";

  switch(Error)
  {
  case sMFE_OK:         message = L"No error (this is strange)."; break;
  case sMFE_NOSUCHFILE: message = L"File not found, or file couldn't be created."; break;
  case sMFE_NOCONNECT:  message = L"Connection lost"; break;
  }

  return message;
}

sBool sMiniFTPClient::SendCommand(sInt command,const sChar *filename,sSize extra)
{
  static const sInt maxFilenameLen = 1024;
  sVERIFY(filename != 0);

  Error = sMFE_OK;

  sInt filenameLen = sGetStringLen(filename);
  sVERIFY(filenameLen < maxFilenameLen);
  sVERIFYSTATIC(sizeof(sChar) == 2);

  sU8 header[16];
  sUnalignedLittleEndianStore32(header+ 0,command);
  sUnalignedLittleEndianStore32(header+ 4,filenameLen);
  sUnalignedLittleEndianStore64(header+ 8,extra);
  if(!WriteAll(header,16))
    return sFALSE;

  sU8 *stringBuf = (sU8*) sALLOCSTACK(sChar,filenameLen*2);
  for(sInt i=0;i<filenameLen;i++)
    sUnalignedLittleEndianStore16(stringBuf + i*2,filename[i]);

  if(!WriteAll(stringBuf,filenameLen*2))
    return sFALSE;

  // read answer code
  sU8 answer;
  if(!ReadAll(&answer,1))
    return sFALSE;

  if(answer == sMFE_OK) // no error
    return sTRUE;
  else
  {
    Error = (sMiniFTPError) answer;
    return sFALSE;
  }
}

sBool sMiniFTPClient::MaybeNextTime()
{
  Socket.Disconnect();
  State = CS_NOTCONNECTED;
  return sFALSE;
}

sBool sMiniFTPClient::TryReconnect(sInt attempts,sInt startDelay)
{
  if(State == CS_NOTARGET)
    return sFALSE;

  if(RetryLastTime && (sGetTime() - RetryLastTime) < ReconnectTimeout)
    return sFALSE;

  sInt delay = (startDelay != -1) ? startDelay : InitialTimeout;
  for(sInt i=0;i<attempts;i++)
  {
    Socket.Connect(TargetAddress,TargetPort);
    if(Socket.IsConnected())
      break;

    // attempt failed, wait a bit and try again. wait duration doubles
    // every time.
    if(i != attempts-1)
    {
      sSleep(delay);
      delay *= 2;
    }
  }

  if(Socket.IsConnected())
  {
    State = CS_CONNECTED;
    Error = sMFE_OK;
    RetryLastTime = 0;
    return sTRUE;
  }

  if(attempts > 1)
    sLogF(L"MiniFTP",L"Connection to %s:%d failed after %d attempts.\n",TargetHost,TargetPort,attempts);
  else
    sLogF(L"MiniFTP",L"Connection to %s:%d failed.\n",TargetHost,TargetPort);

  RetryLastTime = ReconnectTimeout ? sGetTime() : 0;
  Error = sMFE_NOCONNECT;
  return sFALSE;
}

sBool sMiniFTPClient::ReadAll(sU8 *buffer,sInt size)
{
  return Socket.ReadAll(buffer,size) ? sTRUE : MaybeNextTime();
}

sBool sMiniFTPClient::WriteAll(const sU8 *buffer,sInt size)
{
  return Socket.WriteAll(buffer,size) ? sTRUE : MaybeNextTime();
}

/****************************************************************************/

sMiniFTPServer::sMiniFTPServer(sIPPort port)
{
  for(sInt i=0;i<MAXCONN;i++)
    FreeList.AddTail(&ConnStore[i]);
  
  HostSocket.Listen(port);
  Handler = 0;
  OnlyFullFiles = sFALSE;
}

sMiniFTPServer::~sMiniFTPServer()
{
  while(!ConnList.IsEmpty())
  {
    Connection *c = ConnList.RemHead();
    c->Socket->Disconnect();
    sDelete(c->Thread);
    HostSocket.CloseConnection(c->Socket);
  }
  HostSocket.Disconnect();
}

void sMiniFTPServer::SetRequestHandler(RequestHandler handler)
{
  Handler = handler;
}

void sMiniFTPServer::SetOnlyFullFiles(sBool enable)
{
  OnlyFullFiles = enable;
}

sBool sMiniFTPServer::Run(sThread *t)
{
  RequestHandler handler = Handler;
  if(!HostSocket.IsConnected() || !handler)
    return sFALSE;

  while(HostSocket.IsConnected() && (!t || t->CheckTerminate()))
  {
    // wait for something to happen
    sTCPSocket *newconn;
    if(HostSocket.WaitForNewConnection(newconn,100) && newconn)
    {
      // "garbage collect" dropped connections
      Connection *c, *next;
      for(c=ConnList.GetHead(); !ConnList.IsEnd(c); c=next)
      {
        next = ConnList.GetNext(c);
        if(!c->Socket->IsConnected())
        {
          sDelete(c->Thread);
          HostSocket.CloseConnection(c->Socket);
          ConnList.Rem(c);
          FreeList.AddTail(c);
        }
      }

      // add the new connection
      if(!FreeList.IsEmpty())
      {
        c = FreeList.RemHead();
        sSetMem(c,0,sizeof(Connection));
        c->Socket = newconn;
        c->Handler = handler;
        c->OnlyFullFiles = OnlyFullFiles;
        ConnList.AddTail(c);
        c->Thread = new sThread(ClientThreadFunc,0,32768,c);
      }
      else
      {
        sLogF(L"MiniFTP",L"Out of connections!\n");
        HostSocket.CloseConnection(newconn);
      }
    }
  }

  return sTRUE;
}

void sMiniFTPServer::ClientThreadFunc(sThread *t,void *user)
{
  Connection *c = (Connection *) user;
  sTCPSocket *socket = c->Socket;
  RequestInfo info;

  sVERIFYSTATIC(ServerBufferSize >= 16 && ServerBufferSize >= MAXREQUEST*2);
  sFixedArray<sU8> ioBuffer(ServerBufferSize);

  while(socket->IsConnected() && t->CheckTerminate())
  {
    // read request
    if(!socket->ReadAll(&ioBuffer[0],16))
      break;

    // "parse" it
    sU32 command;
    sInt filenameLen;
    sU64 extra;
    sUnalignedLittleEndianLoad32(&ioBuffer[0],command);
    sUnalignedLittleEndianLoad32(&ioBuffer[4],(sU32&) filenameLen);
    sUnalignedLittleEndianLoad64(&ioBuffer[8],extra);

    if(command > sMFC_LAST || filenameLen >= MAXREQUEST)
      break;

    // read the filename
    sString<MAXREQUEST> filename;
    if(!socket->ReadAll(&ioBuffer[0],filenameLen*2))
      break;

    // "parse" it
    for(sInt i=0;i<filenameLen;i++)
    {
      sU16 x;
      sUnalignedLittleEndianLoad16(&ioBuffer[i*2],x);
      filename[i] = x;
    }

    filename[filenameLen] = 0;

    // command-specific handling
    info.Command = (sMiniFTPCommand) command;
    info.Filename = filename;
    info.File = 0;
    info.DirListing.Clear();

    ioBuffer[0] = sU8(c->Handler(info) ? sMFE_OK : sMFE_NOSUCHFILE);
    sBool ok = socket->WriteAll(&ioBuffer[0],1);

    if(ok && ioBuffer[0] == sMFE_OK) // output processing (depends on command)
    {
      ok = sFALSE;

      switch(command)
      {
      case sMFC_EXISTS:
      case sMFC_DELETE:
        ok = sTRUE;
        break;

      case sMFC_GET:
        {
          sSize size = info.File->GetSize();

          sUnalignedLittleEndianStore64(&ioBuffer[0],size);
          if(socket->WriteAll(&ioBuffer[0],8) && info.File->SetOffset(extra))
          {
            sSize pos = extra;
            while(pos<size)
            {
              sInt bytes = (sInt) sMin<sS64>(size-pos,ioBuffer.GetSize());
              if(!info.File->Read(&ioBuffer[0],bytes)
                || !socket->WriteAll(&ioBuffer[0],bytes))
                break;

              pos += bytes;
            }

            ok = (pos == size);
          }
        }
        break;

      case sMFC_PUT:
        {
          sSize size;
          if(c->OnlyFullFiles && extra != 0)
            break;

          if(!socket->ReadAll(&ioBuffer[0],8))
            break;

          sUnalignedLittleEndianLoad64(&ioBuffer[0],(sU64&) size);
          if(info.File->SetOffset(extra))
          {
            sSize pos = extra;
            while(pos<size)
            {
              sInt bytes = (sInt) sMin<sS64>(size-pos,ioBuffer.GetSize());
              if(!socket->ReadAll(&ioBuffer[0],bytes)
                || !info.File->Write(&ioBuffer[0],bytes))
                break;

              pos += bytes;
            }

            ok = (pos == size);
            if(c->OnlyFullFiles && !ok)
            {
              sDelete(info.File);
              sLogF(L"miniftp",L"transfer of file <%s> incomplete, deleting it again.\n",info.Filename);

              // delete the file again
              info.Command = sMFC_DELETE;
              c->Handler(info);
            }
          }
        }
        break;

      case sMFC_LIST:
        {
          sInt count = info.DirListing.GetCount();
          sUnalignedLittleEndianStore64(&ioBuffer[0],count*2);
          if(socket->WriteAll(&ioBuffer[0],8))
          {
            sFixedArray<sU8> buffer(count*2);
            for(sInt i=0;i<count;i++)
              sUnalignedLittleEndianStore16(&buffer[i*2],info.DirListing[i]);

            ok = socket->WriteAll(&buffer[0],count*2);
          }
        }
        break;
      }
    }

    sDelete(info.File);
    if(!ok)
      break;
  }

  socket->Disconnect();
}

/****************************************************************************/

