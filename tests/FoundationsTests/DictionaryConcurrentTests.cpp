#include "SystemDictionary.h"
#include "SystemFunctions.h"
#include "utest.h"

struct DictionaryConcurrentBatchParameter
{
    SystemDictionary<int32_t, int32_t> Dictionary;
    int32_t ThreadId;
    int32_t ItemCount;
};

struct DictionaryConcurrentAddOneParameter
{
    SystemDictionary<int32_t, int32_t> Dictionary;
    int32_t Key;
};

struct DictionaryConcurrentReuseParameter
{
    SystemDictionary<int64_t, uint64_t> Dictionary;
    int64_t CurrentKey;
    uint32_t ThreadId;
    uint32_t IterationCount;
};

void DictionaryConcurrentBatchAddFunction(void* parameter)
{
    auto threadParameter = (DictionaryConcurrentBatchParameter*)parameter;

    for (int32_t i = 0; i < threadParameter->ItemCount; i++)
    {
        auto key = threadParameter->ThreadId * threadParameter->ItemCount + i;
        SystemAddDictionaryEntry(threadParameter->Dictionary, key, key);
    }
}

void DictionaryConcurrentBatchRemoveFunction(void* parameter)
{
    auto threadParameter = (DictionaryConcurrentBatchParameter*)parameter;

    for (int32_t i = 0; i < threadParameter->ItemCount; i++)
    {
        auto key = threadParameter->ThreadId * threadParameter->ItemCount + i;
        SystemRemoveDictionaryEntry(threadParameter->Dictionary, key);
    }
}

void DictionaryConcurrentAddOneFunction(void* parameter)
{
    auto threadParameter = (DictionaryConcurrentAddOneParameter*)parameter;
    SystemAddDictionaryEntry(threadParameter->Dictionary, threadParameter->Key, threadParameter->Key);
}

void DictionaryConcurrentReuseFunction(void* parameter)
{
    auto threadParameter = (DictionaryConcurrentReuseParameter*)parameter;

    for (uint32_t i = 0; i < threadParameter->IterationCount; i++)
    {
        SystemRemoveDictionaryEntry(threadParameter->Dictionary, threadParameter->CurrentKey);

        auto key = (int64_t)threadParameter->ThreadId * 1000000 + i + 1;
        auto value = ((uint64_t)threadParameter->ThreadId << 32) | i;
        SystemAddDictionaryEntry(threadParameter->Dictionary, key, value);
        threadParameter->CurrentKey = key;
    }
}

UTEST(DictionaryConcurrent, Add)
{
    // Arrange
    const int32_t itemCount = 80000;
    const int32_t threadCount = 32;
    auto memoryArena = SystemAllocateMemoryArena();
    auto dictionary = SystemCreateDictionary<int32_t, int32_t>(memoryArena, itemCount);
    SystemThread threads[threadCount];
    DictionaryConcurrentBatchParameter threadParameters[threadCount];

    // Act
    for (int32_t i = 0; i < threadCount; i++)
    {
        threadParameters[i] = { dictionary, i, itemCount / threadCount };
        threads[i] = SystemCreateThread(DictionaryConcurrentBatchAddFunction, &threadParameters[i]);
    }

    for (int32_t i = 0; i < threadCount; i++)
    {
        SystemWaitThread(threads[i]);
        SystemFreeThread(threads[i]);
    }

    // Assert
    auto count = 0;

    for (int32_t i = 0; i < itemCount; i++)
    {
        if (SystemDictionaryContainsKey(dictionary, i))
        {
            count++;
        }
    }

    ASSERT_EQ(itemCount, count);
    SystemFreeMemoryArena(memoryArena);
}

UTEST(DictionaryConcurrent, Remove)
{
    // Arrange
    const int32_t itemCount = 32000;
    const int32_t threadCount = 32;
    auto memoryArena = SystemAllocateMemoryArena();
    auto dictionary = SystemCreateDictionary<int32_t, int32_t>(memoryArena, itemCount);

    for (int32_t i = 0; i < itemCount; i++)
    {
        SystemAddDictionaryEntry(dictionary, i, i);
    }

    SystemThread threads[threadCount];
    DictionaryConcurrentBatchParameter threadParameters[threadCount];

    // Act
    for (int32_t i = 0; i < threadCount; i++)
    {
        threadParameters[i] = { dictionary, i, (itemCount / 2) / threadCount };
        threads[i] = SystemCreateThread(DictionaryConcurrentBatchRemoveFunction, &threadParameters[i]);
    }

    for (int32_t i = 0; i < threadCount; i++)
    {
        SystemWaitThread(threads[i]);
        SystemFreeThread(threads[i]);
    }

    // Assert
    auto count = 0;

    for (int32_t i = 0; i < itemCount; i++)
    {
        if (SystemDictionaryContainsKey(dictionary, i))
        {
            count++;
        }
    }

    ASSERT_EQ(itemCount / 2, count);
    SystemFreeMemoryArena(memoryArena);
}

UTEST(DictionaryConcurrent, AddStopsAtCapacity)
{
    // Arrange
    const int32_t threadCount = 32;
    const int32_t capacity = 8;
    auto memoryArena = SystemAllocateMemoryArena();
    auto dictionary = SystemCreateDictionary<int32_t, int32_t>(memoryArena, capacity);
    SystemThread threads[threadCount];
    DictionaryConcurrentAddOneParameter threadParameters[threadCount];

    for (int32_t i = 0; i < threadCount; i++)
    {
        threadParameters[i] = { dictionary, i };
        threads[i] = SystemCreateThread(DictionaryConcurrentAddOneFunction, &threadParameters[i]);
    }

    // Act
    for (int32_t i = 0; i < threadCount; i++)
    {
        SystemWaitThread(threads[i]);
        SystemFreeThread(threads[i]);
    }

    // Assert
    auto foundCount = 0;

    for (int32_t i = 0; i < threadCount; i++)
    {
        if (SystemDictionaryContainsKey(dictionary, i))
        {
            auto value = SystemGetDictionaryValue(dictionary, i);
            ASSERT_TRUE(value != nullptr);
            ASSERT_EQ(i, *value);
            foundCount++;
        }
    }

    ASSERT_EQ(capacity, foundCount);
    SystemFreeMemoryArena(memoryArena);
}

UTEST(DictionaryConcurrent, ReusePreservesAllEntries)
{
    // Arrange
    const int32_t threadCount = 16;
    const uint32_t iterationCount = 5000;
    auto memoryArena = SystemAllocateMemoryArena();
    auto dictionary = SystemCreateDictionary<int64_t, uint64_t>(memoryArena, threadCount);
    SystemThread threads[threadCount];
    DictionaryConcurrentReuseParameter threadParameters[threadCount];

    for (int32_t i = 0; i < threadCount; i++)
    {
        auto initialKey = -(int64_t)i - 1;
        SystemAddDictionaryEntry(dictionary, initialKey, (uint64_t)i);
        threadParameters[i] = { dictionary, initialKey, (uint32_t)i, iterationCount };
        threads[i] = SystemCreateThread(DictionaryConcurrentReuseFunction, &threadParameters[i]);
    }

    // Act
    for (int32_t i = 0; i < threadCount; i++)
    {
        SystemWaitThread(threads[i]);
        SystemFreeThread(threads[i]);
    }

    // Assert
    for (int32_t i = 0; i < threadCount; i++)
    {
        auto value = SystemGetDictionaryValue(dictionary, threadParameters[i].CurrentKey);
        ASSERT_TRUE(value != nullptr);
        ASSERT_EQ((((uint64_t)i << 32) | (iterationCount - 1)), *value);
    }

    SystemFreeMemoryArena(memoryArena);
}
