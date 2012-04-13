// This file is distributed under a BSD license. See LICENSE.txt for details.

#pragma once
#include "_types.hpp"
#if !sPLAYER
#include "_types2.hpp"
#endif

/****************************************************************************/

namespace Werkkzeug3Textures 
{
  typedef void * GenHandler;
  class GenClass;
  class GenOp;

  enum GenVairantsIds
  {
    GVI_BITMAP_INTRO        = 1,
    GVI_MESH_MOBMESH        = 2,
    GVI_BITMAP_TILE8I       = 3,
    GVI_BITMAP_TILE8C       = 4,
    GVI_BITMAP_TILE16I      = 5,
    GVI_BITMAP_TILE16C      = 6,
  };
};
using namespace Werkkzeug3Textures;

class GenObject
{
public:
  GenObject() { Variant = 0; }
  virtual ~GenObject() {}
  sInt Variant;
};

struct GenNode                              // this is passed to the classes. this is a low profile struct!
{                                           // it may be further processed before execution, like to insert conversions
  Werkkzeug3Textures::GenClass *Class;      // link to class, for optimisations
  Werkkzeug3Textures::GenHandler Handler;   // a pointer to some code
  void *UserData;                           // .. Bitmap_Rotate may store a full bitmap here ..
  void *FullData;                           // Begin/End fullhandler
  sInt UserFlags;
#if !sPLAYER
  Werkkzeug3Textures::GenOp *Op;            // backlink to op
  sAutoArray<GenObject> *Cache;              // backlink to cache

  sInt Variant;                             // used during export optimisation
  sInt SizeX;
  sInt SizeY;

  GenNode *Load;                            // seems to be unused (never read)
  GenNode *StoreLink;                       // during insertion of stores, save the store here
  sInt StoreCount;                          // count required ADDTIONAL stores. so if there are 3 outputs, this is 2.
#endif
#if sPLAYER
  class SoftImage *Image;
  sInt CountOnce;                           // 0 = normal, 1 = iterate once for coutning ops, 2 = don't iterate for counting ops
#endif

  void *StoreCache;

  sInt InputCount;
  sInt LinkCount;
  sInt ParaCount;
  GenNode **Inputs;                         // inputs 
  GenNode **Links;                          // links
  sInt *Para;                               // parameter array
};

struct GenHandlerArray                      // list of handlers defined by module
{
  Werkkzeug3Textures::GenHandler Handler;   // 0 is end of list
  sInt Id;                                  // id stored in file
};

/****************************************************************************/
