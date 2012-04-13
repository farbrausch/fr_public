// This file is distributed under a BSD license. See LICENSE.txt for details.


#define VERSION "0.1.13"

#include "_util.hpp"
#include "_gui.hpp"
#include "_startdx.hpp"
#include "materials/material11.hpp"
#include "_types2.hpp"
#include "win_pagelist.hpp"

#include "doc.hpp"
#include "config.hpp"

#include "main_mobile.hpp"
#include "win_page.hpp"
#include "win_para.hpp"
#include "win_view.hpp"
#include "win_viewmesh.hpp"
 
#include "gen_bitmap_class.hpp"
#include "gen_mobmesh.hpp"
#include "packer.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Class for the example                                              ***/
/***                                                                      ***/
/****************************************************************************/

sGuiManager *sGui;
sGuiPainter *sPainter;
class Wz3Tex *App;

extern const sChar DemoString[];

/****************************************************************************/

class Class_Misc_Comment : public GenClass
{
public:
  Class_Misc_Comment()
  {
    Name = "comment";
    ClassId = GCI_MISC_COMMENT;
    Column = GCC_GENERATOR;
    Output = Doc->FindType(GTI_MISC);
    Flags |= GCF_COMMENT;

    Para.Add(GenPara::MakeFlags(2,"Font","prop|fixed",0));
    Para.Add(GenPara::MakeText(1,"Comment","hund."));
  }
};

/****************************************************************************/
/****************************************************************************/

Wz3Tex::Wz3Tex()
{
  sHSplitFrame *hs;
  sVSplitFrame *vs0,*vs1;
  sToolBorder *tb;

// integer math

  InitBitmaps();
  sSystem->SetWinTitle(".werkkzeug mobile "VERSION);

// create doc

  {
    FinalDestruction = 0;
    Doc = new GenDoc;

    GenType *type;

    // types

    type = new GenType;
    type->Name = "misc";
    type->TypeId = GTI_MISC;
    type->Color = 0xffc0c0c0;
    Doc->Types.Add(type);

    type = new GenType;
    type->Name = "bitmap";
    type->TypeId = GTI_BITMAP;
    type->Color = 0xffc0ffff;
    type->AddVariant("Flat16RGBA",GVI_BITMAP_INTRO      );
    type->AddVariant("Tile8I"    ,GVI_BITMAP_TILE8I     );
    type->AddVariant("Tile8RGBA" ,GVI_BITMAP_TILE8C     );
    type->AddVariant("Tile16I"   ,GVI_BITMAP_TILE16I    );
    type->AddVariant("Tile16RGBA",GVI_BITMAP_TILE16C    );
    Doc->Types.Add(type);

    type = new GenType;
    type->Name = "mesh";
    type->TypeId = GTI_MESH;
    type->Color = 0xffffc0ff;
    type->AddVariant("MinMesh",GVI_MESH_MOBMESH);
    Doc->Types.Add(type);

    // classes

    Doc->Classes.Add(new Class_Misc_Comment);
 
    AddBitmapClasses(Doc);
    AddMobMeshClasses(Doc);
  }

  // default page

  {
    GenPage *page;
    GenOp *op;

    page = new GenPage("page1");
    op = new GenOp(1,1,3,Doc->FindClass(GCI_BITMAP_FLAT),0);     op->Create(); page->Ops.Add(op);
    op = new GenOp(1,2,3,Doc->FindClass(GCI_BITMAP_GLOWRECT),0); op->Create(); page->Ops.Add(op);
    op = new GenOp(1,3,3,Doc->FindClass(GCI_BITMAP_COLOR),0);    op->Create(); page->Ops.Add(op);
    op = new GenOp(1,4,3,Doc->FindClass(GCI_BITMAP_ROTATE),0);   op->Create(); page->Ops.Add(op);
    op = new GenOp(4,3,3,Doc->FindClass(GCI_BITMAP_FLAT),0);     op->Create(); page->Ops.Add(op);
    op = new GenOp(4,4,3,Doc->FindClass(GCI_BITMAP_COLOR),0);    op->Create(); page->Ops.Add(op);
    op = new GenOp(1,5,6,Doc->FindClass(GCI_BITMAP_ADD),"sf1t3");op->Create(); page->Ops.Add(op);
    op = new GenOp(9,3,3,Doc->FindClass(GCI_MESH_CUBE),0);       op->Create(); page->Ops.Add(op);
    Doc->Pages->Add(page);
    Doc->Pages->Add(new GenPage("page2"));
  }

// default environment

  DocPath = "./default.km";
  AutosaveTimer = 0;
  AutosaveRequired = 0;
  SwapViews = 0;
  CurrentViewWin = 0;
  LoadConfig();

// create windows

  AddTitle("Werkkzeug3 for Textures");
  PageListWin = new PageListWin_();
  PageWin = new PageWin_;
  ParaWin = new ParaWin_;
  LogWin = new sLogWindow;
  ParaSwitch = new sSwitchFrame;

  for(sInt i=0;i<MAX_VIEWS;i++)
  {
    ViewSwitch[i] = new sSwitchFrame;
    ViewBitmapWin[i] = new ViewBitmapWin_;
    ViewMeshWin[i] = new ViewMeshWin_;
    ViewSwitch[i]->AddChild(ViewBitmapWin[i]);
    ViewSwitch[i]->AddChild(ViewMeshWin[i]);
  }
  ViewSwitch[0]->Set(0);
  ViewSwitch[1]->Set(1);

  ParaSwitch->AddChild(ParaWin);
  ParaSwitch->AddChild(LogWin);

// chain windows

  hs = new sHSplitFrame;
  vs0 = new sVSplitFrame;
  vs1 = new sVSplitFrame;

  AddChild(hs);
  hs->AddChild(vs0);
  hs->AddChild(vs1);
  for(sInt i=0;i<MAX_VIEWS;i++)
    vs0->AddChild(ViewSwitch[i]);
  vs0->AddChild(ParaSwitch);
  vs1->Pos[1] = 150;
  vs1->AddChild(PageListWin);
  vs1->AddChild(PageWin);

// decoration

  StatusMouse = "";
  StatusWindow = "";
  StatusObject = "";
  StatusLog = "ok.";
  StatusBorder = new sStatusBorder;
  StatusBorder->SetTab(0,StatusMouse,256);
  StatusBorder->SetTab(1,StatusLog,256);
  StatusBorder->SetTab(2,StatusObject,320);
  StatusBorder->SetTab(3,StatusWindow,0);
  AddBorder(StatusBorder);

  PageWin->AddBorder(new sFocusBorder);
  PageListWin->AddBorder(new sFocusBorder);
  ParaWin->AddBorder(new sFocusBorder);
  for(sInt i=0;i<MAX_VIEWS;i++)
  {
    ViewBitmapWin[i]->AddBorder(new sFocusBorder);
    ViewMeshWin[i]->AddBorder(new sFocusBorder);
  }

  {
    tb = new sToolBorder;
    tb->MenuStyle = 1;
    tb->AddMenu("File",CMD_MAIN_FILE);
//    tb->AddMenu("Edit",CMD_MAIN_EDIT);

#if (VARIANT==VARIANT_MOBILE)
    tb->AddLabel(".werkkzeug for mobile v"VERSION);
#endif
#if (VARIANT==VARIANT_TEXTURE)
    tb->AddLabel(".werkkzeug for textures v"VERSION);
#endif
#if DEMO_VERSION
    sChar buffer[64];

    for(sInt i=0;DemoString[i];i++)
      buffer[i] = DemoString[i]^0xea;
    buffer[i] = 0;

    tb->AddLabel(buffer);
#endif
  }

  AddBorder(tb);

// start if off

  Change();
  sGui->Post(CMD_MAIN_FILE_OPEN,this);
//  sGui->Post(CMD_MAIN_CHANGEPAGE,this);

}

Wz3Tex::~Wz3Tex()
{
  SaveConfig();
}

void Wz3Tex::Tag()
{
  sDummyFrame::Tag();
  sBroker->Need(Doc);
  sBroker->Need(PageListWin);
  sBroker->Need(PageWin);
  sBroker->Need(ParaWin);
  for(sInt i=0;i<MAX_VIEWS;i++)
  {
    sBroker->Need(ViewBitmapWin[i]);
    sBroker->Need(ViewMeshWin[i]);
  }
}

void Wz3Tex::OnPaint()
{
  sU32 keyq = sSystem->GetKeyboardShiftState();
  if(PageListWin->Flags & sGWF_FOCUS)
  {
    App->StatusMouse = "l:select";
    sSPrintF(App->StatusWindow,"%d pages",Doc->Pages->GetCount());
  }
  else
  {
    App->StatusMouse = "-/-";
    App->StatusWindow = "";
  }

  sDummyFrame::OnPaint();
}

sBool sBackup(const sChar *name)
{
  sDirEntry *de,*dedel;
  sChar *s0,*s1;
  sInt len;
  sChar path[4096];
  sChar backup[4096];
  sInt best;
  sChar buffer[128];
  sInt i;
  sU8 *mem;
  sInt size;

  sCopyString(path,name,sCOUNTOF(path));
  if(!sSystem->CheckDir("backup"))
    sSystem->MakeDir("backup");

  sCopyString(backup,"backup/",sCOUNTOF(backup));
  s0 = sFileFromPathString(path);
  s1 = sFileExtensionString(s0);
  if(s1==0)
    len = sGetStringLen(s0);
  else 
    len = s1-s0;
  sAppendString(backup,s0,sCOUNTOF(backup));
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

  mem = sSystem->LoadFile(name,size);
  if(mem)
  {
    if(!sSystem->SaveFile(backup,mem,size))
    {      
      delete[] mem;
      return sFALSE;
    }
    delete[] mem;
  }
  return sTRUE;
}

void Wz3Tex::OnFrame()
{
  if(AutosaveRequired)
  {
    if(sSystem->GetTime()>AutosaveTimer+30*1000)
    {
      sGui->Post(CMD_MAIN_FILE_AUTOSAVE,this);
    }
  }
//  sDPrintF("autosave - %d in %d s\n",AutosaveRequired,sSystem->GetTime()-(AutosaveTimer+5*1000));
}

sBool Wz3Tex::OnCommand(sU32 cmd)
{
  GenPage *page;
  sMenuFrame *mf;

  switch(cmd)
  {
  case CMD_MAIN_CHANGEPAGE:
    page = PageListWin->Get();
    if(page)
      PageWin->SetPage(&page->Ops);
    break;

  case CMD_MAIN_FILE:
    mf = new sMenuFrame;

    mf->AddMenu("New",CMD_MAIN_FILE_NEW,0);
    mf->AddMenu("Open..",CMD_MAIN_FILE_OPENAS,'o');
    mf->AddMenu("Save as..",CMD_MAIN_FILE_SAVEAS,'a');
    mf->AddMenu("Save",CMD_MAIN_FILE_SAVE,'s'|sKEYQ_CTRL);
    mf->AddSpacer();
#if !DEMO_VERSION
    mf->AddMenu("Export to Mobile",CMD_MAIN_FILE_EXPORTMOBILE,0);
#endif
    mf->AddMenu("Export Bitmaps",CMD_MAIN_FILE_EXPORTBITMAPS,0);

    mf->SendTo = this;
    mf->AddBorder(new sNiceBorder);
    sGui->AddPulldown(mf);
    break;

  case CMD_MAIN_FILE_NEW:
    Doc->Clear();
    Doc->Pages->Add(new GenPage("page 1"));
    PageWin->SetPage(&Doc->Pages->Get(0)->Ops);
    break;

  case CMD_MAIN_FILE_OPENAS:
    if(sSystem->FileRequester(DocPath,DocPath.Size(),sFRF_OPEN))
      OnCommand(CMD_MAIN_FILE_OPEN);
    break;

  case CMD_MAIN_FILE_OPEN:
    if(Doc->Load(DocPath))
    {
      OnCommand(CMD_MAIN_CHANGEPAGE);
      StatusLog = "Load done";
      AutosaveRequired = 0;
      AutosaveTimer = sSystem->GetTime();
    }
    else
    {
      (new sDialogWindow)->InitOk(0,"error","error while loading",0);
      OnCommand(CMD_MAIN_FILE_NEW);
      StatusLog = "Load error";
    }
    break;

  case CMD_MAIN_FILE_SAVEAS:
    if(sSystem->FileRequester(DocPath,DocPath.Size(),sFRF_SAVE))
      OnCommand(CMD_MAIN_FILE_SAVE);
    break;

  case CMD_MAIN_FILE_SAVE:
    {
      const sChar *error="save done";

      if(!sBackup(DocPath))
        error="backup failed";

      if(Doc->Save(DocPath))
      {
        AutosaveRequired = 0;
        AutosaveTimer = sSystem->GetTime();
      }
      else
      {
        (new sDialogWindow)->InitOk(0,"error","error while saving",0);
        error = "Save error";
      }
      StatusLog = error;
    }
    break;

  case CMD_MAIN_FILE_EXPORTMOBILE:
    {
      sString<4096> newname;

      newname = DocPath;
      *sFileExtensionString(newname) = 0;
      newname.Add(".kkm");

      sU32 *mem = new sU32[16*1024*1024/4];
      sU32 *data=mem;
      if(Doc->Export(data,this->ViewBitmapWin[0]->ShowOp))
      {
        /*
        for(sInt i=0;mem+i<data;i++)
        {
          if((i&15)==0) sDPrintF("\n  ");
          sDPrintF("0x%08x,",mem[i]);
        }
        sDPrintF("\n\n");
        */

        // kkrunch it

        CCAPackerBackEnd pbe;
        GoodPackerFrontEnd pfe(&pbe);
        sU8 *packed = new sU8[1024*64];
        sInt packedsize = pfe.Pack((sU8 *)mem,(data-mem)*4,packed,0,0);
        if(packedsize)
        {
          LogF("Export %d bytes, kkrunchied to %d bytes!",(data-mem)*4,packedsize);
          if(sSystem->SaveFile(newname,(sU8 *)packed,packedsize))
          {
            delete[] mem;
            delete[] packed;
            break;
          }
        }
        delete[] packed;
      }
      delete[] mem;
      (new sDialogWindow)->InitOk(0,"error","error while exporting",0);
      StatusLog = "Export done";
    }
    break;

  case CMD_MAIN_FILE_EXPORTBITMAPS:
    if(Doc->ExportBitmaps()==0)
      StatusLog = "all Bitmaps exported";
    else
      StatusLog = "error while exporting";
    break;

  case CMD_MAIN_FILE_AUTOSAVE:
    {
      if(!sSystem->CheckDir(BACKUPA))
        sSystem->MakeDir(BACKUPA);
      sSystem->DeleteFile(BACKUPA"autosave-9.k");
      sSystem->RenameFile(BACKUPA"autosave-8.k",BACKUPA"autosave-9.k");
      sSystem->RenameFile(BACKUPA"autosave-7.k",BACKUPA"autosave-8.k");
      sSystem->RenameFile(BACKUPA"autosave-6.k",BACKUPA"autosave-7.k");
      sSystem->RenameFile(BACKUPA"autosave-5.k",BACKUPA"autosave-6.k");
      sSystem->RenameFile(BACKUPA"autosave-4.k",BACKUPA"autosave-5.k");
      sSystem->RenameFile(BACKUPA"autosave-3.k",BACKUPA"autosave-4.k");
      sSystem->RenameFile(BACKUPA"autosave-2.k",BACKUPA"autosave-3.k");
      sSystem->RenameFile(BACKUPA"autosave-1.k",BACKUPA"autosave-2.k");
      if(Doc->Save(BACKUPA"autosave-1.k"))
        StatusLog = "autosave ok";
      else
        StatusLog = "autosave failed";
      AutosaveRequired = 0;
      AutosaveTimer = sSystem->GetTime();
    }
    break;


  case CMD_MAIN_EDIT:
    mf = new sMenuFrame;

    mf->AddCheck("Small Fonts",CMD_MAIN_EDIT_SMALLFONTS,0,sGui->GetStyle()&sGS_SMALLFONTS);
    mf->AddCheck("Skin'05",CMD_MAIN_EDIT_SKIN2005,0,sGui->GetStyle()&sGS_SKIN05);
//    mf->AddCheck("Swap Views",CMD_MAIN_EDIT_SWAPVIEWS,0,SwapViews);

    mf->SendTo = this;
    mf->AddBorder(new sNiceBorder);
    sGui->AddPulldown(mf);
    break;

  case CMD_MAIN_EDIT_SMALLFONTS:
    sGui->SetStyle(sGui->GetStyle()^sGS_SMALLFONTS);
    break;
  case CMD_MAIN_EDIT_SKIN2005:
    sGui->SetStyle(sGui->GetStyle()^sGS_SKIN05);
    break;
  case CMD_MAIN_EDIT_SWAPVIEWS:
    SwapViews = !SwapViews;
    break;

  default:
    return sFALSE;
  }
  return sTRUE;
}

sBool Wz3Tex::OnShortcut(sU32 key)
{
  if(key&sKEYQ_CTRL) key|=sKEYQ_CTRL;
  if(key&sKEYQ_SHIFT) key|=sKEYQ_SHIFT;
  switch(key & (0x8001ffff|sKEYQ_CTRL|sKEYQ_SHIFT))
  {
  case 's'|sKEYQ_CTRL:
    OnCommand(CMD_MAIN_FILE_SAVE);
    return 1;
  }
  return 0;
}

void Wz3Tex::Change()
{
  if(!AutosaveRequired)
  {
    AutosaveRequired = 1;
    AutosaveTimer = sSystem->GetTime();
    StatusLog = "document changed - autosave enabled";
  }
}

/****************************************************************************/
/****************************************************************************/

void Wz3Tex::ShowOp(GenOp *op,sInt viewport)
{
  sVERIFY(viewport>=0 && viewport<MAX_VIEWS);

  if(op)
  {
    if(op->Class->Output->Name=="bitmap")
    {
      CurrentViewWin = ViewBitmapWin[viewport];
      ViewBitmapWin[viewport]->SetOp(op);
      ViewSwitch[viewport]->Set(0);
    }
    if(op->Class->Output->Name=="mesh")
    {
      CurrentViewWin = ViewMeshWin[viewport];
      ViewMeshWin[viewport]->SetOp(op);
      ViewSwitch[viewport]->Set(1);
    }
  }
}

void Wz3Tex::EditOp(GenOp *op)
{
  ParaWin->SetOp(op);
  ShowPara();
}

void Wz3Tex::UnsetOp(GenOp *op)
{
  if(!FinalDestruction)
  {
    if(App->ParaWin->GetOp()==op)
      App->ParaWin->SetOp(0);

    if(App->PageWin->EditOp==op)
      App->PageWin->EditOp = 0;

    for(sInt i=0;i<MAX_VIEWS;i++)
    {
      if(ViewBitmapWin[i]->ShowOp==op)
      {
        ViewBitmapWin[i]->SetOp(0);
      }

      if(ViewMeshWin[i]->ShowOp==op)
      {
        ViewMeshWin[i]->SetOp(0);
      }
    }
  }
}

void Wz3Tex::UnsetPage(GenPage *page)
{
  if(!FinalDestruction)
  {
    if(App->PageWin->Ops==&page->Ops)
      App->PageWin->SetPage(0);
  }
}

sBool Wz3Tex::IsShowed(const GenOp *op)
{
  for(sInt i=0;i<MAX_VIEWS;i++)
  {
    if(ViewBitmapWin[i]->ShowOp==op)
      return 1;

    if(ViewMeshWin[i]->ShowOp==op)
      return 1;
  }
  return 0;
}

sBool Wz3Tex::IsEdited(const GenOp *op)
{
  return App->ParaWin->GetOp()==op;
}

GenOp *Wz3Tex::GetEditOp()
{
  return App->ParaWin->GetOp();
}

void Wz3Tex::SetPage(GenPage *page)
{
  PageWin->SetPage(&page->Ops);
  PageListWin->SetPage(page);
}

void Wz3Tex::SetPage(GenOp *op)
{
  GenPage *gp;
  GenOp *go;

  sFORLIST(Doc->Pages,gp)
  {
    sFORALL(gp->Ops,go)
    {
      if(go==op)
      {
        App->SetPage(gp);
        return;
      }
    }
  }
}

void Wz3Tex::GotoOp(GenOp *op)
{
  SetPage(op);
  PageWin->ScrollToOp(op);
  EditOp(op);
}

/****************************************************************************/

void Wz3Tex::ShowPara()
{
  ParaSwitch->Set(0);
}

void Wz3Tex::ShowLog()
{
  ParaSwitch->Set(1);
}

void Wz3Tex::ClearLog()
{
  LogWin->Clear();
}

void Wz3Tex::Log(const sChar *bla)
{
  LogWin->PrintLine(bla);
  ShowLog();
}

void Wz3Tex::LogF(const sChar *format,...)
{
  sChar str[1024];

  sFormatString(str,sCOUNTOF(str),format,&format);
  LogWin->PrintLine(str);
  ShowLog();
}

/****************************************************************************/

void Wz3Tex::SaveConfig()
{
  sU32 *mem = new sU32[1024];
  sU32 *data = mem;

  // header

  sU32 *header = sWriteBegin(data,0,1);

  // basic information

  DocPath.Write(data);
  *data++ = SwapViews;

  // done

  sWriteEnd(data,header);
  sSystem->SaveFile("wz3m.config",(sU8 *)mem,(data-mem)*4);
  delete[] mem;
}

void Wz3Tex::LoadConfig()
{
  // header

  sU32 *mem = (sU32 *) sSystem->LoadFile("wz3m.config");
  if(!mem) goto error2;
  sU32 *data = mem;

  sInt version = sReadBegin(data,0);
  if(version<1 || version>1) goto error;

  // basic information

  DocPath.Read(data);
  SwapViews = *data++;

  // done

  if(!sReadEnd(data)) goto error;
  delete[] mem;
  return;

error:
  delete[] mem;
error2:
  DocPath = "./default.km";
  SwapViews = 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   Event Dispatcher - the system does not define a virtual base class ***/
/***                                                                      ***/
/****************************************************************************/

void InitGui()
{
  sOverlappedFrame *ov;
  sInt i;
  sScreenInfo si;

  for(i=0;i<sGUI_MAXROOTS;i++)
  {
    if(sSystem->GetScreenInfo(i,si))
    {
      ov = new sOverlappedFrame;
      ov->RightClickCmd = 4;
      sGui->SetRoot(ov,i);
    }
  }

  sGui->SetStyle(sGui->GetStyle()|sGS_SMALLFONTS|sGS_SKIN05);

  App = new Wz3Tex;
  App->Position.Init(0,0,300,200);
  sGui->AddApp(App);

  sGui->GetRoot()->GetChild(0)->FindTitle()->Maximise(1);
}

sBool sAppHandler(sInt code,sDInt value)
{
  switch(code)
  {
  case sAPPCODE_CONFIG:
    sSetConfig(sSF_DIRECT3D|sSF_MINIMAL,0,0);
    break;

  case sAPPCODE_INIT:
    sInitPerlin();

    sPainter = new sGuiPainter;
    sPainter->Init();
    sBroker->AddRoot(sPainter);
    sGui = new sGuiManager;
    sBroker->AddRoot(sGui);
    InitGui();
    break;

  case sAPPCODE_EXIT:
    sBroker->RemRoot(sPainter);
    sBroker->RemRoot(sGui); sGui = 0;
    sBroker->Free();
    sSystem->SetSoundHandler(0,0,0);
    sBroker->Free();
    break;

  case sAPPCODE_KEY:
    switch(value)
    {
    case sKEY_ESCAPE|sKEYQ_SHIFTL:
    case sKEY_ESCAPE|sKEYQ_SHIFTR:
    case sKEY_CLOSE:
      sSystem->Exit();
      break;
    default:
      sGui->OnKey(value);
      break;
    }
    break;

  case sAPPCODE_PAINT:
    {
      sInt i;
      sBool ok;

      sGui->OnFrame();
      sGui->OnPaint();
      ok = sFALSE;
      for(i=0;i<sGUI_MAXROOTS;i++)
        if(sGui->GetRoot(i) && sGui->GetRoot(i)->GetChildCount()!=0)
          ok = sTRUE;
      if(!ok) 
        sSystem->Exit();
    }
    break;

  case sAPPCODE_TICK:
    sGui->OnTick(value);
//    sSPrintF(App->StatusLog,"mem %d bytes",sSystem->MemoryUsed());
    break;

  case sAPPCODE_DEBUGPRINT:
    if(sGui)
      return sGui->OnDebugPrint((sChar *)value);
    return sFALSE;

  default:
    return sFALSE;
  }
  return sTRUE;
}

/****************************************************************************/
/****************************************************************************/
