// This code is in the public domain. See LICENSE for details.

#include "stdafx.h"
#include "cmpbase.h"
#include "exporter.h"
#include "engine.h"
#include "directx.h"
#include "geobase.h"
#include "tstream.h"
#include "exportTool.h"
#include "frOpGraph.h"

using namespace fvs;

static sBool aboutEqual(sF32 a, sF32 b)
{
  return fabs(a - b) < 1e-4f;
}

// ---- camera

class frCGCamera: public frComposePlugin
{
  fr::matrix camera, cam2;
	frMaterial* mtrl;

	static void makeCam(fr::matrix& mtx, const fr::vector& pos, const fr::vector& val2, sInt type)
	{
		switch (type)
		{
		case 0: // pos/target
			mtx.camera(pos, val2, fr::vector(0, 1, 0));
			break;

		case 1: // pos/rot
			{
				fr::matrix temp;

				temp.rotateHPB(val2.x * 2.0f * 3.1415926535f, val2.y * 2.0f * 3.1415926535f, val2.z * 2.0f * 3.1415926535f);
				temp(3,0) = pos.x;
				temp(3,1) = pos.y;
				temp(3,2) = pos.z;
				mtx.inverse(temp);
			}
			break;
		}
	}

  sBool doProcess()
  {
    const sInt camType = params[1].selectv.sel;
		fr::vector pos(params[2].trfloatv.a, params[2].trfloatv.b, params[2].trfloatv.c);
		fr::vector val2(params[3].trfloatv.a, params[3].trfloatv.b, params[3].trfloatv.c);

		makeCam(camera, pos, val2, camType);

		if (params[9].selectv.sel) // stereo enabled?
		{
			const float dist = params[10].floatv;
			const float angle = params[11].floatv * 2 * 3.1415926535f;
			fr::vector dv;

			dv.scale(camera.getXVector(), dist * cos(angle));
			dv.addscale(camera.getYVector(), dist * sin(angle));

			makeCam(camera, pos - dv, val2, camType);
			makeCam(cam2, pos + dv, val2, camType);
		}

    return sTRUE;
  }

  void doPaint(const composeViewport& viewport, sF32 w, sF32 h)
  {
    // set & clear the viewport
    sInt x1 = viewport.xstart + fr::clamp(params[6].tfloatv.a, 0.0f, 1.0f) * viewport.w;
    sInt y1 = viewport.ystart + fr::clamp(params[6].tfloatv.b, 0.0f, 1.0f) * viewport.h; 
    sInt x2 = viewport.xstart + fr::clamp(params[7].tfloatv.a, 0.0f, 1.0f) * viewport.w;
    sInt y2 = viewport.ystart + fr::clamp(params[7].tfloatv.b, 0.0f, 1.0f) * viewport.h;
		const sF32 zoom = params[12].floatv;
		const sBool stereo = params[9].selectv.sel;

    if (x2 <= x1 || y2 <= y1)
      return;

    fvs::setViewport(x1, y1, x2-x1, y2-y1);
    if (params[8].selectv.sel)
      fvs::clear(params[8].selectv.sel);

    // get the model & lock the scene
    frGeometryPlugin *scene = (frGeometryPlugin*) getInput(1);
    if (scene)
      scene->lock();

    frModel *model = scene ? scene->getData() : 0;

    // paint the model
		if (stereo && !viewport.camOverride)
			fvs::d3ddev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED);

		const sF32 ws = zoom * w * (x2 - x1) / viewport.w;
		const sF32 hs = zoom * h * (y2 - y1) / viewport.h;

		g_engine->paintModel(model, viewport.camOverride ? *viewport.camOverride : camera, ws, hs, 0);

		if (stereo && !viewport.camOverride)
		{
			if (params[8].selectv.sel & 2)
				fvs::clear(2);

			fvs::d3ddev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
	    g_engine->paintModel(model, cam2, ws, hs, 0);
			fvs::d3ddev->SetRenderState(D3DRS_COLORWRITEENABLE, 15);
		}

    // unlock the scene
    if (scene)
      scene->unlock();

    // perform fade
    sF32 fadeLevel=params[5].floatv;

    if (fadeLevel>0.01f)
    {
      sU32 fadeColor=(params[4].colorv.r<<16)|(params[4].colorv.g<<8)|params[4].colorv.b;

      if (fadeLevel>1.0f)
        fadeLevel=1.0f;

      fadeColor|=sU32(fadeLevel*255)<<24;

      vbSlice vbs(4, sTRUE);
      stdVertex *vtx=(stdVertex *) vbs.lock();

      for (sInt i=0; i<4; i++)
      {
        vtx[i].x=(i & 1) ? conf.xRes : 0;
        vtx[i].y=(i & 2) ? conf.yRes : 0;
        vtx[i].rhw=vtx[i].z=0.5f;
        vtx[i].color=fadeColor;
      }

      vbs.unlock();

			g_engine->setMaterial(mtrl);
      fvs::drawPrimitive(D3DPT_TRIANGLESTRIP, vbs.use(), 2);
    }
  }

public:
  frCGCamera(const frPluginDef* d)
    : frComposePlugin(d, 1)
  {
    addLinkParam("Scene", 0, 1);
    addSelectParam("Type", "Position/Target|Position/Rotation", 0);
    addThreeFloatParam("Position", 0.0f, 0.0f, -5.0f, -1e+15f, 1e+15f, 0.01f, 5);
    addThreeFloatParam("Value 2", 0.0f, 0.0f, 0.0f, -1e+15f, 1e+15f, 0.01f, 5);
    addColorParam("Fade color", 0x000000);
    addFloatParam("Fade level", 0.0f, 0.0f, 1.0f, 0.01f, 3);
    addTwoFloatParam("Viewport top/left", 0.0f, 0.0f, 0.0f, 1.0f, 0.01f, 4);
    addTwoFloatParam("Viewport bottom/right", 1.0f, 1.0f, 0.0f, 1.0f, 0.01f, 4);
    addSelectParam("Clear", "none|Color only|Z only|Color+Z", 3);
		addSelectParam("Stereo mode", "Disable|Enable", 0);
		addFloatParam("Stereo eye distance", 0.2f, 0.0f, 1e+15f, 0.001f, 4);
		addFloatParam("Stereo eye angle", 0.0f, -1e+15f, 1e+15f, 0.01f, 4);
		addFloatParam("Zoom", 1.0f, 0.0f, 1e+15f, 0.01f, 3);

    params[2].animIndex = 1;
    params[3].animIndex = 2;
    params[5].animIndex = 3;
    params[6].animIndex = 4;
    params[7].animIndex = 5;
		params[10].animIndex = 6;
		params[11].animIndex = 7;
		params[12].animIndex = 8;

		mtrl = new frMaterial; // fade material
		mtrl->alphaMode = 1; // blend
		mtrl->zMode = 0; // ignore z
		mtrl->cullMode = 1; // disable cull
  }

  ~frCGCamera()
  {
		mtrl->release();
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver = 8;
    
    strm << ver;

    // old version (import)
    if (ver >= 2 && ver <= 5)
    {
      fr::string tempstr;
      strm << tempstr;
      g_graph->loadRegisterLink(tempstr, 0, 1);

      strm << params[1].selectv;
      strm << params[2].trfloatv;
      strm << params[3].trfloatv;

      if (ver >= 3)
        strm << params[4].colorv << params[5].floatv;
      else
      {
        params[4].colorv.r = params[4].colorv.g = params[4].colorv.b = 0;
        params[5].floatv = 0;
      }

      if (ver >= 4)
        strm << params[6].tfloatv << params[7].tfloatv << params[8].selectv;
      else
      {
        params[6].tfloatv.a = params[6].tfloatv.b = 0.0f;
        params[7].tfloatv.a = params[7].tfloatv.b = 1.0f;
        params[8].selectv.sel = 3;
      }

      if (params[1].selectv.sel == 2)
      {
        fr::debugOut("Camera: Position/Delta/Rotation support is out!\n");
        params[1].selectv.sel = 1;
      }

			params[9].selectv.sel = 0;
			params[10].floatv = 0.2f;
			params[11].floatv = 0.0f;
			params[12].floatv = 1.0f;
    }

    // new version (load)
    if (ver >= 6 && ver <= 8)
    {
      strm << params[0].linkv << params[1].selectv << params[2].trfloatv << params[3].trfloatv;
      strm << params[4].colorv << params[5].floatv;
      strm << params[6].tfloatv << params[7].tfloatv << params[8].selectv;

			if (ver >= 7)
			{
				strm << params[9].selectv << params[10].floatv << params[11].floatv;
			}
			else
			{
				params[9].selectv.sel = 0;
				params[10].floatv = 0.2f;
				params[11].floatv = 0.0f;
			}

			if (ver >= 8)
				strm << params[12].floatv;
			else
				params[12].floatv = 1.0f;
    }
  }
  
  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    const sBool furtherParams=!aboutEqual(params[6].tfloatv.a, 0.0f) || !aboutEqual(params[6].tfloatv.b, 0.0)
      || !aboutEqual(params[7].tfloatv.a, 1.0f) || !aboutEqual(params[7].tfloatv.b, 1.0f)
      || params[8].selectv.sel != 3 || params[9].selectv.sel != 0 ||
			params[12].floatv != 1.0f;

    f.putsU8((furtherParams ? 1 : 0) | (params[1].selectv.sel << 1)
			| (params[8].selectv.sel << 2) | (params[9].selectv.sel << 4));
    putPackedVector(f, params[2].trfloatv, sFALSE);
    putPackedVector(f, params[3].trfloatv, sFALSE);
    f.putsU8(params[4].colorv.b);
    f.putsU8(params[4].colorv.g);
    f.putsU8(params[4].colorv.r);
    putPackedFloat(f, params[5].floatv);

    if (furtherParams)
    {
      putPackedFloat(f, params[6].tfloatv.a);
      putPackedFloat(f, params[6].tfloatv.b);
      putPackedFloat(f, params[7].tfloatv.a);
      putPackedFloat(f, params[7].tfloatv.b);
			if (params[9].selectv.sel)
			{
				putPackedFloat(f, params[10].floatv);
				putPackedFloat(f, params[11].floatv);
			}

			putPackedFloat(f, params[12].floatv);
    }
  }

  void setAnim(sInt index, const sF32* vals)
  {
    if (index >= 3)
      index++;

		if (index >= 7)
			index += 2;

    index++;
    params[index].trfloatv.a = vals[0]; // noch son hack.
    params[index].trfloatv.b = vals[1];
    params[index].trfloatv.c = vals[2];

    dirty = sTRUE;
  }
};

TG2_DECLARE_PLUGIN(frCGCamera, "Camera", 2, 1, 0, sTRUE, sFALSE, 192);
