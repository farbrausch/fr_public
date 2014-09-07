/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_GUI_STYLE_DARK_HPP
#define FILE_ALTONA2_LIBS_GUI_STYLE_DARK_HPP

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Gui/Style.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class sGuiStyle_Flat : public sGuiStyle
{
    bool Tiny;
public:
    sGuiStyle_Flat(sGuiAdapter *ada,bool tiny=0);
    void Rect(int layer,class sWindow *w,sGuiStyleKind style,const sRect &r_,int modifier,const char *text,uptr len,const char *extrastring);
    void SetColors();
};

/****************************************************************************/
}

/****************************************************************************/

#endif  // FILE_ALTONA2_LIBS_GUI_STYLE_DARK_HPP

