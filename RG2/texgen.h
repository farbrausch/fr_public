// This code is in the public domain. See LICENSE for details.

#ifndef __texgen_h_
#define __texgen_h_

#include <vector>
#include "types.h"
#include "stream.h"
#include "tstream.h"
#include "sync.h"
#include "memmgr.h"

#define FR_DECLARE_TEX_PLUGIN(sname, clname, name, inputs, id) \
  static frPlugin *create##sname##clname##Helper(frPluginDef *d) \
  { \
    return new clname(d); \
  } \
  frPluginDef sname={name, inputs, inputs, sTRUE, sFALSE, sTRUE, id, 0, create##sname##clname##Helper}; 

#define FR_DECLARE_TEX_PLUGIN_EX(sname, clname, name, inputs, minin, id) \
  static frPlugin *create##sname##clname##Helper(frPluginDef *d) \
  { \
    return new clname(d); \
  } \
  frPluginDef sname={name, inputs, minin, sTRUE, sFALSE, sTRUE, id, 0, create##sname##clname##Helper}; 

#define FR_DECLARE_SYSTEM_TEX_PLUGIN(sname, clname, name, inputs, hasOutput, id) \
  static frPlugin *create##sname##clname##Helper(frPluginDef *d) \
  { \
    return new clname(d); \
  } \
  frPluginDef sname={name, inputs, inputs, hasOutput, sTRUE, sTRUE, id, 0, create##sname##clname##Helper}; 

#define FR_DECLARE_GEO_PLUGIN(sname, clname, name, inputs, id) \
  static frPlugin *create##sname##clname##Helper(frPluginDef *d) \
  { \
    return new clname(d); \
  } \
  frPluginDef sname={name, inputs, inputs, sTRUE, sFALSE, sTRUE, id, 1, create##sname##clname##Helper}; 

#define FR_DECLARE_GEO_PLUGIN_EX(sname, clname, name, inputs, minin, id) \
  static frPlugin *create##sname##clname##Helper(frPluginDef *d) \
  { \
    return new clname(d); \
  } \
  frPluginDef sname={name, inputs, minin, sTRUE, sFALSE, sTRUE, id, 1, create##sname##clname##Helper}; 

#define FR_DECLARE_SYSTEM_GEO_PLUGIN(sname, clname, name, inputs, hasOutput, id) \
  static frPlugin *create##sname##clname##Helper(frPluginDef *d) \
  { \
    return new clname(d); \
  } \
  frPluginDef sname={name, inputs, inputs, hasOutput, sTRUE, sTRUE, id, 1, create##sname##clname##Helper}; 

#define FR_DECLARE_SYSTEM_GEO_PLUGIN_EX(sname, clname, name, inputs, minin, hasOutput, id) \
  static frPlugin *create##sname##clname##Helper(frPluginDef *d) \
  { \
    return new clname(d); \
  } \
  frPluginDef sname={name, inputs, minin, hasOutput, sTRUE, sTRUE, id, 1, create##sname##clname##Helper}; 

#endif
