#include "SystemDictionary.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"

#define SYSTEM_DICTIONARY_HASH_SEED 123456789
#define SYSTEM_DICTIONARY_INDEX_EMPTY -1
#define SYSTEM_DICTIONARY_ALLOCATE_SIZE 1024

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
    size_t CurrentEntryIndex;
    size_t CurrentCommitIndex;
    int32_t FreeListIndex;
    bool IsCommitOperationInProgress;
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
SystemDictionaryIndexInfo GetDictionaryEntryIndexInfo(SystemDictionaryStorage<TValue>* storage, SystemDictionaryHashInfo hashInfo)
{
    int32_t currentIndex;
    __atomic_load(&storage->Buckets[hashInfo.BucketIndex], &currentIndex, __ATOMIC_ACQUIRE);

    auto rootIndex = currentIndex;
    auto parentIndex = SYSTEM_DICTIONARY_INDEX_EMPTY;

    while (currentIndex != SYSTEM_DICTIONARY_INDEX_EMPTY) 
    {
        auto currentEntry = GetDictionaryEntryByIndex(storage, currentIndex);

        if (currentEntry->Hash == hashInfo.Hash)
        {
            SystemDictionaryIndexInfo result = {};
            result.BucketIndex = hashInfo.BucketIndex;
            result.RootIndex = rootIndex;
            result.ParentIndex = parentIndex;
            result.Index = currentIndex;

            return result;
        }

        parentIndex = currentIndex;
        __atomic_load(&currentEntry->Next, &currentIndex, __ATOMIC_ACQUIRE);
    }

    return { -1, SYSTEM_DICTIONARY_INDEX_EMPTY, SYSTEM_DICTIONARY_INDEX_EMPTY, SYSTEM_DICTIONARY_INDEX_EMPTY };
}

template<typename TValue>
SystemDictionaryEntry<TValue>* GetDictionaryEntryByIndex(SystemDictionaryStorage<TValue>* storage, int32_t index)
{
    if (index == SYSTEM_DICTIONARY_INDEX_EMPTY)
    {
        return nullptr;
    }

    return &(storage->Entries[index]);
}

template<typename TValue>
int32_t GetFreeListEntry(SystemDictionaryStorage<TValue>* storage)
{
    int32_t entryIndex;
    __atomic_load(&storage->FreeListIndex, &entryIndex, __ATOMIC_ACQUIRE);

    SystemDictionaryEntry<TValue>* freeListEntry = nullptr;

    do
    {
        if (entryIndex == SYSTEM_DICTIONARY_INDEX_EMPTY)
        {
            break;
        }

        if (freeListEntry != nullptr)
        {
            SystemYieldThread();
        }

        freeListEntry = GetDictionaryEntryByIndex(storage, entryIndex);
    }
    while (!__atomic_compare_exchange_n(&storage->FreeListIndex, &entryIndex, freeListEntry->Next, true, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE));

    return entryIndex;
}

template<typename TValue>
void InsertFreeListEntry(SystemDictionaryStorage<TValue>* storage, int32_t index, SystemDictionaryEntry<TValue>* entry)
{
    int32_t entryIndex;
    __atomic_load(&storage->FreeListIndex, &entryIndex, __ATOMIC_ACQUIRE);

    entry->Next = entryIndex;

    while (!__atomic_compare_exchange_n(&storage->FreeListIndex, &entryIndex, index, true, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE))
    {
        entry->Next = entryIndex;
        SystemYieldThread();
    }
}
int counter = 0;
template<typename TValue>
void CommitEntriesMemory(SystemDictionaryStorage<TValue>* storage)
{
    auto entrySize = sizeof(SystemDictionaryEntry<TValue>);
    printf("Committing Offset %d: %llu, Size: %llu\n", counter++, storage->CurrentEntryIndex * entrySize, SYSTEM_DICTIONARY_ALLOCATE_SIZE * entrySize);
    
    SystemCommitMemory(storage->MemoryArena, (uint8_t*)storage->Entries.Pointer + storage->CurrentEntryIndex * entrySize, SYSTEM_DICTIONARY_ALLOCATE_SIZE * entrySize, true);
    SystemAtomicAdd(storage->CurrentCommitIndex, SYSTEM_DICTIONARY_ALLOCATE_SIZE);
}

template<typename TValue>
void AddDictionaryEntry(SystemDictionaryStorage<TValue>* storage, SystemDictionaryHashInfo hashInfo, TValue value)
{
    printf("Add Entry %d\n", value);
    // TODO: create a SystemAtomicLoad

    auto entryIndex = GetFreeListEntry(storage); 

    if (entryIndex == SYSTEM_DICTIONARY_INDEX_EMPTY) 
    {
        entryIndex = SystemAtomicAdd(storage->CurrentEntryIndex, 1);

        if (storage->CurrentEntryIndex == storage->Entries.Length)
        {
            SystemLogErrorMessage(LogMessageCategory_NativeApplication, "Max items in dictionary reached, the item will not be added.");
            return;
        }
      
        if (entryIndex > storage->CurrentCommitIndex)                
        {
            SystemAtomicReplaceWithValue(storage->IsCommitOperationInProgress, false, true);

            while (entryIndex > storage->CurrentCommitIndex)
            {
                CommitEntriesMemory(storage);
            }

            SystemAtomicStore(storage->IsCommitOperationInProgress, false);
        }
    }
    
    auto entry = GetDictionaryEntryByIndex(storage, entryIndex);

    int32_t bucketHead;
    __atomic_load(&storage->Buckets[hashInfo.BucketIndex], &bucketHead, __ATOMIC_ACQUIRE);

    auto firstTry = true;

    do
    {
        if (!firstTry)
        {
            SystemYieldThread();
        }
        else 
        {
            firstTry = false;
        }

        if (bucketHead != SYSTEM_DICTIONARY_INDEX_EMPTY)
        {
            auto bucketHeadEntry = GetDictionaryEntryByIndex(storage, bucketHead);

            if (bucketHeadEntry->Hash != 0)
            {
                entry->Next = bucketHead;
            }
            else 
            {
                entry->Next = SYSTEM_DICTIONARY_INDEX_EMPTY;
            }
        }
        else 
        {
            entry->Next = SYSTEM_DICTIONARY_INDEX_EMPTY;
        }
    }
    while (!__atomic_compare_exchange_n(&storage->Buckets[hashInfo.BucketIndex], &bucketHead, entryIndex, true, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE));

    entry->Hash = hashInfo.Hash;
    entry->Value = value;
}

template<typename TValue>
void RemoveDictionaryEntry(SystemDictionaryStorage<TValue>* storage, SystemDictionaryHashInfo hashInfo)
{
    SystemDictionaryIndexInfo entryIndex = {};
    SystemDictionaryEntry<TValue>* entry = nullptr;
    int32_t* parentNextEntryIndex = nullptr;
    int32_t retryCount = 0;

    do
    {
        entryIndex = GetDictionaryEntryIndexInfo(storage, hashInfo);

        if (entryIndex.Index == SYSTEM_DICTIONARY_INDEX_EMPTY)
        {
            if (retryCount < 5)
            {
                SystemLogDebugMessage(LogMessageCategory_NativeApplication, "Retrying to find the item to delete");
                SystemYieldThread();
                retryCount++;
                entry = nullptr;
                continue;
            }

            SystemLogErrorMessage(LogMessageCategory_NativeApplication, "No entry found to delete.");
            return;
        }

        entry = GetDictionaryEntryByIndex(storage, entryIndex.Index);

        if (entryIndex.RootIndex != entryIndex.Index)
        {
            auto parentEntry = GetDictionaryEntryByIndex(storage, entryIndex.ParentIndex);
            parentNextEntryIndex = &parentEntry->Next;
        }
        else 
        {
            parentNextEntryIndex = &storage->Buckets[hashInfo.BucketIndex];
        }

        SystemYieldThread();
    }
    while (entry == nullptr || !__atomic_compare_exchange_n(parentNextEntryIndex, &entryIndex.Index, entry->Next, true, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE));

    entry->Hash = 0;
    entry->Value = {};
    InsertFreeListEntry(storage, entryIndex.Index, entry);
}

template<typename TValue>
TValue* GetDictionaryValue(SystemDictionaryStorage<TValue>* storage, SystemDictionaryHashInfo hashInfo)
{
    auto entryIndex = GetDictionaryEntryIndexInfo(storage, hashInfo);
    auto entry = GetDictionaryEntryByIndex(storage, entryIndex.Index);

    if (entry != nullptr)
    {
        return &entry->Value;
    }

    static TValue defaultValue;
    return &defaultValue;
}

template<typename TValue, typename T>
SystemDictionaryHashInfo DictionaryComputeHashInfo(SystemDictionaryStorage<TValue>* storage, ReadOnlySpan<T> data)
{
    auto hash = XXH64(data.Pointer, data.Length, SYSTEM_DICTIONARY_HASH_SEED);
    auto bucketIndex = (int32_t)(hash % storage->Buckets.Length);

    SystemDictionaryHashInfo result = {};
    result.Hash = hash;
    result.BucketIndex = bucketIndex;

    return result;
}

template<typename TKey, typename TValue>
TValue& SystemDictionary<TKey, TValue>::operator[](TKey key)
{
    return *SystemGetDictionaryValue(*this, key);
}

template<typename TKey, typename TValue>
SystemDictionary<TKey, TValue> SystemCreateDictionary(MemoryArena memoryArena, size_t maxItemsCount)
{
    auto storage = SystemPushStruct<SystemDictionaryStorage<TValue>>(memoryArena);
    storage->MemoryArena = memoryArena;
    storage->Buckets = SystemPushArray<int32_t>(memoryArena, maxItemsCount);

    for (size_t i = 0; i < storage->Buckets.Length; i++)
    {
        storage->Buckets[i] = SYSTEM_DICTIONARY_INDEX_EMPTY;
    }

    storage->Entries = SystemPushArray<SystemDictionaryEntry<TValue>>(memoryArena, maxItemsCount, AllocationState_Reserved);
    storage->CurrentEntryIndex = 0;
    storage->CurrentCommitIndex = 0;

    CommitEntriesMemory(storage);

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
    auto entryIndex = GetDictionaryEntryIndexInfo(dictionary.Storage, hashInfo);

    return entryIndex.Index != SYSTEM_DICTIONARY_INDEX_EMPTY;
}

template<typename TValue>
bool SystemDictionaryContainsKey(SystemDictionary<ReadOnlySpan<char>, TValue> dictionary, ReadOnlySpan<char> key)
{
    auto hashInfo = DictionaryComputeHashInfo(dictionary.Storage, key);
    auto entryIndex = GetDictionaryEntryIndexInfo(dictionary.Storage, hashInfo);

    return entryIndex.Index != SYSTEM_DICTIONARY_INDEX_EMPTY;
}

template<typename TValue, typename T>
bool SystemDictionaryContainsKey(SystemDictionary<ReadOnlySpan<T>, TValue> dictionary, ReadOnlySpan<T> key)
{
    auto hashInfo = DictionaryComputeHashInfo(dictionary.Storage, key);
    auto entryIndex = GetDictionaryEntryIndexInfo(dictionary.Storage, hashInfo);

    return entryIndex.Index != SYSTEM_DICTIONARY_INDEX_EMPTY;
}

template<typename TKey, typename TValue>
SystemDictionaryEnumerator<TKey, TValue> SystemGetDictionaryEnumerator(SystemDictionary<TKey, TValue> dictionary)
{
    SystemDictionaryEnumerator<TKey, TValue> result = {};
    result.Dictionary = dictionary;
    result.Index = 0;

    return result;
}

template<typename TKey, typename TValue>
TValue* SystemGetDictionaryEnumeratorNextValue(SystemDictionaryEnumerator<TKey, TValue>* enumerator)
{
    auto storage = enumerator->Dictionary.Storage;

    while (enumerator->Index < storage->Entries.Length)
    {
        auto result = &(storage->Entries[enumerator->Index]);
        enumerator->Index++;
        
        if (result->Hash != 0)
        {
            return &result->Value;
        }
    }

    return nullptr;
}

template<typename TKey, typename TValue>
void SystemDebugDictionary(SystemDictionary<TKey, TValue> dictionary)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto storage = dictionary.Storage;

    for (size_t i = 0; i < storage->Buckets.Length; i++)
    {
        if (storage->Buckets[i] == SYSTEM_DICTIONARY_INDEX_EMPTY)
        {
            SystemLogDebugMessage(LogMessageCategory_NativeApplication, "Bucket %u => (EMPTY)", i);
        }
        else 
        {
            auto debugMessage = SystemFormatString(stackMemoryArena, "Bucket %u", i);

            auto entryIndex = storage->Buckets[i]; 

            while (entryIndex != SYSTEM_DICTIONARY_INDEX_EMPTY)
            {
                auto entryIndexFull = GetDictionaryEntryIndexFull(entryIndex);
                auto entry = storage->Partitions[entryIndexFull.PartitionIndex]->Entries[entryIndexFull.Index]; 

                if (entry.Hash == 0)
                {
                    debugMessage = SystemConcatBuffers<char>(stackMemoryArena, debugMessage, " => (PREV_REMOVED)");
                    break;
                }

                debugMessage = SystemConcatBuffers<char>(stackMemoryArena, debugMessage, SystemFormatString(stackMemoryArena, " => %u (Value: %d, Partition: %d, Index: %d)", entry.Hash, entry.Value, entryIndexFull.PartitionIndex, entryIndexFull.Index));
                entryIndex = entry.Next;
            }

            SystemLogDebugMessage(LogMessageCategory_NativeApplication, "%s", debugMessage.Pointer);
        }
    }

    auto currentFreeListIndex = storage->FreeListIndex;
    auto debugMessage = ReadOnlySpan<char>("FreeList");

    while (currentFreeListIndex != SYSTEM_DICTIONARY_INDEX_EMPTY)
    {
        auto indexFull = GetDictionaryEntryIndexFull(currentFreeListIndex);
        debugMessage = SystemConcatBuffers<char>(stackMemoryArena, debugMessage, SystemFormatString(stackMemoryArena, " => Partition: %d, Index: %d", indexFull.PartitionIndex, indexFull.Index));
        currentFreeListIndex = storage->Partitions[indexFull.PartitionIndex]->Entries[indexFull.Index].Next;
    }

    SystemLogDebugMessage(LogMessageCategory_NativeApplication, "%s", debugMessage.Pointer);
}
