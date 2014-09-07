/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/DxShaderCompiler.hpp"
#include "Altona2/Libs/Base/SystemPrivate.hpp"

#undef new
#include <d3d11.h>
#include <d3dcompiler.h>
#define new sDEFINE_NEW
#define NSIGHT 0

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

    typedef HRESULT (WINAPI *DXCompLibCompileType)(
        LPCVOID pSrcData,
        SIZE_T SrcDataSize,
        LPCSTR pSourceName,
        CONST D3D10_SHADER_MACRO *pDefines,
        LPD3D10INCLUDE pInclude,
        LPCSTR pEntrypoint,
        LPCSTR pTarget,
        UINT Flags1,
        UINT Flags2,
        LPD3D10BLOB *ppCode,
        LPD3D10BLOB *ppErrorMsgs
        );

    typedef HRESULT (WINAPI *DXCompLibDisassembleType)(
        LPCVOID pSrcData,
        SIZE_T SrcDataSize,
        UINT Flags,
        LPD3D10BLOB *szComments,
        LPD3D10BLOB *ppDisassembly
        );

    typedef HRESULT (WINAPI *DXCompLibStripShaderType)(
        LPCVOID pShaderBytecode,
        SIZE_T BytecodeLength,
        UINT uStripFlags,
        LPD3D10BLOB *ppStrippedBlob
        );


    /****************************************************************************/

    namespace Private
    {
        bool DXCompLibInit;
        HMODULE DXCompLib;
        DXCompLibCompileType DXCompLibCompile;
        DXCompLibDisassembleType DXCompLibDisassemble;
        DXCompLibStripShaderType DXCompLibStripShader;
    }

    using namespace Private;

    /****************************************************************************/


    class sD3DShaderCompiler : public sSubsystem
    {
    public:
        sD3DShaderCompiler() : sSubsystem("D3DCompiler",0x41) {}
        void Init()
        {
            DXCompLibInit = 0;
            DXCompLib = 0;
            DXCompLibCompile = 0;
            DXCompLibDisassemble = 0;
        }
        void Init2()
        {
            if(!DXCompLibInit)
            {
                DXCompLibInit = 1;
                DXCompLib = LoadLibraryW(L"D3DCompiler_47.dll");
                if(!DXCompLib)
                {
                    DXCompLib = LoadLibraryW(L"D3DCompiler_46.dll");
                    if(!DXCompLib)
                    {
                        DXCompLib = LoadLibraryW(L"D3DCompiler_43.dll");
                        if(!DXCompLib)
                        {
                            DXCompLib = LoadLibraryW(L"D3DCompiler_42.dll");
                            if(!DXCompLib)
                            {
                                sFatal("could not load D3DCompiler_XX.dll, trying versions 47, 46, 43 and 42");
                            }
                        }
                    }
                }
                DXCompLibCompile = (DXCompLibCompileType) GetProcAddress(DXCompLib,"D3DCompile");
                sASSERT(DXCompLibCompile);
                DXCompLibDisassemble = (DXCompLibDisassembleType) GetProcAddress(DXCompLib,"D3DDisassemble");
                sASSERT(DXCompLibDisassemble);
                DXCompLibStripShader = (DXCompLibStripShaderType) GetProcAddress(DXCompLib,"D3DStripShader");
                sASSERT(DXCompLibStripShader);
            }
        }
        void Exit()
        {
            if(DXCompLibInit)
            {
                DXCompLibInit = 0;
                FreeLibrary(DXCompLib);
                DXCompLib = 0;
                DXCompLibCompile = 0;
                DXCompLibDisassemble = 0;
            }
        }
    } sD3DShaderCompiler_;

    /****************************************************************************/

    sShaderBinary *sCompileShaderDX(sShaderTypeEnum type,const char *profile,const char *source,sTextLog *errors)
    {
        sD3DShaderCompiler_.Init2();

        UINT f0 = D3DCOMPILE_ENABLE_STRICTNESS;
        UINT f1 = 0;
#if sConfigDebug && NSIGHT
        f0 |= D3D10_SHADER_PREFER_FLOW_CONTROL | D3D10_SHADER_DEBUG | D3D10_SHADER_SKIP_OPTIMIZATION;
#endif
        ID3D10Blob *code=0;
        ID3D10Blob *stripped = 0;
        ID3D10Blob *error=0;

        HRESULT hr = (*DXCompLibCompile)(source,sGetStringLen(source),"mem",0,0,"main",profile,f0,f1,&code,&error);
        if(error)
        {
            if(errors)
            {
                errors->Add((const char *)error->GetBufferPointer(),error->GetBufferSize()-1);
            }
            else
            {
                sString<sFormatBuffer> buffer((const char *)error->GetBufferPointer(),error->GetBufferSize());
                sDPrint(buffer);
                sDPrint("\n");
            }
            error->Release();

            sRelease(code);
            return 0;
        }
        if(FAILED(hr))
        {
            sLog("gfx","shader compiler failed");
            return 0;
        }

#if !(sConfigDebug && NSIGHT)

        UINT fs = D3DCOMPILER_STRIP_DEBUG_INFO|D3DCOMPILER_STRIP_REFLECTION_DATA|D3DCOMPILER_STRIP_TEST_BLOBS;
        hr = (*DXCompLibStripShader)(code->GetBufferPointer(),code->GetBufferSize(),fs,&stripped);
        if(!FAILED(hr))
        {
            code->Release();
            code = stripped;
            stripped = 0;
        }

#endif

        sShaderBinary *bin = new sShaderBinary;
        bin->Profile = profile;
        bin->Type = type;
        bin->DeleteData = 1;
        bin->Size = code->GetBufferSize();
        uint8 *data = new uint8[bin->Size];
        bin->Data = data;
        sCopyMem(data,code->GetBufferPointer(),bin->Size);
        bin->CalcHash();

        return bin;
    }

    void sDisassembleShaderDX(sShaderBinary *bin,sTextLog *log)
    {
        ID3D10Blob *comment=0;
        ID3D10Blob *dis = 0;

        UINT f = D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS | D3D_DISASM_ENABLE_INSTRUCTION_NUMBERING;
        HRESULT hr = (*DXCompLibDisassemble)(bin->Data,bin->Size,f,&comment,&dis);
        if(!FAILED(hr))
        {
            if(comment)
                log->Add((const char *)comment->GetBufferPointer(),comment->GetBufferSize()-1);
            if(dis)
                log->Add((const char *)dis->GetBufferPointer(),dis->GetBufferSize()-1);
        }
    }


/****************************************************************************/

};
