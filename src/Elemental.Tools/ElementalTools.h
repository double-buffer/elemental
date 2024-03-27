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

typedef enum
{
    ElemToolsMessageType_Information = 0,
    ElemToolsMessageType_Warning = 1,
    ElemToolsMessageType_Error = 2,
} ElemToolsMessageType;

typedef struct
{
    uint8_t* Items;
    uint32_t Length;
} ElemToolsDataSpan;

typedef struct
{
    ElemToolsMessageType Type; 
    const char* Message;
} ElemToolsMessage;

typedef struct
{
    ElemToolsMessage* Items;
    uint32_t Length;
} ElemToolsMessageSpan;

typedef struct
{
    ElemShaderLanguage ShaderLanguage;
    ElemToolsDataSpan Data;
} ElemShaderSourceData;

typedef struct
{
    bool DebugMode;
} ElemCompileShaderOptions;

typedef struct
{
    ElemToolsDataSpan Data;
    ElemToolsMessageSpan Messages;
    bool HasErrors;
    // TODO: Metadata?
} ElemShaderCompilationResult;

ElemToolsAPI bool ElemCanCompileShader(ElemShaderLanguage shaderLanguage, ElemToolsGraphicsApi graphicsApi);
ElemToolsAPI ElemShaderCompilationResult ElemCompileShaderLibrary(ElemToolsGraphicsApi graphicsApi, const ElemShaderSourceData* sourceData, const ElemCompileShaderOptions* options);

#ifdef UseToolsLoader
#ifndef ElementalToolsLoader
#include "ElementalToolsLoader.c"
#endif
#endif

#endif  // #ifndef _ELEMENTALTOOLS_H_
