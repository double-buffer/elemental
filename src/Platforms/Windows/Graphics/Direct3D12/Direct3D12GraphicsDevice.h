#pragma once
#include "WindowsCommon.h"
#include "../../../Common/CircularList.h"

struct CommandAllocatorPoolItem
{
    ComPtr<ID3D12CommandAllocator> Allocator;
    Fence Fence;
};

struct CommandListPoolItem
{
    ComPtr<ID3D12GraphicsCommandList7> CommandList;
    bool IsUsed;
};

struct Direct3D12GraphicsDevice : BaseGraphicsObject
{
    Direct3D12GraphicsDevice(BaseGraphicsService* graphicsService) : BaseGraphicsObject(graphicsService), DirectCommandAllocatorsPool(64), DirectCommandListsPool(64)
    {
    }

    ComPtr<ID3D12Device10> Device;
    DXGI_ADAPTER_DESC3 AdapterDescription;

    // HACK: This is temporary, will be refactored later!
    ComPtr<ID3D12DescriptorHeap> RtvDescriptorHeap;
    uint32_t RtvDescriptorHandleSize;
    uint32_t CurrentRtvDescriptorOffset;

    CircularList<CommandAllocatorPoolItem> DirectCommandAllocatorsPool;
    uint64_t CommandAllocationGeneration = 0;

    CircularList<CommandListPoolItem> DirectCommandListsPool;
};

struct DeviceCommandAllocators
{
    uint64_t Generation = 0;
    CommandAllocatorPoolItem* DirectAllocator = nullptr;
    CommandAllocatorPoolItem* ComputeAllocator = nullptr;
    CommandAllocatorPoolItem* CopyAllocator = nullptr;

    bool IsEmpty()
    {
        return DirectAllocator == nullptr;
    }

    void Reset(uint64_t currentGeneration)
    {
        DirectAllocator = nullptr;
        ComputeAllocator = nullptr;
        CopyAllocator = nullptr;
        Generation = currentGeneration;
    }
};