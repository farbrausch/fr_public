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

#include "base/system.hpp"
#include "gui/gui.hpp"
#include "gui/listwindow.hpp"

/****************************************************************************/

class MyWindow : public sWindow
{
public:
  void OnPaint2D()
  {
    sRect2D(Client,sGC_BACK);
    sLine2D(Client.x0,Client.y0,Client.x1-1,Client.y1-1,sGC_DRAW);
    sLine2D(Client.x0,Client.y1-1,Client.x1-1,Client.y0,sGC_DRAW);
  }
};

/****************************************************************************/
/****************************************************************************/

class RectElement : public sObject
{
public:
  sCLASSNAME(RectElement);
  RectElement();
  sBool Add;
  sRect Rect;
  const sChar *GetName();
};

/****************************************************************************/

class Document : public sObject
{
public:
  sArray<RectElement *> Rects;
  Document();
  ~Document();
  void Tag();
};

extern Document *Doc;

/****************************************************************************/
  
class RectWindow : public sWireClientWindow
{
  sCLASSNAME(RectWindow);

  RectElement *DragElem;
  sRect DragRect;
  sInt DragMask;

  sRectRegion Region;
public:
  RectWindow();
  void Tag();
  void InitWire(const sChar *name);
  void OnPaint2D();
  void OnDragMove(const sWindowDrag &dd);
  void OnDragDraw(const sWindowDrag &dd,sDInt mode);
  void OnDragSelect(const sWindowDrag &dd);

  void OnCmdDelete();
  void OnCmdToggle();
};

/****************************************************************************/

class MainWindow : public sWindow
{
  RectWindow *RectWin;
public:
  sCLASSNAME(MainWindow);
  MainWindow();
  ~MainWindow();
  void Tag();
  void UpdateList();

  sSingleListWindow<RectElement> *ListWin;
};

/****************************************************************************/

