/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_UTIL_RASTERIZER_HPP
#define FILE_UTIL_RASTERIZER_HPP

#include "base/types.hpp"
#include "base/types2.hpp"
#include "base/math.hpp"

/****************************************************************************/

class sImage;

enum sVectorRasterizer_FillConvention
{
  sVRFC_EVENODD          = 0,  // even-odd (xor) rule
  sVRFC_NONZERO_WINDING  = 1,  // nonzero winding rule
};

// Simple vector rasterizer. Renders nicely antialiased outlines of polygons,
// efficiently.
class sVectorRasterizer
{
public:
  sVectorRasterizer(sImage *target);

  // Move cursor to specific position
  void MoveTo(const sVector2 &pos);

  // Add an edge to be rasterized
  void Edge(const sVector2 &a,const sVector2 &b);

  // Draw an edge from the current cursor position to b
  void EdgeTo(const sVector2 &b) { Edge(Pos,b); }

  // Add a cubic bezier edge to be rasterized
  void BezierEdge(const sVector2 &a,const sVector2 &b,const sVector2 &c,const sVector2 &d);

  // Rasterize everything currently specified with the given fill convention and color.
  // This also clears all active draw commands.
  void RasterizeAll(sU32 color, sInt fillConvention); 

private:
  struct EdgeRec
  {
    EdgeRec *Prev,*Next;    // Linked list
    sInt x,dx;              // fixed point
    sInt y1,y2;             // y1<y2
    sInt dir;               // original direction (for winding calc)

    sBool operator <(const EdgeRec &b) const
    {
      return y1 < b.y1 || y1 == b.y1 && x < b.x;
    }
  };

  EdgeRec EHead,*Head;
  sArray<EdgeRec> Edges;
  sInt MinY,MaxY;
  sVector2 Pos;

  sImage *Target;
  sInt TargetHeight; // in scaled, fixed-point units
};

/****************************************************************************/

#endif // FILE_UTIL_RASTERIZER_HPP

