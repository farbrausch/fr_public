// This code is in the public domain. See LICENSE for details.

// function curves implementation

#include "stdafx.h"
#include "types.h"
#include "curve.h"
#include "bezier.h"
#include "tstream.h"
#include "tool.h"

namespace fr
{
  // ---- fCurve: function curve

  fCurve::fCurve(curveType type)
    : m_type(type), m_firstFrame(0), m_lastFrame(0)
  {
  }

  fCurve::fCurve(const fCurve& x)
    : m_type(x.m_type), m_firstFrame(x.m_firstFrame), m_lastFrame(x.m_lastFrame), m_keys(x.m_keys)
  {
  }

  fCurve& fCurve::operator =(const fCurve& x)
  {
    m_type=x.m_type;
    m_firstFrame=x.m_firstFrame;
    m_lastFrame=x.m_lastFrame;
    m_keys=x.m_keys;

    return *this;
  }

  fCurve::key& fCurve::addKey(sInt frame)
  {
    keyVectorIt it;
    for (it=m_keys.begin(); it!=m_keys.end() && frame>it->frame; ++it);

    it=m_keys.insert(it, key());
    it->frame=frame;

    m_firstFrame=minimum(m_firstFrame, frame);
    m_lastFrame=maximum(m_lastFrame, frame);

    return *it;
  }

  void fCurve::removeKey(sInt frame)
  {
    for (keyVectorIt it=m_keys.begin(); it!=m_keys.end(); ++it)
    {
      if (it->frame==frame)
      {
        m_keys.erase(it);

        m_firstFrame=(m_keys.size())?m_keys[0].frame:0;
        m_lastFrame=(m_keys.size())?m_keys[m_keys.size()-1].frame:0;

        break;
      }
    }
  }

  fCurve::key* fCurve::getKey(sInt num)
  {
    if (num>=0 && num<m_keys.size())
      return &m_keys[num];
    else
      return 0;
  }

  fCurve::key* fCurve::findKey(sInt frame)
  {
    for (keyVectorIt it=m_keys.begin(); it!=m_keys.end(); ++it)
    {
      if (it->frame==frame)
        return &(*it);
    }

    return 0;
  }

  void fCurve::keyModified(sInt i)
  {
    if (i==0)
      m_firstFrame=m_keys[i].frame;
    else if (i==getNKeys()-1)
      m_lastFrame=m_keys[i].frame;

    if (m_type==constant && i>=getNKeys()-2)
      m_keys[getNKeys()-1].value=m_keys[getNKeys()-2].value;
  }

  void fCurve::setType(curveType type)
  {
    m_type=type;
  }

  fCurve::key& fCurve::subdivide(sInt frame)
  {
    FRDASSERT(frame>m_firstFrame && frame<m_lastFrame);

    keyVectorIt key0 = findKeyByFrame(frame);
    keyVectorIt key1 = key0 + 1;

    const sF32 val = evaluate(frame);

    keyVectorIt newKey = m_keys.insert(key1, key());

    newKey->frame = frame;
    newKey->value = val;

    return *newKey;
  }

  // ---- accessors

  const fCurve::key* fCurve::getKey(sInt num) const
  {
    if (num>=0 && num<m_keys.size())
      return &m_keys[num];
    else
      return 0;
  }

  const sInt fCurve::getNKeys() const
  {
    return m_keys.size();
  }

  const fCurve::key* fCurve::findKey(sInt frame) const
  {
    for (keyVectorCIt it=m_keys.begin(); it!=m_keys.end(); ++it)
    {
      if (it->frame==frame)
        return &(*it);
    }

    return 0;
  }

  sF32 fCurve::evaluate(sF32 frame) const
  {
    if (frame <= m_firstFrame || m_keys.size() == 1)
      return m_keys.front().value;
    else if (frame >= m_lastFrame)
      return m_keys.back().value;
    else
    {
      keyVectorCIt key0, key1;
      
      key0 = findKeyByFrame(frame);

      if (m_type == constant)
        return key0->value;

      key1 = key0 + 1;
      frame -= key0->frame;

      const sInt tLength = key1->frame - key0->frame;
      const sF32 t = (tLength > 1) ? frame / tLength : 0.0f;

      if (m_type == linear)
        return key0->value + t * (key1->value - key0->value);

      const sF32 tt = t * t, ttt = tt * t;
      const sF32 h2 = 3.0f * tt - 2.0f * ttt, h1 = 1.0f - h2, h3 = ttt - 2.0f * tt + t, h4 = ttt - tt;
      
      sF32 dd0, ds1;
      calcTangents(key0, dd0, ds1);
      
      return key0->value * h1 + key1->value * h2 + dd0 * h3 + ds1 * h4;
    }
  }

  sF32 fCurve::evalDerivative(sF32 frame) const
  {
    if (frame < m_firstFrame || m_keys.size() == 1 || m_type == constant)
      return 0;
    else if (frame >= m_lastFrame)
      return 0;
    else
    {
      keyVectorCIt key0, key1;

      key0 = findKeyByFrame(frame);
      key1 = key0 + 1;
      frame -= key0->frame;

      const sInt tLength = key1->frame - key0->frame;

      if (m_type == linear)
        return (key1->value - key0->value) / tLength;

      const sF32 t = (tLength > 1) ? frame / tLength : 0.0f, tt = t * t;
      const sF32 h1 = 6.0f * tt - 6.0f * t, h3=3.0f * tt - 4.0f * t + 1.0f, h4=3.0f * tt - 2.0f * t;
      // h2' is just -h1', so not evaluated here

      sF32 dd0, ds1;
      calcTangents(key0, dd0, ds1);

      return key0->value * h1 - key1->value * h1 + dd0 * h3 + ds1 * h4;
    }
  }

  void fCurve::convertToBezier(cubicBezierCurve2D& t) const
  {
    if (getNKeys()>1)
    {
      t.resize(getNKeys()-1);

      cubicBezierSegment2D *seg = t.segment;

      for (keyVectorCIt it = m_keys.begin(); (it + 1) != m_keys.end(); ++it)
      {
        const sF32 v0 = it->value, v1 = (m_type != constant) ? (it + 1)->value : it->value;
        const sInt f0 = it->frame, f1 = (it + 1) -> frame;

        sF32 dd0, ds1;
        calcTangents(it, dd0, ds1);

        seg->A=vector2(f0, v0);
        seg->B=vector2((2.0f * f0 + f1) / 3.0f, v0 + dd0 / 3.0f);
        seg->C=vector2((f0 + 2.0f * f1) / 3.0f, v1 - ds1 / 3.0f);
        seg->D=vector2(f1, v1);
        seg++;
      }
    }
  }

  void fCurve::getActualTangents(sInt i, sF32& dd0, sF32& ds1) const
  {
    calcTangents(m_keys.begin() + i, dd0, ds1);
  }

  // ---- private stuff

  fCurve::keyVectorCIt fCurve::findKeyByFrame(sInt frame) const
  {
    if (m_keys.size() == 1)
      return m_keys.begin();

    keyVectorCIt keyi;
    for (keyi = m_keys.begin(); (keyi + 1) != m_keys.end() && frame > (keyi + 1)->frame; ++keyi);

    return keyi;
  }

  fCurve::keyVectorIt fCurve::findKeyByFrame(sInt frame)
  {
    if (m_keys.size() == 1)
      return m_keys.begin();

    keyVectorIt keyi;
    for (keyi = m_keys.begin(); (keyi + 1) != m_keys.end() && frame > (keyi + 1)->frame; ++keyi);

    return keyi;
  }

  void fCurve::calcTangents(fCurve::keyVectorCIt key0, sF32& dd0, sF32& ds1) const
  {
    keyVectorCIt key1 = key0 + 1;

    sF32 tDelta = key1->frame - key0->frame;
    sF32 d10 = key1->value - key0->value;
    sF32 adj0 = 1, adj1 = 1;

    if (key0->frame != m_firstFrame)
      adj0 = tDelta / (key1->frame - (key0 - 1)->frame);

    if (key1->frame != m_lastFrame)
      adj1 = tDelta / ((key1 + 1)->frame - key0->frame);
    
    switch (m_type)
    {
    case constant:
      dd0 = 0.0f;
      ds1 = 0.0f;
      break;

    case linear:
      dd0 = d10;
      ds1 = d10;
      break;

    case splineCatmull: 
      dd0 = (key0->frame == m_firstFrame) ? d10 : adj0 * 0.5f * (key1->value - (key0 - 1)->value);
      ds1 = (key1->frame == m_lastFrame)  ? d10 : adj1 * 0.5f * ((key1 + 1)->value - key0->value);
      break;

    case splineC16:
      dd0 = (key0->frame == m_firstFrame) ? d10 : adj0 / 6.0f * (key1->value - (key0 - 1)->value);
      ds1 = (key1->frame == m_lastFrame)  ? d10 : adj1 / 6.0f * ((key1 + 1)->value - key0->value);
      break;
    }
  }

  fr::streamWrapper& operator << (fr::streamWrapper& strm, fCurve& curve)
  {
    sU16 ver = 2;
    sInt nKeys = curve.m_keys.size();

    strm << ver;

    if (ver >= 1 && ver <= 2)
    {
      sInt type = (sInt) curve.m_type;
      strm << type;
      curve.m_type = (fCurve::curveType) type;

      strm << nKeys;
    
      curve.m_keys.resize(nKeys);
      for (sInt i=0; i<nKeys; i++)
      {
        strm << curve.m_keys[i].value;

        if (ver == 1)
        {
          sF32 a, b;
          strm << a << b;
        }

        strm << curve.m_keys[i].frame;
      }

      if (nKeys)
      {
        curve.m_firstFrame = curve.m_keys[0].frame;
        curve.m_lastFrame = curve.m_keys[nKeys - 1].frame;
      }
      else
        curve.m_firstFrame = curve.m_lastFrame = 0;
    }

    return strm;
  }

  // ---- fCurveGroup: Group of function curves

  fCurveGroup::fCurveGroup(sInt nChannels, fCurve::curveType type)
  {
    m_nChannels = nChannels;
    m_curves = new fCurve[m_nChannels];

    for (sInt i = 0; i < m_nChannels; i++)
      m_curves[i].setType(type);
  }

  fCurveGroup::fCurveGroup(const fCurveGroup& x)
  {
    m_nChannels = x.m_nChannels;
    m_curves = new fCurve[m_nChannels];

    for (sInt i = 0; i < m_nChannels; i++)
      m_curves[i] = x.m_curves[i];
  }

  fCurveGroup::~fCurveGroup()
  {
    delete[] m_curves;
  }

  // ---- mutators

  fCurveGroup& fCurveGroup::operator =(const fCurveGroup& x)
  {
    delete[] m_curves;

    m_nChannels = x.m_nChannels;
    m_curves = new fCurve[m_nChannels];

    for (sInt i = 0; i < m_nChannels; i++)
      m_curves[i] = x.m_curves[i];

    return *this;
  }

  fCurve* fCurveGroup::getCurve(sInt channel)
  {
    FRDASSERT(channel>=0 && channel<m_nChannels);
    return m_curves + channel;
  }

  void fCurveGroup::resize(sInt nChans)
  {
    if (m_nChannels!=nChans)
    {
      delete[] m_curves;
      m_curves=new fCurve[nChans];
      m_nChannels=nChans;
    }
  }

  // ---- accessors

  const fCurve* fCurveGroup::getCurve(sInt channel) const
  {
    FRDASSERT(channel>=0 && channel<m_nChannels);
    return m_curves + channel;
  }

  void fCurveGroup::evaluate(sF32 time, sF32 *dest) const
  {
    for (sInt i = 0; i < m_nChannels; i++)
      dest[i] = m_curves[i].evaluate(time);
  }

  sInt fCurveGroup::getLength() const
  {
    sInt start = 0x7fffffff;
    sInt end = 0;

    for (sInt i = 0; i < m_nChannels; i++)
    {
      start = fr::minimum(start, m_curves[i].getFirstFrame());
      end = fr::maximum(end, m_curves[i].getLastFrame());
    }

    return end - start;
  }

  // ---- tools

  fr::streamWrapper& operator << (fr::streamWrapper& strm, fCurveGroup& cgroup)
  {
    sU16 ver = 1;

    strm << ver;

    if (ver == 1)
    {
      sU32 nChans = cgroup.m_nChannels;
      strm << nChans;

      cgroup.resize(nChans);

      for (sInt i = 0; i < cgroup.m_nChannels; i++)
        strm << cgroup.m_curves[i];
    }
    
    return strm;
  }
}
