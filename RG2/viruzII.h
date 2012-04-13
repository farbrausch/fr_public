// This code is in the public domain. See LICENSE for details.

#ifndef __viruzII_h_
#define __viruzII_h_

#include "types.h"

namespace fr
{
  class viruzIIPositions;

  class viruzII
  {
  public:
    viruzII();
    ~viruzII();

    void                    initTune(const sU8 *tune);
    void                    closeTune();

    void                    play(sS32 from=-1);
    void                    stop();

    sS32                    getTime();
    sS32                    getSongTime();
    const viruzIIPositions  *getPositions();

  private:
    struct privateData;

    privateData   *d;
  };

  class viruzIIPositions
  {
  public:
    viruzIIPositions();
    ~viruzIIPositions();

    sS32                    getBarTime(sInt bar) const;
    sS32                    getBarLen(sInt bar) const;

    sInt                    getNumPositions() const;
    sS32                    getPosCode(sInt pos) const;
    sS32                    getTime(sInt pos) const;

  private:
    struct privateData;

    privateData   *d;
  };
}

#endif
