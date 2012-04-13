// This file is distributed under a BSD license. See LICENSE.txt for details.

#pragma once

#ifndef __ASCBASE_HPP__
#define __ASCBASE_HPP__

#include "_start.hpp"

/****************************************************************************/

struct sVertexFormatHandle; // isn't defined anywhere, we don't use it in wz3

#define sRENDER_BLANK     0       // blank screen, no rendering
#define sRENDER_DX9       1       // dx9 rendering
#define sRENDER_OGL2      2       // >=opengl 2.0 rendering
#define sRENDER_DX10      3       // >=dx10 rendering (vista required)
#define sRENDER_PS3GL     4       // ps3 psgl 
#define sRENDER_PS2       5
#define sRENDER_WII       6       // wii GX
#define sRENDER_XBOX360   7

#define sRENDERER         sRENDER_DX9

/****************************************************************************/
/***                                                                      ***/
/***   Constant buffers                                                   ***/
/***                                                                      ***/
/****************************************************************************/

enum
{
  sCBUFFER_MAXSLOT = 32,
  sCBUFFER_SHADERTYPES = 3,
  sCBUFFER_SHADERMASK = sCBUFFER_MAXSLOT*3,
  sCBUFFER_VS = sCBUFFER_MAXSLOT*0,
  sCBUFFER_PS = sCBUFFER_MAXSLOT*1,
  sCBUFFER_GS = sCBUFFER_MAXSLOT*2,
};

class sCBufferBase                           // do not use this class directly!
{
public:
  sCBufferBase();                           // create data and lock
  void SetRegs();                           // unlock and actually set constant registers
  void SetPtr(void **dataptr,void *data);
  void Modify();                            // relock new copy of buffer
  void OverridePtr(void *);                 // reload cb with data from here
  ~sCBufferBase();                          // destroy data

  sInt RegStart;                            // starting register
  sInt RegCount;                            // register count
  sInt Slot;                                // dedicated slot & shader type

protected:
  void *DataPersist;
  void **DataPtr;
private:
};

template <class Type>                       // you are supposed to create this as member variable or directly on stack
class sCBuffer : public sCBufferBase
{
  Type Storage;                             // on most platforms: copy to command buffer
public:
  sCBuffer() { RegStart=Type::RegStart; RegCount=Type::RegCount; Slot=Type::Slot; SetPtr((void**)&Data,(void*)&Storage); }
  Type *Data;
};

void sResetConstantBuffers();

/****************************************************************************/
/***                                                                      ***/
/***   Material                                                           ***/
/***                                                                      ***/
/****************************************************************************/

enum sMaterialConsts
{
  sMTRL_MAXTEX = 8,
};

class sASCMaterial : public sMaterial
{
protected:
  sInt Setup;

  void AddMtrlFlags(sU32 *&data);

public:
  sASCMaterial();
  virtual ~sASCMaterial();

  virtual void Prepare(sVertexFormatHandle *vf)=0;          // prepare the material
  virtual void Set(const sMaterialEnv &env) {}              // set material parameters depending on MaterialEnv
  void Set(sCBufferBase **cbuffers,sInt cbcount);
  void Set(sCBufferBase *a)                                                 { Set(&a,1); }
  void Set(sCBufferBase *a,sCBufferBase *b)                                 { sCBufferBase *A[2] = {a,b};     Set(A,2); }
  void Set(sCBufferBase *a,sCBufferBase *b,sCBufferBase *c)                 { sCBufferBase *A[3] = {a,b,c};   Set(A,3);  }
  void Set(sCBufferBase *a,sCBufferBase *b,sCBufferBase *c,sCBufferBase *d) { sCBufferBase *A[4] = {a,b,c,d}; Set(A,4);  }

  sInt Texture[sMTRL_MAXTEX];

  sU32 NameId;
  sU32 Flags;                               // sMTRL_?? flags
  sU8 FuncFlags[4];                         // FuncFlags[sMFT_??] = sMFF_?? flags for depth test [0], alpha test [1] and stencil test [2]
  sU32 TFlags[sMTRL_MAXTEX];                // sMTF_?? flags
  sU32 BlendColor;                          // sMB?_?? flags for color
  sU32 BlendAlpha;                          // sMB?_?? flags for alpha or sMB_SAMEASCOLOR
  sU32 BlendFactor;                         // blendfactor 
  sU8 StencilOps[6];                        // sMSO_??? two sided stencil op flags for FAIL, ZFAIL, PASS (CW, CCW)
  sU8 StencilRef;														// the stencil reference value
  sU8 StencilMask;                          //
  sU8 AlphaRef;                             // reference for alpha test
};

enum sMaterialFlags
{
  // misc

  sMTRL_ZOFF          = 0x0000,
  sMTRL_ZREAD         = 0x0001,
  sMTRL_ZWRITE        = 0x0002,
  sMTRL_ZON           = 0x0003,
  sMTRL_ZMASK         = 0x0003,

  sMTRL_CULLOFF       = 0x0000,             // no culling, doublesided
  sMTRL_CULLON        = 0x0010,             // normal culling
  sMTRL_CULLINV       = 0x0020,             // inverted culling
  sMTRL_CULLMASK      = 0x0030,

  sMTRL_LIGHTING      = 0x0080,             // enable light calculation!

  // color write mask

  sMTRL_MSK_ALPHA     = 0x0100,             // color write mask
  sMTRL_MSK_RED       = 0x0200, 
  sMTRL_MSK_GREEN     = 0x0400,
  sMTRL_MSK_BLUE      = 0x0800,
  sMTRL_MSK_RGBA      = 0x0f00,

  sMTRL_MSK_MASK      = 0x0f00,

  // vertex color

  sMTRL_VC_NOLIGHT    = 0x0000,
  sMTRL_VC_LIGHT0     = 0x1000,             // vertex color0 used as vertex light
  sMTRL_VC_LIGHT1     = 0x2000,             // vertex color1 used as vertex light
  sMTRL_VC_LIGHT_MSK  = 0x3000,
  sMTRL_VC_LSHIFT     = 12,

  sMTRL_VC_NOCOLOR    = 0x0000,
  sMTRL_VC_COLOR0     = 0x4000,             // vertex color0 used as material color
  sMTRL_VC_COLOR1     = 0x8000,             // vertex color1 used as material color
  sMTRL_VC_COLOR_MSK  = 0xc000,
  sMTRL_VC_CSHIFT     = 14,

  sMTRL_VC_MSK        = 0xf000,
  sMTRL_VC_OFF        = 0x0000,

  // more misc

  sMTRL_STENCILSS     = 0x00010000,         // single sided stencil operation
  sMTRL_STENCILDS     = 0x00020000,         // double sided stencil operation

};


enum sMaterialFunctionType
{
  sMFT_DEPTH = 0,
  sMFT_ALPHA = 1,
  sMFT_STENCIL = 2,
};

enum sMaterialFunctionFlag
{
  sMFF_NEVER          = 0,
  sMFF_LESS           = 1,
  sMFF_EQUAL          = 2,
  sMFF_LESSEQUAL      = 3,
  sMFF_GREATER        = 4,
  sMFF_NOTEQUAL       = 5,
  sMFF_GREATEREQUAL   = 6,
  sMFF_ALWAYS         = 7,
};

enum sMaterialTextureFlags
{
  sMTF_LEVEL0         = 0x0002,                       // off
  sMTF_LEVEL1         = 0x0003,                       // filter
  sMTF_LEVEL2         = 0x0000,                       // filter+mipmap (normal)
  sMTF_LEVEL3         = 0x0001,                       // extended quality
  sMTF_LEVELMASK      = 0x0003,

  sMTF_TILE           = 0x0000,
  sMTF_CLAMP          = 0x0010,
  sMTF_MIRROR         = 0x0020,
  sMTF_BORDER         = 0x0030,
  sMTF_ADDRMASK       = 0x00f0,

  sMTF_UVMASK         = 0x0f00,                       // these bits control uv transformation
  sMTR_UV3DMASK       = 0x0800,                       // if this is set, we need 3d calculations!
  sMTF_UV0            = 0x0000,
  sMTF_UV1            = 0x0100,
  sMTF_WORLDSPACE     = 0x0800,
  sMTF_MODELSPACE     = 0x0900,
  sMTF_CAMERASPACE    = 0x0a00,
  sMTF_REFLECTION     = 0x0b00,                       // lookup texture with reflection vector
  sMTF_SPHERICAL      = 0x0c00,                       // lookup texture with normal vector

  sMTF_SRGB           = 0x1000,                       // gamma corrected sRGB or linear texture, enable sMTF_SRGB for albedo colormaps,
                                                      // disable for lightmaps lookup textures or multiplicative blended textures
  sMTF_EXTERN         = 0x2000,                       // extern texture set by application (eg. render target)

  sMTF_SCALE          = 0x4000,
  sMTF_TRANSFORM      = 0x8000,                       // duplicated, remove and only use matrix selector
  sMTF_MTRL_MAT       = 0x00010000,                   // use sMTR_MTRL_MAT*id to select mtrl transformation matrix [1..4]
  sMTF_MTRL_MAT_MSK   = 0x000f0000,
  sMTF_ENV_MAT        = 0x00100000,                   // use sMTR_ENV_MAT*id to select environment transformation matrix [1..4]
  sMTF_ENV_MAT_MSK    = 0x00f00000,
};

enum sMaterialStencilOp                     // stencil operations, is like DirectX D3DSTENCILOP_??? -1
{
  sMSO_KEEP = 0,                            // no operation
  sMSO_ZERO = 1,                            // set to zero
  sMSO_REPLACE = 2,                         // replace with ref
  sMSO_INCSAT = 3,                          // increase by one and clamp
  sMSO_DECSAT = 4,                          // decrease by one and clamp
  sMSO_INVERT = 5,                          // bit-invert stencil buffer
  sMSO_INC = 6,                             // increase by one
  sMSO_DEC = 7,                             // decrease by one
};
 
enum sMaterialStencilOpIndex
{
  sMSI_CWFAIL = 0,
  sMSI_CWZFAIL = 1,
  sMSI_CWPASS = 2,
  sMSI_CCWFAIL = 3,
  sMSI_CCWZFAIL = 4,
  sMSI_CCWPASS = 5,

  sMSI_FAIL = sMSI_CWFAIL,
  sMSI_ZFAIL = sMSI_CWZFAIL,
  sMSI_PASS = sMSI_CWPASS,
};

enum sMaterialBlend
{
  sMBS_0              = 0x00000001,
  sMBS_1              = 0x00000002,
  sMBS_SC             = 0x00000003,
  sMBS_SCI            = 0x00000004,
  sMBS_SA             = 0x00000005,
  sMBS_SAI            = 0x00000006,
  sMBS_DA             = 0x00000007,
  sMBS_DAI            = 0x00000008,
  sMBS_DC             = 0x00000009,
  sMBS_DCI            = 0x0000000a,
  sMBS_SA_SAT         = 0x0000000b,
  sMBS_F              = 0x0000000e,
  sMBS_FI             = 0x0000000f,

  sMBO_ADD            = 0x00000100,
  sMBO_SUB            = 0x00000200,
  sMBO_SUBR           = 0x00000300,
  sMBO_MIN            = 0x00000400,
  sMBO_MAX            = 0x00000500,

  sMBD_0              = 0x00010000,
  sMBD_1              = 0x00020000,
  sMBD_SC             = 0x00030000,
  sMBD_SCI            = 0x00040000, 
  sMBD_SA             = 0x00050000,
  sMBD_SAI            = 0x00060000,
  sMBD_DA             = 0x00070000,
  sMBD_DAI            = 0x00080000,
  sMBD_DC             = 0x00090000,
  sMBD_DCI            = 0x000a0000,
  sMBD_SA_SAT         = 0x000b0000,
  sMBD_F              = 0x000e0000,
  sMBD_FI             = 0x000f0000,

  sMB_SAMEASCOLOR     = 0xffffffff,
  sMB_OFF             = 0x00000000,
  sMB_ALPHA           = (sMBS_SA |sMBO_ADD|sMBD_SAI),
  sMB_ADD             = (sMBS_1  |sMBO_ADD|sMBD_1  ),
  sMB_MUL             = (sMBS_DC |sMBO_ADD|sMBD_0  ),
  sMB_MUL2            = (sMBS_DC |sMBO_ADD|sMBD_SC ),
  sMB_ADDSMOOTH       = (sMBS_1  |sMBO_ADD|sMBD_SCI),
};

/****************************************************************************/

#endif
