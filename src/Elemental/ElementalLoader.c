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
    static HMODULE libraryElemental = NULL;
#else
    static void* libraryElemental = NULL;
#endif

static int functionPointersLoadedElemental = 0;

typedef struct ElementalFunctions 
{
    void (*ElemConfigureLogHandler)(ElemLogHandlerPtr);
    ElemApplication (*ElemCreateApplication)(const char*);
    void (*ElemFreeApplication)(ElemApplication);
    void (*ElemRunApplication)(ElemApplication, ElemRunHandlerPtr);
    void (*ElemRunApplication2)(const ElemRunApplicationParameters*);
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
    ElemCommandList (*ElemGetCommandList)(ElemCommandQueue, const ElemCommandListOptions*);
    void (*ElemCommitCommandList)(ElemCommandList);
    ElemFence (*ElemExecuteCommandList)(ElemCommandQueue, ElemCommandList, const ElemExecuteCommandListOptions*);
    ElemFence (*ElemExecuteCommandLists)(ElemCommandQueue, ElemCommandListSpan, const ElemExecuteCommandListOptions*);
    void (*ElemWaitForFenceOnCpu)(ElemFence);
    ElemSwapChain (*ElemCreateSwapChain)(ElemCommandQueue, ElemWindow, const ElemSwapChainOptions*);
    ElemSwapChain (*ElemCreateSwapChain2)(ElemCommandQueue, ElemWindow, ElemSwapChainUpdateHandlerPtr, const ElemSwapChainOptions*);
    void (*ElemFreeSwapChain)(ElemSwapChain);
    ElemSwapChainInfo (*ElemGetSwapChainInfo)(ElemSwapChain);
    void (*ElemResizeSwapChain)(ElemSwapChain, unsigned int, unsigned int);
    ElemTexture (*ElemGetSwapChainBackBufferTexture)(ElemSwapChain);
    void (*ElemPresentSwapChain)(ElemSwapChain);
    void (*ElemWaitForSwapChainOnCpu)(ElemSwapChain);
    ElemShaderLibrary (*ElemCreateShaderLibrary)(ElemGraphicsDevice, ElemDataSpan);
    void (*ElemFreeShaderLibrary)(ElemShaderLibrary);
    ElemPipelineState (*ElemCompileGraphicsPipelineState)(ElemGraphicsDevice, const ElemGraphicsPipelineStateParameters*);
    void (*ElemFreePipelineState)(ElemPipelineState);
    void (*ElemBindPipelineState)(ElemCommandList, ElemPipelineState);
    void (*ElemPushPipelineStateConstants)(ElemCommandList, unsigned int, ElemDataSpan);
    void (*ElemBeginRenderPass)(ElemCommandList, const ElemBeginRenderPassParameters*);
    void (*ElemEndRenderPass)(ElemCommandList);
    void (*ElemDispatchMesh)(ElemCommandList, unsigned int, unsigned int, unsigned int);
    
} ElementalFunctions;

static ElementalFunctions listElementalFunctions;

static bool LoadElementalLibrary(void) 
{
    if (!libraryElemental) 
    {
        #if defined(_WIN32)
            libraryElemental = LoadLibrary(L"Elemental.dll");
        #elif __APPLE__
            libraryElemental = dlopen("libElemental.dylib", RTLD_LAZY);
        #else
            libraryElemental = dlopen("libElemental.so", RTLD_LAZY);
        #endif

        if (!libraryElemental) 
        {
            return false;
        }
    }

    return true;
}

void* GetElementalFunctionPointer(const char* functionName) 
{
    if (!libraryElemental) 
    {
        return NULL;
    }

    #if defined(_WIN32)
        return (void*)GetProcAddress(libraryElemental, functionName);
    #else
        return dlsym(libraryElemental, functionName);
    #endif
}

static bool LoadElementalFunctionPointers(void) 
{
    if (!LoadElementalLibrary() || functionPointersLoadedElemental)
    {
        return functionPointersLoadedElemental;
    }

    listElementalFunctions.ElemConfigureLogHandler = (void (*)(ElemLogHandlerPtr))GetElementalFunctionPointer("ElemConfigureLogHandler");
    listElementalFunctions.ElemCreateApplication = (ElemApplication (*)(const char*))GetElementalFunctionPointer("ElemCreateApplication");
    listElementalFunctions.ElemFreeApplication = (void (*)(ElemApplication))GetElementalFunctionPointer("ElemFreeApplication");
    listElementalFunctions.ElemRunApplication = (void (*)(ElemApplication, ElemRunHandlerPtr))GetElementalFunctionPointer("ElemRunApplication");
    listElementalFunctions.ElemRunApplication2 = (void (*)(const ElemRunApplicationParameters*))GetElementalFunctionPointer("ElemRunApplication2");
    listElementalFunctions.ElemCreateWindow = (ElemWindow (*)(ElemApplication, const ElemWindowOptions*))GetElementalFunctionPointer("ElemCreateWindow");
    listElementalFunctions.ElemFreeWindow = (void (*)(ElemWindow))GetElementalFunctionPointer("ElemFreeWindow");
    listElementalFunctions.ElemGetWindowRenderSize = (ElemWindowSize (*)(ElemWindow))GetElementalFunctionPointer("ElemGetWindowRenderSize");
    listElementalFunctions.ElemSetWindowTitle = (void (*)(ElemWindow, const char*))GetElementalFunctionPointer("ElemSetWindowTitle");
    listElementalFunctions.ElemSetWindowState = (void (*)(ElemWindow, ElemWindowState))GetElementalFunctionPointer("ElemSetWindowState");
    listElementalFunctions.ElemSetGraphicsOptions = (void (*)(const ElemGraphicsOptions*))GetElementalFunctionPointer("ElemSetGraphicsOptions");
    listElementalFunctions.ElemGetAvailableGraphicsDevices = (ElemGraphicsDeviceInfoSpan (*)(void))GetElementalFunctionPointer("ElemGetAvailableGraphicsDevices");
    listElementalFunctions.ElemCreateGraphicsDevice = (ElemGraphicsDevice (*)(const ElemGraphicsDeviceOptions*))GetElementalFunctionPointer("ElemCreateGraphicsDevice");
    listElementalFunctions.ElemFreeGraphicsDevice = (void (*)(ElemGraphicsDevice))GetElementalFunctionPointer("ElemFreeGraphicsDevice");
    listElementalFunctions.ElemGetGraphicsDeviceInfo = (ElemGraphicsDeviceInfo (*)(ElemGraphicsDevice))GetElementalFunctionPointer("ElemGetGraphicsDeviceInfo");
    listElementalFunctions.ElemCreateCommandQueue = (ElemCommandQueue (*)(ElemGraphicsDevice, ElemCommandQueueType, const ElemCommandQueueOptions*))GetElementalFunctionPointer("ElemCreateCommandQueue");
    listElementalFunctions.ElemFreeCommandQueue = (void (*)(ElemCommandQueue))GetElementalFunctionPointer("ElemFreeCommandQueue");
    listElementalFunctions.ElemGetCommandList = (ElemCommandList (*)(ElemCommandQueue, const ElemCommandListOptions*))GetElementalFunctionPointer("ElemGetCommandList");
    listElementalFunctions.ElemCommitCommandList = (void (*)(ElemCommandList))GetElementalFunctionPointer("ElemCommitCommandList");
    listElementalFunctions.ElemExecuteCommandList = (ElemFence (*)(ElemCommandQueue, ElemCommandList, const ElemExecuteCommandListOptions*))GetElementalFunctionPointer("ElemExecuteCommandList");
    listElementalFunctions.ElemExecuteCommandLists = (ElemFence (*)(ElemCommandQueue, ElemCommandListSpan, const ElemExecuteCommandListOptions*))GetElementalFunctionPointer("ElemExecuteCommandLists");
    listElementalFunctions.ElemWaitForFenceOnCpu = (void (*)(ElemFence))GetElementalFunctionPointer("ElemWaitForFenceOnCpu");
    listElementalFunctions.ElemCreateSwapChain = (ElemSwapChain (*)(ElemCommandQueue, ElemWindow, const ElemSwapChainOptions*))GetElementalFunctionPointer("ElemCreateSwapChain");
    listElementalFunctions.ElemCreateSwapChain2 = (ElemSwapChain (*)(ElemCommandQueue, ElemWindow, ElemSwapChainUpdateHandlerPtr, const ElemSwapChainOptions*))GetElementalFunctionPointer("ElemCreateSwapChain2");
    listElementalFunctions.ElemFreeSwapChain = (void (*)(ElemSwapChain))GetElementalFunctionPointer("ElemFreeSwapChain");
    listElementalFunctions.ElemGetSwapChainInfo = (ElemSwapChainInfo (*)(ElemSwapChain))GetElementalFunctionPointer("ElemGetSwapChainInfo");
    listElementalFunctions.ElemResizeSwapChain = (void (*)(ElemSwapChain, unsigned int, unsigned int))GetElementalFunctionPointer("ElemResizeSwapChain");
    listElementalFunctions.ElemGetSwapChainBackBufferTexture = (ElemTexture (*)(ElemSwapChain))GetElementalFunctionPointer("ElemGetSwapChainBackBufferTexture");
    listElementalFunctions.ElemPresentSwapChain = (void (*)(ElemSwapChain))GetElementalFunctionPointer("ElemPresentSwapChain");
    listElementalFunctions.ElemWaitForSwapChainOnCpu = (void (*)(ElemSwapChain))GetElementalFunctionPointer("ElemWaitForSwapChainOnCpu");
    listElementalFunctions.ElemCreateShaderLibrary = (ElemShaderLibrary (*)(ElemGraphicsDevice, ElemDataSpan))GetElementalFunctionPointer("ElemCreateShaderLibrary");
    listElementalFunctions.ElemFreeShaderLibrary = (void (*)(ElemShaderLibrary))GetElementalFunctionPointer("ElemFreeShaderLibrary");
    listElementalFunctions.ElemCompileGraphicsPipelineState = (ElemPipelineState (*)(ElemGraphicsDevice, const ElemGraphicsPipelineStateParameters*))GetElementalFunctionPointer("ElemCompileGraphicsPipelineState");
    listElementalFunctions.ElemFreePipelineState = (void (*)(ElemPipelineState))GetElementalFunctionPointer("ElemFreePipelineState");
    listElementalFunctions.ElemBindPipelineState = (void (*)(ElemCommandList, ElemPipelineState))GetElementalFunctionPointer("ElemBindPipelineState");
    listElementalFunctions.ElemPushPipelineStateConstants = (void (*)(ElemCommandList, unsigned int, ElemDataSpan))GetElementalFunctionPointer("ElemPushPipelineStateConstants");
    listElementalFunctions.ElemBeginRenderPass = (void (*)(ElemCommandList, const ElemBeginRenderPassParameters*))GetElementalFunctionPointer("ElemBeginRenderPass");
    listElementalFunctions.ElemEndRenderPass = (void (*)(ElemCommandList))GetElementalFunctionPointer("ElemEndRenderPass");
    listElementalFunctions.ElemDispatchMesh = (void (*)(ElemCommandList, unsigned int, unsigned int, unsigned int))GetElementalFunctionPointer("ElemDispatchMesh");
    

    functionPointersLoadedElemental = 1;
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
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemConfigureLogHandler) 
    {
        assert(listElementalFunctions.ElemConfigureLogHandler);
        return;
    }

    listElementalFunctions.ElemConfigureLogHandler(logHandler);
}

static inline ElemApplication ElemCreateApplication(const char* applicationName)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemApplication result = {};
        #else
        ElemApplication result = (ElemApplication){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemCreateApplication) 
    {
        assert(listElementalFunctions.ElemCreateApplication);

        #ifdef __cplusplus
        ElemApplication result = {};
        #else
        ElemApplication result = (ElemApplication){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemCreateApplication(applicationName);
}

static inline void ElemFreeApplication(ElemApplication application)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemFreeApplication) 
    {
        assert(listElementalFunctions.ElemFreeApplication);
        return;
    }

    listElementalFunctions.ElemFreeApplication(application);
}

static inline void ElemRunApplication(ElemApplication application, ElemRunHandlerPtr runHandler)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemRunApplication) 
    {
        assert(listElementalFunctions.ElemRunApplication);
        return;
    }

    listElementalFunctions.ElemRunApplication(application, runHandler);
}

static inline void ElemRunApplication2(const ElemRunApplicationParameters* parameters)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemRunApplication2) 
    {
        assert(listElementalFunctions.ElemRunApplication2);
        return;
    }

    listElementalFunctions.ElemRunApplication2(parameters);
}

static inline ElemWindow ElemCreateWindow(ElemApplication application, const ElemWindowOptions* options)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemWindow result = {};
        #else
        ElemWindow result = (ElemWindow){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemCreateWindow) 
    {
        assert(listElementalFunctions.ElemCreateWindow);

        #ifdef __cplusplus
        ElemWindow result = {};
        #else
        ElemWindow result = (ElemWindow){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemCreateWindow(application, options);
}

static inline void ElemFreeWindow(ElemWindow window)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemFreeWindow) 
    {
        assert(listElementalFunctions.ElemFreeWindow);
        return;
    }

    listElementalFunctions.ElemFreeWindow(window);
}

static inline ElemWindowSize ElemGetWindowRenderSize(ElemWindow window)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemWindowSize result = {};
        #else
        ElemWindowSize result = (ElemWindowSize){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemGetWindowRenderSize) 
    {
        assert(listElementalFunctions.ElemGetWindowRenderSize);

        #ifdef __cplusplus
        ElemWindowSize result = {};
        #else
        ElemWindowSize result = (ElemWindowSize){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemGetWindowRenderSize(window);
}

static inline void ElemSetWindowTitle(ElemWindow window, const char* title)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemSetWindowTitle) 
    {
        assert(listElementalFunctions.ElemSetWindowTitle);
        return;
    }

    listElementalFunctions.ElemSetWindowTitle(window, title);
}

static inline void ElemSetWindowState(ElemWindow window, ElemWindowState windowState)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemSetWindowState) 
    {
        assert(listElementalFunctions.ElemSetWindowState);
        return;
    }

    listElementalFunctions.ElemSetWindowState(window, windowState);
}

static inline void ElemSetGraphicsOptions(const ElemGraphicsOptions* options)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemSetGraphicsOptions) 
    {
        assert(listElementalFunctions.ElemSetGraphicsOptions);
        return;
    }

    listElementalFunctions.ElemSetGraphicsOptions(options);
}

static inline ElemGraphicsDeviceInfoSpan ElemGetAvailableGraphicsDevices(void)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemGraphicsDeviceInfoSpan result = {};
        #else
        ElemGraphicsDeviceInfoSpan result = (ElemGraphicsDeviceInfoSpan){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemGetAvailableGraphicsDevices) 
    {
        assert(listElementalFunctions.ElemGetAvailableGraphicsDevices);

        #ifdef __cplusplus
        ElemGraphicsDeviceInfoSpan result = {};
        #else
        ElemGraphicsDeviceInfoSpan result = (ElemGraphicsDeviceInfoSpan){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemGetAvailableGraphicsDevices();
}

static inline ElemGraphicsDevice ElemCreateGraphicsDevice(const ElemGraphicsDeviceOptions* options)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemGraphicsDevice result = {};
        #else
        ElemGraphicsDevice result = (ElemGraphicsDevice){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemCreateGraphicsDevice) 
    {
        assert(listElementalFunctions.ElemCreateGraphicsDevice);

        #ifdef __cplusplus
        ElemGraphicsDevice result = {};
        #else
        ElemGraphicsDevice result = (ElemGraphicsDevice){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemCreateGraphicsDevice(options);
}

static inline void ElemFreeGraphicsDevice(ElemGraphicsDevice graphicsDevice)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemFreeGraphicsDevice) 
    {
        assert(listElementalFunctions.ElemFreeGraphicsDevice);
        return;
    }

    listElementalFunctions.ElemFreeGraphicsDevice(graphicsDevice);
}

static inline ElemGraphicsDeviceInfo ElemGetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemGraphicsDeviceInfo result = {};
        #else
        ElemGraphicsDeviceInfo result = (ElemGraphicsDeviceInfo){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemGetGraphicsDeviceInfo) 
    {
        assert(listElementalFunctions.ElemGetGraphicsDeviceInfo);

        #ifdef __cplusplus
        ElemGraphicsDeviceInfo result = {};
        #else
        ElemGraphicsDeviceInfo result = (ElemGraphicsDeviceInfo){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemGetGraphicsDeviceInfo(graphicsDevice);
}

static inline ElemCommandQueue ElemCreateCommandQueue(ElemGraphicsDevice graphicsDevice, ElemCommandQueueType type, const ElemCommandQueueOptions* options)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemCommandQueue result = {};
        #else
        ElemCommandQueue result = (ElemCommandQueue){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemCreateCommandQueue) 
    {
        assert(listElementalFunctions.ElemCreateCommandQueue);

        #ifdef __cplusplus
        ElemCommandQueue result = {};
        #else
        ElemCommandQueue result = (ElemCommandQueue){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemCreateCommandQueue(graphicsDevice, type, options);
}

static inline void ElemFreeCommandQueue(ElemCommandQueue commandQueue)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemFreeCommandQueue) 
    {
        assert(listElementalFunctions.ElemFreeCommandQueue);
        return;
    }

    listElementalFunctions.ElemFreeCommandQueue(commandQueue);
}

static inline ElemCommandList ElemGetCommandList(ElemCommandQueue commandQueue, const ElemCommandListOptions* options)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemCommandList result = {};
        #else
        ElemCommandList result = (ElemCommandList){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemGetCommandList) 
    {
        assert(listElementalFunctions.ElemGetCommandList);

        #ifdef __cplusplus
        ElemCommandList result = {};
        #else
        ElemCommandList result = (ElemCommandList){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemGetCommandList(commandQueue, options);
}

static inline void ElemCommitCommandList(ElemCommandList commandList)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemCommitCommandList) 
    {
        assert(listElementalFunctions.ElemCommitCommandList);
        return;
    }

    listElementalFunctions.ElemCommitCommandList(commandList);
}

static inline ElemFence ElemExecuteCommandList(ElemCommandQueue commandQueue, ElemCommandList commandList, const ElemExecuteCommandListOptions* options)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemFence result = {};
        #else
        ElemFence result = (ElemFence){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemExecuteCommandList) 
    {
        assert(listElementalFunctions.ElemExecuteCommandList);

        #ifdef __cplusplus
        ElemFence result = {};
        #else
        ElemFence result = (ElemFence){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemExecuteCommandList(commandQueue, commandList, options);
}

static inline ElemFence ElemExecuteCommandLists(ElemCommandQueue commandQueue, ElemCommandListSpan commandLists, const ElemExecuteCommandListOptions* options)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemFence result = {};
        #else
        ElemFence result = (ElemFence){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemExecuteCommandLists) 
    {
        assert(listElementalFunctions.ElemExecuteCommandLists);

        #ifdef __cplusplus
        ElemFence result = {};
        #else
        ElemFence result = (ElemFence){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemExecuteCommandLists(commandQueue, commandLists, options);
}

static inline void ElemWaitForFenceOnCpu(ElemFence fence)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemWaitForFenceOnCpu) 
    {
        assert(listElementalFunctions.ElemWaitForFenceOnCpu);
        return;
    }

    listElementalFunctions.ElemWaitForFenceOnCpu(fence);
}

static inline ElemSwapChain ElemCreateSwapChain(ElemCommandQueue commandQueue, ElemWindow window, const ElemSwapChainOptions* options)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemSwapChain result = {};
        #else
        ElemSwapChain result = (ElemSwapChain){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemCreateSwapChain) 
    {
        assert(listElementalFunctions.ElemCreateSwapChain);

        #ifdef __cplusplus
        ElemSwapChain result = {};
        #else
        ElemSwapChain result = (ElemSwapChain){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemCreateSwapChain(commandQueue, window, options);
}

static inline ElemSwapChain ElemCreateSwapChain2(ElemCommandQueue commandQueue, ElemWindow window, ElemSwapChainUpdateHandlerPtr updateHandler, const ElemSwapChainOptions* options)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemSwapChain result = {};
        #else
        ElemSwapChain result = (ElemSwapChain){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemCreateSwapChain2) 
    {
        assert(listElementalFunctions.ElemCreateSwapChain2);

        #ifdef __cplusplus
        ElemSwapChain result = {};
        #else
        ElemSwapChain result = (ElemSwapChain){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemCreateSwapChain2(commandQueue, window, updateHandler, options);
}

static inline void ElemFreeSwapChain(ElemSwapChain swapChain)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemFreeSwapChain) 
    {
        assert(listElementalFunctions.ElemFreeSwapChain);
        return;
    }

    listElementalFunctions.ElemFreeSwapChain(swapChain);
}

static inline ElemSwapChainInfo ElemGetSwapChainInfo(ElemSwapChain swapChain)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemSwapChainInfo result = {};
        #else
        ElemSwapChainInfo result = (ElemSwapChainInfo){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemGetSwapChainInfo) 
    {
        assert(listElementalFunctions.ElemGetSwapChainInfo);

        #ifdef __cplusplus
        ElemSwapChainInfo result = {};
        #else
        ElemSwapChainInfo result = (ElemSwapChainInfo){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemGetSwapChainInfo(swapChain);
}

static inline void ElemResizeSwapChain(ElemSwapChain swapChain, unsigned int width, unsigned int height)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemResizeSwapChain) 
    {
        assert(listElementalFunctions.ElemResizeSwapChain);
        return;
    }

    listElementalFunctions.ElemResizeSwapChain(swapChain, width, height);
}

static inline ElemTexture ElemGetSwapChainBackBufferTexture(ElemSwapChain swapChain)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemTexture result = {};
        #else
        ElemTexture result = (ElemTexture){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemGetSwapChainBackBufferTexture) 
    {
        assert(listElementalFunctions.ElemGetSwapChainBackBufferTexture);

        #ifdef __cplusplus
        ElemTexture result = {};
        #else
        ElemTexture result = (ElemTexture){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemGetSwapChainBackBufferTexture(swapChain);
}

static inline void ElemPresentSwapChain(ElemSwapChain swapChain)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemPresentSwapChain) 
    {
        assert(listElementalFunctions.ElemPresentSwapChain);
        return;
    }

    listElementalFunctions.ElemPresentSwapChain(swapChain);
}

static inline void ElemWaitForSwapChainOnCpu(ElemSwapChain swapChain)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemWaitForSwapChainOnCpu) 
    {
        assert(listElementalFunctions.ElemWaitForSwapChainOnCpu);
        return;
    }

    listElementalFunctions.ElemWaitForSwapChainOnCpu(swapChain);
}

static inline ElemShaderLibrary ElemCreateShaderLibrary(ElemGraphicsDevice graphicsDevice, ElemDataSpan shaderLibraryData)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemShaderLibrary result = {};
        #else
        ElemShaderLibrary result = (ElemShaderLibrary){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemCreateShaderLibrary) 
    {
        assert(listElementalFunctions.ElemCreateShaderLibrary);

        #ifdef __cplusplus
        ElemShaderLibrary result = {};
        #else
        ElemShaderLibrary result = (ElemShaderLibrary){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemCreateShaderLibrary(graphicsDevice, shaderLibraryData);
}

static inline void ElemFreeShaderLibrary(ElemShaderLibrary shaderLibrary)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemFreeShaderLibrary) 
    {
        assert(listElementalFunctions.ElemFreeShaderLibrary);
        return;
    }

    listElementalFunctions.ElemFreeShaderLibrary(shaderLibrary);
}

static inline ElemPipelineState ElemCompileGraphicsPipelineState(ElemGraphicsDevice graphicsDevice, const ElemGraphicsPipelineStateParameters* parameters)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemPipelineState result = {};
        #else
        ElemPipelineState result = (ElemPipelineState){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemCompileGraphicsPipelineState) 
    {
        assert(listElementalFunctions.ElemCompileGraphicsPipelineState);

        #ifdef __cplusplus
        ElemPipelineState result = {};
        #else
        ElemPipelineState result = (ElemPipelineState){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemCompileGraphicsPipelineState(graphicsDevice, parameters);
}

static inline void ElemFreePipelineState(ElemPipelineState pipelineState)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemFreePipelineState) 
    {
        assert(listElementalFunctions.ElemFreePipelineState);
        return;
    }

    listElementalFunctions.ElemFreePipelineState(pipelineState);
}

static inline void ElemBindPipelineState(ElemCommandList commandList, ElemPipelineState pipelineState)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemBindPipelineState) 
    {
        assert(listElementalFunctions.ElemBindPipelineState);
        return;
    }

    listElementalFunctions.ElemBindPipelineState(commandList, pipelineState);
}

static inline void ElemPushPipelineStateConstants(ElemCommandList commandList, unsigned int offsetInBytes, ElemDataSpan data)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemPushPipelineStateConstants) 
    {
        assert(listElementalFunctions.ElemPushPipelineStateConstants);
        return;
    }

    listElementalFunctions.ElemPushPipelineStateConstants(commandList, offsetInBytes, data);
}

static inline void ElemBeginRenderPass(ElemCommandList commandList, const ElemBeginRenderPassParameters* parameters)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemBeginRenderPass) 
    {
        assert(listElementalFunctions.ElemBeginRenderPass);
        return;
    }

    listElementalFunctions.ElemBeginRenderPass(commandList, parameters);
}

static inline void ElemEndRenderPass(ElemCommandList commandList)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemEndRenderPass) 
    {
        assert(listElementalFunctions.ElemEndRenderPass);
        return;
    }

    listElementalFunctions.ElemEndRenderPass(commandList);
}

static inline void ElemDispatchMesh(ElemCommandList commandList, unsigned int threadGroupCountX, unsigned int threadGroupCountY, unsigned int threadGroupCountZ)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemDispatchMesh) 
    {
        assert(listElementalFunctions.ElemDispatchMesh);
        return;
    }

    listElementalFunctions.ElemDispatchMesh(commandList, threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}
