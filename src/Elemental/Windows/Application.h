#pragma once

#include "Elemental.h"
#include "SystemMemory.h"

struct ApplicationData
{
    HINSTANCE ApplicationInstance;
};

struct ApplicationDataFull
{
    ElemApplicationStatus Status;
};

extern MemoryArena ApplicationMemoryArena;

ApplicationData* GetApplicationData(ElemApplication application);
ApplicationDataFull* GetApplicationDataFull(ElemApplication application);
