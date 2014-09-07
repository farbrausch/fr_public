/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_ASL_TRANSFORM_HPP
#define FILE_ALTONA2_LIBS_ASL_TRANSFORM_HPP

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Asl/Tree.hpp"
#include "Altona2/Libs/Util/Scanner.hpp"

namespace Altona2 {
namespace Asl {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class sTransform
{
    sTree *Tree;
    sScanner *Scan;
    sIntrinsic *InMul;
    sIntrinsic *InDot;

    void Common();
    void Matrix();
    void Hlsl3();
    void Hlsl5();
    void Glsl1();
    void Glsl1Attr(sVariable *var,int storage);

public:
    sTransform();
    ~sTransform();

    void Load(sTree *,sScanner *);
    bool Transform(sTarget tar);
    void Unload();

};


/****************************************************************************/
}}
#endif  // FILE_ALTONA2_LIBS_ASL_TRANSFORM_HPP

