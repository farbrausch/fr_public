/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_TOOLS_FONTTOOL_DOC_HPP
#define FILE_ALTONA2_TOOLS_FONTTOOL_DOC_HPP

#include "altona2/libs/base/base.hpp"
#include "altona2/libs/util/image.hpp"

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class wDocument
{
public:
  wDocument();
  ~wDocument();

  void Update();
};

extern wDocument *Doc;

/****************************************************************************/

#endif  // FILE_ALTONA2_TOOLS_FONTTOOL_DOC_HPP

