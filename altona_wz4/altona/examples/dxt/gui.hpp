/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#pragma once
#include "base/types2.hpp"
#include "gui/gui.hpp"
#include "gui/listwindow.hpp"
#include "doc.hpp"

/****************************************************************************/

class MainWindow : public sWindow
{
  class WinView *ViewWin;
  class sWireGridFrame *GridWin;
  sSingleListWindow<DocImage> *ListWin;

public:
  MainWindow();
  ~MainWindow();
  void Tag();
  void CmdSet();

  sInt Coder;
  sInt Decoder;
  sInt Decoder2;
  sInt Format;
  sInt DiffMode;
  sInt DiffBoost;
  sInt All;

  sF32 BrightError[2];
  sF32 LinearError[2];
  sF32 SquareError[2];
  sInt CompressionTime;

  sString<256> CodecString;
};

extern MainWindow *App;

/****************************************************************************/

class WinView : public sWireClientWindow
{
  DocImage *Current;
  sImage *Decoded;
  sImage *Difference;
  sInt AlphaMode;
  sInt Zoom;

  void Blit(const sImage *img,const sRect &r);
public:
  WinView();
  ~WinView();
  void Tag();
  void InitWire(const sChar *name);
  void SetImage(DocImage *, sBool loop = false);

  void CmdToggleAlpha();
  void CmdZoom(sDInt dir);

  void OnCalcSize();
  void OnPaint2D();
};

/****************************************************************************/
