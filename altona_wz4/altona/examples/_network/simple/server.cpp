/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "server.hpp"

/****************************************************************************/

// this is a simple server, that receives packages, changes them a bit and sends them back.

// there is one thread per connection, and ServerCon handles that connection.

ServerCon::ServerCon(sTCPSocket *sock,sTCPHostSocket *host)
{
  Socket = sock;
  Thread = 0;
  Done = 0;
  HostSocket = host;

  new sThread(ServerCon::Start,0,0x8000,this);
}

ServerCon::~ServerCon()
{
  while(Thread==0) sSleep(1);
  Socket->Disconnect();
  delete Thread;
  HostSocket->CloseConnection(Socket);
}

void ServerCon::Start(sThread *t,void *user)
{
  ServerCon *con = (ServerCon *) user;
  con->Thread = t;
  con->Mainloop();
}

void ServerCon::Mainloop()
{
  Message msg;
  sBool ok;
  sPrintF(L"server: connection started\n");
  while(Socket->IsConnected() && Thread->CheckTerminate())
  {
    ok = Socket->ReadAll(msg);
    if(!ok) break;

    if(msg.Magic!=0xdeadbeef)  break;

    sPrintF(L"server: got %08x %08x\n",msg.Para0,msg.Para1);
    msg.Para0 = ~msg.Para1;
    msg.Para1 = 0;

    ok = Socket->WriteAll(msg);
    sPrintF(L"server: send %08x %08x (%d)\n",msg.Para0,msg.Para1,ok);
    if(!ok) break;
  }
  sPrintF(L"server: connection terminated\n");
}

/****************************************************************************/

// Server accepts connections and created ServerCon handlers.

Server::Server(sInt port)
{
  HostSocket.Listen(port);
  Thread = 0;

  new sThread(Start,0,0x8000,this);
}

Server::~Server()
{
  while(Thread==0) sSleep(1);
  delete Thread;
  sDeleteAll(Cons);
}

void Server::Start(sThread *t,void *user)
{
  Server *serv = (Server *) user;
  serv->Thread = t;
  serv->Mainloop();
}

void Server::Mainloop()
{
  sTCPSocket *sock;

  sPrintF(L"server: mainloop started\n");
  while(HostSocket.IsConnected() && (!Thread || Thread->CheckTerminate()))
  {
    sDeleteTrue(Cons,&ServerCon::Done);

    while(HostSocket.WaitForNewConnection(sock,100) && sock)
    {
      Cons.AddTail(new ServerCon(sock,&HostSocket));
    }
  }
  sPrintF(L"server: mainloop terminated\n");
}

/****************************************************************************/

