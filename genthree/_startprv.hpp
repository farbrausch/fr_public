// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef __STARTPRV_HPP__
#define __STARTPRV_HPP__

#include "_types.hpp"

/****************************************************************************/

#define MAX_SCREEN    4
#define MAX_KEYBUFFER 32
#define MAX_DYNBUFFER 8
#define MAX_TEXTURE 128
#define MAX_DYNVBSIZE (48*0x8000)
#define MAX_DYNIBSIZE (8*0x8000)
#define MAX_GEOBUFFER 128
#define MAX_GEOHANDLE 256
#define MAX_LIGHTS 64
#define MAX_SHADERS 128

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
#if !sINTRO
  sInt VertexSize;                // size of one vertex in bytes (intro: only 32 bytes available!)
#endif
  sU32 DiscardCount;              // if dynamic, this is the DISCARD timestamp

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
  struct IDirect3DVertexShader9 *VS;    // only one of these is set!
  struct IDirect3DPixelShader9 *PS;     // (or none)
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

#if !sINTRO

struct sSystemPrivate
{
  void InitX();
  sInt Msg(sU32 msg,sU32 wparam,sU32 lparam);
  virtual void Render()=0;

  virtual void InitScreens()=0;
  virtual void ExitScreens()=0;
//  virtual void ParseFVF(sU32 fvf,sU32 &dxfvf,sInt &floats)=0;
//  sDynBuffer *FindBuffer(sU32 fvf);
  virtual void CreateGeoBuffer(sInt i,sInt dyn,sInt vertex,sInt size)=0;

  sInt WDeviceLost;               // device is lost
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

  sInt WStartTime;
  sInt WScreenCount;
  sScreen Screen[MAX_SCREEN];
  sScreen *scr;
  sInt ScreenNr;
  sInt ViewportX;
  sInt ViewportY;

  sInt ConfigX;
  sInt ConfigY;
  sU32 ConfigFlags;

  sInt KeyIndex;
  sU32 KeyBuffer[MAX_KEYBUFFER];
  sU32 KeyQual;
  sU32 MouseButtons;
  sU32 MouseButtonsSave;
  sInt WMouseX;
  sInt WMouseY;

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

// direct input

  sBool InitDI();
  void ExitDI();
  void PollDI();
  void AddAKey(sU32 *Scans,sInt ascii);

// general 3d stuff

  sMatrix LastCamera;             // all this for GetTransform()
  sMatrix LastMatrix;
  sMatrix LastTransform;
  sMatrix LightMatrix;
  sBool RecalcTransform;
  sBool LightMatrixUsed;
  sGeoBuffer GeoBuffer[MAX_GEOBUFFER];
  sGeoHandle GeoHandle[MAX_GEOHANDLE];
  sU32 DiscardCount;
  sInt LightCount;
  sU32 LightMask[MAX_LIGHTS];
  sVector LightAmbient[MAX_LIGHTS];
  struct sD3DMaterial *DXMat;
  sInt DXMatChanged;

// DirectGraphics specific

  struct IDirect3D9 *DXD;
  struct IDirect3DDevice9 *DXDev;

  struct IDirect3DSurface9 *DXBlockSurface[2];

  struct IDirect3DTexture9 *DXStreamTexture;
  sInt DXStreamLevel;

  sInt ZBufXSize,ZBufYSize;
	sU32 ZBufFormat;
	struct IDirect3DSurface9 *ZBuffer;

#if sUSE_SHADERS
  sShader Shaders[MAX_SHADERS];
  sInt ShaderOn;                  // sTRUE when shader is enabled. for automatic shader disable
  sMatrix ShaderLastProj;         // last projection matrix calculated
#endif

// OpenGL specific

  sU8 *DynamicBuffer;
  sInt DynamicSize;
  sInt DynamicWrite;
  sInt DynamicRead;
};

#endif

/****************************************************************************/

#endif

