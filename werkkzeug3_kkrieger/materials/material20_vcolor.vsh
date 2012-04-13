// This file is distributed under a BSD license. See LICENSE.txt for details.

vs.2.0
// material 2.0: vertex color pass (vertex shader)

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
alias cColor = c4

// inputs
alias vPos = v0

dcl_position  vPos

// as simple as it gets
m4x4  oPos,vPos,mWVP
mov   oD0,cColor

