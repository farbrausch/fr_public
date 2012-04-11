vs.2.0

// inputs
alias					vPos = v0, vUV = v1, vNormal = v2, vTangent = v3
dcl_position  vPos
dcl_texcoord  vUV
dcl_normal    vNormal
dcl_tangent   vTangent

// outputs
alias					oL = oT0, oH = oT1
alias					oUV = oT2, oLightProj = oT3, oLightFall = oT4
alias					oEye = oT5

// constants:
// c0-c3 = world-view-projection
// c4-c6 = light projection xform (mdl)
// c7    = light falloff xform    (mdl)
// c8    = camera                 (mdl)
// c9    = light pos              (mdl)

alias		mWVP = c0, mLightFall = c7
alias		cCamPos = c8, cLightPos = c9
def     c10,2.0,-1.0,0.0,0.5

// step 1: transform position, unpack basis
m4x4		oPos,vPos,mWVP

temp		N,B,T

mad			N,vNormal,c10.x,c10.y
mad			T,vTangent,c10.x,c10.y
mul			B,N.yzx,T.zxy
mad			B,-N.zxy,T.yzx,B

// step 2: mapping, light projections
mov			oUV.xy,vUV

dp4			oLightProj.x,vPos,c4
dp4			oLightProj.y,vPos,c5
mov			oLightProj.z,c10.z
dp4			oLightProj.w,vPos,c6

dp4			oLightFall.x,vPos,mLightFall
mov			oLightFall.y,c10.w

// step 3: tangent space lighting setup
temp		L,eye,normEye

add			L,cLightPos,-vPos
dp3			oL.x,L,N
dp3			oL.y,L,B
dp3			oL.z,L,T

if 3										// specular or paralax
	add			eye,cCamPos,-vPos
	nrm			normEye,eye
endif

if 0										// specular only
	temp		normL,H

	nrm			normL,L
	add			H,normL,normEye

	dp3			oH.x,H,N
	dp3			oH.y,H,B
	dp3			oH.z,H,T
endif

if 1										// paralax
	dp3			oEye.x,normEye,T
	dp3			oEye.y,normEye,B
endif
