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

template<typename T>
bool IsTypeEmpty()
{
    return sizeof(T) == 1;
}

template<typename T, typename TFull>
SystemDataPool<T, TFull> SystemCreateDataPool(MemoryArena memoryArena, size_t maxItems)
{
    auto storage = SystemPushStructZero<SystemDataPoolStorage<T, TFull>>(memoryArena);
    storage->MemoryArena = memoryArena;
    storage->Data = SystemPushArray<SystemDataPoolStorageItem<T>>(memoryArena, maxItems, AllocationState_Reserved);
    
    if (!IsTypeEmpty<TFull>())
    {
        storage->DataFull = SystemPushArray<TFull>(memoryArena, maxItems, AllocationState_Reserved);
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

    auto index = SYSTEM_DATAPOOL_INDEX_EMPTY;

    do
    {
        if (storage->FreeListIndex == SYSTEM_DATAPOOL_INDEX_EMPTY)
        {
            index = SYSTEM_DATAPOOL_INDEX_EMPTY;
            break;
        }
        
        index = storage->FreeListIndex;
    } while (!SystemAtomicCompareExchange(storage->FreeListIndex, index, storage->Data[storage->FreeListIndex].Next));

    if (index == SYSTEM_DATAPOOL_INDEX_EMPTY)
    {
        if (storage->CurrentIndex >= storage->Data.Length)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Memory, "Data Pool is full.");
            return ELEM_HANDLE_NULL;
        }

        index = SystemAtomicAdd(storage->CurrentIndex, 1);
        SystemCommitMemory<SystemDataPoolStorageItem<T>>(storage->MemoryArena, storage->Data.Slice(index), true);
        
        if (!IsTypeEmpty<TFull>())
        {
            SystemCommitMemory<TFull>(storage->MemoryArena, storage->DataFull.Slice(index), true);
        }
    }

    storage->Data[index].Data = data;
    storage->Data[index].Next = SYSTEM_DATAPOOL_INDEX_EMPTY;
        
    SystemAtomicAdd(storage->ItemCount, 1);

    result.Index = index;
    result.Version = storage->Data[index].Version;
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

    storage->DataFull[dataPoolHandle.Index] = data;
}

template<typename T, typename TFull>
void SystemRemoveDataPoolItem(SystemDataPool<T, TFull> dataPool, ElemHandle handle)
{
    auto storage = dataPool.Storage;
    SystemAssert(storage);
    SystemAssert(handle != ELEM_HANDLE_NULL);

    auto dataPoolHandle = UnpackSystemDataPoolHandle(handle);

    if (dataPoolHandle.Version != storage->Data[dataPoolHandle.Index].Version)
    {
        SystemLogWarningMessage(ElemLogMessageCategory_Memory, "Trying to remove an already deleted handle.");
        return;
    }

    storage->Data[dataPoolHandle.Index].Version = SystemAtomicAdd(storage->Data[dataPoolHandle.Index].Version, 1) + 1;
    SystemAtomicSubstract(storage->ItemCount, 1);

    do
    {
        storage->Data[dataPoolHandle.Index].Next = storage->FreeListIndex;
    } while (!SystemAtomicCompareExchange(storage->FreeListIndex, storage->FreeListIndex, dataPoolHandle.Index));
}

template<typename T, typename TFull>
T* SystemGetDataPoolItem(SystemDataPool<T, TFull> dataPool, ElemHandle handle)
{
    auto storage = dataPool.Storage;
    SystemAssert(storage);
    SystemAssert(handle != ELEM_HANDLE_NULL);
    
    auto dataPoolHandle = UnpackSystemDataPoolHandle(handle);
    
    T* result = nullptr;   

    if (dataPoolHandle.Version != SYSTEM_DATAPOOL_INDEX_EMPTY && storage->CurrentIndex > dataPoolHandle.Index && storage->Data[dataPoolHandle.Index].Version == dataPoolHandle.Version)
    {
        result = &storage->Data[dataPoolHandle.Index].Data;
    }

    return result;
}

template<typename T, typename TFull>
TFull* SystemGetDataPoolItemFull(SystemDataPool<T, TFull> dataPool, ElemHandle handle)
{
    auto storage = dataPool.Storage;
    auto dataPoolHandle = UnpackSystemDataPoolHandle(handle);
    SystemAssert(storage);
    SystemAssert(handle != ELEM_HANDLE_NULL);
    
    TFull* result = nullptr;   

    if (dataPoolHandle.Version != SYSTEM_DATAPOOL_INDEX_EMPTY && storage->CurrentIndex > dataPoolHandle.Index && storage->Data[dataPoolHandle.Index].Version == dataPoolHandle.Version)
    {
        result = &storage->DataFull[dataPoolHandle.Index];
    }

    return result;
}

template<typename T, typename TFull>
size_t SystemGetDataPoolItemCount(SystemDataPool<T, TFull> dataPool)
{
    SystemAssert(dataPool.Storage);
    return dataPool.Storage->ItemCount;
}
