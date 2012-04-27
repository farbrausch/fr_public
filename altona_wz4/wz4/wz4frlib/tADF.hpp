/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2008 Bastian Zuehlke, all rights reserved                      ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef HEADER_TRON_ADF
#define HEADER_TRON_ADF

/****************************************************************************/

#ifndef __GNUC__
#pragma once
#endif

/****************************************************************************/

#include <base/types.hpp>
#include <base/types2.hpp>
#include <base/math.hpp>
#include "util/taskscheduler.hpp"
#include "wz4frlib/wz4_bsp.hpp"

/****************************************************************************/

struct tAABBoxOctreeVertex
{      
  sVector31 v;
  sVector30 n;
};

struct tAABBoxOctreeEdgeKey
{      
  unsigned int i1;
  unsigned int i2;

  inline sU32 Hash() const
  {
    return i1+i2;
  }

  sBool operator==(const tAABBoxOctreeEdgeKey &v) const
  {
    return (i1 == v.i1) && (i2 == v.i2);
  }
};


struct tAABBoxOctreeEdge
{      
  sVector30             n;
  tAABBoxOctreeEdgeKey  key;
  unsigned int          ep;
};


struct tAABBoxOctreeChild
{      
  sAABBox                 aabb;
  tAABBoxOctreeChild     *child[8];      
  sArray < sInt >         ta;    //List IDs to Triangle List
  bool                    leaf;
};

struct tAABBoxOctreeTri
{ 
  unsigned int i1;  //Index to Vertex
  unsigned int i2;
  unsigned int i3;
  unsigned int e1;  //Index to Edges
  unsigned int e2;
  unsigned int e3;
  unsigned int m;
  sVector30    n;  //normal vector
  sAABBox      aabb;
};


class sEdgeHash : public sHashTable < tAABBoxOctreeEdgeKey , tAABBoxOctreeEdge >
{
};

class tAABBoxOctree
{
  public:
    sArray < tAABBoxOctreeVertex >      vertices;
    sArray < tAABBoxOctreeTri >         tris;
    sStaticArray < tAABBoxOctreeEdge >  edges;
    sAABBox                             box;
    Wz4BSP                              bsp;

    tAABBoxOctree(int _depth);
    ~tAABBoxOctree();

    Wz4BSPError FromMesh(Wz4Mesh *mesh, sF32 planeThickness, sBool ForceCubeSampling, sBool UserBox, const sVector31 &BoxPos, const sVector30 &BoxDimH, sBool BruteForce, sF32 GuardBand);

    void AddVertex(const sVector31 &_v, const sVector30 &_n);
    void AddTriangle(unsigned int _i1, unsigned int _i2, unsigned int _i3, const sVector30 &_n);

    void FinishVertices(sF32 _guardband, sBool _cube, sBool UserBox, const sVector31 &BoxPos, const sVector30 &BoxDimH);    
    sF32 GetClosestDistance(sVector31 &_p, unsigned int *_id=0, sBool _bruteforce=sFALSE, sBool _nobsp=sFALSE);
    void NormVertex();
    void Finalize();
    inline sInt GetDepth(){return maxdepth;}

    tAABBoxOctreeChild *InitChild(int _depth, sAABBox _b);
    void Add(tAABBoxOctreeChild * _c, sInt _id);
    sF32 GetDistanceToTriangleSq(sInt _tri, sVector31 &_p, sBool &_isneg);         

    void GetClosestDistance(tAABBoxOctreeChild *_c, sVector31 &_p, sF32 &_d, sInt &_id, unsigned int _m, sBool &_isneg );
    unsigned int FindAddEdge(unsigned int _i1, unsigned int _i2);
    tAABBoxOctreeChild *Kill(tAABBoxOctreeChild *_c);
    tAABBoxOctreeChild *Free(tAABBoxOctreeChild *_c);
    void CalcBox(sF32 _guardband, sBool _cube, sBool UserBox, const sVector31 &BoxPos, const sVector30 &BoxDimH);
    
    sEdgeHash edgehash;
   
    int maxdepth;
    tAABBoxOctreeChild *head;
    unsigned int m;
};


// Simple Signed Distance Field

class tSDF
{ 
  public:
    sInt    DimX;
    sInt    DimY;
    sInt    DimZ;
    sInt    DimXY;
    sF32    STBX;
    sF32    STBY;
    sF32    STBZ;
    sF32    PStepX;  
    sF32    PStepY;
    sF32    PStepZ;
    sAABBox Box;
    sAABBox InBox;
    sF32    *SDF;
    sF32    GuardBand;
    


  
   tSDF();
   virtual ~tSDF();

   virtual void Init(sChar *fname);                   //Read from File  
   virtual void Init(tAABBoxOctree *oct, sInt depth, sBool bruteforce, sF32 guardband=0.0f); //Create from AABoxOctree
   virtual void Init(sF32 *distancefield, sAABBox &box, sInt dimx, sInt dimy, sInt dimz);
   void WriteToFile(sChar *fname);

   sBool IsInBox(const sVector31 &pos); //false position not in Distance field


   inline sF32 GetDistance(const sVector31 &pos)
   {
     //  sVERIFY(SDF);
     sF32 d = 0;
     sVector31 p = pos;

     //Handle outside
     if (!Box.HitPoint(pos))
     {    
       d = Box.DistanceTo(pos) + GuardBand; 
       //p.x = sClamp(p.x, Box.Min.x, InBox.Max.x);
       //p.y = sClamp(p.y, Box.Min.y, InBox.Max.y);
       //p.z = sClamp(p.z, Box.Min.z, InBox.Max.z);  
       return d;
       //sVERIFY(0);
     }

     //Scale position into 3D grid
     float fx = (p.x - Box.Min.x) * STBX;
     float fy = (p.y - Box.Min.y) * STBY;
     float fz = (p.z - Box.Min.z) * STBZ;

     int x1 = (int)fx;
     int y1 = (int)fy;
     int z1 = (int)fz;

     int x2 = x1+1;
     int y2 = y1+1;
     int z2 = z1+1;

     sVERIFY(x1>=0 && x1<=(DimX-1));
     sVERIFY(y1>=0 && y1<=(DimY-1));
     sVERIFY(z1>=0 && z1<=(DimZ-1));
 
     y1 *= DimX;
     y2 *= DimX;

     z1 *= DimXY;
     z2 *= DimXY;

     sF32 d1 = SDF[z1+y1+x1];
     sF32 d2 = SDF[z1+y1+x2];
     sF32 d3 = SDF[z1+y2+x1];
     sF32 d4 = SDF[z1+y2+x2];

     sF32 d5 = SDF[z2+y1+x1];
     sF32 d6 = SDF[z2+y1+x2];
     sF32 d7 = SDF[z2+y2+x1];
     sF32 d8 = SDF[z2+y2+x2];

     fx = sFrac(fx);
     fy = sFrac(fy);
     fz = sFrac(fz);

     //Lerp
     d1 = (d2 - d1 ) * fx + d1;
     d3 = (d4 - d3 ) * fx + d3;
     d5 = (d6 - d5 ) * fx + d5;
     d7 = (d8 - d7 ) * fx + d7;

     d1 = (d3 - d1 ) * fy + d1;
     d5 = (d7 - d5 ) * fy + d5;

     d += (d5 - d1 ) * fz + d1;

     return d;
   }

   inline void GetNormal(const sVector31 &p, sVector30 &n)
   {
     sVector31 np[6];

     for (int i=0;i<6;i++)
     {
       np[i] = p;
     }

     np[0].x -= PStepX;
     np[1].x += PStepX;
     np[2].y -= PStepY;
     np[3].y += PStepY;
     np[4].z -= PStepZ;
     np[5].z += PStepZ;

     sF32 nd[6];
     for (int i=0;i<6;i++)
     {
       nd[i] = GetDistance(np[i]);
     }

     n.x = nd[0] - nd[1];
     n.y = nd[2] - nd[3];
     n.z = nd[4] - nd[5];
     n.Unit();          
   }


   inline sF32 *GetDistanceField()
   {
     return SDF;
   }

   inline void GetDim(sInt &dimx, sInt &dimy, sInt &dimz)
   {
     dimx = DimX;
     dimy = DimY;
     dimz = DimZ;
   }

   inline void GetBox(sAABBox &box)
   {
     box = Box;
   }
};


/****************************************************************************/

//HEADER_TRON_ADF
#endif 

