// This code is in the public domain. See LICENSE for details.

// rg2 intro runtime operator system

#ifndef __opsys_h_
#define __opsys_h_

#include "types.h"

struct frBitmap;
class frModel;
struct frOperatorDef;

namespace fvs
{
  class texture;
}

class frOperator
{
protected:
  virtual void        doProcess();
  sU32                revision;

public:
  const frOperatorDef*  def;

  frOperator**        input;
  sInt                nInputs;

  sBool               dirty;
  sBool               anim;
  sInt                opType;                     // 0=soft clone 1=semi-hard 2=hard (only really interesting for geo ops)
  sU32                refNum;

  const sU8*          param;

  frOperator();
  virtual ~frOperator();

  void                process();
  virtual const sU8*  readData(const sU8* strm);
  virtual void        freeData();
  virtual void        setAnim(sInt parm, const sF32* val);
};

class frTextureOperator: public frOperator
{
protected:
  frBitmap*           data;
  fvs::texture*       tex;
  sU32                texRev;

public:
  frTextureOperator();
  ~frTextureOperator();

  void                create(sU32 w, sU32 h);
  void                freeData();

  void                updateTexture();

  inline frBitmap*    getData() const             { return data; }
  inline frBitmap*    getInputData(sU32 i) const  { return ((frTextureOperator*) input[i])->getData(); }

  fvs::texture*       getTexture() const          { return tex; }
};

class frGeometryOperator: public frOperator
{
protected:
  frModel*            data;

public:
  frGeometryOperator();

  void                freeData();

  inline frModel*     getData() const             { return data; }
  inline frModel*     getInputData(sU32 i) const  { return ((frGeometryOperator*) input[i])->getData(); }
};

class frCompositingOperator: public frOperator
{
protected:
  virtual void        doPaint();

public:
  void                paint();
};

struct frOperatorDef
{
  sU8         id, nInputs, nLinks, dataBytes;
  frOperator* (*createFunc)();
};

// ---- op tree

extern const sU8* opsInitialize(const sU8* strm);
extern void       opsCalculate(void (__stdcall *poll)(sInt counter, sInt max));
extern void       opsCleanup();

extern sBool      opsAnimate(sInt clp, sInt startFrame, sInt endFrame);

#endif
