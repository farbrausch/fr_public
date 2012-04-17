// This code is in the public domain. See LICENSE for details.

#include "types.h"
#include "math3d_2.h"
#include "opsys.h"
#include "opdata.h"
#include "geometry.h"
#include "texture.h"
#include "debug.h"

// ---- tools

void matrixSRT(fr::vector* parms, fr::matrix& mtx, fr::matrix *itn=0)
{
  fr::vector rt;
  fr::matrix t1, t2;

  rt.scale(parms[1], 2 * PI);
  t1.scale(parms[0]);
  t2.rotateHPB(rt);
  mtx.mul(t1, t2);
	mtx(3,0) = parms[2].x;
	mtx(3,1) = parms[2].y;
	mtx(3,2) = parms[2].z;
  
  if(itn)
  {
    t1.scale(1.0f/parms[0].x,1.0f/parms[0].y,1.0f/parms[0].z);
    itn->mul(t1, t2);
  }
}

void matrixPivotize(fr::vector* pivot, fr::matrix& mtx)
{
  fr::matrix t1, t2;

	t2 = mtx;
  t1.translate(-pivot->x, -pivot->y, -pivot->z);
  mtx.mul(t1, t2);
	mtx(3,0) += pivot->x;
	mtx(3,1) += pivot->y;
	mtx(3,2) += pivot->z;
}

static sF32 sampleTexMap(const sU16* tmap, sInt w, sInt h, sF32 u, sF32 v)
{
  u -= sInt(u);
  v -= sInt(v);
  
  if (u < 0)
    u += 1.0f;
  
  if (v < 0)
    v += 1.0f;
  
  const sS16* tmpp = (const sS16*) tmap;
  sInt rx, ry;
  sF32 fx, fy;

  __asm // das gleiche in c++ spackt rum... DANKE, codegen!
  {
    fild    dword ptr [w];
    fmul    dword ptr [u];
    fist    dword ptr [rx];
    fisub   dword ptr [rx];
    fstp    dword ptr [fx];
    fild    dword ptr [h];
    fmul    dword ptr [v];
    fist    dword ptr [ry];
    fisub   dword ptr [ry];
    fstp    dword ptr [fy];
  }
  
  const sInt rx0 = rx * 4, ry0 = ry * w * 4, rx1 = ((rx + 1) & (w - 1)) * 4, ry1 = ((ry + 1) & (h - 1)) * w * 4;
  
  const sF32 t0 = tmpp[rx0 + ry0] + (tmpp[rx1 + ry0] - tmpp[rx0 + ry0]) * fx;
  const sF32 t1 = tmpp[rx0 + ry1] + (tmpp[rx1 + ry1] - tmpp[rx0 + ry1]) * fx;
  
  return t0 + (t1 - t0) * fy;
}

// ---- get primitive

class frOpGGPrimitive: public frGeometryOperator
{
  frObject    *object;
  frMesh      *mesh;

  sInt        m_type;
  sInt        m_tesselation[3];
  sF32        m_parm2;
  fr::vector  m_xform[3];
  sBool       m_normalsOut;

  void initNormAlias()
  {
    for (sUInt i = 0; i < mesh->nVerts; i++)
      mesh->vertices[i].normAlias = i;
  }

  void processCube()
  {
    sInt i, j, k;
    
    const sInt nverts=2*((m_tesselation[0]+1)*(m_tesselation[1]+1)+(m_tesselation[0]+1)*(m_tesselation[2]+1)+
      (m_tesselation[1]+1)*(m_tesselation[2]+1));
    const sInt nprims=4*(m_tesselation[0]*m_tesselation[1]+m_tesselation[0]*m_tesselation[2]+
      m_tesselation[1]*m_tesselation[2]);
    
    mesh->resize(nverts, nprims);
    initNormAlias();
    
    frVertex *verts=mesh->vertices, *pv=verts;
    sU16 *inds=mesh->indices, *pi=inds;
    sInt bvi=0;
    
    for (i=0; i<6; i++) // generate vertices+indices
    {
      const sInt t1=i>>1;
      const sInt t2=(t1+1)%3;
      const sInt t3=(t2+1)%3;
      const sF32 v3=(i&1)?-1.0f:1.0f;

      for (j=0; j<m_tesselation[t1]; j++)
      {
        const sInt tes2=m_tesselation[t2]+1;

        for (k=0; k<tes2-1; k++)
        {
          const sInt bi=bvi+j*tes2+k;

          pi[1]=bi+1;
          pi[4]=bi+tes2+1;

          if (i&1)
          {
            pi[0]=bi;
            pi[2]=bi+tes2;
            pi[3]=bi+1;
            pi[5]=bi+tes2;
          }
          else
          {
            pi[0]=bi+tes2;
            pi[2]=bi;
            pi[3]=bi+tes2;
            pi[5]=bi+1;
          }

          pi+=6;
        }
      }

      for (j=0; j<=m_tesselation[t1]; j++)
      {
        for (k=0; k<=m_tesselation[t2]; k++)
        {
          sF32 u = 1.0f * j / m_tesselation[t1];
          sF32 v = 1.0f * k / m_tesselation[t2];

					pv->pos.v[t1] = (u - 0.5f) * 2.0f;
					pv->pos.v[t2] = (v - 0.5f) * 2.0f;
					pv->pos.v[t3] = v3;
					pv->norm.v[t1] = 0.0f;
					pv->norm.v[t2] = 0.0f;
					pv->norm.v[t3] = v3;

					if (t1==1)
					{
						sF32 t = u;
						u = 1.0f - v;
						v = t;
					}

					pv->uvw.x = (i & 1) ? u : 1.0f - u;
					pv->uvw.y = 1.0f - v;
					pv->select = 0;

          pv++;
          bvi++;
        }
      }
    }
  }
  
  void processSphere()
  {
    const sInt rings=m_tesselation[0], sections=m_tesselation[1];
    const sInt ntris=(rings+1)*sections*2;
    const sInt nverts=(rings+1)*(sections+1)+2*sections;
    sInt i, j;
    
    mesh->resize(nverts, ntris);
    initNormAlias();
    
    frVertex *verts=mesh->vertices, *pv=verts;
    sU16 *inds=mesh->indices, *pi=inds;

    // oberster punkt
    for (i=0; i<sections; i++)
    {
      pv->pos.set(0.0f, 1.0f, 0.0f);
      pv->norm.set(0.0f, 1.0f, 0.0f);
			pv->uvw.set(1.0f - 1.0f * i / sections, 0.0f, 0.0f);
			pv->select = 0;
      pv->normAlias = 0;
      pv++;
    }

    sUInt vi=sections;

    for (i=0; i<=rings; i++)
    {
      sF32 v=1.0f*(i+1)/(rings+2), theta=v*3.1415926535f;
      sF32 y=cos(theta);
      sF32 st=sin(theta);

      for (j=0; j<=sections; j++)
      {
        const sF32 phi=2.0f*3.1415926535f*j/sections;
        const sF32 x=st*sin(phi), z=st*cos(phi);
        
        pv->pos.set(x, y, z);
        pv->norm.set(x, y, z);
				pv->uvw.set(1.0f - 1.0f * j / sections, v, 0.0f);
				pv->select = 0;
        if(j == sections)
          pv->normAlias = vi - sections;
        pv++;
        vi++;
      }
    }

    // unterster punkt
    for (i=0; i<sections; i++)
    {
      pv->pos.set(0.0f, -1.0f, 0.0f);
      pv->norm.set(0.0f, -1.0f, 0.0f);
			pv->uvw.set(1.0f - 1.0f * i / sections, 1.0f, 0.0f);
			pv->select = 0;
      pv->normAlias = vi;
      pv++;
    }

    const sInt fo=(ntris-sections)*3;
    
    // indizes
    for (i=0; i<sections; i++)
    {
      pi[0]=i;
      pi[1]=i+sections;
      pi[2]=i+sections+1;
      pi[fo+0]=nverts-1-i;
      pi[fo+1]=nverts-sections-1-i;
      pi[fo+2]=nverts-sections-2-i;
      pi+=3;
    }

    sInt m=-1;

    for (i=0; i<rings; i++)
    {
      m+=sections+1;

      for (j=0; j<sections; j++)
      {
        pi[0]=m+j;
        pi[1]=m+sections+1+j;
        pi[2]=m+sections+2+j;
        pi[3]=pi[0];
        pi[4]=pi[2];
        pi[5]=m+j+1;
        pi+=6;
      }
    }
  }
  
  void processCylinder()
  {
    const sInt sectors=m_tesselation[0], slices=m_tesselation[1];
    const sInt ntris=sectors*slices*2+2*sectors;
    const sInt nverts=(sectors+1)*(slices+3)+2;
    sInt i, j;
    
    mesh->resize(nverts, ntris);
    initNormAlias();
    
    frVertex *verts=mesh->vertices, *pv=verts;
    sU16 *inds=mesh->indices, *pi=inds;

    // -- zuerst die vertices

    // mittelpunkt oben
    pv->pos.set(0.0f, 1.0f, 0.0f);
    pv->norm.set(0.0f, 1.0f, 0.0f);
		pv->uvw.set(0.5f, 0.5f, 0.0f);
		pv->select = 0;
    pv++;

    // kappen
    for (i=0; i<=sectors; i++)
    { 
      sF32 u=1.0f*i/sectors;
      sF32 theta=u*2.0f*PI;
      sF32 st=sin(theta), ct=cos(theta);

      pv->pos.set(st, 1.0f, -ct);
      pv->norm.set(0.0f, 1.0f, 0.0f);
			pv->uvw.set(0.5f * (st + 1.0f), 0.5f * (1.0f - ct), 0.0f);
			pv->select = 0;
      pv++;
    }

    sUInt vi=sectors+2;

    // körper
    for (i=0; i<=slices; i++)
    {
      for (j=0; j<=sectors; j++)
      {
        sF32 u=1.0f*j/sectors, v=1.0f*i/slices;
        sF32 theta=u*2.0f*PI;
        sF32 st=sin(theta), ct=cos(theta);

        if (j == sectors)
          pv->normAlias = vi - sectors;

        pv->pos.set(st, (0.5f-v)*2.0f, -ct);
        pv->norm.set(st, 0, -ct);
				pv->uvw.set(u, v, 0.0f);
				pv->select = 0;
        pv++;
        vi++;
      }
    }
    
    // kappe unten
    for (i=0; i<=sectors; i++)
    {
      sF32 u=1.0f*i/sectors;
      sF32 theta=u*2.0f*PI;
      sF32 st=sin(theta), ct=cos(theta);
      
      pv->pos.set(st, -1.0f, -ct);
      pv->norm.set(0.0f, -1.0f, 0.0f);
			pv->uvw.set(0.5f * (st + 1.0f), 0.5f * (1.0f - ct), 0.0f);
			pv->select = 0;
      pv++;
    }

    // mittelpunkt unten
    pv->pos.set(0.0f, -1.0f, 0.0f);
    pv->norm.set(0.0f, -1.0f, 0.0f);
		pv->uvw.set(0.5f, 0.5f, 0.0f);
		pv->select = 0;
    pv++;
    
    // -- dann die indizes
    
    const sInt fo=(ntris-sectors)*3;
    
    // indizes caps
    for (i=0; i<sectors; i++)
    {
      pi[0]=i+2;
      pi[1]=i+1;
      pi[2]=0;
      pi[fo+0]=nverts-3-i;
      pi[fo+1]=nverts-2-i;
      pi[fo+2]=nverts-1;
      pi+=3;
    }

    sInt mnd=sectors+2;

    for (i=0; i<slices; i++)
    {
      for (j=0; j<sectors; j++)
      {
        pi[0]=mnd+sectors+2;
        pi[1]=mnd+sectors+1;
        pi[2]=mnd;
        pi[3]=mnd+1;
        pi[4]=mnd+sectors+2;
        pi[5]=mnd;
        pi+=6;
        mnd++;
      }

      mnd++;
    }
  }

  void processTorus()
  {
    const sInt rings=m_tesselation[0], sections=m_tesselation[1];
    const sF32 radius2=m_parm2;
    const sF32 rm=(radius2+1.0f)*0.5f;
    const sF32 rd=rm-radius2;   
    const sInt ntris=rings*sections*2;
    const sInt nverts=(rings+1)*(sections+1);
    sInt i, j;
    
    mesh->resize(nverts, ntris);
    initNormAlias();
    
    frVertex *verts=mesh->vertices, *pv=verts;
    sU16 *inds=mesh->indices, *pi=inds;

    // vertices
    sUInt vi=0;

    for (i=0; i<=rings; i++)
    {
      const sF32 u=1.0f*i/rings, theta=u*2*PI, st=sin(theta), ct=cos(theta);
      fr::vector pos, rgt, tmp;

      pos.set(rm*st, 0.0f, -rm*ct);
      rgt.set(-st, 0.0f, ct);
      
      for (j=0; j<=sections; j++)
      {
        const sF32 v=1.0f*j/sections, phi=v*2*PI;
        
        tmp.set(0.0f, 1.0f, 0.0f);
        tmp.spin(rgt, phi);
        
        sUInt tvi=vi;
        if (j == sections)
          tvi -= sections;

        if (i == rings)
          tvi -= rings * (sections+1);

        pv->pos.addscale(pos, tmp, rd);
        pv->norm=tmp;
				pv->uvw.set(u, v, 0.0f);
				pv->select = 0;
        pv->normAlias = tvi;
        pv++;
        vi++;
      }
    }
    
    sU16 ind=0;

    for (i=0; i<rings; i++)
    {
      for (j=0; j<sections; j++)
      {
        pi[0]=ind;
        pi[1]=ind+1;
        pi[2]=ind+sections+2;
        pi[3]=ind;
        pi[4]=ind+sections+2;
        pi[5]=ind+sections+1;
        pi+=6;
        ind++;
      }

      ind++;
    }
  }

  void processGrid()
  {
    const sInt *tes=m_tesselation;
    const sInt nverts=(tes[0]+1)*(tes[1]+1);
    const sInt ntris=2*tes[0]*tes[1];
    sInt i, j;
    
    mesh->resize(nverts, ntris);
    initNormAlias();

    frVertex *verts=mesh->vertices, *pv=verts;
    sU16 *inds=mesh->indices, *pi=inds;

    // vertices
    for (i=0; i<=tes[0]; i++)
    {
      const sF32 v=1.0f*i/tes[0];

      for (j=0; j<=tes[1]; j++)
      {
        const sF32 u=1.0f*j/tes[1];

        pv->pos.set((u-0.5f)*2.0f, (0.5f-v)*2.0f, 0.0f);
        pv->norm.set(0.0f, 0.0f, -1.0f);
				pv->uvw.set(u, v, 0.0f);
				pv->select = 0;
        pv++;
      }
    }

    // indices
    for (i=0; i<tes[0]; i++)
    {
      for (j=0; j<tes[1]; j++)
      {
        const sInt bi=i*(tes[1]+1)+j;

        pi[0]=bi;
        pi[1]=bi+1;
        pi[2]=bi+tes[1]+2;
        pi[3]=bi;
        pi[4]=bi+tes[1]+2;
        pi[5]=bi+tes[1]+1;
        pi+=6;
      }
    }
  }
  
  void initMesh()
  {
    switch (m_type)
    {
    case 0: processCube();      break;
    case 1: processSphere();    break;
    case 2: processCylinder();  break;
    case 3: processTorus();     break;
    case 4: processGrid();      break;
    }
    
    if (!m_normalsOut)
    {
      mesh->invertNormals();
      mesh->invertCulling();
    }

		memset(mesh->primSelect, 0, mesh->nPrims);

    mesh->calcBoundingSphere();
  }

  void doProcess()
  {
    fr::matrix trans, nxf;
    matrixSRT(m_xform, trans, &nxf);
    object->setTransform(trans, nxf);
  }

public:
  frOpGGPrimitive()
  {
    data->createObjects(1);
    object = data->objects;
		mesh = object->getMesh();
  }

  const sU8* readData(const sU8* strm)
  {
    sU8 typeByte = *strm++;

    m_type = typeByte & 63;
    m_normalsOut = typeByte >> 7;

    for (sInt i = 0; i < 3; i++)
      getPackedVec(m_xform[i], strm);

    m_tesselation[0] = *strm++;

    if (typeByte & 64) // tesselation uniform flag
    {
      m_tesselation[1] = m_tesselation[0];
      m_tesselation[2] = m_tesselation[0];
    }
    else
    {
      m_tesselation[1] = *strm++;
      if (m_type == 0)
        m_tesselation[2] = *strm++;
    }

    if (m_type == 3)
      m_parm2 = getPackedFloat(strm);

    opType = 1; // semi-hard
    initMesh();

    return strm;
  }

  void setAnim(sInt parm, const sF32* val)
  {
    for (sInt i = 0; i < 3; i++)
			m_xform[parm - 1].v[i] = val[i];
  }
};

frOperator* create_GGPrimitive() { return new frOpGGPrimitive; }

// ---- add models

class frOpGCAdd: public frGeometryOperator
{
  void doProcess()
  {
		sInt i, objCount;

    for (i = 0, objCount = 0; i < nInputs; i++)
      objCount += getInputData(i)->nObjects;

    data->createObjects(objCount);
    data->lightCount = 0;

    for (i = 0, objCount = 0; i < nInputs; i++)
      objCount += data->cloneModel(*getInputData(i), objCount);
  }
};

frOperator* create_GCAdd() { return new frOpGCAdd; }

// ---- transform

class frOpGFTransform: public frGeometryOperator
{
  fr::vector  m_xform[4];
  sBool       m_what;

  void doProcess()
  {
    fr::matrix xform, nxform;
    matrixSRT(m_xform, xform, &nxform);
    matrixPivotize(m_xform+3, xform);

    data->createObjects(getInputData(0)->nObjects);
    data->lightCount = 0;
    data->cloneModel(*getInputData(0), 0, m_what & 1);

    if ((m_what & 1) == 0)
    {
      for (sInt i = 0; i < data->nObjects; i++)
        data->objects[i].multiplyTransform(xform, nxform);

      data->transformLights(xform);
    }
    else
    {
      for (sInt i = 0; i < data->nObjects; i++)
      {
        frObject *obj = data->objects + i;
        frMesh *msh = obj->getMesh();

				for (sInt j = 0; j < msh->nVerts; j++)
        {
					frVertex* vtx = msh->vertices + j;

					if (vtx->select)
          {
						vtx->pos.transform(xform);
						vtx->norm.transform(nxform);
						vtx->norm.norm();
          }
        }

        msh->calcBoundingSphere();
        obj->updateBounds();
      }
    }
  }

  void setAnim(sInt parm, const sF32* val)
  {
    for (sInt i = 0; i < 3; i++)
			m_xform[parm - 1].v[i] = val[i];
  }

public:
  const sU8* readData(const sU8* strm)
  {
    for (sInt i=0; i<3; i++)
      getPackedVec(m_xform[i], strm);

    m_what = *strm++;
    opType = (m_what & 1) ? 2 : 0;

    if (m_what & 2)
      getPackedVec(m_xform[3], strm);
    else
      m_xform[3].set(0.0f, 0.0f, 0.0f);

    return strm;
  }
};

frOperator* create_GFTransform() { return new frOpGFTransform; }

// ---- clone+transform

class frOpGFCloneXForm: public frGeometryOperator
{
  fr::vector  m_xform[3];
  sInt        m_count;

  void doProcess()
  {
    const frModel *input = getInputData(0);
    const sInt nObj = input->nObjects;

    fr::matrix xform, cxform, nform, cnform;
    matrixSRT(m_xform, xform, &nform);

    data->createObjects(nObj * m_count);
    cxform.identity();
    cnform.identity();

    sInt curInd = 0, count = m_count;

    while (count--)
    {
      data->lightCount = 0;
      data->cloneModel(*input, curInd);
      for (sInt i = curInd; i < curInd + nObj; i++)
        data->objects[i].multiplyTransform(cxform, cnform);

      fr::matrix temp = cxform;
      cxform.mul(temp, xform);
      temp = cnform;
      cnform.mul(temp, nform);

      curInd += nObj;
    }

    data->lightCount = 0;
    data->cloneModelLights(*input);
  }

  void setAnim(sInt parm, const sF32* val)
  {
    for (sInt i = 0; i < 3; i++)
			m_xform[parm - 1].v[i] = val[i];
  }

public:
  const sU8* readData(const sU8* strm)
  {
    for (sInt i = 0; i < 3; i++)
      getPackedVec(m_xform[i], strm);

    sU8 countByte = *strm++;
    if (countByte<255)
      m_count=countByte;
    else
    {
      m_count=*((sU16 *) strm);
      strm+=2;
    }

    return strm;
  }
};

frOperator *create_GFCloneXForm() { return new frOpGFCloneXForm; }

// ---- material

class frOpGSMaterial: public frGeometryOperator
{
  frMaterial*         mtrl;

  void doProcess()
  {
    // update textures
    for (sInt num = 0; num < 2; num++)
    {
      frTextureOperator* texOperator = static_cast<frTextureOperator*>(input[nInputs - 2 + num]);
      if (!texOperator)
        continue;

      texOperator->updateTexture();
      mtrl->tex[num] = texOperator->getTexture();
    }

    data->createObjects((nInputs > 2) ? getInputData(0)->nObjects : 1);
    data->lightCount = 0;
      
    if (nInputs > 2)
      data->cloneModel(*getInputData(0), 0, sFALSE);

    const sInt coordMode = mtrl->flags11 >> 6;
    
    for (sInt i = 0; i < data->nObjects; i++)
    {
      data->objects[i].setMaterial(mtrl);

      if (coordMode == 1) // envmap
      {
        fr::matrix mtx;
        
        mtx.scale(0.5f, -0.5f, 0.0f);
        mtx(3,0)=0.5f;
        mtx(3,1)=0.5f;
        
        data->objects[i].setTexTransform(mtx); 
        data->objects[i].textureMode = 1; // transform don't project
      }
    }
  }

public:
  frOpGSMaterial()
  {
    mtrl = new frMaterial;
  }

  void freeData()
  {
    if (mtrl)
    {
      mtrl->release();
      mtrl = 0;
    }

    frGeometryOperator::freeData();
  }

  const sU8* readData(const sU8 *strm)
  {
    mtrl->flags11 = strm[0];
    mtrl->flags12 = strm[1];
    mtrl->flags13 = strm[2];
    strm += 3;

    if (mtrl->flags13 >> 5)
    {
      mtrl->flags21 = *strm++;

      if ((mtrl->flags21 >> 6) == 0) // uses detail textures
      {
        mtrl->dScale[0] = getPackedFloat(strm);
        mtrl->dScale[1] = getPackedFloat(strm) + mtrl->dScale[0];
      }
    }

    return strm;
  }
};

frOperator* create_GSMaterial() { return new frOpGSMaterial; }

// ---- select

class frOpGFSelect: public frGeometryOperator
{
  sInt        m_mode;
  sInt        m_selShape;
  fr::vector  m_pos, m_size;
  sInt        m_selMode;

  void doProcess()
  {
    const frModel *input = getInputData(0);

    data->createObjects(input->nObjects);
    data->lightCount = 0;
    data->cloneModelLights(*input);

    for (sInt i = 0; i < input->nObjects; i++)
    {
      frObject* inObj = input->objects + i;
      frObject* obj = data->cloneObject(*inObj, i, sTRUE);
			frMesh* msh = obj->getMesh();

      switch (m_mode)
      {
			case 0: // select all
			case 1: // select none
			case 2: // invert selection
				{
					sU32 andMask = m_mode == 2, xorMask = m_mode != 1;

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
					fr::vector sizeTemp;

					// prepare select
					for (sInt i = 0; i < 3; i++)
						sizeTemp.v[i] = m_size.v[i] ? 1.0f / m_size.v[i] : 0.0f;

					// perform select
					for (sInt i = 0; i < msh->nVerts; i++)
					{
						frVertex* vtx = msh->vertices + i;
						sU32 msk = 3;
						sF32 d = 0.0f;

						for (sInt j = 0; j < 3; j++)
						{
							sF32 v = (vtx->pos.v[j] - m_pos.v[j]) * sizeTemp.v[j];
							if (fabs(v) >= 1.0f)
								msk = 0;

							d += v * v;
						}

						if (d >= 1.0f && m_selShape)
							msk = 0;

						switch (m_mode)
						{
						case 3:	vtx->select = msk; break;
						case 4: vtx->select |= msk; break;
						case 5: vtx->select &= ~msk; break;
						}
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

						if (m_selMode)
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
  }

public:
  const sU8* readData(const sU8* strm)
  {
    sU8 stateByte = *strm++;

    m_mode = stateByte & 7;
    m_selShape = (stateByte >> 3) & 1;
    m_selMode = (stateByte >> 4) & 1;

    if (m_mode >= 3)
    {
      getPackedVec(m_pos, strm);
      getPackedVec(m_size, strm);
    }

    opType = 2; // hard

    return strm;
  }
};

frOperator* create_GFSelect() { return new frOpGFSelect; }

// ---- delete selection

class frOpGFDeleteSelection: public frGeometryOperator
{
  void doProcess()
  {
    const frModel *input = getInputData(0);

    data->createObjects(input->nObjects);
    data->lightCount = 0;
    data->cloneModelLights(*input);

    for (sInt i = 0; i < input->nObjects; i++)
    {
      const frObject* inObj = input->objects + i;
			const frMesh* inMesh = inObj->getMesh();
      frObject* obj = data->cloneObject(*inObj, i, sTRUE);

      // count faces in selection
      const sInt faceCount = inMesh->nPrims;
      sInt selFaceCount = 0;

      for (sInt i = 0; i < faceCount; i++)
				selFaceCount += inMesh->primSelect[i] ? 1 : 0;

			// count vertices in selection
      const sInt vertCount = inMesh->nVerts;
      sInt selVertCount = 0;

      for (sInt i = 0; i < vertCount; i++)
      {
				if (inMesh->vertices[i].select == 3)
					selVertCount++;
      }

      frMesh* msh = obj->getMesh();

      msh->resize(vertCount - selVertCount, faceCount - selFaceCount);
      sU16* vertRemap = new sU16[vertCount];

      // build new vertex list
      sU32 vertPos = 0;
      for (sInt i = 0; i < vertCount; i++)
      {
        vertRemap[i] = vertPos;

				if (inMesh->vertices[i].select != 3)
        {
          msh->vertices[vertPos] = inMesh->vertices[i];
          msh->vertices[vertPos].normAlias = vertRemap[msh->vertices[vertPos].normAlias];
          vertPos++;
        }
      }

      // build new face list
      sU32 facePos = 0;
      for (sInt i = 0; i < faceCount; i++)
      {
				if (inMesh->primSelect[i])
					continue;

        for (sInt j = 0; j < 3; j++)
          msh->indices[facePos++] = vertRemap[inMesh->indices[i * 3 + j]];
      }

      delete[] vertRemap;

      msh->calcBoundingSphere();
      obj->updateBounds();
    }
  }

public:
  const sU8* readData(const sU8* strm)
  {
    opType = 2; // hard
    return strm;
  }
};

frOperator* create_GFDeleteSelection() { return new frOpGFDeleteSelection; }

// ---- bend & plode support

static void bendPlodeFirstPass(frModel* data, const frModel* input, sBool useSel, sF32 &minY, sF32 &maxY, sF32 &maxDist)
{
  sBool firstSet = sFALSE;

  data->createObjects(input->nObjects);
  data->lightCount = 0;
  maxDist = 0.0f;

  // first pass over data: hard clone and find minima/maxima

  for (sInt j = 0; j < input->nObjects; j++)
  {
    frObject* obj = data->cloneObject(input->objects[j], j, sTRUE);
    const frMesh *msh = obj->getMesh();

    for (sInt i = 0; i < msh->nVerts; i++)
    {
			frVertex* vtx = msh->vertices + i;

      if (useSel && !vtx->select)
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

      const sF32 dist = vtx->pos.lenSq();
      if (dist > maxDist)
        maxDist = dist;
    }
  }
}

// ---- bend

class frOpGFBend: public frGeometryOperator
{
  fr::vector  m_xform[6];
  sInt        m_weight, m_mode, m_blend;
  
  void doProcess()
  {
    const frModel* input = getInputData(0);
    sF32 minY, maxY, md;

    bendPlodeFirstPass(data, input, m_mode == 1, minY, maxY, md);

    // second pass: perform bend
    fr::matrix xform1, xform2, xfon1, xfon2;
    if (!m_blend)
    {
      matrixSRT(m_xform + 0, xform1, &xfon1);
      matrixSRT(m_xform + 3, xform2, &xfon2);
    }

    sF32 deltaY = minY - maxY;
    if (deltaY)
      deltaY = 1.0f / deltaY;
    
    for (sInt j = 0; j < data->nObjects; j++)
    {
      frObject& obj = data->objects[j];
      frMesh* msh = obj.getMesh();
      
      for (sInt i = 0; i < msh->nVerts; i++)
      {
				frVertex* vtx = msh->vertices + i;

        if (m_mode == 1 && !vtx->select)
          continue;
                
        sF32 weight = (vtx->pos.y - maxY) * deltaY;
        if (m_weight == 1) // fake-spline weighting?
          weight = weight * weight * (3.0f - 2.0f * weight);

        if (!m_blend)
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
          fr::vector t[3];

          for (sInt j = 0; j < 3; j++)
            t[j].lerp(m_xform[j], m_xform[j + 3], weight);

          matrixSRT(t, xform1, &xfon1);
          vtx->pos *= xform1;
          vtx->norm *= xfon1;
          vtx->norm.norm();
        }
      }
      
      msh->calcBoundingSphere();
      obj.updateBounds();
    }
    
    data->cloneModelLights(*input);
  }
  
  void setAnim(sInt parm, const sF32* val)
  {
    for (sInt i = 0; i < 3; i++)
			m_xform[parm - 1].v[i] = val[i];
  }
  
public:
  const sU8* readData(const sU8* strm)
  {
    for (sInt i = 0; i < 6; i++)
      getPackedVec(m_xform[i], strm);

    sU8 opByte = *strm++;

    m_weight = opByte & 1;
    m_mode = (opByte >> 1) & 1;
    m_blend = opByte >> 2;

    opType = 2; // hard
    return strm;
  }
};

frOperator* create_GFBend() { return new frOpGFBend; }

// ---- uv mapping

class frOpGFUVMap: public frGeometryOperator
{
  fr::vector  m_xform[3];
  sInt        m_mode, m_which;

  void doProcess()
  {
    fr::matrix xform, nxform;
    matrixSRT(m_xform, xform, &nxform);

    const frModel* input = getInputData(0);

    data->createObjects(input->nObjects);
    data->lightCount = 0;
    data->cloneModel(*input, 0, opType);

    for (sInt it = 0; it < data->nObjects; it++)
    {
      frObject* inObj = data->objects + it;

      if (m_mode != 1)
      {
        frMesh* msh = inObj->getMesh();
        
        for (sU32 i = 0; i < msh->nVerts; i++)
        {
					frVertex* vt = msh->vertices + i;
					if (m_which && !vt->select)
            continue;
          
          if (m_mode == 0)
						vt->uvw.transform(vt->pos, xform);
          else
          {
            sInt axis = 0;
            sF32 dom = 0.0f;

            fr::vector norm;
            norm.transform(vt->norm, nxform);

            for (sInt i = 0; i < 3; i++)
            {
							const sF32 v = fabs(norm.v[i]);

              if (v > dom)
              {
                axis = i;
                dom = v;
              }
            }

            fr::vector tmp;
            tmp.transform(vt->pos, xform);

            vt->uvw.x = (axis == 0) ? tmp.z : tmp.x;
            vt->uvw.y = (axis == 1) ? tmp.z : tmp.y;
          }
        }
      }
      else
      {
        if (m_which == 0) // all points?
          inObj->multiplyTexTransform(xform); // then use matrix ops
        else // else transform them seperately
        {
          frMesh* msh = inObj->getMesh();
          
          for (sU32 i = 0; i < msh->nVerts; i++)
          {
						frVertex* vt = msh->vertices + i;
						if (vt->select)
							vt->uvw.transform(xform);
          }
        }
      }
    }
  }
  
  void setAnim(sInt parm, const sF32* val)
  {
    for (sInt i = 0; i < 3; i++)
			m_xform[parm - 1].v[i] = val[i];
  }
  
public:
  const sU8* readData(const sU8* strm)
  {
    sU8 modeByte = *strm++;

    m_mode = modeByte & 3;
    m_which = modeByte >> 2;

    for (sInt i = 0; i < 3; i++)
      getPackedVec(m_xform[i], strm);

    opType = (m_mode != 1 || m_which == 1) ? 2 : 0;

    return strm;
  }
};

frOperator* create_GFUVMap() { return new frOpGFUVMap; }

// ---- make doublesided

class frOpGFDoubleSided: public frGeometryOperator
{
  sInt m_mode;

  void doProcess()
  {
    const frModel* input = getInputData(0);
    const sInt inN = input->nObjects;

    data->createObjects(inN * (m_mode & 1 ? 1 : 2));
    data->lightCount = 0;
    data->cloneModel(*input, 0, sTRUE);
    
    for (sInt it = 0; it < data->nObjects; it++)
    {
      frObject* obj = data->objects + it;
      frMesh* msh = obj->getMesh();
      
      msh->invertCulling();

      if (m_mode != 3)
        msh->invertNormals();
    }

    if (m_mode != 1 && m_mode != 3) // not invert only?
    {
      for (sInt i = 0; i < inN; i++)
      {
        frObject* newObj = data->cloneObject(input->objects[i], inN + i, sTRUE);

        if (m_mode == 2) // alpha setup
        {
          if (++newObj->renderPass > 7)
            newObj->renderPass = 7;
        }
      }
    }
  }
  
public:
  const sU8* readData(const sU8* strm)
  {
    m_mode = *strm++;
    opType = 2;

    return strm;
  }
};

frOperator* create_GFDoubleSided() { return new frOpGFDoubleSided; }

// ---- Load material

class frOpGSLoadMaterial: public frGeometryOperator
{
  void doProcess()
  {
    const frModel* inModel = getInputData(0);
    const sInt inN = inModel->nObjects;

    data->createObjects(inN);
    data->lightCount = 0;
    data->cloneModel(*inModel, 0);

    if (input[1])
    {
      const frModel* in2 = getInputData(1);
      frMaterial* mtrl = in2->objects[0].getMaterial();

      for (sInt i = 0; i < inN; i++)
        data->objects[i].setMaterial(mtrl);
    }
  }
};

frOperator* create_GSLoadMaterial() { return new frOpGSLoadMaterial; }

// ---- Lighting

class frOpGSLighting: public frGeometryOperator
{
  sInt        m_type;         // 0=dir 1=point 2=spot
  fr::vector  m_color, m_pos, m_dir, m_att;
  sF32        m_iCone, m_oCone;
  sF32        m_range, m_falloff;

  void doProcess()
  {
    const frModel* input = getInputData(0);
    const sInt inN = input->nObjects;

    data->createObjects(inN);
    data->lightCount = 0;
    data->cloneModel(*input, 0);

    if (data->lightCount < 8)
    {
      frLight* lgt = data->lights + (data->lightCount++);

      lgt->type = m_type;
      memcpy(&lgt->r, &m_color.x, 3*3*sizeof(sF32));  // copy color, position, direciton
      memcpy(&lgt->att[0], &m_att.x, 3*sizeof(sF32)); // copy attenuation
      lgt->range = m_range;
      lgt->innerc = m_iCone*PI;   lgt->outerc = m_oCone*PI;
      lgt->falloff = m_falloff;
    }
  }

  void setVector(fr::vector& vec, const sF32* val)
  {
    for (sInt i = 0; i < 3; i++)
			vec.v[i] = val[i];
  }

  void setAnim(sInt parm, const sF32* val)
  {
    switch (parm)
    {
    case 1: setVector(m_color, val);  break;          // color
    case 2: setVector(m_dir, val);    break;          // direction
    case 3: setVector(m_pos, val);    break;          // position
    case 4: setVector(m_att, val);    break;          // attenuation
    case 5: m_iCone=val[0]; m_oCone=val[1]; break;    // inner/outer cone
    case 6: m_falloff=val[0];         break;          // falloff
    }
  }

public:
  const sU8* readData(const sU8* strm)
  {
    m_type = *strm++;

    getPackedVec(m_color, strm);

    if (m_type != 0) // not dir?
    {
      m_range = getToterFloat(strm);
      getPackedVec(m_pos, strm);
      getPackedVec(m_att, strm);
    }
    else
    {
      m_att.x = 1.0f;
      m_att.y = m_att.z = 0.0f;
    }

    if (m_type != 1) // not point?
      getPackedVec(m_dir, strm);

    if (m_type == 2)
    {
      m_iCone = getToterFloat(strm);
      m_oCone = getToterFloat(strm);
      m_falloff = getToterFloat(strm);
    }

    return strm;
  }
};

frOperator* create_GSLighting() { return new frOpGSLighting; }

// ---- object merge

class frOpGFObjMerge: public frGeometryOperator
{
  void doProcess()
  {
    const frModel* in = getInputData(0);

    data->createObjects(in->nObjects); // this is a worst-case estimate
    data->lightCount = 0;
    data->cloneModelLights(*in);

    sInt nRealObjects = 0;

    for (sInt it = 0; it < in->nObjects; it++)
    {
      frObject* inObj = in->objects + it;

      frObject temp;
      temp.hardClone(*inObj);

      const frMesh* msh = temp.getMesh();
      const frMaterial* mat = temp.getMaterial();

      sInt it2;
      for (it2 = 0; it2 < nRealObjects; it2++)
      {
        frObject* inObj2 = data->objects + it2;
        frMesh* srcMesh2 = inObj2->getMesh();

        if (inObj2->getMaterial() == mat && inObj2->renderPass == inObj->renderPass &&
          srcMesh2->nVerts + msh->nVerts < 65536 && srcMesh2->nPrims + msh->nPrims < 65536)
        {
          frMesh* destMesh = new frMesh();

          destMesh->resize(msh->nVerts + srcMesh2->nVerts, msh->nPrims + srcMesh2->nPrims);
          destMesh->copyData(*msh);

          memcpy(destMesh->vertices + msh->nVerts, srcMesh2->vertices, srcMesh2->nVerts * sizeof(frVertex));
					memcpy(destMesh->primSelect + msh->nPrims, srcMesh2->primSelect, srcMesh2->nPrims * sizeof(sU8));

          const sInt mul = 3 - msh->type;
          sU16* destInd = destMesh->indices + msh->nPrims * mul;
          for (sU32 i = 0; i < srcMesh2->nPrims * mul; i++)
            *destInd++ = srcMesh2->indices[i] + msh->nVerts;

          for (sU32 i = 0; i < srcMesh2->nVerts; i++)
            destMesh->vertices[i + msh->nVerts].normAlias += msh->nVerts;

          destMesh->calcBoundingSphere();
          inObj2->setMesh(destMesh);

          break;
        }
      }

			if (it2 == nRealObjects)
				data->objects[nRealObjects++] = temp;
		}

    data->nObjects = nRealObjects;
  }
  
public:
  const sU8* readData(const sU8* strm)
  {
    opType = 2; // hard
    return strm;
  }
};

frOperator* create_GFObjMerge() { return new frOpGFObjMerge; }

// ---- apply lightmap

class frOpGFApplyLightmap: public frGeometryOperator
{
  frTextureOperator*  m_tex;
  sInt                m_mode;
  fr::vector          m_xform[3];

  void doProcess()
  {
    const frModel* inModel = getInputData(0);
    const sInt inN = inModel->nObjects;

    data->createObjects(inN);
    data->lightCount = 0;
    data->cloneModel(*inModel, 0);

    fr::matrix pMatrix;
    matrixSRT(m_xform, pMatrix);

    for (sInt it = 0; it < data->nObjects; it++)
    {
      frObject* inObj = data->objects + it;
      const frMaterial* mat = inObj->getMaterial();

      frMaterial* cmat = new frMaterial(*mat);
      frTextureOperator* texOp = static_cast<frTextureOperator*>(input[1]);

      texOp->updateTexture();

      cmat->tex[1] = texOp->getTexture();
      cmat->flags13 |= (m_mode + 1) << 5;
      cmat->flags21 |= 0xc0;
      cmat->pMatrix = pMatrix;
      inObj->setMaterial(cmat);
      cmat->release();
    }
  }

public:
  const sU8* readData(const sU8* strm)
  {
    m_mode = *strm++;

    for (sInt i = 0; i < 3; i++)
      getPackedVec(m_xform[i], strm);

    return strm;
  }
};

frOperator* create_GFApplyLightmap() { return new frOpGFApplyLightmap; }

// ---- wuschel

class frOpGFWuschel: public frGeometryOperator
{
  void doProcess()
  {
    const frModel* input = getInputData(0);

    data->createObjects(input->nObjects);
    data->lightCount = 0;
    data->cloneModel(*input, 0,sTRUE);
  }

public:
  const sU8* readData(const sU8* strm)
  {
    opType = 2; // hard
    return strm;
  }
};

frOperator* create_GFWuschel() { return new frOpGFWuschel; }

// ---- plode

class frOpGFPlode: public frGeometryOperator
{
  sInt    op;
  sF32    param;
  sInt    mode;
  
  void doProcess()
  {
    const frModel* input = getInputData(0);
    sF32 maxDist = 0.0f, invMaxDist, miny, maxy;

    bendPlodeFirstPass(data, input, mode == 1, miny, maxy, maxDist);

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

    data->cloneModelLights(*input);
  }

public:
  const sU8* readData(const sU8* strm)
  {
    sU8 ctrlByte = *strm++;

    op = ctrlByte & 1;
    mode = (ctrlByte >> 1) & 1;

    param = getPackedFloat(strm);

    opType = 2; // hard
    return strm;
  }
};

frOperator* create_GFPlode() { return new frOpGFPlode; }

// ---- make dumb normals

class frOpGFDumbNormals: public frGeometryOperator
{
  sBool eliminateSeams;

  void doProcess()
  {
    const frModel* input = getInputData(0);

    data->createObjects(input->nObjects);
    data->lightCount = 0;

    for (sInt cit = 0; cit < input->nObjects; cit++)
    {
      const frObject *inObj = input->objects + cit;
      frObject *obj = data->cloneObject(*inObj, cit, sTRUE);
      frMesh *msh = obj->getMesh();

      sInt i;
      for (i = 0; i < msh->nVerts; i++)
        msh->vertices[i].norm.set(0, 0, 0);

      const sU16 *inds = msh->indices;

      for (i = msh->nPrims; i; i--, inds+=3)
      {
        sU16 i1 = inds[0], i2 = inds[1], i3 = inds[2];

        if (eliminateSeams)
        {
          i1 = msh->vertices[i1].normAlias;
          i2 = msh->vertices[i2].normAlias;
          i3 = msh->vertices[i3].normAlias;
        }

        frVertex* v1 = msh->vertices + i1;
        frVertex* v2 = msh->vertices + i2;
        frVertex* v3 = msh->vertices + i3;
        
        fr::vector d1, d2, n;
        d1.sub(v2->pos, v1->pos);
        d2.sub(v3->pos, v1->pos);
        n.cross(d1, d2);
        n.norm();
        
        v1->norm.add(n);
        v2->norm.add(n);
        v3->norm.add(n);
      }
      
      for (i = 0; i < msh->nVerts; i++)
      {
        msh->vertices[i].norm.norm();
        msh->vertices[i].norm = msh->vertices[msh->vertices[i].normAlias].norm;
      }
    }
    
    data->cloneModelLights(*input);
  }
  
public:
  const sU8* readData(const sU8* strm)
  {
    eliminateSeams = *strm++;
    opType = 2; // hard

    return strm;
  }
};

frOperator* create_GFDumbNormals() { return new frOpGFDumbNormals; }

// ---- displacement

class frOpGFDisplacement: public frGeometryOperator
{
  sInt                chan;
  sF32                strength;
  sF32                approx, sharp;
  
  void doProcess()
  {
    const frModel* inModel = getInputData(0);
    
    data->createObjects(inModel->nObjects);
    data->lightCount = 0;
    frTextureOperator* tex = static_cast<frTextureOperator*>(input[1]);
    
    for (sInt cit = 0; cit < inModel->nObjects; cit++)
    {
      const frObject* inObj = inModel->objects + cit;
      frObject* obj = data->cloneObject(*inObj, cit, sTRUE);
      
      if (!tex)
        continue;

      frBitmap* bmp = tex->getData();
      const sU16* tdata = bmp->data + chan;
      const sInt tw = bmp->w, th = bmp->h;
      const sF32 ix = approx / tw, iy = approx / th;
      const sF32 scsc = (128.0f * sharp) / approx;
      
      frMesh* msh = obj->getMesh();
      msh->calcTangents();
      
      for (sInt i = 0; i < msh->nVerts; i++)
      {
        frVertex* vtx = msh->vertices + i;
        
        const sF32 h = sampleTexMap(tdata, tw, th, vtx->uvw.x, vtx->uvw.y) * strength;
        const sF32 dx = sampleTexMap(tdata, tw, th, vtx->uvw.x + ix, vtx->uvw.y) * strength - h;
        const sF32 dy = sampleTexMap(tdata, tw, th, vtx->uvw.x, vtx->uvw.y + iy) * strength - h;
        
        vtx->pos.addscale(vtx->norm, h);

				fr::vector t0, t1, t2;
				t0.scale(vtx->tangent, dx * scsc);
				t1.cross(vtx->norm, vtx->tangent);
				t1.scale(dy * scsc);
				t2.sub(t0, t1);
        
        vtx->norm.add(t2);
        vtx->norm.norm();
      }

      msh->calcBoundingSphere();
      obj->updateBounds();
    }

    data->cloneModelLights(*inModel);
  }
  
public:
  const sU8* readData(const sU8* strm)
  {
    chan = *strm++;
    strength = getPackedFloat(strm) / 32767.0f;
    approx = getPackedFloat(strm);
    sharp = getPackedFloat(strm);

    opType = 2; // hard

    return strm;
  }
};

frOperator* create_GFDisplacement() { return new frOpGFDisplacement; }

// ---- glow verts

class frOpGFGlowVerts: public frGeometryOperator
{
  sF32        m_size;
  sU32        m_color;
  frMesh      *msh;

  void doProcess()
  {
    const frModel* input = getInputData(0);

    data->createObjects(1);
    data->lightCount = 0;

    sInt numV = 0;

    for (sInt cit = 0; cit < input->nObjects; cit++)
			numV += input->objects[cit].getMesh()->nVerts;

    msh->resize(numV, 0);
    data->objects[0].setMesh(msh);

    frVertex* outVtx = msh->vertices;
    for (sInt it = 0; it < input->nObjects; it++)
    {
      const frMesh* msh = input->objects[it].getMesh();
      const fr::matrix& mt = input->objects[it].getTransform();

      for (sInt i = 0; i < msh->nVerts; i++)
      {
        outVtx->pos.transform(msh->vertices[i].pos, mt);
        outVtx->color = m_color;
				outVtx->uvw.set(m_size, m_size, 0.0f);
        outVtx->normAlias = i;
        outVtx++;
      }
    }

    data->lightCount = 0;
    data->cloneModelLights(*input);
  }

public:
  const sU8* readData(const sU8* strm)
  {
    msh = new frMesh;
    msh->type = 3;
    msh->bRadius = 0.0f;
		msh->addRef();

    m_size = getPackedFloat(strm);
		msh->vFormat = m_size < 0;
		m_size = fabs(m_size);

		if (msh->vFormat)
		{
			m_color = *reinterpret_cast<const sU32*>(strm) | 0xff000000;
			strm += 3;
		}

    opType = 2;
    return strm;
  }

  void freeData()
  {
    if (msh)
    {
      msh->release();
      msh = 0;
    }
  }
};

frOperator* create_GFGlowVerts() { return new frOpGFGlowVerts; }

// ---- shape animation

class frOpGCShapeAnim: public frGeometryOperator
{
  sF32 m_time;

	static sInt truncate(sF32 val)
	{
		sInt r;

		__asm
		{
			push	eax;
			fstcw	[esp];
			push	01e3fh;
			fldcw	[esp];
			fld		[val];
			fistp	dword ptr [esp];
			pop		[r];
			fldcw	[esp];
			pop		ecx;
		}

		return r;
	}

  void doProcess()
  {
    const frModel* input = getInputData(0);
    const sInt ioc = input->nObjects;

    for (sInt i = 1; i < nInputs; i++)
    {
      const frModel *inn = getInputData(i);

      if (inn->nObjects != ioc)
        return;

      for (sInt j = 0; j < ioc; j++)
      {
        const frMesh *msh1 = input->objects[j].getMesh();
        const frMesh *msh2 = inn->objects[j].getMesh();

        if (msh1->nVerts != msh2->nVerts || msh1->nPrims != msh2->nPrims)
          return;
      }
    }

    data->lightCount = 0;
    data->createObjects(ioc);

    if (m_time >= 0.999f)
      data->cloneModel(*getInputData(nInputs - 1), 0, sTRUE);
    else
    {
      const sF32 timeTrans = m_time * (nInputs - 1);
      const sInt inMesh = truncate(timeTrans);
      const sF32 weight = timeTrans - inMesh;

      data->cloneModel(*getInputData(inMesh), 0, sTRUE);
      const frModel* model2 = getInputData(inMesh + 1);

      for (sInt i = 0; i < data->nObjects; i++)
      {
        frMesh* msh1 = data->objects[i].getMesh();

        const frObject& obj2 = model2->objects[i];
        frMesh* msh2 = new frMesh(*obj2.getMesh());
        msh2->transform(obj2.getTransform(), obj2.getNormTransform(), obj2.getTexTransform(), obj2.textureMode);

        for (sInt j = 0; j < msh1->nVerts; j++)
        {
          frVertex* v0 = msh1->vertices + j;
          const frVertex* v1 = msh2->vertices + j;

          v0->pos.lerp(v0->pos, v1->pos, weight);
          v0->norm.lerp(v0->norm, v1->norm, weight);
          v0->norm.norm();
					v0->uvw.lerp(v0->uvw, v1->uvw, weight);
        }

        const sF32 dist = msh1->bPos.distanceTo(msh2->bPos);
        const sF32 newr = (msh1->bRadius + msh2->bRadius + dist) * 0.5f;
        const sF32 r = (newr - msh1->bRadius) / dist;

        msh1->bRadius = newr;
        msh1->bPos.lerp(msh1->bPos, msh2->bPos, r);
        data->objects[i].updateBounds();

        delete msh2;
      }
    }
  }

public:
  const sU8* readData(const sU8* strm)
  {
    m_time = getPackedFloat(strm);
    opType = 2;

    return strm;
  }

  void setAnim(sInt parm, const sF32* val)
  {
    m_time = val[0];
    if (m_time < 0.0f) // don't clamp upper bound, that's handled in doProcess anyway
      m_time = 0.0f;
  }
};

frOperator* create_GCShapeAnim() { return new frOpGCShapeAnim; }

// ---- render pass

class frOpGSRenderPass: public frGeometryOperator
{
  sInt m_pass;

  void doProcess()
  {
    const frModel* input = getInputData(0);

    data->createObjects(input->nObjects);

    for (sInt i = 0; i < input->nObjects; i++)
			data->cloneObject(input->objects[i], i, sFALSE)->renderPass = m_pass;

    data->lightCount = 0;
    data->cloneModelLights(*input);
  }

public:
  const sU8* readData(const sU8* strm)
  {
    m_pass = *strm++;

    return strm;
  }
};

frOperator* create_GSRenderPass() { return new frOpGSRenderPass; }

// ---- lorentz^2

static sInt mrand()
{
	static sInt seed = 1;

	seed = (seed * 214013) + 2531011;
	return (seed >> 16) & 32767;
}

class frOpGGLorentz: public frGeometryOperator
{
	struct particle
	{
		fr::vector	pos;
		fr::vector	vel;
	};

	sInt				nParts;
	particle*		parts;
	sF32				oldt;
	frMesh*			mesh;

	sF32				time;
	fr::vector	behavior, B;
	sF32				pSize;

	void reset()
	{
		for (sInt i = 0; i < nParts; i++)
		{
			parts[i].pos.set(mrand() / 16384.0f - 1.0f, mrand() / 16384.0f - 1.0f, mrand() / 16384.0f - 1.0f);
			parts[i].vel.set(0, 0, 0);
		}
	}

	void freeData()
	{
		frGeometryOperator::freeData();
		delete[] parts;
	}

	void doProcess()
	{
		sF32 tDelta = 0.0f;

		if (time < oldt)
			reset();
		else
			tDelta = time - oldt;

		frVertex* vtx = mesh->vertices;
		particle* prtcl = parts;

		for (sInt i = 0; i < nParts; i++)
		{
			fr::vector temp, rvel;

			rvel.x = prtcl->vel.x + behavior.x * (prtcl->pos.y - prtcl->pos.x);
			rvel.y = prtcl->vel.y + (prtcl->pos.x * (behavior.y - prtcl->pos.z) - prtcl->pos.y);
			rvel.z = prtcl->vel.z + (prtcl->pos.x * prtcl->pos.y - behavior.z * prtcl->pos.z);
			temp.cross(rvel, B);
			prtcl->vel.addscale(temp, tDelta);
			prtcl->pos.addscale(rvel, tDelta);

			vtx->pos = prtcl->pos;
			vtx->uvw.x = vtx->uvw.y = pSize;
			vtx++;
			prtcl++;
		}

		oldt = time;
	}

public:
	const sU8* readData(const sU8* strm)
	{
		nParts = getSmallInt(strm);
		time = getPackedFloat(strm);
		getPackedVec(behavior, strm);
		getPackedVec(B, strm);
		pSize = getPackedFloat(strm);
		nParts = getSmallInt(strm);

		parts = new particle[nParts];
		reset();
		oldt = time;

		data->createObjects(1);
		mesh = data->objects->getMesh();
		mesh->type = 3;
		mesh->resize(nParts, 0);

		opType = 1; // semihard

		return strm;
	}

	void setAnim(sInt parm, const sF32* val)
	{
		switch (parm)
		{
		case 1: // time
			time = val[0];
			break;

		case 2: // behavior
			for (sInt i = 0; i < 3; i++)
				behavior.v[i] = val[i];
			break;

		case 3: // p size
			pSize = val[0];
			break;
		}
	}
};

frOperator* create_GGLorentz() { return new frOpGGLorentz; }

// ---- jitter

static sS32 jrandom(sS32 n)
{
	return (n*(n*n*15731+789221)+1376312589) & 0xffff;
}

class frOpGFJitter: public frGeometryOperator
{
	sF32	m_amount;
	sInt	m_flags;
	sInt	m_seed;

	void doProcess()
	{
		const frModel* input = getInputData(0);
		data->createObjects(input->nObjects);
		data->lightCount = 0;
		data->cloneModel(*input, 0, sTRUE);

		const sInt mode = m_flags & 3;
		const sInt process = m_flags >> 2;
		sInt seed = m_seed * 3;

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

					vtx->pos.addscale(rndm, m_amount);
				}
				else // along normal
					vtx->pos.addscale(vtx->norm, rndm.x * m_amount);
			}
		}
	}

public:
	const sU8* readData(const sU8* strm)
	{
		m_amount = getPackedFloat(strm);
		m_flags = *strm++;
		m_seed = *((sU16*) strm);
		strm += 2;

		opType = 2; // hard

		return strm;
	}
};

frOperator* create_GFJitter() { return new frOpGFJitter; }
