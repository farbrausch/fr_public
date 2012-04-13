// This file is distributed under a BSD license. See LICENSE.txt for details.
#include "_start.hpp"
#include "_startgl.hpp"

#if sUSE_OPENGL

#define WINVER 0x500
#include <windows.h>
#include <gl/gl.h>
#define WINZERO(x) {sSetMem(&x,0,sizeof(x));}
#define WINSET(x) {sSetMem(&x,0,sizeof(x));x.dwSize = sizeof(x);}
#define RELEASE(x) {if(x)x->Release();x=0;}


/****************************************************************************/
/****************************************************************************/

void sSystemGL::InitScreens()
{
  sInt nr;
  static PIXELFORMATDESCRIPTOR pfd;
  sInt pf;
  RECT r;

  DynamicSize = 1024*1024*16;
  DynamicBuffer = new sU8[DynamicSize];
  DynamicRead = 0;
  DynamicWrite = 0;

  for(nr=0;nr<WScreenCount;nr++)
  {
    scr = &Screen[nr];

    GetClientRect((HWND) scr->Window,&r);
    scr->XSize = r.right-r.left;
    scr->YSize = r.bottom-r.top;

    scr->hdc = (sU32) GetDC((HWND)scr->Window);

    memset(&pfd,0,sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 16;
    pfd.cAlphaBits = 0;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    pf = ChoosePixelFormat((HDC)scr->hdc,&pfd);
    sDPrintF("pixel format %d\n",pf);
    if(!pf)
      sFatal("could not find Pixel format");

    if(!SetPixelFormat((HDC)scr->hdc,pf,&pfd))
      sFatal("could not set pixel format");

    scr->glrc = (sU32) wglCreateContext((HDC)scr->hdc);
    if(!scr->glrc)
      sFatal("could not create OpenGL Context");
    if(!wglMakeCurrent((HDC)scr->hdc,(HGLRC)scr->glrc))
      sFatal("could not activate OpenGL Context");

    scr = 0;
  }
}

/****************************************************************************/

void sSystemGL::ExitScreens()
{
  sInt nr;

  wglMakeCurrent(0,0);

  for(nr=0;nr<WScreenCount;nr++)
  {
    scr = &Screen[nr];

    if(scr->glrc)
    {
      wglDeleteContext((HGLRC)scr->glrc);
    } 
    if(scr->hdc)
    {
      ReleaseDC((HWND)scr->Window,(HDC)scr->hdc);
      scr->hdc=0;
    }

    scr = 0;
  }

  if(DynamicBuffer)
    delete[] DynamicBuffer;
}

/****************************************************************************/

void sSystemGL::Render()
{
  sInt i;

  DynamicRead = 0;
  DynamicWrite = 0;

  sAppHandler(sAPPCODE_FRAME,0);
  sAppHandler(sAPPCODE_TICK,1);
  sAppHandler(sAPPCODE_PAINT,0);

  for(i=0;i<WScreenCount;i++)
  {
//    Screen[i].DXDev->Clear(0,0,D3DCLEAR_TARGET,0xff000000|GetTime(),0,0);

    glFinish();
    SwapBuffers((HDC)Screen[i].hdc);
  }

  for(i=1;i<MAX_GEOHANDLE;i++)
    GeoHandle[i].DiscardCount = 0;
  DiscardCount = 1;
}

/****************************************************************************/
/****************************************************************************/

/*
void sSystemGL::ParseFVF(sU32 fvf,sU32 &dxfvf,sInt &floats)
{
  if(fvf&sFVF_3D)
  {
    floats  = 3;
  }
  else
  {
    floats  = 4;
  }

  if(fvf&sFVF_NORMAL)
  {
    floats += 3;
  }
  if(fvf&sFVF_COLOR0)
  {
    floats += 1;
  }
  if(fvf&sFVF_COLOR1)
  {
    floats += 1;
  }
  if(fvf&sFVF_PSIZE)
  {
    floats += 1;
  }
  switch(fvf&0x000f00)
  {
  case sFVF_TEX1:
    floats += 2;
    break;
  case sFVF_TEX2:
    floats += 4;
    break;
  case sFVF_TEX3:
    floats += 6;
    break;
  case sFVF_TEX4:
    floats += 8;
    break;
  }
  if(fvf&sFVF_TEX3D0)
  {
    sVERIFY((fvf&0x000f00)==sFVF_TEX1);
    floats += 1;
  }
}
  */

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

void sSystemGL::BeginViewport(const sViewport &vp)
{
  sInt nr;
  sInt xs,ys;

  nr = vp.Screen;
  ScreenNr = nr;
  scr = &Screen[nr];
  sVERIFY(nr>=0 && nr<WScreenCount);

  xs = vp.Window.XSize();
  ys = vp.Window.YSize();
  if(xs==0 || ys==0)
  {
    glViewport(0,0,Screen[nr].XSize,Screen[nr].YSize);
    glDisable(GL_SCISSOR_TEST);
  }
  else
  {
    glViewport(vp.Window.x0,Screen[nr].YSize-vp.Window.y1,vp.Window.x1-vp.Window.x0,vp.Window.y1-vp.Window.y0);
    glScissor (vp.Window.x0,Screen[nr].YSize-vp.Window.y1,vp.Window.x1-vp.Window.x0,vp.Window.y1-vp.Window.y0);
    glEnable(GL_SCISSOR_TEST);
  }

  glClearColor(vp.ClearColor.r/255.0f,vp.ClearColor.g/255.0f,vp.ClearColor.b/255.0f,vp.ClearColor.a/255.0f);
  glClearDepth(1.0f);

  switch(vp.ClearFlags)
  {
  case sVCF_COLOR:
    glClear(GL_COLOR_BUFFER_BIT);
    break;
  case sVCF_Z:
    glClear(GL_DEPTH_BUFFER_BIT);
    break;
  case sVCF_ALL:
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    break;
  }

  glDisable(GL_SCISSOR_TEST);     // scissor only for clear!
  glShadeModel(GL_SMOOTH);        // gouraud shading
}

void sSystemGL::EndViewport()
{
}

/****************************************************************************/

void sSystemGL::SetCamera(sCamera &cam)
{
  sF32 ratio,q;
  sMatrix mat;

  scr->CamMat = cam.CamPos;
  scr->CamMat.TransR();
  LastCamera = scr->CamMat;
  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixf((sF32 *)&scr->CamMat);

  ratio = cam.AspectX/cam.AspectY;
  q = cam.FarClip/(cam.FarClip-cam.NearClip);

  mat.i.Init(cam.ZoomX    ,0              , 0             ,0);
  mat.j.Init(0            ,cam.ZoomY*ratio, 0             ,0);
  mat.k.Init(cam.CenterX  ,cam.CenterY    , q             ,1);
  mat.l.Init(0            ,0              ,-q*cam.NearClip,0);
  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf((sF32 *)&mat);
}

void sSystemGL::SetCameraOrtho()
{
  sMatrix mat;

  scr->CamMat.Init();
  LastTransform.Init();
  LastMatrix.Init();
  LastCamera.Init();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  mat.i.Init(2.0f/scr->XSize,0                ,0 ,0);
  mat.j.Init(0              ,-2.0f/scr->YSize ,0 ,0);
  mat.k.Init(0              ,0                ,1 ,0);
  mat.l.Init(-1             ,1                ,0 ,1);
//  mat.Trans4();
  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf((sF32 *)&mat);
}

void sSystemGL::SetMatrix(sMatrix &mat)
{
  sMatrix mul;

  LastMatrix = mat;
  mul.Mul4(mat,scr->CamMat);
  LastTransform = mul;

  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixf((sF32 *)&mul);

//  DXDev->SetTransform(D3DTS_WORLD,(D3DMATRIX *) &mat);
}

void sSystemGL::GetTransform(sInt mode,sMatrix &mat)
{
  switch(mode)
  {
  case sGT_UNIT:
    mat.Init();
    break;
  case sGT_MODELVIEW:
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

void sSystemGL::SetTexture(sInt stage,sInt handle,sMatrix *mat)
{
  sHardTex *tex;

  if(stage!=0)
    return;

  if(handle==-1)
  {
    glDisable(GL_TEXTURE_2D);
  }
  else
  {
    sVERIFY(handle>=0 && handle<MAX_TEXTURE);
    tex = &Textures[handle];
    if(tex->Flags)
    {
      glBindTexture(GL_TEXTURE_2D,tex->TexGL);
      glEnable(GL_TEXTURE_2D);
      glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    }
    else
    {
      glDisable(GL_TEXTURE_2D);
    }
  }
}

void sSystemGL::StreamTextureStart(sInt handle,sInt level,sBitmapLock &lock)
{
  sVERIFYFALSE;
}

void sSystemGL::StreamTextureEnd()
{
  sVERIFYFALSE;
}
/****************************************************************************/

sInt sSystemGL::AddTexture(const sTexInfo &ti)
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
      tex->Format = ti.Format;
      tex->Tex = 0;
      tex->TexGL = 0;

	    if(tex->Flags & sTIF_RENDERTARGET)
	    {
	    }
	    else
	    {
        glGenTextures(1,(GLuint*)&tex->TexGL);
        if(tex->TexGL==0)
          glGenTextures(1,(GLuint*)&tex->TexGL);
        sVERIFY(tex->TexGL!=0);
        glBindTexture(GL_TEXTURE_2D,tex->TexGL);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,ti.Bitmap->XSize,ti.Bitmap->YSize,0,GL_RGBA,GL_UNSIGNED_BYTE,ti.Bitmap->Data);
	    }

//      UpdateTexture(i,ti.Bitmap);
      return i;
    }
    tex++;
  }

  return sINVALID;
}

sInt sSystemGL::AddTexture(sInt xs,sInt ys,sInt format,sU16 *data)
{
  return sINVALID;
}

/****************************************************************************/

void sSystemGL::RemTexture(sInt handle)
{
  sHardTex *tex;

  sVERIFY(handle>=0 && handle<MAX_TEXTURE);
  tex = &Textures[handle];
  if(tex->TexGL)
  {
    glDeleteTextures(1,(GLuint *)&tex->TexGL);
  }

  tex->Flags = 0;
  tex->TexGL = 0;
}

/****************************************************************************/
/****************************************************************************/

void sSystemGL::CreateGeoBuffer(sInt i,sInt dyn,sInt vertex,sInt size)
{
  sVERIFY(dyn==0 || dyn==1);
  sVERIFY(vertex==0 || vertex==1);

  GeoBuffer[i].Type = 1+vertex;
  GeoBuffer[i].Size = size;
  GeoBuffer[i].Used = 0;
  GeoBuffer[i].VB = 0;
}

sInt sSystemGL::GeoAdd(sInt fvf,sInt prim)
{
  sInt i;
  sGeoHandle *gh;

  for(i=1;i<MAX_GEOHANDLE;i++)
  {
    gh = &GeoHandle[i];
    if(gh->Mode==0)
    {
      sSetMem(gh,0,sizeof(*gh));
//      ParseFVF(fvf,gh->DXFVF,gh->VertexSize);
//      gh->DXFVF = fvf;
      gh->VertexSize*=4;
      gh->Mode = prim;
      return i;
    }
  }
  sFatal("GeoAdd() ran out of handles");
}

void sSystemGL::GeoRem(sInt handle)
{
  sVERIFY(handle>=1 && handle<MAX_GEOHANDLE);
  sVERIFY(GeoHandle[handle].Mode!=0);

  GeoHandle[handle].Mode = 0;
}

void sSystemGL::GeoFlush()
{
  sInt i;

  for(i=1;i<MAX_GEOHANDLE;i++)
    GeoHandle[i].VertexCount = 0;
  for(i=3;i<MAX_GEOBUFFER;i++)
    GeoBuffer[i].Init();
}

void sSystemGL::GeoFlush(sInt handle)
{
}

sBool sSystemGL::GeoDraw(sInt &handle)
{
  return sTRUE;
}

void sSystemGL::GeoBegin(sInt handle,sInt vc,sInt ic,sF32 **fp,sU16 **ip)
{
  sGeoHandle *gh;

  sVERIFY(handle>=1 && handle<MAX_GEOHANDLE);
  sVERIFY(GeoHandle[handle].Mode!=0);
  sVERIFY(vc*GeoHandle[handle].VertexSize<=MAX_DYNVBSIZE); 
  sVERIFY(ic*2<=MAX_DYNIBSIZE);

  gh = &GeoHandle[handle];
  gh->VertexBuffer = 0;
  gh->VertexStart = 0;
  gh->VertexCount = vc;
  gh->IndexBuffer = 0;
  gh->IndexStart = 0;
  gh->IndexCount = ic;

  sVERIFY(DynamicRead==DynamicWrite);
  if(ip)
    *ip = (sU16 *) (DynamicBuffer+DynamicWrite);
  DynamicWrite += gh->IndexCount*2;
  *fp = (sF32 *) (DynamicBuffer+DynamicWrite);
  DynamicWrite += gh->VertexCount * gh->VertexSize;
  sVERIFY(DynamicWrite<DynamicSize);
}

void sSystemGL::GeoEnd(sInt handle,sInt vc,sInt ic)
{
/*
  static sInt mode[] = { 0,GL_POINTS,GL_LINES,GL_TRIANGLES,GL_LINE_STRIP,GL_TRIANGLE_STRIP,GL_QUADS };
  sGeoHandle *gh;
  sF32 *fp;
  sU32 fvf;
  sU16 *ip;

  sVERIFY(handle>=1 && handle<MAX_GEOHANDLE);
  sVERIFY(GeoHandle[handle].Mode!=0);

  gh = &GeoHandle[handle];
  
  ip = (sU16 *) (DynamicBuffer+DynamicRead);
  DynamicRead += gh->IndexCount*2;
  fp = (sF32 *) (DynamicBuffer+DynamicRead);
  DynamicRead += gh->VertexCount * gh->VertexSize;
  sVERIFY(DynamicRead==DynamicWrite);

  fvf = gh->DXFVF;
  
  if(vc!=-1)
    gh->VertexCount = vc;
  if(ic!=-1)
    gh->IndexCount = ic;

  glVertexPointer(3,GL_FLOAT,gh->VertexSize,fp);
  glEnableClientState(GL_VERTEX_ARRAY);
  fp+=3;

  if(fvf&sFVF_NORMAL)
  {
    glEnableClientState(GL_NORMAL_ARRAY);
    glNormalPointer(GL_FLOAT,gh->VertexSize,fp);
    fp+=3;
  }
  else
  {
    glDisableClientState(GL_NORMAL_ARRAY);
    glNormal3f(0,0,0); 
  }

  if(fvf&sFVF_COLOR0)
  {
    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(4,GL_UNSIGNED_BYTE,gh->VertexSize,fp);
    fp++;
  }
  else
  {
    glDisableClientState(GL_COLOR_ARRAY);
    glColor4ub(255,255,255,255);
  }

  if(fvf & sFVF_TEXMASK)
  {
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2,GL_FLOAT,gh->VertexSize,fp);
    fp+=2;
  }
  else
  {
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  }

  if(gh->IndexCount==0)
    glDrawArrays(mode[gh->Mode&7],0,gh->VertexCount);
  else
    glDrawElements(mode[gh->Mode&7],gh->IndexCount,GL_UNSIGNED_SHORT,ip);
//  glFinish();
*/
}

/****************************************************************************/
/****************************************************************************/

void sSystemGL::StateBase(sU32 *&data,sU32 flags,sInt texmode,sU32 color)
{
  sU16 *stream;
  stream = (sU16 *)data;

  switch(flags & sMBF_BLENDMASK)
  {
  case sMBF_BLENDOFF:
    *stream++ = sGLCMD_DISABLE;
    *stream++ = sGL_BLEND;
    break;
  case sMBF_BLENDADD:
    *stream++ = sGLCMD_ENABLE;
    *stream++ = sGL_BLEND;
    *stream++ = sGLCMD_BLENDFUNC;
    *stream++ = sGL_ONE;
    *stream++ = sGL_ONE;
    break;
  case sMBF_BLENDMUL:
    *stream++ = sGLCMD_ENABLE;
    *stream++ = sGL_BLEND;
    *stream++ = sGLCMD_BLENDFUNC;
    *stream++ = sGL_ZERO;
    *stream++ = sGL_SRC_COLOR;
    break;
  case sMBF_BLENDALPHA:
    *stream++ = sGLCMD_ENABLE;
    *stream++ = sGL_BLEND;
    *stream++ = sGLCMD_BLENDFUNC;
    *stream++ = sGL_SRC_ALPHA;
    *stream++ = sGL_ONE_MINUS_SRC_ALPHA;
    break;
  }

  switch(flags & sMBF_ZMASK)
  {
  case 0:
    *stream++ = sGLCMD_DISABLE;
    *stream++ = sGL_DEPTH_TEST;
    break;
  case sMBF_ZREAD:
    *stream++ = sGLCMD_ENABLE;
    *stream++ = sGL_DEPTH_TEST;
    *stream++ = sGLCMD_DEPTHMASK;
    *stream++ = 0;
    break;
  case sMBF_ZWRITE:
    *stream++ = sGLCMD_DISABLE;
    *stream++ = sGL_DEPTH_TEST;
    *stream++ = sGLCMD_DEPTHMASK;
    *stream++ = 1;
    break;
  case sMBF_ZREAD|sMBF_ZWRITE:
    *stream++ = sGLCMD_ENABLE;
    *stream++ = sGL_DEPTH_TEST;
    *stream++ = sGLCMD_DEPTHMASK;
    *stream++ = 1;
    break;
  }

  switch(texmode)
  {
  case sMBM_FLAT:
    *stream++ = sGLCMD_DISABLE;
    *stream++ = sGL_TEXTURE_2D;
    break;
  default:
  case sMBM_TEX:
    *stream++ = sGLCMD_ENABLE;
    *stream++ = sGL_TEXTURE_2D;
    *stream++ = sGLCMD_TEXENV;
    *stream++ = sGL_TEXTURE_ENV_MODE;
    *stream++ = sGL_MODULATE;
    break;
  }

  data = (sU32 *)stream;
}

void sSystemGL::StateTex(sU32 *&data,sInt nr,sU32 flags)
{
  sU16 *stream;
  stream = (sU16 *)data;


  data = (sU32 *)stream;
}

void sSystemGL::StateLightEnv(sU32 *&data,sLightEnv &le)
{
}

void sSystemGL::StateEnd(sU32 *&data,sU32 *buffer,sInt size)
{
  sU16 *stream;
  stream = (sU16 *)data;
  *stream++ = sGLCMD_END;
  data = (sU32 *)stream;
}

/****************************************************************************/

void sSystemGL::SetStates(sU32 *stream32)
{
  sU16 *stream;

  stream = (sU16 *)stream32;

  for(;;)
  {
    switch(*stream)
    {
    case sGLCMD_END:
      return;
    case sGLCMD_ENABLE:
      glEnable(stream[1]);
      break;
    case sGLCMD_DISABLE:
      glDisable(stream[1]);
      break;
    case sGLCMD_TEXPARA2D:
      glTexParameteri(sGL_TEXTURE_2D,stream[1],stream[2]);
      break;
    case sGLCMD_BLENDFUNC:
      glBlendFunc(stream[1],stream[2]);
      break;
    case sGLCMD_TEXENV:
      glTexEnvi(sGL_TEXTURE_ENV,stream[1],stream[2]);
      break;
    case sGLCMD_DEPTHMASK:
      glDepthMask(stream[1]);
      break;
    }
    stream+=(*stream)>>8;
  }
}

void sSystemGL::SetState(sU32 token,sU32 value)
{
  sVERIFYFALSE;
}

/****************************************************************************/

sBool sSystemGL::StateValidate(sU32 *buffer)
{
  return sTRUE;
}

sU32 *sSystemGL::StateOptimise(sU32 *buffer)
{
  sU16 *stream;

  stream = (sU16 *)buffer;
  while(*stream!=sGLCMD_END)
    stream+=(*stream)>>8;

  return (sU32 *)stream;
}

/****************************************************************************/

void sSystemGL::ClearLights()
{
  LightCount=0;
}

sInt sSystemGL::AddLight(sLightInfo &)
{
  sVERIFY(LightCount<MAX_LIGHTS);
  return LightCount++;
}

void sSystemGL::EnableLights(sU32 mask)
{
}

sInt sSystemGL::GetLightCount()
{
  return LightCount;
}

/****************************************************************************/

#if sUSE_SHADERS

sInt sSystemGL::ShaderCompile(sBool ps,sChar *text)
{
  return sINVALID;
}

void sSystemGL::ShaderFree(sInt)
{
}

void sSystemGL::ShaderWrite(sU32 *&data)
{
}

void sSystemGL::ShaderRead(sU32 *&data)
{
}

void sSystemGL::ShaderTex(sInt nr,sInt flags,sInt tex)
{
}

void sSystemGL::ShaderCam(sCamera *,sMatrix &obj,sCamera *lcam)
{
}

void sSystemGL::ShaderConst(sBool ps,sInt ofs,sInt cnt,sVector *c)
{
}

void sSystemGL::ShaderSet(sInt vs,sInt ps,sU32 flags)
{
}

void sSystemGL::ShaderClear()
{
}

#endif

/****************************************************************************/

#endif // sUSE_OPENGL
