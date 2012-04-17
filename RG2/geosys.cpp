// This code is in the public domain. See LICENSE for details.

#include "stdafx.h"
#include "geobase.h"
#include "debug.h"
#include "exporter.h"
#include "tstream.h"
#include "exportTool.h"
#include "frOpGraph.h"
#include <math.h>

static const sF32 PI = 3.1415926535f;

// ---- system: material

class frGSMaterial: public frGeometryPlugin
{
  frMaterial* mtrl;

  sBool onParamChanged(sInt param)
  {
    switch (param)
    {
    case 0: // texture name
      mtrl->textureRef[0] = params[0].linkv->opID;
      break;

    case 1: // u address mode
      mtrl->uAddressMode[0] = params[1].selectv.sel;
      break;

    case 2: // v address mdoe
      mtrl->vAddressMode[0] = params[2].selectv.sel;
      break;
      
    case 3: // filter mode
      mtrl->filterMode[0] = params[3].selectv.sel;
      break;

    case 4: // alpha mode
      mtrl->alphaMode = params[4].selectv.sel;
      break;

    case 5: // zbuffer mode
      mtrl->zMode = params[5].selectv.sel;
      break;

    case 6: // cull mode
      mtrl->cullMode = params[6].selectv.sel;
      break;

    case 7: // sort mode
      mtrl->sortMode = params[7].selectv.sel;
      break;

    case 8: // dynamic lighting
      mtrl->dynLightMode = params[8].selectv.sel;
      break;

    case 9: // coord mode
      mtrl->coordMode[0] = params[9].selectv.sel;
      break;

    case 10: // render pass
      mtrl->renderPass = params[10].intv;
      break;

    case 11: // stage 2 op
      // TODO: link zur stage2 texture zerstören wenn gewechselt wird!
      if (!mtrl->opSt2 != !params[11].selectv.sel)
      {
        mtrl->opSt2 = params[11].selectv.sel;
        return sTRUE;
      }
      else
        mtrl->opSt2 = params[11].selectv.sel;
      break;

    case 12: // stage 2 tex
      mtrl->textureRef[1] = params[12].linkv->opID;
      break;

    case 13: // stage 2 u address mode
      mtrl->uAddressMode[1] = params[13].selectv.sel;
      break;

    case 14: // stage 2 v address mode
      mtrl->vAddressMode[1] = params[14].selectv.sel;
      break;

    case 15: // stage 2 filter mode
      mtrl->filterMode[1] = params[15].selectv.sel;
      break;

    case 16: // stage 2 alpha op
      mtrl->alphaSt2 = params[16].selectv.sel;
      break;

    case 17: // stage 2 coord mode
      mtrl->coordMode[1] = params[17].selectv.sel;
      break;

    case 18: // stage 2 detail scale
      mtrl->dScaleX = params[18].tfloatv.a;
      mtrl->dScaleY = params[18].tfloatv.b;
      break;
    }

    return sFALSE;
  }

  sBool doProcess()
  {
    const frModel* input = getInputData(0);

    data->createObjects(input ? input->nObjects : 1);
    data->lightCount = 0;

    if (input)
    {
      data->cloneModel(*input, 0);

      for (sInt it = 0; it < data->nObjects; it++)
      {
        frObject* obj = data->objects + it;

        obj->setMaterial(mtrl);

        if (mtrl->coordMode[0] == 1) // set envmap default tex matrix
        {
          fr::matrix mtx;

          mtx.scale(0.5f, -0.5f, 0.0f);
          mtx(3,0)=0.5f;
          mtx(3,1)=0.5f;

          obj->setTexTransform(mtx); 
          obj->setTextureMode(1); // transform don't project
        }
      }
    }
    else
    {
      data->objects[0].setMaterial(mtrl); // null object with material
      data->objects[0].setMesh(0);
      data->objects[0].loadIdentity();
    }

    return sTRUE;
  }

public:
  frGSMaterial(const frPluginDef* d)
    : frGeometryPlugin(d, 2)
  {
    mtrl = new frMaterial;

    addLinkParam("Texture", mtrl->textureRef[0], 0);
    addSelectParam("U Addressing mode", "Tile|Tile+Wrap|Clamp|Mirror", mtrl->uAddressMode[0]);
    addSelectParam("V Addressing mode", "Tile|Tile+Wrap|Clamp|Mirror", mtrl->vAddressMode[0]);
    addSelectParam("Filter mode", "None|Bilinear|Mipmap|Bilinear+Mipmap", mtrl->filterMode[0]);
    addSelectParam("Alpha mode", "Opaque|Alpha Blend|Add|Multiply", mtrl->alphaMode);
    addSelectParam("ZBuffer mode", "Disable|Write Only|Read Only|Read Write", mtrl->zMode);
    addSelectParam("Cull mode", "Enable|Disable", mtrl->cullMode);
    addSelectParam("Sort mode", "Don't sort|Sort", mtrl->sortMode);
    addSelectParam("Dynamic lighting", "Enable|Disable", mtrl->dynLightMode);
    addSelectParam("Coord mode", "UV|Envmap|Reflectionmap", mtrl->coordMode[0]);
    addIntParam("Render pass", mtrl->renderPass, -1, 7, 1);
    addSelectParam("-- Stage 2", "Disable|Modulate|Modulate 2X|Add", mtrl->opSt2);
    addLinkParam("Texture", mtrl->textureRef[1], 0);
    addSelectParam("U Addressing mode", "Tile|Tile+Wrap|Clamp|Mirror", mtrl->uAddressMode[1]);
    addSelectParam("V Addressing mode", "Tile|Tile+Wrap|Clamp|Mirror", mtrl->vAddressMode[1]);
    addSelectParam("Filter mode", "None|Bilinear|Mipmap|Bilinear+Mipmap", mtrl->filterMode[1]);
    addSelectParam("Alpha mode", "Disable|Modulate|Modulate 2X|Add", mtrl->alphaSt2);
    addSelectParam("Coord mode", "Detail|Envmap|Reflectionmap", mtrl->coordMode[1]);
    addTwoFloatParam("Detail scale", mtrl->dScaleX, mtrl->dScaleY, 1.0f, 256.0f, 0.01f, 4);
  }

  ~frGSMaterial()
  {
    mtrl->release();
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver=2;

    strm << ver;

    if (ver==1 || ver==2)
      strm << *mtrl;

    params[0].linkv->opID = mtrl->textureRef[0];
    params[1].selectv.sel = mtrl->uAddressMode[0];
    params[2].selectv.sel = mtrl->vAddressMode[0];
    params[3].selectv.sel = mtrl->filterMode[0];
    params[4].selectv.sel = mtrl->alphaMode;
    params[5].selectv.sel = mtrl->zMode;
    params[6].selectv.sel = mtrl->cullMode;
    params[7].selectv.sel = mtrl->sortMode;
    params[8].selectv.sel = mtrl->dynLightMode;
    params[9].selectv.sel = mtrl->coordMode[0];
    params[10].intv = mtrl->renderPass;
    params[11].selectv.sel = mtrl->opSt2;
    params[12].linkv->opID = mtrl->textureRef[1];
    params[13].selectv.sel = mtrl->uAddressMode[1];
    params[14].selectv.sel = mtrl->vAddressMode[1];
    params[15].selectv.sel = mtrl->filterMode[1];
    params[16].selectv.sel = mtrl->alphaSt2;
    params[17].selectv.sel = mtrl->coordMode[1];
    params[18].tfloatv.a = mtrl->dScaleX;
    params[18].tfloatv.b = mtrl->dScaleY;
  }

	void exportStage(sInt stage, fr::stream& f)
	{
		f.putsU8((mtrl->uAddressMode[stage] << 0) | (mtrl->vAddressMode[stage] << 2)
			| (mtrl->filterMode[stage] << 4) | (mtrl->coordMode[stage] << 6));
	}

  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
		exportStage(0, f);
    f.putsU8((mtrl->zMode << 0) | (mtrl->alphaMode << 2) | (mtrl->sortMode << 4)
			| (mtrl->dynLightMode << 5) | (mtrl->cullMode << 6) |
			(mtrl->renderPass != -1 ? 128 : 0));
    f.putsU8((mtrl->renderPass & 7) | (mtrl->alphaSt2 << 3) | (mtrl->opSt2 << 5));

    if (mtrl->opSt2)
    {
			exportStage(1, f);

      if (mtrl->coordMode[1] == 0)
      {
        putPackedFloat(f, mtrl->dScaleX);
        putPackedFloat(f, mtrl->dScaleY - mtrl->dScaleX);
      }
    }
  }

  sInt getButtonType()
  {
    return getInID(0) ? 1 : 0;
  }

  sBool displayParameter(sU32 index)
  {
    return (index < 12) || mtrl->opSt2;
  }
};

TG2_DECLARE_PLUGIN(frGSMaterial, "Material", 1, 1, 0, sTRUE, sTRUE, 132);

// ---- system: store

class frGSStore: public frGeometryPlugin
{
  sBool doProcess()
  {
    data = getInputData(0);
    return sTRUE;
  }

public:
  frGSStore(const frPluginDef* d)
    : frGeometryPlugin(d)
  {
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver=2;

    strm << ver;

    if (ver==1)
    {
      fr::string tempstr;
      strm << tempstr;

      g_graph->loadRegisterStore(tempstr, 1);
    }
  }

  sInt getButtonType()
  {
    return 3; // store
  }
};

TG2_DECLARE_NOP_PLUGIN(frGSStore, "Store", 1, 1, 1, sFALSE, sTRUE, 138);

// ---- system: load

class frGSLoad: public frGeometryPlugin
{
  void lock()
  {
    frGeometryPlugin* in = (frGeometryPlugin*) getInput(0);
    if (in)
      in->lock();
  }

  void unlock()
  {
    frGeometryPlugin* in = (frGeometryPlugin*) getInput(0);
    if (in)
      in->unlock();
  }

  sBool doProcess()
  {
    data = getInputData(0);
    return sTRUE;
  }
  
public:
  frGSLoad(const frPluginDef* d)
    : frGeometryPlugin(d, 1)
  {
    addLinkParam("Load what", 0, 1);
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver=2;
    
    strm << ver;

    if (ver == 1)
    {
      fr::string tempstr;
      strm << tempstr;

      g_graph->loadRegisterLink(tempstr, 0, 1);
    }
    else if (ver == 2)
      strm << params[0].linkv;
  }
  
  const sChar *getDisplayName() const
  {
    frOpGraph::opMapIt it = g_graph->m_ops.find(params[0].linkv->opID);

    if (it != g_graph->m_ops.end())
      return it->second.getName();
    else
      return "(Invalid)";
  }
  
  sInt getButtonType()
  {
    return 4; // load
  }
};

TG2_DECLARE_NOP_PLUGIN(frGSLoad, "Load", 1, 0, 0, sTRUE, sTRUE, 139);

// ---- system: bridge

class frGSBridge: public frGeometryPlugin
{
  sBool doProcess()
  {
    data = getInputData(0);
    return sTRUE;
  }

public:
  frGSBridge(const frPluginDef* d)
    : frGeometryPlugin(d)
  {
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver=2;

    strm << ver;

    if (ver == 1)
    {
      fr::string tempstr;
      strm << tempstr; // die description war eh ne bescheuerte idee.
    }
  }

  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
  }
};

TG2_DECLARE_NOP_PLUGIN(frGSBridge, "Bridge", 1, 1, 1, sTRUE, sTRUE, 140);

// ---- system: load material

class frGSLoadMaterial: public frGeometryPlugin
{
  sBool doProcess()
  {
    const frModel* input = getInputData(0);

    data->createObjects(input->nObjects);
    data->lightCount = 0;
    data->cloneModel(*input, 0);

    frModel* srcData = getInputData(1);
    if (srcData)
    {
      frMaterial* mat = srcData->objects[0].getMaterial();

      for (sInt it = 0; it < data->nObjects; it++)
        data->objects[it].setMaterial(mat);
    }

    return sTRUE;
  }
  
public:
  frGSLoadMaterial(const frPluginDef* d)
    : frGeometryPlugin(d, 1)
  {
    addLinkParam("Load what", 0, 1);
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver = 2;

    strm << ver;

    if (ver == 1)
    {
      fr::string tempstr;
      strm << tempstr;

      g_graph->loadRegisterLink(tempstr, 0, 1);
    }
    else if (ver == 2)
      strm << params[0].linkv;
  }

  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
  }
};

TG2_DECLARE_PLUGIN(frGSLoadMaterial, "Load Material", 1, 1, 1, sTRUE, sTRUE, 141);

// ---- system: lighting

class frGSLighting: public frGeometryPlugin
{
  frMesh      *m_dirMesh, *m_pointMesh, *m_spotMesh;
  frMaterial  *m_lightMat;

  sBool onParamChanged(sInt param)
  {
    return param == 0; // update if type changed
  }

  sBool doProcess()
  {
    const frModel* input = getInputData(0);

    data->createObjects(input->nObjects, (input->lightCount < 8) ? ((params[0].selectv.sel == 2) ? 2 : 1) : 0);
    data->lightCount = 0;
    data->cloneModel(*input, 0);

    if (data->lightCount < 8)
    {
      frLight* lgt = data->lights + (data->lightCount++);

      lgt->type=params[0].selectv.sel;
      lgt->r = params[1].fcolorv.r;
      lgt->g = params[1].fcolorv.g;
      lgt->b = params[1].fcolorv.b;
      lgt->range = params[2].floatv;
      lgt->dir.x = params[3].trfloatv.a;
      lgt->dir.y = params[3].trfloatv.b;
      lgt->dir.z = params[3].trfloatv.c;
      lgt->pos.x = params[4].trfloatv.a;
      lgt->pos.y = params[4].trfloatv.b;
      lgt->pos.z = params[4].trfloatv.c;
      lgt->att[0] = params[5].trfloatv.a;
      lgt->att[1] = params[5].trfloatv.b;
      lgt->att[2] = params[5].trfloatv.c;
      lgt->innerc = params[6].tfloatv.a * PI;
      lgt->outerc = params[6].tfloatv.b * PI;
      lgt->falloff = params[7].floatv;

      switch (lgt->type)
      {
      case 0: // directional light
        {
          frObject *obj = data->virtObjects;
          fr::matrix temp, t2;

          temp.camera(-lgt->dir, fr::vector(0.0f, 0.0f, 0.0f), fr::vector(0.0f, 1.0f, 0.0f));
          t2.inverseAP(temp);

          obj->setMesh(m_dirMesh);
          obj->setMaterial(m_lightMat);
          obj->setTransform(t2);
        }
        break;

      case 1: // point light
        {
          frObject *obj = data->virtObjects;
          obj->setMesh(m_pointMesh);
          obj->setMaterial(m_lightMat);

          fr::matrix temp;
          temp.translate(lgt->pos);
          obj->setTransform(temp);
        }
        break;

      case 2: // spot light
        {
          fr::matrix trans;
          trans.translate(lgt->pos);

          frObject* pos = data->virtObjects;
          pos->setMesh(m_pointMesh);
          pos->setMaterial(m_lightMat);
          pos->setTransform(trans);

          sF32 ang = (params[6].tfloatv.b - 1e-3f) * PI / 2.0f;
          if (ang < 0)
            ang = 0.0f;
          
          sF32 coef = sin(ang) / cos(ang);

          fr::matrix temp, t2, t3;
          temp.camera(0, lgt->dir, fr::vector(0.0f, 1.0f, 0.0f));
          t2.inverseAP(temp);
          t3.mul(t2, trans);
          temp.scale(coef * 4.0f, coef * 4.0f, 4.0f);
          t2.mul(temp, t3);

          frObject* lspot = pos + 1;
          lspot->setMesh(m_spotMesh);
          lspot->setMaterial(m_lightMat);
          lspot->setTransform(t2);
        };
        break;
      }
    }

    return sTRUE;
  }

  void createVirtualObjects()
  {
    m_pointMesh = new frMesh;
    m_pointMesh->addRef();
    m_pointMesh->type = 1;
		m_pointMesh->resize(6, 3);
    
    frVertex* verts = m_pointMesh->vertices;
    verts[0].pos.set(-1.0f,  0.0f,  0.0f);
    verts[1].pos.set( 1.0f,  0.0f,  0.0f);
    verts[2].pos.set( 0.0f, -1.0f,  0.0f);
    verts[3].pos.set( 0.0f,  1.0f,  0.0f);
    verts[4].pos.set( 0.0f,  0.0f, -1.0f);
    verts[5].pos.set( 0.0f,  0.0f,  1.0f);

		sU16* inds = m_pointMesh->indices;
    for (sInt i = 0; i < 6; i++)
			inds[i] = i;

    m_dirMesh = new frMesh;
    m_dirMesh->addRef();
    m_dirMesh->type = 1;
    m_dirMesh->resize(6, 5);
    
    verts = m_dirMesh->vertices;
    verts[0].pos.set( 0.0f,  0.0f,  2.0f);
    verts[1].pos.set( 0.0f,  0.0f,  0.0f);
    verts[2].pos.set( 0.0f,  0.4f,  1.2f);
    verts[3].pos.set( 0.0f, -0.4f,  1.2f);
    verts[4].pos.set( 0.4f,  0.0f,  1.2f);
    verts[5].pos.set(-0.4f,  0.0f,  1.2f);

		inds = m_dirMesh->indices;
		inds[0] = 0;
		inds[1] = 1;
		for (sInt i = 2; i < 6; i++)
		{
			inds[i * 2 - 2] = 0;
			inds[i * 2 - 1] = i;
		}

    m_spotMesh = new frMesh;
    m_spotMesh->addRef();
    m_spotMesh->type = 1;
    m_spotMesh->resize(48 + 1, 48 + 4);

    verts = m_spotMesh->vertices;
    verts[0].pos.set(0.0f, 0.0f, 0.0f);
    for (sInt i = 0; i < 48; i++)
      verts[i+1].pos.set(cos(i * 2 * PI / 48.0f), sin(i * 2 * PI / 48.0f), 2.0f);

		inds = m_spotMesh->indices;
		for (sInt i = 0; i < 48; i++)
		{
			inds[i * 2 + 0] = i + 1;
			inds[i * 2 + 1] = ((i + 1) % 48) + 1;
		}

		for (sInt i = 0; i < 4; i++)
		{
			inds[i * 2 + 48 * 2 + 0] = 0;
			inds[i * 2 + 48 * 2 + 1] = 1 + i * 12;
		}

    m_lightMat = new frMaterial;
    m_lightMat->lColor = 0xffff00;
  }

public:
  frGSLighting(const frPluginDef* d)
    : frGeometryPlugin(d)
  {
    addSelectParam("Light type", "Directional|Point|Spot", 1);
    addFloatColorParam("Light color", 0.9f, 0.9f, 0.9f);
    addFloatParam("Range", 1e+9f, 0.001f, 1e+15f, 0.1f, 6);
    addThreeFloatParam("Direction", 0.0f, 0.0f, 1.0f, -1e+15f, 1e+15f, 0.01f, 6);
    addThreeFloatParam("Position", 0.0f, 3.0f, -2.5f, -1e+15f, 1e+15f, 0.01f, 6);
    addThreeFloatParam("Attenuation", 1.0f, 0.0f, 0.0f, 0.0f, 1e+15f, 0.01f, 6);
    addTwoFloatParam("Inner/Outer cone", 0.5f, 0.75f, 0.0f, 1.0f, 0.01f, 4);
    addFloatParam("Falloff", 1.0f, 0.0f, 1e+15f, 0.01f, 4);

    params[1].animIndex = 1; // color
    params[3].animIndex = 2; // dir
    params[4].animIndex = 3; // pos
    params[5].animIndex = 4; // att
    params[6].animIndex = 5; // inner/outer cone
    params[7].animIndex = 6; // falloff

    createVirtualObjects();
  }
  
  ~frGSLighting()
  {
    m_pointMesh->release();
    m_dirMesh->release();
    m_spotMesh->release();
    m_lightMat->release();
  }
  
  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver = 2;
    
    strm << ver;

    // old version (import)
    if (ver == 1)
    {
      strm << params[0].selectv;

      sU8 r, g, b;
      strm << r << g << b;

      params[1].fcolorv.r = r / 255.0f;
      params[1].fcolorv.g = g / 255.0f;
      params[1].fcolorv.b = b / 255.0f;

      strm << params[2].floatv;
      strm << params[4].trfloatv << params[3].trfloatv << params[5].trfloatv;
      strm << params[6].tfloatv << params[7].floatv;
    }
    
    // new version
    if (ver == 2)
    {
      strm << params[0].selectv;
      strm << params[1].fcolorv;
      strm << params[2].floatv;
      strm << params[4].trfloatv << params[3].trfloatv;
      strm << params[5].trfloatv;
      strm << params[6].tfloatv << params[7].floatv;
    }
  }

  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    f.putsU8(params[0].selectv.sel);

    fr3Float vcol;
    vcol.a = params[1].fcolorv.r;
    vcol.b = params[1].fcolorv.g;
    vcol.c = params[1].fcolorv.b;
    putPackedVector(f,vcol,sTRUE);
    /*putPackedFloat(f, params[1].fcolorv.r);
    putPackedFloat(f, params[1].fcolorv.g - params[1].fcolorv.r);
    putPackedFloat(f, params[1].fcolorv.b - params[1].fcolorv.r);*/
  
    if (params[0].selectv.sel != 0) // not dir?
    {
      putToterFloat(f, params[2].floatv);
      //putPackedFloat(f, params[2].floatv);
      putPackedVector(f, params[4].trfloatv);
      putPackedVector(f, params[5].trfloatv);
    }
    
    if (params[0].selectv.sel != 1) // not point?
      putPackedVector(f, params[3].trfloatv);

    if (params[0].selectv.sel == 2)
    {
      putToterFloat(f, params[6].tfloatv.a);
      putToterFloat(f, params[6].tfloatv.b);
      putToterFloat(f, params[7].floatv);

      /*putPackedFloat(f, params[6].tfloatv.a);
      putPackedFloat(f, params[6].tfloatv.b);
      putPackedFloat(f, params[7].floatv);*/
    }
  }

  sBool displayParameter(sU32 index)
  {
    sInt type = params[0].selectv.sel;

    switch (index)
    {
    case 0: case 1:
      return sTRUE;

    case 2:
      return type != 0;

    case 3:
      return type != 1;

    case 4: case 5:
      return type != 0;

    case 6: case 7:
      return type == 2;
    }

    return sFALSE;
  }

  void setAnim(sInt index, const sF32* vals)
  {
    if (index != 1) // hack.
      index++;

    params[index].trfloatv.a = vals[0];
    params[index].trfloatv.b = vals[1];
    params[index].trfloatv.c = vals[2];

    dirty = sTRUE;
  }
};

TG2_DECLARE_PLUGIN(frGSLighting, "Lighting", 1, 1, 1, sTRUE, sTRUE, 142);

// ---- render pass

class frGSRenderPass: public frGeometryPlugin
{
  sBool doProcess()
  {
    const frModel* input = getInputData(0);

    data->createObjects(input->nObjects);
    data->lightCount = 0;
    data->cloneModel(*input, 0);

    const sInt pass=params[0].intv;
    for (sInt it = 0; it < data->nObjects; it++)
      data->objects[it].setRenderPass(pass);

    return sTRUE;
  }

public:
  frGSRenderPass(const frPluginDef* d)
    : frGeometryPlugin(d)
  {
    addIntParam("Render Pass", 0, 0, 7, 1);
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver=1;

    strm << ver;

    if (ver==1)
      strm << params[0].intv;
  }

  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    f.putsU8(params[0].intv);
  }
};

TG2_DECLARE_PLUGIN(frGSRenderPass, "Render Pass", 1, 1, 1, sTRUE, sTRUE, 152);
