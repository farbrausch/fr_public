/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_SCREENS4_NETWORK_HPP
#define FILE_SCREENS4_NETWORK_HPP

#include "base/types.hpp"
#include "network/sockets.hpp"
#include "network/http.hpp"
#include "base/system.hpp"
#include "pugixml.hpp"
#include "playlists.hpp"

/****************************************************************************/

class RPCServer
{
  sTCPHostSocket Socket;
  sThread *Thread;
  sInt StartTime;

  PlaylistMgr &PlMgr;

  static void ThreadProxy(sThread *t, void *obj)
  {
    ((RPCServer*)obj)->ThreadFunc(t);
  }

  void ThreadFunc(sThread *t);

  class Buffer
  {
  public:
    static const int SIZE = 16384;
    sU8 Data[SIZE];
    sInt Count;
    sDNode Node;

    Buffer() { Count = 0; }
  };

  class Connection : public pugi::xml_writer
  {
  public:
    sTCPSocket *Socket;
    sDNode Node;
    
    sDList<Buffer, &Buffer::Node> InBuffers, OutBuffers;

    Connection()
    {
      Socket = 0;
    }

    ~Connection()
    {
      if (Socket) Socket->Disconnect();
      sDeleteAll(InBuffers);
      sDeleteAll(OutBuffers);
    }

    // pugi::xml_writer impl
		void write(const void* data, size_t size);
    
  };

  sDList<Connection,&Connection::Node> Connections;

  void OnMessage(Connection *conn, Buffer *lastbuf, const sU8 *lastptr);

  sBool GetPlaylists(Connection *conn, pugi::xml_node &in, pugi::xml_node &out);
  sBool SetPlaylist(pugi::xml_node &in);
  sBool Seek(pugi::xml_node &in);
  sBool Next(pugi::xml_node &in);
  sBool Previous(pugi::xml_node &in);
  sBool GetSystemInfo(pugi::xml_node &in, pugi::xml_node &out);

public:
  RPCServer(PlaylistMgr &plMgr, sInt port);
  ~RPCServer();

};

class WebServer
{

  sHTTPServer Httpd;
  sThread *Thread;

  static void ThreadProxy(sThread *t, void *obj)
  {
    ((sHTTPServer*)obj)->Run(t);
  }

public:

  WebServer(PlaylistMgr &plMgr, sInt port);
  ~WebServer();

};

/****************************************************************************/

#endif // FILE_SCREENS4_NETWORK_HPP

