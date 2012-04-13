// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "appbrowser.hpp"

/****************************************************************************/

/****************************************************************************/
/***                                                                      ***/
/***   Tree Window                                                        ***/
/***                                                                      ***/
/****************************************************************************/

class sBrowserTree : public sGuiWindow
{
  sInt DragStartX,DragStartY;
public:
  sBrowserTree();
  ~sBrowserTree();
  void Tag();
  void SetRoot(sDiskItem *root);
  void Select(sDiskItem *dir);
  void ClearChilds(sDiskItem *dir);
  void OnPaint();
  void OnCalcSize();
  void OnDrag(sDragData &dd);
  void OnKey(sU32 key);

  sChar GetState(sDiskItem *dir);
  void PaintDir(sDiskItem *dir,sInt &pos,sInt indent);
  void DragDir(sDiskItem *dir,sInt &pos,sInt indent,sInt mx,sInt my);
  void DirSelect(sDiskItem *dir);
  void DirNext(sDiskItem *dir);
  void DirPrev(sDiskItem *dir);

  sInt Height;

  sDiskItem *Dir;
  sDiskItem *SelDir;
  sU32 DIKey;
  sBool ScrollToSelection;
  sInt NewScrollValue;
  sInt SerialSelect;
  sInt CalcSizeY;
  sDiskItem *SerialPrev;
};

/****************************************************************************/

sBrowserTree::sBrowserTree()
{
  Dir = 0;
  SelDir = 0;
  ScrollToSelection = 0;
  DIKey = sDiskRoot->AllocKey();
  CalcSizeY = 0;
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
  ScrollToSelection = 1;
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

void sBrowserTree::OnKey(sU32 key)
{
  switch(key&0x8001ffff)
  {
  case sKEY_DOWN:
    if(SelDir)
    {
      SerialSelect = 0;
      DirNext(Dir);
    }
    else
    {
      SelDir = Dir;
    }
    break;
  case sKEY_UP:
    if(SelDir)
    {
      SerialPrev = 0;
      DirPrev(Dir);
    }
    else
    {
      SelDir = Dir;
    }
    break;
  case sKEY_ENTER:
    if(SelDir)
    {
      DirSelect(SelDir);
    }
    break;
  case sKEY_LEFT:
    if(SelDir)
    {
      DirSelect(SelDir);
      SelDir->SetUserData(DIKey,0);
    }
    break;
  case sKEY_RIGHT:
    if(SelDir)
    {
      DirSelect(SelDir);
      SelDir->SetUserData(DIKey,1);
    }
    break;
  case sKEY_SPACE:
    Post(sBTCMD_SELDIR);
    break;
  case sKEY_TAB:
    Post(sBWCMD_FOCLIST);
    break;
  case sKEY_ESCAPE:
    Post(sBWCMD_EXIT);
    break;
  }
}

void sBrowserTree::OnPaint()
{
  sInt pos;
  sRect r;

  NewScrollValue = -1;

  Height = sPainter->GetHeight(sGui->FixedFont)+2;
  pos = 0;
  PaintDir(Dir,pos,0);
  SizeY = Height*pos;
  CalcSizeY = SizeY;

  r = Client;
  r.y0 = Client.y0 + pos*Height;
  r.y1 = Client.y1;
  if(r.y0<r.y1)
    sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_BACK]);

  if(ScrollToSelection && NewScrollValue!=-1)
  {
    sRect r;
    r.Init(Client.x0,NewScrollValue,Client.x1,NewScrollValue+Height-1);
    ScrollTo(r);
  }
  ScrollToSelection = 0;
}

void sBrowserTree::OnCalcSize()
{
  SizeX = 0;
  SizeY = CalcSizeY;
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
    if(ScrollToSelection)
      NewScrollValue = r.y0;
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

  if(MMBScrolling(dd,DragStartX,DragStartY)) return;

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
    DirSelect(dir);
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

void sBrowserTree::DirSelect(sDiskItem *dir)
{
  sInt state;
  state = GetState(dir);
  if(state=='?')
  {
    dir->Cmd(sDIC_FINDALL,0,0);
    state = GetState(dir);
  }
  if(state=='-')
  {
    dir->SetUserData(DIKey,0);
  }
  if(state=='+')
  {
    dir->SetUserData(DIKey,1);
    ClearChilds(dir);
  }
}

void sBrowserTree::DirNext(sDiskItem *dir)
{
  sInt i;
  sDiskItem *di;
  if(dir->GetBool(sDIA_ISDIR))
  {
    if(SerialSelect)
    {
      SelDir = dir;
      SerialSelect = 0;
      ScrollToSelection = 1;
    }
    else
    {
      SerialSelect = (dir==SelDir);
    }
    if(dir->GetUserData(DIKey))
    {
      for(i=0;;i++)
      {
        di = dir->GetChild(i);
        if(!di) break;
        DirNext(di);
      }
    }
  }
}

void sBrowserTree::DirPrev(sDiskItem *dir)
{
  sInt i;
  sDiskItem *di;
  if(dir->GetBool(sDIA_ISDIR))
  {
    if(SelDir == dir)
    {
      SelDir = SerialPrev;
      SerialSelect = 0;
      ScrollToSelection = 1;
    }
    else
    {
      SerialPrev = dir;
    }
    if(dir->GetUserData(DIKey))
    {
      for(i=0;;i++)
      {
        di = dir->GetChild(i);
        if(!di) break;
        DirPrev(di);
      }
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
  sInt DragStartX,DragStartY;
  void Popup(sDiskItem *);
public:
  sBrowserList();
  void Tag();
  void SetDir(sDiskItem *dir);
  void Select(sDiskItem *di);
  void OnCalcSize();
  void OnPaint();
  void OnKey(sU32 key);
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
  sInt i,max;
  sRect r;
  sDiskItem *di;
  if(dir->GetBool(sDIA_INCOMPLETE))
    dir->Cmd(sDIC_FINDALL,0,0);
  Dir = dir;
  max = Dir->GetChildCount();
  LastSelection = 0;
  for(i=0;i<max;i++)
  {
    di = Dir->GetChild(i);
    if(di->GetUserData(DIKey))
    {
      LastSelection = di;
      r.Init(Client.x0,Client.y0+(i)*Height,Client.x0,Client.y0+(i+1)*Height);
      ScrollTo(r);
    }
  }
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

void sBrowserList::OnKey(sU32 key)
{
  sInt i,max;
  sRect r;
  switch(key&0x8001ffff)
  {
  case sKEY_TAB:
    Post(sBWCMD_FOCTREE);
    break;
  case sKEY_ESCAPE:
    Post(sBWCMD_EXIT);
    break;
  case sKEY_SPACE:
  case sKEY_ENTER:
    sGui->Post(sBLCMD_DOUBLE,this);
    break;

  case sKEY_UP:
    if(LastSelection==0)
    {
      if(LastSelection)
      {
        Select(LastSelection);
        sGui->Post(sBLCMD_SELECT,this);
        r.Init(Client.x0,Client.y0,Client.x0,Client.y0+Height);
        ScrollTo(r);
      }
    }
    else
    {
      max = Dir->GetChildCount();
      for(i=1;i<max;i++)
      {
        if(LastSelection==Dir->GetChild(i))
        {
          Select(Dir->GetChild(i-1));
          sGui->Post(sBLCMD_SELECT,this);
          r.Init(Client.x0,Client.y0+(i-1)*Height,Client.x0,Client.y0+(i)*Height);
          ScrollTo(r);
          break;
        }
      }
    }
    break;

  case sKEY_DOWN:
    if(LastSelection==0)
    {
      LastSelection = Dir->GetChild(0);  
      if(LastSelection)
      {
        Select(LastSelection);
        sGui->Post(sBLCMD_SELECT,this);
        r.Init(Client.x0,Client.y0,Client.x0,Client.y0+Height);
        ScrollTo(r);
      }
    }
    else
    {
      max = Dir->GetChildCount();
      for(i=0;i<max-1;i++)
      {
        if(LastSelection==Dir->GetChild(i))
        {
          Select(Dir->GetChild(i+1));
          sGui->Post(sBLCMD_SELECT,this);
          r.Init(Client.x0,Client.y0+(i+1)*Height,Client.x0,Client.y0+(i+2)*Height);
          ScrollTo(r);
          break;
        }
      }
    }
    break;

  case sKEY_BACKSPACE:
    OnCommand(sBLCMD_PARENT);
    break;
  case 'x':
    OnCommand(sDIC_EXECUTE);
    break;
  }
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
    else if(!di->Support(sDIA_DATA))
      sSPrintF(buffer,sizeof(buffer),"");
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

  if(MMBScrolling(dd,DragStartX,DragStartY)) return;

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
      else if(dd.Flags & sDDF_DOUBLE)
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
  sDiskItem *di,*dir;
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

  case sBLCMD_PARENT:
    di = Dir->GetParent();
    if(di)
    {
      dir = Dir;
      SetDir(di);
      Select(dir);
    }
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

  mf->AddMenu("parent",sBLCMD_PARENT,sKEY_BACKSPACE);
  if(di->Support(sDIC_EXECUTE))
    mf->AddMenu("execute",sDIC_EXECUTE,'x');
  dp = di->GetParent();
/*
  if(dp)
  {
    if(dp->Support(sDIC_DELETE))
      mf->AddMenu("delete",sDIC_DELETE,sKEY_DELETE);
    if(dp->Support(sDIA_NAME))
      mf->AddMenu("rename",sDIA_NAME,'r');
  }
*/
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
  ExitCmd = 0;
  SendTo = this;
  TreeFocusDelay = 0;
}

void sBrowserApp::Tag()
{
  sGuiWindow::Tag();
  sBroker->Need(Root);
  sBroker->Need(List);
  sBroker->Need(Tree);
  sBroker->Need(CustomWindow);
}

void sBrowserApp::SetRoot(sDiskItem *root)
{
  Root = root;
  Tree->SetRoot(Root);
  Tree->Select(Root);
  List->SetDir(Root);
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
  if(TreeFocusDelay)
  {
    TreeFocusDelay = 0;
    sGui->Post(sBWCMD_FOCLIST,this);
  }
}

void sBrowserApp::TreeFocus()
{
  TreeFocusDelay = 1;
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
      sGui->Post(DirCmd,SendTo);
    }
    else
    {
      sGui->Post(DoubleCmd,SendTo);
    }
    return sTRUE;
  case sBWCMD_FOCTREE:
    sGui->SetFocus(Tree);
    return sTRUE;
  case sBWCMD_FOCLIST:
    sGui->SetFocus(List);
    return sTRUE;
  case sBWCMD_EXIT:
    if(ExitCmd)
      Post(ExitCmd);
    else
      KillMe();
    return sTRUE;
  default:
    return sFALSE;
  }
}

void sBrowserApp::Reload()
{
  List->Dir->Cmd(sDIC_RELOAD,0,0);
}

void sBrowserApp::GoParent()
{
  List->OnCommand(sBLCMD_PARENT);
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
  PathCon   = new sControl; PathCon  ->EditString(0,Path,"Path",sizeof(Path)); PathCon->EscapeCmd=2;
  NameCon   = new sControl; NameCon  ->EditString(0,Name,"Name",sizeof(Path)); NameCon->EnterCmd=1; NameCon->EscapeCmd=2;
  OkCon     = new sControl; OkCon    ->Button("Ok",1);
  CancelCon = new sControl; CancelCon->Button("Cancel",2);
  ReloadCon = new sControl; ReloadCon->Button("Reload",6);
  ParentCon = new sControl; ParentCon->Button("Parent",7);

  AddChild(Browser);
  AddChild(PathCon);
  AddChild(NameCon);
  AddChild(OkCon);
  AddChild(CancelCon);
  AddChild(ReloadCon);
  AddChild(ParentCon);

  SendTo = this;
  OkCmd = 0;
  CancelCmd = 0;
  Path[0] = 0;
  Name[0] = 0;

  Browser->DirCmd = 3;
  Browser->FileCmd = 4;
  Browser->DoubleCmd = 5;
  Browser->ExitCmd = 2;

  SetPath("disks/c:/nxn");
}

void sFileWindow::OnLayout()
{
  sInt h;
  sInt y;
  sInt x0,xs;

  h = sPainter->GetHeight(sGui->FixedFont)+8;
  y = Client.y1;

  y-=10;
  x0 = Client.x0+5;
  xs = Client.XSize()-10;
  CancelCon->Position.Init(x0+xs*0/4+4,y-h*3/2,x0+xs*1/4-5,y);
  ParentCon->Position.Init(x0+xs*1/4+4,y-h*3/2,x0+xs*2/4-5,y);
  ReloadCon->Position.Init(x0+xs*2/4+4,y-h*3/2,x0+xs*3/4-5,y);
  OkCon    ->Position.Init(x0+xs*3/4+4,y-h*3/2,x0+xs*4/4-5,y);

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

  case 6:
    Browser->Reload();
    return sTRUE;
  case 7:
    Browser->GoParent();
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

void sFileWindow::GetFile(sChar *buffer,sInt size)
{
  sCopyString(buffer,Name,size);
}

void sFileWindow::SetFocus(sBool save)
{
  if(save)
    sGui->SetFocus(NameCon);
  else
    Browser->OnCommand(sBWCMD_FOCLIST);
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
/*
void sFileWindow::OnDrag(sDragData &dd)
{
  MMBScrolling(dd,DragStartX,DragStartY);
}
*/
/****************************************************************************/
/***                                                                      ***/
/***   Load / Save / New / Exit / SaveAs                                  ***/
/***                                                                      ***/
/****************************************************************************/

#define BACKUPA "backup/"
#define BACKUPQ "backup/"

sBool sImplementFileMenu(sU32 cmd,sGuiWindow *win,sObject *obj,sFileWindow *file,sBool dirty,sInt maxsave)
{
  sDiskItem *di;
  sBrowserApp *browser;
  sU32 *mem,*data;
  static sChar path[sDI_PATHSIZE];
  static sChar cancelpath[sDI_PATHSIZE];

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
      (new sDialogWindow)->InitOkCancel(win,"Open File","Current Project is not saved.\nProceed deleting it?",sCMDLS_READx,0);
    }
    else
      sGui->Send(sCMDLS_READx,win);
    return sTRUE;
  case sCMDLS_READx:
    file->OkCmd = sCMDLS_READ;
    file->CancelCmd = sCMDLS_CANCEL;
    file->SendTo = win;
    sGui->AddApp(file);
    file->SetFocus(0);
    return sTRUE;

  case sCMDLS_MERGE:
    file->OkCmd = sCMDLS_MERGEIT;
    file->CancelCmd = sCMDLS_CANCEL;
    file->SendTo = win;
    sGui->AddApp(file);
    file->SetFocus(0);
    return sTRUE;

  case sCMDLS_SAVEAS:
    file->OkCmd = sCMDLS_WRITE;
    file->CancelCmd = sCMDLS_CANCEL;
    file->SendTo = win;
    sGui->AddApp(file);
    file->SetFocus(1);
    return sTRUE;
  case sCMDLS_SAVE:
    sGui->Send(sCMDLS_WRITE,win);
    return sTRUE;

  case sCMDLS_EXIT:
    if(dirty)
    {
      (new sDialogWindow)->InitOkCancel(win,"Exit","Current Project is not saved.\nProceed exit?",sCMDLS_EXITx,0);
    }
    else
      sGui->Send(sCMDLS_CANCEL,win);
    return sTRUE;
  case sCMDLS_EXITx:
    win->Parent->RemChild(win);
    return sTRUE;

  case sCMDLS_CLEAR:
    obj->Clear();
    return sTRUE;
  case sCMDLS_READ:
    if(file->Parent)
      file->Parent->RemChild(file);
    file->GetPath(path,sizeof(path));
    file->GetPath(cancelpath,sizeof(cancelpath));
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
  case sCMDLS_MERGEIT:
    if(file->Parent)
      file->Parent->RemChild(file);
    file->GetPath(path,sizeof(path));
    di = sDiskRoot->Find(path,sDIF_EXISTING);
    if(!di)
    {
      (new sDialogWindow)->InitOk(win,"Merge File","Merge Failed\nFile not Found",0);
      sGui->Send(sCMDLS_CLEAR,win);
    }
    else
    {
      data = mem = (sU32 *)di->Load();
      if(!mem)
      {
        (new sDialogWindow)->InitOk(win,"Merge File","Merge Failed\nDisk Error",0);
        sGui->Send(sCMDLS_CLEAR,win);
      }
      else
      {
        if(!obj->Merge(data))
        {
          (new sDialogWindow)->InitOk(win,"Merge File","Merge Failed\nFile Format Error",0);
          sGui->Send(sCMDLS_CLEAR,win);
        }
        delete[] mem;
      }
    }
    file->SetPath(cancelpath);
    return sTRUE;
  case sCMDLS_WRITE:
    if(file->Parent)                    // destroy window
      file->Parent->RemChild(file);
    file->GetPath(path,sizeof(path));

    di = sDiskRoot->Find(path,sDIF_CREATE);     // write it out
    if(di)
    {
      sDirEntry *de,*dedel;
      sChar *s0,*s1;
      sInt len;
      sChar backup[sDI_PATHSIZE];
      sInt best;
      sChar buffer[128];
      sInt i;
      sU8 *mem;
      sInt size;

      if(!sSystem->CheckDir("backup"))
        sSystem->MakeDir("backup");

      sCopyString(backup,"backup/",sizeof(backup));
      s0 = sFileFromPathString(path);
      s1 = sFileExtensionString(s0);
      if(s1==0)
        len = sGetStringLen(s0);
      else 
        len = s1-s0;
      sAppendString(backup,s0,sizeof(backup));
      s1 = sFileExtensionString(backup);
      if(s1) *s1 = 0;

      dedel = de = sSystem->LoadDir("backup");
      best = 0;
      while(de->Name[0])
      {
        if(!de->IsDir && sCmpMemI(de->Name,s0,len)==0 && de->Name[len]=='#')
        {
          i = sAtoi(de->Name+len+1);
          if(i>best)
            best = i;
        }
        de++;
      }
      delete[] dedel; de=dedel=0;
      sSPrintF(buffer,sizeof(buffer),"#%04d.k",best+1);
      sAppendString(backup,buffer,sizeof(buffer));

      size = di->GetBinarySize(sDIA_DATA);
      mem = (sU8 *)di->Load();
      if(mem)
      {
        if(!sSystem->SaveFile(backup,mem,size))
        {
          (new sDialogWindow)->InitOkCancel(win,"Save File","Backup Failed\nPress OK to continue,\nCANCEL to abort saving.",sCMDLS_WRITEb,0);
          return sTRUE;
        }
        delete[] mem;
        mem = 0;
      }
    }
    /* fall */
  case sCMDLS_WRITEb:
    data = mem = new sU32[maxsave/4];   // create save data
    if(!obj->Write(data))
    {
      (new sDialogWindow)->InitOk(win,"Save File","Save Failed\nInternal Data Error",0);
    }
    else
    {
      di = sDiskRoot->Find(path,sDIF_CREATE);     // write it out
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
    browser->TreeFocus();
    return sTRUE;

  case sCMDLS_CANCEL:
    if(file->Parent)
      file->Parent->RemChild(file);
    return sTRUE;

  case sCMDLS_QUICKSAVE:
    sSystem->DeleteFile(BACKUPQ"quicksave-9.k");
    sSystem->RenameFile(BACKUPQ"quicksave-8.k",BACKUPQ"quicksave-9.k");
    sSystem->RenameFile(BACKUPQ"quicksave-7.k",BACKUPQ"quicksave-8.k");
    sSystem->RenameFile(BACKUPQ"quicksave-6.k",BACKUPQ"quicksave-7.k");
    sSystem->RenameFile(BACKUPQ"quicksave-5.k",BACKUPQ"quicksave-6.k");
    sSystem->RenameFile(BACKUPQ"quicksave-4.k",BACKUPQ"quicksave-5.k");
    sSystem->RenameFile(BACKUPQ"quicksave-3.k",BACKUPQ"quicksave-4.k");
    sSystem->RenameFile(BACKUPQ"quicksave-2.k",BACKUPQ"quicksave-3.k");
    sSystem->RenameFile(BACKUPQ"quicksave-1.k",BACKUPQ"quicksave-2.k");
    data = mem = new sU32[maxsave/4];   // create save data
    if(!obj->Write(data))
    {
      (new sDialogWindow)->InitOk(win,"Save File","Save Failed\nInternal Data Error",0);
    }
    else
    {
      if(!sSystem->SaveFile(BACKUPQ"quicksave-1.k",(sU8*)mem,(data-mem)*4))
      {
        (new sDialogWindow)->InitOk(win,"Save File","Quicksave Failed\nDisk Error",0);
      }
    }
    delete[] mem;
    return sTRUE;

  case sCMDLS_AUTOSAVE:
    sSystem->DeleteFile(BACKUPA"autosave-9.k");
    sSystem->RenameFile(BACKUPA"autosave-8.k",BACKUPA"autosave-9.k");
    sSystem->RenameFile(BACKUPA"autosave-7.k",BACKUPA"autosave-8.k");
    sSystem->RenameFile(BACKUPA"autosave-6.k",BACKUPA"autosave-7.k");
    sSystem->RenameFile(BACKUPA"autosave-5.k",BACKUPA"autosave-6.k");
    sSystem->RenameFile(BACKUPA"autosave-4.k",BACKUPA"autosave-5.k");
    sSystem->RenameFile(BACKUPA"autosave-3.k",BACKUPA"autosave-4.k");
    sSystem->RenameFile(BACKUPA"autosave-2.k",BACKUPA"autosave-3.k");
    sSystem->RenameFile(BACKUPA"autosave-1.k",BACKUPA"autosave-2.k");
    data = mem = new sU32[maxsave/4];   // create save data
    if(!obj->Write(data))
    {
      (new sDialogWindow)->InitOk(win,"Save File","Save Failed\nInternal Data Error",0);
    }
    else
    {
      if(!sSystem->SaveFile(BACKUPA"autosave-1.k",(sU8*)mem,(data-mem)*4))
      {
        (new sDialogWindow)->InitOk(win,"Save File","Autosave Failed\nDisk Error",0);
      }
    }
    delete[] mem;
    return sTRUE;

  default:
    return sFALSE;
  }
}

