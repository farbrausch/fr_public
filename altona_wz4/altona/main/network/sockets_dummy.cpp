/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#define sPEDANTIC_WARN 1

#include "network/sockets.hpp"

// please add all applicable platforms here
#if defined(sCONFIG_DUMMYNETWORK)

#include "base/system.hpp"

sNetworkStatus sGetNetworkStatus() { return sNET_DISCONNECTED; }

/****************************************************************************/
/****************************************************************************/

sBool sGetLocalHostName(const sStringDesc &str)
{
  str.Buffer[0]=0;
  return sFALSE;
}

sBool sGetLocalAddress(sIPAddress &address)
{
  sClear(address);
  return sFALSE;
}

sBool sResolveName(const sChar *hostname, sIPAddress &address)
{
  sClear(address);
  return sFALSE;
}

/****************************************************************************/
/****************************************************************************/


sTCPSocket::sTCPSocket() 
{ 
  P = 0;
  TransferError=0;
}

sTCPSocket::~sTCPSocket() 
{ 
}


sBool sTCPSocket::IsConnected()
{
  return sFALSE;
}


void sTCPSocket::Disconnect()
{
}


sBool sTCPSocket::GetPeerAddress(sIPAddress &address)
{
  sClear(address);
  return sFALSE;
}


sBool sTCPSocket::GetPeerPort(sIPPort &port)
{
  sClear(port);
  return sFALSE; 
}


sBool sTCPSocket::Write(const void *buffer, sDInt bytes, sDInt &written)
{
  written=0;
  TransferError |= 2;
  return sFALSE;
}


sBool sTCPSocket::Read(void *buffer, sDInt size, sDInt &read)
{
  read=0;
  TransferError |= 1;
  return sFALSE;
}


// enable/disable Nagle algorithm. Don't use this unless you really know what
// you're doing and why!
void sTCPSocket::SetNagle(sBool enable)
{
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
  return sFALSE;
}

// wait for a new connection. A return value of sFALSE indicates an error,
// in case of timeout, "connection" will be set to NULL
sBool sTCPHostSocket::WaitForNewConnection(sTCPSocket *&connection, sInt timeout)
{
  connection=0;
  return sFALSE;
}


sBool sTCPHostSocket::WaitForEvents(sInt &numreads, sTCPSocket **reads, sInt& numwrites, sTCPSocket **writes, sTCPSocket **newconn, sInt timeout)
{
  if (newconn) *newconn=0;
  numreads=numwrites=0;
  return sFALSE;
}


void sTCPHostSocket::CloseConnection(sTCPSocket *&connection)
{
}


sTCPSocket* sTCPHostSocket::Accept()
{
  return 0;
}
/****************************************************************************/
/****************************************************************************/

sUDPSocket::sUDPSocket() 
{ 
  P=0;
}

sUDPSocket::~sUDPSocket()
{
}

sBool sUDPSocket::Init(sBool broadcast, sIPPort port)
{
  return sFALSE;
}

void sUDPSocket::Close()
{
}

sBool sUDPSocket::CanRead()
{
  return sFALSE;
}

// check if sockets are able to write
sBool sUDPSocket::CanWrite()
{
  return sFALSE;
}

// wait for read/write to get available
sBool sUDPSocket::Wait(sBool read, sBool write, sInt timeout)
{
  return sFALSE;
}

// get packet
sBool sUDPSocket::Read(void *buffer, sInt size, sInt &read, sIPAddress &srcaddr, sIPPort &srcport)
{
  read=0;
  return sFALSE;
}

// send packet to destination
sBool sUDPSocket::Write(const sIPAddress &destaddr, sIPPort destport, const void *buffer, sInt size)
{
  return sFALSE;
}

// broadcast packet to everyone
sBool sUDPSocket::Broadcast(sIPPort destport, const void *buffer, sInt size)
{
  return sFALSE;
}

#endif // sPLATFORM(s)
