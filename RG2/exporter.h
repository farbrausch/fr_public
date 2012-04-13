// This code is in the public domain. See LICENSE for details.

#ifndef __tg2_exporter_h_
#define __tg2_exporter_h_

#include "types.h"

class frPlugin;

namespace fr
{
  class stream;
}

class frGraphExporter
{
public:
  frGraphExporter();
  ~frGraphExporter();

  void        makeDemo(sInt xRes = 640, sInt yRes = 480);

  void        putLink(fr::stream& f, sU32 linkOp) const;

private:
  struct privateData;

  privateData *d;
};

#endif
