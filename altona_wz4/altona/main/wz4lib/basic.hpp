/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) by Dierk Ohlerich                                    ***/
/***   all rights reserverd                                               ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WERKKZEUG4_BASIC_HPP
#define FILE_WERKKZEUG4_BASIC_HPP

#ifndef __GNUC__
#pragma once
#endif

#include "base/types2.hpp"
#include "base/math.hpp"
#include "util/image.hpp"
#include "doc.hpp"


/****************************************************************************/

class AnyType : public wObject
{
public:
  AnyType();
};

/****************************************************************************/

class GroupType : public wObject
{
public:
  GroupType();
  ~GroupType();
  
  sArray<wObject *> Members;

  void Add(wObject *obj);

  // Helpers to automatically build groups out of several elements and
  // access them later. The static variant of Get will fall-back sensibly if
  // the input object is not a Group (a non-group object is treated as a group containing itself).
  static GroupType *Make(wObject *e1,wObject *e2);
  static GroupType *Make(wObject *e1,wObject *e2,wObject *e3);

  template<typename T> T *Get(sInt ind) { return (ind<Members.GetCount()) ? (T*) Members[ind] : 0; }
  template<typename T> static T *Get(wObject *obj,sInt ind) { return (T*) GetRaw(obj,ind); }

private:
  static wObject *GetRaw(wObject *obj,sInt ind);
  wObject *Copy();
};

/****************************************************************************/

class ScreenshotProxy : public wObject
{
//  wOp *OpLink;
public:
  ScreenshotProxy();
  ~ScreenshotProxy();
//  void SetOp(wOp *op) {OpLink = op; }   // there is no memory management. this is scary!
//  wOp *GetOp() { return OpLink; };
  wObject *Copy();

  wObject *Root;
  sViewport View;
  sF32 Zoom;
  sInt SizeX;
  sInt SizeY;
  sInt Multisample;
  sInt Flags;
  sInt Strobe;
  sF32 StartTime;
  sF32 EndTime;
  sF32 FPS;

  sString<sMAXPATH> SaveName;
};

/****************************************************************************/

class TextObject : public wObject
{
public:
  TextObject();
  sTextBuffer Text;
  wObject *Copy();
};

struct BitmapAtlasEntry
{
  sRect Pixels;   // pixel coordinates
  sFRect UVs;      // uv coordinates (unadjusted for pixel centers, [0..1])
};


class BitmapAtlas
{
public:  
  sArray<BitmapAtlasEntry> Entries;

  template <class streamer> void Serialize_(streamer &stream);
  void Serialize(sWriter &stream);
  void Serialize(sReader &stream);

//  void Default(sInt sizex, sInt sizey);

//  void Default(const sImage *img) { Default(img->SizeX,img->SizeY); }
//  void Default(const sImageData *img) { Default(img->SizeX, img->SizeY); }

  BitmapAtlas() {};
  BitmapAtlas(const BitmapAtlas &b) { Entries=b.Entries; }
  BitmapAtlas & operator= (const BitmapAtlas &b) { Entries=b.Entries; return *this; }
  void InitAtlas(sInt power,sInt xs,sInt ys);       // initialize with (1<<power) tiles
};


class BitmapBase : public wObject
{
public:
  BitmapAtlas Atlas;

  BitmapBase();
  virtual ~BitmapBase() {}
  virtual void CopyTo(sImage *dest) = 0;
  virtual void CopyTo(sImageI16 *dest) = 0;
  virtual void CopyTo(sImageData *dest,sInt format);  // will use CopyTo(sImage)
};

class Texture2D : public wObject
{
public:
  Texture2D();
  ~Texture2D();
  class sTextureBase *Texture;
  sImageData *Cache;                // waste memory to allow saving of textures
  sString<256> Name;    // this string is a hack and must go !

  BitmapAtlas Atlas;

  wObject *Copy();
  void CopyFrom(Texture2D *);

  void ConvertFrom(BitmapBase *,sInt format);
  void ConvertFrom(sImage *,sInt format);
  void ConvertFrom(sImageData *);

  template <class streamer> void Serialize_(streamer &stream);
  void Serialize(sWriter &stream);
  void Serialize(sReader &stream);
};

/****************************************************************************/

class CubemapBase : public wObject
{
public:
  CubemapBase();
  virtual void CopyTo(sImage **dest) = 0;
  virtual void CopyTo(sImageData *dest,sInt format);  // will use CopyTo(sImage)
};

class TextureCube : public wObject
{
public:
  TextureCube();
  ~TextureCube();
  wObject *Copy();
  class sTextureCube *Texture;
  sImageData *Cache;                // waste memory to allow saving of textures
  void ConvertFrom(CubemapBase *,sInt format);
  void InitDummy();

  template <class streamer> void Serialize_(streamer &stream);
  void Serialize(sWriter &stream);
  void Serialize(sReader &stream);
};

/****************************************************************************/

struct MeshStats 
{
  MeshStats() { sClear(*this); }
  sInt Vertices;
  sInt Triangles;
  sInt Batches;
};

class MeshBase : public wObject
{ 
public:
  virtual void GatherStats(MeshStats &stat) {}
};

/****************************************************************************/

// here only instance data which is changed by Scene::OpTransform implementations
struct SceneInstance
{
  sMatrix34 Matrix;
  sInt LightFlags;                      // -1 = ignore, probably should be moved to SceneInstances
  sF32 Time;
  sInt NameId;                          // 0 = unchanged, can this be moved to SceneInstances?
  sInt Temp;
  void Init();
};

struct SceneInstances
{
  MeshBase *Object;
  sArray<SceneInstance> Matrices;
  sU32 LodMapping;                      // 0xffffffff unchanged, otherwise 0x00hhmmll lod remapping
  sInt Temp;
  void Init();
};

struct SceneMatrices
{
  void Seed();                          // initialize with one unit matrix, seeding the iteration
  sArray<sMatrix34> Matrices;           // current matrices for building leaves
  sArray<SceneInstances *> Instances;   // instanciated leaves
  ~SceneMatrices() { sDeleteAll(Instances); }
};

class Scene : public wObject
{
protected:
  sArray<sMatrix34> SaveMatrix;
  virtual void OpTransform(SceneMatrices *sm);          // overload this to implement transformation
  virtual void OpFilter(SceneMatrices *sm,sInt begin,sInt end);
  void TransformCount(SceneMatrices *sm,sInt count);  // register exact number of copies generated, may be 1
  void TransformAdd(SceneMatrices *sm,const sMatrix34 &mat);  // add the matrix for one of the copies
public:
  MeshBase *Node;
  sArray<Scene *> Childs;
  sMatrix34 Transform;
  sBool DoTransform;
  sBool DoFilter;
  sInt LightFlags;
  sInt NameId;
  sU32 LodMapping;            // 0xffffffff unchanged otherwise 0x00hhmmll for mapping lods
  Scene();
  ~Scene();
  wObject *Copy();
  void Recurse(SceneMatrices *,sInt nameid = 0);
  void AddChild(MeshBase *mesh, const sMatrix34 &mat, sInt nameid=0);
  void GatherStats(MeshStats &stat);
};

/****************************************************************************/

class UnitTest : public wObject
{
public:
  UnitTest();
  ~UnitTest();
  sInt Test(sImage &img,const sChar *filename,sInt flags);
  wObject *Copy();

  sInt Errors;
  sInt Total;
  sInt Skipped;

  sImage Render;
  sImage Compare;
};

/****************************************************************************/

#endif // FILE_WERKKZEUG4_BASIC_HPP

