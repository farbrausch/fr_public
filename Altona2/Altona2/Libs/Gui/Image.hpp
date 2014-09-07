/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_GUI_IMAGE_HPP
#define FILE_ALTONA2_LIBS_GUI_IMAGE_HPP

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Gui/Gui.hpp"
#include "Altona2/Libs/Util/Image.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

// it does not take ownership

class sImageWindow : public sWindow
{
    sImage *Image;
    sFImage *FImage;
    int Flags;
    int Format;
public:
    sImageWindow();
    ~sImageWindow();

    void SetImage(sImage *img,int flags=0,int format=sRF_BGRA8888);
    void SetImage(sFImage *img,int flags=0,int format=sRF_BGRA8888);
    void UpdateImage();

    void OnCalcSize();
    void OnPaint(int layer);

    sPainterImage *Painter;
};

/****************************************************************************/
}
#endif  // FILE_ALTONA2_LIBS_GUI_IMAGE_HPP

