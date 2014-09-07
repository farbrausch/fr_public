/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_BASE_GRAPHICS_HPP
#define FILE_ALTONA2_LIBS_BASE_GRAPHICS_HPP

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Base/Base.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***    forwards                                                          ***/
/***                                                                      ***/
/****************************************************************************/

class sResource;
class sVertexFormat;
class sGeometry;

class sCBufferBase;
class sShaderBinary;
class sShader;

class sSamplerState;
class sRenderState;
class sMaterial;

class sDynamicGeometry;

struct sPerfMonSection;

/****************************************************************************/
/***                                                                      ***/
/***    enumerations and defines                                          ***/
/***                                                                      ***/
/****************************************************************************/

enum sScreenModeFlags             // screenmode flags
{
    sSM_NoVSync         = 0x0001,   // do not wait for vsync
    sSM_Fullscreen      = 0x0002,   // open fullscreen
    sSM_FullWindowed    = 0x0004,   // fake fullscreen mode with undecorated window (DX11 only)
    sSM_PartialUpdate   = 0x0008,   // do not discard buffers. You may have to deal with updating 1 to 3 buffers.
    sSM_Multithreaded   = 0x0100,   // DX9: open device with multithreaded flag
    sSM_NoDiscard       = 0x0200,   // DX9: do not discard. This improves flipping timing in some cases
    sSM_BackgroundRender= 0x0400,   // also render when window is inactive

    sSM_NoInitialScreen = 0x0010,   // at startup, do not open an initial screen.
    sSM_Shared          = 0x0020,   // create color & zbuffer with shared surface
    sSM_DepthRead       = 0x0800,   // allow read access to depth buffer. might require some magic.
    sSM_Headless        = 0x0040,   // start up directx-device without actual window
    sSM_PreserveFpuDx9  = 0x0080,   // DX9: do not reduce FPU precision for the whole process...
    sSM_Debug           = 0x0100,   // DX11: enable debug layer
};

enum WindowControlEnum
{
    sWC_Hide,
    sWC_Normal,
    sWC_Minimize,
    sWC_Maximize,
};

enum sResourceFormat
{
    // channels

    sRFC_R            = 0x00000001,
    sRFC_RG           = 0x00000002,
    sRFC_RGB          = 0x00000003,
    sRFC_RGBA         = 0x00000004,
    sRFC_RGBX         = 0x00000005,

    sRFC_A            = 0x00000006,
    sRFC_I            = 0x00000007,
    sRFC_AI           = 0x00000008,
    sRFC_IA           = 0x00000009,
    sRFC_BGRA         = 0x0000000a,
    sRFC_BGRX         = 0x0000000b,
    sRFC_D            = 0x0000000c,
    sRFC_DS           = 0x0000000d,
    sRFC_DX           = 0x0000000e,

    //  sRFC_Raw        = 0x0000000f,
    //  sRFC_Structured = 0x00000010,

    sRFC_Unknown      = 0,
    sRFC_Mask         = 0x000000ff,

    // bits

    sRFB_4            = 0x00000100,
    sRFB_8            = 0x00000200,
    sRFB_16           = 0x00000300,
    sRFB_32           = 0x00000400,

    sRFB_5_6_5        = 0x00000500,
    sRFB_5_5_5_1      = 0x00000600,
    sRFB_10_10_10_2   = 0x00000700,
    sRFB_24_8         = 0x00000800,
    sRFB_11_11_10     = 0x00000900,

    sRFB_BC1          = 0x00001100,   // 4 bbp: DXT1
    sRFB_BC2          = 0x00001200,   // 8 bpp: DXT3
    sRFB_BC3          = 0x00001300,   // 8 bpp: DXT5
    sRFB_BC4          = 0x00001400,   // 4 bpp: DXT5 alpha block to red
    sRFB_BC5          = 0x00001500,   // 8 bpp: two DXT5 alpha blocks to Red & Green
    sRFB_BC6H         = 0x00001600,   // ???
    sRFB_BC7          = 0x00001700,   // ???
    sRFB_PVR2         = 0x00001800,   // 2 bbp RGB
    sRFB_PVR2A        = 0x00001900,   // 2 bpp A
    sRFB_PVR4         = 0x00001a00,
    sRFB_PVR4A        = 0x00001b00,
    sRFB_ETC1         = 0x00001c00,
    sRFB_ETC2         = 0x00001d00,
    sRFB_ETC2A        = 0x00001e00,


    sRFB_Unknown      = 0,
    sRFB_Mask4x4      = 0x00001000,   // use this to check for 4x4 block compression
    sRFB_Mask         = 0x0000ff00,

    // type

    sRFT_UNorm        = 0x00010000,
    sRFT_UInt         = 0x00020000,
    sRFT_SNorm        = 0x00030000,
    sRFT_SInt         = 0x00040000,
    sRFT_Float        = 0x00050000,
    sRFT_Typeless     = 0x00060000,
    sRFT_SRGB         = 0x00070000,  // UNorm with sRGB->Linear conversion

    sRFT_Unknown      = 0,
    sRFT_Mask         = 0x00ff0000,

    // predefined

    sRF_Structure     = 0,
    sRF_RGBA8888      = sRFC_RGBA|sRFB_8|sRFT_UNorm,

    sRF_BGRA8888      = sRFC_BGRA|sRFB_8|sRFT_UNorm,
    sRF_BGRA4444      = sRFC_BGRA|sRFB_4|sRFT_UNorm,
    sRF_BGRA5551      = sRFC_BGRA|sRFB_5_5_5_1|sRFT_UNorm,
    sRF_BGR565        = sRFC_BGRA|sRFB_5_6_5|sRFT_UNorm,
    sRF_A8            = sRFC_A|sRFB_8|sRFT_UNorm,
    sRF_R8            = sRFC_R|sRFB_8|sRFT_UNorm,

    sRF_BC1           = sRFC_RGB  | sRFB_BC1   |sRFT_UNorm,
    sRF_BC2           = sRFC_RGBA | sRFB_BC2   |sRFT_UNorm,
    sRF_BC3           = sRFC_RGBA | sRFB_BC3   |sRFT_UNorm,
    sRF_PVR2          = sRFC_RGB  | sRFB_PVR2  |sRFT_UNorm,
    sRF_PVR4          = sRFC_RGB  | sRFB_PVR4  |sRFT_UNorm,
    sRF_PVR2A         = sRFC_RGBA | sRFB_PVR2A |sRFT_UNorm,
    sRF_PVR4A         = sRFC_RGBA | sRFB_PVR4A |sRFT_UNorm,
    sRF_ETC1          = sRFC_RGB  | sRFB_ETC1  |sRFT_UNorm,
    sRF_ETC2          = sRFC_RGB  | sRFB_ETC2  |sRFT_UNorm,
    sRF_ETC2A         = sRFC_RGBA | sRFB_ETC2A |sRFT_UNorm,

    sRF_RGBA8888_SRGB = sRFC_RGBA | sRFB_8   | sRFT_SRGB,
    sRF_BGRA8888_SRGB = sRFC_BGRA | sRFB_8   | sRFT_SRGB,
    sRF_BC1_SRGB      = sRFC_RGB  | sRFB_BC1 | sRFT_SRGB,
    sRF_BC2_SRGB      = sRFC_RGBA | sRFB_BC2 | sRFT_SRGB,
    sRF_BC3_SRGB      = sRFC_RGBA | sRFB_BC3 | sRFT_SRGB,
 
    sRF_D16           = sRFC_D|sRFB_16|sRFT_UNorm,
    sRF_D24           = sRFC_DX|sRFB_24_8|sRFT_UNorm,
    sRF_D24S8         = sRFC_DS|sRFB_24_8|sRFT_UNorm,
    sRF_D32           = sRFC_D|sRFB_32|sRFT_Float,
    sRF_X32           = sRFC_R|sRFB_32|sRFT_Typeless,
};

enum sResourceMode
{
    // possible bind points

    sRBM_Shader     = 0x00000001,
    sRBM_Index      = 0x00000002,
    sRBM_Vertex     = 0x00000004,
    sRBM_ColorTarget= 0x00000008,
    sRBM_DepthTarget= 0x00000010,
    sRBM_Constant   = 0x00000020,
    sRBM_Unordered  = 0x00000040,
    /*
    sRBM_DrawIndirect=1<<sRB_DrawIndirect,
    sRBM_Raw        = 1<<sRB_Raw,
    sRBM_Structured = 1<<sRB_Structured,
    sRBM_Cubemap    = 1<<sRB_Cubemap,
    sRBM_Mask       = 0x0000ffff,
    */
    // usage (CPU GPU sync method)

    sRU_Static      = 0x00010000,   // load once on creation, never change (immutable)
    sRU_Gpu         = 0x00020000,   // only accessed by GPU (default)
    sRU_MapWrite    = 0x00030000,   // map GPU memory for writing with discard and no-overwrite. not available on OpenGL(ES)2
//    sRU_StageWrite  = 0x00040000,   // use staging to overwrite 
    sRU_MapRead     = 0x00050000,   // map GPU memory for reading
//    sRU_MapWrite2   = 0x00060000,   // directly map with discard and no-overwrite for cpu write access
    sRU_Update      = 0x00070000,   // update by copying from CPU memory
    sRU_SystemMem   = 0x00080000,   // can be locked by CPU for read and write, can only be used with sContext::Copy()
    sRU_Mask        = 0x000f0000,

    // various flags

    sRM_GenerateMips  = 0x00100000,
    sRM_DrawIndirect  = 0x00200000,
    sRM_Raw           = 0x00400000,
    sRM_Structured    = 0x00800000,
    sRM_Cubemap       = 0x01000000,
    sRM_Append        = 0x02000000,
    sRM_Counter       = 0x04000000,
    sRM_Texture       = 0x08000000,
    sRM_Pinned        = 0x10000000, // DX9 only
    sRM_SystemMem2    = 0x20000000, // DX9 only
};

enum sResourceFlags               // you can't have enough space for flags!
{
    sRES_NoMipmaps    = 0x00000001, // set Mipmaps to 1. 
    sRES_Internal     = 0x00000002, // do not allocate buffers
    sRES_AutoMipmap   = 0x00000004,
    sRES_SharedHandle = 0x00010000, // get share handle when creating surface
};

enum sResourceViewFlags
{
    sRVF_Cubemap    = 0x00000001,
};

enum sResourceMapMode                   // MapBuffer()
{
    sRMM_Discard        = 0x00000001,   // do double-buffering in driver and give fresh memory
    sRMM_NoOverwrite    = 0x00000002,   // you promise not to write memory that is used by commands
    sRMM_Sync           = 0x00000003,   // you do double-buffering yourself, it is save to sync with command buffer
};

enum sVertexFlags                       // specify vertex format by a series of uint tokens
{                                       // (xx) <- default type and count
    // bind semantic for this.

    sVF_Position      = 0x00000001,     // (F3) position (always set)
    sVF_Normal        = 0x00000002,     // (F3) normal
    sVF_Tangent       = 0x00000003,     // (F3) tangent
    sVF_BoneIndex     = 0x00000004,     // (F4) 4 indices to matrices
    sVF_BoneWeight    = 0x00000005,     // (F4) 4 weighting factors to matrices
    sVF_Binormal      = 0x00000006,     // (F3) binormal, for certain consoles only, do not use!

    sVF_Color0        = 0x00000008,     // (C4) color
    sVF_Color1        = 0x00000009,     // (C4) color
    sVF_Color2        = 0x0000000a,     // (C4) color
    sVF_Color3        = 0x0000000b,     // (C4) color
    sVF_UV0           = 0x00000010,     // (F2) texture uv's
    sVF_UV1           = 0x00000011,     // (F2) texture uv's
    sVF_UV2           = 0x00000012,     // (F2) texture uv's
    sVF_UV3           = 0x00000013,     // (F2) texture uv's
    sVF_UV4           = 0x00000014,     // (F2) texture uv's
    sVF_UV5           = 0x00000015,     // (F2) texture uv's
    sVF_UV6           = 0x00000016,     // (F2) texture uv's
    sVF_UV7           = 0x00000017,     // (F2) texture uv's
    sVF_Nop           = 0x0000001f,

    sVF_UseMask       = 0x0000001f,     // these flags are also stored in a bitmask, so it's limited to 32.

    sVF_Stream0       = 0x00000000,     // stream descriptor
    sVF_Stream1       = 0x00000100,
    sVF_Stream2       = 0x00000200,
    sVF_Stream3       = 0x00000300,
    sVF_StreamMask    = 0x00000700,     // reserve for 8 streams 
    //  sVF_StreamMax     = 8,              // max number of streams
    sVF_StreamShift   = 8,              // shift to extract sream

    // optional: specify type and count. if missing, defaults are provided.

    sVF_F1            = 0x00010000,     // 1 x float
    sVF_F2            = 0x00020000,     // 2 x float
    sVF_F3            = 0x00030000,     // 3 x float
    sVF_F4            = 0x00040000,     // 4 x float
    sVF_H2            = 0x00050000,     // 2 x sF16
    sVF_H4            = 0x00060000,     // 4 x sF16
    sVF_C4            = 0x00070000,     // 4 x uint8, scaled to 0..1 range
    sVF_I4            = 0x00080000,     // 4 x uint8, not scaled. for bone-indices
    sVF_S2            = 0x00090000,     // 2 x int16, scaled to -1..1 range
    sVF_S4            = 0x000a0000,     // 4 x int16, scaled to -1..1 range
    sVF_I1            = 0x000b0000,     // 1 x uint32, not scaled, for bone-indices

    sVF_TypeMask      = 0x000f0000,

    // other flags

    sVF_InstanceData  = 0x01000000,     // set for each element of the stream

    // special values

    sVF_End           = 0,              // endmarker.
};

#if sConfigRender==sConfigRenderGLES2 || sConfigRender==sConfigRenderGL2
#define sVERTEXCOLOR(x) (((x)&0xff00ff00)|(((x)&0x00ff0000)>>16)|((x)&0x000000ff)<<16)
#else
#define sVERTEXCOLOR(x) (x)
#endif

enum sGeometryModeEnum
{
    sGM_ControlMask     = 0x000000ff,

    sGMP_Error          = 0x00000000,
    sGMP_Points         = 0x00000100,
    sGMP_Lines          = 0x00000200,
    sGMP_Tris           = 0x00000300,
    sGMP_Quads          = 0x00000400,     // only through emulation in sDynamicGeometry! DX does not support this natively!
    sGMP_LinesAdj       = 0x00000500,
    sGMP_TrisAdj        = 0x00000600,
    sGMP_ControlPoints  = 0x00000700,
    sGMP_Mask           = 0x00000f00,
    sGMP_Shift          = 8,

    sGMF_Index16        = 0x00010000,
    sGMF_Index32        = 0x00020000,
    sGMF_IndexMask      = 0x00030000,
};

enum sRenderStateEnum
{
    sMTRL_DepthOff          = 0x00000000, // disable depth buffer completely 
    sMTRL_DepthRead         = 0x00000001, // enable depth test
    sMTRL_DepthWrite        = 0x00000002, // enable depth write
    sMTRL_DepthOn           = 0x00000003, // enable depth buffer completely
    sMTRL_DepthMask         = 0x00000003,

    sMTRL_Stencil           = 0x00000004, // enable stencil read and write

    sMTRL_CullOff           = 0x00000000,
    sMTRL_CullOn            = 0x00000010,
    sMTRL_CullInv           = 0x00000020,
    sMTRL_CullMask          = 0x00000030,

    sMTRL_ScissorDisable    = 0x00000040,
    sMTRL_MultisampleDisable= 0x00000080,
    sMTRL_AALine            = 0x00000100,
    sMTRL_IndividualBlend   = 0x00000200, // for MRT's BlendColor, BlendAlpha and TargetWriteMask
    sMTRL_AlphaToCoverage   = 0x00000400,
    sMTRL_AlphaTest         = 0x00000800,
};

enum sBlendEnum
{
    // source factor

    sMBS_0              = 0x0001,         // 0
    sMBS_1              = 0x0002,         // 1
    sMBS_SC             = 0x0003,         //     source color
    sMBS_SCI            = 0x0004,         // 1 - source color
    sMBS_SA             = 0x0005,         //     source alpha
    sMBS_SAI            = 0x0006,         // 1 - source alpha
    sMBS_DA             = 0x0007,         //     destination alpha
    sMBS_DAI            = 0x0008,         // 1 - destination alpha
    sMBS_DC             = 0x0009,         //     destination color
    sMBS_DCI            = 0x000a,         // 1 - destination color
    sMBS_SA_Sat         = 0x000b,         // min(SA,DAI)
    sMBS_F              = 0x000e,         //     blend factor
    sMBS_FI             = 0x000f,         // 1 - blend factor
    sMBS_S1C            = 0x0010,
    sMBS_IS1C           = 0x0011,
    sMBS_S1A            = 0x0012,
    sMBS_IS1A           = 0x0013,
    sMBS_MASK           = 0x003f,

    // operation (usually add)

    sMBO_Add            = 0x1000,         //     sMBS_??? * Source + sMBD_??? * Dest
    sMBO_Sub            = 0x2000,         //     sMBS_??? * Source - sMBD_??? * Dest
    sMBO_SubR           = 0x3000,         //   - sMBS_??? * Source + sMBD_??? * Dest
    sMBO_Min            = 0x4000,         //     min( Source, Dest )
    sMBO_Max            = 0x5000,         //     max( Source, Dest )
    sMBO_Mask           = 0xf000,

    // destination factor (same as source)

    sMBD_0              = 1<<6,
    sMBD_1              = 2<<6,
    sMBD_SC             = 3<<6,
    sMBD_SCI            = 4<<6,
    sMBD_SA             = 5<<6,
    sMBD_SAI            = 6<<6,
    sMBD_DA             = 7<<6,
    sMBD_DAI            = 8<<6,
    sMBD_DC             = 9<<6,
    sMBD_DCI            = 10<<6,
    sMBD_SA_Sat         = 11<<6,
    sMBD_F              = 14<<6,
    sMBD_FI             = 15<<6,
    sMBD_S1C            = 16<<6,
    sMBD_IS1C           = 17<<6,
    sMBD_S1A            = 18<<6,
    sMBD_IS1A           = 19<<6,
    sMBD_Mask           = 0x0fc0,

    // special values

    sMB_Off             = 0x0000,         // Alphablending disabled

    // common blending equations

    sMB_Alpha           = (sMBS_SA |sMBO_Add |sMBD_SAI),
    sMB_Add             = (sMBS_1  |sMBO_Add |sMBD_1  ),
    sMB_Sub             = (sMBS_1  |sMBO_SubR|sMBD_1  ),
    sMB_Mul             = (sMBS_DC |sMBO_Add |sMBD_0  ),
    sMB_Mul2            = (sMBS_DC |sMBO_Add |sMBD_SC ),
    sMB_MulInv          = (sMBS_0  |sMBO_Add |sMBD_SCI),
    sMB_AddSmooth       = (sMBS_1  |sMBO_Add |sMBD_SCI),
    sMB_PMAlpha         = (sMBS_1  |sMBO_Add |sMBD_SAI),
};

enum sCompareFuncEnum
{
    sCF_0 = 1,
    sCF_LT = 2,
    sCF_EQ = 3,
    sCF_LE = 4,
    sCF_GT = 5,
    sCF_NE = 6,
    sCF_GE = 7,
    sCF_1 = 8,
};

enum sStencilOp
{
    sSO_Keep = 1,
    sSO_Zero = 2,
    sSO_Replace = 3,
    sSO_IncSat = 4,
    sSO_DecSat = 5,
    sSO_Invert = 6,
    sSO_Inc = 7,
    sSO_Dec = 8,
};

enum sSamplerFlags    // min mag mip
{
    sTF_PPP            = 0x00000000,
    sTF_LPP            = 0x00000001,
    sTF_PLP            = 0x00000002,
    sTF_LLP            = 0x00000003,
    sTF_PPL            = 0x00000004,
    sTF_LPL            = 0x00000005,
    sTF_PLL            = 0x00000006,
    sTF_LLL            = 0x00000007,
    sTF_Aniso          = 0x00000008,
    sTF_CmpPPP         = 0x00000010,
    sTF_CmpLPP         = 0x00000011,
    sTF_CmpPLP         = 0x00000012,
    sTF_CmpLLP         = 0x00000013,
    sTF_CmpPPL         = 0x00000014,
    sTF_CmpLPL         = 0x00000015,
    sTF_CmpPLL         = 0x00000016,
    sTF_CmpLLL         = 0x00000017,
    sTF_CmpAniso       = 0x00000018,

    sTF_Point          = sTF_PPP,
    sTF_PointMip       = sTF_PPL,
    sTF_Linear         = sTF_LLP,
    sTF_LinearMip      = sTF_LLL,
    sTF_CmpPoint       = sTF_CmpPPP,
    sTF_CmpPointMip    = sTF_CmpPPL,
    sTF_CmpLinear      = sTF_CmpLLP,
    sTF_CmpLinearMip   = sTF_CmpLLL,

    sTF_FilterMask     = 0x0000001f,

    sTF_TileU          = 0x00000000,
    sTF_ClampU         = 0x00010000,
    sTF_BorderU        = 0x00020000,
    sTF_MirrorU        = 0x00030000,
    sTF_MirrorOnceU    = 0x00040000,
    sTF_UMask          = 0x000f0000,

    sTF_TileV          = 0x00000000,
    sTF_ClampV         = 0x00100000,
    sTF_BorderV        = 0x00200000,
    sTF_MirrorV        = 0x00300000,
    sTF_MirrorOnceV    = 0x00400000,
    sTF_VMask          = 0x00f00000,

    sTF_TileW          = 0x00000000,
    sTF_ClampW         = 0x01000000,
    sTF_BorderW        = 0x02000000,
    sTF_MirrorW        = 0x03000000,
    sTF_MirrorOnceW    = 0x04000000,
    sTF_WMask          = 0x0f000000,

    sTF_Tile           = 0x00000000,
    sTF_Clamp          = 0x01110000,
    sTF_Border         = 0x02220000,
    sTF_Mirror         = 0x03330000,
    sTF_MirrorOnce     = 0x04440000,
};

enum sShaderTypeEnum
{
    sST_Vertex = 0,
    sST_Hull,
    sST_Domain,
    sST_Geometry,
    sST_Pixel,
    sST_Compute,
    sST_Max,
};

enum sDrawFlagsEnum
{
    sDF_Ranges          = 0x0001,   // use list of ranges instead of drawing all
    sDF_Instances       = 0x0002,   // use instancing
    sDF_BlendFactor     = 0x0004,   // update blendfactor before drawing
    sDF_VertexOffset    = 0x0008,   // update vertex offsets
    sDF_IndexOffset     = 0x0010,   // update index offset
    sDF_Compute         = 0x0020,   // dispatch compute shader instead of normal drawing
    sDF_Arrays          = 0x0040,   // use vertex/index arrays instead of geometry.
    sDF_Indirect        = 0x0080,   // use Indirect buffer - DrawIndexedInstancedIndirect()
    sDF_Indirect2       = 0x0100,   // use IndirectBuffer IndirectBufferOffset
};

enum sTargetFlags
{
    sTAR_ClearColor     = 0x0001,   // clear color buffer
    sTAR_ClearDepth     = 0x0002,   // clear depth and stencil buffer
    sTAR_ClearAll       = 0x0003,

    sTAR_ReadZ          = 0x0004,   // allow read access to zbuffer after rendering
    sTAR_OverwriteAll   = 0x0008,   // previous content will be wiped out
    sTAR_GenMipmaps     = 0x0010,   // automatically generate mipmaps
    sTAR_Scissor        = 0x0020,   // set scissor to window
};

enum sCopyTextureFlags
{
    sCT_Filter      = 0x0001,         // use linear filtering if available
    sCT_GenMipmaps  = 0x0002,         // generate mipmaps on destination
};

/****************************************************************************/
/***                                                                      ***/
/***    parameter structures (not dependend on graphics limits)           ***/
/***                                                                      ***/
/****************************************************************************/

struct sResPara
{
    int Mode;                      // sRD_??? | sRB_??? | sRU_???
    int Format;                    // sRF_??? = sRFC_??? | sRFB_??? | sRFT_???
    int Flags;                     // sRES_???
    int BitsPerElement;            // size in bits of each element.
    int SizeX;
    int SizeY;                     // for textures and cubes, otherwise 0. cubes are always square
    int SizeZ;                     // for volumes, otherwise 0
    int SizeA;                     // array size
    int Mipmaps;                   // 0=all mipmaps, 1=only top mipmap.
    int MSCount;                   // 0 or 1 for off, 2,4,8 to enable multisampling
    int MSQuality;                 // 0 for worst, -1 for best, or specify manually
    uint SharedHandle;              // create surface shared

    sResPara();
    sResPara(int mode,int format,int flags,int sx,int sy,int sz=0,int sa=0);
    void UpdatePara();

    int GetMaxMipmaps();           // considering SizeX,SizeY,SizeZ
    int GetDimensions();           // 1,2 or 3 considering SizeX,SizeY,SizeZ
    uptr GetTextureSize(int mipmap);
    int GetBlockSize();            // 4 for dxt / pvrtc / etc
    bool operator==(const sResPara &b) const;
};

struct sResTexPara : public sResPara
{
    sResTexPara(int format,int sx,int sy,int mipmaps=0);
};

struct sResBufferPara : public sResPara
{
    sResBufferPara(int bind,int bytesperelement,int elements,int format=sRFC_Unknown | sRFB_Unknown | sRFT_Unknown);
};

struct sResUpdatePartialWindow
{
    int MinX,MaxX;
    int MinY,MaxY;
    int MinZ,MaxZ;

    sResUpdatePartialWindow();
    sResUpdatePartialWindow(const sRect &r);
};

/****************************************************************************/

struct sSamplerStatePara
{
    int Flags;
    float MipLodBias;
    float MinLod;
    float MaxLod;
    int MaxAnisotropy;
    int CmpFunc;
    sVector4 Border;

    sSamplerStatePara();
    sSamplerStatePara(int flags,float lodbias=0);
};

/****************************************************************************/

struct sSetTexturePara
{
    int BindIndex;
    sShaderTypeEnum BindShader;
    sResource *Texture;
    sSamplerState *Sampler;
    int SliceStart;
    int SliceCount;

    sSetTexturePara();
    sSetTexturePara(sShaderTypeEnum bindshader,int bindindex,sResource *tex,sSamplerState *sam);
    sSetTexturePara(sShaderTypeEnum bindshader,int bindindex,sResource *tex,sSamplerState *sam,int slicestart,int slicecount);
};

/****************************************************************************/

struct sAllShaderPara
{
    const uint8 *Data[sST_Max];
    int Size[sST_Max];

    void Clear() { sClear(*this); }
};

struct sAllShaderPermPara
{
    sptr PermMax;
    const sAllShaderPara *Shaders;

    void Clear() { sClear(*this); }

    const sAllShaderPara &Get(int n) { sASSERT(n>=0 && n<PermMax); return Shaders[n]; }
};

/****************************************************************************/

struct sCopyTexturePara
{
    int Flags;

    sResource *Source;
    sRect SourceRect;
    int SourceCubeface;

    sResource *Dest;
    sRect DestRect;
    int DestCubeface;

    sCopyTexturePara();
    sCopyTexturePara(int flags,sResource *d,sResource *s);
};

/****************************************************************************/

struct sScreenMode                // specify screenmode
{
    int Flags;                     // sSM_???
    int SizeX;                     // 0 for native
    int SizeY;                     // 0 for native
    int Monitor;                   // monitor id, as of sGetDisplayInfo()
    int ColorFormat;               // format of the color buffer. this is read-only.
    int DepthFormat;               // format of the depth buffer. set to 0 to disable creation of depth buffer
    int Frequency;                 // 0 for default frequency (recommended)
//    int Display;                   // specify display to open fullscreen. set to -1 for default.
    int BufferSequence;            // 0 : color buffer is lost. 1 : color buffer is preserved. n>=2 : color buffer is preserved, but there are 'n' buffers flipping in sequence 
    sString<256> WindowTitle;       // 

    sScreenMode();
    sScreenMode(int flags,const char *name,int sx=0,int sy=0);
};

/****************************************************************************/

struct sRenderStatePara;    // forward graphics limit dependend structs
struct sDrawRange;
struct sDrawPara;
struct sTargetPara;
struct sCopyTexturePara;

class sResource;
class sContext;
class sAdapter;
class sScreen;

/****************************************************************************/
/***                                                                      ***/
/***   include private structures                                         ***/
/***                                                                      ***/
/****************************************************************************/

} // namespace

#if sConfigRender==sConfigRenderNull
#include "Altona2/Libs/Base/GraphicsNull.hpp"
#endif
#if sConfigRender==sConfigRenderDX9
#include "Altona2/Libs/Base/GraphicsDx9.hpp"
#endif
#if sConfigRender==sConfigRenderDX11
#include "Altona2/Libs/Base/GraphicsDx11.hpp"
#endif
#if sConfigRender==sConfigRenderGL2 || sConfigRender==sConfigRenderGLES2
#include "Altona2/Libs/Base/GraphicsGl2.hpp"
#endif

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***    parameter structures (dependend on graphics limits)               ***/
/***                                                                      ***/
/****************************************************************************/

struct sRenderStatePara
{
    int Flags;

    uint8 DepthFunc;
    uint8 AlphaFunc;
    uint8 AlphaRef;
    uint8 SampleMask;

    // blending and multisampling

    uint16 BlendColor[sGfxMaxTargets];
    uint16 BlendAlpha[sGfxMaxTargets];
    uint8 TargetWriteMask[sGfxMaxTargets];
    uint BlendFactor;

    // stencil stuff

    uint8 StencilReadMask;
    uint8 StencilWriteMask;
    uint8 StencilRef;
    uint8 pad2;

    uint8 FrontStencilFailOp;
    uint8 FrontStencilDepthFailOp;
    uint8 FrontStencilPassOp;
    uint8 FrontStencilFunc;
    uint8 BackStencilFailOp;
    uint8 BackStencilDepthFailOp;
    uint8 BackStencilPassOp;
    uint8 BackStencilFunc;

    // depth bias

    int DepthBias;
    float DepthBiasSlope;
    float DepthBiasClamp;

    // initializers

    sRenderStatePara();
    sRenderStatePara(int flags,uint16 blendcolor);
};

/****************************************************************************/

struct sDrawRange
{
    int Start;
    int End;
    sDrawRange() { Start=End=0; }
    sDrawRange(int s,int e) { Start=s;End=e; }
};

struct sDrawPara
{
    int Flags;                      // enable special features

    sGeometry *Geo;                 // geometry
    int VertexOffset[sGfxMaxStream];// sDF_VertexOffset -> byte offset into vertex buffers
    int IndexOffset;                // sDF_IndexOffset -> byte offset into index buffers
    int BaseVertexIndex;            // added to all indices
    int InstanceCount;              // sDF_Instances -> enable instancing
    int RangeCount;                 // sDF_Ranges -> draw index ranges
    sResource *IndirectBuffer;      // sDF_Indirect -> use this buffer;
    int IndirectBufferOffset;       // sDF_Indirect -> filled by sAdapter::FillIndirectBuffer
    int IndirectMask;               // used by FillIndirectBuffer() to figure out which shader resources need to be considered
    int IndirectUavMask;            // used by FillIndirectBuffer() to figure out which shader resources need to be considered
    int IndirectAdd;                // sDF_Indirect : add the the instance count after multiplication
    int IndirectThreadGroupSize;    // sDF_Indirect : thread group size to divide instance count for compute shaders
    sDrawRange *Ranges;

    sMaterial *Mtrl;                // material
    sVector4 BlendFactor;           // sDF_BlendFactor -> override blend factor

    sCBufferBase *CBs[sGfxMaxCBs];  // constant buffers
    int CBCount;                    // number of constant buffers used

    int SizeX,SizeY,SizeZ;          // sDF_Compute -> thread group counts
    uint InitialCount[sGfxMaxUav];  // initial count for UAV's

    void *VertexArray;              // sDF_Arrays: vertex data
    void *IndexArray;               // sDF_Arrays: index data (or 0)
    int VertexArrayCount;           // sDF_Arrays: vertex count
    int IndexArrayCount;            // sDF_Arrays: index count
    sVertexFormat *VertexArrayFormat; // sDF_Arrays: format for vertex array
    int IndexArraySize;             // sDF_Arrays: 2 for 16 bit or 4 for 32 bit (or 0 for no indices)

    sDrawPara();
    sDrawPara(sGeometry *geo,sMaterial *mtrl,sCBufferBase *cb0=0,sCBufferBase *cb1=0,sCBufferBase *cb2=0,sCBufferBase *cb3=0);
    sDrawPara(int xs,int ys,int zs,sMaterial *mtrl,sCBufferBase *cb0=0,sCBufferBase *cb1=0,sCBufferBase *cb2=0,sCBufferBase *cb3=0);
};

struct sDrawRangePara : public sDrawPara
{
    sDrawRange Range;

    sDrawRangePara();
    sDrawRangePara(int end,sGeometry *geo,sMaterial *mtrl,sCBufferBase *cb0=0,sCBufferBase *cb1=0,sCBufferBase *cb2=0,sCBufferBase *cb3=0);
    sDrawRangePara(int start,int end,sGeometry *geo,sMaterial *mtrl,sCBufferBase *cb0=0,sCBufferBase *cb1=0,sCBufferBase *cb2=0,sCBufferBase *cb3=0);
};

/****************************************************************************/

struct sTargetPara
{
public:
    int Flags;
    sResource *Depth;                // depth&stencil buffer, if any
    sResource *Target[sGfxMaxTargets]; // color buffers
    int Mipmap;                    // mipmap, for all buffers the same

    sRect Window;
    int SizeX;
    int SizeY;
    float Aspect;
    float ClearZ;
    int ClearStencil;
    sVector4 ClearColor[sGfxMaxTargets];

    sTargetPara();
    sTargetPara(int flags,uint color,sScreen *screen);
    sTargetPara(int flags,uint color,sResource *target,sResource *depth);

    void UpdateSize();
};



/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   The Interface                                                      ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/***                                                                      ***/
/***   Screenmodes                                                        ***/
/***                                                                      ***/
/****************************************************************************/

// this only works on DX11 / DX9

struct sDisplayInfo
{
    int DisplayNumber;
    int AdapterNumber;
    int DisplayInAdapterNumber;
    sString<256> AdapterName;
    sString<256> MonitorName;
    int SizeX;
    int SizeY;
};

/****************************************************************************/
/***                                                                      ***/
/***   Stats                                                              ***/
/***                                                                      ***/
/****************************************************************************/

struct sGfxStats
{
    int Batches;                       // calls to sDraw()
    int Instances;                     // when not using sDF_Instanced, this is not increased
    int Ranges;                        // when not using sDF_Ranges, this is increased by 1
    int Indices;                       // when not using index buffer, the vertex count is added
    int Primitives;                    // lines, triangles, ...
    float LastFrameDuration;             // in seconds
    float FilteredFrameDuration;         // in seconds
};

/****************************************************************************/
/***                                                                      ***/
/***   Hooks                                                              ***/
/***                                                                      ***/
/****************************************************************************/

extern sHook sPreFrameHook;
extern sHook sBeginFrameHook;
extern sHook sEndFrameHook;
extern sHook sBuffersLostHook;        // all non-static buffers and last frames color buffer are lost.
extern sHook sBeforeResetDeviceHook;
extern sHook sAfterResetDeviceHook;

/****************************************************************************/
/***                                                                      ***/
/***   Buffers & Resources                                                ***/
/***                                                                      ***/
/****************************************************************************/

class sResource : public sResourcePrivate
{
public:
    sResource(sAdapter *adapter,const sResPara &,const void *data=0,uptr size=0);
#if sConfigRender==sConfigRenderGLES2
    sResource(sAdapter *adapter,const sResPara &,uint glName,bool externalOES);
#endif
#if sConfigRender==sConfigRenderGLES2 || sConfigRender==sConfigRenderGL2
    void UpdateTexture(void *data, int sizex, int sizey,int miplevel);
#endif

    ~sResource();
    sAdapter *GetAdapter() { return Adapter; }
    uint GetSharedHandle() { return SharedHandle; }

    // nooverwrite-style upload for linear resource like buffers

    void MapBuffer0(sContext *ctx,void **data,sResourceMapMode mode);
    void MapTexture0(sContext *ctx,int mipmap,int index,void **data,int *pitch,int *pitch2,sResourceMapMode mode);
    void Unmap(sContext *ctx);

    void MapBuffer0(void **data,sResourceMapMode mode);
    void MapTexture0(int mipmap,int index,void **data,int *pitch,int *pitch2,sResourceMapMode mode);
    void Unmap();

    void UpdateBuffer(void *data,int startbyte,int bytesize);
    void UpdateTexture(void *data,int miplevel,int arrayindex);
    void ReadTexture(void *data,uptr size);

    template <class T> void MapBuffer(sContext *ctx,T **data,sResourceMapMode mode) 
    { MapBuffer0(ctx,(void **)data,mode); }
    template <class T> void MapTexture(sContext *ctx,int mipmap,int index,T **data,int *pitch,int *pitch2,sResourceMapMode mode) 
    { MapTexture0(ctx,mipmap,index,(void **)data,pitch,pitch2,mode); }
    template <class T> void MapBuffer(T **data,sResourceMapMode mode) 
    { MapBuffer0((void **)data,mode); }
    template <class T> void MapTexture(int mipmap,int index,T **data,int *pitch,int *pitch2,sResourceMapMode mode) 
    { MapTexture0(mipmap,index,(void **)data,pitch,pitch2,mode); }

    void DebugDumpBuffer(int columns,const char *type="float",int max=0,const char *msg = 0);
};

/****************************************************************************/
/***                                                                      ***/
/***   Geometry and Vertex Format                                         ***/
/***                                                                      ***/
/****************************************************************************/

class sVertexFormat : public sVertexFormatPrivate
{
    friend class sContext;
    friend class sAdapter;
    sVertexFormat(sAdapter *adapter,const uint *desc,int count,const sChecksumMD5 &hash);
    ~sVertexFormat();

    sAdapter *Adapter;
    sChecksumMD5 Hash;
    int Count;
    const uint *Desc;
public:
    const uint *GetDesc() { return Desc; }
    int GetStreamCount();
    int GetStreamSize(int stream);
    sAdapter *GetAdapter() { return Adapter; }
};

/****************************************************************************/

class sGeometry : public sGeometryPrivate
{
    friend void sUpdateGfxStats(const sDrawPara &dp);
    friend class sContext;
    sResource *Index;
    sResource *Vertex[sGfxMaxStream];
    bool DeleteIndex;
    bool DeleteVertex[sGfxMaxStream];
    sVertexFormat *Format;
    int Mode;
    sAdapter *Adapter;

    int IndexCount;
    int IndexFirst;
    int VertexOffset;
    int VertexCount;
    int Divider;

    void PrivateInit();
    void PrivateExit();
public:
    sGeometry();
    sGeometry(sAdapter *ada);
    ~sGeometry();

    void SetIndex(sResource *,bool del=1);
    void SetIndex(const sResBufferPara &,const void *data,bool del=1);
    void SetVertex(sResource *,int stream=0,bool del=1);
    void SetVertex(const sResBufferPara &,const void *data,int stream=0,bool del=1);
    void Prepare(sVertexFormat *,int mode,int ic,int i0,int vc,int v0);
    void Prepare(sVertexFormat *,int mode);

    sResource *IB();
    sResource *VB(int stream=0);
};

/****************************************************************************/
/***                                                                      ***/
/***   Shaders                                                            ***/
/***                                                                      ***/
/****************************************************************************/

class sShader : public sShaderPrivate
{
    friend class sContext;
    friend class sAdapter;
    sShader(sAdapter *adapter,sShaderBinary *bin);
    ~sShader();
    sShaderTypeEnum Type;
    sChecksumMD5 Hash;
    sAdapter *Adapter;
public:
    sShaderTypeEnum GetType() { return Type; }
};

class sShaderBinary
{
public:
    sShaderBinary();
    ~sShaderBinary();
    sShaderTypeEnum Type;
    sString<16> Profile;
    bool DeleteData;
    const uint8 *Data;
    uptr Size;
    sChecksumMD5 Hash;

    void CalcHash();
};

sShaderBinary *sCompileShader(sShaderTypeEnum type,const char *profile,const char *source,sTextLog *errors);

class sCBufferBase : public sCBufferBasePrivate
{
    friend class sContext;
    friend class sDispatchContext;
    int Size;
    int Type;
    int Slot;
    sAdapter *Adapter;

public:
    sCBufferBase(sAdapter *adapter,int size,sShaderTypeEnum type,int slot);
    sCBufferBase(sAdapter *adapter,int size,int typesmask,int slot);
    ~sCBufferBase();

    void Map(sContext *ctx,void **);
    void Unmap(sContext *ctx);
    void Map(void **data);
    void Unmap();

    template<class T> void Map(sContext *ctx,T **x) { Map(ctx,(void **)x); }
    template<class T> void Map(T **x) { Map((void **)x); }
};

template <class Type_>
class sCBuffer : public sCBufferBase
{
public:
    sCBuffer(sAdapter *adapter,sShaderTypeEnum type,int slot) : sCBufferBase(adapter,sizeof(Type_),type,slot) { Data=0; }
    sCBuffer(sAdapter *adapter,int typesmask,int slot) : sCBufferBase(adapter,sizeof(Type_),typesmask,slot) { Data=0; }
    Type_ *Data;
    void Map(sContext *ctx) { sCBufferBase::Map(ctx,(void **)&Data); }
    void Map() { sCBufferBase::Map((void **)&Data); }
};

/****************************************************************************/
/***                                                                      ***/
/***   Material Base Class                                                ***/
/***                                                                      ***/
/****************************************************************************/

class sRenderState : public sRenderStatePrivate
{
    friend class sContext;
    friend class sAdapter;
    sChecksumMD5 Hash;
    sAdapter *Adapter;

    sRenderState(sAdapter *adapter,const sRenderStatePara &,const sChecksumMD5 &hash);
    ~sRenderState();
public:
};

/****************************************************************************/

class sSamplerState : public sSamplerStatePrivate
{
    friend class sContext;
    friend class sAdapter;
    sSamplerState(sAdapter *adapter,const sSamplerStatePara &,const sChecksumMD5 &hash);
    ~sSamplerState();
    sChecksumMD5 Hash;
    sAdapter *Adapter;
public:
};

/****************************************************************************/

class sMaterial : public sMaterialPrivate
{
protected:
    friend class sContext;
    sRenderState *RenderState;
    sVertexFormat *Format;
    sShader *Shaders[sST_Max];
    sAdapter *Adapter;

    virtual void PrivateSetBase(sAdapter *adapter);
public:
    sMaterial();
    sMaterial(sAdapter *ada);
    virtual ~sMaterial();

    void SetState(sRenderState *);
    void SetState(const sRenderStatePara &para);
    void SetTexture(sShaderTypeEnum shader,int index,sResource *tex,sSamplerState *sam=0,int slicestart=0,int slicecount=1);
    void SetTexture(sShaderTypeEnum shader,int index,sResource *tex,const sSamplerStatePara &sam=0,int slicestart=0,int slicecount=1);
    void UpdateTexture(sShaderTypeEnum shader,int index,sResource *tex,int slicestart=0,int slicecount=1);
    sResource *GetTexture(sShaderTypeEnum shader,int index);
    sResource *GetUav(sShaderTypeEnum shader,int index);
    void SetUav(sShaderTypeEnum shader,int index,sResource *uav);
    void ClearTextureSamplerUav();
    void SetTexturePS(int index,sResource *tex,sSamplerState *sam);
    void SetTexturePS(int index,sResource *tex,const sSamplerStatePara &sampler);
    void UpdateTexturePS(int index,sResource *tex);

    void SetShader(sShader *,sShaderTypeEnum type);
    void SetShaders(sAdapter *adapter,const sAllShaderPara &as);
    void SetShaders(const sAllShaderPara &as);

    void Prepare(sAdapter *adapter,sVertexFormat *fmt);
    void Prepare(sVertexFormat *fmt);
};

/****************************************************************************/

class sDispatchContext : public sDispatchContextPrivate
{
public:
    sDispatchContext();
    ~sDispatchContext();

    void SetResource(int slot,sResource *res);
    void SetSampler(int slot, sSamplerState *ss);
    void SetUav(int slot,sResource *res,uint hidden=~0);
    void SetCBuffer(sCBufferBase *cb);

    uint UavHiddens[sGfxMaxUav];
};

/****************************************************************************/
/***                                                                      ***/
/***   Simple fixed function materials                                    ***/
/***                                                                      ***/
/****************************************************************************/

// new matrix / vector

struct sFixedMaterialLightPara
{
    sVector3 LightDirWS[4];
    uint LightColor[4];
    uint AmbientColor;

    sFixedMaterialLightPara();
};

struct sFixedMaterialLightVC
{
    sMatrix44 MS2SS;
    sVector4 LightDirX;
    sVector4 LightDirY;
    sVector4 LightDirZ;
    sVector4 LightColR;
    sVector4 LightColG;
    sVector4 LightColB;
    sVector3 Ambient;
    float pad;

    void Set(const struct sViewport &vp,const sFixedMaterialLightPara &para);
};

struct sFixedMaterialVC
{
    sMatrix44 MS2SS;

    void Set(const sViewport &vp);
};

/****************************************************************************/

enum sFixedMaterialFlags
{
    sFMF_TexAlphaOnly =     0x0001, // set texture color to 1. useful for alpha only texures.
    sFMF_TexMonochrome =    0x0002, // use red for rgb and 1 for alpha
    sFMF_TexRedAlpha =      0x0003, // multiply all by red, for premultplied alpha. this is prefered to TexAlphaOnly becase of DXGI does not correctly support alpha only textures
    sFMF_TexTexMask =       0x0003,
};

class sFixedMaterial : public sMaterial
{
protected:
    void SetShader(sShader *sh,sShaderTypeEnum type) { sMaterial::SetShader(sh,type); }
    void SetShaders(sAdapter *adapter,const sAllShaderPara &as) { sMaterial::SetShaders(adapter,as); }
    void PrivateSetBase(sAdapter *);   // pos, [norm], [color], [uv0]
public:
    sFixedMaterial();
    sFixedMaterial(sAdapter *ada);
    int FMFlags;
};

/****************************************************************************/
/***                                                                      ***/
/***   Queries                                                            ***/
/***                                                                      ***/
/****************************************************************************/

enum sGpuQueryKind
{
    sGQK_Error = 0,
    sGQK_Timestamp,                 // Issue(), uint64
    sGQK_Frequency,                 // Issue(), uint64
    sGQK_Occlusion,                 // Begin()/End(), uint or uint64
    sGQK_PipelineStats,             // Begin()/End(), sPipelineStats, DX11 only
};

struct sPipelineStats 
{
    uint64 IAVertices;                // vertices read by IA
    uint64 IAPrimitives;              // primitives generated by IA
    uint64 VS;                        // vs invocations
    uint64 GS;                        // gs invocations
    uint64 GSPrimitives;              // primitives generated by gs
    uint64 Primitives;                // primitives send to the rasterizer
    uint64 ClippedPrimitives;         // primitives after clipping and culling
    uint64 PS;                        // ps invocations
    uint64 HS;                        // hs invocations
    uint64 DS;                        // ds invocations
    uint64 CS;                        // cs invocations
};

class sGpuQuery : public sGpuQueryPrivate
{
    sGpuQueryKind Kind;
public:
    sGpuQuery(sAdapter *adapter,sGpuQueryKind kind);
    ~sGpuQuery();
    void Begin(sContext *ctx);
    void End(sContext *ctx);
    void Issue(sContext *ctx);
    bool GetData(uint &);
    bool GetData(uint64 &);
    bool GetData(sPipelineStats &);
};

/****************************************************************************/
/***                                                                      ***/
/***   The Big Ones                                                       ***/
/***                                                                      ***/
/****************************************************************************/

class sContext : public sContextPrivate
{
public:
    void BeginTarget(const sTargetPara &tar);
    void EndTarget();
    void SetScissor(bool enable,const sRect &r);
    void Resolve(sResource *dest,sResource *src);
    void Copy(const sCopyTexturePara &cp);
    void Draw(const sDrawPara &dp);


    // compute

    void FillIndirectBuffer(sDrawPara &dp);
    void CopyHiddenCounter(sResource *dest,sResource *src,sInt byteoffset);
    void CopyHiddenCounter(sCBufferBase *dest,sResource *src,sInt byteoffset);

    void BeginDispatch(sDispatchContext *dp);
    void EndDispatch(sDispatchContext *dp);
    void Dispatch(sShader *sh,int x,int y,int z);
    void Dispatch(sShader *sh,int x,int y,int z,const sPerfMonSection &perf,bool debug=true);
    void Dispatch(sMaterial *sh,int x,int y,int z);
    void UnbindCBuffer(sDispatchContext *dp);
    void RebindCBuffer(sDispatchContext *dp);

    void ClearUav(sResource *r,const uint v[4]);
    void ClearUav(sResource *r,const float v[4]);
    void ComputeSync();
    sResource *GetComputeSyncUav();
};


namespace Private
{
 extern int DefaultRTColorFormat;
 extern int DefaultRTDepthFormat;
 extern bool DefaultRTDepthIsUsableInShader;
}


class sAdapter : public sAdapterPrivate
{
    sArray<sVertexFormat *> AllVertexFormats;
    sArray<sShader *> AllShaders;
    sArray<sRenderState *> AllRenderStates;
    sArray<sSamplerState *> AllSamplerStates;

    void DeleteGfxStates();
public:
    sAdapter();
    ~sAdapter();
    void Init();

    sVertexFormat *CreateVertexFormat(const uint *desc);
    sShader *CreateShader(sShaderBinary *bin);
    sShader *CreateShader(const sAllShaderPara &shaders,sShaderTypeEnum shader);
    sRenderState *CreateRenderState(const sRenderStatePara &sRenderStatePara);
    sSamplerState *CreateSamplerState(const sSamplerStatePara &para);

    void GetBestMultisampling(const sResPara &respara,int &count,int &quality);
    int GetMultisampleQuality(const sResPara &respara,int count);

    int GetDefaultRTColorFormat(){return Private::DefaultRTColorFormat;}
    int GetDefaultRTDepthFormat(){return Private::DefaultRTDepthFormat;}
    bool IsDefaultRTDepthUsableInShader(){return Private::DefaultRTDepthIsUsableInShader;}

    sVertexFormat *FormatP;
    sVertexFormat *FormatPT;
    sVertexFormat *FormatPWT;
    sVertexFormat *FormatPWCT;
    sVertexFormat *FormatPC;
    sVertexFormat *FormatPCT;
    sVertexFormat *FormatPCTT;
    sVertexFormat *FormatPN;
    sVertexFormat *FormatPNT;
    sVertexFormat *FormatPNTT;
    sVertexFormat *FormatPTT;

    sDynamicGeometry *DynamicGeometry;

    // dx11 helpers

    sResource *CreateBufferRW(int count,int format=sRFC_RGBA|sRFB_32|sRFT_Float,int additionalbind=0);
    sResource *CreateStructRW(int count,int bytesperelement,int additionalbind=0);
    sResource *CreateCounterRW(int count,int bytesperelement,int additionalbind=0);
    sResource *CreateAppendRW(int count,int bytesperelement,int additionalbind=0);
    sResource *CreateRawRW(int bytecount,int additionalbind=0);

    sResource *CreateStruct(int count,int bytesperelement,int additionalbind,void *data);
    sResource *CreateBuffer(int count,int format,int additionalbind,void *data,int size);
};

struct sDragDropInfo
{
    int PosX;
    int PosY;
    sArray<char *> Names;
};

class sScreen : public sScreenPrivate
{
    sDelegate1<void,const sDragDropInfo *> DragDropDelegate;
public:
    sScreen(const sScreenMode &sm);
    ~sScreen();
    void GetSize(int &x,int &y) const;

    void WindowControl(WindowControlEnum mode);
    sResource *GetScreenColor();
    sResource *GetScreenDepth();
    void SetTitle(const char *title);
    void GetMode(sScreenMode &mode) { mode=ScreenMode; }
    int GetDefaultColorFormat() { return ScreenMode.ColorFormat; }
    int GetDefaultDepthFormat() { return ScreenMode.DepthFormat; }

    void SetDragDropCallback(sDelegate1<void,const sDragDropInfo *> del);
    void ClearDragDropCallback();
    void CallDragDropCallback(const sDragDropInfo *ddi) { DragDropDelegate.Call(ddi); }
};

/****************************************************************************/
/***                                                                      ***/
/***   Format Conversion Helpers                                          ***/
/***                                                                      ***/
/****************************************************************************/

inline void sFloatToHalf(float f0,float f1,uint *h)
{
    uint i = *(const uint *)&f0;
    ((uint16*)h)[0] = uint16(((i&0x80000000)>>16)|(((i&0x7f800000)>>13)-(127<<10)+(15<<10))|((i&0x007fe000)>>13));
    i = *(const uint *)&f1;
    ((uint16*)h)[1] = uint16(((i&0x80000000)>>16)|(((i&0x7f800000)>>13)-(127<<10)+(15<<10))|((i&0x007fe000)>>13));
}

inline uint sFloatToHalfSafe(float f0)
{
    // optimize me!
    uint v = *(const uint*)&f0;
    uint s = v >> 31;
    uint e = sClamp<int>((int(v & 0x7f800000) >> 23) - 0x80, -16, 15) + 16;
    uint m = (v & 0x007fffff) >> 13;
    return m | (e << 10) | (s << 15);
}

inline void sFloatToSNorm(float f0,float f1,uint *s)
{
    ((uint16*)s)[0] = uint16(int((f0+1)*32767.5f)-0x8000)&0xffff;
    ((uint16*)s)[1] = uint16(int((f1+1)*32767.5f)-0x8000)&0xffff;
}

/****************************************************************************/
/***                                                                      ***/
/***   Index Buffer Helpers                                               ***/
/***                                                                      ***/
/****************************************************************************/

inline void sQuad(uint16 *&ip,int o,int a,int b,int c,int d)
{
    *ip++ = o+a;
    *ip++ = o+b;
    *ip++ = o+c;
    *ip++ = o+a;
    *ip++ = o+c;
    *ip++ = o+d;
}

inline void sQuadR(uint16 *&ip,int o,int d,int c,int b,int a)
{
    sQuad(ip, o, a, b, c, d);
}

inline void sQuad(uint *&ip,int o,int a,int b,int c,int d)
{
    *ip++ = o+a;
    *ip++ = o+b;
    *ip++ = o+c;
    *ip++ = o+a;
    *ip++ = o+c;
    *ip++ = o+d;
}

inline void sQuadR(uint *&ip,int o,int d,int c,int b,int a)
{
    sQuad(ip, o, a, b, c, d);
}

/****************************************************************************/
/***                                                                      ***/
/***   Standard Vertex Formats                                            ***/
/***                                                                      ***/
/****************************************************************************/

// P = position
// W = w-part of position
// N = Normal
// C = Color
// T = Texcoords (2 dimensional)

struct sVertexP
{
    sVertexP() {}
    sVertexP(float x,float y,float z)
    { px=x; py=y; pz=z; }

    float px,py,pz;
    void Init(float x,float y,float z) 
    { px=x; py=y; pz=z; }
    void Init(const sVector41 &p) 
    { px=p.x; py=p.y; pz=p.z; }
    void Set(float x,float y,float z) 
    { px=x; py=y; pz=z; }
    void Set(const sVector41 &p) 
    { px=p.x; py=p.y; pz=p.z; }
};

struct sVertexPT
{
    sVertexPT() {}
    sVertexPT(float x,float y,float z,float u,float v)
    { px=x; py=y; pz=z; u0=u; v0=v; }

    float px,py,pz;
    float u0,v0;

    void Init(float x,float y,float z,float u,float v) 
    { px=x; py=y; pz=z; u0=u; v0=v; }
    void Init(const sVector41 &p,float u,float v) 
    { px=p.x; py=p.y; pz=p.z; u0=u; v0=v; }
    void Set(float x,float y,float z,float u,float v) 
    { px=x; py=y; pz=z; u0=u; v0=v; }
    void Set(const sVector41 &p,float u,float v) 
    { px=p.x; py=p.y; pz=p.z; u0=u; v0=v; }
};

struct sVertexPWT
{
    sVertexPWT() {}
    sVertexPWT(float x,float y,float z,float w,float u,float v)
    { px=x; py=y; pz=z; pw=w; u0=u; v0=v; }

    float px,py,pz,pw;
    float u0,v0;

    void Init(float x,float y,float z,float w,float u,float v) 
    { px=x; py=y; pz=z; pw=w; u0=u; v0=v; }
    void Init(const sVector4 &p,float u,float v) 
    { px=p.x; py=p.y; pz=p.z; pw=p.w; u0=u; v0=v; }
    void Set(float x,float y,float z,float w,float u,float v) 
    { px=x; py=y; pz=z; pw=w; u0=u; v0=v; }
    void Set(const sVector4 &p,float u,float v) 
    { px=p.x; py=p.y; pz=p.z; pw=p.w; u0=u; v0=v; }
};

struct sVertexPWCT
{
    float px,py,pz,pw;
    uint c0;
    float u0,v0;

    void Init(float x,float y,float z,float w,uint c,float u,float v) 
    { px=x; py=y; pz=z; pw=w; c0=c; u0=u; v0=v; }
    void Init(const sVector4 &p,uint c,float u,float v) 
    { px=p.x; py=p.y; pz=p.z; pw=p.w; c0=c; u0=u; v0=v; }
    void Set(float x,float y,float z,float w,uint c,float u,float v) 
    { px=x; py=y; pz=z; pw=w; c0=c; u0=u; v0=v; }
    void Set(const sVector4 &p,uint c,float u,float v) 
    { px=p.x; py=p.y; pz=p.z; pw=p.w; c0=c; u0=u; v0=v; }
};

struct sVertexPC
{
    float px,py,pz;
    uint c0;

    void Init(float x,float y,float z,uint c) 
    { px=x; py=y; pz=z; c0=c; }
    void Init(const sVector41 &p,uint c) 
    { px=p.x; py=p.y; pz=p.z; c0=c; }
    void Set(float x,float y,float z,uint c) 
    { px=x; py=y; pz=z; c0=c; }
    void Set(const sVector41 &p,uint c) 
    { px=p.x; py=p.y; pz=p.z; c0=c; }
};

struct sVertexPCT
{
    sVertexPCT() {}
    sVertexPCT(float x,float y,float z,uint c,float u,float v)
    { px=x; py=y; pz=z; c0=c; u0=u; v0=v; }

    float px,py,pz;
    uint c0;
    float u0,v0;

    void Init(float x,float y,float z,uint c,float u,float v) 
    { px=x; py=y; pz=z; c0=c; u0=u; v0=v; }
    void Init(const sVector41 &p,uint c,float u,float v) 
    { px=p.x; py=p.y; pz=p.z; c0=c; u0=u; v0=v; }
    void Set(float x,float y,float z,uint c,float u,float v) 
    { px=x; py=y; pz=z; c0=c; u0=u; v0=v; }
    void Set(const sVector41 &p,uint c,float u,float v) 
    { px=p.x; py=p.y; pz=p.z; c0=c; u0=u; v0=v; }
};

struct sVertexPCTT
{
    float px,py,pz;
    uint c0;
    float u0,v0;
    float u1,v1;

    void Set(float x,float y,float z,uint c,float u0_,float v0_,float u1_,float v1_) 
    { px=x; py=y; pz=z; c0=c; u0=u0_; v0=v0_; u1=u1_; v1=v1_; }
    void Set(const sVector41 &p,uint c,float u0_,float v0_,float u1_,float v1_) 
    { px=p.x; py=p.y; pz=p.z; c0=c; u0=u0_; v0=v0_; u1=u1_; v1=v1_; }
};

struct sVertexPN
{
    float px,py,pz;
    float nx,ny,nz;

    void Init(float x,float y,float z,float _x,float _y,float _z) 
    { px=x; py=y; pz=z; nx=_x; ny=_y; nz=_z; }
    void Init(const sVector41 &p,const sVector40 &n) 
    { px=p.x; py=p.y; pz=p.z; nx=n.x; ny=n.y; nz=n.z; }
    void Set(float x,float y,float z,float _x,float _y,float _z) 
    { px=x; py=y; pz=z; nx=_x; ny=_y; nz=_z; }
    void Set(const sVector41 &p,const sVector40 &n) 
    { px=p.x; py=p.y; pz=p.z; nx=n.x; ny=n.y; nz=n.z; }
};

struct sVertexPNT
{
    float px,py,pz;
    float nx,ny,nz;
    float u0,v0;

    void Init(float x,float y,float z,float _x,float _y,float _z,float u,float v) 
    { px=x; py=y; pz=z; nx=_x; ny=_y; nz=_z; u0=u; v0=v; }
    void Init(const sVector41 &p,const sVector40 &n,float u,float v) 
    { px=p.x; py=p.y; pz=p.z; nx=n.x; ny=n.y; nz=n.z; u0=u; v0=v; }
    void Set(float x,float y,float z,float _x,float _y,float _z,float u,float v) 
    { px=x; py=y; pz=z; nx=_x; ny=_y; nz=_z; u0=u; v0=v; }
    void Set(const sVector41 &p,const sVector40 &n,float u,float v) 
    { px=p.x; py=p.y; pz=p.z; nx=n.x; ny=n.y; nz=n.z; u0=u; v0=v; }
};

/****************************************************************************/
/***                                                                      ***/
/***   Dynamic Geometry                                                   ***/
/***                                                                      ***/
/****************************************************************************/

#if sConfigRender==sConfigRenderGL2 || sConfigRender==sConfigRenderGLES2
#define sDG_ARRAYS 1
#else
#define sDG_ARRAYS 0
#endif


class sDynamicGeometry
{
    int VBAlloc;
    int IBAlloc;

    sVertexFormat *TempFormat;
    sMaterial *TempMtrl;
    int TempMode;
    int TempVC;
    int TempIC;
    int TempISize;
    int Quads;

#if sDG_ARRAYS 
    uint8 *VertexBuffer;
    uint8 *IndexBuffer;
    uint8 *IndexQuad;
#else
    int VBUsed;
    int IBUsed;
    bool DiscardVB;
    bool DiscardIB;
    int DiscardCount;
    sResource *IndexBuffer;
    sResource *VertexBuffer;
    sResource *IndexQuad;
    sGeometry *GeoNoIndex;
    sGeometry *GeoIndex;
    sGeometry *GeoQuad;
#endif
    sGeometry *NewGeo();
    void EndFrame();
    sAdapter *Adapter;
    sContext *Context;
public:
    sDynamicGeometry(sAdapter *);
    ~sDynamicGeometry();

    int GetMaxIndex(int minic,int indexsize);
    int GetMaxVertex(int minvc,sVertexFormat *fmt);
    void Begin(sContext *ctx,sVertexFormat *fmt,sMaterial *mtrl,int maxvc,int maxic,int mode=sGMP_Tris);
    void Begin(sContext *ctx,sVertexFormat *fmt,int indexsize,sMaterial *mtrl,int maxvc,int maxic,int mode=sGMP_Tris);
    void Begin(sVertexFormat *fmt,sMaterial *mtrl,int maxvc,int maxic,int mode=sGMP_Tris)
    { Begin(Adapter->ImmediateContext,fmt,mtrl,maxvc,maxic,mode); }
    void Begin(sVertexFormat *fmt,int indexsize,sMaterial *mtrl,int maxvc,int maxic,int mode=sGMP_Tris)
    { Begin(Adapter->ImmediateContext,fmt,indexsize,mtrl,maxvc,maxic,mode); }
    void BeginVB(void **vp);
    void EndVB(int used=-1);
    void BeginIB(void **ip);
    void EndIB(int used=-1);
    void QuadIB();
    void End(sCBufferBase *cb0=0,sCBufferBase *cb1=0,sCBufferBase *cb2=0,sCBufferBase *cb3=0);
    void Flush();

    void Quad(sMaterial *mtrl,const sVertexP &p0,const sVertexP &p1,sCBufferBase *cb0=0,sCBufferBase *cb1=0,sCBufferBase *cb2=0,sCBufferBase *cb3=0);
    void Quad(sMaterial *mtrl,const sVertexPC &p0,const sVertexPC &p1,sCBufferBase *cb0=0,sCBufferBase *cb1=0,sCBufferBase *cb2=0,sCBufferBase *cb3=0);
    void Quad(sMaterial *mtrl,const sVertexPT &p0,const sVertexPT &p1,sCBufferBase *cb0=0,sCBufferBase *cb1=0,sCBufferBase *cb2=0,sCBufferBase *cb3=0);
    void Quad(sMaterial *mtrl,const sVertexPCT &p0,const sVertexPCT &p1,sCBufferBase *cb0=0,sCBufferBase *cb1=0,sCBufferBase *cb2=0,sCBufferBase *cb3=0);

    template <class T> void BeginVB(T **data) 
    { BeginVB((void **)data); }
    template <class T> void BeginIB(T **data) 
    { BeginIB((void **)data); }
};

/****************************************************************************/
/***                                                                      ***/
/***   Functions                                                          ***/
/***                                                                      ***/
/****************************************************************************/

void sGetGfxStats(sGfxStats &);             // get stats of last frame
bool sEnableGfxStats(bool enable);         // disable gfx stats during debug painting, for instance
void sUpdateGfxStats(const sDrawPara &dp);  // internal, called inside sDraw()
void sUpdateGfxStats();                     // internal, called at end of frame

int sGetDisplayCount();                    // number of displays, opened or not
void sGetDisplayInfo(int display,sDisplayInfo &info);  // get info about a display (that can then be opened

int sGetScreenCount();                     // number of opened screens
sScreen *sGetScreen(int n=-1);             // get an opened screen
// to create a new screen, call new sScreen(..). It will be deleted automatically

int sGetAdapterCount();                    // get numner of adapters (open or not)
sAdapter *sCreateHeadlessAdapter(int n);   // create an adapter without window. you are responsible to delete it.
int sGetBitsPerPixel(int format);

bool sIsSupportedFormat(int format);	//returns true if format is supported
bool sIsElementIndexUintSupported();

/****************************************************************************/

} // namespace Altona2

#endif  // FILE_ALTONA2_LIBS_BASE_GRAPHICS_HPP

