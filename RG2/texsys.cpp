// This code is in the public domain. See LICENSE for details.

#include "stdafx.h"
#include "texbase.h"
#include "stream.h"
#include "tstream.h"
#include "TG2Config.h"
#include "debug.h"
#include "frOpGraph.h"

// ---- system: store

class frTSStore: public frTexturePlugin
{
  sBool doProcess()
  {
    data = getInputData(0);

    return sTRUE;
  }

public:
  frTSStore(const frPluginDef* d)
    : frTexturePlugin(d)
  {
  }
  
  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver=2;

    strm << ver;

    // old version (import)
    if (ver == 1)
    {
      fr::string tempstr;
      sInt type;

      strm << tempstr;
      strm << type;

      g_graph->loadRegisterStore(tempstr, 0, type);
    }
  }

  sInt getButtonType()
  {
    return 3; // store
  }
};

TG2_DECLARE_NOP_PLUGIN(frTSStore, "Store", 0, 1, 1, sFALSE, sTRUE, 13);

// ---- system: load

class frTSLoad: public frTexturePlugin
{
  sBool doProcess()
  {
    data = getInputData(0);

    return sTRUE;
  }

public:
  frTSLoad(const frPluginDef *d)
    : frTexturePlugin(d, 1)
  {
    addLinkParam("Load what", 0, 0); // texture ref
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver=2;

    strm << ver;

    // old version (import)
    if (ver == 1)
    {
      fr::string tempstr;
      strm << tempstr;

      g_graph->loadRegisterLink(tempstr, 0, 0);
    }

    // new version
    if (ver == 2)
      strm << params[0].linkv;
  }
 
  const sChar *getDisplayName() const
  {
    frOpGraph::opMapIt it = g_graph->m_ops.find(params[0].linkv->opID);

    if (it != g_graph->m_ops.end())
      return it->second.getName();
    else
      return "(Invalid)";
  }
  
  sInt getButtonType()
  {
    return 4; // load
  }
};

TG2_DECLARE_NOP_PLUGIN(frTSLoad, "Load", 0, 0, 0, sTRUE, sTRUE, 14);

// ---- system: bridge

class frTSBridge: public frTexturePlugin
{
  sBool doProcess()
  {
    data = getInputData(0);

    return sTRUE;
  }
  
public:
  frTSBridge(const frPluginDef *d)
    : frTexturePlugin(d)
  {
  }

  void serialize(fr::streamWrapper &strm)
  {
    sU16 ver=2;
    
    strm << ver;

    // old version (import)
    if (ver == 1)
    {
      fr::string tempstr;
      strm << tempstr; // description ist gekickt.
    }
  }
};

TG2_DECLARE_NOP_PLUGIN(frTSBridge, "Bridge", 0, 1, 1, sTRUE, sTRUE, 4);
