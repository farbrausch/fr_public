// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __START_HPP__
#define __START_HPP__

#include "_types.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Forwards                                                           ***/
/***                                                                      ***/
/****************************************************************************/

struct sScreenInfo;               // find out screen resolution/window size
struct sViewport;                 // render to a portion of the screen
struct sTexInfo;                  // upload a texture
struct sLightEnv;
struct sLightInfo;
struct sBitmapLock;
class sHostFont;

/****************************************************************************/

struct sInputData;                // get joystick and mouse input    

/****************************************************************************/

struct sDirEntry;                 // load a directory

/****************************************************************************/

#define sISHANDLE(x) ((x)>=0)
#define sINVALID -1
#define sEXPORT __declspec(dllexport)

/****************************************************************************/
/***                                                                      ***/
/***   Main Program                                                       ***/
/***                                                                      ***/
/****************************************************************************/

// implement this to write a program

sBool sAppHandler(sInt code,sU32 value);  

// codes for sAppHandler. called in this order

#define sAPPCODE_CONFIG       1   // parse commandline and call sSystem::Init()
#define sAPPCODE_INIT         2   // init your system
#define sAPPCODE_KEY          3   // keyboard input
#define sAPPCODE_FRAME        4   // called once each frame
#define sAPPCODE_TICK         5   // called for each 10ms timeslice
#define sAPPCODE_PAINT        6   // called once each frame
#define sAPPCODE_EXIT         7   // free resources. 
#define sAPPCODE_LAYOUT       8   // layout windows for sGuiManager
#define sAPPCODE_CMD          9   // cmd from sGuiManager
#define sAPPCODE_DEBUGPRINT   10  // print a line of text. value is (char *)

/****************************************************************************/

// config

#define sSF_MULTISCREEN   0x0010  // open window on each screen
#define sSF_FULLSCREEN    0x0020  // go fullscreen (on all screens)
#define sSF_FULLOTHER     0x0080  // all but first screen go fullscreen
#define sSF_MAXIMISE      0x0040  // in windowed mode, max window(s)

#define sSF_GRAPHMASK     0x000f
#define sSF_DIRECT3D      0x0001
#define sSF_OPENGL        0x0002

void sSetConfig(sU32 Flags,sInt xsize=640,sInt ysize=480);

/****************************************************************************/

// sSystem->CpuMask

#define sCPU_CMOVE          0x0001        // CMOV FCMOV FCOMI instructions
#define sCPU_RDTSC          0x0002        // RDTSC instruction
#define sCPU_MMX            0x0004        // Intel MMX Instructions
#define sCPU_MMX2           0x0008        // Extended MMX instructions because of AMD Extension or SSE
#define sCPU_SSE            0x0010        // PIII SSE instructions fully supported
#define sCPU_SSE2           0x0020        // P4 SSE2 instruction fully supported
#define sCPU_3DNOW          0x0040        // 3dNow! instructions of K6
#define sCPU_3DNOW2         0x0080        // extended 3dNow! for Athlon

/****************************************************************************/

// sSystem_::GpuMask

#define sGPU_TWOSIDESTENCIL 0x0001        // two-sided stencil

/****************************************************************************/

// geometry type

#define sGEO_POINT      0x000001  // draw points
#define sGEO_LINE       0x000002  // draw lines
#define sGEO_TRI        0x000003  // draw triangles
#define sGEO_LINESTRIP  0x000004  // draw lines
#define sGEO_TRISTRIP   0x000005  // draw triangles
#define sGEO_QUAD       0x000006  // draw quads (there is a static ib for that in the dx-port)

#define sGEO_DYNVB      0x000000  // dynamic vertexbuffer
#define sGEO_DYNIB      0x000000  // dynamic indexbuffer
#define sGEO_STATVB     0x000008  // static vertexbuffer
#define sGEO_STATIB     0x000010  // static indexbuffer

#define sGEO_DYNAMIC    sGEO_DYNVB|sGEO_DYNIB
#define sGEO_STATIC     sGEO_STATVB|sGEO_STATIB

#define sGEO_VERTEX     0x000001  // for GeoFlush/GeoDraw return code
#define sGEO_INDEX      0x000002  // for GeoFlush/GeoDraw return code

// flexible vertex formats
/*
#define sFVF_2D         0x000000  // xyzrhw    
#define sFVF_3D         0x000001  // xyz
#define sFVF_NORMAL     0x000002  // xyz normal
#define sFVF_COLOR0     0x000010  // diffuse color
#define sFVF_COLOR1     0x000020  // specular color
#define sFVF_PSIZE      0x000040  // point-sprite size (not yet implemented)
#define sFVF_TEX0       0x000000  // no uv
#define sFVF_TEX1       0x000100  // 1 uv
#define sFVF_TEX2       0x000200  // 2 uv's
#define sFVF_TEX3       0x000300  // 3 uv's
#define sFVF_TEX4       0x000400  // 4 uv's
#define sFVF_TEXMASK    0x000f00  // mask for textures
#define sFVF_TEX3D0     0x001000  // enable 3d-texturing for texture stage 0

#define sFVF_DEFAULT3D  (sFVF_3D|sFVF_NORMAL|sFVF_TEX1) 
#define sFVF_DEFAULT2D  (sFVF_3D|sFVF_COLOR0|sFVF_TEX2)
*/

/****************************************************************************/

#define sFVF_STANDARD   1     // pos,normal,tex0
#define sFVF_DOUBLE     2     // pos,color,tex0,tex1
#define sFVF_TSPACE     3     // pos,tspace,tex0,tex1 (not packed)
#define sFVF_COMPACT    4     // pos,color
#define sFVF_XYZW       5     // pos(4-component)
#define sFVF_TSPACE3    6     // pos,tspace,tex0 (packed)


struct sVertexStandard
{
  sF32 x,y,z;
  sF32 nx,ny,nz;
  sF32 u,v;
};

struct sVertexDouble
{
  sF32 x,y,z;
  sU32 c0;
  sF32 u0,v0;
  sF32 u1,v1;

  __forceinline void Init(sF32 X,sF32 Y,sF32 Z=0.5f,
    sU32 C0=~0,sF32 U0=0,sF32 V0=0,sF32 U1=0,sF32 V1=0)
  { x=X;y=Y;z=Z;c0=C0;u0=U0;v0=V0;u1=U1;v1=V1; }
};

struct sVertexTSpace
{
  sF32 x,y,z;
  sF32 nx,ny,nz;
  sU32 c0,c1;
  sF32 u0,v0;
//  sF32 u1,v1;
  sF32 sx,sy,sz;
  sF32 tx,ty,tz;
};

struct sVertexCompact
{
  sF32 x,y,z;
  sU32 c0;
};

struct sVertexTSpace3
{
  sF32 x,y,z;
  sS16 nx,ny,nz,sz;
  sS16 sx,sy;
  sF32 u,v;
};

struct sVertexXYZW
{
  sF32 x,y,z,w;
};

#define sFVF_DEFAULT3D  1     // obsolete
#define sFVF_DEFAULT2D  2     // obsolete
typedef sVertexStandard sVertex3d;
typedef sVertexDouble sVertex2d;

/****************************************************************************/
/*
inline void sQuad (sU16 *&ip,sInt a,sInt b,sInt c,sInt d,sInt o=0) {*ip++=a+o;*ip++=b+o;*ip++=c+o;*ip++=a+o;*ip++=c+o;*ip++=d+o;}
inline void sQuadI(sU16 *&ip,sInt d,sInt c,sInt b,sInt a,sInt o=0) {*ip++=a+o;*ip++=b+o;*ip++=c+o;*ip++=a+o;*ip++=c+o;*ip++=d+o;}
inline void sTri  (sU16 *&ip,sInt a,sInt b,sInt c,       sInt o=0) {*ip++=a+o;*ip++=b+o;*ip++=c+o;}
inline void sTriI (sU16 *&ip,sInt c,sInt b,sInt a,       sInt o=0) {*ip++=a+o;*ip++=b+o;*ip++=c+o;}
*/

#define  sQuad(ip,a,b,c,d) {(ip)[0]=a;(ip)[1]=b;(ip)[2]=c;(ip)[3]=a;(ip)[4]=c;(ip)[5]=d;ip+=6;}
#define sQuadI(ip,d,c,b,a) {(ip)[0]=a;(ip)[1]=b;(ip)[2]=c;(ip)[3]=a;(ip)[4]=c;(ip)[5]=d;ip+=6;}
#define   sTri(ip,a,b,c)   {(ip)[0]=a;(ip)[1]=b;(ip)[2]=c;ip+=3;}
#define  sTriI(ip,c,b,a)   {(ip)[0]=a;(ip)[1]=b;(ip)[2]=c;ip+=3;}

/****************************************************************************/
/***                                                                      ***/
/***   Graphics Structures                                                ***/
/***                                                                      ***/
/****************************************************************************/

struct sScreenInfo
{
  sInt XSize;                     // width of screen
  sInt YSize;                     // height of screen
  sBool FullScreen;               // is this fullscreen?
  sBool SwapVertexColor;          // opengl-style RGBA vertex colors.
  sBool LowQuality;               // low quality mode (for reduced video memory usage);
  sInt ShaderLevel;               // supported Shader level
  sF32 PixelRatio;                // usually 1:1, but 1280x1024 is different
};

                                  // Shader Level
#define sPS_00        0
#define sPS_13        1
#define sPS_14        2
#define sPS_20        3

/****************************************************************************/

struct sPerfInfo
{
  sInt Line;                      // lines submitted
  sInt Triangle;                  // triangles submitted
  sInt Vertex;                    // vertices submitted
  sInt Material;                  // materials changed
  sInt Batches;                   // batches submitted
  sU32 Time;                      // last frames time in microseconds
  sU32 TimeFiltered;              // filtered time
};

#define sPIM_LAST     1           // get total info from last frame
#define sPIM_BEGIN    2           // start counting now
#define sPIM_END      3           // end counting now
#define sPIM_PAUSE    4           // interrupt counting
#define sPIM_CONTINUE 5           // resume counting

/****************************************************************************/

// GetTransform Flags

#define sGT_UNIT          0       // returns a unit matrix. somewhat usefull...
#define sGT_VIEW          1
#define sGT_MODELVIEW     2
#define sGT_MODEL         3
#define sGT_PROJECT       4

/****************************************************************************/
/****************************************************************************/

// base flags

#define smBF_ALPHATEST    0x0001  // zero gets transparent
#define smBF_DOUBLESIDED  0x0002  // draw doublesided
#define smBF_INVERTCULL   0x0004  // invert culling
#define smBF_ZBIASBACK    0x0008  // do some sensible z-biasing (push back)
#define smBF_ZBIASFORE    0x0010  // do some sensible z-biasing (push fore)
#define smBF_ZONLY        0x0020  // do not write the colorbuffer
#define smBF_FOG          0x0040  // enable fogging
#define smBF_ALPHATESTINV 0x0080  // inverts alphatest if enabled

#define smBF_ZMASK        0x0300  // zbuffer control
#define smBF_ZOFF         0x0000  // no z operation
#define smBF_ZWRITE       0x0100  // charge z buffer
#define smBF_ZREAD        0x0200  // blend operation
#define smBF_ZON          0x0300  // normal operation

#define smBF_BLENDMASK    0xf000  // blend stage (d=dest, s=src)
#define smBF_BLENDOFF     0x0000  // d=s
#define smBF_BLENDALPHA   0x1000  // alpha blending
#define smBF_BLENDADD     0x2000  // d=(d+s)
#define smBF_BLENDMUL     0x3000  // d=(d*s)
#define smBF_BLENDMUL2    0x4000  // d=(d*s)*2
#define smBF_BLENDSMOOTH  0x5000  // d=(d+s)-(d*s)
#define smBF_BLENDSUB     0x6000  // d=(d-s)
#define smBF_BLENDDESTA   0x7000  // destination alpha blending
#define smBF_BLENDADDSA   0x8000  // d=d+s*s.alpha

#define smBF_NOTEXTURE    0x010000
#define smBF_NONORMAL     0x020000
#define smBF_SHADOWMASK   0x040000  // the stencil ops requiring shadow casting

#define smBF_STENCILMASK  0xf00000  // all stencil operation
#define smBF_STENCILINC   0x100000  // cast shadow
#define smBF_STENCILDEC   0x200000  // cast shadow
#define smBF_STENCILTEST  0x300000  // receive shadow
#define smBF_STENCILCLR   0x400000  // clear shadow
#define smBF_STENCILINDE  0x500000  // increment/decrement (2sided)
#define smBF_STENCILRENDER 0x600000 // render (+clear)
#define smBF_STENCILZFAIL 0x800000  // for inc/dec

// special flags

#define sMSF_BUMPDETAIL   0x0001  // bump-detail (Tex0)

#define sMSF_ENVIMASK     0x0030  // environment mapping (Tex2)
#define sMSF_ENVIOFF      0x0000  // no envi
#define sMSF_ENVISPHERE   0x0010  // sphere envi (normal)
#define sMSF_ENVIREFLECT  0x0020  // reflection envi ("specular")

#define sMSF_SPECFIXED    0x0000  // use fixed specularity index
#define sMSF_SPECPOWER    0x0040  // use float specularity power
#define sMSF_SPECMAP      0x0080  // use bumpmap alpha as specularity mapping

#define sMSF_PROJMASK     0x0700  // UV Projection (Tex3)
#define sMSF_PROJOFF      0x0000  // no uv projection 
#define sMSF_PROJMODEL    0x0100  // model space
#define sMSF_PROJWORLD    0x0200  // world space
#define sMSF_PROJEYE      0x0300  // eye space
#define sMSF_PROJSCREEN   0x0400  // screen space

#define sMSF_LONGCOLOR    0x1000  // uses long color and no normals

// light flags

#define sMLF_MODEMASK     0x0007  // lighting mode
#define sMLF_OFF          0x0000  // no light calculation
#define sMLF_DIR          0x0001  // directional light
#define sMLF_POINT        0x0002  // position light
#define sMLF_BUMP         0x0003  // position light with specular calc
#define sMLF_BUMPENV      0x0004  // point & bumpenv
#define sMLF_BUMPX        0x0005  // 4x bump
#define sMLF_BUMPXSPEC    0x0006  // 4x bump specular pass

#define sMLF_DIFFUSE      0x0010  // multiply diffuse light to color0
#define sMLF_AMBIENT      0x0020  // multiply ambient light to color2

// texture flags

#define smTF_FILTER       0x0001
#define smTF_MIPMAPS      0x0002
#define smTF_ANISO        0x0003
#define smTF_MIPUF        0x0004  // point-sampled unfiltered mipmaps

#define smTF_UV0          0x0000
#define smTF_UV1          0x0010

#define smTF_TILE         0x0000
#define smTF_CLAMP        0x0100

#define smTF_TRANSMASK    0x3000
#define smTF_DIRECT       0x0000
#define smTF_SCALE        0x1000
#define smTF_TRANS1       0x2000
#define smTF_TRANS2       0x3000

#define smTF_PROJECTIVE   0x4000

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

#define sMCOB_NOP         0x0000
#define sMCOB_SET         0x0100
#define sMCOB_ADD         0x0200
#define sMCOB_SUB         0x0300
#define sMCOB_MUL         0x0400
#define sMCOB_MUL2        0x0500
#define sMCOB_MUL4        0x0600
#define sMCOB_MUL8        0x0700
#define sMCOB_SMOOTH      0x0800
#define sMCOB_LERP        0x0900  // input + current*(1-alpha)    (input is already multiplied with alpha)
#define sMCOB_RSUB        0x0a00
#define sMCOB_DP3         0x0b00

// material combiner stage enumeration

#define sMCS_LIGHT        0
#define sMCS_COLOR0       1
#define sMCS_COLOR1       2
#define sMCS_TEX0         3
#define sMCS_TEX1         4
#define sMCS_MERGE        5
#define sMCS_SPECULAR     6
#define sMCS_COLOR2       7
#define sMCS_COLOR3       8
#define sMCS_TEX2         9
#define sMCS_TEX3         10
#define sMCS_VERTEX       11
#define sMCS_FINAL        12
#define sMCS_MAX          13

/****************************************************************************/
struct sMaterialInfo              // the offsets must match the Material Operator Data!!!
{
  sU32 BaseFlags;                 //  0: smBF_???
  sU32 SpecialFlags;              //  1: sMSF_???
  sU32 LightFlags;                //  2: sMLF_???
  sU32 pad1;                      //  3:
  sU32 TFlags[4];                 //  4: smTF_??

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
  sInt Tex[4];                    // texture handles
  sInt ShaderLevel;               // sPS_?? pixel shader version to use

                                  // user-defined variabled, you will need them anyway...
  sInt Handle;                    // handle for this material
  sInt Usage;                     // different usages
  sInt Single;                    // single material?
  sInt Pass;                      // renderpass sorting
  sInt Program;                   // how to creat vertex-buffer (sprites, spikes, thicklines, flatshading,....)
  sF32 Size;                      // parameter to program
  sF32 Aspect;                    // parameter to program

#if !sPLAYER
  sInt PSOps;                     // PS-Ops used
  sInt VSOps;                     // VS-Ops used
  sInt ShaderLevelUsed;           // shaderlevel really used (to indicate automatic downgrade, which is not implemented yet)
#endif

  void Init();
  void DefaultCombiner(sBool *tex=0); // set combiner to defaults, tex holds an array of bools if the textures are set.
  void AddRefElements(); // addref the textures
};

/****************************************************************************/

struct sFastLight                 // for sMLF_BUMPX mode
{
  sVector Pos;
  sVector Color;
  sF32 Range;
  sF32 Amplify;
};

struct sMaterialEnv
{
  sVector LightPos;               // Main Light Source            
  sVector ModelLightPos;          // ...position in model space
  sF32 LightRange;
  sU32 LightDiffuse;
  sU32 LightAmbient;
  sF32 LightAmplify;

  sFastLight FastLights[4];       // for sMLF_BUMPX mode

  sF32 FogStart;                  // fogging
  sF32 FogEnd;
  sU32 FogColor;

  sMatrix ModelSpace;             // Model in World
  sMatrix CameraSpace;            // Camera in World
  sF32 ZoomX,ZoomY;               // zoom-factors (usually 1.0f)
  sF32 CenterX,CenterY;           // center-offset (usually 0.5f)
  sF32 NearClip;                  // near clipping plane
  sF32 FarClip;                   // far clipping plane
  sInt Orthogonal;                // no perspective 

  void Init();
  void MakeProjectionMatrix(sMatrix &m) const;
};

#define sMEO_PERSPECTIVE  0       // perspective view
#define sMEO_NORMALISED   1       // orthogonal view, -1 .. 1
#define sMEO_PIXELS       2       // orthogonal view, pixels from (0,0) to (xsize-1,ysize-1) inclusive

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
/****************************************************************************/

struct sViewport
{
  sInt Screen;                    // screen to use, or -1
  sInt RenderTarget;              // texture to use, or -1

  sRect Window;                   // render window
  sU32 ClearFlags;                // sCCF_???
  sColor ClearColor;              // color to clear background

  void Init(sInt screen=0);       // initialize for use with screen
  void InitTex(sInt handle);      // initialize for rendertarget
};

// clear flags

#define sVCF_NONE       0         // nothing
#define sVCF_COLOR      1         // color only
#define sVCF_Z          2         // zbuffer only
#define sVCF_ALL        3         // all
#define sVCF_PARTIAL    4
/****************************************************************************/

// flags

#define sTIF_RENDERTARGET 0x0001  // can be used as rendertarget
#define sTIF_STREAM       0x0002  // must be updated each frame
#define sTIF_UPDATE       0x0004  // may be updated
#define sTIF_LOOP         0x0008  // can be tiled, mirrored, clamped
#define sTIF_MIPMAP       0x0010  // generate mipmaps
#define sTIF_NONPOWER2    0x0020  // support for non-power-of-2 textures
#define sTIF_CUBEMAP      0x0040  // create cubemap. Bitmap YSize*6
#define sTIF_ALLOCATED    0x0080  // used internally to mark allocated texture handles

// formats

#define sTF_NONE        0         // illegal texture format
#define sTF_32          1         // 32 bit textures RGB (without alpha)
#define sTF_32A         2         // 32 bit textures RGBA (including alpha)
#define sTF_32T         3         // 32 bit textures RGBA (including 1 bit alpha)
#define sTF_16          4         // 16 bit textures RGB (without alpha)
#define sTF_16A         5         // 16 bit textures RGBA (including alpha)
#define sTF_16T         6         // 16 bit textures RGBA (including 1 bit alpha)
#define sTF_8A          7         // 8 bit texture A (only alpha)
#define sTF_8I          8         // 8 bit intensity texture 
//#define sTF_MAX         9         // number of texture formats

// the structure

struct sTexInfo
{
  sInt XSize;
  sInt YSize;
  sU32 Flags;                     // sTIF_???
  sInt Format;                    // requested sTF_??

  sBitmap *Bitmap;                // texture bitmap
//  sF32 TexTrans[4];               // texture transform for adjusting packed textures, calculated by system
  void Init(sBitmap *,sInt format=sTF_32A,sU32 flags=sTIF_LOOP|sTIF_MIPMAP);
  void InitRT(sInt x=0,sInt y=0);
};

// the new texture formats (for use with the new addtexture)

#define sTF_NONE            0     // illegal texture format
#define sTF_A8R8G8B8        1     // 4 x 8 bit unsigned
#define sTF_A8              2     // 1 x 8 bit unsigned
#define sTF_R16F            3     // 1 x 16 bit float
#define sTF_A2R10G10B10     4     // 3 x 10 bit unsigned + 2 bit unsigned
#define sTF_Q8W8V8U8        5     // 4 x 8 bit signed
#define sTF_A2W10V10U10     6     // 3 x 10 bit signed + 2 bit unsigned
#define sTF_MAX             7     // end of list

/****************************************************************************/

struct sBitmapLock
{
  sU8 *Data;                      // memory of the locked texture
  sInt XSize;                     // width 
  sInt YSize;                     // height
  sInt BPR;                       // bytes per row. 
  sInt Kind;                      // bitmap kind
};

#define sBITMAP_ARGB8888    1
#define sBITMAP_ARGB4444    2
#define sBITMAP_ARGB1555    3
#define sBITMAP_RGB565      4
#define sBITMAP_A8          5
#define sBITMAP_I8          6

/****************************************************************************/
/***                                                                      ***/
/***   State Stream                                                       ***/
/***                                                                      ***/
/****************************************************************************/

// A State-Stream consists of sU32-Duplets. the first contains the token
// and the second the value. Each Engine (DirectX, OpenGL) defines it's own
// set of tokens. most tokens will send the value as is to the driver. some
// tokens use the value as pointer to a structure, and that structure is
// passed to the driver.
//
// you can use the sSystem::State??? functions to set up simple state-streams.
// you can later add modifications. sSystem::StateOptimise() will remove
// doubles.
//
// The system tries to send values only if they changed. for pointer values,
// it is assumed that if the pointer didn't change, the data does not need
// to be resend. all data is send at least once each frame. you should
// define each state-stream as complete as possible and rely on the system to
// filter correctly. but you should not send redundant values, like states
// for an unused texture stage.
//
// please note that textures are not part of the state-stream, they must be
// set seperatly.
//
// if you want to send a value regardless of it's last state, "or" 0x80000000
// to the token. 0xffffffff marks the end of stream. 

#define sMT_FORCE   0x80000000
#define sMT_END     0xffffffff

/****************************************************************************/
/*
struct sLightEnv
{
  sU32 Diffuse;                   // light components
  sU32 Ambient;
  sU32 Specular;
  sU32 Emissive;
  sF32 Power;                     // specular power, set to 0.0 to turn off

  void Init();
};
*/
/****************************************************************************/

struct sLightInfo
{
  sInt Type;                      // sLI_???
  sU32 Mask;                      // light mask

  sVector Pos;                    // position (not sLI_DIR)
  sVector Dir;                    // direction (not sLI_SPOT)
  sF32 Range;                     // range
  sF32 RangeCut;                  // allowed intensity at outer edge of range

  sU32 Diffuse;                   // light components
  sU32 Ambient;
  sU32 Specular;
  sF32 Amplify;                   // amplify all light components

  sF32 Gamma;                     // spotlight border sharpness
  sF32 Inner;                     // spotlight size of inner white spot
  sF32 Outer;                     // spotlight outer angle

  void Init();
};

// Type

#define sLI_POINT     1
#define sLI_SPOT      2
#define sLI_DIR       3
#define sLI_AMBIENT   4

/****************************************************************************/

#define sMBF_DOUBLESIDED  0x0001
#define sMBF_RENORMALIZE  0x0002
#define sMBF_LIGHT        0x0004
#define sMBF_COLOR        0x0008  // use tfactor
#define sMBF_INVERTCULL   0x0010
#define sMBF_ZBIAS        0x0020

#define sMBF_ZMASK        0x0300
#define sMBF_ZOFF         0x0000
#define sMBF_ZWRITE       0x0100
#define sMBF_ZREAD        0x0200
#define sMBF_ZON          0x0300
#define sMBF_ZONLY        0x0800

#define sMBF_BLENDMASK    0x7000
#define sMBF_BLENDSHIFT   12
#define sMBF_BLENDOFF     0x0000
#define sMBF_BLENDALPHA   0x1000
#define sMBF_BLENDADD     0x2000
#define sMBF_BLENDMUL     0x3000
#define sMBF_BLENDSMOOTH  0x4000
#define sMBF_BLENDSUB     0x5000

/****************************************************************************/

#define sMBM_FLAT         1
#define sMBM_TEX          2
#define sMBM_ADD          3
#define sMBM_MUL          4
#define sMBM_BLEND				5
#define sMBM_SUB					6
#define sMBM_ADDSMOOTH		7
#define sMBM_ADDCOL				8
#define sMBM_MULCOL				9
#define sMBM_BLENDCOL			10
#define sMBM_SUBCOL				11
#define sMBM_ADDSMOOTHCOL 12
#define sMBM_SHOWALPHA    13

/****************************************************************************/

#define sMTF_FILTERMIN    0x0001
#define sMTF_FILTERMAG    0x0002
#define sMTF_FILTER       0x0003
#define sMTF_MIPMAP       0x0004
#define sMTF_CLAMP        0x0008
#define sMTF_TILE         0x0000
#define sMTF_UVTRANS      0x0010

#define sMTF_UVMASK       0x0f00
#define sMTF_UVSHIFT      8
#define sMTF_ENVIMASK     0x0800
#define sMTF_UV0          0x0000
#define sMTF_UV1          0x0100
#define sMTF_UV2          0x0200
#define sMTF_UV3          0x0300
#define sMTF_UV4          0x0400
#define sMTF_UV5          0x0500
#define sMTF_UV6          0x0600
#define sMTF_UV7          0x0700
#define sMTF_UVENVI       0x0800
#define sMTF_UVSPEC       0x0900
#define sMTF_UVPOS        0x0a00

/****************************************************************************/
/***                                                                      ***/
/***   Input                                                              ***/
/***                                                                      ***/
/****************************************************************************/

struct sInputData
{
  sU8 Type;                       // sIDT_???
  sU8 AnalogCount;                // number of analog channels
  sU8 DigitalCount;               // number of button-bits
  sU8 pad;
  sInt Analog[4];                 // analog channels
  sU32 Digital;                   // button bits
};

#define sIDT_NONE     0           // no input device connected
#define sIDT_MOUSE    1           // absolute mouse
#define sIDT_TABLET   2           // relative mouse (0..0x10000)
#define sIDT_JOYSTICK 3           // joystick (-0x10000..0x10000)

/****************************************************************************/

#define sKEY_BACKSPACE    8
#define sKEY_TAB          9
#define sKEY_ENTER        10
#define sKEY_ESCAPE       27
#define sKEY_SPACE        32

#define sKEY_UP           0x00010000
#define sKEY_DOWN         0x00010001
#define sKEY_LEFT         0x00010002
#define sKEY_RIGHT        0x00010003
#define sKEY_PAGEUP       0x00010004
#define sKEY_PAGEDOWN     0x00010005
#define sKEY_HOME         0x00010006
#define sKEY_END          0x00010007
#define sKEY_INSERT       0x00010008
#define sKEY_DELETE       0x00010009
#define sKEY_WHEELUP      0x0001000a
#define sKEY_WHEELDOWN    0x0001000b

#define sKEY_PAUSE        0x00010010      // Pause/Break key
#define sKEY_SCROLL       0x00010011      // ScrollLock key
#define sKEY_NUMLOCK      0x00010012      // NumLock key
#define sKEY_WINL         0x00010013      // left windows key
#define sKEY_WINR         0x00010014      // right windows key
#define sKEY_APPPOPUP     0x00010015      // windows menu key
#define sKEY_CLOSE        0x00010016      // system close event (usually alt-f4)
#define sKEY_SIZE         0x00010017      // window resized
#define sKEY_APPCLOSE     0x00010018      // window close button (catch in OnAppKey())
#define sKEY_APPMAX       0x00010019      // window maximize button (catch in OnAppKey())
#define sKEY_APPMIN       0x0001001a      // window minimize button (catch in OnAppKey())
#define sKEY_APPHELP      0x0001001b      // window help button (catch in OnAppKey())
#define sKEY_OPEN         0x0001001c      // send when gui is completly initialised
#define sKEY_APPFOCUS     0x0001001d      // window gained primary focus
#define sKEY_APPFOCLOST   0x0001001e      // window lost primary focus
#define sKEY_MODECHANGE   0x0001001f      // graphics mode or screen size has changed. 

#define sKEY_SHIFTL       0x00010021
#define sKEY_SHIFTR       0x00010022
#define sKEY_CAPS         0x00010023
#define sKEY_CTRLL        0x00010024
#define sKEY_CTRLR        0x00010025
#define sKEY_ALT          0x00010026
#define sKEY_ALTGR        0x00010027
#define sKEY_MOUSEL       0x00010028
#define sKEY_MOUSER       0x00010029
#define sKEY_MOUSEM       0x0001002a

#define sKEY_F1           0x00010030
#define sKEY_F2           0x00010031
#define sKEY_F3           0x00010032
#define sKEY_F4           0x00010033
#define sKEY_F5           0x00010034
#define sKEY_F6           0x00010035
#define sKEY_F7           0x00010036
#define sKEY_F8           0x00010037
#define sKEY_F9           0x00010038
#define sKEY_F10          0x00010039
#define sKEY_F11          0x0001003a
#define sKEY_F12          0x0001003b

#define sKEYQ_SHIFTL      0x00020000
#define sKEYQ_SHIFTR      0x00040000
#define sKEYQ_CAPS        0x00080000
#define sKEYQ_CTRLL       0x00100000
#define sKEYQ_CTRLR       0x00200000
#define sKEYQ_ALT         0x00400000
#define sKEYQ_ALTGR       0x00800000
#define sKEYQ_MOUSEL      0x01000000
#define sKEYQ_MOUSER      0x02000000
#define sKEYQ_MOUSEM      0x04000000
#define sKEYQ_STICKYBREAK 0x10000000
#define sKEYQ_NUM         0x20000000
#define sKEYQ_REPEAT      0x40000000
#define sKEYQ_BREAK       0x80000000

#define sKEYQ_SHIFT       0x000e0000
#define sKEYQ_CTRL        0x00300000

/****************************************************************************/
/***                                                                      ***/
/***   Host Font                                                          ***/
/***                                                                      ***/
/****************************************************************************/

struct sHostFontLetter
{
  sRect Location;
  sInt Pre;
  sInt Post;
  sInt Top;
  sInt pad;
};

/****************************************************************************/

class sHostFont
{
  struct sHostFontPrivate *prv;
  sInt Height;                    // set by Init()
  sInt Advance;
  sInt Baseline;

public:
  sHostFont();
  ~sHostFont();

  __forceinline sInt GetHeight() { return Height; }
  __forceinline sInt GetAdvance() { return Advance; }
  __forceinline sInt GetBaseline() { return Baseline; }

  void Init(sChar *name,sInt height,sInt widht=0,sInt style=0);
  sInt GetWidth(sChar *,sInt len=-1);
  void Print(sBitmap *,sChar *,sInt x,sInt y,sU32 color=0xffffffff,sInt len=-1);
  sBool Prepare(sBitmap *,sChar *,sHostFontLetter *);
  sBool PrepareCheck(sInt xs,sInt ys,sChar *);

  // alternative to print

  void Begin(sBitmap *bm);
  void Print(sChar *text,sInt x,sInt y,sU32 c0,sInt len=-1);
  void End(sBitmap *bm);
};

                                  // font style
#define sHFS_BOLD     0x0001      // bold 
#define sHFS_ITALICS  0x0002      // italics
#define sHFS_PIXEL    0x0004      // no antialiasing


/****************************************************************************/
/***                                                                      ***/
/***   Other Structures                                                   ***/
/***                                                                      ***/
/****************************************************************************/

struct sDirEntry
{
  sChar Name[256];                // filename        
  sInt Size;                      // size of file in bytes
  sBool IsDir;                    // this is a directory, size is 0
  sU8 pad[3];                     // pad it up..
};

/****************************************************************************/

typedef void (*sSoundHandler)(sS16 *steriobuffer,sInt samples,void *user); 



/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Private                                                            ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

#define MAX_SCREEN    4
#define MAX_KEYBUFFER 32
#define MAX_DYNBUFFER 8
#define MAX_TEXTURE 1024
#define MAX_DYNVBSIZE (64*0x8000)
#define MAX_DYNIBSIZE (8*0x8000)
#define MAX_GEOBUFFER 2048
#define MAX_GEOHANDLE 4096
#define MAX_LIGHTS 64
#define MAX_SHADERS 4096
#define MAX_MATERIALS 2048
#define sMAXSAMPLEBUFFER  8
#define sMAXSAMPLEHANDLE  256

/****************************************************************************/

struct sGeoBuffer                 // a vertex or indexbuffer
{
  sBool Type;                     // 0 = unused, 1 = index, 2 = vertex
  sInt Size;                      // Size of the buffer in bytes
  sInt Used;                      // first free byte
  sInt UserCount;                 // counts active allocations. increased with alloc, decreased with free. if decreased to zero, the buffer is assumed to be completly free'ed and Used is reset
  union
  {
    struct IDirect3DVertexBuffer9 *VB;
    struct IDirect3DIndexBuffer9 *IB;
  };

  void Init();                    // clear all field of structure
  sBool AllocVB(sInt count,sInt size,sInt &first);  // alloc COUNT vertices, SIZE bytes long and return first aligned byte index in FIRST if it fits.
  sBool AllocIB(sInt count,sInt &first);  // alloc COUNT indices and return FIRST index if it fits.
  sBool AllocXB(sInt count,sInt shift,sInt &firstp);
  void Free();
};

struct sGeoHandle
{
  sInt Mode;                      // sGEO_??? 
  sU32 FVF;                       // FVF table index
  sInt VertexSize;                // size of one vertex in bytes
  sU32 VBDiscardCount;            // if dynamic, this is the DISCARD timestamp (vb)
  sU32 IBDiscardCount;            // if dynamic, this is the DISCARD timestamp (vb)
  sInt Locked;                    // indicates whether indices/vertices are locked
  
  const sChar *File;
  sInt Line;
  sInt AllocId;

  sInt IndexBuffer;               // sGeoBuffer for indices (0 for none, 1 for dynamic, >=3 for static)
  sInt IndexStart;                // first index to use in buffer
  sInt IndexCount;                // number of indices, 0 for none
  sInt VertexBuffer;              // sGeoBuffer for vertices (0 for dynamic, >=3 for static)
  sInt VertexStart;               // first vertex to use, denoted as index #0
  sInt VertexCount;               // number of vertices, 0 if vertex and index buffer are not loaded
};

/****************************************************************************/

struct sHardTex
{
  sInt RefCount;                  // texture-handles haben jetzt auch refcounting
  sInt XSize;
  sInt YSize;
  sU32 Flags;                     // sTIF_???
  sInt Format;                    // requested sTF_??

  struct IDirect3DTexture9 *Tex;
  sInt TexGL;
};

/****************************************************************************/

struct sShader
{
  sU32 *ShaderData;                     // the compiled shader itself
  struct IDirect3DVertexShader9 *VS;    // only one of these is set!
  struct IDirect3DPixelShader9 *PS;     // (or none)
};

/****************************************************************************/

struct sMaterial                        // material (shader or not)
{
  sInt RefCount;                        // reference count, so that similar shaders are only created once. 0 = not used
  sMaterialInfo Info;                   // copy of the material info for comparison of similar shaders
  sInt TexUse[4];                       // 0..3 Texture 0..3, -1=off, -2=cubemap
  sU32 States[256];                     // renderstates
  struct IDirect3DVertexShader9 *VS;    // vertex shader (or 0)
  struct IDirect3DPixelShader9 *PS;     // pixel shader (or 0);
};

/****************************************************************************/

struct sScreen
{
  sU32 Window;
//  sInt Active;
  struct IDirect3DSurface9 *DXBackBuffer;
  struct IDirect3DSurface9 *DXZBuffer;

  sU32 ZFormat;
  sU32 SFormat;
//  sU32 TFormat[sTF_MAX];
  
  sInt XSize;
  sInt YSize;
  sInt OtherRenderTarget;
//  sInt FullScreen;

  sU32 glrc;
  sU32 hdc;
  sMatrix CamMat;     // OpenGL only!
};

/****************************************************************************/

struct sSample
{
  struct IDirectSoundBuffer *Buffers[sMAXSAMPLEBUFFER];
  sInt Count;
  sInt LRU;
};


/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   System                                                             ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

struct sSystem_
{
  // **************
  //
  // PRIVAT
  //
  // **************
  void InitX();
  sInt Msg(sU32 msg,sU32 wparam,sU32 lparam);
  void Render();
  void InitScreens();
  void ExitScreens();
  void CreateGeoBuffer(sInt i,sInt dyn,sInt vertex,sInt size);

 	void ReCreateZBuffer();
  void MakeCubeNormalizer();
  void MakeSpecularLookupTex();
  void SetStates(sU32 *stream);
  void SetState(sU32 token,sU32 value);

#if !sINTRO_XXX                   // uncommenting this makes everything LARGER!
  sInt CmdFullscreen;
  sInt CmdWindowed;
  sInt CmdLowQuality;
  sInt CmdShaderLevel;
#endif
  sInt CmdLowRes;
  sInt WDeviceLost;               // device is lost
#if !sINTRO_XXX
  sInt WActiveCount;              // incremented and decremented, 0 = active;
  sInt WActiveMsg;                // maintain WActiveCount for WM_ACTIVATE 
  sInt WContinuous;               // (not implemented) enable continous rendering
  sInt WSinglestep;               // (not implemented) render just one frame
  sInt WFullscreen;               // message handling for fullscreen mode
  sInt WWindowedStyle;            // style of the window when in windowed mode (may be maximised)
  sInt WMinimized;                // window is currently minimized
  sInt WMaximized;                // window is currently maximized
  sInt WAborting;                 // application is about to terminate
  sInt WResponse;                 // enable fast responding message loop (lock framebuffer, which hurts performance)
  sInt WConstantUpdate;           // add/rem constant update count
  sInt WFocus;                    // we have focus
  sInt WAbortKeyFlag;             // the abort key (pause) was pressed
#endif
  sU32 CpuMask;
  sU32 GpuMask;

  sInt WStartTime;
  sInt WScreenCount;
  sScreen Screen[MAX_SCREEN];
  sScreen *scr;
  sInt ScreenNr;
  sInt ViewportX;
  sInt ViewportY;
  sViewport CurrentViewport;

  sInt ConfigX;
  sInt ConfigY;
  sU32 ConfigFlags;

  sInt KeyIndex;
  sU32 KeyBuffer[MAX_KEYBUFFER];
  sU32 MouseButtons;
#if !sINTRO
  sU32 KeyQual;
  sU32 MouseButtonsSave;
  sInt WMouseX;
  sInt WMouseY;
#endif

  sHardTex Textures[MAX_TEXTURE];
  sMatrix StdTexTransMat;
  sBool StdTexTransMatSet[MAX_TEXTURE];
  sInt LockedMode;
  sU32 LockedDXFVF;
  sInt LockedVertexSize;
  struct IDirect3DIndexBuffer9 *LockedIB;
  sInt LockedIBStart;
  sInt LockedIBCount;
  struct IDirect3DVertexBuffer9 *LockedVB;
  sInt LockedVBStart;
  sInt LockedVBCount;

// DirectSound

  sBool InitDS();
  void ExitDS();
  void FillDS();
  void MarkDS();
  sSample Sample[sMAXSAMPLEHANDLE];

// direct input

  sBool InitDI();
  void ExitDI();
  void PollDI();
  void AddAKey(sU32 *Scans,sInt ascii);

// general 3d stuff

  sMatrix LastCamera;             // all this for GetTransform()
  sMatrix LastMatrix;
  sMatrix LastTransform;
  sMatrix LastProjection;
  sMatrix LightMatrix;
  sBool RecalcTransform;
  sBool LightMatrixUsed;
  sGeoBuffer GeoBuffer[MAX_GEOBUFFER];
  sGeoHandle GeoHandle[MAX_GEOHANDLE];
  sU32 VBDiscardCount;
  sU32 IBDiscardCount;

  sPerfInfo PerfLast;
  sPerfInfo PerfThis;
  sInt PerfLastTime;
  sMaterialEnv LastEnv;

// DirectGraphics specific

  struct IDirect3D9 *DXD;
  struct IDirect3DDevice9 *DXDev;

  struct IDirect3DTexture9 *DXStreamTexture;
  sInt DXStreamLevel;

  sInt ZBufXSize,ZBufYSize;
	sU32 ZBufFormat;
	struct IDirect3DSurface9 *ZBuffer;

  struct IDirect3DSurface9 *DXReadTex;
  sInt DXReadTexXS;
  sInt DXReadTexYS;

  struct IDirect3DCubeTexture9 *DXNormalCube;
  sInt SpecularLookupTex;
  sInt NullBumpTex;
  sInt ShaderOn;                  // sTRUE when shader is enabled. for automatic shader disable
#if sUSE_SHADERS
  sShader Shaders[MAX_SHADERS];
  sMatrix ShaderLastProj;         // last projection matrix calculated
#endif

  sMaterial Materials[MAX_MATERIALS];

// HostFont

  sInt FontPageX;
  sInt FontPageY;
  sU32 *FontMem;

// OpenGL specific

  sU8 *DynamicBuffer;
  sInt DynamicSize;
  sInt DynamicWrite;
  sInt DynamicRead;

// ***** PUBLIC INTERFACE

// init/exit/debug

  void Log(sChar *);                                      // print out debug string
  sNORETURN void Abort(sChar *msg);                       // terminate now
  void Init(sU32 Flags,sInt xs=-1,sInt ys=-1);            // initialise system
  void Exit();                                            // terminate next frame
  void Tag();                                             // called by broker (in a direct hackish fashion)
  void *FindFunc(sChar *name);                            // find function by name. Must be declared with sEXPORT
  sInt MemoryUsed();                                      // how much ram was allocated so far?
  void CheckMem();
#if !sINTRO
  sU32 GetCpuMask() { return CpuMask; }                   // sCPU_???
  sU32 GetGpuMask() { return GpuMask; }                   // sGPU_???
  void ContinuousUpdate(sBool b) { WConstantUpdate = b; } // false->don't update when no focus
  void SetResponse(sBool fastresponse) {WResponse = fastresponse;} // enable fast responding but low performance mode
#endif

#if sPLAYER
  void Progress(sInt done,sInt max);
#endif

// input

  void GetInput(sInt id,sInputData &data);                // get mouse or joystick
  sU32 GetKeyboardShiftState();                           // get current keyboard shift state (synchrone)
  void GetKeyName(sChar *buffer,sInt size,sU32 key);      // get user-readable key name with qualifier
  sBool GetAbortKey();                                    // was abort key pressed since last call of ClearAbortKey()? (asynchrone)
  void ClearAbortKey();                                   // clear the abort-key-status
  sInt GetTime();                                         // get current time in milliseconds
  void GetPerf(sPerfInfo &,sInt mode);
  sInt GetTimeOfDay();                                    // get current time of day in seconds
  void PerfKalib();                                       // calibrate performancecounter, done by sPerfMon or system
  sU32 PerfTime();                                        // get µsec exact time 

  sBool GetWinMouse(sInt &x,sInt &y);                     // get windows cursor position
  void SetWinMouse(sInt x,sInt y);                        // set windows cursor position
  void SetWinTitle(sChar *name);                          // set window title
  void HideWinMouse(sBool hide);                          // hide windows cursor

// sound

  void SetSoundHandler(sSoundHandler hnd,sInt align=0,void *user=0);     // sets a new soundhandler. resets current sample
  sInt GetCurrentSample();                                // gets sample# played at last flip.
  sInt SampleAdd(sS16 *data,sInt size,sInt buffers=1,sInt usehandle=-1);   // size is in sterio samples, buffers is for DX-Sucking-Buffers
  void SampleRem(sInt handle);
  void SampleRemAll();
  void SamplePlay(sInt handle,sInt volume=0x10000,sInt pan=0,sInt freq=44100);

// file

  sU8 *LoadFile(sChar *name,sInt &size);                  // load file entirely, return size
  sU8 *LoadFile(sChar *name);                             // load file entirely
  sChar *LoadText(sChar *name);                           // load file entirely and add trailing zero
  sBool SaveFile(sChar *name,sU8 *data,sInt size);        // save file entirely

  sDirEntry *LoadDir(sChar *name);                        // load directory. ends with dir[n].Name[0]==0
  sBool MakeDir(sChar *name);                             // create new directory
  sBool CheckDir(sChar *name);                            // check if path is valid
  sBool CheckFile(sChar *name);                           // check if file exists is valid
  sBool RenameFile(sChar *oldname,sChar *newname);        // rename file (old and new are complete path)
  sBool DeleteFile(sChar *name);                          // delete file from disk
  void GetCurrentDir(sChar *buffer,sInt size);            // get current dir as "c:/nxn/genthree"
  sU32 GetDriveMask();                                    // get bitmask of available drives

  sBitmap *LoadBitmap(sU8 *data,sInt size);               // load bitmap using windows loader

// font

  sInt FontBegin(sInt pagex,sInt pagey,sChar *name,sInt xs,sInt ys,sInt style);  // create font and bitmap, return real height
  sInt FontWidth(sChar *string,sInt count);                // get width
  void FontPrint(sInt x,sInt y,sChar *string,sInt count); // print
  sU32 *FontEnd();                                         // return bitmap, caller has to delete it.

// graphics

  sBool GetScreenInfo(sInt i,sScreenInfo &info);          // get screen size and availability
  sInt GetScreenCount();                                  // get number of screens
  sBool GetFullscreen();                                  // at least one screen is fullscreen
  void Reset(sU32 flags,sInt x,sInt y,sInt x2,sInt y2);   // set new screen config

  void SetGamma(sF32 gamma);
  
  void BeginViewport(const sViewport &);        // begin viewport. does not set transform
  void EndViewport();                           // end viewport
  void GetTransform(sInt mode,sMatrix &mat);    // get concatenated Camera and Matrix (for bilboards etc)

  sInt AddTexture(const sTexInfo &);            // add a texture
  sInt AddTexture(sInt xs,sInt ys,sInt format,sU16 *data,sInt mipcount=0,sInt miptresh=0);  // add a texture (new version)
  void AddRefTexture(sInt tex);
  void RemTexture(sInt handle);                 // remove a texture
  void UpdateTexture(sInt handle,sBitmap *bm);    // update texture bitmap (must be same size)
  void UpdateTexture(sInt handle,sU16 *,sInt miptresh=0);         // update texture (new version)
  void ReadTexture(sInt handle,sU32 *data);
  sBool StreamTextureStart(sInt h,sInt l,sBitmapLock &);  // start texture streaming (handle,mipmap,result)
  void StreamTextureEnd();                      // done writing texture
  void FlushTexture(sInt handle);                         // let that texture relaod
  void GetTexSize(sInt handle,sInt &xs,sInt &ys);         // just read some private variables

  void SetScissor(const sFRect *scissor);       // sets scissor rectangle

  sInt GeoAdd(sInt fvf,sInt prim);              // create a geometry handle for given fvf and sGEO_??? flags
  sInt GeoAdd(sInt fvf,sInt prim,const sChar *file,sInt line);
  void GeoRem(sInt handle);                     // free handle and associated geometry
  void GeoFlush();                              // flush all static vertex buffers
  void GeoFlush(sInt handle,sInt what=3);       // flush a specific dynamic vertex buffer
  sInt GeoDraw(sInt &handle);                   // try drawing. returns updateflags.
  void GeoBegin(sInt handle,sInt vc,sInt ic,sF32 **fp,sU16 **ip,sInt upd=3); // begin upload, as requested by GeoDraw()
  void GeoEnd(sInt handle,sInt vc=-1,sInt ic=-1); // done uploading and draw finally. state how many indices/vertices were really needed.

  sInt MtrlAdd(sMaterialInfo *info);
  void MtrlAddRef(sInt mtrl);
  void MtrlRem(sInt mtrl);
  void MtrlSet(sInt mtrl,sMaterialInfo *info,const sMaterialEnv *env);
  void MtrlSet(sMaterialInfo *info,const sMaterialEnv *env) { MtrlSet(info->Handle,info,env); }
  void MtrlShadowStates(sU32 flags);
};

#if !sINTRO
#define GeoAdd(f,p) GeoAdd(f,p,__FILE__,__LINE__)
#endif

/****************************************************************************/

extern sSystem_ *sSystem;

/****************************************************************************/
/****************************************************************************/

#endif
