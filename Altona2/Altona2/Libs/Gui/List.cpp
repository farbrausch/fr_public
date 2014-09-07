/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Gui/Gui.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sListWindowField::sListWindowField()
{
    Name = "";
    Type = sLWT_S32;
    Flags = 0;
    Width = 100;
    Choices = 0;
    S32 = 0;
    MinS32 = 0;
    MaxS32 = 0;
    Choices = 0;
}

sListWindowField::sListWindowField(sListWindowFieldType type,const char *name,int width,int flags)
{
    Name = name;
    Type = type;
    Flags = flags;
    Width = width;
    Choices = 0;
    S32 = 0;
    MinS32 = 0;
    MaxS32 = 0; 
    Choices = 0;
    SortUp = 0;
    SortGrade = 0;
}

sListWindowField::~sListWindowField()
{
    delete Choices;
}

sListWindowElementInfo::sListWindowElementInfo()
{
    Selected = 0;
    Opened = 1;
    HasChilds = 0;
    IsVisible = 0;
    Indent = 0;
    BackColor = 0;
}

/****************************************************************************/

sListWindowBase::sListWindowBase(int flags)
{
    Array = 0;
    InfoPtr = 0;
    Flags = flags;
    DragMode = 0;

    Construct();
}

sListWindowBase::sListWindowBase(sArray<sListWindowDummyElement*> &a,sListWindowElementInfo sListWindowDummyElement::*info,int flags)
{
    Array = &a;
    InfoPtr = info;
    Flags = flags;

    Construct();
}

sListWindowBase::~sListWindowBase()
{
    Fields.DeleteAll();
}

void sListWindowBase::Construct()
{
    Height = 20;
    Cursor = -1;
    SelectStart = -1;
    SearchBorder = 0;
    EditChoiceLine = -1;
    EditChoiceField = 0;
    Strikeout = 0;
    StrikeoutSet = false;

    if(Flags & sLWF_AllowEdit)
        AddDrag("edit"        ,sKEY_LMB|sKEYQ_Double  ,sGuiMsg(this,&sListWindowBase::DragEdit));
    AddDrag("select"        ,sKEY_LMB               ,sGuiMsg(this,&sListWindowBase::DragSelect,0));
    AddKey("select up"      ,sKEY_Up                ,sGuiMsg(this,&sListWindowBase::CmdUp,0));
    AddKey("select down"    ,sKEY_Down              ,sGuiMsg(this,&sListWindowBase::CmdDown,0));
    AddKey("select first"   ,sKEY_Home              ,sGuiMsg(this,&sListWindowBase::CmdBegin,0));
    AddKey("select last"    ,sKEY_End               ,sGuiMsg(this,&sListWindowBase::CmdEnd,0));
    if(Flags & sLWF_Tree)
    {
        AddKey("expand tree"  ,sKEY_Right             ,sGuiMsg(this,&sListWindowBase::CmdTree,1));
        AddKey("collapse tree",sKEY_Left              ,sGuiMsg(this,&sListWindowBase::CmdTree,0));
    }
    if(Flags & sLWF_Move)
    {
        AddKey("move up"      ,sKEYQ_Ctrl|sKEY_Up     ,sGuiMsg(this,&sListWindowBase::CmdMoveUp));
        AddKey("move down"    ,sKEYQ_Ctrl|sKEY_Down   ,sGuiMsg(this,&sListWindowBase::CmdMoveDown));
        if(Flags & sLWF_Tree)
        {
            AddKey("move left"  ,sKEYQ_Ctrl|sKEY_Left   ,sGuiMsg(this,&sListWindowBase::CmdMoveLeft));
            AddKey("move right" ,sKEYQ_Ctrl|sKEY_Right  ,sGuiMsg(this,&sListWindowBase::CmdMoveRight));
        }
    }
    else
    {
        AddKey("cursor up"    ,sKEYQ_Ctrl|sKEY_Up     ,sGuiMsg(this,&sListWindowBase::CmdUp,3));
        AddKey("cursor down"  ,sKEYQ_Ctrl|sKEY_Down   ,sGuiMsg(this,&sListWindowBase::CmdDown,3));
        AddKey("select"       ,sKEY_Space             ,sGuiMsg(this,&sListWindowBase::CmdSelect,0));
    }
    if(Flags & sLWF_MultiSelect)
    {
        AddDrag("select (range)"      ,sKEYQ_Shift|sKEY_LMB  ,sGuiMsg(this,&sListWindowBase::DragSelect,2));
        AddDrag("select (toggle)"     ,sKEYQ_Ctrl|sKEY_LMB   ,sGuiMsg(this,&sListWindowBase::DragSelect,1));
        AddKey("select range up"      ,sKEYQ_Shift|sKEY_Up   ,sGuiMsg(this,&sListWindowBase::CmdUp,1));
        AddKey("select range down"    ,sKEYQ_Shift|sKEY_Down ,sGuiMsg(this,&sListWindowBase::CmdDown,1));
        AddKey("select range to first",sKEYQ_Shift|sKEY_Home ,sGuiMsg(this,&sListWindowBase::CmdBegin,1));
        AddKey("select range to last" ,sKEYQ_Shift|sKEY_End  ,sGuiMsg(this,&sListWindowBase::CmdEnd,1));
        AddKey("select all"           ,sKEYQ_Ctrl|'a'        ,sGuiMsg(this,&sListWindowBase::CmdSelectAll));
        AddKey("toggle select"        ,sKEYQ_Ctrl|sKEY_Space ,sGuiMsg(this,&sListWindowBase::CmdSelect,2));
    }
    AddHelp();
}

void sListWindowBase::SetArray(sArray<sListWindowDummyElement*> &a,sListWindowElementInfo sListWindowDummyElement::*info)
{
    Array = &a;
    InfoPtr = info;

    Update();
}

void sListWindowBase::ResetFields()
{
    Fields.DeleteAll();
}

void sListWindowBase::AddHeader()
{
    sBorder *b = new sListHeaderBorder(this);
    AddBorder(b);
}

void sListWindowBase::AddSearch(const char *defaulttext)
{
    SearchBorder = new sListSearchBorder(this);
    if(defaulttext)
        SearchString = defaulttext;
    AddBorder(SearchBorder);
}

void sListWindowBase::DragSelect(const sDragData &dd,int mode)
{
    int n=0;
    switch(dd.Mode)
    {
    case sDM_Start:
        if(mode==0 || !(Flags & sLWF_MultiSelect))
            ClearSelect();

        n = (dd.StartY - Client.y0)/Height;
        if(FilteredArray.IsIndex(n))
        {
            sListWindowDummyElement *e = FilteredArray[n];
            int x = Client.x0;
            for(auto f : Fields)
            {
                if(dd.StartX>=x && dd.StartX<x+f->Width)
                {
                    int indent = (e->*InfoPtr).Indent+1;
                    if((f->Type==sLWT_Tree || f->Type==sLWT_TreeP) && dd.StartX < x+indent*Style()->Font->CellHeight)
                    {
                        if((e->*InfoPtr).HasChilds)
                        {
                            (e->*InfoPtr).Opened = !(e->*InfoPtr).Opened;
                            TreeMsg.Send();
                            Update();
                        }
                    }
                }
                x+= f->Width;
            }
            if(mode==2)
            {
                Cursor = n;
                SelectByCursor(1);
                SelectMsg.SendCode((int)Array->FindEqualIndex(FilteredArray[n]));
            }
            else
            {
                Cursor = SelectStart = n;
                bool *sel = &(e->*InfoPtr).Selected;
                *sel = !*sel;
                SelectMsg.SendCode(*sel ? (int)Array->FindEqualIndex(FilteredArray[n]) : -1);
            }
            Update();
        }
        else if(mode==0)
        {
            SelectMsg.SendCode(-1);
            Update();
        }
        break;

    case sDM_Drag:
        if(mode==0 || !(Flags & sLWF_MultiSelect))
        {
            ClearSelect();

            int n = (dd.PosY - Client.y0)/Height;
            if(FilteredArray.IsIndex(n))
            {
                sListWindowDummyElement *e = FilteredArray[n];
                Cursor = SelectStart = n;
                bool *sel = &(e->*InfoPtr).Selected;
                *sel = !*sel;
                SelectMsg.SendCode(*sel ? (int)Array->FindEqualIndex(FilteredArray[n]) : -1);
                Update();
            }
        }
        else if(Flags & sLWF_MultiSelect)
        {
            int n = (dd.PosY - Client.y0)/Height;
            if(FilteredArray.IsIndex(n) && n!=Cursor)
            {
                sListWindowDummyElement *e = FilteredArray[n];
                if(mode==2)
                {
                    Cursor = n;
                    SelectByCursor(1);
                    SelectMsg.SendCode((int)Array->FindEqualIndex(FilteredArray[n]));
                }
                else
                {
                    Cursor = SelectStart = n;
                    bool *sel = &(e->*InfoPtr).Selected;
                    *sel = !*sel;
                    SelectMsg.SendCode(*sel ? (int)Array->FindEqualIndex(FilteredArray[n]) : -1);
                }
                Update();
            }
        }

        break;
    default:
        break;
    }
}

void sListWindowBase::DragEdit(const sDragData &dd)
{
    int n=0;
    if(dd.Mode==sDM_Start)
    {
        n = (dd.StartY - Client.y0)/Height;
        if(FilteredArray.IsIndex(n))
        {
            sListWindowDummyElement *e = FilteredArray[n];
            int x = Client.x0;
            for(auto f : Fields)
            {
                if(dd.StartX>=x && dd.StartX<x+f->Width)
                {
                    EditField(e,f,n);
                }
                x+= f->Width;
            }
        }
    }
}

void sListWindowBase::CmdSetChoice(int n)
{
    if(FilteredArray.IsIndex(EditChoiceLine) && EditChoiceField)
    {
        sListWindowDummyElement *e = FilteredArray[EditChoiceLine];
        (e->*(EditChoiceField->S32)) = n;
        ChangeMsg.Post();
        DoneMsg.Post();
    }
    EditChoiceLine = -1;
    EditChoiceField = 0;
}

void sListWindowBase::EditField(sListWindowDummyElement *e,sListWindowField *f,int line)
{
    if(!(f->Flags & sLFF_Edit) || !(Flags & sLWF_AllowEdit))
        return;

    sRect r;
    int x = Client.x0;
    for(auto ff : Fields)
    {
        if(f==ff)
            break;
        x += ff->Width;
    }
    r.x0 = x;
    r.x1 = x + f->Width;
    if(f==Fields[Fields.GetCount()-1])
        r.x1 = Client.x1;
    r.y0 = Client.y0 + line*Height;
    r.y1 = r.y0 + Height;


    sControl *con = 0;
    sStringDesc desc;
    bool killdone = 0;
    switch(f->Type)
    {
    case sLWT_String:
    case sLWT_Tree:
        desc.Buffer = &(e->*(f->String));
        desc.Size = f->StringSize;
        con = new sStringControl(desc);
        killdone = 1;
        break;
    case sLWT_StringDesc:
        con = new sStringControl(e->*(f->StringDesc));
        killdone = 1;
        break;
    case sLWT_PoolString:
    case sLWT_TreeP:
        con = new sPoolStringControl(e->*(f->PoolString));
        killdone = 1;
        break;
    case sLWT_Choice:
        if(f->Choices->Choices.GetCount()==2 && f->Choices->Choices[0].Value==0 && f->Choices->Choices[1].Value==1)
        {
            (e->*(f->S32)) = !(e->*(f->S32));
            Update();
        }
        else
        {
            EditChoiceLine = line;
            EditChoiceField = f;
            auto m = f->Choices->CmdClick(sGuiMsg(this,&sListWindowBase::CmdSetChoice,0),this);
            Gui->AddDialog(m,r.x0,r.y1);
        }
        break;
    case sLWT_S32:
        con = new sIntControl(&(e->*(f->S32)),f->MinS32,f->MaxS32,sClamp<sS32>(0,f->MinS32,f->MaxS32));
        killdone = 1;
        break;
    case sLWT_U32:
        con = new sU32Control(&(e->*(f->U32)),f->MinU32,f->MaxU32,sClamp<sU32>(0,f->MinU32,f->MaxU32));
        killdone = 1;
        break;
    case sLWT_F32:
        con = new sFloatControl(&(e->*(f->F32)),f->MinF32,f->MaxF32,sClamp<sF32>(0,f->MinF32,f->MaxF32));
        killdone = 1;
        break;
    case sLWT_S64:
        con = new sS64Control(&(e->*(f->S64)),f->MinS64,f->MaxS64,sClamp<sS64>(0,f->MinS64,f->MaxS64));
        killdone = 1;
        break;
    case sLWT_U64:
        con = new sU64Control(&(e->*(f->U64)),f->MinU64,f->MaxU64,sClamp<sU64>(0,f->MinU64,f->MaxU64));
        killdone = 1;
        break;
    case sLWT_F64:
        con = new sF64Control(&(e->*(f->F64)),f->MinF64,f->MaxF64,sClamp<sF64>(0,f->MinF64,f->MaxF64));
        killdone = 1;
        break;
    case sLWT_Custom:
        con = EditCustomField(e,f,killdone);
        break;
    default:
        break;
    }
    if(con)
    {
        sKillMeFrame *kill = new sKillMeFrame(con);
        kill->Flags = sWF_Top;
        kill->FocusAfterKill = this;

        if(con->ChangeMsg.IsEmpty())
            con->ChangeMsg = ChangeMsg;
        if(con->DoneMsg.IsEmpty())
            con->DoneMsg = DoneMsg;

        if(killdone)
            con->AddKey("done",sKEY_Enter,sGuiMsg(kill,&sKillMeFrame::CmdKillAndFocus));
        Gui->AddDialog(kill,r);
        Gui->SetFocus(con);
    }
}

void sListWindowBase::ClearSelect()
{
    for(auto e : *Array)
        (e->*InfoPtr).Selected = 0;

    Cursor = -1;

    Update();
}

void sListWindowBase::SetSearchFocus()
{
    if(SearchBorder)
        SearchBorder->SetFocus();
}

void sListWindowBase::SetSelect(int n,bool sel)
{
    if(Array->IsIndex(n))
        ((*Array)[n]->*InfoPtr).Selected = sel;

    Update();
}

void sListWindowBase::SetSelect(sListWindowDummyElement *e,bool sel)
{
    (e->*InfoPtr).Selected = sel;
    Cursor =(int) Array->FindEqualIndex(e);

    Update();
}

void sListWindowBase::ScrollTo(int n)
{
    sRect r(Client.x0,Client.y0+(n-1)*Height,Client.x0,Client.y0+(n+2)*Height);
    sWindow::ScrollTo(r);
}

void sListWindowBase::ScrollTo(sListWindowDummyElement *e)
{
    int n = (int)FilteredArray.FindEqualIndex(e);
    if(n>=0)
        ScrollTo(n);
}

sListWindowDummyElement *sListWindowBase::GetSelect()
{
    if(FilteredArray.IsIndex(Cursor) && (FilteredArray[Cursor]->*InfoPtr).Selected)
        return (FilteredArray)[Cursor];

    for(auto e : FilteredArray)
        if((e->*InfoPtr).Selected)
            return e;
    return 0;
}

sptr sListWindowBase::GetAddPosition()   // add new elements after cursor, or at end of list
{
    sListWindowDummyElement *e = GetSelect();
    if(e)
        return FilteredArray.FindEqualIndex(e)+1;
    else
        return FilteredArray.GetCount();
}

void sListWindowBase::SelectByCursor(int mode)
{
    if(Cursor<0)
        Cursor = 0;
    for(auto e : *Array)
        (e->*InfoPtr).Selected = 0;
    if(mode==0)
        SelectStart = Cursor;
    if((Flags & sLWF_MultiSelect) && FilteredArray.IsIndex(SelectStart) && FilteredArray.IsIndex(Cursor))
    {
        int e0 = Cursor;
        int e1 = SelectStart;
        if(e0>e1) sSwap(e0,e1);
        for(int i=e0;i<=e1;i++)
            (FilteredArray[i]->*InfoPtr).Selected = 1;
    }
    else if(FilteredArray.IsIndex(Cursor))
    {
        (FilteredArray[Cursor]->*InfoPtr).Selected = 1;
    }
    SelectMsg.SendCode((int)Array->FindEqualIndex(FilteredArray[Cursor]));
    ScrollToCursor();
}

void sListWindowBase::ScrollToCursor()
{
    ScrollTo(Cursor);
}

void sListWindowBase::CmdDown(int mode)
{
    if(Cursor<FilteredArray.GetCount()-1)
        Cursor++;
    if(mode<3)
        SelectByCursor(mode);
}

void sListWindowBase::CmdUp(int mode)
{
    if(Cursor>0)
        Cursor--;
    if(mode<3)
        SelectByCursor(mode);
}

void sListWindowBase::CmdBegin(int mode)
{
    Cursor = 0;
    SelectByCursor(mode);
}

void sListWindowBase::CmdEnd(int mode)
{
    Cursor = int(FilteredArray.GetCount()-1);
    SelectByCursor(mode);  
}

void sListWindowBase::CmdSelect(int mode)
{
    if(mode==0 || !(Flags & sLWF_MultiSelect))
        ClearSelect();
    if(FilteredArray.IsIndex(Cursor))
    {
        if(mode==1)
            (FilteredArray[Cursor]->*InfoPtr).Selected = 1;
        else
            (FilteredArray[Cursor]->*InfoPtr).Selected = !(FilteredArray[Cursor]->*InfoPtr).Selected ;
    }
}

void sListWindowBase::CmdSelectAll()
{
    for(auto e : FilteredArray)
        (e->*InfoPtr).Selected = 1;
    Update();
    SelectMsg.SendCode(0);
}

void sListWindowBase::CmdMoveUp()
{
    if((Flags & sLWF_Move) && !Array->IsEmpty() && !((*Array)[0]->*InfoPtr).Selected)
    {
        for(int i=1;i<Array->GetCount();i++)
        {
            if(((*Array)[i]->*InfoPtr).Selected)
                Array->Swap(i-1,i);
        }
        if(Cursor>0)
            Cursor--;
        Filter();
        Update();
        ScrollToCursor();
        MoveMsg.Send();
    }
}

void sListWindowBase::CmdMoveDown()
{
    if((Flags & sLWF_Move) && !Array->IsEmpty() && !((*Array)[Array->GetCount()-1]->*InfoPtr).Selected)
    {
        for(int i=Array->GetCount()-2;i>=0;i--)
        {
            if(((*Array)[i]->*InfoPtr).Selected)
                Array->Swap(i,i+1);
        }
        if(Cursor<Array->GetCount()-1)
            Cursor++;
        Filter();
        Update();
        ScrollToCursor();
        MoveMsg.Send();
    }
}

void sListWindowBase::CmdMoveLeft()
{
    if(Flags & sLWF_Move)
    {
        for(int i=0;i<Array->GetCount();i++)
        {
            sListWindowElementInfo *info = &((*Array)[i]->*InfoPtr);
            if(info->Selected)
            {
                if(info->Indent>0)
                    info->Indent--;
            }
        }
        Filter();
        Update();
        MoveMsg.Send();
        TreeMsg.Send();
    }
}

void sListWindowBase::CmdMoveRight()
{
    if(Flags & sLWF_Move)
    {
        for(int i=0;i<Array->GetCount();i++)
        {
            sListWindowElementInfo *info = &((*Array)[i]->*InfoPtr);
            if(info->Selected)
            {
                info->Indent++;
            }
        }
        Filter();
        Update();
        MoveMsg.Send();
        TreeMsg.Send();
    }
}

void sListWindowBase::CmdTree(int n)
{
    bool change = 0;
    for(int i=0;i<Array->GetCount();i++)
    {
        sListWindowElementInfo *info = &((*Array)[i]->*InfoPtr);
        bool nn = n ? 1 : 0;
        if(info->Selected)
        {
            if(info->Opened!=nn)
            {
                change = 1;
                info->Opened = nn;
            }
        }
    }
    if(change)
    {
        Filter();
        Update();
        TreeMsg.Send();  
    }
}

/****************************************************************************/

void sListWindowBase::AddField(const char *name,int width,int flags,int sListWindowDummyElement::*ref,int min,int max)
{
    sListWindowField *field = new sListWindowField(sLWT_S32,name,width,flags);
    field->S32 = ref;
    field->MinS32 = min;
    field->MaxS32 = max;
    Fields.Add(field);
}

void sListWindowBase::AddField(const char *name,int width,int flags,uint sListWindowDummyElement::*ref,uint min,uint max)
{
    sListWindowField *field = new sListWindowField(sLWT_U32,name,width,flags);
    field->U32 = ref;
    field->MinU32 = min;
    field->MaxU32 = max;
    Fields.Add(field);
}

void sListWindowBase::AddField(const char *name,int width,int flags,float sListWindowDummyElement::*ref,float min,float max)
{
    sListWindowField *field = new sListWindowField(sLWT_F32,name,width,flags);
    field->F32 = ref;
    field->MinF32 = min;
    field->MaxF32 = max;
    Fields.Add(field);
}

void sListWindowBase::AddField(const char *name,int width,int flags,int64 sListWindowDummyElement::*ref,int64 min,int64 max)
{
    sListWindowField *field = new sListWindowField(sLWT_S64,name,width,flags);
    field->S64 = ref;
    field->MinS64 = min;
    field->MaxS64 = max;
    Fields.Add(field);
}

void sListWindowBase::AddField(const char *name,int width,int flags,uint64 sListWindowDummyElement::*ref,uint64 min,uint64 max)
{
    sListWindowField *field = new sListWindowField(sLWT_U64,name,width,flags);
    field->U64 = ref;
    field->MinU64 = min;
    field->MaxU64 = max;
    Fields.Add(field);
}

void sListWindowBase::AddField(const char *name,int width,int flags,double sListWindowDummyElement::*ref,double min,double max)
{
    sListWindowField *field = new sListWindowField(sLWT_F64,name,width,flags);
    field->F64 = ref;
    field->MinF64 = min;
    field->MaxF64 = max;
    Fields.Add(field);
}


void sListWindowBase::AddField(const char *name,int width,int flags,char sListWindowDummyElement::*ref,int size)
{
    sListWindowField *field = new sListWindowField(sLWT_String,name,width,flags);
    field->String = ref;
    field->StringSize = size;
    Fields.Add(field);
}

void sListWindowBase::AddField(const char *name,int width,int flags,sPoolString sListWindowDummyElement::*ref)
{
    sListWindowField *field = new sListWindowField(sLWT_PoolString,name,width,flags);
    field->PoolString = ref;
    Fields.Add(field);
}

void sListWindowBase::AddField(const char *name,int width,int flags,sStringDesc sListWindowDummyElement::*ref)
{
    sListWindowField *field = new sListWindowField(sLWT_StringDesc,name,width,flags);
    field->StringDesc = ref;
    Fields.Add(field);
}

void sListWindowBase::AddFieldChoice(const char *name,int width,int flags,int sListWindowDummyElement::*ref,const char *choices)
{
    sListWindowField *field = new sListWindowField(sLWT_Choice,name,width,flags);
    field->S32 = ref;
    field->Choices = new sChoiceManager();
    field->Choices->SetChoices(choices);
    Fields.Add(field);
}

void sListWindowBase::AddTree(const char *name,int width,int flags,char sListWindowDummyElement::*ref)
{
    sListWindowField *field = new sListWindowField(sLWT_Tree,name,width,flags);
    field->String = ref;
    Fields.Add(field);
}

void sListWindowBase::AddTree(const char *name,int width,int flags,sPoolString sListWindowDummyElement::*ref)
{
    sListWindowField *field = new sListWindowField(sLWT_TreeP,name,width,flags);
    field->PoolString = ref;
    Fields.Add(field);
}

void sListWindowBase::AddStrikeout(bool sListWindowDummyElement::*ref)
{
    Strikeout = ref;
    StrikeoutSet = true;
}

void sListWindowBase::AddFieldCustom(const char *name,int width,int flags,int customid)
{
    sListWindowField *field = new sListWindowField(sLWT_Custom,name,width,flags);
    field->CustomId = customid;
    Fields.Add(field);
}

/****************************************************************************/

void sListWindowBase::OnCalcSize()
{
    Filter();
    Height = Style()->Font->CellHeight +2;

    int w = 0;
    for(auto f : Fields)
        w += f->Width;

    int l = (int)FilteredArray.GetCount();

    ReqSizeX = w;
    ReqSizeY = Height * l;
}

struct CmpListWindow
{ 
    sArray<sListWindowField *> *sorts;

    inline bool operator()(const sListWindowDummyElement *a,const sListWindowDummyElement *b) const
    {
        for(auto f : *sorts)
        {
            int n = compare(a,b,f);
            if(n!=0)
                return n>0;
        }
        return 0;
    }

    inline int compare(const sListWindowDummyElement *a,const sListWindowDummyElement *b,const sListWindowField *f) const 
    { 
        int n = 0;
        switch(f->Type)
        {
        case sLWT_Choice:       n = sCmp((a->*(f->U32)) , (b->*(f->U32))); break;
        case sLWT_S32:          n = sCmp((a->*(f->S32)) , (b->*(f->S32))); break;
        case sLWT_U32:          n = sCmp((a->*(f->U32)) , (b->*(f->U32))); break;
        case sLWT_F32:          n = sCmp((a->*(f->F32)) , (b->*(f->F32))); break;
        case sLWT_S64:          n = sCmp((a->*(f->S64)) , (b->*(f->S64))); break;
        case sLWT_U64:          n = sCmp((a->*(f->U64)) , (b->*(f->U64))); break;
        case sLWT_F64:          n = sCmp((a->*(f->F64)) , (b->*(f->F64))); break;
        case sLWT_String:       n = sCmpStringI(&(a->*(f->String)),&(b->*(f->String))); break;
        case sLWT_PoolString:   n = sCmpStringI(a->*(f->PoolString),b->*(f->PoolString)); break;
        case sLWT_StringDesc:   n = sCmpStringI((a->*(f->StringDesc)).Buffer,(b->*(f->StringDesc)).Buffer); break;
        default: break;
        }
        if(f->SortUp)
            n = -n;      

        return n;
    }
};

void sListWindowBase::Filter()
{
    sListWindowDummyElement *e;
    int max = Array->GetCount();
    FilteredArray.Clear();
    int indent = -1;

    sString<sFormatBuffer> filter;
    bool fflag = 0;
    if(!SearchString.IsEmpty())
    {
        fflag = 1;
        filter.PrintF("*%s*",SearchString);
    }

    for(int i=0;i<max;i++)
    {
        e = (*Array)[i];
        (e->*InfoPtr).HasChilds = i<max-1 && ((*Array)[i+1]->*InfoPtr).Indent > ((*Array)[i]->*InfoPtr).Indent;
        (e->*InfoPtr).IsVisible = 0;
        bool skip = 0;
        if(fflag)
        {
            skip = 1;
            for(auto f : Fields)
            {
                if(f->Type==sLWT_String || f->Type==sLWT_Tree)
                    if(sMatchWildcard(filter,&(e->*(f->String)),0,1))
                        skip = 0;
                if(f->Type==sLWT_PoolString || f->Type==sLWT_TreeP)
                    if(sMatchWildcard(filter,e->*(f->PoolString),0,1))
                        skip = 0;
                if(f->Type==sLWT_StringDesc || f->Type==sLWT_Tree)
                    if(sMatchWildcard(filter,(e->*(f->StringDesc)).Buffer,0,1))
                        skip = 0;
            }
        }
        if(indent!=-1)
        {
            if(indent<(e->*InfoPtr).Indent)
                skip = 1;
            else
                indent = -1;
        }
        if(!skip)
        {
            if(!(e->*InfoPtr).Opened)
                indent = (e->*InfoPtr).Indent;
            FilteredArray.Add(e);
            (e->*InfoPtr).IsVisible = 1;
        }
    }

    CmpListWindow cmp;
    sArray<sListWindowField *> sorts;
    for(auto f : Fields)
        if(f->SortGrade>0)
            sorts.Add(f);
    sorts.QuickSort([=](const sListWindowField *a,const sListWindowField *b){return a->SortGrade>b->SortGrade;});
    //  sHeapSort(sAll(sorts),sMemPtrGreater(&sListWindowField::SortGrade));
    cmp.sorts = &sorts;
    if(!sorts.IsEmpty())
    {
        FilteredArray.QuickSort(cmp);
        //    sHeapSort(sAll(FilteredArray),cmp);
    }
    if(Cursor>=FilteredArray.GetCount())
        Cursor = FilteredArray.GetCount()-1;
}

void sListWindowBase::OnPaint(int layer)
{
    int y = Client.y0;

    Filter();

    sListWindowField *lastfield = Fields[Fields.GetCount()-1];
    int i = 0;
    for(auto e : FilteredArray)
    {
        int x = Client.x0;
        for(auto f : Fields)
        {
            int x1 = x+f->Width-1;
            if(f==lastfield)
                x1 = Client.x1;
            OnPaintField(layer,e,f,sRect(x,y,x1,y+Height));
            x += f->Width;
        }
        if(Cursor==i)
        {
            Painter()->Frame(Style()->Colors[sGC_Draw],sRect(Client.x0,y,Client.x1,y+Height));
        }
        if(StrikeoutSet && e->*Strikeout)
        {
            Painter()->Rect(Style()->Colors[sGC_Text],sRect(Client.x0,y+Height/2-1,Client.x1,y+Height/2+1));
        }
        i++;
        y += Height;
    }

    int x = Client.x0;
    for(auto f : Fields)
    {
        if(f!=lastfield)
        {
            x += f->Width;
            Painter()->Rect(Style()->Colors[sGC_Draw],sRect(x-1,Client.y0,x,Client.y1));
        }
    }

    if(y<Client.y1)
    {
        Style()->Rect(layer,this,sSK_Back,sRect(Client.x0,y,Client.x1,Client.y1));
    }
}

void sListWindowBase::OnPaintField(int layer,sListWindowDummyElement *e,sListWindowField *f,const sRect &r)
{
    sString<64> buffer;
    const char *str = buffer;
    sGuiStyleKind style = sSK_ListField;

    if(!r.IsPartiallyInside(Inner))
        return;

    switch(f->Type)
    {
    case sLWT_S32:
        buffer.PrintF("%d",e->*(f->S32));
        break;
    case sLWT_U32:
        buffer.PrintF("%d",e->*(f->U32));
        break;
    case sLWT_F32:
        buffer.PrintF("%f",e->*(f->F32));
        break;
    case sLWT_S64:
        buffer.PrintF("%d",e->*(f->S64));
        break;
    case sLWT_U64:
        buffer.PrintF("%d",e->*(f->U64));
        break;
    case sLWT_F64:
        buffer.PrintF("%f",e->*(f->F64));
        break;
    case sLWT_String:
        str = &(e->*(f->String));
        break;
    case sLWT_StringDesc:
        str = (e->*(f->StringDesc)).Buffer;
        break;
    case sLWT_PoolString:
        str = (e->*(f->PoolString));
        break;
    case sLWT_Choice:
        {
            sChoiceInfo *ci = f->Choices->FindChoice(e->*(f->S32));
            if(ci)
                buffer.Init(ci->Label,ci->LabelLen);
            else
                buffer = "???";
        }
        break;
    case sLWT_Tree:
        str = &(e->*(f->String));
        style = sSK_TreeField;
        Style()->TreeIndent = (e->*InfoPtr).Indent;
        Style()->TreeOpened = (e->*InfoPtr).Opened;
        Style()->TreeHasChilds = (e->*InfoPtr).HasChilds;
        break;
    case sLWT_TreeP:
        str = (e->*(f->PoolString));
        style = sSK_TreeField;
        Style()->TreeIndent = (e->*InfoPtr).Indent;
        Style()->TreeOpened = (e->*InfoPtr).Opened;
        Style()->TreeHasChilds = (e->*InfoPtr).HasChilds;
        break;
    case sLWT_Custom:
        PaintCustomField(layer,e,f,r);
        break;
    default:
        sASSERT0();
    }

    PaintField(layer,e,f,r,style,str);
}

void sListWindowBase::PaintField(int layer,sListWindowDummyElement *e,sListWindowField *f,const sRect &r,sGuiStyleKind style,const char *str)
{
    int mod = 0;
    if((e->*InfoPtr).Selected)
        mod |= sSM_Pressed;

    if((e->*InfoPtr).BackColor)
    {
        Style()->OverrideColors = 1;
        Style()->OverrideColorsBack = (e->*InfoPtr).BackColor;
        Style()->OverrideColorsText = 0;
    }
    Style()->Rect(layer,this,style,sRect(r.x0,r.y0,r.x1,r.y1),mod,str);
}

/****************************************************************************/
/****************************************************************************/

sListHeaderBorder::sListHeaderBorder(sListWindowBase *lw)
{
    List = lw;
    Height = 0;
    DragField = 0;
    DragStart = 0;

    AddDrag("resize",sKEY_LMB,sGuiMsg(this,&sListHeaderBorder::DragResize,0));
    AddDrag("resize",sKEY_LMB|sKEYQ_Shift,sGuiMsg(this,&sListHeaderBorder::DragResize,1));
    AddHelp();
}

sListHeaderBorder::~sListHeaderBorder()
{
}


void sListHeaderBorder::OnCalcSize()
{
    Height = Style()->Font->CellHeight + 2;
    ReqSizeX = 0;
    ReqSizeY = Height;
}

void sListHeaderBorder::OnLayoutChilds()
{
    Inner.y0 += Height;
}

void sListHeaderBorder::OnPaint(int layer)
{
    int y0 = Outer.y0;
    int y1 = y0+Height-1;

    uint colb = Style()->Colors[sGC_Button];
    uint colt = Style()->Colors[sGC_Text];
    uint cold = Style()->Colors[sGC_Draw];

    Painter()->SetLayer(layer+0);
    Painter()->SetFont(Style()->Font);

    Painter()->Rect(colb,sRect(Outer.x0,y0,Outer.x1,y1));
    Painter()->Rect(cold,sRect(Outer.x0,y1,Outer.x1,y1+1));

    int x = Outer.x0 - List->ScrollX;
    for(auto f : List->Fields)
    {
        int x1 = x+f->Width;
        if(f == List->Fields[List->Fields.GetCount()-1])
            x1 = Outer.x1+1;

        Painter()->SetLayer(layer+2);
        Painter()->Text(sPPF_Left|sPPF_Space,colt,sRect(x,y0,x1-1,y1),f->Name);
        sString<8> sort;
        char *s = sort;
        for(int i=0;i<f->SortGrade && i<7;i++) 
            *s++ = f->SortUp ? '>' : '<';
        *s++ = 0;
        Painter()->Text(sPPF_Right,colt,sRect(x,y0,x1-1,y1),sort);
        Painter()->SetLayer(layer+1);
        x=x1;
        Painter()->Rect(cold,sRect(x-1,y0,x,y1));
    }
}

sListWindowField *sListHeaderBorder::FindFieldSize(int mx)
{
    int x = Outer.x0 - List->ScrollX;
    sListWindowField *lastfield = List->Fields[List->Fields.GetCount()-1];
    for(auto f : List->Fields)
    {
        if(f!=lastfield)
        {
            x += f->Width;
            if(mx>=x-3 && mx<x+3)
                return f;
        }
    }
    return 0;
}

sListWindowField *sListHeaderBorder::FindFieldSort(int mx)
{
    int x = Outer.x0 - List->ScrollX;
    sListWindowField *lastfield = List->Fields[List->Fields.GetCount()-1];
    for(auto f : List->Fields)
    {
        x += f->Width;
        if(f==lastfield)
            x = Outer.x1;
        if(mx>=x-20 && mx<x)
            return f;
    }
    return 0;
}

void sListHeaderBorder::OnHover(const sDragData &dd)
{
    MouseCursor = sHCI_Arrow;
    if(FindFieldSize(dd.PosX))
        MouseCursor = sHCI_SizeH;
    else if(FindFieldSort(dd.PosX))
        MouseCursor = sHCI_SizeV;
}

void sListHeaderBorder::DragResize(const sDragData &dd,int shift)
{
    switch(dd.Mode)
    {
    case sDM_Start:
        DragField = FindFieldSize(dd.StartX);
        if(DragField)
        {
            DragStart = DragField->Width;
        }
        else
        {
            sListWindowField *f = FindFieldSort(dd.StartX);
            if(f && (f->Flags & sLFF_Sort))
            {
                if(shift)
                {
                    if(f->SortGrade==0)
                    {
                        for(auto ff : List->Fields)
                            if(ff->SortGrade>0)
                                ff->SortGrade++;
                        f->SortUp = 1;
                        f->SortGrade = 1;
                    }
                    else
                    {
                        for(auto ff : List->Fields)
                            ff->SortGrade = 0;
                        f->SortUp = 0;
                        f->SortGrade = 1;
                    }
                }
                else
                {
                    if(f->SortGrade!=0)
                    {
                        f->SortUp = !f->SortUp;
                    }
                    else
                    {
                        for(auto ff : List->Fields)
                            ff->SortGrade = 0;
                        f->SortUp = 1;
                        f->SortGrade = 1;
                    }
                }
                List->Filter();
                Update();
            }
        }
        break;

    case sDM_Drag:
        if(DragField)
        {
            DragField->Width = sMax(10,DragStart + dd.DeltaX);
            Update();
        }
        break;

    case sDM_Stop:
        DragField = 0;
        break;
    default:
        break;
    }
}

/****************************************************************************/

sListSearchBorder::sListSearchBorder(sListWindowBase *lw)
{
    List = lw;
    Height = 0;
    StringWin = new sStringControl(List->SearchString);
    StringWin->ChangeMsg = sGuiMsg(List,&sListWindowBase::Filter);
    AddChild(StringWin);
}

void sListSearchBorder::OnCalcSize()
{
    Height = Style()->Font->CellHeight + 4;
    ReqSizeX = 0;
    ReqSizeY = Height;
}

void sListSearchBorder::OnLayoutChilds()
{
    Inner.y1 -= Height;
    StringWin->Outer.Init(Inner.x0,Inner.y1,Inner.x1,Inner.y1+Height);
}

/****************************************************************************/
}
