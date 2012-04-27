/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4TRONLIB_TRON_HPP
#define FILE_WZ4TRONLIB_TRON_HPP


#include "base/types.hpp"
#include "wz4frlib/wz4_demo2.hpp"
#include "wz4frlib/wz4_demo2_ops.hpp"
#include "wz4frlib/wz4_mesh.hpp"
#include "wz4frlib/wz4_mtrl2.hpp"
#include "wz4frlib/chaosfx_ops.hpp"
#include "util/shaders.hpp"
#include "wz4frlib/tron_ops.hpp"
#include "wz4frlib/tron_shader.hpp"
#include "wz4frlib/tADF.hpp"

#define COMPILE_FR033 1

/****************************************************************************/

class TronSFF : public Wz4ParticleNode
{
  sF32 ScaleX;
  sF32 ScaleY;
  sF32 ScaleZ; 
  sVector4 Plane;

public:
  TronSFF();
  ~TronSFF();
  void Init();

  Wz4ParticlesParaSingleForceField Para,ParaBase;
  Wz4ParticlesAnimSingleForceField Anim;
  Wz4ParticleNode *Source;

  void Simulate(Wz4RenderContext *ctx);
  sInt GetPartCount();
  sInt GetPartFlags();
  void Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt);
 
  inline sF32 GetEllipsoidDistance(sVector31 &p)
  {
    sVector30 dist = p - Para.Position;
    dist.x = dist.x * ScaleX;
    dist.y = dist.y * ScaleY;
    dist.z = dist.z * ScaleZ;
    return dist.Length();
  }

  inline sF32 GetPlaneDistance(sVector31 &p)
  {        
    return -(p^Plane / Para.Distance);
  }


  inline sF32 CalcDistractionFactor(sF32 f)
  {
    sF32 ret = 0;
    f = sMax(f,0.001f);
    
    switch (Para.ForceType)
    {
      //1/x^2
      case 3 :
        ret = Para.Scale / (1.0f + f*f);
      break;
      //1/x
      case 2 :
        ret = Para.Scale / (1.0f + f);
      break;
      //sin(x)
      case 1 :
        ret = Para.Scale * sin(f);
      break;
      //Linear
      case 0 :
        ret = Para.Scale - f;
        ret = sMax(ret,0.0f);
      break;
    }    

    return ret;
  }
};

/****************************************************************************/

class TronPOM : public Wz4ParticleNode
{
private:
  Wz4BSP bsp;
  sBool first;
public:
  TronPOM();
  ~TronPOM();
  Wz4BSPError Init(sF32 planeThickness);

  Wz4ParticlesParaPlaceOnMesh Para,ParaBase;
  Wz4ParticlesAnimPlaceOnMesh Anim;
  Wz4ParticleNode *Source;
  Wz4Mesh *Mesh;


  void Simulate(Wz4RenderContext *ctx);
  sInt GetPartCount();
  sInt GetPartFlags();
  void Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt);
  static void Handles(wPaintInfo &pi, Wz4ParticlesParaPlaceOnMesh *para, wOp *op);

};

/****************************************************************************/

#ifdef COMPILE_FR033

class FR033_MeteorShowerSim : public Wz4ParticleNode
{
  struct tMeteor
  {
    sF32      CollTime;
    sBool     Enable;
    sVector31 StartPos;
    Wz4Mesh   *Mesh;
    sVector31 Pos;
    sVector30 Speed;
  };
  
  public:
  FR033_MeteorShowerSim();
  ~FR033_MeteorShowerSim();

  void Init();

  void Simulate(Wz4RenderContext *ctx);
  sInt GetPartCount();
  sInt GetPartFlags();
  void Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt);

  Wz4ParticlesParaFR033_MeteorShowerSim   Para,ParaBase; // animated parameters from op
  Wz4ParticlesAnimFR033_MeteorShowerSim   Anim;          // information for the script engine  

  sF32 LastTime;
  sF32 Time;

  GroupType *GroupLogos;

  sArray<tMeteor>  Meteors;
};




class FR033_WaterSimRender : public Wz4RenderNode
{
  struct tWave
  {
    sF32     Time;
    sVector2 Pos;   
  };

  struct PartUVRect
  {
    sF32 u, v, du, dv;
  };

  struct PartVert0
  {
    sF32 u0,v0,angle;
    void Init(sF32 u,sF32 v,sF32 a) { u0=u; v0=v; angle=a; }
  };

  struct PartVert1
  {
    sF32 px,py,pz,rot;
    sF32 sx,sy,u1,v1;
    PartUVRect uvrect;
    sF32 fade;
    sU32 c0;
  };

  struct tDancer
  {
    sVector31 pos;
    sVector2  scale;
    sInt      atlas;
    sInt      vi;
  };

  public:
  FR033_WaterSimRender();
  ~FR033_WaterSimRender();

  void Init();

  void Simulate(Wz4RenderContext *ctx);   // execute the script. 
  void Prepare(Wz4RenderContext *ctx);    // do simulation
  void Render(Wz4RenderContext *ctx);     // render a pass

  void Simulate(sF32 time);

  void MakeGrid();

  Wz4RenderParaFR033_WaterSimRender   Para,ParaBase; // animated parameters from op
  Wz4RenderAnimFR033_WaterSimRender   Anim;          // information for the script engine

  sStaticArray<PartUVRect> UVRects;

  sMatrix34              Matrix;               // calculated in Prepare, used in Render

  sF32 LastTime;
  sF32 Time;

  Wz4Mesh                WaterMesh;
  sGeometry             *WaterGeo;
   

  sArray<sF32>           PosY;
  Wz4Mtrl               *WaterMtrl;      
  sArray<tWave>          Waves;
  sArray<tDancer>        Dancers;
  sVertexFormatHandle   *DancerFormat;  
  Texture2D             *Dancer;
  sMaterial             *DancerMtrl;
  sGeometry             *DancerGeo;
  Wz4Particles          *MeteorSim;
};

#endif


/****************************************************************************/

class RNSharpen : public Wz4RenderNode
{
  sMaterial *Mtrl;
public:
  RNSharpen();
  ~RNSharpen();

  Wz4RenderParaSharpen Para,ParaBase;
  Wz4RenderAnimSharpen Anim;

  void Simulate(Wz4RenderContext *);
  void Render(Wz4RenderContext *);
};


/****************************************************************************/

class RNBloom : public Wz4RenderNode
{  
  tBloomMaskMat   *MtrlMask;
  tBloomCompMat   *MtrlComp;  

public:
  RNBloom();
  ~RNBloom();
  void Init();

  Wz4RenderParaBloom Para,ParaBase;
  Wz4RenderAnimBloom Anim;

  void Simulate(Wz4RenderContext *);
  void Render(Wz4RenderContext *);
};

/****************************************************************************/

class RNPromist : public Wz4RenderNode
{  
  tBloomMaskMat   *MtrlMask;
  tPromistCompMat *MtrlComp;
  tCopyMat        *MtrlCopy;

public:
  RNPromist();
  ~RNPromist();
  void Init();

  Wz4RenderParaPromist Para,ParaBase;
  Wz4RenderAnimPromist Anim;

  void Simulate(Wz4RenderContext *);
  void Render(Wz4RenderContext *);
};

/****************************************************************************/

class RNSBlur : public Wz4RenderNode
{
  sGeometry *Geo;
  sMaterial *Mtrl_STL;
  sMaterial *Mtrl_LTS;
  
public:
  RNSBlur();
  ~RNSBlur();

  Wz4RenderParaSBlur Para,ParaBase;
  Wz4RenderAnimSBlur Anim;

  void Simulate(Wz4RenderContext *);
  void Render(Wz4RenderContext *);
};

/****************************************************************************/

class RNFocusBlur : public Wz4RenderNode
{
  sGeometry *Geo;
  sMaterial *Mtrl;  
  
public:
  RNFocusBlur();
  ~RNFocusBlur();

  Wz4RenderParaFocusBlur Para,ParaBase;
  Wz4RenderAnimFocusBlur Anim;

  void Simulate(Wz4RenderContext *);
  void Render(Wz4RenderContext *);
};



/****************************************************************************/


class RNDoF3 : public Wz4RenderNode
{  
  tDoF3MaskMat *MtrlMask;
  tDoF3CompMat *MtrlComp;
  tCopyMat     *MtrlCopy;

public:
  RNDoF3();
  ~RNDoF3();
  void Init();

  Wz4RenderParaDepthOfField3 Para,ParaBase;
  Wz4RenderAnimDepthOfField3 Anim;

  void Simulate(Wz4RenderContext *);
  void Render(Wz4RenderContext *);
};

/****************************************************************************/
class RNGlow : public Wz4RenderNode
{  
  tGlowGradientMat *MtrlGradient;
  tGlowMaskMat     *MtrlMask;
  tGlowCombMat     *MtrlComb;

public:
  RNGlow();
  ~RNGlow();
  void Init();

  Wz4RenderParaGlow Para,ParaBase;
  Wz4RenderAnimGlow Anim;

  void Simulate(Wz4RenderContext *);
  void Render(Wz4RenderContext *);
};

/****************************************************************************/

void TronInitFont(GenBitmap *out, const sChar *name, const sChar *letter, sInt sizex, sInt sizey, sInt width, sInt height, sInt style, sInt safety, sInt spacepre, sInt spacecell, sInt space, sU32 col);

/****************************************************************************/

void TronPrint(Wz4Mesh *out, GenBitmap **bmp, sInt nb, const sChar *text, sF32 zoom, sInt width, sF32 spacex, sF32 spacey);

/****************************************************************************/

void TronRTR(Wz4Mesh *out, Wz4Mesh *in1, Wz4Mesh *in2);

/****************************************************************************/


#endif // FILE_WZ4TRONLIB_TRON_HPP

