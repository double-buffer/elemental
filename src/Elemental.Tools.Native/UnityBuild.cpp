#include "PrecompiledHeader.h"
#include "SystemFunctions.h"

#ifdef _DEBUG
void* operator new(size_t size, const char* file, uint32_t lineNumber)
{
    return SystemAllocateMemory(size, file, lineNumber);
}

void operator delete(void* pointer, const char* file, uint32_t lineNumber)
{
    return SystemFreeMemory(pointer, file, lineNumber);
}

void operator delete(void* pointer)
{
    return SystemFreeMemory(pointer, __FILE__, (uint32_t)__LINE__);
}

#define new new(__FILE__, (uint32_t)__LINE__)
#endif

#include "DirectXShaderCompilerProvider.cpp"
#include "SpirvCrossShaderCompilerProvider.cpp"
#include "MetalShaderCompilerProvider.cpp"
#include "ShaderCompiler.cpp"

