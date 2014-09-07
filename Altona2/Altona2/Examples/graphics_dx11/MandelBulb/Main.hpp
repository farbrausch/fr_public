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
    sFixedMaterial *LineMtrl;

    sResource *BigVB;
    sResource *BigIB;
    sResource *CountVC;
    sResource *CountIC;
    sResource *CountNC;
    sResource *McIndexMap;
    sResource *TempBuffer;
    sResource *CNodes;
    sShader *CShader;
    sGeometry *CGeo;
    sMaterial *CMtrl;
    sCBuffer<ComputeShader_csb0> *Csb0;

    sResource *IndirectBuffer;
    sShader *IndirectShader;

    void Register(int x,int y,int z,int s,uint *&data);
    sVector41 Trans(int x,int y,int z,int s);
    float Trans(int x);
    float Map(sVector41 pos);
    float MapTorus(sVector41 pos);
    float MapBox(sVector41 pos);
    float MapBulb(sVector41 pos);
    float MapTemple(sVector41 pos);
public:
    App();
    ~App();

    void OnInit();
    void OnExit();
    void OnFrame();
    void OnPaint();
    void OnKey(const sKeyData &kd);
    void OnDrag(const sDragData &dd);

    sViewport view;
    float PosMul;
    float PosAdd;
    sMatrix44A Matrix;
    static const int GroupDim = 32;
    static const int ThreadDim = 16-1;
    static const int MaxNodes = 256*1024;
    static uint NodeData[MaxNodes*4+256];
    static const int BufferPerGroup = 256*1024;
    static const int GroupCount = 256;
};

/****************************************************************************/

#endif  // FILE_ALTONA2_EXAMPLES_GRAPHICS_DX11_MANDELBULB_MAIN_HPP

