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

    // TODO: This is temporary, will be refactored later!
    ComPtr<ID3D12DescriptorHeap> RtvDescriptorHeap;
    uint32_t RtvDescriptorHandleSize;
    uint32_t CurrentRtvDescriptorOffset;

    std::vector<std::map<D3D12_COMMAND_LIST_TYPE, std::map<uint32_t, ComPtr<ID3D12CommandAllocator>>>> CommandAllocators;
};