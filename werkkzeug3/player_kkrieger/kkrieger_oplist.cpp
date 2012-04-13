// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "kdoc.hpp"
#include "genbitmap.hpp"
#include "genmaterial.hpp"
#include "genmesh.hpp"
#include "genoverlay.hpp"
#include "genscene.hpp"
#include "geneffect.hpp"
#include "kkriegergame.hpp"

 
KHandler KHandlers[] =
{
  // misc
  0x01, Init_Misc_Load,         Exec_Misc_Load,             // load
  0x02, Init_Misc_Nop,          Exec_Misc_Nop,              // store
  0x03, Init_Misc_Nop,          Exec_Misc_Nop,              // nop
  0x05, Init_Misc_Nop,          Exec_Misc_Time,             // time
  0x06, Init_Misc_Event,        Exec_Misc_Event,            // event
  0x07, Init_Misc_Trigger,      Exec_Misc_Trigger,          // trigger
  0x08, Init_Misc_Nop,          Exec_Misc_Delay,            // delay
  0x09, Init_Misc_Nop,          Exec_Misc_If,               // if
  0x0a, Init_Misc_Nop,          Exec_Misc_SetIf,            // setif
  0x0b, Init_Misc_Nop,          Exec_Misc_State,            // state
  0x0c, Init_Misc_Nop,          Exec_Misc_PlaySample,       // play sample
  0x0d, Init_Misc_Demo,         Exec_Misc_Demo,             // demo 
  0x0e, Init_Misc_Nop,          Exec_Misc_Menu,             // menu

  // kkrieger
  0x10, Init_KKrieger_Para,     Exec_KKrieger_Para,         // KKriegerPara
  0x11, Init_KKrieger_Monster,  Exec_KKrieger_Monster,      // KKriegerMonster
  0x12, Init_KKrieger_Events,   Exec_KKrieger_Events,       // KKriegerEvents
  0x13, Init_KKrieger_Respawn,  Exec_KKrieger_Respawn,
  0x14, Init_KKrieger_SplashDamage,  Exec_KKrieger_SplashDamage,
  0x15, Init_KKrieger_MonsterFlags,  Exec_KKrieger_MonsterFlags,

  // bitmap
  0x21, Bitmap_Flat,            Exec_Misc_Nop,              // flat
  0x22, Bitmap_Perlin,          Exec_Misc_Nop,              // perlin
  0x23, Bitmap_Color,           Exec_Misc_Nop,              // color
  0x24, Bitmap_Merge,           Exec_Misc_Nop,              // merge
  0x25, Bitmap_Format,          Exec_Misc_Nop,              // format
  0x26, Bitmap_RenderTarget,    Exec_Misc_Nop,              // render target
  0x27, Bitmap_GlowRect,        Exec_Misc_Nop,              // glowrect (old)
  0x28, Bitmap_Dots,            Exec_Misc_Nop,              // dots
  0x29, Bitmap_Blur,            Exec_Misc_Nop,              // blur
  0x2a, Bitmap_Mask,            Exec_Misc_Nop,              // mask
  0x2b, Bitmap_HSCB,            Exec_Misc_Nop,              // hscb
  0x2c, Bitmap_Rotate,          Exec_Misc_Nop,              // rotate
  0x2d, Bitmap_Distort,         Exec_Misc_Nop,              // distort
  0x2e, Bitmap_Normals,         Exec_Misc_Nop,              // normals
  0x2f, Bitmap_Light,           Exec_Misc_Nop,              // light
  0x30, Bitmap_Bump,            Exec_Misc_Nop,              // bump
  0x31, Bitmap_Text,            Exec_Misc_Nop,              // text
  0x32, Bitmap_Cell,            Exec_Misc_Nop,              // cell
  0x34, Bitmap_Gradient,        Exec_Misc_Nop,              // gradient
  0x35, Bitmap_Range,           Exec_Misc_Nop,              // range
  0x36, Bitmap_RotateMul,       Exec_Misc_Nop,              // rotatemul
  0x37, Bitmap_Twirl,           Exec_Misc_Nop,              // twirl
  0x38, Bitmap_Sharpen,         Exec_Misc_Nop,              // sharpen
  0x39, Bitmap_GlowRect,        Exec_Misc_Nop,              // glowrect
  0x3b, Bitmap_ColorBalance,    Exec_Misc_Nop,              // color balance

  // effecte
//  0x60, Init_Effect_Dummy,      Exec_Effect_Dummy,          // dummy-effect
  0x61, Init_Effect_Print,      Exec_Effect_Print,          // print text/menu
  0x63, Init_Effect_PartEmitter,Exec_Effect_PartEmitter,    // particles
  0x64, Init_Effect_PartSystem, Exec_Effect_PartSystem,     // particles

  // mesh
  0x81, Mesh_Cube,              Exec_Misc_Nop,              // cube
  0x82, Mesh_Cylinder,          Exec_Misc_Nop,              // cylinder
//  0x83, Mesh_Torus,             Exec_Misc_Nop,              // torus
//  0x84, Mesh_Sphere,            Exec_Misc_Nop,              // sphere
//  0x85, Mesh_SelectAll,         Exec_Misc_Nop,              // select all
  0x86, Mesh_SelectCube,        Exec_Misc_Nop,              // select cube
  0x87, Mesh_Subdivide,         Exec_Misc_Nop,              // subdivide
  0x88, Mesh_Transform,         Exec_Misc_Nop,              // transform
  0x89, Mesh_TransformEx,       Exec_Misc_Nop,              // transformex
  0x8a, Mesh_Crease,            Exec_Misc_Nop,              // crease
//  0x8b, Mesh_UnCrease,          Exec_Misc_Nop,              // uncrease
  0x8c, Mesh_Triangulate,       Exec_Misc_Nop,              // triangulate
//  0x8d, Mesh_Cut,               Exec_Misc_Nop,              // cut
//  0x8e, Mesh_ExtrudeNormal,     Exec_Misc_Nop,              // extrudenormal
  0x8f, Mesh_Displace,          Exec_Misc_Nop,              // displace
// bevel ONLY used in full level!
  0x90, Mesh_Bevel,             Exec_Misc_Nop,              // bevel
  0x91, Mesh_Perlin,            Exec_Misc_Nop,              // perlin
  0x92, Mesh_Add,               Exec_Misc_Nop,              // add
// deletefaces ONLY used in full level! (and kkrieger-logo)
  0x93, Mesh_DeleteFaces,       Exec_Misc_Nop,              // delete faces
//  0x94, Mesh_SelectRandom,      Exec_Misc_Nop,              // select random
  0x95, Mesh_Multiply,          Exec_Misc_Nop,              // multiply
  0x96, Mesh_MatLink,           Exec_Misc_Nop,              // matlink
//  0x99, Mesh_BeginRecord,       Exec_Misc_Nop,              // beginrec
  0x9a, Mesh_Extrude,           Exec_Misc_Nop,              // extrude
  0x9c, Mesh_CollisionCube,     Exec_Misc_Nop,              // collcube
// grid ONLY used in full level! (and kkrieger-logo)
  0x9d, Mesh_Grid,              Exec_Misc_Nop,              // grid
  0xa0, Mesh_Bend,              Exec_Misc_Nop,              // bend
//  0xa1, Mesh_SelectLogic,       Exec_Misc_Nop,              // select logic
//  0xa2, Mesh_SelectGrow,        Exec_Misc_Nop,              // select grow
//  0xa3, Mesh_Invert,            Exec_Misc_Nop,              // invert
//  0xa4, Mesh_SetPivot,          Exec_Misc_Nop,              // set pivot
  0xa5, Mesh_UVProjection,      Exec_Misc_Nop,              // uvproject
  0xa6, Mesh_Center,            Exec_Misc_Nop,              // center
  0xa7, Mesh_AutoCollision,     Exec_Misc_Nop,              // autocoll
  0xa8, Mesh_SelectSphere,      Exec_Misc_Nop,              // selectsphere
//  0xa9, Mesh_SelectFace,        Exec_Misc_Nop,              // selectface
//  0xaa, Mesh_SelectAngle,       Exec_Misc_Nop,              // selectangle
  0xab, Mesh_Bend2,             Exec_Misc_Nop,              // bend 2
//  0xac, Mesh_SmoothAngle,       Exec_Misc_Nop,              // smangle
  0xad, Mesh_Color,             Exec_Misc_Nop,              // color
//  0xae, Mesh_BendS,             Exec_Misc_Nop,              // bend spline
  0xaf, Mesh_LightSlot,         Exec_Misc_Nop,              // light slot
  0xb0, Mesh_ShadowEnable,      Exec_Misc_Nop,              // shadowenable
  0xb3, Mesh_SingleVert,        Exec_Misc_Nop,              // single vertex

  // scene
  0xc0, Init_Scene_Scene,       Exec_Scene_Scene,           // scene
  0xc1, Init_Scene_Add,         Exec_Scene_Add,             // add
  0xc2, Init_Scene_Multiply,    Exec_Scene_Multiply,        // multiply
  0xc3, Init_Scene_Transform,   Exec_Scene_Transform,       // transform
  0xc4, Init_Scene_Light,       Exec_Scene_Light,           // light
  0xc5, Init_Scene_Particles,   Exec_Scene_Particles,       // scene multply particles
  0xc6, Init_Scene_Camera,      Exec_Scene_Camera,          // camera

  0xc7, Init_Scene_Limb,        Exec_Scene_Limb,            // limb
  0xc8, Init_Scene_Walk,        Exec_Scene_Walk,            // walk
  0xc9, Init_Scene_Rotate,      Exec_Scene_Rotate,          // rotate
//  0xca, Init_Scene_Forward,     Exec_Scene_Forward,         // forward
  0xcb, Init_Scene_Sector,      Exec_Scene_Sector,          // sector
  0xcd, Init_Scene_Portal,      Exec_Scene_Portal,          // portal
  0xce, Init_Scene_Physic,      Exec_Scene_Physic,          // physic
  0xce, Init_Scene_Physic,      Exec_Scene_Physic,          // physic

  // material
  0xd0, Init_Material_Material, Exec_Material_Material,     // material
  0xd1, Material_Add,           Exec_Misc_Nop,              // add material
  0xd2, Init_Scene_MatHack,     Exec_Scene_MatHack,         // link material for animation

  // ipp
  0xe2, Init_IPP_Copy,          Exec_IPP_Copy,              // copy
  0xe3, Init_IPP_Blur,          Exec_IPP_Blur,              // blur
//  0xe4, Init_IPP_Crashzoom,     Exec_IPP_Crashzoom,         // crashzoom
//  0xe5, Init_IPP_Sharpen,       Exec_IPP_Sharpen,           // sharpen
  0xe6, Init_IPP_Color,         Exec_IPP_Color,             // color
  0xe7, Init_IPP_Merge,         Exec_IPP_Merge,             // merge
  0xe8, Init_IPP_Mask,          Exec_IPP_Mask,              // mask
//  0xe9, Init_IPP_Layer2D,       Exec_IPP_Layer2D,           // layer2d
  0xea, Init_IPP_Select,        Exec_IPP_Select,            // select
  0xeb, Init_IPP_RenderTarget,  Exec_IPP_RenderTarget,      // render target
  0xf0, Init_IPP_Viewport,      Exec_IPP_Viewport,          // viewport

  // minmesh
//  0x100, MinMesh_SingleVert,    Exec_Misc_Nop,              // single vert
//  0x101, MinMesh_Grid,          Exec_Misc_Nop,              // grid
//  0x102, MinMesh_Cube,          Exec_Misc_Nop,              // cube
//  0x103, MinMesh_Torus,         Exec_Misc_Nop,              // torus
//  0x104, MinMesh_Sphere,        Exec_Misc_Nop,              // sphere
//  0x105, MinMesh_Cylinder,      Exec_Misc_Nop,              // cylinder
//  0x106, MinMesh_XSI,           Exec_Misc_Nop,              // xsi

//  0x110, MinMesh_MatLink,       Exec_Misc_Nop,              // matlink
//  0x111, MinMesh_Add,           Exec_Misc_Nop,              // add
//  0x112, MinMesh_SelectAll,     Exec_Misc_Nop,              // select all
//  0x113, MinMesh_SelectCube,    Exec_Misc_Nop,              // select cube
//  0x114, MinMesh_DeleteFaces,   Exec_Misc_Nop,              // delete faces
//  0x115, MinMesh_Invert,        Exec_Misc_Nop,              // invert

//  0x120, MinMesh_TransformEx,   Exec_Misc_Nop,              // transform
//  0x121, MinMesh_TransformEx,   Exec_Misc_Nop,              // transform ex
//  0x122, MinMesh_ExtrudeNormal, Exec_Misc_Nop,              // extrude normal
//  0x123, MinMesh_Displace,      Exec_Misc_Nop,              // displace
//  0x124, MinMesh_Perlin,        Exec_Misc_Nop,              // perlin
//  0x125, MinMesh_Bend2,         Exec_Misc_Nop,              // bend 2

  // end!
  0,0,0,
};
