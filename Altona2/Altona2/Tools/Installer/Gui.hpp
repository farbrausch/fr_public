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
    sTextBuffer StyleChoice;

public:
    MainWindow();
    void OnInit();
    ~MainWindow();

    void SetPara();
    void CmdAction();

    // config

    bool UseCustomPath;
    sString<sMaxPath> UserPath;
    sString<sMaxPath> CustomPath;
    int DefaultStyle;
};

/****************************************************************************/


/****************************************************************************/

#endif  // FILE_ALTONA2_TOOLS_FONTTOOL_GUI_HPP

