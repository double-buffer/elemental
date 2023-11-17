using Elemental;

var counter = 0;

using var applicationService = new NativeApplicationService();
using var application = applicationService.CreateApplication("Hello World");

applicationService.RunApplication(application, (status) =>
{
    if (counter > 10 || !status.IsActive)
    {
        return false;
    }

    Console.WriteLine($"Hello World {counter}!");
    counter++;
    return true;
});
