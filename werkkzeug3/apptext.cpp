// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "apptext.hpp"

/****************************************************************************/

sBool sTextControl2::LoadFile(sChar *name)
{
  sDiskItem *di;
  sU8 *data;
  sInt len;

  di = sDiskRoot->Find(name);
  if(di)
  {
    len = di->GetBinarySize(sDIA_DATA);
    data = new sU8[len+1];
    if(di->GetBinary(sDIA_DATA,data,len))
    {
      data[len] = 0;
      SetText((sChar *)data);
      sCopyString(PathName,name,sDI_PATHSIZE);
      Post(CursorCmd);
      delete[]data;
      return sTRUE;
    }
    delete[]data;
  }
  return sFALSE;
}

/****************************************************************************/
/****************************************************************************/

sTextApp::sTextApp()
{
  sToolBorder *tb;
  sChar *text;

  tb = new sToolBorder;
  tb->AddMenu("File",1);
  tb->AddMenu("Edit",2);

  Status = new sStatusBorder;
  Status->SetTab(0,StatusLine,75);
  Status->SetTab(1,StatusColumn,75);
  Status->SetTab(2,StatusChanged,75);
  Status->SetTab(3,StatusName,0);

  Edit = new sTextControl2;
  Edit->AddBorder(tb);
  Edit->AddBorder(Status);
  Edit->AddScrolling(1,1);
  Edit->CursorCmd = 10;
  Edit->DoneCmd = 11;
  AddChild(Edit);

  text = "";
  Edit->SetText(text);

  File = 0;
  UpdateStatus();
}

void sTextApp::OnKey(sU32 key)
{
  if(key==sKEY_APPCLOSE)
    Parent->RemChild(this);
}

sBool sTextApp::OnCommand(sU32 cmd)
{
  sMenuFrame *mf;
  sDiskItem *di;
  sInt len;
  sChar buffer[sDI_PATHSIZE];

  switch(cmd)
  {
  case 1:     // popup FILE
    mf = new sMenuFrame;
    mf->SendTo = Edit;
    mf->AddBorder(new sNiceBorder);
    mf->AddMenu("New",sTCC_CLEAR,0);
    mf->AddMenu("Open...",sTCC_OPEN,0);
    mf->AddMenu("Save",sTCC_SAVE,0);
    mf->AddMenu("Save As...",sTCC_SAVEAS,0);
    mf->AddSpacer();
    mf->AddMenu("Browser",12,0);
    mf->AddMenu("Exit",sTCC_EXIT,0);
    sGui->AddPulldown(mf);
    return sTRUE;
  case 2:     // popup EDIT
    mf = new sMenuFrame;
    mf->SendTo = Edit;
    mf->AddBorder(new sNiceBorder);
    mf->AddMenu("Cut",sTCC_CUT,'x'|sKEYQ_CTRL);
    mf->AddMenu("Copy",sTCC_COPY,'c'|sKEYQ_CTRL);
    mf->AddMenu("Paste",sTCC_PASTE,'v'|sKEYQ_CTRL);
    mf->AddMenu("Mark Block",sTCC_BLOCK,'b'|sKEYQ_CTRL);
    sGui->AddPulldown(mf);
    return sTRUE;

  case 3:     // Cancel File Requester
    if(File)
    {
      File->Parent->RemChild(File);
      File = 0;
    }
    return sTRUE;

  case sTCC_OPEN:
    if(!File)
    {
      File = new sFileWindow;
      File->AddTitle("Open File");
      sGui->AddApp(File);
    }
    sGui->SetFocus(File);
    File->OkCmd = 4;
    File->CancelCmd = 3;
    File->SendTo = this;
    Modal = File;
    File->SetPath(Edit->PathName);
    return sTRUE;
  case 4:
    if(File)
    {
      File->GetPath(buffer,sizeof(buffer));
      OnCommand(3);
      if(!Edit->LoadFile(buffer))
      {
        Edit->SetText("");
        (new sDialogWindow)->InitOk(this,"Open File","Load failed",0);
      }
    }
    return sTRUE;

  case sTCC_CLEAR:
    Edit->SetText("");
    return sTRUE;      


  case sTCC_SAVEAS:
    if(!File)
    {
      File = new sFileWindow;
      File->AddTitle("Open File");
      sGui->AddApp(File);
    }
    sGui->SetFocus(File);
    File->OkCmd = 5;
    File->CancelCmd = 3;
    File->SendTo = this;
    Modal = File;
    File->SetPath(Edit->PathName);
    return sTRUE;
  case 5:
    if(File)
    {
      File->GetPath(Edit->PathName,sizeof(Edit->PathName));
      OnCommand(3);
      OnCommand(sTCC_SAVE);
    }
    return sTRUE;
  case sTCC_SAVE:
    di = sDiskRoot->Find(Edit->PathName,sDIF_CREATE);
    if(di)
    {
      len = sGetStringLen(Edit->TextBuffer->Text);
      if(di->SetBinary(sDIA_DATA,(sU8 *)Edit->TextBuffer->Text,len))
      {
        UpdateStatus();
        Changed = 0;
      }
      else
      {
        (new sDialogWindow)->InitOk(this,"Save File","Save failed",0);
      }
    }
    return sTRUE;


  case 10:
    UpdateStatus();
    return sTRUE; 
  case 11:
    UpdateStatus();
    return sTRUE; 
  case 12:
    di = sDiskRoot->Find("Programms/Browser");
    if(di)
      di->Cmd(sDIC_EXECUTE,0,0);
    return sTRUE; 
  case sTCC_EXIT:
    Parent->RemChild(this);
    return sTRUE;

  case sTCC_CUT:
    Edit->CmdCut();
    return sTRUE;
  case sTCC_COPY:
    Edit->CmdCopy();
    return sTRUE;
  case sTCC_PASTE:
    Edit->CmdPaste();
    return sTRUE;
  case sTCC_BLOCK:
    Edit->CmdBlock();
    return sTRUE;

  default:
    return sFALSE;
  }
}

void sTextApp::UpdateStatus()
{
  sSPrintF(StatusLine,32,"Line %d",Edit->GetCursorY()+1);
  sSPrintF(StatusColumn,32,"Col %d",Edit->GetCursorX()+1);
  if(Edit->PathName[0]==0)
    sCopyString(StatusName,"(noname)",sizeof(StatusName));
  else
    sCopyString(StatusName,Edit->PathName,sizeof(StatusName));
  StatusChanged[0] = 0;
  if(Edit->Changed)
    sCopyString(StatusChanged,"Changed",32);

}

/****************************************************************************/
/****************************************************************************/

