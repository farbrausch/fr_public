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

#ifndef HEADER_ALTONA_BASE_SERIALIZE
#define HEADER_ALTONA_BASE_SERIALIZE

#ifndef __GNUC__
#pragma once
#endif

#include "base/types.hpp"
#include "base/system.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Serialisation, new endian-safe system                              ***/
/***                                                                      ***/
/****************************************************************************/

class sWriter
{
  sU8 *Buffer;
  sInt BufferSize;
  sU8 *CheckEnd;
  sFile *File;
  sBool Ok;
  sU8 *Data;

  // object serialization
  struct sWriteLink *WOH[256]; 
  struct sWriteLink *WOL;
  sInt WOCount;

public:

  sWriter();
  ~sWriter();
  void Begin(sFile *file);
  sBool End();
  void Check();
  sBool IsOk() { return Ok; }
  void Fail() { Ok = 0; }
  sINLINE sBool IsReading() const { return 0; }
  sINLINE sBool IsWriting() const { return 1; }

  sInt Header(sU32 id,sInt currentversion);
  sU32 PeekHeader() { return 0 /*sSerId::Error*/; }
  sBool PeekFooter() { return sFALSE; }
  sU32 PeekU32() { return 0; }
  void Footer();
  void Skip(sInt bytes);
  void Align(sInt alignment=4);
  sInt RegisterPtr(void *);
  sBool IsRegistered(void *);
  void VoidPtr(const void *obj);
  template <typename T> void Ptr(const T *obj) { VoidPtr((const void *)obj); }
  template <typename T> void Enum(const T val) { U32(val); }
  sBool If(sBool c) { U32(c); return c; }
  template <typename T> void Index(T *e,sStaticArray<T *> &a) { U32(e - a[0]); }
  template <typename T> void Once(T *obj) { if(If(obj && !IsRegistered(obj))){RegisterPtr(obj);obj->Serialize(*this);} Ptr(obj); }
  template <typename T> void OnceRef(T *obj) { if(If(obj && !IsRegistered(obj))){RegisterPtr(obj);obj->Serialize(*this);} Ptr(obj); }
  template <typename T,typename A> void OnceRef(T *obj,A *x) { if(If(obj && !IsRegistered(obj))){RegisterPtr(obj);obj->Serialize(*this,x);} Ptr(obj); }
  void Bits(sInt *a,sInt *b,sInt *c,sInt *d,sInt *e,sInt *f,sInt *g,sInt *h);

  void U8(sU8 v)     { Data[0] = v;                           Data+=1; }
  void U16(sU16 v)   { sUnalignedLittleEndianStore16(Data,v); Data+=2; }
  void U32(sU8 &v)   { sUnalignedLittleEndianStore32(Data,v); Data+=4; } // useful variant for streaming 8 bit variables as 32 bit stream
  void U32(sU32 v)   { sUnalignedLittleEndianStore32(Data,v); Data+=4; }
  void U64(sU64 v)   { sUnalignedLittleEndianStore64(Data,v); Data+=8; }
  void ArrayU8(const sU8 *ptr,sInt count);
  void ArrayU16(const sU16 *ptr,sInt count);
  void ArrayU16Align4(const sU16 *ptr,sInt count);
  void ArrayU32(const sU32 *ptr,sInt count);
  void ArrayU64(const sU64 *ptr,sInt count);
  void String(const sChar *v);

  void S8(sS8 v)     { U8((sU8) v); }
  void S16(sS16 v)    { U16((sU16) v); }
  void S32(sS32 v)    { U32((sU32) v); }
  void S64(sS64 v)    { U64((sU64) v); }
  void F32(sF32 v)   { U32(sRawCast<sU32,sF32>(v)); }
  void F64(sF64 v)   { U64(sRawCast<sU64,sF64>(v)); }
  void ArrayS8(const sS8 *ptr,sInt count)     { ArrayU8((const sU8 *)ptr,count);  }
  void ArrayS16(const sS16 *ptr,sInt count)   { ArrayU16((const sU16 *)ptr,count); }
  void ArrayChar(const sChar *ptr,sInt count) { ArrayU16((const sU16 *)ptr,count); }
  void ArrayS32(const sS32 *ptr,sInt count)   { ArrayU32((const sU32 *)ptr,count); }
  void ArrayS64(const sS64 *ptr,sInt count)   { ArrayU64((const sU64 *)ptr,count); }
  void ArrayF32(const sF32 *ptr,sInt count)   { ArrayU32((const sU32 *)ptr,count); }
  void ArrayF64(const sF64 *ptr,sInt count)   { ArrayU64((const sU64 *)ptr,count); }

  template <class Type> void Array(const sStaticArray<Type> &a) { S32(a.GetCount()); }
  template <class Type> void ArrayNew(const sStaticArray<Type *> &a) { S32(a.GetCount()); }
  template <class Type> void ArrayNewHint(const sStaticArray<Type *> &a,sInt additional) { S32(a.GetCount()); }

  template <class Type> void ArrayRegister(sStaticArray<Type *> &a) {Type *e; sFORALL(a,e) RegisterPtr(e); }

  template <class Type> void ArrayAll(sStaticArray<Type> &a) { S32(a.GetCount()); for (sInt i=0; i<a.GetCount(); i++) { *this | a[i]; if((i&0xFF)==0) Check();} }
  template <class Type> void ArrayAllPtr(sStaticArray<Type *> &a) { S32(a.GetCount()); for (sInt i=0; i<a.GetCount(); i++) { *this | a[i]; if((i&0xFF)==0) Check();}  }

  sDInt GetSize();
};

/****************************************************************************/

class sReader
{
  sFile *File;
  const sU8 *Map;
  sU8 *Buffer;
  sInt BufferSize;
  const sU8 *Data;
  const sU8 *CheckEnd;
  sU8 *LoadEnd;
  sBool Ok;
  sS64 ReadLeft;
  sU32 LastId;

  void **ROL;
  sInt ROCount;

public:
  sBool DontMap;          // for debug purposes
  sReader();
  ~sReader();
  void Begin(sFile *file);
  sBool End();
  void Check();
  sBool IsOk() { return Ok; }
  void Fail() { Ok = 0; }
  sOBSOLETE const sU8 *GetPtr(sInt bytes); // get acces to data and advance pointer
  sINLINE sBool IsReading() const { return 1; }
  sINLINE sBool IsWriting() const { return 0; }
  void DebugPeek(sInt count);

  sInt Header(sU32 id,sInt currentversion);
  sU32 PeekHeader();
  sU32 PeekU32();
  sBool PeekFooter();
  void Footer();
  void Skip(sInt bytes);
  void Align(sInt alignment=4);
  sInt RegisterPtr(void *);
  sBool IsRegistered(void *) { sVERIFYFALSE; return sTRUE; } // don't use. included only to make template functions compile.
  void VoidPtr(void *&obj);
  template <typename T> void Ptr(T *&obj) { VoidPtr((void *&)obj); }
  template <typename T> void Enum(T &val) { sU32 v; U32(v); val=(T)v; }
  sBool If(sBool c) { sU32 c1; U32(c1); return c1; }
  template <typename T> void Index(T *&e,sStaticArray<T *> &a) { sU32 n; U32(n); e = a[n]; }
  template <typename T> void Once(T *&obj) { if(If(0)){obj=new T;RegisterPtr(obj);obj->Serialize(*this); } Ptr(obj); }
  template <typename T> void OnceRef(T *&obj) { if(If(0)){obj=new T;RegisterPtr(obj);obj->Serialize(*this);Ptr(obj);} else {Ptr(obj);obj->AddRef();} }
  template <typename T,typename A> void OnceRef(T *&obj,A *x) { if(If(0)){obj=new T;RegisterPtr(obj);obj->Serialize(*this,x);Ptr(obj);} else {Ptr(obj);obj->AddRef();} }
  void Bits(sInt *a,sInt *b,sInt *c,sInt *d,sInt *e,sInt *f,sInt *g,sInt *h);

  void U8(sU8 &v)    { v = Data[0];                          Data+=1; }
  void U8(sInt &v)   { v = Data[0];                          Data+=1; }
  void U16(sU16 &v)  { sUnalignedLittleEndianLoad16(Data,v); Data+=2; }
  void U16(sInt &v)  { v = Data[0]|(Data[1]<<8);             Data+=2; }
  void U32(sU8 &v)   { v = Data[0];                          Data+=4; } // useful variant for streaming 8 bit variables as 32 bit stream
  void U32(sU32 &v)  { sUnalignedLittleEndianLoad32(Data,v); Data+=4; }
  void U64(sU64 &v)  { sUnalignedLittleEndianLoad64(Data,v); Data+=8; }
  void ArrayU8(sU8 *ptr,sInt count);
  void ArrayU16(sU16 *ptr,sInt count);
  void ArrayU32(sU32 *ptr,sInt count);
  void ArrayU64(sU64 *ptr,sInt count);
  void String(sChar *v,sInt maxsize);

  void S8(sS8 &v)     { U8((sU8 &) v); }
  void S8(sInt &v)     { U8((sInt &) v); }
  void S16(sS16 &v)    { U16((sU16 &) v); }
  void S16(sInt &v)    { U16((sInt &) v); }
  void S32(sS32 &v)    { U32((sU32 &) v); }
  void S64(sS64 &v)    { U64((sU64 &) v); }
  void F32(sF32 &v)   { U32(*((sU32 *) &v)); }
  void F64(sF64 &v)   { U64(*((sU64 *) &v)); }
  void ArrayS8(sS8 *ptr,sInt count)     { ArrayU8((sU8 *)ptr,count);  }
  void ArrayS16(sS16 *ptr,sInt count)   { ArrayU16((sU16 *)ptr,count); }
  void ArrayChar(sChar *ptr,sInt count) { ArrayU16((sU16 *)ptr,count); }
  void ArrayS32(sS32 *ptr,sInt count)   { ArrayU32((sU32 *)ptr,count); }
  void ArrayS64(sS64 *ptr,sInt count)   { ArrayU64((sU64 *)ptr,count); }
  void ArrayF32(sF32 *ptr,sInt count)   { ArrayU32((sU32 *)ptr,count); }
  void ArrayF64(sF64 *ptr,sInt count)   { ArrayU64((sU64 *)ptr,count); }

  template <class Type> void Array(sStaticArray<Type> &a) 
  { sInt max; S32(max); sVERIFY(a.GetCount()==0); sTAG_CALLER(); a.Resize(max); } 
  template <class Type> void ArrayNew(sStaticArray<Type *> &a) 
  { sInt max; S32(max); sVERIFY(a.GetCount()==0); sTAG_CALLER(); a.Resize(max); 
    for(sInt i=0;i<max;i++) { sTAG_CALLER(); a[i]=new Type; } }
  template <class Type> void ArrayNewHint(sStaticArray<Type *> &a,sInt additional) 
  { sInt max; S32(max); sVERIFY(a.GetCount()==0); sTAG_CALLER(); a.HintSize(max+additional); a.Resize(max); 
    for(sInt i=0;i<max;i++) { sTAG_CALLER(); a[i]=new Type; } }

  template <class Type> void ArrayRegister(sStaticArray<Type *> &a) 
  {Type *e; sFORALL(a,e) RegisterPtr(e); }


  template <class Type> void ArrayAll(sStaticArray<Type> &a)
  {
    sInt max; S32(max); sVERIFY(a.GetCount()==0); sTAG_CALLER(); a.Resize(max);
	  sInt interval = 4095/sizeof(Type);
	  sInt ic=0;
    for (sInt i=0; i<max; i++) 
	  {
		  *this | a[i];
		  if (++ic==interval)
		  {
			  Check();
			  ic=0;
		  }
	  }
  }

  template <class Type> void ArrayAllPtr(sStaticArray<Type *> &a)
  {
    sInt max; S32(max); sVERIFY(a.GetCount()==0); sTAG_CALLER(); a.Resize(max);
    sInt interval = 4095/sizeof(Type);
    sInt ic=0;
    for (sInt i=0; i<max; i++)
    { 
      sTAG_CALLER(); a[i] = new Type;
      *this | a[i]; 
      if (++ic==interval)
      {
        Check();
        ic=0;
      }
    }
  }

};

/****************************************************************************/
//note: these operators are not allowed here, because they could break serialization when a header gets changed.
//inline sWriter& operator| (sWriter &s,sS16  v)  { s.S16(v); return s; } 
//inline sReader& operator| (sReader &s,sS16 &v)  { s.S16(v); return s; }
//inline sWriter& operator| (sWriter &s,sU16  v)  { s.U16(v); return s; }
//inline sReader& operator| (sReader &s,sU16 &v)  { s.U16(v); return s; }

inline sWriter& operator| (sWriter &s,sS32  v)  { s.S32(v); return s; } 
inline sReader& operator| (sReader &s,sS32 &v)  { s.S32(v); return s; }
inline sWriter& operator| (sWriter &s,sU32  v)  { s.U32(v); return s; }
inline sReader& operator| (sReader &s,sU32 &v)  { s.U32(v); return s; }
inline sWriter& operator| (sWriter &s,sF32  v)  { s.F32(v); return s; }
inline sReader& operator| (sReader &s,sF32 &v)  { s.F32(v); return s; }
inline sWriter& operator| (sWriter &s,sS64  v)  { s.S64(v); return s; }
inline sReader& operator| (sReader &s,sS64 &v)  { s.S64(v); return s; }
inline sWriter& operator| (sWriter &s,sU64  v)  { s.U64(v); return s; }
inline sReader& operator| (sReader &s,sU64 &v)  { s.U64(v); return s; }
inline sWriter& operator| (sWriter &s,sF64  v)  { s.F64(v); return s; }
inline sReader& operator| (sReader &s,sF64 &v)  { s.F64(v); return s; }
inline sWriter& operator| (sWriter &s,const sChar *v) { s.String(v); return s; }
inline sReader& operator| (sReader &s,const sStringDesc &desc) { s.String(desc.Buffer,desc.Size); return s; }
inline sWriter& operator| (sWriter &s,const sRect &v) { s | v.x0 | v.y0 | v.x1 | v.y1; return s; }
inline sReader& operator| (sReader &s,      sRect &v) { s | v.x0 | v.y0 | v.x1 | v.y1; return s; }
inline sWriter& operator| (sWriter &s,const sFRect &v) { s | v.x0 | v.y0 | v.x1 | v.y1; return s; }
inline sReader& operator| (sReader &s,      sFRect &v) { s | v.x0 | v.y0 | v.x1 | v.y1; return s; }

template <class Type> inline sWriter& operator| (sWriter &s,Type *a) { a->Serialize(s); return s; }
template <class Type> inline sReader& operator| (sReader &s,Type *a) { a->Serialize(s); return s; }

/****************************************************************************/

template<class Type> Type *sLoadObject(const sChar *name)
{
  sFile *file = sCreateFile(name,sFA_READ); if(!file) return 0; 
  sPushMemLeakDesc(sFindFileWithoutPath(name));
  Type *obj = new Type;
  sReader stream; 
  stream.Begin(file); 
  obj->Serialize(stream); 
  stream.End(); 
  delete file; 
  if(!stream.IsOk())
  {
    sLogF(L"file",L"error loading <%s>, Serialize failed 1\n",name);
    sDelete(obj);
  }
  sPopMemLeakDesc();
  return obj;
}

template<class Type> sBool sLoadObject(const sChar *name,Type *obj)
{
  sFile *file = sCreateFile(name,sFA_READ); if(!file) return 0; 
  sPushMemLeakDesc(sFindFileWithoutPath(name));
  sReader stream; stream.Begin(file); obj->Serialize(stream); stream.End();
  if(!stream.IsOk())
    sLogF(L"file",L"error loading <%s>, Serialize failed 2\n",name);
  delete file; 
  sPopMemLeakDesc();
  return stream.IsOk(); 
}

template<class Type> sBool sSaveObject(const sChar *name,Type *obj)
{
  sFile *file = sCreateFile(name,sFA_WRITE); if(!file) return 0; 
  sWriter stream; stream.Begin(file); obj->Serialize(stream); stream.End(); 
  if(!stream.IsOk())
    sLogF(L"file",L"error saving <%s>, Serialize failed 3\n",name);
  delete file; return stream.IsOk(); 
}

template<class Type> sBool sLoadObjectConfig(const sChar *name,Type *obj)
{
  sFile *file = sCreateFile(name,sFA_READ); if(!file) return 0; 
  sPushMemLeakDesc(sFindFileWithoutPath(name));
  sReader stream; stream.Begin(file); obj->SerializeConfig(stream); stream.End();
  if(!stream.IsOk())
    sLogF(L"file",L"error loading <%s>, Serialize failed 2\n",name);
  delete file; 
  sPopMemLeakDesc();
  return stream.IsOk(); 
}

template<class Type> sBool sSaveObjectConfig(const sChar *name,Type *obj)
{
  sFile *file = sCreateFile(name,sFA_WRITE); if(!file) return 0; 
  sWriter stream; stream.Begin(file); obj->SerializeConfig(stream); stream.End(); 
  if(!stream.IsOk())
    sLogF(L"file",L"error saving <%s>, Serialize failed 3\n",name);
  delete file; return stream.IsOk(); 
}


template<class Type> sBool sSaveObjectFailsafe(const sChar *name,Type *obj)
{
  sFile *file = sCreateFailsafeFile(name,sFA_WRITE); if(!file) return 0; 
  sWriter stream; stream.Begin(file); obj->Serialize(stream); stream.End(); 
  if(!stream.IsOk())
    sLogF(L"file",L"error saving <%s>, Serialize failed 3\n",name);
  delete file; return stream.IsOk(); 
}

template <typename T> sBool sCalcObjectMD5(sChecksumMD5 &md5, T* obj)
{
  sCalcMD5File c;
  sWriter s;
  s.Begin(&c);
  obj->Serialize(s);
  if(!s.End()) return sFALSE;
  c.Close();
  md5 = c.Checksum;
  return sTRUE;
}

/****************************************************************************/
/***                                                                      ***/
/***   official registry for serialization ids                            ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***  - always state the enum-values explicitly!                          ***/
/***  - don't leave gaps.                                                 ***/
/***  - if you remove a class, don't delete it, comment it out!           ***/
/***  - mark id's which are reserved but not yet used, so they can be     ***/
/***    safely reassigned.                                                ***/
/***                                                                      ***/
/****************************************************************************/

namespace sSerId
{
  enum Users_
  {
    Altona                = 0x00010000,
    TC3                   = 0x00020000,
    Farbrausch            = 0x00030000,
    Chaos                 = 0x00040000,
    Werkkzeug4            = 0x00050000,
  };

  enum Altona_
  {
    Error                 = 0x00000000,
    sImage                = 0x00010001,
    sTexture              = 0x00010002,
    sGeometry             = 0x00010003,
    sImageData            = 0x00010004,
    sColorGradient        = 0x00010005,
    sImageI16             = 0x00010006,
    sStaticArray          = 0x00010007,
    sGuiTheme             = 0x00010008,
  };

#ifdef sCONFIG_SDK_TC3
  enum TC3_
  {
    sToolMaterial         = 0x00020001,
    sToolMesh             = 0x00020002,
    sToolSkeleton         = 0x00020003,
    sToolLibrary          = 0x00020004,
    sToolTexture          = 0x00020005,
    sToolLight            = 0x00020006,
    sEngMaterial          = 0x00020007,
    sEngLibrary           = 0x00020009,
    SandboxDocument       = 0x00020009, // same as sEngLibrary! but sandbox stuff is obsolete
    SandboxPainter        = 0x0002000a,
    SandboxMaterials      = 0x0002000b, // unused???
    sKDTree               = 0x0002000c,
    SandboxPlacer         = 0x0002000d,
    EntityPlacer          = 0x0002000e,
    EntityMeshGen         = 0x0002000f,

    sEngSkeleton          = 0x00020010,
    EntityLibrary         = 0x00020011,
    EntityLight           = 0x00020012,
    sEngLight             = 0x00020013,
    SandboxParticles      = 0x00020014,
    sEngCubeLights        = 0x00020015,
    sKDToolLink           = 0x00020016,
    sToolLightmaps        = 0x00020017,
    sToolPackage          = 0x00020018,
    sEngPainterFont       = 0x00020019,
    sEngPackage           = 0x0002001a,
    sCubeEffect           = 0x0002001c, // example effect
    sEffectLoader         = 0x0002001d,
    Wz4CarveTrackData     = 0x0002001e,
//    Wz4EditOptions        = 0x0002001e,
//    Wz4DocOptions         = 0x0002001f,

    sAudioSoundBank       = 0x00020020,
    sLineEffect           = 0x00020021, // example effect
    BuildCache            = 0x00020023,
    sAudienceEffect       = 0x00020024,
    sIppEffect            = 0x00020025,
    sBystanderEffect      = 0x00020026,
    sImgBlobExtract       = 0x00020027,
    sSplinedObjectEffect  = 0x00020028,
    sAudience2Effect      = 0x00020029,
    sParticle2System      = 0x0002002a,
    sParticle2Emitter     = 0x0002002b,
    sParticle2Trigger     = 0x0002002c,
    sEffectLightBundle    = 0x0002002d,
    sLensFlareEffect      = 0x0002002e,
    

    sBodyMixer            = 0x00020030,
    sBodyDeformer         = 0x00020031,
    sDisplayEffect        = 0x00020032,
    sFakeWaterEffect      = 0x00020033,
    sRainDropEffect       = 0x00020034,
    sFlummyEffect         = 0x00020035,
    sBigFlagEffect        = 0x00020036,
    sTextFlagEffect       = 0x00020037,
    sStrahlerEffect       = 0x00020038,
    sLightShowLinearEffect= 0x00020039,
    sLightShowLightEnablerEffect= 0x0002003a,
    sRopeWayEffect        = 0x0002003b,
    sWindBagEffect        = 0x0002003c,
    sTrackCloudsEffect    = 0x0002003d,
    sSnowRoutineEffect    = 0x0002003e,
    sSnowGlitterEffect    = 0x0002003f,

    sTerrainEffect              = 0x00020040,
    sShaderWaterEffect          = 0x00020041,
    sTerrainBorderEffect        = 0x00020042,
    sTerrainStartFinishEffect   = 0x00020043,
    sTerrainGateEffect          = 0x00020044,
    sTerrainBestLineEffect      = 0x00020045,
    sTerrainCircleMarkerEffect  = 0x00020046,
    sTerrainSquareMarkerEffect  = 0x00020047,
    DeepSkierTrail              = 0x00020048,
    sBlobTrackerHash            = 0x00020049,
    AdvancedDesaturate          = 0x0002004a,
    BuildDepend                 = 0x0002004b,
    SkierTrail                  = 0x0002004c,
    SkierSnow                   = 0x0002004d,
    sTreePainterEffect          = 0x0002004e,
    sTreeEffect                 = 0x0002004f,
    
    sBobTrackEffect             = 0x00020050,
    MotionBlur                  = 0x00020051,
    sToolMeshData               = 0x00020052,
    JumpingCircleTrail          = 0x00020053,
    JumpingSideSparks           = 0x00020054,
    JumpingLaneSnow             = 0x00020055,
    BobParticleExplosion        = 0x00020056,
    TurboModeCircleTrails       = 0x00020057,
    Ipp2Effect                  = 0x00020058,
    Ipp2EffectTemplate          = 0x00020059,
    sTerrainSprayCircleMarkerEffect = 0x0002005a,
    TraceHandlerEffect          = 0x0002005b,
    WindWheelEffect             = 0x0002005c,
    SnowCanonEffect             = 0x0002005d,
    WaterSurfaceEffect          = 0x0002005e,
    CutsceneDocument            = 0x0002005f,
    
    CutsceneActorClip           = 0x00020060,
    CutsceneCutscene            = 0x00020061,
    sTreeMeshesEffect           = 0x00020062,
    sTerrainSoundMarkerEffect   = 0x00020063,
    AnimachineDocument          = 0x00020064,
    CharacterDefinition         = 0x00020065,
    sTerrainSphereMarkerEffect  = 0x00020066,
    CutsceneClip                = 0x00020067,
    CutsceneCameraClip          = 0x00020068,
    sTerrainJumpMarkerEffect    = 0x00020069,
    PalRuntime                  = 0x0002006a,
    sCutsceneActorClip          = 0x0002006b,
    sCutsceneCameraClip         = 0x0002006c,
    sCutscene                   = 0x0002006d,
    sCutsceneCollection         = 0x0002006e,
    sCutsceneActorDef           = 0x0002006f,

    sBystander2Effect           = 0x00020070,
    sSplinedHelicopterEffect    = 0x00020071,
    sParticle2SystemFast        = 0x00020072,
    sCutsceneLightBundleSpec    = 0x00020073,
    CharacterParameterList      = 0x00020074,
    AnimachineCreateDef         = 0x00020075,
    sEngClipSummary             = 0x00020076,
    MeadowEffect                = 0x00020077,
    CloudsEffect                = 0x00020078,
    LightProbeGridEffect        = 0x00020079,
    sTerrainRailEffect          = 0x0002007a,
    sParticle2GroupFast         = 0x0002007b,
    FerrisWheelEffect           = 0x0002007c,
    sAnimatedEffect             = 0x0002007d,
    sCutsceneIppEffect          = 0x0002007e,
    sRibbonEffect               = 0x0002007f,

    sTextureReference           = 0x00020080,
    sParamSet                   = 0x00020081,
    IppFXEffect                 = 0x00020082,
    IppFXTemplate               = 0x00020083,
    TrampolineEffect            = 0x00020084,
    WavingBannerEffect          = 0x00020085,
    sTonemapRTEffect            = 0x00020086,
    sScenePainterEffect         = 0x00020087,
    sCutsceneReplayClip         = 0x00020088,
    sCutsceneIppTemplate        = 0x00020089,
    sFlagEffect                 = 0x0002008a,
    sCacheTC3Shaders            = 0x0002008b,
    sAudienceTintedEffect       = 0x0002008c,
    sTerrainBorderCollisionEffect = 0x0002008d,
    sMarkerEffect               = 0x0002008e,
    sCutsceneIppParameter       = 0x0002008f,

    sAnimStateMachineObject     = 0x00020090,
    sClipEventData              = 0x00020091,
    sClipEventDefinition        = 0x00020092,
    sClipEventBlock             = 0x00020093,
    sClipEventKey               = 0x00020094,
    sSandboxEffect              = 0x00020095,
    sEngClipPCA                 = 0x00020096,
    sEngLProbeTree              = 0x00020097,
    LightProbeTreeEffect        = 0x00020098,
    CheckpointMark              = 0x00020099,
    IceSurfaceEffect            = 0x0002009a,
    sAvatarEffect               = 0x0002009b,
    RangeMark                   = 0x0002009c,
    MeshGlowEffect              = 0x0002009d,
    RunTrailEffect              = 0x0002009e,
    sGroundCollision            = 0x0002009f,

    KutterDoc                   = 0x000200a0,
    KutterConfig                = 0x000200a1,
    sEngPainterFont2            = 0x000200a2,
    KutterPlayer                = 0x000200a3,
    FadeEffect                  = 0x000200a4,
    CurlingTargetArrow          = 0x000200a5,
    RTEffect                    = 0x000200a6,
    PoleVaultEffect             = 0x000200a7,
    GrassPlaneEffect            = 0x000200a8,
    sParticle3Effect            = 0x000200a9,
    sVegetationEffect           = 0x000200aa,
    RttDisplayEffect            = 0x000200ab,
    LightGlareEffect            = 0x000200ac,
    sMeshMorphEffect            = 0x000200ad,
    sToolClip                   = 0x000200ae,
    sTexUvAnimEffect            = 0x000200af,

    CutsceneDefinition          = 0x000200b0,  
    sSceneInstanceEffect        = 0x000200b1,
    sCameraData                 = 0x000200b2,
    sMorphTargetCollection      = 0x000200b3,
    SnowmobileBacklight         = 0x000200b4,
    BillboardEffect             = 0x000200b5,
    WindTunnelEffect            = 0x000200b6,
    sBobExtraLineEffect         = 0x000200b8,

    // add here!
  };
#endif
  // 0x80000000 .. 0xffffffff -> free for random numbers...
};

/****************************************************************************/


// HEADER_ALTONA_BASE_SERIALIZE
#endif


