// This file is distributed under a BSD license. See LICENSE.txt for details.

vs.2.0

// this shader is so simple that I don't bother with defining aliases
dcl_position  v0
dcl_texcoord0 v1
dcl_texcoord1 v2

m4x4    oPos,v0,c0
add     oT0,v1,c4
mov     oT1,v2
