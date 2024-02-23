#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"

#define SYSTEM_DATAPOOL_INDEX_EMPTY UINT32_MAX

template<typename T>
struct SystemDataPoolStorageItem
{
    T Data;
    uint32_t Version;
    uint32_t Next;
};

template<typename T>
struct SystemDataPoolStorage
{
    MemoryArena MemoryArena;
    Span<SystemDataPoolStorageItem<T>> Data;
    uint32_t CurrentIndex;
    uint32_t FreeListIndex;
};

struct SystemDataPoolHandle
{
    uint32_t Index;
    uint32_t Version;
};

ElemHandle PackSystemDataPoolHandle(SystemDataPoolHandle handle)
{
    return ((uint64_t)handle.Version << 32) | handle.Index;
}

SystemDataPoolHandle UnpackSystemDataPoolHandle(uint64_t packedValue)
{
    SystemDataPoolHandle result = {};

    result.Index = (uint32_t)(packedValue & 0xFFFFFFFF);
    result.Version = (uint32_t)(packedValue >> 32);
    return result;
}

template<typename T>
SystemDataPool<T> SystemCreateDataPool(MemoryArena memoryArena, size_t maxItems)
{
    auto storage = SystemPushStructZero<SystemDataPoolStorage<T>>(memoryArena);
    storage->MemoryArena = memoryArena;
    storage->Data = SystemPushArray<SystemDataPoolStorageItem<T>>(memoryArena, maxItems, AllocationState_Reserved);
    storage->FreeListIndex = SYSTEM_DATAPOOL_INDEX_EMPTY;

    SystemDataPool<T> result = {};
    result.Storage = storage;
    return result;
}

template<typename T>
ElemHandle SystemAddDataPoolItem(SystemDataPool<T> dataPool, T data)
{
    SystemDataPoolHandle result = {};
    auto storage = dataPool.Storage;

    // TODO: Freelist lookup
    // TODO: Atomics!
    auto index = SYSTEM_DATAPOOL_INDEX_EMPTY;

    if (storage->FreeListIndex != SYSTEM_DATAPOOL_INDEX_EMPTY)
    {
        index = storage->FreeListIndex;
        storage->FreeListIndex = storage->Data[storage->FreeListIndex].Next;
    }

    if (index == SYSTEM_DATAPOOL_INDEX_EMPTY)
    {
        if (storage->CurrentIndex >= storage->Data.Length)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Memory, "Data Pool is full.");
            result.Version = SYSTEM_DATAPOOL_INDEX_EMPTY;
            return PackSystemDataPoolHandle(result);
        }

        index = SystemAtomicAdd(storage->CurrentIndex, 1);
        SystemCommitMemory<SystemDataPoolStorageItem<T>>(storage->MemoryArena, storage->Data.Slice(index), true);
    }

    storage->Data[index].Data = data;
    storage->Data[index].Next = SYSTEM_DATAPOOL_INDEX_EMPTY;

    result.Index = index;
    result.Version = storage->Data[index].Version;
    return PackSystemDataPoolHandle(result);
}
    
template<typename T>
void SystemRemoveDataPoolItem(SystemDataPool<T> dataPool, ElemHandle handle)
{
    auto storage = dataPool.Storage;
    auto dataPoolHandle = UnpackSystemDataPoolHandle(handle);

    // TODO: Atomic!

    if (dataPoolHandle.Version != SYSTEM_DATAPOOL_INDEX_EMPTY)
    {
        storage->Data[dataPoolHandle.Index].Version++;
    }

    storage->Data[dataPoolHandle.Index].Next = storage->FreeListIndex;
    storage->FreeListIndex = dataPoolHandle.Index;

    // TODO: Freelist handling
}

template<typename T>
T* SystemGetDataPoolItem(SystemDataPool<T> dataPool, ElemHandle handle)
{
    auto storage = dataPool.Storage;
    auto dataPoolHandle = UnpackSystemDataPoolHandle(handle);
    
    T* result = nullptr;   

    if (dataPoolHandle.Version != SYSTEM_DATAPOOL_INDEX_EMPTY && storage->CurrentIndex > dataPoolHandle.Index && storage->Data[dataPoolHandle.Index].Version == dataPoolHandle.Version)
    {
        result = &storage->Data[dataPoolHandle.Index].Data;
    }

    return result;
}
