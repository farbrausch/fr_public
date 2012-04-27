/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "fr062.hpp"
#include "fr062_ops.hpp"

/****************************************************************************/

RNModSlice::RNModSlice()
{
  Anim.Init(Wz4RenderType->Script);
}

RNModSlice::~RNModSlice()
{
}

void RNModSlice::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);

  SimulateChilds(ctx);

  // initialize lightenvironment

  ModLightEnv *env = ModMtrlType->LightEnv[Para.Index];

  env->Vector[0] = Para.Pos0;
  env->Vector[1] = Para.Pos1;
  env->Vector[2] = Para.Pos2;
  env->Vector[3] = Para.Pos3;
  env->Vector[0].w = Para.Size0;
  env->Vector[1].w = Para.Size1;
  env->Vector[2].w = Para.Size2;
  env->Vector[3].w = Para.Size3;
  env->Vector[4].x = Para.ClipPhase;
  env->Vector[4].y = Para.ClipSize;
  env->Vector[4].z = Para.ColPhase;
  env->Vector[4].w = Para.ColSize;
  env->Vector[5].x = Para.Amp;
  env->Vector[5].y = Para.Bias;
  env->Vector[5].z = Para.TexAmp;
  env->Color[0].InitColor(Para.Color);
}

void RNModSlice::Render(Wz4RenderContext *ctx)
{
}



/****************************************************************************/

