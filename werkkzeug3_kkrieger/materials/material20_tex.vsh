// This file is distributed under a BSD license. See LICENSE.txt for details.

vs.2.0
// material 2.0: texture pass (vertex shader)

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

// inputs
alias vPos = v0,vUV = v1

dcl_position  vPos
dcl_texcoord0 vUV

// actual shader code: position
m4x4  oPos,vPos,mWVP

// actual shader code: texture coordinates
imov  i1,0

for i=0..3
  if TexUsed[i][0]
    if TexFlags[i][12..15] == 1 // scaled
      temp  scale
      vmov  scale,SrcScale[i]
      mul   oT0+i1.xy,vUV,scale
    elif TexFlags[i][12..15] == 2 // trans1
      dp4   oT0+i1.x,vUV,c8
      dp4   oT0+i1.y,vUV,c9
    elif TexFlags[i][12..15] == 3 // trans2
      dp4   oT0+i1.x,vUV,c10
      dp4   oT0+i1.y,vUV,c11
    else // untransformed
      mov   oT0+i1.xy,vUV
    endif
    
    iadd  i1,1
  endif
endfor
