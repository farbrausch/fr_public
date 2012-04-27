/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#define sPEDANTIC_WARN 1

#include "rasterizer.hpp"
#include "util/image.hpp"
#include "util/algorithms.hpp"

/****************************************************************************/

static const sInt OVERSAMPLE_YF = 4; // more is probably overkill
static const sInt OVERSAMPLE_Y = 1<<OVERSAMPLE_YF;

static const sInt FIXSHIFT = 12;
static const sInt FIXSCALE = 1 << FIXSHIFT;
static const sInt SCALE_X = 1 << FIXSHIFT;
static const sInt SCALE_Y = (1 << FIXSHIFT) * OVERSAMPLE_Y;

/****************************************************************************/

static sInt sCeilFix(sInt x)
{
  return (x + (1 << FIXSHIFT)-1) >> FIXSHIFT;
}

sINLINE sInt sMulShift12(sInt var_a,sInt var_b)
{
  return sInt((sS64(var_a) * var_b) >> 12);
}

sINLINE sInt sDivShift12(sInt var_a,sInt var_b)
{
  return sInt((sS64(var_a) << 12) / var_b);
}

sINLINE sU32 FadePixel(sU32 a,sU32 b,sInt fade) // fade in 0..256
{
  // ye olde double blend tricke.
  sU32 arb = (a >> 0) & 0xff00ff;
  sU32 aag = (a >> 8) & 0xff00ff;
  sU32 brb = (b >> 0) & 0xff00ff;
  sU32 bag = (b >> 8) & 0xff00ff;

  sU32 drb = ((brb - arb) * fade) >> 8;
  sU32 dag = ((bag - aag) * fade) >> 8;
  sU32 rb  = (arb + drb) & 0xff00ff;
  sU32 ag  = ((aag + dag) << 8) & 0xff00ff00;

  return rb | ag;
}

/****************************************************************************/

sVectorRasterizer::sVectorRasterizer(sImage *target)
  : Target(target)
{
  Head = &EHead;
  Head->x = -0x80000000LL;
  Head->dx = 0;

  TargetHeight = (target->SizeY * OVERSAMPLE_Y) << FIXSHIFT;
  MinY = TargetHeight >> FIXSHIFT;
  MaxY = 0;
  Pos.Init(0,0);
}

void sVectorRasterizer::MoveTo(const sVector2 &pos)
{
  Pos = pos;
}

void sVectorRasterizer::Edge(const sVector2& a, const sVector2& b)
{
  sInt x1 = sInt(a.x * SCALE_X), y1 = sInt(a.y * SCALE_Y);
  sInt x2 = sInt(b.x * SCALE_X), y2 = sInt(b.y * SCALE_Y);
  sInt dir = 1;
  sInt dx;

  Pos = b;

  if(y1 > y2)
  {
    sSwap(x1, x2);
    sSwap(y1, y2);
    dir = -1;
  }

  sInt iy1 = sCeilFix(y1), iy2 = sCeilFix(y2);
  if(iy1 == iy2) // horizontal edge, skip
    return;

  if(y2 - y1 >= FIXSCALE)
    dx = sDivShift12(x2-x1, y2-y1);
  else
    dx = sMulShift12(x2-x1, (1<<28)/(y2-y1));

  // y clip
  if(y1 < 0)
  {
    x1 += sMulShift12(dx, 0 - y1);
    y1 = iy1 = 0;
  }
  
  if(y2 > TargetHeight)
  {
    x2 += sMulShift12(dx, TargetHeight - y2);
    y2 = TargetHeight;
    iy2 = TargetHeight >> FIXSHIFT;
  }

  if(iy1 >= iy2) // if degenerate after clip, just skip
    return;

  // build edge
  EdgeRec *e = Edges.AddMany(1);
  e->Prev = e->Next = 0;
  e->x = x1 + sMulShift12(dx, (iy1 << 12) - y1); // subpixel correction? of course.
  e->dx = dx;
  e->y1 = iy1;
  e->y2 = iy2;
  e->dir = dir;

  MinY = sMin(MinY, iy1);
  MaxY = sMax(MaxY, iy2);
}

void sVectorRasterizer::BezierEdge(const sVector2& a, const sVector2& b, const sVector2& c, const sVector2& d)
{
  if(sFAbs(a.x + c.x - 2.0f * b.x) + sFAbs(a.y + c.y - 2.0f * b.y)
    + sFAbs(b.x + d.x - 2.0f * c.x) + sFAbs(b.y + d.y - 2.0f * c.y) <= 1.0f) // close enough to a line (or just small)
    Edge(a, d);
  else
  {
    sVector2 ab = sAverage(a,b);
    sVector2 bc = sAverage(b,c);
    sVector2 cd = sAverage(c,d);

    sVector2 abc = sAverage(ab,bc);
    sVector2 bcd = sAverage(bc,cd);

    sVector2 abcd = sAverage(abc,bcd);

    BezierEdge(a, ab, abc, abcd);
    BezierEdge(abcd, bcd, cd, d);
  }
}

void sVectorRasterizer::RasterizeAll(sU32 color,sInt fillConvention)
{
  Head->Prev = Head->Next = Head;
  sIntroSort(sAll(Edges));

  EdgeRec *ei = Edges.GetData(), *ee = Edges.GetData()+Edges.GetCount();
  sInt windingMask = (fillConvention == sVRFC_EVENODD) ? 1 : -1;
  sInt endY = sAlign(MaxY,OVERSAMPLE_Y); // make sure we always resolve the last row
  sInt width = Target->SizeX;
  
  sInt coverSize = width * OVERSAMPLE_Y; 
  sU8* cover = new sU8[coverSize];
  sU8* coverEnd = cover + coverSize;
  sU8* cp = cover + (MinY & (OVERSAMPLE_Y - 1)) * width;
  sU32* op = Target->Data + (MinY / OVERSAMPLE_Y) * width;

  sSetMem(cover, 0, coverSize);

  for(sInt y=MinY;y<endY;y++)
  {
    EdgeRec *e, *en;

    // advance all x coordinates, remove "expired" edges, re-sort active edge list on the way
    for(e = Head->Next; e != Head; e = en)
    {
      en = e->Next;

      if(y < e->y2) // edge is still active
      {
        e->x += e->dx; // step
        
        while(e->x < e->Prev->x) // keep everything sorted
        {
          // move one step towards the beginning
          EdgeRec* ep = e->Prev;
          
          ep->Prev->Next = e;
          e->Next->Prev = ep;
          e->Prev = ep->Prev;
          ep->Next = e->Next;
          e->Next = ep;
          ep->Prev = e;
        }
      }
      else // deactivate edge: remove from active edge list
      {
        e->Prev->Next = e->Next;
        e->Next->Prev = e->Prev;
      }
    }

    // insert new edges if there are any
    e = Head;
    while(ei != ee && ei->y1 == y)
    {
      // search for insert position
      while(e != Head->Prev && e->Next->x <= ei->x)
        e = e->Next;

      // insert new edge behind e
      ei->Prev = e;
      ei->Next = e->Next;
      e->Next->Prev = ei;
      e->Next = ei;

      ++ei;
    }

    // go through new active edge list and generate spans on the way
    sInt winding = 0, lx;

    for(e = Head->Next; e != Head; e = e->Next)
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
            cp[ix0-1] = sU8(sMin(cp[ix0-1] + ((x1 - x0) >> 4), 255));
          else // at least one pixel thick
          {
            if(x0 & 0xfff)
              cp[ix0-1] = sU8(sMin(cp[ix0-1] + ((~x0 & 0xfff) >> 4), 255));

            if(x1 & 0xfff)
            {
              ix1--;
              cp[ix1] = sU8(sMin(cp[ix1] + ((x1 & 0xfff) >> 4), 255));
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

        op[x] = FadePixel(op[x], color, sum >> 8);
      }

      sSetMem(cover, 0, coverSize);
      cp = cover;
      op += width;
    }
  }

  delete[] cover;
  Edges.Clear();

  Pos.Init(0,0);
}

/****************************************************************************/

