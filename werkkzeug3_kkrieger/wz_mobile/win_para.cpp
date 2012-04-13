// This file is distributed under a BSD license. See LICENSE.txt for details.


#include "win_para.hpp"
#include "win_page.hpp"
#include "main_mobile.hpp"

const sInt rowlabel=0;
const sInt rowpara=3;
const sInt rowbuttons=11;
const sInt rowend=13;
  

/****************************************************************************/

ParaWin_::ParaWin_()
{
  Grid = new sGridFrame;
  AddChild(Grid);
  Grid->SetGrid(rowend,4,0,sPainter->GetHeight(sGui->PropFont)+2);
  EditOp = 0;


  Tools = new sToolBorder;
  Tools->AddLabel(".parameters");
  AddBorder(Tools);
  AddScrolling(0,1);
}

ParaWin_::~ParaWin_()
{
}

void ParaWin_::Advance(GenPara *para)
{
  sInt width;
  
  RowStart = Row;

  width = para->Count*2;
  if(para->Type==GPT_STRING || 
     para->Type==GPT_LINK || 
     para->Type==GPT_FILENAME ||
     para->Type==GPT_TEXT) width=2;
  if(para->Flags & GPF_WIDER1)  width += 1;
  if(para->Flags & GPF_WIDER2)  width += 2;

  Row+=width;
  if(Row>rowbuttons)
  {
    Line++;
    RowStart = rowpara;
    Row = rowpara+width;
  }

  if(para->Flags & GPF_FULLWIDTH) Row = rowbuttons;
}

void ParaWin_::AddBox(const sChar *label,sInt pos,sInt cmd)
{
  sControl *con;
  con = new sControl;
  con->Button(label,cmd);
  con->LayoutInfo.Init(12-pos,Line,13-pos,Line+1);
  Grid->AddChild(con);
}

void ParaWin_::SetOp(GenOp *op)
{
  EditOp = op;
  sControl *con;
  GenPara *para;
  Grid->RemChilds();

  static sInt dummy[64];

  App->StatusLog = "";
  LinkParaId = -1;
  StoreNameCount = 0;

  if(EditOp)
  {
    Line = 0;
    Grid->AddLabel(op->Class->Name,rowpara,rowbuttons,Line);
    Line++;

    con = new sControl;
    con->EditString(0,op->StoreName,0,op->StoreName.Size());
    con->LayoutInfo.Init(rowpara,Line,rowbuttons,Line+1);
    Grid->AddChild(con);

    con = new sControl;
    con->Label("Store Name");
    con->LayoutInfo.Init(rowlabel,Line,rowpara,Line+1);
    con->Style |= sCS_LEFT;
    Grid->AddChild(con);

    Line++;

    Grid->Add(new sMenuSpacerControl,rowlabel,rowend,Line);

    sFORLIST(&op->Class->Para,para)
    {
      if(para->Flags & GPF_INVISIBLE)
        continue;

      if(para->Flags & GPF_LABEL)
      {
        Line++;
        con = new sControl;
        con->Label(para->Name);
        con->LayoutInfo.Init(rowlabel,Line,rowbuttons,Line+1);
        con->Style |= sCS_LEFT;
        Grid->AddChild(con);
        Row = rowpara;
      }

      if(para->ParaId)
      {
        Advance(para);
        op->FindValue(para->ParaId)->MakeGui(Grid,RowStart,Row,Line,this);
//        if(para->Type==GPT_TEXT)
//          Line+=9;
      }
/*
      switch(para->Type)
      {
      case GPT_LABEL:
        break;

      case GPT_CHOICE:
      case GPT_FLAGS:
      case GPT_INT:
      case GPT_FLOAT:
        Advance(para);
        op->FindValue(para->ParaId)->MakeGui(Grid,RowStart,Row,Line);
        break;

      default:                    // nothing bad happens here...
        break;
      }
*/
    }
  }

  Grid->Flags |= sGWF_LAYOUT;
}


void ParaWin_::OpList(GenPage *page)
{
  GenOp *op;

  sMenuFrame *mf = new sMenuFrame;

  StoreNameCount = 0;
  sFORALL(page->Ops,op)
  {
    if(op->StoreName[0] && StoreNameCount<256)
    {
      StoreNames[StoreNameCount] = op->StoreName;
      mf->AddMenu(op->StoreName,CMD_PARA_STORENAME+StoreNameCount,0);
      StoreNameCount++;
    }
  }

  mf->SendTo = this;
  mf->AddBorder(new sNiceBorder);
  sGui->AddPulldown(mf);
}

sBool ParaWin_::OnCommand(sU32 cmd)
{
  GenValue *val;
  switch(cmd)
  {
  case CMD_PARA_CHANGE:
    if(EditOp)
    {
      EditOp->Changed = 1;
      Doc->Change();
      App->Change();
    }
    return sTRUE;
  case CMD_PARA_RELAYOUT:
    if(EditOp)
    {
      EditOp->Changed = 1;
      Doc->Connect();
      Doc->Change();
      App->Change();
    }
    return sTRUE;
  }
  if(EditOp)
  {
    switch(cmd&~0xff)
    {
    case CMD_PARA_FILEREQ:
      val = EditOp->FindValue(cmd & 0xff);
      if(val && val->Para->Type == GPT_FILENAME)
      {
        GenValueFilename *vf = (GenValueFilename *)val;
        sSystem->FileRequester(vf->Buffer,vf->Size,sFRF_OPEN);
        EditOp->Changed = 1;
        Doc->Change();
        App->Change();
      }
      return sTRUE;

    case CMD_PARA_FOLLOWLINK:
      val = EditOp->FindValue(cmd & 0xff);
      if(val && val->Para->Type == GPT_LINK)
      {
        GenValueLink *vl = (GenValueLink *)val;
        GenOp *op = Doc->FindOp(vl->Buffer);
        if(op)
          App->GotoOp(op);
      }
      return sTRUE;

    case CMD_PARA_PAGELIST:
      {
        sMenuFrame *mf = new sMenuFrame;
        GenPage *page;

        LinkParaId = cmd&255;

        sFORLIST(Doc->Pages,page)
        {
          if(_i<256)
            mf->AddMenu(page->Name,CMD_PARA_PAGENUM+_i,0);
        }

        mf->SendTo = this;
        mf->AddBorder(new sNiceBorder);
        sGui->AddPulldown(mf);
        break;
      }
      return sTRUE;

    case CMD_PARA_OPLIST:
      LinkParaId = cmd&255;

      if(EditOp)
      {
        GenPage *page = Doc->FindPage(EditOp);
        if(page)
          OpList(page);
      }
      return sTRUE;

    case CMD_PARA_PAGENUM:
      {
        sInt pi = cmd&0xff;
        if(pi<Doc->Pages->GetCount())
          OpList(Doc->Pages->Get(pi));
      }
      return sTRUE;

    case CMD_PARA_STORENAME:
      if(LinkParaId>=0 && sInt(cmd&255)<StoreNameCount)
      {
        val = EditOp->FindValue(LinkParaId);
        LinkParaId = -1;
        if(val && val->Para->Type == GPT_LINK)
        {
          GenValueLink *vl = (GenValueLink *)val;
          sCopyString(vl->Buffer,StoreNames[cmd&255],vl->Size);
          OnCommand(CMD_PARA_RELAYOUT);
        }
      }
      return sTRUE;
    }
  }
  return sFALSE;
}

/****************************************************************************/
