/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_FXPARTICLE_HPP
#define FILE_WZ4FRLIB_FXPARTICLE_HPP

#include "base/types.hpp"
#include "wz4frlib/wz4_demo2.hpp"
#include "wz4frlib/wz4_demo2_ops.hpp"
#include "wz4frlib/wz4_mesh.hpp"
#include "wz4frlib/wz4_mtrl2.hpp"
#include "wz4frlib/fxparticle_ops.hpp"
#include "util/shaders.hpp"
#include "extra/mcubes.hpp"

/****************************************************************************/

class RPCloud : public Wz4ParticleNode
{
  struct Particle
  {
    sVector31 Pos0;
    sVector31 Pos1;
  };
  sArray<Particle> Particles;

public:
  RPCloud();
  ~RPCloud();
  void Init();

  Wz4ParticlesParaCloud Para,ParaBase;
  Wz4ParticlesAnimCloud Anim;

  void Simulate(Wz4RenderContext *ctx);
  sInt GetPartCount();
  sInt GetPartFlags();
  void Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt);
};


class RPBallistic : public Wz4ParticleNode
{
  struct Particle
  {
    sVector31 Pos;
    sVector30 Speed;
    sF32 Time;
  };
  sArray<Particle> Particles;
public:
  RPBallistic();
  ~RPBallistic();
  void Init();

  struct Wz4ParticlesParaBallistic Para,ParaBase;
  struct Wz4ParticlesAnimBallistic Anim;
  Wz4ParticleNode *Source;

  void Simulate(Wz4RenderContext *ctx);
  sInt GetPartCount();
  sInt GetPartFlags();
  void Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt);


};

class RPMesh : public Wz4ParticleNode
{
public:
  RPMesh();
  ~RPMesh();
  void Init(Wz4Mesh *mesh);

  Wz4Mesh *Mesh;

  void Simulate(Wz4RenderContext *ctx);
  sInt GetPartCount();
  sInt GetPartFlags();
  void Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt);
};

/****************************************************************************/

class Wz4Explosion : public wObject
{
public:
  Wz4Explosion();
  ~Wz4Explosion();
  
  Wz4ExplosionParaExplosion Para;
};

class RPExploder : public Wz4ParticleNode
{
  struct Particle
  {
    sF32 Time;
    sVector31 Pos;
    sVector30 Speed;
    sVector30 RotAxis;
    sF32 AngularVelocity;
  };
  sArray<Particle> Particles;

public:
  RPExploder();
  ~RPExploder();
  void Init(Wz4Mesh *mesh,wCommand *cmd);

  Wz4ParticlesParaExploder Para;
  Wz4Mesh *Mesh;

  sInt GetPartCount();
  sInt GetPartFlags();
  void Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt);
};

/****************************************************************************/

class RNSprites : public Wz4RenderNode
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
    sU32 Color;
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
    sU32 Color;
  };


  sArray<Particle> Particles;
  sArray<Particle *> PartOrder;
  Wz4PartInfo PInfo;
  sF32 Time;

public:
  RNSprites();
  ~RNSprites();
  void Init();
  
  Wz4RenderParaSprites Para,ParaBase;
  Wz4RenderAnimSprites Anim;

  Wz4ParticleNode *Source;
  Texture2D *TextureDiff;
  Texture2D *TextureFade;

  sStaticArray<PartUVRect> UVRects; // atlas cache for speeding things up

//  void Simulate(sInt abstime,sInt reltime,sInt reset);
//  void Render(const sViewport &);


  void Simulate(Wz4RenderContext *ctx);
  void Prepare(Wz4RenderContext *ctx);
  void Render(Wz4RenderContext *ctx);
};

/****************************************************************************/

class RNChunks : public Wz4RenderNode
{
  struct Part
  {
    sMatrix34 Mat;
    sInt Index;
    sVector30 Rand;
    sF32 Time;
    sF32 Anim;
  };

  sInt Mode,Samples;
  sF32 Time;
  sArray<Part> Parts;
  sInt BoneCount;         // when in bone-mode (that is, one mesh with bones)
  Wz4PartInfo PInfo[3];

public:
  RNChunks();
  ~RNChunks();
  sBool Init();

  Wz4RenderParaChunks Para,ParaBase;
  Wz4RenderAnimChunks Anim;
  sArray<Wz4Mesh *> Meshes;
  Wz4ParticleNode *Source;

  void Simulate(Wz4RenderContext *ctx);
  void Prepare(Wz4RenderContext *ctx);
  void Render(Wz4RenderContext *ctx);
};

/****************************************************************************/

class RNDebris : public Wz4RenderNode
{
  sF32 Time;

  Wz4PartInfo PInfo;
  sArray<sMatrix34> Parts;

public:
  RNDebris();
  ~RNDebris();
  void Init();

  Wz4RenderParaDebris Para,ParaBase;
  Wz4RenderAnimDebris Anim;
  Wz4Mesh *Mesh;
  Wz4ParticleNode *Source;

  void Simulate(Wz4RenderContext *ctx);
  void Prepare(Wz4RenderContext *ctx);
  void Render(Wz4RenderContext *ctx);
};

/****************************************************************************/


class RNTrails : public Wz4RenderNode
{
  sGeometry *Geo;
  sVertexFormatHandle *Format;

  struct PartVert0
  {
    sF32 u0,v0,angle;
    void Init(sF32 u,sF32 v,sF32 a) { u0=u; v0=v; angle=a; }
  };

  struct PartVert1
  {
    sF32 px,py,pz,rot;
    sF32 sx,sy,u1,v1;
  };


  sF32 Time;
  sInt TrailCount;
  Wz4PartInfo *PInfos;
public:
  RNTrails();
  ~RNTrails();
  void Init();
  
  Wz4RenderParaTrails Para,ParaBase;
  Wz4RenderAnimTrails Anim;

  Wz4ParticleNode *Source;
  Wz4Mtrl *Mtrl;

  void Simulate(Wz4RenderContext *ctx);
  void Prepare(Wz4RenderContext *ctx);
  void Render(Wz4RenderContext *ctx);
};

/****************************************************************************/

class RNMetaballs : public Wz4RenderNode
{
  enum RNMetaballsConsts
  {
    MaxPeel = 8,
  };
  sGeometry *PartGeo;
  sMaterial *PartMtrl[MaxPeel];
  sVertexFormatHandle *PartFormat;

  sGeometry *BlitGeo;
  sMaterial *BlitMtrl;
  sVertexFormatHandle *BlitFormat;

  sMaterial *DebugMtrl;

  sF32 Time;
  Wz4PartInfo PInfo;

  sTexture2D *PeelTex[MaxPeel];

  sInt TexSizeX;
  sInt TexSizeY;
  
public:
  RNMetaballs();
  ~RNMetaballs();
  void Init();
  
  Wz4RenderParaMetaballs Para,ParaBase;
  Wz4RenderAnimMetaballs Anim;

  Wz4ParticleNode *Source;

  void Simulate(Wz4RenderContext *ctx);
  void Prepare(Wz4RenderContext *ctx);
  void Render(Wz4RenderContext *ctx);
};

/****************************************************************************/
/****************************************************************************/

class RPCloud2 : public Wz4ParticleNode
{
  struct Part
  {
    sF32 StartX;
    sF32 StartY;
    sF32 Phase;
    sF32 Speed;
    sInt ClusterId;
  };
  struct Cluster
  {
    sMatrix34 Matrix;
    sF32 Speed;
  };

  sArray<Part> Parts;
  sArray<Cluster> Clusters;
public:
  RPCloud2();
  ~RPCloud2();
  void Init(Wz4ParticlesArrayCloud2 *Array,sInt ArrayCount);

  Wz4ParticlesParaCloud2 Para,ParaBase;

  sInt GetPartCount();
  sInt GetPartFlags();
  void Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt);
};

/****************************************************************************/

class RPWobble : public Wz4ParticleNode
{
  sArray<sF32> Random;
public:
  RPWobble();
  ~RPWobble();
  void Init();

  Wz4ParticlesParaWobble Para,ParaBase;
  Wz4ParticlesAnimWobble Anim;
  Wz4ParticleNode *Source;

  void Simulate(Wz4RenderContext *ctx);
  sInt GetPartCount();
  sInt GetPartFlags();
  void Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt);
};

/****************************************************************************/

class RPTrails : public Wz4ParticleNode
{
  sInt Count;
public:
  RPTrails();
  ~RPTrails();

  Wz4ParticlesParaTrails Para,ParaBase;
  Wz4ParticleNode *Source;

  void Simulate(Wz4RenderContext *ctx);
  sInt GetPartCount();
  sInt GetPartFlags();
  void Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt);
};

/****************************************************************************/

class RPStaticParticles : public Wz4ParticleNode
{
  struct Part
  {
    sF32 Time;
    sVector31 Pos;
    sQuaternion Quat;
    sU32 Color;
  };

  sArray<Part> Parts;
  sInt Flags;

public:
  RPStaticParticles();
  ~RPStaticParticles();
  void Init(Wz4ParticlesArrayStaticParticles *ar,sInt count);

  Wz4ParticlesParaStaticParticles Para,ParaBase;

  void Simulate(Wz4RenderContext *ctx);
  sInt GetPartCount();
  sInt GetPartFlags();
  void Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt);
};

/****************************************************************************/

class RPSparcle : public Wz4ParticleNode
{
  struct Sparc
  {
    sF32 Time0;
    sVector31 Pos;
    sVector30 Speed;
  };
  sArray<Sparc> Sparcs;
  sInt MaxSparks;
  sBool NeedInit;

  void DelayedInit();

public:
  RPSparcle();
  ~RPSparcle();
  void Init();

  Wz4ParticlesParaSparcle Para,ParaBase;
  Wz4ParticleNode *Source;

  void Simulate(Wz4RenderContext *ctx);
  sInt GetPartCount();
  sInt GetPartFlags();
  void Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt);
};

/****************************************************************************/

class RPAdd : public Wz4ParticleNode
{
  sInt Count;
public:
  RPAdd();
  ~RPAdd();

//  Wz4ParticlesParaAdds Para,ParaBase;
  sArray<Wz4ParticleNode *>Sources;

  void Init();
  void Simulate(Wz4RenderContext *ctx);
  sInt GetPartCount();
  sInt GetPartFlags();
  void Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt);
};


/****************************************************************************/

class RPLissajous : public Wz4ParticleNode
{
  struct Part
  {
    sF32 Start;
    sF32 Speed;
    sVector31 Pos;
  };
  sArray<Part> Parts;
  sArray<sF32> Phases;
  sArray<sF32> Freqs;
  sArray<sF32> Amps;

  ScriptSymbol *_Phase;
  ScriptSymbol *_Freq;
  ScriptSymbol *_Amp;
public:
  RPLissajous();
  ~RPLissajous();
  void Init();

  Wz4ParticlesParaLissajous Para,ParaBase;
  Wz4ParticlesAnimLissajous Anim;

  struct Curve
  {
    sInt Axis;
    sF32 Phase;
    sF32 Freq;
    sF32 Amp;
  };
  sArray<Curve> Curves;
  Wz4ParticleNode *Source;

  void Simulate(Wz4RenderContext *ctx);
  sInt GetPartCount();
  sInt GetPartFlags();
  void Func(Wz4PartInfo &parts,sF32 time,sF32 dt);
};

/****************************************************************************/

class RPSplinedParticles : public Wz4ParticleNode
{
  struct Part
  {
    sF32 Start;
    sVector31 Pos;
  };
  sArray<Part> Parts;
public:
  RPSplinedParticles();
  ~RPSplinedParticles();
  void Init();

  Wz4ParticlesParaSplinedParticles Para,ParaBase;
  Wz4ParticlesAnimSplinedParticles Anim;

  ScriptSymbol *Symbol;
  ScriptSymbol *AltSymbol;
  ScriptSpline *Spline;
  ScriptSpline *AltSpline;
  sPoolString Name;
  sPoolString AltName;

  Wz4ParticleNode *Source;

  void Simulate(Wz4RenderContext *ctx);
  sInt GetPartCount();
  sInt GetPartFlags();
  void Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt);
};

/****************************************************************************/

class RPBulge : public Wz4ParticleNode
{
public:
  RPBulge();
  ~RPBulge();
  void Init();

  Wz4ParticlesParaBulge Para,ParaBase;
  Wz4ParticlesAnimBulge Anim;
  Wz4ParticleNode *Source;

  void Simulate(Wz4RenderContext *ctx);
  sInt GetPartCount();
  sInt GetPartFlags();
  void Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt);
};


/****************************************************************************/

class RPFromVertex : public Wz4ParticleNode
{
  struct Part
  {
    sVector31 Pos;
  };
  sArray<Part> Parts;
public:
  RPFromVertex();
  ~RPFromVertex();
  void Init(Wz4Mesh *mesh);

  Wz4ParticlesParaFromVertex Para,ParaBase;
  Wz4ParticlesParaFromVertex Anim;
  Wz4ParticleNode *Source;

  void Simulate(Wz4RenderContext *ctx);
  sInt GetPartCount();
  sInt GetPartFlags();
  void Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt);
};

/****************************************************************************/

class RPFromMesh : public Wz4ParticleNode
{
  struct Part
  {
    sVector31 Pos;
    sF32 Size;
  };
  sArray<Part> Parts;
public:
  RPFromMesh();
  ~RPFromMesh();
  void Init(Wz4Mesh *mesh);

  Wz4ParticlesParaFromMesh Para,ParaBase;
  Wz4ParticlesAnimFromMesh Anim;
  Wz4ParticleNode *Source;

  void Simulate(Wz4RenderContext *ctx);
  sInt GetPartCount();
  sInt GetPartFlags();
  void Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt);
};

/****************************************************************************/

class RPMorph : public Wz4ParticleNode
{
 struct Part
  {
    sVector31 Pos;
  };
  sArray<Part> shapeParts[16];
  sInt max;

public:
  RPMorph();
  ~RPMorph();
  void Init(sArray<Wz4Mesh*> meshArray);

  Wz4ParticlesParaMorph Para,ParaBase;
  Wz4ParticlesAnimMorph Anim;

  void Simulate(Wz4RenderContext *ctx);
  sInt GetPartCount();
  sInt GetPartFlags();
  void Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt);
};

/****************************************************************************/
/****************************************************************************/

#endif // FILE_WZ4FRLIB_FXPARTICLE_HPP

