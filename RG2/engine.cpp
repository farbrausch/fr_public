// This code is in the public domain. See LICENSE for details.

// the rg2 engine
// now brand new and in an own file!

#include "stdafx.h"
#include "engine.h"
#include "geobase.h"
#include "texbase.h"
#include "directx.h"
#include "tool.h"
#include "frOpGraph.h"
#include <algorithm>

using namespace fr;
using namespace fvs;

static const sInt meshConvertThreshold=32; // number of frames after which to assume mesh is static

// ---- engine privates (texture buffering, mainly)

struct frRG2Engine::privateData
{
  static sU32     init;
  static vbSlice* vbs[2];
  static ibSlice* ibs;

  fr::matrix      invCamMat;

  // frame stats
  sU32            vertCount;
  sU32            lineCount;
  sU32            triCount;
  sU32            partCount;
  sU32            objCount;
  sU32            staticObjCount;
	sU32						mtrlChangeCount;

  struct alphaFace
  {
    sU32            zval;
    sU16            ind[3];
    sU16            _padding;
  };

  alphaFace*      alphaFaces1;
	alphaFace*			alphaFaces2;
  sU32            nAlphaFaces;

  frObject*       bSphere;
  frMaterial*     bSphereMat;

	sBool						paintWireframe;
	frRG2Engine*		owner;

	const frMaterial*	oldMaterial;

  privateData()
    : alphaFaces1(0), alphaFaces2(0), nAlphaFaces(0), convertBuffer(0), convertBufferSize(0), currentFrameCounter(1),
		oldMaterial(0)
  {
  }

  ~privateData()
  {
    clearTextureCache();

    FRSAFEDELETEA(alphaFaces1);
    FRSAFEDELETEA(alphaFaces2);
    free(convertBuffer);
  }

  struct cachedTexture
  {
    sU32              id;
    frTexturePlugin*  plg;
    sU32              rev;
    texture*          tex;
		sU32							lastUpdate;

    cachedTexture(sU32 _id)
      : id(_id), plg(0), rev(0), tex(0), lastUpdate(0)
    {
    }
  };

	struct cachedTextureLess
	{
		bool operator () (const cachedTexture& a, const sU32 b) const
		{
			return a.id < b;
		}

		bool operator () (const sU32 a, const cachedTexture& b) const
		{
			return a < b.id;
		}
	};

	typedef std::vector<cachedTexture>	texCacheVec;
	typedef texCacheVec::iterator       texCacheIt;

	texCacheVec		texCache;

  sU8*          convertBuffer;
  sU32          convertBufferSize;
  sU32          currentFrameCounter;

  sU8* getConvertBuffer(sU32 size)
  {
    if (convertBufferSize < size)
    {
      convertBuffer = (sU8*) realloc(convertBuffer, size);
      convertBufferSize = size;
    }

    return convertBuffer;
  }

  texture* getCachedTexture(sU32 id)
  {
		if (!id)
			return 0;

		texCacheIt it(std::lower_bound(texCache.begin(), texCache.end(), id, cachedTextureLess()));

		if (it == texCache.end() || it->id != id)
			it = texCache.insert(it, cachedTexture(id));

		updateCachedTexture(*it);
		return it->tex;
  }

  void updateCachedTexture(cachedTexture& tex)
  {
		if (tex.lastUpdate == currentFrameCounter)
			return;

    frOpGraph::opMapIt it = g_graph->m_ops.find(tex.id);
    if (it == g_graph->m_ops.end())
      return;

    frTexturePlugin* plg = static_cast<frTexturePlugin*>(it->second.plg);
    sU32 rev;

    if (plg)
      plg->process(1);

    rev = plg ? plg->getRevision() : 0;

    if (plg != tex.plg || rev != tex.rev)
    {
      tex.plg = plg;

      if (plg)
      {
        frTexture* texture = plg->getData();

        if (texture)
        {
          plg->process();
          texture->lock();

          if (!tex.tex || texture->w != tex.tex->getWidth() || texture->h != tex.tex->getHeight())
          {
            if (tex.tex)
              delete tex.tex;

            tex.tex = new fvs::texture(texture->w, texture->h, texfRGB | texfAlpha | texfMipMaps);
          }

          const sInt texSize = texture->w * texture->h;
          sU8* texdata = getConvertBuffer(texSize * 4);
          void* src = texture->data;

          __asm
					{
            emms;
            mov       esi, [src];
            mov       edi, [texdata];
            mov       ecx, [texSize];
            sub       ecx, 4;
lp:
            movq      mm0, [esi+ecx*8];
            movq      mm1, [esi+ecx*8+8];
            movq      mm2, [esi+ecx*8+16];
            movq      mm3, [esi+ecx*8+24];
            psrlw     mm0, 7;
            psrlw     mm1, 7;
            packuswb  mm0, mm1;
            psrlw     mm2, 7;
            psrlw     mm3, 7;
            packuswb  mm2, mm3;
            movq      [edi+ecx*4], mm0;
            movq      [edi+ecx*4+8], mm2;
            sub       ecx, 4;
            jns       lp;
            emms;
          }

          texture->unlock();
          tex.tex->upload(texdata);
        }
        else
          FRSAFEDELETE(tex.tex);

        tex.rev = plg->getRevision();
      }
      else
        FRSAFEDELETE(tex.tex);
    }

		tex.lastUpdate = currentFrameCounter;
  }

  void cleanupTextureCache()
  {
    texCacheIt it(texCache.begin());

    while (it != texCache.end())
    {
      if (it->plg)
        ++it;
      else
			{
				FRSAFEDELETE(it->tex);
        it = texCache.erase(it);
			}
    }
  }

  void clearTextureCache()
  {
		for (texCacheIt it = texCache.begin(); it != texCache.end(); ++it)
			FRSAFEDELETE(it->tex);

    texCache.clear();
  }

  void resizeAlphaFaces(sU32 newCount)
  {
    if (nAlphaFaces < newCount)
    {
      nAlphaFaces = newCount;
      FRSAFEDELETEA(alphaFaces1);
      FRSAFEDELETEA(alphaFaces2);

      alphaFaces1 = new alphaFace[nAlphaFaces];
			alphaFaces2 = new alphaFace[nAlphaFaces];
    }
  }

  void beginFrame()
  {
    if (++currentFrameCounter == 0)
      currentFrameCounter = 1;

    if ((currentFrameCounter & 127) == 0) // cleanup round... kill everything not drawn for >=129 frames
    {
      frMesh::killOldStaticBuffers(currentFrameCounter, 129);
      cleanupTextureCache();
    }
  }

  void addMappedMaterial(const frMaterial* mtrl)
  {
		if (mtrl->mapFrameCounter != currentFrameCounter)
    {
			mtrl->mapFrameCounter = currentFrameCounter;

			mtrl->texPointer[0] = (void*) getCachedTexture(mtrl->textureRef[0]);
			mtrl->texPointer[1] = (mtrl->opSt2 ? getCachedTexture(mtrl->textureRef[1])
				: 0);
    }
  }

  sU32 getCurFrame() const
  {
    return currentFrameCounter;
  }

  // engine implementation
  void      paintObject(const frObject& obj, sU32 paintFlags);
	void			sortIndices(const frObject& obj, const fr::vector modelViewZ);

  void      setupLighting(const frModel* model, const fr::matrix& cam);
	void			setupTextureStage(const frMaterial* mat, const frMaterial* oldMat, sInt stage);
  void      setMatrices(const frObject* obj);

  // init/close
  friend void initEngine();
  friend void doneEngine();
};

sU32 frRG2Engine::privateData::init = 0;
vbSlice* frRG2Engine::privateData::vbs[2];
ibSlice* frRG2Engine::privateData::ibs;

// constructor/destructor

frRG2Engine::frRG2Engine()
{
  priv = new privateData;
	priv->owner = this;
 
  priv->bSphere = new frObject;
  frMesh *mesh = priv->bSphere->createMesh();

  mesh->resize(48*3, 48*3*2);
  mesh->type = 1;
  mesh->vFormat = 1;

  frVertex* verts = mesh->vertices;
  for (sInt i = 0; i < 48; i++)
  {
    const sF32 x = cos(i*2*3.1415926535f/48.0f);
    const sF32 y = sin(i*2*3.1415926535f/48.0f);

    verts[i+ 0].pos.set(x, y, 0.0f);
    verts[i+ 0].color = 0x00ff00;
    verts[i+48].pos.set(0.0f, y, x);
    verts[i+48].color = 0x00ff00;
    verts[i+96].pos.set(x, 0.0f, y);
    verts[i+96].color = 0x00ff00;
  }

  sU16* inds = mesh->indices;
  for (sInt i = 0; i < 48; i++)
  {
    const sInt ni=(i + 1) % 48;

    inds[i*2+  0]=i;
    inds[i*2+  1]=ni;
    inds[i*2+ 96]=i + 48;
    inds[i*2+ 97]=ni + 48;
    inds[i*2+192]=i + 96;
    inds[i*2+193]=ni + 96;
  }

  priv->bSphereMat = new frMaterial;
  priv->bSphereMat->lColor = 0x00ff00;

  priv->bSphere->setMaterial(priv->bSphereMat);
  priv->bSphere->drawMesh = mesh;
	mesh->useIndices = mesh->indices;
}

frRG2Engine::~frRG2Engine()
{
  delete priv->bSphere;
  priv->bSphereMat->release();

  delete priv;
}

// ---- paint main

void frRG2Engine::paintModel(const frModel *model, const fr::matrix &cam, sF32 w, sF32 h, sU32 paintFlags)
{
  if (!model)
    return; // when there's nothing to paint, don't paint.

	priv->oldMaterial = 0;
	priv->paintWireframe = (paintFlags & frRG2Engine::pfWireframe) ? sTRUE : sFALSE;

	if (priv->paintWireframe)
	{
		fvs::d3ddev->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
		fvs::d3ddev->SetRenderState(D3DRS_TEXTUREFACTOR, 0xffffff);
		fvs::d3ddev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
		fvs::d3ddev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
	}
	else
	{
		fvs::d3ddev->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
		fvs::d3ddev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		fvs::d3ddev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_CURRENT);
	}

  // build inverse camera matrix
  priv->invCamMat.inverse(cam);

  // build projection matrix & set matrices
  const sF32 zNear = 0.1f;

  fr::matrix proj;
  proj.project(w, h, zNear, 32000.0f);

  fvs::setTransform(D3DTS_VIEW, cam);
  fvs::setTransform(D3DTS_PROJECTION, proj);

  // view cull setup
  const sF32 sx = sqrt(w * w + 1.0f);
  const sF32 sy = sqrt(h * h + 1.0f);

  // model paint setup
  frObject    *passes[17];      // 8 normal+8 sort+1 virt.

  priv->beginFrame();

  for (sInt i = 0; i < 17; i++)
    passes[i] = 0;

  if (!(paintFlags & pfWireframe))
    priv->setupLighting(model, cam);

  // fill draw lists
  for (sInt it = 0; it < model->nObjects; it++)
  {
    const frObject& curObj = model->objects[it];
    frMesh* msh = curObj.getMesh();
    frMesh* realMesh = msh;

    if (!msh)
      continue;

    if (msh->type == 3) // is particles? then use particle temporary mesh
    {
      sInt nParts = msh->nVerts;

      if (!msh->subMesh)
      {
        msh->subMesh = new frMesh;
        msh->subMesh->addRef();
        msh->subMesh->vFormat = msh->vFormat;
        msh->subMesh->resize(nParts * 4, nParts * 2);
      }

			FRASSERT(msh->subMesh->nVerts == nParts * 4);
			FRASSERT(msh->subMesh->nPrims == nParts * 2);

      // build the particle stuff (yeeess!)
      frMesh* destMesh = msh->subMesh;
      const fr::vector vecX = cam.getXVector();
      const fr::vector vecY = cam.getYVector();
      const fr::matrix& mt = curObj.getTransform();

      // vertices
      frVertex* vtx = destMesh->vertices;

      for (sInt i = 0; i < nParts; i++)
      {
        fr::vector gp = msh->vertices[i].pos * mt;
        const sF32 sizeX = msh->vertices[i].uvw.x;
        const sF32 sizeY = msh->vertices[i].uvw.y;
        const sU32 col = msh->vertices[i].color;

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
      }

      // indices (simple)
      sU16* inds = destMesh->indices;
      for (sInt i = 0; i < nParts; i++)
      {
        inds[0] = i * 4 + 0;
        inds[1] = i * 4 + 2;
        inds[2] = i * 4 + 1;
        inds[3] = i * 4 + 1;
        inds[4] = i * 4 + 2;
        inds[5] = i * 4 + 3;
        inds += 6;
      }

      destMesh->drawCount = 0;
      msh = msh->subMesh;
    }

    const_cast<frObject&>(curObj).drawMesh = msh;
		const frMaterial* mat = curObj.getMaterial();
		priv->addMappedMaterial(mat);

    if (curObj.bRadius) // don't cull objects without bSphere.
    {
      fr::vector tPos = curObj.bPos * cam; // cam space bsphere center

      if (tPos.z + curObj.bRadius < zNear)
        continue; // object is behind viewer.

      if (fabs(tPos.x * w) > curObj.bRadius * sx + tPos.z) // out on left/right
        continue;

      if (fabs(tPos.y * h) > curObj.bRadius * sy + tPos.z) // out on top/bottom
        continue;
    }

    // update stats
    priv->objCount++;
    if (msh->vbs)
      priv->staticObjCount++;

    priv->vertCount += msh->nVerts;

    if (realMesh->type == 0) // trimesh
      priv->triCount += realMesh->nPrims;
    else if (realMesh->type == 1) // linemesh
      priv->lineCount += realMesh->nPrims;
    else if (realMesh->type == 3) // particles
      priv->partCount += realMesh->nVerts;

    // calc render pass & stuff our object in the correct paintlist
    sInt objPass = curObj.getRenderPass();

    if (mat->renderPass != -1)
      objPass = mat->renderPass;

    if (mat->sortMode != 0 && !(paintFlags & pfWireframe))
    {
      objPass += 8;
      //priv->staticObjCount -= 500;

			if (!msh->indices2)
				msh->indices2 = new sU16[msh->nPrims * (3 - msh->type)];

			msh->useIndices = msh->indices2;
    }
		else
			msh->useIndices = msh->indices;

    const_cast<frObject&>(curObj).next = passes[objPass];
    passes[objPass] = &const_cast<frObject&>(curObj);
  }

  if (paintFlags & pfShowVirtObjs) // process virtual objects if requested
  {
    for (sInt it = 0; it < model->nVirtObjects; ++it)
    {
      const frObject& curObj = model->virtObjects[it];
      frMesh* msh = curObj.getMesh();
      if (!msh)
        continue;

			msh->useIndices = msh->indices;
			priv->addMappedMaterial(curObj.getMaterial());

      const_cast<frObject&>(curObj).drawMesh = msh;
      const_cast<frObject&>(curObj).next = passes[16];
      passes[16] = &const_cast<frObject&>(curObj);
    }
  }

  // then draw everything
  for (sInt i = 0; i < 8; i++)
  {
    const frObject* obj;

		// paint solid objects
    for (obj = passes[i]; obj; obj = obj->next)
    {
      priv->paintObject(*obj, paintFlags);

      if (paintFlags & pfShowCull) // show culling primitives?
      {
        fr::matrix xform;
        xform.scale(obj->bRadius, obj->bRadius, obj->bRadius);
        xform(3,0) = obj->bPos.x;
        xform(3,1) = obj->bPos.y;
        xform(3,2) = obj->bPos.z;

        priv->bSphere->setTransform(xform);
        priv->paintObject(*priv->bSphere, paintFlags);
      }
    }

		// paint transparent objects
		for (obj = passes[i + 8]; obj; obj = obj->next)
		{
      const frMesh* msh = obj->drawMesh;

			// calc modelview matrix
			fr::matrix modelView;
			modelView.mul(obj->getTransform(), cam);

			// sort then paint
			priv->sortIndices(*obj, fr::vector(modelView(0,2), modelView(1,2), modelView(2,2)));
			priv->paintObject(*obj, paintFlags);
		}
  }

  for (const frObject* obj = passes[16]; obj; obj = obj->next)
    priv->paintObject(*obj, paintFlags); // paint virt objects
}

// ---- stuff

void frRG2Engine::resetStates()
{
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
	fvs::d3ddev->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
}

// ---- stats

void frRG2Engine::clearStats()
{
  priv->vertCount = 0;
  priv->lineCount = 0;
  priv->triCount = 0;
  priv->partCount = 0;
  priv->objCount = 0;
  priv->staticObjCount = 0;
	priv->mtrlChangeCount = 0;
}

void frRG2Engine::getStats(sU32& vertCount, sU32& lineCount, sU32& triCount,
	sU32& partCount, sU32& objCount, sU32& staticObjCount, sU32& mtrlChCount)
{
  // tri/line counts are in indices, not primitives

  vertCount = priv->vertCount;
  lineCount = priv->lineCount;
  triCount = priv->triCount;
  partCount = priv->partCount;
  objCount = priv->objCount;
  staticObjCount = priv->staticObjCount;
	mtrlChCount = priv->mtrlChangeCount;
}

// ---- engine implementation

static sBool cullEnabled, lightChanged = sFALSE;

void frRG2Engine::privateData::paintObject(const frObject& obj, sU32 paintFlags)
{
  if (!obj.drawMesh)
    return;

	const frMaterial* mtrl = obj.getMaterial();

  owner->setMaterial(mtrl);
  setMatrices(&obj);

  const sU32 curFrame = getCurFrame();
  
  const frMesh* myMesh = obj.drawMesh;
  if (myMesh && myMesh->nVerts && myMesh->nPrims)
  {
		if (cullEnabled)
			fvs::d3ddev->SetRenderState(D3DRS_CULLMODE, obj.getFlipCull() ? D3DCULL_CW : D3DCULL_CCW);

    if (myMesh->type == 1 && myMesh->vFormat == 0) // linemesh
    {
			if (!paintWireframe)
			{
				fvs::d3ddev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
				fvs::d3ddev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
			}

			fvs::d3ddev->SetRenderState(D3DRS_TEXTUREFACTOR, mtrl->lColor);

			if (!paintWireframe)
			{
				fvs::d3ddev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
				fvs::d3ddev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_CURRENT);
			}
    }

    if (myMesh->vFormat == 1 || myMesh->type != 0)
		{
      fvs::d3ddev->SetRenderState(D3DRS_LIGHTING, FALSE);
			lightChanged = sTRUE;
		}

    if (myMesh->nVerts < 65536 && myMesh->nPrims < 65536)
    {
      FRASSERT(myMesh->vFormat == 0 || myMesh->vFormat == 1);
			FRASSERT(myMesh->type == 0 || myMesh->type == 1);

      vbSlice* vbslice = vbs[myMesh->vFormat];
      ibSlice* ibslice = ibs;

      if (myMesh->lastDrawFrame != curFrame)
      {
        myMesh->lastDrawFrame = curFrame;
        myMesh->drawCount++;
      }

      if (!myMesh->vbs) // no static vert buffers?
      {
        // stuff the data in dynamic buffers for now.
        vbslice->resize(myMesh->nVerts);
				myMesh->toVertexBuffer(vbslice->lock());
        vbslice->unlock();
			}
			else
				vbslice = (vbSlice*) myMesh->vbs;

			if (!myMesh->ibs || (mtrl && mtrl->sortMode))
			{
        const sU32 nInds = myMesh->nPrims * (3 - myMesh->type);

        ibslice->resize(nInds);
        memcpy(ibslice->lock(), myMesh->useIndices, sizeof(sU16) * nInds);
        ibslice->unlock();

        if (myMesh->drawCount >= meshConvertThreshold && myMesh->type == 0) // we've reached the convert threshold for meshes?
          myMesh->convertToStaticBuffers();
      }
      else
        ibslice = (ibSlice*) myMesh->ibs;

      sU32 vstart = vbslice->use();
      sU32 start = ibslice->use(vstart);

      drawIndexedPrimitive(myMesh->type ? D3DPT_LINELIST : D3DPT_TRIANGLELIST, 0, myMesh->nVerts, start, myMesh->nPrims);
      
      if ((paintFlags & frRG2Engine::pfWireframe) && myMesh->type == 0)
      {
				fvs::d3ddev->SetRenderState(D3DRS_CULLMODE, obj.getFlipCull() ? D3DCULL_CCW : D3DCULL_CW);
        fvs::d3ddev->SetRenderState(D3DRS_TEXTUREFACTOR, 0x9e9997);
        drawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, myMesh->nVerts, start, myMesh->nPrims);
				fvs::d3ddev->SetRenderState(D3DRS_TEXTUREFACTOR, 0xffffff);
      }
    }
  }
}

void frRG2Engine::privateData::sortIndices(const frObject& obj, const fr::vector modelViewZ)
{
	frMesh* msh = obj.drawMesh;
	if (!msh || msh->type != 0)
		return;

	resizeAlphaFaces(msh->nPrims);

	// prepare z sort
	frVertex* vtx = msh->vertices;
	sU16* ind = msh->indices;
	alphaFace* af = alphaFaces1;

	sU32 histogram[1024];
	memset(histogram, 0, 1024 * sizeof(sU32));

	for (sInt i = 0; i < msh->nPrims; i++, ind += 3, af++)
	{
		const sF32 transformedZ = vtx[ind[0]].pos.dot(modelViewZ) + vtx[ind[1]].pos.dot(modelViewZ) + vtx[ind[2]].pos.dot(modelViewZ);
		const sU32 zAsInt = (sU32&) transformedZ;
		const sU32 zVal = ((sS32(zAsInt) >> 31) | 0x80000000) ^ ~zAsInt;

		af->zval = zVal;
		af->ind[0] = ind[0];
		af->ind[1] = ind[1];
		af->ind[2] = ind[2];

		histogram[  0 + ((zVal >>  0) & 0xff)]++;
		histogram[256 + ((zVal >>  8) & 0xff)]++;
		histogram[512 + ((zVal >> 16) & 0xff)]++;
		histogram[768 + ((zVal >> 24) & 0xff)]++;
	}

	// sort it (4 pass radix sort)
	const sU32* currentCount = histogram;
	sU32 offset[256];

	for (sInt i = 0; i < 4; i++)
	{
		for (sU32 j = 0, cur = 0; j < 256; j++)
		{
			offset[j] = cur;
			cur += currentCount[j];
		}

		alphaFace* inputFace = alphaFaces1;
		alphaFace* const output = alphaFaces2;

		for (sU32 count = msh->nPrims; count; count--, inputFace++)
			output[offset[((const sU8*) &inputFace->zval)[i]]++] = *inputFace;

		alphaFaces2 = alphaFaces1; // swap alphaFaces1 and alphaFaces2
		alphaFaces1 = output;

		currentCount += 256;
	}

	// write the resulting index list
	sU16* idest = msh->indices2;
	const alphaFace* daf = alphaFaces1;

	for (sInt i = 0; i < msh->nPrims; i++, daf++)
	{
		*idest++ = daf->ind[0];
		*idest++ = daf->ind[1];
		*idest++ = daf->ind[2];
	}
}

void frRG2Engine::privateData::setupLighting(const frModel* mdl, const matrix& cam)
{
  if (mdl->lightCount)
  {
    sInt i;

    for (i = 0; i < mdl->lightCount; i++)
    {
      const frLight* light = mdl->lights + i;
    
      static const D3DLIGHTTYPE lightTypes[3] = {D3DLIGHT_DIRECTIONAL, D3DLIGHT_POINT, D3DLIGHT_SPOT};

      D3DLIGHT9 d3dlight;
      memset(&d3dlight, 0, sizeof(d3dlight));

      d3dlight.Type = lightTypes[light->type];
      d3dlight.Diffuse.r = light->r;
      d3dlight.Diffuse.g = light->g;
      d3dlight.Diffuse.b = light->b;
      d3dlight.Position.x = light->pos.x;
      d3dlight.Position.y = light->pos.y;
      d3dlight.Position.z = light->pos.z;
      d3dlight.Direction.x = light->dir.x;
      d3dlight.Direction.y = light->dir.y;
      d3dlight.Direction.z = light->dir.z;
      d3dlight.Range = light->range;
      d3dlight.Falloff = light->falloff;
      d3dlight.Attenuation0 = light->att[0];
      d3dlight.Attenuation1 = light->att[1];
      d3dlight.Attenuation2 = light->att[2];
      d3dlight.Theta = light->innerc;
      d3dlight.Phi = light->outerc;

      if (d3dlight.Theta > d3dlight.Phi)
        d3dlight.Theta = d3dlight.Phi;

      d3ddev->SetLight(i, &d3dlight);
      d3ddev->LightEnable(i, TRUE);
    }

    for (; i < 8; i++)
      d3ddev->LightEnable(i, FALSE);
  }
  else
  {
    sInt i;
    vector fwd = cam.getZVector();

    D3DLIGHT9 lgt;
    memset(&lgt, 0, sizeof(lgt));
    lgt.Type = D3DLIGHT_DIRECTIONAL;
    lgt.Diffuse.r = lgt.Diffuse.g = lgt.Diffuse.b = 0.95f;
    lgt.Direction.x = fwd.x;
    lgt.Direction.y = fwd.y;
    lgt.Direction.z = fwd.z;
    lgt.Range = 1e+15f;
    d3ddev->SetLight(0, &lgt);

    for (i = 0; i < 8; i++)
      d3ddev->LightEnable(i, !i);
  }

  D3DMATERIAL9 mtrl;
  memset(&mtrl, 0, sizeof(mtrl));
  mtrl.Diffuse.r = mtrl.Diffuse.g = mtrl.Diffuse.b = mtrl.Diffuse.a = 1.0f;

  d3ddev->SetMaterial(&mtrl);
}

void frRG2Engine::setMaterial(const frMaterial* mat)
{
	const frMaterial* oldMat = priv->oldMaterial;

	if (!mat)
	{
		if (oldMat)
			oldMat->release();

		oldMat = 0;
	}

	if (mat == oldMat)
		return;

	priv->mtrlChangeCount++;
	priv->setupTextureStage(mat, oldMat, 0);

	if (!oldMat || (mat->zMode & 1) != (oldMat->zMode & 1))
		fvs::d3ddev->SetRenderState(D3DRS_ZWRITEENABLE, mat->zMode & 1);

	if (!oldMat || (mat->zMode & 2) != (oldMat->zMode & 2))
		fvs::d3ddev->SetRenderState(D3DRS_ZFUNC, (mat->zMode & 2) ? D3DCMP_LESS : D3DCMP_ALWAYS);

	if (!oldMat || mat->alphaMode != oldMat->alphaMode)
	{
		static const sInt srcblends[4] = { D3DBLEND_ONE,   D3DBLEND_SRCALPHA,    D3DBLEND_ONE, D3DBLEND_DESTCOLOR };
		static const sInt dstblends[4] = { D3DBLEND_ZERO,  D3DBLEND_INVSRCALPHA, D3DBLEND_ONE, D3DBLEND_ZERO      };

		fvs::d3ddev->SetRenderState(D3DRS_ALPHABLENDENABLE, mat->alphaMode != 0);
		fvs::d3ddev->SetRenderState(D3DRS_SRCBLEND, srcblends[mat->alphaMode]);
		fvs::d3ddev->SetRenderState(D3DRS_DESTBLEND, dstblends[mat->alphaMode]);
	}

	if (!oldMat || mat->dynLightMode != oldMat->dynLightMode || lightChanged)
	{
		fvs::d3ddev->SetRenderState(D3DRS_LIGHTING, !mat->dynLightMode);
		lightChanged = sFALSE;
	}

	if (!oldMat || mat->cullMode != oldMat->cullMode)
	{
		cullEnabled = !mat->cullMode;
		if (!cullEnabled)
			fvs::d3ddev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	}

	static const sInt opModes[4] =	{ D3DTOP_DISABLE,    D3DTOP_MODULATE, D3DTOP_MODULATE2X, D3DTOP_ADD };
	static const sInt aOpModes[4] =	{ D3DTOP_SELECTARG1, D3DTOP_MODULATE, D3DTOP_MODULATE2X, D3DTOP_ADD };

	if (!oldMat || mat->opSt2 != oldMat->opSt2)
		fvs::d3ddev->SetTextureStageState(1, D3DTSS_COLOROP, opModes[mat->opSt2]);

	if (mat->opSt2)
	{
		priv->setupTextureStage(mat, oldMat, 1);

		if (!oldMat || !oldMat->alphaSt2 || mat->alphaSt2 != oldMat->alphaSt2)
			fvs::d3ddev->SetTextureStageState(1, D3DTSS_ALPHAOP, aOpModes[mat->alphaSt2]);
	}
	else
		fvs::d3ddev->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

	mat->addRef();
	if (oldMat)
		oldMat->release();

	priv->oldMaterial = mat;
}

void frRG2Engine::privateData::setupTextureStage(const frMaterial* mat, const frMaterial* oldMat, sInt stage)
{
	static const sInt addrsFlg[] = { D3DTADDRESS_WRAP, D3DTADDRESS_WRAP, D3DTADDRESS_CLAMP, D3DTADDRESS_MIRROR };
	static const sInt coordModes[] = { 0, D3DTSS_TCI_CAMERASPACENORMAL, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR, D3DTSS_TCI_CAMERASPACEPOSITION };

	if (stage == 1 && oldMat && mat->opSt2 && !oldMat->opSt2)
		oldMat = 0;

	if (!oldMat || mat->texPointer[stage] != oldMat->texPointer[stage])
		fvs::d3ddev->SetTexture(stage, *((texture*) mat->texPointer[stage]));

	if (!oldMat || mat->uAddressMode[stage] != oldMat->uAddressMode[stage])
		fvs::d3ddev->SetSamplerState(stage, D3DSAMP_ADDRESSU, addrsFlg[mat->uAddressMode[stage]]);

	if (!oldMat || mat->vAddressMode[stage] != oldMat->vAddressMode[stage])
		fvs::d3ddev->SetSamplerState(stage, D3DSAMP_ADDRESSV, addrsFlg[mat->vAddressMode[stage]]);

	if (!oldMat || mat->filterMode[stage] != oldMat->filterMode[stage])
	{
		const sInt filter = mat->filterMode[stage];
		const sInt delta = oldMat ? (oldMat->filterMode[stage] ^ filter) : -1;

		if (delta & 1)
		{
			const sInt filterVal = (filter & 1) ? D3DTEXF_LINEAR : D3DTEXF_POINT;
			fvs::d3ddev->SetSamplerState(stage, D3DSAMP_MINFILTER, filterVal);
			fvs::d3ddev->SetSamplerState(stage, D3DSAMP_MAGFILTER, filterVal);
		}

		if (delta & 2)
			fvs::d3ddev->SetSamplerState(stage, D3DSAMP_MIPFILTER, (filter & 2) ? D3DTEXF_LINEAR : D3DTEXF_NONE); 
	}

	if (!oldMat || mat->coordMode[stage] != oldMat->coordMode[stage])
	{
		const sInt coordMode = mat->coordMode[stage];
		fvs::d3ddev->SetTextureStageState(stage, D3DTSS_TEXCOORDINDEX, coordModes[coordMode]);
	}
}

void frRG2Engine::privateData::setMatrices(const frObject *obj)
{
  const frMaterial* mtrl = obj->getMaterial();
  const identityMatrix ident;
  
  const frMesh* msh = obj->getMesh();

  setTransform(D3DTS_WORLD, (msh && msh->subMesh) ? ident : obj->getTransform());

  const sInt mode = obj->getTextureMode();
  if (mode != 0 || mtrl->coordMode[0] != 0) // not direct uv?
  {
    if (mode == 1 && mtrl->coordMode[0] != 0)
      setTransform(D3DTS_TEXTURE0, obj->getTexTransform());
    else
    {
      matrix temp = obj->getTexTransform();
      temp(2,0) = temp(3,0);
      temp(2,1) = temp(3,1);
      setTransform(D3DTS_TEXTURE0, temp);
    }

		fvs::d3ddev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
  }
  else
    fvs::d3ddev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);

  if (mtrl->opSt2)
  {
    matrix texMatrix;

    switch (mtrl->coordMode[1])
    {
    case 0: // detail
      texMatrix=obj->getTexTransform();
      texMatrix(0,0)*=mtrl->dScaleX;
      texMatrix(1,0)*=mtrl->dScaleX;
      if (mtrl->coordMode[0] == 0)
        texMatrix(2,0)=texMatrix(3,0)*mtrl->dScaleX;
      else
      {
        texMatrix(2,0)*=mtrl->dScaleX;
        texMatrix(3,0)*=mtrl->dScaleX;
      }
      texMatrix(0,1)*=mtrl->dScaleY;
      texMatrix(1,1)*=mtrl->dScaleY;
      if (mtrl->coordMode[0] == 0)
        texMatrix(2,1)=texMatrix(3,1)*mtrl->dScaleY;
      else
      {
        texMatrix(2,1)*=mtrl->dScaleY;
        texMatrix(3,1)*=mtrl->dScaleY;
      }
      break;

    case 1: // envi
      texMatrix.scale(0.5f, -0.5f, 0.0f);
      texMatrix(3,0)=0.5f;
      texMatrix(3,1)=0.5f;
      break;

    case 2: // reflection
      texMatrix.identity();
      break;

    case 3: // lightmap (internal use only)
      texMatrix.mul(invCamMat, mtrl->pMatrix);
      break;
    }

    setTransform(D3DTS_TEXTURE1, texMatrix);
    fvs::d3ddev->SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
  }
}

// global interface

frRG2Engine *g_engine = 0;

void initEngine()
{
  if (!frRG2Engine::privateData::init++)
  {
    frRG2Engine::privateData::vbs[0] = new vbSlice(65535, sTRUE, D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1);
    frRG2Engine::privateData::vbs[1] = new vbSlice(65535, sTRUE, D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX2);
    frRG2Engine::privateData::ibs = new ibSlice(65535*3, sTRUE);

    g_engine = new frRG2Engine();
		g_engine->resetStates();
	}
}

void doneEngine()
{
  if (!--frRG2Engine::privateData::init)
  {
    FRSAFEDELETE(frRG2Engine::privateData::vbs[0]);
    FRSAFEDELETE(frRG2Engine::privateData::vbs[1]);
    FRSAFEDELETE(frRG2Engine::privateData::ibs);

    frMesh::killAllStaticBuffers();

    FRSAFEDELETE(g_engine);
  }
}
