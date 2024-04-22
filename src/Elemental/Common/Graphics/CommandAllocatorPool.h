#pragma once

#include "Elemental.h"

enum CommandAllocatorQueueType
{
    CommandAllocatorQueueType_Graphics = 0,
    CommandAllocatorQueueType_Compute = 1,
    CommandAllocatorQueueType_Copy = 2,
    CommandAllocatorQueueType_Max = 3
};

#define MAX_COMMANDALLOCATOR 3u

template<typename TCommandAllocator, typename TCommandList>
struct CommandAllocatorPoolItem
{
    TCommandAllocator CommandAllocator;
    TCommandList CommandList;
    ElemFence Fence;
    bool IsResetNeeded;
};

template<typename TCommandAllocator, typename TCommandList>
struct CommandAllocatorDevicePool
{
    CommandAllocatorPoolItem<TCommandAllocator, TCommandList> CommandAllocators[MAX_COMMANDALLOCATOR * CommandAllocatorQueueType_Max]; 
    uint32_t CurrentCommandAllocatorIndex;
    CommandAllocatorPoolItem<TCommandAllocator, TCommandList>* CurrentCommandQueuePoolItems[CommandAllocatorQueueType_Max];
    uint64_t Generation;
};

template<typename TCommandAllocator, typename TCommandList>
CommandAllocatorPoolItem<TCommandAllocator, TCommandList>* GetCommandAllocatorPoolItem(CommandAllocatorDevicePool<TCommandAllocator, TCommandList>* commandAllocatorPool, uint64_t generation, CommandAllocatorQueueType commandQueueType);

template<typename TCommandAllocator, typename TCommandList>
void UpdateCommandAllocatorPoolItemFence(CommandAllocatorPoolItem<TCommandAllocator, TCommandList>* commandAllocatorPoolItem, ElemFence fence);
