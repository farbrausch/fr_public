// This file is distributed under a BSD license. See LICENSE.txt for details.

#pragma once

#include "main.hpp"
#include "_intmath.hpp"

/****************************************************************************/

class SoftImage
{
public:
  sInt SizeX;
  sInt SizeY;
  sU32 *Data;

  SoftImage(sInt xs,sInt ys) { SizeX=xs;SizeY=ys;Data=new sU32[xs*ys]; }
  ~SoftImage() { delete[] Data; }
};

/****************************************************************************/

struct SoftVertex
{
  sS16 x,y,z;
  sS16 matrix;
  sS16 u0,v0;
  sS16 u1,v1;
  union
  {
    sU32 col;
    sU8 c[4];
  };
};

struct SoftCluster
{
  sInt IndexStart;
  sInt IndexCount;
  sInt VertexStart;
  sInt VertexCount;
  sInt Flags;
  sInt TFlags[2];
  SoftImage *Texture[2];
};

class SoftMesh
{
public:
  SoftMesh();
  ~SoftMesh();

  SoftVertex *Vertices;
  SoftCluster *Clusters;
  sIMatrix34 *Matrices;
  sU16 *Indices;
  sInt VertexCount;
  sInt ClusterCount;
  sInt MatrixCount;
  sInt IndexCount;
};

SoftMesh *MakeSoftMesh(class GenMobMesh *mob);

/****************************************************************************/

struct SoftPixel
{
  sU16 Color;
  sU16 Depth;
};

struct tvertex;


enum SoftPixelMode
{
  PM_FLAT = 0,                    // whiteness
  PM_PLAIN,                       // plain mode
  PM_BLENDADD,                    // additive blending, zread
  PM_VERTEXCOLOR,                 // vertex color * texture
  PM_TEXADD,                      // texture A + texture B
  PM_TEXMUL,                      // texture A * texture B
};


class SoftEngine
{
  sInt ScreenX;                   // screensize (full pixel)
  sInt ScreenY;
  sU16 *FrontBuffer16;            // where to copy the final image
  sU32 *FrontBuffer32;            // where to copy the final image
  sU32 *ConvertTable;             // pixel format conversion (pc only)
  sInt Stride;
  SoftPixel *BackBuffer;          // color + z buffer
  
  sIMatrix34 View;                // world->view transformation
  sInt Zoom;                      // camera zoom (sFIXONE = 45°)

  sInt TriCount[4];               // performance counters
  sInt PixCount[4];

  void Middle(tvertex &xxx,const struct tvertex &vp0,const struct tvertex &vp1);
  void Begin(sInt ScreenX,sInt ScreenY,sU16 clearcolor);
  void DrawTriangleTess(const tvertex *vp0,const tvertex *vp1,const tvertex *vp2,SoftImage *img0,SoftImage *img1,sInt level=0);
  void DrawTriangle(const tvertex *vp0,const tvertex *vp1,const tvertex *vp2,SoftImage *img0,SoftImage *img1);

  sInt PixelMode;
  sInt PixelFlags;

public:
  SoftEngine();
  ~SoftEngine();
  void CalcConvertTable();
  void Begin(sU16 *dest,sInt ScreenX,sInt ScreenY,sU16 clearcolor=0);
  void Begin(sU32 *dest,sInt stride,sInt ScreenX,sInt ScreenY,sU32 clearcolor=0);
  void End();
  void EndDouble();
  void SetViewport(const sIMatrix34 &mat,sInt zoom);
  void Paint(SoftMesh *);
  void Print(sInt x,sInt y,sChar *text,sU16 color=0xffff);
  void DebugOut();

  SoftPixel *GetBackBuffer() { return BackBuffer; }

  sInt Tesselate;
};

/****************************************************************************/

void AddTexture(sInt xs,sInt ys,sU32 *data,sInt mode);
void OldSoftEngine(sInt ScreenX,sInt ScreenY,sU16 *Buffer,SoftImage *img);

/****************************************************************************/
