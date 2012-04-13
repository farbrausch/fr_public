// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "script.hpp"

#if !sLINK_KKRIEGER || !sPLAYER

/****************************************************************************/
/****************************************************************************/

void sScript::Init()
{
  Var = 0;
  VarCount = 0;
  Code = 0;
  CodeCount = 0;
  ErrorCount = 0;
}

void sScript::Exit()
{
  sDelete(Code);
  sDelete(Var);
}

/****************************************************************************/


sScriptVM::sScriptVM()
{
  Index = 0;
  ip = 0;
  OutAlloc = 0x10000;
  OutBuffer = new sChar[OutAlloc];
  OutBuffer[0] = 0;
  OutUsed = 0;
}

sScriptVM::~sScriptVM()
{
  sDeleteArray(OutBuffer);
}

sBool sScriptVM::Extern(sInt id,sInt offset,sInt type,sF32 *p,sBool store)
{
  return sTRUE;
}

sBool sScriptVM::UseObject(sInt id,void *object)
{
  return sTRUE;
}

/****************************************************************************/

sInt sScriptVM::GetOffset()
{
  sInt v,i;

  v = 0;
  do
  {
    i = *ip++;
    v = (v<<7) + (i&127);
  }
  while(i&128);

  return v;
}

void sScriptVM::Print(const sChar *buffer)
{
   sInt len=sGetStringLen(buffer);
   sVERIFY(OutUsed+len+1 < OutAlloc);
   sCopyMem(OutBuffer+OutUsed,buffer,len);
   OutUsed+=len;
   OutBuffer[OutUsed] = 0;
}

/****************************************************************************/

sBool sScriptVM::Execute(sScript *Script,sInt startip)
{
  static const sInt typesizes[4] = { 1,4,16,1 };
  sInt opcode;
  sInt offset;
  sInt arg;
  sInt size;
  sInt frame;
  sInt i;
  sF32 a;
  sVector v0,v1,v;
  sMatrix m0,m1;
  sF32 val[16];
  sChar buffer[512];

  arg = -1;
  frame = 0;
  Index = 0;
  OutUsed = 0;
  OutBuffer[0] = 0;
  ip = Script->Code+startip;
  for(;;)
  {
    opcode = GetByte();
    sVERIFY(Index>=0 && Index<1000);

    switch(opcode)
    {
    case KB_COMBV:
      // for typesafety only
      break;
    case KB_COMBM:
      // for typesafety only
      break;


    case KB_ADDS:
      Index--;
      Stack[Index-1] = Stack[Index-1]+Stack[Index];
      break;
    case KB_SUBS:
      Index--;
      Stack[Index-1] = Stack[Index-1]-Stack[Index];
      break;
    case KB_MULS:
      Index--;
      Stack[Index-1] = Stack[Index-1]*Stack[Index];
      break;
    case KB_DIVS:
      Index--;
      Stack[Index-1] = Stack[Index-1]/Stack[Index];
      break;
    case KB_MODS:
      Index--;
      Stack[Index-1] = sFMod(Stack[Index-1],Stack[Index]);
      break;
    case KB_ADDV:
      PopV(v1);
      PopV(v0);
      v0.Add4(v1);
      PushV(v0);
      break;
    case KB_SUBV:
      PopV(v1);
      PopV(v0);
      v0.Sub4(v1);
      PushV(v0);
      break;
    case KB_MULV:
      PopV(v1);
      PopV(v0);
      v0.Mul4(v1);
      PushV(v0);
      break;
    case KB_CROSS:
      PopV(v1);
      PopV(v0);
      v.Cross3(v0,v1);
      PushV(v0);
      break;
    case KB_MULM:
      PopM(m1);
      PopM(m0);
      m0.Mul4(m1);
      PushM(m0);
      break;

    case KB_NEG:
      Stack[Index-1] = -Stack[Index-1];
      break;
    case KB_SIN:
      Stack[Index-1] = sFSin(Stack[Index-1]*sPI2F);
      break;
    case KB_COS:
      Stack[Index-1] = sFCos(Stack[Index-1]*sPI2F);
      break;
    case KB_RAMP:
      Stack[Index-1] = sFMod(Stack[Index-1],1);
      break;
    case KB_DP3:
      PopV(v1);
      PopV(v0);
      PushS(v0.Dot3(v1));
      break;
    case KB_DP4:
      PopV(v1);
      PopV(v0);
      PushS(v0.Dot4(v1));
      break;

    case KB_EULER:
      PopV(v0);
      m0.InitEulerPI2(&v0.x);
      PushM(m0);
      break;
    case KB_LOOKAT:
      PopV(v0);
      m0.InitDir(v0);
      PushM(m0);
      break;
    case KB_AXIS:
      PopV(v0);
      m0.InitRot(v0);
      PushM(m0);
      break;
    case KB_QUART:
      PopV(v0);
      m0.Init();
      PushM(m0);
      break;
    case KB_SCALE:
      PopV(v0);
      m0.Init();
      m0.i.x = v0.x;
      m0.j.x = v0.y;
      m0.k.x = v0.z;
      //m0.l.x = v0.w;
      PushM(m0);
      break;

    case KB_EQ:
      Stack[Index-1] = Stack[Index-1]==0;
      break;
    case KB_NE:
      Stack[Index-1] = Stack[Index-1]!=0;
      break;
    case KB_GT:
      Stack[Index-1] = Stack[Index-1]>0;
      break;
    case KB_GE:
      Stack[Index-1] = Stack[Index-1]>=0;
      break;

    case KB_JUMP:
      offset = GetOffset();
      ip = Script->Code+offset;
      break;
    case KB_JUMPT:
      offset = GetOffset();
      a = PopS();
      if(a!=0)
        ip = Script->Code+offset;
      break;
    case KB_JUMPF:
      offset = GetOffset();
      a = PopS();
      if(a==0)
        ip = Script->Code+offset;
      break;

    case KB_CALL:
      offset = GetOffset();
      arg = Index-offset;
      ((sU32 *)Stack)[Index++] = arg;
      ((sU32 *)Stack)[Index++] = ip-Script->Code;
      frame = Index;
      break;

    case KB_OBJECT:
      offset = GetOffset();
      UseObject(offset,PopO());
      break;

    case KB_RET:
      size = 0;
      goto retcode;

    case KB_PRINTS:
      sSPrintF(buffer,sizeof(buffer),"%f ",PopS());
      Print(buffer);
      break;
    case KB_PRINTV:
      PopV(v0);
      sSPrintF(buffer,sizeof(buffer),"[%f,%f,%f,%f]\n",v0.x,v0.y,v0.z,v0.w);
      Print(buffer);
      break;
    case KB_PRINTM:
      PopM(m0);
      sSPrintF(buffer,sizeof(buffer),"[%f,%f,%f,%f]\n",m0.i.x,m0.i.y,m0.i.z,m0.i.w);
      sSPrintF(buffer,sizeof(buffer),"[%f,%f,%f,%f]\n",m0.j.x,m0.j.y,m0.j.z,m0.j.w);
      sSPrintF(buffer,sizeof(buffer),"[%f,%f,%f,%f]\n",m0.k.x,m0.k.y,m0.k.z,m0.k.w);
      sSPrintF(buffer,sizeof(buffer),"[%f,%f,%f,%f]\n",m0.l.x,m0.l.y,m0.l.z,m0.l.w);
      Print(buffer);
      break;
    case KB_PRINTO:
      sSPrintF(buffer,sizeof(buffer),"%f",PopO());
      break;
    case KB_PRINT:
      Print((sChar *)ip);
      while(*ip++);
      break;

    default:
      size = typesizes[opcode&3];

      switch(opcode/4)
      {
      case KB_LOADS/4:
        offset = GetOffset();
        sVERIFY(offset>=0 && offset+size<=Script->VarCount);
        Push(Script->Var+offset,size);
        break;
      case KB_STORES/4:
        offset = GetOffset();
        sVERIFY(offset>=0 && offset+size<=Script->VarCount);
        Pop(Script->Var+offset,size);
        break;
      case KB_ELOADS/4:
        i = GetOffset();
        offset = GetOffset();
        Extern(i,offset,opcode&3,Stack+Index,0);
        Index+=size;
        break;
      case KB_ESTORES/4:
        Index-=size;
        i = GetOffset();
        offset = GetOffset();
        Extern(i,offset,opcode&3,Stack+Index,1);
        break;
      case KB_ALOADS/4:
        sVERIFY(arg!=-1);
        offset = arg+GetOffset();
        sVERIFY(offset>=0 && offset+size<Index);
        Push(Stack+offset,size);
        break;
      case KB_CONDS/4:
        a = PopS();
        Index-=size;
        if(a==0)
          sCopyMem(Stack,Stack+size,size*4);
        break;
      case KB_RET/4:
      retcode:
        Pop(val,size);
        if(arg==-1)
        {
          sVERIFY(Index==0);
          return 1;
        }
        sVERIFY(frame==Index);
        ip = Script->Code+(((sU32 *)Stack)[--Index]);
        Index = ((sU32 *)Stack)[--Index];
        Push(val,size);
        break;

      case KB_SELECTX/4:
        Pop(val,4);
        PushS(val[opcode&3]);
        break;
      case KB_SELECTI/4:
        Pop(val,16);
        Push(val+(opcode&3)*4,4);
        break;
      case KB_LITS/4:
        Push((sF32 *)ip,size);
        ip+=size*4;
        break;

      default:
        sVERIFYFALSE;
        break;
      }
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Compiler                                                           ***/
/***                                                                      ***/
/****************************************************************************/

enum tokens
{
  TOK_EOF=0,
  TOK_ERROR,

  TOK_NAME,
  TOK_FLOAT,
  TOK_STRING,

  TOK_OPEN,
  TOK_CLOSE,
  TOK_SOPEN,
  TOK_SCLOSE,
  TOK_BOPEN,
  TOK_BCLOSE,

  TOK_EQ,
  TOK_NE,
  TOK_GE,
  TOK_GT,
  TOK_LE,
  TOK_LT,
  TOK_NOT,

  TOK_COLON,
  TOK_SEMI,
  TOK_DOT,
  TOK_COMMA,
  TOK_QUESTION,
  TOK_ASSIGN,

  TOK_PLUS,
  TOK_MINUS,
  TOK_MUL,
  TOK_DIV,
  TOK_MOD,

  TOK_SCALAR,
  TOK_VECTOR,
  TOK_MATRIX,
  TOK_OBJECT,
  TOK_EXTERN,

  TOK_IF,
  TOK_ELSE,
  TOK_WHILE,
  TOK_DO,
  TOK_FOR,
};

/****************************************************************************/

struct tokennames_ { sInt token; const sChar *name; };
const tokennames_ tokennames[] = 
{
  { TOK_SCALAR,"scalar" },
  { TOK_VECTOR,"vector" },
  { TOK_MATRIX,"matrix" },
  { TOK_OBJECT,"object" },
  { TOK_EXTERN,"extern" },

  { TOK_IF    ,"if"     },
  { TOK_ELSE  ,"else"   },
  { TOK_WHILE ,"while"  },
  { TOK_DO    ,"do"     },
  { TOK_FOR   ,"for"    },

  { 0 }
};

/****************************************************************************/
/****************************************************************************/

void sScriptCompiler::PrintToken()
{
  static const sChar *toks[] = 
  {
    "EOF",
    "ERROR",
    0,
    0,
    0,

    "(",
    ")",
    "[",
    "]",
    "{",
    "}",

    "==",
    "!=",
    ">=",
    ">",
    "<=",
    "<",
    "!",

    ":",
    ";",
    ".",
    ",",
    "?",
    "=",

    "+",
    "-",
    "*",
    "/",
    "%",

    "scalar",
    "vector",
    "matrix",
    "object",
    "extern",

    "if",
    "else",
    "while",
    "for",
  };

  switch(Token)
  {
  case TOK_NAME:
    sDPrintF("<%s> ",Name);
    break;
  case TOK_FLOAT:
    sDPrintF("%f ",Value);
    break;
  case TOK_STRING:
    sDPrintF("\"%s\" ",String);
    break;
  default:
    if(Token>sizeof(toks)/4 || Token<0 || toks[Token]==0)
      sDPrintF("??? ");
    else
      sDPrintF("%s ",toks[Token]);
    break;
  }
}

/****************************************************************************/

struct OpCode
{
  sChar *Name;
  sU32 Flags;
  sInt Out;
  sInt In[4];
};

enum OpCodeFlags 
{
  OP_OFFSET  =0x0001,
  OP_EXTERN  =0x0002,
  OP_DATA1   =0x0004,
  OP_DATA4   =0x0008,
  OP_JUMP    =0x0010,
  OP_ARGS    =0x0020,
  OP_RET     =0x0040,
  OP_ERROR   =0x0080,
  OP_TEXT    =0x0100,
};

static const OpCode OpCodes[] =
{
  { "LOADS"  ,OP_OFFSET          ,sST_SCALAR },
  { "LOADV"  ,OP_OFFSET          ,sST_VECTOR },
  { "LOADM"  ,OP_OFFSET          ,sST_MATRIX },
  { "LOADO"  ,OP_OFFSET          ,sST_OBJECT },
  { "ELOADS" ,OP_OFFSET|OP_EXTERN,sST_SCALAR },
  { "ELOADV" ,OP_OFFSET|OP_EXTERN,sST_VECTOR },
  { "ELOADM" ,OP_OFFSET|OP_EXTERN,sST_MATRIX },
  { "ELOADO" ,OP_OFFSET|OP_EXTERN,sST_OBJECT },
  { "STORES" ,OP_OFFSET          ,0           ,sST_SCALAR },
  { "STOREV" ,OP_OFFSET          ,0           ,sST_VECTOR },
  { "STOREM" ,OP_OFFSET          ,0           ,sST_MATRIX },
  { "STROEO" ,OP_OFFSET          ,0           ,sST_OBJECT },
  { "ESTORES",OP_OFFSET|OP_EXTERN,0           ,sST_SCALAR },
  { "ESTOREV",OP_OFFSET|OP_EXTERN,0           ,sST_VECTOR },
  { "ESTOREM",OP_OFFSET|OP_EXTERN,0           ,sST_MATRIX },
  { "ESTROEO",OP_OFFSET|OP_EXTERN,0           ,sST_OBJECT },
  { "ARGS"   ,0                  ,sST_SCALAR },
  { "ARGV"   ,0                  ,sST_VECTOR },
  { "ARGM"   ,0                  ,sST_MATRIX },
  { "ARGO"   ,0                  ,sST_OBJECT },
  { "DUPS"   ,0                  ,sST_SCALAR  ,sST_SCALAR },
  { "DUPV"   ,0                  ,sST_VECTOR  ,sST_VECTOR },
  { "DUPM"   ,0                  ,sST_MATRIX  ,sST_MATRIX },
  { "DUPO"   ,0                  ,sST_OBJECT  ,sST_OBJECT },
  { "DROPS"  ,0                  ,0           ,sST_SCALAR },
  { "DROPV"  ,0                  ,0           ,sST_VECTOR },
  { "DROPM"  ,0                  ,0           ,sST_MATRIX },
  { "DROPO"  ,0                  ,0           ,sST_OBJECT },
  { "CONDS"  ,0                  ,sST_SCALAR  ,sST_SCALAR,sST_SCALAR,sST_SCALAR },
  { "CONDV"  ,0                  ,sST_VECTOR  ,sST_SCALAR,sST_VECTOR,sST_VECTOR },
  { "CONDM"  ,0                  ,sST_MATRIX  ,sST_SCALAR,sST_MATRIX,sST_MATRIX },
  { "CONDO"  ,0                  ,sST_OBJECT  ,sST_SCALAR,sST_OBJECT,sST_OBJECT },
  { "RETS"   ,OP_RET             ,0           ,sST_SCALAR },
  { "RETV"   ,OP_RET             ,0           ,sST_VECTOR },
  { "RETM"   ,OP_RET             ,0           ,sST_MATRIX },
  { "RETO"   ,OP_RET             ,0           ,sST_OBJECT },
  { "SELECTX",0                  ,sST_SCALAR  ,sST_VECTOR },
  { "SELECTY",0                  ,sST_SCALAR  ,sST_VECTOR },
  { "SELECTZ",0                  ,sST_SCALAR  ,sST_VECTOR },
  { "SELECTW",0                  ,sST_SCALAR  ,sST_VECTOR },
  { "SELECTI",0                  ,sST_VECTOR  ,sST_MATRIX },
  { "SELECTJ",0                  ,sST_VECTOR  ,sST_MATRIX },
  { "SELECTK",0                  ,sST_VECTOR  ,sST_MATRIX },
  { "SELECTL",0                  ,sST_VECTOR  ,sST_MATRIX },
  { "LITS"   ,OP_DATA1           ,sST_SCALAR },
  { "LITV"   ,OP_DATA4           ,sST_VECTOR },
  { "???"    ,OP_ERROR           ,0 },
  { "???"    ,OP_ERROR           ,0 },

  { "COMBV"  ,0                  ,sST_VECTOR  ,sST_SCALAR,sST_SCALAR,sST_SCALAR,sST_SCALAR },
  { "COMBM"  ,0                  ,sST_MATRIX  ,sST_VECTOR,sST_VECTOR,sST_VECTOR,sST_VECTOR },

  { "ADDS"   ,0                  ,sST_SCALAR  ,sST_SCALAR,sST_SCALAR },
  { "SUBS"   ,0                  ,sST_SCALAR  ,sST_SCALAR,sST_SCALAR },
  { "MULS"   ,0                  ,sST_SCALAR  ,sST_SCALAR,sST_SCALAR },
  { "DIVS"   ,0                  ,sST_SCALAR  ,sST_SCALAR,sST_SCALAR },
  { "MODS"   ,0                  ,sST_SCALAR  ,sST_SCALAR,sST_SCALAR },
  { "ADDV"   ,0                  ,sST_VECTOR  ,sST_VECTOR,sST_VECTOR },
  { "SUBV"   ,0                  ,sST_VECTOR  ,sST_VECTOR,sST_VECTOR },
  { "MULV"   ,0                  ,sST_VECTOR  ,sST_VECTOR,sST_VECTOR },
  { "CROSSV" ,0                  ,sST_VECTOR  ,sST_VECTOR,sST_VECTOR },
  { "MULM"   ,0                  ,sST_MATRIX  ,sST_MATRIX,sST_MATRIX },
  { "NEG"    ,0                  ,sST_SCALAR  ,sST_SCALAR },
  { "SIN"    ,0                  ,sST_SCALAR  ,sST_SCALAR },
  { "COS"    ,0                  ,sST_SCALAR  ,sST_SCALAR },
  { "RAMP"   ,0                  ,sST_SCALAR  ,sST_SCALAR },
  { "DP3"    ,0                  ,sST_SCALAR  ,sST_VECTOR,sST_VECTOR },
  { "DP4"    ,0                  ,sST_SCALAR  ,sST_VECTOR,sST_VECTOR },
  { "EULER"  ,0                  ,sST_MATRIX  ,sST_VECTOR },
  { "LOOKAT" ,0                  ,sST_MATRIX  ,sST_VECTOR },
  { "AXIS"   ,0                  ,sST_MATRIX  ,sST_VECTOR },
  { "QUART"  ,0                  ,sST_MATRIX  ,sST_VECTOR },
  { "SCALE"  ,0                  ,sST_MATRIX  ,sST_VECTOR },

  { "EQ"     ,0                  ,sST_SCALAR  ,sST_SCALAR },
  { "NE"     ,0                  ,sST_SCALAR  ,sST_SCALAR },
  { "GT"     ,0                  ,sST_SCALAR  ,sST_SCALAR },
  { "GE"     ,0                  ,sST_SCALAR  ,sST_SCALAR },
  { "JUMP"   ,OP_JUMP            ,0 },
  { "JUMPT"  ,OP_JUMP            ,0           ,sST_SCALAR },
  { "JUMPF"  ,OP_JUMP            ,0           ,sST_SCALAR },
  { "CALL"   ,OP_JUMP|OP_ARGS    ,0 },
  { "OBJECT" ,OP_EXTERN          ,sST_VECTOR  ,sST_SCALAR,sST_OBJECT },   // there is only one object-use till now. 
  { "RET"    ,OP_RET             ,0 },

  { "PRINT"  ,OP_TEXT            ,0 },
  { "PRINTS" ,0                  ,0,sST_SCALAR },
  { "PRINTV" ,0                  ,0,sST_VECTOR },
  { "PRINTM" ,0                  ,0,sST_MATRIX },
  { "PRINTO" ,0                  ,0,sST_OBJECT },

  { "???"    ,OP_ERROR }
};

struct Intrinsic 
{
  sChar *Name;
  sInt Flags;
  sInt Out;
  sInt In[2];
  sInt Code;
  sInt Extern;
};

const static Intrinsic Intrinsics[] = 
{
  { "dp3"   ,0          ,sST_SCALAR,{sST_VECTOR,sST_VECTOR} ,KB_DP3          },
  { "dp4"   ,0          ,sST_SCALAR,{sST_VECTOR,sST_VECTOR} ,KB_DP4          },
  { "sin"   ,0          ,sST_SCALAR,{sST_SCALAR}            ,KB_SIN          },
  { "cos"   ,0          ,sST_SCALAR,{sST_SCALAR}            ,KB_COS          },
  { "ramp"  ,0          ,sST_SCALAR,{sST_SCALAR}            ,KB_RAMP         },
  { "euler" ,0          ,sST_MATRIX,{sST_VECTOR}            ,KB_EULER        },
  { "lookat",0          ,sST_MATRIX,{sST_VECTOR}            ,KB_LOOKAT       },
  { "axis"  ,0          ,sST_MATRIX,{sST_VECTOR}            ,KB_AXIS         },
  { "quart" ,0          ,sST_MATRIX,{sST_VECTOR}            ,KB_QUART        },
  { "scale" ,0          ,sST_MATRIX,{sST_VECTOR}            ,KB_SCALE        },
  { "eval"  ,0          ,sST_VECTOR,{sST_SCALAR,sST_OBJECT} ,KB_OBJECT ,0    },
/*
  { "unit"  ,0          ,sST_SCALAR,{0}                     ,KB_ELOADM,0,0   },
  { "origin",0          ,sST_SCALAR,{0}                     ,KB_ELOADV,0,16  },
  { "null"  ,0          ,sST_SCALAR,{0}                     ,KB_ELOADV,0,20  },
  { "beat"  ,0          ,sST_SCALAR,{0}                     ,KB_ELOADS,0,24  },
  { "time"  ,0          ,sST_SCALAR,{0}                     ,KB_ELOADS,0,25  },
*/
  { 0 }
};

sU32 getoffset(sU8 *&code)
{
  sU32 val;
  sInt i;

  val = 0;
  do
  {
    i = *code++;
    val = val*0x80 + (i&0x7f); 
  }
  while(i&0x80);

  return val;
}

sBool sScriptCompiler::Disassemble(sScript *script,sText *out,sBool verbose)
{
  sU8 *codeend;
  sU8 *code;
  sU8 *codestart;
  const OpCode *op;
  sInt byte;
  sInt stack[128];
  sInt index;
  sInt error;
  sF32 *fp;
  sInt i,j;
  sU8 *listcode;
  sInt listchar;
  sInt newline;

  sVERIFY(OpCodes[KB_MAX].Name[0]=='?');
  out->Init("");
  codeend = script->Code+script->CodeCount;
  code = codestart = script->Code;
  index = 0;
  error = 0;
  if(!verbose)
  {
    for(i=0;i<script->CodeCount;i++)
    {
      if((i&15)==0)
        out->PrintF("%04x ",i);
      out->PrintF(" %02x",code[i]);
      if((i&15)==15)
        out->PrintF("\n",i);
    }
    if((script->CodeCount&15)!=0)
      out->PrintF("\n");
  }

  newline = 2;

  while(code<codeend)
  {
    if(newline || verbose)
    {
      if(newline!=2)
        out->PrintF("\n");
      out->PrintF("%04x: ",code-codestart);
      listcode = code;
      listchar = out->Used;
      if(verbose)
        out->PrintF("                       ");
      newline = 0;
    }

    byte = *code++;
    if(byte>=0 && byte<KB_MAX)
      op = &OpCodes[byte];
    else 
      op = &OpCodes[KB_MAX];

    if(op->Flags & OP_ERROR)
    {
      out->PrintF("!OP!");
      error |= 1;
    }
    else
    {
      out->PrintF(op->Name);
    }
    if(op->Flags & OP_EXTERN)
      out->PrintF("[%d]",getoffset(code));
    if(op->Flags & OP_OFFSET)
      out->PrintF("(%d)",getoffset(code));
    if(op->Flags & OP_JUMP)
      out->PrintF("@%04x",getoffset(code));
    if(op->Flags & OP_ARGS)
      out->PrintF("#%d",getoffset(code));
    fp = (sF32 *)code;
    if(op->Flags & OP_DATA1)
    {
      out->PrintF("<%f>",fp[0]);
      code+=4;
    }
    if(op->Flags & OP_DATA4)
    {
      out->PrintF("<%f,%f,%f,%f>",fp[0],fp[1],fp[2],fp[3]);
      code+=16;
    }
    if(op->Flags & OP_TEXT)
    {
      out->PrintF("\"%s\"",code);
      while(*code++);
    }
    out->PrintF(" ");

    if((error&2)==0)
    {
      for(i=3;i>=0;i--)
      {
        if(op->In[i])
        {
          if(index<=0)
          {
            out->PrintF("UNDERFLOW! ");
            error |= 2;
            index = 1;
          }
          index--;
          if(stack[index]!=op->In[i])
          {
            out->PrintF("TYPE! ");
            error |= 4;
            index = 1;
          }
        }
      }
      if(op->Out)
        stack[index++] = op->Out;
    }
    if(verbose)
    {
      static const sChar hex[]="0123456789abcdef";
      i = 7;
      j = listchar;
      while(listcode<code && i>0)
      {
        out->Text[j++] = hex[(*listcode)>>4];
        out->Text[j++] = hex[(*listcode)&15];
        j++;
        listcode++;
        i--;
      }
      if(listcode!=code)
      {
        out->Text[j++] = '.';
        out->Text[j++] = '.';
      }
      do
        out->Print(" ");
      while(out->Used<listchar+40);
      static const sChar *types = "SVMO";
      out->Print("; ");
      for(i=0;i<index;i++)
        out->PrintF("%c",types[(stack[i]-1)&3]);
    }
    if(index==0 || (op->Flags & OP_RET))
      newline = 1;
  }
  out->PrintF("\n");
  if(index!=0)
    out->PrintF("stack error :%d\n",index);

  return error==0;
}

/****************************************************************************/

void sScriptCompiler::PrintSymbols()
{
  sInt i;
  sScriptSymbol *sym;

  for(i=0;i<Symbols.Count;i++)
  {
    sym = &Symbols[i];
    if(sym->Flags & sSS_EXTERN)
      sDPrintF("extern ");
    switch(sym->Type)
    {
    case sST_SCALAR:
      sDPrintF("scalar %s; // at %d\n",sym->Name,sym->Offset);
      break;
    case sST_VECTOR:
      sDPrintF("vector %s; // at %d\n",sym->Name,sym->Offset);
      break;
    case sST_MATRIX:
      sDPrintF("matrix %s; // at %d\n",sym->Name,sym->Offset);
      break;
    case sST_OBJECT:
      sDPrintF("object %s; // at %d\n",sym->Name,sym->Offset);
      break;
    default:
      sDPrintF("// error %d %s @ %d;",sym->Type,sym->Name,sym->Offset);
      break;
    }
  }
  if(sym->Offset==0)
    sDPrintF("// end of symbols: %d\n",VarUsed);
}

/****************************************************************************/
/****************************************************************************/

sInt sScriptCompiler::Scan()
{
  sChar *s;
  sF64 val,dec,frac;
  sInt i;

  Token = TOK_ERROR;
  Value = 0;
  Name[0] = 0;
  String[0] = 0;

  for(;;)
  {
    // skip whitespace

    while(*ScanPtr==' ' || *ScanPtr=='\n' || *ScanPtr=='\t' || *ScanPtr=='\r')
    {
      if(*ScanPtr=='\n') Line++;
      ScanPtr++;
    }

    // handle comments
    if(ScanPtr[0]=='/' && ScanPtr[1]=='/')      // eol comment
    {
      ScanPtr+=2;
      while(*ScanPtr!='\n' && *ScanPtr!=0)
        ScanPtr++;
    }
    else if(ScanPtr[0]=='/' && ScanPtr[1]=='*')   // block comment
    {
      ScanPtr+=2;      
      while(*ScanPtr!=0)
      {
        if(ScanPtr[0]=='*' && ScanPtr[1]=='/')
        {
          ScanPtr+=2;
          break;
        }
        if(*ScanPtr=='\n')
          Line++;
        ScanPtr++;
      }
      if(*ScanPtr==0)
        return Token;
    }
    else                                  // no comment
    {
      break;
    }
  }


  switch(*ScanPtr++)
  {
  case 0:
    ScanPtr--;
    Token = TOK_EOF;
    break;

  case '(': Token = TOK_OPEN; break;
  case ')': Token = TOK_CLOSE; break;
  case '[': Token = TOK_SOPEN; break;
  case ']': Token = TOK_SCLOSE; break;
  case '{': Token = TOK_BOPEN; break;
  case '}': Token = TOK_BCLOSE; break;
  case ':': Token = TOK_COLON; break;
  case ';': Token = TOK_SEMI; break;
  case '.': Token = TOK_DOT; break;
  case ',': Token = TOK_COMMA; break;
  case '?': Token = TOK_QUESTION; break;

  case '+': Token = TOK_PLUS; break;
  case '-': Token = TOK_MINUS; break;
  case '*': Token = TOK_MUL; break;
  case '/': Token = TOK_DIV; 
  case '%': Token = TOK_DIV; 
    break;

  case '=': 
    if(*ScanPtr=='=')
    {
      Token = TOK_EQ;
      ScanPtr++;
    }
    else
    {
      Token = TOK_ASSIGN; 
    }
    break;
  case '>': 
    if(*ScanPtr=='=')
    {
      Token = TOK_GE;
      ScanPtr++;
    }
    else
    {
      Token = TOK_GT; 
    }
    break;
  case '<': 
    if(*ScanPtr=='=')
    {
      Token = TOK_LE;
      ScanPtr++;
    }
    else
    {
      Token = TOK_LT; 
    }
    break;
  case '!': 
    if(*ScanPtr=='=')
    {
      Token = TOK_NE;
      ScanPtr++;
    }
    else
    {
      Token = TOK_NOT; 
    }
    break;

  case '"':
    s = String;
    while(*ScanPtr!='"' && *ScanPtr!=0 && s+1<String+sizeof(String))
    {
      if(*ScanPtr=='\n')
        Line++;
      if(*ScanPtr=='\\')
      {
        ScanPtr++;
        switch(*ScanPtr++)
        {
        case 'n':
          *s++ = '\n';
          break;
        case 'r':
          *s++ = '\r';
          break;
        case 'f':
          *s++ = '\f';
          break;
        case 't':
          *s++ = '\t';
          break;
        default:
          *s++ = '?';
          break;
        }
      }
      else
      {
        *s++ = *ScanPtr++;
      }
    }
    *s++ = 0;
    if(*ScanPtr=='"')
    {
      Token = TOK_STRING;
      ScanPtr++;
    }
    break;

  default:
    ScanPtr--;
    if((*ScanPtr>='0' && *ScanPtr<='9') || *ScanPtr=='.')
    {
      val = 0;
      dec = 1;
      frac = 0;

      while(*ScanPtr>='0' && *ScanPtr<='9')
      {
        val = val*10;
        val = val + ((*ScanPtr++)-'0');
      }
      if(*ScanPtr=='.')
      {
        *ScanPtr++;
        while(*ScanPtr>='0' && *ScanPtr<='9')
        {
          frac = frac*10;
          frac = frac + ((*ScanPtr++)-'0');
          dec = dec*10;
        }
        val = val + (frac/dec);       
      }
      Value = val;
      Token = TOK_FLOAT;
    }
    else if((*ScanPtr>='a' && *ScanPtr<='z') || (*ScanPtr>='A' && *ScanPtr<='Z') || *ScanPtr=='_' || *ScanPtr=='$')
    {
      s = Name;
      while((*ScanPtr>='a' && *ScanPtr<='z') || (*ScanPtr>='A' && *ScanPtr<='Z') || *ScanPtr=='_' || *ScanPtr=='$' || (*ScanPtr>='0' && *ScanPtr<='9'))
      {
        *s++ = *ScanPtr++;
        if(s-Name>=sSCRIPTNAME)
          return Token;
      }
      *s++ = 0;
      Token = TOK_NAME;

      for(i=0;tokennames[i].name;i++)
      {
        if(sCmpString(tokennames[i].name,Name)==0)
        {
          Token = tokennames[i].token;
          break;
        }
      }
    }
    else
    {
      // error
    }
    break;
  }
  return Token;
}


void sScriptCompiler::Error(const sChar *name,...)
{
  if(ErrorLine==0)
  {
    ErrorCount++;
    Errors->PrintF("(%d):",Line);
    Errors->PrintArg(name,&name);
    Errors->Print("\n");
  }
  ErrorLine++;
}

sBool sScriptCompiler::Match(sInt token)
{
  if(token!=Token)
  {
    Error("syntax error");
    return 1;
  }
  Scan();
  return 0;
}

/****************************************************************************/

sScriptNode *sScriptCompiler::GetNode(sInt code,sInt type)
{
  sScriptNode *node;
  node = &Nodes[NodeUsed++];
  sVERIFY(NodeUsed<NodeAlloc);
  
  sSetMem(node,0,sizeof(*node));
  node->Code = code;
  node->Type = type;

  return node;
}

void sScriptCompiler::EmitByte(sInt code)
{
  if(CodeUsed+1<CodeAlloc && code>=0 && code<0xff)
    Code[CodeUsed++] = code;
  else
    Error("emit");
}

void sScriptCompiler::EmitOffset(sU32 offset)
{
  if(CodeUsed+5<CodeAlloc)
  {
    if(offset>=(1<<28)) Code[CodeUsed++] = ((offset>>28)&0x7f)|0x80;
    if(offset>=(1<<21)) Code[CodeUsed++] = ((offset>>21)&0x7f)|0x80;
    if(offset>=(1<<14)) Code[CodeUsed++] = ((offset>>14)&0x7f)|0x80;
    if(offset>=(1<<7 )) Code[CodeUsed++] = ((offset>> 7)&0x7f)|0x80;
                        Code[CodeUsed++] =   offset     &0x7f;
  }
  else
    Error("emit");
}

void sScriptCompiler::EmitOffset2(sInt offset)
{
  Code[CodeUsed++] = ((offset>> 7)&0x7f)|0x80;
  Code[CodeUsed++] =   offset     &0x7f;
}

void sScriptCompiler::EmitOffset2(sInt offset,sU8 *ptr)
{
  ptr[0] = ((offset>> 7)&0x7f)|0x80;
  ptr[1] =   offset     &0x7f;
}

void sScriptCompiler::EmitFloat(sF32 val)
{
  if(CodeUsed+4<CodeAlloc)
  {
    *(sF32*)&Code[CodeUsed] = val;
    CodeUsed+=4;
  }
  else
    Error("emit");
}

void sScriptCompiler::EmitString(const sChar *string)
{
  sInt size;
  size = sGetStringLen(string)+1;
  if(CodeUsed+size<CodeAlloc)
  {
    sCopyMem(Code+CodeUsed,string,size);
    CodeUsed+=size;
  }
  else
    Error("emit");
}

/****************************************************************************/

sScriptSymbol *sScriptCompiler::FindSymbol(const sChar *name)
{
  sInt i;

  for(i=0;i<Symbols.Count;i++)
  {
    if(sCmpString(Symbols[i].Name,name)==0)
      return &Symbols[i];
  }
  for(i=0;i<Locals.Count;i++)
  {
    if(sCmpString(Locals[i].Name,name)==0)
      return &Locals[i];
  }
  return 0;
}

/****************************************************************************/


sScriptCompiler::sScriptCompiler()
{
  Symbols.Init();
  Locals.Init();
  NodeAlloc = 0x1000;
  NodeUsed = 0;
  Nodes = new sScriptNode[NodeAlloc];
  CodeAlloc = 0x2000;
  CodeUsed = 0;
  Code = new sU8[CodeAlloc];
}

sScriptCompiler::~sScriptCompiler()
{
  Locals.Exit();
  Symbols.Exit();
  sDeleteArray(Nodes);
  sDeleteArray(Code);
}

sBool sScriptCompiler::ExternSymbol(sChar *group,sChar *name,sU32 &groupid,sU32 &offset)
{
  return sFALSE;
}

sBool sScriptCompiler::Compiler(sScript *Script,const sChar *source,sText *errors)
{
  sScriptNode *node;

  Line = 1;
  ErrorCount = 0;
  ErrorLine = 0;
  CodeUsed = 0;
  ScanPtr = source;
  VarUsed = 0;
  Symbols.Count = 0;
  Errors = errors;
  Errors->Clear();

  Scan();
  while(Token!=TOK_EOF)
  {
    NodeUsed = 0;
    node = _Statement();
    if(node && ErrorCount==0)
      Generate(node);
  }
  EmitByte(KB_RET);

  if(ErrorCount!=0)
  {
    Errors->PrintF("there were %d errors\n",ErrorCount);
    CodeUsed = 0;
    EmitByte(KB_RET);
  }

  Errors = 0;
  delete[] Script->Code;
  delete[] Script->Var;
  Script->Code = new sU8[CodeUsed];
  Script->CodeCount = CodeUsed;
  Script->Var = new sF32[VarUsed];
  Script->VarCount = VarUsed;
  Script->ErrorCount = ErrorCount;
  sCopyMem(Script->Code,Code,CodeUsed);
  sSetMem(Script->Var,0,VarUsed*4);
  return ErrorCount==0;
}

void sScriptCompiler::ClearLocals()
{
  Locals.Count = 0;
}

void sScriptCompiler::AddLocal(const sChar *name,sInt type,sInt flags,sInt global,sInt offset)
{
  sScriptSymbol *sym;
  sym = Locals.Add();
  sCopyString(sym->Name,name,sSCRIPTNAME);
  sym->Type = type;
  sym->Flags = flags;
  sym->Extern = global;
  sym->Offset = offset;
}

/****************************************************************************/

sScriptNode *sScriptCompiler::_Block()
{
  sScriptNode *start,*node,**link;

  start = GetNode(KB_NEXT,sST_CODE);
  link = &start->Right;
  if(Token==TOK_BOPEN)
  {
    Scan();
    while(Token!=TOK_BCLOSE)
    {
      *link = node = GetNode(KB_NEXT,sST_CODE);
      node->Left = _Statement();
      link = &node->Right;
    }
    Match(TOK_BCLOSE);
    return start;
  }
  else
  {
    return _Statement();
  }
}

/****************************************************************************/

sScriptNode *sScriptCompiler::_Statement()
{
  sScriptNode *node,*n2;

  ErrorLine = 0;
  node = 0;

  switch(Token)
  {
  case TOK_SCALAR:
    _Define(sST_SCALAR,1,0);
    break;
  case TOK_VECTOR:
    _Define(sST_VECTOR,4,0);
    break;
  case TOK_MATRIX:
    _Define(sST_MATRIX,16,0);
    break;
  case TOK_OBJECT:
    _Define(sST_OBJECT,1,0);
    break;

  case TOK_EXTERN:
    Scan();
    switch(Token)
    {
    case TOK_SCALAR:
      _Define(sST_SCALAR,1,sSS_EXTERN);
      break;
    case TOK_VECTOR:
      _Define(sST_VECTOR,4,sSS_EXTERN);
      break;
    case TOK_MATRIX:
      _Define(sST_MATRIX,16,sSS_EXTERN);
      break;
    case TOK_OBJECT:
      _Define(sST_OBJECT,1,sSS_EXTERN);
      break;
    default:
      Error("illegal type for extern");
    }
    break;

  case TOK_IF:
    node = _If();
    break;
  case TOK_WHILE:
    node = _While();
    break;
  case TOK_DO:
    node = _DoWhile();
    break;
  case TOK_BOPEN:
    node = _Block();
    break;

  case TOK_NAME:
    if(sCmpString(Name,"print")==0)
    {
      Scan();
      Match(TOK_OPEN);
      if(ErrorLine==0)
      {
        n2 = _Expression();
      }
      Match(TOK_CLOSE);
      Match(TOK_SEMI);
      if(ErrorLine==0)
      {
        node = GetNode(TOK_ERROR,n2->Type);
        node->Left = n2;
        if(node->Type==sST_STRING) node->Code=KB_PRINT;
        else if(node->Type==sST_SCALAR) node->Code=KB_PRINTS;
        else if(node->Type==sST_VECTOR) node->Code=KB_PRINTV;
        else if(node->Type==sST_MATRIX) node->Code=KB_PRINTM;
        else if(node->Type==sST_OBJECT) node->Code=KB_PRINTO;
      }
      break;
    }
    // fall;
  default:
    node = _Expression();
    if(node==0)
    {
      Scan();
    }
    else
    {
      if(Token==TOK_ASSIGN)
      {
        Scan();
        if(node->Code==KB_LOADSYM)
        {
          sVERIFY(node->Symbol);
          if(node->Symbol->Flags & sSS_CONST)
            Error("writing to constant");
          node->Code=KB_STORESYM;

          node->Left = _Expression();
          if(node->Left->Type!=node->Type)
            Error("type mismatch");
        }
        else
        {
          Error("lvalue required");
        }
      }
      Match(TOK_SEMI);
    }
    break;
  }

  return node;
}

sScriptNode *sScriptCompiler::_Expression()
{
  sScriptNode *node,*nx,*na,*nb;
  node = _Sum();
  if(Token==TOK_QUESTION)
  {
    if(node->Type!=sST_SCALAR)
      Error("type mismatch");

    nx = node;
    Scan();
    na = _Sum();
    Match(TOK_COLON);
    nb = _Sum();

    node = GetNode(KB_ERROR,sST_ERROR);
    node->Left = na;
    node->Right = GetNode(KB_NEXT,0);
    node->Right->Left = nb;
    node->Right->Right = nx;

    if(ErrorLine==0)
    {
      if(na->Type!=nb->Type)
        Error("type mismatch");
      node->Type = node->Right->Type = na->Type;
      if(node->Type==sST_SCALAR)
        node->Code = KB_CONDS;
      else if(node->Type==sST_VECTOR)
        node->Code = KB_CONDV;
      else if(node->Type==sST_MATRIX)
        node->Code = KB_CONDM;
      else if(node->Type==sST_OBJECT)
        node->Code = KB_CONDO;
      else 
        Error("type mismatch");
    }
  }

  return node;
}

sScriptNode *sScriptCompiler::_Sum()
{
  sScriptNode *node,*n2;
  sInt token;

  node = _Term();
  while(Token==TOK_PLUS || Token==TOK_MINUS)
  {
    token = Token;
    Scan();

    n2 = node;
    node = GetNode(KB_ERROR,sST_ERROR);
    node->Left = n2;
    node->Right = _Term();

    if(token==TOK_PLUS && node->Left->Type==sST_SCALAR && node->Right->Type==sST_SCALAR)
    {
      node->Type = sST_SCALAR;
      node->Code = KB_ADDS;
    }
    else if(token==TOK_PLUS && node->Left->Type==sST_VECTOR && node->Right->Type==sST_VECTOR)
    {
      node->Type = sST_VECTOR;
      node->Code = KB_ADDS;
    }
    else if(token==TOK_MINUS && node->Left->Type==sST_SCALAR && node->Right->Type==sST_SCALAR)
    {
      node->Type = sST_SCALAR;
      node->Code = KB_SUBS;
    }
    else if(token==TOK_MINUS && node->Left->Type==sST_VECTOR && node->Right->Type==sST_VECTOR)
    {
      node->Type = sST_VECTOR;
      node->Code = KB_SUBS;
    }
    else
    {
      Error("type mismatch");
    }
  }
  return node;
}

sScriptNode *sScriptCompiler::_Term()
{
  sScriptNode *node,*n2;
  sInt token;

  node = _Compare();
  while(Token==TOK_MUL || Token==TOK_DIV || Token==TOK_MOD)
  {
    token = Token;
    Scan();

    n2 = node;
    node = GetNode(KB_ERROR,sST_ERROR);
    node->Left = n2;
    node->Right = _Compare();

    if(token==TOK_MUL && node->Left->Type==sST_SCALAR && node->Right->Type==sST_SCALAR)
    {
      node->Type = sST_SCALAR;
      node->Code = KB_MULS;
    }
    else if(token==TOK_MUL && node->Left->Type==sST_VECTOR && node->Right->Type==sST_VECTOR)
    {
      node->Type = sST_VECTOR;
      node->Code = KB_MULS;
    }
    else if(token==TOK_MUL && node->Left->Type==sST_MATRIX && node->Right->Type==sST_MATRIX)
    {
      node->Type = sST_MATRIX;
      node->Code = KB_MULM;
    }
    else if(token==TOK_MOD && node->Left->Type==sST_SCALAR && node->Right->Type==sST_SCALAR)
    {
      node->Type = sST_SCALAR;
      node->Code = KB_MODS;
    }
    else if(token==TOK_MOD && node->Left->Type==sST_VECTOR && node->Right->Type==sST_VECTOR)
    {
      node->Type = sST_VECTOR;
      node->Code = KB_CROSS;
    }
    else if(token==TOK_DIV && node->Left->Type==sST_SCALAR && node->Right->Type==sST_SCALAR)
    {
      node->Type = sST_SCALAR;
      node->Code = KB_DIVS;
    }
    else
    {
      Error("type mismatch");
    }
  }
  return node;
}

sScriptNode *sScriptCompiler::_Compare()
{
  sScriptNode *nnot,*ncmp,*nsub,*node;
  sInt token;

  node = _Value();
  while(Token==TOK_NE || Token==TOK_EQ || Token==TOK_GT || Token==TOK_GE || Token==TOK_LT || Token==TOK_LE)
  {
    token = Token;
    Scan();

    nnot = GetNode(KB_EQ,sST_SCALAR);
    ncmp = GetNode(KB_ERROR,sST_SCALAR);
    nsub = GetNode(KB_SUBS,sST_SCALAR);
    nnot->Left = ncmp;
    ncmp->Left = nsub;
    nsub->Left = node;
    nsub->Right = _Value();

    node = ncmp;

    if(nsub->Left->Type==sST_SCALAR && nsub->Right->Type==sST_SCALAR)
    {
      if(token==TOK_NE)  ncmp->Code = KB_NE;
      if(token==TOK_EQ)  ncmp->Code = KB_EQ;
      if(token==TOK_GT)  ncmp->Code = KB_GT;
      if(token==TOK_GE)  ncmp->Code = KB_GE;

      if(token==TOK_LT)  { ncmp->Code = KB_GE; node = nnot; }
      if(token==TOK_LE)  { ncmp->Code = KB_GT; node = nnot; }
    }
    else
    {
      Error("illegal type");
    }
  }
  return node;
}

sScriptNode *sScriptCompiler::_Value()
{
  sScriptNode *node,*n2,*n3,*n4;
  sScriptSymbol *sym;
  node = 0;

  switch(Token)
  {
  case TOK_MINUS:
    Scan();
    node = GetNode(KB_NEG,sST_SCALAR);
    node->Left = _Value();
    if(node->Left->Type!=sST_SCALAR)
      Error("type mismatch");
    break;
  case TOK_NOT:
    Scan();
    node = GetNode(KB_EQ,sST_SCALAR);
    node->Left = _Value();
    if(node->Left->Type!=sST_SCALAR)
      Error("type mismatch");
    break;
  case TOK_FLOAT:
    node = GetNode(KB_LITS,sST_SCALAR);
    node->Val[0] = Value;
    Scan();
    break;
  case TOK_OPEN:
    Scan();
    node = _Expression();
    Match(TOK_CLOSE);
    break;
  case TOK_STRING:
    if(sGetStringLen(String)>15)
      Error("string too long (15 is max)");
    node = GetNode(KB_STRING,sST_STRING);
    sCopyString((sChar *)node->Val,String,16);
    Scan();
    break;
  case TOK_SOPEN:
    Scan();
    node = GetNode(KB_ERROR,sST_ERROR);
    n2   = node->Right = GetNode(KB_NEXT,0);
    n3   = n2  ->Right = GetNode(KB_NEXT,0);
    n4   = n3  ->Right = GetNode(KB_NEXT,0);
    node->Left = _Expression();
    Match(TOK_COMMA); 
    n2  ->Left = _Expression();
    Match(TOK_COMMA);
    n3  ->Left = _Expression();
    Match(TOK_COMMA);
    n4  ->Left = _Expression();
    Match(TOK_SCLOSE);
    if(!ErrorLine)
    {
      if(node->Left->Type==sST_SCALAR && n2->Left->Type==sST_SCALAR && n3->Left->Type==sST_SCALAR && n4->Left->Type==sST_SCALAR)
      {
        node->Code = KB_COMBV;
        node->Type = sST_VECTOR;
      }
      else if(node->Left->Type==sST_VECTOR && n2->Left->Type==sST_VECTOR && n3->Left->Type==sST_VECTOR && n4->Left->Type==sST_VECTOR)
      {
        node->Code = KB_COMBM;
        node->Type = sST_MATRIX;
      }
      else
      {
        Error("type error");
      }
    }
    break;
  case TOK_NAME:
    sym = FindSymbol(Name);
    if(sym)
    {
      Scan();
      node = GetNode(KB_LOADSYM,sym->Type);
      node->Symbol = sym;
    }
    else
    {
      for(sInt i=0;Intrinsics[i].Name;i++)
      {
        if(sCmpString(Intrinsics[i].Name,Name)==0)
        {
          Scan();
          node = _Intrinsic(&Intrinsics[i]);
          break;
        }
      }
      if(node==0)
      {
        Error("symbol not found");
        Scan();
      }
    }
    break;
  default:
    Error("what?");
    node = GetNode(KB_ERROR,sST_ERROR);
    break;
  }

  while(Token==TOK_DOT && node)
  {
    sInt i;
    Scan();
    n2 = node;
    node = 0;
    i = 0;
    if(Token==TOK_NAME && n2->Type==sST_VECTOR)
    {
      if(sCmpString(Name,"x")==0) { node=GetNode(KB_SELECTX,sST_SCALAR); i=0; }
      if(sCmpString(Name,"y")==0) { node=GetNode(KB_SELECTY,sST_SCALAR); i=1; }
      if(sCmpString(Name,"z")==0) { node=GetNode(KB_SELECTZ,sST_SCALAR); i=2; }
      if(sCmpString(Name,"w")==0) { node=GetNode(KB_SELECTW,sST_SCALAR); i=3; }
    }
    if(Token==TOK_NAME && n2->Type==sST_MATRIX)
    {
      if(sCmpString(Name,"i")==0) { node=GetNode(KB_SELECTI,sST_VECTOR); i=0; }
      if(sCmpString(Name,"j")==0) { node=GetNode(KB_SELECTJ,sST_VECTOR); i=4; }
      if(sCmpString(Name,"k")==0) { node=GetNode(KB_SELECTK,sST_VECTOR); i=8; }
      if(sCmpString(Name,"l")==0) { node=GetNode(KB_SELECTL,sST_VECTOR); i=12; }
    }
    Scan();
    if(node)
    {
      if(n2->Code==KB_LOADSYM || n2->Code==KB_STORESYM)
      {
        n2->Select += i;
        n2->Type = node->Type;
        node = n2;
      }
      else
      {
        node->Left = n2;
      }
    }
    else
    {
      Error(".select mismatch");
    }
  }

  return node;
}

sScriptNode *sScriptCompiler::_Intrinsic(const struct Intrinsic *intr)
{
  sScriptNode *node;

  node = GetNode(intr->Code,intr->Out);
  node->Select = intr->Extern;
  if(intr->In[0])
  {
    Match(TOK_OPEN);
    node->Left = _Expression();
    if(intr->In[1])
    {
      Match(TOK_COMMA);
      node->Right = _Expression();
    }
    Match(TOK_CLOSE);
  }
  return node;
}

/****************************************************************************/

sScriptNode *sScriptCompiler::_If()
{
  sScriptNode *n1,*n2,*n3,*n4;

  n1 = GetNode(KB_NEXT,sST_CODE);
  n2 = GetNode(KB_NEXT,sST_CODE);
  n3 = GetNode(KB_NEXT,sST_CODE);
  n4 = GetNode(KB_NEXT,sST_CODE);

  n1->Right = n2;
  n2->Left = GetNode(KB_JUMPF,sST_CODE);
  n2->Right = n3;
  n3->Right = n4;
  n4->Left = GetNode(KB_JUMP,sST_CODE);

  Scan();
  Match(TOK_OPEN);
  n1->Left = _Expression();
  Match(TOK_CLOSE);
  n3->Left = _Block();
  if(Token==TOK_ELSE)
  {
    Match(TOK_ELSE);
    n4->Right = _Block();
    n4->Right->BackPatch = n4->Left;
    n4->Left->BackPatch = n2->Left;
  }
  else
  {
    n2->Right = n3->Left;   // remove else branch
    n2->Right->BackPatch = n2->Left;
  }

  return n1;
}

sScriptNode *sScriptCompiler::_While()
{
  sScriptNode *n1,*n2,*n3;

  n1 = GetNode(KB_NEXT,sST_CODE);
  n2 = GetNode(KB_NEXT,sST_CODE);
  n3 = GetNode(KB_NEXT,sST_CODE);

  n1->Left = GetNode(KB_JUMP,sST_CODE);
  n1->Right = n2;
  n2->Right = n3;
  n3->Right = GetNode(KB_JUMPT,sST_CODE);

  Scan();
  Match(TOK_OPEN);
  n3->Left = _Expression();
  Match(TOK_CLOSE);
  n2->Left = _Block();

  n2->Left->BackPatch = n1->Left;
  n3->Right->ForePatch = n2->Left;

  return n1;
}

sScriptNode *sScriptCompiler::_DoWhile()
{
  sScriptNode *node;

  node = GetNode(KB_JUMPT,sST_CODE);
  Scan();
  node->Left = _Block();
  Match(TOK_WHILE);
  Match(TOK_OPEN);
  node->Right = _Expression();
  Match(TOK_CLOSE);
  Match(TOK_SEMI);
  node->ForePatch = node->Left;

  return node;
}


/****************************************************************************/

void sScriptCompiler::_Define(sInt type,sInt size,sInt flags)
{
  sScriptSymbol *sym;

  Scan();   // type 

  for(;;)
  {
    if(Token==TOK_NAME)
    {
      sym = Symbols.Add();
      sCopyString(sym->Name,Name,sizeof(sym->Name));
      sym->Type = type;
      sym->Offset = VarUsed;
      sym->Flags = flags;
      VarUsed += size;
    }
    Match(TOK_NAME);
    if(Token!=TOK_COMMA) break;
    Match(TOK_COMMA);
  }
  Match(TOK_SEMI);
}

/****************************************************************************/
/****************************************************************************/

void sScriptCompiler::GenStatement(sScriptNode *node)
{
  const OpCode *op;

  if(!node) return;

  node->CodeOffset = CodeUsed;
  GenStatement(node->Left);
  GenStatement(node->Right);

  switch(node->Code)
  {
  case KB_STRING:
    sVERIFY(node->Left==0);
    sVERIFY(node->Right==0);
    // is handled by KB_PRINT 
    break;

  case KB_NEXT:
    // nothing to do...
    break;

  case KB_LOADSYM:
    sVERIFY(node->Type>=sST_SCALAR && node->Type<=sST_OBJECT);
    sVERIFY(node->Symbol);
    EmitByte(node->Type-sST_SCALAR+((node->Symbol->Flags&sSS_EXTERN)?KB_ELOADS:KB_LOADS));
    if(node->Symbol->Flags&sSS_EXTERN)
      EmitOffset(node->Symbol->Extern);
    EmitOffset(node->Symbol->Offset+node->Select);
    break;

  case KB_STORESYM:
    sVERIFY(node->Type>=sST_SCALAR && node->Type<=sST_OBJECT);
    sVERIFY(node->Symbol);
    EmitByte(node->Type-sST_SCALAR+((node->Symbol->Flags&sSS_EXTERN)?KB_ESTORES:KB_STORES));
    if(node->Symbol->Flags&sSS_EXTERN)
      EmitOffset(node->Symbol->Extern);
    EmitOffset(node->Symbol->Offset+node->Select);
    break;

  case KB_OBJECT:
    EmitByte(node->Code);
    EmitOffset(node->Select);
    break;

  default:
    sVERIFY(node->Code!=KB_ERROR);
    sVERIFY(node->Type!=sST_ERROR);
    sVERIFY(node->Code>=0 && node->Code<KB_MAX);

    op = &OpCodes[node->Code];

    EmitByte(node->Code);
    if(op->Flags & OP_JUMP)
    {
      if(node->ForePatch)
        EmitOffset2(node->ForePatch->CodeOffset);
      else
        EmitOffset2(0);
    }
    if(op->Flags & OP_TEXT)
    {
      sVERIFY(node->Left && node->Left->Code==KB_STRING);
      EmitString((sChar *)node->Left->Val);
    }
    if(op->Flags & OP_DATA1)
      EmitFloat(node->Val[0]);
    if(op->Flags & OP_DATA4)
    {
      EmitFloat(node->Val[0]);
      EmitFloat(node->Val[1]);
      EmitFloat(node->Val[2]);
      EmitFloat(node->Val[3]);
    }
    break;
  }
  if(node->BackPatch)
    EmitOffset2(CodeUsed,Code+node->BackPatch->CodeOffset+1);
}

void sScriptCompiler::Generate(sScriptNode *node)
{
  GenStatement(node);
}

/****************************************************************************/

#endif
