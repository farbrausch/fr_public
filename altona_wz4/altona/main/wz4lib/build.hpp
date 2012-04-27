/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) by Dierk Ohlerich                                    ***/
/***   all rights reserverd                                               ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WERKKZEUG4_BUILD_HPP
#define FILE_WERKKZEUG4_BUILD_HPP

#ifndef __GNUC__
#pragma once
#endif

#include "base/types2.hpp"
#include "doc.hpp"

class ScriptCompiler;
class ScriptContext;

/****************************************************************************/

struct wNode
{
  wOp *Op;                        // each node has to represent an op.
  wOp *ScriptOp;                  // copy script from this op. Used for subroutine argumtent injection
  wNode **Inputs;
  wType *OutType;                 // real output type. Some ops specify "AnyType" as output.
  sInt InputCount;
  sInt FakeInputCount;            // parameter only inputs
  sInt CycleCheck;
  sInt CallId;                    // this node is part of a subroutine call
  sPoolString LoopName;           // for injecting loop counters with a fake op
  sF32 LoopValue;
  sInt LoopFlag;                  // inside call or loop (possibly multiple different results for the same op)

  sInt OutputCount;               // used internally to find out good cache points
  sU8 StoreCache;                 // while execution, store cache
  sU8 LoadCache;                  // do no execute op, just load cache
  sU8 Visited;                    // used during recursion (OptimizeCacheR)
  wCommand *StoreCacheDone;       // use the store cache!
};

struct wBuilderPush
{
  wNode *CallInputs;
  wOp *CallOp;
  sArray<wNode *> FakeInputs;      // for call and loop
  sInt CurrentCallId;
  sInt LoopFlag;

  void GetFrom(wBuilder *);
  void PutTo(wBuilder *);
};

class wBuilder
{
  friend struct wBuilderPush;

  sMemoryPool *Pool;
  wNode *Root;
  wNode *MakeNode(sInt ic,sInt fc=0);
  const sChar *MakeString(const sChar *str1,const sChar *str2=0,const sChar *str3=0);
  wNode *ParseR(wOp *op,sInt recursion);
  void OptimizeCacheR(wNode **node);
  wCommand *OutputR(wExecutive &,wNode *node);
  wNode *SkipToSlowR(wNode *node);
  wCommand *MakeCommand(wExecutive &exe,wOp *op,wCommand **inputs,sInt inputcount,wOp *scriptop,wOp *d0,const sChar *d1,sInt callid,sInt fakeinputcount);
  void Error(wOp *op,sChar *text);
  sInt Errors;
  void rssall(wNode *node,sInt flag);

  wNode *CallInputs;
  wOp *CallOp;
  sArray<wNode *> FakeInputs;      // for call and loop
  sInt CallId;
  sInt CurrentCallId;
  sInt TypeCheckOnly;
  sInt LoopFlag;


  struct RecursionData_
  {
    sArray<wOp *> inputs;
    sEndlessArray<sInt> inputloop;
    RecursionData_() : inputloop(-1) {}
  };
  sEndlessArray<RecursionData_ *> RecursionData; // the new and delete in the recursion are very costly, especially with debug runtime. We can reuse the arrays!
public:
  wBuilder();
  ~wBuilder();

  sBool Parse(wOp *root);
  sBool Optimize(sBool Cache);
  sBool TypeCheck();
  sBool Output(wExecutive &);
  void SkipToSlow(sBool honorslow);

  sBool Check(wOp *root);
  sBool Depend(wExecutive &exe,wOp *root);
  wObject *Execute(wExecutive &,wOp *root,sBool honorslow,sBool progress);
  wObject *FindCache(wOp *root);

  sArray<wNode *> AllNodes;
};

/****************************************************************************/

#endif // FILE_WERKKZEUG4_BUILD_HPP

