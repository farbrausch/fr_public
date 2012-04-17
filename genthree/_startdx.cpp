// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_start.hpp"
#include "_startdx.hpp"
#if sLINK_UTIL
#include "_util.hpp"
#endif

#define WINVER 0x500
#include <windows.h>
#include <d3d9.h>
#undef new
#if _DEBUG                          // fix memory management
#if !sINTRO
#define new new(__FILE__,__LINE__)
#endif
#endif

#if sDEBUG && !sINTRO
#define LOGGING 1                 // log all create and release calls
#else
#define LOGGING 0
#endif

#if sUSE_DIRECT3D

#define WINZERO(x) {sSetMem(&x,0,sizeof(x));}
#define WINSET(x) {sSetMem(&x,0,sizeof(x));x.dwSize = sizeof(x);}
#define RELEASE(x) {if(x)x->Release();x=0;}
#define DXERROR(hr) {if(FAILED(hr))sFatal("%s(%d) : directx error %08x (%d)",__FILE__,__LINE__,hr,hr&0x3fff);}

typedef IDirect3D9* (WINAPI * Direct3DCreate9T)(UINT SDKVersion);

extern Direct3DCreate9T Direct3DCreate9P;

sMAKEZONE(FlipLock,"FlipLock",0xffff0000);

/****************************************************************************/

struct FVFTableStruct
{
  sInt Size;                      // size in bytes!
  D3DVERTEXELEMENT9 *Info;        // dx declaration
  IDirect3DVertexDeclaration9 *Decl;    // declaration object
};

static D3DVERTEXELEMENT9 DeclDefault[] = 
{
  { 0, 0,D3DDECLTYPE_FLOAT3    ,0,D3DDECLUSAGE_POSITION  ,0 },
  { 0,12,D3DDECLTYPE_FLOAT3    ,0,D3DDECLUSAGE_NORMAL    ,0 },
  { 0,24,D3DDECLTYPE_FLOAT2    ,0,D3DDECLUSAGE_TEXCOORD  ,0 },
  D3DDECL_END()
};
static D3DVERTEXELEMENT9 DeclDouble[] = 
{
  { 0, 0,D3DDECLTYPE_FLOAT3    ,0,D3DDECLUSAGE_POSITION  ,0 },
  { 0,12,D3DDECLTYPE_D3DCOLOR  ,0,D3DDECLUSAGE_COLOR     ,0 },
  { 0,16,D3DDECLTYPE_FLOAT2    ,0,D3DDECLUSAGE_TEXCOORD  ,0 },
  { 0,24,D3DDECLTYPE_FLOAT2    ,0,D3DDECLUSAGE_TEXCOORD  ,1 },
  D3DDECL_END()
};
static D3DVERTEXELEMENT9 DeclTSpace[] = 
{
  { 0, 0,D3DDECLTYPE_FLOAT3    ,0,D3DDECLUSAGE_POSITION  ,0 },
  { 0,12,D3DDECLTYPE_DEC3N     ,0,D3DDECLUSAGE_NORMAL    ,0 },
  { 0,16,D3DDECLTYPE_DEC3N     ,0,D3DDECLUSAGE_TANGENT   ,0 },
  { 0,20,D3DDECLTYPE_DEC3N     ,0,D3DDECLUSAGE_BINORMAL  ,0 },
  { 0,24,D3DDECLTYPE_FLOAT2    ,0,D3DDECLUSAGE_TEXCOORD  ,0 },
  D3DDECL_END()
};
static D3DVERTEXELEMENT9 DeclCompact[] = 
{
  { 0, 0,D3DDECLTYPE_FLOAT3    ,0,D3DDECLUSAGE_POSITION  ,0 },
  { 0,12,D3DDECLTYPE_D3DCOLOR  ,0,D3DDECLUSAGE_COLOR     ,0 },
  D3DDECL_END()
};

static FVFTableStruct FVFTable[] = 
{
  {  0,0           },
  { 32,DeclDefault },
  { 32,DeclDouble  },
  { 32,DeclTSpace  },
  { 16,DeclCompact },
};

#define FVFMax (sizeof(FVFTable)/sizeof(FVFTableStruct))

/****************************************************************************/
/****************************************************************************/

void sSystemDX::InitScreens()
{
  HRESULT hr;
  D3DPRESENT_PARAMETERS d3dpp[8];
  RECT r;
  sInt nr,i;
  sU32 create;
  sU16 *iblock;
  sHardTex *tex;


  sVERIFY(WScreenCount<8);

  sREGZONE(FlipLock);

  if(!DXD)
  {
    DXD = (*Direct3DCreate9P)(D3D_SDK_VERSION);
    sVERIFY(DXD);
  }

	ZBufFormat=D3DFMT_D24S8;

  for(nr=0;nr<WScreenCount;nr++)
  {
// determine n

    Screen[nr].SFormat=D3DFMT_X8R8G8B8;
    Screen[nr].ZFormat=ZBufFormat;

    WINZERO(d3dpp[nr]);
    d3dpp[nr].BackBufferFormat = (enum _D3DFORMAT) Screen[nr].SFormat;
    d3dpp[nr].EnableAutoDepthStencil = sFALSE;
    d3dpp[nr].SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp[nr].BackBufferCount = 1;
    d3dpp[nr].PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    d3dpp[nr].Flags = 0;//D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
    d3dpp[nr].hDeviceWindow = (HWND) Screen[nr].Window;
    if(ConfigFlags & sSF_FULLSCREEN)
    {
      d3dpp[nr].Windowed = FALSE;
      d3dpp[nr].BackBufferWidth = ConfigX;
      d3dpp[nr].BackBufferHeight = ConfigY;
      Screen[nr].XSize = ConfigX;
      Screen[nr].YSize = ConfigY;
      WFullscreen = sTRUE;
      SetWindowLong((HWND)Screen[nr].Window,GWL_STYLE,WS_POPUP|WS_VISIBLE);
    }
    else
    {
      d3dpp[nr].Windowed = TRUE;
      GetClientRect((HWND) Screen[nr].Window,&r);
      Screen[nr].XSize = r.right-r.left;
      Screen[nr].YSize = r.bottom-r.top;
      WFullscreen = sFALSE;
      SetWindowLong((HWND)Screen[nr].Window,GWL_STYLE,WWindowedStyle);
    }
  }

  if(!DXDev)
  {
#if LOGGING
    sDPrintF("Create Device\n");
#endif

    create = D3DCREATE_HARDWARE_VERTEXPROCESSING;
    if(ConfigFlags & sSF_MULTISCREEN) create |= D3DCREATE_ADAPTERGROUP_DEVICE;
    hr = DXD->CreateDevice(0,D3DDEVTYPE_HAL,(HWND) Screen[0].Window,create,d3dpp,&DXDev);
    if(FAILED(hr))
    {
      create = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
      if(ConfigFlags & sSF_MULTISCREEN) create |= D3DCREATE_ADAPTERGROUP_DEVICE;
      hr = DXD->CreateDevice(0,D3DDEVTYPE_HAL,(HWND) Screen[0].Window,create,d3dpp,&DXDev);
      if(FAILED(hr))
        sFatal("could not create screen");
    }

    DXMat = new sD3DMaterial;

    for(i=1;i<FVFMax;i++)
      DXDev->CreateVertexDeclaration(FVFTable[i].Info,&FVFTable[i].Decl);
  }
  else 
  {
		if(ZBuffer)
		{
#if LOGGING
      sDPrintF("Release ZBuffer\n");
#endif
			ZBuffer->Release();
			ZBuffer = 0;
		}
    for(i=0;i<MAX_GEOHANDLE;i++)
    {
      if(GeoHandle[i].VertexBuffer>=3)
        GeoBuffer[GeoHandle[i].VertexBuffer].Free();
      if(GeoHandle[i].IndexBuffer>=3)
        GeoBuffer[GeoHandle[i].IndexBuffer].Free();
      GeoHandle[i].VertexCount = 0;
      GeoHandle[i].VertexBuffer = 0;
      GeoHandle[i].IndexCount = 0;
      GeoHandle[i].IndexBuffer = 0;
    }
    for(i=0;i<MAX_GEOBUFFER;i++)
    {
      if(i>=3)
        sVERIFY(GeoBuffer[i].UserCount==0);
      if(GeoBuffer[i].VB)
      {
        GeoBuffer[i].VB->Release();   // VB & IB are the same
#if LOGGING
        sDPrintF("Release vertex/indexbuffer %d bytes\n",GeoBuffer[i].Size);
#endif
      }
      GeoBuffer[i].Init();
    }
		for(i=0;i<MAX_TEXTURE;i++)
		{
			if(Textures[i].Flags & sTIF_RENDERTARGET)
			{
#if LOGGING
        sDPrintF("Release Rendertarget\n");
#endif
				Textures[i].Tex->Release();
				Textures[i].Tex=0;
			}
		}
    if(DXBlockSurface[0])    
      DXBlockSurface[0]->Release();
    if(DXBlockSurface[1])
      DXBlockSurface[1]->Release();
    DXBlockSurface[0]=0;
    DXBlockSurface[1]=0;


    hr = DXDev->Reset(d3dpp);

    if(FAILED(hr))
    {
      WDeviceLost = 1;
      return;
    }
  }

// other buffers

  DXERROR(DXDev->CreateOffscreenPlainSurface(16,16,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&DXBlockSurface[0],0));
  DXERROR(DXDev->CreateOffscreenPlainSurface(16,16,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&DXBlockSurface[1],0));

// init buffer management

  CreateGeoBuffer(0,1,1,MAX_DYNVBSIZE);
  CreateGeoBuffer(1,1,0,MAX_DYNIBSIZE);
  CreateGeoBuffer(2,0,0,2*0x8000/4*6);
  DXERROR(GeoBuffer[2].IB->Lock(0,2*0x8000/4*6,(void **) &iblock,0));
  for(i=0;i<0x8000/6;i++)
    sQuad(iblock,i*4+0,i*4+1,i*4+2,i*4+3);
  DXERROR(GeoBuffer[2].IB->Unlock());

// set some defaults

  DXDev->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
  DXDev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
  DXDev->SetRenderState(D3DRS_LIGHTING,0);
  DXDev->SetRenderState(D3DRS_AMBIENT,0xff000000);
  DXDev->SetRenderState(D3DRS_COLORVERTEX,1);
  DXDev->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE,D3DMCS_MATERIAL);
  DXDev->SetRenderState(D3DRS_SPECULARMATERIALSOURCE,D3DMCS_MATERIAL);
  DXDev->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE,D3DMCS_MATERIAL);
  DXDev->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE,D3DMCS_MATERIAL);
  DXMat->Diffuse.x = 1.0f;
  DXMat->Diffuse.y = 1.0f;
  DXMat->Diffuse.z = 1.0f;
  DXMat->Diffuse.w = 1.0f;
  DXMat->Ambient.x = 1.0f;
  DXMat->Ambient.y = 1.0f;
  DXMat->Ambient.z = 1.0f;
  DXMat->Ambient.w = 1.0f;
  DXMat->Specular.x = 1.0f;
  DXMat->Specular.y = 1.0f;
  DXMat->Specular.z = 1.0f;
  DXMat->Specular.w = 1.0f;
  DXMat->Emissive.x = 0.0f;
  DXMat->Emissive.y = 0.0f;
  DXMat->Emissive.z = 0.0f;
  DXMat->Emissive.w = 0.0f;
  DXMat->Power = 8.0f;
  DXMatChanged = 1;

  LightCount = 0;
  WDeviceLost = 0;
  if(WFullscreen)
  {
    GetWindowRect((HWND)Screen[0].Window,&r);
    ClipCursor(&r);
    ShowCursor(0);
  }
  else
  {
    ClipCursor(0);
    ShowCursor(1);
  }

	ReCreateZBuffer();
	for(i=0;i<MAX_TEXTURE;i++)
	{
		if(Textures[i].Flags & sTIF_RENDERTARGET)
    {
      tex = &Textures[i];
	    DXERROR(DXDev->CreateTexture(tex->XSize,tex->YSize,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&tex->Tex,0));
    }
	}
}

/****************************************************************************/

void sSystemDX::ExitScreens()
{
  sInt i;

  for(i=0;i<MAX_TEXTURE;i++)
  {
    if(Textures[i].Tex)
    {
      Textures[i].Tex->Release();
      Textures[i].Tex=0;
    }
  }
	if(ZBuffer)
	{
		ZBuffer->Release();
		ZBuffer = 0;
	}
  if(DXBlockSurface[0])    
    DXBlockSurface[0]->Release();
  if(DXBlockSurface[1])
    DXBlockSurface[1]->Release();
  DXBlockSurface[0]=0;
  DXBlockSurface[1]=0;
  for(i=1;i<MAX_GEOHANDLE;i++)
    sVERIFY(GeoHandle[i].Mode==0);
  for(i=0;i<MAX_GEOBUFFER;i++)
  {
    if(i>=3)
      sVERIFY(GeoBuffer[i].UserCount==0);
    if(GeoBuffer[i].VB)
      GeoBuffer[i].VB->Release();   // VB & IB are the same
    GeoBuffer[i].Init();
  }
#if sUSE_SHADERS
  for(i=0;i<MAX_SHADERS;i++)
  {
    sVERIFY(Shaders[i].VS==0);
    sVERIFY(Shaders[i].PS==0);
  }
#endif
  for(i=1;i<FVFMax;i++)
    if(FVFTable[i].Decl)
      FVFTable[i].Decl->Release();
  DXDev->Release();
  DXDev = 0;
  DXD->Release();
  DXD = 0;
  if(DXMat)
    delete DXMat;
  DXMat = 0;
}

/****************************************************************************/

void sSystemDX::Render()
{
  static sInt LastTime=-1;
  sInt time,ticks;
  HRESULT hr;
  sInt i;


  RecalcTransform = sTRUE;
  LastCamera.Init();
  LastMatrix.Init();
  LastTransform.Init();
  StdTexTransMat.i.Init(0.5f,0.0f,0.0f,0.0f);
  StdTexTransMat.j.Init(0.0f,0.5f,0.0f,0.0f);
  StdTexTransMat.k.Init(0.0f,0.0f,0.5f,0.0f);
  StdTexTransMat.l.Init(0.5f,0.5f,0.5f,1.0f);
  StdTexTransMatSet[0] = 0;
  for(i=0;i<8;i++)
    StdTexTransMatSet[i] = 0;

  if(WDeviceLost)
  {
    Sleep(100);
    hr = DXDev->TestCooperativeLevel();
    if(hr!=D3DERR_DEVICENOTRESET)
      return;
    InitScreens();
    if(WDeviceLost)
      return;
  }

  time = sSystem->GetTime();
  ticks = 0;
  if(LastTime==-1)
    LastTime = time;
  while(time>LastTime)
  {
    LastTime+=10;
    ticks++;
  }
  if(ticks>10)
    ticks=10;

  sAppHandler(sAPPCODE_FRAME,0);
  sAppHandler(sAPPCODE_TICK,ticks);
  sAppHandler(sAPPCODE_PAINT,0);

#if !sINTRO
  {
    static sInt dblock;
    D3DLOCKED_RECT lr;
    volatile sInt dummy;

    sZONE(FlipLock);

    DXERROR(DXDev->ColorFill(DXBlockSurface[dblock],0,0xff002050));

    if(!WResponse)
      dblock = 1-dblock;    // in turbo mode, sacrifice one frame of responsiveness

    DXERROR(DXBlockSurface[dblock]->LockRect(&lr,0,D3DLOCK_READONLY));
    dummy = *(sInt *)lr.pBits;
    DXERROR(DXBlockSurface[dblock]->UnlockRect());
  }
#endif
  hr = DXDev->Present(0,0,0,0);
  if(hr==D3DERR_DEVICELOST)
    WDeviceLost = 1;

  for(i=1;i<MAX_GEOHANDLE;i++)
  {
    GeoHandle[i].DiscardCount = 0;
  }
  DiscardCount = 1;
  GeoBuffer[0].Used = GeoBuffer[0].Size;
  GeoBuffer[1].Used = GeoBuffer[1].Size;
}

/****************************************************************************/
/****************************************************************************/

void sSystemDX::BeginViewport(const sViewport &vp)
{
  D3DVIEWPORT9 dview,fview;
  sInt nr;
  sInt xs,ys,dxs,dys;
	sHardTex *tex;
  IDirect3DSurface9 *backbuffer;
  IDirect3DSwapChain9 *swapchain;

  nr = vp.Screen;
	if(nr!=-1)
	{
		ScreenNr = nr;
		scr = &Screen[nr];
		sVERIFY(nr>=0 && nr<WScreenCount);

		dxs = Screen[nr].XSize;
		dys = Screen[nr].YSize;
	}
	else
	{
		sVERIFY(vp.RenderTarget >= 0);
		tex = &Textures[vp.RenderTarget];
		sVERIFY(tex->Flags & sTIF_RENDERTARGET);

		dxs = tex->XSize;
		dys = tex->YSize;
	}

	xs = vp.Window.XSize();
	ys = vp.Window.YSize();
	if(xs==0 || ys==0)
	{
		dview.X = 0;
		dview.Y = 0;
		dview.Width = dxs;
		dview.Height = dys;
	}
	else
	{
		dview.X = vp.Window.x0;
		dview.Y = vp.Window.y0;
		dview.Width = vp.Window.x1-vp.Window.x0;
		dview.Height = vp.Window.y1-vp.Window.y0;
	}
	ViewportX = dview.Width;
	ViewportY = dview.Height;
	dview.MinZ = 0.0f;
	dview.MaxZ = 1.0f;

	if(nr==-1)
	{
		DXERROR(tex->Tex->GetSurfaceLevel(0,&backbuffer));
	}
	else
	{
		DXERROR(DXDev->GetSwapChain(nr,&swapchain));
		DXERROR(swapchain->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&backbuffer));
		swapchain->Release();
	}

	DXERROR(DXDev->SetRenderTarget(0,backbuffer));
	backbuffer->Release();

	if(nr!=-1)
		DXDev->SetViewport(&dview);
	else
	{
		fview.X = 0; // always clear full surface for textures
		fview.Y = 0;
		fview.Width = tex->XSize;
		fview.Height = tex->YSize;
		fview.MinZ = 0.0f;
		fview.MaxZ = 1.0f;
		DXDev->SetViewport(&fview);
	}

  switch(vp.ClearFlags)
  {
  case sVCF_COLOR:
    DXDev->Clear(0,0,D3DCLEAR_TARGET,vp.ClearColor.Color,1.0f,0);
    break;
  case sVCF_Z:
    DXDev->Clear(0,0,D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,vp.ClearColor.Color,1.0f,0);
    break;
  case sVCF_ALL:
    DXDev->Clear(0,0,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,vp.ClearColor.Color,1.0f,0);
    break;
  }

	if(nr==-1)
		DXDev->SetViewport(&dview);

  DXERROR(DXDev->BeginScene());
}

void sSystemDX::EndViewport()
{
  DXERROR(DXDev->EndScene());
  ClearLights();
}

/****************************************************************************/

void sSystemDX::SetCamera(sCamera &cam)
{
  sF32 ratio,q;
  sMatrix mat;

  ratio = cam.AspectX/cam.AspectY;
  q = cam.FarClip/(cam.FarClip-cam.NearClip);
  mat = cam.CamPos;
  mat.TransR();
  LastCamera = mat;
  RecalcTransform = sTRUE;
  DXDev->SetTransform(D3DTS_VIEW,(D3DMATRIX*)&mat);

  mat.i.Init(cam.ZoomX    ,0              , 0             ,0);
  mat.j.Init(0            ,cam.ZoomY*ratio, 0             ,0);
  mat.k.Init(cam.CenterX  ,cam.CenterY    , q             ,1);
  mat.l.Init(0            ,0              ,-q*cam.NearClip,0);
  DXDev->SetTransform(D3DTS_PROJECTION,(D3DMATRIX*)&mat);
}

void sSystemDX::SetCameraOrtho()
{
  sMatrix mat;

  mat.Init();
  LastCamera = mat;
  LastMatrix = mat;
  DXDev->SetTransform(D3DTS_VIEW,(D3DMATRIX*)&mat);
  DXDev->SetTransform(D3DTS_WORLD,(D3DMATRIX *) &mat);

  mat.i.Init(2.0f/ViewportX,0                ,0 ,0);
  mat.j.Init(0             ,-2.0f/ViewportY  ,0 ,0);
  mat.k.Init(0             ,0                ,1 ,0);
  mat.l.Init(-1            ,1                ,0 ,1);
  DXDev->SetTransform(D3DTS_PROJECTION,(D3DMATRIX*)&mat);
}

void sSystemDX::SetMatrix(sMatrix &mat)
{
  LastMatrix = mat;
  RecalcTransform = sTRUE;
  DXDev->SetTransform(D3DTS_WORLD,(D3DMATRIX *) &mat);
}

void sSystemDX::GetTransform(sInt mode,sMatrix &mat)
{
  switch(mode)
  {
  case sGT_UNIT:
    mat.Init();
    break;
  case sGT_MODELVIEW:
    if(RecalcTransform)
    {
      LastTransform.Mul4(LastMatrix,LastCamera);
      RecalcTransform = sFALSE;
    }
    mat = LastTransform;
    break;

  case sGT_MODEL:
    mat = LastMatrix;
    break;

  case sGT_VIEW:
    mat = LastCamera;
    break;

  default:
    mat.Init();
    break;
  }
}


/****************************************************************************/
/****************************************************************************/

void sSystemDX::SetTexture(sInt stage,sInt handle,sMatrix *mat)
{
  sHardTex *tex;

  if(handle==sINVALID)
  {
    DXDev->SetTexture(stage,0);
    return;
  }

  tex = &Textures[handle];
  DXDev->SetTexture(stage,tex->Tex);
  if(mat)
  {
    DXDev->SetTransform((D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0+stage),(D3DMATRIX *) mat);
    StdTexTransMatSet[stage] = 0;
  }
  else
  {
    if(!StdTexTransMatSet[stage])
    {
      DXDev->SetTransform((D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0+stage),(D3DMATRIX *) &StdTexTransMat);
      StdTexTransMatSet[stage] = 1;
    }
  }
}

/****************************************************************************/

sInt sSystemDX::AddTexture(const sTexInfo &ti)
{
  sInt i;
  sHardTex *tex;

  tex = Textures;
  for(i=0;i<MAX_TEXTURE;i++)
  {
    if(tex->Flags==0)
    {
      tex->XSize = ti.XSize;
      tex->YSize = ti.YSize;
      tex->Flags = ti.Flags|sTIF_ALLOCATED;
      tex->Format = sTF_A8R8G8B8;
      tex->Tex = 0;
      tex->TexGL = 0;

	    if(tex->Flags & sTIF_RENDERTARGET)
	    {
        if(tex->XSize==0)
        {
          tex->XSize = Screen[0].XSize;
          tex->YSize = Screen[0].YSize;
        }
//        sVERIFY(tex->XSize<=Screen[0].XSize);
//        sVERIFY(tex->YSize<=Screen[0].YSize);
        #if LOGGING
        sDPrintF("Create Rendertarget %dx%d\n",tex->XSize,tex->YSize);
        #endif
		    DXERROR(DXDev->CreateTexture(tex->XSize,tex->YSize,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&tex->Tex,0));
        ReCreateZBuffer();
	    }
	    else
	    {
        #if LOGGING
        sDPrintF("Create Texture %dx%d\n",tex->XSize,tex->YSize);
        #endif
		    DXERROR(DXDev->CreateTexture(tex->XSize,tex->YSize,0,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&tex->Tex,0));
        UpdateTexture(i,ti.Bitmap);
	    }

      return i;
    }
    tex++;
  }

  return sINVALID;
}

sInt sSystemDX::AddTexture(sInt xs,sInt ys,sInt format,sU16 *data)
{
  sInt i;
  sHardTex *tex;
  static D3DFORMAT formats[] = 
  {
    D3DFMT_UNKNOWN,
    D3DFMT_A8R8G8B8,
    D3DFMT_A8,
    D3DFMT_R16F,
    D3DFMT_A2R10G10B10,
    D3DFMT_Q8W8V8U8,
    D3DFMT_A2W10V10U10,
  };

  sVERIFY(format>0 && format<sTF_MAX);

  tex = Textures;
  for(i=0;i<MAX_TEXTURE;i++)
  {
    if(tex->Flags==0)
    {
      tex->XSize = xs;
      tex->YSize = ys;
      tex->Flags = sTIF_ALLOCATED;
      tex->Format = format;
      tex->Tex = 0;
      tex->TexGL = 0;

	    if(data==0)
	    {
        if(tex->XSize==0)
        {
          tex->XSize = Screen[0].XSize;
          tex->YSize = Screen[0].YSize;
        }
        tex->Flags |= sTIF_RENDERTARGET;
//        sVERIFY(tex->XSize<=Screen[0].XSize);
//        sVERIFY(tex->YSize<=Screen[0].YSize);
        #if LOGGING
        sDPrintF("Create Rendertarget %dx%d\n",tex->XSize,tex->YSize);
        #endif
		    DXERROR(DXDev->CreateTexture(tex->XSize,tex->YSize,1,D3DUSAGE_RENDERTARGET,formats[format],D3DPOOL_DEFAULT,&tex->Tex,0));
        ReCreateZBuffer();
	    }
	    else
	    {
        #if LOGGING
        sDPrintF("Create Texture %dx%d\n",tex->XSize,tex->YSize);
        #endif
		    DXERROR(DXDev->CreateTexture(tex->XSize,tex->YSize,0,0,formats[format],D3DPOOL_MANAGED,&tex->Tex,0));
        UpdateTexture(i,data);
	    }

      return i;
    }
    tex++;
  }
  return sINVALID;
}

/****************************************************************************/

void sSystemDX::RemTexture(sInt handle)
{
  sHardTex *tex;

  sVERIFY(handle>=0 && handle<MAX_TEXTURE);
  tex = &Textures[handle];
  if(tex->Tex)
    tex->Tex->Release();

  tex->Flags = 0;
  tex->Tex = 0;
}

/****************************************************************************/

void sSystemDX::ReCreateZBuffer()
{
	sInt i,xs,ys;
	
	// determine required size
	xs=ys=0;
	for(i=0;i<WScreenCount;i++)
	{
		xs=sMax<sInt>(xs,Screen[i].XSize);
		ys=sMax<sInt>(ys,Screen[i].YSize);
	}

	for(i=0;i<MAX_TEXTURE;i++)
	{
		if(Textures[i].Flags & sTIF_RENDERTARGET)
		{
			xs=sMax<sInt>(xs,Textures[i].XSize);
			ys=sMax<sInt>(ys,Textures[i].YSize);
		}
	}

	// create the zbuffer
	if(xs!=ZBufXSize || ys!=ZBufYSize || !ZBuffer)
	{
		DXERROR(DXDev->SetDepthStencilSurface(0));

		if(ZBuffer)
		{
			ZBuffer->Release();
			ZBuffer=0;
		}

    #if LOGGING
    sDPrintF("Create ZBuffer %dx%d\n",xs,ys);
    #endif
		DXERROR(DXDev->CreateDepthStencilSurface(xs,ys,(D3DFORMAT) ZBufFormat,D3DMULTISAMPLE_NONE,0,TRUE,&ZBuffer,0));
		ZBufXSize=xs;
		ZBufYSize=ys;

		DXERROR(DXDev->SetDepthStencilSurface(ZBuffer));
	}
}

/****************************************************************************/

void sSystemDX::StreamTextureStart(sInt handle,sInt level,sBitmapLock &lock)
{
  sHardTex *tex;
  IDirect3DTexture9 *mst;
  D3DLOCKED_RECT dxlock;

  tex = &Textures[handle];
  mst = tex->Tex;;

  sVERIFY(!DXStreamTexture);
  DXStreamTexture = mst;
  DXStreamLevel = level;

  DXERROR(mst->LockRect(level,&dxlock,0,0));

  lock.Data = (sU8 *)dxlock.pBits;
  lock.XSize = tex->XSize>>level;
  lock.YSize = tex->YSize>>level;
  lock.Kind = tex->Format;
  lock.BPR = dxlock.Pitch;
}

/****************************************************************************/

void sSystemDX::StreamTextureEnd()
{
  sVERIFY(DXStreamTexture);
  DXStreamTexture->UnlockRect(DXStreamLevel);
  DXStreamTexture=0;
}

/****************************************************************************/
/****************************************************************************/

void sSystemDX::SetStates(sU32 *stream)
{
#if sUSE_SHADERS
  ShaderClear();
#endif
  while(*stream!=0xffffffff)
  {
    SetState(stream[0],stream[1]);
    stream+=2;
  }
  if(DXMatChanged)
  {
    DXDev->SetMaterial((D3DMATERIAL9 *)DXMat);
    DXMatChanged = 0;
  }
}

/****************************************************************************/

void sSystemDX::SetState(sU32 token,sU32 value)
{
  if(token<0x0310)
  {
    if(token<0x0100)
    {
      DXDev->SetRenderState((enum _D3DRENDERSTATETYPE)token,value);
    }
    else if(token<0x0200)
    {
      DXDev->SetTextureStageState(((token>>5)&7),(enum _D3DTEXTURESTAGESTATETYPE)(token&31),value);
    }
    else if(token<0x0300)
    {
      DXDev->SetSamplerState(((token>>4)&15),(enum _D3DSAMPLERSTATETYPE)(token&15),value);
    }
    else if(token<0x0310)
    {
      DXDev->SetSamplerState(D3DDMAPSAMPLER,(enum _D3DSAMPLERSTATETYPE)(token&15),value);
    }
  }
  else
  {
    switch(token)
    {
    case sD3DMAT_DIFFUSE:
    case sD3DMAT_AMBIENT:
    case sD3DMAT_SPECULAR:
    case sD3DMAT_EMISSIVE:
      (&DXMat->Diffuse)[token&3].Init(
        ((value>>16)&0xff)/255.0f,
        ((value>> 8)&0xff)/255.0f,
        ((value    )&0xff)/255.0f,
        ((value>>24)&0xff)/255.0f);
      DXMatChanged = 1;
      break;
    case sD3DMAT_POWER:
      DXMat->Power = *(sF32 *)&value;
      DXMatChanged = 1;
      break;
    default:
      sFatal("illegal DirectX token %08x=%08x",token,value);
      break;
    }
  }
}

/****************************************************************************/
/****************************************************************************/

void sSystemDX::CreateGeoBuffer(sInt i,sInt dyn,sInt vertex,sInt size)
{
  sInt usage;

  sVERIFY(dyn==0 || dyn==1);
  sVERIFY(vertex==0 || vertex==1);

  GeoBuffer[i].Type = 1+vertex;
  GeoBuffer[i].Size = size;
  GeoBuffer[i].Used = 0;
  GeoBuffer[i].UserCount = 0;
  GeoBuffer[i].VB = 0;

  usage = D3DUSAGE_WRITEONLY;
  if(dyn)
    usage |= D3DUSAGE_DYNAMIC;

  if(vertex)
  {
    DXERROR(DXDev->CreateVertexBuffer(size,usage,0,D3DPOOL_DEFAULT,&GeoBuffer[i].VB,0));
  }
  else
  {
    DXERROR(DXDev->CreateIndexBuffer(size,usage,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&GeoBuffer[i].IB,0));  
  }
#if LOGGING
  sDPrintF("Create %s %s-Buffer (%d bytes)\n",dyn?"dynamic":"static",vertex?"vertex":"index",size);
#endif
}

sInt sSystemDX::GeoAdd(sInt fvf,sInt prim)
{
  sInt i;
  sGeoHandle *gh;

  sVERIFY(fvf>=1 && fvf<FVFMax);
  for(i=1;i<MAX_GEOHANDLE;i++)
  {
    gh = &GeoHandle[i];
    if(gh->Mode==0)
    {
      sSetMem(gh,0,sizeof(*gh));
      gh->VertexSize = FVFTable[fvf].Size;
      gh->FVF = fvf;
      gh->Mode = prim;
      return i;
    }
  }
  sFatal("GeoAdd() ran out of handles");
	return 0;
}

void sSystemDX::GeoRem(sInt handle)
{
  sVERIFY(handle>=1 && handle<MAX_GEOHANDLE);
  sVERIFY(GeoHandle[handle].Mode!=0);

  if(GeoHandle[handle].VertexBuffer>=3)
    GeoBuffer[GeoHandle[handle].VertexBuffer].Free();
  if(GeoHandle[handle].IndexBuffer>=3)
    GeoBuffer[GeoHandle[handle].IndexBuffer].Free();
  GeoHandle[handle].Mode = 0;
}

void sSystemDX::GeoFlush()
{
  sInt i;

  for(i=1;i<MAX_GEOHANDLE;i++)
    GeoHandle[i].VertexCount = 0;
  for(i=3;i<MAX_GEOBUFFER;i++)
  {
    if(GeoBuffer[i].VB)
      GeoBuffer[i].VB->Release();
    GeoBuffer[i].Init();
  }
}

void sSystemDX::GeoFlush(sInt handle)
{
  sVERIFY(handle>=1 && handle<MAX_GEOHANDLE);
  GeoHandle[handle].DiscardCount = 0;
}

sBool sSystemDX::GeoDraw(sInt &handle)
{
  sGeoHandle *gh;
  enum _D3DPRIMITIVETYPE mode;
  sInt count;

// buffer correctly loaded?

  sVERIFY(handle>=1 && handle<MAX_GEOHANDLE);
  sVERIFY(GeoHandle[handle].Mode!=0);

  gh = &GeoHandle[handle];

  if(gh->VertexCount==0)
    return sTRUE;
  if(gh->VertexBuffer==0 && gh->DiscardCount!=DiscardCount)
    return sTRUE;

// set up buffers

  DXDev->SetVertexDeclaration(FVFTable[gh->FVF].Decl);
  if(gh->IndexCount)
  {
    sVERIFY(gh->IndexBuffer>=0 && gh->IndexBuffer<MAX_GEOBUFFER);
    sVERIFY(GeoBuffer[gh->IndexBuffer].Type == 1);
    sVERIFY(GeoBuffer[gh->IndexBuffer].IB);
    DXDev->SetIndices(GeoBuffer[gh->IndexBuffer].IB);
  }
  sVERIFY(gh->VertexBuffer>=0 && gh->VertexBuffer<MAX_GEOBUFFER);
  sVERIFY(GeoBuffer[gh->VertexBuffer].Type == 2);
  sVERIFY(GeoBuffer[gh->VertexBuffer].VB);
  DXDev->SetStreamSource(0,GeoBuffer[gh->VertexBuffer].VB,0,gh->VertexSize);

// start drawing

  count = gh->IndexCount;
  if(count==0)
    count = gh->VertexCount;

  switch(gh->Mode&7)
  {
  case sGEO_POINT:
    mode = D3DPT_POINTLIST;
    count = count;
    sVERIFY(gh->IndexCount==0);
    break;
  case sGEO_LINE:
    mode = D3DPT_LINELIST;
    count = count/2;
    break;
  case sGEO_LINESTRIP:
    mode = D3DPT_LINESTRIP;
    count = count-1;
    break;
  case sGEO_TRI:
    mode = D3DPT_TRIANGLELIST;
    count = count/3;
    break;
  case sGEO_TRISTRIP:
    mode = D3DPT_TRIANGLESTRIP;
    count = count-2;
    break;
  case sGEO_QUAD:
    sVERIFY(gh->IndexCount==0);
    DXDev->SetIndices(GeoBuffer[2].IB);
    DXDev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,gh->VertexStart,0,gh->VertexCount,0,count/4*2);
    return sFALSE;
  default:
    sVERIFYFALSE;
  }
  if(gh->IndexCount)
    DXDev->DrawIndexedPrimitive(mode,gh->VertexStart,0,gh->VertexCount,gh->IndexStart,count);
  else
    DXDev->DrawPrimitive(mode,gh->VertexStart,count);


//  if(gh->VertexBuffer==0)
//    sDPrintF("draw vb %06x@%06x ib %06x@%06x\n",gh->VertexStart,gh->VertexCount,gh->IndexStart,count);

  return sFALSE;
}

void sSystemDX::GeoBegin(sInt handle,sInt vc,sInt ic,sF32 **fp,sU16 **ip)
{
  sGeoHandle *gh;
  sInt i;
  sInt ok;

  sVERIFY(handle>=1 && handle<MAX_GEOHANDLE);
  sVERIFY(GeoHandle[handle].Mode!=0);
  sVERIFY(vc*GeoHandle[handle].VertexSize<=MAX_DYNVBSIZE); 
  sVERIFY(ic*2<=MAX_DYNIBSIZE);

  gh = &GeoHandle[handle];

  if(gh->VertexBuffer>=3)
    GeoBuffer[gh->VertexBuffer].Free();
  if(gh->IndexBuffer>=3)
    GeoBuffer[gh->IndexBuffer].Free();
  gh->VertexBuffer = 0;
  gh->VertexStart = 0;
  gh->VertexCount = vc;
  gh->IndexBuffer = 0;
  gh->IndexStart = 0;
  gh->IndexCount = ic;
  *fp = 0;
  if(ip)
    *ip = 0;

  if(gh->Mode&sGEO_STATIC)
  {
    for(i=3;i<MAX_GEOBUFFER;i++)
      if(GeoBuffer[i].AllocVB(vc,gh->VertexSize,gh->VertexStart))
        goto foundvb;

    for(i=3;i<MAX_GEOBUFFER;i++)
    {
      if(GeoBuffer[i].Type==0)
      {
        CreateGeoBuffer(i,0,1,MAX_DYNVBSIZE);
        ok = GeoBuffer[i].AllocVB(vc,gh->VertexSize,gh->VertexStart);
        sVERIFY(ok);
        goto foundvb;
      }
    }
    sFatal("GeoBegin(): new free static vertex buffer");
foundvb:
    gh->VertexBuffer = i;

    if(ic)
    {
      for(i=3;i<MAX_GEOBUFFER;i++)
        if(GeoBuffer[i].AllocIB(ic,gh->IndexStart))
          goto foundib;

      for(i=3;i<MAX_GEOBUFFER;i++)
      {
        if(GeoBuffer[i].Type==0)
        {
          CreateGeoBuffer(i,0,0,MAX_DYNVBSIZE);
          ok = GeoBuffer[i].AllocIB(ic,gh->IndexStart);
          sVERIFY(ok);
          goto foundib;
        }
      }
      sFatal("GeoBegin(): new free static index buffer");
foundib:
      gh->IndexBuffer = i;
    }

    GeoBuffer[gh->VertexBuffer].VB->Lock(gh->VertexStart*gh->VertexSize,vc*gh->VertexSize,(void **)fp,0);
    if(ic)
      GeoBuffer[gh->IndexBuffer].IB->Lock(gh->IndexStart*2,ic*2,(void **)ip,0);
//    sDPrintF("lock %d %d\n",gh->VertexBuffer,gh->IndexBuffer);
  }
  else
  {
    ok = GeoBuffer[0].AllocVB(vc,gh->VertexSize,gh->VertexStart);
    gh->VertexBuffer = 0;
    if(ic)
    {
      gh->IndexBuffer = 1;
      ok &= GeoBuffer[1].AllocIB(ic,gh->IndexStart);
    }

    if(!ok)
    {
      GeoBuffer[0].Used = 0;
      GeoBuffer[1].Used = 0;
      DiscardCount++;

      ok  = GeoBuffer[0].AllocVB(vc,gh->VertexSize,gh->VertexStart);
      if(ic)
        ok &= GeoBuffer[1].AllocIB(ic,gh->IndexStart);
      sVERIFY(ok);
    }
    gh->DiscardCount = DiscardCount;

    GeoBuffer[gh->VertexBuffer].VB->Lock(gh->VertexStart*gh->VertexSize,vc*gh->VertexSize,(void **)fp,gh->VertexStart?D3DLOCK_NOOVERWRITE:D3DLOCK_DISCARD);
    if(ic)
      GeoBuffer[gh->IndexBuffer].IB->Lock(gh->IndexStart*2,ic*2,(void **)ip,gh->IndexStart?D3DLOCK_NOOVERWRITE:D3DLOCK_DISCARD);
//    sDPrintF("lck! %d %d\n",gh->VertexBuffer,gh->IndexBuffer);
  }
}

void sSystemDX::GeoEnd(sInt handle,sInt vc,sInt ic)
{
  sGeoHandle *gh;
//  sBool load;

  sVERIFY(handle>=1 && handle<MAX_GEOHANDLE);
  sVERIFY(GeoHandle[handle].Mode!=0);

  gh = &GeoHandle[handle];

  GeoBuffer[gh->VertexBuffer].VB->Unlock();
  if(gh->IndexCount)
    GeoBuffer[gh->IndexBuffer].IB->Unlock();

  if(vc!=-1)
    gh->VertexCount = vc;
  if(ic!=-1)
    gh->IndexCount = ic;
  
//  sDPrintF("unlk %d %d\n",gh->VertexBuffer,gh->IndexBuffer);
/*
  if(vc!=0)
  {
    load = GeoDraw(handle);
    sVERIFY(!load);
  }
  */
}

/****************************************************************************/
/****************************************************************************/

void sSystemDX::StateBase(sU32 *&data,sU32 flags,sInt texmode,sU32 color)
{
  *data++ = sD3DRS_ALPHATESTENABLE;         *data++ = 0;
  *data++ = sD3DRS_ALPHAFUNC;               *data++ = sD3DCMP_GREATER;
  *data++ = sD3DRS_ALPHAREF;                *data++ = 4;
  *data++ = sD3DRS_STENCILENABLE;           *data++ = 0;

  *data++ = sD3DMAT_DIFFUSE;                *data++ = 0xffffffff;
  *data++ = sD3DMAT_AMBIENT;                *data++ = 0xffffffff;
  *data++ = sD3DMAT_SPECULAR;               *data++ = 0xffffffff;
  *data++ = sD3DMAT_EMISSIVE;               *data++ = 0x00000000;
  *data++ = sD3DMAT_POWER;                  *(sF32 *)data++ = 1.0f;
  *data++ = sD3DRS_DIFFUSEMATERIALSOURCE;   *data++ = sD3DMCS_COLOR1;
  *data++ = sD3DRS_AMBIENTMATERIALSOURCE;   *data++ = sD3DMCS_MATERIAL;
  *data++ = sD3DRS_SPECULARMATERIALSOURCE;  *data++ = sD3DMCS_MATERIAL;
  *data++ = sD3DRS_EMISSIVEMATERIALSOURCE;  *data++ = sD3DMCS_MATERIAL;
  *data++ = sD3DRS_SPECULARENABLE;          *data++ = 0;

  *data++ = sD3DRS_ZENABLE;
  *data++ = sD3DZB_TRUE;

  *data++ = sD3DRS_ZWRITEENABLE;
  *data++ = (flags & sMBF_ZWRITE) ? 1 : 0;

  *data++ = sD3DRS_ZFUNC;
  *data++ = (flags & sMBF_ZREAD) ? sD3DCMP_LESSEQUAL : sD3DCMP_ALWAYS;

  *data++ = sD3DRS_CULLMODE;
  *data++ = (flags & sMBF_DOUBLESIDED) ? sD3DCULL_NONE : (flags & sMBF_INVERTCULL?sD3DCULL_CW:sD3DCULL_CCW);

  *data++ = D3DRS_COLORWRITEENABLE;
  *data++ = (flags & sMBF_ZONLY) ? 0 : 15;

  *data++ = sD3DRS_FOGENABLE;
  *data++ = 0;

  *data++ = sD3DRS_CLIPPING;
  *data++ = 1;
  
  *data++ = sD3DRS_LIGHTING;
  *data++ = (flags & sMBF_LIGHT) ? 1 : 0;

  *data++ = sD3DRS_NORMALIZENORMALS;
  *data++ = (flags & sMBF_RENORMALIZE) ? 1 : 0;

  switch(flags&sMBF_BLENDMASK)
  {
  case sMBF_BLENDOFF:
    *data++ = sD3DRS_ALPHABLENDENABLE;
    *data++ = 0;
    break;
  case sMBF_BLENDADD:
    *data++ = sD3DRS_ALPHABLENDENABLE;
    *data++ = 1;
    *data++ = sD3DRS_SRCBLEND;
    *data++ = sD3DBLEND_ONE;
    *data++ = sD3DRS_DESTBLEND;
    *data++ = sD3DBLEND_ONE;
    *data++ = sD3DRS_BLENDOP;
    *data++ = sD3DBLENDOP_ADD;
    break;
  case sMBF_BLENDMUL:
    *data++ = sD3DRS_ALPHABLENDENABLE;
    *data++ = 1;
    *data++ = sD3DRS_SRCBLEND;
    *data++ = sD3DBLEND_ZERO;
    *data++ = sD3DRS_DESTBLEND;
    *data++ = sD3DBLEND_SRCCOLOR;
    *data++ = sD3DRS_BLENDOP;
    *data++ = sD3DBLENDOP_ADD;
    break;
  case sMBF_BLENDALPHA:
    *data++ = sD3DRS_ALPHABLENDENABLE;
    *data++ = 1;
    *data++ = sD3DRS_SRCBLEND;
    *data++ = sD3DBLEND_SRCALPHA;
    *data++ = sD3DRS_DESTBLEND;
    *data++ = sD3DBLEND_INVSRCALPHA;
    *data++ = sD3DRS_BLENDOP;
    *data++ = sD3DBLENDOP_ADD;
    break;
  case sMBF_BLENDSMOOTH:
    *data++ = sD3DRS_ALPHABLENDENABLE;
    *data++ = 1;
    *data++ = sD3DRS_SRCBLEND;
    *data++ = sD3DBLEND_ONE;
    *data++ = sD3DRS_DESTBLEND;
    *data++ = sD3DBLEND_INVSRCCOLOR;
    *data++ = sD3DRS_BLENDOP;
    *data++ = sD3DBLENDOP_ADD;
    break;
  }

  switch(texmode)
  {
  default:
  case sMBM_FLAT:
    *data++ = sD3DTSS_0|sD3DTSS_COLOROP;    *data++ = sD3DTOP_SELECTARG2;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_COLOROP;    *data++ = sD3DTOP_DISABLE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_SELECTARG2;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_DISABLE;
    break;
  case sMBM_TEX:
    *data++ = sD3DTSS_0|sD3DTSS_COLOROP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_COLOROP;    *data++ = sD3DTOP_DISABLE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_DISABLE;
    break;
  case sMBM_ADD:
    *data++ = sD3DTSS_0|sD3DTSS_COLOROP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_COLOROP;    *data++ = sD3DTOP_ADD;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_2|sD3DTSS_COLOROP;    *data++ = sD3DTOP_DISABLE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_SELECTARG2;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_2|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_DISABLE;
    break;
  case sMBM_MUL:
    *data++ = sD3DTSS_0|sD3DTSS_COLOROP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_COLOROP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_2|sD3DTSS_COLOROP;    *data++ = sD3DTOP_DISABLE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_SELECTARG2;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_2|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_DISABLE;
    break;
	case sMBM_BLEND:
    *data++ = sD3DTSS_0|sD3DTSS_COLOROP;    *data++ = sD3DTOP_SELECTARG1;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_COLOROP;    *data++ = sD3DTOP_BLENDDIFFUSEALPHA;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_2|sD3DTSS_COLOROP;    *data++ = sD3DTOP_DISABLE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_SELECTARG1;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_SELECTARG2;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_2|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_DISABLE;
		break;
  case sMBM_SUB:
    *data++ = sD3DTSS_0|sD3DTSS_COLOROP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_COLOROP;    *data++ = sD3DTOP_SUBTRACT;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_2|sD3DTSS_COLOROP;    *data++ = sD3DTOP_DISABLE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_SELECTARG2;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_2|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_DISABLE;
    break;
  case sMBM_ADDSMOOTH:
    *data++ = sD3DTSS_0|sD3DTSS_COLOROP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_COLOROP;    *data++ = sD3DTOP_ADDSMOOTH;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_2|sD3DTSS_COLOROP;    *data++ = sD3DTOP_DISABLE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_SELECTARG2;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_2|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_DISABLE;
    break;
	case sMBM_ADDCOL:
		*data++ = sD3DTSS_0|sD3DTSS_COLOROP;		*data++ = sD3DTOP_ADD;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG1;	*data++ = sD3DTA_TEXTURE;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG2;	*data++ = sD3DTA_DIFFUSE;
		*data++ = sD3DTSS_1|sD3DTSS_COLOROP;		*data++ = sD3DTOP_DISABLE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_MODULATE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;	*data++ = sD3DTA_TEXTURE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;	*data++ = sD3DTA_DIFFUSE;
		*data++	= sD3DTSS_1|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_DISABLE;
		break;
	case sMBM_MULCOL:
		*data++ = sD3DTSS_0|sD3DTSS_COLOROP;		*data++ = sD3DTOP_MODULATE;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG1;	*data++ = sD3DTA_TEXTURE;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG2;	*data++ = sD3DTA_DIFFUSE;
		*data++ = sD3DTSS_1|sD3DTSS_COLOROP;		*data++ = sD3DTOP_DISABLE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_MODULATE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;	*data++ = sD3DTA_TEXTURE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;	*data++ = sD3DTA_DIFFUSE;
		*data++	= sD3DTSS_1|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_DISABLE;
		break;
	case sMBM_BLENDCOL:
		*data++ = sD3DTSS_0|sD3DTSS_COLOROP;		*data++ = sD3DTOP_BLENDDIFFUSEALPHA;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG1;	*data++ = sD3DTA_DIFFUSE;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG2;	*data++ = sD3DTA_TEXTURE;
		*data++ = sD3DTSS_1|sD3DTSS_COLOROP;		*data++ = sD3DTOP_DISABLE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_SELECTARG1;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;	*data++ = sD3DTA_TEXTURE;
		*data++	= sD3DTSS_1|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_DISABLE;
		break;
	case sMBM_SUBCOL:
		*data++ = sD3DTSS_0|sD3DTSS_COLOROP;		*data++ = sD3DTOP_SUBTRACT;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG1;	*data++ = sD3DTA_TEXTURE;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG2;	*data++ = sD3DTA_DIFFUSE;
		*data++ = sD3DTSS_1|sD3DTSS_COLOROP;		*data++ = sD3DTOP_DISABLE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_MODULATE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;	*data++ = sD3DTA_TEXTURE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;	*data++ = sD3DTA_DIFFUSE;
		*data++	= sD3DTSS_1|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_DISABLE;
		break;
	case sMBM_ADDSMOOTHCOL:
		*data++ = sD3DTSS_0|sD3DTSS_COLOROP;		*data++ = sD3DTOP_ADDSMOOTH;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG1;	*data++ = sD3DTA_TEXTURE;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG2;	*data++ = sD3DTA_DIFFUSE;
		*data++ = sD3DTSS_1|sD3DTSS_COLOROP;		*data++ = sD3DTOP_DISABLE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_MODULATE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;	*data++ = sD3DTA_TEXTURE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;	*data++ = sD3DTA_DIFFUSE;
		*data++	= sD3DTSS_1|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_DISABLE;
		break;
  }

  if(flags & sMBF_COLOR)
  {
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG2;	*data++ = sD3DTA_TFACTOR;
    *data++ = sD3DRS_TEXTUREFACTOR;         *data++ = color;
  }
}

/****************************************************************************/

void sSystemDX::StateTex(sU32 *&data,sInt nr,sU32 flags)
{
  sInt tex,sam;
  static sMatrix envimat;

  tex = nr*sD3DTSS_1;
  sam = nr*sD3DSAMP_1;

  *data++ = sam+sD3DSAMP_MAGFILTER;
  *data++ = flags & sMTF_FILTERMAG ? sD3DTEXF_LINEAR : sD3DTEXF_POINT;

  *data++ = sam+sD3DSAMP_MINFILTER;
  *data++ = flags & sMTF_FILTERMIN ? sD3DTEXF_LINEAR : sD3DTEXF_POINT;

  *data++ = sam+sD3DSAMP_MIPFILTER;
  *data++ = flags & sMTF_MIPMAP ? sD3DTEXF_LINEAR : sD3DTEXF_NONE;

  *data++ = sam+sD3DSAMP_ADDRESSU;
  *data++ = flags & sMTF_CLAMP ? sD3DTADDRESS_CLAMP : sD3DTADDRESS_WRAP;

  *data++ = sam+sD3DSAMP_ADDRESSV;
  *data++ = flags & sMTF_CLAMP ? sD3DTADDRESS_CLAMP : sD3DTADDRESS_WRAP;

  *data++ = tex+sD3DTSS_TEXTURETRANSFORMFLAGS;
  *data++ = ((flags & sMTF_UVTRANS)||(flags & sMTF_ENVIMASK)) ? 2 : 0;

  *data++ = tex+sD3DTSS_TEXCOORDINDEX;
  switch(flags & sMTF_UVMASK)
  {
    case sMTF_UVENVI: *data++ = D3DTSS_TCI_CAMERASPACENORMAL;           break;
    case sMTF_UVSPEC: *data++ = D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR; break;
    case sMTF_UVPOS:  *data++ = D3DTSS_TCI_CAMERASPACEPOSITION;         break;
    default:          *data++ = (flags>>sMTF_UVSHIFT)&7;                break;
  }
}

/****************************************************************************/

void sSystemDX::StateLightEnv(sU32 *&data,sLightEnv &le)
{
  sFatal("obsolete. use sD3DMAT_???\n");
}

/****************************************************************************/

void sSystemDX::StateEnd(sU32 *&data,sU32 *buffer,sInt size)
{
  *data++ = ~0;
  *data++ = ~0;
  sVERIFY(data <= buffer+size/4);
}

sBool sSystemDX::StateValidate(sU32 *buffer)
{
  return sTRUE;
}

sU32 *sSystemDX::StateOptimise(sU32 *buffer)
{
  sInt i,j,max;

  max = 0;
  while(buffer[max*2]!=~0)
    max++;

  return buffer+max*2;

  for(i=0;i<max-1;i++)
  {
    for(j=i+1;j<max;)
    {
      if(buffer[i*2]==buffer[j*2])
      {
        buffer[i*2+1] = buffer[j*2+1];
        sCopyMem(buffer+(j*2),buffer+2+(j*2),(max-j)*8);
        max--;
      }
      else
      {
        j++;
      }
    }
  }
  return buffer+max*2;
}

/****************************************************************************/

void sSystemDX::ClearLights()
{
  sInt i;
  
  for(i=0;i<LightCount;i++)
    DXDev->LightEnable(i,0);

  LightCount = 0;
}

sInt sSystemDX::AddLight(sLightInfo &li)
{
  sD3DLight light;
 
  sVERIFY(LightCount<MAX_LIGHTS);

  if(li.Type!=sLI_AMBIENT)
  {
    light.Type = li.Type;
    light.Diffuse.InitColorWithBug(li.Diffuse);
    light.Diffuse.Scale4(li.Amplify);
    light.Specular.InitColorWithBug(li.Specular);
    light.Specular.Scale4(li.Amplify);
    light.Ambient.InitColorWithBug(li.Ambient);
    light.Ambient.Scale4(li.Amplify);
    light.Position[0] = li.Pos.x;
    light.Position[1] = li.Pos.y;
    light.Position[2] = li.Pos.z;
    light.Direction[0] = li.Dir.x;
    light.Direction[1] = li.Dir.y;
    light.Direction[2] = li.Dir.z;
    light.Range = li.Range;
    light.Falloff = li.Gamma;
    light.Attenuation[0] = 1.0;
    light.Attenuation[1] = ((1.0f/li.RangeCut)-1)/li.Range;
    light.Attenuation[2] = 0.0;
    light.Theta = li.Inner*(sPI/2);
    light.Phi = li.Outer*(sPI/2);
    DXDev->SetLight(LightCount,(D3DLIGHT9 *)&light);
    LightAmbient[LightCount].Init();
    LightMask[LightCount] = li.Mask & 0x7fffffff;
  }
  else
  {
    LightAmbient[LightCount].InitColorWithBug(li.Ambient);
    LightMask[LightCount] = li.Mask | 0x80000000;
  }

  return LightCount++;
}

void sSystemDX::EnableLights(sU32 mask)
{
  sInt i;
  sVector ambient;

  ambient.Init();
  for(i=0;i<LightCount;i++)
  {
    if(LightMask[i]&0x80000000)
    {
      if(LightMask[i]&mask)
        ambient.Add4(LightAmbient[i]);
    }
    else
    {
      DXDev->LightEnable(i,(LightMask[i]&mask)?1:0);
    }
  }

  DXDev->SetRenderState(D3DRS_AMBIENT,
    (sRange<sInt>(ambient.x*255,255,0)<<16)+
    (sRange<sInt>(ambient.y*255,255,0)<< 8)+
    (sRange<sInt>(ambient.z*255,255,0)    )+
    (sRange<sInt>(ambient.w*255,255,0)<<24));

  __asm finit; // driver bugs SUCK SUCK SUCK SUCK SUCK SUCK
}

sInt sSystemDX::GetLightCount()
{
  return LightCount;
}

/****************************************************************************/

#if sUSE_SHADERS

sInt sSystemDX::ShaderCompile(sBool ps,sChar *text)
{
  sInt i;
  HRESULT hr;
  ID3DXBuffer *shader;
  ID3DXBuffer *error;

  for(i=0;i<MAX_SHADERS;i++)
  {
    if(Shaders[i].PS==0 && Shaders[i].VS==0)
    {
      hr = D3DXAssembleShader(text,sGetStringLen(text),0,0,0,&shader,&error);
      if(FAILED(hr))
      {
        if(error)
        {
          sDPrint((sChar *)error->GetBufferPointer());
          error->Release();
        }
        return sINVALID;
      }
      if(error)
        error->Release();
      if(ps)
      {
        hr = DXDev->CreatePixelShader((DWORD *)shader->GetBufferPointer(),&Shaders[i].PS);
        shader->Release();
        if(FAILED(hr))
          return sINVALID;
        return i;
      }
      else
      {
        hr = DXDev->CreateVertexShader((DWORD *)shader->GetBufferPointer(),&Shaders[i].VS);
        shader->Release();
        if(FAILED(hr))
          return sINVALID;
        return i;
      }
    }
  }
  return sINVALID;
}

void sSystemDX::ShaderFree(sInt i)
{
  if(Shaders[i].VS)
  {
    Shaders[i].VS->Release();
    Shaders[i].VS = 0;
  }
  if(Shaders[i].PS)
  {
    Shaders[i].PS->Release();
    Shaders[i].PS = 0;
  }
}

void sSystemDX::ShaderWrite(sU32 *&data)
{
  sFatal("sSystemDX::ShaderWrite() not implemented");
}

void sSystemDX::ShaderRead(sU32 *&data)
{
  sFatal("sSystemDX::ShaderRead() not implemented");
}

void sSystemDX::ShaderTex(sInt nr,sInt flags,sInt tex)
{
  sInt sam;

  sam = nr*sD3DSAMP_1;
  SetState(sD3DSAMP_MAGFILTER+sam , flags&sMTF_FILTERMAG ? sD3DTEXF_LINEAR : sD3DTEXF_POINT);
  SetState(sD3DSAMP_MINFILTER+sam , flags&sMTF_FILTERMAG ? sD3DTEXF_LINEAR : sD3DTEXF_POINT);
  SetState(sD3DSAMP_MIPFILTER+sam , flags&sMTF_MIPMAP ? sD3DTEXF_LINEAR : sD3DTEXF_NONE);
  SetState(sD3DSAMP_ADDRESSU+sam , flags&sMTF_CLAMP ? sD3DTADDRESS_CLAMP : sD3DTADDRESS_WRAP);
  SetState(sD3DSAMP_ADDRESSV+sam , flags&sMTF_CLAMP ? sD3DTADDRESS_CLAMP : sD3DTADDRESS_WRAP);

  if(tex==sINVALID)
    DXDev->SetTexture(nr,0);
  else
    DXDev->SetTexture(nr,Textures[tex].Tex);
}

void sSystemDX::ShaderCam(sCamera *cam,sMatrix &obj,sCamera *lcam)
{
  sF32 ratio,q;
  sMatrix mat;
  sMatrix c[4];

// cache camera

  if(cam)
  {
    ratio = cam->AspectX/cam->AspectY;
    q = cam->FarClip/(cam->FarClip-cam->NearClip);
    mat.i.Init(cam->ZoomX    ,0               , 0              ,0);
    mat.j.Init(0             ,cam->ZoomY*ratio, 0              ,0);
    mat.k.Init(cam->CenterX  ,cam->CenterY    , q              ,1);
    mat.l.Init(0             ,0               ,-q*cam->NearClip,0);

    ShaderLastProj = mat;
    LastCamera = cam->CamPos;
    LastCamera.TransR();
    LightMatrix.Init();
    LightMatrixUsed = sFALSE;
  }
  if(lcam)
  {
    LightMatrix = lcam->Matrix;
    LightMatrixUsed = sTRUE;
  }

// matrix stuff

  LastMatrix = obj;
  mat.Mul4(LastMatrix,LastCamera);
  LastTransform = mat;
  RecalcTransform = sFALSE;

  c[0].Mul4(LastTransform,ShaderLastProj);
  c[0].Trans4();
  c[1] = obj;
  c[1].Trans4();
  c[2] = mat;
  c[2].Trans4();
  if(LightMatrixUsed)
  {
    c[3].Mul4(obj,LightMatrix);
    c[3].Trans4();
  }
  else
  {
    c[3].Init();
  }

  DXDev->SetVertexShaderConstantF(0,(sF32 *)c,16); 
}

void sSystemDX::ShaderConst(sBool ps,sInt ofs,sInt cnt,sVector *c)
{
  if(ps)
    DXDev->SetPixelShaderConstantF(ofs,(sF32 *)c,cnt);
  else
    DXDev->SetVertexShaderConstantF(ofs,(sF32 *)c,cnt);
}

void sSystemDX::ShaderSet(sInt vs,sInt ps,sU32 flags)
{
  HRESULT hr;
  hr = DXDev->SetVertexShader(Shaders[vs].VS);
  sVERIFY(!FAILED(hr));
  hr = DXDev->SetPixelShader(Shaders[ps].PS);
  sVERIFY(!FAILED(hr));

  SetState(sD3DRS_ALPHATESTENABLE,0);
  SetState(sD3DRS_ALPHAFUNC,sD3DCMP_GREATER);
  SetState(sD3DRS_ALPHAREF,4);
  SetState(sD3DRS_STENCILENABLE,0);
  SetState(sD3DRS_ZENABLE,sD3DZB_TRUE);
  SetState(sD3DRS_ZWRITEENABLE,(flags & sMBF_ZWRITE) ? 1 : 0);
  SetState(sD3DRS_ZFUNC,(flags & sMBF_ZREAD) ? sD3DCMP_LESSEQUAL : sD3DCMP_ALWAYS);
  SetState(sD3DRS_CULLMODE,(flags & sMBF_DOUBLESIDED) ? sD3DCULL_NONE : (flags & sMBF_INVERTCULL?sD3DCULL_CW:sD3DCULL_CCW));
  SetState(sD3DRS_COLORWRITEENABLE,(flags & sMBF_ZONLY) ? 0 : 15);
  SetState(sD3DRS_FOGENABLE,0);
  SetState(sD3DRS_CLIPPING,1);

  switch(flags&sMBF_BLENDMASK)
  {
  case sMBF_BLENDOFF:
    SetState(sD3DRS_ALPHABLENDENABLE,0);
    break;
  case sMBF_BLENDADD:
    SetState(sD3DRS_ALPHABLENDENABLE,1);
    SetState(sD3DRS_SRCBLEND,sD3DBLEND_ONE);
    SetState(sD3DRS_DESTBLEND,sD3DBLEND_ONE);
    SetState(sD3DRS_BLENDOP,sD3DBLENDOP_ADD);
    break;
  case sMBF_BLENDMUL:
    SetState(sD3DRS_ALPHABLENDENABLE,1);
    SetState(sD3DRS_SRCBLEND,sD3DBLEND_ZERO);
    SetState(sD3DRS_DESTBLEND,sD3DBLEND_SRCCOLOR);
    SetState(sD3DRS_BLENDOP,sD3DBLENDOP_ADD);
    break;
  case sMBF_BLENDALPHA:
    SetState(sD3DRS_ALPHABLENDENABLE,1);
    SetState(sD3DRS_SRCBLEND,sD3DBLEND_SRCALPHA);
    SetState(sD3DRS_DESTBLEND,sD3DBLEND_INVSRCALPHA);
    SetState(sD3DRS_BLENDOP,sD3DBLENDOP_ADD);
    break;
  case sMBF_BLENDSMOOTH:
    SetState(sD3DRS_ALPHABLENDENABLE,1);
    SetState(sD3DRS_SRCBLEND,sD3DBLEND_ONE);
    SetState(sD3DRS_DESTBLEND,sD3DBLEND_INVSRCCOLOR);
    SetState(sD3DRS_BLENDOP,sD3DBLENDOP_ADD);
    break;
  }

  ShaderOn = sTRUE;
}

void sSystemDX::ShaderClear()
{
  if(ShaderOn)
  {
    DXDev->SetVertexShader(0);
    DXDev->SetPixelShader(0);
    ShaderOn = sFALSE;
  }
}

#endif // sUSE_SHADERS

/****************************************************************************/

#endif // sUSE_DIRECT3D
