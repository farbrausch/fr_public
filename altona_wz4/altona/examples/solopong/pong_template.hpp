/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_SOLOPONG_PONG_TEMPLATE_HPP
#define FILE_SOLOPONG_PONG_TEMPLATE_HPP

#include "base/types.hpp"
#include "main.hpp"

/****************************************************************************/

class SoloPong
{
  sInt FlashTimer;
public:
  SoloPong();
  ~SoloPong();

  void OnTick();
  void OnPaint();

  sString<256> ScoreString;
};

/****************************************************************************/

#endif // FILE_SOLOPONG_PONG_TEMPLATE_HPP

