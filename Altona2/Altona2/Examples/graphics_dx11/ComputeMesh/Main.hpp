/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_EXAMPLES_GRAPHICS_DX11_MANDELBULB_MAIN_HPP
#define FILE_ALTONA2_EXAMPLES_GRAPHICS_DX11_MANDELBULB_MAIN_HPP

#include "altona2/libs/base/base.hpp"
#include "altona2/libs/util/debugpainter.hpp"
#include "Shader.hpp"

using namespace Altona2;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class App : public sApp
{
    sAdapter *Adapter;
    sContext *Context;
    sScreen *Screen;

    sCBuffer<sFixedMaterialLightVC> *cbv0;
    sDebugPainter *DPaint;

    sResource *BigVB;
    sResource *BigIB;
    sResource *CountVC;
    sResource *CountIC;
    sShader *CShader;
    sGeometry *CGeo;
    sMaterial *CMtrl;
    sCBuffer<ComputeShader_csb0> *Csb0;

    sResource *IndirectBuffer;
    sShader *IndirectShader;

public:
    App();
    ~App();

    void OnInit();
    void OnExit();
    void OnFrame();
    void OnPaint();
    void OnKey(const sKeyData &kd);
    void OnDrag(const sDragData &dd);
};

/****************************************************************************/

#endif  // FILE_ALTONA2_EXAMPLES_GRAPHICS_DX11_MANDELBULB_MAIN_HPP

