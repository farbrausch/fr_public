// This file is distributed under a BSD license. See LICENSE.txt for details.


/****************************************************************************/
/***                                                                      ***/
/***   System Configuration                                               ***/
/***                                                                      ***/
/****************************************************************************/

// Compile Options

#define sINTRO                  0 // compile for small size
#define sDEBUG                  1 // include debug code even in release build
// #define sSETROOTDIR "c:/nxn"   // auto-change directory at startup
#define sMAGIC      0x214f4344    // magic word for system files
#define sAPPNAME    "GenThree"    // application name for window title
#define sVERSION    "0.42"        // version number string
#define sCODECOVERMODE          0 // 0=disable 1=analyze 2=optimize
#define sSCREENX     800           // screen size for compiling intro
#define sSCREENY     600           // screen size for compiling intro

// what is supported?

#define sUSE_DIRECT3D           1 // use DirectX (at least one of GL/D3D would be a good idea!!)
#define sUSE_OPENGL             1 // use OpenGL
#define sUSE_DIRECTINPUT        1 // use DirectInput (use only messages otherwise)
#define sUSE_DIRECTSOUND        1 // use DirectSound (the alternative is silence!)
#define sUSE_MULTISCREEN        1 // include multiscreen support
#define sUSE_SHADERS            0 // use vertex and pixelshaders

// linkage

#define sLINK_SYSTEM            1 // link system dependent code
#define sLINK_GUI               1 // link gui classes
#define sLINK_ENGINE            1 // link high level engine
#define sLINK_DISKITEM          1 // link the abstract file system
#define sLINK_XSI               1 // link softimage dotXSI loader
#define sLINK_UTIL              1 // link sPainter and sPerfMon
#define sLINK_MP3               1 // link beiserts mp3-player (patent issues! only for experimental builds!)
#define sLINK_OGG               0 // link original ogg-vorbis support
#define sLINK_VIRUZ2            1 // link tammos viruz2 softsynth assemblercode is always linked!


// application specific

#define sPLAYER                 0 // genthree player            
#define sINTRO_X                0
#define sINTRO_Y                0

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#ifdef _PLAYER

// reconfigure

#undef sINTRO
#undef sINTRO_X
#undef sINTRO_Y
#undef sUSE_OPENGL
#undef sUSE_DIRECT3D
#undef sUSE_DIRECTINPUT
#undef sUSE_MULTISCREEN
#undef sLINK_GUI
#undef sLINK_UTIL
#undef sLINK_DISKITEM
#undef sLINK_XSI
#undef sLINK_MP3
#undef sPLAYER
#undef sDEBUG

#define sINTRO									1
#define sINTRO_X                1
#define sINTRO_Y                0
#define sPLAYER                 1
#define sDEBUG									0

#define sUSE_OPENGL             0
#define sUSE_DIRECT3D           0 // intro uses it's own start.cpp
#define sUSE_DIRECTINPUT        0
#define sUSE_MULTISCREEN        0
#define sLINK_GUI               0
#define sLINK_UTIL              0
#define sLINK_DISKITEM          0
#define sLINK_XSI               0
#define sLINK_MP3								0

#endif
