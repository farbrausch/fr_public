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
/***   The Message Class                                                  ***/
/***                                                                      ***/
/****************************************************************************/

void sGuiMsg::Send()
{
    if(Target)
    {
        switch(Type)
        {
        case 0x00: break;
        case 0x01: (Target->*Func01)(); break;
        case 0x02: (Target->*Func02)(Code); break;
        case 0x03: (Target->*Func03)(Data); break;
        default: sASSERT0();
        }
    }
    else 
    {
        switch(Type)
        {
        case 0x00: break;
        case 0x31: (*Func31)(); break;
        case 0x32: (*Func32)(Code); break;
        case 0x33: (*Func33)(Data); break;
        default: sASSERT0();
        }
    }
}

void sGuiMsg::Send(const sDragData &dd)
{
    if(Target)
    {
        switch(Type)
        {
        case 0x00: break;
        case 0x01: (Target->*Func01)(); break;
        case 0x02: (Target->*Func02)(Code); break;
        case 0x03: (Target->*Func03)(Data); break;
        case 0x11: (Target->*Func11)(dd); break;
        case 0x12: (Target->*Func12)(dd,Code); break;
        case 0x13: (Target->*Func13)(dd,Data); break;
        default: sASSERT0();
        }
    }
    else 
    {
        switch(Type)
        {
        case 0x00: break;
        case 0x31: (*Func31)(); break;
        case 0x32: (*Func32)(Code); break;
        case 0x33: (*Func33)(Data); break;
        case 0x41: (*Func41)(dd); break;
        case 0x42: (*Func42)(dd,Code); break;
        case 0x43: (*Func43)(dd,Data); break;
        default: sASSERT0();
        }
    }
}

void sGuiMsg::Send(const sKeyData &kd)
{
    if(Target)
    {
        switch(Type)
        {
        case 0x00: break;
        case 0x01: (Target->*Func01)(); break;
        case 0x02: (Target->*Func02)(Code); break;
        case 0x03: (Target->*Func03)(Data); break;
        case 0x21: (Target->*Func21)(kd); break;
        case 0x22: (Target->*Func22)(kd,Code); break;
        case 0x23: (Target->*Func23)(kd,Data); break;
        default: sASSERT0();
        }
    } 
    else 
    {
        switch(Type)
        {
        case 0x00: break;
        case 0x31: (*Func31)(); break;
        case 0x32: (*Func32)(Code); break;
        case 0x33: (*Func33)(Data); break;
        case 0x51: (*Func51)(kd); break;
        case 0x52: (*Func52)(kd,Code); break;
        case 0x53: (*Func53)(kd,Data); break;
        default: sASSERT0();
        }
    }
}

void sGuiMsg::SendCode(int code_)
{
    if(Target)
    {
        switch(Type)
        {
        case 0x00: break;
        case 0x01: (Target->*Func01)(); break;
        case 0x02: (Target->*Func02)(code_); break;
        case 0x03: (Target->*Func03)(Data); break;
        default: sASSERT0();
        }
    }
    else 
    {
        switch(Type)
        {
        case 0x00: break;
        case 0x31: (*Func31)(); break;
        case 0x32: (*Func32)(Code); break;
        case 0x33: (*Func33)(Data); break;
        default: sASSERT0();
        }
    }
}

void sGuiMsg::SendCode(const sDragData &dd,int code_)
{
    if(Target)
    {
        switch(Type)
        {
        case 0x00: break;
        case 0x01: (Target->*Func01)(); break;
        case 0x02: (Target->*Func02)(code_); break;
        case 0x03: (Target->*Func03)(Data); break;
        case 0x11: (Target->*Func11)(dd); break;
        case 0x12: (Target->*Func12)(dd,code_); break;
        case 0x13: (Target->*Func13)(dd,Data); break;
        default: sASSERT0();
        }
    }
    else 
    {
        switch(Type)
        {
        case 0x00: break;
        case 0x31: (*Func31)(); break;
        case 0x32: (*Func32)(Code); break;
        case 0x33: (*Func33)(Data); break;
        case 0x41: (*Func41)(dd); break;
        case 0x42: (*Func42)(dd,Code); break;
        case 0x43: (*Func43)(dd,Data); break;
        default: sASSERT0();
        }
    }
}

void sGuiMsg::SendCode(const sKeyData &kd,int code_)
{
    if(Target)
    {
        switch(Type)
        {
        case 0x00: break;
        case 0x01: (Target->*Func01)(); break;
        case 0x02: (Target->*Func02)(code_); break;
        case 0x03: (Target->*Func03)(Data); break;
        case 0x21: (Target->*Func21)(kd); break;
        case 0x22: (Target->*Func22)(kd,code_); break;
        case 0x23: (Target->*Func23)(kd,Data); break;
        default: sASSERT0();
        }
    }
    else 
    {
        switch(Type)
        {
        case 0x00: break;
        case 0x31: (*Func31)(); break;
        case 0x32: (*Func32)(Code); break;
        case 0x33: (*Func33)(Data); break;
        case 0x51: (*Func51)(kd); break;
        case 0x52: (*Func52)(kd,Code); break;
        case 0x53: (*Func53)(kd,Data); break;
        default: sASSERT0();
        }
    }
}

void sGuiMsg::Post()
{
    Gui->PostQueue.Add(*this);
}


/****************************************************************************/
/***                                                                      ***/
/***   Update Notification System                                         ***/
/***                                                                      ***/
/****************************************************************************/

sUpdateReceiver::sUpdateReceiver()
{
}

sUpdateReceiver::~sUpdateReceiver()
{
    for(auto p : Pools)
        p->Rem(this);
}
/*
void sUpdateReceiver::Update()
{
sUpdatePool *p;
sFORALL(Pools,p)
p->Update();
}
*/
void sUpdateReceiver::Rem(sUpdatePool *p)
{
    Pools.Rem(p);
}

void sUpdateReceiver::Add(sUpdatePool *p)
{
    Pools.Add(p);
}

/****************************************************************************/

sUpdatePool::sUpdatePool()
{
}

sUpdatePool::~sUpdatePool()
{
    for(auto r : Receivers)
        r->Rem(this);
}

void sUpdatePool::Update()
{
    for(auto r : Receivers)
        r->Call.Call();
}

void sUpdatePool::Rem(sUpdateReceiver *r)
{
    Receivers.Rem(r);
}

void sUpdatePool::Add(sUpdateReceiver *r)
{
    Receivers.Add(r);
}

/****************************************************************************/

void sUpdateConnection(sUpdateReceiver *r,sUpdatePool *p)
{
    r->Add(p);
    p->Add(r);
}

/****************************************************************************/
/***                                                                      ***/
/***   Drag & Drop                                                        ***/
/***                                                                      ***/
/****************************************************************************/

void sDragDropIcon::Clear()
{
    Kind = sDIK_Error;
    Text = 0;
    Image = 0;
    SourceWin = 0;
    GcObject = 0;
    RcObject = 0;
}

sDragDropIcon::sDragDropIcon()
{
    Clear();
}

sDragDropIcon::sDragDropIcon(const sRect &client,int cmd,int index,const char *text)
{
    Clear();
    Kind = sDIK_PushButton;
    Client = client;
    Command = cmd;
    Index = index;
    Text = text;
}

sDragDropIcon::sDragDropIcon(const sRect &client,int cmd,int index,const char *text,sGCObject *obj)
{
    Clear();
    Kind = sDIK_PushButton;
    Client = client;
    Command = cmd;
    Index = index;
    Text = text;
    GcObject = obj;
}

sDragDropIcon::sDragDropIcon(const sRect &client,int cmd,int index,const char *text,sRCObject *obj)
{
    Clear();
    Kind = sDIK_PushButton;
    Client = client;
    Command = cmd;
    Index = index;
    Text = text;
    RcObject = obj;
}

sDragDropIcon::sDragDropIcon(const sRect &client,int cmd,int index,sPainterImage *img)
{
    Clear();
    Kind = sDIK_Image;
    if(img)
        ImageRect.Set(0,0,img->SizeX,img->SizeY);
    Client = client;
    Command = cmd;
    Index = index;
    Image = img;
}

sDragDropIcon::sDragDropIcon(const sRect &client,int cmd,int index,sPainterImage *img,sGCObject *obj)
{
    Clear();
    Kind = sDIK_Image;
    if(img)
        ImageRect.Set(0,0,img->SizeX,img->SizeY);
    Client = client;
    Command = cmd;
    Index = index;
    Image = img;
    GcObject = obj;
}

sDragDropIcon::sDragDropIcon(const sRect &client,int cmd,int index,sPainterImage *img,sRCObject *obj)
{
    Clear();
    Kind = sDIK_Image;
    if(img)
        ImageRect.Set(0,0,img->SizeX,img->SizeY);
    Client = client;
    Command = cmd;
    Index = index;
    Image = img;
    RcObject = obj;
}

sDragDropIcon::sDragDropIcon(const sRect &client,int cmd,int index,sPainterImage *img,const char *text)
{
    Clear();
    Kind = sDIK_Image;
    if(img)
        ImageRect.Set(0,0,img->SizeX,img->SizeY);
    Client = client;
    Command = cmd;
    Index = index;
    Text = text;
    Image = img;
}

sDragDropIcon::sDragDropIcon(const sRect &client,int cmd,int index,sPainterImage *img,const char *text,sGCObject *obj)
{
    Clear();
    Kind = sDIK_Image;
    if(img)
        ImageRect.Set(0,0,img->SizeX,img->SizeY);
    Client = client;
    Command = cmd;
    Index = index;
    Image = img;
    Text = text;
    GcObject = obj;
}

sDragDropIcon::sDragDropIcon(const sRect &client,int cmd,int index,sPainterImage *img,const char *text,sRCObject *obj)
{
    Clear();
    Kind = sDIK_Image;
    if(img)
        ImageRect.Set(0,0,img->SizeX,img->SizeY);
    Client = client;
    Command = cmd;
    Index = index;
    Image = img;
    Text = text;
    RcObject = obj;
}

/****************************************************************************/
/***                                                                      ***/
/***   Window Root Class                                                  ***/
/***                                                                      ***/
/****************************************************************************/

sWindow::sWindow()
{
    ReqSizeX = 0;
    ReqSizeY = 0;
    DecoratedSizeX = 0;
    DecoratedSizeY = 0;
    ScrollX = 0;
    ScrollY = 0;
    ScrollStartX = 0;
    ScrollStartY = 0;
    FocusParent = 0;
    BorderParent = 0;
    IndirectParent = 0;
    Temp = 0;
    Flags = 0;
    ClassName = 0;
    MouseCursor = sHCI_Arrow;
    UpdateReceiver.Call = sDelegate<void>(this,&sWindow::Update);
    LayoutShortcut = 0;
    Screen = 0;
    CurrentTool = 0;
    ToolTipText = 0;
    ToolTipTextLen = -1;
}

sWindow::~sWindow()
{
    KeyInfos.DeleteAll();
    DragInfos.DeleteAll();
    ToolInfos.DeleteAll();
}

/****************************************************************************/

bool sWindow::OnPaintToolTip(int layer,int tx,int ty,int time)
{
    if(ToolTipText && time>500)
    {
        sRect r(tx,ty,tx+100,ty+20);
        Style()->Rect(layer+15,this,sSK_ToolTip,r,0,ToolTipText,ToolTipTextLen);
        return 1;
    }
    return 0;
}

void sWindow::SetScreen(sGuiScreen *scr)
{
    if(Screen==0 && scr!=0)
    {
        Screen = scr;
        OnInit();
        for(auto w : Childs)
            w->SetScreen(scr);
        for(auto w : Borders)
            w->SetScreen(scr);
    }
}

void sWindow::AddChild(sWindow *w)
{
    w->SetScreen(Screen);
    sASSERT(!(w->Flags & sWF_Border));
    Childs.Add(w);
    w->FocusParent = this;
}

void sWindow::AddBorder(sWindow *w)
{
    w->SetScreen(Screen);
    sASSERT(w->Flags & sWF_Border);
    Borders.AddHead(w);
    w->FocusParent = this;
}

void sWindow::AddScrolling(bool x,bool y)
{
    if(x || y)
    {
        AddBorder(new sScrollBorder(x,y));
        AddDrag("Scroll",sKEY_MMB,sGuiMsg(this,&sWindow::DragScroll));
    }
    if(y)
    {
        AddKey("Scroll Up",sKEY_WheelUp,sGuiMsg(this,&sWindow::CmdScrollWheelUp));
        AddKey("Scroll Down",sKEY_WheelDown,sGuiMsg(this,&sWindow::CmdScrollWheelDown));
    }
    if(x) Flags |= sWF_ScrollX;
    if(y) Flags |= sWF_ScrollY;

}

void sWindow::AddHelp()
{
    AddKey("Help",sKEY_RMB,sGuiMsg(this,&sWindow::CmdHelp));
}

void sWindow::ClearDragDrop()
{
    DragDropIcons.Clear();
    Gui->ClearDragDrop(this);
}

void sWindow::AddDragDrop(const sDragDropIcon &icon)
{
    sDragDropIcon *d = DragDropIcons.AddMany(1);
    *d = icon;
    d->SourceWin = this;
}

void sWindow::PaintDragDropIcon(int layer,const sDragDropIcon &icon,int dx,int dy)
{
    auto pnt = Painter();
    auto sty = Style();
    sRect r(icon.Client);
    r.Move(dx,dy);
    sty->DragDropIcon = &icon;
    sty->Rect(layer,this,sSK_DragDropIcon,r,0,icon.Text);
    return;
}

void sWindow::PaintDragDrop(int layer)
{
    for(const auto &icon : DragDropIcons)
    {
        PaintDragDropIcon(layer,icon,0,0);
    }
}

void sWindow::ResetTooltipTimer()
{
    Gui->ResetTooltipTimer(this);
}

sWindowKeyInfo *sWindow::AddKey(const char *label,uint key,const sGuiMsg &msg)
{
    sWindowKeyInfo *ki = KeyInfos.Find([&](sWindowKeyInfo *ki){ return ki->Kind==sWKIK_Key && ki->Key==key; });
    if(!ki)
    {
        ki = new sWindowKeyInfo;
        if(key & sKEYQ_Double)
            KeyInfos.AddHead(ki);
        else
            KeyInfos.AddTail(ki);
    }
    ki->Kind = sWKIK_Key;
    ki->Key = key;
    ki->Label = label;
    ki->Message = msg;
    return ki;
}

sWindowKeyInfo *sWindow::AddAllKeys(const char *label,const sGuiMsg &msg)
{
    sWindowKeyInfo *ki = new sWindowKeyInfo;
    KeyInfos.Add(ki);
    ki->Kind = sWKIK_AllKeys;
    ki->Key = 0;
    ki->Label = label;
    ki->Message = msg;
    return ki;
}

sWindowKeyInfo *sWindow::AddSpecialKeys(const char *label,const sGuiMsg &msg)
{
    sWindowKeyInfo *ki = new sWindowKeyInfo;
    KeyInfos.Add(ki);
    ki->Kind = sWKIK_SpecialKeys;
    ki->Key = 0;
    ki->Label = label;
    ki->Message = msg;
    return ki;
}

sWindowToolInfo *sWindow::AddTool(const char *label,uint key,const sGuiMsg &msg)
{
    sWindowToolInfo *ti = new sWindowToolInfo;
    ToolInfos.Add(ti);
    ti->Key = key;
    ti->Label = label;
    ti->Message = msg;
    return ti;
}


sWindowDragInfo *sWindow::AddDrag(const char *label,uint key,const sGuiMsg &msg,sWindowToolInfo *tool)
{
    sWindowDragInfo *di = new sWindowDragInfo;
    DragInfos.Add(di);
    di->Kind = sWDIK_Drag;
    di->Key = key;
    di->Label = label;
    di->Hit = 0;
    di->Message = msg;
    di->Tool = tool;
    return di;
}

sWindowDragInfo *sWindow::AddDragHit(const char *label,uint key,const sGuiMsg &msg,int checkhit,sWindowToolInfo *tool)
{
    sWindowDragInfo *di = new sWindowDragInfo;
    DragInfos.Add(di);
    di->Kind = sWDIK_DragHit;
    di->Key = key;
    di->Label = label;
    di->Hit = checkhit;
    di->Message = msg;
    di->Tool = tool;
    return di;
}

sWindowDragInfo *sWindow::AddDragMiss(const char *label,uint key,const sGuiMsg &msg,int checkhit,sWindowToolInfo *tool)
{
    sWindowDragInfo *di = new sWindowDragInfo;
    DragInfos.Add(di);
    di->Kind = sWDIK_DragMiss;
    di->Key = key;
    di->Label = label;
    di->Hit = checkhit;
    di->Message = msg;
    di->Tool = tool;
    return di;
}

sWindowDragInfo *sWindow::AddDragAll(const char *label,const sGuiMsg &msg,sWindowToolInfo *tool)
{
    sWindowDragInfo *di = new sWindowDragInfo;
    DragInfos.Add(di);
    di->Kind = sWDIK_DragAll;
    di->Key = 0;
    di->Label = label;
    di->Hit = 0;
    di->Message = msg;
    di->Tool = tool;
    return di;
}

void sWindow::RemKey(const sGuiMsg &msg)
{
    int n = 0;
    for(auto ki : KeyInfos)
    {
        if(ki->Message==msg)
        {
            KeyInfos.RemAtOrder(n);
            delete ki;
            return;
        }
        n++;
    }
}

/****************************************************************************/

void sWindow::CmdKill()
{
    Flags |= sWF_Kill|sWF_Disable; 
    Gui->KillFlag = 1;
    Gui->Layout(); 
    Update();
}

void sWindow::CmdExit()
{
    sExit();
}

void sWindow::CmdScrollWheelUp()
{
    ScrollY -= Style()->Font->Advance*3;
    Gui->Layout();
    Update();
}

void sWindow::CmdScrollWheelDown()
{
    ScrollY += (Style()->Font->Advance+2)*3;
    Gui->Layout();
    Update();
}

void sWindow::DragScroll(const sDragData &dd)
{
    if(dd.Mode==sDM_Start)
    {
        ScrollStartX = ScrollX;
        ScrollStartY = ScrollY;
    }
    if(dd.Mode==sDM_Drag)
    {
        ScrollX = ScrollStartX-dd.DeltaX;
        ScrollY = ScrollStartY-dd.DeltaY;
        Gui->Layout();
        Update();
    }
}

/****************************************************************************/

void sWindow::CmdHelp()
{
    bool c_key = 0;
    bool c_drag = 0;
    bool c_cursor = 0;

    sMenuFrame *m = new sMenuFrame(this);

    for(auto ki : KeyInfos)
    {
        switch(ki->Kind)
        {
        case sWKIK_Key:
            if (ki->Key & sKEYQ_Break)
                break;

            if((ki->Key&sKEYQ_Mask)>=sKEY_Up && (ki->Key&sKEYQ_Mask)<=sKEY_End)
            {
                if(!c_cursor)
                {
                    c_cursor=1;
                    m->AddHeader(2,"Cursor");
                }
                m->SetDefaultColumn(2);
            }
            else
            {
                if(!c_key)
                {
                    c_key=1;
                    m->AddHeader(1,"Key");
                }
                m->SetDefaultColumn(1);
            }
            m->AddItem(ki->Message,ki->Key,ki->Label);
            break;
        default:
            break;
        }
    }

    for(auto di : DragInfos)
    {
        if(!di->Tool)
        {
            if(!c_drag)
            {
                c_drag=1;
                m->AddHeader(3,"Mouse");
            }
            m->SetDefaultColumn(3);

            switch(di->Kind)
            {
            case sWDIK_Drag:      m->AddItem(sGuiMsg(),di->Key,di->Label); break;
            case sWDIK_DragHit:   m->AddItem(sGuiMsg(),di->Key,di->Label,-1,"HIT+"); break;
            case sWDIK_DragMiss:  m->AddItem(sGuiMsg(),di->Key,di->Label,-1,"MISS+"); break;
            default: break;
            }
        }
    }

    if(ToolInfos.GetCount())
    {
        m->AddHeader(4,"Tools");
        m->SetDefaultColumn(4);
        for(auto ti : ToolInfos)
        {
            m->AddHeaderItem(ti->Key,ti->Label);
            for(auto di : DragInfos)
            {
                if(di->Tool==ti)
                {
                    switch(di->Kind)
                    {
                    case sWDIK_Drag:      m->AddItem(sGuiMsg(),di->Key,di->Label); break;
                    case sWDIK_DragHit:   m->AddItem(sGuiMsg(),di->Key,di->Label,-1,"HIT+"); break;
                    case sWDIK_DragMiss:  m->AddItem(sGuiMsg(),di->Key,di->Label,-1,"MISS+"); break;
                    default: break;
                    }
                }
            }
        }
    }


    m->StartContext();
}

/****************************************************************************/

void sWindow::Update() 
{
    Gui->Update(this,Outer); 
}

void sWindow::ScrollTo(const sRect &t)
{
    int scry = ScrollY;
    int scrx = ScrollX;

    if(Flags & sWF_ScrollX)
    {
        int x0 = t.x0 - Client.x0;
        if(x0<ScrollX) ScrollX = x0;
        int x1 = t.x1 - Client.x0;
        if(x1>ScrollX+Inner.SizeX()) ScrollX = x1-Inner.SizeX();
    }
    if(Flags & sWF_ScrollY)
    {
        int y0 = t.y0 - Client.y0;
        if(y0<ScrollY) ScrollY = y0;
        int y1 = t.y1 - Client.y0;
        if(y1>ScrollY+Inner.SizeY()) ScrollY = y1-Inner.SizeY();
    }

    if(scry!=ScrollY || scrx!=ScrollX)
        Gui->Layout();
}

void sWindow::ScrollTo(int x,int y)
{
    int scry = ScrollY;
    int scrx = ScrollX;

    if(Flags & sWF_ScrollX)
        ScrollX = sClamp(x,0,Client.SizeX()-Inner.SizeX());
    if(Flags & sWF_ScrollY)
        ScrollY = sClamp(y,0,Client.SizeY()-Inner.SizeY());

    if(scry!=ScrollY || scrx!=ScrollX)
        Gui->Layout();
}

/****************************************************************************/
/****************************************************************************/

sMultitoucher::sMultitoucher()
{
    Mode = sMM_Off;
    sGetScreen()->GetSize(ScreenX,ScreenY);
}

const sDragData &sMultitoucher::Convert(const sDragData &dd)
{
    uint qual = sGetKeyQualifier();
    DragOut = dd;

    if(Mode==sMM_Off && (dd.Mode==sDM_Start || dd.Mode==sDM_Drag) && (qual & sKEYQ_Shift))
    {
        Mode = sMM_Centered;
        DragOut.Touches[1].Mode = sDM_Start;
        DragOut.Touches[1].StartX = ScreenX - DragOut.Touches[0].StartX;
        DragOut.Touches[1].StartY = ScreenY - DragOut.Touches[0].StartY;
        DragOut.Touches[1].PosX   = ScreenX - DragOut.Touches[0].PosX;
        DragOut.Touches[1].PosY   = ScreenY - DragOut.Touches[0].PosY;
        DragOut.TouchCount = 2;
    }
    else if(Mode==sMM_Centered)
    {
        DragOut.Touches[1].Mode = sDM_Drag;
        DragOut.Touches[1].StartX = ScreenX - DragOut.Touches[0].StartX;
        DragOut.Touches[1].StartY = ScreenY - DragOut.Touches[0].StartY;
        DragOut.Touches[1].PosX   = ScreenX - DragOut.Touches[0].PosX;
        DragOut.Touches[1].PosY   = ScreenY - DragOut.Touches[0].PosY;
        DragOut.TouchCount = 2;
    }

    if(Mode==sMM_Centered && (dd.Mode==sDM_Stop || !(qual & sKEYQ_Shift)))
    {
        DragOut.Touches[1].Mode = sDM_Stop;
        DragOut.Touches[1].StartX = ScreenX - DragOut.Touches[0].StartX;
        DragOut.Touches[1].StartY = ScreenY - DragOut.Touches[0].StartY;
        DragOut.Touches[1].PosX   = ScreenX - DragOut.Touches[0].PosX;
        DragOut.Touches[1].PosY   = ScreenY - DragOut.Touches[0].PosY;
        DragOut.TouchCount = 2;

        Mode = sMM_Off;
    }

    if(Mode==sMM_Off && (dd.Mode==sDM_Start || dd.Mode==sDM_Drag) && (qual & sKEYQ_Ctrl))
    {
        Mode = sMM_MirrorY;
        DragOut.Touches[1].Mode = sDM_Start;
        DragOut.Touches[1].StartX =           DragOut.Touches[0].StartX;
        DragOut.Touches[1].StartY = ScreenY - DragOut.Touches[0].StartY;
        DragOut.Touches[1].PosX   =           DragOut.Touches[0].PosX;
        DragOut.Touches[1].PosY   = ScreenY - DragOut.Touches[0].PosY;
        DragOut.TouchCount = 2;
    }
    else if(Mode==sMM_MirrorY)
    {
        DragOut.Touches[1].Mode = sDM_Drag;
        DragOut.Touches[1].StartX =           DragOut.Touches[0].StartX;
        DragOut.Touches[1].StartY = ScreenY - DragOut.Touches[0].StartY;
        DragOut.Touches[1].PosX   =           DragOut.Touches[0].PosX;
        DragOut.Touches[1].PosY   = ScreenY - DragOut.Touches[0].PosY;
        DragOut.TouchCount = 2;
    }

    if(Mode==sMM_MirrorY && (dd.Mode==sDM_Stop || !(qual & sKEYQ_Ctrl)))
    {
        DragOut.Touches[1].Mode = sDM_Stop;
        DragOut.Touches[1].StartX =           DragOut.Touches[0].StartX;
        DragOut.Touches[1].StartY = ScreenY - DragOut.Touches[0].StartY;
        DragOut.Touches[1].PosX   =           DragOut.Touches[0].PosX;
        DragOut.Touches[1].PosY   = ScreenY - DragOut.Touches[0].PosY;
        DragOut.TouchCount = 2;

        Mode = sMM_Off;
    }


    return DragOut;
}

/****************************************************************************/
}
