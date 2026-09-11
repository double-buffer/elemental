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
    ASSERT_EQ_MSG(testData.Data, result->Data, "DataPool item data should match the value used during insertion.");
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
    ASSERT_TRUE_MSG(result == nullptr, "Removed DataPool handle should no longer resolve to an item.");
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
    ASSERT_FALSE_MSG(result == nullptr, "DataPool should reuse a slot released by a removed item.");
    ASSERT_EQ_MSG(testData.Data, result->Data, "Reused DataPool slot should contain the new item data.");
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
    ASSERT_FALSE_MSG(result == nullptr, "Removing a stale DataPool handle must not remove the reused slot.");
    ASSERT_EQ_MSG(testData.Data, result->Data, "Reused DataPool item should remain unchanged after stale-handle removal.");
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
    ASSERT_EQ_MSG(testData.Data, result->Data, "DataPool primary item data is invalid.");

    auto resultFull = SystemGetDataPoolItemFull(dataPool, handle);
    ASSERT_EQ_MSG(testDataFull.Data1, resultFull->Data1, "DataPool full item field Data1 is invalid.");
    ASSERT_EQ_MSG(testDataFull.Data2, resultFull->Data2, "DataPool full item field Data2 is invalid.");
    ASSERT_EQ_MSG(testDataFull.Data3, resultFull->Data3, "DataPool full item field Data3 is invalid.");
    ASSERT_EQ_MSG(testDataFull.Data4, resultFull->Data4, "DataPool full item field Data4 is invalid.");
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
    ASSERT_TRUE_MSG(result == nullptr, "Removed DataPool handle should not resolve to full item data.");
}
