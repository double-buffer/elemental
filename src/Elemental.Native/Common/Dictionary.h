#pragma once

#include <stdbool.h>
#include <stdint.h>

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
