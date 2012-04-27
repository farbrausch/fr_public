/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_UTIL_SCANCONFIG_HPP
#define FILE_UTIL_SCANCONFIG_HPP

#include "base/types2.hpp"

/****************************************************************************/

enum sScanConfigSDK
{
  sSDK_DX9      = 0x0001,
  sSDK_CG       = 0x0002,
  sSDK_CHAOS    = 0x0080,
  sSDK_XSI      = 0x0100,
  sSDK_DX11     = 0x0200,
  sSDK_FR       = 0x0800,
  sSDK_GECKO    = 0x1000,
};

enum sScanConfigSolution
{
  sSLN_WIN32    = 0x0001,
  sSLN_WIN64    = 0x0002,
};

enum sScanConfigMakefile
{
  sMAKE_NONE    = 0x0000,
  sMAKE_MINGW   = 0x0001,
  sMAKE_LINUX   = 0x0002,
};

enum sScanConfigExecutablePostfix
{
  sEXEPOSTFIX_NONE    = 0x0000,
  sEXEPOSTFIX_CONFIG  = 0x0001,
};

class sScanConfig
{
public:
  sScanConfig();
  void Scan(const sChar *coderoot=0);
  void DPrint();

  sInt SDK;
  sInt Solution;
  sInt Makefile;
  sInt ExecutablePostfix;

  sPoolString CodeRoot_System;
  sPoolString CodeRoot_Windows;
  sPoolString CodeRoot_Linux;
  sPoolString CodeRoot_Makefile;
  sPoolString DataRoot;
  sPoolString IntermediateRoot;
  sPoolString OutputRoot;
  sPoolString MakeprojectTemplate;
  sPoolString ConfigFile;
  sPoolString ToolBin;
  sPoolString VSVersion;            // like "2005sp1" or "2008"
};

sInt sParseSDK(const sChar *name);    // return sSDK_?? for string, or 0

/****************************************************************************/

#endif // FILE_UTIL_SCANCONFIG_HPP
