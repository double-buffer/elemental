#define ElementalLoader

#include <assert.h>
#include <stdio.h>
#include "Elemental.h"

#if defined(_WIN32)
   #define UNICODE
   #include <windows.h>
#else
   #include <dlfcn.h>
   #include <unistd.h>
   #include <string.h>
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
    void (*ElemSetGraphicsOptions)(const ElemGraphicsOptions*);
    ElemGraphicsDeviceInfoSpan (*ElemGetAvailableGraphicsDevices)(void);
    ElemGraphicsDevice (*ElemCreateGraphicsDevice)(const ElemGraphicsDeviceOptions*);
    void (*ElemFreeGraphicsDevice)(ElemGraphicsDevice);
    ElemGraphicsDeviceInfo (*ElemGetGraphicsDeviceInfo)(ElemGraphicsDevice);
    ElemCommandQueue (*ElemCreateCommandQueue)(ElemGraphicsDevice, ElemCommandQueueType, const ElemCommandQueueOptions*);
    void (*ElemFreeCommandQueue)(ElemCommandQueue);
    ElemCommandList (*ElemCreateCommandList)(ElemCommandQueue, const ElemCommandListOptions*);
    void (*ElemCommitCommandList)(ElemCommandList);
    ElemFence (*ElemExecuteCommandList)(ElemCommandQueue, ElemCommandList, const ElemExecuteCommandListOptions*);
    ElemFence (*ElemExecuteCommandLists)(ElemCommandQueue, ElemCommandListSpan, const ElemExecuteCommandListOptions*);
    void (*ElemWaitForFenceOnCpu)(ElemFence);
    
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
    elementalFunctions.ElemSetGraphicsOptions = (void (*)(const ElemGraphicsOptions*))GetFunctionPointer("ElemSetGraphicsOptions");
    elementalFunctions.ElemGetAvailableGraphicsDevices = (ElemGraphicsDeviceInfoSpan (*)(void))GetFunctionPointer("ElemGetAvailableGraphicsDevices");
    elementalFunctions.ElemCreateGraphicsDevice = (ElemGraphicsDevice (*)(const ElemGraphicsDeviceOptions*))GetFunctionPointer("ElemCreateGraphicsDevice");
    elementalFunctions.ElemFreeGraphicsDevice = (void (*)(ElemGraphicsDevice))GetFunctionPointer("ElemFreeGraphicsDevice");
    elementalFunctions.ElemGetGraphicsDeviceInfo = (ElemGraphicsDeviceInfo (*)(ElemGraphicsDevice))GetFunctionPointer("ElemGetGraphicsDeviceInfo");
    elementalFunctions.ElemCreateCommandQueue = (ElemCommandQueue (*)(ElemGraphicsDevice, ElemCommandQueueType, const ElemCommandQueueOptions*))GetFunctionPointer("ElemCreateCommandQueue");
    elementalFunctions.ElemFreeCommandQueue = (void (*)(ElemCommandQueue))GetFunctionPointer("ElemFreeCommandQueue");
    elementalFunctions.ElemCreateCommandList = (ElemCommandList (*)(ElemCommandQueue, const ElemCommandListOptions*))GetFunctionPointer("ElemCreateCommandList");
    elementalFunctions.ElemCommitCommandList = (void (*)(ElemCommandList))GetFunctionPointer("ElemCommitCommandList");
    elementalFunctions.ElemExecuteCommandList = (ElemFence (*)(ElemCommandQueue, ElemCommandList, const ElemExecuteCommandListOptions*))GetFunctionPointer("ElemExecuteCommandList");
    elementalFunctions.ElemExecuteCommandLists = (ElemFence (*)(ElemCommandQueue, ElemCommandListSpan, const ElemExecuteCommandListOptions*))GetFunctionPointer("ElemExecuteCommandLists");
    elementalFunctions.ElemWaitForFenceOnCpu = (void (*)(ElemFence))GetFunctionPointer("ElemWaitForFenceOnCpu");
    

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
        printf("\033[31m Error:");
    }
    else if (messageType == ElemLogMessageType_Warning)
    {
        printf("\033[33m Warning:");
    }
    else if (messageType == ElemLogMessageType_Debug)
    {
        printf("\033[0m Debug:");
    }
    else
    {
        printf("\033[0m");
    }

    printf(" %s\033[0m\n", message);
    fflush(stdout);
}

static inline void ElemConsoleErrorLogHandler(ElemLogMessageType messageType, ElemLogMessageCategory category, const char* function, const char* message) 
{
    if (messageType != ElemLogMessageType_Error)
    {
        return;
    }

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
    printf("\033[32m %s\033[31m Error: %s\033[0m\n", function, message);
    fflush(stdout);
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

        #ifdef __cplusplus
        ElemApplication result = {};
        #else
        ElemApplication result = (ElemApplication){0};
        #endif

        return result;
    }

    if (!elementalFunctions.ElemCreateApplication) 
    {
        assert(elementalFunctions.ElemCreateApplication);

        #ifdef __cplusplus
        ElemApplication result = {};
        #else
        ElemApplication result = (ElemApplication){0};
        #endif

        return result;
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

        #ifdef __cplusplus
        ElemWindow result = {};
        #else
        ElemWindow result = (ElemWindow){0};
        #endif

        return result;
    }

    if (!elementalFunctions.ElemCreateWindow) 
    {
        assert(elementalFunctions.ElemCreateWindow);

        #ifdef __cplusplus
        ElemWindow result = {};
        #else
        ElemWindow result = (ElemWindow){0};
        #endif

        return result;
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

        #ifdef __cplusplus
        ElemWindowSize result = {};
        #else
        ElemWindowSize result = (ElemWindowSize){0};
        #endif

        return result;
    }

    if (!elementalFunctions.ElemGetWindowRenderSize) 
    {
        assert(elementalFunctions.ElemGetWindowRenderSize);

        #ifdef __cplusplus
        ElemWindowSize result = {};
        #else
        ElemWindowSize result = (ElemWindowSize){0};
        #endif

        return result;
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

static inline void ElemSetGraphicsOptions(const ElemGraphicsOptions* options)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);
        return;
    }

    if (!elementalFunctions.ElemSetGraphicsOptions) 
    {
        assert(elementalFunctions.ElemSetGraphicsOptions);
        return;
    }

    elementalFunctions.ElemSetGraphicsOptions(options);
}

static inline ElemGraphicsDeviceInfoSpan ElemGetAvailableGraphicsDevices(void)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);

        #ifdef __cplusplus
        ElemGraphicsDeviceInfoSpan result = {};
        #else
        ElemGraphicsDeviceInfoSpan result = (ElemGraphicsDeviceInfoSpan){0};
        #endif

        return result;
    }

    if (!elementalFunctions.ElemGetAvailableGraphicsDevices) 
    {
        assert(elementalFunctions.ElemGetAvailableGraphicsDevices);

        #ifdef __cplusplus
        ElemGraphicsDeviceInfoSpan result = {};
        #else
        ElemGraphicsDeviceInfoSpan result = (ElemGraphicsDeviceInfoSpan){0};
        #endif

        return result;
    }

    return elementalFunctions.ElemGetAvailableGraphicsDevices();
}

static inline ElemGraphicsDevice ElemCreateGraphicsDevice(const ElemGraphicsDeviceOptions* options)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);

        #ifdef __cplusplus
        ElemGraphicsDevice result = {};
        #else
        ElemGraphicsDevice result = (ElemGraphicsDevice){0};
        #endif

        return result;
    }

    if (!elementalFunctions.ElemCreateGraphicsDevice) 
    {
        assert(elementalFunctions.ElemCreateGraphicsDevice);

        #ifdef __cplusplus
        ElemGraphicsDevice result = {};
        #else
        ElemGraphicsDevice result = (ElemGraphicsDevice){0};
        #endif

        return result;
    }

    return elementalFunctions.ElemCreateGraphicsDevice(options);
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

static inline ElemGraphicsDeviceInfo ElemGetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);

        #ifdef __cplusplus
        ElemGraphicsDeviceInfo result = {};
        #else
        ElemGraphicsDeviceInfo result = (ElemGraphicsDeviceInfo){0};
        #endif

        return result;
    }

    if (!elementalFunctions.ElemGetGraphicsDeviceInfo) 
    {
        assert(elementalFunctions.ElemGetGraphicsDeviceInfo);

        #ifdef __cplusplus
        ElemGraphicsDeviceInfo result = {};
        #else
        ElemGraphicsDeviceInfo result = (ElemGraphicsDeviceInfo){0};
        #endif

        return result;
    }

    return elementalFunctions.ElemGetGraphicsDeviceInfo(graphicsDevice);
}

static inline ElemCommandQueue ElemCreateCommandQueue(ElemGraphicsDevice graphicsDevice, ElemCommandQueueType type, const ElemCommandQueueOptions* options)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);

        #ifdef __cplusplus
        ElemCommandQueue result = {};
        #else
        ElemCommandQueue result = (ElemCommandQueue){0};
        #endif

        return result;
    }

    if (!elementalFunctions.ElemCreateCommandQueue) 
    {
        assert(elementalFunctions.ElemCreateCommandQueue);

        #ifdef __cplusplus
        ElemCommandQueue result = {};
        #else
        ElemCommandQueue result = (ElemCommandQueue){0};
        #endif

        return result;
    }

    return elementalFunctions.ElemCreateCommandQueue(graphicsDevice, type, options);
}

static inline void ElemFreeCommandQueue(ElemCommandQueue commandQueue)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);
        return;
    }

    if (!elementalFunctions.ElemFreeCommandQueue) 
    {
        assert(elementalFunctions.ElemFreeCommandQueue);
        return;
    }

    elementalFunctions.ElemFreeCommandQueue(commandQueue);
}

static inline ElemCommandList ElemCreateCommandList(ElemCommandQueue commandQueue, const ElemCommandListOptions* options)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);

        #ifdef __cplusplus
        ElemCommandList result = {};
        #else
        ElemCommandList result = (ElemCommandList){0};
        #endif

        return result;
    }

    if (!elementalFunctions.ElemCreateCommandList) 
    {
        assert(elementalFunctions.ElemCreateCommandList);

        #ifdef __cplusplus
        ElemCommandList result = {};
        #else
        ElemCommandList result = (ElemCommandList){0};
        #endif

        return result;
    }

    return elementalFunctions.ElemCreateCommandList(commandQueue, options);
}

static inline void ElemCommitCommandList(ElemCommandList commandList)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);
        return;
    }

    if (!elementalFunctions.ElemCommitCommandList) 
    {
        assert(elementalFunctions.ElemCommitCommandList);
        return;
    }

    elementalFunctions.ElemCommitCommandList(commandList);
}

static inline ElemFence ElemExecuteCommandList(ElemCommandQueue commandQueue, ElemCommandList commandList, const ElemExecuteCommandListOptions* options)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);

        #ifdef __cplusplus
        ElemFence result = {};
        #else
        ElemFence result = (ElemFence){0};
        #endif

        return result;
    }

    if (!elementalFunctions.ElemExecuteCommandList) 
    {
        assert(elementalFunctions.ElemExecuteCommandList);

        #ifdef __cplusplus
        ElemFence result = {};
        #else
        ElemFence result = (ElemFence){0};
        #endif

        return result;
    }

    return elementalFunctions.ElemExecuteCommandList(commandQueue, commandList, options);
}

static inline ElemFence ElemExecuteCommandLists(ElemCommandQueue commandQueue, ElemCommandListSpan commandLists, const ElemExecuteCommandListOptions* options)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);

        #ifdef __cplusplus
        ElemFence result = {};
        #else
        ElemFence result = (ElemFence){0};
        #endif

        return result;
    }

    if (!elementalFunctions.ElemExecuteCommandLists) 
    {
        assert(elementalFunctions.ElemExecuteCommandLists);

        #ifdef __cplusplus
        ElemFence result = {};
        #else
        ElemFence result = (ElemFence){0};
        #endif

        return result;
    }

    return elementalFunctions.ElemExecuteCommandLists(commandQueue, commandLists, options);
}

static inline void ElemWaitForFenceOnCpu(ElemFence fence)
{
    if (!LoadFunctionPointers()) 
    {
        assert(library);
        return;
    }

    if (!elementalFunctions.ElemWaitForFenceOnCpu) 
    {
        assert(elementalFunctions.ElemWaitForFenceOnCpu);
        return;
    }

    elementalFunctions.ElemWaitForFenceOnCpu(fence);
}
