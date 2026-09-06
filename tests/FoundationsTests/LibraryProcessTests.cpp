#include "SystemFunctions.h"
#include "utest.h"

UTEST(LibraryProcess, SystemExecuteProcess)
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();

    // Act
    auto result = SystemExecuteProcess(stackMemoryArena, "ping");

    // Assert
    ASSERT_GT_MSG((int32_t)result.Length, 0, "SystemExecuteProcess should return non-empty process output.");
}
