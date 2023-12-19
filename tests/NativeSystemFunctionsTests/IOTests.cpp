#include "SystemFunctions.h"
#include "utest.h"

UTEST(IOFunctions, GeneralIO) 
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto fileContent = ReadOnlySpan<char>("Test File");
    
    // Act
    auto fileName = SystemGenerateTempFilename(stackMemoryArena, "TestFile");
    SystemFileWriteBytes(fileName, Span<uint8_t>((uint8_t*)fileContent.Pointer, fileContent.Length + 1));
    auto fileExists = SystemFileExists(fileName);
    auto result = SystemFileReadBytes(stackMemoryArena, fileName);
    SystemFileDelete(fileName);
    auto fileExistsAfterDelete = SystemFileExists(fileName);

    // Assert
    ASSERT_TRUE(fileExists);
    ASSERT_STREQ((char*)result.Pointer, fileContent.Pointer);
    ASSERT_FALSE(fileExistsAfterDelete);
}
