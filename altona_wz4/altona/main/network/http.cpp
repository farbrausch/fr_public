/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "http.hpp"
#include "base/system.hpp"

/****************************************************************************/
/****************************************************************************/

// base64 encoding tables
static const sChar *Base64Enc[2]=
{
  L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/",
  L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_",
};

void sEncodeBase64(const void *buffer, sInt size, const sStringDesc &out, sBool urlmode, sInt linelen)
{
  static const sChar *table=Base64Enc[urlmode?1:0];
  const sU8 *bp=(const sU8*)buffer;
  const sU8 *bend=bp+size;
  sChar *outp=out.Buffer;
  sChar *outend=out.Buffer+out.Size-1;

  while (bp<bend && outp<outend)
  {
    sU8 in[3];
    for (sInt i=0; i<3; i++)
      in[i]=(bp<bend)?*bp++:0;
    *outp++ = table[ in[0] >> 2 ];
    *outp++ = table[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
    *outp++ = (size > 1 ? table[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
    *outp++ = (size > 2 ? table[ in[2] & 0x3f ] : '=');
    size-=3;
    if (linelen && !((outp-out.Buffer)%(linelen-2)))
    {
      *outp++='\r';
      *outp++='\n';
    }
  }
  *outp++=0;
  
}

// decoding tables
static sU8 Base64Dec[2][128];
static sBool Base64DecInited=sFALSE;

sInt sDecodeBase64(const sChar *str, void *buffer, sInt maxlen, sBool urlmode)
{
  // generate tables on first call (inverse function of encoding tables)
  if (!Base64DecInited)
  {
    sSetMem(Base64Dec,0x80,sizeof(Base64Dec));
    for (sInt i=0; i<2; i++)
    {
      for (sInt j=0; j<64; j++)
        Base64Dec[i][Base64Enc[i][j]]=j;
      Base64Dec[i][(sInt)'=']=0xff;
    }
    Base64DecInited=sTRUE;
  }

  const sU8 *table=Base64Dec[urlmode?1:0];

  sU8 *b8=(sU8*)buffer;
  sU8 *bp=b8;
  sU8 *bend=bp+maxlen;
  
  sBool end=sFALSE;
  while (*str && bp<bend && !end)
  {
    // fetch
    sInt len=0;
    sU8  in[4]={0,0,0,0};
    sInt i=0;
    while (i<4 && *str)
    {
      sInt c=*str++;
      c=(c>0 && c<128)?table[c]:0x80;
      if (c==0xff) 
      {
        end=sTRUE;
        break;
      }
      else if (c!=0x80)
      {
        in[i++]=c;
        len=i-1;
      }
    }
    if (!*str && i<4 && len==3) return 0; // error

    // decode
    if (len>0 && bp<bend) *bp++=(in[0] << 2 | in[1] >> 4);
    if (len>1 && bp<bend) *bp++=(in[1] << 4 | in[2] >> 2);
    if (len>2 && bp<bend) *bp++=(((in[2] << 6) & 0xc0) | in[3]);
  }

  return bp-b8;
}


void sDecodeURL(sChar *url) // in-place, result is never longer
{
  const sChar *uin=url;
  while (*uin)
  {
    if (*uin=='%')
    {
      uin++;
      sInt h=0;
      for (sInt i=0; i<2; i++)
      {
        h<<=4;
        sInt ch=*uin++;
        if (ch>='0' && ch<='9') h|=(ch-'0');
        else if (ch>='a' && ch<='f') h|=(ch-'a')+10;
        else if (ch>='A' && ch<='F') h|=(ch-'A')+10;
      }
      *url++=h;
    }
    else
      *url++=*uin++;
  }
  *url=0;
}

/****************************************************************************/
/****************************************************************************/

sHTTPClient::sHTTPClient()
{
  Method=CM_GET;
  Fill=0;
  Size=-1;
  RetCode=CS_NOTCONNECTED;
}

sHTTPClient::sHTTPClient( const sChar *url, ConnMethod method/*=CM_GET*/ )
{
  Connect(url,method);
}

sBool sHTTPClient::Connect(const sChar *url, ConnMethod method) 
{
  Disconnect();
  Method=method;
  Fill=0;
  Size=-1;
  RetCode=CS_NOTCONNECTED;

  sString<512> host;
  sString<1024> temp;
  sIPPort port=80;

  // parse URL
  if (!sCmpMem(url,L"http://",7)) url+=7;

  // TODO: user/pwd (find @ sign before '/')

  const sChar *p=url;
  while ((*p>='0' && *p<='9') || (*p>='a' && *p<='z') || *p=='-' || *p=='.') p++;
  sCopyString(host,url,p-url+1);
  url=p;

  sIPAddress address;
  if (!sResolveName(host,address))
  {
    sLogF(L"http",L"could not resolve host '%s'\n",host);
    return sFALSE;
  }

  if (*url==':') // port number?
  {
    url++;
    sInt pt;
    if (sScanInt(url,pt)) port=pt;
  }

  sTCPClientSocket::Connect(address,port);

  if (!IsConnected())
  {
    sLogF(L"http",L"connection to %s:%d failed\n",host,port);
    return sFALSE;
  }

  // send request
  sVERIFY(Method==CM_GET);
  static const sChar *methods[]={L"GET ",L"PUT ",L"POST "};
  Send8(methods[Method]);
  Send8(url);
  Send8(L" HTTP/1.1\r\n");

  // send header lines
  sSPrintF(temp,L"Host: %s\r\n",host);
  sCopyString(sGetAppendDesc(temp),L"User-Agent: AltonaHTTPClient\r\n");

  // ... and finalize
  sCopyString(sGetAppendDesc(temp),L"\r\n");
  Send8(temp);

  // check connection status again...
  if (!IsConnected())
  {
    sLogF(L"http",L"sending request to %s:%d failed\n",host,port);
    return sFALSE;
  }

  // now wait for the reply and parse
  sDInt  max, read;
  for (;;)
  {
    // get chunk of data from server
    max=1023-Fill;
    if (!sTCPClientSocket::Read(Buffer+Fill,max,read))
    {
      sLogF(L"http",L"getting reply from %s:%d failed\n",host,port);
      return sFALSE;
    }
    if (!read)
    {
      goto headererr;
    }    
    Fill+=read;
    Buffer[Fill]=0;

    sInt llen;
    for (;;)
    {
      // parse header lines
      llen=-1;
      for (sInt i=0; Buffer[i]; i++)
        if (Buffer[i]=='\r' && Buffer[i+1]=='\n')
        {
          llen=i;
          break;
        }

      if (llen<1) break; // no line found (more data needed)

      // copy line for inspection
      sCopyString(temp,Buffer,llen+1);
      llen+=2;
      sCopyMem(Buffer,Buffer+llen,Fill-llen);
      Fill-=llen;
      const sChar *lp=temp; 

      // parse line
      if (!RetCode) // "HTTP/x.y ###" first!
      {
        if (sCmpMem(lp,L"HTTP/1.",7)) goto headererr;
        lp+=9; // skip "HTTP/1.x "
        if (!sScanInt(lp,RetCode)) goto headererr;
        // not interested in the rest
      }
      else if (sScanMatch(lp,L"Content-Length: "))
      {
        if (!sScanInt(lp,Size)) goto headererr;
      }
      
    }

    if (!llen) 
    {
      // done parsing, kill the CR/LF and go on
      sCopyMem(Buffer,Buffer+2,Fill-2);
      Fill-=2;
      break; 
    }

  }

  return sTRUE;

headererr:
  sLogF(L"http",L"malformed reply from %s:%d\n",host,port);
  return sFALSE;

}



sBool sHTTPClient::Send8(const sChar *str)
{
  sVERIFY(!Fill);
  if (!str) return sTRUE;

  // convert to 8bit naively
  sInt len=0;

  while (*str)
  {
    Buffer[len++]=*str++;
    if (len==1023)
    {
      if (!WriteAll(Buffer,len))
        return sFALSE;
      len=0;
    }
  }
  if (len) return WriteAll(Buffer,len);

  return sTRUE;
}


sBool sHTTPClient::Read(void *buffer, sDInt size, sDInt &read)
{
  read=0;
  sU8 *b8=(sU8*)buffer;

  // still data in our buffer? serve that first!
  if (Fill && size)
  {
    sInt todo=sMin<sInt>(Fill,size);
    
    sCopyMem(b8,Buffer,todo);
    sCopyMem(Buffer,Buffer+todo,Fill-todo);
    Fill-=todo;
    size-=todo;
    if (Size>0) Size-=todo;
    read+=todo;
    b8+=todo;
  }

  if (!Size || !size) return sTRUE;

  sDInt r2=0;
  sBool ret;
  if (Size>0) 
    ret=sTCPClientSocket::Read(b8,sMin(Size,size),r2);
  else
    ret=sTCPClientSocket::Read(b8,size,r2);

  read+=r2;
  return ret;
}


sHTTPClient::ConnStatus sHTTPClient::GetStatus()
{
  return ConnStatus(RetCode/100);
}

sHTTPClient::ConnStatus sHTTPClient::GetStatus(sInt &realcode)
{
  realcode=RetCode;
  return ConnStatus(RetCode/100);
}

sInt sHTTPClient::GetSize() { return IsConnected()?Size:-1; }

/****************************************************************************/
/****************************************************************************/

sHTTPServer::sHTTPServer()
{
  for (sInt i=0; i<MAXCONN; i++) FreeList.AddTail(&ConnStore[i]);
}

sHTTPServer::sHTTPServer(sInt port, const sChar *fileroot)
{
  for (sInt i=0; i<MAXCONN; i++) FreeList.AddTail(&ConnStore[i]);
  Init(port,fileroot);
}

sHTTPServer::~sHTTPServer()
{
  while (!ConnList.IsEmpty())
    HostSocket.CloseConnection(ConnList.RemHead()->Socket);
  HostSocket.Disconnect();

  while (!URLEntries.IsEmpty())
    delete URLEntries.RemHead();
}

sBool sHTTPServer::Init(sInt port, const sChar *fileroot)
{
  if (fileroot)
  {
    FileRoot=fileroot;
    FilesSupported=sTRUE;
  }
  else
    FilesSupported=sFALSE;
  return HostSocket.Listen(port);
}

void sHTTPServer::AddStaticPage(const sChar *wildcard, const void *ptr, sInt length)
{
  Lock.Lock();

  URLEntry *e = new URLEntry;
  sSetMem(e,0,sizeof(URLEntry));
  sCopyString(e->Wildcard,wildcard);

  e->Mem=(const sU8*)ptr;
  e->Size=length;

  URLEntries.AddHead(e);

  Lock.Unlock();
}



void sHTTPServer::AddHandler(const sChar *wildcard, HandlerCreateFunc factory)
{
  Lock.Lock();

  URLEntry *e = new URLEntry;
  sSetMem(e,0,sizeof(URLEntry));
  sCopyString(e->Wildcard,wildcard);

  e->HFactory=factory;

  URLEntries.AddHead(e);
  
  Lock.Unlock();
}


void sHTTPServer::ParseRequestLine(Connection *c)
{
  //sDPrintF(L"<%s>\n",c->RequestLine);
  const sChar *cp=c->RequestLine;

  // no method set? we need "<METHOD> <URL> <HTTPVERSION>"
  if (c->Method == CM_UNKNOWN)
  {
    // parse method
    if (sScanMatch(cp,L"GET ")) c->Method=CM_GET; 
    else if (sScanMatch(cp,L"HEAD ")) c->Method=CM_HEAD;
    else if (sScanMatch(cp,L"POST ")) c->Method=CM_POST;
    else
    {
      c->Error=L"Unknown Method";
      c->RetCode=400;
      return;
    }

    // parse URL
    sChar *url=c->URL;
    while (*cp && *cp>32) *url++=*cp++;
    *url=0;

    if (url==c->URL || (*cp!=32 && !*cp))
    {
      c->Error=L"Malformed URL";
      c->RetCode=400;
      return;
    }

    // make real chars out of % ones
    sDecodeURL(c->URL);
    

    if (!*cp)
    {
      c->Error=L"";
      c->RetCode=1; // probably http 0.9
      return;
    }

    // parse HTTP version (must be 1.0 or upwards anyway, so we don't really bother)
    cp++;
    if (!sScanMatch(cp,L"HTTP/")) c->RetCode=400;
    return;
  }

  // empty line? end of request
  if (!*cp)
  {
    if (c->Method==CM_UNKNOWN)
    {
      c->RetCode=400;
      return;
    }

    c->HeadersDone = sTRUE;

    if (c->Method!=CM_POST)
    {
      // let's go
      c->Error=L"OK";
      c->RetCode=200;
    }
    return;
  }

  // TODO: parse additional header data here
  if (sScanMatch(cp, L"Content-Length: "))
  {
    sScanInt(cp, c->ContentLength);
  }

  if (sScanMatch(cp, L"Content-Type: "))
  {
    sCopyString(c->ContentType, cp, 128);
  }

}

void sHTTPServer::StartServing(Connection *c)
{
  if (c->RetCode<400)
  {
    sString<REQLINE> url2=c->URL;

    if (c->Method==CM_GET || c->Method==CM_HEAD)
    {
      sInt qpos=sFindFirstChar(url2,'?');
      if (qpos>=0) url2[qpos]=0;
    }

    if (c->Method==CM_GET || c->Method==CM_HEAD || c->Method==CM_POST)
    {
      // try to find URL handler
      recheck:
      sBool found=sFALSE;

      URLEntry *e=0;
      sFORALL_LIST(URLEntries,e)
      {
        if (sMatchWildcard(e->Wildcard,url2,sTRUE))
        {
          if (e->HFactory)
          {
            Handler *h = e->HFactory();
            switch (h->Init(c))
            {
            case HR_OK:
              found=sTRUE;
              c->Hndl=h;
              break;
            case HR_IGNORE:
              delete h;
              break;
            case HR_ERROR:
              delete h;
              c->RetCode=500;
              c->Error=L"Handler Error";
              break;           
            case HR_REWRITTEN:
              delete h;
              goto recheck;
            }
          }
          else if (e->Mem)
          {
            found=sTRUE;
            c->Mem=e->Mem;
            c->MemSize=e->Size;
          }
          else
          {
            c->RetCode=500;
            c->Error=L"Invalid URL Entry";
          }
        }
        if (found || c->RetCode>=400) break;
      }

      if (!found && FilesSupported)
      {
        static sString<sMAXPATH> path;
        path=FileRoot;
        sAppendString(path,url2);

        sFile *f=sCreateFile(path);
        if (f)
        {
          c->File=f;
          found=sTRUE;
        }
      }

      if (!found && c->RetCode<400)
      {
        c->RetCode=404;
        c->Error=L"Not Found";
      }
    }
    else
    {
      c->RetCode=400;
      c->Error=L"Unsupported Method";
    }

  }

  sBool doclose=sFALSE;
  if (c->Method==CM_HEAD) doclose=sTRUE;

  if (c->RetCode>=200)
  {
    sString<1024> str;
    sSPrintF(str,L"HTTP/1.0 %d %s\r\n",c->RetCode,c->Error);

    if (c->File)
      sSPrintF(sGetAppendDesc(str),L"Content-Length: %d\r\n",c->File->GetSize());
    else if (c->Mem && c->MemSize)
      sSPrintF(sGetAppendDesc(str),L"Content-Length: %d\r\n",c->MemSize);

    sSPrintF(sGetAppendDesc(str),L"Connection: close\r\n");

    // we encountered an error. too sad.
    if (c->RetCode>=400)
    {
      if (c->Method!=CM_HEAD) sSPrintF(sGetAppendDesc(str),L"\r\n<html><body><h1>%d %s</h1>Sorry.<br><i>Altona Web Server</i></body></html>",c->RetCode,c->Error);
      doclose=sTRUE;
    }
    else
    {
      // add additional header lines here
      if (c->Hndl) c->Hndl->GetAdditionalHeaders(sGetAppendDesc(str));
      sAppendString(sGetAppendDesc(str),L"\r\n");
    }

    // send header
    sChar8 str8[1024];
    sCopyString(str8,str,1024);
    c->Socket->WriteAll(str8,sGetStringLen(str));
  }

  if (doclose)
    c->Socket->Disconnect();
  else
    c->State=CS_SERVE;

}

/****************************************************************************/

sBool sHTTPServer::Run(sThread *t)
{
  sBool aborted = sFALSE;
  if (!HostSocket.IsConnected()) return sFALSE;

  Lock.Lock();

  sTCPSocket **readset=new sTCPSocket*[MAXCONN];
  sTCPSocket **writeset=new sTCPSocket*[MAXCONN];

  while (HostSocket.IsConnected() && (!t || t->CheckTerminate())) 
  {
    //static sInt test=0;
    //sDPrintF(L"tick %d\n",test++);

    // step 1: make list of readable/writable sockets
    sInt nreads=0;
    sInt nwrites=0;
    Connection *c;
    sFORALL_LIST(ConnList,c)
    {
      if (c->State == CS_GETREQUEST) readset[nreads++]=c->Socket;

      if (c->State == CS_SERVE) 
      {
        if (c->File || c->Mem || (c->Hndl && c->Hndl->DataAvailable()))
          writeset[nwrites++]=c->Socket;
      }
    }

    Lock.Unlock();

    sTCPSocket *newconn;
    if (!HostSocket.WaitForEvents(nreads,readset,nwrites,writeset,&newconn, 100))
    {
      sDPrintF(L"httpd wtf\n");
      aborted = sTRUE;
      break;
    }

    Lock.Lock();

    if (newconn) // new connection?
    {

      if (!FreeList.IsEmpty())
      {
        c = FreeList.RemTail();
        //sDPrintF(L"[http] new connection %08x\n",(sDInt)c);
        sSetMem(c,0,sizeof(Connection));
        c->Socket = newconn;
        c->Error=L"Malformed Request";
        c->State=CS_GETREQUEST;
        c->Method=CM_UNKNOWN;
        c->HeadersDone = sFALSE;
        c->ContentLength = 0;
        c->DataPackets.Clear();
        c->POSTData=0;

        ConnList.AddTail(c);
      }
      else
      {
        sLogF(L"http",L"out of connections\n");
        HostSocket.CloseConnection(newconn);
      }

    }

    // read set: retrieve request lines
    for (sInt i=0; i<nreads; i++)
    {
      sFORALL_LIST(ConnList,c) if (c->Socket==readset[i])
      {
        //sDPrintF(L"[http] read ready %08x\n",(sDInt)c);
        sChar8 buffer[REQLINE];
        sDInt read;
        if (c->Socket->Read(buffer,REQLINE,read))
        {
          sInt op=sGetStringLen(c->RequestLine);
          for (sInt ip=0; ip<read; ip++)
          {
            sInt ch=buffer[ip];
            if ( (c->Method!=CM_POST) || (!c->HeadersDone) )
            {
              // parse the request line 
              if (ch==10)
              {
                c->RequestLine[op--]=0;
                while (op>=0 && c->RequestLine[op]<32) c->RequestLine[op--]=0;
                if (!c->RetCode) 
                {
                  ParseRequestLine(c);
                  if (c->RetCode) StartServing(c);
                }
                op=0;
              }
              else
              {
                if (op<(REQLINE-1))
                {
                  c->RequestLine[op++]=ch;
                }
              }
            } else {
              // init POST data of not happened yet            
              if (!c->POSTData)
              {
                if (c->ContentLength>MAXPOSTDATA)
                {
                  c->Error=L"POST payload too big";
                  c->RetCode = 400;
                  StartServing(c);
                  break;
                }
                // We allocate one byte more and add a null-terminator
                // at the end, which makes it easier to work with strings as post data.
                c->POSTData=new sChar[c->ContentLength+1];
                c->POSTData[c->ContentLength]=0;
                c->POSTDataSize=0;
              }

              // parse the POST data
              c->POSTData[c->POSTDataSize++] = ch;
              if (c->POSTDataSize==c->ContentLength)
              {
                // let's go
                c->Error=L"OK";
                c->RetCode = 200;
                const sChar *pct = c->ContentType;
                sString<64> Boundary;
                sString<64> Boundary2;
                sString<64> Boundary3;
                // todo: this should be a lot more generalized.
                if (sScanMatch(pct, L"multipart/form-data; boundary="))
                {
                  sCopyString(Boundary, pct);
                  Boundary2.PrintF(L"--%s\r\n", Boundary);
                  Boundary3.PrintF(L"--%s--\r\n", Boundary);
                  Boundary.Add(L"\r\n");
                  sInt offs=0;
                  sInt result = sFindString(c->POSTData+offs, Boundary2);
                  offs+=result;
                  offs+=Boundary2.Count();
                  while (offs<c->ContentLength)
                  {
                    sInt next = sFindString(c->POSTData+offs, Boundary2);
                    if (next<0) next = c->ContentLength-offs;
                    DataPacket *packet = new DataPacket(c->POSTData+offs, next);
                    packet->Parse();
                    c->DataPackets.AddTail(packet);

                    result = next;
                    offs+=result;
                    offs+=Boundary2.Count();
                  }
                } else {
                  DataPacket *packet = new DataPacket(c->POSTData, c->ContentLength);
                  c->DataPackets.AddTail(packet);
                }
                if (c->RetCode) StartServing(c);
              }
            }
          }
          c->RequestLine[op]=0;
        }
        break;
      }
    }

    // write set: serve
    for (sInt i=0; i<nwrites; i++)
    {
      sFORALL_LIST(ConnList,c) if (c->Socket==writeset[i])
      {
        //sDPrintF(L"[http] write ready %08x\n",(sDInt)c);
        if (c->Hndl)
        {
          sInt read=c->Hndl->GetData(Temp,1024);
          if (read)
          {
            // ok, this may block, but who cares
            c->Socket->WriteAll(Temp,read);            
          }
          else
            c->Socket->Disconnect();
        }
        else if (c->Mem)
        {
          if (c->MemSize)
          {
            sInt chunk=sMin(4096,c->MemSize);
            sDInt written;
            c->Socket->Write(c->Mem,chunk,written);
            c->Mem+=written;
            c->MemSize-=written;
          }
          else
            c->Socket->Disconnect();
        }
        else if (c->File)
        {
          sInt offs=c->File->GetOffset();
          sInt size=c->File->GetSize();
          if (offs<size)
          {
            sInt chunk=sMin(4096,size-offs);
            c->File->Read(Temp,chunk);
            sDInt written;
            c->Socket->Write(Temp,chunk,written);
            if (written<chunk) 
              c->File->SetOffset(offs+written);
            offs+=written;
          }
          else
            c->Socket->Disconnect();
        }
        break;
      }
    }

    // check connection status and discard old connections
    c = ConnList.GetHead();
    while (!ConnList.IsEnd(c))
    {
      Connection *next = ConnList.GetNext(c);
      if (!c->Socket->IsConnected())
      {
        //sDPrintF(L"[http] closing %08x\n",(sDInt)c);
        sDelete(c->File);
        sDelete(c->Hndl);
        sDeleteArray(c->POSTData);
        sDeleteAll(c->DataPackets);
        HostSocket.CloseConnection(c->Socket);
        ConnList.Rem(c);
        FreeList.AddTail(c);
      }
      c=next;
    }
  }

  delete[] readset;
  delete[] writeset;

  if(!aborted)
    Lock.Unlock();

  return sTRUE;
}

/****************************************************************************/
/****************************************************************************/

sHTTPServer::SimpleHandler::SimpleHandler()
{
  FirstB=0;
  SetParamArray(sNULL);
}

sHTTPServer::SimpleHandler::~SimpleHandler()
{
  while (FirstB)
  {
    Buffer *next=FirstB->Next;
    delete[] FirstB;
    FirstB=next;
  }
}


// write a binary buffer
void sHTTPServer::SimpleHandler::Write(void *ptr, sInt size)
{
  while (size)
  {
    if (CurWriteFill==BUFSIZE)
    {
      CurWriteB->Next=new Buffer;
      CurWriteB=CurWriteB->Next;
      CurWriteB->Next=0;
      CurWriteFill=0;
    }
    sInt chunk=sMin(size,BUFSIZE-CurWriteFill);
    sCopyMem(CurWriteB->Mem+CurWriteFill,ptr,chunk);
    ptr=(sU8 *)ptr+chunk;
    size-=chunk;
    CurWriteFill+=chunk;
    TotalLen+=chunk;
  }
}


// print string (will be converted to ASCII)
void sHTTPServer::SimpleHandler::Print(const sChar *str)
{
  sChar8 temp[1024];
  sInt len=0;
  while (*str)
  {
    temp[len++]=*str++; // TODO: maybe a better conversion? Unicode support?
    if (len==1024)
    {
      Write(temp,len);
      len=0;
    }
  }
  if (len) Write(temp,len);
}



void sHTTPServer::SimpleHandler::WriteHTMLHeader(const sChar *title, const sChar *stylesheet, const sChar *addheader)
{
  Print(
     L"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n"
     L"<html><head>\n"
  );

  if (title) PrintF(L"  <title>%s</title>\n",title);
  if (stylesheet) PrintF(L"  <link rel=\"stylesheet\" href=\"%s\">\n",stylesheet);
  if (addheader) Print(addheader);

  Print(
    L"</head>\n"
    L"<body>\n"
  );

  IsHTML=1;
}


void sHTTPServer::SimpleHandler::WriteHTMLFooter()
{
  Print(
    L"</body>\n"
    L"</html>\n"
  );
}

sInt sHTTPServer::SimpleHandler::GetParamI(const sChar *name, sInt deflt)
{
  for (sInt i=0; i<PCount; i++)
    if (!sCmpString(name,Params[i].Name))
      return Params[i].ValI;
  return deflt;
}

const sChar* sHTTPServer::SimpleHandler::GetParamS(const sChar *name,const sChar *deflt)
{
  for (sInt i=0; i<PCount; i++)
    if (!sCmpString(name,Params[i].Name))
      return Params[i].ValS;
  return deflt;
}

void * sHTTPServer::SimpleHandler::GetParamBin(const sChar *name,sInt &size)
{
  for (sInt i=0; i<PCount; i++)
  {
    if (!sCmpString(name,Params[i].Name))
    {
      size = Params[i].SizeBin;
      return Params[i].ValBin;
    }
  }

  size=0;
  return 0;
}

sHTTPServer::HandlerResult sHTTPServer::SimpleHandler::Init(sHTTPServer::Connection *c)
{
  FirstB = new Buffer;
  FirstB->Next=0;  

  CurWriteB=FirstB;
  CurWriteFill=0;
  TotalLen=0;
  IsHTML=0;
  CurReadB=FirstB;
  CurReadPos=0;

  sString<128> baseurl;
  if (c->Method==CM_POST)
  {
    sCopyString(baseurl, c->URL);
    PCount=sParsePOSTData(c,Params.GetDirect(),Params.GetCount());
  } else {
    PCount=sParseURL(c->URL,baseurl,Params.GetDirect(),Params.GetCount());
  }
  
  return WriteDocument(baseurl);
}


void sHTTPServer::SimpleHandler::GetAdditionalHeaders(const sStringDesc &str)
{
  sSPrintF(str,L"Content-Length: %d\r\n",TotalLen);
  if (IsHTML)
    sSPrintF(sGetAppendDesc(str),L"Content-Type: text/html\r\n");
};


sBool sHTTPServer::SimpleHandler::DataAvailable()
{
  return sTRUE;
}



sInt sHTTPServer::SimpleHandler::GetData(sU8 *buffer, sInt len)
{
  sInt written=0;
  while (len && CurReadB)
  {
    sInt bufl=(CurReadB==CurWriteB)?CurWriteFill:BUFSIZE;
    sInt chunk=sMin(len,bufl-CurReadPos);
    sCopyMem(buffer,CurReadB->Mem+CurReadPos,chunk);
    CurReadPos+=chunk;
    written+=chunk;
    len-=chunk;
    if (CurReadPos==bufl)
    {
      CurReadPos=0;
      CurReadB=CurReadB->Next;
    }
  }
  return written;
}

/****************************************************************************/
/****************************************************************************/

sInt sGetHTMLChar(sChar ch,sChar8 *outbuf)
{
  if (ch<0 || ch>=128 || ch=='<' || ch=='>'  || ch=='\"' || ch=='\'' || ch=='&')
  {
    sU16 c=ch;
    sInt l=2;
    sBool yo=sFALSE;
    outbuf[0]='&';
    outbuf[1]='#';

    if (yo || c>=10000) { outbuf[l++]='0'+(c/10000); c%=10000; yo=1; }
    if (yo || c>=1000) { outbuf[l++]='0'+(c/1000); c%=1000; yo=1; }
    if (yo || c>=100) { outbuf[l++]='0'+(c/100); c%=100; yo=1; }
    if (yo || c>=10) { outbuf[l++]='0'+(c/10); c%=10; }
    outbuf[l++]='0'+c;
    outbuf[l++]=';';
    return l;
  }
  outbuf[0]=ch;
  return 1;
}

/****************************************************************************/

sInt sParseURL(const sChar *url, const sStringDesc &base, sURLParam *params, sInt maxparams)
{
  sInt nparam=0;

  sInt qpos=sFindFirstChar(url,'?');
  if (qpos<0)
  {
    sCopyString(base,url);
    return 0;
  }

  sCopyString(base.Buffer,url,qpos+1);
  url=url+qpos+1;
  while (*url)
  {
    const sChar *next;
    sString<128> pstr;
    qpos=sFindFirstChar(url,'&');
    if (qpos>0)
    {
      next=url+qpos+1;
      sCopyString(pstr,url,qpos+1);
    }
    else
    {
      next=url+sGetStringLen(url);
      sCopyString(pstr,url);
    }

    sInt epos=sFindFirstChar(url,'=');
    if (epos>0 && nparam<maxparams)
    {
      sURLParam &p=params[nparam++];
      sCopyString(p.Name,pstr,epos+1);
      sCopyString(p.ValS,pstr+epos+1);
      p.ValI=0;
      const sChar *s=p.ValS;
      sScanInt(s,p.ValI);
      p.ValBin=0; // binary data is used with POST requests only
      p.SizeBin=0; // binary data is used with POST requests only
    }

    url=next;
  }
  return nparam;
}

/****************************************************************************/

sInt sParsePOSTData(const sHTTPServer::Connection *c, sURLParam *params, sInt maxparams)
{
  sInt nparam=0;

  sInt qpos;

  const sHTTPServer::DataPacket *dp;
  dp=c->DataPackets.GetHead();
  while (1)
  {
    const sChar *scan = dp->ContentDisposition;
    if (sScanMatch(scan, L"form-data; "))
    {
      sString<64> key;
      if (sScanMatch(scan, L"name=\""))
      {
        sInt i=0;
        while (*scan!=L'"') { key[i++] = *scan; scan++; }
        key[i]=0;

        sURLParam &p=params[nparam++];
        sCopyString(p.Name,key);
        sCopyString(p.ValS,dp->Payload, sMin(p.ValS.Size(), dp->ContentLength));
        p.ValI=0;
        const sChar *s=p.ValS;
        sScanInt(s,p.ValI);
        p.ValBin=(void *)dp->Payload;
        p.SizeBin=dp->ContentLength; // binary data is used in form-data encoding only
      }

    } else {
      // assume url encoded params
      sInt len = sGetStringLen(dp->Payload);
      sChar *buffer = new sChar[len+1];
      sCopyString(buffer,dp->Payload,len+1);
      sDecodeURL(buffer);

      const sChar *postdata = buffer;
      while (*postdata)
      {
        const sChar *next;
        sString<128> pstr;
        qpos=sFindFirstChar(postdata,'&');
        if (qpos>0)
        {
          next=postdata+qpos+1;
          sCopyString(pstr,postdata,qpos+1);
        }
        else
        {
          next=postdata+sGetStringLen(postdata);
          sCopyString(pstr,postdata);
        }

        sInt epos=sFindFirstChar(postdata,'=');
        if (epos>0 && nparam<maxparams)
        {
          sURLParam &p=params[nparam++];
          sCopyString(p.Name,pstr,epos+1);
          sCopyString(p.ValS,pstr+epos+1);
          p.ValI=0;
          const sChar *s=p.ValS;
          sScanInt(s,p.ValI);
          p.ValBin=0; // binary data is used in form-data encoding only
          p.SizeBin=0; // binary data is used in form-data encoding only
        }

        postdata=next;
      }

      delete[] buffer;
    }

    if (c->DataPackets.IsLast(dp)) break;
    dp = c->DataPackets.GetNext(dp);
  }

  return nparam;
}

/****************************************************************************/

sHTTPServer::DataPacket::DataPacket(const sChar *buffer, sInt buffersize)
{
  Buffer=buffer;
  BufferSize=buffersize;
  sCopyString(ContentType, L"");
  sCopyString(ContentDisposition, L"");
  ContentLength = -1;
  Payload = Buffer;
}

void sHTTPServer::DataPacket::Parse()
{
  const sChar *p=Buffer;

  sString<1024> linebuffer;
  const sChar *line;

  sString<64> key;

  sBool headersdone = sFALSE;
  sInt linelen = sFindFirstChar(p, L'\n');
  while ( (linelen>0) && (*(p+linelen-1)<32) ) linelen--;
  sCopyString(linebuffer, p, linelen+1);
  linebuffer[linelen]=0;
  line = linebuffer;
  while (!headersdone)
  {
    if (linelen==0)
    {
      headersdone = sTRUE;
    } else if (sScanMatch(line, L"Content-Length: "))
    {
      sScanInt(p, ContentLength);
    } else if (sScanMatch(line, L"Content-Type: "))
    {
      sCopyString(ContentType, line, 128);
    } else if (sScanMatch(line, L"Content-Disposition: "))
    {
      sCopyString(ContentDisposition, line, 128);
    } else {
      // ignore unknown headers
    }

    p+=linelen+2;
    linelen = sFindFirstChar(p, L'\n');
    while ( (linelen>0) && (*(p+linelen)<32) ) linelen--;
    sCopyString(linebuffer, p, linelen+1);
    linebuffer[linelen]=0;
    line = linebuffer;
  }

  Payload = p;
  // calc contentlength, if it wasn't supplied.
  if (ContentLength<=0)
  {
    ContentLength = BufferSize - (p-Buffer);
  }
}
/****************************************************************************/
