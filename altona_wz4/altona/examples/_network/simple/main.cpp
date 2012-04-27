/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "client.hpp"
#include "server.hpp"

/****************************************************************************/

// start client and server on the same computer.
// send two messages and break it up.

void sMain()
{
  sGetMemHandler(sAMF_HEAP)->MakeThreadSafe();

  Server *server = new Server(6006);
  Client *client = new Client(6006,L"127.0.0.1");

  client->Test(0x12345678,0xabcdabcd);
  client->Test(12,21);

  delete client;
  delete server;
}


/****************************************************************************/

