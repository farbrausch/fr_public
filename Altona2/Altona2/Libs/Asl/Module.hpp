/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_ASL_MODULE_HPP
#define FILE_ALTONA2_LIBS_ASL_MODULE_HPP

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Asl/Tree.hpp"
#include "Altona2/Libs/Asl/Asl.hpp"
#include "Altona2/Libs/Util/Scanner.hpp"

namespace Altona2 {
namespace Asl {
using namespace Altona2;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

enum sModulePatchKind
{
    sMPK_ConstOffset,
    sMPK_VarSlot,
    sMPK_Import,
    sMPK_Export,
};

struct sModulePatch
{
    sModulePatchKind Kind;
    sInject *Inject;                // sMPK_ConstOffset
    sMember *FirstMember;           // sMPK_ConstOffset
    sVariable *Variable;            // sMPK_VarSlot
    int *Patch;                     // sMPK_ConstOffset, sMPK_ConstOffset

    sPoolString Name;               // sMPK_Import
    int Index;                      // sMPK_Import
};

struct sModuleUse
{
    sModule *Module;
    sPoolArray<sModulePatch *> Patches;
    sPoolArray<sCondition> Conditions;
    int Index;
};

class sModules
{
    sModuleUse *CurrentUse;

    sPoolArray<sModuleUse *> Used;
    void AddDepends(sModule *mod);
    sModuleUse *NewModule(sModule *mod);
public:
    sModules();
    ~sModules();

    void Load(sTree *,sScanner *);
    void UseModule(sPoolString name);
    void UseModule(sModule *mod);
    void CBPatch(sPoolString name,int *patch);
    void CBPatch(sInject *inject,int *patch);       // patch is in Float4-units (16 bytes)
    void VarPatch(sPoolString name,int *patch);
    void VarPatch(sVariable *var,int *patch);       // patch is in slot units
    void SetCondition(sPoolString name);
    void SetCondition(sPoolString name,sPoolString member);
    void SetCondition(sPoolString name,int value);
    sVariable *Export(sPoolString name) { return 0; }
    void Import(sPoolString name,int index);
    bool Transform();
    void Unload();

    sTree *Tree;
    sScanner *Scan;
    int Index;
};

/****************************************************************************/

class sAslHelper
{
    sAdapter *Adapter;
    Document Doc;
    sTextLog ErrorLog;
    sTextLog HeaderLog;
    Material *Aslm;

    bool Ok;
    bool Compiled;

    void Compile(int shadermask);
public:
    sAslHelper(sAdapter *ada);
    ~sAslHelper();

    void AddSource(const char *source,const char *filename);
    void Parse(const char *material);
    sTextLog Source;

    sModules Mod;
    void UseModule(sPoolString name);
    void CBPatch(sPoolString name,int *patch);
    void VarPatch(sPoolString name,int *patch);
    void SetCondition(sPoolString name);
    void SetCondition(sPoolString name,sPoolString member);
    void SetCondition(sPoolString name,int value);
    void Import(sPoolString name,int index);

    void Transform();
    void GetTypeVectorSize(sPoolString name,int &size);

    sShader *CompileShader(sShaderTypeEnum shader);
    sMaterial *CompileMtrl(int shadermask);

    bool IsOk() { return Ok; }
    void SetError() { Ok = false; }
    const char *GetLog();
};


/****************************************************************************/
}}
#endif  // FILE_ALTONA2_LIBS_ASL_MODULE_HPP

