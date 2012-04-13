// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __WINPARA_HPP__
#define __WINPARA_HPP__

#include "_gui.hpp"
#include "werkkzeug.hpp"
#include "kkriegergame.hpp"

/****************************************************************************/

#define CMD_PARA_CHANGE       0x0101    // parameter has changed
#define CMD_PARA_SPLINE       0x0102    // toggle spline mode
#define CMD_PARA_CHANGEX      0x0103    // parameter has changed, page also
#define CMD_PARA_UNDO         0x0104    // undo all operator parameters
#define CMD_PARA_REDO         0x0105    // undo all operator parameters
#define CMD_PARA_ANIMOP       0x0106    // changed something in animation
#define CMD_PARA_SPLINEADD    0x0107    // automatically add animop spline
#define CMD_PARA_SPLINESELECT 0x0108    // select animop spline from page
#define CMD_PARA_SPLINEGOTO   0x0109    // goto animop spline editor
#define CMD_PARA_SPLINEADD2   0x010a    // automatically add animop spline
#define CMD_PARA_ANIMLINEAR   0x010b
#define CMD_PARA_ANIMOLD      0x010c
#define CMD_PARA_ANIMNEW      0x010d
#define CMD_PARA_ANIMNEW2     0x010e
#define CMD_PARA_RELOAD       0x010f    // delete blob
#define CMD_PARA_SETOP        0x0110
#define CMD_PARA_DONEX        0x0111    // like changex, but with relayout
#define CMD_SPLINE_SORT       0x0112
#define CMD_SPLINE_CHANGEMODE 0x0113
#define CMD_SPLINE_SWAPTC     0x0114
#define CMD_SPLINE_SETTARGET  0x0115
#define CMD_SPLINE_NORM       0x0116

#define CMD_PARA_PAGES        0x5100    // "..."
#define CMD_PARA_NAMES        0x5200    // ".."
#define CMD_PARA_GOTO         0x5300    // "->"
#define CMD_PARA_ANIM         0x5400    // "A"
#define CMD_PARA_PAGE         0x5500    // selected from page-list after "..."
#define CMD_PARA_OP           0x5600    // selected from op-list after ".."
#define CMD_PARA_CHCH         0x5800    // change channel
#define CMD_PARA_PAGES2       0x5900    // "..."
#define CMD_PARA_SPECIAL      0x5a00    // "auto-combiner", ...
#define CMD_PARA_STRING       0x5b00    // string 0..7 changed
#define CMD_PARA_FILTER       0x5c00    // ".." filter
#define CMD_PARA_SPLINESEL2   0x5d00    // ".." spline
#define CMD_PARA_TEXT         0x5e00    // string editing
#define CMD_PARA_CHCHX        0x5f00    // change channel
#define CMD_SPLINE_ADD       0x15000
#define CMD_SPLINE_REM       0x15100
#define CMD_SPLINE_SELECT    0x15200
#define CMD_PARA_FILEREQ     0x15300    // edit string with file req
#define CMD_PIPE_ADD         0x15400
#define CMD_PIPE_REM         0x15500

/****************************************************************************/

struct WinParaInfo
{
  sInt Format;                    // sKAF_???
  sInt Channels;                  // dimensions
  sInt Offset;                    // offset, in words (*4)
  sInt Animatable;                // 1 = KAF_STORE; 0 = KAF_CHANGE
  sInt ReSetOp;                   // call SeTOp(Op) on change
  sInt ReConnectOp;               // reconnect graph on change
  const sChar *Name;                    // name of parameter
  sInt SplineKind;                // WSK_???
  void Init(sInt f,sInt c,const sChar *n,sInt o);
};

#define KK_MAXUNDODATA 1024

class WinPara : public sDummyFrame
{
  sInt Line;
  sInt Offset;
//  sInt Spline;
  WerkPage *CurrentPage;
  sChar StringEditBuffer[8][2048];
  class sTextControl *TextControl[8];
  sChar AnimOpBuffer[2048];
  sChar NewSplineName[KK_NAME];
  sInt NewAnimInfo;
  sInt StoreFilter,StoreFilterTemp;

  WerkOp *SetThisOp;
  WerkOpAnim *FindOpAnim();
  WinParaInfo *FindInfo(sInt offset);
  void HandleGroup();

  void AddBlobSplineKey(sInt key);
  void RemBlobSplineKey(sInt key);
  void AddBlobPipeKey(sInt key);
  void RemBlobPipeKey(sInt key);

public:
  class WerkkzeugApp *App;
  WinPara();
  void Reset();
  void Tag();
  sGridFrame *Grid;
  WerkOp *Op;
  sU8 UndoData[KK_MAXUNDODATA];
  sU8 RedoData[KK_MAXUNDODATA];
  sU8 EditData[KK_MAXUNDODATA];
  WinParaInfo Info[64];
  sInt InfoCount;
  sInt GrayLabels;
  const sChar *NextGroup;

  void SetOp(WerkOp *op);
  void SetOpNow(WerkOp *op);
  sBool OnCommand(sU32 cmd);
  void Label(const sChar *label);
  void Space() {Line++;}
  sControl *AddBox(sU32 cmd,sInt pos,sInt offset,sChar *name);
  void AddSpecial(const sChar *name,sInt nr);
  void OnPaint();

  void EditOp(WerkOp *op);
  sControl *EditLink(const sChar *name,sInt offset);
  sControl *EditLinkI(const sChar *name,sInt offset,sInt offsetflags);
  sControl *EditString(const sChar *name,sInt index);
  sControl *EditFileName(const sChar *name,sInt index);
  class sTextControl *EditText(const sChar *name,sInt index);
  sControl *EditSpline(const sChar *name,sInt index);
  sControl *EditInt(const sChar *name,sInt offset,sInt zones,sInt def,sInt min,sInt max,sF32 step=0.25f);
  sControl *EditFloat(const sChar *name,sInt offset,sInt zones,sF32 def,sF32 min,sF32 max,sF32 step=0.01f);
  sControl *EditRGBA(const sChar *name,sInt offset,sU32 def);
  sControl *EditCycle(const sChar *name,sInt offset,sInt def,sChar *cycle);
  sControl *EditBool(const sChar *name,sInt offset,sInt def);
  sControl *EditMask(const sChar *name,sInt offset,sInt def,sInt zones,sChar *picks);
  sControl *EditFlags(const sChar *name,sInt offset,sInt def,sChar *cycle);
  void EditBlobSpline();
  void EditBlobPipe();
  void EditSetGray(sBool gray);
  void EditGroup(const sChar *name);
  void EditSpacer(const sChar *name);

  void DefaultF(sInt offset,sControl *con,sF32 x);
  void DefaultF2(sInt offset,sControl *con,sF32 x,sF32 y);
  void DefaultF3(sInt offset,sControl *con,sF32 x,sF32 y,sF32 z);

  struct BlobSpline *GetBlobSpline();
  struct BlobPipe *GetBlobPipe();
};

/****************************************************************************/

#define CMD_ANIMPAGE_CUT        0x1000
#define CMD_ANIMPAGE_COPY       0x1001
#define CMD_ANIMPAGE_PASTE      0x1002
#define CMD_ANIMPAGE_DELETE     0x1003
#define CMD_ANIMPAGE_GENCODE    0x1004
#define CMD_ANIMPAGE_ADD        0x1100
#define CMD_ANIMPAGE_OP         0x00010000

class WinAnimPage : public sOpWindow
{
  WerkOp *Op;
  sInt AddColumn;
  sList<WerkOpAnim> *Clipboard;
public:
  class WerkkzeugApp *App;

  WinAnimPage();
  ~WinAnimPage();
  void OnKey(sU32 key);
  void OnPaint();
  sBool OnCommand(sU32 cmd);
  void WinAnimPage::Tag();
  void SetOp(WerkOp *op);

  void PopupAdd();
  void PopupMenu();
  void AddOp(sU32 op);
  void DelOp();
  void CopyOp();
  void PasteOp();
  void CheckOp();
  void DeselectOp();

  sInt GetOpCount();
  void GetOpInfo(sInt i,sOpInfo &);
  void SelectRect(sRect cells,sInt mode);
  void MoveDest(sBool dup);  

  void AddAnimOps(sChar *splinename,sInt offset);
  //  sBool Connect();
//  void GenCode(WerkOpAnim *);
};

/****************************************************************************/

#endif
