#ifndef __GENVECTOR_HPP__
#define __GENVECTOR_HPP__

#include "_types.hpp"

/****************************************************************************/

class GenBitmap;
struct KOp;

GenBitmap * __stdcall Bitmap_VectorPath(KOp *op,GenBitmap *bm,sU32 color,sInt flags,sChar *filename);

/****************************************************************************/

#endif