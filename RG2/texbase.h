// This code is in the public domain. See LICENSE for details.

#ifndef __texbase_h_
#define __texbase_h_

#include "types.h"
#include "plugbase.h"
#include "sync.h"

struct frTexture
{
  sU32  w, h;
  sU32  handle;
  sU16* data;
  sBool recalc;
  
  frTexture();
  frTexture(sU32 iw, sU32 ih);
  frTexture(frTexture& x, sBool copy=sFALSE);
  ~frTexture();
  
  void    resize(sU32 nw, sU32 nh);
  void    copy(frTexture& x);
  
  sU16    *lock();
  void    unlock();
};

class frTexturePlugin: public frPlugin
{
protected:
  frTexture*    data;
  fr::cSection  processLock;

public:
  frTexturePlugin(const frPluginDef* d, sInt nLinks=0);
  virtual ~frTexturePlugin();

  virtual sBool     process(sU32 counter = 0);
  void              resize(sU32 iw, sU32 ih);
  inline frTexture* getData()                                 { return data; }
  inline frTexture* getInputData(sU32 i)                      { frPlugin* in = getInput(i); return (in && in->def->type == 0) ? static_cast<frTexturePlugin*>(in)->getData() : 0; }

  sInt              targetWidth, targetHeight;
};

extern void frInitTextureBase();
extern void frCloseTextureBase();

#endif
