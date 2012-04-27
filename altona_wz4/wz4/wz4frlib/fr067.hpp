/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_FR067_HPP
#define FILE_WZ4FRLIB_FR067_HPP

#include "base/types.hpp"
#include "wz4frlib/wz4_demo2.hpp"
#include "wz4frlib/wz4_demo2_ops.hpp"
#include "wz4frlib/fr067_ops.hpp"
#include "extra/mcubes.hpp"

/****************************************************************************/

class RNFR067_IsoSplash : public Wz4RenderNode
{
  friend static void MarchIsoTS(sStsManager *,sStsThread *thread,sInt start,sInt count,void *data);

  enum IsoEnum
  {
    CellSize = 8,
  };
  struct IsoNode
  {
    sVector31 Min,Max;
    sInt px,py,pz;
  };
  sArray<IsoNode> Nodes;

  sInt MaxThread;
  MarchingCubesHelper MC;

  // parameters

  sBool SphereEnable;
  sVector30 SphereAmp;

  sBool CubeEnable;
  sVector30 CubeAmp;

  sBool NoiseEnable;
  sVector30 NoiseFreq1;
  sVector30 NoisePhase1;
  sF32 NoiseAmp1;
  sVector30 NoiseFreq2;
  sVector30 NoisePhase2;
  sF32 NoiseAmp2;

  sBool RotEnable;
  sBool RubberEnable;
  sBool PolarEnable;

  sF32 func(const sVector31 &pp,sInt x,sInt y,sInt z);

  sInt Size;
  sF32 *NoiseXY;
  sF32 *NoiseYZ;
  sF32 *NoiseZX;
  sF32 *PolarPhi;
  sMatrix34 *RubberMat;
public:
  RNFR067_IsoSplash();
  ~RNFR067_IsoSplash();
  void Init();
  void MakeNodes();
  void March();
  void MarchTask(sStsThread *thread,sInt start,sInt count);

  void Simulate(Wz4RenderContext *ctx);   // execute the script. 
  void Prepare(Wz4RenderContext *ctx);    // do simulation
  void Render(Wz4RenderContext *ctx);     // render a pass

  Wz4RenderParaFR067_IsoSplash Para,ParaBase; // animated parameters from op
  Wz4RenderAnimFR067_IsoSplash Anim;          // information for the script engine

  Wz4Mtrl *Mtrl[4];
};


/****************************************************************************/

#endif // FILE_WZ4FRLIB_FR067_HPP

