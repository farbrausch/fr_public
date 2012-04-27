/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) by Dierk Ohlerich                                    ***/
/***   all rights reserved                                                ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "fr033.hpp"
#include "fr033/w3texlib.hpp"
#include "fr033_ops.hpp"

#if sCONFIG_32BIT
#pragma comment(lib,"../wz4frlib/fr033/w3texlib_vc80_Rel.lib")
#endif

namespace Werkk3TexLib
{
  extern sInt GenBitmapTextureSizeOffset;
}

/****************************************************************************/

Wz3TexPackage::Wz3TexPackage()
{
  Type = Wz3TexPackageType;
}

Wz3TexPackage::~Wz3TexPackage()
{
  for(sInt i=0;i<Textures.GetCount();i++)
    Textures[i].Tex->Release();
}

sBool Wz3TexPackage::LoadKTX(const sChar *filename,sInt sizeOffset)
{
#if sCONFIG_32BIT
  sU8 *ktxFile = sLoadFile(filename);
  if(!ktxFile)
    return sFALSE;

  Werkk3TexLib::GenBitmapTextureSizeOffset = sizeOffset;

  W3TextureContext ctx;
  sBool ok = ctx.Load(ktxFile);
  if(!ok)
  {
    delete[] ktxFile;
    return sFALSE;
  }

  sDPrintF(L"calculating textures...\n");
  if(!ctx.Calculate())
    return sFALSE;

  delete[] ktxFile;
  sDPrintF(L"done.\n");

  // convert to our representation
  Textures.Clear();
  Textures.HintSize(ctx.GetImageCount());

  for(sInt i=0;i<ctx.GetImageCount();i++)
  {
    Wz3Texture *tex = Textures.AddMany(1);
    const W3Image *img = ctx.GetImageByNumber(i);

    sCopyString(tex->Name,ctx.GetImageName(i));
    sImage outImg(img->XSize,img->YSize);

    if(img->Format == W3IF_BGRA8)
      sCopyMem(outImg.Data,img->MipData[0],img->XSize*img->YSize*4);
    else if(img->Format == W3IF_UVWQ8)
    {
      // needs some conversion (signed->unsigned)
      sInt nPixels = img->XSize*img->YSize;
      const sS8 *src = (const sS8*) img->MipData[0];
      sU8 *dst = (sU8 *) outImg.Data;

      for(sInt i=0;i<nPixels;i++)
      {
        dst[i*4+0] = 0x80 + src[i*4+0];
        dst[i*4+1] = 0x80 + src[i*4+1];
        dst[i*4+2] = 0x80 + src[i*4+2];
        dst[i*4+3] = 0x80 + src[i*4+3];
      }
    }
    else
      sDPrintF(L"texture %d (%q) has unsupported format %d, leaving it black.\n",i,tex->Name,img->Format);

    tex->Tex = new Texture2D;
    tex->Tex->ConvertFrom(&outImg,sTEX_2D|sTEX_ARGB8888);
  }

  return sTRUE;
#else
  return sFALSE;
#endif
}

Texture2D *Wz3TexPackage::Lookup(const sChar *name)
{
  Wz3Texture *tex;
  sFORALL(Textures,tex)
  {
    if(sCmpString(tex->Name,name) == 0)
      return tex->Tex;
  }

  return 0;
}

/****************************************************************************/

RNLimitTransform::RNLimitTransform()
{
  Anim.Init(Wz4RenderType->Script);
}

void RNLimitTransform::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);

  SimulateChilds(ctx);
}

void RNLimitTransform::Transform(Wz4RenderContext *ctx,const sMatrix34 &mat)
{
  sMatrix34 mat1(mat);
  mat1.l.y = sMax(0.0f,mat1.l.y);
  TransformChilds(ctx,mat1);
}

/****************************************************************************/

RNTrembleMesh::RNTrembleMesh()
{
  Mesh = 0;
  Anim.Init(Wz4RenderType->Script);
}

RNTrembleMesh::~RNTrembleMesh()
{
  Mesh->Release();
}

void RNTrembleMesh::Init()
{
  Bounds.Clear();
  Wz4MeshCluster *cl;
  Mesh->ChargeBBox();
  sFORALL(Mesh->Clusters,cl)
    Bounds.Add(cl->Bounds);
}

void RNTrembleMesh::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
}

void RNTrembleMesh::Prepare(Wz4RenderContext *ctx)
{
  Mesh->BeforeFrame(Para.LightEnv,Matrices.GetCount(),Matrices.GetData());
}

void RNTrembleMesh::Render(Wz4RenderContext *ctx)
{
  sMatrix34 mt,m0,ma,ms;
  sF32 time = Para.Anim*Para.Frequency;
  mt.EulerXYZ(sSin(1.1f+0.4f*time)*Para.Amount
             ,sSin(3.2f+0.5f*time)*Para.Amount
             ,0);


  ma.l = Bounds.Center;
  ms.l = -ma.l;

  mt = ms*mt*ma;

  sMatrix34CM *mat;
  sFORALL(Matrices,mat)
  {
    m0 = mt*sMatrix34(*mat);
    Mesh->Render(ctx->RenderMode,Para.LightEnv,&sMatrix34CM(m0),0,ctx->Frustum);
  }
}

/****************************************************************************/
