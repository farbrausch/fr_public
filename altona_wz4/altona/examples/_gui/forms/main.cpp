/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "main.hpp"
//#include "gui/dialog.hpp"

MainWindow *App;

/****************************************************************************/
/****************************************************************************/

ListItem::ListItem(const sChar *str,sInt val,sU32 col) 
{ 
  Select = 0;
  Select2 = 0;
  Select3 = 0;
  String=str;
  Value=val;
  Color=col; 
  ConstString=L"const"; 
  PoolString=L"pool"; 
  Choice=1; 
  Gradient=new sColorGradient; 
}

ListItem::ListItem() 
{ 
  Select = 0;
  Select2 = 0;
  Select3 = 0;
  String=L"";
  Value=0;
  Color=0xffffffff; 
  ConstString=L"const"; 
  PoolString=L"pool"; 
  Choice=1; 
  Gradient=new sColorGradient; 
}

void ListItem::Tag() 
{ 
  Gradient->Need(); 
}


const sChar *ListItem::GetName()
{
  return String;
}
/*
void ListItem::OnPaintColumn(sInt column,const sRect &client,sBool select)
{
  sString<64> buffer;
  switch(column)
  {
  case 0:
    Paint(String,client,select);
    break;
  case 1:
    sSPrintF(buffer,L"%d",Value);
    Paint(buffer,client,select);
    break;
  case 2:
    sSPrintF(buffer,L"%08x",Color);
    Paint(buffer,client,select);
    break;
  }
}
*/
void ListItem::AddGui(sGridFrameHelper &gh)
{
  gh.Group(String);
  gh.Label(L"Value");
  gh.Int(&Value,0,255,0.25f);
  gh.Label(L"Color");
  gh.ColorPick(&Color,L"rgba",this);
  gh.Label(L"Choice");
  gh.Choice(&Choice,L"no|yes|maybe");
  gh.Label(L"Gradient");
  gh.Gradient(Gradient,1);
}

/****************************************************************************/
/****************************************************************************/

MainWindow::MainWindow()
{
  for(sInt i=0;i<sCOUNTOF(ints);i++)
    ints[i] = 0;
  for(sInt i=0;i<sCOUNTOF(floats);i++)
    floats[i] = 0;
  for(sInt i=0;i<sCOUNTOF(strings);i++)
    strings[i] = L"";

  App = this;
  AddChild(sWire = new sWireMasterWindow);

  sWire->AddWindow(L"main",this);
  sWire->AddKey(L"main",L"AddListItem",sMessage(this,&MainWindow::AddListItem));
  sWire->AddKey(L"main",L"Dialog",sMessage(this,&MainWindow::CmdDialog));
  sWire->AddKey(L"main",L"Complicated",sMessage(this,&MainWindow::CmdComplicated));
  sWire->AddChoice(L"main",L"Value",sMessage(),&ints[2],L"Value A|Value B|Value C|Value D|Value E|Value F|Value G");
  sWire->AddChoice(L"main",L"Toggle",sMessage(),&ints[0],L"-|toggle");

  TextWin = new WinText;
  FrameWin = new WinFrame;
  ItemWin = new sGridFrame;

  ListWin = new sMultiListWindow<ListItem>(&Items,sMEMBERPTR(ListItem,Select));
  ListWin->InitWire(L"List");
  ListWin->AddField(L"Name",sLWF_EDIT|sLWF_SORT,150,sMEMBERPTR(ListItem,String));
  ListWin->AddField(L"Value",sLWF_EDIT|sLWF_SORT,50,sMEMBERPTR(ListItem,Value));
  ListWin->AddFieldColor(L"Color",sLWF_SORT,50,sMEMBERPTR(ListItem,Color));
  ListWin->AddFieldChoice(L"Choice",sLWF_EDIT,50,sMEMBERPTR(ListItem,Choice),L"No|Yes|Maybe");
//  ListWin->AddField(L"Const",0,50,&ListItem::ConstString);
  ListWin->AddField(L"Pool",sLWF_EDIT,50,sMEMBERPTR(ListItem,PoolString));
  ListWin->AddHeader();
  ListWin->SelectMsg = sMessage(this,&MainWindow::UpdateItemList);

  TreeWin = new sMultiTreeWindow<ListItem>(&Items,sMEMBERPTR(ListItem,Select3),sMEMBERPTR(ListItem,TreeInfo));
  TreeWin->InitWire(L"Tree");
  TreeWin->AddField(L"Name",sLWF_EDIT,150,sMEMBERPTR(ListItem,String));
  TreeWin->AddField(L"Value",sLWF_EDIT,50,sMEMBERPTR(ListItem,Value));
  TreeWin->AddFieldColor(L"Color",0,50,sMEMBERPTR(ListItem,Color));
  TreeWin->AddFieldChoice(L"Choice",sLWF_EDIT,50,sMEMBERPTR(ListItem,Choice),L"No|Yes|Maybe");
//  TreeWin->AddField(L"Const",0,50,&ListItem::ConstString);
  TreeWin->AddField(L"Pool",sLWF_EDIT,50,sMEMBERPTR(ListItem,PoolString));
  TreeWin->AddHeader();
  TreeWin->SelectMsg = sMessage(this,&MainWindow::UpdateItemTree);

  TextWin->InitWire(L"Text");
  FrameWin->InitWire(L"Frame");
  sWire->AddWindow(L"Item",ItemWin);
  sWire->ProcessFile(L"forms.wire.txt");
  sWire->ProcessEnd();

  TextWin->UpdateText(0);

  sRandom rnd;
  for(sInt i=0;i<15;i++)
  {
    ListItem *li = new ListItem();
    sInt jmax = rnd.Int(3)+3;
    for(sInt j=0;j<jmax;j++)
      li->String[j] = rnd.Int('z'-'a')+'a';
    li->String[jmax] = 0;
    li->TreeInfo.Level = rnd.Int(5);
    Items.AddTail(li);
  }
  TreeWin->UpdateTreeInfo();
//  Items.AddTail(new ListItem(L"bli",23,0xffffff));
//  Items.AddTail(new ListItem(L"bla",42,0xff00ff));
//  Items.AddTail(new ListItem(L"blub",64,0x0000ff));
//  ListWin->UpdateList();
  ListWin->Update();
  TreeWin->Update();
}

MainWindow::~MainWindow()
{
}

void MainWindow::Tag()
{
  sWindow::Tag();
  sNeed(Items);

  FrameWin->Need();
  TextWin->Need();
  ListWin->Need();
  TreeWin->Need();
  ItemWin->Need();
}

void MainWindow::UpdateText(sDInt mode)
{
  TextWin->UpdateText(mode);
}

void MainWindow::UpdateItemList()
{
  ListItem *li;

  sGridFrameHelper gh(ItemWin);
  sFORALL(Items,li)
    if(li->Select || li->Select2 || li->Select3)
      li->AddGui(gh);
}

void MainWindow::UpdateItemTree()
{
  UpdateItemList();
}

void MainWindow::AddListItem(sDInt pos)
{
  ListWin->AddNew(new ListItem(L"<< new >>",0,0));
}

void MainWindow::CmdDialog()
{
  sMultipleChoiceDialog *diag = new sMultipleChoiceDialog(L"multiple choice test",
    L"click one of these buttons to exit\n"
    L"This is supposed to be a longer text. so i add some random stuff in here.\n"
    L"It's massive. It takes my breath away. It definitely is a solid piece both artistically and technically.\n"
    L"Kasparov is what I immediately drew comparisons to. They share the idea of a machine-generated world that "
/*
    L"at first look seems like a real one, the surface cracking to show the cold dark unreal nature. While "
    L"Kasparov ultimately left it at that despite making the disconnect more and more apparent as the demo "
    L"progressed, Debris ends in the extreme, ripping the whole world apart, leaving only static behind.\n"
    L"It's a really well designed whole where each and every part works toward the desired effect. Should not be missed."
    */
  );
  diag->AddItem(L"Save & Quit",sMessage(),'s');
  diag->AddItem(L"Quit without Saving",sMessage(),'q');
  diag->AddItem(L"Do not Quit",sMessage(),sKEY_ESCAPE);
  diag->Start();
}

void MainWindow::CmdComplicated()
{
  sProgressDialog::Open(L"Complicated operation",L"Counting sheep...");
  
  sProgressDialog::PushLevelCounter(0,2);
  static sInt count = 50;
  for(sInt i=0;i<count;i++)
  {
    if(!sProgressDialog::SetProgress(1.0f * i / count))
    {
      sProgressDialog::Close();
      return;
    }

    sSleep(50);
  }
  sProgressDialog::PopLevel();

  sProgressDialog::SetText(L"Counting sheep in reverse...");
  sProgressDialog::PushLevelCounter(1,2);
  for(sInt i=0;i<count;i++)
  {
    if(!sProgressDialog::SetProgress(1.0f * i / count))
    {
      sProgressDialog::Close();
      return;
    }

    sSleep(50);
  }
  sProgressDialog::Close();
}

/****************************************************************************/

WinFrame::WinFrame()
{
  Grid = new sGridFrame;
  AddChild(Grid);
  Grid->Columns = 12;

  ClearNotify();
//  AddNotify(App->ints,sizeof(App->ints);
//  AddNotify(App->strings,sizeof(App->strings);
//  AddNotify(App->floats,sizeof(App->floats);

  sGridFrameHelper gh(Grid);
  gh.DoneMsg = sMessage(App,&MainWindow::UpdateText,1000);
  gh.ChangeMsg = sMessage(App,&MainWindow::UpdateText,1001);
  gh.Reset();
  gh.Group(L"Buttons");
  gh.Label(L"pushbuttons");
  gh.PushButton(L"push A",sMessage(App,&MainWindow::UpdateText,1));
  gh.PushButton(L"push B",sMessage(App,&MainWindow::UpdateText,2));
  gh.Box(L"->",sMessage(App,&MainWindow::UpdateText,3));
  gh.Box(L"...",sMessage(App,&MainWindow::UpdateText,4));
  gh.Label(L"radio buttons");
  gh.Radio(&App->ints[2],L"A|B|C|D|E|F|G",2);
  gh.Label(L"choices");
  gh.Choice(&App->ints[0],L"off|on");
  gh.Choice(&App->ints[1],L" 0| 1| 2| 3");
  gh.Choice(&App->ints[1],L"*4bli|bla|blub");
  gh.Label(L"Flags");
  gh.Flags(&App->ints[3],L"on|off:*1zero|one:*2A|B|C|:*4-|action");
  gh.Group(L"Text");
  gh.Label(L"String");
  gh.String(App->strings[0]);
  gh.Label(L"int");
  gh.Int(&App->ints[4],0,16,0.25f);
  gh.Int(&App->ints[5],-1024,1024,16);
  gh.Label(L"float");
  gh.Float(&App->floats[0],0,16,0.25f);
  gh.Float(&App->floats[1],-1024,1024,16);
  gh.Label(L"byte");
  gh.Byte((sU8*)&App->ints[6],0,16,0.25f);
  gh.Byte((sU8*)&App->ints[7],0,255,16);
  gh.Group(L"Colors");
  gh.Label(L"byte rgb");
  gh.Color((sU32 *)&App->ints[8],L"rgb");
  gh.Label(L"byte rgba");
  gh.Color((sU32 *)&App->ints[9],L"rgba");
  gh.Label(L"float rgb");
  gh.ColorF(&App->floats[8],L"rgb");
  gh.Label(L"float rgba");
  gh.ColorF(&App->floats[12],L"rgba");

  sWire->AddDrag(L"Frame",L"Scroll",sMessage(this,&WinFrame::DragScroll,0));
  Default = 0;
}

WinFrame::~WinFrame()
{
}

void WinFrame::InitWire(const sChar *name)
{
  sWire->AddWindow(name,this);
  sWire->AddParaChoice<WinFrame>(name,L"Default",sMessage(),&WinFrame::Default,L"zero|mixed|extreme");
  sWire->AddKey(name,L"Reset",sMessage(this,&WinFrame::Reset,0));
}

void WinFrame::Reset(sDInt)
{
  switch(Default)
  {
  case 0:
    for(sInt i=0;i<sCOUNTOF(App->ints);i++)
      App->ints[i] = 0;
    for(sInt i=0;i<sCOUNTOF(App->floats);i++)
      App->floats[i] = 0;
    for(sInt i=0;i<sCOUNTOF(App->strings);i++)
      App->strings[i] = L"";
    break;
  case 1:
    for(sInt i=0;i<sCOUNTOF(App->ints);i++)
      App->ints[i] = 13;
    for(sInt i=0;i<sCOUNTOF(App->floats);i++)
      App->floats[i] = 0.25f;
    for(sInt i=0;i<sCOUNTOF(App->strings);i++)
      App->strings[i] = L"xxx";
    break;
  case 2:
    for(sInt i=0;i<sCOUNTOF(App->ints);i++)
      App->ints[i] = 9999;
    for(sInt i=0;i<sCOUNTOF(App->floats);i++)
      App->floats[i] = 999999;
    for(sInt i=0;i<sCOUNTOF(App->strings);i++)
      App->strings[i] = L"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    break;
  }
  Update();
}

/****************************************************************************/

WinText::WinText()
{
  AddChild(TextWin = new sTextWindow);
  TextWin->SetText(&TextBuffer);
}

void WinText::InitWire(const sChar *name)
{
  sWire->AddWindow(name,this);
  sWire->AddKey(name,L"Update",sMessage(this,&WinText::UpdateText));
}

void WinText::UpdateText(sDInt mode)
{
  sTextBuffer *tb = &TextBuffer;

  tb->Clear();
  tb->PrintF(L"cmd :%08x\n",mode);
  for(sInt i=0;i<sCOUNTOF(App->ints);i+=4)
    tb->PrintF(L"int[%02d]: %08x %08x %08x %08x\n",i,App->ints[i+0],App->ints[i+1],App->ints[i+2],App->ints[i+3]);
  for(sInt i=0;i<sCOUNTOF(App->floats);i+=4)
    tb->PrintF(L"floats[%02d]: %10f %10f %10f %10f\n",i,App->floats[i+0],App->floats[i+1],App->floats[i+2],App->floats[i+3]);
  for(sInt i=0;i<sCOUNTOF(App->strings);i++)
    tb->PrintF(L"string[%d]: <%s>\n",i,App->strings[i]);
  TextWin->Update();
}

/****************************************************************************/
/****************************************************************************/

void sMain()
{
  sInit(sISF_2D,800,600);
  sInitGui();
  sGui->AddBackWindow(new MainWindow);
}

/****************************************************************************/
