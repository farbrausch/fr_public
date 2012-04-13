// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_gui.hpp"
#include "material11.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   The Cursors                                                        ***/
/***                                                                      ***/
/****************************************************************************/

extern "C" sU32 Skin05[];
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
  ViewportValid = sFALSE;
  StartGCFlag = sFALSE;
  LastGCCount = 0;
  OnWindow = 0;
  CurrentRoot = 0;

  bm = new sBitmap;
  bm->Init(32,32);
  for(i=0;i<32*32;i++)
    bm->Data[i] = curcol[cursor[i]&0x0f];
  CursorMat = sPainter->LoadBitmapAlpha(bm);

  bm = new sBitmap;
  bm->Init(128,128);
  for(sInt y=0;y<128;y++)
  {
    sU32 *src = ((sU32*)(((sU8*)Skin05)+0x12))+(127-y)*128;
    for(sInt x=0;x<128;x++)
    {
      sU32 val = src[x];
//      val = ((val&0xff000000)>>24) | ((val&0x00ff0000)>>8) | ((val&0x0000ff00)>>8) | ((val&0x000000ff)<<24);
      bm->Data[y*128+x] = val;
    }
  }

  FlatMat = sPainter->LoadMaterial(new sSimpleMaterial(sINVALID,0,0));

  SkinMat = sPainter->LoadBitmapAlpha(bm);

  AlphaMat = sPainter->LoadMaterial(new sSimpleMaterial(sINVALID,sMBF_BLENDALPHA,0,0));
  AddMat = sPainter->LoadMaterial(new sSimpleMaterial(sINVALID,sMBF_BLENDADD,0,0));

  PropFonts[0]  = sPainter->LoadFont("Arial",12,0,0);
  FixedFonts[0] = sPainter->LoadFont("Courier New",12,0,0);
  FatFonts[0]   = sPainter->LoadFont("Arial",12,0,sHFS_BOLD);
  PropFonts[1]  = sPainter->LoadFont("Tahoma",10,0,sHFS_PIXEL);
  FixedFonts[1] = sPainter->LoadFont("Courier New",11,0,sHFS_PIXEL);
  FatFonts[1]   = sPainter->LoadFont("Tahoma",10,0,sHFS_PIXEL|sHFS_BOLD);

  Palette[sGC_NONE]    = 0xff205080;
  Palette[sGC_BACK]    = 0xff808080+0;
  Palette[sGC_TEXT]    = 0xff101010;
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
      Viewport.Screen = i;
      ViewportValid = sFALSE;

      sSystem->Clear(sVCF_ALL,Palette[0]);

      r = Root[i]->Position;
      PaintR(Root[i],r,0,0);

      if(ViewportValid)
      {
        if(i==CurrentRoot && sSystem->GetFullscreen())
        {
          sPainter->Paint(CursorMat,MouseX,MouseY,0xffffffff);
          sPainter->Flush();
        }
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
  if((win->Flags & sGWF_FLUSH) && ViewportValid)
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
    if(win->Flags & sGWF_PAINT3D)
    {
      sPainter->Flush();
      ViewportValid = sFALSE;

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
//    else
    {
      sPainter->SetClipping(CurrentClip);
      sPainter->EnableClipping(1);
      if(!ViewportValid)
      {
        sSystem->SetViewport(Viewport);
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
  sGuiWindow *win;
  sInt step = 32;

  sZONE(GuiKey);
  switch(key & 0x8001ffff)
  {
  case sKEY_MODECHANGE:
    for(i=0;i<sGUI_MAXROOTS;i++)
    {
      if(Root[i])
      {
        sSystem->GetScreenInfo(i,si);
        Root[i]->Position.Init(0,0,si.XSize,si.YSize);
        Root[i]->Flags |= sGWF_LAYOUT;
      }
    }
    break;
  case sKEY_WHEELUP:
    step = -step;
  case sKEY_WHEELDOWN:
    win = Focus;
    while(win && win->Parent && !(win->Flags & sGWF_SCROLLY))
      win = win->Parent;
    if(win)
      win->ScrollY = sRange<sInt>(win->ScrollY+step,win->SizeY-win->RealY,0);
    /* no break; */

  default:
    if(KeyIndex>=256)
      OnFrame();
    KeyBuffer[KeyIndex++] = key;
    break;
  }
}

/****************************************************************************/

void sGuiManager::SendDrag(sDragData &dd)
{
  sGuiWindow *win;

  win = Focus;
  while(win->Parent && (win->Flags&sGWF_PASSLMB) && (dd.Buttons&1))
    win = win->Parent;
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
  static OldMouseX,OldMouseY;
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
  dd.Flags = 0;
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
    dd.Flags = 0;
    dd.Mode = sDD_START;
    time = sSystem->GetTime();
    if(time-MouseLastTime<250)
      dd.Flags |= sDDF_DOUBLE;
    MouseLastTime = time;
    if((dd.Flags&sDDF_DOUBLE) && dd.Mode==sDD_START && dd.Buttons==4 &&  KeyIndex<256)
      KeyBuffer[KeyIndex++] = sKEY_APPPOPUP;
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
  sGuiWindow *p,*q;

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
    if((p->Flags&sGWF_AUTOKILL) && !(p->Flags&sGWF_CHILDFOCUS) && !p->Position.Hit(MouseX,MouseY))
    {
      q = p;
      p = p->Next;
      win->RemChild(q);
      win->Flags |= sGWF_LAYOUT;
    }
    else
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

  if(mask & sGS_SKIN05)
  {
//    sGui->Palette[sGC_BACK]    = 0xffff0000;
//    sGui->Palette[sGC_TEXT]    = 0xffff0000;
//    sGui->Palette[sGC_DRAW]    = 0xffff0000;
//    sGui->Palette[sGC_HIGH]    = 0xffff0000;
//    sGui->Palette[sGC_LOW]     = 0xffff0000;
//    sGui->Palette[sGC_HIGH2]   = 0xffff0000;
//    sGui->Palette[sGC_LOW2]    = 0xffff0000;
//    sGui->Palette[sGC_SELECT]  = 0xffff0000;
//    sGui->Palette[sGC_SELBACK] = 0xffff0000;
//    sGui->Palette[sGC_BARBACK] = 0xffff0000;

//    sGui->Palette[sGC_BUTTON]  = 0xff737373;
    sGui->Palette[sGC_HIGH]    = 0xffd3d3d3;
    sGui->Palette[sGC_LOW]     = 0xff434343;
    sGui->Palette[sGC_SELECT]  = 0xffffffff;
    sGui->Palette[sGC_SELBACK] = 0xffa8a8a8;
    sGui->Palette[sGC_BARBACK] = 0xff6a6a6a;

  }

  GuiStyle = mask;
  for(i=0;i<sGUI_MAXROOTS;i++)
    if(Root[i])
      Root[i]->Flags |= sGWF_LAYOUT;

  i = (GuiStyle & sGS_SMALLFONTS) ? 1 : 0;
  PropFont  = PropFonts[i];
  FixedFont = FixedFonts[i];
  FatFont   = FatFonts[i];
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

void sGuiManager::AddWindow(sGuiWindow *win,sInt x,sInt y,sInt screen,sInt center)
{
  sInt dx,dy;

  if(screen == -1)
    screen=CurrentRoot;
  win->Flags |= sGWF_LAYOUT|sGWF_UPDATE;
  Root[screen]->AddChild(win);

  dx = x - win->Position.x0;
  dy = y - win->Position.y0;
  CalcSizeR(win);
  if(center)
  {
    dx -= win->Position.XSize()/2;
    dy -= win->Position.YSize()/2;
  }

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

void sGuiManager::Button(sRect r,sBool down,const sChar *text,sInt align,sU32 col)
{
  if(col==0)
    col = sGui->Palette[sGC_BUTTON];
  Bevel(r,down);
  sPainter->Paint(sGui->FlatMat,r,col);
  if(text)
    sPainter->PrintC(sGui->PropFont,r,align,text,sGui->Palette[sGC_TEXT]);
}


void sGuiManager::CheckBox(sRect r,sBool checked,sU32 col1)
{
  sU32 col0;

  col0 = Palette[sGC_BACK];
  if(col1==0)
    col1 = Palette[sGC_DRAW];
  RectHL(r,1,col1,col1);
  sPainter->Paint(FlatMat,r,col0);
  if(checked)
  {
    sPainter->Line(r.x0,r.y0,r.x1,r.y1,col1);
    sPainter->Line(r.x0,r.y1,r.x1,r.y0,col1);
  }
}

void sGuiManager::Group(sRect r,const sChar *label)
{
  sInt xs,ys;
  sInt w;

//  sPainter->Paint(FlatMat,r,Palette[sGC_BUTTON]);
  xs = r.XSize();
  ys = r.YSize();
  if(label)
  {
    w = sPainter->GetWidth(PropFont,label)+8;
    sPainter->PrintC(PropFont,r,0,label,Palette[sGC_TEXT]);
    sPainter->Paint(FlatMat,r.x0+2           ,r.y0+ys/2-1,xs/2-w/2,1,Palette[sGC_LOW]);
    sPainter->Paint(FlatMat,r.x0+2           ,r.y0+ys/2  ,xs/2-w/2,1,Palette[sGC_HIGH]);
    sPainter->Paint(FlatMat,r.x1-2-(xs/2-w/2),r.y0+ys/2-1,xs/2-w/2,1,Palette[sGC_LOW]);
    sPainter->Paint(FlatMat,r.x1-2-(xs/2-w/2),r.y0+ys/2  ,xs/2-w/2,1,Palette[sGC_HIGH]);
  }
  else
  {
    sPainter->Paint(FlatMat,r.x0+2,r.y0+ys/2-1,xs-4,1,Palette[sGC_LOW]);
    sPainter->Paint(FlatMat,r.x0+2,r.y0+ys/2  ,xs-4,1,Palette[sGC_HIGH]);
  }
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
  if(Parent)
  {
    Parent->Flags |= sGWF_LAYOUT;
    Parent->RemChild(this);
    Parent = 0;
  }
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

void sGuiWindow::ScrollTo(sInt x,sInt y)
{
  ScrollX = x;
  if(ScrollX > SizeX-RealX)
    ScrollX = SizeX-RealX;
  if(ScrollX < 0)
    ScrollX = 0;

  ScrollY = y;
  if(ScrollY > SizeY-RealY)
    ScrollY = SizeY-RealY;
  if(ScrollY < 0)
    ScrollY = 0;
}

void sGuiWindow::ScrollTo(sRect vis,sInt mode)
{
  sInt c,s;
  vis.x0 -= Client.x0;
  vis.y0 -= Client.y0;
  vis.x1 -= Client.x0;
  vis.y1 -= Client.y0;
  if(mode==sGWS_SAFE)
  {
    if(Client.XSize()>75)
    {
      vis.x0-=25;
      vis.x1+=25;
    }
    if(Client.YSize()>75)
    {
      vis.y0-=25;
      vis.y1+=25;
    }
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

void sGuiWindow::AddTitle(sChar *title,sU32 flags,sU32 closecmd)
{
  sSizeBorder *sb;

  sb = new sSizeBorder;
  sb->Title = title;
  sb->DontResize = flags;
  sb->CloseCmd = closecmd;
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
  static sInt ctrl;
  if(dd.Buttons==4)
  {
    switch(dd.Mode)
    {
    case sDD_START:
      sx = ScrollX+dd.MouseX;
      sy = ScrollY+dd.MouseY;
      ctrl = (sSystem->GetKeyboardShiftState()&sKEYQ_CTRL)?4:1;
      break;
    case sDD_DRAG:
      ScrollX = sRange<sInt>((sx-dd.MouseX)*ctrl,SizeX-RealX,0);
      ScrollY = sRange<sInt>((sy-dd.MouseY)*ctrl,SizeY-RealY,0);
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
/*
void sGuiWindow::WheelScrolling(sU32 key,sInt step=0)
{
  if(step==0) 
    step = 12;
  switch(key&0x8001ffff)
  {
  case sKEY_WHEELUP:
    ScrollY = sRange<sInt>(ScrollY-step,SizeX-RealX,0);
    break;
  case sKEY_WHEELDOWN:
    ScrollY = sRange<sInt>(ScrollY+step,SizeX-RealX,0);
    break;
  }
}
*/
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

void sGuiWindow::SetFullsize(sBool b)
{
  sInt i,j;
  sGuiWindow *w;

  if(!Parent) return;
  i = -1;
  if(b)
  {
    j = 0;
    w = Parent->Childs;
    while(w)
    {
      if(w==this)
      {
        i = j;
        break;
      }
      w = w->Next;
      j++;
    }
  }

  switch(Parent->GetClass())
  {
  case sCID_VSPLITFRAME:
    ((sVSplitFrame*)(Parent))->Fullscreen = i;
    Parent->Flags |= sGWF_LAYOUT;
    break;

  case sCID_HSPLITFRAME:
    ((sHSplitFrame*)(Parent))->Fullscreen = i;
    Parent->Flags |= sGWF_LAYOUT;
    break;
  }

  Parent->SetFullsize(b);
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

// siplified drawing

sU32 sGuiWindow::Pal(sU32 col) { return (col<0x7f) ? sGui->Palette[col] : col; }

void sGuiWindow::Paint(sInt x,sInt y,sInt xs,sInt ys,sU32 col)
{ sPainter->Paint(sGui->FlatMat,x,y,xs,ys,Pal(col)); }
void sGuiWindow::Paint(const sRect &r,sU32 col)
{ sPainter->Paint(sGui->FlatMat,r,Pal(col)); }
void sGuiWindow::Paint(const sRect &pos,const sFRect &uv,sU32 col)
{ sPainter->Paint(sGui->FlatMat,pos,uv,Pal(col)); }

void sGuiWindow::PaintAlpha(sInt x,sInt y,sInt xs,sInt ys,sU32 col)
{ sPainter->Paint(sGui->AlphaMat,x,y,xs,ys,Pal(col)); }
void sGuiWindow::PaintAlpha(const sRect &r,sU32 col)
{ sPainter->Paint(sGui->AlphaMat,r,Pal(col)); }
void sGuiWindow::PaintAlpha(const sRect &pos,const sFRect &uv,sU32 col)
{ sPainter->Paint(sGui->AlphaMat,pos,uv,Pal(col)); }

void sGuiWindow::Print(sInt x,sInt y,const sChar *text,sU32 col,sInt len)
{ sPainter->Print(sGui->PropFont,x,y,text,Pal(col),len); }
sInt sGuiWindow::Print(sRect &r,sInt align,const sChar *text,sU32 col,sInt len)
{ return sPainter->PrintC(sGui->PropFont,r,align,text,Pal(col),len); }
sInt sGuiWindow::PrintWidth(const sChar *text,sInt len)
{ return sPainter->GetWidth(sGui->PropFont,text,len); }
sInt sGuiWindow::PrintHeight()
{ return sPainter->GetHeight(sGui->PropFont); }

void sGuiWindow::PrintFixed(sInt x,sInt y,const sChar *text,sU32 col,sInt len)
{ sPainter->Print(sGui->FixedFont,x,y,text,Pal(col),len); }
sInt sGuiWindow::PrintFixed(sRect &r,sInt align,const sChar *text,sU32 col,sInt len)
{ return sPainter->PrintC(sGui->FixedFont,r,align,text,Pal(col),len); }
sInt sGuiWindow::PrintFixedWidth(const sChar *text,sInt len)
{ return sPainter->GetWidth(sGui->FixedFont,text,len); }
sInt sGuiWindow::PrintFixedHeight()
{ return sPainter->GetHeight(sGui->FixedFont); }

void sGuiWindow::PaintBevel(sRect &r,sBool down)
{ sGui->Bevel(r,down); }
void sGuiWindow::PaintRectHL(sRect &r,sInt w,sU32 colh,sU32 coll)
{ sGui->RectHL(r,w,Pal(colh),Pal(coll)); }
void sGuiWindow::PaintButton(sRect r,sBool down,const sChar *text,sInt align,sU32 col)
{ sGui->Button(r,down,text,align,Pal(col)); }
void sGuiWindow::PaintCheckBox(sRect r,sBool checked,sU32 col)
{ sGui->CheckBox(r,checked,col); }
void sGuiWindow::PaintGroup(sRect r,const sChar *label)
{ sGui->Group(r,label); }

/****************************************************************************/

void sGuiWindow::PaintGH(const sRect &pos,sU32 col0,sU32 col1)
{
  sGuiPainterVert v[4];

  v[0].Init(pos.x0,pos.y0,col0,0,0);
  v[1].Init(pos.x1,pos.y0,col0,0,0);
  v[2].Init(pos.x1,pos.y1,col1,0,0);
  v[3].Init(pos.x0,pos.y1,col1,0,0);

  sPainter->Paint(sGui->FlatMat,v,1);
}
void sGuiWindow::PaintGV(const sRect &pos,sU32 col0,sU32 col1)
{
  sGuiPainterVert v[4];

  v[0].Init(pos.x0,pos.y0,col0,0,0);
  v[1].Init(pos.x1,pos.y0,col1,0,0);
  v[2].Init(pos.x1,pos.y1,col1,0,0);
  v[3].Init(pos.x0,pos.y1,col0,0,0);

  sPainter->Paint(sGui->FlatMat,v,1);
}

// new skinning support

void sGuiWindow::ClearBack(sInt colindex)
{
  if(sGui->GetStyle()&sGS_SKIN05)
  {
    if(colindex==sGC_BUTTON)
      PaintGH(Client,0xffa3a3a3,0xff6b6b6b);
    else
      PaintGH(Client,0xff787878,0xff9b9b9b);
  }
  else
  {
    Paint(Client,colindex);
  }
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

  ClearBack();

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
  ClearBack();

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

void sDialogWindow::OnKey(sU32 key)
{
  switch(key)
  {
  case sKEY_ESCAPE:
    OnCommand(2);
    break;
  case sKEY_ENTER:
    OnCommand(1);
    break;
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
    CmdOk = 0;
    CmdCancel = 0;
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
    CmdOk = 0;
    CmdCancel = 0;
    if(Parent)
      Parent->RemChild(this);
    break;
  }

  return sTRUE;
}

void sDialogWindow::SetFocus()
{
  if(EditControl)
    sGui->SetFocus(EditControl);
  else
    sGui->SetFocus(this);
//  else if(OkButton)
//    sGui->SetFocus(OkButton);
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
  SetFocus();
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
  EditControl->EnterCmd = 1;
  EditControl->Cursor = sGetStringLen(buffer);
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
  /*sPainter->Paint(sGui->AlphaMat,Client,sGui->Palette[0]);
  sPainter->Flush();*/
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
    {
      sSwap(*p,(*p)->Next);
    }

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
  SortStart = 0;
  ExitKey = 0;
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

sBool sMenuFrame::OnShortcut(sU32 key)
{
  if(key&sKEYQ_SHIFT) key|= sKEYQ_SHIFT;
  if(key&sKEYQ_CTRL) key|= sKEYQ_CTRL;
  key = key&0x8001ffff;
  if(key==sKEY_ESCAPE || (ExitKey && key==ExitKey))
  {
    if(Parent)
    {
      Parent->Flags |= sGWF_LAYOUT;
      Parent->RemChild(this);
    }
    if(SendTo)
      sGui->SetFocus(SendTo);
    return sTRUE;
  }
  else
    return sFALSE;
}

sControl *sMenuFrame::AddMenu(const sChar *name,sU32 cmd,sU32 shortcut)
{
  sControl *con;

  con = new sControl; 
  con->Menu(name,cmd,shortcut); 
  AddChild(con);

  return con;
}

sControl *sMenuFrame::AddMenuSort(const sChar *name,sU32 cmd,sU32 shortcut)
{
  sControl *con;
  sControl *con0;
  sInt i;

  con = new sControl; 
  con->Menu(name,cmd,shortcut); 
  for(i=SortStart;i<GetChildCount();i++)
  {
    con0 = (sControl *) GetChild(i);
    sVERIFY(con0->GetClass()==sCID_CONTROL);
    if(sCmpStringI(con0->GetEditBuffer(),name)>0)
      break;
  }
  AddChild(i,con);

  return con;
}

sControl *sMenuFrame::AddCheck(const sChar *name,sU32 cmd,sU32 shortcut,sInt state)
{
  sControl *con;

  con = new sControl; 
  con->MenuCheck(name,cmd,shortcut,state); 
  AddChild(con);
  SortStart = GetChildCount();

  return con;
}

void sMenuFrame::AddSpacer()
{
  sGuiWindow *con;

  con = new sMenuSpacerControl; 
  SortStart = GetChildCount();
  AddChild(con);
}

void sMenuFrame::AddColumn()
{
  if(ColumnCount<16)
  {
    ColumnIndex[ColumnCount] = GetChildCount()-1;
    ColumnWidth[ColumnCount] = 0;
    ColumnCount++;
    SortStart = GetChildCount();
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
  ClearBack(sGC_BUTTON);
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

void sGridFrame::AddLabel(const sChar *name,sInt x0,sInt x1,sInt y)
{
  sControl *con;

  con = new sControl;
  con->Label(name);
  con->Style = sCS_LEFT|sCS_NOBORDER|sCS_DONTCLEARBACK;
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
  Fullscreen = -1;
}

void sVSplitFrame::OnCalcSize()
{
  sGuiWindow *win;

  sInt w = (sGui->GetStyle() & sGS_SKIN05) ? 1 : Width;

  SizeX = -w;
  SizeY = 0;
  win = Childs;
  sVERIFY(win);
  while(win)
  {
    SizeX += w+win->SizeX;
    SizeY = sMax(SizeY,win->SizeY);
    win = win->Next;
  }
}

void sVSplitFrame::OnLayout()
{
  sGuiWindow *win;
  sInt i,max,x;
  sRect r;

  sInt w = (sGui->GetStyle() & sGS_SKIN05) ? 1 : Width;
  sInt safe =  (sGui->GetStyle() & sGS_SKIN05) ? 8 : 0;

  r = Client;
  max = GetChildCount();

  if(Fullscreen>=0 && Fullscreen<max)
  {
    win = Childs;
    i = 0;
    while(win)
    {
      if(i==Fullscreen)
        win->Flags &= ~sGWF_DISABLED;
      else
        win->Flags |= sGWF_DISABLED;
      win->Position = Client;
      win = win->Next;
      i++;
    }
    return;
  }

  if(max!=Count)
  {
    Count = max;

    win = Childs;
    i = 0;
    Pos[0] = -w;
    while(win)
    {
      if(Pos[i+1]>0)
        Pos[i+1] = Pos[i+1];
      else if(Pos[i+1]<0)
        Pos[i+1] = ((r.x1-r.x0)*-Pos[i+1])>>16;
      else
        Pos[i+1] = (r.x1-r.x0-w*(max-1))*(i+1)/(max) + w*i;
      i++;
      win = win->Next;
    }
    Pos[i] = r.x1-r.x0;
  }
  else
  {
    Pos[0] = -w;
    Pos[Count] = r.x1-r.x0;
  }

  for(i=1;i<Count;i++)
  {
    if(Pos[i]<Pos[i-1]+safe)
      Pos[i] = Pos[i-1]+safe;
    if(Pos[i]>Pos[i+1]-safe)
      Pos[i] = Pos[i+1]-safe;
  }

  win = Childs;
  i = 0;
  while(win)
  {
    win->Flags &= ~sGWF_DISABLED;
    if(i<Count-1)
    {
      x = r.x1-r.x0-(Count-i-1)*w;
      if(Pos[i+1]>x)
        Pos[i+1] = x;
    }

    win->Position.x0 = r.x0+Pos[i]+w;
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

  if(Fullscreen!=-1) return;

  if(sGui->GetStyle() & sGS_SKIN05)
  {
    sPainter->Paint(sGui->FlatMat,Client,sGui->Palette[sGC_DRAW]);
  }
  else
  {
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
}

void sVSplitFrame::OnDrag(sDragData &dd)
{
  sInt i;
  sInt w = (sGui->GetStyle() & sGS_SKIN05) ? 1 : Width;

  if(Fullscreen!=-1) return;

  switch(dd.Mode)
  {
  case sDD_START:
    DragMode = 0;
    for(i=1;i<Count;i++)
    {
      if(dd.MouseX>=Pos[i]+Client.x0 && dd.MouseX<Pos[i]+Client.x0+w)
      {
        DragMode = i;
        DragStart = Pos[DragMode]-dd.MouseX;
      }
    }
    break;
  case sDD_DRAG:
    if(DragMode>0 && DragMode<Count)
    {
      i = sRange(DragStart+dd.MouseX,Pos[DragMode+1]-w,Pos[DragMode-1]+w);
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
    if(pos > 0)
      Pos[pos] = (Pos[pos-1]+Pos[pos]) / 2;
    else
      Pos[pos+1] = (Pos[pos]+Pos[pos+2]) / 2;

    AddChild(pos,win);
  }
}

void sVSplitFrame::RemoveSplit(sGuiWindow *win)
{
  sInt i,j,max;

  max = GetChildCount();
  for(i=0;i<max;i++)
  {
    if(GetChild(i) == win)
    {
      for(j=i;j<max-1;j++)
        Pos[j] = Pos[j+1];

      RemChild(win);
      break;
    }
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
  Fullscreen = -1;
}

void sHSplitFrame::OnCalcSize()
{
  sGuiWindow *win;

  sInt w = (sGui->GetStyle() & sGS_SKIN05) ? 1 : Width;

  SizeY = -w;
  SizeX = 0;

  win = Childs;
  sVERIFY(win);
  while(win)
  {
    SizeY += w+win->SizeY;
    SizeX = sMax(SizeX,win->SizeX);
    win = win->Next;
  }

}

void sHSplitFrame::OnLayout()
{
  sGuiWindow *win;
  sInt i,max,y;
  sRect r;

  sInt w = (sGui->GetStyle() & sGS_SKIN05) ? 1 : Width;
  sInt safe =  (sGui->GetStyle() & sGS_SKIN05) ? 16 : 0;

  r = Client;
  max = GetChildCount();

  if(Fullscreen>=0 && Fullscreen<max)
  {
    win = Childs;
    i = 0;
    while(win)
    {
      if(i==Fullscreen)
        win->Flags &= ~sGWF_DISABLED;
      else
        win->Flags |= sGWF_DISABLED;
      win->Position = Client;
      win = win->Next;
      i++;
    }
    return;
  }

  if(max!=Count)
  {
    Count = max;

    win = Childs;
    i = 0;
    Pos[0] = -w;
    while(win)
    {
      if(Pos[i+1]>0)
        Pos[i+1] = Pos[i+1];
      else if(Pos[i+1]<0)
        Pos[i+1] = ((r.y1-r.y0)*-Pos[i+1])>>16;
      else
        Pos[i+1] = (r.y1-r.y0-w*(max-1))*(i+1)/(max) + w*i;
      i++;
      win = win->Next;
    }
    Pos[i] = r.y1-r.y0;
  }
  else
  {
    Pos[Count] = r.y1-r.y0;
    Pos[0] = -w;
  }

  win = Childs;

  for(i=1;i<Count;i++)
  {
    if(Pos[i]<Pos[i-1]+safe)
      Pos[i] = Pos[i-1]+safe;
    if(Pos[i]>Pos[i+1]-safe)
      Pos[i] = Pos[i+1]-safe;
  }

  i = 0;
  while(win)
  {
    win->Flags &= ~sGWF_DISABLED;
    if(i<Count-1)
    {
      y = r.y1-r.y0-(Count-i-1)*w;
      if(Pos[i+1]>y)
        Pos[i+1] = y;
    }

    win->Position.y0 = r.y0+Pos[i]+w;
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

  if(Fullscreen!=-1) return;

  if(sGui->GetStyle() & sGS_SKIN05)
  {
    sPainter->Paint(sGui->FlatMat,Client,sGui->Palette[sGC_DRAW]);
  }
  else
  {
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
}

void sHSplitFrame::OnDrag(sDragData &dd)
{
  sInt i;
  sInt w = (sGui->GetStyle() & sGS_SKIN05) ? 1 : Width;

  if(Fullscreen!=-1) return;

  switch(dd.Mode)
  {
  case sDD_START:
    DragMode = 0;
    for(i=1;i<Count;i++)
    {
      if(dd.MouseY>=Pos[i]+Client.y0 && dd.MouseY<Pos[i]+Client.y0+w)
      {
        DragMode = i;
        DragStart = Pos[DragMode]-dd.MouseY;
      }
    }
    break;
  case sDD_DRAG:
    if(DragMode>0 && DragMode<Count)
    {
      i = sRange(DragStart+dd.MouseY,Pos[DragMode+1]-w,Pos[DragMode-1]+w);
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

void sHSplitFrame::RemoveSplit(sGuiWindow *win)
{
  sInt i,j,max;

  max = GetChildCount();
  for(i=0;i<max;i++)
  {
    if(GetChild(i) == win)
    {
      for(j=i;j<max-1;j++)
        Pos[j] = Pos[j+1];

      RemChild(win);
      break;
    }
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

void sSwitchFrame::Set(sInt win)
{
  if(win>=0 && win<GetChildCount())
  {
    Current = win;
    Flags |= sGWF_LAYOUT;
  }
}

void sSwitchFrame::OnPaint()
{
}

/****************************************************************************/
/***                                                                      ***/
/***   borders                                                            ***/
/***                                                                      ***/
/****************************************************************************/

void sFocusBorder::OnCalcSize()
{
  if(!(sGui->GetStyle()&sGS_SMALLFONTS))
  {
    SizeX+=4;
    SizeY+=4;
  }
  else
  {
    SizeX+=2;
    SizeY+=2;
  }
}

void sFocusBorder::OnSubBorder()
{
  if(!(sGui->GetStyle()&sGS_SMALLFONTS))
  {
    Parent->Client.Extend(-2);
  }
  else
  {
    Parent->Client.Extend(-1);
  }
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
  CloseCmd = 0;
}

void sSizeBorder::OnCalcSize()
{
  sInt small;

  if(sGui->GetStyle()&sGS_SMALLFONTS)
    small = 1;
  else
    small = 2;

  SizeX = 0;
  SizeY = 0;
  if(!Maximised)
  {
    SizeX += 4+2*small;
    SizeY += 4+2*small;
  }
  if(Title && ((sGui->GetStyle()&sGS_MAXTITLE)||!Maximised))
  {
    SizeY += small+2+sPainter->GetHeight(sGui->PropFont);
  }
}

void sSizeBorder::OnSubBorder()
{
  sInt w;

  w = (sGui->GetStyle()&sGS_SMALLFONTS) ? 1 : 2;
  if(!Maximised)
    Parent->Client.Extend(-w*2);
  if(Title && ((sGui->GetStyle()&sGS_MAXTITLE)||!Maximised))
  {
    Parent->Client.y0 += 4+sPainter->GetHeight(sGui->PropFont);
    TitleRect = Client;
    if(!Maximised)
      TitleRect.Extend(-2-w);
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
    if(CloseCmd)
    {
      col0 = sGui->Palette[sGC_HIGH2];
      col1 = sGui->Palette[sGC_LOW2];
      if(DragClose==1)
        sSwap(col0,col1);
      r = CloseRect;
      sGui->RectHL(r,1,col0,col1);
      sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_BUTTON]);
      r = CloseRect;
      r.Extend(2);
      sPainter->PrintC(sGui->PropFont,r,0,"x",sGui->Palette[sGC_DRAW]);
    }
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
    if(Title && CloseCmd && CloseRect.Hit(dd.MouseX,dd.MouseY))
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
        if((dd.Flags & sDDF_DOUBLE) && !DontResize)
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
    if(DragClose && CloseCmd)
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
    if(DragClose==1 && CloseCmd)
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
  MenuStyle = 0;
  DragFakeMode = 0;
  DragMode = 0;
}

void sToolBorder::OnCalcSize()
{
  sInt x,y;
  sGuiWindow *p;

  x = 0;
  if((sGui->GetStyle() & sGS_SKIN05) && !MenuStyle)
    x += 4;

  y = 10;
  p = Childs;
  while(p)
  {
    x = x + p->SizeX;
    y = sMax(y,p->SizeY);
    p = p->Next;
  }

  Height = y+1;
  if(Height<15) Height = 15;
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
  if((sGui->GetStyle() & sGS_SKIN05) && !MenuStyle)
    x += 4;
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

void sToolBorder::OnDrag(sDragData &dd)
{
  sGuiWindow *win = Parent;
  sDragData df;

  if(MenuStyle)
  {
    sInt x,y;
    switch(dd.Mode)
    {
    case sDD_START:
      if((dd.Buttons&1) && dd.Flags&sDDF_DOUBLE)
      {
        sSystem->SetWinMode(sSystem->GetWinMode()==0 ? 1 : 0);
      }
      else
      {
        DragMode = 1;
        sSystem->GetWinMouseAbs(x,y);
        DragX = x;
        DragY = y;
      }
      break;
    case sDD_DRAG:
      if(DragMode)
      {
        sSystem->GetWinMouseAbs(x,y);
        sSystem->MoveWindow(x - DragX,y - DragY);
        DragX = x;
        DragY = y;
      }
      break;
    case sDD_STOP:
      DragMode = 0;
      break;
    }

  }
  else
  {
    if(dd.Mode == sDD_START)
    {
      if(dd.MouseX < Client.x0+5)
      {
        DragFakeX = Client.x0 - dd.MouseX - 1;
        DragFakeY = 0;
        DragFakeMode = sCID_VSPLITFRAME;
      }
      else if(dd.MouseX > Client.x1-5)
      {
        DragFakeX = Client.x1 - dd.MouseX ;
        DragFakeY = 0;
        DragFakeMode = sCID_VSPLITFRAME;
      }
      else
      {
        DragFakeX = 0;
        DragFakeY = Client.y0 - dd.MouseY - 1;
        DragFakeMode = sCID_HSPLITFRAME;
      }
    }

    if(DragFakeMode)
    {
      while(win)
      {
        if(win->GetClass()==DragFakeMode)
        {
          df = dd;
          df.MouseX += DragFakeX;
          df.MouseY += DragFakeY;
          win->OnDrag(df);
          break;
        }
        win = win->Parent;
      }
    }
  }

  if(dd.Mode == sDD_STOP)
    DragFakeMode = 0;
}

void sToolBorder::OnPaint()
{
  sRect r;

  r = Client;

  r.y1 = Client.y0+Height-1;
  if((sGui->GetStyle() & sGS_SKIN05) && !MenuStyle)
  {
    sFRect uv;
    uv.x0 = 4/128.0f;
    uv.x1 = 4/128.0f;
    uv.y0 = 16/128.0f;
    uv.y1 = 30/128.0f;
    sPainter->Paint(sGui->SkinMat,r,uv,~0);
    sPainter->Paint(sGui->AlphaMat,r,0x40ffffff);
    sPainter->Flush();

    sGuiWindow *w,*ww;


    ww = this;
    w = ww->Parent;
    while(w && w->GetClass()!=sCID_VSPLITFRAME && w->GetClass()!=sCID_HSPLITFRAME)
    {
      ww = w;
      w = w->Parent;
    }

    if(w->GetClass()==sCID_VSPLITFRAME)
    {
      sRect rr;
      if(w->GetChild(0)!=ww)
      {
        rr = r;  rr.x1 = rr.x0+4;
        sPainter->Paint(sGui->AlphaMat,rr,sGui->Palette[sGC_DRAW]);
      }
      if(w->GetChild(w->GetChildCount()-1)!=ww)
      {
        rr = r;  rr.x0 = rr.x1-4;
        sPainter->Paint(sGui->AlphaMat,rr,sGui->Palette[sGC_DRAW]);
      }
    }
  }
  else
  {
    if(sGui->GetStyle() & sGS_SKIN05)
    {
      sRect rr;

      rr = r; rr.y1 = rr.y0+1;
      sPainter->Paint(sGui->FlatMat,rr,sGui->Palette[sGC_HIGH]);
      rr = r; rr.y0 = rr.y1-1;
      sPainter->Paint(sGui->FlatMat,rr,sGui->Palette[sGC_LOW]);
      rr = r; rr.y0++; rr.y1--;
      sPainter->Paint(sGui->FlatMat,rr,sGui->Palette[sGC_BUTTON]);
    }
    else
    {
      sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_BUTTON]);
    }
  }

  r.y0 = Client.y0+Height-1;
  r.y1 = Client.y0+Height;
  sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_DRAW]);
}

sControl *sToolBorder::AddButton(const sChar *name,sU32 cmd)
{
  sControl *con;

  con = new sControl; 
  con->Button(name,cmd); 
  con->Style |= sCS_FAT;
  AddChild(con);

  return con;
}

sControl *sToolBorder::AddMenu(const sChar *name,sU32 cmd)
{
  sControl *con;

  con = new sControl; 
  con->Menu(name,cmd,0); 
  con->Style |= sCS_FAT;
  con->Style |= sCS_DONTCLEARBACK;
  AddChild(con);

  return con;
}

sControl *sToolBorder::AddLabel(const sChar *name)
{
  sControl *con;

  con = new sControl; 
  con->Label(name); 
  con->Style |= sCS_DONTCLEARBACK;
  con->Style |= sCS_FAT;
  con->Flags |= sGWF_PASSMB;
  AddChild(con);

  return con;
}

void sToolBorder::AddContextMenu(sU32 cmd)
{
  if(sGui->GetStyle() & sGS_SKIN05)
  {
    sImageButton *img;

    img = new sImageButton;
    img->InitContext();
    img->Cmd = cmd;
    AddChild(img);
  }
  else
  {
    sControl *con;

    con = new sControl; 
    con->Menu("V",cmd,0); 
    con->Style |= sCS_DONTCLEARBACK;
    AddChild(con);
  }
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
  sBool skin = (sGui->GetStyle()&sGS_SKIN05);

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

        if(skin)
        {
          sFRect uv;
          sRect r;
          uv.y0 = 0/128.0f;
          uv.y1 = 8/128.0f;
          if(DragMode==1) sSwap(uv.y0,uv.y1);

          uv.x0 = 0/128.0f;
          uv.x1 = 2/128.0f;
          r = KnopX;
          r.x1 = r.x0+2;
          sPainter->Paint(sGui->SkinMat,r,uv,~0);
          uv.x0 = 6/128.0f;
          uv.x1 = 8/128.0f;
          r = KnopX;
          r.x0 = r.x1-2;
          sPainter->Paint(sGui->SkinMat,r,uv,~0);
          uv.x0 = 4/128.0f;
          uv.x1 = 4/128.0f;
          r = KnopX;
          r.x0 += 2;
          r.x1 -= 2;
          sPainter->Paint(sGui->SkinMat,r,uv,~0);
        }
        else
        {
          sGui->Button(KnopX,DragMode==1,0);
        }

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
        EnableKnopY = 2;
        Parent->ScrollY = sRange<sInt>(Parent->ScrollY,Parent->SizeY-max,0);

        if(skin)
        {
          sFRect uv;
          sRect r;
          uv.x0 = 16/128.0f;
          uv.x1 = 24/128.0f;
          if(DragMode==2) sSwap(uv.x0,uv.x1);

          uv.y0 = 0/128.0f;
          uv.y1 = 2/128.0f;
          r = KnopY;
          r.y1 = r.y0+2;
          sPainter->Paint(sGui->SkinMat,r,uv,~0);
          uv.y0 = 6/128.0f;
          uv.y1 = 8/128.0f;
          r = KnopY;
          r.y0 = r.y1-2;
          sPainter->Paint(sGui->SkinMat,r,uv,~0);
          uv.y0 = 4/128.0f;
          uv.y1 = 4/128.0f;
          r = KnopY;
          r.y0 += 2;
          r.y1 -= 2;
          sPainter->Paint(sGui->SkinMat,r,uv,~0);
        }
        else
        {
          sGui->Button(KnopY,DragMode==2,0);
        }
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

void sTabBorder::SetTab(sInt nr,const sChar *name,sInt width)
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

void sStatusBorder::SetTab(sInt nr,const sChar *name,sInt width)
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

sMenuSpacerControl::sMenuSpacerControl(const sChar *name)
{
  Label = name;
}

void sMenuSpacerControl::OnCalcSize()
{
  SizeX = 0;
  SizeY = 10;
}

void sMenuSpacerControl::OnPaint()
{
  sGui->Group(Client,Label);
  /*
  sInt xs,ys;
  sInt w;

  sPainter->Paint(sGui->FlatMat,Client,sGui->Palette[sGC_BUTTON]);
  xs = Client.XSize();
  ys = Client.YSize();
  if(Label)
  {
    w = sPainter->GetWidth(sGui->PropFont,Label)+8;
    sPainter->PrintC(sGui->PropFont,Client,0,Label,sGui->Palette[sGC_TEXT]);
    sPainter->Paint(sGui->FlatMat,Client.x0+2           ,Client.y0+ys/2-1,xs/2-w/2,1,sGui->Palette[sGC_LOW]);
    sPainter->Paint(sGui->FlatMat,Client.x0+2           ,Client.y0+ys/2  ,xs/2-w/2,1,sGui->Palette[sGC_HIGH]);
    sPainter->Paint(sGui->FlatMat,Client.x1-2-(xs/2-w/2),Client.y0+ys/2-1,xs/2-w/2,1,sGui->Palette[sGC_LOW]);
    sPainter->Paint(sGui->FlatMat,Client.x1-2-(xs/2-w/2),Client.y0+ys/2  ,xs/2-w/2,1,sGui->Palette[sGC_HIGH]);
  }
  else
  {
    sPainter->Paint(sGui->FlatMat,Client.x0+2,Client.y0+ys/2-1,xs-4,1,sGui->Palette[sGC_LOW]);
    sPainter->Paint(sGui->FlatMat,Client.x0+2,Client.y0+ys/2  ,xs-4,1,sGui->Palette[sGC_HIGH]);
  }
  */
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

sInt sControlTemplate::AddFlags(sGuiWindow *win,sU32 cmd,sPtr data,sU32 style)
{
  sControlTemplate temp;
  sControl *con;
  const sChar *s;
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
    con->Style |= style;
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
  EnterCmd = 0;
  EscapeCmd = 0;
  RightCmd = 0;
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
  SetCursor = -65536;
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
    if(i<3) i = 3;

    sSPrintF(buffer,sizeof(buffer),"%%.%df",i);
    sSPrintF(out,32,buffer,DataF[zone]);
  }
  if(Type==sCT_HEX)
  {
    sSPrintF(out,32,"%08x",DataU[zone]);
  }
  if(Type==sCT_RGBA || Type==sCT_RGB)
  {
    sSPrintF(out,32,"%3d",(DataU[zone]>>8)&255);
  }
  if(Type==sCT_URGBA || Type==sCT_URGB)
  {
    sSPrintF(out,32,"%3d",(DataU8[zone])&255);
  }
  if(Type==sCT_FRGBA || Type==sCT_FRGB)
  {
    sSPrintF(out,32,"%5.1f",DataF[zone]*255);
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

  sBool skin = (sGui->GetStyle()&sGS_SKIN05);
  sInt font = sGui->PropFont;
  if(skin && (Style&sCS_FAT)) font = sGui->FatFont;
  if(Type==sCT_HEX) font = sGui->FixedFont;

  if((Type==sCT_BUTTON || Type==sCT_CHECKMARK) && Text)
  {
    SizeX = sPainter->GetWidth(font,Text,TextSize) + sPainter->GetWidth(font,"  ")+4;
    if(Shortcut)
    {
      sSystem->GetKeyName(buffer,sizeof(buffer),Shortcut);
      SizeX += sPainter->GetWidth(font,buffer)
             + sPainter->GetWidth(font,"  ");
    }
    if(Type==sCT_CHECKMARK)
      SizeX += sPainter->GetWidth(font,"XX");
  }
  else
  {
    SizeX = (Style & sCS_SMALLWIDTH) ? 60 : 120;
  }
  SizeY = 4+sPainter->GetHeight(font); 
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
  const sChar *str;
  sInt strs;
  sChar buffer[64];
//  sU32 cols[4];
  sInt i,x0,x1,xs,tw,space;
  sRect rr,rx,rc,rl;
  sInt font;
  sInt labelsize;
  sU32 gray;

// init 

  val = 0;
  if(Data)
    val = *DataU; 

  select = 0;
  col0 = sGui->Palette[sGC_BUTTON];
  col1 = sGui->Palette[sGC_TEXT];
  gray = (((sGui->Palette[sGC_TEXT]&0xfcfcfc)>>2)+((sGui->Palette[sGC_BUTTON]&0xfcfcfc)>>2)*3)|0xff000000;
  align = 0;
  str = 0;
  strs = -1;
  labelsize = 0;
  sBool skin = (sGui->GetStyle()&sGS_SKIN05);
  font = sGui->PropFont;
  if(skin && (Style&sCS_FAT)) font = sGui->FatFont;
  if(Type==sCT_HEX) font = sGui->FixedFont;

// change by style

  if(Type==sCT_BUTTON || Type==sCT_CHECKMARK || Type==sCT_CYCLE || Type==sCT_CYCLE )
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
  if(Style & (sCS_STATIC|sCS_GRAY))
  {
    col1 = gray;
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
    if(i==0 && str[0]=='|')
    {
      i=1;
      col1 = gray;
    }
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
  space = sPainter->GetWidth(font," ");

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
    else if(Type==sCT_FRGB || Type==sCT_FRGBA)
    {
      col0 = ((sRange<sInt>(DataF[0]*255,255,0)<<16)&0x00ff0000)
           | ((sRange<sInt>(DataF[1]*255,255,0)<< 8)&0x0000ff00)
           | ((sRange<sInt>(DataF[2]*255,255,0)    )&0x000000ff)
           |                0xff000000;

      if(DataF[0]+DataF[1]+DataF[2] > 1.5f)
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
    if(!(Style & sCS_DONTCLEARBACK))
      sPainter->Paint(sGui->FlatMat,rc,col0);
    if(Style & sCS_ZONES)
    {
      for(i=1;i<Zones;i++)
        sPainter->Paint(sGui->FlatMat,rc.x0+(rc.x1-rc.x0)*i/Zones,rc.y0,Type==sCT_MASK &&(i&7)==0?2:1,rc.y1-rc.y0,sGui->Palette[sGC_DRAW]);
    }
  }
  if(str && strs)
  {
    sPainter->PrintC(font,rc,Text ? (sFA_RIGHT|sFA_SPACE) : (sFA_LEFT|sFA_SPACE),str,col1,strs);
  }

  if(Text)
  {
    if(Type==sCT_CHECKMARK)
    {
      if(State)
        sPainter->PrintC(font,rl,sFA_LEFT|sFA_SPACE,"X",sGui->Palette[sGC_TEXT]);
      rl.x0 += sPainter->GetWidth(font,"XX");
    }
    sCopyString(buffer,Text,sizeof(buffer));
    if(!(Style & sCS_SIDELABEL) && (Style & sCS_EDIT))
    {
      sAppendString(buffer,":",sizeof(buffer));
      align = sFA_LEFT|sFA_SPACE;
      labelsize = sPainter->GetWidth(font,buffer);
    }

    if(Style & (sCS_RIGHTLABEL|sCS_SIDELABEL))
    {
      align = sFA_RIGHT|sFA_SPACE;
    }

    if(Style & sCS_GLOW)
    {
      col = sGui->Palette[sGC_BACK];

      rx = Client; rx.x0++;rx.x1++;rx.y0;rx.y1;
      sPainter->PrintC(font,rx,align,buffer,col,TextSize);
      rx = Client; rx.x0--;rx.x1--;rx.y0;rx.y1;
      sPainter->PrintC(font,rx,align,buffer,col,TextSize);
      rx = Client; rx.x0;rx.x1;rx.y0++;rx.y1++;
      sPainter->PrintC(font,rx,align,buffer,col,TextSize);
      rx = Client; rx.x0;rx.x1;rx.y0--;rx.y1--;
      sPainter->PrintC(font,rx,align,buffer,col,TextSize);
    }
    sPainter->PrintC(font,rl,align,buffer,(Style&sCS_SIDELABEL) ? sGui->Palette[sGC_TEXT] : col1,TextSize);
  }

  if((Style & sCS_EDIT) || ((Style&sCS_EDITNUM) && EditMode==1))
  {
    rr = rc;
    col = col1;

    if(SetCursor!=-65536 && Edit)
    {
      Cursor = 0;
      for(i=1;Edit[i-1];i++)
      {
        x0 = rc.x0+labelsize+sPainter->GetWidth(font,Edit,i)-EditScroll;
        if(SetCursor > x0)
          Cursor = i;
      }
      SetCursor = -65536;
    }

    if(Style & sCS_EDITNUM)
      col = 0xffc00000;
    if((Style & sCS_EDITNUM) && (Style & sCS_ZONES))
    {
      rr.x0 = rc.x0 + (rc.x1-rc.x0)*(EditZone+0)/Zones;
      rr.x1 = rc.x0 + (rc.x1-rc.x0)*(EditZone+1)/Zones;
    }
//    rr.y0++;
//    rr.y1--;
    tw = sPainter->GetWidth(font,Edit) + space;

    if(!(Style & sCS_SIDELABEL) && Text)
    {
      rr.x0 += sPainter->GetWidth(font,Text) + sPainter->GetWidth(font,": ");
    }

    if(EditScroll==-65536)
      EditScroll = -space;
    if((Flags & sGWF_FOCUS) && !(Style & sCS_STATIC))
    {
      x0 = sPainter->GetWidth(font,Edit,Cursor);
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
        x1 = sPainter->GetWidth(font,Edit,Cursor+1);

      if(Overwrite)
        sPainter->Paint(sGui->FlatMat,rr.x0+x0-EditScroll,rr.y0,x1-x0,rr.y1-rr.y0,sGui->Palette[sGC_SELBACK]);
      else
        sPainter->Paint(sGui->FlatMat,rr.x0+x0-1-EditScroll,rr.y0,2,rr.y1-rr.y0,sGui->Palette[sGC_SELBACK]);
    }
    sGui->Clip(rr);
    rx = rr;
    rx.x0 -= EditScroll;
    rr.x1 += 4096;
    sPainter->PrintC(font,rx,sFA_LEFT,Edit,col);
    sGui->ClearClip();
  }

  if(Style & sCS_EDITNUM)
  {
    align = (Text?sFA_RIGHT:sFA_LEFT)|sFA_SPACE;
    if(Type==sCT_MASK) align = 0;
//    if(Style & sCS_EDITCOL) align = sFA_TOP|sFA_RIGHT|sFA_SPACE;

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

          if(SetCursor!=-65536 && DragZone==i)
          {
            Cursor = 0;
            for(i=1;buffer[i-1];i++)
            {
              x0 = rr.x0+labelsize+sPainter->GetWidth(font,buffer,i);
              if(SetCursor > x0)
                Cursor = i;
            }
            SetCursor = -65536;
            if(Cursor>sGetStringLen(buffer))
              Cursor = sGetStringLen(buffer);
          }

          x0 = rr.x0+sPainter->GetWidth(font," ")+sPainter->GetWidth(font,buffer,Cursor);
          sPainter->PrintC(font,rr,align,buffer,col1);
          if(!(Style & sCS_STATIC))
          {
            if(Type!=sCT_MASK)
            {
              if(Swizzle[i]==DragZone && (Flags&sGWF_CHILDFOCUS))
                sPainter->Paint(sGui->FlatMat,x0,rr.y0,2,rr.y1-rr.y0,sGui->Palette[sGC_SELBACK]);
            }
            else
            {
              if(DragZone==Zones-1-i && (Flags&sGWF_CHILDFOCUS))
                sPainter->Paint(sGui->FlatMat,x0,rr.y0,2,rr.y1-rr.y0,sGui->Palette[sGC_SELBACK]);
            }
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
  sU32 ak;

  if(Style&sCS_STATIC) return;
  if(key&sKEYQ_SHIFT) key|=sKEYQ_SHIFT;
  if(key&sKEYQ_CTRL) key|=sKEYQ_CTRL;

  if((!EditMode) && Edit)
  {
    switch(key&(0x8001ffff|sKEYQ_CTRL|sKEYQ_SHIFT))
    {
    case sKEY_HOME:
      if(Type==sCT_FLOAT || Type==sCT_FRGB || Type==sCT_FRGBA)
      {
        DataF[DragZone] = Default[DragZone];
        if(!(sGui->GetStyle()&sGS_NOCHANGE)) Post(ChangeCmd);
      }
      else if(Type!=sCT_URGB && Type!=sCT_URGBA && Type!=sCT_MASK)
      {
        DataS[DragZone] = Default[DragZone];
        if(!(sGui->GetStyle()&sGS_NOCHANGE)) Post(ChangeCmd);
      }
      break;
    case sKEY_HOME|sKEYQ_CTRL:
      if(Type==sCT_FLOAT || Type==sCT_FRGB || Type==sCT_FRGBA)
      {
        for(i=0;i<Zones;i++)
          DataF[i] = Default[i];
        if(!(sGui->GetStyle()&sGS_NOCHANGE)) Post(ChangeCmd);
      }
      else if(Type!=sCT_URGB && Type!=sCT_URGBA && Type!=sCT_MASK)
      {
        for(i=0;i<Zones;i++)
          DataS[i] = Default[i];
        if(!(sGui->GetStyle()&sGS_NOCHANGE)) Post(ChangeCmd);
      }
      break;

    case sKEY_END:
      if(Type==sCT_FLOAT || Type==sCT_FRGB || Type==sCT_RGBA)
      {
        DataF[DragZone] = Max[DragZone];
        if(!(sGui->GetStyle()&sGS_NOCHANGE)) Post(ChangeCmd);
      }
      else if(Type!=sCT_URGB && Type!=sCT_URGBA && Type!=sCT_MASK)
      {
        DataS[DragZone] = Max[DragZone];
        if(!(sGui->GetStyle()&sGS_NOCHANGE)) Post(ChangeCmd);
      }
      break;

    case sKEY_END|sKEYQ_CTRL:
      if(Type==sCT_FLOAT || Type==sCT_FRGB || Type==sCT_RGBA)
      {
        for(i=0;i<Zones;i++)
          DataF[i] = Max[i];
        if(!(sGui->GetStyle()&sGS_NOCHANGE)) Post(ChangeCmd);
      }
      if(Type!=sCT_URGB && Type!=sCT_URGBA && Type!=sCT_MASK)
      {
        for(i=0;i<Zones;i++)
          DataS[i] = Max[i];
        if(!(sGui->GetStyle()&sGS_NOCHANGE)) Post(ChangeCmd);
      }
      break;

    default:
      if(Type!=sCT_MASK)
      {
        ak = key & 0x8001ffff;
        if((ak>=0x20 && ak<0x0001000f)||ak==sKEY_BACKSPACE)
        {
          EditMode = 1;
          EditZone = 0;
          if(Style&sCS_ZONES)
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
          if(Cursor<0) Cursor=0;
          FormatValue(Edit,EditZone);
        }
      }
      break;
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
      if(!(sGui->GetStyle()&sGS_NOCHANGE)) 
        Post(ChangeCmd);
    break;
  case sKEY_INSERT:
    Overwrite = !Overwrite;
    break;
  case sKEY_ESCAPE:
    if(EscapeCmd)
      Post(EscapeCmd);
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
        i = sScanInt(s);
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
        i = sScanInt(s);
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
      case sCT_FRGBA:
      case sCT_FRGB:
        sScanSpace(s);
        fval = sScanFloat(s)/255.0f;
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
    {
      if(EnterCmd && (key&0x8001ffff)==sKEY_ENTER)
        Post(EnterCmd);
      else
        Post(DoneCmd);
    }
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
          if(!(sGui->GetStyle()&sGS_NOCHANGE)) 
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
//  sInt speed;
  sInt dx;
  sU32 qual;

  if(Style&sCS_STATIC) return;
  if(RightCmd && (dd.Mode==sDD_START) && (dd.Buttons&2))
  {
    Post(RightCmd);
    return;
  }
  qual = sSystem->GetKeyboardShiftState();
  if((dd.Flags & sDDF_DOUBLE) && (dd.Buttons&1))
    OnKey(sKEY_HOME|(qual&sKEYQ_CTRL));

  switch(dd.Mode)
  {
  case sDD_START:
//    speed = (dd.Buttons==2) ? 16 : 1;
//    DragStep = Step*speed;
//    speed = 
    DragStep = Step;
    if(dd.Buttons==2)
    {
      switch(Type)
      {
      case sCT_RGB:
      case sCT_RGBA:
      case sCT_URGB:
      case sCT_URGBA:
        DragStep = 8;
        break;
      case sCT_FIXED:
        DragStep = 0x8000;
        break;
      default:
        DragStep = 0.125f;
        break;
      case sCT_FRGBA:
      case sCT_FRGB:
        DragStep = 8/255.0f;
        break;
      }
    }

    DragMode = 0;
    r = Client;
    SetCursor = dd.MouseX;
    DragChanged = 0;
    LastDX = 0;
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
            if(qual&sKEYQ_CTRL)
            {
              for(i=0;i<Zones;i++)
                DragStartU[i] = DataU8[i];
              DragMode = 2;
            }
            else if(qual&sKEYQ_ALT)
            {
              for(i=0;i<Zones;i++)
                DragStartU[i] = DataU8[i];
              DragMode = 3;
              DragStep = (dd.Buttons&2) ? 100 : 500;
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
              if(qual&sKEYQ_CTRL)
              {
                for(i=0;i<Zones;i++)
                  DragStartU[i] = DataU[i];
                DragMode = 2;
              }
              else if(qual&sKEYQ_ALT)
              {
                for(i=0;i<Zones;i++)
                  DragStartU[i] = DataU[i];
                DragMode = 3;
                DragStep = (dd.Buttons&2) ? 100 : 500;
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
    if(Type==sCT_MASK)
    {
      if(qual & sKEYQ_SHIFT)
        DataU[0] = DragStartU[0]^(1<<DragZone);
      else if(DataU[0] != (1 << DragZone))
        DataU[0] = (1<<DragZone);
      else
        DataU[0] = 0;

      if(DataU[0]!=DragStartU[0])
      {
        if(!(sGui->GetStyle()&sGS_NOCHANGE)) 
          Post(ChangeCmd);
        Post(DoneCmd);
      }
    }
    break;

  case sDD_DRAG:
    if(DragMode && (Style&sCS_EDITNUM))
    {
      dx = dd.DeltaX/2;
      if(dx!=LastDX)
      {
        LastDX = dx;
        DragChanged = 1;
        if(DragMode==3) // scaling
        {
          i0 = 0;
          i1 = Zones;
          sF32 mydragstep = 1/DragStep;
          dx += DragStep;
          for(i=i0;i<i1;i++)
          {
            if(Type==sCT_INT)
            {
              DataS[i] = sRange<sInt>(DragStartS[i]*dx*mydragstep,Max[i],Min[i]);
            }
            if(Type==sCT_HEX)
            {
              DataU[i] = sRange<sU32>(DragStartU[i]*dx*mydragstep,Max[i],Min[i]);
            }
            if(Type==sCT_FLOAT)
            {
              DataF[i] = sRange<sF32>(DragStartF[i]*dx*mydragstep,Max[i],Min[i]);
            }
            if(Type==sCT_FIXED)
            {
              DataS[i] = sRange<sInt>(DragStartS[i]*dx*(mydragstep),Max[i],Min[i]);
            }
            if(Type==sCT_RGB)
            {
              DataS[i] = sRange<sInt>(DragStartS[i]*dx*(mydragstep),Max[i],Min[i]);
            }
            if(Type==sCT_RGBA)
            {
              if(i!=3)
                DataS[i] = sRange<sInt>(DragStartS[i]*dx*(mydragstep),Max[i],Min[i]);
            }
            if(Type==sCT_FRGB)
            {
              DataF[i] = sRange<sF32>(DragStartF[i]*dx*mydragstep,Max[i],Min[i]);
            }
            if(Type==sCT_FRGBA)
            {
              if(i!=3)
                DataF[i] = sRange<sF32>(DragStartF[i]*dx*mydragstep,Max[i],Min[i]);
            }
            if(Type==sCT_URGB)
            {
              DataU8[i] = sRange<sInt>(DragStartS[i]*dx*mydragstep,Max[i],Min[i]);
            }
            if(Type==sCT_URGBA)
            {
              if(i!=3)
                DataU8[i] = sRange<sInt>(DragStartS[i]*dx*mydragstep,Max[i],Min[i]);
            }
          }
        }
        else
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
              DataS[i] = sRange<sInt>(DragStartS[i]+dx*DragStep,Max[i],Min[i]);
            }
            if(Type==sCT_HEX)
            {
              DataU[i] = sRange<sU32>(DragStartU[i]+dx*DragStep,Max[i],Min[i]);
            }
            if(Type==sCT_FLOAT)
            {
              DataF[i] = sRange<sF32>(DragStartF[i]+dx*DragStep,Max[i],Min[i]);
            }
            if(Type==sCT_FIXED)
            {
              DataS[i] = sRange<sInt>(DragStartS[i]+dx*((sInt)DragStep),Max[i],Min[i]);
            }
            if(Type==sCT_RGB)
            {
              DataS[i] = sRange<sInt>(DragStartS[i]+dx*((sInt)DragStep),Max[i],Min[i]);
            }
            if(Type==sCT_RGBA)
            {
              if(DragMode!=2 || i!=3)
                DataS[i] = sRange<sInt>(DragStartS[i]+dx*((sInt)DragStep),Max[i],Min[i]);
            }
            if(Type==sCT_FRGB)
            {
              DataF[i] = sRange<sF32>(DragStartF[i]+dx*DragStep,Max[i],Min[i]);
            }
            if(Type==sCT_FRGBA)
            {
              if(DragMode!=2 || i!=3)
                DataF[i] = sRange<sF32>(DragStartF[i]+dx*DragStep,Max[i],Min[i]);
            }
            if(Type==sCT_URGB)
            {
              DataU8[i] = sRange<sInt>(DragStartS[i]+dx*DragStep,Max[i],Min[i]);
            }
            if(Type==sCT_URGBA)
            {
              if(DragMode!=2 || i!=3)
                DataU8[i] = sRange<sInt>(DragStartS[i]+dx*DragStep,Max[i],Min[i]);
            }
          }
        }
        if(!(sGui->GetStyle()&sGS_NOCHANGE)) Post(ChangeCmd);
      }
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
      if(DragChanged)
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
  const sChar *str;
  const sChar *s0;
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
      if(*s0=='\n')
      {
        mf->AddColumn();
        s0++;
      }
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

void sControl::Init(sInt type,sU32 style,const sChar *label,sU32 cmd,sPtr data)
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

void sControl::InitCycle(const sChar *str)
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
  const sChar *name;
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
  case sCT_FRGB:
    EditFRGB(cmd,(sF32 *)data,name);
    for(i=0;i<3;i++)
    {
      Min[i] = temp->Min[i];
      Max[i] = temp->Max[i];
      Default[i] = temp->Default[i];
    }
    break;
  case sCT_FRGBA:
    EditFRGBA(cmd,(sF32 *)data,name);
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


void sControl::Button(const sChar *label,sU32 cmd)
{
  sCopyString(NumEditBuf,label,sizeof(NumEditBuf));
  Init(sCT_BUTTON,sCS_FATBORDER|sCS_BORDERSEL,NumEditBuf,cmd,0);
  Flags |= sGWF_SQUEEZE;
}

void sControl::Menu(const sChar *label,sU32 cmd,sU32 shortcut)
{
  sCopyString(NumEditBuf,label,sizeof(NumEditBuf));
  Init(sCT_BUTTON,sCS_NOBORDER|sCS_HOVER|sCS_LEFT,NumEditBuf,cmd,0);
  Shortcut = shortcut;
  Flags |= sGWF_SQUEEZE;
}

void sControl::MenuCheck(const sChar *label,sU32 cmd,sU32 shortcut,sInt state)
{
  Menu(label,cmd,shortcut);
  Type = sCT_CHECKMARK;
  State = state;
}

void sControl::Label(const sChar *label)
{
  if(sGetStringLen(label)<sizeof(NumEditBuf))
  {
    sCopyString(NumEditBuf,label,sizeof(NumEditBuf));
    label = NumEditBuf;
  }
  Init(sCT_BUTTON,sCS_NOBORDER|sCS_DONTCLEARBACK,label,0,0);
  Flags |= sGWF_SQUEEZE;
}

void sControl::EditBool(sU32 cmd,sInt *ptr,const sChar *label)
{
  Init(sCT_TOGGLE,sCS_FATBORDER|sCS_BORDERSEL,label,cmd,ptr);
  Flags |= sGWF_SQUEEZE;
}

void sControl::EditCycle(sU32 cmd,sInt *ptr,const sChar *label,const sChar *choices)
{
  Init(sCT_CYCLE,sCS_FATBORDER|sCS_BORDERSEL|sCS_LEFT,label,cmd,ptr);
  InitCycle(choices);
  Flags |= sGWF_SQUEEZE;
}

void sControl::EditChoice(sU32 cmd,sInt *ptr,const sChar *label,const sChar *choices)
{
  Init(sCT_CHOICE,sCS_FATBORDER|sCS_BORDERSEL|sCS_LEFT,label,cmd,ptr);
  InitCycle(choices);
  Flags |= sGWF_SQUEEZE;
}

void sControl::EditMask(sU32 cmd,sInt *ptr,const sChar *label,sInt max,const sChar *choices)
{
  Init(sCT_MASK,sCS_ZONES|sCS_EDITNUM,label,cmd,ptr);
  InitNum(0,0,0,0);
  Zones = max;
  Cycle = choices;

  Flags |= sGWF_SQUEEZE;
}

void sControl::EditString(sU32 cmd,sChar *edit,const sChar *label,sInt size)
{
  Init(sCT_STRING,sCS_LEFT|sCS_EDIT,label,cmd,0);
  Edit = edit;
  EditSize = size;
  EditMode = 1;
  Flags |= sGWF_SQUEEZE;
}

void sControl::EditInt(sU32 cmd,sInt *ptr,const sChar *label)
{
  Init(sCT_INT,sCS_LEFT|sCS_EDITNUM,label,cmd,ptr);
  InitNum(0,1,0.25,0);
  Flags |= sGWF_SQUEEZE;
}

void sControl::EditFloat(sU32 cmd,sF32 *ptr,const sChar *label)
{
  Init(sCT_FLOAT,sCS_LEFT|sCS_EDITNUM,label,cmd,ptr);
  InitNum(0,1,0.25,0);
  Flags |= sGWF_SQUEEZE;
}

void sControl::EditHex(sU32 cmd,sU32 *ptr,const sChar *label)
{
  Init(sCT_HEX,sCS_LEFT|sCS_EDITNUM,label,cmd,ptr);
  InitNum(0,1,0.25,0);
  Flags |= sGWF_SQUEEZE;
}

void sControl::EditFixed(sU32 cmd,sInt *ptr,const sChar *label)
{
  Init(sCT_FIXED,sCS_LEFT|sCS_EDITNUM,label,cmd,ptr);
  InitNum(0,1,0.25,0);
  Flags |= sGWF_SQUEEZE;
}

void sControl::EditRGB(sU32 cmd,sInt *ptr,const sChar *label)
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

void sControl::EditRGBA(sU32 cmd,sInt *ptr,const sChar *label)
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


void sControl::EditURGB(sU32 cmd,sInt *ptr,const sChar *label)
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

void sControl::EditURGBA(sU32 cmd,sInt *ptr,const sChar *label)
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

void sControl::EditFRGB(sU32 cmd,sF32 *ptr,const sChar *label)
{
  Init(sCT_FRGB,sCS_LEFT|sCS_EDITNUM|sCS_EDITCOL,label,cmd,ptr);
  InitNum(0,1,1.0f/255.0f,1);
  Zones = 3;
  Swizzle[0] = 0;
  Swizzle[1] = 1;
  Swizzle[2] = 2;
  Swizzle[3] = 3;
  Flags |= sGWF_SQUEEZE;
  Style |= sCS_ZONES;
}

void sControl::EditFRGBA(sU32 cmd,sF32 *ptr,const sChar *label)
{
  Init(sCT_FRGBA,sCS_LEFT|sCS_EDITNUM|sCS_EDITCOL,label,cmd,ptr);
  InitNum(0,1,1.0f/255.0f,1);
  Zones = 4;
  Swizzle[0] = 3;
  Swizzle[1] = 0;
  Swizzle[2] = 1;
  Swizzle[3] = 2;
  Flags |= sGWF_SQUEEZE;
  Style |= sCS_ZONES;
}

/****************************************************************************/
/****************************************************************************/

sImageButton::sImageButton()
{
  InitContext();
  Hover = 0;
}

void sImageButton::InitContext()
{
  ImgSizeX = 15;
  ImgSizeY = 14;
  OffUV.Init(64/128.0f,16/128.0f,(64+14)/128.0f,(16+14)/128.0f);
  HoverUV.Init(88/128.0f,16/128.0f,(88+14)/128.0f,(16+14)/128.0f);
  Material = sGui->SkinMat;
}

void sImageButton::OnCalcSize()
{
  SizeX = ImgSizeX;
  SizeY = ImgSizeY;
}

void sImageButton::OnPaint()
{
  sPainter->Paint(Material,Client,(Flags & sGWF_HOVER) ? HoverUV : OffUV,~0);
  sPainter->Paint(sGui->AlphaMat,Client,0x40ffffff);
}

void sImageButton::OnDrag(sDragData &dd)
{
  if(dd.Mode==sDD_STOP && Client.Hit(dd.MouseX,dd.MouseY) && Cmd)
    Send(Cmd);
}

/****************************************************************************/
/****************************************************************************/


#define LCD_SELECT    1
#define LCD_MOVE      2
#define LCD_DUP       3

sListControl::sListControl()
{
  sInt i;

  List.Init(16);
  LastSelected = -1;
  Height = sPainter->GetHeight(sGui->PropFont)+2;
  Style = 0;
  LeftCmd = 0;
  MenuCmd = 0;
  DoubleCmd = 0;
  TreeCmd = 0;
  DragMode = 0;
  DragLineIndex = -1;

  Dialog = 0;
  HandleIndex = 0;
  NameBuffer[0] = 0;


  for(i=0;i<=sLISTCON_TABMAX;i++)
    TabStops[i] = 80*i;
}

sListControl::~sListControl()
{
  List.Exit();
}

void sListControl::OnCalcSize()
{
  sInt i;

  Height = sPainter->GetHeight(sGui->PropFont)+2;
  SizeX = 75;
  SizeY = 0;
  for(i=0;i<List.Count;i++)
    if(!List.Get(i).Hidden)
      SizeY += Height;
}

void sListControl::OnPaint()
{
  sInt i,j,len;
  sInt hidelevel;
  sChar *scan,*start;
  sInt y,x;
  sRect r,rr;
  sListControlEntry *e;
  sChar buffer[2];
  sU32 col;

  ClearBack();

  y = Client.y0;
  hidelevel = 99;
  for(i=0;i<List.Count;i++)
  {
    e = &List[i];
    e->Hidden = 1;
    if(e->Level<=hidelevel)
    {
      hidelevel = 99;
      e->Hidden = 0;
      r = Client;
      r.y0 = y;
      y+=Height;
      r.y1 = y;
      col = 0;
      if(e->Color)
        col = e->Color;
      if(e->Select)
        col = sGui->Palette[sGC_SELBACK];
      if(col)
        sPainter->Paint(sGui->FlatMat,r,col);
      start = e->Name;
      x = Client.x0;
      col = sGui->Palette[e->Select ? sGC_SELECT : sGC_TEXT];
      if(Style&sLCS_TREE)
      {
        x += e->Level*10+10;
        if(e->Button!=' ')
        {
          rr.x0 = x-10;
          rr.x1 = x;
          rr.y0 = r.y0+1;
          rr.y1 = r.y1-1;
          buffer[0] = e->Button;
          buffer[1] = 0;
          sPainter->PrintC(sGui->PropFont,rr,0,buffer,col);
          rr.Extend(-1);
          rr.x0 += -1;
          sGui->RectHL(rr,1,col,col);
        }
        if(e->Button=='+')
          hidelevel = e->Level;
      }
      if(r.y0<ClientClip.y1 && r.y1>ClientClip.y0)
      {
        for(j=0;j<sLISTCON_TABMAX && *start;j++)
        {
          scan = start;
          len = 0;
          while(*scan!='|' && *scan!=0)
          {
            scan++;
            len++;
          }
          r.x0 = x+TabStops[j];
          if(*scan==0)
            r.x1 = Client.x1;
          else
            r.x1 = x+TabStops[j+1];

          sPainter->PrintC(sGui->PropFont,r,sFA_LEFT|sFA_SPACE,start,col,len);

          if(*scan==0)
            start = scan;
          else
            start = scan+1;
        }
        if(i==DragLineIndex)
          sPainter->Paint(sGui->FlatMat,Client.x0,y-1-Height,Client.XSize(),2,sGui->Palette[sGC_DRAW]);
      }
    }
  }
  r = Client;
  r.y0 = y;
//  if(r.y0<r.y1)
//    sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_BACK]);
  if(List.Count==DragLineIndex)
    sPainter->Paint(sGui->FlatMat,Client.x0,y-1,Client.XSize(),2,sGui->Palette[sGC_DRAW]);

  OnCalcSize();
}

void sListControl::OnDrag(sDragData &dd)
{
  sInt nr;
  sU32 qual;
  sInt i,max;

  max = (dd.MouseY-Client.y0)/Height;
  nr = 0;
  for(i=0;i<List.Count;i++)
  {
    if(!List[i].Hidden)
    {
      if(max==0)
        break;
      max--;
    }
    nr++;
  }

  if(dd.Mode==sDD_START)
  {
    DragMode = 0;

    if((dd.Buttons&4) && dd.Flags&sDDF_DOUBLE)
    {
      SetSelect(nr,1);
      Post(LeftCmd);
      Post(MenuCmd);
      return;
    }
    if((dd.Buttons&1) && dd.Flags&sDDF_DOUBLE)
    {
      SetSelect(nr,1);
      Post(DoubleCmd);
      return;
    }
  }
  if(MMBScrolling(dd,DragStartX,DragStartY)) return ;


  qual = sSystem->GetKeyboardShiftState();
  switch(dd.Mode)
  {
  case sDD_START:
    if(nr>=0 && nr<List.Count)
    {
      if(dd.Buttons&1)
      {
        if(qual & sKEYQ_CTRL)
        {
          SetSelect(nr,!GetSelect(nr));
        }
        else
        {
          DragMode = LCD_SELECT;
          ClearSelect();
          SetSelect(nr,1);
          if(dd.MouseX < Client.x0+List[nr].Level*10+10 && dd.MouseX > Client.x0+List[nr].Level*10) 
          {
            if(List[nr].Button=='+')
            {
              List[nr].Button='-';
              Send(TreeCmd);
            }
            else if(List[nr].Button=='-')
            {
              List[nr].Button='+';
              Send(TreeCmd);
            }
          }
        }
        Post(LeftCmd);
      }
      if((dd.Buttons&2) && (Style & sLCS_DRAG))
      {
        HandleIndex = nr;
        SetSelect(nr,1);
        DragMode = LCD_MOVE;
        if(qual & sKEYQ_CTRL)
          DragMode = LCD_DUP;
      }
    }
    break;
  case sDD_DRAG:
    switch(DragMode)
    {
    case LCD_SELECT:
//      sDPrintF("%d %d\n",nr,GetSelect());
      if(nr!=GetSelect())
      {
        ClearSelect();
        SetSelect(nr,1);
        Post(LeftCmd);
      }
      break;

    case LCD_MOVE:
    case LCD_DUP:
      InsertIndex = nr;
      DragLineIndex = InsertIndex;
      if(InsertIndex==HandleIndex)
        DragLineIndex = -1;
      break;
    }
    break;
  case sDD_STOP:
    switch(DragMode)
    {
    case LCD_MOVE:
      if(InsertIndex!=HandleIndex)
      {
        if(InsertIndex>HandleIndex)
          InsertIndex--;
        sVERIFY(HandleIndex<List.Count);
        sVERIFY(InsertIndex<=List.Count-1);
        Send(sLCS_CMD_EXCHANGE);
        SetSelect(InsertIndex,1);
        AverageLevel(InsertIndex);
        OpenFolder(InsertIndex);
      }
      break;
    case LCD_DUP:
      if(InsertIndex!=HandleIndex)
      {
        sVERIFY(HandleIndex<List.Count);
        sVERIFY(InsertIndex<=List.Count);
        Send(sLCS_CMD_DUP);
        SetSelect(InsertIndex,1);
        AverageLevel(InsertIndex);
        OpenFolder(InsertIndex);
      }
      break;
    }
    DragMode = 0;
    DragLineIndex = -1;
    break;
  }
}

/****************************************************************************/

void sListControl::OnKey(sU32 key)
{
  sInt i;

  if(key&sKEYQ_CTRL)
    key|=sKEYQ_CTRL;
  switch(key&(0x8001ffff|sKEYQ_CTRL))
  {
  case sKEY_DOWN:
    for(i=LastSelected+1;i<List.Count;i++)
    {
      if(!List[i].Hidden)
      {
        LastSelected = i;
        Send(LeftCmd);
        break;
      }
    }
    break;
  case sKEY_UP:
    for(i=LastSelected-1;i>=0;i--)
    {
      if(!List[i].Hidden)
      {
        LastSelected = i;
        Send(LeftCmd);
        break;
      }
    }
    break;
  case sKEY_LEFT|sKEYQ_CTRL:
    i = GetSelect();
    if(i>=0 && i<List.Count && List[i].Level>0)
    {
      List[i].Level--;
      CalcLevel();
      Send(TreeCmd);
    }
    break;

  case sKEY_RIGHT|sKEYQ_CTRL:
    i = GetSelect();
    if(i>=0 && i<List.Count)
    {
      List[i].Level++;
      CalcLevel();
      Send(TreeCmd);
    }
    break;

  case sKEY_SPACE:
    Send(DoubleCmd);
    break;

  case '+':
  case sKEY_LEFT:
    i = GetSelect();
    if(i>=0 && i<List.Count && List[i].Button=='-')
    {
      List[i].Button='+';
      Send(TreeCmd);
    }
    break;    

  case '-':
  case sKEY_RIGHT:
    i = GetSelect();
    if(i>=0 && i<List.Count && List[i].Button=='+')
    {
      List[i].Button='-';
      Send(TreeCmd);
    }
    break;    

  case 'a':
    if(Style&sLCS_HANDLING)
      OnCommand(sCMD_LIST_ADD);
    break;
  case sKEY_BACKSPACE:
  case sKEY_DELETE:
    if(Style&sLCS_HANDLING)
      OnCommand(sCMD_LIST_DELETE);
    break;
  case 'r':
    if(Style&sLCS_HANDLING)
      OnCommand(sCMD_LIST_RENAME);
    break;
  case sKEY_UP|sKEYQ_CTRL:
    if((Style&sLCS_HANDLING) && (Style&sLCS_TREE))
      OnCommand(sCMD_LIST_MOVEUP);
    break;
  case sKEY_DOWN|sKEYQ_CTRL:
    if((Style&sLCS_HANDLING) && (Style&sLCS_TREE))
      OnCommand(sCMD_LIST_MOVEDOWN);
    break;
  } 
}

void sListControl::SetRename(sChar *buffer,sInt buffersize)
{
  Dialog->InitString(buffer,buffersize);
}

sBool sListControl::OnCommand(sU32 cmd)
{
  sInt i;

  switch(cmd)
  {
  case sCMD_LIST_ADD:
    sCopyString(NameBuffer,"<<new>>",sizeof(NameBuffer));
    HandleIndex = GetSelect();
    if(HandleIndex<0 || HandleIndex>List.Count)
      HandleIndex = List.Count;
    Send(sLCS_CMD_ADD);
    Send(sLCS_CMD_UPDATE);
    SetSelect(HandleIndex,1);
    return sTRUE;
  case sCMD_LIST_DELETE:
    if(GetSelect()>=0)
    {
      Dialog = new sDialogWindow;
      Dialog->InitOkCancel(this,"delete","delete scene",sCMD_LIST_DELETE2,sCMD_LIST_CANCEL);
    }
    return sTRUE;
  case sCMD_LIST_DELETE2:
    HandleIndex = GetSelect();
    if(HandleIndex>=0)
    {
      Send(sLCS_CMD_DEL);
      Send(sLCS_CMD_UPDATE);
      SetSelect(HandleIndex,1);
      Post(LeftCmd);
      sGui->SetFocus(this);
    }
    Dialog = 0;
    return sTRUE;
  case sCMD_LIST_RENAME:
    HandleIndex = GetSelect();
    if(HandleIndex>=0)
    {
      Dialog = new sDialogWindow;
      Send(sLCS_CMD_RENAME);
      Dialog->InitOkCancel(this,"rename","enter new name",sCMD_LIST_RENAME2,sCMD_LIST_CANCEL);
    }
    Dialog = 0;
    return sTRUE;
  case sCMD_LIST_RENAME2:
    Send(sLCS_CMD_UPDATE);
    sGui->SetFocus(this);
    return sTRUE;
  case sCMD_LIST_MOVEUP:
    i = GetSelect();
    if(i>=1 && i<GetCount())
    {
      HandleIndex = i-1;
      Send(sLCS_CMD_SWAP);
      Send(sLCS_CMD_UPDATE);
      SetSelect(i-1,sTRUE);
      AverageLevel(i-1);
      OpenFolder(i-1);
      Post(LeftCmd);
    }
    return sTRUE;
  case sCMD_LIST_MOVEDOWN:
    i = GetSelect();
    if(i>=0 && i<GetCount()-1)
    {
      HandleIndex = i;
      Send(sLCS_CMD_SWAP);
      Send(sLCS_CMD_UPDATE);
      SetSelect(i+1,sTRUE);
      AverageLevel(i+1);
      OpenFolder(i+1);
      Post(LeftCmd);
    }
    return sTRUE;
  case sCMD_LIST_CANCEL:
    Dialog = 0;
    sGui->SetFocus(this);
    return sTRUE;
  }
  return sFALSE;
}

/****************************************************************************/

void sListControl::Set(sInt nr,sChar *name,sInt cmd,sU32 color)
{
  List.AtLeast(nr+1);
  List.Count = sMax(List.Count,nr+1);
  List[nr].Name = name;
  List[nr].Cmd = cmd;
  List[nr].Select = 0;
  List[nr].Level = 0;
  List[nr].Button = ' ';
  List[nr].Hidden = 0;
  List[nr].Color = color;
}

void sListControl::Add(sChar *name,sInt cmd,sU32 color)
{
  Set(List.Count,name,cmd,color);
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

void sListControl::SetName(sInt nr,sChar *name)
{
  if(nr>=0 && nr<List.Count)
    List[nr].Name = name;
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
    if(Style & sLCS_MULTISELECT)
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

void sListControl::ScrollTo(sInt nr)
{
  sRect r;
  sInt i,j;
  if(nr>=0 && nr<List.Count)
  {
    j = 0;
    for(i=0;i<nr;i++)
      if(!List.Get(i).Hidden)
        j++;
    r.Init(Client.x0,Client.y0+j*Height,Client.x1,Client.y0+(j+1)*Height);
    sGuiWindow::ScrollTo(r,0);
  }
}

void sListControl::GetTree(sInt nr,sInt &level,sInt &button)
{
  if(nr>=0 && nr<List.Count)
  {
    level = List[nr].Level;
    button = List[nr].Button;
  }
  else
  {
    level = 0;
    button = ' ';
  } 
}

void sListControl::SetTree(sInt nr,sInt level,sInt button)
{
  if(nr>=0 && nr<List.Count)
  {
    if(level<0)
      level = 0;
    if(button!='+' && button!='-')
      button = ' ';
    List[nr].Level = level;
    List[nr].Button = button;
  }
}

void sListControl::CalcLevel()
{
  sInt i;
  sListControlEntry *e;
  sInt hidelevel;

  if(List.Count==0) return;

  for(i=0;i<List.Count-1;i++)
  {
    if(List[i+1].Level>List[i].Level)
    {
      if(List[i].Button==' ')
        List[i].Button = '-';
    }
    else
    {
      List[i].Button=' ';
    }
  }
  List[List.Count-1].Button = ' ';

  hidelevel = 99;
  for(i=0;i<List.Count;i++)
  {
    e = &List[i];
    e->Hidden = 1;
    if(e->Level<=hidelevel)
    {
      hidelevel = 99;
      e->Hidden = 0;
      if(e->Button=='+')
        hidelevel = e->Level;
    }
  }
}

void sListControl::OpenFolder(sInt nr)
{
  sInt level;
  sBool change;

  level = List[nr].Level+1;

  change = 0;
  while(nr>=0)
  {
    if(List[nr].Level<level)
    {
      if(List[nr].Button=='+')
      {
        List[nr].Button='-';
        change = 1;
      }
      level = List[nr].Level;
    }
    nr--;
  }
  if(change)
    Send(TreeCmd);
}

void sListControl::AverageLevel(sInt nr)
{
  sInt level[2];
  sInt i,l;
  i= 0;
  if(nr>=0 && nr<List.Count)
  {
    if(nr-1>=0)
      level[i++] = List[nr-1].Level;
    if(nr+2<List.Count)
      level[i++] = List[nr+1].Level;    

    l = List[nr].Level;
    if(i==2)
      l = sMax(level[0],level[1]);
    if(i==1)
      l = level[0];
    if(List[nr].Level != l)
    {
      List[nr].Level = l;
      CalcLevel();
      Send(TreeCmd);
    }
  }
}

/****************************************************************************/

sListHeader::sListHeader(sListControl *list)
{
  List = list;
  TabCount = 0;
  Height = sPainter->GetHeight(sGui->PropFont) + 4;

  DragTab = -1;
}

void sListHeader::OnCalcSize()
{
  SizeY += Height;
}

void sListHeader::OnSubBorder()
{
  Parent->Client.y0 += Height;
}

void sListHeader::OnPaint()
{
  sInt i,x;
  sRect r;

  x = 0;

  for(i=0;i<TabCount;i++)
  {
    r.x0 = Client.x0 + List->TabStops[i];
    r.y0 = Client.y0;
    r.x1 = x = sMin(Client.x0 + List->TabStops[i+1],Client.x1);
    r.y1 = Client.y0 + Height;
    if(DragTab == -1 || (i != DragTab -1 && i != DragTab))
      sGui->Bevel(r,sFALSE);
    else
      sGui->Bevel(r,sTRUE);

    sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_BUTTON]);

    r.Extend(-1);
    r.x0++;
    sPainter->PrintC(sGui->PropFont,r,sFA_CLIP|sFA_LEFT,TabName[i],sGui->Palette[sGC_TEXT]);
  }

  r.Init(x,Client.y0,Client.x1,Client.y0 + Height);
  sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_BUTTON]);
}

void sListHeader::OnDrag(sDragData &dd)
{
  sInt i;
  sRect r;

  switch(dd.Mode)
  {
  case sDD_START:
    for(i=1;i<TabCount;i++)
    {
      r.x0 = Client.x0 + List->TabStops[i] - 2;
      r.y0 = Client.y0;
      r.x1 = r.x0 + 4;
      r.y1 = Client.y0 + Height;

      if(r.Hit(dd.MouseX,dd.MouseY))
      {
        DragTab = i;
        DragStart = List->TabStops[i];
        dd.DeltaX = 0;
        dd.DeltaY = 0;
      }
    }
    break;

  case sDD_DRAG:
    if(DragTab != -1)
      List->TabStops[DragTab] = sRange(DragStart + dd.DeltaX,
      sMin(List->TabStops[DragTab+1]-4,Client.x1-Client.x0-1),
      List->TabStops[DragTab-1]+4);
    break;

  case sDD_STOP:
    DragTab = -1;
    break;
  }
}

void sListHeader::AddTab(sChar *name)
{
  if(TabCount < sLISTCON_TABMAX)
    TabName[TabCount++] = name;
}

/****************************************************************************/
/***                                                                      ***/
/***   Operator Window                                                    ***/
/***                                                                      ***/
/****************************************************************************/

#define DM_PICK         1         // select one element by picking
#define DM_RECT         2         // select by rectangle
#define DM_DUPLICATE    3         // duplicate selection
#define DM_WIDTH        4         // change width
#define DM_SELECT       5         // generic selecting (DM_PICK or DM_RECT)
#define DM_MOVE         6         // move around
#define DM_SCROLL       7
#define DM_BIRDSEYE     8

#define PAGEW           7         // width in pixel of width-dragging button
#define BXSIZE 3                  // size for birdseye
#define BYSIZE 3                  // size for birdseye

#define CMD_PAGE_SHOW     0x1001
#define CMD_PAGE_POPADD   0x1002

/****************************************************************************/

void sOpWindow::sOpInfo::PrefixName(const sChar *str)
{
  sChar buffer[64];
  sCopyString(buffer,str,64);
  sAppendString(buffer,Name,64);
  sCopyString(Name,buffer,64);
}

/****************************************************************************/

sOpWindow::sOpWindow()
{
  PageMaxX = 32;
  PageMaxY = 32;
  PageX = 28;
  PageY = 19;

  Birdseye = 0;
  DragMode = 0;

  CursorX = 0;
  CursorY = 0;
  CursorWidth = 3;

  OpWindowX = -1;
  OpWindowY = -1;
}

sOpWindow::~sOpWindow()
{
}

void sOpWindow::OnCalcSize()
{
  if(sGui->GetStyle()&sGS_SMALLFONTS)
  {
    PageX = 18;
    PageY = 15;
  }
  else
  {
    PageX = 28;
    PageY = 19;
  }
  SizeX = PageX*PageMaxX;
  SizeY = PageY*PageMaxY;
}

void sOpWindow::Tag()
{
  sGuiWindow::Tag();
}

/****************************************************************************/
/****************************************************************************/

void sOpWindow::OnKey(sU32 key)
{
}

void sOpWindow::AddOperatorMenu(sMenuFrame *mf)
{
  mf->AddBorder(new sNiceBorder);
  mf->SendTo = this;
  if(OpWindowX==-1)
  {
    sGui->GetMouse(OpWindowX,OpWindowY);
    sGui->AddWindow(mf,OpWindowX,OpWindowY,-1,1);
    OpWindowX = mf->Position.x0;
    OpWindowY = mf->Position.y0;
  }
  else
  {
    sGui->AddWindow(mf,OpWindowX,OpWindowY);
  }
}

/****************************************************************************/

void sOpWindow::OnPaint() 
{ 
  sInt i,max;
  sInt j,pw;
  sU32 col,colt;
  sRect r,rr;
  sBool small,sel;
  sOpInfo oi;
  sInt shadow;

  if(Flags&sGWF_CHILDFOCUS)
  {
    OpWindowX = -1;
    OpWindowY = -1;
  }

  small = (sGui->GetStyle()&sGS_SMALLFONTS) ? 1 : 0;

  ClearBack();

  r.x0 = Client.x0 + CursorX*PageX;
  r.y0 = Client.y0 + CursorY*PageY;
  r.x1 = Client.x0 + (CursorX+CursorWidth)*PageX;
  r.y1 = Client.y0 + (CursorY+1)*PageY;
  sGui->RectHL(r,1,sGui->Palette[sGC_DRAW],sGui->Palette[sGC_DRAW]);

  max = GetOpCount();
  for(i=0;i<max;i++)
  {
    GetOpInfo(i,oi);

    r.x0 = Client.x0 + oi.PosX*PageX;
    r.y0 = Client.y0 + oi.PosY*PageY;
    r.x1 = Client.x0 + (oi.PosX+oi.Width)*PageX;
    r.y1 = Client.y0 + (oi.PosY+1)*PageY;
    sel = (oi.Style & sOIS_SELECT) ? 1:0;

    col = 0xffc0c0c0;
    if(oi.Color)
      col = oi.Color;
    colt = sGui->Palette[sGC_TEXT];
    if(oi.Style & sOIS_GRAYOUT)
      colt = (((colt&0xfefefe)>>1)+((col&0xfefefe)>>1))|0xff000000;
    if(oi.Style & sOIS_ERROR)
      col = 0xffff8080;

    rr=r;
    sGui->Bevel(rr,sel);
    sPainter->Paint(sGui->FlatMat,rr,col);
    sPainter->PrintC(sGui->PropFont,rr,0,oi.Name,colt);

    if(small)
    {
      if(oi.Style & sOIS_LED1)
        sPainter->Paint(sGui->FlatMat,r.x0+2,r.y0+2,4,3,0xff7f0000);
      if(oi.Style & sOIS_LED2)
        sPainter->Paint(sGui->FlatMat,r.x0+2,r.y1-5,4,3,0xff007f00);
      if(oi.Style & sOIS_LED3)
        sPainter->Paint(sGui->FlatMat,r.x0+2,(r.y0+r.y1)/2-1,4,2,0xff00007f);
    }
    else
    {
      if(oi.Style & sOIS_LED1)
        sPainter->Paint(sGui->FlatMat,r.x0+4,r.y0+4,8,4,0xff7f0000);
      if(oi.Style & sOIS_LED2)
        sPainter->Paint(sGui->FlatMat,r.x0+4,r.y1-8,8,4,0xff007f00);
      if(oi.Style & sOIS_LED3)
        sPainter->Paint(sGui->FlatMat,r.x0+4,r.y0+8,8,4,0xff00007f);
    }

    col = sGui->Palette[sGC_DRAW];
    sPainter->Line(r.x1-PAGEW,r.y0+1,r.x1-PAGEW,r.y1-1,col);
    if(oi.Wheel>=0)
    {
      pw = (PageY-2)/3;
      j = (oi.Wheel&0xffff)*pw/65536+1;
      sPainter->Line(r.x1-PAGEW,r.y0+j,r.x1-1,r.y0+j,col);
      sPainter->Line(r.x1-PAGEW,r.y0+j+pw,r.x1-1,r.y0+j+pw,col);
      sPainter->Line(r.x1-PAGEW,r.y0+j+pw*2,r.x1-1,r.y0+j+pw*2,col);
    }

    if(oi.Style & sOIS_LOAD)
    {
      col = sGui->Palette[sel?sGC_LOW2:sGC_HIGH2];
      for(j=4;j>=0;j--)
      {
        sPainter->Line(r.x0+j,r.y0,r.x0,r.y0+j,col);
        sPainter->Line(r.x1-j-1,r.y0,r.x1-1,r.y0+j,col);
        col = sGui->Palette[sGC_BACK];
      }
      sPainter->Line(r.x0,r.y0,r.x0,r.y0+5-2,col);
      sPainter->Line(r.x1-1,r.y0,r.x1-1,r.y0+5-2,col);
      col = sGui->Palette[sel?sGC_LOW:sGC_HIGH]; j=4;
      sPainter->Line(r.x0+1,r.y0+j,r.x0+j,r.y0+1,col);
      sPainter->Line(r.x1-2,r.y0+j,r.x1-j-1,r.y0+1,col);
    }

    if(oi.Style & sOIS_STORE)
    {
      col = sGui->Palette[sel?sGC_HIGH2:sGC_LOW2];
      for(j=4;j>=0;j--)
      {
        sPainter->Line(r.x0,r.y1-1-j,r.x0+j,r.y1-1,col);
        sPainter->Line(r.x1-1,r.y1-1-j,r.x1-j-1,r.y1-1,col);
        col = sGui->Palette[sGC_BACK];
      }
      sPainter->Line(r.x0,r.y1-1,r.x0,r.y1-5+2,col);
      sPainter->Line(r.x1-1,r.y1-1,r.x1-1,r.y1-5+2,col);
      col = sGui->Palette[sel?sGC_HIGH:sGC_LOW]; j = 4;
      sPainter->Line(r.x0+1,r.y1-1-j,r.x0+j,r.y1-2,col);
      sPainter->Line(r.x1-2,r.y1-1-j,r.x1-j-1,r.y1-2,col);
    }


    sPainter->Flush();
    sPainter->SetClipping(ClientClip);
    sPainter->EnableClipping(sTRUE);
    // paint shadow for dragging

  }

  shadow = 0;
  if(DragMode==DM_MOVE||DragMode==DM_WIDTH)
    shadow = 1;
  if(DragMode==DM_DUPLICATE)
    shadow = 2;
  if(shadow)
  {
    for(i=0;i<max;i++)
    {
      GetOpInfo(i,oi);
      if(oi.Style & sOIS_SELECT)
      {
        r.x0 = Client.x0 + oi.PosX*PageX;
        r.y0 = Client.y0 + oi.PosY*PageY;
        r.x1 = Client.x0 + (oi.PosX+oi.Width)*PageX;
        r.y1 = Client.y0 + (oi.PosY+1)*PageY;

        r.x0 += DragMoveX*PageX;
        r.y0 += DragMoveY*PageY;
        r.x1 += (DragMoveX+DragWidth)*PageX;
        r.y1 += DragMoveY*PageY;

        sGui->RectHL(r,shadow,sGui->Palette[sGC_DRAW],sGui->Palette[sGC_DRAW]);
      }
    }
  }

  if(DragMode==DM_RECT)
  {
    sGui->RectHL(DragRect,1,sGui->Palette[sGC_DRAW],sGui->Palette[sGC_DRAW]);
  }


}

/****************************************************************************/

void sOpWindow::OnDrag(sDragData &dd)
{
  sOpInfo oi;
  sBool found;
  sInt max;
  sRect r;
  sBool pickit;
  sInt mode;
  sU32 keyq;
  sInt cellx,celly;

  max = GetOpCount();

  if(MMBScrolling(dd,DragStartX,DragStartY)) return;

  cellx = sRange<sInt>((dd.MouseX-Client.x0)/PageX,PageMaxX-1,0);
  celly = sRange<sInt>((dd.MouseY-Client.y0)/PageY,PageMaxY-1,0);

  switch(dd.Mode)
  {
  case sDD_START:
    DragMouseX = dd.MouseX;
    DragMouseY = dd.MouseY;
    DragClickX = cellx;
    DragClickY = celly;


    if(Birdseye)
    {
      DragMode = DM_BIRDSEYE;
      if(!BirdView.Hit(dd.MouseX,dd.MouseY))
      {
        ScrollX = (dd.MouseX-Bird.x0)*PageX/BXSIZE - (ClientClip.x1-ClientClip.x0)/2;
        ScrollY = (dd.MouseY-Bird.y0)*PageY/BYSIZE - (ClientClip.y1-ClientClip.y0)/2;
        ScrollX = sRange<sF32>(ScrollX,PageMaxX*PageX - (ClientClip.x1-ClientClip.x0),0);
        ScrollY = sRange<sF32>(ScrollY,PageMaxY*PageY - (ClientClip.y1-ClientClip.y0),0);
      }
      DragStartX = ScrollX;
      DragStartY = ScrollY;
      break;
    }

    mode = 0;
    keyq = sSystem->GetKeyboardShiftState();
    if(dd.Buttons& 1) mode = 1;   // lmb
    if(dd.Buttons& 2) mode = 2;   // rmb
    if(dd.Buttons& 4) mode = 3;   // mmb
    if(dd.Buttons& 8) mode = 4;   // xmb
    if(dd.Buttons&16) mode = 5;   // ymb
    if(keyq&sKEYQ_CTRL)
      mode |= 0x10;               // ctrl
    else if(keyq&sKEYQ_SHIFT)
      mode |= 0x20;               // shift

    found = FindOp(dd.MouseX,dd.MouseY,oi);
    if(found)
    {
      MakeRect(oi,r);
      if(dd.MouseX>=r.x1-PAGEW)
        mode |= 0x80;             // select at width
      mode |= 0x40;               // select at body
    }

    CursorX = sRange(cellx-1,PageMaxX-3,0);
    CursorY = celly;
    CursorWidth = 3;
    pickit = 0;
    DragMode = 0;
    SelectMode = sOPSEL_SET;

    switch(mode&0x3f)
    {
    case 0x01:
      if(dd.Flags&sDDF_DOUBLE)  { sGui->Post(sOIC_SHOW,this); break; }
      DragMode = DM_RECT;
      if(mode&0x40)
      {
        DragMode = DM_PICK;
        if(found && oi.Style&sOIS_SELECT)
          SelectMode = sOPSEL_ADD;
      }
      if(mode&0x80)
      {
        DragMode = DM_WIDTH;
      }
      break;
    case 0x02:
      if(dd.Flags&sDDF_DOUBLE)  { sGui->Post(sOIC_ADD,this); break; }
      DragMode = DM_WIDTH;
      break;
    case 0x03:
      DragMode = DM_SCROLL;
      break;

    case 0x11:
      DragMode = DM_RECT;
      if(mode&0xc0) DragMode = DM_DUPLICATE;
      break;
    case 0x12:
      DragMode = DM_WIDTH;
      break;
    case 0x13:
      DragMode = DM_SCROLL;
      break;

    case 0x21:
      SelectMode = sOPSEL_TOGGLE;
      DragMode = DM_RECT;
      if(mode&0xc0) DragMode = DM_PICK;
      break;
    case 0x22:
      SelectMode = sOPSEL_ADD;
      DragMode = DM_RECT;
      if(mode&0xc0) DragMode = DM_PICK;
      break;
    case 0x23:
      DragMode = DM_SCROLL;
      break;

    case 0x04:
      DragMode = DM_DUPLICATE;
      break;

    case 0x05:
      sGui->Post(sOIC_ADD,this);
      break;
    }

    if(DragMode==DM_DUPLICATE || DragMode==DM_WIDTH || DragMode==DM_MOVE)
    {
      if(found && !(oi.Style&sOIS_SELECT))
      {
        pickit = 1;
        SelectMode = sOPSEL_SET;
      }
    }

    DragStartX = dd.MouseX;
    DragStartY = dd.MouseY;
    DragMoveX = 0;
    DragMoveY = 0;
    DragWidth = 0;

    if(DragMode==DM_PICK || pickit)
    {
      r.x0 = cellx;
      r.y0 = celly;
      r.x1 = r.x0+1;
      r.y1 = r.y0+1;
      SelectRect(r,SelectMode);
      sGui->Post(sOIC_SETEDIT,this);
      /*
      EditOp = po;
      */
    }

    if(DragMode==DM_RECT)
    {
      DragRect.Init(dd.MouseX,dd.MouseY,dd.MouseX+1,dd.MouseY+1);
      sGui->Post(sOIC_SETEDIT,this);
      /*
      EditOp = 0;
      */
    }
    if(DragMode==DM_SCROLL)
    {
      DragStartX = ScrollX;
      DragStartY = ScrollY;
    }
    break;

  case sDD_DRAG:
    switch(DragMode)
    {
    case DM_PICK:
      if((sAbs(dd.MouseX-DragMouseX)>2 || sAbs(dd.MouseY-DragMouseY)>2) && dd.Buttons==1)
        DragMode = DM_MOVE;
      break;
    case DM_RECT:
      DragRect.Init(dd.MouseX,dd.MouseY,DragStartX,DragStartY);
      DragRect.Sort();
      break;
    case DM_MOVE:
    case DM_DUPLICATE:
      DragMoveX = ((dd.MouseX-DragMouseX)+1024*PageX+PageX/2)/PageX-1024;
      DragMoveY = ((dd.MouseY-DragMouseY)+1024*PageY+PageY/2)/PageY-1024;
      break;
    case DM_WIDTH:
      DragWidth = ((dd.MouseX-DragMouseX)+1024*PageX+PageX/2)/PageX-1024;
      break;
    case DM_SCROLL:
      ScrollX = sRange<sInt>(DragStartX-(dd.MouseX-DragMouseX),SizeX-RealX,0);
      ScrollY = sRange<sInt>(DragStartY-(dd.MouseY-DragMouseY),SizeY-RealY,0);
      break;
    case DM_BIRDSEYE:
      ScrollX = DragStartX + (dd.MouseX-DragMouseX)*PageX/BXSIZE;
      ScrollY = DragStartY + (dd.MouseY-DragMouseY)*PageY/BYSIZE;
      break;
    }
    break;

  case sDD_STOP:
    switch(DragMode)
    {
    case DM_RECT:
      r.x0 = DragClickX;
      r.y0 = DragClickY;
      r.x1 = cellx;
      r.y1 = celly;
      r.Sort();
      r.x1++;
      r.y1++;
      SelectRect(r,SelectMode);
      break;
    case DM_MOVE:
    case DM_WIDTH:
      if(CheckDest(sFALSE))
      {
        MoveDest(sFALSE);
        sGui->Post(sOIC_CHANGED,this);
      }
      break;
    case DM_DUPLICATE:
      if(CheckDest(sTRUE))
      {
        MoveDest(sTRUE);
        sGui->Post(sOIC_CHANGED,this);
      }
      break;
    }
    DragMode = 0;
    DragMoveX = 0;
    DragMoveY = 0;
    DragWidth = 0;
    break;
  }
}

/****************************************************************************/
/****************************************************************************/

sInt sOpWindow::GetOpCount()
{
  return 0;
}

void sOpWindow::GetOpInfo(sInt i,sOpInfo &oi)
{
  oi.PosX = 0;
  oi.PosY = 0;
  oi.Width = 3;
  oi.Style = 0;
  oi.SetName("???");
  oi.Color = 0;
  oi.Wheel = -1;
}

void sOpWindow::SelectRect(sRect cells,sInt mode)
{
}

void sOpWindow::MoveDest(sBool dup)
{
}

/****************************************************************************/

void sOpWindow::MakeRect(const sOpInfo &oi,sRect &r)
{
  r.x1 = Client.x0 + (oi.PosX+oi.Width)*PageX;
  r.y1 = Client.y0 + (oi.PosY+1)*PageY;
  r.x0 = Client.x0 + oi.PosX*PageX;
  r.y0 = Client.y0 + oi.PosY*PageY;
}

sBool sOpWindow::FindOp(sInt mousex,sInt mousey,sOpInfo &oi)
{
  sInt i,max;
  sRect r;

  max = GetOpCount();
  for(i=0;i<max;i++)
  {
    GetOpInfo(i,oi);
    MakeRect(oi,r);
    if(r.Hit(mousex,mousey))
      return sTRUE;
  }

  return sFALSE;
}

sBool sOpWindow::CheckDest(sInt x,sInt y,sInt w)
{
  sInt i,max;
  sOpInfo oi;

  max = GetOpCount();
  for(i=0;i<max;i++)
  {
    GetOpInfo(i,oi);
    if(oi.PosY==y && oi.PosX<x+w && oi.PosX+oi.Width>x)
      return sFALSE;
  }
  return sTRUE;
}

sBool sOpWindow::CheckDest(sBool dup)
{
  sInt i,max,j;
  sInt x,y,w;
  sOpInfo oi;
  sU8 *map;
  
  map = new sU8[PageMaxX*PageMaxY];
  sSetMem(map,0,PageMaxX*PageMaxY);
  max = GetOpCount();
  for(i=0;i<max;i++)
  {
    GetOpInfo(i,oi);
    x = oi.PosX;
    y = oi.PosY;
    w = oi.Width;

    if(oi.Style & sOIS_SELECT)
    {
      if(dup)
      {
        for(j=0;j<w;j++)
        {
          if(map[y*PageMaxX+x+j])
          {
            delete[] map;
            return sFALSE;
          }
          map[y*PageMaxX+x+j] = 1;
        }
      }
      x = sRange(x+DragMoveX,PageMaxX-w-1,0);
      y = sRange(y+DragMoveY,PageMaxY  -1,0);
      w = sRange(w+DragWidth,PageMaxX-x-1,1);
    }
    for(j=0;j<w;j++)
    {
      if(map[y*PageMaxX+x+j])
      {
        delete[] map;
        return sFALSE;
      }
      map[y*PageMaxX+x+j] = 1;
    }
  }

  delete[] map;
  return sTRUE;
}

/****************************************************************************/
/***                                                                      ***/
/***   Report Window                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sReportWindow::sReportWindow()
{
  Height = sPainter->GetHeight(sGui->FixedFont);
  AddScrolling(0,1);
  Line = 0;
  Lines = 0;
}

void sReportWindow::PrintLine(const sChar *format,...)
{
  sChar buffer[512];

  sFormatString(buffer,512,format,&format);
  sPainter->Print(sGui->FixedFont,Client.x0+5,Client.y0+5+Line*Height,buffer,sGui->Palette[sGC_TEXT]);
  Line++;
}

void sReportWindow::PrintGroup(sChar *label)
{
  sRect r;
  
  r.x0 = Client.x0+5;
  r.y0 = Client.y0+5+Line*Height;
  r.x1 = Client.x1-5;
  r.y1 = r.y0 + Height;

  sGui->Group(r,label);
  Line++;
}

void sReportWindow::OnPaint()
{
  Height = sPainter->GetHeight(sGui->FixedFont);
  ClearBack(sGC_BUTTON);
  Line = 0;
  Print();
  Lines = Line;
  SizeY = Height * Lines + 10;
}

void sReportWindow::OnCalcSize()
{
  SizeY = Height * Lines + 10;
  SizeX = 0;
}

void sReportWindow::OnDrag(sDragData &dd)
{
  MMBScrolling(dd,DragX,DragY);
}

void sReportWindow::Print()
{
  PrintGroup("derive and conquer");
}

/****************************************************************************/

sLogWindow::sLogWindow(sInt buffermax,sInt linemax)
{
  Line = new sChar*[linemax];
  LineMax = linemax;
  LineUsed = 0;
  Buffer = new sChar[buffermax];
  BufferMax = buffermax;
  BufferUsed = 0;

  Line[0] = Buffer;
  AddScrolling();
}

sLogWindow::~sLogWindow()
{
  delete[] Line;
  delete[] Buffer;
}

void sLogWindow::RemLine()
{
  sInt i,l;
  sChar *s,*d;

  sVERIFY(LineUsed>=2)
  sVERIFY(Line[0] == Buffer);
  d = Line[0];
  s = Line[1];
  l = s-d;
  sVERIFY(l>0);
  for(i=0;i<BufferUsed-l;i++)
    *d++ = *s++;
  BufferUsed -= l;
  for(i=0;i<LineUsed;i++)
    Line[i] = Line[i+1]-l;
  LineUsed--;
}

void sLogWindow::PrintLine(const sChar *string)
{
  sInt size;

  size = sGetStringLen(string)+1;
  while(BufferUsed+size>BufferMax || LineUsed+1>LineMax-1)
    RemLine();

  sVERIFY(Line[LineUsed] == Buffer+BufferUsed);
  sCopyMem(Buffer+BufferUsed,string,size);
  BufferUsed += size;
  LineUsed++;
  Line[LineUsed] = Buffer+BufferUsed;
  OnCalcSize();
  ScrollTo(0,0x7fff);
}

void sLogWindow::PrintFLine(const sChar *format,...)
{
  sChar buffer[sLW_ROWMAX];

  sFormatString(buffer,sLW_ROWMAX,format,&format);
  PrintLine(buffer);
}

void sLogWindow::OnPaint()
{
  sInt i;
  sInt x,y;

  x = Client.x0+5;
  y = Client.y0+5;
  Height = sPainter->GetHeight(sGui->FixedFont);
  ClearBack(sGC_BUTTON);

  for(i=0;i<LineUsed;i++)
  {
    if(y<ClientClip.y1 && y+Height>ClientClip.y0)
      sPainter->Print(sGui->FixedFont,x,y,Line[i],sGui->Palette[sGC_TEXT]);
    y+= Height;
  }
}

void sLogWindow::OnCalcSize()
{
  Height = sPainter->GetHeight(sGui->FixedFont);

  SizeX = 10+sPainter->GetWidth(sGui->FixedFont," ",1)*sLW_ROWMAX;
  SizeY = 10+Height*LineUsed;
}

void sLogWindow::OnDrag(sDragData &dd)
{
  MMBScrolling(dd,DragX,DragY);
}

/****************************************************************************/
/***                                                                      ***/
/***   text editor                                                        ***/
/***                                                                      ***/
/****************************************************************************/

sTextControl::sTextControl()
{
  Cursor = 0;
  Overwrite = 0;
  SelMode = 0;
  SelPos = 0;
  CursorWish = 0;

  ChangeCmd = 0;
  DoneCmd = 0;
  CursorCmd = 0;
  ReallocCmd = 0;
  MenuCmd = 0;
  Changed = 0;
  Static = 0;
  RecalcSize = 0;

//  PathName[0] = 0;
//  sCopyString(PathName,"Disks/c:/NxN",sDI_PATHSIZE);
  TextBuffer = new sText;
  Static = 1;
}

sTextControl::~sTextControl()
{
}

void sTextControl::Tag()
{
  sGuiWindow::Tag();
  sBroker->Need(TextBuffer);  
}

/****************************************************************************/
/****************************************************************************/

void sTextControl::OnCalcSize()
{
  sChar *p;
  sInt i,x,h;

  SizeX = 4;
  SizeY = 4;

  h = sPainter->GetHeight(sGui->FixedFont);
  p = TextBuffer->Text;
  for(;;)
  {
    i = 0;
    while(p[i]!=0 && p[i]!='\n')
      i++;

    SizeY+=h;
    x = sPainter->GetWidth(sGui->FixedFont,p,i)+4+sPainter->GetWidth(sGui->FixedFont," ");
    if(x>SizeX)
      SizeX=x;
    p+=i;
    if(*p==0)
      break;
    if(*p=='\n')
      p++;
  }
}

/****************************************************************************/

void sTextControl::OnPaint()
{
  sChar *p;
  sInt i,x,y,h,xs;
  sInt pos;
  sRect r;
  sInt font;
  sInt s0,s1;

  if(RecalcSize) // usefull for Logwindow
  {
    RecalcSize = 0;
    OnCalcSize();
  }

  x = Client.x0+2;
  y = Client.y0+2;
  font = sGui->FixedFont;
  h = sPainter->GetHeight(font);
  p = TextBuffer->Text;
  pos = 0;

  s0 = s1 = 0;
  if(SelMode)
  {
    s0 = sMin(Cursor,SelPos);
    s1 = sMax(Cursor,SelPos);
  }

  r.y0 = y-2;
  r.y1 = y;
  r.x0 = Client.x0;
  r.x1 = Client.x1;
  sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_BACK]);

  for(;;)
  {
    i = 0;
    while(p[i]!=0 && p[i]!='\n')
      i++;

    r.y0 = y;
    r.y1 = y+h;
    r.x0 = Client.x0;
    r.x1 = Client.x1;
    if(sGui->CurrentClip.Hit(r))
    {
      sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_BACK]);
      if(Cursor>=pos && Cursor<=pos+i)
      {
        r.x0 = x+sPainter->GetWidth(font,p,Cursor-pos);
        if(Overwrite)
        {
          if(Cursor==pos+i)
            r.x1 = x+sPainter->GetWidth(font," ");
          else
            r.x1 = x+sPainter->GetWidth(font,p,Cursor-pos+1);
          sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_SELBACK]);
        }
        else
        {
          r.x1 = r.x0+1;
          r.x0--;
          sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_TEXT]);
        }
      }
      if(SelMode && s0<=pos+i && s1>=pos)
      {
        if(s0<=pos)
          r.x0 = x;
        else
          r.x0 = x+sPainter->GetWidth(font,p,s0-pos);
        if(s1>pos+i)
          r.x1 = x+sPainter->GetWidth(font,p,i)+sPainter->GetWidth(font," ");
        else
          r.x1 = x+sPainter->GetWidth(font,p,s1-pos);
          sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_SELBACK]);
      }
      sPainter->Print(font,x,y,p,sGui->Palette[sGC_TEXT],i);
    }
    xs = sPainter->GetWidth(font,p,i)+4+sPainter->GetWidth(font," ");
    y+=h;
    p+=i;
    pos+=i;
    if(*p==0)
      break;
    if(*p=='\n')
    {
      p++;
      pos++;
    }
  }
  r.y0 = y;
  r.y1 = Client.y1;
  r.x0 = Client.x0;
  r.x1 = Client.x1;
  if(r.y0<r.y1)
    sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_BACK]);
}

/****************************************************************************/

void sTextControl::OnKey(sU32 key)
{
  sInt len;
  sU32 ckey;
  sInt i,j;
  sChar buffer[2];

// prepare...

  if(key&sKEYQ_SHIFT) key|=sKEYQ_SHIFT;
  if(key&sKEYQ_CTRL ) key|=sKEYQ_CTRL ;

  len = sGetStringLen(TextBuffer->Text);
  if(Cursor>len)
    Cursor = len;

// normal keys

  switch(key&(0x8001ffff|sKEYQ_SHIFT|sKEYQ_CTRL))
  {
  case sKEY_BACKSPACE:
    if(SelMode)
    {
      DelSel();
    }
    else
    {
      if(Cursor==0)
        break;
      Cursor--;
      Engine(Cursor,1,0);
      SelMode = 0;
      OnCalcSize();
      ScrollToCursor();
    }
    break;
  case sKEY_DELETE:
    if(SelMode)
    {
      DelSel();
    }
    else
    {
      Engine(Cursor,1,0);
      OnCalcSize();
    }
    ScrollToCursor();
    break;
  case sKEY_ENTER:
    if(SelMode)
      DelSel();
    i = Cursor-GetCursorX();
    for(j=0;i<Cursor && TextBuffer->Text[i]==' ';i++)
      j++;
    if(Cursor>i && TextBuffer->Text[Cursor-1]=='{')
      j+=2;
    Engine(Cursor,1,"\n");
    Cursor++;
    for(i=0;i<j;i++)
    {
      Engine(Cursor,1," ");
      Cursor++;
    }
    OnCalcSize();
    ScrollToCursor();
    break;
  case sKEY_PAGEUP:
    len = RealY/sPainter->GetHeight(sGui->FixedFont)-8;
    if(len<1) len = 1;
    for(i=0;i<len;i++)
      OnKey(sKEY_UP);
    break;
  case sKEY_PAGEDOWN:
    len = RealY/sPainter->GetHeight(sGui->FixedFont)-8;
    if(len<1) len = 1;
    for(i=0;i<len;i++)
      OnKey(sKEY_DOWN);
    break;
  case sKEY_INSERT:
    Overwrite = !Overwrite;
    ScrollToCursor();
    break;
  case 'x'|sKEYQ_CTRL:
    CmdCut();;
    OnCalcSize();
    ScrollToCursor();
    break;
  case 'c'|sKEYQ_CTRL:
    CmdCopy();
    ScrollToCursor();
    break;
  case 'v'|sKEYQ_CTRL:
    CmdPaste();
    OnCalcSize();
    ScrollToCursor();
    break;
  case 'b'|sKEYQ_CTRL:
    CmdBlock();
    ScrollToCursor();
    break;
  }

//  sDPrintF("key %08x\n",key);
  ckey = key&~(sKEYQ_SHIFT|sKEYQ_ALTGR|sKEYQ_REPEAT);
  if((ckey>=0x20 && ckey<=0x7e) || (ckey>=0xa0 && ckey<=0xff))
  {
    DelSel();
    buffer[0] = ckey;
    buffer[1] = 0;
    if(Overwrite && Cursor<len)
    {
      Engine(Cursor,1,0);
      Engine(Cursor,1,buffer);
      Cursor++;
    }
    else
    {
      Engine(Cursor,1,buffer);
      Cursor++;
    } 
    OnCalcSize();
    ScrollToCursor();
  }
  else
  {
    Parent->OnKey(key);
  }

// cursor movement and shift-block-marking

  switch(key&0x8001ffff)
  {
  case sKEY_LEFT:
  case sKEY_RIGHT:
  case sKEY_UP:
  case sKEY_DOWN:
  case sKEY_HOME:
  case sKEY_END:
    if(SelMode==0 && (key&sKEYQ_SHIFT))
    {
      SelMode = 1;
      SelPos = Cursor;
    }
    if(SelMode==1 && !(key&sKEYQ_SHIFT))
    {
      SelMode = 0;
    }
    break;
  }

  switch(key&0x8001ffff)
  {
  case sKEY_LEFT:
    if(Cursor>0)
      Cursor--;
    ScrollToCursor();
    break;
  case sKEY_RIGHT:
    if(Cursor<len)
      Cursor++;
    ScrollToCursor();
    break;
  case sKEY_UP:
    j = i = CursorWish;
    if(TextBuffer->Text[Cursor]=='\n' && Cursor>0)
      Cursor--;
    while(TextBuffer->Text[Cursor]!='\n' && Cursor>0)
      Cursor--;
    while(TextBuffer->Text[Cursor-1]!='\n' && Cursor>0)
      Cursor--;
    while(i>0 && TextBuffer->Text[Cursor]!='\n' && TextBuffer->Text[Cursor]!=0)
    {
      Cursor++;
      i--;
    }
    ScrollToCursor();
    CursorWish = j;
    break;
  case sKEY_DOWN:
    j = i = CursorWish;
    while(TextBuffer->Text[Cursor]!='\n' && TextBuffer->Text[Cursor]!=0)
      Cursor++;
    if(TextBuffer->Text[Cursor]=='\n')
    {
      Cursor++;
      while(i>0 && TextBuffer->Text[Cursor]!='\n' && TextBuffer->Text[Cursor]!=0)
      {
        Cursor++;
        i--;
      }
    }
    ScrollToCursor();
    CursorWish = j;
    break;
  case sKEY_HOME:
    while(Cursor>0 && TextBuffer->Text[Cursor-1]!='\n')
      Cursor--;
    ScrollToCursor();
    break;
  case sKEY_END:
    while(TextBuffer->Text[Cursor]!='\n' && TextBuffer->Text[Cursor]!=0)
      Cursor++;
    ScrollToCursor();
    break;
  }
}

/****************************************************************************/

void sTextControl::OnDrag(sDragData &dd)
{
  if(dd.Mode==sDD_START && (dd.Buttons&4) && dd.Flags&sDDF_DOUBLE) 
  {
    Post(MenuCmd);
    return;
  }
  if(MMBScrolling(dd,MMBX,MMBY)) return;

  switch(dd.Mode)
  {
  case sDD_START:
    Cursor = SelPos = ClickToPos(dd.MouseX,dd.MouseY);
    CursorWish = GetCursorX();
    Post(CursorCmd);
    SelMode = 1;
    break;
  case sDD_DRAG:
    Cursor = ClickToPos(dd.MouseX,dd.MouseY);
    CursorWish = GetCursorX();
    Post(CursorCmd);
    break;
  case sDD_STOP:
    if(Cursor==SelPos)
      SelMode = 0;
    break;
  }
}

/****************************************************************************/
/****************************************************************************/

void sTextControl::SetText(sChar *t)
{
  sChar *d;

  Realloc(sGetStringLen(t)+1);

  d = TextBuffer->Text;
  while(*t)
  {
    if(*t!='\r')
      *d++ = *t;
    t++;
  }
  *d++ = 0;
  Static = 0;
  Cursor = 0;
  SelPos = 0;
  SelMode = 0;
  OnCalcSize();
}

void sTextControl::SetText(sText *t)
{
  TextBuffer = t;
  Static = 0;
  Cursor = 0;
  SelPos = 0;
  SelMode = 0;
  OnCalcSize();
}


/****************************************************************************/

sChar *sTextControl::GetText()
{
  return TextBuffer->Text;
}

/****************************************************************************/

void sTextControl::Engine(sInt pos,sInt count,sChar *insert)
{
  sInt len;
  sInt i;

  if(Static)
    return;
  Post(DoneCmd);
  Post(ChangeCmd);
  Changed = 1;

  len = sGetStringLen(TextBuffer->Text);
  if(insert)
  {
    Realloc(len+count+1);
    for(i=len;i>=pos;i--)
      TextBuffer->Text[i+count] = TextBuffer->Text[i];
    sCopyMem(TextBuffer->Text+pos,insert,count);
  }
  else
  {
    if(pos+count<=len)
      sCopyMem(TextBuffer->Text+pos,TextBuffer->Text+pos+count,len-pos-count+1);
  }
  TextBuffer->FixUsed();
}

void sTextControl::DelSel()
{
  sInt s0,s1,len;
  if(SelMode)
  {
    s0 = sMin(SelPos,Cursor);
    s1 = sMax(SelPos,Cursor);
    len = s1-s0;
    if(len>0)
    {
      Engine(s0,len,0);
      Cursor = s0;
      SelMode = 0;
    }
  }
}

/****************************************************************************/
/****************************************************************************/

sInt sTextControl::GetCursorX()
{
  sInt x;
  sInt i;

  i = Cursor-1;
  x = 0;
  while(i>=0 && TextBuffer->Text[i]!='\n')
  {
    i--;
    x++;
  }

  return x;
}

/****************************************************************************/

sInt sTextControl::GetCursorY()
{
  sInt i;
  sInt y;

  y=0;
  for(i=0;i<Cursor;i++)
    if(TextBuffer->Text[i]<0x20)
      y++;

  return y;
}

/****************************************************************************/

void sTextControl::SetCursor(sInt x,sInt y)
{
  sInt i;

  i = 0;
  while(y>0)
  {
    if(TextBuffer->Text[i]==0)
      return;
    if(TextBuffer->Text[i]=='\n')
      y--;
    i++;
  }
  while(x>0)
  {
    if(TextBuffer->Text[i]==0)
      return;
    if(TextBuffer->Text[i]=='\n')
      y--;
    i++;
  }
  Cursor = i;
  CursorWish = x;
  SelMode = 0;
  ScrollToCursor();
}

/****************************************************************************/

void sTextControl::ScrollToCursor()
{
  sInt i,x,y,h,pos;
  sRect r;

  Post(CursorCmd);
  CursorWish = GetCursorX();

  h = sPainter->GetHeight(sGui->FixedFont);
  y = Client.y0+2;
  x = Client.x0+2;
  pos = 0;
  for(;;)
  {
    i = 0;
    while(TextBuffer->Text[pos+i]!=0 && TextBuffer->Text[pos+i]!='\n')
      i++;
    if(Cursor>=pos && Cursor<=pos+i)
    {
      r.x0 = x+sPainter->GetWidth(sGui->FixedFont,TextBuffer->Text+pos,Cursor-pos);
      r.y0 = y;
      r.x1 = r.x0+1;
      r.y1 = y+h;
      ScrollTo(r,sGWS_SAFE);
      return;
    }
    y+=h;
    pos+=i;
    if(TextBuffer->Text[pos]==0)
      break;
    if(TextBuffer->Text[pos]=='\n')
      pos++;
  }
}

/****************************************************************************/

sInt sTextControl::ClickToPos(sInt mx,sInt my)
{
  sInt i,j,x,y,h,pos;

  h = sPainter->GetHeight(sGui->FixedFont);
  y = Client.y0+2;
  x = Client.x0+2;
  pos = 0;
  for(;;)
  {
    i = 0;
    while(TextBuffer->Text[pos+i]!=0 && TextBuffer->Text[pos+i]!='\n')
      i++;
    if(my<y+h)
    {
      for(j=1;j<i;j++)
      {
        if(mx<Client.x0+2+sPainter->GetWidth(sGui->FixedFont,TextBuffer->Text+pos,j))
          return pos+j-1;
      }
      return pos+i;
    }
    y+=h;
    pos+=i;
    if(TextBuffer->Text[pos]==0)
      break;
    if(TextBuffer->Text[pos]=='\n')
      pos++;
  }
  return sGetStringLen(TextBuffer->Text);
}

/****************************************************************************/

void sTextControl::SetSelection(sInt cursor,sInt sel)
{
  sInt len;

  len = sGetStringLen(TextBuffer->Text);
  if(cursor>=0 && sel>=0 && cursor<=len && sel<=len)
  {
    Cursor = cursor;
    SelPos = sel;
    SelMode = 1;
    ScrollToCursor();
  }
}

/****************************************************************************/

void sTextControl::Realloc(sInt size)
{
  if(TextBuffer->Alloc<size)
  {
    TextBuffer->Realloc(sMax(size*3/2,TextBuffer->Alloc*2));
    Post(ReallocCmd);
  }
}

/****************************************************************************/

void sTextControl::Log(sChar *s)
{
  sInt tl,sl,stat;

  stat = Static;
  Static = 0;
  tl = sGetStringLen(TextBuffer->Text);
  sl = sGetStringLen(s);
  Engine(tl,sl,s);
  Cursor = tl+sl;
  ScrollToCursor();
  Static = stat;
  RecalcSize = sTRUE;
}

/****************************************************************************/

void sTextControl::ClearText()
{
  TextBuffer->Text[0] = 0;
  Cursor = 0;
}

/****************************************************************************/

void sTextControl::CmdCut()
{
  sInt s0,s1,len;

  s0 = sMin(SelPos,Cursor);
  s1 = sMax(SelPos,Cursor);
  len = s1-s0;

  if(SelMode && s0!=s1)
  {
    sGui->ClipboardClear();
    sGui->ClipboardAddText(TextBuffer->Text+s0,len);
    Engine(s0,len,0);
    SelMode = 0;
    Cursor = s0;
  }
}

/****************************************************************************/

void sTextControl::CmdCopy()
{
  sInt s0,s1,len;

  s0 = sMin(SelPos,Cursor);
  s1 = sMax(SelPos,Cursor);
  len = s1-s0;

  if(SelMode && s0!=s1)
  {
    sGui->ClipboardClear();
    sGui->ClipboardAddText(TextBuffer->Text+s0,len);
    SelMode = 0;
  }
}

/****************************************************************************/

void sTextControl::CmdPaste()
{
  sChar *t;

  t = sGui->ClipboardFindText();
  if(t && t[0])
  {
    Engine(Cursor,sGetStringLen(t),t);
    Cursor+=sGetStringLen(t);
  }
}

/****************************************************************************/

void sTextControl::CmdBlock()
{
  if(SelMode==0)
  {
    SelMode = 2;
    SelPos = Cursor;
  }
  else
  {
    SelMode = 0;
  }  
}

/****************************************************************************/
/****************************************************************************/
