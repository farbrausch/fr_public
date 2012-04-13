// This code is in the public domain. See LICENSE for details.

#ifndef __cmpbase_h_
#define __cmpbase_h_

#include "types.h"
#include "plugbase.h"

struct composeViewport
{
  sInt				xstart, ystart;
  sInt				w, h;
	fr::matrix*	camOverride;
};

class frComposePlugin: public frPlugin
{
public:
  frComposePlugin(const frPluginDef* d, sInt nLinks=0);
  virtual ~frComposePlugin();
  
  virtual sBool process(sU32 counter=0);
  void          paint(const composeViewport& viewport, sF32 w, sF32 h);

protected:
  virtual void  doPaint(const composeViewport& viewport, sF32 w, sF32 h) = 0;
};


#endif
