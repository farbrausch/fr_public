/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_UTIL_SCANNER_HPP
#define FILE_ALTONA2_LIBS_UTIL_SCANNER_HPP

#include "Altona2/Libs/Base/Base.hpp"

namespace Altona2 {

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

struct sScannerLoc
{
    sPoolString File;
    int Line;
};

class sScanner
{
private:
    enum ScannerConfig                          // all units in characters, not bytes
    {
        BufferSize = 128*1024,                  // size of scanbuffer lookahead
        LineSize = 32*1024,                     // size of line (so much must be in scanbuffer before each token)
        Alignment = 64,                         // when copying around scanbuffer remainder, use this alignment
    };

    class sScannerSource
    {
    public:
        sScannerSource();
        virtual ~sScannerSource() {}            // destructor has to be virtual
        virtual bool Refill() { return 1; }     // call this every linefeed (at least)
        const char *ScanPtr;                    // *ScanPtr++ to scan forward
        int Line;                               // line reference for error messages
        int BeginOfFile;                        // beginning of line, may be plus some whitespace or comments
        sPoolString Filename;                   // file reference for error messages
    };

    class sScannerSourceText : public sScannerSource
    {
        const char *Start;
        bool DeleteMe;
    public:
        sScannerSourceText(const char *buffer,bool deleteme=0);
        ~sScannerSourceText();
    };

    sString<LineSize> TempString;

    struct ScannerToken
    {
        int Token;
        uptr Length;
        sPoolString Name;
        bool operator == (const ScannerToken &tok) const { return (Token==tok.Token); }
    };

    sArray<ScannerToken> TokensSym;             // symbol tokens: << != && ...
    sArray<ScannerToken> TokensName;            // name tokens: do for while ...

    sScannerSource *Stream;                     // current stream
    sArray<sScannerSource *> Includes;          // include stack

    int Flags;

    void Stop();                                        // prepares starting scanner. resets current token and input stream
    void SortTokens();
    void LogMsg(const sScannerLoc &loc,const char *name,const char *severity);

public:

    sScanner();
    ~sScanner();
    void Init(int flags);                               // full initialisation, delete all tokens
    bool StartFile(const char *filename);               // start parsing
    void Start(const char *text);
    bool IncludeFile(const char *filename);             // include file, no errors generated if file not found
    void AddToken(const char *name,int token);          // add a single token
    void AddTokens(const char *SingleCharacterTokens);  // like AddTokens(L"+-*/")
    void RemoveToken(const char *name);                 // remove a single token
    void AddDefaultTokens();                            // add a useful set of default tokens, see below
    void SetFilename(const char *f) { Stream->Filename = f; }
    void SetLine(int n) { Stream->Line = n; }
    void SetLoc(const sScannerLoc &loc) { TokenLoc = LastLoc = loc; if(Stream) { Stream->Line = loc.Line; Stream->Filename = loc.File; } }
    const char *GetFilename() { return Stream->Filename; }
    int GetLine() { return Stream->Line; }
    sScannerLoc GetLoc() { if(!Stream) return TokenLoc; sScannerLoc loc; loc.File = Stream->Filename; loc.Line = Stream->Line; return loc; }

    // primaray scanning and token status

    int Scan();                    // Consume current token and set values for next token

    int Token;
    int ValI;
    float ValF;
    sPoolString Name;
    sPoolString String;
    sString<256> ValueString;       // exact character representation of sTOK_INT or sTOK_FLOAT
    sString<256> ValueSuffix;       // optional suffix after number, like in "10u" or "40ull"
    int TokenLine;                  // line of token start
    const char *TokenFile;          // file of token start
    const char *TokenScan;          // scan pointer at token start. for replay after include

    sScannerLoc TokenLoc;           // location of current token
    sScannerLoc LastLoc;            // location of last token

    // these functions scan a token and issue an error if that was not found

    void Match(int token);
    void MatchName(const char *);
    void MatchString(const char *);
    void MatchInt(int n);

    int ScanInt();
    float ScanFloat();
    int ScanChar();
    bool ScanName(sPoolString &);
    bool ScanString(sPoolString &);
    bool ScanNameOrString(sPoolString &);
    sPoolString ScanName();
    sPoolString ScanString();
    sPoolString ScanNameOrString();

    bool ScanName(const sStringDesc &);
    bool ScanString(const sStringDesc &);
    bool ScanNameOrString(const sStringDesc &);


    // dirty scanning

    bool ScanRaw(sPoolString &, int opentoken, int closetoken);
    bool ScanRaw(sTextLog &, int opentoken, int closetoken);
    bool ScanLine(sTextLog &);

    char DirtyPeek();                    // what is the next character, skipping comment and whitespace?
    const char * GetScanPosition () { return Stream->ScanPtr; }

    // these function check if a token was found, consume it if true, and do nothing if false

    bool IfToken(int tok);
    bool IfName(const sPoolString name);
    bool IfString(const sPoolString name);

    // Errors

    void Error0(const char *error);
    void Warn0(const char *error);
    void Note0(const char *error);
    void Error0(const sScannerLoc &loc,const char *error);
    void Warn0(const sScannerLoc &loc,const char *error);
    void Note0(const sScannerLoc &loc,const char *error);

    sPRINTING0(Error,sFormatStringBuffer buf=sFormatStringBase(TempString,format);buf,Error0(TempString);)
    sPRINTING0(Warn,sFormatStringBuffer buf=sFormatStringBase(TempString,format);buf,Warn0(TempString);)
    sPRINTING0(Note,sFormatStringBuffer buf=sFormatStringBase(TempString,format);buf,Note0(TempString);)
    sPRINTING1(Error,sFormatStringBuffer buf=sFormatStringBase(TempString,format);buf,Error0(arg1,TempString);,const sScannerLoc &)
    sPRINTING1(Warn,sFormatStringBuffer buf=sFormatStringBase(TempString,format);buf,Warn0(arg1,TempString);,const sScannerLoc &)
    sPRINTING1(Note,sFormatStringBuffer buf=sFormatStringBase(TempString,format);buf,Note0(arg1,TempString);,const sScannerLoc &)

    int Notes;
    int Warnings;
    int Errors;
    sTextLog ErrorLog;

    void (*ErrorCallback)(void *ptr,const char *file,int line,const char *msg);
    void *ErrorCallbackPtr;

    // debugging

    void PrintToken(sTextLog &tb);
};

enum sScannerTokens
{
    // builtin tokens

    sTOK_End        = 0,            // end of file
    sTOK_Error      = 1,            // unknown character
    sTOK_Int        = 2,            // 123
    sTOK_Float      = 3,            // 123.45
    sTOK_Name       = 4,            // bla
    sTOK_String     = 5,            // "bla"
    sTOK_Char       = 6,            // 'a'
    sTOK_Newline    = 7,            // newline token (one or many), if sSF_NEWLINE is set
    sTOK_HashColor  = 8,            // use with sSF_HashColors

    // added with DefaultTokens()

    sTOK_ShiftL     = 16,           // <<
    sTOK_ShiftR     = 17,           // >>
    sTOK_EQ         = 18,           // ==
    sTOK_NE         = 19,           // !=
    sTOK_LE         = 20,           // <=
    sTOK_GE         = 21,           // >=
    sTOK_Ellipses   = 22,           // ..
    sTOK_Scope      = 23,           // ::
    sTOK_DoubleAnd  = 24,           // &&
    sTOK_DoubleOr   = 25,           // ||

    // user tokens start here
    // DefaultTokens add single character tokens for: "+-*/%^~,.;:!?=<>()[]{}"

    sTOK_User       = 32,
};

enum sScannerFlags
{
    sSF_Newline       = 0x0001,     // whitespace sequence with at least one newline generates a token
    sSF_CppComment    = 0x0002,     // accept C / C++ comments
    sSF_SemiComment   = 0x0004,     // accept ';' comments
    sSF_NumberComment = 0x0008,     // accept '#' comments
    sSF_EscapeCodes   = 0x0010,     // in strigns and char literals '\\', '\"', '\n', ...
    sSF_Umlauts       = 0x0020,     // allow character 0xa0..0xff in symbol names
    sSF_TypedNumbers  = 0x0040,     // allow 1.5f, 1.5d, 1.5h, 2346UL, ... use ValueString to get it
    sSF_MergeStrings  = 0x0080,     // merge two sTOK_STRING when using ScanString().
    sSF_Quiet         = 0x0100,     // do not print errors automatically. just log
    sSF_Cpp           = 0x0200,     // c preprocessor #line and #error
    sSF_HashColors    = 0x0400,     // html-style colors with #ffffff
};

/****************************************************************************/
}
#endif  // FILE_ALTONA2_LIBS_UTIL_SCANNER_HPP
