// This code is in the public domain. See LICENSE for details.

#include "stdafx.h"
#include "geobase.h"
#include "debug.h"
#include "WindowMapper.h"
#include "viewcomm.h"
#include "resource.h"
#include "DirectX.h"
#include "tstream.h"
#include "frOpGraph.h"

static frMaterial theDefaultMtrl;
static frMaterial* defaultMaterial = &theDefaultMtrl;

// ---- frMesh

static frMesh* firstStaticMesh = 0;

frMesh::frMesh()
{
  vertices = 0;
  indices = indices2 = 0;
	primSelect = 0;
  nVerts = nPrims = 0;
  refCount = 0;
  type = 0;
  vFormat = 0;
  vbs = 0;
  ibs = 0;
  lastDrawFrame = drawCount = 0;
  nsm = psm = 0;
  normAlias = 0;
  subMesh = 0;
}

frMesh::frMesh(const frMesh &x)
{
  nVerts = x.nVerts;
  nPrims = x.nPrims;
  vertices = new frVertex[nVerts];
  indices = new sU16[nPrims * (3 - x.type)];
	indices2 = 0;
	primSelect = new sU8[nPrims];
  refCount = 0;
  type = x.type;
  vFormat = x.vFormat;
  vbs = 0;
  ibs = 0;
  lastDrawFrame = drawCount = 0;
  nsm = psm = 0;
  subMesh = 0;

  memcpy(vertices, x.vertices, sizeof(frVertex) * nVerts);
  memcpy(indices, x.indices, sizeof(sU16) * nPrims * (3 - type));
	memcpy(primSelect, x.primSelect, nPrims);

  if (x.normAlias)
  {
    normAlias = new sU16[nVerts];
    memcpy(normAlias, x.normAlias, sizeof(sU16) * nVerts);
  }
  else
    normAlias = 0;

  bRadius = x.bRadius;
  bPos = x.bPos;
}

frMesh::~frMesh()
{
  FRDASSERT(refCount == 0);

  FRSAFEDELETEA(vertices);
  FRSAFEDELETEA(indices);
	FRSAFEDELETEA(indices2);
	FRSAFEDELETEA(primSelect);
  FRSAFEDELETEA(normAlias);

  if (subMesh)
	{
    subMesh->release();
		subMesh = 0;
	}

  killStaticBuffers();
}

void frMesh::resize(sU32 verts, sU32 prims)
{
  killStaticBuffers();

  if (verts != nVerts)
  {
    FRSAFEDELETEA(vertices);
		FRSAFEDELETEA(normAlias);

    nVerts = verts;
    vertices = new frVertex[nVerts];
  }

  if (prims != nPrims)
  {
    FRSAFEDELETEA(indices);
		FRSAFEDELETEA(indices2);
		FRSAFEDELETEA(primSelect);

    nPrims = prims;
    indices = new sU16[nPrims * (3 - type)];
		primSelect = new sU8[nPrims];
  }

  if (subMesh)
  {
    subMesh->release();
    subMesh = 0;
  }
}

void frMesh::addRef()
{
  refCount++;
}

void frMesh::release()
{
  FRDASSERT(refCount != 0);

  if (--refCount == 0)
    delete this;
}

void frMesh::toVertexBuffer(void* dest) const
{
	const frVertex* src = vertices;
	sF32* dst = (sF32*) dest;

	if (vFormat == 0) // pos+norm+uv
	{
		for (sInt i = 0; i < nVerts; i++)
		{
			*dst++ = src->pos.x;
			*dst++ = src->pos.y;
			*dst++ = src->pos.z;
			*dst++ = src->norm.x;
			*dst++ = src->norm.y;
			*dst++ = src->norm.z;
			*dst++ = src->uvw.x;
			*dst++ = src->uvw.y;
			src++;
		}
	}
	else // pos+diffuse+2x uv
	{
		for (sInt i = 0; i < nVerts; i++)
		{
			*dst++ = src->pos.x;
			*dst++ = src->pos.y;
			*dst++ = src->pos.z;
			*dst++ = (sF32&) src->color;
			*dst++ = src->uvw.x;
			*dst++ = src->uvw.y;
			*dst++ = 0.0f; // 2nd uv set defaults to 0 for now.
			*dst++ = 0.0f;
			src++;
		}
	}
}

void frMesh::convertToStaticBuffers() const
{
  if (vbs || ibs)
    return;

  static const sU32 vertFormats[]={ D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1, D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX2 };
  const sU32 nInds = nPrims * (3 - type);

  fvs::vbSlice* vb = new fvs::vbSlice(nVerts, sFALSE, vertFormats[vFormat]);
  fvs::ibSlice* ib = new fvs::ibSlice(nInds, sFALSE);

	toVertexBuffer(vb->lock());
  memcpy(ib->lock(), indices, nInds*sizeof(sU16));

  vb->unlock();
  ib->unlock();

  vbs = vb;
  ibs = ib;

  if (vbs && ibs)
  {
    psm = 0;
    nsm = firstStaticMesh;
    if (nsm)
      nsm->psm = const_cast<frMesh*>(this);

    firstStaticMesh = const_cast<frMesh*>(this);
  }
}

void frMesh::killStaticBuffers() const
{
  if (vbs && ibs)
  {
    if (psm)
      psm->nsm = nsm;
    else
      firstStaticMesh = nsm;

    if (nsm)
      nsm->psm = psm;

		delete (fvs::vbSlice*) vbs;
		delete (fvs::ibSlice*) ibs;
		vbs = 0;
		ibs = 0;
  }

  drawCount = 0;
}

void frMesh::killAllStaticBuffers()
{
  for (frMesh* t = firstStaticMesh, *t2 = 0; t; t = t2)
  {
    t2 = t->nsm;
    t->killStaticBuffers();
  }
}

void frMesh::killOldStaticBuffers(sU32 current, sU32 delta)
{
  for (frMesh* t = firstStaticMesh, *t2 = 0; t; t = t2)
  {
    t2 = t->nsm;

    if (current - t->lastDrawFrame >= delta)
      t->killStaticBuffers();
  }
}

void frMesh::transform(const fr::matrix &mtx, const fr::matrix &texmtx, sInt texMode)
{
  fr::matrix onorm;

  killStaticBuffers();
	onorm.invTrans3x3(mtx);

  for (sInt i = 0; i < nVerts; i++)
  {
    vertices[i].pos *= mtx;
		vertices[i].norm *= onorm;
		vertices[i].norm.norm();

		if (texMode != 0)
			vertices[i].uvw *= texmtx;
  }

  calcBoundingSphere();
}

void frMesh::calcBoundingSphere() // nach jack ritter
{
  if (!nVerts || type == 3)
    return;

  fr::vector  abMin=vertices[0].pos, abMax=vertices[0].pos;
  sInt        inds[6], i;
  
  bRadius=0.0f;
  bPos=vertices[0].pos;

  for (i=0; i<6; i++)
    inds[i]=0;

  for (i=1; i<nVerts; i++)
  {
    const fr::vector &pos=vertices[i].pos;

    for (sInt j=0; j<3; j++)
    {
      if (pos.v[j]<abMin.v[j])
      {
        abMin.v[j]=pos.v[j];
        inds[j*2+0]=i;
      }
      else if (pos.v[j]>abMax.v[j])
      {
        abMax.v[j]=pos.v[j];
        inds[j*2+1]=i;
      }
    }
  }

  for (sInt j=0; j<3; j++)
  {
    const fr::vector &lo=vertices[inds[j*2+0]].pos;
    const fr::vector &hi=vertices[inds[j*2+1]].pos;
    const sF32 radius=(lo-hi).len()*0.5f;
    
    if (radius>bRadius)
    {
      bPos.add(lo, hi);
      bPos.scale(0.5f);
      bRadius=radius;
    }
  }

  sF32 rr=bRadius*bRadius;

  for (i=0; i<nVerts; i++)
  {
    const fr::vector &pos=vertices[i].pos;
    const sF32 distSquare=(pos.x-bPos.x)*(pos.x-bPos.x)+(pos.y-bPos.y)*(pos.y-bPos.y)+(pos.z-bPos.z)*(pos.z-bPos.z);

    if (distSquare>rr)
    {
      const sF32 dist=sqrt(distSquare);
      const sF32 newRadius=(dist+bRadius)*0.5f;
      const sF32 ratio=(newRadius-bRadius)/dist;

      bRadius=newRadius;
      rr=bRadius*bRadius;
      bPos.scale(1.0f-ratio);
      bPos.addscale(pos, ratio);
    }
  }

  bRadius+=bRadius*1e-3f;

#ifdef _DEBUG
  rr=bRadius*bRadius;

  for (i=0; i<nVerts; i++)
  {
    const sF32 dist=(vertices[i].pos-bPos).lenSq();
    FRDASSERT(dist <= rr);
  }
#endif
}

void frMesh::calcTangents()
{
  sInt i;
  
  for (i = 0; i < nVerts; i++)
		vertices[i].tangent.set(0.0f, 0.0f, 0.0f);
  
  for (i = 0; i < nPrims*3; i += 3)
  {
    const sInt ind0 = indices[i + 0];
    const sInt ind1 = indices[i + 1];
    const sInt ind2 = indices[i + 2];
    frVertex* vt0 = vertices + ind0;
    frVertex* vt1 = vertices + ind1;
    frVertex* vt2 = vertices + ind2;
    
    for (sInt j=0; j<3; j++)
    {
      fr::vector edge1, edge2, cp;
      
      edge1.set(vt1->pos.v[j] - vt0->pos.v[j], vt1->uvw.x - vt0->uvw.x, vt1->uvw.y - vt0->uvw.y);
      edge2.set(vt2->pos.v[j] - vt0->pos.v[j], vt2->uvw.x - vt0->uvw.x, vt2->uvw.y - vt0->uvw.y);
      cp.cross(edge1, edge2);
      
      if (fabs(cp.x) > 1e-6f)
      {
        const sF32 ivx = -1.0f / cp.x;
        const sF32 yd = cp.y * ivx;

				vt0->tangent.v[j] += yd;
				vt1->tangent.v[j] += yd;
				vt2->tangent.v[j] += yd;
      }
    }
  }
  
  for (i = 0; i < nVerts; i++)
		vertices[i].tangent.norm();
}

void frMesh::dumbNormals(sBool eliminateSeams)
{
	sInt i;

	for (i = 0; i < nVerts; i++)
		vertices[i].norm.set(0.0f, 0.0f, 0.0f);

	if (!normAlias)
		eliminateSeams = sFALSE;

	sU16* inds = indices;

	for (i = nPrims; i; i--, inds += 3)
	{
		sU16 i0 = inds[0], i1 = inds[1], i2 = inds[2];

		if (eliminateSeams)
		{
			i0 = normAlias[i0];
			i1 = normAlias[i1];
			i2 = normAlias[i2];
		}

		frVertex* vt0 = vertices + i0;
		frVertex* vt1 = vertices + i1;
		frVertex* vt2 = vertices + i2;

		fr::vector d1, d2, n;
		d1.sub(vt1->pos, vt0->pos);
		d2.sub(vt2->pos, vt0->pos);
		n.cross(d1, d2);
		n.norm();

		vt0->norm.add(n);
		vt1->norm.add(n);
		vt2->norm.add(n);
	}

	for (i = 0; i < nVerts; i++)
	{
		vertices[i].norm.norm();

		if (eliminateSeams)
			vertices[i].norm = vertices[normAlias[i]].norm;
	}
}

void frMesh::invertCulling()
{
	if (type != 0) // no trimesh? no invert cull then
		return;

	sU16* ind = indices;

	for (sInt i = 0; i < nPrims; i++)
	{
		sU16 t = ind[0];
		ind[0] = ind[2];
		ind[2] = t;
		ind += 3;
	}
}

// ---- frMaterial

frMaterial::frMaterial()
{
	// clear texture stages
	for (sInt i = 0; i < 2; i++)
	{
		textureRef[i] = 0;
		texPointer[i] = 0;
		uAddressMode[i] = 0;	// tile
		vAddressMode[i] = 0;	// tile
		filterMode[i] = 3;		// bilinear+mipmap
		coordMode[i] = 0;			// no coord generation
	}

	// clear global values
	alphaMode = 0;					// alpha disable
	zMode = 3;							// read/write
	renderPass = -1;				// default render pass
	cullMode = 0;						// culling enabled
	sortMode = 0;						// sorting disabled
	dynLightMode = 0;				// dynamic lighting enabled

	opSt2 = 0;							// stage2 disabled
	alphaSt2 = 0;
	dScaleX = 1.0f;					// detail scale 1
	dScaleY = 1.0f;

	// clear internal values
	refCount = 1;						// reference count
	mapFrameCounter = 0;		// last frame in which mapping was updated
}

frMaterial::frMaterial(const frMaterial& x)
{
	copyMaterial(x);

	refCount = 1;
}

frMaterial &frMaterial::operator =(const frMaterial& x)
{
	copyMaterial(x);
  return *this;
}

bool frMaterial::operator == (const frMaterial& x) const
{
	return compare(x) == 0;
}

bool frMaterial::operator < (const frMaterial& x) const
{
	return compare(x) < 0;
}

void frMaterial::addRef() const
{
  refCount++;
}

void frMaterial::release() const
{
  if (--refCount == 0)
    delete this;
}

void frMaterial::copyMaterial(const frMaterial& x)
{
	// copy texture stages
	for (sInt i = 0; i < 2; i++)
	{
		textureRef[i] = x.textureRef[i];
		texPointer[i] = 0;
		uAddressMode[i] = x.uAddressMode[i];
		vAddressMode[i] = x.vAddressMode[i];
		filterMode[i] = x.filterMode[i];
		coordMode[i] = x.coordMode[i];
	}

	// copy global values
	alphaMode = x.alphaMode;
	zMode = x.zMode;
	renderPass = x.renderPass;
	cullMode = x.cullMode;
	sortMode = x.sortMode;
	dynLightMode = x.dynLightMode;

	opSt2 = x.opSt2;
	alphaSt2 = x.alphaSt2;
	dScaleX = x.dScaleX;
	dScaleY = x.dScaleY;

	pMatrix = x.pMatrix;
	mapFrameCounter = 0;
}

sInt frMaterial::compare(const frMaterial& x) const
{
	sInt dif;
	sF32 difF;

	// compare per-stage values
	for (sInt i = 0; i < 2; i++)
	{
		if ((dif = textureRef[i] - x.textureRef[i]))				return dif;
		if ((dif = uAddressMode[i] - x.uAddressMode[i]))		return dif;
		if ((dif = vAddressMode[i] - x.vAddressMode[i]))		return dif;
		if ((dif = filterMode[i] - x.filterMode[i]))				return dif;
		if ((dif = coordMode[i] - x.coordMode[i]))					return dif;
	}

	// compare global values
	if ((dif = alphaMode - x.alphaMode))									return dif;
	if ((dif = zMode - x.zMode))													return dif;
	if ((dif = renderPass - x.renderPass))								return dif;
	if ((dif = cullMode - x.cullMode))										return dif;
	if ((dif = sortMode - x.sortMode))										return dif;
	if ((dif = dynLightMode - x.dynLightMode))						return dif;
	if ((dif = opSt2 - x.opSt2))													return dif;
	if ((dif = alphaSt2 - x.alphaSt2))										return dif;
	if ((difF = dScaleX - x.dScaleX))											return (difF < 0) ? -1 : 1;
	if ((difF = dScaleY - x.dScaleY))											return (difF < 0) ? -1 : 1;

	return 0;
}

fr::streamWrapper& operator << (fr::streamWrapper& strm, frMaterial& m)
{
  sU16 ver = 12;
  
  strm << ver;

  // importer code (yeah baby)
  if (ver >= 1 && ver <= 11)
  {
    sChar buf[256];
    sU8 len=0;

    strm << len;

    for (sInt i = 0; i < len; i++)
      strm << buf[i];

    buf[len] = 0;
    g_graph->loadRegisterLink(buf, 0, 0);

    if (ver==1)
    {
			sInt mode = m.uAddressMode[0];
      strm << mode;
      m.uAddressMode[0] = m.vAddressMode[0] = mode;
    }
    else
      strm << m.uAddressMode[0] << m.vAddressMode[0];

    strm << m.filterMode[0];

    if (ver == 3 || ver >= 5)
      strm << m.alphaMode;
    else
      m.alphaMode = 0;

    if (ver >= 4)
      strm << m.renderPass;
    else
      m.renderPass = 0;

    if (ver >= 6)
      strm << m.coordMode[0];
    else
      m.coordMode[0] = 0; // uv

    if (ver >= 7)
    {
      strm << len;

      for (sInt i = 0; i < len; i++)
        strm << buf[i];

      buf[len] = 0;
      g_graph->loadRegisterLink(buf, 12, 0);

      strm << m.opSt2;
      strm << m.uAddressMode[1] << m.vAddressMode[1];
      strm << m.filterMode[1];
      strm << m.alphaSt2 << m.coordMode[1];
      strm << m.dScaleX << m.dScaleY;

      sF32 dDist;
      strm << dDist;
    }
    else
    {
      m.opSt2 = 0;								// disable
			m.alphaSt2 = 0;							// disable
			m.uAddressMode[1] = 0;			// tile
			m.vAddressMode[1] = 0;			// tile
			m.filterMode[1] = 3;				// bilinear+mip
			m.coordMode[1] = 0;					// detail
			m.dScaleX = 4.0f;						// default scale
			m.dScaleY = 4.0f;
    }

    if (ver >= 8 && ver <= 9)
    {
      m.zMode = 3; // read/write

      sInt zWriteMode;
      strm << zWriteMode;

      if (zWriteMode)
        m.zMode ^= 1;
    }
    else if (ver >= 10)
      strm << m.zMode;
    else
      m.zMode = 3;

    if (ver>=10)
      strm << m.cullMode;
    else
      m.cullMode = 0; // normal cull

    if (ver >= 10)
      strm << m.sortMode;
    else
      m.sortMode = 0; // don't sort

    if (m.sortMode > 1)
      m.sortMode = 1;

    if (ver <= 9) // translate old alpha modes
    {
      if (m.alphaMode)
      {
        if (m.alphaMode != 3) // not add front?
          m.sortMode = 1; // then sort
        else
          m.renderPass = 7;

        if (m.alphaMode >= 3)
          m.alphaMode--;

        m.cullMode = 2; // disable cull
      }
    }

    if (ver >= 9)
      strm << m.dynLightMode;
    else
      m.dynLightMode = 0;

    if (ver <= 10)
    {
      if (m.renderPass == 0)
        m.renderPass = -1;

      if (m.cullMode > 0)
        m.cullMode--;
    }
  }

  // the actual loading code (kinda simple in comparision, huh?)
  if (ver == 12)
  {
		strm << m.textureRef[0] << m.uAddressMode[0] << m.vAddressMode[0];
		strm << m.filterMode[0] << m.alphaMode << m.renderPass;
		strm << m.coordMode[0] << m.textureRef[1] << m.opSt2;
		strm << m.uAddressMode[1] << m.vAddressMode[1] << m.filterMode[1];
		strm << m.alphaSt2 << m.coordMode[1] << m.dScaleX << m.dScaleY;
		strm << m.zMode << m.cullMode << m.sortMode << m.dynLightMode;
  }
  
  return strm;
}

// ---- frObject

frObject::frObject()
{
  mesh = 0;
  transform.identity();
  texTransform.identity();
  textureMode = 0; // direct
  material = defaultMaterial;
	defaultMaterial->addRef();
  nullTransform = sTRUE;
	flipCull = sFALSE;
  renderPass = 0;
}

frObject::frObject(const frObject& x)
{
  mesh = 0;
  material = 0;

  setMesh(x.mesh);

  transform = x.transform;
  texTransform = x.texTransform;
  textureMode = x.textureMode;
  
  setMaterial(x.material);

  nullTransform = x.nullTransform;
	flipCull = x.flipCull;
  renderPass = x.renderPass;

  updateBounds();
}

frObject::~frObject()
{
  setMesh(0);
	material->release();
	material = 0;
}

frObject& frObject::operator =(const frObject& x)
{
  if (&x == this)
    return *this;

  setMesh(x.mesh);
  setMaterial(x.material);

  transform = x.transform;
  texTransform = x.texTransform;
  textureMode = x.textureMode;
  nullTransform = x.nullTransform;
	flipCull = x.flipCull;
  renderPass = x.renderPass;

  updateBounds();

  return *this;
}

void frObject::updateBounds()
{
  bRadius = 0.0f;

  if (mesh)
  {
    bRadius = mesh->bRadius; // start from mesh radius
    sF32 scc = 0.0f;

    const fr::matrix& m = transform;

    for (sInt i = 0; i < 3; i++) // compute largest scaling value from matrix
    {
      const sF32 scd = m(0,i) * m(0,i) + m(1,i) * m(1,i) + m(2,i) * m(2,i);

      if (scd > scc)
        scc = scd;
    }

    // scale radius
    bRadius *= sqrt(scc);

    // bsphere center=mesh center*transform
    bPos = mesh->bPos * transform;
  }

	flipCull = transform.det3x3() < 0;
}

void frObject::reset()
{
  setMesh(0);
  transform.identity();
  texTransform.identity();
  textureMode = 0; // direct
  setMaterial(defaultMaterial);
  nullTransform = sTRUE;
	flipCull = sFALSE;
  renderPass = 0;
}

frMesh* frObject::createMesh()
{
  setMesh(new frMesh);
  return mesh;
}

void frObject::setMesh(frMesh* msh)
{
  if (mesh != msh)
  {
    if (msh)
      msh->addRef();

    if (mesh)
      mesh->release();

    mesh = msh;
  }

  updateBounds();
}

void frObject::setMaterial(frMaterial* mtrl)
{
	FRDASSERT(mtrl && material);

	mtrl->addRef();
	material->release();
  material = mtrl;
}

void frObject::hardClone(const frObject& x)
{
  if (&x == this)
    return;

  if (x.mesh)
  {
    setMesh(new frMesh(*x.mesh));
    if (!x.nullTransform)
      mesh->transform(x.transform, x.texTransform, x.textureMode);

		if (x.flipCull)
			mesh->invertCulling();
  }
  else
    setMesh(0);

  transform.identity();
  texTransform.identity();
  textureMode = 0;
  
  setMaterial(x.material);

  if (material->coordMode[0] != 0) // not using direct uv? then copy texture matrix etc.
  {
    texTransform = x.texTransform;
    textureMode = x.textureMode;
  }

  nullTransform = sTRUE;
	flipCull = sFALSE;
  renderPass = x.renderPass;

  updateBounds();
}

// ---- frModel

frModel::frModel()
  : lightCount(0), objects(0), virtObjects(0), nObjects(0), nAllocObjects(0), nVirtObjects(0)
{
}

frModel::~frModel()
{
  if (objects)
    delete[] objects;

  if (virtObjects)
    delete[] virtObjects;
}

void frModel::createObjects(sInt nobj, sInt nvobj)
{
  if (nobj != nAllocObjects)
  {
    if (objects)
      delete[] objects;

    objects = new frObject[nobj];
    nObjects = nAllocObjects = nobj;
  }
  else
  {
    nObjects = nobj;

    for (sInt i = 0; i < nObjects; i++)
      objects[i].reset();
  }

  if (nvobj != nVirtObjects)
  {
    if (virtObjects)
      delete[] virtObjects;

    virtObjects = new frObject[nvobj];
    nVirtObjects = nvobj;
  }
  else
  {
    for (sInt i = 0; i < nVirtObjects; i++)
      virtObjects[i].reset();
  }
}

void frModel::cloneObject(const frObject& x, sInt ind, sBool cloneHard)
{
  FRDASSERT(ind < nObjects);

  if (!cloneHard)
    objects[ind] = x;
  else
    objects[ind].hardClone(x);
}

sInt frModel::cloneModel(const frModel& x, sInt startInd, sBool cloneHard)
{
  const sInt count = x.nObjects;

  for (sInt i = 0; i < count; i++)
    cloneObject(x.objects[i], startInd + i, cloneHard);

  cloneModelLights(x);
  return count;
}

void frModel::cloneModelLights(const frModel& x)
{
  for (sInt i = 0; i < x.lightCount; i++)
  {
    if (lightCount >= 8)
      break;
    
    lights[lightCount++] = x.lights[i];
  }
}

void frModel::transformLights(const fr::matrix& mtx)
{
  fr::matrix temp = mtx;
  
  temp(3,0) = temp(3,1) = temp(3,2) = 0.0f;
  
  for (sInt i = 0; i < lightCount; i++)
  {
    lights[i].pos.transform(mtx);
    lights[i].dir.transform(temp);
    lights[i].dir.norm();
  }
}

// ---- frGeometryPlugin

frGeometryPlugin::frGeometryPlugin(const frPluginDef* d, sInt nLinks)
  : frPlugin(d, nLinks)
{
  if (!def->isNop)
    data = new frModel;
  else
    data = 0;

  dirty = sTRUE;
}

frGeometryPlugin::~frGeometryPlugin()
{
  if (!def->isNop)
    FRSAFEDELETE(data);
}

void frGeometryPlugin::lock()
{
  if (!def->isNop)
    dataLock.enter();
  else
  {
    frGeometryPlugin* in = static_cast<frGeometryPlugin*>(getInput(0));
    if (in)
      in->lock();
  }
}

void frGeometryPlugin::unlock()
{
  if (!def->isNop)
    dataLock.leave();
  else
  {
    frGeometryPlugin* in = static_cast<frGeometryPlugin*>(getInput(0));
    if (in)
      in->unlock();
  }
}

sBool frGeometryPlugin::process(sU32 counter)
{
  sU32  i;
  sU32  numIn;
  
  if (counter==1024) // bei dieser tiefe ist es fast definitive eine schleife
    return sFALSE;
  
  numIn=0;

  fr::cSectionLocker lock(dataLock);

  for (i=0; i < nTotalInputs; i++)
  {
    frLink* lnk = input + i;
    frPlugin* inPlg = lnk->plg;

    if (!inPlg && lnk->opID)
    {
      frOpGraph::opMapIt it = g_graph->m_ops.find(lnk->opID);

      if (it != g_graph->m_ops.end())
      {
        lnk->plg = it->second.plg;
        inPlg = it->second.plg;
      }
    }

    if (inPlg)
    {
      if (!inPlg->process(counter+1))
      {
        if (data && !def->isNop)
          data->nObjects = 0;

        return sFALSE;
      }
      
      if (revision<=((frGeometryPlugin *) inPlg)->revision)
        dirty = sTRUE;

      if (i < def->minInputs)
        numIn++;
    }
  }
  
  if (numIn < def->minInputs)
  {
    if (data && !def->isNop)
      data->nObjects = 0;
		else if (def->isNop)
			data = 0;

    return sFALSE;
  }

	if (revision == 0)
		dirty = sTRUE; // newly created ops are always dirty

  // nop operators use a slightly different processing
  if (def->isNop)
  {
    if (dirty)
    {
      dirty = sFALSE;

      if (!doProcess())
      {
        dirty = sTRUE;
        return sFALSE;
      }

      updateRevision();
    }

    return sTRUE;
  }

  if (!getData())
    return sFALSE;

  if (dirty)
  {
    static sChar msg[256];
    sprintf(msg, "processing %s", def->name);
    ::PostMessage(g_winMap.Get(IDR_MAINFRAME), WM_SET_ACTION_MESSAGE, 0, (LPARAM) msg);
    
    dirty = sFALSE;
    
    if (!doProcess())
    {
      data->nObjects = 0;
      dirty = sTRUE;
    }
    
    updateRevision();
  }
  
  return sTRUE;
}
