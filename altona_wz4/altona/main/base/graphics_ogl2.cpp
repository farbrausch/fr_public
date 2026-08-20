#include <stdio.h>
#include <stdlib.h>
/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "base/types.hpp"
#if sRENDERER == sRENDER_OGL2

#if sCONFIG_SYSTEM_WINDOWS

#undef new
#include <windows.h>
#define new sDEFINE_NEW

extern HWND sHWND;
static HDC GLDC;
static HGLRC GLRC;

#endif

#ifdef __APPLE__
#define GL_GLEXT_PROTOTYPES 1
#ifndef GL_HALF_FLOAT_NV
#define GL_HALF_FLOAT_NV 0x140B
#endif

#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
#include "gl.h"
#include "glext.h"
#include "glprocs.h"
#endif

#include "base/system.hpp"
#include "base/math.hpp"
#include "base/graphics.hpp"
#include "base/windows.hpp"

#include "base/serialize.hpp"

void sInitGfxCommon();
void sExitGfxCommon();

sInt sEngineHacks;

#if sCONFIG_SYSTEM_LINUX && !defined(__APPLE__)

#include <GL/glx.h>

extern Display *sXDisplay();
extern XVisualInfo *sXVisualInfo;
extern Visual *sXVisual;
extern Window sXWnd;
extern sInt sXScreen;
static GLXContext GLXC;

#endif

void GLError(sU32 err,const sChar *file,sInt line,const sChar *system)
{
  sString<256> buffer;

  sSPrintF(buffer,L"%s(%d): %s %08x %d\n",file,line,system,err,err);
  sDPrint(buffer);
  sFatal(buffer);
}
#define GLErr(hr) { sBool err=!(hr); if(err) GLError(0,sTXT(__FILE__),__LINE__,L"opengl"); }
#define GLERR() { sInt err=glGetError(); if(err) GLError(err,sTXT(__FILE__),__LINE__,L"opengl"); }

/****************************************************************************/
/***                                                                      ***/
/***   Globals                                                            ***/
/***                                                                      ***/
/****************************************************************************/

//static sCBufferBase *CurrentCBs[sCBUFFER_MAXSLOT*sCBUFFER_SHADERTYPES];
static sScreenMode DXScreenMode;

/****************************************************************************/

struct sGeoBuffer
{
  GLuint GLName;                  // gl name
  sInt GLType;                    // GL_ARRAY_BUFFER_ARB or GL_ELEMENT_ARRAY_BUFFER_ARB
  sInt GLUsage;                   // GL_STATIC_DRAW, ...

  sGeometryDuration Duration;     // sGD_???
  sInt Type;                      // 0 = VB, 1 = IB

  sInt Alloc;                     // total available bytes
  sInt Used;                      // alreaedy used bytes
  sInt Freed;                     // bytes freed. when Freed == Used, then the buffer is empty and may be reset
  sInt RefCount;                  // live sGeoBufferParts pointing into this buffer
  void *Owner;                    // the sGeoBufferPart that has taken a slice but not filled it in yet
  sBool Mapped;                   // ... and whether that part has the buffer object mapped
};

#define sMAX_GEOBUFFER 512

sGeoBuffer sGeoBuffers[sMAX_GEOBUFFER];
sInt sGeoBufferCount;

// Scratch fbo for the texture-to-texture and texture-to-window copies, so that
// whatever sSetRendertarget has bound survives them.
static GLuint sRTCopyFrameBuffer = 0;

// Per frame draw statistics, the same bookkeeping the dx9 backend does: Stats
// accumulates during the frame, BufferedStats is the last complete frame and is
// what sGetGraphicsStats hands out. The gui switches counting off while it
// paints its own performance monitor, so that does not show up in its own
// numbers.
static sGraphicsStats Stats;
static sGraphicsStats BufferedStats;
static sGraphicsStats DisabledStats;
static sBool StatsEnable = 1;


/****************************************************************************/
/***                                                                      ***/
/***   GLSL program plumbing (macOS)                                      ***/
/***                                                                      ***/
/****************************************************************************/

#if defined(__APPLE__)

#include "base/shader_glsl_macos.hpp"

// The shader sources are Cg, which nothing on this platform can compile, so
// they are translated to GLSL (see shader_glsl_macos.cpp). GLSL differs from
// the ARB assembly path this backend was written against in two ways that
// matter here: a vertex and a pixel shader must be *linked* into one program
// before they can be used, and uniforms belong to that program rather than to
// a shader. So shaders are compiled individually, programs are cached per
// (vertex,pixel) pair, and the constant registers are shadowed on the CPU and
// uploaded just before drawing.

enum { sGlslMaxShaders = 1024, sGlslMaxPrograms = 1024, sGlslMaxRegs = 1024 };

struct sGlslShaderRec
{
  GLuint Obj;                           // GL shader object, 0 if unusable
  sInt IsVertex;
  sInt RegCount;
  sInt AttrCount;
  char AttrName[16][64];
  sInt AttrIndex[16];
  sInt SamplerCount;
  char SamplerName[16][64];
  sInt SamplerReg[16];
};

struct sGlslProgRec
{
  GLuint Vs, Ps;
  GLuint Prog;                          // 0 if the pair failed to link
  GLint VcLoc, PcLoc;
  sInt VcCount, PcCount;
};

static sGlslShaderRec *sGlslShaders[sGlslMaxShaders];
static sInt sGlslShaderCount = 0;
static sGlslProgRec sGlslProgs[sGlslMaxPrograms];
static sInt sGlslProgCount = 0;

static sVector4 sGlslVsRegs[sGlslMaxRegs];
static sVector4 sGlslPsRegs[sGlslMaxRegs];
static sGlslProgRec *sGlslCurProg = 0;

static sGlslShaderRec *sGlslFindShader(GLuint obj)
{
  if(!obj) return 0;
  for(sInt i=0;i<sGlslShaderCount;i++)
    if(sGlslShaders[i]->Obj==obj)
      return sGlslShaders[i];
  return 0;
}

// Compile one translated shader and remember what it needs bound.
static GLuint sGlslCompile(const sChar8 *src,sInt bytes,sInt isvertex)
{
  char *copy = (char *)malloc(bytes+1);
  if(!copy) return 0;
  sCopyMem(copy,src,bytes);
  copy[bytes] = 0;

  sGlslShaderInfo info;
  if(!sGlslTranslateShader(copy,isvertex,&info))
  {
    sLogF(L"gfx",L"shader could not be translated to glsl (%s)\n",isvertex?L"vs":L"ps");
    if(getenv("WZ4_DUMPSHADER"))
    {
      static int n=0;
      char path[256];
      snprintf(path,sizeof(path),"%s/xfail_%d_%s.cg",getenv("WZ4_DUMPSHADER"),n++,isvertex?"vs":"ps");
      FILE *f = fopen(path,"w");
      if(f) { fputs(copy,f); fclose(f); }
    }
    free(copy);
    return 0;
  }

  GLuint obj = glCreateShader(isvertex ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
  const GLchar *p = info.Source.Ptr;
  glShaderSource(obj,1,&p,0);
  glCompileShader(obj);

  GLint status = 0;
  glGetShaderiv(obj,GL_COMPILE_STATUS,&status);
  if(!status)
  {
    char log[1024];
    GLsizei ll = 0;
    glGetShaderInfoLog(obj,sizeof(log),&ll,log);
    sString<1024> wide;
    sCopyString(wide,(const sChar8 *)log,sCOUNTOF(wide));
    sLogF(L"gfx",L"glsl compile failed (%s): %s\n",isvertex?L"vs":L"ps",(const sChar *)wide);
    if(getenv("WZ4_DUMPSHADER"))
    {
      static int n=0;
      char path[256];
      snprintf(path,sizeof(path),"%s/fail_%d_%s.glsl",getenv("WZ4_DUMPSHADER"),n++,isvertex?"vs":"ps");
      FILE *f = fopen(path,"w");
      // fputs stops at the first NUL; write the whole buffer, or the dump is
      // cut off well before the line the compiler is complaining about.
      if(f) { fwrite(info.Source.Ptr,1,info.Source.Len,f); fclose(f); }
    }
    glDeleteShader(obj);
    sGlslFreeShaderInfo(&info);
    free(copy);
    return 0;
  }

  // WZ4_DUMPSHADERALL=<dir> keeps every shader that did compile, the ASC source
  // next to its translation. A translation that compiles can still be wrong,
  // and comparing the two is the only way to see that from outside.
  if(getenv("WZ4_DUMPSHADERALL"))
  {
    static int n = 0;
    const char *dir = getenv("WZ4_DUMPSHADERALL");
    char path[512];
    FILE *f;

    snprintf(path,sizeof(path),"%s/shader_%03d_%s.cg",dir,n,isvertex?"vs":"ps");
    f = fopen(path,"w");
    if(f) { fwrite(copy,1,bytes,f); fclose(f); }

    snprintf(path,sizeof(path),"%s/shader_%03d_%s.glsl",dir,n,isvertex?"vs":"ps");
    f = fopen(path,"w");
    if(f) { fwrite(info.Source.Ptr,1,info.Source.Len,f); fclose(f); }

    n++;
  }
  free(copy);

  if(sGlslShaderCount < sGlslMaxShaders)
  {
    sGlslShaderRec *rec = (sGlslShaderRec *)malloc(sizeof(sGlslShaderRec));
    sSetMem(rec,0,sizeof(*rec));
    rec->Obj = obj;
    rec->IsVertex = isvertex;
    rec->RegCount = info.RegCount;
    rec->AttrCount = info.AttrCount;
    for(sInt i=0;i<info.AttrCount;i++)
    {
      sCopyMem(rec->AttrName[i],info.AttrName[i],64);
      rec->AttrIndex[i] = info.AttrIndex[i];
    }
    rec->SamplerCount = info.SamplerCount;
    for(sInt i=0;i<info.SamplerCount;i++)
    {
      sCopyMem(rec->SamplerName[i],info.SamplerName[i],64);
      rec->SamplerReg[i] = info.SamplerReg[i];
    }
    sGlslShaders[sGlslShaderCount++] = rec;
  }

  sGlslFreeShaderInfo(&info);
  return obj;
}

// Link (or look up) the program for a vertex/pixel shader pair.
static sGlslProgRec *sGlslGetProgram(GLuint vs,GLuint ps)
{
  for(sInt i=0;i<sGlslProgCount;i++)
    if(sGlslProgs[i].Vs==vs && sGlslProgs[i].Ps==ps)
      return &sGlslProgs[i];

  if(sGlslProgCount >= sGlslMaxPrograms) return 0;
  sGlslProgRec *rec = &sGlslProgs[sGlslProgCount++];
  sSetMem(rec,0,sizeof(*rec));
  rec->Vs = vs;
  rec->Ps = ps;

  if(!vs || !ps) return rec;            // Prog stays 0: nothing will be drawn

  GLuint prog = glCreateProgram();
  glAttachShader(prog,vs);
  glAttachShader(prog,ps);

  // attribute locations have to match the generic indices sGeometry::Draw uses
  sGlslShaderRec *vrec = sGlslFindShader(vs);
  if(vrec)
    for(sInt i=0;i<vrec->AttrCount;i++)
      glBindAttribLocation(prog,vrec->AttrIndex[i],vrec->AttrName[i]);

  glLinkProgram(prog);
  GLint status = 0;
  glGetProgramiv(prog,GL_LINK_STATUS,&status);
  if(!status)
  {
    char log[1024];
    GLsizei ll = 0;
    glGetProgramInfoLog(prog,sizeof(log),&ll,log);
    sString<1024> wide;
    sCopyString(wide,(const sChar8 *)log,sCOUNTOF(wide));
    sLogF(L"gfx",L"glsl link failed: %s\n",(const sChar *)wide);
    glDeleteProgram(prog);
    return rec;
  }

  rec->Prog = prog;
  rec->VcLoc = glGetUniformLocation(prog,"sglsl_vc");
  rec->PcLoc = glGetUniformLocation(prog,"sglsl_pc");
  sGlslShaderRec *prec = sGlslFindShader(ps);
  rec->VcCount = vrec ? vrec->RegCount : 0;
  rec->PcCount = prec ? prec->RegCount : 0;

  // samplers are constant per program: point each at its texture stage
  glUseProgram(prog);
  for(sInt pass=0;pass<2;pass++)
  {
    sGlslShaderRec *r = pass ? prec : vrec;
    if(!r) continue;
    for(sInt i=0;i<r->SamplerCount;i++)
    {
      GLint loc = glGetUniformLocation(prog,r->SamplerName[i]);
      if(loc>=0) glUniform1i(loc,r->SamplerReg[i]);
    }
  }
  glUseProgram(0);

  return rec;
}

// Push the shadowed constant registers into the live program.
static void sGlslFlushRegs()
{
  if(!sGlslCurProg || !sGlslCurProg->Prog) return;
  if(sGlslCurProg->VcLoc>=0 && sGlslCurProg->VcCount>0)
    glUniform4fv(sGlslCurProg->VcLoc,sGlslCurProg->VcCount,&sGlslVsRegs[0].x);
  if(sGlslCurProg->PcLoc>=0 && sGlslCurProg->PcCount>0)
    glUniform4fv(sGlslCurProg->PcLoc,sGlslCurProg->PcCount,&sGlslPsRegs[0].x);
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Vertex Formats                                                     ***/
/***                                                                      ***/
/****************************************************************************/

void sVertexFormatHandle::Create()
{
  sVertexFormatHandle::OGLDecl decl[40];
  sInt dindex = 0;

  sClear(decl);

  for(sInt i=0;i<sVF_STREAMMAX;i++)
    VertexSize[i] = 0;

  for(sInt stream=0;stream<sVF_STREAMMAX;stream++)
  {
    sInt firstvert = 1;
    sInt i = 0;
    while(Data[i])
    {
      if(((Data[i]&sVF_STREAMMASK)>>sVF_STREAMSHIFT)==(sU32)stream)
      {
        if(firstvert)
        {
          sVERIFY(dindex<sCOUNTOF(decl));
          firstvert = 0;
          decl[dindex].Mode = 2;
          decl[dindex].Index = stream;
          dindex++;
          Streams = stream+1;
        }

        sVERIFY(dindex<sCOUNTOF(decl));
        
        switch(Data[i]&sVF_USEMASK)
        {
        case sVF_NOP:         decl[dindex].Index = -1;  break;
        case sVF_POSITION:    decl[dindex].Index =  0;  break;
        case sVF_NORMAL:      decl[dindex].Index =  2;  break;
        case sVF_TANGENT:     decl[dindex].Index =  6;  break;
        case sVF_BONEINDEX:   decl[dindex].Index =  7;  break;
        case sVF_BONEWEIGHT:  decl[dindex].Index =  1;  break;
        case sVF_COLOR0:      decl[dindex].Index =  3;  break;
        case sVF_COLOR1:      decl[dindex].Index =  4;  break;
        case sVF_UV0:         decl[dindex].Index =  8;  break;
        case sVF_UV1:         decl[dindex].Index =  9;  break;
        case sVF_UV2:         decl[dindex].Index = 10;  break;
        case sVF_UV3:         decl[dindex].Index = 11;  break;
        case sVF_UV4:         decl[dindex].Index = 12;  break;
        case sVF_UV5:         decl[dindex].Index = 13;  break;
        case sVF_UV6:         decl[dindex].Index = 14;  break;
        case sVF_UV7:         decl[dindex].Index = 15;  break;
        default: sVERIFYFALSE;
        }

        sInt bytes = 0;
        switch(Data[i]&sVF_TYPEMASK)
        {
        case sVF_F2:  decl[dindex].Type=GL_FLOAT;            decl[dindex].Size=2;   decl[dindex].Normalized=0;   bytes=2*4;   break;
        case sVF_F3:  decl[dindex].Type=GL_FLOAT;            decl[dindex].Size=3;   decl[dindex].Normalized=0;   bytes=3*4;   break;
        case sVF_F4:  decl[dindex].Type=GL_FLOAT;            decl[dindex].Size=4;   decl[dindex].Normalized=0;   bytes=4*4;   break;
        case sVF_I4:  decl[dindex].Type=GL_UNSIGNED_BYTE;    decl[dindex].Size=4;   decl[dindex].Normalized=0;   bytes=1*4;   break;
        case sVF_C4:  decl[dindex].Type=GL_UNSIGNED_BYTE;    decl[dindex].Size=4;   decl[dindex].Normalized=1;   bytes=1*4;   break;
        case sVF_S2:  decl[dindex].Type=GL_UNSIGNED_SHORT;   decl[dindex].Size=2;   decl[dindex].Normalized=1;   bytes=1*4;   break;
        case sVF_S4:  decl[dindex].Type=GL_UNSIGNED_SHORT;   decl[dindex].Size=4;   decl[dindex].Normalized=1;   bytes=2*4;   break;
        case sVF_H2:  decl[dindex].Type=GL_HALF_FLOAT_NV;    decl[dindex].Size=2;   decl[dindex].Normalized=0;   bytes=1*4;   break;
        case sVF_H4:  decl[dindex].Type=GL_HALF_FLOAT_NV;    decl[dindex].Size=4;   decl[dindex].Normalized=0;   bytes=2*4;   break;
        case sVF_F1:  decl[dindex].Type=GL_FLOAT;            decl[dindex].Size=1;   decl[dindex].Normalized=0;   bytes=1*4;   break;
        default: sVERIFYFALSE;
        }

        decl[dindex].Mode = 1;
        decl[dindex].Offset = VertexSize[stream];
        VertexSize[stream] += bytes;
        AvailMask |= 1 << (Data[i]&sVF_USEMASK);
        if(decl[dindex].Index >= 0)
          dindex++;
      }
      i++;
    }
  }

  // write endmarker

  sVERIFY(dindex<sCOUNTOF(decl));
  dindex++;

  // copy to handle

  Decl = new sVertexFormatHandle::OGLDecl[dindex];
  sCopyMem(Decl,decl,dindex*sizeof(sVertexFormatHandle::OGLDecl));
}

void sVertexFormatHandle::Destroy()
{
  delete[] Decl;
}

/****************************************************************************/
/***                                                                      ***/
/***   sGeometry                                                          ***/
/***                                                                      ***/
/****************************************************************************/

static sGeoBuffer *sFindGeoBuffer(sInt bytes,sInt type,sGeometryDuration duration)
{
  // find available buffer (don't allocate, just find!)

  bytes = sAlign(bytes,128);
  for(sInt i=0;i<sGeoBufferCount;i++)
  {
    sGeoBuffer *gb = &sGeoBuffers[i];
    if(gb->Owner)                 // still being filled in, see sGeoBufferPart::Init
      continue;
    if(gb->Duration==duration && gb->Type==type && gb->Used+bytes<=gb->Alloc)
      return gb;
  }

  // allocate new buffer

  sVERIFY(sGeoBufferCount<sMAX_GEOBUFFER);
  sGeoBuffer *gb = &sGeoBuffers[sGeoBufferCount++];
  switch(type)
  {
  case 0:
    gb->Alloc = sMax(bytes,1024*1024);
    gb->GLType = GL_ARRAY_BUFFER_ARB;
    break;
  case 1:
    gb->Alloc = sMax(bytes,128*1024);
    gb->GLType = GL_ELEMENT_ARRAY_BUFFER_ARB;
    break;
  default:
    sVERIFYFALSE;
  }
  gb->Used = 0;
  gb->Freed = 0;
  gb->RefCount = 0;
  gb->Owner = 0;
  gb->Mapped = 0;
  gb->Duration = duration;
  gb->Type = type;
  gb->GLName = 0;
  sVERIFY(bytes<=gb->Alloc);

  // create GL object for buffer

  switch(gb->Duration)
  {
  case sGD_STREAM:
    gb->GLUsage = GL_STREAM_DRAW; 
    break;
  case sGD_FRAME:
    gb->GLUsage = GL_DYNAMIC_DRAW;
    break;
  case sGD_STATIC:
    gb->GLUsage = GL_STATIC_DRAW;
    break;
  default:
    sVERIFYFALSE;
  }
  glGenBuffersARB(1,&gb->GLName);
  GLERR();
  glBindBufferARB(gb->GLType,gb->GLName);
  glBufferDataARB(gb->GLType,gb->Alloc,0,gb->GLUsage);
  GLERR();

  // done

  return gb;
}

sGeoBufferPart::sGeoBufferPart()
{
  Buffer = 0;
  Start = 0;
  Count = 0;
}

sGeoBufferPart::~sGeoBufferPart()
{
  Clear();
}

void sGeoBufferPart::Clear()
{
  if(Buffer)
  {
    // Used counts bytes but Freed counted elements, so "Freed == Used" was
    // essentially never true and a static buffer was never handed back. The
    // slots then ran out (sGeoBufferCount<sMAX_GEOBUFFER) partway through a
    // demo. Track live parts instead, the way the dx9 backend does.
    // Dropping a part that was never unlocked -- the marching cubes helper
    // reopens its buffers every frame and leaves the last frame's hanging --
    // has to give the mapping back, or the buffer object stays mapped for good
    // and nothing may ever map it again.
    if(Buffer->Owner==this)
    {
      if(Buffer->Mapped)
      {
        glBindBufferARB(Buffer->GLType,Buffer->GLName);
        glUnmapBufferARB(Buffer->GLType);
        glBindBufferARB(Buffer->GLType,0);
        Buffer->Mapped = 0;
      }
      Buffer->Owner = 0;
    }
    Buffer->RefCount--;
    sVERIFY(Buffer->RefCount>=0);
    if(Buffer->RefCount==0 && Buffer->Duration==sGD_STATIC)
      Buffer->Used = Buffer->Freed = 0;
    Buffer = 0;
  }
  Start = 0;
  Count = 0;
}

void sGeoBufferPart::Init(sInt count,sInt size,sGeometryDuration duration,sInt buffertype)
{
  // Between here and Unlock() the slice is reserved but Used has not moved on
  // yet, and the buffer object is mapped for the whole of it. A second part
  // taken from the same buffer in that window would be handed the same Start
  // and would trip over glMapBuffer, which only allows one mapping per buffer
  // object -- and mcubes deliberately keeps one buffer open per worker thread.
  // So the buffer is off limits until this part is done with it.
  Buffer = sFindGeoBuffer(size*count,buffertype,duration);
  Buffer->RefCount++;
  Buffer->Owner = this;
  Start = Buffer->Used;
  Count = count;
}

void sGeoBufferPart::Lock(void **ptr)
{
  glBindBufferARB(Buffer->GLType,Buffer->GLName);
  sU8 *data = (sU8 *)glMapBufferARB(Buffer->GLType,GL_WRITE_ONLY);
  GLERR();
  Buffer->Mapped = 1;
  glBindBufferARB(Buffer->GLType,0);
  data+=Start;
  *ptr = data;
}

void sGeoBufferPart::Unlock(sInt count,sInt size)
{
  if(count!=-1)
    Count = count;
  Buffer->Used += sAlign(Count*size,128);
  sVERIFY(Buffer->Used<=Buffer->Alloc);
  glBindBufferARB(Buffer->GLType,Buffer->GLName);
  glUnmapBufferARB(Buffer->GLType);
  GLERR();
  glBindBufferARB(Buffer->GLType,0);
  Buffer->Mapped = 0;
  Buffer->Owner = 0;
}

/****************************************************************************/

void sGeometry::Draw(sDrawRange *ir,sInt irc,sInt instancecount, sVertexOffset *off/*=0*/)
{
  // set vertexformat
  sVERIFY(!off);      // vertex stream offset currently unsupported

  sVertexFormatHandle::OGLDecl *decl = Format->GetDecl();
  sInt stride = 0;
  sInt start = 0;
  sInt disablemask = 0;

  if(DebugBreak) sDEBUGBREAK;
  
#if defined(__APPLE__)
  // With a GLSL program bound, position comes from generic attribute 0. Leaving
  // the fixed-function vertex array enabled as well aliases the two and the
  // draw is discarded.
  glDisableClientState(GL_VERTEX_ARRAY);
#else
  glEnableClientState(GL_VERTEX_ARRAY);
#endif

  while(decl->Mode!=0)
  {
    if(decl->Mode==2)
    {
      stride = Format->GetSize(decl->Index);
      start = VertexPart[decl->Index].Start;
      glBindBufferARB(GL_ARRAY_BUFFER_ARB,VertexPart[decl->Index].Buffer->GLName);
    }
    else
    {
      glVertexAttribPointerARB(
        decl->Index,decl->Size,decl->Type,decl->Normalized,
        stride,(const void *)(sDInt)(decl->Offset+start));
      glEnableVertexAttribArrayARB(decl->Index);
      disablemask |= (1<<decl->Index);
    }
    decl++;
  }

  // figure primitive type

  sInt primtype = 0;
  switch(Flags & sGF_PRIMMASK)
  {
  case sGF_TRILIST:
    primtype = GL_TRIANGLES;
    break;
  case sGF_TRISTRIP:
    primtype = GL_TRIANGLE_STRIP;
    break;
  case sGF_LINELIST:
    primtype = GL_LINES;
    break;
  case sGF_LINESTRIP:
    primtype = GL_LINE_STRIP;
    break;
  case sGF_QUADLIST:
    primtype = GL_QUADS;
    break;
  case sGF_SPRITELIST:
    primtype = GL_POINT;
    break;
  default:
    sVERIFYFALSE;
  }

  // fake color to white when unused

  if(!(disablemask&(1<<3)))
    glVertexAttrib4fARB(3,1,1,1,1);

  // do drawing

  sVERIFY(instancecount==0);

#if defined(__APPLE__)
  // uniforms belong to the linked program, so they can only go up once the
  // material has bound it -- which is right before here
  if(!sGlslCurProg || !sGlslCurProg->Prog)
    return;                             // no usable shader: skip, don't draw garbage
  sGlslFlushRegs();
#endif

#if defined(__APPLE__)
#endif

  // Draw ranges ("splitters"): one geometry holds several slices and the
  // caller says which of them to draw. Asserting them away meant that anything
  // that splits its buffer -- marching cubes and the particle nodes do -- could
  // not draw at all. Without a range the whole buffer is one.
  sDrawRange whole;
  if(!ir || irc<=0)
  {
    whole.Start = 0;
    whole.End = IndexPart.Buffer ? IndexPart.Count : VertexPart[0].Count;
    ir = &whole;
    irc = 1;
  }

  const sInt indexsize = (Flags & sGF_INDEX32) ? 4 : 2;
  if(IndexPart.Buffer)
    glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,IndexPart.Buffer->GLName);

  sInt drawn = 0;
  for(sInt i=0;i<irc;i++)
  {
    const sInt count = ir[i].End - ir[i].Start;
    if(count<=0) continue;

    if(IndexPart.Buffer)
    {
      // IndexPart.Start is a byte offset into the shared buffer, the range is
      // counted in indices.
      glDrawRangeElements(primtype,0,VertexPart[0].Count-1,count,
        (Flags & sGF_INDEX32) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT,
        (void*) sDInt(IndexPart.Start + ir[i].Start*indexsize));
    }
    else
    {
      glDrawArrays(primtype,ir[i].Start,count);
    }
    drawn += count;
  }

  GLERR();

  if(StatsEnable)
  {
    Stats.Batches++;
    Stats.Splitter += irc;
    Stats.Vertices += VertexPart[0].Count;
    Stats.Indices += IndexPart.Buffer ? IndexPart.Count : 0;
    switch(primtype)
    {
    case GL_TRIANGLES:      Stats.Primitives += drawn/3; break;
    case GL_TRIANGLE_STRIP: Stats.Primitives += sMax(0,drawn-2); break;
    case GL_LINES:          Stats.Primitives += drawn/2; break;
    case GL_LINE_STRIP:     Stats.Primitives += sMax(0,drawn-1); break;
    case GL_QUADS:          Stats.Primitives += drawn/4; break;
    default:                Stats.Primitives += drawn; break;
    }
  }

  // disable and unbind

  for(sInt i=0;i<16;i++)
    if(disablemask & (1<<i))
      glDisableVertexAttribArrayARB(i);

  glDisableClientState(GL_VERTEX_ARRAY);
  glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
  glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
}

/****************************************************************************/
/***                                                                      ***/
/***   Viewport                                                           ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/
/***                                                                      ***/
/***   Texture                                                            ***/
/***                                                                      ***/
/****************************************************************************/

sBool sReadTexture(sReader &s, sTextureBase *&tex)
{
  return sFALSE;
}

sU64 sGetAvailTextureFormats()
{
  return (1ULL<<sTEX_ARGB8888);
}


#if defined(__APPLE__)

// One place that knows how an altona texture format maps onto GL. The backend
// used to understand three formats and sFatal on everything else, which is what
// an SSAO node's render targets ran into. Returns 0 for a format we cannot
// represent; compressed formats report their GL enum in "internal" and want
// glCompressedTexImage2D instead.
static sBool sMacTexFormat(sInt flags,GLint &internal,GLenum &format,GLenum &type,sBool &compressed)
{
  compressed = sFALSE;
  format = GL_BGRA;
  type = GL_UNSIGNED_BYTE;

  switch(flags & sTEX_FORMAT)
  {
  case sTEX_ARGB8888:   internal = GL_RGBA8;              format = GL_BGRA;            type = GL_UNSIGNED_BYTE;  break;
  case sTEX_ARGB16:     internal = GL_RGBA16;             format = GL_BGRA;            type = GL_UNSIGNED_SHORT; break;
  case sTEX_ARGB2101010:internal = GL_RGB10_A2;           format = GL_BGRA;            type = GL_UNSIGNED_INT_2_10_10_10_REV; break;
  case sTEX_ARGB1555:   internal = GL_RGB5_A1;            format = GL_BGRA;            type = GL_UNSIGNED_SHORT_1_5_5_5_REV; break;
  case sTEX_ARGB4444:   internal = GL_RGBA4;              format = GL_BGRA;            type = GL_UNSIGNED_SHORT_4_4_4_4_REV; break;
  case sTEX_RGB565:     internal = GL_RGB5;               format = GL_RGB;             type = GL_UNSIGNED_SHORT_5_6_5; break;

  case sTEX_A8:         internal = GL_ALPHA8;             format = GL_ALPHA;           type = GL_UNSIGNED_BYTE;  break;
  case sTEX_I8:         internal = GL_LUMINANCE8;         format = GL_LUMINANCE;       type = GL_UNSIGNED_BYTE;  break;
  case sTEX_8TOIA:      internal = GL_INTENSITY8;         format = GL_LUMINANCE;       type = GL_UNSIGNED_BYTE;  break;
  case sTEX_IA8:        internal = GL_LUMINANCE8_ALPHA8;  format = GL_LUMINANCE_ALPHA; type = GL_UNSIGNED_BYTE;  break;
  case sTEX_GR8:        internal = GL_LUMINANCE8_ALPHA8;  format = GL_LUMINANCE_ALPHA; type = GL_UNSIGNED_BYTE;  break;
  case sTEX_GR16:       internal = GL_LUMINANCE16_ALPHA16;format = GL_LUMINANCE_ALPHA; type = GL_UNSIGNED_SHORT; break;

  case sTEX_R16F:       internal = GL_LUMINANCE16F_ARB;   format = GL_LUMINANCE;       type = GL_HALF_FLOAT_ARB; break;
  case sTEX_R32F:       internal = GL_LUMINANCE32F_ARB;   format = GL_LUMINANCE;       type = GL_FLOAT;          break;
  case sTEX_GR16F:      internal = GL_LUMINANCE_ALPHA16F_ARB; format = GL_LUMINANCE_ALPHA; type = GL_HALF_FLOAT_ARB; break;
  case sTEX_GR32F:      internal = GL_LUMINANCE_ALPHA32F_ARB; format = GL_LUMINANCE_ALPHA; type = GL_FLOAT;      break;
  case sTEX_ARGB16F:    internal = GL_RGBA16F_ARB;        format = GL_BGRA;            type = GL_HALF_FLOAT_ARB; break;
  case sTEX_ARGB32F:    internal = GL_RGBA32F_ARB;        format = GL_BGRA;            type = GL_FLOAT;          break;

  case sTEX_DEPTH16:
  case sTEX_DEPTH16NOREAD:
  case sTEX_PCF16:      internal = GL_DEPTH_COMPONENT16; format = GL_DEPTH_COMPONENT; type = GL_UNSIGNED_SHORT; break;
  case sTEX_DEPTH24:
  case sTEX_DEPTH24NOREAD: internal = GL_DEPTH_COMPONENT24; format = GL_DEPTH_COMPONENT; type = GL_UNSIGNED_INT;   break;

  case sTEX_DXT1:       internal = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;  compressed = sTRUE; break;
  case sTEX_DXT1A:      internal = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; compressed = sTRUE; break;
  case sTEX_DXT3:       internal = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; compressed = sTRUE; break;
  case sTEX_DXT5:
  case sTEX_DXT5N:
  case sTEX_DXT5_AYCOCG:internal = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; compressed = sTRUE; break;

  default:
    return sFALSE;
  }
  return sTRUE;
}

// How many bytes one texel of a <format,type> pair takes in client memory. The
// packed types carry a whole texel each, everything else is one value per
// channel.
static sInt sMacTexelBytes(GLenum format,GLenum type)
{
  switch(type)
  {
  case GL_UNSIGNED_INT_2_10_10_10_REV:  return 4;
  case GL_UNSIGNED_SHORT_1_5_5_5_REV:
  case GL_UNSIGNED_SHORT_4_4_4_4_REV:
  case GL_UNSIGNED_SHORT_5_6_5:         return 2;
  }

  sInt channels = 4;
  switch(format)
  {
  case GL_ALPHA: case GL_LUMINANCE: case GL_DEPTH_COMPONENT: channels = 1; break;
  case GL_LUMINANCE_ALPHA:                                   channels = 2; break;
  case GL_RGB: case GL_BGR:                                  channels = 3; break;
  }

  sInt size = 4;
  switch(type)
  {
  case GL_UNSIGNED_BYTE:  size = 1; break;
  case GL_UNSIGNED_SHORT:
  case GL_HALF_FLOAT_ARB: size = 2; break;
  }
  return channels*size;
}

#endif

void sTexture2D::Create2(sInt flags)
{
  GLuint name;
  glGenTextures(1,&name);
  GLName = name;

#if defined(__APPLE__)
  // A rendertarget is never filled through BeginLoad/EndLoad, so its storage
  // has to be allocated here or the FBO attachment comes out incomplete.
  if(flags & sTEX_RENDERTARGET)
  {
    GLint internal;
    GLenum channels,type;
    sBool compressed;
    if(!sMacTexFormat(Flags,internal,channels,type,compressed) || compressed)
    {
      internal = GL_RGBA8;
      channels = GL_BGRA;
      type = GL_UNSIGNED_BYTE;
    }

    // Luminance, intensity and alpha formats are not colour-renderable, and an
    // FBO built on one comes back GL_FRAMEBUFFER_UNSUPPORTED -- which then
    // poisons the next GL call with GL_INVALID_FRAMEBUFFER_OPERATION and trips
    // the fatal error check. Widen them to the RGBA equivalent.
    switch(channels==GL_DEPTH_COMPONENT ? 0 : internal)
    {
    case GL_LUMINANCE8: case GL_INTENSITY8: case GL_ALPHA8:
    case GL_LUMINANCE8_ALPHA8:
      internal = GL_RGBA8;   channels = GL_BGRA; type = GL_UNSIGNED_BYTE;  break;
    case GL_LUMINANCE16: case GL_LUMINANCE16_ALPHA16:
      internal = GL_RGBA16;  channels = GL_BGRA; type = GL_UNSIGNED_SHORT; break;
    case GL_LUMINANCE16F_ARB: case GL_LUMINANCE_ALPHA16F_ARB:
      internal = GL_RGBA16F_ARB; channels = GL_RGBA; type = GL_HALF_FLOAT_ARB; break;
    case GL_LUMINANCE32F_ARB: case GL_LUMINANCE_ALPHA32F_ARB:
      internal = GL_RGBA32F_ARB; channels = GL_RGBA; type = GL_FLOAT; break;
    }

    // Clear the storage rather than leaving it undefined: a target that is
    // never rendered into then reads as black instead of whatever the driver
    // handed us, which is both correct and far easier to diagnose.
    // 8 bytes per texel was a guess, and a 128 bit target (sTEX_ARGB32F, which
    // the julia and fluid nodes ask for) needs 16 -- glTexImage2D then read off
    // the end of this buffer and the driver went down with it.
    sInt clearbytes = sMax(1,SizeX)*sMax(1,SizeY)*sMacTexelBytes(channels,type);
    sU8 *zero = new sU8[clearbytes];
    sSetMem(zero,0,clearbytes);

    glBindTexture(GL_TEXTURE_2D,name);
    for(sInt i=0;i<sMax(1,Mipmaps);i++)
      glTexImage2D(GL_TEXTURE_2D,i,internal,sMax(1,SizeX>>i),sMax(1,SizeY>>i),0,channels,type,zero);
    sDeleteArray(zero);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,sMax(1,Mipmaps)-1);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,Mipmaps>1?GL_LINEAR_MIPMAP_LINEAR:GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

    // A depth texture reads as a single value, and whether that value is the
    // stored depth or the result of comparing it against the coordinate is a
    // per-bind decision -- the shadow map materials want the comparison, the
    // ones that do their own want the depth. Start with the comparison off and
    // let sMaterial::Set say which it is (sMTF_PCF).
    if(channels==GL_DEPTH_COMPONENT)
    {
      glTexParameteri(GL_TEXTURE_2D,GL_DEPTH_TEXTURE_MODE_ARB,GL_LUMINANCE);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_COMPARE_MODE_ARB,GL_NONE);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_COMPARE_FUNC_ARB,GL_LEQUAL);
    }
    glBindTexture(GL_TEXTURE_2D,0);
  }
#endif
}

void sTexture2D::Destroy2()
{
  GLuint name = GLName;
  glDeleteTextures(1,&name);
  GLName = 0;
}

void sTexture2D::BeginLoad(sU8 *&data,sInt &pitch,sInt mipmap)
{
  sVERIFY(LoadBuffer==0);
  sVERIFY(mipmap>=0 && mipmap<Mipmaps);
  sInt xs = SizeX>>mipmap;
  sInt ys = SizeY>>mipmap;

  LoadMipmap = mipmap;
  LoadBuffer = new sU8[BitsPerPixel*xs*ys/8];
  data = LoadBuffer;
  pitch = BitsPerPixel*xs/8;

  // A block compressed texture is filled in one row of 4x4 blocks at a time,
  // so one "row" of it is four rows of pixels wide. Handing back the pitch of a
  // single pixel row made sTexture2D::Load fail its pitch check outright, which
  // is fatal even in a release build.
  switch(Flags & sTEX_FORMAT)
  {
  case sTEX_DXT1: case sTEX_DXT1A: case sTEX_DXT3:
  case sTEX_DXT5: case sTEX_DXT5N: case sTEX_DXT5_AYCOCG:
    pitch *= 4;
    break;
  }
}

void sTexture2D::EndLoad()
{
  sVERIFY(LoadBuffer!=0);

  sInt xs = SizeX>>LoadMipmap;
  sInt ys = SizeY>>LoadMipmap;

#if defined(__APPLE__)

  GLint internal;
  GLenum channels,type;
  sBool compressed;
  if(!sMacTexFormat(Flags,internal,channels,type,compressed))
  {
    sLogF(L"gfx",L"unsupported texture format %d, texture left blank\n",Flags & sTEX_FORMAT);
    sDeleteArray(LoadBuffer);
    return;
  }

  glBindTexture(GL_TEXTURE_2D,GLName);
  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  if(compressed)
    glCompressedTexImage2D(GL_TEXTURE_2D,LoadMipmap,internal,xs,ys,0,BitsPerPixel*xs*ys/8,LoadBuffer);
  else
    glTexImage2D(GL_TEXTURE_2D,LoadMipmap,internal,xs,ys,0,channels,type,LoadBuffer);
  glBindTexture(GL_TEXTURE_2D,0);

#else

  sInt format = 0, channels = 0, type = 0;
  switch(Flags & sTEX_FORMAT)
  {
  case sTEX_ARGB8888:
    format = GL_RGBA8;
    channels = GL_BGRA;
    type = GL_UNSIGNED_BYTE;
    break;
  case sTEX_8TOIA:
    format = GL_INTENSITY8;
    channels = GL_INTENSITY8;
    type = GL_UNSIGNED_BYTE;
    break;
  case sTEX_I8:
    format = GL_LUMINANCE8;
    channels = GL_LUMINANCE8;
    type = GL_UNSIGNED_BYTE;
    break;
  default:
    sFatal(L"unsupported texture format %d\n",Flags);
  }

  glBindTexture(GL_TEXTURE_2D,GLName);
  glTexImage2D(GL_TEXTURE_2D,LoadMipmap,format,xs,ys,0,channels,type,LoadBuffer);
  glBindTexture(GL_TEXTURE_2D,0);

#endif

  sDeleteArray(LoadBuffer);
}

void *sTexture2D::BeginLoadPalette()
{
  sFatal(L"paletted textures not supported");
  return 0;
}

void sTexture2D::EndLoadPalette()
{
}

/****************************************************************************/

void sTextureCube::Create2(sInt flags)
{
  GLuint name;
  glGenTextures(1,&name);
  GLName = name;

#if defined(__APPLE__)
  if(flags & sTEX_RENDERTARGET)
  {
    glBindTexture(GL_TEXTURE_CUBE_MAP,name);
    for(sInt f=0;f<6;f++)
      for(sInt i=0;i<sMax(1,Mipmaps);i++)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+f,i,GL_RGBA8,
          sMax(1,SizeXY>>i),sMax(1,SizeXY>>i),0,GL_BGRA,GL_UNSIGNED_BYTE,0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MAX_LEVEL,sMax(1,Mipmaps)-1);
    glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glBindTexture(GL_TEXTURE_CUBE_MAP,0);
  }
#endif
}

void sTextureCube::Destroy2()
{
  GLuint name = GLName;
  glDeleteTextures(1,&name);
  GLName = 0;
}

void sTextureCube::BeginLoad(sTexCubeFace cf, sU8*& data, sInt& pitch, sInt mipmap)
{
  sVERIFY(LoadBuffer==0);
  sVERIFY(mipmap>=0 && mipmap<Mipmaps);
  sInt xs = SizeXY>>mipmap;
  sInt ys = SizeXY>>mipmap;

  LoadMipmap = mipmap;
  LoadFace = cf;
  LoadBuffer = new sU8[BitsPerPixel*xs*ys/8];
  data = LoadBuffer;
  pitch = BitsPerPixel*xs/8;
}

void sTextureCube::EndLoad()
{
  sVERIFY(LoadBuffer!=0);
  sInt format = 0, channels = 0, type = 0;

  sInt xs = SizeXY>>LoadMipmap;
  sInt ys = SizeXY>>LoadMipmap;

  switch(Flags & sTEX_FORMAT)
  {
  case sTEX_ARGB8888:
    format = GL_RGBA8;
    channels = GL_BGRA;
    type = GL_UNSIGNED_BYTE;
    break;
  case sTEX_8TOIA:
    format = GL_INTENSITY8;
    channels = GL_INTENSITY8;
    type = GL_UNSIGNED_BYTE;
    break;
  case sTEX_I8:
    format = GL_LUMINANCE8;
    channels = GL_LUMINANCE8;
    type = GL_UNSIGNED_BYTE;
    break;
  default:
    sFatal(L"unsupported texture format %d\n",Flags);
  }

  glBindTexture(GL_TEXTURE_CUBE_MAP,GLName);
  glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + LoadFace,LoadMipmap,format,xs,ys,0,channels,type,LoadBuffer);
  glBindTexture(GL_TEXTURE_CUBE_MAP,0);

  sDeleteArray(LoadBuffer);
}

/****************************************************************************/

void sPackDXT(sU8 *d,sU32 *bmp,sInt xs,sInt ys,sInt format,sBool dither)
{
  sVERIFY("sPackDXT not supported with opengl")
}

/****************************************************************************/
/***                                                                      ***/
/***   sTextureProxy                                                      ***/
/***                                                                      ***/
/****************************************************************************/

// A proxy is only ever read from, so all it has to carry is the linked
// texture's object name -- sSetTexture() binds tex->GLName and takes the
// target from Flags, both of which Connect() has already copied over.

void sTextureProxy::Connect2()
{
  GLName = Link ? Link->GLName : 0;
}

void sTextureProxy::Disconnect2()
{
  GLName = 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   Material                                                           ***/
/***                                                                      ***/
/****************************************************************************/

void sMaterial::Create2()
{
}

void sMaterial::Destroy2()
{
}

void sMaterial::Prepare(sVertexFormatHandle *format)
{
  sShader *oldvs = VertexShader;   VertexShader = 0;
  sShader *oldps = PixelShader;    PixelShader = 0;

  SelectShaders(format);

  oldvs->Release();
  oldps->Release();
}

void sMaterial::InitVariants(sInt variants)
{
}

void sMaterial::DiscardVariants()
{
}

void sMaterial::SetVariant(sInt variants)
{
}

void sMaterial::Set(sCBufferBase **cbuffers,sInt cbcount,sBool additive)
{
  SetStates();

#if defined(__APPLE__)

  // a GLSL program is the linked pair, so it is looked up here rather than
  // bound per stage
  sGlslCurProg = sGlslGetProgram(VertexShader ? VertexShader->GLName : 0,
                                 PixelShader  ? PixelShader->GLName  : 0);
  glUseProgram((sGlslCurProg && sGlslCurProg->Prog) ? sGlslCurProg->Prog : 0);

#else

  if(VertexShader)
  {
    glEnable(GL_VERTEX_PROGRAM_ARB);
    glBindProgramARB(GL_VERTEX_PROGRAM_ARB,VertexShader->GLName);
  }
  else
  {
    glBindProgramARB(GL_VERTEX_PROGRAM_ARB,0);
    glDisable(GL_VERTEX_PROGRAM_ARB);
  }
  if(PixelShader)
  {
    glEnable(GL_FRAGMENT_PROGRAM_ARB);
    glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,PixelShader->GLName);
  }
  else
  {
    glDisable(GL_FRAGMENT_PROGRAM_ARB);
    glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,0);
  }

#endif

  // set constant buffers
/*
  for(sInt i=0;i<cbcount;i++)
  {
    sCBufferBase *cb = cbuffers[i];
    sVERIFY(cb->Slot<sCBUFFER_MAXSLOT*sCBUFFER_SHADERTYPES);
    if(cb != CurrentCBs[cb->Slot])
    {
      cb->SetRegs();
      CurrentCBs[cb->Slot] = cb;
    }
  }
  */
  sSetCBuffers(cbuffers,cbcount);
}

static void makeblend(sInt blend,sInt &s,sInt &d,sInt &f)
{
  static const sInt arg[16] = 
  {
    0,
    GL_ZERO,
    GL_ONE,
    GL_SRC_COLOR,
    GL_ONE_MINUS_SRC_COLOR,
    GL_SRC_ALPHA,
    GL_ONE_MINUS_SRC_ALPHA,
    GL_DST_COLOR,
    GL_ONE_MINUS_DST_COLOR,
    GL_DST_ALPHA,
    GL_ONE_MINUS_DST_ALPHA,
    GL_SRC_ALPHA_SATURATE,
    0,
    0,
    GL_CONSTANT_COLOR,
    GL_ONE_MINUS_CONSTANT_COLOR,
  };
  static const sInt func[16] =
  {
    0,
    GL_FUNC_ADD,
    GL_FUNC_SUBTRACT,
    GL_FUNC_REVERSE_SUBTRACT,
    GL_MIN,
    GL_MAX,
  };

  s = arg[blend&15];
  f = func[(blend>>8)&15];
  d = arg[(blend>>16)&15];
}

void sMaterial::SetStates(sInt variant_ignored)
{
  // basic flags
	glDepthMask((Flags & sMTRL_ZWRITE) != 0);

  if(Flags & sMTRL_ZREAD)
  {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
  }
  else
  {
    glDisable(GL_DEPTH_TEST);
  }

  switch(Flags & sMTRL_CULLMASK)
  {
  case sMTRL_CULLOFF:
    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    break;
  case sMTRL_CULLON:
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    break;
  case sMTRL_CULLINV:
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    break;
  }

  glColorMask(1,1,1,1);

  // stencil (not yet implemented)

  glStencilMask(1);

  // alpha blending

  if(BlendColor==sMB_OFF)
  {
    glDisable(GL_BLEND);
  }
  else
  {
    glEnable(GL_BLEND);
    if(BlendAlpha==sMB_SAMEASCOLOR)
    {
      sInt s,d,f;
      makeblend(BlendColor,s,d,f);
      glBlendEquation(f);
      glBlendFunc(s,d);
    }
    else
    {
      sInt sc,dc,fc;
      sInt sa,da,fa;
      makeblend(BlendColor,sc,dc,fc);
      makeblend(BlendAlpha,sa,da,fa);
      glBlendEquationSeparateEXT(fc,fa);
      glBlendFuncSeparate(sc,dc,sa,da);
    }
  }

  // alpha test
  const sU8 alphaFunc = FuncFlags[sMFT_ALPHA];

  if (sMFF_ALWAYS == alphaFunc)
  {
    glDisable(GL_ALPHA_TEST);
  }
  else
  {
    glEnable(GL_ALPHA_TEST);
    const GLenum alphaFuncs[] = {GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL, GL_GEQUAL};
    sVERIFY(0 <= alphaFunc);
    sVERIFY(alphaFunc < sizeof(alphaFuncs));
    glAlphaFunc(alphaFuncs[alphaFunc], AlphaRef / 255.0f);
  }

  // textures

  for(sInt i=0;i<sMTRL_MAXTEX;i++)
  {
    sSetTexture(i,Texture[i]);

    if(Texture[i])
    {
      sInt mipmaps = Texture[i]->Mipmaps>1;
      switch(TFlags[i] & sMTF_LEVELMASK)
      {
      case sMTF_LEVEL0:
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,mipmaps?GL_NEAREST_MIPMAP_NEAREST:GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        break;
      case sMTF_LEVEL1:
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,mipmaps?GL_NEAREST_MIPMAP_LINEAR:GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        break;
      case sMTF_LEVEL2:
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,mipmaps?GL_LINEAR_MIPMAP_LINEAR:GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        break;
      case sMTF_LEVEL3:
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,mipmaps?GL_LINEAR_MIPMAP_LINEAR:GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        break;
      }
      // A shadow map is sampled through the comparison the material asked for;
      // any other depth texture is read for the depth it stores.
      if((Texture[i]->Flags & sTEX_TYPE_MASK)==sTEX_2D)
      {
        sInt fmt = Texture[i]->Flags & sTEX_FORMAT;
        if(fmt==sTEX_PCF16 || fmt==sTEX_DEPTH16 || fmt==sTEX_DEPTH24)
          glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_COMPARE_MODE_ARB,
            (TFlags[i] & sMTF_PCF) ? GL_COMPARE_R_TO_TEXTURE_ARB : GL_NONE);
      }

      switch(TFlags[i] & sMTF_ADDRMASK)
      {
      case sMTF_TILE:
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
        break;
      case sMTF_CLAMP:
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        break;
      case sMTF_MIRROR:
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_MIRRORED_REPEAT);
        break;
      case sMTF_BORDER:
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_BORDER);
        break;
      case sMTF_TILEU_CLAMPV:
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        break;
      }
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Shaders                                                            ***/
/***                                                                      ***/
/****************************************************************************/

sShaderTypeFlag sGetShaderPlatform()
{
  return sSTF_NVIDIA;
}

sInt sGetShaderProfile()
{
  return sSTF_NVIDIA;
}

void sSetVSParamImpl(sInt o, sInt count, const sVector4* vsf)
{
#if defined(__APPLE__)
  if(o>=0 && o+count<=sGlslMaxRegs)
    sCopyMem(&sGlslVsRegs[o],vsf,count*sizeof(sVector4));
  return;
#endif

  while(count>0)
  {
    glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB,o,&vsf->x);
    o++;
    vsf++;
    count--;
  }
}

void sSetVSParam(sInt o, sInt count, const sVector4* vsf)
{
  sSetVSParamImpl(o, count, vsf);
}

void sSetPSParamImpl(sInt o, sInt count, const sVector4* psf)
{
#if defined(__APPLE__)
  if(o>=0 && o+count<=sGlslMaxRegs)
    sCopyMem(&sGlslPsRegs[o],psf,count*sizeof(sVector4));
  return;
#endif

  while(count>0)
  {
    glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB,o,&psf->x);
    o++;
    psf++;
    count--;
  }
}

void sSetPSParam(sInt o, sInt count, const sVector4* psf)
{
  sSetPSParamImpl(o,count,psf);
}

void sSetVSBool(sU32 bits,sU32 mask)
{
}

void sSetPSBool(sU32 bits,sU32 mask)
{
}


void sCreateShader2(sShader *shader, sShaderBlob *blob)
{
#if defined(__APPLE__)

  sInt size = blob->Size;
  if(size>0 && blob->Data[size-1]==0)     // there might be a trailing zero
    size--;

  sInt isvertex;
  switch(shader->Type&sSTF_KIND)
  {
  case sSTF_VERTEX: isvertex = 1; break;
  case sSTF_PIXEL:  isvertex = 0; break;
  default: shader->GLName = 0; return;
  }

  shader->GLName = sGlslCompile((const sChar8 *)blob->Data,size,isvertex);

#else

  sInt type;
  GLuint name;

  switch(shader->Type&sSTF_KIND)
  {
  case sSTF_VERTEX:
    type = GL_VERTEX_PROGRAM_ARB;
    break;
  case sSTF_PIXEL:
    type = GL_FRAGMENT_PROGRAM_ARB;
    break;
  default:
    type = 0;
    sVERIFYFALSE;
  }
  
  glGenProgramsARB(1,&name);
  glBindProgramARB(type,name);
  sInt size = blob->Size;
  if(size>0 && blob->Data[size-1]==0)     // there might be a trailing zero
    size--;

  glProgramStringARB(type,GL_PROGRAM_FORMAT_ASCII_ARB,size,blob->Data);  
  GLenum err = glGetError();
  if(err!=0)
  {
    const sChar8 *error = (const sChar8 *)glGetString(GL_PROGRAM_ERROR_STRING_ARB);
    sString<256> buffer;
    sCopyString(buffer,error,sCOUNTOF(buffer));
    sLogF(L"gfx", L"shader compiler warning %q (size=%d, type=0x%x)\n", buffer, size, type);
  }

  shader->GLName = name;

#endif
}

void sDeleteShader2(sShader *shader)
{
#if defined(__APPLE__)
  if(shader->GLName)
    glDeleteShader(shader->GLName);
#else
  GLuint name = shader->GLName;
  glDeleteProgramsARB(1,&name);
#endif
  shader->GLName = 0;
}

/****************************************************************************/

sCBufferBase::sCBufferBase()
{
  DataPtr = 0;
  DataPersist = 0;
  Slot = 0;
  Flags = 0;
}

void sCBufferBase::SetPtr(void **dataptr,void *data)
{
  DataPtr = dataptr;
  DataPersist = data;
  if(DataPtr)
    *DataPtr = DataPersist;
}

void sCBufferBase::SetRegs()
{
  switch(Slot & sCBUFFER_SHADERMASK)
  {
  case sCBUFFER_VS:
    sSetVSParamImpl(RegStart,RegCount,(sVector4*)DataPersist);
    break;
  case sCBUFFER_PS:
    sSetPSParamImpl(RegStart,RegCount,(sVector4*)DataPersist);
    break;
  }
  // The pointer stays valid: the registers were just copied out, and callers
  // such as SimpleMtrlType_::BeginShow write through Data before calling
  // Modify(). Clearing it here segfaults them. DX9 dropped this for the same
  // reason -- "should be valid for more than one frame".
}

sCBufferBase::~sCBufferBase()
{
}

/****************************************************************************/
/***                                                                      ***/
/***   Initialisation                                                     ***/
/***                                                                      ***/
/****************************************************************************/

void PreInitGFX(sInt &flags,sInt &xs,sInt &ys)
{
#if sPLATFORM == sPLAT_WINDOWS
  
  if(flags & sISF_FULLSCREEN)
  {
	  DEVMODE dm;
    sClear(dm);
	  dm.dmSize=sizeof(dm);
	  dm.dmPelsWidth	= xs;
	  dm.dmPelsHeight	= ys;
	  dm.dmBitsPerPel	= 32;
	  dm.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

	  if(ChangeDisplaySettings(&dm,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
    {
	    dm.dmPelsWidth	= xs = 800;
	    dm.dmPelsHeight	= ys = 600;
	    if(ChangeDisplaySettings(&dm,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
      {
        sFatal(L"could not set displaymode %dx%d",xs,ys);
      }
    }
  }

#elif defined(__APPLE__)
  // Handled by Cocoa NSOpenGLPixelFormat
#elif sPLATFORM == sPLAT_LINUX
  
  static int attribList[] =
  {
    GLX_RGBA, GLX_DOUBLEBUFFER,
    GLX_RED_SIZE, 8,
    GLX_GREEN_SIZE, 8,
    GLX_BLUE_SIZE, 8,
    GLX_DEPTH_SIZE, 24,
    GLX_STENCIL_SIZE, 8,
    None
  };
  
  Display *dpy = sXDisplay();
  
  sXVisualInfo = glXChooseVisual(dpy,sXScreen,attribList);
  if(!sXVisualInfo)
    sFatal(L"glXChooseVisual returned 0!\n");
  
#endif
}

void InitGFX(sInt flags,sInt xs,sInt ys)
{
#if sPLATFORM == sPLAT_WINDOWS
  PIXELFORMATDESCRIPTOR pfd;
  int format;

  // get the device context (DC). Note: we don't use the forbidden GetDC(0)!

  GLDC = GetDC(sHWND);
  GLErr(GLDC);
  
  // set the pixel format for the DC

  sClear(pfd);
  pfd.nSize = sizeof(pfd);
  pfd.nVersion = 1;
  if(flags & sISF_2D)
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
  else
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = 24;
  pfd.cDepthBits = 24;
  pfd.iLayerType = PFD_MAIN_PLANE;

  GLErr(format = ChoosePixelFormat(GLDC,&pfd));
  GLErr(SetPixelFormat(GLDC,format,&pfd));
  
  // create and enable the render context (RC)

  GLErr(GLRC = wglCreateContext(GLDC));
  GLErr(wglMakeCurrent(GLDC,GLRC));

#elif defined(__APPLE__)
  extern void sMacInitGL();
  sMacInitGL();
#elif sPLATFORM == sPLAT_LINUX  
  
  Display *dpy = sXDisplay();
  GLXC = glXCreateContext(dpy,sXVisualInfo,0,True);
  if(!GLXC)
    sFatal(L"glXCreateContext failed!\n");
  
  if(!glXMakeCurrent(dpy,sXWnd,GLXC))
    sFatal(L"glXMakeCurrent failed!\n");
  
#endif
  
  // query extensions and other strings

  sString<256> Vendor;
  sString<256> Renderer;
  sString<256> Version;
  sString<256> Extension;
  sCopyString(Vendor  ,(const sChar8 *)glGetString(GL_VENDOR)  ,sCOUNTOF(Vendor  ));
  sCopyString(Renderer,(const sChar8 *)glGetString(GL_RENDERER),sCOUNTOF(Renderer));
  sCopyString(Version ,(const sChar8 *)glGetString(GL_VERSION ),sCOUNTOF(Version ));
  const GLubyte *ext = glGetString(GL_EXTENSIONS);

  sLogF(L"gfx",L"GL Vendor  <%s>\n",Vendor);
  sLogF(L"gfx",L"GL Renderer <%s>\n",Renderer);
  sLogF(L"gfx",L"GL Version  <%s>\n",Version);
  sLogF(L"gfx",L"GL Extensions:\n");
  while(*ext!=0)
  {
    sInt i=0;
    while(*ext==' ') ext++;
    while(*ext!=' ' && *ext!=0)
      Extension[i++] = *ext++;
    while(*ext==' ') ext++;
    Extension[i++] = 0;

    sLogF(L"gfx",L"<%s>\n",Extension);
  }
  sLogF(L"gfx",L".. end of extension list\n");

  // other initializaation

  DXScreenMode.ScreenX = xs;
  DXScreenMode.ScreenY = ys;
  DXScreenMode.Aspect = sF32(xs)/ys;
  sGFXViewRect.Init(0,0,xs,ys);

  sInitGfxCommon();
}

void ExitGFX()
{
  sExitGfxCommon();

#if sPLATFORM == sPLAT_WINDOWS
  wglMakeCurrent(0,0);
  wglDeleteContext(GLRC);
  ReleaseDC(sHWND,GLDC);
#elif defined(__APPLE__)
  extern void sMacExitGL();
  sMacExitGL();
#elif sPLATFORM == sPLAT_LINUX
  Display *dpy = sXDisplay();
  
  glXMakeCurrent(dpy,None,0);
  glXDestroyContext(dpy,GLXC);
#endif
}

void ResizeGFX(sInt x,sInt y)  // this is called when the windows size changes
{
  if(x && y && (x!=DXScreenMode.ScreenX || y!=DXScreenMode.ScreenY)) 
  {
    DXScreenMode.ScreenX = x;
    DXScreenMode.ScreenY = y;
  }
}


/****************************************************************************/
/***                                                                      ***/
/***   ScreenModes                                                        ***/
/***                                                                      ***/
/****************************************************************************/

sInt sGetDisplayCount()
{
  return 1;
}

void sGetScreenInfo(sScreenInfo &si,sInt flags,sInt display)
{
  si.Clear();
  si.Resolutions.HintSize(1);
  si.RefreshRates.HintSize(1);
  si.AspectRatios.HintSize(1);
  si.Resolutions.AddMany(1);
  si.RefreshRates.AddMany(1);
  si.AspectRatios.AddMany(1);

  si.Resolutions[0].x = 800;
  si.Resolutions[0].y = 600;
  si.RefreshRates[0] = 60;
  si.AspectRatios[0].x = 4;
  si.AspectRatios[0].y = 3;
  si.CurrentAspect = sF32(DXScreenMode.ScreenX)/DXScreenMode.ScreenY;
  si.CurrentXSize = DXScreenMode.ScreenX;
  si.CurrentYSize = DXScreenMode.ScreenY;
}

void sGetScreenMode(sScreenMode &sm)
{
  sm = DXScreenMode;
}

sBool sSetScreenMode(const sScreenMode &sm)
{
  DXScreenMode = sm;
  return 1;
}

void sGetScreenSafeArea(sF32 &xs, sF32 &ys)
{
#if sCONFIG_BUILD_DEBUG || sCONFIG_BUILD_DEBUGFAST
  xs = 0.95f;
  ys = 0.9f;
#elif sCONFIG_OPTION_LOCAKIT
  xs = 0.95f;
  ys = 0.95f;
#else
  xs=ys=1;
#endif
}

void sSetScreen(class sTexture2D *tex, sGrabFilterFlags filter, const sRect *dst, const sRect *src)
{
#if defined(__APPLE__)

  // The other half of sGrabScreen: put a texture back onto the window. This
  // was an empty stub, and the rendertarget manager builds every frame in an
  // offscreen "screen proxy" and finishes by copying it here -- so nothing a
  // post-processed scene drew ever reached the screen, and the proxy was never
  // released either, leaking one full-size rendertarget per frame.
  if(!tex || !tex->GLName) return;

  sRect s;
  if(src && !src->IsEmpty()) s = *src; else s.Init(0,0,tex->SizeX,tex->SizeY);
  sRect d;
  if(dst && !dst->IsEmpty()) d = *dst; else d.Init(0,0,DXScreenMode.ScreenX,DXScreenMode.ScreenY);
  if(s.IsEmpty() || d.IsEmpty()) return;

  extern sF32 sMacBackingScale();
  const sF32 sc = sMacBackingScale();

  // sGrabScreen copies window rows into the texture without flipping them in
  // GL's bottom-up space, so going back the same way needs no flip either.
  // Only the window side is measured in points and has to be scaled.
  const GLint dy0 = (GLint)((DXScreenMode.ScreenY - d.y1) * sc);
  const GLint dy1 = (GLint)((DXScreenMode.ScreenY - d.y0) * sc);

  GLint oldfb = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT,&oldfb);

  if(!sRTCopyFrameBuffer)
    glGenFramebuffersEXT(1,&sRTCopyFrameBuffer);
  glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT,sRTCopyFrameBuffer);
  glFramebufferTexture2DEXT(GL_READ_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,
    GL_TEXTURE_2D,tex->GLName,0);

  if(glCheckFramebufferStatusEXT(GL_READ_FRAMEBUFFER_EXT)==GL_FRAMEBUFFER_COMPLETE_EXT)
  {
    glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT,0);
    glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
    glDrawBuffer(GL_BACK);
    glDisable(GL_SCISSOR_TEST);
    glBlitFramebufferEXT(s.x0,s.y0,s.x1,s.y1,
                         (GLint)(d.x0*sc),dy0,(GLint)(d.x1*sc),dy1,
                         GL_COLOR_BUFFER_BIT,
                         (filter==sGFF_NONE) ? GL_NEAREST : GL_LINEAR);
  }

  glFramebufferTexture2DEXT(GL_READ_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D,0,0);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,oldfb);
  if(!oldfb) glReadBuffer(GL_BACK);
  while(glGetError()!=GL_NO_ERROR) {}

#endif
}

/****************************************************************************/
/***                                                                      ***/
/***   Rendering                                                          ***/
/***                                                                      ***/
/****************************************************************************/

sTexture2D *sGetCurrentBackBuffer(void)
{
  return 0;
}

sTexture2D *sGetCurrentBackZBuffer(void)
{
  return 0;
}

void sSetRendertarget(const sRect *vrp,sInt flags,sU32 clearcolor,sTexture2D **tex,sInt count)
{
  sVERIFY(0);
}


#if defined(__APPLE__)

/****************************************************************************/
/***                                                                      ***/
/***   Render targets (macOS)                                             ***/
/***                                                                      ***/
/****************************************************************************/

// This backend could only ever draw to the window: sSetRendertarget asserted
// that its texture argument was null and sSetRendertargetCube was an sFatal.
// wz4 leans on render-to-texture heavily -- post processing, shadow maps and
// the cube maps a modular material builds -- so loading any real project and
// showing a scene walked straight into the assert.

static GLuint sRTFrameBuffer = 0;               // one FBO, re-attached per use

enum { sRTMaxDepth = 8 };
static struct sRTDepthBuffer
{
  GLuint Name;
  sInt SizeX, SizeY;
} sRTDepth[sRTMaxDepth];
static sInt sRTDepthCount = 0;

// Depth buffers are cached by size: a frame usually alternates between a
// handful of target sizes, and reallocating on every switch would be silly.
static GLuint sRTGetDepthBuffer(sInt xs,sInt ys)
{
  for(sInt i=0;i<sRTDepthCount;i++)
    if(sRTDepth[i].SizeX==xs && sRTDepth[i].SizeY==ys)
      return sRTDepth[i].Name;

  GLuint name = 0;
  glGenRenderbuffersEXT(1,&name);
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,name);
  glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,GL_DEPTH_COMPONENT24,xs,ys);
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,0);

  if(sRTDepthCount < sRTMaxDepth)
  {
    sRTDepth[sRTDepthCount].Name = name;
    sRTDepth[sRTDepthCount].SizeX = xs;
    sRTDepth[sRTDepthCount].SizeY = ys;
    sRTDepthCount++;
  }
  return name;
}

// Rendering a shadow map means writing the depth into a texture rather than
// into the shared z buffer. The old style entry points below carry only a
// colour texture, so the caller names the depth one separately, just before.
static sTexture2D *sRTDepthTex = 0;

void sSetRendertargetDepth(sTexture2D *depth)
{
  sRTDepthTex = depth;
}

// Point the FBO at one texture image and make it the current target.
static void sRTBindTexture(GLuint tex,GLenum textarget,sInt xs,sInt ys,sInt mipmap)
{
  if(!sRTFrameBuffer)
    glGenFramebuffersEXT(1,&sRTFrameBuffer);

  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,sRTFrameBuffer);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,textarget,tex,mipmap);

  sTexture2D *dtex = sRTDepthTex;
  sRTDepthTex = 0;                      // names one target, not every later one
  if(dtex && dtex->GLName && dtex->SizeX==xs && dtex->SizeY==ys)
  {
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT,0);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,
      GL_TEXTURE_2D,dtex->GLName,0);
  }
  else
  {
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_TEXTURE_2D,0,0);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,
      GL_RENDERBUFFER_EXT,sRTGetDepthBuffer(xs,ys));
  }

  GLenum st = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
  if(st!=GL_FRAMEBUFFER_COMPLETE_EXT)
  {
    // Leaving an incomplete framebuffer bound makes every following GL call
    // raise GL_INVALID_FRAMEBUFFER_OPERATION, which the error check turns into
    // a fatal. Fall back to the window and drop the pending error instead.
    static sInt logged = 0;
    if(logged++ < 8)
      sLogF(L"gfx",L"rendertarget incomplete (0x%x), falling back to screen\n",(sInt)st);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
    while(glGetError()!=GL_NO_ERROR) {}
  }
}

static void sRTBindScreen()
{
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
}

// Shared tail of both entry points: viewport, scissor and the clear. A texture
// target is measured in its own texels, so the retina factor must not apply.
static void sRTSetup(const sRect *vrp,sInt xs,sInt ys,sBool istexture,sInt clearflags,sU32 clearcolor)
{
  sRect r;
  if(vrp)
    r = *vrp;
  else
    r.Init(0,0,xs,ys);

  sF32 sc = 1.0f;
  if(!istexture)
  {
    extern sF32 sMacBackingScale();
    sc = sMacBackingScale();
  }

  GLint x0 = (GLint)(r.x0 * sc);
  GLint x1 = (GLint)(r.x1 * sc);
  GLint y0 = (GLint)((ys - r.y1) * sc);
  GLint y1 = (GLint)((ys - r.y0) * sc);

  glScissor(x0,y0,x1-x0,y1-y0);
  glEnable(GL_SCISSOR_TEST);
  glViewport(x0,y0,x1-x0,y1-y0);

  sGFXRendertargetX = xs;
  sGFXRendertargetY = ys;
  sGFXRendertargetAspect = ys ? sF32(xs)/ys : 1;
  sGFXViewRect = r;

  sVector4 col;
  col.InitColor(clearcolor);
  glDepthMask(1);
  glColorMask(1,1,1,1);
  glStencilMask(1);
  glClearColor(col.x,col.y,col.z,col.w);
  glClearDepth(1.0f);

  sInt mode = 0;
  if(clearflags & sCLEAR_COLOR) mode |= GL_COLOR_BUFFER_BIT;
  if(clearflags & sCLEAR_ZBUFFER) mode |= GL_DEPTH_BUFFER_BIT;
  glClear(mode);
  if(StatsEnable) Stats.Clears++;

}

#endif

static void sSetRendertargetPrivate(const sRect *vrp,sInt flags,sU32 color)
{
}

void sSetRendertarget(const sRect *vrp, sInt clearflags, sU32 clearcolor)
{
  sSetRendertarget(vrp,0,clearflags,clearcolor);
}

void sSetRendertarget(const sRect *vrp,sTexture2D *tex, sInt clearflags, sU32 clearcolor)
{
#if defined(__APPLE__)
  if(tex)
  {
    sRTBindTexture(tex->GLName,GL_TEXTURE_2D,tex->SizeX,tex->SizeY,0);
    sRTSetup(vrp,tex->SizeX,tex->SizeY,sTRUE,clearflags,clearcolor);
    return;
  }
  sRTBindScreen();
#else
  sVERIFY(tex==0);
#endif


  // find rect

  sRect r;
  if(vrp)
    r = *vrp;
  else
    r.Init(0,0,DXScreenMode.ScreenX,DXScreenMode.ScreenY);

  // set scissor

#if defined(__APPLE__)
  // the rect is in window points, GL wants framebuffer pixels
  {
    extern sF32 sMacBackingScale();
    sF32 sc = sMacBackingScale();
    GLint x0 = (GLint)(r.x0 * sc);
    GLint x1 = (GLint)(r.x1 * sc);
    GLint y0 = (GLint)((DXScreenMode.ScreenY - r.y1) * sc);
    GLint y1 = (GLint)((DXScreenMode.ScreenY - r.y0) * sc);
    glScissor(x0,y0,x1-x0,y1-y0);
    glEnable(GL_SCISSOR_TEST);
    glViewport(x0,y0,x1-x0,y1-y0);
  }
#else
  glScissor(r.x0,DXScreenMode.ScreenY-r.y1,r.SizeX(),r.SizeY());
  glEnable(GL_SCISSOR_TEST);
  glViewport(r.x0,DXScreenMode.ScreenY-r.y1,r.SizeX(),r.SizeY());
#endif

  // clear

  sVector4 col;
  col.InitColor(clearcolor);
  glDepthMask(1);
  glColorMask(1,1,1,1);
  glStencilMask(1);
  glClearColor(col.x,col.y,col.z,col.w);
  glClearDepth(1.0f);

  sInt mode = 0;
  if(clearflags & sCLEAR_COLOR) mode |= GL_COLOR_BUFFER_BIT;
  if(clearflags & sCLEAR_ZBUFFER) mode |= GL_DEPTH_BUFFER_BIT;

  glClear(mode);
  if(StatsEnable) Stats.Clears++;

}

// Copy one texture into another. sCopyTexture() only had paths for "from the
// screen" and "to the screen"; anything else hit an sVERIFYFALSE, which is
// what a running timeline walks into as soon as an ipp node reuses a target.
// A private fbo holds the source so the render target currently bound by
// sSetRendertarget survives the copy untouched.

void sMacCopyTexture2D(sTexture2D *dest,sTexture2D *src,const sRect *dstr,const sRect *srcr)
{
  if(!dest || !src || !dest->GLName || !src->GLName) return;

  sRect s;
  if(srcr && !srcr->IsEmpty()) s = *srcr; else s.Init(0,0,src->SizeX,src->SizeY);
  sRect d;
  if(dstr && !dstr->IsEmpty()) d = *dstr; else d.Init(0,0,s.SizeX(),s.SizeY());

  sInt w = sMin(s.SizeX(),d.SizeX());
  sInt h = sMin(s.SizeY(),d.SizeY());
  w = sMin(w,dest->SizeX-d.x0);
  h = sMin(h,dest->SizeY-d.y0);
  if(w<=0 || h<=0) return;

  GLint oldfb = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT,&oldfb);

  if(!sRTCopyFrameBuffer)
    glGenFramebuffersEXT(1,&sRTCopyFrameBuffer);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,sRTCopyFrameBuffer);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,
    GL_TEXTURE_2D,src->GLName,0);

  if(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT)==GL_FRAMEBUFFER_COMPLETE_EXT)
  {
    glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
    glBindTexture(GL_TEXTURE_2D,dest->GLName);
    glCopyTexSubImage2D(GL_TEXTURE_2D,0,d.x0,d.y0,s.x0,s.y0,w,h);
    glBindTexture(GL_TEXTURE_2D,0);
  }

  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D,0,0);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,oldfb);
  if(!oldfb) glReadBuffer(GL_BACK);
  while(glGetError()!=GL_NO_ERROR) {}
}

void sSetRendertargetCube(sTextureCube *tex,sTexCubeFace cf,sInt clearflags, sU32 clearcolor)
{
#if defined(__APPLE__)
  if(tex)
  {
    sRTBindTexture(tex->GLName,GL_TEXTURE_CUBE_MAP_POSITIVE_X+cf,tex->SizeXY,tex->SizeXY,0);
    sRTSetup(0,tex->SizeXY,tex->SizeXY,sTRUE,clearflags,clearcolor);
    return;
  }
#endif
  sFatal(L"sSetRendertargetCube not implemented!");
}

void sGrabScreen(class sTexture2D *tex, sGrabFilterFlags filter, const sRect *dst, const sRect *src)
{
#if defined(__APPLE__)

  // Every post-processing effect starts by copying what has been rendered so
  // far into a texture; this was an sFatal, so adding any ipp node to a scene
  // killed the editor. glCopyTexSubImage2D reads straight out of the current
  // read buffer, which is what we want whether that is the window or an FBO.
  if(!tex || !tex->GLName) return;

  sRect s;
  if(src)
    s = *src;
  else
    s.Init(0,0,sMin(tex->SizeX,(sInt)DXScreenMode.ScreenX),sMin(tex->SizeY,(sInt)DXScreenMode.ScreenY));

  sRect d;
  if(dst)
    d = *dst;
  else
    d.Init(0,0,s.SizeX(),s.SizeY());

  sInt w = sMin(s.SizeX(),d.SizeX());
  sInt h = sMin(s.SizeY(),d.SizeY());
  if(w<=0 || h<=0) return;

  // the source rect is in window points, the framebuffer is in pixels
  extern sF32 sMacBackingScale();
  sF32 sc = 1.0f;
  GLint fb = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT,&fb);
  if(!fb) sc = sMacBackingScale();          // only the window is scaled

  GLint sx = (GLint)(s.x0 * sc);
  GLint sw = (GLint)(w * sc);
  GLint sh = (GLint)(h * sc);
  // GL counts from the bottom of whatever we are reading from
  GLint sy = (GLint)((DXScreenMode.ScreenY - s.y0 - h) * sc);
  if(fb) sy = s.y0;                         // an FBO has no window flip

  glBindTexture(GL_TEXTURE_2D,tex->GLName);
  glCopyTexSubImage2D(GL_TEXTURE_2D,0,d.x0,d.y0,sx,sy,sMin(sw,(GLint)tex->SizeX),sMin(sh,(GLint)tex->SizeY));
  glBindTexture(GL_TEXTURE_2D,0);

  (void)filter;

#else
  sFatal(L"sGrabScreen not implemented!");
#endif
}

void sBeginSaveRT(const sU8 *&data, sS32 &pitch, enum sTextureFlags &flags)
{
}

void sEndSaveRT()
{
}

// Reading a rendertarget back to the cpu. This was an sVERIFYFALSE, so the
// editor's Screenshot operator -- and anything else built on sSaveRT -- went
// down with it. glReadPixels needs the texture attached to a framebuffer, and
// hands back its rows bottom up, so they are turned over on the way into the
// buffer the caller reads.

static sU8 *sRTReadData = 0;

void sBeginReadTexture(const sU8*& data, sS32& pitch, enum sTextureFlags& flags,sTexture2D *tex)
{
#if defined(__APPLE__)

  data = 0;
  pitch = 0;
  flags = sTEX_ARGB8888;

  sEndReadTexture();
  if(!tex || !tex->GLName) return;

  const sInt xs = tex->SizeX;
  const sInt ys = tex->SizeY;
  if(xs<=0 || ys<=0) return;

  pitch = xs*4;
  sRTReadData = new sU8[pitch*ys];
  sSetMem(sRTReadData,0,pitch*ys);
  data = sRTReadData;

  GLint oldfb = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT,&oldfb);

  if(!sRTCopyFrameBuffer)
    glGenFramebuffersEXT(1,&sRTCopyFrameBuffer);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,sRTCopyFrameBuffer);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,
    GL_TEXTURE_2D,tex->GLName,0);

  if(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT)==GL_FRAMEBUFFER_COMPLETE_EXT)
  {
    glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
    glPixelStorei(GL_PACK_ALIGNMENT,1);

    // sTEX_ARGB8888 is one sU32 0xaarrggbb per pixel, which on a little endian
    // machine is the byte order GL_BGRA/GL_UNSIGNED_BYTE writes.
    sU8 *row = new sU8[pitch];
    glReadPixels(0,0,xs,ys,GL_BGRA,GL_UNSIGNED_BYTE,sRTReadData);
    for(sInt y=0;y<ys/2;y++)              // GL row 0 is the bottom one
    {
      sU8 *a = sRTReadData+y*pitch;
      sU8 *b = sRTReadData+(ys-1-y)*pitch;
      sCopyMem(row,a,pitch);
      sCopyMem(a,b,pitch);
      sCopyMem(b,row,pitch);
    }
    delete[] row;
  }

  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D,0,0);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,oldfb);
  if(!oldfb) glReadBuffer(GL_BACK);
  while(glGetError()!=GL_NO_ERROR) {}

#else
  sVERIFYFALSE;
#endif
}

void sEndReadTexture()
{
  sDeleteArray(sRTReadData);
}

void sRender3DEnd(sBool flip)
{
  sPreFlipHook->Call();
#if sPLATFORM == sPLAT_WINDOWS
//  SwapBuffers(GLDC);
#elif defined(__APPLE__)
  // The gui loop composites 2D over the 3D scene and flips itself, so honour
  // the flag instead of presenting a half-drawn frame here.
  if(flip)
  {
    extern void sMacSwapBuffers();
    sMacSwapBuffers();
  }
#elif sPLATFORM == sPLAT_LINUX
  glXSwapBuffers(sXDisplay(),sXWnd);
#endif
  
  BufferedStats = Stats;
  StatsEnable = 1;
  BufferedStats.GeoBuffersActive = 0;
  sDInt vmem = 0;
  for(sInt i=0;i<sGeoBufferCount;i++)
  {
    if(sGeoBuffers[i].Duration==sGD_STATIC)
      vmem += sGeoBuffers[i].Used;
    if(sGeoBuffers[i].Used>0)
      BufferedStats.GeoBuffersActive++;
  }
  BufferedStats.StaticVertexMem = vmem;
  sClear(Stats);

  sFlipMem();
  sPostFlipHook->Call();
}

void sRender3DFlush()
{
}

sBool sRender3DBegin()
{
  // prepare rendering

  sGFXRendertargetX = DXScreenMode.ScreenX;
  sGFXRendertargetY = DXScreenMode.ScreenY;
  sGFXRendertargetAspect = DXScreenMode.Aspect;
  sGFXViewRect.Init(0,0,DXScreenMode.ScreenX,DXScreenMode.ScreenY);
  // reset dynamic buffers

  for(sInt i=0;i<sGeoBufferCount;i++)
  {
    sGeoBuffer *geo = &sGeoBuffers[i];
    if(geo->Duration==sGD_STREAM || geo->Duration==sGD_FRAME)
    {
      sVERIFY(geo->Alloc);
      geo->Used = 0;
      geo->Freed = 0;
    }
  }

  // set some common gl-states

//	glEnable(GL_DEPTH_TEST);
//	glDisable(GL_DEPTH_TEST);
//	glDepthFunc(GL_LEQUAL);
  glDisable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  
  // clear caching

  return 1;
}

void sSetRenderClipping(sRect *r,sInt count)
{
}


void sGetGraphicsStats(sGraphicsStats &stat)
{
  stat = BufferedStats;
}

sInt sRenderStateTexture(sU32* data, sInt texstage, sU32 tflags)
{
  sLogF(L"gfx",L"sRenderStateTexture not implemented\n");
  return 0;
}

void sSetRenderStates(const sU32* data, sInt count)
{
  sLogF(L"gfx",L"sSetRenderStates not implemented\n");
}

sInt sRenderStateTexture(sU32* data, sInt texstage, sU32 tflags,sF32 lodbias)
{
  sFatal(L"sRenderStateTexture not implemented!");
  return 0;
}

void sSetTexture(sInt stage,class sTextureBase *tex)
{
  if(tex)
  {
    sVERIFY(tex->Mipmaps>0);
    switch(tex->Flags&sTEX_TYPE_MASK)
    {
      case sTEX_2D:
        glActiveTexture(GL_TEXTURE0+stage);
        glBindTexture(GL_TEXTURE_2D,tex->GLName);
        glEnable(GL_TEXTURE_2D);
        glDisable(GL_TEXTURE_3D);
        glDisable(GL_TEXTURE_CUBE_MAP);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,tex->Mipmaps-1);
        break;
      case sTEX_3D:
        glActiveTexture(GL_TEXTURE0+stage);
        glBindTexture(GL_TEXTURE_3D,tex->GLName);
        glEnable(GL_TEXTURE_3D);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_TEXTURE_CUBE_MAP);
        glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MAX_LEVEL,tex->Mipmaps-1);
        break;
      case sTEX_CUBE:
        glActiveTexture(GL_TEXTURE0+stage);
        glBindTexture(GL_TEXTURE_CUBE_MAP,tex->GLName);
        glEnable(GL_TEXTURE_CUBE_MAP);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_TEXTURE_3D);
        glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MAX_LEVEL,tex->Mipmaps-1);
        break;
    }
  }
  else
  {
//    glActiveTexture(GL_TEXTURE0+stage);

//    glDisable(GL_TEXTURE_2D);
//    glDisable(GL_TEXTURE_CUBE_MAP);
//    glDisable(GL_TEXTURE_3D);

//    glActiveTexture(GL_TEXTURE0);
  }
}

void sSetDesiredFrameRate(sF32 rate)
{
}

void sSetScreenGamma(sF32 gamma)
{
}

void sGetGraphicsCaps(sGraphicsCaps &caps)
{
  sClear(caps);
  caps.ShaderProfile = sSTF_NVIDIA;
  caps.UVOffset = 0.0f;
  caps.XYOffset = 0.0f;
  caps.RTUpsideDown = 1;

#if defined(__APPLE__)

  // The format masks were left at zero, and zero does not mean "ask the
  // driver" -- it means "supports nothing". A modular material asks
  // caps.Texture2D whether it may sample a texture at all and, being told no,
  // generates float4(0,0,0,0) in place of the fetch. That compiles and runs, so
  // nothing ever reported an error; the materials just sampled black. Every
  // format sMacTexFormat can map is sampleable, so let it answer.

  for(sInt fmt=1;fmt<64;fmt++)
  {
    GLint internal;
    GLenum channels,type;
    sBool compressed;
    if(!sMacTexFormat(fmt,internal,channels,type,compressed))
      continue;

    const sU64 bit = 1ULL<<fmt;
    const sBool isdepth = (channels==GL_DEPTH_COMPONENT);

    if(!isdepth)
    {
      caps.Texture2D |= bit;
      caps.TextureCube |= bit;
    }

    // Compressed textures cannot be rendered into; everything else can, the
    // formats that are not colour-renderable on their own being widened to
    // rgba when the storage is made (see sTexture2D::Create2).
    if(!compressed)
      caps.TextureRT |= bit;
  }

  // The depth formats are sampled through the shadow map path rather than as
  // ordinary textures, and read back as a single float.
  caps.Flags |= sGCF_DEPTHTEX_INTZ;

  // Vertex texture fetch is a shader model 3 feature and the driver may still
  // offer no units for it, so ask before promising anything.
  GLint vtunits = 0;
  glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB,&vtunits);
  while(glGetError()!=GL_NO_ERROR) {}
  if(vtunits>0)
    caps.VertexTex2D = caps.Texture2D;

  sLogF(L"gfx",L"caps tex2d %016x rt %016x vertex %016x (%d units)\n",
    caps.Texture2D,caps.TextureRT,caps.VertexTex2D,(sInt)vtunits);

#endif
}

/****************************************************************************/

/****************************************************************************/
/***                                                                      ***/
/***   Geometry Prim Interface                                            ***/
/***                                                                      ***/
/****************************************************************************/

void sGeometry::BeginQuadrics()
{
  sFatal(L"BeginQuadrics not implemented!");
}

void sGeometry::EndQuadrics()
{
  sFatal(L"EndQuadrics not implemented!");
}

void sGeometry::BeginQuad(void **data,sInt count)
{
  sFatal(L"BeginQuad not implemented!");
}

void sGeometry::EndQuad()
{
  sFatal(L"EndQuad not implemented!");
}

void sGeometry::BeginGrid(void **data,sInt xs,sInt ys)
{
  sFatal(L"BeginGrid not implemented!");
}

void sGeometry::EndGrid()
{
  sFatal(L"EndGrid not implemented!");
}


/****************************************************************************/
/***                                                                      ***/
/***   Geometry & GpuToCpu dynamic extensions                             ***/
/***                                                                      ***/
/****************************************************************************/

void sGeometry::Draw()
{
  Draw(0,0,0,0);
}

void sGeometry::Draw(const sGeometryDrawInfo &di)
{
  Draw((sDrawRange*)di.Ranges, di.RangeCount, di.InstanceCount, (sVertexOffset*)di.VertexOffset);
}

void sGeometry::Merge(sGeometry *geo0, sGeometry *geo1)
{
  if(geo0)
  {
    for(sInt i=0;i<sVF_STREAMMAX;i++)
      VertexPart[i] = geo0->VertexPart[i];
    IndexPart = geo0->IndexPart;
  }
  if(geo1)
  {
    for(sInt i=0;i<sVF_STREAMMAX;i++)
      VertexPart[i] = geo1->VertexPart[i];
    IndexPart = geo1->IndexPart;
  }
}

void sGeometry::MergeVertexStream(sInt DestStream,sGeometry *src,sInt SrcStream)
{
  sVERIFY(src!=this);
  VertexPart[DestStream] = src->VertexPart[SrcStream];
}

void sGeometry::InitDyn(sInt ic,sInt vc0,sInt vc1,sInt vc2,sInt vc3)
{
  if(ic) { IndexPart.Init(ic,IndexSize,sGD_DYNAMIC,1); IndexPart.Unlock(ic,IndexSize); }
  if(vc0) { VertexPart[0].Init(vc0,Format->GetSize(0),sGD_DYNAMIC,0); VertexPart[0].Unlock(vc0,Format->GetSize(0)); }
  if(vc1) { VertexPart[1].Init(vc1,Format->GetSize(1),sGD_DYNAMIC,0); VertexPart[1].Unlock(vc1,Format->GetSize(1)); }
  if(vc2) { VertexPart[2].Init(vc2,Format->GetSize(2),sGD_DYNAMIC,0); VertexPart[2].Unlock(vc2,Format->GetSize(2)); }
  if(vc3) { VertexPart[3].Init(vc3,Format->GetSize(3),sGD_DYNAMIC,0); VertexPart[3].Unlock(vc3,Format->GetSize(3)); }
}

void *sGeometry::BeginDynVB(sBool discard,sInt stream)
{
  void *ptr = 0;
  VertexPart[stream].Lock(&ptr);
  return ptr;
}

void *sGeometry::BeginDynIB(sBool discard)
{
  void *ptr = 0;
  IndexPart.Lock(&ptr);
  return ptr;
}

void sGeometry::EndDynVB(sInt stream)
{
  VertexPart[stream].Unlock(VertexPart[stream].Count, Format->GetSize(stream));
}

void sGeometry::EndDynIB()
{
  IndexPart.Unlock(IndexPart.Count, IndexSize);
}

sGpuToCpu::sGpuToCpu(sInt flags,sInt xs,sInt ys)
{
}

sGpuToCpu::~sGpuToCpu()
{
}

void sGpuToCpu::CopyFrom(sTexture2D *tex,sInt miplevel)
{
}

const void *sGpuToCpu::BeginRead(sDInt &pitch)
{
  pitch = 0;
  return 0;
}

void sGpuToCpu::EndRead()
{
}

class sMidiHandler_ *sMidiHandler = 0;


sTexture3D::sTexture3D(sInt xs, sInt ys, sInt zs, sU32 flags) {}
sTexture3D::~sTexture3D() {}
void sTexture3D::BeginLoad(sU8*& data, sInt& rpitch, sInt& spitch, sInt mipmap) { data = 0; rpitch = 0; spitch = 0; }
void sTexture3D::EndLoad() {}
void sTexture3D::Load(sU8 *data) {}

void sEnableGraphicsStats(sBool enable)
{
  if(!enable && StatsEnable)
  {
    DisabledStats = Stats;
    StatsEnable = 0;
  }
  if(enable && !StatsEnable)
  {
    Stats = DisabledStats;
    StatsEnable = 1;
  }
}

#endif // sRENDERER == sRENDER_OGL2

