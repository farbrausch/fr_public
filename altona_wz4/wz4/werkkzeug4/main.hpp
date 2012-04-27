/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WERKKZEUG4_MAIN_HPP
#define FILE_WERKKZEUG4_MAIN_HPP

#include "base/types.hpp"
#include "wz4lib/gui.hpp"

/****************************************************************************/

class Wz4FrMainWindow : public MainWindow
{
protected:
  void ExtendWireRegister();
  void ExtendWireProcess();
public:
  void CmdCustomEditor();
  void CmdCustomEditor2();
};

/****************************************************************************/

#endif // FILE_WERKKZEUG4_MAIN_HPP

