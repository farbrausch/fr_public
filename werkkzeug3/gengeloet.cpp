// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_types.hpp"
#include "kdoc.hpp"
#include "genbitmap.hpp"
#include "genminmesh.hpp"
#include "gengeloet.hpp"
#include "geneffect.hpp"
#include "genmaterial.hpp"

/****************************************************************************/

// Abstract graph interface for the PathFinder class.
class AbstractGraph
{
public:
  // number of vertices in the graph (vertices are numbered 0..NumVerts()-1)
  virtual sInt NumVerts() const = 0;

  // heuristic estimate of cost of getting from v1 to v2
  virtual sF32 Metric(sInt v1,sInt v2) const = 0;

  // cost of getting from vCur to vNext, coming from vPrev
  // only defined if suitable edges exist. vPrev may be -1.
  virtual sInt Cost(sInt vCur,sInt vNext,sInt vPrev) const = 0;

  // enumerate neighbors of node v
  virtual void EnumNeighbors(sInt v,sArray<sInt> &neighbors) const = 0;

  // set before pathfinding to set the target (can be ignored)
  virtual void SetTarget(sInt v)
  {
  }

  // get occupied state
  virtual sBool IsOccupied(sInt v) const = 0;

  // add a path
  virtual void AddPath(const sArray<sInt> &path) = 0;
};

// A*-based shortest-path finder (on general graphs, possibly implicit).
// This is an abstract class.

class PathFinder
{
  struct Node
  {
    Node *Dad;      // parent node
    sInt V;         // number of current vertex in graph
    sInt RefCount;  // reference count
    sInt Cost;      // cost of current path (costs are always integer)
    sF32 Priority;  // priority in queue
  };

  // ---- Graph and visit structure
  AbstractGraph *Graph;
  sU8 *Visited;
  sU8 VisitCounter;
  sInt MaxHeapSize;
  Node **Heap;

  // ---- Node management
  sGrowableMemStack Mem;
  Node *FreeList;

  void ClearNodes();
  Node *NewNode(Node *parent,sInt v,sInt target);
  void ReleaseNode(Node *node);

public:
  PathFinder();
  ~PathFinder();

  void SetGraph(AbstractGraph *g,sInt heapSize=-1);

  // Find a path from a to b
  bool FindPath(sInt a,sInt b,sArray<sInt> &path,sInt bound=0x7fffffff);
};

// ---- Node management

void PathFinder::ClearNodes()
{
  Mem.Flush();
  FreeList = 0;
}

PathFinder::Node *PathFinder::NewNode(Node *parent,sInt v,sInt target)
{
  Node *node;

  // allocate a node
  if(FreeList)
  {
    node = FreeList;
    FreeList = FreeList->Dad;
  }
  else
    node = Mem.Alloc<Node>();

  // set it up
  node->RefCount = 1;
  node->Dad = parent;
  node->V = v;
  
  if(!parent)
    node->Cost = 0;
  else
  {
    Node *dd = parent->Dad;
    node->Cost = parent->Cost + Graph->Cost(parent->V,node->V,dd ? dd->V : -1);
    parent->RefCount++;
  }

  node->Priority = node->Cost + Graph->Metric(v,target);
  return node;
}

void PathFinder::ReleaseNode(Node *node)
{
  if(--node->RefCount == 0)
  {
    if(node->Dad)
      ReleaseNode(node->Dad);

    node->Dad = FreeList;
    FreeList = node;
  }
}

// ---- Public interface

PathFinder::PathFinder()
{
  Graph = 0;
  Visited = 0;
  VisitCounter = 0;
  Mem.Init(64*1024);
  FreeList = 0;
  MaxHeapSize = 0;
  Heap = 0;
}

PathFinder::~PathFinder()
{
  delete[] Visited;
  delete[] Heap;
  Mem.Exit();
}

void PathFinder::SetGraph(AbstractGraph *g,sInt heapSize)
{
  Graph = g;
  delete[] Visited;

  sInt count = g->NumVerts();
  Visited = new sU8[count];
  sSetMem(Visited,0,count);
  VisitCounter = 0;

  if(heapSize == -1)
    heapSize = sqrt(count) * 16;

  MaxHeapSize = heapSize;
  Heap = new Node *[MaxHeapSize+2];
}

bool PathFinder::FindPath(sInt a,sInt b,sArray<sInt> &path,sInt bound)
{
  // update visit counter, clearing Visited[] array if necessary
  if(++VisitCounter == 0)
  {
    sSetMem(Visited,0,Graph->NumVerts());
    VisitCounter = 1;
  }

  ClearNodes();

  Node **paths = Heap;
  sArray<sInt> adjacent;
  sInt counter = VisitCounter;
  sInt heapSize;
  bool result = false;

  Graph->SetTarget(b);
  adjacent.Init(8);
  paths[1] = NewNode(0,a,b);
  heapSize = 1;

  while(heapSize)
  {
    Node *node = paths[1];
    sInt x = node->V;

    // goal found
    if(x == b)
    {
      // yes, extract path
      sInt len = 0;
      for(const Node *c=node;c;c=c->Dad)
        len++;

      path.Resize(len);
      for(const Node *c=node;c;c=c->Dad)
        path[--len] = c->V;

      // and we're done
      result = true;
      break;
    }

    // check whether we've exceed our bound
    if(node->Priority > bound)
      break;

    // pop node from priority queue
    Node *v = paths[heapSize--];
    sInt vp = v->Priority;
    sInt j,k = 1;

    while(k <= (heapSize>>1))
    {
      j = k+k;
      if(j < heapSize && paths[j]->Priority > paths[j+1]->Priority)
        j++;

      if(vp <= paths[j]->Priority)
        break;

      paths[k] = paths[j];
      k = j;
    }

    paths[k] = v;

    // process node
    if(Visited[x] != counter) // not visited yet
    {
      Visited[x] = counter;
      Graph->EnumNeighbors(x,adjacent);

      for(sInt i=0;i<adjacent.Count;i++)
      {
        sInt n = adjacent[i];
        if(Visited[n] != counter)
        {
          // insert node into heap
          if(heapSize < MaxHeapSize)
            heapSize++;
          else
            ReleaseNode(paths[heapSize]);

          k = heapSize;
          v = NewNode(node,n,b);
          vp = v->Priority;

          while((j=k>>1) && paths[j]->Priority >= vp)
          {
            paths[k] = paths[j];
            k = j;
          }

          paths[k] = v;
        }
      }
    }
  }

  for(sInt i=1;i<=heapSize;i++)
    ReleaseNode(paths[i]);

  adjacent.Exit();
  return result;
}

/****************************************************************************/

class GridLayoutGraph : public AbstractGraph
{
  sInt Width,Height;
  sInt Shift;
  sInt Target;
  sU8 *Map;
  sBool Wrap;

public:
  // Width, Height must be power of 2
  GridLayoutGraph(sInt width,sInt height,sBool wrap)
  {
    Width = width;
    Height = height;
    Shift = sGetPower2(Width);
    Wrap = wrap;
    Target = -1;

    Map = new sU8[width*height];
    sSetMem(Map,0,width*height);
  }

  ~GridLayoutGraph()
  {
    delete[] Map;
  }

  sInt VertexIndex(sInt x,sInt y) const
  {
    if(x < 0 || x >= Width || y < 0 || y >= Height)
      return -1;

    return y * Width + x;
  }

  void GetCoords(sInt v,sInt &x,sInt &y) const
  {
    x = v & (Width - 1);
    y = (v >> Shift) & (Height - 1);
  }

  void GetDistance(sInt v1,sInt v2,sInt &dx,sInt &dy) const
  {
    sInt x1,y1,x2,y2;

    GetCoords(v1,x1,y1);
    GetCoords(v2,x2,y2);

    dx = x2 - x1;
    dy = y2 - y1;

    if(Wrap)
    {
      if(sAbs(dx) > (Width>>1))   dx = dx > 0 ? dx - Width : Width + dx;
      if(sAbs(dy) > (Height>>1))  dy = dy > 0 ? dy - Height : Height + dy;
    }
  }

  sInt NumVerts() const
  {
    return Width * Height;
  }

  sF32 Metric(sInt v1,sInt v2) const
  {
    sInt dx,dy;
    GetDistance(v1,v2,dx,dy);

    return sAbs(dx)+sAbs(dy);
  }

  sInt Cost(sInt vCur,sInt vNext,sInt vPrev) const
  {
    sInt dx,dy,cost;
    GetDistance(vCur,vNext,dx,dy);

    cost = 2 + (dx && dy);
    if(vPrev != -1)
    {
      sInt odx,ody;
      GetDistance(vPrev,vCur,odx,ody);
      if(odx == dx && ody == dy) // cheaper without direction change
        cost--;
    }

    return cost;
  }

  void EnumNeighbors(sInt v,sArray<sInt> &neighbors) const
  {
    static const sInt neigh[8][2] =
    {
      -1,-1,  0,-1,  1,-1,
      -1, 0,         1, 0,
      -1, 1,  0, 1,  1, 1,
    };

    static const sBool isDiag[8] =
    {
      sTRUE,  sFALSE, sTRUE,
      sFALSE,         sFALSE,
      sTRUE,  sFALSE, sTRUE
    };

    sInt x,y;
    GetCoords(v,x,y);

    neighbors.AtLeast(8);
    neighbors.Count = 0;
    for(sInt i=0;i<8;i++)
    {
      sInt nx,ny,mx,my;

      nx = x + neigh[i][0];
      if(Wrap)
        nx &= Width - 1;
      else if(nx < 0 || nx >= Width)
        continue;

      ny = y + neigh[i][1];
      if(Wrap)
        ny &= Height - 1;
      else if(ny < 0 || ny >= Height)
        continue;

      mx = sMin(x,nx);
      my = sMin(y,ny);

      sInt nv = ny * Width + nx;
      if((!(Map[nv] & 1) || nv == Target) // not occupied
        && (!isDiag[i] || !(Map[my*Width+mx] & 2))) // no diagonal
        neighbors[neighbors.Count++] = nv;
    }
  }

  void SetTarget(sInt v)
  {
    Target = v;
  }

  // Managing obstacles
  void AddObstacle(sInt x,sInt y)
  {
    if(x >= 0 && x < Width && y >= 0 && y < Height)
      Map[y*Width+x] |= 1;
  }

  void RectObstacle(sInt x0,sInt y0,sInt x1,sInt y1)
  {
    for(sInt y=y0;y<=y1;y++)
      for(sInt x=x0;x<=x1;x++)
        Map[y*Width+x] |= 1;
  }

  sBool IsOccupied(sInt v) const
  {
    return Map[v] & 1;
  }

  void AddPath(const sArray<sInt> &path)
  {
    for(sInt i=0;i<path.Count;i++)
    {
      sInt x1,y1,x2,y2,mx,my;

      Map[path[i]] |= 1; // occupied
      if(i < path.Count - 1)
      {
        GetCoords(path[i],x1,y1);
        GetCoords(path[i+1],x2,y2);

        mx = sMin(x1,x2);
        my = sMin(y1,y2);
        if(x1 != x2 && y1 != y2) // diagonal
          Map[my*Width+mx] |= 2; // diagonal
      }
    }
  }
};

/****************************************************************************/

class MeshGraph : public AbstractGraph
{
public:
  GenMinMesh *mesh;
  sInt vertCount,target;
  sInt *vertToEdge;
  sU8 *map;
  sF32 maxEdgeLen;

  MeshGraph()
  {
    mesh = 0;
    vertToEdge = 0;
    maxEdgeLen = 0;
    map = 0;
    target = -1;
  }

  ~MeshGraph()
  {
    mesh->Release();
    delete[] vertToEdge;
    delete[] map;
  }

  void InitMesh(GenMinMesh *msh)
  {
    mesh = new GenMinMesh;
    mesh->Copy(msh);

    sInt *merge = mesh->CalcMergeVerts();

    vertCount = 0;
    for(sInt i=0;i<mesh->Vertices.Count;i++)
    {
      if(merge[i] == i)
      {
        merge[i] = vertCount;
        mesh->Vertices[vertCount] = msh->Vertices[i];
        vertCount++;
      }
      else
        merge[i] = merge[merge[i]];
    }

    mesh->Vertices.Resize(vertCount);

    for(sInt i=0;i<mesh->Faces.Count;i++)
      for(sInt j=0;j<mesh->Faces[i].Count;j++)
        mesh->Faces[i].Vertices[j] = merge[mesh->Faces[i].Vertices[j]];

    delete[] merge;
    mesh->CalcAdjacency();
    mesh->VerifyAdjacency();
    mesh->CalcNormals();

    vertToEdge = new sInt[vertCount];
    sSetMem(vertToEdge,0xff,vertCount * sizeof(sInt));
    map = new sU8[vertCount];
    sSetMem(map,0,vertCount);

    maxEdgeLen = 0;

    for(sInt i=0;i<mesh->Faces.Count;i++)
    {
      GenMinFace *face = &mesh->Faces[i];
      if(face->Count < 3)
        continue;

      for(sInt j=0;j<face->Count;j++)
      {
        sInt v = face->Vertices[j];
        sInt v1 = face->Vertices[(j+1) % face->Count];

        GenMinVector dist;
        dist = mesh->Vertices[v].Pos;
        dist.Sub(mesh->Vertices[v1].Pos);
        sF32 len = dist.Len();
        maxEdgeLen = sMax(len,maxEdgeLen);

        vertToEdge[v] = i*8+j;
      }
    }

    msh->Release();
  }

  void GetPosNormal(sInt vert,sVector &pos,sVector &norm)
  {
    const GenMinVert *v = &mesh->Vertices[vert];
    pos.Init(v->Pos.x,v->Pos.y,v->Pos.z,1.0f);
    norm.Init(v->Normal.x,v->Normal.y,v->Normal.z,0.0f);
  }

  // --- graph interface
  sInt NumVerts() const
  {
    return vertCount;
  }

  sF32 Metric(sInt v1,sInt v2) const
  {
    GenMinVector dist;

    dist.Sub(mesh->Vertices[v1].Pos,mesh->Vertices[v2].Pos);
    return dist.Len() / maxEdgeLen;
  }

  sInt Cost(sInt vCur,sInt vNext,sInt vPrev) const
  {
    GenMinVector dir,dir2;
    sInt cost = 2;

    if(vPrev != -1)
    {
      const GenMinVert *prev = &mesh->Vertices[vPrev];
      const GenMinVert *cur = &mesh->Vertices[vCur];
      const GenMinVert *next = &mesh->Vertices[vNext];

      dir.Sub(next->Pos,cur->Pos);
      dir2.Sub(cur->Pos,prev->Pos);

      if(dir.Dot(dir2) >= 0.7f * dir.Len() * dir2.Len())
        cost--;
    }

    return cost;
  }

  void EnumNeighbors(sInt v,sArray<sInt> &neighbors) const
  {
    sInt e = vertToEdge[v],ee = e;

    neighbors.Count = 0;
    if(e != -1)
    {
      do
      {
        sInt o = mesh->OppositeEdge(e);
        sInt v = mesh->Faces[o >> 3].Vertices[o & 7];
        if(!(map[v] & 1) || target == v)
          *neighbors.Add() = v;

        e = mesh->NextVertEdge(e);
      }
      while(e != ee);
    }
  }

  void SetTarget(sInt v)
  {
    target = v;
  }

  sBool IsOccupied(sInt v) const
  {
    return map[v] & 1;
  }

  void AddPath(const sArray<sInt> &path)
  {
    for(sInt i=0;i<path.Count;i++)
      map[path[i]] |= 1; // occupied
  }
};

/****************************************************************************/

static void RandomWiringGraph(AbstractGraph &graph,PathFinder &finder,
  sInt nPointsPerIt,sInt nEdgesPerIt,sInt iterations,sInt maxLen,
  sArray<sInt> &points,sArray<sInt> &paths,sBool dontBound = sFALSE)
{
  sArray<sInt> path;

  points.AtLeast(nPointsPerIt*iterations);
  path.Init();

  sInt origPointCount = points.Count;

  for(sInt iter=0;iter<iterations;iter++)
  {
    sInt oldPoints = points.Count;

    // gen. random points
    for(sInt i=0;i<nPointsPerIt;i++)
    {
      sInt v = sGetRnd(graph.NumVerts());
      if(!graph.IsOccupied(v))
        *points.Add() = v;
    }

    // gen. random paths
    for(sInt i=0;i<nEdgesPerIt;i++)
    {
      sInt p0,p1;

      do
      {
        p0 = points[sGetRnd(points.Count - origPointCount) + origPointCount];
        p1 = points[sGetRnd(points.Count - origPointCount) + origPointCount];
      }
      while(p0 == p1);

      sF32 len = graph.Metric(p0,p1);
      if(!len || len > maxLen)
        continue;

      if(finder.FindPath(p0,p1,path,0x7fffffff/*dontBound ? 0x7fffffff : 2*3*len*/))
      {
        graph.AddPath(path);
        paths.AtLeast(paths.Count + path.Count + 1);
        for(sInt i=0;i<path.Count;i++)
          *paths.Add() = path[i];

        *paths.Add() = -1;
      }
    }

    // remove unused points
    for(sInt i=oldPoints;i<points.Count;)
    {
      if(!graph.IsOccupied(points[i]))
        points[i] = points[--points.Count];
      else
        i++;
    }
  }

  path.Exit();
}

static void ShadeMax(sU64 *point,sInt level)
{
  sU16 *pt = (sU16 *) point;

  pt[0] = sMax<sInt>(pt[0],level);
  pt[1] = sMax<sInt>(pt[1],level);
  pt[2] = sMax<sInt>(pt[2],level);
  pt[3] = sMax<sInt>(pt[3],0);
}

static void ShadeSet(sU64 *point,sInt level)
{
  sU16 *pt = (sU16 *) point;

  pt[0] = level;
  pt[1] = level;
  pt[2] = level;
  pt[3] = 0;
}

GenBitmap * __stdcall Bitmap_Geloet(sInt xs,sInt ys,sInt flags,sInt pointsPerIt,sInt edgesPerIt,sInt iters,sInt maxLen,sInt seed,sF32 lineDiam,sF32 radMiddle,sF32 radDisc)
{
  GenBitmap *bm = Bitmap_Flat(xs,ys,0);

  GridLayoutGraph graph(sMax((1<<xs)/16,1),sMax((1<<ys)/16,1),flags & 1);
  PathFinder finder;
  sArray<sInt> points,paths;

  finder.SetGraph(&graph,64*16);
  points.Init();
  paths.Init();

  sSetRndSeed(seed);
  RandomWiringGraph(graph,finder,pointsPerIt,edgesPerIt,iters,maxLen,points,paths);

  sF32 lineRad = lineDiam * 0.5f;
  
  // render the lines
  sInt lastPoint = -1;
  for(sInt i=0;i<paths.Count;i++)
  {
    sInt newPoint = paths[i];

    if(lastPoint != -1 && newPoint != -1)
    {
      sInt x1,y1;
      sInt dx,dy;

      graph.GetCoords(lastPoint,x1,y1);
      graph.GetDistance(lastPoint,newPoint,dx,dy);

      sInt cx = x1*16 + dx*8;
      sInt cy = y1*16 + dy*8;
      sF32 norm = 1.0f / sFSqrt(dx*dx + dy*dy);
      sF32 lnorm = 1.0f / (sAbs(dx)+sAbs(dy));

      for(sInt y=-9;y<=9;y++)
      {
        sInt ry = (y+cy+8) & (bm->YSize - 1);

        for(sInt x=-9;x<=9;x++)
        {
          sInt rx = (x+cx+8) & (bm->XSize - 1);

          // calc distance to line
          if(sFAbs((x*dx + y*dy) * lnorm) <= 8.0f)
          {
            sF32 dist = sMin<sF32>(lineRad - sFAbs(x*dy - y*dx) * norm,1.0f);
            if(dist > 0)
              ShadeMax(&bm->Data[ry*bm->YSize+rx],sFtol(dist*32767.0f));
          }
        }
      }
      // draw a line from last to current point
    }

    lastPoint = newPoint;
  }

  // render the points
  for(sInt i=0;i<points.Count;i++)
  {
    sInt cx,cy;
    graph.GetCoords(points[i],cx,cy);

    cx *= 16;
    cy *= 16;

    for(sInt y=-8;y<=8;y++)
    {
      sInt ry = (y+cy+8) & (bm->YSize - 1);
      sInt ys = y*y;

      for(sInt x=-8;x<=8;x++)
      {
        sInt rx = (x+cx+8) & (bm->XSize - 1);
        sInt xs = x*x;
        sF32 dist = (sFSqrt(xs+ys) - radMiddle) / radDisc;
        if(dist > 0 && dist < 1.0f)
          ShadeMax(&bm->Data[ry*bm->YSize+rx],sFtol(sMin((1.0f-dist)*radDisc,1.0f)*32767.0f));
        else if(dist <= 0)
          ShadeSet(&bm->Data[ry*bm->YSize+rx],sFtol(sRange((1.0f+dist)*radDisc,1.0f,0.0f)*32767.0f));
      }
    }
  }

  paths.Exit();
  points.Exit();

  return bm;
}

/****************************************************************************/

// character table (rygs font)
static const sChar charOrder[] = "abcdefghijklmnopqrstuvwxyz0123456789-+:";
static const sS8 *charPtr[sizeof(charOrder)];
static const sS8 chars[] = {
  // A
  3,0, 0,1,3, -1,0,2, -1,-1,1, 0,-1,1, 1,-1,1, 1,0,1, 1,1,1, 0,0,1, // stroke 1
  // B
  0,0, 1,0,2, 1,1,1, 0,1,1, -1,1,1, -1,0,2, 0,0,0, // stroke 1
  0,0, 0,1,5, 0,0,1, // stroke 2
  // C
  3,0, -1,0,2, -1,1,1, 0,1,1, 1,1,1, 1,0,1, 1,-1,1, 0,0,1, // stroke 1
  // D
  3,0, -1,0,2, -1,1,1, 0,1,1, 1,1,1, 1,0,2, 0,0,0, // stroke 1
  3,0, 0,1,5, 0,0,1, // stroke 2
  // E
  3,0, -1,0,2, -1,1,1, 0,0,0, // stroke 1
  0,1, 0,1,1, 1,1,1, 1,0,1, 1,-1,1, 0,0,0, // stroke 2
  3,2, -1,0,3, 0,0,1, // stroke 3
  // F
  1,0, 0,1,4, 1,1,1, 1,-1,1, 0,0,0, // stroke 1
  0,3, 1,0,2, 0,0,1, // stroke 2
  // G
  3,0, -1,0,2, -1,1,1, 0,1,1, 1,1,1, 1,0,2, 0,0,0, // stroke 1
  3,0, 0,1,3, 0,0,0, // stroke 2
  3,0, -1,-1,1, -1,0,2, 0,0,1, // stroke 3
  // H
  0,0, 0,1,5, 0,0,0, // stroke 1
  0,3, 1,0,2, 1,-1,1, 0,-1,2, 0,0,1, // stroke 2
  // I
  0,0, 1,0,2, 0,0,0, // stroke 1
  1,0, 0,1,3, 0,0,0, // stroke 2
  0,3, 1,0,1, 0,0,0, // stroke 3
  1,4, 0,0,1, // stroke 4
  // J
  1,3, 1,0,1, 0,0,0, // stroke 1
  2,3, 0,-1,3, -1,-1,1, -1,0,1, 0,0,0, // stroke 2
  2,4, 0,0,1, // stroke 3
  // K
  0,0, 0,1,5, 0,0,0, // stroke 1
  0,1, 1,1,2, 0,0,0, // stroke 2
  1,2, 1,-1,2, 0,0,1, // stroke 3
  // L
  0,0, 1,0,2, 0,0,0, // stroke 1
  1,0, 0,1,5, -1,0,1, 0,0,1, // stroke 2
  // M
  0,0, 0,1,3, 0,0,0, // stroke 1
  0,3, 1,0,2, 0,0,0, // stroke 2
  2,3, 0,-1,3, 0,0,0, // stroke 3
  2,3, 1,0,1, 1,-1,1, 0,-1,2, 0,0,1, // stroke 4
  // N
  0,0, 0,1,3, 0,0,0, // stroke 1
  0,2, 1,1,1, 1,0,1, 1,-1,1, 0,-1,2, 0,0,1, // stroke 2
  // O
  0,2, 1,1,1, 1,0,1, 1,-1,1, 0,-1,1, -1,-1,1, -1,0,1, -1,1,1, 0,1,1, 0,0,1, // stroke 1
  // P
  0,0, 1,0,2, 1,1,1, 0,1,1, -1,1,1, -1,0,2, 0,0,0, // stroke 1
  0,3, 0,-1,4, 0,0,1, // stroke 2
  // Q
  3,0, -1,0,2, -1,1,1, 0,1,1, 1,1,1, 1,0,2, 0,0,0, // stroke 1
  3,3, 0,-1,4, 0,0,1, // stroke 2
  // R
  0,0, 0,1,3, 0,0,0, // stroke 1
  0,2, 1,1,1, 1,0,1, 1,-1,1, 0,0,1, // stroke 2
  // S
  0,0, 1,0,2, 1,1,1, -1,1,1, -1,0,2, 1,1,1, 1,0,2, 0,0,1, // stroke 1
  // T
  2,0, -1,1,1, 0,1,4, 0,0,0, // stroke 1
  0,3, 1,0,2, 0,0,1, // stroke 2
  // U
  3,0, 0,1,3, 0,0,0, // stroke 1
  3,1, -1,-1,1, -1,0,1, -1,1,1, 0,1,2, 0,0,1, // stroke 2
  // V
  2,0, 0,1,3, 0,0,0, // stroke 1
  2,0, -1,1,2, 0,1,1, 0,0,1, // stroke 2
  // W
  0,3, 0,-1,2, 1,-1,1, 1,0,1, 0,0,0, // stroke 1
  2,0, 0,1,3, 0,0,0, // stroke 2
  2,0, 1,0,2, 0,0,0, // stroke 3
  4,0, 0,1,3, 0,0,1, // stroke 4
  // X
  0,0, 1,1,3, 0,0,0, // stroke 1
  0,3, 1,-1,3, 0,0,1, // stroke 2
  // Y
  0,3, 0,-1,2, 1,-1,1, 1,0,2, 0,0,0, // stroke 1
  3,0, 0,1,3, 0,0,0, // stroke 2
  3,0, -1,-1,1, -1,0,2, 0,0,1, // stroke 3
  // Z
  0,0, 1,1,3, 0,0,0, // stroke 1
  0,0, 1,0,3, 0,0,0, // stroke 2
  0,3, 1,0,3, 0,0,1, // stroke 3
  // 0
  0,1, 0,1,3, 1,1,1, 1,0,1, 1,-1,1, 0,-1,3, -1,-1,1, -1,0,1, -1,1,1, 0,0,0, // stroke 1
  0,1, 1,1,3, 0,0,1, // stroke 2
  // 1
  0,3, 1,1,2, 0,0,0, // stroke 1
  2,5, 0,-1,5, 0,0,1, // stroke 2
  // 2
  0,3, 0,1,1, 1,1,1, 1,0,1, 1,-1,1, 0,-1,1, -1,-1,3, 1,0,3, 0,0,1, // stroke 1
  // 3
  0,4, 1,1,1, 1,0,1, 1,-1,1, 0,-1,3, -1,-1,1, -1,0,1, -1,1,1, 0,0,0, // stroke 1
  3,3, -1,0,2, 0,0,1, // stroke 2
  // 4
  2,0, 0,1,5, -1,-1,2, 1,0,3, 0,0,1, // stroke 1
  // 5
  0,0, 1,0,2, 1,1,1, 0,1,1, -1,1,1, -1,0,2, 0,1,2, 1,0,3, 0,0,1, // stroke 1
  // 6
  0,1, 1,-1,1, 1,0,1, 1,1,1, 0,1,1, -1,1,1, -1,0,2, 0,-1,2, 0,0,0, // stroke 1
  0,3, 0,1,1, 1,1,1, 1,0,1, 1,-1,1, 0,0,1, // stroke 2
  // 7
  2,0, 0,1,2, 1,1,1, 0,1,2, -1,0,3, 0,0,1, // stroke 1
  // 8
  1,0, 1,0,1, 1,1,1, 0,1,1, -1,1,1, -1,0,1, -1,-1,1, 0,-1,1, 1,-1,1, 0,0,0, // stroke 1
  2,3, 1,1,1, -1,1,1, -1,0,1, -1,-1,1, 1,-1,1, 0,0,1, // stroke 2
  // 9
  0,0, 1,0,2, 1,1,1, 0,1,2, 0,0,0, // stroke 1
  3,3, 0,1,1, -1,1,1, -1,0,1, -1,-1,1, 1,-1,1, 1,0,2, 0,0,1, // stroke 2
  // -
  0,3, 1,0,3, 0,0,1, // stroke 1
  // +
  0,3, 1,0,2, 0,0,0, // stroke 1
  1,1, 0,1,4, 0,0,1, // stroke 2
  // :
  0,0, 0,0,0, // stroke 1
  0,3, 0,0,1, // stroke 2
};

/*// character table (ways font)
static const sChar charOrder[] = "abcdefghijklmnopqrstuvwxyz1234567890. ";
static const sS8 chars[] = {
  // Ax
  0,0, 0,1,4, 1,1,2, 1,-1,2, 0,-1,4, -1,0,1, 0,-1,2, -1,0,2, 0,0,0, // stroke 1
  1,2, 0,-1,2, -1,0,1, 0,0,0, // stroke 2
  1,4, 0,1,1, 1,1,1, 1,-1,1, 0,-1,1, -1,0,2, 0,0,1, // stroke 3
  // B
  0,0, 0,1,6, 0,0,0, // stroke 1
  0,6, 1,0,3, 1,-1,1, 0,0,0, // stroke 2
  4,5, -1,0,3, 0,1,1, 1,0,2, 0,1,1, 0,0,0, // stroke 3
  4,5, 0,1,1, -1,1,1, 0,0,0, // stroke 4
  3,3, -1,0,2, 0,-1,2, 1,0,1, 1,1,1, -1,1,1, 0,0,0, // stroke 5
  3,3, 1,-1,1, 0,-1,1, -1,-1,1, -1,0,3, 0,0,1, // stroke 6
  // C

};*/

static void initCharset()
{
  sInt nChars = sizeof(charOrder) - 1;
  const sS8 *chptr = chars;

  for(sInt i=0;i<nChars;i++)
  {
    charPtr[i] = chptr;

    do
    {
      chptr += 2;
      while(chptr[0] != 0 || chptr[1] != 0)
        chptr += 3;
      
      chptr += 3;
    }
    while(chptr[-1] != 1);
  }

  sVERIFY(chptr == chars + sizeof(chars));
}

/****************************************************************************/

class GraphPainterEffect : public GenEffect
{
public:
  sArray<sInt> points,paths;
  sArray<sInt> pathLen;
  sArray<sInt> pathStart;
  sArray<sInt> pathTime;
  sVector *positions; // for each graph node
  sVector *normals;
  sInt handle;
  sInt totalLen;

  struct GraphNode
  {
    sArray<sInt> OutgoingVert;
    sArray<sInt> OutgoingPath;
  };

  GraphNode *nodes;

  sInt FindPoint(sInt v)
  {
    sInt i;

    for(i=0;i<points.Count;i++)
      if(points[i] == v)
        return i;

    return -1;
  }

  void BuildNodes()
  {
    nodes = new GraphNode[points.Count];
    for(sInt i=0;i<points.Count;i++)
    {
      nodes[i].OutgoingVert.Init();
      nodes[i].OutgoingPath.Init();
    }

    for(sInt i=0;i<pathStart.Count;i++)
    {
      sInt a = FindPoint(paths[pathStart[i]]);
      sInt b = FindPoint(paths[pathStart[i] + pathLen[i] - 1]);

      *nodes[a].OutgoingVert.Add() = b;
      *nodes[a].OutgoingPath.Add() = i;
      *nodes[b].OutgoingVert.Add() = a;
      *nodes[b].OutgoingPath.Add() = i;
    }
  }

  void DistribStartTime(sInt *start,sInt &uninit,sInt node)
  {
    sInt startTime = start[node];

    for(sInt i=0;i<nodes[node].OutgoingPath.Count;i++)
    {
      sInt path = nodes[node].OutgoingPath[i];
      sInt vert = nodes[node].OutgoingVert[i];

      sInt len = pathLen[path];
      if(start[vert] == -1 || startTime+len < start[vert])
      {
        if(start[vert] == -1)
          uninit--;

        start[vert] = startTime + len;
        DistribStartTime(start,uninit,vert);
      }
    }
  }

  virtual sInt StartMetric(sInt v) const = 0;

  void DetermineStartTime()
  {
    sInt *start = new sInt[points.Count];
    sInt uninit = points.Count,minStart = 0x7fffffff;
    for(sInt i=0;i<points.Count;i++)
      start[i] = -1;

    // while unitialized components exist, distribute start time...
    while(uninit)
    {
      // find min point
      sInt i,j;
      sInt minMetric = 0x7fffffff;
      for(j=0;j<points.Count;j++)
      {
        if(start[j] == -1)
        {
          sInt m = StartMetric(points[j]);
          /*sInt x,y;
          graph.GetCoords(points[j],x,y);
          
          sInt m = x+y;*/
          if(m < minMetric)
          {
            i = j;
            minMetric = m;
          }
        }
      }

      sVERIFY(i < points.Count);

      // distribute start time
      start[i] = minMetric * 3 / 2;
      minStart = sMin(minStart,start[i]);
      uninit--;
      DistribStartTime(start,uninit,i);
    }

    for(sInt i=0;i<points.Count;i++)
      start[i] -= minStart;

    // flip paths as necessary
    for(sInt i=0;i<pathStart.Count;i++)
    {
      sInt s = pathStart[i];
      sInt l = pathLen[i];
      sInt a = FindPoint(paths[s]);
      sInt b = FindPoint(paths[s + l - 1]);

      if(start[b] < start[a]) // need to flip path
      {
        for(sInt j=0;j<l/2;j++)
          sSwap(paths[s+j],paths[s+l-1-j]);

        pathTime[i] = start[b];
      }
      else
        pathTime[i] = start[a];
    }

    delete[] start;
  }

  void DestroyNodes()
  {
    for(sInt i=0;i<points.Count;i++)
    {
      nodes[i].OutgoingPath.Exit();
      nodes[i].OutgoingVert.Exit();
    }

    delete[] nodes;
  }

  void DetermineTotalLen()
  {
    totalLen = 0;
    for(sInt i=0;i<pathStart.Count;i++)
      totalLen = sMax(totalLen,pathTime[i] + pathLen[i]);
  }

  void PreparePaths()
  {
    // determine path lengts and number of paths
    sBool lastOver = sTRUE;
    pathLen.Init();
    pathStart.Init();
    pathTime.Init();

    for(sInt i=0;i<paths.Count;i++)
    {
      if(lastOver)
      {
        *pathStart.Add() = i;
        *pathTime.Add() = 0;
        lastOver = sFALSE;
      }

      if(paths[i] == -1)
      {
        *pathLen.Add() = i - pathStart[pathStart.Count-1];
        lastOver = sTRUE;
      }
    }

    // length
    BuildNodes();
    DetermineStartTime();
    DestroyNodes();

    // determine total length
    DetermineTotalLen();
  }

  GraphPainterEffect()
  {
    points.Init();
    paths.Init();

    handle = sSystem->GeoAdd(sFVF_TSPACE3,sGEO_TRI|sGEO_DYNAMIC);
    positions = 0;
    normals = 0;
  }

  ~GraphPainterEffect()
  {
    sSystem->GeoRem(handle);

    delete[] positions;
    delete[] normals;

    points.Exit();
    paths.Exit();
  }

  void RenderLine(sInt pathStart,sF32 width,sF32 animate)
  {
    // determine path length
    sInt pathLen;
    for(pathLen=0;paths[pathStart+pathLen]!=-1;pathLen++);

    // draw the line (somehow)
    sVertexTSpace3 *vp;
    sU16 *ip;

    sSystem->GeoBegin(handle,pathLen*4,(2*pathLen-1)*6,(sF32 **) &vp,(void **) &ip);

    // prepare vertices
    sVector dir,dirn;

    sInt realLen = animate * (pathLen - 1) + 0.999f;
    sF32 fracLen = realLen - animate * (pathLen - 1);
    sInt icOut=0,vcOut = 0;

    for(sInt i=0;i<realLen;i++)
    {
      sInt v;
      sVector pos;
      sF32 bevelFore=i ? width : 0,bevelAft;

      v = paths[pathStart+i];
      pos = positions[v];
      if(i != pathLen-1)
      {
        dir = positions[paths[pathStart+i+1]];
        dir.Sub3(pos);
      }

      sF32 scale = 1.0f / dir.Abs3();
      sF32 realFrac = (i == realLen - 1) ? 1.0f - fracLen : 1.0f;

      if(realFrac < width)
        bevelFore *= realFrac / width;

      if(i != realLen - 1)
        bevelAft = width;
      else
        bevelAft = 0.0f;

      bevelFore *= scale;
      bevelAft *= scale;
        //bevelAft = sFSqrt(dx*dx+dy*dy) * fracLen;

      dirn.Cross3(dir,normals[v]);
      dirn.Unit3();

      // 4 quad vertices
      /*vp->x = x - width*dyn + bevelFore * dx;
      vp->y = y + width*dxn + bevelFore * dy;
      vp->z = 0.0f;*/
      vp->x = pos.x + width * dirn.x + bevelFore * dir.x;
      vp->y = pos.y + width * dirn.y + bevelFore * dir.y;
      vp->z = pos.z + width * dirn.z + bevelFore * dir.z;
      vp->c = ~0U;
      vp->n = 0x808000;
      vp->s = 0x80ff80;
      vp->u = i;
      vp->v = 0.0f;
      vp++;

      /*vp->x = x + width*dyn + bevelFore * dx;
      vp->y = y - width*dxn + bevelFore * dy;
      vp->z = 0.0f;*/
      vp->x = pos.x - width * dirn.x + bevelFore * dir.x;
      vp->y = pos.y - width * dirn.y + bevelFore * dir.y;
      vp->z = pos.z - width * dirn.z + bevelFore * dir.z;
      vp->c = ~0U;
      vp->n = 0x808000;
      vp->s = 0x80ff80;
      vp->u = i;
      vp->v = 1.0f;
      vp++;

      /*vp->x = x + (realFrac-bevelAft)*dx - width*dyn;
      vp->y = y + (realFrac-bevelAft)*dy + width*dxn;
      vp->z = 0.0f;*/
      vp->x = pos.x + width * dirn.x + (realFrac-bevelAft) * dir.x;
      vp->y = pos.y + width * dirn.y + (realFrac-bevelAft) * dir.y;
      vp->z = pos.z + width * dirn.z + (realFrac-bevelAft) * dir.z;
      vp->c = ~0U;
      vp->n = 0x808000;
      vp->s = 0x80ff80;
      vp->u = i;
      vp->v = 0.0f;
      vp++;

      /*vp->x = x + (realFrac-bevelAft)*dx + width*dyn;
      vp->y = y + (realFrac-bevelAft)*dy - width*dxn;
      vp->z = 0.0f;*/
      vp->x = pos.x - width * dirn.x + (realFrac-bevelAft) * dir.x;
      vp->y = pos.y - width * dirn.y + (realFrac-bevelAft) * dir.y;
      vp->z = pos.z - width * dirn.z + (realFrac-bevelAft) * dir.z;
      vp->c = ~0U;
      vp->n = 0x808000;
      vp->s = 0x80ff80;
      vp->u = i;
      vp->v = 1.0f;
      vp++;
      vcOut += 4;

      // bevel indices
      if(i != 0)
      {
        *ip++ = (i-1) * 4 + 2;
        *ip++ =     i * 4 + 0;
        *ip++ =     i * 4 + 1;

        *ip++ = (i-1) * 4 + 2;
        *ip++ =     i * 4 + 1;
        *ip++ = (i-1) * 4 + 3;
        icOut += 6;
      }

      // indices
      *ip++ = i * 4 + 0;
      *ip++ = i * 4 + 2;
      *ip++ = i * 4 + 1;

      *ip++ = i * 4 + 2;
      *ip++ = i * 4 + 3;
      *ip++ = i * 4 + 1;
      icOut += 6;
    }

    sSystem->GeoEnd(handle,vcOut,icOut);
    sSystem->GeoDraw(handle);
  }

  void RenderLines(sF32 width,sF32 animate)
  {
    sF32 timeInLen = sRange<sF32>(animate * totalLen,totalLen,0);

    // render all lines
    for(sInt i=0;i<pathStart.Count;i++)
    {
      sF32 timeInPath = sRange((timeInLen - pathTime[i]) / pathLen[i],1.0f,0.0f);
      if(timeInPath)
        RenderLine(pathStart[i],width,timeInPath);
    }
  }

  void RenderPoints(sF32 width)
  {
    sVertexTSpace3 *vp;
    sU16 *ip;

    sSystem->GeoBegin(handle,points.Count*4,points.Count*6,(sF32 **) &vp,(void **) &ip);
    sSystem->GeoEnd(handle);

    for(sInt i=0;i<points.Count;i++)
    {
      sInt v = points[i];
      sVector pos = positions[v];
      sVector axis1,axis2;
      /*sInt x,y;

      graph.GetCoords(points[i],x,y);*/

      axis2.Init(1,0,0);
      if(normals[v].Dot3(axis2) > 0.95f)
        axis1.Init(0,1,0);
      else
        axis1.Cross3(axis2,normals[v]);

      axis1.UnitSafe3();
      axis2.Cross3(axis1,normals[v]);

      for(sInt j=0;j<4;j++)
      {
        sVector p;

        p = pos;
        p.AddScale3(axis1,(j & 1) ? width : -width);
        p.AddScale3(axis2,(j & 2) ? -width : width);

        /*vp->x = x + ((j&1) ? width : -width);
        vp->y = y + ((j&2) ? -width : width);
        vp->z = 0.0f;*/
        vp->x = p.x;
        vp->y = p.y;
        vp->z = p.z;
        vp->c = ~0U;
        vp->n = 0x808000;
        vp->s = 0x80ff80;
        vp->u = j&1;
        vp->v = j>>1;
        vp++;
      }

      *ip++ = i*4+0;
      *ip++ = i*4+1;
      *ip++ = i*4+2;
      *ip++ = i*4+1;
      *ip++ = i*4+3;
      *ip++ = i*4+2;
    }

    sSystem->GeoDraw(handle);
  }
};

class GeloetEffect : public GraphPainterEffect
{
  GridLayoutGraph graph;
  PathFinder finder;

  sInt StartMetric(sInt v) const
  {
    sInt x,y;

    graph.GetCoords(v,x,y);
    return x+y;
  }

  void AddPoint(sInt x,sInt y)
  {
    sInt idx = graph.VertexIndex(x,y);
    if(FindPoint(idx) == -1)
      *points.Add() = idx;
  }

  sInt PrintString(sInt x,sInt y,const sChar *msg,sChar term=0)
  {
    sInt ox = x++;

    while(*msg && *msg != term)
    {
      sInt index;
      for(index=0;charOrder[index] && charOrder[index] != *msg;index++);

      if(charOrder[index])
      {
        const sS8 *stroke = charPtr[index];
        sInt mx = 0;
        sInt rx = 0,ry = 0;
        sInt ind;

        do
        {
          rx = *stroke++;
          ry = *stroke++;
          AddPoint(x+rx,y+ry);
          *paths.Add() = ind = graph.VertexIndex(x+rx,y+ry);
          if(ind == -1)
            break;

          while(stroke[0] != 0 || stroke[1] != 0)
          {
            sInt count = stroke[2];
            while(count--)
            {
              rx += stroke[0]; mx = sMax(mx,rx);
              ry += stroke[1];
              *paths.Add() = ind = graph.VertexIndex(x+rx,y+ry);
              if(ind == -1)
                break;
            }

            stroke += 3;
          }

          *paths.Add() = -1;
          AddPoint(x+rx,y+ry);

          stroke += 3;
        }
        while(stroke[-1] != 1);

        x += mx + 1;
      }
      else
        x += 3;

      msg++;
    }

    AddPoint(ox,y-2);
    AddPoint(x,y+6);

    // bottom path
    for(sInt i=ox;i<=x;i++)
      *paths.Add() = graph.VertexIndex(i,y-2);

    for(sInt i=y-1;i<=y+6;i++)
      *paths.Add() = graph.VertexIndex(x,i);

    *paths.Add() = -1;

    // top path
    for(sInt i=y-2;i<=y+6;i++)
      *paths.Add() = graph.VertexIndex(ox,i);

    for(sInt i=ox+1;i<=x;i++)
      *paths.Add() = graph.VertexIndex(i,y+6);

    *paths.Add() = -1;

    graph.RectObstacle(ox,y-2,x,y+6);

    return x;
  }

public:
  GeloetEffect(sInt width,sInt height,sInt flags,sInt pointsIt,sInt edgesIt,sInt iters,sInt maxLen,sInt seed,const sChar *text)
    : graph(width,height,flags & 1)
  {
    finder.SetGraph(&graph,256*16/*64*16*/);

    initCharset();

    sSetRndSeed(seed);
    //RandomWiringGraph(graph,finder,25,50,15,48,points,paths);
    //RandomWiringGraph(graph,finder,100,5000,200,64,points,paths);

    if(flags & 2)
    {
      sInt x = 2;
      const sChar *greets = text;

      /*static const char *greets[] =
      {
        "0ok",
        "asd",
        "bauknecht",
        "calodox",
        "conspiracy",
        "equinox",
        "fairlight",
        "fit+bandwagon",
        "freestyle",
        "haujobb",
        "kewlers",
        "kolor",
        "komplex",
        "lkcc",
        "lunix",
        "matt current",
        "mawi",
        "mfx",
        "moppi",
        "neuro",
        "pain",
        "rgba",
        "scala",
        "smash designs",
        "stravaganza",
        "synesthetics",
        "tbl",
        "xplsv",
        "everyone we forgot",
      };

      for(sInt i=0;i<sizeof(greets)/sizeof(*greets);i++)
        x = PrintString(x,2+sGetRnd(22),greets[i]) + 3 + sGetRnd(8);*/

      while(*greets)
      {
        x = PrintString(x,(height - 28) / 2+sGetRnd(22),greets,'|') + 3 + sGetRnd(8);
        while(*greets && *greets != '|')
          greets++;

        if(*greets == '|')
          greets++;
      }
    }

    //RandomWiringGraph(graph,finder,100,5000,80,64,points,paths);
    if(flags & 4)
    {
      pointsIt *= 10;
      edgesIt *= 10;
    }

    RandomWiringGraph(graph,finder,pointsIt,edgesIt,iters,maxLen,points,paths);

    sInt vCount = graph.NumVerts();
    positions = new sVector[vCount];
    normals = new sVector[vCount];

    for(sInt i=0;i<vCount;i++)
    {
      sInt x,y;

      graph.GetCoords(i,x,y);
      positions[i].Init(x,y,0,1);
      normals[i].Init(0,0,-1,1);
    }

    PreparePaths();
  }

  ~GeloetEffect()
  {
  }
};

class MeshGeloetEffect : public GraphPainterEffect
{
  MeshGraph graph;
  PathFinder finder;

  sInt StartMetric(sInt v) const
  {
    sVector &pos = positions[v];
    return (sFSqrt(pos.x*pos.x + pos.z*pos.z) + sFAbs(pos.y)) * 4;

    return 0;
    //return sGetRnd(200);

    /*sVector &pos = positions[v];
    return pos.x+pos.y+pos.z;*/
  }

public:
  MeshGeloetEffect(GenMinMesh *msh,sInt pointsIt,sInt edgesIt,sInt iters,sInt maxLen,sInt seed)
  {
    graph.InitMesh(msh);
    finder.SetGraph(&graph,32768);

    sSetRndSeed(seed);
    RandomWiringGraph(graph,finder,pointsIt,edgesIt,iters,maxLen,points,paths,sTRUE);

    sInt vCount = graph.NumVerts();
    positions = new sVector[vCount];
    normals = new sVector[vCount];

    for(sInt i=0;i<vCount;i++)
      graph.GetPosNormal(i,positions[i],normals[i]);

    PreparePaths();
  }
};


KObject * __stdcall Init_Effect_Geloet2D(class GenMaterial *mtrl,class GenMaterial *mtrl2,sF32 animate,sInt gw,sInt gh,sInt flags,sInt ptIter,sInt edgeIter,sInt iters,sInt maxLen,sInt rndSeed,sF32 lineWidth,sF32 ptWidth,const sChar *text)
{
  GeloetEffect *fx = new GeloetEffect(1<<gw,1<<gh,flags,ptIter,edgeIter,iters,maxLen,rndSeed,text);
  if(mtrl)
  {
    fx->Pass = mtrl->Passes[0].Pass;
    fx->Usage = mtrl->Passes[0].Usage;
    mtrl->Release();
  }

  return fx;
}

void __stdcall Exec_Effect_Geloet2D(KOp *op,KEnvironment *kenv,sF32 animate,sInt gw,sInt gh,sInt flags,sInt ptIter,sInt edgeIter,sInt iters,sInt maxLen,sInt rndSeed,sF32 lineWidth,sF32 ptWidth,const sChar *text)
{
  GeloetEffect *fx = (GeloetEffect *) op->Cache;
  GenMaterial *mtrl = (GenMaterial *) op->GetLinkCache(0);
  GenMaterial *mtrlPoint = (GenMaterial *) op->GetLinkCache(1);

  if(fx && mtrl && mtrl->Passes.Count > 0)
  {
    kenv->CurrentCam.ModelSpace = kenv->ExecStack.Top();
    sSystem->SetViewProject(&kenv->CurrentCam);

    for(sInt i=0;i<mtrl->Passes.Count;i++)
    {
      mtrl->Passes[i].Mtrl->Set(kenv->CurrentCam);
      fx->RenderLines(lineWidth,sRange<sF32>(animate,1.0f,0.0f));
    }

    if(mtrlPoint && mtrlPoint->Passes.Count > 0)
    {
      mtrlPoint->Passes[0].Mtrl->Set(kenv->CurrentCam);
      fx->RenderPoints(ptWidth);
    }
  }
}

KObject * __stdcall Init_Effect_GeloetMesh(class GenMinMesh *msh,class GenMaterial *mtrl,class GenMaterial *mtrl2,sF32 animate,sInt ptIter,sInt edgeIter,sInt iters,sInt maxLen,sInt rndSeed,sF32 lineWidth,sF32 ptWidth)
{
  if(!msh || !mtrl || !mtrl2)
    return 0;

  MeshGeloetEffect *fx = new MeshGeloetEffect(msh,ptIter,edgeIter,iters,maxLen,rndSeed);
  if(mtrl)
  {
    fx->Pass = mtrl->Passes[0].Pass;
    fx->Usage = mtrl->Passes[0].Usage;
    mtrl->Release();
  }

  return fx;
}

void __stdcall Exec_Effect_GeloetMesh(KOp *op,KEnvironment *kenv,sF32 animate,sInt ptIter,sInt edgeIter,sInt iters,sInt maxLen,sInt rndSeed,sF32 lineWidth,sF32 ptWidth)
{
  MeshGeloetEffect *fx = (MeshGeloetEffect *) op->Cache;
  GenMaterial *mtrl = (GenMaterial *) op->GetLinkCache(0);
  GenMaterial *mtrlPoint = (GenMaterial *) op->GetLinkCache(1);

  if(fx && mtrl && mtrl->Passes.Count > 0)
  {
    kenv->CurrentCam.ModelSpace = kenv->ExecStack.Top();
    sSystem->SetViewProject(&kenv->CurrentCam);

    for(sInt i=0;i<mtrl->Passes.Count;i++)
    {
      mtrl->Passes[i].Mtrl->Set(kenv->CurrentCam);
      fx->RenderLines(lineWidth,sRange<sF32>(animate,1.0f,0.0f));
    }

    if(mtrlPoint && mtrlPoint->Passes.Count > 0)
    {
      mtrlPoint->Passes[0].Mtrl->Set(kenv->CurrentCam);
      fx->RenderPoints(ptWidth);
    }
  }
}

/****************************************************************************/
