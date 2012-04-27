/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_LAYOUT_RESOURCES_HPP
#define FILE_LAYOUT_RESOURCES_HPP

#include "base/types2.hpp"
#include "base/windows.hpp"

/****************************************************************************/

enum sResourceFontFamily
{
  sRFF_PROP = 0,
  sRFF_FIXED = 1,
  sRFF_SYMBOL = 2,
  sRFF_MAX = 16
};

enum sResourceFontSize
{
  sRFS_SMALL2 = 0,
  sRFS_SMALL1 = 1,
  sRFS_NORMAL = 2,
  sRFS_BIG1 = 3,
  sRFS_BIG2 = 4,
  sRFS_BIG3 = 5,
  sRFS_BIG4 = 6,
  sRFS_MAX = 7
};

/****************************************************************************/

struct sFontFamily
{
public:
  sPoolString Name;
  sInt Sizes[sRFS_MAX];
};

class sResourceManager_
{
  friend class sFontResource;
  friend class sImageResource;
  friend class sTextResource;

  sArray<class sFontResource *> Fonts;
  sArray<class sImageResource *> Images;
  sArray<class sTextResource *> Texts;
  sFontFamily FontFamilies[sRFF_MAX];
public:
  sResourceManager_();
  ~sResourceManager_();
  void Flush();               // resources are not deleted immediatly, so please flush from time to time
  void ClearTemp();           // clears temp flag
  
  sFontResource *NewFont(const sChar *name,sInt size,sInt style=0);
  sFontResource *NewLogFont(sInt family,sInt logsize,sInt style=0);
  void SetLogFont(sInt family,const sChar *name,sInt basesize);

  sImageResource *NewImage(const sChar *name);
  sTextResource *NewText(const sChar *name);
};

extern sResourceManager_ *sResourceManager;

void sAddResourceManager();

/****************************************************************************/

class sFontResource
{
  friend class sResourceManager_;
  sInt RefCount;
public:
  sFontResource() { RefCount = 1; sResourceManager->Fonts.AddTail(this); Font=0; }
  ~sFontResource() { sResourceManager->Fonts.Rem(this); delete Font; }
  void AddRef() { RefCount++; }
  void Release() { sVERIFY(RefCount>0); RefCount--; /*if(RefCount==0) delete this;*//* do not delete! */}
  
  sFont2D *Font;

  sPoolString Name;
  sInt Size;           // this is the height of the font when as REQUESTED
  sInt Height;          // this is the actual height, that might differ a little bit.
  sInt Style;
  sInt LogSize;

  sDInt Temp;          // used by PDF write to cache font id's
};

class sImageResource
{
  friend class sResourceManager_;
  sInt RefCount;
public:
  sImageResource() { RefCount = 1; sResourceManager->Images.AddTail(this); Image=0; }
  ~sImageResource() { sResourceManager->Images.Rem(this); delete Image; }
  void AddRef() { RefCount++; }
  void Release() { sVERIFY(RefCount>0); RefCount--; /*if(RefCount==0) delete this;*//* do not delete! */ }

  sPoolString Name;
  sImage2D *Image;

  sDInt Temp;          // free for user
};

class sTextResource
{
  friend class sResourceManager_;
  sInt RefCount;
public:
  sTextResource() { RefCount = 1; sResourceManager->Texts.AddTail(this); Text=0; }
  ~sTextResource() { sResourceManager->Texts.Rem(this); delete Text; }
  void AddRef() { RefCount++; }
  void Release() { sVERIFY(RefCount>0); RefCount--; /*if(RefCount==0) delete this;*//* do not delete! */ }

  sPoolString Name;
  const sChar *Text;
  sDInt Temp;          // free for user
};

/****************************************************************************/

#endif // FILE_LAYOUT_RESOURCES_HPP

