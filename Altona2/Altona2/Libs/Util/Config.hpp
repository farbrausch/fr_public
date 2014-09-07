/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_UTIL_CONFIG_HPP
#define FILE_ALTONA2_LIBS_UTIL_CONFIG_HPP

#include "Altona2/Libs/Base/Base.hpp"
namespace Altona2 {
/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void sEnableAltonaConfig();
const char *sGetConfigString(const char *);
const char *sGetConfigPath();

/****************************************************************************/
}
#endif  // FILE_ALTONA2_LIBS_UTIL_CONFIG_HPP

