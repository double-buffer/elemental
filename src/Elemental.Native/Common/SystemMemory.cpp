#include "SystemMemory.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"

struct MemoryArenaStorage
{
    Span<uint8_t> Memory;
    size_t AllocatedBytes;
    uint8_t Level;
    MemoryArenaStorage* Next;
};

thread_local MemoryArenaStorage* stackMemoryArenaStorage = nullptr;
thread_local MemoryArenaStorage* stackMemoryArenaExtraStorage = nullptr;

MemoryArenaStorage* AllocateMemoryArenaStorage(size_t sizeInBytes)
{
    // TODO: Review the mallocs?
    auto pointer = (uint8_t*)malloc(sizeInBytes); // TODO: System call to review
    auto memoryArenaStorage = (MemoryArenaStorage*)malloc(sizeof(MemoryArenaStorage));
    memoryArenaStorage->Memory = Span<uint8_t>(pointer, sizeInBytes);
    memoryArenaStorage->AllocatedBytes = 0;
    memoryArenaStorage->Level = 0;
    memoryArenaStorage->Next = nullptr;

    return memoryArenaStorage;
}

void FreeMemoryArenaStorage(MemoryArenaStorage* storage)
{
    free(storage->Memory.Pointer); // TODO: System call to review
    free(storage);
}

MemoryArena* AllocateMemoryArena(MemoryArenaStorage* storage, size_t startOffset, uint8_t level)
{
    // TODO: Review the mallocs?
    auto result = (MemoryArena*)malloc(sizeof(MemoryArena)); // TODO: System call to review
    result->Storage = storage;
    result->StartOffset = startOffset;
    result->AllocatedBytes = 0;
    result->SizeInBytes = storage->Memory.Length;
    result->Level = level;
    result->ExtraStorage = nullptr;

    return result;
}

// TODO: Provide an empty one with defaults for the initial size
MemoryArena* SystemAllocateMemoryArena(size_t sizeInBytes)
{
    auto storage = AllocateMemoryArenaStorage(sizeInBytes);
    return AllocateMemoryArena(storage, 0, 0);
}

void SystemFreeMemoryArena(MemoryArena* memoryArena)
{
    FreeMemoryArenaStorage(memoryArena->Storage);
    free(memoryArena); // TODO: System call to review
}

void SystemClearMemoryArena(MemoryArena* memoryArena)
{
    SystemPopMemory(memoryArena, memoryArena->AllocatedBytes);
}

size_t SystemGetMemoryArenaAllocatedBytes(MemoryArena* memoryArena)
{
    return memoryArena->AllocatedBytes;
}

StackMemoryArena SystemGetStackMemoryArena()
{
    if (stackMemoryArenaStorage == nullptr)
    {
        stackMemoryArenaStorage = AllocateMemoryArenaStorage(1024);
        stackMemoryArenaExtraStorage = AllocateMemoryArenaStorage(1024);
    }

    return { AllocateMemoryArena(stackMemoryArenaStorage, stackMemoryArenaStorage->AllocatedBytes, ++stackMemoryArenaStorage->Level)  };
}

StackMemoryArena::~StackMemoryArena()
{
    // BUG: Issue here if we have multiple storages
    // TODO: Can we use the pop function?
    stackMemoryArenaStorage->Level--;
    stackMemoryArenaStorage->AllocatedBytes = MemoryArenaPointer->StartOffset;

    if (MemoryArenaPointer->ExtraStorage != nullptr)
    {
        stackMemoryArenaExtraStorage->AllocatedBytes = MemoryArenaPointer->ExtraStorage->StartOffset;
        // TODO: Refactor
        free(MemoryArenaPointer->ExtraStorage);
    }

    // TODO: Refactor
    free(MemoryArenaPointer);
}

void* SystemPushMemory(MemoryArena* memoryArena, size_t sizeInBytes)
{
    MemoryArena* workingMemoryArena = memoryArena;

    if (memoryArena->Level !=  memoryArena->Storage->Level)
    {
        if (memoryArena->ExtraStorage == nullptr)
        {
            memoryArena->ExtraStorage = AllocateMemoryArena(stackMemoryArenaExtraStorage, stackMemoryArenaExtraStorage->AllocatedBytes, 0);
        }

        workingMemoryArena = memoryArena->ExtraStorage;
        LogDebugMessage(LogMessageCategory_Memory, L"Using stack memory arena extra storage. (Extra allocated bytes: %llu)", stackMemoryArenaExtraStorage->AllocatedBytes + sizeInBytes);
    }

    auto storage = workingMemoryArena->Storage;

    if (workingMemoryArena->Storage->AllocatedBytes + sizeInBytes > storage->Memory.Length)
    {
        auto oldSizeInBytes = workingMemoryArena->SizeInBytes;
        auto newStorageSize = RoundUpToPowerOf2(max(workingMemoryArena->SizeInBytes, sizeInBytes)); 
        workingMemoryArena->SizeInBytes += newStorageSize;
        workingMemoryArena->AllocatedBytes += storage->Memory.Length - storage->AllocatedBytes;
        storage->AllocatedBytes = storage->Memory.Length;

        LogDebugMessage(LogMessageCategory_Memory, L"Resizing MemoryArena to %llu (Previous size was: %llu) -> SizeInBytes: %llu", workingMemoryArena->SizeInBytes, oldSizeInBytes, sizeInBytes);

        storage = AllocateMemoryArenaStorage(newStorageSize);
        storage->Next = workingMemoryArena->Storage;
        workingMemoryArena->Storage = storage;
    }

    auto result = storage->Memory.Pointer + storage->AllocatedBytes;
    storage->AllocatedBytes += sizeInBytes;
    workingMemoryArena->AllocatedBytes += sizeInBytes;

    return result;
}

void SystemPopMemory(MemoryArena* memoryArena, size_t sizeInBytes)
{
    if (sizeInBytes > memoryArena->AllocatedBytes)
    {
        LogErrorMessage(LogMessageCategory_Memory, L"Cannot pop memory arena with: %llu (Allocated size is: %llu)", sizeInBytes, memoryArena->AllocatedBytes);
        return;
    }

    auto oldSizeInBytes = memoryArena->SizeInBytes;
    auto remainingSizeInBytes = sizeInBytes;

    auto storage = memoryArena->Storage;
    auto storageCount = 0;

    while (storage != nullptr && remainingSizeInBytes != 0)
    {
        if (storage->AllocatedBytes < remainingSizeInBytes)
        {
            remainingSizeInBytes -= storage->AllocatedBytes;
            memoryArena->SizeInBytes -= storage->Memory.Length;

            storageCount++;
            auto storageToFree = storage;
            storage = storage->Next;

            FreeMemoryArenaStorage(storageToFree);
        }
        else 
        {
            storage->AllocatedBytes -= remainingSizeInBytes;
            remainingSizeInBytes = 0;
        }
    }

    assert(storage);
    memoryArena->Storage = storage;
    memoryArena->AllocatedBytes -= sizeInBytes;

    LogDebugMessage(LogMessageCategory_Memory, L"Popping MemoryArena to %llu (New size: %llu, Previous size was: %llu) -> Deleted Memory Storages: %d, Allocated Bytes: %llu", sizeInBytes, memoryArena->SizeInBytes, oldSizeInBytes, storageCount, memoryArena->AllocatedBytes);
}

void* SystemPushMemoryZero(MemoryArena* memoryArena, size_t sizeInBytes)
{
    auto result = SystemPushMemory(memoryArena, sizeInBytes);
    memset(result, 0, sizeInBytes); // TODO: System call to review

    return result;
}

template<typename T>
Span<T> SystemPushArray(MemoryArena* memoryArena, size_t count)
{
    auto memory = SystemPushMemory(memoryArena, sizeof(T) * count);
    return Span<T>((T*)memory, count);
}

template<typename T>
Span<T> SystemPushArrayZero(MemoryArena* memoryArena, size_t count)
{
    auto memory = SystemPushMemoryZero(memoryArena, sizeof(T) * count);
    return Span<T>((T*)memory, count);
}

template<>
Span<char> SystemPushArrayZero<char>(MemoryArena* memoryArena, size_t count)
{
    auto memory = SystemPushMemoryZero(memoryArena, sizeof(char) * (count + 1));
    return Span<char>((char*)memory, count);
}

template<>
Span<wchar_t> SystemPushArrayZero<wchar_t>(MemoryArena* memoryArena, size_t count)
{
    auto memory = SystemPushMemoryZero(memoryArena, sizeof(wchar_t) * (count + 1));
    return Span<wchar_t>((wchar_t*)memory, count);
}

template<typename T>
T* SystemPushStruct(MemoryArena* memoryArena)
{
    return (T*)SystemPushMemory(memoryArena, sizeof(T));
}

template<typename T>
T* SystemPushStructZero(MemoryArena* memoryArena)
{
    return (T*)SystemPushMemoryZero(memoryArena, sizeof(T));
}

template<typename T>
Span<T> SystemConcatBuffers(MemoryArena* memoryArena, ReadOnlySpan<T> buffer1, ReadOnlySpan<T> buffer2)
{
    auto result = SystemPushArray<T>(memoryArena, buffer1.Length + buffer2.Length);

    buffer1.CopyTo(result);
    buffer2.CopyTo(result.Slice(buffer1.Length));

    return result;
}

template<>
Span<char> SystemConcatBuffers(MemoryArena* memoryArena, ReadOnlySpan<char> buffer1, ReadOnlySpan<char> buffer2)
{
    auto result = SystemPushArrayZero<char>(memoryArena, buffer1.Length + buffer2.Length);

    buffer1.CopyTo(result);
    buffer2.CopyTo(result.Slice(buffer1.Length));

    return result;
}

template<>
Span<wchar_t> SystemConcatBuffers(MemoryArena* memoryArena, ReadOnlySpan<wchar_t> buffer1, ReadOnlySpan<wchar_t> buffer2)
{
    auto result = SystemPushArrayZero<wchar_t>(memoryArena, buffer1.Length + buffer2.Length);

    buffer1.CopyTo(result);
    buffer2.CopyTo(result.Slice(buffer1.Length));

    return result;
}
