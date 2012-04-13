// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_start.hpp"
#include "scan.hpp"
#include "parse.hpp"

sInt Indent;
sChar OutBuffer[1024*1024];
sChar *OutPtr;
sInt Mode;
sInt RealMode;
sInt Modify;
sU8 *ModArray;
sChar *LekktorName;

/****************************************************************************/
/****************************************************************************/

void Error(sChar *name)
{
  sPrintF("\n#x error %s!\n",name);
  *OutPtr++ = 0;
  if(OutPtr>OutBuffer+1000)
    sDPrint(OutPtr-1000);
  else
    sDPrint(OutBuffer);
  sDPrint("\n\n");
  sSystem->Abort("error");
}

void Out(sChar *text)
{
  sInt len;
  len = sGetStringLen(text);
  if(len+OutPtr > OutBuffer+sizeof(OutBuffer))
    Error("output too long");
  sCopyMem(OutPtr,text,len);
  OutPtr += len;
}

void OutF(sChar *format,...)
{
  static sChar buffer[2048];
  sFormatString(buffer,2048,format,&format);
  Out(buffer);
}

void NewLine(sInt line)
{
  sInt i;
  Out("\n");
  for(i=0;i<Indent+line;i++)
    Out("  ");
}

/****************************************************************************/

#define PATHSIZE 4096

sInt sAppMain(sInt argc,sChar **argv)
{
  sChar *mem;
  sInt len;
  sChar inname[PATHSIZE];
  sChar outname[PATHSIZE];
  sChar modname[PATHSIZE];
  const sChar *modename[3]={"test","pre ","post"};

// commandline

  Mode = -1;
  if(argc==5)
  {
    if(sCmpStringI(argv[3],"test")==0)
      Mode = 0;
    if(sCmpStringI(argv[3],"pre")==0)
      Mode = 1;
    if(sCmpStringI(argv[3],"post")==0)
      Mode = 2;
  }
  if(argc!=5 || Mode==-1)
  {
    sPrintF("lekktor - dead code eliminator - v0.01\n");
    sPrintF("usage: lekktor inputpath outputpath mode file_without_cpp\n");
    sPrintF("modes: test pre post\n");
    sPrintF("will automatically load a file called <typelist.txt>\n");
    return 1;
  }
  RealMode = Mode;

  len = sGetStringLen(argv[1]);
  sCopyString(inname,argv[1],PATHSIZE);
  if(len>=0 && argv[1][len-1]!='\\' && argv[1][len-1]!='/')
    sAppendString(inname,"\\",PATHSIZE);
  sAppendString(inname,argv[4],PATHSIZE);
  sAppendString(inname,".cpp",PATHSIZE);

  len = sGetStringLen(argv[2]);
  sCopyString(outname,argv[2],PATHSIZE);
  if(len>=0 && argv[2][len-1]!='\\' && argv[2][len-1]!='/')
    sAppendString(outname,"\\",PATHSIZE);
  sAppendString(outname,argv[4],PATHSIZE);
  sAppendString(outname,".cpp",PATHSIZE);

  len = sGetStringLen(argv[2]);
  sCopyString(modname,argv[2],PATHSIZE);
  if(len>=0 && argv[2][len-1]!='\\' && argv[2][len-1]!='/')
    sAppendString(modname,"\\",PATHSIZE);
  sAppendString(modname,argv[4],PATHSIZE);
  sAppendString(modname,".lek",PATHSIZE);

  sPrintF("lekktor %s %s\n",modename[Mode],argv[4]);

// typelist.txt

  mem = (sChar *)sSystem->LoadText("typelist.txt");
  if(!mem)
  {
    sPrintF("could not load <typelist.txt>\n");
    return 1;
  }
  LoadTypeList(mem);
  delete[] mem;

// processing

  OutPtr = OutBuffer;
  mem = (sChar *)sSystem->LoadText(inname);
  if(!mem)
  {
    sPrintF("could not load <%s>\n",inname);
    return 1;
  }
  StartScan(mem);

  Out("// proceesed by Lekktor\n");
  if(RealMode==1)
  {
    Out("#include \"_lekktor.hpp\"\n");
    OutF("sLekktor sLekktor_%s;",argv[4],argv[4]);
  }
  if(RealMode==2)
  {
    ModArray = sSystem->LoadFile(modname);
    if(ModArray==0)
    {
      sPrintF("could not load modify-array <%s>\n",modname);
      return 1;
    }
  }
  Modify = 0;
  LekktorName = argv[4];
  Parse();
  Out("\n// proceesed by Lekktor\n");
  delete[] mem;

// output

  if(!sSystem->SaveFile(outname,(sU8 *)OutBuffer,OutPtr-OutBuffer))
  {
    sPrintF("could not save <%s>\n",outname);
    return 1;
  }

// done

  return 0;
}

/****************************************************************************/
