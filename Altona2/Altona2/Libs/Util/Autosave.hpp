/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_UTIL_AUTOSAVE_HPP
#define FILE_ALTONA2_LIBS_UTIL_AUTOSAVE_HPP

#include "Altona2/Libs/Base/Base.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void sAutosaveRotation(const sStringDesc &filename,const char *ext);

class sAutosave
{
    bool Changed;
    uint AutosaveTime;
    int AutosaveTimeout;
public:
    sAutosave();

    void SetChanged();
    bool GetChanged();
    void ClearChanged();
    bool NeedAutosave();
    void AutosaveDone();
};

/****************************************************************************/

}

#endif  // FILE_ALTONA2_LIBS_UTIL_AUTOSAVE_HPP

