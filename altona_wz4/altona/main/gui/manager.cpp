/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "gui/manager.hpp"
#include "gui/overlapped.hpp"
#include "base/windows.hpp"
#include "base/graphics.hpp"
#include "base/input2.hpp"

#define DEBUGOVERDRAW 0

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Gui App                                                            ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

void sInitGui(sGuiApp *newGuiApp)
{
  if (!newGuiApp)
    newGuiApp = new sGuiApp(); // the assignemnt isn't really neccessary
                               // because the contructor calls sSetApp(this)

  sGui = new sGui_;
  sGui->SetRoot(new sOverlappedFrame);
  sAddRoot(sGui);
}

/****************************************************************************/

sGuiApp::sGuiApp()
{
  sSetApp(this);
  sGui = 0;
}
sGuiApp::~sGuiApp()
{
  if(sGui)
    sRemRoot(sGui);
}

void sGuiApp::OnPrepareFrame()
{
  sGui->OnPrepareFrame();
}

void sGuiApp::OnPaint2D(const sRect &client,const sRect &update)
{
  sGui->OnPaint(client,update);
}

void sGuiApp::OnPaint3D()
{
  sGui->OnPaint3d();
}

void sGuiApp::OnInput(const sInput2Event &ie)
{
  sGui->OnInput(ie);
}

void sGuiApp::OnEvent(sInt id)
{
  sGui->OnEvent(id);
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Gui Manager Class                                                  ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

static sInt MouseHardX = 0;
static sInt MouseHardY = 0;
static sInt MouseX = 0;
static sInt MouseY = 0;
void sGetMouseHard(sInt &x, sInt &y)
{
  x = MouseHardX;
  y = MouseHardY;
}

sGui_::sGui_()
{
  Client.Init(0,0,0,0);
  LayoutFlag = 1;
  Focus = 0;
  Hover = 0;
  Root = 0;
  HandleShiftEscape = 1;
  TabletMode = sGetShellSwitch(L"-tablet");

  Shutdown = 0;
  PropFont = 0;
  FixedFont = 0;

  SetTheme(sGuiThemeDefault);

  sClear(DragData);
  DragData.MouseX = -1;
  DragData.MouseY = -1;
  DragData.StartX = -1;
  DragData.StartY = -1;
  HardStartX = HardMouseX;
  HardStartY = HardMouseY;

  Dragging = 0;
  QueueEvents = 0;
  DontQueueEvents = 0;

  BackBuffer = 0;
  BackBufferUsed = 0;

  sClear(AsyncEntry);
}

sGui_::~sGui_()
{
  Shutdown = 1;
  sRemRoot(Root);
  Root = 0;
  delete BackBuffer;
  delete PropFont;
  delete FixedFont;
}

void sGui_::Tag()
{
  sObject::Tag();
  Hover->Need();
  Focus->Need();
  Root->Need();
  AsyncLock.Lock();
  sWindow *w=AsyncEntry.Window;
  AsyncLock.Unlock();
  w->Need();
}


void sGui_::SetTheme(const sGuiTheme &theme)
{
  sSetColor2D(sGC_BACK    ,theme.BackColor);
  sSetColor2D(sGC_DOC     ,theme.DocColor);
  sSetColor2D(sGC_BUTTON  ,theme.ButtonColor);
  sSetColor2D(sGC_TEXT    ,theme.TextColor);
  sSetColor2D(sGC_DRAW    ,theme.DrawColor);
  sSetColor2D(sGC_SELECT  ,theme.SelectColor);
  sSetColor2D(sGC_HIGH    ,theme.HighColor);
  sSetColor2D(sGC_LOW     ,theme.LowColor);
  sSetColor2D(sGC_HIGH2   ,theme.HighColor2);
  sSetColor2D(sGC_LOW2    ,theme.LowColor2);

  sSetColor2D(sGC_RED     ,0xff4040);
  sSetColor2D(sGC_YELLOW  ,0xffff20);
  sSetColor2D(sGC_GREEN   ,0x40ff40);
  sSetColor2D(sGC_BLUE    ,0x4040ff);
  sSetColor2D(sGC_BLACK   ,0x000000);
  sSetColor2D(sGC_WHITE   ,0xffffff);
  sSetColor2D(sGC_DARKGRAY,0x404040);
  sSetColor2D(sGC_GRAY,    0x808080);
  sSetColor2D(sGC_LTGRAY  ,0xc0c0c0);
  sSetColor2D(sGC_PINK    ,0xff8080);

  if (PropFont)
    PropFont->Init(theme.PropFont,14,0);
  else
    PropFont = new sFont2D(theme.PropFont,14,0);

  if (FixedFont)
    FixedFont->Init(theme.FixedFont,14,0);
  else
    FixedFont = new sFont2D(theme.FixedFont,14,0);

  if (Root) Root->Update();
}

void sGui_::CheckToolTip()
{
  sRect tr;
  if(Hover && Hover->ToolTip)
  {
    tr.x0 = Hover->Client.x0;
    tr.y0 = Hover->Client.y0;
    const sChar *str = Hover->ToolTip;
    sInt len = Hover->ToolTipLength;
    if(len==-1) len = sGetStringLen(str);
    sInt lines = 0;
    sInt width = 0;
    sInt t0 = 0;
    for(sInt i=0;i<len;i++)
    {
      if(str[i]=='\n')
      {
        width = sMax(width,sGui->PropFont->GetWidth(str+t0,i-t0));
        lines++;
        t0 = i+1;
      }
    }
    if(t0<len)
    {
      width = sMax(width,sGui->PropFont->GetWidth(str+t0,len-t0));
      lines++;
    }

    tr.x1 = tr.x0+4+width;
    tr.y1 = tr.y0+4+lines*sGui->PropFont->GetHeight();
  }
  if(tr!=ToolTipRect)
  {
    if(!ToolTipRect.IsEmpty())
      Update(ToolTipRect);
    if(!tr.IsEmpty())
      Update(tr);
  }
  ToolTipRect = tr;
}

void sGui_::OnInput(const sInput2Event &ie)
{
  Event ev;
  sClear(ev);

  // translate keyboard events
  if(HandleShiftEscape && (ie.Key&sKEYQ_SHIFT) && ((ie.Key&(sKEYQ_BREAK|sKEYQ_MASK))==sKEY_ESCAPE)) 
    sExit();
  switch (ie.Key) 
  {
  case sKEY_MOUSEMOVE:
    ev.Mode = sIED_MOUSE;
    ev.Key = 0;
#if sCONFIG_OPTION_XSI
    MouseHardX = ie.Payload >> 16;
    MouseHardY = ie.Payload & 0xffff;
#endif
    MouseX = ie.Payload >> 16;
    MouseY = ie.Payload & 0xffff;
    break;
  case sKEY_MOUSEHARD:
    ev.Mode = sIED_MOUSE;
    ev.Key = 0;
#if !sCONFIG_OPTION_XSI
    MouseHardX += sS16(ie.Payload >> 16);
    MouseHardY += sS16(ie.Payload & 0xffff);
#endif
    break;
  default:
    if ((ie.Key & sKEYQ_MASK) >= sKEY_LMB && (ie.Key & sKEYQ_MASK) <= sKEY_MOUSETWIST && (ie.Key & sKEYQ_MASK) != sKEY_WHEELUP && (ie.Key & sKEYQ_MASK) != sKEY_WHEELDOWN)
      ev.Mode = sIED_MOUSE;
    else
      ev.Mode = sIED_KEYBOARD;
    ev.Key = ie.Key;
    sCollect();
    break;
  }

  ev.MouseX = MouseX;
  ev.MouseY = MouseY;
  ev.HardX = MouseHardX;
  ev.HardY = MouseHardY;

  if(ev.Mode)
  {
    if(QueueEvents)
      EventQueue.AddTail(ev);
    else
      DoInput(ev);
  }
}

void sGui_::DoInput(const sGui_::Event &ev)
{
  switch(ev.Mode)
  {
  case sIED_KEYBOARD:
    SendKey(ev.Key);
    break;
  case sIED_MOUSE:
    SendMouse(ev);
    break;
  case sIED_MOUSEHARD:
    SendMouse(ev);
  }
}

void sGui_::SendMouse(const sGui_::Event &ie)
{
  sWindowDrag dd;
  sWindow *w;
  sBool changed;
  sInt rawkey;

  sU32 key = ie.Key;
  sInt mx = ie.MouseX;
  sInt my = ie.MouseY;
  sInt hmx = ie.HardX;
  sInt hmy = ie.HardY;

  // update mouse


  changed = DragData.MouseX!=mx || DragData.MouseY!=my || HardMouseX!=hmx || HardMouseY!=hmy;
  rawkey = key&sKEYQ_MASK;
  if(rawkey>=sInt(sKEY_LMB) && rawkey<=sInt(sKEY_LASTMB))
    rawkey = rawkey-sKEY_LMB;
  else
    rawkey = -1;

  HardMouseX = hmx;
  HardMouseY = hmy;
  DragData.MouseX = mx;
  DragData.MouseY = my;
  DragData.DeltaX = mx-DragData.StartX;
  DragData.DeltaY = my-DragData.StartY;
  if(TabletMode)
  {
    DragData.HardDeltaX = DragData.DeltaX;
    DragData.HardDeltaY = DragData.DeltaY;
  }
  else
  {
    DragData.HardDeltaX = HardMouseX-HardStartX;
    DragData.HardDeltaY = HardMouseY-HardStartY;
  }

  // hover focus

  w = HitWindow(DragData.MouseX,DragData.MouseY);
  if(w!=Hover)
  {
    if(Hover)
    {
      dd = DragData;              // send one last outside-mousemove to terminate hover
      dd.Mode = sDD_HOVER;
      Hover->OnDrag(dd);

      Hover->Flags &= ~sWF_HOVER; // really terminat hover.
      Send(Hover,sCMD_LEAVEHOVER);
    }
    Hover = w;
    if(Hover)
    {
      Hover->Flags |= sWF_HOVER;
      Send(Hover,sCMD_ENTERHOVER);
    }
  }

  // hover ondrag

  if(changed && Hover)
  {
    dd = DragData;
    dd.Mode = sDD_HOVER;
    Hover->OnDrag(dd);
  }

  // tooltips (from hover)

  CheckToolTip();

  // non-hover ondrag handling

  if(!Dragging)
  {
    if(!(key & sKEYQ_BREAK) && rawkey>=0)
    {
      SetFocus(Hover);

      Dragging = 1;
      DragData.Mode = sDD_START;
      DragData.Buttons = 1<<rawkey;
      DragData.Flags = (key & sKEYQ_REPEAT) ? sDDF_DOUBLECLICK : 0;
      DragData.DeltaX = 0;
      DragData.DeltaY = 0;
      DragData.StartX = DragData.MouseX;
      DragData.StartY = DragData.MouseY;
      DragData.HardDeltaX = 0;
      DragData.HardDeltaY = 0;
      HardStartX = HardMouseX;
      HardStartY = HardMouseY;

      if(Focus)                   // send sDD_START
      {
        dd = DragData;
        Focus->OnDrag(dd);
      }
      DragData.Mode = sDD_DRAG;   // and immediatly afterwards sDD_DRAG
      if(Focus) 
      {
        dd = DragData;
        Focus->OnDrag(dd);
      }
    }
    else
    {
      DragData.Mode = sDD_HOVER;
    }
  }
  else
  {
    DragData.Mode = sDD_DRAG;
    if(changed && Focus)
    {
      dd = DragData;
      Focus->OnDrag(dd);
    }
    if((key&sKEYQ_BREAK) && rawkey>=0)      // and dragging if ANY mousebutton was released
    {
      Dragging = 0;
      DragData.Mode = sDD_STOP;
      if(Focus) 
      {
        dd = DragData;
        Focus->OnDrag(dd);
      }
      DragData.Buttons = 0;
    }
  }

  // set mousepointer AFTER dragging!

  if(w)
    sSetMousePointer(w->MousePointer);

  // we may need to handle some messages...

  ProcessPost();
}

static void RecPrepareFrame(sWindow *w)
{
  sWindow *c;
  if(w->Flags & sWF_BEFOREPAINT)
    w->OnBeforePaint();
  sFORALL(w->Childs,c)
    RecPrepareFrame(c);
  sFORALL(w->Borders,c)
    RecPrepareFrame(c);
}

void sGui_::OnPrepareFrame()
{
  Event *ie;
  sArray<Event> copy;     // this is awkward. sometimes the windows file requester simply crashes directly back into the windows message loop. i hate microsoft.
  copy = EventQueue;
  EventQueue.Clear();

  sFORALL(copy,ie)
    DoInput(*ie);
  QueueEvents = 0;
  DontQueueEvents = 1;
  if(Root)
    RecPrepareFrame(Root);
}

void sGui_::OnPaint(const sRect &client,const sRect &update)
{
  static sInt colortimer;
#if DEBUGOVERDRAW
  sSetColor2D(sGC_BACK    ,0xc0c0c0+colortimer-32);
  sSetColor2D(sGC_BUTTON  ,0xa0a0a0+colortimer-32);
  sSetColor2D(sGC_WHITE   ,0xe0e0e0+colortimer-32);
#endif
  colortimer = (colortimer+12)&63;

  if(Client!=client)
  {
    LayoutFlag = 1;
    Client = client;
    sGui->Update();
  }

  if(LayoutFlag)
  {
    RecCalcSize(Root);
    Root->Outer = Client;
    RecLayout(Root);
    LayoutFlag = 0;
  }

  sClipFlush();

#if DEBUGOVERDRAW
  sLogF(L"gui",L"[%08x]----------\n",sGetTime());
#endif

  sVERIFY(BackBufferUsed==0);
  RecPaint(Root,update);
  sVERIFY(BackBufferUsed==0);
  DontQueueEvents = 0;

  CheckToolTip();
  if(Hover && Hover->ToolTip && !ToolTipRect.IsEmpty())
  {
    sRect r = ToolTipRect;
    sRectFrame2D(r,sGC_DRAW);
    r.Extend(-1);
    sGui->PropFont->SetColor(sGC_TEXT,sGC_SELECT);
    sGui->PropFont->Print(sF2P_OPAQUE|sF2P_TOP|sF2P_LEFT|sF2P_MULTILINE,r,Hover->ToolTip,Hover->ToolTipLength,1,0,0,0);
  }
}

void sGui_::OnPaint3d()
{
  Region3D.Clear();
  if(Paint3dFlag)
  {
    Window3D.Clear();
    RecPaint3d(Root,Root->Client);
    Paint3dFlag = 0;

    sWindow *w;
    sFORALL(Window3D,w)
      w->OnPaint3D();
  }
  if(Region3D.Rects.GetCount())
    sSetRenderClipping(&Region3D.Rects[0],Region3D.Rects.GetCount());
  else
    sSetRenderClipping(0,0);
}


sBool sGui_::RecShortcut(sWindow *w,sU32 key)
{
  if(w->Parent)
  {
    if(RecShortcut(w->Parent,key))
      return 1;
  }
  if(w->OnShortcut(key))
    return 1;
  return 0;
}

void sGui_::SendKey(sU32 key)
{
  sWindow *w = Focus;

  if(!RecShortcut(w,key))
  {
    while(w)
    {
      if(w->OnKey(key))
        break;
      w = w->Parent;
    }
  }
  ProcessPost();
}

void sGui_::OnEvent(sInt event)
{
  switch(event)
  {
  case sAE_EXIT:
    Shutdown = 1;
    CommandToAll(sCMD_SHUTDOWN);
    SetRoot(0);
    Focus = 0;
    Hover = 0;
    AsyncEntry.Window = 0;
    break;
  case sAE_TIMER:
    sInput2Update(sGetTime());
    CommandToAll(sCMD_TIMER);   // send to all windows? inefficient. 
    break;
  default:
    if(!Shutdown)
    {
      AsyncLock.Lock();
      if(AsyncEntry.Window) 
      {
        Send(AsyncEntry.Window,AsyncEntry.Command);
        AsyncEntry.Window = 0;
      }
      AsyncLock.Unlock();
    }
    ProcessPost();
    break;
  }
}

/****************************************************************************/

void sGui_::AddWindow(sWindow *w)
{
  Root->AddChild(w);
}

void sGui_::AddBackWindow(sWindow *w)
{
  w->Flags |= sWF_ZORDER_BACK;
  AddWindow(w);
  SetFocus(w);
  ProcessPost();
}

void sGui_::AddFloatingWindow(sWindow *w,const sChar *title, sBool closeable)
{
  w->AddBorder(new sSizeBorder);
  if (closeable)
    w->AddBorder(new sTitleBorder(title,sMessage(w,&sWindow::Close)));
  else
    w->AddBorder(new sTitleBorder(title));
  CalcSize(w);
  PositionWindow(w,
    Root->Client.CenterX()-w->DecoratedSizeX/2,
    Root->Client.CenterY()-w->DecoratedSizeY/2);
  AddWindow(w);
  SetFocus(w);
}

void sGui_::AddWindow(sWindow *w,sInt x,sInt y)
{
  CalcSize(w);
  PositionWindow(w,x,y);
  AddWindow(w);
  SetFocus(w);
}

void sGui_::AddCenterWindow(sWindow *w)
{
  CalcSize(w);
  PositionWindow(w,
    sMax(25,Root->Client.CenterX()-w->DecoratedSizeX/2),
    sMax(50,Root->Client.CenterY()-w->DecoratedSizeY/2));
  AddWindow(w);
  SetFocus(w);
}

void sGui_::AddPopupWindow(sWindow *w)
{
  w->PopupParent = GetFocus();
  AddWindow(w,DragData.MouseX,DragData.MouseY);
}

void sGui_::AddPulldownWindow(sWindow *w)
{
  w->PopupParent = GetFocus();
  AddWindow(w,Focus->Client.x0,Focus->Client.y1);
}

void sGui_::AddPulldownWindow(sWindow *w,const sRect &client)
{
  w->PopupParent = GetFocus();
  AddWindow(w,client.x0,client.y1);
}

void sGui_::PositionWindow(sWindow *w,sInt x,sInt y)
{
  w->Outer.x0 = x;
  w->Outer.y0 = y;
  if(w->Outer.x0+w->DecoratedSizeX > Root->Client.x1)
    w->Outer.x0 = Root->Client.x1-w->DecoratedSizeX;
  if(w->Outer.x0<0)
    w->Outer.x0 = 0;
  if(w->Outer.y0+w->DecoratedSizeY > Root->Client.y1)
    w->Outer.y0 = Root->Client.y1-w->DecoratedSizeY;
  if(w->Outer.y0<0)
    w->Outer.y0 = 0;

  w->Outer.x1 = w->Outer.x0 + w->DecoratedSizeX;
  w->Outer.y1 = w->Outer.y0 + w->DecoratedSizeY;
}

void sGui_::SetRoot(sWindow *r)
{
  Root = r;
  SetFocus(Root);
}

/****************************************************************************/

static void MarkFocusR(sWindow *p,sInt flag)
{
  while(p)
  {
    p->Temp = flag;
    if(p->PopupParent)
      MarkFocusR(p->PopupParent,flag);
     
    p = p->Parent;
  }
}

static void ClearFocusR(sWindow *p)
{
  if(p)
  {
    ClearFocusR(p->Parent);
    ClearFocusR(p->PopupParent);

    p->Flags &= ~sWF_FOCUS;
    if(p->Temp)
    {
      p->Flags &= ~sWF_CHILDFOCUS;
      sGui->Send(p,sCMD_LEAVEFOCUS);
      if(p->Flags & sWF_AUTOKILL)
      {
        sMessage msg(p,&sWindow::Close);
        msg.Post();
      }
    }
  }
}

static void SetFocusR(sWindow *p)
{
  if(p)
  {
    SetFocusR(p->Parent);
    SetFocusR(p->PopupParent);

    p->Flags &= ~sWF_FOCUS;
    if(!(p->Flags & sWF_CHILDFOCUS))
      sGui->Send(p,sCMD_ENTERFOCUS);
    p->Flags |= sWF_CHILDFOCUS;
  }
}

void sGui_::SetFocus(sWindow *w)
{
  sWindow *oldfocus;

  if(w==0)
    w=Root;
  if(w==0)
    return;

  if(Focus!=w)
  {

    // send sDD_STOP if required

    if(Dragging)
    {
      Dragging = 0;
      DragData.Mode = sDD_STOP;
      if(Focus) 
      {
        sWindowDrag dd = DragData;
        Focus->OnDrag(dd);
      }
    }

    // set new focus

    oldfocus = Focus;
    Focus = w;

    // send the minimum amount of focus change messages

    MarkFocusR(oldfocus,1);
    MarkFocusR(Focus,0);

    ClearFocusR(oldfocus);
    SetFocusR(Focus);
    Focus->Flags|=sWF_FOCUS;

    // done

    Layout();     // for sort order in overlapped frames
  }
}

void sGui_::Layout()
{
  LayoutFlag = 1;
  sGui->Update();
  sCollect();
}

void sGui_::Layout(const sRect &r)
{
  LayoutFlag = 1;
  sGui->Update(r);
}

sWindow *sGui_::HitWindow(sInt x,sInt y)
{
  return RecHitWindow(Root,x,y,0);
}

sWindow *sGui_::RecHitWindow(sWindow *w,sInt x,sInt y,sBool border) const
{
  sWindow *c;
  sWindow *t;

  if(!w->Outer.Hit(x,y))
    return 0;

  // first check the childs

  sFORALLREVERSE(w->Childs,c)
  {
    t = RecHitWindow(c,x,y,0);
    if(t) return t;
  }

  // then check the inner area  (without border)

  if(w->Inner.Hit(x,y))
    return w;

  // now it is save to check the borders. 
  // the borders client may overlap with the windows client!

  sFORALLREVERSE(w->Borders,c)
  {
    t = RecHitWindow(c,x,y,1);
    if(t) return t;
  }

  return 0;
}


/****************************************************************************/

void sGui_::RecCalcSize(sWindow *w)
{
  sWindow *c;

  // first calculate all childs

  sFORALL(w->Childs,c)
    RecCalcSize(c);

  // now the window itself may query it's childs.

  w->OnCalcSize();

  // last we have to add size from border

  w->DecoratedSizeX = w->ReqSizeX;
  w->DecoratedSizeY = w->ReqSizeY;
  sFORALL(w->Borders,c)
  {
    RecCalcSize(c);
    w->DecoratedSizeX += c->ReqSizeX;
    w->DecoratedSizeY += c->ReqSizeY;
  }

  // now this information can be used for layouting childs
}

void sGui_::RecLayout(sWindow *w)
{
  sWindow *c;
  w->Flags &=~ sWF_NOTIFYVALID;
  w->Inner = w->Outer;
  sFORALL(w->Borders,c)
  {
    c->Client = c->Inner = c->Outer = w->Inner;
    RecLayout(c);
  }
  w->Client = w->Inner; // add scrolling here!

  if(w->Flags & sWF_SCROLLX)
  {
    w->ScrollX = sClamp(w->ScrollX , 0 , sMax(0,w->ReqSizeX-w->Inner.SizeX()));
    w->Client.x0 -= w->ScrollX;
    w->Client.x1 = w->Client.x0 + sMax(w->Inner.SizeX(),w->ReqSizeX);
  }
  if(w->Flags & sWF_SCROLLY)
  {
    w->ScrollY = sClamp(w->ScrollY , 0 , sMax(0,w->ReqSizeY-w->Inner.SizeY()));
    w->Client.y0 -= w->ScrollY;
    w->Client.y1 = w->Client.y0 + sMax(w->Inner.SizeY(),w->ReqSizeY);
  }

  w->OnLayout();
  sFORALL(w->Childs,c)
    RecLayout(c);
}

void sGui_::RecPaint(sWindow *w,const sRect &update)
{
  sWindow *c;

  // paint borders in any order

  sFORALL(w->Borders,c)
    RecPaint(c,update);

  // enable automatic clipping

  if(w->Flags & (sWF_OVERLAPPEDCHILDS | sWF_CLIENTCLIPPING))
  {
    sClipPush();
    sClipRect(w->Inner);
  }

  // first window is background, last is top

  sFORALLREVERSE(w->Childs,c)
  {
    RecPaint(c,update);
    if(w->Flags & sWF_OVERLAPPEDCHILDS)
      sClipExclude(c->Outer);
  }

  // paint the client area. 

  if(w->Client.SizeX()>0 && w->Client.SizeY()>0 && w->Client.IsInside(update))
  {
    if(w->Flags & sWF_CLIENTCLIPPING)
      sClipRect(w->Inner);
    if(w->Flags & sWF_3D)
    {
      if(w->Inner.SizeX() && w->Inner.SizeY())
      {
        Paint3dFlag = 1;
      }
    }
    else
    {
#if DEBUGOVERDRAW
      sLogF(L"gui",L"repaint %08x:%s\n",sDInt(w),w->GetClassName());
#endif
      w->OnPaint2D();
    }
  }

  // undo automatic clipping

  if(w->Flags & (sWF_OVERLAPPEDCHILDS | sWF_CLIENTCLIPPING))
    sClipPop();
}

void sGui_::RecPaint3d(sWindow *w,const sRect &pw)
{
  sWindow *c;
  sRect mw;

  mw.And(pw,w->Outer);
  sFORALL(w->Borders,c)
    RecPaint3d(c,mw);
  sFORALL(w->Childs,c)
    RecPaint3d(c,mw);
  if(w->Childs.GetCount()==0)
    Region3D.Sub(mw);
  if(w->Flags & sWF_3D)
  {
    if(w->Inner.SizeX() && w->Inner.SizeY())
    {
      Window3D.AddTail(w);
      Region3D.Add(w->Inner);
    }
  }
}


/****************************************************************************/

void sGui_::RecCommand(sWindow *w,sInt cmd)
{
  if(cmd>=sCMD_USER || cmd==sCMD_DUMMY)
  {
    while(w && !w->OnCommand(cmd))
      w = w->Parent;
  }
  else
  {
    w->OnCommand(cmd);
  }
}

void sGui_::CommandToAll(sInt cmd,sWindow *w)
{
  sWindow *c;
  if(w==0) w=Root;
  w->OnCommand(cmd);
  sFORALL(w->Childs,c)
    CommandToAll(cmd,c);
}

void sGui_::ProcessPost()
{
  PostQueueEntry *pq;
  sFORALL(PostQueue,pq)
    RecCommand(pq->Window,pq->Command);
  PostQueue.Clear();
  sMessage::Pump();
}

void sGui_::Post(sWindow *w,sInt cmd)
{
  PostQueueEntry *pq;
  pq = PostQueue.AddMany(1);
  pq->Window = w;
  pq->Command = cmd;
}

void sGui_::PostAsync(sWindow *w,sInt cmd)
{
  if(!Shutdown)
  {
    AsyncLock.Lock();
    AsyncEntry.Window = w;
    AsyncEntry.Command = cmd;
    AsyncLock.Unlock();
    sTriggerEvent();
  }
}

void sGui_::Send(sWindow *w,sInt cmd)
{
  RecCommand(w,cmd);
}

void sGui_::Update(const sRect &r)
{
  if(!r.IsEmpty())          // sometimes we get empty events. for instance, when a CMD_LEAVEFOCUS is executed on a window that is not longer displayed
  {
    sUpdateWindow(r);
    if(!DontQueueEvents)
      QueueEvents = 1;
  }
}

void sGui_::Update()
{
  sUpdateWindow();
  if(!DontQueueEvents)
    QueueEvents = 1;
}


void sGui_::RecNotifyMake(sWindow *p)
{
  sWindowNotify *n;
  sWindow *w;

  if(!(p->Flags & sWF_NOTIFYVALID))
  {
    p->NotifyBounds.Clear();
    sFORALL(p->Childs,w)
    {
      RecNotifyMake(w);
      p->NotifyBounds.Add(w->NotifyBounds);
    }
    sFORALL(p->Borders,w)
    {
      RecNotifyMake(w);
      p->NotifyBounds.Add(w->NotifyBounds);
    }
    sFORALL(p->NotifyList,n)
      p->NotifyBounds.Add(*n);
    p->Flags |= sWF_NOTIFYVALID;
  }
}

void sGui_::RecNotify(sWindow *p,const void *start,const void *end)
{
  sWindowNotify *n;
  sWindow *w;

  if(p->NotifyBounds.Hit(start,end))
  {
    sFORALL(p->NotifyList,n)
      if(n->Hit(start,end))
        p->OnNotify(n->Start,(const sU8*)n->End-(const sU8*)n->Start);

    sFORALL(p->Childs,w)
      RecNotify(w,start,end);
    sFORALL(p->Borders,w)
      RecNotify(w,start,end);
  }
}

void sGui_::Notify(const void *ptr,sDInt n)
{
  if(Root)
  {
    if(!(Root->Flags & sWF_NOTIFYVALID))
      RecNotifyMake(Root);
    RecNotify(Root,ptr,((sU8*)ptr)+n);
  }
}


/****************************************************************************/

void sGui_::RectHL(const sRect &r,sInt colh,sInt coll) const
{
  sRect2D(r.x0,r.y0,r.x1,r.y0+1,colh);
  sRect2D(r.x0,r.y1-1,r.x1,r.y1,coll);
  sRect2D(r.x0,r.y0+1,r.x0+1,r.y1-1,colh);
  sRect2D(r.x1-1,r.y0+1,r.x1,r.y1-1,coll);
}

void sGui_::RectHL(const sRect &r, sBool invert) const
{
  if (invert)
    RectHL(r,sGC_LOW2,sGC_HIGH2);
  else
    RectHL(r,sGC_HIGH2,sGC_LOW2);
}

void sGui_::PaintHandle(sInt x,sInt y,sBool select) const
{
  sRect r(x-3,y-3,x+4,y+4);

  RectHL(r,sGC_DRAW,sGC_DRAW);
  r.Extend(-1);
  sRect2D(r,select?sGC_SELECT:sGC_BACK);
}

sBool sGui_::HitHandle(sInt x,sInt y,sInt mx,sInt my) const
{
  sRect r(x-3,y-3,x+4,y+4);
  return r.Hit(mx,my);
}

void sGui_::PaintButtonBorder(sRect &r,sBool pressed) const
{
  if(!pressed)
  {
    sGui->RectHL(r,sGC_HIGH2,sGC_LOW2); 
    r.Extend(-1);
    sGui->RectHL(r,sGC_HIGH,sGC_LOW); 
    r.Extend(-1);
  }
  else
  {
    sGui->RectHL(r,sGC_LOW2,sGC_HIGH2); 
    r.Extend(-1);
    sGui->RectHL(r,sGC_LOW,sGC_HIGH); 
    r.Extend(-1);
  }
}

void sGui_::PaintButton(const sRect &rect,const sChar *text,sInt flags,sInt len,sU32 backcolor) const
{
  sRect r(rect);

  PaintButtonBorder(r,(flags & sGPB_DOWN));

  sInt bp = sGC_BUTTON;

  sInt fp = (flags&sGPB_GRAY)?sGC_LOW2:sGC_TEXT;
  if(backcolor)
  {
    sSetColor2D(0,backcolor);
    bp = 0;
  }
  else if (flags&sGPB_DOWN)
  {
    bp=sGC_LOW;
    fp=(flags&sGPB_GRAY)?sGC_LOW2:sGC_TEXT;
  }

  sGui->PropFont->SetColor(fp,bp);


  sGui->PropFont->Print(sF2P_OPAQUE,r,text,len);
}

void sGui_::BeginBackBuffer(const sRect &rect)
{
  sVERIFY(BackBufferUsed==0);
  BackBufferUsed = 1;
  BackBufferRect = rect;

  sVERIFY(Client.x0 == 0);
  sVERIFY(Client.y0 == 0);
  if(BackBuffer==0 || BackBuffer->GetSizeX()!=Client.x1 || BackBuffer->GetSizeY()!=Client.y1)
  {
    delete BackBuffer;
    BackBuffer = new sImage2D(Client.x1,Client.y1,0);
  }

  sRender2DBegin(BackBuffer);
}

void sGui_::EndBackBuffer()
{
  sVERIFY(BackBufferUsed==1);
  BackBufferUsed = 0;
  sRender2DEnd();
  BackBuffer->Paint(BackBufferRect,BackBufferRect.x0,BackBufferRect.y0);
}


/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Themes                                                             ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

const sGuiTheme sGuiThemeDefault =
{
  0xe4e4e4, // back
  0xffffff, // doc
  0xd4d4cc, // button
  0x000000, // text
  0x404040, // draw
  0xb0b0b8, // select
  0xf2f2f2, // high
  0xc0c0c0, // low
  0xfafafa, // high2
  0x808080, // low2
  L"Arial", // prop
  L"Courier New", // fixed
};

const sGuiTheme sGuiThemeDarker = 
{
  0xc0c0c0, // back
  0xd0d0d0, // doc
  0xb0b0b0, // button
  0x000000, // text
  0x000000, // draw
  0xff8080, // select
  0xe0e0e0, // high
  0x606060, // low
  0xffffff, // high2
  0x000000, // low2
  L"Arial", // prop
  L"Courier New", // fixed
};

template <class streamer> void sGuiTheme::Serialize_(streamer &s)
{
  sInt version=s.Header(sSerId::sGuiTheme,1);
  sVERIFY(version>0);

  s | BackColor | DocColor | ButtonColor | TextColor | DrawColor;
  s | SelectColor | HighColor | LowColor | HighColor2 | LowColor2;
  s | PropFont | FixedFont;

  s.Footer();
}

void sGuiTheme::Serialize(sReader &s) { Serialize_(s); }
void sGuiTheme::Serialize(sWriter &s) { Serialize_(s); }

void sGuiTheme::Tint(sU32 add,sU32 sub)
{
  sU32 addh = sScaleColorFast(add,0x80);
  sU32 subh = sScaleColorFast(sub,0x80);

  BackColor   = sAddColor(BackColor  ,add);
  ButtonColor = sAddColor(ButtonColor,add);
  SelectColor = sAddColor(SelectColor,add);
  HighColor   = sAddColor(HighColor  ,addh);
  LowColor    = sAddColor(LowColor   ,addh);
  HighColor2  = sAddColor(HighColor2 ,addh);
  LowColor2   = sAddColor(LowColor2  ,addh);

  BackColor   = sSubColor(BackColor  ,sub);
  ButtonColor = sSubColor(ButtonColor,sub);
  SelectColor = sSubColor(SelectColor,sub);
  HighColor   = sSubColor(HighColor  ,subh);
  LowColor    = sSubColor(LowColor   ,subh);
  HighColor2  = sSubColor(HighColor2 ,subh);
  LowColor2   = sSubColor(LowColor2  ,subh);
}

/****************************************************************************/
