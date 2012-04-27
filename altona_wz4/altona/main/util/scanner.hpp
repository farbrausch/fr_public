/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef HEADER_ALTONA_UTIL_SCANNER
#define HEADER_ALTONA_UTIL_SCANNER

#ifndef __GNUC__
#pragma once
#endif

#include "base/types2.hpp"
#include "base/system.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Scanner - good for recursive descent c-like languages              ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***   Init()                                                             ***/
/***   AddToken()                                                         ***/
/***   AddToken()                                                         ***/
/***   AddToken()                                                         ***/
/***   Start()                                                            ***/
/***                                                                      ***/
/***   the first symbol is scanned ahead, that's why you need to add      ***/
/***   tokens first.                                                      ***/
/***                                                                      ***/
/***   you can investigate the Token, ValX, Name, and String variables    ***/
/***   or call the ScanXXX() functions directly. usually you will find    ***/
/***   yourself calling ScanXXX() and Match() functions and sometimes     ***/
/***   looking at Token to decide what to do next.                        ***/
/***                                                                      ***/
/***   i suggest you use ascii-values for one-letter tokens, like:        ***/
/***   Scan.AddToken(L"{",'{');                                           ***/
/***   Scan.AddToken(L"}",'}');                                           ***/
/***   Scan.AddToken(L";",';');                                           ***/
/***                                                                      ***/
/****************************************************************************/

class sScannerSource
{
public:
  sScannerSource();
  virtual ~sScannerSource() {}    // destructor has to be virtual
  virtual sBool Refill() { return 1; }  // call this every linefeed (at least)
  const sChar *ScanPtr;           // *ScanPtr++ to scan forward
  sInt Line;                      // line reference for error messages
  sInt BeginOfFile;               // beginning of line, may be plus some whitespace or comments
  const sChar *Filename;          // file reference for error messages
};

class sScannerSourceText : public sScannerSource
{
  const sChar *Start;
  sBool DeleteMe;
public:
  sScannerSourceText(const sChar *buffer,sBool deleteme=0);
  ~sScannerSourceText();
};

class sScannerSourceFile : public sScannerSource
{
  sFile *File;
  sChar *ScanBuffer;                 // buffer for portion of file
  sInt UnicodeConversion;            // when loading from file: 1=ascii->unicode, 2=unicode byteswap
  sS64 CharsLeftInFile;
public:
  sScannerSourceFile();
  ~sScannerSourceFile();
  sBool Load(const sChar *filename);
  sBool Refill();
};

class sScanner
{
public:
  enum ScannerConfig              // all units in characters, not bytes
  {
    BufferSize = 128*1024,          // size of scanbuffer lookahead
    LineSize = 32*1024,            // size of line (so much must be in scanbuffer before each token)
    Alignment = 64,               // when copying around scanbuffer remainder, use this alignment
  };

private:
  void Stop();                      // prepares starting scanner. resets current token and input stream
  sString<LineSize> TempString;
  void SortTokens();
/*
  struct IncludeStackItem
  {
    File *File;
    sChar *ScanBuffer;                 // buffer for portion of file
    const sChar *ScanPtr;
    sS64 CharsLeftInFile;
    sInt Line;
    void Init(const sChar *text);
    sBool InitFile(const sChar *filename);
    void Exit();
  };
*/

public:
  struct ScannerToken
  {
    sInt Token;
    sInt Length;
    sPoolString Name;
    sInt operator == (const ScannerToken &tok) { return (Token==tok.Token); }
  };
  sArray<ScannerToken> TokensSym;                       // symbol tokens: << != && ...
  sArray<ScannerToken> TokensName;                      // name tokens: do for while ...

  sScanner();
  ~sScanner();
  void Init();                                          // full initialisation, delete all tokens
  sBool StartFile(const sChar *filename, sBool reporterror=sTRUE);  // start parsing
  void Start(const sChar *text);
  sBool IncludeFile(const sChar *filename);             // include file, no errors generated if file not found
  void AddToken(const sChar *name,sInt token);          // add a single token
  void AddTokens(const sChar *SingleCharacterTokens);   // like AddTokens(L"+-*/")
  void RemoveToken(const sChar *name);                  // remove a single token
  void DefaultTokens();                                 // add a useful set of default tokens, see below
  void SetFilename(const sChar *f) { Stream->Filename = f; }
  void SetLine(sInt n) { Stream->Line = n; }
  const sChar *GetFilename() { return Stream->Filename; }
  sInt GetLine() { return Stream->Line; }

  sInt Scan();
  sChar DirtyPeek();                    // what is the next character, skipping comment and whitespace?
  void Print(sTextBuffer &tb);
  sInt Error0(const sChar *error);      // always returns 0, so you can do   return Scan.Error("bla");
  sPRINTING0(Error,sFormatStringBuffer buf=sFormatStringBase(TempString,format);buf,Error0(TempString);)
  sInt Warn0(const sChar *error);      // always returns 0, so you can do   return Scan.Error("bla");
  sPRINTING0(Warn,sFormatStringBuffer buf=sFormatStringBase(TempString,format);buf,Warn0(TempString);)

  // stream status

  sScannerSource *Stream;                 // current stream
  sArray<sScannerSource *> Includes;      // include stack

  // token status

  sInt Token;
  sInt ValI;
  sF32 ValF;
  sPoolString Name;
  sPoolString String;
  sString<256> ValueString;       // exact character representation of sTOK_INT or sTOK_FLOAT
  sString<256> ValueSuffix;       // optional suffix after number, like in "10u" or "40ull"
  sInt TokenLine;                 // line of token start
  const sChar *TokenFile;         // file of token start
  const sChar *TokenScan;         // scan pointer at token start. for replay after include
  sString<1024> ErrorMsg;
  void (*ErrorCallback)(void *ptr,const sChar *file,sInt line,const sChar *msg);
  void *ErrorCallbackPtr;

  // scanning status

  sInt Errors;
  sInt Flags;

  // these functions scan a token and issue an error if that was not found

  sInt ScanInt();
  sF32 ScanFloat();
  sInt ScanChar();
  sBool ScanRaw(sPoolString &, sInt opentoken, sInt closetoken);
  sBool ScanRaw(sTextBuffer &, sInt opentoken, sInt closetoken);
  sBool ScanName(sPoolString &);
  sBool ScanString(sPoolString &);
  sBool ScanNameOrString(sPoolString &);
  sBool ScanLine(sTextBuffer &);
  void Match(sInt token);
  void MatchName(const sChar *);
  void MatchString(const sChar *);
  void MatchInt(sInt n);

  const sChar * GetScanPosition () { return Stream->ScanPtr; }

  // these function check if a token was found, consume it if true, and do nothing if false

  sBool IfToken(sInt tok);
  sBool IfName(const sPoolString name);
  sBool IfString(const sPoolString name);
};

enum sScannerTokens
{
  // builtin tokens

  sTOK_END        = 0,            // end of file
  sTOK_ERROR      = 1,            // unknown character
  sTOK_INT        = 2,            // 123
  sTOK_FLOAT      = 3,            // 123.45
  sTOK_NAME       = 4,            // bla
  sTOK_STRING     = 5,            // "bla"
  sTOK_CHAR       = 6,            // 'a'
  sTOK_NEWLINE    = 7,            // newline token (one or many), if sSF_NEWLINE is set

  // added with DefaultTokens()

  sTOK_SHIFTL     = 16,           // <<
  sTOK_SHIFTR     = 17,           // >>
  sTOK_EQ         = 18,           // ==
  sTOK_NE         = 19,           // !=
  sTOK_LE         = 20,           // <=
  sTOK_GE         = 21,           // >=
  sTOK_ELLIPSES   = 22,           // ..
  sTOK_SCOPE      = 23,           // ::

  // user tokens start here
  // DefaultTokens add single character tokens for: "+-*/%^~,.;:!?=<>()[]{}"

  sTOK_USER       = 32,
};

enum sScannerFlags
{
  sSF_NEWLINE       = 0x0001,     // whitespace sequence with at least one newline generates a token
  sSF_CPPCOMMENT    = 0x0002,     // accept C / C++ comments
  sSF_SEMICOMMENT   = 0x0004,     // accept ';' comments
  sSF_NUMBERCOMMENT = 0x0008,     // accept '#' comments
  sSF_ESCAPECODES   = 0x0010,     // in strigns and char literals '\\', '\"', '\n', ...
  sSF_UMLAUTS       = 0x0020,     // allow character 0xa0..0xff in symbol names
  sSF_TYPEDNUMBERS  = 0x0040,     // allow 1.5f, 1.5d, 1.5h, 2346UL, ... use ValueString to get it
  sSF_MERGESTRINGS  = 0x0080,     // merge two sTOK_STRING when using ScanString().
  sSF_QUIET         = 0x0100,     // do not print errors automatically
  sSF_CPP           = 0x0200,     // c preprocessor #line and #error
};

/****************************************************************************/
/***                                                                      ***/
/***   Scanner utils                                                      ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***   Some convenient helpers for common scanner use cases.              ***/
/***                                                                      ***/
/****************************************************************************/

namespace sScannerUtil
{
  // Automatically generate a fitting "unexpected X" error message, where X
  // is the current token.
  void Unexpected(sScanner *scan);

  // Tries to parse a string property like
  //   name = "value";
  // and returns true if the given property was found.
  sBool StringProp(sScanner *scan,const sChar *name,sPoolString &tgt);
  sBool StringProp(sScanner *scan,const sChar *name,const sStringDesc &tgt);

  // Int property
  sBool IntProp(sScanner *scan,const sChar *name,sInt &tgt);

  // Float property
  sBool FloatProp(sScanner *scan,const sChar *name,sF32 &tgt);

  // GUID property (name="{bla-bla-blah-blah}";)
  sBool GUIDProp(sScanner *scan,const sChar *name,sGUID &tgt);

  // Optional boolean property (set tgt to sTRUE if "name;" found)
  sBool OptionalBoolProp(sScanner *scan,const sChar *name,sBool &tgt);

  // Enum property (choices given as a pattern for sFindChoice)
  sBool EnumProp(sScanner *scan,const sChar *name,const sChar *choices,sInt &tgt);

  // Flags property (pattern like for sFindFlag)
  sBool FlagsProp(sScanner *scan,const sChar *name,const sChar *pattern,sInt &tgt);
}

/****************************************************************************/
/***                                                                      ***/
/***   Simple Regular Expression Evaluator                                ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***  * algorithm: explicitly tracking tokens through an NFA.             ***/
/***  * not a backtracking algorithm.                                     ***/
/***  * will return ALL possible matches, not just the first one          ***/
/***    this may actually not be what you want                            ***/
/***  * subset of pearl regex                                             ***/
/***                                                                      ***/
/***  features:                                                           ***/
/***                                                                      ***/
/***  * normal matching: abc \d \w \s \D \W \S xyz                        ***/
/***  * any: abc...xyz                                                    ***/
/***  * special escapes: \t \r \n                                         ***/
/***  * ranges: [xyz] [^xyz] [a-z] [\s\w\d]                               ***/
/***  * or: abc|xyz                                                       ***/
/***  * repeats: a* a+ a?                                                 ***/
/***  * groups: (xyz)*                                                    ***/
/***  * anchors: "^xxx$"                                                  ***/
/***                                                                      ***/
/***  limitations (common pearl features not implemented here):           ***/
/***                                                                      ***/
/***  * no inverted escapes in ranges: [/S/W/D]                           ***/
/***  * no word boundary: \b\B                                            ***/
/***  * no limited repeats: a{2,4}                                        ***/
/***  * no ranges with unicode chars or escaped chars                     ***/
/***  * no special handling for whitespace                                ***/
/***  * no multiline mode, although repeated matching should be fast      ***/
/***                                                                      ***/
/***  todo:                                                               ***/ 
/***                                                                      ***/
/***  * check greedyness. make ungreedy version.                          ***/
/***  * use sDList instead of sArray where possible                       ***/
/***                                                                      ***/
/****************************************************************************/

struct sRegexTrans;
struct sRegexState;
struct sRegexMarker;

/****************************************************************************/

class sRegex
{
  sArray<sRegexState *> Machine;
  sArray<sRegexMarker *> Markers;
  sArray<sRegexMarker *> OldMarkers;
  sArray<sRegexTrans *> Trans;
  sArray<sRegexMarker *> Success;

  sRegexState *Start,*End;
  sBool Valid;
  const sChar *Scan;
  sInt GroupId;
  const sChar *OriginalPattern;

  // create resources

  sRegexState *MakeState();
  sRegexMarker *MakeMarker();
  sRegexTrans *MakeTrans();
  sRegexTrans *MakeTrans(sRegexState *from,sRegexState *to);
  sRegexTrans *MakeTransCopy(sRegexTrans *src,sRegexState *from,sRegexState *to);

  // regex parser

  sRegexState *_Value(sRegexState *o);
  sRegexState *_Repeat(sRegexState *o);
  sRegexState *_Cat(sRegexState *o);
  sRegexState *_Expr(sRegexState *o);
  sBool PrepareAll(const sChar *expr);

  // state machine maintainance

  void Flush();
  void Validate();
  void RemoveEmptyTrans();
  void RemoveEmptyState();

public:

  // matching rhe regular expression

  sRegex();
  ~sRegex();
  sBool SetPattern(const sChar *expr); 
  sBool MatchPattern(const sChar *string);
  void DebugDump();

  // querying the captured strings.

  sInt GetMatchCount() { return Valid ? Success.GetCount() : 0; }
  sInt GetGroupCount() { return GroupId; }
  sBool GetGroup(sInt n,const sStringDesc &desc,sInt match=0);
};

/****************************************************************************/

// HEADER_ALTONA_UTIL_SCANNER
#endif
