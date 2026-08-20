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

}

#endif // FILE_WZ4FRLIB_WZ4_EXTRA_MESH_HPP
