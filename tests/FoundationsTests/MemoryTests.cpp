#include "SystemFunctions.h"
#include "SystemMemory.h"
#include "SystemPlatformFunctions.h"
#include "utest.h"

struct alignas(64) MemoryAlignedTestData
{
    uint8_t Data[64];
};

bool MemoryCheckStackMemoryArenaNestingLimit(uint32_t remainingLevels)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    if (stackMemoryArena.Arena.Storage == nullptr)
    {
        return false;
    }

    if (remainingLevels == 1)
    {
        auto overflowStackMemoryArena = SystemGetStackMemoryArena();
        return overflowStackMemoryArena.Arena.Storage == nullptr;
    }

    return MemoryCheckStackMemoryArenaNestingLimit(remainingLevels - 1);
}

UTEST(Memory, Allocate)
{
    // Arrange
    auto memoryArena = SystemAllocateMemoryArena();
    auto dataSizeInBytes = 70024llu;

    // Act
    auto data = SystemPushArrayZero<uint8_t>(memoryArena, dataSizeInBytes);

    // Assert
    auto allocationInfos = SystemGetMemoryArenaAllocationInfos(memoryArena);
    ASSERT_EQ_MSG(dataSizeInBytes, allocationInfos.AllocatedBytes, "MemoryArena allocated byte count should match the requested allocation size.");
    ASSERT_EQ_MSG(dataSizeInBytes, data.Length, "Allocated array length should match the requested element count.");
}

UTEST(Memory, AllocateMultiple)
{
    // Arrange
    auto memoryArena = SystemAllocateMemoryArena();
    auto dataSizeInBytes = 70024llu;

    // Act
    SystemPushArrayZero<uint8_t>(memoryArena, dataSizeInBytes);
    SystemPushArrayZero<uint8_t>(memoryArena, 1024);

    // Assert
    auto allocationInfos = SystemGetMemoryArenaAllocationInfos(memoryArena);
    ASSERT_EQ_MSG(dataSizeInBytes + 1024, allocationInfos.AllocatedBytes, "MemoryArena allocated byte count should include every pushed allocation.");
    ASSERT_GT_MSG(allocationInfos.CommittedBytes, allocationInfos.AllocatedBytes, "Committed memory should include page granularity and arena metadata overhead.");
}

UTEST(Memory, ClearMemoryArena)
{
    // Arrange
    auto memoryArena = SystemAllocateMemoryArena();
    auto dataSizeInBytes = 70024llu;

    SystemPushArrayZero<uint8_t>(memoryArena, dataSizeInBytes);
    SystemPushArrayZero<uint8_t>(memoryArena, 1024);

    // Act
    SystemClearMemoryArena(memoryArena);

    // Assert
    auto allocationInfos = SystemGetMemoryArenaAllocationInfos(memoryArena);
    ASSERT_EQ_MSG(0llu, allocationInfos.AllocatedBytes, "Clearing a MemoryArena should reset its logical allocated byte count to zero.");
}

UTEST(Memory, ClearStackMemoryArenaIsIgnored)
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto allocation = SystemPushArrayZero<uint8_t>(stackMemoryArena, 64);
    allocation[0] = 123;
    auto allocationInfosBefore = SystemGetMemoryArenaAllocationInfos(stackMemoryArena);

    // Act
    SystemClearMemoryArena(stackMemoryArena);

    // Assert
    auto allocationInfosAfter = SystemGetMemoryArenaAllocationInfos(stackMemoryArena);
    ASSERT_EQ_MSG(allocationInfosBefore.AllocatedBytes, allocationInfosAfter.AllocatedBytes, "Clearing a StackMemoryArena handle should not change its scoped allocation state.");
    ASSERT_EQ_MSG(123, allocation[0], "Clearing a StackMemoryArena handle should not invalidate existing scoped allocations.");

    auto secondAllocation = SystemPushMemory(stackMemoryArena, 64);
    ASSERT_TRUE_MSG(secondAllocation != nullptr, "StackMemoryArena should remain usable after an ignored explicit clear request.");
}

UTEST(Memory, ClearMemoryArenaResetsPartialCommitTracking)
{
    // Arrange
    auto pageSizeInBytes = SystemPlatformGetPageSize();
    auto memoryArena = SystemAllocateMemoryArena(pageSizeInBytes);
    auto committedBytesBefore = SystemGetMemoryArenaAllocationInfos(memoryArena).CommittedBytes;
    auto buffer = SystemPushArray<uint8_t>(memoryArena, pageSizeInBytes, AllocationState_Reserved);
    auto commitSucceeded = SystemCommitMemory(memoryArena, buffer.Pointer + 64, 64);

    ASSERT_TRUE_MSG(commitSucceeded, "Partial MemoryArena commit should succeed before testing clear behavior.");
    ASSERT_EQ_MSG(committedBytesBefore + pageSizeInBytes, SystemGetMemoryArenaAllocationInfos(memoryArena).CommittedBytes, "Partial commit should commit exactly one data page.");

    // Act
    SystemClearMemoryArena(memoryArena);

    // Assert
    auto allocationInfosAfterClear = SystemGetMemoryArenaAllocationInfos(memoryArena);
    ASSERT_EQ_MSG(0llu, allocationInfosAfterClear.AllocatedBytes, "Clearing an arena with a partial commit should reset logical allocation state.");
    ASSERT_EQ_MSG(committedBytesBefore, allocationInfosAfterClear.CommittedBytes, "Clearing an arena should decommit partially tracked data pages.");

    auto reusedBuffer = SystemPushArray<uint8_t>(memoryArena, pageSizeInBytes, AllocationState_Reserved);
    ASSERT_TRUE_MSG(SystemCommitMemory(memoryArena, reusedBuffer.Pointer + 128, 64), "MemoryArena should allow a new partial commit after clear resets page tracking.");
    SystemDecommitMemory(memoryArena, reusedBuffer.Pointer + 128, 64);
    ASSERT_EQ_MSG(committedBytesBefore, SystemGetMemoryArenaAllocationInfos(memoryArena).CommittedBytes, "Page tracking after clear should allow the reused partial range to decommit normally.");

    SystemFreeMemoryArena(memoryArena);
}

UTEST(Memory, AllocateCheckAlignment)
{
    // Arrange
    auto memoryArena = SystemAllocateMemoryArena();
    auto dataSizeInBytes = 70024llu;
    auto alignment = 8llu;

    // Act
    SystemPushArrayZero<uint8_t>(memoryArena, 455);
    auto data = SystemPushArrayZero<uint8_t>(memoryArena, dataSizeInBytes);

    // Assert
    ASSERT_TRUE_MSG(((size_t)data.Pointer & (alignment - 1)) == 0, "MemoryArena allocations should respect the default alignment.");
}

UTEST(Memory, TypedAllocationsRespectAlignment)
{
    // Arrange
    auto memoryArena = SystemAllocateMemoryArena(1024);
    SystemPushMemory(memoryArena, 8);

    // Act
    auto array = SystemPushArray<MemoryAlignedTestData>(memoryArena, 2);
    SystemPushMemory(memoryArena, 8);
    auto structure = SystemPushStruct<MemoryAlignedTestData>(memoryArena);
    SystemPushMemory(memoryArena, 8);
    auto zeroArray = SystemPushArrayZero<MemoryAlignedTestData>(memoryArena, 1);

    // Assert
    ASSERT_TRUE_MSG(array.Pointer != nullptr, "Aligned array allocation should succeed.");
    ASSERT_TRUE_MSG(((size_t)array.Pointer & (alignof(MemoryAlignedTestData) - 1)) == 0, "SystemPushArray should align returned storage to alignof(T).");
    ASSERT_TRUE_MSG(structure != nullptr, "Aligned structure allocation should succeed.");
    ASSERT_TRUE_MSG(((size_t)structure & (alignof(MemoryAlignedTestData) - 1)) == 0, "SystemPushStruct should align returned storage to alignof(T).");
    ASSERT_TRUE_MSG(zeroArray.Pointer != nullptr, "Zeroed aligned array allocation should succeed.");
    ASSERT_TRUE_MSG(((size_t)zeroArray.Pointer & (alignof(MemoryAlignedTestData) - 1)) == 0, "SystemPushArrayZero should align returned storage to alignof(T).");

    SystemFreeMemoryArena(memoryArena);
}

UTEST(Memory, PushOverflowReturnsNull)
{
    // Arrange
    auto memoryArena = SystemAllocateMemoryArena(64);
    auto allocation = SystemPushMemory(memoryArena, 64, AllocationState_Reserved);

    // Act
    auto overflowAllocation = SystemPushMemory(memoryArena, 8, AllocationState_Reserved);
    auto zeroOverflowAllocation = SystemPushMemoryZero(memoryArena, 8);
    auto overflowArray = SystemPushArray<uint64_t>(memoryArena, 2, AllocationState_Reserved);

    // Assert
    ASSERT_TRUE_MSG(allocation != nullptr, "An allocation that exactly fills the MemoryArena should succeed.");
    ASSERT_TRUE_MSG(overflowAllocation == nullptr, "MemoryArena push beyond capacity should return nullptr.");
    ASSERT_TRUE_MSG(zeroOverflowAllocation == nullptr, "Zero-initialized push beyond MemoryArena capacity should return nullptr.");
    ASSERT_TRUE_MSG(overflowArray.Pointer == nullptr, "Array push beyond MemoryArena capacity should return an empty Span.");
    ASSERT_EQ_MSG(0llu, overflowArray.Length, "Failed array allocation should return a zero-length Span.");

    auto allocationInfos = SystemGetMemoryArenaAllocationInfos(memoryArena);
    ASSERT_EQ_MSG(64llu, allocationInfos.AllocatedBytes, "Failed pushes should not advance the MemoryArena beyond capacity.");
}

UTEST(Memory, ArenaSizeOverflowReturnsEmptyHandle)
{
    // Act
    auto memoryArena = SystemAllocateMemoryArena(SIZE_MAX);

    // Assert
    ASSERT_TRUE_MSG(memoryArena.Storage == nullptr, "MemoryArena allocation should fail when the requested capacity overflows internal size calculations.");
}

UTEST(Memory, PushSizeOverflowDoesNotAdvanceArena)
{
    // Arrange
    auto memoryArena = SystemAllocateMemoryArena(64);

    // Act
    auto allocation = SystemPushMemory(memoryArena, SIZE_MAX, AllocationState_Reserved);
    auto array = SystemPushArray<uint64_t>(memoryArena, SIZE_MAX / sizeof(uint64_t) + 1, AllocationState_Reserved);

    // Assert
    ASSERT_TRUE_MSG(allocation == nullptr, "Memory push should reject a size that overflows alignment or range calculations.");
    ASSERT_TRUE_MSG(array.Pointer == nullptr, "Array push should reject an element count whose byte size overflows.");
    ASSERT_EQ_MSG(0llu, array.Length, "Overflowing array allocation should return a zero-length Span.");
    ASSERT_EQ_MSG(0llu, SystemGetMemoryArenaAllocationInfos(memoryArena).AllocatedBytes, "Rejected overflow allocations should not advance the MemoryArena.");
}

UTEST(Memory, CommitReportsInvalidRange)
{
    // Arrange
    auto memoryArena = SystemAllocateMemoryArena(64);
    auto allocation = SystemPushArray<uint8_t>(memoryArena, 64, AllocationState_Reserved);

    // Act
    auto validCommit = SystemCommitMemory(memoryArena, allocation.Pointer, allocation.Length, true);
    auto invalidCommit = SystemCommitMemory(memoryArena, allocation.Pointer + allocation.Length, 8);

    // Assert
    ASSERT_TRUE_MSG(validCommit, "Committing a valid reserved MemoryArena range should succeed.");
    ASSERT_FALSE_MSG(invalidCommit, "Committing a range outside the MemoryArena allocation should fail.");

    for (size_t i = 0; i < allocation.Length; i++)
    {
        ASSERT_EQ_MSG(0, allocation[i], "Commit with clearMemory should zero newly committed memory.");
    }

    SystemFreeMemoryArena(memoryArena);
}

UTEST(Memory, ConcatBuffers)
{
    // Arrange
    auto memoryArena = SystemAllocateMemoryArena(1024);

    // Act
    auto result = SystemConcatBuffers<char>(memoryArena, "Test1", "Test2");

    // Assert
    ASSERT_STREQ_MSG("Test1Test2", result.Pointer, "Concatenated character buffer content is invalid.");
}

UTEST(Memory, StackMemoryArena)
{
    // Arrange
    auto stackMemoryArena1 = SystemGetStackMemoryArena();
    auto string1 = SystemConcatBuffers<char>(stackMemoryArena1, "Test", "Stack1");
    ReadOnlySpan<char> string2;
    ReadOnlySpan<char> string5;

    // Act
    {
        auto stackMemoryArena2 = SystemGetStackMemoryArena();

        string2 = SystemConcatBuffers<char>(stackMemoryArena1, "Test2", "Stack1");
        SystemConcatBuffers<char>(stackMemoryArena2, "Test", "Stack2");

        {
            auto stackMemoryArena3 = SystemGetStackMemoryArena();
            SystemPushMemory(stackMemoryArena2, 10000);
            SystemPushMemory(stackMemoryArena3, 50000);
            string5 = SystemConcatBuffers<char>(stackMemoryArena1, "Test4", "Stack1");
            SystemPushMemory(stackMemoryArena2, 10000);

            {
                auto stackMemoryArena5 = SystemGetStackMemoryArena();

                SystemPushMemory(stackMemoryArena5, 2000);

                {
                    auto stackMemoryArena6 = SystemGetStackMemoryArena();

                    SystemPushMemory(stackMemoryArena5, 2000);
                    SystemPushMemory(stackMemoryArena2, 2000);
                    SystemPushMemory(stackMemoryArena2, 50000);
                }
            }
        }

        {
            auto stackMemoryArena4 = SystemGetStackMemoryArena();
        }

        SystemPushMemory(stackMemoryArena2, 15000);
    }

    SystemPushMemory(stackMemoryArena1, 400);
    auto string4 = SystemConcatBuffers<char>(stackMemoryArena1, "Test3", "Stack1");

    // Assert
    ASSERT_STREQ_MSG("TestStack1", string1.Pointer, "Root stack-lifetime allocation should survive nested stack scopes.");
    ASSERT_STREQ_MSG("Test2Stack1", string2.Pointer, "Allocation made through an ancestor stack arena should survive younger scopes.");
    ASSERT_STREQ_MSG("Test3Stack1", string4.Pointer, "Root stack arena should remain usable after nested scopes are released.");
    ASSERT_STREQ_MSG("Test4Stack1", string5.Pointer, "Deep ancestor allocation should preserve the ancestor stack lifetime.");
}

UTEST(Memory, StackMemoryArenaRelease)
{
    // Arrange
    auto stackMemoryArena1 = SystemGetStackMemoryArena();
    auto string1 = SystemConcatBuffers<char>(stackMemoryArena1, "Test", "Stack1");
    ReadOnlySpan<char> string2;
    ReadOnlySpan<char> string3;
    ReadOnlySpan<char> string4;
    ReadOnlySpan<char> string5;

    // Act
    {
        auto stackMemoryArena2 = SystemGetStackMemoryArena();
        string2 = SystemConcatBuffers<char>(stackMemoryArena1, "Test2", "Stack1");

        {
            auto stackMemoryArena3 = SystemGetStackMemoryArena();
            SystemConcatBuffers<char>(stackMemoryArena2, "Test", "Stack2");
        }

        {
            auto memoryArenaPointer = (MemoryArena)stackMemoryArena1;
            auto stackMemoryArena4 = SystemGetStackMemoryArena();
            {
                auto stackMemoryArena3 = SystemGetStackMemoryArena();
                SystemConcatBuffers<char>(stackMemoryArena3, "Test", "Stack2");
                SystemConcatBuffers<char>(stackMemoryArena3, "Test", "Stack2");
                SystemConcatBuffers<char>(stackMemoryArena3, "Test", "Stack2");
            }

            SystemConcatBuffers<char>(stackMemoryArena4, "Test", "Stack2");
            string5 = SystemConcatBuffers<char>(memoryArenaPointer, "Test5", "Stack1");
        }

        SystemConcatBuffers<char>(stackMemoryArena2, "Test2", "Stack2");
        string3 = SystemConcatBuffers<char>(stackMemoryArena1, "Test3", "Stack1");
    }

    {
        auto stackMemoryArena3 = SystemGetStackMemoryArena();
        string4 = SystemConcatBuffers<char>(stackMemoryArena1, "Test4", "Stack1");
    }

    // Assert
    ASSERT_STREQ_MSG("TestStack1", string1.Pointer, "Root stack allocation should survive every nested rollback.");
    ASSERT_STREQ_MSG("Test2Stack1", string2.Pointer, "Ancestor allocation should survive the scope in which it was requested.");
    ASSERT_STREQ_MSG("Test3Stack1", string3.Pointer, "Root lifetime allocation should remain valid after child scope release.");
    ASSERT_STREQ_MSG("Test4Stack1", string4.Pointer, "Sibling stack scopes should restore offsets without corrupting ancestor allocations.");
    ASSERT_STREQ_MSG("Test5Stack1", string5.Pointer, "Copied ancestor MemoryArena handle should preserve ancestor allocation lifetime.");
}

UTEST(Memory, StackMemoryArenaNestingLimit)
{
    // Act
    auto nestingLimitHandled = MemoryCheckStackMemoryArenaNestingLimit(UINT8_MAX);

    // Assert
    ASSERT_TRUE_MSG(nestingLimitHandled, "The 256th nested StackMemoryArena scope should fail instead of wrapping the lifetime level.");

    auto stackMemoryArena = SystemGetStackMemoryArena();
    ASSERT_TRUE_MSG(stackMemoryArena.Arena.Storage != nullptr, "StackMemoryArena should remain usable after maximum nesting scopes unwind.");
}

UTEST(Memory, StackAncestorAllocationUsesExtraStorageCapacity)
{
    // Arrange
    auto stackMemoryArena1 = SystemGetStackMemoryArena();
    auto mainAllocation = SystemPushMemory(stackMemoryArena1, 120llu * 1024 * 1024, AllocationState_Reserved);
    void* ancestorAllocation = nullptr;

    // Act
    {
        auto stackMemoryArena2 = SystemGetStackMemoryArena();
        ancestorAllocation = SystemPushMemory(stackMemoryArena1, 16llu * 1024 * 1024, AllocationState_Reserved);
    }

    // Assert
    ASSERT_TRUE_MSG(mainAllocation != nullptr, "Large root stack allocation should fit in the main stack arena storage.");
    ASSERT_TRUE_MSG(ancestorAllocation != nullptr, "Ancestor stack allocation should use extra storage when main stack storage is exhausted.");
}

UTEST(Memory, AllocateReserved)
{
    // Arrange
    auto memoryArena = SystemAllocateMemoryArena();
    auto dataSizeInBytes = 70024llu;

    // Act
    SystemPushArray<uint8_t>(memoryArena, dataSizeInBytes, AllocationState_Reserved);

    // Assert
    auto allocationInfos = SystemGetMemoryArenaAllocationInfos(memoryArena);
    ASSERT_EQ_MSG(dataSizeInBytes, allocationInfos.AllocatedBytes, "Reserved MemoryArena allocation should advance the logical allocated byte count.");
    ASSERT_LT_MSG(allocationInfos.CommittedBytes, allocationInfos.MaximumSizeInBytes, "Reserved allocation should not commit the entire MemoryArena capacity.");
}

UTEST(Memory, AllocateReservedCommit)
{
    // Arrange
    auto maxSizeInBytes = 4000000llu;
    auto dataSizeInBytes = 2000000llu;
    auto offset = 150000llu;
    auto offset2 = 160000llu;
    auto bufferSize = 1024llu;
    auto memoryArena = SystemAllocateMemoryArena(maxSizeInBytes);

    // Act
    auto array = SystemPushArray<uint8_t>(memoryArena, dataSizeInBytes, AllocationState_Reserved);
    SystemCommitMemory(memoryArena, array.Pointer + offset, bufferSize);

    for (size_t i = 0; i < bufferSize; i++)
    {
        array[offset + i] = i % 256;
    }

    SystemCommitMemory(memoryArena, array.Pointer + offset2, bufferSize);

    for (size_t i = 0; i < bufferSize; i++)
    {
        array[offset2 + i] = i % 256;
    }

    // Assert
    auto allocationInfos = SystemGetMemoryArenaAllocationInfos(memoryArena);
    ASSERT_EQ_MSG(dataSizeInBytes, allocationInfos.AllocatedBytes, "Committing reserved ranges should not change the logical allocation size.");
    ASSERT_EQ_MSG(maxSizeInBytes, allocationInfos.MaximumSizeInBytes, "MemoryArena maximum data capacity should remain unchanged after commits.");
    ASSERT_LT_MSG(allocationInfos.CommittedBytes, allocationInfos.AllocatedBytes, "Committing small subranges should not commit the entire reserved allocation.");
}

UTEST(Memory, AllocateReservedDecommit)
{
    // Arrange
    auto maxSizeInBytes = 4000000llu;
    auto dataSizeInBytes = 2000000llu;
    auto offset = 150000llu;
    auto offset2 = 160000llu;
    auto bufferSize = 1024llu;
    auto memoryArena = SystemAllocateMemoryArena(maxSizeInBytes);
    auto array = SystemPushArray<uint8_t>(memoryArena, dataSizeInBytes, AllocationState_Reserved);
    SystemCommitMemory(memoryArena, array.Pointer + offset, bufferSize);

    for (size_t i = 0; i < bufferSize; i++)
    {
        array[offset + i] = i % 256;
    }

    SystemCommitMemory(memoryArena, array.Pointer + offset2, bufferSize);

    for (size_t i = 0; i < bufferSize; i++)
    {
        array[offset2 + i] = i % 256;
    }

    // Act
    SystemDecommitMemory(memoryArena, array.Pointer + offset, (offset2 - offset) + bufferSize * 2);

    // Assert
    auto allocationInfos = SystemGetMemoryArenaAllocationInfos(memoryArena);
    ASSERT_EQ_MSG(dataSizeInBytes, allocationInfos.AllocatedBytes, "Decommitting memory should not release the logical MemoryArena allocation.");
    ASSERT_EQ_MSG(maxSizeInBytes, allocationInfos.MaximumSizeInBytes, "Decommitting memory should not change MemoryArena capacity.");
    ASSERT_LT_MSG(allocationInfos.CommittedBytes, allocationInfos.AllocatedBytes, "Decommitting reserved ranges should leave only the required committed pages.");
}
