// This file is distributed under a BSD license. See LICENSE.txt for details.

vs.1.1

// constants / flag data layout
def	c24,0.5,-0.5,0,1
def	c25,2,-1,1,0.001
def c26,256,0,0,0

flags
	BaseFlags
	SpecialFlags
	LightFlags
	CodegenFlags
	TFlags[4]
	Combiner[13]
	_pad[2]
	AlphaCombiner
  TexUsed[4]
  AlphaOps[2]
  CombinerSrc[13]
  AlphaSrc[13]
endflags

// inputs
alias	vPos = v0,vUV0 = v1,vUV1 = v2,vNormal = v3,vTangent = v4
alias	vColor = v5
temp modelPos

dcl_position vPos
if BaseFlags[18] // shadow extrusion
  if CodegenFlags[18..19] == 2 // shader instancing on
    dcl_color0  vColor
  endif
	add	modelPos,vPos.w,-c24.w
	mad	modelPos,c4,modelPos,v0
else
	if !BaseFlags[16] // texture present
		dcl_texcoord0	vUV0
	endif
	if CodegenFlags[1] // uv1 present
		dcl_texcoord1	vUV1
	endif
	if !BaseFlags[17] // normals present
		dcl_normal	vNormal
		dcl_tangent	vTangent
	endif
	if CodegenFlags[0] // vertex colors
		dcl_color0 	vColor
		mov					oD0,vColor
	endif

	vmov 	modelPos,v0
endif

if CodegenFlags[18..19] == 2 // shader instancing on
  temp  transformedPos,addr
  
  if !BaseFlags[18]
    if !CodegenFlags[0]
      dcl_color0  vColor
    endif
  endif
  
  mul   addr.x,vColor.a,c26.x
  mov   a0.x,addr
  
  dp4   transformedPos.x,modelPos,c27+a0x
  dp4   transformedPos.y,modelPos,c28+a0x
  dp4   transformedPos.z,modelPos,c29+a0x
  mov   transformedPos.w,modelPos
  vmov  modelPos,transformedPos
endif

// position
dp4   oPos.x,modelPos,c0
dp4   oPos.y,modelPos,c1
dp4   oPos.z,modelPos,c2
dp4   oPos.w,modelPos,c3
//m4x4	oPos,modelPos,c0

// fogging
if BaseFlags[6]
	dp4		oFog,modelPos,c20
endif

// unpack tangent space basis
temp	N,B,T

if !BaseFlags[17]
  if CodegenFlags[18..19] == 1 // normals and tangents not normalized or orthogonal
    dp3   N.w,vNormal,vNormal
    rsq   N.w,N.w
    mul   N.xyz,vNormal,N.w
    dp3   N.w,N,vTangent
    mad   T.xyz,N,-N.w,vTangent
    dp3   T.w,T,T
    rsq   T.w,T.w
    mul   T.xyz,T,T.w
  elif CodegenFlags[18..19] == 2 // shader instancing on
    dp3   N.x,vNormal,c27+a0x
    dp3   N.y,vNormal,c28+a0x
    dp3   N.z,vNormal,c29+a0x
    mov   N.w,c27
    dp3   T.x,vTangent,c27+a0x
    dp3   T.y,vTangent,c28+a0x
    dp3   T.z,vTangent,c29+a0x
    mov   T.w,c27
  else
    vmov  N,vNormal
    mov   T,vTangent
  endif

	mul		B.xyz,N.yzx,T.zxy
	mad		B.xyz,-N.zxy,T.yzx,B
endif

// texture coordinates
imov	i1,0

for i = 0..3
	if TexUsed[i][0]
		temp	inUV,eyeNormal,eyePos

		if TFlags[i][4..5] == 0 // uv0
			vmov	inUV,vUV0
    elif TFlags[i][4..5] == 1 // uv1
      vmov	inUV,vUV1
    endif

		if i == 2 // envi stage
			if SpecialFlags[4..5] == 1 // spherical envi
				dp3		eyeNormal.x,N,c12
				dp3		eyeNormal.y,N,c13
				mad		inUV.xy,eyeNormal,c24,c24.x
			elif SpecialFlags[4..5] == 3 // use normal as uv (for debris water lighting)
				mov		inUV.xyz,N
			elif SpecialFlags[4..5] == 2 // reflection envi
				dp4		eyePos.x,modelPos,c12
				dp4		eyePos.y,modelPos,c13
				dp4		eyePos.z,modelPos,c14
				dp3		eyeNormal.x,N,c12
				dp3		eyeNormal.y,N,c13
				dp3		eyeNormal.z,N,c14

				// normalize eye pos vector, calc 2/||eyeNormal||
				dp3		eyePos.w,eyePos,eyePos
				rsq		eyePos.w,eyePos.w
				mul		eyePos,eyePos,eyePos.w

				dp3		eyeNormal.w,eyeNormal,eyeNormal
				rcp		eyeNormal.w,eyeNormal.w
				add		eyeNormal.w,eyeNormal,eyeNormal

				// reflection vector calc
				dp3		inUV.w,eyePos,eyeNormal
				mul		inUV.w,inUV,eyeNormal
				mad		inUV.xyz,eyeNormal,inUV.w,-eyePos

				// rescale
				mad		inUV.xy,inUV,c24,c24.x
			endif
    endif

		if i == 3 // projection stage
			if SpecialFlags[8..11] == 1 // model projection
				vmov	inUV,modelPos
			elif SpecialFlags[8..11] == 2 // world projection
			  dp4   inUV.x,modelPos,c16
			  dp4   inUV.y,modelPos,c17
			  dp4   inUV.z,modelPos,c18
				//m4x3	inUV.xyz,modelPos,c16
				mov		inUV.w,c24.w
			elif SpecialFlags[8..11] == 3 // eye projection
			  dp4   inUV.x,modelPos,c16
			  dp4   inUV.y,modelPos,c17
			  dp4   inUV.z,modelPos,c18
				//m4x3	inUV.xyz,modelPos,c16
				mov		inUV.w,c24.w
			endif
		endif

		// texture transforms
		if TFlags[i][12..15] == 0 // direct uv
			mov		oT0+i1.xy,inUV
		elif TFlags[i][12..15] == 1 // scale
			if i == 0
				mul		oT0+i1.xy,inUV,c7.x
			elif i == 1
				mul		oT0+i1.xy,inUV,c7.y
			elif i == 2
				mul		oT0+i1.xy,inUV,c7.z
			else
				mul		oT0+i1.xy,inUV,c7.w
			endif
		elif TFlags[i][12..15] == 2 // trans1
			dp4		oT0+i1.x,inUV,c8
			dp4		oT0+i1.y,inUV,c9
		elif TFlags[i][12..15] == 3 // trans2
			dp4		oT0+i1.x,inUV,c10
			dp4		oT0+i1.y,inUV,c11
		endif

		iadd	i1,1
	endif
endfor

// ps-level-dependent part
if CodegenFlags[8..9] == 0 // ps0.0
	temp	vertColor

	imov	i0,0

	// color combiner 0
	if Combiner[1][4..7] == 1 // set
		vmov	vertColor,c22
		imov	i0,1
	elif Combiner[1][4..7] != 0 // anything except nop
		error
	endif

	if Combiner[2][4..7] == 0 // nop
	elif Combiner[2][4..7] == 1 // set
		vmov	vertColor,c23
		imov	i0,1
	elif i0 == 1 // other vertex color present
		if Combiner[2][4..7] == 2 // add
			add		vertColor,vertColor,c23
		elif Combiner[2][4..7] == 3 // sub
			add		vertColor,vertColor,-c23
		elif Combiner[2][4..7] == 4 // mul
			mul		vertColor,vertColor,c23
		endif
	else
		error
	endif

	if i0 == 1
		mov		oD0,vertColor
	endif
else // ps1.1/1.4/2.0
	if LightFlags[0..2] == 5 // bumpx, perform tangent space lighting setup
		temp	L

		// tangent space light vector
		if CodegenFlags[16..17] == 0 // point light
		  add		L.xyz,c4,-modelPos
		elif CodegenFlags[16..17] == 1 // directional light
		  mov  L,c4 // can't use vmov here...
		endif
		dp3		oT0+i1.x,L,N
		dp3		oT0+i1.y,L,B
		dp3		oT0+i1.z,L,T
		iadd	i1,1

		if !SpecialFlags[6] // specular?
			temp	eye,H

			// normalize light vector
			dp3		L.w,L,L
			rsq		L.w,L.w
			mul		L.xyz,L,L.w

			// calc and normalize eye vector
			add		eye.xyz,c6,-modelPos
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

		// attenuation
		mad		oT0+i1.xyz,modelPos,c5.w,c5
	endif
endif
