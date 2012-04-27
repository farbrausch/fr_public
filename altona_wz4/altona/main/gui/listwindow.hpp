/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_GUI_LISTWINDOW_HPP
#define FILE_GUI_LISTWINDOW_HPP

#include "gui/window.hpp"
#include "gui/manager.hpp"
#include "gui/wire.hpp"

class sStaticListWindow;

/****************************************************************************/

#define sLW_MAXTREENEST   128     // max nesting of trees

enum sListWindow2Flags
{
  sLWF_RIGHTPICK = 0x0001,        // if nothing is selected and you press the RMB, the entry will be selected. usefull with context menus
};

/*
enum sListWindow2Type
{
  sLWT_CUSTOM = 0,
  sLWT_INT,
  sLWT_FLOAT,
  sLWT_STRING,
  sLWT_POOLSTRING,
//  sLWT_CONSTSTRING,
//  sLWT_TEXTBUFFER,
  sLWT_CHOICE,
  sLWT_COLOR,
  sLWT_PROGRESS,
};
*/
enum sListWindow2FieldFlags
{
  sLWF_EDIT = 0x0001,             // can be edited
  sLWF_SORT = 0x0002,             // can be sorted
};

class sListWindow2Field          // field: information that can potentially be displayed by the list
{
public:
  sInt Id;
  sVoidMemberPtr Ref;

  sInt Flags;
  sInt Size;
  const sChar *Choices;
  const sChar *Label;

  sInt SortDir;                   // 0 = up, 1 = down;
  sInt SortOrder;                 // 0 = off, 1 = primary, 2 = secondary, ..

  sListWindow2Field() { Id=-1; Flags=0; Size=0; Choices=0; Label=0; SortDir=0; SortOrder=0; }
  virtual void PaintField(sStaticListWindow *lw,const sRect &client,sObject *obj,sInt line,sInt select);
  virtual void EditField(sStaticListWindow *lw,const sRect &rect,sObject *obj,sInt line);
  virtual sInt CompareField(sStaticListWindow *lw,sObject *o0,sObject *o1,sInt line0,sInt line1);
};

struct sListWindow2Column         // column: real column in the table, referencing a field
{
  sListWindow2Field *Field;
  sInt Pos;
  sRect SortBox;                  // hitbox for sort-button
};

template <class Type>             // use ptr type!
struct sListWindowTreeInfo
{
  sInt Level;                     // level of indention, starting with 0
  sInt Flags;                     // 1: show children 0: hide children
  Type Parent;                    // link to parent, if not root (there may be multiple roots)
  Type FirstChild;                // link to first children
  Type NextSibling;               // link to next sibling
  Type *TempPtr;                  // for building single linked list
 
  sListWindowTreeInfo() { Level=0; Flags=0; Parent=0; FirstChild=0; NextSibling=0; TempPtr=0; }
  void Need() { Parent->Need(); FirstChild->Need(); NextSibling->Need(); }
};
enum sListWindowTreeInfoFlags
{
  sLWTI_CLOSED = 0x0001,
  sLWTI_HIDDEN = 0x0002,
};

class sStaticListWindow : public sWireClientWindow    // static list: can't select, only display and edit columns
{
  friend class sListWindow2Header;
  sArray<sListWindow2Field *> Fields;
  sArray<sListWindow2Column> Columns;
  sArray<sU8> SelectionCopy;
  sInt Height;
  sInt DragHighlight;
  sInt DragMode;
  sInt DragStart,DragEnd;
  sInt DragSelectLine;
  sInt DragSelectOther;
  sInt Width;
  sInt IndentPixels;

  void Construct();
protected:
  sArray<sObject *> *Array;
  virtual sListWindowTreeInfo<sObject *> *GetTreeInfo(sObject *obj) { return 0; }
  sBool IsMulti;
public:
  virtual sBool GetSelectStatus(sInt n) { return 0; }
  virtual sInt GetLastSelect() { return 0; }
  virtual void SetSelectStatus(sInt n,sInt select) {}
  virtual void ClearSelectStatus() {}
  virtual void CheckSelectStatus() {}

  sMessage SelectMsg;       // send when selection has changed
  sMessage ClickMsg;       // send when selection was made, even if it did not change
  sMessage DoneMsg;         // send while editing items
  sMessage ChangeMsg;       // send while editing items
  sMessage OrderMsg;        // send when order of item is changed. also, send when delete
  sInt ListFlags;           // sLWF_???
  sInt BackColor;           // updated every line. use for painting. default will contain selection-highlight
  sInt TagArray;            // call sNeed(*Array);

  sWindow *ChoiceDropper;     // just to keep the object alive

  template <class T> sStaticListWindow(sArray<T *> *a) { Array = (sArray<sObject *> *) a; Construct(); }
  ~sStaticListWindow();
  void Tag();
  void InitWire(const sChar *name);
  void AddHeader();
  template <class T> void SetArray(sArray<T *> *a) { Array = (sArray<sObject *> *) a; CheckSelectStatus(); Update(); DragMode=0; }


  void OnCalcSize();
  void OnPaint2D();
  void OnDrag(const sWindowDrag &dd);
  void ScrollTo(sInt index);
  void ScrollToItem(sObject *item);

  void Sort(sListWindow2Field *);
  void Sort();

  virtual sInt OnBackColor(sInt line,sInt select,sObject *obj);
  virtual sInt OnCalcFieldHeight(sListWindow2Field *field,sObject *);
  virtual sBool OnPaintField(const sRect &client,sListWindow2Field *field,sObject *,sInt line,sInt select);
  virtual sBool OnEditField(const sRect &client,sListWindow2Field *field,sObject *,sInt line);
  virtual sInt OnCompareField(sListWindow2Field *field,sObject *o0,sObject *o1,sInt line0,sInt line1);
  void PaintField(const sRect &client,sListWindow2Field *field,sObject *,sInt line,sInt select);
  void PaintField(const sRect &client,sInt select,const sChar *text);
  void EditField(const sRect &client,sListWindow2Field *field,sObject *,sInt line);
  sInt CompareField(sListWindow2Field *field,sObject *o0,sObject *o1,sInt line0,sInt line1);

  void AddField(sListWindow2Field *field,sInt width);
  sListWindow2Field *AddField(const sChar *label,sInt flags,sInt width,sInt id,sInt size);    // custom field. set width to add field, too
  sListWindow2Field *AddField(const sChar *label,sInt flags,sInt width,sMemberPtr<sInt> ref,sInt min=0,sInt max=255,sF32 step=0.25f);   // int
  sListWindow2Field *AddField(const sChar *label,sInt flags,sInt width,sMemberPtr<sF32> ref,sF32 min=0,sF32 max=1,sF32 step=0.001f);   // float
  sListWindow2Field *AddFieldColor(const sChar *label,sInt flags,sInt width,sMemberPtr<sU32> ref);   // rgba color
  sListWindow2Field *AddFieldChoice(const sChar *label,sInt flags,sInt width,sMemberPtr<sInt> ref,const sChar *choices);   // choice
  sListWindow2Field *AddFieldProgress(const sChar *label,sInt flags,sInt width,sMemberPtr<sF32> ref); // progress
//  sListWindow2Field *AddField(const sChar *label,sInt flags,sInt width,const sChar *sObject::*ref); // const string
  sListWindow2Field *AddField(const sChar *label,sInt flags,sInt width,sMemberPtr<sChar> ref,sInt size); // string
  sListWindow2Field *AddField(const sChar *label,sInt flags,sInt width,sMemberPtr<sPoolString> ref);   // pool string
  void AddColumn(sListWindow2Field *field,sInt width);                          // add column from field

  template <int n> sListWindow2Field *AddField(const sChar *label,sInt flags,sInt width,sMemberPtr<sString<n> > ref) { return AddField(label,flags,width,sMemberPtr<sChar>(ref.Offset),n); }
  /*
  template <class T> sListWindow2Field *AddField(const sChar *label,sInt flags,sInt width,sInt T::*ref,sInt min=0,sInt max=255,sF32 step=0.25f) { return AddField(label,flags,width,(sInt sObject::*) ref,min,max); }
  template <class T> sListWindow2Field *AddField(const sChar *label,sInt flags,sInt width,sF32 T::*ref,sF32 min=0,sF32 max=1,sF32 step=0.001f) { return AddField(label,flags,width,(sF32 sObject::*) ref,min,max); }
  template <class T> sListWindow2Field *AddFieldColor(const sChar *label,sInt flags,sInt width,sU32 T::*ref) { return AddFieldColor(label,flags,width,(sU32 sObject::*) ref); }
  template <class T> sListWindow2Field *AddFieldChoice(const sChar *label,sInt flags,sInt width,sInt T::*ref,const sChar *choices) { return AddFieldChoice(label,flags,width,(sInt sObject::*) ref,choices); }
  template <class T> sListWindow2Field *AddFieldProgress(const sChar *label,sInt flags,sInt width,sF32 T::*ref) { return AddFieldProgress(label,flags,width,(sF32 sObject::*) ref); }
//  template <class T> sListWindow2Field *AddField(const sChar *label,sInt flags,sInt width,(const sChar *)T::*ref) { return AddField(label,flags,width,((const sChar *)sObject::*) ref,0); }
  template <class T,int n> sListWindow2Field *AddField(const sChar *label,sInt flags,sInt width,sString<n> T::*ref) { return AddField(label,flags,width,(sChar sObject::*) ref,n); } // this cast is totally illegal, but it gives the right result!
  template <class T> sListWindow2Field *AddField(const sChar *label,sInt flags,sInt width,sPoolString T::*ref) { return AddField(label,flags,width,(sPoolString sObject::*) ref); }
*/
  sInt Hit(sInt mx,sInt my,sInt &line,sInt &column,sRect &rect);
  sInt MakeRect(sInt line,sInt column,sRect &rect);
  void AddNew(sObject *Item);
  void OpenFolders(sInt i);
  void UpdateTreeInfo();
  void HitOpenClose(sInt line);
  sInt ForceEditField(sInt line,sInt column);

  void DragSelectSingle(const sWindowDrag &dd);
  void DragSelectMulti(const sWindowDrag &dd,sDInt mode);
  void DragEdit(const sWindowDrag &dd);
  void CmdDelete(sDInt mode=0);
  void CmdSelUp(sDInt mode);
  void CmdSelDown(sDInt mode);
  void CmdSelectAll();
  void CmdMoveUp(sDInt mode);
  void CmdMoveDown(sDInt mode);
  void CmdMoveLeft();
  void CmdMoveRight();
  void CmdOpenClose(sDInt mode);
};

template <class Type> 
class sSingleListWindow : public sStaticListWindow
{
  sInt Selected;
protected:
  sBool GetSelectStatus(sInt n) { return Selected == n; }
  sInt GetLastSelect() { return Selected; }
  void SetSelectStatus(sInt n,sInt select) { if(select && Selected!=n) { Selected = n; Update(); SelectMsg.Post(); } ClickMsg.Post(); }
  void ClearSelectStatus() { Selected = -1; Update(); SelectMsg.Post(); }
  void CheckSelectStatus() { if ((!Array && Selected>=0) || (Array && Selected>=Array->GetCount())) { Selected=-1; SelectMsg.Post(); } }
public:
  sSingleListWindow(sArray<Type *> *a) : sStaticListWindow(a) { Selected = -1; }

  Type *GetSelected() { return Selected>=0 && Selected<Array->GetCount() ? (Type *) (*Array)[Selected] : 0; }
  void SetSelected(sInt n) { if(n==-1) ClearSelectStatus(); else { sVERIFY(n>=0 && n<Array->GetCount()); SetSelectStatus(n,1); } }
  void SetSelected(Type *t) { SetSelected(sFindIndex(*Array,t)); }
};

template <class Type> 
class sMultiListWindow : public sStaticListWindow
{
  sInt Cursor;
  sMemberPtr<sInt> Member;
protected:
public:
  sBool GetSelectStatus(sInt n) { return Member.Ref((Type *)((*Array)[n])); }
  sInt GetLastSelect() { return (Cursor>=0 && Cursor<Array->GetCount()) ? Cursor : -1; }
  void SetSelectStatus(sInt n,sInt select) { if(select) Cursor=n; if(select != Member.Ref((Type *)((*Array)[n]))) { Member.Ref((Type *)((*Array)[n]))=select; Update(); SelectMsg.Post(); } ClickMsg.Post(); }
  void ClearSelectStatus() { sObject *t; sFORALL(*Array,t) Member.Ref((Type *)t) = 0; Update(); SelectMsg.Post(); }

public:
  sMultiListWindow(sArray<Type *> *a,sMemberPtr<sInt> select) 
  : sStaticListWindow(a) { Member=select; Cursor=-1; IsMulti = 1; }
};

template <class Type>
class sSingleTreeWindow : public sSingleListWindow<Type>
{
  sListWindowTreeInfo<Type *> Type::*TreeInfoMember;
protected:
  sListWindowTreeInfo<sObject *> *GetTreeInfo(sObject *obj) 
    { return (sListWindowTreeInfo<sObject *> *)(&(((Type *)obj)->*TreeInfoMember)); }
public:
  sSingleTreeWindow(sArray<Type *> *a,sListWindowTreeInfo<Type *> Type::*treeinfo) 
  : sSingleListWindow<Type>(a) { TreeInfoMember = treeinfo; }
};

template <class Type>
class sMultiTreeWindow : public sMultiListWindow<Type>
{
  sMemberPtr<sListWindowTreeInfo<Type *> > TreeInfoMember;
protected:
  sListWindowTreeInfo<sObject *> *GetTreeInfo(sObject *obj) 
    { return (sListWindowTreeInfo<sObject *> *) &TreeInfoMember.Ref(obj); }
public:
  sMultiTreeWindow(sArray<Type *> *a,sMemberPtr<sInt> select,sMemberPtr<sListWindowTreeInfo<Type *> > treeinfo) 
  : sMultiListWindow<Type>(a,select) { TreeInfoMember = treeinfo; }
};

/****************************************************************************/

class sListWindow2Header : public sWindow    // use as border!
{
  sStaticListWindow *ListWindow;
  sInt Height;
  sInt DragMode;
  sInt DragStart;
  sInt DragAbsolute;
public:
  sListWindow2Header(sStaticListWindow *);
  ~sListWindow2Header();
  void Tag();

  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();
  void OnDrag(const sWindowDrag &dd);
};

/****************************************************************************/

#endif
