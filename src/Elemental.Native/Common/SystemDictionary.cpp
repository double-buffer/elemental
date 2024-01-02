#include "SystemDictionary.h"
#include "SystemLogging.h"

#define SYSTEM_DICTIONARY_HASH_SEED 123456789
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
    MemoryArena* MemoryArena;
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

SystemDictionaryEntryIndex SystemCreateDictionaryEntryIndex(int32_t partitionIndex, int32_t index);
SystemDictionaryEntryIndexFull SystemGetDictionaryEntryIndexFull(SystemDictionaryEntryIndex index);

template<typename TValue>
void SystemAddDictionaryEntry(SystemDictionaryStorage<TValue>* storage, SystemDictionaryHashInfo hashInfo, TValue value);

template<typename TValue>
void SystemRemoveDictionaryEntry(SystemDictionaryStorage<TValue>* storage, SystemDictionaryHashInfo hashInfo);

template<typename TValue>
TValue* SystemGetDictionaryValue(SystemDictionaryStorage<TValue>* storage, SystemDictionaryHashInfo hashInfo);

// TODO: Change the signature to take the max items count
template<typename TValue>
SystemDictionaryStoragePartition<TValue>* SystemCreateDictionaryPartition(MemoryArena* memoryArena, size_t paritionIndex, size_t partitionSize);

template<typename TValue, typename T>
SystemDictionaryHashInfo SystemDictionaryComputeHashInfo(SystemDictionaryStorage<TValue>* storage, ReadOnlySpan<T> data);

template<typename TValue>
SystemDictionaryIndexInfo SystemGetDictionaryEntryIndexInfo(SystemDictionaryStorage<TValue>* storage, SystemDictionaryHashInfo hashInfo);

template<typename TValue>
SystemDictionaryEntry<TValue>* SystemGetDictionaryEntryByIndex(SystemDictionaryStorage<TValue>* storage, SystemDictionaryEntryIndex index);

template<typename TKey, typename TValue>
TValue& SystemDictionary<TKey, TValue>::operator[](TKey key)
{
    return *SystemGetDictionaryValue(*this, key);
}

template<typename TKey, typename TValue>
SystemDictionary<TKey, TValue> SystemCreateDictionary(MemoryArena* memoryArena, size_t maxItemsCount)
{
    // TODO: We need to have to initial size parameters, the entries and buckets ones
    // BUG: All here is wrong
    auto maxPartitionsCount = 2;
    auto partitionSize = maxItemsCount / maxPartitionsCount;

    if (partitionSize == 0)
    {
        partitionSize = 1;
    }

    auto storage = SystemPushStruct<SystemDictionaryStorage<TValue>>(memoryArena);
    storage->MemoryArena = memoryArena;
    storage->Partitions = SystemPushArrayZero<SystemDictionaryStoragePartition<TValue>*>(memoryArena, maxPartitionsCount);
    storage->CurrentPartitionIndex = 0;
    storage->Partitions[storage->CurrentPartitionIndex] = SystemCreateDictionaryPartition<TValue>(memoryArena, 0, partitionSize); // TODO: Find a proper initial size
    storage->PartitionSize = partitionSize;
    storage->IsPartitionBeingCreated = false;
    
    storage->FreeListIndex = SystemCreateDictionaryEntryIndex(0, 0);
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
    auto hashInfo = SystemDictionaryComputeHashInfo(dictionary.Storage, ReadOnlySpan<uint8_t>((uint8_t*)&key, sizeof(key)));
    SystemAddDictionaryEntry(dictionary.Storage, hashInfo, value);
}

template<typename TValue>
void SystemAddDictionaryEntry(SystemDictionary<ReadOnlySpan<char>, TValue> dictionary, ReadOnlySpan<char> key, TValue value)
{
    auto hashInfo = SystemDictionaryComputeHashInfo(dictionary.Storage, key);
    SystemAddDictionaryEntry(dictionary.Storage, hashInfo, value);
}

template<typename TValue, typename T>
void SystemAddDictionaryEntry(SystemDictionary<ReadOnlySpan<T>, TValue> dictionary, ReadOnlySpan<T> key, TValue value)
{
    auto hashInfo = SystemDictionaryComputeHashInfo(dictionary.Storage, key);
    SystemAddDictionaryEntry(dictionary.Storage, hashInfo, value);
}

template<typename TKey, typename TValue>
void SystemRemoveDictionaryEntry(SystemDictionary<TKey, TValue> dictionary, TKey key)
{
    auto hashInfo = SystemDictionaryComputeHashInfo(dictionary.Storage, ReadOnlySpan<uint8_t>((uint8_t*)&key, sizeof(key)));
    SystemRemoveDictionaryEntry(dictionary.Storage, hashInfo);
}

template<typename TValue>
void SystemRemoveDictionaryEntry(SystemDictionary<ReadOnlySpan<char>, TValue> dictionary, ReadOnlySpan<char> key)
{
    auto hashInfo = SystemDictionaryComputeHashInfo(dictionary.Storage, key);
    SystemRemoveDictionaryEntry(dictionary.Storage, hashInfo);
}

template<typename TValue, typename T>
void SystemRemoveDictionaryEntry(SystemDictionary<ReadOnlySpan<T>, TValue> dictionary, ReadOnlySpan<T> key)
{
    auto hashInfo = SystemDictionaryComputeHashInfo(dictionary.Storage, key);
    SystemRemoveDictionaryEntry(dictionary.Storage, hashInfo);
}

template<typename TKey, typename TValue>
TValue* SystemGetDictionaryValue(SystemDictionary<TKey, TValue> dictionary, TKey key)
{
    auto hashInfo = SystemDictionaryComputeHashInfo(dictionary.Storage, ReadOnlySpan<uint8_t>((uint8_t*)&key, sizeof(key)));
    return SystemGetDictionaryValue(dictionary.Storage, hashInfo);
}

template<typename TValue>
TValue* SystemGetDictionaryValue(SystemDictionary<ReadOnlySpan<char>, TValue> dictionary, ReadOnlySpan<char> key)
{
    auto hashInfo = SystemDictionaryComputeHashInfo(dictionary.Storage, key);
    return SystemGetDictionaryValue(dictionary.Storage, hashInfo);
}

template<typename TValue, typename T>
TValue* SystemGetDictionaryValue(SystemDictionary<ReadOnlySpan<T>, TValue> dictionary, ReadOnlySpan<T> key)
{
    auto hashInfo = SystemDictionaryComputeHashInfo(dictionary.Storage, key);
    return SystemGetDictionaryValue(dictionary.Storage, hashInfo);
}

template<typename TKey, typename TValue>
bool SystemDictionaryContainsKey(SystemDictionary<TKey, TValue> dictionary, TKey key)
{
    auto hashInfo = SystemDictionaryComputeHashInfo(dictionary.Storage, ReadOnlySpan<uint8_t>((uint8_t*)&key, sizeof(key)));
    auto entryIndex = SystemGetDictionaryEntryIndexInfo(dictionary.Storage, hashInfo);

    return entryIndex.Index != SYSTEM_DICTIONARY_INDEX_EMPTY;
}

template<typename TValue>
bool SystemDictionaryContainsKey(SystemDictionary<ReadOnlySpan<char>, TValue> dictionary, ReadOnlySpan<char> key)
{
    auto hashInfo = SystemDictionaryComputeHashInfo(dictionary.Storage, key);
    auto entryIndex = SystemGetDictionaryEntryIndexInfo(dictionary.Storage, hashInfo);

    return entryIndex.Index != SYSTEM_DICTIONARY_INDEX_EMPTY;
}

template<typename TValue, typename T>
bool SystemDictionaryContainsKey(SystemDictionary<ReadOnlySpan<T>, TValue> dictionary, ReadOnlySpan<T> key)
{
    auto hashInfo = SystemDictionaryComputeHashInfo(dictionary.Storage, key);
    auto entryIndex = SystemGetDictionaryEntryIndexInfo(dictionary.Storage, hashInfo);

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
    auto storage = dictionary.Storage;

    for (size_t i = 0; i < storage->Buckets.Length; i++)
    {
        if (storage->Buckets[i] == SYSTEM_DICTIONARY_INDEX_EMPTY)
        {
            printf("Bucket %zu => (EMPTY)\n", i);
        }
        else 
        {
            printf("Bucket %zu", i);

            auto entryIndex = storage->Buckets[i]; 

            while (entryIndex != SYSTEM_DICTIONARY_INDEX_EMPTY)
            {
                auto entryIndexFull = SystemGetDictionaryEntryIndexFull(entryIndex);
                auto entry = storage->Partitions[entryIndexFull.PartitionIndex]->Entries[entryIndexFull.Index]; 

                if (entry.Hash == 0)
                {
                    printf(" => (PREV_REMOVED)");
                    break;
                }

                printf(" => %llu (Value: %d, Partition: %d, Index: %d)", entry.Hash, entry.Value, entryIndexFull.PartitionIndex, entryIndexFull.Index);

                entryIndex = entry.Next;
            }

            printf("\n");
        }
    }

    auto currentFreeListIndex = storage->FreeListIndex;
    printf("FreeList");

    while (currentFreeListIndex != SYSTEM_DICTIONARY_INDEX_EMPTY)
    {
        auto indexFull = SystemGetDictionaryEntryIndexFull(currentFreeListIndex);
        printf(" => Partition: %d, Index: %d", indexFull.PartitionIndex, indexFull.Index);
        currentFreeListIndex = storage->Partitions[indexFull.PartitionIndex]->Entries[indexFull.Index].Next;
    }

    printf("\n");

    for (size_t p = 0; p < dictionary.Storage->Partitions.Length; p++)
    {
        auto partition = dictionary.Storage->Partitions[p];

        if (partition != nullptr)
        {
            printf("Partition %zu (Size: %zu): ", p, partition->Entries.Length);

            for (size_t j = 0; j < partition->Entries.Length; j++)
            {
                printf(" %llu =>", partition->Entries[j].Hash);
            }

            printf("\n");
        }
        else 
        {
            printf("(EMPTY)\n"); 
        }
    }
}

SystemDictionaryEntryIndex SystemCreateDictionaryEntryIndex(int32_t partitionIndex, int32_t index)
{
    return { (partitionIndex & SYSTEM_DICTIONARY_MAX_PARTITION_INDEX) << SYSTEM_DICTIONARY_INDEX_BITS | (index & SYSTEM_DICTIONARY_MAX_INDEX) };
}

SystemDictionaryEntryIndexFull SystemGetDictionaryEntryIndexFull(SystemDictionaryEntryIndex index)
{
    return 
    { 
        index.CombinedIndex == -1 ? -1 : (index.CombinedIndex >> SYSTEM_DICTIONARY_INDEX_BITS) & SYSTEM_DICTIONARY_MAX_PARTITION_INDEX, 
        index.CombinedIndex == -1 ? -1 : index.CombinedIndex & SYSTEM_DICTIONARY_MAX_INDEX 
    };
}

template<typename TValue>
SystemDictionaryEntryIndex SystemGetFreeListEntry(SystemDictionaryStorage<TValue>* storage)
{
    SystemDictionaryEntryIndex entryIndex;
    __atomic_load(&storage->FreeListIndex.CombinedIndex, &entryIndex.CombinedIndex, __ATOMIC_ACQUIRE);

    SystemDictionaryEntry<TValue>* freeListEntry = nullptr;

    // TODO: Change this to a while loop and do a Thread Yield
    do
    {
        if (entryIndex == SYSTEM_DICTIONARY_INDEX_EMPTY)
        {
            break;
        }

        freeListEntry = SystemGetDictionaryEntryByIndex(storage, entryIndex);
    }
    while (!__atomic_compare_exchange_n(&storage->FreeListIndex.CombinedIndex, &entryIndex.CombinedIndex, freeListEntry->Next.CombinedIndex, true, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE));

    return entryIndex;
}

template<typename TValue>
void SystemInsertFreeListEntry(SystemDictionaryStorage<TValue>* storage, SystemDictionaryEntryIndex index, SystemDictionaryEntry<TValue>* entry)
{
    SystemDictionaryEntryIndex entryIndex;
    __atomic_load(&storage->FreeListIndex.CombinedIndex, &entryIndex.CombinedIndex, __ATOMIC_ACQUIRE);

    // TODO: Change this to a while loop and do a Thread Yield
    do
    {
        entry->Next = entryIndex;
    }
    while (!__atomic_compare_exchange_n(&storage->FreeListIndex.CombinedIndex, &entryIndex.CombinedIndex, index.CombinedIndex, true, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE));
}

template<typename TValue>
void SystemAddDictionaryEntry(SystemDictionaryStorage<TValue>* storage, SystemDictionaryHashInfo hashInfo, TValue value)
{
    auto entryIndex = SystemGetFreeListEntry(storage); 
    size_t partitionIndex = *(volatile size_t*)&storage->CurrentPartitionIndex;

    if (entryIndex == SYSTEM_DICTIONARY_INDEX_EMPTY) 
    {
        auto expected = false;
        while (!__atomic_compare_exchange_n(&storage->IsPartitionBeingCreated, &expected, true, true, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE))
        {
            expected = false;
            SystemPlatformYieldThread();
        }
            
        size_t currentPartitionIndex;
        __atomic_load(&storage->CurrentPartitionIndex, &currentPartitionIndex, __ATOMIC_ACQUIRE);

        if (currentPartitionIndex == partitionIndex)
        {
            if ((storage->CurrentPartitionIndex + 1) == 2) // TODO: Remove hardcoded value
            {
                SystemLogErrorMessage(LogMessageCategory_NativeApplication, "Max items in dictionary reached, the item will not be added.");
                __atomic_store_n(&storage->IsPartitionBeingCreated, false, __ATOMIC_RELEASE);
                return;
            }

            SystemLogDebugMessage(LogMessageCategory_NativeApplication, "Dictionary partition is full, allocating a new partition.");
            storage->CurrentPartitionIndex++;
            storage->Partitions[storage->CurrentPartitionIndex] = SystemCreateDictionaryPartition<TValue>(storage->MemoryArena, storage->CurrentPartitionIndex, storage->PartitionSize);
            storage->FreeListIndex = SystemCreateDictionaryEntryIndex(storage->CurrentPartitionIndex, 1);

            entryIndex = SystemCreateDictionaryEntryIndex(storage->CurrentPartitionIndex, 0);
        }
        else 
        {
            entryIndex = SystemGetFreeListEntry(storage);
        }
            
        __atomic_store_n(&storage->IsPartitionBeingCreated, false, __ATOMIC_RELEASE);
    }
    
    auto entry = SystemGetDictionaryEntryByIndex(storage, entryIndex);

    SystemDictionaryEntryIndex bucketHead;
    __atomic_load(&storage->Buckets[hashInfo.BucketIndex].CombinedIndex, &bucketHead.CombinedIndex, __ATOMIC_ACQUIRE);

    // TODO: Convert this to a while struct and use yield?
    do
    {
        if (bucketHead != SYSTEM_DICTIONARY_INDEX_EMPTY)
        {
            auto bucketHeadEntry = SystemGetDictionaryEntryByIndex(storage, bucketHead);

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
void SystemRemoveDictionaryEntry(SystemDictionaryStorage<TValue>* storage, SystemDictionaryHashInfo hashInfo)
{
    SystemDictionaryIndexInfo entryIndex = {};
    SystemDictionaryEntry<TValue>* entry = nullptr;
    SystemDictionaryEntryIndex* parentNextEntryIndex = nullptr;

    // TODO: Convert this to a while struct and use yield?
    do
    {
        entryIndex = SystemGetDictionaryEntryIndexInfo(storage, hashInfo);

        if (entryIndex.Index == SYSTEM_DICTIONARY_INDEX_EMPTY)
        {
            printf("No result found\n");
            return;
        }

        entry = SystemGetDictionaryEntryByIndex(storage, entryIndex.Index);

        if (entryIndex.RootIndex != entryIndex.Index)
        {
            auto parentEntry = SystemGetDictionaryEntryByIndex(storage, entryIndex.ParentIndex);
            parentNextEntryIndex = &parentEntry->Next;
        }
        else 
        {
            parentNextEntryIndex = &storage->Buckets[hashInfo.BucketIndex];
        }

        SystemPlatformYieldThread();
    }
    while (!__atomic_compare_exchange_n(&parentNextEntryIndex->CombinedIndex, &entryIndex.Index.CombinedIndex, entry->Next.CombinedIndex, true, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE));

    entry->Hash = 0;
    entry->Value = {};
    SystemInsertFreeListEntry(storage, entryIndex.Index, entry);
}

template<typename TValue>
TValue* SystemGetDictionaryValue(SystemDictionaryStorage<TValue>* storage, SystemDictionaryHashInfo hashInfo)
{
    auto entryIndex = SystemGetDictionaryEntryIndexInfo(storage, hashInfo);
    auto entry = SystemGetDictionaryEntryByIndex(storage, entryIndex.Index);

    if (entry != nullptr)
    {
        return &entry->Value;
    }

    static TValue defaultValue;
    return &defaultValue;
}

template<typename TValue>
SystemDictionaryStoragePartition<TValue>* SystemCreateDictionaryPartition(MemoryArena* memoryArena, size_t partitionIndex, size_t partitionSize)
{
    auto partition = SystemPushStruct<SystemDictionaryStoragePartition<TValue>>(memoryArena);
    partition->Entries = SystemPushArrayZero<SystemDictionaryEntry<TValue>>(memoryArena, partitionSize);

    for (size_t i = 0; i < partition->Entries.Length; i++)
    {
        partition->Entries[i].Next = i < partition->Entries.Length - 1 ? SystemCreateDictionaryEntryIndex(partitionIndex, i + 1) : SYSTEM_DICTIONARY_INDEX_EMPTY;
    }

    return partition;
}

template<typename TValue, typename T>
SystemDictionaryHashInfo SystemDictionaryComputeHashInfo(SystemDictionaryStorage<TValue>* storage, ReadOnlySpan<T> data)
{
    auto hash = XXH64(data.Pointer, data.Length, SYSTEM_DICTIONARY_HASH_SEED);
    auto bucketIndex = (int32_t)(hash % storage->Buckets.Length);

    SystemDictionaryHashInfo result = {};
    result.Hash = hash;
    result.BucketIndex = bucketIndex;

    return result;
}

template<typename TValue>
SystemDictionaryIndexInfo SystemGetDictionaryEntryIndexInfo(SystemDictionaryStorage<TValue>* storage, SystemDictionaryHashInfo hashInfo)
{
    SystemDictionaryEntryIndex currentIndex;
    __atomic_load(&storage->Buckets[hashInfo.BucketIndex].CombinedIndex, &currentIndex.CombinedIndex, __ATOMIC_ACQUIRE);

    auto rootIndex = currentIndex;
    auto parentIndex = SYSTEM_DICTIONARY_INDEX_EMPTY;

    while (currentIndex != SYSTEM_DICTIONARY_INDEX_EMPTY) 
    {
        auto currentEntry = SystemGetDictionaryEntryByIndex(storage, currentIndex);

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

// TODO: Check thread safe?
template<typename TValue>
SystemDictionaryEntry<TValue>* SystemGetDictionaryEntryByIndex(SystemDictionaryStorage<TValue>* storage, SystemDictionaryEntryIndex index)
{
    if (index == SYSTEM_DICTIONARY_INDEX_EMPTY)
    {
        return nullptr;
    }

    auto indexFull = SystemGetDictionaryEntryIndexFull(index);
    return &(storage->Partitions[indexFull.PartitionIndex]->Entries[indexFull.Index]);
}
