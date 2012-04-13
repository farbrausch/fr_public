// This code is in the public domain. See LICENSE for details.

#ifndef __animbase_h_
#define __animbase_h_

#include "types.h"
#include "curve.h"
#include "tool.h"
#include <list>
#include <vector>
#include <map>

namespace fr
{
  class streamWrapper;
}

class frCurveContainer
{
public:
  class opCurves
  {
  public:
    opCurves();
    opCurves(sU32 opID);
    
    sU32            addCurve(sInt param);
    fr::fCurveGroup *getCurve(sU32 curveID);
    sInt            getAnimIndex(sU32 curveID);
    void            deleteCurve(sU32 curveID);
    void            evaluate(sU32 curveID, sF32 time);

    void            ref(sU32 curveID);
    void            unRef(sU32 curveID);

    void            applyBasePoses() const;
    void            updateBasePose(sInt param);
    
  private:
    struct curve
    {
      curve()
        : param(0), refs(1)
      {
      }

      curve(sInt parm, sInt nChannels);

      fr::sharedPtr<fr::fCurveGroup>  grp;
      sInt                            param;
      sUInt                           refs;
    };

    typedef std::map<sU32, curve>             curveMap;
    typedef curveMap::const_iterator          curveMapCIt;
    typedef curveMap::iterator                curveMapIt;
    
    sU32      m_opID;
    sU32      m_curveNdx;
    curveMap  m_curves;

    friend fr::streamWrapper &operator << (fr::streamWrapper &strm, frCurveContainer &cont);
    friend class frCurveContainer;
  };

  frCurveContainer();

  void        clear();

  opCurves    &getOperatorCurves(sU32 opID);
  void        deleteOperatorCurves(sU32 opID);

  void        applyBasePoses(sU32 opID) const;
  void        applyAllBasePoses() const;

  void        fixOldAnimIndices();

  void        updateBasePose(sU32 opID, sInt param);

private:
  typedef std::map<sU32, opCurves>    opCurveMap;
  typedef opCurveMap::const_iterator  opCurveMapCIt;
  typedef opCurveMap::iterator        opCurveMapIt;

  opCurveMap    m_opCurves;

  friend fr::streamWrapper &operator << (fr::streamWrapper &strm, frCurveContainer &cont);
};

fr::streamWrapper &operator << (fr::streamWrapper &strm, frCurveContainer &cont);

class frAnimationClip
{
public:
  frAnimationClip(sU32 id=0);
  frAnimationClip(const frAnimationClip &x);
  ~frAnimationClip();

  // access
  inline const fr::string &getName() const      { return m_name; }
  sBool setName(const fr::string &name);
  void  setID(const sU32 ID);

  // elements
  void  addElement(sInt start, sU32 opID, sU32 curveID);   // add (fcurve) element
  void  addElement(sInt start, sU32 trackID, const fr::string &clipName);   // add subclip
  void  deleteElement(sInt ind);
  void  deleteElementsReferringClip(sU32 clipID);

  void  cleanup();

  // evaluate
  void  evaluate(sF32 time) const;
  void  evaluateRange(sF32 start, sF32 end) const;
  void  applyBasePoses() const;

  // length
  sInt  getLength() const                       { return m_length; }
  sU32  getID() const                           { return m_ID; }
  sF32  getStart() const                        { return m_start; }
  void  setStart(sF32 start)                    { m_start=start; }

  sBool isSimple() const                        { return m_elements.size() == 1 && m_elements.front().type==0; }

  void  updateLength();
  
  struct element
  {
    sInt  relStart; // relative start (ms)
    sInt  len;      // length (ms); this is just a cache

    sInt  type;     // 0=fcurve 1=clip
    sU32  id;       // op/clip ID
    sU32  curveID;  // curve ID (fcurve only)
    
    sInt  trackID;  // track ID (this is symbolic)

    bool operator < (const element &x) const
    {
      if (trackID<x.trackID)
        return true;
      else if (trackID>x.trackID)
        return false;

      if (relStart<x.relStart)
        return true;

      return false;
    }

    bool operator == (const element &x) const
    {
      return (relStart==x.relStart && type==x.type && id==x.id && curveID==x.curveID && trackID==x.trackID);
    }

    bool operator != (const element &x) const
    {
      return !(*this==x);
    }
    
    element();
    element(const element &x);
    ~element();

    element &operator =(const element &x);

    void updateLen();
  };

  typedef std::vector<element>        elementList;
  typedef elementList::const_iterator elementListCIt;
  typedef elementList::iterator       elementListIt;

  elementList   m_elements;
    
private:
  fr::string    m_name;
  sU32          m_ID;
  sInt          m_length;
  sF32          m_start;    // start offset (edit only)
  
  friend fr::streamWrapper &operator << (fr::streamWrapper &strm, frAnimationClip &clip);
};

fr::streamWrapper &operator << (fr::streamWrapper &strm, frAnimationClip &clip);

class frClipContainer
{
public:
  frClipContainer();

  // clip management
  frAnimationClip       *findClipByName(const fr::string &name);
  const frAnimationClip *findClipByName(const fr::string &name) const;

  void                  clear();
  
  sU32                  addClip();
  sU32                  addSimpleClip(sU32 op, sInt param);

  void                  deleteOperatorClips(sU32 opID);
  void                  deleteClip(sU32 clipID);

  frAnimationClip       *getClip(sU32 id);
  const frAnimationClip *getClip(sU32 id) const;

  void                  updateClipLength(sU32 id);

private:
  class strCompare
  {
  public:
    bool operator ()(const fr::string &s1, const fr::string &s2) const
    {
      return s1.compare(s2, sTRUE)<0;
    }
  };

  typedef std::map<sU32, frAnimationClip>         clipMap;
  typedef clipMap::const_iterator                 clipMapCIt;
  typedef clipMap::iterator                       clipMapIt;

  clipMap       m_clips;
  sU32          m_clipIDCounter;

  void operator =(const frAnimationClip &x) {}

  friend fr::streamWrapper &operator << (fr::streamWrapper &strm, frClipContainer &cont);
  
public:
  typedef std::map<fr::string, sU32, strCompare>  nameMap;
  typedef nameMap::const_iterator                 nameMapCIt;
  typedef nameMap::iterator                       nameMapIt;
  
  nameMap       m_names;
};

fr::streamWrapper &operator << (fr::streamWrapper &strm, frClipContainer &cont);

#endif
