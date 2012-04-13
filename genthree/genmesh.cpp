// This file is distributed under a BSD license. See LICENSE.txt for details.
#include "cslrt.hpp"
#include "genmesh.hpp"
#include "genmaterial.hpp"
#include "genbitmap.hpp"
#include "_util.hpp"

/****************************************************************************/

#define RBCOUNTMAX (1024*1024*2)

static sU32 RBData[RBCOUNTMAX];
static sMatrix RBMat[1024];
static sObject *RBObj[256];
static sInt RBCount;
static sInt RBIndex;
static sInt RBObjIndex;
static GenMesh *RBMesh;

sMAKEZONE(MeshPlay ,"MeshPlay" ,0xff000080);
sMAKEZONE(MeshPaint,"MeshPaint",0xff0000ff);

/****************************************************************************/
/****************************************************************************/

void GenMeshElem::InitElem()
{
  Mask = 0;
  Id = 0;
  Select = 0;
  Used = 1;
}

void GenMeshElem::SelElem(sU32 mask1,sU32 mask0)
{
  Mask = (Mask & ~mask0) | mask1;
}

void GenMeshVert::Init()
{
  sInt i;

  InitElem();
  Next = -1;
  First = -1;
  Temp = -1;
  Temp2 = -1;
  ReIndex = -1;
  for(i=0;i<4;i++)
  {
    Matrix[i] = 0xff;
    Weight[i] = 0;
  }
}

void GenMeshEdge::Init()
{
  InitElem();
  Next[0] = -1;
  Next[1] = -1;
  Prev[0] = -1;
  Prev[1] = -1;
  Face[0] = -1;
  Face[1] = -1;
  Vert[0] = -1;
  Vert[1] = -1;
  Temp[0] = -1;
  Temp[1] = -1;
  Crease = 0;
}

void GenMeshFace::Init()
{
  InitElem();
  Material = 1;
  Edge = -1;
  Temp = -1;
}

void GenMeshPass::Init()
{
	MatPass = 0;
	Reset();
}

void GenMeshPass::Reset()
{
	sInt i;
	
	for(i=0;i<GENMAT_MAXBATCH;i++)
	{
		Geometry[i] = 0;
		IndexCount[i] = 0;
		VertexCount[i] = 0;
	}
  MatPass = 0;
	BatchCount = 0;
}

void GenMeshMtrl::Init()
{
  sInt i;

  InitElem();
  for(i=0;i<GENMAT_MAXPASS;i++)
    Pass[i].Init();
  Material = 0;
}

void GenMeshAnim::Copy(sObject *o)
{
  GenMeshAnim *oa;

  sVERIFY(o->GetClass()==sCID_GENMESHANIM);

  oa = (GenMeshAnim *)o;

  s = oa->s;
  r = oa->r;
  t = oa->t;
  v = oa->v;
  p = oa->p;
  MatrixIndex = oa->MatrixIndex;
  Operation = oa->Operation;
  Flags = oa->Flags;
  _Label = oa->_Label;
  OMat = oa->OMat;
  OVec = oa->OVec;
}

/****************************************************************************/
/****************************************************************************/

GenMesh::GenMesh()
{
  Vert.Init(16);
  Edge.Init(16);
  Face.Init(16);
  Mtrl.Init(16);

  Mtrl.Count = 2;
  Mtrl[0].Init();
  Mtrl[1].Init();

  VertMask = 0;
  VertSize = 0;
  VertAlloc = 0;
  VertCount = 0;
  VertBuf = 0;

  BoneMatrix.Init();
  BoneCurve.Init();
  KeyCount = 0;
  CurveCount = 0;
  KeyBuf = 0;

  Prepared = 0;
  Anim0 = 0;
  Anim1 = 16;

  StoreMode = 0;
  RecMode = 0;
  RecLevel = 0;
  RecVert.Init();
  RecMat.Init();
  RecObj.Init();
  RecCmd.Init();
	RecOpCount = 0;
	RecOpCountPtr = 0;
	RecCommandLevel = -1;

  sSetMem(AnimLabels,0,sizeof(AnimLabels));
  AnimLabelCount = 0;
  AnimLabelLastMat = 0;
  AnimLabelLastOp = GMA_NOP;
}

GenMesh::~GenMesh()
{
  sInt i,j,k;
  for(i=0;i<Mtrl.Count;i++)
  {
    for(j=0;j<GENMAT_MAXPASS;j++)
    {
      for(k=0;k<Mtrl[i].Pass[j].BatchCount;k++)
      {
        if(Mtrl[i].Pass[j].Geometry[k])
          sSystem->GeoRem(Mtrl[i].Pass[j].Geometry[k]);
        if(Mtrl[i].Pass[j].IndexBuffers[k])
          delete[] Mtrl[i].Pass[j].IndexBuffers[k];
        if(Mtrl[i].Pass[j].VertexBuffers[k])
          delete[] Mtrl[i].Pass[j].VertexBuffers[k];
      }
    }
  }
  Vert.Exit();
  Edge.Exit();
  Face.Exit();
  Mtrl.Exit();
  BoneMatrix.Exit();
  BoneCurve.Exit();
  RecVert.Exit();
  RecMat.Exit();
  RecObj.Exit();
  RecCmd.Exit();
  if(VertBuf)
    delete[] VertBuf;
  if(KeyBuf)
    delete[] KeyBuf;
}

void GenMesh::Tag()
{
  sInt i,j;
  GenMeshMtrl *gmm;

  for(i=0;i<Mtrl.Count;i++)
  {
    gmm = &Mtrl[i];
    sBroker->Need(gmm->Material);
    for(j=0;j<GENMAT_MAXPASS;j++)
      sBroker->Need(gmm->Pass[j].MatPass);
  }

  for(i=0;i<RecObj.Count;i++)
		sBroker->Need(RecObj.Array[i]);

  for(i=0;i<AnimLabelCount;i++)
    sBroker->Need(AnimLabels[i]);
}

void GenMesh::Copy(sObject *o)
{
  GenMesh *om;
  sInt i,j;

  sVERIFY(o->GetClass()==sCID_GENMESH);
  om = (GenMesh *)o;
  sVERIFY(om->RecMode==0);
  _Label = om->_Label;
  Vert.Copy(om->Vert);
  Edge.Copy(om->Edge);
  Face.Copy(om->Face);
  Mtrl.Copy(om->Mtrl);

  for(i=0;i<Mtrl.Count;i++)
  {
    for(j=0;j<GENMAT_MAXPASS;j++)
      Mtrl[i].Pass[j].Reset();
  }
  Prepared = 0;
  Anim0 = om->Anim0;
  Anim1 = om->Anim1;

  Init(om->VertMask,om->VertAlloc);
  sVERIFY(VertSize==om->VertSize);
  sCopyMem4((sU32 *)VertBuf,(sU32 *)om->VertBuf,om->VertSize*om->VertCount*4);
  VertCount = om->VertCount;

  BoneMatrix.Copy(om->BoneMatrix);
  BoneCurve.Copy(om->BoneCurve);
  KeyCount = om->KeyCount;
  CurveCount = om->CurveCount;
  if(om->KeyBuf)
  {
    KeyBuf = new sF32[KeyCount*CurveCount];
    sCopyMem(KeyBuf,om->KeyBuf,KeyCount*CurveCount*4);
  }

  StoreMode = om->StoreMode;
  RecVert.Copy(om->RecVert);
  RecCmd.Copy(om->RecCmd);
  RecMat.Copy(om->RecMat);
  RecObj.Copy(om->RecObj);

  AnimLabelCount = om->AnimLabelCount;
  AnimLabelLastMat = om->AnimLabelLastMat;
  AnimLabelLastOp = om->AnimLabelLastOp;
  for(i=0;i<om->AnimLabelCount;i++)
  {
    AnimLabels[i] = new GenMeshAnim;
    AnimLabels[i]->Copy(om->AnimLabels[i]);
  }
}

sU32 GenMesh::Features2Mask(sInt colorSets,sInt uvSets)
{
  sVERIFY(colorSets>=0 && colorSets<=1);
  sVERIFY(uvSets>=1 && uvSets<=4);

  return sGMF_POS|sGMF_NORMAL/*|sGMF_TANGENT*/|(((1<<colorSets)-1)<<sGMI_COLOR0)
    |(((1<<uvSets)-1)<<sGMI_UV0);
}

void GenMesh::Init(sU32 vertmask,sInt vertcount)
{
  sInt i;

  sREGZONE(MeshPlay);
  sREGZONE(MeshPaint);

  VertMask = vertmask;
#if sINTRO
  sVERIFY(vertmask == 0x23);
#endif
  VertSize = 0;
  VertAlloc = vertcount;
  VertCount = 0;
  for(i=0;i<16;i++)
  {
    if(VertMask & (1<<i))
      VertMap[i] = VertSize++;
    else
      VertMap[i] = -1;
  }
  sVERIFY(VertSize);
  sVERIFY(VertMask&1);
  VertBuf = new sVector[VertSize*VertAlloc];
  sSetMem4((sU32*)VertBuf,0,VertSize*VertAlloc*4);

  Vert.AtLeast(vertcount);
  Edge.AtLeast(vertcount*2);
  Face.AtLeast(vertcount/2);
}

void GenMesh::Realloc(sInt vertcount)
{
  sInt ns;
  sVector *nd;

  if(vertcount>=VertAlloc)
  {
#if !sINTRO
    sDPrintF("realloc %d\n",vertcount);
#endif
    ns = sMax(vertcount,VertAlloc*2);
    nd = new sVector[ns*VertSize];
    sCopyMem4((sU32 *)nd,(sU32 *)VertBuf,VertSize*4*VertCount);
    sSetMem4((sU32 *)(nd+VertSize*VertCount),0,VertSize*4*(ns-VertCount));

    delete[] VertBuf;
    VertBuf = nd;
    VertAlloc = ns;
  }
  VertCount = vertcount;
}

/****************************************************************************/
/****************************************************************************/

sU32 *GenMesh::RecBegin(sU32 command)
{
  if(RecMode==0)
  {
    sVERIFY(RBMesh==0);
    sVERIFY(RecLevel==0);
    RBCount = 0;
    RBIndex = RecMat.Count;
		RBObjIndex = RecObj.Count;
    RBMesh = this;
    RecMode=1;
  }
  RecLevel++;

	if (command)
	{
		sVERIFY(RecCommandLevel == -1);

		RBData[RBCount++] = command;
		RecOpCountPtr = RBData + RBCount++;
		RecLastSource = 0;
		RecLastDest = 0;
		RecOpCount = 0;
		RecCommandLevel = RecLevel - 1;
	}
  
  return RBData+RBCount;
}

void GenMesh::RecEnd(sU32 *data)
{
  RBCount = data-RBData;
  RecEnd();
}

void GenMesh::RecEnd()
{
  sInt o,n;

  RecLevel--;
	if(RecLevel==RecCommandLevel)
	{
		*RecOpCountPtr = RecOpCount;
		RecCommandLevel = -1;
	}

  if(RecLevel==0 && StoreMode)
  {
    o = RecCmd.Count;
    n = o+RBCount;
    if(n!=o)
    {
      RecCmd.SetMax(n+1);
      RecCmd.Count = n;
      sCopyMem(RecCmd.Array+o,RBData,sizeof(sU32)*(n-o));
      RecCmd.Array[n] = 0;
    }

    o = RecMat.Count;
    n = RBIndex;
    if(n!=o)
    {
      RecMat.SetMax(n);
      RecMat.Count = n;
      sCopyMem(RecMat.Array+o,RBMat+o,sizeof(sMatrix)*(n-o));
    }

    o = RecObj.Count;
    n = RBObjIndex;
    if(n!=o)
    {
      RecObj.SetMax(n);
      RecObj.Count = n;
      sCopyMem(RecObj.Array+o,RBObj+o,sizeof(sObject*)*(n-o));
    }
  }

  if(RecLevel==0)
  {
    RBData[RBCount++] = 0;
    sVERIFY(RecMode!=0);
    sVERIFY(RBMesh==this);
    sVERIFY(RBCount<RBCOUNTMAX);
    sVERIFY(RBIndex<1024);
		sVERIFY(RBObjIndex<256);
    if(RecMode==1)
      RecPlay(RBData,RBMat,RBObj);
    RBMesh = 0;
    RBCount = 0;
    RBIndex = 0;
		RBObjIndex = 0;
    RecMode = 0;
  }
}

void GenMesh::RecStoreMode()
{
  sInt n;

  if(!StoreMode)
  {
    StoreMode = 1;
    n = VertCount*VertSize;
    RecVert.SetMax(n);
    RecVert.Count = n;
    sCopyMem(RecVert.Array,VertBuf,sizeof(sVector)*n);
  }
}

void GenMesh::RecReplay()
{
  sInt i,j;
  GenMeshAnim *gma;
  sMatrix mat;
  sF32 srt[12];

#if !sINTRO  
  srt[0] = 1;  srt[1] = 1;  srt[2] = 1;
  srt[3] = 0;  srt[4] = 0;  srt[5] = 0;
  srt[6] = 0;  srt[7] = 0;  srt[8] = 0;
  srt[9] = 0;  srt[10]= 0;  srt[11]= 0;
#endif

  if(StoreMode && RecCmd.Count>0)
  {
    for(i=0;i<AnimLabelCount;i++)
    {
      gma = AnimLabels[i];
      if(!(gma->Flags & GMF_DOUBLE))
        for(j=0;j<12;j++)
          srt[j] = (&gma->s.x)[j]/65536.0f;

      switch(gma->Operation)
      {
      case GMA_TRANSFORM:
        mat.InitSRT(srt);
        RecMat.Array[gma->MatrixIndex].Mul4(gma->OMat,mat);
        break;
      case GMA_PERLIN:
        mat.InitSRT(srt);
        RecMat.Array[gma->MatrixIndex].Mul4(gma->OMat,mat);
        RecMat.Array[gma->MatrixIndex+1].i.Init(srt[9],srt[10],srt[11]);
        RecMat.Array[gma->MatrixIndex+1].i.Add3(gma->OVec);
        break;
      case GMA_BONE:
        BonesModify(gma->MatrixIndex,((gma->p + sFtol(gma->OMat.i.x*65536.0f))&0xffff)/65536.0f);
        break;
      case GMA_EWK:
        RecMat.Array[gma->MatrixIndex] = gma->OMat;
        RecMat.Array[gma->MatrixIndex].i.x *= srt[0];
        RecMat.Array[gma->MatrixIndex].i.y *= srt[1];
        RecMat.Array[gma->MatrixIndex].i.z *= srt[2];
        RecMat.Array[gma->MatrixIndex].i.w += gma->p/65536.0f;
        RecMat.Array[gma->MatrixIndex].j.x += srt[3];
        RecMat.Array[gma->MatrixIndex].j.y += srt[4];
        RecMat.Array[gma->MatrixIndex].j.z += srt[5];
        break;
      }
    }

    sCopyMem(VertBuf,RecVert.Array,sizeof(sVector)*RecVert.Count);
    RecPlay(RecCmd.Array,RecMat.Array,RecObj.Array);
  }
}

/****************************************************************************/

void GenMesh::MarkAnimLabel(sInt mode)
{
  if(StoreMode)
  {
    AnimLabelLastOp = mode;
    AnimLabelLastMat = RecMat.Count;
  }
}

void GenMesh::SetAnimLabel(sInt label,sU32 flags)
{
  GenMeshAnim *gma;

  if(StoreMode && AnimLabelCount<32 && AnimLabelLastOp!=GMA_NOP)
  {
    gma = new GenMeshAnim;
    gma->Flags = flags;
    gma->s.Init(0x10000,0x10000,0x10000);
    sSetMem4((sU32 *)&gma->r,0,3*3+1); // r,t,v,p
    gma->MatrixIndex = AnimLabelLastMat;
    gma->Operation = AnimLabelLastOp;
    gma->OMat = RecMat[AnimLabelLastMat];
    if(AnimLabelLastMat+1<RecMat.Count)
      gma->OVec = RecMat[AnimLabelLastMat+1].i;
    gma->_Label = label;

    if(flags & GMF_DOUBLE)
      gma->_Label = 0;
    AnimLabels[AnimLabelCount++] = gma;
  }
}

/****************************************************************************/

sInt GenMesh::RecMatrix(const sMatrix &mat)
{
  RBMat[RBIndex] = mat;
  return RBIndex++;
}

sInt GenMesh::RecObject(sObject *obj)
{
	RBObj[RBObjIndex] = obj;
	return RBObjIndex++;
}

void GenMesh::RecPlay(sU32 *data,sMatrix *matrices,sObject **objects)
{
  sZONE(MeshPlay);
	while (*data)
	{
		switch (*data++)
		{
		case IM_XFORM:		data = RecTransformInner(data,matrices);				break;
		case IM_BONE:			data = RecBoneInner(data,matrices);							break;
		case IM_ADDSCL:		data = RecAddScaleInner(data,matrices);					break;
		case IM_NORMAL:		data = RecNormalInner(data,matrices);						break;
#if !sINTRO_X
		case IM_DISPLACE:	data = RecDisplaceInner(data,matrices,objects);	break;
		case IM_BEVEL:		data = RecBevelInner(data,matrices);						break;
#endif
		case IM_PERLIN:		data = RecPerlinInner(data,matrices);						break;
		case IM_COPY:			data = RecCopyInner(data,matrices,objects);			break;
    case IM_F2VN:     data = RecF2VNInner(data,matrices);             break;
    case IM_EWK:      data = RecEWKInner(data,matrices);              break;
		}
	}
}

sU32 *GenMesh::RecSourceInd(sU32 *data,sInt ind)
{
	*data++ = (ind - RecLastSource) * sizeof(sVector);
	RecLastSource = ind;

	return data;
}

sU32 *GenMesh::RecDestInd(sU32 *data,sInt ind)
{
	*data++ = (ind - RecLastDest) * sizeof(sVector);
	RecLastDest = ind;

	return data;
}

void GenMesh::RecAddScaleArray(sInt dest,sU32 count,sInt mode,sU32* dptr)
{
	sU32 *data;
	sInt i,j,o,v,flg;

	dest *= VertSize;
	data = RecBegin();
	mode &= VertMask;

	for(j=0;j<=sGMI_LAST;j++)
	{
		flg = 1 << j;

		if (mode & flg)
		{
			o = VertMap[j];
			sVERIFY(o != -1);

			RecOpCount++;
			*data++ = count;

			for(i=0;i<(sInt)count;i++)
			{
				v = dptr[i*2]*VertSize+o;
				*data++ = (v - RecLastSource) * sizeof(sVector);
				RecLastSource = v;
				*data++ = dptr[i*2+1];
			}

			*data++ = (dest + o - RecLastDest) * sizeof(sVector);
			RecLastDest = dest + o;
		}
	}

	RecEnd(data);
}

void GenMesh::RecAddScale(sInt dest,sU32 count,sInt mode,...)
{
	sU32 values[64*2];
	sU32 i;
	sF32 ftemp;

	sVERIFY(count < 64);

	for(i=0;i<count;i++)
	{
		values[i*2+0] = sVARARG(&mode,i*3);
		ftemp = sVARARGF(&mode,i*3+1);
		values[i*2+1] = *((sU32*) &ftemp);
	}

	RecAddScaleArray(dest,count,mode,values);
}

sU32 *GenMesh::RecTransformInner(sU32 *data,sMatrix *matrices)
{
	sVector *src,*dst;
	sU32 count,sj,dj,vert;
	sF32 sx,sy,sz,f;
	sMatrix *mtx;

	count = *data++;
	sj = *data++;
	dj = *data++;
	mtx = matrices + *data++;

  if(dj&0x10)
  {
    dj &= 0x0f;
    f = 1.0f;
  }
  else
    f = 0.0f;

	while (count--)
	{
		vert = *data++;
		src = VertBuf + vert * VertSize + sj;
		dst = VertBuf + vert * VertSize + dj;

		sx = src->x; sy = src->y; sz = src->z;

		dst->x = dst->x*f+sx*mtx->i.x+sy*mtx->j.x+sz*mtx->k.x+src->w*mtx->l.x;
		dst->y = dst->y*f+sx*mtx->i.y+sy*mtx->j.y+sz*mtx->k.y+src->w*mtx->l.y;
		dst->z = dst->z*f+sx*mtx->i.z+sy*mtx->j.z+sz*mtx->k.z+src->w*mtx->l.z;
		dst->w = src->w;
	}

	return data;
}

sU32 *GenMesh::RecBoneInner(sU32 *data,sMatrix *matrices)
{
  sU32 count;
  sVector *vtx;
  sMatrix *mtx;
  sF32 weight,dx,dy,dz;
	sInt i;

  count = *data++;
	while(count--)
	{
    vtx = VertBuf + *data++;
    dx = dy = dz = 0.0f;

    for(i=0;i<4;i++)
    {
      weight = (sF32 &) *data++;
      mtx = matrices + *data++;

			dx+=weight*(vtx->x*mtx->i.x+vtx->y*mtx->j.x+vtx->z*mtx->k.x+mtx->l.x);
			dy+=weight*(vtx->x*mtx->i.y+vtx->y*mtx->j.y+vtx->z*mtx->k.y+mtx->l.y);
			dz+=weight*(vtx->x*mtx->i.z+vtx->y*mtx->j.z+vtx->z*mtx->k.z+mtx->l.z);
		}

    vtx->x = dx;
    vtx->y = dy;
    vtx->z = dz;
	}

	return data;
}

sU32 *GenMesh::RecAddScaleInner(sU32 *code,sMatrix *matrices)
{
	sU32 count,vcount;
	sF32 *src,*dst;
	sF32 sx,sy,sz,sw,wgt;

	count = *code++;
	dst = src = (sF32 *)VertBuf;

#if !sINTRO
  if(sSystem_::CpuMask&sCPU_3DNOW2)
  {
    _asm
    {
	    femms;
	    mov				ebx, [code];
	    mov				esi, [src];
	    mov				edi, esi;
	    mov				edx, [count];
	    or				edx, edx;
	    jz				end;

    vertloop:
	    pxor			mm0, mm0;
	    pxor			mm1, mm1;

	    mov				ecx, [ebx];
	    lea				ebx, [ebx + ecx * 8 + 8];
	    neg				ecx;
	    jz				noverts;

    sumloop:
	    movd			mm2, [ebx + ecx * 8];
	    add				esi, [ebx + ecx * 8 - 4];
	    punpckldq	mm2, mm2;
	    movq			mm3, mm2;
	    pfmul			mm2, [esi];
	    pfmul			mm3, [esi + 8];
	    pfadd			mm0, mm2;
	    pfadd			mm1, mm3;

	    inc				ecx;
	    jnz				sumloop;

    noverts:
	    add				edi, [ebx - 4];

	    movntq		[edi], mm0;
	    movntq		[edi + 8], mm1;

	    dec				edx;
	    jnz				vertloop;

    end:
	    mov				[code], ebx;
	    femms;
    }
  }
  else
#endif
  {
    // So ein bischen reverse engeneering kann mich nicht aufhalten... :-)
    // dein assemblercode ist aber schick geworden, den mußte ich echt drei mal lesen.
	  while (count--)
	  {
		  vcount = *code++;
		  sx = sy = sz = sw = 0.0f;

		  do
		  {
			  src = (sF32 *)(((sU8 *)src)+(*code++));    
			  wgt = *((sF32*) code); code++;
  			
			  sx += wgt * src[0];
			  sy += wgt * src[1];
			  sz += wgt * src[2];
			  sw += wgt * src[3];
		  }
		  while (--vcount);

			dst = (sF32 *)(((sU8 *)dst)+(*code++));    
		  dst[0] = sx;
		  dst[1] = sy;
		  dst[2] = sz;
		  dst[3] = sw;
	  }
  }
	return code;
}


sU32 *GenMesh::RecNormalInner(sU32 *code,sMatrix *matrices)
{
	sInt count,vcount;
	sU8 *verts;
	sVector *vd,*v0,*v1,*v2,nm;
	sVector n,t,t1,t2;
  sF32 t1l,t2l;
  sInt sn,mask,oldcw;

	count = *code++;
	verts = (sU8*) &VertBuf[0];
  sn = VertMap[sGMI_NORMAL];
  mask = *code++;

	__asm
	{
		fstcw		[oldcw];
    finit;
		push		047fh;
		fldcw		[esp];
		pop			eax;
	}

  // calc the normals
  while(count--)
  {
    vcount = *code++;
    vd = (sVector *) (verts + *code++);
    v0 = (sVector *) (verts + *code++);
    v2 = (sVector *) (verts + *code++);
    t2.Sub3(*v2,*v0); t2l = t2.Abs3();
    n.Init4(0.0f,0.0f,0.0f,0.0f);
    t.Init4(0.0f,0.0f,0.0f,0.0f);

    while(--vcount>0)
    {
      t1 = t2; t1l = t2l; v1 = v2;
      v0 = (sVector *) (verts + *code++);
      v2 = (sVector *) (verts + *code++);
      t2.Sub3(*v2,*v0); t2l = t2.Abs3();

      if(mask&1) // normals?
      {
        nm.Cross3(t1,t2);
        if(t1l*t2l>1e-20f)
          n.AddScale3(nm,1.0f/(t1l*t2l));
      }
    }

    if(mask&1)
    {
      n.UnitSafe3();
      vd[sn] = n;
    }
  }

  __asm fldcw [oldcw];

	return code;
}

#if !sINTRO_X
sU32 *GenMesh::RecDisplaceInner(sU32 *code,sMatrix *matrices,sObject **objects)
{
	sInt sn,su,u,v,xm,ym;
	sU16 *height;
	sU32 count;
	sVector *vert;
	GenBitmap *bmp;
	sF32 ampl,tu,tv,h0,h1;

	count = *code++;
	bmp = (GenBitmap *) objects[*code++];
	ampl = *((sF32 *) code); code++;
	ampl /= 32767.0f;

  sn = VertMap[sGMI_NORMAL];
	su = VertMap[sGMI_UV0];
	height = (sU16 *) bmp->Data;
	xm = bmp->XSize - 1;
	ym = bmp->YSize - 1;

	while(count--)
	{
		vert = VertBuf + *code++;

		// get uv and sample texture
		tu=vert[su].x*bmp->XSize; u=tu; tu-=u; if(tu<0.0f) tu+=1.0f; u&=xm;
		tv=vert[su].y*bmp->YSize; v=tv; tv-=v; if(tv<0.0f) tv+=1.0f; v&=ym;

		h0 = height[(v*bmp->XSize+u)*4];
		h0 = h0 + (height[(v*bmp->XSize+((u+1)&xm))*4] - h0) * tu;
		v = (v+1)&ym;
		h1 = height[(v*bmp->XSize+u)*4];
		h1 = h1 + (height[(v*bmp->XSize+((u+1)&xm))*4] - h1) * tu;
		h0 = (h0 + (h1 - h0) * tv) * ampl;

		// perform displacement
    vert->x += vert[sn].x * h0;
    vert->y += vert[sn].y * h0;
    vert->z += vert[sn].z * h0;
	}

	return code;
}
#endif

#if !sINTRO_X
sU32 *GenMesh::RecBevelInner(sU32 *code,sMatrix *matrices)
{
	sU32 count;
	sF32 pull;
	sInt pv,v,nv,vd,sn,i,sj;
	sVector d0,d1,t;
	sVector *vert,*dvert;
	static sInt items[5] = { sGMI_POS,sGMI_UV0,sGMI_UV1,sGMI_UV2,sGMI_UV3 };

	count = *code++;
	pull = *((sF32 *) code); code++;
  sn = VertMap[sGMI_NORMAL];

	while (count--)
	{
		pv = *code++;
		v = *code++;
		nv = *code++;
		vd = *code++;

		vert = &VertBuf[v*VertSize];
		dvert = &VertBuf[vd*VertSize];
		sCopyMem(dvert,vert,sizeof(sVector)*VertSize);

		for(i=0;i<5;i++)
		{
			sj=VertMap[items[i]];
			if(sj!=-1)
			{
				// calculate pull-in vector
				d0.Sub3(VertBuf[pv*VertSize+sj],vert[sj]); d0.UnitSafe3();
				d1.Sub3(VertBuf[nv*VertSize+sj],vert[sj]); d1.UnitSafe3();
				t.Add3(d0,d1);

				if(t.Abs3()<1e-5f)
					t.Cross3(d1,VertNorm(v)); // fixme

				// do it.
				dvert[sj].x += pull*t.x;
				dvert[sj].y += pull*t.y;
				dvert[sj].z += pull*t.z;
			}
		}
	}

	return code;
}
#endif

sU32 *GenMesh::RecPerlinInner(sU32 *code,sMatrix *matrices)
{
//#if !sINTRO_X
	sU32 count;
	sVector amp,*vert;
  sF32 fs[3];
  sInt i,is[3],oldcw,mat;
	sVector t0,t1,t2;

	count = *code++;
	mat = *code++;
  amp = matrices[mat+1].i;
	// single precision, round towards neg. inf.
	__asm
	{
		fstcw		[oldcw];
		push		0143fh;
		fldcw		[esp];
		pop			eax;
	}

	while(count--)
	{
		vert = &VertBuf[*code++ * VertSize];

		// sampling points
		t0.Rotate34(matrices[mat],*vert);
    for(i=0;i<3;i++)
    {
      fs[i] = (&t0.x)[i];
      is[i] = sFtol(fs[i]);
      fs[i] -= is[i];
      is[i] &= 255;
      fs[i] = fs[i]*fs[i]*fs[i]*(10.0f+fs[i]*(6.0f*fs[i]-15.0f));
    }

#define ix is[0]
#define iy is[1]
#define iz is[2]
#define P sPerlinPermute
#define G sPerlinGradient3D

		// trilerp
		t0.Lin3(G[P[P[P[ix]+iy]+iz]&15],G[P[P[P[ix+1]+iy]+iz]&15],fs[0]);
		t1.Lin3(G[P[P[P[ix]+iy+1]+iz]&15],G[P[P[P[ix+1]+iy+1]+iz]&15],fs[0]);
		t0.Lin3(t0,t1,fs[1]);
		t1.Lin3(G[P[P[P[ix]+iy]+iz+1]&15],G[P[P[P[ix+1]+iy]+iz+1]&15],fs[0]);
		t2.Lin3(G[P[P[P[ix]+iy+1]+iz+1]&15],G[P[P[P[ix+1]+iy+1]+iz+1]&15],fs[0]);
		t1.Lin3(t1,t2,fs[1]);
		t0.Lin3(t0,t1,fs[2]);

#undef ix
#undef iy
#undef iz
#undef P
#undef G

		// noise
		vert->AddMul3(t0,amp);
	}

	__asm fldcw [oldcw];
//#endif

	return code;
}

sU32 *GenMesh::RecCopyInner(sU32 *code,sMatrix *matrices,sObject **objects)
{
	sU32 count;
	GenMesh *srcm;
	sVector *src,*dst;

	count = *code++;
	srcm = (GenMesh *) objects[*code++];

	while(count--)
	{
		dst = &VertBuf[*code++];
		src = &srcm->VertBuf[*code++];
		sCopyMem4((sU32*)dst,(sU32*)src,*code++ * 4);
	}

	return code;
}

sU32 *GenMesh::RecF2VNInner(sU32 *code,sMatrix *matrices)
{
  sU32 count;
  sInt v0,v1,v2;
  sInt sn;

  sn = VertMap[sGMI_NORMAL];
  count = *code++;
  while(count--)
  {
    v0 = *code++;
    v1 = *code++;
    v2 = *code++;
    CalcNormal(VertBuf[v1+sn],v0,v1,v2);
    VertBuf[v1+sn].UnitSafe3();
  }

  return code;
}

sU32 *GenMesh::RecEWKInner(sU32 *code,sMatrix *matrices)
{
  sInt gcount,count,i;
  sF32 ic,dist,vals[3];
  sBool local;
  sInt mtx,sn,vs;
  sVector *vb,*vd,t,sum;
  sMatrix mat;
  
  sn = VertMap[sGMI_NORMAL];
  gcount = *code++;
  local = *code++;
  mtx = *code++;
  vb = VertBuf;
  vs = VertSize;

  ic = 1.0f / matrices[mtx].j.w;
  dist = matrices[mtx].i.w * ic;
  for(i=0;i<3;i++)
    vals[i] = (&matrices[mtx].j.x)[i] * ic;

  mat.InitEulerPI2(vals);

  while(gcount--)
  {
    count = *code++;
    
    // first pass: find middle
    sum.Init(0,0,0,0);
    if(local)
    {
      for(i=0;i!=count;i++)
      {
        vd = VertBuf + code[i]*vs;
        sum.x += vd->x + vd[sn].x*dist;
        sum.y += vd->y + vd[sn].y*dist;
        sum.z += vd->z + vd[sn].z*dist;
      }
    }

    sum.Scale3(1.0f / count);

    // second pass: transform
    for(i=0;i!=count;i++)
    {
      vd = VertBuf + code[i]*vs;
      t.x = (vd->x + vd[sn].x*dist - sum.x) * matrices[mtx].i.x + sum.x;
      t.y = (vd->y + vd[sn].y*dist - sum.y) * matrices[mtx].i.y + sum.y;
      t.z = (vd->z + vd[sn].z*dist - sum.z) * matrices[mtx].i.z + sum.z;

      vd->Rotate3(mat,t);
      vd[sn].Rotate3(mat);
    }

    code += count;
  }

  return code;
}

/****************************************************************************/
/****************************************************************************/

GenMeshFace *GenMesh::GetFace(sU32 i)
{
  return &Face.Array[GetEdge(i)->Face[i&1]];
}

GenMeshFace *GenMesh::GetFaceI(sU32 i)
{
  return &Face.Array[GetEdge(i)->Face[~i&1]];
}

GenMeshVert *GenMesh::GetVert(sU32 i)
{
  return &Vert.Array[GetEdge(i)->Vert[i&1]];
}

sInt GenMesh::GetFaceId(sU32 i)
{
  return GetEdge(i)->Face[i&1];
}

sInt GenMesh::GetVertId(sU32 i)
{
  return GetEdge(i)->Vert[i&1];
}

sInt GenMesh::NextFaceEdge(sU32 i)
{
  return GetEdge(i)->Next[i&1];
}

sInt GenMesh::PrevFaceEdge(sU32 i)
{
  return GetEdge(i)->Prev[i&1];
}

sInt GenMesh::NextVertEdge(sU32 i)
{
  return GetEdge(i)->Prev[i&1]^1;
}

sInt GenMesh::PrevVertEdge(sU32 i)
{
  return GetEdge(i)->Next[~i&1];
}

#if !sPLAYER

sInt GenMesh::GetValence(sU32 e)
{
	sInt v,k;
	sU32 ee;

	v = GetVertId(e); k = 0; ee = e;
	do
	{
		if(GetVertId(e)==v)
			k++;

		e = NextVertEdge(e);
	}
	while(e!=ee);

	return k;
}

sInt GenMesh::GetDegree(sU32 f)
{
	sInt e,ee,i;

	e = ee = Face[f].Edge;
	i = 0;
	do
	{
		i++;
		e = NextFaceEdge(e);
	}
	while(e!=ee);

	return i;
}

#endif

sInt GenMesh::AddVert()
{
  sInt c;

  c = Vert.Count; 
  Realloc(c+1);
  Vert.AtLeast(c+1); 
  Vert.Count++;

  return c;
}

sInt GenMesh::AddNewVert()
{
  sInt v;

  v = AddVert();
  Vert[v].Init();
  Vert[v].First = Vert[v].Next = Vert[v].ReIndex = v;

  return v;
}

sInt GenMesh::AddCopiedVert(sInt src)
{
  sInt v;

  v = AddVert();
  Vert[v] = Vert[src];
  Vert[v].ReIndex = v;
  
  return v;
}

void GenMesh::SplitFace(sU32 i0,sU32 i1,sU32 dmask1,sU32 dmask0)
{
  sInt ee,f0,f1,p0,p1;
  sU32 i;

  sVERIFY(GetFaceId(i0)==GetFaceId(i1));
	sVERIFY(i0!=i1);

  p0 = PrevFaceEdge(i0);
  p1 = PrevFaceEdge(i1);
  f0 = GetFaceId(i0);
  ee = Edge.Count; Edge.AtLeast(ee+1); Edge.Count++;
  f1 = Face.Count; Face.AtLeast(f1+1); Face.Count++;

  register GenMeshEdge *Edge = this->Edge.Array;
  register GenMeshFace *Face = this->Face.Array;
  Edge[ee] = Edge[i0/2];
	Edge[ee].Crease = 0; // kill crease flags
  Edge[ee].Sel(dmask1,dmask0);
  Edge[ee].Vert[0] = GetVertId(i1);
  Edge[ee].Vert[1] = GetVertId(i0);
  Edge[ee].Face[0] = f0;
  Edge[ee].Face[1] = f0;
  Edge[ee].Next[0] = i0;
  Edge[ee].Next[1] = i1;
  Edge[ee].Prev[0] = p1;
  Edge[ee].Prev[1] = p0;
  Edge[ee].Temp[0] = -1;
  Edge[ee].Temp[1] = -1;

  Edge[i0>>1].Prev[i0&1] = ee*2;
  Edge[p0>>1].Next[p0&1] = ee*2+1;
  Edge[i1>>1].Prev[i1&1] = ee*2+1;
  Edge[p1>>1].Next[p1&1] = ee*2;

  Face[f1] = Face[f0];
  Face[f1].Sel(dmask1,dmask0);
  Face[f0].Edge = i0;
  Face[f1].Edge = i1;

  i = i1;
  do
  {
    sVERIFY(GetFaceId(i)==f0);
		sVERIFY(NextFaceEdge(PrevFaceEdge(i)) == i);
		sVERIFY(PrevFaceEdge(NextFaceEdge(i)) == i);
    Edge[i/2].Face[i&1] = f1;
    i = NextFaceEdge(i);
  }
  while(i!=i1);

#if !sPLAYER
 //just checking!
  i = i0;
  do
  {
    sVERIFY(GetFaceId(i)==f0);
		sVERIFY(NextFaceEdge(PrevFaceEdge(i)) == i);
		sVERIFY(PrevFaceEdge(NextFaceEdge(i)) == i);
    i = NextFaceEdge(i);
  }
  while(i!=i0);
#endif
}

void GenMesh::SplitBridge(sU32 a,sU32 d,sInt &va,sInt &vb,sU32 dmask1,sU32 dmask0)
{
  sInt e,p0,p1,n0,n1;
  sInt v0,v1,cf,i,lc;

  v1 = GetVertId(a);
  sVERIFY(Vert[v1].First == GetVert(d)->First);

  e = Edge.Count; Edge.AtLeast(e+1); Edge.Count = e+1;
  v0 = va;

  register GenMeshVert *vert = this->Vert.Array;   // AddVert()!!!
  vert[v0].Sel(dmask1,dmask0);
  vert[v0].First = v0;
  vert[v0].Next = v0;

  a = a^1;
  p0 = PrevFaceEdge(d);
  n1 = NextFaceEdge(a);

  cf=0; // cumulative crease flag
  if(n1==d)
  {
    p0 = e*2+1;
    n1 = e*2;
  }
  else
  {
    for(i=n1;i!=d;i=PrevVertEdge(i))
    {
      if(Edge[i/2].Crease)
      {
        cf|=Edge[i/2].Crease;
        lc=i;
      }
    }
  }

  register GenMeshEdge *Edge = this->Edge.Array;
  Edge[e] = Edge[a/2];
  Edge[e].Crease = cf;
  Edge[e].Sel(dmask1,dmask0);
  Edge[e].Vert[0] = v0;
  Edge[e].Vert[1] = v1;
  Edge[e].Face[0] = GetFaceId(d);
  Edge[e].Face[1] = GetFaceId(a);
  Edge[e].Next[0] = n0 = d;
  Edge[e].Next[1] = n1;
  Edge[e].Prev[0] = p0;
  Edge[e].Prev[1] = p1 = a;
  Edge[e].Temp[0] = -1;
  Edge[e].Temp[1] = -1;

  Edge[n0/2].Prev[n0&1] = Edge[p0/2].Next[p0&1] = e*2;
  Edge[n1/2].Prev[n1&1] = Edge[p1/2].Next[p1&1] = e*2+1;

  if(cf)
  {
    v0 = GetVertId(lc);
    v1 = AddCopiedVert(v0);
    Vert[v1].Sel(dmask1,dmask0);    // not register vert!)
    Edge[lc/2].Vert[lc&1] = v1;
  }

  FixVertCycle(e*2);
  FixVertCycle(d);

  va = GetVertId(e*2);
  vb = GetVertId(n1);
}

sInt GenMesh::AddEdge(sInt v0,sInt v1,sInt face)
{
  sInt i;
  sInt e;

  for(i=0;i<Edge.Count;i++)
  {
    if(Edge[i].Face[1]==-1)
    {
      if(Vert[Edge[i].Vert[0]].First==Vert[v1].First && Vert[Edge[i].Vert[1]].First==Vert[v0].First)
      {
        Edge[i].Vert[1] = v0;
        Edge[i].Face[1] = face;
        Face[face].Edge = i*2+1;
        return i*2+1;
      }
      if(Vert[Edge[i].Vert[0]].First==Vert[v1].First && Vert[Edge[i].Vert[1]].First==Vert[v0].First)
      {
        sVERIFYFALSE;
      }
    }
  }
  e = Edge.Count;
  Edge.AtLeast(e+1);
  Edge.Count = e+1;
  Edge[e].Init();
  Edge[e].Vert[0] = v0;
  Edge[e].Vert[1] = v1;
  Edge[e].Face[0] = face;
  Edge[e].Face[1] = -1;
  Face[face].Edge = e*2;
  return e*2;
}

void GenMesh::MakeFace(sInt face,sInt nedges,...)
{
	sInt i,t,e;
	sInt *edges;

	edges = &nedges + 1;
	Face[face].Init();
	Face[face].Edge = edges[0];
	t = edges[nedges-1];
	for(i=0;i<nedges;i++)
	{
		e=edges[i];
		sVERIFY(Edge[e/2].Face[e&1]==-1 || Face[GetFaceId(e)].Material==0);
		Edge[t/2].Next[t&1]=e;
		Edge[e/2].Prev[e&1]=t;
		Edge[e/2].Face[e&1]=face;
		t=e;
	}
}

#if !sINTRO
void GenMesh::Verify()
{
  sInt i,e,ee;
  sInt ec,fc,vc;

  ec = Edge.Count*2;
  vc = Vert.Count;
  fc = Face.Count;

// check face circles

  for(i=0;i<fc;i++)
  {
		if(Face[i].Material==0)
			continue;
    sVERIFY(Face[i].Edge>=0 && Face[i].Edge<ec);
    ee = e = Face[i].Edge;
    do
    {
      sVERIFY(GetFaceId(e)==i);
      e = NextFaceEdge(e);
    }
    while(ee!=e);
    ee = e = Face[i].Edge;
    do
    {
      sVERIFY(GetFaceId(e)==i);
      e = PrevFaceEdge(e);
    }
    while(ee!=e);
  }

// check vertices

  for(i=0;i<vc;i++)
  {
    sVERIFY(Vert[i].Next>=0 && Vert[i].Next<vc);
    sVERIFY(Vert[Vert[i].First].First == Vert[i].First);
  }

// check edges

  for(i=0;i<Edge.Count;i++)
  {
    sVERIFY(Edge[i].Next[0]>=0 && Edge[i].Next[0]<ec);
    sVERIFY(Edge[i].Next[1]>=0 && Edge[i].Next[1]<ec);
    sVERIFY(Edge[i].Prev[0]>=0 && Edge[i].Prev[0]<ec);
    sVERIFY(Edge[i].Prev[1]>=0 && Edge[i].Prev[1]<ec);
    sVERIFY(Edge[i].Face[0]>=0 && Edge[i].Face[0]<fc);
    sVERIFY(Edge[i].Face[1]>=0 && Edge[i].Face[1]<fc);
    sVERIFY(Edge[i].Vert[0]>=0 && Edge[i].Vert[0]<vc);
    sVERIFY(Edge[i].Vert[1]>=0 && Edge[i].Vert[1]<vc);
    sVERIFY(GetVert(i*2+1)->First == GetVert(NextFaceEdge(i*2  )  )->First);
    sVERIFY(GetVert(i*2  )->First == GetVert(NextFaceEdge(i*2+1)  )->First);
    sVERIFY(GetVert(i*2  )->First == GetVert(PrevFaceEdge(i*2  )^1)->First);
    sVERIFY(GetVert(i*2+1)->First == GetVert(PrevFaceEdge(i*2+1)^1)->First);

    // the following test makes sense, but is currently out because ReadCompact
    // produces slightly broken meshes (no crease, yet double vertices nonetheless)
    /*if(!Edge[i].Crease)
    {
      sVERIFY(GetVertId(i*2  ) == GetVertId(PrevVertEdge(i*2  )));
      sVERIFY(GetVertId(i*2+1) == GetVertId(PrevVertEdge(i*2+1)));
    }*/
  }
}
#endif

void GenMesh::ReIndex()
{
  sInt i,vc,ec;

  vc = Vert.Count;
  for(i=0;i<vc;i++)
  {
    Vert[i].First = Vert[Vert[i].First].ReIndex;
    Vert[i].Next = Vert[Vert[i].Next].ReIndex;
  }

  ec = Edge.Count;
  for(i=0;i<ec;i++)
  {
    Edge[i].Vert[0] = Vert[Edge[i].Vert[0]].ReIndex;
    Edge[i].Vert[1] = Vert[Edge[i].Vert[1]].ReIndex;
  }
}

sBool GenMesh::IsBorderEdge(sU32 i,sInt mode)
{
  sBool s0,s1;

  s0 = GetFace(i)->Select;
  s1 = GetFaceI(i)->Select;
  if((mode==1 && (Edge[i/2].Crease & sGMF_NORMALS)) || mode>=2)
    s1 = 0;

  return s0 && !s1;
}

/****************************************************************************/
/****************************************************************************/

void GenMesh::Mask2Sel(sU32 mask)
{
  sInt i;

  if(mask&0x00ff0000)
    for(i=0;i<Vert.Count;i++)
      Vert[i].Select = ((Vert[i].Mask & (mask>>16))!=0);
  if(mask&0x0000ff00)
    for(i=0;i<Face.Count;i++)
      Face[i].Select = ((Face[i].Mask & (mask>> 8))!=0);
#if !sINTRO_X
  if(mask&0x000000ff)
    for(i=0;i<Edge.Count;i++)
      Edge[i].Select = ((Edge[i].Mask & (mask    ))!=0);
#endif
}

void GenMesh::All2Sel(sBool sel)
{
  sInt i;

  for(i=0;i<Vert.Count;i++)
    Vert[i].Select = sel;
  for(i=0;i<Face.Count;i++)
    Face[i].Select = sel && Face[i].Material;
#if !sINTRO_X
  for(i=0;i<Edge.Count;i++)
    Edge[i].Select = sel;
#endif
}

void GenMesh::Id2Mask(sU32 mask,sInt id)
{
#if !sINTRO
  sFatal("not implememted");
#endif
}

void GenMesh::Mask2Mask(sU32 mask,sU32 dmask1,sU32 dmask0)
{
  sInt i;

  if(mask&0x00ff0000)
    for(i=0;i<Vert.Count;i++)
      if(Vert[i].Mask & (mask>>16))
        Vert[i].Sel(dmask1,dmask0);
  if(mask&0x0000ff00)
    for(i=0;i<Face.Count;i++)
      if(Face[i].Mask & (mask>> 8))
        Face[i].Sel(dmask1,dmask0);
#if !sINTRO_X
  if(mask&0x000000ff)
    for(i=0;i<Edge.Count;i++)
      if(Edge[i].Mask & (mask    ))
        Edge[i].Sel(dmask1,dmask0);
#endif
}

void GenMesh::Sel2Mask(sU32 dmask1,sU32 dmask0)
{
  sInt i;
  sU32 mask = dmask1|dmask0;

  if(mask&0x00ff0000)
    for(i=0;i<Vert.Count;i++)
      if(Vert[i].Select)
        Vert[i].Sel(dmask1,dmask0);
  if(mask&0x0000ff00)
    for(i=0;i<Face.Count;i++)
      if(Face[i].Select)
        Face[i].Sel(dmask1,dmask0);
#if !sINTRO_X
  if(mask&0x000000ff)
    for(i=0;i<Edge.Count;i++)
      if(Edge[i].Select)
        Edge[i].Sel(dmask1,dmask0);
#endif
}

void GenMesh::All2Mask(sU32 dmask1,sU32 dmask0)
{
  sInt i;
  sU32 mask = dmask1|dmask0;

  for(i=0;i<Vert.Count;i++)
  {
    if(mask&0x00ff0000)
      Vert[i].Sel(dmask1,dmask0);
    Vert[i].Select = 1;
  }
  for(i=0;i<Face.Count;i++)
  {
		if(!Face[i].Material)
			continue;
    if(mask&0x0000ff00)
      Face[i].Sel(dmask1,dmask0);
    Face[i].Select = 1;
  }
#if !sINTRO_X
  for(i=0;i<Edge.Count;i++)
  {
    if(mask&0x000000ff)
      Edge[i].Sel(dmask1,dmask0);
    Edge[i].Select = 1;
  }
#endif
}

void GenMesh::Face2Vert()
{
  sInt i,e,ee;

  for(i=0;i<Vert.Count;i++)
    Vert[i].Select = 0;
  for(i=0;i<Face.Count;i++)
  {
    if(Face[i].Select)
    {
      e = ee = Face[i].Edge;
      do
      {
        GetVert(e)->Select = 1;
        e = NextFaceEdge(e);
      }
      while(e!=ee);
    }
  }
}

void GenMesh::Edge2Vert(sInt uvs)
{
	sInt i,j;

  j = VertMap[sGMI_UV0];
	for(i=0;i<Edge.Count*2;i++)
	{
		if(Edge[i/2].Select)
		{
      if((&VertBuf[GetVertId(PrevFaceEdge(i))*VertSize+j].x)[uvs]>0.5f)
      {
        GetVert(i)->Select=1;
        GetVert(NextFaceEdge(i))->Select=1;
      }
		}
	}
}

void GenMesh::SetMaterial(sInt id,GenMaterial *mat)
{
  sInt i,o;

	sVERIFY(id != 0);
  o = Mtrl.Count;
  Mtrl.AtLeast(id+1);
  for(i=o;i<=id;i++)
    Mtrl[i].Init();

  Mtrl.Count = sMax(Mtrl.Count,id+1);
  Mtrl[id].Material = mat;
  for(i=0;i<GENMAT_MAXPASS;i++)
    Mtrl[id].Pass[i].Reset();
  for(i=0;i<Face.Count;i++)
  {
    if(Face[i].Select)
      Face[i].Material = id;
  }
}

sInt GenMesh::FindCrease(sInt edge,sInt mask)
{
	sInt ee;

	ee = edge;
	do
	{
    if(Edge[edge/2].Crease & mask)
      ee = edge;
    else
      edge = PrevVertEdge(edge);
	}
	while(ee!=edge);

	return edge;
}

void GenMesh::CalcNormal(sVector &n,sInt o0,sInt o1,sInt o2)
{
  sVector t1,t2;

  t1.Sub3(VertBuf[o1],VertBuf[o0]);
  t2.Sub3(VertBuf[o2],VertBuf[o0]);
  n.Cross3(t1,t2);
}

void GenMesh::CalcFaceNormal(sVector &n,sInt f)
{
  sInt e;

  e = Face[f].Edge;
  CalcNormal(n,GetVertId(PrevFaceEdge(e))*VertSize,GetVertId(e)*VertSize,
    GetVertId(NextFaceEdge(e))*VertSize);
}

/****************************************************************************/
/****************************************************************************/

void GenMesh::TransVert(sMatrix &mat,sInt sj,sInt djj)
{
  sU32 *data;
  sInt i,dj;

  sj = VertMap[sj]; if(sj==-1) return;
  dj = VertMap[djj&0x0f]; if(dj==-1) return;
  dj |= djj&0x10;

  data = RecBegin(IM_XFORM);
	*data++ = sj;
	*data++ = dj;
  *data++ = RecMatrix(mat);

  for(i=0;i<Vert.Count;i++)
  {
    if(Vert[i].Select)
    {
      *data++ = i;
			RecOpCount++;
    }
  }

  RecEnd(data);
}

/****************************************************************************/

void GenMesh::SelectCube(const sVector &vc,const sVector &vs,sBool set,sU32 dmask1,sU32 dmask0)
{
  sInt i,e,ee;
  sBool all;

  sVERIFY(RecMode==0);

  for(i=0;i<Vert.Count;i++)
  {
    Vert[i].Temp = 0;
    if(sFAbs(VertPos(i).x-vc.x)<=vs.x &&
       sFAbs(VertPos(i).y-vc.y)<=vs.y &&
       sFAbs(VertPos(i).z-vc.z)<=vs.z)
    {
      Vert[i].Sel(dmask1,dmask0);
      Vert[i].Select = 1;
      Vert[i].Temp = 1;
    }
    else if(set)
    {
      Vert[i].Sel(0,dmask1);
      Vert[i].Select = 0;
    }
  }

#if !sINTRO_X
  for(i=0;i<Edge.Count;i++)
  {
    if(Vert[Edge[i].Vert[0]].Temp && Vert[Edge[i].Vert[1]].Temp)
    {
      Edge[i].Sel(dmask1,dmask0);
      Edge[i].Select = 1;
    }
    else if(set)
    {
      Edge[i].Sel(0,dmask1);
      Edge[i].Select = 0;
    }
  }
#endif
  
  for(i=0;i<Face.Count;i++)
  {
		if(!Face[i].Material)
			continue;
    e = ee = Face[i].Edge;
    all = sTRUE;
    do
    {
      if(!GetVert(e)->Temp)
        all = 0;
      e = NextFaceEdge(e);
    }
    while(ee!=e && all);

    if(all)
    {
      Face[i].Sel(dmask1,dmask0);
      Face[i].Select = 1;
    }
    else if(set)
    {
      Face[i].Sel(0,dmask1);
      Face[i].Select = 0;
    }
  }
}

/****************************************************************************/

void GenMesh::Ring(sInt count,sU32 dmask,sF32 radius,sF32 phase)
{
  sInt i,in,ip;
  sInt sj;
  sVector *vp;
  GenMeshEdge *ed;

  sVERIFY(RecMode==0);

  Vert.AtLeast(count);
  Edge.AtLeast(count);  Edge.Count=count;
  Face.AtLeast(2);      Face.Count=2;

  Face[0].Init();
  Face[0].Edge = 0;
  Face[1].Init();
  Face[1].Edge = 1;

  sj = VertMap[sGMI_UV0];

  for(i=0;i<count;i++)
  {
    AddNewVert();
    vp = &VertBuf[i*VertSize];
    vp->x = sFSin(phase+i*sPI2F/count)*radius;
    vp->y = -sFCos(phase+i*sPI2F/count)*radius;
    vp->w = 1.0f;
    if(sj!=-1)
    {
      vp[sj].x = i*1.0f/count;
      vp[sj].w = 1.0f;
    }

    in=(i+1)%count;
    ip=(i+count-1)%count;

    ed = &Edge[i];
    ed->Init();
    ed->Next[0] = 2*in;
    ed->Next[1] = 2*ip+1;
    ed->Prev[0] = 2*ip;
    ed->Prev[1] = 2*in+1;
    ed->Face[0] = 0;
    ed->Face[1] = 1;
    ed->Vert[0] = i;
    ed->Vert[1] = in;
  }

  All2Mask(dmask);
  All2Sel(0);
}

/****************************************************************************/

void GenMesh::Extrude(sU32 dmask1,sU32 dmask0,sInt mode)
{
  sInt i,e,ee;
  sInt ec,fc;
  sU32 i0,i1,*data;
  sInt f,va,vb;

  RecBegin(IM_ADDSCL);

  ec = Edge.Count;
	fc = Face.Count;

  for(i=0;i<ec*2;i++)
  {
    Edge[i/2].Temp[i&1] = -1;
    if(IsBorderEdge(i,mode))
    {
      Edge[i/2].Temp[i&1] = 0;
      e = i;
      do
      {
        e = NextVertEdge(e);
      }
      while(!IsBorderEdge(e^1,mode));

      va = AddCopiedVert(GetVertId(i));
      SplitBridge(e,i,va,vb,dmask1,dmask0);
      RecAddScale(va,1,RM_ALL,GetVertId(i),1.0f);
    }
  }

  for(i=0;i<ec*2;i++)
  {
		if(!Edge[i/2].Temp[i&1])
		{
			i0 = PrevFaceEdge(i);
			i1 = NextFaceEdge(i);
			i1 = NextFaceEdge(i1);
			f = Face.Count;
			SplitFace(i1,i0,dmask1,dmask0);
			Face[f].Select = 0;
		}
  }

  RecEnd();

  if(mode==3)
  {
    data = RecBegin(IM_F2VN);

    for(i=0;i<Face.Count;i++)
    {
      if(Face[i].Select)
      {
        e = ee = Face[i].Edge;
        do
        {
          RecOpCount++;
          *data++ = GetVertId(PrevFaceEdge(e)) * VertSize;
          *data++ = GetVertId(e) * VertSize;
          *data++ = GetVertId(NextFaceEdge(e)) * VertSize;
          e = NextFaceEdge(e);
        }
        while(e!=ee);
      }
    }

  	RecEnd(data);
  }
}

/****************************************************************************/

void GenMesh::Subdivide(sF32 alpha)
{
  sInt i,j,k,count,e,ee,c,v,vv;
  sInt va0,va1,vb0,vb1,va,vb;
  sInt ec,fc,ovc;
  sInt s0,s1;
	sU32 Temp[64*2+2];
	sF32 ftemp;

  fc = Face.Count;

	RecBegin(IM_ADDSCL);
	ovc = Vert.Count;

// face.temp    = first new edge
// face.temp2   = center
// edge.temp[0]
// edge.temp[1]
// vert.temp    = duplicated vertex for storing old value
// vert.temp2   = vertex->edge ptr

  for(i=0;i<Vert.Count;i++)
  {
    Vert[i].Temp = -1;
    Vert[i].ReIndex = i;
  }

// calculate center positions

  fc = Face.Count;
  for(i=0;i<fc;i++)
  {
    Face[i].Temp = -1;
    Face[i].Temp2 = -1;
    if(Face[i].Select)
    {
      count=0;
      ee = e = Face[i].Edge;

			do
			{
				v = GetVertId(e);
				Temp[count*2]=v;

				if (Vert[v].Temp == -1)
				{
          vv = AddCopiedVert(v);
					Vert[v].Temp = vv;
					Vert[v].Temp2 = e;
					Vert[v].ReIndex = vv;
				}

				e = NextFaceEdge(e);
				count++;
			} while (e != ee);

			Face[i].Temp2 = c = AddNewVert();
			Vert[c].Select = 1;

			ftemp = 1.0f/count;
			for(j=0;j<count;j++)
				Temp[j*2+1] = *((sU32*) &ftemp);

			RecAddScaleArray(c,count,RM_POS|RM_REST,Temp);
    }
  }

// even vertices
	alpha *= 4.0f;
	for(i=0;i<ovc;i++)
	{
		if(Vert[i].Temp!=-1)
		{
			vv = Vert[i].Temp;
			k = 0;
			ee = Vert[i].Temp2;
			e = ee;
			ec = 0; // edge crease flags
			c = -1;

			do
			{
				Temp[k*2] = GetVertId(e^1);
				k++;

				if(GetFace(e)->Temp2!=-1)
				{
					Temp[k*2] = GetFace(e)->Temp2;
					k++;
				}
				else
					c = e;

				if (GetVertId(e) == i)
					ec |= Edge[e/2].Crease;

				e = NextVertEdge(e);
			}
			while (ee != e);

			if(c==-1)
			{
				ftemp = alpha / (k * k);
				for (j=0;j<k;j++)
					Temp[j*2+1] = *((sU32*) &ftemp);

				Temp[k*2] = i;
				ftemp = (k - alpha) / k;
				Temp[k*2+1] = *((sU32*) &ftemp);

				RecAddScaleArray(vv,k+1,~ec & ~RM_NORMAL,Temp);
				RecAddScale(vv,1,ec & ~RM_NORMAL,i,1.0f);
			}
			else
			{
				va = GetVertId(c^1);
				do
				{
					c = NextVertEdge(c);
				}
				while (GetFaceI(c)->Temp2!=-1);
				vb = GetVertId(c^1);

				ftemp = alpha/32.0f;
				RecAddScale(vv,3,RM_POS|RM_REST,i,1.0f-2.0f*ftemp,va,ftemp,vb,ftemp);
			}
		}
	}

// split edges
	alpha /= 16.0f;
	ftemp = 0.5f - alpha;

  ec = Edge.Count;
  for(i=0;i<ec*2;i+=2)
  {
    // calc center position
    va0 = GetVertId(i);
    va1 = GetVertId(NextFaceEdge(i));
    vb1 = GetVertId(i^1);
    vb0 = GetVertId(NextFaceEdge(i^1));

    // split if select

    s0 = GetFace(i)->Select;
    s1 = GetFaceI(i)->Select;
    if(s0 || s1)
    {
      // split edge
      va = AddNewVert();
      SplitBridge(NextVertEdge(i^1),PrevVertEdge(i^1),va,vb,0x100000,0);

			// calc position
			if(GetFace(i)->Temp2==-1 || GetFaceI(i)->Temp2==-1)
      {
				RecAddScale(va,2,RM_POS|RM_REST,va0,0.5f,va1,0.5f);
				if (va != vb)
					RecAddScale(vb,2,RM_POS|RM_REST,vb0,0.5f,vb1,0.5f);
      }
      else
      {
				if(va != vb)
				{
					j = Edge[i/2].Crease; // mask out pos

					RecAddScale(va,4,~j & ~RM_NORMAL,va0,ftemp,va1,ftemp,
						Face[Edge[i/2].Face[0]].Temp2,alpha,Face[Edge[i/2].Face[1]].Temp2,alpha);
					RecAddScale(va,2,j & ~RM_NORMAL,va0,0.5f,va1,0.5f);
					RecAddScale(vb,4,~j & ~RM_NORMAL,vb0,ftemp,vb1,ftemp,
						Face[Edge[i/2].Face[0]].Temp2,alpha,Face[Edge[i/2].Face[1]].Temp2,alpha);
					RecAddScale(vb,2,j & ~RM_NORMAL,vb0,0.5f,vb1,0.5f);
				}
				else
				{
					RecAddScale(va,4,RM_POS|RM_REST,va0,ftemp,va1,ftemp,
						Face[Edge[i/2].Face[0]].Temp2,alpha,Face[Edge[i/2].Face[1]].Temp2,alpha);
				}
      }

      // store link to correct edge for face generation

      if(s0)
        GetFace(i)->Temp = i;
      if(s1)
        GetFaceI(i)->Temp = PrevFaceEdge(i^1);
    }
  }

// create faces
  fc = Face.Count;
  for(i=0;i<fc;i++)
  {
		if(Face[i].Select && Face[i].Material!=0)
    {
      sVERIFY(Face[i].Temp!=-1);
      sVERIFY(Face[i].Temp2!=-1);
      e = Face[i].Temp;

      count=0;
      ee = e;
      do
      {
        ee = NextFaceEdge(ee);
        sVERIFY(count<64);
        Temp[count++] = ee;
        ee = NextFaceEdge(ee);
      }
      while(e!=ee);

      va = Face[i].Temp2;
      SplitBridge(NextVertEdge(Temp[0]),Temp[0],va,vb,0,0);
      for(j=1;j<count;j++)
        SplitFace(PrevFaceEdge(PrevFaceEdge(PrevFaceEdge(Temp[j]))),Temp[j]);
    }
  }

  ReIndex();
  RecEnd();
}

/****************************************************************************/

sInt GenMesh::Bones(sF32 phase)
{
	sU32 *data;
  sF32 wgt;
  sInt i,j,mi,lm=0;

	data = RecBegin(IM_BONE);

  sVERIFY(RBIndex+BoneMatrix.Count<1024);
  BonesModify(-1,phase);
  mi = RBIndex;
  RBIndex += BoneMatrix.Count;

  for(i=0;i<Vert.Count;i++)
  {
    if(Vert[i].Matrix[0]!=0xff)
    {
      RecOpCount++;
      *data++ = i * VertSize;
			for(j=0;j<4;j++)
			{
				if(Vert[i].Matrix[j]!=0xff)
				{
          wgt = Vert[i].Weight[j] / 255.0f;
          lm = Vert[i].Matrix[j] + mi;
        }
        else
          wgt = 0.0f;

        *data++ = *((sU32 *) &wgt);
        *data++ = lm;
			}
    }
  }

  RecEnd(data);
  return mi;
}

void GenMesh::BonesModify(sInt matrix,sF32 phase)
{
  sMatrix *mp;
  sInt i;
  sInt ki0,ki1;
  sF32 f1,val;
  sMatrix mat;
  GenMeshCurve *cur;

  if(matrix<0)
  {
    mp = &RBMat[RBIndex];
    sVERIFY(CurveCount==BoneCurve.Count)
  }
  else
  {
    mp = &RecMat.Array[matrix];
    sVERIFY(matrix+BoneMatrix.Count <= RecMat.Count);
  }

  if(KeyCount>0 && BoneCurve.Count>0)
  {
    i = (Anim0 + phase*(Anim1-Anim0))*1024;
    ki0 = (i/1024)%KeyCount;
    ki1 = (i/1024+1)%KeyCount;
    i &= 1023;
    f1 = i/1024.0f;

    cur = BoneCurve.Array;
    for(i=0;i<BoneCurve.Count;i++)
    {
      val = KeyBuf[i*KeyCount+ki0] + (KeyBuf[i*KeyCount+ki1]-KeyBuf[i*KeyCount+ki0]) * f1;
      BoneMatrix[cur->Matrix].TransSRT[cur->Curve] = val;

      cur++;
    }
  }

  BoneMatrix[0].Matrix.InitSRT(BoneMatrix[0].TransSRT);
  mat.InitSRTInv(BoneMatrix[0].BaseSRT);
  mp[0].Mul4(mat,BoneMatrix[0].Matrix);
  
  for(i=1;i<BoneMatrix.Count;i++)
  {
    mat.InitSRT(BoneMatrix[i].TransSRT);
    BoneMatrix[i].Matrix.Mul4(mat,BoneMatrix[BoneMatrix[i].Parent].Matrix);
    mat.InitSRTInv(BoneMatrix[i].BaseSRT);
    mp[i].Mul4(mat,BoneMatrix[i].Matrix);
  }

  for(i=0;i<BoneMatrix.Count;i++)
  {
    mp[i].i.z = -mp[i].i.z;
    mp[i].j.z = -mp[i].j.z;
    mp[i].k.z = -mp[i].k.z;
    mp[i].l.z = -mp[i].l.z;
  }
}

/****************************************************************************/

void GenMesh::FixVertCycle(sInt i)
{
  sInt v,v0,vn,e;

  v0 = v = GetVertId(i);
  Vert[v].First = v0;
  Vert[v].Next = v0;
  e = i;

  do
  {
    Edge[e/2].Vert[e&1] = v;
    e = NextVertEdge(e);
    vn = GetVertId(e);

    if(Edge[e/2].Crease && vn!=v)
    {
      Vert[v].Next = vn;
      v = vn;
      Vert[v].First = v0;
      Vert[v].Next = v0;
    }
  }
  while(e!=i);
}

void GenMesh::SingleCrease(sU32 i,sU32 dmask1,sU32 dmask0,sInt what)
{
  sInt j,k,e,got,v0,v,vf;

  sVERIFY(!(what & 1));
	if(!Edge[i/2].Crease)
	{
		for(j=0;j<2;j++)
		{
			k = i^j;
			v0 = GetVertId(k);
			vf = Vert[v0].First;
      
      got = 0;
      e = k;
      do
      {
        got |= Edge[e/2].Crease;
        e = NextVertEdge(e);
      }
      while(e!=k);

			if(got) // already atleast one crease on this vertex
			{
        v = AddCopiedVert(v0);
				Vert[v].Sel(dmask1,dmask0);
				Vert[v].First = vf;
				Vert[v0].Next = v;
				RecAddScale(v,1,RM_ALL,v0,1.0f);

				do
				{
					sVERIFY(GetVertId(k)==v0);
					Edge[k/2].Vert[k&1] = v;
					k = NextVertEdge(k);
				}
				while(!Edge[k/2].Crease);
			}
		}
	}

	Edge[i/2].Crease|=what; // update crease flags
}

void GenMesh::Crease(sInt selType,sU32 dmask1,sU32 dmask0,sInt what)
{
  sInt i;
  sBool sel;

	RecBegin(IM_ADDSCL);

  for(i=0;i<Edge.Count*2;i++)
  {
    if(selType==0)
      sel = IsBorderEdge(i,0);
    else if(selType==1)
      sel = Edge[i/2].Select;
    else
      sel = GetFace(i)->Select;

    if(sel)
      SingleCrease(i,dmask1,dmask0,what);
  }

  RecEnd();
}

void GenMesh::UnCrease(sBool edge,sInt what)
{
	sInt i;
	sBool sel;

	sVERIFY(!(what & 1));

	for(i=0;i<Edge.Count;i++)
	{
		if(edge)
			sel = Edge[i].Select;
		else
      sel = IsBorderEdge(i*2,0);

		if(sel)
			Edge[i].Crease &= ~what;
	}

	// todo: fix vertices too
}

void GenMesh::CalcNormals(sInt type,sInt calcWhat)
{
	sU32 *data;
	sInt i,vs,e,pe,ee,count,msk;
	sBool ok;

	msk = type ? 0 : sGMF_NORMAL;
	vs = VertSize * sizeof(sVector);

	data = RecBegin(IM_NORMAL);
	// first build vertex->edge links in Temp
	for(i=0;i<Vert.Count;i++)
		Vert[i].Temp = -1;

	for(i=0;i<Edge.Count*2;i++)
		Vert[GetVertId(i)].Temp = i;

	// write data. format: count, face offsets, dest vertex offset.
  *data++ = calcWhat;
  if(calcWhat)
  {
	  for(i=0;i<Vert.Count;i++)
	  {
		  count = 0;
      if(Vert[i].Temp==-1)
        continue;

		  // find "left border" (finds a crease or just a full loop)
		  e = ee = FindCrease(Vert[i].Temp,msk);
		  ok = sFALSE;
  		
		  // add all vertices on this side of the crease
		  do
		  {
			  if(GetFace(e)->Select)
				  ok = sTRUE;

        data[++count + 1] = GetVertId(e) * vs;
        data[++count + 1] = GetVertId(e^1) * vs;
			  e = NextVertEdge(e);
        pe = e;
		  }
		  while(e != ee && !(Edge[pe/2].Crease & msk));

      data[++count + 1] = GetVertId(e) * vs;
      data[++count + 1] = GetVertId(e^1) * vs;

		  if(ok)
		  {
        RecOpCount++;
			  *data = count/2;
        data[1] = i * vs;
			  data += count + 2;
		  }
	  }
  }

	RecEnd(data);
}

void GenMesh::Triangulate(sInt threshold,sU32 dmask1,sU32 dmask0,sInt type)
{
	sInt i,j,c,v,e,ee;
	sInt Edgei[64];
	sU32 Temp[128];
	sF32 ftemp;

  if(!type)
	  RecBegin(IM_ADDSCL);

	for(i=0;i<Face.Count;i++)
	{
		if(Face[i].Select)
		{
			e = ee = Face[i].Edge;
			c = 0;

			do
			{
				sVERIFY(c < 64);
				Edgei[c] = e;
				Temp[c*2] = GetVertId(e);
				e = NextFaceEdge(e);
				c++;
			}
			while(e != ee);

			if(c>=threshold)
			{
        if(!type)
        {
				  v = AddNewVert();
				  Vert[v].Sel(dmask1,dmask0);

				  ftemp = 1.0f / c;
				  for(j=0;j<c;j++)
				  {
					  Temp[j*2+1] = *((sU32 *) &ftemp);
					  if(j==0)
              SplitBridge(NextVertEdge(Edgei[0]),Edgei[0],v,e,dmask1,dmask0);
					  else
						  SplitFace(PrevFaceEdge(PrevFaceEdge(Edgei[j])),Edgei[j],dmask1,dmask0);
				  }

				  RecAddScaleArray(v,c,RM_ALL,Temp);
        }
        else
        {
          for(j=0;j<c-3;j++)
            SplitFace(PrevFaceEdge(PrevFaceEdge(Edgei[j])),Edgei[j],dmask1,dmask0);
        }
			}
		}
	}

  if(!type)
	  RecEnd();
}

void GenMesh::Cut(const sVector &plane,sInt mode)
{
	sInt i,va0,va1,vb0,vb1,ec,va,vb,e,ee,pe,noe,noet;
	sF32 t0,t1;
	sBool force;

	RecBegin(IM_ADDSCL);

	// classify vertices
	for(i=0;i<Vert.Count;i++)
	{
		t0 = VertPos(i).Dot3(plane)+plane.w;
		Vert[i].Temp = *((sU32 *) &t0);
	}

	// clip edges
	ec = Edge.Count;
	for(i=0;i<ec*2;i+=2)
	{
		va0 = GetVertId(i);
		va1 = GetVertId(NextFaceEdge(i));
		vb1 = GetVertId(i^1);
		vb0 = GetVertId(NextFaceEdge(i^1));

		if ((Vert[va0].Temp ^ Vert[vb1].Temp) < 0) // edge crosses plane?
		{
			t0 = *((sF32 *) &Vert[va0].Temp);
			t1 = *((sF32 *) &Vert[vb1].Temp) - t0;

			if (fabs(t1) > 1e-5f) // avoid precision problems
			{
        va = AddNewVert();
        SplitBridge(NextFaceEdge(i^1),PrevFaceEdge(i^1),va,vb);
				t0 = -t0 / t1;
				RecAddScale(va,2,RM_ALL,va0,1.0f-t0,va1,t0);
				Vert[va].Temp = 0;
				if(va!=vb)
				{
					RecAddScale(vb,2,RM_ALL,vb0,1.0f-t0,vb1,t0);
					Vert[vb].Temp = 0;
				}
			}
		}
	}

	for(i=0;i<Edge.Count*2;i+=2)
	{
		GetEdge(i)->Temp[0] = GetVert(i)->Temp >= 0 && GetVert(i^1)->Temp >= 0;
		GetEdge(i)->Temp[1] = 0;
	}

	// fix faces
	noe = 0;
	for(i=0;i<Face.Count;i++)
	{
		ec = 0;
		e = Face[i].Edge;
		ee = PrevFaceEdge(e);
		pe = -1;

		do
		{
			force = sFALSE;

			if(GetEdge(e)->Temp[0]) // edge is on right side of plane?
			{
				if(pe==-1)
				{
					ee = e;
					force = sTRUE;
				}
				else if(pe!=PrevFaceEdge(e)) // need to add new edge
				{
					Edge.AtLeast(Edge.Count+1);
					Edge[Edge.Count].Init();
					Edge[Edge.Count].Vert[0] = GetVertId(NextFaceEdge(pe));
					Edge[Edge.Count].Vert[1] = GetVertId(e);
					Edge[Edge.Count].Next[0] = e;
					Edge[Edge.Count].Prev[0] = pe;
					Edge[Edge.Count].Face[0] = i;
					Edge[Edge.Count].Temp[0] = 0;
					Edge[Edge.Count].Temp[1] = 1;
					Edge[e/2].Prev[e&1] = Edge.Count*2;
					Edge[pe/2].Next[pe&1] = Edge.Count*2;
					e = Edge.Count*2;
					Edge.Count++;
					noe++; // number of open edges
				}

				Face[i].Edge = e;
				pe = e;
				ec++;
			}

			e = NextFaceEdge(e);
		}
		while(PrevFaceEdge(e) != ee || force);

		if(ec<3)
		{
			Face[i].Material=0;
			Face[i].Select=0;
			Face[i].Mask=0;
		}
	}

	// close mesh
	noet = noe;
	while(noe)
	{
		// find first open edge
		i = Edge.Count - noet;
		while(!Edge[i].Temp[1])
			i++;
		sVERIFY(i<Edge.Count);

		// prepare new face
		Face.AtLeast(Face.Count+1);
		Face[Face.Count].Init();
		Face[Face.Count].Edge = i*2+1;
		Face[Face.Count].Material = mode ? Face[Edge[i].Face[0]].Material : 0;
		Face.Count++;

		e = ee = i*2+1;

		// add open edges till face is closed
		do
		{
			GetEdge(e)->Temp[1] = 0;
			sVERIFY(noe>0);
			noe--;

			for(i=(Edge.Count-noet)*2;i<Edge.Count*2;i++)
			{
				if((GetEdge(i)->Temp[1] || i==ee) && Vert[GetVertId(i)].First==Vert[GetVertId(e^1)].First)
					break;
			}
			sVERIFY(i!=Edge.Count*2);

			Edge[e/2].Next[e&1]=i;
			Edge[i/2].Prev[i&1]=e;
			Edge[i/2].Face[i&1]=Face.Count-1;
			e=i;
		}
		while(e!=ee);
	}

	RecEnd();
	CleanupMesh();
}

void GenMesh::CleanupMesh()
{
	sInt i,j,fc,ec,e,ee;
	sInt *remape,*remapf;

	// cleanup faces and mark+renumber edges
	fc = 0;
	ec = 0;
	remape = new sInt[Edge.Count];
	remapf = new sInt[Face.Count];

	// prepare edges
	for(i=0;i<Edge.Count;i++)
		Edge[i].Used = 0;

	// mark+renumber faces
	for(i=0;i<Face.Count;i++)
	{
		if (Face[i].Material != -1)
		{
			e = ee = Face[i].Edge;

			do
			{
				Edge[e/2].Used = 1;
				e = NextFaceEdge(e);
			}
			while(e != ee);

			remapf[i] = fc;
			Face[fc] = Face[i];
			fc++;
		}
	}

	// mark+renumber edges
	for(i=0;i<Edge.Count;i++)
	{
		if(Edge[i].Used)
		{
			remape[i] = ec*2;
			Edge[ec] = Edge[i];
			ec++;
		}
	}
	
	Face.Count = fc;
	Edge.Count = ec;

	// fix links
	for(i=0;i<ec;i++)
	{
		for(j=0;j<4;j++)
		{
			e = Edge[i].Next[j];
			Edge[i].Next[j] = remape[e/2] | (e&1);
		}

		for(j=0;j<2;j++)
			Edge[i].Face[j] = remapf[Edge[i].Face[j]];
	}

	for(i=0;i<fc;i++)
	{
		e = Face[i].Edge;
		Face[i].Edge = remape[e/2] | (e&1);
	}

	delete[] remape;
	delete[] remapf;
}

void GenMesh::ExtrudeNormal(sF32 distance)
{
  sMatrix mat;

  mat.Init();
  mat.i.x = distance;
  mat.j.y = distance;
  mat.k.z = distance;
  TransVert(mat,sGMI_NORMAL,sGMI_POS+0x10);
/*
	sInt i;
  sU32 *data;

  data = RecBegin(IM_ADDSCL);
	for(i=0;i<Vert.Count;i++)
	{
		if(Vert[i].Select)
    {
      *data++ = 2;
      data = RecSourceInd(data,i*VertSize);
      *data++ = 0x3f800000; // 1.0f
      data = RecSourceInd(data,i*VertSize+VertMap[sGMI_NORMAL]);
      *data++ = *(sU32 *)&distance;
      data = RecDestInd(data,i*VertSize);
      RecOpCount++;
    }
	}
	RecEnd(data);
*/
}

#if !sINTRO_X
void GenMesh::Displace(GenBitmap *bitmap,sF32 amplitude)
{
	sInt i;
	sU32 *data;

	data = RecBegin(IM_DISPLACE);
	*data++ = RecObject(bitmap);
	*data++ = *((sU32 *) &amplitude);
	for(i=0;i<Vert.Count;i++)
	{
		if(Vert[i].Select)
		{
			*data++ = i * VertSize;
			RecOpCount++;
		}
	}
	RecEnd(data);
}
#endif

#if !sINTRO_X
void GenMesh::Bevel(sF32 elevate,sF32 pullin,sInt mode,sU32 dmask1,sU32 dmask0)
{
	sInt i,v,vf,vn,e;
	sU32 *data;

	Extrude(dmask1,dmask0,mode);

	data = RecBegin(IM_BEVEL);
	*((sF32 *) data) = pullin; data++;

	for(i=0;i<Vert.Count;i++)
		Vert[i].ReIndex=i;

	for(i=0;i<Edge.Count*2;i++)
    Edge[i/2].Temp[i&1] = IsBorderEdge(i,mode) ? 0 : -1;

	for(i=0;i<Edge.Count*2;i++)
	{
		if(Edge[i/2].Temp[i&1]!=-1)
		{
			e=i;
			do
			{
				e=PrevVertEdge(e);
				sVERIFY(e!=i);
			}
			while(Edge[e/2].Temp[~e&1]==-1);

			Edge[i/2].Temp[i&1]=e^1;
		}
	}

	for(i=0;i<Edge.Count*2;i++)
	{
		e=Edge[i/2].Temp[i&1];
		if(e!=-1)
		{
			sVERIFY(Edge[e/2].Temp[e&1]!=-1);
			v = vf = GetVertId(e);

			do
			{
				*data++ = GetVertId(i);
				*data++ = v;
				*data++ = GetVertId(Edge[e/2].Temp[e&1]);

        Vert[v].ReIndex = vn = AddCopiedVert(v);
				*data++ = vn;
				RecOpCount++;

				v = Vert[v].Next;
			}
			while (mode==1 && v!=vf);
		}
	}

	RecEnd(data);
	ReIndex();

	Face2Vert();
	ExtrudeNormal(elevate);
}
#endif

void GenMesh::Perlin(const sMatrix &mat,const sVector &amp)
{
//#if !sINTRO_X
	sInt i;
	sU32 *data;
  sMatrix ampmat;

	data = RecBegin(IM_PERLIN);
	*data++ = RecMatrix(mat);
  ampmat.Init();
  ampmat.i = amp;
  RecMatrix(ampmat);

	for(i=0;i<Vert.Count;i++)
	{
		if(Vert[i].Select)
		{
			*data++ = i;
			RecOpCount++;
		}
	}

	RecEnd(data);
//#endif
}

sBool GenMesh::Add(GenMesh *other)
{
	sInt i,j,v,e,f,m;
	GenMeshVert *vp;
	GenMeshEdge *ep;
	GenMeshFace *fp;
	GenMeshMtrl *mp;
	sU32 *data;

	if(VertMask!=other->VertMask)
		return sFALSE;

	// add topological info
	v=Vert.Count;		Vert.AtLeast(v+other->Vert.Count); Vert.Count+=other->Vert.Count;
	e=Edge.Count;  	Edge.AtLeast(e+other->Edge.Count); Edge.Count+=other->Edge.Count;
	f=Face.Count;		Face.AtLeast(f+other->Face.Count); Face.Count+=other->Face.Count;
	m=Mtrl.Count-1; Mtrl.AtLeast(m+other->Mtrl.Count); Mtrl.Count+=other->Mtrl.Count-1;
	Realloc(v+other->Vert.Count);

	vp = &Vert[v];
	for(i=0;i<other->Vert.Count;i++,vp++)
	{
		*vp=other->Vert[i];
		vp->Next+=v;
		vp->First+=v;
	}
	
	ep = &Edge[e];
	for(i=0;i<other->Edge.Count;i++,ep++)
	{
		*ep=other->Edge[i];
		for(j=0;j<2;j++)
		{
			ep->Next[j]+=e*2;
			ep->Prev[j]+=e*2;
			ep->Vert[j]+=v;
			ep->Face[j]+=f;
		}
	}

	fp = &Face[f];
	for(i=0;i<other->Face.Count;i++,fp++)
	{
		*fp=other->Face[i];
		fp->Edge+=e*2;
		fp->Material = fp->Material ? fp->Material+m : 0;
	}

	mp = &Mtrl[m+1];
	for(i=1;i<other->Mtrl.Count;i++,mp++)
	{
		*mp=other->Mtrl[i];
		for(j=0;j<GENMAT_MAXBATCH;j++)
			mp->Pass[j].Reset();
	}

	// add vert buffers
	data = RecBegin(IM_COPY);
	*data++ = RecObject(other);
	*data++ = v * VertSize; // dest
	*data++ = 0; // start
	*data++ = other->Vert.Count * VertSize;
	RecOpCount++;
	RecEnd(data);

	return sTRUE;
}

void GenMesh::DeleteFaces()
{
	sInt i;

	for(i=0;i<Face.Count;i++)
	{
		if(Face[i].Select)
		{
			Face[i].Material=0;
			Face[i].Select=0;
			Face[i].Mask=0;
		}
	}
}

/****************************************************************************/
/****************************************************************************/

static sF32 fget(sU8 *ptr)
{
  sU16 v;
  sU32 vd;

  v = *(sU16 *) ptr;
  vd = (v & 32768) << 16 // sign
    | ((((v >> 10) & 31) + 128 - 16) << 23)
    | ((v & 1023) << 13);

  return *(sF32 *) &vd;
}

#if !sINTRO

static sInt doShift(sU32 v,sInt shift)
{
  while(shift>0)
    v<<=1,shift--;

  while(shift<0)
    v>>=1,shift++;

  return v;
}

static sF32 fput(sF32 v,sU8 *ptr)
{
  sU32 value;
  sU16 dest;
  sInt esrc,edst;

  value = *(sU32 *) &v;
  esrc = ((value>>23) & 255) - 128;
  edst = sRange(esrc,15,-16);

  dest = ((value>>16) & 32768) // sign
    | ((edst+16)<<10)
    | (doShift(value>>13,edst-esrc) & 1023);

  *(sU16 *) ptr = dest;
  return fget((sU8 *) &dest);
}

struct fwrite
{
  sF32 min;
  sF32 max;
  
  void Init(sF32);
  void Pre(sF32);
  void Write0(sU8 *&p);
  void Write(sU8 *&p,sF32);
};

void fwrite::Init(sF32 f)
{
  min = max = f;
}

void fwrite::Pre(sF32 f)
{
  if(f<min)
    min = f;
  if(f>max)
    max = f;
}

void fwrite::Write0(sU8 *&p)
{
  if(min+0.125f>=max)
    max=min+0.125f;
  min = fput(min,p); p+=2;
  max = fput(max,p); p+=2;
}

void fwrite::Write(sU8 *&p,sF32 f)
{
  *p++ = sRange<sInt>((f-min)/(max-min)*255,255,0);
}

#endif

/****************************************************************************/

class MeshElement
{
public:
	sInt Degree;
	sInt Temp;
	MeshElement **Elem;
  static sU32 *Storage;

	void Init(sInt deg);
	sInt Find(MeshElement *v,sInt adj) const;

  static void ResetStorage();
};

sU32 *MeshElement::Storage;

void MeshElement::Init(sInt deg)
{
	Degree = deg;
  Elem = (MeshElement **) Storage;
  Storage += deg;
}

sInt MeshElement::Find(MeshElement *v,sInt adj) const
{
	sInt i;
  MeshElement **ep;

  ep = Elem;
	for(i=0;i<Degree;i++)
	{
		if(ep[i]==v)
			return (i+adj+Degree)%Degree;
	}

	sVERIFYFALSE;
  return 0;
}

void MeshElement::ResetStorage()
{
  Storage = RBData;
  sSetMem4(RBData,0,RBCOUNTMAX);
}

class MeshCoder
{
public:
	MeshElement *Face;
	sInt FaceCtr;
	MeshElement *Vert;
	sInt VertCtr;
	GenMesh *Mesh;

#if !sPLAYER
	sInt VertOrder[65536],VertPos;
#endif
  sInt FaceCount;

  RangeCoder Coder;
  RangeModel DV;
  RangeModel DVFlag;
	sInt avList[256],avPos;

#if !sPLAYER
	void Encode(GenMesh *msh,sU8 *&p);
	
	sInt NextVertSlot(sInt e);
	sInt FindFirstVert(sInt v);
	GenMesh *MakeClosedMesh(GenMesh *msh);
	void ActivateVE(MeshElement *f,sInt i,sInt edg);
	MeshElement *AddFace(sInt deg);
	MeshElement *AddVert(sInt deg);
#endif
	
	sU8 *Decode(GenMesh *msh,sU8 *p);

	MeshElement *DecodeFace();
	void ActivateVD(MeshElement *f,sInt i);

	sInt ForceFV(MeshElement *f,sInt j,sInt dir);
	void AddFaceToVertex(MeshElement *f,sInt i,MeshElement *v,sInt j);
};

#define sDV_LOWRANGE 9 // fine-tune this for some additional bytes :)

#if !sPLAYER

void MeshCoder::Encode(GenMesh *msh,sU8 *&p)
{
	sInt i,il,j,k,e,ee,seed,vi,d;
	MeshElement *f,*v;

	Mesh = MakeClosedMesh(msh);
  MeshElement::ResetStorage();
	Face = new MeshElement[Mesh->Face.Count];
	Vert = new MeshElement[Mesh->Vert.Count];
	FaceCtr = 0;
	VertCtr = 0;
	avPos = 0;

	for(i=0;i<Mesh->Face.Count;i++)
		Mesh->Face[i].Temp=-1; // not encoded yet

	for(i=0;i<Mesh->Vert.Count;i++)
		Mesh->Vert[i].Temp=0; // not encoded yet

  Coder.InitEncode(p+2);
  DV.Init(sDV_LOWRANGE);
  DVFlag.Init(2);
	VertPos = 0;

	while(1)
	{
		// initialization
		seed = 0;
		while(seed<Mesh->Face.Count && Mesh->Face[seed].Temp!=-1)
			seed++;

		if(seed == Mesh->Face.Count)
		{
      DV.Encode(Coder,0); // end of stream
			break;
		}

		k = Mesh->GetDegree(seed);
    if(k<sDV_LOWRANGE)
      DV.Encode(Coder,k-1);
    else
    {
      DV.Encode(Coder,sDV_LOWRANGE-1);
      Coder.EncodeGamma(k-sDV_LOWRANGE);
    }
    DVFlag.Encode(Coder,!Mesh->Face[seed].Material);

		f = AddFace(k);
		e = Mesh->Face[seed].Edge;
		Mesh->Face[seed].Temp = (sInt) f;
		f->Temp = e;

		for(i=0;i<k;i++)
		{
			ActivateVE(f,i,e);
			e = Mesh->NextFaceEdge(e);
		}

		// mainloop
		while(avPos)
		{
			// pick vert to complete
			j = 64;
			for(i=0;i<avPos;i++)
			{
				v = (MeshElement *) Mesh->Vert[avList[i]].Temp;
				sVERIFY(v != 0);
				
				k = 0;
				for(il=0;il<v->Degree;il++)
				{
					if(!v->Elem[il])
						k++;
				}

				if(k<j)
				{
					vi = i;
					j = k;
				}
			}

			v = (MeshElement *) Mesh->Vert[avList[vi]].Temp;
			sVERIFY(v != 0);
			e = v->Temp;

			for(j=0;j<v->Degree;j++)
			{
				if(!v->Elem[j]) // empty slot
				{
					// activate face
					sVERIFY(Mesh->GetFace(e)->Temp == -1);
					d = Mesh->GetDegree(Mesh->GetFaceId(e));
          if(d<sDV_LOWRANGE)
            DV.Encode(Coder,d-1);
          else
          {
            DV.Encode(Coder,sDV_LOWRANGE-1);
            Coder.EncodeGamma(d-sDV_LOWRANGE);
          }
          DVFlag.Encode(Coder,!Mesh->GetFace(e)->Material);

					f = AddFace(d);
					Mesh->GetFace(e)->Temp = (sInt) f;
					f->Temp = e;

					AddFaceToVertex(f,0,v,j);

					// complete face
					i = ForceFV(f,j,1);
					il = ForceFV(f,j,-1);
					if(i)
					{
						for(k=0,ee=e;k<i;k++)
							ee = Mesh->NextFaceEdge(ee);

						while(i<=il)
						{
							ActivateVE(f,i++,ee);
							ee = Mesh->NextFaceEdge(ee);
						}
					}

					for(i=0;i<f->Degree;i++)
						sVERIFY(f->Elem[i]!=0);
				}

				e = NextVertSlot(e);
			}

			sVERIFY(e == v->Temp);

			// remove vert from queue
			avList[vi] = avList[--avPos];
		}
	}

	for(i=0;i<Mesh->Face.Count;i++)
		sVERIFY(Mesh->Face[i].Temp != 0);

	for(i=0;i<Mesh->Vert.Count;i++)
		sVERIFY(Mesh->Vert[i].Temp != 0);

	delete[] Face;
	delete[] Vert;

  *(sU16 *) p = Mesh->Vert.Count; p+=2;

  DV.Exit();
  DVFlag.Exit();
  Coder.FinishEncode();
  p += Coder.GetBytes();
}

sInt MeshCoder::NextVertSlot(sInt e)
{
	sInt ec;

	ec = e;
	do
	{
		e = Mesh->NextVertEdge(e);
	}
	while(Mesh->GetVertId(e) != Mesh->GetVertId(ec));

	return e;
}

sInt MeshCoder::FindFirstVert(sInt v)
{
	MeshElement *e;
	sInt vt;

	if(Mesh->Vert[Mesh->Vert[v].First].Temp) // first already encoded?
	{
		e = (MeshElement *) Mesh->Vert[Mesh->Vert[v].First].Temp;
		return e - Vert;
	}
	else
	{
		// make this the first
		vt = v;
		do
		{
			Mesh->Vert[v].First = vt;
			v = Mesh->Vert[v].Next;
		}
		while(v!=vt);

		return -1;
	}
}

GenMesh *MeshCoder::MakeClosedMesh(GenMesh *msh)
{
	sInt i,j,k,d;
	sInt Edges[64],fc,e,ee,fa;
	GenMesh *Mesh;

	Mesh = new GenMesh;
	Mesh->Init(msh->VertMask,msh->VertCount);

	// copy vertices
	Mesh->Vert.AtLeast(msh->Vert.Count);
	Mesh->Vert.Count=msh->Vert.Count;

	for(i=0;i<msh->Vert.Count;i++)
	{
		Mesh->Vert[i]=msh->Vert[i];
		Mesh->Vert[i].First=i;
	}

	sCopyMem4((sU32 *)Mesh->VertBuf,(sU32 *)msh->VertBuf,4*msh->VertSize*msh->VertCount);

	// copy non-deleted faces and create edges
	for(i=0;i<msh->Face.Count;i++)
	{
		if(msh->Face[i].Material)
		{
			Mesh->Face.AtLeast(Mesh->Face.Count+1);
			fc = Mesh->Face.Count++;
			Mesh->Face[fc].Init();
			Mesh->Face[fc].Material = msh->Face[i].Material;

			e = ee = msh->Face[i].Edge;
			j = 0;
			
			do
			{
				Edges[j++] = Mesh->AddEdge(msh->GetVertId(e),msh->GetVertId(msh->NextFaceEdge(e)),fc);
				sVERIFY(j<=64);
				e = msh->NextFaceEdge(e);
			}
			while(e!=ee);

			d=j;
			for(j=0;j<d;j++)
			{
				k=Edges[j];
				Mesh->Edge[k/2].Next[k&1] = Edges[(j+1)  %d];
				Mesh->Edge[k/2].Prev[k&1] = Edges[(j-1+d)%d];
			}
		}
	}

	// fix vertices
	for(i=0;i<msh->Vert.Count;i++)
		Mesh->Vert[i].First=msh->Vert[i].First;

	// close connected components
	fa = 0;
	for(i=0;i<Mesh->Edge.Count;i++)
	{
		if(Mesh->Edge[i].Face[1]==-1) // boundary found
		{
			e = i*2+1;

			Mesh->Face.AtLeast(Mesh->Face.Count+1);
			fc = Mesh->Face.Count++;
			Mesh->Face[fc].Init();
			Mesh->Face[fc].Material = 0;
			Mesh->Face[fc].Edge = e;
			fa++;

			do
			{
				ee = e;
				do
				{
					e = Mesh->PrevVertEdge(e);
				}
				while(Mesh->GetFaceId(e^1)!=-1);
				e ^= 1;

				sVERIFY(Mesh->Edge[ee/2].Prev[ee&1] == -1);
				sVERIFY(Mesh->Edge[e/2].Next[e&1] == -1);
				Mesh->Edge[ee/2].Prev[ee&1] = e;
				Mesh->Edge[e/2].Next[e&1] = ee;
			}
			while(i*2+1!=e);

			ee=e; j=0;
			do
			{
				Mesh->Edge[e/2].Face[e&1] = fc;
				e = Mesh->NextFaceEdge(e);
				j++;
			}
			while(e!=ee);
		}
	}

	Mesh->Verify();
	return Mesh;
}

void MeshCoder::ActivateVE(MeshElement *f,sInt i,sInt edg)
{
	sInt j,k,val,vt,e,ee;
	MeshElement *v;

	vt = Mesh->GetVertId(edg);

	for(j=0;j<avPos;j++)
	{
		if(avList[j]==vt)
			break;
	}

	if(j==avPos) // add
	{
		val = Mesh->GetValence(edg);
		v = AddVert(val);
		VertOrder[VertPos++] = vt;

    if(val<sDV_LOWRANGE)
      DV.Encode(Coder,val-1);
    else
    {
      DV.Encode(Coder,sDV_LOWRANGE-1);
      Coder.EncodeGamma(val-sDV_LOWRANGE);
    }

    j = FindFirstVert(vt);
    DVFlag.Encode(Coder,j != -1);
    if(j!=-1)
      Coder.EncodeGamma(j);

		f->Elem[i] = v;
		v->Elem[0] = f;
		v->Temp = edg;
		avList[avPos++] = vt;
		sVERIFY(avPos<256);
		Mesh->Vert[vt].Temp = (sInt) v;
	}
	else // split
	{
		v = (MeshElement *) Mesh->Vert[vt].Temp;
		sVERIFY(v != 0);

    DV.Encode(Coder,0);
    Coder.EncodeGamma(j);

		j = 0;
		k = Mesh->GetFaceId(f->Temp);
		e = ee = v->Temp;
		while(Mesh->GetFaceId(e) != k)
		{
			j++;
			e = NextVertSlot(e);
			sVERIFY(e != ee);
		}

    Coder.EncodePlain(j,v->Degree);
		AddFaceToVertex(f,i,v,j);
	}
}

MeshElement *MeshCoder::AddFace(sInt deg)
{
	Face[FaceCtr].Init(deg);
	return &Face[FaceCtr++];
}

MeshElement *MeshCoder::AddVert(sInt deg)
{
	Vert[VertCtr].Init(deg);
	return &Vert[VertCtr++];
}

#endif

sU8 *MeshCoder::Decode(GenMesh *msh,sU8 *p)
{
	sInt i,j,k,d,vi,fc;
	MeshElement *f,*v;

	// parse header
  i = *(sU16 *) p; p+=2;
  Coder.InitDecode(p);
  DV.Init(sDV_LOWRANGE);
  DVFlag.Init(2);

  MeshElement::ResetStorage();
	Face = new MeshElement[16384];
	Vert = new MeshElement[i];
	FaceCtr = 0;
	VertCtr = 0;
	avPos = 0;

	Mesh = msh;
  Mesh->Face.Count = 0;
  FaceCount = 0;

	while(1)
	{
		f = DecodeFace();
		if(!f)
			break;

		for(i=0;i<f->Degree;i++)
			ActivateVD(f,i);

		do
		{
			j = 64;
			for(i=0;i<avPos;i++)
			{
				v = &Vert[avList[i]];
				for(d=0,k=0;d<v->Degree;d++)
        {
          if(!v->Elem[d])
            k++;
        }

				if(k<j)
				{
					j=k;
					vi=i;
				}
			}

			v = &Vert[avList[vi]];
			sVERIFY(v != 0);

			for(j=0;j<v->Degree;j++)
			{
				if(!v->Elem[j])
				{
					f = DecodeFace();
					AddFaceToVertex(f,0,v,j);

          k = ForceFV(f,j,1);
          i = ForceFV(f,j,-1);

					if(k)
					{
						while(k<=i)
							ActivateVD(f,k++);
					}
				}
			}

			avList[vi] = avList[--avPos];
		}
		while(avPos);
	}

  DV.Exit();
  DVFlag.Exit();
  p += Coder.GetBytes();

	// the mesh topology should be complete now.
	// try and convert it into halfedges.
	Mesh->Face.AtLeast(FaceCount);
	Mesh->Face.Count=FaceCount;
	fc = 0;

  for(f=Face;f<Face+FaceCtr;f++)
	{
		sInt Edges[66],f0,f1;

		if(f->Temp)
		{
			Mesh->Face[fc].Init();
			Mesh->Face[fc].Material=1;

			d = f->Degree;
      f0 = f->Elem[d-1]->Temp;
			for(j=0;j<d;j++)
			{
				f1 = f0;
        f0 = f->Elem[j]->Temp;
				Edges[j+1] = Mesh->AddEdge(f1,f0,fc);
			}
      Edges[0]=Edges[d];
      Edges[d+1]=Edges[1];

			for(j=0;j<d;j++)
			{
				k = Edges[j+1];
        Mesh->Edge[k/2].Next[k&1] = Edges[j+2];
        Mesh->Edge[k/2].Prev[k&1] = Edges[j];
			}

			fc++;
		}
	}

	delete[] Face;
	delete[] Vert;
  return p;
}

MeshElement *MeshCoder::DecodeFace()
{
	sInt d;
  sBool ok;
	MeshElement *f;

  d = DV.Decode(Coder);
	if(!d)
		return 0;

  if(++d == sDV_LOWRANGE)
    d += Coder.DecodeGamma();

  ok = !DVFlag.Decode(Coder);
  FaceCount += ok;

  f = &Face[FaceCtr++];
  f->Init(d);
  f->Temp = ok;
	return f;
}

void MeshCoder::ActivateVD(MeshElement *f,sInt i)
{
	MeshElement *v;
	sInt val,j,vi;

  val = DV.Decode(Coder);
	if(val) // add
	{
    if(++val == sDV_LOWRANGE)
      val += Coder.DecodeGamma();

    v = &Vert[VertCtr++];
    v->Init(val);
		avList[avPos++] = vi = v->Temp = Mesh->AddNewVert();
    
    if(DVFlag.Decode(Coder))
		{
      j = Coder.DecodeGamma();
      Mesh->Vert[vi].Next = Mesh->Vert[j].Next;
      Mesh->Vert[vi].First = Mesh->Vert[j].First;
			Mesh->Vert[j].Next = vi;
		}

    j=0;
	}
	else // split
	{
    v = &Vert[avList[Coder.DecodeGamma()]];
    j = Coder.DecodePlain(v->Degree);
	}

  AddFaceToVertex(f,i,v,j);
}

sInt MeshCoder::ForceFV(MeshElement *f,sInt j,sInt dir)
{
	MeshElement *v,*vt;
	sInt i;

	v = f->Elem[0];
	i = 0;

  while(1)
	{
    i = (i+dir+f->Degree)%f->Degree;
    vt = v->Elem[(j-dir+v->Degree)%v->Degree];
		v = f->Elem[i];
    if(!i || !vt || !v)
      break;

    j = v->Find(vt,-dir);
		AddFaceToVertex(f,i,v,j);
  }

  return i;
}

void MeshCoder::AddFaceToVertex(MeshElement *f,sInt i,MeshElement *v,sInt j)
{
	sInt dir,in;
	MeshElement *fp;

  f->Elem[i] = v;
	v->Elem[j] = f;

	for(dir=-1;dir<=1;dir+=2)
	{
    fp = v->Elem[(j+dir+v->Degree)%v->Degree];
    in = (i-dir+f->Degree)%f->Degree;
		if(fp && !f->Elem[in])
			f->Elem[in] = fp->Elem[fp->Find(v,dir)];
	}
}

/****************************************************************************/

static sInt VertQuantize[3] = { 319,639,191 };
static sInt AnimQuantize[5] = { 15,31,63,127,255 };
static sF32 AnimThreshold[5] = { 0.0f,0.016f,0.03f,0.06f,0.10f };

void GenMesh::WriteCompact(sU8 *&p)
{
#if !sPLAYER
  sInt i,j,k,l;
  sVector *vp;
  sVector v;
  sF32 *vdim;
//  sInt imin,imax;
  sF32 fmin,fmax;
	MeshCoder *cod;
  sF32 *fp;
  sU8 *pstart;
  sInt vl[4],ev;
  fwrite srt[9];
  RangeCoder coder;
  RangeModel vModel[5];

// check bounds

  sVERIFY(Vert.Count<0x7fff);
  sVERIFY(Face.Count<0x7fff);
  sVERIFY(KeyCount<0x7fff);
  sVERIFY(CurveCount<0xff);
  sVERIFY(BoneMatrix.Count<0xff);
  sVERIFY((VertMask & ~(sGMF_POS|sGMF_NORMALS|sGMF_COLOR0|sGMF_UV0))==0)

// write header

  pstart = p;
  *p++ = KeyCount&0xff;
  *p++ = KeyCount>>8;
  *p++ = CurveCount;
  *p++ = BoneMatrix.Count;
  *p++ = VertMask&0xff;
  *p++ = VertMask>>8;
  sDPrintF("%04x %5d : header\n",p-pstart);	pstart=p;

// topology
	pstart = p;
	cod = new MeshCoder;
	cod->Encode(this,p);
	sDPrintF("%04x %5d : topology\n",p-pstart); pstart=p;
	sVERIFY(cod->VertPos == Vert.Count);

// write vertices
  for(i=0;i<Vert.Count;i++)
  {
		k=cod->VertOrder[i];
    for(j=0;j<4;j++)
      if(Vert[k].Matrix[j]==0xff)
        break;
    Vert[k].Temp = j;
    sVERIFY(j>=1 && j<=4);
    *p++ = j-1;
  }
  sDPrintF("%04x %5d : matrix count\n",p-pstart); pstart=p;
  
  vl[0]=vl[1]=vl[2]=vl[3]=0;
  for(i=0;i<Vert.Count;i++)
	{
		k=cod->VertOrder[i];
    for(j=0;j<Vert[k].Temp;j++)
    {
      *p++ = Vert[k].Matrix[j]-vl[j];
      vl[j] = Vert[k].Matrix[j];
    }
	}
  sDPrintF("%04x %5d : matrix index\n",p-pstart); pstart=p;

  for(i=0;i<Vert.Count;i++)
	{
		k=cod->VertOrder[i];
    for(j=0;j<Vert[k].Temp-1;j++)
      *p++ = Vert[k].Weight[j];
	}
  sDPrintF("%04x %5d : matrix weight\n",p-pstart); pstart=p;

// write vertex buffer

  vdim = (sF32 *) p;
  vdim[0] = vdim[3] = VertPos(0).x;
  vdim[1] = vdim[4] = VertPos(0).y;
  vdim[2] = vdim[5] = VertPos(0).z;
  for(i=1;i<Vert.Count;i++)
  {
    v = VertPos(i);
    vdim[0] = sMin(vdim[0],v.x);
    vdim[1] = sMin(vdim[1],v.y);
    vdim[2] = sMin(vdim[2],v.z);
    vdim[3] = sMax(vdim[3],v.x);
    vdim[4] = sMax(vdim[4],v.y);
    vdim[5] = sMax(vdim[5],v.z);
  }
  vdim[3] = (vdim[3] - vdim[0]) / VertQuantize[0];
  vdim[4] = (vdim[4] - vdim[1]) / VertQuantize[1];
  vdim[5] = (vdim[5] - vdim[2]) / VertQuantize[2];
  p+=24;
  sDPrintF("%04x %5d : vert scaling\n",p-pstart); pstart=p;

  coder.InitEncode(p);

  for(i=0;i<3;i++)
  {
    vModel[i].Init(VertQuantize[i]+1);
    vl[i]=0;
  }
  vModel[3].Init(2);

  sF32 ex = vdim[3]/1.5f, ey = vdim[4]/1.5f, ez = vdim[5]/1.5f;

	for(i=0;i<Vert.Count;i++)
  {
		k = cod->VertOrder[i];
		vp = VertBuf + k * VertSize;

		if(VertMask & sGMF_POS && cod->Mesh->Vert[k].First==k)
		{
      // search for mirror verts
      for(j=0;j<i;j++)
      {
        sVector *vp2 = VertBuf + cod->VertOrder[j] * VertSize;

        if(fabs(vp->x+vp2->x)<ex && fabs(vp->y-vp2->y)<ey && fabs(vp->z-vp2->z)<ez)
          break;
      }

      if(j==i)
      {
        vModel[3].Encode(coder,0);
        for(j=0;j<3;j++)
        {
          ev = sRange<sInt>(((&vp->x)[j]-vdim[j])/vdim[j+3]+0.5f,VertQuantize[j],0);
          vModel[j].Encode(coder,(ev-vl[j]+VertQuantize[j]+1) % (VertQuantize[j]+1));
          vl[j] = ev;
        }
      }
      else
      {
        vModel[3].Encode(coder,1);
        coder.EncodePlain(j,i);
      }
		}
    if(VertMask & (sGMF_NORMAL | sGMF_TANGENT | sGMF_COLOR0 | sGMF_UV0))
      sVERIFYFALSE;
  }
  coder.FinishEncode();
  p += coder.GetBytes();

  for(i=0;i<4;i++)
    vModel[i].Exit();
  sDPrintF("%04x %5d : vertices\n",p-pstart); pstart=p;

// keys

  for(i=0;i<BoneCurve.Count;i++)
  {
    BoneMatrix[BoneCurve[i].Matrix].TransSRT[BoneCurve[i].Curve] = BoneCurve[i].Curve < 3 ? 1 : 0;
  }

  for(i=0;i<BoneMatrix.Count;i++)
  {
    l = -(BoneMatrix[i].Parent+1-i);
    sVERIFY(l>=0 && l<255);
    *p++ = l;
  }
  sDPrintF("%04x %5d : bone matrix parent\n",p-pstart); pstart=p;


  for(j=0;j<9;j++)
  {
    srt[j].Init(BoneMatrix[0].BaseSRT[j]);
    for(i=0;i<BoneMatrix.Count;i++)
    {
      srt[j].Pre(BoneMatrix[i].BaseSRT[j]);
      srt[j].Pre(BoneMatrix[i].TransSRT[j]);
    }
  }
  for(j=0;j<9;j++)
    srt[j].Write0(p);
  for(j=0;j<9;j++)
  {
    for(i=0;i<BoneMatrix.Count;i++)
      srt[j].Write(p,BoneMatrix[i].BaseSRT[j]);
    for(i=0;i<BoneMatrix.Count;i++)
      srt[j].Write(p,BoneMatrix[i].TransSRT[j]);
  }
  sDPrintF("%04x %5d : bone matrix srt\n",p-pstart); pstart=p;

  for(i=0;i<BoneCurve.Count;i++)
    *p++ = BoneCurve[i].Curve;
  vl[0]=0;
  for(i=0;i<BoneCurve.Count;i++)
  {
    *p++ = BoneCurve[i].Matrix-vl[0];
    vl[0] = BoneCurve[i].Matrix;
  }
  sDPrintF("%04x %5d : fcurve info\n",p-pstart); pstart=p;

  for(i=0;i<5;i++)
    vModel[i].Init(AnimQuantize[i]*2+2);
  coder.InitEncode(p+CurveCount*4);

	for(i=0;i<CurveCount;i++)
	{
    sInt sc,nsc;
		fp = &KeyBuf[i*KeyCount];

    fmin = fp[0]; fmax = 0.0f;
    for(j=1;j<KeyCount;j++)
      fmax = sMax<sF32>(fmax,fabs(fp[j]-fmin));

    fmin = fput(fmin,p); p+=2;
    fmax = fput(fmax,p); p+=2;

    for(j=0;j<5;j++)
    {
      if(fmax>=AnimThreshold[j])
      {
        sc = AnimQuantize[j];
        nsc = j;
      }
    }

    l=0;
    for(j=1;j<KeyCount;j++)
    {
      k = sRange<sInt>((fp[j] - fmin) / fmax * sc+0.5f,sc,-sc);
      ev = k-l;
      vModel[nsc].Encode(coder,ev & (sc*2+1));
      l = k;
    }
	}

  coder.FinishEncode();
  p += coder.GetBytes();
  for(i=0;i<5;i++)
    vModel[i].Exit();

  sDPrintF("%04x %5d : fcurve data\n",p-pstart); pstart=p;

// end

  *p++ = 'r';
  *p++ = 'y';
  *p++ = 'g';
  sDPrintF("%04x %5d : footer\n",p-pstart); pstart=p;

	delete cod;
#endif
}

/****************************************************************************/

struct fread
{
  sF32 min;
  sF32 max;
  void Read0(sU8 *&p);
  sF32 Read(sU8 *&p);
};

void fread::Read0(sU8 *&p)
{
  min = fget(p); p+=2;
  max = fget(p); p+=2;
}

sF32 fread::Read(sU8*&p)
{
  return (*p++)*(max-min)/255+min;
}

void GenMesh::ReadCompact(sU8 *&pp,sU32 dgmf)
{
  sInt i,j,k,vl[4],sc,nsc,bc;
  sU32 sgmf;
  sVector *vp,v0;
#if !sINTRO_X
  sVector v;
#endif
//  sInt imin,imax;
  sF32 fmin,fmax;
  sF32 *fp,*vdim;
  fread srt[9];
	MeshCoder meshcode;
  RangeCoder coder;
  RangeModel vModel[5];
  sU8 *p;

  p = pp;

// read header
  KeyCount = *(sU16 *) p;
  CurveCount = p[2];
  bc = p[3];
  sgmf = *(sU16 *) (p+4);
  p+=6;

#if sINTRO
  dgmf = 0x23;
#endif
  Init(dgmf,64*1024);
  BoneMatrix.AtLeast(bc); BoneMatrix.Count = bc;
  BoneCurve.AtLeast(CurveCount); BoneCurve.Count = CurveCount;
  KeyBuf = new sF32[KeyCount*CurveCount];

// read topology
  p = meshcode.Decode(this,p);

// read vertices
  for(i=0;i<Vert.Count;i++)
    Vert[i].Temp = 1 + *p++;

  for(i=0;i<4;i++)
    vl[i]=0;
  for(i=0;i<Vert.Count;i++)
    for(j=0;j<Vert[i].Temp;j++)
    {
      vl[j] = (vl[j] + *p++) & 0xff;
      Vert[i].Matrix[j] = vl[j];
    }
  for(i=0;i<Vert.Count;i++)
  {
    if(Vert[i].Temp>0)
    {
      k = 0xff;
      for(j=0;j<Vert[i].Temp-1;j++)
      {
        k-= *p;
        Vert[i].Weight[j] = *p++;
      }
      Vert[i].Weight[Vert[i].Temp-1] = k;
    }
  }
// read vertex buffer

  vdim = (sF32*) p;
  p += 24;
  vp = VertBuf;

  coder.InitDecode(p);
  for(i=0;i<3;i++)
  {
    vModel[i].Init(VertQuantize[i]+1);
    vl[i]=0;
  }
  vModel[3].Init(2);

  for(i=0;i<Vert.Count;i++)
  {
    if(sgmf & sGMF_POS)
    {
			if(Vert[i].First==i)
			{
        if(vModel[3].Decode(coder))
        {
          k = coder.DecodePlain(i);
          v0 = VertBuf[k*VertSize];
          v0.x = -v0.x;
        }
        else
        {
          for(j=0;j<3;j++)
          {
            vl[j] += vModel[j].Decode(coder);
            if(vl[j] > VertQuantize[j]) vl[j] -= VertQuantize[j]+1;
            (&v0.x)[j] = (vl[j] + sFGetRnd() - 0.5f) * vdim[j+3] + vdim[j];
          }
        }

	      v0.w = 1;
 			}
			else
				v0 = VertBuf[Vert[i].First*VertSize];
    }
    else
      v0.Init(0,0,0,1);

    if(dgmf & sGMF_POS && sCODECOVER(0))
      *vp++ = v0;

#if !sINTRO_X
    v.Init(0,0,0,0);
    if((dgmf & sGMF_NORMAL) && sCODECOVER(1))
      *vp++ = v;

    if((dgmf & sGMF_TANGENT) && sCODECOVER(2))
      *vp++ = v;

    v.Init(1,1,1,1);
    if((dgmf & sGMF_COLOR0) && sCODECOVER(4))
      *vp++ = v;
    if((dgmf & sGMF_COLOR1) && sCODECOVER(5))
      *vp++ = v;

    v.Init(v0.x*0.1f,v0.y*0.1f,0,0);
    if((dgmf & sGMF_UV0) && sCODECOVER(7))
      *vp++ = v;
    if((dgmf & sGMF_UV1) && sCODECOVER(8))
      *vp++ = v;
    if((dgmf & sGMF_UV2) && sCODECOVER(9))
      *vp++ = v;
    if((dgmf & sGMF_UV3) && sCODECOVER(10))
      *vp++ = v;
#else
    /*if(dgmf & sGMF_NORMAL)
      vp++;

    if(dgmf & sGMF_TANGENT)
      vp++;

    if(dgmf & sGMF_UV0)
    {
      vp->Init(v0.x*0.1f,v0.y*0.1f,0,0);
      vp++;
    }*/
    vp += 2;
    vp[-1].Init(v0.x*0.1f,v0.y*0.1f,0,0);
#endif
  }
  p += coder.GetBytes();
  for(i=0;i<4;i++)
    vModel[i].Exit();

// keys

  for(i=0;i<bc;i++)
    BoneMatrix[i].Parent = i - 1 - *p++;

  for(j=0;j<9;j++)
    srt[j].Read0(p);
  for(j=0;j<9;j++)
  {
    for(i=0;i<bc;i++)
      BoneMatrix[i].BaseSRT[j] = srt[j].Read(p);
    for(i=0;i<bc;i++)
      BoneMatrix[i].TransSRT[j] = srt[j].Read(p);
  }

  for(i=0;i<BoneCurve.Count;i++)
  {
    BoneCurve[i].Curve = *p++;
  }

  vl[0]=0;
  for(i=0;i<BoneCurve.Count;i++)
  {
    vl[0] = (vl[0] + *p++) & 0xff;
    BoneCurve[i].Matrix = vl[0];
  }

  coder.InitDecode(p+CurveCount*4);
  for(i=0;i<5;i++)
    vModel[i].Init(AnimQuantize[i]*2+2);
  for(i=0;i<CurveCount;i++)
  {
    fmin = fget(p+0);
    fmax = fget(p+2);
    p += 4;

    for(j=0;j<5;j++)
    {
      if(fmax>=AnimThreshold[j])
      {
        sc = AnimQuantize[j];
        nsc = j;
      }
    }

    fp = &KeyBuf[i*KeyCount];
    *fp++ = fmin;
    k = 0;

    for(j=1;j<KeyCount;j++)
    {
      k += vModel[nsc].Decode(coder);
      if(k & (sc+1))
        k |= ~sc;
      else
        k &= sc;

      *fp++ = k * fmax / sc + fmin;
    }
  }
  p += coder.GetBytes();

  for(i=0;i<5;i++)
    vModel[i].Exit();

// end
#if !sPLAYER
  Verify();
#endif

  sVERIFY(p[0] == 'r');
  sVERIFY(p[1] == 'y');
  sVERIFY(p[2] == 'g');
  p+=3;

  pp = p;
}

/****************************************************************************/
/****************************************************************************/


/****************************************************************************/
/****************************************************************************/

sObject * __stdcall Mesh_Cube(GenMesh *mesh,sInt tx,sInt ty,sInt tz,sInt crease)
{
	sInt tess[3],i,j;
  sInt bm;
  sMatrix mat;
  sVector vc,vs;

	for(i=0;i<3;i++)
		tess[i]=(&tx)[i]>>16;

	SCRIPTVERIFY(mesh);
	SCRIPTVERIFY(mesh->Vert.Count==0);
  mesh->Ring(4,0,1.4142135623730950488016887242097f/2,sPI2F/8);

  mesh->Face[0].Select = sTRUE;

  mesh->Extrude(0x00010000);
  mat.Init();
  mat.l.z = 1;
  mesh->Mask2Sel(0x00010000);
  mesh->TransVert(mat);

  mat.l.Init(0.5f,0.5f,0.0,1.0f);
  mesh->All2Sel();
  mesh->TransVert(mat);
  mat.Init();
  mesh->TransVert(mat,sGMI_POS,sGMI_UV0);

// extrude tesselation

  vc.Init(1,1,1,1);
  for(j=0;j<3;j++)
  {
    vs.Init(1025.0f,1025.0f,1025.0f,1);
    (&vs.x)[j] = 0.1f;
    mesh->SelectCube(vc,vs,1,0,0);
    mat.Init();
    (&mat.l.x)[j] = 1.0f;

    mesh->RecBegin();
    for(i=1;i<tess[j];i++)
    {
      mesh->Extrude(0x00010000,0x00000100);
      mesh->Face2Vert();
      mesh->TransVert(mat);
      mesh->TransVert(mat,sGMI_UV0,sGMI_UV0);
    }
    mesh->RecEnd();
  }

// rescale t0 0.5 .. -0.5

  mat.Init();
  mat.i.x = 1.0f/tess[0];
  mat.j.y = 1.0f/tess[1];
  mat.k.z = 1.0f/tess[2];
  mesh->All2Sel();
  mesh->TransVert(mat,sGMI_UV0,sGMI_UV0);
  mat.l.Init(-0.5f,-0.5f,-0.5f,1);
  mesh->TransVert(mat);

// mapping

  if(crease)
  {
    for(j=0;j<6;j++)
    {
      bm = ((j&1)?1:-1);
      vc.Init(0,0,0,1);
      vs.Init(1025,1025,1025,1);
      (&vc.x)[j/2] = bm*0.5f;
      (&vs.x)[j/2] = 0.001f;
      mesh->SelectCube(vc,vs,1,0,0);
      mesh->Crease();
      mesh->Face2Vert();
      sSetMem4((sU32 *)&mat,0,16);
      mat.l.x = 0.5f;
      mat.l.y = 0.5f;
      if(j/2==0)
      {
        mat.j.y = -1.0f;
        mat.k.x = 1.0f*bm;
      }
      else if(j/2==1)
      {
        mat.i.x = 1.0f;
        mat.k.y = -1.0f*bm;
      }
      else
      {
        mat.i.x = -1.0f*bm;
        mat.j.y = -1.0f;
      }

      mesh->TransVert(mat,sGMI_POS,sGMI_UV0);
    }
  }
  else
  {
    mesh->All2Mask(0,0);
    mat.Init();
    mesh->TransVert(mat,sGMI_POS,sGMI_UV0);
  }
  mesh->All2Mask(0,~0);

	return mesh;
}

/****************************************************************************/

sObject * __stdcall Mesh_SelectAll(GenMesh *mesh,sU32 dmask1,sU32 dmask0)
{
  mesh->All2Mask(dmask1,dmask0);
	return mesh;
}

/****************************************************************************/

sObject * __stdcall Mesh_SelectCube(GenMesh *mesh,sF323 center,sF323 size,sInt flags,sU32 dmask1,sU32 dmask0)
{
	SCRIPTVERIFY(mesh);
	mesh->SelectCube((sVector&)center.x,(sVector&)size.x,flags>>16,dmask1,dmask0);

	return mesh;
}

/****************************************************************************/

sObject * __stdcall Mesh_Subdivide(GenMesh *mesh,sInt mask,sF32 alpha,sInt count)
{
	SCRIPTVERIFY(mesh);
	count >>= 16;

	if(mask)
		mesh->Mask2Sel(mask<<8);
  else
    mesh->All2Sel();

	while(count--)
		mesh->Subdivide(alpha);

	return mesh;
}

/****************************************************************************/

sObject * __stdcall Mesh_Extrude(GenMesh *mesh,sInt smask,sInt mode,sInt dmask)
{
	SCRIPTVERIFY(mesh);

	if(smask)
		mesh->Mask2Sel(smask<<8);
  else
    mesh->All2Sel();

  mesh->Extrude(0,smask<<8,mode>>16);
	mesh->Face2Vert();
  mesh->Sel2Mask(dmask<<16,0);

	return mesh;
}

/****************************************************************************/

sObject * __stdcall Mesh_Transform(GenMesh *mesh,sInt mask,sF323 s,sF323 r,sF323 t)
{
	sMatrix mat;
	SCRIPTVERIFY(mesh);

  mesh->MarkAnimLabel(GMA_TRANSFORM);
	mat.InitSRT(&s.x);
	if(mask)
		mesh->Mask2Sel(mask<<16);
  else
    mesh->All2Sel();
	mesh->TransVert(mat);

	return mesh;
}

/****************************************************************************/

sObject * __stdcall Mesh_Cylinder(GenMesh *mesh,sInt tx,sInt ty)
{
  sInt i,sj,e;
  sMatrix mat,matuv;
  sVector vc,vs;

	SCRIPTVERIFY(mesh);
  SCRIPTVERIFY(mesh->Vert.Count==0);
	tx >>= 16;
	ty >>= 16;

// create cylinder
  mesh->Ring(tx,0,0.5f,0);

  mat.Init();
  mat.l.z = 1;
  matuv.Init();
  matuv.l.y = 1.0f/ty;

  mesh->Face[0].Select = sTRUE;
  for(i=0;i<ty;i++)
  {
		e=mesh->Edge.Count;
    mesh->Extrude(0);
		mesh->Edge[e].Sel(0x000001);
    mesh->Face2Vert();
    mesh->TransVert(mat);
    mesh->TransVert(matuv,sGMI_POS,sGMI_UV0);
  }

// adjust size

  mat.Init();
  mat.k.z = 1.0f/ty;
  mat.l.Init(0,0,-0.5f,1);
  mesh->All2Mask(0x00010000);
  mesh->Mask2Sel(0x00010000);
  mesh->TransVert(mat);

// correct mapping

  sj = mesh->VertMap[sGMI_UV0];
  if(sj!=-1)
  {
		for(i=-1;i<2;i+=2)
    {
      vs.Init(2,2,0.01f,1);
      vc.Init(0,0,i*0.5f,1);
      mesh->All2Mask(0,0x010100);
      mesh->Mask2Sel(0x010100);
      mesh->SelectCube(vc,vs,1,0,0);
      mesh->Crease();
      mesh->Face2Vert();
      matuv.Init();
      matuv.j.y = i;
      matuv.l.Init(0.5f,0.5f,0,1);
      mesh->TransVert(matuv,sGMI_POS,sGMI_UV0);
    }

		mesh->All2Mask(0,0x010000);
    mesh->Mask2Sel(0x010001);
		mesh->Crease(1,0,0,sGMF_UV0); // uv0 only
		mesh->Edge2Vert(0);
    matuv.Init();
    matuv.l.x = 1.0f;
    mesh->TransVert(matuv,sGMI_UV0,sGMI_UV0);
 	}

  mesh->All2Mask(0,~0);
  return mesh;
}

/****************************************************************************/

sObject * __stdcall Mesh_NewMesh(sInt colorSets,sInt uvSets)
{
	GenMesh *mesh;

	mesh = new GenMesh;
  mesh->Init(GenMesh::Features2Mask(0,1),1024);

	return mesh;
}

/****************************************************************************/

sObject * __stdcall Mesh_Material(GenMesh *mesh,GenMaterial *mat,sInt id,sInt mask)
{
	SCRIPTVERIFY(mesh);
	SCRIPTVERIFY(mat);

	if(mask)
		mesh->Mask2Sel(mask<<8);
  else
    mesh->All2Sel();
	mesh->SetMaterial(id>>16,mat);

	return mesh;
}

/****************************************************************************/

sObject * __stdcall Mesh_TransformEx(GenMesh *mesh,sInt mask,sInt sj,sInt dj,sF323 s,sF323 r,sF323 t)
{
	sMatrix mat;

  mesh->MarkAnimLabel(GMA_TRANSFORM);
	SCRIPTVERIFY(mesh);

	mat.InitSRT(&s.x);
	if(mask)
		mesh->Mask2Sel(mask<<16);
  else
    mesh->All2Sel();
	mesh->TransVert(mat,sj>>16,dj>>16);

	return mesh;
}

/****************************************************************************/

sObject * __stdcall Mesh_Crease(GenMesh *mesh,sInt mask,sInt what,sInt selType)
{
	SCRIPTVERIFY(mesh);
	what >>= 16;
	SCRIPTVERIFY(what);

	if(mask)
    mesh->Mask2Sel(mask<<(selType ? 0 : 8));
  else
    mesh->All2Sel();
	mesh->Crease(selType>>16,0,0,what);

	return mesh;
}

/****************************************************************************/

sObject * __stdcall Mesh_UnCrease(GenMesh *mesh,sInt mask,sInt what,sInt selType)
{
	SCRIPTVERIFY(mesh);
	what >>= 16;
	SCRIPTVERIFY(what);

	if(mask)
		mesh->Mask2Sel(mask<<8);
  else
    mesh->All2Sel();
	mesh->UnCrease(selType>>16,what);

	return mesh;
}

/****************************************************************************/

sObject * __stdcall Mesh_CalcNormals(GenMesh *mesh,sInt mode,sInt mask,sInt calcWhat)
{
	SCRIPTVERIFY(mesh);

	if(mask)
		mesh->Mask2Sel(mask<<8);
  else
    mesh->All2Sel();

	mesh->CalcNormals(mode>>16,1/*calcWhat>>16*/);
	return mesh;
}

/****************************************************************************/

sObject * __stdcall Mesh_Torus(GenMesh *mesh,sInt tx,sInt ty,sF32 ri,sInt phase,sF32 arclen)
{
	sInt i,n,e,f,t;
	sInt sj;
	sBool closed;
	sMatrix mat,matuv;

	SCRIPTVERIFY(mesh);
  SCRIPTVERIFY(mesh->Vert.Count==0);

  ri *= 0.5f;
	phase &= 0xffff;
	tx >>= 16;
	ty >>= 16;

	closed = arclen == 1.0f;

// create torus
	mesh->Ring(ty,0,0.5f-ri,phase*sPI2/65536.0f);

	mat.Init();
	mat.l.x = -0.5f-ri;
	mesh->Face[0].Select = sTRUE;

	mesh->Face2Vert();
	mesh->TransVert(mat);

	mat.InitEuler(0.0f,arclen*sPI2/tx,0.0f);
	matuv.Init();
	matuv.l.y = 1.0f/tx;

	for(i=0;i<tx-closed;i++)
	{
		e = mesh->Edge.Count;
		mesh->Extrude(0);
		mesh->Edge[e].Sel(0x000001);
		mesh->Face2Vert();
		mesh->TransVert(mat);
		mesh->TransVert(matuv,sGMI_UV0,sGMI_UV0);
	}

// that was easy... now we need to merge start and end faces!
	if(closed)
	{
		// delete the start/end faces
		mesh->Face[1].Select = sTRUE;
		mesh->DeleteFaces();

		e = mesh->Edge.Count;
		f = mesh->Face.Count;

		mesh->Edge.AtLeast(e+ty);	mesh->Edge.Count+=ty;
		mesh->Face.AtLeast(f+ty); mesh->Face.Count+=ty;

		// make the new edges
		for(i=0;i<ty;i++)
		{
			mesh->Edge[e+i].Init();
			mesh->Edge[e+i].Vert[0] = mesh->Vert.Count-ty+i;
			mesh->Edge[e+i].Vert[1] = i;
		}
		mesh->Edge[e].Sel(0x000001);

		// make the faces (and mark the v crease while we're at it)
		for(i=0;i<ty;i++)
		{
			n = (i+1)%ty;
			t = mesh->NextFaceEdge(mesh->Face[2+i].Edge)^1;
			mesh->MakeFace(f+i,4,t,(e+i)*2+1,mesh->PrevFaceEdge(mesh->Face[f-ty+i].Edge)^1,(e+n)*2);
			mesh->Edge[t/2].Sel(0x000002);
		}
	}

// prepare the creases
	sj = mesh->VertMap[sGMI_UV0];
	if(sj!=-1)
	{
		if(!closed)
		{
			mesh->All2Mask(0,0x000100);
			mesh->Mask2Sel(0x000100);
			mesh->Face[0].Select = 1;
			mesh->Face[1].Select = 1;
			mesh->Crease(0,0,0,sGMF_UV0);
			// perform uv projection at caps
		}

		for(i=0;i<2;i++)
		{
			mesh->All2Mask(0,0x010000);
			mesh->Mask2Sel((i+1)|0x010000);
			mesh->Crease(1,0,0,sGMF_UV0);
			matuv.Init();
			(&matuv.l.x)[i] = 1.0f;
			mesh->Edge2Vert(i);
			mesh->TransVert(matuv,sGMI_UV0,sGMI_UV0);
		}
	}

  mesh->All2Mask(0,~0);
	return mesh;
}

/****************************************************************************/

sObject * __stdcall Mesh_Sphere(GenMesh *mesh,sInt tx,sInt ty)
{
	sInt i,j,v,sj;
	sMatrix mat,matuv;

	SCRIPTVERIFY(mesh);
  SCRIPTVERIFY(mesh->Vert.Count==0);

	tx >>= 16;
	ty >>= 16;

// first, generate a cheesy cylinder
	matuv.Init();
	matuv.l.y = 1.0f / ty;

	mesh->Ring(tx,0,1.0f,0.0f);
	mesh->Face[mesh->Face.Count-1].Select = sTRUE;
	for(i=0;i<ty;i++)
	{
		mesh->Extrude(0);
		mesh->Face2Vert();
		mesh->TransVert(matuv,sGMI_UV0,sGMI_UV0);
	}

// swap axes
	mat.Init();
	mat.j.y = 0.0f; mat.j.z = 1.0f;
	mat.k.y = 1.0f; mat.k.z = 0.0f;
	mesh->All2Mask(0x010000);
	mesh->Mask2Sel(0x010000);
	mesh->TransVert(mat);

// make it a sphere
	v=mesh->Vert.Count-tx*(ty+1);
	for(i=0;i<=ty;i++)
	{
		mesh->All2Mask(0,0x010000);
		mesh->Mask2Sel(0x010000);

		for(j=0;j<tx;j++,v++)
			mesh->Vert[v].Select=1;

		mat.Init();
		mat.i.x = mat.k.z = sFSin((i+0.5f)*sPI/(ty+1));
		mat.l.y = -sFCos((i+0.5f)*sPI/(ty+1));
		mesh->TransVert(mat);
	}

// fix top/bottom
	for(i=0;i<2;i++)
	{
		mesh->All2Mask(0,0x000100);
		mesh->Mask2Sel(0x000100);
		mesh->Face[i].Select=1;
		mesh->Triangulate(4);
	}

// make the u-crease
	sj=mesh->VertMap[sGMI_UV0];
	if(sj!=-1)
	{
		mesh->All2Mask(0,0x010000);
		mesh->Mask2Sel(0x010000);
		mesh->VertBuf[(mesh->Vert.Count-2)*mesh->VertSize+sj].x=0;
		mesh->VertBuf[(mesh->Vert.Count-1)*mesh->VertSize+sj].x=0;

		for(i=0;i<mesh->Edge.Count;i++)
		{
			if(mesh->VertBuf[mesh->GetVertId(i*2+0)*mesh->VertSize+sj].x==0 &&
				 mesh->VertBuf[mesh->GetVertId(i*2+1)*mesh->VertSize+sj].x==0)
			{
				mesh->Edge[i].Select=1;
			}
			else
				mesh->Edge[i].Select=0;
		}

		mesh->Crease(1,0,0,sGMF_UV0); // uv0 only
		mesh->Edge2Vert(0);
		mesh->Vert[0].Select=0;

		matuv.Init();
		matuv.l.x = 1.0f;
		mesh->TransVert(matuv,sGMI_UV0,sGMI_UV0);
	}

  mesh->All2Mask(0,~0);
	return mesh;
}

/****************************************************************************/

sObject * __stdcall Mesh_Triangulate(GenMesh *mesh,sInt thres,sInt mask,sInt type)
{
	SCRIPTVERIFY(mesh);

	if(mask)
		mesh->Mask2Sel(mask<<8);

	mesh->Triangulate(thres>>16,0,0,type>>16);
	return mesh;
}

/****************************************************************************/

sObject * __stdcall Mesh_Cut(GenMesh *mesh,sF322 dir,sF32 offs,sInt mode)
{
	sMatrix mat;
	sVector plane;

	SCRIPTVERIFY(mesh);
	
  mat.InitEulerPI2(&dir.x);
	plane.Init4(mat.i.x,mat.j.x,mat.k.x,offs);
  mesh->All2Sel();
	mesh->Cut(plane,mode>>16);

  return mesh;
}

/****************************************************************************/

sObject * __stdcall Mesh_ExtrudeNormal(GenMesh *mesh,sF32 distance,sInt mask)
{
	SCRIPTVERIFY(mesh);
	if(mask)
		mesh->Mask2Sel(mask<<16);
  mesh->MarkAnimLabel(GMA_TRANSFORM);
	mesh->ExtrudeNormal(distance);
	return mesh;
}

/****************************************************************************/

#if !sINTRO_X
sObject * __stdcall Mesh_Displace(GenMesh *mesh,GenBitmap *bmp,sF32 ampli,sInt mask)
{
	SCRIPTVERIFY(mesh);
	SCRIPTVERIFY(bmp);

	if(mask)
		mesh->Mask2Sel(mask<<16);
	mesh->Displace(bmp,ampli);
	return mesh;
}
#endif

/****************************************************************************/

#if !sINTRO_X
sObject * __stdcall Mesh_Bevel(GenMesh *mesh,sF32 elev,sF32 pull,sInt mask,sInt mode,sInt dmask)
{
	SCRIPTVERIFY(mesh);

  mesh->MarkAnimLabel(GMA_TRANSFORM);

	if(mask)
		mesh->Mask2Sel(mask<<8);
  else
    mesh->All2Sel();
	mesh->Bevel(elev,pull,mode>>16,0,mask<<8);
  mesh->Sel2Mask(dmask<<16,0);

	return mesh;
}
#endif

/****************************************************************************/

sObject * __stdcall Mesh_Perlin(GenMesh *mesh,sInt mask,sF323 s,sF323 r,sF323 t,sF323 ampl)
{
	sMatrix mat;

	SCRIPTVERIFY(mesh);
  mat.InitSRT(&s.x);
  mesh->MarkAnimLabel(GMA_PERLIN);
	if(mask)
		mesh->Mask2Sel(mask<<16);
	mesh->Perlin(mat,(sVector&) ampl.x);
	return mesh;
}

/****************************************************************************/

sObject * __stdcall Mesh_Add(GenMesh *mesh,GenMesh *mesh2)
{
	SCRIPTVERIFY(mesh);
	SCRIPTVERIFY(mesh2);
 
	if(mesh->Add(mesh2))
		return mesh;
	else
		return 0;
  return mesh;
}

/****************************************************************************/

sObject * __stdcall Mesh_DeleteFaces(GenMesh *mesh,sInt mask)
{
	SCRIPTVERIFY(mesh);

	if(mask)
		mesh->Mask2Sel(mask<<8);
  else
    mesh->All2Sel();
	mesh->DeleteFaces();

	return mesh;
}

/****************************************************************************/

#if sINTRO
extern "C" sU8 BabeMesh[];
#endif

sObject * __stdcall Mesh_Babe(sChar *filename,sInt dummy,sInt anim0,sInt anim1,sInt phase,sInt flags)
{
	GenMesh *mesh;
	sU8 *data,*origData;

	SCRIPTVERIFY(filename);
#if !sINTRO
	data = sSystem->LoadFile(filename);
#else
  data = BabeMesh;
#endif
  SCRIPTVERIFY(data);

	if(data)
	{
		mesh = new GenMesh;
		origData = data;
    mesh->ReadCompact(data,GenMesh::Features2Mask(0,1));
    if(!(flags & 0x10000))
      mesh->RecStoreMode();
    mesh->MarkAnimLabel(GMA_BONE);
    mesh->Anim0 = anim0>>16;
    mesh->Anim1 = anim1>>16;
    mesh->Bones((phase&0xffff)/65536.0f);
    mesh->All2Mask(0,~0);
#if !sINTRO
		delete[] origData;
#endif
		return mesh;
	}
	else
		return 0;
}

/****************************************************************************/

sObject * __stdcall Mesh_BeginRecord(GenMesh *mesh)
{
  SCRIPTVERIFY(mesh->RecMode==0);
  mesh->RecStoreMode();
  return mesh;
}

sObject * __stdcall Mesh_AnimLabel(GenMesh *mesh,sInt label,sU32 flags)
{
  mesh->SetAnimLabel(label,flags>>16);
  return mesh;
}

/****************************************************************************/

sObject * __stdcall Mesh_SelectRandom(GenMesh *mesh,sU32 ratio,sInt seed,sU32 dmask1,sU32 dmask0)
{
	sInt i;
  volatile sInt dummy;

	seed>>=16;
	ratio>>=16;
	sSetRndSeed(seed+seed*31743^(seed<<23));
	
#if sINTRO_X
  i = mesh->Vert.Count;
  if(dmask0&0x80)
    i *= 2;
  i += mesh->Edge.Count;
  while(i>0)
  {
    dummy=sGetRnd();
    i--;
  }
	for(i=0;i<mesh->Face.Count;i++)
    mesh->Face[i].Select=(sGetRnd()>>24)<=ratio && mesh->Face[i].Material;

#else
	for(i=0;i<mesh->Edge.Count;i++)
		mesh->Edge[i].Select=(sGetRnd()>>24)<=ratio;
  if(dmask0&0x80)
	  for(i=0;i<mesh->Vert.Count;i++)
      dummy=sGetRnd();
	for(i=0;i<mesh->Vert.Count;i++)
    mesh->Vert[i].Select=(sGetRnd()>>24)<=ratio;
  for(i=0;i<mesh->Vert.Count;i++)
    mesh->Vert[i].Select=mesh->Vert[mesh->Vert[i].First].Select;
	for(i=0;i<mesh->Face.Count;i++)
    mesh->Face[i].Select=(sGetRnd()>>24)<=ratio && mesh->Face[i].Material;
  dmask0 &= ~0x80;
#endif

	mesh->Sel2Mask(dmask1,dmask0);
	return mesh;
}

/****************************************************************************/

sObject * __stdcall Mesh_Multiply(GenMesh *mesh,sF323 s,sF323 r,sF323 t,sInt count)
{
	sInt i;
	GenMesh *out;
	sMatrix xform,step;

	count>>=16;

	step.InitSRT(&s.x);
	out = new GenMesh;
	out->Init(mesh->VertMask,mesh->VertCount*count);
	xform = step;

	while(count--)
	{
		out->Add(mesh);
		for(i=0;i<out->Vert.Count-mesh->Vert.Count;i++)
			out->Vert[i].Select=0;
		while(i<out->Vert.Count)
			out->Vert[i++].Select=1;

		out->TransVert(xform);
		xform.Mul4(step);
	}

	return out;
}

/****************************************************************************/

sObject * __stdcall Mesh_SelectAngle(GenMesh *mesh,sF32 angle,sU32 dmask1,sU32 dmask0)
{
  sInt i;
  sVector f0,f1;

  angle = cos(angle*sPI2F*0.25f);
  for(i=0;i<mesh->Edge.Count;i++)
  {
    mesh->CalcFaceNormal(f0,mesh->GetFaceId(i*2  )); f0.UnitSafe3();
    mesh->CalcFaceNormal(f1,mesh->GetFaceId(i*2+1)); f1.UnitSafe3();

    if(sFAbs(f0.Dot3(f1))<angle)
      mesh->Edge[i].Sel(dmask1,dmask0);
  }

  return mesh;
}

/****************************************************************************/

sObject * __stdcall Mesh_ExtrudeWK(GenMesh *mesh,sInt mask,sInt mode,sInt count,sF32 distance,sF323 s,sF323 r,sInt scalemode)
{
  sInt i,j,grp,e,ee,mi;
  sU32 *data;
  sBool local;
  static sInt stk[4096];
  sMatrix mat;

  mesh->MarkAnimLabel(GMA_EWK);
  if(mask)
    mesh->Mask2Sel(mask<<8);
  else
    mesh->All2Sel();

  mode >>= 16;
  count >>= 16;
  local = !(scalemode >> 16);

  // first, classify faces
  for(i=0;i<mesh->Face.Count;i++)
    mesh->Face[i].Temp = local ? -1 : 0;

  grp = local ? 0 : 1;
  while(1)
  {
    for(i=0;i<mesh->Face.Count;i++)
    {
      if(mesh->Face[i].Temp==-1 && mesh->Face[i].Select)
        break;
    }

    if(i==mesh->Face.Count)
      break;

    stk[0] = i;
    j = 1;
    while(j)
    {
      i = stk[--j];
      mesh->Face[i].Temp = grp;

      e = ee = mesh->Face[i].Edge;
      do
      {
        i = mesh->GetFaceId(e^1);
        if(mesh->Face[i].Temp==-1 && !mesh->IsBorderEdge(e,mode))
        {
          mesh->Face[i].Temp = -2;
          stk[j++] = i;
        }

        e = mesh->NextFaceEdge(e);
      }
      while(e!=ee);
    }

    grp++;
  }

  mesh->RecBegin();
  mat.i.Init(s.x,s.y,s.z,distance);
  mat.j.Init(r.x,r.y,r.z,count);
  mi = mesh->RecMatrix(mat);

  // main extrusion loop
  while(count--)
  {
    mesh->Extrude(0,0,mode);

    data = mesh->RecBegin(IM_EWK);
    mesh->RecOpCount = grp;

    // write parameters
    *data++ = local;
    *data++ = mi;

    // write groups
    for(i=0;i<mesh->Vert.Count;i++)
      mesh->Vert[i].Temp=0;

    for(i=0;i<grp;i++)
    {
      j = 0;

      for(e=0;e<mesh->Edge.Count*2;e++)
      {
        if(mesh->GetFace(e)->Select && mesh->GetFace(e)->Temp==i && !mesh->GetVert(e)->Temp)
        {
          mesh->GetVert(e)->Temp = 1;
          data[++j] = mesh->GetVertId(e);
        }
      }

      data[0] = j;
      data += j+1;
    }

    mesh->RecEnd(data);
  }

  mesh->RecEnd();

  return mesh;
}

/****************************************************************************/
/****************************************************************************/

#if sLINK_XSI

void GenMesh::ImportXSI(sXSILoader *xsi)
{
  sInt i,j,k,fi;
  sInt first;
  sXSIModel *xm;
  sXSICluster *xc;
  sXSIFCurve *xf;

  sVERIFY(KeyCount==0);
  sVERIFY(CurveCount==0);
  sVERIFY(KeyBuf==0);
  KeyCount = 0;
  CurveCount = 0;
  ImportXSIR(xsi->RootModel,-1);
  KeyBuf = new sF32[KeyCount*CurveCount];
  sSetMem4((sU32 *)KeyBuf,0,KeyCount*CurveCount);

  for(i=0;i<xsi->Models->GetCount();i++)
  {
    xm = xsi->Models->Get(i);
    sDPrintF("mesh %08x\n",xm);
    first = Vert.Count;
    for(j=0;j<xm->Clusters->GetCount();j++)
    {
      xc = xm->Clusters->Get(j);
      ImportXSI(xc,first);
    }
    Verify();

    for(j=0;j<xm->FCurves->GetCount();j++)
    {
      xf = xm->FCurves->Get(j);
      fi = xf->Index;
      sVERIFY(fi>=0 && fi<CurveCount);
      BoneCurve[fi].Curve = xf->Offset;
      BoneCurve[fi].Matrix = xm->Index;
      for(k=0;k<xf->KeyCount;k++)
      {
        sVERIFY(xf->Keys[k].Num>=0 && xf->Keys[k].Num<KeyCount);
        KeyBuf[KeyCount*fi+xf->Keys[k].Num] = xf->Keys[k].Pos;
      }
    }
  }

  sDPrintF("Bones: %d, Curves:%d, Keys:%d,Bytes %d\n",BoneMatrix.Count,CurveCount,KeyCount,KeyCount*CurveCount);
}

void GenMesh::ImportXSIR(sXSIModel *xm,sInt parent)
{
  sInt i,cm,cc,fcc;
  GenMeshMatrix *mat;
  sXSIFCurve *xfc;
  
  cm = BoneMatrix.Count;
  xm->Index = cm;
  BoneMatrix.AtLeast(cm+1);
  BoneMatrix.Count = cm+1;
  mat = &BoneMatrix[cm];
  mat->BaseSRT[0] = xm->BaseS.x;
  mat->BaseSRT[1] = xm->BaseS.y;
  mat->BaseSRT[2] = xm->BaseS.z;
  mat->BaseSRT[3] = xm->BaseR.x;
  mat->BaseSRT[4] = xm->BaseR.y;
  mat->BaseSRT[5] = xm->BaseR.z;
  mat->BaseSRT[6] = xm->BaseT.x;
  mat->BaseSRT[7] = xm->BaseT.y;
  mat->BaseSRT[8] = xm->BaseT.z;
  mat->TransSRT[0] = xm->TransS.x;
  mat->TransSRT[1] = xm->TransS.y;
  mat->TransSRT[2] = xm->TransS.z;
  mat->TransSRT[3] = xm->TransR.x;
  mat->TransSRT[4] = xm->TransR.y;
  mat->TransSRT[5] = xm->TransR.z;
  mat->TransSRT[6] = xm->TransT.x;
  mat->TransSRT[7] = xm->TransT.y;
  mat->TransSRT[8] = xm->TransT.z;
  mat->Matrix.Init();
  mat->Parent = parent;
  mat->Used = 0;

  fcc = xm->FCurves->GetCount();
  if(fcc>0)
  {
    cc = BoneCurve.Count;
    BoneCurve.AtLeast(cc+fcc);
    BoneCurve.Count = cc+fcc;
    for(i=0;i<fcc;i++)
    {
      xfc = xm->FCurves->Get(i);
      KeyCount = sMax(KeyCount,xfc->Keys[xfc->KeyCount-1].Num+1);
      xfc->Index = cc+i;
    }
    CurveCount+=fcc;
  }

  for(i=0;i<xm->Childs->GetCount();i++)
    ImportXSIR(xm->Childs->Get(i),cm);
}

void GenMesh::ImportXSI(sXSICluster *xc,sInt first)
{
  sInt i,j,vc,fc,max;
  sVector v;
  sVector *vp;
  sInt vi,vj,fi;
  sInt *fp;
  sInt edges[64];

  sDPrintF("cluster %08x\n",xc);

  vc = Vert.Count;

  Vert.AtLeast(vc+xc->VertexCount);
  Realloc(vc+xc->VertexCount);

// add vertices

  for(i=0;i<xc->VertexCount;i++)
  {
    v = xc->Vertices[i].Pos;
    for(j=first;j<Vert.Count;j++)
    {
      if(v.x==VertPos(j).x && v.y==VertPos(j).y && v.z==VertPos(j).z)
      {
        vi = j;
        goto vertfound;
      }
    }
    vi = Vert.Count;

vertfound:
    vj = Vert.Count++;
    xc->Vertices[i].Temp = vj;
    vp = &VertPos(vj);
    *vp = xc->Vertices[i].Pos;
    vp->w = 1;
    vp++;
    Vert[vj].Init();
    for(j=0;j<4;j++)
    {
      if(j<xc->Vertices[i].WeightCount)
      {
        Vert[vj].Matrix[j] = xc->Vertices[i].WeightModel[j]->Index;
        Vert[vj].Weight[j] = xc->Vertices[i].Weight[j]*255/100;
      }
      else
      {
        Vert[vj].Matrix[j] = 0xff;
        Vert[vj].Weight[j] = 0x00;
      }
    }
    if(VertMask & sGMF_NORMAL)
    {
      *vp = xc->Vertices[i].Normal;
      vp->w = 0;
      vp++;
    }
    if(VertMask & sGMF_COLOR0)
    {
      vp->Init4(((xc->Vertices[i].Color>>16)&0xff)/255.0f,
                ((xc->Vertices[i].Color>> 8)&0xff)/255.0f,
                ((xc->Vertices[i].Color    )&0xff)/255.0f,
                ((xc->Vertices[i].Color>>24)&0xff)/255.0f);
      vp++;
    }
    if(VertMask & sGMF_COLOR1)
    {
      vp->Init4(1,1,1,1);
      vp++;
    }
    if(VertMask & sGMF_UV0)
    {
      vp->Init(xc->Vertices[i].UV[0][0],xc->Vertices[i].UV[0][1],0,0);
      vp->Init(xc->Vertices[i].Pos.x*0.1,xc->Vertices[i].Pos.y*0.1,0,0);
      vp++;
    }
    if(VertMask & sGMF_UV1)
    {
      vp->Init(xc->Vertices[i].UV[1][0],xc->Vertices[i].UV[1][1],0,0);
      vp++;
    }
    if(VertMask & sGMF_UV2)
    {
      vp->Init(xc->Vertices[i].UV[2][0],xc->Vertices[i].UV[2][1],0,0);
      vp++;
    }
    if(VertMask & sGMF_UV3)
    {
      vp->Init(xc->Vertices[i].UV[3][0],xc->Vertices[i].UV[3][1],0,0);
      vp++;
    }
    sVERIFY(vp==&VertBuf[(vj+1)*VertSize]);
    if(vi==vj)
    {
       Vert[vj].Next = vi;
       Vert[vi].First = vj;
    }
    else
    {
      sVERIFY(Vert[vi].First == vi);
      Vert[vj].Next = Vert[vi].Next;
      Vert[vj].First = vi;
      Vert[vi].Next = vj;
    }
  }

// add faces

  fp = xc->Faces;
  fc = 0;

  while(fp<xc->Faces+xc->IndexCount*2)
  {
    max = *fp;
    sVERIFY(max>0);
    sVERIFY(max<64);
    fi = Face.Count;
    Face.AtLeast(fi+1);
    Face.Count=fi+1;
    Face[fi].Init();
    Face[fi].Material = 1;
    vi = fp[1];//max*2-1];
 
    for(i=0;i<max;i++)
    {
      sVERIFY(i==0 || fp[i*2]==0);
      vj = vi;
      vi = fp[max*2-1-2*i];
      edges[i] = AddEdge(xc->Vertices[vj].Temp,xc->Vertices[vi].Temp,fi);
    }
    fp += max*2;

    for(i=0;i<max;i++)
    {
      j = edges[i];
      Edge[j/2].Next[j&1] = edges[(i+1    )%max];
      Edge[j/2].Prev[j&1] = edges[(i-1+max)%max];
    }
  }
}

#endif

/****************************************************************************/
/****************************************************************************/
