/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "chaosmesh_code.hpp"
#include "chaosmesh_ops.hpp"
#include "base/graphics.hpp"
#include "wz4lib/serials.hpp"
#include "wz4lib/basic.hpp"
#include "wz4lib/basic_ops.hpp"


void sOptimizeIndexOrder(sInt ic,sInt vc,sInt *indices);
sF32 sSimulateVertexCache(sInt count,sInt *indices,sInt cachesize);

/****************************************************************************/
/***                                                                      ***/
/***   geometry structures                                                ***/
/***                                                                      ***/
/****************************************************************************/

void ChaosMeshVertexPosition::Init()
{
  Position.Init(0,0,0);
  for(sInt i=0;i<CHAOSMESHWEIGHTS;i++)
  {   
    MatrixIndex[i] = -1;
    MatrixWeight[i] = 0;
  }
  Select = 0;
}

void ChaosMeshVertexPosition::AddWeight(sInt index,sF32 value)
{
  sInt i=0;
  while(i<CHAOSMESHWEIGHTS)
  {
    if(MatrixIndex[i]==-1)
    {
      MatrixIndex[i] = index;
      MatrixWeight[i] = value;
      return;
    }
    if(value>MatrixWeight[i])
    {
      for(sInt j=CHAOSMESHWEIGHTS-2;j>=i;j--)
      {
        MatrixIndex[j+1] = MatrixIndex[j];
        MatrixWeight[j+1] = MatrixWeight[j];
      }
      MatrixIndex[i] = index;
      MatrixWeight[i] = value;
      return;
    }
    i++;
  }
}

void ChaosMeshVertexPosition::ClearWeights()
{
  for(sInt i=0;i<CHAOSMESHWEIGHTS;i++)
  {   
    MatrixIndex[i] = -1;
    MatrixWeight[i] = 0;
  }
}

sBool Equals(const ChaosMeshVertexPosition &a,const ChaosMeshVertexPosition &b)
{
  if(a.Position.x!=b.Position.x) return 0;
  if(a.Position.y!=b.Position.y) return 0;
  if(a.Position.z!=b.Position.z) return 0;
  for(sInt i=0;i<CHAOSMESHWEIGHTS;i++)
  {
    if(a.MatrixIndex[i]!=b.MatrixIndex[i]) return 0;
    if(a.MatrixIndex[i]==-1) return 1;
    if(a.MatrixWeight[i]!=b.MatrixWeight[i]) return 0;
  }
  return 1;
}

void ChaosMeshVertexNormal::Init()
{
  Normal.Init(0,0,0);
}

void ChaosMeshVertexTangent::Init()
{
  Tangent.Init(0,0,0);
  BiSign = 1;
}

void ChaosMeshVertexProperty::Init()
{
  C[0] = 0xffffffff;
  C[1] = 0xff000000;
  sClear(U);
  sClear(V);
}

void ChaosMeshFace::Init()
{
  Cluster = 0;
  Count = 4;
  Select = 0;
  for(sInt i=0;i<4;i++)
  {
    Positions[i] = 0;
    Normals[i] = 0;
    Tangents[i] = 0;
    Property[i] = 0;
  }
}


/****************************************************************************/

ChaosMeshCluster::ChaosMeshCluster()
{
  Material = 0;
  Geo = 0;
  WireGeo = 0;
//  JointId = -1;
}

ChaosMeshCluster::~ChaosMeshCluster()
{
  delete Geo;
  delete WireGeo;
  Material->Release();
}

void ChaosMeshCluster::CopyFrom(const ChaosMeshCluster *src)
{
  sVERIFY(Material == 0);
  Material = src->Material;
  if(Material) Material->AddRef();

  // charged data is not copied...
}

void ChaosMeshCluster::Charge()
{
  ChaosMeshVertexFat *fat;
  sInt *ip;
  sF32 *fp;
  const sU32 *desc;
  sVector4 data;

  if(!Geo)
  {
    if(!Material)
    {
      Material = new Wz4Material;
    }
    if(!Material->Material)
    {
      Material->Material = new Wz4Shader;
      Material->Material->Flags |= sMTRL_LIGHTING;
      Material->Material->Prepare(Material->Format);
    }

    Geo = new sGeometry;
    Geo->Init(sGF_TRILIST|sGF_INDEX32,Material->Format);

    Geo->BeginLoadIB(Indices.GetCount(),sGD_STATIC,(void **)&ip);
    if(Indices.GetCount())
      sCopyMem(ip,&Indices[0],Indices.GetCount()*4);
    Geo->EndLoadIB();

    desc = Material->Format->GetDesc();
    Geo->BeginLoadVB(Vertices.GetCount(),sGD_STATIC,(void **)&fp);
    sFORALL(Vertices,fat)
    {
      for(sInt i=0;desc[i];i++)
      {
        sVERIFY((desc[i]&sVF_STREAMMASK)==sVF_STREAM0);
        switch(desc[i]&sVF_USEMASK)
        {
          case sVF_POSITION:    data = fat->Position; break;
          case sVF_NORMAL:      
            data = fat->Normal; break;
          case sVF_TANGENT:     
            data.Init(fat->Tangent.x,fat->Tangent.y,fat->Tangent.z,fat->BiSign); break;
          case sVF_COLOR0:      data.InitColor(fat->C[0]); break;
          case sVF_COLOR1:      data.InitColor(fat->C[1]); break;
          case sVF_COLOR2:      data.Init(1,0,0,0); break;
          case sVF_COLOR3:      data.Init(1,0,0,0); break;
          case sVF_UV0:         data.Init(fat->U[0],fat->V[0],0,0); break;
          case sVF_UV1:         data.Init(fat->U[1],fat->V[1],0,0); break;
          case sVF_UV2:         data.Init(fat->U[2],fat->V[2],0,0); break;
          case sVF_UV3:         data.Init(fat->U[3],fat->V[3],0,0); break;
          default:              sVERIFYFALSE; break;

        case sVF_BONEINDEX:   
          data.Init(sClamp(fat->MatrixIndex[0],0,79)*3,
                    sClamp(fat->MatrixIndex[1],0,79)*3,
                    sClamp(fat->MatrixIndex[2],0,79)*3,
                    sClamp(fat->MatrixIndex[3],0,79)*3); 
          break;
        case sVF_BONEWEIGHT:  
          data.Init(fat->MatrixIndex[0]>=0 ? fat->MatrixWeight[0] : 0,
                    fat->MatrixIndex[1]>=0 ? fat->MatrixWeight[1] : 0,
                    fat->MatrixIndex[2]>=0 ? fat->MatrixWeight[2] : 0,
                    fat->MatrixIndex[3]>=0 ? fat->MatrixWeight[3] : 0);
          break;
        }
        switch(desc[i]&sVF_TYPEMASK)
        {
        case sVF_F2:
          *fp++ = data.x;
          *fp++ = data.y;
          break;
        case sVF_F3:
          *fp++ = data.x;
          *fp++ = data.y;
          *fp++ = data.z;
          break;
        case sVF_F4:
          *fp++ = data.x;
          *fp++ = data.y;
          *fp++ = data.z;
          *fp++ = data.w;
          break;
        case sVF_C4:
          *(sU32 *)fp = data.GetColor();
          fp++;
          break;
        case sVF_I4:
          ((sU8 *)fp)[0] = sInt(data.x)&255;
          ((sU8 *)fp)[1] = sInt(data.y)&255;
          ((sU8 *)fp)[2] = sInt(data.z)&255;
          ((sU8 *)fp)[3] = sInt(data.w)&255;
          fp++;
          break;
        default:
          sVERIFYFALSE;
          break;
        }
      }
    }
    Geo->EndLoadVB();
  }
}

void ChaosMeshCluster::ChargeWire()
{
  sInt ic;
  sInt *ip;
  sVertexBasic *vp;
  ChaosMeshVertexFat *fat;
  
  if(!WireGeo)
  {
    ic = Indices.GetCount()/3*6;

    WireGeo = new sGeometry;
    WireGeo->Init(sGF_LINELIST|sGF_INDEX32,sVertexFormatBasic);


    WireGeo->BeginLoadVB(Vertices.GetCount(),sGD_STATIC,(void **)&vp);
    sFORALL(Vertices,fat)
    {
      vp->Init(fat->Position,0xffffffff);
      vp++;
    }
    WireGeo->EndLoadVB();

    WireGeo->BeginLoadIB(ic,sGD_STATIC,(void **)&ip);
    for(sInt i=0;i<Indices.GetCount();i+=3)
    {
      *ip++ = Indices[i+0];
      *ip++ = Indices[i+1];
      *ip++ = Indices[i+1];
      *ip++ = Indices[i+2];
      *ip++ = Indices[i+2];
      *ip++ = Indices[i+0];
    }
    WireGeo->EndLoadIB();
  }
}

void ChaosMeshModel::CopyFrom(const ChaosMeshModel *s)
{
  Name = s->Name;
  StartCluster = s->StartCluster;
  EndCluster = s->EndCluster;
  StartPos = s->StartPos;
  EndPos = s->EndPos;
  StartNorm = s->StartNorm;
  EndNorm = s->EndNorm;
  StartTang = s->StartTang;
  EndTang = s->EndTang;
  StartProp = s->StartProp;
  EndProp = s->EndProp;
  JointId = s->JointId;
  BackLink = 0;
}


/****************************************************************************/
/***                                                                      ***/
/***   Mesh                                                               ***/
/***                                                                      ***/
/****************************************************************************/

ChaosMesh::ChaosMesh()
{
  Type = ChaosMeshType;
  Skeleton = 0;
}

ChaosMesh::~ChaosMesh()
{
  sDeleteAll(Clusters);
  sDeleteAll(Models);
  Skeleton->Release();
}

template <class streamer> void ChaosMesh::Serialize_(streamer &stream,sTexture2D *shadow)
{    
  ChaosMeshVertexPosition *pos;
  ChaosMeshVertexNormal *norm;
  ChaosMeshVertexTangent *tang;
  ChaosMeshVertexProperty *prop;
  ChaosMeshFace *face;
  ChaosMeshCluster *cl;
  ChaosMeshModel *mod;

  sInt version = stream.Header(sSerId::Wz4ChaosMesh,5);
  if(version)
  {
    // make the arrays

    stream.Array(Positions);
    stream.Array(Normals);
    stream.Array(Tangents);
    stream.Array(Properties);
    stream.Array(Faces);
    stream.ArrayNew(Clusters);
    stream.ArrayNew(Models);

    // stream arrays

    sFORALL(Positions,pos)
    {
      if((_i&0xff)==0) stream.Check();
      stream | pos->Position;
      if(stream.If(pos->MatrixIndex[0]!=-1))
      {
        for(sInt i=0;i<4;i++)
          stream | pos->MatrixIndex[i] | pos->MatrixWeight[i];
      }
      else if(stream.IsReading())
      {
        pos->MatrixIndex[0] = -1;
        pos->MatrixIndex[1] = -1;
        pos->MatrixIndex[2] = -1;
        pos->MatrixIndex[3] = -1;
        pos->MatrixWeight[0] = 0;
        pos->MatrixWeight[1] = 0;
        pos->MatrixWeight[2] = 0;
        pos->MatrixWeight[3] = 0;
      }
    }
    sFORALL(Normals,norm)
    {
      if((_i&0xfff)==0) stream.Check();
      stream | norm->Normal;
    }
    sFORALL(Tangents,tang)
    {
      if((_i&0x7ff)==0) stream.Check();
      stream | tang->Tangent | tang->BiSign;
    }
    sFORALL(Properties,prop)
    {
      if((_i&0xff)==0) stream.Check();
      stream | prop->C[0] | prop->C[1];
      for(sInt i=0;i<4;i++)
        stream | prop->U[i] | prop->V[i];
    }
    sFORALL(Faces,face)
    {
      if((_i&0xff)==0) stream.Check();
      stream | face->Cluster | face->Count;
      for(sInt i=0;i<face->Count;i++)
        stream | face->Positions[i] | face->Normals[i] | face->Property[i] | face->Tangents[i];
    }

    sFORALL(Clusters,cl)
    {
      if((_i&0xff)==0) stream.Check();
      stream.OnceRef(cl->Material,shadow);
      stream | cl->Bounds;
      if(version<2)
      {
        sInt oldjoint=-1;
        stream | oldjoint;
      }
    }

    sFORALL(Models,mod)
    {
      if((_i&0xff)==0) stream.Check();
      stream | mod->Name;
      stream | mod->StartCluster | mod->EndCluster;
      stream | mod->StartPos | mod->EndPos;
      if(version>=2)
      {
        stream | mod->StartNorm | mod->EndNorm;
        stream | mod->StartTang | mod->EndTang;
      }
      if(version>=3)
        stream | mod->JointId;
      if(version>=4)
      {
        stream | mod->StartProp | mod->EndProp;
      }
      if(stream.IsReading())
        mod->BackLink = this;
    }

    // stream skeleton

    stream.OnceRef(Skeleton);

    if(version>=5)
    {
      ChaosMeshAnimClip *ac;
      stream.Array(AnimClips);
      sFORALL(AnimClips,ac)
        stream | ac->Start | ac->End | ac->Speed | ac->Id;
    }

    // done

    stream.Footer();
  }
}

void ChaosMesh::Serialize(sWriter &stream,sTexture2D *shadow) { Serialize_(stream,shadow); }
void ChaosMesh::Serialize(sReader &stream,sTexture2D *shadow) { Serialize_(stream,shadow); }

/****************************************************************************/

void ChaosMesh::Clear()
{
  Positions.Clear();
  Normals.Clear();
  Properties.Clear();
  Faces.Clear();
  sDeleteAll(Clusters);
  sRelease(Skeleton);
}

sInt ChaosMesh::AddCluster(Wz4Material *mtrl)
{
  ChaosMeshCluster *cl = new ChaosMeshCluster;
  Clusters.AddTail(cl);
  sVERIFY(cl->Material == 0);
  cl->Material = mtrl;
  if(mtrl) mtrl->AddRef();
  return Clusters.GetCount()-1;
}

void ChaosMesh::AddGrid(const sMatrix34 &mat,sInt tx,sInt ty,sBool doublesided)
{
  sInt sides = (doublesided==1) ? 2 : 1;
  sVector31 v;

  sInt vi = Positions.GetCount();
  sInt ni = Normals.GetCount();
  sInt pi = Properties.GetCount();

  sInt n = (tx+1)*(ty+1);
  sInt ta = tx+1;

  ChaosMeshVertexPosition *vp = Positions.AddMany(n);
  ChaosMeshVertexNormal *np = Normals.AddMany(n*sides);
  ChaosMeshVertexProperty *pp = Properties.AddMany(n);
  ChaosMeshFace *fp = Faces.AddMany(tx*ty*sides);

  for(sInt i=0;i<n*sides;i++)
  {
    np[i].Init();
  }

  for(sInt y=0;y<=ty;y++)
  {
    sF32 fy = sF32(y)/ty;
    for(sInt x=0;x<=tx;x++)
    {
      sF32 fx = sF32(x)/tx;

      v.x = fx-0.5f;
      v.y = fy-0.5f;
      v.z = 0;
      vp->Init();
      vp->Position = v*mat;
      vp++;

      pp->Init();
      pp->U[0] = fx;
      pp->V[0] = 1-fy;
      pp++;
    }
  }

  sInt x0,x1,y0,y1;
  for(sInt i=0;i<sides;i++)
  {
    for(sInt y=0;y<ty;y++)
    {
      for(sInt x=0;x<tx;x++)
      {
        fp->Init();
        fp->Count = 4;

        y0 = y;
        y1 = y+1;
        if(i || doublesided==2)
        {
          x0 = x+1;
          x1 = x;
        }
        else
        {
          x0 = x;
          x1 = x+1;
        }

        fp->Positions[0] = y0*ta+x1+vi;
        fp->Positions[1] = y0*ta+x0+vi;
        fp->Positions[2] = y1*ta+x0+vi;
        fp->Positions[3] = y1*ta+x1+vi;

        fp->Normals[0]   = y0*ta+x1+ni + i*n;
        fp->Normals[1]   = y0*ta+x0+ni + i*n;
        fp->Normals[2]   = y1*ta+x0+ni + i*n;
        fp->Normals[3]   = y1*ta+x1+ni + i*n;

        fp->Property[0]  = y0*ta+x1+pi;
        fp->Property[1]  = y0*ta+x0+pi;
        fp->Property[2]  = y1*ta+x0+pi;
        fp->Property[3]  = y1*ta+x1+pi;

        fp++;
      }
    }
  }

  CalcNormals();
  CalcTangents();
}

void ChaosMesh::AddCube(const sMatrix34 &mat0,sInt tx,sInt ty,sInt tz,sBool wrapuv,sF32 sx,sF32 sy,sF32 sz)
{
  sMatrix34 mat;
  sInt *tess=&tx;
  sF32 *scaleuv = &sx;
  sF32 tu,tv,su,sv;

  const static sS8 cube[6][9] =
  {
    { 0,1,  1, 1,  0, 0,-1 ,1 , 0 },  
    { 2,1,  1, 1, -1, 0, 0 ,0 , 2 },  
    { 0,1,  1, 1,  0, 0, 1 ,3 , 2 },  
    { 2,1,  1, 1,  1, 0, 0 ,2 , 0 },  
    { 0,2,  1, 1,  0, 1, 0 ,0 , 0 },  
    { 0,2,  1, 1,  0,-1, 0 ,0 , 2 },  
  };
//  const static sS8 sign[2] = { -1,1 };

  for(sInt i=0;i<6;i++)
  {
    mat.Init();
    mat.i.x = mat.j.y = mat.k.z = 0;

    (&mat.i.x)[cube[i][0]] = cube[i][2];
    (&mat.j.x)[cube[i][1]] = cube[i][3];
    mat.l.Init(cube[i][4]*0.5f,cube[i][5]*0.5f,cube[i][6]*0.5f);

    sInt uv0 = Properties.GetCount();
    AddGrid(mat*mat0,tess[cube[i][0]],tess[cube[i][1]],cube[i][8]);
    sInt uv1 = Properties.GetCount();

    tu = tv = 0;
    su = scaleuv[cube[i][0]];
    sv = scaleuv[cube[i][1]];
    if(cube[i][8])
    {
      tu = su;
      su *= -1;
    }
    if(wrapuv)
    {
      switch(cube[i][7])    // no breaks here!
      {
        case 3:  tu += scaleuv[2];
        case 2:  tu += scaleuv[0];
        case 1:  tu += scaleuv[2];
      }
    }
    for(sInt j=uv0;j<uv1;j++)
    {
      Properties[j].U[0] = (Properties[j].U[0]*su)+tu;
      Properties[j].V[0] = (Properties[j].V[0]*sv)+tv;
    }
  }

  MergePositions();
}

void ChaosMesh::AddTorus(const sMatrix34 &mat,sInt tx,sInt ty,sF32 ri,sF32 ro,sF32 phase,sF32 arclen)
{
  sF32 fx,fy;
  sInt x1,y1;
  sVector31 v;
  sBool arc;

  sInt vi = Positions.GetCount();
  sInt ni = Normals.GetCount();
  sInt pi = Properties.GetCount();

  arc = (arclen!=1);
  sInt ta = tx+arc;

  ChaosMeshVertexPosition *vp = Positions.AddMany(ta*ty);
  ChaosMeshVertexNormal *np = Normals.AddMany(ta*ty);
  ChaosMeshVertexProperty *pp = Properties.AddMany((tx+1)*(ty+1));
  ChaosMeshFace *fp = Faces.AddMany(tx*ty);

  for(sInt y0=0;y0<=ty;y0++)
  {
    fy = sF32(y0)/ty;
    for(sInt x0=0;x0<=tx;x0++)
    {
      fx = sF32(x0)/tx;
      pp->Init();
      pp->U[0] = fx;
      pp->V[0] = fy;
      pp++;
    }
  }

  for(sInt y0=0;y0<ty;y0++)
  {
    fy = sF32(y0+phase)/ty*sPI2F;
    for(sInt x0=0;x0<tx+arc;x0++)
    {
      fx = sF32(x0)*arclen/tx*sPI2F;

      v.x = -sFCos(fx)*(ro+sFCos(fy)*ri);
      v.y = -              sFSin(fy)*ri;
      v.z =  sFSin(fx)*(ro+sFCos(fy)*ri);

      vp->Init();
      np->Init();
      vp->Position = v * mat;

      vp++;
      np++;
    }
  }

  for(sInt y0=0;y0<ty;y0++)
  {
    for(sInt x0=0;x0<tx;x0++)
    {
      x1 = (x0+1);
      if(!arc)
        x1 = x1 % tx;
      y1 = (y0+1)%ty;
      fp->Init();
      fp->Positions[0] = y0*ta+x1+vi;
      fp->Positions[1] = y0*ta+x0+vi;
      fp->Positions[2] = y1*ta+x0+vi;
      fp->Positions[3] = y1*ta+x1+vi;
      fp->Normals[0] = y0*ta+x1+ni;
      fp->Normals[1] = y0*ta+x0+ni;
      fp->Normals[2] = y1*ta+x0+ni;
      fp->Normals[3] = y1*ta+x1+ni;
      fp->Property[0] = (y0+0)*(tx+1)+x0+1+pi;
      fp->Property[1] = (y0+0)*(tx+1)+x0+0+pi;
      fp->Property[2] = (y0+1)*(tx+1)+x0+0+pi;
      fp->Property[3] = (y0+1)*(tx+1)+x0+1+pi;
      fp++;
    }
  }

  CalcNormals();
  CalcTangents();
}

void ChaosMesh::AddSphere(const sMatrix34 &mat,sInt tx,sInt ty)
{
  sVector31 v;

//  sInt vi = Positions.GetCount();
  sInt ni = Normals.GetCount();
  sInt pi = Properties.GetCount();

  sInt tp = ty+1;

  ChaosMeshVertexPosition *vp = Positions.AddMany(2+tx*tp);
  ChaosMeshVertexNormal *np = Normals.AddMany(2+tx*tp);
  ChaosMeshVertexProperty *pp = Properties.AddMany(2+(tx+1)*tp);
  ChaosMeshFace *fp = Faces.AddMany(tx*ty+2*tx);

  // resetters

  for(sInt i=0;i<2+tx*tp;i++)
    vp[i].Init();
  for(sInt i=0;i<2+tx*tp;i++)
    np[i].Init();
  for(sInt i=0;i<2+(tx+1)*tp;i++)
    pp[i].Init();

  // top /& bottom

  v.Init(0,0.5f,0);
  vp[0].Position = v * mat;
  vp[0].Select = 1;
  pp[0].U[0] = 0.5;
  pp[0].V[0] = 0;

  v.Init(0,-0.5f,0);
  vp[1].Position = v * mat;
  vp[1].Select = 1;
  pp[1].U[0] = 0.5;
  pp[1].V[0] = 1;

  // properties
  
  pp+=2;
  for(sInt y=0;y<tp;y++)
  {
    for(sInt x=0;x<=tx;x++)
    {
      pp->U[0] = sF32(x)/tx;
      pp->V[0] = sF32(y+0.5f)/(tp);
      pp++;
    }
  }

  // vertices

  vp+=2;
  for(sInt y=0;y<tp;y++)
  {
    for(sInt x=0;x<tx;x++)
    {
      sF32 fy = (y+0.5f)*sPIF/tp;
      sF32 fx = x*sPI2F/tx;
      v.x = -sFSin(fy)*sFSin(fx)*0.5f;
      v.y = sFCos(fy)*0.5f;
      v.z = -sFSin(fy)*sFCos(fx)*0.5f;

      vp->Position = v * mat;
      vp++;
    }
  }

  // faces around

  for(sInt y0=0;y0<ty;y0++)
  {
    for(sInt x0=0;x0<tx;x0++)
    {
      sInt x1 = (x0+1);
      sInt xr = x1%tx;
      sInt y1 = (y0+1);
      fp->Init();
      fp->Positions[0] = 2+y0*tx+xr+pi;
      fp->Positions[1] = 2+y0*tx+x0+pi;
      fp->Positions[2] = 2+y1*tx+x0+pi;
      fp->Positions[3] = 2+y1*tx+xr+pi;
      fp->Normals[0] = 2+y0*tx+xr+ni;
      fp->Normals[1] = 2+y0*tx+x0+ni;
      fp->Normals[2] = 2+y1*tx+x0+ni;
      fp->Normals[3] = 2+y1*tx+xr+ni;
      fp->Property[0] = 2+y0*(tx+1)+x1+pi;
      fp->Property[1] = 2+y0*(tx+1)+x0+pi;
      fp->Property[2] = 2+y1*(tx+1)+x0+pi;
      fp->Property[3] = 2+y1*(tx+1)+x1+pi;
      fp++;
    }
  }

  // top & button

  for(sInt x0=0;x0<tx;x0++)
  {
    sInt x1 = (x0+1);
    sInt xr = x1%tx;

    fp->Init();
    fp->Count = 3;
    fp->Select = 1;
    fp->Positions[0] = 0;
    fp->Positions[1] = 2+x0+pi;
    fp->Positions[2] = 2+xr+pi;
    fp->Normals[0] = 0;
    fp->Normals[1] = 2+x0+ni;
    fp->Normals[2] = 2+xr+ni;
    fp->Property[0] = 0;
    fp->Property[1] = 2+x0+pi;
    fp->Property[2] = 2+x1+pi;
    fp++;

    fp->Init();
    fp->Count = 3;
    fp->Select = 1;
    fp->Positions[0] = 1;
    fp->Positions[2] = 2+ty*tx+x0+pi;
    fp->Positions[1] = 2+ty*tx+xr+pi;
    fp->Normals[0] = 1;
    fp->Normals[2] = 2+ty*tx+x0+ni;
    fp->Normals[1] = 2+ty*tx+xr+ni;
    fp->Property[0] = 1;
    fp->Property[2] = 2+ty*(tx+1)+x0+pi;
    fp->Property[1] = 2+ty*(tx+1)+x1+pi;
    fp++;
  }

  // done

  CalcNormals();
  CalcTangents();
}

void ChaosMesh::AddCylinder(const sMatrix34 &mat,sInt tx,sInt ty,sInt toprings)
{
  sVector31 v;

//  sInt vi = Positions.GetCount();
  sInt ni = Normals.GetCount();
  sInt pi = Properties.GetCount();

  sInt tp = ty+1;

  ChaosMeshVertexPosition *vp = Positions.AddMany(2+tx*tp);
  ChaosMeshVertexNormal *np = Normals.AddMany(2+tx*tp);
  ChaosMeshVertexProperty *pp = Properties.AddMany(2+(tx+1)*tp);
  ChaosMeshFace *fp = Faces.AddMany(tx*ty+2*tx);

  // resetters

  for(sInt i=0;i<2+tx*tp;i++)
    vp[i].Init();
  for(sInt i=0;i<2+tx*tp;i++)
    np[i].Init();
  for(sInt i=0;i<2+(tx+1)*tp;i++)
    pp[i].Init();

  // top /& bottom

  v.Init(0,0.5f,0);
  vp[0].Position = v * mat;
  pp[0].U[0] = 0.5f;
  pp[0].V[0] = 0.5f;

  v.Init(0,-0.5f,0);
  vp[1].Position = v * mat;
  pp[1].U[0] = 0.5f;
  pp[1].V[0] = 0.5f;

  // properties
  
  pp+=2;
  for(sInt y=0;y<tp;y++)
  {
    for(sInt x=0;x<=tx;x++)
    {
      pp->U[0] = sF32(x)/tx;
      pp->V[0] = sF32(y+0.5f)/(tp);
      pp++;
    }
  }

  // vertices

  vp+=2;
  for(sInt y=0;y<tp;y++)
  {
    for(sInt x=0;x<tx;x++)
    {
      sF32 fy = (y+0.5f)*sPIF/tp;
      sF32 fx = x*sPI2F/tx;
      v.x = -sFSin(fy)*sFSin(fx)*0.5f;
      v.y = sFCos(fy)*0.5f;
      v.z = -sFSin(fy)*sFCos(fx)*0.5f;

      vp->Position = v * mat;
      vp++;
    }
  }

  // faces around

  for(sInt y0=0;y0<ty;y0++)
  {
    for(sInt x0=0;x0<tx;x0++)
    {
      sInt x1 = (x0+1);
      sInt xr = x1%tx;
      sInt y1 = (y0+1);
      fp->Init();
      fp->Positions[0] = 2+y0*tx+xr+pi;
      fp->Positions[1] = 2+y0*tx+x0+pi;
      fp->Positions[2] = 2+y1*tx+x0+pi;
      fp->Positions[3] = 2+y1*tx+xr+pi;
      fp->Normals[0] = 2+y0*tx+xr+ni;
      fp->Normals[1] = 2+y0*tx+x0+ni;
      fp->Normals[2] = 2+y1*tx+x0+ni;
      fp->Normals[3] = 2+y1*tx+xr+ni;
      fp->Property[0] = 2+y0*(tx+1)+x1+pi;
      fp->Property[1] = 2+y0*(tx+1)+x0+pi;
      fp->Property[2] = 2+y1*(tx+1)+x0+pi;
      fp->Property[3] = 2+y1*(tx+1)+x1+pi;
      fp++;
    }
  }

  // top & button

  for(sInt x0=0;x0<tx;x0++)
  {
    sInt x1 = (x0+1);
    sInt xr = x1%tx;

    fp->Init();
    fp->Count = 3;
    fp->Positions[0] = 0;
    fp->Positions[1] = 2+x0+pi;
    fp->Positions[2] = 2+xr+pi;
    fp->Normals[0] = 0;
    fp->Normals[1] = 2+x0+ni;
    fp->Normals[2] = 2+xr+ni;
    fp->Property[0] = 0;
    fp->Property[1] = 2+x0+pi;
    fp->Property[2] = 2+x1+pi;
    fp++;

    fp->Init();
    fp->Count = 3;
    fp->Positions[0] = 1;
    fp->Positions[2] = 2+ty*tx+x0+pi;
    fp->Positions[1] = 2+ty*tx+xr+pi;
    fp->Normals[0] = 1;
    fp->Normals[2] = 2+ty*tx+x0+ni;
    fp->Normals[1] = 2+ty*tx+xr+ni;
    fp->Property[0] = 1;
    fp->Property[2] = 2+ty*(tx+1)+x0+pi;
    fp->Property[1] = 2+ty*(tx+1)+x1+pi;
    fp++;
  }

  // done

  CalcNormals();
  CalcTangents();
}

/****************************************************************************/

void ChaosMesh::CopyFrom(const ChaosMesh *src,sInt sep)
{
  const ChaosMeshCluster *cl;
  ChaosMeshCluster *dcl;
  ChaosMeshModel *dcm;
  const ChaosMeshModel *scm;

  Positions = src->Positions;
  Normals = src->Normals;
  Tangents = src->Tangents;
  Properties = src->Properties;
  Faces = src->Faces;
  Skeleton = src->Skeleton; Skeleton->AddRef();

  sFORALL(src->Models,scm)
  {
    dcm = new ChaosMeshModel;
    dcm->CopyFrom(scm);
    dcm->BackLink = this;
    Models.AddTail(dcm);
  }

  Clusters.HintSize(src->Clusters.GetCount());
  sFORALL(src->Clusters,cl)
  {
    dcl = new ChaosMeshCluster;
    dcl->CopyFrom(cl);
    Clusters.AddTail(dcl);
    if(dcl->Material)
      dcl->Material->TempPtr = 0;
  }

  // splice material

  if(sep & SEPARATE_MATERIAL)
  {
    sFORALL(Clusters,dcl)
    {
      if(dcl->Material)
      {
        Wz4Material *oldmtrl = dcl->Material;
        if(dcl->Material->TempPtr == 0)
        {
          dcl->Material->TempPtr = new Wz4Material;
          dcl->Material->TempPtr->CopyFrom(dcl->Material);
          dcl->Material = dcl->Material->TempPtr;
        }
        else
        {
          dcl->Material = dcl->Material->TempPtr;
          dcl->Material->AddRef();
        }
        oldmtrl->Release();
      }
    }
  }

  // splice skeleton

  if(sep & SEPARATE_SKELETON)
  {
    Skeleton->Release();
    Skeleton = new Wz4Skeleton();
    Skeleton->CopyFrom(src->Skeleton);
  }

  // copy anim clips

  AnimClips = src->AnimClips;
}

void ChaosMesh::CopyFrom(const ChaosMeshModel *model)
{
//  const ChaosMeshCluster *cl;
  ChaosMeshCluster *dcl;
  ChaosMesh *src = model->BackLink;
  ChaosMeshFace *face,*dface;
  sInt cnt;

  cnt = model->EndPos-model->StartPos;
  Positions.AddMany(cnt);
  sCopyMem(&Positions[0],&src->Positions[model->StartPos],cnt*sizeof(ChaosMeshVertexPosition));

  cnt = model->EndNorm-model->StartNorm;
  Normals.AddMany(cnt);
  sCopyMem(&Normals[0],&src->Normals[model->StartNorm],cnt*sizeof(ChaosMeshVertexNormal));

  cnt = model->EndTang-model->StartTang;
  Tangents.AddMany(cnt);
  sCopyMem(&Tangents[0],&src->Tangents[model->StartTang],cnt*sizeof(ChaosMeshVertexTangent));

  cnt = model->EndProp-model->StartProp;
  Properties.AddMany(cnt);
  sCopyMem(&Properties[0],&src->Properties[model->StartProp],cnt*sizeof(ChaosMeshVertexProperty));

  cnt = 0;
  sFORALL(src->Faces,face)
  {
    if(face->Cluster>=model->StartCluster && face->Cluster<model->EndCluster)
      cnt++;
  }
  dface = Faces.AddMany(cnt);
  sFORALL(src->Faces,face)
  {
    if(face->Cluster>=model->StartCluster && face->Cluster<model->EndCluster)
    {
      *dface = *face++;
      for(sInt i=0;i<dface->Count;i++)
      {
        dface->Positions[i] -= model->StartPos;
        dface->Normals[i]   -= model->StartNorm;
        dface->Tangents[i]  -= model->StartTang;
        dface->Property[i]  -= model->StartProp;
      }
      dface->Cluster -= model->StartCluster;
      dface++;
    }
  }

  Clusters.HintSize(src->Clusters.GetCount());
  for(sInt i=model->StartCluster;i<model->EndCluster;i++)
  {
    dcl = new ChaosMeshCluster;
    dcl->CopyFrom(src->Clusters[i]);
    Clusters.AddTail(dcl);
  }

  Skeleton = src->Skeleton;
  Skeleton->AddRef();
}

void ChaosMesh::Add(const ChaosMesh *src)
{
  if(src->Positions.GetCount()==0)
    return;

  sInt nv = Positions.GetCount();    sInt cv = src->Positions.GetCount();
  sInt nn = Normals.GetCount();      sInt cn = src->Normals.GetCount();
  sInt nt = Tangents.GetCount();     sInt ct = src->Tangents.GetCount();
  sInt np = Properties.GetCount();   sInt cp = src->Properties.GetCount();
/*sInt nf=*/Faces.GetCount();        sInt cf = src->Faces.GetCount();
  sInt nc = Clusters.GetCount();     sInt cc = src->Clusters.GetCount();

  ChaosMeshVertexPosition *pv = Positions.AddMany(cv);
  ChaosMeshVertexNormal   *pn = Normals.AddMany(cn);
  ChaosMeshVertexTangent  *pt = Tangents.AddMany(ct);
  ChaosMeshVertexProperty *pp = Properties.AddMany(cp);
  ChaosMeshFace           *pf = Faces.AddMany(cf);

  sCopyMem(pv,&src->Positions [0],cv*sizeof(ChaosMeshVertexPosition));
  sCopyMem(pn,&src->Normals   [0],cn*sizeof(ChaosMeshVertexNormal));
  sCopyMem(pt,&src->Tangents  [0],ct*sizeof(ChaosMeshVertexTangent));
  sCopyMem(pp,&src->Properties[0],cp*sizeof(ChaosMeshVertexProperty));
  sCopyMem(pf,&src->Faces     [0],cf*sizeof(ChaosMeshFace));

  for(sInt i=0;i<cf;i++)
  {
    for(sInt j=0;j<pf->Count;j++)
    {
      pf->Positions[j] += nv;
      pf->Normals  [j] += nn;
      pf->Tangents [j] += nt;
      pf->Property [j] += np;
    }
    pf->Cluster += nc;
    pf++;
  }

  Clusters.HintSize(nc+cc);
  for(sInt i=0;i<cc;i++)
  {
    ChaosMeshCluster *cl = new ChaosMeshCluster;
    cl->CopyFrom(src->Clusters[i]);
    Clusters.AddTail(cl);
  }

//  if(Skeleton != src->Skeleton)
//    sRelease(Skeleton);
}

void ChaosMesh::Triangulate()
{
  sInt nfc = 0;
  ChaosMeshFace *face,*nf;
  sFORALL(Faces,face)
    nfc += face->Count-2;

  if(nfc != Faces.GetCount())
  {
    sArray<ChaosMeshFace> nfa;
    nfa.HintSize(nfc);
    nfa.AddMany(nfc);
    sFORALL(Faces,face)
    {
      for(sInt i=2;i<face->Count;i++)
      {
        nf = nfa.AddMany(1);
        nf->Init();
        nf->Cluster = face->Cluster;
        nf->Count = 3;
        nf->Select = face->Select;

        nf->Positions[0] = face->Positions[0];
        nf->Positions[1] = face->Positions[i-1];
        nf->Positions[2] = face->Positions[i];

        nf->Normals[0] = face->Normals[0];
        nf->Normals[1] = face->Normals[i-1];
        nf->Normals[2] = face->Normals[i];

        nf->Property[0] = face->Property[0];
        nf->Property[1] = face->Property[i-1];
        nf->Property[2] = face->Property[i];

        nf->Tangents[0] = face->Tangents[0];
        nf->Tangents[1] = face->Tangents[i-1];
        nf->Tangents[2] = face->Tangents[i];
      }
    }
    Faces = nfa;
  }
}

void ChaosMesh::CalcNormals()
{
  ChaosMeshFace *face;
  ChaosMeshVertexNormal *np;
  sVector30 d0,d1,n,nn;
/*
  sFORALL(Normals,np)
    np->Normal.Init(0,0,0);

  sFORALL(Faces,face)
  {
    nn.Init(0,0,0);
    for(sInt i=2;i<face->Count;i++)
    {
      d0 = Positions[face->Positions[0]].Position - Positions[face->Positions[i-1]].Position;
      d1 = Positions[face->Positions[0]].Position - Positions[face->Positions[i  ]].Position;
      n.Cross(d0,d1);
      n.Unit();
      nn += n;
    }
    nn.Unit();
    for(sInt i=0;i<face->Count;i++)
      Normals[face->Normals[i]].Normal += nn;
  }

  sFORALL(Normals,np)
    np->Normal.Unit();

    */

  sFORALL(Normals,np)
  {
    sInt ni = _i;
    sVector30 accu;
    accu.Init(0,0,0);
    sFORALL(Faces,face)
    {
      sInt i0 = face->Count-2;
      sInt i1 = face->Count-1;
      for(sInt i2=0;i2<face->Count;i2++)
      {
        if(face->Normals[i1]==ni)
        {
          d0 = Positions[face->Positions[i0]].Position - Positions[face->Positions[i1]].Position;
          d1 = Positions[face->Positions[i1]].Position - Positions[face->Positions[i2]].Position;
          d0.Unit();
          d1.Unit();
          n.Cross(d0,d1);
          accu += n;// * (1.0f/(d0.LengthSq()*d1.LengthSq()));
        }
        i0 = i1;
        i1 = i2;
      }
    }
    accu.Unit();
    np->Normal = accu;
  }

}


struct CalcTangentsTemp
{
  sInt Normal;
  sF32 u;
  sF32 v;
  sBool operator==(const CalcTangentsTemp &v) const
  { return sCmpMem(this,&v,sizeof(CalcTangentsTemp))==0; }
  sU32 Hash() const
  { sU32 *x = (sU32 *)this; return x[0]^x[1]^x[2]; }
};

void ChaosMesh::CalcTangents()
{
  ChaosMeshFace *fp;
  ChaosMeshVertexTangent *tp;



  // scan required tangents

  sHashTable<CalcTangentsTemp,CalcTangentsTemp> hash;

  CalcTangentsTemp *ta = new CalcTangentsTemp[Faces.GetCount()*4];
  sInt ti=0;
  sFORALL(Faces,fp)
  {
    for(sInt i=0;i<fp->Count;i++)
    {
      if(0)
      {
        sInt n = fp->Normals[i];
        sF32 u = Properties[fp->Property[i]].U[0];
        sF32 v = Properties[fp->Property[i]].V[0];
        for(sInt j=0;j<ti;j++)
        {
          if(ta[j].Normal==n && ta[j].u==u && ta[j].v==v)
          {
            fp->Tangents[i] = j;
            goto found;
          }
        }
        fp->Tangents[i] = ti;
        ta[ti].Normal = fp->Normals[i];
        ta[ti].u = u;
        ta[ti].v = v;
        ti++;
        found:;
      }
      else
      {
        sInt n = fp->Normals[i];
        sF32 u = Properties[fp->Property[i]].U[0];
        sF32 v = Properties[fp->Property[i]].V[0];

        CalcTangentsTemp key; key.Normal=n; key.u=u; key.v=v;
        CalcTangentsTemp *t = hash.Find(&key);
        if(t)
        {
          fp->Tangents[i] = t-ta;
        }
        else
        {
          fp->Tangents[i] = ti;
          ta[ti] = key;
          hash.Add(&key,&ta[ti]);
          ti++;
        }
      }
    }
  }

  // allocate tangents

  Tangents.Clear();
  Tangents.AddMany(ti);

  // calc tangent space

  sFORALL(Tangents,tp)
  {
    tp->Tangent.Init(0,0,0);
    tp->BiSign = 1;
  }

  sFORALL(Faces,fp)
  {
    sVector30 t,dp;
    sF32 du;

    for(sInt i0=0;i0<fp->Count;i0++)
    {
      sInt i1 = (i0+1)%fp->Count;
      dp = Positions [fp->Positions[i0]].Position - Positions [fp->Positions[i1]].Position;
      du = Properties[fp->Property[i0]].U[0]  - Properties[fp->Property[i1]].U[0];

      t = dp * (du/(dp^dp));
      Tangents[fp->Tangents[i0]].Tangent += t;
      Tangents[fp->Tangents[i1]].Tangent += t;
    }
  }

  sFORALL(Tangents,tp)
    tp->Tangent.Unit();

  delete[] ta;
}

void ChaosMesh::MergePositions()
{
  sInt count = Positions.GetCount();
  sInt *remap = new sInt[count];
  sInt *domap = new sInt[count];
  sInt used = 0;
  sInt found;

  for(sInt i=0;i<count;i++)
  {
    found = -1;
    for(sInt j=0;j<i && found==-1;j++)
      if(Equals(Positions[i],Positions[j]))
        found = j;

    if(found>=0)
    {
      remap[i] = remap[found];
    }
    else
    {
      domap[used] = i;
      remap[i] = used;
      used++;
    }
  }

  if(used!=count)
  {
    for(sInt i=0;i<used;i++)
      Positions[i] = Positions[domap[i]];
    Positions.Resize(used);

    ChaosMeshFace *face;
    sFORALL(Faces,face)
      for(sInt i=0;i<face->Count;i++)
        face->Positions[i] = remap[face->Positions[i]];
  }  

  delete[] remap;
  delete[] domap;
}

void ChaosMesh::MergeNormals()
{
  sInt *map = new sInt[Normals.GetCount()];
  sInt *remap = new sInt[Normals.GetCount()];
  sInt newcount;
  ChaosMeshVertexNormal *np,*nc;
  ChaosMeshFace *face;

  newcount = 0;
  sFORALL(Normals,np)
  {
    for(sInt i=0;i<newcount;i++)
    {
      nc = &Normals[map[i]];
      if(np->Normal.x==nc->Normal.x && np->Normal.y==nc->Normal.y && np->Normal.z==nc->Normal.z)
      {
        remap[_i] = i;
        goto found;
      }
    }
    map[newcount] = _i;
    remap[_i] = newcount;
    newcount++;
found:;
  }

  for(sInt i=0;i<newcount;i++)
    Normals[i] = Normals[map[i]];
  sFORALL(Faces,face)
    for(sInt i=0;i<face->Count;i++)
      face->Normals[i] = remap[face->Normals[i]];

  delete[] map;
  delete[] remap;
}

void ChaosMesh::Cleanup()
{
  ChaosMeshFace *face;
 
  // prepare remapping tables

  sInt vc = Positions.GetCount();
  sInt nc = Normals.GetCount();
  sInt pc = Properties.GetCount();

  sInt *mapv = sALLOCSTACK(sInt,vc);
  sInt *mapn = sALLOCSTACK(sInt,nc);
  sInt *mapp = sALLOCSTACK(sInt,pc);

  sInt vn = 0;
  sInt nn = 0;
  sInt pn = 0;

  for(sInt i=0;i<vc;i++) mapv[i]=0;
  for(sInt i=0;i<nc;i++) mapn[i]=0;
  for(sInt i=0;i<pc;i++) mapp[i]=0;

  // find used elements

  sFORALL(Faces,face)
  {
    for(sInt i=0;i<face->Count;i++)
    {
      mapv[face->Positions[i]] = 1;
      mapn[face->Normals[i]] = 1;
      mapp[face->Property[i]] = 1;
    }
  }

  // create mappings

  for(sInt i=0;i<vc;i++)
  {
    if(mapv[i])
    {
      Positions[vn] = Positions[i];
      mapv[i] = vn++;
    }
    else
    {
      mapv[i] = -1;
    }
  }
  Positions.Resize(vn);

  for(sInt i=0;i<nc;i++)
  {
    if(mapn[i])
    {
      Normals[nn] = Normals[i];
      mapn[i] = nn++;
    }
    else
    {
      mapn[i] = -1;
    }
  }
  Normals.Resize(nn);

  for(sInt i=0;i<pc;i++)
  {
    if(mapp[i])
    {
      Properties[pn] = Properties[i];
      mapp[i] = pn++;
    }
    else
    {
      mapp[i] = -1;
    }
  }
  Properties.Resize(pn);

  // remap

  sFORALL(Faces,face)
  {
    for(sInt i=0;i<face->Count;i++)
    {
      face->Positions[i] = mapv[face->Positions[i]];
      face->Normals[i]   = mapn[face->Normals[i]];
      face->Property[i]  = mapp[face->Property[i]];
    }
  }

}

/****************************************************************************/


void ChaosMesh::SplitForMatrices()
{
  sInt mc,maxcluster,used,total;
  Wz4AnimJoint *joint;
  ChaosMeshCluster *cl,*clnew;
  ChaosMeshFace *face;
  ChaosMeshVertexPosition *vp;
  sBool found;
  sAABBox box;
  sVector4 plane,pos;
  sVector30 quer;

  if(!Skeleton)
    return;

  // find unused matrices and delete them

  sFORALL(Skeleton->Joints,joint)
    joint->Temp = 0;

  sFORALL(Faces,face)
  {
    for(sInt i=0;i<face->Count;i++)
    {
      for(sInt j=0;j<4;j++)
      {
        sInt mi = Positions[face->Positions[i]].MatrixIndex[j];
        if(mi>=0)
        {
          Skeleton->Joints[mi].Temp = 1;
          for(;;)
          {
            mi = Skeleton->Joints[mi].Parent;
            if(mi==-1)
              break;
            if(Skeleton->Joints[mi].Temp)
              break;
            Skeleton->Joints[mi].Temp = 1;
          }
        }
      }
    }
  }

  mc = 0;
  sFORALL(Skeleton->Joints,joint)
  {
    if(joint->Temp)
      joint->Temp = mc++;
    else
      joint->Temp = -1;
  }
  sDPrintF(L"removing %d joints\n",Skeleton->Joints.GetCount()-mc);

  sFORALL(Positions,vp)
  {
    for(sInt j=0;j<4;j++)
    {
      sInt mi = vp->MatrixIndex[j];
      if(mi>=0)
      {
        sVERIFY(Skeleton->Joints[mi].Temp>=0);

        vp->MatrixIndex[j] = Skeleton->Joints[mi].Temp;
      }
    }
  }

  sFORALL(Skeleton->Joints,joint)
  {
    sInt mi = joint->Parent;
    if(mi>=0 && joint->Temp>=0)
    {
      sVERIFY(Skeleton->Joints[mi].Temp>=0);
      joint->Parent = Skeleton->Joints[mi].Temp;
    }
  }

  sArray<Wz4AnimJoint> nj;
  sFORALL(Skeleton->Joints,joint)
    if(joint->Temp>=0)
      nj.AddTail(*joint);
  Skeleton->Joints = nj;

  // split too-big-clusters
 
  {
restartloop:
    maxcluster = Clusters.GetCount();
    found = 0;
    for(sInt ci=0;ci<maxcluster;ci++)
    {
      cl = Clusters[ci];

      sFORALL(Skeleton->Joints,joint)
        joint->Temp = -1;

      mc = 0;
      sFORALL(Faces,face)
      {
        if(face->Cluster==ci)
        {
          for(sInt i=0;i<face->Count;i++)
          {
            for(sInt j=0;j<4;j++)
            {
              sInt mi = Positions[face->Positions[i]].MatrixIndex[j];
              if(mi!=-1)
              {
                if(Skeleton->Joints[mi].Temp==-1)
                  Skeleton->Joints[mi].Temp = mc++;
              }
            }
          }
        }
      }

      if(mc>=80)
      {
        found = 1;

        // find bounding box

        box.Clear();
        sFORALL(Faces,face)
          if(face->Cluster==ci)
            for(sInt i=0;i<face->Count;i++)
              box.Add(Positions[face->Positions[i]].Position);

        // figure split plane

        quer = box.Max-box.Min;
        if(quer.x>quer.y && quer.x>quer.z)
          plane.Init(1,0,0,-(box.Min.x + box.Max.x)*0.5f);
        else if(quer.y > quer.z)
          plane.Init(0,1,0,-(box.Min.y + box.Max.y)*0.5f);
        else
          plane.Init(0,0,1,-(box.Min.z + box.Max.z)*0.5f);

        // create new cluster

        clnew = new ChaosMeshCluster;
        clnew->Bounds = cl->Bounds;
        clnew->Material = cl->Material;
        clnew->Material->AddRef();
        InsertClusterAfter(clnew,ci);

        // split by plane

        used = 0;
        total = 0;
        sFORALL(Faces,face)
        {
          if(face->Cluster==ci)
          {
            pos.Init(0,0,0,0);
            for(sInt i=0;i<face->Count;i++)
              pos = pos + Positions[face->Positions[i]].Position;
            if((plane ^ pos) > 0)
            {
              face->Cluster++;
              used++;
            }
            total++;
          }
        }
        sDPrintF(L"split %d/%d -> %f\n",used,total,plane);

        if(used==0)
        {
          sDPrintF(L"could not split cluster!\n");
          return;
        }

        goto restartloop;
      }
    }
  }
  while(found);
}

void ChaosMesh::InsertClusterAfter(ChaosMeshCluster *cl,sInt pos)
{
  ChaosMeshFace *face;
  ChaosMeshModel *mod;

  Clusters.AddAfter(cl,pos);

  sFORALL(Faces,face)
  {
    if(face->Cluster>pos)
      face->Cluster++;
  }
  sFORALL(Models,mod)
  {
    if(mod->StartCluster>pos)
      mod->StartCluster++;
    if(mod->EndCluster>pos)
      mod->EndCluster++;
  }
}

/****************************************************************************/

void ChaosMesh::ListTextures(sArray<Texture2D *> &ar)
{
  ChaosMeshCluster *cl;
  Wz4Material *mtrl;

  sFORALL(Clusters,cl)
  {
    mtrl = cl->Material;
    for(sInt i=0;i<sCOUNTOF(mtrl->Tex);i++)
    {
      if(mtrl->Tex[i] && mtrl->Tex[i]->Type==Texture2DType)
      {
        Texture2D *tex = (Texture2D *) mtrl->Tex[i];
        if(!sFindPtr(ar,tex))
        {
          ar.AddTail(tex);
        }
      }
    }
  }
}

void ChaosMesh::ListTextures(sArray<TextureCube *> &ar)
{
  ChaosMeshCluster *cl;
  Wz4Material *mtrl;

  sFORALL(Clusters,cl)
  {
    mtrl = cl->Material;
    for(sInt i=0;i<sCOUNTOF(mtrl->Tex);i++)
    {
      if(mtrl->Tex[i] && mtrl->Tex[i]->Type==TextureCubeType)
      {
        TextureCube *tex = (TextureCube *) mtrl->Tex[i];
        if(!sFindPtr(ar,tex))
        {
          ar.AddTail(tex);
        }
      }
    }
  }
}

/****************************************************************************/

void ChaosMesh::UpdateBuffers(sBool preTransformOpt)
{
  ChaosMeshCluster *cl;
  ChaosMeshFace *face;
  ChaosMeshVertexFat *vp;
  Wz4AnimJoint *joint;
  sInt *ip;
  sInt ci,vc,ic,index,vertex;

  sFORALL(Clusters,cl)
  {
    if(cl->Vertices.GetCount()>0 && cl->Indices.GetCount()>0) continue;
    ci = _i;
    
    // count vertices

    vc = 0;
    ic = 0;
    sFORALL(Faces,face)
    {
      if(face->Cluster==ci)
      {
        vc += face->Count;
        ic += (face->Count-2)*3;
      }
    }
    if(vc==0 || ic==0) continue;

    // create fat vertices (stupid)

    cl->Vertices.Clear();
    cl->Matrices.Clear();
    vp = cl->Vertices.AddMany(vc);
    ip = cl->Indices.AddMany(ic);
    index = 0;
    vertex = 0;
    sBool boundsset = 0;

    if(Skeleton)
      sFORALL(Skeleton->Joints,joint)
        joint->Temp = -1;

    sFORALL(Faces,face)
    {
      if(face->Cluster==ci)
      {
        for(sInt i=0;i<face->Count;i++)
        {
          vp->Index = vertex++;
          vp->Position = Positions[face->Positions[i]].Position;
          
          if(boundsset)
          {
            cl->Bounds.Add(vp->Position);
          }
          else
          {
            cl->Bounds.Min = cl->Bounds.Max = vp->Position;
            boundsset = 1;
          }

          if(Skeleton)
          {
            for(sInt j=0;j<4;j++)
            {
              sInt mi = Positions[face->Positions[i]].MatrixIndex[j];
              sF32 mw = Positions[face->Positions[i]].MatrixWeight[j];

              if(mi!=-1)
              {
                if(Skeleton->Joints[mi].Temp==-1)
                {
                  Skeleton->Joints[mi].Temp = cl->Matrices.GetCount();
                  *cl->Matrices.AddMany(1) = mi;
                }
                mi = Skeleton->Joints[mi].Temp;
              }

              vp->MatrixIndex[j] = mi;
              vp->MatrixWeight[j] = mw;
            }
          }
          else
          {
            for(sInt j=0;j<4;j++)
            {
              vp->MatrixIndex[j] = -1;
              vp->MatrixWeight[j] = 0;
            }
          }

          vp->Normal = Normals[face->Normals[i]].Normal;
          if(Tangents.GetCount()>0)
          {
            vp->Tangent = Tangents[face->Tangents[i]].Tangent;
            vp->BiSign = Tangents[face->Tangents[i]].BiSign<0 ? -1 : 1;     // make sure sign is -1 or 1, so that we can eliminate double vertices!
          }
          else
          {
            vp->Tangent.Init(1,0,0);
            vp->BiSign = 1;
          }

          sCopyMem(vp->C,Properties[face->Property[i]].C,sizeof(vp->C));
          sCopyMem(vp->U,Properties[face->Property[i]].U,sizeof(vp->U));
          sCopyMem(vp->V,Properties[face->Property[i]].V,sizeof(vp->V));

          vp++;
        }
        for(sInt i=2;i<face->Count;i++)
        {
          *ip++ = index;
          *ip++ = index+i-1;
          *ip++ = index+i;
        }
        index += face->Count;
      }
    }

    // optimize matrices

    //...

    // optimize double vertices

    if(cl->Vertices.GetCount()>0)
    {
      sInt *ip;
      sInt max = cl->Vertices.GetCount();
      ChaosMeshVertexFat current;
      sInt *remap = new sInt[max];

      

      sHeapSortUp(cl->Vertices);

      sInt vout = 0;
      sInt vin = 0;
      while(vin<max)
      {
        if(vin>0 && cl->Vertices[vin]==current)
        {
          remap[cl->Vertices[vin].Index] = vout-1;
          vin++;
        }
        else
        {
          remap[cl->Vertices[vin].Index] = vout;
          current = cl->Vertices[vin++];
          cl->Vertices[vout++] = current;
        }
      }
      cl->Vertices.Resize(vout);

      sFORALL(cl->Indices,ip)
        *ip = remap[*ip];

      delete[] remap;
    }

    // reorder indices for post transform cache

    sOptimizeIndexOrder(cl->Indices.GetCount(),cl->Vertices.GetCount(),&cl->Indices[0]);

    // reorder vertices for pre transform cache

    if(preTransformOpt)   // detuned: we can't do this now, because shapes must run in sync with the reference
    {
      sInt n=0;
      sFORALL(cl->Vertices,vp)
        vp->Index = -1;
      sFORALL(cl->Indices,ip)
      {
        if(cl->Vertices[*ip].Index==-1)
          cl->Vertices[*ip].Index = n++;
      }
      sVERIFY(n==cl->Vertices.GetCount());

      sFORALL(cl->Indices,ip)
        *ip = cl->Vertices[*ip].Index;

      sHeapSortUp(cl->Vertices,&ChaosMeshVertexFat::Index);    // this is inefficient! but who cares.
    }

    // stats

    if(0)
    {
      sDPrintF(L"cluster %3d: %5d verts, %4.2f vpt, %4.2f cpt, %3d matrices, <%s>\n",ci,cl->Vertices.GetCount(),
        sF32(cl->Vertices.GetCount())/(cl->Indices.GetCount()/3),
        sSimulateVertexCache(cl->Indices.GetCount(),&cl->Indices[0],24),
        cl->Matrices.GetCount(),cl->Material ? cl->Material->Name : L"???");
    }

    // flush geometries

    delete cl->Geo;
    delete cl->WireGeo;
  }
}

void ChaosMesh::FlushBuffers()
{
  ChaosMeshCluster *cl;

  sFORALL(Clusters,cl)
  {
    delete cl->Geo;
    delete cl->WireGeo;
  }
}


void ChaosMesh::SetShadowMap(sTexture2D *shadowmap)
{
  ChaosMeshCluster *cl;
  sFORALL(Clusters,cl)
  {
    if(cl->Material && cl->Material->Material && cl->Material->Material->BlendColor==0)
    {
      cl->Material->Material->Texture[6] = shadowmap;
      cl->Material->Material->TFlags[6] = sMTF_LEVEL0 | sMTF_CLAMP;
    }
  }
}

ChaosMtrlVariant::ChaosMtrlVariant()
{
  AndFlags = ~0;
  OrFlags = 0;
  BlendColor = ~0;
  BlendAlpha = ~0;
}

void ChaosMesh::SetMaterialVariants(sInt count,ChaosMtrlVariant *v)
{
  ChaosMeshCluster *cl;
  sVERIFY(count>1);
  sFORALL(Clusters,cl)
  {
    if(cl->Material && cl->Material->Material && cl->Material->Material->GetVariantCount()<=1)
    {
      Wz4Shader *mtrl = cl->Material->Material;
      mtrl->InitVariants(count);
      sU32 flags = mtrl->Flags;
      sU32 color = mtrl->BlendColor;
      sU32 alpha = mtrl->BlendAlpha;

      for(sInt i=0;i<count;i++)
      {
        mtrl->Flags = (flags & v[i].AndFlags) | v[i].OrFlags;
        mtrl->BlendColor = (v[i].BlendColor!=~0U) ? v[i].BlendColor : color;
        mtrl->BlendAlpha = (v[i].BlendAlpha!=~0U) ? v[i].BlendAlpha : alpha;
        mtrl->SetVariant(i);
      }
      mtrl->Flags = flags;
      mtrl->BlendColor = color;
      mtrl->BlendAlpha = alpha;
    }
  }
}
  
void ChaosMesh::Charge()
{
  ChaosMeshCluster *cl;
  UpdateBuffers();
  sFORALL(Clusters,cl)
    cl->Charge();
}

void ChaosMesh::Paint(const sViewport *view,const SceneInstance *inst,sInt variant)
{
  ChaosMeshCluster *cl;
  Charge();
  sCBuffer<Wz4ShaderCamera> cb_cam;
  sCBuffer<Wz4ShaderUV> cb_uv;
  sMatrix34CM skin[80*3];
  sMatrix34 *mat;
  sMatrix34CM *bmat;

  if(Skeleton)
  {
    sInt joints = Skeleton->Joints.GetCount();
    mat = sALLOCSTACK(sMatrix34,joints);
    bmat = sALLOCSTACK(sMatrix34CM,joints);
    Skeleton->EvaluateCM(inst->Time,mat,bmat);
  }
  else
  {
    mat = 0;
    bmat = 0;
  }

  sFORALL(Clusters,cl)
  {
    cl->Charge();
    if(!view || view->Visible(cl->Bounds))
    {
      cl->Material->Material->SetCam(cb_cam.Data,view);
      cl->Material->Material->SetUV(cb_uv.Data,view);
      cl->Material->Material->SetV(&cb_cam,&cb_uv,variant);
      sInt mc = cl->Matrices.GetCount();
      
      if(mc>0)
      {
        for(sInt i=0;i<mc;i++)
          skin[i] = bmat[cl->Matrices[i]];

        sSetVSParam(20,sClamp(mc,0,77)*3,(sVector4 *)skin);
      }
      
      cl->Geo->Draw();
    }
  }
}

void ChaosMesh::Paint(const Wz4ShaderEnv *env,sF32 time,sInt variant)
{
  ChaosMeshCluster *cl;
  Charge();
  sCBuffer<Wz4ShaderCamera> cb_cam;
  sCBuffer<Wz4ShaderUV> cb_uv;
  sMatrix34CM skin[80*3];
  sMatrix34 *mat;
  sMatrix34CM *bmat;

  if(Skeleton)
  {
    sInt joints = Skeleton->Joints.GetCount();
    mat = sALLOCSTACK(sMatrix34,joints);
    bmat = sALLOCSTACK(sMatrix34CM,joints);
    Skeleton->EvaluateCM(time,mat,bmat);
  }
  else
  {
    mat = 0;
    bmat = 0;
  }

  sFORALL(Clusters,cl)
  {
    cl->Charge();
    if(env->Visible(cl->Bounds))
    {
      cl->Material->Material->SetCam(cb_cam.Data,env);
      cl->Material->Material->SetUV(cb_uv.Data,env);
      cl->Material->Material->SetV(&cb_cam,&cb_uv,variant);
      sInt mc = cl->Matrices.GetCount();
      
      if(mc>0)
      {
        for(sInt i=0;i<mc;i++)
          skin[i] = bmat[cl->Matrices[i]];

        sSetVSParam(20,sClamp(mc,0,77)*3,(sVector4 *)skin);
      }
      
      cl->Geo->Draw();
    }
  }
}

void ChaosMesh::PaintMtrl(const Wz4ShaderEnv *env,Wz4Material *mtrl)
{
  ChaosMeshCluster *cl;
  Charge();
  sCBuffer<Wz4ShaderCamera> cb_cam;
  sCBuffer<Wz4ShaderUV> cb_uv;

  sFORALL(Clusters,cl)
  {
    if(cl->Material->Material->BlendColor==0 || env->Visible(cl->Bounds))
    {
      cl->Charge();
      if(1) // missing bbox check!
      {
        mtrl->Material->SetCam(cb_cam.Data,env);
        mtrl->Material->SetUV(cb_uv.Data,env);
        mtrl->Material->Set(&cb_cam,&cb_uv);
        
        cl->Geo->Draw();
      }
    }
  }
}

void ChaosMesh::PaintWire(sGeometry *geo,sF32 time)
{
  sInt ic;
  ChaosMeshFace *face;
  ChaosMeshVertexPosition *pos;
  sU32 *ip;
  sVertexSingle *vp;
  sMatrix34 *mat,*bmat;
  sVector31 v;

  ic = 0;
  sFORALL(Faces,face)
  {
    sVERIFY(face->Count>=3);
    if(face->Cluster!=-1)
      ic += face->Count*2;
  }

  if(Skeleton)
  {
    mat = sALLOCSTACK(sMatrix34,Skeleton->Joints.GetCount());
    bmat = sALLOCSTACK(sMatrix34,Skeleton->Joints.GetCount());
    Skeleton->Evaluate(time,mat,bmat);

    geo->BeginLoadVB(Positions.GetCount(),sGD_STREAM,(void**)&vp);
    sFORALL(Positions,pos)
    {
      if(pos->MatrixIndex[0]!=-1)
      {
        v.Init(0,0,0);
        for(sInt i=0;i<4;i++)
        {
          if(pos->MatrixIndex[i]==-1)
            break;
          v = v + sVector30(pos->Position * bmat[pos->MatrixIndex[i]])*pos->MatrixWeight[i];
        }
      }
      else
      {
        v = pos->Position;
      }
      vp->Init(v,~0,0,0);
      vp++;
    }
    geo->EndLoadVB();
  }
  else
  {
    geo->BeginLoadVB(Positions.GetCount(),sGD_STREAM,(void**)&vp);
    sFORALL(Positions,pos)
    {
      vp->Init(pos->Position,~0,0,0);
      vp++;
    }
    geo->EndLoadVB();
  }

  if(ic)
  {
    geo->BeginLoadIB(ic,sGD_STREAM,(void **)&ip);
    sFORALL(Faces,face)
    {
      if(face->Cluster!=-1)
      {
        *ip++ = face->Positions[face->Count-1];
        for(sInt i=0;i<face->Count-1;i++)
        {
          *ip++ = face->Positions[i];
          *ip++ = face->Positions[i];
        }
        *ip++ = face->Positions[face->Count-1];
      }
    }
    geo->EndLoadIB();
    geo->Draw();
  }
}


void ChaosMesh::PaintSelection(sGeometry *geo,sGeometry *quads,sF32 zoom,sF32 time,const sViewport *View)
{
  sInt pc = Positions.GetCount();
  sInt *map = new sInt[pc];
  sInt *rev = new sInt[pc];
  sInt vc = 0;
  sInt ic = 0;
  sU32 *ip;
  sVertexBasic *vp;
  ChaosMeshFace *face;

  for(sInt i=0;i<pc;i++)
    map[i] = -1;
  sFORALL(Faces,face)
  {
    if(face->Select)
    {
      ic += (face->Count-2)*3;
      for(sInt i=0;i<face->Count;i++)
      {
        sInt v = face->Positions[i];
        if(map[v]==-1)
        {
          rev[vc] = v;
          map[v] = vc++;
        }
      }
    }
  }

  if(vc>0)
  {
    geo->BeginLoadIB(ic,sGD_STREAM,(void **)&ip);
    sFORALL(Faces,face)
    {
      if(face->Select)
      {
        for(sInt i=2;i<face->Count;i++)
        {
          ip[0] = map[face->Positions[0]];
          ip[1] = map[face->Positions[i-1]];
          ip[2] = map[face->Positions[i]];
          ip +=3;
        }
      }
    }
    geo->EndLoadIB();

    geo->BeginLoadVB(vc,sGD_STREAM,(void **)&vp);
    for(sInt i=0;i<vc;i++)
    {
      vp->Init(Positions[rev[i]].Position,~0xffc08080);
      vp++;
    }
    geo->EndLoadVB();

    geo->Draw();
  }

  ic = 0;
  for(sInt i=0;i<pc;i++)
    if(Positions[i].Select)
      map[ic++] = i;
  if(ic>0)
  {
    sMatrix34 mat,inv;
    mat = View->View;
    sVector30 d[4];
    sU32 col = 0xfff8080;
    sF32 s;
    inv = mat;
    inv.Invert34();
    d[0] = (inv.i+inv.j);
    d[1] = (inv.i-inv.j);
    d[2] = (-inv.i-inv.j);
    d[3] = (-inv.i+inv.j);
    quads->BeginLoadVB(ic*4,sGD_STREAM,(void **)&vp);
    for(sInt i=0;i<ic;i++)
    {
      sVector31 v = Positions[map[i]].Position;
      s = zoom*(v*mat).z;
      vp[0].Init(v+d[0]*s,col);
      vp[1].Init(v+d[1]*s,col);
      vp[2].Init(v+d[2]*s,col);
      vp[3].Init(v+d[3]*s,col);
      vp+=4;
    }
    quads->EndLoadVB();
    quads->Draw();
  }

  delete[] map;
  delete[] rev;
}

/*




ChaosMesh::ChaosMesh() : wObject((wType *)ChaosMeshType)
{
  SolidGeo = 0;
}
ChaosMesh::~ChaosMesh()
{
  ChaosMeshCluster *cl;
  sFORALL(Clusters,cl)
    cl->Mtrl->Release();
  delete SolidGeo;
}
void ChaosMesh::Init(sInt vc,sInt ic,sInt cc)
{
  ChaosMeshCluster *cl;
  
  Vertices.Resize(vc);
  Indices.Resize(ic);
  Clusters.Resize(cc);
  
  sFORALL(Clusters,cl)
  {
    cl->StartIndex = 0;
    cl->EndIndex = ic;
    cl->Mtrl = 0;
  }
}
void ChaosMesh::CacheSolid()
{
  sVertexStandard *vd;
  ChaosMeshVertex *vs;
  sU32 *id;
  if(!SolidGeo)
  {
    SolidGeo = new sGeometry;
    SolidGeo->Init(sGF_TRILIST|sGF_INDEX32,sVertexFormatStandard);
    SolidGeo->BeginLoadVB(Vertices.GetCount(),sGD_STATIC,(void **)&vd);
    sFORALL(Vertices,vs)
    {
      vd->Init(vs->Pos,vs->Normal,vs->u,vs->v);
      vd++;
    }
    SolidGeo->EndLoadVB();
    SolidGeo->BeginLoadIB(Indices.GetCount(),sGD_STATIC,(void **)&id);
    sCopyMem(id,&Indices[0],sizeof(sU32)*Indices.GetCount());
    SolidGeo->EndLoadIB();
  }
}
void ChaosMesh::Copy(ChaosMesh *src)
{
  Vertices = src->Vertices;
  Indices = src->Indices;
  Clusters = src->Clusters;
}

*/
/****************************************************************************/

sBool ChaosMesh::MergeShape(ChaosMesh *ref,ChaosMesh *shape,sF32 phase,ChaosMesh *weight)
{
  ChaosMeshVertexPosition *pos_out,*pos_ref,*pos_weight;
//  ChaosMeshVertexNormal *norm_out,*norm_ref;
//  ChaosMeshFace *face;
  sMatrix34 *pmat=0,*bmat=0;
  sMatrix34 *thismat;
  sMatrix34 mat;
  sVector31 v;
  sInt *weightindex = 0;

  // sanity checks

  if(ref->Positions.GetCount()!=shape->Positions.GetCount() ||
     ref->Normals.GetCount()!=shape->Normals.GetCount() ||
     ref->Tangents.GetCount()!=shape->Tangents.GetCount())
  {
    return 0;
  }

  // allocate 

  if(Skeleton)
  {
    pmat = sALLOCSTACK(sMatrix34,Skeleton->Joints.GetCount());
    bmat = sALLOCSTACK(sMatrix34,Skeleton->Joints.GetCount());
    Skeleton->Evaluate(phase,pmat,bmat);
  }

  sInt *PosTemp = new sInt[Positions.GetCount()];

  // link joints

  if(weight)
  {
    weightindex = new sInt[weight->Skeleton->Joints.GetCount()];
    for(sInt i=0;i<weight->Skeleton->Joints.GetCount();i++)
    {
      weightindex[i] = Skeleton->FindJoint(weight->Skeleton->Joints[i].Name);
      if(weightindex[i]==-1)
        return 0;
    }
  }


/*
  sInt *NormOut = new sInt[Normals.GetCount()];
  sInt *TangOut = new sInt[Tangents.GetCount()];
  sInt *NormRef = new sInt[ref->Normals.GetCount()];
  sInt *TangRef = new sInt[ref->Tangents.GetCount()];

  sSetMem(NormOut,-1,sizeof(sInt)*Normals.GetCount());
  sSetMem(TangOut,-1,sizeof(sInt)*Tangents.GetCount());
  sSetMem(NormRef,-1,sizeof(sInt)*ref->Normals.GetCount());
  sSetMem(TangRef,-1,sizeof(sInt)*ref->Tangents.GetCount());

  sInt doublen0,doublet0,doublen1,doublet1;
  doublen0 = doublet0 = doublen1 = doublet1 = 0;

  sFORALL(Faces,face)
  {
    for(sInt i=0;i<face->Count;i++)
    {
      sInt ni = face->Normals[i];
      if(NormOut[ni]==-1)
        NormOut[ni] = face->Positions[i];
      else if(NormOut[ni]!=face->Positions[i])
        doublen0++;

      sInt ti = face->Tangents[i];
      if(TangOut[ti]==-1)
        TangOut[ti] = face->Positions[i];
      else if(TangOut[ti]!=face->Positions[i])
        doublet0++;
    }
  }
  sFORALL(ref->Faces,face)
  {
    for(sInt i=0;i<face->Count;i++)
    {
      sInt ni = face->Normals[i];
      if(NormRef[ni]==-1)
        NormRef[ni] = face->Positions[i];
      else if(NormRef[ni]!=face->Positions[i])
        doublen1++;

      sInt ti = face->Tangents[i];
      if(TangRef[ti]==-1)
        TangRef[ti] = face->Positions[i];
      else if(TangRef[ti]!=face->Positions[i])
        doublet1++;
    }
  }
  if(doublen0 || doublet0 || doublen1 || doublet1)
    sDPrintF(L"normals or tangents are kaputt (out norm %d tang %d ; ref norm %d tang %d)\n",doublen0,doublet0,doublen1,doublet1);
*/

  sFORALL(Positions,pos_out)
  {
    pos_out->Select = 0;
    sInt pi = _i;
    PosTemp[pi] = -1;

    mat.Init();
    if(bmat)
    {
      mat.i.Init(0,0,0);
      mat.j.Init(0,0,0);
      mat.k.Init(0,0,0);
      mat.l.Init(0,0,0);
      for(sInt i=0;i<4 && pos_out->MatrixIndex[i]>=0;i++)
      {
        thismat = bmat + pos_out->MatrixIndex[i];
        sF32 f = pos_out->MatrixWeight[i];
        
        mat.i = mat.i + thismat->i * f;
        mat.j = mat.j + thismat->j * f;
        mat.k = mat.k + thismat->k * f;
        mat.l = mat.l + sVector30(thismat->l) * f;
      }
    }

    v = pos_out->Position * mat;
    sInt found = -1;
    sFORALL(ref->Positions,pos_ref)
    {
      sVector30 dist = pos_ref->Position - v;
      if((dist^dist) < 0.00001f*0.00001f)
      {
        found = _i;
        break;
      }
    }
    if(found==-1)
    {
      sFORALL(ref->Positions,pos_ref)
      {
        sVector30 dist = pos_ref->Position - v;
        if((dist^dist) < 0.0001f*0.0001f)
        {
          found = _i;
          break;
        }
      }
    }

    if(found>=0)
    {
      pos_ref = &ref->Positions[found];
      if(weight)
      {
        sFORALL(weight->Positions,pos_weight)
        {
          sVector30 dist = pos_out->Position - pos_weight->Position;
          if((dist^dist) < 0.00001f*0.00001f)
          {
            for(sInt i=0;i<4;i++)
            {
              sInt index = pos_weight->MatrixIndex[i];
              if(index==-1)
              {
                pos_out->MatrixIndex[i]  = -1;
                pos_out->MatrixWeight[i] = 0;
              }
              else
              {
                pos_out->MatrixIndex[i]  = weightindex[index];
                pos_out->MatrixWeight[i] = pos_weight->MatrixWeight[i];
              }
            }
            break;
          }
        }
      }

      pos_out->Position = shape->Positions[found].Position;
      PosTemp[pi] = found;
      pos_out->Select = 1;
    }
  }
/*
  sInt badn,badt;
  badn = badt = 0;
  sInt good = 0;
  sFORALL(Normals,norm_out)
  {
    sInt ni = _i;
    if(NormOut[ni]>=0 && PosTemp[NormOut[ni]]>=0) // used normal, and normal for a morphed position
    {
      sInt np = PosTemp[NormOut[ni]];
      sInt found = 0;
      sFORALL(ref->Normals,norm_ref)
      {
        if(NormRef[_i]==np)
        {
          sVector30 dist = norm_ref->Normal - norm_out->Normal;
          if((dist^dist) < 0.001f*0.001f)
          {
            found = 1;
            norm_out->Normal = shape->Normals[_i].Normal;
            break;
          }
          else
          {
            found = 0;
          }
        }
      }
      if(!found) 
        badn++;
      else
        good++;
    }
  }
  sDPrintF(L"%d/%d normals and %d tangents missing in shape\n",badn,badn+good,badt);
*/

  sFORALL(Positions,pos_out)
  {
    ChaosMeshFace *face;
    sInt pi = _i;

    // find normal from shape 
    // assuming there are no cracks

    sVector30 norm;

    if(pos_out->Select)
    {
      mat.Init();
      if(bmat)
      {
        mat.i.Init(0,0,0);
        mat.j.Init(0,0,0);
        mat.k.Init(0,0,0);
        mat.l.Init(0,0,0);
        for(sInt i=0;i<4 && pos_out->MatrixIndex[i]>=0;i++)
        {
          thismat = bmat + pos_out->MatrixIndex[i];
          sF32 f = pos_out->MatrixWeight[i];
          
          mat.i = mat.i + thismat->i * f;
          mat.j = mat.j + thismat->j * f;
          mat.k = mat.k + thismat->k * f;
          mat.l = mat.l + sVector30(thismat->l) * f;
        }
        mat.InvertOrthogonal();
        pos_out->Position = pos_out->Position * mat;
      }
      
      sFORALL(shape->Faces,face)
      {
        for(sInt i=0;i<face->Count;i++)
        {
          if(face->Positions[i] == PosTemp[pi])
          {
            norm = shape->Normals[face->Normals[i]].Normal * mat;
            norm.Unit();

            // now apply this normal to all occurences...

            sFORALL(Faces,face)
            {
              for(sInt i=0;i<face->Count;i++)
                if(face->Positions[i]==pi)
                  Normals[face->Normals[i]].Normal = norm;
            }

            goto foundnorm;
          }
        }
      }
    }
foundnorm:;
  }

  //Triangulate();
  //CalcNormals();

  // adjust to bone deformations. this should be done after CalcNormas
/*
  if(bmat)
  {
    sFORALL(Positions,pos_out)
    {
      if(pos_out->Select)
      {
      }
    }
  }
*/
  delete[] PosTemp;
  delete[] weightindex;
/*
  delete[] NormOut;
  delete[] TangOut;
  delete[] NormRef;
  delete[] TangRef;
  */
  return 1;
}

/****************************************************************************/


const sChar *XsiHeader = 
L"xsi 0600txt 0032\n"
L"SI_CoordinateSystem { 1,0,1,0,2,5, }\n"
L"SI_Angle { 0, }\n"
L"SI_MaterialLibrary DefaultLib { \n"
L"2, \n"
L"XSI_Material Scene_Material { \n"
L"  3, \n"
L"  \"surface\",\"Phong\",\n"
L"  \"shadow\",\"Phong\",\n"
L"  \"Photon\",\"Phong\",\n"
L"  XSI_MaterialInfo { 0,0,}\n"
L"\n\n"
L"  XSI_Shader Phong { \n"
L"    \"Softimage.material-phong.1\", \n"
L"    4,  \n"
L"    0,  \n"
L"    0,  \n"
L"  } \n"
L"} \n"

L"  XSI_Material Material { \n"
L"  3, \n"
L"  \"surface\",\"Phong\",\n"
L"  \"shadow\",\"Phong\",\n"
L"  \"Photon\",\"Phong\",\n"
L"  XSI_MaterialInfo { \n"
L"    0, \n"
L"    0, \n"
L"  }\n"
L" \n"
L"  XSI_Shader Phong {  \n"
L"    \"Softimage.material-phong.1\",  \n"
L"    4,  \n"
L"    0,  \n"
L"    0,  \n"
L"  } \n"
L"} \n"
L"} \n"
L"XSI_ImageLibrary {  \n"
L"  1,  \n"
L"  XSI_Image noIcon_pic {  \n"
L"    \"$XSI_HOME/Application/rsrc/noIcon.pic\",  \n"
L"    256,  \n"
L"    256,  \n"
L"    4,  \n"
L"    0,  \n"
L"    0.000000,  \n"
L"    1.000000,  \n"
L"    0.000000,  \n"
L"    1.000000,  \n"
L"    0.000000,  \n"
L"    0,  \n"
L"    0,  \n"
L"    0,  \n"
L"  } \n"
L"} \n" ;



void ChaosMesh::ExportXSI(const sChar *filename)
{
  sTextBuffer tb;
  ChaosMeshVertexPosition *pos;
  ChaosMeshVertexNormal *norm;
  ChaosMeshVertexTangent *tang;
  ChaosMeshVertexProperty *prop;
  ChaosMeshFace *face;

  sInt ic;
  ic = 0;
  sFORALL(Faces,face)
    ic += face->Count;

  tb.Print(XsiHeader);
  tb.Print(L"SI_Model MDL-all {\n");
  tb.Print(L"  SI_Visibility { 1, }\n");
  tb.Print(L"  XSI_Transform { \n");
  tb.Print(L"    0.0,0.0,0.0, \n");
  tb.Print(L"    0.0,0.0,0.0, \n");
  tb.Print(L"    \"XYZ\", \n");
  tb.Print(L"    1.0,1.0,1.0, \n");
  tb.Print(L"    1, \n");
  tb.Print(L"    0.0,0.0,0.0, 0.0,0.0,0.0, 0.0,0.0,0.0, 1.0,1.0,1.0, \n");
  tb.Print(L"    0.0,0.0,0.0, 0.0,0.0,0.0, 1.0,1.0,1.0, 1.0,1.0,1.0, 0.0,0.0,0.0, 0.0,0.0,0.0, 0.0,0.0,0.0, \n");
  tb.Print(L"    XSI_Limit { \"posx\",0,0.0,0,0.0, } \n");
  tb.Print(L"    XSI_Limit { \"posy\",0,0.0,0,0.0, } \n");
  tb.Print(L"    XSI_Limit { \"posz\",0,0.0,0,0.0, } \n");
  tb.Print(L"    XSI_Limit { \"rotx\",0,0.0,0,0.0, } \n");
  tb.Print(L"    XSI_Limit { \"roty\",0,0.0,0,0.0, } \n");
  tb.Print(L"    XSI_Limit { \"rotz\",0,0.0,0,0.0, } \n");
  tb.Print(L"  } \n");
  tb.Print(L"  XSI_Mesh MSH-all {\n");
  tb.Print(L"    XSI_Shape SHP-cube {\n");
  tb.Print(L"      \"ORDERED\",\n");
  tb.Print(L"      XSI_SubComponentAttributeList position { \n");
  tb.Print(L"        \"POSITION\",\n");
  tb.Print(L"        \"FLOAT3\",\n");
  tb.PrintF(L"        %d,\n",Positions.GetCount());
  sFORALL(Positions,pos)
    tb.PrintF(L"        %f,%f,%f,\n",pos->Position.x,pos->Position.y,pos->Position.z);
  tb.Print(L"      } \n");
  tb.Print(L"      XSI_SubComponentAttributeList XSINormal { \n");
  tb.Print(L"        \"NORMAL\",\n");
  tb.Print(L"        \"FLOAT3\",\n");
  tb.PrintF(L"        %d,\n",Normals.GetCount());
  sFORALL(Normals,norm)
    tb.PrintF(L"        %f,%f,%f,\n",norm->Normal.x,norm->Normal.y,norm->Normal.z);
  tb.Print(L"      } \n");
  tb.Print(L"      XSI_SubComponentAttributeList Texture_Projection { \n");
  tb.Print(L"        \"TEXCOORD\",\n");
  tb.Print(L"        \"FLOAT2\",\n");
  tb.PrintF(L"        %d,\n",Properties.GetCount());
  sFORALL(Properties,prop)
    tb.PrintF(L"        %f,%f,\n",prop->U[0],1-prop->V[0]);
  tb.Print(L"      } \n");
  tb.Print(L"      XSI_SubComponentAttributeList Tangents { \n");
  tb.Print(L"        \"TEXTANGENT\",\n");
  tb.Print(L"        \"FLOAT4\",\n");
  tb.PrintF(L"        %d,\n",Normals.GetCount());
  sFORALL(Tangents,tang)
    tb.PrintF(L"        %f,%f,%f,%f,\n",tang->Tangent.x,tang->Tangent.y,tang->Tangent.z,tang->BiSign);
  tb.Print(L"      } \n");
  tb.Print(L"    } \n");
  tb.Print(L"    XSI_VertexList { \n");
  tb.Print(L"      1, \n");
  tb.Print(L"      \"position\", \n");
  tb.PrintF(L"      %d, \n",Positions.GetCount());
  for(sInt i=0;i<Positions.GetCount();i++)
    tb.PrintF(L"      %d, \n",i);
  tb.Print(L"    } \n");
  tb.Print(L"    XSI_PolygonList PolyList00 { \n");
  tb.Print(L"      3, \n");
  tb.Print(L"      \"XSINormal\",\"Texture_Projection\",\"Tangents\", \n");
  tb.Print(L"      \"Material\", \n");
  tb.PrintF(L"      %d, \n",ic);
  tb.PrintF(L"      %d, \n",Faces.GetCount());
  sFORALL(Faces,face)
  {
    tb.PrintF(L"      %d, \n",face->Count);
    for(sInt i=0;i<face->Count;i++)
      tb.PrintF(L"      %d,%d,%d,%d,\n",face->Positions[i],face->Normals[i],face->Property[i],face->Tangents[i]);
  }
  tb.Print(L"    } \n");
  tb.Print(L"  } \n");
	tb.Print(L"  SI_GlobalMaterial { \"Material\", \"NODE\", }\n");
  tb.Print(L"} \n");


  sSaveTextAnsi(filename,tb.Get());
}

/****************************************************************************/
/***                                                                      ***/
/***   This uses Tom Forsyth linear time method                           ***/
/***   a.k.a. "don't worry, be happy" stripper                            ***/
/***                                                                      ***/
/***   Implemented by Fabian Giesen.                                      ***/
/***   The above copyright does not apply to this function.               ***/
/***                                                                      ***/
/*** http://home.comcast.net/~tom_forsyth/papers/fast_vert_cache_opt.html ***/
/***                                                                      ***/
/**************************************************************************+*/


struct VCacheVert
{
  sInt CachePos;      // its position in the cache (-1 if not in)
  sInt Score;         // its score (higher=better)
  sInt TrisLeft;      // # of not-yet-used tris
  sInt *TriList;      // list of triangle indices
  sInt OpenPos;       // position in "open vertex" list
};

struct VCacheTri
{
  sInt Score;         // current score (-1 if already done)
  sInt Inds[3];     // vertex indices
};

void sOptimizeIndexOrder(sInt IndexCount,sInt VertexCount,sInt *IndexBuffer)
{
  // alloc+initialize vertices
  VCacheVert *verts = new VCacheVert[VertexCount];
  for(sInt i=0;i<VertexCount;i++)
  {
    verts[i].CachePos = -1;
    verts[i].Score = 0;
    verts[i].TrisLeft = 0;
    verts[i].TriList = 0;
    verts[i].OpenPos = -1;
  }

  // prepare triangles
  sInt nTris = IndexCount/3;
  VCacheTri *tris = new VCacheTri[nTris];
  sInt *indPtr = IndexBuffer;

  for(sInt i=0;i<nTris;i++)
  {
    tris[i].Score = 0;

    for(sInt j=0;j<3;j++)
    {
      sInt ind = *indPtr++;
      tris[i].Inds[j] = ind;
      verts[ind].TrisLeft++;
    }
  }

  // alloc space for vert->tri indices
  sInt *vertTriInd = new sInt[nTris*3];
  sInt *vertTriPtr = vertTriInd;

  for(sInt i=0;i<VertexCount;i++)
  {
    verts[i].TriList = vertTriPtr;
    vertTriPtr += verts[i].TrisLeft;
    verts[i].TrisLeft = 0;
  }

  // make vert->tri tables
  for(sInt i=0;i<nTris;i++)
  {
    for(sInt j=0;j<3;j++)
    {
      sInt ind = tris[i].Inds[j];
      verts[ind].TriList[verts[ind].TrisLeft] = i;
      verts[ind].TrisLeft++;
    }
  }

  // open vertices
  sInt *openVerts = new sInt[VertexCount];
  sInt openCount = 0;

  // the cache
  static const sInt cacheSize = 32;
  static const sInt maxValence = 15;
  sInt cache[cacheSize+3];
  sInt pos2Score[cacheSize];
  sInt val2Score[maxValence+1];

  for(sInt i=0;i<cacheSize+3;i++)
    cache[i] = -1;

  for(sInt i=0;i<cacheSize;i++)
  {
    sF32 score = (i<3) ? 0.75f : sFPow(1.0f - (i-3)/(cacheSize-3),1.5f);
    pos2Score[i] = sInt(score * 65536.0f + 0.5f);
  }

  val2Score[0] = 0;
  for(sInt i=1;i<16;i++)
  {
    sF32 score = 2.0f * sFInvSqrt(i);
    val2Score[i] = sInt(score * 65536.0f + 0.5f);
  }

  // outer loop: find triangle to start with
  indPtr = IndexBuffer;
  sInt seedPos = 0;

  while(1)
  {
    sInt seedScore = -1;
    sInt seedTri = -1;

    // if there are open vertices, search them for the seed triangle
    // which maximum score.
    for(sInt i=0;i<openCount;i++)
    {
      VCacheVert *vert = &verts[openVerts[i]];

      for(sInt j=0;j<vert->TrisLeft;j++)
      {
        sInt triInd = vert->TriList[j];
        VCacheTri *tri = &tris[triInd];

        if(tri->Score > seedScore)
        {
          seedScore = tri->Score;
          seedTri = triInd;
        }
      }
    }

    // if we haven't found a seed triangle yet, there are no open
    // vertices and we can pick any triangle
    if(seedTri == -1)
    {
      while(seedPos < nTris && tris[seedPos].Score<0)
        seedPos++;

      if(seedPos == nTris) // no triangle left, we're done!
        break;

      seedTri = seedPos;
    }

    // the main loop.
    sInt bestTriInd = seedTri;
    while(bestTriInd != -1)
    {
      VCacheTri *bestTri = &tris[bestTriInd];

      // mark this triangle as used, remove it from the "remaining tris"
      // list of the vertices it uses, and add it to the index buffer.
      bestTri->Score = -1;

      for(sInt j=0;j<3;j++)
      {
        sInt vertInd = bestTri->Inds[j];
        *indPtr++ = vertInd;

        VCacheVert *vert = &verts[vertInd];
        
        // find this triangles' entry
        sInt k = 0;
        while(vert->TriList[k] != bestTriInd)
        {
          sVERIFY(k < vert->TrisLeft);
          k++;
        }

        // swap it to the end and decrement # of tris left
        if(--vert->TrisLeft)
          sSwap(vert->TriList[k],vert->TriList[vert->TrisLeft]);
        else if(vert->OpenPos >= 0)
          sSwap(openVerts[vert->OpenPos],openVerts[--openCount]);
      }

      // update cache status
      cache[cacheSize] = cache[cacheSize+1] = cache[cacheSize+2] = -1;

      for(sInt j=0;j<3;j++)
      {
        sInt ind = bestTri->Inds[j];
        cache[cacheSize+2] = ind;

        // find vertex index
        sInt pos;
        for(pos=0;cache[pos]!=ind;pos++);

        // move to front
        for(sInt k=pos;k>0;k--)
          cache[k] = cache[k-1];

        cache[0] = ind;

        // remove sentinel if it wasn't used
        if(pos!=cacheSize+2)
          cache[cacheSize+2] = -1;
      }

      // update vertex scores
      for(sInt i=0;i<cacheSize+3;i++)
      {
        sInt vertInd = cache[i];
        if(vertInd == -1)
          continue;

        VCacheVert *vert = &verts[vertInd];

        vert->Score = val2Score[sMin(vert->TrisLeft,maxValence)];
        if(i < cacheSize)
        {
          vert->CachePos = i;
          vert->Score += pos2Score[i];
        }
        else
          vert->CachePos = -1;

        // also add to open vertices list if the vertex is indeed open
        if(vert->OpenPos<0 && vert->TrisLeft)
        {
          vert->OpenPos = openCount;
          openVerts[openCount++] = vertInd;
        }
      }

      // update triangle scores, find new best triangle
      sInt bestTriScore = -1;
      bestTriInd = -1;

      for(sInt i=0;i<cacheSize;i++)
      {
        if(cache[i] == -1)
          continue;

        const VCacheVert *vert = &verts[cache[i]];

        for(sInt j=0;j<vert->TrisLeft;j++)
        {
          sInt triInd = vert->TriList[j];
          VCacheTri *tri = &tris[triInd];

          sVERIFY(tri->Score != -1);

          sInt score = 0;
          for(sInt k=0;k<3;k++)
            score += verts[tri->Inds[k]].Score;

          tri->Score = score;
          if(score > bestTriScore)
          {
            bestTriScore = score;
            bestTriInd = triInd;
          }
        }
      }
    }
  }

  // cleanup
  delete[] verts;
  delete[] tris;
  delete[] vertTriInd;
  delete[] openVerts;
}

/****************************************************************************/

sF32 sSimulateVertexCache(sInt count,sInt *indices,sInt cachesize)
{
  sVERIFY(cachesize<=64);
  sInt cache[64];
  sInt cp;
  sInt cachemisses = 0;
  for(sInt i=0;i<cachesize;i++)
    cache[i] = -1;

  cp = 0;
  for(sInt i=0;i<count;i++)
  {
    sInt index = indices[i];
    for(sInt i=0;i<cachesize;i++)
      if(cache[i] == index)
        goto ok;

    cachemisses++;
    cache[cp++] = index;
    if(cp==cachesize) cp = 0;
ok:;
  }

  return sF32(cachemisses)/(count/3);
}

/****************************************************************************/