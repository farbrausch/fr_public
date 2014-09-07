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

class MainWindow;
class wTextWindow : public sWindow
{
    MainWindow *App;
public:
    wTextWindow(MainWindow *app) : App(app) {}
    
    void OnPaint(int layer);

};


class MainWindow : public sPassthroughFrame
{
    sGridFrame *ParaWin;
//    sTextWindow *TextWin;
    wTextWindow *TextWin;

    sArray<const char *>FontNames;

    sString<256> FontName;
    int FontSize;
    int FontFlags;

public:
    MainWindow();
    void OnInit();
    ~MainWindow();

    void SetPara();
    void CmdUpdateFontResource();
    void CmdUpdateFont();

    // shared with wTextWin

    sPainterFont *OldFont;
    sFont *Font;
    float Zoom;
    float Blur;
    float Outline;
    sTextBuffer Text;
    int PaintFlags;
};

/****************************************************************************/


/****************************************************************************/

#endif  // FILE_ALTONA2_TOOLS_FONTTOOL_GUI_HPP

