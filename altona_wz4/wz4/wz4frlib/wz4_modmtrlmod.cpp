/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "wz4_modmtrlmod.hpp"
#include "wz4_demo2nodes.hpp"

/****************************************************************************/

MM_DirLight::MM_DirLight()
{
  Phase = MMP_Light;
  Name = L"dir light";
  Select = 0;
  Shaders = 2;
}

void MM_DirLight::PS(ShaderCreator *sc)
{
  if(Select)
  {
    sc->FragBegin(Name);
    sc->FragModify(L"ws_lightdir");
    sc->FragModify(L"lighti");

    for(sInt i=0;i<MM_MaxLight;i++)
    {
      if(Select & (1<<i))
      {
        sc->Para(sPoolF(L"ws_LightDir%d",i));
        sc->TB.PrintF(L"  ws_lightdir[%d] = ws_LightDir%d;\n",i,i);
        sc->TB.PrintF(L"  lighti[%d] = 1;\n",i);
        sc->LightMask |= 1<<i;
      }
    }

    sc->FragEnd();
  }
}

/****************************************************************************/

MM_PointLight::MM_PointLight()
{
  Phase = MMP_Light;
  Name = L"point light";
  Select = 0;
  Shaders = 2;
}

void MM_PointLight::PS(ShaderCreator *sc)
{
  if(Select)
  {
    sc->FragBegin(Name);
    sc->InputPS(L"ws_pos",SCT_FLOAT3);
    sc->FragModify(L"ws_lightdir");
    sc->FragModify(L"lighti");

    sc->TB.PrintF(L"  { float3 delta; float r,dist;\n");
    for(sInt i=0;i<MM_MaxLight;i++)
    {
      if(Select & (1<<i))
      {
        sc->Para(sPoolF(L"ws_LightPos%d",i));
        sc->Para(sPoolF(L"LightRange%d",i));
        sc->TB.PrintF(L"    delta = ws_LightPos%d-ws_pos;\n",i);
        sc->TB.PrintF(L"    r = rsqrt(dot(delta,delta));\n");
        sc->TB.PrintF(L"    dist = 1/(r*LightRange%d);\n",i);
        sc->TB.PrintF(L"    ws_lightdir[%d] = r*delta;\n",i,i);
        sc->TB.PrintF(L"    lighti[%d] = saturate(1-dist);\n",i);
        sc->LightMask |= 1<<i;
      }
    }
    sc->TB.PrintF(L"  }\n");
    sc->FragEnd();
  }
}


/****************************************************************************/


MM_SpotLight::MM_SpotLight()
{
  Phase = MMP_Light;
  Name = L"spot light";
  Select = 0;
  sClear(Flags);
  Shaders = 2;
}

void MM_SpotLight::PS(ShaderCreator *sc)
{
  if(Select)
  {
    sc->FragBegin(Name);
    sc->FragModify(L"ws_lightdir");
    sc->FragModify(L"lighti");
    sc->InputPS(L"ws_pos",SCT_FLOAT3);

    sc->TB.PrintF(L"  { float3 delta; float r,dist,s,l;");
    for(sInt i=0;i<MM_MaxLight;i++)
    {
      if(Select & (1<<i))
      {
        sc->Para(sPoolF(L"ws_LightPos%d",i));
        sc->Para(sPoolF(L"ws_LightDir%d",i));
        sc->Para(sPoolF(L"LightRange%d",i));
        if(Inner & (1<<i))
          sc->Para(sPoolF(L"LightInner%d",i));
        sc->Para(sPoolF(L"LightOuter%d",i));
        if(Falloff & (1<<i))
          sc->Para(sPoolF(L"LightFalloff%d",i));
        sc->TB.PrintF(L"    delta = ws_LightPos%d-ws_pos;\n",i);
        sc->TB.PrintF(L"    r = rsqrt(dot(delta,delta));\n");
        sc->TB.PrintF(L"    dist = 1/(r*LightRange%d);\n",i);
        sc->TB.PrintF(L"    ws_lightdir[%d] = r*delta;\n",i,i);
        sc->TB.PrintF(L"    l = saturate(dot(ws_lightdir[%d],ws_LightDir%d));\n",i,i);
        if(Inner & (1<<i))
          sc->TB.PrintF(L"    s = saturate((l-LightOuter%d)/(LightInner%d-LightOuter%d));\n",i,i,i);
        else
          sc->TB.PrintF(L"    s = saturate((l-LightOuter%d)/(1-LightOuter%d));\n",i,i);
        if(Falloff & (1<<i))
          sc->TB.PrintF(L"    s = pow(s,LightFalloff%d);\n",i);
        sc->TB.PrintF(L"    lighti[%d] = saturate(1-dist)*s;\n",i);
        sc->LightMask |= 1<<i;
      }
    }
    sc->TB.PrintF(L"  }\n");
    sc->FragEnd();
  }
}

/****************************************************************************/
/****************************************************************************/

MM_Flat::MM_Flat()
{
  Phase = MMP_Shader;
  Name = L"flat shader";
  Color.Init(1,1,1);
  Shaders = 2;
}

void MM_Flat::PS(ShaderCreator *sc)
{
  sc->FragBegin(Name);
  sc->TB.PrintF(L"  col_light += float3(%f,%f,%f);\n",Color.x,Color.y,Color.z);
  sc->FragModify(L"col_light");
  sc->FragEnd();
}

/****************************************************************************/

MM_Lambert::MM_Lambert()
{
  Phase = MMP_Shader;
  Name = L"lambert shader";
  LightFlags = 1;
  Shaders = 2;
}

void MM_Lambert::PS(ShaderCreator *sc)
{
  sc->FragBegin(Name);
  sc->Para(L"Ambient");
  sc->FragModify(L"col_light");
  sc->InputPS(L"ws_normal",SCT_FLOAT3);     // vertex format may not change depending on light on/off

  if(!(sc->Mtrl->FeatureFlags & MMFF_AmbientOff))
    sc->TB.PrintF(L"  col_light += Ambient;\n");

  if(sc->LightMask && LightFlags)
  {
    sc->FragRead(L"ws_lightdir");
    sc->FragRead(L"lightshadow");
    sc->FragRead(L"lighti");

    sc->TB.PrintF(L"  { float l,li,ls;\n");
    sc->TB.PrintF(L"    float3 ws_normal_ds = ws_normal;\n");
    sc->DoubleSided(L"ws_normal_ds");
    for(sInt i=0;i<MM_MaxLight;i++)
    {
      if(sc->LightMask & (1<<i))
      {
        sc->Para(sPoolF(L"LightColFront%d",i));
        sc->Para(sPoolF(L"LightColMiddle%d",i));
        sc->Para(sPoolF(L"LightColBack%d",i));
        sc->TB.PrintF(L"    li = lighti[%d];\n",i);
        sc->TB.PrintF(L"    ls = lightshadow[%d];\n",i);
        sc->TB.PrintF(L"    l = (dot(ws_lightdir[%d],normalize(ws_normal_ds)));\n",i);
        if(LightFlags&1)
          sc->TB.PrintF(L"    col_light += saturate(l)*lerp(LightColBack%d,LightColFront%d,ls)*li;\n",i,i);
        if(LightFlags&2)
          sc->TB.PrintF(L"    col_light += saturate(1-abs(l))*LightColMiddle%d*li;\n",i);
        if(LightFlags&4)
          sc->TB.PrintF(L"    col_light += saturate(-l)*LightColBack%d*li;\n",i);
      }
    }
    sc->TB.PrintF(L"  }\n");
  }

  sc->FragEnd();
}

/****************************************************************************/

MM_Phong::MM_Phong()
{
  Phase = MMP_Shader;
  Name = L"phong shader";
  Specular = 32;
  Transpar = 1;
  LightFlags = 9;
  SpecularityMap = 3;
  TransparencyMap = 3;
  TextureFlags = 1;
  Tex = 0;
  Shaders = 2;
}

void MM_Phong::PS(ShaderCreator *sc)
{
  const sChar *texname = 0;
  if(Tex)
    texname = Tex->Get(sc);

  sc->FragBegin(Name);
  sc->Para(L"Ambient");
  sc->InputPS(L"ws_normal",SCT_FLOAT3);
  sc->FragModify(L"col_light");

  if(!(sc->Mtrl->FeatureFlags & MMFF_AmbientOff))
    sc->TB.PrintF(L"  col_light += Ambient;\n");

  if(sc->LightMask && LightFlags)
  {
    sc->InputPS(L"ws_eye",SCT_FLOAT3);
    sc->Require(L"ws_normal",SCT_FLOAT3);
    sc->FragRead(L"ws_lightdir");
    sc->FragRead(L"lighti");
    sc->FragRead(L"lightshadow");

    if(texname)
      sc->FragRead(texname);

    sc->TB.PrintF(L"  { float l,s,li,ls; float3 refl;\n");
    sc->TB.PrintF(L"    float3 ws_normal_ds = ws_normal;\n");
    if(sc->Mtrl->FeatureFlags & MMFF_DoubleSidedLight)
    {
      sc->BindPS(L"faceflag",SCT_FLOAT,SCB_VFACE);
      sc->TB.PrintF(L"  if(faceflag<0) ws_normal_ds = -ws_normal_ds;\n");
    }
    const sChar *sw=L"rgba";
    if(texname && (TextureFlags & 1))
      sc->TB.PrintF(L"    float spec = %s.%c*%f;\n",texname,sw[SpecularityMap&3],Specular);
    else
      sc->TB.PrintF(L"    float spec = %f;\n",Specular);
    if(texname && (TextureFlags & 2))
      sc->TB.PrintF(L"    float tran = %s.%c*%f;\n",texname,sw[TransparencyMap&3],Transpar);
    else
      sc->TB.PrintF(L"    float tran = %f;\n",Transpar);
    if(texname && (TextureFlags & 4))
      sc->TB.PrintF(L"    float3 speccol = %s.xyz;\n",texname);
    else
      sc->TB.PrintF(L"    float3 speccol = 1;\n");

    for(sInt i=0;i<MM_MaxLight;i++)
    {
      if(sc->LightMask & (1<<i))
      {
        sc->Para(sPoolF(L"LightColFront%d",i));
        sc->Para(sPoolF(L"LightColMiddle%d",i));
        sc->Para(sPoolF(L"LightColBack%d",i));
        sc->TB.PrintF(L"    li = lighti[%d];\n",i);
        sc->TB.PrintF(L"    ls = lightshadow[%d];\n",i);
        if(LightFlags&7)
          sc->TB.PrintF(L"    l = (dot(ws_lightdir[%d],normalize(ws_normal_ds)));\n",i);
        if(LightFlags&1)
          sc->TB.PrintF(L"    col_light += saturate(l)*lerp(LightColBack%d,LightColFront%d,ls)*li;\n",i,i);
        if(LightFlags&2)
          sc->TB.PrintF(L"    col_light += saturate(1-abs(l))*LightColMiddle%d*li;\n",i);
        if(LightFlags&4)
          sc->TB.PrintF(L"    col_light += saturate(-l)*LightColBack%d*li;\n",i);

        if(LightFlags&8)
        {
          sc->TB.PrintF(L"    refl = reflect(ws_lightdir[%d],normalize(ws_normal_ds));\n",i);
          sc->TB.PrintF(L"    s = saturate(dot(refl,-normalize(ws_eye)));\n");
          sc->TB.PrintF(L"    col_specular += (pow(s,spec)*li*ls*tran)*speccol;\n");
        }
      }
    }
    sc->TB.PrintF(L"  }\n");
  }
  sc->FragEnd();
}

/****************************************************************************/

MM_Rim::MM_Rim()
{
  Phase = MMP_Shader;
  Name = L"Rimlight shader";
  Flags = 1;
  Width = 0.5f;
  Shadow = 0;
  Color.Init(1,1,1,1);
}

MM_Rim::~MM_Rim()
{
}

void MM_Rim::PS(ShaderCreator *sc)
{
  sc->FragBegin(Name);
  sc->InputPS(L"ws_normal",SCT_FLOAT3);     // vertex format may not change depending on light on/off
  sc->InputPS(L"ws_eye",SCT_FLOAT3);

  sc->TB.Print(L"  {\n");
  sc->TB.PrintF(L"    float3 ws_normal_ds = ws_normal;\n");
  if((Flags & sMTRL_CULLMASK) == sMTRL_CULLINV)
    sc->TB.PrintF(L"  ws_normal_ds = -ws_normal_ds;\n");
  sc->DoubleSided(L"ws_normal_ds");
  sc->TB.PrintF(L"    float d = max(0,1-dot(normalize(ws_normal_ds),normalize(ws_eye))*%f);\n",1.0f/sMax(0.001f,Width));
  if(Flags & 1)
    sc->TB.Print(L"    d = smoothstep(0,1,d);\n");
  if(Shadow)
  {
    sc->FragRead(L"lightshadow");
    for(sInt i=0;i<8;i++)
    {
      if(Shadow & (1<<i))
      {
        sc->TB.PrintF(L"    d *= lightshadow[%d];\n",i);
      }
    }
  }
  if(Flags & 2)
  {
    sc->FragModify(L"col_light");
    sc->TB.PrintF(L"    col_light += float3(%f,%f,%f)*d;\n",Color.x,Color.y,Color.z);
  }
  else
  {
    sc->FragModify(L"col_emissive");
    sc->TB.PrintF(L"    col_emissive += float3(%f,%f,%f)*d;\n",Color.x,Color.y,Color.z);
  }
  sc->TB.Print(L"  }\n");

  sc->FragEnd();
}

/****************************************************************************/

MM_Comic::MM_Comic()
{
  Phase = MMP_Shader;
  Name = L"comic shader";
  Tex = 0;
  Shaders = 2;
}

MM_Comic::~MM_Comic()
{
  Tex->Release();
}

void MM_Comic::PS(ShaderCreator *sc)
{
  sc->FragBegin(Name);
  sc->Para(L"Ambient");
  sc->FragModify(L"col_light");
  sc->InputPS(L"ws_normal",SCT_FLOAT3);     // vertex format may not change depending on light on/off

  if(!(sc->Mtrl->FeatureFlags & MMFF_AmbientOff))
    sc->TB.PrintF(L"  col_light += Ambient;\n");

  if(sc->LightMask)
  {
    sc->FragRead(L"ws_lightdir");
    sc->FragRead(L"lightshadow");
    sc->FragRead(L"lighti");
    sc->InputPS(L"ws_eye",SCT_FLOAT3);

    sc->TB.PrintF(L"  { float l,li,ls; float2 uv;float3 refl,tcol;\n");
    sc->TB.PrintF(L"    float3 ws_normal_ds = ws_normal;\n");
    sc->DoubleSided(L"ws_normal_ds");
    sInt slot = sc->Texture(Tex,sMTF_LEVEL3|sMTF_CLAMP);
    for(sInt i=0;i<MM_MaxLight;i++)
    {
      if(sc->LightMask & (1<<i))
      {
        sc->Para(sPoolF(L"LightColFront%d",i));

        sc->TB.PrintF(L"    li = lighti[%d];\n",i);
        sc->TB.PrintF(L"    ls = lightshadow[%d];\n",i);
        sc->TB.PrintF(L"    uv.x = (dot(ws_lightdir[%d],normalize(ws_normal_ds))*0.5+0.5);\n",i);
        if(Mode)
        {
          sc->TB.PrintF(L"    refl = reflect(ws_lightdir[%d],normalize(ws_normal_ds));\n",i);
          sc->TB.PrintF(L"    uv.y = saturate(dot(refl,-normalize(ws_eye))*0.5+0.5);\n");
        }
        else
        {
          sc->TB.Print(L"    uv.y = 0;\n");
        }

        sc->TB.PrintF(L"    tcol = %s;\n",Tex2D(slot,L"uv.xy",L"xyz",0));
        sc->TB.PrintF(L"    col_light += tcol * LightColFront%d;\n",i);
      }
    }
    sc->TB.PrintF(L"  }\n");
  }

  sc->FragEnd();
}

/****************************************************************************/

MM_BlinnPhong::MM_BlinnPhong()
{
  Phase = MMP_Shader;
  Name = L"blinn-phong shader";
  Specular = 32;
  LightFlags = 9;
  SpecularityMap = 3;
  Tex = 0;
  Shaders = 2;
}

void MM_BlinnPhong::PS(ShaderCreator *sc)
{
  const sChar *texname = 0;
  if(Tex)
    texname = Tex->Get(sc);

  sc->FragBegin(Name);
  sc->Para(L"Ambient");
  sc->InputPS(L"ws_normal",SCT_FLOAT3);
  sc->FragModify(L"col_light");

  if(!(sc->Mtrl->FeatureFlags & MMFF_AmbientOff))
    sc->TB.PrintF(L"  col_light += Ambient;\n");

  if(sc->LightMask && LightFlags)
  {
    sc->InputPS(L"ws_eye",SCT_FLOAT3);
    sc->Require(L"ws_normal",SCT_FLOAT3);
    sc->FragRead(L"ws_lightdir");
    sc->FragRead(L"lighti");
    sc->FragRead(L"lightshadow");

    if(texname)
      sc->FragRead(texname);

    sc->TB.PrintF(L"  { float l,s,li,ls; float3 halfway;\n");
    sc->TB.PrintF(L"    float3 ws_normal_ds = ws_normal;\n");
    sc->DoubleSided(L"ws_normal_ds");
    const sChar *specsw;
    switch(SpecularityMap)
    {
    case 0: specsw = L"r"; break;
    case 1: specsw = L"g"; break;
    case 2: specsw = L"b"; break;
    default:
    case 3: specsw = L"a"; break;
    }
    if(texname)
      sc->TB.PrintF(L"    float spec = %s.%s*%f;\n",texname,specsw,Specular);
    else
      sc->TB.PrintF(L"    float spec = %f;\n",Specular);

    for(sInt i=0;i<MM_MaxLight;i++)
    {
      if(sc->LightMask & (1<<i))
      {
        sc->Para(sPoolF(L"LightColFront%d",i));
        sc->Para(sPoolF(L"LightColMiddle%d",i));
        sc->Para(sPoolF(L"LightColBack%d",i));
        sc->TB.PrintF(L"    li = lighti[%d];\n",i);
        sc->TB.PrintF(L"    ls = lightshadow[%d];\n",i);
        if(LightFlags&7)
          sc->TB.PrintF(L"    l = (dot(ws_lightdir[%d],normalize(ws_normal_ds)));\n",i);
        if(LightFlags&1)
          sc->TB.PrintF(L"    col_light += saturate(l)*lerp(LightColBack%d,LightColFront%d,ls)*li;\n",i,i);
        if(LightFlags&2)
          sc->TB.PrintF(L"    col_light += saturate(1-abs(l))*LightColMiddle%d*li;\n",i);
        if(LightFlags&4)
          sc->TB.PrintF(L"    col_light += saturate(-l)*LightColBack%d*li;\n",i);

        if(LightFlags&8)
        {
          sc->TB.PrintF(L"    halfway = normalize(ws_lightdir[%d] + normalize(ws_eye));\n",i);
          sc->TB.PrintF(L"    s = saturate(dot(halfway,normalize(ws_normal_ds)));\n");
          sc->TB.PrintF(L"    col_specular += pow(s,spec)*ls*li;\n",specsw);
        }
      }
    }
    sc->TB.PrintF(L"  }\n");
  }
  sc->FragEnd();
}

/****************************************************************************/
/****************************************************************************/

MM_Tex2D::MM_Tex2D()
{
  Phase = MMP_TexLD;
  Name = L"texture2D";
  Flags = sMTF_LEVEL2|sMTF_TILE;
  Aux = 0;
  SimpleScale = 0;
  Scale.Init(1,1,1);
  Texture = 0;
  SimpleCase = 0;
  uvname = 0;
  VarName = 0;
}

MM_Tex2D::~MM_Tex2D()
{
  Texture->Release();
}

void MM_Tex2D::Start(ShaderCreator *sc)
{
  VarName = 0;
}

sPoolString MM_Tex2D::Get(ShaderCreator *sc)
{
  if(VarName==0)
  {
    VarName = sc->GetTemp();
    if(UV==0 && Transform==0)
    {
      uvname = L"uv0";
      SimpleCase = 1;
    }
    else if(UV==1 && Transform==0)
    {
      uvname = L"uv1";
      SimpleCase = 1;
    }
    else
    {
      uvname = sc->GetTemp();
      SimpleCase = 0;
    }

    sInt cando = 0;
    sGraphicsCaps caps;
    sGetGraphicsCaps(caps);
    sc->FragBegin(Name);

    switch(UV)
    {
    case 7:
      if(sc->ShaderType==SCS_PS)
        sc->InputPS(L"cs_normal",SCT_FLOAT3);
      else
        sc->Require(L"cs_normal",SCT_FLOAT3);
      sc->TB.PrintF(L"  float2 %s = normalize(cs_normal).xy*float2(0.5,-0.5)+0.5;\n",uvname);
      break;
    case 8:
      if(sc->ShaderType==SCS_PS)
      {
        sc->InputPS(L"cs_normal",SCT_FLOAT3);
        sc->InputPS(L"cs_eye",SCT_FLOAT3);
      }
      else
      {
        sc->Require(L"cs_normal",SCT_FLOAT3);
        sc->Require(L"cs_eye",SCT_FLOAT3);
      }
      sc->TB.PrintF(L"  float2 %s = reflect(normalize(cs_eye),normalize(cs_normal)).xy*float2(-0.5,0.5)+0.5;\n",uvname);
      break;
    default:
      if(sc->ShaderType==SCS_PS)
      {
        sc->InputPS(uvname,SCT_FLOAT2);
      }
      else 
      {
        sc->Require(uvname,SCT_FLOAT2);
      }
      break;
    }

#if sRENDERER!=sRENDER_DX11
    sU64 texmask = 1ULL<<(Texture->Texture->Flags & sTEX_FORMAT);
    if(sc->ShaderType==SCS_PS)
    {
      if(caps.Texture2D & texmask)
        cando = 1;
    }
    else 
    {
      if(caps.VertexTex2D & texmask)
        cando = 1;
    }
#else
    cando = 1;
#endif

    sInt slot = sc->Texture(Texture,Flags);
    if(cando)
    {
      sc->TB.PrintF(L"  float4 %s = %s;\n",VarName,Tex2D(slot,uvname,L"xyzw",Texture,sc));
    }
    else
    {
      sc->TB.PrintF(L"  float4 %s = float4(0,0,0,0); // texture format not supported\n",VarName);
    }
    switch(Aux)
    {
    case 1: sc->TB.PrintF(L"  %s.xyz *= col_aux;\n",VarName);  break;
    case 2: sc->TB.PrintF(L"  %s.xyz *= 1-col_aux;\n",VarName);  break;
    case 3: sc->TB.PrintF(L"  %s.xyz *= saturate(col_aux*2-1);\n",VarName);  break;
    case 4: sc->TB.PrintF(L"  %s.xyz *= saturate(1-col_aux*2);\n",VarName);  break;
    }

    sc->FragFirst(VarName);
    if(Aux)
      sc->FragRead(L"col_aux");

    sc->FragEnd();
  }
  return sPoolString(VarName);
}

void MM_Tex2D::VS(ShaderCreator *sc)
{
  sInt index=-1;
  if(VarName && !SimpleCase && sc->Requires(uvname,index) && UV!=7 && UV!=8)
  {
    const sChar *temp = uvname;
    if(index>=0)
      temp = sc->GetTemp();
    sBool envi = 0;

    sc->FragBegin(Name);
    sc->FragFirst(temp);

    switch(UV)
    {
    case 0:
      sc->Require(L"uv0",SCT_FLOAT2);
      sc->TB.PrintF(L"  float4 %s = float4(uv0,0,1);\n",temp);
      break;
    case 1:
      sc->Require(L"uv1",SCT_FLOAT2);
      sc->TB.PrintF(L"  float4 %s = float4(uv1,0,1);\n",temp);
      break;
    case 2:
      sc->Require(L"ms_pos",SCT_FLOAT3);
      sc->TB.PrintF(L"  float4 %s = float4(ms_pos,1);\n",temp);
      break;
    case 3:
      sc->Require(L"ws_pos",SCT_FLOAT3);
      sc->TB.PrintF(L"  float4 %s = float4(ws_pos,1);\n",temp);
      break;
    case 4:
      sc->Require(L"cs_pos",SCT_FLOAT3);
      sc->TB.PrintF(L"  float4 %s = float4(cs_pos,1);\n",temp);
      break;
    case 5: 
      sc->Require(L"ws_normal",SCT_FLOAT3);
      sc->Para(L"ws_cs");
      sc->TB.PrintF(L"  float4 %s = float4(mul(ws_cs,float4(normalize(ws_normal),0)),1);\n",temp);
      envi = 1;
      break;
    case 6:
      sc->Require(L"cs_reflect",SCT_FLOAT3);
      sc->Para(L"ws_cs");
      sc->TB.PrintF(L"  float4 %s = float4(-cs_reflect,1);\n",temp);
      envi = 1;
      break;
    }
    if(Transform==1)
    {
      sc->TB.PrintF(L"  %s.xyz = %s.xyz*%f;\n",temp,temp,SimpleScale);
    }
    else if(Transform==2)
    {
      sSRT srt;
      srt.Scale = Scale;
//      if(Flags & 0x40000000)
        srt.Rotate = Rot;
//      else
//        srt.Rotate = Rot*(1/sPI2F);
      srt.Translate = Trans;
      sMatrix34 mat;
      srt.MakeMatrix(mat);
        
      sc->TB.PrintF(L"  %s.xy = float2(dot(%s.xyzw,float4(%f,%f,%f,%f)),\n",temp,temp,mat.i.x,mat.j.x,mat.k.x,mat.l.x);
      sc->TB.PrintF(L"                     dot(%s.xyzw,float4(%f,%f,%f,%f)));\n",temp,mat.i.y,mat.j.y,mat.k.y,mat.l.y);
    } 
    else if(Transform>=3 && Transform<7)
    {
      const sChar *mat = sPoolF(L"Matrix%d",Transform-3);
      sc->Para(mat);
      sc->TB.PrintF(L"  %s.xy = mul(%s,%s.xyzw).xy;\n",temp,mat,temp);
    }
    if(envi)
    {
      sc->TB.PrintF(L"  %s.xy = %s.xy*float2(0.5,-0.5)+0.5;\n",temp,temp);
    }

    if(index>=0)
      sc->TB.PrintF(L"  t%d.%s = %s.xy;\n",index/4,sc->swizzle[index&3][2],temp);
    
    sc->FragEnd();
  }
}

/****************************************************************************/


MM_Tex2DAnim::MM_Tex2DAnim()
{
  Phase = MMP_TexLD;
  Name = L"texture2Danim";
  Flags = sMTF_LEVEL2|sMTF_TILE;
  Aux = 0;
  SimpleScale = 0;
  Scale.Init(1,1,1);
  Texture = 0;
  uvname = 0;
  VarName = 0;
}

MM_Tex2DAnim::~MM_Tex2DAnim()
{
  Texture->Release();
}

void MM_Tex2DAnim::Start(ShaderCreator *sc)
{
  VarName = 0;
}

sPoolString MM_Tex2DAnim::Get(ShaderCreator *sc)
{
  if(VarName==0)
  {
    VarName = sc->GetTemp();
    uvname = sc->GetTemp();

    sInt cando = 0;
    sGraphicsCaps caps;
    sGetGraphicsCaps(caps);
    sc->FragBegin(Name);

    switch(UV)
    {
    case 7:
      if(sc->ShaderType==SCS_PS)
        sc->InputPS(L"cs_normal",SCT_FLOAT3);
      else
        sc->Require(L"cs_normal",SCT_FLOAT3);
      sc->TB.PrintF(L"  float2 %s = normalize(cs_normal).xy*float2(0.5,-0.5)+0.5;\n",uvname);
      break;
    case 8:
      if(sc->ShaderType==SCS_PS)
      {
        sc->InputPS(L"cs_normal",SCT_FLOAT3);
        sc->InputPS(L"cs_eye",SCT_FLOAT3);
      }
      else
      {
        sc->Require(L"cs_normal",SCT_FLOAT3);
        sc->Require(L"cs_eye",SCT_FLOAT3);
      }
      sc->TB.PrintF(L"  float2 %s = reflect(normalize(cs_eye),normalize(cs_normal)).xy*float2(-0.5,0.5)+0.5;\n",uvname);
      break;
    default:
      if(sc->ShaderType==SCS_PS)
      {
        sc->InputPS(uvname,SCT_FLOAT2);
      }
      else 
      {
        sc->Require(uvname,SCT_FLOAT2);
      }
      break;
    }

#if sRENDERER!=sRENDER_DX11
    sU64 texmask = 1ULL<<(Texture->Texture->Flags & sTEX_FORMAT);
    if(sc->ShaderType==SCS_PS)
    {
      if(caps.Texture2D & texmask)
        cando = 1;
    }
    else 
    {
      if(caps.VertexTex2D & texmask)
        cando = 1;
    }
#else
    cando = 1;
#endif

    sInt slot = sc->Texture(Texture,Flags);
    if(cando)
    {
      sc->TB.PrintF(L"  float4 %s = %s;\n",VarName,Tex2D(slot,uvname,L"xyzw",Texture,sc));
    }
    else
    {
      sc->TB.PrintF(L"  float4 %s = float4(0,0,0,0); // texture format not supported\n",VarName);
    }
    switch(Aux)
    {
    case 1: sc->TB.PrintF(L"  %s.xyz *= col_aux;\n",VarName);  break;
    case 2: sc->TB.PrintF(L"  %s.xyz *= 1-col_aux;\n",VarName);  break;
    case 3: sc->TB.PrintF(L"  %s.xyz *= saturate(col_aux*2-1);\n",VarName);  break;
    case 4: sc->TB.PrintF(L"  %s.xyz *= saturate(1-col_aux*2);\n",VarName);  break;
    }

    sc->FragFirst(VarName);
    if(Aux)
      sc->FragRead(L"col_aux");

    sc->FragEnd();
  }
  return sPoolString(VarName);
}

void MM_Tex2DAnim::VS(ShaderCreator *sc)
{
  sInt index=-1;
  if(VarName && sc->Requires(uvname,index) && UV!=7 && UV!=8)
  {
    const sChar *temp = uvname;
    if(index>=0)
      temp = sc->GetTemp();
    sBool envi = 0;

    sc->FragBegin(Name);
    sc->FragFirst(temp);

    switch(UV)
    {
    case 0:
      sc->Require(L"uv0",SCT_FLOAT2);
      sc->TB.PrintF(L"  float4 %s = float4(uv0,0,1);\n",temp);
      break;
    case 1:
      sc->Require(L"uv1",SCT_FLOAT2);
      sc->TB.PrintF(L"  float4 %s = float4(uv1,0,1);\n",temp);
      break;
    case 2:
      sc->Require(L"ms_pos",SCT_FLOAT3);
      sc->TB.PrintF(L"  float4 %s = float4(ms_pos,1);\n",temp);
      break;
    case 3:
      sc->Require(L"ws_pos",SCT_FLOAT3);
      sc->TB.PrintF(L"  float4 %s = float4(ws_pos,1);\n",temp);
      break;
    case 4:
      sc->Require(L"cs_pos",SCT_FLOAT3);
      sc->TB.PrintF(L"  float4 %s = float4(cs_pos,1);\n",temp);
      break;
    case 5: 
      sc->Require(L"ws_normal",SCT_FLOAT3);
      sc->Para(L"ws_cs");
      sc->TB.PrintF(L"  float4 %s = float4(mul(ws_cs,float4(normalize(ws_normal),0)),1);\n",temp);
      envi = 1;
      break;
    case 6:
      sc->Require(L"cs_reflect",SCT_FLOAT3);
      sc->Para(L"ws_cs");
      sc->TB.PrintF(L"  float4 %s = float4(-cs_reflect,1);\n",temp);
      envi = 1;
      break;
    }

    if(Transform==1)
    {
      sc->TB.PrintF(L"  %s.xyz = %s.xyz*%f;\n",temp,temp,SimpleScale);
    }
    else if(Transform==2)
    {
      sSRT srt;
      srt.Scale = Scale;
      srt.Rotate = Rot*(1/sPI2F);
      srt.Translate = Trans;
      sMatrix34 mat;
      srt.MakeMatrix(mat);
        
      sc->TB.PrintF(L"  %s.xy = float2(dot(%s.xyzw,float4(%f,%f,%f,%f)),\n",temp,temp,mat.i.x,mat.j.x,mat.k.x,mat.l.x);
      sc->TB.PrintF(L"                     dot(%s.xyzw,float4(%f,%f,%f,%f)));\n",temp,mat.i.y,mat.j.y,mat.k.y,mat.l.y);
    } 
    else if(Transform>=3 && Transform<7)
    {
      const sChar *mat = sPoolF(L"Matrix%d",Transform-3);
      sc->Para(mat);
      sc->TB.PrintF(L"  %s.xy = mul(%s,%s.xyzw).xy;\n",temp,mat,temp);
    }
    if(envi)
    {
      sc->TB.PrintF(L"  %s.xy = %s.xy*float2(0.5,-0.5)+0.5;\n",temp,temp);
    }


    {
      sInt a=Texture->Atlas.Entries.GetCount();
      sF32 w=1;
      sF32 h=1;
      sInt iw=1;
      if(a>0)
      {
        w=Texture->Atlas.Entries[0].UVs.x1;
        h=Texture->Atlas.Entries[0].UVs.y1;
        iw=sFFloor(1.0f/w);    
      }

      const sChar *vect = sPoolF(L"Vector%d",AtlasAnim);
      sc->Para(vect);
      
      sc->TB.PrintF(L"  int %s_a  = %d*frac(Vector%d.x);\n",temp,a,AtlasAnim);  //calculate atlas
      sc->TB.PrintF(L"  int %s_ax  = %s_a %% %d;\n",temp,temp,iw);  //calculate atlas
      sc->TB.PrintF(L"  int %s_ay  = %s_a / %d;\n",temp,temp,iw);  //calculate atlas    
      sc->TB.PrintF(L"  %s.x = (%s.x+%s_ax)*%f;\n",temp,temp,temp,w);
      sc->TB.PrintF(L"  %s.y = (%s.y+%s_ay)*%f;\n",temp,temp,temp,h);
    }
    if(index>=0)
      sc->TB.PrintF(L"  t%d.%s = %s.xy;\n",index/4,sc->swizzle[index&3][2],temp);

    sc->FragEnd();
  }
}

/****************************************************************************/

MM_Tex2DSurround::MM_Tex2DSurround()
{
  Phase = MMP_Pre;
  Name = L"texture2DSurround";
  Flags = sMTF_LEVEL2|sMTF_TILE;
  Aux = 0;
  SimpleScale = 0;
  Texture = 0;
  Space = 0;
  MajorAxis.Init(1,1,1);
}

MM_Tex2DSurround::~MM_Tex2DSurround()
{
  Texture->Release();
}

void MM_Tex2DSurround::Start(ShaderCreator *sc)
{
  VarName = L"";
}

sPoolString MM_Tex2DSurround::Get(ShaderCreator *sc)
{
  if(VarName.IsEmpty())
  {
    VarName = sPoolString(sc->GetTemp());
    const sChar *pos = L"ws_pos";
    const sChar *norm = L"ws_normal";
    if(sc->ShaderType==SCS_VS || (Space&15)==1)
    {
      pos = L"ms_pos";
      norm = L"ms_normal";
    }
    if((Space&15)==2)
    {
      pos = L"ms_posvert";
      norm = L"ms_normalvert";
    }


    sc->FragBegin(Name);
    sc->FragFirst(VarName);
    if(Aux)
      sc->FragRead(L"col_aux");

    if(sc->ShaderType==SCS_PS)
    {
      sc->InputPS(norm,SCT_FLOAT3);
      sc->InputPS(pos,SCT_FLOAT3);
    }
    else
    {
      sc->Require(norm,SCT_FLOAT3);
//          sc->Require(pos,SCT_FLOAT3);   // for unknown reasons, we can not read-depeond on pos in vs
    }

    sc->TB.PrintF(L"  float4 %s;\n",VarName);
    sc->TB.PrintF(L"  {\n");
    switch(Transform)
    {
    case 0:
      sc->TB.PrintF(L"    float3 pos = %s;\n",pos);
      break;
    case 1:
      sc->TB.PrintF(L"    float3 pos = %s*%f;\n",pos,SimpleScale);
      break;
    case 2:
      {
        sMatrix34 mat;
        sSRT srt;
        srt.Scale = Scale;
        srt.Rotate = Rot;
        srt.Translate = Trans;
        srt.MakeMatrix(mat);
        sc->TB.PrintF(L"    float3 pos = float3(dot(float4(%s,1),float4(%f,%f,%f,%f)),\n" ,pos,mat.i.x,mat.j.x,mat.k.x,mat.l.x);
        sc->TB.PrintF(L"                        dot(float4(%s,1),float4(%f,%f,%f,%f)),\n" ,pos,mat.i.y,mat.j.y,mat.k.y,mat.l.y);
        sc->TB.PrintF(L"                        dot(float4(%s,1),float4(%f,%f,%f,%f)));\n",pos,mat.i.z,mat.j.z,mat.k.z,mat.l.z);
      }
      break;
    case 3:
    case 4:
    case 5:
    case 6:
      {
        sc->TB.PrintF(L"    float3 pos = %s;\n",pos);
        const sChar *mat = sPoolF(L"Matrix%d",Transform-3);
        sc->Para(mat);
        sc->TB.PrintF(L"    pos = mul(%s,float4(pos.xyz,1)).xyz;\n",mat);
      }
      break;
    }

    sInt slot = sc->Texture(Texture,Flags);

    sc->TB.PrintF(L"    float4 _xy = %s;\n",Tex2D(slot,L"pos.xy",L"xyzw",Texture,sc));
    sc->TB.PrintF(L"    float4 _yz = %s;\n",Tex2D(slot,L"pos.yz",L"xyzw",Texture,sc));
    sc->TB.PrintF(L"    float4 _xz = %s;\n",Tex2D(slot,L"pos.xz",L"xyzw",Texture,sc));

    sc->TB.PrintF(L"    float3 w = %s*%s;\n",norm,norm);
    if(Space & 16)
      sc->TB.PrintF(L"     w = w * float3(%f,%f,%f);\n",MajorAxis.x,MajorAxis.y,MajorAxis.z);
    sc->TB.PrintF(L"    w = w/(w.x+w.y+w.z);\n");
    sc->TB.PrintF(L"    %s = _xy*w.z + _yz*w.x + _xz*w.y;\n",VarName);
    switch(Aux)
    {
    case 1: sc->TB.PrintF(L"    %s.xyz *= col_aux;\n",VarName);  break;
    case 2: sc->TB.PrintF(L"    %s.xyz *= 1-col_aux;\n",VarName);  break;
    case 3: sc->TB.PrintF(L"    %s.xyz *= saturate(col_aux*2-1);\n",VarName);  break;
    case 4: sc->TB.PrintF(L"    %s.xyz *= saturate(1-col_aux*2);\n",VarName);  break;
    }
    sc->TB.PrintF(L"  }\n");

    sc->FragEnd();
  }

  return sPoolString(VarName);
}

/****************************************************************************/

MM_Tex2DSurroundNormal::MM_Tex2DSurroundNormal()
{
  Phase = MMP_Pre;
  Name = L"texture2DSurround";
  Flags = sMTF_LEVEL2|sMTF_TILE;
  SimpleScale = 0;
  Texture = 0;
  Space = 0;
  MajorAxis.Init(1,1,1);
}

MM_Tex2DSurroundNormal::~MM_Tex2DSurroundNormal()
{
  Texture->Release();
}

void MM_Tex2DSurroundNormal::Start(ShaderCreator *sc)
{
}

void MM_Tex2DSurroundNormal::PS(ShaderCreator *sc)
{
  const sChar *pos = L"ws_pos";
  const sChar *norm = L"ws_normal";
  if((Space&15)==1)
  {
    pos = L"ms_pos";
    norm = L"ms_normal";
  }
  if((Space&15)==2)
  {
    pos = L"ms_posvert";
    norm = L"ms_normalvert";
  }


  sc->FragBegin(Name);
  sc->InputPS(L"ws_normal",SCT_FLOAT3,1);
  sc->InputPS(L"ws_tangent",SCT_FLOAT3);
  sc->InputPS(L"ws_bitangent",SCT_FLOAT3);
  sc->InputPS(norm,SCT_FLOAT3);
  sc->InputPS(pos,SCT_FLOAT3);

  sc->TB.PrintF(L"  {\n");
  switch(Transform)
  {
  case 0:
    sc->TB.PrintF(L"    float3 pos = %s;\n",pos);
    break;
  case 1:
    sc->TB.PrintF(L"    float3 pos = %s*%f;\n",pos,SimpleScale);
    break;
  case 2:
    {
      sMatrix34 mat;
      sSRT srt;
      srt.Scale = Scale;
      srt.Rotate = Rot;
      srt.Translate = Trans;
      srt.MakeMatrix(mat);
      sc->TB.PrintF(L"    float3 pos = float3(dot(float4(%s,1),float4(%f,%f,%f,%f)),\n" ,pos,mat.i.x,mat.j.x,mat.k.x,mat.l.x);
      sc->TB.PrintF(L"                        dot(float4(%s,1),float4(%f,%f,%f,%f)),\n" ,pos,mat.i.y,mat.j.y,mat.k.y,mat.l.y);
      sc->TB.PrintF(L"                        dot(float4(%s,1),float4(%f,%f,%f,%f)));\n",pos,mat.i.z,mat.j.z,mat.k.z,mat.l.z);
    }
    break;
  }

  sc->TB.PrintF(L"    float3 norm = %s;\n",norm);
  sInt slot = sc->Texture(Texture,Flags);
  
  sc->TB.PrintF(L"    float4 _xy = %s;\n",Tex2D(slot,L"pos.xy",L"xyzw",Texture,sc));
  sc->TB.PrintF(L"    float4 _yz = %s;\n",Tex2D(slot,L"pos.yz",L"xyzw",Texture,sc));
  sc->TB.PrintF(L"    float4 _xz = %s;\n",Tex2D(slot,L"pos.xz",L"xyzw",Texture,sc));

/*
#if sRENDERER!=sRENDER_DX11       
  sc->TB.PrintF(L"    float3 _xy = tex2D(tex%d,pos.xy).xyz;\n",slot);
  sc->TB.PrintF(L"    float3 _yz = tex2D(tex%d,pos.yz).xyz;\n",slot);
  sc->TB.PrintF(L"    float3 _xz = tex2D(tex%d,pos.xz).xyz;\n",slot);
#else
  sc->TB.PrintF(L"    float3 _xy = tex%d.Sample(sam%d,pos.xy).xyz;\n",slot,slot);
  sc->TB.PrintF(L"    float3 _yz = tex%d.Sample(sam%d,pos.yz).xyz;\n",slot,slot);
  sc->TB.PrintF(L"    float3 _xz = tex%d.Sample(sam%d,pos.xz).xyz;\n",slot,slot);  
#endif
*/
  sc->TB.PrintF(L"    _xy = normalize(_xy-0.5);\n");
  sc->TB.PrintF(L"    _xy = normalize(\n");
  sc->TB.PrintF(L"      normalize(ws_normal   )*_xy.z+\n");
  sc->TB.PrintF(L"      normalize(ws_bitangent)*_xy.y+\n");
  sc->TB.PrintF(L"      normalize(ws_tangent  )*_xy.x*-sign(norm.z));\n");
  sc->TB.PrintF(L"    _yz = normalize(_yz-0.5);\n");
  sc->TB.PrintF(L"    _yz = normalize(\n");
  sc->TB.PrintF(L"      normalize(ws_normal   )*_yz.z+\n");
  sc->TB.PrintF(L"      normalize(ws_bitangent)*_yz.x+\n");
  sc->TB.PrintF(L"      normalize(ws_tangent  )*_yz.y*sign(norm.x));\n");
  sc->TB.PrintF(L"    _xz = normalize(_xz-0.5);\n");
  sc->TB.PrintF(L"    _xz = normalize(\n");
  sc->TB.PrintF(L"      normalize(ws_normal   )*_xz.z+\n");
  sc->TB.PrintF(L"      normalize(ws_bitangent)*_xz.y+\n");
  sc->TB.PrintF(L"      normalize(ws_tangent  )*_xz.x);\n");

  sc->TB.PrintF(L"    float3 w = norm*norm;\n");
  if(Space & 16)
    sc->TB.PrintF(L"     w = w * float3(%f,%f,%f);\n",MajorAxis.x,MajorAxis.y,MajorAxis.z);
  sc->TB.PrintF(L"    w = w/(w.x+w.y+w.z);\n");
  sc->TB.PrintF(L"    ws_normal = _xz;//_xy*w.z + _yz*w.x + _xz*w.y;\n");
  sc->TB.PrintF(L"  }\n");

  sc->FragEnd();
}


/****************************************************************************/
/****************************************************************************/

MM_Shadow::MM_Shadow()
{
  Phase = MMP_Shadow;
  Name = L"shadow";
  sClear(Slot);
  sClear(Mode);
  Shaders = 2;
  Shadow = 0;
  ShadowOrd = 0;
  ShadowRand = 0;
}

void MM_Shadow::PS(ShaderCreator *sc)
{
  for(sInt i=0;i<MM_MaxLight;i++)
  {
    Slot[i]=-1;
  }
  if(Shadow)
  {
    sc->FragBegin(Name);
    sc->FragModify(L"lightshadow");

    sc->TB.PrintF(L"  { float d; float4 prpos; \n");

    if(ShadowRand)
    {

      sc->Para(L"RandomDisc");
      sc->BindPS(L"vpos",SCT_FLOAT2,SCB_VPOS);
      sInt random = sc->Texture(ModMtrlType->SinCosTex,sMTF_LEVEL0|sMTF_TILE);
      sc->TB.PrintF(L"    float4 rd,dsp,uv,m0,m1; float f;\n");
//      sc->TB.PrintF(L"    float2 sc = tex2D(tex%d,vpos.xy/64).xy*2-1;\n",random);
      sc->TB.PrintF(L"    float2 sc = %s*2-1;\n",Tex2D(random,L"vpos.xy/64",L"xy",0));
    }
    if(ShadowOrd)
    {
      sc->TB.PrintF(L"    float4 dd; float2 uv; float f;\n");
      sc->TB.PrintF(L"    float2 ord[8] = {\n");
      sc->TB.PrintF(L"      {  1.0, 0.0 },\n");
      sc->TB.PrintF(L"      { -1.0, 0.0 },\n");
      sc->TB.PrintF(L"      {  0.0, 1.0 },\n");
      sc->TB.PrintF(L"      {  0.0,-1.0 },\n");

      sc->TB.PrintF(L"      { -0.5, 0.5 },\n");
      sc->TB.PrintF(L"      { -0.5,-0.5 },\n");
      sc->TB.PrintF(L"      {  0.5,-0.5 },\n");
      sc->TB.PrintF(L"      {  0.5, 0.5 },\n");
      sc->TB.PrintF(L"    };\n");
    }

    for(sInt i=0;i<MM_MaxLight;i++)
    {
      sInt bit = 1<<i;
      if(Shadow & bit)
      {
        if(!(sc->LightMask & (1<<i)))
          sc->TB.PrintF(L"#error shadowmap %d calculated and not used!\n",i);

        sInt slot = sc->Texture(ModMtrlType->DummyTex2D,sMTF_LEVEL1|sMTF_BORDER_BLACK|sMTF_PCF);
        Slot[i] = slot;
        sc->UpdateTex[slot].Enable = 1;
        sc->UpdateTex[slot].Light = i;
        sc->UpdateTex[slot].Mode = Mode[i];

        sc->Para(sPoolF(L"SM_Filter%d",i));
        sc->InputPS(sPoolF(L"tp%d_pos",slot),SCT_FLOAT4);
//        sc->InputPS(sPoolF(L"ls_pos%d_z",slot),SCT_FLOAT);

        sc->TB.PrintF(L"    prpos = tp%d_pos;\n",slot);
//        sc->TB.PrintF(L"    z = ls_pos%d_z;\n",slot);

        if(ShadowOrd & bit)
        {
          /*
          sc->TB.PrintF(L"    d = 0;\n");
          sc->TB.PrintF(L"    f = SM_Filter%d;\n",i);
          sc->TB.PrintF(L"    uv = prpos.xy/prpos.w;\n");
          sc->TB.PrintF(L"    dd.x = tex2D(tex%d,uv+ord[0]*f).x;\n",slot);
          sc->TB.PrintF(L"    dd.y = tex2D(tex%d,uv+ord[1]*f).x;\n",slot);
          sc->TB.PrintF(L"    dd.z = tex2D(tex%d,uv+ord[2]*f).x;\n",slot);
          sc->TB.PrintF(L"    dd.w = tex2D(tex%d,uv+ord[3]*f).x;\n",slot);
          sc->TB.PrintF(L"    d += dot(dd<z,1);\n");
          sc->TB.PrintF(L"    dd.x = tex2D(tex%d,uv+ord[4]*f).x;\n",slot);
          sc->TB.PrintF(L"    dd.y = tex2D(tex%d,uv+ord[5]*f).x;\n",slot);
          sc->TB.PrintF(L"    dd.z = tex2D(tex%d,uv+ord[6]*f).x;\n",slot);
          sc->TB.PrintF(L"    dd.w = tex2D(tex%d,uv+ord[7]*f).x;\n",slot);
          sc->TB.PrintF(L"    d += dot(dd<z,1);\n");
          sc->TB.PrintF(L"    lightshadow[%d] *= 1-d/8;\n",i);
          */
          sc->TB.PrintF(L"    d = 0;\n");
          sc->TB.PrintF(L"    f = SM_Filter%d*prpos.w;\n",i);

          sc->TB.PrintF(L"    d += %s;\n",Tex2DProjCmp(slot,L"prpos.xy+ord[0]*f",L"prpos.z",L"prpos.w",L"x",0));
          sc->TB.PrintF(L"    d += %s;\n",Tex2DProjCmp(slot,L"prpos.xy+ord[1]*f",L"prpos.z",L"prpos.w",L"x",0));
          sc->TB.PrintF(L"    d += %s;\n",Tex2DProjCmp(slot,L"prpos.xy+ord[2]*f",L"prpos.z",L"prpos.w",L"x",0));
          sc->TB.PrintF(L"    d += %s;\n",Tex2DProjCmp(slot,L"prpos.xy+ord[3]*f",L"prpos.z",L"prpos.w",L"x",0));
          sc->TB.PrintF(L"    d += %s;\n",Tex2DProjCmp(slot,L"prpos.xy+ord[4]*f",L"prpos.z",L"prpos.w",L"x",0));
          sc->TB.PrintF(L"    d += %s;\n",Tex2DProjCmp(slot,L"prpos.xy+ord[5]*f",L"prpos.z",L"prpos.w",L"x",0));
          sc->TB.PrintF(L"    d += %s;\n",Tex2DProjCmp(slot,L"prpos.xy+ord[6]*f",L"prpos.z",L"prpos.w",L"x",0));
          sc->TB.PrintF(L"    d += %s;\n",Tex2DProjCmp(slot,L"prpos.xy+ord[7]*f",L"prpos.z",L"prpos.w",L"x",0));

          sc->TB.PrintF(L"    lightshadow[%d] *= d/8;\n",i);
        }
        else if(ShadowRand & bit)
        {
          sc->TB.PrintF(L"    f = SM_Filter%d;\n",i);
          sc->TB.PrintF(L"    uv = prpos.xyxy/prpos.w;\n");
          sc->TB.PrintF(L"    m0 = f*float4(sc.x, sc.y,sc.x, sc.y)*prpos.w;\n");
          sc->TB.PrintF(L"    m1 = f*float4(sc.y,-sc.x,sc.y,-sc.x)*prpos.w;\n");
          /*
          sc->TB.PrintF(L"    m0 = f*float4(sc.x, sc.y,sc.x, sc.y);\n");
          sc->TB.PrintF(L"    m1 = f*float4(sc.y,-sc.x,sc.y,-sc.x);\n");
          */

          sc->TB.PrintF(L"    d = 0;\n");
          for(sInt j=0;j<2;j++)
          {
            /*
            sc->TB.PrintF(L"      rd = RandomDisc[%d];\n",j*2+0);
            sc->TB.PrintF(L"      dsp = rd.xxzz*m0 + rd.yyww*m1 + uv;\n");
            sc->TB.PrintF(L"      dd.x = tex2D(tex%d,dsp.xy).x;\n",slot);
            sc->TB.PrintF(L"      dd.y = tex2D(tex%d,dsp.zw).x;\n",slot);

            sc->TB.PrintF(L"      rd = RandomDisc[%d];\n",j*2+1);
            sc->TB.PrintF(L"      dsp = rd.xxzz*m0 + rd.yyww*m1 + uv;\n");
            sc->TB.PrintF(L"      dd.z = tex2D(tex%d,dsp.xy).x;\n",slot);
            sc->TB.PrintF(L"      dd.w = tex2D(tex%d,dsp.zw).x;\n",slot);
            sc->TB.PrintF(L"      d += dot(dd<z,1);\n");
            */

            sc->TB.PrintF(L"      rd = RandomDisc[%d];\n",j*2+0);
            sc->TB.PrintF(L"      dsp = rd.xxzz*m0 + rd.yyww*m1;\n");
            sc->TB.PrintF(L"      d += %s;\n",Tex2DProjCmp(slot,L"prpos.xy+dsp.xy",L"prpos.z",L"prpos.w",L"x",0));
            sc->TB.PrintF(L"      d += %s;\n",Tex2DProjCmp(slot,L"prpos.xy+dsp.zw",L"prpos.z",L"prpos.w",L"x",0));

            sc->TB.PrintF(L"      rd = RandomDisc[%d];\n",j*2+1);
            sc->TB.PrintF(L"      dsp = rd.xxzz*m0 + rd.yyww*m1;\n");
            sc->TB.PrintF(L"      d += %s;\n",Tex2DProjCmp(slot,L"prpos.xy+dsp.xy",L"prpos.z",L"prpos.w",L"x",0));
            sc->TB.PrintF(L"      d += %s;\n",Tex2DProjCmp(slot,L"prpos.xy+dsp.zw",L"prpos.z",L"prpos.w",L"x",0));
          }

          sc->TB.PrintF(L"    lightshadow[%d] *= d/8;\n",i);
        }
        else
        {
          sc->TB.PrintF(L"    lightshadow[%d] *= %s;\n",i,Tex2DProjCmp(slot,L"prpos.xy",L"prpos.z",L"prpos.w",L"x",0));
        }
      }
    }
    sc->TB.PrintF(L"  }\n");

    sc->FragEnd();
  }
}

void MM_Shadow::VS(ShaderCreator *sc)
{
  sBool any = 0;
  for(sInt i=0;i<MM_MaxLight;i++)
    any |= Mode[i];
  if(any)
  {
    sc->FragBegin(Name);
    sc->Require(L"ws_pos",SCT_FLOAT3);

    for(sInt i=0;i<MM_MaxLight;i++)
    {
      if(Slot[i]>=0)
      {
        sInt slot = Slot[i];
        sInt index;
          
        if(sc->Requires(sPoolF(L"tp%d_pos",slot),index))
        {
          sc->Para(sPoolF(L"LightProj%d",slot));
          sc->TB.PrintF(L"  float4 tp%d_pos = mul(float4(ws_pos,1),LightProj%d);\n",slot,slot);
          if(index>=0)
            sc->TB.PrintF(L"  t%d.%s = tp%d_pos;\n",index/4,sc->swizzle[index&3][4],slot);
        }
/*
        if(sc->Requires(sPoolF(L"ls_pos%d_z",slot),index))
        {
          sc->Para(sPoolF(L"LightMatZ%d",slot));
          sc->TB.PrintF(L"  float ls_pos%d = dot(float4(ws_pos,1),LightMatZ%d);\n",slot,slot);
          if(index>=0)
            sc->TB.PrintF(L"  t%d.%s = ls_pos%d;\n",index/4,sc->swizzle[index&3][1],slot);
        }
*/
      }
    }
    sc->FragEnd();
  }
}

/****************************************************************************/

MM_ShadowCube::MM_ShadowCube()
{
  Phase = MMP_Shadow;
  Name = L"shadow";
  sClear(Slot);
  sClear(Mode);
  Shaders = 2;
}

void MM_ShadowCube::PS(ShaderCreator *sc)
{
  sBool any = 0;
  for(sInt i=0;i<MM_MaxLight;i++)
  {
    Slot[i]=-1;
    any |= Mode[i];
  }
  if(any)
  {
    sc->FragBegin(Name);
    sc->TB.PrintF(L"  { float z,d,f; float4 prpos; float3 dir;\n");
    sc->InputPS(L"ws_pos",SCT_FLOAT3);

    for(sInt i=0;i<MM_MaxLight;i++)
    {
      if(Mode[i])
      {
        sc->Para(sPoolF(L"ws_LightPos%d",i));
        sc->Para(sPoolF(L"LightFarR%d",i));
        sc->FragModify(L"lightshadow");
        if(!(sc->LightMask & (1<<i)))
          sc->TB.PrintF(L"#error shadowmap %d calculated and not used!\n",i);

        sInt slot = sc->Texture(ModMtrlType->DummyTexCube,sMTF_LEVEL0|sMTF_BORDER_BLACK);
        Slot[i] = slot;
        sc->InputPS(sPoolF(L"tp%d_pos",slot),SCT_FLOAT4);

        sc->UpdateTex[slot].Enable = 1;
        sc->UpdateTex[slot].Light = i;
        sc->UpdateTex[slot].Mode = Mode[i];

//        sc->Para(sPoolF(L"SM_Filter%d",i));

        sc->TB.PrintF(L"    prpos = tp%d_pos;\n",slot);
        sc->TB.PrintF(L"    dir = ws_pos.xyz-ws_LightPos%d;\n",i);
//        sc->TB.PrintF(L"    f = SM_Filter%d;\n",i);
        sc->TB.PrintF(L"    z = length(dir)*LightFarR%d;\n",i,i);
        if(sRENDERER==sRENDER_DX11)
          sc->TB.PrintF(L"    d = tex%d.Sample(sam%d,dir).x < z;\n",slot,slot);
        else
          sc->TB.PrintF(L"    d = texCUBE(tex%d,dir).x < z;\n",slot);
        sc->TB.PrintF(L"    lightshadow[%d] *= 1-d;\n",i);
      }
    }
    sc->TB.PrintF(L"  }\n");

    sc->FragEnd();
  }
}

void MM_ShadowCube::VS(ShaderCreator *sc)
{
  sBool any = 0;
  for(sInt i=0;i<MM_MaxLight;i++)
    any |= Mode[i];
  if(any)
  {
    sc->FragBegin(Name);
    sc->Require(L"ws_pos",SCT_FLOAT3);

    for(sInt i=0;i<MM_MaxLight;i++)
    {
      if(Slot[i]>=0)
      {
        sInt slot = Slot[i];
        sInt index;
          
        if(sc->Requires(sPoolF(L"tp%d_pos",slot),index))
        {
          sc->Para(sPoolF(L"LightProj%d",slot));
          sc->TB.PrintF(L"  float4 tp%d_pos = mul(float4(ws_pos,1),LightProj%d);\n",slot,slot);
          if(index>=0)
            sc->TB.PrintF(L"  t%d.%s = tp%d_pos;\n",index/4,sc->swizzle[index&3][4],slot);
        }
      }
    }
    sc->FragEnd();
  }
}

/****************************************************************************/
/****************************************************************************/

MM_Fog::MM_Fog()
{
  Phase = MMP_Post;
  Name = L"fog";
  FeatureFlags = 0;
  Shaders = 2;
}

void MM_Fog::PS(ShaderCreator *sc)
{
  sc->FragBegin(Name);

  sc->FragModify(L"col_fog");
  sc->FragModify(L"fogfactor");

  sc->InputPS(L"cs_pos",SCT_FLOAT3);
  sc->Para(L"FogAdd");        // FogAdd = -mindist
  sc->Para(L"FogMul");        // FogMul = 1/maxdist
  sc->Para(L"FogDensity");
  if((FeatureFlags & MMFF_FogIsBlack)==0)
    sc->Para(L"FogColor");

  sc->TB.PrintF(L"  {\n");
  sc->TB.PrintF(L"    float fog = length(cs_pos);\n");
  sc->TB.PrintF(L"    fog = saturate((fog+FogAdd)*FogMul);\n");
  sc->TB.PrintF(L"    fog = 2*fog-fog*fog;\n");
  sc->TB.PrintF(L"    fog = fog*FogDensity;\n");
  sc->TB.PrintF(L"    if(fog+fogfactor>1) fog = 1-fogfactor;\n");
  sc->TB.PrintF(L"    fogfactor += fog;\n");
  if((FeatureFlags & MMFF_FogIsBlack)==0)
    sc->TB.PrintF(L"    col_fog += fog*FogColor;\n");
  sc->TB.PrintF(L"  }\n");

  sc->FragEnd();
}

/****************************************************************************/

MM_GroundFog::MM_GroundFog()
{
  Phase = MMP_Post;
  Name = L"GroundFog";
  FeatureFlags = 0;
  Shaders = 2;
}

void MM_GroundFog::PS(ShaderCreator *sc)
{
  sc->FragBegin(Name);

  sc->FragModify(L"col_fog");
  sc->FragModify(L"fogfactor");

  sc->InputPS(L"cs_pos",SCT_FLOAT3);
  sc->Para(L"GroundFogAdd");        // FogAdd = -mindist
  sc->Para(L"GroundFogMul");        // FogMul = 1/maxdist
  sc->Para(L"GroundFogDensity");
  if((FeatureFlags & MMFF_FogIsBlack)==0)
    sc->Para(L"GroundFogColor");
  sc->Para(L"cs_GroundFogPlane");

  sc->TB.PrintF(L"  {\n");
  sc->TB.PrintF(L"    float fog = length(cs_pos);\n");
  sc->TB.PrintF(L"    float d = dot(normalize(cs_pos),cs_GroundFogPlane.xyz);\n");
  sc->TB.PrintF(L"    if(d < 0)\n");
  sc->TB.PrintF(L"      fog = min(fog,-dot(float4(cs_pos,1),cs_GroundFogPlane)/-d);\n");
  sc->TB.PrintF(L"    else \n");
  sc->TB.PrintF(L"      fog = min(fog,cs_GroundFogPlane.w/-d);\n");
  sc->TB.PrintF(L"    fog = saturate((fog+GroundFogAdd)*GroundFogMul);\n");
  sc->TB.PrintF(L"    fog = 3*fog*fog-2*fog*fog*fog;\n");
  sc->TB.PrintF(L"    fog = fog*GroundFogDensity;\n");
  sc->TB.PrintF(L"    if(fog+fogfactor>1) fog = 1-fogfactor;\n");
  sc->TB.PrintF(L"    fogfactor += fog;\n");
  if((FeatureFlags & MMFF_FogIsBlack)==0)
    sc->TB.PrintF(L"    col_fog += fog*GroundFogColor;\n");
  sc->TB.PrintF(L"  }\n");

  sc->FragEnd();
}

/****************************************************************************/

MM_ClipPlanes::MM_ClipPlanes()
{
  Phase = MMP_Post;
  Name = L"clip planes";
  Planes = 15;
  Shaders = 2;
}

void MM_ClipPlanes::PS(ShaderCreator *sc)
{
  if(Planes)
  {
    sc->FragBegin(Name);

    sc->InputPS(L"cs_pos",SCT_FLOAT3);

    sc->TB.PrintF(L"  {\n");
    sc->TB.PrintF(L"    float4 clipdot = 1;\n");

    if(Planes&1)
    {
      sc->Para(L"cs_Clip0");
      sc->TB.PrintF(L"    clipdot.x = dot(float4(cs_pos,1),cs_Clip0);\n");
    }
    if(Planes&2)
    {
      sc->Para(L"cs_Clip1");
      sc->TB.PrintF(L"    clipdot.y = dot(float4(cs_pos,1),cs_Clip1);\n");
    }
    if(Planes&4)
    {
      sc->Para(L"cs_Clip2");
      sc->TB.PrintF(L"    clipdot.z = dot(float4(cs_pos,1),cs_Clip2);\n");
    }
    if(Planes&8)
    {
      sc->Para(L"cs_Clip3");
      sc->TB.PrintF(L"    clipdot.w = dot(float4(cs_pos,1),cs_Clip3);\n");
    }
    sc->TB.PrintF(L"    clip(clipdot);\n");
    sc->TB.PrintF(L"  }\n");

    sc->FragEnd();
  }
}

/****************************************************************************/
/****************************************************************************/

MM_Const::MM_Const()
{
  Phase = MMP_TexLD;
  Name = L"const";
  Color.Init(0,0,0,0);
  Source = 0;
  VarName = 0;
}

void MM_Const::Start(ShaderCreator *sc)
{
  VarName = 0;
}

sPoolString MM_Const::Get(ShaderCreator *sc)
{
  if(VarName==0)
  {
    VarName = sc->GetTemp();
    sc->FragBegin(Name);
    sc->FragFirst(VarName);

    switch(Source)
    {
    case 0:
      sc->TB.PrintF(L"  float4 %s = float4(%f,%f,%f,%f);\n",VarName,Color.x,Color.y,Color.z,Color.w);
      break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
      sc->Para(sPoolF(L"Color%d",Source-1));
      sc->TB.PrintF(L"  float4 %s = float4(Color%d,1);\n",VarName,Source-1);
      break;
    case 9:
      sc->InputPS(L"color0",SCT_FLOAT4);
      sc->TB.PrintF(L"  float4 %s = color0;\n",VarName);
      break;
    case 10:
      sc->InputPS(L"color1",SCT_FLOAT4);
      sc->TB.PrintF(L"  float4 %s = color1;\n",VarName);
      break;
    case 11:
      sc->InputPS(L"instplus",SCT_FLOAT4);
      sc->TB.PrintF(L"  float4 %s = instplus;\n",VarName);
      break;
    case 12:
      sc->InputPS(L"cs_pos_z",SCT_FLOAT);
      sc->TB.PrintF(L"  float4 %s = cs_pos_z;\n",VarName);
      break;
    case 13:
      sc->Para(L"ShellExtrude");
      sc->TB.PrintF(L"  float4 %s = ShellExtrude;\n",VarName);
      break;
    }
    sc->FragEnd();
  }
  return sPoolString(VarName);
}

/****************************************************************************/

MM_TrickLight::MM_TrickLight()
{
  Phase = MMP_Shader;
  Name = L"trick light";
  LightFlags = 1;
  LightEnable = 0;
  VarName = 0;
}


void MM_TrickLight::Start(ShaderCreator *sc)
{
  VarName = 0;
}


sPoolString MM_TrickLight::Get(ShaderCreator *sc)
{
  if(VarName==0)
  {
    VarName = sc->GetTemp();
    sc->FragBegin(Name);
    sc->FragFirst(VarName);

    sc->TB.PrintF(L"  float4 %s = 0;\n",VarName);
    sc->InputPS(L"ws_normal",SCT_FLOAT3);     // vertex format may not change depending on light on/off

    if(sc->LightMask & LightEnable)
    {
      sc->FragRead(L"ws_lightdir");
      sc->FragRead(L"lighti");

      sc->TB.PrintF(L"  { float l,li;\n");
      for(sInt i=0;i<MM_MaxLight;i++)
      {
        if(sc->LightMask & LightEnable & (1<<i))
        {
          sc->TB.PrintF(L"    li = lighti[%d];\n",i);
          sc->TB.PrintF(L"    l = (dot(ws_lightdir[%d],normalize(ws_normal)));\n",i);
          sc->TB.PrintF(L"    %s += saturate(l)*li;\n",VarName);
        }
      }
      sc->TB.PrintF(L"  }\n");
    }

    sc->FragEnd();
  }
  return sPoolString(VarName);
}

/****************************************************************************/

MM_Math::MM_Math()
{
  Phase = MMP_TexLD;
  Name = L"math";
  Mode = 0;
  VarName = 0;
}

void MM_Math::Start(ShaderCreator *sc)
{
  VarName = 0;
}

sPoolString MM_Math::Get(ShaderCreator *sc)
{
  if(VarName==0)
  {
    // gather inputs
    sInt max = Inputs.GetCount();
    const sChar **tex = new const sChar *[max];
    for(sInt i=0;i<max;i++)
      tex[i] = Inputs[i]->Get(sc);

    // who are we?

    sInt mode = '#';
    switch(Mode)
    {
    case 0: mode = '*'; break;
    case 1: mode = '+'; break;
    case 2: mode = '-'; break;
    }

    // do the math

    VarName = sc->GetTemp();
    sc->FragBegin(Name);
    sc->FragFirst(VarName);
    for(sInt i=0;i<Inputs.GetCount();i++)
      sc->FragRead(tex[i]);

    sc->TB.PrintF(L"  float4 %s = %s;\n",VarName,tex[0]);
    for(sInt i=1;i<Inputs.GetCount();i++)
    {
      sc->TB.PrintF(L"  %s %c= %s;\n",VarName,mode,tex[i]);
    }
    sc->FragEnd();

    // clean up

    delete[] tex;
  }
  return sPoolString(VarName);
}

/****************************************************************************/

MM_ApplyTexture::MM_ApplyTexture()
{
  Phase = MMP_Post;
  Name = L"apply texture";
  Dest = L"col_diffuse";
  Op = 0;
  Swizzle = L"xyz";
  Tex = 0;
  Aux = 0;
  InputRemap = 0;
  Shaders = 2;
}

void MM_ApplyTexture::PS(ShaderCreator *sc)
{
  const sChar *tex = Tex->Get(sc);

  sc->FragBegin(Name);
  sc->FragModify(Dest);
  sc->FragRead(tex);
  if(Aux)
    sc->FragRead(L"col_aux");

  const sChar *aux = L"1";
  switch(Aux)
  {
  case 1: aux = L"col_aux"; break;
  case 2: aux = L"(1-col_aux)"; break;
  case 3: aux = L"saturate(col_aux*2-1)"; break;
  case 4: aux = L"saturate(1-col_aux*2)"; break;
  }

  sString<32> input;
  switch(InputRemap)
  {
  case 1:   input.PrintF(L"saturate(%s.%s*2-1)",tex,Swizzle);
  default:  input.PrintF(L"%s.%s",tex,Swizzle);
  }

  switch(Op)
  {
  default:
  case 0: sc->TB.PrintF(L"  %s += %s * %s;\n",Dest,input,aux); break;
  case 1: sc->TB.PrintF(L"  %s = lerp(%s,%s*%s,%s);\n",Dest,Dest,Dest,input,aux); break;
  case 2: sc->TB.PrintF(L"  %s -= %s * %s;\n",Dest,input,aux); break;
  case 3: sc->TB.PrintF(L"  %s = lerp(%s,%s*%s*2,%s);\n",Dest,Dest,Dest,input,aux); break;
  case 4: sc->TB.PrintF(L"  %s = lerp(%s,%s*%s*4,%s);\n",Dest,Dest,Dest,input,aux); break;
  case 5: sc->TB.PrintF(L"  %s += (1 - %s) * %s * %s;\n",Dest,Dest,input,aux); break;
  case 6: sc->TB.PrintF(L"  %s = %s * %s;\n",Dest,input,aux); break;
  }

  sc->FragEnd();
}

/****************************************************************************/

MM_Kill::MM_Kill()
{
  Phase = MMP_Post;
  Name = L"kill";
  ThreshLo = 0;
  ThreshHi = 1;
  Invert = 0;
  Channel = 0;
}

void MM_Kill::PS(ShaderCreator *sc)
{
  const sChar *tex = Tex->Get(sc);

  sc->FragBegin(Name);
  sc->FragRead(tex);
  if(Invert)
    sc->TB.PrintF(L"  clip(%s>%f && %s<%f ? -1 : 1);\n",tex,ThreshLo,tex,ThreshHi);
  else
    sc->TB.PrintF(L"  clip(%s<%f || %s>%f ? -1 : 1);\n",tex,ThreshLo,tex,ThreshHi);
  sc->FragEnd();
}

/****************************************************************************/
/****************************************************************************/

MM_Displace::MM_Displace()
{
  Name = L"displace";
  Phase = MMP_Pre;
  Tex = 0;
  Source = 0;
  Shaders = 1;
}

void MM_Displace::VS(ShaderCreator *sc)
{
  const sChar *temp = Tex->Get(sc);

  sc->FragBegin(Name);

  if(Source>0)
  {
    sc->Para(sPoolF(L"Vector%d",Source-1));
    sc->TB.PrintF(L"  ms_pos += ms_normal * ((%s.%s+%f)*Vector%d.xyz);\n",temp,Swizzle,Bias,Source-1);
  }
  else
  {
    sc->TB.PrintF(L"  ms_pos += ms_normal * ((%s.%s+%f)*%f);\n",temp,Swizzle,Bias,Scale);
  }

  sc->Require(L"ms_normal",SCT_FLOAT3);
  sc->FragModify(L"ms_pos");
  sc->FragRead(temp);

  sc->FragEnd();
}

/****************************************************************************/

MM_ExtrudeNormal::MM_ExtrudeNormal()
{
  Name = L"ExtrudeNormal";
  Source = 0;
  Value = 0;
}

void MM_ExtrudeNormal::VS(ShaderCreator *sc)
{
  sc->FragBegin(Name);
  sc->Require(L"ms_normal",SCT_FLOAT3);
  sc->FragModify(L"ms_pos");
  switch(Source)
  {
  case 0:
    sc->Para(L"ShellExtrude");
    sc->TB.PrintF(L"  ms_pos += ms_normal * ShellExtrude * %f;\n",Value);
    break;
  case 1:
    sc->TB.PrintF(L"  ms_pos += ms_normal * %f;\n",Value);
    break;
  case 2:
  case 3:
  case 4:
  case 5:
  case 6:
  case 7:
  case 8:
  case 9:
    sc->Para(sPoolF(L"Vector%d",Source-2));
    sc->TB.PrintF(L"  ms_pos += ms_normal * Vector%d.xyz * %f;\n",Source-2,Value);
    break;
  }
  sc->FragEnd();
}

/****************************************************************************/

MM_NormalMap::MM_NormalMap()
{
  Phase = MMP_Post;
  Name = L"normal map";
  Tex = 0;
  Shaders = 2;
}

void MM_NormalMap::PS(ShaderCreator *sc)
{
  const sChar *temp = Tex->Get(sc);
  const sChar *temp2 = sc->GetTemp();

  sc->FragBegin(Name);

  sc->InputPS(L"ws_normal",SCT_FLOAT3,1);
  sc->InputPS(L"ws_tangent",SCT_FLOAT3);
  sc->InputPS(L"ws_bitangent",SCT_FLOAT3);
  sc->FragRead(temp);

  sc->TB.PrintF(L"  float3 %s = normalize(%s.xyz-0.5);\n",temp2,temp);
  sc->TB.PrintF(L"  ws_normal = normalize(\n");
  sc->TB.PrintF(L"    normalize(ws_normal)*%s.zzz+\n",temp2);
  sc->TB.PrintF(L"    normalize(ws_bitangent)*-%s.yyy+\n",temp2);
  sc->TB.PrintF(L"    normalize(ws_tangent)*%s.xxx);\n",temp2);

  sc->FragEnd();
}

/****************************************************************************/

MM_DoubleNormalMap::MM_DoubleNormalMap()
{
  Phase = MMP_Post;
  Name = L"normal map";
  Tex0 = 0;
  Tex1 = 0;
  Shaders = 2;
  Flags = 0;
}

void MM_DoubleNormalMap::PS(ShaderCreator *sc)
{
  const sChar *temp1 = Tex0->Get(sc);
  const sChar *temp2 = Tex1->Get(sc);

  sc->FragBegin(Name);

  sc->InputPS(L"ws_normal",SCT_FLOAT3,1);
  sc->InputPS(L"ws_tangent",SCT_FLOAT3);
  sc->InputPS(L"ws_bitangent",SCT_FLOAT3);
  sc->FragRead(temp1);
  sc->FragRead(temp2);

  if(Flags & 1)
  {
    sc->TB.PrintF(L"  {\n");
    sc->TB.PrintF(L"    %s.xyz = normalize(%s.xyz-0.5);\n",temp1,temp1);
    sc->TB.PrintF(L"    float3 ws_normal1 = (\n");
    sc->TB.PrintF(L"      normalize(ws_normal)*%s.z+\n",temp1);
    sc->TB.PrintF(L"      normalize(ws_bitangent)*%s.y+\n",temp1);
    sc->TB.PrintF(L"      normalize(ws_tangent)*%s.x);\n",temp1);
    sc->TB.PrintF(L"    %s.xyz = normalize(%s.xyz-0.5);\n",temp2,temp2);
    sc->TB.PrintF(L"    float3 ws_normal2 = (\n");
    sc->TB.PrintF(L"      normalize(ws_normal)*%s.z+\n",temp2);
    sc->TB.PrintF(L"      normalize(ws_bitangent)*%s.y+\n",temp2);
    sc->TB.PrintF(L"      normalize(ws_tangent)*%s.x);\n",temp2);
    sc->TB.PrintF(L"    ws_normal = lerp(ws_normal1,ws_normal2,col_aux);\n");
    sc->TB.PrintF(L"  }\n");
  }
  else
  {
    sc->TB.PrintF(L"  %s.xyz = normalize(%s.xyz-0.5)+normalize(%s.xyz-0.5);\n",temp1,temp1,temp2);
    sc->TB.PrintF(L"  ws_normal = (\n");
    sc->TB.PrintF(L"    normalize(ws_normal)*%s.z+\n",temp1);
    sc->TB.PrintF(L"    normalize(ws_bitangent)*%s.y+\n",temp1);
    sc->TB.PrintF(L"    normalize(ws_tangent)*%s.x);\n",temp1);
  }
  sc->FragEnd();
}

/****************************************************************************/
/****************************************************************************/

MM_BakedShadow::MM_BakedShadow()
{
  Phase = MMP_TexLD;
  Name = L"Baked Shadow";

  Tex = 0;
  Dir.Init(0,1,0);
  ShadowSize = 10;
  ShadowBaseBias = 0.5f;
  ShadowFilter = 0.5f;
  ShadowSlopeBias = 0.5f;
  VarName = 0;
}

MM_BakedShadow::~MM_BakedShadow()
{
  sRelease(Tex);
}

void MM_BakedShadow::Render(Wz4Mesh *meshin)
{
  sMatrix34 rot,roti;
  sVector31 v[8];
  sAABBoxC bounds;
  sAABBox sum;
  Wz4MeshCluster *cl;
  Wz4MeshFace *face;
  sBool pcf = (ShadowSize & 0x00100000);

  if(!sRender3DBegin())
    sVERIFYFALSE;

  // create dummy material for mesh

  Wz4Mesh *mesh = new Wz4Mesh;
  mesh->CopyFrom(meshin);
  sFORALL(mesh->Faces,face)
    face->Cluster = 0;
  mesh->ClearClusters();
  mesh->AddDefaultCluster();
  ModMtrl *mm = new ModMtrl();
  mm->SetMtrl(sMTRL_ZON|sMTRL_CULLON,0);
  mm->FeatureFlags |= MMFF_ShadowCastToAll;
  mesh->Clusters[0]->Mtrl = mm;

  // project bbox to right direction

  rot.LookAt(sVector31(-Dir),sVector31(0,0,0));
  roti = rot;
  roti.InvertOrthogonal();

  // shadow receivers determine size of area

  mesh->ChargeBBox();
  bounds.Clear();
  sFORALL(mesh->Clusters,cl)
    bounds.Add(cl->Bounds);
  sum.Init(bounds);

  if(ShadowSize & 0x00200000)
  {
    sAABBox limit;
    limit.Min = LimitCenter-LimitRadius;
    limit.Max = LimitCenter+LimitRadius;
    sum.And(limit);
  }

  sum.MakePoints(v);
  sum.Clear();
  for(sInt i=0;i<8;i++)
    sum.Add(v[i]*roti);

  sF32 _zoomx = sum.Max.x - sum.Min.x;
  sF32 _zoomy = sum.Max.y - sum.Min.y;
  sF32 _maxz = sum.Max.z;
  sVector31 _center = sum.Center();

  // shadow casters determine how far back we need the camera

  sum.Init(bounds);
  sum.MakePoints(v);
  sum.Clear();
  for(sInt i=0;i<8;i++)
    sum.Add(v[i]*roti);

  sF32 _minz = sum.Min.z;
  sF32 _dist = _center.z - _minz + 1;
  sF32 _depth = _maxz - _minz + 2;

  // set rendertarget

  sInt sxy = 1<<(ShadowSize&15);
  sTexture2D *ShadowMap;
  sTexture2D *DepthMap;
  if(pcf)
  {
    ShadowMap = sRTMan->Acquire(sxy,sxy,sTEX_R16F);
    DepthMap = new sTexture2D(sxy,sxy,sTEX_2D|sTEX_PCF16|sTEX_NOMIPMAPS|sTEX_RENDERTARGET);
  }
  else
  {
//    ShadowMap = new sTexture2D(sxy,sxy,sTEX_2D|sTEX_R16F|sTEX_NOMIPMAPS|sTEX_RENDERTARGET);
    ShadowMap = sRTMan->Acquire(sxy,sxy,sTEX_R16F);
    DepthMap = sRTMan->Acquire(sxy,sxy,sTEX_DEPTH16NOREAD);
  }
  sTargetSpec spec(ShadowMap,DepthMap);
  spec.Aspect = 1;

  // set up the right matrix         

  sViewport view;
  view.Camera.LookAt(_center*rot,_center*rot+Dir*_dist);
  view.Orthogonal = sVO_ORTHOGONAL;
  view.ClipNear = 0;
  view.ClipFar = _depth;
  view.SetTarget(spec);
  view.ZoomX = 2/_zoomx;
  view.ZoomY = 2/_zoomy;
  view.Prepare();

  // adjust for uv 

  sMatrix44 mat;
  mat.i.x = 0.5f;
  mat.j.y = -0.5f;
  mat.k.z = 1.0f;
  mat.l.Init(0.5f+0.5f/sxy,0.5f+0.5f/sxy,pcf ? -ShadowBaseBias/sxy : 0,1.0f);
  LightProj = view.ModelScreen*mat;
  LightFarR = 1.0f / view.ClipFar;
  LightMatZ.x = view.View.i.z;
  LightMatZ.y = view.View.j.z;
  LightMatZ.z = view.View.k.z;
  LightMatZ.w = view.View.l.z;
  LightMatZ *= LightFarR;
  ZShaderSlopeBias = ShadowSlopeBias;
  ZShaderBaseBiasOverClipFar = ShadowBaseBias * LightFarR;

  // render prepare

  RNRenderMesh *node = new RNRenderMesh;
  node->Para.BoneTime = 0;
  node->Para.Instances = 0;
  node->Para.LightEnv = 0;
  node->Para.Renderpass = 0;
  node->ParaBase = node->Para;
  node->Mesh = mesh; mesh->AddRef();
  
  Wz4RenderContext ctx;
  ctx.Init(Wz4RenderType->Script,&spec,0);
  ctx.IppHelper = Wz4RenderType->IppHelper;
  ctx.RenderFlags = wRF_RenderZ;
  ctx.View = view;

  ctx.DoScript(node,0,0);

  // render

  if(pcf)
  {
    ModMtrlType->LightEnv[0]->ZShaderSlopeBias = 0;
    ModMtrlType->LightEnv[0]->ZShaderBaseBiasOverClipFar = 0;
  }
  else
  {
    ModMtrlType->LightEnv[0]->ZShaderSlopeBias = ZShaderSlopeBias;
    ModMtrlType->LightEnv[0]->ZShaderBaseBiasOverClipFar = ZShaderBaseBiasOverClipFar;
  }

  ctx.RenderControlZ(node,spec);

  // render end

  delete node;

  // grab texture

  if(pcf)
  {
    Tex = new Texture2D;
    Tex->Texture = DepthMap;
    sRTMan->Release(ShadowMap);
  }
  else
  {
    const sU8 *data;
    sInt pitch;
    sTextureFlags flags;

    sImageData *id = new sImageData;
    id->Init2(sTEX_2D|sTEX_R16F,1,sxy,sxy,1);
    
    sBeginReadTexture(data,pitch,flags,ShadowMap);

    sU16 *dest = (sU16 *) id->Data;
    for(sInt y=0;y<sxy;y++)
    {
      for(sInt x=0;x<sxy;x++)
      {
        *dest++ = *(sU16 *)(data+pitch*y+2*x);
      }
    }

    sEndReadTexture();

    Tex = new Texture2D;
    Tex->ConvertFrom(id);

    sRTMan->Release(DepthMap);
    sRTMan->Release(ShadowMap);

    if(0)
    {
      sImage img(sxy,sxy);
      id->ConvertTo(&img);
      img.Save(L"c:/chaos/temp/test.bmp");
    }
  }

  // done 

  sRender3DEnd(0);
  delete mesh;
}

void MM_BakedShadow::Start(ShaderCreator *sc)
{
  VarName = 0;
 
}

sPoolString MM_BakedShadow::Get(ShaderCreator *sc)
{
  if(VarName == 0)
  {
    sMatrix44 mat;
    mat = LightProj;
    mat.Trans4();
    VarName = sc->GetTemp();
    sBool pcf = (ShadowSize & 0x00100000);
  
    sc->FragBegin(Name);
    sc->FragFirst(VarName);
    sc->TB.PrintF(L"  float4 %s;\n",VarName);
    sc->TB.PrintF(L"  { float z,d; float4 prpos; \n");

    sInt slot;
    if(pcf)
      slot = sc->Texture(Tex,sMTF_LEVEL1|sMTF_BORDER_WHITE|sMTF_PCF);
    else
      slot = sc->Texture(Tex,sMTF_LEVEL0|sMTF_BORDER_WHITE);

    sc->InputPS(sPoolF(L"bs_pos",slot),SCT_FLOAT4);
    sc->InputPS(sPoolF(L"bs_z",slot),SCT_FLOAT);

    sc->TB.PrintF(L"    prpos = bs_pos;\n",slot);
    sc->TB.PrintF(L"    z = bs_z;\n",slot);

    if((ShadowSize & 0x30000)==0x10000)
    {
      sc->TB.PrintF(L"    float4 dd; float2 uv; float f;\n");
      sc->TB.PrintF(L"    float2 ord[8] = {\n");
      sc->TB.PrintF(L"      {  1.0, 0.0 },\n");
      sc->TB.PrintF(L"      { -1.0, 0.0 },\n");
      sc->TB.PrintF(L"      {  0.0, 1.0 },\n");
      sc->TB.PrintF(L"      {  0.0,-1.0 },\n");

      sc->TB.PrintF(L"      { -0.5, 0.5 },\n");
      sc->TB.PrintF(L"      { -0.5,-0.5 },\n");
      sc->TB.PrintF(L"      {  0.5,-0.5 },\n");
      sc->TB.PrintF(L"      {  0.5, 0.5 },\n");
      sc->TB.PrintF(L"    };\n");
      sc->TB.PrintF(L"    d = 0;\n");
      sc->TB.PrintF(L"    f = %f;\n",ShadowFilter);
      if(pcf)
      {
        sc->TB.PrintF(L"    d = 0;\n");
        sc->TB.PrintF(L"    d += %s;\n",Tex2DProjCmp(slot,L"prpos.xy+ord[0]*f",L"prpos.z",L"prpos.w",L"x",0));
        sc->TB.PrintF(L"    d += %s;\n",Tex2DProjCmp(slot,L"prpos.xy+ord[1]*f",L"prpos.z",L"prpos.w",L"x",0));
        sc->TB.PrintF(L"    d += %s;\n",Tex2DProjCmp(slot,L"prpos.xy+ord[2]*f",L"prpos.z",L"prpos.w",L"x",0));
        sc->TB.PrintF(L"    d += %s;\n",Tex2DProjCmp(slot,L"prpos.xy+ord[3]*f",L"prpos.z",L"prpos.w",L"x",0));
        sc->TB.PrintF(L"    d += %s;\n",Tex2DProjCmp(slot,L"prpos.xy+ord[4]*f",L"prpos.z",L"prpos.w",L"x",0));
        sc->TB.PrintF(L"    d += %s;\n",Tex2DProjCmp(slot,L"prpos.xy+ord[5]*f",L"prpos.z",L"prpos.w",L"x",0));
        sc->TB.PrintF(L"    d += %s;\n",Tex2DProjCmp(slot,L"prpos.xy+ord[6]*f",L"prpos.z",L"prpos.w",L"x",0));
        sc->TB.PrintF(L"    d += %s;\n",Tex2DProjCmp(slot,L"prpos.xy+ord[7]*f",L"prpos.z",L"prpos.w",L"x",0));
/*
        sc->TB.PrintF(L"    d += tex2Dproj(tex%d,prpos+float4(ord[0]*f,0,0)).x;\n",slot);
        sc->TB.PrintF(L"    d += tex2Dproj(tex%d,prpos+float4(ord[1]*f,0,0)).x;\n",slot);
        sc->TB.PrintF(L"    d += tex2Dproj(tex%d,prpos+float4(ord[2]*f,0,0)).x;\n",slot);
        sc->TB.PrintF(L"    d += tex2Dproj(tex%d,prpos+float4(ord[3]*f,0,0)).x;\n",slot);
        sc->TB.PrintF(L"    d += tex2Dproj(tex%d,prpos+float4(ord[4]*f,0,0)).x;\n",slot);
        sc->TB.PrintF(L"    d += tex2Dproj(tex%d,prpos+float4(ord[5]*f,0,0)).x;\n",slot);
        sc->TB.PrintF(L"    d += tex2Dproj(tex%d,prpos+float4(ord[6]*f,0,0)).x;\n",slot);
        sc->TB.PrintF(L"    d += tex2Dproj(tex%d,prpos+float4(ord[7]*f,0,0)).x;\n",slot);
*/
        sc->TB.PrintF(L"    float shadow = d/8;\n");
      }
      else
      {
        sc->TB.PrintF(L"    uv = prpos.xy/prpos.w;\n");
        sc->TB.PrintF(L"    dd.x = %s;\n",Tex2D(slot,L"uv.xy+ord[0]*f",L"x",0));
        sc->TB.PrintF(L"    dd.y = %s;\n",Tex2D(slot,L"uv.xy+ord[1]*f",L"x",0));
        sc->TB.PrintF(L"    dd.z = %s;\n",Tex2D(slot,L"uv.xy+ord[2]*f",L"x",0));
        sc->TB.PrintF(L"    dd.w = %s;\n",Tex2D(slot,L"uv.xy+ord[3]*f",L"x",0));
        sc->TB.PrintF(L"    d += dot(dd<z,1);\n");
        sc->TB.PrintF(L"    dd.x = %s;\n",Tex2D(slot,L"uv.xy+ord[4]*f",L"x",0));
        sc->TB.PrintF(L"    dd.y = %s;\n",Tex2D(slot,L"uv.xy+ord[5]*f",L"x",0));
        sc->TB.PrintF(L"    dd.z = %s;\n",Tex2D(slot,L"uv.xy+ord[6]*f",L"x",0));
        sc->TB.PrintF(L"    dd.w = %s;\n",Tex2D(slot,L"uv.xy+ord[7]*f",L"x",0));
        sc->TB.PrintF(L"    d += dot(dd<z,1);\n");
        sc->TB.PrintF(L"    float shadow = 1-d/8;\n");
      }
    }
    else if((ShadowSize & 0x30000)==0x20000)
    {
      sc->Para(L"RandomDisc");
      sc->BindPS(L"vpos",SCT_FLOAT2,SCB_VPOS);
      sInt random = sc->Texture(ModMtrlType->SinCosTex,sMTF_LEVEL0|sMTF_TILE);
      sc->TB.PrintF(L"    float4 dd,rd,dsp,uv,m0,m1; float f;\n");
//      sc->TB.PrintF(L"    float2 sc = tex2D(tex%d,vpos.xy/64).xy*2-1;\n",random);
      sc->TB.PrintF(L"    float2 sc = %s*2-1;\n",Tex2D(random,L"vpos.xy/64",L"xy",0));
      sc->TB.PrintF(L"    f = %f;\n",ShadowFilter);
      sc->TB.PrintF(L"    uv = prpos.xyxy/prpos.w;\n");
      sc->TB.PrintF(L"    m0 = f*float4(sc.x, sc.y,sc.x, sc.y);\n");
      sc->TB.PrintF(L"    m1 = f*float4(sc.y,-sc.x,sc.y,-sc.x);\n");

      sc->TB.PrintF(L"    d = 0;\n");
      if(pcf)
      {
        for(sInt i=0;i<2;i++)
        {
          sc->TB.PrintF(L"    rd = RandomDisc[%d];\n",i*2+0);
          sc->TB.PrintF(L"    dsp = rd.xxzz*m0 + rd.yyww*m1 + prpos.xyxy;\n");
          sc->TB.PrintF(L"    d+= %s;\n",Tex2DProjCmp(slot,L"dsp.xy",L"prpos.z",L"prpos.w",L"x",0));
          sc->TB.PrintF(L"    d+= %s;\n",Tex2DProjCmp(slot,L"dsp.zw",L"prpos.z",L"prpos.w",L"x",0));

          sc->TB.PrintF(L"    rd = RandomDisc[%d];\n",i*2+1);
          sc->TB.PrintF(L"    dsp = rd.xxzz*m0 + rd.yyww*m1 + prpos.xyxy;\n");
          sc->TB.PrintF(L"    d+= %s;\n",Tex2DProjCmp(slot,L"dsp.xy",L"prpos.z",L"prpos.w",L"x",0));
          sc->TB.PrintF(L"    d+= %s;\n",Tex2DProjCmp(slot,L"dsp.zw",L"prpos.z",L"prpos.w",L"x",0));
        }
        sc->TB.PrintF(L"    float shadow = d/8;\n");
      }
      else
      {
        for(sInt i=0;i<2;i++)
        {
          sc->TB.PrintF(L"      rd = RandomDisc[%d];\n",i*2+0);
          sc->TB.PrintF(L"      dsp = rd.xxzz*m0 + rd.yyww*m1 + uv;\n");
          sc->TB.PrintF(L"      dd.x = %s;\n",Tex2D(slot,L"dsp.xy",L"x",0));
          sc->TB.PrintF(L"      dd.y = %s;\n",Tex2D(slot,L"dsp.zw",L"x",0));

          sc->TB.PrintF(L"      rd = RandomDisc[%d];\n",i*2+1);
          sc->TB.PrintF(L"      dsp = rd.xxzz*m0 + rd.yyww*m1 + uv;\n");
          sc->TB.PrintF(L"      dd.z = %s;\n",Tex2D(slot,L"dsp.xy",L"x",0));
          sc->TB.PrintF(L"      dd.w = %s;\n",Tex2D(slot,L"dsp.zw",L"x",0));
          sc->TB.PrintF(L"      d += dot(dd<z,1);\n");
        }
        sc->TB.PrintF(L"    float shadow = 1-d/8;\n");
      }
    }
    else
    {
      if(pcf)
      {
        sc->TB.PrintF(L"    float shadow = %s;\n",Tex2DProjCmp(slot,L"prpos.xy",L"prpos.z",L"prpos.w",L"x",0));
      }
      else
      {
        sc->TB.PrintF(L"    float shadow = 1-((%s)<z);\n",Tex2DProj(slot,L"prpos.xy",L"prpos.z",L"prpos.w",L"x",0));
      }
    }
    sc->TB.PrintF(L"    %s.xyzw = lerp(float4(%f,%f,%f,%f),1,shadow);\n",VarName,Color.x,Color.y,Color.z,Color.w);
    sc->TB.PrintF(L"  }\n");
    sc->FragEnd();

  }
  return sPoolString(VarName);
}


void MM_BakedShadow::VS(ShaderCreator *sc)
{

  sInt index;
    
  if(sc->Requires(L"bs_pos",index))
  {
    sc->FragBegin(Name);
    sc->Require(L"ms_pos",SCT_FLOAT3);
    sc->TB.PrintF(L"  float4x4 bs_LightProj = float4x4(float4(%f,%f,%f,%f),\n" ,LightProj.i.x,LightProj.i.y,LightProj.i.z,LightProj.i.w);
    sc->TB.PrintF(L"                                   float4(%f,%f,%f,%f),\n" ,LightProj.j.x,LightProj.j.y,LightProj.j.z,LightProj.j.w);
    sc->TB.PrintF(L"                                   float4(%f,%f,%f,%f),\n" ,LightProj.k.x,LightProj.k.y,LightProj.k.z,LightProj.k.w);
    sc->TB.PrintF(L"                                   float4(%f,%f,%f,%f));\n",LightProj.l.x,LightProj.l.y,LightProj.l.z,LightProj.l.w);
    sc->TB.PrintF(L"  float4 bs_pos = mul(float4(ms_pos,1),bs_LightProj);\n");
    if(index>=0)
      sc->TB.PrintF(L"  t%d.%s = bs_pos;\n",index/4,sc->swizzle[index&3][4]);
    sc->FragEnd();
  }

  if(sc->Requires(L"bs_z",index))
  {
    sc->FragBegin(Name);
    sc->Require(L"ms_pos",SCT_FLOAT3);
    sc->TB.PrintF(L"  float4 bs_LightMatZ = float4(%f,%f,%f,%f);",LightMatZ.x,LightMatZ.y,LightMatZ.z,LightMatZ.w);
    sc->TB.PrintF(L"  float bs_z = dot(float4(ms_pos,1),bs_LightMatZ);\n");
    if(index>=0)
      sc->TB.PrintF(L"  t%d.%s = bs_z;\n",index/4,sc->swizzle[index&3][1]);
    sc->FragEnd();
  }
}

/****************************************************************************/

