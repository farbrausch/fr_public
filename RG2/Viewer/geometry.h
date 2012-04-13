// This code is in the public domain. See LICENSE for details.

#ifndef __geometry_h_
#define __geometry_h_

#include "types.h"
#include "math3d_2.h"

namespace fvs
{
  class vbSlice;
  class ibSlice;
  class texture;
}

struct frVertex // tris
{
  fr::vector  pos;
  fr::vector  norm;
	fr::vector	tangent;
	fr::vector	uvw;
	sU32				color;
	sU32				select;
  sInt        normAlias;
};

class frMesh
{
public:
  frVertex*     vertices;
  sU16*         indices;
	sU8*					primSelect;
  sInt          refCount;
  sInt          type;      // 0=trimesh 1=linelist
  sInt          vFormat;   // 0=pos+norm+tex 1=pos+color+2tex

  sF32          bRadius;   // bounding sphere radius
  fr::vector    bPos;      // bounding sphere position

  fvs::vbSlice* vbuf;      // vertex/index buffers
  fvs::ibSlice* ibuf;

  frMesh*       subMesh;  // for particles

  sU32          nVerts, nPrims;

  frMesh();
  frMesh(const frMesh& x);
  ~frMesh();

  void    freeData();
  void    copyData(const frMesh& src);
  void    resize(sU32 verts, sU32 prims);

  void    addRef()          { refCount++; }
  void    release()         { if (--refCount==0) delete this; }

	void		toVertexBuffer(void* dest) const;
  void    transform(const fr::matrix& mtx, const fr::matrix& nmtx, const fr::matrix& texmtx, sInt texMode);
  void    calcBoundingSphere();
  void    calcTangents();

  void    invertCulling();
  void    invertNormals();
};

class frMaterial
{
public:
  fvs::texture* tex[2];
  sU8           flags11, flags12, flags13;
  sU8           flags21;

  sF32          dScale[2];
	sInt          refCount;

  fr::matrix    pMatrix; // projection matrix for lightmaps (INTERNAL ONLY)

  frMaterial();
  frMaterial(const frMaterial& x);

  void    addRef();
  void    release();
};

class frObject
{
protected:
  frMesh*           mesh;
  fr::matrix        transform, normTransform, texTransform;
  frMaterial*       material;

	sBool							flipCull;

public:
  frObject();
  frObject(const frObject& x);
  ~frObject();

  frObject&         operator = (const frObject& x);

  void              setMesh(frMesh* msh);
  frMesh*           getMesh() const                           { return mesh; }

  void              setMaterial(frMaterial* mtrl);
  frMaterial*       getMaterial() const                       { return material; }

  void							multiplyTransform(const fr::matrix& x,const fr::matrix& n);
  void							setTransform(const fr::matrix& x,const fr::matrix &n);
  
  fr::matrix&       getTransform()                            { return transform; }
  const fr::matrix& getTransform() const                      { return transform; }

  const fr::matrix& getNormTransform() const                  { return normTransform; }
  
  void              multiplyTexTransform(const fr::matrix& x) { texTransform *= x; textureMode = 1; }
  fr::matrix&       getTexTransform()                         { return texTransform; }
  const fr::matrix& getTexTransform() const                   { return texTransform; }
  void              setTexTransform(const fr::matrix& x)      { texTransform = x; textureMode = 1; }

	sBool							getFlipCull() const												{ return flipCull; }

  void              hardClone(const frObject& x);

  void              updateBounds();
  
  frObject*         next; // dirty hacks for drawing
  const frMesh*     drawMesh;

  sF32              bRadius;  // bounding sphere radius
  fr::vector        bPos;     // bounding sphere position

  sInt              textureMode; // 0=direct 1=transform
  sInt              renderPass;
};

class frLight
{
public:
  sU32        type; // 0=dir 1=point 2=spot
  sF32        r, g, b;
  fr::vector  pos, dir;
  sF32        range, falloff;
  sF32        att[3];
  sF32        innerc, outerc;
};

class frModel
{
public:
  frObject* objects;
  sInt      nObjects;
  sInt      nAllocObjects;

  frLight   lights[8];
  sU32      lightCount;

  frModel();
  ~frModel();

  void      createObjects(sInt count);

	frObject*	cloneObject(const frObject& x, sInt ind, sBool cloneHard = sFALSE);
  sInt      cloneModel(const frModel& x, sInt startInd, sBool cloneHard = sFALSE);
  void      cloneModelLights(const frModel& x);
  void      transformLights(const fr::matrix& mtx);
};

#endif
