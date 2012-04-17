// This code is in the public domain. See LICENSE for details.

/** \file types.h
 * Definiert diverse Standard-Datentypen.
 */

#pragma once

typedef int					      sInt;                 ///< Integer.
typedef unsigned int      sUInt;                ///< Unsigned integer.
typedef sInt				      sBool;                ///< Boolean.
typedef	char			      	sChar;                ///< Character.
typedef unsigned char			sUChar;								///< Unsigned character.

typedef signed   char     sS8;                  ///< Vorzeichenbehafteter 8bit-Integer.
typedef signed   short    sS16;                 ///< Vorzeichenbehafteter 16bit-Integer.
typedef signed   long     sS32;                 ///< Vorzeichenbehafteter 32bit-Integer.
typedef signed   __int64  sS64;                 ///< Vorzeichenbehafteter 64bit-Integer.

typedef unsigned char     sU8;                  ///< Vorzeichenloser 8bit-Integer.
typedef unsigned short    sU16;                 ///< Vorzeichenloser 16bit-Integer.
typedef unsigned long     sU32;                 ///< Vorzeichenloser 32it-Integer.
typedef unsigned __int64  sU64;                 ///< Vorzeichenloser 64bit-Integer.

typedef float             sF32;                 ///< 32bit-Floating point.
typedef double            sF64;                 ///< 64bit-Floating point.

#define sTRUE             1                     ///< wahr
#define sFALSE            0                     ///< falsch

#define FR_NOVTABLE       __declspec(novtable)  ///< Klasse hat keine VMT.

#pragma warning (disable: 4018 4244 4996)
