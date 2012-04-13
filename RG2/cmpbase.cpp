// This code is in the public domain. See LICENSE for details.

#include "stdafx.h"
#include "cmpbase.h"
#include "debug.h"
#include "viewcomm.h"
#include "WindowMapper.h"
#include "resource.h"
#include "frOpGraph.h"

// ---- frComposePlugin

frComposePlugin::frComposePlugin(const frPluginDef* d, sInt nLinks)
  : frPlugin(d, nLinks)
{
}

frComposePlugin::~frComposePlugin()
{
}

sBool frComposePlugin::process(sU32 counter)
{
  sU32  i;
  sU32  numIn;
  
  if (counter>=4096) // bei dieser tiefe ist es definitiv eine schleife
    return sFALSE;
  
  numIn=0;
  
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

    if (inPlg)
    {
      if (!inPlg->process(counter+1))
        return sFALSE;
      
      if (i < def->nInputs)
        numIn++;
    }
  }
  
  if (numIn<def->minInputs)
    return sFALSE;

  if (dirty)
  {
    static sChar msg[256];
    sprintf(msg, "processing %s", def->name);
    ::PostMessage(g_winMap.Get(IDR_MAINFRAME), WM_SET_ACTION_MESSAGE, 0, (LPARAM) msg);

    doProcess();
    dirty = sFALSE;
    updateRevision();
  }

  return sTRUE;
}

void frComposePlugin::paint(const composeViewport& viewport, sF32 w, sF32 h)
{
  for (sU32 i=0; i<def->nInputs; i++)
  {
    frPlugin* inPlg = input[i].plg;

    if (inPlg && inPlg->def->type == 2)
      ((frComposePlugin *) inPlg)->paint(viewport, w, h);
  }

  doPaint(viewport, w, h);
}
