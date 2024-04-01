#include "DirectX12CommandList.h"
#include "DirectX12GraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define DIRECTX12_MAX_COMMANDQUEUES 10u
#define DIRECTX12_MAX_COMMANDLISTS 64u

SystemDataPool<DirectX12CommandQueueData, DirectX12CommandQueueDataFull> directX12CommandQueuePool;
SystemDataPool<DirectX12CommandListData, DirectX12CommandListDataFull> directX12CommandListPool;

HANDLE directX12GlobalFenceEvent;
uint32_t currentAllocatorIndex = 0; // TODO: To remove

// TODO: Review usage of full structures

void InitDirectX12CommandListMemory()
{
    if (!directX12CommandQueuePool.Storage)
    {
        directX12CommandQueuePool = SystemCreateDataPool<DirectX12CommandQueueData, DirectX12CommandQueueDataFull>(DirectX12MemoryArena, DIRECTX12_MAX_COMMANDQUEUES);
        directX12CommandListPool = SystemCreateDataPool<DirectX12CommandListData, DirectX12CommandListDataFull>(DirectX12MemoryArena, DIRECTX12_MAX_COMMANDLISTS);
        
        directX12GlobalFenceEvent = CreateEvent(nullptr, false, false, nullptr);
    }
}

DirectX12CommandQueueData* GetDirectX12CommandQueueData(ElemCommandQueue commandQueue)
{
    return SystemGetDataPoolItem(directX12CommandQueuePool, commandQueue);
}

DirectX12CommandQueueDataFull* GetDirectX12CommandQueueDataFull(ElemCommandQueue commandQueue)
{
    return SystemGetDataPoolItemFull(directX12CommandQueuePool, commandQueue);
}

DirectX12CommandListData* GetDirectX12CommandListData(ElemCommandList commandList)
{
    return SystemGetDataPoolItem(directX12CommandListPool, commandList);
}

DirectX12CommandListDataFull* GetDirectX12CommandListDataFull(ElemCommandList commandList)
{
    return SystemGetDataPoolItemFull(directX12CommandListPool, commandList);
}

ElemFence Direct3D12CreateCommandQueueFence(ElemCommandQueue commandQueue)
{
    auto commandQueueData = GetDirectX12CommandQueueData(commandQueue);
    SystemAssert(commandQueueData);

    auto commandQueueDataFull = GetDirectX12CommandQueueDataFull(commandQueue);
    SystemAssert(commandQueueDataFull);

    auto fenceValue = SystemAtomicAdd(commandQueueDataFull->FenceValue, 1) + 1;
    AssertIfFailed(commandQueueData->DeviceObject->Signal(commandQueueDataFull->Fence.Get(), fenceValue));

    return 
    {
        .CommandQueue = commandQueue,
        .FenceValue = fenceValue
    };
}

ElemCommandQueue DirectX12CreateCommandQueue(ElemGraphicsDevice graphicsDevice, ElemCommandQueueType type, const ElemCommandQueueOptions* options)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    
    InitDirectX12CommandListMemory();
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);
    
    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);
    
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    if (type == ElemCommandQueueType_Compute)
    {
        commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    }

    ComPtr<ID3D12CommandQueue> commandQueue;
    AssertIfFailedReturnNullHandle(graphicsDeviceData->Device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(commandQueue.GetAddressOf())));


    // TODO: To remove, temporary code!
    ComPtr<ID3D12CommandAllocator> commandAllocators[2];
    for (uint32_t i = 0; i < 2; i++)
    {
        AssertIfFailedReturnNullHandle(graphicsDeviceData->Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(commandAllocators[i].GetAddressOf())));
    }

    ComPtr<ID3D12Fence1> fence;
    AssertIfFailedReturnNullHandle(graphicsDeviceData->Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf())));

    if (DirectX12DebugLayerEnabled && options && options->DebugName)
    {
        commandQueue->SetName(SystemConvertUtf8ToWideChar(stackMemoryArena, options->DebugName).Pointer);
    }    

    auto handle = SystemAddDataPoolItem(directX12CommandQueuePool, {
        .DeviceObject = commandQueue,
        .Type = commandQueueDesc.Type
    }); 

    SystemAddDataPoolItemFull(directX12CommandQueuePool, handle, {
        .GraphicsDevice = graphicsDevice,
        .Fence = fence,
        .FenceValue = 0,
        .LastCompletedFenceValue = 0,
        .CommandAllocators = {commandAllocators[0], commandAllocators[1]}
    });

    return handle;
}

void DirectX12FreeCommandQueue(ElemCommandQueue commandQueue)
{
    SystemAssert(commandQueue != ELEM_HANDLE_NULL);

    auto commandQueueData = GetDirectX12CommandQueueData(commandQueue);
    SystemAssert(commandQueueData);

    auto commandQueueDataFull = GetDirectX12CommandQueueDataFull(commandQueue);
    SystemAssert(commandQueueDataFull);

    auto fence = Direct3D12CreateCommandQueueFence(commandQueue);
    ElemWaitForFenceOnCpu(fence);

    // TODO: Temporary
    for (uint32_t i = 0; i < 2; i++)
    {
        commandQueueDataFull->CommandAllocators[i].Reset();
    }

    commandQueueDataFull->Fence.Reset();
    commandQueueData->DeviceObject.Reset();

    SystemRemoveDataPoolItem(directX12CommandQueuePool, commandQueue);
}

ElemCommandList DirectX12GetCommandList(ElemCommandQueue commandQueue, const ElemCommandListOptions* options)
{
    SystemAssert(commandQueue != ELEM_HANDLE_NULL);

    auto commandQueueData = GetDirectX12CommandQueueData(commandQueue);
    SystemAssert(commandQueueData);

    auto commandQueueDataFull = GetDirectX12CommandQueueDataFull(commandQueue);
    SystemAssert(commandQueueDataFull);

    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(commandQueueDataFull->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    // TODO: We don't handle multi thread allocators yet.
    // TODO: For the moment, 2 in flights allocator for Direct queue only.

    // TODO: For the moment we reset the command allocator here but we need to do that in a separate function

    // TODO: This is really bad because we should reuse the command lists objects
    auto commandAllocator = commandQueueDataFull->CommandAllocators[currentAllocatorIndex % 2];
    commandAllocator->Reset();

    ComPtr<ID3D12GraphicsCommandList10> commandList;
    AssertIfFailedReturnNullHandle(graphicsDeviceData->Device->CreateCommandList1(0, commandQueueData->Type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(commandList.GetAddressOf())));
    AssertIfFailedReturnNullHandle(commandList->Reset(commandAllocator.Get(), nullptr));

    // TODO: Can we set the root signature for all types all the time?
    commandList->SetGraphicsRootSignature(graphicsDeviceData->RootSignature.Get());
    //commandList->SetComputeRootSignature(graphicsDeviceData->RootSignature.Get());

    auto handle = SystemAddDataPoolItem(directX12CommandListPool, {
        .DeviceObject = commandList,
    }); 

    SystemAddDataPoolItemFull(directX12CommandListPool, handle, {
    });

    return handle;
}

void DirectX12CommitCommandList(ElemCommandList commandList)
{    
    auto commandListData = GetDirectX12CommandListData(commandList);
    AssertIfFailed(commandListData->DeviceObject->Close());
}

ElemFence DirectX12ExecuteCommandLists(ElemCommandQueue commandQueue, ElemCommandListSpan commandLists, const ElemExecuteCommandListOptions* options)
{
    // TODO: Wait for fences if any

    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto commandListsToExecute = SystemPushArray<ID3D12CommandList*>(stackMemoryArena, DIRECTX12_MAX_COMMANDLISTS);

    for (uint32_t i = 0; i < commandLists.Length; i++)
    {
        auto commandListData = GetDirectX12CommandListData(commandLists.Items[i]);
        commandListsToExecute[i] = commandListData->DeviceObject.Get();
    }

    auto commandQueueData = GetDirectX12CommandQueueData(commandQueue);
    SystemAssert(commandQueueData);

    commandQueueData->DeviceObject->ExecuteCommandLists(commandLists.Length, commandListsToExecute.Pointer);

    // TODO: Signal fence
    ElemFence fence = 
    {
        .CommandQueue = ELEM_HANDLE_NULL,
        .FenceValue = 0
    };

    // TODO: Temporary
    currentAllocatorIndex++;

    // TODO: This is really bad because we should reuse the command lists objects
    for (uint32_t i = 0; i < commandLists.Length; i++)
    {
        auto commandListData = GetDirectX12CommandListData(commandLists.Items[i]);
        commandListData->DeviceObject.Reset();

        SystemRemoveDataPoolItem(directX12CommandListPool, commandLists.Items[i]);
    }

    return fence;
}

void DirectX12WaitForFenceOnCpu(ElemFence fence)
{
    SystemAssert(fence.CommandQueue != ELEM_HANDLE_NULL);

    auto commandQueueToWaitDataFull = GetDirectX12CommandQueueDataFull(fence.CommandQueue);
    SystemAssert(commandQueueToWaitDataFull);

    if (fence.FenceValue > commandQueueToWaitDataFull->LastCompletedFenceValue) 
    {
        commandQueueToWaitDataFull->LastCompletedFenceValue = SystemMax(commandQueueToWaitDataFull->LastCompletedFenceValue, commandQueueToWaitDataFull->Fence->GetCompletedValue());
    }

    if (fence.FenceValue > commandQueueToWaitDataFull->LastCompletedFenceValue)
    {
        SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Wait for Fence on CPU...");
        commandQueueToWaitDataFull->Fence->SetEventOnCompletion(fence.FenceValue, directX12GlobalFenceEvent);
        WaitForSingleObject(directX12GlobalFenceEvent, INFINITE);
    }
}
