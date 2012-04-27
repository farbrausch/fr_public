/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include <base/types2.hpp>
#include <util/scanner.hpp>
#include "shadercomp/asc_doc.hpp"

#define VERSION 1
#define REVISION 63

/****************************************************************************/

// asc shader styles...

class NewCode                     // 
{
public:
  NewCode();
  ~NewCode();

  sInt ShaderType;                // sSTF_???
  sU8 *DX9Code;
  sInt DX9Size;
  sU8 *DX11Code;
  sInt DX11Size; 
  sU8 *GLSLCode;                  // misused as CG right now
  sInt GLSLSize; 
};

class NewShader                   // shader permutations
{
public:
  NewShader();
  ~NewShader();

  sPoolString Name;               // name of shader
  sInt ShaderType;                // sSTF_PIXEL, sSTF_VERTEX, ...

  sPoolString AscSource;          // main sourcecode
  sPoolString GlslSource;         // alternative glsl source
  sPoolString HlslSource;         // alternative hlsl source
  sPoolString CgSource;           // alternative cg source

  sPoolString Profile;            // shader profile string (vs_1_1)

  NewCode *ByteCode;              // no permutation: this code

  NewCode **Permutes;             // with permutation: these codes!
  sInt PermuteCount;              // size of array

  sInt CompileFlags;
};

enum ShaderEnum
{
  _VS_ = 0,
  _HS_,
  _DS_,
  _GS_,
  _PS_,
  _CS_,
  MAXSHADER,
};

class NewMaterial                 // a complete material
{
public:
  NewMaterial();
  ~NewMaterial();

  sPoolString Name;               // name of material
  sPoolString BaseName;           // baseclass, usually sMateral

  sPoolString Header;             // insert this into header
  sPoolString Code;               // insert this into cpp file
  sPoolString New;                // insert this into constructor
  sPoolString Prepare;            // insert this into prepare code

  NewShader *Shaders[MAXSHADER];

  sPoolString AscHeader;          // common asc code for all shaders
};

/****************************************************************************/

extern sBool Level11;
extern sBool PcOnly;

class Compiler
{
public:
  sTextBuffer HPP;
  sTextBuffer CPP;

  sScanner Scan;
  sPoolString Filename;

  ACDoc *ASC;                          // the asc compiler instance

  sArray<sPoolString> Includes;

  sArray<NewShader *> NewShaders;
  sArray<NewMaterial *> NewMtrls;
  sArray<NewMaterial *> ComputeShaders;

  sBool NoStrip;

  void _Code(sPoolString &code);
  void _CodeNoLine(sPoolString &code,sInt lineoffset);
  void _NewShaderCodeStatement(NewShader *ns);
  void _NewShaderCode(NewMaterial *nm,NewShader *ns);
  void _Material();
  void _Shader(NewMaterial *nm,sInt ascshaderkind,sInt altonashaderkind);
  void _Asc();
  void _Include();
  void _Global();

  void OutputShader(NewShader *ns);
  void OutputHPP();
  void OutputCPP();
  sBool Process(NewCode *code,NewShader *ns,sTextBuffer *errors);
  sBool Process(NewShader *ns);
  void Run(const sChar *filename);
};

/****************************************************************************/
