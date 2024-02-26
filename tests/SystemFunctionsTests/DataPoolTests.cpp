#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "utest.h"

struct DataPoolTestData
{
    uint64_t Data;
};

struct DataPoolTestDataFull
{
    uint64_t Data1;
    uint64_t Data2;
    uint64_t Data3;
    uint64_t Data4;
};

struct DataPoolThreadParameter
{
    SystemDataPool<DataPoolTestData, DataPoolTestDataFull> DataPool;
    int32_t ThreadId;
    int32_t ItemCount;
    Span<ElemHandle> Handles;
};

void DataPoolConcurrentAddFunction(void* parameter)
{
    auto threadParameter = (DataPoolThreadParameter*)parameter;
    auto dataPool = threadParameter->DataPool;

    for (int32_t i = 0; i < threadParameter->ItemCount; i++)
    {
        DataPoolTestData testData = {};
        testData.Data = threadParameter->ThreadId * threadParameter->ItemCount + i;

        DataPoolTestDataFull testDataFull = {};
        testDataFull.Data1 = threadParameter->ThreadId * threadParameter->ItemCount + i;
        testDataFull.Data2 = threadParameter->ThreadId * threadParameter->ItemCount + i + 1;
        testDataFull.Data3 = threadParameter->ThreadId * threadParameter->ItemCount + i + 2;
        testDataFull.Data4 = threadParameter->ThreadId * threadParameter->ItemCount + i + 3;

        auto handle = SystemAddDataPoolItem(dataPool, testData);
        SystemAddDataPoolItemFull(dataPool, handle, testDataFull);

        threadParameter->Handles[i] = handle;
    }
}

void DataPoolConcurrentRemoveFunction(void* parameter)
{
    auto threadParameter = (DataPoolThreadParameter*)parameter;
    auto dataPool = threadParameter->DataPool;

    for (int32_t i = 0; i < threadParameter->ItemCount; i++)
    {
        SystemRemoveDataPoolItem(dataPool, threadParameter->Handles[i]);
    }
}

UTEST(DataPool, AddItem) 
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dataPool = SystemCreateDataPool<DataPoolTestData>(stackMemoryArena, 10);

    DataPoolTestData testData = {};
    testData.Data = 5;

    // Act
    auto handle = SystemAddDataPoolItem(dataPool, testData);

    // Assert
    auto result = SystemGetDataPoolItem(dataPool, handle);
    ASSERT_EQ(testData.Data, result->Data);
}

UTEST(DataPool, RemoveItem) 
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dataPool = SystemCreateDataPool<DataPoolTestData>(stackMemoryArena, 10);
    DataPoolTestData testData = {};
    testData.Data = 5;
    auto handle = SystemAddDataPoolItem(dataPool, testData);

    // Act
    SystemRemoveDataPoolItem(dataPool, handle);

    // Assert
    auto result = SystemGetDataPoolItem(dataPool, handle);
    ASSERT_TRUE(result == nullptr);
}

UTEST(DataPool, AddItemReuseDeletedItem) 
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dataPool = SystemCreateDataPool<DataPoolTestData>(stackMemoryArena, 1);

    DataPoolTestData testData = {};
    testData.Data = 5;
    auto handle = SystemAddDataPoolItem(dataPool, testData);
    SystemRemoveDataPoolItem(dataPool, handle);

    // Act
    testData.Data = 15;
    handle = SystemAddDataPoolItem(dataPool, testData);

    // Assert
    auto result = SystemGetDataPoolItem(dataPool, handle);
    ASSERT_FALSE(result == nullptr);
    ASSERT_EQ(testData.Data, result->Data);
}

UTEST(DataPool, RemoveReusedItemWithOldVersion) 
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dataPool = SystemCreateDataPool<DataPoolTestData>(stackMemoryArena, 1);

    DataPoolTestData testData = {};
    testData.Data = 5;
    auto handle = SystemAddDataPoolItem(dataPool, testData);
    SystemRemoveDataPoolItem(dataPool, handle);

    testData.Data = 15;
    auto newHandle = SystemAddDataPoolItem(dataPool, testData);

    // Act
    SystemRemoveDataPoolItem(dataPool, handle);

    // Assert
    auto result = SystemGetDataPoolItem(dataPool, newHandle);
    ASSERT_FALSE(result == nullptr);
    ASSERT_EQ(testData.Data, result->Data);
}

UTEST(DataPool, AddItemWithFull) 
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dataPool = SystemCreateDataPool<DataPoolTestData, DataPoolTestDataFull>(stackMemoryArena, 10);

    DataPoolTestData testData = {};
    testData.Data = 5;

    DataPoolTestDataFull testDataFull = {};
    testDataFull.Data1 = 5;
    testDataFull.Data2 = 6;
    testDataFull.Data3 = 7;
    testDataFull.Data4 = 8;

    // Act
    auto handle = SystemAddDataPoolItem(dataPool, testData);
    SystemAddDataPoolItemFull(dataPool, handle, testDataFull);

    // Assert
    auto result = SystemGetDataPoolItem(dataPool, handle);
    ASSERT_EQ(testData.Data, result->Data);

    auto resultFull = SystemGetDataPoolItemFull(dataPool, handle);
    ASSERT_EQ(testDataFull.Data1, resultFull->Data1);
    ASSERT_EQ(testDataFull.Data2, resultFull->Data2);
    ASSERT_EQ(testDataFull.Data3, resultFull->Data3);
    ASSERT_EQ(testDataFull.Data4, resultFull->Data4);
}

UTEST(DataPool, RemoveItemWithFull) 
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dataPool = SystemCreateDataPool<DataPoolTestData, DataPoolTestDataFull>(stackMemoryArena, 10);

    DataPoolTestData testData = {};
    testData.Data = 5;

    DataPoolTestDataFull testDataFull = {};
    testDataFull.Data1 = 5;
    testDataFull.Data2 = 6;
    testDataFull.Data3 = 7;
    testDataFull.Data4 = 8;

    auto handle = SystemAddDataPoolItem(dataPool, testData);
    SystemAddDataPoolItemFull(dataPool, handle, testDataFull);

    // Act
    SystemRemoveDataPoolItem(dataPool, handle);

    // Assert
    auto result = SystemGetDataPoolItemFull(dataPool, handle);
    ASSERT_TRUE(result == nullptr);
}

UTEST(DataPool, ConcurrentAdd) 
{
    // Arrange
    const int32_t itemCount = 80000;
    const int32_t threadCount = 32;
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dataPool = SystemCreateDataPool<DataPoolTestData, DataPoolTestDataFull>(stackMemoryArena, itemCount);
    
    // Act
    SystemThread threads[threadCount];
    DataPoolThreadParameter threadParameters[threadCount];

    for (int32_t i = 0; i < threadCount; i++)
    {
        threadParameters[i] = { dataPool, i, itemCount / threadCount };
        threadParameters[i].Handles = SystemPushArray<ElemHandle>(stackMemoryArena, threadParameters[i].ItemCount);
        threads[i] = SystemCreateThread(DataPoolConcurrentAddFunction, &threadParameters[i]);
    }

    for (int32_t i = 0; i < threadCount; i++)
    {
        SystemWaitThread(threads[i]);
        SystemFreeThread(threads[i]);
    }

    // Assert
    auto dataPoolCount = SystemGetDataPoolItemCount(dataPool);
    ASSERT_EQ(itemCount, dataPoolCount);

    for (int32_t i = 0; i < threadCount; i++)
    {
        auto threadParameter = threadParameters[i];

        for (int32_t j = 0; j < itemCount / threadCount; j++)
        {
            auto testData = SystemGetDataPoolItem(dataPool, threadParameter.Handles[j]);
            ASSERT_EQ(threadParameter.ThreadId * threadParameter.ItemCount + j, testData->Data);

            auto testDataFull = SystemGetDataPoolItemFull(dataPool, threadParameter.Handles[j]);
            ASSERT_EQ(threadParameter.ThreadId * threadParameter.ItemCount + j, testDataFull->Data1);
            ASSERT_EQ(threadParameter.ThreadId * threadParameter.ItemCount + j + 1, testDataFull->Data2);
            ASSERT_EQ(threadParameter.ThreadId * threadParameter.ItemCount + j + 2, testDataFull->Data3);
            ASSERT_EQ(threadParameter.ThreadId * threadParameter.ItemCount + j + 3, testDataFull->Data4);
        }
    }
}

UTEST(DataPool, ConcurrentRemove) 
{
    // Arrange
    const int32_t itemCount = 80000;
    const int32_t threadCount = 32;
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto dataPool = SystemCreateDataPool<DataPoolTestData, DataPoolTestDataFull>(stackMemoryArena, itemCount);
    
    // Act
    SystemThread threads[threadCount];
    SystemThread threads2[threadCount];
    DataPoolThreadParameter threadParameters[threadCount];
    DataPoolThreadParameter threadParameters2[threadCount];

    for (int32_t i = 0; i < threadCount; i++)
    {
        threadParameters[i] = { dataPool, i, itemCount / 2 / threadCount };
        threadParameters[i].Handles = SystemPushArray<ElemHandle>(stackMemoryArena, threadParameters[i].ItemCount);
        threads[i] = SystemCreateThread(DataPoolConcurrentAddFunction, &threadParameters[i]);
    }

    for (int32_t i = 0; i < threadCount; i++)
    {
        SystemWaitThread(threads[i]);
        SystemFreeThread(threads[i]);
    }

    for (int32_t i = 0; i < threadCount; i++)
    {
        threadParameters2[i] = { dataPool, i, itemCount / 2 / threadCount };
        threadParameters2[i].Handles = SystemPushArray<ElemHandle>(stackMemoryArena, threadParameters2[i].ItemCount);
        threads2[i] = SystemCreateThread(DataPoolConcurrentAddFunction, &threadParameters2[i]);

        threads[i] = SystemCreateThread(DataPoolConcurrentRemoveFunction, &threadParameters[i]);
    }

    for (int32_t i = 0; i < threadCount; i++)
    {
        SystemWaitThread(threads[i]);
        SystemWaitThread(threads2[i]);
        SystemFreeThread(threads[i]);
        SystemFreeThread(threads2[i]);
    }

    // Assert
    auto dataPoolCount = SystemGetDataPoolItemCount(dataPool);
    ASSERT_EQ(itemCount / 2, dataPoolCount);
}
