#include "DirectX12CommandList.h"
#include "DirectX12GraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define DIRECT3D12_MAX_COMMANDQUEUES 10u

SystemDataPool<DirectX12CommandQueueData, DirectX12CommandQueueDataFull> direct3D12CommandQueuePool;

void InitDirectX12CommandListMemory()
{
    if (!direct3D12CommandQueuePool.Storage)
    {
        direct3D12CommandQueuePool = SystemCreateDataPool<DirectX12CommandQueueData, DirectX12CommandQueueDataFull>(DirectX12MemoryArena, DIRECT3D12_MAX_COMMANDQUEUES);
    }
}

DirectX12CommandQueueData* GetDirectX12CommandQueueData(ElemCommandQueue graphicsDevice)
{
    return SystemGetDataPoolItem(direct3D12CommandQueuePool, graphicsDevice);
}

DirectX12CommandQueueDataFull* GetDirectX12CommandQueueDataFull(ElemCommandQueue graphicsDevice)
{
    return SystemGetDataPoolItemFull(direct3D12CommandQueuePool, graphicsDevice);
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

    // TODO: Create fence object?

    if (DirectX12DebugLayerEnabled && options && options->DebugName)
    {
        commandQueue->SetName(SystemConvertUtf8ToWideChar(stackMemoryArena, options->DebugName).Pointer);
    }    

    auto handle = SystemAddDataPoolItem(direct3D12CommandQueuePool, {
        .DeviceObject = commandQueue
    }); 

    SystemAddDataPoolItemFull(direct3D12CommandQueuePool, handle, {
        .Type = commandQueueDesc.Type
    });

    return handle;
}

void DirectX12FreeCommandQueue(ElemCommandQueue commandQueue)
{
    SystemAssert(commandQueue != ELEM_HANDLE_NULL);

    auto commandQueueData = GetDirectX12CommandQueueData(commandQueue);
    SystemAssert(commandQueueData);

    // TODO: Wait for command queue fence?

    commandQueueData->DeviceObject.Reset();
}

ElemCommandList DirectX12CreateCommandList(ElemCommandQueue commandQueue, const ElemCommandListOptions* options)
{
    return ELEM_HANDLE_NULL;
}

void DirectX12CommitCommandList(ElemCommandList commandList)
{
}

ElemFence DirectX12ExecuteCommandList(ElemCommandQueue commandQueue, ElemCommandList commandList, const ElemExecuteCommandListOptions* options)
{
    return ELEM_HANDLE_NULL;
}

ElemFence DirectX12ExecuteCommandLists(ElemCommandQueue commandQueue, ElemCommandListSpan commandLists, const ElemExecuteCommandListOptions* options)
{
    return ELEM_HANDLE_NULL;
}
