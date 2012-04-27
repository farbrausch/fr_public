/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_INTEL09LIB_ADF_HPP
#define FILE_INTEL09LIB_ADF_HPP

/****************************************************************************/

#include "wz4frlib/Wz4_mesh.hpp"
#include "tADF.hpp"
#include "adf_shader.hpp"

/****************************************************************************/

class Wz4ADF : public wObject
{       
  protected:
    tSDF *SDF;
    sTexture3D *Texture;

  public:

    Wz4ADF();
    virtual ~Wz4ADF();

    inline tSDF *GetObj()
    {
      return SDF;
    }

    inline void SetObj(tSDF *sdf)
    {
      SDF=sdf;
    }

    sTexture3D *GetTexture();

    //  
    sBool TraceRay(sVector31 &p, sVector30 &n, const sRay &ray, const sF32 md=0.005f, const sF32 mx=10000.0f, const sInt mi=512);

    //Generator
    Wz4BSPError FromMesh(Wz4Mesh *in, sF32 planeThickness, sInt Depth, sF32 GuardBand, sBool ForceCubeSampling, sBool UserBox, const sVector31 &BoxPos, const sVector30 &BoxDimH, sBool BruteForce);
    void FromFile(sChar *name);
};

/****************************************************************************/

void Wz4ADF_Init(int w, int h);
void Wz4ADF_Exit();
void Wz4ADF_Render(sImage *img, Wz4ADF  *adf, wPaintInfo &pi);

/****************************************************************************/

/*
class RNRenderADF : public Wz4RenderNode
{  
  tADFMat *Mtrl;
  tADFShadowMat *MtrlShadow;

public:
  tSDF *SDF;
  sTexture3D *texture;


  RNRenderADF();
  ~RNRenderADF();
  void Init();

  Wz4RenderParaRenderADF Para,ParaBase;
  Wz4RenderAnimRenderADF Anim;

  void Simulate(Wz4RenderContext *);
  void Render(Wz4RenderContext *);
};
*/

#endif // FILE_INTEL09LIB_ADF_HPP

