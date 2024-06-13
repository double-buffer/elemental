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
    ElemSystemInfo (*ElemGetSystemInfo)(void);
    int (*ElemRunApplication)(ElemRunApplicationParameters const *);
    void (*ElemExitApplication)(int);
    ElemWindow (*ElemCreateWindow)(ElemWindowOptions const *);
    void (*ElemFreeWindow)(ElemWindow);
    ElemWindowSize (*ElemGetWindowRenderSize)(ElemWindow);
    void (*ElemSetWindowTitle)(ElemWindow, char const *);
    void (*ElemSetWindowState)(ElemWindow, ElemWindowState);
    void (*ElemShowWindowCursor)(ElemWindow);
    void (*ElemHideWindowCursor)(ElemWindow);
    ElemWindowCursorPosition (*ElemGetWindowCursorPosition)(ElemWindow);
    void (*ElemSetGraphicsOptions)(ElemGraphicsOptions const *);
    ElemGraphicsDeviceInfoSpan (*ElemGetAvailableGraphicsDevices)(void);
    ElemGraphicsDevice (*ElemCreateGraphicsDevice)(ElemGraphicsDeviceOptions const *);
    void (*ElemFreeGraphicsDevice)(ElemGraphicsDevice);
    ElemGraphicsDeviceInfo (*ElemGetGraphicsDeviceInfo)(ElemGraphicsDevice);
    ElemCommandQueue (*ElemCreateCommandQueue)(ElemGraphicsDevice, ElemCommandQueueType, ElemCommandQueueOptions const *);
    void (*ElemFreeCommandQueue)(ElemCommandQueue);
    void (*ElemResetCommandAllocation)(ElemGraphicsDevice);
    ElemCommandList (*ElemGetCommandList)(ElemCommandQueue, ElemCommandListOptions const *);
    void (*ElemCommitCommandList)(ElemCommandList);
    ElemFence (*ElemExecuteCommandList)(ElemCommandQueue, ElemCommandList, ElemExecuteCommandListOptions const *);
    ElemFence (*ElemExecuteCommandLists)(ElemCommandQueue, ElemCommandListSpan, ElemExecuteCommandListOptions const *);
    void (*ElemWaitForFenceOnCpu)(ElemFence);
    ElemSwapChain (*ElemCreateSwapChain)(ElemCommandQueue, ElemWindow, ElemSwapChainUpdateHandlerPtr, ElemSwapChainOptions const *);
    void (*ElemFreeSwapChain)(ElemSwapChain);
    ElemSwapChainInfo (*ElemGetSwapChainInfo)(ElemSwapChain);
    void (*ElemSetSwapChainTiming)(ElemSwapChain, unsigned int, unsigned int);
    void (*ElemPresentSwapChain)(ElemSwapChain);
    ElemGraphicsHeap (*ElemCreateGraphicsHeap)(ElemGraphicsDevice, unsigned long long, ElemGraphicsHeapOptions const *);
    void (*ElemFreeGraphicsHeap)(ElemGraphicsHeap);
    ElemGraphicsResource (*ElemCreateGraphicsResource)(ElemGraphicsHeap, unsigned long long, ElemGraphicsResourceInfo const *);
    void (*ElemFreeGraphicsResource)(ElemGraphicsResource);
    ElemShaderDescriptor (*ElemCreateTextureShaderDescriptor)(ElemGraphicsResource, ElemTextureShaderDescriptorOptions const *);
    void (*ElemFreeShaderDescriptor)(ElemShaderDescriptor);
    ElemShaderLibrary (*ElemCreateShaderLibrary)(ElemGraphicsDevice, ElemDataSpan);
    void (*ElemFreeShaderLibrary)(ElemShaderLibrary);
    ElemPipelineState (*ElemCompileGraphicsPipelineState)(ElemGraphicsDevice, ElemGraphicsPipelineStateParameters const *);
    ElemPipelineState (*ElemCompileComputePipelineState)(ElemGraphicsDevice, ElemComputePipelineStateParameters const *);
    void (*ElemFreePipelineState)(ElemPipelineState);
    void (*ElemBindPipelineState)(ElemCommandList, ElemPipelineState);
    void (*ElemPushPipelineStateConstants)(ElemCommandList, unsigned int, ElemDataSpan);
    void (*ElemSetResourceBarrier)(ElemCommandList, ElemResourceBarrier const *);
    void (*ElemSetResourceBarriers)(ElemCommandList, ElemResourceBarrierSpan);
    void (*ElemDispatchCompute)(ElemCommandList, unsigned int, unsigned int, unsigned int);
    void (*ElemBeginRenderPass)(ElemCommandList, ElemBeginRenderPassParameters const *);
    void (*ElemEndRenderPass)(ElemCommandList);
    void (*ElemSetViewport)(ElemCommandList, ElemViewport const *);
    void (*ElemSetViewports)(ElemCommandList, ElemViewportSpan);
    void (*ElemDispatchMesh)(ElemCommandList, unsigned int, unsigned int, unsigned int);
    ElemInputDeviceInfo (*ElemGetInputDeviceInfo)(ElemInputDevice);
    ElemInputStream (*ElemGetInputStream)(void);
    
} ElementalFunctions;

static ElementalFunctions listElementalFunctions;

static bool LoadElementalLibrary(void) 
{
    if (!libraryElemental) 
    {
        #if defined(_WIN32)
            libraryElemental = LoadLibrary(L"Elemental.dll");
        #elif __APPLE__
            libraryElemental = dlopen("Elemental.framework/Elemental", RTLD_LAZY);

            if (!libraryElemental)
            {
                libraryElemental = dlopen("libElemental.dylib", RTLD_LAZY);
            }
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
    listElementalFunctions.ElemGetSystemInfo = (ElemSystemInfo (*)(void))GetElementalFunctionPointer("ElemGetSystemInfo");
    listElementalFunctions.ElemRunApplication = (int (*)(ElemRunApplicationParameters const *))GetElementalFunctionPointer("ElemRunApplication");
    listElementalFunctions.ElemExitApplication = (void (*)(int))GetElementalFunctionPointer("ElemExitApplication");
    listElementalFunctions.ElemCreateWindow = (ElemWindow (*)(ElemWindowOptions const *))GetElementalFunctionPointer("ElemCreateWindow");
    listElementalFunctions.ElemFreeWindow = (void (*)(ElemWindow))GetElementalFunctionPointer("ElemFreeWindow");
    listElementalFunctions.ElemGetWindowRenderSize = (ElemWindowSize (*)(ElemWindow))GetElementalFunctionPointer("ElemGetWindowRenderSize");
    listElementalFunctions.ElemSetWindowTitle = (void (*)(ElemWindow, char const *))GetElementalFunctionPointer("ElemSetWindowTitle");
    listElementalFunctions.ElemSetWindowState = (void (*)(ElemWindow, ElemWindowState))GetElementalFunctionPointer("ElemSetWindowState");
    listElementalFunctions.ElemShowWindowCursor = (void (*)(ElemWindow))GetElementalFunctionPointer("ElemShowWindowCursor");
    listElementalFunctions.ElemHideWindowCursor = (void (*)(ElemWindow))GetElementalFunctionPointer("ElemHideWindowCursor");
    listElementalFunctions.ElemGetWindowCursorPosition = (ElemWindowCursorPosition (*)(ElemWindow))GetElementalFunctionPointer("ElemGetWindowCursorPosition");
    listElementalFunctions.ElemSetGraphicsOptions = (void (*)(ElemGraphicsOptions const *))GetElementalFunctionPointer("ElemSetGraphicsOptions");
    listElementalFunctions.ElemGetAvailableGraphicsDevices = (ElemGraphicsDeviceInfoSpan (*)(void))GetElementalFunctionPointer("ElemGetAvailableGraphicsDevices");
    listElementalFunctions.ElemCreateGraphicsDevice = (ElemGraphicsDevice (*)(ElemGraphicsDeviceOptions const *))GetElementalFunctionPointer("ElemCreateGraphicsDevice");
    listElementalFunctions.ElemFreeGraphicsDevice = (void (*)(ElemGraphicsDevice))GetElementalFunctionPointer("ElemFreeGraphicsDevice");
    listElementalFunctions.ElemGetGraphicsDeviceInfo = (ElemGraphicsDeviceInfo (*)(ElemGraphicsDevice))GetElementalFunctionPointer("ElemGetGraphicsDeviceInfo");
    listElementalFunctions.ElemCreateCommandQueue = (ElemCommandQueue (*)(ElemGraphicsDevice, ElemCommandQueueType, ElemCommandQueueOptions const *))GetElementalFunctionPointer("ElemCreateCommandQueue");
    listElementalFunctions.ElemFreeCommandQueue = (void (*)(ElemCommandQueue))GetElementalFunctionPointer("ElemFreeCommandQueue");
    listElementalFunctions.ElemResetCommandAllocation = (void (*)(ElemGraphicsDevice))GetElementalFunctionPointer("ElemResetCommandAllocation");
    listElementalFunctions.ElemGetCommandList = (ElemCommandList (*)(ElemCommandQueue, ElemCommandListOptions const *))GetElementalFunctionPointer("ElemGetCommandList");
    listElementalFunctions.ElemCommitCommandList = (void (*)(ElemCommandList))GetElementalFunctionPointer("ElemCommitCommandList");
    listElementalFunctions.ElemExecuteCommandList = (ElemFence (*)(ElemCommandQueue, ElemCommandList, ElemExecuteCommandListOptions const *))GetElementalFunctionPointer("ElemExecuteCommandList");
    listElementalFunctions.ElemExecuteCommandLists = (ElemFence (*)(ElemCommandQueue, ElemCommandListSpan, ElemExecuteCommandListOptions const *))GetElementalFunctionPointer("ElemExecuteCommandLists");
    listElementalFunctions.ElemWaitForFenceOnCpu = (void (*)(ElemFence))GetElementalFunctionPointer("ElemWaitForFenceOnCpu");
    listElementalFunctions.ElemCreateSwapChain = (ElemSwapChain (*)(ElemCommandQueue, ElemWindow, ElemSwapChainUpdateHandlerPtr, ElemSwapChainOptions const *))GetElementalFunctionPointer("ElemCreateSwapChain");
    listElementalFunctions.ElemFreeSwapChain = (void (*)(ElemSwapChain))GetElementalFunctionPointer("ElemFreeSwapChain");
    listElementalFunctions.ElemGetSwapChainInfo = (ElemSwapChainInfo (*)(ElemSwapChain))GetElementalFunctionPointer("ElemGetSwapChainInfo");
    listElementalFunctions.ElemSetSwapChainTiming = (void (*)(ElemSwapChain, unsigned int, unsigned int))GetElementalFunctionPointer("ElemSetSwapChainTiming");
    listElementalFunctions.ElemPresentSwapChain = (void (*)(ElemSwapChain))GetElementalFunctionPointer("ElemPresentSwapChain");
    listElementalFunctions.ElemCreateGraphicsHeap = (ElemGraphicsHeap (*)(ElemGraphicsDevice, unsigned long long, ElemGraphicsHeapOptions const *))GetElementalFunctionPointer("ElemCreateGraphicsHeap");
    listElementalFunctions.ElemFreeGraphicsHeap = (void (*)(ElemGraphicsHeap))GetElementalFunctionPointer("ElemFreeGraphicsHeap");
    listElementalFunctions.ElemCreateGraphicsResource = (ElemGraphicsResource (*)(ElemGraphicsHeap, unsigned long long, ElemGraphicsResourceInfo const *))GetElementalFunctionPointer("ElemCreateGraphicsResource");
    listElementalFunctions.ElemFreeGraphicsResource = (void (*)(ElemGraphicsResource))GetElementalFunctionPointer("ElemFreeGraphicsResource");
    listElementalFunctions.ElemCreateTextureShaderDescriptor = (ElemShaderDescriptor (*)(ElemGraphicsResource, ElemTextureShaderDescriptorOptions const *))GetElementalFunctionPointer("ElemCreateTextureShaderDescriptor");
    listElementalFunctions.ElemFreeShaderDescriptor = (void (*)(ElemShaderDescriptor))GetElementalFunctionPointer("ElemFreeShaderDescriptor");
    listElementalFunctions.ElemCreateShaderLibrary = (ElemShaderLibrary (*)(ElemGraphicsDevice, ElemDataSpan))GetElementalFunctionPointer("ElemCreateShaderLibrary");
    listElementalFunctions.ElemFreeShaderLibrary = (void (*)(ElemShaderLibrary))GetElementalFunctionPointer("ElemFreeShaderLibrary");
    listElementalFunctions.ElemCompileGraphicsPipelineState = (ElemPipelineState (*)(ElemGraphicsDevice, ElemGraphicsPipelineStateParameters const *))GetElementalFunctionPointer("ElemCompileGraphicsPipelineState");
    listElementalFunctions.ElemCompileComputePipelineState = (ElemPipelineState (*)(ElemGraphicsDevice, ElemComputePipelineStateParameters const *))GetElementalFunctionPointer("ElemCompileComputePipelineState");
    listElementalFunctions.ElemFreePipelineState = (void (*)(ElemPipelineState))GetElementalFunctionPointer("ElemFreePipelineState");
    listElementalFunctions.ElemBindPipelineState = (void (*)(ElemCommandList, ElemPipelineState))GetElementalFunctionPointer("ElemBindPipelineState");
    listElementalFunctions.ElemPushPipelineStateConstants = (void (*)(ElemCommandList, unsigned int, ElemDataSpan))GetElementalFunctionPointer("ElemPushPipelineStateConstants");
    listElementalFunctions.ElemSetResourceBarrier = (void (*)(ElemCommandList, ElemResourceBarrier const *))GetElementalFunctionPointer("ElemSetResourceBarrier");
    listElementalFunctions.ElemSetResourceBarriers = (void (*)(ElemCommandList, ElemResourceBarrierSpan))GetElementalFunctionPointer("ElemSetResourceBarriers");
    listElementalFunctions.ElemDispatchCompute = (void (*)(ElemCommandList, unsigned int, unsigned int, unsigned int))GetElementalFunctionPointer("ElemDispatchCompute");
    listElementalFunctions.ElemBeginRenderPass = (void (*)(ElemCommandList, ElemBeginRenderPassParameters const *))GetElementalFunctionPointer("ElemBeginRenderPass");
    listElementalFunctions.ElemEndRenderPass = (void (*)(ElemCommandList))GetElementalFunctionPointer("ElemEndRenderPass");
    listElementalFunctions.ElemSetViewport = (void (*)(ElemCommandList, ElemViewport const *))GetElementalFunctionPointer("ElemSetViewport");
    listElementalFunctions.ElemSetViewports = (void (*)(ElemCommandList, ElemViewportSpan))GetElementalFunctionPointer("ElemSetViewports");
    listElementalFunctions.ElemDispatchMesh = (void (*)(ElemCommandList, unsigned int, unsigned int, unsigned int))GetElementalFunctionPointer("ElemDispatchMesh");
    listElementalFunctions.ElemGetInputDeviceInfo = (ElemInputDeviceInfo (*)(ElemInputDevice))GetElementalFunctionPointer("ElemGetInputDeviceInfo");
    listElementalFunctions.ElemGetInputStream = (ElemInputStream (*)(void))GetElementalFunctionPointer("ElemGetInputStream");
    

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
    else if (category == ElemLogMessageCategory_Application)
    {
        printf("Application");
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
    else if (category == ElemLogMessageCategory_Application)
    {
        printf("Application");
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

static inline ElemSystemInfo ElemGetSystemInfo(void)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemSystemInfo result = {};
        #else
        ElemSystemInfo result = (ElemSystemInfo){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemGetSystemInfo) 
    {
        assert(listElementalFunctions.ElemGetSystemInfo);

        #ifdef __cplusplus
        ElemSystemInfo result = {};
        #else
        ElemSystemInfo result = (ElemSystemInfo){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemGetSystemInfo();
}

static inline int ElemRunApplication(ElemRunApplicationParameters const * parameters)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        int result = {};
        #else
        int result = (int){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemRunApplication) 
    {
        assert(listElementalFunctions.ElemRunApplication);

        #ifdef __cplusplus
        int result = {};
        #else
        int result = (int){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemRunApplication(parameters);
}

static inline void ElemExitApplication(int exitCode)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemExitApplication) 
    {
        assert(listElementalFunctions.ElemExitApplication);
        return;
    }

    listElementalFunctions.ElemExitApplication(exitCode);
}

static inline ElemWindow ElemCreateWindow(ElemWindowOptions const * options)
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

    return listElementalFunctions.ElemCreateWindow(options);
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

static inline void ElemSetWindowTitle(ElemWindow window, char const * title)
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

static inline void ElemShowWindowCursor(ElemWindow window)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemShowWindowCursor) 
    {
        assert(listElementalFunctions.ElemShowWindowCursor);
        return;
    }

    listElementalFunctions.ElemShowWindowCursor(window);
}

static inline void ElemHideWindowCursor(ElemWindow window)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemHideWindowCursor) 
    {
        assert(listElementalFunctions.ElemHideWindowCursor);
        return;
    }

    listElementalFunctions.ElemHideWindowCursor(window);
}

static inline ElemWindowCursorPosition ElemGetWindowCursorPosition(ElemWindow window)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemWindowCursorPosition result = {};
        #else
        ElemWindowCursorPosition result = (ElemWindowCursorPosition){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemGetWindowCursorPosition) 
    {
        assert(listElementalFunctions.ElemGetWindowCursorPosition);

        #ifdef __cplusplus
        ElemWindowCursorPosition result = {};
        #else
        ElemWindowCursorPosition result = (ElemWindowCursorPosition){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemGetWindowCursorPosition(window);
}

static inline void ElemSetGraphicsOptions(ElemGraphicsOptions const * options)
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

static inline ElemGraphicsDevice ElemCreateGraphicsDevice(ElemGraphicsDeviceOptions const * options)
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

static inline ElemCommandQueue ElemCreateCommandQueue(ElemGraphicsDevice graphicsDevice, ElemCommandQueueType type, ElemCommandQueueOptions const * options)
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

static inline void ElemResetCommandAllocation(ElemGraphicsDevice graphicsDevice)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemResetCommandAllocation) 
    {
        assert(listElementalFunctions.ElemResetCommandAllocation);
        return;
    }

    listElementalFunctions.ElemResetCommandAllocation(graphicsDevice);
}

static inline ElemCommandList ElemGetCommandList(ElemCommandQueue commandQueue, ElemCommandListOptions const * options)
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

static inline ElemFence ElemExecuteCommandList(ElemCommandQueue commandQueue, ElemCommandList commandList, ElemExecuteCommandListOptions const * options)
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

static inline ElemFence ElemExecuteCommandLists(ElemCommandQueue commandQueue, ElemCommandListSpan commandLists, ElemExecuteCommandListOptions const * options)
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

static inline ElemSwapChain ElemCreateSwapChain(ElemCommandQueue commandQueue, ElemWindow window, ElemSwapChainUpdateHandlerPtr updateHandler, ElemSwapChainOptions const * options)
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

    return listElementalFunctions.ElemCreateSwapChain(commandQueue, window, updateHandler, options);
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

static inline void ElemSetSwapChainTiming(ElemSwapChain swapChain, unsigned int frameLatency, unsigned int targetFPS)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemSetSwapChainTiming) 
    {
        assert(listElementalFunctions.ElemSetSwapChainTiming);
        return;
    }

    listElementalFunctions.ElemSetSwapChainTiming(swapChain, frameLatency, targetFPS);
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

static inline ElemGraphicsHeap ElemCreateGraphicsHeap(ElemGraphicsDevice graphicsDevice, unsigned long long sizeInBytes, ElemGraphicsHeapOptions const * options)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemGraphicsHeap result = {};
        #else
        ElemGraphicsHeap result = (ElemGraphicsHeap){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemCreateGraphicsHeap) 
    {
        assert(listElementalFunctions.ElemCreateGraphicsHeap);

        #ifdef __cplusplus
        ElemGraphicsHeap result = {};
        #else
        ElemGraphicsHeap result = (ElemGraphicsHeap){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemCreateGraphicsHeap(graphicsDevice, sizeInBytes, options);
}

static inline void ElemFreeGraphicsHeap(ElemGraphicsHeap graphicsHeap)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemFreeGraphicsHeap) 
    {
        assert(listElementalFunctions.ElemFreeGraphicsHeap);
        return;
    }

    listElementalFunctions.ElemFreeGraphicsHeap(graphicsHeap);
}

static inline ElemGraphicsResource ElemCreateGraphicsResource(ElemGraphicsHeap graphicsHeap, unsigned long long graphicsHeapOffset, ElemGraphicsResourceInfo const * resourceInfo)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemGraphicsResource result = {};
        #else
        ElemGraphicsResource result = (ElemGraphicsResource){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemCreateGraphicsResource) 
    {
        assert(listElementalFunctions.ElemCreateGraphicsResource);

        #ifdef __cplusplus
        ElemGraphicsResource result = {};
        #else
        ElemGraphicsResource result = (ElemGraphicsResource){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemCreateGraphicsResource(graphicsHeap, graphicsHeapOffset, resourceInfo);
}

static inline void ElemFreeGraphicsResource(ElemGraphicsResource resource)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemFreeGraphicsResource) 
    {
        assert(listElementalFunctions.ElemFreeGraphicsResource);
        return;
    }

    listElementalFunctions.ElemFreeGraphicsResource(resource);
}

static inline ElemShaderDescriptor ElemCreateTextureShaderDescriptor(ElemGraphicsResource texture, ElemTextureShaderDescriptorOptions const * options)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemShaderDescriptor result = {};
        #else
        ElemShaderDescriptor result = (ElemShaderDescriptor){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemCreateTextureShaderDescriptor) 
    {
        assert(listElementalFunctions.ElemCreateTextureShaderDescriptor);

        #ifdef __cplusplus
        ElemShaderDescriptor result = {};
        #else
        ElemShaderDescriptor result = (ElemShaderDescriptor){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemCreateTextureShaderDescriptor(texture, options);
}

static inline void ElemFreeShaderDescriptor(ElemShaderDescriptor shaderDescriptor)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemFreeShaderDescriptor) 
    {
        assert(listElementalFunctions.ElemFreeShaderDescriptor);
        return;
    }

    listElementalFunctions.ElemFreeShaderDescriptor(shaderDescriptor);
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

static inline ElemPipelineState ElemCompileGraphicsPipelineState(ElemGraphicsDevice graphicsDevice, ElemGraphicsPipelineStateParameters const * parameters)
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

static inline ElemPipelineState ElemCompileComputePipelineState(ElemGraphicsDevice graphicsDevice, ElemComputePipelineStateParameters const * parameters)
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

    if (!listElementalFunctions.ElemCompileComputePipelineState) 
    {
        assert(listElementalFunctions.ElemCompileComputePipelineState);

        #ifdef __cplusplus
        ElemPipelineState result = {};
        #else
        ElemPipelineState result = (ElemPipelineState){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemCompileComputePipelineState(graphicsDevice, parameters);
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

static inline void ElemSetResourceBarrier(ElemCommandList commandList, ElemResourceBarrier const * resourceBarrier)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemSetResourceBarrier) 
    {
        assert(listElementalFunctions.ElemSetResourceBarrier);
        return;
    }

    listElementalFunctions.ElemSetResourceBarrier(commandList, resourceBarrier);
}

static inline void ElemSetResourceBarriers(ElemCommandList commandList, ElemResourceBarrierSpan resourceBarriers)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemSetResourceBarriers) 
    {
        assert(listElementalFunctions.ElemSetResourceBarriers);
        return;
    }

    listElementalFunctions.ElemSetResourceBarriers(commandList, resourceBarriers);
}

static inline void ElemDispatchCompute(ElemCommandList commandList, unsigned int threadGroupCountX, unsigned int threadGroupCountY, unsigned int threadGroupCountZ)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemDispatchCompute) 
    {
        assert(listElementalFunctions.ElemDispatchCompute);
        return;
    }

    listElementalFunctions.ElemDispatchCompute(commandList, threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

static inline void ElemBeginRenderPass(ElemCommandList commandList, ElemBeginRenderPassParameters const * parameters)
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

static inline void ElemSetViewport(ElemCommandList commandList, ElemViewport const * viewport)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemSetViewport) 
    {
        assert(listElementalFunctions.ElemSetViewport);
        return;
    }

    listElementalFunctions.ElemSetViewport(commandList, viewport);
}

static inline void ElemSetViewports(ElemCommandList commandList, ElemViewportSpan viewports)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemSetViewports) 
    {
        assert(listElementalFunctions.ElemSetViewports);
        return;
    }

    listElementalFunctions.ElemSetViewports(commandList, viewports);
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

static inline ElemInputDeviceInfo ElemGetInputDeviceInfo(ElemInputDevice inputDevice)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemInputDeviceInfo result = {};
        #else
        ElemInputDeviceInfo result = (ElemInputDeviceInfo){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemGetInputDeviceInfo) 
    {
        assert(listElementalFunctions.ElemGetInputDeviceInfo);

        #ifdef __cplusplus
        ElemInputDeviceInfo result = {};
        #else
        ElemInputDeviceInfo result = (ElemInputDeviceInfo){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemGetInputDeviceInfo(inputDevice);
}

static inline ElemInputStream ElemGetInputStream(void)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemInputStream result = {};
        #else
        ElemInputStream result = (ElemInputStream){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemGetInputStream) 
    {
        assert(listElementalFunctions.ElemGetInputStream);

        #ifdef __cplusplus
        ElemInputStream result = {};
        #else
        ElemInputStream result = (ElemInputStream){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemGetInputStream();
}
