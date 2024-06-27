#include "DirectX12SwapChain.h"
#include "DirectX12GraphicsDevice.h"
#include "DirectX12CommandList.h"
#include "DirectX12Resource.h"
#include "Inputs/Inputs.h"
#include "../Win32Application.h"
#include "../Win32Window.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define DIRECTX12_MAX_SWAPCHAINS 10u

// TODO: Without FPS limit we can maybe debug UAV Barrier
#define ENABLE_TEARING 0

SystemDataPool<DirectX12SwapChainData, DirectX12SwapChainDataFull> directX12SwapChainPool;

// TODO: In the near future we should switch to composition swap chains. https://learn.microsoft.com/en-us/windows/win32/comp_swapchain/comp-swapchain-examples
// Currently they are not compatible with DX12. 

void InitDirectX12SwapChainMemory()
{
    if (!directX12SwapChainPool.Storage)
    {
        directX12SwapChainPool = SystemCreateDataPool<DirectX12SwapChainData, DirectX12SwapChainDataFull>(DirectX12MemoryArena, DIRECTX12_MAX_SWAPCHAINS);
    }
}

DirectX12SwapChainData* GetDirectX12SwapChainData(ElemSwapChain swapChain)
{
    return SystemGetDataPoolItem(directX12SwapChainPool, swapChain);
}

DirectX12SwapChainDataFull* GetDirectX12SwapChainDataFull(ElemSwapChain swapChain)
{
    return SystemGetDataPoolItemFull(directX12SwapChainPool, swapChain);
}

void CreateDirectX12SwapChainRenderTargetViews(ElemSwapChain swapChain)
{
    SystemAssert(swapChain != ELEM_HANDLE_NULL);

    auto swapChainData = GetDirectX12SwapChainData(swapChain);
    SystemAssert(swapChainData);

    auto swapChainDataFull = GetDirectX12SwapChainDataFull(swapChain);
    SystemAssert(swapChainDataFull);

    for (uint32_t i = 0; i < DIRECTX12_MAX_SWAPCHAIN_BUFFERS; i++)
    {
        ComPtr<ID3D12Resource> backBuffer;
        AssertIfFailed(swapChainData->DeviceObject->GetBuffer(i, IID_PPV_ARGS(backBuffer.GetAddressOf())));

        swapChainData->BackBufferTextures[i] = CreateDirectX12TextureFromResource(swapChainDataFull->GraphicsDevice, backBuffer, true);
        swapChainData->BackBufferDescriptors[i] = DirectX12CreateGraphicsResourceDescriptor(swapChainData->BackBufferTextures[i], ElemGraphicsResourceUsage_RenderTarget, nullptr);
    }
}

void ResizeDirectX12SwapChain(ElemSwapChain swapChain, uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
    {
        return;
    }
    
    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Resize Swapchain to %dx%d.", width, height);
    SystemAssert(swapChain != ELEM_HANDLE_NULL);

    auto swapChainData = GetDirectX12SwapChainData(swapChain);
    SystemAssert(swapChainData);

    swapChainData->Width = width;
    swapChainData->Height = height;
    swapChainData->AspectRatio = (float)width / height;
 
    auto fence = CreateDirectX12CommandQueueFence(swapChainData->CommandQueue);
    ElemWaitForFenceOnCpu(fence);
    
    for (uint32_t i = 0; i < DIRECTX12_MAX_SWAPCHAIN_BUFFERS; i++)
    {
        if (swapChainData->BackBufferTextures[i] != ELEM_HANDLE_NULL)
        {
            DirectX12FreeGraphicsResourceDescriptor(swapChainData->BackBufferDescriptors[i], nullptr);
            DirectX12FreeGraphicsResource(swapChainData->BackBufferTextures[i], nullptr);
        }
    } 
    #if ENABLE_TEARING
    AssertIfFailed(swapChainData->DeviceObject->ResizeBuffers(DIRECTX12_MAX_SWAPCHAIN_BUFFERS, width, height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING));
    #else
    AssertIfFailed(swapChainData->DeviceObject->ResizeBuffers(DIRECTX12_MAX_SWAPCHAIN_BUFFERS, width, height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT));
    #endif
    CreateDirectX12SwapChainRenderTargetViews(swapChain);
}

uint64_t currentDxgiMessageCount = 0;

uint64_t lastVsyncCounter;

void CheckDirectX12AvailableSwapChain(ElemHandle handle)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    SystemAssert(handle != ELEM_HANDLE_NULL);

    auto swapChainData = GetDirectX12SwapChainData(handle);
    SystemAssert(swapChainData);

    auto windowData = GetWin32WindowData(swapChainData->Window);
    SystemAssert(windowData);

    if (DirectX12DebugLayerEnabled)
    {
        auto messageCount = DxgiInfoQueue->GetNumStoredMessages(DXGI_DEBUG_ALL);

        for (uint64_t i = currentDxgiMessageCount; i < messageCount; i++)
		{
			SIZE_T messageLength;
			AssertIfFailed(DxgiInfoQueue->GetMessage(DXGI_DEBUG_ALL, i, nullptr, &messageLength));

			auto messageData = (DXGI_INFO_QUEUE_MESSAGE*)SystemPushArray<uint8_t>(stackMemoryArena, messageLength).Pointer;
			// get the message and push its description into the vector
			AssertIfFailed(DxgiInfoQueue->GetMessage(DXGI_DEBUG_ALL, i, messageData, &messageLength));

            auto messageType = ElemLogMessageType_Debug;

            if (messageData->Severity == DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING)
            {
                messageType = ElemLogMessageType_Warning;
            }
            else if(messageData->Severity == DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR || messageData->Severity == DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION)
            {
                messageType = ElemLogMessageType_Error;
            }

            SystemLogMessage(messageType, ElemLogMessageCategory_Graphics, "%s", messageData->pDescription);
            currentDxgiMessageCount = messageCount;
		}
    }

    if (WaitForSingleObjectEx(swapChainData->WaitHandle, 1000, false) == WAIT_OBJECT_0)
    {
        // TODO: First calculation of firt delta time. For the moment it is done again the creation of swap chain time.

        // START TIMING CALCULATION
        DXGI_FRAME_STATISTICS stats;

        // TODO: Precompute a max number of values
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        double ticksPerSecond = (double)frequency.QuadPart;

        // TODO: Fail the first time this is normal
        // TODO: Test with framelatency > 1

        LARGE_INTEGER syncQPCTime;
        uint64_t vSyncDelta = 1;
        
        // TODO: What happens if frame rendering goes further than refresh rate. Default latency is one so if we take longer, there will be 
        // dirctly a frame available. 
        // The wait will be ok directly and so the getframestats will contains the stat of the last present which maybe the same of the previous one of the frame that took
        // too much time. And maybe the previous present will not be presented yet.

        //windowData->Output->WaitForVBlank();


        if (SUCCEEDED(swapChainData->DeviceObject->GetFrameStatistics(&stats)) && stats.PresentRefreshCount > 0)
        {
            syncQPCTime.QuadPart = stats.SyncQPCTime.QuadPart;

            if (lastVsyncCounter > 0)
            {
                vSyncDelta = stats.PresentRefreshCount - lastVsyncCounter;
            }
                      
            lastVsyncCounter = stats.PresentRefreshCount;
            //SystemLogWarningMessage(ElemLogMessageCategory_Graphics, "Last Present: %u, %d - %d %d", stats.SyncQPCTime.QuadPart, stats.PresentCount, vSyncDelta, stats.PresentRefreshCount);
        }
        else 
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Frame statistics failed");
            LARGE_INTEGER currentTimestamp;
            QueryPerformanceCounter(&currentTimestamp);

            syncQPCTime.QuadPart = currentTimestamp.QuadPart - Win32PerformanceCounterStart;
        }

        double refreshInterval = (1.0 / windowData->MonitorRefreshRate);
        double intervalTicks = refreshInterval * ticksPerSecond;

        LARGE_INTEGER nextVSyncQPCTime;
        nextVSyncQPCTime.QuadPart = syncQPCTime.QuadPart + (LONGLONG)intervalTicks;

        double nextPresentTimestampInSeconds = nextVSyncQPCTime.QuadPart / ticksPerSecond;
      

        // TODO: If delta time is above a thresold, take the delta time based on target FPS
        auto deltaTime = vSyncDelta * refreshInterval;//(nextVSyncQPCTime.QuadPart - swapChainData->PreviousTargetPresentationTimestamp.QuadPart) / ticksPerSecond;
        swapChainData->PreviousTargetPresentationTimestamp = nextVSyncQPCTime;

        if (vSyncDelta > 1)
        {
            SystemLogWarningMessage(ElemLogMessageCategory_Graphics, "Missed %d VSync! Delta time is %f ms", vSyncDelta - 1, deltaTime * 1000);
        }
        // END TIMING CALCULATION

        ElemWindowSize windowSize = ElemGetWindowRenderSize(swapChainData->Window);

        auto sizeChanged = false;

        if (windowSize.Width > 0 && windowSize.Height > 0 && (windowSize.Width != swapChainData->Width || windowSize.Height != swapChainData->Height))
        {
            ResizeDirectX12SwapChain(handle, windowSize.Width, windowSize.Height);
            sizeChanged = true;
        }
        
        swapChainData->PresentCalled = false;
        auto backBufferDescriptor = swapChainData->BackBufferDescriptors[swapChainData->DeviceObject->GetCurrentBackBufferIndex()];

        ElemSwapChainUpdateParameters updateParameters
        {
            .SwapChainInfo = DirectX12GetSwapChainInfo(handle),
            .BackBufferRenderTarget = backBufferDescriptor,
            //.DeltaTimeInSeconds = 1.0f / swapChainData->TargetFPS,
            .DeltaTimeInSeconds = deltaTime,
                //TODO: Fix next present it is wrong now
            .NextPresentTimestampInSeconds = nextPresentTimestampInSeconds,
            .SizeChanged = sizeChanged
        };
        
        swapChainData->UpdateHandler(&updateParameters, swapChainData->UpdatePayload);
        ResetInputsFrame();

        if (!swapChainData->PresentCalled)
        {
            SystemLogWarningMessage(ElemLogMessageCategory_Graphics, "Present was not called during update.");
            DirectX12PresentSwapChain(handle);
        }

        #if ENABLE_TEARING
        LARGE_INTEGER currentCounter;
        QueryPerformanceCounter(&currentCounter);

        LARGE_INTEGER remainingTime;
        remainingTime.QuadPart = -(nextVSyncQPCTime.QuadPart - currentCounter.QuadPart); 

        auto testDebug = (remainingTime.QuadPart / ticksPerSecond) * 1000.0;
        //SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Wait for: %f, Next Present: %u, Current: %u", testDebug, nextVSyncQPCTime.QuadPart, currentCounter.QuadPart);

        if (testDebug > -20 && testDebug < 0)
        {
            SystemAssert(SetWaitableTimerEx(swapChainData->TimerHandle, &remainingTime, 0, nullptr, nullptr, nullptr, FALSE));

            if (WaitForSingleObject(swapChainData->TimerHandle, INFINITE) != WAIT_OBJECT_0) 
            {
            }

            QueryPerformanceCounter(&currentCounter);
            //SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "After Wait: %u", currentCounter.QuadPart);
        }
        #endif
    }
}

ElemSwapChain DirectX12CreateSwapChain(ElemCommandQueue commandQueue, ElemWindow window, ElemSwapChainUpdateHandlerPtr updateHandler, const ElemSwapChainOptions* options)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    InitDirectX12SwapChainMemory();
    SystemAssert(commandQueue != ELEM_HANDLE_NULL);
    
    auto commandQueueData = GetDirectX12CommandQueueData(commandQueue);
    SystemAssert(commandQueueData);

    auto windowData = GetWin32WindowData(window);
    SystemAssert(windowData);

    auto windowRenderSize = ElemGetWindowRenderSize(window);
    auto width = windowRenderSize.Width;
    auto height = windowRenderSize.Height;
    //auto format = DXGI_FORMAT_B8G8R8A8_UNORM; // TODO: Enumerate compatible formats first
    auto format = DXGI_FORMAT_R8G8B8A8_UNORM; // TODO: Enumerate compatible formats first
    auto frameLatency = 1u;

    auto targetFPS = windowData->MonitorRefreshRate;
    void* updatePayload = nullptr;
 
    if (options)
    {
        if (options->Format == ElemSwapChainFormat_HighDynamicRange)
        {
            format = DXGI_FORMAT_R10G10B10A2_UNORM;
        }

        // TODO: Check boundaries
        if (options->FrameLatency != 0)
        {
            frameLatency = options->FrameLatency;
        }

        if (options->TargetFPS != 0)
        {
            targetFPS = options->TargetFPS;
        }

        if (options->UpdatePayload)
        {
            updatePayload = options->UpdatePayload;
        }
    }

    // TODO: Do we want a vsync parameter? 
    // TODO: Maybe we can go on allowtearing with present 0 and Sleep the remaining of the frame to sync nearly with vsync?
    // On non VRR displays this can maybe cause some tearing...

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc =
    {
        .Width = width,
        .Height = height,
        .Format = format,
        .SampleDesc = { 1, 0 },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = DIRECTX12_MAX_SWAPCHAIN_BUFFERS,
        .Scaling = DXGI_SCALING_STRETCH,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        //.AlphaMode = DXGI_ALPHA_MODE_IGNORE,
        #if ENABLE_TEARING
        .Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
        #else
        .Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT        
        #endif
    };

    ComPtr<IDXGISwapChain4> swapChain;
    AssertIfFailedReturnNullHandle(DxgiFactory->CreateSwapChainForHwnd(commandQueueData->DeviceObject.Get(), windowData->WindowHandle, &swapChainDesc, nullptr, nullptr, (IDXGISwapChain1**)swapChain.GetAddressOf()));  
    AssertIfFailedReturnNullHandle(DxgiFactory->MakeWindowAssociation(windowData->WindowHandle, DXGI_MWA_NO_ALT_ENTER));

    if (options && options->Format == ElemSwapChainFormat_HighDynamicRange)
    {
        // TODO: See https://learn.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range
        // TODO: See https://panoskarabelas.com/blog/posts/hdr_in_under_10_minutes/
        swapChain->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
    }

    swapChain->SetMaximumFrameLatency(frameLatency);
    auto waitHandle = swapChain->GetFrameLatencyWaitableObject();

    LARGE_INTEGER creationTimestamp;
    QueryPerformanceCounter(&creationTimestamp);

    auto timerHandle = CreateWaitableTimerEx(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    SystemAssert(timerHandle);

    auto handle = SystemAddDataPoolItem(directX12SwapChainPool, {
        .DeviceObject = swapChain,
        .CommandQueue = commandQueue,
        .Window = window,
        .WaitHandle = waitHandle,
        .UpdateHandler = updateHandler,
        .UpdatePayload = updatePayload,
        .PreviousTargetPresentationTimestamp = {},
        .TimerHandle = timerHandle,
        .Width = width,
        .Height = height,
        .AspectRatio = (float)width / height,
        .Format = ElemGraphicsFormat_B8G8R8A8_SRGB, // TODO: change that
        .FrameLatency = frameLatency,
        .TargetFPS = targetFPS
    }); 

    SystemAddDataPoolItemFull(directX12SwapChainPool, handle, {
        .GraphicsDevice = commandQueueData->GraphicsDevice,
    });

    CreateDirectX12SwapChainRenderTargetViews(handle);
    AddWin32RunLoopHandler(CheckDirectX12AvailableSwapChain, handle);
    
    return handle;
}

void DirectX12FreeSwapChain(ElemSwapChain swapChain)
{
    SystemAssert(swapChain != ELEM_HANDLE_NULL);

    auto swapChainData = GetDirectX12SwapChainData(swapChain);
    SystemAssert(swapChainData);

    auto fence = CreateDirectX12CommandQueueFence(swapChainData->CommandQueue);
    ElemWaitForFenceOnCpu(fence);

    for (uint32_t i = 0; i < DIRECTX12_MAX_SWAPCHAIN_BUFFERS; i++)
    {
        if (swapChainData->BackBufferTextures[i])
        {
            DirectX12FreeGraphicsResourceDescriptor(swapChainData->BackBufferDescriptors[i], nullptr);
            DirectX12FreeGraphicsResource(swapChainData->BackBufferTextures[i], nullptr);
        }
    }

    if (swapChainData->DeviceObject)
    {
        swapChainData->DeviceObject.Reset();
    }

    SystemRemoveDataPoolItem(directX12SwapChainPool, swapChain);
}

ElemSwapChainInfo DirectX12GetSwapChainInfo(ElemSwapChain swapChain)
{
    SystemAssert(swapChain != ELEM_HANDLE_NULL);

    auto swapChainData = GetDirectX12SwapChainData(swapChain);
    SystemAssert(swapChainData);

    return 
    {
        .Width = swapChainData->Width,
        .Height = swapChainData->Height,
        .AspectRatio = swapChainData->AspectRatio,
        .Format = swapChainData->Format
    };
}

void DirectX12SetSwapChainTiming(ElemSwapChain swapChain, uint32_t frameLatency, uint32_t targetFPS)
{
    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Set Swapchain timing: FrameLatency: %d, TargetFPS: %d.", frameLatency, targetFPS);

    SystemAssert(swapChain != ELEM_HANDLE_NULL);

    auto swapChainData = GetDirectX12SwapChainData(swapChain);
    SystemAssert(swapChainData);

    if (swapChainData->FrameLatency != frameLatency)
    {
        swapChainData->FrameLatency = frameLatency;
        swapChainData->DeviceObject->SetMaximumFrameLatency(frameLatency);
    }
    
    swapChainData->TargetFPS = targetFPS;
}

void DirectX12PresentSwapChain(ElemSwapChain swapChain)
{
    SystemAssert(swapChain != ELEM_HANDLE_NULL);

    auto swapChainData = GetDirectX12SwapChainData(swapChain);
    SystemAssert(swapChainData);
    
    auto swapChainDataFull = GetDirectX12SwapChainDataFull(swapChain);
    SystemAssert(swapChainDataFull);

    auto windowData = GetWin32WindowData(swapChainData->Window);
    SystemAssert(windowData);
    
    swapChainData->PresentCalled = true;

    // TODO: Compute vsync interval based on target fps (rework that, it only works for multiples for now!)
    // TODO: We need to take into account the current present time with the nextPresentTime computed from update function
    // For example, if we present and we missed a vsync, the calculation should take that into account
    auto vsyncInteval = windowData->MonitorRefreshRate / swapChainData->TargetFPS;

    // TODO: Control the next vsync for frame pacing (eg: running at 30fps on a 120hz screen)
    //AssertIfFailed(swapChainData->DeviceObject->Present(vsyncInteval, 0));
    #if ENABLE_TEARING
    AssertIfFailed(swapChainData->DeviceObject->Present(0, DXGI_PRESENT_ALLOW_TEARING));
    #else
    AssertIfFailed(swapChainData->DeviceObject->Present(1, 0));
    #endif
    
    DirectX12ResetCommandAllocation(swapChainDataFull->GraphicsDevice);
}
