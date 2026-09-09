#include "SystemFunctions.h"
#include "SystemMemory.h"
#include "SystemPlatformFunctions.h"
#include "utest.h"

struct alignas(64) MemoryConcurrentAlignedTestData
{
    uint8_t Data[64];
};

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

struct MemoryConcurrentAlignedPushParameter
{
    MemoryArena MemoryArena;
    bool* Start;
    MemoryConcurrentAlignedTestData** Result;
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

struct MemoryConcurrentStackArenaLifetimeParameter
{
    bool* Result;
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

void MemoryConcurrentAlignedPushFunction(void* parameter)
{
    auto threadParameter = (MemoryConcurrentAlignedPushParameter*)parameter;
    bool start = false;

    while (!start)
    {
        SystemAtomicLoad(*threadParameter->Start, start);

        if (!start)
        {
            SystemYieldThread();
        }
    }

    *threadParameter->Result = SystemPushStruct<MemoryConcurrentAlignedTestData>(threadParameter->MemoryArena);
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

void MemoryConcurrentStackArenaLifetimeFunction(void* parameter)
{
    auto threadParameter = (MemoryConcurrentStackArenaLifetimeParameter*)parameter;
    auto outerStackMemoryArena = SystemGetStackMemoryArena();
    auto innerStackMemoryArena = SystemGetStackMemoryArena();
    auto allocation = SystemPushMemory(outerStackMemoryArena, 64);
    *threadParameter->Result = allocation != nullptr;
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
    ASSERT_EQ_MSG(maxSize, allocationInfos.AllocatedBytes, "Concurrent MemoryArena pushes should reserve every requested byte exactly once.");
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
                    ASSERT_TRUE_MSG(results[i] != results[j], "Concurrent MemoryArena pushes must never return the same allocation address twice.");
                }
            }
        }
    }

    ASSERT_EQ_MSG(capacityCount, successCount, "Concurrent overflow attempts should stop exactly at arena capacity.");

    auto allocationInfos = SystemGetMemoryArenaAllocationInfos(memoryArena);
    ASSERT_EQ_MSG(capacityCount * allocationSizeInBytes, allocationInfos.AllocatedBytes, "Concurrent overflow attempts must not advance the MemoryArena beyond capacity.");
}

UTEST(MemoryConcurrent, TypedPushAlignment)
{
    // Arrange
    const int32_t threadCount = 32;
    auto memoryArena = SystemAllocateMemoryArena(threadCount * sizeof(MemoryConcurrentAlignedTestData) + 128);
    SystemPushMemory(memoryArena, 8);
    bool start = false;
    MemoryConcurrentAlignedTestData* results[threadCount] = {};
    SystemThread threads[threadCount];
    MemoryConcurrentAlignedPushParameter threadParameters[threadCount];

    for (int32_t i = 0; i < threadCount; i++)
    {
        threadParameters[i] = { memoryArena, &start, &results[i] };
        threads[i] = SystemCreateThread(MemoryConcurrentAlignedPushFunction, &threadParameters[i]);
    }

    // Act
    SystemAtomicStore(start, true);

    for (int32_t i = 0; i < threadCount; i++)
    {
        SystemWaitThread(threads[i]);
        SystemFreeThread(threads[i]);
    }

    // Assert
    for (int32_t i = 0; i < threadCount; i++)
    {
        ASSERT_TRUE_MSG(results[i] != nullptr, "Concurrent aligned typed allocation should succeed for every thread.");
        ASSERT_TRUE_MSG(((size_t)results[i] & (alignof(MemoryConcurrentAlignedTestData) - 1)) == 0, "Concurrent SystemPushStruct allocations should preserve alignof(T).");

        for (int32_t j = i + 1; j < threadCount; j++)
        {
            ASSERT_TRUE_MSG(results[i] != results[j], "Concurrent aligned typed allocations must not reuse the same address.");
        }
    }

    SystemFreeMemoryArena(memoryArena);
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
    ASSERT_EQ_MSG(committedBytesBefore + pageSizeInBytes, allocationInfos.CommittedBytes, "Concurrent commits within one data page should commit that page exactly once.");

    for (int32_t i = 0; i < threadCount; i++)
    {
        for (size_t j = 0; j < rangeSizeInBytes; j++)
        {
            ASSERT_EQ_MSG((uint8_t)(i + 1), buffer[i * rangeSizeInBytes + j], "Concurrent shared-page commits should preserve each thread's written range.");
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
        ASSERT_TRUE_MSG(memoryArenas[i].Storage != nullptr, "Concurrent MemoryArena creation should return valid storage for every thread.");
    }

    auto allocationInfosAfterAllocate = SystemGetAllocationInfos();
    ASSERT_EQ_MSG(allocationInfosBefore.ReservedBytes + threadCount * pageSizeInBytes * 2, allocationInfosAfterAllocate.ReservedBytes, "Concurrent MemoryArena creation should update reserved-byte accounting exactly once per arena.");
    ASSERT_EQ_MSG(allocationInfosBefore.CommittedBytes + threadCount * pageSizeInBytes, allocationInfosAfterAllocate.CommittedBytes, "Concurrent MemoryArena creation should account for each committed header exactly once.");

    for (int32_t i = 0; i < threadCount; i++)
    {
        SystemFreeMemoryArena(memoryArenas[i]);
    }

    auto allocationInfosAfterFree = SystemGetAllocationInfos();
    ASSERT_EQ_MSG(allocationInfosBefore.ReservedBytes, allocationInfosAfterFree.ReservedBytes, "Freeing concurrently created arenas should restore reserved-byte accounting to the baseline.");
    ASSERT_EQ_MSG(allocationInfosBefore.CommittedBytes, allocationInfosAfterFree.CommittedBytes, "Freeing concurrently created arenas should restore committed-byte accounting to the baseline.");
}

UTEST(MemoryConcurrent, StackArenaThreadLifetime)
{
    // Arrange
    const int32_t threadCount = 16;
    auto allocationInfosBefore = SystemGetAllocationInfos();
    bool results[threadCount] = {};
    SystemThread threads[threadCount];
    MemoryConcurrentStackArenaLifetimeParameter threadParameters[threadCount];

    for (int32_t i = 0; i < threadCount; i++)
    {
        threadParameters[i] = { &results[i] };
        threads[i] = SystemCreateThread(MemoryConcurrentStackArenaLifetimeFunction, &threadParameters[i]);
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
        ASSERT_TRUE_MSG(results[i], "Each short-lived thread should allocate through its nested StackMemoryArena successfully.");
    }

    auto allocationInfosAfter = SystemGetAllocationInfos();
    ASSERT_EQ_MSG(allocationInfosBefore.ReservedBytes, allocationInfosAfter.ReservedBytes, "Thread exit should release both the primary and extra StackMemoryArena reservations.");
    ASSERT_EQ_MSG(allocationInfosBefore.CommittedBytes, allocationInfosAfter.CommittedBytes, "Thread exit should release all StackMemoryArena committed backing storage.");
}
