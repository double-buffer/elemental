using Elemental;

var counter = 0;

var applicationService = new ApplicationService();
applicationService.ConfigureLogHandler(DefaultLogHandlers.ConsoleLogHandler);

using var application = applicationService.CreateApplication("Hello World"u8);

applicationService.RunApplication(application, (status) =>
{
    if (counter > 10 || status != ApplicationStatus.Active)
    {
        return false;
    }

    Console.WriteLine($"Hello World {counter}!");
    counter++;
    return true;
});
