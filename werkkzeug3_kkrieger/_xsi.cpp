// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_xsi.hpp"
#include "_util.hpp"

#define DUMPCHUNK 0
#define DUMPMODEL 0

/****************************************************************************/
/***                                                                      ***/
/***   XSI Model Classes                                                  ***/
/***                                                                      ***/
/****************************************************************************/

void sXSIVertex::Init()
{
  sSetMem(this,0,sizeof(sXSIVertex));
  //Color = 0xffffffff;
  Normal.y = 1.0f;
}

/****************************************************************************/

void sXSIVertex::AddWeight(sXSIModel *m,sF32 w)
{
  sInt i,pos;

  if(w>1.0f && WeightCount<sXSI_MAXWEIGHT)
  {
    pos = WeightCount;
    for(i=0;i<WeightCount;i++)
    {
      if(Weight[i]<w)
      {
        pos = i;
        break;
      }
    }
    for(i=WeightCount;i>pos;i--)
    {
      Weight[i] = Weight[i-1];
      WeightModel[i] = WeightModel[i-1];
    }

    Weight[pos] = w;
    WeightModel[pos] = m;
    WeightCount++;
  }
}

/****************************************************************************/
/****************************************************************************/

sXSITexture::sXSITexture()
{
  Name[0] = 0;
  PathName[0] = 0;
  Bitmap = 0;
}

sXSITexture::~sXSITexture()
{
}

void sXSITexture::Tag()
{
  sBroker->Need(Bitmap);
}

/****************************************************************************/
/****************************************************************************/

sXSIMaterial::sXSIMaterial()
{
  sInt i;

  Name[0] = 0;
  Flags = 0;
  Mode = 0;
  for(i=0;i<4;i++)
  {
    TFlags[i] = 0;
    Tex[i] = 0;
    TUV[i] = 0;
  }
}

sXSIMaterial::~sXSIMaterial()
{
}

void sXSIMaterial::Tag()
{
  sInt i;
  for(i=0;i<4;i++)
    sBroker->Need(Tex[i]);
}

/****************************************************************************/
/****************************************************************************/

sXSICluster::sXSICluster()
{
  VertexCount = 0;
  Vertices = 0;
  Material = 0;
  Faces = 0;
}

/****************************************************************************/

sXSICluster::~sXSICluster()
{
  Clear();
}

/****************************************************************************/

void sXSICluster::Tag()
{
  sBroker->Need(Material);
}

/****************************************************************************/

void sXSICluster::Clear()
{
  if(Vertices)
    delete[] Vertices;
  Vertices = 0;
  VertexCount = 0;

  if(Faces)
    delete[] Faces;
  Faces = 0;
  
  Material = 0;
}

/****************************************************************************/

static bool XSIVertexLess(const sXSIVertex &a,const sXSIVertex &b)
{
  return a.Index < b.Index;
}

void sXSICluster::Optimise()
{
  sInt i,j;
  sInt *nface;
  sInt *mface;
  sInt ncount;
  sXSIVertex *v0,*v1;
  sInt hashbucket[1024];

// init

  ncount = 0;
  nface = new sInt[VertexCount];  // list of new faces
  mface = new sInt[VertexCount];  // old -> new mapping

// find unique
  v0 = Vertices;
  for(i=0;i<1024;i++)
    hashbucket[i] = -1;

  for(i=0;i<VertexCount;i++)
  {
    //for(j=0;j<ncount;j++)
    for(j=hashbucket[v0->Index & 1023];j!=-1;j=v1->Temp)
    {
      v1 = &Vertices[nface[j]];
      if(v0->Index==v1->Index &&
         v0->Color==v1->Color &&
         v0->UV[0][0]==v1->UV[0][0] && v0->UV[0][1]==v1->UV[0][1] &&
         v0->UV[1][0]==v1->UV[1][0] && v0->UV[1][1]==v1->UV[1][1] &&
         v0->Normal.x==v1->Normal.x && v0->Normal.y==v1->Normal.y && v0->Normal.z==v1->Normal.z)
      {
        mface[i] = j;
        goto found;
      }
    }
    mface[i] = ncount;
    nface[ncount] = i;
    v0->Temp = hashbucket[v0->Index & 1023];
    hashbucket[v0->Index & 1023] = ncount;
    ncount++;
found:;
    v0++;
  }

// rearrange

  for(i=0;i<ncount;i++)
    sSwap(Vertices[i],Vertices[nface[i]]);
  VertexCount = ncount;
  for(i=0;i<IndexCount;i++)
    Faces[i*2+1] = mface[Faces[i*2+1]];

// cleanup

  delete[] nface;
  delete[] mface;
}

/****************************************************************************/
/****************************************************************************/

void sXSIFCurveKey::Init()
{
  Num = 0;
  Pos = 0;
  Spline[0] = 0;
  Spline[1] = 0;
  Spline[2] = 0;
  Spline[3] = 0;
}

/****************************************************************************/

sXSIFCurve::sXSIFCurve()
{

  KeyCount = 0;
  Keys = 0;
  Offset = 0;
}

/****************************************************************************/

sXSIFCurve::~sXSIFCurve()
{
  Clear();
}

/****************************************************************************/

void sXSIFCurve::Clear()
{
  if(Keys)
    delete[] Keys;
  Keys = 0;
  KeyCount = 0;
  Offset = 0;
}

/****************************************************************************/
/****************************************************************************/

sXSIModel::sXSIModel()
{
  Childs = new sList<sXSIModel>;
  FCurves = new sList<sXSIFCurve>;
  Clusters = new sList<sXSICluster>;
  BaseS.Init(1,1,1);
  BaseR.Init();
  BaseT.Init();
  TransS.Init(1,1,1);
  TransR.Init();
  TransT.Init();
  Visible = sTRUE;
  Index = 0;
  Name[0] = 0;
}

/****************************************************************************/

sXSIModel::~sXSIModel()
{
}

/****************************************************************************/

void sXSIModel::Clear()
{
  Childs->Clear();
  FCurves->Clear();
  Clusters->Clear();
}

/****************************************************************************/

void sXSIModel::Tag()
{
  sBroker->Need(Childs);
  sBroker->Need(FCurves);
  sBroker->Need(Clusters);
}

/****************************************************************************/

sXSIModel *sXSIModel::FindModel(sChar *name)
{
  sInt i;
  sXSIModel *mod;

  mod = 0;
  if(sCmpString(name,Name)==0)
  {
    mod = this;
  }
  else
  {
    for(i=0;i<Childs->GetCount() && mod==0;i++)
      mod = Childs->Get(i)->FindModel(name);
  }

  return mod;
}

/****************************************************************************/

void sXSIModel::DumpModel(sInt indent)
{
  sInt i;

  for(i=0;i<indent;i++)
    sDPrintF("  ");
  sDPrintF("%s ",Name);
  if(FCurves->GetCount()>0)
    sDPrintF("(%d fc) ",FCurves->GetCount());
  if(Clusters->GetCount())
    sDPrintF("(%d cl) ",Clusters->GetCount());
  if(!Visible)
    sDPrintF("(off) ");
  sDPrintF("\n"); 

  for(i=0;i<Childs->GetCount();i++)
    Childs->Get(i)->DumpModel(indent+1);
}

/****************************************************************************/
/***                                                                      ***/
/***   Basic Scanning                                                     ***/
/***                                                                      ***/
/****************************************************************************/

sInt sXSILoader::ScanInt()
{
  sInt i;
  sScanSpace(Scan);
  i = sScanInt(Scan);
  sScanSpace(Scan);
  if(*Scan==',')
    Scan++;
  else
    Error = sTRUE;
  return i;
}

/****************************************************************************/

sF32 sXSILoader::ScanFloat()
{
  sF32 f;
  sScanSpace(Scan);
  f = sScanFloat(Scan);
  sScanSpace(Scan);
  if(*Scan==',')
    Scan++;
  else
    Error = sTRUE;
  return f;
}

/****************************************************************************/

void sXSILoader::ScanString(sChar *buffer,sInt size)
{
  sScanSpace(Scan);
  sScanString(Scan,buffer,size);
  sScanSpace(Scan);
  if(*Scan==',')
    Scan++;
  else
    Error = sTRUE;
}

/****************************************************************************/
/***                                                                      ***/
/***   Generic Scanning                                                   ***/
/***                                                                      ***/
/****************************************************************************/

void sXSILoader::ScanName(sChar *buffer,sInt size)
{
  sBool done;

  done = sFALSE;

  if(!((*Scan>='a' && *Scan<='z') || 
       (*Scan>='A' && *Scan<='Z') || *Scan=='_' )) 
  {
    Error = sTRUE;
    return;
  }

  *buffer++ = *Scan++;
  size--;

  while(size>1)
  {
    if(!((*Scan>='a' && *Scan<='z') || 
         (*Scan>='A' && *Scan<='Z') || 
         (*Scan>='0' && *Scan<='9') || *Scan=='_' || *Scan=='-' || *Scan=='.' || *Scan==':' )) 
    {
      *buffer++=0;
      return;
    }
    *buffer++ = *Scan++;
    size--;
  }

  *buffer++=0;
  Error = sTRUE;
}

/****************************************************************************/

void sXSILoader::SkipChunk()
{
  sInt i;

  i=0;
  while(*Scan && i>=0)
  {
    if(*Scan=='"')
    {
      Scan++;
      while(*Scan!='"' && *Scan!=0 && *Scan!='\r' && *Scan!='\n')
        Scan++;
      if(*Scan=='"')
        Scan++;
    }
    if(*Scan=='{')
      i++;
    if(*Scan=='}')
      i--;
    *Scan++;
  }
#if DUMPCHUNK
  sDPrintF("  (skipped)");
#endif
  if(i>=0)
    Error = sTRUE;
}

/****************************************************************************/

void sXSILoader::ScanChunk(sInt indent,sChar *chunk,sChar *name)
{
#if DUMPCHUNK
  sInt i;
#endif

  sScanSpace(Scan);
  Error |= !sScanName(Scan,chunk,XSISIZE);
  sScanSpace(Scan);
  if(*Scan=='{')
    name[0]=0;
  else
    ScanName(name,XSISIZE);

  if(!Error)
  {
#if DUMPCHUNK
    sDPrintF("\n");
    for(i=0;i<indent;i++)
      sDPrintF("  ");
    sDPrintF("%s \"%s\"",chunk,name);
#endif
  }

  sScanSpace(Scan);
  if(*Scan!='{')
    Error = sTRUE;
  else
    Scan++;
}

/****************************************************************************/

void sXSILoader::EndChunk()
{
  sScanSpace(Scan);
  if(*Scan=='}')
  {
    Scan++;
    sScanSpace(Scan);
  }
  else
  {
    Error = sTRUE;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   XSI Parsing                                                        ***/
/***                                                                      ***/
/****************************************************************************/

void sXSILoader::ScanMatLib(sInt indent)
{
  sChar chunk[XSISIZE];
  sChar name[XSISIZE];
  sChar texname[256];
  sChar buffer[XSISIZE];
  sChar pname[XSISIZE];
  sChar tname[XSISIZE];
  sChar tsup[2][XSISIZE];
  sInt texena[2];

  sXSIMaterial *mat;
  sXSITexture *tex;
  sInt i,max;
  sInt pcount,ccount;
  sInt ival;
  sF32 fval;

  ScanInt();
  while(*Scan!=0 && *Scan!='}' && !Error)
  {
    ScanChunk(indent,chunk,name);

    if(sCmpString(chunk,"SI_Material")==0)
    {
      mat = new sXSIMaterial;
      sCopyString(mat->Name,name,sizeof(mat->Name));
      mat->Flags |= 0;
      mat->Ambient = mat->Diffuse = mat->Specular = 0;
      Materials->Add(mat);

      ScanFloat(); ScanFloat(); ScanFloat(); ScanFloat();
      ScanFloat(); ScanFloat(); ScanFloat(); ScanFloat();
      ScanFloat(); ScanFloat(); ScanFloat(); 
      ScanInt();
      ScanFloat(); ScanFloat(); ScanFloat();

      sScanSpace(Scan);
      while(*Scan!=0 && *Scan!='}' && !Error)
      {
        ScanChunk(indent+1,chunk,name);
        if(sCmpString(chunk,"SI_Texture2D")==0)
        {
          ScanString(texname,sizeof(texname));
          ScanInt(); ScanInt(); ScanInt(); ScanInt();
          ScanInt(); ScanInt(); ScanInt(); ScanInt();
          ScanInt(); ScanInt(); ScanInt(); ScanInt();
          ScanFloat(); ScanFloat(); ScanFloat(); ScanFloat();
          ScanFloat(); ScanFloat(); ScanFloat(); ScanFloat();
          ScanFloat(); ScanFloat(); ScanFloat(); ScanFloat();
          ScanFloat(); ScanFloat(); ScanFloat(); ScanFloat();
          ScanFloat(); ScanFloat(); ScanFloat(); ScanFloat();
          ScanInt();
          ScanFloat(); ScanFloat(); ScanFloat(); ScanFloat();
          ScanFloat(); ScanFloat(); ScanFloat();
          EndChunk();

          if(mat->Tex[0] == 0)
          {
            //tex = FindTexture(texname);
            tex = 0;
            if(tex)
              mat->Tex[0] = tex;
          }
        }
        else
        {
          SkipChunk();
        }
        sScanSpace(Scan);
      }
      EndChunk();
    }
    else if(sCmpString(chunk,"XSI_Material")==0)
    {
      mat = new sXSIMaterial;
      sCopyString(mat->Name,name,sizeof(mat->Name));
      mat->TFlags[0] = 0;
      mat->TFlags[1] = 0;
      mat->Flags |= 0;
			sDPrintF("%08x:<%s>\n",Scan,name);
      Materials->Add(mat);


      max = ScanInt();
      for(i=0;i<max;i++)
      {
        ScanString(buffer,sizeof(buffer));
        ScanString(buffer,sizeof(buffer));
      }
      sScanSpace(Scan);
      tsup[0][0]=0;
      tsup[1][0]=0;
      texena[0] = 0;
      texena[1] = 0;
      while(*Scan!=0 && *Scan!='}' && !Error)
      {
        ScanChunk(indent+1,chunk,name);

        if(sCmpString(chunk,"XSI_Shader")==0)
        {
          ScanString(buffer,sizeof(buffer));
          ScanInt();
          pcount = ScanInt();
          ccount = ScanInt();
          for(i=0;i<pcount;i++)
          {
            ScanString(pname,sizeof(pname));
            ScanString(tname,sizeof(tname));
            if(sCmpString(tname,"STRING")==0)
            {
              ScanString(buffer,sizeof(buffer));
              if(sCmpString(pname,"tspace_id1")==0)   sCopyString(tsup[0],buffer,sizeof(tsup[0]));
              if(sCmpString(pname,"tspace_id" )==0)   sCopyString(tsup[0],buffer,sizeof(tsup[0]));
              if(sCmpString(pname,"tspace_id2")==0)   sCopyString(tsup[1],buffer,sizeof(tsup[1]));
            }
            else if(sCmpString(tname,"FLOAT")==0)
            {
              fval = ScanFloat();
              ival = sRange<sInt>(255*fval,255,0);
              if(sCmpString(pname,"Ambient.red")==0)    mat->Ambient = (mat->Ambient & 0x00ffff) | (ival << 16);
              if(sCmpString(pname,"Ambient.green")==0)  mat->Ambient = (mat->Ambient & 0xff00ff) | (ival <<  8);
              if(sCmpString(pname,"Ambient.blue")==0)   mat->Ambient = (mat->Ambient & 0xffff00) | (ival <<  0);
              if(sCmpString(pname,"Diffuse.red")==0)    mat->Diffuse = (mat->Diffuse & 0x00ffff) | (ival << 16);
              if(sCmpString(pname,"Diffuse.green")==0)  mat->Diffuse = (mat->Diffuse & 0xff00ff) | (ival <<  8);
              if(sCmpString(pname,"Diffuse.blue")==0)   mat->Diffuse = (mat->Diffuse & 0xffff00) | (ival <<  0);
              if(sCmpString(pname,"Specular.red")==0)   mat->Specular = (mat->Specular & 0x00ffff) | (ival << 16);
              if(sCmpString(pname,"Specular.green")==0) mat->Specular = (mat->Specular & 0xff00ff) | (ival <<  8);
              if(sCmpString(pname,"Specular.blue")==0)  mat->Specular = (mat->Specular & 0xffff00) | (ival <<  0);
              if(sCmpString(pname,"Shininess")==0)      mat->Specularity = fval;
            }
            else if(sCmpString(tname,"INTEGER")==0)
            {
              ival = ScanInt();
            }
            else if(sCmpString(tname,"BOOLEAN")==0)
            {
              ival = ScanInt();
              if(ival!=0)
              {
//                if(sCmpString(pname,"WrapS1")==0)     mat->TFlags[0] |= sMTF_TILE;
//                if(sCmpString(pname,"WrapT1")==0)     mat->TFlags[0] |= sMTF_TILE;
//                if(sCmpString(pname,"Refmap1")==0)    mat->TFlags[0] |= sMTF_UVENVI;
                if(sCmpString(pname,"Texture_1_Enable")==0) texena[0]=1;

//                if(sCmpString(pname,"WrapS2")==0)     mat->TFlags[1] |= sMTF_TILE;
//                if(sCmpString(pname,"WrapT2")==0)     mat->TFlags[1] |= sMTF_TILE;
//                if(sCmpString(pname,"Refmap2")==0)    mat->TFlags[1] |= sMTF_UVENVI;
                if(sCmpString(pname,"Texture_2_Enable")==0) texena[1]=1;
              }
              else
              {
                if(sCmpString(pname,"Enable_Ambient")==0)  mat->Ambient = 0xffffffff;
                if(sCmpString(pname,"Enable_Diffuse")==0)  mat->Diffuse = 0xffffffff;
                if(sCmpString(pname,"Enable_Specular")==0) mat->Specular = 0xffffffff;
                if(sCmpString(pname,"Enable_Shininess")==0) mat->Specularity = 0;
//                if(sCmpString(pname,"Enable_Lighting")==0) mat->Flags &= ~sMF_LIGHTMASK;
              }
              
            }
            else
            {
              Error = sTRUE;
            }
          }
          for(i=0;i<ccount;i++)
          {
            ScanString(pname,sizeof(pname));
            ScanString(buffer,sizeof(buffer));
            ScanString(tname,sizeof(tname));
            if(sCmpString(tname,"IMAGE")==0)
            {
              if((sCmpString(pname,"Texture_1")==0 || sCmpString(pname,"tex")==0) && texena[0])
              {
                if(mat->Tex[0] == 0 
                  && (tsup[0][0]!=0 /*|| (mat->TFlags[0]&sMTF_UVMASK)==sMTF_UVENVI*/) 
                  && TSCount<sXSI_MAXTS)
                {
                  //tex = FindTexture(buffer);
                  tex = 0;
                  if(tex)
                  {
                    mat->Tex[0] = tex;
                    //mat->Mode = sMBM_TEX;
                  }
                  sCopyString(TSName[TSCount],tsup[0],XSISIZE);
                  mat->TUV[0] = TSCount++;
                }
              }

              if(sCmpString(pname,"Texture_2")==0 && texena[1])
              {
                if(mat->Tex[1] == 0 
                  && (tsup[1][0]!=0 /*|| (mat->TFlags[1]&sMTF_UVMASK)==sMTF_UVENVI*/) 
                  && TSCount<sXSI_MAXTS)
                {
                  //tex = FindTexture(buffer);
                  tex = 0;
                  if(tex)
                  {
                    mat->Tex[1] = tex;
                    //mat->Mode = sMBM_MUL;
//                    mat->TFlags[1] = sMTF_FILTER|sMTF_MIPMAP;//|sMTF_UVENVI;
                  }
                  sCopyString(TSName[TSCount],tsup[1],XSISIZE);
                  mat->TUV[1] = TSCount++;
                }
              }
            }
          }
          sScanSpace(Scan);
          while(*Scan!=0 && *Scan!='}' && !Error)
          {
            ScanChunk(indent+1,chunk,name);
            SkipChunk();
            sScanSpace(Scan);
          }
          EndChunk();
        }
        else
        {
          SkipChunk();
        }
        sScanSpace(Scan);
      }      
      /*if((mat->TFlags[1] & sMTF_UVMASK)==0)
        mat->TFlags[1] |= sMTF_UV1;*/
      if(mat->Specularity>0.0001f || mat->Ambient!=0xffffffff 
        || mat->Diffuse != 0xffffffff || mat->Specular != 0xffffffff)
      {
//        mat->Flags = (mat->Flags & (~sMF_LIGHTMASK)) | sMF_LIGHTMAT;
//        sDPrintF("extlight: %08x %08x %08x\n",mat->Diffuse.Color,mat->Ambient.Color,mat->Specular.Color);
      }
      else
      {
//        mat->Flags |= sMF_COLBOTH;
      }
      //mat->TFlags[0] |= sMTF_FILTERMIN | sMTF_FILTERMAG | sMTF_MIPMAP;
      //mat->TFlags[1] |= sMTF_FILTERMIN | sMTF_FILTERMAG | sMTF_MIPMAP;
      EndChunk();
    }
    else
    {
      SkipChunk();
    }
    sScanSpace(Scan);
  }
  EndChunk();
}

/****************************************************************************/

void sXSILoader::ScanMesh(sInt indent,sXSIModel *model)
{
  sChar buffer[256];
  sChar chunk[XSISIZE];
  sChar name[XSISIZE];
  sChar *cmd;
  sInt supports,i,j,k,max;
  sInt fcount;
  sInt vcount;
  sXSICluster *cluster;
  sInt cr,cg,cb,ca;
  sInt set,set2;

  sInt PosCount;
  sVector *Pos;
  sInt NormCount;
  sVector *Norm;
  sInt ColorCount;
  sU32 *Color;
  sInt UVCount[sXSI_MAXUV];
  sF32 *UV[sXSI_MAXUV];
  sChar UVName[sXSI_MAXUV][XSISIZE];

// init shape holder

  PosCount = 0;
  Pos = 0;
  NormCount = 0;
  Norm = 0;
  ColorCount = 0;
  Color = 0;
  for(i=0;i<sXSI_MAXUV;i++)
  {
    UVCount[i] = 0;
    UV[i] = 0;
  }

  while(*Scan!=0 && *Scan!='}' && !Error)
  {
    ScanChunk(indent,chunk,name);

    if(sCmpString(chunk,"SI_Shape")==0)
    {
      supports = ScanInt();
      ScanString(buffer,sizeof(buffer));
      if(sCmpString(buffer,"ORDERED")!=0)
        Error = sTRUE;

      for(i=0;i<supports && !Error;i++)
      {
        max = ScanInt();
        ScanString(buffer,sizeof(buffer));
        if(sCmpString(buffer,"POSITION")==0)
        {
          sVERIFY(Pos==0);
          PosCount = max;
          Pos = new sVector[max];

          for(j=0;j<max;j++)
          {
            Pos[j].x = ScanFloat();
            Pos[j].y = ScanFloat();
            Pos[j].z = ScanFloat();
          }
        }
        else if(sCmpString(buffer,"NORMAL")==0)
        {
          sVERIFY(Norm==0);
          NormCount = max;
          Norm = new sVector[max];

          for(j=0;j<max;j++)
          {
            Norm[j].x = ScanFloat();
            Norm[j].y = ScanFloat();
            Norm[j].z = ScanFloat();
          }
        }
        else if(sCmpString(buffer,"COLOR")==0)
        {
          sVERIFY(Color==0);
          ColorCount = max;
          Color = new sU32[max];

          for(j=0;j<max;j++)
          {
            cr = sRange<sInt>(ScanFloat()*255,255,0);
            cg = sRange<sInt>(ScanFloat()*255,255,0);
            cb = sRange<sInt>(ScanFloat()*255,255,0);
            ca = sRange<sInt>(ScanFloat()*256,255,0);
            Color[j] = (ca<<24)|(cb<<16)|(cg<<8)|(cr);
          }
        }
        else if(sCmpString(buffer,"TEX_COORD_UV")==0)
        {
          set = 0;
          sVERIFY(UV[set]==0);
          UVCount[set] = max;
          UV[set] = new sF32[max*2];

          for(j=0;j<max;j++)
          {
            UV[set][j*2+0] = ScanFloat();
            UV[set][j*2+1] = 1.0f-ScanFloat();
          }
        }
        else if(sCmpMem(buffer,"TEX_COORD_UV",12)==0)
        {
          j=12;
          set = 0;
          while(buffer[j]>='0' && buffer[j]<='9')
            set = set*10 + (buffer[j++]-'0');
          sVERIFY(set>=0 && set<sXSI_MAXUV);
          sVERIFY(UV[set]==0);

          ScanString(UVName[set],sizeof(UVName[set]));

          UVCount[set] = max;
          UV[set] = new sF32[max*2];

          for(j=0;j<max;j++)
          {
            UV[set][j*2+0] = ScanFloat();
            UV[set][j*2+1] = 1.0f-ScanFloat();
          }
        }
        else 
        {
          Error = sTRUE;
        }
      }

      EndChunk();
    }
    else if(sCmpString(chunk,"SI_PolygonList")==0)
    {
      cluster = new sXSICluster;

      fcount = ScanInt();

      sCopyString(buffer,"|POSITION|",sizeof(buffer));
      i = sGetStringLen(buffer);
      ScanString(buffer+i,sizeof(buffer)-i);
#if DUMPCHUNK
      sDPrintF("  %s",buffer);
#endif

      ScanString(name,sizeof(name));
      cluster->Material = FindMaterial(name);

      vcount = ScanInt();
      cluster->VertexCount = vcount;
      cluster->IndexCount = vcount;

      cluster->Vertices = new sXSIVertex[vcount];
      cluster->Faces = new sInt[vcount*2];

      model->Clusters->Add(cluster);


      j = 0;
      for(i=0;i<fcount;i++)
      {
        max = ScanInt();
        cluster->Vertices[j].Init();
        cluster->Faces[j*2+0] = max;
        cluster->Faces[j*2+1] = j;
        j++;
        for(k=1;k<max;k++)
        {
          cluster->Vertices[j].Init();
          cluster->Faces[j*2+0] = 0;
          cluster->Faces[j*2+1] = j;
          j++;
        }
      }
      sVERIFY(j==vcount);

      cmd = buffer;
      while(!Error && *cmd)
      {
        if(sCmpMem(cmd,"|POSITION",9)==0)
        {
          cmd+=9;
          sVERIFY(Pos);
          for(i=0;i<vcount;i++)
          {
            j = ScanInt();
            cluster->Vertices[i].Pos = Pos[j];
            cluster->Vertices[i].Index = j;
          }
        }
        else if(sCmpMem(cmd,"|NORMAL",7)==0)
        {
          cmd+=7;
          sVERIFY(Norm);
          for(i=0;i<vcount;i++)
          {
            j = ScanInt();
            cluster->Vertices[i].Normal = Norm[j];
            cluster->Vertices[i].Normal.Init(0,0,0,1);
          }
        }
        else if(sCmpMem(cmd,"|COLOR",6)==0)
        {
          cmd+=6;
          sVERIFY(Color);
          for(i=0;i<vcount;i++)
          {
            j = ScanInt();
            cluster->Vertices[i].Color = Color[j];
          }
        }
        else if(sCmpMem(cmd,"|TEX_COORD_UV",13)==0)
        {
          cmd+=13;
          set = 0;
          if(*cmd>='0' && *cmd<='9')
          {
            while(*cmd>='0' && *cmd<='9')
              set = set*10 + ((*cmd++)-'0');

            sVERIFY(set>=0 && set<sXSI_MAXUV);
            sVERIFY(UV[set])
            for(set2=0;set2<4;set2++)
              if(sCmpString(TSName[cluster->Material->TUV[set2]],UVName[set])==0)
                break;
          }
          else
          {
            set2 = 0;
          }

          for(i=0;i<vcount;i++)
          {
            j = ScanInt();
            if(set2>=0 && set2<2)
            {
              cluster->Vertices[i].UV[set2][0] = UV[set][j*2+0];
              cluster->Vertices[i].UV[set2][1] = UV[set][j*2+1];
            }
          }
        }
        else
        {
          Error = sTRUE;
        }
      }

      EndChunk();
    }
    else if(sCmpString(chunk,"SI_TriangleList")==0)
    {
      Error = sTRUE;
      SkipChunk();
    }
    else
    {
      SkipChunk();
    }
    sScanSpace(Scan);
  }
  EndChunk();

// free shape holder memory

  if(Pos)
    delete[] Pos;
  if(Norm)
    delete[] Norm;
  if(Color)
    delete[] Color;
  for(i=0;i<sXSI_MAXUV;i++)
    if(UV[i])
      delete[] UV[i];
}

/****************************************************************************/

sXSIFCurve *sXSILoader::ScanFCurve(sInt indent)
{
  sChar buffer[XSISIZE];
  sXSIFCurve *fcurve;
  sInt axe;
  sInt floats;
  sInt keys;
  sInt i,j;
  static sChar *names[] = 
  {
    "SCALING-X",
    "SCALING-Y",
    "SCALING-Z",
    "ROTATION-X",
    "ROTATION-Y",
    "ROTATION-Z",
    "TRANSLATION-X",
    "TRANSLATION-Y",
    "TRANSLATION-Z",
  };

  fcurve = new sXSIFCurve;


  ScanString(buffer,sizeof(buffer));

  ScanString(buffer,sizeof(buffer));
  fcurve->Offset = -1;
  for(i=0;i<9;i++)
  {
    if(sCmpString(buffer,names[i])==0)
      fcurve->Offset = i;
  }
  if(fcurve->Offset==-1)
    Error = sTRUE;

  ScanString(buffer,sizeof(buffer));
  if(sCmpString(buffer,"CUBIC")!=0 && sCmpString(buffer,"LINEAR")!=0)
    Error = sTRUE;

  axe = ScanInt();
  floats = ScanInt();
  keys = ScanInt();
  if(axe!=1)
    Error = sTRUE;

  fcurve->KeyCount = keys;
  fcurve->Keys = new sXSIFCurveKey[keys];

  if(!Error)
  {
    for(i=0;i<keys;i++)
    {
      fcurve->Keys[i].Num = ScanFloat();
      fcurve->Keys[i].Pos = ScanFloat();
/*
      fcurve->Keys[i].Spline[0] = ScanFloat();
      fcurve->Keys[i].Spline[1] = ScanFloat();
      fcurve->Keys[i].Spline[2] = ScanFloat();
      fcurve->Keys[i].Spline[3] = ScanFloat();
*/
      for(j=1;j<floats;j++)
        ScanFloat();
      if(fcurve->Offset>=3 && fcurve->Offset<6)
      {
        fcurve->Keys[i].Pos = fcurve->Keys[i].Pos/360.0f;
//        fcurve->Keys[i].Spline[1] = fcurve->Keys[i].Spline[1]/360.0f;
//        fcurve->Keys[i].Spline[3] = fcurve->Keys[i].Spline[3]/360.0f;
      }
    }

    EndChunk();
  }

  return fcurve;
}

/****************************************************************************/

sXSIModel *sXSILoader::ScanModel(sInt indent,sChar *modname)
{
  sChar chunk[XSISIZE];
  sChar name[XSISIZE];
  sXSIModel *model;

  model = new sXSIModel;
  if(sCmpMem(modname,"MDL-",4)==0)
    modname+=4;
  sCopyString(model->Name,modname,sizeof(model->Name));
  Models->Add(model);

  while(*Scan!=0 && *Scan!='}' && !Error)
  {
    ScanChunk(indent,chunk,name);

    if(sCmpString(chunk,"SI_Model")==0)
    {
      model->Childs->Add(ScanModel(indent+1,name));
    }
    else if(sCmpString(chunk,"SI_Mesh")==0)
    {
      ScanMesh(indent+1,model);
    }
    else if(sCmpString(chunk,"SI_Visibility")==0)
    {
      model->Visible = ScanInt();
      EndChunk();
    }
    else if(sCmpString(chunk,"SI_Transform")==0)
    {
      if(sCmpMem(name,"BASEPOSE-",9)==0)
      {
        model->BaseS.x = ScanFloat();
        model->BaseS.y = ScanFloat();
        model->BaseS.z = ScanFloat();
        model->BaseR.x = ScanFloat()/360.0f;
        model->BaseR.y = ScanFloat()/360.0f;
        model->BaseR.z = ScanFloat()/360.0f;
        model->BaseT.x = ScanFloat();
        model->BaseT.y = ScanFloat();
        model->BaseT.z = ScanFloat();
      }
      else
      {
        model->TransS.x = ScanFloat();
        model->TransS.y = ScanFloat();
        model->TransS.z = ScanFloat();
        model->TransR.x = ScanFloat()/360.0f;
        model->TransR.y = ScanFloat()/360.0f;
        model->TransR.z = ScanFloat()/360.0f;
        model->TransT.x = ScanFloat();
        model->TransT.y = ScanFloat();
        model->TransT.z = ScanFloat();
      }
      EndChunk();
    }
    else if(sCmpString(chunk,"SI_FCurve")==0)
    {
      model->FCurves->Add(ScanFCurve(indent+1));
    }
    else
    {
      SkipChunk();
    }
    sScanSpace(Scan);
  }
  EndChunk();

  return model;
}

/****************************************************************************/

void sXSILoader::ScanEnvList(sInt indent)
{
  sInt i,j,max,nr,maxw;
  sChar chunk[XSISIZE];
  sChar name[XSISIZE];
  sChar *s;
  sXSIModel *mesh;
  sXSIModel *null;
  sXSICluster *cluster;
  sXSIVertex *vp;
  sF32 *weight;

  ScanInt();
  while(*Scan!=0 && *Scan!='}' && !Error)
  {
    ScanChunk(indent,chunk,name);

    if(sCmpString(chunk,"SI_Envelope")==0)
    {
      ScanString(name,XSISIZE); s = name; if(sCmpMem(s,"MDL-",4)==0) s+=4;
      mesh = RootModel->FindModel(s);
      ScanString(name,XSISIZE); s = name; if(sCmpMem(s,"MDL-",4)==0) s+=4;
      null = RootModel->FindModel(s);
      if(!mesh) 
        Error = sTRUE;
      if(!null)
        Error = sTRUE;
      if(!Error)
      {
				maxw = 0;
        for(i=0;i<mesh->Clusters->GetCount();i++)
        {
          cluster = mesh->Clusters->Get(i);
					maxw += cluster->VertexCount;
				}
        
				max = ScanInt();
        weight = new sF32[maxw+1];
				for(i=0;i<maxw+1;i++)
					weight[i] = 0.0f;
        for(i=0;i<max;i++)
        {
					nr = ScanInt();
					if(nr<0||nr>=maxw)
            Error = sTRUE;
          weight[nr] = ScanFloat();
        }
        EndChunk();

        if(!Error)
        {
          for(i=0;i<mesh->Clusters->GetCount();i++)
          {
            cluster = mesh->Clusters->Get(i);
            vp = cluster->Vertices;
            for(j=0;j<cluster->VertexCount;j++)
            {
              sVERIFY(vp->Index<maxw);
              if(weight[vp->Index]>0.0f)
                vp->AddWeight(null,weight[vp->Index]);
              vp++;
            }
          }
        }

        delete[] weight;
      }
    }
    else
    {
      SkipChunk();
    }
    sScanSpace(Scan);
  }
  EndChunk();
}

/****************************************************************************/
/***                                                                      ***/
/***   Main Class                                                         ***/
/***                                                                      ***/
/****************************************************************************/

sXSILoader::sXSILoader()
{
  Textures = new sList<sXSITexture>;
  Materials = new sList<sXSIMaterial>;
  Models = new sList<sXSIModel>;
  RootModel = 0;
  TSCount = 0;
}

/****************************************************************************/

sXSILoader::~sXSILoader()
{
}

/****************************************************************************/

void sXSILoader::Clear()
{
  Textures->Clear();
  Materials->Clear();
  Models->Clear();
}

/****************************************************************************/

void sXSILoader::Tag()
{
  sBroker->Need(Textures);
  sBroker->Need(Materials);
  sBroker->Need(Models);
  sBroker->Need(RootModel);
}

/****************************************************************************/

sXSITexture *sXSILoader::FindTexture(sChar *name)
{
  sInt i;
  sU32 s;
  sBitmap *bm;
  sChar path[XSIPATH];
  sXSITexture *tex;

  tex = 0;

  for(i=0;i<Textures->GetCount();i++)
    if(sCmpString(Textures->Get(i)->Name,name)==0)
      return Textures->Get(i);

  sCopyString(path,BasePath,XSIPATH);
  sAppendString(path,name,XSIPATH);

  bm = sLoadBitmap(path);
  if(bm)
  {
    s=bm->XSize;
    while((s&1)==0)
      s = s>>1;
    if(s==1)
    {
      s=bm->YSize;
      while((s&1)==0)
        s = s>>1;
      if(s==1)
      {
        tex = new sXSITexture;
        tex->Bitmap = bm;
        sCopyString(tex->Name,name,sizeof(tex->Name));

        Textures->Add(tex);
      }
    }
  }
  
  if(tex==0)
    Error = sTRUE;
  return tex;
}

/****************************************************************************/

sXSIMaterial *sXSILoader::FindMaterial(sChar *name)
{
  sInt i;

  for(i=0;i<Materials->GetCount();i++)
    if(sCmpString(Materials->Get(i)->Name,name)==0)
      return Materials->Get(i);
  ;
  Error = sTRUE;
  return 0;
}

/****************************************************************************/

sBool sXSILoader::LoadXSI(sChar *path)
{
  sChar *mem;
  sInt indent;
  sChar chunk[XSISIZE];
  sChar name[XSISIZE];
  sInt i,j;

  sCopyString(BasePath,path,XSIPATH);
  j = 0;
  for(i=0;BasePath[i];i++)
    if(BasePath[i]=='/')
      j = i+1;
  BasePath[j]=0;

  
  Scan = mem = sSystem->LoadText(path);
 
  if(Scan==0)
    return sFALSE;

  Error = sFALSE;
  Version = 0;
  if(sCmpMem(Scan,"xsi 0300txt 0032",16)==0)
    Version = 300;
  if(sCmpMem(Scan,"xsi 0350txt 0032",16)==0)
    Version = 350;
  if(sCmpMem(Scan,"xsi 0360txt 0032",16)==0)
    Version = 360;
  if(Version==0)
    Error = sTRUE;
  Scan += 16;

  indent = 0;
  while(*Scan!=0 && !Error)
  {
    ScanChunk(indent,chunk,name);

    if(sCmpString(chunk,"SI_MaterialLibrary")==0)
    {
      ScanMatLib(indent+1);
    }
    else if(sCmpString(chunk,"SI_Model")==0)
    {
      if(RootModel)
      {
        Error = sTRUE;
      }
      RootModel = ScanModel(indent+1,name);
    }
    else if(sCmpString(chunk,"SI_EnvelopeList")==0)
    {
      ScanEnvList(indent+1);
    }
    else
    {
      SkipChunk();
    }
    sScanSpace(Scan);
  }
  if(RootModel==0)
    Error = sTRUE;
#if DUMPCHUNK
  sDPrintF("\n");
#endif
#if DUMPMODEL
   sDPrintF("Materials\n");
   for(i=0;i<Materials->GetCount();i++)
   {
     sDPrintF("  %s",Materials->Get(i)->Name);
     if(Materials->Get(i)->Tex[0])
       sDPrintF(" (%s)",Materials->Get(i)->Tex[0]->Name);
     sDPrintF("\n");
   }
   if(!Error)
     RootModel->DumpModel(0);
#endif

  if(Error)
    sDPrintF("xsi error\n");
  else
    sDPrintF("xsi ok\n");
  delete[] mem;
  return !Error;
}

/****************************************************************************/

void sXSILoader::DebugStats()
{
  sInt i,j;
  sXSIModel *mod;
  sXSICluster *cl;

  sInt modcount;
  sInt fccount;
  sInt clcount;
  sInt vertcount;

  modcount = 0;
  fccount = 0;
  clcount = 0;
  vertcount = 0;
  for(i=0;i<Models->GetCount();i++)
  {
    mod = Models->Get(i);
   
    modcount++;
    fccount += mod->FCurves->GetCount();
    for(j=0;j<mod->Clusters->GetCount();j++)
    {
      cl = mod->Clusters->Get(j);
      clcount++;
      vertcount += cl->VertexCount;
    }
  }

  sDPrintF("stats:\n");
  sDPrintF("Models: %d\n",modcount);
  sDPrintF("FCurves: %d\n",fccount);
  sDPrintF("Clusters: %d\n",clcount);
  sDPrintF("Vertices: %d\n",vertcount);
}

/****************************************************************************/

void sXSILoader::Optimise()
{
  sInt i,j,k,l;
  sXSIModel *mod;
  sXSIFCurve *fc;
  sXSICluster *cl;
  sBool same;
  sF32 val;
  sXSIVertex *vp;

// remove some stuff

  for(i=0;i<Models->GetCount();i++)
  {
    mod = Models->Get(i);
    Models->Get(i)->Usage = 0;

// remove flat curves

    for(j=0;j<mod->FCurves->GetCount();)
    {
      fc = mod->FCurves->Get(j);

      val = fc->Keys[0].Pos;
      same = sTRUE;
      for(k=0;k<fc->KeyCount;k++)
        if(fc->Keys[k].Pos!=val)
          same = sFALSE;

      if(same)
      {
//        (&mod->TransS.x)[fc->Offset] = val;       // hack for "kerl" NeckRoot. please remove comment!
        mod->FCurves->Rem(fc);
      }
      else
      {
        j++;
      }
    }

// remove hidden clusters

    if(!mod->Visible)
      mod->Clusters->Clear();
  }

// remove unused models (unused = no bone and no cluster)

  for(i=0;i<Models->GetCount();i++)
  {
    mod = Models->Get(i);
    for(j=0;j<mod->Clusters->GetCount();j++)
    {
      mod->Usage|=2;
      cl = mod->Clusters->Get(j);
      vp = cl->Vertices;
      for(k=0;k<cl->VertexCount;k++)
      {
        for(l=0;l<vp->WeightCount;l++)
          vp->WeightModel[l]->Usage|=1;
        vp++;
      }
    }
  }
  for(i=Models->GetCount()-1;i>=0;i--)
  {
    mod = Models->Get(i);
    for(j=0;j<mod->Childs->GetCount();j++)
      if(mod->Childs->Get(j)->Usage)
        mod->Usage |= 4;
  }

  Models->Clear();
  Optimise1(RootModel);

// cluster vertex optimisation

  for(i=0;i<Models->GetCount();i++)
  {
    mod = Models->Get(i);
    for(j=0;j<mod->Clusters->GetCount();j++)
    {
      cl = mod->Clusters->Get(j);
      cl->Optimise();
    }
  }
}

/****************************************************************************/

void sXSILoader::Optimise1(sXSIModel *mod)
{
  sInt i;
  sXSIModel *child;

  for(i=0;i<mod->Childs->GetCount();)
  {
    child = mod->Childs->Get(i);
    if(child->Usage==0)
    {
      mod->Childs->Rem(child);
    }
    else
    {
      Optimise1(child);
      i++;
    }
  }

  Models->Add(mod);
}

/****************************************************************************/
