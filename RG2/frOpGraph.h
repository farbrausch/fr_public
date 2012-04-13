// This code is in the public domain. See LICENSE for details.

// frOpGraph.h: interface for the frOpGraph class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FROPGRAPH_H__AE57F3F0_8546_44E5_9D03_0641261714E4__INCLUDED_)
#define AFX_FROPGRAPH_H__AE57F3F0_8546_44E5_9D03_0641261714E4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include "types.h"
#include "tool.h"
#include "undoBuffer.h"
#include <map>

class frOpGraph;
class frCurveContainer;
class frClipContainer;
class frPlugin;
struct frPluginDef;

namespace fr
{
  class viruzII;
  class streamWrapper;
}

extern frOpGraph* g_graph;
extern fr::viruzII* g_viruz;

class frOpGraph  
{
public:
  class frOperator
  {
    fr::string          name;

  public:
    struct input
    {
      sInt  priority;
      sU32  id; // high bit is used to denote "link" connections
    };

		const frPluginDef*	def;
		frPlugin*						plg;
    RECT                rc;
    sU32                opID, pageID;
    sBool               active;

		input*							inputs;
    sInt                nInputs, nMaxInputs;

		sU32*								outputs; // high bit is used to denote "link" connections
    sInt                nOutputs, nMaxOutputs;

    frOperator();
    ~frOperator();

    const fr::string& getName() const;
    void setName(const fr::string& newName);

    void addInput(sInt priority, sU32 id);
    void delInput(sU32 id);
    void delInputs(sBool delLinks = sFALSE);
    
    void addOutput(sU32 outID);
    void delOutput(sU32 outID);
    void delOutputs(sBool delLinks = sFALSE);

    sBool setConnections();

    void serialize(fr::streamWrapper& strm, sU32 id);
    void import(fr::streamWrapper& strm, sU32 id);
  };

  class frOpPage
  {
    struct privateData;
    privateData*        m_priv;

    friend class frOpIterator;

  public:
    fr::string  name;
    sU32        pageID;
    sInt        type; // 0=texture, 1=geo, 2=compose
    sInt        scrlx, scrly;

    frOpPage();
    frOpPage(const frOpPage& x);
    ~frOpPage();

    frOpPage& operator = (const frOpPage& x);

    void addNamedOp(sU32 id);
    void delNamedOp(sU32 id);

    void serialize(fr::streamWrapper& strm, sU32 id);
    void import(fr::streamWrapper& strm, sU32 id, sU16 ver);
  };

  typedef std::map<sU32, frOperator>  opMap;
  typedef opMap::iterator             opMapIt;
  typedef std::map<sU32, frOpPage>    pgMap;
  typedef pgMap::iterator             pgMapIt;

  struct loadState;

  opMap       m_ops;
  pgMap       m_pages;
  sU32        m_viewOp, m_editOp, m_view3DOp;
  sU32        m_curPage;

  sBool       m_modified;
  sInt        m_bpmRate;

  sU32        m_opIDCounter;
  sU32        m_pageIDCounter;

  fr::string  m_tuneName;
  sU8*        m_tuneBuf;
  sInt        m_tuneSize;
  sBool       m_soundSysInit;

  fr::string  m_loaderTuneName;
  
  loadState*  m_loadState;

  frCurveContainer*	m_curves;
  frClipContainer*	m_clips;

	frUndoBuffer	m_undo;

	frOpGraph();
	virtual ~frOpGraph();

  void        clear();

  sBool       dependsOn(sU32 start, sU32 id);

  void        serialize(fr::streamWrapper &strm);

  void        loadRegisterLink(const fr::string& name, sInt paramNum, sInt linkType);
  void        loadRegisterStore(const fr::string& name, sInt linkType, sBool isFinal = sTRUE);
  void        import(fr::streamWrapper& strm);

  void        setTuneName(const sChar* tuneName);
  const sChar *getTuneName()                      { return m_tuneName; }

  frOperator& addOperator();
	frOperator&	addOperatorWithID(sU32 id);
  frOpPage&   addPage(sInt type);

  void        modified();
  void        resetModified();
};

class frOpIterator // only for named ops!
{
  sInt m_ndx;
  frOpGraph::frOpPage& m_page;

public:
  frOpIterator(frOpGraph::frOpPage& x);

  sU32 getID() const;
  frOpGraph::frOperator& getOperator() const;
  frPlugin* getPlugin() const;

  operator bool() const;
  frOpIterator& operator ++();
};

#endif // !defined(AFX_FROPGRAPH_H__AE57F3F0_8546_44E5_9D03_0641261714E4__INCLUDED_)
