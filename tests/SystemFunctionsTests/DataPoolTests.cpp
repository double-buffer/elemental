#include "SystemDataPool.h"
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
    ASSERT_EQ(testDataFull.Data1, result->Data1);
    ASSERT_EQ(testDataFull.Data2, result->Data2);
    ASSERT_EQ(testDataFull.Data3, result->Data3);
    ASSERT_EQ(testDataFull.Data4, result->Data4);
}
