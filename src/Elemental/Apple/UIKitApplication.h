#pragma once

#include "Elemental.h"
#include "SystemMemory.h"

struct UIKitApplicationData
{
};

struct UIKitApplicationDataFull
{
    ElemApplicationStatus Status;
    int32_t WindowCount;
};

extern MemoryArena ApplicationMemoryArena;

UIKitApplicationData* GetUIKitApplicationData(ElemApplication application);
UIKitApplicationDataFull* GetUIKitApplicationDataFull(ElemApplication application);
