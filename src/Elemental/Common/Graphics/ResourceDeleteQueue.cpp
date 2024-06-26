#include "ResourceDeleteQueue.h"
#include "SystemLogging.h"

#define RESOURCE_DELETEQUEUE_MAX_ITEMS 512

// TODO: Thread safe

struct ResourceDeleteQueueEntry
{
    ElemHandle Resource;
    ResourceDeleteType Type;
    ElemFence Fences[16];
    uint32_t FenceCount;
    int32_t NextEntry;
};

Span<ResourceDeleteQueueEntry> GraphicsResourceDeleteQueue;
uint32_t CurrentGraphicsResourceDeleteQueueIndex;
int32_t GraphicsResourceDeleteQueueFreeListIndex;

void InitResourceDeleteQueueMemory(MemoryArena memoryArena)
{
    if (GraphicsResourceDeleteQueue.Length == 0)
    {
        GraphicsResourceDeleteQueue = SystemPushArrayZero<ResourceDeleteQueueEntry>(memoryArena, RESOURCE_DELETEQUEUE_MAX_ITEMS);
        CurrentGraphicsResourceDeleteQueueIndex = 0;
        GraphicsResourceDeleteQueueFreeListIndex = -1;
    } 
}

void EnqueueResourceDeleteEntry(MemoryArena memoryArena, ElemHandle resource, ResourceDeleteType type, ElemFenceSpan fences)
{
    InitResourceDeleteQueueMemory(memoryArena);

    ResourceDeleteQueueEntry entry = { .Resource = resource, .Type = type, .FenceCount = fences.Length, .NextEntry = -1 };
    SystemCopyBuffer(Span<ElemFence>(entry.Fences, 16), ReadOnlySpan<ElemFence>(fences.Items, fences.Length));

    uint32_t index = 0;

    if (GraphicsResourceDeleteQueueFreeListIndex != -1)
    {
        index = GraphicsResourceDeleteQueueFreeListIndex;
        GraphicsResourceDeleteQueueFreeListIndex = GraphicsResourceDeleteQueue[GraphicsResourceDeleteQueueFreeListIndex].NextEntry;
    }
    else 
    {
        index = CurrentGraphicsResourceDeleteQueueIndex++;    
    }

    GraphicsResourceDeleteQueue[index] = entry;
}

void ProcessResourceDeleteQueue()
{
    for (uint32_t i = 0; i < CurrentGraphicsResourceDeleteQueueIndex; i++)
    {
        auto entry = &GraphicsResourceDeleteQueue[i];

        if (entry->Resource != ELEM_HANDLE_NULL)
        {
            auto fencesCompleted = true;

            for (uint32_t j = 0; j < entry->FenceCount; j++)
            {
                if (!ElemIsFenceCompleted(entry->Fences[j]))
                {
                    fencesCompleted = false;
                    break;
                }
            }

            if (fencesCompleted)
            {
                if (entry->Type == ResourceDeleteType_Resource)
                {
                    ElemFreeGraphicsResource(entry->Resource, nullptr);
                }
                else if (entry->Type == ResourceDeleteType_Descriptor)
                {
                    ElemFreeGraphicsResourceDescriptor(entry->Resource, nullptr);
                }

                entry->Resource = ELEM_HANDLE_NULL;
                entry->NextEntry = GraphicsResourceDeleteQueueFreeListIndex;
                GraphicsResourceDeleteQueueFreeListIndex = i;
            }
        }
    }
}
