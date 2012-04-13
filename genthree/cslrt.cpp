// This file is distributed under a BSD license. See LICENSE.txt for details.
#include "cslrt.hpp"
void ProgressBar(sInt done);

/****************************************************************************/
/****************************************************************************/

sChar *ScriptRuntimeError;
class ScriptRuntime *ScriptRuntimeInterpreter;

ScriptRuntime::ScriptRuntime()
{
  XVar = new sInt[RT_MAXGLOBAL+RT_MAXLOCAL*RT_MAXCALLS];
  Bytecode = 0;
  UserWords = new sInt[RT_MAXUSER];
  CodeCalls = new ScriptRuntimeCode[RT_MAXCODE];

  sSetMem(XVar,0,(RT_MAXGLOBAL+RT_MAXLOCAL*RT_MAXCALLS)*4);
  sSetMem(UserWords,0,RT_MAXUSER*4);
  sSetMem(CodeCalls,0,RT_MAXCODE*sizeof(ScriptRuntimeCode));
  sSetMem(IStack,0,sizeof(IStack));
  sSetMem(OStack,0,sizeof(OStack));
  sSetMem(RStack,0,sizeof(RStack));

  XVarIndex = RT_MAXGLOBAL;
  IIndex = 0;
  OIndex = 0;
  RIndex = 0;
  Error = 0;
  ShowProgress = 0;
#if !sINTRO || !sRELEASE
  ErrorMsg = "generic error";
#endif
}

/****************************************************************************/

ScriptRuntime::~ScriptRuntime()
{
  delete[] XVar;
  delete[] UserWords;
//  delete[] CodeWords;
  delete[] CodeCalls;
}

/****************************************************************************/

void ScriptRuntime::Tag()
{
  sU32 *code;

  if(Bytecode)
  {
    code = BrokerRoots;

#if sINTRO_X
    sInt i;
    sBroker->Need((sObject *)XVar[5]);
    for(i=8;i<0x180;i++)
      sBroker->Need((sObject *)XVar[i]);
#else
    while(*code!=~0)
      sBroker->Need((sObject *)XVar[*code++]);
#endif
  }
}

/****************************************************************************/

void ScriptRuntime::Load(sU32 *code)
{
  sInt i;
  sInt ifstack[256];
  sInt ifindex;
  sU32 word;

  sSetMem(UserWords,0,RT_MAXUSER*4);
  Bytecode = code;
  BytecodeSize = 0;
  if(Bytecode==0)
    return;

  while(*code!=RTC_EOF)
  {
    if(((*code)&RTC_CMDMASK)==RTC_FUNC)
      UserWords[(*code)&(~RTC_CMDMASK)]=code-Bytecode;
    code++;
  }
  BytecodeSize = code-Bytecode;
  code++;
  BrokerRoots = code;
  while(*code!=~0) 
    code++;
  BytecodeStrings = (sChar *) (code+2);

  ifindex = 0;
  for(i=0;i<BytecodeSize;i++)
  {
    word = Bytecode[i];

    switch(word&RTC_CMDMASK)
    {
    case RTC_FUNC:
    case RTC_RTS:
      sVERIFY(ifindex==0);
      break;
    case RTC_IF:
      ifstack[ifindex] = i;
      ifindex++;
      break;
    case RTC_ELSE:
      ifindex--;
      Bytecode[ifstack[ifindex]] |= i+1;
      ifstack[ifindex] = i;
      ifindex++;
      break;
    case RTC_THEN:
      ifindex--;
      Bytecode[ifstack[ifindex]] |= i+1;
      break;
    case RTC_DO:
      ifstack[ifindex+0] = i;
      ifindex+=2;
      break;
    case RTC_WHILE:
      ifindex-=2;
      ifstack[ifindex+1] = i;
      ifindex+=2;
      break;
    case RTC_REPEAT:
      ifindex-=2;
      Bytecode[i] |= ifstack[ifindex+0];
      Bytecode[ifstack[ifindex+1]] |= i+1;
      break;
    } 
  }
}

/****************************************************************************/
/*
void ScriptRuntime::AddCode(sInt word,ScriptHandler handler)
{
  CodeWords[word] = handler;
}
*/
void ScriptRuntime::AddCode(sInt word,sU32 flags,void *code)
{
  CodeCalls[word].Code = code;
  *(sU32 *)&CodeCalls[word].Ints = flags;
}

/****************************************************************************/

sBool ScriptRuntime::Execute(sInt word)
{
#if !sINTRO || !sRELEASE
  sInt oldpc;
#endif
  XVarIndex = RT_MAXGLOBAL;
//  OVarIndex = RT_MAXGLOBAL;
  IIndex = 0;
  OIndex = 0;
  RIndex = 0;
  Error = 0;
#if !sINTRO || !sRELEASE
  ErrorMsg = "generic error";
#endif

  ScriptRuntimeInterpreter = this;
  PC = UserWords[word];
  RStack[RIndex++] = 0;
  while(PC && !Error)
  {
#if !sINTRO || !sRELEASE
    oldpc = PC;
#endif
    Interpret();
  }
#if !sINTRO || !sRELEASE
  if(Error)
    sDPrintF("execution error PC = %d,code = %08x,desc = %s\n",oldpc,Bytecode[oldpc],ErrorMsg);
  else
  {
    if(IIndex!=0)
      sDPrintF("unbalanced IStack = %d\n",IIndex);
    if(OIndex!=0)
      sDPrintF("unbalanced OStack = %d\n",OIndex);
  }
#endif

#if !sINTRO || !sRELEASE
  if(!Error)
    ErrorMsg = "no error";
#endif

  return !Error;
}

/****************************************************************************/

static sInt CallCode(sInt code,sInt *para,sInt count)
{
  sInt result;

  _asm
  {
    mov eax,code
    mov esi,para
    mov ecx,count
    sub esp,ecx
    sub esp,ecx
    sub esp,ecx
    sub esp,ecx
    mov edi,esp
    rep movsd
    call eax
    mov result,eax
  }

  return result;
}

/****************************************************************************/

void ScriptRuntime::Interpret()
{
  sU32 code;
  sInt *obj;
  sInt val;
  sInt imm;
  sInt i,count;
  sInt para[32];
  sInt *ptr;

  if(PC<0 || PC>BytecodeSize)
  {
    Error = 1;
    return;
  } 
  code = Bytecode[PC++];
  if(code&RTC_IMMMASK)
  {
    PushI(code&0x7fffffff | ((code&0x40000000)<<1));
  }
  else if(code&RTC_CMDMASK)
  {
    imm = code&~RTC_CMDMASK;
    if(imm>=BytecodeSize)
      Error = 1;

    switch((code&RTC_CMDMASK)>>24)
    {
    case RTC_IF>>24:
    case RTC_WHILE>>24:
      val = PopI();
      if(!val)
        PC = imm;
      break;
    case RTC_ELSE>>24:
    case RTC_REPEAT>>24:
      PC = imm;
      break;
    case RTC_THEN>>24:
    case RTC_DO>>24:
      break;

    case RTC_CODE>>24:
      if(imm<RT_MAXCODE)
      {
/*
        if(CodeWords[imm])
        {
          (*CodeWords[imm])(this);
          if(ScriptRuntimeError)
            Error=1;
        }
        else */
#if sINTRO
        sVERIFY(CodeCalls[imm].Code || imm==0x56 || imm==0x57);
#endif
        if(CodeCalls[imm].Code)
        {
#if sINTRO
          if(ShowProgress)
            ProgressBar(0);
#endif
          count = 0;
          i=CodeCalls[imm].Objects;
          while(i)
          {
            i--;
            *(sObject **)(para+count+i) = PopO();
          }
          count+=CodeCalls[imm].Objects;
          i=CodeCalls[imm].Ints;
          while(i)
          {
            i--;
            *(sInt*)(para+count+i) = PopI();
          }
          count+=CodeCalls[imm].Ints;
#if !sINTRO || !sRELEASE
          ScriptRuntimeError = 0;
          if(!Error)
#endif
          {
            i=CallCode((sInt)CodeCalls[imm].Code,para,count);
#if !sINTRO || !sRELEASE
            if(ScriptRuntimeError)
            {
              Error = 1;
              ErrorMsg = ScriptRuntimeError;
            }
            else
#endif
            {
              if(CodeCalls[imm].Return == 1)
                PushI(i);
              if(CodeCalls[imm].Return == 2)
                PushO((sObject *)i);
            }
          }
        }
      }
      else
        Error = 1;
      break;
    case RTC_USER>>24:
      if(imm<RT_MAXUSER && UserWords[imm] && RIndex<RT_MAXCALLS)
      {
        RStack[RIndex++] = PC;
        XVarIndex += RT_MAXLOCAL;
//        OVarIndex += RT_MAXLOCAL;
        PC = UserWords[imm];
      }
      break;
    case RTC_FUNC>>24:
      break;

    case RTC_STRING>>24:
      PushI((sInt)(BytecodeStrings+imm));
      break;

    case RTC_STORELOCI>>24:
    case RTC_STORELOCO>>24:
    case RTC_FETCHLOCI>>24:
    case RTC_FETCHLOCO>>24:
#if !sINTRO
      if(imm>=RT_MAXLOCAL)
      {
        Error = 1;
        return;
      }
#endif
      ptr = &XVar[imm+XVarIndex];
      goto fetchstore;

    case RTC_STOREGLOI>>24:
    case RTC_STOREGLOO>>24:
    case RTC_FETCHGLOI>>24:
    case RTC_FETCHGLOO>>24:
#if !sINTRO
      if(imm>=RT_MAXGLOBAL)
      {
        Error = 1;
        return;
      }
#endif
      ptr = &XVar[imm];
      goto fetchstore;

    case RTC_STOREMEMO>>24:
    case RTC_FETCHMEMO>>24:
      Error = 1;
      break;

    case RTC_STOREMEMI>>24:
    case RTC_FETCHMEMI>>24:
#if !sINTRO
      if(imm>=RT_MAXGLOBAL)
      {
        Error = 1;
        return;
      }
#endif
      obj = (sInt *)PopO();
#if !sINTRO
      if(obj==0)
      {
        Error = 1;
        break;
      }
#endif
      ptr = &obj[imm+sizeof(sObject)/4];

fetchstore:
      switch((code>>24)&3)
      {
      case 0: // store int
        *ptr = PopI();
        break;
      case 1: // store obj
        *ptr = (sInt)PopO();
        break;
      case 2: // fetch int
        PushI(*ptr);
        break;
      case 3: // fecth obj
        PushO((sObject *)*ptr);
        break;
      }
      break;

    case RTC_NOP>>24:
      break;
    case RTC_RTS>>24:
#if sINTRO
      XVarIndex -= RT_MAXLOCAL;
      PC = RStack[--RIndex];
#else
      if(RIndex>0)
      {
        XVarIndex -= RT_MAXLOCAL;
//        OVarIndex -= RT_MAXLOCAL;
        PC = RStack[--RIndex];
      }
      else
      {
        Error = 1;
      }
#endif
      break;
    case RTC_I31>>24:
      IStack[IIndex] ^= 0x80000000;
//      PushI(PopI()^0x80000000);
      break;
    case RTC_ADD>>24:
      imm = PopI();
      IStack[IIndex] += imm;
//      PushI(PopI()+imm);
      break;
    case RTC_SUB>>24:
      imm = PopI();
      IStack[IIndex] -= imm;
//      PushI(PopI()-imm);
      break;
    case RTC_MUL>>24:
      imm = PopI();
      IStack[IIndex] = sMulShift(IStack[IIndex],imm);
//      PushI(sMulShift(PopI(),imm));
      break;
    case RTC_DIV>>24:
      imm = PopI();
#if !sINTRO
      if(imm!=0)
      {
        IStack[IIndex] = sDivShift(IStack[IIndex],imm);
      }
      else
      {
        sDPrintF("division by zero\n");
        PushI(0);
      }
#else
      IStack[IIndex] = sDivShift(IStack[IIndex],imm);
#endif
      break;
    case RTC_MOD>>24:
      imm = PopI();
#if !sINTRO
      if(imm!=0)
      {
        IStack[IIndex] = IStack[IIndex]%imm;
//        PushI(imm2%imm);
      }
      else
      {
        sDPrintF("division by zero\n");
        PushI(0);
      }
#else
      IStack[IIndex] = IStack[IIndex]%imm;
#endif
      break;
    case RTC_MIN>>24:
      imm = PopI();
      IStack[IIndex] = sMin(IStack[IIndex],imm);
      break;
    case RTC_MAX>>24:
      imm = PopI();
      IStack[IIndex] = sMax(IStack[IIndex],imm);
      break;
    case RTC_AND>>24:
      imm = PopI();
      IStack[IIndex] &= imm;
      break;
    case RTC_OR>>24:
      imm = PopI();
      IStack[IIndex] |= imm;
      break;
    case RTC_SIN>>24:
      IStack[IIndex] = sFSin(IStack[IIndex]*sPI2F/0x10000)*0x10000;
//      PushI(sFSin(PopI()*sPI2F/0x10000)*0x10000);
      break;
    case RTC_COS>>24:
      IStack[IIndex] = sFCos(IStack[IIndex]*sPI2F/0x10000)*0x10000;
//      PushI(sFCos(PopI()*sPI2F/0x10000)*0x10000);
      break;
    case RTC_GE>>24:
      imm = PopI();
      IStack[IIndex] = (IStack[IIndex] >= imm);
//      PushI(PopI()>=imm);
      break;
    case RTC_LE>>24:
      imm = PopI();
      IStack[IIndex] = (IStack[IIndex] <= imm);
//      PushI(PopI()<=imm);
      break;
    case RTC_GT>>24:
      imm = PopI();
      IStack[IIndex] = (IStack[IIndex] > imm);
//      PushI(PopI()>imm);
      break;
    case RTC_LT>>24:
      imm = PopI();
      IStack[IIndex] = (IStack[IIndex] < imm);
//      PushI(PopI()<imm);
      break;
    case RTC_EQ>>24:
      imm = PopI();
      IStack[IIndex] = (IStack[IIndex] == imm);
//      PushI(PopI()==imm);
      break;
    case RTC_NE>>24:
      imm = PopI();
      IStack[IIndex] = (IStack[IIndex] != imm);
//      PushI(PopI()!=imm);
      break;
    case RTC_DUP>>24:
      IStack[IIndex+1] = IStack[IIndex];
      IIndex++;
//      imm = PopI();
//      PushI(imm);
//      PushI(imm);
      break;
    case RTC_F2I>>24:
//      imm = PopI();
      IStack[IIndex] = (*(sF32 *)&IStack[IIndex])*65536;
//      PushI();
      break;
    case RTC_I2F>>24:
      *(sF32*)&IStack[IIndex] = IStack[IIndex]/65536.0f;
//      PushI(imm);
      break;
    }
  }
}

/****************************************************************************/
/****************************************************************************/

#if !sINTRO

void ScriptRuntime::PushI(sInt val)
{
  if(IIndex<RT_MAXSTACK)
    IStack[++IIndex] = val;
  else
    Error = 1;
}

void ScriptRuntime::PushO(sObject *o)
{
#if !sINTRO
  sVERIFY(o);
#endif

  if(OIndex<RT_MAXSTACK)
    OStack[++OIndex] = o;
  else
    Error = 1;
}

sInt ScriptRuntime::PopI()
{
  if(IIndex>0)
  {
    return IStack[IIndex--];
  }
  else
  {
    Error = 1;
    return 0;
  }
}

sF32 ScriptRuntime::PopX()
{
  if(IIndex>0)
  {
    return IStack[IIndex--]/65536.0f;
  }
  else
  {
    Error = 1;
    return 0;
  }
}

sChar *ScriptRuntime::PopS()
{
  return (sChar *)PopI();
}

sObject *ScriptRuntime::PopO()
{
  if(OIndex>0)
  {
    return OStack[OIndex--];
  }
  else
  {
    Error = 1;
    return 0;
  }
}

sObject *ScriptRuntime::PopO(sU32 cid)
{
  sObject *o;

  o = 0;
  if(OIndex>0)
  {
    o = OStack[OIndex--];
    if(o==0)
    {
#if !sINTRO
      sDPrintF("null-object on stack, expected cid %08x\n",cid);
#endif
      Error = 1;
    }
    else if(o->GetClass()!=cid)
    {
#if !sINTRO
      sDPrintF("unexpected Class %08x instead of %08x\n",o->GetClass(),cid);
#endif
      Error = 1;
      o = 0;
    }
  }
  if(o==0)
    Error = 1;
  return o;
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Packed Format                                                      ***/
/***                                                                      ***/
/****************************************************************************/

// header
// 0x00 offset to code
// 0x04 offset to function table
// 0x08 offset to immediate data
// 0x0c function count
// 0x10 broker roots
// 0x14 string start oggset
// 0x18 variable index offset

// 0x.. offset to immediate data for each function

// function table entry
// 0x00 info-word
// 0x04 ordinal number (later it's the function pointer)

// code: byte per command, followed by
// 0x40 = command byte (command ordinal word)
// 0x41 = command ordinal word
// 0x43 = word offset
// 0x5x = word offset
// 0x80 = (immediate from stream)


/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

#if !sINTRO

#define MAXIMM 65536
#define MAXCODE 65536
#define MAXALL (128*1024)
static sU32 PImm[MAXIMM];
static sU8 PCode[MAXCODE];
static sU8 PAll[MAXALL];
static sU8 PVar[MAXIMM];

sU8 *ScriptRuntime::PackedExport(sInt &size)
{
  sU32 *scan;
  sU32 cmd,val;
  sU8 *code;
  sU8 *pvar;
  sU32 *imm;
  sInt dest;
  sInt i;
  sInt ordercode[512];
  sInt ordercodei[256];
  sInt countcode;

  code = PCode;
  imm = PImm;
  scan = Bytecode;
  pvar = PVar;
  sSetMem(ordercode,0,sizeof(ordercode));
  countcode = 1;

  do
  {
    cmd = *scan++;
    val = cmd & 0x00ffffff;
    switch(cmd>>24)
    {
    case RTC_CODE>>24:
      if(ordercode[val]==0)
      {
        ordercodei[countcode] = val;
        ordercode[val] = countcode;
        countcode++;
      }
      *code++ = cmd>>24;
      *code++ = ordercode[val];
//      sVERIFY(ordercode[val]<0x40);
//       *code++ = 0x40+ordercode[val];
      break;
    case RTC_USER>>24:
      *code++ = cmd>>24;
      *code++ = cmd>>8;
      *code++ = cmd;
      break;
    case RTC_FUNC>>24:
      *code++ = cmd>>24;
      *code++ = cmd>>8;
      *code++ = cmd;
      break;
    case RTC_STRING>>24:
      *code++ = cmd>>24;
      *code++ = cmd>>8;
      *code++ = cmd;
      break;
    case RTC_NOP>>24:
      break;

    default:
      if(cmd>=0x30000000 && cmd<0x40000000)
      {
        *code++ = cmd>>24;
        *pvar++ = cmd;
        *pvar++ = cmd>>8;
        sVERIFY((cmd&0x00ffffff)<1024);
      }
      else if(cmd>=0x80000000)
      {
        *code++ = 0;
        *imm++ = (cmd&0x7fffffff | ((cmd&0x40000000)<<1));
      }
      else
      {
        *code++ = cmd>>24;
      }
      break;
    }
  }
  while(cmd!=RTC_EOF);

  ((sU32 *)PAll)[0] = 0x00; // code
  ((sU32 *)PAll)[1] = 0x00; // ftable
  ((sU32 *)PAll)[2] = 0x00; // immediate
  ((sU32 *)PAll)[3] = 0x00; // function count
  ((sU32 *)PAll)[4] = 0x00; // broker roots
  ((sU32 *)PAll)[5] = 0x00; // string start offset
  ((sU32 *)PAll)[6] = 0x00; // variable index
  dest = 0x1c;

  ((sU32 *)PAll)[0] = dest; // code
  sCopyMem(PAll+dest,PCode,code-PCode);
  dest += (code-PCode+3)&(~3);
 
  ((sU32 *)PAll)[2] = dest; // immediate
  sCopyMem(PAll+dest,PImm,(imm-PImm)*4);
  dest += (imm-PImm)*4;

  ((sU32 *)PAll)[3] = countcode; // function count
  ((sU32 *)PAll)[1] = dest; // ftable
  for(i=0;i<countcode;i++)
  {
    *(sU32 *)(PAll+dest) = ordercodei[i];
    dest+=4;
    *(sU32 *)(PAll+dest) = 0;
    dest+=4;
  } 

  ((sU32 *)PAll)[4] = dest; // broker roots
  while(*scan!=~0)
  {
//    *(sU32 *)(PAll+dest) = *scan++;
//    dest+=4;
    scan++;
  }
//  *(sU32 *)(PAll+dest) = *scan++;
//  dest+=4;
    scan++;

  ((sU32 *)PAll)[6] = dest;
  sCopyMem(PAll+dest,PVar,pvar-PVar);
  dest += (pvar-PVar);

  ((sU32 *)PAll)[5] = dest; // string start offset
  sCopyMem(PAll+dest,scan+1,*scan);
  dest += *scan;

  size = dest;
  return PAll;
}
#endif

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

static sU32 PByteCode[64*1024];

void ScriptRuntime::PackedImport(sU8 *data)
{
  sU32 *dest;
  sU32 *hdr;
  sU32 *imm;
  sU32 *codetable;
  sU8 *src;
  sU32 cmd;
  sInt i;
  sInt ifstack[256];
  sInt ifindex;
  sU32 word;
  sU8 *pvar;
  sU32 val;


  dest = PByteCode;
  hdr = (sU32 *)data;

  Bytecode = PByteCode;
  src = data+hdr[0];
  codetable = (sU32 *)(data+hdr[1]);
  imm = (sU32 *)(data+hdr[2]);
  pvar = (sU8 *)(data+hdr[6]);
//  BrokerRoots = (sU32 *) (data+hdr[4]);
  BytecodeStrings = (sChar *) (data+hdr[5]);

  *dest++=0;
  do
  {
    cmd = (*src++)<<24;
    switch(cmd>>24)
    {
    case RTC_CODE>>24:
      cmd |= codetable[src[0]*2+0];
      src++;
      break;
    case RTC_USER>>24:
      cmd |= src[0]<<8;
      cmd |= src[1];
      src+=2;
      break;
    case RTC_FUNC>>24:
      cmd |= src[0]<<8;
      cmd |= src[1];
      src+=2;
      break;
    case RTC_STRING>>24:
      cmd |= src[0]<<8;
      cmd |= src[1];
//      sVERIFY(src[0]==0);
      src+=2;
      break;
    case RTC_NOP:
      val = *imm++;
/*
      if(val & 1) 
        val ^= 0xfffffffe;
      val = (val>>1) | (val<<31);
      */
      cmd = (val)|0x80000000;
      break;

    default:
      if(cmd>=0x30000000 && cmd<0x40000000)
      {
        cmd |= *pvar++;
        cmd |= (*pvar++)<<8;
      } 
      else if(cmd>0x40000000 && cmd<0x80000000)
      {
        cmd = RTC_CODE|codetable[(cmd>>24)-0x40];
      }
      break;
    }
    *dest++ = cmd;
  }
  while(cmd!=RTC_EOF);

  BytecodeSize = dest-Bytecode;



  ifindex = 0;
  for(i=0;i<BytecodeSize;i++)
  {
    word = Bytecode[i];

    switch(word&RTC_CMDMASK)
    {
    case RTC_FUNC:
      UserWords[(word)&(~RTC_CMDMASK)]=i;
      sVERIFY(ifindex==0);
      break;
    case RTC_RTS:
      sVERIFY(ifindex==0);
      break;
    case RTC_IF:
      ifstack[ifindex] = i;
      ifindex++;
      break;
    case RTC_ELSE:
      ifindex--;
      Bytecode[ifstack[ifindex]] |= i+1;
      ifstack[ifindex] = i;
      ifindex++;
      break;
    case RTC_THEN:
      ifindex--;
      Bytecode[ifstack[ifindex]] |= i+1;
      break;
    case RTC_DO:
      ifstack[ifindex+0] = i;
      ifindex+=2;
      break;
    case RTC_WHILE:
      ifindex-=2;
      ifstack[ifindex+1] = i;
      ifindex+=2;
      break;
    case RTC_REPEAT:
      ifindex-=2;
      Bytecode[i] |= ifstack[ifindex+0];
      Bytecode[ifstack[ifindex+1]] |= i+1;
      break;
    } 
  }

}

/****************************************************************************/
/****************************************************************************/
/* // not needed any more?
void ScriptRuntime::PushIV(sInt *v,sInt count)
{
  if(IIndex+count<=RT_MAXSTACK)
  {
    while(count-->0)
      IStack[IIndex++] = *v++;
  }
  else
    Error = 1;
}

void ScriptRuntime::PopIV(sInt *v,sInt count)
{
  sInt i;
  if(IIndex>=count)
  {
    IIndex-=count;
    for(i=0;i<count;i++)
      v[i] = IStack[IIndex+i];
  }
  else
  {
    for(i=0;i<count;i++)
      v[i] = 0;
    Error = 1;
  }
}

void ScriptRuntime::PopXV(sF32 *v,sInt count)
{
  sInt i;
  if(IIndex>=count)
  {
    IIndex-=count;
    for(i=0;i<count;i++)
      v[i] = IStack[IIndex+i]/65536.0f;
  }
  else
  {
    for(i=0;i<count;i++)
      v[i] = 0;
    Error = 1;
  }
}

void ScriptRuntime::PopOV(sObject **v,sInt count)
{
  sInt i;
  if(OIndex>=count)
  {
    OIndex-=count;
    for(i=0;i<count;i++)
      v[i] = OStack[OIndex+i];
  }
  else
  {
    for(i=0;i<count;i++)
      v[i] = 0;
    Error = 1;
  }
}
*/
/****************************************************************************/
/****************************************************************************/
