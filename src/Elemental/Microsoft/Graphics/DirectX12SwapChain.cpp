#include "DirectX12SwapChain.h"
#include "DirectX12GraphicsDevice.h"
#include "DirectX12CommandList.h"
#include "DirectX12Texture.h"
#include "../Win32Application.h"
#include "../Win32Window.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define DIRECTX12_MAX_SWAPCHAINS 10u

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
    }
}

void ResizeDirectX12SwapChain(ElemSwapChain swapChain, uint32_t width, uint32_t height)
{
    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Resize Swapchain to %dx%d...", width, height);

    if (width == 0 || height == 0)
    {
        return;
    }
    
    SystemAssert(swapChain != ELEM_HANDLE_NULL);

    auto swapChainData = GetDirectX12SwapChainData(swapChain);
    SystemAssert(swapChainData);

    swapChainData->Width = width;
    swapChainData->Height = height;
 
    auto fence = Direct3D12CreateCommandQueueFence(swapChainData->CommandQueue);
    ElemWaitForFenceOnCpu(fence);
    
    for (uint32_t i = 0; i < DIRECTX12_MAX_SWAPCHAIN_BUFFERS; i++)
    {
        if (swapChainData->BackBufferTextures[i] != ELEM_HANDLE_NULL)
        {
            DirectX12FreeTexture(swapChainData->BackBufferTextures[i]);
        }
    } 

    AssertIfFailed(swapChainData->DeviceObject->ResizeBuffers(DIRECTX12_MAX_SWAPCHAIN_BUFFERS, width, height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT));
    CreateDirectX12SwapChainRenderTargetViews(swapChain);
}

void CheckDirectX12AvailableSwapChain(ElemHandle handle)
{
    SystemAssert(handle != ELEM_HANDLE_NULL);

    auto swapChainData = GetDirectX12SwapChainData(handle);
    SystemAssert(swapChainData);

    if (WaitForSingleObjectEx(swapChainData->WaitHandle, 0, true) == WAIT_OBJECT_0)
    {
        // START TIMING CALCULATION
        DXGI_FRAME_STATISTICS stats;

        // TODO: Precompute a max number of values
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        double ticksPerSecond = (double)frequency.QuadPart;

        // TODO: Fail the first time this is normal
        // TODO: Test with framelatency > 1

        LARGE_INTEGER syncQPCTime;
        
        // TODO: What happens if frame rendering goes further than refresh rate. Default latency is one so if we take longer, there will be 
        // dirctly a frame available. 
        // The wait will be ok directly and so the getframestats will contains the stat of the last present which maybe the same of the previous one of the frame that took
        // too much time. And maybe the previous present will not be presented yet.
        if (SUCCEEDED(swapChainData->DeviceObject->GetFrameStatistics(&stats)))
        {
            syncQPCTime.QuadPart= stats.SyncQPCTime.QuadPart - swapChainData->CreationTimestamp.QuadPart;
        }
        else 
        {
            LARGE_INTEGER currentTimeStamp;
            QueryPerformanceCounter(&currentTimeStamp);

            syncQPCTime.QuadPart = currentTimeStamp.QuadPart - swapChainData->CreationTimestamp.QuadPart;
        }

        // TODO: Get proper refresh interval
        double refreshInterval = 1.0 / 120.0;
        double intervalTicks = refreshInterval * ticksPerSecond;

        LARGE_INTEGER nextVSyncQPCTime;
        nextVSyncQPCTime.QuadPart = syncQPCTime.QuadPart + (LONGLONG)intervalTicks;

        double nextPresentTimeStampInSeconds = nextVSyncQPCTime.QuadPart / ticksPerSecond;

        auto deltaTime = (nextVSyncQPCTime.QuadPart - swapChainData->PreviousTargetPresentationTimestamp.QuadPart) / ticksPerSecond;
        swapChainData->PreviousTargetPresentationTimestamp = nextVSyncQPCTime;
        // END TIMING CALCULATION
    
        ElemWindowSize windowSize = ElemGetWindowRenderSize(swapChainData->Window);

        if (windowSize.Width > 0 && windowSize.Height > 0 && (windowSize.Width != swapChainData->Width || windowSize.Height != swapChainData->Height))
        {
            ResizeDirectX12SwapChain(handle, windowSize.Width, windowSize.Height);
        }

        auto backBufferTexture = swapChainData->BackBufferTextures[swapChainData->DeviceObject->GetCurrentBackBufferIndex()];

        ElemSwapChainUpdateParameters updateParameters
        {
            .SwapChainInfo = DirectX12GetSwapChainInfo(handle),
            .BackBufferTexture = backBufferTexture,
            .DeltaTimeInSeconds = deltaTime,
            .NextPresentTimeStampInSeconds = nextPresentTimeStampInSeconds
        };
        
        swapChainData->UpdateHandler(&updateParameters, swapChainData->UpdatePayload);
    }
}

ElemSwapChain DirectX12CreateSwapChain(ElemCommandQueue commandQueue, ElemWindow window, ElemSwapChainUpdateHandlerPtr updateHandler, const ElemSwapChainOptions* options)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    InitDirectX12SwapChainMemory();
    SystemAssert(commandQueue != ELEM_HANDLE_NULL);
    
    auto commandQueueData = GetDirectX12CommandQueueData(commandQueue);
    SystemAssert(commandQueueData);

    auto commandQueueDataFull = GetDirectX12CommandQueueDataFull(commandQueue);
    SystemAssert(commandQueueDataFull);

    auto windowData = GetWin32WindowData(window);
    SystemAssert(windowData);

    auto windowRenderSize = ElemGetWindowRenderSize(window);
    auto width = windowRenderSize.Width;
    auto height = windowRenderSize.Height;
    auto format = DXGI_FORMAT_B8G8R8A8_UNORM; // TODO: Enumerate compatible formats first
    auto frameLatency = 1u;
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

        if (options->UpdatePayload)
        {
            updatePayload = options->UpdatePayload;
        }
    }

    // TODO: Support VRR
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
        .AlphaMode = DXGI_ALPHA_MODE_IGNORE,
        .Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
    };

    // TODO: Support Fullscreen exclusive in the future
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullScreenDesc =
    {
        .Windowed = true
    };

    ComPtr<IDXGISwapChain4> swapChain;
    AssertIfFailedReturnNullHandle(DxgiFactory->CreateSwapChainForHwnd(commandQueueData->DeviceObject.Get(), windowData->WindowHandle, &swapChainDesc, &swapChainFullScreenDesc, nullptr, (IDXGISwapChain1**)swapChain.GetAddressOf()));  
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

    auto handle = SystemAddDataPoolItem(directX12SwapChainPool, {
        .DeviceObject = swapChain,
        .CommandQueue = commandQueue,
        .Window = window,
        .WaitHandle = waitHandle,
        .UpdateHandler = updateHandler,
        .UpdatePayload = updatePayload,
        .CreationTimestamp = creationTimestamp,
        .PreviousTargetPresentationTimestamp = {},
        .Width = width,
        .Height = height,
        .Format = ElemTextureFormat_B8G8R8A8_SRGB // TODO: change that
    }); 

    SystemAddDataPoolItemFull(directX12SwapChainPool, handle, {
        .GraphicsDevice = commandQueueDataFull->GraphicsDevice,
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

    auto fence = Direct3D12CreateCommandQueueFence(swapChainData->CommandQueue);

    // BUG: If wait for swapchain is not called on the client code each frame this line block inf
    // Note that in the test an empty command list was used, that may be the problem
    ElemWaitForFenceOnCpu(fence);

    for (uint32_t i = 0; i < DIRECTX12_MAX_SWAPCHAIN_BUFFERS; i++)
    {
        if (swapChainData->BackBufferTextures[i])
        {
            DirectX12FreeTexture(swapChainData->BackBufferTextures[i]);
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
        .Format = swapChainData->Format
    };
}

void DirectX12PresentSwapChain(ElemSwapChain swapChain)
{
    SystemAssert(swapChain != ELEM_HANDLE_NULL);

    auto swapChainData = GetDirectX12SwapChainData(swapChain);
    SystemAssert(swapChainData);

    // TODO: Review params
    AssertIfFailed(swapChainData->DeviceObject->Present(1, 0));
    
    // TODO: Reset command allocation
    //Direct3D12ResetCommandAllocation(swapChain->GraphicsDevice);
}
