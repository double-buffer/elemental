#include "Dictionary.h"


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
