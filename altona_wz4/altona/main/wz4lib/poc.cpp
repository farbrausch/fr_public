/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) by Dierk Ohlerich                                    ***/
/***   all rights reserverd                                               ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "poc.hpp"
#include "poc_ops.hpp"

/****************************************************************************/

PocBitmap::PocBitmap()
{
  Type = PocBitmapType;
  Image = new sImage();
  Texture = 0;
}

PocBitmap::~PocBitmap()
{
  delete Image;
  delete Texture;
}

wObject *PocBitmap::Copy()
{
  PocBitmap *d = new PocBitmap;
  d->CopyFrom(this);
  return d;
}

void PocBitmap::CopyTo(sImage *dest)
{
  dest->Copy(Image);
}

void PocBitmap::CopyTo(sImageI16 *dest)
{
  dest->CopyFrom(Image);
}

void PocBitmap::CopyTo(sImageData *dest,sInt format)
{
  dest->Init2(format|sTEX_2D,0,Image->SizeX,Image->SizeY,1);
  dest->ConvertFrom(Image);
}

void PocBitmap::CopyFrom(const PocBitmap *src)
{
  Image->Copy(src->Image);
  Atlas = src->Atlas;
}

/****************************************************************************/

PocMaterial::PocMaterial()
{
  Type = PocMaterialType;
  Material = 0; 
  sClear(Tex);
  Format = sVertexFormatStandard;
}
PocMaterial::~PocMaterial() 
{ 
  delete Material; 
  for(sInt i=0;i<sCOUNTOF(Tex);i++)
    sRelease(Tex[i]);
}
wObject *PocMaterial::Copy()
{
  PocMaterial *d = new PocMaterial;

  d->Format = Format;
  d->Material = new sSimpleMaterial;
  d->Material->Flags = Material->Flags;
  d->Material->BlendColor = Material->BlendColor;
  d->Material->BlendAlpha = Material->BlendAlpha;
  for(sInt i=0;i<8;i++)
  {
    d->Tex[i] = Tex[i]; Tex[i]->AddRef();
    if(Tex[i])
    {
      d->Material->Texture[i] = Material->Texture[i];
      d->Material->TFlags[i] = Material->TFlags[i];
    }
  }
  d->Material->Prepare(Format);

  return d;
}

/****************************************************************************/

PocMesh::PocMesh()
{
  Type = PocMeshType;
  SolidGeo = 0;
}

PocMesh::~PocMesh()
{
  PocMeshCluster *cl;
  sFORALL(Clusters,cl)
    cl->Mtrl->Release();
  delete SolidGeo;
}

wObject *PocMesh::Copy()
{
  PocMesh *d = new PocMesh;

  d->Copy(this);

  return d;
}

void PocMesh::Init(sInt vc,sInt ic,sInt cc)
{
  PocMeshCluster *cl;
  
  Vertices.Resize(vc);
  Indices.Resize(ic);
  Clusters.Resize(cc);
  
  sFORALL(Clusters,cl)
  {
    cl->StartIndex = 0;
    cl->EndIndex = ic;
    cl->Mtrl = 0;
  }
}

void PocMesh::CacheSolid()
{
  sVertexStandard *vd;
  PocMeshVertex *vs;
  sU32 *id;
  if(!SolidGeo)
  {
    SolidGeo = new sGeometry;
    SolidGeo->Init(sGF_TRILIST|sGF_INDEX32,sVertexFormatStandard);
    SolidGeo->BeginLoadVB(Vertices.GetCount(),sGD_STATIC,(void **)&vd);
    sFORALL(Vertices,vs)
    {
      vd->Init(vs->Pos,vs->Normal,vs->u,vs->v);
      vd++;
    }
    SolidGeo->EndLoadVB();
    SolidGeo->BeginLoadIB(Indices.GetCount(),sGD_STATIC,(void **)&id);
    sCopyMem(id,&Indices[0],sizeof(sU32)*Indices.GetCount());
    SolidGeo->EndLoadIB();
  }
}
void PocMesh::Copy(PocMesh *src)
{
  Vertices = src->Vertices;
  Indices = src->Indices;
  Clusters = src->Clusters;
}
void PocMesh::Hit(const sRay &ray,wHitInfo &info)
{
  PocMeshCluster *cl;
  sFORALL(Clusters,cl)
  {
    for(sInt i=cl->StartIndex;cl->EndIndex;i+=3)
    {
      const PocMeshVertex *v0,*v1,*v2;
      sF32 dist;

      v0 = &Vertices[Indices[i+0]];
      v1 = &Vertices[Indices[i+1]];
      v2 = &Vertices[Indices[i+2]];
      if(ray.HitTriangle(dist,v0->Pos,v1->Pos,v2->Pos))
      {
        if(!info.Hit || info.Dist > dist)
        {
          info.Hit = 1;
          info.Pos = ray.Start + ray.Dir*dist;
          info.Normal.Cross(v0->Pos-v1->Pos,v1->Pos-v2->Pos);
          info.Dist = dist;
          info.Mesh = this;
          info.Material = cl->Mtrl;
        }
      }
    }
  }
}

void PocMesh::Wireframe(wObject *obj,wPaintInfo &pi,sGeometry *geo,sMatrix34 &mat)
{
  sVertexSingle *vd;
  PocMeshVertex *vs;
  sInt *id;
  sCBuffer<sSimpleMaterialEnvPara> cb; cb.Data->Set(*pi.View,*pi.Env);
  pi.FlatMtrl->Set(&cb);
  geo->BeginLoadVB(Vertices.GetCount(),sGD_STREAM,(void **)&vd);
  sFORALL(Vertices,vs)
  {
    vd->Init(vs->Pos * mat,~0,0,0);
    vd++;
  }
  geo->EndLoadVB();
  geo->BeginLoadIB(Indices.GetCount()/3*6,sGD_STREAM,(void **)&id);
  for(sInt i=0;i<Indices.GetCount();i+=3)
  {
    sInt i0,i1,i2;

    i0 = Indices[i+0];
    i1 = Indices[i+1];
    i2 = Indices[i+2];

    id[0] = i0;
    id[1] = i1;
    id[2] = i1;
    id[3] = i2;
    id[4] = i2;
    id[5] = i0;
    id+=6;
  }
  geo->EndLoadIB();
  geo->Draw();
}


/****************************************************************************/
/***                                                                      ***/
/***   Bitmap Atlas                                                       ***/
/***                                                                      ***/
/****************************************************************************/

void PocBitmapMakeAtlas(sStaticArray<PocBitmapAtlas> &in,sImage *out,sU32 emptycol,sInt mode)
{
  PocBitmapAtlas *atlas;
  sArray<sRect> Free;
  sRect *r;
  sRect q;
  sInt FullX,FullY;
  
  sFORALL(in,atlas)
  {
    atlas->SizeX = atlas->Image->SizeX;
    atlas->SizeY = atlas->Image->SizeY;
    atlas->Order = atlas->Image->SizeX + atlas->Image->SizeY;
    atlas->PosX = 0;
    atlas->PosY = 0;
  }

  if (mode==PBMA_AUTO)
  {
    sSortDown(in,&PocBitmapAtlas::Order);

    // add first rect

    FullX = in[0].SizeX;
    FullY = in[0].SizeY;
    in[0].PosX = 0;
    in[0].PosY = 0;

    // add more rects

    for(sInt i=1;i<in.GetCount();i++)
    {
      // find best fitting free
      atlas = &in[i];
  rescan:
      sInt best = -1;
      sInt bestscore = FullX*FullY+1;
      sFORALL(Free,r)
      {
        sInt score = r->SizeX()*r->SizeY();
        if(score<bestscore && r->SizeX()>=atlas->SizeX && r->SizeY()>=atlas->SizeY)
        {
          best = _i;
          bestscore = score;
        }
      }
      // no fit found, increase size
      if(best==-1)
      {
        r = Free.AddMany(1);
        if(FullX>FullY)
        {
          r->Init(0,FullY,FullX,FullY*2);
          FullY*=2;
        }
        else
        {
          r->Init(FullX,0,FullX*2,FullY);
          FullX*=2;
        }
        goto rescan;
      }
      // fit in
      q = *r;
      Free.RemAt(best);
      atlas->PosX = q.x0;
      atlas->PosY = q.y0;
      
      if(q.SizeX() > atlas->SizeX)
      {
        r = Free.AddMany(1);
        *r = q;
        r->x0 += atlas->SizeX;
        q.x1 = r->x0;
      }
      if(q.SizeY() > atlas->SizeY)
      {
        r = Free.AddMany(1);
        *r = q;
        r->y0 += atlas->SizeY;
        q.y1 = r->y0;
      }
    }
  }
  else if (mode==PBMA_HORIZONTAL)
  {
    FullX=FullY=0;
    sFORALL(in, atlas)
    {
      FullY=sMax(FullY,atlas->SizeY);
      atlas->PosX=FullX;
      FullX+=atlas->SizeX;
    }
  }
  else
  {
    FullX=FullY=0;
    sFORALL(in, atlas)
    {
      FullX=sMax(FullX,atlas->SizeX);
      atlas->PosY=FullY;
      FullY+=atlas->SizeY;
    }
  }

  // copy together
  out->Init(FullX,FullY);
  out->Fill(emptycol);
  sFORALL(in,atlas)
  {
    out->BlitFrom(atlas->Image,0,0,atlas->PosX,atlas->PosY,atlas->Image->SizeX,atlas->Image->SizeY);
  }
}
