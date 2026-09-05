#include "SystemFunctions.h"
#include "SystemMemory.h"
#include "SystemPlatformFunctions.h"
#include "utest.h"

struct MemoryThreadParameter
{
    MemoryArena MemoryArena;
    int32_t ThreadId;
    int32_t ItemCount;
};

struct MemoryConcurrentOverflowThreadParameter
{
    MemoryArena MemoryArena;
    bool* Start;
    void** Results;
    int32_t ThreadId;
};

struct MemoryConcurrentCommitThreadParameter
{
    MemoryArena MemoryArena;
    uint8_t* Pointer;
    size_t SizeInBytes;
    uint8_t Value;
};

void MemoryConcurrentAddFunction(void* parameter)
{
    auto threadParameter = (MemoryThreadParameter*)parameter;

    for (int32_t i = 0; i < threadParameter->ItemCount; i++)
    {
        SystemPushMemoryZero(threadParameter->MemoryArena, 64);
    }
}

void MemoryConcurrentOverflowFunction(void* parameter)
{
    auto threadParameter = (MemoryConcurrentOverflowThreadParameter*)parameter;
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
    auto threadParameter = (MemoryConcurrentCommitThreadParameter*)parameter;
    SystemCommitMemory(threadParameter->MemoryArena, threadParameter->Pointer, threadParameter->SizeInBytes);

    for (size_t i = 0; i < threadParameter->SizeInBytes; i++)
    {
        threadParameter->Pointer[i] = threadParameter->Value;
    }
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

UTEST(Memory, AllocateCheckAlignement) 
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

UTEST(Memory, ConcurrentPush) 
{
    // Arrange
    const int32_t itemCount = 80000;
    const int32_t threadCount = 32;
    auto maxSize = (size_t)itemCount * 64;
    auto memoryArena = SystemAllocateMemoryArena(maxSize);
    
    // Act
    SystemThread threads[threadCount];
    MemoryThreadParameter threadParameters[threadCount];

    for (int32_t i = 0; i < threadCount; i++)
    {
        threadParameters[i] = { memoryArena, i, itemCount / threadCount };
        threads[i] = SystemCreateThread(MemoryConcurrentAddFunction, &threadParameters[i]);
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

UTEST(Memory, ConcurrentPushDoesNotOverflow)
{
    // Arrange
    const int32_t threadCount = 32;
    const int32_t capacityCount = 8;
    const size_t allocationSizeInBytes = 64;
    auto memoryArena = SystemAllocateMemoryArena(capacityCount * allocationSizeInBytes);
    bool start = false;
    void* results[threadCount] = {};
    SystemThread threads[threadCount];
    MemoryConcurrentOverflowThreadParameter threadParameters[threadCount];

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

UTEST(Memory, ConcurrentCommitSharedPage)
{
    // Arrange
    const int32_t threadCount = 32;
    const size_t rangeSizeInBytes = 64;
    auto pageSizeInBytes = SystemPlatformGetPageSize();
    auto memoryArena = SystemAllocateMemoryArena(pageSizeInBytes);
    auto buffer = SystemPushArray<uint8_t>(memoryArena, pageSizeInBytes, AllocationState_Reserved);
    auto committedBytesBefore = SystemGetMemoryArenaAllocationInfos(memoryArena).CommittedBytes;
    SystemThread threads[threadCount];
    MemoryConcurrentCommitThreadParameter threadParameters[threadCount];

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
