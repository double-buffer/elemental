#pragma once

// TODO: Don't malloc items, use a unified malloc here with an array

struct DictionaryEntry
{
    uint64_t Key = UINT64_MAX;
    void* Data;
    DictionaryEntry* Next = NULL;
} typedef DictionaryEntry;

struct Dictionary
{
    DictionaryEntry** DictionaryEntries;
    size_t MaxEntries;
    size_t Count;
} typedef Dictionary;

typedef void (*DictionaryEnumerateEntry)(uint64_t key, void* entry);

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
    Dictionary* dictionary = (Dictionary*)malloc(sizeof(Dictionary));
    dictionary->MaxEntries = maxEntries;
    dictionary->Count = 0;
    dictionary->DictionaryEntries = (DictionaryEntry**)calloc(maxEntries, sizeof(DictionaryEntry*));

    return dictionary;
}

void DictionaryFree(Dictionary* dictionary)
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

void* DictionaryGetEntry(Dictionary* dictionary, uint64_t key)
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

bool DictionaryContains(Dictionary* dictionary, uint64_t key)
{
    return DictionaryGetEntry(dictionary, key) != NULL;
}

bool DictionaryAdd(Dictionary* dictionary, uint64_t key, void* data)
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

void DictionaryDelete(Dictionary* dictionary, uint64_t key)
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

void DictionaryEnumerateEntries(Dictionary* dictionary, DictionaryEnumerateEntry enumerateFunction)
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

void DictionaryPrint(Dictionary* dictionary)
{
    if (dictionary == NULL)
    {
        return;
    }

    for (size_t i = 0; i < dictionary->MaxEntries; i++)
    {
        if (dictionary->DictionaryEntries[i] != NULL)
        {
            printf("Dictionary %zu: %llu", i, dictionary->DictionaryEntries[i]->Key);

            DictionaryEntry* nextEntry = dictionary->DictionaryEntries[i]->Next;

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