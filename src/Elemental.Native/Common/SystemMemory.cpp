#include "SystemMemory.h"

void* SystemAllocateMemory(size_t sizeInBytes, const wchar_t* file, uint32_t lineNumber)
{
    if (debugAllocations == NULL)
    {
        return malloc(sizeInBytes);
    }

    void* pointer = malloc(sizeInBytes);

    SystemAllocation* allocation = (SystemAllocation*)malloc(sizeof(SystemAllocation));
    allocation->SizeInBytes = sizeInBytes;
    wcscpy_s(allocation->File, _countof(allocation->File), file);
    allocation->LineNumber = lineNumber;

    DictionaryAdd(debugAllocations, (size_t)pointer, allocation);

    return pointer;
}

void* SystemAllocateMemoryAndReset(size_t count, size_t size, const wchar_t* file, uint32_t lineNumber)
{
    if (debugAllocations == NULL)
    {
        return calloc(count, size);
    }

    void* pointer = calloc(count, size);
    
    SystemAllocation* allocation = (SystemAllocation*)malloc(sizeof(SystemAllocation));
    allocation->SizeInBytes = count * size;
    wcscpy_s(allocation->File, _countof(allocation->File), file);
    allocation->LineNumber = lineNumber;

    DictionaryAdd(debugAllocations, (size_t)pointer, allocation);
  
    return pointer;
}

void SystemFreeMemory(void* pointer)
{
    if (debugAllocations == NULL)
    {
        free(pointer);
        return;
    }

    if (DictionaryContains(debugAllocations, (size_t)pointer))
    {
        SystemAllocation* allocation = (SystemAllocation*)DictionaryGetEntry(debugAllocations, (size_t)pointer);
        free(allocation);
        DictionaryDelete(debugAllocations, (size_t)pointer);
    }

    free(pointer);
}

void SystemDisplayMemoryLeak(uint64_t key, void* data)
{
    SystemAllocation* value = (SystemAllocation*)data;
    LogWarningMessage(LogMessageCategory_NativeApplication, L"%zu (size in bytes: %zu): %s: %u", (size_t)key, value->SizeInBytes, value->File, value->LineNumber);
}

void SystemInitDebugAllocations()
{
    debugAllocations = DictionaryCreate(64);
}

void SystemCheckAllocations(const wchar_t* description)
{
    //DictionaryPrint(debugAllocations);

    if (debugAllocations->Count > 0)
    {
        LogWarningMessage(LogMessageCategory_NativeApplication, L"Leaked native memory allocations (%s): %zu", description, debugAllocations->Count);
        DictionaryEnumerateEntries(debugAllocations, SystemDisplayMemoryLeak);
    }
    
    DictionaryFree(debugAllocations);
    debugAllocations = NULL;
}

