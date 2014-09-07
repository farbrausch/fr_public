/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_BASE_GRAPHICS_BLANK_HPP
#define FILE_ALTONA2_LIBS_BASE_GRAPHICS_BLANK_HPP

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

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

    /****************************************************************************/

    class sResourcePrivate
    { 
    protected:
        sAdapter *Adapter;
        uint8 *LoadBuffer;
        uint SharedHandle;
    public:
        sResPara Para;
    };

    class sVertexFormatPrivate
    { 
    protected:
        sAdapter *Adapter;
    };

    class sShaderPrivate
    {
    protected:
        sAdapter *Adapter;
    };

    class sCBufferBasePrivate
    { 
    protected:
        uint8 *LoadBuffer;
        sAdapter *Adapter;
    };

    class sRenderStatePrivate
    {
    protected:
        sAdapter *Adapter;
    };

    class sSamplerStatePrivate
    {
    protected:
        sAdapter *Adapter;
    };

    class sMaterialPrivate
    {
    protected:
        sAdapter *Adapter;
    };
    

    class sDispatchContextPrivate
    {
    };

    class sGeometryPrivate
    {
    };

    class sGpuQueryPrivate
    {
    protected:
        sAdapter *Adapter;
    };

    class sAdapterPrivate
    {
    public:
        sContext *ImmediateContext;
    };

    class sContextPrivate
    {
    public:
        sAdapter *Adapter;
    };

    class sScreenPrivate
    {
    public:
        sScreenPrivate(const sScreenMode &sm);
        ~sScreenPrivate();
        sResource *ColorBuffer;
        sResource *DepthBuffer;
        sAdapter *Adapter;

        // semi-private
        sScreenMode ScreenMode;
        void *ScreenWindow;
    };

    /****************************************************************************/

}

#endif  // FILE_ALTONA2_LIBS_BASE_GRAPHICS_BLANK_HPP
