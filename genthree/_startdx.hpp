// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef __STARTDX_HPP__
#define __STARTDX_HPP__

#include "_types.hpp"
#include "_start.hpp"


/****************************************************************************/
/***                                                                      ***/
/***   State Map                                                          ***/
/***                                                                      ***/
/****************************************************************************/

// Scale States
//
// 0x0000-0x00ff  SetRenderState(x&0xff,val)
// 0x0100-0x01ff  SetTextureStageState((x>>5)&7,x&0x1f,val)
// 0x0200-0x02ff  SetSamplerState((x>>4)&15,x&0x0f,val)
// 0x0300-0x0310  SetSamplerState(D3DDMAPSAMPLER,x&0x0f,val)
// 0x0310-0x0315  SetMaterial();

#define sD3DMAT_DIFFUSE                   0x0310
#define sD3DMAT_AMBIENT                   0x0311
#define sD3DMAT_SPECULAR                  0x0312
#define sD3DMAT_EMISSIVE                  0x0313
#define sD3DMAT_POWER                     0x0314

/****************************************************************************/
/***                                                                      ***/
/***   RenderState                                                        ***/
/***                                                                      ***/
/****************************************************************************/

#define sD3DRS_ZENABLE                    7
#define sD3DRS_FILLMODE                   8
#define sD3DRS_SHADEMODE                  9
#define sD3DRS_ZWRITEENABLE               14
#define sD3DRS_ALPHATESTENABLE            15
#define sD3DRS_LASTPIXEL                  16
#define sD3DRS_SRCBLEND                   19
#define sD3DRS_DESTBLEND                  20
#define sD3DRS_CULLMODE                   22
#define sD3DRS_ZFUNC                      23
#define sD3DRS_ALPHAREF                   24
#define sD3DRS_ALPHAFUNC                  25
#define sD3DRS_DITHERENABLE               26
#define sD3DRS_ALPHABLENDENABLE           27
#define sD3DRS_FOGENABLE                  28
#define sD3DRS_SPECULARENABLE             29
#define sD3DRS_FOGCOLOR                   34
#define sD3DRS_FOGTABLEMODE               35
#define sD3DRS_FOGSTART                   36
#define sD3DRS_FOGEND                     37
#define sD3DRS_FOGDENSITY                 38
#define sD3DRS_RANGEFOGENABLE             48
#define sD3DRS_STENCILENABLE              52
#define sD3DRS_STENCILFAIL                53
#define sD3DRS_STENCILZFAIL               54
#define sD3DRS_STENCILPASS                55
#define sD3DRS_STENCILFUNC                56
#define sD3DRS_STENCILREF                 57
#define sD3DRS_STENCILMASK                58
#define sD3DRS_STENCILWRITEMASK           59
#define sD3DRS_TEXTUREFACTOR              60
#define sD3DRS_WRAP0                      128
#define sD3DRS_WRAP1                      129
#define sD3DRS_WRAP2                      130
#define sD3DRS_WRAP3                      131
#define sD3DRS_WRAP4                      132
#define sD3DRS_WRAP5                      133
#define sD3DRS_WRAP6                      134
#define sD3DRS_WRAP7                      135
#define sD3DRS_CLIPPING                   136
#define sD3DRS_LIGHTING                   137
#define sD3DRS_AMBIENT                    139
#define sD3DRS_FOGVERTEXMODE              140
#define sD3DRS_COLORVERTEX                141
#define sD3DRS_LOCALVIEWER                142
#define sD3DRS_NORMALIZENORMALS           143
#define sD3DRS_DIFFUSEMATERIALSOURCE      145
#define sD3DRS_SPECULARMATERIALSOURCE     146
#define sD3DRS_AMBIENTMATERIALSOURCE      147
#define sD3DRS_EMISSIVEMATERIALSOURCE     148
#define sD3DRS_VERTEXBLEND                151
#define sD3DRS_CLIPPLANEENABLE            152
#define sD3DRS_POINTSIZE                  154
#define sD3DRS_POINTSIZE_MIN              155
#define sD3DRS_POINTSPRITEENABLE          156
#define sD3DRS_POINTSCALEENABLE           157
#define sD3DRS_POINTSCALE_A               158
#define sD3DRS_POINTSCALE_B               159
#define sD3DRS_POINTSCALE_C               160
#define sD3DRS_MULTISAMPLEANTIALIAS       161
#define sD3DRS_MULTISAMPLEMASK            162
#define sD3DRS_PATCHEDGESTYLE             163
#define sD3DRS_DEBUGMONITORTOKEN          165
#define sD3DRS_POINTSIZE_MAX              166
#define sD3DRS_INDEXEDVERTEXBLENDENABLE   167
#define sD3DRS_COLORWRITEENABLE           168
#define sD3DRS_TWEENFACTOR                170
#define sD3DRS_BLENDOP                    171
#define sD3DRS_POSITIONDEGREE             172
#define sD3DRS_NORMALDEGREE               173
#define sD3DRS_SCISSORTESTENABLE          174
#define sD3DRS_SLOPESCALEDEPTHBIAS        175
#define sD3DRS_ANTIALIASEDLINEENABLE      176
#define sD3DRS_MINTESSELLATIONLEVEL       178
#define sD3DRS_MAXTESSELLATIONLEVEL       179
#define sD3DRS_ADAPTIVETESS_X             180
#define sD3DRS_ADAPTIVETESS_Y             181
#define sD3DRS_ADAPTIVETESS_Z             182
#define sD3DRS_ADAPTIVETESS_W             183
#define sD3DRS_ENABLEADAPTIVETESSELLATION 184
#define sD3DRS_TWOSIDEDSTENCILMODE        185
#define sD3DRS_CCW_STENCILFAIL            186
#define sD3DRS_CCW_STENCILZFAIL           187
#define sD3DRS_CCW_STENCILPASS            188
#define sD3DRS_CCW_STENCILFUNC            189
#define sD3DRS_COLORWRITEENABLE1          190
#define sD3DRS_COLORWRITEENABLE2          191
#define sD3DRS_COLORWRITEENABLE3          192
#define sD3DRS_BLENDFACTOR                193
#define sD3DRS_SRGBWRITEENABLE            194
#define sD3DRS_DEPTHBIAS                  195
#define sD3DRS_WRAP8                      198
#define sD3DRS_WRAP9                      199
#define sD3DRS_WRAP10                     200
#define sD3DRS_WRAP11                     201
#define sD3DRS_WRAP12                     202
#define sD3DRS_WRAP13                     203
#define sD3DRS_WRAP14                     204
#define sD3DRS_WRAP15                     205
#define sD3DRS_SEPARATEALPHABLENDENABLE   206
#define sD3DRS_SRCBLENDALPHA              207
#define sD3DRS_DESTBLENDALPHA             208
#define sD3DRS_BLENDOPALPHA               209

/****************************************************************************/
/****************************************************************************/

// sD3DRS_ZENABLE                  

#define sD3DZB_FALSE                  0
#define sD3DZB_TRUE                   1
#define sD3DZB_USEW                   2

/****************************************************************************/

// sD3DRS_FILLMODE                 

#define sD3DFILL_POINT                1
#define sD3DFILL_WIREFRAME            2
#define sD3DFILL_SOLID                3

/****************************************************************************/

// sD3DRS_SHADEMODE                

#define sD3DSHADE_FLAT                1
#define sD3DSHADE_GOURAUD             2
#define sD3DSHADE_PHONG               3

/****************************************************************************/

// sD3DRS_SRCBLEND                 
// sD3DRS_DESTBLEND                

#define sD3DBLEND_ZERO                 1
#define sD3DBLEND_ONE                  2
#define sD3DBLEND_SRCCOLOR             3
#define sD3DBLEND_INVSRCCOLOR          4
#define sD3DBLEND_SRCALPHA             5
#define sD3DBLEND_INVSRCALPHA          6
#define sD3DBLEND_DESTALPHA            7
#define sD3DBLEND_INVDESTALPHA         8
#define sD3DBLEND_DESTCOLOR            9
#define sD3DBLEND_INVDESTCOLOR        10
#define sD3DBLEND_SRCALPHASAT         11
#define sD3DBLEND_BOTHSRCALPHA        12
#define sD3DBLEND_BOTHINVSRCALPHA     13

/****************************************************************************/

// sD3DRS_CULLMODE                 

#define sD3DCULL_NONE                 1
#define sD3DCULL_CW                   2
#define sD3DCULL_CCW                  3

/****************************************************************************/

// sD3DRS_ZFUNC                    
// sD3DRS_ALPHAFUNC                
// sD3DRS_STENCILFUNC              

#define sD3DCMP_NEVER                 1
#define sD3DCMP_LESS                  2
#define sD3DCMP_EQUAL                 3
#define sD3DCMP_LESSEQUAL             4
#define sD3DCMP_GREATER               5
#define sD3DCMP_NOTEQUAL              6
#define sD3DCMP_GREATEREQUAL          7
#define sD3DCMP_ALWAYS                8

/****************************************************************************/

// sD3DRS_FOGTABLEMODE             
// sD3DRS_FOGVERTEXMODE            

#define sD3DFOG_NONE                  0
#define sD3DFOG_EXP                   1
#define sD3DFOG_EXP2                  2
#define sD3DFOG_LINEAR                3

/****************************************************************************/

// sD3DRS_STENCILFAIL              
// sD3DRS_STENCILZFAIL             
// sD3DRS_STENCILPASS              

#define sD3DSTENCILOP_KEEP            1
#define sD3DSTENCILOP_ZERO            2
#define sD3DSTENCILOP_REPLACE         3
#define sD3DSTENCILOP_INCRSAT         4
#define sD3DSTENCILOP_DECRSAT         5
#define sD3DSTENCILOP_INVERT          6
#define sD3DSTENCILOP_INCR            7
#define sD3DSTENCILOP_DECR            8

/****************************************************************************/

// sD3DRS_DIFFUSEMATERIALSOURCE    
// sD3DRS_SPECULARMATERIALSOURCE   
// sD3DRS_AMBIENTMATERIALSOURCE    
// sD3DRS_EMISSIVEMATERIALSOURCE   

#define sD3DMCS_MATERIAL          0
#define sD3DMCS_COLOR1            1
#define sD3DMCS_COLOR2            2

/****************************************************************************/

// sD3DRS_VERTEXBLEND              

#define sD3DVBF_DISABLE     0
#define sD3DVBF_1WEIGHTS    1
#define sD3DVBF_2WEIGHTS    2
#define sD3DVBF_3WEIGHTS    3
#define sD3DVBF_TWEENING  255
#define sD3DVBF_0WEIGHTS  256

/****************************************************************************/

// sD3DRS_PATCHEDGESTYLE           

#define s3DPATCHEDGE_DISCRETE     0
#define s3DPATCHEDGE_CONTINUOUS   1

/****************************************************************************/

// sD3DRS_DEBUGMONITORTOKEN        

#define sD3DDMT_ENABLE           0
#define sD3DDMT_DISABLE          1

/****************************************************************************/

// sD3DRS_COLORWRITEENABLE         

#define sD3DCOLORWRITEENABLE_RED     (1L<<0)
#define sD3DCOLORWRITEENABLE_GREEN   (1L<<1)
#define sD3DCOLORWRITEENABLE_BLUE    (1L<<2)
#define sD3DCOLORWRITEENABLE_ALPHA   (1L<<3)

/****************************************************************************/

// sD3DRS_BLENDOP                    

#define sD3DBLENDOP_ADD               1
#define sD3DBLENDOP_SUBTRACT          2
#define sD3DBLENDOP_REVSUBTRACT       3
#define sD3DBLENDOP_MIN               4
#define sD3DBLENDOP_MAX               5

/****************************************************************************/
/***                                                                      ***/
/***   TextureStageState                                                  ***/
/***                                                                      ***/
/****************************************************************************/

#define sD3DTSS_COLOROP                 ( 1|0x100)
#define sD3DTSS_COLORARG1               ( 2|0x100)
#define sD3DTSS_COLORARG2               ( 3|0x100)
#define sD3DTSS_ALPHAOP                 ( 4|0x100)
#define sD3DTSS_ALPHAARG1               ( 5|0x100)
#define sD3DTSS_ALPHAARG2               ( 6|0x100)
#define sD3DTSS_BUMPENVMAT00            ( 7|0x100)
#define sD3DTSS_BUMPENVMAT01            ( 8|0x100)
#define sD3DTSS_BUMPENVMAT10            ( 9|0x100)
#define sD3DTSS_BUMPENVMAT11            (10|0x100)
#define sD3DTSS_TEXCOORDINDEX           (11|0x100)
#define sD3DTSS_BUMPENVLSCALE           (22|0x100)
#define sD3DTSS_BUMPENVLOFFSET          (23|0x100)
#define sD3DTSS_TEXTURETRANSFORMFLAGS   (24|0x100)
#define sD3DTSS_COLORARG0               (26|0x100)
#define sD3DTSS_ALPHAARG0               (27|0x100)
#define sD3DTSS_RESULTARG               (28|0x100)
#define sD3DTSS_CONSTANT                (32|0x100)   // bug!

// please select your texture stage....

#define sD3DTSS_0                     0x00
#define sD3DTSS_1                     0x20
#define sD3DTSS_2                     0x40
#define sD3DTSS_3                     0x60
#define sD3DTSS_4                     0x80
#define sD3DTSS_5                     0xa0
#define sD3DTSS_6                     0xc0
#define sD3DTSS_7                     0xe0

/****************************************************************************/
/****************************************************************************/

// sD3DTSS_COLOROP
// sD3DTSS_ALPHAOP

#define sD3DTOP_DISABLE                     1  // disables stage
#define sD3DTOP_SELECTARG1                  2  // the default
#define sD3DTOP_SELECTARG2                  3
#define sD3DTOP_MODULATE                    4  // multiply args together
#define sD3DTOP_MODULATE2X                  5  // multiply and left-shift 1 bit
#define sD3DTOP_MODULATE4X                  6  // multiply and left-shift  2 bits
#define sD3DTOP_ADD                         7  // add arguments together
#define sD3DTOP_ADDSIGNED                   8  // add with -0.5 bias
#define sD3DTOP_ADDSIGNED2X                 9  // as above but left-shift 1 bit
#define sD3DTOP_SUBTRACT                   10  // Arg1 - Arg2, with no saturation
#define sD3DTOP_ADDSMOOTH                  11  // Arg1 + (1-Arg1)*Arg2
#define sD3DTOP_BLENDDIFFUSEALPHA          12  // Arg1*(Alpha) + Arg2*(1-Alpha), iterated alpha
#define sD3DTOP_BLENDTEXTUREALPHA          13  // Arg1*(Alpha) + Arg2*(1-Alpha), texture alpha
#define sD3DTOP_BLENDFACTORALPHA           14  // Arg1*(Alpha) + Arg2*(1-Alpha), D3DRENDERSTATE_TEXTUREFACTOR
#define sD3DTOP_BLENDTEXTUREALPHAPM        15  // Arg1 + Arg2*(1-Alpha), texture alpha
#define sD3DTOP_BLENDCURRENTALPHA          16  // Arg1 + Arg2*(1-Alpha), alpha of current color
#define sD3DTOP_PREMODULATE                17  // modulate with next texture before use
#define sD3DTOP_MODULATEALPHA_ADDCOLOR     18  // Arg1.RGB + Arg1.A*Arg2.RGB
#define sD3DTOP_MODULATECOLOR_ADDALPHA     19  // Arg1.RGB*Arg2.RGB + Arg1.A
#define sD3DTOP_MODULATEINVALPHA_ADDCOLOR  20  // (1-Arg1.A)*Arg2.RGB + Arg1.RGB
#define sD3DTOP_MODULATEINVCOLOR_ADDALPHA  21  // (1-Arg1.RGB)*Arg2.RGB + Arg1.A
#define sD3DTOP_BUMPENVMAP                 22  // per pixel env map perturbation
#define sD3DTOP_BUMPENVMAPLUMINANCE        23  // with luminance channel
#define sD3DTOP_DOTPRODUCT3                24  // (Arg1.R*Arg2.R + Arg1.G*Arg2.G + Arg1.B*Arg2.B) -> RGB
#define sD3DTOP_MULTIPLYADD                25  // Arg0 + Arg1*Arg2
#define sD3DTOP_LERP                       26  // (Arg0)*Arg1 + (1-Arg0)*Arg2

/****************************************************************************/

// sD3DTSS_COLORARG1
// sD3DTSS_COLORARG2
// sD3DTSS_ALPHAARG1
// sD3DTSS_ALPHAARG1
// sD3DTSS_COLORARG0
// sD3DTSS_ALPHAARG0
// sD3DTSS_RESULTARG

#define sD3DTA_SELECTMASK        0x0000000f  // mask for arg selector
#define sD3DTA_DIFFUSE           0x00000000  // select diffuse color (read only)
#define sD3DTA_CURRENT           0x00000001  // select stage destination register (read/write)
#define sD3DTA_TEXTURE           0x00000002  // select texture color (read only)
#define sD3DTA_TFACTOR           0x00000003  // select RENDERSTATE_TEXTUREFACTOR (read only)
#define sD3DTA_SPECULAR          0x00000004  // select specular color (read only)
#define sD3DTA_TEMP              0x00000005  // select temporary register color (read/write)
#define sD3DTA_CONSTANT          0x00000006  // select texture stage constant
#define sD3DTA_COMPLEMENT        0x00000010  // take 1.0 - x (read modifier)
#define sD3DTA_ALPHAREPLICATE    0x00000020  // replicate alpha to color components (read modifier)

/****************************************************************************/

// sD3DTSS_TEXCOORDINDEX

#define D3DTSS_TCI_0                                    0
#define D3DTSS_TCI_1                                    1
#define D3DTSS_TCI_2                                    2
#define D3DTSS_TCI_3                                    3
#define D3DTSS_TCI_4                                    4
#define D3DTSS_TCI_5                                    5
#define D3DTSS_TCI_6                                    6
#define D3DTSS_TCI_7                                    7
#define D3DTSS_TCI_PASSTHRU                             0x00000000
#define D3DTSS_TCI_CAMERASPACENORMAL                    0x00010000
#define D3DTSS_TCI_CAMERASPACEPOSITION                  0x00020000
#define D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR          0x00030000
#define D3DTSS_TCI_SPHEREMAP                            0x00040000

/****************************************************************************/

// sD3DTSS_ADDRESSU
// sD3DTSS_ADDRESSV
// sD3DTSS_ADDRESSW

#define sD3DTADDRESS_WRAP             1
#define sD3DTADDRESS_MIRROR           2
#define sD3DTADDRESS_CLAMP            3
#define sD3DTADDRESS_BORDER           4
#define sD3DTADDRESS_MIRRORONCE       5

/****************************************************************************/

// sD3DTSS_MAGFILTER
// sD3DTSS_MINFILTER
// sD3DTSS_MIPFILTER

#define sD3DTEXF_NONE             0
#define sD3DTEXF_POINT            1
#define sD3DTEXF_LINEAR           2
#define sD3DTEXF_ANISOTROPIC      3
#define sD3DTEXF_PYRAMIDALQUAD    6
#define sD3DTEXF_GAUSSIANQUAD     7

/****************************************************************************/

// sD3DTSS_TEXTURETRANSFORMFLAGS

#define sD3DTTFF_DISABLE            0
#define sD3DTTFF_COUNT1             1
#define sD3DTTFF_COUNT2             2
#define sD3DTTFF_COUNT3             3
#define sD3DTTFF_COUNT4             4
#define sD3DTTFF_PROJECTED        256


/****************************************************************************/
/***                                                                      ***/
/***   SamplerState                                                       ***/
/***                                                                      ***/
/****************************************************************************/

#define sD3DSAMP_ADDRESSU       ( 1|0x200)
#define sD3DSAMP_ADDRESSV       ( 2|0x200)
#define sD3DSAMP_ADDRESSW       ( 3|0x200)
#define sD3DSAMP_BORDERCOLOR    ( 4|0x200)
#define sD3DSAMP_MAGFILTER      ( 5|0x200)
#define sD3DSAMP_MINFILTER      ( 6|0x200)
#define sD3DSAMP_MIPFILTER      ( 7|0x200)
#define sD3DSAMP_MIPMAPLODBIAS  ( 8|0x200)
#define sD3DSAMP_MAXMIPLEVEL    ( 9|0x200)
#define sD3DSAMP_MAXANISOTROPY  (10|0x200)
#define sD3DSAMP_SRGBTEXTURE    (11|0x200)
#define sD3DSAMP_ELEMENTINDEX   (12|0x200)
#define sD3DSAMP_DMAPOFFSET     (13|0x200)

// please select your samplee....

#define sD3DSAMP_0                    0x000
#define sD3DSAMP_1                    0x010
#define sD3DSAMP_2                    0x020
#define sD3DSAMP_3                    0x030
#define sD3DSAMP_4                    0x040
#define sD3DSAMP_5                    0x050
#define sD3DSAMP_6                    0x060
#define sD3DSAMP_7                    0x070
#define sD3DSAMP_8                    0x080
#define sD3DSAMP_9                    0x090
#define sD3DSAMP_10                   0x0a0
#define sD3DSAMP_11                   0x0b0
#define sD3DSAMP_12                   0x0c0
#define sD3DSAMP_13                   0x0d0
#define sD3DSAMP_14                   0x0e0
#define sD3DSAMP_15                   0x0f0
#define sD3DSAMP_DMAP                 0x100

/****************************************************************************/
/***                                                                      ***/
/***   Material                                                           ***/
/***                                                                      ***/
/****************************************************************************/

struct sD3DMaterial
{
  sVector Diffuse;
  sVector Ambient;
  sVector Specular;
  sVector Emissive;
  sF32 Power;
};

/****************************************************************************/
/***                                                                      ***/
/***   Light                                                              ***/
/***                                                                      ***/
/****************************************************************************/

struct sD3DLight
{
  sInt Type;                        // sD3DLIGHT_???
  sVector Diffuse;
  sVector Specular;
  sVector Ambient;
  sF32 Position[3];
  sF32 Direction[3];
  sF32 Range;
  sF32 Falloff;
  sF32 Attenuation[3];
  sF32 Theta;
  sF32 Phi;
};

/****************************************************************************/

#define sD3DLIGHT_POINT           1
#define sD3DLIGHT_SPOT            2
#define sD3DLIGHT_DIRECTIONAL     3

/****************************************************************************/
/****************************************************************************/

struct sD3DViewport
{
  sU32 X;
  sU32 Y;
  sU32 Width;
  sU32 Height;
  sF32 MinZ;
  sF32 MaxZ;
};

#endif


