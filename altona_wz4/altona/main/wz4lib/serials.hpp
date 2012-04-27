/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) by Dierk Ohlerich                                    ***/
/***   all rights reserved                                                ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4LIB_SERIALS_HPP
#define FILE_WZ4LIB_SERIALS_HPP

#include "base/types.hpp"
#include "base/serialize.hpp"

/****************************************************************************/

namespace sSerId
{
  enum Werkkzeug4_
  {
    // old 
    Werkkzeug4Doc         = Chaos+0x0007,
    Werkkzeug4Op          = Chaos+0x0008,
    // new
    Wz4ChaosMesh          = Werkkzeug4+0x0010,
    Wz4Material           = Werkkzeug4+0x0011,
    Wz4Skeleton           = Werkkzeug4+0x0012,
    Wz4ChannelPerFrame    = Werkkzeug4+0x0013,
    Wz4AnimKey            = Werkkzeug4+0x0014,
    DetunedEngine         = Werkkzeug4+0x0015,
    DetunedIpp            = Werkkzeug4+0x0016,
    DetunedAnim           = Werkkzeug4+0x0017,
    DetunedAnim2          = Werkkzeug4+0x0018,
    wAnimFuncCurve        = Werkkzeug4+0x0019,
    wAnimRandomCurve      = Werkkzeug4+0x001a,
    Wz4ChannelSpline      = Werkkzeug4+0x001b,
    Wz4BSpline            = Werkkzeug4+0x001c,
    Wz4FuncTimeline       = Werkkzeug4+0x001d,
    Wz4FuncClip           = Werkkzeug4+0x001e,
    Wz4FuncSin            = Werkkzeug4+0x001f,

    Wz4FuncHermite        = Werkkzeug4+0x0020,
    Wz4FuncRandom         = Werkkzeug4+0x0021,
    Wz4WriteTimeline      = Werkkzeug4+0x0022,
    Wz4ChaosFont          = Werkkzeug4+0x0023,
    DetunedSpline         = Werkkzeug4+0x0024,
    DetunedClip           = Werkkzeug4+0x0025,
    DetunedSkeleton2      = Werkkzeug4+0x0026,
    Werkkzeug4DocInclude  = Werkkzeug4+0x0027,
    wTreeOp               = Werkkzeug4+0x0028,
    wStackOp              = Werkkzeug4+0x0029,
    wPage                 = Werkkzeug4+0x002a,
    Wz4BitmapAtlas        = Werkkzeug4+0x002b,

    Wz4Mesh               = Werkkzeug4+0x002c,
    Wz4Texture2D          = Werkkzeug4+0x002d,
    Wz4SimpleMtrl         = Werkkzeug4+0x002e,

// these numbers were allocated badly

    Wz4EditOptions        = 0x0002001e,
    Wz4DocOptions         = 0x0002001f,


  };
};

/****************************************************************************/

#endif // FILE_WZ4LIB_SERIALS_HPP

