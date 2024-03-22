#ifndef _ELEMENTALTOOLS_H_
#define _ELEMENTALTOOLS_H_

#include <stdint.h>
#include <stdbool.h>

#ifndef ElemToolsAPI
#define ElemToolsAPI static
#define UseToolsLoader
#endif

//------------------------------------------------------------------------
// ##Module_Application##
//------------------------------------------------------------------------

typedef enum
{
    ElemShaderLanguage_Unknown = 0,
    ElemShaderLanguage_Hlsl = 1,
    ElemShaderLanguage_Glsl = 2,
    ElemShaderLanguage_Msl = 3,
    ElemShaderLanguage_Dxil = 4,
    ElemShaderLanguage_Spirv = 5,
    ElemShaderLanguage_MetalIR = 6
} ElemShaderLanguage;

typedef enum
{
    ElemToolsGraphicsApi_DirectX12 = 0,
    ElemToolsGraphicsApi_Vulkan = 1,
    ElemToolsGraphicsApi_Metal = 2
} ElemToolsGraphicsApi;

ElemToolsAPI bool ElemCanCompileShader(ElemShaderLanguage shaderLanguage, ElemToolsGraphicsApi graphicsApi);

#ifdef UseToolsLoader
#ifndef ElementalToolsLoader
#include "ElementalToolsLoader.c"
#endif
#endif

#endif  // #ifndef _ELEMENTALTOOLS_H_
