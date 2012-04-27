/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "gui/gui.hpp"
#include "gui/dialog.hpp"
#include "base/windows.hpp"
#include "base/graphics.hpp"
#include "gui/overlapped.hpp"

/****************************************************************************/


void sOpenFileDialog(const sChar *label,const sChar *extensions,sInt flags,sPoolString &ps,const sMessage &ok,const sMessage &cancel,sObject *tagme)
{
  sString<sMAXPATH> buffer;

  buffer = ps;
  sBool result = sSystemOpenFileDialog(label,extensions,flags,buffer);
  ps = buffer;
  if (sGui)
    sGui->Notify(ps);

  if(result)
    ok.Post();
  else
    cancel.Post();
}

void sOpenFileDialog(const sChar *label,const sChar *extensions,sInt flags,const sStringDesc &buffer,const sMessage &ok,const sMessage &cancel,sObject *tagme)
{
  sBool result = sSystemOpenFileDialog(label,extensions,flags,buffer);
  if (sGui)
    sGui->Notify(buffer.Buffer,buffer.Size);

  if(result)
    ok.Post();
  else
    cancel.Post();
}

/****************************************************************************/

class sDialogWindow : public sWindow
{
public:
  sTextBuffer Text;
  sButtonControl *LeftButton;
  sButtonControl *RightButton;
  const sChar *LeftLabel;
  const sChar *RightLabel;
  const sChar *Header;
  sStringDesc EditString;
  sPoolString *PoolString;
  sStringControl *EditControl;


  sMessage LeftMessage;
  sMessage RightMessage;


  sDialogWindow();
  void Start();

  void OnLayout();
  void OnCalcSize();
  void OnPaint2D();
  sBool OnShortcut(sU32 key);

  void CmdLeft();
  void CmdRight();
};

sDialogWindow::sDialogWindow() : EditString(0,0)
{
  PoolString = 0;
  LeftButton = 0;
  RightButton = 0;
  LeftLabel = 0;
  RightLabel = 0;
  EditControl = 0;
  Header = L"Message";
  Flags |= sWF_OVERLAPPEDCHILDS|sWF_CLIENTCLIPPING;
}

void sDialogWindow::Start()
{
  if(LeftLabel)
  {
    LeftButton = AddButton(LeftLabel,sMessage(this,&sDialogWindow::CmdLeft));
    LeftButton->Width = 75;
  }
  if(RightLabel)
  {
    RightButton = AddButton(RightLabel,sMessage(this,&sDialogWindow::CmdRight));
    RightButton->Width = 75;
  }
  if(EditString.Buffer)
  {
    EditControl = new sStringControl(EditString);
    AddChild(EditControl);
  }
  if(PoolString)
  {
    EditControl = new sStringControl(PoolString);
    AddChild(EditControl);
  }

  OnCalcSize();
  sGui->AddFloatingWindow(this,Header);
  if(EditControl)
  {
    sGui->SetFocus(EditControl);
  //  EditControl->DoneMsg = sMessage(this,&sDialogWindow::CmdRight);
  }
}

void sDialogWindow::OnLayout()
{
  if(LeftButton)
  {
    LeftButton->Outer.x0 = Client.x0+10;
    LeftButton->Outer.y1 = Client.y1-10;
    LeftButton->Outer.x1 = LeftButton->Outer.x0 + LeftButton->DecoratedSizeX;
    LeftButton->Outer.y0 = LeftButton->Outer.y1 - LeftButton->DecoratedSizeY;
  }
  if(RightButton)
  {
    RightButton->Outer.x1 = Client.x1-10;
    RightButton->Outer.y1 = Client.y1-10;
    RightButton->Outer.x0 = RightButton->Outer.x1 - RightButton->DecoratedSizeX;
    RightButton->Outer.y0 = RightButton->Outer.y1 - RightButton->DecoratedSizeY;
  }
  if(EditControl)
  {
    EditControl->Outer.x0 = Client.x0+10;
    EditControl->Outer.x1 = Client.x1-10;
    EditControl->Outer.y1 = Client.y1-30;
    EditControl->Outer.y0 = EditControl->Outer.y1 - EditControl->DecoratedSizeY;
  }
}

void sDialogWindow::OnCalcSize()
{
  if(Text.GetCount()>100)
  {
    ReqSizeX = 300;
    ReqSizeY = 200;
  }
  else
  {
    ReqSizeX = 200;
    ReqSizeY = 100;
  }
}

void sDialogWindow::OnPaint2D()
{
  sGui->PropFont->SetColor(sGC_TEXT,sGC_BACK);
  sGui->PropFont->Print(sF2P_OPAQUE|sF2P_MULTILINE|sF2P_TOP|sF2P_LEFT,Client,Text.Get(),-1,5);
}

sBool sDialogWindow::OnShortcut(sU32 key)
{
  switch(key & (sKEYQ_MASK|sKEYQ_BREAK))
  {
  case sKEY_ESCAPE:
    CmdLeft();
    return sTRUE;

  case sKEY_ENTER:
    CmdRight();
    return sTRUE;
  }

  return sFALSE;
}

void sDialogWindow::CmdLeft()
{
  Close();
  LeftMessage.Post();
}

void sDialogWindow::CmdRight()
{
  Close();
  RightMessage.Post();
}

/****************************************************************************/

void sErrorDialog(const sMessage &ok,const sChar *text)
{
  sDialogWindow *dlg = new sDialogWindow;

  dlg->RightLabel = L"Continue";
  dlg->RightMessage = ok;
  dlg->Text.Print(text);
  dlg->Header = L"Error";

  dlg->Start();
}

void sOkDialog(const sMessage &ok,const sChar *text)
{
  sDialogWindow *dlg = new sDialogWindow;

  dlg->RightLabel = L"Continue";
  dlg->RightMessage = ok;
  dlg->Text.Print(text);
  dlg->Header = L"Information";

  dlg->Start();
}

void sOkCancelDialog(const sMessage &ok,const sMessage &cancel,const sChar *text)
{
  sDialogWindow *dlg = new sDialogWindow;

  dlg->LeftLabel = L"Cancel";
  dlg->LeftMessage = cancel;
  dlg->RightLabel = L"Ok";
  dlg->RightMessage = ok;
  dlg->Text.Print(text);
  dlg->Header = L"Choose";

  dlg->Start();
}

void sYesNoDialog(const sMessage &yes,const sMessage &no,const sChar *text)
{
  sDialogWindow *dlg = new sDialogWindow;

  dlg->LeftLabel = L"No";
  dlg->LeftMessage = no;
  dlg->RightLabel = L"Yes";
  dlg->RightMessage = yes;
  dlg->Text.Print(text);
  dlg->Header = L"Choose";

  dlg->Start();
}


void sStringDialog(const sStringDesc &string,const sMessage &ok,const sMessage &cancel,const sChar *text)
{
  sDialogWindow *dlg = new sDialogWindow;

  dlg->EditString = string;
  dlg->LeftLabel = L"Cancel";
  dlg->LeftMessage = cancel;
  dlg->RightLabel = L"Ok";
  dlg->RightMessage = ok;
  dlg->Text.Print(text);
  dlg->Header = L"Choose";

  dlg->Start();
}

void sStringDialog(sPoolString *string,const sMessage &ok,const sMessage &cancel,const sChar *text)
{
  sDialogWindow *dlg = new sDialogWindow;

  dlg->PoolString = string;
  dlg->LeftLabel = L"Cancel";
  dlg->LeftMessage = cancel;
  dlg->RightLabel = L"Ok";
  dlg->RightMessage = ok;
  dlg->Text.Print(text);
  dlg->Header = L"Choose";

  dlg->Start();
}

/****************************************************************************/
/****************************************************************************/


sMultipleChoiceDialog::sMultipleChoiceDialog(const sChar *title,const sChar *text)
{
  Title = title;
  Text = text;
  PressedItem = -1;
  PressedOk = 0;
}

sMultipleChoiceDialog::~sMultipleChoiceDialog()
{
}

void sMultipleChoiceDialog::OnPaint2D()
{
  sRect t,b,r,l;
  Item *item;
  sInt h;
  sFont2D *font = sGui->PropFont;

  sClipPush();

  t = b = Client;
  t.x1 = b.x0 = Client.x0 + Client.SizeX()*2/3;

  t.Extend(-4);
  font->SetColor(sGC_TEXT,sGC_BACK);
  font->Print(sF2P_OPAQUE|sF2P_LEFT|sF2P_TOP|sF2P_MULTILINE,t,Text);
  sClipExclude(t);

  b.Extend(-4);
  h = font->GetHeight()*2;
  sFORALL(Items,item)
  {
    r = b;
    r.y0 = b.y0 + (_i+0)*h;
    r.y1 = b.y0 + (_i+1)*h-4;

    item->Rect = r;

    sGui->PaintButtonBorder(r,_i==PressedItem && PressedOk);
    l = r;
    l.x1 = r.x0 = sMin(r.x1,r.x0+font->GetWidth(L" ")+font->GetWidth(item->Text));
    font->SetColor(sGC_TEXT,sGC_BUTTON);
    font->Print(sF2P_OPAQUE|sF2P_SPACE|sF2P_LEFT,l,item->Text);
    sString<64> buffer;
    sButtonControl::MakeShortcut(buffer,item->Shortcut);
    font->Print(sF2P_OPAQUE|sF2P_SPACE|sF2P_RIGHT,r,buffer);    
    font->SetColor(sGC_TEXT,sGC_BACK);

    sClipExclude(item->Rect);
  }

  sRect2D(Client,sGC_BACK);

  sClipPop();
}

void sMultipleChoiceDialog::OnDrag(const sWindowDrag &dd)
{
  Item *item;
  sInt hit = -1;
  sFORALL(Items,item)
    if(item->Rect.Hit(dd.MouseX,dd.MouseY))
      hit = _i;
   
  switch(dd.Mode)
  {
  case sDD_START:
    if(hit>=0)
    {
      PressedItem = hit;
      PressedOk = 1;
    }
    else
    {
      PressedItem = -1;
      PressedOk = 0;
    }
    Update();
    break;
  case sDD_DRAG:
    PressedOk = (PressedItem==hit);
    Update();
    break;
  case sDD_STOP:
    if(PressedOk && PressedItem>=0)
    {
      Items[PressedItem].Cmd.Post();
      Close();
    }
    PressedOk = 0;
    PressedItem = -1;
    Update();
    break;
  }
}

void sMultipleChoiceDialog::OnCalcSize()
{
}

sBool sMultipleChoiceDialog::OnKey(sU32 key)
{
  if(key & sKEYQ_SHIFT) key |= sKEYQ_SHIFT;
  if(key & sKEYQ_CTRL) key |= sKEYQ_CTRL;
  Item *item;
  sFORALL(Items,item)
  {
    if(item->Shortcut == key)
    {
      item->Cmd.Post();
      Close();
      return 1;
    }
  }
  return 0;
}

void sMultipleChoiceDialog::AddItem(const sChar *text,const sMessage &cmd,sU32 shortcut)
{
  Item *item = Items.AddMany(1);
  item->Cmd = cmd;
  item->Text = text;
  item->Shortcut = shortcut;
}

void sMultipleChoiceDialog::Start()
{
  Flags |= sWF_AUTOKILL;
  ReqSizeX = 400;
  ReqSizeY = (Items.GetCount())*sGui->PropFont->GetHeight()*2+4;

  sRect r;
  sPrintInfo pi;
  pi.Mode = sPIM_GETHEIGHT;
  r.Init(0,0,ReqSizeX*2/3-8,1000);
  sInt h = sGui->PropFont->Print(sF2P_OPAQUE|sF2P_LEFT|sF2P_TOP|sF2P_MULTILINE,r,Text,-1,0,0,0,&pi)+8;
  ReqSizeY = sMax(ReqSizeY,h);

  AddBorder(new sThickBorder());
  AddBorder(new sTitleBorder(Title));

  sGui->AddCenterWindow(this);
  sGui->SetFocus(this);
}

/****************************************************************************/

void sQuitReallyDialog(const sMessage &quit,const sMessage &savequit)
{
  sMultipleChoiceDialog *diag = new sMultipleChoiceDialog(
    L"Save and Quit",
    L"Your document was not saved.\n\n"
    L"Do you really want to quit the application without saving?");

  diag->AddItem(L"Save & Quit",sMessage(savequit),'s');
  diag->AddItem(L"Quit",sMessage(quit),'Q'|sKEYQ_SHIFT);
  diag->AddItem(L"Continue",sMessage(),sKEY_ESCAPE);

  diag->Start();
}

/****************************************************************************/

void sContinueReallyDialog(const sMessage &cont,const sMessage &savecont)
{
  sMultipleChoiceDialog *diag = new sMultipleChoiceDialog(
    L"Document not saved",
    L"Your document was not saved.\n\n"
    L"Do you really want to continue without saving?");

  diag->AddItem(L"Save & continue",sMessage(savecont),'s');
  diag->AddItem(L"Continue",sMessage(cont),'Q'|sKEYQ_SHIFT);
  diag->AddItem(L"Cancel",sMessage(),sKEY_ESCAPE);

  diag->Start();
}

/****************************************************************************/

sProgressDialog *sProgressDialog::Instance = 0;

sProgressDialog::sProgressDialog(const sChar *title,const sChar *text)
{
  sInt ReqSizeX = 400;
  sInt ReqSizeY = sGui->PropFont->GetHeight()*5/2 + 8 + 15 + 4;

  sInt sx,sy;
  sGetScreenSize(sx,sy);
  WindowRect.Init((sx - ReqSizeX)/2,(sy - ReqSizeY) / 2,(sx + ReqSizeX + 1)/2,(sy + ReqSizeY + 1)/2);

  Title = title;
  Text = text;
  LastUpdate = 0;
  Percentage = 0.0f;
  CancelFlag = sFALSE;

  BracketStack.AddTail(Bracket(0.0f,1.0f));
}

sProgressDialog::~sProgressDialog()
{
  sVERIFY(Instance == this);
  Instance = 0;
}

void sProgressDialog::OnCancel()
{
  CancelFlag = sTRUE;
}

void sProgressDialog::Render()
{
  sRender2DBegin();

  sRect r = WindowRect;

  // border around window
  sGui->RectHL(r,sGC_HIGH2,sGC_LOW2);
  r.Extend(-1);
  sGui->RectHL(r,sGC_HIGH,sGC_LOW);
  r.Extend(-1);

  // title bar
  sRect titleRect = r;
  titleRect.y1 = titleRect.y0 + 14;

  sGui->PropFont->SetColor(sGC_TEXT,sGC_BUTTON);
  sGui->PropFont->Print(sF2P_OPAQUE|sF2P_LEFT|sF2P_SPACE,titleRect,Title);

  titleRect.y0 = titleRect.y1;
  titleRect.y1 = titleRect.y0+1;
  sRect2D(titleRect,sGC_DRAW); // line separating title bar from window contents

  // window contents
  r.y0 = titleRect.y1;
  sRect inner = r;
  inner.Extend(-4);
  sRectHole2D(r,inner,sGC_BACK);

  sRect rc0,rc1;
  rc0 = rc1 = inner;
  rc0.y1 = rc1.y0 = rc0.y0 + sGui->PropFont->GetHeight() + 2;
  sGui->PropFont->SetColor(sGC_TEXT,sGC_BACK);
  sGui->PropFont->Print(sF2P_OPAQUE|sF2P_LEFT,rc0,Text);

  sGui->RectHL(rc1,sGC_DRAW,sGC_DRAW);
  rc1.Extend(-1);
  rc0 = rc1;
  rc0.x1 = rc1.x0 = rc0.x0 + sClamp(Percentage,0.0f,1.0f) * (rc0.x1-rc0.x0);
  sRect2D(rc0,sGC_SELECT);
  sRect2D(rc1,sGC_BACK);

  sRender2DEnd();
}

void sProgressDialog::Open(const sChar *title,const sChar *text)
{
  sVERIFY(Instance == 0);
#if !sCOMMANDLINE
  Instance = new sProgressDialog(title,text);
#endif
}

sBool sProgressDialog::Close()
{
#if !sCOMMANDLINE
  if(Instance != 0)
  {
    sGui->Update();
    sBool ret = !Instance->CancelFlag;

    delete Instance;
    Instance = 0;

    return ret;
  }
  else
    return sTRUE; // no window => no cancel
#else
  return sTRUE;
#endif
}

void sProgressDialog::SetText(const sChar *message,sBool forceUpdate)
{
  if(Instance)
  {
    Instance->Text = message;
    if(forceUpdate)
      Instance->LastUpdate = 0; // force an update on next SetProgress
  }
}

sBool sProgressDialog::SetProgress(sF32 percentage)
{
  sInt now = sGetTime();

  if(Instance && now >= Instance->LastUpdate + 50)
  {
    percentage = sClamp(percentage,0.0f,1.0f);

    Bracket bracket = Instance->BracketStack.GetTail();
    Instance->Percentage = sFade(percentage,bracket.Start,bracket.End);
    Instance->Render();
    Instance->LastUpdate = now;

    return !Instance->CancelFlag;
  }
  else
    return sTRUE; // no progress bar -> no cancel
}

void sProgressDialog::PushLevel(sF32 start,sF32 end)
{
  if(Instance)
  {
    SetProgress(start);

    const Bracket top = Instance->BracketStack.GetTail();
    start = sMax(start,0.0f);
    end = sMin(end,1.0f);

    Instance->BracketStack.AddTail(Bracket(sFade(start,top.Start,top.End),sFade(end,top.Start,top.End)));
  }
}

void sProgressDialog::PushLevelCounter(sInt current,sInt max)
{
  PushLevel(1.0f*current/max,1.0f*(current+1)/max);
}

void sProgressDialog::PopLevel()
{
  if(Instance)
  {
    sVERIFY(Instance->BracketStack.GetCount() > 1);
    sF32 tailProgress = Instance->BracketStack.GetTail().End;
    Instance->BracketStack.RemTail();
    SetProgress(tailProgress);
  }
}

void sProgressDialog::ChangeLevelCounter(sInt current,sInt max)
{
  PopLevel();
  PushLevelCounter(current,max);
}

/****************************************************************************/

sProgressDialogScope::sProgressDialogScope(const sChar *text,sInt current,sInt max)
{
  sProgressDialog::SetText(text);
  sProgressDialog::PushLevelCounter(current,max);
}

sProgressDialogScope::~sProgressDialogScope()
{
  sProgressDialog::PopLevel();
}

/****************************************************************************/
/***                                                                      ***/
/***   FindDialog, with incremental search list                           ***/
/***                                                                      ***/
/****************************************************************************/


class sFindWindow : public sWindow
{
  void CmdOk();
  void CmdCancel();
  sStringControl *EditControl;
  sWindow *OldFocus;
  sArray<sPoolString> IncList;
  sRect ListRect;
  sInt Selected;
  sInt Height;
public:
  const sChar *Header;
  sStringDesc EditString;
  sMessage OkMsg;
  void (*IncFunc)(sArray<sPoolString> &,const sChar *,void *ref);
  void *IncFuncRef;

  sFindWindow();
  void Tag();
  void Start();
  void UpdateIncList();

  void OnLayout();
  void OnCalcSize();
  void OnPaint2D();
  void OnDrag(const sWindowDrag &dd);
  sBool OnKey(sU32 key);
};

sFindWindow::sFindWindow() : EditString(0,0)
{
  EditControl = 0;
  OldFocus = 0;
  IncFunc = 0;
  IncFuncRef = 0;
  Selected = 0;

  Header = L"Message";
  Flags |= sWF_OVERLAPPEDCHILDS|sWF_CLIENTCLIPPING;
}

void sFindWindow::Tag()
{
  sWindow::Tag();
  OldFocus->Need();
}

void sFindWindow::Start()
{
  OldFocus = sGui->GetFocus();
  EditControl = new sStringControl(EditString);
  EditControl->DoneMsg = sMessage(this,&sFindWindow::CmdOk);
  EditControl->ChangeMsg = sMessage(this,&sFindWindow::UpdateIncList);
  EditControl->OnKey(sKEY_END);
  AddChild(EditControl);

  UpdateIncList();
  OnCalcSize();
  sGui->AddFloatingWindow(this,Header);

  sGui->SetFocus(EditControl);
}

void sFindWindow::OnLayout()
{
  EditControl->Outer.x0 = Client.x0;
  EditControl->Outer.x1 = Client.x1;
  EditControl->Outer.y0 = Client.y0;
  EditControl->Outer.y1 = Client.y0+EditControl->DecoratedSizeY;

  ListRect = Client;
  ListRect.y0 = EditControl->Outer.y1;

  Height = sGui->PropFont->GetHeight()+2;

}

void sFindWindow::OnCalcSize()
{
  ReqSizeX = 200;
  ReqSizeY = 400;
}

void sFindWindow::OnPaint2D()
{
  sInt y = ListRect.y0;
  sPoolString *str;
  sRect r = Client;
  r.y0 = y; y+=1;
  r.y1 = y;
  sRect2D(r,sGC_DRAW);

  sFORALL(IncList,str)
  {
    r.y0 = y; y+=Height;
    r.y1 = y;
    sGui->PropFont->SetColor(sGC_TEXT,_i==Selected ? sGC_SELECT : sGC_BACK);
    sGui->PropFont->Print(sF2P_OPAQUE|sF2P_LEFT|sF2P_SPACE,r,*str);
  }
  r.y0 = y;
  r.y1 = Client.y1;
  sRect2D(r,sGC_BACK);
}

void sFindWindow::OnDrag(const sWindowDrag &dd)
{
  if(dd.Mode==sDD_START)
  {
    if(ListRect.Hit(dd.StartX,dd.StartY))
    {
      sInt i = (dd.StartY-(ListRect.y0+1))/Height;
      if(i>=0 && i<IncList.GetCount())
      {
        Selected = i;
        CmdOk();
      }
    }
  }
}

sBool sFindWindow::OnKey(sU32 key)
{
  switch(key & (sKEYQ_MASK|sKEYQ_BREAK))
  {
  case sKEY_ESCAPE:
    CmdCancel();
    return sTRUE;

  case sKEY_ENTER:
    CmdOk();
    return sTRUE;

  case sKEY_UP:
    if(Selected>0) Selected--;
    Update();
    return sTRUE;

  case sKEY_DOWN:
    if(Selected<IncList.GetCount()-1) Selected++;
    Update();
    return sTRUE;
  }

  return sFALSE;
}

void sFindWindow::UpdateIncList()
{
  if(IncFunc)
  {
    IncList.Clear();
    (*IncFunc)(IncList,EditString.Buffer,IncFuncRef);
    Update();
    sSortUp(IncList);
    Selected = 0;
  }
}

void sFindWindow::CmdOk() 
{
  if(Selected>=0 && Selected<IncList.GetCount())
    sCopyString(EditString,IncList[Selected]);
  Close();
  OkMsg.Post(); 
  sGui->SetFocus(OldFocus);
}

void sFindWindow::CmdCancel()
{
  sCopyString(EditString,L"");
  Close(); 
  sGui->SetFocus(OldFocus);
}


void sFindDialog(const sChar *headline,const sStringDesc &buffer,const sMessage &ok,void *ref,void (*inc)(sArray<sPoolString> &,const sChar *,void *ref))
{
  sFindWindow *fd = new sFindWindow;

  fd->EditString = buffer;
  fd->Header = headline;
  fd->OkMsg = ok;
  fd->IncFunc = inc;
  fd->IncFuncRef = ref;

  fd->Start();
}

/****************************************************************************/
/***                                                                      ***/
/***   GUI theme edit window                                              ***/
/***                                                                      ***/
/****************************************************************************/

class sGuiThemeEditWindow : public sGridFrame
{
protected:

  enum { CANCEL, OK, LOAD, SAVE, };
  sGuiTheme &Theme;
  sInt Live;
  sGuiTheme Buffer;
  sPoolString Filename;

  void CmdUpdate()
  {
    if (Live) sGui->SetTheme(Theme);
  }

  void CmdButton(sDInt code)
  {
    switch (code)
    {
    case CANCEL:
      sCopyMem(&Theme,&Buffer,sizeof(sGuiTheme));
      CmdUpdate();
    case OK:
      Close();
      return;
    case LOAD:
      sOpenFileDialog(L"Open theme",L"agt",sSOF_LOAD,Filename,sMessage(this,&sGuiThemeEditWindow::CmdLoad),sMessage());
      break;
    case SAVE:
      sOpenFileDialog(L"Save theme",L"agt",sSOF_SAVE,Filename,sMessage(this,&sGuiThemeEditWindow::CmdSave),sMessage());
      break;
    }
  }

  void CmdLoad()
  {
    sGuiTheme temp;
    if (sLoadObject(Filename,&temp))
    {
      sCopyMem(&Theme,&temp,sizeof(sGuiTheme));
      sGui->Notify(Theme);
      CmdUpdate();
    }
  }

  void CmdSave()
  {
    sSaveObject(Filename,&Theme);
  }

public:

  sGuiThemeEditWindow(sGuiTheme &theme, sBool live) : Theme(theme), Live(live)
  {
    sCopyMem(&Buffer,&Theme,sizeof(sGuiTheme));

    sGridFrameHelper gh(this);
    gh.ChangeMsg=sMessage(this,&sGuiThemeEditWindow::CmdUpdate);

    gh.Group(L"Colors");
    gh.Label(L"Background"); gh.ColorPick(&Theme.BackColor, L"rgb", 0);
    gh.Label(L"Document"); gh.ColorPick(&Theme.DocColor, L"rgb", 0);
    gh.Label(L"Text"); gh.ColorPick(&Theme.TextColor, L"rgb", 0);
    gh.Label(L"Buttons"); gh.ColorPick(&Theme.ButtonColor, L"rgb", 0);
    gh.Label(L"Draw"); gh.ColorPick(&Theme.DrawColor, L"rgb", 0);
    gh.Label(L"Selection"); gh.ColorPick(&Theme.SelectColor, L"rgb", 0);
    gh.Label(L"Higher"); gh.ColorPick(&Theme.HighColor2, L"rgb", 0);
    gh.Label(L"High"); gh.ColorPick(&Theme.HighColor, L"rgb", 0);
    gh.Label(L"Low"); gh.ColorPick(&Theme.LowColor, L"rgb", 0);
    gh.Label(L"Lower"); gh.ColorPick(&Theme.LowColor2, L"rgb", 0);

    gh.Group(L"Fonts");
    gh.Label(L"Proportional"); gh.String(Theme.PropFont);
    gh.Label(L"Fixed Width"); gh.String(Theme.FixedFont);

    gh.Group();
    gh.Box(L"OK",sMessage(this,&sGuiThemeEditWindow::CmdButton,OK));
    gh.Box(L"Cancel",sMessage(this,&sGuiThemeEditWindow::CmdButton,CANCEL));
    gh.Right-=gh.BoxWidth;
    gh.Box(L"Save",sMessage(this,&sGuiThemeEditWindow::CmdButton,SAVE));
    gh.Box(L"Load",sMessage(this,&sGuiThemeEditWindow::CmdButton,LOAD));

    gh.Grid->ReqSizeX = 400;
  }

};

void sOpenGuiThemeDialog(sGuiTheme *theme, sBool live)
{
  if (!theme) return;
  sGui->AddFloatingWindow(new sGuiThemeEditWindow(*theme,live),L"Edit GUI theme");
}

