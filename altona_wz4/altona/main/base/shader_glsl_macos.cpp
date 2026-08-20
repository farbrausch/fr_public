/*+**************************************************************************/
/***                                                                      ***/
/***   Cg/HLSL -> GLSL 1.20 translator for the macOS OpenGL 2.1 backend   ***/
/***                                                                      ***/
/**************************************************************************+*/

// Altona's OGL2 renderer stores its shaders as Cg source (that is what ASC
// emits for sRENDER_OGL2) and used to hand them to the NVIDIA Cg runtime.
// Cg is long gone, and macOS never had it, so the source is translated to
// GLSL 1.20 here instead.
//
// This is not a general HLSL front end. It handles exactly the dialect ASC
// generates -- a flat list of "TYPE name : register(cN);" declarations, an
// optional handful of helper functions, and a single main() whose parameters
// carry the semantics -- plus the intrinsics those shaders actually use.
// Anything outside that fails to translate, the caller falls back, and the
// material is skipped rather than drawn wrongly.

#include "base/shader_glsl_macos.hpp"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/****************************************************************************/
/***   growable text buffer                                               ***/
/****************************************************************************/

// Deliberately malloc based: altona replaces global operator new with its own
// tracking allocator, and this code runs during shader creation where that
// would only add noise.

void sGlslBuf::Init()
{
  Ptr = (char *)malloc(1024);
  Len = 0;
  Cap = 1024;
  if(Ptr) Ptr[0] = 0;
}

void sGlslBuf::Exit()
{
  free(Ptr);
  Ptr = 0; Len = 0; Cap = 0;
}

void sGlslBuf::AddN(const char *s,int n)
{
  if(!Ptr || n<=0) return;
  if(Len+n+1 > Cap)
  {
    int cap = Cap*2;
    while(Len+n+1 > cap) cap *= 2;
    char *p = (char *)realloc(Ptr,cap);
    if(!p) return;
    Ptr = p; Cap = cap;
  }
  memcpy(Ptr+Len,s,n);
  Len += n;
  Ptr[Len] = 0;
}

void sGlslBuf::Add(const char *s)
{
  if(s) AddN(s,(int)strlen(s));
}

void sGlslBuf::AddF(const char *fmt,...)
{
  char tmp[1024];
  va_list ap;
  va_start(ap,fmt);
  vsnprintf(tmp,sizeof(tmp),fmt,ap);
  va_end(ap);
  Add(tmp);
}

/****************************************************************************/
/***   types and tables                                                   ***/
/****************************************************************************/

enum
{
  TY_UNKNOWN = 0,
  TY_FLOAT, TY_FLOAT2, TY_FLOAT3, TY_FLOAT4,
  TY_INT4,
  TY_BOOL,
  TY_FLOAT4X4, TY_FLOAT4X3, TY_FLOAT3X4,
  TY_SAMPLER2D, TY_SAMPLERCUBE, TY_SAMPLER3D, TY_SAMPLER2DSHADOW,
};

static int ParseType(const char *s,int len)
{
  #define EQ(lit) (len==(int)sizeof(lit)-1 && memcmp(s,lit,len)==0)
  if(EQ("float"))       return TY_FLOAT;
  if(EQ("half"))        return TY_FLOAT;
  if(EQ("half2"))       return TY_FLOAT2;
  if(EQ("half3"))       return TY_FLOAT3;
  if(EQ("half4"))       return TY_FLOAT4;
  if(EQ("float1"))      return TY_FLOAT;
  if(EQ("float2"))      return TY_FLOAT2;
  if(EQ("float3"))      return TY_FLOAT3;
  if(EQ("float4"))      return TY_FLOAT4;
  if(EQ("int4"))        return TY_INT4;
  if(EQ("bool"))        return TY_BOOL;
  if(EQ("uint4"))       return TY_INT4;
  if(EQ("float4x4"))    return TY_FLOAT4X4;
  if(EQ("float4x3"))    return TY_FLOAT4X3;
  if(EQ("float3x4"))    return TY_FLOAT3X4;
  if(EQ("sampler2D"))   return TY_SAMPLER2D;
  if(EQ("samplerCUBE")) return TY_SAMPLERCUBE;
  if(EQ("sampler3D"))   return TY_SAMPLER3D;
  if(EQ("sampler2DShadow")) return TY_SAMPLER2DSHADOW;
  #undef EQ
  return TY_UNKNOWN;
}

static const char *GlslType(int ty)
{
  switch(ty)
  {
  case TY_FLOAT:        return "float";
  case TY_FLOAT2:       return "vec2";
  case TY_FLOAT3:       return "vec3";
  case TY_FLOAT4:       return "vec4";
  case TY_INT4:         return "vec4";
  case TY_BOOL:         return "bool";
  case TY_FLOAT4X4:     return "mat4";
  case TY_FLOAT4X3:     return "mat4";
  case TY_FLOAT3X4:     return "mat4";
  case TY_SAMPLER2D:    return "sampler2D";
  case TY_SAMPLERCUBE:  return "samplerCube";
  case TY_SAMPLER3D:    return "sampler3D";
  case TY_SAMPLER2DSHADOW: return "sampler2DShadow";
  }
  return "vec4";
}

static int TypeComponents(int ty)
{
  switch(ty)
  {
  case TY_FLOAT:  return 1;
  case TY_FLOAT2: return 2;
  case TY_FLOAT3: return 3;
  }
  return 4;
}

// The generic vertex attribute index altona binds each semantic to; see the
// sVF_* switch in sVertexFormatHandle::Create2.
// Semantics come in two spellings: ASC emits "TEXCOORD0", the modular material
// generator emits "texcoord0". Compare case-insensitively and treat the number
// as a suffix.
static void NormaliseSemantic(const char *sem,char *name,int *index)
{
  int i = 0;
  while(sem[i] && !isdigit((unsigned char)sem[i]) && i<31)
  {
    name[i] = (char)toupper((unsigned char)sem[i]);
    i++;
  }
  name[i] = 0;
  *index = isdigit((unsigned char)sem[i]) ? atoi(sem+i) : 0;
}

static int SemanticToAttr(const char *sem)
{
  char n[32];
  int idx;
  NormaliseSemantic(sem,n,&idx);

  if(!strcmp(n,"ATTR"))               return idx;
  if(!strcmp(n,"POSITION"))           return 0;
  if(!strcmp(n,"BLENDWEIGHT"))        return 1;
  if(!strcmp(n,"NORMAL"))             return 2;
  if(!strcmp(n,"COLOR"))              return 3+idx;
  if(!strcmp(n,"TANGENT"))            return 6;
  if(!strcmp(n,"BLENDINDICES"))       return 7;
  if(!strcmp(n,"TEXCOORD"))           return 8+idx;
  return -1;
}

// The varying name a semantic maps to, so a vertex and pixel shader agree.
static void SemanticVaryingName(const char *sem,char *out)
{
  char n[32];
  int idx;
  NormaliseSemantic(sem,n,&idx);
  snprintf(out,64,"%s%d",n,idx);
}

static int SemanticIs(const char *sem,const char *want)
{
  char n[32];
  int idx;
  NormaliseSemantic(sem,n,&idx);
  return !strcmp(n,want);
}

static int SemanticIndex(const char *sem)
{
  char n[32];
  int idx;
  NormaliseSemantic(sem,n,&idx);
  return idx;
}

/****************************************************************************/
/***   scanning helpers                                                   ***/
/****************************************************************************/

// How many float4 registers a matrix occupies. row_major stores one row per
// register, column_major one column, so the two differ for a non-square matrix.
static int MatrixRegSpan(int type,int rowmajor)
{
  switch(type)
  {
  case TY_FLOAT4X4: return 4;
  case TY_FLOAT3X4: return rowmajor ? 3 : 4;
  case TY_FLOAT4X3: return rowmajor ? 4 : 3;
  }
  return 1;
}

static void sCopyName(char *d,const char *s){ int i=0; while(s[i] && i<62){ d[i]=s[i]; i++; } d[i]=0; }

static int IsIdentChar(char c)  { return isalnum((unsigned char)c) || c=='_'; }
static int IsIdentStart(char c) { return isalpha((unsigned char)c) || c=='_'; }

// Index of the ')' matching the '(' at open, or -1.
static int MatchParen(const char *s,int len,int open)
{
  int depth = 0;
  for(int i=open;i<len;i++)
  {
    if(s[i]=='(') depth++;
    else if(s[i]==')') { depth--; if(depth==0) return i; }
  }
  return -1;
}

// Offset of the top level comma inside [from,to), or -1.
static int TopLevelComma(const char *s,int from,int to)
{
  int depth = 0;
  for(int i=from;i<to;i++)
  {
    if(s[i]=='(' || s[i]=='[') depth++;
    else if(s[i]==')' || s[i]==']') depth--;
    else if(s[i]==',' && depth==0) return i;
  }
  return -1;
}

// Same, but braces nest too. Only the array-initializer splitter needs this:
// "{ {1,0}, {-1,0} }" must break at the commas between the groups, not inside.
static int TopLevelCommaB(const char *s,int from,int to)
{
  int depth = 0;
  for(int i=from;i<to;i++)
  {
    if(s[i]=='(' || s[i]=='[' || s[i]=='{') depth++;
    else if(s[i]==')' || s[i]==']' || s[i]=='}') depth--;
    else if(s[i]==',' && depth==0) return i;
  }
  return -1;
}

static int IsNumberLiteral(const char *s,int len)
{
  int i = 0, digits = 0;
  while(i<len && (s[i]==' '||s[i]=='\t')) i++;
  if(i<len && (s[i]=='-'||s[i]=='+')) i++;
  for(;i<len;i++)
  {
    if(isdigit((unsigned char)s[i])) { digits++; continue; }
    if(s[i]=='.' || s[i]=='f' || s[i]=='F') continue;
    if(s[i]==' '||s[i]=='\t') continue;
    return 0;
  }
  return digits>0;
}

/****************************************************************************/
/***   translator                                                         ***/
/****************************************************************************/

struct Sym
{
  char Name[64];
  int Type;
  int Reg;                              // constant register, -1 if none
  int ArraySize;                        // 0 = not an array
  int RowMajor;
};

struct Param
{
  char Name[64];
  char Sem[32];
  int Type;
  int IsOut;
};

struct Func                             // a helper function declared in the shader
{
  char Name[64];
  int Argc;
  int Arg[8];                           // components per parameter, 0 = leave alone
};

struct Ctx
{
  const char *CName;
  const char *RetName;                  // main returns a value, not out params
  int RetComponents;
  const char *RetEpilogue;              // what a return has to emit on its way out
  Sym Syms[128];
  int SymCount;
  Param Params[32];
  int ParamCount;
  Func Funcs[32];
  int FuncCount;
  int IsVertex;
  int Failed;
  sGlslShaderInfo *Info;
};

static Sym *FindSym(Ctx *cx,const char *name,int len)
{
  for(int i=0;i<cx->SymCount;i++)
    if((int)strlen(cx->Syms[i].Name)==len && !memcmp(cx->Syms[i].Name,name,len))
      return &cx->Syms[i];
  return 0;
}

static Func *FindFunc(Ctx *cx,const char *name,int len)
{
  for(int i=0;i<cx->FuncCount;i++)
    if((int)strlen(cx->Funcs[i].Name)==len && !memcmp(cx->Funcs[i].Name,name,len))
      return &cx->Funcs[i];
  return 0;
}

static void TranslateRange(Ctx *cx,const char *s,int from,int to,sGlslBuf *out);

// Emit "mul(a,b)" using what we know about b's declaration.
static void EmitMul(Ctx *cx,const char *s,int a0,int a1,int b0,int b1,sGlslBuf *out)
{
  // trim the second argument so it can be looked up as a plain identifier
  while(b0<b1 && isspace((unsigned char)s[b0])) b0++;
  while(b1>b0 && isspace((unsigned char)s[b1-1])) b1--;
  Sym *m = FindSym(cx,s+b0,b1-b0);

  // mul(M,v) treats v as a column vector: result_i = dot(row_i,v). With
  // row_major storage each row is one register, so this is a dot per row.
  {
    int t0 = a0, t1 = a1;
    while(t0<t1 && isspace((unsigned char)s[t0])) t0++;
    while(t1>t0 && isspace((unsigned char)s[t1-1])) t1--;
    Sym *ma = FindSym(cx,s+t0,t1-t0);
    if(ma && ma->Reg>=0 && !m &&
       (ma->Type==TY_FLOAT4X4 || ma->Type==TY_FLOAT4X3 || ma->Type==TY_FLOAT3X4))
    {
      int rows = (ma->Type==TY_FLOAT3X4) ? 3 : 4;
      if(ma->Type==TY_FLOAT4X3) rows = 4;
      out->AddF("vec%d(",rows);
      for(int i=0;i<rows;i++)
      {
        if(i) out->Add(",");
        out->Add("sglsl_dot(");
        out->AddF("%s[%d],sglsl_v4(",cx->CName,ma->Reg+i);
        TranslateRange(cx,s,b0,b1,out);
        out->Add("))");
      }
      out->Add(")");
      return;
    }
  }

  if(m && m->Type==TY_FLOAT4X4 && m->Reg>=0)
  {
    if(m->RowMajor)
    {
      // rows live in consecutive registers; mat4() takes columns, which is
      // exactly the transpose a row-vector multiply needs
      out->AddF("(mat4(%s[%d],%s[%d],%s[%d],%s[%d])*(",cx->CName,m->Reg,cx->CName,m->Reg+1,cx->CName,m->Reg+2,cx->CName,m->Reg+3);
      TranslateRange(cx,s,a0,a1,out);
      out->Add("))");
    }
    else
    {
      out->Add("vec4(");
      for(int i=0;i<4;i++)
      {
        if(i) out->Add(",");
        out->Add("sglsl_dot(");
        TranslateRange(cx,s,a0,a1,out);
        out->AddF(",%s[%d])",cx->CName,m->Reg+i);
      }
      out->Add(")");
    }
    return;
  }
  if(m && m->Type==TY_FLOAT4X3 && m->Reg>=0)
  {
    out->Add("vec3(");
    for(int i=0;i<3;i++)
    {
      if(i) out->Add(",");
      out->Add("sglsl_dot(");
      TranslateRange(cx,s,a0,a1,out);
      out->AddF(",%s[%d])",cx->CName,m->Reg+i);
    }
    out->Add(")");
    return;
  }

  // A local matrix carries the same layout as its HLSL counterpart, and GLSL
  // reads "vector * matrix" as a row-vector multiply -- exactly what mul does.
  if(m && (m->Type==TY_FLOAT4X4 || m->Type==TY_FLOAT4X3 || m->Type==TY_FLOAT3X4) && m->Reg<0)
  {
    // the spare fourth column of a float4x3 is dropped again here
    const char *swz = (m->Type==TY_FLOAT4X3) ? ".xyz" : "";
    out->Add("((");
    TranslateRange(cx,s,a0,a1,out);
    out->Add(")*(");
    TranslateRange(cx,s,b0,b1,out);
    out->AddF("))%s",swz);
    return;
  }

  // mul(M,v) where M is a local matrix. The constructor above stored HLSL rows
  // as GLSL rows, and GLSL's mat*vec dots each row with the vector -- which is
  // what mul() does with the matrix on the left.
  {
    int t0 = a0, t1 = a1;
    while(t0<t1 && isspace((unsigned char)s[t0])) t0++;
    while(t1>t0 && isspace((unsigned char)s[t1-1])) t1--;
    Sym *ma = FindSym(cx,s+t0,t1-t0);
    if(ma && ma->Reg<0 &&
       (ma->Type==TY_FLOAT4X4 || ma->Type==TY_FLOAT4X3 || ma->Type==TY_FLOAT3X4))
    {
      // a float3x4 was padded out to mat4 with a zero row: drop it again
      const char *swz = (ma->Type==TY_FLOAT3X4) ? ".xyz" : "";
      out->Add("((");
      TranslateRange(cx,s,a0,a1,out);
      out->Add(")*(");
      TranslateRange(cx,s,b0,b1,out);
      out->AddF("))%s",swz);
      return;
    }
  }

  // not a declared matrix: a plain product is the best we can do, and if that
  // is wrong the shader fails to compile and the material falls back
  cx->Failed = 1;
  out->Add("((");
  TranslateRange(cx,s,a0,a1,out);
  out->Add(")*(");
  TranslateRange(cx,s,b0,b1,out);
  out->Add("))");
}

// True if [from,to) has a comparison operator at bracket depth 0.
static int HasTopLevelCompare(const char *s,int from,int to)
{
  int depth = 0;
  for(int i=from;i<to;i++)
  {
    if(s[i]=='('||s[i]=='[') depth++;
    else if(s[i]==')'||s[i]==']') depth--;
    else if(depth==0)
    {
      if(s[i]=='<' || s[i]=='>') return 1;
      if((s[i]=='='||s[i]=='!') && i+1<to && s[i+1]=='=') return 1;
    }
  }
  return 0;
}

static int IsArith(char c) { return c=='*'||c=='/'||c=='+'||c=='-'; }

/****************************************************************************/

// HLSL truncates the wider operand of a mixed-width binary expression down to
// the narrower one -- "float4 + float3" is a float3 -- while GLSL rejects the
// pair outright. The generated Cg is fully parenthesised, so a group holds at
// most one top level operator and both sides can be measured and cut to size.

static int WidthOfType(int ty)
{
  switch(ty)
  {
  case TY_FLOAT:  return 1;
  case TY_FLOAT2: return 2;
  case TY_FLOAT3: return 3;
  case TY_FLOAT4: return 4;
  case TY_INT4:   return 4;
  }
  return 0;                             // matrices, samplers, bool: don't guess
}

// Offset of the one top level +,-,*,/ in [from,to), or -1 when there is none,
// more than one, or anything that makes the range something other than a plain
// arithmetic group. A leading sign and a float exponent are not operators.
static int TopLevelBinary(const char *s,int from,int to)
{
  int depth = 0, found = -1;
  for(int i=from;i<to;i++)
  {
    char c = s[i];
    if(c=='(' || c=='[' || c=='{') { depth++; continue; }
    if(c==')' || c==']' || c=='}') { depth--; continue; }
    if(depth) continue;
    if(c==',' || c==';' || c=='?' || c==':' ||
       c=='<' || c=='>' || c=='=' || c=='!' || c=='&' || c=='|' || c=='%')
      return -1;
    if(!IsArith(c)) continue;

    int p = i-1;
    while(p>=from && isspace((unsigned char)s[p])) p--;
    if(p<from) continue;                                    // leading sign
    if(IsArith(s[p]) || s[p]=='(' || s[p]==',') continue;    // unary after op
    if((c=='+'||c=='-') && (s[p]=='e'||s[p]=='E')) continue; // 1e-5

    if(found>=0) return -1;
    found = i;
  }
  return found;
}

static int ExprWidth(Ctx *cx,const char *s,int from,int to)
{
  while(from<to && isspace((unsigned char)s[from])) from++;
  while(to>from && isspace((unsigned char)s[to-1])) to--;
  if(to<=from) return 0;

  if(s[from]=='(' && MatchParen(s,to,from)==to-1)
    return ExprWidth(cx,s,from+1,to-1);

  if(s[from]=='-' || s[from]=='+' || s[from]=='!')
    return ExprWidth(cx,s,from+1,to);

  {
    int op = TopLevelBinary(s,from,to);
    if(op>=0)
    {
      int a = ExprWidth(cx,s,from,op);
      int b = ExprWidth(cx,s,op+1,to);
      if(!a) return b;
      if(!b) return a;
      if(a==1) return b;
      if(b==1) return a;
      return (a<b) ? a : b;
    }
  }

  // a trailing .xyzw / .rgba decides the width on its own
  {
    int d = -1, depth = 0;
    for(int i=from;i<to;i++)
    {
      if(s[i]=='(' || s[i]=='[') depth++;
      else if(s[i]==')' || s[i]==']') depth--;
      else if(s[i]=='.' && depth==0) d = i;
    }
    if(d>from && d+1<to)
    {
      int n = 0, ok = 1;
      for(int i=d+1;i<to;i++)
      {
        if(strchr("xyzwrgba",s[i])) n++;
        else { ok = 0; break; }
      }
      if(ok && n>=1 && n<=4)            // "1.0" fails this: '0' is not a swizzle
        return n;
    }
  }

  // constructor or intrinsic call
  {
    int e = from;
    while(e<to && IsIdentChar(s[e])) e++;
    if(e>from && e<to)
    {
      int q = e;
      while(q<to && isspace((unsigned char)s[q])) q++;
      if(q<to && s[q]=='(' && MatchParen(s,to,q)==to-1)
      {
        int len = e-from;
        const char *id = s+from;
        int w = WidthOfType(ParseType(id,len));
        if(w) return w;

        #define ISN(lit) (len==(int)sizeof(lit)-1 && !memcmp(id,lit,len))
        if(ISN("tex2D") || ISN("tex2Dproj") || ISN("tex3D") || ISN("texCUBE") ||
           ISN("tex2Dlod") || ISN("texCUBElod") || ISN("texture2D") ||
           ISN("texture2DProj") || ISN("texture3D") || ISN("textureCube"))
          return 4;
        if(ISN("dot") || ISN("length") || ISN("distance") || ISN("determinant"))
          return 1;
        if(ISN("cross") || ISN("reflect"))
          return 3;
        if(ISN("saturate") || ISN("abs") || ISN("normalize") || ISN("frac") ||
           ISN("fract") || ISN("floor") || ISN("ceil") || ISN("sqrt") ||
           ISN("rsqrt") || ISN("inversesqrt") || ISN("exp") || ISN("exp2") ||
           ISN("log") || ISN("log2") || ISN("sin") || ISN("cos") || ISN("tan") ||
           ISN("sign") || ISN("ddx") || ISN("ddy") || ISN("dFdx") || ISN("dFdy") ||
           ISN("min") || ISN("max") || ISN("pow") || ISN("mod") || ISN("fmod") ||
           ISN("clamp") || ISN("lerp") || ISN("mix"))
        {
          int a0 = q+1, a1 = TopLevelComma(s,q+1,to-1);
          if(a1<0) a1 = to-1;
          return ExprWidth(cx,s,a0,a1);
        }
        #undef ISN
        return 0;                       // a user helper: unknown
      }
    }
  }

  // array element: the constant register file is float4, anything else carries
  // the element type of its declaration
  {
    int e = from;
    while(e<to && IsIdentChar(s[e])) e++;
    if(e>from && e<to && s[e]=='[' && s[to-1]==']')
    {
      if(cx->CName && (int)strlen(cx->CName)==e-from && !memcmp(s+from,cx->CName,e-from))
        return 4;
      Sym *sy = FindSym(cx,s+from,e-from);
      if(sy) return WidthOfType(sy->Type);
      return 0;
    }
  }

  // plain identifier or literal
  {
    int e = from;
    while(e<to && IsIdentChar(s[e])) e++;
    if(e==to)
    {
      if(IsNumberLiteral(s+from,to-from)) return 1;
      Sym *sy = FindSym(cx,s+from,to-from);
      if(sy) return WidthOfType(sy->Type);
    }
  }

  return 0;
}

static void TranslateRange(Ctx *cx,const char *s,int from,int to,sGlslBuf *out)
{
  int i = from;
  while(i<to)
  {
    char c = s[i];

    // HLSL truncates the wider side of a mixed-width binary expression. Cut it
    // down here rather than letting the glsl compiler reject the whole shader.
    if(c=='(')
    {
      int k = MatchParen(s,to,i);
      int op = (k>0) ? TopLevelBinary(s,i+1,k) : -1;
      if(op>i)
      {
        int wa = ExprWidth(cx,s,i+1,op);
        int wb = ExprWidth(cx,s,op+1,k);
        if(wa>1 && wb>1 && wa!=wb)
        {
          static const char *swz[5] = { "", ".x", ".xy", ".xyz", "" };
          int w = (wa<wb) ? wa : wb;
          out->Add("(");
          if(wa>w) { out->Add("("); TranslateRange(cx,s,i+1,op,out); out->AddF(")%s",swz[w]); }
          else     TranslateRange(cx,s,i+1,op,out);
          out->AddN(s+op,1);
          if(wb>w) { out->Add("("); TranslateRange(cx,s,op+1,k,out); out->AddF(")%s",swz[w]); }
          else     TranslateRange(cx,s,op+1,k,out);
          out->Add(")");
          i = k+1;
          continue;
        }
      }
    }

    // HLSL happily multiplies a comparison result; GLSL needs the cast
    if(c=='(' && HasTopLevelCompare(s,i+1,to))
    {
      int k = MatchParen(s,to,i);
      if(k>0 && HasTopLevelCompare(s,i+1,k))
      {
        int a = k+1;
        while(a<to && isspace((unsigned char)s[a])) a++;
        int b = i-1;
        while(b>=from && isspace((unsigned char)s[b])) b--;
        if((a<to && IsArith(s[a])) || (b>=from && IsArith(s[b])))
          out->Add("float");
      }
    }

    if(!IsIdentStart(c))
    {
      out->AddN(&c,1);
      i++;
      continue;
    }

    int b = i;
    while(i<to && IsIdentChar(s[i])) i++;
    int len = i-b;
    const char *id = s+b;

    // A name right after a '.' is a swizzle or a member, never a symbol of its
    // own. Looking it up is how "ss_pos.w" came out as "ss_pos.w.x": the
    // shader also declares a float3 named w, so the narrowing below took the
    // swizzle letter for that vector and cut it down to a float's width.
    if(b>0 && s[b-1]=='.')
    {
      out->AddN(id,len);
      continue;
    }

    #define IS(lit) (len==(int)sizeof(lit)-1 && !memcmp(id,lit,len))

    // HLSL matrix constructors take rows; GLSL's take columns. Build the
    // transpose so the element layout matches, and pad a 3-row matrix out to
    // mat4 (Apple's GLSL 1.20 has no non-square matrix types).
    {
      int mty = ParseType(id,len);
      if(mty==TY_FLOAT4X4 || mty==TY_FLOAT3X4 || mty==TY_FLOAT4X3)
      {
        int op = i;
        while(op<to && isspace((unsigned char)s[op])) op++;
        if(op<to && s[op]=='(')
        {
          int cl = MatchParen(s,to,op);
          if(cl>0)
          {
            int rows = 0, p = op+1;
            int argstart[4], argend[4];
            while(p<cl && rows<4)
            {
              int e = TopLevelComma(s,p,cl);
              if(e<0) e = cl;
              argstart[rows] = p;
              argend[rows] = e;
              rows++;
              p = e+1;
            }
            out->Add("transpose(mat4(");
            for(int k=0;k<4;k++)
            {
              if(k) out->Add(",");
              if(k<rows) TranslateRange(cx,s,argstart[k],argend[k],out);
              else       out->Add("vec4(0.0)");
            }
            out->Add("))");
            i = cl+1;
            continue;
          }
        }
      }
    }

    // types
    int ty = ParseType(id,len);
    if(ty && ty<TY_SAMPLER2D && !(len==5 && !memcmp(id,"float",5) && 0))
    {
      // "float" itself is also a valid GLSL type, GlslType maps it through
      out->Add(GlslType(ty));
      continue;
    }

    // "transpose(float3x4(r0,r1,r2))" builds a float4x3 whose columns are the
    // three row vectors -- which is just a GLSL mat3x4 of the same three.
    if(IS("transpose"))
    {
      int op = i;
      while(op<to && isspace((unsigned char)s[op])) op++;
      if(op<to && s[op]=='(')
      {
        int cl = MatchParen(s,to,op);
        int q = op+1;
        while(q<cl && isspace((unsigned char)s[q])) q++;
        if(cl>0 && cl-q>9 && !memcmp(s+q,"float3x4(",9))
        {
          int icl = MatchParen(s,cl,q+8);
          if(icl>0 && icl<cl)
          {
            out->Add("mat4(");
            TranslateRange(cx,s,q+9,icl,out);
            out->Add(",vec4(0.0))");
            i = cl+1;
            continue;
          }
        }
      }
      out->Add("transpose");
      continue;
    }

    // mul() needs both arguments
    if(IS("mul") )
    {
      int op = i;
      while(op<to && s[op]!='(') op++;
      int cl = MatchParen(s,to,op);
      int cm = (cl>0) ? TopLevelComma(s,op+1,cl) : -1;
      if(cl>0 && cm>0)
      {
        EmitMul(cx,s,op+1,cm,cm+1,cl,out);
        i = cl+1;
        continue;
      }
      cx->Failed = 1;
      out->AddN(id,len);
      continue;
    }

    // intrinsics that only need renaming
    if(IS("tex2D"))      { out->Add("texture2D");     continue; }
    if(IS("tex2Dproj"))
    {
      // On a shadow sampler this is the depth comparison d3d9 does in the
      // texture unit -- GLSL spells that shadow2DProj, and will not even take
      // a sampler2DShadow anywhere else.
      int a = i;
      while(a<to && isspace((unsigned char)s[a])) a++;
      int shadow = 0;
      if(a<to && s[a]=='(')
      {
        a++;
        while(a<to && isspace((unsigned char)s[a])) a++;
        int n0 = a;
        while(a<to && IsIdentChar(s[a])) a++;
        Sym *sa = (a>n0) ? FindSym(cx,s+n0,a-n0) : 0;
        shadow = sa && sa->Type==TY_SAMPLER2DSHADOW;
      }
      out->Add(shadow ? "shadow2DProj" : "texture2DProj");
      continue;
    }
    if(IS("tex3D"))      { out->Add("texture3D");     continue; }
    if(IS("texCUBE"))    { out->Add("textureCube");   continue; }
    if(IS("lerp"))       { out->Add("sglsl_lerp");     continue; }
    if(IS("dot"))        { out->Add("sglsl_dot");      continue; }
    if(IS("frac"))       { out->Add("fract");         continue; }
    if(IS("rsqrt"))      { out->Add("inversesqrt");   continue; }
    if(IS("atan2"))      { out->Add("atan");          continue; }
    if(IS("fmod"))       { out->Add("mod");           continue; }
    if(IS("ddx"))        { out->Add("dFdx");          continue; }
    if(IS("ddy"))        { out->Add("dFdy");          continue; }
    // saturate/clip/tex2Dlod/sincos come from the preamble helpers

    // A shader whose result is its return value can return from anywhere,
    // including out of a braceless if. main() is void here, so each return
    // becomes "write the result, emit it, leave" -- the epilogue has to be
    // repeated or an early exit would skip it.
    if(cx->RetName && IS("return"))
    {
      int e = i, d2 = 0;
      for(;e<to;e++)
      {
        if(s[e]=='('||s[e]=='[') d2++;
        else if(s[e]==')'||s[e]==']') d2--;
        else if(s[e]==';' && d2==0) break;
      }
      out->AddF("{ %s=",cx->RetName);
      if(cx->RetComponents>=1 && cx->RetComponents<=4)
      {
        out->AddF("sglsl_v%d(",cx->RetComponents);
        TranslateRange(cx,s,i,e,out);
        out->Add(")");
      }
      else
        TranslateRange(cx,s,i,e,out);
      out->AddF("; %s return; }",cx->RetEpilogue ? cx->RetEpilogue : "");
      i = (e<to) ? e+1 : to;
      continue;
    }

    // HLSL qualifiers with no GLSL equivalent. row_major only describes how a
    // matrix is packed into constant registers, which never reaches the body.
    if(IS("static") || IS("row_major") || IS("column_major")) { continue; }

    // a lone h/f straight after a number is an HLSL literal suffix
    if(len==1 && (id[0]=='h'||id[0]=='H'||id[0]=='f'||id[0]=='F') &&
       b>from && (isdigit((unsigned char)s[b-1]) || s[b-1]=='.'))
      continue;

    // Calling one of those helpers: adapt every argument to the width its
    // parameter was declared with, the way HLSL does silently.
    {
      Func *fn = FindFunc(cx,id,len);
      int nx = i;
      while(nx<to && isspace((unsigned char)s[nx])) nx++;

      // the definition reads exactly like a call -- what is in front of the
      // name tells them apart, and its parameter list must be left be. The
      // return type is not always one ParseType knows: "void IterateJulia(
      // inout float4 q, ...)" is a definition too, and wrapping its parameters
      // as if they were arguments is a syntax error.
      int pb = b-1;
      while(pb>=0 && isspace((unsigned char)s[pb])) pb--;
      int pe = pb+1;
      while(pb>=0 && IsIdentChar(s[pb])) pb--;
      int isdef = 0;
      if(pe>pb+1)
      {
        int rl = pe-(pb+1);
        const char *r = s+pb+1;
        static const char *decl[] = { "void","int","uint","inline","static",
                                      "const","uniform","inout","out","in",0 };
        isdef = ParseType(r,rl)!=0;
        for(int k=0;decl[k] && !isdef;k++)
          if((int)strlen(decl[k])==rl && !memcmp(decl[k],r,rl))
            isdef = 1;
      }

      if(fn && !isdef && nx<to && s[nx]=='(')
      {
        int cl = MatchParen(s,to,nx);
        if(cl>0)
        {
          out->AddF("%.*s(",len,id);
          int p = nx+1, a = 0;
          while(p<cl)
          {
            int e = TopLevelComma(s,p,cl);
            if(e<0) e = cl;
            if(a) out->Add(",");
            int w = (a<fn->Argc) ? fn->Arg[a] : 0;
            if(w>=1 && w<=4)
            {
              out->AddF("sglsl_v%d(",w);
              TranslateRange(cx,s,p,e,out);
              out->Add(")");
            }
            else
              TranslateRange(cx,s,p,e,out);
            a++;
            p = e+1;
          }
          out->Add(")");
          i = cl+1;
          continue;
        }
      }
    }

    Sym *sym = FindSym(cx,id,len);

    // A variable named after a GLSL builtin is legal HLSL but rejected here.
    // Anything not followed by '(' is a variable, and a shader that declares
    // one of these as a variable cannot also be calling the intrinsic.
    {
      static const char *shadowed[] = { "min","max","mod","step","mix","length",
                                        "distance","normalize","sign","fract",0 };
      int nx = i;
      while(nx<to && isspace((unsigned char)s[nx])) nx++;
      if(nx>=to || s[nx]!='(')
      {
        for(int k=0;shadowed[k];k++)
          if((int)strlen(shadowed[k])==len && !memcmp(shadowed[k],id,len))
          {
            out->AddF("%.*s_v",len,id);
            goto nextToken;
          }
      }
    }

    // HLSL lets a scalar be swizzled ("f.xxxx"), GLSL does not
    if(sym && sym->Type==TY_FLOAT && sym->ArraySize==0 && i<to && s[i]=='.')
    {
      int j = i+1, nsw = 0;
      while(j<to && strchr("xyzwrgba",s[j])) { j++; nsw++; }
      if(nsw>0 && (j>=to || !IsIdentChar(s[j])))
      {
        if(nsw==1) out->AddN(id,len);
        else       out->AddF("vec%d(%.*s)",nsw,len,id);
        i = j;
        continue;
      }
    }

    // a constant-register array turns into an offset into the register file
    if(sym && sym->ArraySize>0 && sym->Reg>=0)
    {
      int j = i;
      while(j<to && isspace((unsigned char)s[j])) j++;
      if(j<to && s[j]=='[')
      {
        // find the matching ']' so the subscript can be wrapped
        int depth = 0, k = j;
        for(;k<to;k++)
        {
          if(s[k]=='[') depth++;
          else if(s[k]==']') { depth--; if(!depth) break; }
        }
        if(k<to)
        {
          // GLSL wants an int subscript, and index attributes arrive as float
          out->AddF("%s[%d+int(",cx->CName,sym->Reg);
          TranslateRange(cx,s,j+1,k,out);
          out->Add(")]");
          i = k+1;
          continue;
        }
      }
    }

    // HLSL narrows the wider operand when two vectors of different width meet
    // in an arithmetic expression. Full inference is out of scope, but the
    // common case -- both operands plain identifiers we know the type of -- is
    // worth catching, and covers "pos - center" with a float4 and a float3.
    if(sym && sym->ArraySize==0 && sym->Type>=TY_FLOAT && sym->Type<=TY_FLOAT4 &&
       (i>=to || s[i]!='.'))
    {
      int mywidth = TypeComponents(sym->Type);
      int other = 0;

      // this identifier followed by "op ident"
      int j = i;
      while(j<to && isspace((unsigned char)s[j])) j++;
      if(j<to && strchr("+-*/",s[j]) && (j+1>=to || !strchr("+-*/=",s[j+1])))
      {
        int k = j+1;
        while(k<to && isspace((unsigned char)s[k])) k++;
        int b2 = k;
        while(k<to && IsIdentChar(s[k])) k++;
        if(k>b2 && (k>=to || s[k]!='.') && (k>=to || s[k]!='('))
        {
          Sym *o = FindSym(cx,s+b2,k-b2);
          if(o && o->ArraySize==0 && o->Type>=TY_FLOAT && o->Type<=TY_FLOAT4)
            other = TypeComponents(o->Type);
        }
      }

      // or preceded by "ident op"
      if(!other)
      {
        int j2 = b-1;
        while(j2>=from && isspace((unsigned char)s[j2])) j2--;
        if(j2>=from && strchr("+-*/",s[j2]) && (j2<=from || s[j2-1]!='='))
        {
          int k = j2-1;
          while(k>=from && isspace((unsigned char)s[k])) k--;
          int e2 = k+1;
          while(k>=from && IsIdentChar(s[k])) k--;
          if(e2>k+1)
          {
            Sym *o = FindSym(cx,s+k+1,e2-(k+1));
            if(o && o->ArraySize==0 && o->Type>=TY_FLOAT && o->Type<=TY_FLOAT4)
              other = TypeComponents(o->Type);
          }
        }
      }

      // A scalar operand splats in HLSL as it does in GLSL -- "r*delta" with
      // a float r stays a float3. Only a narrower *vector* truncates the
      // wider one, so leave width 1 alone.
      if(other>1 && other<mywidth)
      {
        static const char *swz[5] = { "", ".x", ".xy", ".xyz", "" };
        out->AddF("%.*s%s",len,id,swz[other]);
        goto nextToken;
      }
    }

    out->AddN(id,len);
  nextToken:;
    #undef IS
  }
}

// Copy a statement, promoting "v = 1;" to "v = vec4(1.0);" where v is a vector.
// HLSL splats scalars across a vector on assignment, GLSL does not.
static void TranslateStatement(Ctx *cx,const char *s,int from,int to,sGlslBuf *out)
{
  // "float a=x, b=y+z, c=w;" declares several variables in one statement. The
  // narrowing further down takes everything after the first '=' as a single
  // right hand side, so it wrapped the remaining declarators up as extra
  // arguments of sglsl_v1(...) and they were never declared at all -- which is
  // what "use of undeclared identifier" was, twenty four shaders' worth of it.
  // Give each declarator a statement of its own.
  {
    int t0 = from;
    while(t0<to && isspace((unsigned char)s[t0])) t0++;
    int t1 = t0;
    while(t1<to && IsIdentChar(s[t1])) t1++;
    int declty = ParseType(s+t0,t1-t0);

    if(declty>=TY_FLOAT && declty<=TY_FLOAT4 && t1>t0)
    {
      // top level commas, the ones that separate declarators
      int commas = 0, depth = 0;
      for(int i=t1;i<to;i++)
      {
        char c = s[i];
        if(c=='('||c=='['||c=='{') depth++;
        else if(c==')'||c==']'||c=='}') depth--;
        else if(c==',' && depth==0) commas++;
      }

      if(commas>0)
      {
        const int want = TypeComponents(declty);
        int p = t1;
        while(p<to)
        {
          // one declarator, up to the next top level comma or the end
          int d0 = p, d1 = to, dep = 0;
          for(int i=p;i<to;i++)
          {
            char c = s[i];
            if(c=='('||c=='['||c=='{') dep++;
            else if(c==')'||c==']'||c=='}') dep--;
            else if((c==','||c==';') && dep==0) { d1 = i; break; }
          }
          while(d0<d1 && isspace((unsigned char)s[d0])) d0++;
          while(d1>d0 && isspace((unsigned char)s[d1-1])) d1--;

          if(d1>d0)
          {
            out->AddF("%.*s ",t1-t0,s+t0);          // the shared type

            // split the declarator at its own '='
            int de = -1, dd = 0;
            for(int i=d0;i<d1;i++)
            {
              char c = s[i];
              if(c=='('||c=='[') dd++;
              else if(c==')'||c==']') dd--;
              else if(c=='=' && dd==0 && !(i+1<d1 && s[i+1]=='=')) { de = i; break; }
            }
            if(de<0)
            {
              TranslateRange(cx,s,d0,d1,out);
            }
            else
            {
              TranslateRange(cx,s,d0,de+1,out);
              out->AddF("sglsl_v%d(",want);
              TranslateRange(cx,s,de+1,d1,out);
              out->Add(")");
            }
            out->Add(";");
          }

          p = d1+1;
          if(d1>=to || s[d1]==';') break;
        }
        return;
      }
    }
  }

  // "if(cond) col += lerp(a,b,t);" is one statement, and the assignment inside
  // it needs the same width fixing as a free-standing one. Emit the condition,
  // then look at what it guards on its own.
  {
    int k0 = from;
    while(k0<to && isspace((unsigned char)s[k0])) k0++;
    int k1 = k0;
    while(k1<to && IsIdentChar(s[k1])) k1++;

    #define KW(lit) (k1-k0==(int)sizeof(lit)-1 && !memcmp(s+k0,lit,k1-k0))
    if(k1>k0 && (KW("if") || KW("while") || KW("for")))
    {
      int p = k1;
      while(p<to && isspace((unsigned char)s[p])) p++;
      if(p<to && s[p]=='(')
      {
        int cl = MatchParen(s,to,p);
        if(cl>0 && cl+1<to)
        {
          TranslateRange(cx,s,from,cl+1,out);
          TranslateStatement(cx,s,cl+1,to,out);
          return;
        }
      }
    }
    if(k1>k0 && KW("else"))
    {
      TranslateRange(cx,s,from,k1,out);
      TranslateStatement(cx,s,k1,to,out);
      return;
    }
    #undef KW
  }

  int eq = -1, depth = 0;
  for(int i=from;i<to;i++)
  {
    if(s[i]=='('||s[i]=='[') depth++;
    else if(s[i]==')'||s[i]==']') depth--;
    else if(s[i]=='=' && depth==0)
    {
      if(i+1<to && s[i+1]=='=') { i++; continue; }
      if(i>from && (s[i-1]=='='||s[i-1]=='<'||s[i-1]=='>'||s[i-1]=='!')) continue;
      eq = i;
      break;
    }
  }

  if(eq>0)
  {
    // left hand side: bare identifier, no swizzle
    int l0 = from, l1 = eq;
    while(l0<l1 && isspace((unsigned char)s[l0])) l0++;
    while(l1>l0 && (isspace((unsigned char)s[l1-1]) || strchr("+-*/",s[l1-1]))) l1--;

    int r0 = eq+1, r1 = to;
    while(r1>r0 && (isspace((unsigned char)s[r1-1]) || s[r1-1]==';')) r1--;

    // "float a[8] = { ... };" -- GLSL 1.20 has no array initializer syntax, so
    // declare the array and fill it element by element.
    {
      int q0 = r0, q1 = r1;
      while(q0<q1 && isspace((unsigned char)s[q0])) q0++;
      while(q1>q0 && (isspace((unsigned char)s[q1-1]) || s[q1-1]==';')) q1--;
      if(q1-q0>=2 && s[q0]=='{' && s[q1-1]=='}')
      {
        // the declaration itself, without the initializer
        TranslateRange(cx,s,from,l1,out);
        out->Add(";");

        // the name is the last identifier before any '['
        int ne = l1;
        while(ne>l0 && s[ne-1]!='[') ne--;
        int nend = (ne>l0) ? ne-1 : l1;
        while(nend>l0 && isspace((unsigned char)s[nend-1])) nend--;
        int nstart = nend;
        while(nstart>l0 && IsIdentChar(s[nstart-1])) nstart--;

        // element type, for the "float2 ord[8] = { {1,0}, .. }" form where
        // each element is a braced group and needs an explicit constructor
        int et0 = l0;
        while(et0<l1 && IsIdentChar(s[et0])) et0++;
        int elemty = ParseType(s+l0,et0-l0);

        int idx = 0, p = q0+1;
        while(p<q1-1)
        {
          int e = TopLevelCommaB(s,p,q1-1);
          if(e<0) e = q1-1;
          int v0 = p, v1 = e;
          while(v0<v1 && isspace((unsigned char)s[v0])) v0++;
          while(v1>v0 && isspace((unsigned char)s[v1-1])) v1--;
          if(v1>v0)
          {
            out->AddF("%.*s[%d]=",nend-nstart,s+nstart,idx++);
            if(v1-v0>=2 && s[v0]=='{' && s[v1-1]=='}' && elemty)
            {
              out->AddF("%s(",GlslType(elemty));
              TranslateRange(cx,s,v0+1,v1-1,out);
              out->Add(")");
            }
            else
            {
              TranslateRange(cx,s,v0,v1,out);
            }
            out->Add(";");
          }
          p = e+1;
        }
        return;
      }
    }

    // How many components does the left hand side want? It is either a
    // declaration ("float3 x = .."), a bare name, or a swizzle of one.
    int target = 0;
    {
      int t0 = l0;
      while(t0<l1 && IsIdentChar(s[t0])) t0++;
      int declty = ParseType(s+l0,t0-l0);

      int p = t0;
      while(p<l1 && isspace((unsigned char)s[p])) p++;
      int n0 = p;
      while(p<l1 && IsIdentChar(s[p])) p++;

      if(declty>=TY_FLOAT && declty<=TY_FLOAT4 && p>n0 && p==l1)
      {
        target = TypeComponents(declty);         // "float3 x = ..."
      }
      else if(t0>l0)
      {
        int nswizzle = 0, ok = 1;
        if(t0<l1)
        {
          if(s[t0]!='.') ok = 0;
          else
          {
            int j = t0+1;
            while(j<l1 && strchr("xyzwrgba",s[j])) { j++; nswizzle++; }
            if(j!=l1) ok = 0;
          }
        }
        if(ok)
        {
          if(nswizzle) target = nswizzle;
          else
          {
            Sym *sym = FindSym(cx,s+l0,t0-l0);
            if(sym && sym->Type==TY_BOOL && sym->ArraySize==0)
              target = -1;
            else if(sym && sym->Type>=TY_FLOAT && sym->Type<=TY_FLOAT4 && sym->ArraySize==0)
              target = TypeComponents(sym->Type);
          }
        }
      }
    }

    // HLSL splats a scalar across the target and truncates anything wider;
    // GLSL does neither. Routing the value through an overload set lets the
    // compiler pick the right conversion without us inferring the type.
    if(target!=0 && r1>r0)
    {
      TranslateRange(cx,s,from,eq+1,out);
      if(target<0) out->Add("sglsl_vb(");
      else         out->AddF("sglsl_v%d(",target);
      TranslateRange(cx,s,r0,r1,out);
      out->Add(");");
      return;
    }
  }

  TranslateRange(cx,s,from,to,out);
}

// Split the body into statements so scalar promotion can look at one at a time.
static void TranslateBody(Ctx *cx,const char *s,int from,int to,sGlslBuf *out)
{
  int start = from, depth = 0, initdepth = 0;
  for(int i=from;i<to;i++)
  {
    if(s[i]=='('||s[i]=='[') depth++;
    else if(s[i]==')'||s[i]==']') depth--;
    else if(s[i]=='{' && depth==0)
    {
      // "= { ... }" is an initializer, not a block: keep it in the statement
      int p = i-1;
      while(p>=from && isspace((unsigned char)s[p])) p--;
      if((p>=from && s[p]=='=') || initdepth)
      {
        initdepth++;
        continue;
      }
      TranslateStatement(cx,s,start,i+1,out);
      start = i+1;
    }
    else if(s[i]=='}' && depth==0 && initdepth)
    {
      initdepth--;
    }
    else if((s[i]==';' || s[i]=='}') && depth==0 && !initdepth)
    {
      TranslateStatement(cx,s,start,i+1,out);
      start = i+1;
    }
  }
  if(start<to)
    TranslateRange(cx,s,start,to,out);
}

/****************************************************************************/


// ASC likes to name a helper function's parameters after the uniforms the
// caller passes in ("float curve(float z, float4 Color0, ...)"). Register
// aliases are emitted as #defines, so the preprocessor rewrites those
// parameter declarations into "vec4 sglsl_pc[0]" and the function -- and every
// local in it -- disappears in a cascade of syntax errors. Rename the
// shadowing parameters instead; the call site still passes the uniform.
static void FixHelperShadowing(Ctx *cx,sGlslBuf *helpers)
{
  const char *s = helpers->Ptr;
  int len = helpers->Len;
  if(!s || len<=0) return;

  sGlslBuf out;
  out.Init();

  int i = 0;
  while(i<len)
  {
    // a function definition looks like: name ( params ) {
    if(s[i]=='(')
    {
      int close = MatchParen(s,len,i);
      int brace = close+1;
      while(brace<len && isspace((unsigned char)s[brace])) brace++;

      if(close>0 && brace<len && s[brace]=='{')
      {
        int depth = 0, bodyend = brace;
        for(;bodyend<len;bodyend++)
        {
          if(s[bodyend]=='{') depth++;
          else if(s[bodyend]=='}') { depth--; if(!depth) break; }
        }
        if(bodyend>=len) bodyend = len-1;

        // which parameters shadow a register alias?
        char shadow[16][64];
        int shadowcount = 0;
        Func sig;                       // and how wide is each parameter
        sig.Name[0] = 0;
        sig.Argc = 0;
        int p = i+1;
        while(p<close && shadowcount<16)
        {
          int e = TopLevelComma(s,p,close);
          if(e<0) e = close;
          int q = p;
          while(q<e && isspace((unsigned char)s[q])) q++;
          int t0 = q;
          while(q<e && IsIdentChar(s[q])) q++;
          int pty = ParseType(s+t0,q-t0);
          if(sig.Argc<8)
          {
            // "out"/"inout" parses as no type at all, and an argument passed to
            // one must stay the variable it is -- 0 leaves it alone.
            sig.Arg[sig.Argc++] = (pty>=TY_FLOAT && pty<=TY_FLOAT4) ? TypeComponents(pty) : 0;
          }
          if(pty)
          {
            while(q<e && isspace((unsigned char)s[q])) q++;
            int n0 = q;
            while(q<e && IsIdentChar(s[q])) q++;
            Sym *sy = FindSym(cx,s+n0,q-n0);
            if(sy && sy->Reg>=0 && q-n0<63)
            {
              memcpy(shadow[shadowcount],s+n0,q-n0);
              shadow[shadowcount][q-n0] = 0;
              shadowcount++;
            }
          }
          p = e+1;
        }

        // a local can shadow one just as well: "float C = ..." inside a helper,
        // where C is a constant register, expands to "float sglsl_pc[0] = ..."
        // and takes the whole shader with it. Collect those too.
        {
          int q = brace;
          while(q<=bodyend && shadowcount<16)
          {
            if(!IsIdentStart(s[q]) || (q>0 && (IsIdentChar(s[q-1]) || s[q-1]=='.')))
            {
              q++;
              continue;
            }
            int t0 = q;
            while(q<=bodyend && IsIdentChar(s[q])) q++;
            if(!ParseType(s+t0,q-t0)) continue;

            while(q<=bodyend && isspace((unsigned char)s[q])) q++;
            int n0 = q;
            while(q<=bodyend && IsIdentChar(s[q])) q++;
            if(q<=n0 || q-n0>=63) continue;

            Sym *sy = FindSym(cx,s+n0,q-n0);
            if(!sy || sy->Reg<0) continue;

            int known = 0;
            for(int k=0;k<shadowcount;k++)
              if((int)strlen(shadow[k])==q-n0 && !memcmp(shadow[k],s+n0,q-n0))
                known = 1;
            if(known) continue;

            memcpy(shadow[shadowcount],s+n0,q-n0);
            shadow[shadowcount][q-n0] = 0;
            shadowcount++;
          }
        }

        // the declared return type, so "return" can adapt its width the same
        // way an assignment does
        int rettype = 0;
        {
          int q = i-1;
          while(q>=0 && isspace((unsigned char)s[q])) q--;
          int nend = q+1;
          while(q>=0 && IsIdentChar(s[q])) q--;          // skip the name
          int ne = q+1;

          // HLSL narrows an argument to the parameter's width, GLSL will not
          // even find the function: the depth of field pass calls
          // "circleOfConfusion(tex2D(zbuffer,uv))" for a float z. Remember the
          // signature so the call sites can adapt.
          if(nend>ne && nend-ne<64 && cx->FuncCount<32)
          {
            memcpy(sig.Name,s+ne,nend-ne);
            sig.Name[nend-ne] = 0;
            if(!FindFunc(cx,sig.Name,nend-ne))
              cx->Funcs[cx->FuncCount++] = sig;
          }
          while(q>=0 && isspace((unsigned char)s[q])) q--;
          int te = q+1;
          while(q>=0 && IsIdentChar(s[q])) q--;
          if(te>q+1 && ne>0)
          {
            int t = ParseType(s+q+1,te-(q+1));
            if(t==TY_BOOL) rettype = -1;
            else if(t>=TY_FLOAT && t<=TY_FLOAT4) rettype = TypeComponents(t);
          }
        }

        // copy the whole function, renaming those names as we go
        int j = i;
        while(j<=bodyend)
        {
          if(IsIdentStart(s[j]))
          {
            int b = j;
            while(j<=bodyend && IsIdentChar(s[j])) j++;
            int nl = j-b, hit = -1;
            for(int k=0;k<shadowcount;k++)
              if((int)strlen(shadow[k])==nl && !memcmp(shadow[k],s+b,nl))
                hit = k;

            if(rettype && nl==6 && !memcmp(s+b,"return",6) && j<=bodyend)
            {
              int e = j;
              int d2 = 0;
              for(;e<=bodyend;e++)
              {
                if(s[e]=='('||s[e]=='[') d2++;
                else if(s[e]==')'||s[e]==']') d2--;
                else if(s[e]==';' && d2==0) break;
              }
              if(e<=bodyend && e>j)
              {
                if(rettype<0) out.Add("return sglsl_vb(");
                else          out.AddF("return sglsl_v%d(",rettype);
                out.AddN(s+j,e-j);
                out.Add(");");
                j = e+1;
                continue;
              }
            }

            if(hit>=0) out.AddF("%s_a",shadow[hit]);
            else       out.AddN(s+b,nl);
          }
          else
          {
            out.AddN(&s[j],1);
            j++;
          }
        }
        i = bodyend+1;
        continue;
      }
    }
    out.AddN(&s[i],1);
    i++;
  }

  helpers->Exit();
  *helpers = out;
}

static void AddSym(Ctx *cx,const char *name,int namelen,int type,int reg,int arr,int rowmajor)
{
  if(cx->SymCount >= (int)(sizeof(cx->Syms)/sizeof(cx->Syms[0]))) { cx->Failed = 1; return; }
  Sym *sy = &cx->Syms[cx->SymCount++];
  if(namelen > 63) namelen = 63;
  memcpy(sy->Name,name,namelen);
  sy->Name[namelen] = 0;
  sy->Type = type;
  sy->Reg = reg;
  sy->ArraySize = arr;
  sy->RowMajor = rowmajor;
}

int sGlslTranslateShader(const char *src,int isvertex,sGlslShaderInfo *info)
{
  Ctx cx;
  memset(&cx,0,sizeof(cx));
  cx.IsVertex = isvertex;
  const char *cname = isvertex ? "sglsl_vc" : "sglsl_pc";
  cx.CName = cname;
  cx.Info = info;

  memset(info,0,sizeof(*info));
  info->Source.Init();

  int len = (int)strlen(src);

  // Blank out comments first, keeping every offset and newline in place. The
  // statement splitter works on ';' and '{', so a trailing "// setup" would
  // otherwise glue itself onto the next declaration and stop it being seen as
  // one.
  char *work = (char *)malloc(len+1);
  if(!work) { info->Source.Exit(); return 0; }
  memcpy(work,src,len+1);
  for(int i=0;i<len;i++)
  {
    if(work[i]=='/' && i+1<len && work[i+1]=='/')
    {
      while(i<len && work[i]!='\n') work[i++] = ' ';
    }
    else if(work[i]=='/' && i+1<len && work[i+1]=='*')
    {
      work[i] = work[i+1] = ' ';
      i += 2;
      while(i<len && !(work[i]=='*' && i+1<len && work[i+1]=='/'))
      {
        if(work[i]!='\n') work[i] = ' ';
        i++;
      }
      if(i<len) { work[i] = ' '; if(i+1<len) work[i+1] = ' '; i++; }
    }
  }
  src = work;

  // ---- find main() -------------------------------------------------------

  const char *mainp = strstr(src,"void main");
  char retsem[32] = "";
  if(!mainp)
  {
    // "float4 main( .. ) : COLOR0 { .. }" -- the output semantic is on the
    // return type rather than on an out parameter
    mainp = strstr(src," main");
    if(!mainp) mainp = strstr(src,"\nmain");
    if(!mainp) { free(work); info->Source.Exit(); return 0; }
    while(mainp>src && (isspace((unsigned char)mainp[0]) || mainp[0]=='\n')) mainp++;
    const char *ln = mainp;
    while(ln>src && ln[-1]!='\n') ln--;
    mainp = ln;
  }
  int mainat = (int)(mainp-src);

  int paren0 = mainat;
  while(paren0<len && src[paren0]!='(') paren0++;
  int paren1 = MatchParen(src,len,paren0);
  if(paren1<0) { free(work); info->Source.Exit(); return 0; }

  {
    int p = paren1+1;
    while(p<len && isspace((unsigned char)src[p])) p++;
    if(p<len && src[p]==':')
    {
      p++;
      while(p<len && isspace((unsigned char)src[p])) p++;
      int t = 0;
      while(p<len && IsIdentChar(src[p]) && t<31) retsem[t++] = src[p++];
      retsem[t] = 0;
    }
  }

  int brace0 = paren1;
  while(brace0<len && src[brace0]!='{') brace0++;
  if(brace0>=len) { free(work); info->Source.Exit(); return 0; }
  int brace1 = len;
  while(brace1>brace0 && src[brace1-1]!='}') brace1--;
  if(brace1<=brace0) { free(work); info->Source.Exit(); return 0; }

  // ---- parse the register declarations in the header ---------------------

  int maxreg = 0;
  sGlslBuf helpers;                     // helper functions declared before main
  helpers.Init();

  {
    int i = 0;
    while(i<mainat)
    {
      int e = i;
      while(e<mainat && src[e]!='\n') e++;

      // "[row_major ]TYPE name[[N]] : register(cN);"
      int p = i;
      while(p<e && isspace((unsigned char)src[p])) p++;
      int rowmajor = 0;
      if(e-p>10 && !memcmp(src+p,"row_major",9)) { rowmajor = 1; p += 9; while(p<e && isspace((unsigned char)src[p])) p++; }

      int t0 = p;
      while(p<e && IsIdentChar(src[p])) p++;
      int type = ParseType(src+t0,p-t0);
      int consumed = 0;

      if(type)
      {
        while(p<e && isspace((unsigned char)src[p])) p++;
        int n0 = p;
        while(p<e && IsIdentChar(src[p])) p++;
        int n1 = p;
        int arr = 0;
        if(p<e && src[p]=='[')
        {
          arr = atoi(src+p+1);
          while(p<e && src[p]!=']') p++;
          if(p<e) p++;
        }
        while(p<e && isspace((unsigned char)src[p])) p++;
        if(n1>n0 && p<e && src[p]==':')
        {
          p++;
          while(p<e && isspace((unsigned char)src[p])) p++;
          if(e-p>9 && !memcmp(src+p,"register(",9))
          {
            char cls = src[p+9];
            int reg = atoi(src+p+10);
            AddSym(&cx,src+n0,n1-n0,type,reg,arr,rowmajor);

            if(type>=TY_SAMPLER2D)
            {
              if(info->SamplerCount < 16)
              {
                int nl = n1-n0; if(nl>63) nl = 63;
                memcpy(info->SamplerName[info->SamplerCount],src+n0,nl);
                info->SamplerName[info->SamplerCount][nl] = 0;
                info->SamplerReg[info->SamplerCount] = reg;
                info->SamplerCount++;
              }
              (void)cls;
            }
            else
            {
              int span = MatrixRegSpan(type,rowmajor);
              if(arr>0) span = arr;
              if(reg+span > maxreg) maxreg = reg+span;
            }
            consumed = 1;
          }
        }
      }

      if(!consumed)
        helpers.AddN(src+i,e-i+(e<mainat?1:0));

      i = e+1;
    }
  }

  // ---- parse main's parameter list ---------------------------------------

  {
    int i = paren0+1;
    while(i<paren1)
    {
      int e = TopLevelComma(src,i,paren1);
      if(e<0) e = paren1;

      int p = i;
      while(p<e && isspace((unsigned char)src[p])) p++;
      int isout = 0, isuniform = 0;
      if(e-p>7 && !memcmp(src+p,"uniform",7) && !IsIdentChar(src[p+7])) { isuniform = 1; p += 7; }
      else if(e-p>3 && !memcmp(src+p,"out",3) && !IsIdentChar(src[p+3])) { isout = 1; p += 3; }
      else if(e-p>2 && !memcmp(src+p,"in",2) && !IsIdentChar(src[p+2])) { p += 2; }
      while(p<e && isspace((unsigned char)src[p])) p++;

      // the matrix layout qualifier sits between the storage class and the type
      int prowmajor = 0;
      if(e-p>9 && !memcmp(src+p,"row_major",9) && !IsIdentChar(src[p+9]))
      {
        prowmajor = 1;
        p += 9;
        while(p<e && isspace((unsigned char)src[p])) p++;
      }
      else if(e-p>12 && !memcmp(src+p,"column_major",12) && !IsIdentChar(src[p+12]))
      {
        p += 12;
        while(p<e && isspace((unsigned char)src[p])) p++;
      }

      int t0 = p;
      while(p<e && IsIdentChar(src[p])) p++;
      int type = ParseType(src+t0,p-t0);
      while(p<e && isspace((unsigned char)src[p])) p++;
      int n0 = p;
      while(p<e && IsIdentChar(src[p])) p++;
      int n1 = p;
      int parr = 0;
      if(p<e && src[p]=='[')
      {
        parr = atoi(src+p+1);
        while(p<e && src[p]!=']') p++;
        if(p<e) p++;
      }
      while(p<e && isspace((unsigned char)src[p])) p++;

      // a uniform parameter is really just another register declaration
      if(isuniform && type && n1>n0 && p<e && src[p]==':')
      {
        p++;
        while(p<e && isspace((unsigned char)src[p])) p++;
        if(e-p>9 && !memcmp(src+p,"register(",9))
        {
          int reg = atoi(src+p+10);
          AddSym(&cx,src+n0,n1-n0,type,reg,parr,prowmajor);
          if(type>=TY_SAMPLER2D)
          {
            if(info->SamplerCount < 16)
            {
              int nl = n1-n0; if(nl>63) nl = 63;
              memcpy(info->SamplerName[info->SamplerCount],src+n0,nl);
              info->SamplerName[info->SamplerCount][nl] = 0;
              info->SamplerReg[info->SamplerCount] = reg;
              info->SamplerCount++;
            }
          }
          else
          {
            int span = MatrixRegSpan(type,prowmajor);
            if(parr>0) span = parr;
            if(reg+span > maxreg) maxreg = reg+span;
          }
        }
        i = e+1;
        continue;
      }

      if(type && n1>n0 && p<e && src[p]==':')
      {
        p++;
        while(p<e && isspace((unsigned char)src[p])) p++;
        int s0 = p;
        while(p<e && IsIdentChar(src[p])) p++;

        if(cx.ParamCount < 32)
        {
          Param *pa = &cx.Params[cx.ParamCount++];
          int nl = n1-n0; if(nl>63) nl = 63;
          memcpy(pa->Name,src+n0,nl); pa->Name[nl] = 0;
          int sl = p-s0; if(sl>31) sl = 31;
          memcpy(pa->Sem,src+s0,sl); pa->Sem[sl] = 0;
          pa->Type = type;
          pa->IsOut = isout;
        }
      }
      i = e+1;
    }
  }

  // a return semantic behaves like an out parameter called sglsl_ret
  if(retsem[0] && cx.ParamCount<32)
  {
    int t0 = mainat;
    while(t0<len && isspace((unsigned char)src[t0])) t0++;
    int t1 = t0;
    while(t1<len && IsIdentChar(src[t1])) t1++;
    Param *pa = &cx.Params[cx.ParamCount++];
    sCopyName(pa->Name,"sglsl_ret");
    sCopyName(pa->Sem,retsem);
    pa->Type = ParseType(src+t0,t1-t0);
    if(!pa->Type) pa->Type = TY_FLOAT4;
    pa->IsOut = 1;
  }

  for(int i=0;i<cx.ParamCount;i++)
    if(!FindSym(&cx,cx.Params[i].Name,(int)strlen(cx.Params[i].Name)))
      AddSym(&cx,cx.Params[i].Name,(int)strlen(cx.Params[i].Name),cx.Params[i].Type,-1,0,0);

  if(maxreg < 1) maxreg = 1;
  if(maxreg > 1024) { free(work); helpers.Exit(); info->Source.Exit(); return 0; }

  // ---- emit --------------------------------------------------------------

  sGlslBuf *o = &info->Source;
  o->Add("#version 120\n");
  o->AddF("uniform vec4 %s[%d];\n",cname,maxreg);
  info->RegCount = maxreg;

  for(int i=0;i<info->SamplerCount;i++)
  {
    Sym *sy = FindSym(&cx,info->SamplerName[i],(int)strlen(info->SamplerName[i]));
    o->AddF("uniform %s %s;\n",GlslType(sy?sy->Type:TY_SAMPLER2D),info->SamplerName[i]);
  }

  // scalar and vector constants become aliases into c[]
  for(int i=0;i<cx.SymCount;i++)
  {
    Sym *sy = &cx.Syms[i];
    if(sy->Type>=TY_SAMPLER2D || sy->Reg<0 || sy->ArraySize>0) continue;
    if(sy->Type==TY_FLOAT4X4 || sy->Type==TY_FLOAT4X3) continue;   // only via mul()
    static const char *swz[5] = { "", ".x", ".xy", ".xyz", "" };
    o->AddF("#define %s %s[%d]%s\n",sy->Name,cname,sy->Reg,swz[TypeComponents(sy->Type)]);
  }

  // helpers for the intrinsics that are not a plain rename
  o->Add(
    "float saturate(float x){return clamp(x,0.0,1.0);}\n"
    "vec2 saturate(vec2 x){return clamp(x,0.0,1.0);}\n"
    "vec3 saturate(vec3 x){return clamp(x,0.0,1.0);}\n"
    "vec4 saturate(vec4 x){return clamp(x,0.0,1.0);}\n"
    "float round(float x){return floor(x+0.5);}\n"
    "vec2 round(vec2 x){return floor(x+0.5);}\n"
    "vec3 round(vec3 x){return floor(x+0.5);}\n"
    "vec4 round(vec4 x){return floor(x+0.5);}\n"
    "float modf(float x,out float ip){ip=floor(x);return x-ip;}\n"
    "float trunc(float x){return sign(x)*floor(abs(x));}\n"
    "float sglsl_lerp(float a,float b,float t){return mix(a,b,t);}\n"
    "vec2 sglsl_lerp(vec2 a,vec2 b,float t){return mix(a,b,t);}\n"
    "vec3 sglsl_lerp(vec3 a,vec3 b,float t){return mix(a,b,t);}\n"
    "vec4 sglsl_lerp(vec4 a,vec4 b,float t){return mix(a,b,t);}\n"
    "vec2 sglsl_lerp(vec2 a,vec4 b,float t){return mix(a,b.xy,t);}\n"
    "vec3 sglsl_lerp(vec3 a,vec4 b,float t){return mix(a,b.xyz,t);}\n"
    "vec2 sglsl_lerp(vec2 a,vec2 b,vec2 t){return mix(a,b,t);}\n"
    "vec3 sglsl_lerp(vec3 a,vec3 b,vec3 t){return mix(a,b,t);}\n"
    "vec4 sglsl_lerp(vec4 a,vec4 b,vec4 t){return mix(a,b,t);}\n"
    "vec3 sglsl_lerp(vec3 a,vec4 b,vec3 t){return mix(a,b.xyz,t);}\n"
    "vec4 sglsl_lerp(vec4 a,vec4 b,float t,float u){return mix(a,b,t);}\n"
    "float sglsl_v1(float x){return x;}\n"
    "float sglsl_v1(vec2 x){return x.x;}\n"
    "float sglsl_v1(vec3 x){return x.x;}\n"
    "float sglsl_v1(vec4 x){return x.x;}\n"
    "vec2 sglsl_v2(float x){return vec2(x);}\n"
    "vec2 sglsl_v2(vec2 x){return x;}\n"
    "vec2 sglsl_v2(vec3 x){return x.xy;}\n"
    "vec2 sglsl_v2(vec4 x){return x.xy;}\n"
    "vec3 sglsl_v3(float x){return vec3(x);}\n"
    "vec3 sglsl_v3(vec2 x){return vec3(x,0.0);}\n"
    "vec3 sglsl_v3(vec3 x){return x;}\n"
    "vec3 sglsl_v3(vec4 x){return x.xyz;}\n"
    "vec4 sglsl_v4(float x){return vec4(x);}\n"
    "vec4 sglsl_v4(vec2 x){return vec4(x,0.0,0.0);}\n"
    "vec4 sglsl_v4(vec3 x){return vec4(x,0.0);}\n"
    "vec4 sglsl_v4(vec4 x){return x;}\n"
    // HLSL cuts the wider argument of a mixed-width dot down to the narrower
    // one; GLSL has no dot(vec3,vec4) at all. Expanding a matrix multiply into
    // one dot per register row hits this every time the vector is a float3 and
    // the constant registers are, as they always are, float4.
    "float sglsl_dot(float a,float b){return a*b;}\n"
    "float sglsl_dot(vec2 a,vec2 b){return dot(a,b);}\n"
    "float sglsl_dot(vec3 a,vec3 b){return dot(a,b);}\n"
    "float sglsl_dot(vec4 a,vec4 b){return dot(a,b);}\n"
    "float sglsl_dot(vec2 a,vec3 b){return dot(a,b.xy);}\n"
    "float sglsl_dot(vec3 a,vec2 b){return dot(a.xy,b);}\n"
    "float sglsl_dot(vec2 a,vec4 b){return dot(a,b.xy);}\n"
    "float sglsl_dot(vec4 a,vec2 b){return dot(a.xy,b);}\n"
    "float sglsl_dot(vec3 a,vec4 b){return dot(a,b.xyz);}\n"
    "float sglsl_dot(vec4 a,vec3 b){return dot(a.xyz,b);}\n"
    "bool sglsl_vb(bool x){return x;}\n"
    "bool sglsl_vb(float x){return x!=0.0;}\n"
    "float sglsl_v1(bool x){return x?1.0:0.0;}\n"
    "vec2 sglsl_v2(bool x){return vec2(x?1.0:0.0);}\n"
    "vec3 sglsl_v3(bool x){return vec3(x?1.0:0.0);}\n"
    "vec4 sglsl_v4(bool x){return vec4(x?1.0:0.0);}\n");
  if(!isvertex)
    o->Add(
      "void clip(float x){if(x<0.0)discard;}\n"
      "void clip(vec4 x){if(any(lessThan(x,vec4(0.0))))discard;}\n");
  if(isvertex)
    o->Add("vec4 tex2Dlod(sampler2D s,vec4 t){return texture2DLod(s,t.xy,t.w);}\n");
  else
    o->Add("vec4 tex2Dlod(sampler2D s,vec4 t){return texture2D(s,t.xy);}\n");

  // attributes and varyings
  for(int i=0;i<cx.ParamCount;i++)
  {
    Param *pa = &cx.Params[i];
    if(isvertex && !pa->IsOut)
    {
      int idx = SemanticToAttr(pa->Sem);
      if(idx<0) { cx.Failed = 1; continue; }
      if(info->AttrCount < 16)
      {
        snprintf(info->AttrName[info->AttrCount],64,"a_%s",pa->Name);
        info->AttrIndex[info->AttrCount] = idx;
        info->AttrCount++;
      }
      o->AddF("attribute vec4 a_%s;\n",pa->Name);
    }
    else if(isvertex && pa->IsOut)
    {
      if(SemanticIs(pa->Sem,"POSITION")) continue;
      { char vn[64]; SemanticVaryingName(pa->Sem,vn); o->AddF("varying vec4 v_%s;\n",vn); }
    }
    else if(!isvertex && !pa->IsOut)
    {
      if(SemanticIs(pa->Sem,"WPOS") || SemanticIs(pa->Sem,"VPOS")) continue;
      { char vn[64]; SemanticVaryingName(pa->Sem,vn); o->AddF("varying vec4 v_%s;\n",vn); }
    }
  }

  // helper functions declared before main
  FixHelperShadowing(&cx,&helpers);
  {
    Ctx hx = cx;
    TranslateBody(&hx,helpers.Ptr,0,helpers.Len,o);
    if(hx.Failed) cx.Failed = 1;
    o->Add("\n");
  }
  helpers.Exit();

  o->Add("void main()\n{\n");

  // matrix aliases have to be inside a function: their initialisers read c[]
  for(int i=0;i<cx.SymCount;i++)
  {
    Sym *sy = &cx.Syms[i];
    if(sy->Type!=TY_FLOAT4X4 || sy->Reg<0) continue;
    if(sy->RowMajor)
      o->AddF("  mat4 %s=mat4(%s[%d],%s[%d],%s[%d],%s[%d]);\n",sy->Name,cname,sy->Reg,cname,sy->Reg+1,cname,sy->Reg+2,cname,sy->Reg+3);
    else
      o->AddF("  mat4 %s=transpose(mat4(%s[%d],%s[%d],%s[%d],%s[%d]));\n",sy->Name,cname,sy->Reg,cname,sy->Reg+1,cname,sy->Reg+2,cname,sy->Reg+3);
  }

  // bring parameters in as locals so the body can stay as written
  for(int i=0;i<cx.ParamCount;i++)
  {
    Param *pa = &cx.Params[i];
    const char *gt = GlslType(pa->Type);
    int nc = TypeComponents(pa->Type);
    static const char *swz[5] = { "", ".x", ".xy", ".xyz", "" };

    if(isvertex && !pa->IsOut)
      o->AddF("  %s %s=a_%s%s;\n",gt,pa->Name,pa->Name,swz[nc]);
    else if(!isvertex && !pa->IsOut)
    {
      if(SemanticIs(pa->Sem,"WPOS") || SemanticIs(pa->Sem,"VPOS"))
        o->AddF("  %s %s=gl_FragCoord%s;\n",gt,pa->Name,swz[nc]);
      else
      {
        char vn[64]; SemanticVaryingName(pa->Sem,vn);
        o->AddF("  %s %s=v_%s%s;\n",gt,pa->Name,vn,swz[nc]);
      }
    }
    else
      o->AddF("  %s %s;\n",gt,pa->Name);
  }

  // pick up locals declared inside the body; scalar promotion needs their types
  {
    int i = brace0+1;
    while(i<brace1-1)
    {
      while(i<brace1-1 && !IsIdentStart(src[i]))
      {
        if(src[i]=='\'') { i++; continue; }
        i++;
      }
      int t0 = i;
      while(i<brace1-1 && IsIdentChar(src[i])) i++;
      int ty = ParseType(src+t0,i-t0);
      if(ty && ty<TY_SAMPLER2D)
      {
        // "float z,d,f;" declares three of them, and only the first was ever
        // registered -- the other two then had no type for the promotion to
        // work from, so "d = texCUBE(..).x < z;" kept its bool and the point
        // light shadow shaders would not compile. Walk every declarator.
        int j = i;
        for(;;)
        {
          while(j<brace1-1 && isspace((unsigned char)src[j])) j++;
          int n0 = j;
          while(j<brace1-1 && IsIdentChar(src[j])) j++;
          int n1 = j;
          while(j<brace1-1 && isspace((unsigned char)src[j])) j++;

          // a declaration, not a constructor call: "float4 name;" or "name ="
          if(n1<=n0 || j>=brace1-1) break;
          if(src[j]!=';' && src[j]!='=' && src[j]!=',' && src[j]!='[') break;
          if(!FindSym(&cx,src+n0,n1-n0))
            AddSym(&cx,src+n0,n1-n0,ty,-1,0,0);

          // step over this declarator's initializer to the next one
          int depth = 0;
          while(j<brace1-1)
          {
            char c = src[j];
            if(c=='('||c=='['||c=='{') depth++;
            else if(c==')'||c==']'||c=='}') depth--;
            else if(depth==0 && (c==','||c==';')) break;
            j++;
          }
          if(j>=brace1-1 || src[j]==';') { j++; break; }
          j++;
        }
        i = j;
      }
      // skip to the next token boundary
      while(i<brace1-1 && IsIdentChar(src[i])) i++;
    }
  }

  cx.RetName = retsem[0] ? "sglsl_ret" : 0;
  cx.RetComponents = 0;
  for(int i=0;i<cx.ParamCount;i++)
    if(cx.Params[i].IsOut && !strcmp(cx.Params[i].Name,"sglsl_ret"))
      cx.RetComponents = TypeComponents(cx.Params[i].Type);

  static char retepi[128];
  retepi[0] = 0;
  if(cx.RetName)
  {
    int nc = cx.RetComponents ? cx.RetComponents : 4;
    const char *pad = (nc==1) ? ",0.0,0.0,0.0" : (nc==2) ? ",0.0,0.0" : (nc==3) ? ",0.0" : "";
    if(SemanticIs(retsem,"DEPTH"))
      snprintf(retepi,sizeof(retepi),"gl_FragDepth=float(sglsl_ret);");
    else
      snprintf(retepi,sizeof(retepi),"gl_FragColor=vec4(sglsl_ret%s);",pad);
  }
  cx.RetEpilogue = retepi[0] ? retepi : 0;
  TranslateBody(&cx,src,brace0+1,brace1-1,o);

  // and write them back out
  int multitarget = 0;
  {
    int colors = 0;
    for(int i=0;i<cx.ParamCount;i++)
      if(cx.Params[i].IsOut && SemanticIs(cx.Params[i].Sem,"COLOR"))
        colors++;
    multitarget = (colors>1);
  }
  o->Add("\n");
  for(int i=0;i<cx.ParamCount;i++)
  {
    Param *pa = &cx.Params[i];
    if(!pa->IsOut) continue;
    int nc = TypeComponents(pa->Type);
    const char *pad = (nc==1) ? ",0.0,0.0,0.0" : (nc==2) ? ",0.0,0.0" : (nc==3) ? ",0.0" : "";

    if(isvertex)
    {
      if(SemanticIs(pa->Sem,"POSITION"))
        o->AddF("  gl_Position=%s;\n",pa->Name);
      else
      {
        char vn[64]; SemanticVaryingName(pa->Sem,vn);
        o->AddF("  v_%s=vec4(%s%s);\n",vn,pa->Name,pad);
      }
    }
    else
    {
      if(SemanticIs(pa->Sem,"DEPTH"))
        o->AddF("  gl_FragDepth=float(%s);\n",pa->Name);
      else if(SemanticIs(pa->Sem,"COLOR") && SemanticIndex(pa->Sem)==0 && !multitarget)
        // A single output has to go to gl_FragColor: writing gl_FragData[0]
        // instead compiles and links fine here, but the driver never lands the
        // fragment in the default framebuffer.
        o->AddF("  gl_FragColor=vec4(%s%s);\n",pa->Name,pad);
      else if(SemanticIs(pa->Sem,"COLOR"))
        o->AddF("  gl_FragData[%d]=vec4(%s%s);\n",SemanticIndex(pa->Sem),pa->Name,pad);
    }
  }
  o->Add("}\n");

  free(work);

  if(cx.Failed)
  {
    info->Source.Exit();
    return 0;
  }
  return 1;
}

void sGlslFreeShaderInfo(sGlslShaderInfo *info)
{
  info->Source.Exit();
}

/****************************************************************************/
