// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef ___LOADER_HPP__
#define ___LOADER_HPP__

#include "_types.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Loader for 3d model files                                          ***/
/***                                                                      ***/
/****************************************************************************/

class GenMinMesh;

namespace sLoader
{
  struct Vertex;
  struct Face;
  class Material;
  class Texture;
  class Cluster;
  class Model;
  class Scene;

  enum
  {
    MAX_WEIGHTS = 4,
  };

  struct Vertex                              // a vertex
  {
    sVector Pos;                            // position
    sVector Normal;                         // normal (should be recalculated!)
    sU32 Color;                             // one vertex color
    sF32 UV[4][2];                          // multiple sets of 2d-uv

    sInt Index;                             // backlink to model / mesh index for envelopes
    sInt BoneCount;
    sF32 BoneWeight[MAX_WEIGHTS]; 
    Model *BoneModel[MAX_WEIGHTS];

    sBool Equals(const Vertex &) const;
    sBool LessThan(const Vertex &) const;

    void AddBone(Model *bone,sF32 val);
  };

  struct Face                                // a face
  {
    void Init(sInt count);                  // initialize correctly
    void Exit();

    sInt Count;                             // Number of Indices
    sInt *Index;                            // Array of indices
  };

  class Material                            // placeholder for a material
  {
  public:
    sString<256> Name;
    sInt Id;
  };

  class Cluster                             // a chunk of geometry
  {
  public:
    Cluster();
    ~Cluster();
    Material *Mtrl;                         // material for this cluster
    sInt AnimType;                          // 0=static 1=rigid 2=skinned
    sArray2<Face> Faces;                    // faces
    sArray2<Vertex> Vertices;               // vertices
  };

  struct CubicKey                           // cubic fcurve key
  {
    sF32 Value;                             // value at that point
    sF32 LTanX,LTanY;                       // left tangent
    sF32 RTanX,RTanY;                       // right tangent
  };

  sF32 CubicInterpolate(const CubicKey &a,const CubicKey &b,sF32 t);

  class Model                               // one model in the scene
  {
  public:
    Model();
    ~Model();
    sString<256> Name;
    sF32 Transform[9];                      // transform matrix
    sF32 BasePose[9];
    sInt AnimKeys;
    sF32 *AnimCurves[9];
    sInt AnimIndex;                         // for writing out mesh
    sBool Animated;                         // animation present?
    Model *Parent;                          // set during OptimizeR()
    sInt OriginalVertexCount;               // needed for weighting
    sBool Visibility;                        // softimage visibility (0 = invisible)

    sAutoArray<Model> Childs;               // model hirarchy for FK
    sAutoArray<Cluster> Clusters;           // geometry data
  };

  class Scene                               // the result of loading a scene
  {
    sBool Optimized;
    void OptimizeR(Model *,const sMatrix &mat);
    void VertexListSort(const Vertex *verts,sInt *inds,sInt l,sInt r);
    Model *FindModelR(const sChar *name,Model *parent);

  public:
    Scene();
    ~Scene();

    Model *Root;                            // the scene root object (before optimize)
    sAutoArray<Material> Mtrls;             // material library
    sPtrArray<Cluster> Clusters;            // all clusters (after optimize)
    sPtrArray<Model> Models;                // all models, linear order
    Material *FindMtrl(const sChar *name);
    Model *FindModel(const sChar *name);

    sBool LoadCube();                       // create debugging data
    sBool LoadXSI(sChar *name);             // load a real xsi scene

    sBool Optimize();                       // optimize mesh

    sBool CreateMesh(GenMinMesh *);         // write data to minmesh
  };

};

/****************************************************************************/

#endif
