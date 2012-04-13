// This file is distributed under a BSD license. See LICENSE.txt for details.

#pragma once

#include "win_page.hpp"
#include "main_mobile.hpp"
#include "gen_bitmap_class.hpp"

void ImportDump(sObjList<GenOp> *Ops,sInt x,sInt y);

/****************************************************************************/
/***                                                                      ***/
/***   Page Window - most of it is derived..                              ***/
/***                                                                      ***/
/****************************************************************************/

PageWin_::PageWin_()
{
  {
    using namespace Types2;
    static sGuiMenuList ml[] = 
    {
      { sGML_COMMAND  ,'a'              ,CMD_PAGE_ADD       ,0,"Add" },
      { sGML_COMMAND  ,'0'              ,CMD_PAGE_ADD0      ,0,"Add Misc" },
      { sGML_COMMAND  ,'1'              ,CMD_PAGE_ADD1      ,0,"Add Bitmap" },
      { sGML_COMMAND  ,'2'              ,CMD_PAGE_ADD2      ,0,"Add Mesh" },
      { sGML_COMMAND  ,'s'              ,CMD_PAGE_SHOW0     ,0,"Show left" },
      { sGML_COMMAND  ,'S'|sKEYQ_SHIFT  ,CMD_PAGE_SHOW1     ,0,"Show right" },
      { sGML_SPACER },
      { sGML_COMMAND  ,'x'              ,CMD_PAGE_CUT       ,0,"Cut" },
      { sGML_COMMAND  ,'c'              ,CMD_PAGE_COPY      ,0,"Copy" },
      { sGML_COMMAND  ,'v'              ,CMD_PAGE_PASTE     ,0,"Paste" },
      { sGML_COMMAND  ,sKEY_DELETE      ,CMD_PAGE_DELETE    ,0,"Delete" },
      { sGML_SPACER },
      { sGML_COMMAND  ,'y'              ,CMD_PAGE_BIRDON },
      { sGML_COMMAND  ,'y'|sKEYQ_BREAK  ,CMD_PAGE_BIRDOFF },
      { sGML_CHECKMARK,'y'              ,CMD_PAGE_BIRDTOGGLE,sOFFSET(PageWin_,BirdsEye),"Birdseye" },
      { sGML_CHECKMARK,sKEY_TAB         ,CMD_PAGE_FULLSCREEN,sOFFSET(PageWin_,Fullsize),"Full size" },
      { sGML_SPACER },
      { sGML_COMMAND  ,0                ,CMD_PAGE_FLUSHCACHE,0,"Flush Cache" },
      { sGML_COMMAND  ,'g'              ,CMD_PAGE_GOTO      ,0,"Goto" },
      { sGML_COMMAND  ,'e'              ,CMD_PAGE_EXCHANGE  ,0,"Exchange op" },
      { sGML_COMMAND  ,'b'              ,CMD_PAGE_BYPASS    ,0,"bypassed ops will be skipped" },
      { sGML_COMMAND  ,'h'              ,CMD_PAGE_HIDE      ,0,"hidden ops will be ignored" },
      { sGML_COMMAND  ,'l'              ,CMD_PAGE_LOG       ,0,"op info" },
      { sGML_COMMAND  ,'r'              ,CMD_PAGE_RENAME0   ,0,"rename op" },
      { sGML_COMMAND  ,'i'              ,CMD_PAGE_IMPORTDUMP,0,"import dump" },
      { 0 },
    };
    SetMenuList(ml);
  }

  PageX = 24;
  PageY = 16;

  LastAdd = GTI_BITMAP;

  Tools = new sToolBorder;
  Tools->AddLabel(".operator page");
  Tools->AddContextMenu(CMD_PAGE_MENU);
  AddBorder(Tools);
  AddScrolling();
}

PageWin_::~PageWin_()
{
}

void PageWin_::PaintOp(const GenOp *op)
{
  sRect r;
  const sChar *str;
  sU32 color;
  sU32 col00,col01,col10,col11;
  sGuiPainterVert v[4];
  sBool comment=0;

  str = op->Class->Name;
  if(!op->StoreName.IsEmpty())
    str = op->StoreName;
  if((op->Class->Flags&GCF_LOAD) && (op->Links[0].Name->Buffer))
    str = op->Links[0].Name->Buffer;
  if(op->Class->Flags & GCF_COMMENT)
  {
    GenValueText *vt = (GenValueText *)(op->FindValue(1));
    if(vt && vt->Para->Type==GPT_TEXT && vt->Text->Used>0)
    {
      str = vt->Text->Text;
      comment = 1;
    }
  }

  color = op->Class->Output->Color;
  if(op->Error)
    color = 0xffff0000;
  if(color==0)
    color = sGui->Palette[sGC_BUTTON];

  col00=col01=col10=col11 = color;

  if(op->Bypass)
    col00=col01=col10=col11= ((color&0xfefefe)>>1)|0xff000000;
  if(op->Hide)
    col10=col11= sGui->Palette[sGC_BACK];


  MakeRect(op,r);

  sGui->Bevel(r,op->Selected);
  v[0].Init(r.x0,r.y0,col00);
  v[1].Init(r.x0,r.y1,col01);
  v[2].Init(r.x1,r.y1,col11);
  v[3].Init(r.x1,r.y0,col10);
  sPainter->Paint(sGui->FlatMat,v,1);
  if(comment)
  {
    sRect rr;
    rr = r;
    rr.Extend(-3);
    rr.x0 += 6;
    rr.x1 -= 6;
    sInt font=0;
    GenValueInt *val = (GenValueInt *) op->FindValue(2);
    if(val) font = val->Value;
    
    sPainter->PrintM(font?sGui->FixedFont:sGui->PropFont,rr,sFA_TOP|sFA_LEFT,str,sGui->Palette[sGC_TEXT]);
  }
  else
  {
    sPainter->PrintC(sGui->PropFont,r,0,str,sGui->Palette[sGC_TEXT]);
  }

  if(!op->Bypass && !op->Hide && !comment)
  {
    if(App->IsShowed(op))
    {
      Paint(r.x0+2,r.y1-5,4,3,0xff007f00);
    }
    if(App->IsEdited(op))
    {
      Paint(r.x0+2,r.y0+2,4,3,0xff7f0000);
    }
    if(op->Cache.GetCount() || op->CacheMesh)
    {
      Paint(r.x0+2,(r.y0+r.y1-2)/2,4,2,0xff00007f);
    }

    sF32 x=(r.x1+r.x0)/2;
    switch(op->CacheVariant)
    {
    case GVI_BITMAP_TILE16C:
      v[0].Init(r.x1-8,r.y0+1,0xff0080ff);
      v[1].Init(r.x1-1,r.y0+1,0xffff8000);
      v[2].Init(r.x1-1,r.y1-1,0xffff8000);
      v[3].Init(r.x1-8,r.y1-1,0xff0080ff);
      sPainter->Paint(sGui->FlatMat,v,1);
      break;
    case GVI_BITMAP_TILE16I:
      v[0].Init(r.x1-8,r.y0+1,0xff000000);
      v[1].Init(r.x1-1,r.y0+1,0xffffffff);
      v[2].Init(r.x1-1,r.y1-1,0xffffffff);
      v[3].Init(r.x1-8,r.y1-1,0xff000000);
      sPainter->Paint(sGui->FlatMat,v,1);
      break;
    case GVI_BITMAP_TILE8C:
      v[0].Init(r.x1-7,r.y0+1,0xff0080ff);
      v[1].Init(r.x1-4,r.y0+1,0xff0080ff);
      v[2].Init(r.x1-4,r.y1-1,0xff0080ff);
      v[3].Init(r.x1-7,r.y1-1,0xff0080ff);
      sPainter->Paint(sGui->FlatMat,v,1);
      v[0].Init(r.x1-4,r.y0+1,0xffff8000);
      v[1].Init(r.x1-1,r.y0+1,0xffff8000);
      v[2].Init(r.x1-1,r.y1-1,0xffff8000);
      v[3].Init(r.x1-4,r.y1-1,0xffff8000);
      sPainter->Paint(sGui->FlatMat,v,1);
      break;
    case GVI_BITMAP_TILE8I:
      v[0].Init(r.x1-8,r.y0+1,0xff000000);
      v[1].Init(r.x1-4,r.y0+1,0xff000000);
      v[2].Init(r.x1-4,r.y1-1,0xff000000);
      v[3].Init(r.x1-8,r.y1-1,0xff000000);
      sPainter->Paint(sGui->FlatMat,v,1);
      v[0].Init(r.x1-4,r.y0+1,0xffffffff);
      v[1].Init(r.x1-1,r.y0+1,0xffffffff);
      v[2].Init(r.x1-1,r.y1-1,0xffffffff);
      v[3].Init(r.x1-4,r.y1-1,0xffffffff);
      sPainter->Paint(sGui->FlatMat,v,1);
      break;
    }

    sInt wheel = (op->CalcCount&0xffff) % (r.y1-r.y0-3) + 1;
    if(op->Cache.GetCount()>1)
    {
      sPainter->LineF(r.x1-8,r.y0+wheel+0,r.x1,r.y0+wheel,0xffff0000);
      sPainter->LineF(r.x1-8,r.y0+wheel+1,r.x1,r.y0+wheel,0xffff0000);
    }
    else
    {
      sPainter->LineF(r.x1-8,r.y0+wheel+0,r.x1,r.y0+wheel,0xff000000);
      sPainter->LineF(r.x1-8,r.y0+wheel+1,r.x1,r.y0+wheel,0xffffffff);
    }

    if(op->Class->Flags & GCF_LOAD)
    {
      sPainter->LineF(r.x0  +4,r.y0,r.x0  ,r.y0+4,0xff000000);
      sPainter->LineF(r.x1-1-4,r.y0,r.x1-1,r.y0+4,0xff000000);
    }
    if(op->Class->Flags & GCF_STORE)
    {
      sPainter->LineF(r.x0  +4,r.y1-1,r.x0  ,r.y1-1-4,0xff000000);
      sPainter->LineF(r.x1-1-4,r.y1-1,r.x1-1,r.y1-1-4,0xff000000);
    }
  }
}

void PageWin_::OnPaint()
{
  sU32 keyq = sSystem->GetKeyboardShiftState();
  if(Flags & sGWF_FOCUS)
  {
    if(keyq&sKEYQ_CTRL)
      App->StatusMouse = "l:duplicate  m:scroll [menu]   r:width";
    else if(keyq&sKEYQ_SHIFT)
      App->StatusMouse = "l:select toggle  m:scroll [menu]   r:select add";
    else
      App->StatusMouse = "l:select/width [show]   m:scroll [menu]   r:width [add]";
    sSPrintF(App->StatusWindow,"x:%d - y:%d ; %d in clipboard",CursorX,CursorY,Clipboard.GetCount());
  }

  sGuiOpPage<GenOp>::OnPaint();
}

sBool PageWin_::OnCommand(sU32 cmd)
{
  GenOp *op;
  sInt sv;

  switch(cmd)
  {
  case CMD_PAGE_MENU:
    PopupMenuList();
    break;

  case CMD_PAGE_ADD:
    AddOpMenu(LastAdd);
    break;

  case CMD_PAGE_ADD0:
    LastAdd = GTI_MISC;
    AddOpMenu(LastAdd);
    break;

  case CMD_PAGE_ADD1:
    LastAdd = GTI_BITMAP;
    AddOpMenu(LastAdd);
    break;

  case CMD_PAGE_ADD2:
    LastAdd = GTI_MESH;
    AddOpMenu(LastAdd);
    break;

  case CMD_PAGE_SHOW:               // from baseclass
  case CMD_PAGE_SHOW0:              // from this class
    sv = App->SwapViews;
    if(EditOp && EditOp->Class->Output->TypeId==GTI_MESH) sv = !sv;
    App->ShowOp(EditOp,sv);
    break;

  case CMD_PAGE_SHOW1:
    sv = App->SwapViews;
    if(EditOp && EditOp->Class->Output->TypeId==GTI_MESH) sv = !sv;
    App->ShowOp(EditOp,!sv);
    break;

  case CMD_PAGE_SETOP:
    App->EditOp(EditOp);
    break;

  case CMD_PAGE_FLUSHCACHE:
    Doc->FlushCache();
    break;

  case CMD_PAGE_CHANGED:
    App->Change();
    break;

  case CMD_PAGE_GOTO:
    op = App->GetEditOp();
    if(op && op->Links.GetCount()>0 && op->Links[0].Link)
    {
      App->SetPage(op->Links[0].Link);
      ScrollToOp(op->Links[0].Link);
      App->EditOp(op->Links[0].Link);
    }
    break;

  case CMD_PAGE_EXCHANGE:
    Exchange(App->GetEditOp());
    break;

  case CMD_PAGE_HIDE:
    op = App->GetEditOp();
    if(op)
    {
      op->Hide = !op->Hide;
      Doc->Connect();
      App->Change();
    }
    break;

  case CMD_PAGE_BYPASS:
    op = App->GetEditOp();
    if(op)
    {
      op->Bypass = !op->Bypass;
      Doc->Connect();
      App->Change();
    }
    break;

  case CMD_PAGE_LOG:
    op = App->GetEditOp();
    if(op)
    {
      static const sChar *var[] = { "???","???","mesh","8 Grey","8 Color","16 Grey","16 Color" };
      GenObject *co;
      GenBitmap *bm;
      App->LogF("=== operator %s ===\n",(sChar *)op->Class->Name);
      sFORALL(op->Cache,co)
      {
        if(co->Variant==GVI_BITMAP_TILE8I || co->Variant==GVI_BITMAP_TILE8C || co->Variant==GVI_BITMAP_TILE16I || co->Variant==GVI_BITMAP_TILE16C)
        {
          bm = (GenBitmap*) co;
          App->LogF("  variant %s; %d x %d\n",var[co->Variant],bm->XSize,bm->YSize);
        }
        else
        {
          App->LogF("  variant %s\n",var[co->Variant]);
        }
      }
    }
    if(op && op->CacheMesh)
    {
      App->Log("  variant mobmesh\n");
    }
    break;
    
  case CMD_PAGE_RENAME0:
    op = App->GetEditOp();
    if(op && op->StoreName[0])
    {
      sDialogWindow *dlg = new sDialogWindow;
      RenameOld = op->StoreName;
      RenameNew = (sChar *) RenameOld;
      sSPrintF(RenameMsg,"rename all loads and stores from \"%s\" to:",(sChar *)RenameOld);
      dlg->InitString(RenameNew,RenameNew.Size());
      dlg->InitOkCancel(this,"rename ops",RenameMsg,CMD_PAGE_RENAME1,0);
    }
    break;
    
  case CMD_PAGE_RENAME1:
    if(Doc->FindOp(RenameNew))
    {
      sDialogWindow *dlg = new sDialogWindow;
      dlg->InitOk(this,"rename ops","this operator name is already in use",0);
    }
    else
    {
      Doc->RenameOps(RenameOld,RenameNew);
    }
    break;

  case CMD_PAGE_IMPORTDUMP:
    ImportDump(Ops,CursorX,CursorY);
    break;


  default:
    if((cmd&0xfffff000)==CMD_PAGE_ADDOP)
    {
      AddOp(cmd&0xfff);
      OnKey(sKEY_DOWN);
    }
    else
    {
      return sGuiOpPage<GenOp>::OnCommand(cmd);
    }
  }
  return sTRUE;
}
 
void PageWin_::AddOpMenu(sInt type)
{
  GenClass *oc;
  sMenuFrame *mf;
  GenType *tp;
//  sBool letter[256];
  sInt shortcut;
  sInt found,add;

//  sSetMem(letter,0,sizeof(letter));
  mf = new sMenuFrame;

  sFORALL(Doc->Types,tp)
    mf->AddMenu(tp->Name,CMD_PAGE_ADD0+_i,'0'+_i)->BackColor = tp->Color;

  add = 1;
  found = 0;
  for(sInt c=0;c<GCC_MAX;c++)
  {
    sFORALL(Doc->Classes,oc)
    {
      if(oc->Column==c && oc->Output && oc->Output->TypeId==type)
      {
//        shortcut = oc->Name[0];
//        if(letter[shortcut]==0)
//          letter[shortcut]=1;
//        else
//          shortcut=0;
        shortcut = oc->Shortcut;

        if(add)
        {
          mf->AddColumn();
          add = 0;
          found = 0;
        }
        mf->AddMenu(oc->Name,CMD_PAGE_ADDOP+_i,shortcut);
        found = 1;
      }
    }
    add = found;
  }
  mf->AddBorder(new sNiceBorder);
  mf->SendTo = this;
  sGui->AddPopup(mf);
}

void PageWin_::AddOp(sInt id)
{
  if(CheckDest(CursorX,CursorY,CursorWidth,1))
  {
    GenOp *op = new GenOp(CursorX,CursorY,CursorWidth,Doc->Classes.Get(id),0);
    op->Create();
    Ops->Add(op);
  }
}

void PageWin_::Exchange(GenOp *op)
{
  GenOp *no;
  GenValueString *val;
  
  no = 0;
  switch(op->Class->ClassId)
  {
  case GCI_BITMAP_LOAD:
    val = (GenValueString *) op->FindValue(1);
    no = new GenOp(op->PosX,op->PosY,op->Width,Doc->FindClass(GCI_BITMAP_STORE),val->Buffer);
    no->Create();
    break;
  case GCI_BITMAP_STORE:
    no = new GenOp(op->PosX,op->PosY,op->Width,Doc->FindClass(GCI_BITMAP_LOAD),0);
    no->Create();
    val = (GenValueString *) no->FindValue(1);
    sCopyString(val->Buffer,op->StoreName,val->Size);
    break;
  }

  if(no)
  {
    Ops->Rem(op);
    Ops->Add(no);
    App->EditOp(no);
    App->Change();
  }
}


/****************************************************************************/
/****************************************************************************/
