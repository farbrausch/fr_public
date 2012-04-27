/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_SIMPLE_SERVER_HPP
#define FILE_SIMPLE_SERVER_HPP

#include "base/types2.hpp"
#include "network/sockets.hpp"
#include "base/system.hpp"

/****************************************************************************/

// the message is shared with the client

struct Message
{
  sU32 Magic;
  sU32 Command;
  sU32 Para0;
  sU32 Para1;
};

/****************************************************************************/

// each connection has it's own thread
// all the work is here

class ServerCon
{
public:
  sTCPSocket *Socket;
  sTCPHostSocket *HostSocket;
  sThread *Thread;
  sBool Done;

  ServerCon(sTCPSocket *,sTCPHostSocket *host);
  ~ServerCon();
  static void Start(sThread *t,void *);
  void Mainloop();
};

/****************************************************************************/

// the server just starts new connections

class Server
{
  sArray<ServerCon *> Cons;
  sThread *Thread;
  sTCPHostSocket HostSocket;
public:
  Server(sInt port);
  ~Server();
  static void Start(sThread *t,void *);
  void Mainloop();
};

/****************************************************************************/

#endif 

