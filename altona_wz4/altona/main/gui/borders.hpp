/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_GUI_BORDERS_HPP
#define FILE_GUI_BORDERS_HPP

#ifndef __GNUC__
#pragma once
#endif

#include "gui/window.hpp"
#include "gui/manager.hpp"
#include "gui/controls.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Borders                                                            ***/
/***                                                                      ***/
/****************************************************************************/

class sFocusBorder : public sWindow
{
public:
  sCLASSNAME(sFocusBorder);
  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();
};

class sThickBorder : public sWindow
{
public:
  sCLASSNAME(sThickBorder);
  sThickBorder();
  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();
  sBool Inverted;
};

class sThinBorder : public sWindow
{
public:
  sCLASSNAME(sThinBorder);
  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();
};

class sSpaceBorder : public sWindow
{
  sInt Pen;
public:
  sCLASSNAME(sThinBorder);
  sSpaceBorder(sInt pen);
  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();
};

class sToolBorder : public sWindow
{
protected:
  void OnLayout(sInt y0,sInt y1);

  enum
  {
    WF_TB_RIGHTALIGNED = sWF_USER1, // additional window flag
  };

public:
  sCLASSNAME(sToolBorder);
  sToolBorder();
  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();

  void AddMenu(sChar *name,const sMessage &msg);
  void AddSpace(sBool rightaligned=sFALSE);
  void AddRightAligned(sWindow *w);
  sBool Bottom;
};

class sScrollBorder : public sWindow
{
  sInt Width;
  sInt KnopMin;
  sInt DragMode;
  sInt DragStartX;
  sInt DragStartY;
  sRect ButtonX;
  sRect ButtonY;
  sRect ButtonZ;
  sBool CalcKnop(sInt &a,sInt &b,sInt client,sInt inner,sInt button,sInt scroll);
public:
  sCLASSNAME(sScrollBorder);
  sScrollBorder();
  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();
  void OnDrag(const sWindowDrag &dd);
};

struct sStatusBorderItem
{
  sStatusBorderItem();
  sInt Width;                     // width in pixel, negative from right border, 0 for all remaining
  sStringDesc Buffer;             // text mode
  sInt ProgressMax;               // progress bar mode
  sInt ProgressValue;
  sU32 Color;                     // 0 for default

  sRect Client;                   // calculated automatically
};

class sStatusBorder : public sWindow
{
  sArray<sStatusBorderItem> Items;
  sString<1024> PrintBuffer;
  sInt Height;
  sU32 Color;
public:
  sCLASSNAME(sStatusBorder);
  sStatusBorder();
  ~sStatusBorder();

  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();

  void AddTab(sInt width,sInt maxstring=256);
  void Print(sInt tab,const sChar *string,sInt len=-1,sU32 color=0);
  void SetColor(sInt tab,sU32 rgb);        // gets reset by Print/PrintF
  void Progress(sInt tab,sInt value,sInt max);

  sPRINTING1(PrintF,sFormatStringBuffer buf=sFormatStringBase(PrintBuffer,format);buf,Print(arg1,PrintBuffer);,sInt);  
};

/****************************************************************************/

#endif
