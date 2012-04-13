// This file is distributed under a BSD license. See LICENSE.txt for details.

#pragma once
#include "_types.hpp"
#include "_gui.hpp"

namespace Types2 {
;


/****************************************************************************/

template <class ValueType,class KeyType> class sMap   // simple map, without hashing. efficient for SMALL numbers of entries!
{
  struct entry
  {
    ValueType Value;
    KeyType Key;
  };
  sArray2<entry> Array;

  entry *GetEntry(const KeyType &key)                 { entry *e; sFORLIST(&Array,e){if(e->Key==key) return e;} return 0; }

public:
  ValueType *Get(const KeyType &key)                  { entry *e=GetEntry(key); return e ? &e->Value : 0; }
  void Set(const KeyType &key,const ValueType &val)   { entry *e=GetEntry(key); if(!e){e=Array.Add(); e->Key=key;} e->Value=val; }
  sBool IsIn(const KeyType &key)                      { entry *e=GetEntry(key); return e!=0; }
  sBool Rem(const KeyType &key)                       { entry *e; sFORLIST(&Array,e){if(e->Key==key){Array[_i]=Array[--Array.Count]; return 1;}} return 0;}

  const ValueType *GetFOR(sInt i)                     { return &Array.Get(i)->Value; }
  sInt GetCount() const                               { return Array.Count; }
};

/****************************************************************************/
/***                                                                      ***/
/***   User Interface Extensions                                          ***/
/***                                                                      ***/
/****************************************************************************/

enum sGuiMenuListKind
{
  sGML_END = 0,                   // end of list
  sGML_COMMAND,                   // send command when key is pressed
  sGML_CHECKMARK,                 // display checkmark and send command when pressed
  sGML_SPACER,                    // add a spacer in menu
};

struct sGuiMenuList
{
  sInt Kind;                      // sGML_???
  sU32 Shortcut;                  // sKEY_??? shortcut
  sU32 Command;                   // cmd-id
  sDInt Offset;                   // offset for checkmark (to this ptr, in bytes, to sInt)
  sChar *Name;                    // Text for menu
};

class sGuiWindow2 : public sGuiWindow
{
  sGuiMenuList *MenuList;
public:
  void SetMenuList(sGuiMenuList *);         // initialize the auto-menu
  sBool MenuListKey(sU32 key);              // keyboard-processing for menu
  void PopupMenuList(sBool popup = 0);                     // popup menu as specified by list

};

/****************************************************************************/
/****************************************************************************/

}; // namespace

using namespace Types2;
