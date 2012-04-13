// This file is distributed under a BSD license. See LICENSE.txt for details.



/****************************************************************************/
/***                                                                      ***/
/***   System Configuration                                               ***/
/***                                                                      ***/
/****************************************************************************/

// Compile Options

#define sNEWCONFIG        1
#define sINTRO            1                         // compile for small size
#define sPLAYER           1                         // this is a player, not the tool.
#define sPROFILE          0                         // include profiling code
#define sUNICODE          0                         // define sCHAR as 16bit
#define sMOBILE           0                         // mobile devices - very limited types.hpp
#define sDEBUG            0                         // include debug code in release build
#define sMEMDEBUG         0                         // memory leak checking
#define sLIBPATH          "../"                     // libs (ogg,...) relative to project path

// what is supported?

#define sUSE_DIRECTINPUT  0                         // use DirectInput (window messages otherwise)
#define sUSE_DIRECTSOUND  0                         // use DirectSound (the alternative is silence!)
#define sUSE_MULTISCREEN  0                         // include multiscreen support
#define sUSE_SHADERS      0                         // include support for vertex/pixel shaders.
#define sUSE_LEKKTOR      0                         // include lekktor runtime support

// linkage

#define sLINK_SYSTEM      0                         // link system dependent code
#define sLINK_GUI         0                         // link gui classes
#define sLINK_ENGINE      0                         // link high level engine
#define sLINK_DISKITEM    0                         // link the abstract file system
#define sLINK_XSI         0                         // link softimage dotXSI loader
#define sLINK_UTIL        0                         // link sPainter and sPerfMon
#define sLINK_MP3         0                         // link mp3 player (patent issues!)
#define sLINK_OGG         0                         // link ogg vorbis decoder
#define sLINK_VIRUZ2      0                         // link viruz2 softsynth (assemblercode is always linked!)
#define sLINK_KKRIEGER    0                         // link special code for kkrieger
#define sLINK_MINMESH     0                         // link minmesh
#define sLINK_LOADER      0                         // link new model loader (XSI2)
#define sLINK_INTMATH     0                         // link new integer math model (for mobile)
#define sLINK_PNG         0                         // link libpng (big, but supports images with alpha)
#define sLINK_FRIED       0                         // link FRIED
#define sLINK_MTRL20      0                         // link material 20
#define sLINK_FATMESH     0                         // link fat old genmesh
#define sLINK_RYGDXT      1                         // link fast dxt encoder

/****************************************************************************/
/****************************************************************************/
