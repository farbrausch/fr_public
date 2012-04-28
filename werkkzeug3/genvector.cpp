// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "genvector.hpp"
#include "genbitmap.hpp"
#include "_start.hpp"
#include <stdlib.h>

static const sInt OVERSAMPLE_YF = 4; // more is probably overkill
static const sInt OVERSAMPLE_Y = 1<<OVERSAMPLE_YF;

/****************************************************************************/

struct Vec2
{
	sF32 x,y;

	Vec2()
	{
	}

	Vec2(sF32 _x,sF32 _y)
		: x(_x), y(_y)
	{
	}

	void mid(const Vec2 &a,const Vec2 &b)
	{
		x = 0.5f * (a.x + b.x);
		y = 0.5f * (a.y + b.y);
	}
};

__forceinline sInt sMulShift12(sInt var_a,sInt var_b)
{
  __asm
  {
    mov eax, var_a
    imul var_b
    shrd eax, edx, 12
  }
}

__forceinline sInt sDivShift12(sInt var_a,sInt var_b)
{
  __asm
  {
    mov eax, var_a
    mov edx, eax
    shl eax, 12
    sar edx, 20
    idiv var_b
  }
}

/****************************************************************************/

enum PathCommand
{
	PATH_CUBIC=0, // most common one
	PATH_LINE,
	PATH_MOVE,
  PATH_END
};

static const sU8 pathCommandSize[] = { // match commands in PathCommand
  3,
  1,
  1,
  0
};

struct Path
{
  sArray2<sU8> cmds;
  sArray2<Vec2> points;

	const sChar* parseError;
	sInt parseErrorPos;

	sBool parse(const sChar *str);

  sU8 *exportData(sInt &size) const;
  void importData(const sU8 *data);

private:
	const sChar* text;
	const sChar* cp;

	void skipWhitespace();
	sBool expect(sChar ch);
	sBool floatv(sF32& v);
	sBool vec2d(Vec2& v);
	sBool command(sInt cmd, sInt params);
	sBool error(sInt backtrack, const sChar* msg);
};

/****************************************************************************/

sBool Path::parse(const sChar *str)
{
	Vec2 first;

	text = cp = str;
	parseError = 0;
	parseErrorPos = 0;

	points.Clear();
	cmds.Clear();

	skipWhitespace();
	while(*cp)
	{
		sChar cmd = *cp++;
		skipWhitespace();

		sInt pcmd = -1, pparm;

		switch(cmd)
		{
		case 'M': pcmd = PATH_MOVE; pparm = 1; break;	// moveto
		case 'L': pcmd = PATH_LINE; pparm = 1; break;	// lineto
		case 'C': pcmd = PATH_CUBIC; pparm = 3; break;	// cubic
		case 'z':
		case 'Z': pcmd = PATH_LINE; pparm = 0; break;	// closepath
		default:  return error(1, "unknown command");
		}

		if(pcmd != -1 && !command(pcmd, pparm))
			return false;

		if(cmd == 'M')
      first = points[points.Count-1];
		else if(cmd == 'z' || cmd == 'Z') // close
      *points.Add() = first;

		skipWhitespace();
	}

	return true;
}

static sU32 deltaCode(sInt val, sInt& last)
{
  sInt delta = val-last;
  last = val;
  return sAbs(delta) * 2 - (delta < 0);
}

static sInt deltaDecode(sU32 val, sInt& last)
{
  sInt v = (val >> 1) ^ -sInt(val & 1);
  return last += v;
}

static void scatterStore(sU8* dst, sInt stride, sU32 v)
{
  dst[0*stride] = (v >>  0) & 0xff;
  dst[1*stride] = (v >>  8) & 0xff;
  dst[2*stride] = (v >> 16) & 0xff;
  dst[3*stride] = (v >> 24) & 0xff;
}

static sU32 gatherRead(const sU8* src, sInt stride)
{
  return src[0*stride] + (src[1*stride] << 8)
    + (src[2*stride] << 16) + (src[3*stride] << 24);
}

sU8 *Path::exportData(sInt &size) const
{
  // commands
  sArray2<sU8> data = cmds;
  *data.Add() = PATH_END;

  // points
  sInt pts = points.Count;
  data.Resize(data.Count + 4 * 2 * pts);
  sU8* ptd = data.Array + cmds.Count + 1;
  sInt lx = 0, ly = 0;

  for(sInt i=0;i<pts;i++)
  {
    scatterStore(ptd++, 2*pts, deltaCode(points[i].x*8, lx));
    scatterStore(ptd++, 2*pts, deltaCode(points[i].y*8, ly));
  }

  // detach data from array
  sU8* ptr = data.Array;
  size = data.Count;
  data.Count = data.Alloc = 0;
  data.Array = 0;
  return ptr;
}

void Path::importData(const sU8 *data)
{
  // commands
  sInt pts = 0;
  const sU8* ptr = data;
  while(*ptr != PATH_END)
    pts += pathCommandSize[*ptr++];

  cmds.Resize(ptr-data);
  sCopyMem(cmds.Array, data, ptr-data);
  ptr++;

  // points
  points.Resize(pts);
  sInt lx = 0, ly = 0;
  sF32 scalef = sFPow(2.0f, GenBitmapTextureSizeOffset - 3.0f);

  for(sInt i=0;i<pts;i++)
  {
    points[i].x = deltaDecode(gatherRead(ptr++, 2*pts), lx) * scalef;
    points[i].y = deltaDecode(gatherRead(ptr++, 2*pts), ly) * scalef;
  }
}

void Path::skipWhitespace()
{
	const sChar* rcp = cp;
	while(*rcp == ' ' || *rcp == '\t' || *rcp == '\r' || *rcp == '\n')
		rcp++;

	cp = rcp;
}

sBool Path::expect(sChar ch)
{
	return (*cp++ != ch) ? error(1, "unexpected character") : sTRUE;
}

sBool Path::floatv(sF32 &v)
{
	const sChar* oldcp = cp;
	v = strtod(cp, (char**) &cp);
	return (cp == oldcp) ? error(0, "float expected") : sTRUE;
}

sBool Path::vec2d(Vec2& v)
{
	if(!floatv(v.x) || !expect(',') || !floatv(v.y))
		return sFALSE;

	skipWhitespace();
	return sTRUE;
}

sBool Path::command(sInt cmd, sInt params)
{
  *cmds.Add() = cmd;
	while(params--)
	{
		Vec2 v;
		if(!vec2d(v))
			return sFALSE;
		else
      *points.Add() = v;
	}

	return sTRUE;
}

sBool Path::error(sInt backtrack, const sChar *msg)
{
	points.Clear();
	cmds.Clear();

	parseError = msg;
	parseErrorPos = cp - text - backtrack;
	return sFALSE;
}

/****************************************************************************/

class VectorRasterizer
{
public:
	VectorRasterizer(GenBitmap *target);

	void renderPath(const Path& path, sU32 color, sBool evenOddRule);

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
	sArray2<Edge> edges;
	sInt minY, maxY;

  GenBitmap *target;

	void line(const Vec2& a, const Vec2& b);
	void cubic(const Vec2& a, const Vec2& b, const Vec2& c, const Vec2& d);

	void submitPath(const Path& path);
  void edgeSiftDown(sInt n,sInt k);
  void sortEdges();
	void rasterizeAll(sU32 color, sBool evenOddRule);
};

/****************************************************************************/

VectorRasterizer::VectorRasterizer(GenBitmap *target_)
	: target(target_)
{
	head = &ehead;
	head->x = -0x7fffffff - 1; // VC warns for literal -0x80000000
	head->dx = 0;
}

void VectorRasterizer::renderPath(const Path& path, sU32 color, sBool evenOddRule)
{
	minY = target->YSize * OVERSAMPLE_Y;
	maxY = 0;

	submitPath(path);
	rasterizeAll(color, evenOddRule);

	edges.Clear();
}

void VectorRasterizer::line(const Vec2& a, const Vec2& b)
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
	Edge *e = edges.Add();
	e->prev = e->next = 0;
	e->x = x1 + sMulShift12(dx, (iy1 << 12) - y1); // subpixel correction? of course.
	e->dx = dx;
	e->y1 = iy1;
	e->y2 = iy2;
	e->dir = dir;

	minY = sMin(minY, iy1);
	maxY = sMax(maxY, iy2);
}

void VectorRasterizer::cubic(const Vec2& a, const Vec2& b, const Vec2& c, const Vec2& d)
{
	if(sFAbs(a.x + c.x - 2.0f * b.x) + sFAbs(a.y + c.y - 2.0f * b.y)
		+ sFAbs(b.x + d.x - 2.0f * c.x) + sFAbs(b.y + d.y - 2.0f * c.y) <= 1.0f)
		line(a, d);
	else
	{
		Vec2 ab,bc,cd,abc,bcd,abcd;

		ab.mid(a,b);
		bc.mid(b,c);
		cd.mid(c,d);
		abc.mid(ab,bc);
		bcd.mid(bc,cd);
		abcd.mid(abc,bcd);

		cubic(a, ab, abc, abcd);
		cubic(abcd, bcd, cd, d);
	}
}

void VectorRasterizer::submitPath(const Path& path)
{
	Vec2 cur(0, 0);
	int cmdi = 0, pti = 0;

	while(cmdi < path.cmds.Count)
	{
    sInt cmd = path.cmds[cmdi++];

		switch(cmd)
		{
		case PATH_LINE:
      line(cur, path.points[pti]);
      break;

		case PATH_CUBIC:
			cubic(cur, path.points[pti+0], path.points[pti+1], path.points[pti+2]);
			break;
		}

    pti += pathCommandSize[cmd];
		cur = path.points[pti-1];
	}
}

void VectorRasterizer::edgeSiftDown(sInt n,sInt k)
{
  Edge v = edges[k];

  while(k < (n>>1))
  {
    sInt j = k*2+1;
    if(j+1 < n && edges[j] < edges[j+1])
      j++;

    if(!(v < edges[j]))
      break;

    edges[k] = edges[j];
    k = j;
  }

  edges[k] = v;
}

void VectorRasterizer::sortEdges()
{
  // heapsort.
  sInt n = edges.Count;
  for(sInt k=n/2-1;k>=0;k--)
    edgeSiftDown(n,k);

  while(--n > 0)
  {
    sSwap(edges[0],edges[n]);
    edgeSiftDown(n,0);
  }
}

void VectorRasterizer::rasterizeAll(sU32 color, sBool evenOddRule)
{
	head->prev = head->next = head;
  sortEdges();

  Edge *ei = &edges[0], *ee = &edges[edges.Count];
	sInt windingMask = evenOddRule ? 1 : -1;
	sInt endY = (maxY + OVERSAMPLE_Y - 1) & -OVERSAMPLE_Y;
  sInt width = target->XSize;
	
	sU8* cover = new sU8[width * OVERSAMPLE_Y];
	sU8* cp = cover + (minY & (OVERSAMPLE_Y - 1)) * width;
  sU64* ip = target->Data + (minY / OVERSAMPLE_Y) * width;
  sU64 col64 = GetColor64(color);

	sSetMem(cover, 0, width * OVERSAMPLE_Y);

	for(sInt y=minY;y<endY;y++)
	{
		Edge *e, *en;

		// advance all x coordinates, remove "expired" edges, re-sort active edge list on the way
		for(e = head->next; e != head; e = en)
		{
			en = e->next;

			if(y < e->y2) // edge is still active
			{
				e->x += e->dx;
				
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
			else // deactivate edge
			{
				e->prev->next = e->next;
				e->next->prev = e->prev;
			}
		}

		// insert new edges if there are any
		e = head;
		while(ei != ee && ei->y1 == y)
		{
			// search insert position
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
		if((y & (OVERSAMPLE_Y - 1)) == OVERSAMPLE_Y - 1) // row complete
		{
			for(sInt x=0;x<width;x++)
			{
				sU32 sum = 0;
				for(sInt s=0;s<OVERSAMPLE_Y;s++)
					sum += cover[x+s*width];

        sum <<= 8 - OVERSAMPLE_YF;
        sum += (sum >> (8 + OVERSAMPLE_YF));

        Fade64(ip[x], ip[x], col64, sum);
			}

			sSetMem(cover, 0, width * OVERSAMPLE_Y);
			cp = cover;
			ip += width;
		}
	}

	delete[] cover;
}

/****************************************************************************/

static sBool CheckBitmap(GenBitmap *&bm)
{
  GenBitmap *oldbm = bm;
  
  if(bm==0)
    return 1;
  if(bm->ClassId!=KC_BITMAP)
    return 1;
  if(bm->Data==0)
    return 1;
  if(bm->RefCount>1) 
  {
    bm = (GenBitmap *)oldbm->Copy();
    oldbm->Release();
  }
  return 0;
}

GenBitmap * __stdcall Bitmap_VectorPath(KOp *op,GenBitmap *bm,sU32 color,sInt flags,sChar *filename)
{
  const sU8* data;

  data = op->GetBlobData();
#if !sPLAYER
  if(!data)
  {
    sChar* pathText = sSystem->LoadText(filename);
    if(!pathText)
    {
      sDPrintF("couldn't open input file!\n");
      return 0;
    }

    Path path;
    sBool result = path.parse(pathText);
    delete[] pathText;

    if(!result)
    {
      sDPrintF("error while processing '%s': %s at byte %d.\n", filename,
        path.parseError, path.parseErrorPos);
      return 0;
    }

    // export
    sInt size;
    data = path.exportData(size);
    op->SetBlob(data, size);
  }
#endif

  if(!data || CheckBitmap(bm))
    return 0;

  // read path
  Path path;
  path.importData(data);

  // render!
  VectorRasterizer raster(bm);
  raster.renderPath(path, color, flags & 1);

  return bm;
}
