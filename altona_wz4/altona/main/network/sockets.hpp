/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_NETWORK_SOCKETS_HPP
#define FILE_NETWORK_SOCKETS_HPP

#include "base/types.hpp"

/****************************************************************************/

enum sNetworkStatus
{
  sNET_DISCONNECTED,
  sNET_CONNECTING,
  sNET_CONNECTED,
};

sNetworkStatus sGetNetworkStatus();

/****************************************************************************/

struct sIPAddress
{
  sU8 Addr[4];

  sIPAddress() { Clear(); }
  sIPAddress(sU8 a, sU8 b, sU8 c, sU8 d) { Addr[0]=a; Addr[1]=b; Addr[2]=c; Addr[3]=d; }
  sIPAddress(const sIPAddress &a) { sCopyMem(this,&a,sizeof(sIPAddress)); }

  bool operator == (const sIPAddress &a) const { return !sCmpMem(this,&a,sizeof(sIPAddress)); }
  bool operator != (const sIPAddress &a) const { return !(*this == a); }

  void Clear() { sClear(*this); }
  const sChar *ToString(const sStringDesc &dest) const { sSPrintF(dest,L"%d.%d.%d.%d",Addr[0],Addr[1],Addr[2],Addr[3]); return dest.Buffer; }
};

sFormatStringBuffer& operator% (sFormatStringBuffer &, const sIPAddress &);
sBool sScanIPAddress(const sChar *&ptr, sIPAddress &ip);

typedef sU16 sIPPort;

/****************************************************************************/

sBool sGetLocalHostName(const sStringDesc &str);
sBool sGetLocalAddress(sIPAddress &address);
sBool sGetHostName(const sStringDesc &str, const sIPAddress &address);
sBool sResolveName(const sChar *hostname, sIPAddress &address);
sBool sResolveNameAndPort(const sChar *hostname, sIPAddress &address, sIPPort &port);

// Gets online name of local user associated to the specified
// sJoypad-id and stores the result of the operation in dest.
sBool sGetOnlineUserName(const sStringDesc &dest, sInt joypadId);

// Gets if user with the specified sJoypad-id is connected
// to the console vendor online service.
sBool sIsUserOnline(sInt joypadId);

// Gets online id of a local user associated to the specified
// sJoypad-id and stores the result of the operation in dest.
sBool sGetOnlineUserId(sU64 &dest, sInt joypadId);

/****************************************************************************/

enum sSocketError
{
  sSE_OK=0,
};

/****************************************************************************/

class sTCPSocket
{
public:

  void Disconnect();
  sBool IsConnected();

  sBool GetPeerAddress(sIPAddress &address);
  sBool GetPeerPort(sIPPort &port);

  // check if sockets are able to read/write
  sBool CanRead();
  sBool CanWrite();

  // synchronous read
  // a return value of sTRUE and a length of 0 denotes the 
  // end of the stream
  virtual sBool Read(void *buffer, sDInt size, sDInt &read);

  // synchronous write. May write less than specified, it's
  // up to you to deliver the rest
  virtual sBool Write(const void *buffer, sDInt bytes, sDInt &written);

  // enable/disable Nagle algorithm. Don't use this unless you really know what
  // you're doing and why!
  void SetNagle(sBool enable);

  // convenience functions
  sBool ReadAll(void *buffer, sDInt bytes);
  sBool WriteAll(const void *buffer, sDInt bytes);
  template<typename T> inline sBool ReadAll(T &var) { return ReadAll(&var,sizeof(var)); }
  template<typename T> inline sBool WriteAll(const T &var) { return WriteAll(&var,sizeof(var)); }
  sBool ReadAll(const sStringDesc &s) { sInt len=0; ReadAll(len); if(len>s.Size) sFatal(L"ReadAll(string) buffer overflow"); return ReadAll(s.Buffer,len*sizeof(sChar)); }
  sBool WriteAll(const sStringDesc &s) { sInt len=sGetStringLen(s.Buffer)+1; WriteAll(len); return WriteAll(s.Buffer,len*sizeof(sChar)); }

  // check for read/write errors
  sBool CheckReadError() const { return TransferError&1; }
  sBool CheckWriteError() const { return TransferError&2; }
  sBool CheckError() const { return TransferError; }
  void ClearErrorFlags() { TransferError=0; }
protected:

  friend class sTCPClientSocket;
  friend class sTCPHostSocket;

  struct Private;
  Private *P;
  sInt TransferError;

  sTCPSocket();
  virtual ~sTCPSocket();
};

/****************************************************************************/

class sTCPClientSocket : public sTCPSocket
{
public:

  // connect to server
  sBool Connect(sIPAddress address, sIPPort port);

  sTCPClientSocket();
  virtual ~sTCPClientSocket();
};

/****************************************************************************/

class sTCPHostSocket : public sTCPSocket
{
public:

  // start listening on a specific port
  sBool Listen(sIPPort port); // TODO: specify interface address?

  // wait for a new connection. A return value of sFALSE indicates an error,
  // in case of timeout, "connection" will be set to NULL
  sBool WaitForNewConnection(sTCPSocket *&connection, sInt timeout=1000);

  // or better: wait for events.
  // this call is equivalent to the select() call - you specify two arrays 
  // of socket pointers for read and write, and you'll get them
  // back with only the sockets that are readable or writable. Additionally
  // you can specify a pointer to a socket pointer that receives new connections
  sBool WaitForEvents(sInt &numreads, sTCPSocket **reads, sInt &numwrites, sTCPSocket **writes, sTCPSocket **newconn, sInt timeout=1000);

  // closes connection, discards socket object
  void CloseConnection(sTCPSocket *&connection);

  // host sockets can neither read nor write themselves
  sBool Write(const void *, sDInt, sDInt &) { return sFALSE; }
  sBool Read(void *, sDInt, sDInt &) { return sFALSE; }

  sTCPHostSocket();
  virtual ~sTCPHostSocket();

private:

  sTCPSocket *Accept();

};

/****************************************************************************/

class sUDPSocket
{
public:

  sUDPSocket();
  ~sUDPSocket();

  // init socket.
  // broadcast: specify sTRUE if you're going to broadcast packets
  // port: port to bind to. Set to 0 if you don't care
  sBool Init(sBool broadcast, sIPPort port);

  // close socket
  void Close();

  // check if sockets are able to read/write
  sBool CanRead();
  sBool CanWrite();

  // wait for read/write to get available
  // a timeout of 0 means infinite
  sBool Wait(sBool read, sBool write, sInt timeout);

  // get packet
  sBool Read(void *buffer, sInt size, sInt &read, sIPAddress &srcaddr, sIPPort &srcport);

  // send packet to destination
  sBool Write(const sIPAddress &destaddr, sIPPort destport, const void *buffer, sInt size);

  // broadcast packet to everyone
  sBool Broadcast(sIPPort destport, const void *buffer, sInt size);

  // convenience
  template <typename T> inline sBool Write(const sIPAddress &destaddr, sIPPort destport, const T &var) { return Write(destaddr,destport,&var,sizeof(var)); }
  template <typename T> inline sBool Broadcast(sIPPort destport, const T &var) { return Broadcast(destport,&var,sizeof(var)); }

private: 

  struct Private;
  Private *P;
};

/****************************************************************************/

#endif // FILE_NETWORK_SOCKETS_HPP

