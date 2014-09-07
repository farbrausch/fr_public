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
/***   Just pass through single child                                     ***/
/***                                                                      ***/
/****************************************************************************/

void sPassthroughFrame::OnCalcSize()
{
    if(Childs.GetCount()>0)
    {
        ReqSizeX = Childs[0]->DecoratedSizeX;
        ReqSizeY = Childs[0]->DecoratedSizeY;
    }
}

void sPassthroughFrame::OnLayoutChilds()
{
    int i = 0;
    for(auto w : Childs)
    {
        w->Outer = Client;
        if(i==0)
            w->Flags &= ~sWF_Disable;
        else
            w->Flags |= sWF_Disable;
        i++;
    }
}

/****************************************************************************/

void sPassthroughFrame2::OnCalcSize()
{
    if(Childs.GetCount()>0)
    {
        ReqSizeX = 0;
        ReqSizeY = 0;
    }
}

void sPassthroughFrame2::OnLayoutChilds()
{
    int i = 0;
    for(auto w : Childs)
    {
        w->Outer = Client;
        if(i==0)
            w->Flags &= ~sWF_Disable;
        else
            w->Flags |= sWF_Disable;
        i++;
    }
}

/****************************************************************************/

sKillMeFrame::sKillMeFrame()
{
    FocusAfterKill = 0;
}

sKillMeFrame::sKillMeFrame(sWindow *c)
{
    FocusAfterKill = 0;
    AddChild(c);
}

void sKillMeFrame::OnPaint(int layer)
{
    if(!(Flags & sWF_ChildFocus))
    {
        CmdKillAndFocus();
    }
}

void sKillMeFrame::CmdKillAndFocus()
{
    CmdKill();
    if(FocusAfterKill)
        Gui->SetFocus(FocusAfterKill);
}

/****************************************************************************/
/***                                                                      ***/
/***   Box Layout (h/v)                                                   ***/
/***                                                                      ***/
/****************************************************************************/

sBoxFrame::sBoxFrame()
{
    KnopSize = 0;
}

void sBoxFrame::UpdateInfo()
{
    int max = Childs.GetCount();
    if(Infos.GetCount() != max)
    {
        Infos.SetSize(max);
        for(int i=0;i<max;i++)
        {
            Infos[i].Weight = -1;
            Infos[i].Width = -1;
            Infos[i].p0 = 0;
            Infos[i].p1 = 0;
        }
    }
}

void sBoxFrame::LayoutInfo(int width)
{
    UpdateInfo();

    int max = Childs.GetCount();
    sASSERT(max==Infos.GetCount());

    int weight = 0;
    int left = width;
    for(int i=0;i<max;i++)
    {
        if(Infos[i].Width==-1)
            Infos[i].Width = Childs[i]->Temp;
        if(Infos[i].Weight==-1)
            Infos[i].Weight = Infos[i].Width==0;

        weight += Infos[i].Weight;
        left -= Infos[i].Width;
    }

    if(left<0) left = 0;
    int pos = 0;
    int waccu = 0;
    for(int i=0;i<max;i++)
    {
        Infos[i].p0 = pos;

        pos += Infos[i].Width;
        pos += (Infos[i].Weight+waccu)*left/weight - waccu*left/weight;
        waccu += Infos[i].Weight;
        Infos[i].p1 = pos;
    }
}


void sBoxFrame::SetWeight(int n,int weight)
{
    UpdateInfo();
    Infos[n].Weight = weight;
}

/****************************************************************************/

sVBoxFrame::sVBoxFrame()
{
}

void sVBoxFrame::OnCalcSize()
{
    ReqSizeX = 0;
    ReqSizeY = 0;
    for(auto c : Childs)
    {
        ReqSizeX += c->ReqSizeX;
        ReqSizeY = sMax(ReqSizeY,c->ReqSizeY);
    }
}

void sVBoxFrame::OnLayoutChilds()
{
    for(auto c : Childs)
        c->Temp = c->DecoratedSizeX;

    LayoutInfo(Inner.SizeX());

    int i = 0;
    for(auto c : Childs)
    {
        c->Outer.x0 = Inner.x0 + Infos[i].p0; 
        c->Outer.x1 = Inner.x0 + Infos[i].p1;
        c->Outer.y0 = Inner.y0;
        c->Outer.y1 = Inner.y1;
        i++;
    }
}

/****************************************************************************/

sHBoxFrame::sHBoxFrame()
{
}

void sHBoxFrame::OnCalcSize()
{
    ReqSizeX = 0;
    ReqSizeY = 0;
    for(auto c : Childs)
    {
        ReqSizeY += c->ReqSizeY;
        ReqSizeX = sMax(ReqSizeX,c->ReqSizeX);
    }
}

void sHBoxFrame::OnLayoutChilds()
{
    for(auto c : Childs)
        c->Temp = c->DecoratedSizeY;

    LayoutInfo(Inner.SizeY());

    int i = 0;
    for(auto c : Childs)
    {
        c->Outer.y0 = Inner.y0 + Infos[i].p0; 
        c->Outer.y1 = Inner.y0 + Infos[i].p1;
        c->Outer.x0 = Inner.x0;
        c->Outer.x1 = Inner.x1;
        i++;
    }
}
/****************************************************************************/
/***                                                                      ***/
/***   Splitters (h/v)                                                    ***/
/***                                                                      ***/
/****************************************************************************/

sSplitterFrame::sSplitterFrame()
{
    KnopSize = 0;
    DragKnop = -1;
    DragStart = 0;
    FirstLayout = 1;
    OldWidth = 0;
}

void sSplitterFrame::OnInit()
{
    KnopSize = Style()->SplitterKnopSize;
}

void sSplitterFrame::UpdateInfo()
{
    int max = Childs.GetCount();
    if(Infos.GetCount() != max)
    {
        Infos.SetSize(max);
        for(int i=0;i<max;i++)
        {
            Infos[i].Preset = -1;
            Infos[i].Pos = 0;
            Infos[i].p0 = 0;
            Infos[i].p1 = 0;
        }
    }
}

void sSplitterFrame::LayoutInfo(int width)
{
    UpdateInfo();

    int max = Childs.GetCount();
    sASSERT(max==Infos.GetCount());

    if(FirstLayout)
    {
        FirstLayout = 0;

        int weight = 0;
        int left = width;
        for(int i=0;i<max;i++)
        {
            if(Infos[i].Preset<0)
                weight -= Infos[i].Preset;
            else
                left -= Infos[i].Preset;
        }
        if(left<0)
            left = 0;
        int waccu = 0;
        int p = 0;
        for(int i=0;i<max;i++)
        {
            Infos[i].Pos = p;
            if(Infos[i].Preset<0)
            {
                p += (waccu-Infos[i].Preset)*left/weight - waccu*left/weight;
                waccu -= Infos[i].Preset;
            }
            else
            {
                p += Infos[i].Preset;
            }
        }
        RestInfos(width);
    }
    else if(OldWidth!=width)
    {
        int weight = 0;
        int left = width;
        for(int i=0;i<max;i++)
        {
            if(Infos[i].Preset<0)
                weight -= Infos[i].Preset;
            else
                left -= Infos[i].p1-Infos[i].p0+KnopSize;
        }
        if(left<0) left = 0;
        int delta = 0;
        int waccu = 0;
        for(int i=0;i<max;i++)
        {
            Infos[i].Pos = Infos[i].RestPos + delta;
            if(Infos[i].Preset<0)
            {
                delta += (waccu-Infos[i].Preset)*(width-RestWidth)/weight - waccu*(width-RestWidth)/weight;
                waccu -= Infos[i].Preset;
            }
        }
    }
    OldWidth = width;

    LimitSplitter(width);

    for(int i=0;i<max-1;i++)
    {
        Infos[i].p0 = Infos[i].Pos;
        Infos[i].p1 = Infos[i+1].Pos - KnopSize;
    }
    Infos[max-1].p0 = Infos[max-1].Pos;
    Infos[max-1].p1 = width;
}


void sSplitterFrame::SetSize(int n,int s)
{
    UpdateInfo();
    Infos[n].Preset = s;
}

void sSplitterFrame::RestInfos(int width)
{
    int max = Infos.GetCount();
    for(int i=0;i<max;i++)
        Infos[i].RestPos = Infos[i].Pos;
    RestWidth = width;
}

void sSplitterFrame::LimitSplitter(int width,int n)
{
    int max = (int) Infos.GetCount();

    Infos[0].Pos = 0;

    for(int i=0;i<max;i++)
    {
        if(Infos[i].Pos > width-(max-1-i)*KnopSize)
            Infos[i].Pos = width-(max-1-i)*KnopSize;
        if(Infos[i].Pos < i*KnopSize)
            Infos[i].Pos = i*KnopSize;
    }

    for(int i=n-1;i>=0;i--)
    {
        if(Infos[i].Pos > Infos[i+1].Pos-KnopSize)
            Infos[i].Pos = Infos[i+1].Pos-KnopSize;
    }

    for(int i=n+1;i<max;i++)
    {
        if(Infos[i].Pos < Infos[i-1].Pos+KnopSize)
            Infos[i].Pos = Infos[i-1].Pos+KnopSize;
    }
}

/****************************************************************************/

sVSplitterFrame::sVSplitterFrame()
{
    MouseCursor = sHCI_SizeH;
    AddDrag("move",sKEY_LMB,sGuiMsg(this,&sVSplitterFrame::Drag));
}

void sVSplitterFrame::OnCalcSize()
{
    ReqSizeX = 0;
    ReqSizeY = 0;
    for(auto c : Childs)
    {
        ReqSizeX += c->ReqSizeX;
        ReqSizeY = sMax(ReqSizeY,c->ReqSizeY);
    }
}

void sVSplitterFrame::OnLayoutChilds()
{
    for(auto c : Childs)
        c->Temp = c->DecoratedSizeX;

    LayoutInfo(Inner.SizeX());

    int i = 0;
    for(auto c : Childs)
    {
        c->Outer.x0 = Inner.x0 + Infos[i].p0; 
        c->Outer.x1 = Inner.x0 + Infos[i].p1;
        c->Outer.y0 = Inner.y0;
        c->Outer.y1 = Inner.y1;

        Infos[i].HitBox.x0 = Inner.x0 + Infos[i].p0 - KnopSize;
        Infos[i].HitBox.x1 = Inner.x0 + Infos[i].p0;
        Infos[i].HitBox.y0 = Inner.y0;
        Infos[i].HitBox.y1 = Inner.y1;
        i++;
    }
}

void sVSplitterFrame::OnPaint(int layer)
{
    int max = Childs.GetCount();
    for(int i=1;i<max;i++)
        Style()->Rect(layer,this,sSK_VSplitter,Infos[i].HitBox,DragKnop==i ? sSM_Pressed : 0);
}

void sVSplitterFrame::Drag(const sDragData &dd)
{
    switch(dd.Mode)
    {
    case sDM_Start:
        for(int i=1;i<Infos.GetCount();i++)
        {
            if(Infos[i].HitBox.Hit(dd.StartX,dd.StartY))
            {
                DragKnop = i;
                DragStart = Infos[i].Pos;
                break;
            }
        }
        break;
    case sDM_Drag:
        if(DragKnop>=0)
        {
            Infos[DragKnop].Pos = DragStart+dd.DeltaX;
            LimitSplitter(Client.SizeX(),DragKnop);
            RestInfos(Client.SizeX());
            Gui->Layout();
            Update();
        }
        break;
    case sDM_Stop:
        DragKnop = -1;
        Update();
        break;
    default:
        break;
    }
}

/****************************************************************************/

sHSplitterFrame::sHSplitterFrame()
{
    MouseCursor = sHCI_SizeV;
    AddDrag("move",sKEY_LMB,sGuiMsg(this,&sHSplitterFrame::Drag));
}

void sHSplitterFrame::OnCalcSize()
{
    ReqSizeX = 0;
    ReqSizeY = 0;
    for(auto c : Childs)
    {
        ReqSizeY += c->ReqSizeY;
        ReqSizeX = sMax(ReqSizeX,c->ReqSizeX);
    }
}

void sHSplitterFrame::OnLayoutChilds()
{
    for(auto c : Childs)
        c->Temp = c->DecoratedSizeY;

    LayoutInfo(Inner.SizeY());

    int i = 0;
    for(auto c : Childs)
    {
        c->Outer.y0 = Inner.y0 + Infos[i].p0; 
        c->Outer.y1 = Inner.y0 + Infos[i].p1;
        c->Outer.x0 = Inner.x0;
        c->Outer.x1 = Inner.x1;

        Infos[i].HitBox.y0 = Inner.y0 + Infos[i].p0 - KnopSize;
        Infos[i].HitBox.y1 = Inner.y0 + Infos[i].p0;
        Infos[i].HitBox.x0 = Inner.x0;
        Infos[i].HitBox.x1 = Inner.x1;
        i++;
    }
}

void sHSplitterFrame::OnPaint(int layer)
{
    int max = Childs.GetCount();
    for(int i=1;i<max;i++)
        Style()->Rect(layer,this,sSK_HSplitter,Infos[i].HitBox,DragKnop==i ? sSM_Pressed : 0);
}

void sHSplitterFrame::Drag(const sDragData &dd)
{
    switch(dd.Mode)
    {
    case sDM_Start:
        for(int i=1;i<Infos.GetCount();i++)
        {
            if(Infos[i].HitBox.Hit(dd.StartX,dd.StartY))
            {
                DragKnop = i;
                DragStart = Infos[i].Pos;
            }
        }
        break;
    case sDM_Drag:
        if(DragKnop>=0)
        {
            Infos[DragKnop].Pos = DragStart+dd.DeltaY;
            LimitSplitter(Client.SizeY(),DragKnop);
            RestInfos(Client.SizeY());
            Gui->Layout();
            Update();
        }
        break;
    case sDM_Stop:
        Update();
        DragKnop = -1;
        break;
    default:
        break;
    }
}

/****************************************************************************/
/***                                                                      ***/
/***   Switch                                                             ***/
/***                                                                      ***/
/****************************************************************************/

sSwitchFrame::sSwitchFrame()
{
    Switch = 0;
}

void sSwitchFrame::OnCalcSize()
{
    ReqSizeX = 0;
    ReqSizeY = 0;
}

void sSwitchFrame::OnLayoutChilds()
{
    int i = 0;
    for(auto w : Childs)
    {
        w->Outer = Client;
        if(i==Switch)
            w->Flags &= ~sWF_Disable;
        else
            w->Flags |= sWF_Disable;
        i++;
    }
}

void sSwitchFrame::SetSwitch(int n)
{
    if(Switch!=n)
    {
        Switch = n;

        int i = 0;
        for(auto w : Childs)
        {
            if(i==Switch)
            {
                w->Flags &= ~sWF_Disable;
                w->FocusParent = this;
                Gui->SetFocus(w);
            }
            else
            {
                w->Flags |= sWF_Disable;
            }
            i++;
        }
        Gui->Layout();
        Update();
    }
}


/****************************************************************************/
/***                                                                      ***/
/***   Grid                                                               ***/
/***                                                                      ***/
/****************************************************************************/

sGridFrame::sGridFrame()
{
    GridX = 1;
    GridY = 1;
    Pool = 0;
}

sGridFrame::~sGridFrame()
{
    delete Pool;
}

void sGridFrame::OnInit()
{
    Height = Style()->Font->CellHeight + 4;
}

void sGridFrame::GetRect(sGridFrameItem *it,sRect &r)
{
    r.x0 = Client.x0 +  it->PosX           *Client.SizeX() / GridX;
    r.x1 = Client.x0 + (it->PosX+it->SizeX)*Client.SizeX() / GridX;
    r.y0 = Client.y0 +  it->PosY           *Height;
    r.y1 = Client.y0 + (it->PosY+it->SizeY)*Height;
}

void sGridFrame::OnCalcSize()
{
    ReqSizeY = Height*GridY;
    ReqSizeX = 50*GridX;
}

void sGridFrame::OnLayoutChilds()
{
    for(auto &it : Items)
    {
        if(it.Window)
        {
            /*
            it->Window->ReqSizeX = 0;
            it->Window->ReqSizeY = 0;
            it->Window->DecoratedSizeX = 0;
            it->Window->DecoratedSizeY = 0;
            */
            GetRect(&it,it.Window->Outer);
        }
    }
}

void sGridFrame::OnPaint(int layer)
{
    sRect r;
    Style()->Rect(layer,this,sSK_Back,Client);
    int mx,my,mb;
    Gui->GetMouse(mx,my,mb);
    for(auto &it : Items)
    {
        if(it.Label)
        {
            GetRect(&it,r);
            if(it.Group) 
            {
                Style()->Rect(layer,this,sSK_GridGroup,r,it.Group,it.Label,it.LabelLen);
            }
            else
            {
                Painter()->SetLayer(layer+3);
                Painter()->SetFont(Style()->Font);
                Painter()->Text(it.LabelFlags,Style()->Colors[sGC_Text],r,it.Label,it.LabelLen);
                //        Style()->Rect(layer,this,sSK_GridLabel,r,0,it->Label,it->LabelLen);

                int w = Style()->Font->GetWidth(it.Label,it.LabelLen) + Style()->Font->CellHeight;
                if(r.SizeX()<w && r.Hit(mx,my))
                {
                    sRect rr;
                    rr = r;
                    rr.x0 = rr.x1 - w;
                    Style()->Rect(layer,this,sSK_ToolTip,rr,0,it.Label,it.LabelLen,0);
                }
            }
        }
    }
}

sGridFrameItem *sGridFrame::AddItem(int x,int y,int sx,int sy)
{
    sGridFrameItem *it = Items.Add();

    it->PosX = x;
    it->PosY = y;
    it->SizeX = sx;
    it->SizeY = sy;

    if(x+sx>GridX) GridX = x+sx;
    if(y+sy>GridY) GridY = y+sy;

    return it;
}

void sGridFrame::AddLabel(int x,int y,int sx,int sy,const char *label,int len,int flags,int group)
{
    sGridFrameItem *it = AddItem(x,y,sx,sy);

    it->PosX = x;
    it->PosY = y;
    it->SizeX = sx;
    it->SizeY = sy;
    it->Window = 0;
    it->Label = label;
    it->LabelLen = len;
    it->LabelFlags = flags;
    it->Group = group;
}

void sGridFrame::AddWindow(int x,int y,int sx,int sy,sWindow *w)
{
    sGridFrameItem *it = AddItem(x,y,sx,sy);
    AddChild(w);

    it->PosX = x;
    it->PosY = y;
    it->SizeX = sx;
    it->SizeY = sy;
    it->Window = w;
    it->Label = 0;
    it->LabelLen = -1;
    it->LabelFlags = 0;
    it->Group = 0;
}

void sGridFrame::Clear()
{
    for(auto w : Childs)
        w->CmdKill();
    Items.Clear();
    GridY = 1;
    if(Pool)
        Pool->Reset();
}

const char *sGridFrame::AllocTempString(const char *src)
{
    if(!Pool)
        Pool = new sMemoryPool(0x4000);
    return Pool->AllocString(src);
}


/****************************************************************************/
/****************************************************************************/

sGridFrameHelper::sGridFrameHelper(sGridFrame *g)
{
    Grid = g;
    if(g->GridX==1)
        g->GridX = 14;
    Columns = g->GridX;
    Left = 3;
    Right = Columns;
    Small = 1;
    Normal = 2;
    Wide = 8;
    OverrideWidth = 0;
    UpdatePool = 0;

    LineIsEmpty = 1;
    Pos0 = Left;
    Pos1 = Right;
    Line = 0;

    LinkMode = 0;
    FirstLink = 0;
    CurrentLink = 0;

    Grid->Clear();
}

sGridFrameHelper::~sGridFrameHelper()
{
    sASSERT(!LinkMode);
    Gui->Layout();
}

/****************************************************************************/

void sGridFrameHelper::NextLine()
{
    if(!LineIsEmpty)
        ForceNextLine();
}

void sGridFrameHelper::ForceNextLine()
{
    LineIsEmpty = 0;
    Pos0 = Left;
    Pos1 = Right;
    Line++;
}

void sGridFrameHelper::FullWidth() 
{
    NextLine(); 
    Pos0 = 0; 
    OverrideWidth = Grid->GridX; 
}

void sGridFrameHelper::ExtendRight() 
{
    OverrideWidth = Grid->GridX-Pos0; 
}

void sGridFrameHelper::Label(const char *str)
{
    NextLine();
    Grid->AddLabel(0,Line,Left,1,str);
    LineIsEmpty = 0;
}

void sGridFrameHelper::Label(const char *str,int flags)
{
    NextLine();
    Grid->AddLabel(0,Line,Left,1,str,-1,flags);
    LineIsEmpty = 0;
}

void sGridFrameHelper::Group(const char *str,int priority)
{
    LineIsEmpty = 0;
    NextLine();
    Grid->AddLabel(0,Line,Columns,1,str,-1,0,priority);
    Pos0 = Pos1;
    LineIsEmpty = 0;
}

void sGridFrameHelper::Space(int n)
{
    Pos0 += n;
}

void sGridFrameHelper::AddLeft(sWindow *w,int width,int lines)
{
    if(OverrideWidth)
    {
        width = OverrideWidth;
        OverrideWidth = 0;
    }
    if(Pos0+width>Pos1)
        NextLine();
    Grid->AddWindow(Pos0,Line,width,lines,w);
    Pos0 += width;
    LineIsEmpty = 0;
    if(UpdatePool)
        sUpdateConnection(&w->UpdateReceiver,UpdatePool);
    if(lines>1)
    {
        Line+=lines-1;
    }
}

void sGridFrameHelper::AddRight(sWindow *w,int width)
{
    if(Pos0+width>Pos1)
        NextLine();
    Pos1 -= width;
    Grid->AddWindow(Pos1,Line,width,1,w);
    LineIsEmpty = 0;
    if(UpdatePool)
        sUpdateConnection(&w->UpdateReceiver,UpdatePool);
}

void sGridFrameHelper::AddLeft(sControl *w,int width,int lines)
{
    AddLeft((sWindow *)w,width,lines);
    w->ChangeMsg = ChangeMsg;
    w->DoneMsg = DoneMsg;
}

void sGridFrameHelper::AddRight(sControl *w,int width)
{
    AddRight((sWindow *)w,width);
    w->ChangeMsg = ChangeMsg;
    w->DoneMsg = DoneMsg;
}

void sGridFrameHelper::AddLines(sWindow *w,int lines)
{
    AddLeft(w,Wide,lines);
}

void sGridFrameHelper::BeginLink()
{
    sASSERT(!LinkMode)
        LinkMode = 1;
    FirstLink = 0;
    CurrentLink = 0;
}

void sGridFrameHelper::EndLink()
{
    sASSERT(LinkMode);
    LinkMode = 0;
}

/****************************************************************************/

sButtonControl *sGridFrameHelper::Box(sGuiMsg msg,const char *name)
{
    sButtonControl *b = new sButtonControl(msg,name,sBS_Push);
    AddRight(b,Small);
    return b;
}

sButtonControl *sGridFrameHelper::Button(sGuiMsg msg,const char *name)
{
    sButtonControl *b = new sButtonControl(msg,name,sBS_Push);
    AddLeft(b,Normal);
    return b;
}

sButtonControl *sGridFrameHelper::Toggle(sGuiMsg msg,int &ref,int value,const char *name,int mask)
{
    sButtonControl *b = new sButtonControl(msg,name,sBS_Toggle);
    b->SetRef(&ref,value,mask);
    AddLeft(b,Normal);
    return b;
}

sButtonControl *sGridFrameHelper::Radio(sGuiMsg msg,int &ref,int value,const char *name,int mask)
{
    sButtonControl *b = new sButtonControl(msg,name,sBS_Radio);
    b->SetRef(&ref,value,mask);
    AddLeft(b,Normal);
    return b;
}

sChoiceControl *sGridFrameHelper::Choice(int &ref,const char *choices)
{
    sChoiceControl *c = 0;
    while(*choices)
    {
        c = new sChoiceControl(&ref,choices);
        AddLeft(c,Normal);

        while(*choices!=0 && *choices!=':')
            choices++;
        while(*choices==':')
            choices++;
    }
    return c;
}

sButtonControl *sGridFrameHelper::ConstString(const char *str)
{
    sButtonControl *c = new sButtonControl(sGuiMsg(),str,sBS_Label);
    AddLeft(c,Wide);
    return c;
}

sStringControl *sGridFrameHelper::String(const sStringDesc &desc)
{
    sStringControl *c = new sStringControl(desc);
    AddLeft(c,Wide);
    return c;
}

sPoolStringControl *sGridFrameHelper::String(sPoolString &p)
{
    sPoolStringControl *c = new sPoolStringControl(p);
    AddLeft(c,Wide);
    return c;
}

sTextWindow *sGridFrameHelper::Text(sTextBuffer &text,int lines)
{
    sTextWindow *c = new sTextWindow(&text);
    c->ControlStyle = 1;
    c->AddScrolling(1,1);
    c->Wrapping = 1;
    AddLeft(c,Wide,lines);
    return c;
}

sTextControl *sGridFrameHelper::Text(const sStringDesc &desc,int lines)
{
    sTextControl *c = new sTextControl(desc);
    c->ControlStyle = 1;
    c->AddScrolling(1,1);
    c->Wrapping = 1;
    AddLeft(c,Wide,lines);
    return c;
}

sTextControl *sGridFrameHelper::Text(sPoolString &pool,int lines)
{
    sTextControl *c = new sTextControl(pool);
    c->ControlStyle = 1;
    c->AddScrolling(1,1);
    c->Wrapping = 1;
    AddLeft(c,Wide,lines);
    return c;
}

sIntControl *sGridFrameHelper::Int(int &ref,int min,int max,float step,int def,float step2)
{
    sIntControl *c = new sIntControl(&ref,min,max,def);
    LinkControl(c);
    if(step) c->SetDrag(step,step2);
    AddLeft(c,Normal);
    return c;
}

sU32Control *sGridFrameHelper::Int(uint &ref,uint min,uint max,float step,uint def,float step2)
{
    sU32Control *c = new sU32Control(&ref,min,max,def);
    LinkControl(c);
    if(step) c->SetDrag(step,step2);
    AddLeft(c,Normal);
    return c;
}

sFloatControl *sGridFrameHelper::Float(float &ref,float min,float max,float step,float def,float step2)
{
    sFloatControl *c = new sFloatControl(&ref,min,max,def);
    LinkControl(c);
    if(step) c->SetDrag(step,step2);
    AddLeft(c,Normal);
    return c;
}

sF64Control *sGridFrameHelper::F64(double &ref,double min,double max,float step,double def,float step2)
{
    sF64Control *c = new sF64Control(&ref,min,max,def);
    LinkControl(c);
    if(step) c->SetDrag(step,step2);
    AddLeft(c,Normal);
    return c;
}

sS64Control *sGridFrameHelper::S64(int64 &ref,int64 min,int64 max,float step,int64 def,float step2)
{
    sS64Control *c = new sS64Control(&ref,min,max,def);
    LinkControl(c);
    if(step) c->SetDrag(step,step2);
    AddLeft(c,Normal);
    return c;
}

void sGridFrameHelper::Color(uint &col,uint def,bool alpha)
{
    sU8Control *con[4];
    const uint8 *defp = (const uint8 *) &def;

    BeginLink();
    con[0] = new sU8Control( ((uint8 *)&col)+2 ,0,255,defp[2]);
    LinkControl(con[0]);
    con[1] = new sU8Control( ((uint8 *)&col)+1 ,0,255,defp[1]);
    LinkControl(con[1]);
    con[2] = new sU8Control( ((uint8 *)&col)+0 ,0,255,defp[0]);
    LinkControl(con[2]);
    EndLink();
    if(alpha)
        con[3] = new sU8Control( ((uint8 *)&col)+3 ,0,255,defp[3]);

    for(int i=0;i<(alpha?4:3);i++)
    {
        AddLeft(con[i],Normal);
        if(i!=3)
            con[i]->BackColorPtr = &col;
        con[i]->SetDrag(1);
    }
}

void sGridFrameHelper::Color(sVector4 &col)
{
    Color(col,sVector4(0,0,0,0));
}

void sGridFrameHelper::Color(sVector4 &col,const sVector4 &def)
{
    sFloatControl *con[4];

    BeginLink();
    con[0] = new sFloatControl(&col.x,0,1,def.x);
    LinkControl(con[0]);
    con[1] = new sFloatControl(&col.y,0,1,def.y);
    LinkControl(con[1]);
    con[2] = new sFloatControl(&col.z,0,1,def.z);
    LinkControl(con[2]);
    EndLink();
    con[3] = new sFloatControl(&col.w,0,1,def.w);

    for(int i=0;i<4;i++)
    {
        AddLeft(con[i],Normal);
        if(i!=3)
            con[i]->BackColorFloatPtr = &col;
        con[i]->SetDrag(0.005f);
    }
}

void sGridFrameHelper::Color(sVector3 &col,const sVector4 &def)
{
    sFloatControl *con[4];

    BeginLink();
    con[0] = new sFloatControl(&col.x,0,1,def.x);
    LinkControl(con[0]);
    con[1] = new sFloatControl(&col.y,0,1,def.y);
    LinkControl(con[1]);
    con[2] = new sFloatControl(&col.z,0,1,def.z);
    LinkControl(con[2]);
    EndLink();

    for(int i=0;i<3;i++)
    {
        AddLeft(con[i],Normal);
        if(i!=3)
            con[i]->BackColorFloatPtr = (sVector4 *) &col;
        con[i]->SetDrag(0.005f);
    }
}

/****************************************************************************/
/***                                                                      ***/
/***   Overlapping Window                                                 ***/
/***                                                                      ***/
/****************************************************************************/

sOverlappedFrame::sOverlappedFrame()
{
    Flags |= sWF_Overlapped;
}

struct cmp
{ 
    inline bool operator()(const sWindow *a,const sWindow *b) const 
    { return a->Temp<b->Temp; }
};

void sOverlappedFrame::OnLayoutChilds()
{
    for(auto w : Childs)
    {
        w->OnLayoutOverlapped(Client);
        if(w->Flags & sWF_Back)
        {
            w->Temp = 0;
            w->Outer = Client;
        }
        else if(w->Flags & sWF_Top)
        {
            w->Temp = 3;

            // move popups / pulldowns into screen (if possible)

            int sx = w->Outer.SizeX();
            int sy = w->Outer.SizeY();

            if(w->Outer.x1 > Client.x1)
            {
                w->Outer.x0 = sMax(Client.x0,Client.x1-sx);
                w->Outer.x1 = w->Outer.x0+sx;
            }
            if(w->Outer.y1 > Client.y1)
            {
                w->Outer.y0 = sMax(Client.y0,Client.y1-sy);
                w->Outer.y1 = w->Outer.y0+sy;
            }
        }
        else
        {
            w->Temp = (w->Flags & sWF_ChildFocus) ? 2 : 1;
        }
    }
    //  Childs.QuickSort(cmp());
    Childs.InsertionSort(cmp());
    //  sInsertionSort(sAll(Childs),cmp());
}

/****************************************************************************/
/***                                                                      ***/
/***   Menu                                                               ***/
/***                                                                      ***/
/****************************************************************************/

sMenuFrameItem::sMenuFrameItem()
{
    KeyString = 0;
    Kind = sMIK_Error;
    Key = 0;
    Column = 0;
}

sMenuFrameItem::~sMenuFrameItem()
{
    delete[] KeyString;
}

/****************************************************************************/

sMenuFrame::sMenuFrame(sWindow *master)
{
    IndirectParent = master;
    AddBorder(new sThinBorder);
    DefaultColumn = 0;
    Flags |= sWF_Top;
    UpdatePool = 0;
}

sMenuFrame::~sMenuFrame()
{
    Items.DeleteAll();
}

sMenuFrameItem *sMenuFrame::AddSpacer()
{
    sMenuFrameItem *mi = new sMenuFrameItem;

    sWindow *b = new sMenuFrameSpacer();

    mi->Kind = sMIK_Spacer;
    mi->Window = b;
    mi->Column = DefaultColumn;

    Items.Add(mi);
    AddChild(mi->Window);
    return mi;
}

sMenuFrameItem *sMenuFrame::AddHeader(int column,const char *text,uptr len)
{
    sMenuFrameItem *mi = new sMenuFrameItem;
    DefaultColumn = column;

    sWindow *b = new sMenuFrameHeader(text,len);

    mi->Kind = sMIK_Header;
    mi->Window = b;
    mi->Column = DefaultColumn;

    Items.Add(mi);
    AddChild(mi->Window);

    return mi;
}

sMenuFrameItem *sMenuFrame::AddHeaderItem(uint key,const char *text,uptr len)
{
    if(key>='A' && key<='Z') key |= sKEYQ_Shift;

    sMenuFrameItem *mi = new sMenuFrameItem;

    sGuiMsg mymsg(this,&sMenuFrame::CmdPress,(int)Items.GetCount());

    sButtonControl *b = new sButtonControl(mymsg,text,sBS_Menu);
    b->LabelLen = len;
    b->Key = key;
    b->StyleKind = sSK_Button_MenuHeader;

    mi->Kind = sMIK_Push;
    mi->Column = DefaultColumn;
    mi->Window = b;
    mi->Key = key;

    Items.Add(mi);
    AddChild(mi->Window);
    if(key && sCleanKeyCode(key)>=0x20 && sCleanKeyCode(key)<=0xff)
        AddKey(text,key,mymsg);
    return mi;
}

sMenuFrameItem *sMenuFrame::AddItem(const sGuiMsg &msg,uint key,const char *text,uptr len,const char *prefix)
{
    if(key>='A' && key<='Z') 
        key |= sKEYQ_Shift;

    sMenuFrameItem *mi = new sMenuFrameItem;

    sGuiMsg mymsg(this,&sMenuFrame::CmdPress,(int)Items.GetCount());

    sButtonControl *b = new sButtonControl(mymsg,text,sBS_Menu);
    if(msg.IsEmpty())
        b->StyleKind = sSK_Button_MenuUnclickable;
    b->LabelLen = len;
    b->Key = key;

    mi->Kind = sMIK_Push;
    mi->Column = DefaultColumn;
    mi->Window = b;
    mi->Key = key;
    mi->Msg = msg;

    Items.Add(mi);
    AddChild(mi->Window);
    uint cleankey = key & ~(sKEYQ_Shift|sKEYQ_Ctrl|sKEYQ_Alt);
    if(key && cleankey>=0x20 && cleankey<=0xff)
        AddKey(text,key,mymsg);
    return mi;
}

sMenuFrameItem *sMenuFrame::AddToggle(int *ref,int val,int mask,const sGuiMsg &msg,uint key,const char *text,uptr len)
{
    if(key>='A' && key<='Z') key |= sKEYQ_Shift;

    sMenuFrameItem *mi = new sMenuFrameItem;

    sButtonControl *b = new sButtonControl(sGuiMsg(this,&sMenuFrame::CmdPressNoKill,(int)Items.GetCount()),text,sBS_MenuToggle);
    b->LabelLen = len;
    b->SetRef(ref,val,mask);
    b->Key = key;

    mi->Kind = sMIK_Check;
    mi->Column = DefaultColumn;
    mi->Window = b;
    mi->Key = key;
    mi->Msg = msg;

    Items.Add(mi);
    AddChild(mi->Window);
    if(UpdatePool)
        sUpdateConnection(&mi->Window->UpdateReceiver,UpdatePool);
    return mi;
}

sMenuFrameItem *sMenuFrame::AddRadio(int *ref,int val,int mask,const sGuiMsg &msg,uint key,const char *text,uptr len)
{
    sMenuFrameItem *mi = new sMenuFrameItem;

    sButtonControl *b = new sButtonControl(sGuiMsg(this,&sMenuFrame::CmdPressNoKill,(int)Items.GetCount()),text,sBS_MenuRadio);
    b->LabelLen = len;
    b->SetRef(ref,val,mask);
    b->Key = key;

    mi->Kind = sMIK_Check;
    mi->Column = DefaultColumn;
    mi->Window = b;
    mi->Key = key;
    mi->Msg = msg;

    Items.Add(mi);
    AddChild(mi->Window);
    if(UpdatePool)
        sUpdateConnection(&mi->Window->UpdateReceiver,UpdatePool);
    return mi;
}


sMenuFrameItem *sMenuFrame::AddGrid(sGridFrame *grid)
{
    sMenuFrameItem *mi = new sMenuFrameItem;

    mi->Kind = sMIK_Grid;
    mi->Column = DefaultColumn;
    mi->Window = grid;

    Items.Add(mi);
    AddChild(mi->Window);
    if(UpdatePool)
        sUpdateConnection(&mi->Window->UpdateReceiver,UpdatePool);
    return mi;
}

void sMenuFrame::StartMenu(bool up)
{
    Gui->AddPulldown(IndirectParent?IndirectParent->Screen:Gui->Screens[0],this,up);
}

void sMenuFrame::StartContext()
{
    Gui->AddPopup(IndirectParent?IndirectParent->Screen:Gui->Screens[0],this);
}

/****************************************************************************/

void sMenuFrame::OnCalcSize()
{
    colmask = 0;
    sClear(sx);
    sClear(sy);
    sClear(x0);
    sClear(x1);

    // columns sizes

    for(auto mi : Items)
    {
        sx[mi->Column] = sMax(sx[mi->Column],mi->Window->ReqSizeX);
        sy[mi->Column] += mi->Window->ReqSizeY;
        colmask |= 1U<<mi->Column;
    }

    // count columns in use

    int columns = 0;
    for(int i=0;i<sMI_MaxColumns;i++)
        if(colmask & (1U<<i))
            columns++;

    // window size

    ReqSizeX = columns-1;
    ReqSizeY = 0;
    for(int i=0;i<sMI_MaxColumns;i++)
    {
        ReqSizeX += sx[i];
        ReqSizeY = sMax(ReqSizeY,sy[i]);
    }

    // we are supposed to be in an overlapped window, so we can change our outer rect

    Outer.x1 = Outer.x0 + ReqSizeX + 2*Style()->ThinBorderSize;
    Outer.y1 = Outer.y0 + ReqSizeY + 2*Style()->ThinBorderSize;
}

void sMenuFrame::OnLayoutChilds()
{
    // left and right border of each columns

    int pos = 0;
    for(int i=0;i<sMI_MaxColumns;i++)
    {
        if(colmask & (1U<<i))
        {
            x0[i] = pos;
            pos += sx[i];
            x1[i] = pos;
            pos++;
        }
    }

    // layout individual windows

    sClear(sy);
    for(auto mi : Items)
    {
        mi->Window->Outer.x0 = Client.x0 + x0[mi->Column];
        mi->Window->Outer.x1 = Client.x0 + x1[mi->Column];
        mi->Window->Outer.y0 = Client.y0 + sy[mi->Column];
        sy[mi->Column] += mi->Window->ReqSizeY;
        mi->Window->Outer.y1 = Client.y0 + sy[mi->Column];
    }
}

void sMenuFrame::OnPaint(int layer)
{
    // paint the lines between the columns

    Painter()->SetLayer(layer);
    Painter()->Rect(Style()->Colors[sGC_Back],Client);
    Painter()->SetLayer(layer+1);
    for(int i=0;i<sMI_MaxColumns;i++)
    {
        if((colmask & (1U<<i)) && x0[i]!=0)
            Painter()->Rect(Style()->Colors[sGC_Draw],sRect(Client.x0+x0[i]-1,Client.y0,Client.x0+x0[i],Client.y1));
    }

    if(!(Flags & sWF_ChildFocus))
    {
        CmdKill();
    }
}



/****************************************************************************/

void sMenuFrame::CmdPress(int n)
{
    if(n>=0 && n<Items.GetCount())
    {
        sMenuFrameItem *mi = Items[n];
        if(mi->Kind==sMIK_Push || mi->Kind==sMIK_Check)
        {
            mi->Msg.Post();
            CmdKill();
//            Gui->SetFocus(Master);
        }
    }
}

void sMenuFrame::CmdPressNoKill(int n)
{
    if(n>=0 && n<Items.GetCount())
    {
        sMenuFrameItem *mi = Items[n];
        if(mi->Kind==sMIK_Push || mi->Kind==sMIK_Check)
        {
            mi->Msg.Post();
        }
    }
}

/****************************************************************************/
/****************************************************************************/

void sMenuFrameSpacer::OnCalcSize()
{
    ReqSizeY = 7;
}

void sMenuFrameSpacer::OnPaint(int layer)
{
    int y = Client.CenterY();
    int w = Style()->Font->CellHeight / 2;
    Painter()->SetLayer(layer);
    Painter()->Rect(Style()->Colors[sGC_Draw],sRect(Client.x0+w,y,Client.x1-w,y+1));
}

/****************************************************************************/

sMenuFrameHeader::sMenuFrameHeader(const char *label,uptr len)
{
    Label = label;
    LabelLen = len;  
}

void sMenuFrameHeader::OnCalcSize()
{
    ReqSizeX = Style()->Font->CellHeight + Style()->Font->GetWidth(Label,LabelLen);
    ReqSizeY = Style()->Font->CellHeight+3;
}

void sMenuFrameHeader::OnPaint(int layer)
{
    sRect r(Client);
    r.y1 -= 3;
    Painter()->SetLayer(layer);
    Painter()->Rect(Style()->Colors[sGC_Button],r);
    Painter()->SetLayer(layer+1);
    Painter()->SetFont(Style()->Font);
    Painter()->Text(sPPF_Left|sPPF_Space,Style()->Colors[sGC_Text],r,Label,LabelLen);
    Painter()->SetLayer(layer);
    r = Client;
    r.y0 = r.y1-3;
    r.y1 = r.y0+1;
    Painter()->Rect(Style()->Colors[sGC_Draw],r);
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/
}
