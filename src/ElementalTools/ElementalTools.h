//--------------------------------------------------------------------------------
// Elemental Tools Library
// Version: 1.0.0-dev5
//
// MIT License
//
// Copyright (c) 2023-2024 Double Buffer SRL
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//--------------------------------------------------------------------------------

#ifndef _ELEMENTALTOOLS_H_
#define _ELEMENTALTOOLS_H_

#include <stdint.h>
#include <stdbool.h>

#ifndef ElemToolsAPI
#define ElemToolsAPI static
#define UseToolsLoader
#endif

//------------------------------------------------------------------------
// ##Module_Tools##
//------------------------------------------------------------------------

/**
 * Enumerates supported graphics APIs.
 */
typedef enum
{
    // DirectX 12, a low-level graphics API by Microsoft.
    ElemToolsGraphicsApi_DirectX12 = 0,
    // Vulkan, a cross-platform graphics API.
    ElemToolsGraphicsApi_Vulkan = 1,
    // Metal, a graphics API by Apple.
    ElemToolsGraphicsApi_Metal = 2
} ElemToolsGraphicsApi;

/**
 * Enumerates supported platforms.
 */
typedef enum
{
    // Microsoft Windows.
    ElemToolsPlatform_Windows = 0,
    // Apple macOS.
    ElemToolsPlatform_MacOS = 1,
    // Apple iOS.
    ElemToolsPlatform_iOS = 2,
    // Linux platforms.
    ElemToolsPlatform_Linux = 3
} ElemToolsPlatform;

/**
 * Enumerates types of messages that can be produced by tools.
 */
typedef enum
{
    // Informational message.
    ElemToolsMessageType_Information = 0,
    // Warning message.
    ElemToolsMessageType_Warning = 1,
    // Error message indicating a problem.
    ElemToolsMessageType_Error = 2,
} ElemToolsMessageType;

/**
 * Represents a span of data, typically used for handling arrays or blocks of memory.
 */
typedef struct
{
    // Pointer to the data items.
    uint8_t* Items;
    // Number of items in the span.
    uint32_t Length;
} ElemToolsDataSpan;

/**
 * Represents a message produced by the tools, including a type and text.
 */
typedef struct
{
    // The type of message (info, warning, error).
    ElemToolsMessageType Type;
    // The message text.
    const char* Message;
} ElemToolsMessage;

/**
 * Represents a collection of messages produced during operations.
 */
typedef struct
{
    // Pointer to an array of messages.
    ElemToolsMessage* Items;
    // Number of messages in the array.
    uint32_t Length;
} ElemToolsMessageSpan;

typedef ElemToolsDataSpan (*ElemToolsLoadFileHandlerPtr)(const char* path);

ElemToolsAPI void ElemToolsConfigureFileIO(ElemToolsLoadFileHandlerPtr loadFileHandler);


//------------------------------------------------------------------------
// ##Module_Shaders##
//------------------------------------------------------------------------

// TODO: Change the list
/**
 * Enumerates supported shader languages.
 */
typedef enum
{
    // Unknown shader language.
    ElemShaderLanguage_Unknown = 0,
    // High Level Shading Language used by DirectX.
    ElemShaderLanguage_Hlsl = 1,
    // OpenGL Shading Language.
    ElemShaderLanguage_Glsl = 2,
    // Metal Shading Language for Apple devices.
    ElemShaderLanguage_Msl = 3,
    // DirectX Intermediate Language.
    ElemShaderLanguage_Dxil = 4,
    // Standard Portable Intermediate Representation.
    ElemShaderLanguage_Spirv = 5,
    // Intermediate representation for Metal.
    ElemShaderLanguage_MetalIR = 6
} ElemShaderLanguage;

/**
 * Represents shader source data, including language and the actual code.
 */
typedef struct
{
    // Language of the shader source code.
    ElemShaderLanguage ShaderLanguage;
    // Raw shader source code data.
    ElemToolsDataSpan Data;
} ElemShaderSourceData;

/**
 * Configuration options for compiling shaders.
 */
typedef struct
{
    // If true, compile shaders in debug mode to provide more information.
    bool DebugMode;

    // TODO: Add the ability to add an override for shader language
} ElemCompileShaderOptions;

/**
 * Represents the result of a shader compilation process.
 */
typedef struct
{
    // Compiled shader binary data.
    ElemToolsDataSpan Data;
    // Messages generated during the compilation.
    ElemToolsMessageSpan Messages;
    // True if compilation errors occurred.
    bool HasErrors;
} ElemShaderCompilationResult;

/**
 * Determines if a shader can be compiled for a specific graphics API and platform.
 * @param shaderLanguage The language of the shader to be compiled.
 * @param graphicsApi The graphics API for which the shader will be compiled.
 * @param platform The platform for which the shader will be compiled.
 * @return True if the shader can be compiled, false otherwise.
 */
ElemToolsAPI bool ElemCanCompileShader(ElemShaderLanguage shaderLanguage, ElemToolsGraphicsApi graphicsApi, ElemToolsPlatform platform);

/**
 * Compiles a shader library from source code.
 * @param graphicsApi The graphics API for which to compile the shader library.
 * @param platform The platform for which to compile the shader library.
 * @param sourceData The source data of the shaders to be compiled.
 * @param options Compilation options such as debug mode.
 * @return The result of the compilation, including any binaries and messages.
 */
// TODO: Put graphics api and platform into options (default to current one)
ElemToolsAPI ElemShaderCompilationResult ElemCompileShaderLibrary(ElemToolsGraphicsApi graphicsApi, ElemToolsPlatform platform, const char* path, const ElemCompileShaderOptions* options);

// TODO: Can we compile multiple source files into one library?


//------------------------------------------------------------------------
// ##Module_Meshes##
//------------------------------------------------------------------------

typedef enum
{
    ElemMeshFormat_Obj = 0,
} ElemMeshFormat;

typedef struct
{
    ElemToolsDataSpan Data;
    uint32_t VertexSize;
} ElemVertexBuffer;

typedef struct
{
    uint32_t Reserved;
} ElemLoadMeshOptions;

// TODO: Allow to specify the offset of the vertex position? (For now we assume vertex position is at offset 0)
typedef struct
{
    uint32_t Reserved;
} ElemBuildMeshletsOptions;

typedef struct
{
    uint32_t VertexIndexOffset;
    uint32_t VertexIndexCount;
    uint32_t TriangleOffset;
    uint32_t TriangleCount;
} ElemMeshlet;

typedef struct
{
    ElemMeshlet* Items;
    uint32_t Length;
} ElemMeshletSpan;

typedef struct
{
    uint32_t* Items;
    uint32_t Length;
} ElemUInt32Span;

typedef struct
{
    ElemVertexBuffer VertexBuffer;
    uint32_t VertexCount;
    ElemToolsMessageSpan Messages;
    bool HasErrors;
} ElemLoadMeshResult;

typedef struct
{
    uint8_t MeshletMaxVertexCount;
    uint8_t MeshletMaxTriangleCount;
    ElemVertexBuffer VertexBuffer;
    ElemMeshletSpan Meshlets;
    ElemUInt32Span MeshletVertexIndexBuffer;
    ElemUInt32Span MeshletTriangleIndexBuffer;
    ElemToolsMessageSpan Messages;
    bool HasErrors;
} ElemBuildMeshletResult;

// TODO: Add a way to pass multiple files?
// TODO: We need to have also a callback system for providing data based on files. Because for shaders and meshes you don't know
// in advance what files are used. The model of fast_obj is actually pretty good
ElemToolsAPI ElemLoadMeshResult ElemLoadMesh(ElemToolsDataSpan data, ElemMeshFormat meshFormat, const ElemLoadMeshOptions* options);

ElemToolsAPI ElemBuildMeshletResult ElemBuildMeshlets(ElemVertexBuffer vertexBuffer, const ElemBuildMeshletsOptions* options);

// TODO: Do an optimize mesh function that can be used to optimize a normal mesh (for Raytracing for example)

#ifdef UseToolsLoader
#ifndef ElementalToolsLoader
#include "ElementalToolsLoader.c"
#endif
#endif

#endif  // #ifndef _ELEMENTALTOOLS_H_
