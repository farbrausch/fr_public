// This code is in the public domain. See LICENSE for details.

#include "stdafx.h"

#include "types.h"
#include "tool.h"
#include "plugbase.h"
#include "debug.h"
#include "tstream.h"
#include <string.h>

static sU32 globalRevCounter = 0;

// ---- toolstuff

fr::streamWrapper& operator << (fr::streamWrapper& strm, frColor& col)
{
  return strm << col.r << col.g << col.b;
}

fr::streamWrapper& operator << (fr::streamWrapper& strm, frPoint& p)
{
  return strm << p.x << p.y;
}

fr::streamWrapper& operator << (fr::streamWrapper& strm, frTFloat& t)
{
  return strm << t.a << t.b;
}

fr::streamWrapper& operator << (fr::streamWrapper& strm, fr3Float& t)
{
  return strm << t.a << t.b << t.c;
}

fr::streamWrapper& operator << (fr::streamWrapper& strm, frSelect& s)
{
  return strm << s.sel;
}

fr::streamWrapper& operator << (fr::streamWrapper& strm, frFColor& c)
{
  return strm << c.r << c.g << c.b;
}

fr::streamWrapper& operator << (fr::streamWrapper& strm, frLink*& link)
{
  // links aren't really stored, they are resolved by the load/store code by the dependency graph anyway!
  return strm;
}

// ---- frPlugin

frPlugin::frPlugin(const frPluginDef* d, sInt nLinks)
{
  def = d;
  if (d->nInputs || nLinks)
    input = new frLink[d->nInputs + nLinks];
  else
    input = 0;

  dirty = sTRUE;
  revision = 0;

  params = 0;
  nParams = 0;
  nTotalInputs = d->nInputs + nLinks;
  nInputIdx = d->nInputs;

  for (sU32 i = 0; i < nTotalInputs; i++)
  {
    input[i].opID = 0;
    input[i].plg = 0;
  }
}

frPlugin::~frPlugin()
{
  FRSAFEDELETEA(input);

  clearParams();
  def = 0;
}

void frPlugin::clearParams()
{
  FRSAFEDELETEA(params);
  nParams = 0;
  nInputIdx = def->nInputs;
}

sInt frPlugin::addParam()
{
  frParam* oldParams = params;

  params = new frParam[++nParams];
  if (oldParams)
  {
    for (sInt i = 0; i < nParams - 1; i++)
      params[i] = oldParams[i];
    
    delete[] oldParams;
  }

  params[nParams-1].animIndex = 0;

  return nParams - 1;
}

sInt frPlugin::addColorParam(const sChar* desc, sU32 defColor)
{
  sInt ndx = addParam();

  params[ndx].type = frtpColor;
  params[ndx].desc = desc;
  params[ndx].colorv.r = (defColor >> 16) & 0xff;
  params[ndx].colorv.g = (defColor >> 8)  & 0xff;
  params[ndx].colorv.b = defColor         & 0xff;

  return ndx;
}

sInt frPlugin::addColorParam(const sChar* desc, const frColor& defColor)
{
  return addColorParam(desc, ((sU32) defColor.r << 16) | ((sU32) defColor.g << 8) | ((sU32) defColor.b));
}

sInt frPlugin::addPointParam(const sChar* desc, sF32 x, sF32 y, sBool clipped)
{
  sInt ndx = addParam();

  params[ndx].type = frtpPoint;
  params[ndx].desc = desc;
  params[ndx].pointv.x = x;
  params[ndx].pointv.y = y;
  params[ndx].clip = clipped;

  return ndx;
}

sInt frPlugin::addSelectParam(const sChar* desc, const sChar* opts, sInt sel)
{
  sInt ndx = addParam();

  params[ndx].type = frtpSelect;
  params[ndx].desc = desc;
  params[ndx].selectv.opts = opts;
  params[ndx].selectv.sel = sel;
  
  return ndx;
}

sInt frPlugin::addIntParam(const sChar* desc, sInt val, sInt min, sInt max, sF32 step)
{
  sInt ndx = addParam();

  params[ndx].type = frtpInt;
  params[ndx].desc = desc;
  params[ndx].intv = val;
  params[ndx].min = min;
  params[ndx].max = max;
  params[ndx].step = step;
  params[ndx].prec = 0;

  return ndx;
}

sInt frPlugin::addFloatParam(const sChar* desc, sF32 val, sF32 min, sF32 max, sF32 step, sInt prec)
{
  sInt ndx = addParam();
  
  params[ndx].type = frtpFloat;
  params[ndx].desc = desc;
  params[ndx].floatv = val;
  params[ndx].min = min;
  params[ndx].max = max;
  params[ndx].step = step;
  params[ndx].prec = prec;
  
  return ndx;
}

sInt frPlugin::addStringParam(const sChar* desc, sInt len, const sChar* val)
{
  sInt ndx = addParam();

  params[ndx].type = frtpString;
  params[ndx].desc = desc;
  params[ndx].maxLen = len;
  params[ndx].stringv = val;

  return ndx;
}

sInt frPlugin::addTwoFloatParam(const sChar* desc, sF32 val1, sF32 val2, sF32 min, sF32 max, sF32 step, sInt prec)
{
  sInt ndx = addParam();
  
  params[ndx].type = frtpTwoFloat;
  params[ndx].desc = desc;
  params[ndx].tfloatv.a = val1;
  params[ndx].tfloatv.b = val2;
  params[ndx].min = min;
  params[ndx].max = max;
  params[ndx].step = step;
  params[ndx].prec = prec;
  
  return ndx;
}

sInt frPlugin::addThreeFloatParam(const sChar* desc, sF32 val1, sF32 val2, sF32 val3, sF32 min, sF32 max, sF32 step, sInt prec)
{
  sInt ndx = addParam();
  
  params[ndx].type = frtpThreeFloat;
  params[ndx].desc = desc;
  params[ndx].trfloatv.a = val1;
  params[ndx].trfloatv.b = val2;
  params[ndx].trfloatv.c = val3;
  params[ndx].min = min;
  params[ndx].max = max;
  params[ndx].step = step;
  params[ndx].prec = prec;
  
  return ndx;
}

sInt frPlugin::addFloatColorParam(const sChar* desc, sF32 r, sF32 g, sF32 b)
{
  sInt ndx = addParam();

  params[ndx].type = frtpFloatColor;
  params[ndx].desc = desc;
  params[ndx].fcolorv.r = r;
  params[ndx].fcolorv.g = g;
  params[ndx].fcolorv.b = b;
  
  return ndx;
}

sInt frPlugin::addLinkParam(const sChar *desc, sU32 defaultOp, sInt opType)
{
  sInt ndx = addParam();

  params[ndx].type = frtpLink;
  params[ndx].desc = desc;
  params[ndx].linkv = input + nInputIdx;
  params[ndx].linkv->opID = defaultOp;
  params[ndx].linkv->plg = 0;
  params[ndx].maxLen = opType;
  nInputIdx++;

  return ndx;
}

void frPlugin::updateRevision()
{
  revision = globalRevCounter++;
}

sBool frPlugin::onParamChanged(sInt param)
{
  return sFALSE;
}

sBool frPlugin::doProcess()
{
  return sFALSE;
}

sBool frPlugin::process(sU32 counter)
{
  return sFALSE;
}

sBool frPlugin::onConnectUpdate()
{
  return sFALSE;
}

const sChar* frPlugin::getDisplayName() const
{
  return def->name;
}

void frPlugin::setInput(sU32 index, sU32 refID)
{
  if (index < def->nInputs)
  {
    if (input[index].opID != refID)
    {
      input[index].opID = refID;
      input[index].plg = 0;
      
      dirty = sTRUE;
    }
  }
}

sBool frPlugin::setParam(sU32 i, frParam* p)
{
  sBool ret = sFALSE;
  
  if (i < nParams)
  {
    ATLASSERT(p == (params+i));
    
    ret = onParamChanged(i);
    dirty = sTRUE;
  }
  
  return ret;
}

void frPlugin::markNew()
{
  dirty = sTRUE;
}

void frPlugin::setAnim(sInt index, const sF32* vals)
{
}

void frPlugin::serialize(fr::streamWrapper& strm)
{
}

void frPlugin::exportTo(fr::stream& f, const frGraphExporter& exp)
{
}

sInt frPlugin::getButtonType()
{
  if (def->nInputs == 0)
    return 0; // generator
  else if (def->nInputs == 1)
    return 1; // filter
  else
    return 2; // combiner
}

sBool frPlugin::displayParameter(sU32 index)
{
  return sTRUE;
}

// ---- housekeeping

static frPluginDef* plugins = 0;
static sInt         nPlugins = 0, maxPlugins = 0;

int __cdecl defCompareFunc(const void* elem1, const void* elem2)
{
  const frPluginDef* d1=(const frPluginDef*) elem1;
  const frPluginDef* d2=(const frPluginDef*) elem2;

  return stricmp(d1->name, d2->name);
}

void frRegisterPlugin(frPluginDef* d)
{
  if (d->ID < 0)
  {
    d->isVisible = sFALSE;
    d->ID = -d->ID;
  }
  else
    d->isVisible = sTRUE;
  
  for (sInt i = 0; i < nPlugins; i++)
    FRASSERT(plugins[i].ID != d->ID);

  if (nPlugins == maxPlugins)
    plugins = (frPluginDef*) realloc(plugins, (maxPlugins += 8) * sizeof(frPluginDef));

  plugins[nPlugins] = *d;
  nPlugins++;

  qsort(plugins, nPlugins, sizeof(frPluginDef), defCompareFunc);
}

frPluginDef* frGetPluginByIndex(sInt i)
{
  return (i < nPlugins) ? plugins + i : 0;
}

frPluginDef* frGetPluginByID(sU32 id)
{
  for (sInt i = 0; i < nPlugins; i++)
  {
    if (plugins[i].ID == id)
      return plugins + i;
  }

  return 0;
}

struct pluginDeInit
{
  ~pluginDeInit()
  {
    free(plugins);
  }
};

static pluginDeInit pdi;

// ---- frPluginDefIterator

frPluginDefIterator::frPluginDefIterator()
	: index(0)
{
}

frPluginDefIterator& frPluginDefIterator::operator ++()
{
	index++;
	return *this;
}

const frPluginDef* frPluginDefIterator::operator ->() const
{
	return (index < nPlugins) ? plugins + index : 0;
}

frPluginDefIterator::operator const frPluginDef* () const
{
	return (index < nPlugins) ? plugins + index : 0;
}

sInt frPluginDefIterator::getIndex() const
{
	return index;
}
