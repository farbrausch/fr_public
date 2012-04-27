/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_WZ4_DEMO2_HPP
#define FILE_WZ4FRLIB_WZ4_DEMO2_HPP

#include "base/types.hpp"
#include "wz4lib/doc.hpp"
#include "wz4lib/basic.hpp"
//#include "wz4frlib/wz4_mtrl.hpp"
#include "util/ipp.hpp"
#include "wz4lib/script.hpp"
#include "util/taskscheduler.hpp"

class Wz4RenderNode;

/****************************************************************************/

// rendernodes

enum Wz4RenderFlags
{
  wRF_RenderWire      = 0x0001,   // render wireframe only
  wRF_RenderMain      = 0x0002,   // render main 
  wRF_RenderZ         = 0x0004,   // render z
  wRF_RenderZLowres   = 0x0008,   // generate lowres version of z buffer (also set RenderZ!)
  wRF_RenderShadows   = 0x0010,   // render shadow buffer (cubemap currently)
  wRF_RenderMask      = 0x001f,

  wRF_FreeCam         = 0x0100,   // camera control by gui
};

class Wz4RenderContext            // misc render infos
{
public:
  Wz4RenderContext();
  ~Wz4RenderContext();
  void Init(ScriptContext *script,const sTargetSpec *spec,wPaintInfo *pi);
  void PrepareView(sViewport &view);  // copy viewport to this->view, set rendertarget and then call this.
  sF32 GetTime();
  sF32 GetBaseTime();
  sF32 GetLocalTime();
  void SetTime(sF32);
  void SetLocalTime(sF32);
  void RenderControl(Wz4RenderNode *root,sInt clearflags,sU32 clrcol,const sTargetSpec &spec);
  void RenderControlZ(Wz4RenderNode *root,const sTargetSpec &spec);   // render only zbuffer in target[0] 
  void ClearRecFlags(Wz4RenderNode *root);           // prepare recursions
  void NextRecMask();
  void DoScript(Wz4RenderNode *node,sF32 time=0,sF32 basetime=0);
  sInt RecMask;                   // mask to use with RecFlags (prevent double evaluation)

  ScriptContext *Script;          // variables for scripting
  ScriptSymbol *_Time;            // time 0..1
  ScriptSymbol *_BaseTime;        // beat 0..oo
  ScriptSymbol *_LocalTime;       // beat locally after Clips
  sViewport View;                 // This is always the main camera. Every node has to recalculate the viewport! model matrix will be random!
  sFrustum Frustum;               // this will reflect shadow cameras, etc. unlike View!
  sInt RenderFlags;
  sBool CameraFlag;               // set by any camera to disable default cam
  sBool DisableShadowFlag;
//  const sRect *Window;            // renderwindow, as used for SetRenderTarget
  sTargetSpec RTSpec;       // where to render (window and texture)
  sInt ScreenX,ScreenY;           // actual screen size
  sBool SetCam;
  sMatrix34 SetCamMatrix;
  sF32 SetCamZoom;
  sMatrix34 ShakeCamMatrix;       // used to shake the camera. is this a hack?
  wPaintInfo *PaintInfo;          // before using wPaintInfo members, please check if the information is already duplicated in the context!
  sF32 LocalBeat;                 // this is set by Clip

  sInt RenderMode;                // sRF_TARGET_???
  sBool IsCommonRendermode(sBool zwrite=1);
  Wz4RenderNode *Root;            // root node during RenderControl();
 
  // render to texture helpers

  sTexture2D *ZTarget;            // access the zbuffer (0..1, z/w as usual)
  sTexture2D *ZTargetLowRes;      // lowres z rendertarget
  class IppHelper2 *IppHelper;
};

class Wz4BaseNode 
{
  sInt RefCount;
public:
  Wz4BaseNode();
  virtual ~Wz4BaseNode();
  void AddRef()    { if(this) RefCount++; }
  void Release()   { if(this) { if(--RefCount<=0) delete this; } }
};

class Wz4RenderNode : public Wz4BaseNode
{
public:
  Wz4RenderNode();
  ~Wz4RenderNode();

  virtual void Simulate(Wz4RenderContext *ctx);             // perform calculations
  virtual void Transform(Wz4RenderContext *ctx,const sMatrix34 &); // build list of model matrices with GRAPH!
  virtual void Prepare(Wz4RenderContext *ctx);              // prepare rendering
  virtual void Render(Wz4RenderContext *ctx);               // render z only

  ScriptSymbol *AddSymbol(const sChar *);                   // add symbol for animation
  void ClearRecFlagsR();                                    // prepare recursions
  void ClearMatricesR(); 
  void SimulateCalc(Wz4RenderContext *ctx);                 // call the script
  void SimulateChilds(Wz4RenderContext *ctx);               // recurse to childs
  void TransformChilds(Wz4RenderContext *ctx,const sMatrix34 &); // recurse to childs
  void PrepareChilds(Wz4RenderContext *ctx);                // recurse to childs
  void RenderChilds(Wz4RenderContext *ctx);                 // recurse to childs

  void SimulateChild(Wz4RenderContext *ctx,Wz4RenderNode *c); // just one child...
  void SetTimeAndSimulate(Wz4RenderContext *ctx,sF32 newtime,sF32 localoffset);

  sArray<Wz4RenderNode *> Childs; // tree
  ScriptCode *Code;               // code for animation
  wOp *Op;                        // (insecure) backlink to op, for error messages
  sInt Renderpass;                // used for sorting nodes 
  sInt RenderpassSort;            // used for sorting nodes 
  sBool MayCollapse;              // childs in this node may be collapsed into another node, for better renderpass sorting.
  sInt RecFlags;                  // recursion flags: prevent double calls of Simulate & Prepare
  sArray<sMatrix34CM> Matrices;   // Liste von Matrix für's zeichnen. 
  sInt TimelineStart;             // timeline range
  sInt TimelineEnd;
  sF32 ClipRandDuration;          // hack for the cliprand operator
};

class Wz4Render : public wObject
{
public:
  Wz4Render();
  ~Wz4Render();

  Wz4RenderNode *RootNode;        // do not modify the tree! it's shared!
  void AddChilds(wCommand *,sInt Renderpass,sInt first=0,sInt last=-1);
  void AddCode(wCommand *,sInt Renderpass);
};

/****************************************************************************/

enum Wz4ParticleNodeFlags
{
  wPNF_Orientation    = 0x0001,   // quaternion per particle
  wPNF_Color          = 0x0002,   // color per particle
};

struct Wz4Particle
{
  sVector31 Pos;                  // position
  sF32 Time;                      // lifetime 0..1 | -1 = not active | may loop beyond 1

  void Init(const sVector31 &p,sF32 t) { Pos=p; Time=t; }
  void Get(sVector31 &p,sF32 &t) { p=Pos; t=Time; }
  void Get(sVector31 &p) { p=Pos; }
};

class Wz4PartInfo
{
public:
  sInt Alloc;                     // Number of particles Allocated
  sInt Used;                      // Number of particles Used
  sBool Compact;                  // note that all used particles are at the beginning of the array
  sInt Flags;                     // wPNF_??? , redundant

  Wz4Particle *Parts;             // Pos and Time (required)
  sQuaternion *Quats;             // Rotation as Quaternion
  sU32 *Colors;

  Wz4PartInfo();
  ~Wz4PartInfo();
  void Init(sInt flags,sInt count);
  void Reset();
  sInt GetCount() { return Compact ? Used : Alloc; }

  struct SaveInfo
  {
    sInt Alloc;
    Wz4Particle *Parts;
    sQuaternion *Quats;
    sU32 *Colors;
  };
  void Save(SaveInfo &);
  void Load(SaveInfo &);
  void Inc(sInt i);
};


class Wz4ParticleNode : public Wz4BaseNode
{
public:
  Wz4ParticleNode();
  ~Wz4ParticleNode();
  void SimulateCalc(Wz4RenderContext *ctx);
  ScriptSymbol *AddSymbol(const sChar *);                   // add symbol for animation

  virtual void Simulate(Wz4RenderContext *ctx) { SimulateCalc(ctx); }
  virtual sInt GetPartCount() { return 0; }
  virtual sInt GetPartFlags() { return 0; }
  virtual void Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt) { }; // calc particles for time, return number of alive particles

  ScriptCode *Code;               // code for animation
  wOp *Op;                        // (insecure) backlink to op, for error messages
};

class Wz4Particles : public wObject
{
public:
  Wz4Particles();
  ~Wz4Particles();
  Wz4ParticleNode *RootNode;

  void AddCode(wCommand *cmd);
};

/****************************************************************************/

class Rendertarget2D : public Texture2D
{
  sInt RealX,RealY;
  sTextureBase *RealTexture;
  sTextureProxy *ProxyTexture;
public:
  Rendertarget2D();
  ~Rendertarget2D();
  void Init();
  void Acquire(sInt sx,sInt sy);
  void Flush();
  void EndShow();

  sInt Flags;
  sInt Format;
  sInt SizeX,SizeY;
  sInt AtlasPower;
};

/****************************************************************************/

void UnitTestWz4Render(sImage &img,struct UnitTestParaUnitTestWz4 *para,Wz4Render *in0);

/****************************************************************************/

#endif // FILE_WZ4FRLIB_WZ4_DEMO2_HPP

