/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_BASE_GRAPHICS_GL2_HPP
#define FILE_ALTONA2_LIBS_BASE_GRAPHICS_GL2_HPP

#include "Altona2/Libs/Base/Base.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/



#if (sConfigPlatform==sConfigPlatformIOS || sConfigPlatform==sConfigPlatformOSX || sConfigPlatform==sConfigPlatformAndroid)
enum sGraphicsLimits
{
    sGfxMaxTargets = 1,
    sGfxMaxCBs = 4,
    sGfxMaxStream = 4,
    sGfxMaxTexture = 4,
    sGfxMaxSampler = 4,
    sGfxMaxVSAttrib = 4,
    sGfxMaxUav = 1,
};
#else
enum sGraphicsLimits
{
    sGfxMaxTargets = 4,
    sGfxMaxCBs = 8,
    sGfxMaxStream = 4,
    sGfxMaxTexture = 16,
    sGfxMaxSampler = 16,
    sGfxMaxVSAttrib = 16,
    sGfxMaxUav = 1,
};
#endif

/****************************************************************************/

class sResourcePrivate
{ 
    friend class sContext;
protected:
    uint GLInternalFormat;
    int MainBind;                  // sRBM_??
    int TotalSize;
    int Usage;
    int Loading;
    uint8 *LoadBuffer;
    int LoadMipmap;
    sAdapter *Adapter;
    uint SharedHandle;
public:
    sResPara Para;
    uint GLName;
    bool ExternalOES;
    bool External;
};

/****************************************************************************/

class sVertexFormatPrivate
{ 
    friend class sContext;
protected:
    struct attrib
    {
        int Stream;
        int Size;
        int Type;
        bool Normalized;
        bool Instanced;
        int Pitch;
        int Offset;
    };

    attrib Attribs[sGfxMaxVSAttrib];
    int VertexSize[sGfxMaxStream];
    int Streams;
    sAdapter *Adapter;
};

/****************************************************************************/

class sShaderPrivate
{ 
    friend class sContext;
    friend class sMaterial;
protected:
    uint GLName;
    sAdapter *Adapter;
};

/****************************************************************************/

class sCBufferBasePrivate
{ 
    friend class sContext;
protected:
    uint8 *LoadBuffer;
    sAdapter *Adapter;
};

/****************************************************************************/

class sRenderStatePrivate
{ 
    friend class sContext;
protected:
    uint GLFlags;
    int GLDepthFunc;
    int GL_BOC,GL_BSC,GL_BDC;
    int GL_BOA,GL_BSA,GL_BDA;
    uint GL_BlendCache;
    sVector4 GLBlendColor;
    int GLMask;
    int GLCull;
    sAdapter *Adapter;
};

/****************************************************************************/

class sSamplerStatePrivate
{ 
    friend class sContext;
protected:
    int GLMaxLevel;
    int GLMinFilter;
    int GLMinFilterNoMipmap;
    int GLMagFilter;
    int GLWrapS;
    int GLWrapT;
    sAdapter *Adapter;
};

/****************************************************************************/

class sMaterialPrivate
{ 
    friend class sContext;
protected:
    ~sMaterialPrivate();
    struct CBInfo
    {
        int Loc;
        int Slot;
        int Count;
        sShaderTypeEnum Type;
    };
    struct SRInfo   // sr = shader resource = another word for texture
    {
        int Loc;
        int Slot;
    };

    uint GLName;
    int AttribMap[sGfxMaxVSAttrib];
    sArray<SRInfo> SRInfos;
    sArray<CBInfo> CBInfos;

    sArray<sSamplerState *> SamplerVS;
    sArray<sSamplerState *> SamplerPS;
    sArray<sResource *> TextureVS;
    sArray<sResource *> TexturePS;
};

/****************************************************************************/

class sDispatchContextPrivate
{
};

/****************************************************************************/

class sGeometryPrivate
{ 
protected:
    int Topology;
};

/****************************************************************************/

class sGpuQueryPrivate
{
protected:
    sAdapter *Adapter;
};

/****************************************************************************/

class sAdapterPrivate
{
public:
    sContext *ImmediateContext;
};

/****************************************************************************/

class sContextPrivate
{
protected:
    class sFixedMaterial *BlitMtrl;
    void *BlitCB;
    sContextPrivate();
    ~sContextPrivate();
public:
    sAdapter *Adapter;
};

/****************************************************************************/

class sScreenPrivate
{
public:
    sScreenPrivate(const sScreenMode &sm);
    ~sScreenPrivate();
    sResource *ColorBuffer;
    sResource *DepthBuffer;
    sAdapter *Adapter;

    // semi-protected

    sScreenMode ScreenMode;
    void *ScreenWindow;
};

/****************************************************************************/


}

#endif  // FILE_ALTONA2_LIBS_BASE_GRAPHICS_GL2_HPP
