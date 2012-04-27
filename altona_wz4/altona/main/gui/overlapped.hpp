/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "window.hpp"

/****************************************************************************/

class sOverlappedFrame : public sWindow
{
  sBool BackClear;
public:
  sOverlappedFrame();
  sCLASSNAME(sOverlappedFrame);
  void OnLayout();
  void OnPaint2D();
};

/****************************************************************************/

class sSizeBorder : public sWindow
{
  sInt DragMask;
  sRect DragStart;
public:
  sCLASSNAME(sSizeBorder);
  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();
  void OnDrag(const sWindowDrag &dd);
};

/****************************************************************************/

class sTitleBorder : public sWindow
{
  sRect DragStart;
  sButtonControl *CloseButton;
public:
  sCLASSNAME(sTitleBorder);
  const sChar *Title;

  sTitleBorder(sMessage closemsg=sMessage());
  sTitleBorder(const sChar *title, sMessage closemsg=sMessage());
  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();
  void OnDrag(const sWindowDrag &dd);
};

/****************************************************************************/
