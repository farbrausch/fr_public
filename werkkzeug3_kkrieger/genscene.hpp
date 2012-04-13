// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __GENSCENE__
#define __GENSCENE__

#include "_types.hpp"
#include "kdoc.hpp"

/****************************************************************************/
/****************************************************************************/

class GenMesh;
class EngMesh;
class KKriegerMesh;
class GenEffect;

class GenScene : public KObject
{
public:
  GenScene();
  ~GenScene();
  void Copy(KObject *);

  sArray<GenScene *> Childs;      // childs

  // mesh
  EngMesh *DrawMesh;              // mesh to use for drawing
#if sLINK_KKRIEGER
  KKriegerMesh *CollMesh;         // collision mesh
#endif
  GenEffect *Effect;              // or maybe just an effect?

  // monster
//  struct KKriegerMonster *Monster;// monster info

  // sector
  sBool IsSector;                 // is sector yes/no.
  sBool SectorPaint;              // paint/don't paint this sector
  sInt SectorVisited;             // for portal traversal
  sFRect PortalBox;               // portalbox for this sector
  GenScene *Next;                 // linked list (sector painting)
  sMatrix SectorMatrix;           // matrix
  KOp *Sector;                    // input op

  sF32 SRT[9];                    // transform
  sInt Count;                     // 0 is normal transform, >=1 is multiply
};

/****************************************************************************/

GenScene * __stdcall Init_Scene_Scene(GenMesh *mesh,sF323 s,sF323 r,sF323 t,sBool lightmap);
GenScene * __stdcall Init_Scene_Add(sInt count,GenScene *s0,...);
GenScene * __stdcall Init_Scene_Multiply(GenScene *scene,sF323 s,sF323 r,sF323 t,sInt count);
GenScene * __stdcall Init_Scene_Transform(GenScene *scene,sF323 s,sF323 r,sF323 t);
GenScene * __stdcall Init_Scene_Particles(GenScene *scene,sInt mode,sInt count,sInt seed,sF323 rand,sF323 rot,sF323 rotspeed,sF32 anim,sF323 line,KSpline *spline);
GenScene * __stdcall Init_Scene_Light(sF323 r,sF323 t,sU32 flags,sF32 aspect,sU32 color,sF32 amplify,sF32 range);
GenScene * __stdcall Init_Scene_Camera(sF323 s,sF323 r,sF323 t);
GenScene * __stdcall Init_Scene_Limb(GenScene *scene0,GenScene *scene1,sF323 pos,sF323 dir,sF32 l0,sF32 l1,sU32 flags);
GenScene * __stdcall Init_Scene_Walk(GenScene *scene0,
  sU32 Flags,sInt FootCount,sF32 StepTresh,sF32 RunLowTresh,sF32 RunHighTresh,
  sInt2 ft,sF322 sl,sF322 ss,sF322 sn,
  sF323 l0,sF323 l1,sF323 l2,sF323 l3,sF323 l4,sF323 l5,sF323 l6,sF323 l7,
  sF32 scanup,sF32 scandown,
  KSpline *stepspline);
GenScene * __stdcall Init_Scene_Rotate(GenScene *scene0,sF323 dir,sInt axxis);
GenScene * __stdcall Init_Scene_Forward(GenScene *scene0,sF32 tresh);
GenScene * __stdcall Init_Scene_Sector(GenScene *scene0);
GenScene * __stdcall Init_Scene_RenderLight(GenScene *scene);
GenScene * __stdcall Init_Scene_Portal(GenScene *in,GenScene *s0,GenScene *s1,GenScene *si,sF322 x,sF322 y,sF322 z,sInt cost,sF32 door);
GenScene * __stdcall Init_Scene_Physic(GenScene *in,sInt flags,sF323 speed,sF323 scale,sF323 rmass,sF32 mass,sInt partkind);
GenScene * __stdcall Init_Scene_ForceLights(GenScene *in,sInt mode,sInt sw);
GenScene * __stdcall Init_Scene_MatHack(class GenMaterial *);
GenScene * __stdcall Init_Scene_AdjustPass(GenScene *in,sInt adjust);
GenScene * __stdcall Init_Scene_ApplySpline(GenScene *in,class GenSpline *sp);
GenScene * __stdcall Init_Scene_Marker(GenScene *add,sInt marker);
GenScene * __stdcall Init_Scene_LOD(GenScene *high,GenScene *low,sF32 lod);
GenScene * __stdcall Init_Scene_Ambient(sU32 color);

void __stdcall Exec_Scene_Scene(KOp *op,KEnvironment *kenv,sF323 s,sF323 r,sF323 t,sBool lightmap);
void __stdcall Exec_Scene_Add(KOp *op,KEnvironment *kenv);
void __stdcall Exec_Scene_Multiply(KOp *op,KEnvironment *kenv,sF323 s,sF323 r,sF323 t,sInt count);
void __stdcall Exec_Scene_Transform(KOp *op,KEnvironment *kenv,sF323 s,sF323 r,sF323 t);
void __stdcall Exec_Scene_Light(KOp *op,KEnvironment *kenv,sF323 r,sF323 t,sU32 flags,sF32 zoom,sU32 color,sF32 amplify,sF32 range);
void __stdcall Exec_Scene_Particles(KOp *op,KEnvironment *kenv,sInt mode,sInt count,sInt seed,sF323 rand,sF323 rot,sF323 rotspeed,sF32 anim,sF323 line,KSpline *spline);
void __stdcall Exec_Scene_Camera(KOp *op,KEnvironment *kenv,sF323 s,sF323 r,sF323 t);
void __stdcall Exec_Scene_Limb(KOp *op,KEnvironment *kenv,sF323 pos,sF323 dir,sF32 l0,sF32 l1,sU32 flags);
void __stdcall Exec_Scene_Walk(KOp *op,KEnvironment *kenv,
  sU32 Flags,sInt FootCount,sF32 StepTresh,sF32 RunLowTresh,sF32 RunHighTresh,
  sInt2 ft,sF322 sl,sF322 ss,sF322 sn,
  sF323 l0,sF323 l1,sF323 l2,sF323 l3,sF323 l4,sF323 l5,sF323 l6,sF323 l7,
  sF32 scanup,sF32 scandown,
  KSpline *stepspline);
void __stdcall Exec_Scene_Rotate(KOp *op,KEnvironment *kenv,sF323 dir,sInt axxis);
void __stdcall Exec_Scene_Forward(KOp *op,KEnvironment *kenv,sF32 tresh);
void __stdcall Exec_Scene_Sector(KOp *op,KEnvironment *kenv);
void __stdcall Exec_Scene_Portal(KOp *op,KEnvironment *kenv,sF322 x,sF322 y,sF322 z,sInt cost,sF32 door);
void __stdcall Exec_Scene_Physic(KOp *op,KEnvironment *kenv,sInt flags,sF323 speed,sF323 scale,sF323 rmassf,sF32 mass,sInt partkind);
void __stdcall Exec_Scene_ForceLights(KOp *op,KEnvironment *kenv,sInt mode,sInt sw);
void __stdcall Exec_Scene_MatHack(KOp *op,KEnvironment *kenv);
void __stdcall Exec_Scene_AdjustPass(KOp *op,KEnvironment *kenv,sInt adjustPass);
void __stdcall Exec_Scene_ApplySpline(KOp *op,KEnvironment *kenv,sF32 time);
void __stdcall Exec_Scene_Marker(KOp *op,KEnvironment *kenv,sInt marker);
void __stdcall Exec_Scene_LOD(KOp *op,KEnvironment *kenv,sF32 lod);
void __stdcall Exec_Scene_Ambient(KOp *op,KEnvironment *kenv,sU32 color);

/****************************************************************************/

GenScene * MakeScene(KObject *in);

/****************************************************************************/
/****************************************************************************/

#if !sPLAYER
extern KOp *ShowPortalOp;
extern sVector ShowPortalCube[8];
extern sBool ShowPortalOpProcessed;

extern sBool SceneWireframe;
extern sInt SceneWireFlags;
extern sU32 SceneWireMask;
#endif

#define DPS_WIRE    0x0001
#define DPS_LIGHTS  0x0002

/****************************************************************************/
/****************************************************************************/

#endif
