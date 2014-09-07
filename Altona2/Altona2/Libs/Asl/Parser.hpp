/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_ASL_PARSER_HPP
#define FILE_ALTONA2_LIBS_ASL_PARSER_HPP

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Asl/Tree.hpp"
#include "Altona2/Libs/Util/Scanner.hpp"

namespace Altona2 {
namespace Asl {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class sParse
{
    sScanner *Scan;

    bool IsType();
    sType *_Type();
    sLiteral *_Literal();
    sExpr *_Intrinsic(sIntrinsic *in);
    sExpr *_NameExpr();
    sExpr *_Value();
    sExpr *_ExprR(int level);
    sExpr *_Expr();
    sVariable *_Decl(bool initializer,bool local);
    sStmt *_Stmt();
    sStmt *_Block(bool swtch = false);
    sStmt *_BlockNoBrackets(bool swtch = false);
    void _Func();
    sStorage _Storage();
    sSemantic _Semantic();
    void _Members(sType *str,sInject *inj = 0,sModule *mod=0);
    void _GDecl(sTypeKind);
    void _Shader(int shader);
    void _ModuleShader(sModule *mod,int shader);
    void _ModuleDepends(sModule *mod,sExpr *cond);
    void _Module();
    void _VarBool();
    void _Var();
    void _VarUse();
    void _Global();

    sTree *Tree;
    sStmt *Scope;
public:
    sParse();
    ~sParse();

    void InitScanner(sScanner *scan);
    bool Parse(sScanner *scan,bool needEof,sTree *tree);
    sScanner *GetScanner() { return Scan; }
};

/****************************************************************************/
} // namespace Asl
} // namespace Altona2
#endif  // FILE_ALTONA2_LIBS_ASL_PARSER_HPP

