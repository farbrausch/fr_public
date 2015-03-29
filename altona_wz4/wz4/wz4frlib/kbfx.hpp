/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_KBFX_HPP
#define FILE_WZ4FRLIB_KBFX_HPP

#include "base/types.hpp"
#include "wz4frlib/kbfx_ops.hpp"

/****************************************************************************/

class RNVideoDistort : public Wz4RenderNode
{
    sMaterial *Mtrl;
    sGeometry *Geo;

    sF32 LastTime;
    sRandomKISS Rnd;

    struct Block
    {
        sInt Index;
        sU32 Color;
    };

    sArray<Block> Blocks;

    void AddGlitch(int gx, int gy);

public:
    RNVideoDistort();
    ~RNVideoDistort();

    Wz4RenderParaVideoDistort Para, ParaBase;
    Wz4RenderAnimVideoDistort Anim;

    void Simulate(Wz4RenderContext *);
    void Render(Wz4RenderContext *);

};


/****************************************************************************/

#endif // FILE_WZ4FRLIB_KBFX_HPP

