/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_GUI_FRAMES_HPP
#define FILE_ALTONA2_LIBS_GUI_FRAMES_HPP

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Gui/Window.hpp"
#include "Altona2/Libs/Gui/Style.hpp"
#include "Altona2/Libs/Gui/Manager.hpp"
#include "Altona2/Libs/Gui/Buttons.hpp"

namespace Altona2 {

class sTextWindow;
class sTextControl;

/****************************************************************************/
/***                                                                      ***/
/***   Just pass through single child                                     ***/
/***                                                                      ***/
/****************************************************************************/

class sPassthroughFrame : public sWindow
{
public:
    void OnCalcSize();
    void OnLayoutChilds();
    sPassthroughFrame() {}
    sPassthroughFrame(sWindow *w) { AddChild(w); }
};


class sPassthroughFrame2 : public sWindow
{
public:
    void OnCalcSize();
    void OnLayoutChilds();
    sPassthroughFrame2() {}
    sPassthroughFrame2(sWindow *w) { AddChild(w); }
};


class sKillMeFrame : public sPassthroughFrame
{
public:
    sKillMeFrame();
    sKillMeFrame(sWindow *c);
    void OnPaint(int layer);
    sWindow *FocusAfterKill;
    void CmdKillAndFocus();
};

/****************************************************************************/
/***                                                                      ***/
/***   Box Layout (h/v)                                                   ***/
/***                                                                      ***/
/****************************************************************************/

struct sBoxWindowInfo
{
    int Width;
    int Weight;
    int p0,p1;
};

class sBoxFrame : public sWindow
{
protected:
    sArray<sBoxWindowInfo> Infos;
    void UpdateInfo();
    void LayoutInfo(int width);
    int KnopSize;
    sBoxFrame();
public:
    void SetWeight(int n,int weight);
};

class sVBoxFrame : public sBoxFrame
{
public:
    sVBoxFrame();
    void OnCalcSize();
    void OnLayoutChilds();
};

class sHBoxFrame : public sBoxFrame
{
public:
    sHBoxFrame();
    void OnCalcSize();
    void OnLayoutChilds();
};

/****************************************************************************/
/***                                                                      ***/
/***   Splitters (h/v)                                                    ***/
/***                                                                      ***/
/****************************************************************************/

struct sSplitterWindowInfo
{
    int Preset;
    int Pos;
    int p0,p1;
    sRect HitBox;
    int RestPos;
};

class sSplitterFrame : public sWindow
{
    int FirstLayout;
    int OldWidth;
protected:
    sArray<sSplitterWindowInfo> Infos;
    void UpdateInfo();
    void LayoutInfo(int width);
    void RestInfos(int width);
    void LimitSplitter(int width,int n=0);
    int KnopSize;
    int DragKnop;
    int DragStart;
    int RestWidth;
    sSplitterFrame();
    void OnInit();
public:
    void SetSize(int n,int s);
};

class sVSplitterFrame : public sSplitterFrame
{
public:
    sVSplitterFrame();
    void OnCalcSize();
    void OnLayoutChilds();
    void OnPaint(int layer);
    void Drag(const sDragData &dd);
};

class sHSplitterFrame : public sSplitterFrame
{
public:
    sHSplitterFrame();
    void OnCalcSize();
    void OnLayoutChilds();
    void OnPaint(int layer);
    void Drag(const sDragData &dd);
};

/****************************************************************************/
/***                                                                      ***/
/***   Switch                                                             ***/
/***                                                                      ***/
/****************************************************************************/

class sSwitchFrame : public sWindow
{
    int Switch;
public:
    sSwitchFrame();
    void OnCalcSize();
    void OnLayoutChilds();

    void SetSwitch(int n);
    int GetSwitch() { return Switch; }
};


/****************************************************************************/
/***                                                                      ***/
/***   Grid                                                               ***/
/***                                                                      ***/
/****************************************************************************/

struct sGridFrameItem
{
    int PosX;
    int PosY;
    int SizeX;
    int SizeY;

    sWindow *Window;                // either a window

    const char *Label;             // or a label
    int LabelLen;
    int LabelFlags;
    int Group;                     // 0 - no group, 1..n - group with emphasis
};

class sGridFrame : public sWindow
{
    sArray<sGridFrameItem> Items;
    void GetRect(sGridFrameItem *it,sRect &r);
    sGridFrameItem *AddItem(int x,int y,int sx,int sy);
    sMemoryPool *Pool;
public:
    sGridFrame();
    ~sGridFrame();
    void OnInit();
    void OnCalcSize();
    void OnLayoutChilds();
    void OnPaint(int layer);

    void AddLabel(int x,int y,int sx,int sy,const char *label,int len=-1,int flags=sPPF_Right|sPPF_Space,int group=0);
    void AddWindow(int x,int y,int sx,int sy,sWindow *w);
    void Clear();
    int GridX;
    int GridY;
    int Height;

    const char *AllocTempString(const char *);  // temp strings until clear is called..
};

class sGridFrameHelper
{
    bool LineIsEmpty;
    int Line;                      // current line
    int Pos0;                      // left cursor
    int Pos1;                      // right cursor

    bool LinkMode;
    sControl *FirstLink;
    sControl *CurrentLink;
    template <typename T,typename TF>
    void LinkControl(sValueControl<T,TF> *c)
    {
        if(LinkMode)
        {
            if(FirstLink==0)
                FirstLink=c;
            c->Link = static_cast<sValueControl<T,TF> *>(CurrentLink);
            CurrentLink = c;
            static_cast<sValueControl<T,TF> *>(FirstLink)->Link = static_cast<sValueControl<T,TF> *>(CurrentLink);
        }
    }
public:
    int GetLine() { return Line; }

    void AddLeft(sWindow *w,int width,int lines=1);
    void AddRight(sWindow *w,int width);
    void AddLeft(sControl *w,int width,int lines=1);
    void AddRight(sControl *w,int width);
    void AddLines(sWindow *w,int lines);

    // geometry

    int Columns;                   // total width (12)
    int Left;                      // left border for buttons (3)
    int Right;                     // right border for buttons (11)
    int Small;                     // width of a small button (1)
    int Normal;                    // width of a normal button (2)
    int Wide;                      // width of a wide button (4)
    int OverrideWidth;             // width for next item
    sUpdatePool *UpdatePool;
    sGuiMsg ChangeMsg;
    sGuiMsg DoneMsg;
    sGridFrame *Grid;

    // class

    sGridFrameHelper(sGridFrame *);
    ~sGridFrameHelper();

    // control functions

    void RestartLine() { Pos0 = Left; Pos1 = Right = Columns = Grid->GridX; }
    void NextLine();
    void ForceNextLine();
    void FullWidth();
    void ExtendRight();
    void Label(const char *str);
    void Label(const char *str,int flags);
    void Group(const char *str,int priority=1);
    void Space(int n);

    void BeginLink();
    void EndLink();

    // button functions

    sButtonControl *Box(sGuiMsg msg,const char *name);
    void EmptyBox() { Pos1--; }
    sButtonControl *Button(sGuiMsg msg,const char *name);
    sButtonControl *Toggle(sGuiMsg msg,int &ref,int value,const char *name,int mask=~0);
    sButtonControl *Radio(sGuiMsg msg,int &ref,int value,const char *name,int mask=~0);
    sChoiceControl *Choice(int &ref,const char *choices);
    sButtonControl *ConstString(const char *);
    sStringControl *String(const sStringDesc &desc);
    sPoolStringControl *String(sPoolString &p);
    sTextWindow *Text(sTextBuffer &text,int lines=5);
    sTextControl *Text(const sStringDesc &desc,int lines=5);
    sTextControl *Text(sPoolString &pool,int lines=5);

    sIntControl *Int(int &ref,int min,int max,float step=0,int def=0,float step2=0.25f);
    sU32Control *Int(uint &ref,uint min,uint max,float step=0,uint def=0,float step2=0.25f);
    sFloatControl *Float(float &ref,float min,float max,float step=0,float def=0,float step2=0.25f);
    sF64Control *F64(double &ref,double min,double max,float step=0,double def=0,float step2=0.25f);
    sS64Control *S64(int64 &ref,int64 min,int64 max,float step=0,int64 def=0,float step2=0.25f);

    void Color(uint &col,uint def=0,bool alpha=1);
    void Color(sVector4 &col);
    void Color(sVector4 &col,const sVector4 &def);
    void Color(sVector3 &col,const sVector4 &def);

    template<typename T> sChoiceControl *Choice(T &ref,const char *choices)
    { return Choice((int &)ref,choices); }
};

/****************************************************************************/
/***                                                                      ***/
/***   Overlapping Window                                                 ***/
/***                                                                      ***/
/****************************************************************************/

class sOverlappedFrame : public sWindow
{
public:
    sOverlappedFrame();

    void OnLayoutChilds();
};

/****************************************************************************/
/***                                                                      ***/
/***   Menu                                                               ***/
/***                                                                      ***/
/****************************************************************************/

enum sMenuFrameItemKind
{
    sMIK_Error = 0,
    sMIK_Push,
    sMIK_Check,
    sMIK_Spacer,
    sMIK_Header,
    sMIK_Grid,


    sMI_MaxColumns = 32,
};

class sMenuFrameItem
{
    friend class sMenuFrame;
    sRect Client;
    const char *KeyString;
    sWindow *Window;

    int Kind;
    sGuiMsg Msg;
    uint Key;
public:

    int Column;

    sMenuFrameItem();
    ~sMenuFrameItem();
    sWindow *GetWindow() { return Window; }
};

/****************************************************************************/

class sMenuFrame : public sWindow
{
    sArray<sMenuFrameItem *> Items;

    int sx[sMI_MaxColumns];
    int sy[sMI_MaxColumns];
    int x0[sMI_MaxColumns];
    int x1[sMI_MaxColumns];
    uint colmask;
    int DefaultColumn;

    void CmdPress(int n);
    void CmdPressNoKill(int n);
public:
    sMenuFrame(sWindow *master=0);
    ~sMenuFrame();

    sMenuFrameItem *AddSpacer();
    sMenuFrameItem *AddHeader(int column,const char *text,uptr len=-1);
    sMenuFrameItem *AddHeaderItem(uint key,const char *text,uptr len=-1);
    sMenuFrameItem *AddItem(const sGuiMsg &msg,uint key,const char *text,uptr len=-1,const char *prefix=0);
    sMenuFrameItem *AddToggle(int *ref,int val,int mask,const sGuiMsg &msg,uint key,const char *text,uptr len=-1);
    sMenuFrameItem *AddRadio(int *ref,int val,int mask,const sGuiMsg &msg,uint key,const char *text,uptr len=-1);
    sMenuFrameItem *AddGrid(sGridFrame *grid);
    void SetDefaultColumn(int n) { DefaultColumn = n; };
    void StartMenu(bool up=0);
    void StartContext();

    void OnCalcSize();
    void OnLayoutChilds();
    void OnPaint(int layer);

    sUpdatePool *UpdatePool;
};

/****************************************************************************/

class sMenuFrameSpacer : public sWindow
{
public:
    void OnCalcSize();
    void OnPaint(int layer);   
};

/****************************************************************************/

class sMenuFrameHeader : public sWindow
{
    const char *Label;
    uptr LabelLen;
public:
    sMenuFrameHeader(const char *label,uptr len=-1);

    void OnCalcSize();
    void OnPaint(int layer);   
};

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/
}
#endif  // FILE_ALTONA2_LIBS_GUI_FRAMES_HPP

