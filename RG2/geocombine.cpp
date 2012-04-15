// This code is in the public domain. See LICENSE for details.

#include "stdafx.h"
#include "geobase.h"
#include "debug.h"
#include "tstream.h"
#include "exportTool.h"
#include <math.h>

// ---- add models

class frGCAdd: public frGeometryPlugin
{
  sBool doProcess()
  {
    sInt nObjs = 0;

    for (sInt i = 0; i < def->nInputs; i++)
    {
      const frModel* in = getInputData(i);

      if (in)
        nObjs += in->nObjects;
      else
        break;
    }

    data->lightCount = 0;
    data->createObjects(nObjs);

    nObjs = 0;
    
    for (sInt i = 0; i < def->nInputs; i++)
    {
      const frModel* in = getInputData(i);

      if (in)
      {
        data->cloneModel(*in, nObjs);
        nObjs += in->nObjects;
      }
      else
        break;
    }
      
    return sTRUE;
  }
  
public:
  frGCAdd(const frPluginDef* d)
    : frGeometryPlugin(d)
  {
  }
};

TG2_DECLARE_PLUGIN(frGCAdd, "Add", 1, 255, 2, sTRUE, sFALSE, 129);

// ---- shape animation

class frGCShapeAnim: public frGeometryPlugin
{
  sBool doProcess()
  {
    const frModel* input = getInputData(0);
    const sInt ioc = input->nObjects;
    const sF32 time = params[0].floatv;
    sInt inCount = 0;
    
    for (sInt i = 1; i < def->nInputs; i++)
    {
      const frModel* inn = getInputData(i);

      if (!inn)
        break;
      else
        inCount++;

      if (inn->nObjects != ioc)
        return sFALSE;

      for (sInt j = 0; j < ioc; j++)
      {
        const frMesh* msh1 = input->objects[j].getMesh();
        const frMesh* msh2 = inn->objects[j].getMesh();

        if ((msh1 && !msh2) || (msh2 && !msh1) || msh1->nVerts != msh2->nVerts || msh1->nPrims != msh2->nPrims)
          return sTRUE;
      }
    }

    data->lightCount = 0;
    data->createObjects(ioc);

    if (time >= 0.999f)
      data->cloneModel(*getInputData(inCount), 0, sTRUE);
    else
    {
      const sF32 timeTrans = time*inCount;
      const sInt inMesh = timeTrans;
      const sF32 weight = timeTrans - inMesh;

      data->cloneModel(*getInputData(inMesh), 0, sTRUE);
      const frModel* model2 = getInputData(inMesh + 1);
      
      for (sInt i = 0; i < data->nObjects; i++)
      {
        frMesh* msh1 = data->objects[i].getMesh();
        if (!msh1)
          continue;

        const frObject& obj2 = model2->objects[i];
        frMesh* tempmsh = 0;
        const frMesh* msh2;

        if (!obj2.hasNullTransform() || obj2.getTextureMode() != 0)
        {
          tempmsh = new frMesh(*obj2.getMesh());
          tempmsh->transform(obj2.getTransform(), obj2.getTexTransform(), obj2.getTextureMode());
          msh2 = tempmsh;
        }
        else
          msh2 = obj2.getMesh();

        for (sInt j = 0; j < msh1->nVerts; j++)
        {
          frVertex* v0 = msh1->vertices + j;
          const frVertex* v1 = msh2->vertices + j;

          v0->pos.lerp(v0->pos, v1->pos, weight);
          v0->norm.lerp(v0->norm, v1->norm, weight);
          v0->norm.norm();
					v0->tangent.lerp(v0->tangent, v1->tangent, weight);
					v0->tangent.norm();
					v0->uvw.lerp(v0->uvw, v1->uvw, weight);
        }

        const sF32 dist = (msh1->bPos - msh2->bPos).len();
        const sF32 newr = (msh1->bRadius + msh2->bRadius + dist) * 0.5f;
        const sF32 r = (newr - msh1->bRadius) / dist;
        
        msh1->bRadius = newr;
        msh1->bPos.scale(1.0f - r);
        msh1->bPos.addscale(msh2->bPos, r);
        data->objects[i].updateBounds();

        delete tempmsh;
      }
    }

    return sTRUE;
  }

public:
  frGCShapeAnim(const frPluginDef* d)
    : frGeometryPlugin(d)
  {
    addFloatParam("Time", 0.0f, 0.0f, 1.0f, 0.001f, 5);
    params[0].animIndex=1;
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver=1;

    strm << ver;

    if (ver >= 1 && ver <= 1)
    {
      strm << params[0].floatv;
    }
  }

  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    putPackedFloat(f, params[0].floatv);
  }

  void setAnim(sInt index, const sF32* vals)
  {
    if (index == 1) // time
      params[0].floatv = vals[0];

    dirty = sTRUE;
  }
};

TG2_DECLARE_PLUGIN(frGCShapeAnim, "Shape Animation", 1, 255, 2, sTRUE, sFALSE, 151);
