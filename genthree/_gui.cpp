// This file is distributed under a BSD license. See LICENSE.txt for details.
#include "_gui.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   The Cursors                                                        ***/
/***                                                                      ***/
/****************************************************************************/

static sChar cursor[32*32+1] = 

"10000000000000000000000000000000"
"11000000000000000000000000000000"
"12100000000000000000000000000000"
"12210000000000000000000000000000"
"12221000000000000000000000000000"
"12222100000000000000000000000000"
"12222210000000000000000000000000"
"12222221000000000000000000000000"
"12222222100000000000000000000000"
"12222222210000000000000000000000"
"12222211111000000000000000000000"
"12212210000000000000000000000000"
"12101221000000000000000000000000"
"11001221000000000000000000000000"
"10000122100000000000000000000000"
"00000122100000000000000000000000"
"00000012210000000000000000000000"
"00000012210000000000000000000000"
"00000001100000000000000000000000"
"00000000000000000000000000000000"
"00000000000000000000000000000000"
"00000000000000000000000000000000"
"00000000000000000000000000000000"
"00000000000000000000000000000000"
"00000000000000000000000000000000"
"00000000000000000000000000000000"
"00000000000000000000000000000000"
"00000000000000000000000000000000"
"00000000000000000000000000000000"
"00000000000000000000000000000000"
"00000000000000000000000000000000"
"00000000000000000000000000000000";

static sU32 curcol[3] = { 0x00000000,0xff000000,0xffffffff };

/****************************************************************************/
/***                                                                      ***/
/***   The main class                                                     ***/
/***                                                                      ***/
/****************************************************************************/

sMAKEZONE(GuiPaint    ,"GuiPaint"   ,0xffffff00);
sMAKEZONE(GuiPaint3d  ,"GuiPaint3d" ,0xffffff20);
sMAKEZONE(GuiFrame    ,"GuiFrame"   ,0xffffffc0);
sMAKEZONE(GuiKey      ,"GuiKey"     ,0xffffff40);

sGuiManager::sGuiManager()
{
  sU32 *p,*b;
  sBitmap *bm;
  sInt i;

  sREGZONE(GuiPaint   );
  sREGZONE(GuiPaint3d );
  sREGZONE(GuiFrame   );
  sREGZONE(GuiKey     );

  for(i=0;i<sGUI_MAXROOTS;i++)
    Root[i] = 0;
  Focus = 0;
  NewFocus = 0;
  DragWindow = 0;
  MouseX = 0;
  MouseY = 0;
  MouseButtons = 0;
  MouseTrigger = 0;
  MouseLastTime = 0;
  KeyIndex = 0;
  PostIndex = 0;
  NewAppPosToggle = 0;
  Viewport.Init();
  Viewport.ClearFlags = sVCF_NONE;
  ViewportValid = sFALSE;
  StartGCFlag = sFALSE;
  LastGCCount = 0;
  OnWindow = 0;
  CurrentRoot = 0;


  b = p = FlatMatStates;
  sSystem->StateBase(p,0,sMBM_FLAT,0);
  sSystem->StateEnd(p,b,256*4);
  FlatMat = sPainter->LoadMaterial(b,0,1,1);

  PropFonts[0] = sPainter->LoadFont("Arial",12,0,0);
  FixedFonts[0] = sPainter->LoadFont("Courier New",12,0,0);
  PropFonts[1] = sPainter->LoadFont("Tahoma",9,0,sHFS_PIXEL);
  FixedFonts[1] = sPainter->LoadFont("Courier New",9,0,sHFS_PIXEL);

  b = p = AlphaMatStates;
  sSystem->StateBase(p,sMBF_BLENDALPHA,sMBM_FLAT,0);
  sSystem->StateEnd(p,b,256*4);
  AlphaMat = sPainter->LoadMaterial(b,0,1,1);

  b = p = AddMatStates;
  sSystem->StateBase(p,sMBF_BLENDADD,sMBM_FLAT,0);
  sSystem->StateEnd(p,b,256*4);
  AddMat = sPainter->LoadMaterial(b,0,1,1);

  bm = new sBitmap;
  bm->Init(32,32);
  for(i=0;i<32*32;i++)
    bm->Data[i] = curcol[cursor[i]&0x0f];
  CursorMat = sPainter->LoadBitmapAlpha(bm);

  Palette[sGC_NONE]    = 0xff205080;
  Palette[sGC_BACK]    = 0xff808080+0;
  Palette[sGC_TEXT]    = 0xff000000;
  Palette[sGC_DRAW]    = 0xff000000;
  Palette[sGC_HIGH]    = 0xffe0e0e0+0;
  Palette[sGC_LOW]     = 0xff606060+0;
  Palette[sGC_HIGH2]   = 0xffffffff;
  Palette[sGC_LOW2]    = 0xff000000;
  Palette[sGC_BUTTON]  = 0xffa0a0a0+0;
  Palette[sGC_BARBACK] = 0xffe0e0e0+0;
  Palette[sGC_SELECT]  = 0xffc0c0c0+0;
  Palette[sGC_SELBACK] = 0xffc08060;

  GuiStyle = 0;
  i = sGS_SCROLLBARS;
  if(sSystem->GetFullscreen())
    i |= sGS_MAXTITLE;
  SetStyle(i);

  Clipboard = new sList<sObject>;
  
  sSystem->SetResponse(1);
}

/****************************************************************************/

sGuiManager::~sGuiManager()
{
}

/****************************************************************************/

void sGuiManager::Tag()
{
  sInt i;

  for(i=0;i<sGUI_MAXROOTS;i++)
    sBroker->Need(Root[i]);
  sBroker->Need(Focus);
  sBroker->Need(NewFocus);
  sBroker->Need(DragWindow);
  sBroker->Need(Clipboard);
  sBroker->Need(OnWindow);

  for(i=0;i<PostIndex;i++)
    sBroker->Need(PostBufferWin[i]);
}

/****************************************************************************/
/****************************************************************************/

sBool sGuiManager::OnDebugPrint(sChar *t)
{
  sGuiWindow *win;

  win = OnWindow;
  while(win)
  {
    if(win->OnDebugPrint(t))
      return sTRUE;
    win = win->Parent;
  }

  return sFALSE;
}

/****************************************************************************/

void sGuiManager::OnPaint()
{
  sRect r;
  sInt i;

  sZONE(GuiPaint);
//  Palette[sGC_BACK].r ^= 0x80;

/*
  sSystem->BeginViewport(view);
  sGui->OnPaint3d();
  mat.Init();
  sSystem->SetMatrix(mat);
  sGui->OnPaint();
  sPainter->Flush();
  sSystem->EndViewport();
*/
  for(i=0;i<sGUI_MAXROOTS;i++)
  {
    if(Root[i])
    {
      Viewport.Init();
      Viewport.ClearFlags = sVCF_ALL;
      Viewport.ClearColor = Palette[0];
      Viewport.Screen = i;
      ViewportValid = sFALSE;

      r = Root[i]->Position;
      PaintR(Root[i],r,0,0);

      if(ViewportValid)
      {
        if(i==CurrentRoot && sSystem->GetFullscreen())
          sPainter->Paint(CursorMat,MouseX,MouseY,0xffffffff);
        sPainter->Flush();
        sSystem->EndViewport();
        ViewportValid = sFALSE;
      } 
    }
  }
}

/****************************************************************************/

void sGuiManager::PaintR(sGuiWindow *win,sRect clip,sInt sx,sInt sy)
{
  sGuiWindow *p;
  sViewport view;
  sRect r;

  if(win->Flags & sGWF_DISABLED)
    return;
  if(win->Flags & sGWF_FLUSH)
    sPainter->Flush();

  win->Flags &= ~sGWF_UPDATE;
  win->ClientSave.x1 = win->ClientSave.x0 + sMax(win->RealX,win->SizeX);
  win->ClientSave.y1 = win->ClientSave.y0 + sMax(win->RealY,win->SizeY);
  if(win->ScrollX + win->RealX > win->SizeX)
    win->ScrollX = win->SizeX-win->RealX;
  if(win->ScrollX<0)
    win->ScrollX = 0;
  if(win->ScrollY + win->RealY > win->SizeY)
    win->ScrollY = win->SizeY-win->RealY;
  if(win->ScrollY<0)
    win->ScrollY = 0;
  win->Client = win->ClientSave;
  win->Client.x0 -= sx+win->ScrollX;
  win->Client.x1 -= sx+win->ScrollX;
  win->Client.y0 -= sy+win->ScrollY;
  win->Client.y1 -= sy+win->ScrollY;

  r.x0 = win->ClientSave.x0-sx;
  r.y0 = win->ClientSave.y0-sy;
  r.x1 = win->ClientSave.x0-sx+win->RealX;
  r.y1 = win->ClientSave.y0-sy+win->RealY;
  r.And(clip);
  win->ClientClip.Init(0,0,0,0);
  if(r.x0<r.x1 && r.y0<r.y1)
  {
    win->ClientClip = r;
    CurrentClip = r;
    sPainter->SetClipping(CurrentClip);
    sPainter->EnableClipping(1);
    if(win->Flags & sGWF_PAINT3D)
    {
      if(ViewportValid)
      {
        sPainter->Flush();
        sSystem->EndViewport();
        ViewportValid = sFALSE;
      }
      view.Init();
      view.Screen = Viewport.Screen;
      view.Window = CurrentClip;
      OnWindow = win;
      {
        sZONE(GuiPaint3d);
        win->OnPaint3d(view);
      }
      OnWindow = 0;
    }
    else
    {
      if(!ViewportValid)
      {
        sSystem->BeginViewport(Viewport);
        Viewport.ClearFlags = sVCF_NONE;
        ViewportValid = sTRUE;
      }
      OnWindow = win;
      win->OnPaint();
      OnWindow = 0;
    }
    sPainter->EnableClipping(0);
  }

  p = win->Childs;
  while(p)
  {
    PaintR(p,r,sx+win->ScrollX,sy+win->ScrollY);
    if(win->Flags & sGWF_FLUSH)
      sPainter->Flush();
    p = p->Next;
  }

  p = win->Borders;
  while(p)
  {
    r = clip;
//    r.And(p->Position);
    PaintR(p,r,sx,sy);
    p = p->Next;
  }
}

/****************************************************************************/

void sGuiManager::OnPaint3d()
{
}

/****************************************************************************/

void sGuiManager::OnKey(sU32 key)
{
  sInt i;
  sScreenInfo si;

  sZONE(GuiKey);
  if(key==sKEY_MODECHANGE)
  {
    for(i=0;i<sGUI_MAXROOTS;i++)
    {
      if(Root[i])
      {
        sSystem->GetScreenInfo(i,si);
        Root[i]->Position.Init(0,0,si.XSize,si.YSize);
        Root[i]->Flags |= sGWF_LAYOUT;
      }
    }
  }
  else
  {
    if(KeyIndex>=256)
      OnFrame();
    KeyBuffer[KeyIndex++] = key;
  }
}

/****************************************************************************/

void sGuiManager::SendDrag(sDragData &dd)
{
  sGuiWindow *win;

  win = Focus;
  while(win->Parent && (win->Flags&sGWF_PASSRMB) && (dd.Buttons&2))
    win = win->Parent;
  while(win->Parent && (win->Flags&sGWF_PASSMMB) && (dd.Buttons&4))
    win = win->Parent;
  OnWindow = win;
  win->OnDrag(dd);
  OnWindow = 0;
}

void sGuiManager::OnFrame()
{
  sInputData id;
  sInt i,time;
  sInt oc,rc;
  sDragData dd;
  static sDragData ddo;
  sGuiWindow *win;
  sScreenInfo si;
  sInt count;
  static sInt OldMouseX,OldMouseY;
  sZONE(GuiFrame);

  for(i=0;i<PostIndex;i++)
    Send(PostBuffer[i],PostBufferWin[i]);
  PostIndex = 0;

  sSystem->GetScreenInfo(CurrentRoot,si);
  sSystem->GetInput(0,id);
  count = sSystem->GetScreenCount();
  if(sSystem->GetFullscreen())
  {
    MouseX += (id.Analog[0]-OldMouseX);
    MouseY += (id.Analog[1]-OldMouseY);

    while(CurrentRoot>0 && MouseX<0)
    {
      CurrentRoot--;
      MouseX += si.XSize;
      sSystem->GetScreenInfo(CurrentRoot,si);
    }
    while(CurrentRoot<count-1 && MouseX>=si.XSize)
    {
      CurrentRoot++;
      MouseX -= si.XSize;
      sSystem->GetScreenInfo(CurrentRoot,si);
    }

    MouseX = sRange(MouseX,si.XSize-1,0);
    MouseY = sRange(MouseY,si.YSize-1,0);
  }
  else
  {
    sSystem->GetWinMouse(MouseX,MouseY);
  }
  OldMouseX = id.Analog[0];
  OldMouseY = id.Analog[1];
  if(MouseButtons==0)
  {
    MouseStartX = MouseX;
    MouseStartY = MouseY;
    MouseAStartX = id.Analog[0];
    MouseAStartY = id.Analog[1];
  }
  MouseTrigger = id.Digital & ~MouseButtons;
  MouseButtons = id.Digital;

  if(!MouseButtons && DragWindow)
  {
    dd = ddo;
    dd.Mode = sDD_STOP;
    SendDrag(dd);
//    Focus->OnDrag(dd);
    DragWindow = 0;
  }
 
  dd.MouseX = MouseX;
  dd.MouseY = MouseY;
  dd.Buttons = MouseDragButtons;
  dd.DeltaX = id.Analog[0]-MouseAStartX;//MouseX-MouseStartX;
  dd.DeltaY = id.Analog[1]-MouseAStartY;//MouseY-MouseStartY;
  ddo = dd;

  if(MouseTrigger && !NewFocus)
  {
    NewFocus = FocusR(Root[CurrentRoot],MouseX,MouseY,~(sGWF_FOCUS|sGWF_CHILDFOCUS));
  }
  if(NewFocus)
  {
    if(Focus!=NewFocus)
    {
      if(DragWindow)
      {
        dd.Mode = sDD_STOP;
        SendDrag(dd);
        DragWindow = 0;
      }
      Focus->OnKey(sKEY_APPFOCLOST);
    }
    Focus = NewFocus;
    Focus->OnKey(sKEY_APPFOCUS);
    NewFocus = 0;
  }
  if(DragWindow)
  {
    dd.Mode = sDD_DRAG;
    SendDrag(dd);
//    Focus->OnDrag(dd);
  }
  if(MouseButtons && !DragWindow)
  {
    DragWindow = Focus;
    MouseDragButtons = MouseButtons;
    dd.Buttons = MouseDragButtons;
    dd.Mode = sDD_START;
    time = sSystem->GetTime();
    if(time-MouseLastTime<250)
      dd.Buttons |= sDDB_DOUBLE;
    MouseLastTime = time;
    SendDrag(dd);
//    Focus->OnDrag(dd);
  }

  for(i=0;i<KeyIndex;i++)
  {
    if(!KeyR(Focus,KeyBuffer[i]))
    {
      win = Focus;
      while(win->Parent && (win->Flags & sGWF_PASSKEY))
        win = win->Parent;
      OnWindow = win;
      win->OnKey(KeyBuffer[i]);
      OnWindow = 0;
    }
  }
  KeyIndex = 0;

  for(i=0;i<sGUI_MAXROOTS;i++)
    if(Root[i])
      FrameR(Root[i]);

  sBroker->Stats(oc,rc);
  if(oc > LastGCCount+100)
    StartGCFlag = 1;
  if(StartGCFlag)
  {
    sBroker->Free();
    StartGCFlag = sFALSE;
    sBroker->Stats(oc,rc);
    LastGCCount = oc;
  }
}

/****************************************************************************/

static void OnTickR(sGuiWindow *win,sInt ticks)
{
  sInt i,max;
  max = win->GetChildCount();
  for(i=0;i<max;i++)
    OnTickR(win->GetChild(i),ticks);
  win->OnTick(ticks);
}

void sGuiManager::OnTick(sInt ticks)
{
  sInt i;
  for(i=0;i<4;i++)
  {
    if(Root[i])
      OnTickR(Root[i],ticks);
  }
}

/****************************************************************************/

sBool sGuiManager::KeyR(sGuiWindow *win,sU32 key)
{
  sInt result;

  if(win->Parent)
    if(KeyR(win->Parent,key))
      return sTRUE;
  OnWindow = win;
  result = win->OnShortcut(key);
  OnWindow = 0;

  return result;
}

void sGuiManager::FrameR(sGuiWindow *win)
{
  sGuiWindow *p;

// clear flags

  win->Flags &= ~(sGWF_HOVER|sGWF_FOCUS|sGWF_CHILDFOCUS);
  if(win->Flags & sGWF_DISABLED)
    return;

// start layout recusion if necessary

  if(win->Flags & sGWF_LAYOUT)
  {
    CalcSizeR(win);
    LayoutR(win);
  }

// borders

  p = win->Borders;
  while(p)
  {
    FrameR(p);
    win->Flags |= p->Flags & sGWF_CHILDFOCUS;
    p = p->Next;
  }

// childs

  p = win->Childs;
  while(p)
  {
    FrameR(p);
    win->Flags |= p->Flags & sGWF_CHILDFOCUS;
    p = p->Next;
  }

// check new focus

  if(win->Client.Hit(MouseX,MouseY))
    win->Flags |= sGWF_HOVER;
  if(win == Focus)
    win->Flags |= sGWF_FOCUS|sGWF_CHILDFOCUS;

// call user function

  OnWindow = win;
  win->OnFrame();
  OnWindow = 0;
}

/****************************************************************************/

sGuiWindow *sGuiManager::FocusR(sGuiWindow *win,sInt mx,sInt my,sInt flagmask)
{
  sGuiWindow *foc,*p;
  sInt i,max;

  win->Flags &= flagmask;//~(sGWF_FOCUS|sGWF_CHILDFOCUS);
  if(win->Flags & sGWF_DISABLED)
    return 0;
  foc = 0;

  max = win->GetChildCount();
  for(i=max-1;i>=0 && !foc;i--)
  {
    p = win->GetChild(i);
    foc = FocusR(p,mx,my,flagmask);
  }

  p = win->Borders;
  while(p && !foc)
  {
    foc = FocusR(p,mx,my,flagmask);
    p = p->Next;
  }

  if(!foc)
  {
    if(win->ClientClip.Hit(mx,my))
    {
      foc = win;
      if(win->Flags & sGWF_BORDER)
      {
        if(win->Next)
        {
          if(win->Next->ClientClip.Hit(mx,my))
            foc = 0;
        }
        else
        {
          if(win->Parent->ClientClip.Hit(mx,my))
            foc = 0;
        }
      }
    }
  }

  return foc;
}

/****************************************************************************/

void sGuiManager::CalcSizeR(sGuiWindow *win)
{
  sGuiWindow *p;

  if(win->Flags & sGWF_DISABLED)
    return;

  p = win->Childs;
  while(p)
  {
    CalcSizeR(p);
    p = p->Next;
  }

  win->Flags &= ~(sGWF_SCROLLX|sGWF_SCROLLY);
  win->SizeX = 0;
  win->SizeY = 0;
  OnWindow = win;
  win->OnCalcSize();
  OnWindow = 0;

  p = win->Borders;
  while(p)
  {
    CalcSizeR(p);
    win->SizeX += p->SizeX;
    win->SizeY += p->SizeY;
    p = p->Next;
  }

  if(win->Flags & sGWF_SETSIZE)
  {
    win->Position.x1 = win->Position.x0 + win->SizeX;
    win->Position.y1 = win->Position.y0 + win->SizeY;
  }
}

/****************************************************************************/

void sGuiManager::LayoutR(sGuiWindow *win)
{
  sGuiWindow *p;

  if(win->Flags & sGWF_DISABLED)
    return;

  win->Flags &= ~sGWF_LAYOUT;
  win->Client = win->Position;

  p = win->Borders;
  while(p)
  {
    p->Position = p->Client = win->Client;
    p->OnSubBorder();
    LayoutR(p);
    p = p->Next;
  }

  if(win->Flags & sGWF_SQUEEZE)
  {
    win->SizeX = win->Client.XSize();
    win->SizeY = win->Client.YSize();
  }

  win->RealX = win->Client.XSize();
  if(win->Flags & sGWF_SCROLLX)
  {
    if(win->RealX < win->SizeX)
      win->Client.x1 =win->Client.x0 + win->SizeX;
  }
  win->RealY = win->Client.YSize();
  if(win->Flags & sGWF_SCROLLY)
  {
    if(win->RealY < win->SizeY)
      win->Client.y1 =win->Client.y0 + win->SizeY;
  }
  win->ClientSave = win->Client;

  OnWindow = win;
  win->OnLayout();
  OnWindow = 0;

  p = win->Childs;
  while(p)
  {
    LayoutR(p);
    p = p->Next;
  }
}

/****************************************************************************/
/****************************************************************************/

void sGuiManager::SetRoot(sGuiWindow *root,sInt screen)
{
  sScreenInfo si;

  sVERIFY(screen>=0 && screen<sGUI_MAXROOTS)

  sSystem->GetScreenInfo(0,si);
  Root[screen] = root;
  Root[screen]->Position.Init(0,0,si.XSize,si.YSize);
  Root[screen]->Client = root->Position;
  Root[screen]->Flags |= sGWF_LAYOUT;
  CurrentRoot = screen;

  Focus = root;
}

sGuiWindow *sGuiManager::GetRoot(sInt screen)
{
  sVERIFY(screen>=0 && screen<sGUI_MAXROOTS)
    
  return Root[screen];
}

sGuiWindow *sGuiManager::GetFocus()
{
  return Focus;
}

void sGuiManager::SetFocus(sGuiWindow *win)
{
  NewFocus = win;
}

void sGuiManager::SetFocus(sInt mx,sInt my)
{
  sGuiWindow *win;

  win = FocusR(Root[CurrentRoot],mx,my,~0);
  if(win)
    NewFocus = win;
}

/****************************************************************************/

sBool sGuiManager::Send(sU32 cmd,sGuiWindow *win)
{
  if(cmd==0) return sTRUE;
  while(win)
  {
    OnWindow = win;
    if(win->OnCommand(cmd))
    {
      OnWindow = 0;
      return sTRUE;
    }
    win = win->Parent;
  }
  OnWindow = 0;
  return sAppHandler(sAPPCODE_CMD,cmd);
}

void sGuiManager::Post(sU32 cmd,sGuiWindow *win)
{
  if(cmd)
  {
    sVERIFY(PostIndex<256);
    PostBuffer[PostIndex] = cmd;
    PostBufferWin[PostIndex] = win;
    PostIndex++;
  }
}

void sGuiManager::RemWindow(sGuiWindow *win)
{
  if(win==NewFocus)
    NewFocus = 0;
  if(win==Focus)
    Focus = Root[CurrentRoot];
}

void sGuiManager::Clip(sRect &r)
{
  r.And(CurrentClip);
  sPainter->SetClipping(r);
  sPainter->EnableClipping(sTRUE);
}
  
void sGuiManager::ClearClip()
{
  sPainter->SetClipping(CurrentClip);
  sPainter->EnableClipping(sTRUE);
}

sU32 sGuiManager::GetStyle()
{
  return GuiStyle;
}

void sGuiManager::SetStyle(sU32 mask)
{
  sInt i;

  GuiStyle = mask;
  for(i=0;i<sGUI_MAXROOTS;i++)
    if(Root[i])
      Root[i]->Flags |= sGWF_LAYOUT;

  i = (GuiStyle & sGS_SMALLFONTS) ? 1 : 0;
  PropFont = PropFonts[i];
  FixedFont = FixedFonts[i];
}

/****************************************************************************/

void sGuiManager::AddPulldown(sGuiWindow *win)
{
  AddWindow(win,Focus->Client.x0,Focus->Client.y1);
}

void sGuiManager::AddPopup(sGuiWindow *win)
{
  AddWindow(win,MouseX-4,MouseY-4);
}

void sGuiManager::AddApp(sGuiWindow *win,sInt screen)
{
  AddWindow(win,20+NewAppPosToggle*20,20+NewAppPosToggle*20,screen);
  NewAppPosToggle = (NewAppPosToggle+1)%10;
  NewFocus = win;
}

void sGuiManager::AddWindow(sGuiWindow *win,sInt x,sInt y,sInt screen)
{
  sInt dx,dy;

  if(screen == -1)
    screen=CurrentRoot;
  win->Flags |= sGWF_LAYOUT|sGWF_UPDATE;
  Root[screen]->AddChild(win);

  dx = x - win->Position.x0;
  dy = y - win->Position.y0;

  CalcSizeR(win);
  if(win->Position.XSize()==0)  win->Position.x1 = win->Position.x0 + win->SizeX;
  if(win->Position.YSize()==0)  win->Position.y1 = win->Position.y0 + win->SizeY;
  if(win->Position.XSize()>Root[CurrentRoot]->Client.XSize())
  {
    win->Position.x0 = Root[CurrentRoot]->Client.x0;
    win->Position.x1 = Root[CurrentRoot]->Client.x1;
  }
  if(win->Position.YSize()>Root[CurrentRoot]->Client.YSize())
  {
    win->Position.y0 = Root[CurrentRoot]->Client.y0;
    win->Position.y1 = Root[CurrentRoot]->Client.y1;
  }

  if(win->Position.x0+dx<Root[CurrentRoot]->Client.x0) dx-=win->Position.x0+dx-Root[CurrentRoot]->Client.x0;
  if(win->Position.y0+dy<Root[CurrentRoot]->Client.y0) dy-=win->Position.y0+dy-Root[CurrentRoot]->Client.y0;
  if(win->Position.x1+dx>Root[CurrentRoot]->Client.x1) dx-=win->Position.x1+dx-Root[CurrentRoot]->Client.x1;
  if(win->Position.y1+dy>Root[CurrentRoot]->Client.y1) dy-=win->Position.y1+dy-Root[CurrentRoot]->Client.y1;

  win->Position.x0 += dx;
  win->Position.y0 += dy;
  win->Position.x1 += dx;
  win->Position.y1 += dy;

  SetFocus(win);
}

/****************************************************************************/

void sGuiManager::ClipboardClear()
{
  Clipboard->Clear();
}

void sGuiManager::ClipboardAdd(sObject *obj)
{
  Clipboard->Add(obj);
}

void sGuiManager::ClipboardAddText(sChar *text,sInt len)
{
  sText *to;

  to = new sText;
  to->Init(text,len);
  Clipboard->Add(to);
}

sObject *sGuiManager::ClipboardFind(sU32 cid)
{
  sObject *co;
  sInt i,max;

  max = Clipboard->GetCount();
  for(i=0;i<max;i++)
  {
    co = Clipboard->Get(i);
    if(co->GetClass()==cid)
      return co;
  }

  return 0;
}

sChar *sGuiManager::ClipboardFindText()
{
  sText *to;

  to = (sText *) ClipboardFind(sCID_TEXT);
  if(to)
    return to->Text;
  else
    return 0;
}

/****************************************************************************/
/****************************************************************************/

void sGuiManager::Bevel(sRect &r,sBool down)
{
  sU32 colh0,colh1;
  sU32 coll0,coll1; 
    
  if(!down)
  {
    colh0 = Palette[sGC_HIGH];
    coll0 = Palette[sGC_LOW];
    colh1 = Palette[sGC_HIGH2];
    coll1 = Palette[sGC_LOW2];
  }
  else
  {
    coll0 = Palette[sGC_HIGH];
    colh0 = Palette[sGC_LOW];
    coll1 = Palette[sGC_HIGH2];
    colh1 = Palette[sGC_LOW2];
  }
  RectHL(r,1,colh1,coll1);
  if(!(GuiStyle&sGS_SMALLFONTS))
    RectHL(r,1,colh0,coll0);
}

/****************************************************************************/

void sGuiManager::RectHL(sRect &r,sInt w,sU32 colh,sU32 coll)
{
  sInt xs,ys;
    
  xs = r.x1-r.x0;
  ys = r.y1-r.y0;
  sPainter->Paint(FlatMat,r.x0  ,r.y0  ,xs,w     ,colh);
  sPainter->Paint(FlatMat,r.x0  ,r.y1-w,xs,w     ,coll);
  sPainter->Paint(FlatMat,r.x0  ,r.y0+w,w ,ys-2*w,colh);
  sPainter->Paint(FlatMat,r.x1-w,r.y0+w,w ,ys-2*w,coll);
  r.x0+=w; r.y0+=w; r.x1-=w; r.y1-=w; xs-=2*w; ys-=2*w;
}

/****************************************************************************/

void sGuiManager::Button(sRect r,sBool down,sChar *text,sInt align,sU32 col)
{
  if(col==0)
    col = sGui->Palette[sGC_BUTTON];
  Bevel(r,down);
  sPainter->Paint(sGui->FlatMat,r,col);
  if(text)
    sPainter->PrintC(sGui->PropFont,r,align,text,sGui->Palette[sGC_TEXT]);
}

/****************************************************************************/
/****************************************************************************/

/*
sClipboard::sClipboard()
{
  TextAlloc = 1024;
  Text = new sChar[TextAlloc];
}

sClipboard::~sClipboard()
{
  delete[] Text;
}

void sClipboard::Tag()
{
}

void sClipboard::Clear()
{
  Text[0] = 0;
}

void sClipboard::SetText(sChar *t,sInt len)
{
  sInt size;

  if(len==-1)
    len = sGetStringLen(t);
  size = len+1;
  if(size>TextAlloc)
  {
    size = sMax(size*2/3,TextAlloc*2);
    delete[] Text;
    Text = new sChar[size];
    TextAlloc = size;
  }
  sCopyMem(Text,t,len);
  Text[len] = 0;
}

sChar *sClipboard::GetText()
{
  return Text;
}
*/
/****************************************************************************/
/***                                                                      ***/
/***   window class                                                       ***/
/***                                                                      ***/
/****************************************************************************/

sGuiWindow::sGuiWindow()
{
  Parent = 0;
  Childs = 0;
  Borders = 0;
  Next = 0;
  Modal = 0;
  Flags = sGWF_LAYOUT|sGWF_UPDATE;
  Client.Init(0,0,0,0);
  Position.Init(0,0,0,0);
  ClientSave.Init(0,0,0,0);
  ClientClip.Init(0,0,0,0);
  SizeX = 0;
  SizeY = 0;
  RealX = 0;
  RealY = 0;
  ScrollX = 0;
  ScrollY = 0;
}

/****************************************************************************/

sGuiWindow::~sGuiWindow()
{
}

/****************************************************************************/

void sGuiWindow::Tag()
{
  sBroker->Need(Childs);
  sBroker->Need(Borders);
  sBroker->Need(Next);
  sBroker->Need(Modal);
  sBroker->Need(Parent);
}

/****************************************************************************/

void sGuiWindow::KillMe()
{
  Parent->Flags |= sGWF_LAYOUT;
  Parent->RemChild(this);
}

/****************************************************************************/
/****************************************************************************/

void sGuiWindow::AddBorder(sGuiWindow *win)
{
  sGuiWindow **p;

  p = &Borders;
  while(*p)
    p = &((*p)->Next);
  *p = win;
  win->Next = 0;
  win->Parent = this;
  win->Flags |= sGWF_BORDER|sGWF_PASSRMB|sGWF_PASSKEY;
}

/****************************************************************************/

void sGuiWindow::AddBorderHead(sGuiWindow *win)
{
  win->Next = Borders;
  Borders = win;
  win->Parent = this;
  win->Flags |= sGWF_BORDER|sGWF_PASSRMB|sGWF_PASSKEY;
}

/****************************************************************************/

void sGuiWindow::AddChild(sGuiWindow *win)
{
  sGuiWindow **p;

  p = &Childs;

  while(*p)
    p = &((*p)->Next);
  *p = win;
  win->Next = 0;
  win->Parent = this;
  sGui->GarbageCollection();
}

/****************************************************************************/

void sGuiWindow::AddChild(sInt pos,sGuiWindow *win)
{
  sGuiWindow *child;
  if(pos==0)
  {
    win->Next = Childs;
    win->Parent = this;
    Childs = win;
  }
  else
  {
    child = GetChild(pos-1);
    win->Next = child->Next;
    win->Parent = this;
    child->Next = win;
  }
}

/****************************************************************************/

void sGuiWindow::RemChild(sGuiWindow *win)
{
  sGuiWindow **p;

  p = &Childs;
  while(*p)
  {
    if(*p == win)
    {
      *p = win->Next;
      win->Parent = 0;
      win->Next = 0;
      win->Modal = 0;
      sGui->RemWindow(win);
      sGui->GarbageCollection();
      return;
    }
    p = &((*p)->Next);
  }
  sVERIFYFALSE;
}

/****************************************************************************/

void sGuiWindow::RemChilds()
{
  while(Childs)
    RemChild(Childs);
}

/****************************************************************************/

sInt sGuiWindow::GetChildCount()
{
  sGuiWindow *p;
  sInt count;

  p = Childs;
  count = 0;
  while(p)
  {
    count++;
    p = p->Next;
  }

  return count;
}

/****************************************************************************/

sGuiWindow *sGuiWindow::GetChild(sInt i)
{
  sGuiWindow *p;
  
  p = Childs;
  while(p && i)
  {
    i--;
    p = p->Next;
  }

  return p;
}


/****************************************************************************/

sGuiWindow *sGuiWindow::SetChild(sInt i,sGuiWindow *win)
{
  sGuiWindow *p,**pp;
  
  sVERIFY(i<GetChildCount());
  sVERIFY(win->Parent==0);

  pp = &Childs;
  p = *pp;
  while(i)
  {
    i--;
    pp = &p->Next;
    p = *pp;
  }

  sVERIFY(pp);
  sVERIFY(p);

  *pp = win;
  win->Next = p->Next;
  win->Parent = p->Parent;
  p->Next = 0;
  p->Parent = 0;

  return p;
}


sGuiWindow *sGuiWindow::SetChild(sGuiWindow *old,sGuiWindow *win)
{
  sGuiWindow *p,**pp;

  sVERIFY(old->Parent==this);
  sVERIFY(win->Parent==0);

  pp = &Childs;
  p = *pp;
  while(p!=old)
  {
    pp = &p->Next;
    p = *pp;
    sVERIFY(p);
  }

  sVERIFY(pp);
  sVERIFY(p);

  *pp = win;
  win->Next = p->Next;
  win->Parent = p->Parent;
  p->Next = 0;
  p->Parent = 0;

  return p;
}


/****************************************************************************/

sBool sGuiWindow::Send(sU32 cmd)
{
  return sGui->Send(cmd,this->Parent);
}

/****************************************************************************/

void sGuiWindow::Post(sU32 cmd)
{
  if(cmd)
    sGui->Post(cmd,this->Parent);
}

/****************************************************************************/
/****************************************************************************/

void sGuiWindow::ScrollTo(sRect vis,sInt mode)
{
  sInt c,s;
  vis.x0 -= Client.x0;
  vis.y0 -= Client.y0;
  vis.x1 -= Client.x0;
  vis.y1 -= Client.y0;
  if(mode==sGWS_SAFE)
  {
    vis.x0-=45;
    vis.y0-=45;
    vis.x1+=sPainter->GetHeight(sGui->FixedFont)*4;
    vis.y1+=sPainter->GetHeight(sGui->FixedFont)*4;
  }
  if(vis.x0<0)
    vis.x0 = 0;
  if(vis.x1>SizeX)
    vis.x1 = SizeX;
  if(vis.y0<0)
    vis.y0 = 0;
  if(vis.y1>SizeY)
    vis.y1 = SizeY;
  if(vis.x1-vis.x0 > RealX)
  {
    s = Client.XSize();
    c = (vis.x1+vis.x0)/2;
    vis.x0 = c-s/2;
    vis.x1 = vis.x0+s;
  }
  if(vis.y1-vis.y0 > RealY)
  {
    s = Client.YSize();
    c = (vis.y1+vis.y0)/2;
    vis.y0 = c-s/2;
    vis.y1 = vis.y0+s;
  }
  if(vis.x0 < ScrollX)
    ScrollX = vis.x0;
  if(vis.x1 > ScrollX+RealX)
    ScrollX = vis.x1-RealX;
  if(vis.y0 < ScrollY)
    ScrollY = vis.y0;
  if(vis.y1 > ScrollY+RealY)
    ScrollY = vis.y1-RealY;
}

void sGuiWindow::AddTitle(sChar *title,sU32 flags)
{
  sSizeBorder *sb;

  sb = new sSizeBorder;
  sb->Title = title;
  sb->DontResize = flags;
  AddBorderHead(sb);
}

/****************************************************************************/

void sGuiWindow::AddScrolling(sInt x,sInt y)
{
  sScrollBorder *sb;

  sb = new sScrollBorder;
  sb->EnableX = x;
  sb->EnableY = y;
  AddBorder(sb);
}

/****************************************************************************/

sBool sGuiWindow::MMBScrolling(sDragData &dd,sInt &sx,sInt &sy)
{
  if(dd.Buttons==4)
  {
    switch(dd.Mode)
    {
    case sDD_START:
      sx = ScrollX;
      sy = ScrollY;
      break;
    case sDD_DRAG:
      ScrollX = sRange<sInt>(sx-dd.DeltaX,SizeX-RealX,0);
      ScrollY = sRange<sInt>(sy-dd.DeltaY,SizeY-RealY,0);
      break;
    case sDD_STOP:
      break;
    }
    return sTRUE;
  }
  else
  {
    return sFALSE;
  }
}

/****************************************************************************/

sGuiWindow *sGuiWindow::FindBorder(sU32 cid)
{
  sGuiWindow *win;

  win = Borders;
  while(win)
  {
    if(win->GetClass()==cid)
      return win;
    win = win->Next;
  }
  return 0;
}

sSizeBorder *sGuiWindow::FindTitle()
{
  return (sSizeBorder *)FindBorder(sCID_SIZEBORDER);
}

/****************************************************************************/


/****************************************************************************/
/****************************************************************************/

void sGuiWindow::OnCalcSize()
{
  SizeX = 0;
  SizeY = 0;
}

/****************************************************************************/

void sGuiWindow::OnSubBorder()
{
}

/****************************************************************************/

void sGuiWindow::OnLayout()
{
  sVERIFY(Childs==0)
}

/****************************************************************************/

void sGuiWindow::OnPaint3d(sViewport &)
{
}

/****************************************************************************/

void sGuiWindow::OnPaint()
{
  sPainter->Paint(sGui->FlatMat,Client,0xffff0000);
}

/****************************************************************************/

void sGuiWindow::OnKey(sU32 key)
{
}

/****************************************************************************/

sBool sGuiWindow::OnShortcut(sU32 key)
{
  return sFALSE;
}

/****************************************************************************/

void sGuiWindow::OnDrag(sDragData &)
{
}

/****************************************************************************/

void sGuiWindow::OnFrame()
{
}

/****************************************************************************/

void sGuiWindow::OnTick(sInt ticks)
{
}

/****************************************************************************/

sBool sGuiWindow::OnCommand(sU32 cmd)
{
  return sFALSE;
}

/****************************************************************************/

sBool sGuiWindow::OnDebugPrint(sChar *t)
{
  return sFALSE;
}

/****************************************************************************/
/***                                                                      ***/
/***   frames                                                             ***/
/***                                                                      ***/
/****************************************************************************/

  sU32 LastKey;
  sU32 LastX,LastY;

sTestWindow::sTestWindow()
{
  LastKey = 0;
  LastX = 0;
  LastY = 0;
}

/****************************************************************************/

void sTestWindow::OnCalcSize()
{
  SizeX = 400;
  SizeY = 150;
}

/****************************************************************************/

void sTestWindow::OnSubBorder()
{
}

/****************************************************************************/

void sTestWindow::OnLayout()
{
}

/****************************************************************************/

void sTestWindow::OnPaint()
{
  sChar buffer[256];
  sInt x,y,h;
  x = Client.x0+2;
  y = Client.y0+2;
  h = sPainter->GetHeight(sGui->FixedFont);

  sPainter->Paint(sGui->FlatMat,Client,sGui->Palette[sGC_BACK]);

  sSPrintF(buffer,sizeof(buffer),"Flags: %08x",Flags);
  sPainter->Print(sGui->FixedFont,x,y,buffer); y+=h;
  sSPrintF(buffer,sizeof(buffer),"Key:   %08x",LastKey);
  sPainter->Print(sGui->FixedFont,x,y,buffer); y+=h;
  sSPrintF(buffer,sizeof(buffer),"Mouse: %d %d",LastX,LastY);
  sPainter->Print(sGui->FixedFont,x,y,buffer); y+=h;
}

/****************************************************************************/

void sTestWindow::OnKey(sU32 key)
{
  LastKey = key;
  if(key==sKEY_ESCAPE)
  {
    Parent->RemChild(this);
  }
}

/****************************************************************************/

void sTestWindow::OnDrag(sDragData &dd)
{
  LastX = dd.MouseX;
  LastY = dd.MouseY;
}

/****************************************************************************/

void sTestWindow::OnFrame()
{
}

/****************************************************************************/

sBool sTestWindow::OnCommand(sU32 cmd)
{
  return sFALSE;
}

/****************************************************************************/
/****************************************************************************/

sDialogWindow::sDialogWindow()
{
  SendTo = 0;
  Text = 0;
  CmdOk = 0;
  CmdCancel = 0;
  OkButton = 0;
  CanButton = 0;
  EditControl = 0;
  EditBuffer = 0;
  Edit = 0;
  EditSize = 0;
}

sDialogWindow::~sDialogWindow()
{
  if(EditBuffer)
    delete[] EditBuffer;
}

void sDialogWindow::Tag()
{
  sGuiWindow::Tag();
  sBroker->Need(SendTo);
}

/****************************************************************************/

void sDialogWindow::OnPaint()
{
  sRect r;

  r = Client;
  sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_BACK]);

  r.Extend(-10);
  if(Text)
    sPainter->PrintM(sGui->PropFont,r,sFA_TOP|sFA_LEFT,Text,sGui->Palette[sGC_TEXT]);
}

void sDialogWindow::OnLayout()
{
  if(CanButton)
  {
    CanButton->Position.x0 = Client.x0+10;
    CanButton->Position.y0 = Client.y1-10-20;
    CanButton->Position.x1 = Client.x0+10+70;
    CanButton->Position.y1 = Client.y1-10;
  }
  if(OkButton)
  {
    OkButton->Position.x0 = Client.x1-10-70;
    OkButton->Position.y0 = Client.y1-10-20;
    OkButton->Position.x1 = Client.x1-10;
    OkButton->Position.y1 = Client.y1-10;
  }
  if(EditControl)
  {
    EditControl->Position.x0 = Client.x0+10;
    EditControl->Position.y0 = Client.y1-40-20;
    EditControl->Position.x1 = Client.x1-10;
    EditControl->Position.y1 = Client.y1-40;
  }
}

sBool sDialogWindow::OnCommand(sU32 cmd)
{
  switch(cmd)
  {
  case 1:
    if(EditControl && EditBuffer && Edit)
    {
      sCopyString(Edit,EditBuffer,EditSize);
    } 
    if(SendTo)
    {
      sGui->Post(CmdOk,SendTo);
      sGui->SetFocus(SendTo);
    }
    else
      Post(CmdOk);
    if(Parent)
      Parent->RemChild(this);
    break;

  case 2:
    if(SendTo)
    {
      sGui->Post(CmdCancel,SendTo);
      sGui->SetFocus(SendTo);
    }
    else
      Post(CmdCancel);
    if(Parent)
      Parent->RemChild(this);
    break;
  }

  return sTRUE;
}

/****************************************************************************/

void sDialogWindow::InitAB(sGuiWindow *sendto,sChar *title,sChar *message,sU32 cmdok,sU32 cmdcan,sChar *ok,sChar *can)
{
  sSizeBorder *sb;

  SendTo = sendto;
  Flags |= sGWF_TOPMOST;
  sb = new sSizeBorder;
  sb->Title = title;
  AddBorder(sb);

  Position.Init(100,100,100+250,100+150);

  Text = message;
  CmdOk = cmdok;
  CmdCancel = cmdcan;

  if(ok)
  {
    OkButton = new sControl;
    OkButton->Button(ok,1);
    AddChild(OkButton);
  }
  if(can)
  {
    CanButton = new sControl;
    CanButton->Button(can,2);
    AddChild(CanButton);
  }

  sGui->GetRoot()->AddChild(this);
}

void sDialogWindow::InitOk(sGuiWindow *sendto,sChar *title,sChar *message,sU32 cmdok)
{
  InitAB(sendto,title,message,cmdok,0,"Ok",0);
}

void sDialogWindow::InitOkCancel(sGuiWindow *sendto,sChar *title,sChar *message,sU32 cmdok,sU32 cmdcan)
{
  InitAB(sendto,title,message,cmdok,cmdcan,"Ok","Cancel");
}

void sDialogWindow::InitYesNo(sGuiWindow *sendto,sChar *title,sChar *message,sU32 cmdok,sU32 cmdcan)
{
  InitAB(sendto,title,message,cmdok,cmdcan,"Yes","No");
}

void sDialogWindow::InitString(sChar *buffer,sInt size)
{
  EditBuffer = new sChar[size];
  Edit = buffer;
  EditSize = size;
  sCopyString(EditBuffer,Edit,EditSize);
  EditControl = new sControl;
  EditControl->EditString(0,EditBuffer,0,EditSize);
  EditControl->DoneCmd = 1;
  AddChild(EditControl);
}

/****************************************************************************/
/***                                                                      ***/
/***   frames                                                             ***/
/***                                                                      ***/
/****************************************************************************/

sOverlappedFrame::sOverlappedFrame()
{
  Flags |= sGWF_FLUSH;
  RightClickCmd = 0;
  SendTo = 0;
}

void sOverlappedFrame::OnCalcSize()
{
}

void sOverlappedFrame::OnLayout()
{
  sGuiWindow *win;
  sSizeBorder *sb;

  win = Childs;
  while(win)
  {
    sb = win->FindTitle();
    if(sb && sb->Maximised)
    {
      win->Position = Client;
    }
    else
    {
      if(win->Position.x0 < Client.x0) win->Position.x0 = Client.x0;
      if(win->Position.x1 > Client.x1) win->Position.x1 = Client.x1;
      if(win->Position.y0 < Client.y0) win->Position.y0 = Client.y0;
      if(win->Position.y1 > Client.y1) win->Position.y1 = Client.y1;
    }
    win = win->Next;
  }
}

void sOverlappedFrame::OnPaint()
{
  sPainter->Paint(sGui->AlphaMat,Client,sGui->Palette[0]);
  sPainter->Flush();
}

void sOverlappedFrame::OnFrame()
{
  sGuiWindow **p,*w0,*w1;
  sBool swap;

  p = &Childs;
  while(*p && (*p)->Next)
  {
    w0 = *p;
    w1 = (*p)->Next;
    swap = sFALSE;

    if((w0->Flags & sGWF_TOPMOST) == (w1->Flags & sGWF_TOPMOST))
    {
      if((w0->Flags & sGWF_CHILDFOCUS) && !(w1->Flags & sGWF_CHILDFOCUS))
        swap = sTRUE;
    }
    else if((w0->Flags & sGWF_TOPMOST) && !(w1->Flags & sGWF_TOPMOST))
      sSwap(*p,(*p)->Next);

    if(swap)
    {
      *p = w1;
      w0->Next = (*p)->Next;
      (*p)->Next = w0;
    }
    p = &(*p)->Next;
  }
}

void sOverlappedFrame::OnDrag(sDragData &dd)
{
  if(dd.Mode==sDD_START && (dd.Buttons&2))
  {
    if(SendTo)
      sGui->Post(RightClickCmd,SendTo);
    else
      Post(RightClickCmd);
  }
}

/****************************************************************************/
/****************************************************************************/

void sDummyFrame::OnCalcSize()
{
  if(Childs)
  {
    SizeX = Childs->SizeX;
    SizeY = Childs->SizeY;
  }
}

void sDummyFrame::OnLayout()
{
  if(Childs)
  {
    Childs->Position = Client;
  }
}

void sDummyFrame::OnPaint()
{
}

/****************************************************************************/
/****************************************************************************/

sMenuFrame::sMenuFrame()
{
  sInt i;

  SendTo = 0;
  Flags |= sGWF_SETSIZE|sGWF_TOPMOST;
  ColumnCount = 0;
  for(i=0;i<16;i++)
  {
    ColumnIndex[i] = 0xffff;
    ColumnWidth[i] = 0;
  }
}

void sMenuFrame::Tag()
{
  sGuiWindow::Tag();
  sBroker->Need(SendTo);
}

void sMenuFrame::OnCalcSize()
{
  sInt x,y,c,i;
  sGuiWindow *p;

  x = 10;
  y = 0;
  c = 0;
  i = 0;
  SizeX = 0;
  SizeY = 0;
  p = Childs;
  while(p)
  {
    x = sMax(x,p->SizeX);
    y = y + p->SizeY;
    p = p->Next;
    if(i==ColumnIndex[c])
    {
      SizeX += x+2;
      ColumnWidth[c] = x;
      SizeY = sMax(SizeY,y);
      x = 10;
      y = 0;
      c++;
    }
    i++;
  }
  SizeX += x;
  ColumnWidth[c] = x;
  SizeY = sMax(SizeY,y);
}

void sMenuFrame::OnLayout()
{
  sInt y,count,i,c,tx;
  sGuiWindow *p;

  tx = Client.x0;
  i = 0;
  c = 0;
  y = Client.y0;
  p = Childs;
  count = 0;
  while(p)
  {
    p->Position.x0 = tx;
    p->Position.y0 = y;
    y += p->SizeY;
    p->Position.x1 = tx+ColumnWidth[c];
    p->Position.y1 = y;
    p = p->Next;
    if(i==ColumnIndex[c])
    {
      y = Client.y0;
      tx += ColumnWidth[c]+2;
      c++;
    }
    i++;
  }
}

void sMenuFrame::OnPaint()
{
  sInt i,x;
  sPainter->Paint(sGui->FlatMat,Client,sGui->Palette[sGC_BUTTON]);

  x = Client.x0;
  for(i=0;i<ColumnCount;i++)
  {
    x += ColumnWidth[i];
    sPainter->Paint(sGui->FlatMat,x  ,Client.y0,1,Client.YSize(),sGui->Palette[sGC_LOW] );
    sPainter->Paint(sGui->FlatMat,x+1,Client.y0,1,Client.YSize(),sGui->Palette[sGC_HIGH]);
    x += 2;
  }

  if(!(Flags & (sGWF_CHILDFOCUS|sGWF_HOVER)))
    sGui->Post(1,this);
}

sBool sMenuFrame::OnCommand(sU32 cmd)
{
  if(cmd)
  {
    if(cmd!=1)
    {
      if(SendTo)
      {
        sGui->SetFocus(SendTo);
        sGui->Send(cmd,SendTo);
      }
      else
      {
        Post(cmd);
      }
    }
    if(Parent)
    {
      Parent->Flags |= sGWF_LAYOUT;
      Parent->RemChild(this);
    }
  }
  return cmd==1;
}

void sMenuFrame::OnKey(sU32 key)
{
  sGuiWindow *win;
  sControl *con;

  if(key&sKEYQ_SHIFT) key|= sKEYQ_SHIFT;
  if(key&sKEYQ_CTRL) key|= sKEYQ_CTRL;
  key = key & (0x8001ffff|sKEYQ_SHIFT|sKEYQ_CTRL);
  if(key==sKEY_ESCAPE/* || key==sKEY_APPFOCLOST*/)
  {
    if(Parent)
    {
      Parent->Flags |= sGWF_LAYOUT;
      Parent->RemChild(this);
    }
  }
  else
  {
    win = Childs;
    while(win)
    {
      if(win->GetClass()==sCID_CONTROL)
      {
        con = (sControl *)win;

        if(con->Shortcut == key)
          sGui->Post(con->DoneCmd,this);
      }
      win = win->Next;
    }
  }
}

sControl *sMenuFrame::AddMenu(sChar *name,sU32 cmd,sU32 shortcut)
{
  sControl *con;

  con = new sControl; 
  con->Menu(name,cmd,shortcut); 
  AddChild(con);

  return con;
}

sControl *sMenuFrame::AddCheck(sChar *name,sU32 cmd,sU32 shortcut,sInt state)
{
  sControl *con;

  con = new sControl; 
  con->MenuCheck(name,cmd,shortcut,state); 
  AddChild(con);

  return con;
}

void sMenuFrame::AddSpacer()
{
  sGuiWindow *con;

  con = new sMenuSpacerControl; 
  AddChild(con);
}

void sMenuFrame::AddColumn()
{
  if(ColumnCount<16)
  {
    ColumnIndex[ColumnCount] = GetChildCount()-1;
    ColumnWidth[ColumnCount] = 0;
    ColumnCount++;
  }
}

/****************************************************************************/
/****************************************************************************/

sGridFrame::sGridFrame()
{
  GridX = 1;
  GridY = 1;
  MinX = 0;
  MinY = 0;
}

void sGridFrame::OnCalcSize()
{
  SizeX = MinX*GridX;
  SizeY = MinY*GridY;
}

void sGridFrame::OnLayout()
{
  sGuiWindow *p;
  sInt x0,y0,xs,ys;
  sInt rx,ry;

  rx = GridX;
  ry = GridY;
  x0 = Client.x0;
  y0 = Client.y0;
  xs = Client.x1-Client.x0;
  ys = Client.y1-Client.y0;
  if(MinX) xs = MinX*GridX;
  if(MinY) ys = MinY*GridY;

  p = Childs;
  while(p)
  {
    rx = sMax(rx,p->LayoutInfo.x1);
    ry = sMax(ry,p->LayoutInfo.y1);
    p->Position.x0 = x0 + sMulDiv(p->LayoutInfo.x0,xs,GridX);
    p->Position.y0 = y0 + sMulDiv(p->LayoutInfo.y0,ys,GridY);
    p->Position.x1 = x0 + sMulDiv(p->LayoutInfo.x1,xs,GridX);
    p->Position.y1 = y0 + sMulDiv(p->LayoutInfo.y1,ys,GridY);
    p = p->Next;
  }

  SizeX = rx*MinX;
  SizeY = ry*MinY;
}

void sGridFrame::OnDrag(sDragData &dd)
{
  MMBScrolling(dd,DragStartX,DragStartY);
}

void sGridFrame::OnPaint()
{
  sPainter->Paint(sGui->FlatMat,Client,sGui->Palette[sGC_BUTTON]);
}

void sGridFrame::SetGrid(sInt x,sInt y,sInt xs,sInt ys)
{
  GridX = x;
  GridY = y;
  MinX = xs;
  MinY = ys;
}

void sGridFrame::Add(sGuiWindow *win,sInt x0,sInt y0,sInt xs,sInt ys)
{
  win->LayoutInfo.Init(x0,y0,x0+xs,y0+ys);
  AddChild(win);
}

void sGridFrame::Add(sGuiWindow *win,sRect r)
{
  win->LayoutInfo = r;
  AddChild(win);
}

void sGridFrame::AddLabel(sChar *name,sInt x0,sInt x1,sInt y)
{
  sControl *con;

  con = new sControl;
  con->Label(name);
  con->Style = sCS_LEFT;
  Add(con,x0,x1,y);
}

void sGridFrame::Add(sGuiWindow *win,sInt x0,sInt x1,sInt y)
{
  Add(win,x0,y,x1-x0,1);
}


sControl *sGridFrame::AddCon(sInt x0,sInt y0,sInt xs,sInt ys,sInt zones)
{
  sControl *con;
  con = new sControl;
  Add(con,x0,y0,xs,ys);
  con->Zones = zones;
  return con;
}

/****************************************************************************/

sVSplitFrame::sVSplitFrame()
{
  sInt i;

  for(i=0;i<16;i++)
    Pos[i] = 0;
  Width = 6;
  Count = 0;
  DragMode = 0;
  DragStart = 0;
}

void sVSplitFrame::OnCalcSize()
{
  sGuiWindow *win;

  SizeX = -Width;
  SizeY = 0;
  win = Childs;
  sVERIFY(win);
  while(win)
  {
    SizeX += Width+win->SizeX;
    SizeY = sMax(SizeY,win->SizeY);
    win = win->Next;
  }
}

void sVSplitFrame::OnLayout()
{
  sGuiWindow *win;
  sInt i,max,x;
  sRect r;

  r = Client;
  max = GetChildCount();
  if(max!=Count)
  {
    Count = max;

    win = Childs;
    i = 0;
    Pos[0] = -Width;
    while(win)
    {
      if(Pos[i+1]>0)
        Pos[i+1] = Pos[i+1];
      else if(Pos[i+1]<0)
        Pos[i+1] = ((r.x1-r.x0)*-Pos[i+1])>>16;
      else
        Pos[i+1] = (r.x1-r.x0-Width*(max-1))*(i+1)/(max) + Width*i;
      i++;
      win = win->Next;
    }
    Pos[i] = r.x1-r.x0;
  }
  else
  {
    Pos[Count] = r.x1-r.x0;
  }

  win = Childs;
  i = 0;
  while(win)
  {
    if(i<Count-1)
    {
      x = r.x1-r.x0-(Count-i-1)*Width;
      if(Pos[i+1]>x)
        Pos[i+1] = x;
    }

    win->Position.x0 = r.x0+Pos[i]+Width;
    win->Position.y0 = r.y0;
    win->Position.x1 = r.x0+Pos[i+1];
    win->Position.y1 = r.y1;
    i++;
    win = win->Next;
  }
}

void sVSplitFrame::OnPaint()
{
  sInt i;
  sRect r;

  for(i=1;i<Count;i++)
  {
    r.x0 = Client.x0+Pos[i];
    r.y0 = Client.y0;
    r.x1 = Client.x0+Pos[i]+Width;
    r.y1 = Client.y1;
    sGui->Bevel(r,DragMode==i);
    sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_BUTTON]);
  }
}

void sVSplitFrame::OnDrag(sDragData &dd)
{
  sInt i;
  switch(dd.Mode)
  {
  case sDD_START:
    DragMode = 0;
    for(i=1;i<Count;i++)
    {
      if(dd.MouseX>=Pos[i]+Client.x0 && dd.MouseX<Pos[i]+Client.x0+Width)
      {
        DragMode = i;
        DragStart = Pos[DragMode]-dd.MouseX;
      }
    }
    break;
  case sDD_DRAG:
    if(DragMode>0 && DragMode<Count)
    {
      i = sRange(DragStart+dd.MouseX,Pos[DragMode+1]-Width,Pos[DragMode-1]+Width);
      if(Pos[DragMode]!=i)
      {
        Pos[DragMode] = i;
        Flags |= sGWF_LAYOUT;
      }
    }
    break;
  case sDD_STOP:
    DragMode = 0;
    break;
  }
}

void sVSplitFrame::SplitChild(sInt pos,sGuiWindow *win)
{
  sInt i,max;

  max = GetChildCount();
  sVERIFY(pos<max);
  if(max<15)
  {
    for(i=max;i>=pos;i--)
      Pos[i+1] = Pos[i];
    Pos[pos+1] = (Pos[pos]+Pos[pos+2])/2;

    AddChild(pos,win);
  }
}

/****************************************************************************/

sHSplitFrame::sHSplitFrame()
{
  sInt i;

  for(i=0;i<16;i++)
    Pos[i] = 0;
  Width = 6;
  Count = 0;
  DragMode = 0;
  DragStart = 0;
}

void sHSplitFrame::OnCalcSize()
{
  sGuiWindow *win;

  SizeY = -Width;
  SizeX = 0;
  win = Childs;
  sVERIFY(win);
  while(win)
  {
    SizeY += Width+win->SizeY;
    SizeX = sMax(SizeX,win->SizeX);
    win = win->Next;
  }
}

void sHSplitFrame::OnLayout()
{
  sGuiWindow *win;
  sInt i,max,y;
  sRect r;

  r = Client;
  max = GetChildCount();
  if(max!=Count)
  {
    Count = max;

    win = Childs;
    i = 0;
    Pos[0] = -Width;
    while(win)
    {
      if(Pos[i+1]>0)
        Pos[i+1] = Pos[i+1];
      else if(Pos[i+1]<0)
        Pos[i+1] = ((r.y1-r.y0)*-Pos[i+1])>>16;
      else
        Pos[i+1] = (r.y1-r.y0-Width*(max-1))*(i+1)/(max) + Width*i;
      i++;
      win = win->Next;
    }
    Pos[i] = r.y1-r.y0;
  }
  else
  {
    Pos[Count] = r.y1-r.y0;
  }

  win = Childs;
  i = 0;
  while(win)
  {
    if(i<Count-1)
    {
      y = r.y1-r.y0-(Count-i-1)*Width;
      if(Pos[i+1]>y)
        Pos[i+1] = y;
    }

    win->Position.y0 = r.y0+Pos[i]+Width;
    win->Position.x0 = r.x0;
    win->Position.y1 = r.y0+Pos[i+1];
    win->Position.x1 = r.x1;
    i++;
    win = win->Next;
  }
}

void sHSplitFrame::OnPaint()
{
  sInt i;
  sRect r;

  for(i=1;i<Count;i++)
  {
    r.y0 = Client.y0+Pos[i];
    r.x0 = Client.x0;
    r.y1 = Client.y0+Pos[i]+Width;
    r.x1 = Client.x1;
    sGui->Bevel(r,DragMode==i);
    sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_BUTTON]);
  }
}

void sHSplitFrame::OnDrag(sDragData &dd)
{
  sInt i;
  switch(dd.Mode)
  {
  case sDD_START:
    DragMode = 0;
    for(i=1;i<Count;i++)
    {
      if(dd.MouseY>=Pos[i]+Client.y0 && dd.MouseY<Pos[i]+Client.y0+Width)
      {
        DragMode = i;
        DragStart = Pos[DragMode]-dd.MouseY;
      }
    }
    break;
  case sDD_DRAG:
    if(DragMode>0 && DragMode<Count)
    {
      i = sRange(DragStart+dd.MouseY,Pos[DragMode+1]-Width,Pos[DragMode-1]+Width);
      if(Pos[DragMode]!=i)
      {
        Pos[DragMode] = i;
        Flags |= sGWF_LAYOUT;
      }
    }
    break;
  case sDD_STOP:
    DragMode = 0;
    break;
  }
}

void sHSplitFrame::SplitChild(sInt pos,sGuiWindow *win)
{
  sInt i,max;

  max = GetChildCount();
  sVERIFY(pos<max);
  if(max<15)
  {
    for(i=max;i>=pos;i--)
      Pos[i+1] = Pos[i];
    Pos[pos+1] = (Pos[pos]+Pos[pos+2])/2;

    AddChild(pos,win);
  }
}

/****************************************************************************/

sSwitchFrame::sSwitchFrame()
{
  Current = 0;
}

void sSwitchFrame::OnCalcSize()
{
  sGuiWindow *win;
  sInt x,y;

  x = 0;
  y = 0;

  win = Childs;
  while(win)
  {
    x = sMax(x,win->SizeX);
    y = sMax(y,win->SizeY);
    win = win->Next;
  }
  SizeX = x;
  SizeY = y;
}

void sSwitchFrame::OnLayout()
{
  sGuiWindow *win;
  sInt i;

  win = Childs;
  i = 0;
  while(win)
  {
    if(i==Current)
      win->Flags &= ~sGWF_DISABLED;
    else
      win->Flags |= sGWF_DISABLED;
    win->Position = Client;
    win = win->Next;
    i++;
  }
}

void sSwitchFrame::OnPaint()
{
}

void sSwitchFrame::Set(sInt win)
{
  if(win>=0 && win<GetChildCount())
  {
    Current = win;
    Flags |= sGWF_LAYOUT;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   borders                                                            ***/
/***                                                                      ***/
/****************************************************************************/

void sFocusBorder::OnCalcSize()
{
  SizeX+=4;
  SizeY+=4;
}

void sFocusBorder::OnSubBorder()
{
  Parent->Client.Extend(-2);
}

void sFocusBorder::OnPaint()
{
  sRect r;
  r = Client;
  sGui->Bevel(r,Parent->Flags&sGWF_CHILDFOCUS?1:0);
}

/****************************************************************************/

void sNiceBorder::OnCalcSize()
{
  if(sGui->GetStyle()&sGS_SMALLFONTS)
  {
    SizeX+=2;
    SizeY+=2;
  }
  else
  {
    SizeX+=4;
    SizeY+=4;
  }
}

void sNiceBorder::OnSubBorder()
{
  if(sGui->GetStyle()&sGS_SMALLFONTS)
  {
    Parent->Client.Extend(-1);
  }
  else
  {
    Parent->Client.Extend(-2);
  }
}

void sNiceBorder::OnPaint()
{
  sRect r;
  r = Client;
  sGui->Bevel(r,0);
}

/****************************************************************************/

void sThinBorder::OnCalcSize()
{
  SizeX+=1;
  SizeY+=1;
}

void sThinBorder::OnSubBorder()
{
  Parent->Client.Extend(-1);
}

void sThinBorder::OnPaint()
{
  sRect r;
  r = Client;
  sGui->RectHL(r,1,sGui->Palette[sGC_DRAW],sGui->Palette[sGC_DRAW]);
}

/****************************************************************************/
/****************************************************************************/

sSizeBorder::sSizeBorder()
{
  DragStart.Init(0,0,0,0);
  DragMode = 0;
  DragClose = 0;
  Title = "no title";
  Maximised = 0;
  DontResize = 0;
}

void sSizeBorder::OnCalcSize()
{
  SizeX = 0;
  SizeY = 0;
  if(!Maximised)
  {
    if(sGui->GetStyle()&sGS_SMALLFONTS)
    {
      SizeX += 6;
      SizeY += 6;
    }
    else
    {
      SizeX += 8;
      SizeY += 8;
    }
  }
  if(Title && ((sGui->GetStyle()&sGS_MAXTITLE)||!Maximised))
    SizeY += 4+sPainter->GetHeight(sGui->PropFont);
}

void sSizeBorder::OnSubBorder()
{
  sInt w;
  w = (sGui->GetStyle()&sGS_SMALLFONTS) ? 2 : 4;
  if(!Maximised)
    Parent->Client.Extend(-w);
  if(Title && ((sGui->GetStyle()&sGS_MAXTITLE)||!Maximised))
  {
    Parent->Client.y0 += 4+sPainter->GetHeight(sGui->PropFont);
    TitleRect = Client;
    if(!Maximised)
      TitleRect.Extend(-4);
    TitleRect.y1 = Parent->Client.y0;
    CloseRect.x1 = TitleRect.x1-2;
    CloseRect.y0 = TitleRect.y0+2;
    CloseRect.y1 = TitleRect.y1-2;
    CloseRect.x0 = CloseRect.x1-CloseRect.YSize();
  }
}

void sSizeBorder::OnPaint()
{
  sRect r;
  sBool focus;
  sU32 col0,col1;

  r = Client;
  focus = (Parent->Flags & sGWF_CHILDFOCUS)?1:0;

  if(!Maximised)
  {
    sGui->Bevel(r,focus&&!Title);
    sGui->RectHL(r,2,sGui->Palette[sGC_BUTTON],sGui->Palette[sGC_BUTTON]);
  }
  if(Title && ((sGui->GetStyle()&sGS_MAXTITLE)||!Maximised))
  {
    sPainter->Paint(sGui->FlatMat,TitleRect,sGui->Palette[focus?sGC_SELBACK:sGC_BARBACK]);
    sPainter->PrintC(sGui->PropFont,TitleRect,sFA_LEFT|sFA_SPACE,Title,sGui->Palette[focus?sGC_SELECT:sGC_TEXT]);
    col0 = sGui->Palette[sGC_HIGH2];
    col1 = sGui->Palette[sGC_LOW2];
    if(DragClose==1)
      sSwap(col0,col1);
    r = CloseRect;
    sGui->RectHL(r,1,col0,col1);
    sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_BUTTON]);
    sPainter->PrintC(sGui->PropFont,CloseRect,0,"x",sGui->Palette[sGC_DRAW]);
  }
}

void sSizeBorder::OnDrag(sDragData &dd)
{
  sInt dx,dy;
  switch(dd.Mode)
  {
  case sDD_START:
    DragStart = Parent->Position;
    OldMouseX = dd.MouseX;
    OldMouseY = dd.MouseY;
    DragMode = 0;
    if(Title && CloseRect.Hit(dd.MouseX,dd.MouseY))
    {
      DragClose = 1;
    }
    else
    {
      if(dd.MouseX < Parent->Position.x0+20) DragMode |= 1;
      if(dd.MouseY < Parent->Position.y0+20) DragMode |= 2;
      if(dd.MouseX > Parent->Position.x1-20) DragMode |= 4;
      if(dd.MouseY > Parent->Position.y1-20) DragMode |= 8;
      if(DontResize) DragMode = 0;
      if(Title && ((sGui->GetStyle()&sGS_MAXTITLE)||!Maximised) && TitleRect.Hit(dd.MouseX,dd.MouseY))
      {
        DragMode = 15;
        if((dd.Buttons & sDDB_DOUBLE) && !DontResize)
        {
          Maximise(!Maximised);
          DragMode = 0;
        }
      }
      if(Maximised)
        DragMode = 0;
    }
    break;

  case sDD_DRAG:
    if(DragClose)
    {
      if(Title && CloseRect.Hit(dd.MouseX,dd.MouseY))
        DragClose = 1;
      else
        DragClose = 2;
    }
    dx = dd.MouseX - OldMouseX;
    dy = dd.MouseY - OldMouseY;
//    OldMouseX = dd.MouseX;
//    OldMouseY = dd.MouseY;
    if(DragMode==15)
    {
      if(Parent->Parent)
      {
        if(DragStart.x0+dx<Parent->Parent->Client.x0) dx-=DragStart.x0+dx-Parent->Parent->Client.x0;
        if(DragStart.y0+dy<Parent->Parent->Client.y0) dy-=DragStart.y0+dy-Parent->Parent->Client.y0;
        if(DragStart.x1+dx>Parent->Parent->Client.x1) dx-=DragStart.x1+dx-Parent->Parent->Client.x1;
        if(DragStart.y1+dy>Parent->Parent->Client.y1) dy-=DragStart.y1+dy-Parent->Parent->Client.y1;
      }
      Parent->Position.x0 = DragStart.x0+dx;
      Parent->Position.y0 = DragStart.y0+dy;
      Parent->Position.x1 = DragStart.x1+dx;
      Parent->Position.y1 = DragStart.y1+dy;
    }
    else
    {
      if(DragMode & 1) Parent->Position.x0 = sMin(DragStart.x0+dx,DragStart.x1-100);
      if(DragMode & 2) Parent->Position.y0 = sMin(DragStart.y0+dy,DragStart.y1-50);
      if(DragMode & 4) Parent->Position.x1 = sMax(DragStart.x1+dx,DragStart.x0+100);
      if(DragMode & 8) Parent->Position.y1 = sMax(DragStart.y1+dy,DragStart.y0+50);
    }
    if(DragMode & 15) Parent->Flags |= sGWF_LAYOUT;
    break;

  case sDD_STOP:
    DragMode = 0;
    if(DragClose==1)
    {
      if(!Parent->OnShortcut(sKEY_APPCLOSE))
        Parent->OnKey(sKEY_APPCLOSE);
    }
    DragClose = 0;
    break;
  }
}

void sSizeBorder::Maximise(sBool max)
{
  if(!Maximised && max)
  {
    MinPos = Parent->Position;
    Parent->Position = sGui->GetRoot()->Client;
    Parent->Flags |= sGWF_LAYOUT;
    Maximised = 1;
    Parent->OnKey(sKEY_APPMAX);
  }
  else if(Maximised && !max)
  {
    Maximised = 0;
    Parent->Position = MinPos;
    Parent->Client = MinPos;
    Parent->Flags |= sGWF_LAYOUT;
    Parent->OnKey(sKEY_APPMIN);
  }
}

/****************************************************************************/

sToolBorder::sToolBorder()
{
  Height = 10;
}

void sToolBorder::OnCalcSize()
{
  sInt x,y;
  sGuiWindow *p;

  x = 0;
  y = 10;
  p = Childs;
  while(p)
  {
    x = x + p->SizeX;
    y = sMax(y,p->SizeY);
    p = p->Next;
  }

  Height = y+1;
  SizeY += Height;
}

void sToolBorder::OnSubBorder()
{
  Parent->Client.y0 += Height;
}

void sToolBorder::OnLayout()
{
  sInt x;
  sGuiWindow *p;

  x = Client.x0;
  p = Childs;
  while(p)
  {
    p->Position.x0 = x;
    p->Position.y0 = Client.y0;
    x += p->SizeX;
    p->Position.x1 = x;
    p->Position.y1 = Client.y0+Height-1;
    p = p->Next;
  }
}

void sToolBorder::OnPaint()
{
  sRect r;

  r = Client;

  r.y1 = Client.y0+Height-1;
  sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_BUTTON]);

  r.y0 = Client.y0+Height-1;
  r.y1 = Client.y0+Height;
  sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_DRAW]);
}

sControl *sToolBorder::AddButton(sChar *name,sU32 cmd)
{
  sControl *con;

  con = new sControl; 
  con->Button(name,cmd); 
  AddChild(con);

  return con;
}

sControl *sToolBorder::AddMenu(sChar *name,sU32 cmd)
{
  sControl *con;

  con = new sControl; 
  con->Menu(name,cmd,0); 
  AddChild(con);

  return con;
}

/****************************************************************************/

sScrollBorder::sScrollBorder()
{
  Width = 15;
  EnableX = 1;
  EnableY = 1;
  EnableKnopX = 0;
  EnableKnopY = 0;
  DragMode = 0;
  DragStart = 0;
}

void sScrollBorder::OnCalcSize()
{
  Width = (sGui->GetStyle()&sGS_SMALLFONTS) ? 8 : 15;
  SizeX = 0;
  SizeY = 0;
  if(sGui->GetStyle()&sGS_SCROLLBARS)
  {
    if(EnableX)
    {
      Parent->Flags |= sGWF_SCROLLX;
      SizeY += Width;
    }
    if(EnableY)
    {
      Parent->Flags |= sGWF_SCROLLY;
      SizeX += Width;
    }
  }
}

void sScrollBorder::OnSubBorder()
{
  if(sGui->GetStyle()&sGS_SCROLLBARS)
  {
    if(EnableX)
    {
      BodyX = Client;
      Parent->Client.y1-=Width;
      BodyX.y0 = Parent->Client.y1;
      KnopX = BodyX;
    }
    if(EnableY)
    {
      BodyY = Client;
      Parent->Client.x1-=Width;
      BodyY.x0 = Parent->Client.x1;
      KnopY = BodyY;
    }
    if(EnableX && EnableY)
    {
      BodyS.Init(BodyY.x0,BodyX.y0,Client.x1,Client.y1);
      BodyX.x1 = BodyY.x0;
      BodyY.y1 = BodyX.y0;
    }
  }
}

void sScrollBorder::OnPaint()
{
  sInt max;

  EnableKnopX = 0;
  EnableKnopY = 0;

  if(sGui->GetStyle()&sGS_SCROLLBARS)
  {
    if(EnableX)
    {
      BodyX = Client;
      BodyX.y0 = BodyX.y1-Width;
      KnopX = BodyX;
    }
    if(EnableY)
    {
      BodyY = Client;
      BodyY.x0 = BodyY.x1-Width;
      KnopY = BodyY;
    }
    if(EnableX && EnableY)
    {
      BodyS.Init(BodyY.x0,BodyX.y0,Client.x1,Client.y1);
      BodyX.x1 = BodyY.x0;
      BodyY.y1 = BodyX.y0;
    }
    
    if(EnableX)
    {
      sPainter->Paint(sGui->FlatMat,BodyX,sGui->Palette[sGC_BARBACK]);
      max = Parent->RealX;
      if(Parent->SizeX > max && max>Width*2)
      {
        KnopXSize = sMulDiv(max,BodyX.XSize(),Parent->SizeX);
        if(KnopXSize<Width*2-4) KnopXSize = Width*2-4;
        if(KnopXSize>BodyX.XSize()) KnopXSize = BodyX.XSize();
        KnopX.x0 = BodyX.x0+sMulDiv(Parent->ScrollX,BodyX.XSize()-KnopXSize,Parent->SizeX-max);
        KnopX.x1 = KnopX.x0+KnopXSize;
        sGui->Button(KnopX,DragMode==1,0);
        EnableKnopX = 1;

        Parent->ScrollX = sRange<sInt>(Parent->ScrollX,Parent->SizeX-max,0);
      }
      else
        Parent->ScrollX = 0;
    }
    if(EnableY)
    {
      sPainter->Paint(sGui->FlatMat,BodyY,sGui->Palette[sGC_BARBACK]);
      max = Parent->RealY;
      if(Parent->SizeY > max && max>Width*2)
      {
        KnopYSize = sMulDiv(max,BodyY.YSize(),Parent->SizeY);
        if(KnopYSize<Width*2-4) KnopYSize = Width*2-4;
        if(KnopYSize>BodyY.YSize()) KnopYSize = BodyY.YSize();
        KnopY.y0 = BodyY.y0+sMulDiv(Parent->ScrollY,BodyY.YSize()-KnopYSize,Parent->SizeY-max);
        KnopY.y1 = KnopY.y0+KnopYSize;
        sGui->Button(KnopY,DragMode==2,0);
        EnableKnopY = 2;

        Parent->ScrollY = sRange<sInt>(Parent->ScrollY,Parent->SizeY-max,0);
      }
      else
        Parent->ScrollY = 0;
    }
    if(EnableX && EnableY)
    {
      sPainter->Paint(sGui->FlatMat,BodyS,sGui->Palette[sGC_BUTTON]);
    }
  }
}

void sScrollBorder::OnDrag(sDragData &dd)
{
  switch(dd.Mode)
  {
  case sDD_START:
    if(EnableKnopX && KnopX.Hit(dd.MouseX,dd.MouseY))
    {
      DragMode = 1;
      DragStart = Parent->ScrollX;
    }
    if(EnableKnopY && KnopY.Hit(dd.MouseX,dd.MouseY))
    {
      DragMode = 2;
      DragStart = Parent->ScrollY;
    }
    break;

  case sDD_DRAG:
    if(DragMode == 1)
    {
      Parent->ScrollX = DragStart+sMulDiv(dd.DeltaX,Parent->SizeX-Parent->RealX,BodyX.XSize()-KnopXSize);
      Parent->ScrollX = sRange<sInt>(Parent->ScrollX,Parent->SizeX-Parent->RealX,0);
    }
    if(DragMode == 2)
    {
      Parent->ScrollY = DragStart+sMulDiv(dd.DeltaY,Parent->SizeY-Parent->RealY,BodyY.YSize()-KnopYSize);
      Parent->ScrollY = sRange<sInt>(Parent->ScrollY,Parent->SizeY-Parent->RealY,0);
    }
    break;

  case sDD_STOP:
    DragMode = 0;
    break;
  }
}

/****************************************************************************/

sTabBorder::sTabBorder()
{
  DragMode = -1;
  DragStart = 0;
  Count = 0;
  Height = sPainter->GetHeight(sGui->PropFont)+4;
}

void sTabBorder::OnCalcSize()
{
  sInt i;

  Height = sPainter->GetHeight(sGui->PropFont)+2;
  SizeY = Height;
  SizeX = 0;
  for(i=0;i<Count;i++)
    SizeX += Tabs[i].Width;
}

void sTabBorder::OnSubBorder()
{
  Parent->Client.y0 += Height;
}

void sTabBorder::OnPaint()
{
  sInt i;
  sInt x;
  sRect r;


  r.y0 = Client.y0;
  r.y1 = Client.y0 + Height;
  x = Client.x0-Parent->ScrollX;
  for(i=0;i<Count;i++)
  {
    r.x0 = x;
    x+=Tabs[i].Width;
    r.x1 = x;
    sGui->Button(r,DragMode==i,Tabs[i].Name,sFA_LEFT);
  }

  r.x0 = x;
  r.x1 = Client.x1;
  sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_BUTTON]);
}

void sTabBorder::OnDrag(sDragData &dd)
{
  sInt i;
  sInt x;
  switch(dd.Mode)
  {
  case sDD_START:
    x = Client.x0-Parent->ScrollX;
    DragMode = -1;
    for(i=0;i<Count;i++)
    {
      x += Tabs[i].Width;
      if(dd.MouseX > x-2 && dd.MouseX < x+2)
      {
        DragMode = i;
        DragStart = Tabs[i].Width;
      }
    }
    break;
  case sDD_DRAG:
    if(DragMode>=0 && DragMode<Count)
    {
      x = DragStart+dd.DeltaX;
      if(x<8)
        x = 8;
      Tabs[DragMode].Width = x;
    }
    break;
  case sDD_STOP:
    DragMode = -1;
    break;
  }
}

void sTabBorder::SetTab(sInt nr,sChar *name,sInt width)
{
  Tabs[nr].Name = name;
  Tabs[nr].Width = width;
  Count = sMax(Count,nr+1);
}

sInt sTabBorder::GetTab(sInt nr)
{
  sInt i;
  sInt x;
  x=Client.x0-Parent->ScrollX;
  for(i=0;i<nr && i<Count;i++)
    x+=Tabs[i].Width;
  return x;
}

sInt sTabBorder::GetTotalWidth()
{
  sInt i;
  sInt x;
  x=0;
  for(i=0;i<Count;i++)
    x+=Tabs[i].Width;
  return x;
}

/****************************************************************************/

sStatusBorder::sStatusBorder()
{
  Count = 0;
  Height = sPainter->GetHeight(sGui->PropFont)+8;
}

void sStatusBorder::OnCalcSize()
{
  sInt i;

  Height = sPainter->GetHeight(sGui->PropFont)+8;
  SizeY = Height;
  SizeX = 0;
  for(i=0;i<Count;i++)
    SizeX += Tabs[i].Width;
}

void sStatusBorder::OnSubBorder()
{
  Parent->Client.y1 -= Height;
}

void sStatusBorder::OnPaint()
{
  sInt i;
  sInt x;
  sRect r;
  sInt total;

  total = GetTotalWidth();

  r = Client;
  r.y0 = Client.y1 - Height;
  sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_BUTTON]);
/*
  r.y0 = Client.y1 - Height;
  r.y1 = Client.y1 - Height+1;
  sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_DRAW]);
*/
  x = Client.x0-Parent->ScrollX;
  for(i=0;i<Count;i++)
  {
    r.y0 = Client.y1 - Height + 1;
    r.y1 = Client.y1;
    r.x0 = x;
    if(Tabs[i].Width==0)
      x+=Client.XSize()-total;
    else
      x+=Tabs[i].Width;
    r.x1 = x;
    r.Extend(-1);
    sGui->RectHL(r,1,sGui->Palette[sGC_LOW],sGui->Palette[sGC_HIGH]);
    sPainter->PrintC(sGui->PropFont,r,sFA_LEFT|sFA_SPACE,Tabs[i].Name,sGui->Palette[sGC_TEXT]);
  }
}

void sStatusBorder::SetTab(sInt nr,sChar *name,sInt width)
{
  Tabs[nr].Name = name;
  Tabs[nr].Width = width;
  Count = sMax(Count,nr+1);
}

sInt sStatusBorder::GetTotalWidth()
{
  sInt i;
  sInt x;
  x=0;
  for(i=0;i<Count;i++)
    x+=Tabs[i].Width;
  return x;
}

/****************************************************************************/
/***                                                                      ***/
/***   controls and buttons                                               ***/
/***                                                                      ***/
/****************************************************************************/

void sMenuSpacerControl::OnCalcSize()
{
  SizeX = 0;
  SizeY = 10;
}

void sMenuSpacerControl::OnPaint()
{
  sPainter->Paint(sGui->FlatMat,Client,sGui->Palette[sGC_BUTTON]);
  sPainter->Paint(sGui->FlatMat,Client.x0+2,Client.y0+4,Client.XSize()-4,1,sGui->Palette[sGC_LOW]);
  sPainter->Paint(sGui->FlatMat,Client.x0+2,Client.y0+5,Client.XSize()-4,1,sGui->Palette[sGC_HIGH]);
}

/****************************************************************************/
/****************************************************************************/

void sControlTemplate::Init()
{
  sSetMem(this,0,sizeof(*this));
  Type = sCT_NONE;
  Name = "";
  Step = 0.25f;
}

void sControlTemplate::Init(sInt type,sChar *name)
{
  Type = type;
  Name = name;
}

void sControlTemplate::InitNum(sF32 min,sF32 max,sF32 step,sF32 def)
{
  sInt i;
  for(i=0;i<4;i++)
  {
    Min[i] = min;
    Max[i] = max;
    Default[i] = def;
  }
  Step = step;
}

void sControlTemplate::InitNum(sF32 *min,sF32 *max,sF32 step,sF32 *def,sInt zones)
{
  sInt i;

  for(i=0;i<zones;i++)
  {
    Min[i] = min[i];
    Max[i] = max[i];
    Default[i] = def[i];
  }
  Zones = zones;
  Step = step;
}

sInt sControlTemplate::AddFlags(sGuiWindow *win,sU32 cmd,sPtr data)
{
  sControlTemplate temp;
  sControl *con;
  sChar *s;
  sInt x,y,lines;
  sInt max;
  sInt step;

  lines = 1;
  x = XPos;
  y = YPos;
  s = Cycle;
  step = 2;
  while(*s)
  {
    temp.Init();
    temp.Init(Type,0);
    x+=step;
    step=2;
    if(x>=12)
    {
      x=2;
      y++;
      lines++;
    }
    if(*s=='\n')
    {
      x=2;
      y++;
      lines++;
      s++;
    }
    while(*s=='#')
    {
      s++;
      step++;
    }
    if(*s=='*')
    {
      s++;
      temp.CycleShift = sScanInt(s);
    }
    if(*s=='+')
    {
      s++;
      sScanInt(s);
    }
    max = 1;
    temp.Cycle = s;
    while(*s!=':' && *s!=0)
    {
      if(*s++=='|')
        max++;
    }
    if(max>2)
      temp.Type = sCT_CHOICE;
    temp.CycleBits = 1;
    while((1<<temp.CycleBits) < max)
      temp.CycleBits++;

    con = new sControl;
    con->LayoutInfo.Init(x,y,x+step,y+1);
    con->InitTemplate(&temp,cmd,data);
    win->AddChild(con);
    if(*s==':')
      s++;
  }
  return lines;
}

/****************************************************************************/

sControl::sControl()
{
  Flags |= sGWF_PASSMMB;
  Type = sCT_NONE;
  Style = 0;
  Text = 0;
  TextSize = -1;
  TextColor = 0x00000000;
  BackColor = 0x00000000;
  Edit = 0;
  EditSize = 0;
  ChangeCmd = 0;
  DoneCmd = 0;
  Cycle = 0;
  CycleSize = 0;
  CycleShift = 0;
  CycleBits = 0;
  CycleOffset = 0;
  Shortcut = 0;
  EditMode = 0;
  EditZone = 0;
  Cursor = 0;
  Overwrite = 0;
  DragZone = 0;
  Zones = 1;
  DragMode = 0;
  LabelWidth = 0;
  State = 0;
  EditScroll = -65536;
  Swizzle[0] = 0;
  Swizzle[1] = 1;
  Swizzle[2] = 2;
  Swizzle[3] = 3;
  Min[0] = 0;
  Min[1] = 0;
  Min[2] = 0;
  Min[3] = 0;
  Max[0] = 1;
  Max[1] = 1;
  Max[2] = 1;
  Max[3] = 1;
  Default[0] = 0;
  Default[1] = 0;
  Default[2] = 0;
  Default[3] = 0;
  Step = 0.125f;
}

sControl::~sControl()
{
}

/****************************************************************************/

sInt sControl::GetCycle()
{
  sU32 val,mask;

  if(CycleBits==0)
    mask = ~0;
  else
    mask = ((1<<CycleBits)-1)<<CycleShift;
  val = DataU[0];
  return ((val&mask)>>CycleShift)-CycleOffset;
}

void sControl::SetCycle(sInt val)
{
  sU32 mask;

  if(CycleBits==0)
    mask = ~0;
  else
    mask = ((1<<CycleBits)-1)<<CycleShift;

  DataU[0] = (DataU[0] & ~mask) | (((val+CycleOffset)<<CycleShift)&mask);
}

void sControl::FormatValue(sChar *out,sInt zone)
{
  sF32 f;
  sInt i;
  sChar buffer[64];
  sChar c;

  if(Type!=sCT_MASK)
    zone = Swizzle[zone];
  else
    zone = Zones-zone-1;

  if(Type==sCT_INT)
  {
    sSPrintF(out,32,"%d",DataS[zone]);
  }
  if(Type==sCT_FLOAT)
  {
    f=Step;
    i = 0;
    while(f<0.49 && i<4)
    {
      i++;
      f=f*10;
    }
    if(i<1) i = 1;

    sSPrintF(buffer,sizeof(buffer),"%%.%df",i);
    sSPrintF(out,32,buffer,DataF[zone]);
  }
  if(Type==sCT_HEX)
  {
    sSPrintF(out,32,"%08x",DataU[zone]);
  }
  if(Type==sCT_RGBA || Type==sCT_RGB)
  {
    sSPrintF(out,32,"%02x",(DataU[zone]>>8)&255);
  }
  if(Type==sCT_URGBA || Type==sCT_URGB)
  {
    sSPrintF(out,32,"%02x",(DataU8[zone])&255);
  }
  if(Type==sCT_FIXED)
  {
    f=Step/65536.0f;
    i = 0;
    while(f<0.49 && i<4)
    {
      i++;
      f=f*10;
    }

    sSPrintF(buffer,sizeof(buffer),"%%.%df",i);
    sSPrintF(out,32,buffer,DataS[zone]/65536.0f);
  }
  if(Type==sCT_MASK)
  {
    out[0] = '0';
    c = '1';
    if(Cycle && sGetStringLen(Cycle)>=Zones/8)
    {
      out[0] = ' ';
      if(sGetStringLen(Cycle)<Zones)
        c = Cycle[zone/8];
      else
        c = Cycle[zone];
    }
    if((DataU[0]>>zone)&1)
      out[0] = c;
    out[1] = 0;
  }
}

void sControl::OnCalcSize()
{
  sChar buffer[64];

  if((Type==sCT_BUTTON || Type==sCT_CHECKMARK) && Text)
  {
    SizeX = sPainter->GetWidth(sGui->PropFont,Text,TextSize) + sPainter->GetWidth(sGui->PropFont,"  ")+4;
    if(Shortcut)
    {
      sSystem->GetKeyName(buffer,sizeof(buffer),Shortcut);
      SizeX += sPainter->GetWidth(sGui->PropFont,buffer)
             + sPainter->GetWidth(sGui->PropFont,"  ");
    }
    if(Type==sCT_CHECKMARK)
      SizeX += sPainter->GetWidth(sGui->PropFont,"XX");
  }
  else
  {
    SizeX = 120;
  }
  SizeY = 4+sPainter->GetHeight(sGui->PropFont); 
  if(Style & sCS_FATBORDER)
  {
    SizeX += 2;
    SizeY += 2;
    if(!(sGui->GetStyle()&sGS_SMALLFONTS))
    {
      SizeX += 2;
      SizeY += 2;
    }
  }
  if(Style & sCS_NOBORDER)
  {
    SizeX -= 2;
    SizeY -= 2;
  }
}

void sControl::OnPaint()
{
  sBool select;
  sInt align;
  sInt val;
  sU32 col0,col1,col;
  sChar *str;
  sInt strs;
  sChar buffer[64];
//  sU32 cols[4];
  sInt i,x0,x1,xs,tw,space;
  sRect rr,rx,rc,rl;
  sInt font;

// init 

  val = 0;
  if(Data)
    val = *DataU;

  select = 0;
  col0 = sGui->Palette[sGC_BUTTON];
  col1 = sGui->Palette[sGC_TEXT];
  align = 0;
  str = 0;
  strs = -1;

// change by style

  if(Type==sCT_BUTTON || Type==sCT_CHECKMARK || Type==sCT_CYCLE || Type==sCT_CHOICE)
  {
    select = DragMode;
    if(!(Flags&sGWF_HOVER))
      select = 0;
  }
  if(Type==sCT_TOGGLE)
  {
    select = val;
  }
  if((Style & sCS_FACESEL) && val)
  {
    col0 = sGui->Palette[sGC_SELBACK];
    col1 = sGui->Palette[sGC_SELECT];
  }
  if((Style & sCS_HOVER) && (Flags & sGWF_HOVER))
  {
    col1 = sGui->Palette[sGC_HIGH];
  }
  if((Style & sCS_LEFT))
  {
    align = sFA_LEFT|sFA_SPACE;
  }
  if(Shortcut)
  {
    sSystem->GetKeyName(buffer,sizeof(buffer),Shortcut);
    str = buffer;
  }
  if(Type==sCT_CYCLE || Type==sCT_CHOICE)
  {
    str = Cycle;
    i = GetCycle();
    while(i>0 && *str!=0 && *str!=':' )
    {
      if(*str=='|')
        i--;
      str++;
    }
    strs = 0;
    while(str[strs]!=0 && str[strs]!=':' && str[strs]!='|')
      strs++;
  }
  space = sPainter->GetWidth(sGui->PropFont," ");

// draw

  rc = Client;
  rl = Client;
  if(Style & sCS_SIDELABEL)
  {
    if(LabelWidth)
      rl.x1 = rc.x0 = rc.x0+LabelWidth;
    else
      rl.x1 = rc.x0 = rc.x0+Client.XSize()/4;
  }
  if(Type==sCT_MASK && rc.XSize()>rc.YSize()*Zones)
    rc.x1 = rc.x0 + rc.YSize()*Zones;

  if(Style & sCS_FATBORDER)
    sGui->Bevel(rc,select);
  else if(!(Style & sCS_NOBORDER))
    sGui->RectHL(rc,1,sGui->Palette[sGC_TEXT],sGui->Palette[sGC_TEXT]);
  if(Style & sCS_EDITCOL)
  {
    if(Type==sCT_URGB || Type==sCT_URGBA)
    {
      col0 = ((DataU8[2]<<16)&0x00ff0000)
           | ((DataU8[1]<< 8)&0x0000ff00)
           | ((DataU8[0]    )&0x000000ff)
           |                  0xff000000;

      if(DataU8[0]+DataU8[1]+DataU8[2] > 0x180)
        col1 = 0xff000000;
      else 
        col1 = 0xffffffff;
    }
    else
    {
      col0 = ((DataU[0]<<8)&0x00ff0000)
          | ((DataU[1]   )&0x0000ff00)
          | ((DataU[2]>>8)&0x000000ff)
          |                0xff000000;

      if(DataU[0]+DataU[1]+DataU[2] > 0x18000)
        col1 = 0xff000000;
      else 
        col1 = 0xffffffff;
    }
  }
  if(TextColor!=0)
    col1 = TextColor;
  if(BackColor!=0)
    col0 = BackColor;

/*
  if(Style & sCS_EDITCOL)
  {
    y = rc.y1-4;
    sPainter->Paint(sGui->FlatMat,rc.x0,rc.y0,rc.x1-rc.x0,y-rc.y0,col0);
    for(i=0;i<Zones;i++)
      sPainter->Paint(sGui->FlatMat,rc.x0+(rc.x1-rc.x0)*i/Zones,y,(rc.x1-rc.x0)*(i+1)/Zones-(rc.x1-rc.x0)*(i)/Zones,rc.y1-y,cols[i]);
  }
  else*/
  {
    sPainter->Paint(sGui->FlatMat,rc,col0);
    if(Style & sCS_ZONES)
    {
      for(i=1;i<Zones;i++)
        sPainter->Paint(sGui->FlatMat,rc.x0+(rc.x1-rc.x0)*i/Zones,rc.y0,Type==sCT_MASK &&(i&7)==0?2:1,rc.y1-rc.y0,sGui->Palette[sGC_DRAW]);
    }
  }
  if(str && strs)
  {
    sPainter->PrintC(sGui->PropFont,rc,Text ? (sFA_RIGHT|sFA_SPACE) : (sFA_LEFT|sFA_SPACE),str,col1,strs);
  }

  if(Text)
  {
    if(Type==sCT_CHECKMARK)
    {
      if(State)
        sPainter->PrintC(sGui->PropFont,rl,sFA_LEFT|sFA_SPACE,"X",sGui->Palette[sGC_TEXT]);
      rl.x0 += sPainter->GetWidth(sGui->PropFont,"XX");
    }
    sCopyString(buffer,Text,sizeof(buffer));
    if(!(Style & sCS_SIDELABEL) && (Style & sCS_EDIT))
    {
      sAppendString(buffer,":",sizeof(buffer));
      align = sFA_LEFT|sFA_SPACE;
    }
    if(Style & (sCS_RIGHTLABEL|sCS_SIDELABEL))
    {
      align = sFA_RIGHT|sFA_SPACE;
    }

    if(Style & sCS_GLOW)
    {
      col = sGui->Palette[sGC_BACK];
/*
      rx = Client; rx.x0++;rx.x1++;rx.y0++;rx.y1++;
      sPainter->PrintC(sGui->PropFont,rx,align,buffer,col,TextSize);
      rx = Client; rx.x0--;rx.x1--;rx.y0++;rx.y1++;
      sPainter->PrintC(sGui->PropFont,rx,align,buffer,col,TextSize);
      rx = Client; rx.x0--;rx.x1--;rx.y0--;rx.y1--;
      sPainter->PrintC(sGui->PropFont,rx,align,buffer,col,TextSize);
      rx = Client; rx.x0++;rx.x1++;rx.y0--;rx.y1--;
      sPainter->PrintC(sGui->PropFont,rx,align,buffer,col,TextSize);
*/
      rx = Client; rx.x0++;rx.x1++;rx.y0;rx.y1;
      sPainter->PrintC(sGui->PropFont,rx,align,buffer,col,TextSize);
      rx = Client; rx.x0--;rx.x1--;rx.y0;rx.y1;
      sPainter->PrintC(sGui->PropFont,rx,align,buffer,col,TextSize);
      rx = Client; rx.x0;rx.x1;rx.y0++;rx.y1++;
      sPainter->PrintC(sGui->PropFont,rx,align,buffer,col,TextSize);
      rx = Client; rx.x0;rx.x1;rx.y0--;rx.y1--;
      sPainter->PrintC(sGui->PropFont,rx,align,buffer,col,TextSize);
    }
    sPainter->PrintC(sGui->PropFont,rl,align,buffer,(Style&sCS_SIDELABEL) ? sGui->Palette[sGC_TEXT] : col1,TextSize);
  }

  if((Style & sCS_EDIT) || ((Style&sCS_EDITNUM) && EditMode==1))
  {
    rr = rc;
    col = col1;
    if(Style & sCS_EDITNUM)
      col = 0xffc00000;
    if((Style & sCS_EDITNUM) && (Style & sCS_ZONES))
    {
      rr.x0 = rc.x0 + (rc.x1-rc.x0)*(EditZone+0)/Zones;
      rr.x1 = rc.x0 + (rc.x1-rc.x0)*(EditZone+1)/Zones;
    }
//    rr.y0++;
//    rr.y1--;
    tw = sPainter->GetWidth(sGui->PropFont,Edit) + space;

    if(!(Style & sCS_SIDELABEL) && Text)
    {
      rr.x0 += sPainter->GetWidth(sGui->PropFont,Text) + sPainter->GetWidth(sGui->PropFont,": ");
    }

    if(EditScroll==-65536)
      EditScroll = -space;
    if(Flags & sGWF_FOCUS)
    {
      x0 = sPainter->GetWidth(sGui->PropFont,Edit,Cursor);
      xs = rr.XSize();
      
      if(x0-EditScroll>xs-30)
        EditScroll = x0-xs+30;
      if(x0-EditScroll<30)
        EditScroll = x0-30;
      if(tw-EditScroll<xs)
        EditScroll = tw-xs+1;
      if(0-EditScroll>-space)
        EditScroll = -space;

      if(Edit[Cursor]==0)
        x1 = x0+space;
      else
        x1 = sPainter->GetWidth(sGui->PropFont,Edit,Cursor+1);

      if(Overwrite)
        sPainter->Paint(sGui->FlatMat,rr.x0+x0-EditScroll,rr.y0,x1-x0,rr.y1-rr.y0,sGui->Palette[sGC_SELBACK]);
      else
        sPainter->Paint(sGui->FlatMat,rr.x0+x0-1-EditScroll,rr.y0,2,rr.y1-rr.y0,sGui->Palette[sGC_SELBACK]);
    }
    sGui->Clip(rr);
    rx = rr;
    rx.x0 -= EditScroll;
    rr.x1 += 4096;
    sPainter->PrintC(sGui->PropFont,rx,sFA_LEFT,Edit,col);
    sGui->ClearClip();
  }

  if(Style & sCS_EDITNUM)
  {
    align = (Text?sFA_RIGHT:sFA_LEFT)|sFA_SPACE;
    if(Type==sCT_MASK) align = 0;
    if(Style & sCS_EDITCOL) align = sFA_TOP|sFA_RIGHT|sFA_SPACE;
    font = Type==sCT_HEX?sGui->FixedFont:sGui->PropFont;

    if(Style & sCS_ZONES)
    {
      rr = rc;
      for(i=0;i<Zones;i++)
      {
        if(!EditMode || EditZone!=i)
        {
          FormatValue(buffer,i);
          rr.x0 = rc.x0 + (rc.x1-rc.x0)*(i+0)/Zones;
          rr.x1 = rc.x0 + (rc.x1-rc.x0)*(i+1)/Zones;
          sPainter->PrintC(font,rr,align,buffer,col1);
          if(Type!=sCT_MASK)
          {
            if(Swizzle[i]==DragZone && (Flags&sGWF_CHILDFOCUS))
              sPainter->Paint(sGui->FlatMat,rr.x0+sPainter->GetWidth(font," "),rr.y0,2,rr.y1-rr.y0,sGui->Palette[sGC_SELBACK]);
          }
          else
          {
            if(DragZone==i && (Flags&sGWF_CHILDFOCUS))
              sPainter->Paint(sGui->FlatMat,rr.x0+sPainter->GetWidth(font," "),rr.y0,2,rr.y1-rr.y0,sGui->Palette[sGC_SELBACK]);
          }
        }
      }
    }
    else if(!EditMode)
    {
      FormatValue(buffer,0);
      sPainter->PrintC(font,rc,align,buffer,col1);
      if(Flags&sGWF_CHILDFOCUS)
        sPainter->Paint(sGui->FlatMat,rc.x0+sPainter->GetWidth(font," "),rc.y0,2,rc.y1-rc.y0,sGui->Palette[sGC_SELBACK]);
    }
  }
}

void sControl::OnKey(sU32 key)
{
  sChar *s;
  sF32 fval;
  sInt len,i;

  if(key&sKEYQ_SHIFT) key|=sKEYQ_SHIFT;
  if(key&sKEYQ_CTRL) key|=sKEYQ_CTRL;

  if((!EditMode) && Edit)
  {
    switch(key&(0x8001ffff|sKEYQ_CTRL|sKEYQ_SHIFT))
    {
    case sKEY_HOME:
      if(Type!=sCT_FLOAT && Type!=sCT_URGB && Type!=sCT_URGBA)
      {
        DataS[DragZone] = Default[DragZone];
      }
      if(Type==sCT_FLOAT)
      {
        DataF[DragZone] = Default[DragZone];
      }
      break;
    case sKEY_HOME|sKEYQ_CTRL:
      if(Type!=sCT_FLOAT && Type!=sCT_URGB && Type!=sCT_URGBA)
      {
        for(i=0;i<Zones;i++)
          DataS[i] = Default[i];
      }
      if(Type==sCT_FLOAT)
      {
        for(i=0;i<Zones;i++)
          DataF[i] = Default[i];
      }
      break;

    case sKEY_END:
      if(Type!=sCT_FLOAT && Type!=sCT_URGB && Type!=sCT_URGBA)
      {
        DataS[DragZone] = Max[DragZone];
      }
      if(Type==sCT_FLOAT)
      {
        DataF[DragZone] = Max[DragZone];
      }
      break;

    case sKEY_END|sKEYQ_CTRL:
      if(Type!=sCT_FLOAT && Type!=sCT_URGB && Type!=sCT_URGBA)
      {
        for(i=0;i<Zones;i++)
          DataS[i] = Max[i];
      }
      if(Type==sCT_FLOAT)
      {
        for(i=0;i<Zones;i++)
          DataF[i] = Max[i];
      }
      break;

    default:
      if((key&0x8001ffff)>=0x20 && (key&0x8001ffff)<0x0001000f)
      {
        EditMode = 1;
        EditZone = 0;
        if(Type!=sCT_MASK && (Style&sCS_ZONES))
        {
          for(i=0;i<Zones;i++)
          {
            if(DragZone==Swizzle[i])
            {
              EditZone = i;
              break;
            }
          }
        }
        else
          EditZone = DragZone;
        Cursor = 0;
        FormatValue(Edit,EditZone);
      }
    }
  }

  if(!EditMode || !Edit)
    return;

  len = sGetStringLen(Edit);
  if(Cursor>len)
    Cursor = len;
  switch(key&0x8001ffff)
  {
  case sKEY_LEFT:
    if(Cursor>0)
      Cursor--;
    break;
  case sKEY_RIGHT:
    if(Cursor<len)
      Cursor++;
    break;
  case sKEY_BACKSPACE:
    if(Cursor==0)
      break;
    Cursor--;
  case sKEY_DELETE:
    s = Edit+Cursor;
    while(*s)
    {
      s[0] = s[1];
      s++;
    }
    if(!(Style & sCS_EDITNUM))
      Post(ChangeCmd);
    break;
  case sKEY_INSERT:
    Overwrite = !Overwrite;
    break;
  case sKEY_ESCAPE:
    EditMode = 0;
    Post(DoneCmd);
    break;

  case sKEY_ENTER:
  case sKEY_APPFOCLOST:
    if(Style & sCS_EDITNUM)
    {
      EditMode = 0;
      s = Edit;
      switch(Type)
      {
      case sCT_FIXED:
        sScanSpace(s);
        fval = sScanFloat(s);
        sScanSpace(s);
        if(*s==0)
        {
          DataS[DragZone] = sRange<sInt>(fval*0x10000,Max[DragZone],Min[DragZone]);
          Post(DoneCmd);
        }
        break;
      case sCT_INT:
        sScanSpace(s);
        i = sScanInt(s);
        sScanSpace(s);
        if(*s==0)
        {
          DataS[DragZone] = sRange<sInt>(i,Max[DragZone],Min[DragZone]);
          Post(DoneCmd);
        }
        break;
      case sCT_HEX:
        sScanSpace(s);
        i = sScanHex(s);
        sScanSpace(s);
        if(*s==0)
        {
          DataU[DragZone] = sRange<sU32>(i,Max[DragZone],Min[DragZone]);
          Post(DoneCmd);
        }
        break;
      case sCT_RGBA:
      case sCT_RGB:
        sScanSpace(s);
        i = sScanHex(s);
        sScanSpace(s);
        if(*s==0)
        {
          DataU[DragZone] = sRange<sU32>(i,255,0);
          DataU[DragZone] |= DataU[DragZone]<<8;
          Post(DoneCmd);
        }
        break;
      case sCT_URGBA:
      case sCT_URGB:
        sScanSpace(s);
        i = sScanHex(s);
        sScanSpace(s);
        if(*s==0)
        {
          DataU8[DragZone] = sRange<sU32>(i,255,0);
          Post(DoneCmd);
        }
        break;
      case sCT_FLOAT:
        sScanSpace(s);
        fval = sScanFloat(s);
        sScanSpace(s);
        if(*s==0)
        {
          DataF[DragZone] = sRange<sF32>(fval,Max[DragZone],Min[DragZone]);
          Post(DoneCmd);
        }
        break;
      }
      /* convert string to number here! */
    }
    else
      Post(DoneCmd);
    break;
  default:
    key = key&0x8001ffff;
    if((key>=0x20 && key<=0x7e) || (key>=0xa0 && key<=0xff))
    {
      if(len<EditSize-1)
      {
        for(i=len;i>Cursor;i--)
          Edit[i] = Edit[i-1];
        Edit[Cursor++] = key;
        Edit[len+1] = 0;
        if(!(Style & sCS_EDITNUM))
          Post(ChangeCmd);
      }
    }
    break;
  }
}

void sControl::OnDrag(sDragData &dd)
{
  sInt i,i0,i1;
  sRect r;
  sInt speed;

  speed = (dd.Buttons==2) ? 16 : 1;

  switch(dd.Mode)
  {
  case sDD_START:
    DragMode = 0;
    r = Client;
    if(Style & sCS_SIDELABEL)
    {
      if(LabelWidth)
        r.x0 += LabelWidth;
      else
        r.x0 += r.XSize()/4;
    }
    if(Type==sCT_MASK && r.XSize()>r.YSize()*Zones)
      r.x1 = r.x0 + r.YSize()*Zones;


    if(Type==sCT_TOGGLE)
    {
      *DataU = !*DataU;
      Post(DoneCmd);
    }
    if(Type==sCT_CHOICE)
    {
      sGui->Post(1,this);
    }

    if(EditMode==0)
    {
      DragZone = 0;
      if(r.x1>r.x0 && (Style&sCS_ZONES))
      {
        DragZone = (dd.MouseX-r.x0)*Zones/(r.x1-r.x0);
        if(dd.MouseX >= r.x0 && DragZone>=0 && DragZone<Zones)
        {
          if(Type!=sCT_MASK)
          {
            for(i=0;i<4;i++)
            {
              if(i == Swizzle[DragZone])
              {
                DragZone = i;
                break;
              }
            }
          }
          else
            DragZone = Zones-1-DragZone;
        }
        else
        {
          DragZone = 0;
        }
      }
      if(DragZone>=0 && DragZone<Zones)
      {
        DragMode = 1;
        if(Style&sCS_EDITNUM)
        {
          if(Type==sCT_MASK)
          {
            DragStartU[0] = DataU[0];
          }
          else if (Type==sCT_URGB || Type==sCT_URGBA)
          {
            if(sSystem->GetKeyboardShiftState()&sKEYQ_CTRL)
            {
              for(i=0;i<Zones;i++)
              {
                DragStartU[i] = DataU8[i];
              }
              DragMode = 2;
            }
            else
            {
              DragStartU[DragZone] = DataU8[DragZone];
            }
          }
          else
          {
            if(Zones>=1)
            {
              if(sSystem->GetKeyboardShiftState()&sKEYQ_CTRL)
              {
                for(i=0;i<Zones;i++)
                {
                  DragStartU[i] = DataU[i];
                }
                DragMode = 2;
              }
              else
              {
                DragStartU[DragZone] = DataU[DragZone];
              }
            }
            else
            {
              DragStartU[DragZone] = DataU[DragZone];
            }
          }
        }
      }
    }
    break;
  case sDD_DRAG:
    if(DragMode && (Style&sCS_EDITNUM))
    {
      i0 = DragZone;
      i1 = DragZone+1;
      if(DragMode==2)
      {
        i0 = 0;
        i1 = Zones;
      }
      for(i=i0;i<i1;i++)
      {
        if(Type==sCT_INT)
        {
          DataS[i] = sRange<sInt>(DragStartS[i]+dd.DeltaX*Step*speed,Max[i],Min[i]);
        }
        if(Type==sCT_HEX)
        {
          DataU[i] = sRange<sU32>(DragStartU[i]+dd.DeltaX*Step*speed,Max[i],Min[i]);
        }
        if(Type==sCT_FLOAT)
        {
          DataF[i] = sRange<sF32>(DragStartF[i]+dd.DeltaX*Step*speed,Max[i],Min[i]);
        }
        if(Type==sCT_FIXED)
        {
          DataS[i] = sRange<sInt>(DragStartS[i]+dd.DeltaX*((sInt)Step)*speed,Max[i],Min[i]);
        }
        if(Type==sCT_RGB)
        {
          DataS[i] = sRange<sInt>(DragStartS[i]+dd.DeltaX*((sInt)Step)*speed,Max[i],Min[i]);
        }
        if(Type==sCT_RGBA)
        {
          if(DragMode!=2 || i!=3)
          {
            DataS[i] = sRange<sInt>(DragStartS[i]+dd.DeltaX*((sInt)Step)*speed,Max[i],Min[i]);
          }
        }
        if(Type==sCT_URGB)
        {
          DataU8[i] = sRange<sInt>(DragStartS[i]+dd.DeltaX*Step*speed,Max[i],Min[i]);
        }
        if(Type==sCT_URGBA)
        {
          if(DragMode!=2 || i!=3)
          {
            DataU8[i] = sRange<sInt>(DragStartS[i]+dd.DeltaX*Step*speed,Max[i],Min[i]);
          }
        }
        if(Type==sCT_MASK)
        {
          DataU[0]=DragStartU[0]^(1<<i);
        }
      }
      Post(ChangeCmd);
    }
    break;
  case sDD_STOP:
    DragMode = 0;
    if((Type==sCT_BUTTON || Type==sCT_CHECKMARK) && (Flags&sGWF_HOVER))
    {
      Post(DoneCmd);
    }
    if(Style&sCS_EDITNUM)
    {
      Post(DoneCmd);
    }
    if(Type==sCT_CYCLE && (Flags&sGWF_HOVER))
    { 
      SetCycle((GetCycle()+1)%CycleSize);
      Post(DoneCmd);
    }
    break;
  }
}

sBool sControl::OnCommand(sU32 cmd)
{
  sMenuFrame *mf;
  sChar *str,*s0;
  sInt i;

  switch(cmd)
  {
  case 1:
    mf = new sMenuFrame;
    mf->AddBorder(new sNiceBorder);
    mf->SendTo = this;
    str = Cycle;
    i=0;
    while(*str && *str!=':')
    {
      s0 = str;
      while(*str && *str!=':' && *str!='|')
        str++;
      mf->AddMenu(s0,128+i,0)->TextSize = str-s0;
      i++;
      if(*str=='|')
        str++;
    }
    sGui->AddPulldown(mf);
    return sTRUE;
  default:
    if(cmd>=128 && cmd<(sU32)128+CycleSize)
    {
      SetCycle(cmd-128);
      Post(DoneCmd);
      return sTRUE;
    }
    return sFALSE;
  }
}

/****************************************************************************/

void sControl::Init(sInt type,sU32 style,sChar *label,sU32 cmd,sPtr data)
{
  Type = type;
  Style = style;
  Text = label;
  DoneCmd = cmd;
  ChangeCmd = cmd;
  Data = data;
}

void sControl::InitNum(sF32 min,sF32 max,sF32 step,sF32 def)
{
  Min[0] = Min[1] = Min[2] = Min[3] = min;
  Max[0] = Max[1] = Max[2] = Max[3] = max;
  Step = step;
  Default[0] = Default[1] = Default[2] = Default[3] = def;
  Edit = NumEditBuf;
  EditSize = sizeof(NumEditBuf);
  if(Zones>1)
    Style |= sCS_ZONES;
}

void sControl::InitNum(sF32 *min,sF32 *max,sF32 step,sF32 *def,sInt zones)
{
  sInt i;

  for(i=0;i<zones;i++)
  {
    Min[i] = min[i];
    Max[i] = max[i];
    Default[i] = def[i];
  }
  Step = step;
  Edit = NumEditBuf;
  EditSize = sizeof(NumEditBuf);
  Zones = zones;
  Style |= sCS_ZONES;
}

void sControl::InitCycle(sChar *str)
{
  Cycle = str;
  CycleSize = 1;

  while(*str && *str!=':')
  {
    if(*str=='|')
      CycleSize++;
    str++;
  }
}

void sControl::InitTemplate(sControlTemplate *temp,sU32 cmd,sPtr data)
{
  sChar *name;
  sInt i;

  name = temp->Name;
  if(name && name[0]=='+')
    name++;
  Zones = temp->Zones;
  if(Zones==0)
    Zones=1;

  switch(temp->Type)
  {
  case sCT_NONE:
    break;
  case sCT_LABEL:
    Label(name);
    break;
  case sCT_BUTTON:
    Button(name,cmd);
    break;
  case sCT_CHECKMARK:
    Button(name,cmd);
    Type = sCT_CHECKMARK;
    break;
  case sCT_TOGGLE:
    EditBool(cmd,(sInt *)data,name);
    break;
  case sCT_CYCLE:
    EditCycle(cmd,(sInt *)data,name,temp->Cycle);
    CycleOffset = temp->Size;
    CycleShift = temp->CycleShift;
    CycleBits = temp->CycleBits;
    break;
  case sCT_CHOICE:
    EditChoice(cmd,(sInt *)data,name,temp->Cycle);
    CycleOffset = temp->Size;
    CycleShift = temp->CycleShift;
    CycleBits = temp->CycleBits;
    break;
  case sCT_MASK:
    EditMask(cmd,(sInt *)data,name,temp->Zones,temp->Cycle);
    break;
  case sCT_STRING:
    EditString(cmd,(sChar *)data,name,temp->Size);
    break;
  case sCT_INT:
    EditInt(cmd,(sInt *)data,name);
    if(temp->Zones>1)
      InitNum(temp->Min,temp->Max,temp->Step,temp->Default,temp->Zones);
    else
      InitNum(temp->Min[0],temp->Max[0],temp->Step,temp->Default[0]);
    break;
  case sCT_FLOAT:
    EditFloat(cmd,(sF32 *)data,name);
    if(temp->Zones>1)
      InitNum(temp->Min,temp->Max,temp->Step,temp->Default,temp->Zones);
    else
      InitNum(temp->Min[0],temp->Max[0],temp->Step,temp->Default[0]);
    break;
  case sCT_FIXED:
    EditFixed(cmd,(sInt *)data,name);
    if(temp->Zones>1)
      InitNum(temp->Min,temp->Max,temp->Step,temp->Default,temp->Zones);
    else
      InitNum(temp->Min[0],temp->Max[0],temp->Step,temp->Default[0]);
    break;
  case sCT_HEX:
    EditHex(cmd,(sU32 *)data,name);
    if(temp->Zones>1)
      InitNum(temp->Min,temp->Max,temp->Step,temp->Default,temp->Zones);
    else
      InitNum(temp->Min[0],temp->Max[0],temp->Step,temp->Default[0]);
    break;
  case sCT_RGB:
    EditRGB(cmd,(sInt *)data,name);
    for(i=0;i<3;i++)
    {
      Min[i] = temp->Min[i];
      Max[i] = temp->Max[i];
      Default[i] = temp->Default[i];
    }
    break;
  case sCT_RGBA:
    EditRGBA(cmd,(sInt *)data,name);
    for(i=0;i<4;i++)
    {
      Min[i] = temp->Min[i];
      Max[i] = temp->Max[i];
      Default[i] = temp->Default[i];
    }
    break;
  case sCT_URGB:
    EditURGB(cmd,(sInt *)data,name);
    for(i=0;i<3;i++)
    {
      Min[i] = temp->Min[i];
      Max[i] = temp->Max[i];
      Default[i] = temp->Default[i];
    }
    break;
  case sCT_URGBA:
    EditURGBA(cmd,(sInt *)data,name);
    for(i=0;i<4;i++)
    {
      Min[i] = temp->Min[i];
      Max[i] = temp->Max[i];
      Default[i] = temp->Default[i];
    }
    break;
  }
//  if(Zones>1)
//    Style |= sCS_SIDELABEL;
}


void sControl::Button(sChar *label,sU32 cmd)
{
  sCopyString(NumEditBuf,label,sizeof(NumEditBuf));
  Init(sCT_BUTTON,sCS_FATBORDER|sCS_BORDERSEL,NumEditBuf,cmd,0);
  Flags |= sGWF_SQUEEZE;
}

void sControl::Menu(sChar *label,sU32 cmd,sU32 shortcut)
{
  sCopyString(NumEditBuf,label,sizeof(NumEditBuf));
  Init(sCT_BUTTON,sCS_NOBORDER|sCS_HOVER|sCS_LEFT,NumEditBuf,cmd,0);
  Shortcut = shortcut;
  Flags |= sGWF_SQUEEZE;
}

void sControl::MenuCheck(sChar *label,sU32 cmd,sU32 shortcut,sInt state)
{
  Menu(label,cmd,shortcut);
  Type = sCT_CHECKMARK;
  State = state;
}

void sControl::Label(sChar *label)
{
  Init(sCT_BUTTON,sCS_NOBORDER,label,0,0);
  Flags |= sGWF_SQUEEZE;
}

void sControl::EditBool(sU32 cmd,sInt *ptr,sChar *label)
{
  Init(sCT_TOGGLE,sCS_FATBORDER|sCS_BORDERSEL,label,cmd,ptr);
  Flags |= sGWF_SQUEEZE;
}

void sControl::EditCycle(sU32 cmd,sInt *ptr,sChar *label,sChar *choices)
{
  Init(sCT_CYCLE,sCS_FATBORDER|sCS_BORDERSEL|sCS_LEFT,label,cmd,ptr);
  InitCycle(choices);
  Flags |= sGWF_SQUEEZE;
}

void sControl::EditChoice(sU32 cmd,sInt *ptr,sChar *label,sChar *choices)
{
  Init(sCT_CHOICE,sCS_FATBORDER|sCS_BORDERSEL|sCS_LEFT,label,cmd,ptr);
  InitCycle(choices);
  Flags |= sGWF_SQUEEZE;
}

void sControl::EditMask(sU32 cmd,sInt *ptr,sChar *label,sInt max,sChar *choices)
{
  Init(sCT_MASK,sCS_ZONES|sCS_EDITNUM,label,cmd,ptr);
  InitNum(0,0,0,0);
  Zones = max;
  Cycle = choices;

  Flags |= sGWF_SQUEEZE;
}

void sControl::EditString(sU32 cmd,sChar *edit,sChar *label,sInt size)
{
  Init(sCT_STRING,sCS_LEFT|sCS_EDIT,label,cmd,0);
  Edit = edit;
  EditSize = size;
  EditMode = 1;
  Flags |= sGWF_SQUEEZE;
}

void sControl::EditInt(sU32 cmd,sInt *ptr,sChar *label)
{
  Init(sCT_INT,sCS_LEFT|sCS_EDITNUM,label,cmd,ptr);
  InitNum(0,1,0.25,0);
  Flags |= sGWF_SQUEEZE;
}

void sControl::EditFloat(sU32 cmd,sF32 *ptr,sChar *label)
{
  Init(sCT_FLOAT,sCS_LEFT|sCS_EDITNUM,label,cmd,ptr);
  InitNum(0,1,0.25,0);
  Flags |= sGWF_SQUEEZE;
}

void sControl::EditHex(sU32 cmd,sU32 *ptr,sChar *label)
{
  Init(sCT_HEX,sCS_LEFT|sCS_EDITNUM,label,cmd,ptr);
  InitNum(0,1,0.25,0);
  Flags |= sGWF_SQUEEZE;
}

void sControl::EditFixed(sU32 cmd,sInt *ptr,sChar *label)
{
  Init(sCT_FIXED,sCS_LEFT|sCS_EDITNUM,label,cmd,ptr);
  InitNum(0,1,0.25,0);
  Flags |= sGWF_SQUEEZE;
}

void sControl::EditRGB(sU32 cmd,sInt *ptr,sChar *label)
{
  Init(sCT_RGB,sCS_LEFT|sCS_EDITNUM|sCS_EDITCOL,label,cmd,ptr);
  InitNum(0,0xff00,0x100,0xff00);
  Zones = 3;
  Swizzle[0] = 0;
  Swizzle[1] = 1;
  Swizzle[2] = 2;
  Swizzle[3] = 3;
  Flags |= sGWF_SQUEEZE;
  Style |= sCS_ZONES;
}

void sControl::EditRGBA(sU32 cmd,sInt *ptr,sChar *label)
{
  Init(sCT_RGBA,sCS_LEFT|sCS_EDITNUM|sCS_EDITCOL,label,cmd,ptr);
  InitNum(0,0xff00,0x100,0xff00);
  Zones = 4;
  Swizzle[0] = 3;
  Swizzle[1] = 0;
  Swizzle[2] = 1;
  Swizzle[3] = 2;
  Flags |= sGWF_SQUEEZE;
  Style |= sCS_ZONES;
}


void sControl::EditURGB(sU32 cmd,sInt *ptr,sChar *label)
{
  Init(sCT_URGB,sCS_LEFT|sCS_EDITNUM|sCS_EDITCOL,label,cmd,ptr);
  InitNum(0,0xff,0x1,0xff);
  Zones = 3;
  Swizzle[0] = 2;
  Swizzle[1] = 1;
  Swizzle[2] = 0;
  Swizzle[3] = 3;
  Flags |= sGWF_SQUEEZE;
  Style |= sCS_ZONES;
}

void sControl::EditURGBA(sU32 cmd,sInt *ptr,sChar *label)
{
  Init(sCT_URGBA,sCS_LEFT|sCS_EDITNUM|sCS_EDITCOL,label,cmd,ptr);
  InitNum(0,0xff,0x1,0xff);
  Zones = 4;
  Swizzle[0] = 3;
  Swizzle[1] = 2;
  Swizzle[2] = 1;
  Swizzle[3] = 0;
  Flags |= sGWF_SQUEEZE;
  Style |= sCS_ZONES;
}

/****************************************************************************/
/****************************************************************************/

sListControl::sListControl()
{
  List.Init(16);
  LastSelected = -1;
  Height = sPainter->GetHeight(sGui->PropFont)+2;
  Flags = 0;
  LeftCmd = 0;
  RightCmd = 0;
  DoubleCmd = 0;
}

sListControl::~sListControl()
{
  List.Exit();
}

void sListControl::OnCalcSize()
{
  Height = sPainter->GetHeight(sGui->PropFont)+2;
  SizeX = 75;
  SizeY = Height * List.Count;
}

void sListControl::OnPaint()
{
  sInt i;
  sInt y;
  sRect r;
  sListControlEntry *e;

  r = Client;

  y = Client.y0;
  for(i=0;i<List.Count;i++)
  {
    r.y0 = y;
    y+=Height;
    r.y1 = y;
    e = &List[i];
    sPainter->Paint(sGui->FlatMat,r,sGui->Palette[e->Select ? sGC_SELBACK : sGC_BACK]);
    sPainter->PrintC(sGui->PropFont,r,sFA_LEFT|sFA_SPACE,e->Name,sGui->Palette[e->Select ? sGC_SELECT : sGC_TEXT]);
  }
  r.y0 = y;
  r.y1 = Client.y1;
  if(r.y0<r.y1)
    sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_BACK]);
}

void sListControl::OnDrag(sDragData &dd)
{
  sInt nr;
  sU32 qual;

  if(MMBScrolling(dd,DragStartX,DragStartY)) return ;

  switch(dd.Mode)
  {
  case sDD_START:
    nr = (dd.MouseY-Client.y0)/Height;
    if(nr>=0 && nr<List.Count)
    {
      if(dd.Buttons&1)
      {
        qual = sSystem->GetKeyboardShiftState();
        if(qual & sKEYQ_CTRL)
        {
          SetSelect(nr,!GetSelect(nr));
        }
        else
        {
          ClearSelect();
          SetSelect(nr,1);
        }
        Post(LeftCmd);
      }
      if(dd.Buttons&2)
      {
        SetSelect(nr,1);
        Post(RightCmd);
      }
    }
    else
    {
      if(dd.Buttons&2)
        Post(RightCmd);
    }
    break;
  }
}

/****************************************************************************/

void sListControl::Set(sInt nr,sChar *name,sInt cmd)
{
  List.AtLeast(nr+1);
  List.Count = nr+1;
  List[nr].Name = name;
  List[nr].Cmd = cmd;
  List[nr].Select = 0;
}

void sListControl::Add(sChar *name,sInt cmd)
{
  Set(List.Count,name,cmd);
}

void sListControl::Rem(sInt nr)
{
  sInt i;
 
  if(nr>=0 && nr<List.Count)
  {
    for(i=nr+1;i<List.Count;i++)
      List[i-1] = List[i];
    List.Count--;
  }
}

void sListControl::ClearList()
{
  List.Count = 0;
}

sChar *sListControl::GetName(sInt nr)
{
  return List[nr].Name;
}

sInt sListControl::GetCmd(sInt nr)
{
  return List[nr].Cmd;
}

sBool sListControl::GetSelect(sInt nr)
{
  return List[nr].Select;
}

sInt sListControl::GetCount()
{
  return List.Count;
}

sInt sListControl::GetSelect()
{
  sInt i;
  if(LastSelected!=-1)
    return LastSelected;
  for(i=0;i<List.Count;i++)
  {
    if(GetSelect(i))
      return i;
  }
  return -1;
}

void sListControl::ClearSelect()
{
  sInt i;

  for(i=0;i<List.Count;i++)
    List[i].Select = 0;
  LastSelected = -1;
}

void sListControl::SetSelect(sInt nr,sBool sel)
{
  sel = sel ? sTRUE : sFALSE;
  if(GetSelect(nr)!=sel)
  {
    if(Flags & sLCF_MULTISELECT)
    {
      if(LastSelected==nr && !sel)
        LastSelected = -1;
    }
    else
    {
      ClearSelect();
    }
    List[nr].Select = sel;
    if(sel)
      LastSelected = nr;
  }
}

sInt sListControl::FindName(sChar *name)
{
  sInt i;

  for(i=0;i<List.Count;i++)
  {
    if(sCmpString(GetName(i),name)==0)
      return i;
  }

  return -1;
}


/****************************************************************************/

