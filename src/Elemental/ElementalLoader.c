#define ElementalLoader

#include <assert.h>
#include <stdio.h>
#include "Elemental.h"

#if defined(_WIN32)
   #define UNICODE
   #include <windows.h>
#else
   #include <dlfcn.h>
#endif

#if defined(_WIN32)
    static HMODULE library = NULL;
#else
    static void* library = NULL;
#endif

static int functionPointersLoaded = 0;

typedef struct ElementalFunctions 
{
    void (*ElemConfigureLogHandler)(ElemLogHandlerPtr);
    ElemApplication (*ElemCreateApplication)(const char*);
    void (*ElemFreeApplication)(ElemApplication);
    void (*ElemRunApplication)(ElemApplication, ElemRunHandlerPtr);
    ElemWindow (*ElemCreateWindow)(ElemApplication, const ElemWindowOptions*);
    void (*ElemFreeWindow)(ElemWindow);
    ElemWindowSize (*ElemGetWindowRenderSize)(ElemWindow);
    void (*ElemSetWindowTitle)(ElemWindow, const char*);
    void (*ElemSetWindowState)(ElemWindow, ElemWindowState);
    ElemGraphicsDevice (*ElemCreateGraphicsDevice)(void);
    void (*ElemFreeGraphicsDevice)(ElemGraphicsDevice);
    
} ElementalFunctions;

static ElementalFunctions elementalFunctions;

static bool LoadElementalLibrary(void) 
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
        return (void*)GetProcAddress(library, functionName);
    #else
        return dlsym(library, functionName);
    #endif
}

static bool LoadFunctionPointers(void) 
{
    if (!LoadElementalLibrary() || functionPointersLoaded)
    {
        return functionPointersLoaded;
    }

    elementalFunctions.ElemConfigureLogHandler = (void (*)(ElemLogHandlerPtr))GetFunctionPointer("ElemConfigureLogHandler");
    elementalFunctions.ElemCreateApplication = (ElemApplication (*)(const char*))GetFunctionPointer("ElemCreateApplication");
    elementalFunctions.ElemFreeApplication = (void (*)(ElemApplication))GetFunctionPointer("ElemFreeApplication");
    elementalFunctions.ElemRunApplication = (void (*)(ElemApplication, ElemRunHandlerPtr))GetFunctionPointer("ElemRunApplication");
    elementalFunctions.ElemCreateWindow = (ElemWindow (*)(ElemApplication, const ElemWindowOptions*))GetFunctionPointer("ElemCreateWindow");
    elementalFunctions.ElemFreeWindow = (void (*)(ElemWindow))GetFunctionPointer("ElemFreeWindow");
    elementalFunctions.ElemGetWindowRenderSize = (ElemWindowSize (*)(ElemWindow))GetFunctionPointer("ElemGetWindowRenderSize");
    elementalFunctions.ElemSetWindowTitle = (void (*)(ElemWindow, const char*))GetFunctionPointer("ElemSetWindowTitle");
    elementalFunctions.ElemSetWindowState = (void (*)(ElemWindow, ElemWindowState))GetFunctionPointer("ElemSetWindowState");
    elementalFunctions.ElemCreateGraphicsDevice = (ElemGraphicsDevice (*)(void))GetFunctionPointer("ElemCreateGraphicsDevice");
    elementalFunctions.ElemFreeGraphicsDevice = (void (*)(ElemGraphicsDevice))GetFunctionPointer("ElemFreeGraphicsDevice");
    

    functionPointersLoaded = 1;
    return true;
}

static inline void ElemConsoleLogHandler(ElemLogMessageType messageType, ElemLogMessageCategory category, const char* function, const char* message) 
{
    printf("[");
    printf("\033[36m");

    if (category == ElemLogMessageCategory_Assert)
    {
        printf("Assert");
    }
    else if (category == ElemLogMessageCategory_Memory)
    {
        printf("Memory");
    }
    else if (category == ElemLogMessageCategory_NativeApplication)
    {
        printf("NativeApplication");
    }
    else if (category == ElemLogMessageCategory_Graphics)
    {
        printf("Graphics");
    }
    else if (category == ElemLogMessageCategory_Inputs)
    {
        printf("Inputs");
    }

    printf("\033[0m]");

    printf("\033[32m %s", function);

    if (messageType == ElemLogMessageType_Error)
    {
        printf("\033[31m");
    }
    else if (messageType == ElemLogMessageType_Warning)
    {
        printf("\033[33m");
    }
    else
    {
        printf("\033[0m");
    }

    printf(" %s\n\033[0m", message);
}

static inline void ElemConfigureLogHandler(ElemLogHandlerPtr logHandler)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);
        return;
    }

    if (!elementalFunctions.ElemConfigureLogHandler) 
    {
        assert(elementalFunctions.ElemConfigureLogHandler);
        return;
    }

    elementalFunctions.ElemConfigureLogHandler(logHandler);
}

static inline ElemApplication ElemCreateApplication(const char* applicationName)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);
        return (ElemApplication){0};
    }

    if (!elementalFunctions.ElemCreateApplication) 
    {
        assert(elementalFunctions.ElemCreateApplication);
        return (ElemApplication){0};
    }

    return elementalFunctions.ElemCreateApplication(applicationName);
}

static inline void ElemFreeApplication(ElemApplication application)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);
        return;
    }

    if (!elementalFunctions.ElemFreeApplication) 
    {
        assert(elementalFunctions.ElemFreeApplication);
        return;
    }

    elementalFunctions.ElemFreeApplication(application);
}

static inline void ElemRunApplication(ElemApplication application, ElemRunHandlerPtr runHandler)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);
        return;
    }

    if (!elementalFunctions.ElemRunApplication) 
    {
        assert(elementalFunctions.ElemRunApplication);
        return;
    }

    elementalFunctions.ElemRunApplication(application, runHandler);
}

static inline ElemWindow ElemCreateWindow(ElemApplication application, const ElemWindowOptions* options)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);
        return (ElemWindow){0};
    }

    if (!elementalFunctions.ElemCreateWindow) 
    {
        assert(elementalFunctions.ElemCreateWindow);
        return (ElemWindow){0};
    }

    return elementalFunctions.ElemCreateWindow(application, options);
}

static inline void ElemFreeWindow(ElemWindow window)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);
        return;
    }

    if (!elementalFunctions.ElemFreeWindow) 
    {
        assert(elementalFunctions.ElemFreeWindow);
        return;
    }

    elementalFunctions.ElemFreeWindow(window);
}

static inline ElemWindowSize ElemGetWindowRenderSize(ElemWindow window)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);
        return (ElemWindowSize){0};
    }

    if (!elementalFunctions.ElemGetWindowRenderSize) 
    {
        assert(elementalFunctions.ElemGetWindowRenderSize);
        return (ElemWindowSize){0};
    }

    return elementalFunctions.ElemGetWindowRenderSize(window);
}

static inline void ElemSetWindowTitle(ElemWindow window, const char* title)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);
        return;
    }

    if (!elementalFunctions.ElemSetWindowTitle) 
    {
        assert(elementalFunctions.ElemSetWindowTitle);
        return;
    }

    elementalFunctions.ElemSetWindowTitle(window, title);
}

static inline void ElemSetWindowState(ElemWindow window, ElemWindowState windowState)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);
        return;
    }

    if (!elementalFunctions.ElemSetWindowState) 
    {
        assert(elementalFunctions.ElemSetWindowState);
        return;
    }

    elementalFunctions.ElemSetWindowState(window, windowState);
}

static inline ElemGraphicsDevice ElemCreateGraphicsDevice(void)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);
        return (ElemGraphicsDevice){0};
    }

    if (!elementalFunctions.ElemCreateGraphicsDevice) 
    {
        assert(elementalFunctions.ElemCreateGraphicsDevice);
        return (ElemGraphicsDevice){0};
    }

    return elementalFunctions.ElemCreateGraphicsDevice();
}

static inline void ElemFreeGraphicsDevice(ElemGraphicsDevice graphicsDevice)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);
        return;
    }

    if (!elementalFunctions.ElemFreeGraphicsDevice) 
    {
        assert(elementalFunctions.ElemFreeGraphicsDevice);
        return;
    }

    elementalFunctions.ElemFreeGraphicsDevice(graphicsDevice);
}
