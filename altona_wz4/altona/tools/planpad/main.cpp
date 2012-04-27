/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "main.hpp"
#include "util/scanner.hpp"
#include "base/windows.hpp"
#include "wiki/resources.hpp"
#include "wiki/pdf.hpp"

#define VERSION 1
#define REVISION 34

MainWindow *App;
extern "C" sChar8 WireTXT[];

/****************************************************************************/

static void FakeScript(const sChar *code,sTextBuffer *output)
{
  App->FakeScript(code,output);
}

void FocusHook(sBool enter,void *user)
{
  if(App->DocChanged)
    App->CmdSave();
}

class sMyStringControl : public sStringControl
{
public:
  sMyStringControl(const sStringDesc &desc) : sStringControl(desc) {}
  void OnCalcSize()
  {
    sStringControl::OnCalcSize();
    ReqSizeX = 200;
  }
};



Tab::Tab(const sChar *p) 
{
  sString<512> str;
  Label = p;
  Dir = L"";
  str.PrintF(L"%s.wiki.txt",p);
  Filename=str; 
}

/****************************************************************************/
/****************************************************************************/

MainWindow::MainWindow()
{
  // prevent unitialized variables

  ClickViewHasHit = 0;
  Parser = 0;
  ViewWin = 0;
  TextWin = 0;
  TextEditFont = 0;
  DocChanged = 0;
  FindDir = 0;
  FontSize = 1;
  EditFontSize = 1;
  DebugUndo = 0;
  DebugTimer = 0;
  RightBorder = 0;
  SaveAsUTF8 = 0;
  sGetCurrentDir(OriginalPath);

  // create windows

  App = this;
  AddChild(sWire = new sWireMasterWindow);

  TextWin = new sWireTextWindow;
  TextWin->SetText(&Text);
  TextWin->AddBorder(new sFocusBorder);
  TextWin->AddScrolling(0,1);
  TextWin->SetFont(sGui->FixedFont);
  TextWin->ChangeMsg = sMessage(this,&MainWindow::CmdDocChange);
  TextWin->BackColor = sGC_WHITE;
  TextWin->ShowCursorAlways = 1;
  TextWin->CursorMsg = sMessage(this,&MainWindow::CmdCursorChange);
  TextWin->CursorFlashMsg = sMessage(this,&MainWindow::CmdCursorFlash);
  TextWin->TabSize = 2;
  ViewWin = new sLayoutWindow;
  ViewWin->AddBorder(new sFocusBorder);
  ViewWin->AddScrolling(1,1);
  ViewWin->ActivateOnKeyWindow = TextWin;
  FindStringWin = new sMyStringControl(FindString);
  FindStringWin->DecoratedSizeX = 200;
  FindStringWin->ChangeMsg = sMessage(this,&MainWindow::CmdFindIncremental);
  FindStringWin->DoneMsg = sMessage(this,&MainWindow::CmdFindDone);
  FindDirWin = new sChoiceControl(L"\x25bc|\x25b2",&FindDir);
  FindDirWin->Style = sChoiceControl::sCBS_CYCLE;
  BackWin = new sButtonControl(L"\x25c4",sMessage(this,&MainWindow::CmdBack),0);
  TabWin = new sTabBorder<Tab *>(Tabs);
  TabWin->ChangeMsg = sMessage(this,&MainWindow::CmdTab);

  // initialize stuff

  Parser = new Markup;
  Parser->Script = ::FakeScript;
  Tabs.AddTail(CurrentTab = new Tab(L"index"));
  CmdClear();
  sActivateHook->Add(FocusHook);

  // wiring

  sWire->AddWindow(L"main",this);
  sWire->AddWindow(L"Text",TextWin);
  sWire->AddWindow(L"FindString",FindStringWin);
  sWire->AddWindow(L"FindDir",FindDirWin);
  sWire->AddWindow(L"Back",BackWin);
  ViewWin->InitWire(L"View");
  sWire->AddDrag(L"View",L"Click",sMessage(this,&MainWindow::DragClickView));
  sWire->AddWindow(L"Tabs",TabWin);


  sWire->AddKey(L"main",L"Open",sMessage(this,&MainWindow::CmdOpen));
  sWire->AddKey(L"main",L"Save",sMessage(this,&MainWindow::CmdSave));
  sWire->AddKey(L"main",L"SaveAs",sMessage(this,&MainWindow::CmdSaveAs));
  sWire->AddKey(L"main",L"Clear",sMessage(this,&MainWindow::CmdClear));
  sWire->AddKey(L"main",L"RemWriteProtect",sMessage(this,&MainWindow::CmdRemWriteProtect));
  sWire->AddKey(L"main",L"OpenInclude",sMessage(this,&MainWindow::CmdOpenInclude));
  sWire->AddKey(L"main",L"OpenDict",sMessage(this,&MainWindow::CmdOpenDict));
  sWire->AddKey(L"main",L"NewTab",sMessage(this,&MainWindow::CmdNewTab));

  sWire->AddKey(L"main",L"Toggle",sMessage(this,&MainWindow::CmdToggle));
  sWire->AddKey(L"main",L"ToggleBoth",sMessage(this,&MainWindow::CmdToggleBoth));
  sWire->AddKey(L"main",L"Undo",sMessage(this,&MainWindow::CmdUndo));
  sWire->AddKey(L"main",L"Switch0",sMessage(this,&MainWindow::CmdSwitch,0));
  sWire->AddKey(L"main",L"Switch1",sMessage(this,&MainWindow::CmdSwitch,1));
  sWire->AddKey(L"main",L"Switch2",sMessage(this,&MainWindow::CmdSwitch,2));
  sWire->AddKey(L"main",L"Switch3",sMessage(this,&MainWindow::CmdSwitch,3));
  sWire->AddKey(L"main",L"Switch4",sMessage(this,&MainWindow::CmdSwitch,4));

  sWire->AddKey(L"main",L"Exit",sMessage(this,&MainWindow::CmdExit));
  sWire->AddKey(L"main",L"ExitDiscard",sMessage(this,&MainWindow::CmdExitDiscard));

  sWire->AddKey(L"main",L"Index",sMessage(this,&MainWindow::CmdIndex));
  sWire->AddKey(L"main",L"StartIndex",sMessage(this,&MainWindow::CmdStartIndex));
  sWire->AddKey(L"main",L"LoadDict",sMessage(this,&MainWindow::CmdLoadDict));
  sWire->AddKey(L"main",L"HTML",sMessage(this,&MainWindow::CmdHtml));
  sWire->AddKey(L"main",L"PDF",sMessage(this,&MainWindow::CmdPdf));
  sWire->AddChoice(L"main",L"DebugUndo",sMessage(this,&MainWindow::Update),&DebugUndo,L"-|Debug Undo");
  sWire->AddChoice(L"main",L"DebugTimer",sMessage(this,&MainWindow::Update),&DebugTimer,L"-|Debug Timer");
  sWire->AddChoice(L"main",L"FontSize",sMessage(this,&MainWindow::CmdFontSize),&FontSize,L"Small Fonts|Medium Fonts|Large Fonts");
  sWire->AddChoice(L"main",L"EditFontSize",sMessage(this,&MainWindow::CmdEditFontSize),&EditFontSize,L"Small Edit|Medium Edit|Large Edit");
  sWire->AddChoice(L"main",L"RightBorder",sMessage(this,&MainWindow::CmdEditFontSize),&RightBorder,L"-|Right Border");
  sWire->AddChoice(L"main",L"PageMode",sMessage(this,&MainWindow::CmdPageMode),&ViewWin->PageMode,L"-|Page Mode");
  sWire->AddChoice(L"main",L"ChapterFormfeeds",sMessage(this,&MainWindow::CmdChapterFormfeed),&Parser->ChapterFormfeed,L"-|Chapter Formfeed");
  sWire->AddChoice(L"main",L"SaveAsUTF8",sMessage(),&SaveAsUTF8,L"-|Save as UTF8");

  sWire->AddKey(L"main",L"Find",sMessage(this,&MainWindow::CmdFind,0));
  sWire->AddKey(L"main",L"FindNext",sMessage(this,&MainWindow::CmdFind,1));
  sWire->AddKey(L"main",L"FindPrev",sMessage(this,&MainWindow::CmdFind,2));

  sWire->AddKey(L"main",L"MarkI",sMessage(this,&MainWindow::CmdMark,0));
  sWire->AddKey(L"main",L"MarkB",sMessage(this,&MainWindow::CmdMark,1));
  sWire->AddKey(L"main",L"MarkU",sMessage(this,&MainWindow::CmdMark,2));
  sWire->AddKey(L"main",L"MarkS",sMessage(this,&MainWindow::CmdMark,3));
  sWire->AddKey(L"main",L"MarkR",sMessage(this,&MainWindow::CmdMarkRed));
  sWire->AddKey(L"main",L"RemoveMark",sMessage(this,&MainWindow::CmdRemoveMark));

  sWire->ProcessText(WireTXT,L"planpad.wire.txt");
  sWire->ProcessEnd();
  sGui->HandleShiftEscape = sRELEASE ? 0 : 1;

  sWire->SwitchScreen(2);

  // command line

  sGetCurrentDir(ConfigFilename);
  ConfigFilename.AddPath(L"planpad.config.txt");
  LoadConfig();
  FontSize = sGetShellInt(L"f",L"-font",FontSize);
  EditFontSize = sGetShellInt(L"f",L"-editfont",EditFontSize);
  const sChar *filename = sGetShellParameter(0,0);
  if(filename == 0) filename = L"index.wiki.txt";

  // initial load

  CmdFontSize();
  CmdEditFontSize();
  Load(OriginalPath,filename,0);
  
}

void MainWindow::Finalize()
{
  SaveConfig();
  sActivateHook->Rem(FocusHook);
  Unprotect();
}

MainWindow::~MainWindow()
{
  sRelease(TextEditFont);
  ClearFileCache();
  delete Parser;
}

void MainWindow::Tag()
{
  sWireClientWindow::Tag();
  TextWin->Need();
  ViewWin->Need();
  sNeed(Tabs);
}

void MainWindow::OnPaint2D()
{
  sWireClientWindow::OnPaint2D();
  
  if(DebugTimer)
  {
    static sTiming timer;
    timer.OnFrame(sGetTimeUS());
    FindString.PrintF(L"%d us",timer.GetDelta());
    FindStringWin->Update();
    sGui->Update(Client);

    if(sGetKeyQualifier()&sKEYQ_SHIFT)
      UpdateDoc();
  }
}

Tab *MainWindow::GetTab()
{
  return CurrentTab;
}

void MainWindow::Unprotect()
{
  if(!ProtectedFile.IsEmpty())
  {
    sSetFileWriteProtect(ProtectedFile,0);
    ProtectedFile = L"";
  }
}

void MainWindow::Protect(const sChar *file)
{
  Unprotect();
  sBool prot,ok;
  ok = sGetFileWriteProtect(file,prot);
  if(ok && !prot)
  {
    ProtectedFile = file;
    sSetFileWriteProtect(file,1);
  }
}

const sChar *MainWindow::GetCurrentFile()
{
  return GetTab()->Path;
}

void MainWindow::CmdDocChange()
{
  if(!DocChanged)
  {
    DocChanged=1;
    UpdateWindowTitle();
  }
  UpdateDoc();
}

void MainWindow::ClearFileCache()
{
  sDeleteAll(FileCache);
}

sTextBuffer *MainWindow::LoadFileCache(sPoolString filename)
{
  FileCacheItem *fc;

  fc = sFind(FileCache,&FileCacheItem::Name,filename);
  if(!fc)
  {
    const sChar *text = sLoadText(filename);
    if(!text) 
      return 0;
    fc = new FileCacheItem;
    fc->Name = filename;
    fc->Text = new sTextBuffer(text);
    FileCache.AddTail(fc);
    delete[] text;
  }
  return fc->Text;
}

void MainWindow::UpdateDoc(const sChar *gotoheadline)
{
  MarkupFile mf;
  mf.Text = &Text;

  Parser->Clear();
  Parser->PageMode = ViewWin->PageMode;
  Parser->FindHeadline = gotoheadline;
  Parser->Parse(&mf);
  ViewWin->Clear();
  Parser->Generate(ViewWin->Root);
  if(!Parser->Title.IsEmpty())
    ViewWin->Root->Page.Title = Parser->Title;
  ViewWin->Landscape = Parser->Landscape;

  ViewWin->Layout();
  ViewWin->Update();
  TextWin->Update();
  sGui->Layout();

  if(Parser->FindHeadlineCursor)
    TextWin->SetCursor(Parser->FindHeadlineCursor);

  if(DebugUndo)
    UpdateWindowTitle();
}

sBool MainWindow::OnCommand(sInt cmd)
{
  if(cmd==sCMD_SHUTDOWN)
  {
    if(GetTab()->Filename.IsEmpty() && DocChanged)
      CmdSave();
    return 1;
  }
  return 0;
}


void MainWindow::DragClickView(const sWindowDrag &dd)
{
  sLayoutBox *box;
  ViewWin->MMBScroll(dd);
  
  switch(dd.Mode)
  {
  case sDD_START:
    ClickViewHasHit = 0;
  case sDD_DRAG:
    box = ViewWin->FindBox(dd.MouseX,dd.MouseY,ViewWin->Root);
    if(box)
    {      
      if(dd.Buttons==1)
      {
        const sChar *text = box->Click(dd.MouseX,dd.MouseY);
        const sChar *s = TextWin->GetText()->Get();
        const sChar *e = s + TextWin->GetText()->GetCount();
        if(text>=s && text<=e)
        {
          ClickViewHasHit = 1;
          TextWin->SetCursor(text-s);
        }
      }
      if(dd.Mode==sDD_START)
      {
        switch(dd.Buttons)
        {
        case 1:  if(!box->LeftAction.IsEmpty())   FakeScript(box->LeftAction  ,0); break;
        case 2:  if(!box->RightAction.IsEmpty())  FakeScript(box->RightAction ,0); break;
        case 4:  if(!box->MiddleAction.IsEmpty()) FakeScript(box->MiddleAction,0); break;
        }
      }
    }
    break;
//  case sDD_STOP:
//    if(ClickViewHasHit)
//      sGui->SetFocus(TextWin);
//    break;
  }
}


void MainWindow::LoadConfig()
{
  sScanner Scan;
  Scan.Init();
  Scan.DefaultTokens();
  Scan.StartFile(ConfigFilename);

  sInt sx = Client.SizeX();
  sInt sy = Client.SizeY();
  sInt maxi = (sGetWindowMode() == sWM_MAXIMIZED);

  while(!Scan.Errors && Scan.Token!=sTOK_END)
  {
    if(Scan.IfName(L"FontSize"))
    {
      Scan.Match('=');
      FontSize = sClamp(Scan.ScanInt(),0,2);
      Scan.Match(';');
    }
    else if(Scan.IfName(L"EditFontSize"))
    {
      Scan.Match('=');
      EditFontSize = sClamp(Scan.ScanInt(),0,2);
      Scan.Match(';');
    }
    else if(Scan.IfName(L"Screen"))
    {
      Scan.Match('=');
      sInt s = Scan.ScanInt();
      if(s==0) s = 1;
      sWire->SwitchScreen(s);
      Scan.Match(';');
    }
    else if(Scan.IfName(L"SplitterPos"))
    {
      Scan.Match('=');
      sInt n = Scan.ScanInt();

      Scan.Match(';');

      sInt s = sWire->GetScreen();

      if(s==2 || s==3)
      {
        sSplitFrame *sw = (sSplitFrame *) sWire->Childs[0]->Childs[0]->Childs[0];
        sVERIFY(sw && (sCmpString(sw->GetClassName(),L"sVSplitFrame")==0 || sCmpString(sw->GetClassName(),L"sHSplitFrame")==0));
        sw->PresetPos(1,n);
      }
    }
    else if(Scan.IfName(L"SizeX"))
    {
      Scan.Match('=');
      sx = Scan.ScanInt();
      Scan.Match(';');
    }
    else if(Scan.IfName(L"SizeY"))
    {
      Scan.Match('=');
      sy = Scan.ScanInt();
      Scan.Match(';');
    }
    else if(Scan.IfName(L"Maximized"))
    {
      Scan.Match('=');
      maxi = Scan.ScanInt();
      Scan.Match(';');
    }
    else if(Scan.IfName(L"RightBorder"))
    {
      Scan.Match('=');
      RightBorder = Scan.ScanInt() ? 1:0;
      Scan.Match(';');
    }
    else if(Scan.IfName(L"ChapterFormfeed"))
    {
      Scan.Match('=');
      Parser->ChapterFormfeed = Scan.ScanInt() ? 1:0;
      Scan.Match(';');
    }
    else if(Scan.IfName(L"SaveAsUTF8"))
    {
      Scan.Match('=');
      SaveAsUTF8 = Scan.ScanInt() ? 1:0;
      Scan.Match(';');
    }
    else
    {
      Scan.Error(L"unknown command in config file");
    }
  }

  if(maxi==0)
  {
    sSetWindowMode(sWM_NORMAL);
    if(sx!=Client.SizeX() || sy!=Client.SizeY())
      sSetWindowSize(sx,sy);
  }
  else
  {
    sSetWindowMode(sWM_MAXIMIZED);
  }
}

void MainWindow::SaveConfig()
{
  sInt s = sWire->GetScreen();

  sTextBuffer tb;
  tb.PrintF(L"FontSize = %d;\n",FontSize);
  tb.PrintF(L"EditFontSize = %d;\n",EditFontSize);
  tb.PrintF(L"Screen = %d;\n",s);
  if(s==2 || s==3)
  {
    sSplitFrame *sw = (sSplitFrame *) sWire->Childs[0]->Childs[0]->Childs[0];
    sVERIFY(sw && (sCmpString(sw->GetClassName(),L"sVSplitFrame")==0 || sCmpString(sw->GetClassName(),L"sHSplitFrame")==0));
    tb.PrintF(L"SplitterPos = %d;\n",sw->GetPos(1));
  }

  tb.PrintF(L"SizeX = %d;\n",Client.SizeX());
  tb.PrintF(L"SizeY = %d;\n",Client.SizeY());
  tb.PrintF(L"Maximized = %d;\n",sGetWindowMode() == sWM_MAXIMIZED);
  tb.PrintF(L"RightBorder = %d;\n",RightBorder ? 80 : 0);
  tb.PrintF(L"ChapterFormfeed = %d;\n",Parser->ChapterFormfeed);
  tb.PrintF(L"SaveAsUTF8 = %d;\n",SaveAsUTF8 ? 80 : 0);
  sSaveTextAnsi(ConfigFilename,tb.Get());
}

/****************************************************************************/

sTextBuffer *MainWindow::FakeScript_Inline(sScanner &scan,sTextBuffer *output,sPoolString *chapter)
{
  sPoolString filename;
  scan.Match('(');
  scan.ScanString(filename);
  if(chapter)
  {
    scan.Match(',');
    scan.ScanString(*chapter);
  }
  scan.Match(')');
  scan.Match(';');
  sTextBuffer *tb = 0;
  if(!scan.Errors && output)
  {
    sString<sMAXPATH> path;
    path = GetTab()->Dir;
    path.AddPath(filename);
    tb = LoadFileCache((const sChar *)path);
    if(!tb)
    {
      scan.Error(L"could not load file %q",filename);
    }
  }
  return tb;
}

void MainWindow::FakeScript_FindChapter(sTextBuffer *tb,const sChar *&start,const sChar *&end,sBool section,sPoolString chapter)
{
  sChar *c = tb->Get();
  start = end = 0;
  sInt len = sGetStringLen(chapter);
  while(*c!=0 && end==0)
  {
    if(*c=='=')
    {
      sInt n = 0;
      start = c;
      while(*c=='=') { c++; n++; }
      while(*c==' ') c++;
      const sChar *namestart = c;
      while(*c!='\n' && *c!=0)
        c++;
      if(section)
        start = c;
      const sChar *nameend = c;
      while(*nameend==' ' && nameend>namestart) nameend--;

      if(n>0 && n<=4 && nameend-namestart==len && sCmpStringILen(chapter,namestart,len)==0)
      {

        while(*c!=0)
        {
          end = c;
          sInt m = 0;
          while(*c=='=') { c++; m++; }
          if(m>0 && m<=n)
          {
            break;
          }
          while(*c!='\n' && *c!=0)
            c++;
          while(*c=='\n')
            c++;
        }
      }
    }

    while(*c!='\n' && *c!=0)
      c++;
    while(*c=='\n')
      c++;
  }
}

void MainWindow::FakeScript(const sChar *code,sTextBuffer *output)
{
  sChar *terminated = 0;
  sScanner scan;
  scan.Init();
  scan.DefaultTokens();
  scan.Start(code);

  sInt TabStack = 0;

  while(!scan.Errors && scan.Token!=sTOK_END)
  {
    if(scan.IfName(L"load"))
    {
      sPoolString filename;
      scan.Match('(');
      scan.ScanString(filename);
      scan.Match(')');
      scan.Match(';');
      if(!scan.Errors)
      {
        const sChar *headline = 0;
        sPoolString wikidir = GetTab()->Dir;

        sInt interwikipos = sFindFirstChar(filename,'!');
        if(interwikipos>=0)
        {
          wikidir = sPoolString(filename,interwikipos);
          filename = sPoolString(((const sChar *)filename)+interwikipos+1);
        }

        sInt headlinepos = sFindFirstChar(filename,'#');
        if(headlinepos>=0)
        {
          headline = ((const sChar *)filename)+headlinepos+1;
          filename = sPoolString(filename,headlinepos);
        }
        if(sFindFirstChar(filename,'.')==-1)
        {
          sString<sMAXPATH> buffer;
          buffer.PrintF(L"%s.wiki.txt",filename);
          Load(wikidir,buffer,headline);
        }
        else
        {
          Load(wikidir,filename,headline);
        }
      }
    }
    else if(scan.IfName(L"open"))
    {
      sPoolString filename;
      scan.Match('(');
      scan.ScanString(filename);
      scan.Match(')');
      scan.Match(';');
      if(!scan.Errors)
        sExecuteOpen(filename);
    }
    else if(scan.IfName(L"shell"))
    {
      sPoolString filename;
      scan.Match('(');
      scan.ScanString(filename);
      scan.Match(')');
      scan.Match(';');
      if(!scan.Errors)
      {
        sString<256> buffer;
        buffer.PrintF(L"security risk\ndo you really want to execute\n%q",filename);
        if(sSystemMessageDialog(buffer,sSMF_YESNO))
          sExecuteShellDetached(filename);
      }
    }
    else if(scan.IfName(L"inline"))
    {
      sTextBuffer *tb = FakeScript_Inline(scan,output,0);
      if(tb&&output)
        output->Print(tb->Get());
    }   
    else if(scan.IfName(L"inlinechapter"))
    {
      sPoolString chapter;
      sTextBuffer *tb = FakeScript_Inline(scan,output,&chapter);
      const sChar *start = 0;
      const sChar *end = 0;
      if(tb&&output)
      {
        FakeScript_FindChapter(tb,start,end,0,chapter);
        if(end)
          output->Print(start,end-start);
        else
          scan.Error(L"inline: chapter %q not found in file",chapter);
      }
    }   
    else if(scan.IfName(L"inlinesection"))
    {
      sPoolString chapter;
      sTextBuffer *tb = FakeScript_Inline(scan,output,&chapter);
  //    sInt len = sGetStringLen(chapter);
      const sChar *start = 0;
      const sChar *end = 0;
      if(tb&&output)
      {
        FakeScript_FindChapter(tb,start,end,1,chapter);
        if(end)
          output->Print(start,end-start);
        else
          scan.Error(L"inline: chapter %q not found in file",chapter);
      }
    }   
    else if(scan.IfName(L"print"))
    {
      sPoolString text;
      scan.Match('(');
      scan.ScanString(text);
      scan.Match(')');
      scan.Match(';');
      if(!scan.Errors && output)
      {
        output->Print(text);
      }
    }   
    else if(scan.IfName(L"newtab"))
    {
      scan.Match('(');
      scan.Match(')');
      scan.Match(';');
      CmdNewTab();
    }
    else if(scan.IfName(L"pushtab"))
    {
      scan.Match('(');
      scan.Match(')');
      scan.Match(';');
      TabStack = App->TabWin->GetTab();
    }
    else if(scan.IfName(L"poptab"))
    {
      scan.Match('(');
      scan.Match(')');
      scan.Match(';');
      App->TabWin->SetTab(TabStack);
      TabStack = 0;
    }
    else if(scan.IfName(L"noprint"))
    {
      scan.Match(';');
      if(Parser->PageMode)
        break;
    }
    else
    {
      scan.Error(L"syntax error");
    }
  }
  if(scan.Errors && output)
    output->Print(scan.ErrorMsg);

  if(terminated) delete[] terminated;
}

/****************************************************************************/

void MainWindow::UpdateWindowTitle()
{
  sString<sMAXPATH> buffer;
  if(GetTab()->Filename.IsEmpty())
    buffer.PrintF(L"Planpad V%d.%d",VERSION,REVISION);
  else
    buffer.PrintF(L"Planpad V%d.%d - %s",VERSION,REVISION,GetTab()->Filename);
  if(DocChanged)
    buffer.Add(L" - changed");
  if(TextWin->EditFlags & sTEF_STATIC)
    buffer.Add(L" - readonly");
  buffer.Add(L" - ");
  buffer.Add(GetTab()->Dir);

  if(DebugUndo)
  {
    sString<64> s;
    sInt u,r;
    TextWin->UndoGetStats(u,r);
    if(u>0 || r>0)
      s.PrintF(L" - undos %d redos %r",u,r);
    buffer.Add(s);
  }

  if(buffer!=WindowTitle)
  {
    WindowTitle = buffer;
    sSetWindowName(buffer);
  }
}

/****************************************************************************/

void MainWindow::Load(const sChar *dir,const sChar *file,const sChar *gotoheadline)
{
  // save old file

  if(!GetTab()->Filename.IsEmpty() && DocChanged)
    CmdSave();
  Unprotect();

  // figure out new tab data

  sInt tab = TabWin->GetTab();
  sVERIFY(tab>=0);

  sString<sMAXPATH> s;
  s = file;
  sInt n = sFindFirstChar(s,'.');
  if(n>=0)
    s[n] = 0;

  sString<sMAXPATH> path;

  path = dir;
  path.AddPath(file);

  Tabs[tab]->Label = s;
  Tabs[tab]->Filename = file;
  Tabs[tab]->Dir = dir;
  Tabs[tab]->Path = path;

  // load file

  const sChar *data = sLoadText(Tabs[tab]->Path);
  const sChar *deltext = data;

  sResourceManager->Flush();
  ClearFileCache();

  if(data!=0)
  {
    Text = data;
  }
  else
  {
    sString<256> buffer;
    buffer = file;
    sInt n = sFindFirstChar(buffer,'.');
    if(n>=0)
      buffer[n]=0;
    
    Text.Clear();
    Text.PrintF(L"= %s =\n\n",buffer);
  }
  Undo = Text.Get();
  TextWin->SetText(&Text);
  DocChanged = 0;
  sBool prot,ok;
  ok = sGetFileWriteProtect(Tabs[tab]->Path,prot);
  if(prot && ok)
    TextWin->EditFlags |= sTEF_STATIC;
  else
    TextWin->EditFlags &= ~sTEF_STATIC;
  if(!prot && ok)
    Protect(Tabs[tab]->Path);

  ViewWin->ScrollY = ViewWin->ScrollX = 0;
  UpdateDoc(gotoheadline);
  delete[] deltext;

  TabHistory th;
  th.Dir = Tabs[tab]->Dir;
  th.File = Tabs[tab]->Filename;
  Tabs[tab]->History.AddTail(th);

  TabWin->Update();
  
  UpdateWindowTitle();
}

void MainWindow::CmdOpen()
{
  sString<sMAXPATH> path;
  if(sSystemOpenFileDialog(L"open",L"txt",sSOF_LOAD,path))
  {
    sString<sMAXPATH> buffer;
    buffer.PrintF(L"%p",path);
    sInt pos = sFindLastChar(buffer,'/');
    if(pos>0)
    {
      buffer[pos]=0;
      Load(buffer,buffer+pos+1,0);
    }
    else
    {
      Load(OriginalPath,path,0);
    }
  }
}

void MainWindow::CmdSave()
{
  if(!GetTab()->Filename.IsEmpty())
  {
    DocChanged = 0;             // do this first, otherwise we get called from WM_ACTIVATE when opening a window MsgBox

    sString<sMAXPATH> filename;
    filename.PrintF(L"%p",GetCurrentFile());
    const sChar *s = filename;
    while(*s)
    {
      while(*s!=0 && *s!='/') s++;
      if(*s==0) break;
      sString<sMAXPATH> dirname;
      dirname.Init(filename,s-filename);
      if(!sCheckDir(dirname))
      {
        sString<sMAXPATH> error;
        error.PrintF(L"Directory %q does not exist.\nShould I create it?",dirname);
        if(!sSystemMessageDialog(error,sSMF_YESNO,L"Planpad"))
          break;
        sMakeDir(dirname);
      }

      s++;
    }


    Unprotect();
    sBool ok = 0;
    if(SaveAsUTF8)
      ok = sSaveTextUTF8(GetCurrentFile(),Text.Get());
    else
      ok = sSaveTextAnsi(GetCurrentFile(),Text.Get());
    if(ok)
    {
      Protect(GetCurrentFile());
      Undo = Text.Get();
      UpdateWindowTitle();
    }
    else
    {
      sSystemMessageDialog(L"Failed to save file",sSMF_ERROR);
    }
  }
}

void MainWindow::CmdSaveAs()
{
  Unprotect();
  sString<sMAXPATH> path;
  if(sSystemOpenFileDialog(L"save as",L"*.txt",sSOF_SAVE,path))
    sSaveTextAnsi(path,Text.Get());
}

void MainWindow::CmdClear()
{
  if(!GetTab()->Filename.IsEmpty() && DocChanged)
    CmdSave();
  Unprotect();

  Text = L"= Empty Document =";
  Undo = Text.Get();
  GetTab()->Filename = L"";
  DocChanged = 0;
  TextWin->EditFlags |= sTEF_STATIC;
  UpdateDoc();
  ViewWin->ScrollY = ViewWin->ScrollX = 0;

//  buffer.PrintF(L"Layoutwindow V%d.%d Example Application",VERSION,REVISION);
  UpdateWindowTitle();
}

void MainWindow::CmdOpenInclude()
{
  if(!FileCache.IsEmpty())
  {
    sMenuFrame *mf = new sMenuFrame(this);
    mf->AddHeader(L"files");
    FileCacheItem *item;
    sFORALL(FileCache,item)
      mf->AddItem(item->Name,sMessage(this,&MainWindow::CmdOpenInclude2,_i),0);
    sGui->AddPopupWindow(mf);
  }
  else
  {
    sOkDialog(sMessage(),L"this text has no references with 'inline()'");
  }
}

void MainWindow::CmdOpenInclude2(sDInt n)
{
  if(n>=0 && n<FileCache.GetCount())
    Load(GetTab()->Dir,FileCache[n]->Name,0);
}

void MainWindow::CmdOpenDict()
{
  sLoadDir(DictDir,L"spelldict",L"user*.txt");
  if(!DictDir.IsEmpty())
  {
    sMenuFrame *mf = new sMenuFrame(this);
    mf->AddHeader(L"files");
    sDirEntry *item;
    sFORALL(DictDir,item)
      mf->AddItem(item->Name,sMessage(this,&MainWindow::CmdOpenDict2,_i),0);
    sGui->AddPopupWindow(mf);
  }
  else
  {
    sOkDialog(sMessage(),L"No Dictionaries found");
  }
}

void MainWindow::CmdOpenDict2(sDInt n)
{
  if(n>=0 && n<DictDir.GetCount())
  {
    sString<256> buffer;
    buffer = L"spelldict";
    buffer.AddPath(DictDir[n].Name);
    Load(L"",buffer,0);
  }
}

void MainWindow::CmdLoadDict()
{
  sDeleteAll(Parser->Dicts);
  UpdateDoc();
}

void MainWindow::CmdRemWriteProtect()
{
  if(ProtectedFile.IsEmpty())
  {
    sSetFileWriteProtect(GetCurrentFile(),0);
    TextWin->EditFlags &= ~sTEF_STATIC;
    UpdateWindowTitle();
    Protect(GetCurrentFile());
  }
}

void MainWindow::CmdTab()
{
  sInt tab = TabWin->GetTab();
  if(tab>=0)
  {
    if(!GetTab()->Filename.IsEmpty() && DocChanged)
      CmdSave();
    Unprotect();
    CurrentTab = Tabs[tab];
    Load(Tabs[tab]->Dir,Tabs[tab]->Filename,0);
  }
}

void MainWindow::CmdNewTab()
{
  sInt index = Tabs.GetCount();
  Tabs.AddTail(new Tab(L"index"));
  TabWin->SetTab(index);
}

void MainWindow::CmdSwitch(sDInt n)
{
  sWire->SwitchScreen(n);
}

void MainWindow::CmdToggle()
{
  if(!TextWin->HasSelection())
  {
    if(sWire->GetScreen()==1)
      sWire->SwitchScreen(0);
    else
      sWire->SwitchScreen(1);
  }
  else
  {
    TextWin->sTextWindow::OnKey(sKEY_TAB);
  }
}

void MainWindow::CmdToggleBoth()
{
  if(!TextWin->HasSelection())
  {
    if(sWire->GetScreen()==2 || sWire->GetScreen()==3)
      sWire->SwitchScreen(1);
    else
      sWire->SwitchScreen(2);
  }
  else
  {
    TextWin->sTextWindow::OnKey(sKEY_TAB|sKEYQ_SHIFT);
  }
}

void MainWindow::CmdUndo()
{
  Text = Undo.Get();
  DocChanged = 0;
  UpdateWindowTitle();
  UpdateDoc();
}

void MainWindow::CmdExit()
{
  if(!GetTab()->Filename.IsEmpty() && DocChanged)
    CmdSave();
  sExit();
}

void MainWindow::CmdExitDiscard()
{
  sYesNoDialog(sMessage(this,&MainWindow::CmdExitDiscard2),sMessage(),L"Exit without saving?");
}

void MainWindow::CmdExitDiscard2()
{
  Unprotect();
  GetTab()->Filename = L"";
  sExit();
}


void MainWindow::CmdIndex()
{
  GetTab()->History.Clear();
  Load(GetTab()->Dir,L"index.wiki.txt",0);
}

void MainWindow::CmdStartIndex()
{
  CmdClear();
  GetTab()->History.Clear();
  Load(OriginalPath,L"index.wiki.txt",0);
}

void MainWindow::CmdHtml()
{
  sTextBuffer tb;
  ViewWin->MakeHtml(tb);
  sSaveTextUnicode(L"dummy.html",tb.Get());
  sExecuteOpen(L"dummy.html");
}


void MainWindow::CmdPdf()
{
  sInt oldmode = ViewWin->PageMode;
  ViewWin->PageMode = 1;
  UpdateDoc();
  ViewWin->DoLayout();
  ViewWin->Layout();

  sLBPdfState pdfs;

  ViewWin->Root->MakePDF(&pdfs);

  sExecuteOpen(L"dummy.pdf");

  ViewWin->PageMode = oldmode;
  UpdateDoc();
  ViewWin->DoLayout();
  ViewWin->Layout();
}

void MainWindow::CmdFontSize()
{
  switch(FontSize)
  {
  case 0:
    for(sInt i=0;i<sRFF_MAX;i++)
      sResourceManager->SetLogFont(i,L"Arial",14);
    sResourceManager->SetLogFont(sRFF_FIXED,L"Courier New",13);
    sResourceManager->SetLogFont(sRFF_SYMBOL,L"Webdings",14);
    break;
  case 1:
    for(sInt i=0;i<sRFF_MAX;i++)
      sResourceManager->SetLogFont(i,L"Arial",16);
    sResourceManager->SetLogFont(sRFF_FIXED,L"Courier New",15);
    sResourceManager->SetLogFont(sRFF_SYMBOL,L"Webdings",16);
    break;
  case 2:
    for(sInt i=0;i<sRFF_MAX;i++)
      sResourceManager->SetLogFont(i,L"Arial",22);
    sResourceManager->SetLogFont(sRFF_FIXED,L"Courier New",20);
    sResourceManager->SetLogFont(sRFF_SYMBOL,L"Webdings",22);
    break;
  }

  UpdateDoc();
}

void MainWindow::CmdEditFontSize()
{
  sRelease(TextEditFont);
  switch(EditFontSize)
  {
  case 0:
    TextEditFont = sResourceManager->NewFont(L"Courier New",14,0);
    break;
  case 1:
    TextEditFont = sResourceManager->NewFont(L"Courier New",16,0);
    break;
  case 2:
    TextEditFont = sResourceManager->NewFont(L"Courier New",20,0);
    break;
  }
  TextWin->HintLine = RightBorder ? 80 : 0;
  TextWin->SetFont(TextEditFont->Font);
  TextWin->Update();
}

void MainWindow::CmdPageMode()
{
  UpdateDoc();
}

void MainWindow::CmdChapterFormfeed()
{
  UpdateDoc();
}

void MainWindow::CmdDebugBoxes()
{
  ViewWin->DebugBoxes = !ViewWin->DebugBoxes;
  ViewWin->Update();
}

void MainWindow::CmdCursorChange()
{
  ViewWin->CursorChar = TextWin->GetText()->Get() + TextWin->GetCursorPos();
  ViewWin->ScrollToText(ViewWin->CursorChar);
  ViewWin->Update();
}

void MainWindow::CmdCursorFlash()
{
  ViewWin->CursorFlash = TextWin->GetCursorFlash();
  ViewWin->Update();
}

void MainWindow::CmdFindIncremental()
{
  TextWin->Find(FindString,FindDir,0);
}

void MainWindow::CmdFindDone()
{
  sGui->SetFocus(TextWin);
}

void MainWindow::CmdFind(sDInt mode)
{
  switch(mode)
  {
  case 0:
    sGui->SetFocus(FindStringWin);
    break;
  case 1:
    FindDir = 0;
    FindDirWin->Update();
    TextWin->Find(FindString,FindDir,1);
    break;
  case 2:
    FindDir = 1;
    FindDirWin->Update();
    TextWin->Find(FindString,FindDir,1);
    break;
  }
}

void MainWindow::CmdBack()
{
  sInt n = GetTab()->History.GetCount();
  if(n>=2)
  {
    Load(GetTab()->History[n-2].Dir,GetTab()->History[n-2].File,0);
    GetTab()->History.RemTail();
    GetTab()->History.RemTail();
  }
}

void MainWindow::CmdMark(sDInt n)
{
  const sChar *marks = L"ibus";
  sInt t0,t1;
  TextWin->GetMark(t0,t1);
  if(t1>t0)
  {
    TextWin->SetCursor(t1);
    TextWin->InsertChar(']');
    TextWin->SetCursor(t0);
    TextWin->InsertChar('[');
    TextWin->InsertChar(marks[n]);
    TextWin->InsertChar(' ');
    sGui->SetFocus(TextWin);
  }
}

void MainWindow::CmdMarkRed()
{
  sInt t0,t1;
  TextWin->GetMark(t0,t1);
  if(t1>t0)
  {
    TextWin->SetCursor(t1);
    TextWin->InsertChar(']');
    TextWin->SetCursor(t0);
    TextWin->InsertString(L"[tc ");
    sGui->SetFocus(TextWin);
  }
}
void MainWindow::CmdRemoveMark()
{
  sInt t0 = TextWin->GetCursorPos();
  const sChar *text = TextWin->GetText()->Get();

  while(t0>0 && text[t0]!='[')
    t0--;
  if(text[t0]=='[')
  {
    sInt t1 = t0+1;
    sInt c = 1;
    while(c && text[t1])
    {
      if(text[t1]=='[') c++;
      if(text[t1]==']') c--;
      t1++;
    }
    t1--;
    sInt t0c = t0;
    while(text[t0c]!=0 && text[t0c]!=' ') 
      t0c++;

    if(text[t0c]==' ' && text[t1]==']')
    {
      TextWin->SetCursor(t1);
      TextWin->DeleteChar();
      TextWin->SetCursor(t0);
      for(sInt i=0;i<t0c-t0;i++)
        TextWin->DeleteChar();
    }
    sGui->SetFocus(TextWin);
  }
}

/****************************************************************************/

void sMain()
{
//  sBreakOnAllocation(816);
  sAddResourceManager();
  sInit(sISF_2D,1024,768);
  sInitGui();
  sGui->AddBackWindow(new MainWindow);
}

/****************************************************************************/
