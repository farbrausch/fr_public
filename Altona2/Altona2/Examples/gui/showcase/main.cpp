/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "main.hpp"

using namespace Altona2;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class ScrollWindow : public sWindow
{
  sInt DragStartX;
  sInt DragStartY;
  sBool DragMode;
public:
  ScrollWindow()
  {
    DragStartX = DragStartY = 0;
    DragMode = 0;
    AddHelp();
  }

  void OnCalcSize()
  {
    ReqSizeX = 512;
    ReqSizeY = 512;
  };

  void OnPaint(sInt layer)
  {
    Painter()->SetLayer(layer);
    for(sInt y=0;y<8;y++)
    {
      for(sInt x=0;x<8;x++)
      {
        sRect r;
        r.x0 = Client.x0 + (x+0)*ReqSizeX/8;
        r.y0 = Client.y0 + (y+0)*ReqSizeY/8;
        r.x1 = Client.x0 + (x+1)*ReqSizeX/8;
        r.y1 = Client.y0 + (y+1)*ReqSizeY/8;
        Painter()->Rect(((x+y)&1)?0xffff0000:0xff00ff00,r);
      }
    }
  }
};

class ListElement
{
public:
  sListWindowElementInfo Info;
  sInt n;
  sF32 f;
  int c;
  sString<64> s;

  ListElement(sInt _n,sF32 _f,const sChar *_s)
  {
    n = _n;
    f = _f;
    s = _s;
    c = 0;
  }
};

class MainWindow : public sSwitchFrame
{
  sInt ref0;
  sInt ref1;
  sInt Switch;
  sInt int0,int1;
  sF32 float0,float1;
  sString<64> FpsString;
  sString<64> EditString1;
  sString<8> EditString2;
  sUpdatePool RadioPool;
  sUpdatePool ParaPool;
  sUpdateReceiver ParaRec;
  sTextBuffer EditBuffer;
  sTextBuffer ParaBuffer;
  sString<64> DialogString[2];

  sArray<ListElement *> ListArray;
  sArray<ListElement *> TreeArray;
public:
  ~MainWindow()
  {
    ListArray.DeleteAll();
    TreeArray.DeleteAll();
  }
  MainWindow()
  { 
    ref0 = 0;
    ref1 = 0;
    Switch = 0;
    int0 = int1 = 0;
    float0 = float1 = 0;
    EditString1 = "test";
    EditString2 = "01234567";
    EditBuffer.Set("bla\nblub\n");
    ListArray.Add(new ListElement(1,1.1f,"bla"));
    ListArray.Add(new ListElement(2,2.2f,"blub"));
    ListArray.Add(new ListElement(3,3.3f,"blatter"));
    ListArray.Add(new ListElement(4,4.0f,"more"));

    sRandomKISS rnd;
    sInt indent = 0;
    for(sInt i=0;i<100;i++)
    {
      sString<64> n;
      n.PrintF("[%08x]",rnd.Int32());
      ListElement *le = new ListElement(i,rnd.Float(100),n);
      le->Info.Indent = indent;
      TreeArray.Add(le);
      if(rnd.Int(10)==0)
        indent++;
      if(rnd.Int(8)==0)
        indent = sMax(0,indent-1);
    }

    sGuiMsg rpmsg(&RadioPool,&sUpdatePool::Update);
    sGuiMsg ppmsg(&ParaPool,&sUpdatePool::Update);
    ParaRec.Call = sDelegate<void>(this,&MainWindow::UpdatePara);
    RadioPool.Add(&ParaRec);
    ParaPool.Add(&ParaRec);

    sWindow *w;
    sButtonControl *b;
    sGridFrame *g;
    sHSplitterFrame *h;
    sVSplitterFrame *v;

    AddKey("screen 1",sKEY_F1,sGuiMsg(this,&MainWindow::CmdSwitch,0));
    AddKey("screen 2",sKEY_F2,sGuiMsg(this,&MainWindow::CmdSwitch,1));
    AddKey("screen 3",sKEY_F3,sGuiMsg(this,&MainWindow::CmdSwitch,2));
    AddKey("Exit",sKEY_Escape|sKEYQ_Shift,sGuiMsg(this,&sWindow::CmdExit));
//    AddHelp();

    // tool border

    sToolBorder *tb = new sToolBorder; AddBorder(tb);
    tb->AddChild(new sButtonControl(sGuiMsg(this,&MainWindow::CmdMenu),"menu",sBS_Menu));
    tb->AddChild(new sButtonControl(sGuiMsg(),"  ",sBS_Label));

    b = new sButtonControl(sGuiMsg(this,&MainWindow::CmdSwitch,0),"splitters",sBS_Radio);
    b->SetRef(&Switch,0);
    tb->AddChild(b);
    b = new sButtonControl(sGuiMsg(this,&MainWindow::CmdSwitch,1),"grid",sBS_Radio);
    b->SetRef(&Switch,1);
    tb->AddChild(b);
    b = new sButtonControl(sGuiMsg(this,&MainWindow::CmdSwitch,2),"scroll",sBS_Radio);
    b->SetRef(&Switch,2);
    tb->AddChild(b);

    b = new sButtonControl(sGuiMsg(),FpsString,sBS_Label);
    b->FixedSizeX = 200;
    tb->AddChild(b);
 
    // page 1: grid and buttons

    // grid

    g = new sGridFrame(); g->AddBorder(new sFocusBorder); g->AddScrolling(0,1);

    sGridFrameHelper gh(g);
    gh.ChangeMsg = ppmsg;
    gh.Label("Buttons");
    gh.UpdatePool = 0;
    gh.Button(sGuiMsg(),"A");
    gh.Button(sGuiMsg(),"B");
    gh.Button(sGuiMsg(),"C");
    gh.Button(sGuiMsg(),"D");

    gh.Label("Toggle");
    gh.Toggle(sGuiMsg(),ref0,1,"Toggle");

    gh.Label("Radio");
    gh.UpdatePool = &RadioPool;
    gh.Radio(rpmsg,ref1,0,"0");
    gh.Radio(rpmsg,ref1,1,"1");
    gh.Radio(rpmsg,ref1,2,"2");
    gh.Radio(rpmsg,ref1,3,"3");
    gh.UpdatePool = 0;

    gh.Label("Dialogs");
    gh.Button(sGuiMsg(this,&MainWindow::CmdAddDialog,0),"OK");
    gh.Button(sGuiMsg(this,&MainWindow::CmdAddDialog,1),"Yes/No");
    gh.Button(sGuiMsg(this,&MainWindow::CmdAddDialog,2),"Save");
    gh.Button(sGuiMsg(this,&MainWindow::CmdAddDialog,3),"String");
    gh.Button(sGuiMsg(this,&MainWindow::CmdAddDialog,4),"Rename");

    gh.Label("Choice");
    gh.UpdatePool = &RadioPool;
    gh.Choice(ref1," 0|| 2| 3|1 1");
    gh.Choice(ref1,"-| 1| 2| 3");
    gh.UpdatePool = 0;

    gh.Label("String<64>");
    gh.String(EditString1);
    gh.Label("String<8>");
    gh.String(EditString2);
    gh.Label("int");
    gh.Int(int0,0,32,0.25f);
    gh.Int(int1,-0x7fffffff,0x7fffffff,0.25f);
    gh.Label("float");
    gh.Float(float0,0,32,0.25f);
    gh.Float(float1,(sF32)-0x7fffffff,(sF32)0x7fffffff,0.25f);

    g->AddLabel(0,20,3,1,"scrolling...");

    // list

    sListWindow<ListElement> *lw0 = new sListWindow<ListElement>(ListArray,&ListElement::Info,sLWF_MultiSelect|sLWF_Move|sLWF_AllowEdit);
    lw0->AddField("int",50,sLFF_Edit|sLFF_Sort,&ListElement::n,-100,100);
    lw0->AddField("float",50,sLFF_Edit|sLFF_Sort,&ListElement::f,-100,100);
    lw0->AddFieldChoice("choice",50,sLFF_Edit|sLFF_Sort,&ListElement::c,"a|b|c|d|e|f|g|h");
    lw0->AddField("string",150,sLFF_Edit|sLFF_Sort,&ListElement::s);


    sListWindow<ListElement> *lw1 = new sListWindow<ListElement>(TreeArray,&ListElement::Info,sLWF_Tree);
    lw1->AddTree("tree",100,0,&ListElement::s);
    lw1->AddField("int",50,sLFF_Edit|sLFF_Sort,&ListElement::n,-100,100);
    lw1->AddField("float",50,sLFF_Edit|sLFF_Sort,&ListElement::f,-100,100);
    lw1->AddField("string",150,sLFF_Edit|sLFF_Sort,&ListElement::s);


    // put it together

    h = new sHSplitterFrame; AddChild(h);
    w = new sTextWindow(&EditBuffer); h->AddChild(w); w->AddBorder(new sFocusBorder); w->AddScrolling(1,1);
    v = new sVSplitterFrame; h->AddChild(v);
    w = lw0; v->AddChild(w); w->AddScrolling(1,1); lw0->AddHeader(); lw0->AddSearch(); w->AddBorder(new sFocusBorder);
    v->AddChild(g);
    w = lw1; v->AddChild(w); w->AddScrolling(1,1); lw1->AddHeader(); w->AddBorder(new sFocusBorder);
    w = new sTextWindow(&ParaBuffer); h->AddChild(w); w->AddBorder(new sFocusBorder); w->AddScrolling(1,1);
    v->SetSize(1,800);

   
    // page 2: splitters

    h = new sHSplitterFrame; AddChild(h);

    w = new sDummyWindow(); h->AddChild(w); w->AddBorder(new sFocusBorder);
    w = new sDummyWindow(); h->AddChild(w); w->AddBorder(new sFocusBorder);
    v = new sVSplitterFrame; h->AddChild(v);

    w = new sDummyWindow(); v->AddChild(w); w->AddBorder(new sFocusBorder); 
    w = new sDummyWindow(); v->AddChild(w); w->AddBorder(new sFocusBorder);
    w = new sDummyWindow(); v->AddChild(w); w->AddBorder(new sFocusBorder);
    v->SetSize(0,100);
    v->SetSize(2,100);

    // page 3: scrolling

    h = new sHSplitterFrame; AddChild(h);
    w = new sDummyWindow(); h->AddChild(w); w->AddBorder(new sFocusBorder);
    v = new sVSplitterFrame; h->AddChild(v);
    w = new sDummyWindow(); v->AddChild(w); w->AddBorder(new sFocusBorder);
    w = new ScrollWindow; v->AddChild(w); w->AddBorder(new sFocusBorder); w->AddScrolling(1,1);
    w = new sDummyWindow(); v->AddChild(w); w->AddBorder(new sFocusBorder);
    w = new sDummyWindow(); h->AddChild(w); w->AddBorder(new sFocusBorder);

    // done

    SetSwitch(0);
    UpdatePara();
  }

  void UpdatePara()
  {
    ParaBuffer.Clear();
    ParaBuffer.PrintF("ref0/1: %d %d\n",ref0,ref1);
    ParaBuffer.PrintF("int0/1: %d %d\n",int0,int1);
    ParaBuffer.PrintF("float0/1: %f %f\n",float0,float1);
    ParaBuffer.PrintF("string<64>: %q\n",this->EditString1);
    ParaBuffer.PrintF("string<8>: %q\n",this->EditString2);
    Update();
  }

  void CmdSwitch(sInt n)
  {
    sDPrintF("switch to %d\n",n);
    Switch = n;
    SetSwitch(n);
  }

  void CmdAddDialog(sInt n)
  {
    /*
    sWindow *w = new sDummyWindow;
    sTitleBorder *tb = new sTitleBorder("some window");
    w->AddBorder(tb);
    w->AddBorder(new sSizeBorder);
    tb->AddChild(new sButtonControl(sGuiMsg(w,&sWindow::CmdKill),"x"));
    sGui->AddDialog(w,sRect(100,100,300,200));
    */

    sDialogPara dp;
    dp.Init("Dialog","sdfh sjdfg asfhjasgf hasdf asdkf skjdgf kasdjgfk asdf jasdgfwifgruioqgh rjvnfjdvdsvbhqe viwen jvbch bhfv rivwjcv");
    switch(n)
    {
    default:
    case 0:
      dp.InitOk(sGuiMsg());
      break;
    case 1:
      dp.InitYesNo(sGuiMsg(),sGuiMsg());
      break;
    case 2:
      dp.InitSaveDiscardCancel(sGuiMsg(),sGuiMsg(),sGuiMsg());
      break;
    case 3:
      dp.InitOk(sGuiMsg());
      dp.AddString("Name",DialogString[0]);
      break;
    case 4:
      dp.InitOk(sGuiMsg());
      dp.AddString("Old",DialogString[0]);
      dp.AddString("New",DialogString[1]);
      break;
    }
    dp.Start(this);
  }

  void CmdMenu()
  {
    sMenuFrame *m = new sMenuFrame(this);
    m->AddHeader(0,"Column A");
    m->AddItem(sGuiMsg(),'a',"bli");
    m->AddItem(sGuiMsg(),'b',"bla");
    m->AddItem(sGuiMsg(),'c',"blub");
    m->AddHeader(1,"Column B");
    m->UpdatePool = &RadioPool;
    m->AddRadio(&ref1,0,~0,sGuiMsg(),'0',"check 0");
    m->AddRadio(&ref1,1,~0,sGuiMsg(),'1',"check 1");
    m->AddRadio(&ref1,2,~0,sGuiMsg(),'2',"check 2");
    m->AddRadio(&ref1,3,~0,sGuiMsg(),'3',"check 3");
    m->UpdatePool = 0;
    m->AddSpacer();
    m->AddToggle(&ref0,1,~0,sGuiMsg(),0,"toggle");
    m->AddItem(sGuiMsg(),0,"more");
    m->AddItem(sGuiMsg(),0,"options");
    m->StartMenu();
  }

  void OnPaint(sInt layer)
  {
    sSwitchFrame::OnPaint(layer);
    sGfxStats st;
    sGetGfxStats(st);
    FpsString.PrintF("%5.2f ms, %5.2f fps",st.FilteredFrameDuration*1000.0f,1.0f/st.FilteredFrameDuration);
  }
};


/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class AppInit : public sGuiInitializer
{
public:
    sWindow *CreateRoot() { return new MainWindow; }
    void GetTitle(const sStringDesc &desc) { sSPrintF(desc,"altona2 gui showcase"); }
};

void Altona2::Main()
{
    sRunGui(new AppInit);
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/


/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/

