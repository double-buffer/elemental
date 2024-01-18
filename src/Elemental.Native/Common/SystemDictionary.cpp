#include "SystemDictionary.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"

#define SYSTEM_DICTIONARY_HASH_SEED 123456789
#define SYSTEM_DICTIONARY_PARTITION_COUNT 16
#define SYSTEM_DICTIONARY_PARTITION_MIN_SIZE 1024
#define SYSTEM_DICTIONARY_PARTITION_BITS 10
#define SYSTEM_DICTIONARY_INDEX_BITS 22
#define SYSTEM_DICTIONARY_MAX_PARTITION_INDEX ((1 << SYSTEM_DICTIONARY_PARTITION_BITS) - 1)
#define SYSTEM_DICTIONARY_MAX_INDEX ((1 << SYSTEM_DICTIONARY_INDEX_BITS) - 1)

struct SystemDictionaryEntryIndex
{
    int32_t CombinedIndex;

    bool operator==(const SystemDictionaryEntryIndex& other) const 
    {
        return CombinedIndex == other.CombinedIndex;
    }

    bool operator!=(const SystemDictionaryEntryIndex& other) const 
    {
        return CombinedIndex != other.CombinedIndex;
    }
};

struct SystemDictionaryEntryIndexFull
{
    int32_t PartitionIndex;
    int32_t Index;
};
    
SystemDictionaryEntryIndex SYSTEM_DICTIONARY_INDEX_EMPTY = { -1 };

template<typename TValue>
struct SystemDictionaryEntry
{
    uint64_t Hash;
    TValue Value;
    SystemDictionaryEntryIndex Next;
};

template<typename TValue> 
struct SystemDictionaryStoragePartition
{
    Span<SystemDictionaryEntry<TValue>> Entries;
};

template<typename TValue> 
struct SystemDictionaryStorage
{
    MemoryArena MemoryArena;
    Span<SystemDictionaryEntryIndex> Buckets;
    Span<SystemDictionaryStoragePartition<TValue>*> Partitions;
    size_t PartitionSize;
    size_t CurrentPartitionIndex;
    bool IsPartitionBeingCreated;
    SystemDictionaryEntryIndex FreeListIndex;
};

struct SystemDictionaryIndexInfo
{
    int32_t BucketIndex;
    SystemDictionaryEntryIndex RootIndex;
    SystemDictionaryEntryIndex ParentIndex;
    SystemDictionaryEntryIndex Index;
};

struct SystemDictionaryHashInfo
{
    uint64_t Hash;
    int32_t BucketIndex;
};

SystemDictionaryEntryIndex CreateDictionaryEntryIndex(int32_t partitionIndex, int32_t index)
{
    return { (partitionIndex & SYSTEM_DICTIONARY_MAX_PARTITION_INDEX) << SYSTEM_DICTIONARY_INDEX_BITS | (index & SYSTEM_DICTIONARY_MAX_INDEX) };
}

SystemDictionaryEntryIndexFull GetDictionaryEntryIndexFull(SystemDictionaryEntryIndex index)
{
    return 
    { 
        index.CombinedIndex == -1 ? -1 : (index.CombinedIndex >> SYSTEM_DICTIONARY_INDEX_BITS) & SYSTEM_DICTIONARY_MAX_PARTITION_INDEX, 
        index.CombinedIndex == -1 ? -1 : index.CombinedIndex & SYSTEM_DICTIONARY_MAX_INDEX 
    };
}

template<typename TValue>
SystemDictionaryEntryIndex GetFreeListEntry(SystemDictionaryStorage<TValue>* storage)
{
    SystemDictionaryEntryIndex entryIndex;
    __atomic_load(&storage->FreeListIndex.CombinedIndex, &entryIndex.CombinedIndex, __ATOMIC_ACQUIRE);

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
    while (!__atomic_compare_exchange_n(&storage->FreeListIndex.CombinedIndex, &entryIndex.CombinedIndex, freeListEntry->Next.CombinedIndex, true, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE));

    return entryIndex;
}

template<typename TValue>
void InsertFreeListEntry(SystemDictionaryStorage<TValue>* storage, SystemDictionaryEntryIndex index, SystemDictionaryEntry<TValue>* entry)
{
    SystemDictionaryEntryIndex entryIndex;
    __atomic_load(&storage->FreeListIndex.CombinedIndex, &entryIndex.CombinedIndex, __ATOMIC_ACQUIRE);

    entry->Next = entryIndex;

    while (!__atomic_compare_exchange_n(&storage->FreeListIndex.CombinedIndex, &entryIndex.CombinedIndex, index.CombinedIndex, true, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE))
    {
        entry->Next = entryIndex;
        SystemYieldThread();
    }
}

template<typename TValue>
SystemDictionaryStoragePartition<TValue>* CreateDictionaryPartition(MemoryArena memoryArena, size_t partitionIndex, size_t partitionSize)
{
    auto partition = SystemPushStruct<SystemDictionaryStoragePartition<TValue>>(memoryArena);
    partition->Entries = SystemPushArrayZero<SystemDictionaryEntry<TValue>>(memoryArena, partitionSize);

    for (size_t i = 0; i < partition->Entries.Length; i++)
    {
        partition->Entries[i].Next = i < partition->Entries.Length - 1 ? CreateDictionaryEntryIndex(partitionIndex, i + 1) : SYSTEM_DICTIONARY_INDEX_EMPTY;
    }

    return partition;
}

template<typename TValue>
void AddDictionaryEntry(SystemDictionaryStorage<TValue>* storage, SystemDictionaryHashInfo hashInfo, TValue value)
{
    auto entryIndex = GetFreeListEntry(storage); 

    if (entryIndex == SYSTEM_DICTIONARY_INDEX_EMPTY) 
    {
        auto expected = false;

        while (!__atomic_compare_exchange_n(&storage->IsPartitionBeingCreated, &expected, true, true, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE))
        {
            expected = false;
            SystemYieldThread();
        }
            
        entryIndex = GetFreeListEntry(storage);

        if (entryIndex == SYSTEM_DICTIONARY_INDEX_EMPTY) 
        {
            if ((storage->CurrentPartitionIndex + 1) == storage->Partitions.Length)
            {
                SystemLogErrorMessage(LogMessageCategory_NativeApplication, "Max items in dictionary reached, the item will not be added.");
                __atomic_store_n(&storage->IsPartitionBeingCreated, false, __ATOMIC_RELEASE);
                return;
            }

            SystemLogDebugMessage(LogMessageCategory_NativeApplication, "Dictionary partition is full, allocating a new partition.");
            storage->CurrentPartitionIndex++;
            storage->Partitions[storage->CurrentPartitionIndex] = CreateDictionaryPartition<TValue>(storage->MemoryArena, storage->CurrentPartitionIndex, storage->PartitionSize);
            storage->FreeListIndex = CreateDictionaryEntryIndex(storage->CurrentPartitionIndex, 1);

            entryIndex = CreateDictionaryEntryIndex(storage->CurrentPartitionIndex, 0);
        }

        __atomic_store_n(&storage->IsPartitionBeingCreated, false, __ATOMIC_RELEASE);
    }
    
    auto entry = GetDictionaryEntryByIndex(storage, entryIndex);

    SystemDictionaryEntryIndex bucketHead;
    __atomic_load(&storage->Buckets[hashInfo.BucketIndex].CombinedIndex, &bucketHead.CombinedIndex, __ATOMIC_ACQUIRE);

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
    while (!__atomic_compare_exchange_n(&storage->Buckets[hashInfo.BucketIndex].CombinedIndex, &bucketHead.CombinedIndex, entryIndex.CombinedIndex, true, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE));

    entry->Hash = hashInfo.Hash;
    entry->Value = value;
}

template<typename TValue>
void RemoveDictionaryEntry(SystemDictionaryStorage<TValue>* storage, SystemDictionaryHashInfo hashInfo)
{
    SystemDictionaryIndexInfo entryIndex = {};
    SystemDictionaryEntry<TValue>* entry = nullptr;
    SystemDictionaryEntryIndex* parentNextEntryIndex = nullptr;
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
    while (entry == nullptr || !__atomic_compare_exchange_n(&parentNextEntryIndex->CombinedIndex, &entryIndex.Index.CombinedIndex, entry->Next.CombinedIndex, true, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE));

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

template<typename TValue>
SystemDictionaryIndexInfo GetDictionaryEntryIndexInfo(SystemDictionaryStorage<TValue>* storage, SystemDictionaryHashInfo hashInfo)
{
    SystemDictionaryEntryIndex currentIndex;
    __atomic_load(&storage->Buckets[hashInfo.BucketIndex].CombinedIndex, &currentIndex.CombinedIndex, __ATOMIC_ACQUIRE);

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
        __atomic_load(&currentEntry->Next.CombinedIndex, &currentIndex.CombinedIndex, __ATOMIC_ACQUIRE);
    }

    return { -1, SYSTEM_DICTIONARY_INDEX_EMPTY, SYSTEM_DICTIONARY_INDEX_EMPTY, SYSTEM_DICTIONARY_INDEX_EMPTY };
}

template<typename TValue>
SystemDictionaryEntry<TValue>* GetDictionaryEntryByIndex(SystemDictionaryStorage<TValue>* storage, SystemDictionaryEntryIndex index)
{
    if (index == SYSTEM_DICTIONARY_INDEX_EMPTY)
    {
        return nullptr;
    }

    auto indexFull = GetDictionaryEntryIndexFull(index);
    return &(storage->Partitions[indexFull.PartitionIndex]->Entries[indexFull.Index]);
}

template<typename TKey, typename TValue>
TValue& SystemDictionary<TKey, TValue>::operator[](TKey key)
{
    return *SystemGetDictionaryValue(*this, key);
}

template<typename TKey, typename TValue>
SystemDictionary<TKey, TValue> SystemCreateDictionary(MemoryArena memoryArena, size_t maxItemsCount)
{
    auto partitionCount = (maxItemsCount + SYSTEM_DICTIONARY_PARTITION_MIN_SIZE - 1) / SYSTEM_DICTIONARY_PARTITION_MIN_SIZE;
    
    if (partitionCount > SYSTEM_DICTIONARY_PARTITION_COUNT) 
    {
        partitionCount = SYSTEM_DICTIONARY_PARTITION_COUNT;
    }

    auto partitionSize = (maxItemsCount + partitionCount - 1) / partitionCount;

    if (maxItemsCount < SYSTEM_DICTIONARY_PARTITION_MIN_SIZE && partitionCount == 1) 
    {
        partitionSize = maxItemsCount;
    }
   
    auto storage = SystemPushStruct<SystemDictionaryStorage<TValue>>(memoryArena);
    storage->MemoryArena = memoryArena;
    storage->Partitions = SystemPushArrayZero<SystemDictionaryStoragePartition<TValue>*>(memoryArena, partitionCount);
    storage->CurrentPartitionIndex = 0;
    storage->Partitions[storage->CurrentPartitionIndex] = CreateDictionaryPartition<TValue>(memoryArena, 0, partitionSize);
    storage->PartitionSize = partitionSize;
    storage->IsPartitionBeingCreated = false;
    
    storage->FreeListIndex = CreateDictionaryEntryIndex(0, 0);
    storage->Buckets = SystemPushArrayZero<SystemDictionaryEntryIndex>(memoryArena, maxItemsCount);

    for (size_t i = 0; i < storage->Buckets.Length; i++)
    {
        storage->Buckets[i] = SYSTEM_DICTIONARY_INDEX_EMPTY;
    }

    return { storage };
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
    result.PartitionIndex = 0;
    result.Index = 0;

    return result;
}

template<typename TKey, typename TValue>
TValue* SystemGetDictionaryEnumeratorNextValue(SystemDictionaryEnumerator<TKey, TValue>* enumerator)
{
    auto storage = enumerator->Dictionary.Storage;

    while (enumerator->PartitionIndex < storage->Partitions.Length)
    {
        if (storage->Partitions[enumerator->PartitionIndex] == nullptr)
        {
            break;
        }

        auto result = &(storage->Partitions[enumerator->PartitionIndex]->Entries[enumerator->Index]);
        enumerator->Index++;
        
        if (enumerator->Index == storage->Partitions[enumerator->PartitionIndex]->Entries.Length)
        {
            enumerator->PartitionIndex++;
            enumerator->Index = 0;
        }
        
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
                auto entryIndexFull = SystemGetDictionaryEntryIndexFull(entryIndex);
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
        auto indexFull = SystemGetDictionaryEntryIndexFull(currentFreeListIndex);
        debugMessage = SystemConcatBuffers<char>(stackMemoryArena, debugMessage, SystemFormatString(stackMemoryArena, " => Partition: %d, Index: %d", indexFull.PartitionIndex, indexFull.Index));
        currentFreeListIndex = storage->Partitions[indexFull.PartitionIndex]->Entries[indexFull.Index].Next;
    }

    SystemLogDebugMessage(LogMessageCategory_NativeApplication, "%s", debugMessage.Pointer);

    for (size_t p = 0; p < dictionary.Storage->Partitions.Length; p++)
    {
        auto partition = dictionary.Storage->Partitions[p];

        if (partition != nullptr)
        {
            debugMessage = SystemFormatString(stackMemoryArena, "Partition %u (Size: %u):", p, partition->Entries.Length);

            for (size_t j = 0; j < partition->Entries.Length; j++)
            {
                debugMessage = SystemConcatBuffers<char>(stackMemoryArena, debugMessage, SystemFormatString(stackMemoryArena, " %u =>", partition->Entries[j].Hash));
            }

            SystemLogDebugMessage(LogMessageCategory_NativeApplication, "%s", debugMessage.Pointer);
        }
        else 
        {
            SystemLogDebugMessage(LogMessageCategory_NativeApplication, "Partition %u => (EMPTY)\n", p); 
        }
    }
}

