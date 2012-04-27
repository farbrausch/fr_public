/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "wz4_mtrl.hpp"
#include "wz4_mtrl_shader.hpp"
#include "wz4_mtrl_ops.hpp"
#include "wz4lib/serials.hpp"

/****************************************************************************/

Wz4Shader::Wz4Shader()
{
  sClear(UVMatrix0); UVMatrix0[0][0] = UVMatrix0[1][1] = 1;
  sClear(UVMatrix1); UVMatrix1[0][0] = UVMatrix1[1][1] = 1;
  UVMatrix2.Init();
  DetailMode = 0;
}

void Wz4Shader::SelectShaders(sVertexFormatHandle *fmt)
{
//  sU32 buffer[512];
//  sU32 *data=buffer;

  // sanity checking

  if((DetailMode&0x0f)==3 && !Texture[0]) DetailMode &= ~0x0f;    // detail bump
  if((DetailMode&0x0f)==4 && !Texture[3]) DetailMode &= ~0x0f;    // gloss

  // pick vertex shader

  sInt ps = 0;
  sInt vs = 0;

  if(DetailMode & 0x200)
  {
    if(fmt->Has(sVF_BONEINDEX)) vs |= Wz4CShaderVSPermMask_SkinEnable;

    VertexShader = Wz4CShader::GetVS(vs);
    PixelShader = Wz4CShader::GetPS(ps);
  }
  else if(Texture[0]==0 && Texture[3]==0 && Texture[4]==0 && Texture[6]==0 && Texture[1]!=0)
  {
    if(Texture[2])
    {
      switch(DetailMode&0x0f)
      {
      case 1: ps |= Wz4LShaderPSPerm_TexDetailMul; break;
      case 2: ps |= Wz4LShaderPSPerm_TexDetailAdd; break;
      }
      switch(DetailMode&0xf0)
      {
      default: 
        vs |= Wz4LShaderVSPerm_DetailUV0; 
        break;
      case 0x10:
        vs |= Wz4LShaderVSPerm_DetailUV1; 
        break;
      case 0x20:
      case 0x30:
        vs |= Wz4LShaderVSPerm_DetailPos; 
        break;
      case 0x40:
        vs |= Wz4LShaderVSPerm_DetailNorm; 
        break;
      case 0x50:
        vs |= Wz4LShaderVSPerm_DetailRefl; 
        break;
      }
    }
//    if(Texture[5]) ps |= Wz4LShaderPSPermMask_TexEnvi;

    if(fmt->Has(sVF_BONEINDEX)) vs |= Wz4LShaderVSPermMask_SkinEnable;
    if(DetailMode & 0x400)  
      ps |= Wz4LShaderPSPermMask_AlphaDiffuse;

    VertexShader = Wz4LShader::GetVS(vs);
    PixelShader = Wz4LShader::GetPS(ps);
  }
  else
  {
    if(Texture[0])
    {
      ps |= Wz4ShaderPSPermMask_TexBump;
      vs |= Wz4ShaderVSPermMask_NeedTangent;
    }
    if(Texture[3]) ps |= Wz4ShaderPSPermMask_TexSpecularCube;
    if(Texture[4]) ps |= Wz4ShaderPSPermMask_TexDiffuseCube;
//    if(Texture[5]) ps |= Wz4ShaderPSPermMask_TexEnvi;
    if(Texture[6]) 
    {
      ps |= Wz4ShaderPSPerm_ShadowReceive;
      vs |= Wz4ShaderVSPerm_ShadowReceive; 
    }

    if(Texture[2])
    {
      switch(DetailMode&0x0f)
      {
      case 1: ps |= Wz4ShaderPSPerm_TexDetailMul; break;
      case 2: ps |= Wz4ShaderPSPerm_TexDetailAdd; break;
      case 3: ps |= Wz4ShaderPSPerm_TexDetailBump; break;
      case 4: ps |= Wz4ShaderPSPerm_TexDetailSpecMul; break;
      }
      switch(DetailMode&0xf0)
      {
      default: 
        vs |= Wz4ShaderVSPerm_DetailUV0; 
        break;
      case 0x10:
        vs |= Wz4ShaderVSPerm_DetailUV1; 
        break;
      case 0x20:
      case 0x30:
        vs |= Wz4ShaderVSPerm_DetailPos; 
        break;
      case 0x40:
        vs |= Wz4ShaderVSPerm_DetailNorm; 
        break;
      case 0x50:
        vs |= Wz4ShaderVSPerm_DetailRefl; 
        break;
      }
    }
    if(DetailMode & 0x100)  
      ps |= Wz4ShaderPSPerm_AlphaDist;
    if(DetailMode & 0x400)  
      ps |= Wz4ShaderPSPerm_AlphaDiffuse;


    if(fmt->Has(sVF_BONEINDEX)) vs |= Wz4ShaderVSPermMask_SkinEnable;

    VertexShader = Wz4XShader::GetVS(vs);
    PixelShader = Wz4XShader::GetPS(ps);
  }
}


void Wz4Shader::MakeMatrix(sInt id,sInt flags,sF32 scale,sF32 *m0,sF32 *m1)
{
  sMatrix34 mat;
  mat.Init();
 
  switch(flags & 0xc000)
  {
  default:
    break;
  case 0x4000:
    mat.i.x = scale;
    mat.j.y = scale;
    mat.k.z = scale;
    break;
  case 0x8000:
    mat.EulerXYZ(0,0,m0[2]);
    mat.i *= m0[0];
    mat.j *= m0[1];
    mat.l.x = m0[3];
    mat.l.y = m0[4];
    break;
  case 0xc000:
    mat.EulerXYZ(m1[3],m1[4],m1[5]);
    mat.i *= m1[0];
    mat.j *= m1[1];
    mat.k *= m1[2];
    mat.l.x = m1[6];
    mat.l.y = m1[7];
    mat.l.z = m1[8];
    break;
  }

  switch(id)
  {
  default:
    UVMatrix0[0].Init(mat.i.x,mat.j.x,mat.k.x,mat.l.x);
    UVMatrix0[1].Init(mat.i.y,mat.j.y,mat.k.y,mat.l.y);
    break;
  case 1:
    UVMatrix1[0].Init(mat.i.x,mat.j.x,mat.k.x,mat.l.x);
    UVMatrix1[1].Init(mat.i.y,mat.j.y,mat.k.y,mat.l.y);
    break;
  case 2:
    UVMatrix2 = mat;
    break;
  }
}

void Wz4Shader::SetUV(Wz4ShaderUV *cb,const sViewport *vp)
{
  cb->UV[0] = UVMatrix0[0];
  cb->UV[1] = UVMatrix0[1];
  cb->UV[2] = UVMatrix1[0];
  cb->UV[3] = UVMatrix1[1];

  sMatrix34 mat;

  switch(DetailMode&0xf0)
  {
  default:
  case 0x10:
  case 0x20:
    mat = UVMatrix2;
    break;
  case 0x30:
    mat = vp->Model * UVMatrix2;
    break;
  case 0x40:
  case 0x50:
    mat = vp->ModelView * UVMatrix2;
//    mat.TransR();

    mat.i.x *= 0.5f;
    mat.i.y *= 0.5f;
    mat.j.x *= 0.5f;
    mat.j.y *= 0.5f;
    mat.k.x *= 0.5f;
    mat.k.y *= 0.5f;
    
    mat.l.Init(0.5f,0.5f,0);
//    mat.l.x += 0.5f;
//    mat.l.y += 0.5f;
    break;
  }

  cb->UV[4].Init(mat.i.x,mat.j.x,mat.k.x,mat.l.x);
  cb->UV[5].Init(mat.i.y,mat.j.y,mat.k.y,mat.l.y);
}

void Wz4Shader::SetCam(Wz4ShaderCamera *cb,const sViewport *vp)
{
  sMatrix34 mat;
  cb->MVP = vp->ModelScreen;

  mat = vp->Model;
  mat.TransR();
  cb->LightPos = vp->Camera.l * mat;
  cb->EyePos = vp->Camera.l * mat;
  cb->EyePos.w = 1/16.0f;
}

/****************************************************************************/

void Wz4Shader::SetUV(Wz4ShaderUV *cb,const Wz4ShaderEnv *env)
{
  cb->UV[0] = UVMatrix0[0];
  cb->UV[1] = UVMatrix0[1];
  cb->UV[2] = UVMatrix1[0];
  cb->UV[3] = UVMatrix1[1];

  sMatrix34 mat;

  switch(DetailMode&0xf0)
  {
  default:
  case 0x10:
  case 0x20:
    mat = UVMatrix2;
    break;
  case 0x30:
    mat = env->Model * UVMatrix2;
    break;
  case 0x40:
    mat = env->ModelView * UVMatrix2;
//    mat.TransR();

    mat.i.x *= 0.5f;
    mat.i.y *= 0.5f;
    mat.j.x *= 0.5f;
    mat.j.y *= 0.5f;
    mat.k.x *= 0.5f;
    mat.k.y *= 0.5f;
    
    mat.l.Init(0.5f,0.5f,0);
    break;
  case 0x50:
    mat.i.Init(0.5f,0,0);
    mat.j.Init(0,0.5f,0);
    mat.k.Init(0,0,0);
    mat.l.Init(0.5f,0.5f,0);
    break;
  }

  cb->UV[4].Init(mat.i.x,mat.j.x,mat.k.x,mat.l.x);
  cb->UV[5].Init(mat.i.y,mat.j.y,mat.k.y,mat.l.y);
}

void Wz4Shader::SetCam(Wz4ShaderCamera *cb,const Wz4ShaderEnv *env)
{
  cb->MVP = env->ModelScreen;
  cb->LightPos = env->Camera.l * env->ModelI;
  cb->EyePos = env->Camera.l * env->ModelI;
  cb->EyePos.w = env->FocalRangeI;
  cb->ShadowMatrix = env->ShadowMat;
}

void Wz4ShaderEnv::Set(const sViewport *vp,sF32 focalrange)
{
  ModelScreen = vp->ModelScreen;
  Model = vp->Model;
  ModelView = vp->ModelView;
  ModelI = vp->Model;
  ModelI.TransR();
  Camera = vp->Camera;
  FocalRangeI = 1/focalrange;
  FadeColor.Init(1,1,1,1);
  ShadowMat.Init();
  ClipNear = vp->ClipNear;
  ClipFar = vp->ClipFar;
  ZoomX = vp->ZoomX;
  ZoomY = vp->ZoomY;
}

void Wz4ShaderEnv::Set(const sViewport *vp,const sMatrix34 &mat)
{
  ModelScreen = mat*vp->ModelScreen;
  Model = mat*vp->Model;
  ModelView = mat*vp->ModelView;
  ModelI = Model;
  ModelI.TransR();
  Camera = vp->Camera;
  ClipNear = vp->ClipNear;
  ClipFar = vp->ClipFar;
  ZoomX = vp->ZoomX;
  ZoomY = vp->ZoomY;
}

void Wz4ShaderEnv::SetShadow(const sViewport *vp)
{
  sMatrix44 mat;
  mat.i.x = 0.5f;
  mat.j.y = -0.5f;
  mat.k.z = 1.0f;
  mat.l.Init(0.5f,0.5f,0.0f,1.0f);
  ShadowMat = vp->ModelScreen*mat;
}

sInt Wz4ShaderEnv::Visible(const sAABBox &box) const
{
  register const sMatrix34 &mv=ModelView;
  register const sF32 zx=ZoomX;
  register const sF32 zy=ZoomY;
  sVector30 r[2][3];
  r[0][0].x = (mv.i.x*box.Min.x+mv.l.x)*zx; r[0][0].y=(mv.i.y*box.Min.x)*zy; r[0][0].z=mv.i.z*box.Min.x;
  r[0][1].x = (mv.j.x*box.Min.y)*zx; r[0][1].y=(mv.j.y*box.Min.y+mv.l.y)*zy; r[0][1].z=mv.j.z*box.Min.y;
  r[0][2].x = (mv.k.x*box.Min.z)*zx; r[0][2].y=(mv.k.y*box.Min.z)*zy; r[0][2].z=mv.k.z*box.Min.z+mv.l.z;
  r[1][0].x = (mv.i.x*box.Max.x+mv.l.x)*zx; r[1][0].y=(mv.i.y*box.Max.x)*zy; r[1][0].z=mv.i.z*box.Max.x;
  r[1][1].x = (mv.j.x*box.Max.y)*zx; r[1][1].y=(mv.j.y*box.Max.y+mv.l.y)*zy; r[1][1].z=mv.j.z*box.Max.y;
  r[1][2].x = (mv.k.x*box.Max.z)*zx; r[1][2].y=(mv.k.y*box.Max.z)*zy; r[1][2].z=mv.k.z*box.Max.z+mv.l.z;

  register sU32 amask = ~0;
  register sU32 omask = 0;
  for(sInt i=0;i<8;i++)
  {
    register const sVector30 t = r[(i&1)][0]+r[(i&2)>>1][1]+r[(i&4)>>2][2];

    register sU32 clip=0;
    if(t.x> t.z) clip |= 0x01;
    else if(t.x<-t.z) clip |= 0x04;
    if(t.y> t.z) clip |= 0x02;
    else if(t.y<-t.z) clip |= 0x08;
    if(t.z<ClipNear) clip |= 0x10;
    else if(t.z>ClipFar) clip |= 0x20;

    amask &= clip;
    omask |= clip;
  }
  if(amask!=0) return 0;          // total out
  if(omask==0) return 2;          // total in
  return 1;                       // part in
}


/****************************************************************************/

Wz4Material::Wz4Material() 
{
  Type = Wz4MaterialType;
  Material = 0; 
  TempPtr = 0;
  sClear(Tex);
  Format = sVertexFormatTSpace4;
}

Wz4Material::~Wz4Material() 
{ 
  delete Material; 
  for(sInt i=0;i<sCOUNTOF(Tex);i++)
    sRelease(Tex[i]);
}

void Wz4Material::CopyFrom(Wz4Material *src)
{
  Material = new Wz4Shader;

  Format = src->Format;
  Name = src->Name;
  TempPtr = 0;
  for(sInt i=0;i<sCOUNTOF(Tex);i++)
  {
    Tex[i] = src->Tex[i];
    if(Tex[i])
      Tex[i]->AddRef();
  }

  Material->UVMatrix0[0] = src->Material->UVMatrix0[0];
  Material->UVMatrix0[1] = src->Material->UVMatrix0[1];
  Material->UVMatrix1[0] = src->Material->UVMatrix1[0];
  Material->UVMatrix1[1] = src->Material->UVMatrix1[1];
  Material->UVMatrix2 = src->Material->UVMatrix2;
  Material->DetailMode = src->Material->DetailMode;

  Material->CopyBaseFrom(src->Material);
}


template <class streamer> void Wz4Material::Serialize_(streamer &stream,sTexture2D *shadow)
{
  sInt version = stream.Header(sSerId::Wz4Material,1);
  Texture2D *tex2d;
  TextureCube *texcube;
  sInt n;

  if(version)
  {
    sInt maxtex=8;
    sVERIFY(maxtex==sCOUNTOF(Tex));

    if(stream.IsReading())
      Material = new Wz4Shader;

    // top level

    stream | Name;
    sStreamVertexFormat(stream,Format);
    for(sInt i=0;i<maxtex;i++)
    {
      tex2d = 0;
      texcube = 0;
      n = 0;
      if(stream.IsWriting())
      {
        if(Tex[i])
        {
          if(Tex[i]->Type==Texture2DType)
          {
            tex2d = (Texture2D *) Tex[i];
            n = 1;
          }
          else if(Tex[i]->Type==TextureCubeType)
          {
            texcube = (TextureCube *) Tex[i];
            n = 2;
          }
        }
      }

      stream | n;
      switch(n)
      {
        case 1: stream.OnceRef(tex2d); break;
        case 2: stream.OnceRef(texcube); break;
      }

      if(stream.IsReading())
      {
        switch(n)
        {
        case 0:
          Tex[i] = 0;
          Material->Texture[i] = 0;
          break;
        case 1:
          Tex[i] = tex2d;
          Material->Texture[i] = tex2d->Texture;
          break;
        case 2:
          Tex[i] = texcube;
          Material->Texture[i] = texcube->Texture;
          break;
        }
      }
    }

    // general material properties

    for(sInt i=0;i<maxtex;i++)
      stream | Material->TFlags[i];
    stream | Material->NameId | Material->Flags;
    stream | Material->BlendColor | Material->BlendAlpha | Material->BlendFactor;

    for(sInt i=0;i<4;i++) stream.U8(Material->FuncFlags[i]);
    for(sInt i=0;i<6;i++) stream.U8(Material->StencilOps[i]);
    stream.U8(Material->StencilRef);
    stream.U8(Material->StencilMask);
    stream.U8(Material->AlphaRef);
    stream.Align();

    // special material properties

    stream | Material->UVMatrix0[0] | Material->UVMatrix0[1];
    stream | Material->UVMatrix1[0] | Material->UVMatrix1[1];
    stream | Material->UVMatrix2;
    stream | Material->DetailMode;

    if(stream.IsReading())
    {
      
      if(Material->Texture[5] && !Material->Texture[2])  // oldschool envi mode: bad idea
      {
        Material->Texture[2] = Material->Texture[5];
        Material->Texture[5] = 0;
        Material->DetailMode = (Material->DetailMode & ~0xff) | 0x51;
      }
      
      if(shadow && Material->BlendColor==0)
      {
        Material->Texture[6] = shadow;
//        Material->TFlags[6] = sMTF_LEVEL2 | sMTF_BORDER_BLACK;      // debug: see shadowmap border clearly
        Material->TFlags[6] = sMTF_LEVEL2 | sMTF_BORDER_WHITE;      // no debug
      }
      Material->Prepare(Format);
    }

    stream.Footer();
  }
}
void Wz4Material::Serialize(sWriter &stream,sTexture2D *shadow) { Serialize_(stream,0); }
void Wz4Material::Serialize(sReader &stream,sTexture2D *shadow) { Serialize_(stream,shadow); }


/****************************************************************************/
