#include "ToolsTests.h"

#define MAX_TEST_FILES 256

struct TestFileEntry
{
    char Path[256];
    ElemToolsDataSpan Data;
};

static TestFileEntry GlobalTestFiles[MAX_TEST_FILES];
static uint32_t GlobalTestFilesCount;

ElemToolsDataSpan LoadTestFilesHandler(const char* path)
{
    for (uint32_t i = 0; i < GlobalTestFilesCount; i++)
    {
        auto testFile = GlobalTestFiles[i];

        if (strstr(path, testFile.Path))
        {
            return testFile.Data;
        }
    }

    return {};
}

void ConfigureTestFileIO()
{
    ElemToolsConfigureFileIO(LoadTestFilesHandler);
}

void AddTestFile(const char* path, ElemToolsDataSpan data)
{
    TestFileEntry fileEntry
    {
        .Data = data
    };
    
    strncpy(fileEntry.Path, path, strlen(path));

    GlobalTestFiles[GlobalTestFilesCount++] = fileEntry;
}
