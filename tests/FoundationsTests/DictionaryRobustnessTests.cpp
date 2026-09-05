#include "SystemDictionary.h"
#include "SystemFunctions.h"
#include "utest.h"

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

UTEST(DictionaryRobustness, ReadOnlySpanHashUsesAllBytes)
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dictionary = SystemCreateDictionary<ReadOnlySpan<uint32_t>, int32_t>(stackMemoryArena, 8);
    uint32_t key1[] = { 0x00001234, 1 };
    uint32_t key2[] = { 0x00001234, 2 };

    // Act
    SystemAddDictionaryEntry(dictionary, ReadOnlySpan<uint32_t>(key1, 2), 10);
    SystemAddDictionaryEntry(dictionary, ReadOnlySpan<uint32_t>(key2, 2), 20);

    // Assert
    auto value1 = SystemGetDictionaryValue(dictionary, ReadOnlySpan<uint32_t>(key1, 2));
    auto value2 = SystemGetDictionaryValue(dictionary, ReadOnlySpan<uint32_t>(key2, 2));
    ASSERT_TRUE(value1 != nullptr);
    ASSERT_TRUE(value2 != nullptr);
    ASSERT_EQ(10, *value1);
    ASSERT_EQ(20, *value2);
}

UTEST(DictionaryRobustness, MissingValueReturnsNull)
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dictionary = SystemCreateDictionary<int32_t, int32_t>(stackMemoryArena, 8);

    // Act
    auto value = SystemGetDictionaryValue(dictionary, 42);

    // Assert
    ASSERT_TRUE(value == nullptr);
    ASSERT_EQ(0, dictionary[42]);
}

UTEST(DictionaryRobustness, ConcurrentAddStopsAtCapacity)
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

UTEST(DictionaryRobustness, ConcurrentReusePreservesAllEntries)
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
