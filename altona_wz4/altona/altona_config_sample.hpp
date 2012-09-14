/****************************************************************************/
/***                                                                      ***/
/***   Altona main configuration file.                                    ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***   Please rename this file to "altona_config.hpp" and edit it to      ***/
/***   reflect your needs.                                                ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***   This file is compiled in every altona application                  ***/
/***   In addition to that, makeproject parses this file by hand on       ***/
/***   every invokation.                                                  ***/
/***                                                                      ***/
/****************************************************************************/

// default directories.
// used by various applications
// compiled in, need to recompile application when changed

#define sCONFIG_CODEROOT_WINDOWS  L"C:/source/altona"
#define sCONFIG_CODEROOT_LINUX    L"~/svn2"
#define sCONFIG_DATAROOT          L"C:/NxN"
#define sCONFIG_MP_TEMPLATES      L"altona/tools/makeproject/makeproject.txt"
#define sCONFIG_CONFIGFILE        L"altona/altona_config.hpp"
#define sCONFIG_TOOLBIN           L"C:/source/altona/altona/bin"
#define sCONFIG_VSVERSION         L"2010"    // visual studio version, like "2005sp1", "2008" , "2010" or "2012"

#define sCONFIG_MP_EXEPOSTFIX_CONFIG  0     // postfix configurationname to executable

#define sCONFIG_INTERMEDIATEROOT  L"" // example: "c:\temp\_vs\" leave blank ("") for default behaviour
#define sCONFIG_OUTPUTROOT        L"" // example: "c:\temp\_vs\" leave blank ("") for default behaviour

// installed SDK.
// MAKEPROJECT: scanned at program start, no need to recompile
// ASC: compiled into application and statically linked 

#define sCONFIG_SDK_CHAOS         0         // chaos toolchain

#define sCONFIG_SDK_DX9           1         // Microsoft DX9 sdk installed (required for input, sound, graphics)
#define sCONFIG_SDK_DX11          1         // Microsoft DX11 sdk installed
#define sCONFIG_SDK_CG            0         // NVidia CG SDK (required for ASC)
#define sCONFIG_SDK_XSI           0         // Softimage XSI SDK
#define sCONFIG_SDK_GECKO         0         // Mozilla Gecko (XULRunner) SDK

// makeproject: what project files should be created?
// scanned at program start, no need to recompile
// sCONFIG_MP_VS_???: all platforms will be integrated into one project file
// sCONFIG_MP_MAKE_???: choose only ---ONE--- platform to be put into the "Makefile"

#define sCONFIG_MP_VS_WIN32       1         // create VS project files for win32
#define sCONFIG_MP_VS_WIN64       0         // create VS project files for win64
#define sCONFIG_MP_MAKE_MINGW     0         // create makefiles for win32 mingw
#define sCONFIG_MP_MAKE_LINUX     0         // create makefiles for linux

/****************************************************************************/
