// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __SHADERCODEGEN_HPP_
#define __SHADERCODEGEN_HPP_

#include "_types.hpp"

/****************************************************************************/

struct sShaderCodeGen
{
  enum { MAXTEMP = 256, MAXTEMP2 = 512, MAXNEST = 32 };
  enum { MAXPHYSREG = 12, MAXINDEX = 16 };
  enum { MAXOUTINSTR = 512, LONGESTINSTR = 6 };
  enum { MAXCODEWORDS = MAXOUTINSTR*LONGESTINSTR };

  struct Control;

  const sU32 *DataArea;
  sU8 Index[MAXINDEX];
  sBool IsV1Shader;
  sBool IsPS11;

  sU32 CodeLen;     // in words
  sU32 *FirstInstr; // pointer to first actual instruction in code stream
  sU32 *LifeEnd[MAXTEMP2];
  sU32 Code[MAXCODEWORDS];

  sU32 GetFlagWord(sU32 addr);
  sBool EvalCond(sU32 condition);
  sBool Phase1(const sU32 *input);
  sBool Phase2();
  void RemoveOpcodeLengths();

public:
  // frontend
  sU32 *GenCode(const sU32 *input,const sU32 *data);
};

/****************************************************************************/

#endif
