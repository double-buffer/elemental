#include "DirectX12CommandList.h"
#include "DirectX12GraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define DIRECTX12_MAX_COMMANDQUEUES 10u
#define DIRECTX12_MAX_COMMANDLISTS 64u

SystemDataPool<DirectX12CommandQueueData, DirectX12CommandQueueDataFull> directX12CommandQueuePool;
SystemDataPool<DirectX12CommandListData, DirectX12CommandListDataFull> directX12CommandListPool;

thread_local CommandAllocatorDevicePool<ID3D12CommandAllocator*, ID3D12GraphicsCommandList10*> threadDirectX12DeviceCommandPools[DIRECTX12_MAX_DEVICES];
thread_local bool threadDirectX12CommandBufferCommitted = true;

HANDLE directX12GlobalFenceEvent;

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

ElemFence CreateDirectX12CommandQueueFence(ElemCommandQueue commandQueue)
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
    auto commandAllocatorQueueType = CommandAllocatorQueueType_Graphics;

    if (type == ElemCommandQueueType_Compute)
    {
        commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
        commandAllocatorQueueType = CommandAllocatorQueueType_Compute;
    }

    ComPtr<ID3D12CommandQueue> commandQueue;
    AssertIfFailedReturnNullHandle(graphicsDeviceData->Device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(commandQueue.GetAddressOf())));

    ComPtr<ID3D12Fence1> fence;
    AssertIfFailedReturnNullHandle(graphicsDeviceData->Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf())));

    if (DirectX12DebugLayerEnabled && options && options->DebugName)
    {
        commandQueue->SetName(SystemConvertUtf8ToWideChar(stackMemoryArena, options->DebugName).Pointer);
    }

    auto commandAllocators = SystemPushArray<ComPtr<ID3D12CommandAllocator>>(DirectX12MemoryArena, DIRECTX12_MAX_COMMANDLISTS);
    auto commandLists = SystemPushArray<ComPtr<ID3D12GraphicsCommandList10>>(DirectX12MemoryArena, DIRECTX12_MAX_COMMANDLISTS * DIRECTX12_MAX_COMMANDLISTS);

    auto handle = SystemAddDataPoolItem(directX12CommandQueuePool, {
        .DeviceObject = commandQueue,
        .Type = commandQueueDesc.Type,
        .CommandAllocatorQueueType = commandAllocatorQueueType,
        .GraphicsDevice = graphicsDevice,
    }); 

    SystemAddDataPoolItemFull(directX12CommandQueuePool, handle, {
        .Fence = fence,
        .FenceValue = 0,
        .LastCompletedFenceValue = 0,
        .CommandAllocators = commandAllocators,
        .CommandLists = commandLists
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

    auto fence = CreateDirectX12CommandQueueFence(commandQueue);
    ElemWaitForFenceOnCpu(fence);

    commandQueueDataFull->Fence.Reset();
    commandQueueData->DeviceObject.Reset();

    for (uint32_t i = 0; i < commandQueueDataFull->CurrentCommandAllocatorIndex; i++)
    {
        commandQueueDataFull->CommandAllocators[i].Reset();
    }

    for (uint32_t i = 0; i < commandQueueDataFull->CurrentCommandListIndex; i++)
    {
        commandQueueDataFull->CommandLists[i].Reset();
    }

    auto graphicsIdUnpacked = UnpackSystemDataPoolHandle(commandQueueData->GraphicsDevice);
    threadDirectX12DeviceCommandPools[graphicsIdUnpacked.Index] = {};

    SystemRemoveDataPoolItem(directX12CommandQueuePool, commandQueue);
}

void DirectX12ResetCommandAllocation(ElemGraphicsDevice graphicsDevice)
{
    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    graphicsDeviceData->CommandAllocationGeneration++;
    threadDirectX12CommandBufferCommitted = true;
}

ElemCommandList DirectX12GetCommandList(ElemCommandQueue commandQueue, const ElemCommandListOptions* options)
{
    if (!threadDirectX12CommandBufferCommitted)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Cannot get a command list if commit was not called on the same thread.");
        return ELEM_HANDLE_NULL;
    }

    auto stackMemoryArena = SystemGetStackMemoryArena();

    SystemAssert(commandQueue != ELEM_HANDLE_NULL);

    auto commandQueueData = GetDirectX12CommandQueueData(commandQueue);
    SystemAssert(commandQueueData);

    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(commandQueueData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto graphicsIdUnpacked = UnpackSystemDataPoolHandle(commandQueueData->GraphicsDevice);
    auto commandAllocatorPoolItem = GetCommandAllocatorPoolItem(&threadDirectX12DeviceCommandPools[graphicsIdUnpacked.Index], 
                                                                graphicsDeviceData->CommandAllocationGeneration, 
                                                                commandQueueData->CommandAllocatorQueueType);

    if (commandAllocatorPoolItem->IsResetNeeded)
    {
        if (commandAllocatorPoolItem->CommandAllocator)
        {
            if (commandAllocatorPoolItem->Fence.FenceValue > 0)
            {
                DirectX12WaitForFenceOnCpu(commandAllocatorPoolItem->Fence);
            }

            AssertIfFailed(commandAllocatorPoolItem->CommandAllocator->Reset());
        }
        else 
        {
            auto commandQueueDataFull = GetDirectX12CommandQueueDataFull(commandQueue);
            SystemAssert(commandQueueDataFull);

            ComPtr<ID3D12CommandAllocator> commandAllocator;
            AssertIfFailed(graphicsDeviceData->Device->CreateCommandAllocator(commandQueueData->Type, IID_PPV_ARGS(commandAllocator.GetAddressOf())));
            commandAllocatorPoolItem->CommandAllocator = commandAllocator.Get();

            auto commandAllocatorIndex = SystemAtomicAdd(commandQueueDataFull->CurrentCommandAllocatorIndex, 1);
            commandQueueDataFull->CommandAllocators[commandAllocatorIndex] = commandAllocator;

            for (uint32_t i = 0; i < MAX_COMMANDLIST; i++)
            {
                ComPtr<ID3D12GraphicsCommandList10> commandList;
                AssertIfFailed(graphicsDeviceData->Device->CreateCommandList1(0, commandQueueData->Type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(commandList.GetAddressOf())));
                commandAllocatorPoolItem->CommandListPoolItems[i].CommandList = commandList.Get();

                auto commandListIndex = SystemAtomicAdd(commandQueueDataFull->CurrentCommandListIndex, 1);
                commandQueueDataFull->CommandLists[commandListIndex] = commandList;
            }
        }

        commandAllocatorPoolItem->IsResetNeeded = false;
    }

    auto commandListPoolItem = GetCommandListPoolItem(commandAllocatorPoolItem);

    if (DirectX12DebugLayerEnabled && options && options->DebugName)
    {
        commandListPoolItem->CommandList->SetName(SystemConvertUtf8ToWideChar(stackMemoryArena, options->DebugName).Pointer);
    }

    AssertIfFailedReturnNullHandle(commandListPoolItem->CommandList->Reset(commandAllocatorPoolItem->CommandAllocator, nullptr));
            
    auto descriptorHeaps = SystemPushArray<ID3D12DescriptorHeap*>(stackMemoryArena, 1);
    descriptorHeaps[0] = graphicsDeviceData->ResourceDescriptorHeap.Storage->DescriptorHeap.Get();

    commandListPoolItem->CommandList->SetDescriptorHeaps(1, descriptorHeaps.Pointer);

    // TODO: Can we set the root signature for all types all the time?
    commandListPoolItem->CommandList->SetGraphicsRootSignature(graphicsDeviceData->RootSignature.Get());
    commandListPoolItem->CommandList->SetComputeRootSignature(graphicsDeviceData->RootSignature.Get());

    auto handle = SystemAddDataPoolItem(directX12CommandListPool, {
        .DeviceObject = commandListPoolItem->CommandList,
        .CommandAllocatorPoolItem = commandAllocatorPoolItem,
        .CommandListPoolItem = commandListPoolItem,
        .GraphicsDevice = commandQueueData->GraphicsDevice
    }); 

    SystemAddDataPoolItemFull(directX12CommandListPool, handle, {
    });
    
    threadDirectX12CommandBufferCommitted = false;

    return handle;
}

void DirectX12CommitCommandList(ElemCommandList commandList)
{    
    auto commandListData = GetDirectX12CommandListData(commandList);
    AssertIfFailed(commandListData->DeviceObject->Close());
    
    commandListData->IsCommitted = true;
    threadDirectX12CommandBufferCommitted = true;
}

ElemFence DirectX12ExecuteCommandLists(ElemCommandQueue commandQueue, ElemCommandListSpan commandLists, const ElemExecuteCommandListOptions* options)
{
    auto commandQueueData = GetDirectX12CommandQueueData(commandQueue);
    SystemAssert(commandQueueData);

    bool hasError = false;

    // TODO: Wait for fences if any

    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto commandListsToExecute = SystemPushArray<ID3D12CommandList*>(stackMemoryArena, commandLists.Length);

    for (uint32_t i = 0; i < commandLists.Length; i++)
    {
        auto commandListData = GetDirectX12CommandListData(commandLists.Items[i]);

        if (!commandListData->IsCommitted)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Commandlist needs to be committed before executing it.");
            hasError = true;
            break;
        }

        commandListsToExecute[i] = commandListData->DeviceObject;
    }

    if (!hasError)
    {
        commandQueueData->DeviceObject->ExecuteCommandLists(commandLists.Length, commandListsToExecute.Pointer);
    }

    auto fence = CreateDirectX12CommandQueueFence(commandQueue);
    
    for (uint32_t i = 0; i < commandLists.Length; i++)
    {
        auto commandListData = GetDirectX12CommandListData(commandLists.Items[i]);

        if (!commandListData->IsCommitted)
        {
            DirectX12CommitCommandList(commandLists.Items[i]);
        }

        UpdateCommandAllocatorPoolItemFence(commandListData->CommandAllocatorPoolItem, fence);
        ReleaseCommandListPoolItem(commandListData->CommandListPoolItem);

        SystemRemoveDataPoolItem(directX12CommandListPool, commandLists.Items[i]);
    }

    return fence;
}

void DirectX12WaitForFenceOnCpu(ElemFence fence)
{
    SystemAssert(fence.CommandQueue != ELEM_HANDLE_NULL);

    auto commandQueueToWaitDataFull = GetDirectX12CommandQueueDataFull(fence.CommandQueue);

    if (!commandQueueToWaitDataFull)
    {
        return;
    }

    if (fence.FenceValue > commandQueueToWaitDataFull->LastCompletedFenceValue) 
    {
        commandQueueToWaitDataFull->LastCompletedFenceValue = SystemMax(commandQueueToWaitDataFull->LastCompletedFenceValue, commandQueueToWaitDataFull->Fence->GetCompletedValue());
    }

    if (fence.FenceValue > commandQueueToWaitDataFull->LastCompletedFenceValue)
    {
        SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Wait for Fence on CPU...");
        commandQueueToWaitDataFull->Fence->SetEventOnCompletion(fence.FenceValue, directX12GlobalFenceEvent);

        // TODO: It is a little bit extreme to block at infinity. We should set a timeout and break the program.
        WaitForSingleObject(directX12GlobalFenceEvent, INFINITE);
    }
}

bool DirectX12IsFenceCompleted(ElemFence fence)
{
    SystemAssert(fence.CommandQueue != ELEM_HANDLE_NULL);

    auto commandQueueToWaitDataFull = GetDirectX12CommandQueueDataFull(fence.CommandQueue);

    if (!commandQueueToWaitDataFull)
    {
        return true;
    }

    if (fence.FenceValue > commandQueueToWaitDataFull->LastCompletedFenceValue) 
    {
        commandQueueToWaitDataFull->LastCompletedFenceValue = SystemMax(commandQueueToWaitDataFull->LastCompletedFenceValue, commandQueueToWaitDataFull->Fence->GetCompletedValue());
    }

    return fence.FenceValue <= commandQueueToWaitDataFull->LastCompletedFenceValue;
}
