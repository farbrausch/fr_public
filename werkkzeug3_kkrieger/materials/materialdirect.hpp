// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __MATERIALDIRECT_HPP__
#define __MATERIALDIRECT_HPP__

#include "_types.hpp"
#include "_start.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Direct Material System (Raw Shaders+States)                        ***/
/***                                                                      ***/
/****************************************************************************/

class sMaterialDirect : public sMaterial
{
  sMaterialInstance Instance;
  sInt Setup;

public:
  sMaterialDirect(const sU32 *vShader,const sU32 *pShader,const sU32 *states,
    const sU32 *codegenData);
  ~sMaterialDirect();

  void SetInstance(const sMaterialInstance &inst);

  void Set(const sMaterialEnv &env);
};

#endif
