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

class RNBlockTransition : public Wz4RenderNode
{
  sMaterial *Mtrl;
  sGeometry *Geo;

  sF32 LastTime;
  sRandomKISS Rnd;
  
  struct Block
  {
    sInt Index;
    sInt SrcIndex;
    sU32 Color;
  };

  sArray<Block> Blocks;
  
public:
  RNBlockTransition();
  ~RNBlockTransition();

  Wz4RenderParaBlockTransition Para, ParaBase;
  Wz4RenderAnimBlockTransition Anim;

  void Init();
  void Simulate(Wz4RenderContext *);
  void Render(Wz4RenderContext *);

  Texture2D *Texture;
};

/****************************************************************************/

class RNGlitch2D : public Wz4RenderNode
{
  sMaterial *Mtrl;
  sGeometry *Geo;

  sF32 LastTime;
  sRandomKISS Rnd;

public:
  RNGlitch2D();
  ~RNGlitch2D();

  Wz4RenderParaGlitch2D Para, ParaBase;
  Wz4RenderAnimGlitch2D Anim;

  void Init();
  void Simulate(Wz4RenderContext *);
  void Render(Wz4RenderContext *);

  Texture2D *Texture;
};

/****************************************************************************/
#endif // FILE_WZ4FRLIB_KBFX_HPP

