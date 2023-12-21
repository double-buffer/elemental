#pragma once

#include "SystemMemory.h"

template<typename TValue>
struct SystemDictionaryStorage;

template<typename TKey, typename TValue>
struct SystemDictionary
{
    SystemDictionaryStorage<TValue>* Storage;
    
    TValue& operator[](TKey key);
};

template<typename TKey, typename TValue>
SystemDictionary<TKey, TValue> SystemCreateDictionary(MemoryArena* memoryArena, size_t maxItemsCount);

template<typename TKey, typename TValue>
void SystemAddDictionaryEntry(SystemDictionary<TKey, TValue> dictionary, TKey key, TValue value);

template<typename TValue>
void SystemAddDictionaryEntry(SystemDictionary<ReadOnlySpan<char>, TValue> dictionary, ReadOnlySpan<char> key, TValue value);

template<typename TValue, typename T>
void SystemAddDictionaryEntry(SystemDictionary<ReadOnlySpan<T>, TValue> dictionary, ReadOnlySpan<T> key, TValue value);

template<typename TKey, typename TValue>
void SystemRemoveDictionaryEntry(SystemDictionary<TKey, TValue> dictionary, TKey key);

template<typename TValue>
void SystemRemoveDictionaryEntry(SystemDictionary<ReadOnlySpan<char>, TValue> dictionary, ReadOnlySpan<char> key);

template<typename TValue, typename T>
void SystemRemoveDictionaryEntry(SystemDictionary<ReadOnlySpan<T>, TValue> dictionary, ReadOnlySpan<T> key);

template<typename TKey, typename TValue>
TValue* SystemGetDictionaryValue(SystemDictionary<TKey, TValue> dictionary, TKey key);

template<typename TValue>
TValue* SystemGetDictionaryValue(SystemDictionary<ReadOnlySpan<char>, TValue> dictionary, ReadOnlySpan<char> key);

template<typename TValue, typename T>
TValue* SystemGetDictionaryValue(SystemDictionary<ReadOnlySpan<T>, TValue> dictionary, ReadOnlySpan<T> key);

// TODO: ContainsKey

// TODO: Enumerate?

template<typename TKey, typename TValue>
void DebugDictionary(SystemDictionary<TKey, TValue> dictionary);




















// TODO: Don't malloc items, use a unified malloc here with an array

typedef struct _DictionaryEntry
{
    uint64_t Key;
    void* Data;
    struct _DictionaryEntry* Next;
} DictionaryEntry;

typedef struct
{
    DictionaryEntry** DictionaryEntries;
    size_t MaxEntries;
    size_t Count;
} DictionaryStruct;

typedef void (*DictionaryEnumerateEntry)(uint64_t key, void* entry);

// TODO: Use byte array instead
uint64_t HashFunction(uint64_t key);
DictionaryStruct* DictionaryCreate(size_t maxEntries);
void DictionaryFree(DictionaryStruct* dictionary);
void* DictionaryGetEntry(DictionaryStruct* dictionary, uint64_t key);
bool DictionaryContains(DictionaryStruct* dictionary, uint64_t key);
bool DictionaryAdd(DictionaryStruct* dictionary, uint64_t key, void* data);
void DictionaryDelete(DictionaryStruct* dictionary, uint64_t key);
void DictionaryEnumerateEntries(DictionaryStruct* dictionary, DictionaryEnumerateEntry enumerateFunction);
void DictionaryPrint(DictionaryStruct* dictionary);

template<typename TKey, typename TValue>
class Dictionary
{
public:
    Dictionary()
    {
        _dictionaryStruct = DictionaryCreate(64);
    }

    ~Dictionary()
    {
        DictionaryFree(_dictionaryStruct);
    }

    bool ContainsKey(TKey key)
    {
        return DictionaryContains(_dictionaryStruct, key);
    }

    void Add(TKey key, TValue* value)
    {
        DictionaryAdd(_dictionaryStruct, key, value);
    }

    TValue* operator[](TKey key)
    {
        return (TValue*)DictionaryGetEntry(_dictionaryStruct, key);
    }

    uint32_t Count()
    {
        return _dictionaryStruct->Count;
    }

    void EnumerateEntries(DictionaryEnumerateEntry enumerateFunction)
    {
        DictionaryEnumerateEntries(_dictionaryStruct, enumerateFunction);
    }

private:
    DictionaryStruct* _dictionaryStruct;
};
