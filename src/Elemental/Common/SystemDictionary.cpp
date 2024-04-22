#include "SystemDictionary.h"
#include "SystemFunctions.h"

#ifdef ElemAPI
#include "SystemLogging.h"
#endif

#define SYSTEM_DICTIONARY_HASH_SEED 123456789

// TODO: Convert the indexes to uint32_t and use maxsize for empty
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
SystemDictionaryIndexInfo GetDictionaryEntryIndexInfo(SystemDictionaryStorage<TValue>* storage, SystemDictionaryHashInfo hashInfo)
{
    int32_t currentIndex;
    SystemAtomicLoad(storage->Buckets[hashInfo.BucketIndex], currentIndex);

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
        SystemAtomicLoad(currentEntry->Next, currentIndex);
    }

    SystemDictionaryIndexInfo result = {};
    result.BucketIndex = SYSTEM_DICTIONARY_INDEX_EMPTY;
    result.RootIndex = SYSTEM_DICTIONARY_INDEX_EMPTY;
    result.ParentIndex = SYSTEM_DICTIONARY_INDEX_EMPTY;
    result.Index = SYSTEM_DICTIONARY_INDEX_EMPTY;

    return result;
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
    SystemAtomicLoad(storage->FreeListIndex, entryIndex);

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
    while (!SystemAtomicCompareExchange(storage->FreeListIndex, entryIndex, freeListEntry->Next));

    return entryIndex;
}

template<typename TValue>
void InsertFreeListEntry(SystemDictionaryStorage<TValue>* storage, int32_t index, SystemDictionaryEntry<TValue>* entry)
{
    int32_t entryIndex;
    SystemAtomicLoad(storage->FreeListIndex, entryIndex);

    entry->Next = entryIndex;

    while (!SystemAtomicCompareExchange(storage->FreeListIndex, entryIndex, index))
    {
        entry->Next = entryIndex;
        SystemYieldThread();
    }
}

template<typename TValue>
void AddDictionaryEntry(SystemDictionaryStorage<TValue>* storage, SystemDictionaryHashInfo hashInfo, TValue value)
{
    auto entryIndex = GetFreeListEntry(storage); 

    if (entryIndex == SYSTEM_DICTIONARY_INDEX_EMPTY) 
    {
        entryIndex = SystemAtomicAdd(storage->CurrentEntryIndex, 1);

        if (entryIndex == (int32_t)storage->Entries.Length)
        {
            #ifdef ElemAPI
            SystemLogErrorMessage(ElemLogMessageCategory_NativeApplication, "Max items in dictionary reached, the item will not be added.");
            #endif
            return;
        }
                
        SystemCommitMemory<SystemDictionaryEntry<TValue>>(storage->MemoryArena, storage->Entries.Slice(entryIndex, 1), true);
    }
    
    auto entry = GetDictionaryEntryByIndex(storage, entryIndex);

    int32_t bucketHead;
    SystemAtomicLoad(storage->Buckets[hashInfo.BucketIndex], bucketHead);

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
    while (!SystemAtomicCompareExchange(storage->Buckets[hashInfo.BucketIndex], bucketHead, entryIndex));

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
                #ifdef ElemAPI
                SystemLogDebugMessage(ElemLogMessageCategory_NativeApplication, "Retrying to find the item to delete.");
                #endif
                SystemYieldThread();
                retryCount++;
                entry = nullptr;
                continue;
            }

            #ifdef ElemAPI
            SystemLogErrorMessage(ElemLogMessageCategory_NativeApplication, "No entry found to delete.");
            #endif
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
    while (entry == nullptr || !SystemAtomicCompareExchange(*parentNextEntryIndex, entryIndex.Index, entry->Next));

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
            #ifdef ElemAPI
            SystemLogDebugMessage(ElemLogMessageCategory_NativeApplication, "Bucket %u => (EMPTY)", i);
            #endif
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

            #ifdef ElemAPI
            SystemLogDebugMessage(ElemLogMessageCategory_NativeApplication, "%s", debugMessage.Pointer);
            #endif
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

    #ifdef ElemAPI
    SystemLogDebugMessage(ElemLogMessageCategory_NativeApplication, "%s", debugMessage.Pointer);
    #endif
}
