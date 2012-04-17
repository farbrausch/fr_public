// This code is in the public domain. See LICENSE for details.

#include "stdafx.h"
#include "geobase.h"
#include "debug.h"
#include "exporter.h"
#include "tstream.h"
#include "exportTool.h"
#include <math.h>

static const sF32 PI = 3.1415926535f;

static void matrixSRT(fr::matrix& dest, const frParam* srtparams)
{
  FRDASSERT(srtparams[0].type == frtpThreeFloat);
  FRDASSERT(srtparams[1].type == frtpThreeFloat);
  FRDASSERT(srtparams[2].type == frtpThreeFloat);

  fr::matrix t1, t2;
  t1.scale(srtparams[0].trfloatv.a, srtparams[0].trfloatv.b, srtparams[0].trfloatv.c);
  dest.rotateHPB(srtparams[1].trfloatv.a * 2 * PI, srtparams[1].trfloatv.b * 2 * PI, srtparams[1].trfloatv.c * 2 * PI);
  t2.mul(t1, dest);
  t1.translate(srtparams[2].trfloatv.a, srtparams[2].trfloatv.b, srtparams[2].trfloatv.c);
  dest.mul(t2, t1);
}

// ---- get primitive

class frGGPrimitive: public frGeometryPlugin
{
  frObject* object;
  frMesh* mesh;
	sBool animUpdate;

  sBool onParamChanged(sInt param)
  {
		if (param != 1 && param != 2 && param != 3)
			animUpdate = sFALSE;

    return param == 0; // update parameters if obj type changed
  }

  void initNormAlias(sBool enable)
  {
    if (enable)
    {
      if (!mesh->normAlias)
        mesh->normAlias = new sU16[mesh->nVerts];

      for (sUInt i = 0; i < mesh->nVerts; i++)
        mesh->normAlias[i] = i;
    }
    else if (mesh->normAlias)
    {
      delete[] mesh->normAlias;
      mesh->normAlias = 0;
    }
  }

  void processCube()
  {
    sInt i, j, k;
    const sInt tes[3] = { params[4].trfloatv.a, params[4].trfloatv.b, params[4].trfloatv.c };
    const sBool ns = !!params[9].selectv.sel;
    
    const sInt nverts = 2 * ((tes[0] + 1) * (tes[1] + 1) + (tes[0] + 1) * (tes[2] + 1) + (tes[1] + 1) * (tes[2] + 1));
    const sInt nprims = 4 * (tes[0] * tes[1] + tes[0] * tes[2] + tes[1] * tes[2]);
    
    mesh->resize(nverts, nprims);
    initNormAlias(sFALSE);
    
    frVertex* verts = mesh->vertices, *pv = verts;
    sU16 *inds = mesh->indices, *pi = inds;
    sInt bvi = 0;
    
    for (i=0; i<6; i++) // generate vertices+indices
    {
      const sInt t1=i>>1;
      const sInt t2=(t1+1)%3;
      const sInt t3=(t2+1)%3;
      const sF32 v3=(i&1)?-1.0f:1.0f;

      for (j=0; j<tes[t1]; j++)
      {
        for (k=0; k<tes[t2]; k++)
        {
          const sInt bi=bvi+j*(tes[t2]+1)+k;

          if ((i&1)^ns)
          {
            pi[0]=bi+tes[t2]+1;
            pi[1]=bi+1;
            pi[2]=bi;
            pi[3]=bi+tes[t2]+1;
            pi[4]=bi+tes[t2]+2;
            pi[5]=bi+1;
          }
          else
          {
            pi[0]=bi;
            pi[1]=bi+1;
            pi[2]=bi+tes[t2]+1;
            pi[3]=bi+1;
            pi[4]=bi+tes[t2]+2;
            pi[5]=bi+tes[t2]+1;
          }

          pi+=6;
        }
      }

      for (j=0; j<=tes[t1]; j++)
      {
        for (k=0; k<=tes[t2]; k++)
        {
          sF32 u = 1.0f * j / tes[t1];
          sF32 v = 1.0f * k / tes[t2];
          
					pv->pos.v[t1] = (u - 0.5f) * 2.0f;
					pv->pos.v[t2] = (v - 0.5f) * 2.0f;
					pv->pos.v[t3] = v3;
					pv->norm.v[t1] = 0.0f;
					pv->norm.v[t2] = 0.0f;
					pv->norm.v[t3] = v3 * (ns ? 1.0f : -1.0f);
					
					if (t1==1)
					{
						sF32 t = u;
						u = 1.0f - v;
						v = t;
					}

					pv->uvw.set((i & 1)  ? u : 1.0f - u, 1.0f - v, 0.0f);
					pv->color = 0;
					pv->select = 0;
          
          pv++;
          bvi++;
        }
      }
    }
  }
  
  void processSphere()
  {
    sInt i, j;
    const sInt rings = params[5].tfloatv.a, sections = params[5].tfloatv.b;
    const sBool ninvert = !params[9].selectv.sel;
    
    const sInt ntris = (rings + 1) * sections * 2;
    const sInt nverts = (rings + 1) * (sections + 1) + 2 * sections;
    const sInt ninds = ntris * 3;
    
    mesh->resize(nverts, ntris);
    
    frVertex* verts = mesh->vertices, *pv = verts;
    sU16* inds = mesh->indices, *pi = inds;

    initNormAlias(sTRUE);

    // oberster punkt
    for (i = 0; i < sections; i++)
    {
			const sF32 phi = 2.0f * 3.1415926535f * i / sections;

      pv->pos.set(0.0f, 1.0f, 0.0f);
      pv->norm.set(0.0f, 1.0f, 0.0f);
			pv->uvw.set(1.0f - 1.0f * i / sections, 0.0f, 0.0f);
			pv->color = 0;
			pv->select = 0;
      pv++;

      mesh->normAlias[i] = 0;
    }

    sUInt vi = sections;

    for (i = 0; i <= rings; i++)
    {
      sF32 v = 1.0f * (i + 1) / (rings + 2), theta = v * 3.1415926535f;
      sF32 y = cos(theta);
      sF32 st = sin(theta);

      for (j = 0; j <= sections; j++)
      {
        sF32 phi = 2.0f * 3.1415926535f * j / sections;
        sF32 u = 1.0f - 1.0f * j / sections;
        sF32 x = sin(phi), z = cos(phi);

        if (j == sections)
          mesh->normAlias[vi] = vi - sections;
        
        pv->pos.set(st * x, y, st * z);
        pv->norm.set(st * x, y, st * z);
				pv->uvw.set(u, v, 0.0f);
				pv->color = 0;
				pv->select = 0;
        pv++;
        vi++;
      }
    }

    // unterster punkt
    for (i = 0; i < sections; i++)
    {
			const sF32 phi = 2.0f * 3.1415926535f * i / sections;

      pv->pos.set(0.0f, -1.0f, 0.0f);
      pv->norm.set(0.0f, -1.0f, 0.0f);
			pv->uvw.set(1.0f - 1.0f * i / sections, 1.0f, 0.0f);
			pv->color = 0;
			pv->select = 0;
      pv++;

      mesh->normAlias[vi + i] = vi;
    }

    // normalen invertieren wenn nötig
    if (ninvert)
    {
      for (i = 0; i < nverts; i++)
        verts[i].norm.scale(-1.0f);
    }

    const sInt fo = (ntris - sections) * 3;

    // indizes
    for (i = 0; i < sections; i++)
    {
      pi[0] = i;
      pi[1] = i + sections;
      pi[2] = i + sections + 1;
      pi[fo+0] = nverts - 1 - i;
      pi[fo+1] = nverts - sections - 1 - i;
      pi[fo+2] = nverts - sections - 2 - i;
      pi += 3;
    }

    sInt m = -1;

    for (i = 0; i < rings; i++)
    {
      m += sections + 1;

      for (j = 0; j < sections; j++)
      {
        pi[0] = m + j;
        pi[1] = m + sections + 1 + j;
        pi[2] = m + sections + 2 + j;
        pi[3] = pi[0];
        pi[4] = pi[2];
        pi[5] = m + j + 1;
        pi += 6;
      }
    }
    
    // indizes umdrehen wenn nötig
    if (ninvert)
    {
      for (i = 0; i < ninds; i += 3)
      {
        sU16 t = inds[i + 0];
        inds[i + 0] = inds[i + 2];
        inds[i + 2] = t;
      }
    }
  }
  
  void processCylinder()
  {
    sInt i, j;
    const sInt sectors = params[7].tfloatv.a, slices = params[7].tfloatv.b;
    const sBool ninvert = !params[9].selectv.sel;
    
    const sInt ntris = sectors * slices * 2 + 2 * sectors;
    const sInt nverts = (sectors + 1) * (slices + 3) + 2;
    const sInt ninds = ntris * 3;
    
    mesh->resize(nverts, ntris);
    initNormAlias(sTRUE);
    
    frVertex* verts = mesh->vertices, *pv = verts;
    sU16* inds = mesh->indices, *pi = inds;

    // -- zuerst die vertices

    // mittelpunkt oben
    pv->pos.set(0.0f, 1.0f, 0.0f);
    pv->norm.set(0.0f, 1.0f, 0.0f);
		pv->uvw.set(0.5f, 0.5f, 0.0f);
		pv->color = 0;
		pv->select = 0;
    pv++;

    // kappe oben
    for (i=0; i<=sectors; i++)
    {
      const sF32 u = 1.0f * i / sectors;
      const sF32 theta = u * 2.0f * PI;
      const sF32 st = sin(theta), ct = cos(theta);

      pv->pos.set(st, 1.0f, -ct);
      pv->norm.set(0.0f, 1.0f, 0.0f);
			pv->uvw.set(0.5f * (st + 1.0f), 0.5f * (1.0f - ct), 0.0f);
			pv->color = 0;
			pv->select = 0;
      pv++;
    }

    sUInt vi = sectors + 2;

    // körper
    for (i = 0; i <= slices; i++)
    {
      for (j = 0; j <= sectors; j++)
      {
        sF32 u = 1.0f * j / sectors, v = 1.0f * i / slices;
        sF32 theta = u * 2.0f * PI;
        sF32 st = sin(theta), ct = cos(theta);

        pv->pos.set(st, (0.5f - v) * 2.0f, -ct);
        pv->norm.set(st, 0, -ct);
        if (ninvert)
          pv->norm.scale(-1.0f);
				pv->uvw.set(u, v, 0.0f);
				pv->color = 0.0f;
				pv->select = 0;

        if (j == sectors)
          mesh->normAlias[vi] = vi - sectors;
        
        pv++;
        vi++;
      }
    }
    
    // kappe unten
    for (i = 0; i <= sectors; i++)
    {
      sF32 u = 1.0f * i / sectors;
      sF32 theta = u * 2.0f * PI;
      sF32 st = sin(theta), ct = cos(theta);
      
      pv->pos.set(st, -1.0f, -ct);
      pv->norm.set(0.0f, -1.0f, 0.0f);
			pv->uvw.set(0.5f * (st + 1.0f), 0.5f * (1.0f - ct), 0.0f);
			pv->color = 0;
			pv->select = 0;
      pv++;
    }

    // mittelpunkt unten
    pv->pos.set(0.0f, -1.0f, 0.0f);
    pv->norm.set(0.0f, -1.0f, 0.0f);
		pv->uvw.set(0.5f, 0.5f, 0.0f);
		pv->color = 0;
		pv->select = 0;
    pv++;
    
    // -- dann die indizes
    const sInt fo = (ntris - sectors) * 3;
    
    // indizes caps
    for (i = 0; i < sectors; i++)
    {
      pi[0] = 0;
      pi[1] = i + 1;
      pi[2] = i + 2;
      pi[fo+0] = nverts - 1;
      pi[fo+1] = nverts - 2 - i;
      pi[fo+2] = nverts - 3 - i;
      pi += 3;
    }

    sInt m = sectors + 2;
    
    for (i = 0; i < slices; i++)
    {
      for (j = 0; j < sectors; j++)
      {
        pi[0] = m + j;
        pi[1] = m + sectors + j + 1;
        pi[2] = m + sectors + j + 2;
        pi[3] = m + j;
        pi[4] = m + sectors + j + 2;
        pi[5] = m + j + 1;
        pi += 6;
      }
      
      m += sectors + 1;
    }
    
    // indizes umdrehen wenn nötig
    if (!ninvert)
    {
      for (i = 0; i < ninds; i += 3)
      {
        sU16 t = inds[i+0];
        inds[i+0] = inds[i+2];
        inds[i+2] = t;
      }
    }
  }

  void processTorus()
  {
    sInt i, j;
    const sInt rings = params[5].tfloatv.a, sections = params[5].tfloatv.b;
    const sBool ninvert = !params[9].selectv.sel;
    const sF32 radius2 = params[6].floatv;
    const sF32 rm = (radius2 + 1.0f) * 0.5f;
    const sF32 rd = rm - radius2;
    
    const sInt ntris = rings * sections * 2;
    const sInt nverts = (rings + 1) * (sections + 1);
    const sInt ninds = ntris * 3;
    
    mesh->resize(nverts, ntris);
    initNormAlias(sTRUE);
    
    frVertex* verts = mesh->vertices, *pv = verts;
    sU16* inds = mesh->indices, *pi = inds;

    // vertices
    sUInt vi = 0;

    for (i = 0; i <= rings; i++)
    {
      const sF32 u = 1.0f * i / rings, theta = u * 2 * PI, st = sin(theta), ct = cos(theta);
      fr::vector pos, rgt, tmp;

      pos.set(rm * st, 0.0f, -rm * ct);
      rgt.set(-st, 0.0f, ct);
      
      for (j = 0; j <= sections; j++)
      {
        const sF32 v = 1.0f * j / sections, phi = v * 2 * PI;
        
        tmp.set(0.0f, 1.0f, 0.0f);
        tmp.spin(rgt, phi);
        
        pv->pos.addscale(pos, tmp, rd);
        pv->norm = tmp;
        if (ninvert)
          pv->norm.scale(-1.0f);

				pv->uvw.set(u, v, 0.0f);

        sUInt tvi = vi;
        if (j == sections)
          tvi -= sections;

        if (i == rings)
          tvi -= rings * (sections+1);

        mesh->normAlias[vi] = tvi;

        pv->color = 0;
				pv->select = 0;
        pv++;
        vi++;
      }
    }
    
    sInt m = 0;

    for (i = 0; i < rings; i++)
    {
      for (j = 0; j < sections; j++)
      {
        pi[0] = m + j;
        pi[1] = m + j + 1;
        pi[2] = m + sections + j + 2;
        pi[3] = m + j;
        pi[4] = m + sections + j + 2;
        pi[5] = m + sections + j + 1;
        pi += 6;
      }

      m += sections + 1;
    }
    
    // indizes umdrehen wenn nötig
    if (ninvert)
    {
      for (i = 0;  i< ninds; i += 3)
      {
        sU16 t = inds[i + 0];
        inds[i + 0] = inds[i + 2];
        inds[i + 2] = t;
      }
    }
  }

  void processGrid()
  {
    sInt i, j;
    const sInt tes[2] = { params[8].tfloatv.a, params[8].tfloatv.b };
    const sBool ninvert = !params[9].selectv.sel;
    
    const sInt nverts = (tes[0] + 1) * (tes[1] + 1);
    const sInt nprims = 2 * tes[0] * tes[1];
    const sInt ninds = nprims * 3;
    
    mesh->resize(nverts, nprims);
    initNormAlias(sFALSE);

    frVertex* verts = mesh->vertices, *pv = verts;
    sU16* inds = mesh->indices, *pi = inds;

    // vertices
    for (i = 0; i <= tes[0]; i++)
    {
      const sF32 v = 1.0f * i / tes[0];

      for (j = 0; j <= tes[1]; j++)
      {
        const sF32 u = 1.0f * j / tes[1];

        pv->pos.set((u - 0.5f) * 2.0f, (0.5f - v) * 2.0f, 0.0f);
        pv->norm.set(0.0f, 0.0f, ninvert ? 1.0f : -1.0f);
				pv->uvw.set(u, v, 0.0f);
				pv->color = 0;
				pv->select = 0;
        pv++;
      }
    }

    // indices
    for (i = 0; i < tes[0]; i++)
    {
      for (j = 0; j < tes[1]; j++)
      {
        const sInt bi = i * (tes[1] + 1) + j;

        pi[0] = bi;
        pi[1] = bi + 1;
        pi[2] = bi + tes[1] + 2;
        pi[3] = bi;
        pi[4] = bi + tes[1] + 2;
        pi[5] = bi + tes[1] + 1;
        pi += 6;
      }
    }

    // indizes umdrehen wenn nötig
    if (ninvert)
    {
      for (i = 0; i < ninds; i += 3)
      {
        sU16 t = inds[i + 0];
        inds[i + 0] = inds[i + 2];
        inds[i + 2] = t;
      }
    }
  }
  
  sBool doProcess()
  {
		if (!animUpdate)
		{
			const sInt objType = params[0].selectv.sel;

			switch (objType)
			{
			case 0: processCube();      break;
			case 1: processSphere();    break;
			case 2: processCylinder();  break;
			case 3: processTorus();     break;
			case 4: processGrid();      break;
			}
	    
			memset(mesh->primSelect, 0, mesh->nPrims);

			mesh->calcBoundingSphere();
		}

    fr::matrix trans;
    matrixSRT(trans, params + 1);
    object->setTransform(trans);
		animUpdate = sTRUE;
    
    return sTRUE;
  }

public:
  frGGPrimitive(const frPluginDef* d)
    : frGeometryPlugin(d), animUpdate(sFALSE)
  {
    data->createObjects(1);
    object = data->objects;
    mesh = object->createMesh();

    addSelectParam("Object type", "Cube|Sphere|Cylinder|Torus|Grid", 0);
    addThreeFloatParam("Scale", 1.0f, 1.0f, 1.0f, 0.0f, 1e+15f, 0.01f, 5);
    addThreeFloatParam("Rotate", 0.0f, 0.0f, 0.0f, -1e+15f, 1e+15f, 0.01f, 5);
    addThreeFloatParam("Translate", 0.0f, 0.0f, 0.0f, -1e+15f, 1e+15f, 0.01f, 5);

    params[1].animIndex = 1; // scale
    params[2].animIndex = 2; // rotate
    params[3].animIndex = 3; // translate

    addThreeFloatParam("Tesselation", 1.0f, 1.0f, 1.0f, 1.0f, 128.0f, 1.0f, 0);
    addTwoFloatParam("Rings/Sections", 12.0f, 24.0f, 3, 256, 1, 0);
    addFloatParam("Radius 2", 0.5f, 0.0f, 1.0f, 0.001f, 5);
    addTwoFloatParam("Sectors/Slices", 12.0f, 1.0f, 1, 256, 1, 0);
    addTwoFloatParam("Tesselation", 1.0f, 1.0f, 1.0f, 128.0f, 1.0f, 0);
    addSelectParam("Normals", "Inward|Outward", 1);
  }
  
  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver = 7;
    
    strm << ver;

    if (ver == 7)
    {
      strm << params[0].selectv;
      strm << params[1].trfloatv << params[2].trfloatv << params[3].trfloatv;
      strm << params[4].trfloatv;
      strm << params[5].tfloatv;
      strm << params[9].selectv.sel;
      strm << params[7].tfloatv;
      strm << params[6].floatv;
      strm << params[8].tfloatv;
    }
    else if (ver < 7)
    {
      if (ver<2)
      {
        params[0].selectv.sel = ver;
        sU16 buf=0;
        strm << buf;
      }
      else
        strm << params[0].selectv;

      strm << params[1].trfloatv << params[2].trfloatv << params[3].trfloatv;
      strm << params[4].trfloatv;
      strm << params[5].tfloatv;
      strm << params[9].selectv.sel;

      if (ver>=3)
        strm << params[7].tfloatv;
      else
      {
        params[7].tfloatv.a = 12.0f;
        params[7].tfloatv.b = 1.0f;
      }
      
      if (ver>=4)
        strm << params[6].floatv;
      else
        params[6].floatv = 0.5f;
      
      if (ver>=5)
        strm << params[8].tfloatv;
      else
        params[8].tfloatv.a = params[8].tfloatv.b = 1.0f;
      
      if (ver==6)
      {
        sInt mappingAtCaps=0;
        strm << mappingAtCaps;
      }    
    }
  }

  void exportTo(fr::stream &f, const frGraphExporter &exp)
  {
    sBool tessPacked = sFALSE;
    
    switch (params[0].selectv.sel)
    {
    case 0: // cube
      if (params[4].trfloatv.a == params[4].trfloatv.b && params[4].trfloatv.b == params[4].trfloatv.c)
        tessPacked = sTRUE;
      break;

    case 1: // sphere
    case 3: // torus
      if (params[5].tfloatv.a == params[5].tfloatv.b)
        tessPacked = sTRUE;
      break;

    case 2: // cylinder
      if (params[7].tfloatv.a == params[7].tfloatv.b)
        tessPacked = sTRUE;
      break;

    case 4: // grid
      if (params[8].tfloatv.a == params[8].tfloatv.b)
        tessPacked = sTRUE;
      break;
    }

    f.putsU8(params[0].selectv.sel | (tessPacked ? 64 : 0) | (params[9].selectv.sel ? 128 : 0));

    putPackedVector(f, params[1].trfloatv, sTRUE, (params[0].selectv.sel == 4) ? 4 : 0);
    putPackedVector(f, params[2].trfloatv);
    putPackedVector(f, params[3].trfloatv);

    switch (params[0].selectv.sel)
    {
    case 0: // cube
      f.putsU8(params[4].trfloatv.a);
      if (!tessPacked)
      {
        f.putsU8(params[4].trfloatv.b);
        f.putsU8(params[4].trfloatv.c);
      }
      break;

    case 1: // sphere
      f.putsU8(params[5].tfloatv.a);
      if (!tessPacked)
        f.putsU8(params[5].tfloatv.b);
      break;

    case 2: // cylinder
      f.putsU8(params[7].tfloatv.a);
      if (!tessPacked)
        f.putsU8(params[7].tfloatv.b);
      break;

    case 3: // torus
      f.putsU8(params[5].tfloatv.a);
      if (!tessPacked)
        f.putsU8(params[5].tfloatv.b);
      putPackedFloat(f, params[6].floatv);
      break;

    case 4: // grid
      f.putsU8(params[8].tfloatv.a);
      if (!tessPacked)
        f.putsU8(params[8].tfloatv.b);
      break;
    }
  }

  sBool displayParameter(sU32 index)
  {
    const sInt type = params[0].selectv.sel;

    switch (index)
    {
    case 0: case 1: case 2: case 3: case 9:
      return sTRUE;

    case 4:
      return type == 0;

    case 5:
      return type == 1 || type == 3;

    case 6:
      return type == 3;

    case 7:
      return type == 2;

    case 8:
      return type == 4;
    }

    return sFALSE;
  }

  void setAnim(sInt index, const sF32* vals)
  {
    params[index].trfloatv.a = vals[0];
    params[index].trfloatv.b = vals[1];
    params[index].trfloatv.c = vals[2];

    dirty = sTRUE;
  }
};

TG2_DECLARE_PLUGIN(frGGPrimitive, "Get Primitive", 1, 0, 0, sTRUE, sFALSE, 128);

// ---- lorentz

static sInt mrand()
{
	static sInt seed = 1;

	seed = (seed * 214013) + 2531011;
	return (seed >> 16) & 32767;
}

class frGGLorentz: public frGeometryPlugin
{
	struct particle
	{
		fr::vector	pos;
		fr::vector	vel;
	};

	sInt				nParts;
	particle*		parts;
	sF32				oldt;
	frObject*		object;
	frMesh*			mesh;

	void reset()
	{
		for (sInt i = 0; i < nParts; i++)
		{
			parts[i].pos.set(mrand() / 16384.0f - 1.0f, mrand() / 16384.0f - 1.0f, mrand() / 16384.0f - 1.0f);
			parts[i].vel.set(0, 0, 0);
		}
	}

	sBool doProcess()
	{
		const sF32 time = params[0].floatv;
		const sF32 a = params[1].trfloatv.a * 10.0f;
		const sF32 b = params[1].trfloatv.b * 28.0f;
		const sF32 c = params[1].trfloatv.c * 2.66667f;
		const sF32 sz = params[2].floatv;
		const sInt count = params[3].floatv;
		const fr::vector B(params[4].trfloatv.a, params[4].trfloatv.b, params[4].trfloatv.c);
		sF32 tDelta = 0.0f;
		
		if (count != nParts)
		{
			nParts = count;
			delete[] parts;
			parts = new particle[nParts];

			mesh->resize(nParts, 0);
			reset();
		}
		else if (time < oldt)
			reset();
		else
			tDelta = time - oldt;

		frVertex* vtx = mesh->vertices;
		for (sInt i = 0; i < nParts; i++)
		{
			fr::vector temp, rvel;

			rvel.x = parts[i].vel.x + a * (parts[i].pos.y - parts[i].pos.x);
			rvel.y = parts[i].vel.y + (parts[i].pos.x * (b - parts[i].pos.z) - parts[i].pos.y);
			rvel.z = parts[i].vel.z + (parts[i].pos.x * parts[i].pos.y - c * parts[i].pos.z);

			temp.cross(rvel, B);

			parts[i].vel.addscale(temp, tDelta);
			parts[i].pos.addscale(rvel, tDelta);

			vtx->pos = parts[i].pos;
			vtx->uvw.x = vtx->uvw.y = sz;
			vtx++;
		}

		oldt = time;
		object->updateBounds();

		return sTRUE;
	}

public:
	frGGLorentz(const frPluginDef* d)
		: frGeometryPlugin(d), nParts(0), parts(0), oldt(0.0f)
	{
		data->createObjects(1);
		object = data->objects;
		mesh = object->createMesh();
		mesh->type = 3;
		mesh->vFormat = 0;
		mesh->bRadius = 0.0f;

		addFloatParam("Time", 0.0f, 0.0f, 1e+15f, 0.01f, 4);
		addThreeFloatParam("Behavior", 1.0f, 1.0f, 1.0f, 0.0f, 64.0f, 0.001f, 4);
		addFloatParam("PSize", 0.01f, 0.0001f, 100.0f, 0.01f, 3);
		addFloatParam("Count", 200, 10, 5000, 1, 0);
		addThreeFloatParam("B", 0.0f, 0.0f, 0.0f, -1e+15f, 1e+15f, 0.001f, 3);

		params[0].animIndex = 1; // time
		params[1].animIndex = 2; // behavior
		params[2].animIndex = 3; // size
	}

	~frGGLorentz()
	{
		delete[] parts;
	}

	void serialize(fr::streamWrapper& strm)
	{
		sU16 ver = 1;
		strm << ver;

		if (ver == 1)
		{
			strm << params[0].floatv;
			strm << params[1].trfloatv;
			strm << params[2].floatv << params[3].floatv;
			strm << params[4].trfloatv;
		}
	}

	void exportTo(fr::stream& f, const frGraphExporter& exp)
	{
		putSmallInt(f, params[3].floatv);
		putPackedFloat(f, params[0].floatv);
		putPackedFloat(f, params[1].trfloatv.a * 10.0f);
		putPackedFloat(f, params[1].trfloatv.b * 28.0f);
		putPackedFloat(f, params[1].trfloatv.c * 2.66667f);
		putPackedVector(f, params[4].trfloatv, sFALSE);
		putPackedFloat(f, params[2].floatv);
		putSmallInt(f, params[3].floatv);
	}

	void setAnim(sInt index, const sF32* vals)
	{
		params[index - 1].trfloatv.a = vals[0];
		params[index - 1].trfloatv.b = vals[1];
		params[index - 1].trfloatv.c = vals[2];

		dirty = sTRUE;
	}
};

TG2_DECLARE_PLUGIN(frGGLorentz, "Lorentz^2", 1, 0, 0, sTRUE, sFALSE, 156);
