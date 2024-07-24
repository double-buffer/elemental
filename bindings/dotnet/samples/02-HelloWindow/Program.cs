using Elemental;

var applicationService = new ApplicationService();
applicationService.ConfigureLogHandler(DefaultLogHandlers.ConsoleLogHandler);

applicationService.RunApplication<TestPayload>(new ()
{
    ApplicationName = "Hello Window"u8,
    InitHandler = InitSample,
    FreeHandler = FreeSample,
    Payload = new TestPayload()
});

void InitSample(ref TestPayload payload) 
{
    payload.Window = applicationService.CreateWindow();
}

void FreeSample(ref TestPayload payload) 
{
    payload.Window.Dispose();
    Console.WriteLine("Exit Sample");
}

record struct TestPayload 
{
    public Window Window { get; set; }
}
