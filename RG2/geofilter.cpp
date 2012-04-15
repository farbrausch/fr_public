// This code is in the public domain. See LICENSE for details.

#include "stdafx.h"
#include "texbase.h"
#include "geobase.h"
#include "debug.h"
#include "exporter.h"
#include "tstream.h"
#include "exportTool.h"
#include "frOpGraph.h"
#include <math.h>

static const sF32 PI = 3.1415926535f;

static void matrixSRT(fr::matrix& dest, const frParam* srtparams)
{
  FRDASSERT(srtparams[0].type == frtpThreeFloat);
  FRDASSERT(srtparams[1].type == frtpThreeFloat);
  FRDASSERT(srtparams[2].type == frtpThreeFloat);
  
  fr::matrix t1, t2;
  t1.scale(srtparams[0].trfloatv.a, srtparams[0].trfloatv.b, srtparams[0].trfloatv.c);
  t2.rotateHPB(srtparams[1].trfloatv.a * 2 * PI, srtparams[1].trfloatv.b * 2 * PI, srtparams[1].trfloatv.c * 2 * PI);
  dest.mul(t1, t2);

	dest(3,0) = srtparams[2].trfloatv.a;
	dest(3,1) = srtparams[2].trfloatv.b;
	dest(3,2) = srtparams[2].trfloatv.c;
}

static void matrixSRT(fr::matrix& dest, const fr3Float* srtparams)
{
  fr::matrix t1, t2;
  t1.scale(srtparams[0].a, srtparams[0].b, srtparams[0].c);
  t2.rotateHPB(srtparams[1].a * 2 * PI, srtparams[1].b * 2 * PI, srtparams[1].c * 2 * PI);
  dest.mul(t1, t2);

	dest(3,0) = srtparams[2].a;
	dest(3,1) = srtparams[2].b;
	dest(3,2) = srtparams[2].c;
}

static void matrixPivotize(fr::matrix& mtx, const frParam* pivot)
{
  FRDASSERT(pivot[0].type == frtpThreeFloat);
  
  fr::matrix t1, t2;
  t1.translate(-pivot[0].trfloatv.a, -pivot[0].trfloatv.b, -pivot[0].trfloatv.c);
  t2.mul(t1, mtx);
  t1.translate(pivot[0].trfloatv.a, pivot[0].trfloatv.b, pivot[0].trfloatv.c);
  mtx.mul(t2, t1);
}

// ---- transform

class frGFTransform: public frGeometryPlugin
{
  sBool doProcess()
  {
    fr::matrix xform;
    const sInt what = params[4].selectv.sel;

    matrixSRT(xform, params+0);
    matrixPivotize(xform, params+3);
    
    const frModel* inModel = getInputData(0);

    data->lightCount = 0;
    data->createObjects(inModel->nObjects);
    data->cloneModel(*inModel, 0, what != 0);

    if (what == 0)
    {
      for (sInt it = 0; it < data->nObjects; it++)
        data->objects[it].multiplyTransform(xform);
      
      data->transformLights(xform);
    }
    else
    {
      fr::matrix nxform;

      nxform.invTrans3x3(xform);

      for (sInt it = 0; it < data->nObjects; it++)
      {
        frObject* obj = data->objects + it;
        frMesh* msh = obj->getMesh();
        if (!msh)
          continue;
        
        for (sInt i = 0; i < msh->nVerts; i++)
        {
					frVertex* vtx = msh->vertices + i;

					if (vtx->select)
					{
						vtx->pos *= xform;
						vtx->norm *= nxform;
						vtx->norm.norm();
          }
        }

        msh->calcBoundingSphere();
        obj->updateBounds();
      }
    }

    return sTRUE;
  }

public:
  frGFTransform(const frPluginDef* d)
    : frGeometryPlugin(d)
  {
    addThreeFloatParam("Scale", 1.0f, 1.0f, 1.0f, -1e+15f, 1e+15f, 0.01f, 5);
    addThreeFloatParam("Rotate", 0.0f, 0.0f, 0.0f, -1e+15f, 1e+15f, 0.01f, 5);
    addThreeFloatParam("Translate", 0.0f, 0.0f, 0.0f, -1e+15f, 1e+15f, 0.01f, 5);
    addThreeFloatParam("Pivot", 0.0f, 0.0f, 0.0f, -1e+15f, 1e+15f, 0.01f, 5);
    addSelectParam("Transform what", "All Points|Selection", 0);

    params[0].animIndex=1; // scale
    params[1].animIndex=2; // rotate
    params[2].animIndex=3; // translate
    params[3].animIndex=4; // pivot
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver=3;

    strm << ver;

    if (ver>=1)
      strm << params[0].trfloatv << params[1].trfloatv << params[2].trfloatv;

    if (ver>=2)
      strm << params[4].selectv;
    else
      params[4].selectv.sel=0;

    if (ver>=3)
      strm << params[3].trfloatv;
    else
      params[3].trfloatv.a = params[3].trfloatv.b = params[3].trfloatv.c = 0.0f;
  }

  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    putPackedVector(f, params[0].trfloatv, sTRUE);
    putPackedVector(f, params[1].trfloatv);
    putPackedVector(f, params[2].trfloatv);

    fr::vector ppoint(params[3].trfloatv.a, params[3].trfloatv.b, params[3].trfloatv.c);
    sBool usePivot=ppoint.lenSq()>1e-6f;

    f.putsU8(params[4].selectv.sel|(usePivot?2:0));

    if (usePivot)
      putPackedVector(f, params[3].trfloatv);
  }

  void setAnim(sInt index, const sF32* vals)
  {
    params[index - 1].trfloatv.a = vals[0];
    params[index - 1].trfloatv.b = vals[1];
    params[index - 1].trfloatv.c = vals[2];

    dirty = sTRUE;
  }
};

TG2_DECLARE_PLUGIN(frGFTransform, "Transform", 1, 1, 1, sTRUE, sFALSE, 130);

// ---- clone+transform

class frGFCloneXForm: public frGeometryPlugin
{
  sBool doProcess()
  {
    fr::matrix current, xform;
    sInt count = params[3].intv;

    matrixSRT(xform, params + 0);

    const frModel* inModel = getInputData(0);
    const sInt inObjects = inModel->nObjects;

    data->createObjects(inObjects * count);

    current.identity();
    sInt curInd = 0;
    
    while (count--)
    {
      data->lightCount = 0;
      data->cloneModel(*inModel, curInd);
      
      for (sInt i = curInd; i < curInd + inObjects; i++)
        data->objects[i].multiplyTransform(current);
      
      current *= xform;
      curInd += inObjects;
    }
    
    return sTRUE;
  }
  
public:
  frGFCloneXForm(const frPluginDef* d)
    : frGeometryPlugin(d)
  {
    addThreeFloatParam("Scale", 1.0f, 1.0f, 1.0f, -1e+15f, 1e+15f, 0.01f, 5);
    addThreeFloatParam("Rotate", 0.0f, 0.0f, 0.0f, -1e+15f, 1e+15f, 0.01f, 5);
    addThreeFloatParam("Translate", 0.0f, 0.0f, 0.0f, -1e+15f, 1e+15f, 0.01f, 5);
    addIntParam("Count", 1, 1, 16384, 1);
    
    params[0].animIndex=1; // scale
    params[1].animIndex=2; // rotate
    params[2].animIndex=3; // translate
  }
  
  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver=1;
    
    strm << ver;
    
    if (ver==1)
    {
      strm << params[0].trfloatv << params[1].trfloatv << params[2].trfloatv;
      strm << params[3].intv;
    }
  }
  
  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    putPackedVector(f, params[0].trfloatv);
    putPackedVector(f, params[1].trfloatv);
    putPackedVector(f, params[2].trfloatv);

    if (params[3].intv<255)
      f.putsU8(params[3].intv);
    else
    {
      f.putsU8(255);
      f.putsU16(params[3].intv);
    }
  }

  void setAnim(sInt index, const sF32* vals)
  {
    params[index - 1].trfloatv.a = vals[0];
    params[index - 1].trfloatv.b = vals[1];
    params[index - 1].trfloatv.c = vals[2];

    dirty = sTRUE;
  }
};

TG2_DECLARE_PLUGIN(frGFCloneXForm, "Clone/XForm", 1, 1, 1, sTRUE, sFALSE, 131);

// ---- select

class frGFSelect: public frGeometryPlugin
{
  frMesh      *m_cube, *m_sphere;
  frMaterial  *m_objMtrl;

  sBool doProcess()
  {
    const sInt mode = params[0].selectv.sel;

    const frModel* input = getInputData(0);
    data->lightCount = 0;
    data->createObjects(input->nObjects, mode >= 3 && mode <= 5);
    data->cloneModelLights(*input);

    for (sInt it = 0; it < input->nObjects; it++)
    {
      const frObject* inObj = input->objects + it;
      frObject* obj = data->objects + it;

      data->cloneObject(*inObj, it, sTRUE);

			frMesh* msh = obj->getMesh();
      if (!msh)
        continue;

      switch (mode)
      {
      case 0: // select all
			case 1: // select none
			case 2: // invert
				{
					sU32 andMask = mode == 2, xorMask = mode != 1;

					for (sInt i = 0; i < msh->nVerts; i++)
						msh->vertices[i].select = (msh->vertices[i].select & andMask) ^ xorMask;

					for (sInt i = 0; i < msh->nPrims; i++)
						msh->primSelect[i] = (msh->primSelect[i] & andMask) ^ xorMask;
				}
				break;

      case 3: // set
      case 4: // add
      case 5: // sub
        {
          fr::vector shpPos(params[2].trfloatv.a, params[2].trfloatv.b, params[2].trfloatv.c);
          fr::vector shpSize(params[3].trfloatv.a, params[3].trfloatv.b, params[3].trfloatv.c);
          const sBool greedy = !!params[4].selectv.sel;
					const sInt selMode = params[1].selectv.sel;

					for (sInt j = 0; j < 3; j++)
					{
						if (shpSize.v[j])
							shpSize.v[j] = 1.0f / shpSize.v[j];
					}
				
					for (sInt i = 0; i < msh->nVerts; i++)
					{
						frVertex* vtx = msh->vertices + i;
						fr::vector rPos = vtx->pos - shpPos;
						sU32 msk = 3;
						sF32 d = 0.0f;

						for (sInt j = 0; j < 3; j++)
						{
							sF32 v = rPos.v[j] * shpSize.v[j];
							if (fabs(v) >= 1.0f)
								msk = 0;

							d += v * v;
						}

						if (d >= 1.0f && selMode)
							msk = 0;

						if (mode == 3)
							vtx->select = msk;
						else if (mode == 4)
							vtx->select |= msk;
						else if (mode == 5)
							vtx->select &= ~msk;
					}

					// vertex->face selection
					for (sInt i = 0; i < msh->nPrims; i++)
					{
						const sU16* bi = msh->indices + i * 3;
						frVertex* vt0 = msh->vertices + bi[0];
						frVertex* vt1 = msh->vertices + bi[1];
						frVertex* vt2 = msh->vertices + bi[2];
						const sU32 s0 = vt0->select;
						const sU32 s1 = vt1->select;
						const sU32 s2 = vt2->select;

						if (greedy)
							msh->primSelect[i] = s0 | s1 | s2;
						else
							msh->primSelect[i] = s0 & s1 & s2;

						if (!msh->primSelect[i])
						{
							vt0->select &= 1;
							vt1->select &= 1;
							vt2->select &= 1;
						}
					}
        }
        break;
      }
    }

    if (mode >= 3 && mode <= 5)
    {
      frObject* obj = data->virtObjects;

      obj->setMesh(params[1].selectv.sel ? m_sphere : m_cube);
      obj->setMaterial(m_objMtrl);

      fr::matrix t1, t2, t3;
      t1.scale(params[3].trfloatv.a, params[3].trfloatv.b, params[3].trfloatv.c);
      t2.translate(params[2].trfloatv.a, params[2].trfloatv.b, params[2].trfloatv.c);
      t3.mul(t1, t2);

      obj->setTransform(t3);
    }
    
    return sTRUE;
  }

  sBool onParamChanged(sInt param)
  {
    return param == 0; // update params if type changed
  }

  void createVirtualObjects()
  {
    sInt i;

    m_cube = new frMesh;
    m_cube->addRef();
    m_cube->type = 1;
    m_cube->resize(8, 12);

    frVertex* verts = m_cube->vertices;
    for (i = 0; i < 8; i++)
      verts[i].pos.set(i & 1 ? 1.0f : -1.0f, i & 2 ? 1.0f : -1.0f, i & 4 ? 1.0f : -1.0f);

		sU16* inds = m_cube->indices;
    sInt bm1 = 1, bm2 = 2, bm4 = 4, bi;
    for (i = 0; i < 3; i++)
    {
      bi = 0;

      for (sInt j = 0; j < 2; j++)
      {
				inds[0] = bi;
				inds[1] = bi ^ bm1;
				inds[2] = bi ^ bm2;
				inds[3] = bi ^ (bm1 | bm2);
				inds += 4;
        bi ^= bm4;
      }

      bm1 = ((bm1 << 1) | (bm1 >> 2)) & 7;
      bm2 = ((bm2 << 1) | (bm2 >> 2)) & 7;
      bm4 = ((bm4 << 1) | (bm4 >> 2)) & 7;
    }

    m_sphere = new frMesh;
    m_sphere->addRef();
    m_sphere->type = 1;
    m_sphere->resize(48 * 3, 48 * 3);

    verts = m_sphere->vertices;
    for (i = 0; i < 48; i++)
    {
      const sF32 x = cos(i * 2 * PI / 48.0f), y = sin(i * 2 * PI / 48.0f);

      verts[i +  0].pos.set(x, y, 0.0f);
      verts[i + 48].pos.set(0.0f, y, x);
      verts[i + 96].pos.set(x, 0.0f, y);
    }

		inds = m_sphere->indices;
    for (i = 0; i < 48; i++)
    {
      const sInt ni = (i + 1) % 48;

			inds[i * 2 +   0] = i;
			inds[i * 2 +   1] = ni;
			inds[i * 2 +  96] = i + 48;
			inds[i * 2 +  97] = ni + 48;
			inds[i * 2 + 192] = i + 96;
			inds[i * 2 + 193] = ni + 96;
    }

    m_objMtrl = new frMaterial;
    m_objMtrl->lColor = 0xff0000;
  }

public:
  frGFSelect(const frPluginDef* d)
    : frGeometryPlugin(d)
  {
    addSelectParam("Mode", "All|None|Invert|Set|Add|Subtract", 0);
    addSelectParam("Shape", "Cube|Sphere", 0);
    addThreeFloatParam("Position", 0.0f, 0.0f, 0.0f, -1e+15f, 1e+15f, 0.01f, 5);
    addThreeFloatParam("Size", 1.0f, 1.0f, 1.0f, 0.0f, 1e+15f, 0.01f, 5);
    addSelectParam("Face selection", "Lazy|Greedy", 0);

    createVirtualObjects();
  }

  ~frGFSelect()
  {
    m_cube->release();
    m_sphere->release();
    m_objMtrl->release();
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver = 1;

    strm << ver;

    if (ver == 1)
    {
      strm << params[0].selectv << params[1].selectv;
      strm << params[2].trfloatv << params[3].trfloatv;
      strm << params[4].selectv;
    }
  }

  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    f.putsU8(params[0].selectv.sel | (params[1].selectv.sel << 3) | (params[4].selectv.sel << 4));

    if (params[0].selectv.sel >= 3)
    {
      putPackedVector(f, params[2].trfloatv);
      putPackedVector(f, params[3].trfloatv, sTRUE);
    }
  }

  sBool displayParameter(sU32 index)
  {
    switch (index)
    {
    case 0: 
      return sTRUE;

    case 1: case 2: case 3: case 4:
      return params[0].selectv.sel >= 3;
    }

    return sFALSE;
  }
};

TG2_DECLARE_PLUGIN(frGFSelect, "Select", 1, 1, 1, sTRUE, sFALSE, 133);

// ---- delete selection

class frGFDeleteSelection: public frGeometryPlugin
{
  sBool doProcess()
  {
    const frModel* input = getInputData(0);

    data->lightCount = 0;
    data->createObjects(input->nObjects);
    data->cloneModelLights(*input);

    for (sInt it = 0; it < data->nObjects; it++)
    {
      const frObject* inObj =  input->objects + it;
      frObject *obj = data->objects + it;

      if (!inObj->getMesh())
      {
        data->cloneObject(*inObj, it);
        continue;
      }
      else
        data->cloneObject(*inObj, it, sTRUE); // hardclone "real" objects

      const frMesh* mesh = obj->getMesh();
			const sU32 selMask = 1;

      // count faces in selection
      const sInt faceCount = mesh->nPrims;
      sInt selFaceCount = 0;
      for (sInt i = 0; i < faceCount; i++)
				selFaceCount += mesh->primSelect[i] ? 1 : 0;

      // count vertices in selection
      const sInt vertCount = mesh->nVerts;
      sInt selVertCount = 0;
      for (sInt i = 0; i < vertCount; i++)
      {
				if (mesh->vertices[i].select == 3)
					selVertCount++;
      }

      if (selFaceCount == faceCount && selVertCount == vertCount) // everything selected?
        obj->setMesh(0); // simply kill the mesh
      else // else delete selectively.
      {
        frMesh* msh = obj->getMesh(), *origMesh = inObj->getMesh();

        sU32* vertRemap = new sU32[vertCount];
        msh->resize(vertCount - selVertCount, faceCount - selFaceCount);

        // build new vertex list
        sU32 vertPos = 0;
        for (sInt i = 0; i < vertCount; i++)
        {
          vertRemap[i] = vertPos;
					if (origMesh->vertices[i].select != 3)
					{
            msh->vertices[vertPos] = origMesh->vertices[i];
						msh->vertices[vertPos++].select = 0;
					}
        }

        // build new face list
        sU32 facePos = 0;
        for (sInt i = 0; i < faceCount; i++)
        {
					if (origMesh->primSelect[i])
						continue;

          sU16* dst = msh->indices + facePos * 3;
          const sU16* src = origMesh->indices + i * 3;

          dst[0] = vertRemap[src[0]];
          dst[1] = vertRemap[src[1]];
          dst[2] = vertRemap[src[2]];
					msh->primSelect[facePos++] = 0;
        }

        delete[] vertRemap;

        msh->calcBoundingSphere();
        obj->updateBounds();
      }
    }
    
    return sTRUE;
  }

public:
  frGFDeleteSelection(const frPluginDef* d)
    : frGeometryPlugin(d)
  {
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver=1;

    strm << ver;
  }

  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
  }
};

TG2_DECLARE_PLUGIN(frGFDeleteSelection, "Delete Selection", 1, 1, 1, sTRUE, sFALSE, 134);

// ---- bend

class frGFBend: public frGeometryPlugin
{
  sBool doProcess()
  {
    const frModel* input = getInputData(0);
    const sInt wgtType = params[6].selectv.sel, blend = params[7].selectv.sel, mode = params[8].selectv.sel;
    sBool firstSet = sFALSE;
    sF32  minY, maxY;
    
    data->createObjects(input->nObjects);
    data->lightCount = 0;
    data->cloneModelLights(*input);

    // first pass over data: hard clone and find minima/maxima

    for (sInt cit = 0; cit < input->nObjects; cit++)
    {
      frObject* obj = data->objects + cit;

      data->cloneObject(input->objects[cit], cit, sTRUE);

      const frMesh* msh = obj->getMesh();
      if (!msh)
        continue;

      for (sInt i = 0; i < msh->nVerts; i++)
      {
        frVertex* vtx = msh->vertices + i;

				if (mode == 1 && !vtx->select)
					continue;

        if (!firstSet)
        {
          minY = maxY = vtx->pos.y;
          firstSet = sTRUE;
        }
        else
        {
          if (vtx->pos.y < minY)
            minY = vtx->pos.y;

          if (vtx->pos.y > maxY)
            maxY = vtx->pos.y;
        }
      }
    }

    // second pass: perform bend
    fr::matrix xform1, xform2;
    if (!blend)
    {
      matrixSRT(xform1, params+0);
      matrixSRT(xform2, params+3);
    }

    fr::matrix xfon1, xfon2;

    if (!blend)
    {
      xfon1.invTrans3x3(xform1);
      xfon2.invTrans3x3(xform2);
    }

    sF32 deltaY = maxY - minY;
    if (deltaY)
      deltaY = -1.0f / deltaY;

    for (sInt it = 0; it < data->nObjects; it++)
    {
      frObject& obj = data->objects[it];
      frMesh* msh = obj.getMesh();
      if (!msh)
        continue;

      for (sInt i = 0; i < msh->nVerts; i++)
      {
				frVertex* vtx = msh->vertices + i;
				if (mode == 1 && !vtx->select)
					continue;

        sF32 weight = (vtx->pos.y - maxY) * deltaY;
        if (wgtType == 1) // fake-spline weighting?
          weight = weight * weight * (3.0f - 2.0f * weight);

        if (!blend)
        {
          fr::vector t1, t2;

          t1.transform(vtx->pos, xform1);
          t2.transform(vtx->pos, xform2);
          vtx->pos.lerp(t1, t2, weight);

					t1.transform(vtx->norm, xfon1);
					t2.transform(vtx->norm, xfon2);
					vtx->norm.lerp(t1, t2, weight);
					vtx->norm.norm();
        }
        else
        {
          fr3Float f[3];

          for (sInt i = 0; i < 3; i++)
          {
            f[i].a = params[i].trfloatv.a + (params[i + 3].trfloatv.a - params[i].trfloatv.a) * weight;
            f[i].b = params[i].trfloatv.b + (params[i + 3].trfloatv.b - params[i].trfloatv.b) * weight;
            f[i].c = params[i].trfloatv.c + (params[i + 3].trfloatv.c - params[i].trfloatv.c) * weight;
          }

          matrixSRT(xform1, f);
          vtx->pos *= xform1;

          xfon1.invTrans3x3(xform1);
          vtx->norm *= xfon1;
          vtx->norm.norm();
        }
      }

      msh->calcBoundingSphere();
      obj.updateBounds();
    }

    return sTRUE;
  }

public:
  frGFBend(const frPluginDef* d)
    : frGeometryPlugin(d)
  {
    addThreeFloatParam("Scale 1", 1.0f, 1.0f, 1.0f, 0.0f, 1e+15f, 0.01f, 5);
    addThreeFloatParam("Rotate 1", 0.0f, 0.0f, 0.0f, -1e+15f, 1e+15f, 0.01f, 5);
    addThreeFloatParam("Translate 1", 0.0f, 0.0f, 0.0f, -1e+15f, 1e+15f, 0.01f, 5);
    params[0].animIndex=1; // scale
    params[1].animIndex=2; // rotate
    params[2].animIndex=3; // translate
    
    addThreeFloatParam("Scale 2", 1.0f, 1.0f, 1.0f, 0.0f, 1e+15f, 0.01f, 5);
    addThreeFloatParam("Rotate 2", 0.0f, 0.0f, 0.0f, -1e+15f, 1e+15f, 0.01f, 5);
    addThreeFloatParam("Translate 2", 0.0f, 0.0f, 0.0f, -1e+15f, 1e+15f, 0.01f, 5);
    params[3].animIndex=4; // scale
    params[4].animIndex=5; // rotate
    params[5].animIndex=6; // translate

    addSelectParam("Weighting", "Linear|Fake-spline", 0);
    addSelectParam("Blend", "Vertices|Transform", 0);
    addSelectParam("Process", "All Points|Selection", 0);
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver = 4;

    strm << ver;

    // old versions (import)
    if (ver >= 1 && ver <= 3)
    {
      strm << params[0].trfloatv << params[1].trfloatv << params[2].trfloatv;
      strm << params[3].trfloatv << params[4].trfloatv << params[5].trfloatv;
      strm << params[8].selectv;

      if (ver >= 2)
        strm << params[6].selectv;
      else
        params[6].selectv.sel = 0; // linear weighting

      if (ver >= 3)
        strm << params[7].selectv;
      else
        params[7].selectv.sel = 0; // blend vertices

      if (params[7].selectv.sel == 1) // blend transform? swap the matrices
      {
        fr3Float t;

        for (sInt i = 0; i < 3; i++)
        {
          t = params[i].trfloatv;
          params[i].trfloatv = params[i + 3].trfloatv;
          params[i + 3].trfloatv = t;
        }
      }
    }

    // new version
    if (ver == 4)
    {
      strm << params[0].trfloatv << params[1].trfloatv << params[2].trfloatv;
      strm << params[3].trfloatv << params[4].trfloatv << params[5].trfloatv;
      strm << params[6].selectv << params[7].selectv << params[8].selectv;
    }
  }

  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    for (sInt i = 0; i < 6; i++)
      putPackedVector(f, params[i].trfloatv);

    f.putsU8(params[6].selectv.sel+(params[8].selectv.sel<<1)+(params[7].selectv.sel<<2));
  }

  void setAnim(sInt index, const sF32* vals)
  {
    params[index - 1].trfloatv.a = vals[0];
    params[index - 1].trfloatv.b = vals[1];
    params[index - 1].trfloatv.c = vals[2];

    dirty = sTRUE;
  }
};

TG2_DECLARE_PLUGIN(frGFBend, "Bend", 1, 1, 1, sTRUE, sFALSE, 135);

// ---- UV mapping

class frGFUVMap: public frGeometryPlugin
{
  sBool doProcess()
  {
    const sInt  mode = params[0].selectv.sel;
    const sInt  which = params[4].selectv.sel;

    const frModel* input = getInputData(0);

    fr::matrix xform, nxform;
    matrixSRT(xform, params + 1);
    nxform.invTrans3x3(xform);

    data->createObjects(input->nObjects);
    data->lightCount = 0;
    data->cloneModel(*input, 0, mode != 1 || which == 1);

    for (sInt it = 0; it < data->nObjects; it++)
    {
      frObject* obj = data->objects + it;

      if (mode == 0 || mode == 2)
      {
        frMesh* msh = obj->getMesh();

        if (msh)
        {
          frVertex* vt = msh->vertices;

          for (sU32 i = 0; i < msh->nVerts; i++, vt++)
          {
						if (which != 0 && !vt->select)
							continue;

            if (mode == 0) // project
							vt->uvw.transform(vt->pos, xform);
            else if (mode == 2) // cubic projection
            {
              sInt axis = 0;
              sF32 dom = 0.0f;
              fr::vector norm;

              norm.transform(vt->norm, nxform);

              for (sInt i = 0; i < 3; i++)
              {
                sF32 v = norm.v[i];
                v *= v;

                if (v > dom)
                {
                  axis = i;
                  dom = v;
                }
              }

              fr::vector tmp = vt->pos;
              tmp *= xform;

              vt->uvw.x = (axis == 0) ? tmp.z : tmp.x;
              vt->uvw.y = (axis == 1) ? tmp.z : tmp.y;
							vt->uvw.z = 0.0f;
            }
          }
        }

        obj->updateBounds();
      }
      else
      {
        if (which == 0) // all points?
          obj->multiplyTexTransform(xform); // then use matrix ops
        else // else transform them seperately
        {
          frMesh* msh = obj->getMesh();

          if (msh)
          {
            frVertex* vt = msh->vertices;

            for (sU32 i = 0; i < msh->nVerts; i++, vt++)
            {
							if (vt->select)
								vt->uvw *= xform;
            }
          }

          obj->updateBounds();
        }
      }
    }

    return sTRUE;
  }

public:
  frGFUVMap(const frPluginDef* d)
    : frGeometryPlugin(d)
  {
    addSelectParam("Mode", "Project|Transform|Cubic Projection", 0);

    addThreeFloatParam("Scale", 1.0f, 1.0f, 1.0f, -1e+15f, 1e+15f, 0.01f, 5);
    addThreeFloatParam("Rotate", 0.0f, 0.0f, 0.0f, -1e+15f, 1e+15f, 0.01f, 5);
    addThreeFloatParam("Translate", 0.0f, 0.0f, 0.0f, -1e+15f, 1e+15f, 0.01f, 5);

    params[1].animIndex=1; // scale
    params[2].animIndex=2; // rotate
    params[3].animIndex=3; // translate

    addSelectParam("Process", "All Points|Selection", 0);
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver=2;

    strm << ver;

    if (ver==1 || ver==2)
    {
      strm << params[0].selectv;
      strm << params[1].trfloatv << params[2].trfloatv << params[3].trfloatv;
    }

    if (ver==2)
      strm << params[4].selectv;
    else
      params[4].selectv.sel=0;
  }

  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    f.putsU8(params[0].selectv.sel|(params[4].selectv.sel<<2));

    for (sInt i=1; i<4; i++)
      putPackedVector(f, params[i].trfloatv, i==1, ((i == 1 && params[0].selectv.sel == 1) || (i == 3)) ? 4 : 0);
  }

  void setAnim(sInt index, const sF32* vals)
  {
    params[index].trfloatv.a = vals[0];
    params[index].trfloatv.b = vals[1];
    params[index].trfloatv.c = vals[2];

    dirty = sTRUE;
  }
};

TG2_DECLARE_PLUGIN(frGFUVMap, "UV Mapping", 1, 1, 1, sTRUE, sFALSE, 136);

// ---- make doublesided

class frGFDoubleSided: public frGeometryPlugin
{
  sBool doProcess()
  {
    const sInt mode = params[0].selectv.sel;
    const frModel* input = getInputData(0);

    data->lightCount = 0;
    data->createObjects(input->nObjects * (mode & 1 ? 1 : 2));
    data->cloneModel(*input, 0, sTRUE);

    for (sInt it = 0; it < input->nObjects; it++)
    {
      frMesh *msh = data->objects[it].getMesh();
      if (!msh)
        continue;
      
      // flip triangle indices
      for (sInt i = 0; i<msh->nPrims * 3; i += 3)
      {
        sU16 t = msh->indices[i];
        msh->indices[i] = msh->indices[i + 2];
        msh->indices[i + 2] = t;
      }
      
      if (msh->vFormat == 0 && mode != 3)
      {
        // flip vertex normals
        for (sInt i = 0; i < msh->nVerts; i++)
          msh->vertices[i].norm.scale(-1.0f);
      }
    }
    
    if (mode != 1 && mode != 3)
    {
      for (sInt it = 0; it < input->nObjects; it++)
      {
        frObject* newObj = data->objects + it + input->nObjects;

        data->cloneObject(input->objects[it], input->nObjects + it, sTRUE);

        if (mode == 2) // alpha setup
        {
          sInt pass = newObj->getRenderPass();
          if (++pass > 7)
            pass = 7;

          newObj->setRenderPass(pass);
        }
      }
    }
    
    return sTRUE;
  }
  
public:
  frGFDoubleSided(const frPluginDef* d)
    : frGeometryPlugin(d)
  {
    addSelectParam("Mode", "Make Doublesided|Invert (Real)|Alpha Setup|Invert (Cull)", 0);
  }
  
  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver=2;
    
    strm << ver;

    if (ver >= 2)
      strm << params[0].selectv;
    else
      params[0].selectv.sel=0;
  }

  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    f.putsU8(params[0].selectv.sel);
  }
  
  const sChar *getName() const
  {
    return "Doublesided";
  }
};

TG2_DECLARE_PLUGIN(frGFDoubleSided, "Make Double Sided", 1, 1, 1, sTRUE, sFALSE, 137);

// ---- object merge

class frGFObjMerge: public frGeometryPlugin
{
  sBool doProcess()
  {
    const frModel *in = getInputData(0);

    data->createObjects(in->nObjects);
    data->lightCount = 0;
    data->cloneModelLights(*in);

    sInt nRealObjects = 0;

    for (sInt it = 0; it != in->nObjects; it++)
    {
      const frObject* inObj = in->objects + it;

      frObject temp;
      temp.hardClone(*inObj);

      const frMesh* msh = temp.getMesh();
      const frMaterial* mat = temp.getMaterial();

      if (msh)
      {
        sInt it2;
        for (it2 = 0; it2 < nRealObjects; it2++)
        {
          frObject* inObj2 = data->objects + it2;

          if (inObj2->getMaterial() == mat && inObj2->getMesh()->nVerts + msh->nVerts < 65536 && inObj2->getMesh()->nPrims + msh->nPrims < 65536 &&
            inObj2->getRenderPass() == inObj->getRenderPass() && inObj2->getMesh()->type == msh->type)
          {
            frMesh* destMesh = new frMesh;
            frMesh* srcMesh2 = inObj2->getMesh();
            const sInt tMod = 3 - msh->type;

            destMesh->resize(msh->nVerts + srcMesh2->nVerts, msh->nPrims + srcMesh2->nPrims);
            memcpy(destMesh->vertices, msh->vertices, msh->nVerts * sizeof(frVertex));
            memcpy(destMesh->vertices + msh->nVerts, srcMesh2->vertices, srcMesh2->nVerts * sizeof(frVertex));
            memcpy(destMesh->indices, msh->indices, msh->nPrims * sizeof(sU16) * tMod);
						memcpy(destMesh->primSelect, msh->primSelect, msh->nPrims);
						memcpy(destMesh->primSelect + msh->nPrims, srcMesh2->primSelect, srcMesh2->nPrims);
						destMesh->normAlias = new sU16[destMesh->nVerts];
						if (msh->normAlias)
							memcpy(destMesh->normAlias, msh->normAlias, sizeof(sU16) * msh->nVerts);
						else
						{
							for (sU32 i = 0; i < msh->nVerts; i++)
								destMesh->normAlias[i] = i;
						}

            sU16* destInd = destMesh->indices + msh->nPrims * tMod;
            for (sU32 i = 0; i < srcMesh2->nPrims * tMod; i++)
              *destInd++ = srcMesh2->indices[i] + msh->nVerts;

						sU16* destNorm = destMesh->normAlias + msh->nVerts;
						for (sU32 i = 0; i < srcMesh2->nVerts; i++)
							*destNorm++ = (srcMesh2->normAlias ? srcMesh2->normAlias[i] : i) + msh->nVerts;

            destMesh->calcBoundingSphere();
            inObj2->setMesh(destMesh);

            break;
          }
        }

        if (it2 == nRealObjects)
          data->objects[nRealObjects++] = temp;
      }
    }

    data->nObjects = nRealObjects;

    return sTRUE;
  }

public:
  frGFObjMerge(const frPluginDef* d)
    : frGeometryPlugin(d)
  {
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver=1;

    strm << ver;
  }

  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
  }
};

TG2_DECLARE_PLUGIN(frGFObjMerge, "Obj Merge", 1, 1, 1, sTRUE, sFALSE, 143);

// ---- apply lightmap

class frGFApplyLightmap: public frGeometryPlugin
{
  sBool doProcess()
  {
    const frModel* input = getInputData(0);

    data->createObjects(input->nObjects);
    data->lightCount = 0;
    data->cloneModel(*input, 0);

    fr::matrix pMatrix;
    matrixSRT(pMatrix, params + 2);

    for (sInt it = 0; it < data->nObjects; it++)
    {
      frObject* inObj = data->objects + it;
      const frMaterial* mat = inObj->getMaterial();
      if (!inObj->getMesh() || !mat)
        continue;

      frMaterial *cmat = new frMaterial(*mat);
			cmat->textureRef[1] = params[0].linkv->opID;
			cmat->opSt2 = params[1].selectv.sel + 1;
			cmat->coordMode[1] = 3; // lightmap
      cmat->pMatrix = pMatrix;

      inObj->setMaterial(cmat);
      cmat->release();
    }

    return sTRUE;
  }

public:
  frGFApplyLightmap(const frPluginDef* d)
    : frGeometryPlugin(d, 1)
  {
    addLinkParam("Lightmap", 0, 0);
    addSelectParam("Mode", "Modulate|Modulate2X", 0);
    addThreeFloatParam("Scale", 1.0f, 1.0f, 1.0f, -1e+15f, 1e+15f, 0.01f, 5);
    addThreeFloatParam("Rotate", 0.0f, 0.0f, 0.0f, -1e+15f, 1e+15f, 0.01f, 5);
    addThreeFloatParam("Translate", 0.0f, 0.0f, 0.0f, -1e+15f, 1e+15f, 0.01f, 5);
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver = 3;

    strm << ver;

    if (ver >= 1 && ver <= 2)
    {
      fr::string tempstr;

      strm << tempstr;
      g_graph->loadRegisterLink(tempstr, 0, 0);

      if (ver == 1)
      {
        strm << tempstr;
        params[1].selectv.sel = 0;
      }
      else
        strm << params[1].selectv;

      strm << params[2].trfloatv << params[3].trfloatv << params[4].trfloatv;
    }
    else if (ver == 3)
      strm << params[0].linkv << params[1].selectv.sel << params[2].trfloatv << params[3].trfloatv << params[4].trfloatv;
  }

  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    f.putsU8(params[1].selectv.sel);

    for (sInt i=2; i<5; i++)
      putPackedVector(f, params[i].trfloatv);
  }
};

TG2_DECLARE_PLUGIN(frGFApplyLightmap, "Light map", 1, 1, 1, sTRUE, sFALSE, 144);

// ---- wuschel

class frGFWuschel: public frGeometryPlugin
{
  sInt        hairCount, hairSegments;
  sF32        hairLen;
  fr::matrix  omtx;
  sBool       justInit;

  struct hair
  {
    fr::vector  *positions;
    fr::vector  norm;
    sU32        color;
    sF32        length;

    ~hair()
    {
      delete[] positions;
    }
    
    void anim(sInt count, const fr::matrix &dx, const fr::matrix &nxf, sF32 hlen, sF32 delay)
    {
      fr::vector temp, t2;

      positions[0].transform(dx);

      for (sInt i=1; i<count; i++)
      {
        t2=norm;
        t2.transform(nxf);
        temp.addscale(positions[i-1], t2, hlen);
        positions[i].scale(delay);
        positions[i].addscale(temp, 1.0f-delay);
      }
    }
  };

  hair      *hairs;
  frMesh    *hMesh;

  inline sU32 srandom(sU32 seed)
  {
    return ((seed*31337303+1) & 0x7fffffff);
  }

  sBool doProcess()
  {
    const frModel* input = getInputData(0);
    data->createObjects(input->nObjects + 1);
    data->lightCount = 0;
    data->cloneModel(*input, 0, sTRUE);

    sU32 seed=params[0].intv;
    hairCount=params[1].intv;
    hairSegments=params[2].intv;
    hairLen=params[3].floatv;
    
    seed^=seed<<13;
    seed+=0xdeadbeef;

    sU32 nPolys=0;

    for(sInt it=0;it<data->nObjects-1;++it)
    {
      const frMesh* msh = data->objects[it].getMesh();

      if (msh && msh->type==0 && msh->vFormat==0)
        nPolys += msh->nPrims;
    }

    if (!nPolys)
      return sTRUE;

    FRSAFEDELETEA(hairs);
    hairs=new hair[hairCount];

    for (sInt i=0; i<hairCount; i++)
    {
      seed=srandom(seed);
      sInt poly=(seed>>16) % nPolys;
      const frMesh *mesh;
      const frMaterial *mtrl;

      for (sInt it=0; it!=data->nObjects; ++it)
      {
        mesh = data->objects[it].getMesh();
        mtrl = data->objects[it].getMaterial();

        if (mesh && mesh->type==0 && mesh->vFormat==0)
        {
          if (poly >= mesh->nPrims)
            poly -= mesh->nPrims;
          else
            break;
        }
      }

      seed=srandom(seed); sF32 s=seed/2147483648.0f;
      seed=srandom(seed); sF32 t=seed/2147483648.0f;
      if (s+t>1.0f)
      {
        s=1.0f-s;
        t=1.0f-t;
      }

      poly *= 3;
      const sF32 w1=1.0f-s-t, w2=s, w3=t;
      const frVertex *v1=mesh->vertices+mesh->indices[poly+0];
      const frVertex *v2=mesh->vertices+mesh->indices[poly+1];
      const frVertex *v3=mesh->vertices+mesh->indices[poly+2];

      fr::vector pos, norm;
      pos.scale(v1->pos, w1);     norm.scale(v1->norm, w1);
      pos.addscale(v2->pos, w2);  norm.addscale(v2->norm, w2);
      pos.addscale(v3->pos, w3);  norm.addscale(v3->norm, w3);
      norm.norm();

      hair *h=hairs+i;
      h->positions=new fr::vector[hairSegments];
      h->norm=norm;

      h->length=hairLen;
      h->color=0xffff;

      if (mtrl && mtrl->textureRef[0])
      {
        frTexturePlugin *tp = (frTexturePlugin *) g_graph->m_ops[mtrl->textureRef[0]].plg;
        frTexture *tex=tp?tp->getData():0;
        
        if (tex)
        {
          tex->lock();
          tp->process();

          const sU16 *tdata=tex->data;
          const sU32 u=(v1->uvw.x*w1+v2->uvw.x*w2+v3->uvw.x*w3)*tex->w*65536;
          const sU32 v=(v1->uvw.y*w1+v2->uvw.y*w2+v3->uvw.y*w3)*tex->h*65536;
          const sU32 x=(u>>16)&(tex->w-1), y=(v>>16)&(tex->h-1), xp=(x+1)&(tex->w-1), yp=(y+1)&(tex->h-1);
          const sU32 xf=(u&0xffff)>>8, yf=(v&0xffff)>>8;
          const sU32 o1=((y*tex->w)+x)*4, o2=((y*tex->w)+xp)*4, o3=((yp*tex->w)+x)*4, o4=((yp*tex->w)+xp)*4;
          sS32 c[4];

          for (sInt i=0; i<4; i++)
          {
            const sU16 s1=tdata[o1+i], s2=tdata[o2+i], s3=tdata[o3+i], s4=tdata[o4+i];
            const sU32 t1=(s1<<8)+((s2-s1)*xf);
            const sU32 t2=(s3<<8)+((s4-s3)*xf);
            const sU32 t=(t1<<8)+((t2-t1)*yf);
            c[i]=t>>(16+7);
          }

          h->color=(c[2]<<16)|(c[1]<<8)|c[0];
          h->length*=1.0f-(c[3]/255.0f);

          tex->unlock();
        }
      }

      for (sInt j=0; j<hairSegments; j++)
        h->positions[j].addscale(pos, norm, h->length*j/(hairSegments-1));
    }

    hMesh->resize(hairCount*hairSegments, hairCount*(hairSegments-1)*2);

    //data->objects[data->nObjects - 1].worldSpace = this; // fake this

    justInit=sTRUE;
    omtx.identity();
    
    return sTRUE;
  }

public:
  frGFWuschel(const frPluginDef *d)
    : frGeometryPlugin(d), hairs(0)
  {
    addIntParam("Random seed", 4567, 0, 65535, 8);
    addIntParam("Hair count", 1000, 1, 5000, 5);
    addIntParam("Hair segments", 5, 3, 8, 1);
    addFloatParam("Hair length", 0.25f, 0.1f, 10.0f, 0.01f, 4);
    addFloatParam("Delay", 0.65f, 0.00f, 1.00f, 0.01f, 4);

    hMesh=new frMesh;
    hMesh->type=1;
    hMesh->vFormat=1;
  }

  ~frGFWuschel()
  {
    FRSAFEDELETEA(hairs);
    hMesh->release();
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver=1;

    strm << ver;

    if (ver==1)
    {
      strm << params[0].intv << params[1].intv << params[2].intv;
      strm << params[3].floatv << params[4].floatv;
    }
  }

  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    /*f.putsU16(params[0].intv);
    f.putsU16(params[1].intv);
    f.putsU8(params[2].intv);
    putPackedFloat(f, params[3].floatv);
    putPackedFloat(f, params[4].floatv);*/
  }

  frMesh *frame(frObject *obj, const fr::matrix &cam)
  {    
    sU16      *indptr=hMesh->indices;
    frVertex  *vertptr = hMesh->vertices;
    const sF32 delay=params[4].floatv;
    sInt      basev=0;

    fr::matrix xf=obj->getTransform(), nxf=xf, temp;
    nxf(3,0)=nxf(3,1)=nxf(3,2)=0.0f;
    
    if (justInit)
    {
      for (sInt i=0; i<hairCount; i++)
      {
        for (sInt j=0; j<hairSegments; j++)
          hairs[i].positions[j].transform(xf);
      }

      temp.identity();
      xf.identity();
      nxf.identity();

      justInit=sFALSE;
    }
    else
      temp.mul(omtx, xf);

    const sF32 ihs=1.0f/(hairSegments-1);
    
    for (sInt i=0; i<hairCount; i++)
    {
      hairs[i].anim(hairSegments, temp, nxf, hairs[i].length*ihs, delay);

      for (sInt j=0; j<hairSegments; j++)
      {
        vertptr->pos=hairs[i].positions[j];
        vertptr->color=hairs[i].color;

        vertptr++;
      }

      for (sInt j=1; j<hairSegments; j++)
      {
        *indptr++=basev+j-1;
        *indptr++=basev+j;
      }

      basev+=hairSegments;
    }

    omtx.inverse(obj->getTransform());

    return hMesh;
  }
};

TG2_DECLARE_PLUGIN(frGFWuschel, "Wuschel", 1, 1, 1, sTRUE, sFALSE, 146);

// ---- plode

class frGFPlode: public frGeometryPlugin
{
  sInt    m_type;      // 0=explode 1=implode
  sF32    m_distance;  // (explode only)
  sF32    m_exponent;  // (implode only)
  sInt    m_process;   // 0=all 1=selection

  sBool doProcess()
  {
    const frModel* input = getInputData(0);
    sF32 maxDist = 0.0f, invMaxDist;

    const sInt op = params[0].selectv.sel;
    const sF32 param = params[1 + op].floatv;
    const sInt mode = params[3].selectv.sel;
    
    data->createObjects(input->nObjects);
    data->lightCount = 0;
    data->cloneModelLights(*input);

    // first pass over data: hard clone and find max distance (implode only)
    for (sInt cit = 0; cit < input->nObjects; cit++)
    {
      frObject* obj = data->objects + cit;

      data->cloneObject(input->objects[cit], cit, sTRUE);

      const frMesh* msh = obj->getMesh();
      if (!msh)
        continue;

			if (op)
			{
				for (sInt i = 0; i < msh->nVerts; i++)
				{
					frVertex* vtx = msh->vertices + i;

					if (mode == 1 && !vtx->select)
						continue;

					const sF32 dist = vtx->pos.lenSq();
					if (dist > maxDist)
						maxDist = dist;
				}
			}
    }

    if (maxDist != 0.0f)
    {
      maxDist = sqrt(maxDist);
      invMaxDist = 1.0f / maxDist;
    }
    else
      invMaxDist = 0.0f;

    // second pass: perform plode
    for (sInt it = 0; it < data->nObjects; it++)
    {
      frObject& obj = data->objects[it];
      frMesh* msh = obj.getMesh();
      if (!msh)
        continue;

      for (sInt i = 0; i < msh->nVerts; i++)
      {
        frVertex* vtx = msh->vertices + i;

				if (mode == 1 && !vtx->select)
					continue;

        if (op == 0) // explode
          vtx->pos.addscale(vtx->norm, param);
        else if (op == 1) // implode
        {
          sF32 vecLen = vtx->pos.lenSq();

          if (vecLen) // AUTSCH. dieser op tut WEH.
          {
            vecLen = sqrt(vecLen);
            vtx->pos.scale(pow(vecLen * invMaxDist, param) * maxDist / vecLen);
          }
        }
      }

      msh->calcBoundingSphere();
      obj.updateBounds();
    }

    return sTRUE;
  }

  sBool onParamChanged(sInt param)
  {
    return param == 0; // update parameters if mode changed
  }

public:
  frGFPlode(const frPluginDef* d)
    : frGeometryPlugin(d)
  {
    addSelectParam("Mode", "Explode|Implode", 0);
    addFloatParam("Distance", 0.0f, -1e+15f, 1e+15f, 0.001f, 5);
    addFloatParam("Exponent", 1.0f, 0.0f, 8192.0f, 0.01f, 5);
    addSelectParam("Process", "All|Selection", 0);
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver=1;

    strm << ver;

    if (ver==1)
    {
      strm << params[0].selectv.sel;
      strm << params[1].floatv << params[2].floatv;
      strm << params[3].selectv.sel;
    }
  }

  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    f.putsU8(params[0].selectv.sel | (params[3].selectv.sel << 1));
    putPackedFloat(f, params[params[0].selectv.sel + 1].floatv);
  }

  sBool displayParameter(sU32 index)
  {
    switch (index)
    {
    case 0: case 3:
      return sTRUE;

    case 1: case 2:
      return index == params[0].selectv.sel + 1;
    }

    return sFALSE;
  }
};

TG2_DECLARE_PLUGIN(frGFPlode, "Plode", 1, 1, 1, sTRUE, sFALSE, 147);

// ---- make dumb normals

class frGFDumbNormals: public frGeometryPlugin
{
  sBool doProcess()
  {
    const frModel* input = getInputData(0);

    data->createObjects(input->nObjects);
    data->lightCount = 0;
    data->cloneModelLights(*input);

    const sBool useNormAlias = params[0].selectv.sel;

    // go through the objects and smoothen
    for (sInt cit = 0; cit < data->nObjects; cit++)
    {
      frObject* obj = data->objects + cit;

      data->cloneObject(input->objects[cit], cit, sTRUE);

      frMesh* msh = obj->getMesh();
      if (!msh)
        continue;

			msh->dumbNormals(useNormAlias);
    }
        
    return sTRUE;
  }

public:
  frGFDumbNormals(const frPluginDef* d)
    : frGeometryPlugin(d)
  {
    addSelectParam("Eliminate Seams", "No|Yes", 1);
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver=2;

    strm << ver;

    if (ver >= 2)
      strm << params[0].selectv;
    else
      params[0].selectv.sel = 0;
  }

  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    f.putsU8(params[0].selectv.sel);
  }
};

TG2_DECLARE_PLUGIN(frGFDumbNormals, "Dumb Normals", 1, 1, 1, sTRUE, sFALSE, 148);

// ---- displacement

static sF32 sampleTexMap(const sU16 *tmap, sInt w, sInt h, sF32 u, sF32 v)
{
  u-=sInt(u);
  v-=sInt(v);

  if (u<0)
    u+=1.0f;

  if (v<0)
    v+=1.0f;

  const sS16 *tmpp=(const sS16 *) tmap;
  
  const sInt rx=u*w, ry=v*h;
  const sF32 fx=(u*w)-rx, fy=(v*h)-ry;
  const sInt rx0=rx*4, ry0=ry*w*4, rx1=((rx+1)&(w-1))*4, ry1=((ry+1)&(h-1))*w*4;
  
  const sF32 t0=tmpp[rx0+ry0]+(tmpp[rx1+ry0]-tmpp[rx0+ry0])*fx;
  const sF32 t1=tmpp[rx0+ry1]+(tmpp[rx1+ry1]-tmpp[rx0+ry1])*fx;

  return t0+(t1-t0)*fy;
}

class frGFDisplacement: public frGeometryPlugin
{
  sBool doProcess()
  {
    const frModel* input = getInputData(0);

    data->createObjects(input->nObjects);
    data->lightCount = 0;
    data->cloneModel(*input, 0, sTRUE);

    frTexturePlugin* tplg = static_cast<frTexturePlugin*>(getInput(1));
    
    if (tplg)
    {
      frTexture* tex = tplg->getData();
      tex->lock();
      tplg->process();

      const sU16* tdata = tex->data + params[1].selectv.sel;
      const sF32 sc = params[2].floatv/32767.0f;
      const sF32 approx = params[3].floatv;
      const sF32 sharp = params[4].floatv;
      const sF32 ix = approx / tex->w, iy = approx / tex->h;
      const sF32 scSharp = (128.0f * sharp) / approx;
      const sBool doNormals = sharp != 0.0f;

      for (sInt it = 0; it < data->nObjects; it++)
      {
        frObject* obj = data->objects + it;
        frMesh* msh = obj->getMesh();
        if (!msh)
          continue;

        if (doNormals)
          msh->calcTangents();

        for (sInt i = 0; i < msh->nVerts; i++)
        {
          frVertex* vtx = msh->vertices + i;
          const sF32 h = sampleTexMap(tdata, tex->w, tex->h, vtx->uvw.x, vtx->uvw.y) * sc;
          
          vtx->pos.addscale(vtx->norm, h);

          if (doNormals)
          {
            const sF32 dx = sampleTexMap(tdata, tex->w, tex->h, vtx->uvw.x + ix, vtx->uvw.y) * sc - h;
            const sF32 dy = sampleTexMap(tdata, tex->w, tex->h, vtx->uvw.x, vtx->uvw.y + iy) * sc - h;

            fr::vector t0, t1, t2;
						t0.scale(vtx->tangent, dx * scSharp);
						t1.cross(vtx->norm, vtx->tangent);
						t1.scale(dy * scSharp);
						t2.sub(t0, t1);
            
            vtx->norm.add(t2);
            vtx->norm.norm();
          }
        }

				msh->calcBoundingSphere();
				obj->updateBounds();
      }

      tex->unlock();
    }

    return sTRUE;
  }

public:
  frGFDisplacement(const frPluginDef* d)
    : frGeometryPlugin(d, 1)
  {
    addLinkParam("Displacement map", 0, 0);
    addSelectParam("Use color channel", "Red|Green|Blue|Alpha", 1);
    addFloatParam("Strength", 1.0f, -1e+10f, 1e+10f, 0.01f, 5);
    addFloatParam("Surface approximation", 1.0f, 0.001f, 1e+10f, 0.01f, 5);
    addFloatParam("Normal sharpness", 1.0f, 0.0f, 1e+10f, 0.01f, 5);
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver = 5;

    strm << ver;

    if (ver >= 1 && ver <= 3)
    {
      fr::string tempstr;
      strm << tempstr;

      g_graph->loadRegisterLink(tempstr, 0, 0);

      strm << params[1].selectv << params[2].floatv;

      if (ver >= 2)
        strm << params[4].floatv;
      else
        params[4].floatv = 1.0f;

      if (ver >= 3)
        strm << params[3].floatv;
      else
        params[3].floatv = 1.0f;
    }

    if (ver == 4)
      fr::errorExit("Version 4 displacement? Mail the file to ryg *immediately*!");

    if (ver == 5)
    {
      strm << params[0].linkv << params[1].selectv << params[2].floatv;
      strm << params[3].floatv << params[4].floatv;
    }
  }
  
  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    f.putsU8(params[1].selectv.sel);

    putPackedFloat(f, params[2].floatv);
    putPackedFloat(f, params[3].floatv);
    putPackedFloat(f, params[4].floatv);
  }
};

TG2_DECLARE_PLUGIN(frGFDisplacement, "Displacement", 1, 1, 1, sTRUE, sFALSE, 149);

// ---- glow vertices

class frGFGlowVerts: public frGeometryPlugin
{
  frMesh* msh;
  
	sBool doProcess()
	{
    const frModel* input = getInputData(0);

    data->createObjects(1);
    data->lightCount = 0;

    sInt numV = 0;
    for (sInt it = 0; it < input->nObjects; it++)
    {
      frMesh* msh = input->objects[it].getMesh();
      if (!msh)
        continue;

      numV += msh->nVerts;
    }

    const sF32 size = params[0].floatv;
    const sU32 color = (params[2].colorv.r << 16) | (params[2].colorv.g << 8) | params[2].colorv.b | 0xff000000;

		msh->vFormat = params[1].selectv.sel;
		msh->resize(numV, 0);

    frObject* obj = data->objects;
    obj->setMesh(msh);

		frVertex* outVtx = msh->vertices;

    for (sInt it = 0; it < input->nObjects; it++)
    {
      const frObject* obj = input->objects + it;
  		const fr::matrix& mt = obj->getTransform();

      frMesh* msh = obj->getMesh();
      if (!msh)
        continue; 

	    for (sInt i = 0; i < msh->nVerts; i++)
		  {
        outVtx->pos = msh->vertices[i].pos * mt;
        outVtx->color = color;
				outVtx->uvw.set(size, size, 0);
        outVtx++;
			}
    }

    obj->updateBounds();

		return sTRUE;
	}

public:
	frGFGlowVerts(const frPluginDef* d)
    : frGeometryPlugin(d)
  {
    addFloatParam("Size", 0.015f, 0, 1e+10f, 0.01f, 5);
		addSelectParam("Vertex type", "With Normals|With Colors", 1);
    addColorParam("Color", 0xffffff);

		msh = new frMesh;
    msh->addRef();
    msh->vFormat = 1;
    msh->type = 3; // billboards
    msh->bPos.set(0, 0, 0); // empty bsphere
    msh->bRadius = 0.0f;
  }

	~frGFGlowVerts(void)
	{
		msh->release();
	}

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver = 3;

    strm << ver;

    if (ver >= 1 && ver <= 3)
    {
      strm << params[0].floatv;
      
      if (ver == 1)
      {
        fr::string temp;
        strm << temp;

        fr::debugOut("import: v1 glowVert was associated with texture '%s\n", (const sChar*) temp);
      }

      if (ver >= 2)
        strm << params[2].colorv;
      else
        params[2].colorv.r = params[2].colorv.g = params[2].colorv.b = 255;

			if (ver >= 3)
				strm << params[1].selectv;
			else
				params[1].selectv.sel = 1;
    }
  }
  
  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
		putPackedFloat(f, params[1].selectv.sel ? -params[0].floatv : params[0].floatv);

		if (params[1].selectv.sel)
		{
			f.putsU8(params[2].colorv.b);
			f.putsU8(params[2].colorv.g);
			f.putsU8(params[2].colorv.r);
		}
  }
};

TG2_DECLARE_PLUGIN(frGFGlowVerts, "GlowVert", 1, 1, 1, sTRUE, sFALSE, 150);

// ---- make ryg happy

class frGFMakeRygHappy: public frGeometryPlugin
{
  sBool doProcess()
  {
    const frModel* input = getInputData(0);

    data->createObjects(input->nObjects);
    data->lightCount = 0;
    data->cloneModel(*input, 0);

    return sTRUE;
  }

public:
  frGFMakeRygHappy(const frPluginDef* d)
    : frGeometryPlugin(d)
  {
  }

  void serialize(fr::streamWrapper& strm)
  {
    sU16 ver = 1;

    strm << ver;

    fr::debugOut("Make ryg happy is deprecated!\n");
  }

  void exportTo(fr::stream& f, const frGraphExporter& exp)
  {
  }
};

TG2_DECLARE_PLUGIN(frGFMakeRygHappy, "Mk Ryg Happy", 1, 1, 1, sTRUE, sFALSE, 153);

// ---- jitter

static sS32 jrandom(sS32 n)
{
	return (n*(n*n*15731+789221)+1376312589) & 0xffff;
}

class frGFJitter: public frGeometryPlugin
{
	sBool doProcess()
	{
		const frModel* input = getInputData(0);
		data->createObjects(input->nObjects);
		data->lightCount = 0;
		data->cloneModel(*input, 0, sTRUE);

		const sF32 amount = params[0].floatv;
		const sInt mode = params[1].selectv.sel;
		const sInt process = params[2].selectv.sel;
		sInt seed = sInt(params[3].floatv) * 3;

		for (sInt it = 0; it < data->nObjects; it++)
		{
			frMesh* msh = data->objects[it].getMesh();
			
			if (!msh)
				continue;

			for (sInt i = 0; i < msh->nVerts; i++)
			{
				frVertex* vtx = msh->vertices + i;
				const sInt posHash = ((sInt&) vtx->pos.x) + ((sInt&) vtx->pos.y) + ((sInt&) vtx->pos.z);
				fr::vector rndm;

				if (process && !(vtx->select & 1))
					continue;

				for (sInt j = 0; j < 3; j++)
					rndm.v[j] = jrandom(posHash + seed + j) / 32768.0 - 1.0f;

				if (mode != 2)
				{
					if (mode == 1) // stay on plane
						rndm.subscale(vtx->norm, rndm.dot(vtx->norm));

					vtx->pos.addscale(rndm, amount);
				}
				else // along normal
					vtx->pos.addscale(vtx->norm, rndm.x * amount);
			}
		}

		return sTRUE;
	}

public:
	frGFJitter(const frPluginDef* d)
		: frGeometryPlugin(d)
	{
		addFloatParam("Amount", 0.01f, 0.0f, 1e+15f, 0.001f, 4);
		addSelectParam("Operation", "Unconstrained|Stay on plane|Along normal", 0);
		addSelectParam("Process", "All|Selection", 0);
		addFloatParam("Seed", 1337, 0, 65535, 1, 0);
	}

	void serialize(fr::streamWrapper& strm)
	{
		sU16 ver = 1;

		strm << ver;

		if (ver == 1)
		{
			strm << params[0].floatv << params[1].selectv << params[2].selectv << params[3].floatv;
		}
	}

	void exportTo(fr::stream& f, const frGraphExporter& exp)
	{
		putPackedFloat(f, params[0].floatv);
		f.putsU8(params[1].selectv.sel | (params[2].selectv.sel << 2));
		f.putsU16(params[3].floatv);
	}
};

TG2_DECLARE_PLUGIN(frGFJitter, "Jitter", 1, 1, 1, sTRUE, sFALSE, 157);
