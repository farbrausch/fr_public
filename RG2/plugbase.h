// This code is in the public domain. See LICENSE for details.

#ifndef __plugbase_h_
#define __plugbase_h_

#include "types.h"

class frPlugin;
class frGraphExporter;

namespace fr
{
  class streamWrapper;
}

struct frPluginDef
{
  sChar           *name;
  sU32            nInputs, minInputs;
  sBool           hasOutput, isSystem, isVisible, isNop;
  sS32            ID;
  sInt            type;
  
  frPlugin        *(*create)(const frPluginDef *d);
};

#define TG2_DECLARE_PLUGIN(clname, name, type, nInputs, minInputs, hasOutput, isSystem, ID) \
  namespace __tempns__##clname { \
    static frPlugin *createFunc(const frPluginDef *d) { return new clname(d); } \
    frPluginDef def={ name, nInputs, minInputs, hasOutput, isSystem, sFALSE, sFALSE, ID, type, createFunc }; \
    struct regClass { \
      regClass() { frRegisterPlugin(&def); } \
    }; \
    static regClass x; \
  }

#define TG2_DECLARE_NOP_PLUGIN(clname, name, type, nInputs, minInputs, hasOutput, isSystem, ID) \
  namespace __tempns__##clname { \
    static frPlugin *createFunc(const frPluginDef *d) { return new clname(d); } \
    frPluginDef def={ name, nInputs, minInputs, hasOutput, isSystem, sFALSE, sTRUE, ID, type, createFunc }; \
    struct regClass { \
      regClass() { frRegisterPlugin(&def); } \
    }; \
    static regClass x; \
  }

enum frParamType
{
  frtpColor=0,
  frtpInt=1,
  frtpFloat=2,
  frtpPoint=3,
  frtpSelect=4,
  frtpButton=5,
  frtpString=6,
  frtpSize=7, // don't use in operators
  frtpTwoFloat=8,
  frtpThreeFloat=9,
  frtpFloatColor=10,
  frtpLink=11,
};

struct frColor
{
  union
  {
    struct
    {
      sU8   b, g, r;
    };
    sU8   v[3];
  };
};

struct frPoint
{
  sF32  x, y;
};

struct frTFloat
{
  sF32  a, b;
};

struct fr3Float
{
  sF32  a, b, c;
};

struct frSelect
{
  const sChar*  opts;
  sInt          sel;
};

struct frFColor
{
  sF32  r, g, b;
};

struct frLink
{
  sU32      opID;
  frPlugin* plg;
};

fr::streamWrapper& operator << (fr::streamWrapper& strm, frColor& col);
fr::streamWrapper& operator << (fr::streamWrapper& strm, frPoint& p);
fr::streamWrapper& operator << (fr::streamWrapper& strm, frTFloat& t);
fr::streamWrapper& operator << (fr::streamWrapper& strm, fr3Float& t);
fr::streamWrapper& operator << (fr::streamWrapper& strm, frSelect& s);
fr::streamWrapper& operator << (fr::streamWrapper& strm, frFColor& c);
fr::streamWrapper& operator << (fr::streamWrapper& strm, frLink*& link);

struct frParam
{
  frParamType         type;
  const sChar*        desc;
  union
  {
    frColor     colorv;
    frPoint     pointv;
    frSelect    selectv;
    sInt        intv;
    sF32        floatv;
    frTFloat    tfloatv;
    fr3Float    trfloatv;
    frFColor    fcolorv;
    frLink*     linkv;
  };
  fr::string          stringv;
  sF64                min, max, step;
  union
  {
    sInt              prec;
    sInt              maxLen;
    sBool             clip;
  };
  sInt                animIndex;
};

class frPlugin
{
protected:
  sBool               dirty;
  sU32                revision;

  frLink*             input;
  
  sU32                nParams;
  frParam*            params;

  sU32                nTotalInputs;
  sU32                nInputIdx;

  void                clearParams();
  sInt                addParam();
  sInt                addColorParam(const sChar* desc, sU32 defColor);
  sInt                addColorParam(const sChar* desc, const frColor& defColor);
  sInt                addSelectParam(const sChar* desc, const sChar* opts, sInt sel);
  sInt                addIntParam(const sChar* desc, sInt val, sInt min, sInt max, sF32 step);
  sInt                addFloatParam(const sChar* desc, sF32 val, sF32 min, sF32 max, sF32 step, sInt prec);
  sInt                addStringParam(const sChar* desc, sInt len, const sChar *val);
  sInt                addPointParam(const sChar* desc, sF32 x, sF32 y, sBool clipped);
  sInt                addTwoFloatParam(const sChar* desc, sF32 val1, sF32 val2, sF32 min, sF32 max, sF32 step, sInt prec);
  sInt                addThreeFloatParam(const sChar* desc, sF32 val1, sF32 val2, sF32 val3, sF32 min, sF32 max, sF32 step, sInt prec);
  sInt                addFloatColorParam(const sChar* desc, sF32 r, sF32 g, sF32 b);
  sInt                addLinkParam(const sChar* desc, sU32 defaultOp, sInt opType);
  
  void updateRevision();

  virtual sBool onParamChanged(sInt param);
  virtual sBool doProcess();
  
public:
	const frPluginDef*	def;
  
  frPlugin(const frPluginDef* d, sInt nLinks = 0);
  virtual ~frPlugin();
  
  virtual sBool       process(sU32 counter = 0);
  virtual sBool       onConnectUpdate();
  
  sU32                getNParams()                { return nParams; }
  frParam*            getParam(sU32 i)            { return params + i; }

  sU32                getRevision()               { return revision; }

  frPlugin*           getInput(sU32 index) const  { return (index < nTotalInputs) ? input[index].plg : 0; }
  sU32                getInID(sU32 index) const   { return (index < nTotalInputs) ? input[index].opID : 0; }
  virtual const sChar* getDisplayName() const;
  sU32                getNTotalInputs() const     { return nTotalInputs; }

  void                setInput(sU32 index, sU32 refID);
  sBool               setParam(sU32 i, frParam* p);
  void                markNew();

  virtual void        setAnim(sInt index, const sF32* vals);
  
  virtual void        serialize(fr::streamWrapper& strm);
  virtual void        exportTo(fr::stream& f, const frGraphExporter& exp);
  
  virtual sInt        getButtonType();
  virtual sBool       displayParameter(sU32 index);
};

class frPluginDefIterator
{
	sInt index;

public:
	frPluginDefIterator();

	frPluginDefIterator& operator ++();
	const frPluginDef* operator ->() const;
	operator const frPluginDef* () const;

	sInt getIndex() const;
};

extern void frRegisterPlugin(frPluginDef* d);

extern frPluginDef* frGetPluginByIndex(sInt i);
extern frPluginDef* frGetPluginByID(sU32 id);

#endif
