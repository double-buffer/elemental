#include "SystemMemory.h"

#ifdef _DEBUG

static DictionaryStruct* debugAllocations = NULL;

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

void* operator new(size_t size, const wchar_t* file, uint32_t lineNumber)
{
    return SystemAllocateMemory(size, file, lineNumber);
}

void* operator new[](size_t size, const wchar_t* file, uint32_t lineNumber)
{
    return SystemAllocateMemory(size, file, lineNumber);
}

void operator delete(void* pointer) noexcept
{
    return SystemFreeMemory(pointer);
}

void operator delete[](void* pointer) noexcept
{
    return SystemFreeMemory(pointer);
}

void operator delete(void* pointer, const wchar_t* file, uint32_t lineNumber)
{
    SystemFreeMemory(pointer);
}

void operator delete[](void* pointer, const wchar_t* file, uint32_t lineNumber)
{
    SystemFreeMemory(pointer);
}

#endif

// NEW CODE

MemoryArena* SystemAllocateMemoryArena(size_t sizeInBytes)
{
    // TODO: Review the 2 mallocs?
    auto pointer = (uint8_t*)malloc(sizeInBytes);

    auto result = (MemoryArena*)malloc(sizeof(MemoryArena));
    result->Memory = Span<uint8_t>(pointer, sizeInBytes);
    result->AllocatedBytes = 0;

    return result;
}

void* SystemAllocateMemory(MemoryArena* memoryArena, size_t sizeInBytes)
{
    // TODO: Add checks

    auto result = memoryArena->Memory.Pointer + memoryArena->AllocatedBytes;
    memoryArena->AllocatedBytes += sizeInBytes;

    // TODO: Make it optional
    memset(result, 0, sizeInBytes);

    return result;
}

template<typename T>
Span<T> SystemPushArray(MemoryArena* memoryArena, size_t count)
{
    auto memory = SystemAllocateMemory(memoryArena, sizeof(T) * count);
    return Span<T>((T*)memory, count);
}

template<>
Span<char> SystemPushArray<char>(MemoryArena* memoryArena, size_t count)
{
    auto memory = SystemAllocateMemory(memoryArena, sizeof(char) * (count + 1));
    return Span<char>((char*)memory, count);
}

template<>
Span<wchar_t> SystemPushArray<wchar_t>(MemoryArena* memoryArena, size_t count)
{
    auto memory = SystemAllocateMemory(memoryArena, sizeof(wchar_t) * (count + 1));
    return Span<wchar_t>((wchar_t*)memory, count);
}

template<typename T>
Span<T> SystemConcatBuffers(MemoryArena* memoryArena, ReadOnlySpan<T> buffer1, ReadOnlySpan<T> buffer2)
{
    auto result = SystemPushArray<T>(memoryArena, buffer1.Length + buffer2.Length);

    buffer1.CopyTo(result);
    buffer2.CopyTo(result.Slice(buffer1.Length));

    return result;
}
