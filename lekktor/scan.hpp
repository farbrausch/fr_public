// This file is distributed under a BSD license. See LICENSE.txt for details.

#pragma once

#include "_types.hpp"

/****************************************************************************/

void StartScan(sChar *txt);
sInt Scan();
sInt Peek();
void PrintToken();
void LoadTypeList(sChar *txt);
void ScanSafe();
void ScanRestore();
void InlineAssembly();
void AddType(sChar *name);

/****************************************************************************/

extern sInt AToken;
extern sInt Token;
extern sChar Value[256];

#define TOK_EOF       0           // EOF, will loop
#define TOK_NAME      1           // symbolic name (-azAZ09)
#define TOK_VALUE     2           // int, float, string, literal
#define TOK_KEYWORD   3           // any other sequence of characters
#define TOK_PRE       4           // preprocessor directive
#define TOK_ERROR     5           // error condition
#define TOK_TYPE      6           // name that is a type (by typelist)
//#define TOK_LABEL     7           // name:

#define TOK_ASSADD    0x80
#define TOK_ASSSUB    0x81
#define TOK_ASSMUL    0x82
#define TOK_ASSDIV    0x83
#define TOK_ASSMOD    0x84
#define TOK_ASSEOR    0x85
#define TOK_ASSAND    0x86
#define TOK_ASSOR     0x87
#define TOK_ASSRIGHT  0x88
#define TOK_ASSLEFT   0x89
#define TOK_ELIPSIS   0x8a
#define TOK_SCOPE     0x8b

#define TOK_INC       0x90
#define TOK_DEC       0x91
#define TOK_GE        0x92
#define TOK_LE        0x93
#define TOK_EQ        0x94
#define TOK_NE        0x95
#define TOK_RIGHT     0x96
#define TOK_LEFT      0x97
#define TOK_ANDAND    0x98
#define TOK_OROR      0x99
#define TOK_PTR       0x9a  
#define TOK_COMMA     0x9c
#define TOK_DOT       0x9d
#define TOK_SEMI      0x9e
#define TOK_COLON     0x9f

#define TOK_ADD       0xa0
#define TOK_SUB       0xa1
#define TOK_MUL       0xa2
#define TOK_DIV       0xa3
#define TOK_MOD       0xa4
#define TOK_EOR       0xa5
#define TOK_AND       0xa6
#define TOK_OR        0xa7
#define TOK_NOT       0xa8
#define TOK_NEG       0xa9
#define TOK_ASSIGN    0xaa

#define TOK_BOPEN     0xac
#define TOK_BCLOSE    0xad
#define TOK_OPEN      0xae
#define TOK_CLOSE     0xaf
#define TOK_SOPEN     0xb0
#define TOK_SCLOSE    0xb1
#define TOK_LT        0xb2
#define TOK_GT        0xb3
#define TOK_COND      0xb4


#define TOK_WHILE     0xc0
#define TOK_FOR       0xc1
#define TOK_IF        0xc2
#define TOK_ELSE      0xc3
#define TOK_SWITCH    0xc4
#define TOK_CASE      0xc5
#define TOK_RETURN    0xc6
#define TOK_NEW       0xc7
#define TOK_DELETEA   0xc8
#define TOK_DELETE    0xc9
#define TOK_BREAK     0xca
#define TOK_STATIC    0xcb
#define TOK_EXTERN    0xcc
#define TOK_AUTO      0xcd
#define TOK_REGISTER  0xce
#define TOK_INT       0xcf
#define TOK_FLOAT     0xd0
#define TOK_LONG      0xd1
#define TOK_SHORT     0xd2
#define TOK_CHAR      0xd3
#define TOK_DOUBLE    0xd4
#define TOK_CONST     0xd5
#define TOK_ASM       0xd6
#define TOK_STDCALL   0xd7
#define TOK_DO        0xd8
#define TOK_SIZEOF    0xd9
#define TOK_GOTO      0xda
#define TOK_STRUCT    0xdb
#define TOK_CLASS     0xdc
#define TOK_UNION     0xdd
#define TOK_PUBLIC    0xde
#define TOK_PROTECTED 0xdf
#define TOK_PRIVATE   0xe0
#define TOK_DEFAULT   0xe1
#define TOK_CDECL     0xe2
#define TOK_FASTCALL  0xe3
#define TOK_WINAPI    0xe4
#define TOK_TYPEDEF   0xe5
#define TOK_VOLATILE  0xe6
#define TOK_SIGNED    0xe7
#define TOK_UNSIGNED  0xe8
#define TOK_OPERATOR  0xe9
#define TOK_VCTRY     0xea
#define TOK_VCEXCEPT  0xeb
#define TOK_APIENTRY  0xec
#define TOK_CALLBACK  0xed
#define TOK_ENUM      0xee
#define TOK_NAMESPACE 0xef
#define TOK_INLINE    0xf0
#define TOK_FINLINE   0xf1
#define TOK_USING     0xf2
