// This file is distributed under a BSD license. See LICENSE.txt for details.

#pragma once

#include "_types2.hpp"
#include "doc.hpp"

/****************************************************************************/
/****************************************************************************/

enum ParaWinCmd
{
  CMD_PARA_CHANGE = 0x1000,
  CMD_PARA_RELAYOUT,

  CMD_PARA_FILEREQ = 0x2000,
  CMD_PARA_FOLLOWLINK = 0x2100,
  CMD_PARA_PAGELIST = 0x2200,
  CMD_PARA_OPLIST = 0x2300,
  CMD_PARA_PAGENUM = 0x2400,
  CMD_PARA_STORENAME= 0x2500,

};

class ParaWin_ : public sDummyFrame
{
  sGridFrame *Grid;
  GenOp *EditOp;
  sInt Line;
  sInt Row;
  sInt RowStart;
  void Advance(GenPara *para);
  void OpList(GenPage *page);

  sInt StoreNameCount;
  GenName StoreNames[256];
  sInt LinkParaId;

  sToolBorder *Tools;
public:
  ParaWin_();
  ~ParaWin_();

  void SetOp(GenOp *);
  GenOp *GetOp() { return EditOp; }
  sBool OnCommand(sU32 cmd);

  void AddBox(const sChar *label,sInt pos,sInt command);
};


/****************************************************************************/
/****************************************************************************/
