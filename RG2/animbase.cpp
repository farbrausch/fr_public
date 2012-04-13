// This code is in the public domain. See LICENSE for details.

#include "stdafx.h"
#include "animbase.h"
#include "plugbase.h"
#include "frOpGraph.h"
#include "debug.h"
#include "viewcomm.h"
#include "WindowMapper.h"
#include "resource.h"
#include "viruzII.h"
#include "curve.h"
#include "tstream.h"
#include <algorithm>

// ---- frCurveContainer::opCurves

frCurveContainer::opCurves::curve::curve(sInt parm, sInt nChannels)
  : param(parm), refs(1)
{
  grp = new fr::fCurveGroup(nChannels);
}

frCurveContainer::opCurves::opCurves()
  : m_curveNdx(0x10000), m_opID(0)
{
}

frCurveContainer::opCurves::opCurves(sU32 opID)
  : m_curveNdx(0x10000), m_opID(opID)
{
}

sU32 frCurveContainer::opCurves::addCurve(sInt paramNdx)
{
  frOpGraph::opMapIt it = g_graph->m_ops.find(m_opID);
  
  if (it != g_graph->m_ops.end())
  {
    frParam* param = it->second.plg->getParam(paramNdx);
    
    if (param)
    {
      sInt nChannels = -1;
      
      switch (param->type)
      {
      case frtpColor:       nChannels = 3;	break;
      case frtpInt:         nChannels = 1;  break;
      case frtpFloat:       nChannels = 1;  break;
      case frtpPoint:       nChannels = 2;  break;
      case frtpSelect:      nChannels = 1;  break;
      case frtpButton:      nChannels = 0;  break;
      case frtpString:      nChannels = 0;  break;
      case frtpSize:        nChannels = 2;  break;
      case frtpTwoFloat:    nChannels = 2;  break;
      case frtpThreeFloat:  nChannels = 3;  break;
      case frtpFloatColor:  nChannels = 3;  break;
      default:              return 0;
      }
      
      m_curveNdx++;
      FRASSERT(m_curves.find(m_curveNdx) == m_curves.end());

      sInt animIndex = param->animIndex;
      
      if (m_curves.find(animIndex) == m_curves.end()) // no basepose for that parameter yet? create it!
      {
        curve& bpose = m_curves[animIndex] = curve(animIndex, nChannels);
        updateBasePose(paramNdx);
      }

      curve& curv = m_curves[m_curveNdx] = curve(animIndex, nChannels);
      curve& bpose = m_curves[animIndex];
      
      for (sInt i = 0; i < nChannels; i++)
      {
        fr::fCurve* fc = curv.grp->getCurve(i);
        fc->setType(fr::fCurve::linear);
        
        const sF32 value = bpose.grp->getCurve(i)->evaluate(0);
				sInt keyTime = g_viruz->getPositions()->getBarLen(0);
				if (keyTime < 100 || keyTime > 10000)
					keyTime = 1000;
        
				fc->addKey(0).value = value;
				fc->addKey(keyTime).value = value;
      }
      
      return m_curveNdx;
    }
    else
      return 0;
  }
  else
    return 0;
}

fr::fCurveGroup* frCurveContainer::opCurves::getCurve(sU32 curveID)
{
  curveMapIt it = m_curves.find(curveID);
  return (it != m_curves.end()) ? ((fr::fCurveGroup*) it->second.grp) : 0;
}

sInt frCurveContainer::opCurves::getAnimIndex(sU32 curveID)
{
  curveMapIt it = m_curves.find(curveID);
  return (it != m_curves.end()) ? it->second.param : -1;
}

void frCurveContainer::opCurves::deleteCurve(sU32 curveID)
{
  curveMapIt it = m_curves.find(curveID);
  if (it != m_curves.end())
    m_curves.erase(it);
}

extern sInt g_lastCompositingOp;

void frCurveContainer::opCurves::evaluate(sU32 curveID, sF32 time)
{
  frOpGraph::opMapIt it = g_graph->m_ops.find(m_opID);
  
  if (it != g_graph->m_ops.end() && it->second.plg)
  {
    frPlugin* plg = it->second.plg;
    
    curveMapCIt cit = m_curves.find(curveID);
    if (cit != m_curves.end())
    {
      sF32 output[3]; // 3=maximum output count
      cit->second.grp->evaluate(time, output);
      plg->setAnim(cit->second.param, output);

      if (it->second.def->type == 2) // compositing?
        g_lastCompositingOp = it->first;
    }
  }
}

void frCurveContainer::opCurves::ref(sU32 curveID)
{
  curveMapIt it = m_curves.find(curveID);

  if (it != m_curves.end())
    it->second.refs++;
}

void frCurveContainer::opCurves::unRef(sU32 curveID)
{
  curveMapIt it = m_curves.find(curveID);

  if (it != m_curves.end())
  {
    if (--it->second.refs == 0)
      deleteCurve(curveID);
  }
}

void frCurveContainer::opCurves::applyBasePoses() const
{
  frOpGraph::opMapIt it = g_graph->m_ops.find(m_opID);
  
  if (it != g_graph->m_ops.end() && it->second.plg)
  {
    frPlugin* plg = it->second.plg;
    
    for (sInt i = 0; i < plg->getNParams(); i++)
    {
      const sInt animIndex = plg->getParam(i)->animIndex;
      if (!animIndex) // skip non-animated parameters
        continue;

      curveMapCIt cit = m_curves.find(animIndex); // try to find base pose for this parameter
      
      if (cit != m_curves.end()) // base pose there? if yes, apply.
      {
        sF32 output[3];
        cit->second.grp->evaluate(0, output);
        plg->setAnim(cit->second.param, output);
      }
    }
  }
}

void frCurveContainer::opCurves::updateBasePose(sInt param)
{
  frOpGraph::opMapIt it = g_graph->m_ops.find(m_opID);
  
  if (it != g_graph->m_ops.end() && it->second.plg)
  {
    frPlugin* plg = it->second.plg;
    
    for (sInt i = 0; i < plg->getNParams(); i++)
    {
      const frParam* param = plg->getParam(i);
      const sInt animIndex = param->animIndex;
      if (!animIndex)
        continue;

      curveMapIt cit = m_curves.find(animIndex); // try to find base pose for this parameter
      
      if (cit != m_curves.end()) // base pose there? if yes, update.
      {
        for (sInt i = 0; i < cit->second.grp->getNChannels(); i++)
        {
          fr::fCurve *fc = cit->second.grp->getCurve(i);
          fc->setType(fr::fCurve::constant);
					fc->addKey(0).value = ((sF32*) &param->trfloatv.a)[i];
        }
      }
    }
  }
}

// ---- frCurveContainer

frCurveContainer::frCurveContainer()
{
}

void frCurveContainer::clear()
{
  m_opCurves.clear();
}

frCurveContainer::opCurves &frCurveContainer::getOperatorCurves(sU32 opID)
{
  opCurveMapIt it = m_opCurves.find(opID);

  if (it == m_opCurves.end())
    return (m_opCurves[opID] = opCurves(opID));
  else
    return it->second;
}

void frCurveContainer::deleteOperatorCurves(sU32 opID)
{
  opCurveMapIt it = m_opCurves.find(opID);

  if (it != m_opCurves.end())
    m_opCurves.erase(it);
}

void frCurveContainer::applyBasePoses(sU32 opID) const
{
  opCurveMapCIt it = m_opCurves.find(opID);

  if (it != m_opCurves.end())
    it->second.applyBasePoses();
}

void frCurveContainer::applyAllBasePoses() const
{
  for (opCurveMapCIt it = m_opCurves.begin(); it != m_opCurves.end(); ++it)
    it->second.applyBasePoses();
}

static sInt translateParamToAnim(sU32 opID, sInt paramNum)
{
  // hardcoding this is pretty sucky, but it belongs to old versions and packing it into the operators
  // is no good idea either.

  const sInt opType = g_graph->m_ops[opID].def->ID;

  switch (opType)
  {
  case 128: // get primitive
    if (paramNum >= 1 && paramNum <= 3)
      return paramNum;
    break;

  case 130: // transform
    if (paramNum < 4)
      return paramNum + 1;
    break;

  case 131: // clone+transform
    if (paramNum < 3)
      return paramNum + 1;
    break;

  case 135: // bend
    if (paramNum < 6)
    {
      sInt blendType = g_graph->m_ops[opID].plg->getParam(7)->selectv.sel;

      if (blendType == 1) // blend transform
        paramNum += (paramNum < 3) ? 3 : -3;

      return paramNum + 1;
    }
    break;

  case 136: // uv mapping
    if (paramNum >= 1 && paramNum <= 3)
      return paramNum;
    break;

  case 142: // lighting
    if (paramNum == 1) // color
      return 1;

    {
      sInt lType = g_graph->m_ops[opID].plg->getParam(0)->selectv.sel;

      if (lType == 0 && paramNum == 3)
        return 2; // direction
      else if (lType == 1 && paramNum >= 3 && paramNum <= 4)
        return paramNum; // pos/attn
      else if (lType == 2)
      {
        switch (paramNum)
        {
        case 3:   return 3;
        case 4:   return 2;
        case 5:   return 4;
        case 6:   return 5;
        case 7:   return 6;
        }
      }
    }
    break;

  case 151: // shape animation
    if (paramNum == 0)
      return 1; // time
    break;

  case 192: // camera
    if (paramNum == 2 || paramNum == 3)
      return paramNum - 1; // position / value 2
    else if (paramNum >= 5 && paramNum <= 7)
      return paramNum - 2; // fadelevel / viewport
    break;
  }

  return -1;
}

void frCurveContainer::fixOldAnimIndices()
{
  // fixes the anim indices, pre-rg21 versions actually were pretty much fucked up concerning this and used
  // the wrong values internally. it still worked because they were unique, but still, it's not the way it
  // should've been, so it's corrected now.

  for (opCurveMapIt cit = m_opCurves.begin(); cit != m_opCurves.end(); ++cit)
  {
    // first, fix the curves internally (the easy part)
    for (opCurves::curveMapIt it = cit->second.m_curves.begin(); it != cit->second.m_curves.end(); ++it)
    {
      const sInt newParm = translateParamToAnim(cit->second.m_opID, it->second.param);
      FRASSERT(newParm != -1);
      it->second.param = newParm;
    }

    // now, we have to fix the curve IDs (which requires some modification of keys and thus is more complicated)
    sBool isOk = sFALSE;

    while (!isOk)
    {
      isOk = sTRUE;

      for (opCurves::curveMapIt it = cit->second.m_curves.begin(); it != cit->second.m_curves.end(); ++it)
      {
        if (it->first < 0x10000 && it->first != it->second.param) // basepose that doesn't match param index?
        {
          sU32 id = it->first;

          // replicate the key to the correct value
          cit->second.m_curves[it->second.param] = cit->second.m_curves[id];

          // and delete the old version
          it = cit->second.m_curves.find(id);
          FRASSERT(it != cit->second.m_curves.end());
          cit->second.m_curves.erase(it);

          isOk = sFALSE;
          break;
        }
      }
    }
  }
}

void frCurveContainer::updateBasePose(sU32 opID, sInt param)
{
  opCurveMapIt cit = m_opCurves.find(opID);
  
  if (cit != m_opCurves.end())
    cit->second.updateBasePose(param);
}

fr::streamWrapper& operator << (fr::streamWrapper& strm, frCurveContainer& cont)
{
  sU16 ver = 2;
  strm << ver;

  if (ver >= 1 && ver <= 2)
  {
    if (strm.isWrite())
    {
      sU32 size = cont.m_opCurves.size();
      strm << size;

      for (frCurveContainer::opCurveMapIt it = cont.m_opCurves.begin(); it != cont.m_opCurves.end(); ++it)
      {
        sU32 id = it->first;

        strm << id;
        frCurveContainer::opCurves& crv = it->second;

        strm << crv.m_curveNdx;

        sU32 size = crv.m_curves.size();
        strm << size;

        for (frCurveContainer::opCurves::curveMapIt cit = crv.m_curves.begin(); cit != crv.m_curves.end(); ++cit)
        {
          sU32 id = cit->first;

          strm << id;
          strm << cit->second.param;
          strm << *cit->second.grp;
        }
      }
    }
    else
    {
      cont.m_opCurves.clear();

      sU32 sz = 0;
      strm << sz;

      while (sz--)
      {
        sU32 id;
        strm << id;

        frCurveContainer::opCurves& crv = cont.m_opCurves[id] = frCurveContainer::opCurves(id);
        strm << crv.m_curveNdx;

        sU32 size = 0;
        strm << size;

        while (size--)
        {
          sU32 id;
          strm << id;
          strm << crv.m_curves[id].param;
          
          if (!strm.isWrite())
            crv.m_curves[id].grp = new fr::fCurveGroup;

          strm << *crv.m_curves[id].grp;

          if (ver == 1)
          {
            fr::fCurveGroup* grp = crv.m_curves[id].grp;

            for (sInt i = 0; i < grp->getNChannels(); i++)
            {
              fr::fCurve* crv = grp->getCurve(i);

              for (sInt j = 0; j < crv->getNKeys(); j++)
              {
                crv->getKey(j)->frame *= 40;
                crv->keyModified(j);
              }
            }
          }
        }
      }
    }
  }

  return strm;
}

// ---- frAnimationClip

frAnimationClip::frAnimationClip(sU32 id)
  : m_ID(id), m_start(0.0f), m_length(0)
{
}

frAnimationClip::frAnimationClip(const frAnimationClip& x)
{
  FRDASSERT(x.m_ID == 0); // may not copy construct applied clips
  FRDASSERT(x.m_name.isEmpty() == sTRUE); // may not copy construct named clips

  m_elements = x.m_elements;
  m_ID = 0;
  m_start = x.m_start;
  m_length = x.m_length;
}

frAnimationClip::~frAnimationClip()
{
  if (m_ID && g_graph->m_clips->m_names.size())
  {
    frClipContainer::nameMapIt it = g_graph->m_clips->m_names.find(m_name);
    FRDASSERT(it != g_graph->m_clips->m_names.end());

    g_graph->m_clips->m_names.erase(it);
  }
}

sBool frAnimationClip::setName(const fr::string& name)
{
  FRDASSERT(m_ID != 0);

  if (name.isEmpty())
    return sFALSE;

  frClipContainer* cont = g_graph->m_clips;
  frClipContainer::nameMapCIt nit = cont->m_names.find(name);

  if (nit == cont->m_names.end())
  {
    frClipContainer::nameMapIt onit = cont->m_names.find(m_name);

    if (onit != cont->m_names.end())
    {
      FRDASSERT(onit->second == m_ID);
      cont->m_names.erase(onit);
    }

    m_name = name;
    cont->m_names[name] = m_ID;
  }
  else
    return sFALSE;

  return sTRUE;
}

void frAnimationClip::setID(const sU32 ID)
{
  FRDASSERT(m_ID == 0); // may only set ID for non-identified clips
  m_ID = ID;
}

void frAnimationClip::addElement(sInt start, sU32 id, sU32 curveID)
{
  element elem;

  elem.relStart = start;
  elem.type = 0; // fcurve
  elem.id = id;
  elem.curveID = curveID;
  elem.trackID = 0;

  m_elements.push_back(elem);
}

void frAnimationClip::addElement(sInt start, sU32 trackID, const fr::string& clipName)
{
  const frAnimationClip* clp = g_graph->m_clips->findClipByName(clipName);

  if (clp)
  {
    element elem;

    elem.relStart = start;
    elem.type = 1; // subclip
    elem.id = clp->getID();
    elem.trackID = trackID;

    m_elements.push_back(elem);
  }
}

void frAnimationClip::deleteElement(sInt ind)
{
  m_elements.erase(m_elements.begin() + ind);
}

void frAnimationClip::deleteElementsReferringClip(sU32 clipID)
{
	elementListIt it = m_elements.begin();

	while (it != m_elements.end())
  {
    if (it->type == 1 && it->id == clipID)
      it = m_elements.erase(it);
    else
      ++it;
  }

  updateLength();
}

void frAnimationClip::cleanup()
{
  updateLength();

	elementListIt it = m_elements.begin();

	while (it != m_elements.end())
	{
		if (it->len==0)
			it=m_elements.erase(it);
		else
			++it;
	}
}

void frAnimationClip::evaluate(sF32 time) const
{
  for (elementListCIt it = m_elements.begin(); it != m_elements.end(); ++it)
  {
    sInt relTime = time - it->relStart;

    if (relTime >= 0 && relTime < it->len)
    {
      switch (it->type)
      {
      case 0: // fcurve
        {
          frCurveContainer::opCurves& crv = g_graph->m_curves->getOperatorCurves(it->id);
          crv.evaluate(it->curveID, relTime);
        }
        break;

      case 1: // clip
        {
          const frAnimationClip* clp = g_graph->m_clips->getClip(it->id);
          clp->evaluate(relTime);
        }
        break;
      }
    }
  }
}

struct evalItem
{
  evalItem(sS32 atime, sS32 rtime, sU32 ID, sU32 cID)
    : m_atime(atime), m_rtime(rtime), m_ID(ID), m_cID(cID)
  {
  }

  bool operator < (const evalItem &b) const
  {
    return m_atime < b.m_atime;
  }

  sS32 m_atime, m_rtime;
  sU32 m_ID, m_cID;  
};

static void evalRangeRecurse(const frAnimationClip *clip, sInt clipStart, sInt start, sInt end, std::vector<evalItem> &evalList)
{
  for (frAnimationClip::elementListCIt it = clip->m_elements.begin(); it != clip->m_elements.end(); ++it)
  {
    const sInt stime = fr::maximum(sInt(start) - it->relStart, 0);
    const sInt etime = fr::minimum(sInt(end) - it->relStart, it->len);
    const sInt cStart = clipStart + it->relStart;

    if (stime < etime)
    {
      switch (it->type)
      {
      case 0: // fcurve (evaluate last value in range)
        evalList.push_back(evalItem(cStart + etime, etime, it->id, it->curveID));
        break;

      case 1: // clip (evaluate clipped range)
        evalRangeRecurse(g_graph->m_clips->getClip(it->id), cStart, stime, etime, evalList);
        break;
      }
    }
  }
}

void frAnimationClip::evaluateRange(sF32 start, sF32 end) const
{
  std::vector<evalItem> evalList; // i hate it that i have to do that.

  evalRangeRecurse(this, 0, start, end, evalList);

  std::stable_sort(evalList.begin(), evalList.end());
  for (sInt i = 0; i < evalList.size(); i++)
    g_graph->m_curves->getOperatorCurves(evalList[i].m_ID).evaluate(evalList[i].m_cID, evalList[i].m_rtime);
}

void frAnimationClip::applyBasePoses() const
{
  for (elementListCIt it = m_elements.begin(); it != m_elements.end(); ++it)
  {
    switch (it->type)
    {
    case 0:
      g_graph->m_curves->applyBasePoses(it->id);
      break;

    case 1:
      g_graph->m_clips->getClip(it->id)->applyBasePoses();
      break;
    }
  }
}

void frAnimationClip::updateLength()
{
  sInt end = 0, curPos = 0;

  for (elementListIt it = m_elements.begin(); it != m_elements.end(); ++it)
  {
    it->updateLen();
    end = fr::maximum(it->relStart + it->len, end);
  }

  m_length = end;
}

// ---- frAnimationClip::element

void frAnimationClip::element::updateLen()
{
  switch (type)
  {
  case 0: // fcurve
    {
      frCurveContainer::opCurves& crv = g_graph->m_curves->getOperatorCurves(id);
      fr::fCurveGroup* curv = crv.getCurve(curveID);
      len = curv ? curv->getLength() : 0;
    }
    break;
    
  case 1: // clip
    {
      frAnimationClip* clp = g_graph->m_clips->getClip(id);

      if (clp)
      {
        clp->updateLength();
        len = clp->getLength();
      }
      else
        len = 0;
    }
    break;
  }
}

frAnimationClip::element::element()
: relStart(0), len(0), type(0), id(0), curveID(0), trackID(0)
{
}

frAnimationClip::element::element(const frAnimationClip::element& x)
  : relStart(x.relStart), len(x.len), type(x.type), id(x.id), curveID(x.curveID),
  trackID(x.trackID)
{
  if (type == 0 && curveID && g_graph->m_ops.find(id) != g_graph->m_ops.end())
		g_graph->m_curves->getOperatorCurves(id).ref(curveID);
}

frAnimationClip::element::~element()
{
  if (type == 0 && curveID && g_graph->m_ops.find(id) != g_graph->m_ops.end())
		g_graph->m_curves->getOperatorCurves(id).unRef(curveID);
}

frAnimationClip::element& frAnimationClip::element::operator =(const frAnimationClip::element& x)
{
  if (&x == this)
    return *this;

	if (x.type == 0 && x.curveID && g_graph->m_ops.find(x.id) != g_graph->m_ops.end())
		g_graph->m_curves->getOperatorCurves(x.id).ref(x.curveID);

  if (type == 0 && curveID && g_graph->m_ops.find(id) != g_graph->m_ops.end())
		g_graph->m_curves->getOperatorCurves(id).unRef(curveID);

  relStart = x.relStart;
  len = x.len;
  type = x.type;
  id = x.id;
  curveID = x.curveID;
  trackID = x.trackID;

  return *this;
}

fr::streamWrapper& operator << (fr::streamWrapper& strm, frAnimationClip& clp)
{
  sU16 ver = 3;

  strm << ver;

  if (ver >= 1 && ver <= 3)
  {
    strm << clp.m_ID;

    fr::string name = clp.m_name;
    strm << name;
    clp.setName(name);

    if (ver>=3)
      strm << clp.m_start;
    else
      clp.m_start = 0;

    sInt elemSize = clp.m_elements.size();
    strm << elemSize;

    if (strm.isWrite())
    {
      for (frAnimationClip::elementListIt it = clp.m_elements.begin(); it != clp.m_elements.end(); ++it)
      {
        strm << it->relStart;
        strm << it->type;
        strm << it->id;

        if (it->type == 0)
          strm << it->curveID;

        if (ver >= 2)
          strm << it->trackID;
      }
    }
    else
    {
      clp.m_elements.clear();

      while (elemSize--)
      {
        frAnimationClip::element elem;

        strm << elem.relStart;
        strm << elem.type;
        strm << elem.id;

        if (elem.type == 0)
          strm << elem.curveID;
        else
          elem.curveID = 0;

        if (ver >= 2)
          strm << elem.trackID;
        else
          elem.trackID = 0;
        
        clp.m_elements.push_back(elem);
      }
    }
  }

  return strm;
}

// ---- frClipContainer

frClipContainer::frClipContainer()
  : m_clipIDCounter(1)
{ 
}

frAnimationClip* frClipContainer::findClipByName(const fr::string& name)
{
  nameMapCIt it = m_names.find(name);

  if (it != m_names.end())
    return &m_clips[it->second];
  else
    return 0;
}

const frAnimationClip* frClipContainer::findClipByName(const fr::string& name) const
{
  nameMapCIt it = m_names.find(name);

  if (it != m_names.end())
  {
    clipMapCIt mit = m_clips.find(it->second);
    FRASSERT(mit != m_clips.end());

    return &mit->second;
  }
  else
    return 0;
}

sU32 frClipContainer::addClip()
{
  sU32 myID = m_clipIDCounter++;

  frAnimationClip& clip = m_clips[myID];

  clip.setID(myID);

  sInt i = 1;
  fr::string name;

  while (name.isEmpty())
  {
    name="New clip";

    if (i != 1)
    {
      sChar buf[16];
      sprintf(buf, " (%d)", i);

      name += buf;
    }

    if (m_names.find(name) != m_names.end())
      name.empty();

    i++;
  }
  
  clip.setName(name);

  return myID;
}

void frClipContainer::clear()
{
  m_clips.clear();
  FRDASSERT(m_names.size() == 0);
}

sU32 frClipContainer::addSimpleClip(sU32 op, sInt param)
{
  const sU32 clipID = addClip();
	const sU32 curveID = g_graph->m_curves->getOperatorCurves(op).addCurve(param);

  FRASSERT(curveID != 0);
  FRDASSERT(m_clips.find(clipID) != m_clips.end());

  m_clips[clipID].addElement(0, op, curveID);

  return clipID;
}

void frClipContainer::deleteOperatorClips(sU32 opID)
{
  clipMapIt it = m_clips.begin();

  while (it != m_clips.end())
  {
		if (it->first == opID && it->second.isSimple())
      it = m_clips.erase(it);
    else
      ++it;
  }
}

void frClipContainer::deleteClip(sU32 clipID)
{
  clipMapIt it = m_clips.find(clipID);
  
  if (it != m_clips.end())
  {
    for (clipMapIt it2 = m_clips.begin(); it2 != m_clips.end(); ++it2)
    {
      if (it2 != it)
        it2->second.deleteElementsReferringClip(clipID);
    }

    m_clips.erase(it);
  }
}

frAnimationClip* frClipContainer::getClip(sU32 id)
{
  clipMapIt it = m_clips.find(id);

  if (it != m_clips.end())
    return &it->second;
  else
    return 0;
}

const frAnimationClip* frClipContainer::getClip(sU32 id) const
{
  clipMapCIt it = m_clips.find(id);

  if (it != m_clips.end())
    return &it->second;
  else
    return 0;
}

void frClipContainer::updateClipLength(sU32 id)
{
  clipMapIt it = m_clips.find(id);
  
  if (it != m_clips.end())
		it->second.updateLength();
}

fr::streamWrapper& operator << (fr::streamWrapper& strm, frClipContainer& cont)
{
  sU16 ver = 2;

  strm << ver;

  if (ver >= 1 && ver <= 2)
  {
    sInt clipSize = cont.m_clips.size();

    if (ver >= 2)
      strm << clipSize;
    else
      clipSize = 0;

    strm << cont.m_clipIDCounter;

    if (strm.isWrite())
    {
      for (frClipContainer::clipMapIt it = cont.m_clips.begin(); it != cont.m_clips.end(); ++it)
      {
        sU32 id = it->first;
        strm << id;
        strm << it->second;
      }
    }
    else
    {
      cont.m_clips.clear();
      cont.m_names.clear();

      while (clipSize--)
      {
        sU32 id = 0;

        strm << id;
        strm << cont.m_clips[id];
      }

      for (frClipContainer::clipMapIt it = cont.m_clips.begin(); it != cont.m_clips.end(); ++it)
      {
        it->second.updateLength();
        it->second.cleanup();
      }
    }
  }

  return strm;
}
