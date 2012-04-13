// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef __STARTINTRO_HPP__
#define __STARTINTRO_HPP__

#include "_startdx.hpp"

struct sSystem_
{
  
  // private interface

  void Run();
  void Render();
  void InitScreens();
  void ExitScreens();
  void Progress(sInt done);

//  void ParseFVF(sU32 fvf,sU32 &dxfvf,sInt &floats);
  void CreateGeoBuffer(sInt i,sInt dyn,sInt vertex);

  sScreen Screen;
  sD3DViewport DView;

  sInt NextTexture;
  sHardTex Textures[MAX_TEXTURE];
//  sMatrix StdTexTransMat;
//  sBool StdTexTransMatSet[MAX_TEXTURE];
  sInt LockedMode;
  sU32 LockedDXFVF;
  sInt LockedVertexSize;
  struct IDirect3DIndexBuffer9 *LockedIB;
  sInt LockedIBStart;
  sInt LockedIBCount;
  struct IDirect3DVertexBuffer9 *LockedVB;
  sInt LockedVBStart;
  sInt LockedVBCount;
  struct IDirect3DSurface9 *DXBlockSurface[2];

// DirectSound

  void InitDS();
  void ExitDS();
  void FillDS();

// general 3d stuff

  sMatrix LastUnit;    // dont change order of these four!
  sMatrix LastCamera;
  sMatrix LastTransform;
  sMatrix LastMatrix;

  sBool RecalcTransform;
  sGeoBuffer GeoBuffer[MAX_GEOBUFFER];
  sGeoHandle GeoHandle[MAX_GEOHANDLE];
  sInt GeoThisIndex;
  sInt GeoThisVertex;
  sU32 DiscardCount;
  sInt LightCount;
  sU32 LightMask[MAX_LIGHTS];
//  sVector LightAmbient[MAX_LIGHTS];

// DirectGraphics specific

  struct IDirect3DDevice9 *DXDev;
  struct IDirect3DSurface9 *ZBuffer;
  struct IDirect3DSurface9 *BackBuffer;

// public interface


// init/exit/debug

  void Log(sChar *);                                      // print out debug string
  sNORETURN void Abort(sChar *msg);                       // terminate now
  void Init(sU32 Flags,sInt xs=-1,sInt ys=-1);            // initialise system
  void Exit();                                            // terminate next frame
  sInt MemoryUsed();                                      // how much ram was allocated so far?
  sU32 GetCpuMask() { return 0; }                         // sCPU_???
  sInt GetGfxSystem() { return sSF_DIRECT3D; }            // sSF_DIRECT3D or sSF_OPENGL
  sU32 GetTime();

// sound

  sInt GetCurrentSample();                                // gets sample# played at last flip.

// font (only intro)

  sU32 *RenderFont(sInt bmx,sInt bmy,sChar *font,sChar *text,sInt x,sInt y,sInt width,sInt height);

// graphics

  sBool GetScreenInfo(sInt i,sScreenInfo &info);          // get screen size and availability
  sInt GetScreenCount();                                  // get number of screens
  sBool GetFullscreen();                                  // at least one screen is fullscreen
  void Reset(sU32 flags,sInt x,sInt y,sInt x2,sInt y2);   // set new screen config

  
  void BeginViewport(const sViewport &);        // begin viewport. does not set transform
  void EndViewport();                           // end viewport
  void SetCamera(const sCamera &);                    // set viewport and projection transform 
  void SetCameraOrtho();                        // set viewport for 1:1 pixel orthogonal
  void SetMatrix(sMatrix &mat);                 // set world transform (object position)
  void GetTransform(sInt mode,sMatrix &mat);    // get concatenated Camera and Matrix (for bilboards etc)

  sInt AddTexture(const sTexInfo &);                      // add a texture
  void RemTexture(sInt handle) {}
  void UpdateTexture(sInt handle,sBitmap *bm);            // update texture bitmap (must be same size)
  void SetTexture(sInt stage,sInt handle,sMatrix *mat=0); // select texture and texture transform matrix. matrix is adjusted for packed textures.
  void SetStates(sU32 *stream);                 // set state stream 
  void SetState(sU32 token,sU32 value);         // set single state

  sInt GeoAdd(sInt fvf,sInt prim);              // create a geometry handle for given fvf and sGEO_??? flags
  void GeoRem(sInt handle);                     // free handle and associated geometry
//  void GeoFlush();                              // flush all static vertex buffers
  void GeoFlush(sInt handle);                   // flush a specific dynamic vertex buffer
  sBool GeoDraw(sInt &handle);                  // try drawing. if result is true, drawing faled and geometry needs to be uploaded
  void GeoBegin(sInt handle,sInt vc,sInt ic,sF32 **fp,sU16 **ip); // begin upload, as requested by GeoDraw()
  void GeoEnd(sInt handle,sInt vc=-1,sInt ic=-1); // done uploading and draw finally. state how many indices/vertices were really needed.

  void StateBase(sU32 *&data,sU32 flags,sInt texmode,sU32 color);    // initialises for simple drawing
  void StateTex(sU32 *&data,sInt nr,sU32 flags);          // add a texture specific flags
  void StateEnd(sU32 *&data,sU32 *buffer,sInt size);      // appends end token and checks for buffer overflow
  void StateOptimise(sU32 *buffer) {}                       // throw out doubles, returns ptr to end token in case you want to ad more
  void SetMatLight(const sD3DMaterial &ml);
  void ClearLights()
  {
    EnableLights(0);
    LightCount = 0;
  }

  sInt AddLight(sD3DLight &,sU32 mask);         // add a lightsource, don't enable
  void EnableLights(sU32 mask);                 // enable all lights that match mask
  sInt GetLightCount() { return LightCount; }
};

#endif
