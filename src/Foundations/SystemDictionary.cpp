#include "SystemDictionary.h"
#include "SystemFunctions.h"

#ifdef ElemAPI
#include "SystemLogging.h"
#endif

#define SYSTEM_DICTIONARY_HASH_SEED 123456789
#define SYSTEM_DICTIONARY_INDEX_EMPTY -1

template<typename TValue>
struct SystemDictionaryEntry
{
    uint64_t Hash;
    TValue Value;
    int32_t Next;
};

template<typename TValue> 
struct SystemDictionaryStorage
{
    MemoryArena MemoryArena;
    Span<int32_t> Buckets;
    Span<SystemDictionaryEntry<TValue>> Entries;
    bool IsOperationInProgress;
    size_t CurrentEntryIndex;
    int32_t FreeListIndex;
};

struct SystemDictionaryIndexInfo
{
    int32_t BucketIndex;
    int32_t RootIndex;
    int32_t ParentIndex;
    int32_t Index;
};

struct SystemDictionaryHashInfo
{
    uint64_t Hash;
    int32_t BucketIndex;
};

template<typename TValue>
void LockSystemDictionary(SystemDictionaryStorage<TValue>* storage)
{
    SystemAtomicReplace(storage->IsOperationInProgress, false, true);
}

template<typename TValue>
void UnlockSystemDictionary(SystemDictionaryStorage<TValue>* storage)
{
    SystemAtomicStore(storage->IsOperationInProgress, false);
}

template<typename TValue>
SystemDictionaryEntry<TValue>* GetDictionaryEntryByIndex(SystemDictionaryStorage<TValue>* storage, int32_t index)
{
    if (index == SYSTEM_DICTIONARY_INDEX_EMPTY || index < 0 || (size_t)index >= storage->Entries.Length)
    {
        return nullptr;
    }

    return &(storage->Entries[index]);
}

template<typename TValue>
SystemDictionaryIndexInfo GetDictionaryEntryIndexInfo(SystemDictionaryStorage<TValue>* storage, SystemDictionaryHashInfo hashInfo)
{
    if (hashInfo.BucketIndex == SYSTEM_DICTIONARY_INDEX_EMPTY)
    {
        return { SYSTEM_DICTIONARY_INDEX_EMPTY, SYSTEM_DICTIONARY_INDEX_EMPTY, SYSTEM_DICTIONARY_INDEX_EMPTY, SYSTEM_DICTIONARY_INDEX_EMPTY };
    }

    auto currentIndex = storage->Buckets[hashInfo.BucketIndex];
    auto rootIndex = currentIndex;
    auto parentIndex = SYSTEM_DICTIONARY_INDEX_EMPTY;

    while (currentIndex != SYSTEM_DICTIONARY_INDEX_EMPTY) 
    {
        auto currentEntry = GetDictionaryEntryByIndex(storage, currentIndex);

        if (currentEntry == nullptr)
        {
            break;
        }

        if (currentEntry->Hash == hashInfo.Hash)
        {
            return { hashInfo.BucketIndex, rootIndex, parentIndex, currentIndex };
        }

        parentIndex = currentIndex;
        currentIndex = currentEntry->Next;
    }

    return { SYSTEM_DICTIONARY_INDEX_EMPTY, SYSTEM_DICTIONARY_INDEX_EMPTY, SYSTEM_DICTIONARY_INDEX_EMPTY, SYSTEM_DICTIONARY_INDEX_EMPTY };
}

template<typename TValue>
int32_t GetFreeListEntry(SystemDictionaryStorage<TValue>* storage)
{
    if (storage->FreeListIndex == SYSTEM_DICTIONARY_INDEX_EMPTY)
    {
        return SYSTEM_DICTIONARY_INDEX_EMPTY;
    }

    auto entryIndex = storage->FreeListIndex;
    auto entry = GetDictionaryEntryByIndex(storage, entryIndex);
    SystemAssert(entry);

    storage->FreeListIndex = entry->Next;
    entry->Next = SYSTEM_DICTIONARY_INDEX_EMPTY;
    return entryIndex;
}

template<typename TValue>
void InsertFreeListEntry(SystemDictionaryStorage<TValue>* storage, int32_t index, SystemDictionaryEntry<TValue>* entry)
{
    entry->Next = storage->FreeListIndex;
    storage->FreeListIndex = index;
}

template<typename TValue>
void AddDictionaryEntry(SystemDictionaryStorage<TValue>* storage, SystemDictionaryHashInfo hashInfo, TValue value)
{
    if (storage == nullptr || hashInfo.BucketIndex == SYSTEM_DICTIONARY_INDEX_EMPTY)
    {
        return;
    }

    LockSystemDictionary(storage);

    auto entryIndex = GetFreeListEntry(storage);

    if (entryIndex == SYSTEM_DICTIONARY_INDEX_EMPTY) 
    {
        if (storage->CurrentEntryIndex >= storage->Entries.Length)
        {
            UnlockSystemDictionary(storage);

            #ifdef ElemAPI
            SystemLogErrorMessage(ElemLogMessageCategory_Application, "Max items in dictionary reached, the item will not be added.");
            #endif
            return;
        }

        entryIndex = (int32_t)storage->CurrentEntryIndex;
        storage->CurrentEntryIndex++;
        SystemCommitMemory<SystemDictionaryEntry<TValue>>(storage->MemoryArena, storage->Entries.Slice(entryIndex, 1), true);
    }

    auto entry = GetDictionaryEntryByIndex(storage, entryIndex);
    SystemAssert(entry);

    auto bucketHead = storage->Buckets[hashInfo.BucketIndex];
    entry->Hash = hashInfo.Hash;
    entry->Value = value;
    entry->Next = bucketHead;

    storage->Buckets[hashInfo.BucketIndex] = entryIndex;
    UnlockSystemDictionary(storage);
}

template<typename TValue>
void RemoveDictionaryEntry(SystemDictionaryStorage<TValue>* storage, SystemDictionaryHashInfo hashInfo)
{
    if (storage == nullptr || hashInfo.BucketIndex == SYSTEM_DICTIONARY_INDEX_EMPTY)
    {
        return;
    }

    LockSystemDictionary(storage);
    auto entryIndex = GetDictionaryEntryIndexInfo(storage, hashInfo);

    if (entryIndex.Index == SYSTEM_DICTIONARY_INDEX_EMPTY)
    {
        UnlockSystemDictionary(storage);

        #ifdef ElemAPI
        SystemLogErrorMessage(ElemLogMessageCategory_Application, "No entry found to delete.");
        #endif
        return;
    }

    auto entry = GetDictionaryEntryByIndex(storage, entryIndex.Index);
    SystemAssert(entry);

    if (entryIndex.ParentIndex == SYSTEM_DICTIONARY_INDEX_EMPTY)
    {
        storage->Buckets[hashInfo.BucketIndex] = entry->Next;
    }
    else
    {
        auto parentEntry = GetDictionaryEntryByIndex(storage, entryIndex.ParentIndex);
        SystemAssert(parentEntry);
        parentEntry->Next = entry->Next;
    }

    entry->Hash = 0;
    entry->Value = {};
    InsertFreeListEntry(storage, entryIndex.Index, entry);
    UnlockSystemDictionary(storage);
}

template<typename TValue>
TValue* GetDictionaryValue(SystemDictionaryStorage<TValue>* storage, SystemDictionaryHashInfo hashInfo)
{
    if (storage == nullptr || hashInfo.BucketIndex == SYSTEM_DICTIONARY_INDEX_EMPTY)
    {
        return nullptr;
    }

    LockSystemDictionary(storage);
    auto entryIndex = GetDictionaryEntryIndexInfo(storage, hashInfo);
    auto entry = GetDictionaryEntryByIndex(storage, entryIndex.Index);
    auto result = entry != nullptr ? &entry->Value : nullptr;
    UnlockSystemDictionary(storage);
    return result;
}

template<typename TValue>
bool ContainsDictionaryValue(SystemDictionaryStorage<TValue>* storage, SystemDictionaryHashInfo hashInfo)
{
    if (storage == nullptr || hashInfo.BucketIndex == SYSTEM_DICTIONARY_INDEX_EMPTY)
    {
        return false;
    }

    LockSystemDictionary(storage);
    auto entryIndex = GetDictionaryEntryIndexInfo(storage, hashInfo);
    auto result = entryIndex.Index != SYSTEM_DICTIONARY_INDEX_EMPTY;
    UnlockSystemDictionary(storage);
    return result;
}

template<typename TValue, typename T>
SystemDictionaryHashInfo DictionaryComputeHashInfo(SystemDictionaryStorage<TValue>* storage, ReadOnlySpan<T> data)
{
    if (storage == nullptr || storage->Buckets.Length == 0 || data.Length > SIZE_MAX / sizeof(T))
    {
        return { 0, SYSTEM_DICTIONARY_INDEX_EMPTY };
    }

    auto dataSizeInBytes = data.Length * sizeof(T);
    auto hash = XXH64(data.Pointer, dataSizeInBytes, SYSTEM_DICTIONARY_HASH_SEED);
    auto bucketIndex = (int32_t)(hash % storage->Buckets.Length);

    return { hash, bucketIndex };
}

template<typename TKey, typename TValue>
TValue& SystemDictionary<TKey, TValue>::operator[](TKey key)
{
    auto value = SystemGetDictionaryValue(*this, key);

    if (value != nullptr)
    {
        return *value;
    }

    static thread_local TValue defaultValue = {};
    defaultValue = {};
    return defaultValue;
}

template<typename TKey, typename TValue>
SystemDictionary<TKey, TValue> SystemCreateDictionary(MemoryArena memoryArena, size_t maxItemsCount)
{
    if (maxItemsCount > INT32_MAX)
    {
        return {};
    }

    auto storage = SystemPushStructZero<SystemDictionaryStorage<TValue>>(memoryArena);

    if (storage == nullptr)
    {
        return {};
    }

    storage->MemoryArena = memoryArena;
    storage->Buckets = SystemPushArray<int32_t>(memoryArena, maxItemsCount);

    if (maxItemsCount > 0 && storage->Buckets.Pointer == nullptr)
    {
        return {};
    }

    for (size_t i = 0; i < storage->Buckets.Length; i++)
    {
        storage->Buckets[i] = SYSTEM_DICTIONARY_INDEX_EMPTY;
    }

    storage->Entries = SystemPushArray<SystemDictionaryEntry<TValue>>(memoryArena, maxItemsCount, AllocationState_Reserved);

    if (maxItemsCount > 0 && storage->Entries.Pointer == nullptr)
    {
        return {};
    }

    storage->FreeListIndex = SYSTEM_DICTIONARY_INDEX_EMPTY;

    SystemDictionary<TKey, TValue> result = {};
    result.Storage = storage;
    return result;
}

template<typename TKey, typename TValue>
void SystemAddDictionaryEntry(SystemDictionary<TKey, TValue> dictionary, TKey key, TValue value)
{
    auto hashInfo = DictionaryComputeHashInfo(dictionary.Storage, ReadOnlySpan<uint8_t>((uint8_t*)&key, sizeof(key)));
    AddDictionaryEntry(dictionary.Storage, hashInfo, value);
}

template<typename TValue>
void SystemAddDictionaryEntry(SystemDictionary<ReadOnlySpan<char>, TValue> dictionary, ReadOnlySpan<char> key, TValue value)
{
    auto hashInfo = DictionaryComputeHashInfo(dictionary.Storage, key);
    AddDictionaryEntry(dictionary.Storage, hashInfo, value);
}

template<typename TValue, typename T>
void SystemAddDictionaryEntry(SystemDictionary<ReadOnlySpan<T>, TValue> dictionary, ReadOnlySpan<T> key, TValue value)
{
    auto hashInfo = DictionaryComputeHashInfo(dictionary.Storage, key);
    AddDictionaryEntry(dictionary.Storage, hashInfo, value);
}

template<typename TKey, typename TValue>
void SystemRemoveDictionaryEntry(SystemDictionary<TKey, TValue> dictionary, TKey key)
{
    auto hashInfo = DictionaryComputeHashInfo(dictionary.Storage, ReadOnlySpan<uint8_t>((uint8_t*)&key, sizeof(key)));
    RemoveDictionaryEntry(dictionary.Storage, hashInfo);
}

template<typename TValue>
void SystemRemoveDictionaryEntry(SystemDictionary<ReadOnlySpan<char>, TValue> dictionary, ReadOnlySpan<char> key)
{
    auto hashInfo = DictionaryComputeHashInfo(dictionary.Storage, key);
    RemoveDictionaryEntry(dictionary.Storage, hashInfo);
}

template<typename TValue, typename T>
void SystemRemoveDictionaryEntry(SystemDictionary<ReadOnlySpan<T>, TValue> dictionary, ReadOnlySpan<T> key)
{
    auto hashInfo = DictionaryComputeHashInfo(dictionary.Storage, key);
    RemoveDictionaryEntry(dictionary.Storage, hashInfo);
}

template<typename TKey, typename TValue>
TValue* SystemGetDictionaryValue(SystemDictionary<TKey, TValue> dictionary, TKey key)
{
    auto hashInfo = DictionaryComputeHashInfo(dictionary.Storage, ReadOnlySpan<uint8_t>((uint8_t*)&key, sizeof(key)));
    return GetDictionaryValue(dictionary.Storage, hashInfo);
}

template<typename TValue>
TValue* SystemGetDictionaryValue(SystemDictionary<ReadOnlySpan<char>, TValue> dictionary, ReadOnlySpan<char> key)
{
    auto hashInfo = DictionaryComputeHashInfo(dictionary.Storage, key);
    return GetDictionaryValue(dictionary.Storage, hashInfo);
}

template<typename TValue, typename T>
TValue* SystemGetDictionaryValue(SystemDictionary<ReadOnlySpan<T>, TValue> dictionary, ReadOnlySpan<T> key)
{
    auto hashInfo = DictionaryComputeHashInfo(dictionary.Storage, key);
    return GetDictionaryValue(dictionary.Storage, hashInfo);
}

template<typename TKey, typename TValue>
bool SystemDictionaryContainsKey(SystemDictionary<TKey, TValue> dictionary, TKey key)
{
    auto hashInfo = DictionaryComputeHashInfo(dictionary.Storage, ReadOnlySpan<uint8_t>((uint8_t*)&key, sizeof(key)));
    return ContainsDictionaryValue(dictionary.Storage, hashInfo);
}

template<typename TValue>
bool SystemDictionaryContainsKey(SystemDictionary<ReadOnlySpan<char>, TValue> dictionary, ReadOnlySpan<char> key)
{
    auto hashInfo = DictionaryComputeHashInfo(dictionary.Storage, key);
    return ContainsDictionaryValue(dictionary.Storage, hashInfo);
}

template<typename TValue, typename T>
bool SystemDictionaryContainsKey(SystemDictionary<ReadOnlySpan<T>, TValue> dictionary, ReadOnlySpan<T> key)
{
    auto hashInfo = DictionaryComputeHashInfo(dictionary.Storage, key);
    return ContainsDictionaryValue(dictionary.Storage, hashInfo);
}

template<typename TKey, typename TValue>
void SystemDebugDictionary(SystemDictionary<TKey, TValue> dictionary)
{
    #ifdef ElemAPI
    auto storage = dictionary.Storage;

    if (storage == nullptr)
    {
        return;
    }

    LockSystemDictionary(storage);

    for (size_t i = 0; i < storage->Buckets.Length; i++)
    {
        auto entryIndex = storage->Buckets[i];
        SystemLogDebugMessage(ElemLogMessageCategory_Application, "Bucket %u => %d", (uint32_t)i, entryIndex);
    }

    UnlockSystemDictionary(storage);
    #endif
}
