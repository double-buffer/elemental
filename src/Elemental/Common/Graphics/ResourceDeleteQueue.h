#pragma once

#include "Elemental.h"
#include "SystemMemory.h"

enum ResourceDeleteType
{
    ResourceDeleteType_Resource,
    ResourceDeleteType_Descriptor
};

void EnqueueResourceDeleteEntry(MemoryArena memoryArena, ElemHandle resource, ResourceDeleteType type, ElemFenceSpan fences);
void ProcessResourceDeleteQueue();
