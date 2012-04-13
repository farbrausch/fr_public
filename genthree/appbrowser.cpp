// This file is distributed under a BSD license. See LICENSE.txt for details.
#include "appbrowser.hpp"

/****************************************************************************/

#define sBTCMD_SELDIR   0x10101
#define sBLCMD_SELECT   0x10201
#define sBLCMD_POPUP    0x10202
#define sBLCMD_DOUBLE   0x10203

/****************************************************************************/
/***                                                                      ***/
/***   Tree Window                                                        ***/
/***                                                                      ***/
/****************************************************************************/

class sBrowserTree : public sGuiWindow
{
public:
  sBrowserTree();
  ~sBrowserTree();
  void Tag();
  void SetRoot(sDiskItem *root);
  void Select(sDiskItem *dir);
  void ClearChilds(sDiskItem *dir);
  void OnPaint();
  void OnDrag(sDragData &dd);

  sChar GetState(sDiskItem *dir);
  void PaintDir(sDiskItem *dir,sInt &pos,sInt indent);
  void DragDir(sDiskItem *dir,sInt &pos,sInt indent,sInt mx,sInt my);

  sInt Height;

  sDiskItem *Dir;
  sDiskItem *SelDir;
  sU32 DIKey;
};

/****************************************************************************/

sBrowserTree::sBrowserTree()
{
  Dir = 0;
  SelDir = 0;
  DIKey = sDiskRoot->AllocKey();
}

sBrowserTree::~sBrowserTree()
{
  sDiskRoot->FreeKey(DIKey);
}

void sBrowserTree::Tag()
{
  sGuiWindow::Tag();
  sBroker->Need(Dir);
  sBroker->Need(SelDir);
}

void sBrowserTree::SetRoot(sDiskItem *root)
{
  Dir = root;
  SelDir = root;
  Dir->Cmd(sDIC_FINDALL,0,0);
  Dir->SetUserData(DIKey,1);
  ClearChilds(Dir);
}

void sBrowserTree::Select(sDiskItem *dir)
{
  while(dir && !dir->GetBool(sDIA_ISDIR))
    dir = dir->GetParent();

  SelDir = dir;
  while(dir)
  {
    dir->SetUserData(DIKey,1);
    dir = dir->GetParent();
  }
}

void sBrowserTree::ClearChilds(sDiskItem *dir)
{
  // now, new keys are always 0
/*
  sDiskItem *di;
  sInt i;

  for(i=0;;i++)
  {
    di =dir->GetChild(i);
    if(!di)
      break;

    di->SetUserData(DIKey,0);
  }
  */
}

void sBrowserTree::OnPaint()
{
  sInt pos;
  sRect r;

  Height = sPainter->GetHeight(sGui->FixedFont)+2;
  pos = 0;
  PaintDir(Dir,pos,0);
  SizeY = Height*pos;

  r = Client;
  r.y0 = Client.y0 + pos*Height;
  r.y1 = Client.y1;
  if(r.y0<r.y1)
    sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_BACK]);
}

sChar sBrowserTree::GetState(sDiskItem *dir)
{
  sChar state;
  sInt i;
  sDiskItem *di;

  if(dir->GetBool(sDIA_ISDIR))
  {
    if(dir->GetBool(sDIA_INCOMPLETE))
      state = '?';
    else
      state = ' ';
    if(dir->GetUserData(DIKey))
    {
      state = '-';
    }
    else
    {
      for(i=0;;i++)
      {
        di = dir->GetChild(i);
        if(!di) 
          break;
        if(di->GetBool(sDIA_ISDIR) || di->Support(sDIC_CUSTOMWINDOW))
        {
          state = '+';
          break;
        }
      }
    }
  }
  else
  {
    state = ' ';
  }

  return state;
}

void sBrowserTree::PaintDir(sDiskItem *dir,sInt &pos,sInt indent)
{
  sDiskItem *di;
  sChar buffer[sDI_NAMESIZE];
  sInt i;
  sRect r,rr;
  sChar state;
  sInt col0,col1,cold;

  state = GetState(dir);

  r = Client;
  r.y0 = pos*Height + Client.y0;
  pos++;
  r.y1 = pos*Height + Client.y0;
  r.x1 = r.x0+indent * Height;
  rr = r;
  r.x0 = r.x1;
  r.x1 = Client.x1;

  col0 = sGui->Palette[sGC_BACK];
  col1 = sGui->Palette[sGC_TEXT];
  cold = sGui->Palette[sGC_DRAW];
  if(dir==SelDir)
  {
    col0 = sGui->Palette[sGC_SELBACK];
    col1 = sGui->Palette[sGC_SELECT];
  }

  if(indent>0)
  {
    buffer[0] = state;
    buffer[1] = 0;
    sPainter->Paint(sGui->FlatMat,rr,col0);
    rr.x0 = rr.x1-Height;
    sPainter->PrintC(sGui->PropFont,rr,0,buffer,cold);
    rr.Extend(-2);
    sGui->RectHL(rr,1,sGui->Palette[sGC_DRAW],cold);
  }

  dir->GetString(sDIA_NAME,buffer,sizeof(buffer));
  sPainter->Paint(sGui->FlatMat,r,col0);
  sPainter->PrintC(sGui->FixedFont,r,sFA_LEFT|sFA_SPACE,buffer,col1);

  if(state=='-')
  {
    for(i=0;;i++)
    {
      di = dir->GetChild(i);
      if(!di)
        break;

      if(di->GetBool(sDIA_ISDIR) || di->Support(sDIC_CUSTOMWINDOW))
        PaintDir(di,pos,indent+1);
    }
  }
}


void sBrowserTree::OnDrag(sDragData &dd)
{
  sInt pos;

  switch(dd.Mode)
  {
  case sDD_START:
    pos = 0;
    DragDir(Dir,pos,0,dd.MouseX,dd.MouseY);
    break;
  }
}

void sBrowserTree::DragDir(sDiskItem *dir,sInt &pos,sInt indent,sInt mx,sInt my)
{
  sChar state;
  sRect r,rr;
  sInt i;
  sDiskItem *di;

  state = GetState(dir);

  r = Client;
  r.y0 = pos*Height + Client.y0;
  pos++;
  r.y1 = pos*Height + Client.y0;
  rr = r;
  rr.x1 = r.x0+indent * Height;
  rr.x0 = rr.x1-Height;

  if(rr.Hit(mx,my))
  {
    if(state=='?')
    {
      dir->Cmd(sDIC_FINDALL,0,0);
      state = GetState(dir);
    }
    if(state=='-')
      dir->SetUserData(DIKey,0);
    if(state=='+')
    {
      dir->SetUserData(DIKey,1);
      ClearChilds(dir);
    }
    return;
  }
  else if(r.Hit(mx,my))
  {
    if(SelDir!=dir)
    {
      SelDir = dir;
      dir->Cmd(sDIC_FINDALL,0,0);
      Post(sBTCMD_SELDIR);
    }
    return;
  }

  if(state=='-')
  {
    for(i=0;;i++)
    {
      di = dir->GetChild(i);
      if(!di)
        break;

      if(di->GetBool(sDIA_ISDIR) || di->Support(sDIC_CUSTOMWINDOW))
        DragDir(di,pos,indent+1,mx,my);
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   List Window                                                        ***/
/***                                                                      ***/
/****************************************************************************/

class sBrowserList : public sGuiWindow
{
  void Popup(sDiskItem *);
public:
  sBrowserList();
  void Tag();
  void SetDir(sDiskItem *dir);
  void Select(sDiskItem *di);
  void OnCalcSize();
  void OnPaint();
  void OnDrag(sDragData &);
  sBool OnCommand(sU32);


  sDiskItem *Dir;
  sTabBorder *Tabs;
  sU32 DIKey;
  sInt Height;
  sDiskItem *LastSelection;
};

/****************************************************************************/

sBrowserList::sBrowserList()
{
  Dir = 0;
  DIKey = sDiskRoot->AllocKey();
  Height = 10;
  LastSelection = 0;

  Tabs = new sTabBorder;
  Tabs->SetTab(0,"Name",175);
  Tabs->SetTab(1,"Size",100);
}

void sBrowserList::Tag()
{
  sGuiWindow::Tag();
  sBroker->Need(Dir);
  sDiskRoot->FreeKey(DIKey);
}

void sBrowserList::SetDir(sDiskItem *dir)
{
  if(dir->GetBool(sDIA_INCOMPLETE))
    dir->Cmd(sDIC_FINDALL,0,0);
  Dir = dir;
}

void sBrowserList::Select(sDiskItem *di)
{
  sInt i;
  sDiskItem *d2;

  LastSelection = 0;
  for(i=0;(d2=Dir->GetChild(i))!=0;i++)
  {
    if(d2==di)
    {
      d2->SetUserData(DIKey,1);
      LastSelection = d2;
    }
    else
    {
      d2->SetUserData(DIKey,0);
    }
  }
}

void sBrowserList::OnCalcSize()
{
  sInt h;

  h = sPainter->GetHeight(sGui->FixedFont)+2;
  SizeX = Tabs->GetTotalWidth();
  SizeY = Dir->GetChildCount()*h;
}

void sBrowserList::OnPaint()
{
  sDiskItem *di;
  sChar buffer[sDI_NAMESIZE];
  sInt i;
  sInt y;
  sRect r;
  sU32 col0,col1;

  y = Client.y0;
  Height = sPainter->GetHeight(sGui->FixedFont)+2;

  for(i=0;;i++)
  {
    di = Dir->GetChild(i);
    if(!di)
      break;

    if(di->GetUserData(DIKey))
    {
      col0 = sGui->Palette[sGC_SELBACK];
      col1 = sGui->Palette[sGC_SELECT];
    }
    else
    {
      col0 = sGui->Palette[sGC_BACK];
      col1 = sGui->Palette[sGC_TEXT];
    }

    r = Client;
    r.y0 = y;
    y+=Height;
    r.y1 = y;
    sPainter->Paint(sGui->FlatMat,r,col0);

    r.x0 = Tabs->GetTab(0);
    r.x1 = Tabs->GetTab(1);
    di->GetString(sDIA_NAME,buffer,sizeof(buffer));
    sPainter->PrintC(sGui->FixedFont,r,sFA_LEFT|sFA_SPACE,buffer,col1);

    r.x0 = Tabs->GetTab(1);
    r.x1 = Tabs->GetTab(2);
    if(di->GetBool(sDIA_ISDIR))
      sSPrintF(buffer,sizeof(buffer),"[dir]");
    else if(di->Support(sDIC_CUSTOMWINDOW))
      sSPrintF(buffer,sizeof(buffer),"[special]");
    else
      sSPrintF(buffer,sizeof(buffer),"%d",di->GetBinarySize(sDIA_DATA));
    sPainter->PrintC(sGui->FixedFont,r,sFA_LEFT|sFA_SPACE,buffer,col1);
  }
  r = Client;
  r.y0 = y;
  if(r.y1>r.y0)
    sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_BACK]);

  SizeX = Tabs->GetTotalWidth();
  SizeY = Dir->GetChildCount()*Height;
}

void sBrowserList::OnDrag(sDragData &dd)
{
  sInt pos,i;
  sDiskItem *di;
  sU32 qual;

  switch(dd.Mode)
  {
  case sDD_START:
    qual = sSystem->GetKeyboardShiftState();
    if(dd.Buttons&2)
      qual = 0;
    pos = (dd.MouseY-Client.y0)/Height;
    if(pos>=0 && pos<Dir->GetChildCount())
    {
      if(qual & sKEYQ_CTRL)
      {
        di = Dir->GetChild(pos);
        if(di)
        {
          di->SetUserData(DIKey,!di->GetUserData(DIKey));
          LastSelection = di;
        }
      }
      else
      {
        for(i=0;;i++)
        {
          di = Dir->GetChild(i);
          if(!di) 
            break;
          di->SetUserData(DIKey,pos==i?1:0);
          if(pos==i)
            LastSelection = di;
        }
      }
      if(dd.Buttons&2)
      {
        sGui->Post(sBLCMD_POPUP,this);
      }
      else if(dd.Buttons & sDDB_DOUBLE)
      {
        sGui->Post(sBLCMD_SELECT,this);
        sGui->Post(sBLCMD_DOUBLE,this);
      }
      else
      {
        sGui->Post(sBLCMD_SELECT,this);
      }
    }
    break;
  }
}

sBool sBrowserList::OnCommand(sU32 cmd)
{
  switch(cmd)
  {
  case sDIC_EXECUTE:
    if(LastSelection)
      LastSelection->Cmd(cmd,0,0);
    return sTRUE;
  case sBLCMD_POPUP:
    if(LastSelection)
      Popup(LastSelection);
    return sTRUE;

  default:
    return sFALSE;
  }
}

void sBrowserList::Popup(sDiskItem *di)
{
  sMenuFrame *mf;
  sDiskItem *dp;

  mf = new sMenuFrame;
  mf->AddBorder(new sNiceBorder);

  if(di->Support(sDIC_EXECUTE))
    mf->AddMenu("execute",sDIC_EXECUTE,sKEY_ENTER);
  dp = di->GetParent();
  if(dp)
  {
    if(dp->Support(sDIC_DELETE))
      mf->AddMenu("delete",sDIC_DELETE,sKEY_DELETE);
    if(dp->Support(sDIA_NAME))
      mf->AddMenu("rename",sDIA_NAME,'r');
  }

  mf->SendTo = this;
  if(mf->GetChildCount()>0)
    sGui->AddPopup(mf);
}

/****************************************************************************/
/***                                                                      ***/
/***   Main Window                                                        ***/
/***                                                                      ***/
/****************************************************************************/

sBrowserApp::sBrowserApp()
{
  sGuiWindow *dummy;

  dummy = new sDummyFrame;
  CustomWindow = 0;
  Root = sDiskRoot;

  Tree = new sBrowserTree;
  Tree->AddBorder(new sFocusBorder);
  Tree->AddScrolling(0,1);
  Tree->SetRoot(Root);
  Tree->Select(Root->Find("Programms"));

  List = new sBrowserList;
  List->AddBorder(new sFocusBorder);
  List->AddBorder(List->Tabs);
  List->AddScrolling(1,1);
  List->SetDir(Root->Find("Programms"));

  Split = new sVSplitFrame;
  Split->AddChild(Tree);
  Split->AddChild(List);
  Split->Pos[1] = 200;
  AddChild(Split);

  DirCmd = 0;
  FileCmd = 0;
  DoubleCmd = 0;
  SendTo = this;
}

void sBrowserApp::Tag()
{
  sGuiWindow::Tag();
  sBroker->Need(Root);
  sBroker->Need(List);
  sBroker->Need(Tree);
  sBroker->Need(CustomWindow);
}

/****************************************************************************/

void sBrowserApp::OnCalcSize()
{
  SizeX = 500;
  SizeY = 300;
}

/****************************************************************************/

void sBrowserApp::OnLayout()
{
  Childs->Position = Client;
}

void sBrowserApp::OnPaint()
{
}

void sBrowserApp::OnKey(sU32 key)
{
  if(key==sKEY_APPCLOSE)
    Parent->RemChild(this);
}

void sBrowserApp::OnDrag(sDragData &)
{
}

void sBrowserApp::OnFrame()
{
}

sBool sBrowserApp::OnCommand(sU32 cmd)
{
  switch(cmd)
  {
  case sBTCMD_SELDIR:
    if(CustomWindow)
    {
      Split->RemChild(CustomWindow);
      Split->AddChild(List);
      Flags |= sGWF_LAYOUT;
      CustomWindow = 0;
    }
    if(Tree->SelDir->Support(sDIC_CUSTOMWINDOW))
    {
      CustomWindow = (sGuiWindow *) Tree->SelDir->Cmd(sDIC_CUSTOMWINDOW,0,0);
    }
    if(CustomWindow)
    {
      if(List->Parent)
        Split->RemChild(List);
      Split->AddChild(CustomWindow);
      Flags |= sGWF_LAYOUT;
    }
    else
    {
      List->SetDir(Tree->SelDir);
    }
    sGui->Post(DirCmd,SendTo);
    return sTRUE;

  case sBLCMD_SELECT:
    sGui->Post(FileCmd,SendTo);
    return sTRUE;
  case sBLCMD_DOUBLE:
    if(List->LastSelection && List->LastSelection->GetBool(sDIA_ISDIR))
    {
      Tree->Select(List->LastSelection);
      List->SetDir(List->LastSelection);
      List->LastSelection = 0;
      sGui->Post(DirCmd,SendTo);
    }
    else
    {
      sGui->Post(DoubleCmd,SendTo);
    }
    return sTRUE;
  default:
    return sFALSE;
  }
}

/****************************************************************************/

void sBrowserApp::SetPath(sChar *path)
{
  sDiskItem *di;

  di = Root->Find(path);
  if(di)
  {
    Tree->Select(di);
    if(!di->GetBool(sDIA_ISDIR) && di->GetParent())
    {
      List->SetDir(di->GetParent());
      List->Select(di);
    }
    else
    {
      List->SetDir(di);
    }
  }
}

void sBrowserApp::GetFileName(sChar *buffer,sInt size)
{
  if(List->LastSelection)
    List->LastSelection->GetString(sDIA_NAME,buffer,size);
  else
    buffer[0] = 0;
}

void sBrowserApp::GetDirName(sChar *buffer,sInt size)
{
  if(Tree->SelDir)
    Tree->SelDir->GetPath(buffer,size);
  else
    buffer[0] = 0;
}

void sBrowserApp::GetPath(sChar *buffer,sInt size)
{
  sInt i;

  GetDirName(buffer,size);
  i = sGetStringLen(buffer);
  buffer+=i;
  *buffer++ = '/';
  size-=i+1;
  GetFileName(buffer,size);
  if(*buffer==0)
    buffer[-1] = 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   File Requester                                                     ***/
/***                                                                      ***/
/****************************************************************************/

sFileWindow::sFileWindow()
{
  Position.Init(0,0,600,400);
  Browser = new sBrowserApp;
  Browser->AddBorder(new sThinBorder);
  PathCon   = new sControl; PathCon  ->EditString(0,Path,"Path",sizeof(Path));
  NameCon   = new sControl; NameCon  ->EditString(0,Name,"Name",sizeof(Path));
  OkCon     = new sControl; OkCon    ->Button("Ok",1);
  CancelCon = new sControl; CancelCon->Button("Cancel",2);

  AddChild(Browser);
  AddChild(PathCon);
  AddChild(NameCon);
  AddChild(OkCon);
  AddChild(CancelCon);

  SendTo = this;
  OkCmd = 0;
  CancelCmd = 0;
  Path[0] = 0;
  Name[0] = 0;

  Browser->DirCmd = 3;
  Browser->FileCmd = 4;
  Browser->DoubleCmd = 5;

  SetPath("disks/c:/nxn");
}

void sFileWindow::OnLayout()
{
  sInt h;
  sInt y;

  h = sPainter->GetHeight(sGui->FixedFont)+8;
  y = Client.y1;

  y-=10;
  CancelCon->Position.x0 = Client.x0+10;
  CancelCon->Position.y0 = y-h*3/2;
  CancelCon->Position.x1 = Client.x0+10+70;
  CancelCon->Position.y1 = y;
  OkCon->Position.x0 = Client.x1-10-70;
  OkCon->Position.y0 = y-h*3/2;
  OkCon->Position.x1 = Client.x1-10;
  OkCon->Position.y1 = y;

  y-=h*3/2;
  y-=5;
  NameCon->Position.Init(Client.x0+10,y-h,Client.x1-10,y);
  y-=h+5;
  PathCon->Position.Init(Client.x0+10,y-h,Client.x1-10,y);
  y-=h+10;
  Browser->Position.Init(Client.x0,Client.y0,Client.x1,y);

  Height = Client.y1-y;
}

void sFileWindow::OnPaint()
{
  sPainter->Paint(sGui->FlatMat,Client.x0,Client.y1-Height,Client.x1,Client.y1,sGui->Palette[sGC_BACK]);
}

sBool sFileWindow::OnCommand(sU32 cmd)
{
  switch(cmd)
  {
  case 1:
    sGui->Post(OkCmd,SendTo);
    return sTRUE;
  case 2:
    sGui->Post(CancelCmd,SendTo);
    return sTRUE;
  case 3:
    Browser->GetDirName(Path,sizeof(Path));    
    Browser->GetFileName(Name,sizeof(Name));
    return sTRUE;
  case 4:
    Browser->GetFileName(Name,sizeof(Name));
    return sTRUE;
  case 5:
    sGui->Post(OkCmd,SendTo);
    return sTRUE;
  default:
    return sFALSE;
  }
}

void sFileWindow::SetPath(sChar *name)
{
  Browser->SetPath(name);
  Browser->GetDirName(Path,sizeof(Path));
  Browser->GetFileName(Name,sizeof(Name));
}

void sFileWindow::GetPath(sChar *buffer,sInt size)
{
  sCopyString(buffer,Path,size);
  if(Name[0])
  {
    sAppendString(buffer,"/",size);
    sAppendString(buffer,Name,size);
  }
}

/****************************************************************************/

sBool sFileWindow::ChangeExtension(sChar *path,sChar *newext)
{
  sInt len,pos,i;

  GetPath(path,sDI_PATHSIZE);
  len = sGetStringLen(path);
  pos = len;
  for(i=0;i<len;i++)
    if(path[i]=='.')
      pos = i;
  path[pos] = 0;
  sAppendString(path,newext,sDI_PATHSIZE);
  return sTRUE;
}

/****************************************************************************/
/***                                                                      ***/
/***   Load / Save / New / Exit / SaveAs                                  ***/
/***                                                                      ***/
/****************************************************************************/

sBool sImplementFileMenu(sU32 cmd,sGuiWindow *win,sObject *obj,sFileWindow *file,sBool dirty,sInt maxsave)
{
  sBrowserApp *browser;
  sDiskItem *di;
  sChar path[sDI_PATHSIZE];
  sU32 *mem,*data;

  switch(cmd)
  {
  case sCMDLS_NEW:
    if(dirty)
    {
      (new sDialogWindow)->InitOkCancel(win,"New File","Current Project is not saved.\nProceed deleting it?",sCMDLS_CLEAR,0);
    }
    else
      sGui->Send(sCMDLS_CLEAR,win);
    return sTRUE;

  case sCMDLS_OPEN:
    if(dirty)
    {
      (new sDialogWindow)->InitOkCancel(win,"Open File","Current Project is not saved.\nProceed deleting it?",sCMDLS_RESERVED,0);
    }
    else
      sGui->Send(sCMDLS_RESERVED,win);
    return sTRUE;
  case sCMDLS_RESERVED:
    file->OkCmd = sCMDLS_READ;
    file->CancelCmd = sCMDLS_RESERVED-2;
    file->SendTo = win;
    sGui->AddApp(file);
    return sTRUE;

  case sCMDLS_SAVEAS:
    file->OkCmd = sCMDLS_WRITE;
    file->CancelCmd = sCMDLS_RESERVED-2;
    file->SendTo = win;
    sGui->AddApp(file);
    return sTRUE;
  case sCMDLS_SAVE:
    sGui->Send(sCMDLS_WRITE,win);
    return sTRUE;

  case sCMDLS_EXIT:
    if(dirty)
    {
      (new sDialogWindow)->InitOkCancel(win,"Exit","Current Project is not saved.\nProceed exit?",sCMDLS_RESERVED-1,0);
    }
    else
      sGui->Send(sCMDLS_RESERVED-1,win);
    return sTRUE;
  case sCMDLS_RESERVED-1:
    win->Parent->RemChild(win);
    return sTRUE;

  case sCMDLS_CLEAR:
    obj->Clear();
    return sTRUE;
  case sCMDLS_READ:
    if(file->Parent)
      file->Parent->RemChild(file);
    file->GetPath(path,sizeof(path));
    di = sDiskRoot->Find(path,sDIF_EXISTING);
    if(!di)
    {
      (new sDialogWindow)->InitOk(win,"Open File","Load Failed\nFile not Found",0);
      sGui->Send(sCMDLS_CLEAR,win);
    }
    else
    {
      data = mem = (sU32 *)di->Load();
      if(!mem)
      {
        (new sDialogWindow)->InitOk(win,"Open File","Load Failed\nDisk Error",0);
        sGui->Send(sCMDLS_CLEAR,win);
      }
      else
      {
        if(!obj->Read(data))
        {
          (new sDialogWindow)->InitOk(win,"Open File","Load Failed\nFile Format Error",0);
          sGui->Send(sCMDLS_CLEAR,win);
        }
        delete[] mem;
      }
    }
    return sTRUE;
  case sCMDLS_WRITE:
    if(file->Parent)
      file->Parent->RemChild(file);
    data = mem = new sU32[maxsave/4];
    if(!obj->Write(data))
    {
      (new sDialogWindow)->InitOk(win,"Save File","Save Failed\nInternal Data Error",0);
    }
    else
    {
      file->GetPath(path,sizeof(path));
      di = sDiskRoot->Find(path,sDIF_CREATE);
      if(di==0 || !di->Save(mem,(data-mem)*4))
      {
        (new sDialogWindow)->InitOk(win,"Save File","Save Failed\nDisk Error",0);
      }
    }
    delete[] mem;
    return sTRUE;

  case sCMDLS_BROWSER:
    browser = new sBrowserApp;
    browser->AddTitle("Browser");
    file->GetPath(path,sizeof(path));
    browser->SetPath(path);
    sGui->AddApp(browser);
    return sTRUE;

  case sCMDLS_RESERVED-2:
    if(file->Parent)
      file->Parent->RemChild(file);
    return sTRUE;
  default:
    return sFALSE;
  }
}

