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

#include "util/effect.hpp"
#include "base/system.hpp"
#if 1
/****************************************************************************/

sEffectManager_ *sEffectManager = 0;

/****************************************************************************/

sEffect::sEffect() 
{
  Valid = sFALSE;
  Active = sTRUE;
  Order = 0;
}

sEffect::~sEffect()
{
}

sBool sEffect::OnInit()
{
  return sFALSE;
}

void sEffect::OnFrame(sInt)
{
}

void sEffect::OnPaintPre(const sViewport &vp)
{
}

void sEffect::OnPaintMain()
{
}

void sEffect::OnPaintIPP()
{
}

sBool sEffect::OnInput(const sInput2Event &ie)
{
  return sFALSE;
}

/****************************************************************************/
/****************************************************************************/

sEffectManager_::sEffectManager_()
{
  Effects.HintSize(64);
}

sEffectManager_::~sEffectManager_()
{
  sDeleteAll(Effects);
}

void sEffectManager_::AddEffect(sEffect *e)
{
  Effects.AddTail(e);
}

void sEffectManager_::RemoveEffect(sEffect *e)
{
  sEffect *ei;
  sFORALL(Effects,ei)
  {
    if (ei==e)
    {
      Effects.RemOrder(e);
      sDelete(e);
      return;
    }
  }
}

/****************************************************************************/

void sEffectManager_::OnInit()
{
  sEffect *e;

  sSortUp(Effects,&sEffect::Order);

  sFORALL(Effects,e)
    e->Valid = e->OnInit();
}

void sEffectManager_::OnFrame(sInt delta)
{
  sEffect *e;

  sFORALL(Effects,e)
  {
    if (e->Valid && e->Active)
      e->OnFrame(delta);
  }
}

void sEffectManager_::OnPaintPre(const sViewport &vp)
{
  sEffect *e;

  sFORALL(Effects,e)
  {
    if (e->Valid && e->Active)
      e->OnPaintPre(vp);
  }
}

void sEffectManager_::OnPaintMain()
{
  sEffect *e;

  sFORALL(Effects,e)
  {
    if (e->Valid && e->Active)
      e->OnPaintMain();
  }
}

void sEffectManager_::OnPaintIPP()
{
  sEffect *e;

  sFORALL(Effects,e)
  {
    if (e->Valid && e->Active)
      e->OnPaintIPP();
  }
}

sBool sEffectManager_::OnInput(const sInput2Event &ie)
{
  sEffect *e;

  sFORALL(Effects,e)
  {
    if(e->Valid && e->Active && e->OnInput(ie))
      return 1;
  }
  return 0;
}

/****************************************************************************/

void sEffectManager_::FullService(sInt delta, const sViewport &vp)
{
  OnFrame(delta);
  OnPaintPre(vp);
  OnPaintMain();
  OnPaintIPP();
}
#endif
/****************************************************************************/
/****************************************************************************/
