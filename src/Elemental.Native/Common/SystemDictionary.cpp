#include "SystemDictionary.h"
#include "SystemLogging.h"

#define DICTIONARY_HASH_SEED 123456789

struct SystemDictionaryEntryIndex
{
    // TODO: Remove constructors
    SystemDictionaryEntryIndex()
    {
        PartitionIndex = -1;
        Index = -1;
    }
    SystemDictionaryEntryIndex(int32_t index)
    {
        PartitionIndex = 0;
        Index = index;
    }

    int32_t PartitionIndex;
    int32_t Index;
};

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

// TODO: Change the signature to take the max items count
template<typename TValue>
SystemDictionaryStoragePartition<TValue>* SystemCreateDictionaryPartition(MemoryArena* memoryArena, size_t partitionSize);

template<typename TKey, typename TValue, typename T>
SystemDictionaryHashInfo SystemDictionaryComputeHashInfo(SystemDictionary<TKey, TValue> dictionary, ReadOnlySpan<T> data);

template<typename TKey, typename TValue>
SystemDictionaryIndexInfo SystemGetDictionaryEntryIndexInfo(SystemDictionary<TKey, TValue> dictionary, TKey key);

template<typename TKey, typename TValue>
SystemDictionaryEntry<TValue>* SystemGetDictionaryEntryByIndex(SystemDictionary<TKey, TValue> dictionary, SystemDictionaryEntryIndex index);

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

    printf("Partition Size: %d\n", partitionSize);

    auto storage = SystemPushStruct<SystemDictionaryStorage<TValue>>(memoryArena);
    storage->MemoryArena = memoryArena;
    storage->Partitions = SystemPushArrayZero<SystemDictionaryStoragePartition<TValue>*>(memoryArena, maxPartitionsCount);
    storage->CurrentPartitionIndex = 0;
    storage->Partitions[storage->CurrentPartitionIndex++] = SystemCreateDictionaryPartition<TValue>(memoryArena, partitionSize); // TODO: Find a proper initial size
    
    storage->FreeListIndex = -1;
    storage->Buckets = SystemPushArrayZero<SystemDictionaryEntryIndex>(memoryArena, maxItemsCount);

    for (size_t i = 0; i < storage->Buckets.Length; i++)
    {
        storage->Buckets[i] = -1;
    }

    return { storage };
}

// TODO: What will happens in case we need to grow the storage?
// How the old storage will be reused in the memory arena?
template<typename TKey, typename TValue>
void SystemAddDictionaryEntry(SystemDictionary<TKey, TValue> dictionary, TKey key, TValue value)
{
    auto hashInfo = SystemDictionaryComputeHashInfo(ReadOnlySpan<uint8_t>(&key, sizeof(key)));
}

template<typename TValue>
void SystemAddDictionaryEntry(SystemDictionary<ReadOnlySpan<char>, TValue> dictionary, ReadOnlySpan<char> key, TValue value)
{
    auto hashInfo = SystemDictionaryComputeHashInfo(dictionary, key);
    auto storage = dictionary.Storage;
    auto partition = dictionary.Storage->Partitions[storage->CurrentPartitionIndex - 1];
    SystemDictionaryEntryIndex entryIndex = -1;
    SystemDictionaryEntry<TValue>* entry;

    if (storage->FreeListIndex.Index != -1)
    {
        entryIndex = storage->FreeListIndex;
        entry = SystemGetDictionaryEntryByIndex(dictionary, entryIndex);
        storage->FreeListIndex = entry->Next;
    }
    else 
    {
        entryIndex.PartitionIndex = storage->CurrentPartitionIndex - 1;
        entryIndex.Index = partition->CurrentEntryIndex++;
        entry = SystemGetDictionaryEntryByIndex(dictionary, entryIndex);
    }

    if (entryIndex.Index == (int32_t)partition->Entries.Length)
    {
        SystemLogDebugMessage(LogMessageCategory_NativeApplication, "Dictionary is full");
        partition = SystemCreateDictionaryPartition<TValue>(storage->MemoryArena, 10); // TODO: Find a proper initial size
        storage->Partitions[storage->CurrentPartitionIndex++] = partition;

        entryIndex.PartitionIndex = storage->CurrentPartitionIndex - 1;
        entryIndex.Index = partition->CurrentEntryIndex++;
        entry = SystemGetDictionaryEntryByIndex(dictionary, entryIndex);
    }

    // TODO: Retrieve the buckets with a function
    if (storage->Buckets[hashInfo.BucketIndex].Index == -1)
    {
        storage->Buckets[hashInfo.BucketIndex] = entryIndex;
    }
    else
    {
        auto indexStorage = storage->Buckets[hashInfo.BucketIndex];
        auto indexEntry = SystemGetDictionaryEntryByIndex(dictionary, indexStorage);

        while (indexEntry->Next.Index != -1)
        {
            indexStorage = indexEntry->Next;
            indexEntry = SystemGetDictionaryEntryByIndex(dictionary, indexStorage);
        }

        indexEntry->Next = entryIndex;
    }

    entry->Hash = hashInfo.Hash;
    entry->Value = value;
    entry->Next = -1;
}

template<typename TValue>
void SystemAddDictionaryEntry(SystemDictionary<ReadOnlySpan<uint8_t>, TValue> dictionary, ReadOnlySpan<uint8_t> key, TValue value)
{
}

template<typename TKey, typename TValue>
void SystemRemoveDictionaryEntry(SystemDictionary<TKey, TValue> dictionary, TKey key)
{

}

template<typename TValue>
void SystemRemoveDictionaryEntry(SystemDictionary<ReadOnlySpan<char>, TValue> dictionary, ReadOnlySpan<char> key)
{
    auto entryIndex = SystemGetDictionaryEntryIndexInfo(dictionary, key);
    auto storage = dictionary.Storage;

    if (entryIndex.Index.Index != -1)
    {
        auto entry = SystemGetDictionaryEntryByIndex(dictionary, entryIndex.Index);
        auto nextEntry = entry->Next;

        if (entryIndex.ParentIndex.Index != -1)
        {
            auto parentEntry = SystemGetDictionaryEntryByIndex(dictionary, entryIndex.ParentIndex);
            parentEntry->Next = nextEntry;
        }
        else 
        {
            storage->Buckets[entryIndex.BucketIndex] = -1;
        }

        entry->Hash = 0;
        entry->Next = storage->FreeListIndex;
        storage->FreeListIndex = entryIndex.Index;
    }
}

template<typename TKey, typename TValue>
TValue* SystemGetDictionaryValue(SystemDictionary<TKey, TValue> dictionary, TKey key)
{
    auto entryIndex = SystemGetDictionaryEntryIndexInfo(dictionary, key);
    auto entry = SystemGetDictionaryEntryByIndex(dictionary, entryIndex.Index);

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
        partition->Entries[i].Next = -1;
    }

    return partition;
}

template<typename TKey, typename TValue, typename T>
SystemDictionaryHashInfo SystemDictionaryComputeHashInfo(SystemDictionary<TKey, TValue> dictionary, ReadOnlySpan<T> data)
{
    auto hash = XXH64(data.Pointer, data.Length, DICTIONARY_HASH_SEED);
    auto bucketIndex = (int32_t)(hash % dictionary.Storage->Buckets.Length);

    SystemDictionaryHashInfo result = {};
    result.Hash = hash;
    result.BucketIndex = bucketIndex;

    return result;
}

template<typename TKey, typename TValue>
SystemDictionaryIndexInfo SystemGetDictionaryEntryIndexInfo(SystemDictionary<TKey, TValue> dictionary, TKey key)
{
    auto hashInfo = SystemDictionaryComputeHashInfo(dictionary, key); 
    auto storage = dictionary.Storage;

    SystemDictionaryEntryIndex parentIndex = -1;
    auto currentIndex = storage->Buckets[hashInfo.BucketIndex];

    while (currentIndex.Index != -1) 
    {
        auto currentEntry = SystemGetDictionaryEntryByIndex(dictionary, currentIndex);

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

    return { -1, -1, -1 };
}

template<typename TKey, typename TValue>
SystemDictionaryEntry<TValue>* SystemGetDictionaryEntryByIndex(SystemDictionary<TKey, TValue> dictionary, SystemDictionaryEntryIndex index)
{
    if (index.Index == -1)
    {
        return nullptr;
    }

    return &(dictionary.Storage->Partitions[index.PartitionIndex]->Entries[index.Index]);
}

template<typename TKey, typename TValue>
void DebugDictionary(SystemDictionary<TKey, TValue> dictionary)
{
    auto storage = dictionary.Storage;

    for (int32_t i = 0; i < storage->Buckets.Length; i++)
    {
        if (storage->Buckets[i].Index == -1)
        {
            printf("Bucket %d => (EMPTY)\n");
        }
        else 
    {
            printf("Bucket %d", i);

            auto entryIndex = storage->Buckets[i]; 

            while (entryIndex.Index != -1)
            {
                auto entry = storage->Partitions[entryIndex.PartitionIndex]->Entries[entryIndex.Index]; 
                printf(" => %lu (Value: %d, Partition: %d, Index: %d)", entry.Hash, entry.Value, entryIndex.PartitionIndex, entryIndex.Index);

                entryIndex = entry.Next;
            }

            printf("\n");
        }
    }

    auto currentFreeListIndex = storage->FreeListIndex;
    printf("FreeList");

    while (currentFreeListIndex.Index != -1)
    {
        printf(" => %d", currentFreeListIndex);
        currentFreeListIndex = storage->Partitions[currentFreeListIndex.PartitionIndex]->Entries[currentFreeListIndex.Index].Next;
    }

    printf("\n");


    for (int32_t p = 0; p < dictionary.Storage->Partitions.Length; p++)
    {
        printf("Partition %d: ", p);
        auto partition = dictionary.Storage->Partitions[p];

        if (partition != nullptr)
        {
            printf("%d\n", partition->CurrentEntryIndex);

            for (int32_t j = 0; j < partition->Entries.Length; j++)
            {
                printf("%lu =>", partition->Entries[j].Hash);
            }

            printf("\n");
        }
        else 
        {
            printf("(EMPTY)\n"); 
        }
    }
}





// TODO: Don't malloc items, use a unified malloc here with an array

// TODO: Use byte array instead
uint64_t HashFunction(uint64_t key)
{
  key ^= (key >> 33);
  key *= 0xff51afd7ed558ccd;
  key ^= (key >> 33);
  key *= 0xc4ceb9fe1a85ec53;
  key ^= (key >> 33);
  return key;
}

DictionaryStruct* DictionaryCreate(size_t maxEntries)
{
    DictionaryStruct* dictionary = (DictionaryStruct*)malloc(sizeof(DictionaryStruct));
    dictionary->MaxEntries = maxEntries;
    dictionary->Count = 0;
    dictionary->DictionaryEntries = (DictionaryEntry**)calloc(maxEntries, sizeof(DictionaryEntry*));

    return dictionary;
}

void DictionaryFree(DictionaryStruct* dictionary)
{
    if (dictionary == NULL)
    {
        return;
    }

    for (size_t i = 0; i < dictionary->MaxEntries; i++)
    {
        DictionaryEntry* entry = dictionary->DictionaryEntries[i];

        while (entry != NULL)
        {
            DictionaryEntry* next = entry->Next;
            free(entry);

            entry = next;
        }
    }

    free(dictionary->DictionaryEntries);
    free(dictionary);
}

void* DictionaryGetEntry(DictionaryStruct* dictionary, uint64_t key)
{
    if (dictionary == NULL || dictionary->Count == 0)
    {
        return NULL;
    }

    uint64_t index = HashFunction(key) % dictionary->MaxEntries;

    if (dictionary->DictionaryEntries[index] != NULL)
    {
        DictionaryEntry* entry = dictionary->DictionaryEntries[index];

        while (entry != NULL)
        {
            if (entry->Key == key)
            {
                return entry->Data;
            }

            entry = entry->Next;
        }
    }

    return NULL;
}

bool DictionaryContains(DictionaryStruct* dictionary, uint64_t key)
{
    return DictionaryGetEntry(dictionary, key) != NULL;
}

bool DictionaryAdd(DictionaryStruct* dictionary, uint64_t key, void* data)
{
    if (dictionary == NULL || data == NULL || DictionaryContains(dictionary, key))
    {
        return false;
    }

    uint64_t index = HashFunction(key) % dictionary->MaxEntries;
    dictionary->Count++;
    
    DictionaryEntry* nextEntry = (DictionaryEntry*)malloc(sizeof(DictionaryEntry));
    nextEntry->Key = key;
    nextEntry->Data = data;
    nextEntry->Next = NULL;

    if (dictionary->DictionaryEntries[index] != NULL)
    {
        nextEntry->Next = dictionary->DictionaryEntries[index];
    }

    dictionary->DictionaryEntries[index] = nextEntry;

    return true;
}

void DictionaryDelete(DictionaryStruct* dictionary, uint64_t key)
{
    if (dictionary == NULL)
    {
        return;
    }

    uint64_t index = HashFunction(key) % dictionary->MaxEntries;
    dictionary->Count--;

    if (dictionary->DictionaryEntries[index] != NULL)
    {
        DictionaryEntry* entry = dictionary->DictionaryEntries[index];

        if (entry->Key == key)
        {
            if (entry->Next != NULL)
            {
                DictionaryEntry* nextEntry = entry->Next;
                free(dictionary->DictionaryEntries[index]);
                dictionary->DictionaryEntries[index] = nextEntry;
            }
            else
            {
                // TODO: MEMORY LEAK
                free(entry);
                dictionary->DictionaryEntries[index] = NULL;
            }

            return;
        }

        while (entry->Next != NULL)
        {
            if (entry->Next->Key == key)
            {
                DictionaryEntry* nextEntry = entry->Next->Next;
                free(entry->Next);
                entry->Next = nextEntry;
                return;
            }

            entry = entry->Next;
        }
    }
}

void DictionaryEnumerateEntries(DictionaryStruct* dictionary, DictionaryEnumerateEntry enumerateFunction)
{
    if (dictionary == NULL)
    {
        return;
    }

    for (size_t i = 0; i < dictionary->MaxEntries; i++)
    {
        if (dictionary->DictionaryEntries[i] != NULL)
        {
            enumerateFunction(dictionary->DictionaryEntries[i]->Key, dictionary->DictionaryEntries[i]->Data);
            DictionaryEntry* nextEntry = dictionary->DictionaryEntries[i]->Next;

            while (nextEntry != NULL)
            {
                enumerateFunction(nextEntry->Key, nextEntry->Data);
                nextEntry = nextEntry->Next;
            }
        }
    }
}

void DictionaryPrint(DictionaryStruct* dictionary)
{
    if (dictionary == NULL)
    {
        return;
    }

    for (size_t i = 0; i < dictionary->MaxEntries; i++)
    {
        if (dictionary->DictionaryEntries[i] != NULL)
        {
            //printf("Dictionary %zu: %llu", i, dictionary->DictionaryEntries[i]->Key);

            DictionaryEntry* nextEntry = dictionary->DictionaryEntries[i]->Next;

            while (nextEntry != NULL)
            {
                //printf(" -> %llu", nextEntry->Key);
                nextEntry = nextEntry->Next;
            }

            //printf("\n");
        }
        else
        {
            //printf("Dictionary %zu: ---\n", i);
        }
    }
}
