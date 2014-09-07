/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_GUI_STYLE_SMALL_HPP
#define FILE_ALTONA2_LIBS_GUI_STYLE_SMALL_HPP

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Gui/Style.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class sGuiStyle_Small : public sGuiStyle
{
    void Hilo(sPainter *painter,const sRect &r,uint col0,uint col1,int w=1);
    void BevelBox(sRect &r,int modifier,uint colormask=0xffffffff);
public:
    sGuiStyle_Small(sGuiAdapter *ada);
    void Rect(int layer,class sWindow *w,sGuiStyleKind style,const sRect &r_,int modifier,const char *text,uptr len,const char *extrastring);
};

/****************************************************************************/
}
#endif  // FILE_ALTONA2_LIBS_GUI_STYLE_SMALL_HPP

