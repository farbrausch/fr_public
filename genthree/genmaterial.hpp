// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef __GENMATERIAL_HPP__
#define __GENMATERIAL_HPP__

#include "_start.hpp"
#include "genmisc.hpp"

#define GENMAT_MAXTEX     8       // that many textures can be used in one pass
#define GENMAT_MAXPASS    16      // multiple passes with different geometry
#define GENMAT_MAXBATCH   16      // each pass may be devided into multiple batches, since you should not draw more than 0x7fff vertices per batch
#define GENMAT_MAXSCENE   32      // max number of static scenes

/****************************************************************************/
/****************************************************************************/

class GenLight : public sObject
{
public:
  GenLight();
  sU32 GetClass() { return sCID_GENLIGHT; }
#if !sINTRO
  void Copy(sObject *);
#endif
  void Set(sMatrix &mat);

// begin
  sInt4 Diffuse;                  // light parameters
  sInt4 Ambient;
  sInt4 Specular;
  sF32 Range;
  sF32 RangeCut;
  sF32 Amplify;
  sF32 SpotFalloff;
  sF32 SpotInner;
  sF32 SpotOuter;
  sInt Mode;                      // ambient / point / spot / dir
  sU32 Mask;                      // bitmask for light management
  sInt Ordinal;                   // light ordinal in DirectX
// end

};

class GenScene : public sObject
{
public:
  GenScene();
  ~GenScene();
  void Tag();
  sU32 GetClass() { return sCID_GENSCENE; }
#if !sINTRO
  void Copy(sObject *);
#endif

  void Prepare();
  void PaintR(sMatrix &mat,sInt action);
  void Paint(sMatrix &mat);

  // start of block
  sF32 SRT[9];
  sInt Multiply;
  class GenMesh *Mesh;
  class GenLight *Light;
  class GenParticles *Part;

  // end of block
  sArray<class GenScene *> Childs;
};

/****************************************************************************/
/****************************************************************************/

class GenMatPass : public sObject
{
public:
  GenMatPass();
  void Tag();
  sU32 GetClass() { return sCID_GENMATPASS; }
#if !sINTRO
  void Copy(sObject *);
#endif

  void Set();
// begin
                                  // variables passed to script:
  sF32 Aspect;                    // sprite aspect ratio, line texture loop
  sF32 Size;                      // sprite/line size
  sInt RenderPass;                // 0..255
  sInt4 Color;                    // DX-TFactor
  sF32 Enlarge;                   // tststs


                                  // non-animatable parameters
  sInt Programm;                  // MPP_???
  sU32 FVF;                       // requested destination FVF
  GenMatPass *Next;               // reuse geometry with other renderstates. Prgramm, FVF and RenderPass ignored on next GenMatPass
  sU32 LightMask;                 // enable lights. default 0xffff
                          
  sU32 Flags;                     // 1 = set tfactor, 2 = set material
  class GenBitmap *Texture[GENMAT_MAXTEX];
  class GenMatrix *TexTrans[GENMAT_MAXTEX];
  sInt TexTransMode[GENMAT_MAXTEX];
  sU32 States[256];
#if sINTRO
  sD3DMaterial MatLight;
  sInt MatLightSet;
#endif
  sU32 *StatePtr;
  sInt Optimised;
// end

};

#define MPP_OFF             0     // unused
#define MPP_STATIC          1     // static vertex and index buffer
#define MPP_DYNAMIC         2     // dynamic vertex and static index buffer
#define MPP_SPRITES         3     // pointsprites
#define MPP_TREES           4     // tree-like billboards
#define MPP_THICKLINES      5     // thick lines
#define MPP_OUTLINES        6     // thick outlines
#define MPP_SHADOW          7     // shadow volume extrusion
#define MPP_SPIKES          8     // tree-like billboards
#define MPP_FINNS           9

/****************************************************************************/

class GenMaterial : public sObject
{
public:
  GenMaterial();
  void Set();
  void Tag();
  void Default();
  sU32 GetClass() { return sCID_GENMATERIAL; }
#if !sINTRO
  void Copy(sObject *);
#endif
  GenMatPass *MakePass(sInt pass);

  GenMatPass *Passes[GENMAT_MAXPASS];
};

/****************************************************************************/
/****************************************************************************/

struct GenParticle
{
  sF32 x,y,z;
  sF32 time;
  sF32 xs,ys,zs;
  sF32 rot;
};

class GenParticles : public sObject
{
public:
  GenParticles();
  ~GenParticles();
  void Tag();
  sU32 GetClass() { return sCID_GENPARTICLES; }
#if !sINTRO
  void Copy(sObject *);
#endif
#if !sINTRO_X
  void Paint(sMatrix &mat,sInt slices);
#endif

  sF32 Rate;
  sF32 Jitter;
  sInt MaxPart;
  sF32 Lifetime;
  sF323 Gravity;

  sF323 Scale;
  sF323 Rotate;
  sF323 Translate;
  sU32 Flags; 
  sF32 Surface;
  sF32 Scatter;
  sF32 Speed;
  sF32 SpeedRandom;

  sF32 RotStart;
  sF32 RotRand;
  sF32 RotSpeed;

  sF322 Size;
  sF322 SizeTime;
  sF32 Aspect;
  sInt4 Col0;
  sInt4 Col1;
  sInt4 Col2;
  sF323 ColTime;

  sF32 SplineForce;
  sInt SplineMode;


private:
  sF32 RateCount;
  sInt Geometry;
  sInt LastFrame;
  sInt LastTime;

public:
  GenMaterial *Material;
  GenSpline *Spline;
  sInt __last;

private:
  sArray<GenParticle> Part;
};

/****************************************************************************/
/****************************************************************************/

//sBool Scene_New(class ScriptRuntime *);  // obsolete

GenScene * __stdcall Scene_Mesh(GenMesh *,sF323 s,sF323 r,sF323 t);
GenScene * __stdcall Scene_Add(GenScene *,GenScene *);
GenScene * __stdcall Scene_Trans(GenScene *,sF323 s,sF323 r,sF323 t);
GenScene * __stdcall Scene_Multiply(GenScene *,sF323 s,sF323 r,sF323 t,sInt count);
GenScene * __stdcall Scene_Paint(GenScene *);
GenScene * __stdcall Scene_Label(GenScene *,sInt string);
GenScene * __stdcall Scene_Light(sF323 s,sF323 r,sF323 t,sInt mode,sInt mask,sInt label,sF32 range,sF32 rangecut,sF32 amplify,sF32 spotfalloff,sF32 spotinner,sF32 spotouter,sInt4 diff,sInt4 amb,sInt4 spec);
//GenScene * __stdcall Scene_PaintLater(GenScene *sa);
GenScene * __stdcall Scene_PartScene(GenParticles *p);

GenMaterial * __stdcall Material_NewMaterial();
sObject * __stdcall MatPass_MatBase(sObject *obj,sInt pass,sInt flags,sInt mode,sInt rp,sInt4 color);
sObject * __stdcall MatPass_MatTexture(sObject *obj,GenBitmap *bm,sInt pass,sInt stage,sInt flags);
sObject * __stdcall MatPass_MatFinalizer(sObject *obj,sInt pass,sInt program,sF32 aspect,sF32 size,sF32 enlarge);
sObject * __stdcall MatPass_MatLabel(sObject *obj,sInt label,sInt pass);
sObject * __stdcall MatPass_MatTexTrans(sObject *obj,sInt label,sInt pass,sInt stage,sF323 s,sF323 r,sF323 t);

sObject * __stdcall MatPass_MatDX7(sObject *obj,sInt pass,sInt color0,sInt color1,sInt alpha0,sInt alpha1);
sObject * __stdcall MatPass_MatBlend(sObject *obj,sInt pass,sInt alphatest,sInt alpharef,sInt source,sInt op,sInt dest);
sObject * __stdcall MatPass_MatStencil(sObject *obj,sInt pass,sInt stenciltest,sInt stencilref,sInt sfail,sInt zfail,sInt zpass);
sObject * __stdcall MatPass_MatLight(sObject *obj,sInt pass,sInt ds,sInt ss,sInt as,sInt es,sInt4 d,sInt4 s,sInt4 a,sInt4 e,sF32 specularity,sInt specularadd,sInt mask);

GenParticles * __stdcall Part_New(GenMaterial *mtrl,sF32 rate,sF32 jitter,sInt maxpart,sF32 life,sF323 gravity);
GenParticles * __stdcall Part_Emitter(GenParticles *sys,sF323 s,sF323 r,sF323 t,sInt flags,sF32 surface,sF32 scatter,sF32 speed,sF32 speedrand);
GenParticles * __stdcall Part_Rotate(GenParticles *sys,sF32 rotstart,sF32 rotrand,sF32 rotspeed);
GenParticles * __stdcall Part_Life(GenParticles *sys,sF322 size,sF322 sizetime,sF32 aspect,sInt4 col0,sInt4 col1,sInt4 col2,sF323 coltime);
GenParticles * __stdcall Part_Spline(GenParticles *sys,GenSpline *spline,sInt mode,sF32 force);

/****************************************************************************/
/****************************************************************************/

#endif
