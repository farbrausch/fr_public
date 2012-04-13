// This code is in the public domain. See LICENSE for details.

#include "types.h"
#include "rtlib.h"
#include "geometry.h"
#include "math3d_2.h"
#include "engine.h"
#include "directx.h"
#include "debug.h"

#pragma warning (disable: 4018)

// ---- frMesh

frMesh::frMesh()
{
  vertices = 0;
  indices = 0;
	primSelect = 0;
  nVerts = nPrims = 0;
  refCount = 0;
  type = 0;
  vFormat = 0;
  vbuf = 0;
  ibuf = 0;
  bRadius = 0.0f;
  subMesh = 0;
}

frMesh::frMesh(const frMesh &x)
{
  nVerts = x.nVerts;
  nPrims = x.nPrims;
  vertices = new frVertex[nVerts];
  indices = new sU16[nPrims * (3 - x.type)];
	primSelect = new sU8[nPrims];
  refCount = 0;
  type = x.type;
  vFormat = x.vFormat;
  vbuf = 0;
  ibuf = 0;
  bPos = x.bPos;
  bRadius = x.bRadius;
  subMesh = 0;

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
	delete[] primSelect;

  if (subMesh)
  {
		delete subMesh;
    subMesh = 0;
  }

	if (vbuf)
	{
		delete (fvs::vbSlice*) vbuf;
		vbuf = 0;
	}

	if (ibuf)
	{
		delete (fvs::ibSlice*) ibuf;
		ibuf = 0;
	}
}

void frMesh::copyData(const frMesh& src)
{
  memcpy(vertices, src.vertices, sizeof(frVertex) * src.nVerts); // kinda strange semantics enforced by the way copyData is used :)
  memcpy(indices, src.indices, sizeof(sU16) * src.nPrims * (3 - src.type));
	memcpy(primSelect, src.primSelect, sizeof(sU8) * src.nPrims);
}

void frMesh::resize(sU32 verts, sU32 prims)
{
  if (verts != nVerts || prims != nPrims)
  {
    freeData();

    nVerts = verts;
    nPrims = prims;

    vertices = new frVertex[nVerts];
    indices = new sU16[nPrims * (3 - type)];
		primSelect = new sU8[nPrims];
  }
}

void frMesh::toVertexBuffer(void* dest) const
{
	sF32* dst = (sF32*) dest;
	frVertex* src = vertices;
	sInt nv = nVerts;

	if (vFormat == 0)
	{
		while (nv--)
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
	else
	{
		while (nv--)
		{
			*dst++ = src->pos.x;
			*dst++ = src->pos.y;
			*dst++ = src->pos.z;
			*dst++ = (sF32&) src->color;
			*dst++ = src->uvw.x;
			*dst++ = src->uvw.y;
			*dst++ = 0.0f;
			*dst++ = 0.0f;
			src++;
		}
	}
}

void frMesh::transform(const fr::matrix &mtx, const fr::matrix &nmtx, const fr::matrix &texmtx, sInt texMode)
{
  for (sInt i = 0; i < nVerts; i++)
  {
    vertices[i].pos *= mtx;
    vertices[i].norm *= nmtx;
    vertices[i].norm.norm();

    if (texMode != 0)
			vertices[i].uvw.transform(texmtx);
  }

  calcBoundingSphere();
}

void frMesh::calcBoundingSphere() // nach jack ritter
{
  fr::vector  abMin, abMax;
  sInt        inds[6], i;
  
  bRadius = 0.0f;
  if (type == 3 || !nVerts)
    return;

  for (i = 0; i < nVerts; i++)
  {
    for (sInt j = 0; j < 3; j++)
    {
			const sF32 val = vertices[i].pos.v[j];

      if (!i || val < abMin.v[j])
      {
        abMin.v[j] = val;
        inds[j + 0] = i;
      }
      
      if (!i || val > abMax.v[j])
      {
        abMax.v[j] = val;
        inds[j + 3] = i;
      }
    }
  }

  for (i = 0; i < 3; i++)
  {
    const fr::vector& lo = vertices[inds[i + 0]].pos;
    const fr::vector& hi = vertices[inds[i + 3]].pos;
    const sF32 radius = lo.distanceTo(hi) * 0.5f;
    
    if (radius > bRadius)
    {
      bPos.lerp(lo, hi, 0.5f);
      bRadius = radius;
    }
  }

  sF32 rr = bRadius * bRadius;

  for (i = 0; i < nVerts; i++)
  {
    const fr::vector& pos = vertices[i].pos;
    const sF32 distSquare = pos.distanceToSquared(bPos);
    
    if (distSquare > rr)
    {
      const sF32 dist = sqrt(distSquare);
      const sF32 newRadius = (dist + bRadius) * 0.5f;
      const sF32 ratio = (newRadius - bRadius) / dist;
      
      bRadius = newRadius;
      rr = bRadius * bRadius;
      bPos.lerp(bPos, pos, ratio);
    }
  }
}

void frMesh::calcTangents()
{
	for (sInt i = 0; i < nVerts; i++)
		vertices[i].tangent.set(0.0f, 0.0f, 0.0f);
  
	const sU16* iptr = indices;

  for (sInt i = 0; i < nPrims; i++, iptr += 3)
  {
    frVertex* vt0 = vertices + iptr[0];
    frVertex* vt1 = vertices + iptr[1];
    frVertex* vt2 = vertices + iptr[2];

		const sF32 e1z = vt1->uvw.y - vt0->uvw.y, e2z = vt2->uvw.y - vt0->uvw.y;
		const sF32 cpx = (vt1->uvw.x - vt0->uvw.x) * e2z - (vt2->uvw.x - vt0->uvw.x) * e1z;

    if (fabs(cpx) > 1e-6f)
    {
      const sF32 ivx = 1.0f / cpx;

      for (sInt j = 0; j < 3; j++)
      {
				const sF32 yd = ((vt1->pos.v[j] - vt0->pos.v[j]) * e2z - (vt2->pos.v[j] - vt0->pos.v[j]) * e1z) * ivx;

				vt0->tangent.v[j] += yd;
				vt1->tangent.v[j] += yd;
				vt2->tangent.v[j] += yd;
      }
    }
  }
  
  for (sInt i = 0; i < nVerts; i++)
		vertices[i].tangent.norm();
}

void frMesh::invertCulling()
{
	if (type != 0)
		return;

  // indices
  sU16* ind = indices;
  for (sInt i = nPrims; i; i--, ind += 3)
  {
    sU16 t = ind[0];
    ind[0] = ind[2];
    ind[2] = t;
  }
}

void frMesh::invertNormals()
{
  if (vFormat == 0 && type == 0)
  {
		frVertex* vptr = vertices;

    for (sInt i = nVerts; i; i--, vptr++)
		{
			vptr->norm.x = -vptr->norm.x;
			vptr->norm.y = -vptr->norm.y;
			vptr->norm.z = -vptr->norm.z;
		}
  }
}

// ---- frMaterial

struct defaultMaterialStruct // must match with frMaterial's definition!
{
	sInt		tex1, tex2;
	sU8			flags11, flags12, flags13, flags21;
	sF32		dScale[2];
	sInt		refCount;
} defaultMaterial = {
	0, 0,		// no textures
	0x00,		// standard stage1 setup
	0x03,		// z readwrite no alpha uv coord no cull default render pass
	0x00,		// default render pass
	0x00,		// <stage2 disabled>
	0.0f,		// dscalex=0
	0.0f,		// dscaley=0
	1 			// one reference
};

frMaterial::frMaterial()
{
  flags11 = flags12 = flags13 = 0;
  flags21 = 0;
  tex[0] = tex[1] = 0;
	dScale[0] = dScale[1] = 0;

  refCount = 1;
}

frMaterial::frMaterial(const frMaterial &x)
{
  tex[0] = x.tex[0];
  tex[1] = x.tex[1];
  flags11 = x.flags11;
  flags12 = x.flags12;
  flags13 = x.flags13;
  flags21 = x.flags21;
	dScale[0] = x.dScale[0];
	dScale[1] = x.dScale[1];
  
  refCount = 1;
}

void frMaterial::addRef()
{
  refCount++;
}

void frMaterial::release()
{
  if (--refCount == 0)
    delete this;
}

// ---- frObject

frObject::frObject()
{
  mesh = new frMesh;
	mesh->addRef();
  transform.identity();
  normTransform.identity();
  texTransform.identity();
  textureMode = 0; // direct
  material = (frMaterial*) &defaultMaterial;
	material->addRef();
  drawMesh = 0;
	flipCull = sFALSE;
  renderPass = 0;
}

frObject::frObject(const frObject& x)
{
  mesh = new frMesh;
	mesh->addRef();
  material = (frMaterial*) &defaultMaterial;
	material->addRef();

  *this = x; // schlechter stil
}

frObject::~frObject()
{
	mesh->release();
	material->release();
}

frObject& frObject::operator =(const frObject& x)
{
  if (&x == this)
    return *this;

  setMesh(x.mesh);
  setMaterial(x.material);
  
  transform = x.transform;
  normTransform = x.normTransform;
  texTransform = x.texTransform;
  textureMode = x.textureMode;

  drawMesh = 0;
	flipCull = x.flipCull;
  renderPass = x.renderPass;

  updateBounds();

  return *this;
}

void frObject::updateBounds()
{
  bRadius = mesh->bRadius; // start from mesh radius
  sF32 scc = 0.0f;

  const fr::matrix &m = transform;

  for (sInt i = 0; i < 3; i++) // compute largest scaling value from matrix
  {
    const sF32 scd = m(0,i) * m(0,i) + m(1,i) * m(1,i) + m(2,i) * m(2,i);

    if (scd > scc)
      scc = scd;
  }

  // scale radius
  bRadius *= sqrt(scc);

  // bsphere center=mesh center*transform
  bPos.transform(mesh->bPos, transform);

  fr::vector temp;
  temp.cross((fr::vector&) transform(0,0),(fr::vector&) transform(1,0));
  flipCull = temp.dot((fr::vector&) transform(2,0)) < 0;
	//flipCull = transform.det3x3() < 0;
}

void frObject::setMesh(frMesh* msh)
{
	msh->addRef();
	mesh->release();

  mesh = msh;
  drawMesh = 0;

  updateBounds();
}

void frObject::setMaterial(frMaterial* mtrl)
{
	mtrl->addRef();
	material->release();

  material = mtrl;
}

void frObject::multiplyTransform(const fr::matrix& x, const fr::matrix& n)
{
	transform *= x;
  normTransform *= n;
	updateBounds();
}

void frObject::setTransform(const fr::matrix& x, const fr::matrix &n)
{
	transform = x;
  normTransform = n;
	updateBounds();
}

void frObject::hardClone(const frObject& x)
{
  if (&x == this)
    return;

  setMesh(new frMesh(*x.mesh));
  mesh->transform(x.transform, x.normTransform, x.texTransform, x.textureMode);
	if (x.flipCull)
		mesh->invertCulling();

  transform.identity();
  normTransform.identity();
  texTransform.identity();
  textureMode = 0;

  setMaterial(x.material);

	if (material->flags11 >> 6) // not using direct uv? then copy texture matrix etc.
  {
    texTransform = x.texTransform;
    textureMode = x.textureMode;
  }

  drawMesh = 0;
	flipCull = sFALSE;
  renderPass = x.renderPass;

  updateBounds();
}

// ---- frModel

frModel::frModel()
  : lightCount(0), objects(0), nObjects(0), nAllocObjects(0)
{
}

frModel::~frModel()
{
  delete[] objects;
}

void frModel::createObjects(sInt count)
{
  if (nAllocObjects != count)
  {
    delete[] objects;

    objects = new frObject[count];
    nObjects = nAllocObjects = count;
  }
}

frObject* frModel::cloneObject(const frObject& x, sInt ind, sBool cloneHard)
{
	frObject* obj = objects + ind;

  if (!cloneHard)
		*obj = x;
  else
		obj->hardClone(x);

	return obj;
}

sInt frModel::cloneModel(const frModel& x, sInt startInd, sBool cloneHard)
{
  sInt count = x.nObjects;

  for (sInt i = 0; i < count; i++)
    cloneObject(x.objects[i], startInd+i, cloneHard);

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
