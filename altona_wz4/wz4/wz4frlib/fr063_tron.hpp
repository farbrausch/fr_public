/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_FR063_TRON_HPP
#define FILE_WZ4FRLIB_FR063_TRON_HPP
#include <float.h>
#include "base/types.hpp"
#include "wz4frlib/tADF.hpp"
#include "util/taskscheduler.hpp"
#include "Wz4frlib/Wz4_mesh.hpp"
#include "wz4frlib/fr063_tron_shader.hpp"
#include "fr063_tron_ops.hpp"
//#include "../wz4tronlib/adf.hpp"
#include "wz4frlib/pdf.hpp"
#include "fr063_sph.hpp"



/****************************************************************************/

struct SCMass
{
  inline void Init(const	sVector31 &p, const sVector30 &v, float m, sInt t)
  {
		ROP=RP=OP=P=p;
		ROV=RV=OV=V=v;
		M=m;
    T=t;
    F.Init(0,0,0);
  }

  inline void ClearForce()
  {
    F.Init(0,0,0);
  }

  inline void AddForce(const sVector30 &f)
  {
    F = F + f;
  }

  inline void SubForce(const sVector30 &f)
  {
    F = F - f;
  }

  inline void Save()
  {
    OP=P;
    OV=V;
  }

  inline void Restore()
  {
    P=OP;
    V=OV;
  }
  

  inline void Freeze()
  {
    RP=P;
    RV=V;
    ROP=OP;
    ROV=OV;
  }

  inline void Reset()
  {
    P=OP=RP;
    V=OV=RV;
    OP=ROP;
    OV=ROV;
  }

  sVector31 RP;	  //Reset Position
	sVector30 RV;	  //Reset Velocity
  sVector31 ROP;	//Reset Old Position
	sVector30 ROV;	//Reset Old Velocity  
  sVector31 OP;	  //Old Position
	sVector30 OV;	  //Old Velocity	
  sVector31 P;    //Position
  sVector30 V;    //Velocity
	sVector30 F;	  //Force			
	float     M;    //Mass
  sInt      T;    //Type, 0=normal, 1=Fixing-Total, 2=Fixing-AfterInit, 3=Release-AfterInit
};


struct SCSpring
{
  inline void Init(SCMass *s, SCMass *e, float c)
  {
		S=s;
		E=e;
    L=(e->P-s->P).Length();
		C=c;
  }

  inline void CalcForces()
  {
	  sVector30 posdiff = (S->P - E->P);
    sF32 distanceq = posdiff.LengthSq();    

    if (distanceq<0.0001f || distanceq>100000.0f)
    {
      F=sVector30(0,0,0);      
      return;
    }    
    sF32 distance = sSqrt(distanceq);
	  sF32 cforce = ((distance-L) * C)/distance;
    F = -cforce*posdiff;
  }

	SCMass *S;
  SCMass *E;
	float   L;
	float   C;	
  sVector30 F;  
};

/****************************************************************************/

class RNFR063_Sprites : public Wz4RenderNode
{
  sGeometry *Geo;
  sMaterial *Mtrl;
  sVertexFormatHandle *Format;

  sF32 Time;
//  sInt KeepCount;

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
    sVector4 color;
    sF32 sx,sy,u1,v1;
    PartUVRect uvrect;    
    sF32 fade;
  };

  struct Particle 
  {
    sInt Group;
    sVector31 Pos;
    sF32 RotStart;
    sF32 RotRand;
    sF32 FadeRow;
    sF32 SizeRand;
    sF32 TexAnimRand;
    sF32 Time;
    sF32 Dist;
    sF32 DistFade;
    sBool Drop;
    PartVert1 GeoDataCache;
  };

  sArray<Particle> Particles;
  sArray<Particle *> PartOrder;
  Wz4PartInfo PInfo;
  sInt KeepCount;

public:
  RNFR063_Sprites();
  ~RNFR063_Sprites();
  void Init();
  
  Wz4RenderParaFR063_SpritesADF Para,ParaBase;
  Wz4RenderAnimFR063_SpritesADF Anim;

  Wz4ParticleNode *Source;
  Texture2D *TextureDiff;  
  Wz4ADF    *SDF;  

  sStaticArray<PartUVRect> UVRects; // atlas cache for speeding things up

//  void Simulate(sInt abstime,sInt reltime,sInt reset);
//  void Render(const sViewport &);


  void Simulate(Wz4RenderContext *ctx);
  void Prepare(Wz4RenderContext *ctx);
  void Render(Wz4RenderContext *ctx);
};

/****************************************************************************/

class RNFR063ClothGridSimRender : public Wz4RenderNode
{
public:
  sGeometry *Geo;   // geometry holding the Grid    
  
	sArray <SCMass>   Particles;	
	sArray <SCSpring> Springs;
  sArray <SCMass *>   Moveable;

  sF32 LastTime;
  sF32 Time;

  struct ArrayData
  {    
    ScriptSymbol                          *Symbol;
    ScriptSpline                          *Script;
    sVector31                              Pos;
    Wz4RenderArrayFR063_ClothGridSimRender Para;
  };

  RNFR063ClothGridSimRender();
  ~RNFR063ClothGridSimRender();

  void Init(Wz4ADF *dfobj);

  void Simulate(Wz4RenderContext *ctx);   // execute the script. 
  void Prepare(Wz4RenderContext *ctx);    // do simulation
  void Render(Wz4RenderContext *ctx);     // render a pass

  void CalcParticles(sInt steps, sBool init, sF32 time);
  void Simulate(sF32 time);
  void Reset();
  static void Handles(wPaintInfo &pi, Wz4RenderParaFR063_ClothGridSimRender *para, wOp *op);
  
  sArray<sVertexStandard>                 Vertices;
  sArray <ArrayData>                      Array;
  Wz4RenderParaFR063_ClothGridSimRender   Para,ParaBase; // animated parameters from op  
  Wz4RenderAnimFR063_ClothGridSimRender   Anim;          // information for the script engine

  Wz4Mtrl *Mtrl;                  // material from inputs
  Wz4ADF *DF;    
};

/****************************************************************************/

class RNFR063_MassBallColl : public Wz4RenderNode
{
  struct MBCBall
  {
    sVector31 CP; //Current Position
    sVector30 CV; //Current Velocity
    sVector30 CR; //Current Respone
    sVector31 RP; //Reset Position
    sVector30 RV; //Reset Velocity    
    sF32      R;  //Radius
    sF32      M;  //Mass

    inline void Init(const sVector31 &p, const sVector30 &v, const sF32 &r, const sF32 &m)
    {
      CP=RP=p;
      CV=RV=v;
      R=r;
      M=m;
      CR.Init(0,0,0);
    }
    inline MBCBall()
    {
      Init(sVector31(0.0f,0.0f,0.0f), sVector30(0.0f,0.0f,0.0f), 1.0f, 1.0f);
    }
    inline MBCBall(const sVector31 &p, const sVector30 &v, const sF32 &r, const sF32 &m)
    {
      Init(p,v,r,m);
    }
    inline void Reset()
    {
      CP=RP;
      CV=RV;      
    }
  };
  
  struct VertStream0
  {
    sF32 u0,v0,angle;
    inline void Init(sF32 u,sF32 v,sF32 a) { u0=u; v0=v; angle=a; }
  };

  struct VertStream1
  {
    sF32 px,py,pz,radius;
    inline void Init(sVector31 pos, sF32 r)
    {
      px=pos.x;
      py=pos.y;
      pz=pos.z;
      radius=r;
    }
  };

  enum CollType
  {
    COLLTYPE_NONE = 0,
    COLLTYPE_STUCK,
    COLLTYPE_HIT
  };


  public:

  RNFR063_MassBallColl();
  ~RNFR063_MassBallColl();
  void Init();
  void Simulate(Wz4RenderContext *ctx);  
  void Simulate(sInt steps);
  void Prepare(Wz4RenderContext *ctx);
  void Render(Wz4RenderContext *ctx);

  sInt CollCheck(const sVector31 &sp, const sVector31 &ep, const sInt ball, sVector31 &cp, sF32 &t);
  

  sGeometry                      *Geo;    
  sMaterial                      *Mtrl;
  sVertexFormatHandle            *Format;  
  sF32                            LastTime;
  
  Wz4ADF                         *SDF;  
  Wz4RenderParaFR063_MassBallColl Para,ParaBase;
  Wz4RenderAnimFR063_MassBallColl Anim;

  sArray <MBCBall>               Balls;

};

/****************************************************************************/

class SphCollSDF : public SphCollision
{
public:
  SphCollisionParaSphSDFColl Para;
  Wz4ADF                    *SDF;  
  SphCollSDF();
  ~SphCollSDF();
  void Init();
  void CollPart(RPSPH *);
};

/****************************************************************************/

class SphCollPDF : public SphCollision
{
public:
  SphCollisionParaSphPDFColl Para;
  Wz4PDF                    *PDF;  
  void Init();
  void CollPart(RPSPH *);
};

/****************************************************************************/

class SphMorphSDF : public SphCollision
{
public:
  SphCollisionParaSphSDFMorph Para;
  Wz4ADF                    *SDF;  
  void Init();
  void CollPart(RPSPH *);
};


/****************************************************************************/

class FR063_SpritesExt : public Wz4RenderNode
{
  sGeometry *Geo;
  sMaterial *Mtrl;
  sVertexFormatHandle *Format;

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
    sF32 dix,diy,diz,diw;
    sF32 djx,djy,djz,djw;
    sU32 Color;
  };
  
  sBool DoRender;
public:

  struct Particle 
  {    
    sVector31 Pos;
    sF32 FadeRow;
    sF32 TexAnimRand;
    sF32 Time;
    sF32 Dist;
    sF32 DistFade;    
    sVector30 Di;
    sVector30 Dj;
    Wz4RenderArrayFR063_SpritesExt Para;
  };

  sArray<Particle> Particles;  

  FR063_SpritesExt();
  ~FR063_SpritesExt();
  void Init();
  
  Wz4RenderParaFR063_SpritesExt Para,ParaBase;
  Wz4RenderAnimFR063_SpritesExt Anim;
  
  Texture2D *TextureDiff;
  Texture2D *TextureFade;

  sStaticArray<PartUVRect> UVRects; // atlas cache for speeding things up

  void Simulate(Wz4RenderContext *ctx);
  void Prepare(Wz4RenderContext *ctx);
  void Render(Wz4RenderContext *ctx);
  static void Handles(wPaintInfo &pi, Wz4RenderParaFR063_SpritesExt *para, wOp *op);
};


/****************************************************************************/

class RNFR063AermelKanal : public Wz4RenderNode
{
public:  
  sMatrix34 Matrix; // calculated in Prepare, used in Render
  
  ScriptSymbol *PathSymbol;  
  ScriptSymbol *MovementSymbol;

  ScriptSpline *PathSpline;  
  ScriptSpline *MovementSpline;
  
  sPoolString PathName;
  sPoolString MovementName;

  RNFR063AermelKanal();
  ~RNFR063AermelKanal();

  void Init();  
  void Simulate(Wz4RenderContext *ctx);
  void Prepare(Wz4RenderContext *ctx);
  void Render(Wz4RenderContext *ctx);
  void Transform(Wz4RenderContext *ctx,const sMatrix34 &);
  void TransformChilds(Wz4RenderContext *ctx,const sMatrix34 &mat);
  static void Handles(wPaintInfo &pi, Wz4RenderParaFR063_ClothGridSimRender *para, wOp *op);
    
  void Init2();  
  sF32 CalcTime(float time);
  int GetDataInfo(sInt &dim);
  int GetData(float time, sVector31 &pos, sVector2 &radius, sF32 &rotation);
  void GetPos(float time, sVector31 &pos);
  void CalcRing(const sVector31 &pos, const sVector30 dir, const sVector2 radius, const sF32 rotation, sVertexStandard *buf);  
  void GetMovementData(float time, float &tunneltime, float &objtime, float &objrot);
  

  Wz4RenderParaFR063_AermelKanal Para,ParaBase;
  Wz4RenderAnimFR063_AermelKanal Anim;
  
  sBool InitDone;
  sGeometry *FixGeo;
  sGeometry *DynGeo;
  sArray<sVertexStandard> FixVertices;
  sArray<sVertexStandard> DynVertices;
  sMatrix34 ObjMat;
  sMatrix34 TunnelMat;
  sF32 Sts; //Time step from one segment to another
  sDrawRange DrawRange[2];
  
  //Wz4RenderNode *Object;
//Filled by op
  Wz4Mesh *Mesh;  //Mesh from Input  
  Wz4Mtrl *Mtrl;  //Material from Input
  Wz4ADF  *DF;     //Distance Field from the input
};

/****************************************************************************/

#endif // FILE_WZ4FRLIB_FR063_TRON_HPP

