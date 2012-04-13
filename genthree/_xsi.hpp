// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef ___XSI_HPP__
#define ___XSI_HPP__

#include "_types.hpp"
#include "_start.hpp"

/****************************************************************************/
/****************************************************************************/

#define sXSI_MAXUV  256           // maximum uv-sets per cluster
#define sXSI_MAXWEIGHT 16         // maximum relevant bone-weigth
#define sXSI_MAXTS  256           // maximum total texture supports in scene
#define XSISIZE 256               // maximum xsi string size
#define XSINAME 256
#define XSIPATH 4096

struct sXSIVertex
{
  sInt Index;                     // position index from which this vertex was created
  sVector Pos;                    // 3d Position
  sVector Normal;                 // 3d Normal
  sU32 Color;                     // Color Support
  sF32 UV[4][2];         // Multiple UV Supports
  sF32 Weight[sXSI_MAXWEIGHT];    // Weights (compressed)
  class sXSIModel *WeightModel[sXSI_MAXWEIGHT];
  sInt WeightCount;
  sInt Temp;

  void Init();
  void AddWeight(class sXSIModel *,sF32 w);
};

/****************************************************************************/

class sXSITexture : public sObject
{
public:
  sXSITexture();
  ~sXSITexture();
  void Tag();

  sChar Name[XSINAME];
  sChar PathName[XSIPATH];
  sBitmap *Bitmap;
};

/****************************************************************************/

class sXSIMaterial : public sObject
{
public:
  sXSIMaterial();
  ~sXSIMaterial();
  void Tag();

  sChar Name[XSINAME];
  sU32 Flags;
  sU32 Mode;
  sU32 TFlags[4];
  sU32 TUV[4];
  sXSITexture *Tex[4];

  sColor Ambient;
  sColor Diffuse;
  sColor Specular;
  sF32 Specularity;
};

/****************************************************************************/

class sXSICluster : public sObject
{
public:
  sXSICluster();
  ~sXSICluster();
  void Clear();
  void Tag();

  void Optimise();

  sInt VertexCount;               // number of vertices
  sInt IndexCount;                // number of face-entries
  sXSIVertex *Vertices;           // vertices
  sXSIMaterial *Material;            // used material

  sInt *Faces;                    // fan,index,fan,index,...
};

/****************************************************************************/

struct sXSIFCurveKey
{
  sInt Num;
  sF32 Pos;
  sF32 Spline[4];

  void Init();
};

class sXSIFCurve : public sObject
{
public:
  sXSIFCurve();
  ~sXSIFCurve();
  void Clear();

  sInt Offset;
  sInt KeyCount;
  sXSIFCurveKey *Keys;
  sInt Index;
};

/****************************************************************************/

class sXSIModel : public sObject
{
public:
  sXSIModel();
  ~sXSIModel();
  void Tag();
  void Clear();

  sChar Name[XSINAME];

  sList<sXSIModel> *Childs;
  sList<sXSIFCurve> *FCurves;
  sList<sXSICluster> *Clusters;

  sVector BaseS,BaseR,BaseT;
  sVector TransS,TransR,TransT;
  sBool Visible;
  sInt Index;                     // use at your will    
  sInt Usage;

  sXSIModel *FindModel(sChar *name);
  void DumpModel(sInt indent);
};

/****************************************************************************/

class sXSILoader : public sObject
{

// document

  sChar BasePath[XSIPATH];

  sXSITexture *FindTexture(sChar *name);
  sXSIMaterial *FindMaterial(sChar *name);
  sChar TSName[sXSI_MAXTS][XSISIZE];
  sInt TSCount;

// type scanning

  sInt ScanInt();
  sF32 ScanFloat();
  void ScanString(sChar *buffer,sInt size);

// general scanning

  void ScanName(sChar *buffer,sInt size);
  void SkipChunk();
  void ScanChunk(sInt indent,sChar *chunk,sChar *name);
  void EndChunk();

// specific parsing

  void ScanMatLib(sInt indent);
  void ScanMesh(sInt indent,sXSIModel *model);
  sXSIFCurve *ScanFCurve(sInt indent);
  sXSIModel *ScanModel(sInt indent,sChar *name);
  void ScanEnvList(sInt indent);

// work

  void Optimise1(sXSIModel *model);

public:

  sBool Error;
  sChar *Scan;
  sInt Version;

  sList<sXSITexture> *Textures;
  sList<sXSIMaterial> *Materials;
  sList<sXSIModel> *Models;
  sXSIModel *RootModel;

  sXSILoader();
  ~sXSILoader();
  void Tag();
  void Clear();

  sBool LoadXSI(sChar *path);
  void Optimise();
  void DebugStats();
};

/****************************************************************************/
/****************************************************************************/

#endif
