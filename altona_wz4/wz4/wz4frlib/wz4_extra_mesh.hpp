/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_WZ4_EXTRA_MESH_HPP
#define FILE_WZ4FRLIB_WZ4_EXTRA_MESH_HPP

#include "base/types.hpp"
#include "base/math.hpp"
#include "wz4frlib/wz4_mesh.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Mesh operators the wz4 mesh does not otherwise have.               ***/
/***                                                                      ***/
/***   Everything here builds or edits a plain Wz4Mesh and leaves normals  ***/
/***   to the caller (the operators call CalcNormalAndTangents), so none   ***/
/***   of it needs a material or a device.                                ***/
/***                                                                      ***/
/****************************************************************************/

namespace Wz4Extra
{

/****************************************************************************/
/***   primitives                                                         ***/
/****************************************************************************/

// subdivided icosahedron: unlike a uv sphere it has no poles and its faces
// are all about the same size
void MakeIcosphere(Wz4Mesh *out,sInt subdiv,sF32 radius);

enum PlatonicType
{
  PLATONIC_TETRA = 0,
  PLATONIC_CUBE,
  PLATONIC_OCTA,
  PLATONIC_DODECA,
  PLATONIC_ICOSA,
};

void MakePlatonic(Wz4Mesh *out,sInt type,sF32 radius);

// (p,q) torus knot, as a tube. p turns around the axis while q turns through
// the hole; (2,3) is the trefoil.
void MakeTorusKnot(Wz4Mesh *out,sInt p,sInt q,sInt segments,sInt slices,
                   sF32 radius,sF32 ringradius,sF32 tuberadius,sBool caps);

// Gielis superformula, once for each of the two angles. Between them the two
// parameter sets cover most of the shapes people draw as "organic".
void MakeSupershape(Wz4Mesh *out,sInt segments,sInt slices,sF32 radius,
                    const sF32 *para0,const sF32 *para1);

// a helix, as a tube. taper scales the tube from one end to the other.
void MakeSpring(Wz4Mesh *out,sF32 turns,sF32 radius,sF32 height,sInt segments,
                sInt slices,sF32 tuberadius,sF32 taper,sBool caps);

// revolve a profile around the y axis. profile is count pairs of
// (radius,height), bottom to top.
void MakeLathe(Wz4Mesh *out,const sF32 *profile,sInt count,sInt segments,
               sF32 arc,sBool caps);

/****************************************************************************/
/***   modifiers                                                          ***/
/****************************************************************************/

enum ShellMode
{
  SHELL_OUTSIDE = 0,     // the new surface is pushed out, the original stays
  SHELL_INSIDE,          // the new surface is pushed in
  SHELL_CENTERED,        // both move by half, around the original
};

// give a surface a thickness: a second, inverted copy offset along the
// normals, plus a rim closing whatever open edges the surface had
void Shell(Wz4Mesh *mesh,sF32 thickness,sInt mode);

// relax vertices towards the average of their neighbours, without adding
// geometry. taubin alternates shrinking and expanding passes, which keeps the
// volume roughly where it was.
void Smooth(Wz4Mesh *mesh,sInt iterations,sF32 strength,sBool taubin,sInt selection);

enum UVProjectMode
{
  UVPROJECT_PLANAR = 0,  // straight down the projector's z
  UVPROJECT_CYLINDER,    // angle around the projector's y
  UVPROJECT_SPHERE,      // longitude and latitude
  UVPROJECT_BOX,         // whichever of the six directions the face points at
};

void UVProject(Wz4Mesh *mesh,sInt mode,sInt uvset,const sMatrix34 &mat,
               sF32 tilex,sF32 tiley,sF32 offsetx,sF32 offsety);

// merge vertices that sit closer together than threshold, drop the faces that
// collapse in the process
void Weld(Wz4Mesh *mesh,sF32 threshold,sBool recalcnormals);

// place copies of item over the surface of target, one per sampled point,
// area weighted so that big faces get their share
void ScatterOnMesh(Wz4Mesh *out,Wz4Mesh *target,Wz4Mesh *item,sInt count,
                   sInt seed,sF32 scalemin,sF32 scalemax,sF32 align,
                   sBool spin,sBool keeptarget);

/****************************************************************************/
/***   primitives, second batch                                           ***/
/****************************************************************************/

// a frustum: radius0 at the bottom, radius1 at the top. Either one may be 0,
// which is what makes it a cone.
void MakeCone(Wz4Mesh *out,sInt slices,sInt segments,sF32 radius0,sF32 radius1,
              sF32 height,sBool caps);

// cylinder with hemispherical ends. height is the straight part.
void MakeCapsule(Wz4Mesh *out,sInt slices,sInt segments,sInt capsegments,
                 sF32 radius,sF32 height);

// hollow cylinder: an outer and an inner wall, joined by two rings
void MakeTube(Wz4Mesh *out,sInt slices,sInt segments,sF32 outer,sF32 inner,
              sF32 height,sBool caps);

// a box whose edges and corners are rounded off by radius
void MakeRoundedBox(Wz4Mesh *out,sInt segments,sF32 sx,sF32 sy,sF32 sz,sF32 radius);

// spur gear: teeth around the rim, an optional bore through the middle
void MakeGear(Wz4Mesh *out,sInt teeth,sF32 outer,sF32 root,sF32 bore,
              sF32 thickness,sF32 toothwidth);

// extruded star polygon
void MakeStar(Wz4Mesh *out,sInt points,sF32 outer,sF32 inner,sF32 bore,
              sF32 thickness);

// a strip with a half twist in it, or more than one
void MakeMobius(Wz4Mesh *out,sInt segments,sInt slices,sF32 radius,sF32 width,
                sInt twists);

// a screw thread: a triangular profile revolved while it climbs
void MakeScrew(Wz4Mesh *out,sInt segments,sF32 turns,sF32 radius,sF32 depth,
               sF32 pitch,sF32 shoulder,sBool caps);

// skin between two closed cross sections, one at each end. Both are count
// pairs of (x,z), and they have to have the same number of points -- which
// point pairs up with which is what a loft is.
void MakeLoft(Wz4Mesh *out,const sF32 *profile0,const sF32 *profile1,sInt count,
              sInt steps,sF32 height,sBool caps);

// sweep a closed 2D profile along a 3D path. profile is count pairs of (x,y)
// in the frame the path carries, path is pathcount xyz triples.
void MakeSweep(Wz4Mesh *out,const sF32 *profile,sInt count,const sF32 *path,
               sInt pathcount,sBool closed,sBool caps,sF32 twist);

/****************************************************************************/
/***   modifiers, second batch                                            ***/
/****************************************************************************/

// rotate around an axis by an angle that grows along it -- the one deform
// TransformNonLinear cannot reach, because it needs a sine of a coordinate
void Twist(Wz4Mesh *mesh,sInt axis,sF32 angle,sF32 center,sInt selection);

// scale across an axis by an amount that grows along it
void Taper(Wz4Mesh *mesh,sInt axis,sF32 amount,sF32 center,sInt selection);

// shrink every face towards its own centre and bridge the gap, which is what
// panelling, tiling and greebles are built out of
void Inset(Wz4Mesh *mesh,sF32 amount,sF32 offset,sInt selection);

// detach every face and move it away, along its normal or out from a point
void ExplodeFaces(Wz4Mesh *mesh,sF32 distance,sF32 fromcenter,sF32 scale,
                  sF32 randomize,sInt seed,sInt selection);

// pull a point out of every face and fan the face to it
void SpikeFaces(Wz4Mesh *mesh,sF32 height,sF32 inset,sF32 randomize,sInt seed,
                sInt selection);

// turn every face into a frame: keep a border of the given width, drop the
// middle, and optionally give the frame some depth
void Wireframe(Wz4Mesh *mesh,sF32 width,sF32 depth,sInt selection);

// the convex hull of the mesh's vertices
void ConvexHull(Wz4Mesh *mesh);

// vertex clustering: collapse everything inside a grid cell to one vertex.
// Not a quadric error simplifier -- it is much cruder and much faster, and it
// keeps the silhouette rather than the detail.
void Simplify(Wz4Mesh *mesh,sF32 cellsize,sBool recalcnormals);

// recompute normals, averaging across an edge only when the two faces meet at
// less than the given angle. Vertices get split where they have to.
void SmoothNormals(Wz4Mesh *mesh,sF32 angle);

// bake occlusion per vertex. Approximate: it weighs up the neighbourhood of
// each vertex rather than tracing rays. This tree has no vertex colours
// (WZ4MESH_LOWMEM), so target picks between the second uv set and the
// selection -- both of which something downstream can read.
void VertexAO(Wz4Mesh *mesh,sF32 radius,sF32 strength,sInt target);

/****************************************************************************/

}

#endif // FILE_WZ4FRLIB_WZ4_EXTRA_MESH_HPP
