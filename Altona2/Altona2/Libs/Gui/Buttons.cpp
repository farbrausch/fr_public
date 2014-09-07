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
/***   Simple Windows                                                     ***/
/***                                                                      ***/
/****************************************************************************/

sDummyWindow::sDummyWindow()
{
    Label = "sDummyWindow";
    ClassName = "sDummyWindow";
    Flags |= sWF_ReceiveHover;
}

sDummyWindow::sDummyWindow(const char *label)
{
    Label = label;
    Flags |= sWF_ReceiveHover;
}


void sDummyWindow::OnPaint(int layer)
{
    Style()->Rect(layer,this,sSK_Dummy,Client,Flags & sWF_Hover ? sSM_Pressed : 0,Label);
}

/****************************************************************************/

sHSeparatorWindow::sHSeparatorWindow()
{
    ClassName = "sHSeparatorWindow";
}

void sHSeparatorWindow::OnCalcSize()
{
    ReqSizeX = 1;
    ReqSizeY = 0;
}

void sHSeparatorWindow::OnPaint(int layer)
{
    Style()->Rect(layer,this,sSK_HSeperator,Client);
}

/****************************************************************************/

sVSeparatorWindow::sVSeparatorWindow()
{
    ClassName = "sVSeparatorWindow";
}

void sVSeparatorWindow::OnCalcSize()
{
    ReqSizeX = 0;
    ReqSizeY = 1;
}

void sVSeparatorWindow::OnPaint(int layer)
{
    Style()->Rect(layer,this,sSK_HSeperator,Client);
}

/****************************************************************************/
/***                                                                      ***/
/***   Button, Choice                                                     ***/
/***                                                                      ***/
/****************************************************************************/

sControl::sControl()
{
    ClassName = "sControl";
}

sButtonControl::sButtonControl()
{
    ClassName = "sButtonControl";
    DragMode = 0;
    DragVerify = 0;
    InternalValue = 0;
    FixedSizeX = 0;
    Label = "???";
    LabelLen = -1;
    ButtonStyle = sBS_Push;
    RefPtr = &InternalValue;
    RefMask = ~0;
    RefValue = 1;
    Key = 0;
    SimulatePush = 0;
    BackColor = 0;

    AddKey("click",sKEY_Space,sGuiMsg(this,&sButtonControl::CmdClick));
    AddKey("click",sKEY_LMB,sGuiMsg(this,&sButtonControl::CmdClick));
    AddKey("click",sKEY_Pen,sGuiMsg(this,&sButtonControl::CmdClick));
    AddHelp();
}

sButtonControl::sButtonControl(const sGuiMsg &msg,const char *label,sButtonStyleEnum bs)
{
    ClassName = "Button";
    ClickMsg = msg;
    DragMode = 0;
    DragVerify = 0;
    InternalValue = 0;
    FixedSizeX = 0;
    Label = label;
    LabelLen = -1;
    ButtonStyle = bs;
    RefPtr = &InternalValue;
    RefMask = ~0;
    RefValue = 1;
    Key = 0;
    SimulatePush = 0;
    BackColor = 0;

    AddKey("click",sKEY_Space,sGuiMsg(this,&sButtonControl::CmdClick));
    AddKey("click",sKEY_LMB,sGuiMsg(this,&sButtonControl::CmdClick));
    AddKey("click",sKEY_Pen,sGuiMsg(this,&sButtonControl::CmdClick));
    AddHelp();

    SetStyle(bs);

    LoadIcon();
}

void sButtonControl::LoadIcon()
{
    /*
    if(Label && Label[0]=='#')
    {
        sString<256> buffer;
        int i = 0;
        buffer = Label+1;
        while(buffer[i]!=0 && buffer[i]!=' ')
            i++;
        if(buffer[i]==' ')
        {
            ToolTipText = Label+2+i;
            buffer[i] = 0;
        }
        Gui->IconManager.GetIcon(buffer,Icon);
    }
*/
    if(Gui->IconManager.GetIcon(Label,Icon,ToolTipText))
        Flags |= sWF_GiveSpace;
}

sButtonControl *sButtonControl::SetRef(int *ref,int val,int mask)
{
    RefPtr = ref;
    RefValue = val;
    RefMask = mask;
    return this;
}

sButtonControl *sButtonControl::SetStyle(sButtonStyleEnum bs)
{
    StyleKind = sSK_Back;
    switch(bs)
    {
    case sBS_Label:      StyleKind = sSK_Button_Label;     ClassName = "Label"; break;
    case sBS_Push:       StyleKind = sSK_Button_Push;      ClassName = "Push"; break;
    case sBS_Toggle:     StyleKind = sSK_Button_Toggle;    ClassName = "Toggle"; break;
    case sBS_Radio:      StyleKind = sSK_Button_Radio;     ClassName = "Radio"; break;

    case sBS_Menu:       StyleKind = sSK_Button_Menu;      Flags |= sWF_ReceiveHover; ClassName = "MenuPush";   DragVerify = Gui->TouchMode; break;
    case sBS_MenuToggle: StyleKind = sSK_Button_MenuCheck; Flags |= sWF_ReceiveHover; ClassName = "MenuToggle"; break;
    case sBS_MenuRadio:  StyleKind = sSK_Button_MenuCheck; Flags |= sWF_ReceiveHover; ClassName = "MenuRadio"; break;
    case sBS_ErrorMsg:   StyleKind = sSK_Button_ErrorMsg;  ClassName = "ErrorMsg"; break;
    }
    return this;
}

void sButtonControl::OnCalcSize()
{
    if(Icon.Loaded)
    {
        ReqSizeX = Icon.Uv.SizeX()+4;
        ReqSizeY = Icon.Uv.SizeY()+4;
    }
    else
    {
        sString<256> KeyBuffer;

        sFont *f = Style()->Font;

        if(FixedSizeX)
            ReqSizeX = FixedSizeX;
        else 
            ReqSizeX = f->CellHeight + f->GetWidth(Label,LabelLen);
        ReqSizeY = f->CellHeight;
        if(ButtonStyle==sBS_Menu || ButtonStyle==sBS_MenuRadio || ButtonStyle==sBS_MenuToggle)
            ReqSizeY += 1;
        else
            ReqSizeY += 4;
        if(ButtonStyle==sBS_MenuRadio || ButtonStyle==sBS_MenuToggle)
            ReqSizeX += f->CellHeight;

        if(Key)
        {
            sKeyName(KeyBuffer,Key);
            ReqSizeX += f->CellHeight + f->GetWidth(KeyBuffer);
        }
    }
}

void sButtonControl::OnPaint(int layer)
{
    int modifier = 0;

    if(ButtonStyle==sBS_Toggle || ButtonStyle==sBS_Radio || ButtonStyle==sBS_MenuToggle || ButtonStyle==sBS_MenuRadio)
    {
        if(((*RefPtr) & RefMask) == RefValue)
            modifier = sSM_Pressed;
    }
    else if(DragMode && DragVerify)
        modifier = sSM_Pressed;
    if(SimulatePush>0)
    {
        modifier = sSM_Pressed;
        SimulatePush--;
        Update();
    }
    if(Label[0]=='(')
        modifier |= sSM_Grayed;

    if(Icon.Loaded)
    {
        Style()->Rect(layer,this,StyleKind,Client,modifier,"");
        sRect r = Client;
        r.Extend(-2);
        Painter()->SetLayer(layer+3);
        Painter()->Image(Screen->GuiAdapter->IconAtlas,0xffffffff,Icon.Uv,r);
    }
    else
    {
        sString<256> KeyBuffer;

        if(Key)
            sKeyName(KeyBuffer,Key);

        if(BackColor)
        {
            Style()->OverrideColorsBack = BackColor;
            Style()->OverrideColors = 1;
        }
        Style()->Rect(layer,this,StyleKind,Client,modifier,Label,LabelLen,KeyBuffer);
    }
}

void sButtonControl::DragClick(const sDragData &dd)
{
    switch(dd.Mode)
    {
    case sDM_Start:
        if(dd.Buttons == 1)
            DragMode = 1;
        break;
    case sDM_Drag:
        DragVerify = Client.Hit(dd.PosX,dd.PosY);
        Update();
        break;
    case sDM_Stop:
        if(DragMode && DragVerify)
        {
            Click();
        }
        DragMode = 0;
        DragVerify = 0;
        Update();
        break;
    default:
        break;
    }
}

void sButtonControl::Click()
{
    int oldval = RefPtr ? *RefPtr : 0;
    switch(ButtonStyle)
    {
    default:
        break;
    case sBS_Toggle:
    case sBS_MenuToggle:
        if((*RefPtr & RefMask)==RefValue)
            *RefPtr = ((*RefPtr) & ~RefMask);
        else
            *RefPtr = ((*RefPtr) & ~RefMask) | RefValue;
        break;
    case sBS_Radio:
    case sBS_MenuRadio:
        *RefPtr = ((*RefPtr) & ~RefMask) | RefValue;
        break;
    }
    ClickMsg.Post();
    if(RefPtr && oldval!=*RefPtr)
    {
        ChangeMsg.Send();
        DoneMsg.Send();
    }
}

void sButtonControl::CmdClick()
{
    Click();
    if(ButtonStyle==sBS_Push)
    {
        SimulatePush = 10;
        Update();
    }
}

/****************************************************************************/
/****************************************************************************/

sBoolToggle::sBoolToggle(bool &ref,const char *on,const char *off)
{
    Ref = &ref;
    Label[0] = on;
    Label[1] = off;

    AddKey("click",sKEY_Space,sGuiMsg(this,&sBoolToggle::CmdClick));
    AddKey("click",sKEY_LMB,sGuiMsg(this,&sBoolToggle::CmdClick));
    AddKey("click",sKEY_Pen,sGuiMsg(this,&sBoolToggle::CmdClick));
    AddHelp();
}

void sBoolToggle::OnCalcSize()
{
    sFont *f = Style()->Font;

    if(FixedSizeX)
        ReqSizeX = FixedSizeX;
    else 
        ReqSizeX = f->CellHeight + sMax(f->GetWidth(Label[0]),f->GetWidth(Label[0]));
    ReqSizeY = f->CellHeight;
}

void sBoolToggle::OnPaint(int layer)
{
    int modifier = 0;

    if(*Ref)
        modifier = sSM_Pressed;
    Style()->Rect(layer,this,sSK_Button_Push,Client,modifier,Label[(*Ref)?1:0]);
}

void sBoolToggle::CmdClick()
{
    *Ref = !*Ref;
}

/****************************************************************************/
/****************************************************************************/

sChoiceManager::sChoiceManager()
{
    HasIcons = false;
}

sChoiceManager::~sChoiceManager()
{
}

int sChoiceManager::SetChoices(const char *t1)
{
    Choices.Clear();
    int val = 0;
    int shift = 0;
    int maxval = 0;
    if(*t1=='*')
    {
        t1++;
        while(sIsDigit(*t1))
            shift = shift*10 + sGetDigit(*t1++);
    }
    while(*t1!=0 && *t1!=':')
    {
        if(sIsDigit(*t1))
        {
            val = 0;
            while(sIsDigit(*t1))
                val = val*10 + sGetDigit(*t1++);
        }
        while(sIsSpace(*t1)) t1++;
        const char *t0 = t1;
        while(*t1!=0 && *t1!='|' && *t1!=':')
            t1++;
        if(t0!=t1)
        {
            AddChoice(val<<shift,t0,int(t1-t0));
        }
        while(*t1=='|')
        {
            t1++;
            val++;
        }
        maxval = sMax(val,maxval);      // maxval should include trailing '|'.
    }
    return int(sMakeMask(uint(maxval))<<shift);
}

void sChoiceManager::AddChoice(int value,const char *label,uptr len)
{
    sChoiceInfo *ci = Choices.Add();
    ci->Value = value;
    ci->Label = label;
    if(len==-1) 
        len = sGetStringLen(label);
    ci->LabelLen = len;
    ci->IconToolTip = 0;
    ci->IconToolTipLen = 0;
    ci->Icon.Init();

    if(ci->Label && ci->LabelLen>0 && ci->Label[0]=='#')
    {
        HasIcons = true;
        Gui->IconManager.GetIcon(ci->Label,ci->LabelLen,ci->Icon,ci->IconToolTip,ci->IconToolTipLen);
    }
}

sChoiceInfo *sChoiceManager::FindChoice(int val)
{
    for(auto &ci : Choices)
    {
        if(ci.Value==val)
            return &ci;
    }
    return 0;
}
int sChoiceManager::GetMaxWidth(sFont *f)
{
    int max = 0;
    for(auto &ci : Choices)
    {
        if(ci.Icon.Loaded)
            max = sMax(max,ci.Icon.Uv.SizeY());
        else
            max = sMax(max,f->GetWidth(ci.Label,ci.LabelLen));
    }
    return max;
}

void sChoiceManager::GetMaxSize(sFont *f,int &sx,int &sy)
{
    sy = f->CellHeight + 4;
    for(auto &ci : Choices)
    {
        if(ci.Icon.Loaded)
        {
            sx = sMax(sx,ci.Icon.Uv.SizeX()+4);
            sy = sMax(sy,ci.Icon.Uv.SizeY()+4);
        }
        else
        {
            sx = sMax(sx,f->GetWidth(ci.Label,ci.LabelLen)+f->CellHeight);
        }
    }
}


sMenuFrame *sChoiceManager::CmdClick(const sGuiMsg &msg_,sWindow *parent)
{
    sMenuFrame *m = new sMenuFrame(parent);
    for(auto &c : Choices)
    {
        sGuiMsg msg(msg_);
        msg.SetCode(c.Value);
        m->AddItem(msg,0,c.Label,c.LabelLen);
    }
    return m;
}

/****************************************************************************/

sChoiceControl::sChoiceControl()
{
    ClassName = "sChoiceControl";

    InternalValue = 0;
    RefPtr = &InternalValue;
    RefMask = ~0U;
    FixedSizeX = 0;
    DragMode = 0;
    DragVerify = 0;

    AddKey("click",sKEY_Space,sGuiMsg(this,&sChoiceControl::CmdClick));
    AddKey("click",sKEY_LMB,sGuiMsg(this,&sChoiceControl::CmdClick));
    AddKey("click",sKEY_Pen,sGuiMsg(this,&sChoiceControl::CmdClick));
    AddHelp();
}

sChoiceControl::sChoiceControl(int *ref,const char *str)
{
    InternalValue = 0;
    RefPtr = ref;
    RefMask = ~0U;
    FixedSizeX = 0;
    DragMode = 0;
    DragVerify = 0;

    AddKey("click",sKEY_Space,sGuiMsg(this,&sChoiceControl::CmdClick));
    AddKey("click",sKEY_LMB,sGuiMsg(this,&sChoiceControl::CmdClick));
    AddKey("click",sKEY_Pen,sGuiMsg(this,&sChoiceControl::CmdClick));
    AddHelp();

    SetChoices(str);
}

void sChoiceControl::SetChoices(const char *t1)
{
    RefMask = Choices.SetChoices(t1);
    if(Choices.HasIcons)
        Flags |= sWF_GiveSpace;
}

void sChoiceControl::AddChoice(int value,const char *label,int len)
{
    Choices.AddChoice(value,label,len);
    if(Choices.HasIcons)
        Flags |= sWF_GiveSpace;
}

void sChoiceControl::OnCalcSize()
{
    sFont *f = Style()->Font;
    Choices.GetMaxSize(f,ReqSizeX,ReqSizeY);
    if(FixedSizeX)
        ReqSizeX = FixedSizeX;
}

void sChoiceControl::OnPaint(int layer)
{
    sString<sFormatBuffer> buffer;
    const char *label = "???";
    uptr labellen = 3;
    int modifier = DragVerify ? sSM_Pressed : 0;
    sChoiceInfo *c = Choices.FindChoice((*RefPtr)&RefMask);
    if(c)
    {
        label = c->Label;
        labellen = c->LabelLen;
        if(sCmpStringLen("-",c->Label,c->LabelLen)==0 && Choices.Choices.GetCount()>=2)
        {
            c = &Choices.Choices[1];
            modifier |= sSM_Grayed;
            buffer.PrintF("(");
            buffer.Add(c->Label,c->LabelLen);
            buffer.Add(")");
            label = buffer;
            labellen = -1;
        }
    }

    if(c && c->Icon.Loaded)
    {
        Style()->Rect(layer,this,sSK_Button_Push,Client,modifier,"",0,0);
        sRect r = Client;
        r.Extend(-2);
        Painter()->SetLayer(layer+3);
        Painter()->Image(Screen->GuiAdapter->IconAtlas,0xffffffff,c->Icon.Uv,r);
        if(c->IconToolTipLen>0)
        {
            ToolTipText = c->IconToolTip;
            ToolTipTextLen = c->IconToolTipLen;
        }
    }
    else
    {
        Style()->Rect(layer,this,sSK_Button_Push,Client,modifier,label,labellen,0);
    }
}


void sChoiceControl::DragClick(const sDragData &dd)
{
    switch(dd.Mode)
    {
    case sDM_Start:
        if(dd.Buttons == 1)
            DragMode = 1;
        break;
    case sDM_Drag:
        DragVerify = Client.Hit(dd.PosX,dd.PosY);
        break;
    case sDM_Stop:
        if(DragMode && DragVerify)
        {
            CmdClick();
        }
        DragMode = 0;
        DragVerify = 0;
        break;
    default:
        break;
    }
}

void sChoiceControl::CmdClick()
{
    if(Choices.Choices.GetCount()==2)
    {
        int val = (*RefPtr) & RefMask;
        if(Choices.Choices[0].Value == val)
            SetValue(Choices.Choices[1].Value);
        else
            SetValue(Choices.Choices[0].Value);
    }
    else
    {
        Choices.CmdClick(sGuiMsg(this,&sChoiceControl::SetValue,0),this)->StartMenu();
    }
}

void sChoiceControl::SetValue(int n)
{
    int old = *RefPtr;
    *RefPtr = ((*RefPtr) & (~RefMask)) | (n & RefMask); 
    if(old!=*RefPtr)
    {
        ChangeMsg.Send();
        DoneMsg.Send();
    }
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sStringEditorUndo::sStringEditorUndo() : Pool(0x10000)
{
    UndoIndex = 0;
    InsertCursor = 0;
    InsertUsed = 0;
    DeleteCursor = 0;
    DeleteUsed = 0;
    Immune = false;
}

void sStringEditorUndo::Insert(uptr cursor,const char *newtext,uptr len)
{
    if(Immune) return;
    sASSERT(len>0);

    // discontinuity - can not append

    if(InsertCursor+InsertUsed!=cursor || DeleteUsed)
        Flush();

    // first try to append to insert buffer

    if(len+InsertUsed+1 < uptr(Buffer.Size()) && (InsertCursor+InsertUsed==cursor || InsertUsed==0))
    {
        if(InsertUsed==0)
            InsertCursor = cursor;
        for(uptr i=0;i<len;i++)
            Buffer[InsertUsed++] = *newtext++;
        Buffer[InsertUsed] = 0;
        return;
    }

    // otherwise insert directly

    Flush();
    Edits.SetSize(UndoIndex+1);
    Edits[UndoIndex].Kind = EK_Insert;
    Edits[UndoIndex].Cursor = cursor;
    Edits[UndoIndex].Text = newtext;
    Edits[UndoIndex].Len = len;
    UndoIndex++;
}

void sStringEditorUndo::Delete(uptr cursor,const char *oldtext,uptr len)
{
    if(Immune) return;
    sASSERT(len>0);

    // discontinuity - can not append

    if((DeleteCursor != cursor+len && DeleteCursor!=cursor) || InsertUsed)
        Flush();

    // first try to append to Delete buffer
    
    if(len+DeleteUsed+1 < uptr(Buffer.Size()) && (DeleteCursor==cursor))
    {
        if(DeleteUsed==0)
            DeleteCursor = cursor;
        for(uptr i=0;i<len;i++)
            Buffer[DeleteUsed++] = *oldtext++;
        Buffer[DeleteUsed] = 0;
        return;
    }
    
    if(len+DeleteUsed+1 < uptr(Buffer.Size()) && (DeleteCursor == cursor+len || DeleteUsed==0))
    {
        if(DeleteUsed==0)
            DeleteCursor = cursor;
        for(int i=DeleteUsed-1;i>=0;i--)
            Buffer[i+len] = Buffer[i];
        for(uptr i=0;i<len;i++)
            Buffer[i] = *oldtext++;
        DeleteUsed += len;
        Buffer[DeleteUsed] = 0;
        DeleteCursor = cursor;
        return;
    }

    // otherwise insert directly

    Flush();
    Edits.SetSize(UndoIndex+1);
    Edits[UndoIndex].Kind = EK_Delete;
    Edits[UndoIndex].Cursor = cursor;
    Edits[UndoIndex].Text = Pool.AllocString(oldtext,len);
    Edits[UndoIndex].Len = len;
    UndoIndex++;
}

void sStringEditorUndo::Flush()
{
    sASSERT(!Immune);
    if(InsertUsed>0)
    {
        Edits.SetSize(UndoIndex+1);
        Edits[UndoIndex].Kind = EK_Insert;
        Edits[UndoIndex].Cursor = InsertCursor;
        Edits[UndoIndex].Text = Pool.AllocString(Buffer,InsertUsed);
        Edits[UndoIndex].Len = InsertUsed;
        UndoIndex++;
    }
    if(DeleteUsed>0)
    {
        Edits.SetSize(UndoIndex+1);
        Edits[UndoIndex].Kind = EK_Delete;
        Edits[UndoIndex].Cursor = DeleteCursor;
        Edits[UndoIndex].Text = Pool.AllocString(Buffer,DeleteUsed);
        Edits[UndoIndex].Len = DeleteUsed;
        UndoIndex++;
    }
    InsertUsed = 0;
    InsertCursor = 0;
    DeleteUsed = 0;
    DeleteCursor = 0;
}

void sStringEditorUndo::Reset()
{
    sASSERT(!Immune);
    Edits.Clear();
    UndoIndex = 0;
}

void sStringEditorUndo::Undo(sStringEditorHelper *editor)
{
    sASSERT(!Immune);
    Flush();
    if(UndoIndex>0)
    {
        Immune = true;
        const auto &e = Edits[--UndoIndex];
        if(e.Kind==EK_Insert)
        {
            editor->Delete(e.Cursor,e.Cursor+e.Len);
            editor->SetCursor(e.Cursor);
        }
        if(e.Kind==EK_Delete)
        {
            editor->Insert(e.Cursor,e.Text,e.Len);
            editor->SetCursor(e.Cursor+e.Len);
        }
        Immune = false;
    }
}

void sStringEditorUndo::Redo(sStringEditorHelper *editor)
{
    sASSERT(!Immune);
    Flush();
    if(Edits.IsIndex(UndoIndex))
    {
        Immune = true;
        const auto &e = Edits[UndoIndex++];
        if(e.Kind==EK_Insert)
        {
            editor->Insert(e.Cursor,e.Text,e.Len);
            editor->SetCursor(e.Cursor+e.Len);
        }
        if(e.Kind==EK_Delete)
        {
            editor->Delete(e.Cursor,e.Cursor+e.Len);
            editor->SetCursor(e.Cursor);
        }
        Immune = false;
    }
}

/****************************************************************************/
/****************************************************************************/

sStringEditorHelper::sStringEditorHelper()
{
    Static = false;
    Cursor = 0;
    Overwrite = 0;
    SelectMode = 0;
    SelectStart = 0;
    HasChanged = 0;

    Window = 0;
    Multiline = 0;
    UpDownX = -1;
    Tabsize = 4;

    OwnUndo = 0;
    Undo = 0;
}

sStringEditorHelper::~sStringEditorHelper()
{
    delete OwnUndo;
}

void sStringEditorHelper::Update()
{
    Window->Update();
}

void sStringEditorHelper::Change()
{
    ValidateMsg.Send();
    UpdateTargetMsg.Send();
    HasChanged = 1;
    if(Window) 
    {
        Window->ChangeMsg.Send();
    }
    sASSERT(Cursor>=0 && Cursor<=GetBufferUsed());
}

void sStringEditorHelper::ChangeDone()
{
    if(HasChanged)
    {
        Window->DoneMsg.Send();
        HasChanged = 0;
    }
}

void sStringEditorHelper::NewText()
{
    Cursor = 0;
    SelectMode = 0;
    Undo = 0;
}

void sStringEditorHelper::SetCursor(uptr cur)
{
    if(cur<GetBufferUsed())
    {
        Cursor = cur;
        ScrollToCursor();
        SelectMode = 0;
        Update();
    }
}

void sStringEditorHelper::SetSelect(uptr t0, uptr t1)
{
    if(t0<GetBufferUsed() && t1<GetBufferUsed())
    {
        SelectStart = t0;
        Cursor = t1;
        SelectMode = 1;
        ScrollToCursor();
        Update();
    }
}

void sStringEditorHelper::Register(sControl *w)
{
    Window = w;
    w->AddAllKeys("...",sGuiMsg(this,&sStringEditorHelper::CmdKey));

    w->AddKey("Tab"          ,sKEY_Tab     ,sGuiMsg(this,&sStringEditorHelper::CmdTab));
    w->AddKey("Untab"        ,sKEY_Tab|sKEYQ_Shift     ,sGuiMsg(this,&sStringEditorHelper::CmdUntab));

    w->AddKey("left"         ,sKEY_Left     ,sGuiMsg(this,&sStringEditorHelper::CmdLeft,0));
    w->AddKey("right"        ,sKEY_Right    ,sGuiMsg(this,&sStringEditorHelper::CmdRight,0));
    w->AddKey("word left"    ,sKEYQ_Ctrl|sKEY_Left     ,sGuiMsg(this,&sStringEditorHelper::CmdWordLeft,0));
    w->AddKey("word right"   ,sKEYQ_Ctrl|sKEY_Right    ,sGuiMsg(this,&sStringEditorHelper::CmdWordRight,0));
    w->AddKey("begin of line",sKEY_Home     ,sGuiMsg(this,&sStringEditorHelper::CmdLineBegin,0));
    w->AddKey("end of line"  ,sKEY_End      ,sGuiMsg(this,&sStringEditorHelper::CmdLineEnd,0));

    w->AddKey("select left" ,sKEYQ_Shift|sKEY_Left ,sGuiMsg(this,&sStringEditorHelper::CmdLeft,1));
    w->AddKey("select right",sKEYQ_Shift|sKEY_Right,sGuiMsg(this,&sStringEditorHelper::CmdRight,1));
    w->AddKey("select word left"    ,sKEYQ_Shift|sKEYQ_Ctrl|sKEY_Left     ,sGuiMsg(this,&sStringEditorHelper::CmdWordLeft,1));
    w->AddKey("select word right"   ,sKEYQ_Shift|sKEYQ_Ctrl|sKEY_Right    ,sGuiMsg(this,&sStringEditorHelper::CmdWordRight,1));
    w->AddKey("select begin of line",sKEYQ_Shift|sKEY_Home     ,sGuiMsg(this,&sStringEditorHelper::CmdLineBegin,1));
    w->AddKey("select end of line"  ,sKEYQ_Shift|sKEY_End      ,sGuiMsg(this,&sStringEditorHelper::CmdLineEnd,1));

    w->AddKey("insert"      ,sKEY_Insert   ,sGuiMsg(this,&sStringEditorHelper::CmdInsert));
    w->AddKey("delete"      ,sKEY_Delete   ,sGuiMsg(this,&sStringEditorHelper::CmdDelete));
    w->AddKey("backspace"   ,sKEY_Backspace,sGuiMsg(this,&sStringEditorHelper::CmdBackspace));

    w->AddKey("select word" ,sKEYQ_Double|sKEY_LMB,sGuiMsg(this,&sStringEditorHelper::CmdSelectWord));
    w->AddKey("select all"  ,sKEYQ_Ctrl|'a',sGuiMsg(this,&sStringEditorHelper::CmdSelectAll));
    w->AddKey("cut"         ,sKEYQ_Ctrl|'x',sGuiMsg(this,&sStringEditorHelper::CmdCut));
    w->AddKey("copy"        ,sKEYQ_Ctrl|'c',sGuiMsg(this,&sStringEditorHelper::CmdCopy));
    w->AddKey("paste"       ,sKEYQ_Ctrl|'v',sGuiMsg(this,&sStringEditorHelper::CmdPaste));

    w->AddKey("undo"        ,sKEYQ_Ctrl|'z',sGuiMsg(this,&sStringEditorHelper::CmdUndo));
    w->AddKey("redo"        ,sKEYQ_Ctrl|sKEYQ_Shift|'Z',sGuiMsg(this,&sStringEditorHelper::CmdRedo));

    if(Multiline)
    {
        w->AddKey("newline"   ,sKEY_Enter    ,sGuiMsg(this,&sStringEditorHelper::CmdEnter));

        w->AddKey("up"        ,sKEY_Up       ,sGuiMsg(this,&sStringEditorHelper::CmdUp,0));
        w->AddKey("down"      ,sKEY_Down     ,sGuiMsg(this,&sStringEditorHelper::CmdDown,0));
        w->AddKey("page up"      ,sKEY_PageUp       ,sGuiMsg(this,&sStringEditorHelper::CmdPageUp,0));
        w->AddKey("page down"   ,sKEY_PageDown     ,sGuiMsg(this,&sStringEditorHelper::CmdPageDown,0));
        w->AddKey("select up"   ,sKEYQ_Shift|sKEY_Up     ,sGuiMsg(this,&sStringEditorHelper::CmdUp,1));
        w->AddKey("select down" ,sKEYQ_Shift|sKEY_Down   ,sGuiMsg(this,&sStringEditorHelper::CmdDown,1));
        w->AddKey("select page up"     ,sKEYQ_Shift|sKEY_PageUp       ,sGuiMsg(this,&sStringEditorHelper::CmdPageUp,1));
        w->AddKey("select page down"   ,sKEYQ_Shift|sKEY_PageDown     ,sGuiMsg(this,&sStringEditorHelper::CmdPageDown,1));

        w->AddKey("begin of text",sKEYQ_Ctrl|sKEY_Home     ,sGuiMsg(this,&sStringEditorHelper::CmdTextBegin,0));
        w->AddKey("end of text"  ,sKEYQ_Ctrl|sKEY_End      ,sGuiMsg(this,&sStringEditorHelper::CmdTextEnd,0));
        w->AddKey("begin of text",sKEYQ_Shift|sKEYQ_Ctrl|sKEY_Home     ,sGuiMsg(this,&sStringEditorHelper::CmdTextBegin,1));
        w->AddKey("end of text"  ,sKEYQ_Shift|sKEYQ_Ctrl|sKEY_End      ,sGuiMsg(this,&sStringEditorHelper::CmdTextEnd,1));
    }
    else
    {
        w->AddKey("done"        ,sKEY_Enter    ,sGuiMsg(this,&sStringEditorHelper::CmdDone));
    }
}

/****************************************************************************/

bool sStringEditorHelper::Left()
{
    if(Cursor>0)
    {
        const char *b = GetBuffer();
        Cursor--;
        while(Cursor>0 && (b[Cursor]&0xc0)==0x80)
            Cursor--;
        ScrollToCursor();
        return 1;
    }
    return 0;
}

bool sStringEditorHelper::Right()
{
    const char *b = GetBuffer();
    if(b[Cursor]!=0)
    {
        Cursor++;
        while((b[Cursor]&0xc0)==0x80)
            Cursor++;
        ScrollToCursor();
        return 1;
    }
    return 0;
}

uptr sStringEditorHelper::GetCursorX()
{
    const char *b = GetBuffer();
    uptr c = Cursor;
    while(c>0 && b[c-1]!='\n') c--;
    return Cursor-c;
}

/****************************************************************************/

bool sStringEditorHelper::GetSelection(uptr &t0,uptr &t1)
{
    if(SelectMode && SelectStart!=Cursor)
    {
        t0 = sMin(SelectStart,Cursor);
        t1 = sMax(SelectStart,Cursor);
        return 1;
    }
    else
    {
        t0 = t1 = 0;
        return 0;
    }
}

void sStringEditorHelper::Select0(int select)
{
    if(select)
    {
        if(SelectMode==0)
        {
            SelectMode = 1;
            SelectStart = Cursor;
        }
    }
    else
    {
        SelectMode = 0;
    }
}

void sStringEditorHelper::AllocUndo()
{
    if(!Undo)
    {
        if(!OwnUndo)
            OwnUndo = new sStringEditorUndo;
        Undo = OwnUndo;
        Undo->Reset();
    }
}

/****************************************************************************/

void sStringEditorHelper::CmdKey(const sKeyData &kd)
{
    if((kd.Key & sKEYQ_Break)) return;
    if((kd.Key & sKEYQ_Ctrl)) return;

    uint key = kd.Key & sKEYQ_Mask;
    if(key<0x20 || (key>=0xe000 && key<=0xffff)) return;

    UpDownX = -1;
    uptr t0,t1;

    if(GetSelection(t0,t1))
    {
        Delete(t0,t1);
        if(Insert(Cursor,key))
            Right();
    }
    else
    {
        if(Overwrite && GetBuffer()[Cursor]!=0 && GetBuffer()[Cursor]!='\n')
        {
            if(Cursor<GetBufferSize())
            {
                if(Insert(Cursor,key))
                {
                    Right();
                    Delete(Cursor);
                }
            }
        }
        else
        {
            if(Insert(Cursor,key))
            {
                Right();
            }
        }
    }
    SelectMode = 0;
    Change();
    ScrollToCursor();
    Update();
}

void sStringEditorHelper::CmdTab()
{
    if(!SelectMode)
    {
        Insert(Cursor,' ');
        Right();
        uptr x = GetCursorX();
        while(x % Tabsize!=0)
        {
            Insert(Cursor,' ');
            Right();
            x++;
        }
    }
    else
    {
        uptr t0;
        uptr t1;
        if(GetSelection(t0,t1))
        {
            // find first selected line
            while(t0>0 && GetBuffer()[t0-1]!='\n')
                t0--;

            uptr start = t0;

            while(t0<=t1)
            {
                // insert tab

                for(int i=0;i<Tabsize;i++)
                {
                    Insert(t0++,' ');
                    Cursor++;
                    t1++;
                }

                // next line

                while(t0<=t1 && GetBuffer()[t0]!='\n')
                    t0++;
                if(GetBuffer()[t0]=='\n')
                    t0++;
            }
        }
    }
    Change();
}

void sStringEditorHelper::CmdUntab()
{
    if(!SelectMode)
    {
        uptr x = GetCursorX();
        if(x>0 && GetBuffer()[Cursor-1]==' ')
        {
            CmdBackspace();
            x--;
            while(x>0 && x % Tabsize!=0 && GetBuffer()[Cursor-1]==' ')
            {
                CmdBackspace();
                x--;
            }
        }
    }
    else
    {
        uptr t0;
        uptr t1;
        if(GetSelection(t0,t1))
        {
            // find first selected line
            while(t0>0 && GetBuffer()[t0-1]!='\n')
                t0--;

            uptr start = t0;

            while(t0<=t1)
            {
                // remove spaces

                for(int i=0;i<Tabsize;i++)
                {
                    if(GetBuffer()[t0]==' ')
                        Delete(t0);
                    t1--;
                }

                // next line

                while(t0<=t1 && GetBuffer()[t0]!='\n')
                    t0++;
                if(GetBuffer()[t0]=='\n')
                    t0++;
            }
        }
    }
    Change();
}

void sStringEditorHelper::CmdLeft(int select)
{
    UpDownX = -1;
    Select0(select);
    Left();
    Update();
}

void sStringEditorHelper::CmdRight(int select)
{
    UpDownX = -1;
    Select0(select);
    Right();
    Update();
}


void sStringEditorHelper::Up(int n)
{
    const char *b = GetBuffer();

    for(int i=0;i<n;i++)
    {
        uptr x = GetCursorX();
        uptr c = Cursor;
        c -= x;
        if(UpDownX==-1)
            UpDownX = x ;
        else
            x = UpDownX;
        sASSERT(c==0 || b[c-1]=='\n');
        if(c>0)
        {
            c--;
            while(c>0 && b[c-1]!='\n')
                c--;
            while(x>0 && b[c]!=0 && b[c]!='\n')
            {
                c++;
                x--;
            }
            Cursor = c;
        }
    }
}

void sStringEditorHelper::CmdUp(int select)
{
    Select0(select);

    Up(1);

    ScrollToCursor();
    Update();
}

void sStringEditorHelper::Down(int n)
{
    const char *b = GetBuffer();
    uptr x = GetCursorX();
    if(UpDownX==-1)
        UpDownX = x;
    else
        x = UpDownX;

    for(int i=0;i<n;i++)
    {
        uptr c = Cursor;
        while(b[c]!=0 && b[c]!='\n') 
            c++;
        if(b[c]=='\n')
        {
            c++;
            while(x>0 && b[c]!=0 && b[c]!='\n')
            {
                c++;
                x--;
            }
            Cursor = c;
        }
    }
}

void sStringEditorHelper::CmdDown(int select)
{
    Select0(select);

    Down(1);

    ScrollToCursor();
    Update();
}

void sStringEditorHelper::CmdWordLeft(int select)
{
    UpDownX = -1;
    Select0(select);

    const char *b = GetBuffer();
    while(Cursor>0 && sIsSpace(b[Cursor-1]))
        Cursor--;
    while(Cursor>0 && !sIsSpace(b[Cursor-1]))
        Cursor--;

    ScrollToCursor();
    Update();
}

void sStringEditorHelper::CmdWordRight(int select)
{
    UpDownX = -1;
    Select0(select);

    const char *b = GetBuffer();
    while(b[Cursor]!=0 && sIsSpace(b[Cursor-1]))
        Cursor++;
    while(b[Cursor]!=0 && !sIsSpace(b[Cursor-1]))
        Cursor++;

    ScrollToCursor();
    Update();
}

void sStringEditorHelper::CmdPageUp(int select)
{
    Select0(select);

    Up(GetLinesInPage());

    ScrollToCursor();
    Update();
}

void sStringEditorHelper::CmdPageDown(int select)
{
    Select0(select);

    Down(GetLinesInPage());

    ScrollToCursor();
    Update();
}

void sStringEditorHelper::CmdTextBegin(int select)
{
    UpDownX = -1;
    Select0(select);
    Cursor = 0;
    ScrollToCursor();
    Update();
}

void sStringEditorHelper::CmdTextEnd(int select)
{
    UpDownX = -1;
    Select0(select);
    const char *b = GetBuffer();
    Cursor = sGetStringLen(b);
    ScrollToCursor();
    Update();
}


void sStringEditorHelper::CmdLineBegin(int select)
{
    UpDownX = -1;
    Select0(select);
    const char *b = GetBuffer();
    while(Cursor>0 && b[Cursor-1]!='\n') 
        Cursor--;
    ScrollToCursor();
    Update();
}

void sStringEditorHelper::CmdLineEnd(int select)
{
    UpDownX = -1;
    Select0(select);
    const char *b = GetBuffer();
    while(b[Cursor]!='\n' && b[Cursor]!=0) 
        Cursor++;
    ScrollToCursor();
    Update();
}


void sStringEditorHelper::CmdInsert()
{
    UpDownX = -1;
    Overwrite = !Overwrite;
    Update();
}

void sStringEditorHelper::CmdDelete()
{
    UpDownX = -1;
    uptr t0,t1;
    if(GetSelection(t0,t1))
    {
        Delete(t0,t1);
    }
    else
    {
        Delete(Cursor);
    }
    SelectMode = 0;
    Change();
    ScrollToCursor();
    Update();
}

void sStringEditorHelper::CmdBackspace()
{
    UpDownX = -1;
    uptr t0,t1;
    if(GetSelection(t0,t1))
    {
        Delete(t0,t1);
    }
    else
    {
        if(Left())
        {
            Delete(Cursor);
        }
    }
    SelectMode = 0;
    Change();
    ScrollToCursor();
    Update();
}

void sStringEditorHelper::CmdEnter()
{
    UpDownX = -1;

    uptr x = GetCursorX();
    int tab = 0;
    while(GetBuffer()[Cursor-x+tab]==' ') tab++;

    Insert(Cursor++,'\n');
    for(int i=0;i<tab;i++)
        Insert(Cursor++,' ');
    Change();
    ScrollToCursor();
    Update();
}

void sStringEditorHelper::CmdDone()
{
    DoneMsg.Post();
}

void sStringEditorHelper::CmdUndo()
{
    if(Undo)
        Undo->Undo(this);
}

void sStringEditorHelper::CmdRedo()
{
    if(Undo)
        Undo->Redo(this);
}

/****************************************************************************/

void sStringEditorHelper::CmdSelectAll()
{
    UpDownX = -1;
    SelectMode = 1;
    SelectStart = 0;
    const char *b = GetBuffer();
    Cursor = sGetStringLen(b);
    Update();
}

void sStringEditorHelper::CmdSelectWord()
{
    uptr t0 = Cursor;
    uptr t1 = Cursor;
    const char *b = GetBuffer();
    while(t0>0 && !sIsSpace(b[t0-1])) t0--;
    while(b[t1] && !sIsSpace(b[t1])) t1++;

    SetSelect(t0,t1);
}
void sStringEditorHelper::CmdCut()
{
    UpDownX = -1;
    uptr t0,t1;
    if(GetSelection(t0,t1))
    {
        const char *b = GetBuffer();
        sClipboard->SetText(b + t0,t1-t0);
        Delete(t0,t1);
        Change();
        Cursor = t0;
        SelectMode = 0;
    }
    Update();
}

void sStringEditorHelper::CmdCopy()
{
    UpDownX = -1;
    uptr t0,t1;
    if(GetSelection(t0,t1))
    {
        const char *b = GetBuffer();
        sClipboard->SetText(b + t0,t1-t0);
    }
}

void sStringEditorHelper::CmdPaste()
{
    UpDownX = -1;
    uptr t0,t1;
    if(GetSelection(t0,t1))
    {
        Delete(t0,t1);
    }

    char *paste = sClipboard->GetText();
    if(paste)
    {
        Insert(Cursor,paste);
        Cursor += sGetStringLen(paste);
        delete[] paste;
    }
    Select0(false);
    Change();
    Update();
}

/****************************************************************************/

sStringControlEditorHelper::sStringControlEditorHelper(sStringDesc &desc) : Desc(desc)
{
}

void sStringControlEditorHelper::Delete(uptr pos)
{
    uptr n = 1;
    const char *b = GetBuffer();
    if(b[pos])
    {
        while((b[pos+n]&0xc0)==0x80) n++;
        Delete(pos,pos+n);
    }
}

void sStringControlEditorHelper::Delete(uptr start,uptr end)
{
    const char *b = GetBuffer();
    uptr len = sGetStringLen(b);
    uptr n = end-start;
    sASSERT(n>=0);
    sASSERT(start<=len);
    sASSERT(end<=len);

    for(uptr i=start;i<len-n+1;i++)
        Desc.Buffer[i] = b[i+n];

    SelectStart = Cursor = start;
    SelectMode = 0;
}

bool sStringControlEditorHelper::Insert(uptr pos,int c)
{
    sString<8> cc;
    cc.PrintF("%c",c);
    return Insert(pos,cc);
}

bool sStringControlEditorHelper::Insert(uptr pos,const char *cc,uptr n)
{
    if(n==-1)
        n = sGetStringLen(cc);
    char *b = Desc.Buffer;
    uptr len = sGetStringLen(b);

    if(len+n<GetBufferSize())
    {
        for(uptr i=len+n;i>=pos+n;i--)
            b[i] = b[i-n];

        for(uptr i=0;i<n;i++)
            b[pos+i] = cc[i];
        return 1;
    }
    return 0;
}

void sStringControlEditorHelper::SetCursor(uptr pos)
{
    Cursor = sMin(pos,GetBufferUsed());
}

/****************************************************************************/
/****************************************************************************/

sStringControl::sStringControl() : Editor(Desc)
{
    ClassName = "sStringControl";
    Desc.Buffer = (char *)"";
    Desc.Size = 0;

    Construct();
}

sStringControl::sStringControl(const sStringDesc &desc) : Editor(Desc)
{
    ClassName = "sStringControl";
    Desc = desc;

    Construct();
}


void sStringControl::Construct()
{
    FixedSizeX = 0;
    StyleScroll = 0;
    Validated = 1;
    BackColorPtr = 0;
    BackColorFloatPtr = 0;
    Flags |= sWF_Keyboard;

    AddDrag("click",sKEY_LMB,sGuiMsg(this,&sStringControl::DragClick));

    Editor.Register(this);
    Editor.DoneMsg = sGuiMsg(this,&sStringControl::CmdEnter);
    // this does not work, because sometimes the window is deleted immediately.
    //  sGuiMsg msg(&Editor,&sStringControlEditorHelper::CmdAll);
    //  msg.Post();
    AddHelp();
}

void sStringControl::SetBuffer(const sStringDesc &desc)
{
    Desc = desc;
    Update();
}

void sStringControl::SetText(const char *text)
{
    sCopyString(Desc,text);
    Update();
}

void sStringControl::OnLeaveFocus()
{
    Editor.ChangeDone();
}

void sStringControl::CmdStartEditing()
{
    Editor.CmdTextEnd(0);
    Editor.CmdSelectAll();
}

void sStringControl::CmdEnter()
{
    DoneMsg.Send();
}

/****************************************************************************/

void sStringControl::OnCalcSize()
{
    sFont *f = Style()->Font;
    ReqSizeX = FixedSizeX;
    ReqSizeY = f->CellHeight+4;
}

void sStringControl::OnPaint(int layer)
{
    Style()->CursorPos = Editor.Cursor;
    Style()->CursorOverwrite = Editor.Overwrite;
    if(Editor.SelectMode)
    {
        Style()->SelectStart = Editor.SelectStart;
        Style()->SelectEnd = Editor.Cursor;
    }
    else
    {
        Style()->SelectStart = 0;
        Style()->SelectEnd = 0;
    }

    int modifier = 0;
    if(DragMode) modifier |= sSM_Pressed;
    if(!Validated) modifier |= sSM_Error;
    if(BackColorPtr || BackColorFloatPtr)
    {
        uint col;
        if(BackColorPtr)
            col = *BackColorPtr;
        if(BackColorFloatPtr)
            col = BackColorFloatPtr->GetColor();
        Style()->OverrideColors = 1;
        Style()->OverrideColorsBack = col | 0xff000000;
        int amp = ((col>>16)&255) + ((col>>8)&255) + ((col>>0)&255);
        Style()->OverrideColorsText = amp < 256*3/2 ? 0xffffffff : 0xff000000;
    }
    Style()->CursorScroll = StyleScroll;
    Style()->Rect(layer,this,sSK_String,Client,modifier,Desc.Buffer,-1);
    StyleScroll = Style()->CursorScroll;
}

void sStringControl::DragClick(const sDragData &dd)
{
    switch(dd.Mode)
    {
    case sDM_Start:
        Editor.SelectStart = Editor.Cursor = FindClick(dd.StartX);
        Editor.SelectMode = 0;
        DragMode = 1;
        Update();
        break;
    case sDM_Drag:
        Editor.Cursor = FindClick(dd.PosX);
        if(Editor.SelectStart!=Editor.Cursor)
            Editor.SelectMode = 1;
        Update();
        break;
    case sDM_Stop:
        DragMode = 0;
        break;
    default:
        break;            
    }
}


uptr sStringControl::FindClick(int xpos)
{
    int x0 = Client.x0 + StyleScroll + Style()->Font->CellHeight/2;
    int x1;

    if(xpos<=x0) return 0;
    const char *str = Desc.Buffer;
    while(*str)
    {
        const char *oldstr = str;
        int c = sReadUTF8(str);
        int w = Style()->Font->GetWidth(c);
        x1 = x0+w;

        int xm = (x0+x1)/2;
        if(xpos>=x0 && xpos<xm) return oldstr-Desc.Buffer;
        else if(xpos>=xm && xpos<x1) return str-Desc.Buffer;

        x0 = x1;
    }

    return str-Desc.Buffer;
}

/****************************************************************************/

sPoolStringControl::sPoolStringControl() : sStringControl(Buffer)
{
    PoolString = 0;
    Editor.UpdateTargetMsg = sGuiMsg(this,&sPoolStringControl::UpdateTarget);
}

sPoolStringControl::sPoolStringControl(sPoolString &p) : sStringControl(Buffer)
{
    SetBuffer(p);
    Editor.UpdateTargetMsg = sGuiMsg(this,&sPoolStringControl::UpdateTarget);
}

void sPoolStringControl::SetBuffer(sPoolString &p)
{
    PoolString = &p;
    Buffer = p;
}

void sPoolStringControl::UpdateTarget()
{
    if(PoolString)
        *PoolString = Editor.GetBuffer();
}


/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sDropControl::sDropControl()
{
    Label = 0;
    StoreIcon = 0;
}

sDropControl::~sDropControl()
{
}

void sDropControl::Set(const sGuiMsg &msg,const char *label,sDragDropIcon *storeicon)
{
    DoneMsg = msg;
    Label = label;
    StoreIcon = storeicon;
}

void sDropControl::OnPaint(int layer)
{
    sDragDropIcon icon;
    int mx,my;
    int modify = 0;

    if(Gui->GetDragDropInfo(this,icon,mx,my) && Client.Hit(mx,my) && HandleDragDrop(icon,mx,my,true))
        modify = sSM_Pressed;

    Style()->Rect(layer,this,sSK_Button_Drop,Client,modify,Label);
}

bool sDropControl::OnDragDropTo(const sDragDropIcon &icon,int &mx,int &my,bool checkonly)
{
    if(Client.Hit(mx,my))
        return HandleDragDrop(icon,mx,my,checkonly);
    return false;
}


/****************************************************************************/
/****************************************************************************/


/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/
}
