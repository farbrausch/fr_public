/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_BASE_SYSTEM_POSIX_HPP
#define FILE_ALTONA2_LIBS_BASE_SYSTEM_POSIX_HPP

#include "Altona2/Libs/Base/Base.hpp"

namespace Altona2 {

#if sConfigPlatform==sConfigPlatformLinux || sConfigPlatform==sConfigPlatformOSX || sConfigPlatform==sConfigPlatformIOS || sConfigPlatform==sConfigPlatformAndroid

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class sThreadPrivate
{
public:
  sThreadPrivate();
};

/****************************************************************************/

class sThreadLockPrivate
{
public:
  void *lock;
};

/****************************************************************************/

class sThreadEventPrivate
{
public:
  sThreadEventPrivate();
};

/****************************************************************************/

class sSharedMemoryPrivate  // dummy
{
protected:
};

/****************************************************************************/

#endif  // sConfigPlatform==sConfigPlatformLinux || sConfigPlatform==sConfigPlatformOSX || sConfigPlatform==sConfigPlatformIOS

}

#endif  // FILE_ALTONA2_LIBS_BASE_SYSTEM_POSIX_HPP
