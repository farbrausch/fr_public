// This file is distributed under a BSD license. See LICENSE.txt for details.

vs.2.0
// material 2.0: zfill pass (vertex shader)

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
alias mTex1a = c4
alias mTex1b = c5
alias mTex2a = c6
alias mTex2b = c7
alias tScale = c4

// inputs
alias vPos = v0,vUV = v1

dcl_position  vPos
dcl_texcoord0 vUV

// position
m4x4  oPos,vPos,mWVP

// uv
if TexUsed[3][0]
  if TexFlags[3][12..15] == 1 // scaled
    mul   oT0.xy,vUV,tScale.x
  elif TexFlags[3][12..15] == 2 // trans1
    dp4   oT0.x,vUV,mTex1a
    dp4   oT0.y,vUV,mTex1b
  elif TexFlags[3][12..15] == 3 // trans2
    dp4   oT0.x,vUV,mTex2a
    dp4   oT0.y,vUV,mTex2b
  else // untransformed
    mov   oT0.xy,vUV
  endif
endif
