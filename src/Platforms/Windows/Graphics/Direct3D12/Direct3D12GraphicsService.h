#pragma once
#include "WindowsCommon.h"
#include "../../../Common/BaseGraphicsService.h"
#include "../../../Common/BaseGraphicsObject.h"
#include "../../../Common/SystemFunctions.h"
#include "../../../Common/StringConverters.h"

#include "Direct3D12GraphicsDevice.h"
#include "Direct3D12CommandQueue.h"
#include "Direct3D12CommandList.h"

class Direct3D12GraphicsService : BaseGraphicsService
{
public:
    Direct3D12GraphicsService(GraphicsServiceOptions options);
    ~Direct3D12GraphicsService();

    void GetAvailableGraphicsDevices(GraphicsDeviceInfo* graphicsDevices, int* count) override;
    void* CreateGraphicsDevice(GraphicsDeviceOptions options) override;
    void FreeGraphicsDevice(void* graphicsDevicePointer) override;
    GraphicsDeviceInfo GetGraphicsDeviceInfo(void* graphicsDevicePointer) override;
    
    void* CreateCommandQueue(void* graphicsDevicePointer, CommandQueueType type) override;
    void FreeCommandQueue(void* commandQueuePointer) override;
    void SetCommandQueueLabel(void* commandQueuePointer, uint8_t* label) override;
    
    void* CreateCommandList(void* commandQueuePointer) override;
    void FreeCommandList(void* commandListPointer) override;
    void SetCommandListLabel(void* commandListPointer, uint8_t* label) override;
    void CommitCommandList(void* commandList) override;
    
    Fence ExecuteCommandLists(void* commandQueuePointer, void** commandLists, int32_t commandListCount, Fence* fencesToWait, int32_t fenceToWaitCount) override;

private:
    ComPtr<ID3D12SDKConfiguration> _sdkConfiguration;
    ComPtr<IDXGIFactory6> _dxgiFactory; 
    ComPtr<ID3D12Debug6> _debugInterface;
    ComPtr<IDXGIDebug1> _dxgiDebugInterface;

    GraphicsDeviceInfo ConstructGraphicsDeviceInfo(DXGI_ADAPTER_DESC3 adapterDescription);
    uint64_t GetDeviceId(DXGI_ADAPTER_DESC3 adapterDescription);

    ComPtr<ID3D12CommandAllocator> GetCommandAllocator(Direct3D12CommandQueue* commandQueue);
    ComPtr<ID3D12GraphicsCommandList7> GetCommandList(Direct3D12CommandQueue* commandQueue);
    void PushFreeCommandList(Direct3D12CommandQueue* commandQueue, ComPtr<ID3D12GraphicsCommandList7> commandList);
};