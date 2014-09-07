/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "altona2/libs/base/base.hpp"

void Altona2::Main()
{
    sArray<sDirEntry> dir;
    if(sLoadVolumes(dir))
    {
        for(auto &de : dir)
        {
            sPrintF("%s\n",de.Name);
        }
    }
    else
    {
        sDPrintF("Load Failed (not windows)");
    }
}

/****************************************************************************/
