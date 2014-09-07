/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_TOOLS_PACKFILE_DOC_HPP
#define FILE_ALTONA2_TOOLS_PACKFILE_DOC_HPP

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Util/Scanner.hpp"

using namespace Altona2;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class wFile
{
public:
    wFile() { Size = 0; }
    ~wFile() {}
    sBool GetSize();
    sPoolString SourceName;
    sPoolString PackName;
    sS64 Size;
};

class wDocument
{
    sScanner Scan;
    sArray<wFile *> Files;
public:
    wDocument();
    ~wDocument();

    sBool Parse(const sChar *file);
    sBool Output(const sChar *file);
};

/****************************************************************************/

#endif  // FILE_ALTONA2_TOOLS_PACKFILE_DOC_HPP

