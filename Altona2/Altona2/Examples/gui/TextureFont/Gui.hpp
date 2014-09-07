/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_TOOLS_FONTTOOL_GUI_HPP
#define FILE_ALTONA2_TOOLS_FONTTOOL_GUI_HPP

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Gui/Gui.hpp"
#include "Altona2/Libs/Util/UtilShaders.hpp"
#include "Altona2/Libs/Util/TextureFont.hpp"

using namespace Altona2;

class FontWindow;
class TextWindow;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class MainWindow : public sPassthroughFrame
{
    sGridFrame *ParaWin;
    FontWindow *FontWin;
    TextWindow *TextWin;
    sArray<const char *>FontNames;

    sTextureFontGenerator *FontGen;

    sString<256> Text;
    sTextureFontPara Para;
    int LastChar;

    void SetPara();
    void InitFont();
    void RenderGlyph();
    void SetShader();

    void CmdFont();
    void CmdFont2(int n);
public:
    MainWindow();
    void OnInit();
    ~MainWindow();

    void CmdSetPara();
    void CmdUpdate();
    void CmdEdit();

    sTextureFont *Font;
};

/****************************************************************************/

class FontWindow : public s3dWindow
{
    sCBuffer<sFixedMaterialVC> *cbv0;
    sCBuffer<sDistanceFieldShader_cbps> *cbp0;
    sFixedMaterial *FlatMtrl;
    sMaterial *DistMtrl;
    sContext *Context;
    MainWindow *App;

    float ZoomLin;
    float Zoom;
    float ScrollX;
    float ScrollY;

    float DragStartX;
    float DragStartY;
    float DragStart;

    bool UseDist;

    void DragZoom(const sDragData &dd);
    void DragScroll(const sDragData &dd);

    void CmdReset();
    void CmdDist() { UseDist = !UseDist; }
public:
    FontWindow(MainWindow *);
    ~FontWindow();
    void OnInit();
    void Render(const sTargetPara &tp);

    float Outline;
    float Blur;
    bool CorrectZoom;
};

/****************************************************************************/

class TextWindow : public s3dWindow
{
    sCBuffer<sFixedMaterialVC> *cbv0;
    sCBuffer<sDistanceFieldShader_cbps> *cbp0;
    sMaterial *DistMtrl;
    sContext *Context;

    MainWindow *App;
    
    float ZoomLin;
    float Zoom;
    float ScrollX;
    float ScrollY;

    float DragStartX;
    float DragStartY;
    float DragStart;

    void DragZoom(const sDragData &dd);
    void DragScroll(const sDragData &dd,int speed);

    void CmdReset();
public:
    TextWindow(MainWindow *);
    ~TextWindow();
    void OnInit();
    void Render(const sTargetPara &tp);

    sTextBuffer Text;
    sTextureFontPrintPara Para;
};

/****************************************************************************/

#endif  // FILE_ALTONA2_TOOLS_FONTTOOL_GUI_HPP

