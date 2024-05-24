#pragma once

#include "Elemental.h"
#include "SystemMemory.h"

extern MemoryArena ApplicationMemoryArena;

typedef void (*Win32RunLoopHandlerPtr)(ElemHandle handle);

struct Win32RunLoopHandler
{
    Win32RunLoopHandlerPtr Function;
    ElemHandle Handle;
    uint32_t NextIndex;
};

extern uint64_t Win32PerformanceCounterStart;
extern uint64_t Win32PerformanceCounterFrequencyInSeconds;

void AddWin32RunLoopHandler(Win32RunLoopHandler handler);
void RemoveWin32RunLoopHandler(Win32RunLoopHandler handler);
