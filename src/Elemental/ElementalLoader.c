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
    ElemApplication (*ElemCreateApplication)(const char*);
    void (*ElemRunApplication)(ElemApplication, ElemRunHandlerPtr);
    void (*ElemFreeApplication)(ElemApplication);
    
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
    {
        return functionPointersLoaded;
    }

    elementalFunctions.ElemConfigureLogHandler = (void (*)(ElemLogHandlerPtr))GetFunctionPointer("ElemConfigureLogHandler");
    elementalFunctions.ElemCreateApplication = (ElemApplication (*)(const char*))GetFunctionPointer("ElemCreateApplication");
    elementalFunctions.ElemRunApplication = (void (*)(ElemApplication, ElemRunHandlerPtr))GetFunctionPointer("ElemRunApplication");
    elementalFunctions.ElemFreeApplication = (void (*)(ElemApplication))GetFunctionPointer("ElemFreeApplication");
    

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

static ElemApplication ElemCreateApplication(const char* applicationName)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);
        return (ElemApplication){0};
    }

    return elementalFunctions.ElemCreateApplication(applicationName);
}

static void ElemRunApplication(ElemApplication application, ElemRunHandlerPtr runHandler)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);
        return;
    }

    elementalFunctions.ElemRunApplication(application, runHandler);
}

static void ElemFreeApplication(ElemApplication application)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);
        return;
    }

    elementalFunctions.ElemFreeApplication(application);
}
