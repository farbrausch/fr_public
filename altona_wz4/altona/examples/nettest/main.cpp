/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "main.hpp"
#include "network/sockets.hpp"

/****************************************************************************/

void sMain()
{
  sDPrintF(L"\n\n");

  sString<512> hostname;
  sGetLocalHostName(hostname);
  sDPrintF(L"hostname: %s\n",hostname);
  
  /*
  sIPAddress pouet;
  sResolveName(L"www.pouet.net",pouet);
  */

  /*
  sIPAddress toaster2(127,0,0,1);

  sTCPClientSocket client;
  client.Connect(toaster2,1234);

  //sChar8 *request="GET /\r\n";
  //sInt rlen=7;
  //client.WriteAll(request,rlen);

  sInt read;
  do
  {
    sChar8 buffer[129];
    client.Read(buffer,128,read);
    buffer[read]=0;

    sString<130> str;
    sCopyString(str,buffer);
    //sDPrintF(L"read %3d: %s\n",read,str);
    sDPrintF(L"%s",str);

  } while (read);

  sDPrintF(L"\n\n");
  */

  sIPAddress localhost(127,0,0,1);

  sUDPSocket sock;
  sock.Init(sFALSE,1000);

  const sChar blah[]=L"wer das liest ist doof";
  sInt size=sizeof(blah);

  sock.Write(localhost,1000,blah,size);

  sock.Wait(sTRUE,sFALSE,2000);

  if (sock.CanRead())
  {
    sChar buffer[1024];
    sIPAddress ad;
    sIPPort port;
    sock.Read(buffer,sizeof(buffer),size,ad,port);
    sDPrintF(L"got %d bytes from %d.%d.%d.%d:%d - <%s>\n",size,ad.Addr[0],ad.Addr[1],ad.Addr[2],ad.Addr[3],port,buffer);
  }

  sock.Close();


};


/****************************************************************************/

