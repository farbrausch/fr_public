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

#pragma once
#include "base/types2.hpp"
#include "base/serialize.hpp"

/****************************************************************************/

class Document : public sObject
{
public:
  sCLASSNAME(Document);
  Document();
  ~Document();
  void Tag();
   
  sString<sMAXPATH> Filename;
  void New();
  sBool Load();
  sBool Save();

  template <class streamer> void Serialize_(streamer &);
  void Serialize(sReader &stream);
  void Serialize(sWriter &stream);
};

extern Document *Doc;

/****************************************************************************/

