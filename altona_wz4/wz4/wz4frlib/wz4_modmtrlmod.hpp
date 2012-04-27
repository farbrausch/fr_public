/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_WZ4_MODMTRLMOD_HPP
#define FILE_WZ4FRLIB_WZ4_MODMTRLMOD_HPP

#include "base/types2.hpp"
#include "wz4frlib/wz4_modmtrl.hpp"
#include "wz4frlib/wz4_modmtrl_ops.hpp"
#include "wz4frlib/wz4_modmtrlsc.hpp"

/****************************************************************************/

class MM_DirLight : public MtrlModule
{
public:
  sInt Select;
  MM_DirLight();
  void PS(ShaderCreator *sc);
};


class MM_PointLight : public MtrlModule
{
public:
  sInt Select;
  MM_PointLight();
  void PS(ShaderCreator *sc);
};

class MM_SpotLight : public MtrlModule
{
public:
  sInt Select;
  sInt Inner;
  sInt Falloff;
  sInt Flags[MM_MaxLight];
  MM_SpotLight();
  void PS(ShaderCreator *sc);
};

/****************************************************************************/

class MM_Flat : public MtrlModule
{
public:
  sVector30 Color;
  
  MM_Flat();
  void PS(ShaderCreator *sc);
};

class MM_Lambert : public MtrlModule
{
public:
  sInt LightFlags;

  MM_Lambert();
  void PS(ShaderCreator *sc);
};

class MM_Phong : public MtrlModule
{
public:

  sF32 Specular;
  sF32 Transpar;
  sInt LightFlags;
  sInt SpecularityMap;
  sInt TransparencyMap;
  MtrlModule *Tex;
  sInt TextureFlags;

  MM_Phong();
  void PS(ShaderCreator *sc);
};

class MM_Rim : public MtrlModule
{
public:
  sInt Flags;
  sF32 Width;
  sVector4 Color;
  sInt Shadow;

  MM_Rim();
  ~MM_Rim();
  void PS(ShaderCreator *sc);
};

class MM_Comic : public MtrlModule
{
public:
  sInt Mode;
  Texture2D *Tex;

  MM_Comic();
  ~MM_Comic();
  void PS(ShaderCreator *sc);
};

class MM_BlinnPhong : public MtrlModule
{
public:

  sF32 Specular;
  sInt LightFlags;
  sInt SpecularityMap;
  MtrlModule *Tex;

  MM_BlinnPhong();
  void PS(ShaderCreator *sc);
};

/****************************************************************************/

class MM_Tex2D : public MtrlModule
{
public:
  sInt Flags;
  sInt UV;
  sInt Transform;
  sInt Aux;
  sF32 SimpleScale;
  sVector31 Scale;
  sVector30 Rot;
  sVector31 Trans;
  Texture2D *Texture;
  sInt SimpleCase;

  const sChar *uvname;
  const sChar *VarName;

  MM_Tex2D();
  ~MM_Tex2D();

  void Start(ShaderCreator *sc);
  sPoolString Get(ShaderCreator *sc);

  void VS(ShaderCreator *sc);
};

class MM_Tex2DAnim : public MtrlModule
{
public:
  sInt Flags;
  sInt UV;
  sInt Transform;
  sInt Aux;
  sInt AtlasAnim;
  sF32 SimpleScale;
  sVector31 Scale;
  sVector30 Rot;
  sVector31 Trans;
  Texture2D *Texture;
  

  const sChar *uvname;
  const sChar *VarName;

  MM_Tex2DAnim();
  ~MM_Tex2DAnim();

  void Start(ShaderCreator *sc);
  sPoolString Get(ShaderCreator *sc);

  void VS(ShaderCreator *sc);
};

class MM_Tex2DSurround : public MtrlModule
{
public:
  sInt Flags;
  Texture2D *Texture;
  sInt Aux;
  sPoolString VarName;

  sInt Transform;
  sInt Space;
  sF32 SimpleScale;
  sVector31 Scale;
  sVector30 Rot;
  sVector31 Trans;
  sVector30 MajorAxis;

  MM_Tex2DSurround();
  ~MM_Tex2DSurround();

  void Start(ShaderCreator *sc);
  sPoolString Get(ShaderCreator *sc);
};

class MM_Tex2DSurroundNormal : public MtrlModule
{
public:
  sInt Flags;
  Texture2D *Texture;

  sInt Transform;
  sInt Space;
  sF32 SimpleScale;
  sVector31 Scale;
  sVector30 Rot;
  sVector31 Trans;
  sVector30 MajorAxis;

  MM_Tex2DSurroundNormal();
  ~MM_Tex2DSurroundNormal();

  void Start(ShaderCreator *sc);
  void PS(ShaderCreator *sc);
};

/****************************************************************************/

class MM_Shadow : public MtrlModule
{
public:
  sInt Slot[MM_MaxLight];
  sInt Mode[MM_MaxLight];
  sU8 Shadow;
  sU8 ShadowOrd;
  sU8 ShadowRand;

  MM_Shadow();
  void PS(ShaderCreator *sc);
  void VS(ShaderCreator *sc);
};

class MM_ShadowCube : public MtrlModule
{
public:
  sInt Slot[MM_MaxLight];
  sInt Mode[MM_MaxLight];

  MM_ShadowCube();
  void PS(ShaderCreator *sc);
  void VS(ShaderCreator *sc);
};

/****************************************************************************/

class MM_Fog : public MtrlModule
{
public:
  sInt FeatureFlags;
  MM_Fog();
  void PS(ShaderCreator *sc);
};

class MM_GroundFog : public MtrlModule
{
public:
  sInt FeatureFlags;
  MM_GroundFog();
  void PS(ShaderCreator *sc);
};

/****************************************************************************/

class MM_ClipPlanes : public MtrlModule
{
public:
  sInt Planes;

  MM_ClipPlanes();
  void PS(ShaderCreator *sc);
};

/****************************************************************************/

class MM_Const : public MtrlModule
{
public:
  sInt Source;
  sVector4 Color;
  const sChar *VarName;

  MM_Const();

  void Start(ShaderCreator *sc);
  sPoolString Get(ShaderCreator *sc);
};

class MM_TrickLight : public MtrlModule
{
public:
  sInt LightFlags;
  sInt LightEnable;
  const sChar *VarName;

  MM_TrickLight();

  void Start(ShaderCreator *sc);
  sPoolString Get(ShaderCreator *sc);
};

class MM_Math : public MtrlModule
{
public:
  sInt Mode;
  const sChar *VarName;
  sArray<MtrlModule *> Inputs;
  
  MM_Math();

  void Start(ShaderCreator *sc);
  sPoolString Get(ShaderCreator *sc);
};

class MM_ApplyTexture : public MtrlModule
{
public:
  MtrlModule *Tex;
  const sChar *Dest;
  sInt Op;
  sInt Aux;
  sInt InputRemap; // 0=no remap, 1=saturate(x*2-1) (used for wz3 style normalmaps)
  const sChar *Swizzle;
  
  MM_ApplyTexture();
  void PS(ShaderCreator *sc);
};

class MM_Kill : public MtrlModule
{
public:
  MtrlModule *Tex;
  sF32 ThreshLo;
  sF32 ThreshHi;
  sInt Invert;
  sInt Channel;
  
  MM_Kill();
  void PS(ShaderCreator *sc);
};

/****************************************************************************/

class MM_Displace : public MtrlModule
{
public:
  const sChar *Swizzle;
  MtrlModule *Tex;
  sF32 Scale;
  sF32 Bias;
  sInt Source;

  MM_Displace();
  void VS(ShaderCreator *sc);
};

class MM_ExtrudeNormal : public MtrlModule
{
public:
  sInt Source;
  sF32 Value;

  MM_ExtrudeNormal();
  void VS(ShaderCreator *sc);
};

class MM_NormalMap : public MtrlModule
{
public:
  MtrlModule *Tex;
  
  MM_NormalMap();
  void PS(ShaderCreator *sc);
};

class MM_DoubleNormalMap : public MtrlModule
{
public:
  MtrlModule *Tex0;
  MtrlModule *Tex1;
  sInt Flags;

  MM_DoubleNormalMap();
  void PS(ShaderCreator *sc);
};

class MM_BakedShadow : public MtrlModule
{
  sMatrix44 LightProj;
  sVector4 LightMatZ;
  sF32 LightFarZ;
  sF32 LightFarR;
  sF32 ZShaderSlopeBias;
  sF32 ZShaderBaseBiasOverClipFar;

public:
  Texture2D *Tex;
  sVector30 Dir;
  sInt ShadowSize;
  sF32 ShadowBaseBias;
  sF32 ShadowFilter;
  sF32 ShadowSlopeBias;
  const sChar *VarName;
  sVector4 Color;
  sVector31 LimitCenter;
  sVector30 LimitRadius;

  MM_BakedShadow();
  ~MM_BakedShadow();
  void Render(Wz4Mesh *scene);

  void Start(ShaderCreator *sc);
  sPoolString Get(ShaderCreator *sc);

  void VS(ShaderCreator *sc);
};

/****************************************************************************/
/****************************************************************************/


#endif // FILE_WZ4FRLIB_WZ4_MODMTRLMOD_HPP

