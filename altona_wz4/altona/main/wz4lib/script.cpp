/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) by Dierk Ohlerich                                    ***/
/***   all rights reserved                                                ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "wz4lib/script.hpp"
#include "base/math.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Helpers                                                            ***/ 
/***                                                                      ***/
/****************************************************************************/

static sF32 ExpEase(sF32 x)
{
  if(x < 1.0f)
    return x - (1.0f - sFExp(-x));
  else
  {
    sF32 start = 0.3678794f; // exp(-1) - this is where the first phase ends

    x -= 1.0f;
    sF32 ex = 1.0f - sFExp(-x);
    return start + ex * (1.0f - start);
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Constants                                                          ***/ 
/***                                                                      ***/
/****************************************************************************/

enum ScriptCodeEnum
{
  // types

  SC_INT     = ScriptTypeInt<<8,
  SC_FLOAT   = ScriptTypeFloat<<8,
  SC_STRING  = ScriptTypeString<<8,
  SC_COLOR   = ScriptTypeColor<<8,

  // expression nodes. I=int, F=float, A=array, R=array range, S=result is scalar B=result is bool

  SC_ADD = 1,                     // IFA
  SC_SUB,                         // IFA
  SC_MUL,                         // IFA
  SC_DIV,                         // IF 
  SC_MOD,                         // IF 
  SC_DOT,                         //  FAS

  SC_NEG,                         // IFA
  SC_FTOI,                        //      fa -> ia
  SC_ITOF,                        //      ia -> fa

  SC_SHIFTL,                      // I
  SC_SHIFTR,                      // I
  SC_ROLL,                        // I
  SC_ROLR,                        // I

  SC_NOT,                         // I  
  SC_NOTNOT,                      // I  
  SC_LOGAND,                      // I 
  SC_LOGOR,                       // I 
  SC_BINAND,                      // I 
  SC_BINOR,                       // I 
  SC_BINXOR,                      // I 

  SC_EQ,                          // IF B
  SC_NE,                          // IF B
  SC_GT,                          // IF B
  SC_GE,                          // IF B
  SC_LT,                          // IF B
  SC_LE,                          // IF B

  SC_B,                           //       branch always
  SC_BT,                          //       branch if true
  SC_BF,                          //       branch if true
  SC_STOP,                        //       end execution
  
  SC_LITERAL,                     // IFA   integer literal
  SC_GETVAR,                      // IF    load variable to stack
  SC_SETVAR,                      // IF    store variable from stack
  SC_CLEARVAR,                    // IF    clear variable
  SC_GETVARR,                     // IFR   load variable to stack
  SC_SETVARR,                     // IFR   store variable from stack
  SC_CLEARVARR,                   // IFR   clear variable
  SC_MAKELOCAL,                   // IFA   create new variable
  SC_MAKEGLOBAL,                  // IFA   create new variable
  SC_IMPORT,                      // IFA   import variable from imports list by sPoolString
  SC_IMPORTDEFAULT,               // IFA   import with default in case of failure
  SC_SPLINE,                      // IFA   evaluate spline
  SC_CAT,                         //       [ifa  ifa] -> ifa (cat)
  SC_INDEX,                       //       ifa[i] -> ifs
  SC_SPLICE,                      //       ifa[n..n] -> ifa
  SC_DUP,                         //       ifs -> ifa
  SC_CALL,

  SC_COND,                        // I ? IFA : IFA
  SC_TILDE,                       // virtual
  SC_ASSIGN,                      // IFA

  SC_ABS,                         // IF
  SC_SIGN,
  SC_MAX,                         // IF
  SC_MIN,                         // IF
  SC_SIN,
  SC_COS,
  SC_SIN1,
  SC_COS1,
  SC_TAN,
  SC_ATAN,
  SC_ATAN2,
  SC_SQRT,
  SC_POW,
  SC_EXP,
  SC_LOG,
  SC_SMOOTHSTEP,
  SC_RAMPUP,
  SC_RAMPDOWN,
  SC_TRIANGLE,
  SC_PULSE,
  SC_EXPEASE,
  SC_FADEINOUT,                   // time,in,out,smooth
  SC_NOISE,                       // noise(time). random value between -1 and 1 (inclusive)
  SC_PERLINNOISE,                 // perlinnoise(time,variant). similar to noise, but bandlimited.

  SC_CLAMP,                       // IF
  SC_LENGTH,
  SC_NORMALIZE,
  SC_MAP,

  SC_PRINT,
  SC_CTOF,
  SC_FTOC,

  // statements

  STAT_BLOCK = 1,
  STAT_LOCAL,
  STAT_GLOBAL,
  STAT_IMPORT,
  STAT_ASSIGN,
  STAT_IF,
  STAT_DO,
  STAT_WHILE,

  // script

  TOK_INT = 0x100,
  TOK_FLOAT,
  TOK_STRING,
  TOK_COLOR,

  TOK_ENUM,
  TOK_STRUCT,
  TOK_UNION,
  TOK_CLASS,
  TOK_TYPE,
  TOK_FUNC,
  TOK_ATTRIBUTE,
  TOK_CAST,

  TOK_IF,
  TOK_ELSE,
  TOK_SWITCH,
  TOK_CASE,
  TOK_WHILE,
  TOK_DO,
  TOK_BREAK,
  TOK_CONTINUE,
  TOK_GLOBAL,
  TOK_IMPORT,
  TOK_PI,
  TOK_PI2,

  TOK_ROLR,
  TOK_ROLL,
  TOK_INC,
  TOK_DEC,

  TOK_SHIFTR = sTOK_SHIFTR,
  TOK_SHIFTL = sTOK_SHIFTL,
  TOK_EQ = sTOK_EQ,
  TOK_NE = sTOK_NE,
  TOK_LE = sTOK_LE,
  TOK_GE = sTOK_GE,
  TOK_TILDETILDE,

  TOK_LOGAND,
  TOK_LOGOR
};

// command code
//
// bit0..6: command
// bit7   : float
// bit8..15: element count
// bit16..31: extra
// bit16..23: range start 
// bit24..31: range end
//
// specials:
// GETVAR/SETVAR:  ptr to symbol, element range
// LITERAL: values
// BT/BF: offset, relative to next command

/****************************************************************************/
/***                                                                      ***/
/***   spline commands                                                    ***/
/***                                                                      ***/
/****************************************************************************/

ScriptSpline::ScriptSpline()
{
  Count = 0;
  Curves = 0;
  Bindings = -1;
  RotClampMask = 0;
}

ScriptSpline::~ScriptSpline()
{
  for(sInt i=0;i<Count;i++)
    delete Curves[i];
  delete[] Curves;
}

void ScriptSpline::Init(sInt count)
{
  Count = count;
  Curves = new sArray<ScriptSplineKey>*[Count];
  for(sInt i=0;i<Count;i++)
    Curves[i] = new sArray<ScriptSplineKey>;
}

void ScriptSpline::Eval(sF32 time,sF32 *result,sInt count)
{
  sVERIFY(count<=Count);

  switch(Flags & SSF_BoundMask)
  {
  case SSF_BoundClamp:
    time = sClamp<sF32>(time,0,MaxTime);
    break;
  case SSF_BoundWrap:
    time = sAbsMod(time,MaxTime);
    break;
  case SSF_BoundZero:
    if(time<=0 || time>=MaxTime)
    {
      for(sInt i=0;i<count;i++)
        result[i] = 0;
      return;
    }
    break;
  case SSF_BoundExtrapolate:
    break;
  }

  for(sInt i=0;i<count;i++)
  {
    sArray<ScriptSplineKey> &curve = *Curves[i];
    ScriptSplineKey k0,k1,k2,k3;
    sF32 c0,c1,c2,c3;
    sF32 t,t0,t1,t2;

    sInt max = curve.GetCount();
    sInt n;
    for(n=0;n<max-1;n++)
      if(time >= curve[n].Time && time < curve[n+1].Time)
        goto found;

    if(max==0)
      result[i] = 0;
    else if(max==1)
      result[i] = curve[0].Value;
    else 
    {
      if(time <= curve[0].Time)
      {
        sF32 v0 = curve[0].Value;
        sF32 t0 = curve[0].Time;
        sF32 v1 = curve[1].Value;
        sF32 t1 = curve[1].Time;
        result[i] = v0 - (v1-v0)*(t0-time)/(t1-t0);
      }
      else 
      {
        sF32 v0 = curve[max-2].Value;
        sF32 t0 = curve[max-2].Time;
        sF32 v1 = curve[max-1].Value;
        sF32 t1 = curve[max-1].Time;
        result[i] = v1 + (v1-v0)*(time-t1)/(t1-t0);
      }
    }
      
    continue;
found:
    k1 = curve[n];
    k2 = curve[n+1];
    if(n-1<0)
    {
      k0.Time  = k1.Time *2 - k2.Time;
      k0.Value = k1.Value*2 - k2.Value;
    }
    else
    {
      k0 = curve[n-1];
    }
    if(n+2>=max)
    {
      k3.Time  = k2.Time *2 - k1.Time;
      k3.Value = k2.Value*2 - k1.Value;
    }
    else
    {
      k3 = curve[n+2];
    }

    t1 = k2.Time-k1.Time;
    t = (time-k1.Time)/t1;

    switch(Mode)
    {
    case SSM_Step:
      c0=0; c1=1; c2=0; c3=0;
      break;
    case SSM_Linear:
      c0=0; c1=1-t; c2=t; c3=0;
      break;
    case SSM_UniformHermite:
      t0 = k1.Time-k0.Time;
      t2 = k3.Time-k2.Time;
      sHermiteUniform(t,t0,t1,t2,c0,c1,c2,c3);
      break;
    case SSM_UniformBSpline:
      sUniformBSpline(t,c0,c1,c2,c3);
      break;
    case SSM_Hermite:
      sHermite(t,c0,c1,c2,c3);
      break;
    }

    if((1<<i) & RotClampMask)
    {
      sF32 v[4];
      v[0] = sMod(k0.Value,1);
      v[1] = sMod(k1.Value,1);
      v[2] = sMod(k2.Value,1);
      v[3] = sMod(k3.Value,1);

      while(v[1]>v[0]+0.5f) v[1]-=1;
      while(v[1]<v[0]-0.5f) v[1]+=1;

      while(v[2]>v[1]+0.5f) v[2]-=1;
      while(v[2]<v[1]-0.5f) v[2]+=1;

      while(v[3]>v[2]+0.5f) v[3]-=1;
      while(v[3]<v[2]-0.5f) v[3]+=1;

      result[i] = sMod(c0*v[0] + c1*v[1] + c2*v[2] + c3*v[3],1);
    }
    else
    {
      result[i] = c0*k0.Value + c1*k1.Value + c2*k2.Value + c3*k3.Value;
    }
  }
}

void ScriptSpline::EvalD(sF32 time,sF32 *result,sInt count)
{
  sVERIFY(count<=Count);

  switch(Flags & SSF_BoundMask)
  {
  case SSF_BoundZero:
  case SSF_BoundClamp: // if we clamp time, the derivative becomes zero outside the valid time range
    if(time<0 || time>MaxTime)
    {
      for(sInt i=0;i<count;i++)
        result[i] = 0;
      return;
    }
    break;
  case SSF_BoundWrap:
    time = sAbsMod(time,MaxTime);
    break;
  case SSF_BoundExtrapolate:
    break;
  }

  for(sInt i=0;i<count;i++)
  {
    sArray<ScriptSplineKey> *curve = Curves[i];
    ScriptSplineKey k0,k1,k2,k3;
    sF32 c0,c1,c2,c3;
    sF32 t,t0,t1,t2;

    sInt max = curve->GetCount();
    sInt n;
    for(n=0;n<max-1;n++)
      if(time >= (*curve)[n].Time && time < (*curve)[n+1].Time)
        goto found;

    result[i] = 0;
    continue;

found:
    k1 = (*curve)[n];
    k2 = (*curve)[n+1];
    if(n-1<0)
    {
      k0.Time  = k1.Time *2 - k2.Time;
      k0.Value = k1.Value*2 - k2.Value;
    }
    else
    {
      k0 = (*curve)[n-1];
    }
    if(n+2>=max)
    {
      k3.Time  = k2.Time *2 - k1.Time;
      k3.Value = k2.Value*2 - k1.Value;
    }
    else
    {
      k3 = (*curve)[n+2];
    }

    t1 = k2.Time-k1.Time;
    t = (time-k1.Time)/t1;

    switch(Mode)
    {
    case SSM_Step:
      c0=0; c1=0; c2=0; c3=0;
      break;
    case SSM_Linear:
      c0=0; c1=-1; c2=1; c3=0;
      break;
    case SSM_UniformHermite:
      t0 = k1.Time-k0.Time;
      t2 = k3.Time-k2.Time;
      sHermiteUniformD(t,t0,t1,t2,c0,c1,c2,c3);
      break;
    case SSM_UniformBSpline:
      sUniformBSplineD(t,c0,c1,c2,c3);
      break;
    case SSM_Hermite:
      sHermiteD(t,c0,c1,c2,c3);
      break;
    }

    result[i] = (c0*k0.Value + c1*k1.Value + c2*k2.Value + c3*k3.Value) / t1;
  }
}

sF32 ScriptSpline::Length()
{
  return MaxTime;
/*
    sF32 x = 0;
  for(sInt i=0;i<Count;i++)
  {
    sInt max = Curves[i]->GetCount();
    if(max>0)
      x = sMax(x,(*Curves[i])[max-1].Time);
  }
  return x;
  */
}

sF32 ScriptSpline::ArcLength(sF32 startTime,sF32 endTime) const
{
  sF32 ret = sRombergIntegral(ArcLengthIntegrand,(void*) this,startTime,endTime,12,1e-6f);
  return ret;
}

sF64 ScriptSpline::ArcLengthIntegrand(sF64 x,void *user)
{
  ScriptSpline *me = (ScriptSpline *) user;
  sF32 vals[16];
  sVERIFY(me->Count <= sCOUNTOF(vals));

  // calc derivative at given time
  me->EvalD(x,vals,me->Count);

  // calc length of derivative
  sF64 sumSq = 0.0;
  for(sInt i=0;i<me->Count;i++)
    sumSq += vals[i]*vals[i];


  sF64 sum = sFSqrt(sumSq);
  return sum;
}

/****************************************************************************/
/***                                                                      ***/
/***   Context                                                            ***/
/***                                                                      ***/
/****************************************************************************/

ScriptContext::ScriptContext()
{
  Locals = 0;
  Scope = 0;
  Pool = new sMemoryPool(0x2000);
  Code = 0;

  AddFunc(L"abs",L"f:f")->Primitive=SC_ABS;
  AddFunc(L"sign",L"f:f")->Primitive=SC_SIGN;
  AddFunc(L"max",L"f:ff")->Primitive=SC_MAX;
  AddFunc(L"min",L"f:ff")->Primitive=SC_MIN;
  AddFunc(L"sin",L"f:f")->Primitive=SC_SIN;
  AddFunc(L"cos",L"f:f")->Primitive=SC_COS;
  AddFunc(L"sin1",L"f:f")->Primitive=SC_SIN1;
  AddFunc(L"cos1",L"f:f")->Primitive=SC_COS1;
  AddFunc(L"tan",L"f:f")->Primitive=SC_TAN;
  AddFunc(L"atan",L"f:f")->Primitive=SC_ATAN;
  AddFunc(L"atan2",L"f:ff")->Primitive=SC_ATAN2;
  AddFunc(L"sqrt",L"f:f")->Primitive=SC_SQRT;
  AddFunc(L"pow",L"f:ff")->Primitive=SC_POW;
  AddFunc(L"exp",L"f:f")->Primitive=SC_EXP;
  AddFunc(L"log",L"f:f")->Primitive=SC_LOG;
  AddFunc(L"smoothstep",L"f:f")->Primitive=SC_SMOOTHSTEP;
  AddFunc(L"fadeinout",L"f:ffff")->Primitive=SC_FADEINOUT;
  AddFunc(L"rampup",L"f:f")->Primitive=SC_RAMPUP;
  AddFunc(L"rampdown",L"f:f")->Primitive=SC_RAMPDOWN;
  AddFunc(L"triangle",L"f:f")->Primitive=SC_TRIANGLE;
  AddFunc(L"pulse",L"f:ff")->Primitive=SC_PULSE;
  AddFunc(L"expease",L"f:ff")->Primitive=SC_EXPEASE;
  AddFunc(L"noise",L"f:f")->Primitive=SC_NOISE;
  AddFunc(L"perlinnoise",L"f:ff")->Primitive=SC_PERLINNOISE;

  AddFunc(L"clamp",L"f#1:f#1,f,f")->Primitive=SC_CLAMP;
  AddFunc(L"length",L"f:f#1")->Primitive=SC_LENGTH;
  AddFunc(L"dot",L"f:f#1,f#1")->Primitive=SC_DOT;
  AddFunc(L"normalize",L"f#1:f#1")->Primitive=SC_NORMALIZE;
  AddFunc(L"map",L"f#1:f#1,f,f")->Primitive=SC_MAP;
}

ScriptContext::~ScriptContext()
{
  delete Pool;
  delete[] Code;
  sDeleteAll(Symbols);
  sDeleteAll(Funcs);
}

/****************************************************************************/

ScriptSymbol *ScriptContext::AddSymbol(sPoolString name)
{
  if(1)   // case insensitive
  {
    sString<256> buffer = name;
    sMakeLower(buffer);
    name = sPoolString(buffer);
  }
  ScriptSymbol *sym = sFind(Symbols,&ScriptSymbol::Name,name);
  if(sym==0)
  {
    sym = new ScriptSymbol;
    sym->Name = name;
    sym->Value = 0;
    sym->CType = 0;
    sym->CCount = 0;
    Symbols.AddTail(sym);
  }
  return sym;
}

ScriptValue *ScriptContext::MakeValue(sInt type,sInt dim)
{
  ScriptValue *val = Pool->Alloc<ScriptValue>();
  val->Type = type;
  val->Count = sAbs(dim);
  val->Symbol = 0;
  val->ScopeLink = 0;
  val->OldValue = 0;
  val->Func = 0;
  val->Spline = 0;

  if(dim==0)
    dim = 1;
  if(dim>0)
  {
    val->IntPtr = Pool->Alloc<sInt>(dim);
    for(sInt i=0;i<dim;i++)
      val->IntPtr[i] = 0;
  }
  return val;
}

ScriptValue *ScriptContext::CopyValue(ScriptValue *s)
{
  sVERIFY(!s->Func);
  sVERIFY(!s->Spline);
  ScriptValue *d = MakeValue(s->Type,s->Count);
  if(s->Type==ScriptTypeString)
    for(sInt i=0;i<s->Count;i++)
      d->StringPtr[i] = s->StringPtr[i];
  else
    for(sInt i=0;i<s->Count;i++)
      d->IntPtr[i] = s->IntPtr[i];
  return d;
}

ScriptValue *ScriptContext::MakeValue(ScriptFunc *func)
{
  ScriptValue *val = Pool->Alloc<ScriptValue>();
  val->Type = func->Result.Type;
  val->Count = func->Result.Count;
  val->Symbol = 0;
  val->ScopeLink = 0;
  val->OldValue = 0;
  val->Func = func;
  val->Spline = 0;

  return val;
}

ScriptValue *ScriptContext::MakeValue(ScriptSpline *spl)
{
  ScriptValue *val = Pool->Alloc<ScriptValue>();
  val->Type = 2;
  val->Count = spl->Count;
  val->Symbol = 0;
  val->ScopeLink = 0;
  val->OldValue = 0;
  val->Func = 0;
  val->Spline = spl;

  return val;
}

void ScriptContext::BindLocal(ScriptSymbol *sym,ScriptValue *val)
{
  sVERIFY(val->OldValue==0);
  sVERIFY(val->Symbol==0);
  sVERIFY(val->ScopeLink==0);

  val->OldValue = sym->Value;
  val->Symbol = sym;
  sym->Value = val;

  val->ScopeLink = Locals;
  Locals = val;
}

void ScriptContext::BindGlobal(ScriptSymbol *sym,ScriptValue *val)
{
  sVERIFY(val->OldValue==0);
  sVERIFY(val->Symbol==0);
  sVERIFY(val->ScopeLink==0);

  val->OldValue = sym->Value;
  val->Symbol = sym;
  sym->Value = val;

  val->ScopeLink = *Scope;  
  *Scope = val;
}

void ScriptContext::UpdateCType()
{
  ScriptSymbol *sym;

  sFORALL(Symbols,sym)
  {
    if(sym->Value)
    {
      sym->CType = sym->Value->Type;
      sym->CCount = sym->Value->Count;
    }
    else
    {
      sym->CType = 0;
      sym->CCount = 0;
    }
  }
}

void ScriptContext::Compile(const sChar *code)
{
  ScriptCompiler c;
  delete[] Code;

  BeginExe();
  PushGlobal();
  Code = c.Parse(this,code,0);
  ErrorMsg = c.ErrorMsg;
}

const sChar *ScriptContext::Run()
{
  if(Code==0)
    return ErrorMsg;
  else
  {
    if(ScriptExecute(Code,this,ErrorMsg))
      return 0;
    else
      return ErrorMsg;
  }
}

/****************************************************************************/

ScriptFunc::ScriptFunc()
{
  Args = 0;
}

ScriptFunc::~ScriptFunc()
{
  delete[] Args;
}

sBool ScanArg(const sChar *&s,ScriptFuncArg &arg)
{
  sBool index = 0;
  sInt n = 1;
  if(*s=='i')
    arg.Type = 1;
  else if(*s=='f')
    arg.Type = 2;
  else 
    return 0;
  s++;
  if(*s=='#')
  {
    index = 1;
    s++;
  }
  if(sIsDigit(*s))
  {
    sScanInt(s,n);
  }
  if(*s==',')
    s++;
  if(index)
  {
    arg.Count = 0;
    arg.VarCount = n;
  }
  else
  {
    arg.Count = n;
    arg.VarCount = 0;
  }
  return 1;
}

ScriptFunc *ScriptContext::AddFunc(sPoolString name,const sChar *proto)
{
  ScriptFunc *func = new ScriptFunc;
  sClear(*func);
  Funcs.AddTail(func);
  func->Name = name;

  // result type

  ScanArg(proto,func->Result);

  // arguments

  if(*proto==':')
  {
    sArray<ScriptFuncArg> args;
    ScriptFuncArg arg;
    proto++;

    while(ScanArg(proto,arg))
    {
      *args.AddMany(1) = arg;
    }
    func->ArgCount = args.GetCount();
    func->Args = new ScriptFuncArg[func->ArgCount];
    for(sInt i=0;i<func->ArgCount;i++)
      func->Args[i] = args[i];
  }

  sVERIFY(*proto==0);

  return func;
}

/****************************************************************************/

static void Unlink(ScriptValue *sv)
{
  ScriptValue *next;

  while(sv)
  {
    next = sv->ScopeLink;
    sVERIFY(sv->Symbol->Value == sv);
    sv->Symbol->Value = sv->OldValue;

    sv->OldValue = 0;
    sv->Symbol = 0;
    sv->ScopeLink = 0;

    sv = next;
  }
}

void ScriptContext::ClearImports()
{
  Imports.Clear();
}

void ScriptContext::AddImport(sPoolString name,sInt type,sInt count,void *ptr)
{
  ScriptImport *imp = sFind(Imports,&ScriptImport::Name,name);

  if(imp==0)
    imp = Pool->Alloc<ScriptImport>(1);
  
  // simply overwrite existing imports!

  imp->Name = name;
  imp->Type = type;
  imp->Count = count;
  imp->IntPtr = (sInt *) ptr;
  Imports.AddTail(imp);
}

ScriptImport *ScriptContext::FindImport(sPoolString name)
{
  return sFind(Imports,&ScriptImport::Name,name);
}


void ScriptContext::PushGlobal()
{
  Scopes.AddMany(1);
  Scope = &Scopes[Scopes.GetCount()-1];
  *Scope = 0;
}

void ScriptContext::PopGlobal()
{
  sInt n = Scopes.GetCount()-1;
  sVERIFY(n>=0);
  ScriptValue *sv = Scopes[n];
  Scopes[n] = 0;
  Scopes.RemTail();
  Unlink(sv);
}

void ScriptContext::FlushLocal()
{
  Unlink(Locals);
  Locals = 0;
}

void ScriptContext::ProtectGlobal()
{
  ScriptValue *s,*v;
  TVA.Clear();
  sFORALL(Scopes,s)
  {
    v = s;
    while(v)
    {
      if(v->Func==0 && v->Spline==0 && v->Symbol->Value==v)
        TVA.AddTail(v);
      v = v->ScopeLink;
    }
  }
  sFORALL(TVA,v)
  {
    BindGlobal(v->Symbol,CopyValue(v));
  }
  TVA.Clear();
}

/****************************************************************************/

void ScriptContext::BeginExe()
{
  ScriptSymbol *sym;
  ScriptFunc *func;

  sFORALL(Symbols,sym)
    sVERIFY(sym->Value == 0);
  sVERIFY(Scopes.GetCount()==0);
  ClearImports();
  
  Pool->Reset();

  PushGlobal();

  sFORALL(Funcs,func)
  {
    sym = AddSymbol(func->Name);
    ScriptValue *val = MakeValue(func);
    BindGlobal(sym,val);
  }
}

void ScriptContext::EndExe()
{
  PopGlobal();
  FlushLocal();
  sVERIFY(Scopes.GetCount()==0);

  Pool->Reset();
}

/****************************************************************************/

ScriptValue *ScriptContext::MakeFloat(sInt dim) 
{ 
  return MakeValue(ScriptTypeFloat,dim); 
}
ScriptValue *ScriptContext::MakeInt(sInt dim) 
{ 
  return MakeValue(ScriptTypeInt,dim); 
}
ScriptValue *ScriptContext::MakeColor(sInt dim) 
{ 
  return MakeValue(ScriptTypeColor,dim); 
}
ScriptValue *ScriptContext::MakeString(sInt dim) 
{ 
  return MakeValue(ScriptTypeString,dim); 
}

void ScriptContext::BindLocalFloat(ScriptSymbol *sym,sInt dim,sF32 *ptr)
{
  ScriptValue *val = MakeFloat(-dim);
  val->FloatPtr = ptr;
  BindLocal(sym,val);
}

void ScriptContext::BindLocalInt(ScriptSymbol *sym,sInt dim,sInt *ptr)
{
  ScriptValue *val = MakeInt(-dim);
  val->IntPtr = ptr;
  BindLocal(sym,val);
}

void ScriptContext::BindLocalColor(ScriptSymbol *sym,sU32 *ptr)
{
  ScriptValue *val = MakeColor(1);
  val->ColorPtr = ptr;
  BindLocal(sym,val);
/*

  sVector4 c;

  ScriptValue *val = MakeFloat(4);
  c.InitColor(*ptr);
  val->FloatPtr[0] = c.x;
  val->FloatPtr[1] = c.y;
  val->FloatPtr[2] = c.z;
  val->FloatPtr[3] = c.w;
  BindLocal(sym,val);
  return val->FloatPtr;
  */
}


/*
void ScriptContext::UnbindLocalColor(ScriptSymbol *sym,sU32 *ptr)
{
  sVector4 c;
  sVERIFY(sym->CCount==4 && sym->CType==2);
  c.x = sym->Value->FloatPtr[0];
  c.y = sym->Value->FloatPtr[1];
  c.z = sym->Value->FloatPtr[2];
  c.w = sym->Value->FloatPtr[3];
  *ptr = c.GetColor();
}
*/

/****************************************************************************/
/***                                                                      ***/
/***   Code                                                               ***/
/***                                                                      ***/
/****************************************************************************/

static void *readptr(const sU32 *&ptr)
{
  void **wptr = (void **) ptr;
  void *r = *wptr++;
  ptr = (sU32 *) wptr;
  return r;
}


sBool ScriptCode::Exe(ScriptContext *ctx)
{
  return ScriptExecute(Code,ctx,ErrorMsg);
}


void ScriptDump(const sU32 *code,sTextBuffer &tb);

sBool ScriptExecute(const sU32 *Code,ScriptContext *ctx,sString<1024> &ErrorMsg)
{
  union value
  {
    sS32 i;
    sF32 f;
    sU32 c;
    const sChar *s;
  };

static  value Stack[0x10000];
  sInt index = 0;

#if !sRELEASE
  sSetMem(Stack,0x11,sizeof(Stack));
#endif
 
  if(!Code)
    return 0;

  if(0)                           // debug: disassemble the code before execution
  {
    sTextBuffer tb;
    ScriptDump(Code,tb);
    sDPrint(L"----\n");
    sDPrint(tb.Get());
  }

  const sU32 *ptr = Code;

  sInt timeout = 0x10000;
  for(;;)
  {
    timeout--;
    if(timeout<=0)
    {
      ErrorMsg.PrintF(L"endless loop (or at least a very long one)");
      return 0;
    }

    sU32 cmd = *ptr++;
    sInt count = (cmd>>16)&0xffff;
    sInt type = (cmd>>8)&0xff;
    switch(cmd&0xffff)
    {
    case SC_ADD|SC_INT:
      for(sInt i=0;i<count;i++)
        Stack[index-count*2+i].i = Stack[index-count*2+i].i + Stack[index-count+i].i;
      index-=count;
      break;
    case SC_ADD|SC_FLOAT:
      for(sInt i=0;i<count;i++)
        Stack[index-count*2+i].f = Stack[index-count*2+i].f + Stack[index-count+i].f;
      index-=count;
      break;
    case SC_SUB|SC_INT:
      for(sInt i=0;i<count;i++)
        Stack[index-count*2+i].i = Stack[index-count*2+i].i - Stack[index-count+i].i;
      index-=count;
      break;
    case SC_SUB|SC_FLOAT:
      for(sInt i=0;i<count;i++)
        Stack[index-count*2+i].f = Stack[index-count*2+i].f - Stack[index-count+i].f;
      index-=count;
      break;
    case SC_MUL|SC_INT:
      for(sInt i=0;i<count;i++)
        Stack[index-count*2+i].i = Stack[index-count*2+i].i * Stack[index-count+i].i;
      index-=count;
      break;
    case SC_MUL|SC_FLOAT:
      for(sInt i=0;i<count;i++)
        Stack[index-count*2+i].f = Stack[index-count*2+i].f * Stack[index-count+i].f;
      index-=count;
      break;
    case SC_DIV|SC_INT:
      for(sInt i=0;i<count;i++)
        Stack[index-count*2+i].i = Stack[index-count*2+i].i / Stack[index-count+i].i;
      index-=count;
      break;
    case SC_DIV|SC_FLOAT:
      for(sInt i=0;i<count;i++)
        Stack[index-count*2+i].f = Stack[index-count*2+i].f / Stack[index-count+i].f;
      index-=count;
      break;
    case SC_MOD|SC_INT:
      for(sInt i=0;i<count;i++)
        Stack[index-count*2+i].i = Stack[index-count*2+i].i % Stack[index-count+i].i;
      index-=count;
      break;
    case SC_MOD|SC_FLOAT:
      for(sInt i=0;i<count;i++)
        Stack[index-count*2+i].f = sMod(Stack[index-count*2+i].f , Stack[index-count+i].f);
      index-=count;
      break;

    case SC_SHIFTL|SC_INT:
      for(sInt i=0;i<count;i++)
        Stack[index-count*2+i].i = Stack[index-count*2+i].i << Stack[index-count+i].i;
      index-=count;
      break;
    case SC_SHIFTR|SC_INT:
      for(sInt i=0;i<count;i++)
        Stack[index-count*2+i].i = Stack[index-count*2+i].i >> Stack[index-count+i].i;
      index-=count;
      break;
    case SC_ROLL|SC_INT:
      for(sInt i=0;i<count;i++)
        Stack[index-count*2+i].i = (Stack[index-count*2+i].i << Stack[index-count+i].i)
                                 | (Stack[index-count*2+i].i >> (32-Stack[index-count+i].i));
      index-=count;
      break;
    case SC_ROLR|SC_INT:
      for(sInt i=0;i<count;i++)
        Stack[index-count*2+i].i = (Stack[index-count*2+i].i >> Stack[index-count+i].i)
                                 | (Stack[index-count*2+i].i << (32-Stack[index-count+i].i));
      index-=count;
      break;

    case SC_DOT|SC_FLOAT:
      {
        sF32 accu = 0;
        for(sInt i=0;i<count;i++)
          accu += Stack[index-count*2+i].f*Stack[index-count+i].f;
        index-=count*2;
        Stack[index++].f = accu;
      }
      break;
    case SC_NEG|SC_INT:
      for(sInt i=0;i<count;i++)
        Stack[index-count+i].i = -Stack[index-count+i].i;
      break;
    case SC_NEG|SC_FLOAT:
      for(sInt i=0;i<count;i++)
        Stack[index-count+i].f = -Stack[index-count+i].f;
      break;
    case SC_FTOI|SC_INT:
      for(sInt i=0;i<count;i++)
        Stack[index-count+i].i = sInt(Stack[index-count+i].f);
      break;
    case SC_ITOF|SC_FLOAT:
      for(sInt i=0;i<count;i++)
        Stack[index-count+i].f = sF32(Stack[index-count+i].i);
      break;

    case SC_NOT|SC_INT:
      sVERIFY(count==1);
      Stack[index-1].i = !Stack[index-1].i;
      break;
    case SC_NOTNOT|SC_INT:
      sVERIFY(count==1);
      Stack[index-1].i = !!Stack[index-1].i;
      break;
    case SC_LOGAND|SC_INT:
      sVERIFY(count==1);
      Stack[index-2].i = Stack[index-2].i && Stack[index-1].i;
      index--;
      break;
    case SC_LOGOR|SC_INT:
      sVERIFY(count==1);
      Stack[index-2].i = Stack[index-2].i || Stack[index-1].i;
      index--;
      break;
    case SC_BINAND|SC_INT:
      sVERIFY(count==1);
      Stack[index-2].i = Stack[index-2].i & Stack[index-1].i;
      index--;
      break;
    case SC_BINOR|SC_INT:
      sVERIFY(count==1);
      Stack[index-2].i = Stack[index-2].i | Stack[index-1].i;
      index--;
      break;
    case SC_BINXOR|SC_INT:
      sVERIFY(count==1);
      Stack[index-2].i = Stack[index-2].i ^ Stack[index-1].i;
      index--;
      break;

    case SC_EQ|SC_INT:
      sVERIFY(count==1);
      Stack[index-2].i = Stack[index-2].i == Stack[index-1].i;
      index--;
      break;
    case SC_EQ|SC_FLOAT:
      sVERIFY(count==1);
      Stack[index-2].i = Stack[index-2].f == Stack[index-1].f;
      index--;
      break;
    case SC_NE|SC_INT:
      sVERIFY(count==1);
      Stack[index-2].i = Stack[index-2].i != Stack[index-1].i;
      index--;
      break;
    case SC_NE|SC_FLOAT:
      sVERIFY(count==1);
      Stack[index-2].i = Stack[index-2].f != Stack[index-1].f;
      index--;
      break;
    case SC_GT|SC_INT:
      sVERIFY(count==1);
      Stack[index-2].i = Stack[index-2].i > Stack[index-1].i;
      index--;
      break;
    case SC_GT|SC_FLOAT:
      sVERIFY(count==1);
      Stack[index-2].i = Stack[index-2].f > Stack[index-1].f;
      index--;
      break;
    case SC_GE|SC_INT:
      sVERIFY(count==1);
      Stack[index-2].i = Stack[index-2].i >= Stack[index-1].i;
      index--;
      break;
    case SC_GE|SC_FLOAT:
      sVERIFY(count==1);
      Stack[index-2].i = Stack[index-2].f >= Stack[index-1].f;
      index--;
      break;
    case SC_LT|SC_INT:
      sVERIFY(count==1);
      Stack[index-2].i = Stack[index-2].i < Stack[index-1].i;
      index--;
      break;
    case SC_LT|SC_FLOAT:
      sVERIFY(count==1);
      Stack[index-2].i = Stack[index-2].f < Stack[index-1].f;
      index--;
      break;
    case SC_LE|SC_INT:
      sVERIFY(count==1);
      Stack[index-2].i = Stack[index-2].i <= Stack[index-1].i;
      index--;
      break;
    case SC_LE|SC_FLOAT:
      sVERIFY(count==1);
      Stack[index-2].i = Stack[index-2].f <= Stack[index-1].f;
      index--;
      break;

    case SC_B:
      ptr += sS16((cmd>>16)&0xffff);
      break;
    case SC_BT:
      index--;
      if(Stack[index].i)
        ptr += sS16((cmd>>16)&0xffff);
      break;
    case SC_BF:
      index--;
      if(!Stack[index].i)
        ptr += sS16((cmd>>16)&0xffff);
      break;
    case SC_STOP:
      if(index!=0)
      {
        ErrorMsg.PrintF(L"stack error");
        return 0;
      }
      return 1;

    case SC_LITERAL|SC_INT:
    case SC_LITERAL|SC_FLOAT:
    case SC_LITERAL|SC_COLOR:
      for(sInt i=0;i<count;i++)
        Stack[index++].i = *ptr++;
      break;

    case SC_LITERAL|SC_STRING:
      for(sInt i=0;i<count;i++)
        Stack[index++].s = (const sChar *) readptr(ptr);
      break;

    case SC_MAKELOCAL|SC_INT:
    case SC_MAKELOCAL|SC_FLOAT:
    case SC_MAKELOCAL|SC_STRING:
    case SC_MAKELOCAL|SC_COLOR:
      {
        ScriptSymbol *sym = (ScriptSymbol *) readptr(ptr);
        ScriptValue *val = ctx->MakeValue(type,count);
        ctx->BindLocal(sym,val);
      }
      break;

    case SC_MAKEGLOBAL|SC_INT:
    case SC_MAKEGLOBAL|SC_FLOAT:
    case SC_MAKEGLOBAL|SC_STRING:
    case SC_MAKEGLOBAL|SC_COLOR:
      {
        ScriptSymbol *sym = (ScriptSymbol *) readptr(ptr);
        ScriptValue *val = ctx->MakeValue(type,count);
        ctx->BindGlobal(sym,val);
      }
      break;

    case SC_IMPORT|SC_INT:
    case SC_IMPORT|SC_FLOAT:
    case SC_IMPORT|SC_STRING:
    case SC_IMPORT|SC_COLOR:
      {
        sPoolString name;
        *((void **)&name) = readptr(ptr);
        ScriptImport *import = ctx->FindImport(name);
        if(import==0)
        {
          ErrorMsg.PrintF(L"import %q not found",name);
          return 0;
        }
        if(import->Type!=type)
        {
          ErrorMsg.PrintF(L"import %q has wrong type",name);
          return 0;
        }
        if(import->Count!=count)
        {
          ErrorMsg.PrintF(L"import %q has wrong count",name);
          return 0;
        }
        ScriptSymbol *sym = ctx->AddSymbol(name);
        ScriptValue *val = ctx->MakeValue(type,count);
        val->IntPtr = import->IntPtr;
        ctx->BindGlobal(sym,val);
      }
      break;

    case SC_IMPORTDEFAULT|SC_INT:
    case SC_IMPORTDEFAULT|SC_FLOAT:
    case SC_IMPORTDEFAULT|SC_STRING:
    case SC_IMPORTDEFAULT|SC_COLOR:
      {
        sPoolString name;
        *((void **)&name) = readptr(ptr);
        ScriptImport *import = ctx->FindImport(name);
        if(import==0)   // use default
        {
          ScriptSymbol *sym = ctx->AddSymbol(name);
          ScriptValue *val = ctx->MakeValue(type,count);
          ctx->BindLocal(sym,val);

          if(val==0 || val->Type!=type || count!=val->Count)
          {
            ErrorMsg.PrintF(L"type error");
            return 0;
          }
          index -= count;
          if(type==ScriptTypeString)
          {
            for(sInt i=0;i<count;i++)
              val->StringPtr[i] = Stack[index+i].s;
          }
          else
          {
            for(sInt i=0;i<count;i++)
              val->IntPtr[i] = Stack[index+i].i;
          }
        }
        else    // dont use...
        {
          index -= count;
          if(import->Type!=type)
          {
            ErrorMsg.PrintF(L"import %q has wrong type",name);
            return 0;
          }
          if(import->Count!=count)
          {
            ErrorMsg.PrintF(L"import %q has wrong count",name);
            return 0;
          }
          ScriptSymbol *sym = ctx->AddSymbol(name);
          ScriptValue *val = ctx->MakeValue(type,count);
          val->IntPtr = import->IntPtr;
          ctx->BindGlobal(sym,val);
        }
      }
      break;


    case SC_GETVARR|SC_INT:
    case SC_GETVARR|SC_FLOAT:
    case SC_GETVARR|SC_STRING:
    case SC_GETVARR|SC_COLOR:
      {
        sU32 cmd2 = *ptr++;
        sInt range0 = cmd2&0xffff;
        sInt range1 = cmd2>>16;
        ScriptSymbol *sym = (ScriptSymbol *) readptr(ptr);
        ScriptValue *val = sym->Value;
        if(val==0 || (range1-range0)!=count || val->Type!=type || range1>val->Count)
        {
          ErrorMsg.PrintF(L"type error");
          return 0;
        }
        if(type==ScriptTypeString)
        {
          for(sInt i=range0;i<range1;i++)
            Stack[index++].s = val->StringPtr[i];
        }
        else
        {
          for(sInt i=range0;i<range1;i++)
            Stack[index++].i = val->IntPtr[i];
        }
      }
      break;

    case SC_GETVAR|SC_INT:
    case SC_GETVAR|SC_FLOAT:
    case SC_GETVAR|SC_STRING:
    case SC_GETVAR|SC_COLOR:
      {
        sInt range0 = 0;
        sInt range1 = count;
        ScriptSymbol *sym = (ScriptSymbol *) readptr(ptr);
        ScriptValue *val = sym->Value;
        if(val==0 || (range1-range0)!=count || val->Type!=type || range1>val->Count)
        {
          ErrorMsg.PrintF(L"type error");
          return 0;
        }
        if(type==ScriptTypeString)
        {
          for(sInt i=range0;i<range1;i++)
            Stack[index++].s = val->StringPtr[i];
        }
        else
        {
          for(sInt i=range0;i<range1;i++)
            Stack[index++].i = val->IntPtr[i];
        }
      }
      break;

    case SC_SETVARR|SC_INT:
    case SC_SETVARR|SC_FLOAT:
    case SC_SETVARR|SC_STRING:
    case SC_SETVARR|SC_COLOR:
      {
        sU32 cmd2 = *ptr++;
        sInt range0 = cmd2&0xffff;
        sInt range1 = cmd2>>16;
        ScriptSymbol *sym = (ScriptSymbol *) readptr(ptr);
        ScriptValue *val = sym->Value;
        if(val==0 || (range1-range0)!=count || val->Type!=type || range1>val->Count)
        {
          ErrorMsg.PrintF(L"type error");
          return 0;
        }
        index -= (range1-range0);
        if(type==ScriptTypeString)
        {
          for(sInt i=range0;i<range1;i++)
            val->StringPtr[i] = Stack[index+i-range0].s;
        }
        else
        {
          for(sInt i=range0;i<range1;i++)
            val->IntPtr[i] = Stack[index+i-range0].i;
        }
      }
      break;

    case SC_SETVAR|SC_INT:
    case SC_SETVAR|SC_FLOAT:
    case SC_SETVAR|SC_STRING:
    case SC_SETVAR|SC_COLOR:
      {
        sInt range0 = 0;
        sInt range1 = count;
        ScriptSymbol *sym = (ScriptSymbol *) readptr(ptr);
        ScriptValue *val = sym->Value;
        if(val==0 || (range1-range0)!=count || val->Type!=type || range1>val->Count)
        {
          ErrorMsg.PrintF(L"type error");
          return 0;
        }
        index -= (range1-range0);
        if(type==ScriptTypeString)
        {
          for(sInt i=range0;i<range1;i++)
            val->StringPtr[i] = Stack[index+i-range0].s;
        }
        else
        {
          for(sInt i=range0;i<range1;i++)
            val->IntPtr[i] = Stack[index+i-range0].i;
        }
      }
      break;

    case SC_CLEARVARR|SC_INT:
    case SC_CLEARVARR|SC_FLOAT:
    case SC_CLEARVARR|SC_STRING:
    case SC_CLEARVARR|SC_COLOR:
      {
        sU32 cmd2 = *ptr++;
        sInt range0 = cmd2&0xffff;
        sInt range1 = cmd2>>16;
        ScriptSymbol *sym = (ScriptSymbol *) readptr(ptr);
        ScriptValue *val = sym->Value;
        if(val==0 || (range1-range0)!=count || val->Type!=type || range1>val->Count)
        {
          ErrorMsg.PrintF(L"type error");
          return 0;
        }
        if(type==ScriptTypeString)
        {
          for(sInt i=range0;i<range1;i++)
            val->StringPtr[i] = L"";
        }
        else
        {
          for(sInt i=range0;i<range1;i++)
            val->IntPtr[i] = 0;
        }
      }
      break;

    case SC_CLEARVAR|SC_INT:
    case SC_CLEARVAR|SC_FLOAT:
    case SC_CLEARVAR|SC_STRING:
    case SC_CLEARVAR|SC_COLOR:
      {
        sInt range0 = 0;
        sInt range1 = count;
        ScriptSymbol *sym = (ScriptSymbol *) readptr(ptr);
        ScriptValue *val = sym->Value;
        if(val==0 || (range1-range0)!=count || val->Type!=type || range1>val->Count)
        {
          ErrorMsg.PrintF(L"type error");
          return 0;
        }
        if(type==ScriptTypeString)
        {
          for(sInt i=range0;i<range1;i++)
            val->StringPtr[i] = L"";
        }
        else
        {
          for(sInt i=range0;i<range1;i++)
            val->IntPtr[i] = 0;
        }
      }
      break;

    case SC_SPLINE|SC_FLOAT:
      {
        ScriptSymbol *sym = (ScriptSymbol *) readptr(ptr);
        ScriptValue *val = sym->Value;
        if(val==0 || val->Spline==0 || val->Count!=count || val->Type!=2)
        {
          ErrorMsg.PrintF(L"type error (spline)");
          return 0;
        }
        index--;
        sF32 result[8];
        sVERIFY(count<=8);
        val->Spline->Eval(Stack[index].f,result,count);
        for(sInt i=0;i<count;i++)
          Stack[index+i].f = result[i];
        index += count;
      }
      break;

    case SC_CAT|SC_INT:
    case SC_CAT|SC_FLOAT:
    case SC_CAT|SC_STRING:
    case SC_CAT|SC_COLOR:
      break;

    case SC_INDEX|SC_INT:
    case SC_INDEX|SC_FLOAT:
    case SC_INDEX|SC_STRING:
    case SC_INDEX|SC_COLOR:
      {
        index--;
        sInt n = Stack[index].i;
        index -= count;
        if(n<0 || n>=count)
        {
          ErrorMsg.PrintF(L"array out of bounds");
          return 0;
        }
        Stack[index] = Stack[index+n];
        index++;
      }
      break;

    case SC_SPLICE|SC_INT:
    case SC_SPLICE|SC_FLOAT:
    case SC_SPLICE|SC_STRING:
    case SC_SPLICE|SC_COLOR:
      {
        sU32 cmd2 = *ptr++;
        sInt range0 = cmd2&0xffff;
        sInt range1 = cmd2>>16;
        index -= count;
        sInt old = index;
        for(sInt i=range0;i<range1;i++)
          Stack[index++] = Stack[old+i];
      }
      break;

    case SC_DUP|SC_INT:
    case SC_DUP|SC_FLOAT:
    case SC_DUP|SC_STRING:
    case SC_DUP|SC_COLOR:
      for(sInt i=1;i<count;i++)
        Stack[index+i-1] = Stack[index-1];
      index += count-1;
      break;

    case SC_COND:     // c ? a : b
      {
        index--;
        sInt c = Stack[index].i;
        index -= count*2;
        if(c==0)
          for(sInt i=0;i<count;i++)
            Stack[index+i] = Stack[index+i+count];
        index+=count;
      }
      break;

    case SC_ABS|SC_FLOAT:
      Stack[index-1].f = sAbs(Stack[index-1].f);
      break;
    case SC_SIGN|SC_FLOAT:
      Stack[index-1].f = sSign(Stack[index-1].f);
      break;
    case SC_MAX|SC_FLOAT:
      index--;
      Stack[index-1].f = sMax(Stack[index-1].f,Stack[index].f);
      break;
    case SC_MIN|SC_FLOAT:
      index--;
      Stack[index-1].f = sMin(Stack[index-1].f,Stack[index].f);
      break;
    case SC_SIN|SC_FLOAT:
      Stack[index-1].f = sSin(Stack[index-1].f);
      break;
    case SC_COS|SC_FLOAT:
      Stack[index-1].f = sCos(Stack[index-1].f);
      break;
    case SC_SIN1|SC_FLOAT:
      Stack[index-1].f = sSin(Stack[index-1].f*sPI2F);
      break;
    case SC_COS1|SC_FLOAT:
      Stack[index-1].f = sCos(Stack[index-1].f*sPI2F);
      break;
    case SC_TAN|SC_FLOAT:
      Stack[index-1].f = sTan(Stack[index-1].f);
      break;
    case SC_ATAN|SC_FLOAT:
      Stack[index-1].f = sATan(Stack[index-1].f);
      break;
    case SC_ATAN2|SC_FLOAT:
      index--;
      Stack[index-1].f = sATan2(Stack[index-1].f,Stack[index].f);
      break;
    case SC_SQRT|SC_FLOAT:
      Stack[index-1].f = sSqrt(Stack[index-1].f);
      break;
    case SC_POW|SC_FLOAT:
      index--;
      Stack[index-1].f = sPow(Stack[index-1].f,Stack[index].f);
      break;
    case SC_EXP|SC_FLOAT:
      Stack[index-1].f = sExp(Stack[index-1].f);
      break;
    case SC_LOG|SC_FLOAT:
      Stack[index-1].f = sLog(Stack[index-1].f);
      break;
    case SC_SMOOTHSTEP|SC_FLOAT:
      Stack[index-1].f = sSmoothStep(Stack[index-1].f);
      break;
    case SC_RAMPUP|SC_FLOAT:
      Stack[index-1].f = sAbsMod(Stack[index-1].f,1.0f)*2-1;
      break;
    case SC_RAMPDOWN|SC_FLOAT:
      Stack[index-1].f = (1-sAbsMod(Stack[index-1].f,1.0f))*2-1;
      break;
    case SC_TRIANGLE|SC_FLOAT:
      {
        sF32 f = sAbsMod(Stack[index-1].f+0.25f,1.0f)*2;
        if(f>1) f=2-f;
        Stack[index-1].f = f*2-1;
      }
      break;
    case SC_PULSE|SC_FLOAT:
      index--;
      Stack[index-1].f = sAbsMod(Stack[index-1].f,1.0f)>Stack[index].f ? 1 : -1;
      break;
    case SC_EXPEASE|SC_FLOAT:
      index--;
      {
        sF32 ratio = Stack[index].f;
        sF32 f = Stack[index-1].f;
        if(f <= 0.0f)
          Stack[index-1].f = 0.0f;
        else if(f >= 1.0f)
          Stack[index-1].f = 1.0f;
        else
          Stack[index-1].f = ExpEase(f * ratio) / ExpEase(ratio);
      }
      break;
    case SC_FADEINOUT|SC_FLOAT:
      {
        index-=4;
        sF32 t = Stack[index+0].f;
        sF32 ti = Stack[index+1].f;
        sF32 to = Stack[index+2].f;
        sF32 s = Stack[index+3].f;
        sF32 v = 0;
        sF32 t0 = ti-s;
        sF32 t1 = ti+s;
        sF32 t2 = to-s;
        sF32 t3 = to+s;

        if(t<t0)
          v = 0;
        else if(t<t1)
          v = sSmoothStep((t-t0)/(t1-t0));
        else if(t<t2)
          v = 1;
        else if(t<t3)
          v = 1-sSmoothStep((t-t2)/(t3-t2));
        else
          v = 0;

        Stack[index++].f = v;
      }
      break;
    case SC_NOISE|SC_FLOAT:
      index--;
      {
        // hash time value to get a repeatable "random" value
        // intentionally use .c even though input is float!
        sU32 hash = sChecksumMurMur(&Stack[index+0].c,1);
        static const sU32 mask = 0xffffff; // 24 bits is enough for float
        sF32 v = 2.0f * sInt(hash & mask) / sF32(mask) - 1.0f;
        
        Stack[index++].f = v;
      }
      break;
    case SC_PERLINNOISE|SC_FLOAT:
      index-=2;
      {
        sInt time = sInt(Stack[index+0].f * 65536.0f);
        sInt curve = sInt(Stack[index+1].f * 65536.0f);
        sF32 v = sPerlin2D(time,curve);
        Stack[index++].f = v;
      }
      break;

    case SC_CLAMP|SC_FLOAT:
      {
        index-=2;
        sF32 min = Stack[index].f;
        sF32 max = Stack[index+1].f;
        for(sInt i=0;i<count;i++)
          Stack[index-count+i].f = sClamp(Stack[index-count+i].f,min,max);
      }
      break;
    case SC_LENGTH|SC_FLOAT:
      {
        index -=count;
        sF32 accu=0;
        for(sInt i=0;i<count;i++)
          accu += Stack[index+i].f*Stack[index+i].f;
        Stack[index++].f = sSqrt(accu);
      }
      break;
    case SC_NORMALIZE|SC_FLOAT:
      {
        sF32 accu=0;
        for(sInt i=0;i<count;i++)
          accu += Stack[index-count+i].f*Stack[index-count+i].f;
        if(accu<1e-20)
        {
          Stack[index-count].f = 1;
          for(sInt i=1;i<count;i++)
            Stack[index-count+i].f = 0;
        }
        else
        {
          accu = sRSqrt(accu);
          for(sInt i=0;i<count;i++)
            Stack[index-count+i].f *= accu;
        }
      }
      break;
    case SC_MAP|SC_FLOAT:
      {
        index-=2;
        sF32 min = Stack[index].f;
        sF32 max = Stack[index+1].f;
        for(sInt i=0;i<count;i++)
        {
          sF32 f = Stack[index-count+i].f*0.5+0.5;
          f = sClamp<sF32>(f,0,1);
          Stack[index-count+i].f = min + f*(max-min);
        }
      }
      break;


    case SC_ADD|SC_STRING:
      for(sInt i=0;i<count;i++)
      {
        sPoolString p;
        p.Add(Stack[index-count*2+i].s,Stack[index-count+i].s);
        Stack[index-count*2+i].s = p;
      }
      index-=count;
      break;

    case SC_PRINT|SC_INT:
      {
        sTextBuffer tb;
        const sChar *p = Stack[index-count-2].s;
        const sChar *fmt = Stack[index-1].s;
        if (fmt==0||fmt[0]==0)
          fmt=L"%d";
        tb.Print(p);
        tb.PrintF(fmt,Stack[index-count-1].i);
        for(sInt i=1;i<count;i++)
        {
          tb.Print(L" ");
          tb.PrintF(fmt,Stack[index-count-1+i].i);
        }
        Stack[index-count-2].s = sPoolString(tb.Get());
        index-=count+1;
      }
      break;

    case SC_PRINT|SC_FLOAT:
      {
        sTextBuffer tb;
        const sChar *p = Stack[index-count-2].s;
        const sChar *fmt = Stack[index-1].s;
        if (fmt==0||fmt[0]==0)
          fmt=L"%f";
        tb.Print(p);
        tb.PrintF(fmt,Stack[index-count-1].f);
        for(sInt i=1;i<count;i++)
        {
          tb.Print(L" ");
          tb.PrintF(fmt,Stack[index-count-1+i].f);
        }
        Stack[index-count-2].s = sPoolString(tb.Get());
        index-=count+1;
      }
      break;

    case SC_PRINT|SC_STRING:
      {
        sTextBuffer tb;
        const sChar *p = Stack[index-count-2].s;
        const sChar *fmt = Stack[index-1].s;
        if (fmt==0||fmt[0]==0)
          fmt=L"%s";
        tb.Print(p);
        tb.PrintF(fmt,Stack[index-count-1].s);
        for(sInt i=1;i<count;i++)
        {
          tb.Print(L" ");
          tb.PrintF(fmt,Stack[index-count-1+i].s);
        }
        Stack[index-count-2].s = sPoolString(tb.Get());
        index-=count+1;
      }
      break;

    case SC_PRINT|SC_COLOR:
      {
        sTextBuffer tb;
        const sChar *p = Stack[index-count-2].s;
        const sChar *fmt = Stack[index-1].s;
        if (fmt==0||fmt[0]==0)
          fmt=L"%08x";
        tb.Print(p);
        tb.PrintF(fmt,Stack[index-count-1].c);
        for(sInt i=1;i<count;i++)
        {
          tb.Print(L" ");
          tb.PrintF(fmt,Stack[index-count-1+i].c);
        }
        Stack[index-count-2].s = sPoolString(tb.Get());
        index-=count+1;
      }
      break;

    case SC_FTOC|SC_COLOR:
      {
        sVector4 c(Stack[index-4].f,Stack[index-3].f,Stack[index-2].f,Stack[index-1].f);
        index -= 3;
        Stack[index-1].c = c.GetColor();
      }
      break;
    case SC_CTOF|SC_FLOAT:
      {
        sVector4  c;
        c.InitColor(Stack[index-1].c);
        index += 3;
        Stack[index-4].f = c.x;
        Stack[index-3].f = c.y;
        Stack[index-2].f = c.z;
        Stack[index-1].f = c.w;
      }
      break;


    default:
      sFatal(L"unknown opcode");
    }
  }
}





void ScriptDump(const sU32 *code,sTextBuffer &tb)
{
  const sU32 *ptr = code;
  sInt index=0;
  const sChar *tname[] = { L"???",L"i",L"f",L"s",L"c" };
  for(;;)
  {
    sU32 cmd = *ptr++;
    sInt count = (cmd>>16)&0xffff;
    sInt type = (cmd>>8)&0xff;
    switch(cmd&0xffff)
    {
    case SC_ADD|SC_INT:
      tb.PrintF(L"+%d ",count);
      index-=count;
      break;
    case SC_ADD|SC_FLOAT:
      tb.PrintF(L"f+%d ",count);
      index-=count;
      break;
    case SC_SUB|SC_INT:
      tb.PrintF(L"-%d ",count);
      index-=count;
      break;
    case SC_SUB|SC_FLOAT:
      tb.PrintF(L"f-%d ",count);
      index-=count;
      break;
    case SC_MUL|SC_INT:
      tb.PrintF(L"*%d ",count);
      index-=count;
      break;
    case SC_MUL|SC_FLOAT:
      tb.PrintF(L"f*%d ",count);
      index-=count;
      break;
    case SC_DIV|SC_INT:
      tb.PrintF(L"/%d ",count);
      index-=count;
      break;
    case SC_DIV|SC_FLOAT:
      tb.PrintF(L"f/%d ",count);
      index-=count;
      break;
    case SC_MOD|SC_INT:
      tb.PrintF(L"%%%d ",count);
      index-=count;
      break;
    case SC_MOD|SC_FLOAT:
      tb.PrintF(L"f%%%d ",count);
      index-=count;
      break;

    case SC_SHIFTL|SC_INT:
      tb.PrintF(L"<<%d ",count);
      index-=count;
      break;
    case SC_SHIFTR|SC_INT:
      tb.PrintF(L">>%d ",count);
      index-=count;
      break;
    case SC_ROLL|SC_INT:
      tb.PrintF(L"<<<%d ",count);
      index-=count;
      break;
    case SC_ROLR|SC_INT:
      tb.PrintF(L">>>%d ",count);
      index-=count;
      break;

    case SC_DOT|SC_FLOAT:
      tb.PrintF(L"°%d ",count);
      index-=count*2-1;
      break;
    case SC_NEG|SC_INT:
      tb.PrintF(L"u-%d ",count);
      break;
    case SC_NEG|SC_FLOAT:
      tb.PrintF(L"uf-%d ",count);
      break;
    case SC_FTOI|SC_INT:
      tb.PrintF(L"ftoi%d ",count);
      break;
    case SC_ITOF|SC_FLOAT:
      tb.PrintF(L"itof%d ",count);
      break;

    case SC_NOT|SC_INT:
      tb.PrintF(L"! ");
      break;
    case SC_NOTNOT|SC_INT:
      tb.PrintF(L"? ");
      break;
    case SC_LOGAND|SC_INT:
      tb.PrintF(L"&& ");
      index--;
      break;
    case SC_LOGOR|SC_INT:
      tb.PrintF(L"|| ");
      index--;
      break;
    case SC_BINAND|SC_INT:
      tb.PrintF(L"& ");
      index--;
      break;
    case SC_BINOR|SC_INT:
      tb.PrintF(L"| ");
      index--;
      break;
    case SC_BINXOR|SC_INT:
      tb.PrintF(L"^ ");
      index--;
      break;

    case SC_EQ|SC_INT:
      tb.PrintF(L"==%d ");
      index--;
      break;
    case SC_EQ|SC_FLOAT:
      tb.PrintF(L"f==%d ");
      index--;
      break;
    case SC_NE|SC_INT:
      tb.PrintF(L"!=%d ");
      index--;
      break;
    case SC_NE|SC_FLOAT:
      tb.PrintF(L"f!=%d ");
      index--;
      break;
    case SC_GT|SC_INT:
      tb.PrintF(L">%d ");
      index--;
      break;
    case SC_GT|SC_FLOAT:
      tb.PrintF(L"f>%d ");
      index--;
      break;
    case SC_GE|SC_INT:
      tb.PrintF(L">=%d ");
      index--;
      break;
    case SC_GE|SC_FLOAT:
      tb.PrintF(L"f>=%d ");
      index--;
      break;
    case SC_LT|SC_INT:
      tb.PrintF(L"<%d ");
      index--;
      break;
    case SC_LT|SC_FLOAT:
      tb.PrintF(L"f<%d ");
      index--;
      break;
    case SC_LE|SC_INT:
      tb.PrintF(L"<=%d ");
      index--;
      break;
    case SC_LE|SC_FLOAT:
      tb.PrintF(L"f<=%d ");
      index--;
      break;

    case SC_B:
      tb.PrintF(L"B(%d) ",sS16(cmd>>16));
      break;
    case SC_BT:
      index--;
      tb.PrintF(L"BT(%d) ",sS16(cmd>>16));
      break;
    case SC_BF:
      index--;
      tb.PrintF(L"BF(%d) ",sS16(cmd>>16));
      break;
    case SC_STOP:
      tb.PrintF(L"STOP\n");
      if(index!=0)
        tb.PrintF(L"(stack error, %d left)\n",index); 
      return;

    case SC_LITERAL|SC_INT:
      tb.PrintF(L"[ ");
      for(sInt i=0;i<count;i++)
      {
        tb.PrintF(L"%d ",*(sS32 *)ptr);
        ptr++;
      }
      tb.PrintF(L"] ");
      index+=count;
      break;
    case SC_LITERAL|SC_FLOAT:
      tb.PrintF(L"[ ");
      for(sInt i=0;i<count;i++)
      {
        tb.PrintF(L"%f ",*(sF32 *)ptr);
        ptr++;
      }
      tb.PrintF(L"] ");
      index+=count;
      break;
    case SC_LITERAL|SC_COLOR:
      tb.PrintF(L"[ ");
      for(sInt i=0;i<count;i++)
      {
        tb.PrintF(L"%08x ",*(sU32 *)ptr);
        ptr++;
      }
      tb.PrintF(L"] ");
      index+=count;
      break;
    case SC_LITERAL|SC_STRING:
      tb.PrintF(L"[ ");
      for(sInt i=0;i<count;i++)
      {
        tb.PrintF(L"%q ",(const sChar *)readptr(ptr));
      }
      tb.PrintF(L"] ");
      index+=count;
      break;

    case SC_MAKELOCAL|SC_INT:
    case SC_MAKELOCAL|SC_FLOAT:
    case SC_MAKELOCAL|SC_STRING:
    case SC_MAKELOCAL|SC_COLOR:
      {
        ScriptSymbol *sym = (ScriptSymbol *) readptr(ptr);
        tb.PrintF(L"makelocal_%s(%s) ",tname[type],sym->Name);
      }
      break;

    case SC_MAKEGLOBAL|SC_INT:
    case SC_MAKEGLOBAL|SC_FLOAT:
    case SC_MAKEGLOBAL|SC_STRING:
    case SC_MAKEGLOBAL|SC_COLOR:
      {
        ScriptSymbol *sym = (ScriptSymbol *) readptr(ptr);
        tb.PrintF(L"makeglobal_%s(%s) ",tname[type],sym->Name);
      }
      break;

    case SC_IMPORT|SC_INT:
    case SC_IMPORT|SC_FLOAT:
    case SC_IMPORT|SC_STRING:
    case SC_IMPORT|SC_COLOR:
      {
        const sChar *name = (const sChar *) readptr(ptr);
        tb.PrintF(L"import_%s(%s) ",tname[type],name);
      }
      break;

    case SC_IMPORTDEFAULT|SC_INT:
    case SC_IMPORTDEFAULT|SC_FLOAT:
    case SC_IMPORTDEFAULT|SC_STRING:
    case SC_IMPORTDEFAULT|SC_COLOR:
      {
        const sChar *name = (const sChar *) readptr(ptr);
        tb.PrintF(L"importdefault_%s(%s) ",tname[type],name);
      }
      break;


    case SC_GETVARR|SC_INT:
    case SC_GETVARR|SC_FLOAT:
    case SC_GETVARR|SC_STRING:
    case SC_GETVARR|SC_COLOR:
      {
        sU32 cmd2 = *ptr++;
        sInt range0 = cmd2&0xffff;
        sInt range1 = cmd2>>16;
        ScriptSymbol *sym = (ScriptSymbol *) readptr(ptr);
        tb.PrintF(L"getvarr_%s(%s[%d..%d]) ",tname[type],sym->Name,range0,range1-1);
        index += (range1-range0);
      }
      break;

    case SC_GETVAR|SC_INT:
    case SC_GETVAR|SC_FLOAT:
    case SC_GETVAR|SC_STRING:
    case SC_GETVAR|SC_COLOR:
      {
        sInt range0 = 0;
        sInt range1 = count;
        ScriptSymbol *sym = (ScriptSymbol *) readptr(ptr);
        tb.PrintF(L"getvar_%s(%s[%d..%d]) ",tname[type],sym->Name,range0,range1-1);
        index += (range1-range0);
      }
      break;

    case SC_SETVARR|SC_INT:
    case SC_SETVARR|SC_FLOAT:
    case SC_SETVARR|SC_STRING:
    case SC_SETVARR|SC_COLOR:
      {
        sU32 cmd2 = *ptr++;
        sInt range0 = cmd2&0xffff;
        sInt range1 = cmd2>>16;
        ScriptSymbol *sym = (ScriptSymbol *) readptr(ptr);
        tb.PrintF(L"setvarr_%s(%s[%d..%d]) ",tname[type],sym->Name,range0,range1-1);
        index -= (range1-range0);
      }
      break;

    case SC_SETVAR|SC_INT:
    case SC_SETVAR|SC_FLOAT:
    case SC_SETVAR|SC_STRING:
    case SC_SETVAR|SC_COLOR:
      {
        sInt range0 = 0;
        sInt range1 = count;
        ScriptSymbol *sym = (ScriptSymbol *) readptr(ptr);
        tb.PrintF(L"setvar_%s(%s[%d..%d]) ",tname[type],sym->Name,range0,range1-1);
        index -= (range1-range0);
      }
      break;

    case SC_CLEARVARR|SC_INT:
    case SC_CLEARVARR|SC_FLOAT:
    case SC_CLEARVARR|SC_STRING:
    case SC_CLEARVARR|SC_COLOR:
      {
        sU32 cmd2 = *ptr++;
        sInt range0 = cmd2&0xffff;
        sInt range1 = cmd2>>16;
        ScriptSymbol *sym = (ScriptSymbol *) readptr(ptr);
        tb.PrintF(L"clearvar(%s[%d..%d]) ",sym->Name,range0,range1-1);
      }
      break;

    case SC_CLEARVAR|SC_INT:
    case SC_CLEARVAR|SC_FLOAT:
    case SC_CLEARVAR|SC_STRING:
    case SC_CLEARVAR|SC_COLOR:
      {
        sInt range0 = 0;
        sInt range1 = count;
        ScriptSymbol *sym = (ScriptSymbol *) readptr(ptr);
        tb.PrintF(L"clearvar(%s[%d..%d]) ",sym->Name,range0,range1-1);
      }
      break;

    case SC_SPLINE|SC_FLOAT:
      {
        ScriptSymbol *sym = (ScriptSymbol *) readptr(ptr);
        tb.PrintF(L"spline%d(%s) ",count,sym->Name);
        index--;
        index += count;
      }
      break;

    case SC_CAT|SC_INT:
    case SC_CAT|SC_FLOAT:
    case SC_CAT|SC_STRING:
    case SC_CAT|SC_COLOR:
      tb.Print(L"cat ");
      break;

    case SC_INDEX|SC_INT:
    case SC_INDEX|SC_FLOAT:
    case SC_INDEX|SC_STRING:
    case SC_INDEX|SC_COLOR:
      tb.Print(L"index ");
      index -= count;
      break;

    case SC_SPLICE|SC_INT:
    case SC_SPLICE|SC_FLOAT:
    case SC_SPLICE|SC_STRING:
    case SC_SPLICE|SC_COLOR:
      {
        sU32 cmd2 = *ptr++;
        sInt range0 = cmd2&0xffff;
        sInt range1 = cmd2>>16;
        index -= count;
        index += range1-range0;
        tb.PrintF(L"splice%d[%d..%d] ",count,range0,range1-1);
      }
      break;

    case SC_DUP|SC_INT:
    case SC_DUP|SC_FLOAT:
    case SC_DUP|SC_STRING:
    case SC_DUP|SC_COLOR:
      tb.PrintF(L"dup%d ",count);
      index += count-1;
      break;

    case SC_COND:     // c ? a : b
      tb.PrintF(L"cond%d ",count);
      index--;
      index -= count*2;
      index+=count;
      break;

    case SC_ABS|SC_FLOAT:
      tb.PrintF(L"abs ");
      break;
    case SC_SIGN|SC_FLOAT:
      tb.PrintF(L"sign ");
      break;
    case SC_MAX|SC_FLOAT:
      tb.PrintF(L"max ");
      index--;
      break;
    case SC_MIN|SC_FLOAT:
      tb.PrintF(L"min ");
      index--;
      break;
    case SC_SIN|SC_FLOAT:
      tb.PrintF(L"sin ");
      break;
    case SC_COS|SC_FLOAT:
      tb.PrintF(L"cos ");
      break;
    case SC_SIN1|SC_FLOAT:
      tb.PrintF(L"sin1 ");
      break;
    case SC_COS1|SC_FLOAT:
      tb.PrintF(L"cos1 ");
      break;
    case SC_TAN|SC_FLOAT:
      tb.PrintF(L"tan ");
      break;
    case SC_ATAN|SC_FLOAT:
      tb.PrintF(L"atan ");
      break;
    case SC_ATAN2|SC_FLOAT:
      tb.PrintF(L"atan2 ");
      index--;
      break;
    case SC_SQRT|SC_FLOAT:
      tb.PrintF(L"sqrt ");
      break;
    case SC_POW|SC_FLOAT:
      tb.PrintF(L"pow ");
      index--;
      break;
    case SC_EXP|SC_FLOAT:
      tb.PrintF(L"exp ");
      break;
    case SC_LOG|SC_FLOAT:
      tb.PrintF(L"log ");
      break;
    case SC_SMOOTHSTEP|SC_FLOAT:
      tb.PrintF(L"smoothstep ");
      break;
    case SC_RAMPUP|SC_FLOAT:
      tb.PrintF(L"rampup ");
      break;
    case SC_RAMPDOWN|SC_FLOAT:
      tb.PrintF(L"rampdown ");
      break;
    case SC_TRIANGLE|SC_FLOAT:
      tb.PrintF(L"triangle ");
      break;
    case SC_PULSE|SC_FLOAT:
      tb.PrintF(L"pulse ");
      index--;
      break;
    case SC_EXPEASE|SC_FLOAT:
      tb.PrintF(L"expease ");
      index--;
      break;
    case SC_FADEINOUT|SC_FLOAT:
      tb.PrintF(L"fadeinout ");
      break;

    case SC_CLAMP|SC_FLOAT:
      tb.PrintF(L"clamp ");
      index-=2;
      break;
    case SC_LENGTH|SC_FLOAT:
      tb.PrintF(L"length%d ",count);
      index -=count-1;
      break;
    case SC_NORMALIZE|SC_FLOAT:
      tb.PrintF(L"normalize%d ",count);
      break;
    case SC_MAP|SC_FLOAT:
      tb.PrintF(L"map%d ",count);
      index-=2;
      break;


    case SC_ADD|SC_STRING:
      tb.PrintF(L"s+%d ",count);
      index-=count;
      break;

    case SC_PRINT|SC_INT:
    case SC_PRINT|SC_FLOAT:
    case SC_PRINT|SC_STRING:
    case SC_PRINT|SC_COLOR:
      tb.PrintF(L"print%d ",count);
      index-=count+1;
      break;

    case SC_FTOC|SC_COLOR:
      tb.PrintF(L"ftoc ");
      index -= 3;
      break;
    case SC_CTOF|SC_FLOAT:
      tb.PrintF(L"ctof ");
      index += 3;
      break;

    default:
      sFatal(L"unknown opcode");
    }
    if(index==0)
      tb.PrintF(L"\n");
  }
}

ScriptCode::ScriptCode(const sChar *src,sInt line)
{
  Code = 0;
  sInt len = sGetStringLen(src);
  Source = new sChar[len+1];
  SourceLine = line;
  sCopyString(Source,src,len+1);
}

ScriptCode::~ScriptCode()
{
  delete[] Source;
  delete[] Code;
}

static sBool iswhite(const sChar *s)
{
  if(s==0) return 1;
  while(*s)
  {
    if(!(*s==' ' || *s=='\n' || *s=='\r' || *s=='\t'))
      return 0;
    s++;
  }
  return 1;
}

sBool ScriptCode::Execute(ScriptContext *ctx)
{
  ErrorMsg[0] = 0;
  if(Code==0)
  {
    if(iswhite(Source)) return 1;

    ScriptCompiler comp;
    Code = comp.Parse(ctx,Source,SourceLine);
    if(!Code)
    {
      ErrorMsg = comp.ErrorMsg;
      sLogF(L"Script",ErrorMsg);
    }
  }
  if(Code)
  {
    if(!Exe(ctx))
      sDeleteArray(Code);
  }
  return Code!=0;
}

/****************************************************************************/
/***                                                                      ***/
/***   Compiler                                                           ***/
/***                                                                      ***/
/****************************************************************************/

ScriptCompiler::Statement *ScriptCompiler::NewStat(sInt code)
{
  Statement *stat = Pool->Alloc<Statement>(1);
  sClear(*stat);
  stat->Kind = code;
  return stat;
}

ScriptCompiler::Expression *ScriptCompiler::NewExpr(sInt code,Expression *a,Expression *b,Expression *c,sInt lit)
{
  Expression *expr = Pool->Alloc<Expression>(1);
  sClear(*expr);
  expr->Kind = code;
  expr->a = a;
  expr->b = b;
  expr->c = c;
  expr->d = 0;
  if(lit>0)
  {
    expr->Literal = Pool->Alloc<sU32>(lit);
  }
  return expr;
};

ScriptCompiler::Expression *ScriptCompiler::NewExprCT(sInt code,Expression *a,Expression *b,Expression *c,sInt lit)
{
  Expression *expr = NewExpr(code,a,b,c,lit);

  if(expr->a)
  {
    if(expr->a->Type==0)
      Scan.Error(L"no type");
    if(expr->b)
    {
      if(expr->a->Type==ScriptTypeColor && expr->a->Count==1)
      {
        expr->a = NewExpr(SC_CTOF,expr->a,0,0,0);
        expr->a->Type = ScriptTypeFloat;
        expr->a->Count = 4;
      }
      if(expr->b->Type==ScriptTypeColor && expr->b->Count==1)
      {
        expr->b = NewExpr(SC_CTOF,expr->b,0,0,0);
        expr->b->Type = ScriptTypeFloat;
        expr->b->Count = 4;
      }
      if(expr->a->Type!=expr->b->Type)
      {
        if(expr->a->Type==ScriptTypeInt && expr->b->Type==ScriptTypeFloat)
          PromoteType(&expr->a);
        else if(expr->b->Type==ScriptTypeInt && expr->a->Type==ScriptTypeFloat)
          PromoteType(&expr->b);
        else
          Scan.Error(L"type mismatch");
      }
      if(expr->a->Count!=expr->b->Count)
      {
        if(expr->a->Count==1 && expr->b->Count>1)
          PromoteCount(&expr->a,expr->b->Count);
        else if(expr->b->Count==1 && expr->a->Count>1)
          PromoteCount(&expr->b,expr->a->Count);
        else 
          Scan.Error(L"count mismatch");
      }
    }
    expr->Type = expr->a->Type;
    expr->Count= expr->a->Count;
  }

  return expr;
}

void ScriptCompiler::PromoteType(Expression **p)
{
  sVERIFY((*p)->Type==ScriptTypeInt);
  Expression *expr = NewExpr(SC_ITOF,*p);
  expr->Type = ScriptTypeFloat;
  expr->Count = expr->a->Count;
  *p = expr;
}

void ScriptCompiler::PromoteCount(Expression **p,sInt count)
{
  sVERIFY((*p)->Count==1);
  sVERIFY(count > 1);
  Expression *expr = NewExpr(SC_DUP,*p);
  expr->Type = (*p)->Type;
  expr->Count = count;
  *p = expr;
}



/****************************************************************************/

void ScriptCompiler::_Range(sInt &r0,sInt &r1,ScriptCompiler::Expression *&ei)
{
  r0 = r1 = 0;
  ei = 0;

  if(Scan.IfToken('['))
  {
    if(Scan.Token==sTOK_INT)
    {
      r0 = Scan.ScanInt();
      if(Scan.IfToken(sTOK_ELLIPSES))
        r1 = Scan.ScanInt()+1;
      else
        r1 = r0+1;
    }
    else
    {
      ei = _Expr();
    }
    Scan.Match(']');
  }
  else if(Scan.IfToken('.'))
  {
    sPoolString swizzle;
    Scan.ScanName(swizzle);
    sInt n[4];
    sInt c = 0;
    const sChar *s = swizzle;
    while(*s)
    {
      if(c==4)
        Scan.Error(L"swizzle mask too long");
      if(Scan.Errors)
        break;
      switch(*s)
      {
        case 'x': n[c++] = 0; break;
        case 'y': n[c++] = 1; break;
        case 'z': n[c++] = 2; break;
        case 'w': n[c++] = 3; break;
        case 'r': n[c++] = 0; break;
        case 'g': n[c++] = 1; break;
        case 'b': n[c++] = 2; break;
        case 'a': n[c++] = 3; break;
        default: Scan.Error(L"swizzle mask may only contain .xyzw or .rgba");
      }
      s++;
    }
    if(Scan.Errors) return;
    sVERIFY(c>=1 && c<=4);
    r0 = n[0];
    r1 = r0+c;
    for(sInt i=1;i<c;i++)
      if(n[0]+i!=n[i])
        Scan.Error(L"can't swizzle. 'xyzw' or 'rgba' must be in that exact order!");
  }
  else
    Scan.Error(L"expected [x], [x..x] or .xyz");
}

void ScriptCompiler::CheckFunc(Expression *expr,ScriptFunc *func)
{
  sInt argcount = 0;
  Expression *ae[4];
  sClear(ae);

  if(expr->a) ae[argcount++] = expr->a;
  if(expr->b) ae[argcount++] = expr->b;
  if(expr->c) ae[argcount++] = expr->c;
  if(expr->d) ae[argcount++] = expr->d;

  if(func->Code)
  {
    expr->Kind = SC_CALL;
    expr->Code = func->Code;
  }
  else
  {
    expr->Kind = func->Primitive;
  }

  if(argcount==func->ArgCount && Scan.Errors==0)
  {
    sInt ac[10];
    sClear(ac);
    for(sInt i=0;i<argcount;i++)
    {
      sInt count = func->Args[i].VarCount;
      sVERIFY(count<10);
      if(count==0)
        count = func->Args[i].Count;
      else
      {
        if(ac[count]==0)
          ac[count] = ae[i]->Count;
        count = ac[count];
      }
      if(ae[i]->Type==1 && func->Args[i].Type==2)
        PromoteType(&ae[i]);
      if(ae[i]->Type!=func->Args[i].Type)
        Scan.Error(L"wrong type for argument %d of function %q",i+1,func->Name);
      if(ae[i]->Count!=count)
        Scan.Error(L"wrong count for argument %d of function %q",i+1,func->Name);
    }
    expr->Type = func->Result.Type;
    sInt count = func->Result.VarCount;
    if(count>0)
    {
      sVERIFY(count<10);
      count = ac[count];
    }
    else
    {
      count = func->Result.Count;
    }
    expr->a = ae[0];
    expr->b = ae[1];
    expr->c = ae[2];
    expr->d = ae[3];
    expr->Count = count;
  }
  else
    Scan.Error(L"wrong number of arguments for function %q",func->Name);
}

ScriptCompiler::Expression *ScriptCompiler::_Name(sInt cmd)
{
  sPoolString name;
  Scan.ScanName(name);
  Expression *expr = NewExpr(cmd);
  expr->Symbol = ctx->AddSymbol(name);
  expr->Count = expr->Symbol->CCount;
  expr->Type = expr->Symbol->CType;
  expr->Range0 = 0;
  expr->Range1 = expr->Count;
  sInt checkrange = 1;

  if(cmd==SC_GETVARR || cmd==SC_SETVARR)
  {
    if(expr->Symbol->CType==0)
    {
      Scan.Error(L"unknown symbol %q",name);
      return expr;
    }
  }
  if(cmd==SC_GETVARR && expr->Symbol->Value)
  {
    ScriptFunc *func = expr->Symbol->Value->Func;
    ScriptSpline *spl = expr->Symbol->Value->Spline;
    if(func)
    {
      Scan.Match('(');
      if(Scan.Token!=')') 
      {
        expr->a = _Expr();
        if(Scan.IfToken(','))
          expr->b = _Expr();
        if(Scan.IfToken(','))
          expr->c = _Expr();
        if(Scan.IfToken(','))
          expr->d = _Expr();
      }
      CheckFunc(expr,func);
      Scan.Match(')');
    }
    if(spl)
    {
      Scan.Match('(');
      expr->a = _Expr();
      if(expr->a->Type==1)
        PromoteType(&expr->a);
      if(expr->a->Count!=1)
        Scan.Error(L"scalar only");
      expr->Kind = SC_SPLINE;
      Scan.Match(')');
      expr->Type = 2;
      expr->Count= spl->Count;
      checkrange = 0;     // not implemented! use postfix []
    }
  }

  if(checkrange)
  {
    sInt r0,r1;
    Expression *ei;
    if(Scan.Token=='[' || Scan.Token=='.')
    {
      _Range(r0,r1,ei);
      if(ei)
      {
        expr = NewExpr(SC_INDEX,expr,ei);
        expr->Type = expr->a->Type;
        expr->Count = 1;
      }
      else
      {
        expr->Range0 = r0;
        expr->Range1 = r1;
        expr->Count = r1-r0;
      }
    }
  }
  return expr;
}

ScriptCompiler::Expression *ScriptCompiler::_Value()
{
  if(Scan.IfToken('-'))
    return NewExprCT(SC_NEG,_Value());
  else if(Scan.IfToken('!'))
    return NewExprCT(SC_NOT,_Value());
  else if(Scan.IfToken('?'))
    return NewExprCT(SC_NOTNOT,_Value());

  // value

  Expression *expr = 0;

  if(Scan.IfToken('('))
  {
    expr = _Expr();
    Scan.Match(')');
  }
  else if(Scan.IfToken('['))
  {
    expr = _Expr();
    while(Scan.IfToken(','))
    {
      expr = NewExpr(SC_CAT,expr,_Expr());
      if(Scan.Errors==0)
      {
        if(expr->a->Type==0)
          Scan.Error(L"no type");
        if(expr->a->Type!=expr->b->Type)
        {
          if(expr->a->Type==1)
            PromoteType(&expr->a);
          else if(expr->b->Type==1)
            PromoteType(&expr->b);
          else 
            Scan.Error(L"type error");
        }
        expr->Type = expr->a->Type;
        expr->Count = expr->a->Count + expr->b->Count;
        if(expr->Count>255)
          Scan.Error(L"array too large");
      }
    }
    Scan.Match(']');
  }
  else if(Scan.Token==sTOK_INT)
  {
    expr = NewExpr(SC_LITERAL,0,0,0,1);
    expr->Type = ScriptTypeInt;
    expr->Count = 1;
    expr->LiteralInt[0] = Scan.ScanInt();
  }
  else if(Scan.Token==sTOK_FLOAT)
  {
    expr = NewExpr(SC_LITERAL,0,0,0,1);
    expr->Type = ScriptTypeFloat;
    expr->Count = 1;
    expr->LiteralFloat[0] = Scan.ScanFloat();
  }
  else if(Scan.Token==sTOK_STRING)
  {
    expr = NewExpr(SC_LITERAL,0,0,0,1);
    expr->Type = ScriptTypeString;
    expr->Count = 1;
    Scan.ScanString(expr->LiteralString[0]);
  }
  else if(Scan.Token==sTOK_NAME)
  {
    expr = _Name(SC_GETVARR);
  }
  else if(Scan.IfToken(TOK_PI))
  {
    expr = NewExpr(SC_LITERAL,0,0,0,2);
    expr->Type = ScriptTypeFloat;
    expr->Count = 1;
    expr->LiteralFloat[0] = sPIF;
  }
  else if(Scan.IfToken(TOK_PI2))
  {
    expr = NewExpr(SC_LITERAL,0,0,0,1);
    expr->Type = ScriptTypeFloat;
    expr->Count = 1;
    expr->LiteralFloat[0] = sPI2F;
  }
  else if(Scan.IfToken(TOK_INT))
  {
    Scan.Match('(');
    expr = _Expr();
    if(expr->Type==ScriptTypeInt)
    {
      // ok
    }
    else if(expr->Type==ScriptTypeFloat)
    {
      expr = NewExpr(SC_FTOI,expr,0,0,0);
      expr->Type= ScriptTypeInt;
      expr->Count = expr->a->Count;
    }
    else
    {
      Scan.Error(L"type error");
    }
    Scan.Match(')');
  }
  else if(Scan.IfToken(TOK_FLOAT))
  {
    Scan.Match('(');
    expr = _Expr();
    if(expr->Type==ScriptTypeFloat)
    {
      // ok
    }
    else if(expr->Type==ScriptTypeInt)
    {
      expr = NewExpr(SC_ITOF,expr,0,0,0);
      expr->Type= ScriptTypeFloat;
      expr->Count = expr->a->Count;
    }
    else
    {
      Scan.Error(L"type error");
    }
    Scan.Match(')');
  }
  else if(Scan.IfToken(TOK_FLOAT))
  {
  }
  else
  {
    Scan.Error(L"value expected");
  }

  // postfixes

  if(Scan.Token=='[')
  {
    sInt r0,r1;
    Expression *ei;
    _Range(r0,r1,ei);
    if(ei)
    {
      expr = NewExpr(SC_INDEX,expr,ei);
      expr->Type = expr->a->Type;
      expr->Count = 1;
    }
    else
    {
      expr = NewExprCT(SC_SPLICE,expr);
      expr->Range0 = r0;
      expr->Range1 = r1;
      expr->Count = r1-r0;
    }
  }

  if(expr==0)
  {
    expr = NewExpr(SC_LITERAL,0,0,0,1);
    expr->Type = 2;
    expr->Count = 1;
    ((sF32 *)expr->Literal)[0] = 0;
  }

  return expr;
}

// mode 0 = normal check
// mode 1 = scalar output
// mode 2 = float only
// mode 4 = int only
// mode 8 = int output
//   0x10 = scalar only
//   0x20 = allow color & string

// modes
// 0x0001 = int allowed
// 0x0002 = float allowed
// 0x0004 = string allowed
// 0x0008 = color allowed
//
// 0x0100 = scalar output (dot product)
// 0x0200 = int output (compare)
// 0x0400 = scalar only

sInt BinaryOp(sInt token,sInt &pri,sInt &mode)
{
  mode = 0;
  switch(token)
  {
  case '*':               pri= 1; mode=0x0003; return SC_MUL;
  case '/':               pri= 1; mode=0x0003; return SC_DIV;
  case '%':               pri= 1; mode=0x0003; return SC_MOD;
  case 0xb0:              pri= 1; mode=0x0103; return SC_DOT;   // '°'

  case TOK_TILDETILDE:    pri= 2; mode=0x000f; return SC_CAT;

  case '+':               pri= 3; mode=0x0007; return SC_ADD;
  case '-':               pri= 3; mode=0x0003; return SC_SUB;

  case TOK_SHIFTL:        pri= 4; mode=0x0001; return SC_SHIFTL;
  case TOK_SHIFTR:        pri= 4; mode=0x0001; return SC_SHIFTR;
  case TOK_ROLL:          pri= 4; mode=0x0001; return SC_ROLL;
  case TOK_ROLR:          pri= 4; mode=0x0001; return SC_ROLR;

  case '&':               pri= 4; mode=0x0401; return SC_BINAND;
  case '^':               pri= 5; mode=0x0401; return SC_BINXOR;
  case '|':               pri= 6; mode=0x0401; return SC_BINOR;

  case '~':               pri= 7; mode=0x0003; return SC_TILDE;

  case '>':               pri= 8; mode=0x0703; return SC_GT;
  case '<':               pri= 8; mode=0x0703; return SC_LT;
  case sTOK_GE:           pri= 8; mode=0x0703; return SC_GE;
  case sTOK_LE:           pri= 8; mode=0x0703; return SC_LE;
  case sTOK_EQ:           pri= 9; mode=0x0703; return SC_EQ;
  case sTOK_NE:           pri= 9; mode=0x0703; return SC_NE;

  case TOK_LOGAND:        pri=10; mode=0x0401; return SC_BINAND;
  case TOK_LOGOR:         pri=11; mode=0x0401; return SC_BINOR;

  case '?':               pri=12; mode=0x000f; return SC_COND;

  }
  return 0;
}

static const sInt EXPRLEVEL = 12;

ScriptCompiler::Expression *ScriptCompiler::_Expr()
{
  return _ExprR(EXPRLEVEL);
}

ScriptCompiler::Expression *ScriptCompiler::_ExprR(sInt maxlevel)
{
  sInt op,pri,mode;
  sInt level;

  Expression *expr = _Value();

  for(level = 1; level<=maxlevel;level++)
  {
    while(!Scan.Errors)
    {
      op = BinaryOp(Scan.Token,pri,mode);
      if(!op || pri!=level)
        break;
      Scan.Scan();

      if(op==SC_MOD && expr->Type==ScriptTypeString)
        op = SC_PRINT;

      if(op==SC_COND)
      {
        Expression *a,*b,*c;
        
        c = expr;
        a = _ExprR(level-1);
        Scan.Match(':');
        b = _ExprR(level-1);

        expr = NewExpr(op,a,b,c);     // c ? a : b
        if(Scan.Errors==0)
        {
          if(expr->a->Type==0)
            Scan.Error(L"no type");
          if(expr->b->Type!=expr->a->Type)
            Scan.Error(L"type mismatch");
          if(expr->b->Count!=expr->a->Count)
            Scan.Error(L"count mismatch");
          if(expr->c->Type!=1)
            Scan.Error(L"int only");
          if(expr->c->Count!=1)
            Scan.Error(L"scalar only");
          expr->Type = expr->a->Type;
          expr->Count = expr->a->Count;
        }
      }
      else if(op==SC_TILDE)
      {
        expr = NewExpr(0,expr);
        sPoolString name;
        ScriptFunc *func = 0;
        Scan.ScanName(name);
        ScriptSymbol *sym = ctx->AddSymbol(name);
        if(sym->Value==0 || sym->Value->Func==0)
          Scan.Error(L"after tilde, %q is not a function",name);
        else
          func = sym->Value->Func;

        if(Scan.IfToken('('))
        {
          expr->b = _Expr();
          if(Scan.IfToken(','))
            expr->c = _Expr();
          Scan.Match(')');
        }
        if(Scan.Errors==0)
          CheckFunc(expr,func);
        level = 1;
      }
      else if(op==SC_CAT)
      {
        expr = NewExpr(op,expr,_ExprR(level-1));
        if(expr->a->Type!=expr->b->Type)
          Scan.Error(L"type mismatch");
        expr->Type = expr->a->Type;
        expr->Count = expr->a->Count + expr->b->Count;
      }
      else if(op==SC_PRINT)
      {
        Expression *b, *c;
        b = _ExprR(level-1);

        if (Scan.IfToken(':'))
        {
          c=_ExprR(level-1);
          if(c->Type!=ScriptTypeString)
            Scan.Error(L"string only");
          if(c->Count!=1)
            Scan.Error(L"scalar only");
        }
        else
        {
          // create empty format string (will result in default formatting for given type)
          c = NewExpr(SC_LITERAL,0,0,0,1);
          c->Type = ScriptTypeString;
          c->Count = 1;
          c->LiteralString[0]=L"";
        }

        expr = NewExpr(op,expr,b,c);
        if(expr->a->Count!=1)
          Scan.Error(L"scalar required");
        expr->Type = expr->a->Type;
        expr->Count = expr->a->Count;
      }
      else
      {
        expr = NewExprCT(op,expr,_ExprR(level-1));

        if((!(mode&0x0001)) && expr->Type==ScriptTypeInt   ) 
          Scan.Error(L"type int not allowed for this operation");
        if((!(mode&0x0002)) && expr->Type==ScriptTypeFloat ) 
          Scan.Error(L"type float not allowed for this operation");
        if((!(mode&0x0004)) && expr->Type==ScriptTypeString) 
          Scan.Error(L"type strign not allowed for this operation");
        if((!(mode&0x0008)) && expr->Type==ScriptTypeColor ) 
          Scan.Error(L"type color not allowed for this operation");
        if(mode&0x0100)
          expr->Count = 1;
        if(mode&0x0200)
          expr->Type = ScriptTypeInt;
        if((mode&0x0400) && expr->Count!=1) 
          Scan.Error(L"scalar required");
      }
    }
  }
  return expr;
}


ScriptCompiler::Statement *ScriptCompiler::_VarDef(sInt mode)
{
  sInt type = 0;
  sInt count = 1;

  // name : type[x]

  sArray<sPoolString> names;

  do
  {
    sPoolString name;
    Scan.ScanName(name);
    names.AddTail(name);
  }
  while(!Scan.Errors && Scan.IfToken(','));
  Scan.IfToken(',');


  Scan.Match(':');

  if(Scan.IfToken(TOK_INT))
    type = ScriptTypeInt;
  else if(Scan.IfToken(TOK_FLOAT))
    type = ScriptTypeFloat;
  else if(Scan.IfToken(TOK_STRING))
    type = ScriptTypeString;
  else if(Scan.IfToken(TOK_COLOR))
    type = ScriptTypeColor;
  else
    Scan.Error(L"unknown type");

  if(Scan.IfToken('['))
  {
    count = Scan.ScanInt();
    Scan.Match(']');
    if(count<1 && count>255)
      Scan.Error(L"array dimension out of range, %d",count);
  }

  Expression *initexpr = 0;
  if(Scan.IfToken('='))
    initexpr = _Expr();
  Scan.Match(';');

  Statement *block = NewStat(STAT_BLOCK);
  Statement **next = &block->Child;

  sPoolString *namep;
  sFORALL(names,namep)
  {
    if(mode!=2)
    {
      Statement *r = NewStat(mode==1 ? STAT_GLOBAL : STAT_LOCAL);
      *next = r;
      next = &r->Next;
      
      r->Expr = NewExpr(0);
      r->Expr->Symbol = ctx->AddSymbol(*namep);
      r->Expr->Symbol->CType = type;
      r->Expr->Symbol->CCount = count;
      r->Expr->Type = type;
      r->Expr->Count = count;

      if(initexpr)
      {
        Expression *expr = NewExpr(SC_SETVARR);
        expr->Symbol = r->Expr->Symbol;
        expr->Count = expr->Symbol->CCount;
        expr->Type = expr->Symbol->CType;
        expr->Range0 = 0;
        expr->Range1 = expr->Count;
        
        r->Expr->a = _AssignTo(initexpr,expr);
      }
    }
    else
    {
      Statement *r = NewStat(STAT_IMPORT);
      *next = r;
      next = &r->Next;

      r->Expr = NewExpr(SC_IMPORT);
      r->Expr->Symbol = ctx->AddSymbol(*namep);
      r->Expr->Symbol->CType = type;
      r->Expr->Symbol->CCount = count;
      r->Expr->Type = type;
      r->Expr->Count = count;
      if(initexpr)
      {
        if(initexpr->Type==ScriptTypeInt && type==ScriptTypeFloat)
        {
          initexpr = NewExpr(SC_ITOF,initexpr);
          initexpr->Type = ScriptTypeFloat;
          initexpr->Count = initexpr->a->Count;
        }
        if(initexpr->Type!=type || initexpr->Count!=count)
        {
          Scan.Error(L"type mismatch");
        }

        r->Expr->Kind = SC_IMPORTDEFAULT;
        r->Expr->a = initexpr;
      }
    }
  }

  return block;
}

ScriptCompiler::Statement *ScriptCompiler::_Assign()
{
  Expression *a = _Name(SC_SETVARR);

  Statement *r = NewStat(STAT_ASSIGN);
  Scan.Match('=');
  r->Expr = _AssignTo(a);
  Scan.Match(';');
  return r;
}

ScriptCompiler::Expression *ScriptCompiler::_AssignTo(ScriptCompiler::Expression *b)
{
  return _AssignTo(_Expr(),b);
}

ScriptCompiler::Expression *ScriptCompiler::_AssignTo(ScriptCompiler::Expression *a,ScriptCompiler::Expression *b)
{
  // b = a;
  // a is evaluated first!

  Expression *expr = NewExpr(SC_ASSIGN,a,b);
  if(Scan.Errors==0)
  {
    if(expr->b->Type==ScriptTypeColor && expr->a->Type!=ScriptTypeColor)
    {
      if(expr->a->Type==ScriptTypeInt)
        PromoteType(&expr->a);
      if(expr->a->Count==1)
        PromoteCount(&expr->a,4);
      if(expr->a->Type==ScriptTypeFloat && expr->a->Count==4)
      {
        expr->a = NewExpr(SC_FTOC,expr->a);
        expr->a->Type = ScriptTypeColor;
        expr->a->Count = 1;
      }
    }
    if(expr->b->Type!=ScriptTypeColor && expr->a->Type==ScriptTypeColor && expr->a->Count==1)
    {
      expr->a = NewExpr(SC_CTOF,expr->a);
      expr->a->Type = ScriptTypeFloat;
      expr->a->Count = 4;
    }
    if(expr->a->Type!=expr->b->Type)
    {
      if(expr->a->Type==1)      // this is the src, remember, b = a;
        PromoteType(&expr->a);  // can't promote dest..
    }
    if(expr->b->Count>1 && expr->a->Count==1)
      PromoteCount(&expr->a,expr->b->Count);
    if(expr->a->Type!=expr->b->Type)
      Scan.Error(L"type mismatch");
    if(expr->a->Count!=expr->b->Count)
      Scan.Error(L"count mismatch");
  }
  return expr;
}

ScriptCompiler::Statement *ScriptCompiler::_If()
{
  Statement *r = NewStat(STAT_IF);
  Scan.Match(TOK_IF);
  Scan.Match('(');
  r->Expr = _Expr();
  Scan.Match(')');
  if(Scan.Errors) return r;
  if(r->Expr->Type!=1)
    Scan.Error(L"only integer");
  if(r->Expr->Count!=1)
    Scan.Error(L"only scalar");
  r->Child = _Block();
  if(Scan.IfToken(TOK_ELSE))
    r->Alt = _Block();
  return r;
}

ScriptCompiler::Statement *ScriptCompiler::_Do()
{
  Statement *r = NewStat(STAT_DO);
  Scan.Match(TOK_DO);
  r->Child = _Block();
  Scan.Match(TOK_WHILE);
  Scan.Match('(');
  r->Expr = _Expr();
  Scan.Match(')');
  Scan.Match(';');
  if(Scan.Errors) return r;
  if(r->Expr->Type!=1)
    Scan.Error(L"only integer");
  if(r->Expr->Count!=1)
    Scan.Error(L"only scalar");
  return r;
}

ScriptCompiler::Statement *ScriptCompiler::_While()
{
  Statement *r = NewStat(STAT_WHILE);
  Scan.Match(TOK_WHILE);
  Scan.Match('(');
  r->Expr = _Expr();
  Scan.Match(')');
  if(Scan.Errors) return r;
  if(r->Expr->Type!=1)
    Scan.Error(L"only integer");
  if(r->Expr->Count!=1)
    Scan.Error(L"only scalar");
  r->Child = _Block();
  return r;
}

ScriptCompiler::Statement *ScriptCompiler::_Block()
{
  Statement *r;
  if(Scan.IfToken('{'))
  {
    r = NewStat(STAT_BLOCK);
    Statement **p = &r->Child;
    while(!Scan.Errors && !Scan.IfToken('}'))
    {
      Statement *s = _Statement();
      *p = s;
      p = &s->Next;
    }
  }
  else
  {
    r = _Statement();
  }
  return r;
}

ScriptCompiler::Statement *ScriptCompiler::_Statement()
{
  Statement *r = 0;
  sChar c;

  switch(Scan.Token)
  {
  case TOK_IF:  r = _If();   break;
  case TOK_DO:  r = _Do();   break;
  case TOK_WHILE:  r = _While();   break;

  case sTOK_NAME:
    c = Scan.DirtyPeek();
    if(c==':' || c==',')
      r = _VarDef(0);
    else
      r = _Assign();
    break;
  case TOK_GLOBAL:
    Scan.Match(TOK_GLOBAL);
    r = _VarDef(1);
    break;
  case TOK_IMPORT:
    Scan.Match(TOK_IMPORT);
    r = _VarDef(2);
    break;
  default:
    r = _Assign();
    break;
  }
  return r;
}

ScriptCompiler::Statement *ScriptCompiler::_Global()
{
  Statement *r = NewStat(STAT_BLOCK);
  Statement **p = &r->Child;
  while(!Scan.Errors && Scan.Token!=sTOK_END)
  {
    Statement *s = _Statement();
    *p = s;
    p = &s->Next;
  }

  return r;
}

/****************************************************************************/

void ScriptCompiler::PrintType(sInt type,sInt count)
{
  if(type==ScriptTypeInt)
    sDPrintF(L"int");
  else if(type==ScriptTypeFloat)
    sDPrintF(L"float");
  else if(type==ScriptTypeString)
    sDPrintF(L"string");
  else if(type==ScriptTypeColor)
    sDPrintF(L"color");
  else 
    sFatal(L"unknown type %d",type);
  if(count!=1)
    sDPrintF(L"[%d]",count);
}

void ScriptCompiler::PrintStat(Statement *stat,sInt indent)
{
  switch(stat->Kind)
  {
  case STAT_BLOCK:
    sDPrintF(L"%_{\n",indent);
    stat = stat->Child;
    while(stat)
    {
      PrintStat(stat,indent+2);
      stat = stat->Next;
    }
    sDPrintF(L"%_}\n",indent);
    break;
  case STAT_GLOBAL:
  case STAT_LOCAL:
    sDPrintF(L"%_",indent);
    if(stat->Kind==STAT_GLOBAL) sDPrintF(L"global ");
    sDPrintF(L"%s:",stat->Expr->Symbol->Name);
    PrintType(stat->Expr->Type,stat->Expr->Count);
    if(stat->Expr->a)
    {
      sDPrintF(L";  ");
      PrintExpr(stat->Expr->a);
    }
    sDPrintF(L";\n");
    break;
  case STAT_IMPORT:
    if(stat->Expr->a)
      PrintExpr(stat->Expr->a);
    sDPrintF(L"%_import %s:",indent,stat->Expr->Symbol->Name);
    PrintType(stat->Expr->Type,stat->Expr->Count);
    sDPrintF(L";\n");
    break;

  case STAT_ASSIGN:
    sDPrintF(L"%_",indent);
    PrintExpr(stat->Expr);
    sDPrintF(L";\n");
    break;
  case STAT_IF:
    sDPrintF(L"%_if(",indent);
    PrintExpr(stat->Expr);
    sDPrintF(L")\n");
    PrintStat(stat->Child,indent+2);
    if(stat->Alt)
    {
      sDPrintF(L"%_else\n",indent);
      PrintStat(stat->Alt,indent+2);
    }
    break;
  case STAT_WHILE:
    sDPrintF(L"%_while(",indent);
    PrintExpr(stat->Expr);
    sDPrintF(L")\n");
    PrintStat(stat->Child,indent+2);
    break;
  case STAT_DO:
    sDPrintF(L"&_do\n",indent);
    PrintStat(stat->Child,indent+2);
    sDPrintF(L"%_while(",indent);
    PrintExpr(stat->Expr);
    sDPrintF(L");\n");
    break;
  default:
    sDPrintF(L"???");
    break;
  }
}

void ScriptCompiler::PrintFunc(const sChar *name,Expression *a,Expression *b)
{
  if(b==0)
  {
    sDPrintF(L"(%s",name);
    PrintExpr(a);
    sDPrintF(L")");
  }
  else
  {
    sDPrintF(L"(");
    PrintExpr(a);
    sDPrintF(L"%s",name);
    PrintExpr(b);
    sDPrintF(L")");
  }
}


void ScriptCompiler::PrintFunc2(const sChar *name,Expression *e)
{
  sDPrintF(L" %s(",name);
  if(e->a)
    PrintExpr(e->a);
  if(e->b)
  {
    sDPrint(L",");
    PrintExpr(e->b);
  }
  if(e->c)
  {
    sDPrint(L",");
    PrintExpr(e->c);
  }
  if(e->d)
  {
    sDPrint(L",");
    PrintExpr(e->d);
  }
  sDPrintF(L")");
}

void ScriptCompiler::PrintExpr(Expression *expr)
{
  switch(expr->Kind&0x7f)
  {
  case SC_ADD:    PrintFunc(L"+" ,expr->a,expr->b); break;
  case SC_SUB:    PrintFunc(L"-" ,expr->a,expr->b); break;
  case SC_MUL:    PrintFunc(L"*" ,expr->a,expr->b); break;
  case SC_DIV:    PrintFunc(L"/" ,expr->a,expr->b); break;
  case SC_MOD:    PrintFunc(L"%" ,expr->a,expr->b); break;
  case SC_DOT:    PrintFunc(L"\x00ba" ,expr->a,expr->b); break;

  case SC_NEG:    PrintFunc(L"-" ,expr->a); break;
  case SC_FTOI:   PrintFunc(L"ftoi ",expr->a); break;
  case SC_ITOF:   PrintFunc(L"itof ",expr->a); break;
  case SC_FTOC:   PrintFunc(L"ftoc ",expr->a); break;
  case SC_CTOF:   PrintFunc(L"ctof ",expr->a); break;

  case SC_SHIFTL: PrintFunc(L">>",expr->a,expr->b); break;
  case SC_SHIFTR: PrintFunc(L"<<",expr->a,expr->b); break;
  case SC_ROLL:   PrintFunc(L">>>",expr->a,expr->b); break;
  case SC_ROLR:   PrintFunc(L"<<<",expr->a,expr->b); break;

  case SC_NOT:    PrintFunc(L"!" ,expr->a); break;
  case SC_NOTNOT: PrintFunc(L"?" ,expr->a); break;
  case SC_LOGAND: PrintFunc(L"&&",expr->a,expr->b); break;
  case SC_LOGOR:  PrintFunc(L"||",expr->a,expr->b); break;
  case SC_BINAND: PrintFunc(L"&" ,expr->a,expr->b); break;
  case SC_BINOR:  PrintFunc(L"|" ,expr->a,expr->b); break;
  case SC_BINXOR: PrintFunc(L"^" ,expr->a,expr->b); break;

  case SC_EQ:     PrintFunc(L"==",expr->a,expr->b); break;
  case SC_NE:     PrintFunc(L"!=",expr->a,expr->b); break;
  case SC_GT:     PrintFunc(L">" ,expr->a,expr->b); break;
  case SC_GE:     PrintFunc(L">=",expr->a,expr->b); break;
  case SC_LT:     PrintFunc(L"<" ,expr->a,expr->b); break;
  case SC_LE:     PrintFunc(L"<=",expr->a,expr->b); break;

  case SC_ABS:    PrintFunc2(L"abs",expr); break;
  case SC_SIGN:   PrintFunc2(L"sign",expr); break;
  case SC_MAX:    PrintFunc2(L"max",expr); break;
  case SC_MIN:    PrintFunc2(L"min",expr); break;
  case SC_SIN:    PrintFunc2(L"sin",expr); break;
  case SC_COS:    PrintFunc2(L"cos",expr); break;
  case SC_SIN1:   PrintFunc2(L"sin1",expr); break;
  case SC_COS1:   PrintFunc2(L"cos1",expr); break;
  case SC_TAN:    PrintFunc2(L"tan",expr); break;
  case SC_ATAN:   PrintFunc2(L"atan",expr); break;
  case SC_ATAN2:  PrintFunc2(L"atan2",expr); break;
  case SC_SQRT:   PrintFunc2(L"sqrt",expr); break;
  case SC_POW:    PrintFunc2(L"pow",expr); break;
  case SC_EXP:    PrintFunc2(L"exp",expr); break;
  case SC_LOG:    PrintFunc2(L"log",expr); break;
  case SC_SMOOTHSTEP: PrintFunc2(L"smoothstep",expr); break;
  case SC_RAMPUP: PrintFunc2(L"rampup",expr); break;
  case SC_RAMPDOWN: PrintFunc2(L"rampdown",expr); break;
  case SC_TRIANGLE: PrintFunc2(L"triangle",expr); break;
  case SC_PULSE: PrintFunc2(L"pulse",expr); break;
  case SC_EXPEASE: PrintFunc2(L"expease",expr); break;
  case SC_FADEINOUT: PrintFunc2(L"fadeinout",expr); break;
  case SC_NOISE:  PrintFunc2(L"noise",expr); break;
  case SC_PERLINNOISE: PrintFunc2(L"perlinnoise",expr); break;

  case SC_CLAMP:  PrintFunc2(L"clamp",expr); break;
  case SC_LENGTH: PrintFunc2(L"length",expr); break;
  case SC_NORMALIZE:  PrintFunc2(L"normalize",expr); break;
  case SC_MAP:    PrintFunc2(L"map",expr); break;

  case SC_COND:
    sDPrintF(L"(");
    PrintExpr(expr->a);
    sDPrintF(L"?");
    PrintExpr(expr->b);
    sDPrintF(L":");
    PrintExpr(expr->c);
    sDPrintF(L")");
    break;

  case SC_LITERAL:
    if(expr->Count!=1)
      sDPrintF(L"[");
    for(sInt i=0;i<expr->Count;i++)
    {
      if(i!=0)
        sDPrintF(L",");
      switch(expr->Type)
      {
      case ScriptTypeInt:
        sDPrintF(L"%d",expr->LiteralInt[i]);
        break;
      case ScriptTypeFloat:
        sDPrintF(L"%f",expr->LiteralFloat[i]);
        break;
      case ScriptTypeString:
        sDPrintF(L"%q",expr->LiteralString[i]);
        break;
      case ScriptTypeColor:
        sDPrintF(L"0x%08x",expr->LiteralColor[i]);
        break;
      default:
        sDPrintF(L"???");
        break;
      }
    }
    if(expr->Count!=1)
      sDPrintF(L"]");
    break;

  case SC_GETVARR:
  case SC_SETVARR:
    sDPrintF(L"%s",expr->Symbol->Name);
    if(expr->Range0==expr->Range1-1)
      sDPrintF(L"[%d]",expr->Range0);
    else
      sDPrintF(L"[%d..%d]",expr->Range0,expr->Range1-1);
    break;
  case SC_CLEARVARR:
    sDPrintF(L"(clear %s",expr->Symbol->Name);
    if(expr->Range0==expr->Range1-1)
      sDPrintF(L"[%d]",expr->Range0);
    else
      sDPrintF(L"[%d..%d]",expr->Range0,expr->Range1-1);
    sDPrintF(L")");
    break;


  case SC_GETVAR:
  case SC_SETVAR:
    sDPrintF(L"%s",expr->Symbol->Name);
    break;
  case SC_CLEARVAR:
    sDPrintF(L"(clear %s)",expr->Symbol->Name);
    break;

  case SC_MAKELOCAL:
    PrintType(expr->Type,expr->Count);
    sDPrintF(L" %s",expr->Symbol->Name);
    break;
  case SC_MAKEGLOBAL:
    PrintType(expr->Type,expr->Count);
    sDPrintF(L" global %s",expr->Symbol->Name);
    break;
  case SC_IMPORT:
    PrintType(expr->Type,expr->Count);
    sDPrintF(L" import %s",expr->Symbol->Name);
    break;
  case SC_IMPORTDEFAULT:
    PrintType(expr->Type,expr->Count);
    sDPrintF(L" importdefault %s",expr->Symbol->Name);
    break;

  case SC_CAT:
    sDPrintF(L"[");
    PrintExpr(expr->a);
    sDPrintF(L",");
    PrintExpr(expr->b);
    sDPrintF(L"]");
    break;

  case SC_INDEX:
    PrintExpr(expr->a);
    sDPrintF(L"[");
    PrintExpr(expr->b);
    sDPrintF(L"]");
    break;

  case SC_SPLICE:
    PrintExpr(expr->a);
    sDPrintF(L"[%d..%d]",expr->Range0,expr->Range1-1);
    break;

  case SC_DUP:
    sDPrint(L"(dup ");
    PrintExpr(expr->a);
    sDPrintF(L",%d)",expr->Count);
    break;

  case SC_SPLINE:
    sDPrintF(L"(spline %s",expr->Symbol->Name);
    PrintExpr(expr->a);
    sDPrintF(L")");
    break;

  case SC_ASSIGN: PrintFunc(L"=",expr->b,expr->a); break; 

  default:
    sDPrintF(L"???");
  }
}

/****************************************************************************/

static sU32 makecmd(sInt cmd,sInt type,sInt count)
{
  return (cmd&0xff)|((type&0xff)<<8)|((count&0xffff)<<16);
  /*
  if(type==2)
    cmd |= SC_FLOAT;
  if(count==0)
    cmd |= 0x100;
  else
    cmd |= count<<8;

  return cmd;
  */
}

static void writeptr(sU32 *code,sInt &index,void *ptr)
{
  void **dest = (void **) (code+index);
  *dest = ptr;
  index += sizeof(void *)/sizeof(sU32);
}

void ScriptCompiler::OutputExpr(Expression *expr)
{
  if(expr->a) OutputExpr(expr->a);
  if(expr->b) OutputExpr(expr->b);
  if(expr->c) OutputExpr(expr->c);
  if(expr->d) OutputExpr(expr->d);

  switch(expr->Kind)
  {
  case SC_ADD:
  case SC_SUB:
  case SC_MUL:
  case SC_DIV:
  case SC_MOD:
  case SC_NEG:
  case SC_FTOI:
  case SC_ITOF:
  case SC_FTOC:
  case SC_CTOF:
  case SC_NOT:
  case SC_SHIFTL:
  case SC_SHIFTR:
  case SC_ROLL:
  case SC_ROLR:
  case SC_NOTNOT:
  case SC_LOGAND:
  case SC_LOGOR:
  case SC_BINAND:
  case SC_BINOR:
  case SC_BINXOR:
  case SC_EQ:
  case SC_NE:
  case SC_GT:
  case SC_GE:
  case SC_LT:
  case SC_LE:

  case SC_COND:
  case SC_CAT:
  case SC_DUP:

  case SC_ABS:
  case SC_SIGN:
  case SC_MAX:
  case SC_MIN:
  case SC_SIN:
  case SC_COS:
  case SC_SIN1:
  case SC_COS1:
  case SC_TAN:
  case SC_ATAN:
  case SC_ATAN2:
  case SC_SQRT:
  case SC_POW:
  case SC_EXP:
  case SC_LOG:
  case SC_SMOOTHSTEP:
  case SC_RAMPUP:
  case SC_RAMPDOWN:
  case SC_TRIANGLE:
  case SC_PULSE:
  case SC_EXPEASE:
  case SC_FADEINOUT:
  case SC_NOISE:
  case SC_PERLINNOISE:

  case SC_CLAMP:
  case SC_NORMALIZE:
  case SC_MAP:
    Code[Index++] = makecmd(expr->Kind,expr->Type,expr->Count);
    break;

  case SC_LENGTH:
  case SC_DOT:
    Code[Index++] = makecmd(expr->Kind,expr->Type,expr->a->Count);
    break;

  case SC_LITERAL:
    Code[Index++] = makecmd(expr->Kind,expr->Type,expr->Count);
    if(expr->Type==ScriptTypeString)
    {
      for(sInt i=0;i<expr->Count;i++)
        writeptr(Code,Index,(void *)((const sChar *)expr->LiteralString[i]));
    }
    else
    {
      for(sInt i=0;i<expr->Count;i++)
        Code[Index++] = expr->Literal[i];
    }
    break;

  case SC_GETVARR:
  case SC_SETVARR:
    Code[Index++] = makecmd(expr->Kind,expr->Type,expr->Count);
    Code[Index++] = expr->Range0 | (expr->Range1<<16);
    writeptr(Code,Index,expr->Symbol);
    break;

  case SC_GETVAR:
  case SC_SETVAR:
    Code[Index++] = makecmd(expr->Kind,expr->Type,expr->Count);
    writeptr(Code,Index,expr->Symbol);
    break;

  case SC_SPLINE:
    Code[Index++] = makecmd(expr->Kind,expr->Type,expr->Count);
    writeptr(Code,Index,expr->Symbol);
    break;

  case SC_INDEX:
    Code[Index++] = makecmd(expr->Kind,expr->a->Type,expr->a->Count);
    break;

  case SC_SPLICE:
    Code[Index++] = makecmd(expr->Kind,expr->Type,expr->a->Count);
    Code[Index++] = expr->Range0 | (expr->Range1<<16);
    break;  

  case SC_PRINT:
    Code[Index++] = makecmd(expr->Kind,expr->b->Type,expr->b->Count);
    break;

  case SC_ASSIGN:
    break;

  default:
    sFatal(L"unknown opcode");
  }
}

void ScriptCompiler::OutputStat(Statement *stat)
{
  switch(stat->Kind)
  {
  case STAT_BLOCK:
    stat = stat->Child;
    while(stat)
    {
      OutputStat(stat);
      stat = stat->Next;
    }
    break;

  case STAT_LOCAL:
    Code[Index++] = makecmd(SC_MAKELOCAL,stat->Expr->Type,stat->Expr->Count);
    writeptr(Code,Index,stat->Expr->Symbol);
    if(stat->Expr->a)
      OutputExpr(stat->Expr->a);
    break;

  case STAT_GLOBAL:
    Code[Index++] = makecmd(SC_MAKEGLOBAL,stat->Expr->Type,stat->Expr->Count);
    writeptr(Code,Index,stat->Expr->Symbol);
    if(stat->Expr->a)
      OutputExpr(stat->Expr->a);
    break;

  case STAT_IMPORT:
    if(stat->Expr->a)
      OutputExpr(stat->Expr->a);
    Code[Index++] = makecmd(stat->Expr->Kind,stat->Expr->Type,stat->Expr->Count);
    writeptr(Code,Index,(void *)(const sChar *)stat->Expr->Symbol->Name);
    break;

  case STAT_ASSIGN:
    OutputExpr(stat->Expr);
    break;

  case STAT_IF:
    OutputExpr(stat->Expr);

    if(stat->Alt)
    {
      sInt p0 = Index;
      Code[Index++] = makecmd(SC_BF,0,1);
      OutputStat(stat->Child);
      sInt p1 = Index;
      Code[Index++] = makecmd(SC_B,0,1);
      PatchCmd(p0,Index);
      OutputStat(stat->Alt);
      PatchCmd(p1,Index);
    }
    else
    {
      sInt p0 = Index;
      Code[Index++] = makecmd(SC_BF,0,1);
      OutputStat(stat->Child);
      PatchCmd(p0,Index);
    }
    break;

  case STAT_DO:
    {
      sInt p0 = Index;
      OutputStat(stat->Child);
      OutputExpr(stat->Expr);
      Code[Index] = makecmd(SC_BT,0,1);
      PatchCmd(Index++,p0);
    }
    break;

  case STAT_WHILE:
    {
      sInt p1 = Index;
      Code[Index++] = makecmd(SC_B,0,1);
      sInt p0 = Index;
      OutputStat(stat->Child);
      PatchCmd(p1,Index);
      OutputExpr(stat->Expr);
      Code[Index] = makecmd(SC_BT,0,1);
      PatchCmd(Index++,p0);
    }
    break;

  default:
    sDPrintF(L"???");
    break;
  }
}

void ScriptCompiler::PatchCmd(sInt cmdindex,sInt target)
{
  sInt dist = target - (cmdindex+1);
  if(dist<-0x8000 || dist > 0x7fff)
    Scan.Error(L"branch out of range");
  Code[cmdindex] = (Code[cmdindex] & 0x0000ffff) | (dist<<16);
}

/****************************************************************************/

void ScriptCompiler::ConstFold(Expression *expr)
{
  if(expr->a) ConstFold(expr->a);
  if(expr->b) ConstFold(expr->b);
  if(expr->c) ConstFold(expr->c);
  if(expr->d) ConstFold(expr->d);

  if(expr->a && expr->a->Kind!=SC_LITERAL) return;
  if(expr->b && expr->b->Kind!=SC_LITERAL) return;
  if(expr->c && expr->c->Kind!=SC_LITERAL) return;
  if(expr->d && expr->d->Kind!=SC_LITERAL) return;

  if(expr->Kind==SC_GETVARR && expr->Range0==0 && expr->Range1==expr->Count)
    expr->Kind=SC_GETVAR;
  if(expr->Kind==SC_SETVARR && expr->Range0==0 && expr->Range1==expr->Count)
    expr->Kind=SC_SETVAR;
  if(expr->Kind==SC_CLEARVARR && expr->Range0==0 && expr->Range1==expr->Count)
    expr->Kind=SC_CLEARVAR;


  if(expr->Kind==SC_CAT)
  {
    sInt type = expr->Type;
    sInt ca = expr->a->Count;
    sInt cb = expr->b->Count;
    if(type==ScriptTypeString)
    {
      sPoolString *val = Pool->Alloc<sPoolString>(ca+cb);
      for(sInt i=0;i<ca;i++)
        val[i] = expr->a->LiteralString[i];
      for(sInt i=0;i<cb;i++)
        val[ca+i] = expr->b->LiteralString[i];

      sClear(*expr);
      expr->LiteralString = val;
    }
    else
    {
      sU32 *val = Pool->Alloc<sU32>(ca+cb);
      for(sInt i=0;i<ca;i++)
        val[i] = expr->a->Literal[i];
      for(sInt i=0;i<cb;i++)
        val[ca+i] = expr->b->Literal[i];

      sClear(*expr);
      expr->Literal = val;
    }
    expr->Kind = SC_LITERAL;
    expr->Type = type;
    expr->Count = ca+cb;

    return;
  }

  if(expr->Kind==SC_DUP)
  {
    sInt type = expr->Type;
    sInt max = expr->Count;
    sVERIFY(expr->a->Count==1)

    sU32 *val = Pool->Alloc<sU32>(max);
    for(sInt i=0;i<max;i++)
      val[i] = expr->a->Literal[0];

    sClear(*expr);
    expr->Kind = SC_LITERAL;
    expr->Type = type;
    expr->Count = max;
    expr->Literal = val;
    return;
  }

  if(expr->Count>0)
  {
    sInt max = expr->Count;
    sInt type = expr->Type;
    if(type==ScriptTypeInt)  // int
    {
      sInt a = 0;
      sInt b = 0;
      sS32 *val = (sS32 *) Pool->Alloc<sU32>(max);

      for(sInt i=0;i<max;i++)
      {
        if(expr->a) a = ((sInt *)expr->a->Literal)[i];
        if(expr->b) b = ((sInt *)expr->b->Literal)[i];

        switch(expr->Kind)
        {
        case SC_ADD: val[i] = a+b; break;
        case SC_SUB: val[i] = a-b; break;
        case SC_MUL: val[i] = a*b; break;
        case SC_DIV: if(b==0) return; val[i] = a/b; break;
        case SC_MOD: if(b==0) return; val[i] = a%b; break;

        case SC_SHIFTL: val[i] = a<<b; break;
        case SC_SHIFTR: val[i] = a>>b; break;
        case SC_ROLL:   val[i] = (a<<b)|(a>>(32-b)); break;
        case SC_ROLR:   val[i] = (a>>b)|(a<<(32-b)); break;

        case SC_LOGAND: val[i] = a && b; break;
        case SC_LOGOR:  val[i] = a || b; break;
        case SC_BINAND: val[i] = a & b; break;
        case SC_BINOR:  val[i] = a | b; break;
        case SC_BINXOR: val[i] = a ^ b; break;

        case SC_EQ: val[i] = (a==b); break;
        case SC_NE: val[i] = (a!=b); break;
        case SC_GT: val[i] = (a> b); break;
        case SC_GE: val[i] = (a>=b); break;
        case SC_LT: val[i] = (a< b); break;
        case SC_LE: val[i] = (a<=b); break;
        
        case SC_NEG:  val[i] = -a; break;
        case SC_FTOI: val[i] = sInt(((sF32 *)expr->a->Literal)[i]); break;
        case SC_NOT:  val[i] = !a; break;
        case SC_NOTNOT: val[i] = !!a; break;

        default: return;
        }
      }

      sClear(*expr);
      expr->Kind = SC_LITERAL;
      expr->Type = 1;
      expr->Count = max;
      expr->Literal = (sU32 *) val;
    }
    else if(type==ScriptTypeFloat)    // float
    {
      sF32 a = 0;
      sF32 b = 0;
      sF32 *val = (sF32 *) Pool->Alloc<sU32>(max);

      for(sInt i=0;i<max;i++)
      {
        if(expr->a) a = ((sF32 *)expr->a->Literal)[i];
        if(expr->b) b = ((sF32 *)expr->b->Literal)[i];

        switch(expr->Kind)
        {
        case SC_ADD: val[i] = a+b; break;
        case SC_SUB: val[i] = a-b; break;
        case SC_MUL: val[i] = a*b; break;
        case SC_DIV: val[i] = a/b; break;
        case SC_MOD: val[i] = sFMod(a,b); break;

        case SC_EQ: val[i] = (a==b); type=1; break;
        case SC_NE: val[i] = (a!=b); type=1; break;
        case SC_GT: val[i] = (a> b); type=1; break;
        case SC_GE: val[i] = (a>=b); type=1; break;
        case SC_LT: val[i] = (a< b); type=1; break;
        case SC_LE: val[i] = (a<=b); type=1; break;

        case SC_ITOF: val[i] = sF32(((sS32 *)expr->a->Literal)[i]); break;

        default: return;
        }
      }

      sClear(*expr);
      expr->Kind = SC_LITERAL;
      expr->Type = type;
      expr->Count = max;
      expr->Literal = (sU32 *) val;
    }
  }
}

void ScriptCompiler::ConstFold(Statement *stat)
{
  switch(stat->Kind)
  {
  case STAT_BLOCK:
    stat = stat->Child;
    while(stat)
    {
      ConstFold(stat);
      stat = stat->Next;
    }
    break;

  case STAT_LOCAL:
  case STAT_GLOBAL:
    if(stat->Expr->a)
      ConstFold(stat->Expr->a);
    break;

  case STAT_ASSIGN:
    ConstFold(stat->Expr);
    break;

  case STAT_IF:
    ConstFold(stat->Expr);
    if(stat->Alt)
      ConstFold(stat->Alt);
    if(stat->Child)
      ConstFold(stat->Child);

    if(stat->Expr->Kind==SC_LITERAL)
    {
      stat->Kind=STAT_BLOCK;
      if(!*(sInt *)stat->Expr->Literal)
        stat->Child = stat->Alt;
      stat->Alt = 0;
    }
    break;

  case STAT_DO:
  case STAT_WHILE:
    ConstFold(stat->Child);
    ConstFold(stat->Expr);
    break;
  }
}

/****************************************************************************/

ScriptCompiler::ScriptCompiler()
{
  ctx = 0;
  Pool = new sMemoryPool;
}

ScriptCompiler::~ScriptCompiler()
{
  delete Pool;
}

void ScriptCompiler::InitScanner()
{
  Scan.Init();

  Scan.Flags = sSF_CPPCOMMENT|sSF_NUMBERCOMMENT|sSF_ESCAPECODES|sSF_MERGESTRINGS|sSF_CPP;
//  Scan.Flags |= sSF_QUIET;

  Scan.AddToken(L">>>",TOK_ROLR);
  Scan.AddToken(L"<<<",TOK_ROLL);
  Scan.AddToken(L"++",TOK_INC);
  Scan.AddToken(L"--",TOK_DEC);
  Scan.AddToken(L"..",sTOK_ELLIPSES);
  Scan.AddToken(L"&&",TOK_LOGAND);
  Scan.AddToken(L"||",TOK_LOGOR);
  Scan.AddToken(L"~~",TOK_TILDETILDE);

  Scan.AddToken(L">>",TOK_SHIFTR);
  Scan.AddToken(L"<<",TOK_SHIFTL);
  Scan.AddToken(L"==",sTOK_EQ);
  Scan.AddToken(L"!=",sTOK_NE);
  Scan.AddToken(L"<=",sTOK_LE);
  Scan.AddToken(L">=",sTOK_GE);

  Scan.AddToken(L"int",TOK_INT);
  Scan.AddToken(L"float",TOK_FLOAT);
  Scan.AddToken(L"string",TOK_STRING);
  Scan.AddToken(L"color",TOK_COLOR);
  Scan.AddToken(L"enum",TOK_ENUM);
  Scan.AddToken(L"struct",TOK_STRUCT);
  Scan.AddToken(L"union",TOK_UNION);
  Scan.AddToken(L"class",TOK_CLASS);
  Scan.AddToken(L"type",TOK_TYPE);
  Scan.AddToken(L"func",TOK_FUNC);
  Scan.AddToken(L"attribute",TOK_ATTRIBUTE);
  Scan.AddToken(L"cast",TOK_CAST);

  Scan.AddToken(L"if",TOK_IF);
  Scan.AddToken(L"else",TOK_ELSE);
  Scan.AddToken(L"switch",TOK_SWITCH);
  Scan.AddToken(L"case",TOK_CASE);
  Scan.AddToken(L"while",TOK_WHILE);
  Scan.AddToken(L"do",TOK_DO);
  Scan.AddToken(L"break",TOK_BREAK);
  Scan.AddToken(L"continue",TOK_CONTINUE);
  Scan.AddToken(L"global",TOK_GLOBAL);
  Scan.AddToken(L"import",TOK_IMPORT);

  Scan.AddToken(L"pi2",TOK_PI2);
  Scan.AddToken(L"pi",TOK_PI);

  Scan.AddTokens(L"+-*/%^~\x00ba\x00b0,.;:!?=<>()[]{}&|");
}

sU32 *ScriptCompiler::Parse(ScriptContext *ctx_,const sChar *src,sInt line)
{
  // Init

  InitScanner();

  Scan.Start(src);
  if(line)
    Scan.SetLine(line);
  Code = new sU32[0x10000];
  Index = 0;

  // do

  ctx = ctx_;
  ctx->UpdateCType();
  Statement *stat = _Global();
  ctx = 0;
  
  // error handling

  ErrorMsg = Scan.ErrorMsg;
  if(Scan.Errors>0)
  {
    delete[] Code;
    return 0;
  }

  // optimize

  ConstFold(stat);

  // write code

//  if(!sRELEASE) PrintStat(stat,0);
  OutputStat(stat);
  Code[Index++] = makecmd(SC_STOP,0,1);

  sU32 *code = new sU32[Index];
  sCopyMem(code,Code,Index*4);


  // exit

  delete[] Code;

  return code;
}

/****************************************************************************/
