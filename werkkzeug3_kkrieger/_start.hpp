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

sBool sAppHandler(sInt code,sDInt value);  

// codes for sAppHandler. called in this order

#define sAPPCODE_CONFIG       1   // parse commandline and call sSystem::Init()
#define sAPPCODE_INIT         2   // init your system
#define sAPPCODE_KEY          3   // keyboard input
#define sAPPCODE_FRAME        4   // called once each frame
#define sAPPCODE_TICK         5   // called for each 10ms timeslice
#define sAPPCODE_PAINT        6   // called once each frame
#define sAPPCODE_EXIT         7   // free resources. 
#define sAPPCODE_CMD          9   // cmd from sGuiManager
#define sAPPCODE_DEBUGPRINT   10  // print a line of text. value is (char *)

/****************************************************************************/

// config

#define sSF_MULTISCREEN   0x0010  // open window on each screen
#define sSF_FULLSCREEN    0x0020  // go fullscreen (on all screens)
#define sSF_FULLOTHER     0x0080  // all but first screen go fullscreen
#define sSF_MAXIMISE      0x0040  // in windowed mode, max window(s)
#define sSF_WAITVSYNC     0x0080  // wait vor vsync
#define sSF_MINIMAL       0x0100  // in windowed mode, no windows title border

#define sSF_GRAPHMASK     0x000f
#define sSF_DIRECT3D      0x0001

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

#define sGEO_IND32B     0x000020  // 32bit index buffer

#define sGEO_DYNAMIC    sGEO_DYNVB|sGEO_DYNIB
#define sGEO_STATIC     sGEO_STATVB|sGEO_STATIB

#define sGEO_VERTEX     0x000001  // for GeoFlush/GeoDraw return code
#define sGEO_INDEX      0x000002  // for GeoFlush/GeoDraw return code

/****************************************************************************/

#define sFVF_STANDARD   1     // pos,normal,tex0
#define sFVF_DOUBLE     2     // pos,color,tex0,tex1
#define sFVF_TSPACE     3     // pos,tspace,tex0,tex1 (not packed)
#define sFVF_COMPACT    4     // pos,color
#define sFVF_XYZW       5     // pos(4-component)
#define sFVF_TSPACE3    6     // pos,tspace,color,tex0 (packed)
#define sFVF_TSPACE3BIG 7     // pos,tscape,color,tex0

struct sVertexStandard // 32 bytes
{
  sF32 x,y,z;
  sF32 nx,ny,nz;
  sF32 u,v;
};

struct sVertexDouble // 32 bytes
{
  sF32 x,y,z;
  sU32 c0;
  sF32 u0,v0;
  sF32 u1,v1;

  __forceinline void Init(sF32 X,sF32 Y,sF32 Z=0.5f,
    sU32 C0=~0,sF32 U0=0,sF32 V0=0,sF32 U1=0,sF32 V1=0)
  { x=X;y=Y;z=Z;c0=C0;u0=U0;v0=V0;u1=U1;v1=V1; }
};

struct sVertexTSpace // 64 bytes
{
  sF32 x,y,z;
  sF32 nx,ny,nz;
  sU32 c0,c1;
  sF32 u0,v0;
  sF32 sx,sy,sz;
  sF32 tx,ty,tz;
};

struct sVertexCompact // 16 bytes
{
  sF32 x,y,z;
  sU32 c0;

  __forceinline void Init(sF32 X,sF32 Y,sF32 Z,sU32 C)
  { x=X;y=Y;z=Z;c0=C; }
};

struct sVertexTSpace3 // 32 bytes
{
  sF32 x,y,z;
  sU32 n,s,c;
  sF32 u,v;

  __forceinline void Init(sF32 X,sF32 Y,sF32 Z,sU32 C,sF32 U,sF32 V,
    sU32 N=0x808000,sF32 S=0x80ff80)
  { x=X;y=Y;z=Z;c=C;u=U;v=V;n=N;s=S; }
};

struct sVertexTSpace3Big // 48 bytes
{
  sF32 x,y,z;
  sF32 nx,ny,nz;
  sF32 sx,sy,sz;
  sU32 c;
  sF32 u,v;
};

struct sVertexXYZW // 16 bytes
{
  sF32 x,y,z,w;
};

/****************************************************************************/

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
  sBool LowQuality;               // low quality mode (for reduced video memory usage);
  sInt ShaderLevel;               // supported Shader level
  sF32 PixelRatio;                // usually 1:1, but 1280x1024 is different
};

                                  // Shader Level
#define sPS_00        0
#define sPS_11        1
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

#define sGT_VIEW          0
#define sGT_MODELVIEW     1
#define sGT_MODEL         2
#define sGT_PROJECT       3

/****************************************************************************/
/****************************************************************************/

struct sViewport
{
  sInt Screen;                    // screen to use, or -1
  sInt RenderTarget;              // texture to use, or -1

  sRect Window;                   // render window

  void Init(sInt screen=0);       // initialize for use with screen
  void InitTex(sInt handle);      // initialize for rendertarget
};

// clear flags

#define sVCF_NONE       0         // nothing
#define sVCF_COLOR      1         // color only
#define sVCF_Z          2         // zbuffer *and* stencil
#define sVCF_STENCIL    4         // stencil only
#define sVCF_ALL        7         // all
#define sVCF_PARTIAL    8

/****************************************************************************/

// flags
#define sTIF_RENDERTARGET 0x0001  // can be used as rendertarget
#define sTIF_ALLOCATED    0x0002  // used internally to mark allocated texture handles

// texture formats (for use with the new addtexture)
#define sTF_NONE            0     // illegal texture format
#define sTF_A8R8G8B8        1     // 4 x 8 bit unsigned
#define sTF_A8              2     // 1 x 8 bit unsigned
#define sTF_R16F            3     // 1 x 16 bit float
#define sTF_A2R10G10B10     4     // 3 x 10 bit unsigned + 2 bit unsigned
#define sTF_Q8W8V8U8        5     // 4 x 8 bit signed
#define sTF_A2W10V10U10     6     // 3 x 10 bit signed + 2 bit unsigned
#define sTF_A1R5G5B5        7     // 3 x 5 bit unsigned + 1 bit
#define sTF_MAX             8     // end of list

// the structure
struct sTexInfo
{
  sInt XSize;
  sInt YSize;
  sU32 Flags;                     // sTIF_???
  sInt Format;                    // requested sTF_??

  sBitmap *Bitmap;                // texture bitmap

  void Init(sBitmap *,sInt format=sTF_A8R8G8B8,sU32 flags=0);
  void InitRT(sInt x=0,sInt y=0);
};

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
#define sHFS_BOLD     0x0002      // bold 
#define sHFS_ITALICS  0x0001      // italics
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
#define MAX_TEXTURE   1024
#define MAX_TEXSTAGE  16
#define MAX_DYNVBSIZE (64*0x8000)
#define MAX_DYNIBSIZE (3*2*0x40000)
#define MAX_GEOBUFFER 2048
#define MAX_GEOHANDLE 4096
#define MAX_SETUPS    1024
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
  sBool Alloc(sInt count,sInt size,sInt &first,sInt type);  // alloc space for COUNT elements of size SIZE, return first aligned byte index in FIRST if it fits.
  void Free();
};

struct sGeoBufferRef              // geobuffer reference
{
  sInt Buffer;                    // sGeoBuffer handle
  sInt Start;                     // start offset
  sInt Count;                     // number of elements
  sInt DiscardCount;              // discard counter
};

struct sGeoHandle
{
  sInt Mode;                      // sGEO_??? 
  sU32 FVF;                       // FVF table index
  sInt VertexSize;                // size of one vertex in bytes
  sInt Locked;                    // indicates whether indices/vertices are locked
  
  sGeoBufferRef Vertex;           // vertex geobuffer
  sGeoBufferRef Index;            // index geobuffer
};

/****************************************************************************/

struct sHardTex
{
  sInt RefCount;                  // texture-handles haben jetzt auch refcounting
  sInt XSize;
  sInt YSize;
  sInt MipLevels;                 // number of mip levels
  sU32 Flags;                     // sTIF_???
  sInt Format;                    // requested sTF_??

  struct IDirect3DTexture9 *Tex;
};

/****************************************************************************/

#define sMEO_PERSPECTIVE  0       // perspective view
#define sMEO_NORMALISED   1       // orthogonal view, -1 .. 1
#define sMEO_PIXELS       2       // orthogonal view, pixels from (0,0) to (xsize-1,ysize-1) inclusive

struct sMaterialEnv
{
  sVector LightPos;               // for sMLF_BUMPX mode
  sVector LightColor;
  sF32 LightRange;
  sF32 LightAmplify;

  sF32 FogStart;                  // fogging
  sF32 FogEnd;
  sU32 FogColor;

  sMatrix ModelSpace;             // Model in World
  sMatrix CameraSpace;            // Camera in World
  sF32 ZoomX,ZoomY;               // zoom-factors (usually 1.0f)
  sF32 CenterX,CenterY;           // center-offset (usually 0.0f)
  sF32 NearClip;                  // near clipping plane
  sF32 FarClip;                   // far clipping plane
  sInt Orthogonal;                // no perspective 

  void Init();
  void MakeProjectionMatrix(sMatrix &m) const;
};

class sMaterial                   // material base class
{
  sInt RefCount;

public:
  sMaterial()                     { RefCount = 1; }
  virtual ~sMaterial();

  virtual void Set(const sMaterialEnv &env) = 0;

  void AddRef()                   { RefCount++; }
  void Release()                  { if(--RefCount == 0) delete this; }
};

struct sMaterialSetup             // actual data used by D3D
{
  sInt RefCount;

  void AddRef()                   { RefCount++; }
  void Release()                  { if(--RefCount == 0) Cleanup(); }
  void Cleanup();

  sU32 *States;                   // render states
  sU32 *VSCode;                   // vertexshader bytecode
  sU32 *PSCode;                   // pixelshader bytecode

  struct IDirect3DVertexShader9 *VS;    // vertex shader
  struct IDirect3DPixelShader9 *PS;     // pixel shader
};

struct sMaterialInstance                // set of textures+constants
{
  sInt NumTextures;                     // # of textures
  sInt Textures[MAX_TEXSTAGE];          // textures
  
  sInt NumVSConstants;                  // # of vertex shader constants
  sVector *VSConstants;                 // vertex shader constants

  sInt NumPSConstants;                  // # of pixel shader constants
  sVector *PSConstants;                 // pixel shader constants

  static sMaterialInstance Null;
};

/****************************************************************************/

class sSimpleMaterial : public sMaterial
{
  sInt Setup;                     // material setup
  sInt Tex;                       // texture

public:
  sU32 Color;                     // constant color (if used)

  sSimpleMaterial(sInt tex,sU32 flags,sU32 flags2,sU32 color=0);
  ~sSimpleMaterial();

  void SetTex(sInt tex);
  void Set(const sMaterialEnv &env);
};

/****************************************************************************/

struct sScreen
{
  sDInt Window;

  sU32 ZFormat;
  sU32 SFormat;
  
  sInt XSize;
  sInt YSize;
};

/****************************************************************************/

struct sSampleBuffer
{
  struct IDirectSoundBuffer *Buffer;
  struct IDirectSound3DBuffer *Buf3D;
  sInt PlayTime;
};

struct sSample
{
  sSampleBuffer Buffers[sMAXSAMPLEBUFFER];
  sBool Is3D;
  sInt Count;
  sInt LRU;
  sInt Uses;
  sInt LenMs;
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
  void CreateGeoBuffer(sInt i,sInt dyn,sInt index,sInt size);

 	void ReCreateZBuffer();
  void MakeCubeNormalizer();
  void MakeAttenuationVolume();
  void SetStates(sU32 *stream);
  void SetState(sU32 token,sU32 value);

  void AllocBufferInternal(sGeoBufferRef &ref,sInt size,sBool stat,sInt index,void **ptr);

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
#if !sINTRO
  sInt HardwareShaderLevel;
#endif

  sInt WStartTime;
  sInt WScreenCount;
  sScreen Screen[MAX_SCREEN];
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

// DirectSound

  sBool InitDS();
  void ExitDS();
  void FillDS();
  void MarkDS();
  sSample Sample[sMAXSAMPLEHANDLE];
  struct IDirectSound3DListener *Listener;
  sInt SoundTime;

// direct input

  sBool InitDI();
  void ExitDI();
  void PollDI();
  void AddAKey(sU32 *Scans,sInt ascii);

// general 3d stuff

  sMatrix LastCamera;             // all this for GetTransform()
  sMatrix LastMatrix;
  sMatrix LastProjection;
  sMatrix LastViewProject;

  sGeoBuffer GeoBuffer[MAX_GEOBUFFER];
  sGeoHandle GeoHandle[MAX_GEOHANDLE];
  sInt DiscardCount[3];           // 0=vertex 1=index16 2=index32

  sInt LastDecl;
  sInt LastVB;
  sInt LastVSize;
  sInt LastIB;
  sBool MtrlReset;

#if !sINTRO
  sPerfInfo PerfLast;
  sPerfInfo PerfThis;
#endif
  sInt PerfLastTime;

// DirectGraphics specific

  struct IDirect3D9 *DXD;
  struct IDirect3DDevice9 *DXDev;

  struct IDirect3DTexture9 *DXStreamTexture;
  sInt DXStreamLevel;

  sInt ZBufXSize,ZBufYSize;
	sU32 ZBufFormat;
	struct IDirect3DSurface9 *ZBuffer;
  struct IDirect3DSurface9 *DXBlockSurface[2];

  struct IDirect3DCubeTexture9 *DXNormalCube;
  struct IDirect3DVolumeTexture9 *DXAttenuationVolume;

  sInt CurrentTarget;

// new material system
  sMaterialSetup Setups[MAX_SETUPS];
  sInt CurrentSetupId;
  struct IDirect3DVertexShader9 *CurrentVS;
  struct IDirect3DPixelShader9 *CurrentPS;
  sInt CurrentTextures[MAX_TEXSTAGE];
  sU32 CurrentStates[0x310];

// HostFont
  sU32 *FontMem;

#ifdef _DOPE
  sInt TexMemAlloc;
  sInt BufferMemAlloc;
#endif

// ***** PUBLIC INTERFACE

// init/exit/debug

  void Log(sChar *);                                      // print out debug string
  sNORETURN void Abort(sChar *msg);                       // terminate now
  void Init(sU32 Flags,sInt xs=-1,sInt ys=-1);            // initialise system
  void Exit();                                            // terminate next frame
  void Tag();                                             // called by broker (in a direct hackish fashion)

  sInt MemoryUsed();                                      // how much ram was allocated so far?
  void CheckMem();
#if !sINTRO
  sU32 GetCpuMask() { return CpuMask; }                   // sCPU_???
  sU32 GetGpuMask() { return GpuMask; }                   // sGPU_???
  sInt GetShaderLevel() { return HardwareShaderLevel; }     // sPS_??
  void ContinuousUpdate(sBool b) { WConstantUpdate = b; } // false->don't update when no focus
  void SetResponse(sBool fastresponse) {WResponse = fastresponse;} // enable fast responding but low performance mode
#endif
  sChar *GetCmdLine();

#if sPLAYER
  void Progress(sInt done,sInt max);
  void WaitForKey();                                      // paint black screen and wait for key
#endif

// windows user interface access

  sBool FileRequester(sChar *buffer,sInt size,sU32 flags);

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
  sBool GetWinMouseAbs(sInt &x,sInt &y);                  // get windows cursor position (screen coordinates)
  void SetWinMouse(sInt x,sInt y);                        // set windows cursor position
  void SetWinTitle(sChar *name);                          // set window title
  void MoveWindow(sInt dx,sInt dy);                       // drag the window around
  void SetWinMode(sInt mode);                             // 0=normal, 1=max, 2=min
  sInt GetWinMode();                                      // siehe oben
  void HideWinMouse(sBool hide);                          // hide windows cursor

// sound

  void SetSoundHandler(sSoundHandler hnd,sInt align=0,void *user=0);     // sets a new soundhandler. resets current sample
  sInt GetCurrentSample();                                // gets sample# played at last flip.
#if sLINK_KKRIEGER
  sInt SampleAdd(sS16 *data,sInt size,sInt buffers=1,sInt usehandle=-1,sBool is3d=sFALSE);   // size is in stereo samples, buffers is for DX-Sucking-Buffers
  void SampleRem(sInt handle);
  void SampleRemAll();
  sInt SamplePlay(sInt handle,sF32 volume=1.0f,sF32 pan=0,sInt freq=44100);
  
// 3d sound
  void Sample3DParam(sInt handle,sInt buffer,const sVector &pos,const sVector &vel,sF32 minDist,sF32 maxDist);
  void Sample3DListener(const sVector &pos,const sVector &vel,const sVector &up,const sVector &fwd,sF32 doppler=1.0f,sF32 rolloff=1.0f);
  void Sample3DCommit();
#endif

// file

  sU8 *LoadFile(const sChar *name,sInt &size);                  // load file entirely, return size
  sU8 *LoadFile(const sChar *name);                             // load file entirely
  sU8 *LoadFileIfNewerThan(const sChar *name,const sChar *other,sInt &size);  // for caching
  sChar *LoadText(const sChar *name);                           // load file entirely and add trailing zero
  sBool SaveFile(const sChar *name,const sU8 *data,sInt size);  // save file entirely  
  sU64 GetFileStamp(const sChar *name);                         // get a number. when it changes, the file has changed...

  sDirEntry *LoadDir(const sChar *name);                        // load directory. ends with dir[n].Name[0]==0
  sBool MakeDir(const sChar *name);                             // create new directory
  sBool CheckDir(const sChar *name);                            // check if path is valid
  sBool CheckFile(const sChar *name);                           // check if file exists is valid
  sBool RenameFile(const sChar *oldname,const sChar *newname);  // rename file (old and new are complete path)
  sBool DeleteFile(const sChar *name);                          // delete file from disk
  void GetCurrentDir(sChar *buffer,sInt size);            // get current dir as "c:/nxn/genthree"
  sU32 GetDriveMask();                                    // get bitmask of available drives

  

  sBitmap *LoadBitmap(const sU8 *data,sInt size);               // load bitmap using windows loader
  sBool LoadBitmapCore(const sU8 *data,sInt size,sInt &x,sInt &y,sU8 *&d);   // simplified version for intro

// font

  sInt FontBegin(sInt pagex,sInt pagey,const sChar *name,sInt xs,sInt ys,sInt style);  // create font and bitmap, return real height
  sInt FontWidth(const sChar *string,sInt count);                // get width
  void FontCharWidth(sInt ch,sInt *widths);                // get char width (with kerning)
  void FontPrint(sInt x,sInt y,const sChar *string,sInt count); // print
  void FontPrint(sInt x,sInt y,const sU16 *string,sInt count); // print
  sU32 *FontBitmap() { return FontMem; }                  // return font bitmap
  void FontEnd();                                         // delete bitmap.

// graphics

  sBool GetScreenInfo(sInt i,sScreenInfo &info);          // get screen size and availability
  sInt GetScreenCount();                                  // get number of screens
  sBool GetFullscreen();                                  // at least one screen is fullscreen
  void Reset(sU32 flags,sInt x,sInt y,sInt x2,sInt y2);   // set new screen config

  void SetGamma(sF32 gamma);

  void Clear(sU32 flags,sU32 color=0);          // clear.
  void SetViewport(const sViewport &vp);        // set viewport.
  void GrabScreen(sInt sourceRT,const sRect &win,sInt tex,sBool stretch=sTRUE);   // grab screen
  void Begin2D(sU32 **buffer,sInt &stride);
  void End2D();

  void GetTransform(sInt mode,sMatrix &mat);    // get concatenated Camera and Matrix (for bilboards etc)

  sInt AddTexture(const sTexInfo &);            // add a texture
  sInt AddTexture(sInt xs,sInt ys,sInt format,sU16 *data,sInt mipcount=0,sInt miptresh=0);  // add a texture (new version)
  void AddRefTexture(sInt tex);
  void RemTexture(sInt handle);                 // remove a texture
  void GetTextureSize(sInt tex,sInt &w,sInt &h);
  void UpdateTexture(sInt handle,sBitmap *bm);    // update texture bitmap (must be same size)
  void UpdateTexture(sInt handle,sU16 *,sInt miptresh=0);         // update texture (new version)
  void ReadTexture(sInt handle,sU16 *data);
  sBool StreamTextureStart(sInt h,sInt l,sBitmapLock &);  // start texture streaming (handle,mipmap,result)
  void StreamTextureEnd();                      // done writing texture
  void FlushTexture(sInt handle);                         // let that texture relaod

  void SetScissor(const sFRect *scissor);       // sets scissor rectangle

  sInt GeoAdd(sInt fvf,sInt prim);              // create a geometry handle for given fvf and sGEO_??? flags
  void GeoRem(sInt handle);                     // free handle and associated geometry
  void GeoFlush();                              // flush all static vertex buffers
  void GeoFlush(sInt handle,sInt what=3);       // flush a specific dynamic vertex buffer
  sInt GeoDraw(sInt &handle);                   // try drawing. returns updateflags.
  void GeoBegin(sInt handle,sInt vc,sInt ic,sF32 **fp,void **ip,sInt upd=3); // begin upload, as requested by GeoDraw()
  void GeoEnd(sInt handle,sInt vc=-1,sInt ic=-1); // done uploading and draw finally. state how many indices/vertices were really needed.

  void SetViewProject(const sMaterialEnv *env);

#if !sPLAYER
  void PrintShader(const sU32 *shader);
#endif

  sInt MtrlAddSetup(const sU32 *states,const sU32 *vs,const sU32 *ps);
  void MtrlAddRefSetup(sInt sid);
  void MtrlRemSetup(sInt sid);
  void MtrlClearCaches();
  void MtrlSetSetup(sInt sid);
  void MtrlSetInstance(const sMaterialInstance &inst);

// video recorder
#if !sPLAYER
  sBool VideoStart(sChar *filename,sF32 fpsrate,sInt xRes,sInt yRes);
  void VideoViewport(sViewport &vp);
  sInt VideoWriteFrame(sF32 &u1,sF32 &v1); // returns rendertarget tex
  void VideoEnd();
#endif
};

/****************************************************************************/

// for FileRequester

#define sFRF_MODEMASK   0x0003
#define sFRF_OPEN       0x0000
#define sFRF_SAVE       0x0001
#define sFRF_PATH       0x0002

/****************************************************************************/

extern sSystem_ *sSystem;

/****************************************************************************/
/****************************************************************************/

#endif
