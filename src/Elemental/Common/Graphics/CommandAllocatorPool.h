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
#define MAX_COMMANDLIST 64u

template<typename TCommandList>
struct CommandListPoolItem
{
    TCommandList CommandList;
    bool IsInUse;
};

template<typename TCommandAllocator, typename TCommandList>
struct CommandAllocatorPoolItem
{
    TCommandAllocator CommandAllocator;
    CommandListPoolItem<TCommandList> CommandListPoolItems[MAX_COMMANDLIST];
    ElemFence Fence;
    bool IsResetNeeded;
    uint32_t CurrentCommandListIndex;
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
CommandListPoolItem<TCommandList>* GetCommandListPoolItem(CommandAllocatorPoolItem<TCommandAllocator, TCommandList>* commandAllocatorPoolItem);

template<typename TCommandList>
void ReleaseCommandListPoolItem(CommandListPoolItem<TCommandList>* commandListPoolItem);

template<typename TCommandAllocator, typename TCommandList>
void UpdateCommandAllocatorPoolItemFence(CommandAllocatorPoolItem<TCommandAllocator, TCommandList>* commandAllocatorPoolItem, ElemFence fence);
