// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "shadercompile.hpp"
#include "_startdx.hpp"

/****************************************************************************/

enum ShaderToken
{
  TOK_EOW = 256,    // end of word
  TOK_WORD,

  TOK_DOTDOT,       // ".."

  // predefined word tokens from here on
  TOK_FIRSTPREDEF = 512,
  TOK_DCL = TOK_FIRSTPREDEF,
  TOK_DEF,
  TOK_NOP,
  TOK_TEXKILL,
  TOK_TEMP,
  TOK_ALIAS,
  TOK_VMOV,
  TOK_IF,
  TOK_ELSE,
  TOK_ELIF,
  TOK_ENDIF,
  TOK_FOR,
  TOK_ENDFOR,
  TOK_WHILE,
  TOK_ENDWHILE,
  TOK_FLAGS,
  TOK_ENDFLAGS,
  TOK_ERROR,
};

static const sChar *Tokens[] =
{
  "dcl",
  "def",
  "nop",
  "texkill",
  "temp",
  "alias",
  "vmov",
  "if",
  "else",
  "elif",
  "endif",
  "for",
  "endfor",
  "while",
  "endwhile",
  "flags",
  "endflags",
  "error",
  0,
};

struct ShaderInstr
{
  sU32 opcode;
  const sChar *name;
  sInt params;
  sU8 vsver;
  sU8 psver;
  sU16 flags;
};

enum ControlStructure
{
  CS_IF,
  CS_WHILE,
  CS_FOR,
};

static const sChar *controlNames[] =
{
  "if",
  "while",
  "for",
};

enum ShaderInstrFlags
{
  SIF_NONE            = 0x0000, // no flags
  SIF_NOCOISSUE       = 0x0001, // cannot be coissued
  SIF_VECTORONLY      = 0x0002, // must be issued in vector pipe
  SIF_REPLICATELAST   = 0x0004, // last parameter must have replicate swizzle
  SIF_NOSWIZZLE       = 0x0008, // source parameters must have default swizzle
  SIF_NOSRCDESTEQUAL  = 0x0010, // dest != all sources
  SIF_DESTMUSTTEMP    = 0x0020, // dest register must be temporary
  SIF_DESTNOW         = 0x0040, // destination write mask may not include w
  SIF_VS1xWRITEYORXY  = 0x0080, // for vs1.1, may only write .y or .xy
  SIF_LASTNOSWIZZLE   = 0x0100, // last parameter may not have swizzle
  SIF_REPLICATE       = 0x0200, // all sources must have replicate swizzle
  SIF_NOWRITEMASK     = 0x0400, // write mask must be .xyzw (default)
  SIF_TEXLD20         = 0x0800, // second parameter must be a sampler
  SIF_WRITEXY         = 0x1000, // write mask must be .xy
  SIF_WRITEXYZ        = 0x2000, // write mask must be .xyz
  SIF_DESTMUSTA0      = 0x4000, // destination register must be a0
  SIF_SRC23DIFFER     = 0x8000, // all source register need to be unique
};

static const ShaderInstr ShaderInstrs[] =
{
  { XO_ABS,     "abs",    1, 0x20,0x20, SIF_NONE },
  { XO_ADD,     "add",    2, 0x10,0x10, SIF_NONE },
  { XO_CMP,     "cmp",    3, 0x00,0x10, SIF_NOSRCDESTEQUAL },
  { XO_CND,     "cnd",    3, 0x00,0x10, SIF_NONE },
  { XO_CRS,     "crs",    2, 0x20,0x20, SIF_NOSWIZZLE|SIF_NOSRCDESTEQUAL|SIF_DESTMUSTTEMP|SIF_DESTNOW },
  { XO_DP2ADD,  "dp2add", 3, 0x00,0x20, SIF_REPLICATELAST },
  { XO_DP3,     "dp3",    2, 0x10,0x10, SIF_VECTORONLY },
  { XO_DP4,     "dp4",    2, 0x10,0x12, SIF_NOCOISSUE },
  { XO_DST,     "dst",    2, 0x10,0x00, SIF_NONE },
  { XO_EXP,     "exp",    1, 0x10,0x20, SIF_REPLICATELAST },
  { XO_EXPP,    "expp",   1, 0x10,0x00, SIF_REPLICATELAST },
  { XO_FRC,     "frc",    1, 0x10,0x20, SIF_VS1xWRITEYORXY },
  { XO_LOG,     "log",    1, 0x10,0x20, SIF_REPLICATELAST },
  { XO_LOGP,    "logp",   1, 0x10,0x00, SIF_REPLICATELAST },
  { XO_LRP,     "lrp",    3, 0x20,0x10, SIF_NONE },
  { XO_M3x2,    "m3x2",   2, 0x10,0x20, SIF_LASTNOSWIZZLE|SIF_WRITEXY },
  { XO_M3x3,    "m3x3",   2, 0x10,0x20, SIF_LASTNOSWIZZLE|SIF_WRITEXYZ },
  { XO_M3x4,    "m3x4",   2, 0x10,0x20, SIF_LASTNOSWIZZLE|SIF_NOWRITEMASK },
  { XO_M4x3,    "m4x3",   2, 0x10,0x20, SIF_LASTNOSWIZZLE|SIF_WRITEXYZ },
  { XO_M4x4,    "m4x4",   2, 0x10,0x20, SIF_LASTNOSWIZZLE|SIF_NOWRITEMASK },
  { XO_MAD,     "mad",    3, 0x10,0x10, SIF_NONE },
  { XO_MAX,     "max",    2, 0x10,0x20, SIF_NONE },
  { XO_MIN,     "min",    2, 0x10,0x20, SIF_NONE },
  { XO_MOV,     "mov",    1, 0x10,0x10, SIF_NONE },
  { XO_MOVA,    "mova",   1, 0x10,0x00, SIF_DESTMUSTA0 },
  { XO_MUL,     "mul",    2, 0x10,0x10, SIF_NONE },
  { XO_NRM,     "nrm",    1, 0x20,0x20, SIF_LASTNOSWIZZLE },
  { XO_POW,     "pow",    2, 0x20,0x20, SIF_REPLICATE },
  { XO_RCP,     "rcp",    1, 0x10,0x20, SIF_REPLICATELAST },
  { XO_RSQ,     "rsq",    1, 0x10,0x20, SIF_REPLICATELAST },
  { XO_SGE,     "sge",    2, 0x10,0x00, SIF_NONE },
  { XO_SGN,     "sgn",    3, 0x20,0x00, SIF_SRC23DIFFER },
  { XO_SLT,     "slt",    2, 0x10,0x00, SIF_NONE },
  { XO_SUB,     "sub",    2, 0x10,0x10, SIF_NONE },
  { XO_TEX13,   "tex",    0, 0x00,0x10, SIF_NOWRITEMASK },
  { XO_TEXLD,   "texld",  2, 0x00,0x20, SIF_NOWRITEMASK|SIF_NOSWIZZLE|SIF_TEXLD20|SIF_DESTMUSTTEMP },
  { XO_TEXLDB,  "texldb", 2, 0x00,0x20, SIF_NOWRITEMASK|SIF_NOSWIZZLE|SIF_TEXLD20|SIF_DESTMUSTTEMP },
  { XO_TEXLDP,  "texldp", 2, 0x00,0x20, SIF_NOWRITEMASK|SIF_NOSWIZZLE|SIF_TEXLD20|SIF_DESTMUSTTEMP },
  { XO_EXT_FREE,"free",   0, 0x10,0x10, SIF_NOWRITEMASK|SIF_NOSWIZZLE|SIF_DESTMUSTTEMP },

  { 0 },
};

static const sChar *emptylist[] =
{
  0
};

static const sChar *vsusagesuffix[] =
{
  "_position",
  "_blendweight",
  "_blendindices",
  "_normal",
  "_psize",
  "_texcoord",
  "_tangent",
  "_binormal",
  "_tessfactor",
  "_positiont",
  "_color",
  "_fog",
  "_depth",
  0
};

static const sChar *pssamplersuffix[] =
{
  " ",
  " ",
  "_2d",
  "_cube",
  "_volume",
  0
};

static const sChar *vs2instmodifier[] =
{
  0,
};

static const sChar *ps1instmodifier[] =
{
  "_sat",
  0,
};

static const sChar *ps1instshift[] =
{
  " ",    
  "_x2",
  "_x4",
  " ",
  " ",
  " ",
  " ",
  " ",
  " ",
  " ",
  " ",
  " ",
  " ",
  " ",
  " ",
  "_d2",
  0,
};

static const sChar *ps1srcmodifier[] =
{
  " ",
  "_bias",
  "_bx2",
  0,
};

static const sChar *ps2instmodifier[] =
{
  "_sat",
  "_pp",
  "_centroid",
  0,
};

static const sChar *compareops[] =
{
  "==",
  "<",
  ">",
  " ",
  "!=",
  ">=",
  "<=",
  0
};

/****************************************************************************/

static sBool IsSeperator(const sChar *scan)
{
  return (*scan == 0 || 
    *scan == ' ' || *scan == '\n' || *scan == '\t' || *scan == '\r' ||
    scan[0] == '/' && (scan[1] == '/' || scan[1] == '*') ||
    *scan == '(' || *scan == ')' || *scan == '[' || *scan == ']' ||
    *scan == ',' ||
    *scan == '=' || *scan == '!' || *scan == '<' || *scan == '>');
}

void sShaderCompiler::WordScan()
{
  sChar *s = Word;

  *s = 0;
  SubScanPtr = Word;
  IsNewLine = sFALSE;

  for(;;)
  {
    // skip whitespace
    while(*ScanPtr==' ' || *ScanPtr=='\n' || *ScanPtr=='\t' || *ScanPtr=='\r')
    {
      if(*ScanPtr=='\n')
      {
        if(ErrorLine)
          return;

        Line++;
        IsNewLine = sTRUE;
      }
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
      {
        ErrorLineNum = Line;
        Error("Block comment not closed");
        return;
      }
    }
    else                                  // no comment
      break;
  }

  switch(*ScanPtr)
  {
  case 0:
    break;

  case '(': case ')':
  case '[': case ']':
  case ',':
    *s++ = *ScanPtr++;
    *s++ = 0;
    break;

  case '!': case '=':
  case '<': case '>':
    *s++ = *ScanPtr++;
    if(*ScanPtr == '=')
      *s++ = *ScanPtr++;
    *s++ = 0;
    break;

  default:
    while(!IsSeperator(ScanPtr))
    {
      *s++ = *ScanPtr++;
      if(s-Word >= MAXTOKEN)
      {
        ErrorLineNum = Line;
        Error("Token too long");
        break;
      }
    }

    *s++ = 0;
    break;
  }

  Scan();
}

sInt sShaderCompiler::Scan()
{
  sChar *s = TokValue;
  Token = TOK_EOW;

  // copy first char, advance if it's not 0
  *s++ = *SubScanPtr;
  if(!*SubScanPtr)
    return Token;
  else // recognize one-character tokens
  {
    sChar ch = *SubScanPtr++;
    *s = 0;

    if(ch == '-' || ch == '+')
    {
      Token = ch;
      return Token;
    }
    else if(!*SubScanPtr && ch == '!')
    {
      Token = ch;
      return Token;
    }
    else if(ch == '.' && *SubScanPtr == '.')
    {
      SubScanPtr++;
      Token = TOK_DOTDOT;
      return Token;
    }
  }

  // copy other characters up to next . or _
  while(*SubScanPtr && *SubScanPtr != '.' && *SubScanPtr != '_'
    && *SubScanPtr != '-' && *SubScanPtr != '+')
    *s++ = *SubScanPtr++;

  *s++ = 0;
  Token = TOK_WORD;

  // try and recognize known word
  for(sInt i=0;Tokens[i];i++)
  {
    if(sCmpString(TokValue,Tokens[i]) == 0)
    {
      Token = TOK_FIRSTPREDEF + i;
      break;
    }
  }

  return Token;
}

void sShaderCompiler::SkipRestOfLine()
{
  do
  {
    WordScan();
  }
  while(!IsNewLine && *ScanPtr);
}

void sShaderCompiler::ExpectEndOfLine()
{
  if(!IsNewLine && *ScanPtr)
  {
    Error("Extra characters on line!");
    ErrorLine = 0;
    SkipRestOfLine();
  }
}

void sShaderCompiler::ExpectEndOfWord()
{
  if(Token != TOK_EOW)
    Error("Extra characters in word!");

  WordScan();
}

void sShaderCompiler::ExpectEndOfWordAndLine()
{
  if(Token != TOK_EOW)
    Error("Extra characters in word!");

  ErrorLine = 0;
  WordScan();
  
  if(!IsNewLine && *ScanPtr)
  {
    Error("Extra characters on line!");
    SkipRestOfLine();
  }
}

void sShaderCompiler::Error(const sChar *name,...)
{
  if(ErrorLine==0)
  {
    ErrorCount++;
    Errors->PrintF("%s(%d): ",FileName,ErrorLineNum);
    Errors->PrintArg(name,&name);
    Errors->Print("\n");
  }
  ErrorLine++;
}

sBool sShaderCompiler::ExpectWord(const sChar *word)
{
  if(sCmpString(Word,word))
  {
    Error("Syntax error: '%s' expected",word);
    WordScan();
    return sFALSE;
  }
  
  WordScan();
  return sTRUE;
}

sBool sShaderCompiler::ExpectSubWord(const sChar *subWord)
{
  if(sCmpString(TokValue,subWord))
  {
    Error("Syntax error: '%s' expected",subWord);
    Scan();
    return sFALSE;
  }
  
  Scan();
  return sTRUE;
}

void sShaderCompiler::VerifyToken(sInt token)
{
  if(Token != token)
    Error("Internal parse error");

  Scan();
}

/****************************************************************************/

struct sShaderCompiler::Symbol
{
  sChar Name[MAXTOKEN+1];
  sU32 Register;
  sInt FieldOffset;
  sInt FieldDimension; // 0 = single value
};

class sShaderCompiler::SymbolTable
{
  sArray<Symbol> Table;
  sShaderCompiler *Compiler;

  Symbol *LookupInternal(const sChar *name);
  void AddInternal(const sChar *name,sU32 reg,sInt offs,sInt dim);

public:
  SymbolTable *ParentScope;

  SymbolTable(sShaderCompiler *compiler,SymbolTable *parent = 0);
  ~SymbolTable();

  void AddReg(const sChar *name,sU32 reg);
  void AddField(const sChar *name,sInt offs,sInt dim);

  Symbol *Lookup(const sChar *name);
};

sShaderCompiler::Symbol *sShaderCompiler::SymbolTable::LookupInternal(const sChar *name)
{
  for(sInt i=0;i<Table.Count;i++)
    if(!sCmpString(Table[i].Name,name))
      return &Table[i];

  return 0;
}

void sShaderCompiler::SymbolTable::AddInternal(const sChar *name,sU32 reg,sInt offs,sInt dim)
{
  Symbol *sym = LookupInternal(name);

  if(sym)
    Compiler->Error("Symbol '%s' already defined in this scope",name);
  else
  {
    sym = Table.Add();
    sCopyString(sym->Name,name,MAXTOKEN+1);
    sym->Register = reg;
    sym->FieldOffset = offs;
    sym->FieldDimension = dim;
  }
}

sShaderCompiler::SymbolTable::SymbolTable(sShaderCompiler *compiler,sShaderCompiler::SymbolTable *parent)
{
  Table.Init();
  Compiler = compiler;
  ParentScope = parent;
}

sShaderCompiler::SymbolTable::~SymbolTable()
{
  Table.Exit();
}

void sShaderCompiler::SymbolTable::AddReg(const sChar *name,sU32 reg)
{
  AddInternal(name,reg,0,-1);
}

void sShaderCompiler::SymbolTable::AddField(const sChar *name,sInt offs,sInt dim)
{
  AddInternal(name,~0U,offs,dim);
}

sShaderCompiler::Symbol *sShaderCompiler::SymbolTable::Lookup(const sChar *name)
{
  for(SymbolTable *table=this;table;table=table->ParentScope)
  {
    Symbol *sym = table->LookupInternal(name);
    if(sym)
      return sym;
  }

  return 0;
}

/****************************************************************************/

void sShaderCompiler::OpenScope()
{
  Identifiers = new SymbolTable(this,Identifiers);
}

void sShaderCompiler::EndScope()
{
  if(Identifiers->ParentScope)
  {
    SymbolTable *old = Identifiers;
    Identifiers = Identifiers->ParentScope;
    delete old;
  }
}

sU32 sShaderCompiler::FindSymRegister(const sChar *name)
{
  Symbol *sym = Identifiers->Lookup(name);

  if(!sym)
  {
    Error("'%s': Identifier must be declared before use",name);
    return ~0;
  }
  else
    return sym->Register;
}

void sShaderCompiler::AddTempRegister(const sChar *name)
{
  if(NumTempRegs >= 256)
    Error("Only 256 temporaries supported at the moment");
  else
    Identifiers->AddReg(name,NumTempRegs++);
}

void sShaderCompiler::AddPredefinedSymbols()
{
  if(ShaderType == 0) // vertex shader
  {
    Identifiers->AddReg("oPos",X_OPOS);
    Identifiers->AddReg("oFog",X_OFOG);
    Identifiers->AddReg("oPts",X_PSIZE);
    Identifiers->AddReg("oD0",X_OCOLOR|0);
    Identifiers->AddReg("oD1",X_OCOLOR|1);
    Identifiers->AddReg("oT0",X_OUV|0);
    Identifiers->AddReg("oT1",X_OUV|1);
    Identifiers->AddReg("oT2",X_OUV|2);
    Identifiers->AddReg("oT3",X_OUV|3);
    Identifiers->AddReg("oT4",X_OUV|4);
    Identifiers->AddReg("oT5",X_OUV|5);
    Identifiers->AddReg("oT6",X_OUV|6);
    Identifiers->AddReg("oT7",X_OUV|7);
  }
  else // pixel shader
  {
    Identifiers->AddReg("oC0",X_COLOR|0);
    Identifiers->AddReg("oC1",X_COLOR|1);
    Identifiers->AddReg("oC2",X_COLOR|2);
    Identifiers->AddReg("oC3",X_COLOR|3);
    Identifiers->AddReg("oDepth",X_DEPTH);
  }
}

/****************************************************************************/

void sShaderCompiler::AddFlag(const sChar *name,sInt dim)
{
  Flags->AddField(name,NumFlags,dim);
  NumFlags += dim ? dim : 1;

  if(NumFlags >= 496)
    Error("You may not use more than 496 words of flags");
}

/****************************************************************************/

struct sShaderCompiler::Control
{
  sInt Type;
  sInt Param;
};

void sShaderCompiler::PushControl(sInt type,sInt param)
{
  if(ControlStack.Count >= 32)
    Error("Control structures nested too deeply! (Max. 32 levels)");

  Control *ctrl = ControlStack.Add();
  ctrl->Type = type;
  ctrl->Param = param;

  OpenScope();
}

void sShaderCompiler::PopControl(sInt type)
{
  if(!ControlStack.Count)
    Error("'end%s' without %s",controlNames[type],controlNames[type]);
  else
  {
    sInt refType = TopControl()->Type;

    if(type != refType)
      Error("'end%s' read, 'end%s' expected",controlNames[type],controlNames[refType]);

    EndScope();
    ControlStack.Count--;
  }
}

sShaderCompiler::Control *sShaderCompiler::TopControl()
{
  static Control defaultControl = { 0 };

  if(!ControlStack.Count)
    return &defaultControl;
  else
    return &ControlStack[ControlStack.Count-1];
}

void sShaderCompiler::ElseControl(sBool set)
{
  if(!ControlStack.Count)
    Error("else without if");
  else
  {
    Control *top = TopControl();

    if(top->Type != CS_IF)
      Error("'%s' has no 'else'",controlNames[top->Type]);
    else if(top->Param)
      Error("More than one else per if isn't allowed");
    else
    {
      EndScope();
      OpenScope();

      if(set)
        top->Param = 1;
    }
  }
}

/****************************************************************************/

struct sShaderCompiler::IndexedPatch
{
  sInt Location;
  sInt Reg;
};

void sShaderCompiler::Emit(sU32 token)
{
  *Output.Add() = token;
}

void sShaderCompiler::EmitIndexBackpatches()
{
  sInt baseOffs = Output.Count;

  for(sInt i=0;i<IndexedPatches.Count;i++)
  {
    IndexedPatch *patch = &IndexedPatches[i];

    if(baseOffs - patch->Location >= 256)
      Error("Code generation error: Indexed backpatch offset exceeds 255");

    Emit(XO_EXT_INDEXED);
    Emit((baseOffs - patch->Location) | (patch->Reg << 8));
  }

  IndexedPatches.Count = 0;
}

/****************************************************************************/

sInt sShaderCompiler::CharInList(sChar ch,const sChar *list)
{
  sInt i;
  
  for(i=0;list[i] && list[i] != ch;i++);
  return list[i] ? i : -1;
}

sInt sShaderCompiler::StringInPrefixList(const sChar *str,const sChar **prefixList,const sChar *&rest)
{
  for(sInt i=0;prefixList[i];i++)
  {
    const sChar *prefix = prefixList[i];
    sInt j;

    for(j=0;prefix[j] && prefix[j] == str[j];j++);
    if(!prefix[j])
    {
      rest = str + j;
      return i;
    }
  }

  return -1;
}

sInt sShaderCompiler::MatchStringList(const sChar **stringList)
{
  for(sInt i=0;stringList[i];i++)
  {
    if(!sCmpString(stringList[i],TokValue))
      return i;
  }

  return -1;
}

sInt sShaderCompiler::DecodeInt(const sChar *what)
{
  sInt number = 0;

  while(*what)
  {
    if(*what >= '0' && *what <= '9')
      number = (number * 10) + (*what++ - '0');
    else
      return -1;
  }

  return number;
}

/****************************************************************************/

sF32 sShaderCompiler::ParseFloat()
{
  const sChar *scan = Word;
  sF64 val,dec,frac,neg;

  val = 0;
  dec = 1;
  frac = 0;
  neg = 1;

  if((*scan >= '0' && *scan <= '9') || *scan == '.' || *scan == '-')
  {
    if(*scan == '-')
    {
      neg = -1;
      scan++;
    }

    while(*scan >= '0' && *scan <= '9')
      val = val * 10 + (*scan++ - '0');

    if(*scan == '.')
    {
      scan++;

      while(*scan >= '0' && *scan <= '9')
      {
        frac = frac * 10 + (*scan++ - '0');
        dec *= 10;
      }

      val += frac / dec;
    }

    if(*scan != 0)
    {
      val = 0.0f;
      Error("Not a valid float value");
    }
  }
  else
    Error("Not a valid float value");

  WordScan();
  return val * neg;
}

sInt sShaderCompiler::ParseWriteMask()
{
  sInt mask = 0;

  if(Token == TOK_WORD && TokValue[0] == '.')
  {
    static const sChar *maskChars[] = { "xyzw", "rgba" };
    sInt maskType = -1;
    const sChar *namePtr = TokValue + 1;

    while(*namePtr)
    {
      if(maskType == -1)
      {
        if(CharInList(*namePtr,maskChars[0]) != -1)
          maskType = 0;
        else if(CharInList(*namePtr,maskChars[1]) != -1)
          maskType = 1;
        else
          Error("Invalid character in write mask");
      }

      sInt ind = CharInList(*namePtr,maskChars[maskType]);
      if(ind == -1)
        Error("Invalid character in write mask");
      else if(mask & (1 << ind))
        Error("Same channel specified twice in write mask (%c)",*namePtr);

      mask |= 1 << ind;
      namePtr++;
    }
  }

  Scan();

  if(mask == 0)
    Error("Invalid write mask");

  return mask;
}

sU32 sShaderCompiler::ParseSwizzle()
{
  if(Token != TOK_WORD || TokValue[0] != '.') // no swizzle
    return XS_XYZW;
  else
  {
    const sChar *str = TokValue + 1; // skip '.'

    static const sChar *maskChars[] = { "xyzw", "rgba" };
    sU32 mask = 0;
    sInt lastOne=0,numComponents=0;
    sInt maskType = -1;

    // swizzle (hopefully) follows
    while(*str)
    {
      if(maskType == -1)
      {
        if(CharInList(*str,maskChars[0]) != -1)
          maskType = 0;
        else if(CharInList(*str,maskChars[1]) != -1)
          maskType = 1;
        else
          Error("Invalid character in swizzle mask");
      }

      lastOne = CharInList(*str,maskChars[maskType]);
      if(lastOne == -1)
        Error("Invalid character in swizzle mask");
      else
        mask |= lastOne << (16 + 2*numComponents);

      str++;
      if(++numComponents > 4)
        Error("Swizzle mask too long (4 components only)");
    }

    while(numComponents < 4)
    {
      mask |= lastOne << (16 + 2*numComponents);
      numComponents++;
    }

    Scan();
    return mask;
  }
}

sU32 sShaderCompiler::ParseRegNum()
{
  sU32 regNum = 0;

  if(Token != TOK_WORD)
    Error("'%s': Register name expected",TokValue);
  else
  {
    // check whether register is directly addressed
    const sChar *regs[] = { " vca", " vct      s" };
    sInt regType = CharInList(TokValue[0],regs[ShaderType]);
    sInt index = DecodeInt(TokValue+1);

    if(regType != -1 && index != -1) // yes, directly addressed
      regNum = ((regType & 7) << 28) | ((regType & 24) << 8) | index;
    else // no, look up name in symbol table
      regNum = FindSymRegister(TokValue);
  }

  Scan();
  if(Token == '+') // indexed access?
  {
    Scan();
    sInt index = ParseIndexReg();
    if(index < 0)
      Error("Index register expected");
    else if(index >= 15)
      Error("Only 15 index registers supported");
    else
    {
      IndexedPatch *patch = IndexedPatches.Add();
      patch->Location = Output.Count;
      patch->Reg = index;
    }
  }

  return regNum;
}

sU32 sShaderCompiler::ParseFlagIdentifier(sBool expectIt)
{
  sU32 code;

  Symbol *sym = Flags->Lookup(TokValue);

  if(sym)
  {
    WordScan();

    code = 0x1e000000 | (sym->FieldOffset << 16);
    if(sym->FieldDimension) // for arrays, we now need an index
    {
      if(!ExpectWord("["))
        return ~0U;

      sInt indexNum = ParseIndexReg();

      if(indexNum >= 0 && Token == TOK_EOW) // index register
      {
        if(indexNum >= 15)
          Error("Only 15 index registers supported");

        code = (code & ~0x1e000000) | (indexNum << 25);
      }
      else
      {
        sInt index = DecodeInt(Word);
        if(index < 0)
          Error("Array index expected");
        else if(index >= sym->FieldDimension)
          Error("Array index out of range");

        code += index << 16;
      }

      WordScan();
      if(!ExpectWord("]"))
        return ~0U;
    }
  }
  else
  {
    if(expectIt)
    {
      WordScan();
      Error("Unknown flag identifier '%s'",Word);
    }

    return ~0U;
  }

  return code;
}

/****************************************************************************/

sU32 sShaderCompiler::ParseDestReg(sBool writeMask)
{
  sU32 out = 0;
  sInt mask = 0xf;
  sU32 regNum = ParseRegNum();

  // Parse write mask if desired
  if(Token == TOK_WORD && TokValue[0] == '.')
  {
    if(writeMask)
      mask = ParseWriteMask();
    else
      Error("No write mask allowed");
  }

  // Emit register specifier
  out = 0x80000000 | (mask << 16) | regNum | DestModifier;
  DestModifier = 0;

  Emit(out);
  ExpectEndOfWord();

  return out;
}

sU32 sShaderCompiler::ParseSourceReg()
{
  sU32 out = 0;

  // complement modifier?
  if(ShaderType && ShaderVer < 0x20 && Token == TOK_WORD && TokValue[0] == '1' && !TokValue[1])
  {
    Scan();
    ExpectSubWord("-");
    
    out |= XS_COMP;
  }

  // negate modifier?
  if(Token == '-')
  {
    if(out & XS_COMP)
      Error("Negate and complement modifiers cannot be combined");

    out |= XS_NEG;
    Scan();
  }

  out |= ParseRegNum();   // register number
  out |= ParseSwizzle();  // swizzle
  out |= 0x80000000;      // tag bit

  // source modifiers
  while(Token != TOK_EOW)
  {
    const sChar **list;

    if(ShaderType)
      list = (ShaderVer >= 0x20) ? emptylist : ps1srcmodifier;
    else
      list = emptylist;

    sInt mod = MatchStringList(list);
    if(mod != -1)
    {
      if(out & XS_COMP)
        Error("Complement modifier cannot be combined with other modifiers");

      out |= mod << 25;
    }
    else
      Error("Unknown source modifier '%s'",TokValue);

    Scan();
  }

  Emit(out);
  ExpectEndOfWord();

  return out;
}

sInt sShaderCompiler::ParseIndexReg()
{
  sInt num = DecodeInt(TokValue+1);

  if(TokValue[0] == 'i' && num >= 0)
  {
    Scan();
    return num;
  }
  else
    return -1;
}

void sShaderCompiler::ParseRange(sInt &start,sInt &end)
{
  if(Token == TOK_WORD)
  {
    start = DecodeInt(TokValue);
    Scan();

    if(start >= 0 && (Token == TOK_EOW || Token == TOK_DOTDOT))
    {
      end = start;

      if(Token == TOK_DOTDOT)
      {
        Scan();
        end = DecodeInt(TokValue);
        Scan();

        if(end < start || Token != TOK_EOW)
          Error("Illegal range");
      }
    }
    else
      Error("Single number or range expected");
  }
  else
    Error("Single number or range expected");

  WordScan();
}

sU32 sShaderCompiler::ParseIfCondition()
{
  sBool negate = sFALSE;
  sU32 code;

  // remove all negate prefixes
  while(Token == '!')
  {
    negate = !negate;
    WordScan();
  }

  // now, there should either be a flag identifier or an index register specification
  if(Token == TOK_WORD)
  {
    sInt indexNum = ParseIndexReg();

    if(indexNum >= 0) // index register
    {
      if(indexNum >= 15)
        Error("Only 15 index registers supported");

      code = 0x1e0000e0 | (((indexNum >> 2) + 496) << 16) | ((indexNum & 3) << 3);
      WordScan();
    }
    else
    {
      code = ParseFlagIdentifier(sTRUE);
      if(code != ~0U)
      {
        // now, the bitfield specifier
        if(!ExpectWord("["))
          return 0;

        sInt bitStart,bitEnd;
        ParseRange(bitStart,bitEnd);

        if(bitEnd > 31)
          Error("Invalid bitfield end point! (Max 31)");
        else if(bitEnd - bitStart >= 8)
          Error("Invalid bitfield width! (Max 8 bit)");

        code |= bitStart | ((bitEnd - bitStart) << 5);

        if(!ExpectWord("]"))
          return 0;
      }
      else
        return 0;
    }
  }
  else
    Error("Identifier expected");

  // Next, expect either a comparision operator or end of line
  if(!*Word || IsNewLine) // end of line
  {
    negate = !negate; // we get != 0 as test
  }
  else
  {
    sInt cond = MatchStringList(compareops);
    Scan();

    if(cond == -1 || Token != TOK_EOW)
      Error("Invalid comparision operator!");

    code |= (cond & 3) << 29;
    if(cond & 4)
      negate = !negate;

    WordScan();

    sInt value = DecodeInt(Word);
    if(value < 0 || value > 255)
      Error("'%s': Integer in the range 0-255 expected",Word);

    code |= value << 8;
    WordScan();
  }

  if(negate)
    code ^= 0x80000000;

  Emit(code);
  return code;
}

/****************************************************************************/

void sShaderCompiler::ParseVersion()
{
  ErrorLineNum = Line;

  if(sCmpString(TokValue,"vs") && sCmpString(TokValue,"ps"))
    Error("Shader must start with version directive");
  else
  {
    ShaderType = TokValue[0] == 'p';
    ShaderVer = 0;
    Scan();

    sInt MajorVer = TokValue[0] == '.' ? DecodeInt(TokValue+1) : -1;
    Scan();
    sInt MinorVer = TokValue[0] == '.' ? DecodeInt(TokValue+1) : -1;
    Scan();

    if(MajorVer == -1 || MinorVer == -1)
      Error("Invalid syntax for version directive");
    else
      ShaderVer = (MajorVer << 4) | MinorVer;

    if(ShaderVer != 0x20 && ShaderVer != 0x11)
      Error("Sorry, only 2.0 shaders and 1.1 vertex shaders supported at the moment");

    Emit(((0xfffe + ShaderType) << 16) | (MajorVer << 8) | MinorVer);

    ExpectEndOfWordAndLine();
  }
}

void sShaderCompiler::ParseDcl()
{
  VerifyToken(TOK_DCL);
  Emit(XO_DCL);

  if(ShaderType == 0) // vertex
  {
    const sChar *rest;
    sInt usage = StringInPrefixList(TokValue,vsusagesuffix,rest);
    if(usage == -1)
    {
      Error("Unknown usage type");
      return;
    }

    // determine usage index
    sInt index = DecodeInt(rest);
    if(index < 0 || index > 15)
      Error("Invalid usage index");

    Scan();

    Emit(0x80000000 | usage | (index << 16));
    ExpectEndOfWord();

    // determine dest register
    sU32 regType = ParseDestReg(sFALSE) & 0xf0001800;
    if(regType != X_V)
      Error("Destination register needs to be an input register");
  }
  else if(ShaderType == 1) // pixel
  {
    if(Token != TOK_EOW) // sampler declaration
    {
      sInt samplerId = MatchStringList(pssamplersuffix);
      Scan();

      if(samplerId == -1)
        Error("Invalid sampler specification");
      else
      {
        Emit(0x80000000 | (samplerId << 27));
        ExpectEndOfWord();

        // determine destination sampler register
        sU32 regType = ParseDestReg(sFALSE) & 0xf0001800;
        if(regType != X_S)
          Error("Destination register needs to be a sampler register");
      }
    }
    else // register
    {
      Emit(XD_REG);
      WordScan();

      sU32 regType = ParseDestReg(sTRUE) & 0xf0001800;
      if(regType != X_V && regType != X_T)
        Error("Destination register needs to be a color or texture register");
    }
  }
}

void sShaderCompiler::ParseDef()
{
  VerifyToken(TOK_DEF);
  Emit(XO_DEF);
  ExpectEndOfWord();

  // destination register
  sU32 dstreg = ParseDestReg(sFALSE);
  if((dstreg & 0xf0000000) != X_C)
    Error("Destination register needs to be a constant register for def");

  // parameters
  for(sInt i=0;i<4;i++)
  {
    ExpectWord(",");
    sF32 value = ParseFloat();
    Emit(*(sU32 *) &value);
  }
}

void sShaderCompiler::ParseTemp()
{
  VerifyToken(TOK_TEMP);
  ExpectEndOfWord();

  sBool first = sTRUE;
  
  // read variable definitions till end of line
  while(!IsNewLine)
  {
    if(ErrorLine)
      return;

    if(!first)
      ExpectWord(",");

    if(Token == TOK_WORD)
      AddTempRegister(TokValue);
    else
      Error("Invalid name for temporary: '%s'",Word);

    Scan();
    if(Token != TOK_EOW)
      Error("Invalid name for temporary: '%s'",Word);

    WordScan();
    first = sFALSE;
  }
}

void sShaderCompiler::ParseAlias()
{
  VerifyToken(TOK_ALIAS);
  ExpectEndOfWord();

  sBool first = sTRUE;

  // read alias definitions till end of line
  while(!IsNewLine)
  {
    if(ErrorLine)
      return;

    if(!first)
      ExpectWord(",");

    if(Token == TOK_WORD)
    {
      sChar aliasName[MAXTOKEN+1];
      sCopyString(aliasName,TokValue,sizeof(aliasName));
      Scan();
      if(Token != TOK_EOW)
        Error("Invalid name for alias: '%s'",Word);
      
      WordScan();
      ExpectWord("=");

      if(Token == TOK_WORD)
      {
        sU32 srcReg = ParseRegNum();
        if(srcReg != ~0U)
          Identifiers->AddReg(aliasName,srcReg);

        Scan();
        if(Token != TOK_EOW)
          Error("Invalid name for alias target. '%s'",Word);
      }
      else
        Error("Aliases must be of the form newreg = oldreg");
    }
    else
      Error("Aliases must be of the form newreg = oldreg");

    WordScan();
    first = sFALSE;
  }
}

void sShaderCompiler::ParseFor()
{
  VerifyToken(TOK_FOR);
  ExpectEndOfWord();

  sInt indexNum = DecodeInt(Word+1);
  if(Word[0] == 'i' && indexNum >= 0)
  {
    if(indexNum >= 15)
      Error("Only 15 index registers supported");

    WordScan();
    ExpectWord("=");

    sInt start,end;
    ParseRange(start,end);

    if(end > 254)
      Error("For end point must be <=254.");

    Emit(XO_EXT_IADD);
    Emit(0xf000 | (indexNum << 8) | start);
    Emit(XO_EXT_WHILE);
    Emit(0xde0000e0 | (((indexNum >> 2) + 496) << 16) | (end << 8)
      | ((indexNum & 3) << 3));

    PushControl(CS_FOR,indexNum);
  }
  else
    Error("Index register expected");
}

static sBool HasReplicateSwizzle(sU32 reg)
{
  reg &= 0x00ff0000; // use swizzle only
  return reg == XS_X || reg == XS_Y || reg == XS_Z || reg == XS_W;
}

void sShaderCompiler::ParseArith()
{
  const ShaderInstr *instr;
  sU32 opModifier = 0;

  if(Token == '+' && ShaderType && ShaderVer < 0x20) // coissue
  {
    opModifier |= XO_CO;
    Scan();
  }

  for(instr=ShaderInstrs;instr->name;instr++)
  {
    if(sCmpString(TokValue,instr->name) == 0)
      break;
  }

  if(!instr->name) // instruction not recognized
  {
    Error("Instruction expected");
    return;
  }

  // version check, emit instr
  if((ShaderType == 0 && instr->vsver > ShaderVer)
    || (ShaderType == 1 && instr->psver > ShaderVer))
    Error("Instruction '%s' not supported in this shader version!",instr->name);

  Emit(instr->opcode | opModifier);
  Scan();

  // parse (optional) instruction modifiers
  while(Token != TOK_EOW)
  {
    const sChar **list;
    
    if(ShaderType)
      list = (ShaderVer >= 0x20) ? ps2instmodifier : ps1instmodifier;
    else
      list = vs2instmodifier;

    sInt mod = MatchStringList(list);
    if(mod != -1)
    {
      sU32 modmask = 1 << (mod + 20);
      if(DestModifier & modmask)
        Error("Instruction modifier '%s' already specified",TokValue);
      else
        DestModifier |= modmask;
    }
    else if(ShaderType && ShaderVer < 0x20)
    {
      sInt shift = MatchStringList(ps1instshift);
      if(shift != -1)
      {
        if(DestModifier & 0x0f000000)
          Error("A shift modifier has already been specified");
        else
          DestModifier |= shift << 24;
      }
      else
        Error("Unknown instruction modifier '%s'",TokValue);
    }
    else
      Error("Unknown instruction modifier '%s'",TokValue);

    Scan();
  }

  WordScan();

  // destination register
  sU32 dstreg = ParseDestReg(sTRUE);

  if((instr->flags & SIF_DESTMUSTTEMP) && (dstreg & 0x70000000) != 0)
    Error("Destination register must be a temporary register for '%s'",instr->name);
  if((instr->flags & SIF_DESTNOW) && (dstreg & 0x80000))
    Error("Destination register may not include .w in write mask for '%s'",instr->name);
  if((instr->flags & SIF_NOWRITEMASK) && (dstreg & 0xf0000) != 0xf0000)
    Error("Destination register needs to have .xyzw write mask for '%s'",instr->name);
  if((instr->flags & SIF_WRITEXY) && (dstreg & 0xf0000) != 0x30000)
    Error("Destination register needs to have .xy write mask for '%s'",instr->name);
  if((instr->flags & SIF_WRITEXYZ) && (dstreg & 0xf0000) != 0x70000)
    Error("Destination register needs to have .xy write mask for '%s'",instr->name);
  if((instr->flags & SIF_DESTMUSTA0) && (dstreg & 0xf0001fff) != X_T)
    Error("Destination register needs to be a0 for '%s'",instr->name);

  sU32 srcreg = 0;

  // source registers
  for(sInt i=0;i<instr->params;i++)
  {
    ExpectWord(",");

    sU32 oldsrcreg = srcreg;
    srcreg = ParseSourceReg();
    
    if((instr->flags & SIF_NOSWIZZLE) && (srcreg & 0x00ff0000) != XS_XYZW)
      Error("'%s' doesn't allow source swizzle",instr->name);
    if((instr->flags & SIF_NOSRCDESTEQUAL) && (srcreg & 0x70001fff) == (dstreg & 0x70001fff) && ShaderVer < 0x20)
      Error("'%s' requires source registers to be different from dest registers",instr->name);
    if((instr->flags & SIF_REPLICATE) && !HasReplicateSwizzle(srcreg))
      Error("'%s' requires source registers to have replicate swizzle",instr->name);
    if(instr->flags & SIF_TEXLD20)
    {
      sU32 regType = srcreg & 0xf0001800;

      if(i == 0 && regType != X_T && regType != X_R)
        Error("'%s' requires first source register to be either a temporary or texture register",instr->name);
      else if(i == 1 && regType != X_S)
        Error("'%s' requires second source register to be a sampler register",instr->name);
    }
    if(i == 0 && (instr->flags & SIF_SRC23DIFFER) && (oldsrcreg & 0x70001fff) == (srcreg & 0x70001fff))
      Error("'%s' requires second and third source register to be different",instr->name);
  }

  if(instr->flags & SIF_REPLICATELAST && !HasReplicateSwizzle(srcreg))
    Error("'%s' requires last parameter to have replicate swizzle",instr->name);
  if ((instr->flags & SIF_LASTNOSWIZZLE) && (srcreg & 0x00ff0000) != XS_XYZW)
    Error("'%s' doesn't allow last parameter to have swizzle",instr->name);
}

sBool sShaderCompiler::ParseIndexInstr()
{
  if(!sCmpString(TokValue,"imov"))
  {
    Emit(XO_EXT_IADD);
    Scan();
    ExpectEndOfWord();

    sInt dest = ParseIndexReg();
    if(dest < 0)
      Error("Index register expected");

    if(dest >= 15)
      Error("Only 15 index registers supported");

    WordScan();
    ExpectWord(",");

    sInt src = ParseIndexReg();
    if(src >= 0) // register source?
    {
      ExpectEndOfWord();

      if(src >= 15)
        Error("Only 15 index registers supported");

      Emit((dest << 8) | (src << 12));
    }
    else // assume constant
    {
      sInt value = DecodeInt(Word);
      WordScan();

      if(value < 0)
        Error("Invalid integer value");
      else if(value > 255)
        Error("Integer value out of range");

      Emit(0xf000 | (dest << 8) | value);
    }
  }
  else if(!sCmpString(TokValue,"iadd"))
  {
    Emit(XO_EXT_IADD);
    Scan();
    ExpectEndOfWord();

    sInt dest = ParseIndexReg();
    if(dest < 0)
      Error("Index register expected");

    if(dest >= 15)
      Error("Only 15 index registers supported");

    WordScan();
    ExpectWord(",");

    sInt src = ParseIndexReg();
    if(src >= 0) // register source
    {
      if(src >= 15)
        Error("Only 15 index registers supported");

      WordScan();
      ExpectWord(",");
    }
    else
      src = dest;

    sInt value = DecodeInt(Word);
    WordScan();

    if(value < 0)
      Error("Invalid integer value");
    else if(value > 255)
      Error("Integer value out of range");

    Emit((src << 12) | (dest << 8) | value);
  }
  else
    return sFALSE;

  return sTRUE;
}

void sShaderCompiler::ParseVMov()
{
  VerifyToken(TOK_VMOV);
  ExpectEndOfWord();

  Emit(XO_EXT_VMOV);

  // destination register
  sU32 dstreg = ParseDestReg(sTRUE);
  if(dstreg & 0x70000000)
    Error("Destination register must be a temporary register for 'vmov'");
  else if((dstreg & 0xf0000) != 0xf0000)
    Error("Destination register needs to have .xyzw write mask for 'vmov'");

  // source operand
  ExpectWord(",");
  sU32 flag = ParseFlagIdentifier(sFALSE);
  if(flag != ~0U) // flag? emit it
    Emit(flag);
  else // assume it's a register
    ParseSourceReg();
}

/****************************************************************************/

void sShaderCompiler::ParseFlags()
{
  while(*ScanPtr && Token != TOK_ENDFLAGS)
  {
    ErrorLine = 0;
    ErrorLineNum = Line;

    if(Token != TOK_WORD)
      Error("Identifier expected!");
    else
    {
      sChar name[MAXTOKEN+1];
      sCopyString(name,TokValue,sizeof(name));
      Scan();

      ExpectEndOfWord();

      if(!sCmpString(Word,"[")) // array declaration?
      {
        ExpectWord("[");
        sInt dim = DecodeInt(Word);
        WordScan();

        if(dim == -1)
          Error("Array dimension expected!");
        else if(dim == 0)
          Error("Invalid array dimension!");

        AddFlag(name,dim);
        ExpectWord("]");
      }
      else
        AddFlag(name,0);
    }

    if(!ErrorLine)
      ExpectEndOfLine();
    else
    {
      ErrorLine = 0;
      SkipRestOfLine();
    }
  }

  if(!*ScanPtr)
    Error("'flags' without terminating 'endflags'");
  else
  {
    WordScan();
    ExpectEndOfLine();
  }
}

void sShaderCompiler::ParseCode()
{
  while(*ScanPtr)
  {
    ErrorLine = 0;
    ErrorLineNum = Line;

    switch(Token)
    {
    case TOK_DCL:
      ParseDcl();
      break;

    case TOK_DEF:
      ParseDef();
      break;

    case TOK_NOP:
      VerifyToken(TOK_NOP);
      Emit(XO_NOP);
      ExpectEndOfWordAndLine();
      break;

    case TOK_TEXKILL:
      {
        VerifyToken(TOK_TEXKILL);
        if(ShaderType != 1)
          Error("Instruction 'texkill' not supported in this shader version!");
        
        Emit(XO_TEXKILL);
        ExpectEndOfWord();

        sU32 regType = ParseSourceReg() & 0xf0001800;
        if(regType != X_R && regType != X_T)
          Error("'texkill' requires source register to be either a temporary or texture register");
      }
      break;

    case TOK_TEMP:
      ParseTemp();
      break;

    case TOK_ALIAS:
      ParseAlias();
      break;

    case TOK_IF:
      VerifyToken(TOK_IF);
      ExpectEndOfWord();
      Emit(XO_EXT_IF);
      ParseIfCondition();

      PushControl(CS_IF,0);
      break;

    case TOK_ELSE:
      VerifyToken(TOK_ELSE);
      ExpectEndOfWord();
      Emit(XO_EXT_ELSE);

      ElseControl(sTRUE);
      break;

    case TOK_ELIF:
      VerifyToken(TOK_ELIF);
      ExpectEndOfWord();
      Emit(XO_EXT_ELIF);
      ParseIfCondition();

      ElseControl(sFALSE);
      break;

    case TOK_ENDIF:
      VerifyToken(TOK_ENDIF);
      ExpectEndOfWord();
      Emit(XO_EXT_END);

      PopControl(CS_IF);
      break;

    case TOK_FOR:
      ParseFor();
      break;

    case TOK_ENDFOR:
      VerifyToken(TOK_ENDFOR);
      ExpectEndOfWord();

      Emit(XO_EXT_IADD);
      Emit((TopControl()->Param << 8) | 0x01);
      Emit(XO_EXT_END);

      PopControl(CS_FOR);
      break;

    case TOK_WHILE:
      VerifyToken(TOK_WHILE);
      ExpectEndOfWord();
      Emit(XO_EXT_WHILE);
      ParseIfCondition();

      PushControl(CS_WHILE,0);
      break;

    case TOK_ENDWHILE:
      VerifyToken(TOK_ENDWHILE);
      ExpectEndOfWord();
      Emit(XO_EXT_END);

      PopControl(CS_WHILE);
      break;

    case TOK_FLAGS:
      VerifyToken(TOK_FLAGS);
      ExpectEndOfWord();
      ExpectEndOfLine();

      ParseFlags();
      break;

    case TOK_ERROR:
      VerifyToken(TOK_ERROR);
      ExpectEndOfWord();
      Emit(XO_EXT_ERROR);
      break;

    case TOK_VMOV:
      ParseVMov();
      break;

    default:
      if(!ParseIndexInstr())
        ParseArith();
      break;
    }

    if(!ErrorLine)
    {
      EmitIndexBackpatches();
      ExpectEndOfLine();
    }
    else
    {
      ErrorLine = 0;
      SkipRestOfLine();
    }
  }
}

/****************************************************************************/

sShaderCompiler::sShaderCompiler()
{
  Identifiers = 0;
  Flags = 0;
  ControlStack.Init();
  IndexedPatches.Init();

  Output.Init();
}

sShaderCompiler::~sShaderCompiler()
{
  ControlStack.Exit();
  IndexedPatches.Exit();
  Output.Exit();
}

sBool sShaderCompiler::Compile(const sChar *source,const sChar *fileName,sText *errors)
{
  if(!source)
    source = "";

  Line = 1;
  ErrorCount = 0;
  ErrorLine = 0;
  ScanPtr = source;
  Identifiers = new SymbolTable(this);
  Flags = new SymbolTable(this);
  ControlStack.Count = 0;
  Output.Count = 0;
  IndexedPatches.Count = 0;
  FileName = fileName;
  Errors = errors;
  Errors->Clear();

  DestModifier = 0;
  NumTempRegs = 0;
  NumFlags = 0;

  WordScan();
  ParseVersion();

  AddPredefinedSymbols();
  ParseCode();

  if(ControlStack.Count)
    Error("Not all control structures closed!");
  
  Emit(XO_END);

  if(ErrorCount != 0)
  {
    Errors->PrintF("\n%d error(s)\n",ErrorCount);
    
    Output[0] = XO_END;
    Output.Count = 1;
  }

  delete Identifiers;
  delete Flags;

  FileName = 0;
  Errors = 0;
  return ErrorCount == 0;
}

const sU32 *sShaderCompiler::GetShader() const
{
  return &Output[0];
}

sU32 *sShaderCompiler::GetShaderCopy() const
{
  sU32 *shaderCopy = new sU32[Output.Count];
  sCopyMem4(shaderCopy,&Output[0],Output.Count);

  return shaderCopy;
}

sU32 sShaderCompiler::GetShaderSize() const
{
  return Output.Count;
}

/****************************************************************************/
