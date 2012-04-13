// This file is distributed under a BSD license. See LICENSE.txt for details.



/****************************************************************************/
/***                                                                      ***/
/***   System Configuration                                               ***/
/***                                                                      ***/
/****************************************************************************/

// Compile Options

#define sNEWCONFIG        1
#define sINTRO            0                         // compile for small size
#define sPLAYER           0                         // this is a player, not the tool.
#define sPROFILE          1                         // include profiling code
#define sUNICODE          0                         // define sCHAR as 16bit
#define sMOBILE           0                         // mobile devices - very limited types.hpp
#define sDEBUG            1                         // include debug code in release build
#define sLIBPATH          "../"                     // libs (ogg,...) relative to project path

// what is supported?

#define sUSE_DIRECTINPUT  1                         // use DirectInput (window messages otherwise)
#define sUSE_DIRECTSOUND  1                         // use DirectSound (the alternative is silence!)
#define sUSE_MULTISCREEN  0                         // include multiscreen support
#define sUSE_SHADERS      1                         // include support for vertex/pixel shaders.
#define sUSE_LEKKTOR      0                         // include lekktor runtime support

// linkage

#define sLINK_SYSTEM      1                         // link system dependent code
#define sLINK_GUI         1                         // link gui classes
#define sLINK_ENGINE      1                         // link high level engine
#define sLINK_DISKITEM    1                         // link the abstract file system
#define sLINK_XSI         1                         // link softimage dotXSI loader
#define sLINK_UTIL        1                         // link sPainter and sPerfMon
#define sLINK_MP3         1                         // link mp3 player (patent issues!)
#define sLINK_OGG         1                         // link ogg vorbis decoder
#define sLINK_VIRUZ2      1                         // link viruz2 softsynth (assemblercode is always linked!)
#define sLINK_KKRIEGER    1                         // link special code for kkrieger
#define sLINK_MINMESH     1                         // link minmesh
#define sLINK_LOADER      1                         // link new model loader (XSI2)
#define sLINK_INTMATH     1                         // link new integer math model (for mobile)
#define sLINK_PNG         1                         // link libpng (big, but supports images with alpha)
#define sLINK_FRIED       1                         // link FRIED
#define sLINK_MTRL20      1                         // link material 20

/****************************************************************************/
/****************************************************************************/
