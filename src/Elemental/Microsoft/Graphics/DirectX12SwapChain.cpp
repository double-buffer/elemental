#include "DirectX12SwapChain.h"
#include "DirectX12GraphicsDevice.h"
#include "DirectX12CommandList.h"
#include "DirectX12Texture.h"
#include "../Win32Window.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define DIRECTX12_MAX_SWAPCHAINS 10u

SystemDataPool<DirectX12SwapChainData, DirectX12SwapChainDataFull> directX12SwapChainPool;

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

ElemSwapChain DirectX12CreateSwapChain(ElemCommandQueue commandQueue, ElemWindow window, const ElemSwapChainOptions* options)
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
    auto maximumFrameLatency = 1u;
 
    if (options)
    {
        if (options->Width != 0)
        {
            width = options->Width;
        }

        if (options->Height != 0)
        {
            height = options->Height;
        }

        if (options->Format == ElemSwapChainFormat_HighDynamicRange)
        {
            format = DXGI_FORMAT_R10G10B10A2_UNORM;
        }

        // TODO: Check boundaries
        if (options->MaximumFrameLatency != 0)
        {
            maximumFrameLatency = options->MaximumFrameLatency;
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

    swapChain->SetMaximumFrameLatency(maximumFrameLatency);
    auto waitHandle = swapChain->GetFrameLatencyWaitableObject();

    auto handle = SystemAddDataPoolItem(directX12SwapChainPool, {
        .DeviceObject = swapChain,
        .WaitHandle = waitHandle
    }); 

    SystemAddDataPoolItemFull(directX12SwapChainPool, handle, {
        .GraphicsDevice = commandQueueDataFull->GraphicsDevice,
        .CommandQueue = commandQueue
    });

    CreateDirectX12SwapChainRenderTargetViews(handle);
    
    return handle;
}

void DirectX12FreeSwapChain(ElemSwapChain swapChain)
{
    SystemAssert(swapChain != ELEM_HANDLE_NULL);

    auto swapChainData = GetDirectX12SwapChainData(swapChain);
    SystemAssert(swapChainData);

    auto swapChainDataFull = GetDirectX12SwapChainDataFull(swapChain);
    SystemAssert(swapChainDataFull);

    auto fence = Direct3D12CreateCommandQueueFence(swapChainDataFull->CommandQueue);

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
}

void DirectX12ResizeSwapChain(ElemSwapChain swapChain, uint32_t width, uint32_t height)
{
}

ElemTexture DirectX12GetSwapChainBackBufferTexture(ElemSwapChain swapChain)
{
    SystemAssert(swapChain != ELEM_HANDLE_NULL);

    auto swapChainData = GetDirectX12SwapChainData(swapChain);
    SystemAssert(swapChainData);

    return swapChainData->BackBufferTextures[swapChainData->DeviceObject->GetCurrentBackBufferIndex()];
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

void DirectX12WaitForSwapChainOnCpu(ElemSwapChain swapChain)
{
    SystemAssert(swapChain != ELEM_HANDLE_NULL);

    auto swapChainData = GetDirectX12SwapChainData(swapChain);
    SystemAssert(swapChainData);

    if (WaitForSingleObjectEx(swapChainData->WaitHandle, 1000, true) == WAIT_TIMEOUT)
    {
        SystemAssert("Wait for SwapChain timeout");
    }
}
