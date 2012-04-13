// This file is distributed under a BSD license. See LICENSE.txt for details.

vs.2.0
// material 2.0: fat shader (vertex shader)

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
alias cLight = c4
alias cEye = c5
alias cRange = c6
alias cScale1 = c7
alias mTex1a = c8
alias mTex1b = c9
alias mTex2a = c10
alias mTex2b = c11
alias cScale2 = c12

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
/*mad		N,vNormal,c16.x,c16.y
mad		T,vTangent,c16.x,c16.y
mov   N,vNormal
vmov  T,vTangent*/

mul		B.xyz,N.yzx,T.zxy
mad		B.xyz,-N.zxy,T.yzx,B

// attenuation
mad   oT0.xyz,vPos,cRange.w,cRange

// light vector
temp  L
add   L.xyz,cLight,-vPos
dp3   oT1.x,L,N
dp3   oT1.y,L,B
dp3   oT1.z,L,T

// halfway vector (if required)
imov  i1,2
if Flags[0]
  temp  eye,H

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

// bump/detail bump coords
for i=0..1
  if LgtUsed[i][0]
    if LgtFlags[i][12..15] == 1 // scaled
      temp  scale
      vmov  scale,SrcScale2[i]
      mul   oT0+i1.xy,vUV,scale
    elif LgtFlags[i][12..15] == 2 // trans1
      dp4   oT0+i1.x,vUV,mTex1a
      dp4   oT0+i1.y,vUV,mTex1b
    elif LgtFlags[i][12..15] == 3 // trans2
      dp4   oT0+i1.x,vUV,mTex2a
      dp4   oT0+i1.y,vUV,mTex2b
    else // untransformed
      mov   oT0+i1.xy,vUV
    endif
    
    iadd  i1,1
  endif
endfor

// texture coordinates
for i=0..2
  if TexUsed[i][0]
    if TexFlags[i][12..15] == 1 // scaled
      temp  scale
      vmov  scale,SrcScale[i]
      mul   oT0+i1.xy,vUV,scale
    elif TexFlags[i][12..15] == 2 // trans1
      dp4   oT0+i1.x,vUV,mTex1a
      dp4   oT0+i1.y,vUV,mTex1b
    elif TexFlags[i][12..15] == 3 // trans2
      dp4   oT0+i1.x,vUV,mTex2a
      dp4   oT0+i1.y,vUV,mTex2b
    else // untransformed
      mov   oT0+i1.xy,vUV
    endif
    
    iadd  i1,1
  endif
endfor
