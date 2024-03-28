#pragma once

#include "ElementalTools.h"
#include "SystemMemory.h"

// https://github.com/microsoft/DirectXShaderCompiler/issues/2833
enum DxilShaderKind 
{
    Pixel = 0,
    Vertex,
    Geometry,
    Hull,
    Domain,
    Compute,
    Library,
    RayGeneration,
    Intersection,
    AnyHit,
    ClosestHit,
    Miss,
    Callable,
    Mesh,
    Amplification,
    Node,
    Invalid
};

bool DirectXShaderCompilerIsInstalled();
ElemShaderCompilationResult DirectXShaderCompilerCompileShader(ReadOnlySpan<uint8_t> shaderCode, ElemShaderLanguage targetLanguage, ElemToolsGraphicsApi targetGraphicsApi, const ElemCompileShaderOptions* options);
