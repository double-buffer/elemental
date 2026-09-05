#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "utest.h"

struct DataPoolRobustnessData
{
    uint64_t Value;
};

struct DataPoolConcurrentAddOneParameter
{
    SystemDataPool<DataPoolRobustnessData, SystemDataPoolDefaultFull> DataPool;
    ElemHandle* Result;
    uint64_t Value;
};

struct DataPoolConcurrentRemoveOneParameter
{
    SystemDataPool<DataPoolRobustnessData, SystemDataPoolDefaultFull> DataPool;
    ElemHandle Handle;
};

struct DataPoolConcurrentReuseParameter
{
    SystemDataPool<DataPoolRobustnessData, SystemDataPoolDefaultFull> DataPool;
    ElemHandle Handle;
    uint32_t ThreadId;
    uint32_t IterationCount;
    bool Failed;
};

void DataPoolConcurrentAddOneFunction(void* parameter)
{
    auto threadParameter = (DataPoolConcurrentAddOneParameter*)parameter;
    DataPoolRobustnessData data = { threadParameter->Value };
    *threadParameter->Result = SystemAddDataPoolItem(threadParameter->DataPool, data);
}

void DataPoolConcurrentRemoveOneFunction(void* parameter)
{
    auto threadParameter = (DataPoolConcurrentRemoveOneParameter*)parameter;
    SystemRemoveDataPoolItem(threadParameter->DataPool, threadParameter->Handle);
}

void DataPoolConcurrentReuseFunction(void* parameter)
{
    auto threadParameter = (DataPoolConcurrentReuseParameter*)parameter;
    auto handle = threadParameter->Handle;

    for (uint32_t i = 0; i < threadParameter->IterationCount; i++)
    {
        SystemRemoveDataPoolItem(threadParameter->DataPool, handle);

        DataPoolRobustnessData data = {};
        data.Value = ((uint64_t)threadParameter->ThreadId << 32) | i;
        handle = SystemAddDataPoolItem(threadParameter->DataPool, data);

        if (handle == ELEM_HANDLE_NULL)
        {
            threadParameter->Failed = true;
            return;
        }
    }

    threadParameter->Handle = handle;
}

UTEST(DataPoolRobustness, ConcurrentAddStopsAtCapacity)
{
    // Arrange
    const int32_t threadCount = 32;
    const int32_t capacity = 8;
    auto memoryArena = SystemAllocateMemoryArena();
    auto dataPool = SystemCreateDataPool<DataPoolRobustnessData>(memoryArena, capacity);
    ElemHandle handles[threadCount] = {};
    SystemThread threads[threadCount];
    DataPoolConcurrentAddOneParameter threadParameters[threadCount];

    for (int32_t i = 0; i < threadCount; i++)
    {
        threadParameters[i] = { dataPool, &handles[i], (uint64_t)i };
        threads[i] = SystemCreateThread(DataPoolConcurrentAddOneFunction, &threadParameters[i]);
    }

    // Act
    for (int32_t i = 0; i < threadCount; i++)
    {
        SystemWaitThread(threads[i]);
        SystemFreeThread(threads[i]);
    }

    // Assert
    auto successCount = 0;

    for (int32_t i = 0; i < threadCount; i++)
    {
        if (handles[i] == ELEM_HANDLE_NULL)
        {
            continue;
        }

        successCount++;
        auto handleInfo = UnpackSystemDataPoolHandle(handles[i]);

        for (int32_t j = i + 1; j < threadCount; j++)
        {
            if (handles[j] != ELEM_HANDLE_NULL)
            {
                ASSERT_TRUE(handleInfo.Index != UnpackSystemDataPoolHandle(handles[j]).Index);
            }
        }
    }

    ASSERT_EQ(capacity, successCount);
    ASSERT_EQ((size_t)capacity, SystemGetDataPoolItemCount(dataPool));
    SystemFreeMemoryArena(memoryArena);
}

UTEST(DataPoolRobustness, ConcurrentRemoveSameHandleOnlyFreesOnce)
{
    // Arrange
    const int32_t threadCount = 16;
    auto memoryArena = SystemAllocateMemoryArena();
    auto dataPool = SystemCreateDataPool<DataPoolRobustnessData>(memoryArena, 1);
    auto handle = SystemAddDataPoolItem(dataPool, DataPoolRobustnessData { 42 });
    SystemThread threads[threadCount];
    DataPoolConcurrentRemoveOneParameter threadParameters[threadCount];

    for (int32_t i = 0; i < threadCount; i++)
    {
        threadParameters[i] = { dataPool, handle };
        threads[i] = SystemCreateThread(DataPoolConcurrentRemoveOneFunction, &threadParameters[i]);
    }

    // Act
    for (int32_t i = 0; i < threadCount; i++)
    {
        SystemWaitThread(threads[i]);
        SystemFreeThread(threads[i]);
    }

    // Assert
    ASSERT_EQ(0llu, SystemGetDataPoolItemCount(dataPool));

    auto reusedHandle = SystemAddDataPoolItem(dataPool, DataPoolRobustnessData { 100 });
    auto overflowHandle = SystemAddDataPoolItem(dataPool, DataPoolRobustnessData { 200 });
    ASSERT_TRUE(reusedHandle != ELEM_HANDLE_NULL);
    ASSERT_TRUE(overflowHandle == ELEM_HANDLE_NULL);
    ASSERT_EQ(1llu, SystemGetDataPoolItemCount(dataPool));
    SystemFreeMemoryArena(memoryArena);
}

UTEST(DataPoolRobustness, ConcurrentReuseKeepsSlotsUnique)
{
    // Arrange
    const int32_t threadCount = 16;
    const uint32_t iterationCount = 5000;
    auto memoryArena = SystemAllocateMemoryArena();
    auto dataPool = SystemCreateDataPool<DataPoolRobustnessData>(memoryArena, threadCount);
    SystemThread threads[threadCount];
    DataPoolConcurrentReuseParameter threadParameters[threadCount];

    for (int32_t i = 0; i < threadCount; i++)
    {
        auto handle = SystemAddDataPoolItem(dataPool, DataPoolRobustnessData { (uint64_t)i });
        threadParameters[i] = { dataPool, handle, (uint32_t)i, iterationCount, false };
        threads[i] = SystemCreateThread(DataPoolConcurrentReuseFunction, &threadParameters[i]);
    }

    // Act
    for (int32_t i = 0; i < threadCount; i++)
    {
        SystemWaitThread(threads[i]);
        SystemFreeThread(threads[i]);
    }

    // Assert
    ASSERT_EQ((size_t)threadCount, SystemGetDataPoolItemCount(dataPool));

    for (int32_t i = 0; i < threadCount; i++)
    {
        ASSERT_FALSE(threadParameters[i].Failed);
        ASSERT_TRUE(threadParameters[i].Handle != ELEM_HANDLE_NULL);

        auto data = SystemGetDataPoolItem(dataPool, threadParameters[i].Handle);
        ASSERT_TRUE(data != nullptr);
        ASSERT_EQ((((uint64_t)i << 32) | (iterationCount - 1)), data->Value);

        auto handleInfo = UnpackSystemDataPoolHandle(threadParameters[i].Handle);

        for (int32_t j = i + 1; j < threadCount; j++)
        {
            auto otherHandleInfo = UnpackSystemDataPoolHandle(threadParameters[j].Handle);
            ASSERT_TRUE(handleInfo.Index != otherHandleInfo.Index);
        }
    }

    SystemFreeMemoryArena(memoryArena);
}
