#include "SystemFunctions.h"
#include "SystemMemory.h"
#include "SystemPlatformFunctions.h"
#include "utest.h"

struct MemoryConcurrentPushParameter
{
    MemoryArena MemoryArena;
    int32_t ItemCount;
};

struct MemoryConcurrentOverflowParameter
{
    MemoryArena MemoryArena;
    bool* Start;
    void** Results;
    int32_t ThreadId;
};

struct MemoryConcurrentCommitParameter
{
    MemoryArena MemoryArena;
    uint8_t* Pointer;
    size_t SizeInBytes;
    uint8_t Value;
};

struct MemoryConcurrentArenaAllocationParameter
{
    MemoryArena* Result;
    size_t SizeInBytes;
};

void MemoryConcurrentPushFunction(void* parameter)
{
    auto threadParameter = (MemoryConcurrentPushParameter*)parameter;

    for (int32_t i = 0; i < threadParameter->ItemCount; i++)
    {
        SystemPushMemoryZero(threadParameter->MemoryArena, 64);
    }
}

void MemoryConcurrentOverflowFunction(void* parameter)
{
    auto threadParameter = (MemoryConcurrentOverflowParameter*)parameter;
    bool start = false;

    while (!start)
    {
        SystemAtomicLoad(*threadParameter->Start, start);

        if (!start)
        {
            SystemYieldThread();
        }
    }

    threadParameter->Results[threadParameter->ThreadId] = SystemPushMemory(threadParameter->MemoryArena, 64, AllocationState_Reserved);
}

void MemoryConcurrentCommitFunction(void* parameter)
{
    auto threadParameter = (MemoryConcurrentCommitParameter*)parameter;
    SystemCommitMemory(threadParameter->MemoryArena, threadParameter->Pointer, threadParameter->SizeInBytes);

    for (size_t i = 0; i < threadParameter->SizeInBytes; i++)
    {
        threadParameter->Pointer[i] = threadParameter->Value;
    }
}

void MemoryConcurrentArenaAllocationFunction(void* parameter)
{
    auto threadParameter = (MemoryConcurrentArenaAllocationParameter*)parameter;
    *threadParameter->Result = SystemAllocateMemoryArena(threadParameter->SizeInBytes);
}

UTEST(MemoryConcurrent, Push)
{
    // Arrange
    const int32_t itemCount = 80000;
    const int32_t threadCount = 32;
    auto maxSize = (size_t)itemCount * 64;
    auto memoryArena = SystemAllocateMemoryArena(maxSize);
    SystemThread threads[threadCount];
    MemoryConcurrentPushParameter threadParameters[threadCount];

    // Act
    for (int32_t i = 0; i < threadCount; i++)
    {
        threadParameters[i] = { memoryArena, itemCount / threadCount };
        threads[i] = SystemCreateThread(MemoryConcurrentPushFunction, &threadParameters[i]);
    }

    for (int32_t i = 0; i < threadCount; i++)
    {
        SystemWaitThread(threads[i]);
        SystemFreeThread(threads[i]);
    }

    // Assert
    auto allocationInfos = SystemGetMemoryArenaAllocationInfos(memoryArena);
    ASSERT_EQ(maxSize, allocationInfos.AllocatedBytes);
}

UTEST(MemoryConcurrent, PushDoesNotOverflow)
{
    // Arrange
    const int32_t threadCount = 32;
    const int32_t capacityCount = 8;
    const size_t allocationSizeInBytes = 64;
    auto memoryArena = SystemAllocateMemoryArena(capacityCount * allocationSizeInBytes);
    bool start = false;
    void* results[threadCount] = {};
    SystemThread threads[threadCount];
    MemoryConcurrentOverflowParameter threadParameters[threadCount];

    for (int32_t i = 0; i < threadCount; i++)
    {
        threadParameters[i] = { memoryArena, &start, results, i };
        threads[i] = SystemCreateThread(MemoryConcurrentOverflowFunction, &threadParameters[i]);
    }

    // Act
    SystemAtomicStore(start, true);

    for (int32_t i = 0; i < threadCount; i++)
    {
        SystemWaitThread(threads[i]);
        SystemFreeThread(threads[i]);
    }

    // Assert
    auto successCount = 0;

    for (int32_t i = 0; i < threadCount; i++)
    {
        if (results[i] != nullptr)
        {
            successCount++;

            for (int32_t j = i + 1; j < threadCount; j++)
            {
                if (results[j] != nullptr)
                {
                    ASSERT_TRUE(results[i] != results[j]);
                }
            }
        }
    }

    ASSERT_EQ(capacityCount, successCount);

    auto allocationInfos = SystemGetMemoryArenaAllocationInfos(memoryArena);
    ASSERT_EQ(capacityCount * allocationSizeInBytes, allocationInfos.AllocatedBytes);
}

UTEST(MemoryConcurrent, CommitSharedPage)
{
    // Arrange
    const int32_t threadCount = 32;
    const size_t rangeSizeInBytes = 64;
    auto pageSizeInBytes = SystemPlatformGetPageSize();
    auto memoryArena = SystemAllocateMemoryArena(pageSizeInBytes);
    auto buffer = SystemPushArray<uint8_t>(memoryArena, pageSizeInBytes, AllocationState_Reserved);
    auto committedBytesBefore = SystemGetMemoryArenaAllocationInfos(memoryArena).CommittedBytes;
    SystemThread threads[threadCount];
    MemoryConcurrentCommitParameter threadParameters[threadCount];

    for (int32_t i = 0; i < threadCount; i++)
    {
        threadParameters[i] = { memoryArena, buffer.Pointer + i * rangeSizeInBytes, rangeSizeInBytes, (uint8_t)(i + 1) };
        threads[i] = SystemCreateThread(MemoryConcurrentCommitFunction, &threadParameters[i]);
    }

    // Act
    for (int32_t i = 0; i < threadCount; i++)
    {
        SystemWaitThread(threads[i]);
        SystemFreeThread(threads[i]);
    }

    // Assert
    auto allocationInfos = SystemGetMemoryArenaAllocationInfos(memoryArena);
    ASSERT_EQ(committedBytesBefore + pageSizeInBytes, allocationInfos.CommittedBytes);

    for (int32_t i = 0; i < threadCount; i++)
    {
        for (size_t j = 0; j < rangeSizeInBytes; j++)
        {
            ASSERT_EQ((uint8_t)(i + 1), buffer[i * rangeSizeInBytes + j]);
        }
    }
}

UTEST(MemoryConcurrent, ArenaAllocationAccounting)
{
    // Arrange
    const int32_t threadCount = 16;
    auto pageSizeInBytes = SystemPlatformGetPageSize();
    auto allocationInfosBefore = SystemGetAllocationInfos();
    MemoryArena memoryArenas[threadCount] = {};
    SystemThread threads[threadCount];
    MemoryConcurrentArenaAllocationParameter threadParameters[threadCount];

    for (int32_t i = 0; i < threadCount; i++)
    {
        threadParameters[i] = { &memoryArenas[i], pageSizeInBytes };
        threads[i] = SystemCreateThread(MemoryConcurrentArenaAllocationFunction, &threadParameters[i]);
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
        ASSERT_TRUE(memoryArenas[i].Storage != nullptr);
    }

    auto allocationInfosAfterAllocate = SystemGetAllocationInfos();
    ASSERT_EQ(allocationInfosBefore.ReservedBytes + threadCount * pageSizeInBytes * 2, allocationInfosAfterAllocate.ReservedBytes);
    ASSERT_EQ(allocationInfosBefore.CommittedBytes + threadCount * pageSizeInBytes, allocationInfosAfterAllocate.CommittedBytes);

    for (int32_t i = 0; i < threadCount; i++)
    {
        SystemFreeMemoryArena(memoryArenas[i]);
    }

    auto allocationInfosAfterFree = SystemGetAllocationInfos();
    ASSERT_EQ(allocationInfosBefore.ReservedBytes, allocationInfosAfterFree.ReservedBytes);
    ASSERT_EQ(allocationInfosBefore.CommittedBytes, allocationInfosAfterFree.CommittedBytes);
}
