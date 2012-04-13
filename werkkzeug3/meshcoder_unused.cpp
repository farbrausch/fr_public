// This file is distributed under a BSD license. See LICENSE.txt for details.

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

  sRangeCoder Coder;
  sRangeModel DV;
  sRangeModel DVFlag;
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
	Mesh->Init(msh->VertMask(),msh->VertCount);

	// copy vertices
	Mesh->Vert.AtLeast(msh->Vert.Count);
	Mesh->Vert.Count=msh->Vert.Count;

	for(i=0;i<msh->Vert.Count;i++)
	{
		Mesh->Vert[i]=msh->Vert[i];
		Mesh->Vert[i].First=i;
	}

	sCopyMem4((sU32 *)Mesh->VertBuf,(sU32 *)msh->VertBuf,4*msh->VertSize()*msh->VertCount);

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
static sInt AnimQuantize[5] = { 1,3,7,31,127 };
static sF32 AnimThreshold[5] = { 0.0f,0.01f,0.02f,0.04f,0.08f };

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
  sRangeCoder coder;
  sRangeModel vModel[5];

// check bounds

  sVERIFY(Vert.Count<0x7fff);
  sVERIFY(Face.Count<0x7fff);
  sVERIFY(KeyCount<0x7fff);
  sVERIFY(CurveCount<0xff);
  sVERIFY(BoneMatrix.Count<0xff);
  sVERIFY((VertMask() & ~(sGMF_POS|sGMF_NORMALS|sGMF_COLOR0|sGMF_UV0))==0);

// write header

  pstart = p;
  *p++ = KeyCount&0xff;
  *p++ = KeyCount>>8;
  *p++ = CurveCount;
  *p++ = BoneMatrix.Count;
  *p++ = VertMask()&0xff;
  *p++ = VertMask()>>8;
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
		vp = VertBuf + k * VertSize();

		if(VertMask() & sGMF_POS && cod->Mesh->Vert[k].First==k)
		{
      // search for mirror verts
      for(j=0;j<i;j++)
      {
        sVector *vp2 = VertBuf + cod->VertOrder[j] * VertSize();

        if(sFAbs(vp->x+vp2->x)<ex && sFAbs(vp->y-vp2->y)<ey && sFAbs(vp->z-vp2->z)<ez)
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
    if(VertMask() & (sGMF_NORMAL | sGMF_TANGENT | sGMF_COLOR0 | sGMF_UV0))
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
#if !sINTRO

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

  //  sInt imin,imax;
  sF32 fmin,fmax;
  sF32 *fp,*vdim;
  fread srt[9];
	MeshCoder meshcode;
  sRangeCoder coder;
  sRangeModel vModel[5];
  sU8 *p;
  sVector v;

  p = pp;

// read header
  KeyCount = *(sU16 *) p;
  CurveCount = p[2];
  bc = p[3];
  sgmf = *(sU16 *) (p+4);
  p+=6;

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
          v0 = VertBuf[k*VertSize()];
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
				v0 = VertBuf[Vert[i].First*VertSize()];
    }
    else
      v0.Init(0,0,0,1);

    if(dgmf & sGMF_POS && sCODECOVER(0))
      *vp++ = v0;

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
#endif

