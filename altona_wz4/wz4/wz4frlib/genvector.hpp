/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef __GENVECTOR_HPP__
#define __GENVECTOR_HPP__

#include "base/types2.hpp"
#include "base/math.hpp"
#include "wz3_bitmap_code.hpp"

namespace Vectorizer
{

/****************************************************************************/

class VectorRasterizer
{
public:
	VectorRasterizer(GenBitmap *target);
	void line(const sVector2& a, const sVector2& b);
	void cubic(const sVector2& a, const sVector2& b, const sVector2& c, const sVector2& d);
	void rasterizeAll(sU32 color, sBool evenOddRule);

//	void renderPath(const Path& path, sU32 color, sBool evenOddRule);

private:
	struct Edge
	{
		Edge *prev, *next;	// linked list
		sInt x,dx;			    // 16.16 fixed point
		sInt y1,y2;			    // actual pixels (y1<y2)
		sInt dir;

		bool operator < (const Edge& b) const
		{
			return y1 < b.y1 || y1 == b.y1 && x < b.x;
		}
	};

	Edge ehead, *head;
	sArray<Edge> edges;
	sInt minY, maxY;

  GenBitmap *target;
};

/****************************************************************************/
}

#endif