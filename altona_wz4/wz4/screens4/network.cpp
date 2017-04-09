/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#define sPEDANTIC_WARN 1
#include "network.hpp"
#include "base/types2.hpp"
#include "playlists.hpp"

/****************************************************************************/

void RPCServer::Connection::write(const void* data, size_t size)
{
  const sU8 *dptr = (const sU8*) data;

  while (size>0)
  {
    Buffer *buf = OutBuffers.IsEmpty()?0:OutBuffers.GetTail();
    if (!buf || buf->Count == Buffer::SIZE)
    {
      buf = new Buffer;
      OutBuffers.AddTail(buf);
    }
    sInt todo = (sInt)sMin<sDInt>(size, Buffer::SIZE-buf->Count);
    sCopyMem(buf->Data+buf->Count, dptr, todo);
    buf->Count += todo;
    dptr += todo;
    size -= todo;
  }
}

/****************************************************************************/

RPCServer::RPCServer(PlaylistMgr &plMgr, sInt port) : PlMgr(plMgr)
{
  Socket.Listen((sIPPort)port);
  StartTime = sGetTime();
  Thread = new sThread(ThreadProxy,0,0,this);
}

RPCServer::~RPCServer()
{
  Thread->Terminate();
  sDelete(Thread);
}

void RPCServer::ThreadFunc(sThread *t)
{

  while (t->CheckTerminate())
  {
    sTCPSocket *reads[100], *writes[100], *newconn;
    sInt nReads = 0, nWrites = 0;

    Connection *conn;
    sFORALL_LIST(Connections, conn)
    {
      reads[nReads++] = conn->Socket;
      if (!conn->OutBuffers.IsEmpty())
        writes[nWrites++] = conn->Socket;
    }

    if (Socket.WaitForEvents(nReads,reads,nWrites,writes,&newconn,100))
    {
      if (newconn)
      {
        conn = new Connection;
        conn->Socket = newconn;
        Connections.AddTail(conn);
      }

      // handle incoming data
      for (int i=0; i<nReads; i++)
      {
        sFORALL_LIST(Connections,conn) if (conn->Socket == reads[i])
        {
          sDInt read;
          Buffer *buf = conn->InBuffers.IsEmpty()?0:conn->InBuffers.GetTail();
          if (!buf || buf->Count == Buffer::SIZE)
          {
            buf = new Buffer;
            conn->InBuffers.AddTail(buf);
          }
          sBool ret = conn->Socket->Read(buf->Data+buf->Count,Buffer::SIZE-buf->Count,read);

          // close connection?
          if (!ret || read==0)
          {
            Connections.Rem(conn);
            sDelete(conn);
            break;
          }
          else
          {
            // add to in buffer
            buf->Count+=(sInt)read;

            // seach for end marker
            const sU8 *patptr = (const sU8*)"\n\r\n\r";
            const sU8 *bufptr = buf->Data+buf->Count;
            for (;;)
            {
              if (*--bufptr != *patptr) break;
              if (!*++patptr) break;
              if (bufptr == buf->Data)
              {
                if (buf == conn->InBuffers.GetHead())
                  break;
                buf = conn->InBuffers.GetPrev(buf);
                bufptr = buf->Data+buf->Count;
              }
            }

            if (!*patptr)
            {
              OnMessage(conn, buf, bufptr);
              sDeleteAll(conn->InBuffers);
            }
          }
          break;
        }

      }

      // handle outgoing data
      for (int i=0; i<nWrites; i++)
      {
        sFORALL_LIST(Connections,conn) if (conn->Socket == writes[i])
        {
          Buffer *buf = conn->OutBuffers.GetHead();
          sDInt written = 0;
          sBool ret = conn->Socket->Write(buf->Data, buf->Count, written);

          // close connection?
          if (!ret)
          {
            Connections.Rem(conn);
            sDelete(conn);
            break;
          }
          else
          {
            if (written<buf->Count)
              sMoveMem(buf->Data, buf->Data+written, sInt(buf->Count-written));
            buf->Count-=(sInt)written;

            if (!buf->Count)
            {
              conn->OutBuffers.RemHead();
              delete buf;
            }

            // close connection after sending it all out
            if (conn->OutBuffers.IsEmpty())
            {
              Connections.Rem(conn);
              sDelete(conn);
            }
          }
          
          break;
        }
      }

    }
  }
  
}

using namespace pugi;

#define Error(c,s) { errNo = (c); errStr = (s); goto onError; }

void RPCServer::OnMessage(Connection *conn, Buffer *lastbuf, const sU8 *lastptr)
{
  int errNo = 0;
  sString<256> errStr = L"SUCCESS";

  xml_document doc, resultdoc;
  xml_node xml, outxml, rpc, para;
  const sChar *type = L"";
  const sChar *name = L"";

  // build resultdoc first.
  outxml = resultdoc.append_child(L"xml");
  rpc = outxml.append_child(L"rpc");
  rpc.append_attribute(L"type").set_value(L"result");

  // make buffer for whole xml and parse
  sInt len = 0;
  Buffer *buf;
  sFORALL_LIST(conn->InBuffers, buf)
    if (buf==lastbuf)
      len += sInt(lastptr-buf->Data);
    else
      len += buf->Count;

  sInt xmlpos = 0;
  sU8 *xmldata = new sU8[len+1];
  buf = conn->InBuffers.RemHead();
  while (xmlpos<len)
  {
    if (!buf->Count)
    {
      delete buf;
      buf = conn->InBuffers.RemHead();
    }
    int todo = sMin<int>(len-xmlpos,buf->Count);
    sCopyMem(xmldata+xmlpos,buf->Data,todo);
    xmlpos+=todo;
    if (todo<buf->Count) sMoveMem(buf->Data,buf->Data+todo,buf->Count-todo);
    buf->Count-=todo;
  }
  xmldata[xmlpos]=0;
  conn->InBuffers.AddHead(buf);

  if (!doc.load_buffer_inplace(xmldata,len))
    Error(1,L"FAILED: invalid xml");

  // get rpc node
  xml = doc.child(L"xml");
  if (!xml)
    Error(1,L"FAILED: no xml node");
  rpc = xml.child(L"rpc");
  if (!rpc)
    Error(1,L"FAILED: no rpc node");

  // get rpc request
  type = rpc.attribute(L"type").value();
  name = rpc.attribute(L"name").value();
  if (sCmpStringI(type,L"request"))
    Error(1,L"FAILED: bad rpc type");

  // dispatch rpc call
  sBool ret;
  if (!sCmpStringI(name,L"get_playlists"))
    ret=GetPlaylists(conn,xml,outxml);
  else if (!sCmpStringI(name,L"playlist"))
    ret=SetPlaylist(xml);
  else if (!sCmpStringI(name,L"seek"))
    ret=Seek(xml);
  else if (!sCmpStringI(name,L"next"))
    ret=Next(xml);
  else if (!sCmpStringI(name,L"previous"))
    ret=Previous(xml);
  else if (!sCmpStringI(name,L"get_system_info"))
    ret=GetSystemInfo(xml, outxml);
  else
    Error(1,L"FAILED: unknown rpc method");

  if (!ret)
    Error(2,L"FAILED: method call failed");

onError:

  outxml.child(L"rpc").append_attribute(L"name").set_value(name);
  para = outxml.prepend_child(L"parameter");
  para.append_attribute(L"name").set_value(L"error_string");
  para.text().set(errStr);
  para = outxml.prepend_child(L"parameter");
  para.append_attribute(L"name").set_value(L"error");
  sSPrintF(errStr, L"%d", errNo);
  para.text().set(errStr);

  resultdoc.save(*conn,L"",1,encoding_utf8);

  delete[] xmldata;
}

#undef Error

/****************************************************************************/

// GetPlaylists: show all stored play lists
sBool RPCServer::GetPlaylists(Connection *conn, xml_node &in, xml_node &out)
{
  sIPAddress addr;
  sString<32> addrstr;
  conn->Socket->GetLocalAddress(addr);
  sSPrintF(addrstr,L"%d",addr);

  xml_node para = out.prepend_child(L"parameter");
  para.append_attribute(L"name").set_value(L"ip_address");
  para.text().set(addrstr);

  xml_node data = out.append_child(L"data");

  sArray<Playlist*> lists;
  Playlist *pl;
  lists.HintSize(100);
  for (pl = PlMgr.GetBegin(); pl; pl = PlMgr.GetNext(pl))
  {
    pl->AddRef();
    lists.AddTail(pl);
  }

  sString<256> curPl;
  sString<256> curSlide;
  PlMgr.GetCurrentPos(curPl, curSlide);

  sFORALL(lists,pl)
  {
    xml_node plnode = data.append_child(L"playlist");
    plnode.append_child(L"name").text() = pl->ID;
    plnode.append_child(L"playing").text() = (curPl == (const sChar*)pl->ID);
    plnode.append_child(L"timestamp").text() = pl->Timestamp;
		plnode.append_child(L"callbacks").text() = (int)(pl->CallbacksOn && PlMgr.CallbacksOn);
    PlaylistItem *it;
    sFORALL(pl->Items,it)
    {
      xml_node item = plnode.append_child(L"slide");
      item.append_child(L"id").text() = it->ID;
    }
    if (pl->LastPlayedItem >=0 && pl->LastPlayedItem < pl->Items.GetCount())
    {
      plnode.append_child(L"current_slide").text() = pl->Items[pl->LastPlayedItem]->ID;
    }
    pl->Release();
  }

  data.append_child(L"item_current").text() = curSlide;

  return sTRUE;
}

// SetPlaylist: set new play list
sBool RPCServer::SetPlaylist(xml_node &in)
{
  Playlist *pl = new Playlist;
  pl->Loop = sTRUE;
  pl->Dirty = sTRUE;

  for (xml_node para = in.child(L"parameter"); para; para=para.next_sibling(L"parameter"))
  {
    const sChar *name = para.attribute(L"name").as_string();
    if (!sCmpStringI(name, L"name"))
      pl->ID = para.text().as_string();
    else if (!sCmpStringI(name, L"callback"))
      pl->CallbackUrl = para.text().as_string();
    else if (!sCmpStringI(name, L"timestamp"))
      pl->Timestamp = para.text().as_uint();
		else if (!sCmpStringI(name, L"loop"))
			pl->Loop = para.text().as_int();
		else if (!sCmpStringI(name, L"callbacks_active"))
			pl->CallbacksOn = para.text().as_int();
  }

  for (xml_node item = in.child(L"data").child(L"item"); item; item=item.next_sibling(L"item"))
  {
    PlaylistItem *pi = new PlaylistItem;
    pi->ID = item.attribute(L"name").as_string();
    pi->Type = item.attribute(L"type").as_string();
    pi->Path = item.child(L"path").text().as_string();
    pi->SlideType = item.child(L"slide_type").text().as_string();
    pi->Duration = item.child(L"duration").text().as_float();
    pi->TransitionId = item.child(L"transition").text().as_int();
    pi->TransitionDuration = item.child(L"transition").attribute(L"duration").as_int()/1000.0f;
    pi->ManualAdvance = item.child(L"manual_advance").text().as_int();
    pi->Mute = item.child(L"mute").text().as_int();
    pi->MidiNote = item.child(L"midi").text().as_int();
    pi->Cached = sCmpStringI(item.attribute(L"type").as_string(),L"web") ? sTRUE : sFALSE;
	pi->CallbackUrl = item.child(L"callback").text().as_string();
	pi->CallbackDelay = item.child(L"callback").attribute(L"delay").as_float();

    xml_node sieg = item.child(L"siegmeister");
    pi->BarColor = sieg.child(L"bar_color").text().as_string();
    pi->BarBlinkColor1 = sieg.child(L"bar_blink_color_1").text().as_string();
    pi->BarBlinkColor2 = sieg.child(L"bar_blink_color_2").text().as_string();
    pi->BarAlpha = sieg.child(L"bar_alpha").text().as_float();
    for (xml_node bar = sieg.child(L"bar"); bar; bar=bar.next_sibling(L"bar"))
    {
      sFRect rect;
      rect.x0 = bar.attribute(L"x1").as_float();
      rect.x1 = bar.attribute(L"x2").as_float();
      rect.y0 = bar.attribute(L"y1").as_float();
      rect.y1 = bar.attribute(L"y2").as_float();
      pi->BarPositions.AddTail(rect);
    }

    pl->Items.AddTail(pi);
  }

  PlMgr.AddPlaylist(pl);

  return sTRUE;
}

sBool RPCServer::Seek(pugi::xml_node &in)
{
  const sChar *plId = 0;
  const sChar *slideId = 0;
  sBool hard = false;
  for (xml_node para = in.child(L"parameter"); para; para=para.next_sibling(L"parameter"))
  {
    const sChar *name = para.attribute(L"name").as_string();
    if (!sCmpStringI(name, L"playlist"))
      plId = para.text().as_string();
    else if (!sCmpStringI(name, L"slide"))
      slideId = para.text().as_string();
    else if (!sCmpStringI(name, L"hard"))
      hard = para.text().as_int();
  }

  if (plId && slideId)
  {
    PlMgr.Seek(plId, slideId, hard);
    return sTRUE;
  }
  return sFALSE;
}

sBool RPCServer::Next(pugi::xml_node &in)
{
  sBool hard = sFALSE;
  for (xml_node para = in.child(L"parameter"); para; para=para.next_sibling(L"parameter"))
  {
    const sChar *name = para.attribute(L"name").as_string();
    if (!sCmpStringI(name, L"hard"))
      hard = para.text().as_int();
  }

  PlMgr.Next(hard, sFALSE);
  return sTRUE;
}

sBool RPCServer::Previous(pugi::xml_node &in)
{
  sBool hard = sFALSE;
  for (xml_node para = in.child(L"parameter"); para; para=para.next_sibling(L"parameter"))
  {
    const sChar *name = para.attribute(L"name").as_string();
    if (!sCmpStringI(name, L"hard"))
      hard = para.text().as_int();
  }
  PlMgr.Previous(hard);
  return sTRUE;
}

#define widen(x) L ## x
#define strg(x) widen(#x)

sBool RPCServer::GetSystemInfo(pugi::xml_node &in, pugi::xml_node &out)
{
  xml_node data = out.append_child(L"data");
  data.append_child(L"version").text().set(L"2.0");
  data.append_child(L"timestamp").text().set(strg(__DATE__) L" " strg(__TIME__));
  data.append_child(L"uptime").text().set((sGetTime()-StartTime)/1000.0f);
  return sTRUE;
}

/****************************************************************************/
/****************************************************************************/

struct PlayControlHandler : public sHTTPServer::SimpleHandler
{
  const sChar *MyUrl;

  void Link(const sChar *text, sInt set=-1, sInt value=0)
  {
    if (set>=0)
      PrintF(L"<a href=\"%s?set=%d&value=%d\">%s</a>",MyUrl,set,value,text);
    else
      PrintF(L"<a href=\"%s\">%s</a>",MyUrl,text);
  }

  sHTTPServer::HandlerResult WriteDocument(const sChar *URL)
  {
    MyUrl=URL;

    WriteHTMLHeader(L"Screens4 playlist control",0);

    Print(L"<h1>Screens4 playlist control</h1><hr>");

    Print(L"<button type=\"button\">|&lt;</button> ");
    Print(L"<button type=\"button\">&lt;</button> ");
    Print(L"<button type=\"button\">&gt;</button> ");
    Print(L"<button type=\"button\">&gt;|</button> ");

    WriteHTMLFooter();
    return sHTTPServer::HR_OK;
  }

  static PlaylistMgr *PlMgr;
  static Handler *Factory() { return new PlayControlHandler; }
};

PlaylistMgr *PlayControlHandler::PlMgr = 0;


struct ThumbnailHandler : public sHTTPServer::Handler
{
public:

  sHTTPServer::HandlerResult Init(sHTTPServer::Connection *conn)
  {
    return sHTTPServer::HR_OK;
  }

  sBool DataAvailable()
  {
    return sTRUE;
  }

  sInt GetData(sU8 *,sInt)
  {
    return 0;
  }

  static Handler *Factory() { return new ThumbnailHandler; }
};

WebServer::WebServer(PlaylistMgr &plMgr, sInt port)
{
  Httpd.Init(port);

  Thread = new sThread(ThreadProxy, -1, 0, &Httpd);

  PlayControlHandler::PlMgr = &plMgr;
  Httpd.AddHandler(L"/",PlayControlHandler::Factory);
  Httpd.AddHandler(L"/thumb/*",ThumbnailHandler::Factory);
}

WebServer::~WebServer()
{
  Thread->Terminate();
  sDelete(Thread);
}