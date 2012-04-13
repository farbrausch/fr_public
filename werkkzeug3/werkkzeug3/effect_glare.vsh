// This file is distributed under a BSD license. See LICENSE.txt for details.

vs.2.0

dcl_position    v0
dcl_texcoord0   v1

m4x4    oPos,v0,c0
add     oT0.xy,v1,c4.xy
add     oT1.xy,v1,c4.zw
add     oT2.xy,v1,c5.xy
add     oT3.xy,v1,c5.zw
add     oT4.xy,v1,c6.xy
add     oT0.zw,v1.yxyx,c6.wzwz
add     oT1.zw,v1.yxyx,c7.yxyx
add     oT2.zw,v1.yxyx,c7.wzwz
add     oT3.zw,v1.yxyx,c8.yxyx
