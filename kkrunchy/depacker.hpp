// Written by Fabian "ryg" Giesen.
// I hereby place this code in the public domain.

#ifndef __DEPACKER_HPP_
#define __DEPACKER_HPP_

#include "_types.hpp"
#include "packer.hpp"

/****************************************************************************/

sU32 CCADepacker(sU8 *dst,const sU8 *src,TokenizeCallback cb,void *cbuser);
extern "C" sU32 __stdcall CCADepackerA(sU8 *dst,const sU8 *src);

/****************************************************************************/

#endif
