#include "SystemFunctions.h"
#include "SystemMemory.h"
#include "utest.h"

struct MemoryThreadParameter
{
    MemoryArena MemoryArena;
    int32_t ThreadId;
    int32_t ItemCount;
};

void MemoryConcurrentAddFunction(void* parameter)
{
    auto threadParameter = (MemoryThreadParameter*)parameter;

    for (int32_t i = 0; i < threadParameter->ItemCount; i++)
    {
        SystemPushMemoryZero(threadParameter->MemoryArena, 64);
    }
}

void MemoryConcurrentPopFunction(void* parameter)
{
    auto threadParameter = (MemoryThreadParameter*)parameter;

    for (int32_t i = 0; i < threadParameter->ItemCount; i++)
    {
        SystemPopMemory(threadParameter->MemoryArena, 64);
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
    SystemPopMemory(memoryArena, 20000);

    // Assert
    auto allocationInfos = SystemGetMemoryArenaAllocationInfos(memoryArena);
    ASSERT_EQ(dataSizeInBytes + 1024 - 20000, allocationInfos.AllocatedBytes);
    ASSERT_GT(allocationInfos.CommittedBytes, allocationInfos.AllocatedBytes);
}

UTEST(Memory, AllocatePop) 
{
    // Arrange
    auto memoryArena = SystemAllocateMemoryArena();
    auto dataSizeInBytes = 64llu;
    
    // Act
    SystemPushArrayZero<uint8_t>(memoryArena, dataSizeInBytes); 
    SystemPopMemory(memoryArena, dataSizeInBytes);

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
        SystemPushMemory(stackMemoryArena1, 10000);
        SystemConcatBuffers<char>(stackMemoryArena2, "Test", "Stack2");

        {
            auto stackMemoryArena3 = SystemGetStackMemoryArena();
            string5 = SystemConcatBuffers<char>(stackMemoryArena1, "Test4", "Stack1");
        }
    }

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

UTEST(Memory, ConcurrentPop) 
{
    // Arrange
    const int32_t itemCount = 80000;
    const int32_t threadCount = 32;
    auto maxSize = (size_t)itemCount * 64;
    auto memoryArena = SystemAllocateMemoryArena(maxSize);
    SystemPushMemory(memoryArena, maxSize);
    
    // Act
    SystemThread threads[threadCount];
    MemoryThreadParameter threadParameters[threadCount];

    for (int32_t i = 0; i < threadCount; i++)
    {
        threadParameters[i] = { memoryArena, i, itemCount / threadCount };
        threads[i] = SystemCreateThread(MemoryConcurrentPopFunction, &threadParameters[i]);
    }

    for (int32_t i = 0; i < threadCount; i++)
    {
        SystemWaitThread(threads[i]);
        SystemFreeThread(threads[i]);
    }

    // Assert
    auto allocationInfos = SystemGetMemoryArenaAllocationInfos(memoryArena);
    ASSERT_EQ(0llu, allocationInfos.AllocatedBytes);
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
    ASSERT_LT(allocationInfos.CommittedBytes, allocationInfos.AllocatedBytes);
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
        array[offset - bufferSize + i] = i % 256;
    }

    SystemCommitMemory(memoryArena, array.Pointer + offset2, bufferSize);

    for (size_t i = 0; i < bufferSize; i++)
    {
        array[offset2 - bufferSize + i] = i % 256;
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
        array[offset - bufferSize + i] = i % 256;
    }

    SystemCommitMemory(memoryArena, array.Pointer + offset2, bufferSize);

    for (size_t i = 0; i < bufferSize; i++)
    {
        array[offset2 - bufferSize + i] = i % 256;
    }

    // Act
    SystemDecommitMemory(memoryArena, array.Pointer + offset, (offset2 - offset) + bufferSize * 2);

    // Assert
    auto allocationInfos = SystemGetMemoryArenaAllocationInfos(memoryArena);
    ASSERT_EQ(dataSizeInBytes, allocationInfos.AllocatedBytes);
    ASSERT_EQ(maxSizeInBytes, allocationInfos.MaximumSizeInBytes);
    ASSERT_LT(allocationInfos.CommittedBytes, allocationInfos.AllocatedBytes);
}
