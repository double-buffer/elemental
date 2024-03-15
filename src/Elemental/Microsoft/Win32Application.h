#pragma once

#include "Elemental.h"
#include "SystemMemory.h"

// TODO: Refactor with prefix for Win32
struct Win32ApplicationData
{
    HINSTANCE ApplicationInstance;
};

struct Win32ApplicationDataFull
{
    ElemApplicationStatus Status;
    int32_t WindowCount;
};

extern MemoryArena ApplicationMemoryArena;

Win32ApplicationData* GetWin32ApplicationData(ElemApplication application);
Win32ApplicationDataFull* GetWin32ApplicationDataFull(ElemApplication application);
