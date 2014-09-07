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
/***   Decorative Borders                                                 ***/
/***                                                                      ***/
/****************************************************************************/

void sNiceBorder::OnCalcSize()
{
    ReqSizeX = 8;
    ReqSizeY = 8;
}

void sNiceBorder::OnLayoutChilds()
{
    Inner.Extend(-Style()->NiceBorderSize);
}

void sNiceBorder::OnPaint(int layer)
{
    Style()->Rect(layer,this,sSK_NiceBorder,Outer);
}

/****************************************************************************/

void sFocusBorder::OnCalcSize()
{
    ReqSizeX = 2*Style()->FocusBorderSize;
    ReqSizeY = 2*Style()->FocusBorderSize;
}

void sFocusBorder::OnLayoutChilds()
{
    Inner.Extend(-Style()->FocusBorderSize);
}

void sFocusBorder::OnPaint(int layer)
{
    Style()->Rect(layer,this,sSK_FocusBorder,Outer,FocusParent && (FocusParent->Flags & sWF_ChildFocus) ? sSM_Pressed : 0);
}

/****************************************************************************/

void sThinBorder::OnCalcSize()
{
    ReqSizeX = 2*Style()->ThinBorderSize;
    ReqSizeY = 2*Style()->ThinBorderSize;
}

void sThinBorder::OnLayoutChilds()
{
    Inner.Extend(-Style()->ThinBorderSize);
}

void sThinBorder::OnPaint(int layer)
{
    Style()->Rect(layer,this,sSK_ThinBorder,Outer);
}

/****************************************************************************/
/***                                                                      ***/
/***   Toolbar (horizontal)                                               ***/
/***                                                                      ***/
/****************************************************************************/

sToolBorder::sToolBorder(bool top)
{
    Top = top;
}

void sToolBorder::OnCalcSize()
{
    ReqSizeX = 0;
    ReqSizeY = 0;
    for(auto w : Childs)
        ReqSizeY = sMax(ReqSizeY,w->DecoratedSizeY);
    ReqSizeY += Style()->ToolBorderSize;
}

void sToolBorder::OnLayoutChilds()
{
    if(Top)
        Inner.y0 += ReqSizeY;
    else
        Inner.y1 -= ReqSizeY;

    int pos = Client.x0;
    int y0 = Client.y0;
    int y1 = Client.y1;
    if(Top)
        y1 = y0+ReqSizeY-Style()->ToolBorderSize;
    else
        y0 = y1-ReqSizeY+Style()->ToolBorderSize;

    bool first = true;
    bool spaced = false;
    for(auto w : Childs)
    {
        if(!first && (w->Flags & sWF_GiveSpace) && !spaced)
            pos += Style()->Font->CellHeight;
        first = false;
        spaced = (w->Flags & sWF_GiveSpace)!=0;
        w->Outer.x0 = pos;
        pos += w->DecoratedSizeX;
        w->Outer.x1 = pos;
        w->Outer.y0 = y0;
        w->Outer.y1 = y1;
    }
}

void sToolBorder::OnPaint(int layer)
{
    if(Top)
    {
        sRect r(Outer);
        r.y1 = r.y0 + ReqSizeY;
        Style()->Rect(layer,this,sSK_ToolBorder_Top,r);
    }
    else
    {
        sRect r(Outer);
        r.y0 = r.y1 - ReqSizeY;
        Style()->Rect(layer,this,sSK_ToolBorder_Bottom,r);
    }
}

/****************************************************************************/
/***                                                                      ***/
/***   Status Border (Bottom)                                             ***/
/***                                                                      ***/
/****************************************************************************/

sStatusBorder::sStatusBorder()
{
}

void sStatusBorder::OnCalcSize()
{
    Height = Style()->SmallFont->Advance+1+2*Style()->GeneralSize;
    ReqSizeY = Height;
}

void sStatusBorder::OnLayoutChilds()
{
    Inner.y1 -= Height;
}

void sStatusBorder::OnPaint(int layer)
{
    int left = Client.SizeX();
    int spacers = 0;
    for(auto &i : Items)
    {
        left -= i.Size;
        if(i.Size==0)
            spacers++;
    }

    int x = Client.x0;
    int n = 0;
    for(auto &i : Items)
    {
        i.r.y0 = Client.y1-Height+1;
        i.r.y1 = Client.y1;
        i.r.x0 = x;
        x += i.Size;
        if(i.Size==0)
        {
            x += (n+1)*left/spacers - n*left/spacers;
            n++;
        }
        i.r.x1 = x;
    }

    auto pnt = Painter();
    auto sty = Style();

    sRect r = Client;
    r.y0 = r.y1 - Height;
    r.y1 = r.y0 + 1;
    pnt->SetLayer(layer+0);
    pnt->Rect(sty->Colors[sGC_Draw],r);
    int flag = 1;
    for(auto i : Items)
    {
        sty->Rect(layer,this,sSK_StatusBorder,i.r,flag,i.Text);
        flag = 0;
    }
}

int sStatusBorder::AddItem(int s)
{
    int n = Items.GetCount();
    Items.AddTail(Item(s));
    return n;
}

void sStatusBorder::Print(int n,const char *text)
{
    Items[n].Text = text;
    Update();
}

/****************************************************************************/
/***                                                                      ***/
/***   For overlapped frames                                              ***/
/***                                                                      ***/
/****************************************************************************/

sSizeBorder::sSizeBorder()
{
    DragMode = 0;
    MouseCursor = sHCI_SizeAll;
    AddDrag("resize",sKEY_LMB,sGuiMsg(this,&sSizeBorder::Drag));
}

void sSizeBorder::OnCalcSize()
{
    ReqSizeX = 2*Style()->SizeBorderSize;
    ReqSizeY = 2*Style()->SizeBorderSize;
}

void sSizeBorder::OnLayoutChilds()
{
    Inner.Extend(-Style()->SizeBorderSize);
}

void sSizeBorder::OnPaint(int layer)
{
    Style()->Rect(layer,this,sSK_SizeBorder,Outer,DragMode);
}

void sSizeBorder::Drag(const sDragData &dd)
{
    if(dd.Mode==sDM_Start && dd.Buttons==1)
    {
        DragStart = BorderParent->Outer;
        sRect bits(DragStart);
        bits.Extend(-20);
        if(bits.x0>bits.x1) bits.x0 = bits.x1 = DragStart.CenterX();
        if(bits.y0>bits.y1) bits.y0 = bits.y1 = DragStart.CenterY();

        DragMode = 0;
        if(dd.StartX<bits.x0) DragMode |= 1;
        if(dd.StartX>bits.x1) DragMode |= 2;
        if(dd.StartY<bits.y0) DragMode |= 4;
        if(dd.StartY>bits.y1) DragMode |= 8;
    }

    if(dd.Mode==sDM_Drag && DragMode)
    {
        int dx = dd.DeltaX;
        int dy = dd.DeltaY;
        int minx = 100;
        int miny = 50;
        sRect limit(Screen->Root->Outer);

        if(DragMode & 1) if(dx + DragStart.x0 < limit.x0) dx = limit.x0 - DragStart.x0;
        if(DragMode & 2) if(dx + DragStart.x1 > limit.x1) dx = limit.x1 - DragStart.x1;
        if(DragMode & 4) if(dy + DragStart.y0 < limit.y0) dy = limit.y0 - DragStart.y0;
        if(DragMode & 8) if(dy + DragStart.y1 > limit.y1) dy = limit.y1 - DragStart.y1;

        if((DragMode & 3) == 1) if(DragStart.SizeX()-dx < minx) dx =  DragStart.SizeX() - minx;
        if((DragMode & 3) == 2) if(DragStart.SizeX()+dx < minx) dx = -DragStart.SizeX() + minx;
        if((DragMode &12) == 4) if(DragStart.SizeY()-dy < miny) dy =  DragStart.SizeY() - miny;
        if((DragMode &12) == 8) if(DragStart.SizeY()+dy < miny) dy = -DragStart.SizeY() + miny;

        if(DragMode & 1) BorderParent->Outer.x0 = DragStart.x0 + dx;
        if(DragMode & 2) BorderParent->Outer.x1 = DragStart.x1 + dx;
        if(DragMode & 4) BorderParent->Outer.y0 = DragStart.y0 + dy;
        if(DragMode & 8) BorderParent->Outer.y1 = DragStart.y1 + dy;
        Gui->Layout();
        Gui->UpdateClient(this);
    }

    if(dd.Mode==sDM_Stop)
        DragMode = 0;
}

/****************************************************************************/

sTitleBorder::sTitleBorder()
{
    Text = 0;
    DragMode = 0;
    MouseCursor = sHCI_Hand;
    AddDrag("move",sKEY_LMB,sGuiMsg(this,&sTitleBorder::Drag));
}

sTitleBorder::sTitleBorder(const char *text)
{
    Text = text;
    DragMode = 0;
    MouseCursor = sHCI_Hand;
    AddDrag("move",sKEY_LMB,sGuiMsg(this,&sTitleBorder::Drag));
}


void sTitleBorder::OnCalcSize()
{
    ReqSizeY = Style()->TitleBorderSize;
}

void sTitleBorder::OnLayoutChilds()
{
    Inner.y0 += ReqSizeY;

    int y0 = Outer.y0 + 1;
    int y1 = Outer.y0 + ReqSizeY - 3;
    int pos = Outer.x1;

    for(auto w : Childs)
    {
        w->Outer.y0 = y0;
        w->Outer.y1 = y1;
        w->Outer.x1 = pos;
        pos -= w->ReqSizeX;
        w->Outer.x0 = pos;
    }
}

void sTitleBorder::OnPaint(int layer)
{
    Style()->Rect(layer,this,sSK_TitleBorder,Outer,DragMode ? sSM_Pressed : 0,Text);
}

void sTitleBorder::Drag(const sDragData &dd)
{
    if(dd.Mode==sDM_Start && dd.Buttons==1)
    {
        DragMode = 1;
        DragStart = BorderParent->Outer;
    }

    if(dd.Mode==sDM_Drag && DragMode)
    {
        int dx = dd.DeltaX;
        int dy = dd.DeltaY;
        sRect limit(Screen->Root->Outer);

        if(dx + DragStart.x0 < limit.x0) dx = limit.x0 - DragStart.x0;
        if(dx + DragStart.x1 > limit.x1) dx = limit.x1 - DragStart.x1;
        if(dy + DragStart.y0 < limit.y0) dy = limit.y0 - DragStart.y0;
        if(dy + DragStart.y1 > limit.y1) dy = limit.y1 - DragStart.y1;

        BorderParent->Outer.x0 = DragStart.x0 + dx;
        BorderParent->Outer.y0 = DragStart.y0 + dy;
        BorderParent->Outer.x1 = DragStart.x1 + dx;
        BorderParent->Outer.y1 = DragStart.y1 + dy;
        Gui->Layout();
        Gui->UpdateClient(this);
    }

    if(dd.Mode==sDM_Stop)
        DragMode = 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   Scrolling                                                          ***/
/***                                                                      ***/
/****************************************************************************/

sScrollBorder::sScrollBorder(bool x,bool y)
{
    DragStartX = 0;
    DragStartY = 0;
    DragMode = 0;
    EnableX = x;
    EnableY = y;

    AddDrag("scroll",sKEY_LMB,sGuiMsg(this,&sScrollBorder::Drag));
}

void sScrollBorder::OnCalcSize()
{
    int w = Style()->ScrollBorderSize;
    if(EnableX)
        ReqSizeY = w;
    if(EnableY)
        ReqSizeX = w;
}

void sScrollBorder::OnLayoutChilds()
{
    int w = Style()->ScrollBorderSize;
    RectX.Set();
    RectY.Set();
    RectC.Set();
    BarX.Set();
    BarY.Set();
    if(EnableX && EnableY)
    {
        RectX = RectY = RectC = Outer;
        RectX.y0 = RectX.y1-w;
        RectX.x1 = RectX.x1-w;
        RectY.x0 = RectY.x1-w;
        RectY.y1 = RectY.y1-w;
        RectC.y0 = RectC.y1-w;
        RectC.x0 = RectC.x1-w;

        Inner.x1 -= w;
        Inner.y1 -= w;
    }
    else if(EnableX)
    {
        RectX = Outer;
        RectX.y0 = RectX.y1-w;

        Inner.y1 -= w;
    }
    else if(EnableY)
    {
        RectY = Outer;
        RectY.x0 = RectY.x1-w;

        Inner.x1 -= w;
    }
}

void sScrollBorder::OnPaint(int layer)
{
    sRect r;

    uint lo = Style()->Colors[sGC_Low];
    uint hi = Style()->Colors[sGC_High];
    //  uint ba = Style()->Colors[sGC_Back];
    uint bu = Style()->Colors[sGC_Button];
    int f = Style()->ScrollBorderFrameSize;
    sWindow *root = BorderParent;
    BarX.Set();
    BarY.Set();

    if(EnableX)
    {
        Painter()->SetLayer(layer);
        r = RectX;
        Painter()->Frame(hi,lo,r,f);
        r.Extend(-f);
        Style()->Rect(layer,this->FocusParent,sSK_Back,r);
        //    Painter()->Rect(ba,r);

        if(root->Inner.SizeX()<root->ReqSizeX && root->Inner.SizeX()>0)
        {
            int s = r.SizeX();
            int h = sMax(root->Inner.SizeX()*s/root->ReqSizeX, sMin(Style()->ScrollBorderSize,s/2));
            r.x0 = r.x0 + root->ScrollX * (s-h) / (root->ReqSizeX-root->Inner.SizeX());
            r.x1 = r.x0 + h;
            BarX = r;

            Painter()->SetLayer(layer+1);
            Painter()->Frame(hi,lo,r,f);
            r.Extend(-f);
            Painter()->Rect(bu,r);
        }
    }
    if(EnableY)
    {
        Painter()->SetLayer(layer);
        r = RectY;
        Painter()->Frame(hi,lo,r,f);
        r.Extend(-f);
        Style()->Rect(layer,this->FocusParent,sSK_Back,r);
        //    Painter()->Rect(ba,r);

        if(root->Inner.SizeY()<root->ReqSizeY && root->Inner.SizeY()>0)
        {
            int s = r.SizeY();
            int h = sMax(root->Inner.SizeY()*s/root->ReqSizeY, sMin(Style()->ScrollBorderSize,s/2));
            r.y0 = r.y0 + root->ScrollY * (s-h) / (root->ReqSizeY-root->Inner.SizeY());
            r.y1 = r.y0 + h;
            BarY = r;

            Painter()->SetLayer(layer+1);
            Painter()->Frame(hi,lo,r,f);
            r.Extend(-f);
            Painter()->Rect(bu,r);
        }
    }
    if(EnableX && EnableY)
    {
        Painter()->SetLayer(layer);
        Style()->Rect(layer,this->FocusParent,sSK_Back,RectC);
        //    Painter()->Rect(ba,RectC);
    }
}

void sScrollBorder::Drag(const sDragData &dd)
{
    sWindow *root = BorderParent;
    if(dd.Mode==sDM_Start && dd.Buttons==1) 
    {
        DragStartX = root->ScrollX;
        DragStartY = root->ScrollY;
        DragMode = 0;
        if(BarX.Hit(dd.StartX,dd.StartY))
            DragMode = 1;
        if(BarY.Hit(dd.StartX,dd.StartY))
            DragMode = 2;
        if(DragMode==0)
        {
            if(RectX.Hit(dd.StartX,dd.StartY))
            {
                if(dd.StartX<BarX.x0)
                    root->ScrollX -= root->Inner.SizeX();
                if(dd.StartX>BarX.x1)
                    root->ScrollX += root->Inner.SizeX();
                Gui->Layout();
                root->Update();
            }
            if(RectY.Hit(dd.StartX,dd.StartY))
            {
                if(dd.StartY<BarY.y0)
                    root->ScrollY -= root->Inner.SizeY();
                if(dd.StartY>BarY.y1)
                    root->ScrollY += root->Inner.SizeY();
                Gui->Layout();
                root->Update();
            }
        }
    }
    if(dd.Mode==sDM_Stop)
        DragMode = 0;
    if(dd.Mode==sDM_Drag && DragMode==1)
    {
        root->ScrollX = DragStartX + dd.DeltaX * (root->ReqSizeX-root->Inner.SizeX()) / (RectX.SizeX()-Style()->ScrollBorderFrameSize*2-BarX.SizeX());
        Gui->Layout();
        root->Update();
    }
    if(dd.Mode==sDM_Drag && DragMode==2)
    {
        root->ScrollY = DragStartY + dd.DeltaY * (root->ReqSizeY-root->Inner.SizeY()) / (RectY.SizeY()-Style()->ScrollBorderFrameSize*2-BarY.SizeY());
        Gui->Layout();
        root->Update();
    }
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void sTabBorderBase::Construct()
{
    Flags |= sWF_Border;
    Array = 0;
    Info = 0;
    WindowSelect = 0;
    GlobalSelect = 0;
    Selected = 0;
    Pressed = 0;
    DelayedDelete = false;
    DropGroup = 0;
    BoxesDirty = false;

    AddDrag("Select",sKEY_LMB,sGuiMsg(this,&sTabBorderBase::DragSelect));
}

sTabBorderBase::sTabBorderBase()
{
    Construct();
}

sTabBorderBase::sTabBorderBase(sArray<sTabDummyElement *> &a,sTabElementInfo sTabDummyElement::*info,sTabDummyElement **windowselect,sTabDummyElement **globalselect)
{
    Construct();
    Array = &a;
    Info = info;
    WindowSelect = windowselect;
    GlobalSelect = globalselect;
}

void sTabBorderBase::SetArray(sArray<sTabDummyElement *> &a,sTabElementInfo sTabDummyElement::*info)
{
    Array = &a;
    Info = info;
    Selected = 0;
}

void sTabBorderBase::SetWindowSelect(sTabDummyElement **e)
{
    WindowSelect = e;
}

void sTabBorderBase::SetGlobalSelect(sTabDummyElement **e)
{
    GlobalSelect = e;
}

/****************************************************************************/

void sTabBorderBase::OnCalcSize()
{
    ReqSizeX = 0;
    ReqSizeY = Style()->Font->CellHeight+4+Style()->ToolBorderSize;
}

void sTabBorderBase::OnLayoutChilds()
{
    Inner.y0 += ReqSizeY;
    Client.y1 = Inner.y0;
    UpdateElements();
}

void sTabBorderBase::UpdateElements()
{
    if(Screen==0)
        return;
    auto sty = Style();
    int x = Client.x0;
    ClearDragDrop();
    int n = 0;
    for(sTabDummyElement *e : *Array)
    {
        sTabElementInfo &info = e->*Info;
        int w = sty->Font->GetWidth(info.Name) + sty->Font->Advance*2;
        info.Client.x0 = x+4;
        info.Client.y0 = Client.y0;
        x += w;
        info.Client.x1 = x-4;
        info.Client.y1 = Client.y1;

        info.HoverDelete = false;

        sDragDropIcon dd(info.Client,1,n++,info.Name);
        AddDragDrop(dd);
    }

    BoxesDirty = false;
}

int sTabBorderBase::FindDragDrop(const sDragDropIcon &icon,int mx,int my)
{
    int n = 0;
    int drophere = -1;
    for(sTabDummyElement *e : *Array)
    {
        sTabElementInfo &info = e->*Info;
        if(mx < info.Client.CenterX())
            break;
        n++;
    }
    drophere = n;

    auto *src = sTryCast<sTabBorderBase>(icon.SourceWin);
    if(src && icon.Command==1)
    {
        if(src==this)
        {
            if(drophere<icon.Index || drophere>icon.Index+1)
                return drophere;
            return -1;
        }
        else if(DropGroup && DropGroup==src->DropGroup)
        {
            return drophere;
        }
    }
    return -1;
}

void sTabBorderBase::OnPaint(int layer)
{
    auto sty = Style();
    auto pnt = Painter();

    if(BoxesDirty)
        UpdateElements();

    sRect r(Outer);
    r.y1 = r.y0 + ReqSizeY;

    sty->Rect(layer,this,sSK_ToolBorder_Top,r);

    sDragDropIcon icon;
    int mx,my;
    int drophere = -1;
    if(Gui->GetDragDropInfo(this,icon,mx,my))
        drophere = FindDragDrop(icon,mx,my);

    if(drophere==0)
    {
        pnt->SetLayer(layer+1);
        pnt->Rect(sty->Colors[sGC_HoverFeedback],sRect(Client.x0,Client.y0,Client.x0+2,Client.y1));
    }

    int n = 0;
    for(sTabDummyElement *e : *Array)
    {
        sTabElementInfo &info = e->*Info;
        int mod = 0;
        if(Pressed && e==Selected)
            mod |= sSM_Pressed;
        if(e==*WindowSelect)
            mod |= sSM_WindowSel;
        if(e==*GlobalSelect)
            mod |= sSM_GlobalSel;
        if(info.HoverDelete && (Flags & sWF_Hover))
            mod |= sSM_HoverIcon;

        sty->Rect(layer,this,sSK_TabItem,info.Client,mod,info.Name);

        n++;
        if(n==drophere)
        {
            int x = info.Client.x1 + 4;
            pnt->SetLayer(layer+1);
            pnt->Rect(sty->Colors[sGC_HoverFeedback],sRect(x-1,Client.y0,x+1,Client.y1));
        }
    }
}

void sTabBorderBase::DragSelect(const sDragData &dd)
{
    if(dd.Mode==sDM_Start)
    {
        DelayedDelete = false;
        for(sTabDummyElement *e : *Array)
        {
            sTabElementInfo &info = e->*Info;
            if(info.Client.Hit(dd.PosX,dd.PosY))
            {
                Selected = e;
                Pressed = true;
                SelectMsg.Post();
                if(info.HoverDelete)
                    DelayedDelete = true;
            }
        }
    }
    if(dd.Mode==sDM_Stop)
    {
        if(DelayedDelete && Selected && (Selected->*Info).HoverDelete)
            DeleteMsg.Post();
        Pressed = false;
    }
}

void sTabBorderBase::OnHover(const sDragData &dd)
{
    for(sTabDummyElement *e : *Array)
    {
        sTabElementInfo &info = e->*Info;
        info.HoverDelete = false;
        sRect r = info.Client;
        r.x0 = r.x1 - Style()->Font->Advance;
        info.HoverDelete = r.Hit(dd.PosX,dd.PosY);
    }
}

void sTabBorderBase::OnDragDropFrom(const sDragDropIcon &icon,bool remove)
{
}

bool sTabBorderBase::OnDragDropTo(const sDragDropIcon &icon,int &mx,int &my,bool checkonly)
{
    auto *src = sTryCast<sTabBorderBase>(icon.SourceWin);
    if(!src) return false;
    int drophere = FindDragDrop(icon,mx,my);

    if(drophere>=0 && drophere<=Array->GetCount() && icon.Command==1)
    {
        if(src==this)
        {
            if(!checkonly)
            {
                auto obj = (*Array)[icon.Index];
                Array->RemAtOrder(icon.Index);
                if(drophere > icon.Index)
                    drophere--;
                Array->AddAt(obj,drophere);
                BoxesDirty = true;
            }
            return true;
        }
        if(src!=this && DropGroup && DropGroup==src->DropGroup)
        {
            if(!checkonly)
            {
                auto obj = (*src->Array)[icon.Index];
                src->Array->RemAtOrder(icon.Index);
                Array->AddAt(obj,drophere);
                src->BoxesDirty = true;
                BoxesDirty = true;
            }
            return true;
        }
    }

    return false;
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/
}
