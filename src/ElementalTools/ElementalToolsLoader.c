#define ElementalToolsLoader

#include <assert.h>
#include <stdio.h>
#include "ElementalTools.h"

#if defined(_WIN32)
   #define UNICODE
   #include <windows.h>
#else
   #include <dlfcn.h>
   #include <unistd.h>
   #include <string.h>
#endif

#if defined(_WIN32)
    static HMODULE libraryElementalTools = NULL;
#else
    static void* libraryElementalTools = NULL;
#endif

static int functionPointersLoadedElementalTools = 0;

typedef struct ElementalToolsFunctions 
{
    void (*ElemToolsConfigureFileIO)(ElemToolsLoadFileHandlerPtr);
    bool (*ElemCanCompileShader)(ElemShaderLanguage, ElemToolsGraphicsApi, ElemToolsPlatform);
    ElemShaderCompilationResult (*ElemCompileShaderLibrary)(ElemToolsGraphicsApi, ElemToolsPlatform, char const *, ElemCompileShaderOptions const *);
    ElemLoadSceneResult (*ElemLoadScene)(char const *, ElemLoadSceneOptions const *);
    ElemBuildMeshletResult (*ElemBuildMeshlets)(ElemVertexBuffer, ElemUInt32Span, ElemBuildMeshletsOptions const *);
    ElemLoadTextureResult (*ElemLoadTexture)(char const *, ElemLoadTextureOptions const *);
    ElemGenerateTextureMipDataResult (*ElemGenerateTextureMipData)(ElemToolsGraphicsFormat, ElemTextureMipData, ElemGenerateTextureMipDataOptions const *);
    
} ElementalToolsFunctions;

static ElementalToolsFunctions listElementalToolsFunctions;

static bool LoadElementalToolsLibrary(void) 
{
    if (!libraryElementalTools) 
    {
        #if defined(_WIN32)
            libraryElementalTools = LoadLibrary(L"ElementalTools.dll");
        #elif __APPLE__
            libraryElementalTools = dlopen("ElementalTools.framework/ElementalTools", RTLD_LAZY);

            if (!libraryElementalTools)
            {
                libraryElementalTools = dlopen("libElementalTools.dylib", RTLD_LAZY);
            }
        #else
            libraryElementalTools = dlopen("libElementalTools.so", RTLD_LAZY);
        #endif

        if (!libraryElementalTools) 
        {
            return false;
        }
    }

    return true;
}

void* GetElementalToolsFunctionPointer(const char* functionName) 
{
    if (!libraryElementalTools) 
    {
        return NULL;
    }

    #if defined(_WIN32)
        return (void*)GetProcAddress(libraryElementalTools, functionName);
    #else
        return dlsym(libraryElementalTools, functionName);
    #endif
}

static bool LoadElementalToolsFunctionPointers(void) 
{
    if (!LoadElementalToolsLibrary() || functionPointersLoadedElementalTools)
    {
        return functionPointersLoadedElementalTools;
    }

    listElementalToolsFunctions.ElemToolsConfigureFileIO = (void (*)(ElemToolsLoadFileHandlerPtr))GetElementalToolsFunctionPointer("ElemToolsConfigureFileIO");
    listElementalToolsFunctions.ElemCanCompileShader = (bool (*)(ElemShaderLanguage, ElemToolsGraphicsApi, ElemToolsPlatform))GetElementalToolsFunctionPointer("ElemCanCompileShader");
    listElementalToolsFunctions.ElemCompileShaderLibrary = (ElemShaderCompilationResult (*)(ElemToolsGraphicsApi, ElemToolsPlatform, char const *, ElemCompileShaderOptions const *))GetElementalToolsFunctionPointer("ElemCompileShaderLibrary");
    listElementalToolsFunctions.ElemLoadScene = (ElemLoadSceneResult (*)(char const *, ElemLoadSceneOptions const *))GetElementalToolsFunctionPointer("ElemLoadScene");
    listElementalToolsFunctions.ElemBuildMeshlets = (ElemBuildMeshletResult (*)(ElemVertexBuffer, ElemUInt32Span, ElemBuildMeshletsOptions const *))GetElementalToolsFunctionPointer("ElemBuildMeshlets");
    listElementalToolsFunctions.ElemLoadTexture = (ElemLoadTextureResult (*)(char const *, ElemLoadTextureOptions const *))GetElementalToolsFunctionPointer("ElemLoadTexture");
    listElementalToolsFunctions.ElemGenerateTextureMipData = (ElemGenerateTextureMipDataResult (*)(ElemToolsGraphicsFormat, ElemTextureMipData, ElemGenerateTextureMipDataOptions const *))GetElementalToolsFunctionPointer("ElemGenerateTextureMipData");
    

    functionPointersLoadedElementalTools = 1;
    return true;
}

static inline void ElemToolsConfigureFileIO(ElemToolsLoadFileHandlerPtr loadFileHandler)
{
    if (!LoadElementalToolsFunctionPointers()) 
    {
        assert(libraryElementalTools);
        return;
    }

    if (!listElementalToolsFunctions.ElemToolsConfigureFileIO) 
    {
        assert(listElementalToolsFunctions.ElemToolsConfigureFileIO);
        return;
    }

    listElementalToolsFunctions.ElemToolsConfigureFileIO(loadFileHandler);
}

static inline bool ElemCanCompileShader(ElemShaderLanguage shaderLanguage, ElemToolsGraphicsApi graphicsApi, ElemToolsPlatform platform)
{
    if (!LoadElementalToolsFunctionPointers()) 
    {
        assert(libraryElementalTools);

        #ifdef __cplusplus
        bool result = {};
        #else
        bool result = (bool){0};
        #endif

        return result;
    }

    if (!listElementalToolsFunctions.ElemCanCompileShader) 
    {
        assert(listElementalToolsFunctions.ElemCanCompileShader);

        #ifdef __cplusplus
        bool result = {};
        #else
        bool result = (bool){0};
        #endif

        return result;
    }

    return listElementalToolsFunctions.ElemCanCompileShader(shaderLanguage, graphicsApi, platform);
}

static inline ElemShaderCompilationResult ElemCompileShaderLibrary(ElemToolsGraphicsApi graphicsApi, ElemToolsPlatform platform, char const * path, ElemCompileShaderOptions const * options)
{
    if (!LoadElementalToolsFunctionPointers()) 
    {
        assert(libraryElementalTools);

        #ifdef __cplusplus
        ElemShaderCompilationResult result = {};
        #else
        ElemShaderCompilationResult result = (ElemShaderCompilationResult){0};
        #endif

        return result;
    }

    if (!listElementalToolsFunctions.ElemCompileShaderLibrary) 
    {
        assert(listElementalToolsFunctions.ElemCompileShaderLibrary);

        #ifdef __cplusplus
        ElemShaderCompilationResult result = {};
        #else
        ElemShaderCompilationResult result = (ElemShaderCompilationResult){0};
        #endif

        return result;
    }

    return listElementalToolsFunctions.ElemCompileShaderLibrary(graphicsApi, platform, path, options);
}

static inline ElemLoadSceneResult ElemLoadScene(char const * path, ElemLoadSceneOptions const * options)
{
    if (!LoadElementalToolsFunctionPointers()) 
    {
        assert(libraryElementalTools);

        #ifdef __cplusplus
        ElemLoadSceneResult result = {};
        #else
        ElemLoadSceneResult result = (ElemLoadSceneResult){0};
        #endif

        return result;
    }

    if (!listElementalToolsFunctions.ElemLoadScene) 
    {
        assert(listElementalToolsFunctions.ElemLoadScene);

        #ifdef __cplusplus
        ElemLoadSceneResult result = {};
        #else
        ElemLoadSceneResult result = (ElemLoadSceneResult){0};
        #endif

        return result;
    }

    return listElementalToolsFunctions.ElemLoadScene(path, options);
}

static inline ElemBuildMeshletResult ElemBuildMeshlets(ElemVertexBuffer vertexBuffer, ElemUInt32Span indexBuffer, ElemBuildMeshletsOptions const * options)
{
    if (!LoadElementalToolsFunctionPointers()) 
    {
        assert(libraryElementalTools);

        #ifdef __cplusplus
        ElemBuildMeshletResult result = {};
        #else
        ElemBuildMeshletResult result = (ElemBuildMeshletResult){0};
        #endif

        return result;
    }

    if (!listElementalToolsFunctions.ElemBuildMeshlets) 
    {
        assert(listElementalToolsFunctions.ElemBuildMeshlets);

        #ifdef __cplusplus
        ElemBuildMeshletResult result = {};
        #else
        ElemBuildMeshletResult result = (ElemBuildMeshletResult){0};
        #endif

        return result;
    }

    return listElementalToolsFunctions.ElemBuildMeshlets(vertexBuffer, indexBuffer, options);
}

static inline ElemLoadTextureResult ElemLoadTexture(char const * path, ElemLoadTextureOptions const * options)
{
    if (!LoadElementalToolsFunctionPointers()) 
    {
        assert(libraryElementalTools);

        #ifdef __cplusplus
        ElemLoadTextureResult result = {};
        #else
        ElemLoadTextureResult result = (ElemLoadTextureResult){0};
        #endif

        return result;
    }

    if (!listElementalToolsFunctions.ElemLoadTexture) 
    {
        assert(listElementalToolsFunctions.ElemLoadTexture);

        #ifdef __cplusplus
        ElemLoadTextureResult result = {};
        #else
        ElemLoadTextureResult result = (ElemLoadTextureResult){0};
        #endif

        return result;
    }

    return listElementalToolsFunctions.ElemLoadTexture(path, options);
}

static inline ElemGenerateTextureMipDataResult ElemGenerateTextureMipData(ElemToolsGraphicsFormat format, ElemTextureMipData baseMip, ElemGenerateTextureMipDataOptions const * options)
{
    if (!LoadElementalToolsFunctionPointers()) 
    {
        assert(libraryElementalTools);

        #ifdef __cplusplus
        ElemGenerateTextureMipDataResult result = {};
        #else
        ElemGenerateTextureMipDataResult result = (ElemGenerateTextureMipDataResult){0};
        #endif

        return result;
    }

    if (!listElementalToolsFunctions.ElemGenerateTextureMipData) 
    {
        assert(listElementalToolsFunctions.ElemGenerateTextureMipData);

        #ifdef __cplusplus
        ElemGenerateTextureMipDataResult result = {};
        #else
        ElemGenerateTextureMipDataResult result = (ElemGenerateTextureMipDataResult){0};
        #endif

        return result;
    }

    return listElementalToolsFunctions.ElemGenerateTextureMipData(format, baseMip, options);
}
