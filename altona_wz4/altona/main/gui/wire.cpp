/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "gui/gui.hpp"
#include "util/scanner.hpp"
#include "base/system.hpp"
#include "base/windows.hpp"
#include "gui/overlapped.hpp"

sWireMasterWindow *sWire;

/****************************************************************************/

namespace sWireNamespace {
;

struct Shortcut;
struct FormInstance;

/****************************************************************************/

enum CommandType
{
  CT_COMMAND = 1,
  CT_DRAG = 2,
  CT_MENU = 3,
  CT_SPACER = 4,
  CT_CONTEXT = 5,
  CT_CALLBACK = 6,
  CT_TOOL = 7,
  CT_CHOICE = 8,
};

const sChar *CTString[] =
{
  L"???",
  L"key",
  L"drag",
  L"menu",
  L"spacer",
  L"context",
  L"callback",
  L"tool",
  L"choice",
};

enum QualifierEnum
{
  QUAL_MISS       = 0x00000001,     // valid when hit flag is set
  QUAL_HIT        = 0x00000002,     // valid when hit flag is not set
  QUAL_REPEAT     = 0x00000004,     // repeated keys are passed
  QUAL_DRAG       = 0x00000008,
  QUAL_FREEZE     = 0x00000020,     // freeze cursor
  QUAL_DOUBLE     = 0x00000040,     // double-click
  QUAL_GLOBAL     = 0x00000080,     // make this a global shortcut
  QUAL_ONOFF      = 0x00000100,     // send key-on (sDInt=1) and key-off (sDInt=0)
  QUAL_ALLSHIFT   = 0x00000200,     // don't care about shift/ctrl/alt/alt-gr
  QUAL_CURSOR     = 0x00000400,     // for help menu: to cursor column
  QUAL_EXTRA      = 0x00000800,     // for help menu: to extra column
  QUAL_EXTRA2     = 0x00001000,     // for help menu: to extra column
  QUAL_EXTRA3     = 0x00002000,     // for help menu: to extra column

  QUAL_SCOPE0     = 0x01000000,     // like global scope, but you can switch scopes (with screens, for intance)
  QUAL_SCOPE1     = 0x02000000,
  QUAL_SCOPE2     = 0x04000000,
  QUAL_SCOPE3     = 0x08000000,
  QUAL_SCOPE4     = 0x10000000,
  QUAL_SCOPE5     = 0x20000000,
  QUAL_SCOPE6     = 0x40000000,
  QUAL_SCOPE7     = 0x80000000,
  QUAL_SCOPEMASK  = 0xff000000,


  KEY_LMB         = sKEY_LMB,
  KEY_RMB         = sKEY_RMB,
  KEY_MMB         = sKEY_MMB,
  KEY_X4          = sKEY_X1MB,
  KEY_X5          = sKEY_X2MB,
  KEY_NULL        = 1,
  KEY_HOVER       = sKEY_HOVER,
};

// defined by code

struct CommandBase
{
  sPoolString Symbol;             // how it is called
  sPoolString Text;             // name for user
  sInt Type;                    // Command Type
  sMessage Message;

  virtual void AddMenu(struct Shortcut *) {}
  virtual ~CommandBase() {}
};

struct CommandKey : public CommandBase
{
  void AddMenu(struct Shortcut *);
};

struct CommandDrag : public CommandBase
{
};

struct CommandMenu : public CommandBase
{
  void AddMenu(struct Shortcut *);
  sArray<Shortcut *> Items;  // this is a menu!
  ~CommandMenu() { sDeleteAll(Items); }
};

struct CommandSpacer : public CommandBase
{
  void AddMenu(struct Shortcut *);
};

struct CommandContext : public CommandBase
{
  void AddMenu(struct Shortcut *);
  Form *Context;
};

struct CommandCallback : public CommandBase
{
  void AddMenu(struct Shortcut *);
};

struct CommandTool : public CommandBase
{
  void AddMenu(struct Shortcut *);
};

struct CommandChoice : public CommandBase
{
  sInt *Ptr;
  sPoolString Choices;
  void AddMenu(struct Shortcut *);
};

struct Parameter                // parameter for a form
{
  sPoolString Symbol;             // symbol name
  sPoolString Text;             // user name
  sInt Type;                    // sWPT_???
  sDInt Offset;                 // offset in form
  sF32 Min,Max,Step;            // for int and float
  sPoolString Choices;          // for choices

  sMessage Message;
};

struct FormInstance             // an instance
{
  sObject *Object;              // object link
  const sChar *Symbol;            // instances may be named
  Form *Class;                  // backlink to form
  FormInstance() : Object(0),Symbol(0),Class(0) {}
  FormInstance(sObject *o,const sChar *n,Form *f) : Object(o),Symbol(n),Class(f) {}
};

struct SetInfo
{
  sPoolString Symbol;
  sInt Mask;
};

struct Form              // there may be multiple windows of same kind!
{
  sPoolString Symbol;            // symbolic name
  sPoolString Text;             // name for user
  sBool IsWindow;               // this is a window.
  sBool IsInScreen;             // window was used in screen.
  const sChar *ClassName;       // sObject::GetClassName()
  CommandTool *CurrentTool;     // current active tool
  sArray<FormInstance> Instances;    // the actual data (windows)
  sArray<CommandBase *> Commands;     // what the window can do (from code)
  sArray<Parameter> Parameters; // parameters
  sArray<SetInfo> SetInfos;

  sArray<Shortcut *> Shortcuts;   // what the user needs to press (from wire.txt)

  Form()
  {
    Symbol = L"";
    Text = L"";
    IsInScreen = IsWindow = 0;
    ClassName = 0;
    CurrentTool = 0;
  }
  ~Form()
  {
    sDeleteAll(Commands);
    sDeleteAll(Shortcuts);
  }
};

// defined by script

struct Shortcut                 // some kind of key binding
{
  CommandBase *Cmd;             // what to do
  FormInstance *Instance;       // with whom. may be 0
  Form *Class;                  // class to bind to. must be set
  sU32 Key;                     // key to press (may be 0)
  sInt Qual;                    // zusätzliche wire qualifier
  sInt Hit;                     // for drags, when QUAL_HIT is set
  sInt Sets;                    // swithc between command sets, 0 = all sets
  CommandTool *Tool;            // use only when tool is active!

  Shortcut() 
  {
    Cmd = 0;
    Instance = 0;
    Class = 0;
    Key = 0;
    Qual = 0;
    Hit = 0;
    Tool = 0;
    Sets = -1;
  }
//  sObject *FindObject();
};

struct RawShortcut
{
  sPoolString Form;
  sPoolString InstanceName;
  sInt InstanceIndex;
  sPoolString Command;
  sPoolString Label;
  sPoolString Tool;
  sU32 Key;
  sInt KeyQual;
  sInt KeyHit;
  sInt Sets;
};

struct Screen                   // register screens
{
  sPoolString Name;             // name in user interface
  sInt Index;                   // index in LayoutScreen
};

struct Layout                   // a part of the screen layout tree
{
  sArray<Layout *> Childs;      // hirarchy
  Form *Window;          // link to window (class)
  sInt Index;                   // link to window (index)
  sInt Position;                // position when the window was last layouted, -1 when it was never layouted
  sBool Align;                  // TRUE when window is right/bottom aligned
  sInt InitialPosition;         // position for initial layout, negative for position relative to right/bottom, 0 for normal layout.
};

/****************************************************************************/

}; // end of namespace
using namespace sWireNamespace;

/****************************************************************************/

void CommandSpacer::AddMenu(Shortcut *sc)
{
  sWire->CurrentMenu->AddSpacer();
}

void CommandMenu::AddMenu(Shortcut *sc)
{
  sWire->CurrentMenu->AddItem(Text,Message,sc->Key);
}

void CommandKey::AddMenu(Shortcut *sc)
{
  sWire->CurrentMenu->AddItem(Text,Message,sc->Key);
}

void CommandTool::AddMenu(Shortcut *sc)
{
  sWire->CurrentMenu->AddItem(Text,Message,sc->Key);
}

void CommandContext::AddMenu(Shortcut *dummy)
{
  Shortcut *sc;
  sInt hastools = 0;
  sInt other = 0;
  sInt sets = -1;
  if(Context->IsWindow && Context->Instances.GetCount()==1)
    sets = ((sWindow *)Context->Instances[0].Object)->WireSets;
  sFORALL(Context->Shortcuts,sc)
  {
    if(sc->Sets & sets)
    {
      switch(sc->Cmd->Type)
      {
      case CT_COMMAND:
        if((sc->Key&sKEYQ_MASK)<KEY_LMB || (sc->Key&sKEYQ_MASK)>KEY_X5)
          sWire->CurrentMenu->AddItem(sc->Cmd->Text,sc->Cmd->Message,sc->Key);
        other++;
        break;
      case CT_TOOL:
        hastools++;
        break;
      }
    }
  }
  if(hastools)
  {
    sInt column = 0;
    if(hastools>4 && other>3)
      column = 1;
    else
      sWire->CurrentMenu->AddSpacer(column);
    sFORALL(Context->Shortcuts,sc)
    {
      if(sc->Sets & sets)
      {
        switch(sc->Cmd->Type)
        {
        case CT_TOOL:
          sWire->CurrentMenu->AddItem(sc->Cmd->Text,sc->Cmd->Message,sc->Key,-1,column);
          break;
        }
      }
    }
  }
}

void CommandCallback::AddMenu(Shortcut *sc)
{
  sMessage msg = Message;
  msg.Code = sDInt(sWire->CurrentMenu);
  msg.Send();
}

void CommandChoice::AddMenu(Shortcut *sc)
{
  sString<256> choice;
  sInt max = sCountChoice(Choices);

  if(max==2 && sCmpStringLen(Choices,L"-|",2)==0)
  {
    sMakeChoice(choice,Choices,1);
    sWire->CurrentMenu->AddCheckmark(sPoolString(choice),Message,sc->Key,Ptr,-1);
  }
  else
  {
    for(sInt i=0;i<max;i++)
    {
      sMakeChoice(choice,Choices,i);
      if(!choice.IsEmpty())
        sWire->CurrentMenu->AddCheckmark(sPoolString(choice),Message,sc->Key,Ptr,i);
    }
  }
}

/****************************************************************************/
/****************************************************************************/

class sLayoutFrameSize : public sLayoutFrame
{
public:
  sInt MaxSizeY;
  sInt MinSizeY;

  sCLASSNAME(sLayoutFrameSize);
  void OnCalcSize(); 
};

void sLayoutFrameSize::OnCalcSize()
{
  ReqSizeX = 0;
  ReqSizeY = 0;

  if(Screens.IsIndexValid(GetSwitch()))
  {
    if(sWindow *w = Screens[GetSwitch()]->Window)
    {
      ReqSizeX = w->DecoratedSizeX;
      ReqSizeY = w->DecoratedSizeY;
    }
  }

  if(MaxSizeY)
    ReqSizeY = sMin(ReqSizeY,MaxSizeY);
  if(MinSizeY)
    ReqSizeY = sMax(ReqSizeY,MinSizeY);

  if(!ReqSizeX || !ReqSizeY)
  {
    ReqSizeX = 400;
    ReqSizeY = 200;
  }
}

void sWireMasterAltF4(sBool &stop,void *user)
{
  if (sWire->OnShortcut(sKEY_ESCAPE|sKEYQ_SHIFT))
  {
    stop=0;
  }
}

sWireMasterWindow::sWireMasterWindow()
{
  sLayoutFrameSize *popupFrame = new sLayoutFrameSize;

  LayoutFrame = new sLayoutFrame;
  PopupFrame = popupFrame;
  AddChild(LayoutFrame);
  popupFrame->AddBorder(new sSizeBorder);
  popupFrame->AddBorder(new sTitleBorder(L"popup",sMessage(this,&sWireMasterWindow::CmdClosePopup)));
  popupFrame->ReqSizeX = 400;
  popupFrame->ReqSizeY = 200;
  popupFrame->MaxSizeY = 300;
  popupFrame->MinSizeY = 300;
  Flags |= sWF_ZORDER_BACK;
  DragModeFocus = 0;
  MainWindow = 0;
  CurrentToolName = L"";
  CurrentScreen = 0;
  FullscreenWindow = 0;
  sAltF4Hook->Add(sWireMasterAltF4);
  ProcessEndWasCalled = 0;
  QualScope = QUAL_SCOPE0;

  AddWindow(L"wire",this);
  AddKey(L"wire",L"Switch0",sMessage(this,&sWireMasterWindow::CmdSwitchScreen,0));
  AddKey(L"wire",L"Switch1",sMessage(this,&sWireMasterWindow::CmdSwitchScreen,1));
  AddKey(L"wire",L"Switch2",sMessage(this,&sWireMasterWindow::CmdSwitchScreen,2));
  AddKey(L"wire",L"Switch3",sMessage(this,&sWireMasterWindow::CmdSwitchScreen,3));
  AddKey(L"wire",L"Switch4",sMessage(this,&sWireMasterWindow::CmdSwitchScreen,4));
  AddKey(L"wire",L"Switch5",sMessage(this,&sWireMasterWindow::CmdSwitchScreen,5));
  AddKey(L"wire",L"Switch6",sMessage(this,&sWireMasterWindow::CmdSwitchScreen,6));
  AddKey(L"wire",L"Switch7",sMessage(this,&sWireMasterWindow::CmdSwitchScreen,7));
  AddKey(L"wire",L"Switch8",sMessage(this,&sWireMasterWindow::CmdSwitchScreen,8));
  AddKey(L"wire",L"Switch9",sMessage(this,&sWireMasterWindow::CmdSwitchScreen,9));
  AddKey(L"wire",L"Switch10",sMessage(this,&sWireMasterWindow::CmdSwitchScreen,10));
  AddKey(L"wire",L"Switch11",sMessage(this,&sWireMasterWindow::CmdSwitchScreen,11));
}

sWireMasterWindow::~sWireMasterWindow()
{
  sAltF4Hook->Rem(sWireMasterAltF4);
  sDeleteAll(Menus);
  sDeleteAll(Screens);
  sDeleteAll(Classes);
  sDeleteAll(Garbage);
  sDeleteAll(Popups);
}

void sWireMasterWindow::Tag()
{
  if(!ProcessEndWasCalled)
    sFatal(L"please call sWire->ProcessEnd() after sWire->ProcessText() or sWire->ProcessFile()");
  sWindow::Tag();
  LayoutFrame->Need();
  PopupFrame->Need();
  sNeed(Forms);
  MainWindow->Need();
  FullscreenWindow->Need();
}

/****************************************************************************/
/*
void sWireMasterWindow::ExecuteCommand(sDInt cmdptr)
{
  CommandBase *cmd = (CommandBase *)cmdptr;
  cmd->OnMenu();
}
*/
/****************************************************************************/

Form *sWireMasterWindow::MakeClass(sPoolString name)
{
  Form *wc;

  wc = FindClass(name);
  if(!wc)
  {
    wc = new Form;
    wc->Symbol = name;
    Classes.AddTail(wc);
  }
  return wc;
}

FormInstance *sWireMasterWindow::FindInstance(sObject *obj)
{
  Form *wc;
  FormInstance *inst;

  sFORALL(Classes,wc)
    sFORALL(wc->Instances,inst)
      if(inst->Object == obj)
        return inst;

  return 0;
}

Form *sWireMasterWindow::FindClass(sPoolString name)
{
  Form *wc;

  wc = sFind(Classes,&Form::Symbol,name);

  return wc;
}

sWindow *sWireMasterWindow::FindWindow(sPoolString name,sInt index)
{
  Form *wc;

  wc = FindClass(name);
  if(wc==0 || index<0 || index>=wc->Instances.GetCount()) return 0;

  sVERIFY(wc->IsWindow);

  return (sWindow *)(wc->Instances[index].Object);
}

/****************************************************************************/

void sWireMasterWindow::AddForm(const sChar *name,sObject *obj,const sChar *instancename,sBool iswindow)
{
  Form *wc;

  wc = MakeClass(name);
  if(wc->ClassName==0)
  {
    wc->ClassName = obj->GetClassName();
    wc->IsWindow = iswindow;
  }
  sVERIFY(wc->ClassName == obj->GetClassName());
  sVERIFY(wc->IsWindow == iswindow);

  wc->Instances.AddTail(FormInstance(obj,instancename,wc));

  Forms.AddTail(obj);
  if(iswindow && sCmpString(name,L"main")!=0)
    Windows.AddTail((sWindow *)obj);
}

void sWireMasterWindow::AddForm(const sChar *name,sObject *obj,const sChar *instancename)
{
  AddForm(name,obj,instancename,0);
}

void sWireMasterWindow::AddWindow(const sChar *name,sWindow *win,const sChar *instancename)
{
  AddForm(name,win,instancename,1);
}

void sWireMasterWindow::AddKey(const sChar *classname,const sChar *cmdname,const sMessage &msg)
{
  Form *wc;
  CommandKey *cmd;
  
  wc = MakeClass(classname);
  cmd = new CommandKey;
  cmd->Symbol = cmdname;
  cmd->Text = cmdname;
  cmd->Type = CT_COMMAND;
  cmd->Message = msg;
  wc->Commands.AddTail(cmd);
}

void sWireMasterWindow::AddTool(const sChar *classname,const sChar *cmdname,const sMessage &msg)
{
  Form *wc;
  CommandKey *cmd;
  
  wc = MakeClass(classname);
  cmd = new CommandKey;
  cmd->Symbol = cmdname;
  cmd->Text = cmdname;
  cmd->Type = CT_TOOL;
  cmd->Message = msg;
  wc->Commands.AddTail(cmd);
}

void sWireMasterWindow::AddDrag(const sChar *classname,const sChar *cmdname,const sMessage &msg)
{
  Form *wc;
  CommandDrag *cmd;
  
  wc = MakeClass(classname);
  cmd = new CommandDrag;
  cmd->Symbol = cmdname;
  cmd->Text = cmdname;
  cmd->Type = CT_DRAG;
  cmd->Message = msg;
  wc->Commands.AddTail(cmd);
}

void sWireMasterWindow::AddCallback(const sChar *classname,const sChar *cmdname,const sMessage &msg)
{
  Form *wc;
  CommandCallback *cmd;
  
  wc = MakeClass(classname);
  cmd = new CommandCallback;
  cmd->Symbol = cmdname;
  cmd->Text = cmdname;
  cmd->Type = CT_CALLBACK;
  cmd->Message = msg;
  wc->Commands.AddTail(cmd);
}

void sWireMasterWindow::AddChoice(const sChar *classname,const sChar *cmdname,const sMessage &msg,sInt *ptr,const sChar *choices)
{
  Form *wc;
  CommandChoice *cmd;
  
  wc = MakeClass(classname);
  cmd = new CommandChoice;
  cmd->Symbol = cmdname;
  cmd->Text = cmdname;
  cmd->Type = CT_CHOICE;
  cmd->Message = msg;
  cmd->Ptr = ptr;
  cmd->Choices = choices;
  wc->Commands.AddTail(cmd);
}

void sWireMasterWindow::AddPara1(const sChar *classname,const sChar *commandname,const sMessage &msg,sInt type,sDInt offset,const sChar *choices,sF32 min,sF32 max,sF32 step)
{
  Form *wc;
  Parameter *para;

  wc = MakeClass(classname);
  para = wc->Parameters.AddMany(1);
  para->Symbol = commandname;
  para->Text = commandname;
  para->Message = msg;
  para->Type = type;
  para->Offset = offset;
  para->Choices = choices;
  para->Min = min;
  para->Max = max;
  para->Step = step;
}


void sWireMasterWindow::OverrideCmd(const sChar *classname,const sChar *cmdname,const sMessage &msg)
{
  Form *wc;
  CommandBase *cmd;
  
  wc = MakeClass(classname);
  sFORALL(wc->Commands,cmd)
  {
    if (cmd->Symbol==cmdname)
    {
      cmd->Message=msg;
      break;
    }
  }
}


/****************************************************************************/

void sWireMasterWindow::ProcessText(const sChar8 *text,const sChar *reffilename)
{
  sChar *copy;
  sInt len;

  len = 0;
  while(text[len]) len++;

  copy = new sChar[len+1];
  sCopyString(copy,text,len+1);
  
  ProcessText(copy,reffilename);
  delete[] copy;
}


void sWireMasterWindow::ProcessText(const sChar *text,const sChar *reffilename)
{
  Filename = L"internal file";

  Scan = new sScanner;
  Scan->Init();
  Scan->AddTokens(L"{}[]();-|%.:,");
  Scan->Start(text);
  Scan->SetFilename(reffilename);

  _Global();

  sVERIFY(Scan->Errors == 0);
  sDelete(Scan);
}


void sWireMasterWindow::ProcessFile(const sChar *name)
{
  Filename = name;

  Scan = new sScanner;
  Scan->Init();
  Scan->AddTokens(L"{}[]();-|%.:,");
  Scan->StartFile(Filename);

  _Global();

  sVERIFY(Scan->Errors == 0);
  sDelete(Scan);
}

void sWireMasterWindow::ProcessEnd()
{
  sWindow *w;
  Screen *scr;
  sLayoutFrameWindow *lfw;

  ProcessEndWasCalled = 1;

  // add radiobutton to switch screens

  sFORALL(Screens,scr)
    scr->Index = _i;
  sFORALL(Popups,scr)
    scr->Index = _i;
  sFORALL(ScreenSwitchAdd,lfw)
  {
    sFORALL(Screens,scr)
    {
      sButtonControl *bc = new sButtonControl(scr->Name,sMessage(this,&sWireMasterWindow::CmdSwitchScreen,_i),0);
      bc->InitRadio(&CurrentScreen,_i);
      Windows.AddTail(bc);
      lfw->Add(new sLayoutFrameWindow(sLFWM_WINDOW,bc,0));
    }
  }

  // add the windows to the layout frame

  sFORALL(Windows,w)
  {
    LayoutFrame->Windows.AddTail(w);
    PopupFrame->Windows.AddTail(w);
  }

  // get started

  CurrentScreen = 0;
  FullscreenWindow = 0;
  LayoutFrame->Switch(CurrentScreen);
}

/****************************************************************************/

void sWireMasterWindow::Popup(const sChar *name)
{
  if(PopupFrame->Parent==0)
  {
    sWireNamespace::Screen *pop = sFind(Popups,&Screen::Name,name);
    if(pop)
    {
      PopupFrame->Switch(pop->Index);
      sWindow *w;

      // TODO: how about a "real" title in wire.txt
      sFORALL(PopupFrame->Borders,w)
        if (w->GetClassName()==sTitleBorder::ClassName())
          ((sTitleBorder*)w)->Title=pop->Name;

      if(PopupFrame->Client.SizeX()>0)
      {
        sGui->AddWindow(PopupFrame);
        sGui->SetFocus(PopupFrame);
      }
      else
      {
        sGui->AddPopupWindow(PopupFrame);     // first time
      }

      sGui->SetFocus(PopupFrame->Childs[0]);
    }
  }
}


void sWireMasterWindow::CmdSwitchScreen(sDInt screen)
{
  CurrentScreen = screen;
  FullscreenWindow = 0;
  DoLayout();
}

void sWireMasterWindow::SetSubSwitch(sPoolString name,sInt nr)
{
  LayoutFrame->SetSubSwitch(name,nr);
  DoLayout();
}

sInt sWireMasterWindow::GetSubSwitch(sPoolString name)
{
  return LayoutFrame->GetSubSwitch(name);
}

void sWireMasterWindow::SetFullscreen(sWindow *win,sBool toggle)
{
  if(toggle && win==FullscreenWindow)
    win = 0;
  FullscreenWindow = win;
  DoLayout();
}

void sWireMasterWindow::DoLayout()
{
  if(!ProcessEndWasCalled)
    sFatal(L"please call sWire->ProcessEnd() after sWire->ProcessText() or sWire->ProcessFile()");

  Childs.Clear();
  if(FullscreenWindow)
  {
    AddChild(FullscreenWindow);
  }
  else
  {
    AddChild(LayoutFrame);
    LayoutFrame->Switch(CurrentScreen);
  }
}

void sWireMasterWindow::CmdMakeMenu(CommandMenu *menu)
{
  Shortcut *sc;

  sWire->CurrentMenu = new sMenuFrame();
  sWire->CurrentMenu->AddBorder(new sThickBorder);
  
  sInt sets = sGui->GetFocus()->WireSets;

  sFORALL(menu->Items,sc)
    if(sets & sc->Sets)
      sc->Cmd->AddMenu(sc);
}

void sWireMasterWindow::CmdMakePopup(sDInt menuptr)
{
  CmdMakeMenu((CommandMenu *) menuptr);

  sGui->AddPopupWindow(sWire->CurrentMenu);
  sWire->CurrentMenu = 0;
}

void sWireMasterWindow::CmdMakePulldown(sDInt menuptr)
{
  CmdMakeMenu((CommandMenu *) menuptr);

  sGui->AddPulldownWindow(sWire->CurrentMenu);
  sWire->CurrentMenu = 0;
}

void sWireMasterWindow::CmdClosePopup()
{
  PopupFrame->Close();
  PopupFrame->Switch(-1);
}

void sWireMasterWindow::FormChangeTool(sWireNamespace::Form *wc,sWireNamespace::CommandTool *newtool)
{
  if(newtool!=wc->CurrentTool)
  {
    if(wc->CurrentTool)
    {
      wc->CurrentTool->Message.Code = 0;
      wc->CurrentTool->Message.Send();
      CurrentToolName = L"";
    }
    wc->CurrentTool = newtool;
    if(wc->CurrentTool)
    {
      wc->CurrentTool->Message.Code = 1;
      wc->CurrentTool->Message.Send();
      CurrentToolName = wc->CurrentTool->Text;
    }
    ToolChangedMsg.Post();
  }
}

/****************************************************************************/

sBool sWireMasterWindow::CanBeGlobal(sU32 m) 
{
  return (m & QUAL_GLOBAL) || (m & QUAL_SCOPEMASK); 
}

sBool sWireMasterWindow::IsGlobal(sU32 m) 
{
  return (m & QUAL_GLOBAL) || (m & QualScope); 
}

void sWireMasterWindow::SwitchScope(sInt scope) 
{
  QualScope = QUAL_SCOPE0<<scope; 
}


sBool sWireMasterWindow::OnShortcut(sU32 key)
{
  Shortcut *sc;
  if(key&sKEYQ_SHIFT) key |= sKEYQ_SHIFT;
  if(key&sKEYQ_CTRL) key |= sKEYQ_CTRL;

  sFORALL(GlobalKeys,sc)
  {
    if(key == sc->Key && IsGlobal(sc->Qual))
    {
      sc->Cmd->Message.Send();
      return 1;
    }
  }
  return 0;
}

sBool sWireMasterWindow::HandleKey(sWindow *win,sU32 key)
{
  Form *wc;
  FormInstance *inst;
  Shortcut *sc;
  sU32 mkey;

  key &= ~sKEYQ_CAPS;
  if(key&sKEYQ_SHIFT) key |= sKEYQ_SHIFT;
  if(key&sKEYQ_CTRL) key |= sKEYQ_CTRL;

  key = (key & ~sKEYQ_MASK) | sMakeUnshiftedKey(key & sKEYQ_MASK);
  mkey = key & (sKEYQ_MASK|sKEYQ_BREAK);

  inst = FindInstance(win);
  sVERIFY(inst);
  wc = inst->Class;


  sFORALL(GlobalKeys,sc)
  {
    if(key == sc->Key && IsGlobal(sc->Qual))
    {
      sc->Cmd->Message.Send();
      return 1;
    }
  }
/*
  if(key & sKEYQ_BREAK)
  {
    if(wc->CurrentTool)
    {
      wc->CurrentTool->Message.Code = 0;
      wc->CurrentTool->Message.Send();
      wc->CurrentTool = 0;
      CurrentToolName = L"";
      ToolChangedMsg.Post();
    }
  }
*/
  sFORALL(wc->Shortcuts,sc)
  {
    if((sc->Sets & win->WireSets) && (sc->Cmd->Type==CT_TOOL || sc->Tool==wc->CurrentTool))
    {
      sU32 qkey = key & (sKEYQ_REPEAT|sKEYQ_BREAK|sKEYQ_SHIFT|sKEYQ_CTRL|sKEYQ_ALT|sKEYQ_ALTGR|sKEYQ_MASK);
      if(sc->Qual & QUAL_REPEAT)
        qkey &= ~sKEYQ_REPEAT;
      if(sc->Qual & QUAL_ONOFF)
        qkey &= ~sKEYQ_BREAK;
      if(sc->Qual & QUAL_ALLSHIFT)
      {
        qkey &= ~(sKEYQ_SHIFT|sKEYQ_CTRL|sKEYQ_ALT|sKEYQ_ALTGR);
        if((qkey&sKEYQ_MASK)>='A' && (qkey&sKEYQ_MASK)<='Z')
          qkey = qkey-'A'+'a';
      }

      if(qkey == sc->Key)
      {
        if(sc->Cmd->Type==CT_TOOL)          // special handling for tools
        {
          CommandTool *newtool = (CommandTool *) sc->Cmd;

          if(sc->Qual & QUAL_ONOFF)        // keydown: on, keyup: off
          {
            if(key & sKEYQ_BREAK)
              newtool = 0;
          }
          else                              // each press toggles
          {
            if(newtool==wc->CurrentTool)
              newtool = 0;
          }

          FormChangeTool(wc,newtool);
        }
        else
        {
          if(sc->Qual & QUAL_ONOFF)
            sc->Cmd->Message.Code = (key&sKEYQ_BREAK) ? 0 : 1;
          sc->Cmd->Message.Send();
        }
        return 1;
      }
    }
  }

  if(mkey==sKEY_WHEELUP || mkey==sKEY_WHEELDOWN)
  {
    sWindow *ws = win;
    while(ws)
    {
      if(ws->Flags & sWF_SCROLLY)
      {
        if(mkey==sKEY_WHEELUP)
          ws->ScrollTo(ws->ScrollX,ws->ScrollY-14*3);
        else
          ws->ScrollTo(ws->ScrollX,ws->ScrollY+14*3);
        return 1;
      }
      ws = ws->Parent;
    }
  }

  return 0;
}

sBool sWireMasterWindow::HandleCommand(sWindow *win,sInt cmd)
{
  FormInstance *inst = 0;
  switch(cmd)
  {
  case sCMD_LEAVEFOCUS:
    inst = FindInstance(win);
    if(inst && inst->Class)
    {
      if(inst->Class->CurrentTool)
        FormChangeTool(inst->Class,0);
    }
    break;
  default:
    return 0;
  }
  return 1;
}


sBool sWireMasterWindow::HandleDrag(sWindow *win,const sWindowDrag &dd,sInt hit)
{
  Form *wc;
  FormInstance *inst;
  CommandDrag *drag;
  Shortcut *sc;
  sBool ok;
  sU32 key;
  sU32 keyq;

  if(dd.Mode==sDD_START)
  {
    sVERIFY(DragMessage.IsEmpty());
    key = 0;
    if(dd.Buttons& 1) key = KEY_LMB;
    if(dd.Buttons& 2) key = KEY_RMB;
    if(dd.Buttons& 4) key = KEY_MMB;
    if(dd.Buttons& 8) key = KEY_X4;
    if(dd.Buttons&16) key = KEY_X5;

    keyq = sGetKeyQualifier();
    if(keyq & sKEYQ_CTRL)  key |= sKEYQ_CTRL;
    if(keyq & sKEYQ_SHIFT) key |= sKEYQ_SHIFT;
    if(keyq & sKEYQ_ALT)   key |= sKEYQ_ALT;
    
    inst = FindInstance(win);
    sVERIFY(inst);
    wc = inst->Class;
//    CurrentToolName = wc->CurrentTool ? wc->CurrentTool->Text : L""

    sFORALL(wc->Shortcuts,sc)
    {
      if(sc->Sets & win->WireSets)
      {
        ok = 1;
        if(sc->Qual & QUAL_ALLSHIFT)
        {
          if(sc->Key != (key & ~(sKEYQ_SHIFT|sKEYQ_CTRL|sKEYQ_ALT|sKEYQ_ALTGR))) ok=0;
        }
        else
        {
          if(sc->Key != key) ok = 0;
        }
        if(sc->Tool != wc->CurrentTool) ok = 0;
        if((sc->Qual & QUAL_HIT) && hit!=sc->Hit) ok = 0;
        if((sc->Qual & QUAL_MISS) && hit) ok = 0;
        if((sc->Qual & QUAL_DOUBLE) && !(dd.Flags&sDDF_DOUBLECLICK)) ok = 0;

        if(ok)
        {
          switch(sc->Cmd->Type)
          {
          case CT_DRAG:
            sVERIFY(sc->Instance==0);
            drag = (CommandDrag *) sc->Cmd;
            DragMessage = drag->Message;
            DragModeQual = sc->Qual;
            DragModeFocus = win;
            break;
          case CT_MENU:
          case CT_COMMAND:
            sc->Cmd->Message.Post();
            break;
          }
        }
      }
    }
  }


  sInt result = DragMessage.IsValid();

  if(dd.Mode!=sDD_HOVER)          // hover messages have to be handled directly...
  {
    if(DragMessage.IsValid())
    {
      sVERIFY(DragModeFocus==win);
      DragMessage.Drag(dd);

      if(dd.Mode==sDD_STOP)
      {
        sVERIFY(DragModeFocus==win);
        DragModeFocus = 0;
        DragMessage.Clear();
      }
      if((DragModeQual & QUAL_FREEZE) && dd.Mode==sDD_DRAG)
      {
        sSetMouse(dd.StartX,dd.StartY);
      }
    }
  }

  if(dd.Mode==sDD_HOVER)          // now handle hover messages, if sWF_HOVER is enabled
  {
    inst = FindInstance(win);
    sVERIFY(inst);
    wc = inst->Class;
    sFORALL(wc->Shortcuts,sc)
    {
      if((sc->Sets & win->WireSets) && 
          sc->Cmd->Type==CT_DRAG && 
          sc->Key==KEY_HOVER &&
          sc->Tool == wc->CurrentTool)
      {
        sVERIFY(sc->Instance==0);
        drag = (CommandDrag *) sc->Cmd;
        drag->Message.Drag(dd);
      }
    }
  }

  return result;
}

void sWireMasterWindow::ChangeCurrentTool(sWindow *window,const sChar *name)
{
  FormInstance *inst = FindInstance(window);
  sVERIFY(inst);

  Form *wc = inst->Class;
  CommandTool *newtool = 0;

  if(name)
  {
    CommandBase *cmd = sFind(wc->Commands,&CommandBase::Symbol,name);
    if(!cmd)
    {
      sLogF(L"wire",L"trying to activate tool '%s' but no such thing exists!",name);
      return;
    }

    if(cmd->Type != CT_TOOL)
    {
      sLogF(L"wire",L"trying to activate tool '%s' but that symbol is a %s, not a tool!\n",CTString[cmd->Type]);
      return;
    }

    newtool = (CommandTool*) cmd;
  }

  FormChangeTool(wc,newtool);
}

void sWireMasterWindow::Help(sWindow *window)
{
  sMenuFrame *mf;
  Shortcut *sc,*tool;
  sInt hastools = 0;
  sInt hasaction = 0;
  sInt hasdrag = 0;
  sInt hascursor = 0;
  sInt hasextra[3] = {0,0,0};
//  sInt other = 0;
  sInt sets = -1;
  FormInstance *fi;
  Form *form;
  sArray<Shortcut *> tools;

  // find form

  fi = FindInstance(window);
  if(!fi) return;
  form = fi->Class;
  if(!form) return;
  sets = window->WireSets;

  // scan required columns

  sFORALL(form->Shortcuts,sc)
  {
    if(sc->Sets & sets)
    {
      switch(sc->Cmd->Type)
      {
      case CT_COMMAND:
        if((sc->Key&sKEYQ_MASK)>=KEY_LMB && (sc->Key&sKEYQ_MASK)<=KEY_X5)
          hasdrag = 1;
        else if(sc->Qual & QUAL_CURSOR)
          hascursor = 1;
        else if(sc->Qual & QUAL_EXTRA)
          hasextra[0] = 1;
        else if(sc->Qual & QUAL_EXTRA2)
          hasextra[1] = 1;
        else if(sc->Qual & QUAL_EXTRA3)
          hasextra[2] = 1;
        else
          hasaction = 1;
        break;
      case CT_TOOL:
        hastools++;
        break;
      case CT_DRAG:
        hasdrag++;
      }
    }
  }

  mf = new sMenuFrame();
  mf->AddBorder(new sThickBorder);
  if(hasaction)   mf->AddHeader(L"Actions",0);
  if(hascursor)   mf->AddHeader(L"Cursor",4);
  if(hasextra[0]) mf->AddHeader(L"Extra",8);
  if(hasextra[1]) mf->AddHeader(L"Extra(2)",12);
  if(hasextra[2]) mf->AddHeader(L"Extra(3)",16);
  if(hasdrag)     mf->AddHeader(L"Mouse",20);
  if(hastools)    mf->AddHeader(L"Tools",24);

  // add items

  sFORALL(form->Shortcuts,sc)
  {
    if(sc->Sets & sets)
    {
      sU32 qual = 0;
      if(sc->Qual & QUAL_HIT)    qual |= 0x01000000;
      if(sc->Qual & QUAL_MISS)   qual |= 0x02000000;
      if(sc->Qual & QUAL_DOUBLE) qual |= 0x04000000;

      sInt col = 0;
      if((sc->Key&sKEYQ_MASK)>=KEY_LMB && (sc->Key&sKEYQ_MASK)<=KEY_X5)
        col = 20;
      else if(sc->Qual & QUAL_CURSOR)
        col = 4;
      else if(sc->Qual & QUAL_EXTRA)
        col = 8;
      else if(sc->Qual & QUAL_EXTRA2)
        col = 12;
      else if(sc->Qual & QUAL_EXTRA3)
        col = 16;
      else
        col = 0;


      switch(sc->Cmd->Type)
      {
      case CT_COMMAND:
        if(sc->Tool == 0)
          mf->AddItem(sc->Cmd->Text,sc->Cmd->Message,sc->Key|qual,-1,col);
        break;
      case CT_DRAG:
        if(sc->Tool == 0)
          mf->AddItem(sc->Cmd->Text,sMessage(),sc->Key|qual,-1,20);
        break;
      case CT_TOOL:
        tools.AddTail(sc);
        break;
      case CT_CHOICE:
        {
          CommandChoice *cc = (CommandChoice *) sc->Cmd;
          if(sCountChoice(cc->Choices)==2)
            mf->AddCheckmark(sc->Cmd->Text,cc->Message,sc->Key|qual,cc->Ptr,-1,-1,col);
        }
        break;
      }
    }
  }

  // add tools in order

  sFORALL(tools,tool)
  {
    mf->AddItem(tool->Cmd->Text,tool->Cmd->Message,tool->Key,-1,24,sGetColor2D(sGC_BUTTON));
    sFORALL(form->Shortcuts,sc)
    {
      if(sc->Tool==(CommandTool *) tool->Cmd && (sc->Sets&sets))
      {
        sU32 qual = 0;
        if(sc->Qual & QUAL_HIT)    qual |= 0x01000000;
        if(sc->Qual & QUAL_MISS)   qual |= 0x02000000;
        if(sc->Qual & QUAL_DOUBLE) qual |= 0x04000000;
        switch(sc->Cmd->Type)
        {
        case CT_COMMAND:
          if((sc->Key&sKEYQ_MASK)>=KEY_LMB && (sc->Key&sKEYQ_MASK)<=KEY_X5)
            mf->AddItem(sc->Cmd->Text,sc->Cmd->Message,sc->Key|qual,-1,24);
          else if(sc->Qual & QUAL_CURSOR)
            mf->AddItem(sc->Cmd->Text,sc->Cmd->Message,sc->Key|qual,-1,24);
          else
            mf->AddItem(sc->Cmd->Text,sc->Cmd->Message,sc->Key|qual,-1,24);
          break;
        case CT_DRAG:
          mf->AddItem(sc->Cmd->Text,sMessage(),sc->Key|qual,-1,24);
          break;
        }
      }
    }
  }

  // ...

  sGui->AddPopupWindow(mf);
}

/****************************************************************************/
/****************************************************************************/

FormInstance *sWireMasterWindow::_FormName(sBool window)
{
  Form *form=0;
  FormInstance *inst=0;
  sPoolString fname,iname;
  sInt index = -1;

  Scan->ScanName(fname);
  form = sFind(Classes,&Form::Symbol,fname);

  if(!form)
  {
    Scan->Error(L"form/window <%s> not found",fname);
    return 0;
  }
  if(form->Instances.GetCount()==1)
    inst = &form->Instances[0];
  if(window && !form->IsWindow)
    Scan->Error(L"<%s> is a form, not a window");

  if(Scan->Token=='[')
  {
    Scan->Match('[');
    if(Scan->Token==sTOK_INT)
    {
      index = Scan->ScanInt();
      if(index<0 || index>=form->Instances.GetCount())
        Scan->Error(L"index out of range");
      else
        inst = &form->Instances[index];
    }
    else
    {
      Scan->ScanName(iname);
      inst = sFind(form->Instances,&FormInstance::Symbol,iname);
    }
    Scan->Match(']');
  }
  if(inst==0)
    Scan->Error(L"instance not unique. specify with []");

  if(Scan->Errors)
    return 0;
  return inst;
}

void sWireMasterWindow::_Key(sU32 &key,sInt &qual,sInt &hit)
{
  key = 0;
  qual = 0;
  hit = 0;
  if(Scan->Token==sTOK_CHAR)
  {
    key = sMakeUnshiftedKey(Scan->ScanChar());
  }
  else if(Scan->Token==sTOK_NAME)
  {
         if(Scan->Name == L"UP")          key = sKEY_UP;
    else if(Scan->Name == L"DOWN")        key = sKEY_DOWN;
    else if(Scan->Name == L"LEFT")        key = sKEY_LEFT;
    else if(Scan->Name == L"RIGHT")       key = sKEY_RIGHT;
    else if(Scan->Name == L"HOME")        key = sKEY_HOME;
    else if(Scan->Name == L"END")         key = sKEY_END;
    else if(Scan->Name == L"PAGEUP")      key = sKEY_PAGEUP;
    else if(Scan->Name == L"PAGEDOWN")    key = sKEY_PAGEDOWN;
    else if(Scan->Name == L"INSERT")      key = sKEY_INSERT;
    else if(Scan->Name == L"DELETE")      key = sKEY_DELETE;
    else if(Scan->Name == L"BACKSPACE")   key = sKEY_BACKSPACE;
    else if(Scan->Name == L"TAB")         key = sKEY_TAB;
    else if(Scan->Name == L"ENTER")       key = sKEY_ENTER;
    else if(Scan->Name == L"ESCAPE")      key = sKEY_ESCAPE;
    else if(Scan->Name == L"LMB")         key = KEY_LMB;
    else if(Scan->Name == L"RMB")         key = KEY_RMB;
    else if(Scan->Name == L"MMB")         key = KEY_MMB;
    else if(Scan->Name == L"X4")          key = KEY_X4;
    else if(Scan->Name == L"X5")          key = KEY_X5;
    else if(Scan->Name == L"PAUSE")       key = sKEY_PAUSE;
    else if(Scan->Name == L"MENU")        key = sKEY_WINM;
    else if(Scan->Name == L"DROPFILES") { key = sKEY_DROPFILES; sEnableFileDrop(sTRUE); }
    else if(Scan->Name == L"WHEELUP")     key = sKEY_WHEELUP;
    else if(Scan->Name == L"WHEELDOWN")   key = sKEY_WHEELDOWN;
    else if(Scan->Name == L"NULL")        key = KEY_NULL;
    else if(Scan->Name == L"HOVER")       key = KEY_HOVER;

    else if(Scan->Name == L"F1") key = sKEY_F1;
    else if(Scan->Name == L"F2") key = sKEY_F2;
    else if(Scan->Name == L"F3") key = sKEY_F3;
    else if(Scan->Name == L"F4") key = sKEY_F4;
    else if(Scan->Name == L"F5") key = sKEY_F5;
    else if(Scan->Name == L"F6") key = sKEY_F6;
    else if(Scan->Name == L"F7") key = sKEY_F7;
    else if(Scan->Name == L"F8") key = sKEY_F8;
    else if(Scan->Name == L"F9") key = sKEY_F9;
    else if(Scan->Name == L"F10") key = sKEY_F10;
    else if(Scan->Name == L"F11") key = sKEY_F11;
    else if(Scan->Name == L"F12") key = sKEY_F12;
    if(key==0)    
      Scan->Error(L"key expected");
    else
      Scan->Scan();
    if(key==1)
      key = 0;
  }
  else 
  {
    Scan->Error(L"key expected");
  }

  while(Scan->Token=='|' && Scan->Errors==0)
  {
    Scan->Match('|');
    if(Scan->Token==sTOK_NAME && Scan->Name==L"SHIFT")
    {
      key |= sKEYQ_SHIFT;
      Scan->Scan();
    }
    else if(Scan->Token==sTOK_NAME && Scan->Name==L"CTRL")
    {
      key |= sKEYQ_CTRL;
      Scan->Scan();
    }
    else if(Scan->Token==sTOK_NAME && Scan->Name==L"ALT")
    {
      key |= sKEYQ_ALT;
      Scan->Scan();
    }
    else if(Scan->Token==sTOK_NAME && Scan->Name==L"HIT")
    {
      qual |= QUAL_HIT;
      hit = 1;
      if(Scan->Token=='(')
      {
        Scan->Match('(');
        hit = Scan->ScanInt();
        Scan->Match(')');
      }
      Scan->Scan();
    }
    else if(Scan->Token==sTOK_NAME && Scan->Name==L"MISS")
    {
      qual |= QUAL_MISS;
      Scan->Scan();
    }
    else if(Scan->Token==sTOK_NAME && Scan->Name==L"REPEAT")
    {
      qual |= QUAL_REPEAT;
      Scan->Scan();
    }
    else if(Scan->Token==sTOK_NAME && Scan->Name==L"ONOFF")
    {
      qual |= QUAL_ONOFF;
      Scan->Scan();
    }
    else if(Scan->Token==sTOK_NAME && Scan->Name==L"ALLSHIFT")
    {
      qual |= QUAL_ALLSHIFT;
      Scan->Scan();
    }
    else if(Scan->Token==sTOK_NAME && Scan->Name==L"DOUBLE")
    {
      qual |= QUAL_DOUBLE;
      Scan->Scan();
    }
    else if(Scan->Token==sTOK_NAME && Scan->Name==L"DRAG")
    {
      qual |= QUAL_DRAG;
      Scan->Scan();
    }
    else if(Scan->Token==sTOK_NAME && Scan->Name==L"FREEZE")
    {
      qual |= QUAL_FREEZE;
      Scan->Scan();
    }
    else if(Scan->Token==sTOK_NAME && Scan->Name==L"GLOBAL")
    {
      qual |= QUAL_GLOBAL;
      Scan->Scan();
    }
    else if(Scan->Token==sTOK_NAME && Scan->Name==L"BREAK")
    {
      key |= sKEYQ_BREAK;
      Scan->Scan();
    }
    else if(Scan->Token==sTOK_NAME && Scan->Name==L"CURSOR")
    {
      qual |= QUAL_CURSOR;
      Scan->Scan();
    }
    else if(Scan->Token==sTOK_NAME && (Scan->Name==L"EXTRA" || Scan->Name==L"EXTRA1"))
    {
      qual |= QUAL_EXTRA;
      Scan->Scan();
    }
    else if(Scan->Token==sTOK_NAME && Scan->Name==L"EXTRA2")
    {
      qual |= QUAL_EXTRA2;
      Scan->Scan();
    }
    else if(Scan->Token==sTOK_NAME && Scan->Name==L"EXTRA3")
    {
      qual |= QUAL_EXTRA3;
      Scan->Scan();
    }
    else if(Scan->Token==sTOK_NAME && Scan->Name==L"SCOPE0")
    {
      qual |= QUAL_SCOPE0;
      Scan->Scan();
    }
    else if(Scan->Token==sTOK_NAME && Scan->Name==L"SCOPE1")
    {
      qual |= QUAL_SCOPE1;
      Scan->Scan();
    }
    else if(Scan->Token==sTOK_NAME && Scan->Name==L"SCOPE2")
    {
      qual |= QUAL_SCOPE2;
      Scan->Scan();
    }
    else if(Scan->Token==sTOK_NAME && Scan->Name==L"SCOPE3")
    {
      qual |= QUAL_SCOPE3;
      Scan->Scan();
    }
    else if(Scan->Token==sTOK_NAME && Scan->Name==L"SCOPE4")
    {
      qual |= QUAL_SCOPE4;
      Scan->Scan();
    }
    else if(Scan->Token==sTOK_NAME && Scan->Name==L"SCOPE5")
    {
      qual |= QUAL_SCOPE5;
      Scan->Scan();
    }
    else if(Scan->Token==sTOK_NAME && Scan->Name==L"SCOPE6")
    {
      qual |= QUAL_SCOPE6;
      Scan->Scan();
    }
    else if(Scan->Token==sTOK_NAME && Scan->Name==L"SCOPE7")
    {
      qual |= QUAL_SCOPE7;
      Scan->Scan();
    }
    else
    {
      Scan->Error(L"unknown key qualifier");
    }
  }
}

void sWireMasterWindow::_ScreenAlign(sLayoutFrameWindow *lfw)
{
  if(Scan->Token=='-')
  {
    Scan->Match('-');
    lfw->Pos = -Scan->ScanInt();
  }
  else if(Scan->Token==sTOK_INT)
  {
    lfw->Pos = Scan->ScanInt();
  }
  if(Scan->Token==sTOK_NAME && (Scan->Name==L"bottom" || Scan->Name==L"right"))
  {
    Scan->Match(sTOK_NAME);
    lfw->Align = 1;
  }
  else if(Scan->Token==sTOK_NAME && (Scan->Name==L"top" || Scan->Name==L"left"))
  {
    Scan->Match(sTOK_NAME);
  }
}

void sWireMasterWindow::_ScreenLevel(sLayoutFrameWindow *parent)
{
  sLayoutFrameWindow *lfw = 0;
//  sLayoutFrameWindow *child;
  sBool childs = 0;  // 0:can not 1:may 2:must ... have childs

  if(Scan->IfName(L"horizontal"))
  {
    sWindow *w = new sHSplitFrame;
    lfw = new sLayoutFrameWindow(sLFWM_HORIZONTAL,w,0);
    if(Scan->IfName(L"prop"))
      lfw->Proportional = 1;
    Windows.AddTail(w);
    _ScreenAlign(lfw);
    childs = 2;
  }
  else if(Scan->IfName(L"vertical"))
  {
    sWindow *w = new sVSplitFrame;
    lfw = new sLayoutFrameWindow(sLFWM_VERTICAL,w,0);
    if(Scan->IfName(L"prop"))
      lfw->Proportional = 1;
    Windows.AddTail(w);
    _ScreenAlign(lfw);
    childs = 2;
  }
  else if(Scan->IfName(L"switch"))
  {
    sWindow *w = new sVSplitFrame;
    lfw = new sLayoutFrameWindow(sLFWM_SWITCH,w,0);
    Scan->ScanName(lfw->Name);
    Windows.AddTail(w);
    _ScreenAlign(lfw);
    childs = 2;
  }
  else if(Scan->IfName(L"window"))
  {
    FormInstance *inst = _FormName(1);
    if(inst && inst->Object)
    {
      lfw = new sLayoutFrameWindow(sLFWM_WINDOW,(sWindow *)inst->Object,0);
      inst->Class->IsInScreen = 1;
    }
    _ScreenAlign(lfw);
    childs = 1;
  }
  else if(Scan->IfName(L"dummy"))
  {
    sWireDummyWindow *w = new sWireDummyWindow;
    if(Scan->Token==sTOK_STRING)
    {
      sPoolString name;
      Scan->ScanString(name);
      w->Label = name;
      sDPrintF(L"%s(%d): use NAME not STRING for dummies!");
    }
    if(Scan->Token==sTOK_NAME)
    {
      sPoolString name;
      Scan->ScanName(name);
      w->Label = name;
    }

    AddWindow(L"wire_internal_dummy",w);
    Windows.AddTail(w);
    lfw = new sLayoutFrameWindow(sLFWM_WINDOW,w,0);
    _ScreenAlign(lfw);
    childs = 1;
  }
  else if(Scan->IfName(L"border"))
  {
    FormInstance *inst = _FormName(1);
    if(inst && inst->Object)
    {
      lfw = new sLayoutFrameWindow(sLFWM_BORDERPRE,(sWindow *)inst->Object,0);
      inst->Class->IsInScreen = 1;
    }
    childs = 1;
  }
  else if(Scan->IfName(L"focusborder"))
  {
    sWindow *w = new sFocusBorder;
    Windows.AddTail(w);
    lfw = new sLayoutFrameWindow(sLFWM_BORDERPRE,w,0);
    childs = 0;
  }
  else if(Scan->IfName(L"toolborder"))
  {
    sWindow *w = new sToolBorder;
    Windows.AddTail(w);
    lfw = new sLayoutFrameWindow(sLFWM_BORDERPRE,w,0);
    childs = 1;
  }
  else if(Scan->IfName(L"label"))
  {
    sPoolString name; Scan->ScanString(name);
    sWindow *w = new sButtonControl(name,sMessage(),sBCS_NOBORDER|sBCS_STATIC);
    Windows.AddTail(w);
    lfw = new sLayoutFrameWindow(sLFWM_WINDOW,w,0);
    childs = 0;
  }
  else if(Scan->IfName(L"menu"))
  {
    CommandMenu *menu = _Menu(0);

    sButtonControl *w = new sButtonControl(menu->Text,sMessage(this,&sWireMasterWindow::CmdMakePulldown,sDInt(menu)),sBCS_NOBORDER|sBCS_LABEL);
    Windows.AddTail(w);
    lfw = new sLayoutFrameWindow(sLFWM_WINDOW,w,0);
    childs = 0;
  }
  else if(Scan->IfName(L"spacer"))
  {
//    /*CommandMenu *menu =*/ _Menu(0);

    sButtonControl *w = new sButtonControl(L"  ",sMessage(),sBCS_NOBORDER|sBCS_LABEL|sBCS_STATIC);
    Windows.AddTail(w);
    lfw = new sLayoutFrameWindow(sLFWM_WINDOW,w,0);
    childs = 0;
  }
  else if(Scan->IfName(L"button"))
  {
    CommandBase *cmd = this->_Command(0);
    if(cmd && cmd->Type==CT_COMMAND)
    {
      sPoolString name = cmd->Symbol;
      if(cmd->Text[0]) name=cmd->Text;
      if(Scan->Token==sTOK_STRING)
        Scan->ScanString(name);
      sButtonControl *w = new sButtonControl(name,cmd->Message);
      Windows.AddTail(w);
      lfw = new sLayoutFrameWindow(sLFWM_WINDOW,w,0);
      childs = 0;
    }
    else if(cmd && cmd->Type==CT_CHOICE)
    {
      CommandChoice *cmdch = (CommandChoice *) cmd;
      sPoolString name = cmd->Symbol;
      if(cmd->Text[0]) name=cmd->Text;
      if(Scan->Token==sTOK_STRING)
        Scan->ScanString(name);
      sChoiceControl *w = new sChoiceControl(cmdch->Choices,cmdch->Ptr);
      w->ChangeMsg = cmd->Message;
      Windows.AddTail(w);
      lfw = new sLayoutFrameWindow(sLFWM_WINDOW,w,0);
      childs = 0;
    }
    else
    {
      Scan->Error(L"key command expected");
    }
  }
  else if(Scan->IfName(L"screenswitch"))
  {
    ScreenSwitchAdd.AddTail(parent);
    childs = 0;
  }
  else if(Scan->IfName(L"scrollx"))
  {
    sScrollBorder *w = new sScrollBorder;
    Windows.AddTail(w);
    parent->Window->Flags |= sWF_CLIENTCLIPPING|sWF_SCROLLX;
    lfw = new sLayoutFrameWindow(sLFWM_BORDER,w,0);
    childs = 0;
  }
  else if(Scan->IfName(L"scrolly"))
  {
    sScrollBorder *w = new sScrollBorder;
    Windows.AddTail(w);
    parent->Window->Flags |= sWF_CLIENTCLIPPING|sWF_SCROLLY;
    lfw = new sLayoutFrameWindow(sLFWM_BORDER,w,0);
    childs = 0;
  }
  else if(Scan->IfName(L"scroll"))
  {
    sScrollBorder *w = new sScrollBorder;
    Windows.AddTail(w);
    parent->Window->Flags |= sWF_CLIENTCLIPPING|sWF_SCROLLX|sWF_SCROLLY;
    lfw = new sLayoutFrameWindow(sLFWM_BORDER,w,0);
    childs = 0;
  }
  else
  {
    Scan->Error(L"syntax error");
    return;
  }

  if(lfw)
    parent->Add(lfw);

  if(Scan->Token==';')
  {
    Scan->Scan();
    if(childs==2)
      Scan->Error(L"window must have childs");
  }
  else if(Scan->Token=='{')
  {
    if(childs==0)
      Scan->Error(L"window may not have childs");
    Scan->Scan();
    while(Scan->Errors==0 && Scan->Token!='}')
      _ScreenLevel(lfw);
    Scan->Match('}');
  }
  else
  {
    Scan->Error(L"expected ';' or '{'");
  }
}

void sWireMasterWindow::_Popup()
{
  Screen *scr;
  sLayoutFrameWindow *parent = new sLayoutFrameWindow(sLFWM_WINDOW,0,0);

  scr = new Screen;

  Scan->ScanString(scr->Name);


  Scan->Match('{');
  _ScreenLevel(parent);
  if(Scan->Token!='}') Scan->Error(L"only one window at top level. use dummy splitter");
  Scan->Match('}');

  if(!Scan->Errors)
  {
    Popups.AddTail(scr);
    PopupFrame->Screens.AddTail(parent->Childs[0]); 
  }

  parent->Childs.Clear();
  delete parent;
}

void sWireMasterWindow::_OnSwitch(sLayoutFrameWindow *parent)
{
  sPoolString window;
  sPoolString command;
  Scan->ScanName(window);
  Scan->Match('.');
  Scan->ScanName(command);
  Scan->Match(';');
  if(Scan->Errors==0)
  {
    Form *form = FindClass(window);
    if(!form)
    {
      Scan->Error(L"form %q not found",form);
    }
    else
    {
      CommandBase *cb = sFind(form->Commands,&CommandBase::Symbol,command);
      if(!cb)
      {
        Scan->Error(L"command %q not found in form %q",command,form);
      }
      else
      {
        parent->OnSwitch = cb->Message;
      }
    }
  }
}

void sWireMasterWindow::_Screen()
{
  Screen *scr;
  sLayoutFrameWindow *parent = new sLayoutFrameWindow(sLFWM_WINDOW,0,0);

  scr = new Screen;

  Scan->ScanString(scr->Name);
  Scan->Match('{');
  if(Scan->IfName(L"onswitch"))
  {
    _OnSwitch(parent);
  }

  _ScreenLevel(parent);
  if(Scan->Token!='}') Scan->Error(L"only one window at top level. use dummy splitter");
  Scan->Match('}');

  if(!Scan->Errors)
  {
    Screens.AddTail(scr);
    parent->Childs[0]->OnSwitch = parent->OnSwitch;
    LayoutFrame->Screens.AddTail(parent->Childs[0]); 
  }

  parent->Childs.Clear();
  delete parent;
}

CommandBase *sWireMasterWindow::_Command(Form *wc)
{
  sPoolString name;
  CommandBase *cmd = 0;
  FormInstance *inst = 0;
  Form *form = wc;

  Scan->ScanName(name);
  if(Scan->Token=='[' || Scan->Token=='.')
  {
    form = sFind(Classes,&Form::Symbol,name);
    if(!form)
    {
      Scan->Error(L"could not find form <%s>",name);
      form = wc;
    }

    if(Scan->Token=='[')
    {
      Scan->Scan();

      if(Scan->Token==sTOK_NAME)
      {
        Scan->ScanName(name);
        inst = sFind(form->Instances,&FormInstance::Symbol,name);
        if(!inst) Scan->Error(L"could not find instance <%s> in form <%s>",name,form->Symbol);
      }
      else
      {
        sInt i = Scan->ScanInt();
        if(i<0 || i>=form->Instances.GetCount())
          Scan->Error(L"instance index out of bounds");
        else
          inst = &form->Instances[i];
      }
      Scan->Match(']');
    }
    Scan->Match('.');
    Scan->ScanName(name);
  }
  if(!form)
  {
    Scan->Error(L"you must specify form/window here!");
  }
  else
  {
    cmd = sFind(form->Commands,&CommandBase::Symbol,name);
  }
  if(!cmd)
  {
    Scan->Error(L"unknown command");
    return 0;
  }

  return cmd;
}

void sWireMasterWindow::_Add0(RawShortcut &sc)
{
  sPoolString name;
  sc.Form = name;
  sc.InstanceName = name;
  sc.InstanceIndex = -1;
  sc.Command = name;
  sc.Label = name;
  sc.Tool = name;
  sc.Key = 0;
  sc.KeyQual = 0;
  sc.KeyHit = 0;
  sc.Sets = -1;

  // scan name (may be form if followed by '[' or '.')

  Scan->ScanName(name);

  // scan form

  if(Scan->Token=='[' || Scan->Token=='.')
  {
    sc.Form = name;
    if(Scan->IfToken('['))
    {
      if(Scan->Token==sTOK_NAME)
      {
        Scan->ScanName(sc.InstanceName);
      }
      else
      {
        sc.InstanceIndex = Scan->ScanInt();
        if(sc.InstanceIndex<0)
          Scan->Error(L"instance index out of bounds");
      }
      Scan->Match(']');
    }
    Scan->Match('.');
    Scan->ScanName(name);
  }

  sc.Command = name;
}

void sWireMasterWindow::_Add(RawShortcut &sc)
{
  _Add0(sc);

  // get optional tool reference

  if(Scan->IfToken(':'))
    Scan->ScanName(sc.Tool);

  // get optional text string

  if(Scan->Token==sTOK_STRING)
    Scan->ScanString(sc.Label);

  // get keyboard shortcut

  if(Scan->Token!=';')
    _Key(sc.Key,sc.KeyQual,sc.KeyHit);

  // done

  Scan->Match(';');
}


Shortcut *sWireMasterWindow::MakeShortcut(Form *wc,sInt type,RawShortcut &raw)
{
//  sU32 key = 0;
//  sInt qual = 0;
//  sInt hit = 0;
  CommandBase *cmd = 0;
  FormInstance *inst = 0;
  Form *form = wc;
  CommandBase *tool = 0;

  if(!raw.Form.IsEmpty())
  {
    form = sFind(Classes,&Form::Symbol,raw.Form);
    if(!form)
    {
      Scan->Error(L"could not find form <%s>",raw.Form);
      form = wc;
    }

    if(!raw.InstanceName.IsEmpty())
    {
      inst = sFind(form->Instances,&FormInstance::Symbol,raw.InstanceName);
      if(!inst) Scan->Error(L"could not find instance <%s> in form <%s>",raw.InstanceName,form->Symbol);
    }
    if(raw.InstanceIndex>=0)
    {
      if(raw.InstanceIndex>=form->Instances.GetCount())
        Scan->Error(L"instance index out of bounds");
      else
        inst = &form->Instances[raw.InstanceIndex];
    }
  }
  if(!form)
  {
    Scan->Error(L"you must specify form/window here!");
    return 0;
  }

  // find command and check type

  cmd = sFind(form->Commands,&CommandBase::Symbol,raw.Command);
  if(!cmd && type==CT_TOOL)
  {
    cmd = new CommandTool;
    cmd->Symbol = raw.Command;
    cmd->Text = raw.Command;
    cmd->Type = CT_TOOL;
    cmd->Message = sMessage();
    form->Commands.AddTail(cmd);
  }
  if(!cmd)
  {
    Scan->Error(L"unknown command <%s> in form <%s>",raw.Command,form->ClassName);
    return 0;
  }
  if(cmd->Type!=type)
  {
    Scan->Error(L"expected %s, found %s",CTString[type],CTString[cmd->Type]);
  }

  // get optional text string

  if(!raw.Label.IsEmpty())
  {
    if(cmd->Text!=L"" && cmd->Text!=raw.Label && cmd->Text!=cmd->Symbol)
      Scan->Error(L"text defined twice differently");
    cmd->Text = raw.Label;
  }

  // get optional tool reference

  if(!raw.Tool.IsEmpty())
  {
    tool = sFind(form->Commands,&CommandBase::Symbol,raw.Tool);
    if(!tool || tool->Type!=CT_TOOL)
    {
      Scan->Error(L"could not find tool %s",raw.Tool);
      tool = 0;
    }
  }

  // done

  if(Scan->Errors==0)
  {
    Shortcut *sc = new Shortcut;
    sc->Class = form;
    sc->Instance = inst;
    sc->Cmd = cmd;
    sc->Key = raw.Key;
    sc->Qual = raw.KeyQual;
    sc->Hit = raw.KeyHit;
    sc->Tool = (CommandTool *)tool;
    sc->Sets = raw.Sets;
    return sc;
  }
  return 0;
}

void sWireMasterWindow::_Sets(sArray<Form *> &windows)
{
  sPoolString name;
  Form *wc;

  if(Scan->IfToken('('))     // use sets
  {
    sInt mask = 0;
    do
    {
      Scan->ScanName(name);
      SetInfo *info = sFind(windows[0]->SetInfos,&SetInfo::Symbol,name);
      if(info)
        mask |= info->Mask;
      else
        Scan->Error(L"unknown set <%s>",name);
    }
    while(Scan->IfToken(','));
    Scan->Match(')');
    if(Scan->IfToken('{'))
    {
      while(!Scan->IfToken('}') && !Scan->Errors)
        _WindowCmd(windows,mask);
    }
    else
      _WindowCmd(windows,mask);
  }
  else                      // define sets
  {
    sArray<SetInfo> set;
    set.HintSize(16);
    sInt mask = 1;

    do
    {
      SetInfo *info = set.AddMany(1);
      Scan->ScanName(info->Symbol);
      info->Mask = mask;
      mask = mask<<1;
    }
    while(Scan->IfToken(','));

    sFORALL(windows,wc)
      wc->SetInfos = set;

    Scan->Match(';');
  }
}

void sWireMasterWindow::_WindowCmd(sArray<Form *> &windows,sInt sets)
{
  RawShortcut raw;
  Shortcut *sc;
  Form *wc;

  if(Scan->IfName(L"key"))
  {
    _Add(raw);
    raw.Sets = sets;
    sFORALL(windows,wc)
    {
      sc = MakeShortcut(wc,CT_COMMAND,raw);
      if(sc)
      {
        wc->Shortcuts.AddTail(sc);
        if(CanBeGlobal(raw.KeyQual) && windows.GetCount()==1)
          GlobalKeys.AddTail(sc);
      }
    }
  }
  else if(Scan->IfName(L"check"))
  {
    _Add(raw);
    raw.Sets = sets;
    sFORALL(windows,wc)
    {
      sc = MakeShortcut(wc,CT_CHOICE,raw);
      if(sc)
      {
        wc->Shortcuts.AddTail(sc);
        if(CanBeGlobal(raw.KeyQual) && windows.GetCount()==1)
          GlobalKeys.AddTail(sc);
      }
    }
  }
  else if(Scan->IfName(L"drag"))
  {
    _Add(raw);
    raw.Sets = sets;
    sFORALL(windows,wc)
    {
      sc = MakeShortcut(wc,CT_DRAG,raw);
      if(sc)
        wc->Shortcuts.AddTail(sc);
    }
  }
  else if(Scan->IfName(L"sets"))
  {
    if(sets!=-1)
      Scan->Error(L"no nested sets");
    _Sets(windows);
  }
  else if(Scan->IfName(L"tool"))
  {
    _Add(raw);
    raw.Sets = sets;
    sFORALL(windows,wc)
    {
      sc = MakeShortcut(wc,CT_TOOL,raw);
      if(sc)
        wc->Shortcuts.AddTail(sc);
    }
  }
  else if(Scan->IfName(L"menu"))
  {
    CommandMenu *menu = _Menu(windows[0]);
    sU32 key;
    sInt qual;
    sInt hit;

    _Key(key,qual,hit);
    Scan->Match(';');

    if(Scan->Errors==0)
    {
      sFORALL(windows,wc)
      {
        Shortcut *sc = new Shortcut;
        sc->Class = wc;
        sc->Cmd = menu;
        sc->Key = key;
        sc->Qual = qual;
        sc->Hit = hit;
        wc->Shortcuts.AddTail(sc);
      }
    }
  }
  else
  {
    Scan->Error(L"unknown directive in window");
  }
}

void sWireMasterWindow::_Window()
{
  sPoolString name;
  sPoolString text;
  Form *wc;
  sArray<Form *> windows;

  // list of windows, comma seperated

  Scan->ScanName(name);
  wc = FindClass(name);
  if(wc==0) Scan->Error(L"class not found");
  if(wc->IsInScreen) Scan->Error(L"first define windows, then use in screens!");
  windows.AddTail(wc);
  while(Scan->IfToken(','))
  {
    Scan->ScanName(name);
    wc = FindClass(name);
    if(wc==0) Scan->Error(L"class not found");
    if(wc->IsInScreen) Scan->Error(L"first define windows, then use in screens!");
    windows.AddTail(wc);
  }

  // name a window

  if(Scan->Token==sTOK_STRING)
  {
    if(windows.GetCount()>=2) Scan->Error(L"can't name multiple windows");
    Scan->ScanString(text);
    if(wc->Text!=L"") Scan->Error(L"window defined twice");
    wc->Text = text;
  }

  // define stuff

  Scan->Match('{');
  while(Scan->Token!='}' && Scan->Errors==0)
  {
    _WindowCmd(windows,-1);
  }
  Scan->Match('}');
}

CommandMenu *sWireMasterWindow::_Menu(Form *form)
{
  CommandMenu *menu;
  sPoolString symbol,text;

  if(Scan->Token==sTOK_NAME)
  {
    Scan->ScanName(symbol);

    text = symbol;
  }
  if(Scan->Token==sTOK_STRING)
  {
    Scan->ScanString(text);
  }

  menu = (CommandMenu *)sFind(Menus,&CommandMenu::Symbol,symbol);

  if(Scan->Token!='{')
  {
    if(menu)
    {
      return menu;
    }
    else
    {
      Scan->Error(L"menu <%s> not found",symbol);
      return 0;
    }
  }

  if(!menu)
  {
    menu = new CommandMenu;
    menu->Symbol = symbol;
    menu->Text = text;
    menu->Type = CT_MENU;
    menu->Message = sMessage(this,&sWireMasterWindow::CmdMakePopup,sDInt(menu));
    Menus.AddTail(menu);
  }

  Scan->Match('{');
  while(Scan->Token!='}' && Scan->Errors==0)
  {
    if(Scan->IfName(L"item"))
    {
      RawShortcut raw;
      _Add(raw);
      Shortcut *sc = MakeShortcut(form,CT_COMMAND,raw);
      if(sc)
      {
        menu->Items.AddTail(sc);
        if(CanBeGlobal(raw.KeyQual))
          GlobalKeys.AddTail(sc);
      }
    }
    else if(Scan->IfName(L"check"))
    {
      RawShortcut raw;
      _Add(raw);
      Shortcut *sc = MakeShortcut(form,CT_CHOICE,raw);
      if(sc)
      {
        menu->Items.AddTail(sc);
        if(CanBeGlobal(raw.KeyQual))
          GlobalKeys.AddTail(sc);
      }
    }
    else if(Scan->IfName(L"menu"))
    {
      CommandMenu *m = _Menu(form);
      if(m)
      {
        Shortcut *sc = new Shortcut;
        sc->Cmd = m;
        menu->Items.AddTail(sc);
      }
    }
    else if(Scan->IfName(L"context"))
    {
      Form *context = form;
      if(Scan->Token=='.')
      {
        Scan->Scan();
        FormInstance *inst = _FormName(0);
        if(inst) context = inst->Class;
      }
      if(context==0) 
        Scan->Error(L"need context in \"context\" statement. try \"context.windowname;\"");
      Scan->Match(';');
      CommandContext *cmd = new CommandContext;
      Shortcut *sc = new Shortcut;
      sc->Cmd = cmd;
      sc->Class = context;

      cmd->Symbol = L"";
      cmd->Text = L"";
      cmd->Type = CT_CONTEXT;
      cmd->Context = context;

      menu->Items.AddTail(sc);
      Garbage.AddTail(cmd);
    }
    else if(Scan->IfName(L"spacer"))
    {
      Scan->Match(';');
      CommandSpacer *cmd = new CommandSpacer;
      Shortcut *sc = new Shortcut;
      sc->Cmd = cmd;

      cmd->Symbol = L"";
      cmd->Text = L"";
      cmd->Type = CT_SPACER;

      menu->Items.AddTail(sc);
      Garbage.AddTail(cmd);
    }
    else
    {
      Scan->Error(L"syntax error");
    }
  }
  Scan->Match('}');

  return menu;
}

void sWireMasterWindow::_Global()
{
  while(Scan->Token!=sTOK_ERROR && Scan->Token!=sTOK_END && Scan->Errors==0)
  {
    if(Scan->IfName(L"screen"))
    {
      _Screen();
    }
    else if(Scan->IfName(L"popup"))
    {
      _Popup();
    }
    else if(Scan->IfName(L"window"))
    {
      _Window();
    }
    else if(Scan->IfName(L"menu"))
    {
      /*CommandMenu *menu =*/ _Menu(0);
    }
    else
    {
      Scan->Error(L"unknown top level directive");
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   WireClient                                                         ***/
/***                                                                      ***/
/****************************************************************************/

sWireClientWindow::sWireClientWindow()
{
}

sWireClientWindow::~sWireClientWindow()
{
}

void sWireClientWindow::DragScroll(const sWindowDrag &dd,sDInt)
{
  MMBScroll(dd);
}

void sWireClientWindow::InitWire(const sChar *name)
{
  sWire->AddWindow(name,this);
  sWire->AddDrag(name,L"Scroll",sMessage(this,&sWireClientWindow::DragScroll));
  sWire->AddKey(name,L"Maximize",sMessage(this,&sWireClientWindow::CmdMaximize));
  sWire->AddKey(name,L"Help",sMessage(this,&sWireClientWindow::CmdHelp));
}

void sWireClientWindow::OpenCode(const sChar *path)
{
  sString<sMAXPATH> buffer;
  buffer.PrintF(L"%P\\%P",sCONFIG_CODEROOT,path);
  sLogF(L"sys",L"shell <%s>\n",buffer);
  sExecuteOpen(buffer);
}

void sWireClientWindow::CmdHelp()
{
  sWire->Help(this);
}

/****************************************************************************/

sWireDummyWindow::sWireDummyWindow()
{
  Label = L"";
}

void sWireDummyWindow::OnPaint2D()
{
  sGui->PropFont->SetColor(sGC_TEXT,sGC_BACK);
  sGui->PropFont->Print(sF2P_OPAQUE,Client,Label);
}

/****************************************************************************/

void sWireGridFrame::InitWire(const sChar *name)
{
  sWire->AddWindow(name,this);
  sWire->AddDrag(name,L"Scroll",sMessage(this,&sWireGridFrame::DragScroll));
  sWire->AddKey(name,L"Maximize",sMessage(this,&sWireGridFrame::CmdMaximize));
}

void sWireGridFrame::DragScroll(const sWindowDrag &dd,sDInt)
{
  MMBScroll(dd);
}


/****************************************************************************/

