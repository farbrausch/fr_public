// This code is in the public domain. See LICENSE for details.

#include "stdafx.h"
#include "texbase.h"
#include "debug.h"
#include "WindowMapper.h"
#include "resource.h"
#include "viewcomm.h"
#include "memmgr.h"
#include "frOpGraph.h"
#include <string.h>

static fr::bufferMemoryManager  *texMemMgr = 0;

// ---- frTexture

frTexture::frTexture()
  : w(0), h(0), handle(0), data(0), recalc(sFALSE)
{
  w = h = 0;
  handle = 0;
  data = 0;
}

frTexture::frTexture(sU32 iw, sU32 ih)
  : w(0), h(0), handle(0), data(0), recalc(sFALSE)
{
  resize(iw, ih);
}

frTexture::frTexture(frTexture& x, sBool copy)
  : w(0), h(0), handle(0), data(0), recalc(sFALSE)
{
  resize(x.w, x.h);
  if (copy)
    this->copy(x);
}

frTexture::~frTexture()
{
  if (handle)
    texMemMgr->dealloc(handle);
}

void frTexture::resize(sU32 nw, sU32 nh)
{
  if (nw != w || nh != h)
  {
    if (handle)
      texMemMgr->dealloc(handle);

    w = nw;
    h = nh;

    handle = texMemMgr->alloc(w * h * 4 * sizeof(sU16));
  }
}

void frTexture::copy(frTexture &x)
{
  ATLASSERT(w == x.w && h == x.h);

  lock();
  x.lock();

  memcpy(data, x.data, w * h * 4 * sizeof(sU16));

  unlock();
  x.unlock();

  recalc = sFALSE;
}

sU16* frTexture::lock()
{
  sBool lost;

  data = (sU16 *) texMemMgr->lock(handle, lost);
  recalc |= lost;

  return data;
}

void frTexture::unlock()
{
  texMemMgr->unlock(handle);
}

// ---- frTexturePlugin

frTexturePlugin::frTexturePlugin(const frPluginDef* d, sInt nLinks)
  : frPlugin(d, nLinks), data(0)
{
  resize(256, 256);
  clearParams();
}

frTexturePlugin::~frTexturePlugin()
{
  if (!def->isNop)
    FRSAFEDELETE(data);
}

void frTexturePlugin::resize(sU32 iw, sU32 ih)
{
  if (!def->isNop)
  {
    if (data)
      data->resize(iw, ih);
    else
      data = new frTexture(iw, ih);
  }

  targetWidth = iw;
  targetHeight = ih;
  dirty = sTRUE;
}

sBool frTexturePlugin::process(sU32 counter)
{
  sU32  i;
  sU32  numIn = 0;
  fr::cSectionLocker locker(processLock);
  
  if (counter>=4096) // bei dieser tiefe ist es fast definitive eine schleife
    return sFALSE;

  sU32 refWidth = 0, refHeight = 0;
  
  for (i=0; i < nTotalInputs; i++)
  {
    frLink* lnk = input + i;
    frPlugin* inPlg = lnk->plg;

    if (!inPlg && lnk->opID)
    {
      frOpGraph::opMapIt it = g_graph->m_ops.find(lnk->opID);
      
      if (it != g_graph->m_ops.end())
      {
        lnk->plg = it->second.plg;
        inPlg = it->second.plg;
      }
    }

    if (inPlg && inPlg->def->type == 0)
    {
      if (!inPlg->process(def->isNop ? counter : counter + 1))
        return sFALSE;

      frTexture* data = static_cast<frTexturePlugin*>(inPlg)->getData();
      if (!data)
        continue;

      if (data->w != refWidth || data->h != refHeight)
      {
        if (!i)
        {
          refWidth = data->w;
          refHeight = data->h;
        }
        else
          return sFALSE;
      }
      
      const sU32 inputRevision = static_cast<frTexturePlugin*>(inPlg)->revision;
      if (revision <= inputRevision)
        dirty = sTRUE;

      if (i < def->nInputs)
        numIn++;
    }
  }
  
  if (numIn < def->minInputs)
    return sFALSE;

	if (revision == 0)
		dirty = sTRUE; // newly created ops are always dirty.

  // nop operators use a slightly different processing
  if (def->isNop)
  {
    if (dirty)
    {
      dirty = sFALSE;

      if (!doProcess())
      {
        dirty = sTRUE;
        return sFALSE;
      }

      updateRevision();
    }

    return sTRUE;
  }

  frTexture* myData = getData();
  if (!myData)
    return sFALSE;

  if (def->nInputs == 0 && !def->isSystem) // generator?
  {
    refWidth = targetWidth;
    refHeight = targetHeight;
  }

  if (refWidth != myData->w || refHeight != myData->h)
    resize(refWidth, refHeight);

  sBool     locked=sFALSE;

  if (dirty || counter==0)
  {
    myData->lock(); // make sure we got space for our data
    dirty |= myData->recalc;

    locked = sTRUE;
  }
  
  if (dirty)
  {
    static sChar msg[256];
    sprintf(msg, "processing %s", def->name);
    ::PostMessage(g_winMap.Get(IDR_MAINFRAME), WM_SET_ACTION_MESSAGE, 0, (LPARAM) msg);

    dirty = sFALSE;
  
    for (i = 0; i < nTotalInputs; i++)
    {
      frPlugin* inPlg = input[i].plg;

      if (inPlg && inPlg->def->type == 0)
      {
        inPlg->process(0);

        frTexture* data = static_cast<frTexturePlugin*>(inPlg)->getData();
        if (data)
          data->lock();
      }
    }

    if (!doProcess())
      dirty = sTRUE;
  
    if(this == (frTexturePlugin *) 0x00f00cb0)
      fr::debugOut("processed starget real dirty %d\n",numIn);
    updateRevision();

    for (i = 0; i < nTotalInputs; i++)
    {
      frPlugin* inPlg = input[i].plg;

      if (inPlg && inPlg->def->type == 0)
      {
        frTexture *data = static_cast<frTexturePlugin*>(inPlg)->getData();
        if (data)
          data->unlock();
      }
    }
  }
  
  if (locked)
  {
    myData->recalc = sFALSE;
    myData->unlock();
  }

  return sTRUE;
}

// ---- interface routines

void frInitTextureBase()
{
  texMemMgr = new fr::poolBufferMemoryManager(96*1048576L);
}

void frCloseTextureBase()
{
  delete texMemMgr;
}
