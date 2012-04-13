// This code is in the public domain. See LICENSE for details.

#pragma once

// rg2 engine definition

#ifndef __RG2__ENGINE_H__
#define __RG2__ENGINE_H__

#include "types.h"

class frModel;
class frMaterial;
class frObject;

namespace fr
{
  struct matrix;
}

class frRG2Engine
{
public:
  frRG2Engine();
  ~frRG2Engine();

  enum ePaintFlags
  {
    pfWireframe     = 0x01,
    pfShowCull      = 0x02,
    pfShowVirtObjs  = 0x04,
  };

  void    paintModel(const frModel* model, const fr::matrix& cam, sF32 w, sF32 h, sU32 paintFlags);
	void		setMaterial(const frMaterial* mat);
	void		resetStates();

  void    clearStats();
  void    getStats(sU32& vertCount, sU32& lineCount, sU32& triCount,
		sU32& partCount, sU32& objCount, sU32& staticObjCount, sU32& mtrlChCount);

private:
  struct privateData;

  privateData* priv;

  friend void initEngine();
  friend void doneEngine();
};

extern frRG2Engine* g_engine;

extern void initEngine();
extern void doneEngine();

#endif
