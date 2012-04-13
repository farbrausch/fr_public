// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __SHADERCOMPILE_HPP_
#define __SHADERCOMPILE_HPP_

#include "_types.hpp"

/****************************************************************************/

#if !sPLAYER

class sShaderCompiler
{
  enum { MAXTOKEN = 63 };

  // scanner
  const sChar *ScanPtr;
  sChar Word[MAXTOKEN+1];

  sInt Token;
  sInt SubCode;
  const sChar *SubScanPtr;
  sChar TokValue[MAXTOKEN+1];

  void WordScan();
  sInt Scan();

  void SkipRestOfLine();
  void ExpectEndOfLine();
  void ExpectEndOfWord();
  void ExpectEndOfWordAndLine();
  
  sBool ExpectWord(const sChar *word);
  sBool ExpectSubWord(const sChar *subWord);
  void VerifyToken(sInt token);

  // error messages
  sText *Errors;
  sInt ErrorCount;
  sInt ErrorLine;
  sInt ErrorLineNum;
  sInt Line;
  sBool IsNewLine;
  const sChar *FileName;

  void Error(const sChar *error,...);

  // symbol tables
  struct Symbol;
  class SymbolTable;

  SymbolTable *Identifiers;
  SymbolTable *Flags;

  // identifier management
  sInt NumTempRegs;

  void OpenScope();
  void EndScope();

  sU32 FindSymRegister(const sChar *name);
  void AddTempRegister(const sChar *name);
  void AddPredefinedSymbols();

  // flag management
  sInt NumFlags;
  
  void AddFlag(const sChar *name,sInt dim);

  // control structures
  struct Control;
  sArray<Control> ControlStack;

  void PushControl(sInt type,sInt param);
  void PopControl(sInt type);
  Control *TopControl();
  void ElseControl(sBool set);

  // output
  sArray<sU32> Output;

  struct IndexedPatch;
  sArray<IndexedPatch> IndexedPatches;
  sU32 DestModifier;

  void Emit(sU32 token);
  void EmitIndexBackpatches();

  // shader type/version number
  sInt ShaderType;  // 0=vertex 1=pixel
  sInt ShaderVer;   // shader version number

  // parsing helper functions
  sInt CharInList(sChar ch,const sChar *list);
  sInt StringInPrefixList(const sChar *str,const sChar **prefixList,const sChar *&rest);
  sInt MatchStringList(const sChar **stringList);
  sInt DecodeInt(const sChar *what);

  // subword-level parsing routines
  sF32 ParseFloat();
  sInt ParseWriteMask();
  sU32 ParseSwizzle();
  sU32 ParseRegNum();
  sU32 ParseFlagIdentifier(sBool expectIt);

  // word-level parsing routines
  sU32 ParseDestReg(sBool writeMask);
  sU32 ParseSourceReg();
  sInt ParseIndexReg();
  void ParseRange(sInt &start,sInt &end);
  sU32 ParseIfCondition();

  // line-level parsing routines
  void ParseVersion();
  void ParseDcl();
  void ParseDef();
  void ParseTemp();
  void ParseAlias();
  void ParseFor();
  void ParseArith();
  sBool ParseIndexInstr();
  void ParseVMov();

  // block-level parsing routines
  void ParseFlags();
  void ParseCode();

public:
  sShaderCompiler();
  ~sShaderCompiler();

  // frontend
  sBool Compile(const sChar *source,const sChar *fileName,sText *errors);
  
  const sU32 *GetShader() const;
  sU32 *GetShaderCopy() const;
  sU32 GetShaderSize() const;
};

/****************************************************************************/

#endif
#endif
