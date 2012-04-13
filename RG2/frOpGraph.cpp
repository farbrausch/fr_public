// This code is in the public domain. See LICENSE for details.

// frOpGraph.cpp: implementation of the frOpGraph class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "frOpGraph.h"
#include "debug.h"
#include "plugbase.h"
#include "texbase.h"
#include "animbase.h"
#include "ViruzII.h"
#include "ViruzII/v2mconv.h"
#include "ViruzII/sounddef.h"
#include "tstream.h"
#include "WindowMapper.h"
#include "viewcomm.h"
#include "resource.h"
#include <set>
#include <list>

frOpGraph *g_graph=0;
fr::viruzII *g_viruz=0;

static sInt g_viruzRefCount=0;

// ---- import helpers

class importException
{
public:
  sChar*  message;

  importException(const sChar* msg)
  {
    message = strdup(msg);
  }

  importException(const importException& x)
  {
    message = strdup(x.message);
  }

  importException& operator = (const importException& x)
  {
    if (message)
      free(message);

    message = strdup(x.message);
  }

  ~importException()
  {
    if (message)
      free(message);
  }
};

static void throwImportException(const sChar* fmt, ...)
{
  va_list arg;
  sChar buf[2048];

  va_start(arg, fmt);
  vsprintf(buf, fmt, arg);
  va_end(arg);

  throw importException(buf);
}

// ---- frOpGraph::frOperator

frOpGraph::frOperator::frOperator()
: def(0), plg(0), opID(0), pageID(0), active(sFALSE), inputs(0), nInputs(0), nMaxInputs(0), outputs(0), nOutputs(0), nMaxOutputs(0)
{
}

frOpGraph::frOperator::~frOperator()
{
  FRSAFEDELETE(plg);

  if (inputs)
  {
    free(inputs);
    inputs = 0;
  }

  if (outputs)
  {
    free(outputs);
    outputs = 0;
  }
}

const fr::string& frOpGraph::frOperator::getName() const
{
  return name;
}

void frOpGraph::frOperator::setName(const fr::string& newName)
{
  const sInt oldLen = name.getLength();
  name = newName;

  if (!oldLen && name.getLength()) // operator just got a name
    g_graph->m_pages[pageID].addNamedOp(opID);

  if (oldLen && !name.getLength()) // operator hasn't got a name anymore
    g_graph->m_pages[pageID].delNamedOp(opID);
}

void frOpGraph::frOperator::addInput(sInt priority, sU32 id)
{
  // make sure we have enough space
  if (nInputs == nMaxInputs)
    inputs = (input*) realloc(inputs, (nMaxInputs += 4) * sizeof(input));

  // find the correct position to insert
  sInt insertPos = 0;

  while (insertPos < nInputs && inputs[insertPos].priority <= priority)
    insertPos++;

  // move everything after the insert pos one field to the right
  memmove(inputs + insertPos + 1, inputs + insertPos, (nInputs - insertPos) * sizeof(input));

  // insert and increment count
  inputs[insertPos].priority = priority;
  inputs[insertPos].id = id;

  nInputs++;
}

void frOpGraph::frOperator::delInput(sU32 id)
{
  sInt pos = -1;

  // try to find the operator
  for (sInt i = 0; i < nInputs; i++)
  {
    if (inputs[i].id == id)
    {
      pos = i;
      break;
    }
  }

  if (pos != -1) // we found it!
  {
    // move the entries in the array and decrement number of elements
    memmove(inputs + pos, inputs + pos + 1, (nInputs - pos - 1) * sizeof(input));
    nInputs--;
  }
}

void frOpGraph::frOperator::delInputs(sBool delLinks)
{
  if (delLinks)
    nInputs = 0;
  else
  {
    sInt wrPos = 0;

    for (sInt i = 0; i < nInputs; i++)
    {
      if (inputs[i].id & 0x80000000)
        inputs[wrPos++] = inputs[i];
    }

    nInputs = wrPos;
  }
}

void frOpGraph::frOperator::addOutput(sU32 outID)
{
  // make sure we have enough space
  if (nOutputs == nMaxOutputs)
    outputs = (sU32*) realloc(outputs, (nMaxOutputs += 4) * sizeof(sU32));

  // add the output
  outputs[nOutputs++] = outID;
}

void frOpGraph::frOperator::delOutput(sU32 outID)
{
  for (sInt i = 0; i < nOutputs; i++)
  {
    if (outputs[i] == outID) // we found the op?
    {
      outputs[i] = outputs[--nOutputs]; // remove it
      break;
    }
  }
}

void frOpGraph::frOperator::delOutputs(sBool delLinks)
{
  if (delLinks)
    nOutputs = 0;
  else
  {
    sInt wrPos = 0;

    for (sInt i = 0; i < nOutputs; i++)
    {
      if (outputs[i] & 0x80000000)
        outputs[wrPos++] = outputs[i];
    }

    nOutputs = wrPos;
  }
}

sBool frOpGraph::frOperator::setConnections()
{
  // set the input connections in the plugin
  sInt slot = 0;

  for (sInt i = 0; i < nInputs; i++)
  {
    const sU32 opID = inputs[i].id;

    if (opID & 0x80000000) // link connection
    {
      const sInt paramNum = inputs[i].priority;
      frParam* param = plg->getParam(paramNum);

      FRDASSERT(param->type == frtpLink);

      if (param->linkv->opID != (opID & 0x7fffffff))
      {
        param->linkv->opID = opID & 0x7fffffff;
        param->linkv->plg = 0;

        plg->setParam(paramNum, param);
      }
    }
    else if (slot < def->nInputs)
      plg->setInput(slot++, opID);
  }

  while (slot < def->nInputs)
    plg->setInput(slot++, 0);

  return plg->onConnectUpdate();
}

void frOpGraph::frOperator::serialize(fr::streamWrapper& strm, sU32 id)
{
  opID = id;

  sU16 ver = 1;
  strm << ver;
  
  if (ver == 1)
  {
    sU32 defID, w=0, h=0;
    
    // store/get id
    if (def)
      defID = def->ID;
    
    strm << defID;

    if (!def)
    {
      def = frGetPluginByID(defID);
      plg = def->create(def);
      FRASSERT(plg);
      
      if (def->type == 0) // texture
      {
        strm << w << h;
        static_cast<frTexturePlugin*>(plg)->resize(w, h);
      }
    }
    else
    {
      if (def->type == 0) // texture
      {
        frTexturePlugin* tpg = static_cast<frTexturePlugin*>(plg);
        
        if (tpg->getData())
        {
          w = tpg->getData()->w;
          h = tpg->getData()->h;
        }
        
        strm << w << h;
      }
    }
    
    // location, flags & name
    strm << rc.left << rc.top << rc.right << rc.bottom;
    strm << pageID << active;
    strm << name;
    
    // operator-specific data
    plg->serialize(strm);

    // dependencies
    strm << nInputs;
    nMaxInputs = nInputs;
    inputs = (input*) realloc(inputs, nInputs * sizeof(input));

    for (sInt i = 0; i < nInputs; i++)
    {
      strm << inputs[i].id;
      strm << inputs[i].priority;
    }

    strm << nOutputs;
    nMaxOutputs = nOutputs;
    outputs = (sU32*) realloc(outputs, nOutputs * sizeof(sU32));

    for (sInt i = 0; i < nOutputs; i++)
      strm << outputs[i];

    setConnections();
  }
}

void frOpGraph::frOperator::import(fr::streamWrapper& strm, sU32 id)
{
  static sInt previd = 0;

  opID = id;
  nOutputs = nMaxOutputs = 0;
  nInputs = nMaxInputs = 0;

  sU32 defID;
  strm << defID;

  def = frGetPluginByID(defID);
  if (!def)
    throwImportException("Op with ID %d is not supported in this version!", defID);

  plg = def->create(def);

  if (def->type == 0) // texture
  {
    sU32 w, h;
    strm << w << h;
    static_cast<frTexturePlugin*>(plg)->resize(w, h);
  }

  strm << rc.left << rc.top << rc.right << rc.bottom;
  strm << pageID << active;

  plg->serialize(strm); // yep, we use normal serialize functionality for this. (wow, das ist ja sooo MULTIFUNKTIONAL!)

  previd = defID;
}

// ---- frOpGraph::frOpPage

struct frOpGraph::frOpPage::privateData
{
  typedef std::vector<sU32> opList;
  typedef opList::iterator  opListIt;

  opList    m_operators;
};

frOpGraph::frOpPage::frOpPage()
  : pageID(0), type(0), scrlx(0), scrly(0)
{
  m_priv = new privateData;
}

frOpGraph::frOpPage::frOpPage(const frOpPage& x)
  : pageID(x.pageID), type(x.type), scrlx(x.scrlx), scrly(x.scrly)
{
  m_priv = new privateData;
  m_priv->m_operators = x.m_priv->m_operators;
}

frOpGraph::frOpPage::~frOpPage()
{
  FRSAFEDELETE(m_priv);
}

frOpGraph::frOpPage& frOpGraph::frOpPage::operator = (const frOpPage& x)
{
  pageID = x.pageID;
  type = x.type;
  scrlx = x.scrlx;
  scrly = x.scrly;
  m_priv->m_operators = x.m_priv->m_operators;

  return *this;
}

void frOpGraph::frOpPage::addNamedOp(sU32 id)
{
  m_priv->m_operators.push_back(id);
}

void frOpGraph::frOpPage::delNamedOp(sU32 id)
{
  for (privateData::opListIt it = m_priv->m_operators.begin(); it != m_priv->m_operators.end(); ++it)
  {
    if (*it == id)
    {
      m_priv->m_operators.erase(it);
      return;
    }
  }
}

void frOpGraph::frOpPage::serialize(fr::streamWrapper& strm, sU32 id)
{
  pageID = id;

  sU16 ver = 1;

  strm << ver;

  if (ver == 1)
  {
    strm << name;
    strm << type << scrlx << scrly;
  }
}

void frOpGraph::frOpPage::import(fr::streamWrapper& strm, sU32 id, sU16 ver)
{
  pageID = id;

  strm << name;

  if (ver > 1)
    strm << type;
  else
    type = 0; // texture

  if (ver >= 3)
    strm << scrlx << scrly;
  else
    scrlx = scrly = 0;
}

// ---- frOpGraph

struct frOpGraph::loadState
{
  struct loadItem
  {
    fr::string    nameRef;
    sU32          opID;
    sInt          paramNum;
    sInt          type;

    loadItem(const fr::string& name, sU32 oid, sInt pn, sInt t)
      : nameRef(name), opID(oid), paramNum(pn), type(t)
    {
    }
  };

  struct storeItem
  {
    fr::string    name;
    sU32          opID;
    sInt          type;
    sBool         isFinal;

    storeItem(const fr::string& _name, sU32 oid, sInt t, sBool final)
      : name(_name), opID(oid), type(t), isFinal(final)
    {
    }
  };

  typedef std::list<loadItem>             loadItemList;
  typedef loadItemList::const_iterator    loadItemListCIt;
  typedef loadItemList::iterator          loadItemListIt;

  typedef std::list<storeItem>            storeItemList;
  typedef storeItemList::const_iterator   storeItemListCIt;
  typedef storeItemList::iterator         storeItemListIt;

  sU32          m_curOp;
  loadItemList  m_loadItems;
  storeItemList m_storeItems;
};

frOpGraph::frOpGraph()
  : m_viewOp(0), m_editOp(0), m_curPage(0), m_opIDCounter(1), m_pageIDCounter(1), m_modified(sFALSE), m_tuneBuf(0),
  m_bpmRate(120), m_soundSysInit(sFALSE), m_loadState(0)
{
  m_curves = new frCurveContainer;
  m_clips = new frClipContainer;

  sdInit();

  if (g_viruzRefCount++ == 0)
    g_viruz = new fr::viruzII;
}

frOpGraph::~frOpGraph()
{
  FRSAFEDELETE(m_clips);
  FRSAFEDELETE(m_curves);

  g_viruz->closeTune();
  sdClose();

  FRSAFEDELETEA(m_tuneBuf);

  if (--g_viruzRefCount==0)
    FRSAFEDELETE(g_viruz);
}

void frOpGraph::clear()
{
	::SendMessage(g_winMap.Get(ID_TIMELINE_WINDOW), WM_SET_CURRENT_CLIP, 0, 0); // because the timeline window caches ptrs to clips

  m_ops.clear();
  m_pages.clear();
  
  m_opIDCounter = 1;
  m_pageIDCounter = 1;
  m_editOp = m_viewOp = m_view3DOp = 0;
  m_curPage = 0;

  m_bpmRate = 120;

  m_curves->clear();
  m_clips->clear();

  setTuneName("");
  sU32 mainClip = m_clips->addClip();
  m_clips->getClip(mainClip)->setName("Timeline");

	m_undo.purge();
    
  m_modified = sFALSE;
}

sBool frOpGraph::dependsOn(sU32 start, sU32 id)
{
  if (start == id)
    return sTRUE;

  opMapIt it = m_ops.find(start);
  if (it != m_ops.end())
  {
    for (sInt i = 0; i < it->second.nInputs; i++)
    {
      const sU32 opID = it->second.inputs[i].id & 0x7fffffff;

      if (opID == id || dependsOn(opID, id))
        return sTRUE;
    }
  }

  return sFALSE;
}

// ---- persistence

void frOpGraph::serialize(fr::streamWrapper& strm)
{
  sU32 id = '12GR';
  strm << id;
  if (id != '12GR')
    return;

  if (!strm.isWrite())
    clear();

  sU16 ver = 1;
  strm << ver;

  if (ver == 1)
  {
    // ops
    sU32 opCount = m_ops.size();
    strm << opCount;

    opMapIt oit = m_ops.begin();
    while (opCount--)
    {
      // op id
      sU32 id;

      if (strm.isWrite())
      {
        id  = oit->first;
        ++oit;
      }

      strm << id;
      if (id >= m_opIDCounter)
        m_opIDCounter = id + 1;

      // actual data
      m_ops[id].serialize(strm, id);
    }

    // pages
    sU32 pgCount = m_pages.size();
    strm << pgCount;

    pgMapIt pit = m_pages.begin();
    while (pgCount--)
    {
      // page id
      sU32 id;

      if (strm.isWrite())
      {
        id = pit->first;
        ++pit;
      }

      strm << id;
      if (id >= m_pageIDCounter)
        m_pageIDCounter = id + 1;

      // actual data
      m_pages[id].serialize(strm, id);
    }

    // misc stuff
    strm << m_viewOp << m_view3DOp << m_editOp << m_curPage;

    // curves & clips
    strm << *m_curves;
    strm << *m_clips;
    m_curves->applyAllBasePoses();

    // viruzII stuff
    strm << m_tuneName;
    setTuneName(m_tuneName);
    strm << m_bpmRate;
    strm << m_loaderTuneName;
  }

  // update list of named ops for the pages if we're loading
  if (!strm.isWrite())
  {
    for (opMapIt it = m_ops.begin(); it != m_ops.end(); ++it)
    {
      if (it->second.getName().getLength())
        m_pages[it->second.pageID].addNamedOp(it->first);
    }
  }

  m_modified = sFALSE;
}

// ---- .tg2 import

void frOpGraph::loadRegisterLink(const fr::string& name, sInt paramNum, sInt linkType)
{
  m_loadState->m_loadItems.push_back(loadState::loadItem(name, m_loadState->m_curOp, paramNum, linkType));
}

void frOpGraph::loadRegisterStore(const fr::string& name, sInt linkType, sBool isFinal)
{
  m_loadState->m_storeItems.push_back(loadState::storeItem(name, m_loadState->m_curOp, linkType, isFinal));
}

void frOpGraph::import(fr::streamWrapper& strm)
{
  // Imports .tg2 files

  m_loadState = new loadState;

  try
  {
    FRASSERT(!strm.isWrite());

    clear();

    sU32 id;
    strm << id;
    if (id != 'GTRF')
      throwImportException("Invalid file");

    sU16 ver;
    strm << ver;
    if (ver >= 9)
      throwImportException("Invalid version number of file to import");

    // import ops
    sU32 count;
    strm << count;
    while (count--)
    {
      sU32 id;
      strm << id;

      m_loadState->m_curOp = id;

      m_ops[id].import(strm, id);
      if (id >= m_opIDCounter)
        m_opIDCounter = id + 1;
    }

    // import pages
    strm << count;
    while (count--)
    {
      sU32 id;
      strm << id;

      m_pages[id].import(strm, id, ver);
      if (id >= m_pageIDCounter)
        m_pageIDCounter = id + 1;
    }

    // import misc data
    strm << m_viewOp << m_editOp << m_curPage;

    if (ver >= 2)
      strm << m_view3DOp;
    else
      m_view3DOp = 0;

    if (ver >= 4)
    {
      strm << *m_curves;
      m_curves->fixOldAnimIndices();
    }

    if (ver >= 5)
    {
      strm << *m_clips;
      m_curves->applyAllBasePoses();
    }

    if (ver >= 6)
    {
      strm << m_tuneName;
      setTuneName(m_tuneName);
    }
    else
      setTuneName("");

    if (ver >= 7)
      strm << m_bpmRate;
    else
      m_bpmRate = 120;

    if (ver >= 8)
      strm << m_loaderTuneName;
    else
      m_loaderTuneName.empty();

    // the actual data is loaded now - let's start with the interesting part of the show:
    // rebuilding the dependencies, handling the loads, that kind of stuff.

    // step 1: handle the names of the store operators (was a parameter of the store operator in former versions)
    for (loadState::storeItemListCIt sit = m_loadState->m_storeItems.begin(); sit != m_loadState->m_storeItems.end(); ++sit)
    {
      // get the operator
      frOperator& op = m_ops[sit->opID];

      // and set the name
      op.setName(sit->name);
    }

    // step 2: connect all operators
    for (opMapIt oit = m_ops.begin(); oit != m_ops.end(); ++oit)
    {
      frOperator& op = oit->second;
      CRect rc = op.rc;

      // go to the list of operators to find a possible connection
      for (opMapIt it = m_ops.begin(); it != m_ops.end(); ++it)
      {
        frOperator& op2 = it->second;

        if (op2.pageID != op.pageID || op2.rc.left >= rc.right || op2.rc.right <= rc.left)
          continue; // not on the same page or no intersection on the x axis, skip this op

        if (op2.rc.top == rc.top - 25) // above the operator (input row)
        {
          op.addInput(op2.rc.left, it->first);
          op2.addOutput(op.opID);
        }
      }
    }

    // step 3: process the loads
    for (loadState::loadItemListCIt lit = m_loadState->m_loadItems.begin(); lit != m_loadState->m_loadItems.end(); ++lit)
    {
      // get the operator
      frOperator& op = m_ops[lit->opID];

      // find the corresponding store
      loadState::storeItemListCIt sit;
      sU32 matchID = 0;

      for (sit = m_loadState->m_storeItems.begin(); sit != m_loadState->m_storeItems.end(); ++sit)
      {
        if (sit->type == lit->type && !stricmp(sit->name, lit->nameRef) && (sit->type || sit->isFinal || sit->type == op.def->type))
        {
          // we got a match!
          matchID = sit->opID;
          break; 
        }
      }

      if (matchID && dependsOn(matchID, lit->opID)) // if we're trying to build a cycle, break it NOW!
        matchID = 0;

      if (matchID)
      {
        // add the corresponding inputs/outputs
        op.addInput(lit->paramNum, matchID | 0x80000000);
        m_ops[matchID].addOutput(lit->opID | 0x80000000);
      }
    }

    // step 4: set the connections for all operators
    for (opMapIt oit = m_ops.begin(); oit != m_ops.end(); ++oit)
      oit->second.setConnections();
  }
  catch (importException& e)
  {
    MessageBox(GetForegroundWindow(), e.message, "RG2.1 .TG2 Import", MB_ICONERROR|MB_OK);
  }

  // get rid of temporary data and finish the process
  delete m_loadState;
  m_loadState = 0;

  m_modified = sFALSE;
}

// ---- tune

extern "C" sU8 loaderTune[];
extern "C" sU8 loaderTuneEnd[];

void frOpGraph::setTuneName(const sChar *tuneName)
{
  m_tuneName = tuneName;
  g_viruz->closeTune();

  sU8* tuneBuf;
  sU32 tuneSize;

  FRSAFEDELETEA(m_tuneBuf);

  fr::streamF tune;
  if (!m_tuneName.isEmpty() && tune.open(m_tuneName, fr::streamF::rd|fr::streamF::ex))
  {
    tuneSize = tune.size();
    tuneBuf = new sU8[tuneSize];
    tune.read(tuneBuf, tuneSize);
    tune.close();
  }
  else
  {
    tuneSize = loaderTuneEnd - loaderTune;
    tuneBuf = new sU8[tuneSize];
    memcpy(tuneBuf, loaderTune, tuneSize);
  }

  ConvertV2M(tuneBuf, tuneSize, &m_tuneBuf, &m_tuneSize);
  delete[] tuneBuf;
  
  if (m_tuneBuf == 0)
    ConvertV2M(loaderTune, loaderTuneEnd-loaderTune, &m_tuneBuf, &m_tuneSize);

  g_viruz->initTune(m_tuneBuf);
}

// ---- adding items

frOpGraph::frOperator& frOpGraph::addOperator()
{
  sU32 id = m_opIDCounter++;

  frOperator& opr = (m_ops[id] = frOperator());
  opr.opID = id;
  opr.pageID = m_curPage;

  return opr;
}

frOpGraph::frOperator& frOpGraph::addOperatorWithID(sU32 id)
{
	FRASSERT(id != 0);
	FRASSERT(m_ops.find(id) == m_ops.end());

	frOperator& opr = (m_ops[id] =  frOperator());
	opr.opID = id;
	opr.pageID = m_curPage;

	return opr;
}

frOpGraph::frOpPage &frOpGraph::addPage(sInt type)
{
  sU32 id = m_pageIDCounter++;

  frOpPage &pgr = (m_pages[id] = frOpPage());
  pgr.pageID = id;
  pgr.type = type;

  return pgr;
}

void frOpGraph::modified()
{
  m_modified = sTRUE;
}

void frOpGraph::resetModified()
{
  m_modified = sFALSE;
}

// ---- frOpterator

frOpIterator::frOpIterator(frOpGraph::frOpPage& x)
  : m_page(x), m_ndx(0)
{
}

sU32 frOpIterator::getID() const
{
  if (m_ndx < m_page.m_priv->m_operators.size())
    return m_page.m_priv->m_operators[m_ndx];
  else
    return 0;
}

frOpGraph::frOperator& frOpIterator::getOperator() const
{
  sU32 id = getID();
  FRASSERT(id != 0);

  return g_graph->m_ops[id];
}

frPlugin* frOpIterator::getPlugin() const
{
  frOpGraph::opMapIt it = g_graph->m_ops.find(getID());
  return (it != g_graph->m_ops.end()) ? it->second.plg : 0;
}

frOpIterator::operator bool() const
{
  return m_ndx < m_page.m_priv->m_operators.size();
}

frOpIterator& frOpIterator::operator ++()
{
  m_ndx++;
  return *this;
}
