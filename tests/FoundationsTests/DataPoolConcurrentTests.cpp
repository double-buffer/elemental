#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "utest.h"

struct DataPoolConcurrentTestData
{
    uint64_t Value;
};

struct DataPoolConcurrentTestDataFull
{
    uint64_t Value1;
    uint64_t Value2;
    uint64_t Value3;
    uint64_t Value4;
};

struct DataPoolConcurrentBatchParameter
{
    SystemDataPool<DataPoolConcurrentTestData, DataPoolConcurrentTestDataFull> DataPool;
    int32_t ThreadId;
    int32_t ItemCount;
    Span<ElemHandle> Handles;
};

struct DataPoolConcurrentAddOneParameter
{
    SystemDataPool<DataPoolConcurrentTestData, SystemDataPoolDefaultFull> DataPool;
    ElemHandle* Result;
    uint64_t Value;
};

struct DataPoolConcurrentRemoveOneParameter
{
    SystemDataPool<DataPoolConcurrentTestData, SystemDataPoolDefaultFull> DataPool;
    ElemHandle Handle;
};

struct DataPoolConcurrentReuseParameter
{
    SystemDataPool<DataPoolConcurrentTestData, SystemDataPoolDefaultFull> DataPool;
    ElemHandle Handle;
    uint32_t ThreadId;
    uint32_t IterationCount;
    bool Failed;
};

void DataPoolConcurrentBatchAddFunction(void* parameter)
{
    auto threadParameter = (DataPoolConcurrentBatchParameter*)parameter;
    auto dataPool = threadParameter->DataPool;

    for (int32_t i = 0; i < threadParameter->ItemCount; i++)
    {
        auto value = (uint64_t)threadParameter->ThreadId * threadParameter->ItemCount + i;
        DataPoolConcurrentTestData data = { value };
        DataPoolConcurrentTestDataFull dataFull = { value, value + 1, value + 2, value + 3 };

        auto handle = SystemAddDataPoolItem(dataPool, data);
        SystemAddDataPoolItemFull(dataPool, handle, dataFull);
        threadParameter->Handles[i] = handle;
    }
}

void DataPoolConcurrentBatchRemoveFunction(void* parameter)
{
    auto threadParameter = (DataPoolConcurrentBatchParameter*)parameter;

    for (int32_t i = 0; i < threadParameter->ItemCount; i++)
    {
        SystemRemoveDataPoolItem(threadParameter->DataPool, threadParameter->Handles[i]);
    }
}

void DataPoolConcurrentAddOneFunction(void* parameter)
{
    auto threadParameter = (DataPoolConcurrentAddOneParameter*)parameter;
    *threadParameter->Result = SystemAddDataPoolItem(threadParameter->DataPool, DataPoolConcurrentTestData { threadParameter->Value });
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

        auto value = ((uint64_t)threadParameter->ThreadId << 32) | i;
        handle = SystemAddDataPoolItem(threadParameter->DataPool, DataPoolConcurrentTestData { value });

        if (handle == ELEM_HANDLE_NULL)
        {
            threadParameter->Failed = true;
            return;
        }
    }

    threadParameter->Handle = handle;
}

UTEST(DataPoolConcurrent, Add)
{
    // Arrange
    const int32_t itemCount = 80000;
    const int32_t threadCount = 32;
    auto memoryArena = SystemAllocateMemoryArena();
    auto dataPool = SystemCreateDataPool<DataPoolConcurrentTestData, DataPoolConcurrentTestDataFull>(memoryArena, itemCount);
    SystemThread threads[threadCount];
    DataPoolConcurrentBatchParameter threadParameters[threadCount];

    // Act
    for (int32_t i = 0; i < threadCount; i++)
    {
        threadParameters[i].DataPool = dataPool;
        threadParameters[i].ItemCount = itemCount / threadCount;
        threadParameters[i].ThreadId = i;
        threadParameters[i].Handles = SystemPushArray<ElemHandle>(memoryArena, threadParameters[i].ItemCount);
        threads[i] = SystemCreateThread(DataPoolConcurrentBatchAddFunction, &threadParameters[i]);
    }

    for (int32_t i = 0; i < threadCount; i++)
    {
        SystemWaitThread(threads[i]);
        SystemFreeThread(threads[i]);
    }

    // Assert
    ASSERT_EQ_MSG(itemCount, (int32_t)SystemGetDataPoolItemCount(dataPool), "Concurrent DataPool insertion should publish every requested item.");

    for (int32_t i = 0; i < threadCount; i++)
    {
        auto threadParameter = threadParameters[i];

        for (int32_t j = 0; j < threadParameter.ItemCount; j++)
        {
            auto expectedValue = (uint64_t)threadParameter.ThreadId * threadParameter.ItemCount + j;
            auto data = SystemGetDataPoolItem(dataPool, threadParameter.Handles[j]);
            auto dataFull = SystemGetDataPoolItemFull(dataPool, threadParameter.Handles[j]);

            ASSERT_EQ_MSG(expectedValue, data->Value, "Concurrent DataPool insertion corrupted primary item data.");
            ASSERT_EQ_MSG(expectedValue, dataFull->Value1, "Concurrent DataPool insertion corrupted full item Value1.");
            ASSERT_EQ_MSG(expectedValue + 1, dataFull->Value2, "Concurrent DataPool insertion corrupted full item Value2.");
            ASSERT_EQ_MSG(expectedValue + 2, dataFull->Value3, "Concurrent DataPool insertion corrupted full item Value3.");
            ASSERT_EQ_MSG(expectedValue + 3, dataFull->Value4, "Concurrent DataPool insertion corrupted full item Value4.");
        }
    }

    SystemFreeMemoryArena(memoryArena);
}

UTEST(DataPoolConcurrent, AddAndRemove)
{
    // Arrange
    const int32_t itemCount = 80000;
    const int32_t threadCount = 32;
    auto memoryArena = SystemAllocateMemoryArena();
    auto dataPool = SystemCreateDataPool<DataPoolConcurrentTestData, DataPoolConcurrentTestDataFull>(memoryArena, itemCount);
    SystemThread addThreads[threadCount];
    SystemThread removeThreads[threadCount];
    DataPoolConcurrentBatchParameter firstParameters[threadCount];
    DataPoolConcurrentBatchParameter secondParameters[threadCount];

    for (int32_t i = 0; i < threadCount; i++)
    {
        firstParameters[i].DataPool = dataPool;
        firstParameters[i].ItemCount = itemCount / 2 / threadCount;
        firstParameters[i].ThreadId = i;
        firstParameters[i].Handles = SystemPushArray<ElemHandle>(memoryArena, firstParameters[i].ItemCount);
        addThreads[i] = SystemCreateThread(DataPoolConcurrentBatchAddFunction, &firstParameters[i]);
    }

    for (int32_t i = 0; i < threadCount; i++)
    {
        SystemWaitThread(addThreads[i]);
        SystemFreeThread(addThreads[i]);
    }

    // Act
    for (int32_t i = 0; i < threadCount; i++)
    {
        secondParameters[i].DataPool = dataPool;
        secondParameters[i].ItemCount = itemCount / 2 / threadCount;
        secondParameters[i].ThreadId = i;
        secondParameters[i].Handles = SystemPushArray<ElemHandle>(memoryArena, secondParameters[i].ItemCount);

        addThreads[i] = SystemCreateThread(DataPoolConcurrentBatchAddFunction, &secondParameters[i]);
        removeThreads[i] = SystemCreateThread(DataPoolConcurrentBatchRemoveFunction, &firstParameters[i]);
    }

    for (int32_t i = 0; i < threadCount; i++)
    {
        SystemWaitThread(addThreads[i]);
        SystemWaitThread(removeThreads[i]);
        SystemFreeThread(addThreads[i]);
        SystemFreeThread(removeThreads[i]);
    }

    // Assert
    ASSERT_EQ_MSG((size_t)itemCount / 2, SystemGetDataPoolItemCount(dataPool), "Concurrent DataPool add/remove should preserve the expected final item count.");
    SystemFreeMemoryArena(memoryArena);
}

UTEST(DataPoolConcurrent, AddStopsAtCapacity)
{
    // Arrange
    const int32_t threadCount = 32;
    const int32_t capacity = 8;
    auto memoryArena = SystemAllocateMemoryArena();
    auto dataPool = SystemCreateDataPool<DataPoolConcurrentTestData>(memoryArena, capacity);
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
                ASSERT_TRUE_MSG(handleInfo.Index != UnpackSystemDataPoolHandle(handles[j]).Index, "Concurrent DataPool allocations must never publish the same slot twice.");
            }
        }
    }

    ASSERT_EQ_MSG(capacity, successCount, "Concurrent DataPool insertion should stop exactly at pool capacity.");
    ASSERT_EQ_MSG((size_t)capacity, SystemGetDataPoolItemCount(dataPool), "DataPool item count should never exceed capacity under contention.");
    SystemFreeMemoryArena(memoryArena);
}

UTEST(DataPoolConcurrent, RemoveSameHandleOnlyFreesOnce)
{
    // Arrange
    const int32_t threadCount = 16;
    auto memoryArena = SystemAllocateMemoryArena();
    auto dataPool = SystemCreateDataPool<DataPoolConcurrentTestData>(memoryArena, 1);
    auto handle = SystemAddDataPoolItem(dataPool, DataPoolConcurrentTestData { 42 });
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
    ASSERT_EQ_MSG(0llu, SystemGetDataPoolItemCount(dataPool), "Concurrent removal of one handle should decrement the pool count exactly once.");

    auto reusedHandle = SystemAddDataPoolItem(dataPool, DataPoolConcurrentTestData { 100 });
    auto overflowHandle = SystemAddDataPoolItem(dataPool, DataPoolConcurrentTestData { 200 });
    ASSERT_TRUE_MSG(reusedHandle != ELEM_HANDLE_NULL, "Slot removed concurrently should remain reusable exactly once.");
    ASSERT_TRUE_MSG(overflowHandle == ELEM_HANDLE_NULL, "A concurrently removed slot must not be recycled more than once.");
    ASSERT_EQ_MSG(1llu, SystemGetDataPoolItemCount(dataPool), "DataPool count should remain consistent after duplicate concurrent removal attempts.");
    SystemFreeMemoryArena(memoryArena);
}

UTEST(DataPoolConcurrent, ReuseKeepsSlotsUnique)
{
    // Arrange
    const int32_t threadCount = 16;
    const uint32_t iterationCount = 20000;
    auto memoryArena = SystemAllocateMemoryArena();
    auto dataPool = SystemCreateDataPool<DataPoolConcurrentTestData>(memoryArena, threadCount);
    SystemThread threads[threadCount];
    DataPoolConcurrentReuseParameter threadParameters[threadCount];

    for (int32_t i = 0; i < threadCount; i++)
    {
        auto handle = SystemAddDataPoolItem(dataPool, DataPoolConcurrentTestData { (uint64_t)i });
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
    ASSERT_EQ_MSG((size_t)threadCount, SystemGetDataPoolItemCount(dataPool), "Concurrent remove/reuse cycles should preserve the total live item count.");

    for (int32_t i = 0; i < threadCount; i++)
    {
        ASSERT_FALSE_MSG(threadParameters[i].Failed, "Concurrent DataPool remove/reuse unexpectedly exhausted a reusable pool.");
        ASSERT_TRUE_MSG(threadParameters[i].Handle != ELEM_HANDLE_NULL, "Concurrent DataPool remove/reuse should leave every thread with a valid handle.");

        auto data = SystemGetDataPoolItem(dataPool, threadParameters[i].Handle);
        ASSERT_TRUE_MSG(data != nullptr, "Final DataPool handle should resolve after concurrent reuse cycles.");
        ASSERT_EQ_MSG((((uint64_t)i << 32) | (iterationCount - 1)), data->Value, "Concurrent DataPool reuse corrupted final item data.");

        auto handleInfo = UnpackSystemDataPoolHandle(threadParameters[i].Handle);

        for (int32_t j = i + 1; j < threadCount; j++)
        {
            auto otherHandleInfo = UnpackSystemDataPoolHandle(threadParameters[j].Handle);
            ASSERT_TRUE_MSG(handleInfo.Index != otherHandleInfo.Index, "Concurrent DataPool reuse must not assign one slot to multiple live handles.");
        }
    }

    SystemFreeMemoryArena(memoryArena);
}
