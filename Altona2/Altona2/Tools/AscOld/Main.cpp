/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "altona2/libs/base/base.hpp"
#include "altona2/tools/AscOld/main.hpp"
#include "altona2/tools/AscOld/asc.hpp"

const sInt AppVersion = 0;
const sInt AppRevision = 7;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void sMain()
{
  sCommandlineParser cmd;

  sPrintF("asc2 v%d.%d\n",AppVersion,AppRevision);

  sString<sMaxPath> infile,objfile,cppfile,platformstring,dumpfile,headerfile;
  sInt dump=0;
  sInt isobjfile=0;
  sInt iscppfile=0;
  sInt platform=0;

  cmd.AddHelp("?");
  cmd.AddFile("!i",infile,0,"input file (xxx.asc.txt)");
  cmd.AddFile("!h",headerfile,0,"header file (xxx.hpp)");
  cmd.AddFile("o",objfile,&isobjfile,"output object file (final_null_shell_win32/xxx.o)");
  cmd.AddFile("cpp",cppfile,&iscppfile,"output cpp file (xxx.cpp)");
  cmd.AddString("!p",platformstring,0,"platform: dx9 dx11 blank gl2 gles");
  cmd.AddFile("d",dumpfile,&dump,"dump debug info into file");

  if(!cmd.Parse())
  {
    sSetExitCode(1);
    return;
  }
  if(isobjfile+iscppfile!=1)
  {
    sPrint("you must specify either the -cpp option or the -o options,\nnot both or none\n");
    sSetExitCode(1);
    return;
  }

  sBool ok = 1;

  platform = 0;
  if(platformstring=="dx9")
    platform = sConfigRenderDX9;
  if(platformstring=="dx11")
    platform = sConfigRenderDX11;
  if(platformstring=="gl2")
    platform = sConfigRenderGL2;
  if(platformstring=="gles2")
    platform = sConfigRenderGLES2;
  if(platformstring=="null")
    platform = sConfigRenderNull;
  if(platform==0)
  {
    sPrintF("unknown platform %s\n",platformstring);
    sSetExitCode(1);
    return;
  }

  AltonaShaderLanguage::Document *doc = new AltonaShaderLanguage::Document;

  if(!doc->Parse(infile,platform,dump))
    ok = 0;

  if(ok)
  {
    sSaveTextAnsi(headerfile,doc->GetHpp(),1);

    if(isobjfile)
    {
      // save output

      sString<sMaxPath> asmfile;
      asmfile.PrintF("%s.asm",objfile);
      sSaveTextAnsi(asmfile,doc->GetAsm(),0);

      sInt plat = sConfigPlatform;
      sBool bit64 = sConfig64Bit;
      
      if(sConfigRender==sConfigRenderGLES2)
      {
        plat = sConfigPlatformIOS;
        bit64 = 0;
      }

      if(!sAssemble(asmfile,objfile,plat,bit64))
      {
        sPrint("yasm failed\n");
        ok = 0;
      }
    }

    if(iscppfile)
    {
      sSaveTextAnsi(cppfile,doc->GetCpp(),0);
    }
  }

  if(dump)
    sSaveTextAnsi(dumpfile,doc->GetDump(),1);

  delete doc;
  if(ok)
  {
    sPrint("ok\n");
  }
  else
  {
    sPrint("failed\n");
    sSetExitCode(1);
  }
}

/****************************************************************************/

