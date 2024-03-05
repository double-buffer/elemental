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
};

extern MemoryArena ApplicationMemoryArena;

Win32ApplicationData* GetApplicationData(ElemApplication application);
Win32ApplicationDataFull* GetApplicationDataFull(ElemApplication application);
