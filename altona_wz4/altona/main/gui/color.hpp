/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#pragma once
#include "base/types.hpp"
#include "base/math.hpp"
#include "gui/window.hpp"
#include "gui/controls.hpp"
#include "util/image.hpp"

/****************************************************************************/

struct sColorGradientKey
{
  sF32 Time;
  sVector4 Color;
};

class sColorGradient : public sObject
{
public:
  sArray<sColorGradientKey> Keys;
  sF32 Gamma;
  sInt Flags;

  void AddKey(sF32 time,sU32 col);
  void AddKey(sF32 time,const sVector4 &col);
  void Sort();
  void CalcUnwarped(sF32 time,sVector4 &col);
  void Calc(sF32 time,sVector4 &col);

  sCLASSNAME(sColorGradient);
  sColorGradient();
  ~sColorGradient();
  void Clear();

  template <class streamer> void Serialize_(streamer &);
  void Serialize(sReader &stream);
  void Serialize(sWriter &stream);
};


class sColorGradientControl : public sControl
{
  sImage *Bar;
  sInt AlphaEnable;
public:
  sCLASSNAME_NONEW(sColorGradientControl);
  sColorGradientControl(sColorGradient *,sBool alpha);
  ~sColorGradientControl();
  void Tag();

  sColorGradient *Gradient;
  void OnPaint2D();
  void OnDrag(const sWindowDrag &dd);
};

class sColorPickerWindow : public sWindow
{
  class sGridFrame *Grid;

  sF32 *FPtr;
  sU8 *UPtr;

  sF32 Red;
  sF32 Green;
  sF32 Blue;
  sF32 Alpha;

  sF32 Hue;
  sF32 Sat;
  sF32 Value;


  sRect BarRect;
  sRect PickRect;
  sRect WarpRect;
  sRect PaletteRect;

  sInt DragMode;
  sObject *TagRef;
  sBool AlphaEnable;
  sColorGradient *Gradient;
  sInt DragKey;
  sF32 DragStart;

  sImage *PickImage;
  sF32 PickSat;
  sF32 PickHue;
  sImage *GradientImage;
  sImage *WarpImage;
  sBool GradientChanged;

  sRect PaletteBoxes[32];

  void MakeGui();
  void SelectKey(sInt nr);
public:
  sCLASSNAME(sColorPickerWindow);
  sColorPickerWindow();
  ~sColorPickerWindow();
  void Tag();

  void Init(sF32 *,sObject *tagref,sBool alpha);
  void Init(sU32 *,sObject *tagref,sBool alpha);
  void Init(sColorGradient *grad,sBool alpha);
  void ChangeGradient();
  void ChangeRGB();
  void ChangeHSV();
  void UpdateOutput();

  void OnPaint2D();
  void OnCalcSize();
  void OnLayout();
  void OnDrag(const sWindowDrag &dd);
  sBool OnKey(sU32 key);

  void CmdDelete();

  sMessage ChangeMsg;

  static sF32 PaletteColors[32][4];
};

/****************************************************************************/

