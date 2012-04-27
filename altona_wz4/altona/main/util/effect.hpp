/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef HEADER_ALTONA_UTIL_EFFECT
#define HEADER_ALTONA_UTIL_EFFECT

#ifndef __GNUC__
#pragma once
#endif

#include "base/types.hpp"
#include "base/math.hpp"

#if 1
/****************************************************************************/

class sEffect 
{
  friend class sEffectManager_;
  sBool Valid;
public:

  sBool Active;

  // general interface

  sEffect();
  virtual ~sEffect();
  virtual sBool OnInit();
  virtual sBool OnInput(const sInput2Event &ie); // please use only for debugging

  // these are called every frame, in this order
  virtual void OnFrame(sInt delta);
  virtual void OnPaintPre(const class sViewport &vp); // per viewport
  virtual void OnPaintMain(); // per viewport
  virtual void OnPaintIPP(); // per viewport

  sInt Order;                     // suggested range: -128..127
};

/****************************************************************************/

//! howto handle viewports and splitscreen?
class sEffectManager_
{
  sStaticArray<sEffect *> Effects;
public:
  sEffectManager_();
  ~sEffectManager_();
  void AddEffect(sEffect *);
  void RemoveEffect(sEffect *);

  void OnInit();
  sBool OnInput(const sInput2Event &ie); 

  void OnFrame(sInt delta);
  void OnPaintPre(const class sViewport &vp); // per viewport
  void OnPaintMain(); // per viewport
  void OnPaintIPP(); // per viewport

  void FullService(sInt delta,const class sViewport &vp);   // implement OnTick(), OnFrame(), OnPaint*()
};

extern sEffectManager_ *sEffectManager; // global effect manager is not really required.

/****************************************************************************/
#endif
// HEADER_ALTONA_UTIL_EFFECT
#endif


