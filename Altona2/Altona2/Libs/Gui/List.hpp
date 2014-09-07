/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_GUI_LIST_HPP
#define FILE_ALTONA2_LIBS_GUI_LIST_HPP

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Gui/Window.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

struct sListWindowDummyElement
{
};

enum sListWindowFieldType
{
    sLWT_S32 = 1,
    sLWT_U32,
    sLWT_F32,
    sLWT_S64,
    sLWT_U64,
    sLWT_F64,
    sLWT_String,
    sLWT_PoolString,
    sLWT_Choice,
    sLWT_Tree,
    sLWT_TreeP,
    sLWT_StringDesc,
    sLWT_Custom,
};

enum sListWindowFieldFlags
{
    sLFF_Edit         = 0x0001,
    sLFF_Sort         = 0x0002,
};

enum sListWindowFlags
{
    sLWF_MultiSelect  = 0x0001,
    sLWF_Tree         = 0x0002,
    sLWF_Move         = 0x0004,
    sLWF_AllowEdit    = 0x0008,
};

struct sListWindowField
{
    sListWindowField();
    sListWindowField(sListWindowFieldType type,const char *name,int width,int flags);
    ~sListWindowField();
    const char *Name;
    sListWindowFieldType Type;
    int Flags;
    int Width;
    union
    {
        int sListWindowDummyElement::*S32;
        uint sListWindowDummyElement::*U32;
        float sListWindowDummyElement::*F32;
        int64 sListWindowDummyElement::*S64;
        uint64 sListWindowDummyElement::*U64;
        double sListWindowDummyElement::*F64;
        char sListWindowDummyElement::*String;
        sPoolString sListWindowDummyElement::*PoolString;
        sStringDesc sListWindowDummyElement::*StringDesc;
        int CustomId;
    };
    union
    {
        int MinS32;
        uint MinU32;
        float MinF32;
        int64 MinS64;
        uint64 MinU64;
        double MinF64;
    };
    union
    {
        int MaxS32;
        uint MaxU32;
        float MaxF32;
        int64 MaxS64;
        uint64 MaxU64;
        double MaxF64;
    };
    uptr StringSize;
    sChoiceManager *Choices;
    bool SortUp;
    int SortGrade;

    void SetSort(bool up=1) { SortUp = up; SortGrade = 1; }
};

struct sListWindowElementInfo
{
    bool Selected;
    bool Opened;
    bool HasChilds;
    bool IsVisible;
    int Indent;
    uint BackColor;
    sListWindowElementInfo();
};

class sListWindowBase : public sWindow
{
    friend class sListHeaderBorder;

    int Flags;

    int Height;
    int Cursor;
    int SelectStart;
    bool DragMode;

    int EditChoiceLine;
    sListWindowField *EditChoiceField;
    bool sListWindowDummyElement::*Strikeout;
    bool StrikeoutSet;

    class sListSearchBorder *SearchBorder;

    void Construct();
protected:
    sArray<sListWindowDummyElement *> *Array;
    sListWindowElementInfo sListWindowDummyElement::*InfoPtr;
    sArray<sListWindowField *> Fields;

    void SetArray(sArray<sListWindowDummyElement*> &a,sListWindowElementInfo sListWindowDummyElement::*info);
    void AddField(const char *name,int width,int flags,int sListWindowDummyElement::*ref,int min=0,int max=0);
    void AddField(const char *name,int width,int flags,uint sListWindowDummyElement::*ref,uint min=0,uint max=0);
    void AddField(const char *name,int width,int flags,float sListWindowDummyElement::*ref,float min=0,float max=0);
    void AddField(const char *name,int width,int flags,int64 sListWindowDummyElement::*ref,int64 min=0,int64 max=0);
    void AddField(const char *name,int width,int flags,uint64 sListWindowDummyElement::*ref,uint64 min=0,uint64 max=0);
    void AddField(const char *name,int width,int flags,double sListWindowDummyElement::*ref,double min=0,double max=0);
    void AddField(const char *name,int width,int flags,char sListWindowDummyElement::*ref,int size);
    void AddField(const char *name,int width,int flags,sPoolString sListWindowDummyElement::*ref);
    void AddField(const char *name,int width,int flags,sStringDesc sListWindowDummyElement::*ref);
    void AddFieldChoice(const char *name,int width,int flags,int sListWindowDummyElement::*ref,const char *choices);
    void AddTree(const char *name,int width,int flags,char sListWindowDummyElement::*ref);
    void AddTree(const char *name,int width,int flags,sPoolString sListWindowDummyElement::*ref);
    void AddStrikeout(bool sListWindowDummyElement::*ref);

    virtual void PaintCustomField(int layer,sListWindowDummyElement *e,sListWindowField *f,const sRect r) {}
    virtual sControl *EditCustomField(sListWindowDummyElement *e,sListWindowField *f,bool &killdone) { return 0; }

    void CmdSetChoice(int n);
    void EditField(sListWindowDummyElement *e,sListWindowField *f,int line);
    void SelectByCursor(int mode);
    void ScrollToCursor();
    void DragSelect(const sDragData &dd,int mode);
    void DragEdit(const sDragData &dd);

    void CmdUp(int mode);
    void CmdDown(int mode);
    void CmdBegin(int mode);
    void CmdEnd(int mode);
    void CmdSelect(int mode);
    void CmdSelectAll();
    void CmdMoveUp();
    void CmdMoveDown();
    void CmdMoveLeft();
    void CmdMoveRight();
    void CmdTree(int n);
public:
    sListWindowBase(int flags);
    ~sListWindowBase();
    sListWindowBase(sArray<sListWindowDummyElement*> &a,sListWindowElementInfo sListWindowDummyElement::*info,int flags);

    void Filter();
    void AddHeader();
    void AddSearch(const char *defaulttext = 0);
    void OnCalcSize();
    void OnPaint(int layer);
    void OnPaintField(int layer,sListWindowDummyElement *e,sListWindowField *f,const sRect &r);
    void ResetFields();
    void AddFieldCustom(const char *name,int width,int flags,int customid);
    void PaintField(int layer,sListWindowDummyElement *e,sListWindowField *f,const sRect &r,sGuiStyleKind style,const char *str);

    void ClearSelect();
    void SetSelect(int n,bool sel=1);
    void SetSelect(sListWindowDummyElement *e,bool sel=1);
    void ScrollTo(int n);
    void ScrollTo(sListWindowDummyElement *e);
    void SetSearchFocus();

    sptr GetAddPosition();
    sListWindowField *GetFieldInfo(int n) { return Fields[n]; }
    sListWindowField *GetFieldInfo(const char *name) { return Fields.Find([=](sListWindowField *f){ return sCmpStringI(f->Name,name)==0; }); }
    sListWindowDummyElement *GetSelect();
    sGuiMsg SelectMsg;
    sGuiMsg TreeMsg;
    sGuiMsg MoveMsg;
    sGuiMsg ChangeMsg;      // for editing
    sGuiMsg DoneMsg;        // for editing

    sString<sFormatBuffer> SearchString;
    sArray<sListWindowDummyElement *> FilteredArray;    // access with care
};

template <class T> class sListWindow : public sListWindowBase
{
public:
    sListWindow(int flags)
        : sListWindowBase(flags) {}
    sListWindow(sArray<T*> &a,sListWindowElementInfo T::*info,int flags)
        : sListWindowBase((sArray<sListWindowDummyElement *> &)a,(sListWindowElementInfo sListWindowDummyElement::*)info,flags) {}

    T *GetSelect() 
    { return (T*) sListWindowBase::GetSelect(); }
    void SetSelect(int n,bool sel=1) 
    { sListWindowBase::SetSelect(n,sel); }
    void SetSelect(T *e,bool sel = 1)
    { sListWindowBase::SetSelect((sListWindowDummyElement*)e,sel); }
    void ScrollTo(T *e)
    { sListWindowBase::ScrollTo((sListWindowDummyElement*)e); }

    void SetArray(sArray<T*> &a,sListWindowElementInfo T::*info)
    { sListWindowBase::SetArray((sArray<sListWindowDummyElement *> &)a,(sListWindowElementInfo sListWindowDummyElement::*)info); }
    void AddField(const char *name,int width,int flags,int T::*ref,int min=0,int max=0)
    { sListWindowBase::AddField(name,width,flags,(int sListWindowDummyElement::*) ref,min,max); }
    void AddField(const char *name,int width,int flags,uint T::*ref,uint min=0,uint max=0)
    { sListWindowBase::AddField(name,width,flags,(uint sListWindowDummyElement::*) ref,min,max); }
    void AddField(const char *name,int width,int flags,float T::*ref,float min=0,float max=0)
    { sListWindowBase::AddField(name,width,flags,(float sListWindowDummyElement::*) ref,min,max); }

    void AddField(const char *name,int width,int flags,int64 T::*ref,int min=0,int max=0)
    { sListWindowBase::AddField(name,width,flags,(int64 sListWindowDummyElement::*) ref,min,max); }
    void AddField(const char *name,int width,int flags,uint64 T::*ref,uint min=0,uint max=0)
    { sListWindowBase::AddField(name,width,flags,(uint64 sListWindowDummyElement::*) ref,min,max); }
    void AddField(const char *name,int width,int flags,double T::*ref,float min=0,float max=0)
    { sListWindowBase::AddField(name,width,flags,(double sListWindowDummyElement::*) ref,min,max); }

    void AddField(const char *name,int width,int flags,char T::*ref,int size)
    { sListWindowBase::AddField(name,width,flags,(char sListWindowDummyElement::*) ref,size); }
    void AddField(const char *name,int width,int flags,sPoolString T::*ref)
    { sListWindowBase::AddField(name,width,flags,(sPoolString sListWindowDummyElement::*) ref); }
    void AddField(const char *name,int width,int flags,sStringDesc T::*ref)
    { sListWindowBase::AddField(name,width,flags,(sStringDesc sListWindowDummyElement::*) ref); }
    void AddFieldChoice(const char *name,int width,int flags,int T::*ref,const char *choices)
    { sListWindowBase::AddFieldChoice(name,width,flags,(int sListWindowDummyElement::*) ref,choices); }

    template <int n> void AddField(const char *name,int width,int flags,sString<n> T::*ref)
    { AddField(name,width,flags,(char T::*) ref,n); }
    template <int n> void AddTree(const char *name,int width,int flags,sString<n> T::*ref)
    { sListWindowBase::AddTree(name,width,flags,(char sListWindowDummyElement::*) ref); }
    void AddTree(const char *name,int width,int flags,sPoolString T::*ref)
    { sListWindowBase::AddTree(name,width,flags,(sPoolString sListWindowDummyElement::*) ref); }

    void AddStrikeout(bool T::*ref)
    { sListWindowBase::AddStrikeout((bool sListWindowDummyElement::*)ref); }

    void EditField(T *element,int field)
    { auto e=(sListWindowDummyElement *)element; sListWindowBase::EditField((sListWindowDummyElement *)element,Fields[field],Array->FindEqualIndex(e)); }
};

/****************************************************************************/

class sListHeaderBorder : public sBorder
{
    sListWindowBase *List;
    int Height;
    sListWindowField *DragField;
    int DragStart;
    sListWindowField *FindFieldSize(int mx);
    sListWindowField *FindFieldSort(int mx);
public:
    sListHeaderBorder(sListWindowBase *lw);
    ~sListHeaderBorder();

    void OnCalcSize();
    void OnLayoutChilds();
    void OnPaint(int layer);
    void OnHover(const sDragData &dd);
    void DragResize(const sDragData &dd,int shift);
};

class sListSearchBorder : public sBorder
{
    sListWindowBase *List;
    int Height;
    sStringControl *StringWin;
public:
    sListSearchBorder(sListWindowBase *lw);

    void OnCalcSize();
    void OnLayoutChilds();

    void SetFocus() { Gui->SetFocus(StringWin); }
};

/****************************************************************************/
}
#endif  // FILE_ALTONA2_LIBS_GUI_LIST_HPP

