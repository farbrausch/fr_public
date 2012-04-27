/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "base/types.hpp"
#include "base/system.hpp"
#include "util/fastcompress.hpp"

/****************************************************************************/

static sBool CompareFiles(const sChar *name1,const sChar *name2)
{
  sFile *f1 = sCreateFile(name1,sFA_READ);
  sFile *f2 = sCreateFile(name2,sFA_READ);
  if(!f1 || !f2)
  {
    delete f1;
    delete f2;
    sFatal(L"Error opening files \"%s\", \"%s\"!\n",name1,name2);
  }

  if(f1->GetSize() != f2->GetSize())
  {
    delete f1;
    delete f2;
    sPrintF(L"Sizes of \"%s\" and \"%s\" don't match!\n",name1,name2);
    return sFALSE;
  }

  sU8 buf1[4096],buf2[4096];
  sS64 size = f1->GetSize();
  while(size > 0)
  {
    sInt bsize = sMin<sS64>(4096,size);
    if(!f1->Read(buf1,bsize) || !f2->Read(buf2,bsize))
    {
      delete f1;
      delete f2;
      sPrintF(L"Error reading while comparing \"%s\" and \"%s\"!\n",name1,name2);
      return sFALSE;
    }

    if(sCmpMem(buf1,buf2,bsize))
    {
      delete f1;
      delete f2;
      sPrintF(L"\"%s\" and \"%s\" don't match!\n",name1,name2);
      return sFALSE;
    }

    size -= bsize;
  }

  delete f1;
  delete f2;
  return sTRUE;
}

static void TestDirectAPI()
{
  sFastLzpCompressor compress;
  sFastLzpDecompressor decompress;
  sInt start;

  // compress
  //sFile *in = sCreateFile(L"c:/nxn/SummerGames2009/arenas/ceremony/ceremony_lighting_03.pch.eng",sFA_READ);
  sFile *in = sCreateFile(L"c:/nxn/SummerGames2009/data/menu.tp",sFA_READ);
  //sFile *in = sCreateFile(L"test.dat",sFA_READ);
  sFile *out = sCreateFile(L"test.zap",sFA_WRITE);
  if(!in || !out)
  {
    sPrintF(L"Couldn't create input/output files for compression!\n");
    delete in;
    delete out;
    return;
  }

  start = sGetTime();
  if(!compress.Compress(out,in))
    sPrintF(L"Compression failed!\n");
  else
  {
    sInt duration = sGetTime() - start;
    sPrintF(L"Compression done, %d->%d bytes.\n",in->GetOffset(),out->GetOffset());
    sPrintF(L"%.2f seconds, %.2f MB/sec\n",duration/1000.0f,(1000.0f*in->GetOffset()) / (duration*1048576.0f));
  }

  delete in;
  delete out;

  // decompress
  in = sCreateFile(L"test.zap",sFA_READ);
  out = sCreateFile(L"test.out",sFA_WRITE);
  if(!in || !out)
  {
    sPrintF(L"Couldn't create input/output files for decompression!\n");
    delete in;
    delete out;
    return;
  }

  start = sGetTime();
  if(!decompress.Decompress(out,in))
    sPrintF(L"Decompression failed!\n");
  else
  {
    sInt duration = sGetTime() - start;
    sPrintF(L"Decompression done, %d->%d bytes.\n",in->GetOffset(),out->GetOffset());
    sPrintF(L"%.2f seconds, %.2f MB/sec\n",duration/1000.0f,(1000.0f*out->GetOffset()) / (duration*1048576.0f));
  }

  delete in;
  delete out;
}

static void TestLzpFile()
{
  // try out sFastLzpFile
  sFastLzpFile *lzp = new sFastLzpFile;
  
  // compress with sFastLzpFile
  sFile *in = sCreateFile(L"c:/nxn/SummerGames2009/data/menu.tp",sFA_READ);

  if(!lzp->Open(sCreateFile(L"test.zap",sFA_WRITE),sTRUE))
  {
    sPrintF(L"Couldn't open sFastLzpFile for writing!\n");
    delete in;
    delete lzp;
    return;
  }

  // just use copy to compress
  sPrintF(L"compressing...\n");
  lzp->CopyFrom(in);
  lzp->Close();
  sPrintF(L"done. again.\n");
  delete in;

  // decompress with sFastLzpFile
  sFile *out = sCreateFile(L"test.out",sFA_WRITE);
  if(!lzp->Open(sCreateFile(L"test.zap",sFA_READ),sFALSE))
  {
    sPrintF(L"Couldn't open sFastLzpFile for reading!\n");
    delete out;
    delete lzp;
    return;
  }

  // use copy to decompress
  sPrintF(L"decompressing...\n");
  out->CopyFrom(lzp);
  lzp->Close();
  sPrintF(L"done. AGAIN AND AGAIN.\n");
  delete out;

  // all done
  delete lzp;
}

static void TestLotsOfFilesR(const sChar *dir)
{
  sArray<sDirEntry> list;
  if(sLoadDir(list,dir))
  {
    for(sInt i=0;i<list.GetCount();i++)
    {
      sString<1016> path = dir;
      path.AddPath(list[i].Name);

      if(list[i].Flags & sDEF_DIR)
        TestLotsOfFilesR(path);
      else
      {
        sString<1024> zapname = path;
        sString<1024> outname = path;
        zapname.Add(L".zap");
        outname.Add(L".out");

        // compress
        sPrintF(L"%s: compress... ",path);

        sFastLzpFile *lzp = new sFastLzpFile;
        sFile *in = sCreateFile(path,sFA_READ);
        if(!in)
          sFatal(L"Couldn't open file \"%s\".",path);

        if(!lzp->Open(sCreateFile(zapname,sFA_WRITE),sTRUE))
          sFatal(L"Couldn't open zapfile for \"%s\".",path);

        lzp->CopyFrom(in);
        lzp->Close();
        delete in;

        // decompress
        sPrint(L"decompress... ");
        sFile *out = sCreateFile(outname,sFA_WRITE);
        if(!out)
          sFatal(L"Couldn't open output file \"%s\".",outname);

        if(!lzp->Open(sCreateFile(zapname,sFA_READ),sFALSE))
          sFatal(L"Couldn't open zapfile \"%s\" for reading.\n",zapname);

        out->CopyFrom(lzp);
        lzp->Close();
        delete out;
        delete lzp;

        // verify
        sPrint(L"verify... ");
        if(!CompareFiles(path,outname))
          sExit();

        // clean up
        sPrint(L"done.\n");
        sDeleteFile(zapname);
        sDeleteFile(outname);
      }
    }
  }
  else
    sFatal(L"Error loading dir \"%s\"!\n",dir);
}

static void TestLotsOfFiles(const sChar *basedir)
{
  TestLotsOfFilesR(basedir);
}

void sMain()
{
  //TestDirectAPI();
  //TestLzpFile();
  TestLotsOfFiles(L"c:/nxn/SummerGames2009/");
}

/****************************************************************************/

