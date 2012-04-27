/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#define sPEDANTIC_WARN 1

#include "sockets.hpp"

/****************************************************************************/

sFormatStringBuffer& operator%( sFormatStringBuffer &f, const sIPAddress &ip )
{
  if (!*f.Format)
    return f;

  sFormatStringInfo info;
  f.GetInfo(info);
  if(info.Format == 'x' || info.Format == 'X')
  {
    info.Null = 1;
    for (sInt i=0; i<4; i++)
      f.PrintInt<sU8>(info,ip.Addr[i],sFALSE);
  }
  else
  {
    info.Null = 0;
    f.PrintInt<sU8>(info,ip.Addr[0],sFALSE);
    f.Print(L".");
    f.PrintInt<sU8>(info,ip.Addr[1],sFALSE);
    f.Print(L".");
    f.PrintInt<sU8>(info,ip.Addr[2],sFALSE);
    f.Print(L".");
    f.PrintInt<sU8>(info,ip.Addr[3],sFALSE);
  }

  f.Fill();
  return f;
}

/****************************************************************************/

sBool sScanIPAddress(const sChar *&str, sIPAddress &ip)
{
  sBool error=sFALSE;
  sInt a[4];
  const sChar *str2=str;

  if (!error) sScanInt(str2,a[0]);
  if (!error) if (*str2++!='.') error=1;
  if (!error) sScanInt(str2,a[1]);
  if (!error) if (*str2++!='.') error=1;
  if (!error) sScanInt(str2,a[2]);
  if (!error) if (*str2++!='.') error=1;
  if (!error) sScanInt(str2,a[3]);
  for (sInt i=0; i<4; i++) 
    if (a[i]<0 || a[i]>255) 
      error=1;

  if (error) 
    return sFALSE;

  for (sInt i=0; i<4; i++) 
    ip.Addr[i]=sU8(a[i]);

  str=str2;
  return sTRUE;
}

/****************************************************************************/

sBool sTCPSocket::ReadAll(void *buffer, sDInt bytes)
{
  sU8 *ptr=(sU8*)buffer;
  while (bytes>0)
  {
    sDInt read;
    if (!Read(ptr,bytes,read)) return sFALSE;
    if (!read)
    {
      TransferError |= 1;
      return sFALSE;
    }
    ptr+=read;
    bytes-=read;
  }
  return sTRUE;
}

// synchronous write. Always blocks until everything is written
sBool sTCPSocket::WriteAll(const void *buffer, sDInt bytes)
{
  const sU8 *ptr=(const sU8*)buffer;

  while (bytes>0)
  {
    sDInt written;
    if (!Write(ptr,bytes,written)) return sFALSE;
    ptr+=written;
    bytes-=written;
  }
  return sTRUE;
}

/****************************************************************************/

sBool sResolveNameAndPort(const sChar *hostname, sIPAddress &address, sIPPort &port)
{
  sString<1024> addr;
  sInt cpos=sFindFirstChar(hostname,':');
  if (cpos>=0)
  {
    sCopyString(addr,hostname,cpos+1);
    sInt p;
    const sChar *ptr=&hostname[cpos+1];
    if (!sScanInt(ptr,p) || p > sMAX_U16)
      return sFALSE;
    port=sIPPort(p);
  }
  else
    addr=hostname;

  const sChar *ptr=addr;
  if (!sScanIPAddress(ptr,address)) 
    if (!sResolveName(addr,address))
      return sFALSE;

  return sTRUE;
}

