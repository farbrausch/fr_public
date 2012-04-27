/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#define sPEDANTIC_WARN 1

#include "network/sockets.hpp"

// please add all applicable platforms here
#if sPLATFORM==sPLAT_WINDOWS || sPLATFORM==sPLAT_LINUX

#include "base/system.hpp"
#include <errno.h>

/****************************************************************************/
/****************************************************************************/

#undef new

#define sNET_ALLOC(size) sDEFINE_NEW sU8[size]
#define sNET_FREE(ptr) delete[] ((sU8*)ptr)

#define sNET_CLASS \
  void *operator new (sCONFIG_SIZET size) { return sNET_ALLOC(size); } \
  void operator delete (void *ptr) { sNET_FREE(ptr); }

#define sNET_SOCKTYPE int

#define sNETERR(x) x
#define sNETGETERR errno

/****************************************************************************/
/****************************************************************************/

#if sPLATFORM==sPLAT_WINDOWS

#include <winsock2.h>
#pragma comment (lib,"ws2_32.lib")

static void sInitNetwork()
{
  WSADATA wsadata;
  WSAStartup(0x0202,&wsadata);
}

static void sExitNetwork()
{
  WSACleanup();
}

static void sPrintNetError(sInt error, const sStringDesc &str)
{
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,0,error,0,str.Buffer,str.Size,0);
}

sADDSUBSYSTEM(Network,0x14,sInitNetwork,sExitNetwork);

#undef sNET_SOCKTYPE
#define sNET_SOCKTYPE SOCKET

#undef sNETERR
#define sNETERR(x) WSA#x

#undef sNETGETERR
#define sNETGETERR WSAGetLastError()

#define sINVALID_SOCKET INVALID_SOCKET
#define sSOCKET_ERROR SOCKET_ERROR
#define sFD_SET(fd,set,nfds) {FD_SET(fd,set);nfds++;}

typedef int socklen_t;

sNetworkStatus sGetNetworkStatus() 
{
  if (sIsUserOnline(0))
    return sNET_CONNECTED;

  return sNET_DISCONNECTED;
}


#else // bsd sockets

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <unistd.h>
#include <netdb.h>
#include <string.h>

#define sINVALID_SOCKET -1
#define sSOCKET_ERROR ((ssize_t)-1)
#define sFD_SET(fd,set,nfds) {FD_SET(fd,set);nfds=sMax(nfds,(fd)+1);}
#define closesocket close

static void sPrintNetError(sInt error, const sStringDesc &str)
{
  sCopyString(str,strerror(error));
}

sNetworkStatus sGetNetworkStatus() 
{ 
  return sNET_CONNECTED;
}


#endif

/****************************************************************************/
/****************************************************************************/

sBool sGetLocalHostName(const sStringDesc &str)
{
  sChar8 name[512];
  str.Buffer[0]=0;
  if (!gethostname(name,512))
  {
    sCopyString(str,name);
    return sTRUE;
  }
  return sFALSE;
}

sBool sGetLocalAddress(sIPAddress &address)
{
  return sResolveName(L"",address);
}

sBool sResolveName(const sChar *hostname, sIPAddress &address)
{
  sChar8 name[512];
  sCopyString(name,hostname,511);

  sClear(address);

  struct hostent *result=gethostbyname(name);

  if (result && result->h_addrtype==AF_INET && result->h_length==4)
  {
    sInt n=0;
    while (result->h_addr_list[n]) n++;
    if (n>1)
    {
      // if there are several IP addresses for a host name, pick one
      // randomly. In most cases this is due to load balancing anyway.
      sRandom rnd;
      rnd.Seed(sGetRandomSeed());
      n=rnd.Int(n);
    }
    else n--;

    for (sInt i=0; i<4; i++)
      address.Addr[i]=result->h_addr_list[n][i];
    return sTRUE;
  }
  return sFALSE;
}

sBool sGetHostName(const sStringDesc &str, const sIPAddress &address)
{
  struct hostent *result = gethostbyaddr((const char*)address.Addr,4,AF_INET);
  if(result)
  {
    sCopyString(str.Buffer,result->h_name,str.Size);
    return sTRUE;
  }
  return sFALSE;
}

/****************************************************************************/
/****************************************************************************/

struct sTCPSocket::Private
{
  sNET_CLASS

  sNET_SOCKTYPE Socket;
  sockaddr_in Address;
  sBool Connected;

  void HandleError()
  {
    // TODO: do something more sensible than printing here

    int err=sNETGETERR;

    Connected=sFALSE;

    sString<512> errstr;
    sPrintNetError(err,errstr);
    sLogF(L"net",L"ERROR: %s\n",errstr);


  }

};

sTCPSocket::sTCPSocket() 
{ 
  P = new Private; sClear(*P); 
  P->Socket=sINVALID_SOCKET;
  TransferError = 0;
}

sTCPSocket::~sTCPSocket() 
{ 
  Disconnect();
  delete P; 
}


sBool sTCPSocket::IsConnected()
{
  if (P->Socket==sINVALID_SOCKET) return sFALSE;
  return P->Connected;
}


void sTCPSocket::Disconnect()
{
  if (P->Socket!=sINVALID_SOCKET)
  {
    shutdown(P->Socket,2);
    closesocket(P->Socket);
    P->Socket=sINVALID_SOCKET;
  }
  P->Connected=sFALSE;
}


sBool sTCPSocket::GetPeerAddress(sIPAddress &address)
{
  sClear(address);
  if (!IsConnected()) return sFALSE;
  sCopyMem(&address,&P->Address.sin_addr,4);
  return sTRUE;
}


sBool sTCPSocket::GetPeerPort(sIPPort &port)
{
  sClear(port);
  if (!IsConnected()) return sFALSE;
  port=ntohs(P->Address.sin_port);
  return sTRUE;
}


// check if sockets are able to read
sBool sTCPSocket::CanRead()
{
  if (!IsConnected()) return sFALSE;

  fd_set set;
  sInt nfds = 0;
  FD_ZERO(&set);
  sFD_SET(P->Socket,&set,nfds);

  timeval tv;
  tv.tv_sec=tv.tv_usec=0;

  select(nfds,&set,0,0,&tv);
  return FD_ISSET(P->Socket,&set)?sTRUE:sFALSE;
}

// check if sockets are able to write
sBool sTCPSocket::CanWrite()
{
  if (!IsConnected()) return sFALSE;

  fd_set set;
  sInt nfds = 0;
  FD_ZERO(&set);
  sFD_SET(P->Socket,&set,nfds);

  timeval tv;
  tv.tv_sec=tv.tv_usec=0;

  select(nfds,0,&set,0,&tv);
  return FD_ISSET(P->Socket,&set)?sTRUE:sFALSE;

}

// synchronous write. May write less than specified, it's
// up to you to deliver the rest
sBool sTCPSocket::Write(const void *buffer, sDInt size, sDInt &written)
{
  written=0;
  if (!IsConnected())
  {
    TransferError |= 2;
    return sFALSE;
  }

  sInt truncSize = (sInt) size;
  sVERIFY(truncSize == size);

  sInt res = send(P->Socket,(const char*)buffer,truncSize,0);
  if (res==sSOCKET_ERROR)
  {
    P->HandleError();
    TransferError |= 2;
    return sFALSE;
  }
  written=res;
  return sTRUE;
}


// synchronous read
sBool sTCPSocket::Read(void *buffer, sDInt size, sDInt &read)
{
  read=0;
  if (!IsConnected())
  {
    TransferError |= 1;
    return sFALSE;
  }

  sInt truncSize = (sInt) size;
  sVERIFY(truncSize == size);

  sInt res = recv(P->Socket,(char*)buffer,truncSize,0);
  
  if (res==sSOCKET_ERROR)
  {
    P->HandleError();
    return sFALSE;
  }
  read=res;
  return sTRUE;
}


// enable/disable Nagle algorithm. Don't use this unless you really know what
// you're doing and why!
void sTCPSocket::SetNagle(sBool enable)
{
  if (!IsConnected()) return;

  sU32 optval=enable?0:1;
  setsockopt(P->Socket,SOL_SOCKET,TCP_NODELAY,(char*)&optval,sizeof(optval));
}

/****************************************************************************/

sTCPClientSocket::sTCPClientSocket()
{
}

sTCPClientSocket::~sTCPClientSocket()
{
}

sBool sTCPClientSocket::Connect(sIPAddress address, sIPPort port)
{
  TransferError=0;

  if (IsConnected()) Disconnect();

  sNET_SOCKTYPE s=socket(PF_INET,SOCK_STREAM,0);

  if (s==sINVALID_SOCKET)
  {
    P->HandleError();
    return sFALSE;
  }

  sClear(P->Address);
  P->Address.sin_family=AF_INET;
  P->Address.sin_port=htons(port);
  sCopyMem(&P->Address.sin_addr,&address,4);

  sInt res=connect(s,(sockaddr*)&P->Address,sizeof(P->Address));

  if (!res)
  {
    P->Socket=s;
    P->Connected=sTRUE;
    return sTRUE;
  }

  P->HandleError();
  closesocket(s);

  return sFALSE;
}

/****************************************************************************/

sTCPHostSocket::sTCPHostSocket()
{
}

sTCPHostSocket::~sTCPHostSocket()
{
}

sBool sTCPHostSocket::Listen(sIPPort port)
{
  if (IsConnected()) Disconnect();

  sNET_SOCKTYPE s=socket(PF_INET,SOCK_STREAM,0);
  if (s==sINVALID_SOCKET)
  {
    P->HandleError();
    return sFALSE;
  }
  
  int on = 1;
#if sPLATFORM==sPLAT_WINDOWS
  setsockopt(s,SOL_SOCKET,SO_REUSEADDR,(const char *)&on,sizeof(on));
#else
  setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
#endif

  sClear(P->Address);
  P->Address.sin_family=AF_INET;
  P->Address.sin_port=htons(port);
  P->Address.sin_addr.s_addr=INADDR_ANY;

  sInt res = bind(s,(sockaddr*)&P->Address,sizeof(P->Address));

  if (!res)
  {

    res = listen(s, SOMAXCONN);
    if (!res)
    {
      P->Socket=s;
      P->Connected=sTRUE;
      return sTRUE;
    }
  }

  P->HandleError();
  closesocket(s);

  return sFALSE;
}

// wait for a new connection. A return value of sFALSE indicates an error,
// in case of timeout, "connection" will be set to NULL
sBool sTCPHostSocket::WaitForNewConnection(sTCPSocket *&connection, sInt timeout)
{
  connection=0;
  if (!IsConnected()) return sFALSE;

  fd_set set;
  sInt nfds = 0;
  FD_ZERO(&set);
  sFD_SET(P->Socket,&set,nfds);

  timeval tv;
  tv.tv_sec=timeout/1000;
  tv.tv_usec=(timeout%1000)*1000;

  sInt res=select(nfds,&set,0,0,timeout>=0?&tv:0);
  if (res==sSOCKET_ERROR)
  {
    P->HandleError();
    return sFALSE;
  }

  if (FD_ISSET(P->Socket,&set))
  {
    sTCPSocket *ts=Accept();
    if (!ts) return sFALSE;
    connection=ts;
    return sTRUE;
  }

  return sFALSE;
}


sBool sTCPHostSocket::WaitForEvents(sInt &numreads, sTCPSocket **reads, sInt& numwrites, sTCPSocket **writes, sTCPSocket **newconn, sInt timeout)
{
  if (newconn) *newconn=0;

  if (!IsConnected()) 
  {
    numreads=numwrites=0;
    return sFALSE;
  }

  fd_set readset, writeset;
  sInt nfds = 0;
  FD_ZERO(&readset);
  FD_ZERO(&writeset);

  for (sInt i=0; i<numreads; i++)
    sFD_SET(reads[i]->P->Socket,&readset,nfds);

  for (sInt i=0; i<numwrites; i++)
    sFD_SET(writes[i]->P->Socket,&writeset,nfds);

  sInt realnumreads=numreads;
  if (newconn)
  {
    sFD_SET(P->Socket,&readset,nfds);
    realnumreads++;
  }

  if (!realnumreads && !numwrites)
  {
    sSleep(timeout);
    return sTRUE;
  }

  timeval tv;
  tv.tv_sec=timeout/1000;
  tv.tv_usec=(timeout%1000)*1000;

  sInt res=select(nfds,(realnumreads?&readset:0),(numwrites?&writeset:0),0,(timeout>=0?&tv:0));
  if (res==sSOCKET_ERROR)
  {
    P->HandleError();
    numreads=numwrites=0;
    return sFALSE;
  }

  // new connection?
  if (FD_ISSET(P->Socket,&readset))
  {
    *newconn=Accept();
    if (!*newconn)
    {
      numreads=numwrites=0;
      return sFALSE;
    }
  }

  // read set...
  sInt nr=0;
  for (sInt i=0; i<numreads; i++)
    if (FD_ISSET(reads[i]->P->Socket,&readset))
      reads[nr++]=reads[i];
  numreads=nr;

  // write set...
  sInt nw=0;
  for (sInt i=0; i<numwrites; i++)
    if (FD_ISSET(writes[i]->P->Socket,&writeset))
      writes[nw++]=writes[i];
  numwrites=nw;

  return sTRUE;
}


void sTCPHostSocket::CloseConnection(sTCPSocket *&connection)
{
  connection->~sTCPSocket();
  sNET_FREE(connection);
  connection=0;
}


sTCPSocket* sTCPHostSocket::Accept()
{
  // we got one!
  sockaddr_in addr;
  socklen_t len=sizeof(addr);
  sNET_SOCKTYPE s=accept(P->Socket,(sockaddr*)&addr,&len);
  if (s==sINVALID_SOCKET)
  {
    P->HandleError();
    return 0;
  }
  
  sTCPSocket *ts=new(sNET_ALLOC(sizeof(sTCPSocket))) sTCPSocket;
  ts->P->Socket=s;
  ts->P->Address=addr;
  ts->P->Connected=sTRUE;
  return ts;
}


/****************************************************************************/
/****************************************************************************/

struct sUDPSocket::Private
{
  sNET_SOCKTYPE Socket;
};

sUDPSocket::sUDPSocket()
{
  P=0;
}

sUDPSocket::~sUDPSocket()
{
  Close();
}

// init socket.
// broadcast: specify sTRUE if you're going to broadcast packets
// port: port to bind to. Set to 0 if you don't care
sBool sUDPSocket::Init(sBool broadcast, sIPPort port)
{
  Close();

  sNET_SOCKTYPE s=socket(PF_INET,SOCK_DGRAM,0);
  if (s==sINVALID_SOCKET)
  {
    return sFALSE;
  }

  if (port)
  {
    sockaddr_in addr;
    sClear(addr);
    addr.sin_family=AF_INET;
    addr.sin_port=htons(port);
    sInt res = bind(s,(sockaddr*)&addr,sizeof(addr));

    if (res) // error?
    {
      closesocket(s);
      return sFALSE;
    }
  }

  if (broadcast)
  {
    int broadcast = 1;
    if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof(broadcast)) == -1) 
    {
      closesocket(s);
      return sFALSE;
    }
  }
  
  P = new Private;
  P->Socket=s;
  return sTRUE; 
}

// close socket
void sUDPSocket::Close()
{
  if (!P) return;
  closesocket(P->Socket);
  sDelete(P);
}

// check if sockets are able to read
sBool sUDPSocket::CanRead()
{
  if (!P) return sFALSE;

  fd_set set;
  sInt nfds = 0;
  FD_ZERO(&set);
  sFD_SET(P->Socket,&set,nfds);
  
  timeval tv;
  tv.tv_sec=tv.tv_usec=0;

  select(nfds,&set,0,0,&tv);
  return FD_ISSET(P->Socket,&set)?sTRUE:sFALSE;
}

// check if sockets are able to write
sBool sUDPSocket::CanWrite()
{
  if (!P) return sFALSE;

  fd_set set;
  sInt nfds = 0;
  FD_ZERO(&set);
  sFD_SET(P->Socket,&set,nfds);
  
  timeval tv;
  tv.tv_sec=tv.tv_usec=0;

  select(nfds,0,&set,0,&tv);
  return FD_ISSET(P->Socket,&set)?sTRUE:sFALSE;

}

// wait for read/write to get available
sBool sUDPSocket::Wait(sBool read, sBool write, sInt timeout)
{
  if (!P || (!read && !write))
  {
    sSleep(timeout);
    return sFALSE;
  }

  fd_set readset, writeset;
  FD_ZERO(&readset);
  FD_ZERO(&writeset);
  sInt nfds=0;
  if (read) { sFD_SET(P->Socket,&readset,nfds); }
  if (write) { sFD_SET(P->Socket,&writeset,nfds); }
  
  timeval tv;
  tv.tv_sec=timeout/1000;
  tv.tv_usec=(timeout%1000)*1000;

  sInt res=select(nfds,&readset,&writeset,0,timeout?&tv:0);
  return res!=-1 && (FD_ISSET(P->Socket,&readset)||FD_ISSET(P->Socket,&writeset));
}

// get packet
sBool sUDPSocket::Read(void *buffer, sInt size, sInt &read, sIPAddress &srcaddr, sIPPort &srcport)
{
  read=0;
  if (!P) return sFALSE;

  sockaddr_in from;
  sClear(from);
  from.sin_family=AF_INET;
  socklen_t fromlen=sizeof(from);

  sInt res=recvfrom(P->Socket,(char*)buffer,size,0,(sockaddr*)&from,&fromlen);
  sU32 addr=ntohl(from.sin_addr.s_addr);

  srcaddr.Addr[0]=(addr>>24)&0xff;
  srcaddr.Addr[1]=(addr>>16)&0xff;
  srcaddr.Addr[2]=(addr>> 8)&0xff;
  srcaddr.Addr[3]=(addr>> 0)&0xff;
  srcport=htons(from.sin_port);

  if (res==sSOCKET_ERROR)
  {
    read=0;
    return sFALSE;
  }

  read=res;
  return sTRUE;
}

// send packet to destination
sBool sUDPSocket::Write(const sIPAddress &destaddr, sIPPort destport, const void *buffer, sInt size)
{
  if (!P) return sFALSE;

  sockaddr_in to;
  sClear(to);
  to.sin_family=AF_INET;
  to.sin_port=htons(destport);
  sCopyMem(&to.sin_addr,&destaddr,4);

  sInt res=sendto(P->Socket,(const char*)buffer,size,0,(sockaddr*)&to,sizeof(to));

  return res==size;

}

// broadcast packet to everyone
sBool sUDPSocket::Broadcast(sIPPort destport, const void *buffer, sInt size)
{
  if (!P) return sFALSE;

  sockaddr_in to;
  sClear(to);
  to.sin_family=AF_INET;
    to.sin_port=htons(destport);
  to.sin_addr.s_addr=INADDR_BROADCAST;

  sInt res=sendto(P->Socket,(const char*)buffer,size,0,(sockaddr*)&to,sizeof(to));

  return res==size;
}

/****************************************************************************/

#endif // sPLATFORM(s)

