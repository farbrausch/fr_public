/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_BASE_DXSHADERCOMPILER_HPP
#define FILE_ALTONA2_LIBS_BASE_DXSHADERCOMPILER_HPP

#include "Altona2/Libs/Base/Base.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sShaderBinary *sCompileShaderDX(sShaderTypeEnum type,const char *profile,const char *source,sTextLog *errors);
void sDisassembleShaderDX(sShaderBinary *bin,sTextLog *log);

#if sConfigPlatform != sConfigPlatformWin

inline sShaderBinary *sCompileShaderDX(sShaderTypeEnum type,const char *profile,const char *source,sTextLog *errors)
{ return 0; }
inline void sDisassembleShaderDX(sShaderBinary *bin,sTextLog *log)
{ }


#endif

/****************************************************************************/

};
#endif  // FILE_ALTONA2_LIBS_BASE_DXSHADERCOMPILER_HPP

