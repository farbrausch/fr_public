// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __MATERIAL20_HPP__
#define __MATERIAL20_HPP__

#include "_types.hpp"
#include "_start.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   2.0 Material System                                                ***/
/***                                                                      ***/
/****************************************************************************/

struct sMaterial20Para
{
  // general section
  sU32 Flags;
  sU32 Diffuse;
  sU32 Specular;
  sF32 SpecularPow;
  sF32 ParalaxStrength;
  sInt _pad[3];

  // texture section
  sU32 TexFlags[4];
  sF32 TexScale[4];

  // bump/lighting section
  sU32 LgtFlags[4];
  sF32 LgtScale[4];

  // uv transform
  sF32 SRT1[9];
  sF32 SRT2[5];

  // internals (for codegen)
  sInt TexUsed[4];
  sInt LgtUsed[4];
  sU32 SrcScale[4];
  sU32 SrcScale2[4];
};

class sMaterial20Base : public sMaterial
{
protected:
  sInt Setup;
  sInt Tex[8];
  sInt TexSet[8];
  sInt TexCount;

  sMaterial20Para Para;
  sBool UseSRT;

  sMaterial20Base(const sMaterial20Para &para);
  ~sMaterial20Base();

  void DefaultStates(sU32 *&states,sBool alphaTest,sBool zFill,sBool stencilTest,sInt alphaBlend);
  void AddSampler(sU32 *&states,sInt handle,sU32 flags);
  void UseTex(sU32 *&states,sInt index,sInt *handles);
  void Compile(const sU32 *states,const sU32 *vs,const sU32 *ps);

  void TexTransformMat(sMatrix &mat);
};

class sMaterial20ZFill : public sMaterial20Base
{
public:
  sMaterial20ZFill(const sMaterial20Para &para,sInt *tex);

  void Set(const sMaterialEnv &env);
};

class sMaterial20Tex : public sMaterial20Base
{
public:
  sMaterial20Tex(const sMaterial20Para &para,sInt *tex);

  void Set(const sMaterialEnv &env);
};

class sMaterial20Light : public sMaterial20Base
{
  sU32 TexTarget;

public:
  sMaterial20Light(const sMaterial20Para &para,sInt *tex,sU32 texTarget);

  void Set(const sMaterialEnv &env);
};

class sMaterial20Fat : public sMaterial20Base
{
public:
  sMaterial20Fat(const sMaterial20Para &para,sInt *tex);

  void Set(const sMaterialEnv &env);
};

class sMaterial20Envi : public sMaterial20Base
{
public:
  sMaterial20Envi(const sMaterial20Para &para,sInt *tex);

  void Set(const sMaterialEnv &env);
};

class sMaterial20VColor : public sMaterial20Base
{
public:
  sMaterial20VColor(const sMaterial20Para &para,sInt *tex);

  void Set(const sMaterialEnv &env);
};

/****************************************************************************/

#endif
