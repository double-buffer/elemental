#include "CommandAllocatorPool.h"
#include "SystemFunctions.h"

template<typename TCommandAllocator, typename TCommandList>
CommandAllocatorPoolItem<TCommandAllocator, TCommandList>* GetCommandAllocatorPoolItem(CommandAllocatorDevicePool<TCommandAllocator, TCommandList>* commandAllocatorPool, uint64_t generation, CommandAllocatorQueueType commandQueueType)
{
    if (commandAllocatorPool->Generation != generation)
    {
        for (uint32_t i = 0; i < CommandAllocatorQueueType_Max; i++)
        {
            commandAllocatorPool->CurrentCommandQueuePoolItems[i] = nullptr;
        }

        commandAllocatorPool->Generation = generation;
    }

    if (commandAllocatorPool->CurrentCommandQueuePoolItems[commandQueueType] == nullptr)
    {
        auto commandAllocatorIndex = commandQueueType * CommandAllocatorQueueType_Max + commandAllocatorPool->CurrentCommandAllocatorIndex;

        commandAllocatorPool->CurrentCommandQueuePoolItems[commandQueueType] = &commandAllocatorPool->CommandAllocators[commandAllocatorIndex];
        commandAllocatorPool->CurrentCommandQueuePoolItems[commandQueueType]->IsResetNeeded = true;
        commandAllocatorPool->CurrentCommandAllocatorIndex = (commandAllocatorPool->CurrentCommandAllocatorIndex + 1) % MAX_COMMANDALLOCATOR;
    }

    return commandAllocatorPool->CurrentCommandQueuePoolItems[commandQueueType];
}

template<typename TCommandAllocator, typename TCommandList>
CommandListPoolItem<TCommandList>* GetCommandListPoolItem(CommandAllocatorPoolItem<TCommandAllocator, TCommandList>* commandAllocatorPoolItem)
{
    auto commandListPoolItem = &commandAllocatorPoolItem->CommandListPoolItems[commandAllocatorPoolItem->CurrentCommandListIndex];
    SystemAssert (!commandListPoolItem->IsInUse);

    commandAllocatorPoolItem->CurrentCommandListIndex = (commandAllocatorPoolItem->CurrentCommandListIndex + 1) % MAX_COMMANDLIST;

    commandListPoolItem->IsInUse = true;
    return commandListPoolItem;
}

template<typename TCommandList>
void ReleaseCommandListPoolItem(CommandListPoolItem<TCommandList>* commandListPoolItem)
{
    commandListPoolItem->IsInUse = false;
}

template<typename TCommandAllocator, typename TCommandList>
void UpdateCommandAllocatorPoolItemFence(CommandAllocatorPoolItem<TCommandAllocator, TCommandList>* commandAllocatorPoolItem, ElemFence fence)
{
    commandAllocatorPoolItem->Fence = fence;
}
