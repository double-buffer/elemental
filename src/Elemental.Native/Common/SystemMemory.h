#pragma once

#include "Dictionary.h"
#include "ElementalCommon.h"

//---------------------------------------------------------------------------------------------------------------
// Debug Memory management functions
//---------------------------------------------------------------------------------------------------------------
#ifdef _DEBUG

typedef struct
{
    size_t SizeInBytes;
    wchar_t File[MAX_PATH];
    uint32_t LineNumber;
} SystemAllocation;

static DictionaryStruct* debugAllocations = NULL;

void* SystemAllocateMemory(size_t sizeInBytes, const wchar_t* file, uint32_t lineNumber);
void* SystemAllocateMemoryAndReset(size_t count, size_t size, const wchar_t* file, uint32_t lineNumber);
void SystemFreeMemory(void* pointer);
void SystemDisplayMemoryLeak(uint64_t key, void* data);
void SystemInitDebugAllocations();
void SystemCheckAllocations(const wchar_t* description);

#endif


