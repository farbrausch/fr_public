// This code is in the public domain. See LICENSE for details.

#ifndef __fr_curve_h_
#define __fr_curve_h_

#include "types.h"
#include "math3d_2.h"
#include <vector>

namespace fr
{
  class cubicBezierCurve2D;
  class streamWrapper;

  // ---- function curves
  
  class fCurve
  {
  public:
    enum curveType
    {
      constant = 0,
      linear = 1,
      splineCatmull = 2,
      splineC16 = 3,
    };

    struct key
    {
      sF32  value;
      sInt  frame;
      
      key()
        : value(0), frame(0)
      {
      }
    };
    
    // ---- constructors
    explicit fCurve(curveType type = splineCatmull);
    fCurve(const fCurve& x);

    // ---- mutators: copying
    fCurve& operator =(const fCurve& x);

    // ---- mutators: key management

    key &addKey(sInt frame);
    void removeKey(sInt frame);

    key *getKey(sInt num);
    key *findKey(sInt frame);

    void keyModified(sInt i);
    void setType(curveType type);

    key &subdivide(sInt frame); // subdivison at given frame

    // ---- accessors: keys
    const key *getKey(sInt num) const;
    const sInt getNKeys() const;

    const key *findKey(sInt frame) const;

    // ---- accessors: statistics

    const sInt getFirstFrame() const                          { return m_firstFrame; }
    const sInt getLastFrame() const                           { return m_lastFrame;  }
    const curveType getType() const                           { return m_type; }

    // ---- accessors: evaluation

    sF32 evaluate(sF32 frame) const;
    sF32 evalDerivative(sF32 frame) const;
    
    void convertToBezier(cubicBezierCurve2D& t) const; // conversion to 2d bezier form (for displaying)
    void getActualTangents(sInt i, sF32& dd0, sF32& ds1) const;

  private:
    typedef std::vector<key>          keyVector;
    typedef keyVector::iterator       keyVectorIt;
    typedef keyVector::const_iterator keyVectorCIt;

    keyVectorCIt  findKeyByFrame(sInt frame) const;
    keyVectorIt   findKeyByFrame(sInt frame);

    sF32          adjustOut(keyVectorCIt key0) const          { return (sF32) ((key0 + 1)->frame - key0->frame) / ((key0 + 1)->frame - (key0 - 1)->frame); }
    sF32          adjustIn(keyVectorCIt key0) const           { return (sF32) ((key0 + 1)->frame - key0->frame) / ((key0 + 2)->frame - key0->frame); }
    
    void          calcTangents(keyVectorCIt key0, sF32& dd0, sF32& ds1) const;

    keyVector   m_keys;
    curveType   m_type;
    sInt        m_firstFrame, m_lastFrame;

    friend fr::streamWrapper& operator << (fr::streamWrapper& strm, fCurve& curve);
  };

  fr::streamWrapper& operator << (fr::streamWrapper& strm, fCurve& curve);
  
  // ---- function curve groups
  
  class fCurveGroup
  {
  public:
    // ---- creators
    fCurveGroup(sInt nChannels = 1, fCurve::curveType type = fCurve::splineCatmull);
    fCurveGroup(const fCurveGroup& x);
    ~fCurveGroup();

    // ---- mutators
    fCurveGroup& operator =(const fCurveGroup& x);

    fCurve* getCurve(sInt channel);
    void resize(sInt nChans);

    // ---- accessors

    void evaluate(sF32 time, sF32 *dest) const;

    const fCurve *getCurve(sInt channel) const;
    sInt getLength() const;
    sInt getNChannels() const                                 { return m_nChannels; }

  private:
    fCurve    *m_curves;
    sInt      m_nChannels;

    friend fr::streamWrapper& operator << (fr::streamWrapper& strm, fCurveGroup& cgroup);
  };

  fr::streamWrapper& operator << (fr::streamWrapper& strm, fCurveGroup& cgroup);
};

#endif
