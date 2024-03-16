#include "Direct3D12CommandList.h"
#include "Direct3D12GraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define DIRECT3D12_MAX_COMMANDQUEUES 10u

SystemDataPool<Direct3D12GraphicsCommandQueueData, Direct3D12GraphicsCommandQueueDataFull> direct3D12GraphicsCommandQueuePool;

void InitDirect3D12CommandListMemory()
{
    if (!direct3D12GraphicsCommandQueuePool.Storage)
    {
        direct3D12GraphicsCommandQueuePool = SystemCreateDataPool<Direct3D12GraphicsCommandQueueData, Direct3D12GraphicsCommandQueueDataFull>(Direct3D12MemoryArena, DIRECT3D12_MAX_COMMANDQUEUES);
    }
}

Direct3D12GraphicsCommandQueueData* GetDirect3D12GraphicsCommandQueueData(ElemGraphicsCommandQueue graphicsDevice)
{
    return SystemGetDataPoolItem(direct3D12GraphicsCommandQueuePool, graphicsDevice);
}

Direct3D12GraphicsCommandQueueDataFull* GetDirect3D12GraphicsCommandQueueDataFull(ElemGraphicsCommandQueue graphicsDevice)
{
    return SystemGetDataPoolItemFull(direct3D12GraphicsCommandQueuePool, graphicsDevice);
}

ElemGraphicsCommandQueue Direct3D12CreateGraphicsCommandQueue(ElemGraphicsDevice graphicsDevice, ElemGraphicsCommandQueueType type, const ElemGraphicsCommandQueueOptions* options)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    
    InitDirect3D12CommandListMemory();
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);
    
    auto graphicsDeviceData = GetDirect3D12GraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);
    
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    if (type == ElemGraphicsCommandQueueType_Compute)
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

    auto handle = SystemAddDataPoolItem(direct3D12GraphicsCommandQueuePool, {
        .DeviceObject = commandQueue
    }); 

    SystemAddDataPoolItemFull(direct3D12GraphicsCommandQueuePool, handle, {
        .Type = commandQueueDesc.Type
    });

    return handle;
}

void Direct3D12FreeGraphicsCommandQueue(ElemGraphicsCommandQueue commandQueue)
{
    SystemAssert(commandQueue != ELEM_HANDLE_NULL);

    auto commandQueueData = GetDirect3D12GraphicsCommandQueueData(commandQueue);
    SystemAssert(commandQueueData);

    // TODO: Wait for command queue fence?

    commandQueueData->DeviceObject.Reset();
}
