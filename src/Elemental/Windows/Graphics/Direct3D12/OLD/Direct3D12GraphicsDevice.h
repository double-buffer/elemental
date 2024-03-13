#pragma once

#include "ElementalOld.h"
#include "CircularList.h"
#include "SystemDictionary.h"
#include "GraphicsObject.h"
#include "Direct3D12Common.h"
#include "Direct3D12CommandList.h"

struct CommandAllocatorPoolItem
{
    ComPtr<ID3D12CommandAllocator> Allocator;
    Fence Fence;
    bool IsInUse = true;
};

struct CommandListPoolItem
{
    Direct3D12CommandList* CommandList;

    ~CommandListPoolItem()
    {
        if (CommandList)
        {
            delete CommandList;
        }
    }
};

struct PipelineStateCacheItem
{
    ComPtr<ID3D12PipelineState> PipelineState;
};

struct Direct3D12GraphicsDevice : GraphicsObject
{
    Direct3D12GraphicsDevice(): 
        DirectCommandAllocatorsPool(MAX_DIRECT3D12_COMMAND_ALLOCATORS), 
        ComputeCommandAllocatorsPool(MAX_DIRECT3D12_COMMAND_ALLOCATORS), 
        CopyCommandAllocatorsPool(MAX_DIRECT3D12_COMMAND_ALLOCATORS), 
        DirectCommandListsPool(MAX_DIRECT3D12_COMMAND_LISTS),
        ComputeCommandListsPool(MAX_DIRECT3D12_COMMAND_LISTS),
        CopyCommandListsPool(MAX_DIRECT3D12_COMMAND_LISTS)
    {
        InternalId = 0;
        GraphicsApi = GraphicsApi_Direct3D12;
    }

    uint32_t InternalId;
    ComPtr<ID3D12Device10> Device;
    DXGI_ADAPTER_DESC3 AdapterDescription;

    CircularList<CommandAllocatorPoolItem> DirectCommandAllocatorsPool;
    CircularList<CommandAllocatorPoolItem> ComputeCommandAllocatorsPool;
    CircularList<CommandAllocatorPoolItem> CopyCommandAllocatorsPool;
    uint64_t CommandAllocationGeneration = 0;

    CircularList<CommandListPoolItem> DirectCommandListsPool;
    CircularList<CommandListPoolItem> ComputeCommandListsPool;
    CircularList<CommandListPoolItem> CopyCommandListsPool;

    // TODO: Have a common pipelinstate manager
    // TODO: Don't compute the hash manually, pass the struct directly
    SystemDictionary<uint64_t, PipelineStateCacheItem> PipelineStates;

    // HACK: This is temporary, will be refactored later!
    ComPtr<ID3D12DescriptorHeap> RtvDescriptorHeap;
    uint32_t RtvDescriptorHandleSize;
    uint32_t CurrentRtvDescriptorOffset;
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
