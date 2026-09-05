#include "SystemFunctions.h"
#include "SystemMemory.h"
#include "SystemPlatformFunctions.h"
#include "utest.h"

struct ConcurrentArenaAllocationParameter
{
    MemoryArena* Result;
    size_t SizeInBytes;
};

void ConcurrentArenaAllocationFunction(void* parameter)
{
    auto threadParameter = (ConcurrentArenaAllocationParameter*)parameter;
    *threadParameter->Result = SystemAllocateMemoryArena(threadParameter->SizeInBytes);
}

UTEST(MemoryRobustness, ArenaSizeOverflowReturnsEmptyHandle)
{
    // Act
    auto memoryArena = SystemAllocateMemoryArena(SIZE_MAX);

    // Assert
    ASSERT_TRUE(memoryArena.Storage == nullptr);
}

UTEST(MemoryRobustness, PushSizeOverflowDoesNotAdvanceArena)
{
    // Arrange
    auto memoryArena = SystemAllocateMemoryArena(64);

    // Act
    auto allocation = SystemPushMemory(memoryArena, SIZE_MAX, AllocationState_Reserved);
    auto array = SystemPushArray<uint64_t>(memoryArena, SIZE_MAX / sizeof(uint64_t) + 1, AllocationState_Reserved);

    // Assert
    ASSERT_TRUE(allocation == nullptr);
    ASSERT_TRUE(array.Pointer == nullptr);
    ASSERT_EQ(0llu, array.Length);
    ASSERT_EQ(0llu, SystemGetMemoryArenaAllocationInfos(memoryArena).AllocatedBytes);
}

UTEST(MemoryRobustness, ConcurrentArenaAllocationAccounting)
{
    // Arrange
    const int32_t threadCount = 16;
    auto pageSizeInBytes = SystemPlatformGetPageSize();
    auto allocationInfosBefore = SystemGetAllocationInfos();
    MemoryArena memoryArenas[threadCount] = {};
    SystemThread threads[threadCount];
    ConcurrentArenaAllocationParameter threadParameters[threadCount];

    for (int32_t i = 0; i < threadCount; i++)
    {
        threadParameters[i] = { &memoryArenas[i], pageSizeInBytes };
        threads[i] = SystemCreateThread(ConcurrentArenaAllocationFunction, &threadParameters[i]);
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
    ASSERT_EQ(allocationInfosBefore.CommittedBytes + threadCount * pageSizeInBytes, allocationInfosAfterFree.CommittedBytes);
}
