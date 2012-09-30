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
#include "base/system.hpp"
#include "base/math.hpp"
#include "base/graphics.hpp"
#include "base/windows.hpp"
#include "base/serialize.hpp"

sInt sGFXRendertargetX=0;
sInt sGFXRendertargetY=0;
sF32 sGFXRendertargetAspect=1.0;
sALIGNED(sRect, sGFXViewRect, 16);

sHooks *sPreFlipHook = 0;
sHooks *sPostFlipHook = 0;
sHooks *sGraphicsLostHook = 0;


#if sRENDERER == sRENDER_DX11 || sRENDERER == sRENDER_DX9 || sRENDERER == sRENDER_OGLES2
#define RENDERTARGET_OLD_TO_NEW 1
#define RENDERTARGET_NEW_TO_OLD 0
#else // sRENDERER == sRENDER_OGL
#define RENDERTARGET_OLD_TO_NEW 0
#define RENDERTARGET_NEW_TO_OLD 1
#endif

/****************************************************************************/
// globals

#if sRENDERER!=sRENDER_DX11 && sRENDERER != sRENDER_OGLES2
// be sure to keep this 128 bytes aligned for fast clears on some platforms
sALIGNED(sCBufferBase *, CurrentCBs[sCBUFFER_MAXSLOT*sCBUFFER_SHADERTYPES], 128);
sALIGNED(sU64, CurrentCBsSlotMask[sCBUFFER_MAXSLOT*sCBUFFER_SHADERTYPES], 128);
sU64 CurrentCBsMask[sCBUFFER_SHADERTYPES] = { 0 };
#endif

/****************************************************************************/
// externs

extern sInt sSystemFlags;
void sCreateShader2(sShader *shader, sShaderBlob *code);
void sDeleteShader2(sShader *shader);

/****************************************************************************/
// forwards

void sInitShaders();
void sExitShaders();
void sFlushVertexFormat(sBool flush,void *user);

/****************************************************************************/
/***                                                                      ***/
/***   Initialisation                                                     ***/
/***                                                                      ***/
/****************************************************************************/

sVertexFormatHandle *sVertexFormatBasic;
sVertexFormatHandle *sVertexFormatStandard;
sVertexFormatHandle *sVertexFormatSingle;
sVertexFormatHandle *sVertexFormatDouble;
sVertexFormatHandle *sVertexFormatTangent;
sVertexFormatHandle *sVertexFormatTSpace;
sVertexFormatHandle *sVertexFormatTSpace4;
sVertexFormatHandle *sVertexFormatTSpace4_uv3;
sVertexFormatHandle *sVertexFormatTSpace4Anim;
sVertexFormatHandle *sVertexFormatInstance;
sVertexFormatHandle *sVertexFormatInstancePlus;

sU32 DeclBasic[]         = { sVF_POSITION,sVF_COLOR0,0 };
sU32 DeclStandard[]      = { sVF_POSITION,sVF_NORMAL,sVF_UV0,0 };
sU32 DeclSingle[]        = { sVF_POSITION,sVF_COLOR0,sVF_UV0,0 };
sU32 DeclDouble[]        = { sVF_POSITION,sVF_COLOR0,sVF_UV0,sVF_UV1,0 };
sU32 DeclTangent[]       = { sVF_POSITION,sVF_NORMAL,sVF_TANGENT|sVF_F3,sVF_UV0,0 };
sU32 DeclTSpace[]        = { sVF_POSITION,sVF_NORMAL,sVF_TANGENT|sVF_F3,sVF_COLOR0,sVF_UV0,sVF_UV1,0 };
sU32 DeclTSpace4[]       = { sVF_POSITION,sVF_NORMAL,sVF_TANGENT|sVF_F4,sVF_COLOR0,sVF_UV0,sVF_UV1,0 };
sU32 DeclTSpace4_uv3[]   = { sVF_POSITION,sVF_NORMAL,sVF_TANGENT|sVF_F4,sVF_COLOR0,sVF_UV0,sVF_UV1,sVF_UV2,0 };
sU32 DeclTSpace4Anim[]   = { sVF_POSITION,sVF_NORMAL,sVF_TANGENT|sVF_F4,sVF_BONEINDEX|sVF_I4,sVF_BONEWEIGHT|sVF_C4,sVF_COLOR0,sVF_UV0,sVF_UV1,0 };
sU32 DeclInstance[]      = { sVF_STREAM1|sVF_UV5|sVF_F4|sVF_INSTANCEDATA , sVF_STREAM1|sVF_UV6|sVF_F4|sVF_INSTANCEDATA , sVF_STREAM1|sVF_UV7|sVF_F4|sVF_INSTANCEDATA,0 };
sU32 DeclInstancePlus[]  = { sVF_STREAM1|sVF_UV5|sVF_F4|sVF_INSTANCEDATA , sVF_STREAM1|sVF_UV6|sVF_F4|sVF_INSTANCEDATA , sVF_STREAM1|sVF_UV7|sVF_F4|sVF_INSTANCEDATA, sVF_STREAM1|sVF_UV4|sVF_F4|sVF_INSTANCEDATA, 0 };

/****************************************************************************/

void sInitGfxCommon()
{
  sInitShaders();
  sAddMemMarkCallback(sFlushVertexFormat);

  sVertexFormatBasic    = sCreateVertexFormat(DeclBasic);
  sVertexFormatStandard = sCreateVertexFormat(DeclStandard);
  sVertexFormatSingle   = sCreateVertexFormat(DeclSingle);
  sVertexFormatDouble   = sCreateVertexFormat(DeclDouble);
  sVertexFormatTangent   = sCreateVertexFormat(DeclTangent);
  sVertexFormatTSpace   = sCreateVertexFormat(DeclTSpace);
  sVertexFormatTSpace4  = sCreateVertexFormat(DeclTSpace4);
  sVertexFormatTSpace4_uv3  = sCreateVertexFormat(DeclTSpace4_uv3);
  sVertexFormatTSpace4Anim  = sCreateVertexFormat(DeclTSpace4Anim);
  sVertexFormatInstance = sCreateVertexFormat(DeclInstance);
  sVertexFormatInstancePlus = sCreateVertexFormat(DeclInstancePlus);
  sPreFlipHook = new sHooks;
  sPostFlipHook = new sHooks;
  sGraphicsLostHook = new sHooks;
}

void sExitGfxCommon()
{
  sDelete(sPostFlipHook);
  sDelete(sPreFlipHook);
  sDelete(sGraphicsLostHook);
  sDestroyAllVertexFormats();
  sExitShaders();
}

/****************************************************************************/

sScreenMode::sScreenMode()
{
  Clear();
}

void sScreenMode::Clear()
{
  Flags = 0;
  Display = -1;
  ScreenX = 800;
  ScreenY = 600;
  OverX = 0;
  OverY = 0;
  Aspect = 0;
  MultiLevel = -1;
  OverMultiLevel = -1;
  RTZBufferX = 0;
  RTZBufferY = 0;
  Frequency = 0;
}

sScreenInfo::sScreenInfo()
{
  Clear();
}

void sScreenInfo::Clear()
{
  CurrentXSize = 0;
  CurrentYSize = 0;
  CurrentAspect = 0;
  MultisampleLevels = 0;
  Resolutions.Clear();
  RefreshRates.Clear();
  AspectRatios.Clear();
  MonitorName.Clear();
}

#if sRENDERER!=sRENDER_DX9
void sDbgPaintWireFrame(sBool enable)
{
}
#endif

/****************************************************************************/
/***                                                                      ***/
/***   old screenmode interface                                           ***/
/***                                                                      ***/
/****************************************************************************/

void sSetScreenResolution(sInt xs,sInt ys)
{
  sScreenMode sm;
  sGetScreenMode(sm);

  sm.ScreenX = xs;
  sm.ScreenY = ys;

  sSetScreenMode(sm);
}

void sSetScreenResolution(sInt xs,sInt ys, sF32 aspectRatio)
{
  sScreenMode sm;
  sGetScreenMode(sm);

  sm.ScreenX = xs;
  sm.ScreenY = ys;
  sm.Aspect  = aspectRatio;

  sSetScreenMode(sm);
}

void sGetScreenSize(sInt &xs,sInt &ys)
{
  sScreenMode sm;
  sGetScreenMode(sm);

  if(sm.OverX && sm.OverY)
  {
    xs = sm.OverX;
    ys = sm.OverY;
  }
  else
  {
    xs = sm.ScreenX;
    ys = sm.ScreenY;
  }
}

sF32 sGetScreenAspect()
{
  sScreenMode sm;
  sGetScreenMode(sm);

  return sm.Aspect;
}

void sEnlargeZBufferRT(sInt x,sInt y)
{
  sScreenMode sm;
  sGetScreenMode(sm);

  if(x>sm.RTZBufferX || y>sm.RTZBufferY)
  {
    sInt nx = sMax(sm.RTZBufferX,x);
    sInt ny = sMax(sm.RTZBufferY,y);
    sLogF(L"gfx",L"zbufferrt enlarged, this is very bad. %dx%d -> %dx%d\n",sm.RTZBufferX,sm.RTZBufferY,nx,ny);
    sCreateZBufferRT(nx,ny);
  }
}

#if sRENDERER!=sRENDER_DX9
void sCreateZBufferRT(sInt xs,sInt ys)
{
  sScreenMode sm;
  sGetScreenMode(sm);

  sm.RTZBufferX = xs;
  sm.RTZBufferY = ys;

  sSetScreenMode(sm);
}
#endif

void sDestroyZBufferRT()
{
  sCreateZBufferRT(0,0);
}


/****************************************************************************/

sBool sGetFullscreen()
{
  sScreenMode sm;
  sGetScreenMode(sm);
  return (sm.Flags&sSM_FULLSCREEN)!=0;
}

void sSetFullscreen(sBool enable)
{
  sScreenMode sm;
  sGetScreenMode(sm);

  if (enable)
    sm.Flags = sm.Flags | sSM_FULLSCREEN;
  else
    sm.Flags = sm.Flags & ~sSM_FULLSCREEN;

  sSetScreenMode(sm);
}

void sGetScreenResolution(sInt &xs, sInt &ys)
{
  sGetScreenSize(xs,ys);
}

/****************************************************************************/
/***                                                                      ***/
/***   Shaders                                                            ***/
/***                                                                      ***/
/****************************************************************************/

template <class streamer> void Serialize_(streamer &);

void sShaderBlob::SetNext(sInt type)
{
  sVERIFY(Type != sSTF_NONE);
  sShaderBlob *blob=(sShaderBlob*)(Data+sAlign(Size,4));
  blob->Type = type;
}

sShaderBlob *sShaderBlob::Next()
{
  if(!Type)
    return 0;
  sShaderBlob *blob=(sShaderBlob*)(Data+sAlign(Size,4));
  if(blob->Type!=sSTF_NONE)
    return blob;
  return 0;
}

sShaderBlob *sShaderBlob::Get(sInt type)
{
  sShaderBlob *blob = this;
  while(blob && blob->Type!=type)
    blob = blob->Next();
  return blob;
}

sShaderBlob *sShaderBlob::GetAny(sInt kind, sInt platform)
{
  sShaderBlob *blob = this;
  while(blob && (blob->Type&0xff00)!=((kind|platform)&0xff00))
    blob = blob->Next();
  return blob;
}

void sSerializeShaderBlob(sU8 *data, sInt &size, sReader &s)
{
  sInt readsize=0;
  s | readsize;
  sVERIFY(!(size&3));
  sVERIFY(readsize<size);
  size = readsize;

  sShaderBlob *blob = 0;
  if(size)
  {
    blob = (sShaderBlob*)data;
    s | blob->Type;
  }
  while(blob && blob->Type != sSTF_NONE)
  {
    s | blob->Size;
    s.ArrayU8(blob->Data,blob->Size);
    s.Align();
    blob = (sShaderBlob*) (blob->Data+sAlign(blob->Size,4));
    s | blob->Type;
  }
}

void sSerializeShaderBlob(const sU8 *data, sInt size, sWriter &s)
{
  s | size;
  sShaderBlob *blob = 0;
  if(size)
  {
    blob = (sShaderBlob*)data;
    s | blob->Type;
  }
  while(blob && blob->Type != sSTF_NONE)
  {
    s | blob->Size;
    s.ArrayU8(blob->Data,blob->Size);
    s.Align();
    blob = (sShaderBlob*) (blob->Data+sAlign(blob->Size,4));
    s | blob->Type;
  }
}

/****************************************************************************/

static sDList<sShader,&sShader::Node> *Shaders=0;

void sInitShaders()
{
  Shaders = new sDList<sShader,&sShader::Node>;
}

void sExitShaders()
{
  sDelete(Shaders);
}

sShader *sCreateShaderRaw(sInt type,const sU8 *code,sInt bytes)
{
  sVERIFY(type & sSTF_PLATFORM);        // no more automatic setting of shader platform!
//  type |= sGetShaderPlatform();
  sU32 hash;
  sShader *sh;

  sChecksumAdler32Begin();

  sShaderBlob header;
  header.Type = type;
  header.Size = bytes;

  sChecksumAdler32Add((sU8*)&header,8);
  sChecksumAdler32Add(code,bytes);
  sU8 tmp0 = 0;
  sInt tmp1 = sSTF_NONE;
  for(sInt i=bytes&3;(bytes+i)&3;i++)
    sChecksumAdler32Add(&tmp0,1);
  sChecksumAdler32Add((sU8*)&tmp1,4);

  hash = sChecksumAdler32End();

  sInt cmpbytes = ((bytes+3)&~3)+12;
  sFORALL_LIST(*Shaders,sh)
  {
    if(type==sh->Type && hash==sh->Hash && cmpbytes==sh->Size)
    {
      sShaderBlob *blob = (sShaderBlob*)sh->Data;
      if(blob->Type == header.Type && !blob->Next() && sCmpMem(blob->Data,code,bytes)==0)
      {
        sh->AddRef();
        return sh;
      }
    }
  }

  sScopeMem(sAMF_HEAP);   // shaders are shared and need to be allocated on heap
  return new sShader(type,code,bytes,hash,sTRUE);
}

sShader *sCreateShader(sInt type,const sU8 *code,sInt bytes)
{
  sVERIFY(type & sSTF_PLATFORM);        // no more automatic setting of shader platform!
  sU32 hash;
  sShader *sh;

  hash = sChecksumAdler32(code,bytes);

  sFORALL_LIST(*Shaders,sh)
  {
    if(type==sh->Type && hash==sh->Hash && bytes==sh->Size)
    {
      if(sCmpMem(sh->Data,code,bytes)==0)
      {
        sh->AddRef();
        return sh;
      }
    }
  }

  sScopeMem(sAMF_HEAP);   // shaders are shared and need to be allocated on heap
  return new sShader(type,code,bytes,hash);
}

void sShader::Bind2(sVertexFormatHandle *vformat, sShader *pshader)
{
}
sShader *sShader::Bind(sVertexFormatHandle *vformat, sShader *pshader)
{
  AddRef();
  return this;
}

sShader::sShader(sInt type,const sU8 *data,sInt length,sU32 hash,sBool raw/*=sFALSE*/)
{
  Temp = -1;
#if sRENDERER == sRENDER_DX9
  vs = 0;
  ps=0;
#endif
#if sRENDERER == sRENDER_OGL2
  GLName = 0;
#endif

  sVERIFY(Shaders);
  Shaders->AddTail(this);

  if(raw)
  {
    sInt blen = ((length+3)&~3)+12;
    Data = new sU8[blen];
    sShaderBlob *blob = (sShaderBlob*)Data;
    blob->Type = type;
    blob->Size = length;

    Size = blen;
    Type = type;
    Hash = hash;
    UseCount = 1;

    sCopyMem(blob->Data,data,length);
    for(sInt i=length&3;(length+i)&3;i++)
      blob->Data[length+i] = 0;
    blob = (sShaderBlob*)(blob->Data+((length+3)&~3));
    blob->Type = sSTF_NONE;
  }
  else
  {
    Data = new sU8[length];
    Size = length;
    Type = type;
    Hash = hash;
    UseCount = 1;

    sCopyMem(Data,data,length);
  }
  sShaderBlob *blob = Blob->GetAny(type,sGetShaderPlatform());
  if(!blob)
  {
    sLog(L"gfx",L"warning: shader creation failed, no suitable shader found\n");
  }
  else
  {
    sCreateShader2(this,blob);
  }
}

sShader::~sShader()
{
  sVERIFY(Shaders);
  Shaders->Rem(this);
  sDeleteShader2(this);
  delete[] Data;
}

void sShader::AddRef()
{
  UseCount++;
}

void sShader::Release()
{
  if(this!=0) 
  {
    UseCount--;
    if(UseCount<=0)
      delete this;
  }
}

sBool sShader::CheckKind(sInt type)
{
  return (type&sSTF_KIND)==(Type&sSTF_KIND);
}

const sU8 *sShader::GetCode(sInt &bytes)
{
  bytes = Size;
  return Data;
}

/****************************************************************************/

// obsolete

void sReleaseShader(sShader * sh)
{
  sh->Release();
}

void sAddRefShader(sShader * sh)
{
  sh->AddRef();
}

sBool sCheckShader(sShader * sh,sInt type)
{
  return sh->CheckKind(type);
}

const sU8 *sGetShaderCode(sShader * sh,sInt &bytes)
{
  return sh->GetCode(bytes);
}

// obsolete

void sAddRefVS(sShader * vsh)
{
  vsh->AddRef();
}

void sAddRefPS(sShader * psh)
{
  psh->AddRef();
}

sShader * sCreateVS(const sU32 *data,sInt count)
{
  return sCreateShader(sSTF_VERTEX,(const sU8 *) data,count*4);
}

sShader * sCreatePS(const sU32 *data,sInt count)
{
  return sCreateShader(sSTF_PIXEL,(const sU8 *) data,count*4);
}

void sDeleteVS(sShader * &handle)
{
  handle->Release();
  handle = 0;
}

void sDeletePS(sShader * &handle)
{
  handle->Release();
  handle = 0;
}

sBool sValidVS(sShader * vsh)
{
  return vsh->CheckKind(sSTF_VERTEX);
}

sBool sValidPS(sShader * psh)
{
  return psh->CheckKind(sSTF_PIXEL);
}

/****************************************************************************/
/***                                                                      ***/
/***   sVertexFormatHandle                                                ***/
/***                                                                      ***/
/****************************************************************************/

#define MAXVFHASH 64


const sInt sVertexFormatTypeSizes[]=
{
  0,  // invalid
  8,  // sVF_F2
  12, // sVF_F3
  16, // sVF_F4
  4,  // sVF_C4,
  4,  // sVF_I4,
  4,  // sVF_S2
  8,  // sVF_S4
  4,  // sVF_H2
  8,  // sVF_H4
  4,  // sVF_F1
  6,  // sVF_H3
};

sVertexFormatHandle *VertexFormatHashTable[MAXVFHASH];


sVertexFormatHandle *sCreateVertexFormat(const sU32 *disc)
{
  sScopeMem(sAMF_HEAP);   // vertex formats are global objects and need to be allocated on heap
 
  sInt count;
  sVertexFormatHandle *handle;
  sU32 data[32];
  sInt hash;
  static const sInt defaulttypes[32] = 
  {
    0,sVF_F3,sVF_F3,sVF_F4,
    sVF_C4,sVF_C4,0,0,
    sVF_C4,sVF_C4,sVF_C4,sVF_C4,
    sVF_F2,sVF_F2,sVF_F2,sVF_F2,
  };
  sVERIFYSTATIC(sVF_USEMASK==31); 

  // count elements and fill default values and copy discriptor
  // count includes trailing zero.

  for(count=0;disc[count];count++)
  {
    sVERIFY(count<31);
    data[count] = disc[count];

    if((data[count] & sVF_TYPEMASK)==0)
      data[count] |= defaulttypes[data[count]&sVF_USEMASK];
  }
  data[count++] = 0;

  // find in hashtable

  hash = sChecksumCRC32((sU8 *)data,count*4) & (MAXVFHASH-1);
  handle = VertexFormatHashTable[hash];
  while(handle)
  {
    if(handle->Count==count && sCmpMem(data,handle->Data,count*4)==0)
    {
      handle->UseCount++;
      return handle;
    }
    handle = handle->Next;
  }

  // not found, create new element

  handle = new sVertexFormatHandle;
  handle->Data = new sU32[count];
  handle->Count = count;
  handle->UseCount = 1;
  handle->AvailMask = 0;
  handle->Streams = 1;
  handle->IsMemMarkSet = sIsMemMarkSet();

  for(sInt i=0;i<sVF_STREAMMAX;i++)
    handle->VertexSize[i] = 0;
  handle->Next = 0;
  sCopyMem(handle->Data,data,count*4);

  // link in hashtable

  handle->Next = VertexFormatHashTable[hash];
  VertexFormatHashTable[hash] = handle;

  // prepare drawing

  handle->Create();

  // done

  return handle;
}

#if sRENDERER==sRENDER_DX9
void sFlushOccQueryNodes();
#endif

void sFlushVertexFormat(sBool flush,void *user)
{
  if(flush)
  {
    sVertexFormatHandle *handle,*next;
    sLogF(L"gfx",L"flushing vertex format handles\n");
    for(sInt i=0;i<MAXVFHASH;i++)
    {
      handle = VertexFormatHashTable[i];
      VertexFormatHashTable[i] = 0;
      while(handle)
      {
        next = handle->Next;

        if(handle->IsMemMarkSet)
        {
          handle->Destroy();
          delete handle->Data;
          delete handle;
        }
        else
        {
          handle->Next = VertexFormatHashTable[i];
          VertexFormatHashTable[i] = handle;
        }

        handle = next;
      }
    }
#if sRENDERER==sRENDER_DX9
    sFlushOccQueryNodes();
#endif
  }
  sLogF(L"gfx",L"%d registered shaders\n",Shaders->GetCount());
}

void sStreamVertexFormat(sWriter &s,const sVertexFormatHandle *vhandle)
{
  s | vhandle->Count;
  s.ArrayU32(vhandle->Data,vhandle->Count);
}

void sStreamVertexFormat(sReader &s,sVertexFormatHandle *&vhandle)
{
  sU32 vd[16];
  sU32 count;
  
  s | count;
  sVERIFY(count < 16);
  s.ArrayU32(vd,count);

  vhandle = sCreateVertexFormat(vd);
}

void sDestroyAllVertexFormats()
{
  sVertexFormatHandle *h,*n;
  for(sInt i=0;i<MAXVFHASH;i++)
  {
    h = VertexFormatHashTable[i];
    while(h)
    {
      n = h->Next;

      h->Destroy();

      delete[] h->Data;
      delete h;
      h = n;
    }
  }
  sClear(VertexFormatHashTable);
}

sInt sVertexFormatHandle::GetOffset(sInt semantic_and_format)
{
  sInt offset = 0;
  sInt i = 0;
  const sU32 *data = GetDesc();
  while(data[i])
  {
    sVERIFY((data[i]&sVF_STREAMMASK)==0);

    if( (data[i]&(sVF_USEMASK|sVF_TYPEMASK)) == (sU32)semantic_and_format)
      return offset;

    switch(data[i]&sVF_TYPEMASK)
    {
      case sVF_F2:  offset+=2*4;  break;
      case sVF_F3:  offset+=3*4;  break;
      case sVF_F4:  offset+=4*4;  break;
      case sVF_I4:  offset+=1*4;  break;
      case sVF_C4:  offset+=1*4;  break;
      case sVF_S2:  offset+=1*4;  break;
      case sVF_S4:  offset+=2*4;  break;
      case sVF_H2:  offset+=2*2;  break;
      case sVF_H3:  offset+=3*2;  break;
      case sVF_H4:  offset+=4*2;  break;
      case sVF_F1:  offset+=1*4;  break;
      default: sVERIFYFALSE;
    }

    i++;
  }

  return -1;
}

sInt sVertexFormatHandle::GetDataType(sInt semantic)const
{
  semantic &= sVF_USEMASK;
  const sU32 *desc = Data;
  while(*desc)
  {
    sU32 tmp = *desc++;
    if((tmp&sVF_USEMASK)==(sU32)semantic)
    {
      return sInt(tmp&sVF_TYPEMASK);
    }
  }
  sFatal(L"semantic %02x not available",semantic);
  return 0;
}


/****************************************************************************/
/***                                                                      ***/
/***   sGeometry                                                          ***/
/***                                                                      ***/
/****************************************************************************/

sGeometryDrawInfo::sGeometryDrawInfo()
{
  Flags = 0;
  RangeCount = 0;
  Ranges = 0;
  InstanceCount = 0;
  sClear(VertexOffset);
  BlendFactor = 0;
  Indirect = 0;
}

sGeometryDrawInfo::sGeometryDrawInfo(sDrawRange *ir,sInt irc,sInt instancecount, sVertexOffset *off)
{
  Flags = 0;
  BlendFactor = 0;
  Indirect = 0;

  if(ir && irc)
  { 
    Flags |= sGDI_Ranges;
    RangeCount = irc;
    Ranges = ir;
  }
  else
  {
    RangeCount = 0;
    Ranges = 0;
  }
  if(instancecount)
  {
    Flags |= sGDI_Instances;
    InstanceCount = instancecount;
  }
  else
  {
    InstanceCount = 0;
  }
  if(off)
  {
    Flags |= sGDI_VertexOffset;
    for(sInt i=0;i<sVF_STREAMMAX;i++)
      VertexOffset[i] = off->VOff[i];
  }
  else
  {
    sClear(VertexOffset);
  }
}

/****************************************************************************/

sGeometry::sGeometry()
{
  Flags = 0;
  Format = 0;
  IndexSize = 0;
  PrimMode = 0;
  FirstPrim = 0;
  CurrentPrim = 0;
  DebugBreak = 0;
  MorphTargetId = ~0;
  InitPrivate();
}

sGeometry::sGeometry(sInt flags,sVertexFormatHandle *hnd)
{
  Flags = 0;
  Format = 0;
  IndexSize = 0;
  PrimMode = 0;
  FirstPrim = 0;
  CurrentPrim = 0;
  DebugBreak = 0;

  InitPrivate();

  Init(flags,hnd);
}

sGeometry::~sGeometry()
{
  Clear();

  ExitPrivate();
}

#if sRENDERER!=sRENDER_DX11 && sRENDERER!=sRENDER_OGLES2 

void sGeometry::Clear()
{
  for(sInt i=0;i<sVF_STREAMMAX;i++)
    VertexPart[i].Clear();
  IndexPart.Clear();
}

void sGeometry::InitPrivate() {}
void sGeometry::ExitPrivate() {}
#endif

void sGeometry::Serialize(sReader &s)
{
  sInt version = s.Header(sSerId::sGeometry,1);
  if(version>0)
  {
    sInt vc,ic;
    sInt vs,is;
    sInt flags;
    sU32 *ip=0;
    sU32 *vp=0;
    sVertexFormatHandle *vhandle;

    s | vs | is |vc | ic | flags;
    sStreamVertexFormat(s,vhandle);

    flags &= sGF_PRIMMASK|sGF_INDEXMASK;
    if(!(flags & sGF_INDEX32))
      flags |= sGF_INDEX16;

    BeginLoad(vhandle,flags,sGD_STATIC,vc,ic,&vp,&ip);
    s.ArrayU8((sU8 *)vp,vs*vc);  // not endian safe
    s.Align(4);
    s.ArrayU8((sU8 *)ip,is*ic);  // not endian safe
    s.Align(4);
    EndLoad();

    s.Footer();
  }
}

/****************************************************************************/

#if sRENDERER==sRENDER_OGL2 || sRENDERER==sRENDER_BLANK

void sGeometry::Init(sInt flags,sVertexFormatHandle *form)
{
  sVERIFY(!(flags&sGF_CPU_MEM));
  Flags = flags;
  Format = form;

  switch(Flags & sGF_INDEXMASK)
  {
    case sGF_INDEX16:  IndexSize = 2; break;
    case sGF_INDEX32:  IndexSize = 4; break;
    default: IndexSize = 0;
  }
}

void sGeometry::BeginLoadIB(sInt ic,sGeometryDuration duration,void **ip)
{
  sVERIFY(IndexSize>0)
  IndexPart.Clear();
#if sRENDERER==sRENDER_DX9 || sRENDERER==sRENDER_DX11
  IndexPart.Init(ic,IndexSize,duration,(Flags & sGF_INDEX32)?2:1);
#else
  IndexPart.Init(ic,IndexSize,duration,1);
#endif
  IndexPart.Lock(ip);
}

void sGeometry::EndLoadIB(sInt ic)
{
  IndexPart.Unlock(ic,IndexSize);
}

void sGeometry::BeginLoadVB(sInt vc,sGeometryDuration duration,void **vp,sInt stream)
{
  VertexPart[stream].Clear();
  VertexPart[stream].Init(vc,Format->GetSize(stream),duration,0);
  VertexPart[stream].Lock(vp);
}

void sGeometry::EndLoadVB(sInt vc,sInt stream)
{
  VertexPart[stream].Unlock(vc,Format->GetSize(stream));
}

void sGeometry::BeginLoad(sVertexFormatHandle *vf,sInt flags,sGeometryDuration duration,sInt vc,sInt ic,void **vp,void **ip)
{
  Init(flags,vf);
  BeginLoadVB(vc,duration,vp,0);
  if(ic)
    BeginLoadIB(ic,duration,ip);
  else
    IndexPart.Clear();
}

void sGeometry::EndLoad(sInt vc,sInt ic)
{
  VertexPart[0].Unlock(vc,Format->GetSize(0));
  if(IndexPart.Buffer)
    IndexPart.Unlock(ic,IndexSize);
}

// obsolete

void sGeometry::BeginLoad(sInt vc,sInt ic,sInt flags,sVertexFormatHandle *vf,void **vp,void **ip)
{
  if(!(flags & sGF_INDEX32))
    flags |= sGF_INDEX16;

  Init(flags&(sGF_PRIMMASK|sGF_INDEXMASK|sGF_INSTANCES),vf);
  BeginLoadVB(vc,sGeometryDuration(flags&3),vp,0);
  if(ic)
    BeginLoadIB(ic,sGeometryDuration((flags>>4)&3),ip);
  else
    IndexPart.Clear();
}
#endif //  renderer

/****************************************************************************/

// now this is cheesy...

void sGeometry::LoadCube(sU32 c0,sF32 sx,sF32 sy,sF32 sz,sGeometryDuration gd)
{
  struct fat
  {
    sF32 px,py,pz;
    sF32 nx,ny,nz;
    sF32 u,v;
  };
  fat src[24] =
  {
    {-1, 1,-1,  0, 0,-1, 1,0},
    { 1, 1,-1,  0, 0,-1, 1,1},
    { 1,-1,-1,  0, 0,-1, 0,1},
    {-1,-1,-1,  0, 0,-1, 0,0},

    {-1,-1, 1,  0, 0, 1, 1,0},
    { 1,-1, 1,  0, 0, 1, 1,1},
    { 1, 1, 1,  0, 0, 1, 0,1},
    {-1, 1, 1,  0, 0, 1, 0,0},

    {-1,-1,-1,  0,-1, 0, 1,0},
    { 1,-1,-1,  0,-1, 0, 1,1},
    { 1,-1, 1,  0,-1, 0, 0,1},
    {-1,-1, 1,  0,-1, 0, 0,0},

    { 1,-1,-1,  1, 0, 0, 1,0},
    { 1, 1,-1,  1, 0, 0, 1,1},
    { 1, 1, 1,  1, 0, 0, 0,1},
    { 1,-1, 1,  1, 0, 0, 0,0},

    { 1, 1,-1,  0, 1, 0, 1,0},
    {-1, 1,-1,  0, 1, 0, 1,1},
    {-1, 1, 1,  0, 1, 0, 0,1},
    { 1, 1, 1,  0, 1, 0, 0,0},

    {-1, 1,-1, -1, 0, 0, 1,0},
    {-1,-1,-1, -1, 0, 0, 1,1},
    {-1,-1, 1, -1, 0, 0, 0,1},
    {-1, 1, 1, -1, 0, 0, 0,0},
  };

  sF32 *fp;
  const sU32 *desc;

  sx = sx*0.5f;
  sy = sy*0.5f;
  sz = sz*0.5f;
    
  BeginLoadVB(24,gd,&fp);
  for(sInt i=0;i<24;i++)
  {
    desc = GetFormat()->GetDesc();
    while(*desc!=sVF_END)
    {
      sU32 d = *desc++;
      if((d & sVF_STREAMMASK)==0)
      {
        switch(d)
        {
        case sVF_POSITION|sVF_F3:
          *fp++ = src[i].px*sx;
          *fp++ = src[i].py*sy;
          *fp++ = src[i].pz*sz;
          break;
        case sVF_NORMAL|sVF_F3:
          *fp++ = src[i].nx;
          *fp++ = src[i].ny;
          *fp++ = src[i].nz;
          break;
        case sVF_NORMAL|sVF_I4:
          ((sU8 *)fp)[0] = sU8(src[i].nx*127.f+128.f);
          ((sU8 *)fp)[1] = sU8(src[i].ny*127.f+128.f);
          ((sU8 *)fp)[2] = sU8(src[i].nz*127.f+128.f);
          ((sU8 *)fp)[3] = 0;
          fp++;
          break;
        case sVF_TANGENT|sVF_F3:
          *fp++ = 0;
          *fp++ = 0;
          *fp++ = 0;
          break;
        case sVF_TANGENT|sVF_F4:
          *fp++ = 0;
          *fp++ = 0;
          *fp++ = 0;
          *fp++ = 1;
          break;
        case sVF_UV0|sVF_F2:
          *fp++ = src[i].u;
          *fp++ = src[i].v;
          break;
        case sVF_UV1|sVF_F2:
          *fp++ = 0;
          *fp++ = 0;
          break;
        case sVF_COLOR0|sVF_C4:
          *(sU32 *)fp = c0;
          fp++;
          break;
        default:
          sVERIFYFALSE;
        }
      }
    }
  }
  EndLoadVB();

  if(Flags == (sGF_QUADLIST|sGF_INDEXOFF))
  {
  }
  else if(Flags == (sGF_TRILIST|sGF_INDEX16) || Flags == (sGF_PATCHLIST|sGF_INDEX16|3))
  {
    sU16 *ip;
    BeginLoadIB(6*6,gd,&ip);
    for(sInt i=0;i<6;i++)
      sQuad(ip,i*4,0,1,2,3);
    EndLoadIB();
  }
  else if(Flags == (sGF_TRILIST|sGF_INDEX32) || Flags == (sGF_PATCHLIST|sGF_INDEX32|3))
  {
    sU32 *ip;
    BeginLoadIB(6*6,gd,&ip);
    for(sInt i=0;i<6;i++)
      sQuad(ip,i*4,0,1,2,3);
    EndLoadIB();
  }
  else
  {
    sVERIFYFALSE;
  }
}

void sGeometry::LoadTorus(sInt tx,sInt ty,sF32 ro,sF32 ri,sGeometryDuration gd,sU32 c0)
{
  sF32 *fp;
  const sU32 *desc;

  BeginLoadVB((tx+1)*(ty+1),gd,&fp);
  for(sInt y=0;y<ty+1;y++)
  {
    sF32 fy = y*sPI2F/ty;
    for(sInt x=0;x<tx+1;x++)
    {
      sF32 fx = x*sPI2F/tx;

      desc = GetFormat()->GetDesc();
      while(*desc!=sVF_END)
      {
        sU32 d = *desc++;
        if((d & sVF_STREAMMASK)==0)
        {
          switch(d)
          {
          case sVF_POSITION|sVF_F3:
            *fp++ = -sFCos(fy)*(ro+sFSin(fx)*ri);
            *fp++ = sFCos(fx)*ri;
            *fp++ = sFSin(fy)*(ro+sFSin(fx)*ri);
            break;
          case sVF_NORMAL|sVF_F3:
            *fp++ = -sFCos(fy)*sFSin(fx);
            *fp++ = sFCos(fx);
            *fp++ = sFSin(fy)*sFSin(fx);
            break;
          case sVF_NORMAL|sVF_I4:
            {
              sF32 nx = -sFCos(fy)*sFSin(fx);
              sF32 ny = sFCos(fx);
              sF32 nz = sFSin(fy)*sFSin(fx);
              ((sU8 *)fp)[0] = sU8(nx*127.f+128.f);
              ((sU8 *)fp)[1] = sU8(ny*127.f+128.f);
              ((sU8 *)fp)[2] = sU8(nz*127.f+128.f);
              ((sU8 *)fp)[3] = 0;
            }
            fp++;
            break;
          case sVF_TANGENT|sVF_F3:
            *fp++ = sFSin(fy);
            *fp++ = 0;
            *fp++ = sFCos(fy);
            break;
          case sVF_TANGENT|sVF_F4:
            *fp++ = sFSin(fy);
            *fp++ = 0;
            *fp++ = sFCos(fy);
            *fp++ = 1;
            break;
          case sVF_UV0|sVF_F2:
            *fp++ = sF32(x)/tx;
            *fp++ = sF32(y)/ty;
            break;
          case sVF_UV1|sVF_F2:
            *fp++ = sF32(x)/tx;
            *fp++ = sF32(y)/ty;
            break;
          case sVF_COLOR0|sVF_C4:
            *(sU32 *)fp = c0;
            fp++;
            break;
          default:
            switch(*desc++ & sVF_TYPEMASK)
            {
            case sVF_F4: 
              *fp++ = 0;
            case sVF_F3: 
              *fp++ = 0;
            case sVF_F2: 
              *fp++ = 0;
            case sVF_F1: 
              *fp++ = 0;
              break;

            case sVF_H4:
            case sVF_S4:
              *(sU32 *)fp = 0;
              fp++;
            case sVF_C4:
            case sVF_I4:
            case sVF_H2:
            case sVF_S2:
            case sVF_SN_11_11_10:
              *(sU32 *)fp = 0;
              fp++;
              break;
            default:
            case sVF_H3:
              sVERIFYFALSE;
              break;
            }
          }
        }
      }
    }
  }
  EndLoadVB();

  if(Flags == (sGF_TRILIST|sGF_INDEX16) || Flags == (sGF_PATCHLIST|sGF_INDEX16|3))
  {
    sU16 *ip;
    BeginLoadIB(tx*ty*6,gd,&ip);
    for(sInt y=0;y<ty;y++)
      for(sInt x=0;x<tx;x++)
        sQuad(ip,0, 
          (y+0)*(tx+1) + (x+0),
          (y+0)*(tx+1) + (x+1),
          (y+1)*(tx+1) + (x+1),
          (y+1)*(tx+1) + (x+0));
    EndLoadIB();
  }
  else if(Flags == (sGF_TRILIST|sGF_INDEX32) || Flags == (sGF_PATCHLIST|sGF_INDEX32|3))
  {
    sU32 *ip;
    BeginLoadIB(tx*ty*6,gd,&ip);
    for(sInt y=0;y<ty;y++)
      for(sInt x=0;x<tx;x++)
        sQuad(ip,0, 
          (y+0)*(tx+1) + (x+0),
          (y+0)*(tx+1) + (x+1),
          (y+1)*(tx+1) + (x+1),
          (y+1)*(tx+1) + (x+0));
    EndLoadIB();
  }
  else
  {
    sVERIFYFALSE;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   sTexture                                                           ***/
/***                                                                      ***/
/****************************************************************************/

sInt sReadTextureSkipLevels(sInt skiplevels)
{
  return 0;
}

/****************************************************************************/

sInt sGetBitsPerPixel(sInt format)
{
  sInt BitsPerPixel = 0;

  switch(format&sTEX_FORMAT)
  {
    case sTEX_ARGB8888: BitsPerPixel = 32;  break;
    case sTEX_QWVU8888: BitsPerPixel = 32;  break;
    
    case sTEX_GR16:     BitsPerPixel = 32;  break;
    case sTEX_ARGB16:   BitsPerPixel = 64;  break;

    case sTEX_R32F:     BitsPerPixel = 32;  break;
    case sTEX_GR32F:    BitsPerPixel = 64;  break;
    case sTEX_ARGB32F:  BitsPerPixel = 128; break;
    case sTEX_R16F:     BitsPerPixel = 16;  break;
    case sTEX_GR16F:    BitsPerPixel = 32;  break;
    case sTEX_ARGB16F:  BitsPerPixel = 64;  break;

    case sTEX_A8:       BitsPerPixel =  8;  break;
    case sTEX_I8:       BitsPerPixel =  8;  break;
    case sTEX_DXT1:     BitsPerPixel =  4;  break;
    case sTEX_DXT1A:    BitsPerPixel =  4;  break;
    case sTEX_DXT3:     BitsPerPixel =  8;  break;
    case sTEX_DXT5:     BitsPerPixel =  8;  break;
    case sTEX_DXT5N:    BitsPerPixel =  8;  break;

    case sTEX_MRGB8:    BitsPerPixel = 32;  break;
    case sTEX_MRGB16:   BitsPerPixel = 64;  break;

    case sTEX_ARGB2101010: BitsPerPixel=32; break;
    case sTEX_D24S8:    BitsPerPixel = 32;  break;

//    case sTEX_A4:       BitsPerPixel =  4; break;
    case sTEX_I4:       BitsPerPixel =  4; break;
    case sTEX_IA4:      BitsPerPixel =  8; break;
    case sTEX_IA8:      BitsPerPixel = 16; break;
    case sTEX_RGB5A3:   BitsPerPixel = 16; break;

    case sTEX_INDEX4:   BitsPerPixel =  4; break;
    case sTEX_INDEX8:   BitsPerPixel =  8; break;

    case sTEX_ARGB1555: BitsPerPixel = 16; break;
    case sTEX_ARGB4444: BitsPerPixel = 16; break;
    case sTEX_RGB565:   BitsPerPixel = 16; break;
    case sTEX_GR8:      BitsPerPixel = 16; break;

    case sTEX_DEPTH16NOREAD:  BitsPerPixel = 16; break;
    case sTEX_DEPTH24NOREAD:  BitsPerPixel = 32; break;
    case sTEX_PCF16:    BitsPerPixel = 16; break;
    case sTEX_PCF24:    BitsPerPixel = 32; break;
    case sTEX_DEPTH16:  BitsPerPixel = 16; break;
    case sTEX_DEPTH24:  BitsPerPixel = 32; break;

    case sTEX_DXT5_AYCOCG: BitsPerPixel =  8;  break;

    case sTEX_8TOIA:       BitsPerPixel =  8;  break;

    case sTEX_UINT32:   BitsPerPixel = 32;  break;

    default: sVERIFYFALSE;
  }
  return BitsPerPixel;
}

/****************************************************************************/

sBool sIsBlockCompression(sInt texflags)
{
  switch(texflags&sTEX_FORMAT)
  {
  case sTEX_DXT1:
  case sTEX_DXT1A:
  case sTEX_DXT3:
  case sTEX_DXT5:
  case sTEX_DXT5N:
  case sTEX_DXT5_AYCOCG:
    return 1;
  default:
    return 0;
  }
}


sU32 sAYCoCgtoARGB(sU32 val);
sU32 sARGBtoAYCoCg(sU32 val)
{
  sInt a = (val>>24)&0xff;
  sInt r = (val>>16)&0xff;
  sInt g = (val>>8 )&0xff;
  sInt b = (val    )&0xff;

  sInt Co = r-b;
  sInt t = b+(Co>>1);
  sInt Cg = g-t;
  sInt Y = t+(Cg>>1);
  Y = sClamp(Y,0,255);
  Co = sClamp(Co,0,255);
  Cg = sClamp(Cg,0,255);

  sU32 result = (sU32(a)<<24)|(sU32(Y)<<16)|(sU32(Co)<<8)|Cg;
  return result;
}

sU32 sAYCoCgtoARGB(sU32 val)
{
  sInt a = (val>>24)&0xff;
  sInt Y =  (val>>16)&0xff;
  sInt Co = (val>>8 )&0xff;
  sInt Cg = (val    )&0xff;

  sInt t = Y-(Cg>>1);
  sInt g = Cg+t;
  sInt b = t-(Co>>1);
  sInt r = Co+b;

  r = sClamp(r,0,255);
  g = sClamp(g,0,255);
  b = sClamp(b,0,255);

  sU32 result = (sU32(a)<<24)|(sU32(r)<<16)|(sU32(g)<<8)|b;
  return result;
}


/****************************************************************************/

sTextureBase::sTextureBase()
{
  Loading = -1;
  LockFlags = -1;

  Temp = 0;
  LoadBuffer = 0;

  Flags = 0;
  Mipmaps = 0;
  BitsPerPixel = 0;
  SizeX = 0;
  SizeY = 0;
  SizeZ = 0;

  FrameRT = 0xffff;
  SceneRT = 0xffff;
#if sRENDERER==sRENDER_OGL2
  GLName = 0;
#endif

  NameId = 0;
   
  InitPrivate();
}

sTextureBase::~sTextureBase()
{
  sVERIFY(LoadBuffer==0);     // if this is not 0, then EndLoad() has not been called!
  sTextureProxy *pr;
  while(!Proxies.IsEmpty())
  {
    pr=Proxies.GetHead()->Proxy;
    sVERIFY(pr->Link==this);
    pr->Disconnect();
  }
  Proxies.Clear();
  ExitPrivate();
}

void sTextureBase::InitPrivate() {}
void sTextureBase::ExitPrivate() {}

/****************************************************************************/

sTexture2D::sTexture2D()
{
  Flags = sTEX_2D;
#if sRENDERER==sRENDER_OGL2
  GLName = 0;
  GLFBName = 0;
  LoadMipmap = 0;
#endif
}

sTexture2D::sTexture2D(sInt xs,sInt ys,sU32 flags,sInt mipmaps)
{
  Flags = sTEX_2D;
  Init(xs,ys,flags|sTEX_2D,mipmaps);
}

sTexture2D::~sTexture2D()
{
  Clear();
}

void sTexture2D::Clear()
{
  sVERIFY(LoadBuffer==0);
  Destroy2();
  SizeX = 0;
  SizeY = 0;
  SizeZ = 0;
  Flags = sTEX_2D;
  Mipmaps = 0;
  Loading = -1;
  BitsPerPixel = 0;
}

void sTexture2D::ReInit(sInt xs, sInt ys, sInt flags,sInt mipmaps)
{
  // modify parameters 
  if(flags & sTEX_SCREENSIZE)
    sFatal(L"can not reinit a texture with sTEX_SCREENSIZE");

  sInt realmip = 0;
  if(mipmaps==0) mipmaps=32;
  while(realmip<mipmaps && xs>=(1<<realmip) && ys>=(1<<realmip))
    realmip++;
  if(flags & sTEX_NOMIPMAPS)
    realmip = 1;

  // texture changed?

  if(SizeX!=xs || SizeY!=ys || Flags!=((flags&~sTEX_TYPE_MASK)|sTEX_2D) || Mipmaps!=realmip)
  {
    Init(xs,ys,flags,mipmaps);
  }
}


void sTexture2D::Init(sInt xs, sInt ys, sInt flags,sInt mipmaps, sBool force/*=sFALSE*/)
{
  sInt realmip;
  sVERIFY(LoadBuffer==0);

  // use screen size

  OriginalSizeX = xs;
  OriginalSizeY = ys;

  if(flags & sTEX_SCREENSIZE)
  {
    sGetScreenSize(xs,ys);
    xs = xs >> OriginalSizeX;
    ys = ys >> OriginalSizeY;
  }

  // calculate "real" mipmap count

  realmip = 0;
  sInt shift=0;
  switch(flags&sTEX_FORMAT)
  { case sTEX_DXT1:
    case sTEX_DXT1A:
    case sTEX_DXT3:
    case sTEX_DXT5:
    case sTEX_DXT5N:
    case sTEX_DXT5_AYCOCG:
      shift=2;
      break;
  }

  if(mipmaps==0) mipmaps=32;
  while(realmip<mipmaps && xs>=((1<<realmip)<<shift) && ys>=((1<<realmip)<<shift))
    realmip++;
  if(flags & sTEX_NOMIPMAPS)
    realmip = 1;

  // early out: nothing to do!

  if(xs==SizeX && ys==SizeY && flags==Flags && realmip==Mipmaps && !force) 
    return;

  // clear (possible) old texture

  Clear();

  // set new parameters

  SizeX = xs;
  SizeY = ys;
  SizeZ = 1;
  Flags = (flags&~sTEX_TYPE_MASK)|sTEX_2D;
  Mipmaps = realmip;
  Loading = -1;
  BitsPerPixel = sGetBitsPerPixel(Flags);

  // create resource
  sPushMemLeakDesc(L"sTexture2D");
  Create2(flags);
  sPopMemLeakDesc();
}

void sTexture2D::LoadAllMipmaps(sU8 *source)
{
  sU8 *dest;
  sU8 *p;
  sInt pitch_source,pitch_dest;
  sInt xs,ys;
  sInt blockSize;
  sInt level;

  xs = SizeX;
  ys = SizeY;
  blockSize = BitsPerPixel/8;
  switch(Flags & sTEX_FORMAT)
  {
  case sTEX_DXT1:
  case sTEX_DXT1A:
    xs /= 4;
    ys /= 4;
    blockSize = 8;
    break;
  case sTEX_DXT3:
  case sTEX_DXT5:
  case sTEX_DXT5N:
  case sTEX_DXT5_AYCOCG:
    xs /= 4;
    ys /= 4;
    blockSize = 16;
    break;
  }
  level = 0;
  p = source;

  for(;;)
  {
    // copy mipmap
    pitch_source = xs * blockSize;
    BeginLoad(dest,pitch_dest,level);

    sVERIFYRELEASE(pitch_source<=pitch_dest);
    for(sInt y=0;y<ys;y++)
    {
      sCopyMem(dest,p,pitch_source);
      dest += pitch_dest;
      p += pitch_source;
    }
    EndLoad();

    // check if next;

    level++;
    if(level>=Mipmaps) break;
    xs = xs/2;
    ys = ys/2;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   sTextureCube                                                       ***/
/***                                                                      ***/
/****************************************************************************/

sTextureCube::sTextureCube()
{
  Flags = sTEX_CUBE;
  SizeXY = 0;
}

sTextureCube::sTextureCube(sInt dim, sInt flags,sInt mipmaps/*=0*/)
{
  Flags = sTEX_CUBE;
  SizeXY = 0;
  Init(dim,flags,mipmaps);
}

sTextureCube::~sTextureCube()
{
  Clear();
}

/****************************************************************************/

void sTextureCube::Init(sInt dim, sInt flags, sInt mipmaps/*=0*/, sBool force/*=sFALSE*/)
{
  sInt realmip;
  sVERIFY(LoadBuffer==0);

  // calculate "real" mipmap count
  realmip = 0;
  if(mipmaps==0) mipmaps=32;
  while(realmip<mipmaps && dim>=(1<<realmip))
    realmip++;
  if(flags & sTEX_NOMIPMAPS)
    realmip = 1;

  // early out: nothing to do!
  if(dim==SizeXY && flags==Flags && realmip==Mipmaps && !force) 
    return;

  // clear (possible) old texture
  Clear();

  // set new parameters
  SizeXY = dim;
  SizeX = dim;
  SizeY = dim;
  SizeZ = 6;
  Flags = (flags&~sTEX_TYPE_MASK)|sTEX_CUBE;
  Mipmaps = realmip;
  Loading = -1;
  BitsPerPixel = sGetBitsPerPixel(Flags);
  LockedFace = sTCF_NONE;

  // create resource
  sPushMemLeakDesc(L"sTextureCube");
  Create2(flags);
  sPopMemLeakDesc();
}

void sTextureCube::Clear()
{
  sVERIFY(LoadBuffer==0);
  Destroy2();
  SizeXY = 0;
  Flags = sTEX_CUBE;
  Mipmaps = 0;
  Loading = -1;
  BitsPerPixel = 0;
  LockedFace = sTCF_NONE;
}

/****************************************************************************/

void sTextureCube::LoadAllMipmaps(sU8 *data)
{
  sU8 *dest;
  sU8 *p;
  sInt pitch_source,pitch_dest;
  sInt level;

  sInt initsize = SizeXY;
  pitch_source = BitsPerPixel*initsize/8;
  switch(Flags & sTEX_FORMAT)
  {
  case sTEX_DXT1:
  case sTEX_DXT1A:
    initsize /= 4;
    pitch_source = 8*initsize;
    break;
  case sTEX_DXT3:
  case sTEX_DXT5:
  case sTEX_DXT5N:
  case sTEX_DXT5_AYCOCG:
    initsize /= 4;
    pitch_source = 16*initsize;
    break;
  }
  p = data;

  sInt pitch_init = pitch_source;

  for(sInt f=0;f<6;f++)
  {
    level = 0;
    sInt size = initsize;
    pitch_source = pitch_init;

    for(;;)
    {
      // copy mipmap
      BeginLoad(sTexCubeFace(f),dest,pitch_dest,level);
      for(sInt y=0;y<size;y++)
      {
        sCopyMem(dest,p,pitch_source);
        dest += pitch_dest;
        p += pitch_source;
      }
      EndLoad();

      // check if next;

      level++;
      if(level>=Mipmaps) break;
      size = size/2;
      pitch_source = pitch_source/2;
    }

    p = sAlign(p,4);
  }
}


/****************************************************************************/
/***                                                                      ***/
/***   sTextureProxy                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sTextureProxy::sTextureProxy()
{
  Link = 0;
  Node.Proxy = this;
  Flags = sTEX_PROXY;
}

sTextureProxy::~sTextureProxy()
{
  Disconnect();
}

void sTextureProxy::GetSize(sInt &xs,sInt &ys,sInt &zs)
{
  if(Link)
  {
    Link->GetSize(xs,ys,zs);
  }
  else
  {
    xs = ys = zs = 0;
  }
}

void sTextureProxy::Connect(sTextureBase *tex)
{
  if(Link)
  {
    Disconnect2();

    Link->Proxies.Rem(&Node);
    Link = 0;

    Flags = sTEX_PROXY;
    Mipmaps = 0;
    BitsPerPixel = 0;
    SizeX = 0;
    SizeY = 0;
    SizeZ = 0;
  }

  while (tex && tex->Flags&sTEX_PROXY)
  {
    tex = ((sTextureProxy*)tex)->Link;
  }

  Link = tex;
  if(Link)
  {
    Flags = Link->Flags|sTEX_PROXY;
    Mipmaps = Link->Mipmaps;
    BitsPerPixel = Link->BitsPerPixel;
    SizeX = Link->SizeX;
    SizeY = Link->SizeY;
    SizeZ = Link->SizeZ;

    Link->Proxies.AddTail(&Node);
    Connect2();
  }
}

#if sRENDERER!=sRENDER_DX9 && sRENDERER!=sRENDER_DX11 && sRENDERER!=sRENDER_OGLES2 && sRENDERER!=sRENDER_BLANK

void sTextureProxy::Connect2()
{
  sFatal(L"sTextureProxy not supported");
}

void sTextureProxy::Disconnect2()
{
  sFatal(L"sTextureProxy not supported");
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   sMaterial                                                          ***/
/***                                                                      ***/
/****************************************************************************/

sInt sConvertOldUvFlags(sInt flags)
{
  sInt tmp = (flags&~0xf0);

  switch(flags&0xf0)
  {
  case 0x00:  return sMTF_TILE|tmp;
  case 0x10:  return sMTF_CLAMP|tmp;
  case 0x20:  return sMTF_MIRROR|tmp;
  case 0x30:  return sMTF_BORDER_BLACK|tmp;
  case 0x40:  return sMTF_BORDER_WHITE|tmp;
  case 0x50:  return sMTF_TILEU_CLAMPV|tmp;
  default:
    sFatal(L"sConvertOldUvFlags: unknown flags %08x\n",flags);
  }
  return flags;
}

/****************************************************************************/

sMaterialEnv::sMaterialEnv()
{
  for(sInt i=0;i<sMTRLENV_LIGHTS;i++)
  {
    LightDir[i].Init(0,0,0);
    LightColor[i] = 0;
  }
  AmbientColor = 0xffffffff;

  FogMax = 1.0f;
  FogStart = 75.0f;
  FogEnd = 1024.0f;
  FogDensity = 1.0f;
  FogColor = 0xffffffff;

  DOFBlurNear = 0.0f;
  DOFFocusNear = 1.0f;
  DOFFocusFar = 20.0f;
  DOFBlurFar = 40.0f;
  DOFBlurMax = 0.75f;
}

void sMaterialEnv::Fix()
{
}

/****************************************************************************/

sMaterialRS::sMaterialRS()
{
  Flags = sMTRL_ZON|sMTRL_CULLON;
  FuncFlags[0] = sMFF_LESSEQUAL;
  FuncFlags[1] = sMFF_ALWAYS;
  FuncFlags[2] = sMFF_ALWAYS;
  FuncFlags[3] = 0;
  BlendColor = sMB_OFF;
  BlendAlpha = sMB_SAMEASCOLOR;
  BlendFactor = 0xffffffff;
  StencilOps[0] = sMSO_KEEP;
  StencilOps[1] = sMSO_KEEP;
  StencilOps[2] = sMSO_KEEP;
  StencilOps[3] = sMSO_KEEP;
  StencilOps[4] = sMSO_KEEP;
  StencilOps[5] = sMSO_KEEP;
  StencilRef = 0;
  AlphaRef = 10;
  sClear(pad);
  StencilMask = 255;
  for(sInt i=0;i<sMTRL_MAXTEX;i++)
  {
    TFlags[i] = 0;
    LodBias[i] = 0.0f;
  }
}

sBool sMaterialRS::operator==(const sMaterialRS& rs)const
{
  if(Flags!=rs.Flags)
    return sFALSE;
  if(FuncFlags[0]!=rs.FuncFlags[0]||FuncFlags[1]!=rs.FuncFlags[1]||FuncFlags[2]!=rs.FuncFlags[2]||FuncFlags[3]!=rs.FuncFlags[3])
    return sFALSE;
  if(BlendColor!=rs.BlendColor)
    return sFALSE;
  if(BlendAlpha!=rs.BlendAlpha)
    return sFALSE;
  if(BlendFactor!=rs.BlendFactor)
    return sFALSE;
  if(StencilOps[0]!=rs.StencilOps[0]||StencilOps[1]!=rs.StencilOps[1]||StencilOps[2]!=rs.StencilOps[2]||StencilOps[3]!=rs.StencilOps[3]||StencilOps[4]!=rs.StencilOps[4]||StencilOps[5]!=rs.StencilOps[5])
    return sFALSE;
  if(StencilRef!=rs.StencilRef)
    return sFALSE;
  if(StencilMask!=rs.StencilMask)
    return sFALSE;
  if(AlphaRef!=rs.AlphaRef)
    return sFALSE;
  for(sInt i=0;i<sMTRL_MAXTEX;i++)
  {
    if(TFlags[i]!=rs.TFlags[i])
      return sFALSE;
    if(LodBias[i]!=rs.LodBias[i])
      return sFALSE;
  }
  return sTRUE;
}

template <class streamer> static inline void SerializeMaterialRSOld(sMaterialRS &rs, streamer &s)
{  
  sVERIFY(sMTRL_MAXTEX>=8);
  s.U32(rs.Flags);
  s.ArrayU8(rs.FuncFlags,4);
  s.ArrayU32(rs.TFlags,8);
  s.U32(rs.BlendColor);
  s.U32(rs.BlendAlpha);
  s.U32(rs.BlendFactor);
  s.ArrayU8(rs.StencilOps,6);
  s.Align(4);
  s.U32(rs.StencilRef);
  s.U32(rs.StencilMask);
  s.U32(rs.AlphaRef);
  s.ArrayF32(rs.LodBias,8);
};

template <class streamer> static inline void SerializeMaterialRS(sMaterialRS &rs, streamer &s)
{
  sInt version = 2;
  sInt texcount = sMTRL_MAXTEX; 
  s | version;
  if (version>=2)
    s | texcount;
  else
    texcount=16;  // old format always had 16 texture slots
  s.U32(rs.Flags);
  s.ArrayU8(rs.FuncFlags,4);
  s.ArrayU32(rs.TFlags,sMin<sInt>(texcount,sMTRL_MAXTEX));
  if (texcount>sMTRL_MAXTEX) 
    s.Skip((texcount-sMTRL_MAXTEX)*sizeof(sU32));
  s.U32(rs.BlendColor);
  s.U32(rs.BlendAlpha);
  s.U32(rs.BlendFactor);
  s.ArrayU8(rs.StencilOps,6);
  s.Align(4);
  s.U32(rs.StencilRef);
  s.U32(rs.StencilMask);
  s.U32(rs.AlphaRef);
  s.ArrayF32(rs.LodBias,sMin<sInt>(texcount,sMTRL_MAXTEX));
  if (texcount>sMTRL_MAXTEX) 
    s.Skip((texcount-sMTRL_MAXTEX)*sizeof(sF32));
};

void sMaterialRS::SerializeOld(sReader &s) { SerializeMaterialRSOld(*this,s); }
void sMaterialRS::SerializeOld(sWriter &s) { SerializeMaterialRSOld(*this,s); }

void sMaterialRS::Serialize(sReader &s) { SerializeMaterialRS(*this,s); }
void sMaterialRS::Serialize(sWriter &s) { SerializeMaterialRS(*this,s); }

/****************************************************************************/

sMaterial::sMaterial()
{
  NameId = 0;
  Flags = sMTRL_ZON|sMTRL_CULLON;
  FuncFlags[0] = sMFF_LESSEQUAL;
  FuncFlags[1] = sMFF_ALWAYS;
  FuncFlags[2] = sMFF_ALWAYS;
  FuncFlags[3] = 0;
  BlendColor = sMB_OFF;
  BlendAlpha = sMB_SAMEASCOLOR;
  BlendFactor = 0xffffffff;
  StencilOps[0] = sMSO_KEEP;
  StencilOps[1] = sMSO_KEEP;
  StencilOps[2] = sMSO_KEEP;
  StencilOps[3] = sMSO_KEEP;
  StencilOps[4] = sMSO_KEEP;
  StencilOps[5] = sMSO_KEEP;
  StencilRef = 0;
  AlphaRef = 10;
  sClear(pad);
  StencilMask = 255;
  VertexShader = 0;
  PixelShader = 0;
#if sRENDERER==sRENDER_DX11
  HullShader = 0;
  DomainShader = 0;
  GeometryShader = 0;
  ComputeShader = 0;
#endif

  StateVariants = 0;
  VariantFlags = 0;

#if sPLATFORM == sPLAT_WINDOWS 
  PreparedFormat = 0;
#endif
#if sRENDERER==sRENDER_DX9
  StateCount = 0;
  States = 0;
#endif
#if sRENDERER==sRENDER_DX9 || sRENDERER==sRENDER_DX11
  for(sInt i=0;i<sMTRL_MAXTEX;i++)
    TBind[i] = i | sMTB_PS;
#endif

  for(sInt i=0;i<sMTRL_MAXTEX;i++)
  {
    Texture[i] = 0;
    TFlags[i] = 0;
    LodBias[i] = 0.0f;
  }

  sPushMemLeakDesc(L"sMaterial");
  Create2();
  sPopMemLeakDesc();
}

sMaterial::~sMaterial()
{
  Destroy2();
  if(VertexShader) 
    VertexShader->Release();
  if(PixelShader) 
    PixelShader->Release();

#if sRENDERER==sRENDER_DX9
  if(States)
  {
    for(sInt i=0;i<StateVariants;i++)
      delete[] States[i];
    delete[] States;
    delete[] StateCount;
  }
#endif
  delete[] VariantFlags;
}

/*
void sMaterial::SetShader(sShader * vs,sShader * ps)
{
  sSetShaders(vs,ps,0,0);
}
*/

void sMaterial::CopyBaseFrom(const sMaterial *src)
{
  sVERIFY(src->GetVariantCount()<=1);   // variants are not copied, so don't use CopyBaseFrom with
                                        // materials having multiple variants
  for(sInt i=0;i<sMTRL_MAXTEX;i++)
    Texture[i] = src->Texture[i];
  NameId = src->NameId;
  Flags = src->Flags;
  for(sInt i=0;i<sCOUNTOF(FuncFlags);i++)
    FuncFlags[i] = src->FuncFlags[i];
  for(sInt i=0;i<sMTRL_MAXTEX;i++)
    TFlags[i] = src->TFlags[i];
  BlendColor = src->BlendColor;
  BlendAlpha = src->BlendAlpha;
  BlendFactor = src->BlendFactor;
  for(sInt i=0;i<sCOUNTOF(StencilOps);i++)
    StencilOps[i] = src->StencilOps[i];
  StencilRef = src->StencilRef;
  StencilMask = src->StencilMask;
  AlphaRef = src->AlphaRef;
#if sRENDERER==sRENDER_DX9 || sRENDERER==sRENDER_DX11
  for(sInt i=0;i<sMTRL_MAXTEX;i++)
    TBind[i] = src->TBind[i];
#endif

  // variants are not copied. sorry.

  //// copy variants: sMaterial::Prepare after CopyBaseFrom ignore changed states because of existing variants
  //// so use the old variants ignoring CopyBaseFrom code
  //for(sInt i=0;i<sMTRL_MAXTEX;i++)
  //  Texture[i] = src->Texture[i];
  //NameId = src->NameId;

  //sInt vcount = src->GetVariantCount();
  //InitVariants(vcount);
  //for(sInt i=0;i<vcount;i++)
  //  SetVariantRS(i,src->GetVariantRS(i));
  //*(sMaterialRS*)&Flags = *(sMaterialRS*)&src->Flags;
}

/****************************************************************************/

#if sRENDERER==sRENDER_DX9

void sMaterial::InitVariants(sInt max)
{

  if(StateVariants==max && States)
    return;
  if(States)          // it happens that this is initialised twice
  {
    sLogF(L"gfx",L"material variants initialized twice\n");
    for(sInt i=0;i<StateVariants;i++)
      sDeleteArray(States[i]);
    sDeleteArray(VariantFlags);
    sDeleteArray(States);
    sDeleteArray(StateCount);
  }

  StateVariants = max;
  VariantFlags = new sMaterialRS[max];

  States = new sU32 *[max];
  StateCount = new sInt[max];
  for(sInt i=0;i<max;i++)
  {
    States[i] = 0;
    StateCount[i] = 0;
  }
}

void sMaterial::DiscardVariants()
{
  sLogF(L"gfx",L"sMaterial::DiscardVariants: only use this for testing\n");

  if(States)
  {
    for(sInt i=0;i<StateVariants;i++)
      sDeleteArray(States[i]);
    sDeleteArray(VariantFlags);
    sDeleteArray(States);
    sDeleteArray(StateCount);
  }

  StateVariants = 0;
}

#if sRENDERER!=sRENDER_OGL2
void sMaterial::AllocStates(const sU32 *data,sInt count,sInt var)
{
  sVERIFY(var<StateVariants);
  sVERIFY(States[var]==0);

  States[var] = new sU32[count*2];
  StateCount[var] = count;
  sCopyMem(States[var],data,count*8);
}

void sMaterial::SetVariant(sInt var)
{
  sVERIFY(var>=0 && var<StateVariants);

  sU32 buffer[512];
  sU32 *data=buffer;

  VariantFlags[var] = *(sMaterialRS*)&Flags;
  AddMtrlFlags(data);
  sVERIFY(data>=buffer && data<=buffer+sCOUNTOF(buffer));
  AllocStates(buffer,(data-buffer)/2,var);
}
#endif

void sMaterial::SetVariantRS(sInt var, const sMaterialRS &rs)
{
  sMaterialRS temp;
  temp = *(sMaterialRS*)&Flags;
  *(sMaterialRS*)&Flags = rs;
  SetVariant(var);
  *(sMaterialRS*)&Flags = temp;
}

sMaterialRS &sMaterial::GetVariantRS(sInt var) const
{
  sVERIFY(var>=0 && var<StateVariants);
  return VariantFlags[var];
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Viewport                                                           ***/
/***                                                                      ***/
/****************************************************************************/

sViewport::sViewport()
{
  ClipNear = 0.125f;
  ClipFar = 1024.0f*2;
  ZoomX = 1.0f;
  ZoomY = 1.0f;
  CenterX = 0.5f;
  CenterY = 0.5f;
  Orthogonal = sVO_PROJECTIVE;
  DepthOffset = 0.0f;
  SetTarget(0);
};

void sViewport::CopyFrom(const sViewport &src)
{
  Model = src.Model;
  Camera = src.Camera;
  TargetSizeX = src.TargetSizeX;
  TargetSizeY = src.TargetSizeY;
  TargetAspect = src.TargetAspect;
  Target = src.Target;
  ClipNear = src.ClipNear;
  ClipFar = src.ClipFar;
  ZoomX = src.ZoomX;
  ZoomY = src.ZoomY;
  CenterX = src.CenterX;
  CenterY = src.CenterY;
  Orthogonal = src.Orthogonal;
  DepthOffset = src.DepthOffset;

  Proj = src.Proj;
  View = src.View;
  ModelView = src.ModelView;
  ModelScreen = src.ModelScreen;
}

void sViewport::SetTarget(const sTargetSpec &spec)
{
  Target = spec.Window;
  if (spec.Color)
  {
    TargetSizeX=spec.Color->SizeX;
    TargetSizeY=spec.Color->SizeY;
  }
  else
  {
    sGetScreenResolution(TargetSizeX,TargetSizeY);
  }
  TargetAspect = spec.Aspect;
}

void sViewport::SetTarget(sTexture2D *tex)
{
  if(tex)
  {
    TargetSizeX = tex->SizeX;
    TargetSizeY = tex->SizeY;
    TargetAspect = sF32(tex->SizeX)/sF32(tex->SizeY);
    Target.Init(0,0,TargetSizeX,TargetSizeY);
  }
  else
    SetTargetCurrent();
}

void sViewport::SetTarget(sTextureCube *tex, sTexCubeFace cf)
{
  if(tex)
  {
    TargetSizeX = tex->SizeXY;
    TargetSizeY = tex->SizeXY;
    TargetAspect = 1.0f;
    Target.Init(0,0,TargetSizeX,TargetSizeY);
  }
  else
    SetTargetCurrent();
}

void sViewport::SetTargetCurrent(const sRect *rect/*=0*/)
{
  TargetSizeX = sGFXRendertargetX;
  TargetSizeY = sGFXRendertargetY;
  if(!rect)
    rect = &sGFXViewRect;
  TargetAspect = sGFXRendertargetAspect*sGFXRendertargetY*sF32(rect->SizeX())/(sF32(rect->SizeY())*sGFXRendertargetX);
  Target = *rect;
}

void sViewport::SetTargetScreen(const sRect *rect/*=0*/)
{
  sRect r;
  sGetScreenResolution(TargetSizeX,TargetSizeY);
  if(!rect)
  {
    r.Init(0,0,TargetSizeX,TargetSizeY);
    rect = &r;
  }
  TargetAspect = sGetScreenAspect()*TargetSizeY*sF32(rect->SizeX())/(sF32(rect->SizeY())*TargetSizeX);
  Target = *rect;
}

void sViewport::SetZoom(sF32 aspect,sF32 zoom)
{
  ZoomX = zoom;
  ZoomY = zoom;
  if (aspect<1.0f)
    ZoomY *= aspect;
  else
    ZoomX /= aspect;
}

sF32 sViewport::GetZoom() const
{
  return TargetAspect<1.0f ? ZoomX : ZoomY;
}

void sViewport::SetZoom(sF32 zoom)
{
  SetZoom(TargetAspect,zoom);
}

void sViewport::SetDepthOffset(sF32 cs_distance, sF32 cs_offset)
{
  cs_distance = sMax(cs_distance,0.001f);
  cs_offset = sClamp(cs_offset,0.001f,cs_distance-0.0001f);
  // based on "Projection Matrix Tricks" http://www.terathon.com/gdc07_lengyel.pdf
  DepthOffset = cs_offset / (cs_distance * (cs_distance-cs_offset));
  DepthOffset *= -ClipNear*ClipFar / (ClipFar-ClipNear);
}

void sViewport::UpdateModelMatrix(const sMatrix34 &mat)
{
  Model = mat;
  Prepare(sVUF_MODEL);
}

void sViewport::Prepare(sInt update)
{
  if(update&(sVUF_PROJ|sVUF_WINDOW))
  {
    sMatrix44 mat(sDontInitialize);
    //sF32 xs = sF32(Target.SizeX())/TargetSizeX;
    //sF32 ys = sF32(Target.SizeY())/TargetSizeY;
    //sF32 xo = sF32(Target.CenterX()-0.5f)/TargetSizeX*2-1.0f;
    //sF32 yo = sF32(Target.CenterY()-0.5f)/TargetSizeY*2-1.0f;

    sF32 xs = 1.0f;
    sF32 ys = 1.0f;

    switch(Orthogonal)
    {
    case sVO_PROJECTIVE:
    case sVO_PROJECTIVE_CLIPFAR:
      {
#if sRENDERER==sRENDER_DX11
        sF32 xo = 0;
        sF32 yo = 0;
#else
        sF32 xo = sF32(TargetSizeX/2-0.5f)/TargetSizeX*2-1.0f;
        sF32 yo = sF32(TargetSizeY/2-0.5f)/TargetSizeY*2-1.0f;
#endif
        sF32 q = 1/(ClipFar-ClipNear);
        
        mat.i.x = xs * ZoomX;
        mat.i.y = 0;
        mat.i.z = 0;
        mat.i.w = 0;

        mat.j.x = 0;
        mat.j.y = ys * ZoomY;
        mat.j.z = 0;
        mat.j.w = 0;

        mat.k.x =   xo;
        mat.k.y = - yo;
        mat.k.z = q*ClipFar+DepthOffset;
        mat.k.w = 1;

        mat.l.x = 0;
        mat.l.y = 0;
        mat.l.z = -q*ClipFar*ClipNear;
        mat.l.w = 0;

        if(Orthogonal==sVO_PROJECTIVE_CLIPFAR)
        {
          const sF32 epsilon = 0.0000005f;
          mat.k.z = 1.0f-epsilon;
          mat.l.z = 0.0f;          
        }
      }
      break;

    case sVO_PIXELS:
      mat.i.x = 2.0f / Target.SizeX(); //TargetSizeX;
      mat.i.y = 0;
      mat.i.z = 0;
      mat.i.w = 0;

      mat.j.x = 0;
      mat.j.y = -2.0f / Target.SizeY(); //TargetSizeY;
      mat.j.z = 0;
      mat.j.w = 0;

      mat.k.x = 0;
      mat.k.y = 0;
      mat.k.z = 1+DepthOffset;
      mat.k.w = 0;

      mat.l.x = -1.0 /*+ 2.0f*Target.x0 / TargetSizeX*/;
      mat.l.y =  1.0 /*- 2.0f*Target.y0 / TargetSizeY*/;

      mat.l.z = 0;
      mat.l.w = 1;
      break;

    case sVO_SCREEN:
      mat.i.x = 2.0f * xs;
      mat.i.y = 0;
      mat.i.z = 0;
      mat.i.w = 0;

      mat.j.x = 0;
      mat.j.y = -2.0f * ys;
      mat.j.z = 0;
      mat.j.w = 0;

      mat.k.x = 0;
      mat.k.y = 0;
      mat.k.z = 1+DepthOffset;
      mat.k.w = 0;

      mat.l.x = -1.0 /*+ 2.0f*Target.x0 / TargetSizeX*/;
      mat.l.y =  1.0 /*- 2.0f*Target.y0 / TargetSizeY*/;
      mat.l.z = 0;
      mat.l.w = 1;
      break;
    case sVO_ORTHOGONAL:
      {
        sF32 q = 1/(ClipFar-ClipNear);

        mat.i.x = ZoomX;
        mat.i.y = 0;
        mat.i.z = 0;
        mat.i.w = 0;

        mat.j.x = 0;
        mat.j.y = ZoomY;
        mat.j.z = 0;
        mat.j.w = 0;

        mat.k.x = 0;
        mat.k.y = 0;
        mat.k.z = q+DepthOffset;
        mat.k.w = 0;

        mat.l.x = 0;
        mat.l.y = 0;
        mat.l.z = -q*ClipNear;
        mat.l.w = 1;
      }
      break;

    default:
      sVERIFYFALSE;
      return;
    }
    /*
#if sRENDERER==sRENDER_OGL2
    mat.k.z = 2*mat.k.z - 1;
    mat.l.z = 2*mat.l.z;
#endif
    */
    
    Proj = mat;
  }
  if(update)
  {
    if (update & sVUF_VIEW)
    {
      View = Camera;
      View.Invert34();

      if (CenterX!=0.5f || CenterY!=0.5f)
      {
        sMatrix44 transScale;
        transScale.Init();
        transScale.l.Init(2*(CenterX-0.5f),2*(CenterY-0.5f),0,1);

        sMatrix44 invProj=Proj;
        invProj.Invert();

        View *= sMatrix34(Proj * transScale * invProj);
      }
    }

    ModelView = Model*View;
    ModelScreen = ModelView*Proj;
  }
}

sBool sViewport::Transform(const sVector31 &p,sInt &ix,sInt &iy) const
{
  sVector4 s = p * ModelScreen;
  if(s.z>0)
  {
    ix = sInt(((s.x/s.w)*0.5f+0.5f) * TargetSizeX) - Target.x0;
    iy = sInt((0.5f-(s.y/s.w)*0.5f) * TargetSizeY) - Target.y0; 
    return 1;
  }
  else
  {
    ix = 0;
    iy = 0;
  }
  return 0;
}


sBool sViewport::Transform(const sVector31 &p,sF32 &ix,sF32 &iy) const
{
  sVector4 s = p * ModelScreen;
  if(s.z>0)
  {
    ix = ((s.x/s.w)*0.5f+0.5f) - (sF32)Target.x0 / TargetSizeX;
    iy = (0.5f-(s.y/s.w)*0.5f) - (sF32)Target.y0 / TargetSizeY; 
    return 1;
  }
  else
  {
    ix = 0;
    iy = 0;
  }
  return 0;
}

void sViewport::MakeRayPixel(sInt ix,sInt iy,sRay &ray) const
{
  sF32 mx = (ix-Target.x0) / sF32(Target.SizeX());
  sF32 my = (iy-Target.y0) / sF32(Target.SizeY());
  mx = mx*2-1;
  my = 1-my*2;
  ray.Dir = Camera.k + Camera.i*mx/ZoomX + Camera.j*my/ZoomY;
  ray.Start = Camera.l + ray.Dir*ClipNear;
  ray.Dir.Unit();
}

void sViewport::MakeRay(sF32 mx,sF32 my,sRay &ray) const
{
  ray.Dir = Camera.k + Camera.i*mx/ZoomX + Camera.j*my/ZoomY;    // this might be wrong!
  ray.Start = Camera.l + ray.Dir*ClipNear;
  ray.Dir.Unit();
}

sInt sViewport::Visible(const sAABBox &box, sF32 clipnear, sF32 clipfar) const
{
  register const sMatrix34 &mv=ModelView;
  register const sF32 zx=ZoomX;
  register const sF32 zy=ZoomY;
  sVector30 r[2][3];
  r[0][0].x = (mv.i.x*box.Min.x+mv.l.x)*zx; r[0][0].y=(mv.i.y*box.Min.x)*zy; r[0][0].z=mv.i.z*box.Min.x;
  r[0][1].x = (mv.j.x*box.Min.y)*zx; r[0][1].y=(mv.j.y*box.Min.y+mv.l.y)*zy; r[0][1].z=mv.j.z*box.Min.y;
  r[0][2].x = (mv.k.x*box.Min.z)*zx; r[0][2].y=(mv.k.y*box.Min.z)*zy; r[0][2].z=mv.k.z*box.Min.z+mv.l.z;
  r[1][0].x = (mv.i.x*box.Max.x+mv.l.x)*zx; r[1][0].y=(mv.i.y*box.Max.x)*zy; r[1][0].z=mv.i.z*box.Max.x;
  r[1][1].x = (mv.j.x*box.Max.y)*zx; r[1][1].y=(mv.j.y*box.Max.y+mv.l.y)*zy; r[1][1].z=mv.j.z*box.Max.y;
  r[1][2].x = (mv.k.x*box.Max.z)*zx; r[1][2].y=(mv.k.y*box.Max.z)*zy; r[1][2].z=mv.k.z*box.Max.z+mv.l.z;

  register sU32 amask = ~0;
  register sU32 omask = 0;
#if 0   // better for some consoles...
  for(sInt i=0;i<8;i++)
  {
    register const sVector30 t = r[(i&1)][0]+r[(i&2)>>1][1]+r[(i&4)>>2][2];
    register sU32 clip=0;
    if(t.x> t.z) clip |= 0x01;
    else if(t.x<-t.z) clip |= 0x04;
    if(t.y> t.z) clip |= 0x02;
    else if(t.y<-t.z) clip |= 0x08;
    if(t.z<clipnear) clip |= 0x10;
    else if(t.z>clipfar) clip |= 0x20;
    amask &= clip;
    omask |= clip;
  }
  if(amask!=0) return 0;          // total out
  if(omask==0) return 2;          // total in
  return 1;                       // part in

#else 

  for(sInt i=0;i<8;i++)
  {
    register const sVector30 t = r[(i&1)][0]+r[(i&2)>>1][1]+r[(i&4)>>2][2];
    register const sU32 clip = ((t.x> t.z)?0x01:((t.x<-t.z)?0x4:0))|
                         ((t.y> t.z)?0x02:((t.y<-t.z)?0x8:0))|
                         ((t.z<clipnear)?0x10:((t.z>clipfar)?0x20:0));
    amask &= clip;
    omask |= clip;
  }
  return (amask!=0)?0:(omask==0?2:1);

#endif

}


sInt sViewport::VisibleDist(const sAABBox &box,sF32 &dist) const
{
  sMatrix34 r[2];
  sVector30 v[8];
  sVector30 t;
  sInt i;
  sU32 clip,amask,omask;

  r[0] = ModelView;
  r[1] = ModelView;
  r[0].i *= box.Min.x; r[0].i.x += ModelView.l.x;
  r[0].j *= box.Min.y; r[0].j.y += ModelView.l.y;
  r[0].k *= box.Min.z; r[0].k.z += ModelView.l.z;
  r[1].i *= box.Max.x; r[1].i.x += ModelView.l.x;
  r[1].j *= box.Max.y; r[1].j.y += ModelView.l.y;
  r[1].k *= box.Max.z; r[1].k.z += ModelView.l.z;

  v[0] = r[0].i+r[0].j+r[0].k;
  v[1] = r[0].i+r[1].j+r[0].k;
  v[2] = r[1].i+r[0].j+r[0].k;
  v[3] = r[1].i+r[1].j+r[0].k;
  v[4] = r[0].i+r[0].j+r[1].k;
  v[5] = r[0].i+r[1].j+r[1].k;
  v[6] = r[1].i+r[0].j+r[1].k;
  v[7] = r[1].i+r[1].j+r[1].k;

  amask = ~0;
  omask = 0;
  dist = ClipFar;

  for(i=0;i<8;i++)
  {
    t = v[i];
    t.x = -t.x*ZoomX;
    t.y = -t.y*ZoomY;

    if(dist>t.z)
      dist = t.z;
    
    clip = 0;
    if(t.z>0)
    {

      if(t.x<-t.z) clip |= 0x01;
      if(t.y<-t.z) clip |= 0x02;
      if(t.x> t.z) clip |= 0x04;
      if(t.y> t.z) clip |= 0x08;
    }
    else
    {
      if(t.x<0) clip |= 0x01;
      if(t.y<0) clip |= 0x02;
      if(t.x>0) clip |= 0x04;
      if(t.y>0) clip |= 0x08;
    }
    if(t.z<ClipNear)
      clip |= 0x10;
    if(t.z>ClipFar)
      clip |= 0x20;

    amask &= clip;
    omask |= clip;
  }

  if(amask!=0) return 0;          // total out
  if(omask==0) return 2;          // total in
  return 1;                       // part in
}

sInt sViewport::VisibleDist2(const sAABBox &box,sF32 &near, sF32 &far) const
{
  sMatrix34 r[2];
  sVector30 v[8];
  sVector30 t;
  sInt i;
  sU32 clip,amask,omask;

  r[0] = ModelView;
  r[1] = ModelView;
  r[0].i *= box.Min.x; r[0].i.x += ModelView.l.x;
  r[0].j *= box.Min.y; r[0].j.y += ModelView.l.y;
  r[0].k *= box.Min.z; r[0].k.z += ModelView.l.z;
  r[1].i *= box.Max.x; r[1].i.x += ModelView.l.x;
  r[1].j *= box.Max.y; r[1].j.y += ModelView.l.y;
  r[1].k *= box.Max.z; r[1].k.z += ModelView.l.z;

  v[0] = r[0].i+r[0].j+r[0].k;
  v[1] = r[0].i+r[1].j+r[0].k;
  v[2] = r[1].i+r[0].j+r[0].k;
  v[3] = r[1].i+r[1].j+r[0].k;
  v[4] = r[0].i+r[0].j+r[1].k;
  v[5] = r[0].i+r[1].j+r[1].k;
  v[6] = r[1].i+r[0].j+r[1].k;
  v[7] = r[1].i+r[1].j+r[1].k;

  amask = ~0;
  omask = 0;
  far  = ClipNear;
  near = ClipFar;

  for(i=0;i<8;i++)
  {
    t = v[i];
    t.x = -t.x*ZoomX;
    t.y = -t.y*ZoomY;

    if(near > t.z) near = t.z;
    if(far  < t.z) far  = t.z;

    clip = 0;
    if(t.z>0)
    {

      if(t.x<-t.z) clip |= 0x01;
      if(t.y<-t.z) clip |= 0x02;
      if(t.x> t.z) clip |= 0x04;
      if(t.y> t.z) clip |= 0x08;
    }
    else
    {
      if(t.x<0) clip |= 0x01;
      if(t.y<0) clip |= 0x02;
      if(t.x>0) clip |= 0x04;
      if(t.y>0) clip |= 0x08;
    }
    if(t.z<ClipNear)
      clip |= 0x10;
    if(t.z>ClipFar)
      clip |= 0x20;

    amask &= clip;
    omask |= clip;
  }

  if(amask!=0) return 0;          // total out
  if(omask==0) return 2;          // total in
  return 1;                       // part in
}

/****************************************************************************/

sBool sViewport::Get2DBounds(const sAABBox &box,sFRect &bounds) const
{
  sVector31 points[8];
  box.MakePoints(points);
  return Get2DBounds(points,8,bounds);
}

sBool sViewport::Get2DBounds(const sVector31 points[],sInt count,sFRect &bounds) const
{
  // Based on "Calculating Screen Coverage", Chapter 6 from "Jim Blinn's Corner:
  // Notation, Notation, Notation", Morgan-Kaufman Publishers, 2003
  sF32 xMin = 1.0f, xMax = -1.0f;
  sF32 yMin = 1.0f, yMax = -1.0f;
  sVector4 *proj = sALLOCSTACK(sVector4,count);
  sU32 *outcodes = sALLOCSTACK(sU32,count);
  sU32 Ocumulate = 0, Acumulate = ~0u;
  sBool anyVisible = sFALSE;

  // pass 1: project all points
  for(sInt i=0;i<count;i++)
  {
    sVector4 p(sDontInitialize);
    proj[i] = p = points[i] * ModelScreen;

    // calculate outcodes for x and y
    sU32 out = 0;
    if(p.x < -p.w)  out |= 0x01;
    if(p.x >  p.w)  out |= 0x02;
    if(p.y < -p.w)  out |= 0x04;
    if(p.y >  p.w)  out |= 0x08;
    if(p.z <  0.0f) out |= 0x10;
    if(p.z >  p.w)  out |= 0x20;

    outcodes[i] = out;
    Ocumulate |= out;
    Acumulate &= out;

    // if the point is inside, update bounds
    if((out & 0x3) == 0)
    {
      if(p.x - xMin*p.w < 0.0f) xMin = p.x / p.w;
      if(p.x - xMax*p.w > 0.0f) xMax = p.x / p.w;
    }

    if((out & 0xc) == 0)
    {
      if(p.y - yMin*p.w < 0.0f) yMin = p.y / p.w;
      if(p.y - yMax*p.w > 0.0f) yMax = p.y / p.w;
    }

    if(out == 0)
      anyVisible = sTRUE;
  }

  // trivial rejection/acception
  if(Acumulate) // all outside at least one clip plane
    return sFALSE;

  if(!Ocumulate) // none clipped
  {
    bounds.Init(xMin,yMin,xMax,yMax);
    return sTRUE;
  }

  if(!anyVisible) // none of the points visible but they span across all clip planes
  {
    bounds.Init(-1.0f,-1.0f,1.0f,1.0f); // full screen
    return sTRUE;
  }

  // pass 2: process clipped points
  for(sInt i=0;i<count;i++)
  {
    const sVector4 &p = proj[i];
    if((outcodes[i] & 1) && (p.x - xMin*p.w < 0.0f))  xMin = -1.0f;
    if((outcodes[i] & 2) && (p.x - xMax*p.w > 0.0f))  xMax =  1.0f;
    if((outcodes[i] & 4) && (p.y - yMin*p.w < 0.0f))  yMin = -1.0f;
    if((outcodes[i] & 8) && (p.y - yMax*p.w > 0.0f))  yMax =  1.0f;
  }

  bounds.Init(xMin,yMin,xMax,yMax);
  return sTRUE;
}

/****************************************************************************/

void sGetRendertargetSize(sInt &dx,sInt &dy)
{
  dx = sGFXViewRect.SizeX();//DXRendertargetX;
  dy = sGFXViewRect.SizeY();//DXRendertargetY;
}

sF32 sGetRendertargetAspect()
{
  return sGFXRendertargetAspect*sGFXRendertargetY*sF32(sGFXViewRect.SizeX())/(sF32(sGFXViewRect.SizeY())*sGFXRendertargetX);
}

sBool sIsFormatDXT(sInt format)
{
  switch(format & sTEX_FORMAT)
  {
  case sTEX_DXT1:
  case sTEX_DXT1A:
  case sTEX_DXT3:
  case sTEX_DXT5:
  case sTEX_DXT5N:
  case sTEX_DXT5_AYCOCG:
    return 1;
  default:
    return 0;
  }
}


void Render3D()
{
  if(sRender3DBegin())
  {
    if(sGetApp())
      sGetApp()->OnPaint3D();
    
    sRender3DEnd();
  }
}

/****************************************************************************/

void sSetFrameRateMode(sFrameRateMode mode)
{
}

#if !sFAKECBUFFER

#if sRENDERER!=sRENDER_DX11 && sRENDERER != sRENDER_OGLES2

void sClearCurrentCBuffers()
{
  sClear(CurrentCBs);
  sClear(CurrentCBsMask);
  sClear(CurrentCBsSlotMask);
}
#endif // sRENDERER!=sRENDER_DX11 

#if 0

// experimental. should solve "all" problems :-)
// this fails, when previous constantbuffers overlap with current ones.
// simple solution: whenever 

void sSetCBuffers(sCBufferBase **cbuffers,sInt cbcount)
{
  for(sInt i=0;i<cbcount;i++)
  {
    sCBufferBase *cb = cbuffers[i];
    sInt type = cb->Slot>>5;

    if(CurrentCBs[cb->Slot]==cb) // possible cache hit. check range overlap
    {
      sInt mask = CurrentCBsMask[type];
      mask &= ~CurrentCBsSlotMask[cb->Slot]; // mask = all regs that are used by OTHER slots.
      if(mask & cb->Mask)       // possible range overlap. take no risks. flush all (for this shader)
      {
        for(sInt i=0;i<sCBUFFER_MAXSLOT;i++)
        {
          CurrentCBs[i+type*sCBUFFER_MAXSLOT] = 0;
          CurrentCBsSlotMask[i+type*sCBUFFER_MAXSLOT] = 0;
        }
        CurrentCBsMask[type] = 0;

        // start anew and set this one
        CurrentCBs[cb->Slot] = cb;
        CurrentCBsMask[type] |= cb->Mask;
        CurrentCBsSlotMask[cb->Slot] = cb->Mask;
        cb->SetRegs();
      }
      else                      // no range overlap, but range might be extended!
      {
        CurrentCBsMask[type] |= cb->Mask;
      }
    }
    else  // all time fail
    {
      CurrentCBs[cb->Slot] = cb;
      CurrentCBsMask[type] |= cb->Mask;
      CurrentCBsSlotMask[cb->Slot] = cb->Mask;
      cb->SetRegs();
    }
  }
}

#else


//extern sU32 setcubfferscount;

#if sRENDERER == sRENDER_DX11

// has own implementation...

#elif sRENDERER == sRENDER_OGLES2

// has own implementation...

#else

void sSetCBuffers(sCBufferBase **cbuffers,sInt cbcount)
{
  static const sChar *typenam[] = { L"VS",L"PS",L"GS" };

  // set constant buffers
  for(sInt j=0;j<cbcount;j++)
  {
//    setcbuffercount++;
    sCBufferBase *cb = cbuffers[j];
    if(cb)
    {
      sVERIFY(cb->Slot<sCBUFFER_MAXSLOT*sCBUFFER_SHADERTYPES);
      if(cb != CurrentCBs[cb->Slot])
      {
        sInt type = cb->Slot>>5;
        CurrentCBsMask[type] &= ~CurrentCBsSlotMask[cb->Slot];
        if(CurrentCBsMask[type]&cb->Mask)
        {
          // handle register collision
          CurrentCBsSlotMask[cb->Slot] = 0;    // prevent detection with itself
          sBool unhandled = 1;
          for(sInt k=type*sCBUFFER_MAXSLOT;k<(type+1)*sCBUFFER_MAXSLOT;k++)
          {
            if(cb->Mask & CurrentCBsSlotMask[k])
            {
              // handled cbuffer collisions are ok (in most cases)
              //sDPrintF(L"register collision %s, %02d: %03d..%03d vs %02d: %016x\n",
              //  typenam[type],
              //  cb->Slot,cb->RegStart,cb->RegStart+cb->RegCount-1,
              //  i,CurrentCBsSlotMask[i]);
              CurrentCBsMask[type] &= ~CurrentCBsSlotMask[k];
              CurrentCBsSlotMask[k] = 0;
              CurrentCBs[k] = 0;
              unhandled = 0;
            }
          }
          if(unhandled)
          {
            sLogF(L"gfx",L"register collision %s, %02d: %03d..%03d\n",
              typenam[type],
              cb->Slot,cb->RegStart,cb->RegStart+cb->RegCount-1);
          }
        }
        cb->SetRegs();
        CurrentCBs[cb->Slot] = cb;
        CurrentCBsMask[type] |= cb->Mask;
        CurrentCBsSlotMask[cb->Slot] = cb->Mask;
      }
    }
  }
}

#endif
#endif

#if sRENDERER!=sRENDER_DX11 && sRENDERER!=sRENDER_OGLES2

void sCBufferBase::Modify()
{
  if(DataPtr)
    *DataPtr = DataPersist;
  if(CurrentCBs[Slot]==this)
    CurrentCBs[Slot] = 0;
}

void sCBufferBase::SetCfg(sInt slot, sInt start, sInt count)
{
  sU64 msk = ((sU64(1)<<(sClamp((RegCount+3)/4-32,0,32)))-1)<<32;
  msk |= (sU64(1)<<(sClamp((RegCount+3)/4,0,32)))-1;
  msk = msk<<(RegStart/4);
  SetCfg(slot,start,count,msk);
}

void sCBufferBase::SetCfg(sInt slot, sInt start, sInt count, sU64 mask)
{
  Slot = slot;
  RegStart = start;
  RegCount = count;
  Mask = mask;
  Flags = 0;
}

#endif // sRENDERER!=sRENDER_DX11

#endif // !sFAKECBUFFER


#if sRENDERER!=sRENDER_DX11 && sRENDERER!=sRENDER_OGLES2

sCBufferBase *sGetCurrentCBuffer(sInt slot)
{ sVERIFY(slot<sCBUFFER_MAXSLOT*sCBUFFER_SHADERTYPES);
  return CurrentCBs[slot];
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   DXT Codec                                                          ***/
/***                                                                      ***/
/****************************************************************************/

namespace rygdxt
{
  // for implicit init...
  static sInt Inited=0;

  // Couple of tables...
  static sU8 Expand5[32];
  static sU8 Expand6[64];
  static sU8 OMatch5[256][2];
  static sU8 OMatch6[256][2];
  static sU8 QuantRBTab[256+16];
  static sU8 QuantGTab[256+16];

  /**************************************************************************/

  static sInt Mul8Bit(sInt a,sInt b)
  {
    sInt t = a*b + 128;
    return (t + (t >> 8)) >> 8;
  }

  static sInt SquaredDist(sU8 a,sU8 b)
  {
    return sSquare(a-b);
  }

  union Pixel
  {
    struct
    {
#if sCONFIG_LE
      sU8 b,g,r,a;
#else
      sU8 a,r,g,b;
#endif
    } p;
    sU32 v;

    void From16Bit(sU16 v)
    {
      sInt rv = (v & 0xf800) >> 11;
      sInt gv = (v & 0x07e0) >>  5;
      sInt bv = (v & 0x001f) >>  0;

      p.a = 0;
      p.r = Expand5[rv];
      p.g = Expand6[gv];
      p.b = Expand5[bv];
    }

    sU16 As16Bit() const
    {
      return (Mul8Bit(p.r,31) << 11) + (Mul8Bit(p.g,63) << 5) + Mul8Bit(p.b,31);
    }

    void LerpRGB(const Pixel &p1,const Pixel &p2,sInt f)
    {
      p.r = p1.p.r + Mul8Bit(p2.p.r - p1.p.r,f);
      p.g = p1.p.g + Mul8Bit(p2.p.g - p1.p.g,f);
      p.b = p1.p.b + Mul8Bit(p2.p.b - p1.p.b,f);
    }
  };

  /****************************************************************************/

  static void PrepareOptTable(sU8 *Table,const sU8 *expand,sInt size)
  {
    for(sInt i=0;i<256;i++)
    {
      sInt bestErr = 256;

      for(sInt min=0;min<size;min++)
      {
        for(sInt max=0;max<size;max++)
        {
          sInt mine = expand[min];
          sInt maxe = expand[max];
          sInt err;

          err = sAbs(maxe + Mul8Bit(mine-maxe,0x55) - i);

          err += 3 * sAbs(maxe - mine) / 100; // fudge factor for different graphics cards (DX10 specifies 3% max decode error)

          if(err < bestErr)
          {
            Table[i*2+0] = max;
            Table[i*2+1] = min;
            bestErr = err;
          }
        }
      }
    }
  }

  static void EvalColors(Pixel *color,sU16 c0,sU16 c1)
  {
    color[0].From16Bit(c0);
    color[1].From16Bit(c1);
    color[2].LerpRGB(color[0],color[1],0x55);
    color[3].LerpRGB(color[0],color[1],0xaa);
  }

  // Block dithering function. Simply dithers a block to 565 RGB.
  // (Floyd-Steinberg)
  static void DitherBlock(Pixel *dest,const Pixel *block)
  {
    sInt err[8],*ep1 = err,*ep2 = err+4;

    // process channels seperately
    for(sInt ch=0;ch<3;ch++)
    {
      sU8 *bp = (sU8 *) block;
      sU8 *dp = (sU8 *) dest;
      sU8 *quant = (ch == 1) ? QuantGTab+8 : QuantRBTab+8;

#if sCONFIG_BE
      bp += 3-ch;
      dp += 3-ch;
#else
      bp += ch;
      dp += ch;
#endif     
      sSetMem(err,0,sizeof(err));

      for(sInt y=0;y<4;y++)
      {
        // pixel 0
        dp[ 0] = quant[bp[ 0] + ((3*ep2[1] + 5*ep2[0]) >> 4)];
        ep1[0] = bp[ 0] - dp[ 0];

        // pixel 1
        dp[ 4] = quant[bp[ 4] + ((7*ep1[0] + 3*ep2[2] + 5*ep2[1] + ep2[0]) >> 4)];
        ep1[1] = bp[ 4] - dp[ 4];

        // pixel 2
        dp[ 8] = quant[bp[ 8] + ((7*ep1[1] + 3*ep2[3] + 5*ep2[2] + ep2[1]) >> 4)];
        ep1[2] = bp[ 8] - dp[ 8];

        // pixel 3
        dp[12] = quant[bp[12] + ((7*ep1[2] + 5*ep2[3] + ep2[2]) >> 4)];
        ep1[3] = bp[12] - dp[12];

        // advance to next line
        sSwap(ep1,ep2);
        bp += 16;
        dp += 16;
      }
    }
  }

  // The color matching function (returns error metric)
  static sInt MatchColorsBlock(const Pixel *block,const Pixel *color,sU32 &outMask,sBool dither)
  {
    sU32 mask = 0;
    sInt dirr = color[0].p.r - color[1].p.r;
    sInt dirg = color[0].p.g - color[1].p.g;
    sInt dirb = color[0].p.b - color[1].p.b;

    sInt dots[16];
    for(sInt i=0;i<16;i++)
      dots[i] = block[i].p.r*dirr + block[i].p.g*dirg + block[i].p.b*dirb;

    sInt stops[4];
    for(sInt i=0;i<4;i++)
      stops[i] = color[i].p.r*dirr + color[i].p.g*dirg + color[i].p.b*dirb;
    
    sInt c0Point = (stops[1] + stops[3]) >> 1;
    sInt halfPoint = (stops[3] + stops[2]) >> 1;
    sInt c3Point = (stops[2] + stops[0]) >> 1;

    sInt error = 0;

    if(!dither)
    {
      // the version without dithering is straightforward
      for(sInt i=15;i>=0;i--)
      {
        mask <<= 2;
        sInt dot = dots[i];

        if(dot < halfPoint)
          mask |= (dot < c0Point) ? 1 : 3;
        else
          mask |= (dot < c3Point) ? 2 : 0;

        sInt j = mask&3;
        error += SquaredDist(block[i].p.r,color[j].p.r) + SquaredDist(block[i].p.g,color[j].p.g) + SquaredDist(block[i].p.b,color[j].p.b);
      }
    }
    else
    {
      // with floyd-steinberg dithering (see above)
      sInt err[8],*ep1 = err,*ep2 = err+4;
      sInt *dp = dots;

      c0Point <<= 4;
      halfPoint <<= 4;
      c3Point <<= 4;
      for(sInt i=0;i<8;i++)
        err[i] = 0;

      for(sInt y=0;y<4;y++)
      {
        sInt dot,lmask,step;

        // pixel 0
        dot = (dp[0] << 4) + (3*ep2[1] + 5*ep2[0]);
        if(dot < halfPoint)
          step = (dot < c0Point) ? 1 : 3;
        else
          step = (dot < c3Point) ? 2 : 0;

        ep1[0] = dp[0] - stops[step];
        lmask = step;

        // pixel 1
        dot = (dp[1] << 4) + (7*ep1[0] + 3*ep2[2] + 5*ep2[1] + ep2[0]);
        if(dot < halfPoint)
          step = (dot < c0Point) ? 1 : 3;
        else
          step = (dot < c3Point) ? 2 : 0;

        ep1[1] = dp[1] - stops[step];
        lmask |= step<<2;

        // pixel 2
        dot = (dp[2] << 4) + (7*ep1[1] + 3*ep2[3] + 5*ep2[2] + ep2[1]);
        if(dot < halfPoint)
          step = (dot < c0Point) ? 1 : 3;
        else
          step = (dot < c3Point) ? 2 : 0;

        ep1[2] = dp[2] - stops[step];
        lmask |= step<<4;

        // pixel 3
        dot = (dp[3] << 4) + (7*ep1[2] + 5*ep2[3] + ep2[2]);
        if(dot < halfPoint)
          step = (dot < c0Point) ? 1 : 3;
        else
          step = (dot < c3Point) ? 2 : 0;

        ep1[3] = dp[3] - stops[step];
        lmask |= step<<6;

        // advance to next line
        sSwap(ep1,ep2);
        dp += 4;
        mask |= lmask << (y*8);
      }

      // calc errors
      for(sInt i=0;i<16;i++)
      {
        sInt j = (mask >> (2*i)) & 3;
        error += SquaredDist(block[i].p.r,color[j].p.r) + SquaredDist(block[i].p.g,color[j].p.g) + SquaredDist(block[i].p.b,color[j].p.b);
      }
    }

    outMask = mask;
    return error;
  }

  // The color optimization function. (Clever code, part 1)
  static void OptimizeColorsBlock(const Pixel *block,sU16 &max16,sU16 &min16)
  {
    static const sInt nIterPower = 4;

    // determine color distribution
    sInt mu[3],min[3],max[3];

    for(sInt ch=0;ch<3;ch++)
    {
      const sU8 *bp = ((const sU8 *) block) + ch;
      sInt muv,minv,maxv;

      muv = minv = maxv = bp[0];
      for(sInt i=4;i<64;i+=4)
      {
        muv += bp[i];
        minv = sMin<sInt>(minv,bp[i]);
        maxv = sMax<sInt>(maxv,bp[i]);
      }

      mu[ch] = (muv + 8) >> 4;
      min[ch] = minv;
      max[ch] = maxv;
    }

    // determine covariance matrix
    sInt cov[6];
    for(sInt i=0;i<6;i++)
      cov[i] = 0;

    for(sInt i=0;i<16;i++)
    {
      sInt r = block[i].p.r - mu[2];
      sInt g = block[i].p.g - mu[1];
      sInt b = block[i].p.b - mu[0];

      cov[0] += r*r;
      cov[1] += r*g;
      cov[2] += r*b;
      cov[3] += g*g;
      cov[4] += g*b;
      cov[5] += b*b;
    }

    // convert covariance matrix to float, find principal axis via power iter
    sF32 covf[6],vfr,vfg,vfb;
    for(sInt i=0;i<6;i++)
      covf[i] = cov[i] / 255.0f;

    vfr = max[2] - min[2];
    vfg = max[1] - min[1];
    vfb = max[0] - min[0];

    for(sInt iter=0;iter<nIterPower;iter++)
    {
      sF32 r = vfr*covf[0] + vfg*covf[1] + vfb*covf[2];
      sF32 g = vfr*covf[1] + vfg*covf[3] + vfb*covf[4];
      sF32 b = vfr*covf[2] + vfg*covf[4] + vfb*covf[5];

      vfr = r;
      vfg = g;
      vfb = b;
    }

    sF32 magn = sMax(sMax(sFAbs(vfr),sFAbs(vfg)),sFAbs(vfb));
    sInt v_r,v_g,v_b;

    if(magn < 4.0f) // too small, default to luminance
    {
      v_r = 148;
      v_g = 300;
      v_b = 58;
    }
    else
    {
      magn = 512.0f / magn;
      v_r = sInt(vfr * magn);
      v_g = sInt(vfg * magn);
      v_b = sInt(vfb * magn);
    }

    // Pick colors at extreme points
    Pixel minp, maxp;
    sInt mind, maxd;

    minp = maxp = block[0];
    mind = maxd = block[0].p.r*v_r + block[0].p.g*v_g + block[0].p.b*v_b;
    for(sInt i=1;i<16;i++)
    {
      sInt dot = block[i].p.r*v_r + block[i].p.g*v_g + block[i].p.b*v_b;

      if(dot < mind)
      {
        mind = dot;
        minp = block[i];
      }

      if(dot > maxd)
      {
        maxd = dot;
        maxp = block[i];
      }
    }

    // Reduce to 16 bit colors
    max16 = maxp.As16Bit();
    min16 = minp.As16Bit();
  }

  // The refinement function. (Clever code, part 2)
  // Tries to optimize colors to suit block contents better.
  // (By solving a least squares system via normal equations+Cramer's rule)
  static sBool RefineBlock(const Pixel *block,sU16 &max16,sU16 &min16,sU32 mask)
  {
    static const sInt w1Tab[4] = { 3,0,2,1 };
    static const sInt prods[4] = { 0x090000,0x000900,0x040102,0x010402 };
    // ^some magic to save a lot of multiplies in the accumulating loop...

    sU16 oldMin = min16;
    sU16 oldMax = max16;

    if((mask ^ (mask << 2)) < 4) // just one index => degenerate system
    {
      sInt r,g,b;

      // calc average color
      r = g = b = 8; // rounding factor
      for(sInt i=0;i<16;i++)
      {
        r += block[i].p.r;
        g += block[i].p.g;
        b += block[i].p.b;
      }

      r >>= 4;
      g >>= 4;
      b >>= 4;

      // use optimal single-color match
      max16 = (OMatch5[r][0]<<11) | (OMatch6[g][0]<<5) | OMatch5[b][0];
      min16 = (OMatch5[r][1]<<11) | (OMatch6[g][1]<<5) | OMatch5[b][1];
    }
    else
    {
      sInt akku = 0;
      sInt At1_r,At1_g,At1_b;
      sInt At2_r,At2_g,At2_b;
      sU32 cm = mask;

      At1_r = At1_g = At1_b = 0;
      At2_r = At2_g = At2_b = 0;
      for(sInt i=0;i<16;i++,cm>>=2)
      {
        sInt step = cm&3;
        sInt w1 = w1Tab[step];
        sInt r = block[i].p.r;
        sInt g = block[i].p.g;
        sInt b = block[i].p.b;

        akku    += prods[step];
        At1_r   += w1*r;
        At1_g   += w1*g;
        At1_b   += w1*b;
        At2_r   += r;
        At2_g   += g;
        At2_b   += b;
      }

      At2_r = 3*At2_r - At1_r;
      At2_g = 3*At2_g - At1_g;
      At2_b = 3*At2_b - At1_b;

      // extract solutions (this system is always regular, thanks to the check above)
      sInt xx = akku >> 16;
      sInt yy = (akku >> 8) & 0xff;
      sInt xy = (akku >> 0) & 0xff;

      sF32 frb = 3.0f * 31.0f / 255.0f / (xx*yy - xy*xy);
      sF32 fg = frb * 63.0f / 31.0f;

      // solve.
      max16 =   sClamp<sInt>(sInt((At1_r*yy - At2_r*xy)*frb+0.5f),0,31) << 11;
      max16 |=  sClamp<sInt>(sInt((At1_g*yy - At2_g*xy)*fg +0.5f),0,63) << 5;
      max16 |=  sClamp<sInt>(sInt((At1_b*yy - At2_b*xy)*frb+0.5f),0,31) << 0;

      min16 =   sClamp<sInt>(sInt((At2_r*xx - At1_r*xy)*frb+0.5f),0,31) << 11;
      min16 |=  sClamp<sInt>(sInt((At2_g*xx - At1_g*xy)*fg +0.5f),0,63) << 5;
      min16 |=  sClamp<sInt>(sInt((At2_b*xx - At1_b*xy)*frb+0.5f),0,31) << 0;
    }

    return oldMin != min16 || oldMax != max16;
  }

  // Color block compression
  static void CompressColorBlock(sU8 *dest,const sU32 *src,sInt qualityDither)
  {
    sInt quality = qualityDither & 0x3f;
    sBool dither = (qualityDither & 0x80) != 0;

    const Pixel *block = (const Pixel *) src;
    Pixel dblock[16],color[4];

    // check if block is constant
    sInt i=1;
    while(i<16 && block[i].v == block[0].v)
      i++;

    // perform block compression
    sU16 min16,max16;
    sU32 mask;

    if(i != 16) // no constant color
    {
      // first step: dither (if requested)
      if(dither)
        DitherBlock(dblock,block);

      // first step: pca+map along principal axis
      OptimizeColorsBlock(dither ? dblock : block,max16,min16);
      EvalColors(color,max16,min16);
      sInt error = MatchColorsBlock(block,color,mask,dither);

      // second step: refine, take it if error improves
      sU16 tryMax16 = max16;
      sU16 tryMin16 = min16;
      sU32 tryMask = mask;
      for(sInt j=0;j<(quality ? 3 : 1);j++)
      {
        if(RefineBlock(dither ? dblock : block,tryMax16,tryMin16,tryMask))
        {
          EvalColors(color,tryMax16,tryMin16);
          sInt tryErr = MatchColorsBlock(block,color,tryMask,dither);

          if(tryErr < error) // i'll take it!
          {
            error = tryErr;
            mask = tryMask;
            max16 = tryMax16;
            min16 = tryMin16;
          }
        }
        else
          break; // got the same colors as the last attempt, forget it.
      }
    }
    else // constant color
    {
      sInt r = block[0].p.r;
      sInt g = block[0].p.g;
      sInt b = block[0].p.b;

      mask  = 0xaaaaaaaa;
      max16 = (OMatch5[r][0]<<11) | (OMatch6[g][0]<<5) | OMatch5[b][0];
      min16 = (OMatch5[r][1]<<11) | (OMatch6[g][1]<<5) | OMatch5[b][1];
    }

    // write the color block
    if(max16 < min16)
    {
      sSwap(max16,min16);
      mask ^= 0x55555555;
    }

    ((sU16 *) dest)[0] = sSwapIfBE(max16);
    ((sU16 *) dest)[1] = sSwapIfBE(min16);
    ((sU32 *) dest)[1] = sSwapIfBE(mask);
  }

  // Alpha block compression (this is easy for a change)
  static void CompressAlphaBlock(sU8 *dest,const sU32 *src,sInt quality)
  {
    const Pixel *block = (const Pixel *) src;

    // find min/max color
    sInt min,max;
    min = max = block[0].p.a;

    for(sInt i=1;i<16;i++)
    {
      min = sMin<sInt>(min,block[i].p.a);
      max = sMax<sInt>(max,block[i].p.a);
    }

    // encode them
    *dest++ = max;
    *dest++ = min;

    // determine bias and emit color indices
    sInt dist = max-min;
    sInt bias = min*7 - (dist >> 1);
    sInt dist4 = dist*4;
    sInt dist2 = dist*2;
    sInt bits = 0,mask=0;
    
    for(sInt i=0;i<16;i++)
    {
      sInt a = block[i].p.a*7 - bias;
      sInt ind,t;

      // select index (hooray for bit magic)
      t = (dist4 - a) >> 31;  ind =  t & 4; a -= dist4 & t;
      t = (dist2 - a) >> 31;  ind += t & 2; a -= dist2 & t;
      t = (dist - a) >> 31;   ind += t & 1;

      ind = -ind & 7;
      ind ^= (2 > ind);

      // write index
      mask |= ind << bits;
      if((bits += 3) >= 8)
      {
        *dest++ = mask;
        mask >>= 8;
        bits -= 8;
      }
    }
  }
}

/****************************************************************************/

using namespace rygdxt;

void sCompressDXTBlock(sU8 *dest,const sU32 *src,sBool alpha,sInt quality)
{
  // generate tables for the first time
  if(!Inited)
  {
    for(sInt i=0;i<32;i++)
      Expand5[i] = (i<<3)|(i>>2);

    for(sInt i=0;i<64;i++)
      Expand6[i] = (i<<2)|(i>>4);

    for(sInt i=0;i<256+16;i++)
    {
      sInt v = sClamp(i-8,0,255);
      QuantRBTab[i] = Expand5[Mul8Bit(v,31)];
      QuantGTab[i] = Expand6[Mul8Bit(v,63)];
    }

    PrepareOptTable(&OMatch5[0][0],Expand5,32);
    PrepareOptTable(&OMatch6[0][0],Expand6,64);

    Inited = 1;
  }

  // if alpha specified, compress alpha aswell
  if(alpha)
  {
    CompressAlphaBlock(dest,src,quality);
    dest += 8;
  }

  // compress the color part
  CompressColorBlock(dest,src,quality);
}

void sFastPackDXT(sU8 *d,sU32 *bmp,sInt xs,sInt ys,sInt format,sInt quality)
{
  sInt xb=(xs+3)/4;
  sInt yb=(ys+3)/4;
  sU32 block[16];

  for (sInt y=0; y<yb; y++)
  {
    for (sInt x=0; x<xb; x++)
    {
      if(x != (xb-1) && y != (yb-1))
      {
        for (sInt yy=0; yy<4; yy++)
          for (sInt xx=0; xx<4; xx++)
            block[4*yy+xx]=bmp[xs*yy+xx];
      }
      else
      {
        sInt xm = (xs&3) ? (xs&3) : 4;
        sInt ym = (ys&3) ? (ys&3) : 4;

        for (sInt yy=0; yy<4; yy++)
          for (sInt xx=0; xx<4; xx++)
            block[4*yy+xx]=bmp[xs*(yy%ym)+(xx%xm)];
      }

      switch (format)
      {
      case sTEX_DXT1:
        sCompressDXTBlock(d,block,sFALSE,quality);
        d+=8;
        break;
      case sTEX_DXT5:
        sCompressDXTBlock(d,block,sTRUE,quality);
        d+=16;
        break;
      case sTEX_DXT5N:
        for(sInt i=0;i<16;i++)
          block[i] = (block[i] & 0x0000ff00) | ((block[i] & 0x00ff0000) << 8);
        sCompressDXTBlock(d,block,sTRUE,quality);
        d+=16;
        break;
      case sTEX_DXT5_AYCOCG:
        for(sInt i=0;i<16;i++)
          block[i] = sARGBtoAYCoCg(block[i]);
        sCompressDXTBlock(d,block,sTRUE,quality);
        d+=16;
        break;
      default:
        sVERIFYFALSE;
      }

      bmp+=4;
    }
    bmp+=3*xs;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   BeginTarget() interface platform independent                       ***/
/***                                                                      ***/
/****************************************************************************/

sTargetSpec::sTargetSpec()
{
  Init();
}

sTargetSpec::sTargetSpec(const sRect &r)
{
  Init(r);
}

sTargetSpec::sTargetSpec(sTexture2D *tex,sTexture2D *depth)
{
  Init(tex,depth);
}

sTargetSpec::sTargetSpec(sTextureCube *tex,sTexture2D *depth,sInt face)
{
  Init(tex,depth,face);
}

void sTargetSpec::Init()
{
  Init(0,0);
}

void sTargetSpec::Init(const sRect &r)
{
  Init(0,0);
  Window = r;
  Aspect = sF32(r.SizeX())/sF32(r.SizeY());
}

void sTargetSpec::Init(sTexture2D *color,sTexture2D *depth)
{
  Window.Init(0,0,0,0);
  if(color)
  {
    Color = color;
    Depth = depth;
    sInt z;
    Color->GetSize(Window.x1,Window.y1,z);
    Aspect = sF32(Window.x1)/sF32(Window.y1);
  }
  else
  {
    Color = sGetScreenColorBuffer();
    Depth = sGetScreenDepthBuffer();
    sGetScreenSize(Window.x1,Window.y1);
    Aspect=sGetScreenAspect();
  }
  Cubeface = -1;
}

void sTargetSpec::Init(sTextureCube *color,sTexture2D *depth,sInt face)
{
  Color = color;
  Depth = depth;
  Cubeface = face;
  Window.Init(0,0,0,0);
}

/****************************************************************************/

sTargetPara::sTargetPara()
{
  Init();
}

sTargetPara::sTargetPara(sInt flags,sU32 clearcol,const sRect *window)
{
  Init(flags,clearcol,window);
}

sTargetPara::sTargetPara(sInt flags,sU32 clearcol,const sRect *window,sTextureBase *colorbuffer,sTextureBase *depthbuffer)
{
  Init(flags,clearcol,window,colorbuffer,depthbuffer);
}


sTargetPara::sTargetPara(sInt flags,sU32 clearcol,const sTargetSpec &spec)
{
  Init();
  Init(flags,clearcol,spec);
}

void sTargetPara::SetTarget(sInt i, sTextureBase *tex, sU32 clearcol)
{
  sVERIFY(i>=0&&i<sCOUNTOF(Target));
  Target[i] = tex;
  ClearColor[i].InitColor(clearcol);
}

void sTargetPara::Init()
{
  Flags = 0;
  for(sInt i=0;i<sCOUNTOF(ClearColor);i++)
    ClearColor[i].Init(0,0,0,0);
  ClearZ = 1.0f;
  Cubeface = -1;
  Mipmap = 0;
  Depth = 0;
  Aspect = 1;
  sClear(Target);
  Window.Init(0,0,0,0);
}

void sTargetPara::Init(sInt flags,sU32 clearcol,const sRect *window)
{
  Init(flags,clearcol,window,sGetScreenColorBuffer(),sGetScreenDepthBuffer());
  if(!window)
    Aspect = sGetScreenAspect();
}

void sTargetPara::Init(sInt flags,sU32 clearcol,const sRect *window,sTextureBase *colorbuffer,sTextureBase *depthbuffer)
{
  Flags = flags;
  for(sInt i=0;i<sCOUNTOF(ClearColor);i++)
    ClearColor[i].InitColor(clearcol);
  ClearZ = 1.0f;
  Cubeface = -1;
  Mipmap = 0;
  sClear(Target);

  Target[0] = colorbuffer;
  Depth = depthbuffer;

  if(window)
  {
    Window = *window;
  }
  else if(colorbuffer)
  {
    Window.Init(0,0,colorbuffer->SizeX,colorbuffer->SizeY);
  }
  else
  {
    Window.Init(0,0,0,0);
  }
  if (Window.SizeY())
    Aspect = sF32(Window.SizeX()) / Window.SizeY();
  else
    Aspect=1;
}

void sTargetPara::Init(sInt flags,sU32 clearcol,const sTargetSpec &spec)
{
  if(spec.Window.IsEmpty())
    Init(flags,clearcol,0,spec.Color,spec.Depth);
  else
    Init(flags,clearcol,&spec.Window,spec.Color,spec.Depth);
  Cubeface = spec.Cubeface;
}

/****************************************************************************/

sCopyTexturePara::sCopyTexturePara()
{
  Flags = 0;
  Source = 0;
  SourceRect.Init(0,0,0,0);
  SourceCubeface = -1;
  Dest = 0;
  DestRect.Init(0,0,0,0);
  DestCubeface = -1;
}

sCopyTexturePara::sCopyTexturePara(sInt flags,sTextureBase *d,sTextureBase *s)
{
  sInt z;

  Flags = flags;
  Source = s;
  SourceRect.Init(0,0,0,0);
#if RENDERTARGET_NEW_TO_OLD
  if (!Source)
    sGetScreenSize(SourceRect.x1,SourceRect.y1);
  else
#endif
  Source->GetSize(SourceRect.x1,SourceRect.y1,z);
  SourceCubeface = -1;
  Dest = d;
  DestRect.Init(0,0,0,0);
  Dest->GetSize(DestRect.x1,DestRect.y1,z);
  DestCubeface = -1;
}

/****************************************************************************/
/***                                                                      ***/
/***   Implement old SetRendertarget() on top of new sSetTarget()         ***/
/***                                                                      ***/
/****************************************************************************/

#if RENDERTARGET_OLD_TO_NEW

void sSetRendertarget(const sRect *vrp,sInt flags,sU32 clearcolor)
{
  sInt newflags = 0;
  if(flags & sCLEAR_COLOR) newflags |= sST_CLEARCOLOR;
  if(flags & sCLEAR_ZBUFFER) newflags |= sST_CLEARDEPTH;

  sTargetPara para(newflags,clearcolor,vrp);
  sSetTarget(para);
}

void sSetRendertarget(const sRect *vrp, sTexture2D *tex,sInt flags,sU32 clearcolor)
{
  sInt newflags = 0;
  if(flags & sCLEAR_COLOR) newflags |= sST_CLEARCOLOR;
  if(flags & sCLEAR_ZBUFFER) newflags |= sST_CLEARDEPTH;

  sTexture2D *zb = sGetScreenDepthBuffer();
  switch(flags & sRTZBUF_MASK)
  {
  case sRTZBUF_DEFAULT:
    if(tex)
      zb = sGetRTDepthBuffer();
    break;
  case sRTZBUF_NONE:
    zb = 0;
    break;
  case sRTZBUF_FORCEMAIN:
    break;
  default:
    sVERIFYFALSE;
  }

  if(tex==0)
    tex = sGetScreenColorBuffer();

  sTargetPara para(newflags,clearcolor,vrp,tex,zb);
  sSetTarget(para);
}

void sSetRendertarget(const sRect *vrp,sInt flags,sU32 clearcolor,sTexture2D **tex,sInt count)
{
  sInt newflags = 0;
  if(flags & sCLEAR_COLOR) newflags |= sST_CLEARCOLOR;
  if(flags & sCLEAR_ZBUFFER) newflags |= sST_CLEARDEPTH;

  sTargetPara para;
  para.Flags = newflags;
  for(sInt i=0;i<count;i++)
  {
    para.ClearColor[i].InitColor(clearcolor);
    para.Target[i] = tex[i];
  }
  if(vrp)
  {
    para.Window = *vrp;
  }
  else
  {
    if(para.Target[0] && (para.Target[0]->Flags & sTEX_TYPE_MASK)==sTEX_2D)
      para.Window.Init(0,0,((sTexture2D*)para.Target[0])->SizeX,((sTexture2D*)para.Target[0])->SizeY);
    else
      para.Window.Init(0,0,0,0);
  }

  para.Depth = sGetScreenDepthBuffer();
  switch(flags & sRTZBUF_MASK)
  {
  case sRTZBUF_DEFAULT:
    break;
  case sRTZBUF_NONE:
    para.Depth = 0;
    break;
  case sRTZBUF_FORCEMAIN:
    para.Depth = sGetScreenDepthBuffer();
    break;
  default:
    sVERIFYFALSE;
  }
  sSetTarget(para);
}

void sSetRendertargetCube(sTextureCube* tex, sTexCubeFace cf, sInt flags,sU32 clearcolor)
{
  sInt newflags = 0;
  if(flags & sCLEAR_COLOR) newflags |= sST_CLEARCOLOR;
  if(flags & sCLEAR_ZBUFFER) newflags |= sST_CLEARDEPTH;

  sTargetPara para(newflags,clearcolor,0,tex,0);
  para.Cubeface = cf;

  para.Depth = sGetScreenDepthBuffer();
  switch(flags & sRTZBUF_MASK)
  {
  case sRTZBUF_DEFAULT:
    break;
  case sRTZBUF_NONE:
    para.Depth = 0;
    break;
  case sRTZBUF_FORCEMAIN:
    para.Depth = sGetScreenDepthBuffer();
    break;
  default:
    sVERIFYFALSE;
  }

  sSetTarget(para);
}

sINLINE sTexture2D *sGetCurrentBackBuffer() { return sGetScreenColorBuffer(); }
sINLINE sTexture2D *sGetCurrentBackZBuffer() { return sGetScreenDepthBuffer(); }
sINLINE void sEnlargeRTDepthBuffer(sInt x, sInt y) { sEnlargeZBufferRT(x,y); }

void sGrabScreen(class sTexture2D *tex, sGrabFilterFlags filter, const sRect *dst, const sRect *src)
{
  sInt flags = 0;
  if(filter==sGFF_LINEAR)
    flags = sCT_FILTER;

  sCopyTexturePara para(flags,tex,sGetScreenColorBuffer());
  if(dst)
    para.DestRect = *dst;
  if(src)
    para.SourceRect = *src;

  sCopyTexture(para);
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Implement new sSetTarget() on top of old SetRendertarget()         ***/
/***                                                                      ***/
/****************************************************************************/

#if RENDERTARGET_NEW_TO_OLD

void sSetTarget(const sTargetPara &para)
{
  const sRect *r=0;
  if(!para.Window.IsEmpty())
    r = &para.Window;
  sInt oldflags = 0;
  if(para.Flags & sST_CLEARCOLOR) oldflags |= sCLEAR_COLOR;
  if(para.Flags & sST_CLEARDEPTH) oldflags |= sCLEAR_ZBUFFER;
  if(para.Flags & sST_NOMSAA    ) oldflags |= sRTF_NO_MULTISAMPLING;
  if(!(para.Flags & sST_SCISSOR)) oldflags |= sCLEAR_NOSCISSOR;
  if(!para.Depth)
  {
    oldflags |= sRTZBUF_NONE;
    oldflags &= ~sCLEAR_ZBUFFER;
  }

  if(para.Target[1]==0 && para.Target[2]==0 && para.Target[3]==0)
  {
    if(para.Target[0]==0)
      sSetRendertarget(r,oldflags,para.ClearColor[0].GetColor());
    else if(para.Target[0]->CastTex2D())
      sSetRendertarget(r,para.Target[0]->CastTex2D(),oldflags,para.ClearColor[0].GetColor());
    else if(para.Target[0]->CastTexCube())
      sSetRendertargetCube(para.Target[0]->CastTexCube(),sTexCubeFace(para.Cubeface),oldflags,para.ClearColor[0].GetColor());
    else
      sVERIFYFALSE;
  }
  else
  {
    sBool differentCols = sFALSE;
    sInt n = 0;
    while(n<4 && para.Target[n])
    {
      if(para.ClearColor[n] != para.ClearColor[0]) differentCols = sTRUE;
      n++;
    }

    if(differentCols) // need to clear each rendertarget individually...
    {
      for(sInt i=0;i<n;i++)
        sSetRendertarget(r,(sTexture2D*)para.Target[i],sCLEAR_COLOR|sRTZBUF_NONE,para.ClearColor[i].GetColor());

      oldflags &= ~sCLEAR_COLOR;
    }

    sSetRendertarget(r,oldflags,para.ClearColor[0].GetColor(),(sTexture2D **)para.Target,n);
  }
}

void sResolveTarget()
{
  sSetRendertarget((const sRect *)0,0,0UL);
}

void sCopyTexture(const sCopyTexturePara &para)
{
  sGrabFilterFlags oldflags = sGFF_NONE;
  if(para.Flags & sCT_FILTER)
    oldflags = sGFF_LINEAR;

  if(para.Source == sGetScreenColorBuffer())
  {
    sVERIFY((para.Dest->Flags & sTEX_TYPE_MASK)==sTEX_2D);
    sGrabScreen((sTexture2D *)para.Dest,oldflags,&para.DestRect,&para.SourceRect);
  }
  else if(para.Dest == sGetScreenColorBuffer())
  {
    sVERIFY((para.Dest->Flags & sTEX_TYPE_MASK)==sTEX_2D);
    sSetScreen((sTexture2D *)para.Source,oldflags,&para.DestRect,&para.SourceRect);
  }
  else
  {
    sVERIFYFALSE;
  }
}

sTexture2D *sGetScreenColorBuffer(sInt screeen)  { return sGetCurrentBackBuffer(); }
sTexture2D *sGetScreenDepthBuffer(sInt screeen)  { return sGetCurrentBackZBuffer(); }
sTexture2D *sGetRTDepthBuffer()                  { static sTexture2D dummy; return &dummy; }  // we need to return dummy texture to correctly handle no zbuffer case
void sEnlargeRTDepthBuffer(sInt x, sInt y)       { sEnlargeZBufferRT(x,y); }

#endif

/****************************************************************************/
