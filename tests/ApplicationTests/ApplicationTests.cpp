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

// TODO: Test Init and Free
// TODO: Test ExitApplication

// TODO: GetSystemInfo

/*
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
    auto application = ElemCreateApplication("TestApplication");

    // Assert
    ElemFreeApplication(application);
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
    ElemFreeApplication(application);
    ASSERT_EQ(10, TestCounter);
}*/
