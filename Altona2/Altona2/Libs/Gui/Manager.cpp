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

sGuiManager *Gui;

/****************************************************************************/
/***                                                                      ***/
/***   Initialization Procedure                                           ***/
/***                                                                      ***/
/****************************************************************************/

void sRunGui(sGuiInitializer *appinit,const sScreenMode *smp)
{
    sRunGui(appinit,smp,new sGuiManager(appinit));
}

void sRunGui(sGuiInitializer *appinit,const sScreenMode *smp,sGuiManager *gui)
{
    Gui = gui;

    sString<sFormatBuffer> buffer;
    appinit->GetTitle(buffer);

    int flags = 0;
    //  flags |= sSM_PartialUpdate;
    //  flags |= sSM_Fullscreen;
    sScreenMode sm(flags,buffer,1280,720);
    if(smp)
    {
        sm = *smp;
        sm.WindowTitle = buffer;
        sm.Flags |= flags;
    }
    sRunApp(Gui,sm);
}


/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sGuiAdapter::sGuiAdapter()
{
    Adapter = 0;
    Context = 0;
    Painter = 0;
    Style = 0;
    IconAtlas = 0;
}

sGuiAdapter::sGuiAdapter(sAdapter *ada,sGuiInitializer *initapp)
{
    Adapter = ada;
    Context = ada->ImmediateContext;
    Painter = new sPainter(ada);
    Style = initapp->CreateStyle(this);
    IconAtlas = Gui->IconManager.GetAtlas(ada);
}


sGuiAdapter::~sGuiAdapter()
{
    delete Painter;
    delete Style;
    delete IconAtlas;
}

/****************************************************************************/

sGuiScreen::sGuiScreen()
{
    Root = 0;
    GuiAdapter = 0;
    BufferSequence = 0;
}

sGuiScreen::~sGuiScreen()
{
}

/****************************************************************************/

sRegularGuiScreen::sRegularGuiScreen(sScreen *scr)
{
    Screen = scr;
    BufferSequence = scr->ScreenMode.BufferSequence;
}

sRegularGuiScreen::~sRegularGuiScreen()
{
}

void sRegularGuiScreen::GetSize(int &sx,int &sy)
{
    sx = Screen->ScreenMode.SizeX;
    sy = Screen->ScreenMode.SizeY;
}

bool sRegularGuiScreen::GetTargetPara(sTargetPara &tp)
{
    tp = sTargetPara(sTAR_ClearAll,0,Screen);
    return 1;
}

bool sRegularGuiScreen::IsRightScreen(sScreen *scr)
{
    return Screen==scr;
}


/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sGuiManager::sGuiManager(sGuiInitializer *ai)
{
    AppInit = ai;
    /*
    Painter = 0;
    Style = 0;
    Root = 0;

    Screen = sGetScreen();
    Adapter = Screen->Adapter;
    Context = Adapter->ImmediateContext;
    */

    Focus = 0;
    CurrentDrag = 0;
    HoverFocus = 0;
    HoverWindow = 0;
    ToolTipTimer= 0;
    ToolTipLocked = 0;
    ToolTipX = 0;
    ToolTipY = 0;
    LayoutFlag = 1;
    KillFlag = 0;
    MaxLayer = 0;
    TouchMode = 0;
    DragDropIcon = 0;

    QuickMsg = 0;
    QuickMsgEndTime = 0;

    sBuffersLostHook.Add(sDelegate<void>(this,&sGuiManager::Layout));
    sGC->TagHook.Add(sDelegate<void>(this,&sGuiManager::TagWindows));
}

sGuiManager::~sGuiManager()
{
    sBuffersLostHook.Rem(sDelegate<void>(this,&sGuiManager::Layout));
    sGC->TagHook.Rem(sDelegate<void>(this,&sGuiManager::TagWindows));
    delete AppInit;
}

/****************************************************************************/

void sGuiManager::OnInit()
{

    sWindow *root = OpenScreen(sGetScreen());
    sGuiScreen  *gscr = root->Screen;;

    sWindow *w = AppInit->CreateRoot();
    w->Flags |= sWF_Back;
    gscr->Root->AddChild(w);
    SetFocus(w);
}

sWindow *sGuiManager::OpenScreen(sScreen *screen)
{
    sRegularGuiScreen  *gscr = new sRegularGuiScreen(screen);

    sGuiAdapter *gada = 0;
    for(auto i : Adapters)
    {
        if(i->Adapter == screen->Adapter)
        {
            gada = i;
            break;
        }
    }
    if(!gada)
    {
        gada = new sGuiAdapter(screen->Adapter,AppInit);;
        Adapters.Add(gada);
    }

    gscr->GuiAdapter = gada;
    gscr->Root = new sOverlappedFrame();
    gscr->Root->Screen = gscr;
    gscr->Root->OnInit();

    Screens.Add(gscr);

    return gscr->Root;
}

sWindow *sGuiManager::OpenScreen(const sScreenMode &sm)
{
    return OpenScreen(new sScreen(sm));
}

void sGuiManager::OnExit()
{
    sArray<sWindow *> allwindows;
    ListWindows(allwindows);
    allwindows.DeleteAll();

    Screens.DeleteAll();
    Adapters.DeleteAll();
}

bool sGuiManager::OnClose()
{
    sKeyData kd;
    kd.Key = sKEY_Escape | sKEYQ_Shift;
    kd.MouseX = 0;
    kd.MouseY = 0;
    kd.Timestamp = 0;
    OnKey(kd);
    return 0;
}

void sGuiManager::OnFrame()
{
}

void sGuiManager::LayoutR0(sWindow *w)
{
    for(auto c : w->Childs)
        LayoutR0(c);
    for(auto c : w->Borders)
        LayoutR0(c);

    for(int i=0;i<w->Childs.GetCount();)
    {
        if(w->Childs[i]->Flags & sWF_Kill)
            w->Childs.RemAt(i);
        else
            i++;
    }
    for(int i=0;i<w->Borders.GetCount();)
    {
        if(w->Borders[i]->Flags & sWF_Kill)
            w->Borders.RemAt(i);
        else
            i++;
    }
}

void sGuiManager::CalcSize(sWindow *w)
{
    LayoutR1(w,w->Screen);
}

void sGuiManager::LayoutR1(sWindow *w,sGuiScreen *scr)
{
    if(!(w->Flags & sWF_Disable))
    {
        if(w->Flags & sWF_Paint3d)
            Paint3dWindows.Add(w);

        for(auto c : w->Borders)
            LayoutR1(c,scr);
        if(w->LayoutShortcut)
            LayoutR1(w->LayoutShortcut,scr);
        else
            for(auto c : w->Childs)
                LayoutR1(c,scr);

        w->OnCalcSize();

        w->DecoratedSizeX = w->ReqSizeX;
        w->DecoratedSizeY = w->ReqSizeY;
        for(auto c : w->Borders)
        {
            w->DecoratedSizeX += c->ReqSizeX;
            w->DecoratedSizeY += c->ReqSizeY;
        }
    }
}

void sGuiManager::LayoutR2(sWindow *w)
{
    if(!(w->Flags & sWF_Disable))
    {
        for(auto c : w->Borders)
        {
            c->FocusParent = (c->Flags & sWF_Disable) ? 0 : w;
            c->BorderParent = (w->Flags & sWF_Border) ? w->BorderParent : w;
        }

        w->Inner = w->Outer;
        for(auto c : w->Borders)
        {
            c->Outer = w->Inner;
            LayoutR2(c);
            w->Inner = c->Inner;
        }

        w->Client = w->Inner;
        if(w->Flags & sWF_ScrollX)
        {
            w->Client.x1 = sMax(w->Inner.x1,w->Inner.x0+w->ReqSizeX);
            if(w->ScrollX+w->Inner.SizeX() > w->Client.SizeX()) 
                w->ScrollX = w->Client.SizeX()-w->Inner.SizeX();
            if(w->ScrollX<0) 
                w->ScrollX = 0;
            w->Client.x0 -= w->ScrollX;
            w->Client.x1 -= w->ScrollX;
        }
        if(w->Flags & sWF_ScrollY)
        {
            w->Client.y1 = sMax(w->Inner.y1,w->Inner.y0+w->ReqSizeY);
            if(w->ScrollY+w->Inner.SizeY() > w->Client.SizeY()) 
                w->ScrollY = w->Client.SizeY()-w->Inner.SizeY();
            if(w->ScrollY<0) 
                w->ScrollY = 0;
            w->Client.y0 -= w->ScrollY;
            w->Client.y1 -= w->ScrollY;
        }

        if(w->Client.x0 > w->Client.x1) w->Client.x0 = w->Client.x1;
        if(w->Client.y0 > w->Client.y1) w->Client.y0 = w->Client.y1;

        if(w->LayoutShortcut)
        {
            sWindow *l = w->LayoutShortcut;
            l->Outer = w->Client;
            l->FocusParent = w;
            l->Flags &= ~sWF_Disable;
            LayoutR2(l);
        }
        else
        {
            w->OnLayoutChilds();

            for(auto c : w->Childs)
            {
                c->BorderParent = w;
                if(c->Flags & sWF_Disable)
                {
                    c->FocusParent = 0;
                }
                else
                {
                    c->FocusParent = w;
                    LayoutR2(c);
                }
            }
        }
    }
}

void sGuiManager::DoLayout()
{
    sZONE("Gui Layout",0xffff4040);

    // possibly seek out windows that want to be killed

    if(KillFlag)
    {
		sWindow *newFocus = 0;
        sArray<sWindow *> before;
        sArray<sWindow *> after;

        ListWindows(before);
        for(auto scr : Screens)
            LayoutR0(scr->Root);
        ListWindows(after);

        for(auto w : after)
            before.Rem(w);
        for(auto w : before)
        {
            if(w==Focus) 
			{
				auto ww = w;
				while(ww->IndirectParent==0 && ww->FocusParent!=0)
					ww = ww->FocusParent;
				if(ww->IndirectParent)
					newFocus = ww->IndirectParent;
			}
		}
        for(auto w : before)
        {
            if(w==Focus) 
                Focus = 0;
            if(w==HoverFocus) 
                HoverFocus = 0;
            if(w==HoverWindow) 
                HoverWindow = 0;
            if(CurrentDrag && w==CurrentDrag->Message.GetTarget())
                CurrentDrag = 0;
            delete w;
        }
        KillFlag = 0;
        Update();
        Layout();

		if(newFocus && after.FindEqual(newFocus))
			SetFocus(newFocus);
    }

    // do the layout

    if(LayoutFlag)
    {
        ClearDragDrop(0);
        Paint3dWindows.Clear();
        for(auto scr : Screens)
        {
            scr->Root->BorderParent = 0;
            scr->Root->FocusParent = 0;
            scr->Root->OnCalcSize();
            LayoutR1(scr->Root,scr);
            LayoutR2(scr->Root);
            LayoutFlag = 0;
        }
    }
}

void sGuiManager::PaintR(sWindow *w,int layer,const sRect &clip,sGuiScreen *gscr)
{
    if(!(w->Flags & sWF_Disable))
    {
        int layers = (w->Flags & sWF_ManyLayers) ? 0x1000 : 16;

        sRect outerclip(clip);
        outerclip.And(w->Outer);

        sRect innerclip(outerclip);
        if(!(w->Flags & sWF_Border))
            innerclip.And(w->Inner);

        if(!outerclip.IsEmpty() && outerclip.IsPartiallyInside(w->Screen->ScissorRect))
        {
            for(auto c : w->Borders)
                PaintR(c,layer+layers,outerclip,gscr);
        }

        if(w->LayoutShortcut)
            w = w->LayoutShortcut;

        if((w->Flags & sWF_AutoKill) && !(w->Flags & sWF_ChildFocus))
            w->CmdKill();

        if(!innerclip.IsEmpty() && innerclip.IsPartiallyInside(w->Screen->ScissorRect))
        {
            MaxLayer = sMax(layer+layers,MaxLayer);
            w->Painter()->SetClip(innerclip);
            w->OnPaint(layer);
            if(w->Flags & sWF_Overlapped)
            {
                int old = MaxLayer;
                MaxLayer = layer+layers;
                for(auto c : w->Childs)
                    PaintR(c,MaxLayer,innerclip,gscr);
                MaxLayer = old;
            }
            else
            {
                for(auto c : w->Childs)
                    PaintR(c,layer+layers,innerclip,gscr);
            }
        }
    }
}

void sGuiManager::OnPaint()
{

    for(auto gada : Adapters)
        gada->Style->OnFrame();

    // process post queue

    sArray<sGuiMsg> msgs;
    msgs.Add(PostQueue);
    PostQueue.Clear();
    for(auto &msg : msgs)
        msg.Send();

    // paint

    for(auto gscr : Screens)
    {
        int sx,sy;
        gscr->GetSize(sx,sy);
        sRect r(0,0,sx,sy);

        sWindow *root = gscr->Root;

        if(root->Outer!=r)
        {
            root->Outer = r;
            Layout();
            Update(gscr);
        }
    }

    DoLayout();

    for(auto w : Paint3dWindows)
        w->OnPaint3d();

    sZONE("Gui Painting",0xffff8080);
    sGPU_ZONE("Gui Painting",0xffff8080);

    for(auto gscr : Screens)
    {
        int sx,sy;
        gscr->GetSize(sx,sy);
        sRect r(0,0,sx,sy);

        sPainter *pnt = gscr->GuiAdapter->Painter;
        sGuiStyle *sty = gscr->GuiAdapter->Style;
        sWindow *root = gscr->Root;
        sContext *ctx = gscr->GuiAdapter->Context;

        gscr->SequenceIndex++;
        gscr->SequenceRects[gscr->SequenceIndex&3] = gscr->UpdateRect;
        if(gscr->BufferSequence == 0)
        {
            gscr->ScissorRect = r;
        }
        else
        {
            gscr->ScissorRect = gscr->UpdateRect;
            if(gscr->BufferSequence>1)
                for(int i=1;i<gscr->BufferSequence;i++)
                    gscr->ScissorRect.Add(gscr->SequenceRects[(gscr->SequenceIndex-i)&3]);
        }
        if(sGUI_ConstantUpdate)
            gscr->ScissorRect = r;
        sRect sr(gscr->ScissorRect);
        gscr->UpdateRect.Init(0,0,0,0);

        if(!gscr->ScissorRect.IsEmpty())
        {
            if(sGUI_RegionUpdateDebug)
                gscr->ScissorRect = gscr->Root->Outer;
            sTargetPara tp;
            if(gscr->GetTargetPara(tp))
            {
                tp.Flags = sTAR_ClearDepth;
                tp.ClearColor[0].SetColor(0xff405060);
                if(gscr->BufferSequence == 0)
                    tp.Flags = sTAR_ClearAll;
                ctx->BeginTarget(tp);
                ctx->SetScissor(1,gscr->ScissorRect);

                pnt->Begin(tp);
                PaintR(gscr->Root,0,gscr->Root->Outer,gscr);

                if(QuickMsg)
                {
                    int xs = sty->Font->GetWidth(QuickMsg) + sty->Font->CellHeight*2;
                    int ys = sty->Font->CellHeight*2;

                    sRect r;
                    r.x0 = root->Client.CenterX()-xs/2;
                    r.x1 = r.x0 + xs;
                    r.y0 = root->Client.CenterY()-ys/2;
                    r.y1 = r.y0 + ys;
                    pnt->ResetClip();
                    pnt->SetLayer(0x2010);
                    pnt->Frame(sty->Colors[sGC_Draw],r);
                    r.Extend(-1);
                    pnt->Rect(sty->Colors[sGC_Select],r);
                    pnt->SetLayer(0x2011);
                    pnt->SetFont(sty->Font);
                    pnt->Text(0,sty->Colors[sGC_Draw],r,QuickMsg);

                    if(sGetTimeMS()>QuickMsgEndTime)
                    {
                        QuickMsg = 0;
                    }
                }
                if(DragDropIcon)
                {
                    pnt->ResetClip();
                    sty->DragDropIcon = DragDropIcon;
                    sRect r(DragDropIcon->Client);
                    r.Move(LastDeltaX,LastDeltaY);
                    sty->Rect(0x2000,0,sSK_DragDropIcon,r,sSM_Grayed,DragDropIcon->Text);
                }
                if(sGUI_RegionUpdateDebug)
                {
                    pnt->SetLayer(127);
                    pnt->ResetTransform();
                    pnt->ResetClip();
                    pnt->Frame(0xffff0000,sr);
                }

                if(HoverFocus)
                {
                    if(ToolTipLocked)
                    {
                        HoverFocus->OnPaintToolTip(0x1ff0,ToolTipX,ToolTipY,sGetTimeMS()-ToolTipTimer);
                    }
                    else
                    {
                        if(HoverFocus->OnPaintToolTip(0x1ff0,LastMouseX,LastMouseY,sGetTimeMS()-ToolTipTimer))
                        {
                            ToolTipX = LastMouseX;
                            ToolTipY = LastMouseY;
                            ToolTipLocked = 1;
                        }
                    }
                }

                pnt->End(ctx);
                ctx->EndTarget();
            }
        }
    }
}

void sGuiManager::OnKey(const sKeyData &kd)
{
    if(Focus && Focus->OnKey(kd))
        return;

    uint key = sCleanKeyCode(kd.Key);
    uint keydbl = key | (kd.Key & sKEYQ_Double);
    uint allkey = (key & (sKEYQ_Mask | sKEYQ_Ctrl | sKEYQ_Alt | sKEYQ_AltGr));
    sWindow *w = Focus;

    // tools

    if(w)
    {
        if(w->CurrentTool)
        {
            if(key==(w->CurrentTool->Key | sKEYQ_Break))
            {
                w->CurrentTool->Message.SendCode(0);
                w->CurrentTool = 0;
            }
        }

        for(auto ti : w->ToolInfos)
        {
            if(key==ti->Key)
            {
                w->CurrentTool = ti;
                w->CurrentTool->Message.SendCode(1);
                break;
            }
        }
    }

    // keys

    if(w && !w->CurrentTool)
    {
        while(w)
        {
            for(auto ki : w->KeyInfos)
            {
                switch(ki->Kind)
                {
                case sWKIK_AllKeys:
                    if(allkey>=0x0020 && allkey<0xe000)
                    {
                        ki->Message.Send(kd);
                        goto done;
                    }
                    break;
                case sWKIK_SpecialKeys:
                    if(allkey<0x0020 || allkey>=0xe000)
                    {
                        ki->Message.Send(kd);
                        goto done;
                    }
                    break;
                case sWKIK_Key:
                    if(key==ki->Key || keydbl==ki->Key)     // when adding keys, place double keys before all other..
                    {
                        ki->Message.Send(kd);
                        goto done;
                    }
                    break;
                }
            }

            if(w->IndirectParent)
                w = w->IndirectParent;
            else
                w = w->FocusParent;
        }
    }
done:;
}

void sGuiManager::QuickMessage(const char *msg)
{
    QuickMsg = msg;
    QuickMsgEndTime = sGetTimeMS() + 500;
}

void sGuiManager::ClearDragDrop(sWindow *w)
{
    if(DragDropIcon)
        if(w==0 || DragDropIcon->SourceWin==w)
            DragDropIcon = 0;
}

bool sGuiManager::GetDragDropInfo(sWindow *mywindow,sDragDropIcon &icon,int &mx,int &my)
{
    if(mywindow == HoverWindow && DragDropIcon)
    {
        icon = *DragDropIcon;
        mx = LastMouseX;
        my = LastMouseY;
        return true;
    }
    return false;
}

void sGuiManager::ResetTooltipTimer(sWindow *win)
{
    if(HoverFocus==win)
    {
        ToolTipTimer = sGetTimeMS();
        ToolTipLocked = false;
    }
}

void sGuiManager::GetMouse(int &x,int &y,int &b)
{
    x = LastMouseX;
    y = LastMouseY;
    b = LastMouseB;
}

void sGuiManager::OnDrag(const sDragData &dd)
{
    sGuiScreen *scr = 0;
    for(auto i : Screens)
    {
        if(i->IsRightScreen(dd.Screen))
        {
            scr = i;
            break;
        }
    }

    if(!scr)                // mouse is over a screen that is not part of the gui
        return;

    sWindow *hit = HitWindow(scr,dd.PosX,dd.PosY);
    HoverWindow = hit;

    LastMouseX = dd.PosX;
    LastMouseY = dd.PosY;
    LastMouseB = dd.Buttons;
    LastDeltaX = dd.PosX - dd.StartX;
    LastDeltaY = dd.PosY - dd.StartY;

    if(hit!=HoverFocus && dd.Buttons==0)
    {
        if(HoverFocus)
        {
            HoverFocus->Flags &= ~sWF_Hover;
            if(HoverFocus->Flags & sWF_ReceiveHover)
                HoverFocus->Update();
        }
        HoverFocus = hit;
        ToolTipTimer = sGetTimeMS();
        ToolTipLocked = 0;
        if(HoverFocus)
        {
            HoverFocus->Flags |= sWF_Hover;
            if(HoverFocus->Flags & sWF_ReceiveHover)
                HoverFocus->Update();
        }
    }

    if(HoverFocus)
    {
        HoverFocus->OnHover(dd);
    }

    if(dd.Mode==sDM_Start)
    {
        if(hit)
            SetFocus(hit);
    }

    if(dd.Mode==sDM_Start)
    {
        CurrentDrag = 0;
        sWindowDragInfo *alldi = 0;
        uint key = sGetKeyQualifier() & ~(sKEYQ_Repeat | sKEYQ_Caps);
        if(key & sKEYQ_Shift)                   key |= sKEYQ_Shift;
        if(key & sKEYQ_Ctrl )                   key |= sKEYQ_Ctrl;
        if(dd.Buttons&sMB_DoubleClick)          key |= sKEYQ_Double;
        if((dd.Buttons&sMB_Mask)==sMB_Left)     key |= sKEY_LMB;
        if((dd.Buttons&sMB_Mask)==sMB_Right)    key |= sKEY_RMB;
        if((dd.Buttons&sMB_Mask)==sMB_Middle)   key |= sKEY_MMB;
        if((dd.Buttons&sMB_Mask)==sMB_Extra1)   key |= sKEY_Extra1;
        if((dd.Buttons&sMB_Mask)==sMB_Extra2)   key |= sKEY_Extra2;
        if((dd.Buttons&sMB_Mask)==sMB_Pen)      key |= sKEY_Pen;
        if((dd.Buttons&sMB_Mask)==sMB_PenF)     key |= sKEY_PenF;
        if((dd.Buttons&sMB_Mask)==sMB_PenB)     key |= sKEY_PenB;
        if((dd.Buttons&sMB_Mask)==sMB_Rubber)   key |= sKEY_Rubber;
        uint keydbl = key;
        if(dd.Buttons&sMB_DoubleClick)   keydbl |= sKEYQ_Double;

    RepeatForPen:
        sWindow *w = Focus;
        while(w)
        {
            int check = -1;
            for(auto di : w->DragInfos)
            {
                if(di->Tool == w->CurrentTool)
                {
                    switch(di->Kind)
                    {
                    case sWDIK_DragAll:
                        if(alldi==0)
                            alldi = di;
                        break;
                    case sWDIK_Drag:
                        if(key==di->Key || keydbl==di->Key)
                        {
                            CurrentDrag = di;
                            goto done;
                        }
                        break;
                    case sWDIK_DragHit:
                        if(key==di->Key || key==di->Key)
                        {
                            if(check==-1) check = w->OnCheckHit(dd);
                            if(check==di->Hit)
                            {
                                CurrentDrag = di;
                                goto done;
                            }
                        }
                        break;
                    case sWDIK_DragMiss:
                        if(key==di->Key || key==di->Key)
                        {
                            if(check==-1) check = w->OnCheckHit(dd);
                            if(check!=di->Hit)
                            {
                                CurrentDrag = di;
                                goto done;
                            }
                        }
                        break;
                    }
                }
            }
            w = w->FocusParent;
        }
        if(alldi)
            CurrentDrag = alldi;

        if((key & sKEYQ_Mask)==sKEY_Pen)
        {
            key = (key & ~sKEYQ_Mask) | sKEY_LMB;
            goto RepeatForPen;
        }
done:;

        ClearDragDrop(0);
    }

    if(!(Focus && Focus->OnDrag(dd)))
    {
        if(CurrentDrag && dd.Mode!=sDM_Hover)
        {
            CurrentDrag->Message.Send(dd);
        }
    }

    if(dd.Mode==sDM_Stop)
        CurrentDrag = 0;

    // drag & drop

    bool dropremove = false;
    if(!(Focus && Focus->OnDrag(dd)))
    {
        if(dd.Mode==sDM_Drag && dd.Buttons==sMB_Left)
        {
            if(Focus && !DragDropIcon && sAbs(dd.DeltaX)+sAbs(dd.DeltaY)>4)
            {
                for(const auto &icon : Focus->DragDropIcons)
                {
                    if(icon.Client.Hit(dd.StartX,dd.StartY))
                    {
                        DragDropIcon = &icon;
                        break;
                    }
                }
            }
            if(DragDropIcon && HoverWindow)
                dropremove = !HoverWindow->OnDragDropTo(*DragDropIcon,LastMouseX,LastMouseY,true);
        }
        if(DragDropIcon && dd.Mode==sDM_Stop)
        {
            dropremove = true;
            if(HoverWindow)
                dropremove = !HoverWindow->OnDragDropTo(*DragDropIcon,LastMouseX,LastMouseY,false);
            if(DragDropIcon)
                DragDropIcon->SourceWin->OnDragDropFrom(*DragDropIcon,dropremove);
            ClearDragDrop(0);
        }
    }

    // Set mouse cursor

    if(DragDropIcon)
    {
        sSetHardwareCursor(dropremove ? sHCI_No : sHCI_Crosshair);
    }
    else if(hit && (dd.Mode==sDM_Hover || dd.Mode==sDM_Start))
    {
        sSetHardwareCursor(hit->MouseCursor);
    }

}

void sGuiManager::Update()
{
    for(auto gscr : Screens)
        Update(gscr);
}

void sGuiManager::Update(sGuiScreen *gscr)
{
    gscr->UpdateRect = gscr->Root->Outer;
}

void sGuiManager::UpdateClient(sWindow *w)
{
    if(w->Screen)
        w->Screen->UpdateRect.Add(w->Client);
}

void sGuiManager::Update(sWindow *w,const sRect &r)
{
    if(w->Screen)
        w->Screen->UpdateRect.Add(r);
}

void sGuiManager::Layout()
{
    LayoutFlag = 1;
}

void sGuiManager::AddPopup(sGuiScreen *scr,sWindow *w)
{
    int x,y,b;
    sGetMouse(x,y,b);

    w->Outer.x0 = x;
    w->Outer.y0 = y;
    w->Outer.x1 = x + 200;
    w->Outer.y1 = y + 200;

    scr->Root->AddChild(w);
    SetFocus(w);
    Layout();
    UpdateClient(w);
}

void sGuiManager::AddPulldown(sGuiScreen *scr,sWindow *w,bool up)
{
    int x,y,b;
    sGetMouse(x,y,b);
    w->SetScreen(scr);
    if(GetFocus())
    {
        if(up)
        {
            CalcSize(w);
            x = GetFocus()->Outer.x0;
            y = GetFocus()->Outer.y0-1-w->ReqSizeY;
        }
        else
        {
            x = GetFocus()->Outer.x0;
            y = GetFocus()->Outer.y1+1;
        }
    }

    w->Outer.x0 = x;
    w->Outer.y0 = y;
    w->Outer.x1 = x + 200;
    w->Outer.y1 = y + 200;
    scr->Root->AddChild(w);
    SetFocus(w);
    Layout();
    Update(scr);
}

void sGuiManager::AddDialog(sGuiScreen *scr,sWindow *w,const sRect &r)
{
    w->Outer = r;
    scr->Root->AddChild(w);
    SetFocus(w);
    Layout();
    Update(scr);
}

void sGuiManager::AddDialog(sGuiScreen *scr,sWindow *w)
{
    w->SetScreen(scr);
    CalcSize(w);
    w->Outer.x0 = scr->Root->Inner.CenterX() - w->DecoratedSizeX/2;
    w->Outer.y0 = scr->Root->Inner.CenterY() - w->DecoratedSizeY/2;
    w->Outer.x1 = w->Outer.x0 + w->DecoratedSizeX;
    w->Outer.y1 = w->Outer.y0 + w->DecoratedSizeY;
    scr->Root->AddChild(w);
    SetFocus(w);
    Layout();
    Update(scr);
}

void sGuiManager::AddDialog(sGuiScreen *scr,sWindow *w,int x,int y)
{
    w->SetScreen(scr);
    CalcSize(w);
    w->Outer.x0 = x;
    w->Outer.y0 = y;
    w->Outer.x1 = w->Outer.x0 + w->DecoratedSizeX;
    w->Outer.y1 = w->Outer.y0 + w->DecoratedSizeY;
    scr->Root->AddChild(w);
    SetFocus(w);
    Layout();
    Update(scr);
}

/****************************************************************************/

void sGuiManager::SetFocusRC(sWindow *w)
{
    if(w)
    {
        w->Temp = 0;
        SetFocusRC(w->FocusParent);
    }
}

void sGuiManager::SetFocusR0(sWindow *w)
{
    if(w)
    {
        w->Temp |= 1;
        int old = w->Flags;
        w->Flags &= ~sWF_Focus;
        w->Flags |= sWF_ChildFocus;
        if(old!=w->Flags)
            Update(w,w->Outer);
        SetFocusR0(w->FocusParent);
        SetFocusR0(w->IndirectParent);
    }
}

void sGuiManager::SetFocusR1(sWindow *w)
{
    if(w)
    {
        if(w->Temp==0)
        {
            w->Flags &= ~(sWF_Focus | sWF_ChildFocus);
            Update(w,w->Outer);
        }
        SetFocusR1(w->FocusParent);
        SetFocusR1(w->IndirectParent);
    }
}

void sGuiManager::SetFocus(sWindow *w)
{
    if(w!=Focus)
    {
        SetFocusRC(Focus);
        SetFocusRC(w);
        SetFocusR0(w);
        SetFocusR1(Focus);

        if(Focus)
        {
            if(Focus->CurrentTool)
            {
                Focus->CurrentTool->Message.SendCode(0);
                Focus->CurrentTool = 0;
            }
            Focus->OnLeaveFocus();
        }
        Focus = w;
        if(Focus)
        {
            Focus->Flags |= sWF_Focus;
            Focus->OnEnterFocus();
            Update(Focus,Focus->Outer);
        }
    }

    sEnableVirtualKeyboard(Focus && (Focus->Flags & sWF_Keyboard));
}

void sGuiManager::TagWindows()
{
    for(auto gscr : Screens)
        TagWindow(gscr->Root);
}

void sGuiManager::TagWindow(sWindow *w)
{
    w->Tag();
    for(auto c : w->Childs)
        TagWindow(c);
    for(auto c : w->Borders)
        TagWindow(c);
}

sWindow *sGuiManager::HitWindowR(sWindow *w,int x,int y)
{
    sWindow *hit;
    while(w->LayoutShortcut)
        w = w->LayoutShortcut;

    if(!(w->Flags & sWF_Disable) && w->Outer.Hit(x,y))
    {
        if((w->Flags & sWF_Border) || w->Inner.Hit(x,y))
            for(int i=w->Childs.GetCount()-1;i>=0;i--)
                if((hit = HitWindowR(w->Childs[i],x,y)))
                    return hit;
        for(int i=w->Borders.GetCount()-1;i>=0;i--)
            if((hit = HitWindowR(w->Borders[i],x,y)))
                return hit;
        if(w->Flags & sWF_Border)
        {
            if(!w->Inner.Hit(x,y) && w->Outer.Hit(x,y)) // border is inside 'Outer' and outside 'Inner'
                return w;
        }
        else
        {
            if(w->Inner.Hit(x,y))
                return w;
        }
    }
    return 0;
}

sWindow *sGuiManager::HitWindow(sGuiScreen *gscr,int x,int y)
{
    return HitWindowR(gscr->Root,x,y);
}

/****************************************************************************/

void sGuiManager::ListWindowsR1(sWindow *w)
{
    w->Temp = 0;
    for(auto c : w->Childs)
        ListWindowsR1(c);
    for(auto c : w->Borders)
        ListWindowsR1(c);
}

void sGuiManager::ListWindowsR2(sArray<sWindow *> &a,sWindow *w)
{
    if(w->Temp==0)
    {
        w->Temp = 1;
        for(auto c : w->Childs)
            ListWindowsR2(a,c);
        for(auto c : w->Borders)
            ListWindowsR2(a,c);
        a.Add(w);
    }
}

void sGuiManager::ListWindows(sArray<sWindow *> &a)
{
    for(auto gscr : Screens)
    {
        ListWindowsR1(gscr->Root);
        ListWindowsR2(a,gscr->Root);
    }
}

/****************************************************************************/
}
