
// https://github.com/EpicGames/UnrealEngine/blob/5ca9da84c694c6eee288c30a547fcaa1a40aed9b/Engine/Source/Runtime/Core/Public/Containers/Map.h

// TODO: Basic implementation with std:map first

#pragma once

struct DictionaryEntry
{
    uint64_t Key = UINT64_MAX;
    void* Data;
    DictionaryEntry* Next = NULL;
} typedef DictionaryEntry;

struct Dictionary
{
    struct DictionaryEntry* DictionaryEntries;
    size_t MaxEntries;
    size_t Count;
} typedef Dictionary;

uint64_t HashFunction(uint64_t key)
{
  key ^= (key >> 33);
  key *= 0xff51afd7ed558ccd;
  key ^= (key >> 33);
  key *= 0xc4ceb9fe1a85ec53;
  key ^= (key >> 33);
  return key;
}

Dictionary* DictionaryCreate(size_t maxEntries)
{
    size_t arraySizeInBytes = maxEntries * sizeof(DictionaryEntry);

    Dictionary* dictionary = (Dictionary*)malloc(sizeof(Dictionary));
    dictionary->DictionaryEntries = (DictionaryEntry*)malloc(arraySizeInBytes);
    dictionary->MaxEntries = maxEntries;
    dictionary->Count = 0;

    ZeroMemory(dictionary->DictionaryEntries, arraySizeInBytes);

    return dictionary;
}

void DictionaryFree(Dictionary* dictionary)
{
    // TODO
}

void DictionaryAdd(Dictionary* dictionary, uint64_t key, void* data)
{
    if (dictionary == NULL)
    {
        return;
    }

    uint64_t hash = HashFunction(key) % dictionary->MaxEntries;
    dictionary->Count++;

    if (dictionary->DictionaryEntries[hash].Data == NULL)
    {
        dictionary->DictionaryEntries[hash].Key = key;
        dictionary->DictionaryEntries[hash].Data = data;
    }
    else
    {
        DictionaryEntry* entry = &dictionary->DictionaryEntries[hash];

        while (entry->Next != NULL)
        {
            entry = entry->Next;
        }

        DictionaryEntry* nextEntry = (DictionaryEntry*)malloc(sizeof(DictionaryEntry));
        nextEntry->Key = key;
        nextEntry->Data = data;
        nextEntry->Next = NULL;

        entry->Next = nextEntry;
    }
}

void DictionaryDelete(Dictionary* dictionary, uint64_t key)
{
    if (dictionary == NULL)
    {
        return;
    }

    uint64_t hash = HashFunction(key) % dictionary->MaxEntries;
    dictionary->Count--;

    if (dictionary->DictionaryEntries[hash].Data != NULL)
    {
        DictionaryEntry* entry = &dictionary->DictionaryEntries[hash];

        if (entry->Key == key)
        {
            if (entry->Next != NULL)
            {
                DictionaryEntry* nextEntry = entry->Next;
                dictionary->DictionaryEntries[hash] = *entry->Next;
                free(nextEntry);
            }
            else
            {
                dictionary->DictionaryEntries[hash].Key = 0;
                dictionary->DictionaryEntries[hash].Data = NULL;
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

bool DictionaryContains(Dictionary* dictionary, uint64_t key)
{
    if (dictionary == NULL)
    {
        return false;
    }

    uint64_t hash = HashFunction(key) % dictionary->MaxEntries;

    if (dictionary->DictionaryEntries[hash].Data != NULL)
    {
        DictionaryEntry* entry = &dictionary->DictionaryEntries[hash];

        if (entry->Key == key)
        {
            return true;
        }

        while (entry->Next != NULL)
        {
            entry = entry->Next;

            if (entry->Key == key)
            {
                return true;
            }
        }
    }

    return false;
}

void DictionaryPrint(Dictionary* dictionary)
{
    if (dictionary == NULL)
    {
        return;
    }

    for (size_t i = 0; i < dictionary->MaxEntries; i++)
    {
        if (dictionary->DictionaryEntries[i].Data != NULL)
        {
            printf("Dictionary %zu: %llu", i, dictionary->DictionaryEntries[i].Key);

            DictionaryEntry* nextEntry = dictionary->DictionaryEntries[i].Next;

            while (nextEntry != NULL)
            {
                printf(" -> %llu", nextEntry->Key);
                nextEntry = nextEntry->Next;
            }

            printf("\n");
        }
        else
        {
            printf("Dictionary %zu: ---\n", i);
        }
    }
}

// HACK: Temporary
#include <map>

template<typename TKey, typename TValue>
class DictionaryOld
{
public:
    DictionaryOld()
    {
    }

    ~DictionaryOld()
    {
    }

    bool ContainsKey(TKey key)
    {
        return _map.count(key) != 0;
    }

    void Add(TKey key, TValue value)
    {
        _map[key] = value;
    }

    TValue& operator[](TKey key)
    {
        return _map[key];
    }

    TValue& operator[](int32_t index)
    {
        auto iterator = _map.begin();
        std::advance(iterator, index);
        return iterator->second;
    }

    uint32_t Count()
    {
        return (uint32_t)_map.size();
    }

private:
    std::map<TKey, TValue> _map;
};