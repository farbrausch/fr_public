/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_BASE_DEVICES_HPP
#define FILE_BASE_DEVICES_HPP

#include "base/types2.hpp"

/****************************************************************************/

void sAddMidi(sBool logging=0);


struct sMidiEvent
{
  sU8 Device;
  sU8 Status;
  sU8 Value1;
  sU8 Value2;
  sU32 TimeStamp;
};


class sMidiHandler_
{
public:
  virtual ~sMidiHandler_() {}
  virtual sBool HasInput()=0;
  virtual sBool GetInput(sMidiEvent &e)=0;
  virtual void Output(sU8 dev,sU8 stat,sU8 val1,sU8 val2)=0;
  virtual const sChar *GetDeviceName(sBool out,sInt dev)=0;

  void Output(sMidiEvent &e) { Output(e.Device,e.Status,e.Value1,e.Value2); }
  sMessage InputMsg;
};

extern sMidiHandler_ *sMidiHandler;


/****************************************************************************/

#endif // FILE_BASE_DEVICES_HPP

