/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_TOOLS_ASC_DOC_HPP
#define FILE_ALTONA2_TOOLS_ASC_DOC_HPP

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Util/Scanner.hpp"

using namespace Altona2;

/****************************************************************************/

class wMaterial;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

struct wPermuteOption
{
    sPoolString Name;
    sInt Value;                     // the unshifted value
};

class wPermute
{
public:
    wPermute();
    sPoolString Name;
    sArray<wPermuteOption> Options; // there may be zero options
    sInt Count;                     // how many options? this will be 2 if no options are given
    sInt Mask;                      // mask after shifting (the mask is not shifted)
    sInt Shift;                     // bit position
};

enum wPredicateOp
{
    wPO_Error = 0,
    wPO_Permute,                    // link to wPermute
    wPO_Literal,                    // for comparing and logicals
    wPO_And,
    wPO_Or,
    wPO_Not,
    wPO_Equal,
    wPO_NotEqual,
};

class wPredicate
{
public:
    wPredicate();
    wPredicate(wPredicateOp op,wPredicate *a_,wPredicate *b_=0);
    wPredicate(sInt lit);
    wPredicate(wPermute *perm);

    wPredicateOp Op;
    wPredicate *a;
    wPredicate *b;
    wPermute *Permute;              // for wPO_Permute
    sInt Literal;                   // for wPO_Literal

    sInt Evaluate(wMaterial *mtrl,sU32 permutaton);
    sU32 UsedPermutations();
};

/****************************************************************************/

enum wAscTypeBase                    // the enum values are hard-coded in various tables
{
    wTB_Error = 0,
    wTB_Bool = 1,
    wTB_Int = 2,
    wTB_UInt = 3,
    wTB_Float = 4,
    wTB_Double = 5,
    wTB_Min16Float = 6,
    wTB_Min10Float = 7,
    wTB_Min16Int = 8,
    wTB_Min12Int = 9,
    wTB_Min16UInt = 10,
};

class wAscType
{
public:
    wAscType();

    wAscTypeBase Base;              // what type?
    sInt Columns;                   // 0 for scalar, 1..4 for vectors and matrices
    sInt Rows;                      // 0 for scalar and vectors, 1..4 for matrices
    sInt Array;                     // 0 for no array, 1..n for arrays
    sBool ColumnMajor;              // default is row major

    sInt GetWordSize();             // number of words (4 byte units) occupied by this
    sInt GetRegisterSize();         // number of registers occupied by this
};

enum wAscMemberFlag
{
    wMF_Extern = 0x0001,
    wMF_NoInterpolation = 0x0002,
    wMF_Precise = 0x0004,
    wMF_Shared = 0x0008,
    wMF_GroupShared = 0x0010,
    wMF_Static = 0x0020,
    wMF_Uniform = 0x0040,
    wMF_Volatile = 0x0080,
};

class wAscMember
{
public:
    wAscMember();

    sPoolString Name;
    sPoolString Binding;
    wAscType *Type;
    wPredicate *Predicate;
    sInt Flags;
};

/****************************************************************************/

enum wAscBufferKind
{
    wBK_Error = 0,
    wBK_Struct,
    wBK_Constants,
};


class wAscBuffer
{
public:
    wAscBuffer();

    wAscBufferKind Kind;
    sPoolString Name;
    sShaderTypeEnum Shader;
    sInt Slot;
    sInt Register;

    sArray<wAscMember *> Members;
};

/****************************************************************************/

class wShader
{
public:
    wShader();
    wShader(sPtr size,const sU8 *data);
    ~wShader();
    sU8 *Data;
    sPtr Size;
    sChecksumMD5 Checksum;
};

/****************************************************************************/

struct wSource
{
    wSource() { Line = 0; }
    sInt Line;
    sPoolString Source;
};

enum wSourceLanguage
{
    wSL_Hlsl3 = 0,
    wSL_Hlsl5,
    wSL_Glsl1,
    wSL_Max,
};

class wMaterial
{
public:
    wMaterial();

    wSource Sources[wSL_Max][sST_Max];

    sPoolString Name;
    sArray<wPermute *> Permutes;
    sArray<wAscBuffer *> Buffers;
    sArray<wShader *> Shaders[sST_Max];
    sInt PermuteShift;

    sBool PermutationIsValid(sU32 p);
};

/****************************************************************************/

enum wTokenEnum
{
    TokBreak = 256,
    TokCase,
    TokColumnMajor,
    TokConst,
    TokContinue,
    TokDo,
    TokDefault,
    TokDiscard,
    TokElse,
    TokExtern,
    TokFalse,
    TokFor,
    TokIf,
    TokIn,
    TokInOut,
    TokMatrix,
    TokNull,
    TokOut,
    TokReturn,
    TokRegister,
    TokRowMajor,
    TokStatic,
    TokStruct,
    TokSwitch,
    TokTypedef,
    TokUniform,
    TokVector,
    TokVolatile,
    TokWhile,

    TokPif,
    TokPelse,

    TokInc,
    TokDec,
};

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class wDocument
{
    sBool EnableDump;
    sInt Platform;
    const sChar *InputFilename;
public:
    wDocument();
    ~wDocument();

    sBool ErrorFlag;
    sBool Bits64;

    // primary interface

    sBool Parse(const sChar *inputfilename,sInt platform,sInt dump);
    const sChar *GetAsm();
    const sChar *GetCpp();
    const sChar *GetHpp(const sChar *filename);
    const sChar *GetDump();

    // memory management: have list of all created objects, so we can easily dispose of them

    sArray<wPermute *> Permutes;
    sArray<wPredicate *> Predicates;
    sArray<wAscType *> AscTypes;
    sArray<wAscMember *> AscMembers;
    sArray<wAscBuffer *> AscBuffers;
    sArray<wMaterial *> Materials;
    sArray<wShader *> Shaders;

private:
    // shared resources

    sScanner *Scan;
    sTextBuffer Out;
    sTextBuffer OutAlt;             // all the discarded pif/pelse goes here
    sTextBuffer *OutPtr;            // points to either Out or OutAlt
    sTextBuffer Dump;

    wMaterial *Material;            // current material
    wPredicate *Predicate;          // current predication while parsing.
    sU32 Permutation;               // current permutation
    sArray<sPoolString> Typenames;  // so we know user defined types from variables
    sArray<sPoolString> NameSpace;

    wPredicate *Negate(wPredicate *a);
    wPredicate *Combine(wPredicate *a,wPredicate *b);
    wPredicate *_PredValue();
    wPredicate *_PredOr();
    wPredicate *_PredAnd();
    wPredicate *_PredCmp();
    wPredicate *_Predicate();

    // asc parsing

    void AscShader(sShaderTypeEnum st);
    void AscPermute();
    void AscMemberSingle(wAscBuffer *,sBool allowpif);
    void AscMemberBlock(wAscBuffer *,sBool allowpif);
    void AscCBuffer();
    void AscStruct();

    void AscStmt();
    void AscBlock();
    void AscMaterial();
    sBool AscParse(const sChar *filename);

    // hlsl parsing

    sBool HlslIsStorageClass();
    sBool HlslIsTypeModifer();
    sBool HlslType();
    void HlslValue();
    void HlslExpr();
    void HlslStmt(sInt indent);
    void HlslBlock(sInt indent,sBool pif=0);
    void HlslTemplateArgs();
    void HlslDecl();
    void HlslGlobal();

    void HlslPrintMember(wAscMember *mem);
    sBool HlslParse(sInt stage,const sChar *source,sInt line);

    // glsl parsing

    struct GlslReplacement
    {
        sPoolString From;
        sPoolString To;
        void Set(sPoolString f,sPoolString t) { From=f; To=t; }
        void Set(sPoolString f,const sChar *t) { From=f; To=t; }
        void Set(const sChar *f,const sChar *t) { From=f; To=t; }
    };
    sArray<GlslReplacement> GlslReplacements;
    sInt GlslSamplerCount;

    sBool GlslIsTypeModifer();
    sBool GlslType();
    void GlslValue();
    void GlslExpr();
    void GlslStmt(sInt indent);
    void GlslBlock(sInt indent,sBool pif=0);
    void GlslDecl();
    void GlslGlobal();

    void GlslPrintType(wAscType *type);
    sBool GlslParse(sInt stage,const sChar *source,sInt line);

    // output helpers

    void PrintHeader(const sChar *prefix = "");
    void GetCode(sBool cpp);
    wShader *AddShader(wShader *sh);
};

extern class wDocument *Doc;

/****************************************************************************/

#endif  // FILE_ALTONA2_TOOLS_ASC_DOC_HPP

