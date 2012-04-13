// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef __WINPROJECT_HPP__
#define __WINPROJECT_HPP__

#include "_util.hpp"
#include "_gui.hpp"
#include "apptool.hpp"
#include "apptext.hpp"


/****************************************************************************/
/***                                                                      ***/
/***   windows                                                            ***/
/***                                                                      ***/
/****************************************************************************/

class ProjectWin : public ToolWindow
{
  sListControl *DocList;
public:
  ProjectWin();
  ~ProjectWin();
  sU32 GetClass() { return sCID_TOOL_PROJECTWIN; }
  void OnInit();
  void OnCalcSize();
  void OnLayout();
  void OnPaint() {}
  sBool OnCommand(sU32 cmd);
  void UpdateList();
};

/****************************************************************************/

class ObjectWin : public ToolWindow
{
  sListControl *ObjectList;
public:
  ObjectWin();
  ~ObjectWin();
  sU32 GetClass() { return sCID_TOOL_OBJECTWIN; }
  void OnInit();
  void OnCalcSize();
  void OnLayout();
  void OnPaint() {}
  sBool OnCommand(sU32 cmd);
  void UpdateList();
  void SetDoc(ToolDoc *) { UpdateList(); }
};


/****************************************************************************/

class EditDoc : public ToolDoc
{
public:
  EditDoc();
  ~EditDoc();
  void Clear();
  void Tag();
  sU32 GetClass() {return sCID_TOOL_EDITDOC;}
  sBool Write(sU32 *&);
  sBool Read(sU32 *&);


  void Update(ToolObject *);     // update object in this doc

  sChar *Text;
  sInt Alloc;
};

class EditWin : public ToolWindow
{
  sTextControl *TextControl;
  EditDoc *Doc;
public:
  EditWin();
  ~EditWin();
  void Tag();
  sU32 GetClass() { return sCID_TOOL_EDITWIN; }
  void OnInit();
  void OnCalcSize();
  void OnLayout();
  void OnPaint() {}
  sBool OnCommand(sU32 cmd);
  void SetDoc(ToolDoc *);
  void SetLine(sInt line) {TextControl->SetCursor(0,line);}
  void SetCursor(sInt x,sInt y) {TextControl->SetCursor(x,y);}
  void SetSelection(sInt cursor,sInt sel) {TextControl->SetSelection(cursor,sel); }
};


/****************************************************************************/

class PerfMonWin : public ToolWindow
{
  sInt Mode;
public:
  PerfMonWin();
  ~PerfMonWin();
  sU32 GetClass() { return sCID_TOOL_PERFMONWIN; }
//  void OnInit();
//  void OnCalcSize();
//  void OnLayout();
  void OnPaint();
  void OnKey(sU32 key);
//  sBool OnCommand(sU32 cmd);
};

/****************************************************************************/

#endif
