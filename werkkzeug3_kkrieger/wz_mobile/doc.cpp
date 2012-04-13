// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "doc.hpp"
#include "main_mobile.hpp"
#include "win_para.hpp"
#include "win_page.hpp"
#include "gen_bitmap_class.hpp"
#include "gen_mobmesh.hpp"

GenDoc *Werkkzeug3Textures::Doc;

/****************************************************************************/

GenInput GenInput::Make(sInt id,GenType *type,const sChar *name,sInt flags)
{
  GenInput gi;

  gi.Name = name;
  gi.ParaId = id;
  gi.Type = type;
  gi.Flags = flags;

  return gi;
}

/****************************************************************************/

void GenType::AddVariant(const sChar *name,sInt varid)
{
  GenVariant *var;
  var = new GenVariant;
  var->Name = name;
  var->VariantId = varid;
  var->Type = this;
  Variants.Add(var);
}

/****************************************************************************/

GenClass::GenClass()
{
  Version = 0;
  ClassId = 0;
  Flags = 0;
  Output = 0;
  Shortcut = 0;
  NextParaId = 0;
}

GenClass::~GenClass()
{
}

void GenClass::DrawHints(GenOp *op,sInt handle)
{
}

GenNode *GenClass::MakeNode(GenOp *op,sInt variantid,sInt sx,sInt sy)
{
  GenValue *val;
  GenPara *para;
  sInt *d;
  sInt size;
  GenOp *input;
  GenNode *node;
  sInt error;
  GenOp::Link_ *link;
  sInt varid[4];

  error = 0;

  // allocate node

  node = Doc->AllocNode<GenNode>();
  node->Class = op->Class;
  GenHandler *h=Handlers.Get(variantid);
  if(h)
    node->Handler = *h;
  else if(!(node->Class->Flags & (GCF_NOP|GCF_LOAD)))
    error=1;
  node->Op = op;
  node->Cache = &op->Cache;
  op->CacheVariant = variantid;

  node->SizeX = sx;
  node->SizeY = sy;
  node->Variant = variantid;
  node->Load = 0;
  node->StoreCount = 0;
  node->StoreLink = 0;

  // parameter. 

  size = 0;
  sFORLIST(&Para,para)
  {
    para->Value = 0;
    sFORLIST(&op->Values,val)
    {
      if(val->Para==para)
      {
        sVERIFY(para->Value==0);
        sVERIFY(para->ParaId==val->ParaId);
        para->Value = val;
        size += val->GetParaSize();
      }
    }
    if(para->Value==0 && para->Type!=GPT_LABEL)    // parameter fehlt!
      error = 1;
  }
  node->Para = Doc->AllocNode<sInt>(size);
  node->ParaCount = size;
  d=node->Para;
  sFORLIST(&Para,para)
  {
    val = para->Value;
    if(val)
    {
      val->StorePara(op,d);
      d+=val->GetParaSize();
    }
  }
  sVERIFY(d-node->Para == size);

  // special nodes)
  
  varid[0] = variantid;
  varid[1] = variantid;
  varid[2] = variantid;
  varid[3] = variantid;
  switch(node->Class->ClassId)
  {
  case GCI_BITMAP_DEPTH:
    switch(node->Para[0])
    {
      case 0: varid[0] = GVI_BITMAP_TILE16C; break;
      case 1: varid[0] = GVI_BITMAP_TILE16I; break;
      case 2: varid[0] = GVI_BITMAP_TILE8C;  break;
      case 3: varid[0] = GVI_BITMAP_TILE8I;  break;
    }
    break;

  case GCI_BITMAP_RESIZE:
    sx = node->Para[0]<<16;
    sy = node->Para[1]<<16;
    break;

  case GCI_BITMAP_NORMALS:
    varid[0] = GVI_BITMAP_TILE16I;
    break;

  case GCI_BITMAP_BUMP:
    varid[1] = GVI_BITMAP_TILE16C;
    break;

  case GCI_BITMAP_DISTORT:
    varid[1] = GVI_BITMAP_TILE16C;
    break;
  }

  // inputs

  if(Inputs.Count>0)
  {
    node->InputCount = Inputs.Count;
    node->Inputs = Doc->AllocNode<GenNode *>(Inputs.Count);
    for(sInt i=0;i<Inputs.Count;i++)
    {
      input = (i<op->Inputs->GetCount()) ? op->Inputs->Get(i) : 0;
      if(input)
      {
        if(Inputs[i].Flags&GIF_DONTFOLLOW)
        {
          node->Inputs[i] = Doc->AllocNode<GenNode>(1);
          node->Inputs[i]->Op = input;
        }
        else
        {
          node->Inputs[i] = input->Class->MakeNode(input,varid[sMin(3,i)],sx,sy);
          if(node->Inputs[i]==0)
            error = 1;
        }
      }
      if(input==0 && (Inputs[i].Flags&GIF_REQUIRED))
        error = 1;
    }
  }

  // links

  node->LinkCount = op->Links.GetCount();
  if(node->LinkCount)
    node->Links = Doc->AllocNode<GenNode *>(node->LinkCount);
  sFORLIST(&op->Links,link)
  {
    node->Links[_i] = 0;
    if(link->Link)
      node->Links[_i] = link->Link->Class->MakeNode(link->Link,variantid,sx,sy);
    if(node->Links[_i]==0 && (link->Name->Para->Flags & GPF_LINK_REQUIRED))
      error = 1;
  }

  // carefully clearing temporary pointers

  sFORLIST(&Para,para) para->Value=0;     
  sFORLIST(&op->Links,link) link->Node = 0;

  return error?0:node;
}

/****************************************************************************/
/****************************************************************************/

void GenOp::CombinedConstructor()
{
  StoreName[0] = 0;
  PosX = 0;
  PosY = 0;
  Width = 0;
  Height = 1;
  Selected = 0;
  Class = 0;

  Bypass = 0;
  Hide = 0;
  Error = 0;

  CacheVariant = 0;
  Changed = 0;
  CacheMesh = 0;
  CalcCount = 0;

  Inputs  = new sList<GenOp>;
  Outputs = new sList<GenOp>;
}

GenOp::GenOp()
{
  CombinedConstructor();
}

void GenOp::Tag()
{
  sBroker->Need(Inputs);
  sBroker->Need(Outputs);
  sObject::Tag();
  GenOp::Link_ *l;
  sFORLIST(&Links,l)
    if(l->Link)
      sBroker->Need(l->Link);
}

GenOp::GenOp(sInt x,sInt y,sInt w,GenClass *cl,const sChar *n)
{
  CombinedConstructor();
  if(n)
    StoreName = n;
  PosX = x;
  PosY = y;
  Width = w;
  Height = 1;
  Class = cl;
}

GenOp::~GenOp()
{
  FlushCache();
}

void GenOp::Copy(GenOp *o)
{
  GenValue *val;
  GenValue *valcopy;
  GenOp::Link_ *link;

  StoreName = o->StoreName;
  PosX = o->PosX;
  PosY = o->PosY;
  Width = o->Width;
  Height = o->Height;
  Class = o->Class;

  Values.Clear();
  Links.Clear();
  sFORLIST(&o->Values,val)
  {
    valcopy = val->Copy();
    Values.Add(valcopy);
    if(val->Para->Type==GPT_LINK)
    {
      link = Links.Add();
      link->Link = 0;
      link->Name = (GenValueLink *) valcopy;
    }
  }
}


void GenOp::Create()
{
  GenPara *gp;
  GenValue *gv;
  GenOp::Link_ *link;
  sFORLIST(&Class->Para,gp)
  {
    gv = CreateValue(gp);
    if(gv)
    {
      gv->Default(gp);
      Values.Add(gv);
      if(gv->Para->Type==GPT_LINK)
      {
        link = Links.Add();
        link->Link = 0;
        link->Name = (GenValueLink *) gv;
      }
    }
  }
}

GenValue *GenOp::FindValue(sInt paraid)
{
  GenValue *val;
  sFORLIST(&Values,val)
  {
    if(val->ParaId==paraid)
      return val;
  }
  sFatal("GenOp::FindValue ParaId %08x not found",paraid);
  return 0;
}

const GenValue *GenOp::FindValue(sInt paraid) const
{
  const GenValue *val;
  sFORLIST(&Values,val)
  {
    if(val->ParaId==paraid)
      return val;
  }
  sFatal("GenOp::FindValue ParaId %08x not found",paraid);
  return 0;
}

void GenOp::FlushCache()
{
  Cache.Clear();
  sRelease(CacheMesh);
  CacheVariant = 0;
}

GenObject *GenOp::FindVariant(sInt var)
{
  GenObject *obj;
  if(CacheMesh && CacheMesh->Variant==var)
    return CacheMesh;

  sFORALL(Cache,obj)
  {
    if(obj->Variant==var)
      return obj;
  }
  return 0;
}

GenOp *Werkkzeug3Textures::FollowOp(GenOp *op)
{
  GenOp *nop;
  for(;;)
  {
    if(op->Class->Flags & (GCF_NOP|GCF_STORE))
    {
      sVERIFY(op->Inputs->GetCount()==1)
      nop = op->Inputs->Get(0);
    }
    else if(op->Class->Flags & GCF_LOAD)
    {
      sVERIFY(op->Links.Count==1)
      nop = op->Links[0].Link;
    }
    else
    {
      return op;
    }
    if(nop==0)
    {
      return op;
    }
    op = nop;
  }
}


/****************************************************************************/
/****************************************************************************/

GenPage::GenPage(const sChar *n)
{
  Name = n;
  Stores = new sList<GenOp>;
}

GenPage::~GenPage()
{
}

void GenPage::Tag()
{
  Ops.Need();
  sBroker->Need(Stores);
  sObject::Tag();
}


GenOp *GenPage::FindOp(const sChar *name)
{
  GenOp *op;

  sFORLIST(Stores,op)
    if(op->StoreName==name)
      return op;
  return 0;
}


/****************************************************************************/

void GenPage::Connect1()
{
  GenOp *op;
  GenOp *old;

  sFORALL(Ops,op)
  {
    if(op->StoreName[0])
    {
      old = Doc->FindOp(op->StoreName);
      if(old)
      {
        op->Error = 1;
        old->Error = 1;
      }
      else
      {
        Stores->Add(op);
      }
    }
  }
}

sBool GenPage::Connect2()
{
  const sInt maxinput=256;
  GenOp *po,*pp;
  GenOp *in[maxinput];
  sInt onehaschanged;

  onehaschanged = 0;

// sort operators. this speeds up the search for TOP operator

  {
    sInt max = Ops.GetCount();
    sInt swapped = 1;
    for(sInt i=0;i<max-1 && swapped;i++)
    {
      swapped = 0;
      for(sInt j=0;j<max-i-1;j++)
      {
        po = Ops.Get(j); // i > j
        pp = Ops.Get(j+1); // j < i
        if(po->PosY > pp->PosY)
        {
          Ops.Swap(j,j+1);
          swapped = 1;
        }
      }
    }
  }


// find input operators

  sFORALL(Ops,po)
  {
//    sVERIFY(po->Page==this);

    if(po->Hide || (po->Class->Flags & GCF_COMMENT)) 
      continue;

    {
      sInt inc = 0;
      sInt changed;

      for(sInt j=_i-1;j>=0;j--)             // find all inputs
      {
        pp = Ops.Get(j);
//        if(pp->PosY<po->PosY-po->Height)    // the advantage of sorted ops!
//          continue;                         // height != 1
        if(pp->Hide || (pp->Class->Flags & GCF_COMMENT))
          continue;

        if(pp->PosY+pp->Height == po->PosY &&
          pp->PosX+pp->Width > po->PosX &&
          pp->PosX < po->PosX+po->Width)
        {
          while(pp && pp->Bypass)           // skip bypassed ops
          {
            if(pp->Error==0 && pp->Inputs->GetCount()>0)
              pp = pp->Inputs->Get(0);
            else
              pp = 0;
          }
          if(pp)
          {
            if(inc<maxinput)
              in[inc++] = pp;
            else
              po->Error = 1;
          }
        }
      }
        
      for(sInt j=0;j<inc-1;j++)             // sort inputs by x
        for(sInt k=j+1;k<inc;k++)
          if(in[j]->PosX > in[k]->PosX)
            sSwap(in[j],in[k]);

      changed = (inc!=po->Inputs->GetCount());    // changed?
      for(sInt j=0;j<inc && !changed;j++)
        if(in[j] != po->Inputs->Get(j))
          changed = 1;

      for(sInt j=0;j<po->Links.GetCount();j++)   // links
      {
        GenOp *link = Doc->FindOp(po->Links[j].Name->Buffer);
        if(po->Links[j].Link!=link)
        {
          po->Links[j].Link=link;
          changed = 1;
        }
      }

      if(changed)                           // if changed reset inputs 
      {
        onehaschanged = 1;
        po->Changed = 1;
        po->Inputs->Clear();
        for(j=0;j<inc;j++)
          po->Inputs->Add(in[j]);
      }
    }

    if(po->Bypass)                          // check bypass 
    {
      if(po->StoreName[0])
        po->Error = 1;
    }

    if(po->Inputs->GetCount()>po->Class->Inputs.Count)
      po->Error = 1;
    else
    {                                       // check inputs
      GenInput *ci;
      GenOp *oi;
      GenOp::Link_ *li;
      sFORALL(po->Class->Inputs,ci)
      {
        oi = _i<po->Inputs->GetCount()?po->Inputs->Get(_i):0;
        if(oi==0 && (ci->Flags&GIF_REQUIRED))
          po->Error = 1;
        if(oi && oi->Class->Output!=ci->Type)
          po->Error = 1;
      }
      sFORALL(po->Links,li)
      {
        if(li->Link==0 && (li->Name->Para->Flags & GPF_LINK_REQUIRED))
          po->Error = 1;
        if(li->Link && li->Link->Class->Output!=li->Name->Para->OutputType)
          po->Error = 1;
      }
    }

/*
    if(po->Class->Convention&OPC_VARIABLEINPUT)  // typecheck
    {
      if(inc<po->Class->MinInput)
        po->Error = 1;
      for(j=0;j<inc;j++)
      {
        if(po->GetInput(j)->Class->Output != po->Class->Inputs[0])
        {
          if(po->GetInput(j)->Class->Output==KC_ANY || po->Class->Inputs[0]==KC_ANY)
          {
            // check here if the actual objects are correct!
          }
          else
          {
            po->Error = 1;
          }
        }
      }
    }
    else
    {
      if(inc<po->Class->MinInput || inc>po->Class->MaxInput)
        po->Error = 1;
      for(j=0;j<inc;j++)
      {
        if(po->GetInput(j)->Class->Output != po->Class->Inputs[j])
        {
          if(po->GetInput(j)->Class->Output==KC_ANY || po->Class->Inputs[j]==KC_ANY)
          {
            // check here if the actual objects are correct! 
          }
          else
          {
            po->Error = 1;
          }
        }
      }
    }
*/

  }

  return onehaschanged;
//  Reconnect = sFALSE;
}

/****************************************************************************/
/****************************************************************************/

GenDoc::GenDoc()
{
#if !sPLAYER
  NodeHeapAlloc = 16*1024*1024;
#else
  NodeHeapAlloc = 0x40000;
#endif
  NodeHeapUsed = 0;
  NodeHeap = new sU8[NodeHeapAlloc];

  Pages = new sList<GenPage>;
}

GenDoc::~GenDoc()
{
  delete[] NodeHeap;
}

void GenDoc::Clear()
{
  Pages->Clear();
}

void GenDoc::Tag()
{
  sBroker->Need(Pages);
  sObject::Tag();
}

sBool GenDoc::Write(sU32 *&data)
{
  GenPage *page;
  GenOp *op;
  GenValue *val;
  sU32 *header = sWriteBegin(data,sCID_WZMOBILE_DOC,1);

  *data++ = Pages->GetCount();
  *data++ = 0;
  *data++ = 0;
  *data++ = 0;
  *data++ = 0;
  *data++ = 0;
  *data++ = 0;
  *data++ = 0;

  sFORLIST(Pages,page)
  {
    *data++ = page->Ops.GetCount();
    *data++ = 0;
    *data++ = 0;
    *data++ = 0;
    page->Name.Write(data);
    
    sFORALL(page->Ops,op)
    {
      *data++ = op->PosX;
      *data++ = op->PosY;
      *data++ = op->Width;
      *data++ = op->Class->ClassId;
      *data++ = op->Bypass;
      *data++ = op->Hide;
      *data++ = op->Values.GetCount();
      *data++ = op->Height;
      op->StoreName.Write(data);

      sFORALL(op->Values,val)
      {
        *data++ = val->ParaId;
        val->Write(data);
      }
    }
  }

  sWriteEnd(data,header); 
  return 1;
}

sBool GenDoc::Read(sU32 *&data)
{
  sInt pagecount;
  sInt opcount;
  GenName name;
  GenPage *page;
  GenOp *op;
  sU32 *header = sWriteBegin(data,sCID_WZMOBILE_DOC,1);

  pagecount = *data++;
  data+=7;

  for(sInt i=0;i<pagecount;i++)
  {
    page = new GenPage("");
    Pages->Add(page);

    opcount = *data++;
    data+=3;
    page->Name.Read(data);

    for(sInt j=0;j<opcount;j++)
    {
      sInt posx = *data++;
      sInt posy = *data++;
      sInt width = *data++;
      GenClass *clas = FindClass(*data++);
      sInt bypass = *data++;
      sInt hide = *data++;
      sInt valcount = *data++;
      sInt height = *data++;
      if(height==0) height = 1;
      name.Read(data);

      op = new GenOp(posx,posy,width,clas,name);
      op->Height = height;
      op->Bypass = bypass;
      op->Hide = hide;
      op->Create();
      page->Ops.Add(op);

      for(sInt k=0;k<valcount;k++)
      {
        sInt valid = *data++;
        GenValue *val = op->FindValue(valid);
        if(!val) return 0;
        val->Read(val->Para,data);
      }
    }
  }

  Connect();

  return sReadEnd(data); 
}

sBool GenDoc::Save(const sChar *name)
{
  sU32 *mem = new sU32[16*1024*1024/4];
  sU32 *data=mem;
  sInt result=0;

  if(Write(data))
    result = sSystem->SaveFile(name,(sU8 *)mem,(data-mem)*4);

  delete[] mem;
  return result;
}

sBool GenDoc::Load(const sChar *name)
{
  sU32 *mem = (sU32 *)sSystem->LoadFile(name);
  sU32 *data = mem;
  sInt result=0;

  if(data)
  {
    Clear();
    result = Read(data);
  }

  delete[] mem;
  return result;
}

GenType *GenDoc::FindType(sInt gti)
{
  GenType *e;
  sFORALL(Types,e)
    if(e->TypeId==gti)
      return e;
  sFatal("type %08x not found",gti);
}

GenClass *GenDoc::FindClass(sInt gci)
{
  GenClass *e;
  sFORALL(Classes,e)
    if(e->ClassId==gci)
      return e;
  sFatal("class %08x not found",gci);
}

/****************************************************************************/

void GenDoc::MakeNodePrepare()
{
  NodeHeapUsed = 0;
}

GenNode *GenDoc::OptimiseNode1R(GenNode *node)    // skip load,store,nop from ops
{
  if(!node) return 0;

  node->Op->Nodes.Clear();  // safety clear

  if(node->Class==0)        // link to other type
  {
    FollowOp(node->Op)->ExportRoot = 1;
    return node;
  }

  for(sInt i=0;i<node->InputCount;i++)
    node->Inputs[i] = OptimiseNode1R(node->Inputs[i]);
  for(sInt i=0;i<node->LinkCount;i++)
    node->Links[i] = OptimiseNode1R(node->Links[i]);

  if(node->Class->Flags & (GCF_NOP|GCF_STORE))
    return node->Inputs[0];
  else if(node->Class->Flags & GCF_LOAD)
  {
    sVERIFY(node->LinkCount==1)
    return (GenNode *)node->Links[0];
  }
  else
    return node;
}

GenNode *GenDoc::MakeNodeTree(GenOp *op,sInt variantid,sInt sx,sInt sy)
{
  GenNode *node;

  node = MakeNodeR(op,variantid,sx,sy);
  node = OptimiseNode1R(node);

  // this leaves the GenOp::Nodes array with old pointers!
  return node;
}

GenNode *GenDoc::MakeNodeR(GenOp *op,sInt variantid,sInt sx,sInt sy)
{
  return op ? op->Class->MakeNode(op,variantid,sx,sy) : 0;
}

sU8 *GenDoc::AllocNode(sInt bytes)
{
  sU8 *x = (NodeHeap+NodeHeapUsed);
  bytes = sAlign(bytes,4);
  NodeHeapUsed += bytes;
  sVERIFY(NodeHeapUsed<=NodeHeapAlloc);
  sSetMem(x,0,bytes);
  return x;
}

GenOp *GenDoc::FindOp(const sChar *name)
{
  GenPage *page;
  GenOp *op;

  sFORLIST(Pages,page)
  {
    op = page->FindOp(name);
    if(op) return op;
  }
  return 0;
}

GenPage *GenDoc::FindPage(GenOp *op)
{
  GenOp *o;
  GenPage *page;
  sFORLIST(Pages,page)
  {
    sFORALL(page->Ops,o)
      if(o==op)
        return page;
  }
  return 0;
}

void GenDoc::RenameOps(const sChar *oldname,const sChar *newname)
{
  GenPage *page;
  GenOp *op;
  GenValue *val;
  GenValueLink *link;
  sFORLIST(Pages,page)
  {
    sFORALL(page->Ops,op)
    {
      if(op->StoreName==oldname)
        op->StoreName = newname;
      sFORALL(op->Values,val)
      {
        if(val->Para->Type == GPT_LINK)
        {
          link = (GenValueLink *) val;
          if(sCmpString(link->Buffer,oldname)==0)
            sCopyString(link->Buffer,newname,link->Size);
        }
      }
      op->Changed |= 1;
    }
  }
  Change();
}

void GenDoc::Connect()
{
  GenPage *page;
  GenOp *op;
  GenOp *in;
  GenOp::Link_ *link;
  sBool changed;

  sFORLIST(Pages,page)
  {
    page->Stores->Clear();
    sFORALL(page->Ops,op)
    {
      op->Error = 0;
      op->Outputs->Clear();
    }
  }

  sFORLIST(Pages,page)
    page->Connect1();
  changed = 0;
  sFORLIST(Pages,page)
    changed |= page->Connect2();

  sFORLIST(Pages,page)
  {
    sFORALL(page->Ops,op)
    {
      sFORLIST(op->Inputs,in)
        in->Outputs->Add(op);
      sFORALL(op->Links,link)
        if(link->Link)
          link->Link->Outputs->Add(op);
    }
  }

  if(changed)
    Change();
}

void GenDoc::ChangeR(GenOp *op)
{
  GenOp *child;
  op->FlushCache();
  op->CacheVariant = 0;
  op->Changed |= 2;
  sFORLIST(op->Outputs,child)
  {
    if(!(child->Changed & 2))
      ChangeR(child);
  }
}

void GenDoc::Change()
{
  GenPage *page;
  GenOp *op;

  sFORLIST(Pages,page)
    sFORALL(page->Ops,op)
      op->Changed = op->Changed & 1;

  sFORLIST(Pages,page)
    sFORALL(page->Ops,op)
      if(op->Changed == 1)
        ChangeR(op);

  sFORLIST(Pages,page)
    sFORALL(page->Ops,op)
      op->Changed = 0;
}

void GenDoc::FlushCache()
{
  GenPage *page;
  GenOp *op;

  sFORLIST(Pages,page)
  {
    page->Stores->Clear();
    sFORALL(page->Ops,op)
    {
      op->FlushCache();
    }
  }
}

/****************************************************************************/

sInt GenDoc::ExportBitmaps()
{
  GenPage *page;
  GenOp *op;
  GenBitmap *gen;
  sBitmap *bm;
  sBool errors;
  GenValue *val;

  static sInt var[] = 
  {
    GenBitmapTile::Pixel8I::Variant,
    GenBitmapTile::Pixel8C::Variant,
    GenBitmapTile::Pixel16I::Variant,
    GenBitmapTile::Pixel16C::Variant,
  };

  Connect();

  errors = 0;
  sFORLIST(Pages,page)
  {
    sFORALL(page->Ops,op)
    {
      if(op->Class->ClassId == GCI_BITMAP_EXPORT)
      {
        if(op->Error) goto error;

        sInt xs,ys;
        sInt variant;
        sChar *name;
        
        val = op->FindValue(2); 
        if(!val || val->Para->Type!=GPT_CHOICE) goto error;
        xs = 16<<((GenValueInt *)val)->Value;

        val = op->FindValue(3); 
        if(!val || val->Para->Type!=GPT_CHOICE) goto error;
        ys = 16<<((GenValueInt *)val)->Value;

        val = op->FindValue(4); 
        if(!val || val->Para->Type!=GPT_CHOICE) goto error;
        variant = ((GenValueInt *)val)->Value; if(variant<0 || variant>3) goto error;

        val = op->FindValue(1); 
        if(!val || (val->Para->Type!=GPT_STRING && val->Para->Type!=GPT_FILENAME)) goto error;
        name = ((GenValueString *)val)->Buffer;

        gen = CalcBitmap(op,xs,ys,var[variant]);
        if(!gen) goto error;

        bm = gen->MakeBitmap();
        sVERIFY(bm);

        if(!sSaveBitmap(name,bm))
          errors++;
        delete bm;
        continue;
error: 
        errors++;
      }
    }
  }
  return errors;
}

/****************************************************************************/

void ExportGather(GenNode *node,sArray2<GenNode *> &list)
{
  if(node)
  {
    list.Add(node);
    node->UserFlags = 0;
    for(sInt i=0;i<node->InputCount;i++)
      ExportGather(node->Inputs[i],list);
  }
}

sInt FindHandler(void *handler)
{
  for(sInt i=0;GenBitmapHandlers[i].Handler;i++)
    if(GenBitmapHandlers[i].Handler==handler)
      return GenBitmapHandlers[i].Id;
  for(sInt i=0;GenMobMeshHandlers[i].Handler;i++)
    if(GenMobMeshHandlers[i].Handler==handler)
      return GenMobMeshHandlers[i].Id;
  return -1;
}

void GenDoc::OptimiseNode2R(GenNode *node)    // add load & store flags
{
  GenNode *store,*n2;

  if(!node) return;

  // find something

  store = 0;

  for(sInt i=0;i<node->Op->Nodes.GetCount();i++)
  {
    n2 = node->Op->Nodes[i];
    if(n2->Variant==node->Variant && n2->SizeX==node->SizeX && n2->SizeY==node->SizeY)
    {
      store = n2;
      break;
    }
  }

  if(store)
  {
    // link the load & store

    node->Load = store;
    store->StoreCount++;

    // and cut rest of tree

    node->Class = FindClass(GCI_NODELOAD);
    node->Handler = *node->Class->Handlers.Get(node->Variant);
    node->ParaCount = 0;
    node->InputCount = 0;
    node->LinkCount = 1;
    node->Para = 0;
    node->Inputs = 0;
    node->Links = AllocNode<GenNode *>(1);
    node->Links[0] = store;

    sVERIFY(node->Class);
    sVERIFY(node->Handler);
  }
  else
  {
    // add myself

    *node->Op->Nodes.Add() = node;

    // recurse

    for(sInt i=0;i<node->InputCount;i++)
      OptimiseNode2R(node->Inputs[i]);
    for(sInt i=0;i<node->LinkCount;i++)
      OptimiseNode2R(node->Links[i]);
  }
}

void GenDoc::OptimiseNode3R(GenNode *node,GenNode **patch)    // add load & store flags
{
  GenNode *store;

  if(!node) return;
  if(node->Class->ClassId==GCI_NODELOAD) return;

  if(node->StoreLink)
  {
    *patch = node->StoreLink;
  }
  else if(node->StoreCount>0)
  {
    store = AllocNode<GenNode>();
    store->Class = FindClass(GCI_NODESTORE);
    store->Handler = *store->Class->Handlers.Get(node->Variant);
    store->Variant = node->Variant;
    store->SizeX = node->SizeX;
    store->SizeY = node->SizeY;
    
    store->InputCount = 1;
    store->Inputs = AllocNode<GenNode *>(1);
    store->ParaCount = 1;
    store->Para = AllocNode<sInt>(1);

    sVERIFY(patch);
    sVERIFY(*patch);
    sVERIFY(node->Load==0);
    sVERIFY(store->Class);

    store->Para[0] = node->StoreCount;
    store->Inputs[0] = node;
    node->StoreLink = store;
  }

  for(sInt i=0;i<node->InputCount;i++)
    OptimiseNode3R(node->Inputs[i],&node->Inputs[i]);
  for(sInt i=0;i<node->LinkCount;i++)
    OptimiseNode3R(node->Links[i],&node->Links[i]);
}

void GenDoc::OptimiseNode4R(GenNode *node,sInt &count)
{
  if(!node) return;

  if(node->Class == 0) return;                    // a mesh links a textuer

  node->UserFlags = count++;

  if(node->Class->ClassId==GCI_NODELOAD) return;  // enum LOAD, but don't follow link!

  for(sInt i=0;i<node->InputCount;i++)
    OptimiseNode4R(node->Inputs[i],count);
  for(sInt i=0;i<node->LinkCount;i++)
    OptimiseNode4R(node->Links[i],count);
}

void GenDoc::WriteNode(GenNode *node,sU32 *&data)
{
  sInt hid;
  sInt endit = 0;

  if(!node) return;

  hid = FindHandler(node->Handler);
  if(hid==-1) 
    return;
  *data++ = hid;                  // handler id
  *data++ = node->InputCount;     // inputs
  *data++ = node->LinkCount;      // unused
  *data++ = node->ParaCount;      // parameters
  for(sInt i=0;i<node->InputCount;i++)
  {
    if(node->Inputs[i])
    {
      if(node->Inputs[i]->Class==0)
        *data++ = FollowOp(node->Inputs[i]->Op)->Nodes.Get(0)->UserFlags;
      else
        *data++ = node->Inputs[i]->UserFlags;
    }
    else
    {
      *data++ = ~0;
    }
  }

  for(sInt i=0;i<node->LinkCount;i++)
    *data++ = node->Links[i]?node->Links[i]->UserFlags:~0;
  for(sInt i=0;i<node->ParaCount;i++)
    *data++ = node->Para[i];

  if(endit) return;

  if(node->Class->ClassId==GCI_NODELOAD)
  {
    sVERIFY(node->LinkCount==1);
    sVERIFY(node->Links[0]);
    sVERIFY(node->Links[0]->UserFlags<node->UserFlags);
    return;
  }
  for(sInt i=0;i<node->InputCount;i++)
  {
    if(node->Inputs[i] && node->Inputs[i]->Class)
      WriteNode(node->Inputs[i],data);
  }
  for(sInt i=0;i<node->LinkCount;i++)
    WriteNode(node->Links[i],data);
}

sBool GenDoc::Export(sU32 *&data,GenOp *op_)
{
  GenNode *root,**node;
  GenPage *page;
  GenOp *op;
  sInt count;

  Connect();
  sArray2<GenNode *> list;
  sArray2<GenNode *> roots_bitmap;
  sArray2<GenNode *> roots_mesh;
  MakeNodePrepare();

  sFORLIST(Pages,page)
    sFORALL(page->Ops,op)
      /*FollowOp*/(op)->ExportRoot = (op->StoreName[0]=='_' && op->StoreName[1]=='_');

  // export meshes

  sFORLIST(Pages,page)
  {
    sFORALL(page->Ops,op)
    {
      if(op->ExportRoot && op->Class->Output->TypeId==GTI_MESH)
      {
        root = MakeNodeTree(op,GVI_MESH_MOBMESH,256,256);
        if(!root) 
          return 0;
        roots_mesh.Add(root);
        ExportGather(root,list);
      }
    }
  }

  // export bitmaps

  sFORLIST(Pages,page)
  {
    sFORALL(page->Ops,op)
    {
      if(op->ExportRoot && op->Class->Output->TypeId==GTI_BITMAP)
      {
        root = MakeNodeTree(op,GVI_BITMAP_TILE8C,256,256);
        if(!root) 
          return 0;
        roots_bitmap.Add(root);
        ExportGather(root,list);
      }
    }
  }
  
  sFORALL(roots_bitmap,node)                // find common subexpressions and insert loads
    OptimiseNode2R(*node);
  for(sInt i=0;i<roots_bitmap.GetCount();i++) // insert stores
  {
    root = roots_bitmap[i];
    OptimiseNode3R(root,&root);
    roots_bitmap[i] = root;
  }

  // enumerate ops

  count = 0;
  sFORALL(roots_bitmap,node)
    OptimiseNode4R(*node,count);
  sFORALL(roots_mesh,node)
    OptimiseNode4R(*node,count);

  // write it out

  sU32 *header = sWriteBegin(data,0,1);
  *data++ = count;                          // node count
  *data++ = roots_bitmap.GetCount();        // bitmap root count
  *data++ = roots_mesh.GetCount();          // mehs root cont
  *data++ = 0;
  *data++ = 0;
  *data++ = 0;

  sFORALL(roots_bitmap,node)                // bitmap roots
    *data++ = (*node)->UserFlags; 
  sFORALL(roots_mesh,node)                  // mesh roots
    *data++ = (*node)->UserFlags; 

  sFORALL(roots_bitmap,node)                // bitmap ops
    WriteNode(*node,data);
  sFORALL(roots_mesh,node)                  // mesh ops
    WriteNode(*node,data);

  sWriteEnd(data,header); 

  return 1;
}

/****************************************************************************/
/****************************************************************************/
