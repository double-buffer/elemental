#include "Direct3D12CommandList.h"
#include "Direct3D12GraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define DIRECT3D12_MAX_COMMANDQUEUES 10u

SystemDataPool<Direct3D12CommandQueueData, Direct3D12CommandQueueDataFull> direct3D12CommandQueuePool;

void InitDirect3D12CommandListMemory()
{
    if (!direct3D12CommandQueuePool.Storage)
    {
        direct3D12CommandQueuePool = SystemCreateDataPool<Direct3D12CommandQueueData, Direct3D12CommandQueueDataFull>(Direct3D12MemoryArena, DIRECT3D12_MAX_COMMANDQUEUES);
    }
}

Direct3D12CommandQueueData* GetDirect3D12CommandQueueData(ElemCommandQueue graphicsDevice)
{
    return SystemGetDataPoolItem(direct3D12CommandQueuePool, graphicsDevice);
}

Direct3D12CommandQueueDataFull* GetDirect3D12CommandQueueDataFull(ElemCommandQueue graphicsDevice)
{
    return SystemGetDataPoolItemFull(direct3D12CommandQueuePool, graphicsDevice);
}

ElemCommandQueue Direct3D12CreateCommandQueue(ElemGraphicsDevice graphicsDevice, ElemCommandQueueType type, const ElemCommandQueueOptions* options)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    
    InitDirect3D12CommandListMemory();
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);
    
    auto graphicsDeviceData = GetDirect3D12GraphicsDeviceData(graphicsDevice);
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

    if (Direct3D12DebugLayerEnabled && options && options->DebugName)
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

void Direct3D12FreeCommandQueue(ElemCommandQueue commandQueue)
{
    SystemAssert(commandQueue != ELEM_HANDLE_NULL);

    auto commandQueueData = GetDirect3D12CommandQueueData(commandQueue);
    SystemAssert(commandQueueData);

    // TODO: Wait for command queue fence?

    commandQueueData->DeviceObject.Reset();
}

ElemCommandList Direct3D12CreateCommandList(ElemCommandQueue commandQueue, const ElemCommandListOptions* options)
{
    return ELEM_HANDLE_NULL;
}

void Direct3D12CommitCommandList(ElemCommandList commandList)
{
}

ElemFence Direct3D12ExecuteCommandList(ElemCommandQueue commandQueue, ElemCommandList commandList, const ElemExecuteCommandListOptions* options)
{
    return ELEM_HANDLE_NULL;
}

ElemFence Direct3D12ExecuteCommandLists(ElemCommandQueue commandQueue, ElemCommandListSpan commandLists, const ElemExecuteCommandListOptions* options)
{
    return ELEM_HANDLE_NULL;
}
