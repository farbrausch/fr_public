/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) by Dierk Ohlerich                                    ***/
/***   all rights reserverd                                               ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "build.hpp"
#include "gui/gui.hpp"      // for notify
#include "wz4lib/basic_ops.hpp"
#include "wz4lib/script.hpp"

/****************************************************************************/
/****************************************************************************/

wBuilder::wBuilder() : RecursionData(0)
{
  Pool = new sMemoryPool(0x10000);
  CallInputs = 0;
  CallOp = 0;
  CallId = 1;
  CurrentCallId = 0;
  TypeCheckOnly = 0;
}

wBuilder::~wBuilder()
{
  delete Pool;
  for(sInt i=0;i<RecursionData.GetCount();i++)
    delete RecursionData[i];
}

const sChar *wBuilder::MakeString(const sChar *str1,const sChar *str2,const sChar *str3)
{
  sInt len1 = sGetStringLen(str1);
  sInt len2 = 0;
  sInt len3 = 0;
  if(str2)
    len2 = sGetStringLen(str2);
  if(str3)
    len3 = sGetStringLen(str3);

  sChar *buffer = Pool->Alloc<sChar>(len1+len2+len3+1);
  sChar *d = buffer;

  for(sInt i=0;i<len1;i++)
    *d++ = str1[i];
  for(sInt i=0;i<len2;i++)
    *d++ = str2[i];
  for(sInt i=0;i<len3;i++)
    *d++ = str3[i];
  *d++ = 0;

  return buffer;
}

wNode *wBuilder::MakeNode(sInt ic,sInt fc)
{
  wNode *node = Pool->Alloc<wNode>(1);
  sClear(*node);
  node->InputCount = ic;
  node->FakeInputCount = ic+fc;
  node->Inputs = Pool->Alloc<wNode *>(ic+fc);
  node->LoopName = L"";
  node->LoopFlag = LoopFlag;
  for(sInt i=0;i<ic+fc;i++)
    node->Inputs[i] = 0;
  return node;
}

void wBuilder::Error(wOp *op,sChar *text)
{
  if(!op->CalcErrorString)
    op->CalcErrorString = text;
  Errors++;
}

/****************************************************************************/
/****************************************************************************/

void wBuilderPush::GetFrom(wBuilder *b)
{
  CallInputs = b->CallInputs;
  CallOp = b->CallOp;
  FakeInputs = b->FakeInputs;
  CurrentCallId = b->CurrentCallId;
  LoopFlag = b->LoopFlag;
}

void wBuilderPush::PutTo(wBuilder *b)
{
  b->CallInputs = CallInputs;
  b->CallOp = CallOp;
  b->FakeInputs = FakeInputs;
  b->CurrentCallId = CurrentCallId;
  b->LoopFlag = LoopFlag;
}

sBool wBuilder::Parse(wOp *root)
{
  Pool->Reset();
  AllNodes.Clear();
  Errors = 0;
  CallInputs = 0;
  CallOp = 0;
  CurrentCallId = 0;
  LoopFlag = 0;
  sVERIFY((CallId&0x80000000)==0)   // hope two billion is enough. if this fails, we have to reset all callid's everywhere. would not be that bad.

  while(root && root->Bypass)
    root = root->Inputs.GetCount()>0 ? root->Inputs[0] : 0;

  if(!root)
    return 0;
  Root = ParseR(root,0);

  return Errors == 0;
}

wNode *wBuilder::ParseR(wOp *op,sInt recursion)
{
  wOpInputInfo *info;
  wOp *op2;

  // pre checks

  sFORALL(op->Conversions,op2)
    op2->ConversionOrExtractionUsed = 0;
  sFORALL(op->Extractions,op2)
    op2->ConversionOrExtractionUsed = 0;

  if(op->BuilderNode && op->BuilderNodeCallId==CurrentCallId) // evaluate only once (per subroutine call, if any)
    return op->BuilderNode;

  if(!TypeCheckOnly && CallOp && op->Cache && op->Cache->CallId!=CurrentCallId)         // when in soubroutine, clear all caches
  {
    op->Conversions.Clear();      // also, don't reuse conversions. conversions are reused during one call, but not across builds.
    op->Extractions.Clear();
    op->Cache->CallId = 0;        // make sure subroutines are recalculated
    op->BuilderNodeCallerId = 0;
    sRelease(op->Cache);
  }

  if(op->CycleCheck!=0) 
  {
    Error(op,L"cyclic connection");
    return 0;
  }

  op->CycleCheck++;

  // is it a good op?

  if(op->ConnectError)
    Error(op,L"connect error");

  // allocate temp memory for inputs

  if(RecursionData[recursion]==0)
    RecursionData[recursion] = new RecursionData_;
  RecursionData_ *rd = RecursionData[recursion];
  sArray<wOp *> &inputs = rd->inputs;
  inputs.Clear();
  sEndlessArray<sInt> &inputloop = rd->inputloop;
  inputloop.Clear();
  inputs.HintSize(op->Inputs.GetCount()+op->Links.GetCount());

  // map inputs: inputs, links and defaults
  
  sInt index = 0;
  sInt lastlink = -1;
  sInt max = op->Inputs.GetCount();
  sBool choose = 0;
  sFORALL(op->Links,info)
  {
    if((op->Class->Inputs[_i].Flags & wCIF_METHODMASK)==wCIF_METHODCHOOSE)
      choose = 1;
    sBool optional = op->Class->Inputs[_i].Flags & wCIF_OPTIONAL;
    wOp *in = 0;
    switch(info->Select)
    {
    case 0:
      if(index<max)
        in = op->Inputs[index++];
      break;
    case 1:
      in = info->Link;
      lastlink = inputs.GetCount();
      break;
    case 2:
      in = 0;
      lastlink = inputs.GetCount();
      break;
    default:
      if(info->Select>=3 && info->Select<3+max)
        in = op->Inputs[info->Select-3];
      lastlink = inputs.GetCount();
      break;
    }
    info->DefaultUsed = 0;
    if(info->Default && !in)
    {
      in = info->Default;
      info->DefaultUsed = 1;
    }

    inputs.AddTail(in);

    if(!optional && in==0)
      Error(op,L"required input is missing");
    if(op->Class->Inputs[_i].Flags & wCIF_WEAK && in)
      in->WeakOutputs.AddTail(op);
  }

  // map inputs: append varargs

  if(op->Class->Flags & wCF_VARARGS)
  {
    while(index<max)
    {
      wOp *in = op->Inputs[index++];
      inputs.AddTail(in);
    }
  }
  else if(!choose)
  {
    if(inputs.GetCount() != op->Class->Inputs.GetCount())
      Error(op,L"too few inputs");
    if(index!=max)
      Error(op,L"too many inputs");
  }

  // perform loop op

  for(sInt i=lastlink+1;i<inputs.GetCount();i++)
  {
    if(inputs[i] && inputs[i]->Class->Flags & wCF_LOOP)
    {
      wOp *mul = inputs[i];
      inputloop[i] = 0;
      sInt max = inputs[i]->EditU()[0];
      if(TypeCheckOnly)
        max = 1;
      for(sInt j=1;j<max;j++)
      {
        inputs.AddAfter(mul,i++);
        inputloop[i] = j;
      }
    }
  }

  // notify op

  op->ConnectionMask = 0;
  for(sInt i=0;i<inputs.GetCount() && i<32;i++)
    if(inputs[i])
      op->ConnectionMask |= 1<<i;

  // create node and parse inputs

  wNode *node = 0;
  if(op->Class->Flags & wCF_CALL) // call subroutine node
  {
    sInt flags = op->EditU()[0];

    // first evaluate inputs

    node = MakeNode(inputs.GetCount());
    node->CallId = CurrentCallId;
    node->ScriptOp = op;
    for(sInt i=1;i<inputs.GetCount();i++)
    {
      if(inputs[i]==0)
        node->Inputs[i] = 0;
      else
        node->Inputs[i] = ParseR(inputs[i],recursion+1);
    }

    // next evaluate subroutine

    wBuilderPush push;
    push.GetFrom(this);

    CallInputs = node;
    if(!(flags & 1))
      FakeInputs.AddTail(node);
    CallOp = op;
    if(op->BuilderNodeCallerId)
      CurrentCallId = op->BuilderNodeCallerId;
    else
      CurrentCallId = op->BuilderNodeCallerId = CallId++;


    if(inputs[0])
    {
      node = MakeNode(1);
      LoopFlag = 1;
      node->Op = op;
      node->CallId = CurrentCallId;
      node->Inputs[0] = ParseR(inputs[0],recursion+1);
      if(node->Inputs[0])
        node->OutType = node->Inputs[0]->OutType;
      else
        Error(op,L"something wierd");
    }

    push.PutTo(this);
  }
  else if(op->Class->Flags & wCF_INPUT) // handle Input op (for subroutine calls)
  {
    sInt n = op->EditU()[0]+1;

    if(CallInputs==0 || CallOp==0)
    {
      Error(op,L"input node not in subroutine call");
    }
    else if(n>=CallInputs->InputCount  || CallInputs->Inputs[n]==0)
    {
      Error(CallOp,L"call has too few inputs");
    }
    else
    {
      node = MakeNode(1,FakeInputs.GetCount());
      node->Op = op;
      node->CallId = CurrentCallId;
      node->ScriptOp = op;
//      node->OutType = CallOp->Class->OutputType;
      node->Inputs[0] = CallInputs->Inputs[n];
      sVERIFY(node->Inputs[0])
      node->OutType = node->Inputs[0]->OutType;
      wNode *fn;
      sFORALL(FakeInputs,fn)
        node->Inputs[1+_i] = fn;
    }
  }
  else if(op->Class->Flags & wCF_ENDLOOP)
  {
    wBuilderPush push;
    push.GetFrom(this);

    CallInputs = 0;
    FakeInputs.Clear();
    CallOp = 0;
    CurrentCallId = 0;
    LoopFlag = 0;

    if(inputs[0])
    {
      node = MakeNode(1);
      node->Op = op;
      node->CallId = CurrentCallId;
      node->Inputs[0] = ParseR(inputs[0],recursion+1);
      if(node->Inputs[0])
        node->OutType = node->Inputs[0]->OutType;
      else
        Error(op,L"something wierd");
    }

    push.PutTo(this);
  }
  else 
  {
    if(op->Class->Flags & wCF_LOOP)
    {
      Error(op,L"unexpected loop op");
    }

    node = MakeNode(inputs.GetCount(),FakeInputs.GetCount());
    node->Op = op;
    node->CallId = CurrentCallId;
    node->ScriptOp = op;
    node->OutType = op->Class->OutputType;
    op->BuilderNode = node;
    op->BuilderNodeCallId = CurrentCallId;
    AllNodes.AddTail(node);

    for(sInt i=0;i<inputs.GetCount();i++)
    {
      if(inputs[i]==0)
        node->Inputs[i] = 0;
      else
      {
        if(inputloop.Get(i)>=0)
        {
          sVERIFY(inputs[i]->Class->Flags & wCF_LOOP)
          if(inputs[i]->Inputs.GetCount()>0 && inputs[i]->Inputs[0])
          {
            wBuilderPush push;
            push.GetFrom(this);
            LoopFlag = 1;

            CallInputs = 0;
            CallOp = op;
            if(op->BuilderNodeCallerId)
            {
              op->BuilderNodeCallerId;
            }
            else
            {
              op->BuilderNodeCallerId = CallId;
              CallId += inputs[i]->EditU()[0];
            }
            CurrentCallId = op->BuilderNodeCallerId + inputloop.Get(i);


            if(inputs[i]->EditStringCount>0 && inputs[i]->EditString[0])
            {
              wNode *fake = MakeNode(0,0);
              sString<256> buffer;
              buffer = inputs[i]->EditString[0]->Get();
              sMakeLower(buffer);
              fake->LoopName = sPoolString(buffer);     // this might be a bottleneck!
              fake->LoopValue = inputloop.Get(i);
              FakeInputs.AddTail(fake);
            }
            node->Inputs[i] = ParseR(inputs[i]->Inputs[0],recursion+1);

            push.PutTo(this);
          }
          else
          {
            Error(op,L"required input of loop missing");
          }
        }
        else
        {
          node->Inputs[i] = ParseR(inputs[i],recursion+1);
        }
      }
    }
    if(node->OutType==AnyTypeType && inputs.GetCount()>0 && node->Inputs[0] && (node->Op->Class->Flags & wCF_TYPEFROMINPUT))
      node->OutType = node->Inputs[0]->OutType;
    for(sInt i=node->InputCount;i<node->FakeInputCount;i++)
      node->Inputs[i] = FakeInputs[i-node->InputCount];
  }


  // done

  op->CycleCheck--;

  return node;
}

/****************************************************************************/

sBool wBuilder::TypeCheck()
{
  wNode *node;
  wOp *op,*in;

  sFORALL(AllNodes,node)
  {
    op = node->Op;
    if(op && !node->LoadCache)
    {
      sInt max = op->Class->Inputs.GetCount();
        
      for(sInt i=0;i<node->InputCount;i++)
      {
        if(node->Inputs[i])
        {
          in = node->Inputs[i]->Op;
          if(in && !node->Inputs[i]->OutType->IsType(op->Class->Inputs[sMin(i,max-1)].Type))
            Error(op,L"input has wrong type");
        }
      }
      op->CheckedByBuild = 1;
    }
  }
  return Errors==0;
}

/****************************************************************************/
/****************************************************************************/

void wBuilder::OptimizeCacheR(wNode **node)
{
  if((*node)->Visited!=1)
  {
    wOp *op = (*node)->Op;
    if(op)
    {
      if(op->Cache && op->Cache->CallId==(*node)->CallId)
      {
        *node = MakeNode(0);
        (*node)->Op = op;
        (*node)->LoadCache = 1;
        (*node)->OutType = op->Cache->Type;
        (*node)->CallId = op->Cache->CallId;
        op->BuilderNode = *node;
        op->BuilderNodeCallId = op->Cache->CallId;
        op->CacheLRU = Doc->CacheLRU++;
        AllNodes.AddTail(*node);
      }
      else if(!((op->Class->Flags & wCF_PASSOUTPUT) && (*node)->OutputCount==1 && !op->ImportantOp)) // reasons not to cache
      {
        if (!Doc->IsPlayer) 
          (*node)->StoreCache = 1;
      }
    }
    (*node)->Visited = 1;

    for(sInt i=0;i<(*node)->FakeInputCount;i++)
    {
      if((*node)->Inputs[i])
        OptimizeCacheR(&((*node)->Inputs[i]));
    }
  }
}

sBool wBuilder::Optimize(sBool cache)
{
  wNode *node;
  wNode *in;

  sVERIFY(Root);

  // remove nops

  sFORALL(AllNodes,node)
  {
    for(sInt i=0;i<node->InputCount;i++)
    {
      in = node->Inputs[i];
      if(in)
      {
        wNode *cycle = in;
        while(in && in->Op && in->Op->Class->Command==0 
          && !(in->Op->Class->Flags & wCF_CALL) && in->InputCount==1 
          && !(in->ScriptOp && in->ScriptOp->ScriptSourceValid))
        {
          in = in->Inputs[0];
          if(in==cycle)
          {
            Error(in->Op,L"cyclic connection");
            break;
          }
        }
      }
      node->Inputs[i] = in;
    }
  }

  if(Errors)
    return 0;

  in = Root;
  while(in->Op->Class->Command==0 
    && !(in->Op->Class->Flags & wCF_CALL) && in->InputCount==1
    && !(in->ScriptOp && in->ScriptOp->ScriptSourceValid))
  {
    in = in->Inputs[0];
  }
  Root = in;

  // insert conversions. could be interleaved with typecheck, but i want typecheck checking the conversions

  sFORALL(AllNodes,node)
  {
    if(node->Op)
    {
      sInt max = node->Op->Class->Inputs.GetCount();
      for(sInt i=0;i<node->InputCount;i++)
      {
        in = node->Inputs[i];
        if(in && in->Op)
        {
          wType *reqtype = node->Op->Class->Inputs[sMin(i,max-1)].Type;
          if(!in->OutType->IsType(reqtype))
          {
            wOp *convop = in->Op->MakeConversionTo(reqtype,node->CallId);
            if(convop)
            {
              wNode *newnode = convop->BuilderNode;
              if(!newnode)
              {
                newnode = MakeNode(1);
                newnode->Inputs[0] = in;
                newnode->Op = convop;
                newnode->ScriptOp = convop;
                newnode->OutType = convop->Class->OutputType;
                newnode->CallId = node->CallId;
                convop->BuilderNode = newnode;
                convop->BuilderNodeCallId = node->CallId;
                AllNodes.AddTail(newnode);
              }
              node->Inputs[i] = newnode;
            }
          }
        }
      }
    }
  }

  // count outputs

  sFORALL(AllNodes,node)
    for(sInt i=0;i<node->InputCount;i++)
      if(node->Inputs[i])
        node->Inputs[i]->OutputCount++;

  // add loadcache and storecache

  if(cache)
  {
    OptimizeCacheR(&Root);
  }

  return 1;
}

/****************************************************************************/
/****************************************************************************/

wCommand *wBuilder::MakeCommand(wExecutive &exe,wOp *op,wCommand **inputs,sInt inputcount,wOp *scriptop,wOp *dummy,const sChar *dummy1,sInt callid,sInt fakeinputcount)
{
  wCommand *cmd = exe.MemPool->Alloc<wCommand>();
  cmd->Init();

  cmd->Op = op;
  cmd->Code = op ? op->Class->Command : 0;
  cmd->CallId = callid;
  if(op && (op->Class->Flags & wCF_PASSINPUT) && 1)
    cmd->PassInput = 0;
  if(op && op->Strobe)
    sGui->Notify(op->Strobe);
  if(scriptop && scriptop->ScriptSourceValid)
  {
    sTextBuffer tb;
    scriptop->MakeSource(tb);
    cmd->ScriptSource = MakeString(tb.Get());
    cmd->Script = scriptop->GetScript();
    cmd->ScriptBind2 = scriptop->Class->Bind2Para;
  }
  
  // data

  if(op)
  {
    cmd->DataCount = op->Class->ParaWords;
    cmd->Data = exe.MemPool->Alloc<sU32>(cmd->DataCount);
    sCopyMem(cmd->Data,op->EditData,cmd->DataCount*sizeof(sU32));
  }

  // strings

  if(op)
  {
    cmd->StringCount = op->Class->ParaStrings;
    cmd->Strings = exe.MemPool->Alloc<const sChar *>(cmd->StringCount);
    for(sInt i=0;i<cmd->StringCount;i++)
    {
      sInt len = op->EditString[i]->GetCount();
      sChar *dest = exe.MemPool->Alloc<sChar>(len+1);
      cmd->Strings[i] = dest;
      sCopyMem(dest,op->EditString[i]->Get(),sizeof(sChar)*(len+1));
    }
  }

  // array

  if(op)
  {
    sInt words = op->Class->ArrayCount;
    sInt elems = op->ArrayData.GetCount();
    if(words>0 && elems>0)
    {
      sU32 *ptr = exe.MemPool->Alloc<sU32>(words * elems);
      cmd->ArrayCount = elems;
      cmd->Array = ptr;
      for(sInt i=0;i<elems;i++)
      {
        sCopyMem(ptr,op->ArrayData[i],words*4);
        ptr += words;
      }
    }
  }

  // inputs

  cmd->InputCount = inputcount;
  cmd->FakeInputCount = fakeinputcount;
  cmd->Inputs = exe.MemPool->Alloc<wCommand *>(cmd->FakeInputCount);
  for(sInt i=0;i<cmd->FakeInputCount;i++)
    cmd->Inputs[i] = inputs[i];

  exe.Commands.AddTail(cmd);

  return cmd;
}

wCommand *wBuilder::OutputR(wExecutive &exe,wNode *node)
{
  sVERIFY(node);
  sVERIFY(node->LoadCache==0);

  sInt ic = 0;
  sInt fc = 0;
  wCommand **objs = 0;

  if(node->CycleCheck)
  {
    if(node->Op)
      Error(node->Op,L"cyclic connection");
  }
  else
  {
    node->CycleCheck++;

    ic = node->InputCount;
    fc = node->FakeInputCount;
    objs = sALLOCSTACK(wCommand *,fc);

    for(sInt i=0;i<fc;i++)
    {
      objs[i] = 0;
      if(node->Inputs[i])
      {
        if(node->Inputs[i]->LoadCache)
        {
          sVERIFY(node->Inputs[i]->Op->Cache);
          wCommand *lc = exe.MemPool->Alloc<wCommand>();
          lc->Init();
          lc->Output = node->Inputs[i]->Op->Cache;
          lc->OutputVarCount = node->Inputs[i]->Op->CacheVars.GetCount();
          lc->OutputVars = exe.MemPool->Alloc<wScriptVar>(lc->OutputVarCount);
          for(sInt j=0;j<lc->OutputVarCount;j++)
            lc->OutputVars[j] = node->Inputs[i]->Op->CacheVars[j];
          objs[i] = lc;
        }
        else
        {
          if(!node->Inputs[i]->StoreCacheDone)
            node->Inputs[i]->StoreCacheDone = OutputR(exe,node->Inputs[i]);
          objs[i] = node->Inputs[i]->StoreCacheDone;
        }

        sVERIFY(objs[i]);
      }
    }

    node->CycleCheck--;
  }

  wCommand *cmd = MakeCommand(exe,node->Op,objs,ic,node->ScriptOp,0,0,node->CallId,fc);
  cmd->LoopName = node->LoopName;
  cmd->LoopValue = node->LoopValue;
  cmd->LoopFlag = node->LoopFlag;
  sVERIFY(cmd);
  if(node->StoreCache)
    cmd->StoreCacheOp = node->Op;
  return cmd;
}

sBool wBuilder::Output(wExecutive &exe)
{
  wCommand *cmd;

  Errors = 0;
  exe.Commands.Clear();
  exe.MemPool->Reset();
  OutputR(exe,Root);

  // addref if everything went right

  sFORALL(exe.Commands,cmd)
  {
    for(sInt i=0;i<cmd->InputCount;i++)
    {
      if(cmd->Inputs[i]) 
      {
        if(cmd->Inputs[i]->Output)
          cmd->Inputs[i]->Output->AddRef();
        else
          cmd->Inputs[i]->OutputRefs++;
      }
    }
  }

  // done

  return Errors==0;
}


/****************************************************************************/

wNode *wBuilder::SkipToSlowR(wNode *node)
{
  wNode *result = 0;

  // return first slow node, 
  // search deep first, then siblings

  if(node->CycleCheck)
  {
    if(node->Op)
      Error(node->Op,L"cyclic connection");
  }
  else
  {
    node->CycleCheck++;

    sInt ic = node->InputCount;
    for(sInt i=0;i<ic && !result;i++)
      if(node->Inputs[i])
        result = SkipToSlowR(node->Inputs[i]);

    node->CycleCheck--;
  }

  // are we a slow node? then return first input!
  // remember, deep first search, so this check is AFTER the recursion

  if(result == 0 && node->Op && (node->Op->Class->Flags & wCF_SLOW) && node->InputCount>0 && node->Inputs[0])
    return node->Inputs[0];
  return result;
}

void wBuilder::rssall(wNode *node,sInt flag)
{
  if(node->CycleCheck)
  {
    if(node->Op)
      Error(node->Op,L"cyclic connection");
  }
  else
  {
    node->CycleCheck++;

    sInt ic = node->InputCount;
    for(sInt i=0;i<ic;i++)
      if(node->Inputs[i])
        rssall(node->Inputs[i],flag);
    node->Op->SlowSkipFlag = flag;

    node->CycleCheck--;
  }
}

void wBuilder::SkipToSlow(sBool honorslow)
{
  if(honorslow)
  {
    wNode *node = SkipToSlowR(Root);
    if(node)
    {
      rssall(Root,1);
      Root = node;
    }
  }
  rssall(Root,0);
}

/****************************************************************************/

sBool wBuilder::Check(wOp *root)
{
  sBool result = 0;
  wNode *node;
  TypeCheckOnly = 1;

  if(!Parse(root)) goto ende;
  if(!Optimize(0)) goto ende;
  if(!TypeCheck()) goto ende;
  result = 1;

ende:
  sFORALL(AllNodes,node)
  {
    if(node->Op)
    {
      node->Op->BuilderNode = 0;
//      node->Op->BuilderNodeCallId = 0;
      node->Op->CycleCheck = 0;
    }
  }

  return result;
}

wObject *wBuilder::Execute(wExecutive &exe,wOp *root,sBool honorslow,sBool progress)
{
  wNode *node;
  exe.Commands.Clear();
  exe.MemPool->Reset();
  wObject *result = 0;
  TypeCheckOnly = 0;

  if(!Parse(root)) goto ende;
  if(!Optimize(1)) goto ende;
  if(!TypeCheck()) goto ende;
  SkipToSlow(honorslow);
  if(Root->LoadCache)             // bypass execution when loading directly
  {
    result = Root->Op->Cache;
    sVERIFY(result);
    result->AddRef();
  }
  else
  {
    if(!Output(exe)) goto ende;
    if(exe.Commands.GetCount()>0)
      result = exe.Execute(progress);
  }

ende:

  exe.Commands.Clear();
  sFORALL(AllNodes,node)
  {
    if(node->Op)
    {
      node->Op->BuilderNode = 0;
//      node->Op->BuilderNodeCallId = 0;
      node->Op->CycleCheck = 0;
      node->Op->ConversionOrExtractionUsed = 1;
    }
  }
  sFORALL(AllNodes,node)
  {
    if(node->Op)
    {
      sRemFalse(node->Op->Conversions,&wOp::ConversionOrExtractionUsed);
      sRemFalse(node->Op->Extractions,&wOp::ConversionOrExtractionUsed);
    }
  }
  return result;
}

sBool wBuilder::Depend(wExecutive &exe,wOp *root)
{
  wNode *node;
  exe.Commands.Clear();
  exe.MemPool->Reset();
  sBool result = 0;
  TypeCheckOnly = 0;

  if(!Parse(root)) goto ende;
  if(!Optimize(1)) goto ende;
  if(!TypeCheck()) goto ende;
  if(!Root->LoadCache)
  {
    if(!Output(exe)) goto ende;
    if(exe.Commands.GetCount()>0)
      result = (exe.Execute(0,1)!=0);
  }

ende:

  exe.Commands.Clear();
  sFORALL(AllNodes,node)
  {
    if(node->Op)
    {      
      node->Op->BuilderNode = 0;
//      node->Op->BuilderNodeCallId = 0;
      node->Op->CycleCheck = 0;
    }
  }

  return result;
}

wObject *wBuilder::FindCache(wOp *root)
{
  wNode *node;
  wObject *result = 0;

  if(!Parse(root)) goto ende;
  if(!Optimize(1)) goto ende;
  if(!TypeCheck()) goto ende;
  if(Root->LoadCache)             // bypass execution when loading directly
  {
    result = Root->Op->Cache;
    sVERIFY(result);
    result->AddRef();
  }

ende:

  sFORALL(AllNodes,node)
  {
    if(node->Op)
    {
      node->Op->BuilderNode = 0;
//      node->Op->BuilderNodeCallId = 0;
      node->Op->CycleCheck = 0;
    }
  }

  return result;
}

/****************************************************************************/
