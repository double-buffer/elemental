#define ElementalLoader

#include <assert.h>
#include "Elemental.h"

#if defined(_WIN32)
   #include <windows.h>
#else
   #include <dlfcn.h>
#endif

static void* library = NULL;

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
    if (!library) {
        return NULL;
    }

    #if defined(_WIN32)
        return GetProcAddress(library, functionName);
    #else
        return dlsym(library, functionName);
    #endif
}

static void ElemConfigureLogHandler(ElemLogHandlerPtr logHandler)
{
    if (!LoadElementalLibrary()) 
    {
        assert(library);
        return;
    }

    typedef void (*FunctionType)(ElemLogHandlerPtr);
    FunctionType functionPointer;

    functionPointer = (FunctionType)GetFunctionPointer("ElemConfigureLogHandler");

    if (!functionPointer) 
    {
        assert(functionPointer);
        return;
    }

    functionPointer(logHandler);
}

static uint64_t ElemCreateApplication(const char* applicationName)
{
    if (!LoadElementalLibrary()) 
    {
        assert(library);
        return (uint64_t){0};
    }

    typedef uint64_t (*FunctionType)(const char*);
    FunctionType functionPointer;

    functionPointer = (FunctionType)GetFunctionPointer("ElemCreateApplication");

    if (!functionPointer) 
    {
        assert(functionPointer);
        return (uint64_t){0};
    }

    return functionPointer(applicationName);
}

static void ElemRunApplication(uint64_t application, ElemRunHandlerPtr runHandler)
{
    if (!LoadElementalLibrary()) 
    {
        assert(library);
        return;
    }

    typedef void (*FunctionType)(uint64_t, ElemRunHandlerPtr);
    FunctionType functionPointer;

    functionPointer = (FunctionType)GetFunctionPointer("ElemRunApplication");

    if (!functionPointer) 
    {
        assert(functionPointer);
        return;
    }

    functionPointer(application, runHandler);
}

static void ElemFreeApplication(uint64_t application)
{
    if (!LoadElementalLibrary()) 
    {
        assert(library);
        return;
    }

    typedef void (*FunctionType)(uint64_t);
    FunctionType functionPointer;

    functionPointer = (FunctionType)GetFunctionPointer("ElemFreeApplication");

    if (!functionPointer) 
    {
        assert(functionPointer);
        return;
    }

    functionPointer(application);
}
