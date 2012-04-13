// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef __WINPARA_HPP__
#define __WINPARA_HPP__

#include "_util.hpp"
#include "_gui.hpp"
#include "apptool.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   windows                                                            ***/
/***                                                                      ***/
/****************************************************************************/

class ParaWin : public ToolWindow
{
  class PageOp *Op;
  class PageDoc *Doc;
  class TimeOp *OpTime;
  class TimeDoc *DocTime;
  sGridFrame *Grid;
  sInt ShowEvent;
  class ViewWin *View;
public:
  ParaWin();
  ~ParaWin();
  void Tag();
  sU32 GetClass() { return sCID_TOOL_PARAWIN; }
  void OnCalcSize();
  void OnLayout();
  void OnPaint() {}
  sBool OnCommand(sU32 cmd);

  void SetOp(PageOp *op,PageDoc *doc);
  PageOp *GetOp() { return Op; }  
  void SetTime(TimeOp *op,TimeDoc *doc);
  void SetView(ViewWin *win);
};

/****************************************************************************/

#endif
