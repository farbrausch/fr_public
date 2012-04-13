// This code is in the public domain. See LICENSE for details.

#include "stdafx.h"
#include "ViruzII.h"
#include "debug.h"
#include "ViruzII/soundsys.h"
#include "tool.h"
#include "WindowMapper.h"
#include "resource.h"

namespace fr
{
  // ---- viruzII

  struct viruzII::privateData
  {
    sBool             isInitialized;
    viruzIIPositions  *positions;

    privateData()
      : isInitialized(sFALSE), positions(0)
    {
    }
  };

  // -- construction/destruction
  
  viruzII::viruzII()
  {
    d=new privateData;
  }

  viruzII::~viruzII()
  {
    closeTune();

    delete d;
  }

  // -- tune management

  void viruzII::initTune(const sU8 *tune)
  {
    closeTune();

    ssInit(const_cast<sU8 *>(tune), g_winMap.Get(ID_MAIN_VIEW));
    d->isInitialized = sTRUE;
  }

  void viruzII::closeTune()
  {
    FRSAFEDELETE(d->positions);

    if (d->isInitialized)
    {
      ssClose();
      d->isInitialized=sFALSE;
    }
  }

  // -- play/stop handling

  void viruzII::play(sS32 from)
  {
    if (from==-1) // from point not specified?
      ssPlay();
    else
      ssPlayFrom(from);
  }

  void viruzII::stop()
  {
    ssStop();
  }

  // -- time management

  sS32 viruzII::getTime()
  {
    return ssGetTime();
  }

  sS32 viruzII::getSongTime()
  {
    return ssGetSongTime();
  }

  const viruzIIPositions *viruzII::getPositions()
  {
    if (!d->positions)
      d->positions=new viruzIIPositions;

    return d->positions;
  }

  // ---- viruzIIPositions

  struct viruzIIPositions::privateData
  {
    sS32    *positions;
    sU32    numPositions;
    sInt    lastBar;

    void init()
    {
      if (positions)
        lastBar=positions[(numPositions-1)*2+1]>>16;
      else
      {
        numPositions=0;
        lastBar=0;
      }
    }

    sInt findBar(sInt bar) const
    {
      sInt l=0, r=numPositions-1;
      sInt lb=0, rb=lastBar;
      sInt x, xb;
      
      do
      {
        x=(bar-lb)*(r-l)/(rb-lb);
        xb=positions[x*2+1]>>16;

        if (xb==bar)
          break;

        if (xb>bar)
        {
          l=x;
          lb=xb;
        }
        else
        {
          r=x;
          rb=xb;
        }
      } while (rb-lb>1);

      while (x>=1 && (positions[x*2-1]>>16)==bar)
        x--;

      return x;
    }
  };

  viruzIIPositions::viruzIIPositions()
  {
    d=new privateData;

    d->positions=0;
    d->numPositions=ssCalcPositions(&d->positions);

    if (!d->positions)
      d->numPositions=0;

    d->init();
  }

  viruzIIPositions::~viruzIIPositions()
  {
    delete[] d->positions;
    delete d;
  }

  sS32 viruzIIPositions::getBarTime(sInt bar) const
  {
    if (d->positions)
    {
      const sInt barIndex=d->findBar(bar);
      return d->positions[barIndex*2];
    }
    else
      return -1;
  }

  sS32 viruzIIPositions::getBarLen(sInt bar) const
  {
    if (d->positions)
    {
      const sInt barIndex=d->findBar(bar);
      const sS32 barTime=d->positions[barIndex*2];

      if (bar!=d->lastBar)
      {
        const sInt nextBarIndex=d->findBar(bar+1);
        return d->positions[nextBarIndex*2]-barTime;
      }
      else
      {
        const sS32 lastTime=d->positions[(d->numPositions-1)*2];
        const sInt lastPos=d->positions[(d->numPositions-1)*2+1];

        const sInt numTicks=lastPos & 0xff;
        const sInt tick=(lastPos >> 8) & 0xff;

        return (lastTime-barTime)*numTicks/tick;
      }
    }
    else
      return -1;
  }

  sInt viruzIIPositions::getNumPositions() const
  {
    return d->numPositions;
  }

  sS32 viruzIIPositions::getPosCode(sInt pos) const
  {
    if (pos<0 || pos>=d->numPositions)
      return -1;
    
    return d->positions[pos*2+1];
  }
    
  sS32 viruzIIPositions::getTime(sInt pos) const
  {
    if (pos<0 || pos>=d->numPositions)
      return -1;

    return d->positions[pos*2+0];
  }
}
