// This file is distributed under a BSD license. See LICENSE.txt for details.

vs.2.0
// material 2.0: light pass (vertex shader)

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
endflags

alias mWVP = c0
alias mMTex = c4
alias cLight = c8
alias cEye = c9
alias cRange = c10
alias cScales = c11
alias mTex1 = c12
alias mTex2 = c14

def c16,2,-1,0,0

// inputs
alias vPos = v0,vUV = v1,vNormal = v2,vTangent = v3,vColor = v4

dcl_position  vPos
dcl_texcoord0 vUV
dcl_normal    vNormal
dcl_tangent   vTangent
dcl_color0    vColor

// position, vertex color
m4x4  oPos,vPos,mWVP

// unpack tangent space basis
temp  N,B,T
mov   N,vNormal
vmov  T,vTangent
/*mad		N,vNormal,c16.x,c16.y
mad		T,vTangent,c16.x,c16.y*/

mul		B.xyz,N.yzx,T.zxy
mad		B.xyz,-N.zxy,T.yzx,B

// material pass texture 
m4x4  oT0,vPos,mMTex

// attenuation
mad   oT1.xyz,vPos,cRange.w,cRange

// light vector
temp  L
add   L.xyz,cLight,-vPos
dp3   oT2.x,L,N
dp3   oT2.y,L,B
dp3   oT2.z,L,T

// halfway vector (if required)
imov  i1,3
if Flags[0]
	temp	eye,H

	// normalize light vector
	dp3		L.w,L,L
	rsq		L.w,L.w
	mul		L.xyz,L,L.w

	// calc and normalize eye vector
	add		eye.xyz,cEye,-vPos
	dp3		eye.w,eye,eye
	rsq		eye.w,eye.w
	mul		eye.xyz,eye,eye.w

	// calc halfway vector and transform to texture space
	add		H.xyz,L,eye
	dp3		oT0+i1.x,H,N	
	dp3		oT0+i1.y,H,B
	dp3		oT0+i1.z,H,T
	iadd	i1,1
endif

// bump/detail bump texture coordinates
for i=0..1
  if LgtUsed[i][0]
    if LgtFlags[i][12..15] == 1 // scaled
      temp  scale
      vmov  scale,SrcScale[i]
      mul   oT0+i1.xy,vUV,scale
    elif LgtFlags[i][12..15] == 2 // trans1
      dp4   oT0+i1.x,vUV,c12
      dp4   oT0+i1.y,vUV,c13
    elif LgtFlags[i][12..15] == 3 // trans2
      dp4   oT0+i1.x,vUV,c14
      dp4   oT0+i1.y,vUV,c15
    else // untransformed
      mov   oT0+i1.xy,vUV
    endif
    
    iadd  i1,1
  endif
endfor

mov     oD0,vColor

//mov   oT0,v303
