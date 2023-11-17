namespace Elemental.Tests;

public class NativeApplicationTests
{
    [Fact]
    public void Constructor_WithLogger_WriteInitOKMessage()
    {
        // Arrange
        var initCalled = false;
        var logHandler = new LogMessageHandler((LogMessageType messageType, LogMessageCategory category, string function, string message) => 
        {
            if (messageType == LogMessageType.Debug && category == LogMessageCategory.NativeApplication && message == "Init OK")
            {
                initCalled = true;
            }
        });
        
        // Act
        using var applicationService = new NativeApplicationService(new() { LogMessageHandler = logHandler });

        // Assert
        Assert.True(initCalled);
    }

    [Fact]
    public void CreateApplication_WithName_ReturnsValidObject()
    {
        // Arrange
        using var applicationService = new NativeApplicationService();

        // Act
        var application = applicationService.CreateApplication("TestApp");

        // Assert
        Assert.NotEqual(nint.Zero, application.NativePointer);
    }

    [Fact]
    public void RunApplication_WithValidApplication_CallRunHandler()
    {
        // Arrange
        using var applicationService = new NativeApplicationService();
        var application = applicationService.CreateApplication("TestApp");

        var runHandlerCalled = false;
        var runHandler = new RunHandler((status) => 
        {
            runHandlerCalled = true;
            return false;
        });

        // Act
        applicationService.RunApplication(application, runHandler);

        // Assert
        Assert.True(runHandlerCalled);
    }
}
