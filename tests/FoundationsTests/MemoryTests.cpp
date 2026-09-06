#include "SystemFunctions.h"
#include "SystemMemory.h"
#include "SystemPlatformFunctions.h"
#include "utest.h"

UTEST(Memory, Allocate)
{
    // Arrange
    auto memoryArena = SystemAllocateMemoryArena();
    auto dataSizeInBytes = 70024llu;

    // Act
    auto data = SystemPushArrayZero<uint8_t>(memoryArena, dataSizeInBytes);

    // Assert
    auto allocationInfos = SystemGetMemoryArenaAllocationInfos(memoryArena);
    ASSERT_EQ(dataSizeInBytes, allocationInfos.AllocatedBytes);
    ASSERT_EQ(dataSizeInBytes, data.Length);
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
    ASSERT_EQ(dataSizeInBytes + 1024, allocationInfos.AllocatedBytes);
    ASSERT_GT(allocationInfos.CommittedBytes, allocationInfos.AllocatedBytes);
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
    ASSERT_EQ(0llu, allocationInfos.AllocatedBytes);
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
    ASSERT_TRUE(((size_t)data.Pointer & (alignment - 1)) == 0);
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
    ASSERT_TRUE(allocation != nullptr);
    ASSERT_TRUE(overflowAllocation == nullptr);
    ASSERT_TRUE(zeroOverflowAllocation == nullptr);
    ASSERT_TRUE(overflowArray.Pointer == nullptr);
    ASSERT_EQ(0llu, overflowArray.Length);

    auto allocationInfos = SystemGetMemoryArenaAllocationInfos(memoryArena);
    ASSERT_EQ(64llu, allocationInfos.AllocatedBytes);
}

UTEST(Memory, ArenaSizeOverflowReturnsEmptyHandle)
{
    // Act
    auto memoryArena = SystemAllocateMemoryArena(SIZE_MAX);

    // Assert
    ASSERT_TRUE(memoryArena.Storage == nullptr);
}

UTEST(Memory, PushSizeOverflowDoesNotAdvanceArena)
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

UTEST(Memory, CommitReportsInvalidRange)
{
    // Arrange
    auto memoryArena = SystemAllocateMemoryArena(64);
    auto allocation = SystemPushArray<uint8_t>(memoryArena, 64, AllocationState_Reserved);

    // Act
    auto validCommit = SystemCommitMemory(memoryArena, allocation.Pointer, allocation.Length, true);
    auto invalidCommit = SystemCommitMemory(memoryArena, allocation.Pointer + allocation.Length, 8);

    // Assert
    ASSERT_TRUE(validCommit);
    ASSERT_FALSE(invalidCommit);

    for (size_t i = 0; i < allocation.Length; i++)
    {
        ASSERT_EQ(0, allocation[i]);
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
    ASSERT_STREQ("Test1Test2", result.Pointer);
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
    ASSERT_STREQ("TestStack1", string1.Pointer);
    ASSERT_STREQ("Test2Stack1", string2.Pointer);
    ASSERT_STREQ("Test3Stack1", string4.Pointer);
    ASSERT_STREQ("Test4Stack1", string5.Pointer);
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
    ASSERT_STREQ("TestStack1", string1.Pointer);
    ASSERT_STREQ("Test2Stack1", string2.Pointer);
    ASSERT_STREQ("Test3Stack1", string3.Pointer);
    ASSERT_STREQ("Test4Stack1", string4.Pointer);
    ASSERT_STREQ("Test5Stack1", string5.Pointer);
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
    ASSERT_TRUE(mainAllocation != nullptr);
    ASSERT_TRUE(ancestorAllocation != nullptr);
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
    ASSERT_EQ(dataSizeInBytes, allocationInfos.AllocatedBytes);
    ASSERT_LT(allocationInfos.CommittedBytes, allocationInfos.MaximumSizeInBytes);
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
    ASSERT_EQ(dataSizeInBytes, allocationInfos.AllocatedBytes);
    ASSERT_EQ(maxSizeInBytes, allocationInfos.MaximumSizeInBytes);
    ASSERT_LT(allocationInfos.CommittedBytes, allocationInfos.AllocatedBytes);
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
    ASSERT_EQ(dataSizeInBytes, allocationInfos.AllocatedBytes);
    ASSERT_EQ(maxSizeInBytes, allocationInfos.MaximumSizeInBytes);
    ASSERT_LT(allocationInfos.CommittedBytes, allocationInfos.AllocatedBytes);
}
