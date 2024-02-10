#define ElementalLoader

#include <assert.h>
#include "Elemental.h"

#if defined(_WIN32)
   #include <windows.h>
#else
   #include <dlfcn.h>
#endif

static void* library = NULL;
static int functionPointersLoaded = 0;

typedef struct ElementalFunctions 
{
    void (*ElemConfigureLogHandler)(ElemLogHandlerPtr);
    uint64_t (*ElemCreateApplication)(const char*);
    void (*ElemRunApplication)(uint64_t, ElemRunHandlerPtr);
    void (*ElemFreeApplication)(uint64_t);
    
} ElementalFunctions;

static ElementalFunctions elementalFunctions;

static bool LoadElementalLibrary() 
{
    if (!library) 
    {
        #if defined(_WIN32)
            library = LoadLibrary(L"Elemental.dll");
        #elif __APPLE__
            library = dlopen("libElemental.dylib", RTLD_LAZY);
        #else
            library = dlopen("libElemental.so", RTLD_LAZY);
        #endif

        if (!library) 
        {
            return false;
        }
    }

    return true;
}

void* GetFunctionPointer(const char* functionName) 
{
    if (!library) 
    {
        return NULL;
    }

    #if defined(_WIN32)
        return GetProcAddress(library, functionName);
    #else
        return dlsym(library, functionName);
    #endif
}

static bool LoadFunctionPointers() 
{
    if (!LoadElementalLibrary() || functionPointersLoaded)
        return functionPointersLoaded;

    elementalFunctions.ElemConfigureLogHandler = (void (*)(ElemLogHandlerPtr))GetFunctionPointer("ElemConfigureLogHandler");
    elementalFunctions.ElemCreateApplication = (uint64_t (*)(const char*))GetFunctionPointer("ElemCreateApplication");
    elementalFunctions.ElemRunApplication = (void (*)(uint64_t, ElemRunHandlerPtr))GetFunctionPointer("ElemRunApplication");
    elementalFunctions.ElemFreeApplication = (void (*)(uint64_t))GetFunctionPointer("ElemFreeApplication");
    

    functionPointersLoaded = 1;
    return true;
}

static void ElemConfigureLogHandler(ElemLogHandlerPtr logHandler)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);
        return;
    }

    elementalFunctions.ElemConfigureLogHandler(logHandler);
}

static uint64_t ElemCreateApplication(const char* applicationName)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);
        return (uint64_t){0};
    }

    return elementalFunctions.ElemCreateApplication(applicationName);
}

static void ElemRunApplication(uint64_t application, ElemRunHandlerPtr runHandler)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);
        return;
    }

    elementalFunctions.ElemRunApplication(application, runHandler);
}

static void ElemFreeApplication(uint64_t application)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);
        return;
    }

    elementalFunctions.ElemFreeApplication(application);
}
