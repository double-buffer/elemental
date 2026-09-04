#include "SystemFunctions.h"
#include "utest.h"

UTEST(LibraryProcess, SystemExecuteProcess) 
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    
    // Act
    auto result = SystemExecuteProcess(stackMemoryArena, "ping");

    // Assert
    ASSERT_GT((int32_t)result.Length, 0);
}
