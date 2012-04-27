/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#define MULTISAMPLING 1

/****************************************************************************/

#include "selector_win.hpp"
#include "wz4lib/doc.hpp"

#if sPLATFORM==sPLAT_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

extern HWND sHWND;

/****************************************************************************/

namespace bob
{
  static WNDCLASSEX SelWinClass;
  static WNDCLASSEX LinkClass;
  static HWND SelWin;

  static sBool Done;
  static const bSelectorSetup *Setup;
  static bSelectorResult *Result;
  static sScreenInfo *SI;

  static sArray<sScreenInfoXY> Aspects;

  // size of dialog window
  //static const sInt H=160;

  // dialog window members
  static HFONT CaptionFont;
  static HICON LinkIcon;
  static HWND ResBox, AspectBox, FSAABox, FSCheck, VSCheck, LoopCheck, BenchmarkCheck, Die, Demo,CoreCheck,LowQCheck,HiddenPartBox;


  struct Link
  {
    HWND Wnd;
    HICON Icon;

    Link() : Wnd(0), Icon(0) {}
  };
  sArray<Link> Links;

  // reads controls and fills out results struct
  static void FillResults()
  {
    sScreenMode &m=Result->Mode;

    sInt res=SendMessage(ResBox,CB_GETCURSEL,0,0);
    sInt asp=SendMessage(AspectBox,CB_GETCURSEL,0,0);
    sInt fsaa = 0;
    if(MULTISAMPLING)
      fsaa=SendMessage(FSAABox,CB_GETCURSEL,0,0);
    
    sBool fullscreen=SendMessage(FSCheck,BM_GETCHECK,0,0)==BST_CHECKED;
    sBool vsync=SendMessage(VSCheck,BM_GETCHECK,0,0)==BST_CHECKED;
    sBool loop=SendMessage(LoopCheck,BM_GETCHECK,0,0)==BST_CHECKED;
    sBool core = SendMessage(CoreCheck,BM_GETCHECK,0,0)==BST_CHECKED;
    sBool lowq = SendMessage(LowQCheck,BM_GETCHECK,0,0)==BST_CHECKED;
    sBool benchmark=sFALSE;
    if (Setup->DialogFlags & wDODF_Benchmark)
      benchmark=SendMessage(BenchmarkCheck,BM_GETCHECK,0,0)==BST_CHECKED;
    //sBool mblur=SendMessage(MBlurCheck,BM_GETCHECK,0,0)==BST_CHECKED;
    sInt hp = SendMessage(HiddenPartBox,CB_GETCURSEL,0,0);

    m.Display=-1; // use default
    m.ScreenX=SI->Resolutions[res].x;
    m.ScreenY=SI->Resolutions[res].y;

    if (asp)
      m.Aspect=sF32(Aspects[asp-1].x)/sF32(Aspects[asp-1].y);
    else
      m.Aspect=sF32(m.ScreenX)/sF32(m.ScreenY);

    m.MultiLevel=fsaa-1;

    m.Flags=sSM_VALID;
    if (fullscreen) m.Flags|=sSM_FULLSCREEN;
    if (!vsync) m.Flags|=sSM_NOVSYNC;

    Result->Loop=loop;
    Result->Benchmark=benchmark;
    Result->OneCoreForOS = core;
    Result->LowQuality = lowq;
    Result->HiddenPart = hp-1;

    Done=1;
  }



  static HWND AddStatic(const sChar *text,sInt x,sInt y,sInt w, sBool caption=sFALSE, sBool disabled=sFALSE)
  {
    sInt h=caption?30:20;
    HWND wnd=CreateWindowEx(0,L"STATIC",text,WS_VISIBLE|WS_CHILD|(disabled?WS_DISABLED:0),x,y,w,h,SelWin,0,0,0);
    SendMessage(wnd,WM_SETFONT,(WPARAM)(caption?CaptionFont:GetStockObject(DEFAULT_GUI_FONT)),0);
    return wnd;
  }

  static HWND AddButton(const sChar *text,sInt x,sInt y, sInt w,sBool deflt)
  {
    if (!w) w=80;

    sInt style=WS_VISIBLE|WS_CHILD|WS_TABSTOP;
    if (deflt) style|=BS_DEFPUSHBUTTON; else style|=BS_PUSHBUTTON;

    HWND wnd=CreateWindowEx(0,L"BUTTON",text,style,x,y,w,25,SelWin,0,0,0);
    SendMessage(wnd,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
    if (deflt) SetFocus(wnd);
    return wnd;
  }

  static HWND AddGroup(const sChar *text,sInt x,sInt y, sInt w,sInt h)
  {
    sInt style=WS_VISIBLE|WS_CHILD|BS_GROUPBOX;
    HWND wnd=CreateWindowEx(0,L"BUTTON",text,style,x,y,w,h,SelWin,0,0,0);
    SendMessage(wnd,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
    return wnd;
  }

  static Link& AddLink(const sChar *text,sInt x,sInt y, sInt w, sInt h)
  {
    if (!w) w=32;
    if (!h) h=w;

    sInt style=WS_VISIBLE|WS_CHILD|BS_OWNERDRAW;
    HWND wnd=CreateWindowEx(0,L"bLink",text,style,x,y,w,h,SelWin,0,0,0);

    Link l;
    l.Wnd=wnd;
    Links.AddTail(l);

    return Links[Links.GetCount()-1];
  }

  static HWND AddCheck(const sChar *text,sInt x,sInt y, sInt w, sInt state)
  {
    sInt style=WS_VISIBLE|WS_CHILD|WS_TABSTOP;
    style|=BS_AUTOCHECKBOX ;
    HWND wnd=CreateWindowEx(0,L"BUTTON",text,style,x,y,w,22,SelWin,0,0,0);
    SendMessage(wnd,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);
    SendMessage(wnd,BM_SETCHECK,state,0);
    return wnd;
  }

  static HWND AddComboBox(const sChar *choices, sInt x, sInt y, sInt w, sInt sel)
  {
    sInt style=WS_VISIBLE|WS_CHILD|WS_TABSTOP|WS_VSCROLL;
    style|=CBS_DROPDOWNLIST;
    
    HWND wnd=CreateWindowEx(0,L"COMBOBOX",L"",style,x,y,w,125,SelWin,0,0,0);
    SendMessage(wnd,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),0);

    while (choices && choices[0])
    {
      sString<64> opt;
      sInt pipe=sFindFirstChar(choices,'|');
      if (pipe>0)
      {
        sCopyString(opt,choices,pipe+1);
        choices+=pipe+1;
      }
      else
      {
        sCopyString(opt,choices);
        choices+=sGetStringLen(choices);
      }
      SendMessage(wnd,CB_ADDSTRING,0,(LPARAM)&opt[0]);
    }

    SendMessage(wnd,CB_SETCURSEL,sel,0);

    return wnd;
  }


  static void OpenURL(HWND from)
  {
    sString<1024> url;
    GetWindowText(from,url,1024);

    ShellExecute(NULL,NULL,url,NULL,NULL,SW_SHOW);
  }
 

  static LRESULT CALLBACK SelWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {

    switch (uMsg)
    {
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    case WM_COMMAND:
      {
        HWND from=(HWND)lParam;
        sInt code=wParam>>16;

        if (from==Die && code==BN_CLICKED)
          DestroyWindow(hwnd);

        if (from==Demo && code==BN_CLICKED)
        {
          FillResults();
          DestroyWindow(hwnd);
        }
    
        if ((Setup->DialogFlags & wDODF_Benchmark) && from==BenchmarkCheck && code==BN_CLICKED)
        {
          sBool bm=SendMessage(BenchmarkCheck,BM_GETCHECK,0,0)==BST_CHECKED;

          EnableWindow(FSCheck,!bm);
          EnableWindow(VSCheck,!bm);
          if(LoopCheck)
            EnableWindow(LoopCheck,!bm);
          if(CoreCheck)
            EnableWindow(CoreCheck,!bm);
          if(LowQCheck)
            EnableWindow(LowQCheck,!bm);
          if (bm)
          {
            SendMessage(FSCheck,BM_SETCHECK,BST_CHECKED,0);
            SendMessage(VSCheck,BM_SETCHECK,BST_UNCHECKED,0);
            if(LoopCheck)
              SendMessage(LoopCheck,BM_SETCHECK,BST_UNCHECKED,0);
            if(CoreCheck)
              SendMessage(CoreCheck,BM_SETCHECK,BST_UNCHECKED,0);
            if(LowQCheck)
              SendMessage(LowQCheck,BM_SETCHECK,BST_UNCHECKED,0);
          }
        }

        Link *l;
        sFORALL(Links, l)
          if (from==l->Wnd && code==BN_CLICKED)
            OpenURL(from);

      }
      break;
    case WM_KEYDOWN:
      {
        switch (wParam)
        {
        case VK_TAB:        
          SetFocus(GetNextDlgTabItem(SelWin,GetFocus(),GetAsyncKeyState(VK_SHIFT)&0x8000));
          break;
        case VK_ESCAPE:
          DestroyWindow(hwnd);
          break;
        case VK_RETURN:
          FillResults();
          DestroyWindow(hwnd);
          break;
        }
      }
      break;
    case WM_DRAWITEM:
      {
        DRAWITEMSTRUCT *dis = (LPDRAWITEMSTRUCT)lParam;

        RECT rect;
        GetWindowRect(dis->hwndItem,&rect);
        sInt w=rect.right-rect.left;
        sInt h=rect.bottom-rect.top;

        Link *l;
        sFORALL(Links, l) if (dis->hwndItem==l->Wnd)
        {
          if (l->Icon)
            DrawIconEx(dis->hDC,0,0,l->Icon,w,h,0,0,DI_NORMAL);
        }

      }
      break;

    }

    return DefWindowProc(hwnd,uMsg,wParam,lParam);
  }

};

using namespace bob;


sBool bOpenSelector(const bSelectorSetup &setup,bSelectorResult &result)
{

  // make choices
  Done=sFALSE;
  sClear(result);
  Result=&result;
  Setup=&setup;

  sString<2048> resolutions;
  sInt defres=0;
  sString<1024> aspects;
  sInt defasp=0;
  sString<1024> fsaa;
  sInt deffsaa=0;

  SI = new sScreenInfo;
  sGetScreenInfo(*SI,sSM_FULLSCREEN);

  // resolutions...
  for (sInt i=0; i<SI->Resolutions.GetCount(); i++)
  {
    const sScreenInfoXY &r=SI->Resolutions[i];
    sString<64> str;
    str.PrintF(L"%s%dx%d",i?L"|":L"",r.x,r.y);
    sAppendString(resolutions,str);
    if(sRELEASE && !sGetShellSwitch(L"w"))
    {
      if (r.x==SI->CurrentXSize && r.y==SI->CurrentYSize) 
        defres=i;
    }
    else
    {
      if (r.x==1280 && r.y==720)      // optimal resolution for debugging - not to big and still quite big
        defres=i;
    }
  }
  if(setup.DialogFlags & wDODF_SetResolution)
  {
    sBool found=sFALSE;

    for (sInt i=0; i<SI->Resolutions.GetCount(); i++)
    {
      const sScreenInfoXY &r=SI->Resolutions[i];
      if(r.x==setup.DialogScreenX && r.y==setup.DialogScreenY)
      {
        defres=i;
        found=sTRUE;
      }
    }

    // if not found, try if there's a resolution with letterboxing that fits
    if (!found) for (sInt i=0; i<SI->Resolutions.GetCount(); i++)
    {
      const sScreenInfoXY &r=SI->Resolutions[i];
      if(r.x==setup.DialogScreenX && r.y>=setup.DialogScreenY && (defres<0 || r.y<SI->Resolutions[defres].y))
      {
        defres=i;
        found=sTRUE;
      }
    }

    // if not found, try if there's a resolution with pillarboxing that fits
    if (!found) for (sInt i=0; i<SI->Resolutions.GetCount(); i++)
    {
      const sScreenInfoXY &r=SI->Resolutions[i];
      if(r.x>=setup.DialogScreenX && r.y==setup.DialogScreenY && (defres<0 || r.x<SI->Resolutions[defres].x))
      {
        defres=i;
        found=sTRUE;
      }
    }
  }

  // aspects...
  Aspects.HintSize(32);

  static const sScreenInfoXY defaultaspects[]={{4,3},{5,4},{16,10},{16,9}};
  for (sInt i=0; i<sCOUNTOF(defaultaspects); i++)
    Aspects.AddTail(defaultaspects[i]);

  for (sInt i=0; i<SI->AspectRatios.GetCount(); i++)
  {
    sScreenInfoXY &r=SI->AspectRatios[i];
    sInt found=0;
    for (sInt j=0; j<Aspects.GetCount(); j++)
      if (!sCmpMem(&r,&Aspects[j],sizeof(sScreenInfoXY)))
        found=1;
    if (!found)
      Aspects.AddTail(r);
  }

  for (sInt i=0; i<Aspects.GetCount(); i++)
    for (sInt j=i+1; j<Aspects.GetCount(); j++)
      if ((sF32(Aspects[j].x)/sF32(Aspects[j].y))<(sF32(Aspects[i].x)/sF32(Aspects[i].y)))
        sSwap(Aspects[i],Aspects[j]);

  aspects.PrintF(L"Auto");
  for (sInt i=0; i<Aspects.GetCount(); i++)
  {
    const sScreenInfoXY &r=Aspects[i];
    sString<64> str;
    str.PrintF(L"|%d:%d",r.x,r.y);
    sAppendString(aspects,str);
  }

  // FSAA..
  fsaa.PrintF(L"off");
  for (sInt i=0; i<SI->MultisampleLevels; i++)
  {
    sString<64> str;
    str.PrintF(L"|Level %d",i);
    sAppendString(fsaa,str);
  }

  sBool full=sTRUE;
  if(!sRELEASE || sGetShellSwitch(L"w"))
    full = 0;
  
  // create window class for links (like button, but: hand cursor)
  LinkClass.cbSize=sizeof(LinkClass);
  GetClassInfoEx(0,L"BUTTON",&LinkClass);
  LinkClass.hCursor=LoadCursorW(0,IDC_HAND);
  LinkClass.lpszClassName=L"bLink";
  RegisterClassEx(&LinkClass);

  // create window class and window
  sClear(SelWinClass);
  SelWinClass.cbSize=sizeof(SelWinClass);
  SelWinClass.style=CS_DROPSHADOW;
  SelWinClass.lpfnWndProc=SelWndProc;
  SelWinClass.hInstance=GetModuleHandle(0);
  SelWinClass.hIcon = LoadIcon(SelWinClass.hInstance,MAKEINTRESOURCE(100));

  SelWinClass.hCursor=LoadCursorW(0,IDC_ARROW);
  SelWinClass.hbrBackground=GetSysColorBrush(COLOR_BTNFACE);
  SelWinClass.lpszClassName=L"bSelector";
  RegisterClassEx(&SelWinClass);

  static const sInt W=200;

  CaptionFont=CreateFont(30,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
                         OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,PROOF_QUALITY,DEFAULT_PITCH|FF_DONTCARE,L"Tahoma");

  SelWin=CreateWindowEx(0,L"bSelector",setup.Title,WS_CAPTION,CW_USEDEFAULT,CW_USEDEFAULT,W,100,0,0,0,0);

  Links.HintSize(32);

  sInt y=0;
  sInt leftx=5;
  sInt leftx2=10;
  sInt margin=5;

  // add main link/icon if applicable

  if (setup.IconInt)
  {
    y+=margin;
    Link &l=AddLink(setup.IconURL,leftx,y,32,32);
    l.Icon=LoadIcon(GetModuleHandle(0),MAKEINTRESOURCE(setup.IconInt));
    AddStatic(setup.Caption,leftx+32+margin,y+2,200,sTRUE);
    y+=32;
  }

  if (!setup.SubCaption.IsEmpty())
  {
    AddStatic(setup.SubCaption,leftx+32+margin,y,200,sFALSE,sTRUE);
    y+=10;
  }

  y+=margin; sInt gy=y; y+=margin+15;

  // add video options
  sInt gw=90;
  AddStatic(L"Resolution",leftx2,y,200);
  ResBox=AddComboBox(resolutions,W-leftx2-gw,y-3,gw,defres);
  y+=25;
  AddStatic(L"Aspect Ratio",leftx2,y,200);
  AspectBox=AddComboBox(aspects,W-leftx2-gw,y-3,gw,defasp);
  y+=25;
  if(MULTISAMPLING)
  {
    AddStatic(L"Multisampling",leftx2,y,200);
    FSAABox=AddComboBox(fsaa,W-leftx2-gw,y-3,gw,deffsaa);
    y+=25;
  }

  FSCheck=AddCheck(L"Fullscreen",leftx2,y,70,full);
  VSCheck=AddCheck(L"Wait for VSync",W-leftx2-90,y,90,1);
  y+=25;

  AddGroup(L"Video",leftx,gy,W-2*leftx,y-gy);
  y+=margin; gy=y; y+=margin+15;

  //MBlurCheck=AddCheck(L"Motion Blur Effect",10,H-85,200,1);
  LoopCheck = 0;
  CoreCheck = 0;
  LowQCheck = 0;
  sBool advance = 0;

  if(Setup->DialogFlags & wDODF_HiddenParts)
  {
    AddStatic(L"Hidden Parts",leftx2,y,200);
    HiddenPartBox=AddComboBox(setup.HiddenPartChoices,W-leftx2-gw,y-3,gw,0);

    y+=25;
  }
  if(!(Setup->DialogFlags & wDODF_NoLoop))
  {
    LoopCheck=AddCheck(L"Loop Demo",leftx2,y,100,0);
    advance = 1;
  }
  if (Setup->DialogFlags & wDODF_Benchmark)
  {
    BenchmarkCheck=AddCheck(L"Benchmark",W-leftx2-72,y,72,0);
    advance = 1;
  }
  if(advance)
    y+=25;
  
  if(Setup->DialogFlags & wDODF_Multithreading)
  {
    CoreCheck = AddCheck(L"Reserve one core for Windows",leftx2,y,200,0);
    y+=25;
  }
  if(Setup->DialogFlags & wDODF_LowQuality)
  {
    LowQCheck = AddCheck(L"Low Quality Mode",leftx2,y,200,0);
    y+=25;
  }


  AddGroup(L"Options",leftx,gy,W-2*leftx,y-gy);
  y+=margin; gy=y; y+=15;

  // sharing sites...
  if (setup.Sites[0].IconInt)
  {
    sInt ssx=leftx2;
    for (sInt i=0; i<sCOUNTOF(setup.Sites); i++)
    {
      const bSelectorSetup::ShareSite &site=setup.Sites[i];
      if (!site.IconInt) break;

      Link &l=AddLink(site.URL,ssx,y,16,16);
      l.Icon=LoadIcon(GetModuleHandle(0),MAKEINTRESOURCE(site.IconInt));
      ssx+=20;
    }
    y+=20;
    AddGroup(L"Share!",leftx,gy,W-2*leftx,y-gy);
  }

  y+=2*margin;
  Die=AddButton(L"Die",leftx,y,0,0);
  Demo=AddButton(L"Demo",W-leftx-80,y,0,1);
  AddStatic(L"||",leftx+92,y+6,15);
  y+=25;

  
  // size and position window
  RECT crect, wrect;
  sInt H=y+margin;
  GetWindowRect(SelWin,&wrect);
  GetClientRect(SelWin,&crect);
  sInt ww=(wrect.right-wrect.left); ww+=W-(crect.right-crect.left);
  sInt wh=(wrect.bottom-wrect.top); wh+=H-(crect.bottom-crect.top);
  HWND dw=GetDesktopWindow();
  GetWindowRect(dw,&wrect); 
  MoveWindow(SelWin,(wrect.right+wrect.left-ww)/2,(wrect.top+wrect.bottom-wh)/2,ww,wh,FALSE);

  // ... and go!
  UpdateWindow(SelWin);
  ShowWindow(SelWin,SW_SHOW);
  MSG msg;
  GetMessage(&msg,0,0,0);
  while(msg.message!=WM_QUIT)
  {
    TranslateMessage(&msg);

    // message filter
    if (msg.message==WM_KEYDOWN && (msg.wParam==VK_TAB || msg.wParam==VK_ESCAPE || msg.wParam==VK_RETURN))
      msg.hwnd=SelWin;

    DispatchMessage(&msg);
    GetMessage(&msg,0,0,0);
  }

  DeleteObject(CaptionFont);

  UnregisterClass(L"bSelector",0);
  UnregisterClass(L"bLink",0);

  sDelete(SI);
  Aspects.Reset();
  Links.Reset();

  return Done;
}

/****************************************************************************/

#endif