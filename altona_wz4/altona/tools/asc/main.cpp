/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "main.hpp"
#include "shadercomp/shadercomp.hpp"
#include "base/system.hpp"

/****************************************************************************/

sBool Level11=0;
sBool PcOnly=0;

void Compiler::Run(const sChar *fn)
{
//  sShaderCompilerInit();
  ASC = 0;
  Level11 = 0;

  sString<sMAXPATH> buffer;
//  sExtractPath(fn,Path,CurScanFile);

  buffer=fn;
  *sFindFileExtension(buffer)=0;
  Filename = buffer;
  
  Scan.Init();
  Scan.AddToken(L"{",'{');
  Scan.AddToken(L"}",'}');
  Scan.AddToken(L"[",'[');
  Scan.AddToken(L"]",']');
  Scan.AddToken(L"(",'(');
  Scan.AddToken(L")",')');
  Scan.AddToken(L".",'.');
  Scan.AddToken(L",",',');
  Scan.AddToken(L":",':');
  Scan.AddToken(L";",';');
  Scan.AddToken(L"*",'*');
  Scan.AddToken(L"=",'=');
  Scan.StartFile(fn);

  NoStrip = sGetShellSwitch(L"-nostrip");

  ASC = new ACDoc;

  _Global();

  // process all new shaders in new materials
  NewMaterial *nm;
  sFORALL(NewMtrls,nm)
  {
    for(sInt i=0;i<sCOUNTOF(nm->Shaders);i++)
      if(nm->Shaders[i]) Process(nm->Shaders[i]);
  }
  sFORALL(ComputeShaders,nm)
  {
    for(sInt i=0;i<sCOUNTOF(nm->Shaders);i++)
      if(nm->Shaders[i]) Process(nm->Shaders[i]);
  }

  if(Scan.Errors==0)
  {
    OutputHPP();
    OutputCPP();
  }

  if(Scan.Errors==0)
  {
    buffer = Filename;
    buffer.Add(L"cpp");
    if(!sSaveTextAnsi(buffer,CPP.Get()))
    {
      sPrintF(L"failed writing %q\n",buffer);
      sSetErrorCode();
    }

    buffer = Filename;
    buffer.Add(L"hpp");
    if(!sSaveTextAnsi(buffer,HPP.Get()))
    {
      sPrintF(L"failed writing %q\n",buffer);
      sSetErrorCode();
    }
  }
  else
  {
    sSetErrorCode();
  }
  
  sDeleteAll(NewShaders);
  sDeleteAll(NewMtrls);
  sDeleteAll(ComputeShaders);

  sDelete(ASC);

//  sShaderCompilerExit();
}

/****************************************************************************/

void sMain()
{
  const sChar *name = sGetShellParameter(0,0);
  if(name)
  {
    if (!sGetShellSwitch(L"q"))
      sPrintF(L"Altona Shader Compiler V%d.%d\n",VERSION,REVISION);

    Compiler c; 
    c.Run(name);
  }
  else
  {
    sPrintF(L"\nAltona Shader Compiler V%d.%d\n\n",VERSION,REVISION);
    sPrintF(L"usage: asc file.asc [switches]\n");
    sPrintF(L"outputs file.cpp and file.hpp\n\n");
    sPrintF(L"switches:\n");
    sPrintF(L"-q            quiet operation (if successful)\n");
    sPrintF(L"-d            include debug info\n");
    sPrintF(L"-n            turn off shader optimization\n");
    sPrintF(L"              (only use this in case of emergency)\n");
    sPrintF(L"--nostrip     don't strip comments from generated code\n");  
    sPrintF(L"--noauto      turn off automatic shader constants\n");
//    sPrintF(L"--oldengine   generate output for the old engine at 49games\n");
    sPrintF(L"--printasc2dx print asc compiler output\n");
    sPrintF(L"--d3dold      use d3dx9_31.dll compiler\n");
    sPrintF(L"--gen_cg      generate OGL Cg shaders (disabled by default on Windows)");
    sPrintF(L"\n");
  }
}

