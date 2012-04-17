// This code is in the public domain. See LICENSE for details.

#include "types.h"
#include "rtlib.h"
#include "geometry.h"
#include "directx.h"
#include "math3d_2.h"
#include "engine.h"
#include "opsys.h"
#include "debug.h"

#pragma warning (disable: 4244 4018)

static fvs::vbSlice*	vbs[2];
static fvs::ibSlice*	ibs;
//static fr::matrix			invCamMat;

// engine internals

static void setupLighting(const frModel* mdl, const fr::vector defaultLightDir);
static void paintObject(const frObject& obj, const sU16 *indices);
static void setMatrices(const frObject* obj);

// code

void engineInit()
{
  vbs[0] = new fvs::vbSlice(49152, sTRUE, D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1);
  vbs[1] = new fvs::vbSlice(32768, sTRUE, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX2);
  ibs = new fvs::ibSlice(65535 * 3, sTRUE);

	// set default render/texture stage states
	for (sInt i = 0; i < 2; i++)
	{
		fvs::d3ddev->SetTextureStageState(i, D3DTSS_COLORARG1, D3DTA_CURRENT);
		fvs::d3ddev->SetTextureStageState(i, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
		fvs::d3ddev->SetTextureStageState(i, D3DTSS_COLORARG2, D3DTA_TEXTURE);
		fvs::d3ddev->SetTextureStageState(i, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);
	}

	fvs::d3ddev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	fvs::d3ddev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	fvs::d3ddev->SetRenderState(D3DRS_ZENABLE, TRUE);
}

void engineClose()
{
  delete vbs[0];
  delete vbs[1];
  delete ibs;
}

void enginePaint(frGeometryOperator* plg, const fr::matrix& cam, sF32 w, sF32 h)
{
  const sF32 sx = sqrt(w * w + 1.0f), sy = sqrt(h * h + 1.0f);

  fr::matrix proj;
  proj.project(w, h, 0.1f, 32000.0f);

  fvs::d3ddev->SetTransform(D3DTS_VIEW, cam.d3d());
  fvs::d3ddev->SetTransform(D3DTS_PROJECTION, proj.d3d());

  frModel* model=plg->getData();
  if (model)
  {
    frObject* passes[8];
    sInt      i;

    setupLighting(model, cam.getZVector()); // prepare lighting
    
    for (i = 0; i < 8; i++)
      passes[i] = 0;

    // prepare draw lists
    for (sInt it=0; it<model->nObjects; it++)
    {
      frObject& curObj = model->objects[it];
      frMesh *msh = curObj.getMesh();
      if (!msh)
        continue;
      
      if (msh->type == 3)
      {
        sInt nParts = msh->nVerts;

        if (!msh->subMesh)
        {
          msh->subMesh = new frMesh;
          msh->subMesh->vFormat = msh->vFormat;
          msh->subMesh->resize(nParts * 4, nParts * 2);
        }

        // build the particle stuff (this is too big, fix it!)
        frMesh* destMesh = msh->subMesh;
        const fr::vector vecX(cam(0,0), cam(1,0), cam(2,0));
        const fr::vector vecY(cam(0,1), cam(1,1), cam(2,1));
        const fr::matrix& mt = curObj.getTransform();

        // vertices
        frVertex* vtx = (frVertex*) destMesh->vertices;
        sU16 *inds = destMesh->indices;

        for (sInt i = 0; i < nParts; i++)
        {
          fr::vector gp;
					const frVertex* srcv = msh->vertices + i;
          const sF32 sizeX = srcv->uvw.x;
          const sF32 sizeY = srcv->uvw.y;
          const sU32 col = srcv->color;

          gp.transform(srcv->pos, mt);

          for (sInt j = 0; j < 4; j++)
          {
            vtx->pos = gp;
            vtx->pos.addscale(vecX, (j & 1) ? sizeX : -sizeX);
            vtx->pos.addscale(vecY, (j & 2) ? sizeY : -sizeY);
						vtx->norm.set(-cam(0,2), -cam(1,2), -cam(2,2));
            vtx->color = col;
            vtx->uvw.x = j & 1;
            vtx->uvw.y = 1 - (j >> 1);
            vtx++;
          }

          sInt j = i * 4;
          *inds++ = j + 0;
          *inds++ = j + 2;
          *inds++ = j + 1;
          *inds++ = j + 1;
          *inds++ = j + 2;
          *inds++ = j + 3;
        }

        msh = msh->subMesh;
      }
      else
      {
        fr::vector tPos;
        tPos.transform(curObj.bPos, cam); // =bounding sphere center in view space

        if (tPos.z + curObj.bRadius < 0.1f) // object behind znear?
          continue;

        if (fabs(tPos.x * w) > curObj.bRadius * sx + tPos.z) // not in left/right bounds?
          continue;

        if (fabs(tPos.y * h) > curObj.bRadius * sy + tPos.z) // not in top/bottom bounds?
          continue;
      }

      curObj.drawMesh = msh;
      
      const frMaterial* mat = curObj.getMaterial();
      sInt objPass = curObj.renderPass;

      if (mat->flags12 & 128)
        objPass = mat->flags13 & 7;

      curObj.next = passes[objPass];
      passes[objPass] = &curObj;
    }

    // draw it
    for (i = 0; i < 8; i++)
      for (const frObject* obj = passes[i]; obj; obj = obj->next)
        paintObject(*obj, obj->drawMesh->indices);
  }
}

static sBool cullDisabled, lightChanged = sFALSE;

static void paintObject(const frObject &obj, const sU16 *indices)
{
  if (!obj.drawMesh)
    return;

	const frMaterial* mtrl = obj.getMaterial();

  engineSetMaterial(mtrl);
  setMatrices(&obj);
  
  const frMesh* myMesh = obj.drawMesh;
  if (myMesh && myMesh->nVerts && myMesh->nPrims)
  {
    if (myMesh->vFormat == 1)
		{
			fvs::d3ddev->SetRenderState(D3DRS_LIGHTING, FALSE);
			lightChanged = sTRUE;
		}

		if (!cullDisabled)
			fvs::d3ddev->SetRenderState(D3DRS_CULLMODE, obj.getFlipCull() ? D3DCULL_CW : D3DCULL_CCW);

    if (myMesh->type != 0)
			fvs::d3ddev->SetTexture(0, 0);

    if (myMesh->nVerts < 65536 && myMesh->nPrims < 65536)
    {
      fvs::vbSlice*	vb = vbs[myMesh->vFormat];
      sU32          vstart, start;

      if (!myMesh->vbuf)
      {
				myMesh->toVertexBuffer(vb->lock(0, myMesh->nVerts));
				vb->unlock();
				vstart = vb->use();
			}
			else
				vstart = myMesh->vbuf->use();

			if (!myMesh->ibuf) // no static buffer
			{
        const sInt indexCount = myMesh->nPrims * (3 - myMesh->type);
				ibs->fillFrom(indices, indexCount);
    
        start = ibs->use(vstart);
      }
      else
        start = myMesh->ibuf->use(vstart);

      fvs::drawIndexedPrimitive(myMesh->type ? D3DPT_LINELIST : D3DPT_TRIANGLELIST, 0, myMesh->nVerts, start, myMesh->nPrims);
    }
  }
}

// this structure has to be compatible with frLight, atleast the member important for directional lights!

static struct defaultLightStruct
{
  sU32  type;
  sF32  r, g, b;
  sF32  posx, posy, posz;
  sF32  dirx, diry, dirz;
}
defaultLightInit=
{
  0,                    // directional
  0.95f, 0.95f, 0.95f,  // 95% grey
  0.0f, 0.0f, 0.0f,     // position
  0.0f, 0.0f, 0.0f,     // direction
};

static void setupLighting(const frModel* mdl, const fr::vector defaultLightDir)
{
  frLight* defaultLight = (frLight*) &defaultLightInit;
  defaultLight->dir = defaultLightDir;

  sInt lightCount = mdl->lightCount;
  if (!lightCount)
    lightCount++;

  sInt i;
  for (i = 0; i < lightCount; i++)
  {
    const frLight* light = mdl->lights + i;
    if (!mdl->lightCount)
      light = defaultLight;
  
    static const D3DLIGHTTYPE lightTypes[3] = { D3DLIGHT_DIRECTIONAL, D3DLIGHT_POINT, D3DLIGHT_SPOT };

    D3DLIGHT8 d3dlight;
    d3dlight.Type = lightTypes[light->type];
    memcpy(&d3dlight.Diffuse.r, &light->r, 3 * sizeof(sF32));
    memset(&d3dlight.Specular.r, 0, 3 * 2 * sizeof(sF32)); // clear specular AND ambient
    memcpy(&d3dlight.Position.x, &light->pos.x, (3 + 3 + 1 + 1 + 3 + 2) * sizeof(sF32)); // this utter hack copies the rest of the frLight struct

    if (d3dlight.Theta > d3dlight.Phi)
      d3dlight.Theta = d3dlight.Phi;

    fvs::d3ddev->SetLight(i, &d3dlight);
    fvs::d3ddev->LightEnable(i, TRUE);
  }

  while (i < 8)
    fvs::d3ddev->LightEnable(i++, FALSE);

  D3DMATERIAL8 mtrl;
  memset(&mtrl, 0, sizeof(mtrl));
  mtrl.Diffuse.r = mtrl.Diffuse.g = mtrl.Diffuse.b = mtrl.Diffuse.a = 1.0f;

  fvs::d3ddev->SetMaterial(&mtrl);
}

// this structure has to be compatible with frMaterial, atleast the first few members!

static struct defaultMaterialStruct
{
  sInt  tex1, tex2;
  sU8   flags11, flags12, flags13;
}
defaultMaterialData=
{
  0, 0, // no textures
  0x00, // standard uv no sort dynlight on
  0x03, // z readwrite no alpha uv coord no cull def render pass
  0x00, // default render pass
};

const frMaterial* defaultMaterial = (const frMaterial*) &defaultMaterialData;

static void setupTextureStage(sInt stage, sU8 flags)
{
  static const sInt addressModes[] = { D3DTADDRESS_WRAP, D3DTADDRESS_WRAP, D3DTADDRESS_CLAMP, D3DTADDRESS_MIRROR };
	static const sInt coordModes[] = { 0, D3DTSS_TCI_CAMERASPACENORMAL, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR, D3DTSS_TCI_CAMERASPACEPOSITION };

	const sInt filterVal = (flags & 16) ? D3DTEXF_LINEAR : D3DTEXF_POINT;
	fvs::d3ddev->SetTextureStageState(stage, D3DTSS_MINFILTER, filterVal);
	fvs::d3ddev->SetTextureStageState(stage, D3DTSS_MAGFILTER, filterVal);
	fvs::d3ddev->SetTextureStageState(stage, D3DTSS_MIPFILTER, (flags & 32) ? D3DTEXF_LINEAR : D3DTEXF_NONE);
	fvs::d3ddev->SetTextureStageState(stage, D3DTSS_ADDRESSU, addressModes[flags & 3]);
	fvs::d3ddev->SetTextureStageState(stage, D3DTSS_ADDRESSV, addressModes[(flags >> 2) & 3]);
	fvs::d3ddev->SetTextureStageState(stage, D3DTSS_TEXCOORDINDEX, coordModes[flags >> 6]);
}

void engineSetMaterial(const frMaterial* mat)
{
	static const frMaterial* oldMaterial = 0;
	static const sInt srcblends[4] = { D3DBLEND_ONE,   D3DBLEND_SRCALPHA,    D3DBLEND_ONE, D3DBLEND_DESTCOLOR };
	static const sInt dstblends[4] = { D3DBLEND_ZERO,  D3DBLEND_INVSRCALPHA, D3DBLEND_ONE, D3DBLEND_ZERO      };
  static const sInt opModes[4]   = { D3DTOP_DISABLE,    D3DTOP_MODULATE, D3DTOP_MODULATE2X, D3DTOP_ADD };
  static const sInt aOpModes[4]  = { D3DTOP_SELECTARG1, D3DTOP_MODULATE, D3DTOP_MODULATE2X, D3DTOP_ADD };

	if (!mat || mat == oldMaterial)
	{
		oldMaterial = mat;
		return;
	}

  mat->tex[0]->use();

	setupTextureStage(0, mat->flags11);

	fvs::d3ddev->SetRenderState(D3DRS_ZWRITEENABLE, mat->flags12 & 1);
	fvs::d3ddev->SetRenderState(D3DRS_ZFUNC, (mat->flags12 & 2) ? D3DCMP_LESS : D3DCMP_ALWAYS);
	sInt alphaMode = (mat->flags12 >> 2) & 3;
	fvs::d3ddev->SetRenderState(D3DRS_ALPHABLENDENABLE, alphaMode != 0);
	fvs::d3ddev->SetRenderState(D3DRS_SRCBLEND, srcblends[alphaMode]);
	fvs::d3ddev->SetRenderState(D3DRS_DESTBLEND, dstblends[alphaMode]);
	fvs::d3ddev->SetRenderState(D3DRS_LIGHTING, !(mat->flags12 & 32));
	lightChanged = sFALSE;
	cullDisabled = mat->flags12 & 64;
	if (cullDisabled)
		fvs::d3ddev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

  fvs::d3ddev->SetTextureStageState(1, D3DTSS_COLOROP, opModes[mat->flags13 >> 5]);

  if (mat->flags13 >> 5) // stage 2 in use?
  {
    mat->tex[1]->use(1);
		setupTextureStage(1, mat->flags21);
		fvs::d3ddev->SetTextureStageState(1, D3DTSS_ALPHAOP, aOpModes[(mat->flags13 >> 3) & 3]);
  }
  else
    fvs::d3ddev->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

	oldMaterial = mat;
}

static sBool rowLenIsOne(const fr::matrix& mtx, sInt row)
{
  const sF32 len = ((fr::vector&) mtx(row, 0)).lenSq(); // haaaaaaaack
  return (fabs(len - 1.0f) < 1e-3f);
}

void setMatrices(const frObject *obj)
{
  using namespace fvs;
  
  fr::matrix ident;
  ident.identity();

  const frMesh* msh = obj->getMesh();
  const fr::matrix& worldTransform = (msh && msh->subMesh) ? ident : obj->getTransform();
  fvs::d3ddev->SetTransform(D3DTS_WORLD, worldTransform.d3d());

  // set normalizenormals based on the world matrix
	fvs::d3ddev->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);

  const frMaterial* mtrl = obj->getMaterial();
  const sInt coordMode = mtrl->flags11 >> 6;
  const sInt mode = obj->textureMode;
	sInt trans0 = D3DTTFF_DISABLE;

  if (mode != 0 || coordMode != 0) // not direct uv? (transformed uv or texgen)
  {
    if (mode == 1 && coordMode != 0)
      fvs::d3ddev->SetTransform(D3DTS_TEXTURE0, obj->getTexTransform().d3d());
    else
    {
      fr::matrix temp = obj->getTexTransform();
      temp(2,0) = temp(3,0);
      temp(2,1) = temp(3,1);
      fvs::d3ddev->SetTransform(D3DTS_TEXTURE0, temp.d3d());
    }

		trans0 = D3DTTFF_COUNT2;
  }

	fvs::d3ddev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, trans0);

  if (mtrl->flags13 >> 5)
  {
    fr::matrix texMatrix;

    switch (mtrl->flags21 >> 6)
    {
    case 0: // detail
			{
				const fr::matrix& tform = obj->getTexTransform();

				for (int i = 0; i < 2; i++)
				{
					const sF32 scale = mtrl->dScale[i];

					for (int j = 0; j < 4; j++)
						texMatrix(j,i) = tform(j,i) * scale;

					if (coordMode == 0)
						texMatrix(2,i) = texMatrix(3,i);
				}
			}
			break;
      
    case 1: // envi
      texMatrix.scale(0.5f, -0.5f, 0.0f);
      texMatrix(3,0) = 0.5f;
      texMatrix(3,1) = 0.5f;
      break;
      
    case 2: // reflection
      texMatrix.identity();
      break;
      
/*    default: // position (internal use only)
      texMatrix.mul(invCamMat, mtrl->pMatrix);
      break;*/
    }

    fvs::d3ddev->SetTransform(D3DTS_TEXTURE1, texMatrix.d3d());
		fvs::d3ddev->SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
  }
}
