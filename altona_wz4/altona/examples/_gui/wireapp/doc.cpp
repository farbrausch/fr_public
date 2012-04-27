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
 
#define sPEDANTIC_OBSOLETE 1
//#define sPEDANTIC_WARN 1

#include "doc.hpp"
#include "base/system.hpp"

Document *Doc;

/****************************************************************************/

Document::Document()
{
  Doc = this;
  Filename = L"c:/temp/test.bla";
}

Document::~Document()
{
}

void Document::Tag()
{
}

/****************************************************************************/
 
void Document::New()
{
}

sBool Document::Save()
{
  return sSaveObject(Filename,this);
}

sBool Document::Load()
{
  return sLoadObject(Filename,this);
}


template <class streamer> void Document::Serialize_(streamer &s)
{
  s.Header(sSerId::Error,1);

  // [..]

  s.Footer();

  if(s.IsOk() && s.IsReading())
    New();
}

void Document::Serialize(sReader &stream)
{
  Serialize_(stream);
}

void Document::Serialize(sWriter &stream)
{
  Serialize_(stream);
}

/****************************************************************************/

