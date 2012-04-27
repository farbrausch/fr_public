/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WERKKZEUG4_WZ4_SKELETON_HPP
#define FILE_WERKKZEUG4_WZ4_SKELETON_HPP

#include "base/types2.hpp"
#include "base/math.hpp"
#include "wz4lib/doc.hpp"
#include "wz4frlib/bspline.hpp"

class Wz4Channel;

/****************************************************************************/

struct Wz4AnimKey       // 16 words
{
  void Init();
  void ToMatrix(sMatrix34 &mat) const;
  void ToMatrixInv(sMatrix34 &mat) const;
  template <class streamer> void Serialize_(streamer &stream);
  void Serialize(sWriter &);
  void Serialize(sReader &);

  sF32 Time;
  sF32 Weight;
  sU32 pad;
  sVector31 Scale;
  sQuaternion Rot;
  sVector31 Trans;
  sVector31 User;
};

struct Wz4AnimJoint 
{
  void Init();
  Wz4Channel *Channel;
  sMatrix34 fake; // only used internally by xsi loader. just ignore.
  sMatrix34 BasePose;
  sInt Parent;
  wDocName Name;
  sInt Temp;
};

/****************************************************************************/

void SerializeWz4Channel(sReader &,Wz4Channel *&);
void SerializeWz4Channel(sWriter &,Wz4Channel *);

enum Wz4ChannelKind
{
  Wz4ChannelKindIllegal = 0,
  Wz4ChannelKindConstant,
  Wz4ChannelKindLinear,
  Wz4ChannelKindPerFrame,
  Wz4ChannelKindSpline,
  Wz4ChannelKindCat,
};

class Wz4Channel : public wObject
{
public:
  Wz4Channel();
  sF32 MaxTime;
  sInt Kind;
  virtual void Evaluate(sF32 time,Wz4AnimKey &result) {}
  virtual void Serialize(sWriter &) { sFatal(L"cant serialize this kind of channel"); }
  virtual Wz4Channel *CopyTo() { sFatal(L"cant copy this kind of channel"); return 0; }
};

class Wz4ChannelConstant : public Wz4Channel
{
public:
  Wz4ChannelConstant();
  void Evaluate(sF32 time,Wz4AnimKey &result);

  Wz4AnimKey Start;
  Wz4Channel *CopyTo();
};

class Wz4ChannelLinear : public Wz4Channel
{
  sBool prepared;
  sQuaternion RotStart;
  sVector31 ScaleStart;
public:
  Wz4ChannelLinear();
  void Prepare();
  void Evaluate(sF32 time,Wz4AnimKey &result);

  sMatrix34 Start;
  sVector30 Speed;
  sVector30 Gravity;

  sVector30 ScaleSpeed;

  sVector30 RotAxis;
  sF32 RotSpeed;

  sVector31 UserStart;
  sVector30 UserSpeed;

  sF32 Weight;
};

class Wz4ChannelPerFrame : public Wz4Channel
{
public:
  Wz4ChannelPerFrame();
  ~Wz4ChannelPerFrame();
  template <class streamer> void Serialize_(streamer &stream);
  void Serialize(sWriter &);
  void Serialize(sReader &);
  Wz4Channel *CopyTo();

  void Evaluate(sF32 time,Wz4AnimKey &result);

  Wz4AnimKey Start;
  sInt Keys;
  sVector31 *Scale;
  sQuaternion *Rot;
  sVector31 *Trans;
  sVector31 *User;
};

class Wz4ChannelSpline : public Wz4Channel
{
public:
  Wz4ChannelSpline();
  ~Wz4ChannelSpline();
  template <class streamer> void Serialize_(streamer &stream);
  void Serialize(sWriter &);
  void Serialize(sReader &);

  sBool Approximate(const Wz4ChannelPerFrame& target,sF32 relativeError=0.001f);
  void Evaluate(sF32 time,Wz4AnimKey &result);

  Wz4AnimKey Start;
  sInt Keys;
  sBSpline<sVector30> Scale; // yes, sVector30 (we need to do linear combinations)
  sBSpline<sQuaternion> Rot;
  sBSpline<sVector30> Trans; // yes, sVector30 (we need to do linear combinations)
};

class Wz4ChannelCat : public Wz4Channel
{
public:
  Wz4ChannelCat();
  ~Wz4ChannelCat();
  void Evaluate(sF32 time,Wz4AnimKey &result);

  sArray<Wz4Channel *> Channels;
};

/****************************************************************************/

class Wz4Skeleton : public wObject
{
public:
  Wz4Skeleton();
  ~Wz4Skeleton();
  template <class streamer> void Serialize_(streamer &stream);
  void Serialize(sWriter &);
  void Serialize(sReader &);
  void CopyFrom(Wz4Skeleton *src);
  sInt FindJoint(const sChar *name);
  void FixTime(sF32 spf);

  void Evaluate(sF32 time,sMatrix34 *mat,sMatrix34 *basemat=0);
  void EvaluateCM(sF32 time,sMatrix34 *mat,sMatrix34CM *basemat);
  void EvaluateBlendCM(sF32 time1,sF32 time2,sF32 reftime,sMatrix34 *mat,sMatrix34CM *basemat);
  void EvaluateFadeCM(sF32 time1,sF32 time2,sF32 fade,sMatrix34 *mat,sMatrix34CM *basemat);

  sArray<Wz4AnimJoint> Joints;
  sF32 TotalTime;                     // total time in seconds
};
 
/****************************************************************************/

#endif // FILE_WERKKZEUG4_WZ4_SKELETON_HPP

