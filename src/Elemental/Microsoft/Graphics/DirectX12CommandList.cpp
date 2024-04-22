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
    auto commandLists = SystemPushArray<ComPtr<ID3D12GraphicsCommandList10>>(DirectX12MemoryArena, DIRECTX12_MAX_COMMANDLISTS);

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
        commandQueueDataFull->CommandLists[i].Reset();
    }

    SystemRemoveDataPoolItem(directX12CommandQueuePool, commandQueue);
}

void DirectX12ResetCommandAllocation(ElemGraphicsDevice graphicsDevice)
{
    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    graphicsDeviceData->CommandAllocationGeneration++;
}

ElemCommandList DirectX12GetCommandList(ElemCommandQueue commandQueue, const ElemCommandListOptions* options)
{
    SystemAssert(commandQueue != ELEM_HANDLE_NULL);

    auto commandQueueData = GetDirectX12CommandQueueData(commandQueue);
    SystemAssert(commandQueueData);

    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(commandQueueData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto commandAllocatorPoolItem = GetCommandAllocatorPoolItem(&threadDirectX12DeviceCommandPools[commandQueueData->GraphicsDevice], 
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
            ComPtr<ID3D12CommandAllocator> commandAllocator;
        }
        else 
        {
            auto commandQueueDataFull = GetDirectX12CommandQueueDataFull(commandQueue);
            SystemAssert(commandQueueDataFull);

            ComPtr<ID3D12CommandAllocator> commandAllocator;
            ComPtr<ID3D12GraphicsCommandList10> commandList;

            AssertIfFailed(graphicsDeviceData->Device->CreateCommandAllocator(commandQueueData->Type, IID_PPV_ARGS(commandAllocator.GetAddressOf())));
            AssertIfFailed(graphicsDeviceData->Device->CreateCommandList1(0, commandQueueData->Type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(commandList.GetAddressOf())));
        
            commandAllocatorPoolItem->CommandAllocator = commandAllocator.Get();
            commandAllocatorPoolItem->CommandList = commandList.Get();

            auto commandAllocatorIndex = SystemAtomicAdd(commandQueueDataFull->CurrentCommandAllocatorIndex, 1);
            commandQueueDataFull->CommandAllocators[commandAllocatorIndex] = commandAllocator;

            auto commandListIndex = SystemAtomicAdd(commandQueueDataFull->CurrentCommandListIndex, 1);
            commandQueueDataFull->CommandLists[commandListIndex] = commandList;
        }

        commandAllocatorPoolItem->IsResetNeeded = false;
    }
            
    AssertIfFailedReturnNullHandle(commandAllocatorPoolItem->CommandList->Reset(commandAllocatorPoolItem->CommandAllocator, nullptr));

    // TODO: Can we set the root signature for all types all the time?
    commandAllocatorPoolItem->CommandList->SetGraphicsRootSignature(graphicsDeviceData->RootSignature.Get());
    //commandList->SetComputeRootSignature(graphicsDeviceData->RootSignature.Get());

    auto handle = SystemAddDataPoolItem(directX12CommandListPool, {
        .DeviceObject = commandAllocatorPoolItem->CommandList,
        .CommandAllocatorPoolItem = commandAllocatorPoolItem
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
    auto commandListsToExecute = SystemPushArray<ID3D12CommandList*>(stackMemoryArena, commandLists.Length);

    for (uint32_t i = 0; i < commandLists.Length; i++)
    {
        auto commandListData = GetDirectX12CommandListData(commandLists.Items[i]);
        commandListsToExecute[i] = commandListData->DeviceObject.Get();
    }

    auto commandQueueData = GetDirectX12CommandQueueData(commandQueue);
    SystemAssert(commandQueueData);

    commandQueueData->DeviceObject->ExecuteCommandLists(commandLists.Length, commandListsToExecute.Pointer);

    auto fence = CreateDirectX12CommandQueueFence(commandQueue);
    
    for (uint32_t i = 0; i < commandLists.Length; i++)
    {
        auto commandListData = GetDirectX12CommandListData(commandLists.Items[i]);

        UpdateCommandAllocatorPoolItemFence(commandListData->CommandAllocatorPoolItem, fence);

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

        // TODO: It is a little bit extreme to block at infinity. We should set a timeout and break the program.
        WaitForSingleObject(directX12GlobalFenceEvent, INFINITE);
    }
}
