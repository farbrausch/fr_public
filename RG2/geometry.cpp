// This code is in the public domain. See LICENSE for details.

#include "types.h"
#include "rtlib.h"
#include "geometry.h"
#include "math3d_2.h"

static sU32 globalRevCounter=0;

#pragma warning (disable: 4018)

// ---- frMesh

frMesh::frMesh()
{
  vertices=0;
  indices=0;
  basis=0;
  nVerts=nPrims=0;
  refCount=0;
  type=0;
  vFormat=0;
  vbuf=0;
  ibuf=0;
  bRadius=0.0f;
}

frMesh::frMesh(const frMesh &x)
{
  nVerts=x.nVerts;
  nPrims=x.nPrims;
  vertices=new frVertex[nVerts];
  indices=new sU16[nPrims*(3-x.type)];
  refCount=0;
  type=x.type;
  vFormat=x.vFormat;
  vbuf=0;
  ibuf=0;
  bPos=x.bPos;
  bRadius=x.bRadius;

  copyData(x);
}

frMesh::~frMesh()
{
  freeData();
}

void frMesh::freeData()
{
  delete[] vertices;
  delete[] indices;
  delete[] basis;
}

void frMesh::copyData(const frMesh &src)
{
  memcpy(vertices, src.vertices, sizeof(frVertex)*src.nVerts); // kinda strange semantics enforced by the way copyData is used :)
  memcpy(indices, src.indices, sizeof(sU16)*src.nPrims*(3-src.type));
  basis=0;
}

void frMesh::resize(sU32 verts, sU32 prims)
{
  if (verts!=nVerts || prims!=nPrims)
  {
    freeData();

    nVerts=verts;
    nPrims=prims;

    vertices=new frVertex[nVerts];
    indices=new sU16[nPrims*(3-type)];
    basis=0;
  }
  else
  {
    delete[] basis;
    basis=0;
  }
}

void frMesh::transform(const fr::matrix &mtx, const fr::matrix &texmtx, sInt texMode)
{
  fr::matrix onorm;
  onorm.invTrans3x3(mtx);

  delete[] basis;
  basis=0;

  for (sInt i=0; i<nVerts; i++)
  {
    vertices[i].pos*=mtx;

    if (vFormat==0)
    {
      vertices[i].norm*=onorm;
      vertices[i].norm.norm();
    }

    if (texMode!=0)
    {
      fr::vector uvc(vertices[i].u, vertices[i].v, 0.0f);
      uvc*=texmtx;

      // ...projected textures are not available anyway

      vertices[i].u=uvc.x;
      vertices[i].v=uvc.y;
    }
  }

  calcBoundingSphere();
}

void frMesh::calcBoundingSphere() // nach jack ritter
{
  fr::vector  abMin, abMax;
  sInt        inds[6], i;
  
  bRadius=0.0f;

  for (i=0; i<nVerts; i++)
  {
    const fr::vector &pos=vertices[i].pos;

    for (sInt j=0; j<3; j++)
    {
      if (!i || pos.v[j]<abMin.v[j])
      {
        abMin.v[j]=pos.v[j];
        inds[j*2+0]=i;
      }
      
      if (!i || pos.v[j]>abMax.v[j])
      {
        abMax.v[j]=pos.v[j];
        inds[j*2+1]=i;
      }
    }
  }

  for (i=0; i<3; i++)
  {
    const fr::vector &lo=vertices[inds[i*2+0]].pos;
    const fr::vector &hi=vertices[inds[i*2+1]].pos;
    const sF32 radius=lo.distanceTo(hi)*0.5f;
    
    if (radius>bRadius)
    {
      bPos.lerp(lo, hi, 0.5f);
      bRadius=radius;
    }
  }

  sF32 rr=bRadius*bRadius;

  for (i=0; i<nVerts; i++)
  {
    const fr::vector &pos=vertices[i].pos;
    const sF32 distSquare=pos.distanceToSquared(bPos);
    
    if (distSquare>rr)
    {
      const sF32 dist=sqrt(distSquare);
      const sF32 newRadius=(dist+bRadius)*0.5f;
      const sF32 ratio=(newRadius-bRadius)/dist;
      
      bRadius=newRadius;
      rr=bRadius*bRadius;
      bPos.lerp(bPos, pos, ratio);
    }
  }
}

void frMesh::calcBasisVectors()
{
  if (!basis)
  {
    basis=new fr::vector[nVerts*2];
    sInt i;
    
    for (i=0; i<nVerts*2; i++)
      basis[i].set(0.0f, 0.0f, 0.0f);
    
    for (i=0; i<nPrims*3; i+=3)
    {
      const sInt ind0=indices[i+0];
      const sInt ind1=indices[i+1];
      const sInt ind2=indices[i+2];
      const frVertex *vt0=vertices+ind0;
      const frVertex *vt1=vertices+ind1;
      const frVertex *vt2=vertices+ind2;

      fr::vector edge1(0, vt1->u-vt0->u, vt1->v-vt0->v), edge2(0, vt2->u-vt0->u, vt2->v-vt0->v);
      const sF32 cpx=edge1.y*edge2.z-edge1.z*edge2.y;

      if (fabs(cpx)>1e-6f)
      {
        const sF32 ivx=1.0f/cpx;

        for (sInt j=0; j<3; j++)
        {
          edge1.x=vt1->pos.v[j]-vt0->pos.v[j];
          edge2.x=vt2->pos.v[j]-vt0->pos.v[j];

          const sF32 yd=(edge1.x*edge2.z-edge1.z*edge2.x)*ivx;
          basis[ind0*2+0].v[j]+=yd;
          basis[ind1*2+0].v[j]+=yd;
          basis[ind2*2+0].v[j]+=yd;

          const sF32 zd=(edge1.y*edge2.x-edge1.x*edge2.y)*ivx;
          basis[ind0*2+1].v[j]+=zd;
          basis[ind1*2+1].v[j]+=zd;
          basis[ind2*2+1].v[j]+=zd;
        }
      }
    }
    
    for (i=0; i<nVerts*2; i++)
      basis[i].norm();
  }
}

void frMesh::invertCulling()
{
  if (type==0)
  {
    // indices
    sU16 *ind=indices;
    for (sInt i=nPrims; i; i--, ind+=3)
    {
      sU16 t=ind[0];
      ind[0]=ind[2];
      ind[2]=t;
    }
  }
}

void frMesh::invertNormals()
{
  if (vFormat==0 && type==0)
  {
    // vertices
    for (sInt i=0; i<nVerts; i++)
      vertices[i].norm.set(-vertices[i].norm.x, -vertices[i].norm.y, -vertices[i].norm.z);
  }
}

// ---- frMaterial

frMaterial::frMaterial()
{
  flags11=flags12=flags13=0;
  flags21=0;
  texture=texture2=0xff;
  dScaleX=dScaleY=0;

  refCount=1;
}

frMaterial::frMaterial(const frMaterial &x)
{
  texture=x.texture;
  flags11=x.flags11;
  flags12=x.flags12;
  flags13=x.flags13;
  texture2=x.texture2;
  flags21=x.flags21;
  dScaleX=x.dScaleX;
  dScaleY=x.dScaleY;
  
  refCount=1;
}

void frMaterial::addRef()
{
  refCount++;
}

void frMaterial::release()
{
  if (--refCount==0)
    delete this;
}

// ---- frMeshSelection

frMeshSelection::frMeshSelection(frMesh *hostMesh)
{
  mesh=hostMesh;
  mesh->addRef();

  vertSelection=new sU8[mesh->nVerts];
  faceSelection=new sU8[mesh->nPrims];

  refCount=0;
}

frMeshSelection::frMeshSelection(const frMeshSelection &x)
{
  mesh=x.mesh;
  mesh->addRef();
  vertSelection=faceSelection=0;
  refCount=0;

  *this=x; // schlechter stil.
}

frMeshSelection::~frMeshSelection()
{
  mesh->release();
  delete[] vertSelection;
  delete[] faceSelection;
}

frMeshSelection &frMeshSelection::operator =(const frMeshSelection &x)
{
  mesh->release();
  delete[] vertSelection;
  delete[] faceSelection;

  mesh=x.mesh;
  mesh->addRef();
  
  vertSelection=new sU8[mesh->nVerts];
  faceSelection=new sU8[mesh->nPrims];

  copy(x);
  
  return *this;
}

void frMeshSelection::copy(const frMeshSelection &x)
{
  memcpy(vertSelection, x.vertSelection, mesh->nVerts);
  memcpy(faceSelection, x.faceSelection, mesh->nPrims);
}

void frMeshSelection::addRef()
{
  refCount++;
}

void frMeshSelection::release()
{
  if (--refCount==0)
    delete this;
}

void frMeshSelection::all()
{
  memset(vertSelection, 1, mesh->nVerts);
  memset(faceSelection, 1, mesh->nPrims);
}

void frMeshSelection::none()
{
  memset(vertSelection, 0, mesh->nVerts);
  memset(faceSelection, 0, mesh->nPrims);
}

void frMeshSelection::invert()
{
  sInt i;
  
  for (i=0; i<mesh->nVerts; i++)
    vertSelection[i]^=1;
  
  for (i=0; i<mesh->nPrims; i++)
    faceSelection[i]^=1;
}

void frMeshSelection::add(const frMeshSelection &x)
{
  sInt i;
  
  for (i=0; i<mesh->nVerts; i++)
    vertSelection[i]|=x.vertSelection[i];
  
  for (i=0; i<mesh->nPrims; i++)
    faceSelection[i]|=x.faceSelection[i];
}

void frMeshSelection::sub(const frMeshSelection &x)
{
  sInt i;
  
  for (i=0; i<mesh->nVerts; i++)
    vertSelection[i]&=~x.vertSelection[i];
  
  for (i=0; i<mesh->nPrims; i++)
    faceSelection[i]&=~x.faceSelection[i];
}

void frMeshSelection::facesFromVerts(sBool greedy)
{
  for (sInt i=0; i<mesh->nPrims; i++)
  {
    const sU16 *bi=mesh->indices+i*3;
    const sU8 s1=vertSelection[bi[0]];
    const sU8 s2=vertSelection[bi[1]];
    const sU8 s3=vertSelection[bi[2]];

    if (greedy)
      faceSelection[i]=s1|s2|s3;
    else
      faceSelection[i]=s1&s2&s3;
  }
}

void frMeshSelection::selectCube(fr::vector pos, fr::vector size, sBool greedy)
{
  size.x=fabs(size.x);
  size.y=fabs(size.y);
  size.z=fabs(size.z);

  for (sInt i=0; i<mesh->nVerts; i++)
    vertSelection[i]=fabs(mesh->vertices[i].pos.x-pos.x)<size.x &&
      fabs(mesh->vertices[i].pos.y-pos.y)<size.y && fabs(mesh->vertices[i].pos.z-pos.z)<size.z;

  facesFromVerts(greedy);
}

void frMeshSelection::selectSphere(fr::vector pos, fr::vector size, sBool greedy)
{
  if (size.x)
    size.x=1.0f/size.x;
  
  if (size.y)
    size.y=1.0f/size.y;
  
  if (size.z)
    size.z=1.0f/size.z;
  
  for (sInt i=0; i<mesh->nVerts; i++)
  {
    fr::vector p=mesh->vertices[i].pos;
    p-=pos;

    p.x*=size.x;
    p.y*=size.y;
    p.z*=size.z;
    
    vertSelection[i]=(p.x*p.x+p.y*p.y+p.z*p.z)<1.0f;
  }
  
  facesFromVerts(greedy);
}

// ---- frObject

frObject::frObject()
{
  mesh=0;
  transform.identity();
  texTransform.identity();
  normTransform.identity();
  textureMode=0; // direct
  material=0;
  selection=0;
  worldspace=0;
  drawMesh=0;
  renderPass=0;
}

frObject::frObject(const frObject &x)
{
  mesh=0;
  material=0;
  selection=0;

  *this=x; // schlechter stil
}

frObject::~frObject()
{
  releaseReferences();
}

frObject &frObject::operator =(const frObject &x)
{
  if (&x==this)
    return *this;

  releaseReferences();

  worldspace=x.worldspace;
  mesh=worldspace?0:x.mesh;
  if (mesh)
    mesh->addRef();
  
  transform=x.transform;
  texTransform=x.texTransform;
  normTransform=x.normTransform;
  textureMode=x.textureMode;

  material=x.material;
  if (material)
    material->addRef();

  selection=x.selection;
  if (selection)
    selection->addRef();

  drawMesh=0;
  renderPass=x.renderPass;

  updateBounds();

  return *this;
}

void frObject::updateBounds()
{
  bRadius=0.0f;

  if (mesh)
  {
    bRadius=mesh->bRadius; // start from mesh radius
    sF32 scc=0.0f;

    const fr::matrix &m=transform;

    for (sInt i=0; i<3; i++) // compute largest scaling value from matrix
    {
      const sF32 scd=m(0,i)*m(0,i)+m(1,i)*m(1,i)+m(2,i)*m(2,i);

      if (scd>scc)
        scc=scd;
    }

    // scale radius
    bRadius*=sqrt(scc);

    // bsphere center=mesh center*transform
    bPos.transform(mesh->bPos, transform);
  }
}

void frObject::releaseReferences()
{
  if (mesh)
  {
    mesh->release();
    mesh=0;
  }

  if (material)
  {
    material->release();
    material=0;
  }

  if (selection)
  {
    selection->release();
    selection=0;
  }
}

frMesh *frObject::createMesh()
{
  setMesh(new frMesh());
  return mesh;
}

void frObject::setMesh(frMesh *msh)
{
  if (msh)
    msh->addRef();

  if (mesh)
    mesh->release();

  setSelection(0);
  mesh=msh;
  drawMesh=0;

  updateBounds();
}

frMeshSelection *frObject::createSelection()
{
  setSelection(mesh ? new frMeshSelection(mesh) : 0);
  return selection;
}

void frObject::setSelection(frMeshSelection *sel)
{
  if (sel)
    sel->addRef();
  
  if (selection)
    selection->release();

  selection=sel;
}

void frObject::setMaterial(frMaterial *mtrl)
{
  if (mtrl)
    mtrl->addRef();

  if (material)
    material->release();

  material=mtrl;
}

void frObject::hardClone(const frObject &x)
{
  if (&x==this)
    return;

  releaseReferences();

  worldspace=x.worldspace;

  if (!worldspace)
  {
    if (x.mesh)
    {
      setMesh(new frMesh(*x.mesh));
      mesh->transform(x.transform, x.texTransform, x.textureMode);
    }
  }
  else
    mesh=0;

  transform.identity();
  texTransform.identity();
  normTransform.identity();
  textureMode=0;

  setMaterial(x.material);

  if (material && (((material->flags12&127)/5)%3)!=0) // not using direct uv? then copy texture matrix etc :)
  {
    texTransform=x.texTransform;
    textureMode=x.textureMode;
  }

  if (mesh && x.selection)
  {
    setSelection(new frMeshSelection(mesh));
    selection->copy(*x.selection);
  }

  drawMesh=0;
  renderPass=x.renderPass;

  updateBounds();
}

// ---- frModel

frModel::frModel()
  : lightCount(0), gamma(1.0f)
{
  reset();
}

frModel::~frModel()
{
  delete[] objects;
}

void frModel::createObjects(sInt count)
{
  delete[] objects;

  objects=new frObject[count];
  nObjects=count;
}

void frModel::reset()
{
  objects=0;
  nObjects=0;
  lightCount=0;
}

void frModel::cloneObject(const frObject &x, sInt ind, sBool cloneHard)
{
  if (!cloneHard)
    objects[ind]=x;
  else
    objects[ind].hardClone(x);
}

sInt frModel::cloneModel(const frModel &x, sInt startInd, sBool cloneHard)
{
  sInt count=x.nObjects;

  for (sInt i=0; i<count; i++)
    cloneObject(x.objects[i], startInd+i, cloneHard);

  cloneModelLights(x);

  return count;
}

void frModel::cloneModelLights(const frModel &x)
{
  for (sInt i=0; i<x.lightCount; i++)
  {
    if (lightCount>=8)
      break;
    
    lights[lightCount++]=x.lights[i];
  }
  
  gamma=x.gamma;
}

void frModel::transformLights(const fr::matrix &mtx)
{
  fr::matrix temp;

  temp=mtx;
  temp(3,0)=temp(3,1)=temp(3,2)=0.0f;

  for (sInt i=0; i<lightCount; i++)
  {
    lights[i].pos.transform(mtx);
    lights[i].dir.transform(temp);
    lights[i].dir.norm();
  }
}

// ---- frGeometryPlugin

frGeometryPlugin::frGeometryPlugin(const sU8 *&strm)
{
  data=new frModel;
  dirty=sTRUE;
  revision=0;
  input=0;
  nInputs=0;
  opType=0;

  cmds=strm;
}

frGeometryPlugin::~frGeometryPlugin()
{
  delete data;
}

void frGeometryPlugin::process()
{
  for (sInt i=0; i<nInputs; i++)
  {
    input[i]->process();

    if (revision<=input[i]->revision)
      dirty=sTRUE;
  }
  
  if (dirty)
  {
    dirty=sFALSE;
    doProcess();

    revision=++globalRevCounter;
  }
}

void frGeometryPlugin::reset()
{
  const sU8 *strm=cmds;

  parseParams(strm);
}
