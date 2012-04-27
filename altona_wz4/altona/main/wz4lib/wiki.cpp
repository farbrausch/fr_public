/****************************************************************************/

#include "wiki.hpp"
#include "doc.hpp"
#include "gui.hpp"

#include "wiki/layoutwindow.hpp"
#include "wiki/markup.hpp"
#include "wiki/resources.hpp"
#include "gui/gui.hpp"
#include "gui/textwindow.hpp"
#include "wz4lib/doc.hpp"

static void FakeScript(const sChar *code,sTextBuffer *output)
{
  App->Wiki->FakeScript(code,output);
}

/****************************************************************************/


WikiHelper::WikiHelper()
{
  sAddResourceManager();

  WikiPath = 0;
  WikiCheckout = 0;
  Commit = 0;
  Update = 1;
  Inititalize = 1;
  UpdateIndexWhenChanged = 1;
  Parser = new Markup;
  Parser->Script = ::FakeScript;
  Op = 0;

  WriteWin = new sWireTextWindow();
  WriteWin->SetText(&Text);
  WriteWin->SetFont(sGui->FixedFont);
  WriteWin->ChangeMsg = sMessage(this,&WikiHelper::CmdWikiChange);
  WriteWin->CursorMsg = sMessage(this,&WikiHelper::CmdCursorChange);
  WriteWin->BackColor = sGC_DOC;
  WriteWin->ShowCursorAlways = 1;

  WriteWin->InitWire(L"WikiWrite");
  WriteWin->AddBorder(new sFocusBorder);
  WriteWin->AddScrolling(0,1);

  ReadWin = new sLayoutWindow();
  ReadWin->InitWire(L"WikiRead");
  ReadWin->AddBorder(new sFocusBorder);
  ReadWin->AddScrolling(0,1);
  ReadWin->ActivateOnKeyWindow = WriteWin;

  sWire->AddDrag(L"WikiRead",L"Click",sMessage(this,&WikiHelper::DragClickView));
}

WikiHelper::~WikiHelper()
{
  delete Parser;
  Close();
  if(Commit)
  {
    Execute(L"svn add %s/*.wiki.txt");
    Execute(L"svn add %s/presets/*.preset");
    sString<256> user;
    sGetUserName(user,0);
    if(user.IsEmpty())
      user = L"unknown";
    Execute(L"svn commit %s --username wiki --password qwer12 --message '%s'",user);
  }
}

void WikiHelper::InitWire(const sChar *name)
{
  sWire->AddKey(name,L"WikiIndex",sMessage(this,&WikiHelper::CmdIndex));
  sWire->AddKey(name,L"WikiDefault",sMessage(this,&WikiHelper::CmdDefault));
  sWire->AddKey(name,L"WikiBack",sMessage(this,&WikiHelper::CmdBack));
  sWire->AddKey(name,L"WikiLink",sMessage(this,&WikiHelper::CmdLink));
  sWire->AddKey(name,L"WikiUndo",sMessage(this,&WikiHelper::CmdUndo));
}

void WikiHelper::Execute(const sChar *format,const sChar *a,const sChar *b)
{
  sString<1024> svn;
  sTextBuffer error;

  svn.PrintF(format,WikiPath,a,b);

  sLogF(L"svn",L"%s\n",svn);
  
  sExecuteShell(svn,&error);

  sLogF(L"svn",error.Get());
}

void WikiHelper::Tag()
{
  WriteWin->Need();
  ReadWin->Need();
}

void WikiHelper::CmdWikiChange()
{
  MarkupFile mf;
  mf.Text = &Text;

  Parser->Clear();
  Parser->PageMode = ReadWin->PageMode;
//  Parser->FindHeadline = gotoheadline;
  Parser->Parse(&mf);
  ReadWin->Clear();
  Parser->Generate(ReadWin->Root);
  if(!Parser->Title.IsEmpty())
    ReadWin->Root->Page.Title = Parser->Title;

  ReadWin->Layout();
  ReadWin->Update();

  WriteWin->Update();
}

void WikiHelper::CmdCursorChange()
{
  ReadWin->CursorChar = WriteWin->GetText()->Get() + WriteWin->GetCursorPos();
  ReadWin->ScrollToText(ReadWin->CursorChar);
  ReadWin->Update();
}

void WikiHelper::CmdUndo()
{
  Text = Original;
  UpdateWindows();
}

void WikiHelper::SetOp(wOp *op)
{
  if(op)
  {
    wClass *cl = op->Class;
    sString<256> name;
    name.PrintF(L"op_%s_%s",cl->OutputType->Symbol,cl->Name);
    Load(name,op->Class->WikiText);
    Op = op;
  }
  else
  {
    CmdIndex();
  }
}

void WikiHelper::InitSvn()
{
  if(Inititalize)
  {
    Inititalize = 0;
    if(!sCheckDir(WikiPath))
    {
      sMakeDirAll(WikiPath);
      if(WikiCheckout)
        Execute(WikiCheckout);
    }
    if(Update)
    {
      Execute(L"svn update %s");
      Update = 0;
    }
  }

  if(0) // for debugging
    UpdateIndex();
}

void WikiHelper::Load(const sChar *name,const sChar *def)
{
  const sChar *str = 0;
  Close();
  if(WikiPath)
  {
    InitSvn();
    Filename.PrintF(L"%s/%s.wiki.txt",WikiPath,name);
    str = sLoadText(Filename);
    History.AddTail(sPoolString(name));
  }
  if(str)
  {
    Text = str;
    Original = str;
    delete[] str;
  }
  else
  {
    if(def==0)
      Text.PrintF(L"= %s",name);
    else
      Text = def;
    Original = Text.Get();
  }
  UpdateWindows();
}

void WikiHelper::UpdateWindows()
{
  ReadWin->ScrollY = 0;
  ReadWin->ScrollX = 0;
  WriteWin->ScrollY = 0;
  WriteWin->ScrollX = 0;
  ReadWin->Layout();
  WriteWin->Layout();
  CmdWikiChange();
}

void WikiHelper::Close()
{
  if(!Filename.IsEmpty() && sCmpString(Original.Get(),Text.Get())!=0)
  {
    Commit = 1;
    sSaveTextAnsi(Filename,Text.Get());
    Filename = L"";
    if(UpdateIndexWhenChanged)
    {
      UpdateIndex();
      UpdateIndexWhenChanged = 0;
    }
  }
  Text.Clear();
  Original.Clear();
  Filename = L"";
  Op = 0;
}

template <class ArrayType,class BaseType,class MemberType> 
void sSortUpString(ArrayType &a,MemberType BaseType::*o)
{
  sInt max = a.GetCount();
  for(sInt i=0;i<max-1;i++)
    for(sInt j=i+1;j<max;j++)
      if(sCmpStringI(sGetPtr(a,i)->*o,sGetPtr(a,j)->*o)>0)
        sSwap(a[i],a[j]);
}

void WikiHelper::UpdateIndex()
{
  sTextBuffer tb;
  wClass *cl;
  wType *type;
  sString<512> filename;

  // write main directory

  filename.PrintF(L"%s/index_gen.wiki.txt",WikiPath);
  tb.Clear();
  sArray<wType *> types_major;
  sArray<wType *> types_minor;
  sFORALL(Doc->Types,type)
  {
    if((type->Flags & wTF_NOTAB) || type->Secondary)
      types_minor.AddTail(type);
    else
      types_major.AddTail(type);
  }
  sSortUpString(types_minor,&wType::Label);
  sSortUpString(types_major,&wType::Label);

  tb.Print(L"== Major Types\n\n");
  sFORALL(types_major,type)
    tb.PrintF(L"~~ [> \"type_%s\" %s]\n",type->Symbol,type->Label);
  tb.Print(L"== Minor Types\n\n");
  sFORALL(types_minor,type)
    tb.PrintF(L"~~ [> \"type_%s\" %s]\n",type->Symbol,type->Label);

  tb.Print(L"\n");
  sSaveTextAnsiIfDifferent(filename,tb.Get());

  // write subdirectories

  sArray<wClass *> classes = Doc->Classes;
  sSortUp(classes,&wClass::Label);
  sFORALL(Doc->Types,type)
  {
    filename.PrintF(L"%s/type_%s.wiki.txt",WikiPath,type->Symbol);
    tb.Clear();
    tb.PrintF(L"= %s\n\n",type->Label);
    sFORALL(classes,cl)
    {
      if(cl->OutputType==type)
      {
        tb.PrintF(L"~~ [> \"op_%s_%s\" %s]\n",type->Symbol,cl->Name,cl->Label);        
      }
    }
    tb.Print(L"\n");
    sSaveTextAnsiIfDifferent(filename,tb.Get());
  }
}

/****************************************************************************/

void WikiHelper::CmdIndex()
{
  History.Clear();
  Load(L"index");
}

void WikiHelper::CmdBack()
{
  if(History.GetCount()>1)
  {
    History.RemTail();
    sPoolString str = History.RemTail();
    Load(str);
  }
}

void WikiHelper::CmdDefault()
{
  if(Op)
  {
    Text.Print(L"\n---------------\n\n");
    Text.Print(Op->Class->WikiText);
    CmdWikiChange();
  }
}

void WikiHelper::CmdLink()
{
  sString<256> name;
  sString<256> link;
  sInt n;

  const sChar *path = Filename;
  n = sFindLastChar(path,'/');
  if(n>0)
    path += n+1;
  n = sFindLastChar(path,'\\');
  if(n>0)
    path += n+1;
  name = path;
  n = sFindFirstChar(name,'.');
  if(n>=0)
    name[n] = 0;

  const sChar *label = name;
  if(sCmpStringLen(label,L"op_",3)==0)
  {
    n = sFindLastChar(label,'_');
    if(n>0)
      label += n+1;
  }

  link.PrintF(L"[> %q %s]",name,label);
  sSetClipboard(link);
}

/****************************************************************************/

void WikiHelper::DragClickView(const sWindowDrag &dd)
{
  sLayoutBox *box;
  ReadWin->MMBScroll(dd);
  
  switch(dd.Mode)
  {
  case sDD_START:
    ClickViewHasHit = 0;
  case sDD_DRAG:
    box = ReadWin->FindBox(dd.MouseX,dd.MouseY,ReadWin->Root);
    if(box)
    {      
      if(dd.Buttons==1)
      {
        const sChar *text = box->Click(dd.MouseX,dd.MouseY);
        const sChar *s = WriteWin->GetText()->Get();
        const sChar *e = s + WriteWin->GetText()->GetCount();
        if(text>=s && text<=e)
        {
          ClickViewHasHit = 1;
          WriteWin->SetCursor(text-s);
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
  }
}

/****************************************************************************/

void WikiHelper::FakeScript(const sChar *code,sTextBuffer *output)
{
  sChar *terminated = 0;
  sScanner scan;
  scan.Init();
  scan.DefaultTokens();
  scan.Start(code);

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
        Load(filename);
      }
    }
    else if(scan.IfName(L"inline"))
    {
      sPoolString filename;
      scan.Match('(');
      scan.ScanString(filename);
      scan.Match(')');
      scan.Match(';');
      const sChar *str = 0;
      if(!scan.Errors && output)
      {
        sString<sMAXPATH> path;
        if(sCheckFileExtension(filename,L"wiki.txt"))
          path.PrintF(L"%s/%s",WikiPath,filename);
        else
          path.PrintF(L"%s/%s.wiki.txt",WikiPath,filename);
        str = sLoadText(path);
        if(!str)
        {
          scan.Error(L"could not load file %q",filename);
        }
      }
      if(str&&output)
        output->Print(str);
      if(str)
        delete[] str;
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
