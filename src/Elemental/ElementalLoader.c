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
    bool (*ElemIsFenceCompleted)(ElemFence);
    ElemSwapChain (*ElemCreateSwapChain)(ElemCommandQueue, ElemWindow, ElemSwapChainUpdateHandlerPtr, ElemSwapChainOptions const *);
    void (*ElemFreeSwapChain)(ElemSwapChain);
    ElemSwapChainInfo (*ElemGetSwapChainInfo)(ElemSwapChain);
    void (*ElemSetSwapChainTiming)(ElemSwapChain, unsigned int, unsigned int);
    void (*ElemPresentSwapChain)(ElemSwapChain);
    ElemGraphicsHeap (*ElemCreateGraphicsHeap)(ElemGraphicsDevice, uint64_t, ElemGraphicsHeapOptions const *);
    void (*ElemFreeGraphicsHeap)(ElemGraphicsHeap);
    ElemGraphicsResourceInfo (*ElemCreateGraphicsBufferResourceInfo)(ElemGraphicsDevice, unsigned int, ElemGraphicsResourceUsage, ElemGraphicsResourceInfoOptions const *);
    ElemGraphicsResourceInfo (*ElemCreateTexture2DResourceInfo)(ElemGraphicsDevice, unsigned int, unsigned int, unsigned int, ElemGraphicsFormat, ElemGraphicsResourceUsage, ElemGraphicsResourceInfoOptions const *);
    ElemGraphicsResource (*ElemCreateGraphicsResource)(ElemGraphicsHeap, uint64_t, ElemGraphicsResourceInfo const *);
    void (*ElemFreeGraphicsResource)(ElemGraphicsResource, ElemFreeGraphicsResourceOptions const *);
    ElemGraphicsResourceInfo (*ElemGetGraphicsResourceInfo)(ElemGraphicsResource);
    void (*ElemUploadGraphicsBufferData)(ElemGraphicsResource, unsigned int, ElemDataSpan);
    ElemDataSpan (*ElemDownloadGraphicsBufferData)(ElemGraphicsResource, ElemDownloadGraphicsBufferDataOptions const *);
    void (*ElemCopyDataToGraphicsResource)(ElemCommandList, ElemCopyDataToGraphicsResourceParameters const *);
    ElemGraphicsResourceDescriptor (*ElemCreateGraphicsResourceDescriptor)(ElemGraphicsResource, ElemGraphicsResourceDescriptorUsage, ElemGraphicsResourceDescriptorOptions const *);
    ElemGraphicsResourceDescriptorInfo (*ElemGetGraphicsResourceDescriptorInfo)(ElemGraphicsResourceDescriptor);
    void (*ElemFreeGraphicsResourceDescriptor)(ElemGraphicsResourceDescriptor, ElemFreeGraphicsResourceDescriptorOptions const *);
    void (*ElemProcessGraphicsResourceDeleteQueue)(ElemGraphicsDevice);
    ElemGraphicsSampler (*ElemCreateGraphicsSampler)(ElemGraphicsDevice, ElemGraphicsSamplerInfo const *);
    ElemGraphicsSamplerInfo (*ElemGetGraphicsSamplerInfo)(ElemGraphicsSampler);
    void (*ElemFreeGraphicsSampler)(ElemGraphicsSampler, ElemFreeGraphicsSamplerOptions const *);
    ElemShaderLibrary (*ElemCreateShaderLibrary)(ElemGraphicsDevice, ElemDataSpan);
    void (*ElemFreeShaderLibrary)(ElemShaderLibrary);
    ElemPipelineState (*ElemCompileGraphicsPipelineState)(ElemGraphicsDevice, ElemGraphicsPipelineStateParameters const *);
    ElemPipelineState (*ElemCompileComputePipelineState)(ElemGraphicsDevice, ElemComputePipelineStateParameters const *);
    void (*ElemFreePipelineState)(ElemPipelineState);
    void (*ElemBindPipelineState)(ElemCommandList, ElemPipelineState);
    void (*ElemPushPipelineStateConstants)(ElemCommandList, unsigned int, ElemDataSpan);
    void (*ElemGraphicsResourceBarrier)(ElemCommandList, ElemGraphicsResourceDescriptor, ElemGraphicsResourceBarrierOptions const *);
    void (*ElemDispatchCompute)(ElemCommandList, unsigned int, unsigned int, unsigned int);
    void (*ElemBeginRenderPass)(ElemCommandList, ElemBeginRenderPassParameters const *);
    void (*ElemEndRenderPass)(ElemCommandList);
    void (*ElemSetViewport)(ElemCommandList, ElemViewport const *);
    void (*ElemSetViewports)(ElemCommandList, ElemViewportSpan);
    void (*ElemSetScissorRectangle)(ElemCommandList, ElemRectangle const *);
    void (*ElemSetScissorRectangles)(ElemCommandList, ElemRectangleSpan);
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
    listElementalFunctions.ElemIsFenceCompleted = (bool (*)(ElemFence))GetElementalFunctionPointer("ElemIsFenceCompleted");
    listElementalFunctions.ElemCreateSwapChain = (ElemSwapChain (*)(ElemCommandQueue, ElemWindow, ElemSwapChainUpdateHandlerPtr, ElemSwapChainOptions const *))GetElementalFunctionPointer("ElemCreateSwapChain");
    listElementalFunctions.ElemFreeSwapChain = (void (*)(ElemSwapChain))GetElementalFunctionPointer("ElemFreeSwapChain");
    listElementalFunctions.ElemGetSwapChainInfo = (ElemSwapChainInfo (*)(ElemSwapChain))GetElementalFunctionPointer("ElemGetSwapChainInfo");
    listElementalFunctions.ElemSetSwapChainTiming = (void (*)(ElemSwapChain, unsigned int, unsigned int))GetElementalFunctionPointer("ElemSetSwapChainTiming");
    listElementalFunctions.ElemPresentSwapChain = (void (*)(ElemSwapChain))GetElementalFunctionPointer("ElemPresentSwapChain");
    listElementalFunctions.ElemCreateGraphicsHeap = (ElemGraphicsHeap (*)(ElemGraphicsDevice, uint64_t, ElemGraphicsHeapOptions const *))GetElementalFunctionPointer("ElemCreateGraphicsHeap");
    listElementalFunctions.ElemFreeGraphicsHeap = (void (*)(ElemGraphicsHeap))GetElementalFunctionPointer("ElemFreeGraphicsHeap");
    listElementalFunctions.ElemCreateGraphicsBufferResourceInfo = (ElemGraphicsResourceInfo (*)(ElemGraphicsDevice, unsigned int, ElemGraphicsResourceUsage, ElemGraphicsResourceInfoOptions const *))GetElementalFunctionPointer("ElemCreateGraphicsBufferResourceInfo");
    listElementalFunctions.ElemCreateTexture2DResourceInfo = (ElemGraphicsResourceInfo (*)(ElemGraphicsDevice, unsigned int, unsigned int, unsigned int, ElemGraphicsFormat, ElemGraphicsResourceUsage, ElemGraphicsResourceInfoOptions const *))GetElementalFunctionPointer("ElemCreateTexture2DResourceInfo");
    listElementalFunctions.ElemCreateGraphicsResource = (ElemGraphicsResource (*)(ElemGraphicsHeap, uint64_t, ElemGraphicsResourceInfo const *))GetElementalFunctionPointer("ElemCreateGraphicsResource");
    listElementalFunctions.ElemFreeGraphicsResource = (void (*)(ElemGraphicsResource, ElemFreeGraphicsResourceOptions const *))GetElementalFunctionPointer("ElemFreeGraphicsResource");
    listElementalFunctions.ElemGetGraphicsResourceInfo = (ElemGraphicsResourceInfo (*)(ElemGraphicsResource))GetElementalFunctionPointer("ElemGetGraphicsResourceInfo");
    listElementalFunctions.ElemUploadGraphicsBufferData = (void (*)(ElemGraphicsResource, unsigned int, ElemDataSpan))GetElementalFunctionPointer("ElemUploadGraphicsBufferData");
    listElementalFunctions.ElemDownloadGraphicsBufferData = (ElemDataSpan (*)(ElemGraphicsResource, ElemDownloadGraphicsBufferDataOptions const *))GetElementalFunctionPointer("ElemDownloadGraphicsBufferData");
    listElementalFunctions.ElemCopyDataToGraphicsResource = (void (*)(ElemCommandList, ElemCopyDataToGraphicsResourceParameters const *))GetElementalFunctionPointer("ElemCopyDataToGraphicsResource");
    listElementalFunctions.ElemCreateGraphicsResourceDescriptor = (ElemGraphicsResourceDescriptor (*)(ElemGraphicsResource, ElemGraphicsResourceDescriptorUsage, ElemGraphicsResourceDescriptorOptions const *))GetElementalFunctionPointer("ElemCreateGraphicsResourceDescriptor");
    listElementalFunctions.ElemGetGraphicsResourceDescriptorInfo = (ElemGraphicsResourceDescriptorInfo (*)(ElemGraphicsResourceDescriptor))GetElementalFunctionPointer("ElemGetGraphicsResourceDescriptorInfo");
    listElementalFunctions.ElemFreeGraphicsResourceDescriptor = (void (*)(ElemGraphicsResourceDescriptor, ElemFreeGraphicsResourceDescriptorOptions const *))GetElementalFunctionPointer("ElemFreeGraphicsResourceDescriptor");
    listElementalFunctions.ElemProcessGraphicsResourceDeleteQueue = (void (*)(ElemGraphicsDevice))GetElementalFunctionPointer("ElemProcessGraphicsResourceDeleteQueue");
    listElementalFunctions.ElemCreateGraphicsSampler = (ElemGraphicsSampler (*)(ElemGraphicsDevice, ElemGraphicsSamplerInfo const *))GetElementalFunctionPointer("ElemCreateGraphicsSampler");
    listElementalFunctions.ElemGetGraphicsSamplerInfo = (ElemGraphicsSamplerInfo (*)(ElemGraphicsSampler))GetElementalFunctionPointer("ElemGetGraphicsSamplerInfo");
    listElementalFunctions.ElemFreeGraphicsSampler = (void (*)(ElemGraphicsSampler, ElemFreeGraphicsSamplerOptions const *))GetElementalFunctionPointer("ElemFreeGraphicsSampler");
    listElementalFunctions.ElemCreateShaderLibrary = (ElemShaderLibrary (*)(ElemGraphicsDevice, ElemDataSpan))GetElementalFunctionPointer("ElemCreateShaderLibrary");
    listElementalFunctions.ElemFreeShaderLibrary = (void (*)(ElemShaderLibrary))GetElementalFunctionPointer("ElemFreeShaderLibrary");
    listElementalFunctions.ElemCompileGraphicsPipelineState = (ElemPipelineState (*)(ElemGraphicsDevice, ElemGraphicsPipelineStateParameters const *))GetElementalFunctionPointer("ElemCompileGraphicsPipelineState");
    listElementalFunctions.ElemCompileComputePipelineState = (ElemPipelineState (*)(ElemGraphicsDevice, ElemComputePipelineStateParameters const *))GetElementalFunctionPointer("ElemCompileComputePipelineState");
    listElementalFunctions.ElemFreePipelineState = (void (*)(ElemPipelineState))GetElementalFunctionPointer("ElemFreePipelineState");
    listElementalFunctions.ElemBindPipelineState = (void (*)(ElemCommandList, ElemPipelineState))GetElementalFunctionPointer("ElemBindPipelineState");
    listElementalFunctions.ElemPushPipelineStateConstants = (void (*)(ElemCommandList, unsigned int, ElemDataSpan))GetElementalFunctionPointer("ElemPushPipelineStateConstants");
    listElementalFunctions.ElemGraphicsResourceBarrier = (void (*)(ElemCommandList, ElemGraphicsResourceDescriptor, ElemGraphicsResourceBarrierOptions const *))GetElementalFunctionPointer("ElemGraphicsResourceBarrier");
    listElementalFunctions.ElemDispatchCompute = (void (*)(ElemCommandList, unsigned int, unsigned int, unsigned int))GetElementalFunctionPointer("ElemDispatchCompute");
    listElementalFunctions.ElemBeginRenderPass = (void (*)(ElemCommandList, ElemBeginRenderPassParameters const *))GetElementalFunctionPointer("ElemBeginRenderPass");
    listElementalFunctions.ElemEndRenderPass = (void (*)(ElemCommandList))GetElementalFunctionPointer("ElemEndRenderPass");
    listElementalFunctions.ElemSetViewport = (void (*)(ElemCommandList, ElemViewport const *))GetElementalFunctionPointer("ElemSetViewport");
    listElementalFunctions.ElemSetViewports = (void (*)(ElemCommandList, ElemViewportSpan))GetElementalFunctionPointer("ElemSetViewports");
    listElementalFunctions.ElemSetScissorRectangle = (void (*)(ElemCommandList, ElemRectangle const *))GetElementalFunctionPointer("ElemSetScissorRectangle");
    listElementalFunctions.ElemSetScissorRectangles = (void (*)(ElemCommandList, ElemRectangleSpan))GetElementalFunctionPointer("ElemSetScissorRectangles");
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

static inline bool ElemIsFenceCompleted(ElemFence fence)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        bool result = {};
        #else
        bool result = (bool){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemIsFenceCompleted) 
    {
        assert(listElementalFunctions.ElemIsFenceCompleted);

        #ifdef __cplusplus
        bool result = {};
        #else
        bool result = (bool){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemIsFenceCompleted(fence);
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

static inline ElemGraphicsHeap ElemCreateGraphicsHeap(ElemGraphicsDevice graphicsDevice, uint64_t sizeInBytes, ElemGraphicsHeapOptions const * options)
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

static inline ElemGraphicsResourceInfo ElemCreateGraphicsBufferResourceInfo(ElemGraphicsDevice graphicsDevice, unsigned int sizeInBytes, ElemGraphicsResourceUsage usage, ElemGraphicsResourceInfoOptions const * options)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemGraphicsResourceInfo result = {};
        #else
        ElemGraphicsResourceInfo result = (ElemGraphicsResourceInfo){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemCreateGraphicsBufferResourceInfo) 
    {
        assert(listElementalFunctions.ElemCreateGraphicsBufferResourceInfo);

        #ifdef __cplusplus
        ElemGraphicsResourceInfo result = {};
        #else
        ElemGraphicsResourceInfo result = (ElemGraphicsResourceInfo){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemCreateGraphicsBufferResourceInfo(graphicsDevice, sizeInBytes, usage, options);
}

static inline ElemGraphicsResourceInfo ElemCreateTexture2DResourceInfo(ElemGraphicsDevice graphicsDevice, unsigned int width, unsigned int height, unsigned int mipLevels, ElemGraphicsFormat format, ElemGraphicsResourceUsage usage, ElemGraphicsResourceInfoOptions const * options)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemGraphicsResourceInfo result = {};
        #else
        ElemGraphicsResourceInfo result = (ElemGraphicsResourceInfo){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemCreateTexture2DResourceInfo) 
    {
        assert(listElementalFunctions.ElemCreateTexture2DResourceInfo);

        #ifdef __cplusplus
        ElemGraphicsResourceInfo result = {};
        #else
        ElemGraphicsResourceInfo result = (ElemGraphicsResourceInfo){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemCreateTexture2DResourceInfo(graphicsDevice, width, height, mipLevels, format, usage, options);
}

static inline ElemGraphicsResource ElemCreateGraphicsResource(ElemGraphicsHeap graphicsHeap, uint64_t graphicsHeapOffset, ElemGraphicsResourceInfo const * resourceInfo)
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

static inline void ElemFreeGraphicsResource(ElemGraphicsResource resource, ElemFreeGraphicsResourceOptions const * options)
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

    listElementalFunctions.ElemFreeGraphicsResource(resource, options);
}

static inline ElemGraphicsResourceInfo ElemGetGraphicsResourceInfo(ElemGraphicsResource resource)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemGraphicsResourceInfo result = {};
        #else
        ElemGraphicsResourceInfo result = (ElemGraphicsResourceInfo){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemGetGraphicsResourceInfo) 
    {
        assert(listElementalFunctions.ElemGetGraphicsResourceInfo);

        #ifdef __cplusplus
        ElemGraphicsResourceInfo result = {};
        #else
        ElemGraphicsResourceInfo result = (ElemGraphicsResourceInfo){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemGetGraphicsResourceInfo(resource);
}

static inline void ElemUploadGraphicsBufferData(ElemGraphicsResource resource, unsigned int offset, ElemDataSpan data)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemUploadGraphicsBufferData) 
    {
        assert(listElementalFunctions.ElemUploadGraphicsBufferData);
        return;
    }

    listElementalFunctions.ElemUploadGraphicsBufferData(resource, offset, data);
}

static inline ElemDataSpan ElemDownloadGraphicsBufferData(ElemGraphicsResource resource, ElemDownloadGraphicsBufferDataOptions const * options)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemDataSpan result = {};
        #else
        ElemDataSpan result = (ElemDataSpan){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemDownloadGraphicsBufferData) 
    {
        assert(listElementalFunctions.ElemDownloadGraphicsBufferData);

        #ifdef __cplusplus
        ElemDataSpan result = {};
        #else
        ElemDataSpan result = (ElemDataSpan){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemDownloadGraphicsBufferData(resource, options);
}

static inline void ElemCopyDataToGraphicsResource(ElemCommandList commandList, ElemCopyDataToGraphicsResourceParameters const * parameters)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemCopyDataToGraphicsResource) 
    {
        assert(listElementalFunctions.ElemCopyDataToGraphicsResource);
        return;
    }

    listElementalFunctions.ElemCopyDataToGraphicsResource(commandList, parameters);
}

static inline ElemGraphicsResourceDescriptor ElemCreateGraphicsResourceDescriptor(ElemGraphicsResource resource, ElemGraphicsResourceDescriptorUsage usage, ElemGraphicsResourceDescriptorOptions const * options)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemGraphicsResourceDescriptor result = {};
        #else
        ElemGraphicsResourceDescriptor result = (ElemGraphicsResourceDescriptor){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemCreateGraphicsResourceDescriptor) 
    {
        assert(listElementalFunctions.ElemCreateGraphicsResourceDescriptor);

        #ifdef __cplusplus
        ElemGraphicsResourceDescriptor result = {};
        #else
        ElemGraphicsResourceDescriptor result = (ElemGraphicsResourceDescriptor){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemCreateGraphicsResourceDescriptor(resource, usage, options);
}

static inline ElemGraphicsResourceDescriptorInfo ElemGetGraphicsResourceDescriptorInfo(ElemGraphicsResourceDescriptor descriptor)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemGraphicsResourceDescriptorInfo result = {};
        #else
        ElemGraphicsResourceDescriptorInfo result = (ElemGraphicsResourceDescriptorInfo){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemGetGraphicsResourceDescriptorInfo) 
    {
        assert(listElementalFunctions.ElemGetGraphicsResourceDescriptorInfo);

        #ifdef __cplusplus
        ElemGraphicsResourceDescriptorInfo result = {};
        #else
        ElemGraphicsResourceDescriptorInfo result = (ElemGraphicsResourceDescriptorInfo){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemGetGraphicsResourceDescriptorInfo(descriptor);
}

static inline void ElemFreeGraphicsResourceDescriptor(ElemGraphicsResourceDescriptor descriptor, ElemFreeGraphicsResourceDescriptorOptions const * options)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemFreeGraphicsResourceDescriptor) 
    {
        assert(listElementalFunctions.ElemFreeGraphicsResourceDescriptor);
        return;
    }

    listElementalFunctions.ElemFreeGraphicsResourceDescriptor(descriptor, options);
}

static inline void ElemProcessGraphicsResourceDeleteQueue(ElemGraphicsDevice graphicsDevice)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemProcessGraphicsResourceDeleteQueue) 
    {
        assert(listElementalFunctions.ElemProcessGraphicsResourceDeleteQueue);
        return;
    }

    listElementalFunctions.ElemProcessGraphicsResourceDeleteQueue(graphicsDevice);
}

static inline ElemGraphicsSampler ElemCreateGraphicsSampler(ElemGraphicsDevice graphicsDevice, ElemGraphicsSamplerInfo const * samplerInfo)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemGraphicsSampler result = {};
        #else
        ElemGraphicsSampler result = (ElemGraphicsSampler){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemCreateGraphicsSampler) 
    {
        assert(listElementalFunctions.ElemCreateGraphicsSampler);

        #ifdef __cplusplus
        ElemGraphicsSampler result = {};
        #else
        ElemGraphicsSampler result = (ElemGraphicsSampler){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemCreateGraphicsSampler(graphicsDevice, samplerInfo);
}

static inline ElemGraphicsSamplerInfo ElemGetGraphicsSamplerInfo(ElemGraphicsSampler sampler)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);

        #ifdef __cplusplus
        ElemGraphicsSamplerInfo result = {};
        #else
        ElemGraphicsSamplerInfo result = (ElemGraphicsSamplerInfo){0};
        #endif

        return result;
    }

    if (!listElementalFunctions.ElemGetGraphicsSamplerInfo) 
    {
        assert(listElementalFunctions.ElemGetGraphicsSamplerInfo);

        #ifdef __cplusplus
        ElemGraphicsSamplerInfo result = {};
        #else
        ElemGraphicsSamplerInfo result = (ElemGraphicsSamplerInfo){0};
        #endif

        return result;
    }

    return listElementalFunctions.ElemGetGraphicsSamplerInfo(sampler);
}

static inline void ElemFreeGraphicsSampler(ElemGraphicsSampler sampler, ElemFreeGraphicsSamplerOptions const * options)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemFreeGraphicsSampler) 
    {
        assert(listElementalFunctions.ElemFreeGraphicsSampler);
        return;
    }

    listElementalFunctions.ElemFreeGraphicsSampler(sampler, options);
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

static inline void ElemGraphicsResourceBarrier(ElemCommandList commandList, ElemGraphicsResourceDescriptor descriptor, ElemGraphicsResourceBarrierOptions const * options)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemGraphicsResourceBarrier) 
    {
        assert(listElementalFunctions.ElemGraphicsResourceBarrier);
        return;
    }

    listElementalFunctions.ElemGraphicsResourceBarrier(commandList, descriptor, options);
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

static inline void ElemSetScissorRectangle(ElemCommandList commandList, ElemRectangle const * rectangle)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemSetScissorRectangle) 
    {
        assert(listElementalFunctions.ElemSetScissorRectangle);
        return;
    }

    listElementalFunctions.ElemSetScissorRectangle(commandList, rectangle);
}

static inline void ElemSetScissorRectangles(ElemCommandList commandList, ElemRectangleSpan rectangles)
{
    if (!LoadElementalFunctionPointers()) 
    {
        assert(libraryElemental);
        return;
    }

    if (!listElementalFunctions.ElemSetScissorRectangles) 
    {
        assert(listElementalFunctions.ElemSetScissorRectangles);
        return;
    }

    listElementalFunctions.ElemSetScissorRectangles(commandList, rectangles);
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
