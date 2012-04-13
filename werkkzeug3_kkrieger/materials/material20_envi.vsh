// This file is distributed under a BSD license. See LICENSE.txt for details.

vs.2.0
// material 2.0: envi pass (vertex shader)

// flag data layout, constants
flags
	Flags
	_pad[7]
	
	TexFlags[4]
	TexScale[4]
	
	LgtFlags[4]
	LgtScale[4]
	
	SRT1[9]
	SRT2[5]
	
	TexUsed[4]
	LgtUsed[4]
	SrcScale[4]
	SrcScale2[4]
endflags

alias mWVP = c0
alias mModel = c4
alias cCam = c8

def c16,2,-1,0,0

// inputs
alias vPos = v0,vUV = v1,vNormal = v2,vTangent = v3

dcl_position  vPos
dcl_texcoord0 vUV
dcl_normal    vNormal
dcl_tangent   vTangent

// position
m4x4  oPos,vPos,mWVP

// unpack tangent space basis
temp  N,B,T
vmov  N,vNormal
mov   T,vTangent
/*mov   N,vNormal
vmov  T,vTangent*/

mul		B.xyz,N.yzx,T.zxy
mad		B.xyz,-N.zxy,T.yzx,B

// normal (=x basis vector)
m3x3  oT0.xyz,N,mModel

// eye
temp  eye
add   eye,cCam,-vPos
m3x3  oT1.xyz,eye,mModel

// uv, y/z basis vector (only if bumped)
if LgtFlags[2][17]
  mov   oT2.xy,vUV
  m3x3  oT3.xyz,B,mModel
  m3x3  oT4.xyz,T,mModel
endif
