// This code is in the public domain. See LICENSE for details.

#ifndef __geobase_h_
#define __geobase_h_

#include "types.h"
#include "math3d_2.h"
#include "plugbase.h"
#include "sync.h"

// ---- forward decls

class frGeometryPlugin;

namespace fr
{
  class streamWrapper;
}

// ---- structural elements

struct frVertex // rule-version (56 bytes)
{
	fr::vector		pos;
	fr::vector		norm;
	fr::vector		uvw;
	fr::vector		tangent;
	sU32					color;
	sU32					select;	// selection flags
};

class frMesh
{
public:
  frVertex*     vertices;

  sU16*         indices;
	sU16*					indices2;
	sU16*					useIndices;
  sInt          refCount;
  
	sInt          type;           // 0=trimesh 1=linelist 3=particles (obsolete now)
  sInt          vFormat;        // 0=pos+norm+tex 1=pos+color+2tex

  sU16*         normAlias;
	sU8*					primSelect;

  sF32          bRadius;        // bounding sphere radius
  fr::vector    bPos;           // bounding sphere position

  mutable void* vbs;            // corresponding vb slice (only if it's static!)
  mutable void* ibs;            // corresponding ib slice (only if it's static!)
  mutable sU32  lastDrawFrame;  // used to decide whether to make a mesh static or not
  mutable sU32  drawCount;

  mutable frMesh* nsm;          // next static mesh
  mutable frMesh* psm;

  mutable frMesh* subMesh;      // submesh (used for particles)

  sU32        nVerts, nPrims;

  frMesh();
  frMesh(const frMesh& x);
  ~frMesh();

  void    resize(sU32 verts, sU32 prims);

  void    addRef();
  void    release();

	void		toVertexBuffer(void* dest) const;
  void    convertToStaticBuffers() const;
  void    killStaticBuffers() const;

  static void killAllStaticBuffers();
  static void killOldStaticBuffers(sU32 current, sU32 delta);

  void    transform(const fr::matrix& mtx, const fr::matrix& texmtx, sInt texMode);
  void    calcBoundingSphere();
	void		calcTangents();
	void		dumbNormals(sBool eliminateSeams);
	void		invertCulling();

	void		makeMesh();
};

class frMaterial
{
public:
	// ---- per-stage variables
	sU32						textureRef[2];								// texture ref ID
	mutable void*		texPointer[2];								// will hold texture ptr
	sInt						uAddressMode[2];							// texture u addressing mode
	sInt						vAddressMode[2];							// texture v addressing mode
	sInt						filterMode[2];								// filter/mipmap on/off
	sInt						coordMode[2];									// coordinate generation

	// ---- global per-material values
	sInt						alphaMode;										// opaque/blend/add/mul
	sInt						zMode;												// read/write enable/disable
	sInt						renderPass;										// 0-7 or -1 for default
	sInt						cullMode;											// bf culling enable/disable
	sInt						sortMode;											// facesorting enable/disable
	sInt						dynLightMode;									// dyn. light enable/disable

	sInt						opSt2;												// stage2 operation mode
	sInt						alphaSt2;											// stage2 alpha mode
	sF32						dScaleX, dScaleY;							// detail tex. scale

	// ---- internal stuff
	mutable sInt		refCount;											// reference count
	sU32						lColor;												// line color
	fr::matrix			pMatrix;											// lightmap proj. matrix
	mutable sU32		mapFrameCounter;							// mapping frame counter

	frMaterial();
  frMaterial(const frMaterial& x);

  frMaterial& operator = (const frMaterial& x);
  
  bool        operator == (const frMaterial& x) const;
  bool        operator < (const frMaterial& x) const;

  void				addRef() const;
  void				release() const;

	void				copyMaterial(const frMaterial& x);
	sInt				compare(const frMaterial& x) const;
};

class frObject
{
protected:
  frMesh*           mesh;
  fr::matrix        transform, texTransform;
  sInt              textureMode; // 0=direct 1=transform 2=transform&project
  sInt              renderPass; // 0..7
  frMaterial*       material;
  sBool             nullTransform;

	sBool							flipCull;

public:
  frObject();
  frObject(const frObject& x);
  ~frObject();

  frObject&         operator = (const frObject& x);

  frMesh*           createMesh();
  void              setMesh(frMesh* msh);
  frMesh*           getMesh() const                           { return mesh; }

  void              setMaterial(frMaterial* mtrl);
  frMaterial*       getMaterial() const                       { return material; }

  void              loadIdentity()                            { transform.identity(); updateBounds(); nullTransform = sFALSE; }
  void              multiplyTransform(const fr::matrix& x)    { transform *= x; updateBounds(); nullTransform = sFALSE; }
  fr::matrix&       getTransform()                            { return transform; }
  const fr::matrix& getTransform() const                      { return transform; }
  void              setTransform(const fr::matrix& x)         { transform = x; updateBounds(); nullTransform = sFALSE; }

  sInt              getRenderPass() const                     { return renderPass; }
  void              setRenderPass(sInt pass)                  { renderPass = pass; }
  
  void              multiplyTexTransform(const fr::matrix& x) { texTransform *= x; setTextureMode(1); nullTransform = sFALSE; }
  fr::matrix&       getTexTransform()                         { return texTransform; }
  const fr::matrix& getTexTransform() const                   { return texTransform; }
  void              setTexTransform(const fr::matrix& x)      { texTransform = x; nullTransform = sFALSE; }

  void              setTextureMode(sInt mode)                 { textureMode = mode; }
  sInt              getTextureMode() const                    { return textureMode; }

	sBool							getFlipCull() const												{ return flipCull; }

  void              hardClone(const frObject& x);

  void              updateBounds();
  void              reset();

  inline sBool      hasNullTransform() const                  { return nullTransform; }
  
  frObject*         next;     // dirty hack for drawing
  frMesh*           drawMesh; // this one too.

  sF32              bRadius;  // bounding sphere radius
  fr::vector        bPos;     // bounding sphere position
};

class frLight
{
public:
  sU32        type; // 0=dir 1=point 2=spot
  fr::vector  pos, dir;
  sF32        r, g, b;
  sF32        range, att[3];
  sF32        innerc, outerc, falloff;
};

class frModel
{
public:
  frObject* objects;
  frObject* virtObjects;

  sInt      nObjects, nAllocObjects;
  sInt      nVirtObjects;

  frLight   lights[8];
  sU32      lightCount;

  frModel();
  ~frModel();

  void      createObjects(sInt nObjects, sInt nVirtObjects = 0);

  void      cloneObject(const frObject& x, sInt ind, sBool cloneHard = sFALSE);
  sInt      cloneModel(const frModel& x, sInt startInd, sBool cloneHard = sFALSE);
  void      cloneModelLights(const frModel& x);
  void      transformLights(const fr::matrix& xform);
};

extern fr::streamWrapper &operator << (fr::streamWrapper &strm, frMaterial &m);

// ---- plugin interface

class frGeometryPlugin: public frPlugin
{
protected:
  frModel*      data;
  fr::cSection  dataLock;
  
public:
  frGeometryPlugin(const frPluginDef* d, sInt nLinks = 0);
  virtual ~frGeometryPlugin();
  
  void              lock();
  void              unlock();
  
  virtual sBool     process(sU32 counter=0);
  inline frModel*   getData()                 { return data; }
  inline frModel*   getInputData(sU32 i)      { frPlugin* inPlg = getInput(i); return inPlg ? static_cast<frGeometryPlugin*>(inPlg)->getData() : 0; }
};

#endif
