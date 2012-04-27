/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_NETWORK_HTTPSERVER_HPP
#define FILE_NETWORK_HTTPSERVER_HPP

#include "base/types.hpp"
#include "base/system.hpp"
#include "util/algorithms.hpp"
#include "network/sockets.hpp"


class sFile;
class sThread;

/****************************************************************************/
/***                                                                      ***/
/*** Helpers                                                              ***/
/***                                                                      ***/
/**************************************************************************+*/

// gets HTML character from unicode character.
// supply the original char and a buffer (8 bytes are enough)
// return value is length of output in buffer. May be 0.
sInt sGetHTMLChar(sChar ch,sChar8 *outbuf);

// parses URL and extracts parameters

struct sURLParam
{
  sString<64> Name;
  sString<1024> ValS;
  sInt ValI;
  void *ValBin; // for binary blobs
  sInt SizeBin;
};

// URL encoding/decoding
void sDecodeURL(sChar *url); // in-place, result is never longer

// Base64 encoding/decoding
void sEncodeBase64(const void *buffer, sInt size, const sStringDesc &out, sBool urlmode=sFALSE, sInt linelen=0);
sInt sDecodeBase64(const sChar *str, void *buffer, sInt maxlen, sBool urlmode=sFALSE);

/****************************************************************************/
/***                                                                      ***/
/*** HTTP Client (GET method only so far)                                 ***/
/***                                                                      ***/
/**************************************************************************+*/

class sHTTPClient : public sTCPClientSocket
{
public:
  enum ConnMethod
  {
    CM_GET,
    CM_PUT, // not implemented!
    CM_POST, // not implemented!
  };

  enum ConnStatus
  {
    CS_NOTCONNECTED = 0,
    CS_INFO = 1,
    CS_OK = 2,
    CS_REDIRECT = 3,
    CS_CLIENTERROR = 4,
    CS_SERVERERROR = 5,
  };

  sHTTPClient();
  sHTTPClient(const sChar *url, ConnMethod method=CM_GET);

  sBool Connect(const sChar *url, ConnMethod method=CM_GET);

  ConnStatus GetStatus();
  ConnStatus GetStatus(sInt &realcode);

  sInt GetSize(); // returns size of unread data, -1 if unknown

  sBool Read(void *buffer, sDInt size, sDInt &read);

  // not implemented yet!
  //virtual sBool Write(const void *buffer, sDInt bytes, sDInt &written) { sVERIFYFALSE; return sFALSE; }

protected:

  ConnMethod Method;
  sInt RetCode;
  sChar8 Buffer[1024];
  sInt Fill;
  sDInt Size;   // CHAOS: das habe ich mal auf 64 bit gestellt...

  sBool Send8(const sChar *str);


};

/****************************************************************************/
/***                                                                      ***/
/*** HTTP Server                                                          ***/
/***                                                                      ***/
/**************************************************************************+*/

class sHTTPServer
{
public:

  /****************************************************************************/
  // public interface

  // constructor. if fileroot is NULL, serving of files on disk
  // is disabled. Else it specifies the root directory for the server
  sHTTPServer(); // please call Init() afterwards;
  sHTTPServer(sInt port, const sChar *fileroot);
  ~sHTTPServer();

  sBool Init(sInt port=8080, const sChar *fileroot=0);

  class Handler;
  typedef Handler *(*HandlerCreateFunc)();

  // newly-added stuff will have highest priority if multiple wildcards match
  void AddStaticPage(const sChar *wildcard, const void *ptr, sInt length);
  void AddHandler(const sChar *wildcard, HandlerCreateFunc factory);
  
  // serve. The thread specified will be checked for termination
  sBool Run(sThread *t);

  /****************************************************************************/
  // semi-public interface for handlers

  static const sInt MAXCONN=64;
  static const sInt REQLINE=1024;
  static const sInt MAXPOSTDATA=1024*1024;

  enum ConnState
  {
    CS_GETREQUEST,
    CS_SERVE,
    CS_DONE,
  };

  enum ConnMethod
  {
    // add others if applicable
    CM_UNKNOWN,
    CM_GET,
    CM_HEAD,
    CM_POST,    
  };

  // Warning:
  // This struct just holds pointers to buffers and does _not_ copy any buffer data.
  // So make sure your buffers live as long as this Instance...
  //     shamada
  struct DataPacket
  {
    DataPacket(const sChar *buffer, sInt buffersize);

    void Parse();

    const sChar *Buffer;
    sInt BufferSize;

    sString<128> ContentType;
    sString<128> ContentDisposition;
    sInt ContentLength;
    const sChar *Payload;

    sDNode Node;
  };

  struct Connection
  {
    sTCPSocket * Socket;
    ConnState State;

    sString<REQLINE> RequestLine;
    sString<REQLINE> URL;
    sString<1024> Error;
    //sString<MAXPOSTDATA> POSTData;
    sChar *POSTData;
    sInt  POSTDataSize;

    ConnMethod Method;

    sInt RetCode; // HTTP result code, or 0:unknown / 1:HTTP 0.9
    sBool HeadersDone;
    sInt ContentLength;
    sString<128> ContentType;

    sDList <DataPacket,&DataPacket::Node> DataPackets;

    Handler *Hndl;   
    sFile *File;  // for serving local files
    const sU8 *Mem;     // for serving binary chunks
    sInt MemSize; 

    sDNode Link;
  };

  enum HandlerResult
  {
    HR_OK,
    HR_IGNORE,
    HR_ERROR,
    HR_REWRITTEN,
  };



  // handler interface for serving documents based on the URL
  class Handler
  {
  public:
    virtual ~Handler() {}

    // initializes handler. 
    virtual HandlerResult Init(Connection *c)=0;

    // add additional header
    virtual void GetAdditionalHeaders(const sStringDesc &str) {}

    // checks if handler wants to output
    virtual sBool DataAvailable()=0;
    
    // get next chunk of data
    // returns length, 0 means end of document
    virtual sInt GetData(sU8 *buffer, sInt len)=0;
  };



  // simple handler class: buffers whole document to make it easier
  // to write stuff
  class SimpleHandler : public Handler
  {
  public:

    // overload this!
    virtual HandlerResult WriteDocument(const sChar *URL)=0;

    // and perhaps this (remember to call the super function!)
    virtual void GetAdditionalHeaders(const sStringDesc &str);

    // print string (will be converted to ASCII)
    void Print(const sChar *string);

  protected:

    // get URL Parameters
    sInt GetParamI(const sChar *name, sInt deflt=0);
    const sChar* GetParamS(const sChar *name, const sChar *deflt=L"");
    void * GetParamBin(const sChar *name,sInt &size);
    
    sInt GetParamCount() { return PCount; }
    const sChar *GetParamName(sInt n) { sVERIFY(n<PCount); return Params[n].Name; }

    // write a binary buffer
    void Write(void *ptr, sInt size);

    // print string (will be converted to ASCII)
    sPRINTING0(PrintF, sString<0x400> tmp; sFormatStringBuffer buf=sFormatStringBase(tmp,format);buf,Print((const sChar*)tmp););

    // for start/end of HTML files
    void WriteHTMLHeader(const sChar *title=0, const sChar *stylesheet=0, const sChar *addheader=0);
    void WriteHTMLFooter();

    SimpleHandler();
    ~SimpleHandler();

    // option to use a different (bigger) array for parameters
    void SetParamArray(sArrayRange<sURLParam> *newParamArray) { Params = newParamArray ? *newParamArray : sAll(DefaultParamArray); }

  private:
    HandlerResult Init(Connection *c);
    sBool DataAvailable();
    sInt GetData(sU8 *buffer, sInt len);   
    sArrayRange<sURLParam> Params;
    sURLParam DefaultParamArray[16];
    sInt PCount;
    
    static const sInt BUFSIZE=16384;

    struct Buffer
    {
      sU8 Mem[BUFSIZE];
      Buffer *Next;
    } *FirstB;

    Buffer *CurWriteB;
    sInt CurWriteFill;

    Buffer *CurReadB;
    sInt CurReadPos;

    sInt TotalLen;
    sBool IsHTML;
  };

  /****************************************************************************/
  // no need to look here

private:

  sU8 Temp[4096];

  struct URLEntry
  {
    sString<REQLINE> Wildcard;

    // either... 
    HandlerCreateFunc HFactory;

    // ... or 
    const sU8 *Mem;
    sInt Size;

    sDNode Link;
  };

  sDList<URLEntry,&URLEntry::Link> URLEntries;

  sTCPHostSocket HostSocket;

  sBool FilesSupported;
  sString<sMAXPATH> FileRoot;

  sDList<Connection,&Connection::Link> ConnList;
  Connection ConnStore[MAXCONN];
  sDList<Connection,&Connection::Link> FreeList;

  sThreadLock Lock;

  void ParseRequestLine(Connection *c);
  void StartServing(Connection *c);


};

sInt sParseURL(const sChar *url, const sStringDesc &base, sURLParam *params, sInt maxparams);
sInt sParsePOSTData(const sHTTPServer::Connection *c, sURLParam *params, sInt maxparams);

#endif

