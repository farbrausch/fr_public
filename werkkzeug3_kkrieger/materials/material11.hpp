// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __MATERIAL11_HPP__
#define __MATERIAL11_HPP__

#include "_types.hpp"
#include "_start.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   1.1 Material System                                                ***/
/***                                                                      ***/
/****************************************************************************/

// base flags

#define sMBF_ALPHATEST    0x0001  // zero gets transparent
#define sMBF_DOUBLESIDED  0x0002  // draw doublesided
#define sMBF_INVERTCULL   0x0004  // invert culling
#define sMBF_ZBIASBACK    0x0008  // do some sensible z-biasing (push back)
#define sMBF_ZBIASFORE    0x0010  // do some sensible z-biasing (push fore)
#define sMBF_ZONLY        0x0020  // do not write the colorbuffer
#define sMBF_FOG          0x0040  // enable fogging
#define sMBF_ZEQUAL       0x0080  // equal z-test (for secondary passes)

#define sMBF_ZMASK        0x0300  // zbuffer control
#define sMBF_ZOFF         0x0000  // no z operation
#define sMBF_ZWRITE       0x0100  // charge z buffer
#define sMBF_ZREAD        0x0200  // blend operation
#define sMBF_ZON          0x0300  // normal operation

#define sMBF_BLENDMASK    0xf000  // blend stage (d=dest, s=src)
#define sMBF_BLENDOFF     0x0000  // d=s
#define sMBF_BLENDALPHA   0x1000  // alpha blending
#define sMBF_BLENDADD     0x2000  // d=(d+s)
#define sMBF_BLENDMUL     0x3000  // d=(d*s)
#define sMBF_BLENDMUL2    0x4000  // d=(d*s)*2
#define sMBF_BLENDSMOOTH  0x5000  // d=(d+s)-(d*s)
#define sMBF_BLENDSUB     0x6000  // d=(d-s)
#define sMBF_BLENDINVDEST 0x7000  // d=(1-d)+(1-s)
#define sMBF_BLENDADDDA   0x8000  // d=d+s*d.alpha
#define sMBF_BLENDADDSA   0x9000  // d=d+s*s.alpha

#define sMBF_NOTEXTURE    0x010000
#define sMBF_NONORMAL     0x020000
#define sMBF_SHADOWMASK   0x040000  // the stencil ops requiring shadow casting

#define sMBF_STENCILMASK  0x700000  // all stencil operation
#define sMBF_STENCILINC   0x100000  // cast shadow
#define sMBF_STENCILDEC   0x200000  // cast shadow
#define sMBF_STENCILTEST  0x300000  // receive shadow
#define sMBF_STENCILCLR   0x400000  // clear shadow
#define sMBF_STENCILINDE  0x500000  // increment/decrement (2sided)
#define sMBF_STENCILRENDER 0x600000 // render (+clear)
#define sMBF_STENCILZFAIL 0x800000  // for inc/dec

// special flags

#define sMSF_BUMPDETAIL   0x0001  // bump-detail (Tex0)

#define sMSF_ENVIMASK     0x0030  // environment mapping (Tex2)
#define sMSF_ENVIOFF      0x0000  // no envi
#define sMSF_ENVISPHERE   0x0010  // sphere envi (normal)
#define sMSF_ENVIREFLECT  0x0020  // reflection envi ("specular")

#define sMSF_NOSPECULAR   0x0040  // disable specular
#define sMSF_SPECMAP      0x0080  // use bumpmap alpha as specularity mapping

#define sMSF_PROJMASK     0x0700  // UV Projection (Tex3)
#define sMSF_PROJOFF      0x0000  // no uv projection 
#define sMSF_PROJMODEL    0x0100  // model space
#define sMSF_PROJWORLD    0x0200  // world space
#define sMSF_PROJEYE      0x0300  // eye space

// light flags

#define sMLF_MODEMASK     0x0007  // lighting mode
#define sMLF_OFF          0x0000  // no light calculation
#define sMLF_BUMPX        0x0005  // bump

#define sMLF_DIFFUSE      0x0010  // multiply diffuse light to color0
#define sMLF_AMBIENT      0x0020  // multiply ambient light to color2

// texture flags

#define sMTF_FILTER       0x0001
#define sMTF_MIPMAPS      0x0002
#define sMTF_ANISO        0x0003
#define sMTF_MIPUF        0x0004  // point-sampled unfiltered mipmaps

#define sMTF_UV0          0x0000
#define sMTF_UV1          0x0010

#define sMTF_TILE         0x0000
#define sMTF_CLAMP        0x0100

#define sMTF_TRANSMASK    0x3000
#define sMTF_DIRECT       0x0000
#define sMTF_SCALE        0x1000
#define sMTF_TRANS1       0x2000
#define sMTF_TRANS2       0x3000

#define sMTF_PROJECTIVE   0x4000

// material combiner alpha (multiply input with this)

#define sMCA_OFF          0x0000
#define sMCA_LIGHT        0x0001
#define sMCA_SPECULAR     0x0002
#define sMCA_ATTENUATION  0x0003
#define sMCA_TEX0         0x0004
#define sMCA_TEX1         0x0005
#define sMCA_TEX2         0x0006
#define sMCA_TEX3         0x0007
#define sMCA_COL0         0x0008
#define sMCA_COL1         0x0009
#define sMCA_COL2         0x000a
#define sMCA_COL3         0x000b
#define sMCA_HALF         0x000c
#define sMCA_ZERO         0x000d
#define sMCA_MIX01        0x000e

#define sMCA_INVERTA      0x0100
#define sMCA_INVERTB      0x0200

// material combiner op

#define sMCOA_NOP         0x0000
#define sMCOA_SET         0x0010
#define sMCOA_ADD         0x0020
#define sMCOA_SUB         0x0030
#define sMCOA_MUL         0x0040
#define sMCOA_MUL2        0x0050
#define sMCOA_MUL4        0x0060
#define sMCOA_MUL8        0x0070
#define sMCOA_SMOOTH      0x0080
#define sMCOA_LERP        0x0090  // input + current*(1-alpha)    (input is already multiplied with alpha)
#define sMCOA_RSUB        0x00a0
#define sMCOA_DP3         0x00b0
#define sMCOA_MULS        0x00c0  // mul+saturate

// material combiner stage enumeration

#define sMCS_LIGHT        0
#define sMCS_COLOR0       1
#define sMCS_COLOR1       2
#define sMCS_TEX0         3
#define sMCS_TEX1         4
/*#define sMCS_MERGE        5
#define sMCS_SPECULAR     6*/
#define sMCS_COLOR2       7
#define sMCS_COLOR3       8
#define sMCS_TEX2         9
#define sMCS_TEX3         10
#define sMCS_VERTEX       11
//#define sMCS_FINAL        12
#define sMCS_MAX          13

/****************************************************************************/

class sMaterial11 : public sMaterial  // order+size must match material operator
{
  sInt Tex[4];                    // texture handles
  sInt TexUse[4];                 // maps texture stages to texture numbers
  sBool HasTexSRT;                // texture srt used?
  sInt Setup;                     // setup id

  void FreeEverything();
  void LoadDefaults();

public:
  sU32 BaseFlags;                 //  0: sMBF_???
  sU32 SpecialFlags;              //  1: sMSF_???
  sU32 LightFlags;                //  2: sMLF_???
  sU32 pad1;                      //  3:
  sU32 TFlags[4];                 //  4: sMTF_??

  sU32 Combiner[sMCS_MAX];        //  8: sMCFA_??, sMCFB_???, sMAF_???
  sU32 pad3;                      // 21
  sU32 pad4;                      // 22
  sU32 AlphaCombiner;             // 23: sMAF_???, sMAF_???<<4

  sF32 TScale[4];                 // 24: uv scale factors for texture
  sU32 Color[4];                  // 28: colors
  sF32 SpecPower;                 // 32: Specular power
  sU32 pad5;                      // 33
  sF32 SRT1[9];                   // 34: uv transform 1 : SSS RRR TTT
  sF32 SRT2[5];                   // 43: uv transform 2 : SS R TT

                                  // 48: no byte-counting needed after this
  sInt ShaderLevel;               // sPS_?? pixel shader version to use

#if !sPLAYER
  sInt PSOps;                     // PS-Ops used
  sInt VSOps;                     // VS-Ops used
  sInt ShaderLevelUsed;           // shaderlevel really used (to indicate automatic downgrade, which is not implemented yet)
#endif

  sMaterial11();
  ~sMaterial11();

  void Reset();
  void CopyFrom(const sMaterial11 *x);
  void DefaultCombiner(sBool *tex=0); // set combiner to defaults, tex holds an array of bools if the textures are set.
  void SetTex(sInt i,sInt handle);

  sBool Compile();
  void Set(const sMaterialEnv &env);

  static void SetShadowStates(sU32 flags);
};

/****************************************************************************/

// layout of the color-combiner
// input      L   R          usage           specials
//
// Tex[0]    set  *          base texture    bump-detail    
// Tex[1]     *   *          bump texture    bump
// Color[0]  mul  *          diffuse
// Color[1]   *   *                                           
// Light     mul  *     
//              * |                                           
// Tex[2]    mul  *          detail          envi-mapping
// Tex[3]    mul  *          detail          projection-mapping    
// Color[2]  add  *          ambient
// Color[3]   *   *                                           
// Specular  add  *                          gloss mapping
// combiner    add                                            

/****************************************************************************/

#endif
