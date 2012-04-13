// This code is in the public domain. See LICENSE for details.

#include "types.h"
#include "math3d_2.h"
#include "opsys.h"
#include "opdata.h"
#include "geometry.h"
#include "engine.h"
#include "directx.h"

// this must match (the beginning of) frMaterial!
struct fadeMaterialDef
{
	sInt		tex0, tex1;		// textures
	sU8			flags11, flags12, flags13;
} fadeMaterial = {
	0, 0,		// no textures
	0x00,		// standard stage1 setup
	0x24,		// z disabled alpha blend no lighting cull normal
	0x00,		// default stuff no stage2
};

static void performFade(sU32 color, sF32 level)
{
  // fading code
  if (level > 0.01f)
  {
    if (level > 1.0f)
      level = 1.0f;

    color |= sU32(level * 255) << 24;

		engineSetMaterial((const frMaterial*) &fadeMaterial);
		fvs::d3ddev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    fvs::vbSlice vbs(4, sTRUE);
    fvs::stdVertex* v = (fvs::stdVertex*) vbs.lock();
    
		for (sInt i = 0; i < 4; i++)
    {
      v[i].x = (i & 1) ? fvs::viewport.xend : fvs::viewport.xstart;
      v[i].y = (i & 2) ? fvs::viewport.yend : fvs::viewport.ystart;
      v[i].z = v[i].rhw = 0.5f;
      v[i].color = color;
    }
    vbs.unlock();

    fvs::drawPrimitive(D3DPT_TRIANGLESTRIP, vbs.use(), 2);
		fvs::d3ddev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
  }
}

static void distributeBuffers(frOperator* op)
{
	if (!op)
		return;

	if (op->opType && op->def->id >= 0x80 && op->def->id < 0xc0)
	{
		if (op->opType == 1 || !op->anim)
		{
      frModel* mdl = static_cast<frGeometryOperator*>(op)->getData();

			for (sInt i = 0; i < mdl->nObjects; i++)
			{
				frMesh* msh = mdl->objects[i].getMesh();

				if (msh && !msh->vbuf && msh->type == 0) // trimesh that doesn't have a static vb yet?
				{
					static const sU32 fvfType[]={ D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1, D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX2 };

					// create buffers
					msh->vbuf = new fvs::vbSlice(msh->nVerts, sFALSE, fvfType[msh->vFormat]);
					msh->ibuf = new fvs::ibSlice(msh->nPrims * 3, sFALSE);

					// fill buffers
					msh->toVertexBuffer(msh->vbuf->lock());
					msh->vbuf->unlock();

					msh->ibuf->fillFrom(msh->indices);
				}
			}
		}
	}
	else
	{
		for (sInt i = 0; i < op->nInputs; i++)
			distributeBuffers(op->input[i]);
	}
}

static fr::vector unitY(0.0f, 1.0f, 0.0f);

static void makeCamera(fr::matrix& mat, const fr::vector& pos, const fr::vector& val2, sInt type)
{
	switch (type)
	{
	case 0: // position/target
		mat.camera(pos, val2, unitY);
		break;

	case 1: // position/rotation
		{
			fr::matrix temp;

      temp.rotateHPB(val2.x * 2 * PI, val2.y * 2 * PI, val2.z * 2 * PI);
      mat.transpose(temp);
      ((fr::vector&) mat(3,0)).transform(pos,mat);
      ((fr::vector&) mat(3,0)).scale(-1.0f);
		}
		break;
	}
}

// ---- camera

static frCompositingOperator *lastComposeOp = 0;
static sBool firstCompose;

class frOpCGCamera: public frCompositingOperator
{
  fr::matrix          camera, cam2;
  fr::vector          vals[2];
  sF32                vp[4];
  sInt                camType;
  sInt                clearMode;
  sU32                fadeCol;
  sF32                fadeLvl;
	sBool								stereoFlag;
	sF32								stereoParm[2];
	sF32								zoom;

protected:
  void doProcess()
  {
		makeCamera(camera, vals[0], vals[1], camType);

		if (stereoFlag)
		{
			const sF32 angle = stereoParm[1] * 2 * PI;
			fr::vector dv;

			dv.scale(camera.getXVector(), stereoParm[0] * cos(angle));
			dv.addscale(camera.getYVector(), stereoParm[0] * sin(angle));

			makeCamera(camera, vals[0] - dv, vals[1], camType);
			makeCamera(cam2, vals[0] + dv, vals[1], camType);
		}
  }

  void doPaint()
  {
    sInt window[4];
    for (sInt i=0; i<4; i++)
    {
      sF32 v = vp[i];
      v = (v < 0.0f) ? 0.0f : (v > 1.0f) ? 1.0f : v; // clamp it
      window[i] = v * (i & 1 ? fvs::conf.yRes : fvs::conf.xRes);
      if (i >= 2)
        window[i] -= window[i-2];
    }

    if (window[2] < 0 || window[3] < 0)
      return;

		fvs::setViewport(window[0], window[1], window[2], window[3]);
    if (!firstCompose && clearMode)
      fvs::clear(clearMode);

		if (stereoFlag)
			fvs::d3ddev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED);

		frGeometryOperator* scene = static_cast<frGeometryOperator*>(input[nInputs - 1]);
		const sF32 scaleX = zoom * window[2] / fvs::conf.xRes;
		const sF32 scaleY = zoom * 1.333f * window[3] / fvs::conf.yRes;
    enginePaint(scene, camera, scaleX, scaleY);

		if (stereoFlag)
		{
			if (clearMode & 2)
				fvs::clear(2); // zbuffer

			fvs::d3ddev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
			enginePaint(scene, cam2, scaleX, scaleY);
			fvs::d3ddev->SetRenderState(D3DRS_COLORWRITEENABLE, 0xf);
		}

    performFade(fadeCol, fadeLvl);

    firstCompose = sFALSE;    
  }

public:
  const sU8* readData(const sU8* strm)
  {
    sU8 flagByte = *strm++;

    camType = (flagByte >> 1) & 1;

    for (sInt i = 0; i < 2; i++)
      getPackedVec(vals[i], strm);

    fadeCol = *((sU32 *) strm) & 0xffffff;
    strm += 3;
    fadeLvl = getPackedFloat(strm);

    if (flagByte & 1)
    {
      for (sInt i = 0; i < 4; i++)
        vp[i] = getPackedFloat(strm);

			clearMode = (flagByte >> 2) & 3;
			stereoFlag = flagByte >> 4;

			if (stereoFlag)
			{
				for (sInt i = 0; i < 2; i++)
					stereoParm[i] = getPackedFloat(strm);
			}

			zoom = getPackedFloat(strm);
    }
    else
    {
      clearMode = 3;
      vp[0] = 0.0f;
      vp[1] = 0.0f;
      vp[2] = 1.0f;
      vp[3] = 1.0f;
			stereoFlag = sFALSE;
			zoom = 1.0f;
    }

		distributeBuffers(input[nInputs - 1]);

    return strm;
  }

  void setAnim(sInt parm, const sF32* values)
  {
    if (parm < 3)
    {
      for (sInt i = 0; i < 3; i++)
        ((sF32 *) &vals[parm - 1])[i] = values[i];
    }
    else if (parm == 3)
      fadeLvl = values[0];
    else if (parm < 6)
    {
      for (sInt i = 0; i < 2; i++)
        vp[(parm - 4) * 2 + i] = values[i];
    }
		else if (parm < 8)
			stereoParm[parm - 6] = values[0];
		else
			zoom = values[0];

    lastComposeOp = this;
  }
};

frOperator* create_CGCamera() { return new frOpCGCamera; }

// ---- main compositing hub

void cmpPaintFrame()
{
  fvs::setViewport(0, 0, fvs::conf.xRes, fvs::conf.yRes);
  fvs::clear();

  firstCompose = sTRUE;

  if (lastComposeOp)
    lastComposeOp->paint();
}
