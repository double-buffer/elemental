#pragma once

#include <stdbool.h>
#include <stdint.h>

// TODO: Don't malloc items, use a unified malloc here with an array

struct DictionaryEntry
{
    uint64_t Key;
    void* Data;
    struct DictionaryEntry* Next;
} typedef DictionaryEntry;

struct DictionaryStruct
{
    struct DictionaryEntry** DictionaryEntries;
    size_t MaxEntries;
    size_t Count;
} typedef DictionaryStruct;

typedef void (*DictionaryEnumerateEntry)(uint64_t key, void* entry);

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

struct DictionaryStruct* DictionaryCreate(size_t maxEntries)
{
    struct DictionaryStruct* dictionary = (struct DictionaryStruct*)malloc(sizeof(struct DictionaryStruct));
    dictionary->MaxEntries = maxEntries;
    dictionary->Count = 0;
    dictionary->DictionaryEntries = (struct DictionaryEntry**)calloc(maxEntries, sizeof(struct DictionaryEntry*));

    return dictionary;
}

void DictionaryFree(struct DictionaryStruct* dictionary)
{
    if (dictionary == NULL)
    {
        return;
    }

    for (size_t i = 0; i < dictionary->MaxEntries; i++)
    {
        struct DictionaryEntry* entry = dictionary->DictionaryEntries[i];

        while (entry != NULL)
        {
            struct DictionaryEntry* next = entry->Next;
            free(entry);

            entry = next;
        }
    }

    free(dictionary->DictionaryEntries);
    free(dictionary);
}

void* DictionaryGetEntry(struct DictionaryStruct* dictionary, uint64_t key)
{
    if (dictionary == NULL || dictionary->Count == 0)
    {
        return NULL;
    }

    uint64_t index = HashFunction(key) % dictionary->MaxEntries;

    if (dictionary->DictionaryEntries[index] != NULL)
    {
        struct DictionaryEntry* entry = dictionary->DictionaryEntries[index];

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

bool DictionaryContains(struct DictionaryStruct* dictionary, uint64_t key)
{
    return DictionaryGetEntry(dictionary, key) != NULL;
}

bool DictionaryAdd(struct DictionaryStruct* dictionary, uint64_t key, void* data)
{
    if (dictionary == NULL || data == NULL || DictionaryContains(dictionary, key))
    {
        return false;
    }

    uint64_t index = HashFunction(key) % dictionary->MaxEntries;
    dictionary->Count++;
    
    struct DictionaryEntry* nextEntry = (struct DictionaryEntry*)malloc(sizeof(struct DictionaryEntry));
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

void DictionaryDelete(struct DictionaryStruct* dictionary, uint64_t key)
{
    if (dictionary == NULL)
    {
        return;
    }

    uint64_t index = HashFunction(key) % dictionary->MaxEntries;
    dictionary->Count--;

    if (dictionary->DictionaryEntries[index] != NULL)
    {
        struct DictionaryEntry* entry = dictionary->DictionaryEntries[index];

        if (entry->Key == key)
        {
            if (entry->Next != NULL)
            {
                struct DictionaryEntry* nextEntry = entry->Next;
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
                struct DictionaryEntry* nextEntry = entry->Next->Next;
                free(entry->Next);
                entry->Next = nextEntry;
                return;
            }

            entry = entry->Next;
        }
    }
}

void DictionaryEnumerateEntries(struct DictionaryStruct* dictionary, DictionaryEnumerateEntry enumerateFunction)
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
            struct DictionaryEntry* nextEntry = dictionary->DictionaryEntries[i]->Next;

            while (nextEntry != NULL)
            {
                enumerateFunction(nextEntry->Key, nextEntry->Data);
                nextEntry = nextEntry->Next;
            }
        }
    }
}

void DictionaryPrint(struct DictionaryStruct* dictionary)
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

            struct DictionaryEntry* nextEntry = dictionary->DictionaryEntries[i]->Next;

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

#ifdef _cplusplus
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
#endif