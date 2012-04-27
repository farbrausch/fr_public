/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_WZ4_MODMTRL_HPP
#define FILE_WZ4FRLIB_WZ4_MODMTRL_HPP

#include "base/types.hpp"
#include "wz4frlib/wz4_mtrl2.hpp"

class ModMtrl;
class MtrlModule;
class ShaderCreator;

/****************************************************************************/

enum ModMtrlEnum
{
  // request from the pixel shader
  MMV_WS_Pos          = 0x00000001, // worldspace
  MMV_WS_Normal       = 0x00000002,
  MMV_WS_Tangent      = 0x00000004,
  MMV_WS_Bitangent    = 0x00000008,

  MMV_UV0             = 0x00000010, // nospace
  MMV_UV1             = 0x00000020,

  MMP_Pre             = 1,          // phases for the shader
  MMP_Light,
  MMP_Shadow,
  MMP_Shader,
  MMP_Post,
  MMP_TexLD,                      // do this last, because the PS/VS code must be processed AFTER the Get() call

  MMF_Fog             = 0x00000001, // features (shader enabled from environment)
  MMF_GroundFog       = 0x00000002,
  MMF_ClipPlanes      = 0x00000004,

  MMFF_FogIsBlack     = 0x00000001, // feature flags
  MMFF_ShadowCastOff  = 0x00000002,
  MMFF_AmbientOff     = 0x00000004,
  MMFF_SwapFog        = 0x00000008,
  MMFF_EmissiveScreen = 0x00000010,
  MMFF_EmissiveNoRim  = 0x00000040,
  MMFF_EmissiveNoCenter=0x00000080,
  MMFF_FogAlpha       = 0x00000100,
  MMFF_ShadowCastToAll= 0x00000200, // cast shadows only in my own light environment, not just all of them
  MMFF_NormalI4       = 0x00000400, // use sVF_I4 for normals instead of sVF_F3
  MMFF_DoubleSidedLight = 0x000800, // use doublesided lighting

  MM_MaxLight         = 8,          // some more constants
  MM_MaxT             = 10,
};

struct ModShaderVEnv
{
  sVector4 para[28];
  static const sInt RegStart = 0;
  static const sInt RegCount = 28;
  static const sInt Slot = sCBUFFER_VS|0;
};
struct ModShaderPEnv
{
  sVector4 para[32];
  static const sInt RegStart = 0;
  static const sInt RegCount = 32;
  static const sInt Slot = sCBUFFER_PS|0;
};


struct ModLightEnv                // light environment
{
  sCBuffer<ModShaderVEnv> cbv;
  sCBuffer<ModShaderPEnv> cbp;
  sCBuffer<Wz4MtrlModelPara> cbm;

  void Init();
  void Calc(sViewport &view);
  ModLightEnv();

  // parameters modified by operator

  sF32 FogAdd;
  sF32 FogMul;
  sF32 FogDensity;
  sVector30 FogColor;

  sF32 GroundFogAdd;
  sF32 GroundFogMul;
  sF32 GroundFogDensity;
  sVector30 GroundFogColor;
  sVector4 ws_GroundFogPlane;
  sVector4 cs_GroundFogPlane;

  sVector4 Clip0;
  sVector4 Clip1;
  sVector4 Clip2;
  sVector4 Clip3;
  sVector4 cs_Clip0;
  sVector4 cs_Clip1;
  sVector4 cs_Clip2;
  sVector4 cs_Clip3;

  // light

  sVector30 Ambient;
  struct Light
  {
    sVector30 ColFront;
    sVector30 ColMiddle;
    sVector30 ColBack;
    sVector31 ws_Pos;
    sVector30 ws_Dir;
    sF32 Range;
    sF32 Inner;
    sF32 Outer;
    sF32 Falloff;
    sInt SM_Size;
    sF32 SM_BaseBias;
    sF32 SM_Filter;
    sF32 SM_SlopeBias;

    sF32 SM_BaseBiasOverClipFar;
    sF32 SM_SlopeBiasOverClipFar;

    sTexture2D *ShadowMap;
    sTextureCube *ShadowCube;
    sMatrix44 LightProj;
    sVector4 LightMatZ;
    sF32 LightFarR;

    // gathering information about bboxes

    sAABBox ShadowReceiver;

    // information for headlights

    sInt Mode;
    sVector31 ws_Pos_;
    sVector30 ws_Dir_;
  } Lights[MM_MaxLight];

  sVector31 LimitShadowCenter;
  sVector30 LimitShadowRadius;
  sInt Features;
  sU8 DirLight;
  sU8 PointLight;
  sU8 SpotLight;
  sU8 Shadow;                     // any shadow at all=
  sU8 ShadowOrd;                  // ordered filtering for shadow
  sU8 ShadowRand;                 // random filtering for shadow
  sU8 SpotInner;
  sU8 SpotFalloff;
  sU8 RequireShadow;              // or'ed killshadow flags, see if ALL materials opt out of this shadow!
  sU8 LimitShadow;                // limit shadowmaps of these directional lights
  sU8 LimitShadowFlags;           // limit to light or dark
  sAABBox ShadowCaster;           // bounding box for shadow casters that only cast into this le. there is another global ShadowCaster BBox

  // misc

  sVector30 Color[8];
  sVector4 Vector[8];
  sMatrix34CM Matrix[4];

  // internal

  sMatrix44 LightProj[sMTRL_MAXTEX];
  sVector4 LightMatZ[sMTRL_MAXTEX];
  sF32 LightFarR[sMTRL_MAXTEX];
  sMatrix44 ws_ss;                // world space to screen space
  sMatrix34CM ws_cs;
  sVector31 ws_campos;            // camera position
  sMatrix34 cs_ws;
  sF32 ClipFarR;                  // clip far reciproce
  sVector4 RandomDisc[8];         // 8 pairs of 2d-vectors that reside in unit-circle

  sF32 ZShaderSlopeBias;
  sF32 ZShaderBaseBiasOverClipFar;
  sF32 ZShaderProjA;
  sF32 ZShaderProjB;

  sF32 ShellExtrude;
};

struct CachedModInfo
{
  CachedModInfo();
  CachedModInfo(sInt matmode,class ModMtrl *,sInt lightenv);
  sInt Features;
  sInt FeatureFlags;
  sU8 LightEnv;
  sU8 MatrixMode;
  sU8 RenderTarget;
  sU8 DirLight;
  sU8 PointLight;
  sU8 SpotLight;
  sU8 Shadow;
  sU8 ShadowOrd;
  sU8 ShadowRand;
  sU8 SpotInner;
  sU8 SpotFalloff;
};

bool operator == (const CachedModInfo &a,const CachedModInfo &b);

/****************************************************************************/
/***                                                                      ***/
/***   Parameter Assignment                                               ***/
/***                                                                      ***/
/****************************************************************************/

class ModMtrlParaAssign
{
  struct ModPara
  {
    sInt SourceOffset;
    sInt DestOffset;
    sInt Count;
  };

  sInt *Alloc;                    // each float4 has a count 0=empty, 4=full
  sInt MapEnd;                    // first free index in map
  sInt MapMax;
  sInt Error;
  sArray<ModPara> CopyLoop;       // copy from ModLightPara to constant buffer

public:

  ModMtrlParaAssign();
  ~ModMtrlParaAssign();
  void Init(sInt max);            // start working and set max float4's to handle
  sInt Assign(sInt count,sInt index);
  void Copy(sU32 *src,sU32 *dest);

  sInt GetSize() { return MapEnd; }
  sInt GetError() { return Error; }
  sInt GetAlloc(sInt r) { return Alloc[r]; }
};

/****************************************************************************/
/***                                                                      ***/
/***   A shader permutation in the cache                                  ***/
/***                                                                      ***/
/****************************************************************************/

struct UpdateTexInfo
{
  sInt Enable;
  sInt Light;
  sInt Mode;
  sTextureBase *Save;
};

class CachedModShader
{
public:
  CachedModShader();
  ~CachedModShader();

  CachedModInfo Info;
  sTextBuffer Code;
  class ModMtrl2 *Shader;
  sVertexFormatHandle *Format;
  ModMtrlParaAssign VSPara;
  ModMtrlParaAssign PSPara;
  UpdateTexInfo UpdateTex[sMTRL_MAXTEX];
};

/****************************************************************************/
/***                                                                      ***/
/***   Modules                                                            ***/
/***                                                                      ***/
/****************************************************************************/

class MtrlModule                                  // the real thing
{
protected:
  virtual ~MtrlModule() {}
  sInt RefCount;
public:
  MtrlModule() { RefCount = 1; Phase=0; Temp=0; Shaders=3; Name=0; }

  virtual void Start(ShaderCreator *sc) {}        // is called once at start. useful to prepare for being called with Get()
  virtual void VS(ShaderCreator *sc) {}           // is called to emit VS
  virtual void PS(ShaderCreator *sc) {}           // is called to emit PS
  virtual void Post(ShaderCreator *sc) {}         // is called after all modules have emitted VS / PS. for Get()
  virtual sPoolString Get(ShaderCreator *sc)      // can be called by other modules. 
  { return sPoolString(L"float4(1,1,1,1)"); }     // you can use the post phase to add more code
  const sChar *Name;
  sInt Phase;
  sInt Temp;
  sInt Shaders;                                   // bit0: VS   bit1: PS

  void AddRef()    { if(this) RefCount++; }
  void Release()   { if(this) { if(--RefCount<=0) delete this; } }
};

class ModShader : public wObject  // wz wrapper
{ 
public:
  sArray<MtrlModule *> Modules;

  ModShader();
  ~ModShader();

  void Add(ModShader *);
  void Add(MtrlModule *);
};

/****************************************************************************/
/***                                                                      ***/
/***   The Material Itself                                                ***/
/***                                                                      ***/
/****************************************************************************/

class ModMtrl : public Wz4Mtrl
{
  sArray<CachedModShader *>Cache;

  sInt Flags;
  sU32 BlendColor;
  sU32 BlendAlpha;
  sInt AlphaTest;
  sInt AlphaRef;

  CachedModShader *CreateShader(const CachedModInfo &info);
  CachedModShader *FindShader(const CachedModInfo &info);
public:
  ModMtrl();
  ~ModMtrl();
  void SetMtrl(sInt flags=sMTRL_ZON|sMTRL_CULLON,sU32 blend=0,sU32 blenda=0);
  void SetAlphaTest(sInt test,sInt ref);
  sU8 KillLight;

  sU8 KillShadow;
  sInt KillFeatures;
  sInt FeatureFlags;

  // use material

  void Set(sInt flags,sInt index,const sMatrix34CM *mat,sInt SkinMatCount,const sMatrix34CM *SkinMats,sInt *SkinMatMap);
  void Prepare();
  void BeforeFrame(sInt lightenv,sInt boxcount,const sAABBoxC *boxes,sInt matcount,const sMatrix34CM *matrices);


  sVertexFormatHandle *GetFormatHandle(sInt flags);
  sBool SkipPhase(sInt flags,sInt lightenv);

  sArray<MtrlModule *> ModulesUser;         // modules registered by user
  sArray<MtrlModule *> ModulesTotal;        // plus some modules from system

  sBool Error;
  sTextBuffer ShaderLog;

  wDocName PageName;
  sInt PageX;
  sInt PageY;
};

/****************************************************************************/

#endif // FILE_WZ4FRLIB_WZ4_MODMTRL_HPP

