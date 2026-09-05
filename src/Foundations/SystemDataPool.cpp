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

template<typename T, typename TFull>
struct SystemDataPoolStorage
{
    MemoryArena MemoryArena;
    Span<SystemDataPoolStorageItem<T>> Data;
    Span<TFull> DataFull;
    bool IsItemAllocationInProgress;
    uint32_t CurrentIndex;
    uint32_t FreeListIndex;
    uint32_t ItemCount;
};

ElemHandle PackSystemDataPoolHandle(SystemDataPoolHandle handle)
{
    return ((uint64_t)handle.Version << 32) | (handle.Index + 1);
}

SystemDataPoolHandle UnpackSystemDataPoolHandle(uint64_t packedValue)
{
    SystemDataPoolHandle result = {};

    result.Index = (uint32_t)(packedValue & 0xFFFFFFFF) - 1;
    result.Version = (uint32_t)(packedValue >> 32);
    return result;
}

uint32_t GetNextSystemDataPoolVersion(uint32_t version)
{
    auto result = version + 1;

    if (result == SYSTEM_DATAPOOL_INDEX_EMPTY)
    {
        result = 0;
    }

    return result;
}

template<typename T>
bool IsTypeEmpty()
{
    return sizeof(T) == 1;
}

template<typename T, typename TFull>
void LockSystemDataPoolItemAllocation(SystemDataPoolStorage<T, TFull>* storage)
{
    SystemAtomicReplace(storage->IsItemAllocationInProgress, false, true);
}

template<typename T, typename TFull>
void UnlockSystemDataPoolItemAllocation(SystemDataPoolStorage<T, TFull>* storage)
{
    SystemAtomicStore(storage->IsItemAllocationInProgress, false);
}

template<typename T, typename TFull>
uint32_t AcquireSystemDataPoolItemIndex(SystemDataPoolStorage<T, TFull>* storage, bool* isNewIndex)
{
    LockSystemDataPoolItemAllocation(storage);

    if (storage->FreeListIndex != SYSTEM_DATAPOOL_INDEX_EMPTY)
    {
        auto index = storage->FreeListIndex;
        storage->FreeListIndex = storage->Data[index].Next;
        storage->Data[index].Next = SYSTEM_DATAPOOL_INDEX_EMPTY;
        *isNewIndex = false;
        UnlockSystemDataPoolItemAllocation(storage);
        return index;
    }

    uint32_t currentIndex;
    SystemAtomicLoad(storage->CurrentIndex, currentIndex);

    if (currentIndex >= storage->Data.Length)
    {
        UnlockSystemDataPoolItemAllocation(storage);
        return SYSTEM_DATAPOOL_INDEX_EMPTY;
    }

    SystemAtomicStore(storage->CurrentIndex, currentIndex + 1);
    *isNewIndex = true;
    UnlockSystemDataPoolItemAllocation(storage);
    return currentIndex;
}

template<typename T, typename TFull>
SystemDataPool<T, TFull> SystemCreateDataPool(MemoryArena memoryArena, size_t maxItems)
{
    if (maxItems > UINT32_MAX)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Memory, "Data Pool maximum item count is too large.");
        return {};
    }

    auto storage = SystemPushStructZero<SystemDataPoolStorage<T, TFull>>(memoryArena);

    if (storage == nullptr)
    {
        return {};
    }

    storage->MemoryArena = memoryArena;
    storage->Data = SystemPushArray<SystemDataPoolStorageItem<T>>(memoryArena, maxItems, AllocationState_Reserved);

    if (maxItems > 0 && storage->Data.Pointer == nullptr)
    {
        return {};
    }
    
    if (!IsTypeEmpty<TFull>())
    {
        storage->DataFull = SystemPushArray<TFull>(memoryArena, maxItems, AllocationState_Reserved);

        if (maxItems > 0 && storage->DataFull.Pointer == nullptr)
        {
            return {};
        }
    }
    
    storage->FreeListIndex = SYSTEM_DATAPOOL_INDEX_EMPTY;

    SystemDataPool<T, TFull> result = {};
    result.Storage = storage;
    return result;
}

template<typename T, typename TFull>
ElemHandle SystemAddDataPoolItem(SystemDataPool<T, TFull> dataPool, T data)
{
    SystemDataPoolHandle result = {};
    auto storage = dataPool.Storage;
    SystemAssert(storage);

    auto isNewIndex = false;
    auto index = AcquireSystemDataPoolItemIndex(storage, &isNewIndex);

    if (index == SYSTEM_DATAPOOL_INDEX_EMPTY)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Memory, "Data Pool is full.");
        return ELEM_HANDLE_NULL;
    }

    if (isNewIndex)
    {
        auto remainingItemCount = storage->Data.Length - index;
        auto itemCountToCommit = remainingItemCount > 1000 ? 1000 : remainingItemCount;
        SystemCommitMemory<SystemDataPoolStorageItem<T>>(storage->MemoryArena, storage->Data.Slice(index, itemCountToCommit), true);
        
        if (!IsTypeEmpty<TFull>())
        {
            SystemCommitMemory<TFull>(storage->MemoryArena, storage->DataFull.Slice(index, itemCountToCommit), true);
        }
    }

    storage->Data[index].Data = data;
    SystemAtomicStore(storage->Data[index].Next, SYSTEM_DATAPOOL_INDEX_EMPTY);
    SystemAtomicAdd(storage->ItemCount, 1);

    uint32_t version;
    SystemAtomicLoad(storage->Data[index].Version, version);

    result.Index = index;
    result.Version = version;
    return PackSystemDataPoolHandle(result);
}
    
template<typename T, typename TFull>
void SystemAddDataPoolItemFull(SystemDataPool<T, TFull> dataPool, ElemHandle handle, TFull data)
{
    if (IsTypeEmpty<TFull>())
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Memory, "Cannot add full data because data pool was not created with a full type.");
        return;
    }
    
    auto storage = dataPool.Storage;
    SystemAssert(storage);
    SystemAssert(handle != ELEM_HANDLE_NULL);

    auto dataPoolHandle = UnpackSystemDataPoolHandle(handle);

    if (dataPoolHandle.Index >= storage->Data.Length)
    {
        return;
    }

    uint32_t version;
    SystemAtomicLoad(storage->Data[dataPoolHandle.Index].Version, version);

    if (version != dataPoolHandle.Version)
    {
        return;
    }

    storage->DataFull[dataPoolHandle.Index] = data;
}

template<typename T, typename TFull>
void SystemRemoveDataPoolItem(SystemDataPool<T, TFull> dataPool, ElemHandle handle)
{
    auto storage = dataPool.Storage;
    SystemAssert(storage);
    SystemAssert(handle != ELEM_HANDLE_NULL);

    auto dataPoolHandle = UnpackSystemDataPoolHandle(handle);

    if (dataPoolHandle.Index >= storage->Data.Length)
    {
        return;
    }

    LockSystemDataPoolItemAllocation(storage);

    uint32_t version;
    SystemAtomicLoad(storage->Data[dataPoolHandle.Index].Version, version);

    if (dataPoolHandle.Version != version)
    {
        UnlockSystemDataPoolItemAllocation(storage);
        SystemLogWarningMessage(ElemLogMessageCategory_Memory, "Trying to remove an already deleted handle.");
        return;
    }

    auto nextVersion = GetNextSystemDataPoolVersion(version);
    SystemAtomicStore(storage->Data[dataPoolHandle.Index].Version, nextVersion);
    SystemAtomicSubstract(storage->ItemCount, 1);

    storage->Data[dataPoolHandle.Index].Next = storage->FreeListIndex;
    storage->FreeListIndex = dataPoolHandle.Index;

    UnlockSystemDataPoolItemAllocation(storage);
}

template<typename T, typename TFull>
T* SystemGetDataPoolItem(SystemDataPool<T, TFull> dataPool, ElemHandle handle)
{
    auto storage = dataPool.Storage;
    SystemAssert(storage);
    SystemAssert(handle != ELEM_HANDLE_NULL);
    
    auto dataPoolHandle = UnpackSystemDataPoolHandle(handle);

    if (dataPoolHandle.Version == SYSTEM_DATAPOOL_INDEX_EMPTY || dataPoolHandle.Index >= storage->Data.Length)
    {
        return nullptr;
    }

    uint32_t currentIndex;
    SystemAtomicLoad(storage->CurrentIndex, currentIndex);

    if (dataPoolHandle.Index >= currentIndex)
    {
        return nullptr;
    }

    uint32_t version;
    SystemAtomicLoad(storage->Data[dataPoolHandle.Index].Version, version);

    if (version != dataPoolHandle.Version)
    {
        return nullptr;
    }

    return &storage->Data[dataPoolHandle.Index].Data;
}

template<typename T, typename TFull>
TFull* SystemGetDataPoolItemFull(SystemDataPool<T, TFull> dataPool, ElemHandle handle)
{
    auto storage = dataPool.Storage;
    SystemAssert(storage);
    SystemAssert(handle != ELEM_HANDLE_NULL);

    auto dataPoolHandle = UnpackSystemDataPoolHandle(handle);

    if (dataPoolHandle.Version == SYSTEM_DATAPOOL_INDEX_EMPTY || dataPoolHandle.Index >= storage->Data.Length)
    {
        return nullptr;
    }

    uint32_t currentIndex;
    SystemAtomicLoad(storage->CurrentIndex, currentIndex);

    if (dataPoolHandle.Index >= currentIndex)
    {
        return nullptr;
    }

    uint32_t version;
    SystemAtomicLoad(storage->Data[dataPoolHandle.Index].Version, version);

    if (version != dataPoolHandle.Version)
    {
        return nullptr;
    }

    return &storage->DataFull[dataPoolHandle.Index];
}

template<typename T, typename TFull>
size_t SystemGetDataPoolItemCount(SystemDataPool<T, TFull> dataPool)
{
    SystemAssert(dataPool.Storage);

    uint32_t itemCount;
    SystemAtomicLoad(dataPool.Storage->ItemCount, itemCount);
    return itemCount;
}
