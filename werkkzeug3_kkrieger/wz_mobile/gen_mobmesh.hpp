// This file is distributed under a BSD license. See LICENSE.txt for details.

#pragma once
#include "_types.hpp"
#include "_intmath.hpp"
#if !sPLAYER
#include "doc.hpp"
#endif
#include "doc2.hpp"

/****************************************************************************/

#define MOBMESHSKIN  4


enum GenMobMeshSelectMode
{
  MMS_ADD       = 0x0000,
  MMS_SUB       = 0x0001,
  MMS_SET       = 0x0002,
  MMS_SETNOT    = 0x0003,

  MMS_VERTEX    = 0x0000,
  MMS_FULLFACE  = 0x0004,
  MMS_PARTFACE  = 0x0008,
};

enum GenMobMeshSelectUse
{
  MMU_ALL         = 0,
  MMU_SELECTED    = 1,
  MMU_UNSELECTED  = 2,
};

struct GenMobSample               // a vertex of a face. complete with UV
{
  sInt Index;                     // position index
  sInt TexU0;                     // 1:15:16 texture coordinate
  sInt TexV0;                     // 1:15:16 texture coordinate
  sInt TexU1;                     // 1:15:16 texture coordinate
  sInt TexV1;                     // 1:15:16 texture coordinate
  sSVector3 Color;                // RGB color (for high dynamic range)
  sSVector3 Normal;               // 1:1:14 vertex normal (has to be calculated)
};

struct GenMobVertex               // a vertex
{
  sU8 Select;                     // selection bit
  sU8 Bones;                      // vertex skinning
  sU8 TempByte;                   // use as you like
  sU8 pad3;
  sIVector3 Pos;                  // 1:15:16 position
  sIVector3 Normal;               // 1:17:14 normal

  sU8 Matrix[MOBMESHSKIN];        // skinning matrix index
  sS16 Weight[MOBMESHSKIN];       // 1:1:14 skinning wieght
};

struct GenMobFace                 // a face
{
  sU8 Count;                      // number of vertices. 3 or 4
  sU8 Select;                     // selection bit
  sU16 Cluster;                   // material cluster or 0 (deleted)
  GenMobSample Vertex[4];         // up to 4 vertices.
  sSVector3 Normal;               // 1:1:14 face normal (has to be calculated)
  sU16 pad;
};

struct GenMobCluster
{
  sInt Flags;
  sInt TFlags[2];
#if !sPLAYER
  GenOp *TextureOp[2];
#endif
  class SoftImage *Image[2];

  void Init();
};

struct GenMobCamKey
{
  sInt Time;
  union
  {
    sInt Value[6];
    struct
    {
      sIVector3 Cam;
      sIVector3 Target;
    };
  };
};

class GenMobMesh : public GenObject
{  
public:
  GenMobMesh();
  ~GenMobMesh();
  void Copy(GenMobMesh *);
  GenMobMesh *Copy();

  sInt RefCount;
  void AddRef() { RefCount++; }
  void Release() { RefCount--; if(RefCount==0) delete this; }

  void Transform(sInt sel,const sIMatrix34 &mat,sInt src=0,sInt dest=0);
  void SelectAll(sInt in,sInt out);
  void SetAllFaces(sInt sel);
  void SetAllVerts(sInt sel);
  void CalcNormals();
  void MergeVerts();

  void SortSpline();
  void CalcSpline(sInt i,sIVector3 &c,sIVector3 &t);


  GenMobVertex *AddVertices(sInt count);
  GenMobFace *AddFaces(sInt count);
  GenMobCluster *AddClusters(sInt count);
  GenMobVertex *AddGrid(sInt tx,sInt ty,sInt flags);
  void Add(const GenMobMesh *add,sBool newclusters);

  sArray2<GenMobVertex> Vertices;
  sArray2<GenMobFace> Faces;
  sArray2<GenMobCluster> Clusters;
  sArray2<GenMobCamKey> CamSpline;
//  sInt CreaseAngle;                 // cos(alpha) für crease

  class SoftMesh *Soft;         // use for caching
};

GenMobMesh * CalcMobMesh(GenNode *node);

/****************************************************************************/

#if !sMOBILE
void AddMobMeshClasses(GenDoc *doc);
#endif
extern GenHandlerArray GenMobMeshHandlers[];

/****************************************************************************/
