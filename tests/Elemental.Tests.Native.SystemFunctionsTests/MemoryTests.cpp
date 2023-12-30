#include "SystemMemory.h"
#include "utest.h"

// TODO: Write thread safe tests!!!!

UTEST(Memory, Allocate) 
{
    // Arrange
    auto memoryArena = SystemAllocateMemoryArena();
    auto dataSizeInBytes = 1024zu;
    
    // Act
    auto data = SystemPushArrayZero<uint8_t>(memoryArena, dataSizeInBytes); 

    // Assert
    ASSERT_EQ(dataSizeInBytes, SystemGetMemoryArenaAllocatedBytes(memoryArena));
    ASSERT_EQ(dataSizeInBytes, data.Length);
}
