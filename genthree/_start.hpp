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
struct sCamera;                   // set a tranformation for rendering
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

// sSystem_::CpuMask

#define sCPU_CMOVE          0x0001        // CMOV FCMOV FCOMI instructions
#define sCPU_RDTSC          0x0002        // RDTSC instruction
#define sCPU_MMX            0x0004        // Intel MMX Instructions
#define sCPU_MMX2           0x0008        // Extended MMX instructions because of AMD Extension or SSE
#define sCPU_SSE            0x0010        // PIII SSE instructions fully supported
#define sCPU_SSE2           0x0020        // P4 SSE2 instruction fully supported
#define sCPU_3DNOW          0x0040        // 3dNow! instructions of K6
#define sCPU_3DNOW2         0x0080        // extended 3dNow! for Athlon

/****************************************************************************/

// geometry type

#define sGEO_POINT      0x000001  // draw points
#define sGEO_LINE       0x000002  // draw lines
#define sGEO_TRI        0x000003  // draw triangles
#define sGEO_LINESTRIP  0x000004  // draw lines
#define sGEO_TRISTRIP   0x000005  // draw triangles
#define sGEO_QUAD       0x000006  // draw quads (there is a static ib for that in the dx-port)

#define sGEO_DYNAMIC    0x000000  // dynamic vertex AND indexbuffer
#define sGEO_STATIC     0x000008  // static vertex AND indexbuffer

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
#define sFVF_TSPACE     3     // pos,tspace,tex0
#define sFVF_COMPACT    4     // pos,color


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
  sU32 n,s,t;
  sF32 u,v;
};

struct sVertexCompact
{
  sF32 x,y,z;
  sU32 c0;
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
  sF32 PixelRatio;                // usually 1:1, but 1280x1024 is different
};

/****************************************************************************/

struct sCamera
{
private:
  /*
#if !sINTRO
  sMatrix View;
  sMatrix Project;
  sMatrix Final;
#endif
  */
public:
  sMatrix CamPos;                 // position of camera

  sF32 ZoomX,ZoomY;               // zoom-factors (usually 1.0f)
  sF32 CenterX,CenterY;           // center-offset (usually 0.5f)
  sF32 AspectX,AspectY;           // aspect ratio (usually window size)

  sF32 NearClip;                  // near clipping plane
  sF32 FarClip;                   // far clipping plane

  sMatrix Matrix;                 // camera * projection, set by Calc()

  void Init();                    // default all, Aspect Ratio incorrect!
  void Init(const sViewport &);   // default all
  void Calc();                    // calculate matrix
  void CalcTS();                  // calculate matrix with adjustment for texture space


  /*
  void Calc();                    // prepare for SetMatrix()
  void SetMatrix(const sMatrix &); // set model matrix
  void SetMatrix2d();             // prepare 2d drawing

  void Transform3d(const sVector *in,sVector *out,sInt count);
  sInt Transform2d(const sVector *in,sVector *out,sInt count);
  void TransformScreen(const sVector *in,sVector *out,sInt count);
  */
};

// GetTransform Flags

#define sGT_UNIT          0       // returns a unit matrix. somewhat usefull...
#define sGT_VIEW          1
#define sGT_MODELVIEW     2
#define sGT_MODEL         3

/****************************************************************************/

struct sViewport
{
  sInt Screen;                    // screen to use, or -1
  sInt RenderTarget;              // texture to use, or -1

  sRect Window;                   // render window
  sU32 ClearFlags;                // sCCF_???
  sColor ClearColor;              // color to clear background

  void Init(sInt screen=0);       // initialize for use with screen
#if !sINTRO
  void InitTex(sInt handle);      // initialize for rendertarget
#endif
};

// clear flags

#define sVCF_NONE       0         // nothing
#define sVCF_COLOR      1         // color only
#define sVCF_Z          2         // zbuffer only
#define sVCF_ALL        3         // all

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

struct sDLLSYSTEM sInputData
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

#define sKEY_PAUSE        0x00010010      // Pause/Break key
#define sKEY_SCROLL       0x00010011      // ScrollLock key
#define sKEY_NUMLOCK      0x00010012      // NumLock key
#define sKEY_WINL         0x00010013      // left windows key
#define sKEY_WINR         0x00010014      // right windows key
#define sKEY_APPS         0x00010015      // windows menu key
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

struct sDLLSYSTEM sHostFontLetter
{
  sRect Location;
  sInt Pre;
  sInt Post;
  sInt Top;
  sInt pad;
};

/****************************************************************************/

class sDLLSYSTEM sHostFont
{
  struct sHostFontPrivate *prv;
  void Begin(sBitmap *bm);
  void Print(sChar *text,sInt x,sInt y,sU32 c0,sInt len=-1);
  void End(sBitmap *bm);

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
  void GetWidth(sChar *);
  void Print(sBitmap *,sChar *,sInt x,sInt y,sU32 color=0xffffffff,sInt len=-1);
  sBool Prepare(sBitmap *,sChar *,sHostFontLetter *);
  sBool PrepareCheck(sInt xs,sInt ys,sChar *);
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

struct sDLLSYSTEM sDirEntry
{
  sChar Name[256];                // filename        
  sInt Size;                      // size of file in bytes
  sBool IsDir;                    // this is a directory, size is 0
  sU8 pad[3];                     // pad it up..
};

/****************************************************************************/

typedef void (*sSoundHandler)(sS16 *steriobuffer,sInt samples,void *user); 

/****************************************************************************/
/***                                                                      ***/
/***   System                                                             ***/
/***                                                                      ***/
/****************************************************************************/

#include "_startprv.hpp"

#if sINTRO

#include "_startintro.hpp"

#endif

/****************************************************************************/

#if !sINTRO

struct sSystem_ : public sSystemPrivate
{

// init/exit/debug

  void Log(sChar *);                                      // print out debug string
  sNORETURN void Abort(sChar *msg);                       // terminate now
  void Init(sU32 Flags,sInt xs=-1,sInt ys=-1);            // initialise system
  void Exit();                                            // terminate next frame
  void Tag();                                             // called by broker (in a direct hackish fashion)
  void *FindFunc(sChar *name);                            // find function by name. Must be declared with sEXPORT
  sInt MemoryUsed();                                      // how much ram was allocated so far?
  static sU32 CpuMask;
  static sU32 GfxSystem;
  sU32 GetCpuMask() { return CpuMask; }                   // sCPU_???
  sInt GetGfxSystem() { return GfxSystem; }               // sSF_DIRECT3D or sSF_OPENGL

// input

  void GetInput(sInt id,sInputData &data);                // get mouse or joystick
  sU32 GetKeyboardShiftState();                           // get current keyboard shift state
  void GetKeyName(sChar *buffer,sInt size,sU32 key);      // get user-readable key name with qualifier
  sInt GetTime();                                         // get current time in milliseconds
#if !sINTRO
  sInt GetTimeOfDay();                                    // get current time of day in seconds
  void PerfKalib();                                       // calibrate performancecounter, done by sPerfMon or system
  sU32 PerfTime();                                        // get µsec exact time 
#endif

  sBool GetWinMouse(sInt &x,sInt &y);                     // get windows cursor position
  void SetWinMouse(sInt x,sInt y);                        // set windows cursor position
  void SetWinTitle(sChar *name);                          // set window title
  void HideWinMouse(sBool hide);                          // hide windows cursor
  void SetResponse(sBool fastresponse) {WResponse = fastresponse;} // enable fast responding but low performance mode

// sound

  void SetSoundHandler(sSoundHandler hnd,sInt align=0,void *user=0);     // sets a new soundhandler. resets current sample
  sInt GetCurrentSample();                                // gets sample# played at last flip.

// file

  sU8 *LoadFile(sChar *name,sInt &size);                  // load file entirely, return size
  sU8 *LoadFile(sChar *name);                             // load file entirely
  sChar *LoadText(sChar *name);                           // load file entirely and add trailing zero
  sBool SaveFile(sChar *name,sU8 *data,sInt size);        // save file entirely

  sDirEntry *LoadDir(sChar *name);                        // load directory. ends with dir[n].Name[0]==0
  sBool MakeDir(sChar *name);                             // create new directory
  sBool CheckDir(sChar *name);                            // check if path is valid
  sBool RenameFile(sChar *oldname,sChar *newname);        // rename file (old and new are complete path)
  sBool DeleteFile(sChar *name);                          // delete file from disk
  void GetCurrentDir(sChar *buffer,sInt size);            // get current dir as "c:/nxn/genthree"
  sU32 GetDriveMask();                                    // get bitmask of available drives

// graphics

  sBool GetScreenInfo(sInt i,sScreenInfo &info);          // get screen size and availability
  sInt GetScreenCount();                                  // get number of screens
  sBool GetFullscreen();                                  // at least one screen is fullscreen
  void Reset(sU32 flags,sInt x,sInt y,sInt x2,sInt y2);   // set new screen config

  
  virtual void BeginViewport(const sViewport &)=0;        // begin viewport. does not set transform
  virtual void EndViewport()=0;                           // end viewport
  virtual void SetCamera(sCamera &)=0;                    // set viewport and projection transform 
  virtual void SetCameraOrtho()=0;                        // set viewport for 1:1 pixel orthogonal
  virtual void SetMatrix(sMatrix &mat)=0;                 // set world transform (object position)
  virtual void GetTransform(sInt mode,sMatrix &mat)=0;    // get concatenated Camera and Matrix (for bilboards etc)

  virtual sInt AddTexture(const sTexInfo &)=0;            // add a texture
  virtual sInt AddTexture(sInt xs,sInt ys,sInt format,sU16 *data)=0;  // add a texture (new version)
  virtual void RemTexture(sInt handle)=0;                 // remove a texture
  virtual void UpdateTexture(sInt handle,sBitmap *bm);    // update texture bitmap (must be same size)
  virtual void UpdateTexture(sInt handle,sU16 *);         // update texture (new version)
  virtual void StreamTextureStart(sInt h,sInt l,sBitmapLock &)=0;  // start texture streaming (handle,mipmap,result)
  virtual void StreamTextureEnd()=0;                      // done writing texture
  void FlushTexture(sInt handle);                         // removes texture from direct3d for all screens
  virtual void SetTexture(sInt stage,sInt handle,sMatrix *mat=0)=0; // select texture and texture transform matrix. matrix is adjusted for packed textures.
  virtual void SetStates(sU32 *stream)=0;                 // set state stream 
  virtual void SetState(sU32 token,sU32 value)=0;         // set single state

  virtual sInt GeoAdd(sInt fvf,sInt prim)=0;              // create a geometry handle for given fvf and sGEO_??? flags
  virtual void GeoRem(sInt handle)=0;                     // free handle and associated geometry
  virtual void GeoFlush()=0;                              // flush all static vertex buffers
  virtual void GeoFlush(sInt handle)=0;                   // flush a specific dynamic vertex buffer
  virtual sBool GeoDraw(sInt &handle)=0;                  // try drawing. if result is true, drawing faled and geometry needs to be uploaded
  virtual void GeoBegin(sInt handle,sInt vc,sInt ic,sF32 **fp,sU16 **ip)=0; // begin upload, as requested by GeoDraw()
  virtual void GeoEnd(sInt handle,sInt vc=-1,sInt ic=-1)=0; // done uploading and draw finally. state how many indices/vertices were really needed.

  virtual void StateBase(sU32 *&data,sU32 flags,sInt tex,sU32 col)=0; // initialises for simple drawing
  virtual void StateTex(sU32 *&data,sInt nr,sU32 flags)=0;            // add a texture specific flags
  virtual void StateLightEnv(sU32 *&data,sLightEnv &)=0;              // sets how to react on lights ("material")
  virtual void StateEnd(sU32 *&data,sU32 *buffer,sInt size)=0;        // appends end token and checks for buffer overflow
  virtual sBool StateValidate(sU32 *buffer)=0;                        // this is a dummy
  virtual sU32 *StateOptimise(sU32 *buffer)=0;                        // throw out doubles, returns ptr to end token in case you want to ad more

#if sUSE_SHADERS
  virtual sInt ShaderCompile(sBool ps,sChar *text)=0;                 // compile a vertex or pixel shader
  virtual void ShaderFree(sInt)=0;                                    // free a shader
  virtual void ShaderWrite(sU32 *&data)=0;                            // write all shaders
  virtual void ShaderRead(sU32 *&data)=0;                             // read all shaders
  virtual void ShaderTex(sInt nr,sInt flags,sInt tex)=0;              // set texture sampler for shaders
  virtual void ShaderCam(sCamera *,sMatrix &obj,sCamera *lcam=0)=0;   // set transform matrix to first VS-constants
  virtual void ShaderConst(sBool ps,sInt ofs,sInt cnt,sVector *c)=0;  // set shader constants
  virtual void ShaderSet(sInt vs,sInt ps,sU32 flags)=0;               // set both vertex and pixel shader
  virtual void ShaderClear()=0;                                       // after using shaders, please disable!
#endif

  virtual void ClearLights()=0;                           // remove all lightsource
  virtual sInt AddLight(sLightInfo &)=0;                  // add a lightsource, don't enable
  virtual void EnableLights(sU32 mask)=0;                 // enable all lights that match mask
  virtual sInt GetLightCount()=0;                         // return number of lights set
};

#endif

/****************************************************************************/

#if !sINTRO

struct sSystemDX : public sSystem_
{
  void Render();
	void ReCreateZBuffer();

  void InitScreens();
  void ExitScreens();
//  void ParseFVF(sU32 fvf,sU32 &dxfvf,sInt &floats);
  void CreateGeoBuffer(sInt i,sInt dyn,sInt vertex,sInt size);

  void BeginViewport(const sViewport &);
  void EndViewport();
  void SetCamera(sCamera &);
  void SetCameraOrtho();
  void SetMatrix(sMatrix &mat);
  void GetTransform(sInt mode,sMatrix &mat);

  sInt AddTexture(const sTexInfo &);
  sInt AddTexture(sInt xs,sInt ys,sInt format,sU16 *data);
  void RemTexture(sInt handle);
  void StreamTextureStart(sInt h,sInt l,sBitmapLock &);
  void StreamTextureEnd();
  void SetTexture(sInt stage,sInt handle,sMatrix *mat=0);
  void SetStates(sU32 *stream);
  void SetState(sU32 token,sU32 value);

  sInt GeoAdd(sInt fvf,sInt prim);
  void GeoRem(sInt handle);
  void GeoFlush();
  void GeoFlush(sInt handle);
  sBool GeoDraw(sInt &handle);
  void GeoBegin(sInt handle,sInt vc,sInt ic,sF32 **fp,sU16 **ip);
  void GeoEnd(sInt handle,sInt vc=-1,sInt ic=-1);

  void StateBase(sU32 *&data,sU32 flags,sInt texmode,sU32 color);
  void StateTex(sU32 *&data,sInt nr,sU32 flags);
  void StateLightEnv(sU32 *&data,sLightEnv &);
  void StateEnd(sU32 *&data,sU32 *buffer,sInt size);
  sBool StateValidate(sU32 *buffer);
  sU32 *StateOptimise(sU32 *buffer);

#if sUSE_SHADERS
  sInt ShaderCompile(sBool ps,sChar *text);
  void ShaderFree(sInt);
  void ShaderWrite(sU32 *&data);
  void ShaderRead(sU32 *&data);
  void ShaderTex(sInt nr,sInt flags,sInt tex);
  void ShaderCam(sCamera *,sMatrix &obj,sCamera *lcam=0);
  void ShaderConst(sBool ps,sInt ofs,sInt cnt,sVector *c);
  void ShaderSet(sInt vs,sInt ps,sU32 flags);
  void ShaderClear();
#endif

  void ClearLights();
  sInt AddLight(sLightInfo &);
  void EnableLights(sU32 mask);
  sInt GetLightCount();
};

#endif

/****************************************************************************/

#if !sINTRO

struct sSystemGL : public sSystem_
{
  void Render();

  void InitScreens();
  void ExitScreens();
//  void ParseFVF(sU32 fvf,sU32 &dxfvf,sInt &floats);
  void CreateGeoBuffer(sInt i,sInt dyn,sInt vertex,sInt size);

  void BeginViewport(const sViewport &);
  void EndViewport();
  void SetCamera(sCamera &);
  void SetCameraOrtho();
  void SetMatrix(sMatrix &mat);
  void GetTransform(sInt mode,sMatrix &mat);

  sInt AddTexture(const sTexInfo &);
  sInt AddTexture(sInt xs,sInt ys,sInt format,sU16 *data);
  void RemTexture(sInt handle);
  void StreamTextureStart(sInt h,sInt l,sBitmapLock &);
  void StreamTextureEnd();
  void SetTexture(sInt stage,sInt handle,sMatrix *mat=0);
  void SetStates(sU32 *stream);
  void SetState(sU32 token,sU32 value);

  sInt GeoAdd(sInt fvf,sInt prim);
  void GeoRem(sInt handle);
  void GeoFlush();
  void GeoFlush(sInt handle);
  sBool GeoDraw(sInt &handle);
  void GeoBegin(sInt handle,sInt vc,sInt ic,sF32 **fp,sU16 **ip);
  void GeoEnd(sInt handle,sInt vc=-1,sInt ic=-1);

  void StateBase(sU32 *&data,sU32 flags,sInt texmode,sU32 color);
  void StateTex(sU32 *&data,sInt nr,sU32 flags);
  void StateLightEnv(sU32 *&data,sLightEnv &);
  void StateEnd(sU32 *&data,sU32 *buffer,sInt size);
  sBool StateValidate(sU32 *buffer);
  sU32 *StateOptimise(sU32 *buffer);

#if sUSE_SHADERS
  sInt ShaderCompile(sBool ps,sChar *text);
  void ShaderFree(sInt);
  void ShaderWrite(sU32 *&data);
  void ShaderRead(sU32 *&data);
  void ShaderTex(sInt nr,sInt flags,sInt tex);
  void ShaderCam(sCamera *,sMatrix &obj,sCamera *lcam=0);
  void ShaderConst(sBool ps,sInt ofs,sInt cnt,sVector *c);
  void ShaderSet(sInt vs,sInt ps,sU32 flags);
  void ShaderClear();
#endif

  void ClearLights();
  sInt AddLight(sLightInfo &);
  void EnableLights(sU32 mask);
  sInt GetLightCount();
};

#endif

extern sSystem_ *sSystem;

/****************************************************************************/
/****************************************************************************/

#endif
