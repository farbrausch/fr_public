/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "client.hpp"

extern "C" const sChar8 WireTXT[];

/****************************************************************************/

Client::Client(sInt port,const sChar *ip)
{
  Connected = 0;
  ServerPort = port;
  IpString = ip;

  CheckConnect();
}

Client::~Client()
{
  Socket.Disconnect();
  sPrintF(L"client: disconnected from server\n");
}

// this will check for connection, and reconnect if the connection was lost.

// it is best to have a stateless server protocoll, so that each message is
// selfcontained. otherwise your state would be lost on reconnection
// an example for a really bad designed stateful protocol is the official FTP
// protocol, where the current directory is part of the state.

void Client::CheckConnect()
{
  if(!Socket.IsConnected())
  {
    if(sResolveNameAndPort(IpString,ServerIP,ServerPort))
    {
      sInt delay = 250;
      for(sInt i=0;i<10;i++)
      {
        Socket.Connect(ServerIP,ServerPort);
        if(Socket.IsConnected())
        {
          Connected = 1;
          break;
        }
        sSleep(delay);
        delay *= 2;
      }
    }

    if(Connected)
      sPrintF(L"client: connected to server\n");
  }
}

/****************************************************************************/

// just send a messages 

sBool Client::Test(sU32 p0,sU32 p1)
{
  sBool ok = 0;
  if(Connected)
  {
    ok = 1;
    Message msg;
    msg.Magic = 0xdeadbeef;
    msg.Command = 0;
    msg.Para0 = p0;
    msg.Para1 = p1;

    sPrintF(L"client: send %08x %08x\n",msg.Para0,msg.Para1);

    ok = Socket.WriteAll(msg);
    if(ok) ok = Socket.ReadAll(msg);

    sPrintF(L"client: got %08x %08x (%d)\n",msg.Para0,msg.Para1,ok);
  }
  return ok;
}

/****************************************************************************/
