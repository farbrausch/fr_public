// This file is distributed under a BSD license. See LICENSE.txt for details.

#pragma once
#include "kdoc.hpp"
#include "script.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   forwards                                                           ***/
/***                                                                      ***/
/****************************************************************************/

class KSCeneNode2;
struct KEnvironment2;
struct KEffect2;
struct KEvent2;

/****************************************************************************/
/***                                                                      ***/
/***   scene                                                              ***/
/***                                                                      ***/
/****************************************************************************/

class KSceneNode2 : public KObject
{
public:
  KSceneNode2();
  ~KSceneNode2();
  void Copy(KObject *);

  sMatrix Matrix;                 // transformation from animation
  sMatrix WorldMatrix;            // transformation after concatenation
  sAABox Bounds;                  // bound of all childs (and this one)
  KSceneNode2 *Next;              // next sibling on same lavel
  KSceneNode2 *First;             // first child

  class EngMesh *Mesh;            // -> drawing
  void *Light;                    // -> lighting
  class WerkSceneNode2 *BackLink; // go back to werk-gui
};

/****************************************************************************/
/***                                                                      ***/
/***   effects                                                            ***/
/***                                                                      ***/
/****************************************************************************/


struct KEnvironment2
{
  void Init();
  void Exit();
  sViewport View;                           // viewport 
  sMaterialEnv Env;                         // camera
  sBool InScene;                            // BeginViewport() already called
  sInt BeatTime;                            // absolte time
  sInt EffectBeatTime;                      // time relative to effect
  sArray<struct KEvent2 *> Events;          // list of all effects, for globals.
};

#define K2MAX_LINK  8

/****************************************************************************/

struct KEffect2Symbol
{
  sChar Name[KK_NAME];
  sInt Id;
  sInt Type;            // 0..3 -> Scalar, Vector, Matrix, Spline
};

enum KEffect2ParaInfoType 
{  
  EPT_INT = 1,
  EPT_CHOICE,
  EPT_FLAGS,
  EPT_FLOAT,
  EPT_MATRIX,
  EPT_COLOR,
  EPT_LINK,
  EPT_GROUP,
  EPT_ALIAS,
};

enum KEffect2ParaAnimType
{
  EAT_SCALAR      = 0x0001,
  EAT_VECTOR      = 0x0002,
  EAT_EULER       = 0x0004,
  EAT_SRT         = 0x0008,
  EAT_TARGET      = 0x0010,
  EAT_COLOR       = 0x0020,
};

struct KEffect2ParaInfo2
{
  sInt Type;
  sInt Offset;
  sInt EditOffset;                      // secondary offset for sMatrixEdit parameters
  sInt Count;
  sInt KCType;
  sF32 Min,Max,Step;
  sInt AnimType;                        // bitmask of allowed animationtypes
  const sChar *Name;
  const sChar *Choice;
};

struct KEffectInfo
{
  void Init();
  void Exit();
  void Clear();
  sArray<KEffect2ParaInfo2> ParaInfo;
  sArray<KEffect2Symbol> Exports;
  sF32 DefaultPara[64];
  sF32 DefaultEdit[64];

  KEffect2ParaInfo2 *AddPara  (const sChar *name,sInt offset,sInt Type);            // use for matrix,color
  KEffect2ParaInfo2 *AddLink  (const sChar *name,sInt index,sInt kc_type); 
  KEffect2ParaInfo2 *AddAlias (const sChar *name); 
  KEffect2ParaInfo2 *AddGroup (const sChar *name);
  KEffect2ParaInfo2 *AddChoice(const sChar *name,sInt offset,const sChar *choices);
  KEffect2ParaInfo2 *AddFlags (const sChar *name,sInt offset,const sChar *choices);
  KEffect2ParaInfo2 *AddInt   (const sChar *name,sInt offset,sF32 Min,sF32 Max,sF32 Step,sInt Count=1);
  KEffect2ParaInfo2 *AddFloat (const sChar *name,sInt offset,sF32 Min,sF32 Max,sF32 Step,sInt Count=1);
  KEffect2ParaInfo2 *AddColor (const sChar *name,sInt offset);            // use for matrix,color
  KEffect2ParaInfo2 *AddMatrix(const sChar *name,sInt offset,sInt editoffset);   // use for matrix,color

  void AddSymbol(const sChar *name,sInt id,sInt type);
};

struct KEffect2
{
  const sChar *Name;
  sU32 ButtonColor;
  sInt Id;
  sU32 Flags;
  sInt ParaSize;
  sInt EditSize;

  // player calls
  void (*OnInit)(KEvent2 *,KEnvironment2 *);
  void (*OnExit)(KEvent2 *,KEnvironment2 *);
  void (*OnPaint)(KEvent2 *,KEnvironment2 *);
  void (*OnSet)(KEvent2 *,KEnvironment2 *,sInt id,sF32 *values,sU32 mask);

  // editor calls
  void (*OnPara)(KEffectInfo *);

  sInt Types[K2MAX_LINK];

  KEffectInfo *Info;
};

enum KEffect2Flags
{
  KEF_NEEDBEGINSCENE    = 1,
};

/****************************************************************************/
/***                                                                      ***/
/***   events                                                             ***/
/***                                                                      ***/
/****************************************************************************/


struct KEvent2
{
public:
  void Init(const KEffect2 *ef);
  void Exit();
  sInt Start;                     // time start
  sInt End;                       // time end
  sInt LoopEnd;                   // loop end

  const KEffect2 *Effect;         // effect-class
  sF32 *Para;                     // effect parameters          
  sF32 *Edit;                     // editor parameters
  KObject *Links[K2MAX_LINK];     // links to mesh/texture/material/...
  KEvent2 *Alias;                 // alias-event
  sArray<KSpline *> Splines;      // list of splines, updated during Build()
  sArray<KSceneNode2 *> SceneNodes; // list of nodes, updated during Build()

#if !sLINK_KKRIEGER || !sPLAYER
  sScript Code;
#endif
};


/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   bytecode                                                           ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   some code                                                          ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

extern KEffect2 EffectList[];

void NovaEngine(class WerkDoc2 *doc2,class WerkDoc *doc);
void NovaSceneWalk(KSceneNode2 *node,const sMatrix &pmat);

/****************************************************************************/
/****************************************************************************/
