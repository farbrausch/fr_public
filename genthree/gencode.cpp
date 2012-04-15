// This file is distributed under a BSD license. See LICENSE.txt for details.
#include "gencode.hpp"

#include "genplayer.hpp"
#include "genbitmap.hpp"
#include "genmesh.hpp"
#include "genmaterial.hpp"
#include "genfx.hpp"

#include "winpage.hpp"
#include "wintime.hpp"
#include "winproject.hpp"

extern "C" sChar SystemTxt[];


/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Class Information that should be extracted from system.txt         ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

/****************************************************************************/

sChar *OffsetName(sU32 cid,sInt offset)
{
  switch(cid)
  {
  case sCID_GENSCENE:
    switch(offset)
    {
      case -1: return "scale|rotate|transform|count";
      case 0: return "scale";
      case 1: return "rotate";
      case 2: return "translate";
      case 3: return "count";
    }
    break;

  case sCID_GENMATRIX:
    switch(offset)
    {
      case -1: return "scale|rotate|transform";
      case 0: return "scale";
      case 1: return "rotate";
      case 2: return "translate";
    }
    break;

  case sCID_GENMATPASS:
    switch(offset)
    {
      case -1: return "aspect|size|renderpass|color|enlarge";
      case 0: return "aspect";
      case 1: return "size";
      case 2: return "renderpass";
      case 3: return "color";
      case 4: return "enlarge";
    }
    break;

  case sCID_GENMESHANIM:
    switch(offset)
    {
      case -1: return "scale|rotate|translate|vector|parameter";
      case 0: return "scale";
      case 1: return "rotate";
      case 2: return "translate";
      case 3: return "vector";
      case 4: return "parameter";
    }
    break;

  case sCID_GENLIGHT:
    switch(offset)
    {
      case -1: return "diffuse|ambient|specular|range|rangecut|amplify|spotfaloff|spotinner|spotouter";
      case 0: return "diffuse";
      case 1: return "ambient";
      case 2: return "specular";
      case 3: return "range";
      case 4: return "rangecut";
      case 5: return "amplify";
      case 6: return "spotfalloff";
      case 7: return "spotinner";
      case 8: return "spotouter";
    }
		break;

	case sCID_GENFXCHAIN:
		switch(offset)
		{
		case -1: return "color|zoom|rotate|center|campos|camrot";
		case 0: return "color";
		case 1: return "zoom";
		case 2: return "rotate";
		case 3: return "center";
		case 4: return "campos";
		case 5: return "camrot";
		}
		break;
  }
  return 0;
}


sInt AdressName(sU32 cid,sInt offset,sInt &f)
{
  f = 0;
  switch(cid)
  {
  case sCID_GENSCENE:
    switch(offset)
    {
      case 0: f=1; return sizeof(sObject)/4+0;
      case 1: f=1; return sizeof(sObject)/4+3;
      case 2: f=1; return sizeof(sObject)/4+6;
      case 3: return sizeof(sObject)/4+9;
    }
    break;

  case sCID_GENMATRIX:
    switch(offset)
    {
      case 0: f=1; return sizeof(sObject)/4+0;
      case 1: f=1; return sizeof(sObject)/4+3;
      case 2: f=1; return sizeof(sObject)/4+6;
    }
    break;

  case sCID_GENMATPASS:
    switch(offset)
    {
      case 0: f=1;  return sizeof(sObject)/4+0;
      case 1: f=1;  return sizeof(sObject)/4+1;
      case 2: return sizeof(sObject)/4+2;
      case 3: return sizeof(sObject)/4+3;
      case 4: f=1;  return sizeof(sObject)/4+7;
    }
    break;

  case sCID_GENMESHANIM:
    switch(offset)
    {
      case 0: return sizeof(sObject)/4+0;
      case 1: return sizeof(sObject)/4+3;
      case 2: return sizeof(sObject)/4+6;
      case 3: return sizeof(sObject)/4+9;
      case 4: return sizeof(sObject)/4+12;
    }
    break;
  case sCID_GENLIGHT:
    switch(offset)
    {
      case 0: return sizeof(sObject)/4+0;
      case 1: return sizeof(sObject)/4+3;
      case 2: return sizeof(sObject)/4+6;
      case 3: return sizeof(sObject)/4+9;
      case 4: return sizeof(sObject)/4+10;
      case 5: return sizeof(sObject)/4+11;
      case 6: return sizeof(sObject)/4+12;
      case 7: return sizeof(sObject)/4+13;
      case 8: return sizeof(sObject)/4+14;
    }
    break;

	case sCID_GENFXCHAIN:
		switch(offset)
		{
		case 0: return sizeof(sObject)/4+0;
		case 1: return sizeof(sObject)/4+4;
		case 2: return sizeof(sObject)/4+6;
		case 3: f=1; return sizeof(sObject)/4+7;
		case 4: f=1; return sizeof(sObject)/4+9;
		case 5: f=1; return sizeof(sObject)/4+12;
		}
  }
  return 0;
}

sInt OffsetSize(sU32 cid,sInt offset)
{
  switch(cid)
  {
  case sCID_GENSCENE:
    return offset<3 ? 3 : 1;
  case sCID_GENMATRIX:
    return 3;
  case sCID_GENMATPASS:
    return offset==3 ? 4 : 1;
  case sCID_GENMESHANIM:
    return offset<4 ? 3 : 1;
  case sCID_GENLIGHT:
    return offset<3 ? 4 : 1;
	case sCID_GENFXCHAIN:
		return "\x04\x02\x01\x02\x03\x03"[offset];
  }
  return 0;
}

sBool DontCache(sU32 cid)
{
  switch(cid)
  {
  case sCID_GENBITMAP:
  case sCID_GENMESH:
    return sFALSE;
  default:
    return sTRUE;
  }
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Codegenerator, 2nd try                                             ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/


ToolCodeSnip *ToolCodeGen::NewSnip()
{
  ToolCodeSnip *snip;

  if(Snips.Count>=SnipsAllocated)
  {
    sVERIFY(Snips.Count==SnipsAllocated);
    Snips.Add();
    Snips[SnipsAllocated] = new ToolCodeSnip;
    snip = Snips[SnipsAllocated++];
  }
  else
  {
    snip = *Snips.Add();
  }

  sSetMem(snip,0,sizeof(*snip));
  snip->Flags = TCS_VALID;

  return snip;
}

void ToolCodeGen::Emit(sChar *format,...)
{
  sFormatString(Source+SourceUsed,SourceAlloc-SourceUsed,format,&format);
  while(Source[SourceUsed]) SourceUsed++;
}

void ToolCodeGen::Emit(ToolCodeSnip *snip)
{
  PageOp *op;
  sInt i,j;
  sInt comma;
  sInt column;

  if(snip->Flags & TCS_LOAD)
  {
    if(snip->OutputCount>1)
    {
      Error = 1;
      return;
    }

    Emit("  load %s;\n",snip->Name);
  }
  else if(snip->Flags & TCS_STORE)
  {
    if(snip->InputCount!=1)
    {
      Error = 1;
      return;
    }

    Emit("  store %s;\n",snip->Name);
  }
  else if(snip->CodeObject && snip->CodeObject->GetClass()==sCID_TOOL_PAGEOP)
  {
    if(snip->OutputCount>1)
    {
      Error = 1;
      return;
    }

    op = (PageOp *) snip->CodeObject;

    column = SourceUsed;
    Emit("  %s(",op->Class->Name);
    comma = 0;
    for(i=0;i<op->Class->ParaCount;i++)
    {
      if(comma)
      {
        Emit(",");
        if(SourceUsed-column>60)
        {
          Emit("\n    ");
          column = SourceUsed-4;
        }
      }
      comma = sTRUE;
      if(op->Data[i].Anim)
      {
        if(op->Class->Para[i].Cycle && sCmpString(op->Class->Para[i].Cycle,"1234")==0)
          Emit("%d",MakeLabel(op->Data[i].Anim));
        else
          Emit("\"%s\"",op->Data[i].Anim);
      }
      else
      {
        switch(op->Class->Para[i].Type)
        {
        case sCT_LABEL:
          comma = sFALSE;
          break;

        case sCT_FIXED:
          if(op->Class->Para[i].Zones<2)
          {
            Emit("%H",op->Data[i].Data[0]);
          }
          else
          {
            Emit("[");
            for(j=0;j<op->Class->Para[i].Zones;j++)
            {
              if(j>0)
                Emit(",");
              Emit("%H",op->Data[i].Data[j]);
            }
            Emit("]");
          }
          break;

        case sCT_INT:
          if(op->Class->Para[i].Zones<2)
          {
            if(sCmpString(op->Class->Para[i].Name,"ocount")==0)
              Emit("%d",snip->FXViewport ? -snip->OriginalOutputCount : snip->OriginalOutputCount);
            else
              Emit("%d",op->Data[i].Data[0]);
          }
          else
          {
            Emit("[");
            for(j=0;j<op->Class->Para[i].Zones;j++)
            {
              if(j>0)
                Emit(",");
              Emit("%d",op->Data[i].Data[j]);
            }
            Emit("]");
          }
          break;

        case sCT_RGBA:
          Emit("#%02x%02x%02x%02x",
            sRange(op->Data[i].Data[3]>>8,255,0),
            sRange(op->Data[i].Data[0]>>8,255,0),
            sRange(op->Data[i].Data[1]>>8,255,0),
            sRange(op->Data[i].Data[2]>>8,255,0));
          break;

        case sCT_RGB:
          Emit("#%02x%02x%02x",
            sRange(op->Data[i].Data[0]>>8,255,0),
            sRange(op->Data[i].Data[1]>>8,255,0),
            sRange(op->Data[i].Data[2]>>8,255,0));
          break;

        case sCT_CYCLE:
        case sCT_CHOICE:
          Emit("%d",op->Data[i].Data[0]);
          break;

        case sCT_MASK:
          Emit("%h",op->Data[i].Data[0]);
          break;

        default:
          sVERIFYFALSE;
          break;
        }
      }
    }
    Emit(");\n");
  }
}

/****************************************************************************/

void ToolCodeGen::EmitR(ToolCodeSnip *snip)
{
  sInt i;
  sInt out,in;

  if(!(snip->Flags & TCS_GENERATED))
  {
    snip->Flags |= TCS_GENERATED;
 
    if(snip->Flags & TCS_ADD)
    {
      for(i=0;i<snip->InputCount;i++)
      {
        EmitR(snip->Inputs[i]);
        if(i>0)
          Emit(snip);
      }
    }
    else
    {
      for(i=0;i<snip->InputCount;i++)
        EmitR(snip->Inputs[i]);
      Emit(snip);
    }
  }

  out = TCG_DISCARD;
  if(snip->Flags & TCS_LOAD)
  {
    if(snip->OutputCID==sCID_GENSCENE || snip->OutputCID==sCID_GENFXCHAIN)
      out = TCG_POINTER;
    else
      out = TCG_KEEP;
  }

  for(i=0;i<snip->OutputCount;i++)
  {
    if(snip->Outputs[i]->CodeObject && snip->Outputs[i]->CodeObject->GetClass()==sCID_TOOL_PAGEOP)
    {
      in = ((PageOp *)snip->Outputs[i]->CodeObject)->Class->InputFlags;
      if((in&0xf0) && snip->Outputs[i]->Inputs[0]!=snip)
        in = in>>4;
      in = in&15;

      if(in==SCFU_MOD && out==TCG_KEEP)
        Emit("  Copy();\n");
      if(in==SCFU_MOD && out==TCG_POINTER)
      {
        Emit("// error here\n");
        Error = 1;
      }
    }
  }
}

void ToolCodeGen::AddR(ToolObject *to,ToolCodeSnip *root)
{
  ToolCodeSnip *snip;
  PageOp *op;
  sVERIFY(App);
  ToolObject *loadto;

  sInt i;

  if(to->Snip==0)
  {
    snip = NewSnip();
    to->Snip = snip;
    snip->CodeObject = to;
    snip->OutputCID = to->CID;

    if(to->GetClass()==sCID_TOOL_PAGEOP)
    {
      op = (PageOp *) to;
      if(op->Error)
        Error = 1;
      if(op->Class->InputFlags & SCFU_MULTI)
        snip->Flags |= TCS_ADD;
      if(op->Class->InputFlags & SCFU_FXALLOC)
        snip->Flags |= TCS_FXALLOC;

      if(op->Class->Flags & POCF_LOAD)
      {
        snip->CodeObject = 0;
        snip->StoreObject = to;
        snip->Flags |= TCS_LOAD;
        sCopyString(snip->Name,op->Data[1].Anim,sizeof(snip->Name));

        loadto = App->FindObject(snip->Name);
        if(loadto)
          AddR(loadto,snip);
        else
          Error = 1;
      }
      if(op->Class->Flags & POCF_STORE)
      {
        snip->CodeObject = 0;
        snip->StoreObject = to;
        snip->Flags |= TCS_STORE;

        if(sCmpString(op->Name,op->Data[1].Anim)!=0)
          Error = 1;
        sCopyString(snip->Name,op->Data[1].Anim,sizeof(snip->Name));
      }

      for(i=0;i<op->InputCount;i++)
      {
        AddR(op->Inputs[i],snip);
      }
    }
    else
    {
      sVERIFYFALSE;
    }
  }

  if(root)
  {
    sVERIFY(root->InputCount<TC_INOUTMAX);
    sVERIFY(to->Snip->OutputCount<TC_INOUTMAX);
    root->Inputs[root->InputCount++] = to->Snip;
    to->Snip->Outputs[to->Snip->OutputCount++] = root;
  }
}

sInt ToolCodeGen::ChangeR(ToolCodeSnip *snip)
{
  sInt i;
  sU32 change,j;

  change = 0;
  if(snip->CodeObject && snip->CodeObject->ChangeCount > change)
    change = snip->CodeObject->ChangeCount;
  if(snip->StoreObject && snip->StoreObject->ChangeCount > change)
    change = snip->StoreObject->ChangeCount;
  for(i=0;i<snip->InputCount;i++)
  {
    j = ChangeR(snip->Inputs[i]);
    if(j>change) 
      change = j;
  }
  if(snip->CodeObject)
  {
    snip->CodeObject->ChangeCount = change;
    if(snip->CodeObject->ChangeCount>=snip->CodeObject->CalcCount)
      snip->CodeObject->Cache = 0;
  }
  if(snip->StoreObject)
  {
    snip->StoreObject->ChangeCount = change;
    if(snip->StoreObject->ChangeCount>=snip->StoreObject->CalcCount)
      snip->StoreObject->Cache = 0;
  }
  return change;
}

/****************************************************************************/
/****************************************************************************/

sMAKEZONE(Compile,"Compile",0xff00ffc0);
sMAKEZONE(CompGraph,"CompGraph",0xffc0ffc0);
sMAKEZONE(CompReal,"CompReal",0xff40ffc0);
sMAKEZONE(Run,"Run",0xff00ffff);

void ToolCodeGen::Init()
{
  sREGZONE(Compile);
  sREGZONE(CompGraph);
  sREGZONE(CompReal);
  sREGZONE(Run);
  Snips.Init(128);
  SnipsAllocated = 0;
  Labels.Init(128);
  SourceUsed = 0;
  SourceAlloc = 0x100000;
  Source = new sChar[SourceAlloc];
  Source[0] = 0;
  Bytecode = 0;
  State = 0;
  System = 0;
  System = SystemTxt;
  App = 0;
  Error = 0;
  Temp = 0;
  Index = 0;
  TimelineCount = 0;
  LabelNames.Init(128);
  MakeLabel("@@@@illegal_label_with_code_0");
}

void ToolCodeGen::Exit()
{
  sInt i;

  for(i=0;i<SnipsAllocated;i++)
    delete Snips[i];

  Snips.Exit();
  Labels.Exit();
  LabelNames.Exit();
  if(Source) delete[] Source;
}

sInt ToolCodeGen::MakeLabel(sChar *name)
{
  sInt i;
  ToolCodeLabelName *l;

  if(name==0 || name[0]==0)
    return 0;

  for(i=1;i<LabelNames.Count;i++)
  {
    if(sCmpString(LabelNames[i].Name,name)==0)
      return i;
  }

  i = LabelNames.Count;
  l = LabelNames.Add();
  sCopyString(l->Name,name,sizeof(l->Name));
  return i;
}

/****************************************************************************/
/****************************************************************************/

void ToolCodeGen::Begin()
{
  sZONE(Compile);
  sVERIFY(State==0);
  sVERIFY(Snips.Count==0);
  State = 1;
  Error = 0;
  Index = FIRSTGLOBAL;
  Temp = 0;
  TimelineCount = 0;

  App->SetStat(2,"code generation failed");
}

void ToolCodeGen::AddResult(ToolObject *to)
{
  sZONE(Compile);
  ToolCodeSnip *snip,*store;

  sVERIFY(State==1);

  if(to->Snip==0)
  {
    AddR(to,0);
  }
  snip = to->Snip;
  if(!(snip->Flags & TCS_STORE))
  {
    store = NewSnip();
    store->Flags |= TCS_STORE;
    to->Snip = store;
    sSPrintF(store->Name,sizeof(store->Name),"_temp%04d",Temp++);
    store->InputCount = 1;
    store->Inputs[0] = snip;
    store->OutputCID = snip->OutputCID;
    snip->OutputCount = 1;
    snip->Outputs[0] = store;
    snip = store;
  }
  snip->Flags |= TCS_ROOT;
  snip->StoreObject = to;
}

void ToolCodeGen::AddTimeline(TimeDoc *td)
{
  sVERIFY(TimelineCount<16);
  Timeline[TimelineCount++] = td;
}

sBool ToolCodeGen::End(ScriptCompiler *sc,sBool addsource,sBool doexport)
{
  sZONE(Compile);
  sInt i,max,j,k,l;
  sInt off,fl;
  ToolCodeSnip *snip,*store,*load,*in;
  ToolObject *to;
  PageOp *op;
  EditDoc *ed;
  TimeDoc *td;
  TimeOp *top;
  AnimChannel *chan;
  AnimKey *key;
  AnimDoc *anim;
  ToolCodeLabel *label;
  sInt splinecount;
  sChar *member;
  sChar memstr[128];
  sInt *vals;
  sF32 *fvals;

// prepare

  {
    sZONE(CompReal);
    sVERIFY(State==1);
    State = 2;
    SourceUsed = 0;
    splinecount = 0;
    Labels.Count = 0;
    if(Error)
      return sFALSE;
    sc->Reset();
    if(!sc->Parse(System,"system.txt")) 
      return sFALSE;
  }

// changes

  {
    sZONE(CompGraph);

    max = Snips.Count;
    for(i=0;i<max;i++)
    {
      snip = Snips[i];
      if((snip->Flags & TCS_VALID) && (snip->Flags & TCS_ROOT))
        ChangeR(snip);
    }

// find ops that need changes

    max = Snips.Count;
    for(i=0;i<max;i++)
    {
      snip = Snips[i];

      for(j=0;j<snip->InputCount;j++)
      {
        if(snip->Inputs[j]->OutputCID != snip->OutputCID)
          if(!DontCache(snip->Inputs[j]->OutputCID))
            snip->Inputs[j]->NeedsCache = 1;
      }
      if(snip->CodeObject && snip->CodeObject->GetClass()==sCID_TOOL_PAGEOP)
      {
        op = (PageOp *) snip->CodeObject;
        if(op->Showed)
          snip->NeedsCache = 1;
        if(op->Edited)
        {
          for(j=0;j<snip->InputCount;j++)
            if(!DontCache(snip->Inputs[j]->OutputCID))
              snip->Inputs[j]->NeedsCache = 1;
        }
      }
    }

// use cached objects

    if(!doexport)
    {
      max = Snips.Count;
      for(i=0;i<max;i++)
      {
        snip = Snips[i];
        if(snip->Flags & TCS_VALID)
        {
          to = snip->CodeObject;
          if(to && !DontCache(to->CID) && to->CalcCount > to->ChangeCount && to->Cache)       // use cached object
          {
            snip->Flags = TCS_VALID|TCS_CACHED|TCS_LOAD;
            snip->StoreObject = to;
            snip->CodeObject = 0;
            snip->InputCount = 0;
            sCopyString(snip->Name,to->Name,sizeof(snip->Name));
            if(!snip->Name[0])
              sSPrintF(snip->Name,sizeof(snip->Name),"_temp%04d",Temp++);
          }
          else    // generate code to store cached object
          {
            to = snip->StoreObject;
            if(to && !DontCache(to->CID) && to->CalcCount > to->ChangeCount && to->Cache && (snip->Flags & TCS_STORE))
            {
              store = NewSnip();
              store->Flags = TCS_VALID|TCS_CACHED|TCS_LOAD;
              store->StoreObject = to;
              sSPrintF(store->Name,sizeof(store->Name),"_temp%04d",Temp++);
              store->Outputs[0] = snip;
              store->OutputCount = 1;

              snip->Inputs[0] = store;
              snip->InputCount = 1;
            }
          }
        }
      }
    }
 
// split multiple outputs to store -> load,load
// also, insert store for "needscache"

    max = Snips.Count;
    for(i=0;i<max;i++)
    {
      snip = Snips[i];
      snip->OriginalOutputCount = snip->OutputCount;
      if(snip->CodeObject==0)
        snip->NeedsCache=0;

      if((snip->OutputCount>1 || snip->NeedsCache) && !(snip->Flags&TCS_STORE) && (snip->Flags&TCS_VALID))
      {
        store = NewSnip();
        store->Flags |= TCS_STORE;
        if(snip->NeedsCache)
        {
          store->Flags |= TCS_ROOT;
          store->StoreObject = snip->CodeObject;
        }
        store->OutputCID = snip->OutputCID;
        if(store->Name[0]==0)
          sSPrintF(store->Name,sizeof(store->Name),"_temp%04d",Temp++);
        
        for(j=0;j<snip->OutputCount;j++)
        {
          in = snip->Outputs[j];
          store->Outputs[j] = in;

          for(k=0;k<in->InputCount;k++)       // loads are inserted in next step!
            if(in->Inputs[k]==snip)
              in->Inputs[k] = store;
        }
        store->OutputCount = snip->OutputCount;
        store->OriginalOutputCount = 1;
        store->InputCount = 1;
        store->Inputs[0] = snip;
        snip->OutputCount = 1;
        snip->Outputs[0] = store;
      }
    }

// insert loads after store

    max = Snips.Count;
    for(i=0;i<max;i++)
    {
      if(Snips[i]->Flags&TCS_STORE)
      {
        snip = Snips[i];
        
        for(j=0;j<snip->OutputCount;j++)
        {
          in = snip->Outputs[j];

          if(!(in->Flags & TCS_LOAD))
          {
            load = NewSnip();
            load->Flags |= TCS_LOAD;
            sCopyString(load->Name,snip->Name,sizeof(load->Name));
            load->OutputCID = snip->OutputCID;
            load->InputCount = 1;
            load->Inputs[0] = snip;
            load->OutputCount = 1;
            load->OriginalOutputCount = 1;
            load->Outputs[0] = in;
            for(k=0;k<in->InputCount;k++)
              if(in->Inputs[k]==snip)
                in->Inputs[k] = load;
          }
        }
      }
    }

// mark FXViewport 

    max = Snips.Count;
    for(i=0;i<max;i++)
    {
      snip = Snips[i];
      if((snip->Flags&TCS_ROOT) && (snip->Flags&TCS_VALID) && !(snip->Flags&TCS_GENERATED))
      {
        for(;;)
        {
          if(snip->OutputCID!=sCID_GENFXCHAIN)
            break;
          snip->FXViewport = 1;
          if(snip->Flags & TCS_FXALLOC)
            break;
          if(snip->InputCount==0)
            break;
          snip = snip->Inputs[0];
        }
      }
    }

// create global variables

    max = Snips.Count;
    for(i=0;i<max;i++)
    {
      snip = Snips[i];
      if((snip->Flags&(TCS_STORE|TCS_CACHED)) && (snip->Flags&TCS_VALID))
      {
        Emit("%s %s; // global #%d",sc->GetClassName(snip->OutputCID),snip->Name,Index);
        if(snip->Flags&TCS_CACHED)
          Emit(", load from cache");
        Emit("\n");
        snip->Index = Index++;
      }
    }

// generate code

    Emit("void OnGenerate()\n{\n");

    max = Snips.Count;
    for(i=0;i<max;i++)
    {
      snip = Snips[i];
      if((snip->Flags&TCS_ROOT) && (snip->Flags&TCS_VALID) && !(snip->Flags&TCS_GENERATED))
      {
        if(snip->OutputCID==sCID_GENFXCHAIN)
          Emit("  FXResetAlloc();\n");
        EmitR(snip);
      }
    }
    Emit("}\n");

// find splines and labels

    for(i=0;i<TimelineCount;i++)
    {
      td = Timeline[i];
      td->SortOps();
      for(j=0;j<td->Ops->GetCount();j++)
      {
        anim = td->Ops->Get(j)->Anim;
        if(anim)
          anim->Temp = -1;
        anim->Update();
      }
    }

    for(i=0;i<TimelineCount;i++)
    {
      td = Timeline[i];
      for(j=0;j<td->Ops->GetCount();j++)
      {
        top = td->Ops->Get(j);
        anim = top->Anim;
        if(anim && anim->Temp==-1)            // visit every animation only once!
        {
          anim->Temp = 0;
          for(k=0;k<anim->Channels.Count;k++)
          {
            chan = &anim->Channels[k];
            if(chan->Target)
            {
              for(l=0;l<Labels.Count;l++)
              {
                label = &Labels.Get(l);
                if(sCmpString(label->Name,chan->Name)==0 && sCmpString(label->RootName,anim->RootName)==0)
                  goto foundlabel;
              }
              label = Labels.Add();
              label->Name = chan->Name;
              label->Root = chan->Target;
              label->RootName = anim->RootName;
              Emit("%s %s_%s;\n",sc->GetClassName(label->Root->GetClass()),label->RootName,label->Name);
  foundlabel:;
              chan->Temp = splinecount++;
              Emit("spline Spline_%d;\n",chan->Temp);
            }
          }
        }
      }
    }

// generate timeline_oninit

    for(i=0;i<TimelineCount;i++)
    {
      td = Timeline[i];
      Emit("void %s_OnInit()\n{\n",td->Name);

      for(j=0;j<td->Ops->GetCount();j++)
      {
        top = td->Ops->Get(j);
        anim = top->Anim;
        if(anim && anim->Temp==0 && anim->Root && anim->Root->Cache)
        {
          anim->Temp = 1;
          for(k=0;k<anim->Channels.Count;k++)
          {
            chan = &anim->Channels[k]; 
            if(chan->Target)
            {
              Emit("  Spline(%d,%h,%h);\n",chan->Count,chan->Tension,chan->Continuity);
              for(l=0;l<anim->Keys.Count;l++)
              { 
                key = &anim->Keys[l];
                if(key->Channel == k)
                {
                  static sChar *splineformat[] =
                  {
                    "  SplineKey1(%H,%H);\n",
                    "  SplineKey2(%H,[%H,%H]);\n",
                    "  SplineKey3(%H,[%H,%H,%H]);\n",
                    "  SplineKey4(%H,[%H,%H,%H,%H]);\n"
                  };
                  Emit(splineformat[chan->Count-1],
                    key->Time,
                    key->Value[0]&0xffffff00,
                    key->Value[1]&0xffffff00,
                    key->Value[2]&0xffffff00,
                    key->Value[3]&0xffffff00);
                }
              }
              Emit("  store Spline_%d;\n",chan->Temp);
            }
          }
        }
      }

      for(j=0;j<Labels.Count;j++)
      {
        label = &Labels.Get(j);
        Emit("  load %s; FindLabel(%d); store %s_%s;\n",label->RootName,MakeLabel(label->Name),label->RootName,label->Name);
      }

      Emit("}\n");
    }

// generate timeline_onframe

    for(i=0;i<TimelineCount;i++)
    {
      td = Timeline[i];
      Emit("void %s_OnFrame()\n{\n",td->Name);
      Emit("  int time;\n");
      Emit("  int t0;\n");
      Emit("  int t1;\n");

      for(j=0;j<td->Ops->GetCount();j++)
      {
        top = td->Ops->Get(j);
        anim = top->Anim;
        if(anim==0 || anim->Root==0 || anim->Root->Cache==0)
          continue;
        Emit("  t0=%h;t1=%h;if((SystemBeat>=t0) & (SystemBeat<t1))\n",top->x0,top->x1);
        Emit("  {\n");
        Emit("    time = (SystemBeat-t0)/(t1-t0);\n");
        for(k=0;k<anim->Channels.Count;k++)
        {
          chan = &anim->Channels[k];
          if(chan->Target)
          {
            static sChar *splinecalc[] = 
            {
              "    load Spline_%d; %s = %s + SplineCalc%d(time);\n",
              "    load Spline_%d; %s = %s * SplineCalc%d(time);\n",
              "    load Spline_%d; %s = %s + (SplineCalc%d(time)*%h);\n",
              "    load Spline_%d; %s = %s * (SplineCalc%d(time)*%h);\n",
              "    load Spline_%d; %s = %s + SplineCalc%d(SystemMouseX);\n",
              "    load Spline_%d; %s = %s + SplineCalc%d(SystemMouseY);\n",
            };
            member = OffsetName(chan->Target->GetClass(),chan->Offset);
            if(!member)
            {
              Error = 1;
              member = "unknown_member";
            }
            sSPrintF(memstr,sizeof(memstr),"%s_%s.%s",anim->RootName,chan->Name,member);
            switch(chan->Mode)
            {
            case 0:
            case 1:
            case 2:
            case 3:
              Emit(splinecalc[chan->Mode],chan->Temp,memstr,memstr,chan->Count,top->Velo);
              break;
            case 4:
            case 5:
            case 6:
            case 7:
              Emit("    %s = %s %c %h;\n",memstr,memstr,(chan->Mode&1)?'*':'+',(chan->Mode%2)?top->Mod:top->Velo);
              break;
            case 8:
            case 9:
            case 10:
              l = (chan->Mode-8)*3;
              Emit("    %s = [%h,%h,%h];\n",memstr,
                sFtol(top->SRT[l]*65536.0f),
                sFtol(top->SRT[l+1]*65536.0f),
                sFtol(top->SRT[l+2]*65536.0f));
              break;
            case 11:
            case 12:
            case 13:
              l = (chan->Mode-11)*3;
              Emit("    load Spline_%d; %s = SplineCalc3(time)*[%h,%h,%h]*%h;\n",
                chan->Temp,memstr,
                sFtol(top->SRT[l]*65536.0f),
                sFtol(top->SRT[l+1]*65536.0f),
                sFtol(top->SRT[l+2]*65536.0f),top->Velo);
              break;
            case 14:
              Emit(splinecalc[4],chan->Temp,memstr,memstr,chan->Count,top->Velo);
              break;
            case 15:
              Emit(splinecalc[5],chan->Temp,memstr,memstr,chan->Count,top->Velo);
              break;
            case 16:
              Emit("    %s = \"%s\";\n",memstr,top->Text);
              break;
            }
          }
        }
        if(anim->PaintRoot)
        {
          switch(anim->Root->Cache->GetClass())
          {
          case sCID_GENFXCHAIN:
            Emit("    load %s; RenderChain();\n",anim->RootName);
            break;
          case sCID_GENSCENE:
  //          Emit("    NewFxChain(); load %s; Viewport([0,0,0],[0,0,0],0,0,3); RenderChain();\n",anim->RootName);
            Emit("    load %s; PaintSceneLater(); drop;\n",anim->RootName);
            break;
          }
        }
        Emit("  }\n");
      }
      for(j=0;j<td->Ops->GetCount();j++)
      {
        top = td->Ops->Get(j);
        anim = top->Anim;
        if(anim==0 || anim->Root==0 || anim->Root->Cache==0)
          continue;
        for(k=0;k<anim->Channels.Count;k++)
        {
          chan = &anim->Channels[k];
          if(chan->Target)
          {
            static sChar *clearanim[] =
            {
              " ",
              "  %s = %H;\n",
              "  %s = [%H,%H];\n",
              "  %s = [%H,%H,%H];\n",
              "  %s = [%H,%H,%H,%H];\n",
            };
            member = OffsetName(chan->Target->GetClass(),chan->Offset);
            if(member)
            {
              sSPrintF(memstr,sizeof(memstr),"%s_%s.%s",anim->RootName,chan->Name,member);
              off = AdressName(chan->Target->GetClass(),chan->Offset,fl);
              if(fl)
              {
                fvals = ((sF32 *)chan->Target)+off;
                Emit(clearanim[chan->Count],memstr,sFtol(fvals[0]*65536),sFtol(fvals[1]*65536),sFtol(fvals[2]*65536),sFtol(fvals[3]*65536));
              }
              else
              {
                vals = ((sInt *)chan->Target)+off;
                Emit(clearanim[chan->Count],memstr,vals[0],vals[1],vals[2],vals[3]);
              }
            }
            else
            {
              Error = 1;
            }
          }
        }
      }

      Emit("}\n");
    }


// compile

    if(Error)
      return sFALSE;
    if(SourceUsed>=SourceAlloc)
      return sFALSE;

    if(addsource)
    {
      for(i=0;i<App->Docs->GetCount() && !Error;i++)
      {
        ed = (EditDoc *) App->Docs->Get(i);
        if(ed->GetClass()==sCID_TOOL_EDITDOC)
        {
          Emit(ed->Text);
        }
      }
    }
  }

  {
    sZONE(CompReal);
    Source[SourceUsed++] = 0;
//    sDPrint(Source);
    if(!sc->Parse(Source,"source.txt"))
    {
      App->SetStat(2,sc->GetErrorPos());
      return sFALSE;
    }

    sc->Optimise();
    Bytecode = sc->Generate();

    if(Bytecode==0)
    {
      App->SetStat(2,sc->GetErrorPos());
      return sFALSE;
    }
    App->SetStat(2,"compilation successfull");
  }

  return sTRUE;
}

sBool ToolCodeGen::Execute(ScriptRuntime *sr)
{
  sZONE(Run);
  sInt i,max;
  ToolCodeSnip *snip;

// load program

  sVERIFY(State==2);
  sr->Load(Bytecode);

// load cached objects

  max = Snips.Count;
  for(i=0;i<max;i++)
  {
    snip = Snips[i];
    if(snip->Index && snip->StoreObject)
    {
      if(snip->Flags & (TCS_CACHED))
      {
        sr->SetGlobal(snip->Index,(sInt)snip->StoreObject->Cache);
      }
    }
  }

// execute

  FXMaster->ResetAlloc();
  if(!sr->Execute(4))
  {
    sChar buffer[512];
    sSPrintF(buffer,512,"execution failed: %s",sr->GetErrorMessage());
    App->SetStat(2,buffer);
    //App->SetStat(2,"execution failed");
    return sFALSE;
  }

// store results

  max = Snips.Count;
  for(i=0;i<max;i++)
  {
    snip = Snips[i];
    if(snip->Index && snip->StoreObject)
    {
      if(snip->Flags & TCS_STORE)
      {
        snip->StoreObject->Cache = (sObject *)sr->GetGlobal(snip->Index);
        snip->StoreObject->CalcCount = App->CheckChange();
      }
    }
  }

  return sTRUE;
}

void ToolCodeGen::Cleanup()
{
  sZONE(Compile);
  sInt i;

  sVERIFY(State==2);
  State = 0;

  for(i=0;i<Snips.Count;i++)
  {
    if(Snips[i]->CodeObject)
      Snips[i]->CodeObject->Snip = 0;
    if(Snips[i]->StoreObject)
      Snips[i]->StoreObject->Snip = 0;
  }
  Snips.Count = 0;

  sGui->GarbageCollection();
}
