/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_EXTRA_MCUBES_HPP
#define FILE_EXTRA_MCUBES_HPP

#include "base/types.hpp"
#include "base/graphics.hpp"
#include "util/taskscheduler.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   old marching cube (not flexible)                                   ***/
/***                                                                      ***/
/****************************************************************************/

struct MCFieldType
{
  sF32 x,y,z;
  sF32 w;
};

//template <class FieldType>
class MarchingCubes
{
public:
  enum MarchingCubesConst
  {
    CellSize = 8,                // a cell holds many cubes 
    GridSize = 16,                 // a grid hold many cells
    ContainerSize = 64,

    MaxVertex = (CellSize+1)*(CellSize+1)*(CellSize+1),
    MaxIndex = (CellSize)*(CellSize)*(CellSize)*15,
  };
private:
  struct Container
  {
    Container *Next;
    sInt Count;
    sVector31 Pos;

    sVector31 Parts[ContainerSize];

    Container();
  };
  struct GeoBuffer
  {
    sGeometry *Geo;
    sInt Vertex;
    sInt Index;
    sInt VertexMax;
    sInt IndexMax;
    sVertexStandard *VP;
    sU16 *IP;
  };


  sInt MyTriTable[256][16];
  Container *FreeContainers;
  Container *Grid[GridSize][GridSize][GridSize];


  sF32 Influence;
  sF32 IsoValue;
  sF32 Scale;


  sArray<GeoBuffer> GeoBuffers;
  sArray<Container *> ActiveCells[2];
  sInt ActiveCellsDB;
  sU32 GeoBufferUseIndex;
  sThreadLock *GeoBufferLock;

  GeoBuffer **ThreadBuffers;
  sInt MaxThreads;

  void Recycle();
  Container *GetContainer();
  void RenderCell(GeoBuffer *gb,Container *con);
  GeoBuffer *GetGeoBuffer();
public:
  MarchingCubes(sInt vertices = 1024*1024);
  ~MarchingCubes();

  void Begin(sStsManager *sched,sStsWorkload *wl,sInt granularity = 1);
  void Render(sVector31 *parts,sInt pn);
  void End();
  void Draw();

  void MCTask(sInt container,sInt thread);
};

/****************************************************************************/
/***                                                                      ***/
/***   GeoBuffer. Usefull in many contexts                                ***/
/***                                                                      ***/
/****************************************************************************/

class GeoBufferHelper
{
public:
  class GeoBuffer
  {
  public:    
    GeoBuffer(GeoBufferHelper *base);
    ~GeoBuffer();
    void Begin(sBool lock = 1);
    void End();
    void Draw();

    sGeometry *Geo;
    sInt VertexAlloc;
    sInt IndexAlloc;
    sInt VertexUsed;
    sInt IndexUsed;
    sF32 *VP;
    sU16 *IP;
    GeoBufferHelper *Base;
  };
private:
  sInt MaxThread;
  sArray<GeoBuffer *> FreeGeos;
  sArray<GeoBuffer *> ReadyGeos;
  sArray<GeoBuffer *> DrawGeos;
  sInt GeoCount;

  sVertexFormatHandle *Format;
  sInt FloatSize;
  sInt VertexMax;
  sInt IndexMax;
  sStsLock Lock;
  sBool Pipelined;
  sGeometryDuration Duration;

  GeoBuffer **ThreadGeo;
public:
  GeoBufferHelper();
  ~GeoBufferHelper();

  void Init(sVertexFormatHandle *fmt,sInt vc,sInt ic,sBool pipelined,sBool static_);
  void Begin();
  void End();
  void Draw();

  GeoBuffer *GetBuffer(sInt thread);
  void ChargeGeo(sInt thread);
  void BeginLoad(sInt thread,sInt vcmax,sInt icmax,sInt &vi,void **vc,sU16 **ic);
  void EndLoad(sInt thread,sInt vc,sInt ic);

  template<typename T> 
  void BeginLoad(sInt thread,sInt vcmax,sInt icmax,sInt &vi,T **vc,sU16 **ic)
  { BeginLoad(thread,vcmax,icmax,vi,(void **)vc,ic); }
};

struct MCPotField
{
  sF32 x,y,z;
  sF32 w;
  static sU32 c;

  static const sInt ColorFlag=0;
};
struct MCPotFieldColor
{
  sF32 x,y,z;
  sF32 w;
  sU32 c;

  static const sInt ColorFlag=1;
};


class MarchingCubesHelper
{
  sInt MaxThread;
  GeoBufferHelper Geos;
  sInt MyTriTable[6][256][16];
  sVertexFormatHandle *Format;

public:
  MarchingCubesHelper(sBool color);
  ~MarchingCubesHelper();
  void Init(sBool pipelined,sBool static_=0);
  void Begin();
  void End();
  void Draw();

  void ChargeGeo();

  // marching

  template<class fieldtype,int CellSizeP,int Iso0>
  void March(const fieldtype *pot,sF32 scale,sVector31 tpos,sF32 iso,sInt thread);

  void March(sInt grid,const MCPotField *pot,sF32 scale,sVector31 tpos,sInt thread);
  void March_0_1(const MCPotField *pot,sF32 scale,sVector31 tpos,sInt thread);
  void March_1_1(const MCPotField *pot,sF32 scale,sVector31 tpos,sInt thread);
  void March_2_1(const MCPotField *pot,sF32 scale,sVector31 tpos,sInt thread);
  void March_3_1(const MCPotField *pot,sF32 scale,sVector31 tpos,sInt thread);
  void March_4_1(const MCPotField *pot,sF32 scale,sVector31 tpos,sInt thread);
  void March_5_1(const MCPotField *pot,sF32 scale,sVector31 tpos,sInt thread);

  void MarchIso(sInt grid,const MCPotField *pot,sF32 scale,sVector31 tpos,sF32 iso,sInt thread);
  void March_0_0(const MCPotField *pot,sF32 scale,sVector31 tpos,sF32 iso,sInt thread);
  void March_1_0(const MCPotField *pot,sF32 scale,sVector31 tpos,sF32 iso,sInt thread);
  void March_2_0(const MCPotField *pot,sF32 scale,sVector31 tpos,sF32 iso,sInt thread);
  void March_3_0(const MCPotField *pot,sF32 scale,sVector31 tpos,sF32 iso,sInt thread);
  void March_4_0(const MCPotField *pot,sF32 scale,sVector31 tpos,sF32 iso,sInt thread);
  void March_5_0(const MCPotField *pot,sF32 scale,sVector31 tpos,sF32 iso,sInt thread);

  void MarchColor(sInt grid,const MCPotFieldColor *pot,sF32 scale,sVector31 tpos,sInt thread);
  void March_0_1(const MCPotFieldColor *pot,sF32 scale,sVector31 tpos,sInt thread);
  void March_1_1(const MCPotFieldColor *pot,sF32 scale,sVector31 tpos,sInt thread);
  void March_2_1(const MCPotFieldColor *pot,sF32 scale,sVector31 tpos,sInt thread);
  void March_3_1(const MCPotFieldColor *pot,sF32 scale,sVector31 tpos,sInt thread);
  void March_4_1(const MCPotFieldColor *pot,sF32 scale,sVector31 tpos,sInt thread);
  void March_5_1(const MCPotFieldColor *pot,sF32 scale,sVector31 tpos,sInt thread);
};

/****************************************************************************/

#endif // FILE_EXTRA_MCUBES_HPP

