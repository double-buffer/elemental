#pragma once
#include "WindowsCommon.h"
#include <vector>
#include <map>

struct Direct3D12GraphicsDevice : BaseGraphicsObject
{
    Direct3D12GraphicsDevice(BaseGraphicsService* graphicsService) : BaseGraphicsObject(graphicsService)
    {
    }

    ComPtr<ID3D12Device10> Device;
    DXGI_ADAPTER_DESC3 AdapterDescription;
    std::vector<std::map<D3D12_COMMAND_LIST_TYPE, std::map<uint32_t, ComPtr<ID3D12CommandAllocator>>>> CommandAllocators;
    
    // TODO: For the moment we will support only one swapchain per device
    uint64_t CurrentFrameNumber;
};