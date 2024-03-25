#include "DirectXShaderCompiler.h"
#include "SystemFunctions.h"

SystemLibrary directXShaderCompilerLibrary = {};
DxcCreateInstanceProc directXShaderCompilerCreateInstanceFunction = nullptr;

void InitDirectXShaderCompiler()
{
    if (!directXShaderCompilerLibrary.Handle)
    {
        directXShaderCompilerLibrary = SystemLoadLibrary("dxcompiler");

        if (directXShaderCompilerLibrary.Handle != nullptr)
        {
            directXShaderCompilerCreateInstanceFunction = (DxcCreateInstanceProc)SystemGetFunctionExport(directXShaderCompilerLibrary, "DxcCreateInstance");
        }
    }
}

bool DirectXCompilerIsInstalled()
{
    InitDirectXShaderCompiler();

    printf("Check DirectXShader\n");
    return directXShaderCompilerCreateInstanceFunction != nullptr;
}
