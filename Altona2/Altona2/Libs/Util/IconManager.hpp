/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_UTIL_ICONMANAGER_HPP
#define FILE_ALTONA2_LIBS_UTIL_ICONMANAGER_HPP

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Util/Painter.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

struct sIcon
{
    bool Loaded;
    sRect Uv;
    
    sIcon() { Loaded = false; }
    void Init() { Loaded = false; Uv.Set(0,0,0,0); }
};

class sIconManager
{
    struct sIconPrivate
    {
        sString<64> Name;
        sRect Uv;
        sImage *Image;
        int SizeX;
        int SizeY;
    };

    sArray<sIconPrivate> Icons;
    sImage *Atlas;
public:
    sIconManager();
    ~sIconManager();

    bool Load(const char *path);
    sPainterImage *GetAtlas(sAdapter *ada);
    bool GetIcon(const char *label,sIcon &icon);
    bool GetIcon(const char *label,sIcon &icon,const char *&tooltip);
    bool GetIcon(const char *label,int labellen,sIcon &icon,const char *&tooltip,int &tooltiplen);

    bool Loaded;
};

/****************************************************************************/
} // namespace Util
#endif  // FILE_ALTONA2_LIBS_UTIL_ICONMANAGER_HPP

