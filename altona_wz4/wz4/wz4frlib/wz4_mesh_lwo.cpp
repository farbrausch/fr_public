/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "wz4_mesh.hpp"
#include "wz4_mtrl2_ops.hpp"
#include "util/scanner.hpp"
#include "wz4lib/basic_ops.hpp"

/****************************************************************************/

#define LW_DO_TAG(a,b,c,d) (((a)<<24)|((b)<<16)|((c)<<8)|(d))

/****************************************************************************/

static int LW_ReadString(unsigned int &pos, char *in, char *out);
static unsigned char LW_ReadUByte(unsigned int &pos, char *in);
static unsigned short LW_ReadUShort(unsigned int &pos, char *in);
static unsigned int LW_ReadUInt(unsigned int &pos, char *in);
static float LW_ReadFloat(unsigned int &pos, char *in);

/****************************************************************************/

static int LW_ReadString(unsigned int &pos, char *in, char *out)
{
  char c;
  int i=0;
  do
  {
    c=in[pos+i];
    i++;    
    *out++= c;
  }
  while ( c!=0 && c!=13 && c!=10 );
    
  out[-1]=0;
  if (c==10 || c==13)
    i++;

  pos+=i;
  return i-1;
}

static unsigned char LW_ReadUByte(unsigned int &pos, char *in)
{
  return in[pos++];
}

static unsigned short LW_ReadUShort(unsigned int &pos, char *in)
{  
  unsigned short r1 = LW_ReadUByte(pos,in);
  unsigned short r2 = LW_ReadUByte(pos,in);
  return r2|(r1<<8);
}


static unsigned int LW_ReadUInt(unsigned int &pos, char *in)
{
  unsigned int r1 = LW_ReadUShort(pos,in);
  unsigned int r2 = LW_ReadUShort(pos,in);
  return r2|(r1<<16);
}


static float LW_ReadFloat(unsigned int &pos, char *in)
{  
  unsigned int r = LW_ReadUInt(pos, in);
  return ((float *)(&r))[0];
}

/****************************************************************************/

class tLWObject
{
//Static 
public:
  static tLWObject *Load(char *lwo, unsigned int size, float scale=1.0f);

//Instance
public:
  tLWObject();
  ~tLWObject();

  inline unsigned int GetNbOfVertices() { return nbvertices; }
  inline unsigned int GetNbOfIndices() { return nbindices; }
  inline unsigned int *GetIndices() { return indices; }
  inline unsigned int GetNbOfTex() { return nbtex; }

  inline float *GetVertices() { return vertices; }               /* Get vertices */
  inline float *GetVNormals() { return vnormals; }               /* Get normals of vertices */
  inline float *GetFNormals() { return fnormals; }               /* Get normals of faces */
  inline float *GetSTs( unsigned int tex ) { return vst[tex]; }  

  void CalcNormals();

protected:
  void Normalize( float &x, float &y, float &z);

  unsigned int nbvertices;
  float *vertices;
  unsigned int *vcolors;
  float *vnormals;
  
  unsigned int nbtex;
  
  float *vst[16];

  unsigned int nbindices;
  unsigned int *indices;
  float *fnormals;

  char texturename[1024];
};


/****************************************************************************/

tLWObject *tLWObject::Load(char *in, unsigned int size, float scale)
{
  tLWObject *ret= new tLWObject();
  unsigned int pos=0;

  //Controls the small state maschine
  bool bFoundFORM=false;
  bool bFoundLWO2=false;
  bool bFoundUV1=false;

  while ( pos<size )
  {
    switch (LW_ReadUInt(pos,in))
    {
      case LW_DO_TAG('F','O','R','M') :
        pos += 4;
        bFoundFORM = true;
      break;
      
      case LW_DO_TAG('L','W','O','2') :
        bFoundLWO2 = true;
      break;
     
      case LW_DO_TAG('P','N','T','S') :
      {
        ret->nbvertices=LW_ReadUInt(pos,in)/12;                  
        ret->vertices=new float[ret->nbvertices*3];
        for (unsigned int i=0;i<(ret->nbvertices*3);i++)
        {
          ret->vertices[i] = LW_ReadFloat(pos,in)*scale;
        }
      }
      break;
      
      case LW_DO_TAG('P','O','L','S') :
      {        
        unsigned int chunksize = LW_ReadUInt(pos,in);
        unsigned int tmppos = pos + chunksize;
        
        if (LW_ReadUInt(pos,in)==LW_DO_TAG('F','A','C','E'))
        {
          ret->nbindices = (chunksize-4)/8;
          ret->indices   = new unsigned int[ret->nbindices*3];
          unsigned int *tmp = ret->indices;

          while ( pos < tmppos )
          {             
            unsigned short id= LW_ReadUShort(pos,in);

            if ( (id&1023) == 3)
            {
              tmp[0]=LW_ReadUShort(pos, in);
              tmp[1]=LW_ReadUShort(pos, in);
              tmp[2]=LW_ReadUShort(pos, in);
              tmp+=3;
            }
            else
            {              
              pos = tmppos;
              ret->nbindices = 0;
              delete []ret->indices;
              ret->indices = 0;               
              break;
            }
          }
        }
        else
        {        
          pos = tmppos;
        }
      }
      break;

/*      
      case LW_DO_TAG('C','L','I','P') :
      {
        unsigned int uiTPos = LW_ReadUInt(pos,in);

        uiTPos += pos;
        
        LW_ReadUInt(pos,in);
        if (LW_ReadUInt(pos,in) == LW_DO_TAG('S','T','I','L'))
        {
          LW_ReadUShort(pos, in);
          int iLen = LW_ReadString(pos, acTextureName, in);
        }
        
        pos = uiTPos;
      }
      break;
*/
      case LW_DO_TAG('V','M','A','P'):
      {
        unsigned int uiSize = LW_ReadUInt(pos,in);
        unsigned int uiTmp = pos + uiSize;
        if (LW_ReadUInt(pos,in)==LW_DO_TAG('T','X','U','V'))
          if (LW_ReadUShort(pos,in)==2)
          {
            char acTxt[64];
            int iLen = LW_ReadString(pos, in, acTxt ); iLen;
            
            ret->vst[ret->nbtex] = new float[ret->nbvertices*2];            

             bFoundUV1 = true;
            //Read UV-Data
            while (pos<uiTmp)
            {
              unsigned int uiI;
              
              if (LW_ReadUByte(pos, in)!=255)                             
                uiI = (unsigned int)LW_ReadUShort(--pos, in);
              else
                uiI = LW_ReadUInt(--pos, in);
              
              float v = LW_ReadFloat(pos,in);
              float u = LW_ReadFloat(pos,in);
              ret->vst[ret->nbtex][uiI*2+0] = v; 
              ret->vst[ret->nbtex][uiI*2+1] = 1.0-u;
            }
          }
          ret->nbtex++;
          pos = uiTmp;
      }
      break;
      
      default :        
        pos+= LW_ReadUInt(pos,in);
      break;
    }

    /*
    if (bFoundFORM && !bFoundLWO2)
    {
      delete ret;
      return 0;
    } 
    */

    if (!bFoundFORM)
    {      
      delete ret;
      return 0;
    }
  }
  return ret;
}

void tLWObject::Normalize( float &x, float &y, float &z)
{
  float l= sqrtf(x*x+y*y+z*z);
  x/=l;
  y/=l;
  z/=l;
}

void tLWObject::CalcNormals()
{
  //if face normals not exist, create face normals
  if(fnormals==0)
  {
    fnormals=new float[nbindices*3];
    for(unsigned int i=0;i<nbindices;i++)
    {
      float x1=vertices[indices[i*3+0]*3+0];
      float y1=vertices[indices[i*3+0]*3+1];
      float z1=vertices[indices[i*3+0]*3+2];

      float x2=vertices[indices[i*3+1]*3+0];
      float y2=vertices[indices[i*3+1]*3+1];
      float z2=vertices[indices[i*3+1]*3+2];

      float x3=vertices[indices[i*3+2]*3+0];
      float y3=vertices[indices[i*3+2]*3+1];
      float z3=vertices[indices[i*3+2]*3+2];

      x2-=x1;
      y2-=y1;
      z2-=z1;

      x3-=x1;
      y3-=y1;
      z3-=z1;
      
      float x=y2*z3-z2*y3; 
      float y=z2*x3-x2*z3; 
      float z=x2*y3-y2*x3;

      Normalize(x,y,z);

      fnormals[i*3+0]=x; 
      fnormals[i*3+1]=y;
      fnormals[i*3+2]=z;
    }
  }

  // if vertex normals not exist, create normals
  if(vnormals==0)
  {
    vnormals=new float[nbvertices*3];
    for(unsigned int i=0;i<nbvertices;i++)
    {
      float x=0;
      float y=0;
      float z=0;
      for(unsigned int j=0;j<nbindices;j++)
      {
        if(i==indices[j*3+0] || i==indices[j*3+1] || i==indices[j*3+2])
        {
          x+=fnormals[j*3+0];
          y+=fnormals[j*3+1];
          z+=fnormals[j*3+2];
        }
      }
      Normalize(x,y,z);
      vnormals[i*3+0]=x;
      vnormals[i*3+1]=y;
      vnormals[i*3+2]=z;
    }
  }
}

tLWObject::tLWObject()
{
  nbvertices=nbindices=0;
  vertices=0;
  vcolors=0;
  vnormals=0;
  nbtex=0;
  indices=0;
  fnormals=0;
  texturename[0]=0;  


  for(int i=0;i<sizeof(vst)/sizeof(float *);i++)
    vst[i]=0;
}

tLWObject::~tLWObject()
{
  if (vertices)
    delete []vertices;

  if (vcolors)
    delete []vcolors;

  if (vnormals)
    delete []vnormals;
  
  for(int i=0;i<sizeof(vst)/sizeof(float *);i++)
    if (vst[i]) delete []vst[i];

  if (fnormals)
    delete []fnormals;
}

/****************************************************************************/
/***                                                                      ***/
/***   Main                                                               ***/
/***                                                                      ***/
/****************************************************************************/

sBool Wz4Mesh::LoadLWO(const sChar *file)
{
  int i;
  sDInt size=0;
  sU8 *data=sLoadFile(file,size);
  
  if (data==0 || size==0)
  {
    sDPrintF(L"Wz4Mesh::LoadLWO - error can not load %s \n",file);   
    return false;
  }

  tLWObject *lwo=tLWObject::Load((char *)data,size);
  delete data;

  if (lwo==0)
  {
    sDPrintF(L"Wz4Mesh::LoadLWO - no valid LWO file %s \n",file);   
    return false;
  }   
  lwo->CalcNormals();
    
  Vertices.Resize(lwo->GetNbOfVertices());

  float *vert=lwo->GetVertices();
  float *vnrm=lwo->GetVNormals();
  float *vst0=lwo->GetSTs(0);
  float *vst1=lwo->GetSTs(1);

  for (i=0;i<Vertices.GetCount();i++)
  {
    int j=i*3;
    int k=i*2;
    Vertices[i].Pos=sVector31(vert[j+0],vert[j+1],vert[j+2]);
    
    if (vnrm)
      Vertices[i].Normal=sVector30(vnrm[j+0],vnrm[j+1],vnrm[j+2]);

    if (vst0)
    {
      Vertices[i].U0=vst0[k+0];
      Vertices[i].V0=vst0[k+1];
    }

    if (vst1)
    {
      Vertices[i].U1=vst1[k+0];
      Vertices[i].V1=vst1[k+1];
    }

  }


  AddDefaultCluster();
  
  Faces.Resize(lwo->GetNbOfIndices());
  unsigned int *ind=lwo->GetIndices();

  for (i=0;i<Faces.GetCount();i++)
  {
    int j=i*3;
    Faces[i].Cluster=0;
    Faces[i].Count=3;
    Faces[i].Select=0.0f;
    Faces[i].Vertex[0]=ind[j+0];
    Faces[i].Vertex[1]=ind[j+1];
    Faces[i].Vertex[2]=ind[j+2];
  }

  delete lwo;

  //Wz4::XSILoader *load = new Wz4::XSILoader;
  //load->ForceAnim = forceanim;
  //load->ForceRGB = forcergb;
  //sBool result = load->Load(this,file);
  //delete load;
  return true;
}

/****************************************************************************/

