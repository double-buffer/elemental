#include "SystemDataPool.h"
#include "utest.h"

struct DataPoolTestData
{
    uint64_t Data;
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
// TODO: Remove wrong version should not delete

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
