/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_SIMPLE_CLIENT_HPP
#define FILE_SIMPLE_CLIENT_HPP

#include "base/types2.hpp"
#include "server.hpp"

/****************************************************************************/

class Client
{
  sIPAddress ServerIP;
  sIPPort ServerPort;
  sTCPClientSocket Socket;
  sString<256> IpString;

  sBool Connected;
  void CheckConnect();
public:
  Client(sInt port,const sChar *ip);
  ~Client();

  sBool Test(sU32 p0,sU32 p1);
  sBool Update();
};

/****************************************************************************/

#endif

