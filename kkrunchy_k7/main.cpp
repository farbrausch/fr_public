// Written by Fabian "ryg" Giesen.
// I hereby place this code in the public domain.

#include "_types.hpp"
#include "_startconsole.hpp"
#include "exepacker.hpp"
#include "debuginfo.hpp"
#include "mapfile.hpp"
#include "pdbfile.hpp"

/****************************************************************************/

void packerCallback(sU32 srcPos,sU32 srcSize,sU32 dstPos)
{
  static sInt pos,startTime;
  sInt nPos,tdelta;

  if(!srcPos)
  {
    pos = 0;
    sSystem->PrintF(" - packing [");
    startTime = sSystem->GetTime();
  }
  else
  {
    for(nPos=srcPos*32/srcSize;pos<nPos;pos++)
      sSystem->PrintF("#");

    if(srcPos == srcSize)
    {
      tdelta = sSystem->GetTime() - startTime;
      sSystem->PrintF("] => %d bytes (in %d.%02ds)\n",dstPos,tdelta/1000,(tdelta/10)%100);
    }
  }
}

sBool loadDebugInfo(sChar *fileName,DebugInfo &info)
{
  MAPFileReader mapreader;
  PDBFileReader pdbreader;

  // go through supported file types in order of preference
  if(pdbreader.ReadDebugInfo(fileName,info)) // .pdb file
  {
    sSystem->PrintF(" - program database successfully loaded\n");
    return sTRUE;
  }
  else if(mapreader.ReadDebugInfo(fileName,info)) // .map file
  {
    sSystem->PrintF(" - .map file successfully loaded\n");
    return sTRUE;
  }
  else // no match
  {
    sSystem->PrintF(" - no symbol info present\n");
    return sFALSE;
  }
}

sInt sAppMain(sInt argc,sChar **argv)
{
  sU8 *in,*out,*mapf;
  sChar *p,*fileName,*outFileName,fileBuf[260],*warn;
  sInt i,packMode,inSize,outSize,refSize;
  sBool gotMap;
  EXEPacker pack;
  DebugInfo info;

  sSystem->PrintF(sAPPNAME " " sVERSION " >> radical exe packer (c) f. giesen 2003-2007\n\n");

  // parser command line
  fileName = 0;
  outFileName = 0;
  packMode = 0; // default to good
  refSize = 0;

  for(i=1;i<argc;i++)
  {
    if(!sCmpString(argv[i],"--good"))
      packMode = 0;
    else if(!sCmpString(argv[i],"--best"))
      packMode = 1;
    else if(!sCmpString(argv[i],"--new"))
      packMode = 2;
    else if(!sCmpString(argv[i],"--out") && i+1<argc)
      outFileName = argv[++i];
    else if(!sCmpString(argv[i],"--refsize") && i+1<argc)
    {
      p = argv[++i];
      refSize = sScanInt(p);
    }
    else
      fileName = argv[i];
  }

  if(!outFileName)
    outFileName = fileName;

  info.Init();

  if(!fileName)
  {
    sSystem->PrintF("Syntax: " sAPPNAME " <options> file.exe\n\n"
                    "Options:\n"
                    "--good           Use good packer frontend (default)\n"
                    "--best           Use best packer frontend (notably slower)\n"
                    "--new            Use new packer frontend (experimental)\n"
                    "--refsize <size> Reference size in kilobytes\n"
                    "--out <filename> Write output to <filename>\n");
    return 1;
  }
  else
  {
    in = sSystem->LoadFile(fileName,inSize);
    if(!in)
    {
      sSystem->PrintF("\a - ERROR: couldn't open input file!\n");
      return 1;
    }

    gotMap = loadDebugInfo(fileName,info);
    info.FinishedReading();

    sSystem->PrintF(" - preprocessing, filtering & reslicing\n");

    out = pack.Pack(in,outSize,gotMap ? &info : 0,packerCallback,0);
    if(out)
    {
      warn = pack.GetInfoMessages();
      while(*warn)
      {
        sSystem->PrintF(" - %s\n",warn);
        warn += sGetStringLen(warn)+1;
      }

      warn = pack.GetWarningMessages();
      while(*warn)
      {
        sSystem->PrintF(" - WARNING: %s\n",warn);
        warn += sGetStringLen(warn)+1;
      }

      sSystem->PrintF(" - writing output file %s\n",outFileName);
      if(!sSystem->SaveFile(outFileName,out,outSize))
      {
        sSystem->PrintF("\a - ERROR: couldn't write output file!\n");
        return 1;
      }
      sSystem->PrintF(" - packed executable %d -> %d bytes\n",inSize,outSize);
      delete[] out;

      if(gotMap)
      {
        sCopyString(fileBuf,fileName,260);
        for(i=sGetStringLen(fileBuf)-1;i>=0 && fileBuf[i]!='.';i--);
        if(i>0)
          sCopyString(fileBuf+i,".kkm",260-i);
        else
          sAppendString(fileBuf,".kkm",260);

        sSystem->PrintF(" - writing pack ratio analysis %s\n",fileBuf);

        mapf = (sU8 *) info.WriteReport();
        sSystem->SaveFile(fileBuf,mapf,sGetStringLen((sChar *) mapf));
        delete[] mapf;
      }

      if(refSize)
      {
        i = pack.GetActualSize() - refSize * 1024;
        sSystem->PrintF(" - delta to reference size: %c%d bytes\n",
          (i<0)?'-':'+',sAbs(i));
      }
    }
    else
      sSystem->PrintF("\a - ERROR: %s\n",pack.GetErrorMessage());

    delete[] in;
  }

  info.Exit();

  return 0;
}
