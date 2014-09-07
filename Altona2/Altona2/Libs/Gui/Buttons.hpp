/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_GUI_BUTTONS_HPP
#define FILE_ALTONA2_LIBS_GUI_BUTTONS_HPP

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Gui/Window.hpp"
#include "Altona2/Libs/Gui/Style.hpp"
#include "Altona2/Libs/Util/IconManager.hpp"

namespace Altona2 {
class sMenuFrame;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class sDummyWindow : public sWindow
{
public:
    sDummyWindow();
    sDummyWindow(const char *label);
    void OnPaint(int layer);
    const char *Label;
};

class sHSeparatorWindow : public sWindow
{
public:
    sHSeparatorWindow();
    void OnCalcSize();
    void OnPaint(int layer);
};

class sVSeparatorWindow : public sWindow
{
public:
    sVSeparatorWindow();
    void OnCalcSize();
    void OnPaint(int layer);
};

class sControl : public sWindow
{
public:
    sControl();
    sGuiMsg ChangeMsg;
    sGuiMsg DoneMsg;
};

/****************************************************************************/

enum sButtonStyleEnum
{
    sBS_Label = 0,                  // display only
    sBS_Push,                       // just send message
    sBS_Toggle,                     // toggles between two states
    sBS_Radio,                      // radio button
    sBS_Menu,                       // used in menus (no border)
    sBS_MenuToggle,                 // used in menus (no border)
    sBS_MenuRadio,                  // used in menus (no border)
    sBS_ErrorMsg,                   // used in WZ5
};

class sButtonControl : public sControl
{
    int InternalValue;
    bool DragMode;
    bool DragVerify;
    int SimulatePush;
    sButtonStyleEnum ButtonStyle;         // sBS_???
    void Click();
    sIcon Icon;
public:
    int FixedSizeX;                // width of this label, or 0
    const char *Label;             // text to display
    uptr LabelLen;                  // length of text or -1
    sGuiMsg ClickMsg;               // message to send on click

    int *RefPtr;                   // reference for toggle / radio
    int RefMask;                   // bits used in reference
    int RefValue;                  // value for representing this button

    uint Key;                       // shortcut key used in menu styles
    uint BackColor;                 // alternative back-color (or 0 for normal color)

    sGuiStyleKind StyleKind;

    sButtonControl();
    sButtonControl(const sGuiMsg &msg,const char *label,sButtonStyleEnum bs=sBS_Push);
    sButtonControl *SetRef(int *ref,int val,int mask=~0);
    sButtonControl *SetStyle(sButtonStyleEnum bs);
    void OnCalcSize();
    void OnPaint(int layer);

    void DragClick(const sDragData &dd);
    void CmdClick();
    void LoadIcon();
};

/****************************************************************************/

class sBoolToggle : public sControl
{
    const char *Label[2];
    bool *Ref;
public:
    sBoolToggle(bool &ref,const char *on,const char *off=0);

    void OnCalcSize();
    void OnPaint(int layer);
    void CmdClick();

    int FixedSizeX;                // width of this label, or 0
};

/****************************************************************************/

struct sChoiceInfo
{
    const char *Label;             // text for this choice
    uptr LabelLen;                  // length of text or -1
    int Value;                     // value for representing this choice

    const char *IconToolTip;
    int IconToolTipLen;
    sIcon Icon;
};

class sChoiceManager
{
public:
    sArray<sChoiceInfo> Choices;
    bool HasIcons;

    sChoiceManager();
    ~sChoiceManager();
    int SetChoices(const char *str);
    void AddChoice(int value,const char *label,uptr len);
    sChoiceInfo *FindChoice(int val);
    int GetMaxWidth(sFont *f);
    void GetMaxSize(sFont *f,int &sx,int &sy);
    sMenuFrame *CmdClick(const sGuiMsg &msg,sWindow *parent);
};

class sChoiceControl : public sControl
{
    int InternalValue;
    bool DragMode;
    bool DragVerify;
    sChoiceManager Choices;
public:
    sChoiceControl();
    sChoiceControl(int *ref,const char *str);

    void SetChoices(const char *choicestring);
    void AddChoice(int value,const char *label,int len=-1);

    int FixedSizeX;                // width of this label, or 0
    int *RefPtr;                   // reference 
    int RefMask;                   // bits used in reference

    void OnCalcSize();
    void OnPaint(int layer);
    void DragClick(const sDragData &dd);
    void CmdClick();
    void SetValue(int n);
};

/****************************************************************************/

class sStringEditorHelper;
class sStringEditorUndo
{
    enum EditKind
    {
        EK_Nop,
        EK_Insert,
        EK_Delete,
    };

    struct Edit
    {
        EditKind Kind;
        uptr Cursor;
        uptr Len;
        const char *Text;
        Edit() { Kind = EK_Nop; Cursor=0; Len=0; Text=0; }
    };

    sMemoryPool Pool;
    sArray<Edit> Edits;
    int UndoIndex;

    uptr InsertCursor;
    uptr InsertUsed;
    uptr DeleteCursor;
    uptr DeleteUsed;
    sString<sMaxPath> Buffer;

    bool Immune;
public:
    sStringEditorUndo();
    void Insert(uptr Cursor,const char *newtext,uptr len);
    void Delete(uptr Cursor,const char *oldtext,uptr len);
    void Flush();
    void Reset();
    void Undo(sStringEditorHelper *editor);
    void Redo(sStringEditorHelper *editor);
};

class sStringEditorHelper
{
protected:
    sControl *Window;
    uptr UpDownX;
    sStringEditorUndo *OwnUndo;

    void Update();
    void Change();
    bool Left();
    bool Right();
    void Up(int n);
    void Down(int n);
    uptr GetCursorX();
    bool GetSelection(uptr &t0,uptr &t1);
    void Select0(int select);
    void AllocUndo();

public:
    void CmdKey(const sKeyData &kd);
    void CmdTab();
    void CmdUntab();
    void CmdLeft(int select);
    void CmdRight(int select);
    void CmdUp(int select);
    void CmdDown(int select);
    void CmdWordLeft(int select);
    void CmdWordRight(int select);
    void CmdPageUp(int select);
    void CmdPageDown(int select);
    void CmdLineBegin(int select);
    void CmdLineEnd(int select);
    void CmdTextBegin(int select);
    void CmdTextEnd(int select);
    void CmdInsert();
    void CmdDelete();
    void CmdBackspace();
    void CmdEnter();
    void CmdSelectAll();
    void CmdSelectWord();
    void CmdCut();
    void CmdCopy();
    void CmdPaste();
    void CmdDone();
    void CmdUndo();
    void CmdRedo();

    uptr Cursor;
    bool Overwrite;
    bool SelectMode;
    bool HasChanged;
    uptr SelectStart;
    int Tabsize;

    bool Multiline;
    sGuiMsg ValidateMsg;
    sGuiMsg UpdateTargetMsg;
    bool Static;

    sStringEditorHelper();
    ~sStringEditorHelper();
    void Register(sControl *w);
    void ChangeDone();
    void NewText();

    sStringEditorUndo *Undo;
    sGuiMsg DoneMsg;

    virtual const char *GetBuffer() = 0;
    virtual uptr GetBufferUsed() = 0;
    virtual uptr GetBufferSize() = 0;

    virtual void Delete(uptr pos)=0;
    virtual void Delete(uptr start,uptr end)=0;
    virtual bool Insert(uptr pos,int c)=0;
    virtual bool Insert(uptr pos,const char *text,uptr len=~uptr(0))=0;
    virtual void SetCursor(uptr );
    virtual void SetSelect(uptr t0, uptr t1);
    virtual uptr GetCursor() { return Cursor; }
    virtual void ScrollToCursor() {}
    virtual int GetLinesInPage() { return 5; }
};

class sStringControlEditorHelper : public sStringEditorHelper
{
    sStringDesc &Desc;
public:
    sStringControlEditorHelper(sStringDesc &desc);
    const char *GetBuffer() { return Desc.Buffer; }
    uptr GetBufferUsed() { return sGetStringLen(Desc.Buffer); }
    uptr GetBufferSize() { return Desc.Size; }
    void Delete(uptr pos);
    void Delete(uptr start,uptr end);
    bool Insert(uptr pos,int c);
    bool Insert(uptr pos,const char *text,uptr len=~uptr(0));
    void SetCursor(uptr );
};

class sStringControl : public sControl
{
    sStringDesc Desc;
    int StyleScroll;
    bool DragMode;

    void Construct();

    uptr FindClick(int xpos);

protected:
    // for value control 
    sStringControlEditorHelper Editor;
    bool Validated;

public:
    sStringControl();
    sStringControl(const sStringDesc &desc);

    void SetBuffer(const sStringDesc &desc);
    void SetText(const char *);

    void OnCalcSize();
    void OnPaint(int layer);
    void DragClick(const sDragData &dd);
    void OnLeaveFocus();
    void CmdStartEditing();
    void CmdEnter();
    void SetCursor(uptr cursor) { Editor.SetCursor(cursor); }
    uptr GetCursor() { return Editor.GetCursor(); }

    int FixedSizeX;
    uint *BackColorPtr;
    sVector4 *BackColorFloatPtr;
};

class sPoolStringControl : public sStringControl
{
    sString<sFormatBuffer> Buffer;
    sPoolString *PoolString;
    void UpdateTarget();
public:
    sPoolStringControl();
    sPoolStringControl(sPoolString &p);

    void SetBuffer(sPoolString &p);
};

/****************************************************************************/

template <typename T,typename TF>
class sValueControl : public sStringControl
{
    sString<64> Buffer;
    T *Ref;
    T Min,Max,Default;
    T DragStart;
    float Step[2];
public:
    sValueControl<T,TF> *Link;

    sValueControl() : sStringControl(Buffer)
    {
        Step = 0;
        DragStart = 0;
        Link = 0;
        SetRef(0,0,1);
        Editor.ValidateMsg = sGuiMsg(this,&sValueControl<T,TF>::ValueValidate);
    }

    sValueControl(T *ref,T min,T max,T def) : sStringControl(Buffer)
    {
        Step[0] = 0;
        Step[1] = 0;
        DragStart = 0;
        Link = 0;
        SetRef(ref,min,max,def);
        Editor.ValidateMsg = sGuiMsg(this,&sValueControl<T,TF>::ValueValidate);
    }

    void SetDrag(float step,float step2=0.25f)
    {
        Step[0] = step;
        Step[1] = step2;
        AddKey("default",sKEY_LMB|sKEYQ_Double,sGuiMsg(this,&sValueControl<T,TF>::CmdDefault));
        AddDrag("drag",sKEY_RMB,sGuiMsg(this,&sValueControl<T,TF>::Drag,0));
        AddDrag("drag graphics tablet",sKEY_Pen,sGuiMsg(this,&sValueControl<T,TF>::Drag,0));
        AddDrag("drag alternative speed",sKEY_MMB,sGuiMsg(this,&sValueControl<T,TF>::Drag,1));
        if(Link)
        {
            AddKey("default",sKEY_LMB|sKEYQ_Double|sKEYQ_Ctrl,sGuiMsg(this,&sValueControl<T,TF>::CmdDefaultLinked));
            AddDrag("drag linked",sKEY_RMB|sKEYQ_Ctrl,sGuiMsg(this,&sValueControl<T,TF>::DragLinked,0));
            AddDrag("drag linked graphics tablet",sKEY_PenF,sGuiMsg(this,&sValueControl<T,TF>::DragLinked,0));
            AddDrag("drag linked alternative speed",sKEY_MMB|sKEYQ_Ctrl,sGuiMsg(this,&sValueControl<T,TF>::DragLinked,1));
        }
        RemKey(sGuiMsg(this,&sWindow::CmdHelp));
    }

    void SetRef(T *ref,T min,T max,T def)
    {
        Ref = ref;
        Min = min;
        Max = max;
        Default = def;

        if(Ref)
            Buffer.PrintF("%f",*Ref);
        else
            Buffer = "0";

        Editor.NewText();
    }

    void ValueValidate()
    {
        T val;
        const char *s = Buffer;
        Validated = 0;
        if(sScanValue(s,val) && *s==0)
        {
            if(Ref)
                *Ref = val;
            Validated = 1;
        }
    }

    void OnPaint(int layer)
    {
        if(!(Flags&sWF_Focus) && Ref)
        {
            Buffer.PrintF("%f",*Ref);
            Editor.NewText();
            Validated = 1;
        }
        sStringControl::OnPaint(layer);
    }

    void Drag(const sDragData &dd,int speed)
    {
        T v;
        switch(dd.Mode)
        {
        case sDM_Start:
            DragStart = *Ref;
            break;
        case sDM_Drag:
            v = (T) sClamp<TF>(TF(DragStart+float(int(dd.RawDeltaX/2))*Step[speed]),Min,Max);
            if(v!=*Ref)
            {
                *Ref = v;
                ChangeMsg.Send();
                v = *Ref;
                Buffer.PrintF("%f",v);
                Editor.NewText();
            }
            break;
        case sDM_Stop:
            DoneMsg.Send();
            break;
        }
    }

    void DragLinked(const sDragData &dd,int speed)
    {
        sValueControl<T,TF> *l = Link;
        do
        {
            l->Drag(dd,speed);
            l = l->Link;
        }
        while(l!=Link);
    }

    void CmdDefault()
    {
        if(Ref)
        {
            *Ref = Default;
            ChangeMsg.Send();
            DoneMsg.Send();
            Buffer.PrintF("%f",Default);
            Editor.NewText();
        }
    }

    void CmdDefaultLinked()
    {
        sValueControl<T,TF> *l = Link;
        do
        {
            l->CmdDefault();
            l = l->Link;
        }
        while(l!=Link);
    }
};

typedef sValueControl<int,int> sIntControl;
typedef sValueControl<float,float> sFloatControl;

typedef sValueControl<int,int> sS32Control;
typedef sValueControl<uint,uint> sU32Control;
typedef sValueControl<float,float> sF32Control;
typedef sValueControl<int64,int64> sS64Control;
typedef sValueControl<uint64,uint64> sU64Control;
typedef sValueControl<double,double> sF64Control;

typedef sValueControl< int8,int>  sS8Control;
typedef sValueControl< uint8,int>  sU8Control;
typedef sValueControl<int16,int> sS16Control;
typedef sValueControl<uint16,int> sU16Control;

/****************************************************************************/

class sDropControl : public sControl
{
    const char *Label;
    sDragDropIcon *StoreIcon;
public:
    sDropControl();
    ~sDropControl();
    void Set(const sGuiMsg &msg,const char *label,sDragDropIcon *storeicon);
    void OnPaint(int layer);

    bool OnDragDropTo(const sDragDropIcon &icon,int &mx,int &my,bool checkonly);
    virtual bool HandleDragDrop(const sDragDropIcon &icon,int &mx,int &my,bool checkonly) { return true; }
};

/****************************************************************************/
}
#endif  // FILE_ALTONA2_LIBS_GUI_BUTTONS_HPP

