/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "genvector.hpp"
#include "util/algorithms.hpp"

using namespace Vectorizer;

static const sInt OVERSAMPLE_YF = 4; // more is probably overkill
static const sInt OVERSAMPLE_Y = 1<<OVERSAMPLE_YF;

/****************************************************************************/


__forceinline sInt sMulShift12(sInt var_a,sInt var_b)
{
  return (sS64(var_a) * var_b) >> 12;
}

__forceinline sInt sDivShift12(sInt var_a,sInt var_b)
{
  return (sS64(var_a) << 12) / var_b;
}

/****************************************************************************/

VectorRasterizer::VectorRasterizer(GenBitmap *target_)
  : target(target_)
{
  head = &ehead;
  head->x = -0x80000000LL;
  head->dx = 0;
  minY = target->YSize * OVERSAMPLE_Y;
  maxY = 0;
}

void VectorRasterizer::line(const sVector2& a, const sVector2& b)
{
  int x1 = int(a.x * 4096), y1 = int(a.y * 4096 * OVERSAMPLE_Y);
  int x2 = int(b.x * 4096), y2 = int(b.y * 4096 * OVERSAMPLE_Y);
  int dir = 1;
  int dx;

  if(y1 > y2)
  {
    sSwap(x1, x2);
    sSwap(y1, y2);
    dir = -1;
  }

  int iy1 = (y1 + 0xfff) >> 12, iy2 = (y2 + 0xfff) >> 12;
  if(iy1 == iy2) // horizontal edge, skip
    return;

  if(y2 - y1 >= 4096)
    dx = sDivShift12(x2-x1, y2-y1);
  else
    dx = sMulShift(x2-x1, (1<<28)/(y2-y1));

  // y clip
  if(y1 < 0)
  {
    x1 += sMulShift12(dx, 0 - y1);
    y1 = iy1 = 0;
  }
  
  if(y2 > OVERSAMPLE_Y * (target->YSize << 12))
  {
    x2 += sMulShift12(dx, OVERSAMPLE_Y * (target->YSize << 12) - y2);
    y2 = OVERSAMPLE_Y * (target->YSize << 12);
    iy2 = OVERSAMPLE_Y * target->YSize;
  }

  if(iy1 >= iy2) // if degenerate after clip, just skip
    return;

  // build edge
  Edge *e = edges.AddMany(1);
  e->prev = e->next = 0;
  e->x = x1 + sMulShift12(dx, (iy1 << 12) - y1); // subpixel correction? of course.
  e->dx = dx;
  e->y1 = iy1;
  e->y2 = iy2;
  e->dir = dir;

  minY = sMin(minY, iy1);
  maxY = sMax(maxY, iy2);
}

void VectorRasterizer::cubic(const sVector2& a, const sVector2& b, const sVector2& c, const sVector2& d)
{
  if(sFAbs(a.x + c.x - 2.0f * b.x) + sFAbs(a.y + c.y - 2.0f * b.y)
    + sFAbs(b.x + d.x - 2.0f * c.x) + sFAbs(b.y + d.y - 2.0f * c.y) <= 1.0f) // close enough to a line (or just small)
    line(a, d);
  else
  {
    sVector2 ab = sAverage(a,b);
    sVector2 bc = sAverage(b,c);
    sVector2 cd = sAverage(c,d);

    sVector2 abc = sAverage(ab,bc);
    sVector2 bcd = sAverage(bc,cd);

    sVector2 abcd = sAverage(abc,bcd);

    cubic(a, ab, abc, abcd);
    cubic(abcd, bcd, cd, d);
  }
}

void VectorRasterizer::rasterizeAll(sU32 color, sBool evenOddRule)
{
  head->prev = head->next = head;
  sIntroSort(sAll(edges));

  Edge *ei = edges.GetData(), *ee = edges.GetData()+edges.GetCount();
  sInt windingMask = evenOddRule ? 1 : -1;
  sInt endY = sAlign(maxY,OVERSAMPLE_Y); // make sure we always resolve the last row
  sInt width = target->XSize;
  
  sInt coverSize = width * OVERSAMPLE_Y; 
  sU8* cover = new sU8[coverSize];
  sU8* coverEnd = cover + coverSize;
  sU8* cp = cover + (minY & (OVERSAMPLE_Y - 1)) * width;
  sU64* ip = target->Data + (minY / OVERSAMPLE_Y) * width;
  sU64 col64 = GenBitmap::GetColor64(color);
  sInt fade = sInt(col64>>48) * 256 / 0x7fff;

  sSetMem(cover, 0, coverSize);

  for(sInt y=minY;y<endY;y++)
  {
    Edge *e, *en;

    // advance all x coordinates, remove "expired" edges, re-sort active edge list on the way
    for(e = head->next; e != head; e = en)
    {
      en = e->next;

      if(y < e->y2) // edge is still active
      {
        e->x += e->dx; // step
        
        while(e->x < e->prev->x) // keep everything sorted
        {
          // move one step towards the beginning
          Edge* ep = e->prev;
          
          ep->prev->next = e;
          e->next->prev = ep;
          e->prev = ep->prev;
          ep->next = e->next;
          e->next = ep;
          ep->prev = e;
        }
      }
      else // deactivate edge: remove from active edge list
      {
        e->prev->next = e->next;
        e->next->prev = e->prev;
      }
    }

    // insert new edges if there are any
    e = head;
    while(ei != ee && ei->y1 == y)
    {
      // search for insert position
      while(e != head->prev && e->next->x <= ei->x)
        e = e->next;

      // insert new edge behind e
      ei->prev = e;
      ei->next = e->next;
      e->next->prev = ei;
      e->next = ei;

      ++ei;
    }

    // go through new active edge list and generate spans on the way
    sInt winding = 0, lx;

    for(e = head->next; e != head; e = e->next)
    {
      if(winding & windingMask)
      {
        // clip
        sInt x0 = sMax(lx, 0);
        sInt x1 = sMin(e->x, width << 12);

        if(x1 > x0)
        {
          // generate span
          sInt ix0 = (x0 + 0xfff) >> 12;
          sInt ix1 = (x1 + 0xfff) >> 12;

          if(ix0 == ix1) // very thin part
            cp[ix0-1] = sMin(cp[ix0-1] + ((x1 - x0) >> 4), 255);
          else // at least one pixel thick
          {
            if(x0 & 0xfff)
              cp[ix0-1] = sMin(cp[ix0-1] + ((~x0 & 0xfff) >> 4), 255);

            if(x1 & 0xfff)
            {
              ix1--;
              cp[ix1] = sMin(cp[ix1] + ((x1 & 0xfff) >> 4), 255);
            }

            while(ix0 < ix1)
              cp[ix0++] = 255;
          }
        }
      }

      winding += e->dir;
      lx = e->x;
    }

    // advance and resolve
    cp += width;
    if(cp == coverEnd) // row complete
    {
      for(sInt x=0;x<width;x++)
      {
        sU32 sum = 0;
        for(sInt s=0;s<OVERSAMPLE_Y;s++)
          sum += cover[x+s*width];

        sum <<= 8 - OVERSAMPLE_YF;
        sum += (sum >> (8 + OVERSAMPLE_YF));

        GenBitmap::Fade64(ip[x], ip[x], col64, sum*fade/256);
      }

      sSetMem(cover, 0, coverSize);
      cp = cover;
      ip += width;
    }
  }

  delete[] cover;
  edges.Clear();
}
