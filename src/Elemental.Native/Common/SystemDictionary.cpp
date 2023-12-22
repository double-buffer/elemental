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
    int32_t CurrentEntryIndex;
};

template<typename TValue> 
struct SystemDictionaryStorage
{
    MemoryArena* MemoryArena;
    Span<SystemDictionaryEntryIndex> Buckets;
    Span<SystemDictionaryStoragePartition<TValue>*> Partitions;
    int32_t CurrentPartitionIndex;
    SystemDictionaryEntryIndex FreeListIndex;
};

struct SystemDictionaryIndexInfo
{
    int32_t BucketIndex;
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
SystemDictionaryStoragePartition<TValue>* SystemCreateDictionaryPartition(MemoryArena* memoryArena, size_t partitionSize);

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
    storage->Partitions[storage->CurrentPartitionIndex] = SystemCreateDictionaryPartition<TValue>(memoryArena, partitionSize); // TODO: Find a proper initial size
    
    storage->FreeListIndex = SYSTEM_DICTIONARY_INDEX_EMPTY;
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

        printf("Partition %zu (Size: %zu): ", p, partition->Entries.Length);

        if (partition != nullptr)
        {
            printf("%d\n", partition->CurrentEntryIndex);

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
void SystemAddDictionaryEntry(SystemDictionaryStorage<TValue>* storage, SystemDictionaryHashInfo hashInfo, TValue value)
{
    auto partition = storage->Partitions[storage->CurrentPartitionIndex];
    auto entryIndex = SYSTEM_DICTIONARY_INDEX_EMPTY;
    SystemDictionaryEntry<TValue>* entry;

    if (storage->FreeListIndex != SYSTEM_DICTIONARY_INDEX_EMPTY)
    {
        entryIndex = storage->FreeListIndex;
        entry = SystemGetDictionaryEntryByIndex(storage, entryIndex);
        storage->FreeListIndex = entry->Next;
    }
    else 
    {
        auto newPartitionIndex = storage->CurrentPartitionIndex;
        auto newIndex = partition->CurrentEntryIndex;

        if (newIndex == (int32_t)partition->Entries.Length)
        {
            storage->CurrentPartitionIndex++;

            if (storage->CurrentPartitionIndex == 2) // TODO: Remove hardcoded value
            {
                SystemLogErrorMessage(LogMessageCategory_NativeApplication, "Max items in dictionary reached, the item will not be added.");
                return;
            }

            SystemLogDebugMessage(LogMessageCategory_NativeApplication, "Dictionary partition is full, allocating a new partition.");
            partition = SystemCreateDictionaryPartition<TValue>(storage->MemoryArena, partition->Entries.Length);
            storage->Partitions[storage->CurrentPartitionIndex] = partition;

            newPartitionIndex = storage->CurrentPartitionIndex;
            newIndex = partition->CurrentEntryIndex;
        }

        entryIndex = SystemCreateDictionaryEntryIndex(newPartitionIndex, newIndex);
        entry = SystemGetDictionaryEntryByIndex(storage, entryIndex);

        partition->CurrentEntryIndex++;
    }

    if (storage->Buckets[hashInfo.BucketIndex] == SYSTEM_DICTIONARY_INDEX_EMPTY)
    {
        storage->Buckets[hashInfo.BucketIndex] = entryIndex;
    }
    else
    {
        auto indexStorage = storage->Buckets[hashInfo.BucketIndex];
        auto indexEntry = SystemGetDictionaryEntryByIndex(storage, indexStorage);

        while (indexEntry->Next != SYSTEM_DICTIONARY_INDEX_EMPTY)
        {
            indexStorage = indexEntry->Next;
            indexEntry = SystemGetDictionaryEntryByIndex(storage, indexStorage);
        }

        indexEntry->Next = entryIndex;
    }

    entry->Hash = hashInfo.Hash;
    entry->Value = value;
    entry->Next = SYSTEM_DICTIONARY_INDEX_EMPTY;
}

template<typename TValue>
void SystemRemoveDictionaryEntry(SystemDictionaryStorage<TValue>* storage, SystemDictionaryHashInfo hashInfo)
{
    auto entryIndex = SystemGetDictionaryEntryIndexInfo(storage, hashInfo);

    if (entryIndex.Index != SYSTEM_DICTIONARY_INDEX_EMPTY)
    {
        auto entry = SystemGetDictionaryEntryByIndex(storage, entryIndex.Index);
        auto nextEntry = entry->Next;

        if (entryIndex.ParentIndex != SYSTEM_DICTIONARY_INDEX_EMPTY)
        {
            auto parentEntry = SystemGetDictionaryEntryByIndex(storage, entryIndex.ParentIndex);
            parentEntry->Next = nextEntry;
        }
        else 
        {
            storage->Buckets[entryIndex.BucketIndex] = SYSTEM_DICTIONARY_INDEX_EMPTY;
        }

        entry->Hash = 0;
        entry->Next = storage->FreeListIndex;
        storage->FreeListIndex = entryIndex.Index;
    }
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
SystemDictionaryStoragePartition<TValue>* SystemCreateDictionaryPartition(MemoryArena* memoryArena, size_t partitionSize)
{
    auto partition = SystemPushStruct<SystemDictionaryStoragePartition<TValue>>(memoryArena);
    partition->Entries = SystemPushArrayZero<SystemDictionaryEntry<TValue>>(memoryArena, partitionSize);
    partition->CurrentEntryIndex = 0;

    for (size_t i = 0; i < partition->Entries.Length; i++)
    {
        partition->Entries[i].Next = SYSTEM_DICTIONARY_INDEX_EMPTY;
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
    auto parentIndex = SYSTEM_DICTIONARY_INDEX_EMPTY;
    auto currentIndex = storage->Buckets[hashInfo.BucketIndex];

    while (currentIndex != SYSTEM_DICTIONARY_INDEX_EMPTY) 
    {
        auto currentEntry = SystemGetDictionaryEntryByIndex(storage, currentIndex);

        if (currentEntry->Hash == hashInfo.Hash)
        {
            SystemDictionaryIndexInfo result = {};
            result.BucketIndex = hashInfo.BucketIndex;
            result.ParentIndex = parentIndex;
            result.Index = currentIndex;

            return result;
        }

        parentIndex = currentIndex;
        currentIndex = currentEntry->Next;
    }

    return { -1, SYSTEM_DICTIONARY_INDEX_EMPTY, SYSTEM_DICTIONARY_INDEX_EMPTY };
}

template<typename TValue>
SystemDictionaryIndexInfo SystemGetDictionaryEntryIndexInfo(SystemDictionaryStorage<TValue>* storage, ReadOnlySpan<char> key)
{
    auto hashInfo = SystemDictionaryComputeHashInfo(storage, key); 

    auto parentIndex = SYSTEM_DICTIONARY_INDEX_EMPTY;
    auto currentIndex = storage->Buckets[hashInfo.BucketIndex];

    while (currentIndex != SYSTEM_DICTIONARY_INDEX_EMPTY) 
    {
        auto currentEntry = SystemGetDictionaryEntryByIndex(storage, currentIndex);

        if (currentEntry->Hash == hashInfo.Hash)
        {
            SystemDictionaryIndexInfo result = {};
            result.BucketIndex = hashInfo.BucketIndex;
            result.ParentIndex = parentIndex;
            result.Index = currentIndex;

            return result;
        }

        parentIndex = currentIndex;
        currentIndex = currentEntry->Next;
    }

    return { -1, SYSTEM_DICTIONARY_INDEX_EMPTY, SYSTEM_DICTIONARY_INDEX_EMPTY };
}

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
