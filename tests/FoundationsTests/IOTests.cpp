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
    auto result = SystemFileReadBytes(stackMemoryArena, 0, 0, fileName);
    SystemFileDelete(fileName);
    auto fileExistsAfterDelete = SystemFileExists(fileName);

    // Assert
    ASSERT_TRUE_MSG(fileExists, "Temporary file should exist after writing it.");
    ASSERT_STREQ_MSG(fileContent.Pointer, (char*)result.Pointer, "File contents should match the bytes that were written.");
    ASSERT_FALSE_MSG(fileExistsAfterDelete, "Temporary file should not exist after deletion.");
}

UTEST(IOFunctions, SystemGetExecutableFolderPath)
{
    // Arrange
    auto stackMemoryArena = SystemGetStackMemoryArena();

    // Act
    auto result = SystemGetExecutableFolderPath(stackMemoryArena);

    // Assert
    ASSERT_GT_MSG((int32_t)result.Length, 0, "Executable folder path should not be empty.");
}
