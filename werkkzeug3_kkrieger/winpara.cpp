// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "winpara.hpp"
#include "wintimeline.hpp"
#include "winspline.hpp"
#include "winpage.hpp"
#include "winview.hpp"
#include "material11.hpp"
#include "genblobspline.hpp"

/****************************************************************************/
/****************************************************************************/

static const sChar *AnimOpVars[0x40] =
{
  "time","velo","mod","select",                     // 0x00
  "event_s","event_r","event_t","event_c",          // 0x04
  "cam_s","cam_r","cam_t","_0b",                    // 0x08
  "player_s","player_r","player_t","_0f",           // 0x0c
  "leg_l0","leg_r0","leg_l1","leg_r1",              // 0x10
  "leg_l2","leg_r2","leg_l3","leg_r3",              // 0x14
  "_18","_19","attack","leg_times",                 // 0x18
  "matrix_i","matrix_j","matrix_k","matrix_l",      // 0x1c
};

/****************************************************************************/
/****************************************************************************/

void WinParaInfo::Init(sInt f,sInt c,const sChar *n,sInt o)
{
  Animatable = 0;
  SplineKind = 0;
  if(*n=='*')
  {
    Animatable = 1;
    n++;
    if(*n>='1' && *n<='4')
    {
      SplineKind = *n-'0';
      n++;
    }
  }
  ReSetOp = 0;
  ReConnectOp = 0;
  Format=f;
  Channels=c;
  Name=n;
  Offset=o;
}

/****************************************************************************/
/***                                                                      ***/
/***   para window                                                        ***/
/***                                                                      ***/
/****************************************************************************/


WinPara::WinPara()
{
  sInt i;
  Grid = new sGridFrame;
  Grid->SetGrid(12,4,0,sPainter->GetHeight(sGui->PropFont)+2);
  Grid->AddScrolling(0,1);
  AddChild(Grid);
  Op = 0;
  SetThisOp = 0;
  Line = 0;
  Offset = 0;
//  Spline = 0;
  CurrentPage = 0;
  StoreFilter = KC_ANY;
  NextGroup = 0;
  GrayLabels = 0;
  
  for(i=0;i<8;i++)
  {
    TextControl[i] = 0;
    StringEditBuffer[i][0] = 0;
  }
}

sBool WinPara::OnCommand(sU32 cmd)
{
  sInt result;
  sMenuFrame *mf;
  sDialogWindow *diag;
  sInt i,j,val,max;
  WerkPage *page;
  WerkOp *op;
  static sInt PopupX,PopupY;
  sInt PopupNew;
  WerkOpAnim *oa;
  WerkSpline *spline;
  BlobSpline *bs;

//  WerkAnim *anim;
  sChar buffer[256];

  PopupNew = 1;
  page = 0;
  result = sTRUE;
  val = cmd & 0xff;
  StoreFilterTemp = StoreFilter;

  switch(cmd)
  {
  case CMD_PARA_CHANGE:
    if(Op)
    {
      Op->Change(0);
    }
    return sTRUE;

  case CMD_PARA_CHANGEX:
    if(Op)
    {
      Op->Change(1);
    }
    return sTRUE;

  case CMD_PARA_DONEX:
    if(Op)
    {
      if(!SetThisOp)
        SetOp(Op);
      Op->Change(1);
    }
    return sTRUE;

/*
  case CMD_PARA_SPLINE:
    Spline = !Spline;
    App->SetMode(Spline ? 1 : 0);
//    App->SplineWin->SetOp(Op);
    SetOp(Op);
    return sTRUE;
*/
  case CMD_PARA_UNDO:
    if(Op)
    {
      sInt bytes = sMin(KK_MAXUNDODATA,Op->Op.GetDataWords()*4);
      sCopyMem(RedoData,Op->Op.GetEditPtr(0),bytes);
      sCopyMem(Op->Op.GetEditPtr(0),UndoData,bytes);
      sCopyMem(EditData,Op->Op.GetEditPtr(0),bytes);
      Op->Change(0);
    }
    return sTRUE;

  case CMD_PARA_REDO:
    if(Op)
    {
      sInt bytes = sMin(KK_MAXUNDODATA,Op->Op.GetDataWords()*4);
      sCopyMem(Op->Op.GetEditPtr(0),RedoData,bytes);
      sCopyMem(EditData,Op->Op.GetEditPtr(0),bytes);
      Op->Change(0);
    }
    return sTRUE;

  case CMD_PARA_SPECIAL + 0:
    if(Op->Class->Id==0xd0)
    {
      sBool tex[4];

      // this is a hack.
      tex[0] = (Op->Op.GetLink(0)!=0);
      tex[1] = (Op->Op.GetLink(1)!=0);
      tex[2] = (Op->Op.GetLink(2)!=0);
      tex[3] = (Op->Op.GetLink(3)!=0);

      sMaterial11 *tempMat = new sMaterial11;
      sCopyMem(&tempMat->BaseFlags,Op->Op.GetEditPtr(0),48*4);
      tempMat->DefaultCombiner(tex);
      sCopyMem(Op->Op.GetEditPtr(0),&tempMat->BaseFlags,48*4);
      tempMat->Release();

      Op->Change(0);
    }
    return sTRUE;

  case CMD_PARA_ANIMOP:
    if(App->AnimPageWin)
    {
      App->AnimPageWin->OnCommand(CMD_ANIMPAGE_GENCODE);
    }
    return sTRUE;

  case CMD_PARA_SPLINEADD:
    diag = new sDialogWindow;
    NewSplineName[0] = 0;
    diag->InitString(NewSplineName,KK_NAME);
    diag->InitOkCancel(this,"New Spline","Enter Name",CMD_PARA_SPLINEADD2,0);
    return sTRUE;

  case CMD_PARA_SPLINEADD2:
    oa = FindOpAnim();
    if(Op && oa && NewSplineName[0] && oa->Bytecode==KA_SPLINE && App->PageWin->Page)
    {
      sCopyString(oa->SplineName,NewSplineName,KK_NAME);
      spline = new WerkSpline;
      spline->Init(3,NewSplineName,0,1,0);
      App->PageWin->Page->Splines->Add(spline);
      App->Doc->AddSpline(spline);
      App->AnimPageWin->OnCommand(CMD_ANIMPAGE_GENCODE);
    }
    return sTRUE;

  case CMD_PARA_SPLINESELECT:
    if(App->PageWin->Page)
    {
      max = App->PageWin->Page->Splines->GetCount();
      if(max>0)
      {
        mf = new sMenuFrame();
        mf->SendTo = this;
        mf->AddBorder(new sNiceBorder);
        for(i=0;i<max;i++)
          mf->AddMenuSort(App->PageWin->Page->Splines->Get(i)->Name,CMD_PARA_SPLINESEL2+i,0);
        sGui->AddPulldown(mf);
      }
    }
    return sTRUE;

  case CMD_PARA_SPLINEGOTO:
    oa = FindOpAnim();
    if(oa)
    {
      spline = App->Doc->FindSpline(oa->SplineName);
      if(spline)
      {
        App->SetMode(1);
        App->SplineListWin->SetSpline(spline);
      }
    }
    return sTRUE;

  case CMD_PARA_ANIMLINEAR:
    App->AnimPageWin->AddAnimOps(0,NewAnimInfo);
    SetOp(Op);
    return sTRUE;

  case CMD_PARA_ANIMNEW:
    diag = new sDialogWindow;
    NewSplineName[0] = 0;
    diag->InitString(NewSplineName,KK_NAME);
    diag->InitOkCancel(this,"New Spline","Enter Name",CMD_PARA_ANIMNEW2,0);
    return sTRUE;

  case CMD_PARA_ANIMNEW2:
    if(NewSplineName[0] && App->PageWin->Page && Info[NewAnimInfo].Channels>=1)
    {
      spline = new WerkSpline;
      spline->Init(Info[NewAnimInfo].Channels,NewSplineName,Info[NewAnimInfo].SplineKind,1,0);
      if(Info[NewAnimInfo].Format==KAF_FLOAT && Op)
      {        
        for(i=0;i<spline->Spline.Count;i++)
        {
          spline->Channel[i].Keys.Array[0].Value = *Op->Op.GetEditPtrF(Info[NewAnimInfo].Offset+i);
          spline->Channel[i].Keys.Array[1].Value = *Op->Op.GetEditPtrF(Info[NewAnimInfo].Offset+i);
        }
      }
      App->PageWin->Page->Splines->Add(spline);
      App->Doc->AddSpline(spline);
    }
    App->AnimPageWin->AddAnimOps(NewSplineName,NewAnimInfo);
    SetOp(Op);
    return sTRUE;

  case CMD_PARA_ANIMOLD:
    App->AnimPageWin->AddAnimOps("",NewAnimInfo);
    SetOp(Op);
    return sTRUE;

  case CMD_PARA_RELOAD:
    if(Op)
    {
      Op->Op.SetBlob(0,0);
      Op->Change(0);
    }
    return sTRUE;

  case CMD_PARA_SETOP:
    if(SetThisOp)
    {
      SetOpNow(SetThisOp);
      SetThisOp = 0;
    }
    return sTRUE;

  case CMD_SPLINE_SORT:
    bs = GetBlobSpline();
    if(bs)
    {
      bs->Sort();
      Op->Change(-1);
      SetOp(Op);
    }
    return sTRUE;

  case CMD_SPLINE_NORM:
    bs = GetBlobSpline();
    if(bs)
    {
      bs->Normalize();
      Op->Change(-1);
      SetOp(Op);
    }
    return sTRUE;

  case CMD_SPLINE_CHANGEMODE:
    if(Op)
    {
      Op->Change(-1);
      SetOp(Op);
    }
    return sTRUE;

  case CMD_SPLINE_SWAPTC:
    bs = GetBlobSpline();
    if(bs)
    {
      bs->SwapTargetCam();
      Op->Change(-1);
    }
    return sTRUE;
  case CMD_SPLINE_SETTARGET:
    bs = GetBlobSpline();
    if(bs)
    {
      bs->SetTarget();
      Op->Change(-1);
    }
    return sTRUE;
  }

  switch(cmd&0xffffff00)
  {
  case CMD_PARA_CHCH:
    if(Op)
    {
      sCopyMem(Op->Op.GetEditPtr(0),EditData,Op->Op.GetDataWords()*4);
      /*if(Info[cmd&0xff].Animatable && !Op->Op.SkipExec)
        Op->Op.CopyEditToAnim();
      else*/
        Op->Change(0);
      if(Info[cmd&0xff].ReSetOp)
        SetOp(Op);
      if(Info[cmd&0xff].ReConnectOp)
        Op->Change(1);
    }
    return sTRUE;

  case CMD_PARA_CHCHX:
    if(Op)
    {
      sCopyMem(Op->Op.GetEditPtr(0),EditData,Op->Op.GetDataWords()*4);
      /*if(Info[cmd&0xff].Animatable && !Op->Op.SkipExec)
        Op->Op.CopyEditToAnim();
      else*/
        Op->Change(1);
      if(Info[cmd&0xff].ReSetOp)
        SetOp(Op);
    }
    return sTRUE;

  case CMD_PARA_PAGES:
    CurrentPage = 0;
    Offset = val;
/*
    mf = new sMenuFrame;
    mf->SendTo = this;
    mf->AddBorder(new sNiceBorder);
    for(i=0;i<App->Doc->Pages->GetCount() && i<=0xff;i++)
      mf->AddMenu(App->Doc->Pages->Get(i)->Name,CMD_PARA_PAGE+i,0);
    sGui->AddPulldown(mf);
*/
    App->OpBrowser(this,CMD_PARA_PAGES2);
    return sTRUE;
    
  case CMD_PARA_PAGES2:
    if(App->OpBrowserWin)
    {
      App->OpBrowserWin->GetFileName(buffer,sizeof(buffer));
      sCopyString(Op->LinkName[Offset],buffer,KK_NAME);
      SetOp(Op);
      Op->Change(1);
      App->OpBrowserOff();
    }
    return sTRUE;

  case CMD_PARA_PAGE:
    CurrentPage = 0;
    if(val>=0 && val<App->Doc->Pages->GetCount())
    {
      page = App->Doc->Pages->Get(val);
      break;
    }
    return sTRUE;

  case CMD_PARA_FILTER:
    StoreFilter = val;
    StoreFilterTemp = StoreFilter;
    PopupNew = 0;
    if(App->PageWin->Page)
    {
      page = App->PageWin->Page;
      break;
    }
    return sTRUE;

  case CMD_PARA_NAMES:
    CurrentPage = 0;
    Offset = val;
    if(Op->Class->Links[val]!=KC_ANY && Op->Class->Links[val]!=0)
    {
      StoreFilterTemp = Op->Class->Links[val];
    }
    else if(Op->GetLink(val))
    {
      i = Op->GetLink(val)->GetResultClass();
      if(i!=KC_ANY)
        StoreFilterTemp = i;
    }
    if(App->PageWin->Page)
    {
      page = App->PageWin->Page;
      break;
    }
    return sTRUE;

  case CMD_PARA_GOTO:
    CurrentPage = 0;
    op = App->Doc->FindName(Op->LinkName[val]);
    if(op)
    {
      App->PageWin->GotoOp(op);
      SetOp(op);
    }
    return sTRUE;

  case CMD_PARA_ANIM:
    NewAnimInfo = val;
    mf = new sMenuFrame;
    mf->SendTo = this;
    mf->AddBorder(new sNiceBorder);
    mf->AddMenu("linear",CMD_PARA_ANIMLINEAR,'l');
    mf->AddMenu("existing spline",CMD_PARA_ANIMOLD,'o');
    mf->AddMenu("new spline",CMD_PARA_ANIMNEW,'n');
    sGui->AddPulldown(mf);
    return sTRUE;

  case CMD_PARA_OP:
    page = CurrentPage;
    CurrentPage = 0;
    if(page && val>=0 && val<page->Ops->GetCount() && Offset>=0 && Offset<KK_MAXLINK)
    {
      j = 1;
      if(val==0)
      {
        Op->LinkName[Offset][0] = 0;
        SetOp(Op);
        Op->Change(1);
      }
      else
      {
        for(i=0;i<page->Ops->GetCount() && j<=0xff;i++)
        {
          op = page->Ops->Get(i);
          if(op->Name[0])
          {
            if(j==val)
            {
              sCopyString(Op->LinkName[Offset],op->Name,KK_NAME);
              SetOp(Op);
              Op->Change(1);
              break;
            }
            j++;
          }
        }
      }
    }
    return sTRUE;

  case CMD_PARA_STRING:
    if(Op)
    {
      Op->Op.SetString(cmd&7,StringEditBuffer[cmd&7]);
      Op->Change(0);
    }
    return sTRUE;

  case CMD_PARA_TEXT:
    if(Op && TextControl[cmd&7])
    {
      Op->Op.SetString(cmd&7,TextControl[cmd&7]->GetText());
      Op->Change(0);
    }
    return sTRUE;

  case CMD_PARA_SPLINESEL2:
    oa = FindOpAnim();
    if(oa && oa->Bytecode==KA_SPLINE && App->PageWin->Page)
    {
      if(val<App->PageWin->Page->Splines->GetCount())
        sCopyString(oa->SplineName,App->PageWin->Page->Splines->Get(val)->Name,KK_NAME);
    }
    return sTRUE;

  case CMD_PARA_FILEREQ:
    if(sSystem->FileRequester(StringEditBuffer[cmd&7],sCOUNTOF(StringEditBuffer[cmd&7]),sFRF_OPEN))
    {
      Op->Op.SetString(cmd&7,StringEditBuffer[cmd&7]);
      Op->Change(0);
    }
    return sTRUE;

  case CMD_SPLINE_ADD:
    AddBlobSplineKey(val);
    return sTRUE;
  case CMD_SPLINE_REM:
    RemBlobSplineKey(val);
    return sTRUE;
  case CMD_PIPE_ADD:
    AddBlobPipeKey(val);
    return sTRUE;
  case CMD_PIPE_REM:
    RemBlobPipeKey(val);
    return sTRUE;
  case CMD_SPLINE_SELECT:
    bs = GetBlobSpline();
    if(bs)
    {
      if(bs->Select==val)
        bs->SetSelect(-1);
      else
        bs->SetSelect(val);
    }
    OnCommand(CMD_PARA_CHANGE);
    return sTRUE;
  }

  if(page)
  {
    CurrentPage = page;
    mf = new sMenuFrame;
    mf->SendTo = this;
    mf->AddBorder(new sNiceBorder);
    j = 1;
    mf->AddMenuSort("<< none >>",CMD_PARA_OP,0);
    for(i=0;i<page->Ops->GetCount() && j<=0xff;i++)
    {
      op = page->Ops->Get(i);
      if(op->Name[0])
      {
        if(StoreFilter==KC_ANY || op->GetResultClass()==StoreFilterTemp)
          mf->AddMenuSort(op->Name,CMD_PARA_OP+j,0);
        j++;
      }
    }
    mf->AddColumn();
    mf->AddCheck("all"     ,CMD_PARA_FILTER+KC_ANY      ,'1',StoreFilterTemp==KC_ANY);
    mf->AddCheck("bitmap"  ,CMD_PARA_FILTER+KC_BITMAP   ,'2',StoreFilterTemp==KC_BITMAP);
    mf->AddCheck("mesh"    ,CMD_PARA_FILTER+KC_MESH     ,'3',StoreFilterTemp==KC_MESH);
    mf->AddCheck("scene"   ,CMD_PARA_FILTER+KC_SCENE    ,'4',StoreFilterTemp==KC_SCENE);
    mf->AddCheck("ipp"     ,CMD_PARA_FILTER+KC_IPP      ,'5',StoreFilterTemp==KC_IPP);
    mf->AddCheck("material",CMD_PARA_FILTER+KC_MATERIAL ,'6',StoreFilterTemp==KC_MATERIAL);
    if(PopupNew)
    {
      sGui->AddPulldown(mf);
      PopupX = mf->Position.x0;
      PopupY = mf->Position.y0;
    }
    else
    {
      sGui->AddWindow(mf,PopupX,PopupY);
    }
    return sTRUE;
  }

  return sFALSE;
}

static void RecHigh(WerkOp *op)
{
  sInt i;
  op->Op.Highlight = 1;
  for(i=0;i<op->Op.GetInputCount();i++)
    if(op->GetInput(i) && !op->GetInput(i)->Op.Highlight)
      RecHigh(op->GetInput(i));
}

WerkOpAnim *WinPara::FindOpAnim()
{
  sInt i,max,j;
  WerkOpAnim *oa;

  j = 0;
  oa = 0;
  if(Op)
  {
    max = Op->AnimOps->GetCount();
    for(i=0;i<max;i++)
    {
      if(Op->AnimOps->Get(i)->Select)
      {
        oa = Op->AnimOps->Get(i);
        j++;
      }
    }
  }

  return j==1?oa:0;
}


WinParaInfo *WinPara::FindInfo(sInt offset)
{
  sInt i;
  for(i=0;i<InfoCount;i++)
    if(Info[i].Offset==offset)
      return &Info[i];
  return 0;
}

void WinPara::SetOp(WerkOp *op)
{
  sVERIFY(SetThisOp==0);
  SetThisOp = op;
  sGui->Post(CMD_PARA_SETOP,this);
}

void WinPara::SetOpNow(WerkOp *op)
{
  sInt i,j;
  sChar *names[8]={"X","Y","Z","W","R","G","B","A"};
//  WerkAnim *anim;
//  sInt n;
  WerkOpAnim *oa;
  sControl *con;
  WerkPage *page;
  WinParaInfo *pi;

  for(i=0;i<App->Doc->Pages->GetCount();i++)
  {
    page = App->Doc->Pages->Get(i);
    for(j=0;j<page->Ops->GetCount();j++)
      page->Ops->Get(j)->Op.Highlight = 0;
  }
  if(op)
    RecHigh(op);

  Grid->Flags &= ~sGWF_DISABLED;
  CurrentPage = 0;
  Op = op;
  Grid->RemChilds();
  Flags |= sGWF_LAYOUT;
  if(op)
  {
    // operator

    j = op->Op.GetDataWords()*4;
    sVERIFY(KK_MAXUNDODATA>=j);
    sCopyMem(UndoData,op->Op.GetEditPtr(0),j);
    sCopyMem(RedoData,op->Op.GetEditPtr(0),j);
    sCopyMem(EditData,op->Op.GetEditPtr(0),j);

    if(op->Class->EditHandler)
      (*op->Class->EditHandler)(App,op);

    // animation parameter

    oa = FindOpAnim();
    EditGroup("Animation Parameter");
    if(oa)
    {
      i = oa->Bytecode;
      if(i>=0x80) i&=0xf0;

      switch(oa->UnionKind)
      {
      case AOU_VECTOR:
        HandleGroup();
        con = new sControl;
        con->EditFloat(CMD_PARA_ANIMOP,&oa->ConstVector.x,0);
        con->InitNum(-1024,1024,0.01f,1.0f);
        con->Zones = 4;
        con->Style |= sCS_ZONES;
        con->LayoutInfo.Init(3,Line,11,Line+1);
        Grid->AddChild(con);
        Label("Const");
        Line++;
        break;
      case AOU_COLOR:
        HandleGroup();
        con = new sControl;
        con->EditURGBA(CMD_PARA_ANIMOP,(sInt *)&oa->ConstColor,0);
        con->InitNum(-1024,1024,0.01f,1.0f);
        con->Zones = 4;
        con->LayoutInfo.Init(3,Line,11,Line+1);
        Grid->AddChild(con);
        Label("Const");
        Line++;
        break;
      case AOU_SCALAR:
        HandleGroup();
        con = new sControl;
        con->EditFloat(CMD_PARA_ANIMOP,&oa->ConstVector.x,0);
        con->InitNum(-1024,1024,0.01f,1.0f);
        con->LayoutInfo.Init(3,Line,11,Line+1);
        Grid->AddChild(con);
        Label("Const");
        Line++;
        break;
      case AOU_NAME:
        HandleGroup();
        con = new sControl;
        con->EditString(CMD_PARA_ANIMOP,oa->SplineName,0,KK_NAME);
        con->LayoutInfo.Init(3,Line,9,Line+1);
        Grid->AddChild(con);
        AddBox(CMD_PARA_SPLINEADD,2,0,"Add");
        AddBox(CMD_PARA_SPLINESELECT,1,0,"..");
        AddBox(CMD_PARA_SPLINEGOTO,0,0,"->");
        Label("Spline Name");
        Line++;
        break;
      case AOU_EASE:
        HandleGroup();
        con = new sControl;
        con->EditFloat(CMD_PARA_ANIMOP,&oa->ConstVector.x,0);
        con->InitNum(0,0.5f,0.01f,1.0f);
        con->Zones = 2;
        con->Style |= sCS_ZONES;
        con->LayoutInfo.Init(3,Line,11,Line+1);
        Grid->AddChild(con);
        Label("Ease In/Out");
        Line++;
        break;
      }
      switch(i)
      {
      case KA_LOADVAR:
        HandleGroup();
        Label("Load Variable");
        goto loadstorevar;
      case KA_STOREVAR:
        HandleGroup();
        Label("Store Variable");
loadstorevar:
        AnimOpBuffer[0] = 0;
        for(i=0;i<32;i++)
        {
          if(i==8||i==16||i==24)
            sAppendString(AnimOpBuffer,"\n",sizeof(AnimOpBuffer));
          sAppendString(AnimOpBuffer,AnimOpVars[i],sizeof(AnimOpBuffer));
          if(i<31)
            sAppendString(AnimOpBuffer,"|",sizeof(AnimOpBuffer));
        }
        con = new sControl;
        con->EditChoice(CMD_PARA_ANIMOP,&oa->Parameter,0,AnimOpBuffer);
        con->LayoutInfo.Init(3,Line,11,Line+1);
        Grid->AddChild(con);
        Line++;
        break;
/*
      case KA_LOADPARA1:
      case KA_LOADPARA2:
      case KA_LOADPARA3:
      case KA_LOADPARA4:
      case KA_STOREPARAFLOAT:
      case KA_STOREPARAINT:
      case KA_STOREPARABYTE:
      case KA_CHANGEPARAFLOAT:
      case KA_CHANGEPARAINT:
      case KA_CHANGEPARABYTE:
        break;
*/
      }

      if(oa->Bytecode>=0x80)
      {
        static sChar *writemask[5] = 
        {
          "",
          "*0|x",
          "*0|x:*1|y",
          "*0|x:*1|y:*2|z",
          "*0|x:*1|y:*2|z:*3|w",
        };
        sControlTemplate temp;

        i = 4;
        if(oa->Bytecode>=0x90)
        {
          pi = FindInfo(oa->Parameter);
          if(pi && pi->Channels>0)
            i = pi->Channels;
        }

        HandleGroup();
        temp.Init();
        temp.Type = sCT_CYCLE;
        temp.Cycle = writemask[i];
        temp.YPos = Line;
        temp.XPos = 3-2;
        temp.AddFlags(Grid,CMD_PARA_ANIMOP,&oa->Bytecode,0);
        Label("WriteMask");
        Line++;
      }
      if(oa->Info->Inputs==0 && oa->InputCount==1)
      {
        sControlTemplate temp;

        HandleGroup();
        temp.Init();
        temp.Type = sCT_CYCLE;
        temp.Cycle = "mul|add|sub|div";
        temp.YPos = Line;
        temp.XPos = 3-2;
        temp.AddFlags(Grid,CMD_PARA_ANIMOP,&oa->AutoOp,0);
        Label("Operation");
        Line++;
      }
      if(NextGroup==0)
      {
        Grid->ScrollTo(0,9999);
      }
    }
  }

  App->AnimPageWin->SetOp(op);
  App->SetMode(MODE_PAGE);
}

void WinPara::Label(const sChar *label)
{
  sControl *con;

  if(*label=='*')
  {
    label++;        // skip "animatable" star
    if(*label>='0' && *label<='9')
      label++;
  }

  con = new sControl;
  con->Label(label);
  con->LayoutInfo.Init(0,Line,3,Line+1);
  con->Style |= sCS_LEFT;
  if(GrayLabels)
    con->Style |= sCS_GRAY;
  Grid->AddChild(con);
}

void WinPara::Reset()
{
  Grid->RemChilds();
  Flags |= sGWF_LAYOUT;
  Op = 0;
}

void WinPara::Tag()
{
  sInt i;
  sGuiWindow::Tag();
  sBroker->Need(Op);
  sBroker->Need(SetThisOp);
  sBroker->Need(CurrentPage);
  for(i=0;i<8;i++)
    sBroker->Need(TextControl[i]);
}

void WinPara::OnPaint()
{
}

/****************************************************************************/

void WinPara::DefaultF(sInt offset,sControl *con,sF32 x)
{
  sVERIFY(Op);
  if(Op->SetDefaults)
  {
    *Op->Op.GetEditPtrF(offset+0) = x;
  }
  else
  {
    con->Default[0] = x;
  }
}

void WinPara::DefaultF2(sInt offset,sControl *con,sF32 x,sF32 y)
{
  sVERIFY(Op);
  if(Op->SetDefaults)
  {
    *Op->Op.GetEditPtrF(offset+0) = x;
    *Op->Op.GetEditPtrF(offset+1) = y;
  }
  else
  {
    con->Default[0] = x;
    con->Default[1] = y;
  }
}

void WinPara::DefaultF3(sInt offset,sControl *con,sF32 x,sF32 y,sF32 z)
{
  sVERIFY(Op);
  if(Op->SetDefaults)
  {
    *Op->Op.GetEditPtrF(offset+0) = x;
    *Op->Op.GetEditPtrF(offset+1) = y;
    *Op->Op.GetEditPtrF(offset+2) = z;
  }
  else
  {
    con->Default[0] = x;
    con->Default[1] = y;
    con->Default[2] = z;
  }
}

/****************************************************************************/

void WinPara::EditOp(WerkOp *op)
{
  sInt i;
  sControl *con;

  Line = 0;

  Grid->RemChilds();
  Flags |= sGWF_LAYOUT;
  Op = op;
  sSetMem(Info,0,sizeof(Info));
  InfoCount = 0;
  GrayLabels = 0;

  if(!Op->SetDefaults)
  {
    for(i=0;i<8;i++)
      TextControl[i] = 0;

    Grid->SetGrid(12,4,0,sPainter->GetHeight(sGui->PropFont)+4);

    con = new sControl;
    con->EditString(CMD_PARA_CHANGEX,op->Name,0,KK_NAME);
    con->ChangeCmd = 0;
    con->LayoutInfo.Init(3,Line,8,Line+1);
    if(!op->Page->Access()) con->Style |= sCS_STATIC;
    Grid->AddChild(con);

    con = new sControl;
    con->EditCycle(CMD_PARA_CHANGEX,&op->Bypass,0,"|Bypass");
    con->LayoutInfo.Init(10,Line,11,Line+1);
    if(!op->Page->Access()) con->Style |= sCS_STATIC;
    Grid->AddChild(con);

    con = new sControl;
    con->EditCycle(CMD_PARA_CHANGEX,&op->Hide,0,"|Hide");
    con->LayoutInfo.Init(11,Line,12,Line+1);
    if(!op->Page->Access()) con->Style |= sCS_STATIC;
    Grid->AddChild(con);

//    AddBox(CMD_PARA_SPLINE ,0,0,"Spline");
    AddBox(CMD_PARA_UNDO ,2,0,"Undo");
    AddBox(CMD_PARA_REDO ,3,0,"Redo");
    Label(op->Class->Name);
    Line++;

    EditGroup("Parameters");
  }
  else
  {
  }
}

sControl *WinPara::AddBox(sU32 cmd,sInt pos,sInt offset,sChar *name)
{
  sControl *con;
  con = 0;
  if(!Op->SetDefaults)
  {
    con = new sControl;
    con->Button(name,cmd|offset);
    con->LayoutInfo.Init(11-pos,Line,12-pos,Line+1);
    if(!Op->Page->Access() && name[0]!='-' && name[1]!='>') con->Style |= sCS_STATIC;
    Grid->AddChild(con);
  }
  return con;
}


void WinPara::AddSpecial(const sChar *name,sInt nr)
{
  sControl *con;

  HandleGroup();

  Label(name);
  con = new sControl;
  con->Button(name,CMD_PARA_SPECIAL+nr);
  con->LayoutInfo.Init(3,Line,12,Line+1);
  Grid->AddChild(con);

  Line++;
}

sControl *WinPara::EditString(const sChar *name,sInt index)
{
  if(!Op->SetDefaults)
  {
    sControl *con;

    HandleGroup();
    con = new sControl;
    sCopyString(StringEditBuffer[index],Op->Op.GetString(index),sizeof(StringEditBuffer[index]));
    con->EditString(CMD_PARA_STRING+index,StringEditBuffer[index],0,sizeof(StringEditBuffer[index]));
    con->LayoutInfo.Init(3,Line,12,Line+1);
    if(!Op->Page->Access()) con->Style |= sCS_STATIC;
    Grid->AddChild(con);
    Label(name);
    Line++;
    return con;
  }
  else
  {
    Op->Op.SetString(index,"");
//    sSetMem(Op->Op.Data+offset+4,0,size);
    return 0;
  }
}

sControl *WinPara::EditFileName(const sChar *name,sInt index)
{
  if(!Op->SetDefaults)
  {
    sControl *con;

    HandleGroup();
    con = new sControl;
    sCopyString(StringEditBuffer[index],Op->Op.GetString(index),sizeof(StringEditBuffer[index]));
    con->EditString(CMD_PARA_STRING+index,StringEditBuffer[index],0,sizeof(StringEditBuffer[index]));
    con->LayoutInfo.Init(3,Line,11,Line+1);
    if(!Op->Page->Access()) con->Style |= sCS_STATIC;
    Grid->AddChild(con);
    Label(name);
    AddBox(CMD_PARA_FILEREQ,0,index,"...");
    Line++;
    return con;
  }
  else
  {
    Op->Op.SetString(index,"");
//    sSetMem(Op->Op.Data+offset+4,0,size);
    return 0;
  }
}

sTextControl *WinPara::EditText(const sChar *name,sInt index)
{
  if(!Op->SetDefaults)
  {
    sTextControl *con;

    HandleGroup();
    con = new sTextControl;
    con->DoneCmd = CMD_PARA_TEXT+index;
    con->SetText(Op->Op.GetString(index));
    con->LayoutInfo.Init(3,Line,11,Line+6);
    con->AddBorder(new sThinBorder);
    con->AddScrolling(1,1);
    TextControl[index] = con;
//    if(!Op->Page->Access()) con->Style |= sCS_STATIC;
    Grid->AddChild(con);
    Label(name);
    Line+=6;
    return con;
  }
  else
  {
    Op->Op.SetString(index,"");
    return 0;
  }
}

sControl *WinPara::EditSpline(const sChar *name,sInt offset)
{
  if(!Op->SetDefaults)
  {
    HandleGroup();

    sControl *con;
    con = new sControl;
    con->EditString(CMD_PARA_CHANGEX,Op->SplineName[offset],0,KK_NAME);
    con->LayoutInfo.Init(3,Line,12,Line+1);
    if(!Op->Page->Access()) con->Style |= sCS_STATIC;
    Grid->AddChild(con);
    Label(name);
    Line++;
    return con;
  }
  else
  {
    return 0;
  }
}

sControl *WinPara::EditLink(const sChar *name,sInt offset)
{
  if(!Op->SetDefaults)
  {
    sControl *con;

    HandleGroup();
    con = new sControl;
    con->EditString(CMD_PARA_CHANGEX,Op->LinkName[offset],0,KK_NAME);
    con->DoneCmd = CMD_PARA_DONEX;
    con->LayoutInfo.Init(3,Line,9,Line+1);
    if(!Op->Page->Access()) con->Style |= sCS_STATIC;
    Grid->AddChild(con);
    AddBox(CMD_PARA_PAGES,2,offset,"...");
    AddBox(CMD_PARA_NAMES,1,offset,"..");
    AddBox(CMD_PARA_GOTO ,0,offset,"->");
    Label(name);
    Line++;
    return con;
  }
  else
  {
    return 0;
  }
}

sControl *WinPara::EditLinkI(const sChar *name,sInt offset,sInt offsetflags)
{
  if(!Op->SetDefaults)
  {
    sControl *con;
    sControlTemplate temp;

    HandleGroup();

    con = new sControl;
    con->EditString(CMD_PARA_CHANGEX,Op->LinkName[offset],0,KK_NAME);
    con->DoneCmd = CMD_PARA_DONEX;
    con->LayoutInfo.Init(3,Line,8,Line+1);
    if(!Op->Page->Access()) con->Style |= sCS_STATIC;
    Grid->AddChild(con);

    temp.Init();
    temp.Type = sCT_CHOICE;
    temp.Cycle = "link|input 1|input 2|input 3";
    temp.CycleBits = 2;
    temp.CycleShift = 24;
    con = new sControl;
    con->InitTemplate(&temp,CMD_PARA_CHCHX+InfoCount,(sInt *)(EditData+offsetflags*4));
    con->LayoutInfo.Init(8,Line,9,Line+1);
    if(!Op->Page->Access()) con->Style |= sCS_STATIC;
    Grid->AddChild(con);

    Info[InfoCount].Init(KAF_INT,1,name,offset);
    Info[InfoCount].ReSetOp = 1;
    InfoCount++;


    AddBox(CMD_PARA_PAGES,2,offset,"...");
    AddBox(CMD_PARA_NAMES,1,offset,"..");
    AddBox(CMD_PARA_GOTO ,0,offset,"->");
    Label(name);
    Line++;
    return con;
  }
  else
  {
    return 0;
  }
}

sControl *WinPara::EditInt(const sChar *name,sInt offset,sInt zones,sInt def,sInt min,sInt max,sF32 step)
{
  if(!Op->SetDefaults)
  {
    sControl *con;

    if(*name==':') 
      Line--;
    else
      HandleGroup();
    con = new sControl;
    con->EditInt(CMD_PARA_CHCH+InfoCount,(sInt *)(EditData+offset*4),0);
    con->InitNum(min,max,step,def);
    con->Zones = zones;
    con->Style|=sCS_ZONES;
    con->LayoutInfo.Init(3,Line,11,Line+1);
    if(*name==':')
      con->LayoutInfo.Init(7,Line,9,Line+1);
    if(!Op->Page->Access()) con->Style |= sCS_STATIC;
    Grid->AddChild(con);

    if(*name==':')
    {
      name++;
      Info[InfoCount].Init(KAF_FLOAT,zones,name,offset);
    }
    else
    {
      Info[InfoCount].Init(KAF_INT,zones,name,offset);
      if(*name=='?') 
      {
        Info[InfoCount].ReConnectOp = 1;
        name++;
      }
      if(Info[InfoCount].Animatable)
        AddBox(CMD_PARA_ANIM,0,InfoCount,"Anim");
      Label(name);
    }
    InfoCount++;
    Line++;
    return con;
  }
  else
  {
    sInt i;
    for(i=0;i<zones;i++)
    {
      *Op->Op.GetEditPtrS(offset+i) = def;
    }
    return 0;
  }
}


sControl *WinPara::EditFloat(const sChar *name,sInt offset,sInt zones,sF32 def,sF32 min,sF32 max,sF32 step)
{
  if(!Op->SetDefaults)
  {
    sControl *con,*box;

    if(*name==':') 
      Line--;
    else
      HandleGroup();
    con = new sControl;
    con->EditFloat(CMD_PARA_CHCH+InfoCount,(sF32 *)(EditData+offset*4),0);
    con->InitNum(min,max,step,def);
    con->Zones = zones;
    con->Style|=sCS_ZONES;
    con->LayoutInfo.Init(*name==':'?9:3,Line,11,Line+1);
    if(!Op->Page->Access()) con->Style |= sCS_STATIC;
    Grid->AddChild(con);
    if(*name==':')
    {
      name++;
      Info[InfoCount].Init(KAF_FLOAT,zones,name,offset);
    }
    else
    {
      Info[InfoCount].Init(KAF_FLOAT,zones,name,offset);
      if(Info[InfoCount].Animatable)
      {
        box = AddBox(CMD_PARA_ANIM,0,InfoCount,"Anim");
        if(AnimCodeWritesTo(Op->Op.GetAnimCode(),offset))
          box->Style |= sCS_STATIC;
      }
      Label(name);
    }
    InfoCount++;
    Line++;
    return con;
  }
  else
  {
    sInt i;
    for(i=0;i<zones;i++)
    {
      *Op->Op.GetEditPtrF(offset+i) = def;
    }
    return 0;
  }
}

sControl *WinPara::EditRGBA(const sChar *name,sInt offset,sU32 def)
{
  if(!Op->SetDefaults)
  {
    sControl *con;

    HandleGroup();
    con = new sControl;
    con->EditURGBA(CMD_PARA_CHCH+InfoCount,(sInt *)(EditData+offset*4),0);
    con->LayoutInfo.Init(3,Line,11,Line+1);
    if(!Op->Page->Access()) con->Style |= sCS_STATIC;
    Grid->AddChild(con);
    Info[InfoCount].Init(KAF_UBYTE,4,name,offset);
    if(Info[InfoCount].Animatable)
      AddBox(CMD_PARA_ANIM,0,InfoCount,"Anim");
    Label(name);
    InfoCount++;
    Line++;
    return con;
  }
  else
  {
    *Op->Op.GetEditPtrU(offset) = def;
    return 0;
  }
}

sControl *WinPara::EditCycle(const sChar *name,sInt offset,sInt def,sChar *cycle)
{
  if(!Op->SetDefaults)
  {
    sControl *con;

    HandleGroup();
    con = new sControl;
    con->EditChoice(CMD_PARA_CHCH+InfoCount,(sInt *)(EditData+offset*4),0,cycle);
    con->LayoutInfo.Init(3,Line,12,Line+1);
    if(!Op->Page->Access()) con->Style |= sCS_STATIC;
    Grid->AddChild(con);
    Info[InfoCount++].Init(KAF_INT,1,name,offset);
    Label(name);
    Line++;
    return con;
  }
  else
  {
    *Op->Op.GetEditPtrU(offset) = def;
    return 0;
  }
}

sControl *WinPara::EditBool(const sChar *name,sInt offset,sInt def)
{
  if(!Op->SetDefaults)
  {
    sControl *con;

    HandleGroup();
    con = new sControl;
    con->EditCycle(CMD_PARA_CHCH+InfoCount,(sInt *)(EditData+offset*4),0,"off|on");
    con->LayoutInfo.Init(3,Line,12,Line+1);
    if(!Op->Page->Access()) con->Style |= sCS_STATIC;
    Grid->AddChild(con);
    Info[InfoCount++].Init(KAF_INT,1,name,offset);
    Label(name);
    Line++;
    return con;
  }
  else
  {
    *Op->Op.GetEditPtrU(offset) = def;
    return 0;
  }
}

sControl *WinPara::EditMask(const sChar *name,sInt offset,sInt def,sInt zones,sChar *picks)
{
  if(!Op->SetDefaults)
  {
    sControl *con;

    HandleGroup();
    con = new sControl;
    con->EditMask(CMD_PARA_CHCH+InfoCount,(sInt *)(EditData+offset*4),0,zones,picks);
    con->LayoutInfo.Init(3,Line,12,Line+1);
    con->Style |= sCS_ZONES;
    if(!Op->Page->Access()) con->Style |= sCS_STATIC;
    Grid->AddChild(con);
    Info[InfoCount++].Init(KAF_INT,1,name,offset);
    Label(name);
    Line++;
    return con;
  }
  else
  {
    *Op->Op.GetEditPtrU(offset) = def;
    return 0;
  }
}

sControl *WinPara::EditFlags(const sChar *name,sInt offset,sInt def,sChar *cycle)
{
  if(!Op->SetDefaults)
  {
    sControlTemplate temp;

    HandleGroup();
    temp.Init();
    temp.Type = sCT_CYCLE;
    temp.Cycle = cycle;
    temp.YPos = Line;
    temp.XPos = 3-2;
    temp.AddFlags(Grid,CMD_PARA_CHCH+InfoCount,(sInt *)(EditData+offset*4),Op->Page->Access()?0:sCS_STATIC);
    Info[InfoCount].Init(KAF_INT,1,name,offset);
    if(*name=='!') 
    {
      Info[InfoCount].ReSetOp = 1;
      name++;
    }
    if(*name=='?') 
    {
      Info[InfoCount].ReConnectOp = 1;
      name++;
    }
    Label(name);
    InfoCount++;
    Line++;
    return 0;
  }
  else
  {
    *Op->Op.GetEditPtrU(offset) = def;
    return 0;
  }
}

void WinPara::EditGroup(const sChar *name)
{
  NextGroup = name;
}

void WinPara::EditSpacer(const sChar *name)
{
  if(!Op->SetDefaults)
  {
    Grid->Add(new sMenuSpacerControl(name),0,12,Line++);
  }
}

void WinPara::HandleGroup()
{
  if(NextGroup)
    EditSpacer(NextGroup);
  NextGroup = 0;
}

void WinPara::EditSetGray(sBool g)
{
  GrayLabels = g;
}


/****************************************************************************/
/***                                                                      ***/
/***   animation page window                                              ***/
/***                                                                      ***/
/****************************************************************************/


const WerkOpAnimInfo AnimOpCmds[] = 
{
  { "nop"         ,KA_NOP             ,1,0            ,AOU_OFF    },
  { "end"         ,KA_END             ,1,0            ,AOU_OFF    },
  { "loadvar"     ,KA_LOADVAR         ,0,AOIF_LOADVAR ,AOU_OFF    },
  { "loadpara"    ,KA_LOADPARA1       ,0,AOIF_LOADPARA,AOU_OFF    },
  { "loadpara"    ,KA_LOADPARA2       ,0,AOIF_LOADPARA,AOU_OFF    },
  { "loadpara"    ,KA_LOADPARA3       ,0,AOIF_LOADPARA,AOU_OFF    },
  { "loadpara"    ,KA_LOADPARA4       ,0,AOIF_LOADPARA,AOU_OFF    },
  { "swizzle.x"   ,KA_SWIZZLEX        ,1,AOIF_ADDOP   ,AOU_OFF    },
  { "swizzle.y"   ,KA_SWIZZLEY        ,1,AOIF_ADDOP   ,AOU_OFF    },
  { "swizzle.z"   ,KA_SWIZZLEZ        ,1,AOIF_ADDOP   ,AOU_OFF    },
  { "swizzle.w"   ,KA_SWIZZLEW        ,1,AOIF_ADDOP   ,AOU_OFF    },
  { "add"         ,KA_ADD             ,2,AOIF_ADDOP   ,AOU_OFF    },
  { "sub"         ,KA_SUB             ,2,AOIF_ADDOP   ,AOU_OFF    },
  { "mul"         ,KA_MUL             ,2,AOIF_ADDOP   ,AOU_OFF    },
  { "div"         ,KA_DIV             ,2,AOIF_ADDOP   ,AOU_OFF    },
  { "frac"        ,KA_MOD             ,2,AOIF_ADDOP   ,AOU_OFF    },
  { "invert"      ,KA_INVERT          ,1,AOIF_ADDOP   ,AOU_OFF    },
  { "neg"         ,KA_NEG             ,1,AOIF_ADDOP   ,AOU_OFF    },
  { "sin"         ,KA_SIN             ,1,AOIF_ADDOP   ,AOU_OFF    },
  { "cos"         ,KA_COS             ,1,AOIF_ADDOP   ,AOU_OFF    },
  { "pulse"       ,KA_PULSE           ,1,AOIF_ADDOP   ,AOU_OFF    },
  { "ramp"        ,KA_RAMP            ,1,AOIF_ADDOP   ,AOU_OFF    },
  { "vector"      ,KA_CONSTV          ,0,AOIF_ADDOP   ,AOU_VECTOR },
  { "color"       ,KA_CONSTC          ,0,AOIF_ADDOP   ,AOU_COLOR  },
  { "scalar"      ,KA_CONSTS          ,0,AOIF_ADDOP   ,AOU_SCALAR },
  { "spline"      ,KA_SPLINE          ,1,AOIF_ADDOP   ,AOU_NAME   },
  { "eventspline" ,KA_EVENTSPLINE     ,1,AOIF_ADDOP   ,AOU_OFF    },
  { "matrix"      ,KA_MATRIX          ,1,AOIF_ADDOP   ,AOU_OFF    },
  { "noise"       ,KA_NOISE           ,1,AOIF_ADDOP   ,AOU_OFF    },
  { "ease"        ,KA_EASE            ,1,AOIF_ADDOP   ,AOU_EASE   },

  { "storevar"    ,KA_STOREVAR        ,1,AOIF_STOREVAR,AOU_MASK   },
  { "storeparaf"  ,KA_STOREPARAFLOAT  ,1,AOIF_STOREPARA,AOU_MASK  },
  { "storeparai"  ,KA_STOREPARAINT    ,1,AOIF_STOREPARA,AOU_MASK  },
  { "storeparab"  ,KA_STOREPARABYTE   ,1,AOIF_STOREPARA,AOU_MASK  },
  { "changeparaf" ,KA_CHANGEPARAFLOAT ,1,AOIF_STOREPARA,AOU_MASK  },
  { "changeparai" ,KA_CHANGEPARAINT   ,1,AOIF_STOREPARA,AOU_MASK  },
  { "changeparab" ,KA_CHANGEPARABYTE  ,1,AOIF_STOREPARA,AOU_MASK  },
  { 0 }
};

/****************************************************************************/

WinAnimPage::WinAnimPage()
{
  Op=0;
  PageMaxX = 32;
  PageMaxY = 32;
  AddBorder(new sScrollBorder);
  AddColumn = 1;

  Clipboard = new sList<WerkOpAnim>;
}

WinAnimPage::~WinAnimPage()
{
}

void WinAnimPage::Tag()
{
  sOpWindow::Tag();
  sBroker->Need(Op);
  sBroker->Need(Clipboard);
}

void WinAnimPage::OnKey(sU32 key)
{
  switch(key&0x8001ffff)
  {
  case sKEY_APPPOPUP:
    sGui->Post(sOIC_MENU,this);
    break;
  case sKEY_DELETE:
  case sKEY_BACKSPACE:
    OnCommand(CMD_ANIMPAGE_DELETE);
    break;
  case 'x':
    OnCommand(CMD_ANIMPAGE_CUT);
    break;
  case 'c':
    OnCommand(CMD_ANIMPAGE_COPY);
    break;
  case 'v':
    OnCommand(CMD_ANIMPAGE_PASTE);
    break;
  case 'a':
    sGui->Post(sOIC_ADD,this);
    break;
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
    sGui->Send(CMD_ANIMPAGE_ADD+(key&0xff)-'1',this);
    break;
  default:
    sOpWindow::OnKey(key);
    break;
  }
}

void WinAnimPage::OnPaint()
{
  sChar buffer[4096];
  sChar *d;
  sU8 *code;
  sInt count;
  static const sChar *hex="0123456789abcdef";

  sOpWindow::OnPaint();
  if(Op)
  {
    code = Op->Op.GetAnimCode();

    d=buffer;
    while(*code!=KA_END)
    {
      count = 1;
      if(*code==KA_CONSTV)
        count = 17;
      if(*code==KA_CONSTC)
        count = 5;
      if(*code==KA_CONSTS)
        count = 5;
      if(*code>=0x80 || *code==KA_SPLINE)
        count = 2;
      if(*code==KA_LOADVAR)
        count = 2;
      if(*code==KA_LOADPARA1)
        count = 2;
      if(*code==KA_LOADPARA2)
        count = 2;
      if(*code==KA_LOADPARA3)
        count = 2;
      if(*code==KA_LOADPARA4)
        count = 2;

      while(count)
      {
        *d++ = hex[(*code)>>4];
        *d++ = hex[(*code)&15];
        code++;
        count--;
      }
      *d++ = ' ';
    }
    *d++ = '.';
    *d++ = 0;
    sPainter->Print(sGui->PropFont,Client.x0,Client.y0,buffer,sGui->Palette[sGC_TEXT]);
  }
}


sBool WinAnimPage::OnCommand(sU32 cmd)
{
  switch(cmd)
  {
  case CMD_ANIMPAGE_GENCODE:
    if(Op)
    {
      Op->Op.CopyEditToAnim();
      Op->ConnectAnim(App->Doc);
    }
    return sTRUE;
  case sOIC_CHANGED:
    CheckOp();
    return sTRUE;
  case sOIC_MENU:
    PopupMenu();
    return sTRUE;
  case sOIC_SHOW:
    return sTRUE;
  case sOIC_SETEDIT:
    App->ParaWin->SetOp(App->ParaWin->Op);
    return sTRUE;

  case CMD_ANIMPAGE_COPY:
    CopyOp();
    return sTRUE;
  case CMD_ANIMPAGE_PASTE:
    PasteOp();
    CheckOp();
    return sTRUE;
  case CMD_ANIMPAGE_CUT:
    CopyOp();
    DelOp();
    CheckOp();
    return sTRUE;
  case CMD_ANIMPAGE_DELETE:
    DelOp();
    CheckOp();
    return sTRUE;

  case CMD_ANIMPAGE_ADD+0:
  case CMD_ANIMPAGE_ADD+1:
  case CMD_ANIMPAGE_ADD+2:
  case CMD_ANIMPAGE_ADD+3:
  case CMD_ANIMPAGE_ADD+4:
    AddColumn = cmd-CMD_ANIMPAGE_ADD;
  case sOIC_ADD:
    PopupAdd();
    return sTRUE;

  default:
    if((cmd&~0xffff)==CMD_ANIMPAGE_OP)
    {
      AddOp(cmd&0xffff);
      return sTRUE;
    }
    break;
  }
  return sFALSE;
}

/****************************************************************************/

void WinAnimPage::SetOp(WerkOp *op) 
{ 
  Op = op; 
}

void WinAnimPage::PopupMenu()
{
  sMenuFrame *mf;
  mf = new sMenuFrame;

  mf->AddMenu("Add"            ,sOIC_ADD,'a');
  mf->AddSpacer();
  mf->AddMenu("Cut"            ,CMD_ANIMPAGE_CUT,'x');
  mf->AddMenu("Copy"           ,CMD_ANIMPAGE_COPY,'c');
  mf->AddMenu("Paste"          ,CMD_ANIMPAGE_PASTE,'p');
  mf->AddMenu("Delete"         ,CMD_ANIMPAGE_DELETE,sKEY_BACKSPACE);
  /*
  mf->AddMenu("Add Load Var"   ,CMD_ANIMPAGE_ADD+0 ,'1');
  mf->AddMenu("Add Load Para"  ,CMD_ANIMPAGE_ADD+1 ,'2');
  mf->AddMenu("Add Op"         ,CMD_ANIMPAGE_ADD+2 ,'3');
  mf->AddMenu("Add Store Var"  ,CMD_ANIMPAGE_ADD+3 ,'4');
  mf->AddMenu("Add Store Para" ,CMD_ANIMPAGE_ADD+4 ,'5');
*/

  mf->SendTo = this;
  mf->AddBorder(new sNiceBorder);
  sGui->AddPopup(mf);
}

void WinAnimPage::PopupAdd()
{
  sMenuFrame *mf;
  sInt i,j;
  sU32 cmd;

  mf = new sMenuFrame;

  mf->AddMenu("Add Load Var"   ,CMD_ANIMPAGE_ADD+0 ,'1');
  mf->AddMenu("Add Load Para"  ,CMD_ANIMPAGE_ADD+1 ,'2');
  mf->AddMenu("Add Op"         ,CMD_ANIMPAGE_ADD+2 ,'3');
  mf->AddMenu("Add Store Var"  ,CMD_ANIMPAGE_ADD+3 ,'4');
  mf->AddMenu("Add Store Para" ,CMD_ANIMPAGE_ADD+4 ,'5');

  mf->AddColumn();

  switch(AddColumn)
  {
  case 0:
  case 3:
    if(AddColumn==0)
      cmd = KA_LOADVAR<<8;
    else
      cmd = (KA_STOREVAR<<8)|(15<<8);

    for(i=0;AnimOpVars[i];i++)
    {
      if(i!=0 && (i&15)==0)
        mf->AddColumn();
      mf->AddMenu(AnimOpVars[i],CMD_ANIMPAGE_OP|cmd|i,0);
    }
    break;
  case 2:
    for(i=0;AnimOpCmds[i].Name;i++)
    {
      if((AnimOpCmds[i].Flags & AOIF_ADDOP) && AnimOpCmds[i].Inputs==1)
        mf->AddMenu(AnimOpCmds[i].Name,CMD_ANIMPAGE_OP|(AnimOpCmds[i].Code<<8),0);
    }
    mf->AddColumn();
    for(i=0;AnimOpCmds[i].Name;i++)
    {
      if((AnimOpCmds[i].Flags & AOIF_ADDOP) && AnimOpCmds[i].Inputs!=1)
        mf->AddMenu(AnimOpCmds[i].Name,CMD_ANIMPAGE_OP|(AnimOpCmds[i].Code<<8),0);
    }
    break;
  case 1:
  case 4:
    if(Op && Op==App->ParaWin->Op)
    {
      for(i=0;i<App->ParaWin->InfoCount;i++)
      {
        cmd = 0;
        if(AddColumn==1)
        {
          if(App->ParaWin->Info[i].Format==KAF_FLOAT)
            cmd = (KA_LOADPARA1+App->ParaWin->Info[i].Channels-1)<<8;
        }
        else
        {
          j = (1<<App->ParaWin->Info[i].Channels)-1;
          if(App->ParaWin->Info[i].Format==KAF_FLOAT)
            cmd = (KA_STOREPARAFLOAT|j)<<8;
          if(App->ParaWin->Info[i].Format==KAF_INT)
            cmd = (KA_STOREPARAINT|j)<<8;
          if(App->ParaWin->Info[i].Format==KAF_UBYTE)
            cmd = (KA_STOREPARABYTE|j)<<8;
          if(!(App->ParaWin->Info[i].Animatable))
            cmd |= 0xc0<<8;
        }
        if(cmd!=0)
          mf->AddMenu(App->ParaWin->Info[i].Name,CMD_ANIMPAGE_OP|cmd|App->ParaWin->Info[i].Offset,0);
      }
    }
    break;
  }

  AddOperatorMenu(mf);
/*
  mf->SendTo = this;
  mf->AddBorder(new sNiceBorder);
  sGui->AddPopup(mf);
  */
}

void WinAnimPage::AddOp(sU32 op)
{
  WerkOpAnim *oa;

  if(Op)
  {
    oa = new WerkOpAnim;
    oa->PosX = CursorX;
    oa->PosY = CursorY;
    oa->Width = CursorWidth;
    oa->Init((op>>8)&0xff,op&0xff);
    oa->Select = 1;

    Op->AnimOps->Add(oa);
    if(CursorY < PageMaxY-1)
      CursorY++;
    sGui->Post(sOIC_CHANGED,this);
  }
}

void WinAnimPage::CheckOp()
{
  if(Op)
  {
    Op->Op.CopyEditToAnim();
    if(Op->ConnectAnim(App->Doc))
    {
      App->ParaWin->SetOp(App->ParaWin->Op);
    }
  }
}

void WinAnimPage::DelOp()
{
  sInt i,max;
  WerkOpAnim *oa;

  if(Op)
  {
    max = Op->AnimOps->GetCount();
    for(i=0;i<max;)
    {
      oa = Op->AnimOps->Get(i);
      if(oa->Select)
      {
        Op->AnimOps->Rem(oa);
        max--;
      }
      else
      {
        i++;
      }
    }
  }
}

void WinAnimPage::CopyOp()
{
  sInt i,max;
  WerkOpAnim *wa,*copy;

  Clipboard->Clear();
  if(Op)
  {
    max = Op->AnimOps->GetCount();
    for(i=0;i<max;i++)
    {
      wa = Op->AnimOps->Get(i);
      if(wa->Select)
      {
        copy = new WerkOpAnim;
        copy->Copy(wa);
        Clipboard->Add(copy);
      }
    }
  }
}

void WinAnimPage::PasteOp()
{
  sInt i,max;
  sInt x,y;
  WerkOpAnim *wa,*copy;

  DeselectOp();
  max = Clipboard->GetCount();
  if(Op && max>0)
  {
    wa = Clipboard->Get(0);
    x = wa->PosX;
    y = wa->PosY;
    for(i=1;i<max;i++)
    {
      wa = Clipboard->Get(i);
      x = sMin(wa->PosX,x);
      y = sMin(wa->PosY,y);
    }
    for(i=0;i<max;i++)
    {
      wa = Clipboard->Get(i);
      if(!CheckDest(wa->PosX-x+CursorX,wa->PosY-y+CursorY,wa->Width))
        return;
    }
    for(i=0;i<max;i++)
    {
      wa = Clipboard->Get(i);
      copy = new WerkOpAnim;
      copy->Copy(wa);
      copy->Select = sTRUE;
      copy->PosX = wa->PosX - x + this->CursorX;
      copy->PosY = wa->PosY - y + this->CursorY;
      Op->AnimOps->Add(copy); 
    }
  }
}

void WinAnimPage::DeselectOp()
{
  sInt i,max;

  if(Op)
  {
    max = Op->AnimOps->GetCount();
    for(i=0;i<max;i++)
    {
      Op->AnimOps->Get(i)->Select = 0;
    }
  }
}

/****************************************************************************/

sInt WinAnimPage::GetOpCount()
{
  if(Op)
    return Op->AnimOps->GetCount();
  else
    return 0;
}

void WinAnimPage::GetOpInfo(sInt i,sOpInfo &oi)
{
  WerkOpAnim *oa;
  sInt j;
  const static sChar *mask[16] = 
  {
    ".0",
    ".x",
    ".y",
    ".xy",
    ".z",
    ".xz",
    ".yz",
    ".xyz",
    ".w",
    ".xw",
    ".yw",
    ".xyw",
    ".zw",
    ".xzw",
    ".yzw",
    ".xyzw",
  };
  const static sChar *mask2[4] =
  {
    ".x",
    ".xy",
    ".xyz",
    ".xyzw",
  };
  const static sChar *AutoOpName[4] =
  {
    "* ",
    "+ ",
    "- ",
    "/ ",
  };

  if(Op && i>=0 && i<Op->AnimOps->GetCount())
  {
    oa = Op->AnimOps->Get(i);
    oi.PosX = oa->PosX;
    oi.PosY = oa->PosY;
    oi.Width = oa->Width;
    oi.Style = oa->Select ? sOIS_SELECT : 0;
    oi.SetName("bla");
    oi.Color = 0xffc0c0c0;
    oi.Wheel = -1;

    sVERIFY(oa->Info);
    oi.SetName(oa->Info->Name);
    if(oa->Info->Inputs==0)
    {
      oi.Style |= sOIS_LOAD;
    }
    if(oa->Info->Flags & AOIF_LOADVAR)
    {
      oi.SetName(AnimOpVars[oa->Parameter]);
    }
    if(oa->Info->Flags & AOIF_STOREVAR)
    {
      oi.Style |= sOIS_STORE;
      oi.SetName(AnimOpVars[oa->Parameter]);
      oi.AppendName(mask[oa->Bytecode&15]);
    }
    if(oa->Info->Flags & AOIF_LOADPARA)
    {
      if(Op && Op==App->ParaWin->Op)
      {
        for(j=0;j<App->ParaWin->InfoCount;j++)
        {
          if(App->ParaWin->Info[j].Offset==oa->Parameter)
          {
            oi.SetName(App->ParaWin->Info[j].Name);
            oi.AppendName(mask2[oa->Bytecode-KA_LOADPARA1]);
            break;
          }
        }
      }
    }
    if(oa->Info->Flags & AOIF_STOREPARA)
    {
      oi.Style |= sOIS_STORE;
      if(Op && Op==App->ParaWin->Op)
      {
        for(j=0;j<App->ParaWin->InfoCount;j++)
        {
          if(App->ParaWin->Info[j].Offset==oa->Parameter)
          {
            if(oa->Bytecode>=KA_CHANGEPARAFLOAT)
            {
              oi.SetName("(");
              oi.AppendName(App->ParaWin->Info[j].Name);
              oi.AppendName(")");
            }
            else
            {
              oi.SetName(App->ParaWin->Info[j].Name);
            }
            oi.AppendName(mask[oa->Bytecode&15]);
          }
        }
      }
    }
    if(oa->Info->Inputs==0 && oa->InputCount==1)
    {
      oi.PrefixName(AutoOpName[oa->AutoOp]);
      oi.Style &= ~sOIS_LOAD;
    }
    if(oa->Error)
      oi.Style |= sOIS_ERROR;
    if(!oa->UsedInCode)
      oi.Color = 0xff989898;

  } 
  else
  {
    sOpWindow::GetOpInfo(0,oi);
  }
}

void WinAnimPage::SelectRect(sRect cells,sInt mode)
{
  sInt i,max;
  WerkOpAnim *oa;
  sRect r;

  if(Op)
  {
    max = Op->AnimOps->GetCount();
    for(i=0;i<max;i++)
    {
      oa = Op->AnimOps->Get(i);
      
      if(mode==sOPSEL_SET)
        oa->Select = sFALSE;

      r.Init(oa->PosX,oa->PosY,oa->PosX+oa->Width,oa->PosY+1);
      if(cells.Hit(r))
      {
        oa->Select = !oa->Select;
        if(mode==sOPSEL_ADD)
          oa->Select = sTRUE;
        if(mode==sOPSEL_CLEAR)
          oa->Select = sFALSE;
      }
    }
    OnCommand(sOIC_SETEDIT);
  }
}

void WinAnimPage::MoveDest(sBool dup)
{
  sInt i,max;
  sInt x,y,w;
  WerkOpAnim *po;
//  WerkOpAnim *opo;

  if(Op)
  {
    max = Op->AnimOps->GetCount();

    for(i=0;i<max;i++)
    {
      po = Op->AnimOps->Get(i);
      x = po->PosX;
      y = po->PosY;
      w = po->Width;
      if(po->Select)
      {
        x = sRange(x+DragMoveX,PageMaxX-w-1,0);
        y = sRange(y+DragMoveY,PageMaxY  -1,0);
        w = sRange(w+DragWidth,PageMaxX-x-1,1);
/*
        if(dup)
        {
          opo = po;
          po = new WerkOp;
          po->Copy(opo);
          po->Page = Page;
          Op->AnimOps->Add(po);
          opo->Selected = sFALSE;
        }
        */
        po->PosX = x;
        po->PosY = y;
        po->Width = w;
      }
    }
  }
}



void WinAnimPage::AddAnimOps(sChar *splinename,sInt info)
{
  sInt x,y;
  sBool ok;
  WerkOpAnim *oa;
  sInt cmd,maxmask;
  sInt offset;

  cmd = 0;
  if(info < App->ParaWin->InfoCount)
  {
    maxmask = (1<<App->ParaWin->Info[info].Channels)-1;
    if(App->ParaWin->Info[info].Format==KAF_FLOAT)
      cmd = (KA_STOREPARAFLOAT|maxmask);
    if(App->ParaWin->Info[info].Format==KAF_INT)
      cmd = (KA_STOREPARAINT|maxmask);
    if(App->ParaWin->Info[info].Format==KAF_UBYTE)
      cmd = (KA_STOREPARABYTE|maxmask);
    if(cmd && !(App->ParaWin->Info[info].Animatable))
      cmd |= 0xc0;
    offset = App->ParaWin->Info[info].Offset;
  }

  for(y=1;y<this->PageMaxY-7;y+=6)
  {
    for(x=1;x<this->PageMaxX-7;x+=6)
    {
      ok = sTRUE;
      ok &= CheckDest(x,y-1,3);
      ok &= CheckDest(x,y+0,3);
      ok &= CheckDest(x,y+1,3);
      ok &= CheckDest(x,y+2,3);
      ok &= CheckDest(x,y+3,3);
      if(ok && cmd)
      {
        CursorX = x;
        CursorY = y;
        CursorWidth = 3;

        oa = new WerkOpAnim;
        oa->PosX = CursorX;
        oa->PosY = CursorY++;
        oa->Width = CursorWidth;
        oa->Init(KA_LOADVAR,KV_TIME);
        Op->AnimOps->Add(oa);

        if(splinename)
        {
          oa = new WerkOpAnim;
          oa->PosX = CursorX;
          oa->PosY = CursorY++;
          oa->Width = CursorWidth;
          oa->Init(KA_SPLINE,0);
          sCopyString(oa->SplineName,splinename,KK_NAME);
          Op->AnimOps->Add(oa);
        }

        oa = new WerkOpAnim;
        oa->PosX = CursorX;
        oa->PosY = CursorY++;
        oa->Width = CursorWidth;
        oa->Init(cmd,offset);
        Op->AnimOps->Add(oa);

        Op->Op.CopyEditToAnim();
        Op->ConnectAnim(App->Doc);
        return;
      }
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   The Blob Spline Editor                                             ***/
/***                                                                      ***/
/****************************************************************************/

BlobSpline *WinPara::GetBlobSpline()
{
  if(!Op) return 0;
  return (BlobSpline *) Op->Op.GetBlobData();
}

void WinPara::EditBlobSpline()
{
  BlobSpline *Spline;
  sControl *con;
   
  if(!Op) return;
  Spline = GetBlobSpline();
  if(!Spline)
  {
    sInt bytes;
    Spline = MakeDummyBlobSpline(&bytes);
    Op->Op.SetBlob((sU8 *)Spline,bytes);
  }

  Spline->SetSelect(Spline->Select);
  for(sInt i=0;i<Spline->Count;i++)
  {
    sChar buffer[16];
    sInt Pos;

    sSPrintF(buffer,sCOUNTOF(buffer),"%d",i);
    Pos = 0;


    con = new sControl;
    con->Label(buffer);
    con->LayoutInfo.Init(Pos,Line,Pos+1,Line+1); Pos+=1;
    con->Style |= sCS_LEFT;
    Grid->AddChild(con);

    con = new sControl;
    con->EditCycle(CMD_SPLINE_SELECT+i,&Spline->Keys[i].Select,0,"|select");
    con->Zones = 1;
    con->Style|=sCS_ZONES;
    con->LayoutInfo.Init(Pos,Line,Pos+1,Line+1); Pos+=1;
    Grid->AddChild(con);

    con = new sControl;
    con->EditFloat(CMD_PARA_CHANGE,&Spline->Keys[i].Time,0);
    con->InitNum(-16,16,0.01f,0);
    con->Zones = 1;
    con->Style|=sCS_ZONES;
    con->LayoutInfo.Init(Pos,Line,Pos+1,Line+1); Pos+=1;
    Grid->AddChild(con);

    con = new sControl;
    con->EditFloat(CMD_PARA_CHANGE,(&Spline->Keys[i].px),0);
    con->InitNum(-256,256,0.05f,0);
    con->Zones = 3;
    con->Style|=sCS_ZONES;
    con->LayoutInfo.Init(Pos,Line,Pos+3,Line+1); Pos+=3;
    Grid->AddChild(con);

    con = new sControl;
    con->EditFloat(CMD_PARA_CHANGE,(&Spline->Keys[i].rx),0);
    if(Spline->Mode==0)
      con->InitNum(-16,16,0.01f,0);
    else
      con->InitNum(-256,256,0.05f,0);
    con->Zones = 3;
    con->Style|=sCS_ZONES;
    con->LayoutInfo.Init(Pos,Line,Pos+3,Line+1); Pos+=3;
    Grid->AddChild(con);

    con = new sControl;
    con->EditFloat(CMD_PARA_CHANGE,&Spline->Keys[i].Zoom,0);
    con->InitNum(0,64,0.01f,1);
    con->Zones = 1;
    con->Style|=sCS_ZONES;
    con->LayoutInfo.Init(Pos,Line,Pos+1,Line+1); Pos+=1;
    Grid->AddChild(con);

    if(Spline->Count>2)
      AddBox(CMD_SPLINE_REM+i,0,0,"Rem");
    if(Spline->Count<256 && i+1<Spline->Count)
      AddBox(CMD_SPLINE_ADD+i,1,0,"Add");

    Line++;
  }

  con = new sControl;
  con->EditChoice(CMD_SPLINE_CHANGEMODE,&Spline->Mode,0,"Rot/Trans|Fix Target|Animated Target|Forward");
  con->Zones = 1;
  con->Style|=sCS_ZONES;
  con->LayoutInfo.Init(3,Line,6,Line+1);
  Grid->AddChild(con);

  if(Spline->Mode==1)
  {
    con = new sControl;
    con->EditFloat(CMD_PARA_CHANGE,(&Spline->Target.x),0);
    con->InitNum(-256,256,0.05f,0);
    con->Zones = 3;
    con->Style|=sCS_ZONES;
    con->LayoutInfo.Init(6,Line,9,Line+1);
    Grid->AddChild(con);
    AddBox(CMD_SPLINE_SETTARGET,2,0,"Set");
  }
  if(Spline->Mode==2)
  {
    AddBox(CMD_SPLINE_SWAPTC,2,0,"Swap");
  }
  AddBox(CMD_SPLINE_SORT,1,0,"SORT");
  AddBox(CMD_SPLINE_NORM,9,0,"Norm");
  Line++;

  Label("Spline Timing");
  con = new sControl;
  con->EditChoice(CMD_PARA_CHANGE,&Spline->Uniform,0,"Smooth|Uniform");
  con->Zones = 1;
  con->Style|=sCS_ZONES;
  con->LayoutInfo.Init(3,Line,10,Line+1);
  Grid->AddChild(con);
  Line++;

  Label("Continuity / Tension");
  con = new sControl;
  con->EditFloat(CMD_PARA_CHANGE,&Spline->Continuity,0);
  con->InitNum(-16,16,0.05f,0);
  con->Zones = 2;
  con->Style|=sCS_ZONES;
  con->LayoutInfo.Init(3,Line,10,Line+1); 
  Grid->AddChild(con);
  Line++;
}

void WinPara::AddBlobSplineKey(sInt key)
{
  BlobSpline *oldspline;
  BlobSpline *newspline;

  sF32 time;
   
  if(!Op) return;
  oldspline = (BlobSpline *) Op->Op.GetBlobData();
  if(oldspline && oldspline->Count<256 && key>=0 && key<oldspline->Count)
  {
    sInt bytes;
    newspline = MakeBlobSpline(&bytes,oldspline->Count+1);

    newspline->CopyPara(oldspline);

    for(sInt i=0;i<=key;i++)
      newspline->Keys[i] = oldspline->Keys[i];
    for(sInt i=key;i<oldspline->Count;i++)
      newspline->Keys[i+1] = oldspline->Keys[i];
    if(key+1<oldspline->Count)
    {
      time = (oldspline->Keys[key].Time+oldspline->Keys[key+1].Time)/2;
      oldspline->CalcKey(time,newspline->Keys[key+1]);
      newspline->Keys[key+1].Time = time;
    }

    Op->Op.SetBlob((sU8 *)newspline,bytes);
    Op->Change(-1);
  }
  SetOp(Op);
}

void WinPara::RemBlobSplineKey(sInt key)
{
  BlobSpline *spline;
   
  if(!Op) return;
  spline = (BlobSpline *) Op->Op.GetBlobData();
  if(spline && spline->Count>2 && key>=0 && key<spline->Count)
  {
    spline->Count--;
    for(sInt i=key;i<spline->Count;i++)
      spline->Keys[i] =spline->Keys[i+1];
  }
  Op->Change(-1);
  SetOp(Op);
}

/****************************************************************************/

BlobPipe *WinPara::GetBlobPipe()
{
  if(!Op) return 0;
  return (BlobPipe *) Op->Op.GetBlobData();
}

void WinPara::EditBlobPipe()
{
  BlobPipe *Pipe;
  sControl *con;
   
  if(!Op) return;
  Pipe = GetBlobPipe();
  if(!Pipe)
  {
    sInt bytes;
    Pipe = MakeDummyBlobPipe(&bytes);
    Op->Op.SetBlob((sU8 *)Pipe,bytes);
  }

  con = new sControl;
  con->Label("start");
  con->LayoutInfo.Init(0,Line,3,Line+1);
  con->Style |= sCS_LEFT;
  Grid->AddChild(con);

  con = new sControl;
  con->EditFloat(CMD_PARA_CHANGE,&Pipe->StartX,0);
  con->InitNum(-256,256,0.01f,0);
  con->Zones = 3;
  con->Style|=sCS_ZONES;
  con->LayoutInfo.Init(3,Line,10,Line+1);
  Grid->AddChild(con);
  Line++;

  for(sInt i=0;i<Pipe->Count;i++)
  {
    sChar buffer[16];
    sInt Pos;
//    sControlTemplate temp;

    sSPrintF(buffer,sCOUNTOF(buffer),"%d",i);
    Pos = 0;


    con = new sControl;
    con->Label(buffer);
    con->LayoutInfo.Init(Pos,Line,Pos+3,Line+1); Pos+=3;
    con->Style |= sCS_LEFT;
    Grid->AddChild(con);

    con = new sControl;
    con->EditFloat(CMD_PARA_CHANGE,&Pipe->Keys[i].PosX,0);
    con->InitNum(-256,256,0.01f,0);
    con->Zones = 3;
    con->Style|=sCS_ZONES;
    con->LayoutInfo.Init(Pos,Line,Pos+5,Line+1); Pos+=5;
    Grid->AddChild(con);
    con = new sControl;
    con->EditFloat(CMD_PARA_CHANGE,&Pipe->Keys[i].Radius,0);
    con->InitNum(-256,256,0.01f,0);
    con->Zones = 1;
    con->Style|=sCS_ZONES;
    con->LayoutInfo.Init(Pos,Line,Pos+2,Line+1); Pos+=2;
    Grid->AddChild(con);
/*
    temp.Init();
    temp.Type = sCT_CYCLE;
    temp.Cycle = "left|right|up|down|forw|back:*3relative|absolute";
    temp.YPos = Line;
    temp.XPos = Pos-2;
    temp.AddFlags(Grid,CMD_PARA_CHANGE,&Pipe->Keys[i].Flags,0);
*/
    if(Pipe->Count>2)
      AddBox(CMD_PIPE_REM+i,0,0,"Rem");
    if(Pipe->Count<256 && i<Pipe->Count)
      AddBox(CMD_PIPE_ADD+i,1,0,"Add");

    Line++;
  }

  con = new sControl;
  con->Label("Tension");
  con->LayoutInfo.Init(0,Line,3,Line+1);
  con->Style |= sCS_LEFT;
  Grid->AddChild(con);

  con = new sControl;
  con->EditFloat(CMD_PARA_CHANGE,&Pipe->Tension,0);
  con->InitNum(-16,16,0.01f,0);
  con->Zones = 1;
  con->LayoutInfo.Init(3,Line,10,Line+1);
  Grid->AddChild(con);

}

void WinPara::AddBlobPipeKey(sInt key)
{
  BlobPipe *oldpipe;
  BlobPipe *newpipe;
   
  if(!Op) return;
  oldpipe = GetBlobPipe();
  if(oldpipe && oldpipe->Count<256 && key>=0 && key<oldpipe->Count)
  {
    sInt bytes;
    newpipe = MakeBlobPipe(&bytes,oldpipe->Count+1);
    newpipe->CopyPara(oldpipe);

    for(sInt i=0;i<=key;i++)
      newpipe->Keys[i] = oldpipe->Keys[i];
    for(sInt i=key;i<oldpipe->Count;i++)
      newpipe->Keys[i+1] = oldpipe->Keys[i];

    Op->Op.SetBlob((sU8 *)newpipe,bytes);
    Op->Change(-1);
  }
  SetOp(Op);
}

void WinPara::RemBlobPipeKey(sInt key)
{
  BlobPipe *pipe;
   
  if(!Op) return;
  pipe = GetBlobPipe();
  if(pipe && pipe->Count>2 && key>=0 && key<pipe->Count)
  {
    pipe->Count--;
    for(sInt i=key;i<pipe->Count;i++)
      pipe->Keys[i] =pipe->Keys[i+1];
  }
  Op->Change(-1);
  SetOp(Op);
}

/****************************************************************************/
/****************************************************************************/
