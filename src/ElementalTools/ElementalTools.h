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
// Module: Elemental Tools
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

typedef struct
{
    uint32_t* Items;
    uint32_t Length;
} ElemUInt32Span;

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

typedef struct
{
    float X, Y, Z;
} ElemToolsVector3;

typedef union
{
    struct
    {
        float X, Y, Z, W;
    };

    struct
    {
        ElemToolsVector3 XYZ;
    }; 
} ElemToolsVector4;

typedef union
{
    float m[4][4];
    ElemToolsVector4 Rows[4];
} ElemToolsMatrix4x4;

typedef struct
{
    ElemToolsVector3 MinPoint;
    ElemToolsVector3 MaxPoint;
} ElemToolsBoundingBox;

typedef struct
{
    ElemToolsDataSpan Data;
    uint32_t VertexSize;
    uint32_t VertexCount;
    // TODO: Add vertex description structure
} ElemVertexBuffer;

typedef ElemToolsDataSpan (*ElemToolsLoadFileHandlerPtr)(const char* path);

ElemToolsAPI void ElemToolsConfigureFileIO(ElemToolsLoadFileHandlerPtr loadFileHandler);


//------------------------------------------------------------------------
// Module: Shaders
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
    // Slang Shading Language.
    ElemShaderLanguage_Slang = 2,
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
    // TODO: Add the ability to choose Matrix packing options (default to row major for now?)
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
// Module: SceneLoading
//------------------------------------------------------------------------

typedef enum
{
    ElemSceneFormat_Unknown = 0,
    ElemSceneFormat_Obj = 1,
    ElemSceneFormat_Gltf = 2
} ElemSceneFormat;

typedef enum
{
    ElemSceneCoordinateSystem_LeftHanded = 0,
    ElemSceneCoordinateSystem_RightHanded = 1
} ElemSceneCoordinateSystem;

typedef enum
{
    ElemSceneNodeType_Unknown = 0,
    ElemSceneNodeType_Mesh = 1
} ElemSceneNodeType;

typedef struct
{
    ElemSceneCoordinateSystem CoordinateSystem;
    float Scaling;
    ElemToolsVector3 Rotation;
    ElemToolsVector3 Translation;
    // TODO: Don't add options for the format here but in the builder instead
    // we still want the same format mechanism but hardcoded 

    // TODO: Add options to specify the vertex format wanted
} ElemLoadSceneOptions;

typedef struct
{
    ElemVertexBuffer VertexBuffer;
    ElemUInt32Span IndexBuffer;
    ElemToolsBoundingBox BoundingBox;
    // TODO: SphereVolume?
    // TODO: Material
} ElemSceneMeshPrimitive;

typedef struct
{
    ElemSceneMeshPrimitive* Items;
    uint32_t Length;
} ElemSceneMeshPrimitiveSpan;

typedef struct
{
    const char* Name;
    ElemSceneMeshPrimitiveSpan MeshPrimitives;
    ElemToolsBoundingBox BoundingBox;
    // TODO: SphereVolume?
} ElemSceneMesh;

typedef struct
{
    ElemSceneMesh* Items;
    uint32_t Length;
} ElemSceneMeshSpan;

typedef struct
{
    const char* Name;
    ElemSceneNodeType NodeType;
    int32_t ReferenceIndex;
    ElemToolsVector4 Rotation;
    float Scale;
    ElemToolsVector3 Translation;
    // TODO: Children
} ElemSceneNode;

typedef struct
{
    ElemSceneNode* Items;
    uint32_t Length;
} ElemSceneNodeSpan;

typedef struct
{
    ElemSceneFormat SceneFormat;
    ElemSceneCoordinateSystem CoordinateSystem;

    ElemSceneMeshSpan Meshes;
    ElemSceneNodeSpan Nodes;

    ElemToolsMessageSpan Messages;
    bool HasErrors;
} ElemLoadSceneResult;

ElemToolsAPI ElemLoadSceneResult ElemLoadScene(const char* path, const ElemLoadSceneOptions* options);


//------------------------------------------------------------------------
// Module: Meshes
//------------------------------------------------------------------------

// TODO: Allow to specify the offset of the vertex position? (For now we assume vertex position is at offset 0)
typedef struct
{
    // TODO: Allow bypass mesh format
    // TODO: Allow customize index packing
    uint32_t Reserved;
} ElemBuildMeshletsOptions;

typedef struct
{
    // TODO: Add base vertex
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
    uint8_t MeshletMaxVertexCount;
    uint8_t MeshletMaxTriangleCount;
    ElemVertexBuffer VertexBuffer;
    ElemMeshletSpan Meshlets;
    ElemUInt32Span MeshletVertexIndexBuffer;
    ElemUInt32Span MeshletTriangleIndexBuffer;
    ElemToolsMessageSpan Messages;
    bool HasErrors;
} ElemBuildMeshletResult;

ElemToolsAPI ElemBuildMeshletResult ElemBuildMeshlets(ElemVertexBuffer vertexBuffer, ElemUInt32Span indexBuffer, const ElemBuildMeshletsOptions* options);

// TODO: Do an optimize mesh function that can be used to optimize a normal mesh (for Raytracing for example)

//------------------------------------------------------------------------
// Module: Textures
//------------------------------------------------------------------------

typedef enum
{
    ElemToolsGraphicsFormat_R8G8B8A8_SRGB
} ElemToolsGraphicsFormat;

typedef enum
{
    ElemLoadTextureFileFormat_Unknown = 0,
    ElemLoadTextureFileFormat_Tga = 1,
} ElemLoadTextureFileFormat;

typedef struct
{
    uint32_t Width;
    uint32_t Height;
    ElemToolsDataSpan Data;
} ElemTextureMipData;

typedef struct
{
    ElemTextureMipData* Items;
    uint32_t Length;
} ElemTextureMipDataSpan;

typedef struct
{
    uint32_t Reserved;
} ElemLoadTextureOptions;

typedef struct
{
    ElemLoadTextureFileFormat FileFormat;
    ElemToolsGraphicsFormat Format;
    uint32_t Width;
    uint32_t Height;
    ElemTextureMipDataSpan MipData;
    ElemToolsMessageSpan Messages;
    bool HasErrors;
} ElemLoadTextureResult;

typedef struct
{
    // TODO: Alpha options
    uint32_t Reserved;
} ElemGenerateTextureMipDataOptions;

typedef struct
{
    ElemTextureMipDataSpan MipData;
    ElemToolsMessageSpan Messages;
    bool HasErrors;
} ElemGenerateTextureMipDataResult;

ElemToolsAPI ElemLoadTextureResult ElemLoadTexture(const char* path, const ElemLoadTextureOptions* options);

ElemToolsAPI ElemGenerateTextureMipDataResult ElemGenerateTextureMipData(ElemToolsGraphicsFormat format, ElemTextureMipData baseMip, const ElemGenerateTextureMipDataOptions* options);

#ifdef UseToolsLoader
#ifndef ElementalToolsLoader
#include "ElementalToolsLoader.c"
#endif
#endif

#endif  // #ifndef _ELEMENTALTOOLS_H_
