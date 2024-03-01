#include "Elemental.h"
#include "ApplicationTests.h"
#include "utest.h"

static bool TestInitCalled = false;
static int32_t TestCounter = 0;

void TestLogHandler(ElemLogMessageType messageType, ElemLogMessageCategory category, const char* function, const char* message) 
{
    if (strcmp(message, "Init OK") == 0)
    {
        TestInitCalled = true;
    }
}

bool TestRunHandler(ElemApplicationStatus status)
{
    if (TestCounter >= 10 || status != ElemApplicationStatus_Active)
    {
        return false;
    }

    TestCounter++;
    return true;
}

UTEST(Application, CreateApplication) 
{
    // Arrange
    ElemConfigureLogHandler(TestLogHandler);

    // Act
    ElemCreateApplication("TestApplication");

    // Assert
    ASSERT_TRUE(TestInitCalled);
}

UTEST(Application, RunApplication) 
{
    // Arrange
    InitLog();
    auto application = ElemCreateApplication("TestApplication");

    // Act
    ElemRunApplication(application, TestRunHandler); 

    // Assert
    ASSERT_EQ(10, TestCounter);
}
