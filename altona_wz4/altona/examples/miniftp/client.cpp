/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "client.hpp"
#include "base/system.hpp"
#include "network/miniftp.hpp"

/****************************************************************************/

static void CompareFiles(const sChar *fn1,const sChar *fn2)
{
  sChecksumMD5 check1,check2;
  sFileCalcMD5(fn1,check1);
  sFileCalcMD5(fn2,check2);

  if(check1 == check2)
    sPrintF(L"  \"%s\" <-> \"%s\" File transfer ok!\n",fn1,fn2);
  else
    sPrintF(L"  \"%s\" <-> \"%s\" Transferred file doesn't match original!\n",fn1,fn2);
}

void sMain()
{
  sMiniFTPClient client;

  if(client.Connect(L"localhost",12345))
  {
    sPrintF(L"FileExists(\"nope.txt\") says: %d\n",client.FileExists(L"nope.txt"));
    sPrintF(L"FileExists(\"test1.txt\") says: %d\n",client.FileExists(L"test1.txt"));

    // read test1.txt and print it
    sFile *temp = sCreateFile(L"temp.txt",sFA_WRITE);
    sBool result = client.GetFile(L"test1.txt",temp);
    sPrintF(L"GetFile(\"test1.txt\"): %d\n",result);
    delete temp;

    // check whether the retrieved file is okay
    CompareFiles(L"test1.txt",L"temp.txt");
    sDeleteFile(L"temp.txt");

    // upload temp1.txt as temp2.txt
    sFile *in = sCreateFile(L"test1.txt",sFA_READ);
    result = client.PutFile(L"test2.txt",in);
    sPrintF(L"PutFile(\"test2.txt\"): %d\n",result);
    delete in;
    
    // download it again
    temp = sCreateFile(L"temp.txt",sFA_WRITE);
    result = client.GetFile(L"test2.txt",temp);
    sPrintF(L"GetFile(\"test2.txt\"): %d\n",result);
    delete temp;

    // again, check for correctness
    CompareFiles(L"test1.txt",L"temp.txt");
    sDeleteFile(L"temp.txt");
    sDeleteFile(L"test2.txt");
  }
  else
    sPrintF(L"Couldn't connect to server! Is it running?\n");
}

/****************************************************************************/

