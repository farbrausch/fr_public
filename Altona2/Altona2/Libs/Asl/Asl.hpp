/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_ASL_ASL_HPP
#define FILE_ALTONA2_LIBS_ASL_ASL_HPP

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Util/Scanner.hpp"

#include "Tree.hpp"
#include "Parser.hpp"

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

namespace Altona2 {
namespace Asl {
using namespace Altona2;

class Material
{
    sScanner *Scan;
    const char *BaseTypeInC(sType *type);
    sTarget Target;
public:
    Material(sScanner *scan);
    ~Material();

    bool Virtual;
    sPoolString Name;
    sTree *Tree;

    Material *Clone();
    int GetShaderMask();
    bool ConfigureModules(int var,sTarget tar);
    bool Transform();
    bool WriteHeader(sTextLog *log);
    bool GetTypeVectorSize(sPoolString name,int &size);
    sShaderBinary *CompileBinary(sShaderTypeEnum shader,sTextLog *log,sTarget tar);
    sMaterial *CompileMaterial(sAdapter *ada);
    sShader *Compile(sAdapter *ada,sTextLog *log,sShaderTypeEnum shader);
    sShader *CompileVS(sAdapter *ada,sTextLog *log);
    sShader *CompilePS(sAdapter *ada,sTextLog *log);
    sShader *CompileCS(sAdapter *ada,sTextLog *log);
};

class Document
{
    sParse *Parser;
    sArray<Material *> Materials;
    void Parse();
public:
    Document();
    ~Document();

    sScanner *Scan;
    void ParseText(const char *source,const char *file="memory",int line=0);
    void ParseFile(const char *filename);
    Material *FindMaterial(sPoolString name);

    static sTarget GetDefaultTarget();
};

/****************************************************************************/
}}
#endif  // FILE_ALTONA2_LIBS_ASL_ASL_HPP

