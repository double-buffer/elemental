using Elemental;

var applicationService = new ApplicationService();
applicationService.ConfigureLogHandler(DefaultLogHandlers.ConsoleLogHandler);

applicationService.RunApplication<TestPayload>(new ()
{
    ApplicationName = "Hello Window"u8,
    InitHandler = InitSample,
    FreeHandler = FreeSample,
    Payload = new TestPayload
    {
        Test = 28
    }
});

void InitSample(ref TestPayload payload) 
{
    var systemInfo = applicationService.GetSystemInfo();

    Console.WriteLine($"Path: {System.Text.Encoding.UTF8.GetString(systemInfo.ApplicationPath)}");

    payload.Window = applicationService.CreateWindow();
    Console.WriteLine($"Test: {payload.Test}");
}

void FreeSample(ref TestPayload payload) 
{
    payload.Window.Dispose();
    Console.WriteLine("Exit Sample");
}

record struct TestPayload 
{
    public Window Window { get; set; }
    public int Test { get; set; }
}
