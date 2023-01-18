using Elemental;

var counter = 0;

var applicationService = new NativeApplicationService();
var application = applicationService.CreateApplication("Hello World");

applicationService.RunApplication(application, (status) =>
{
    if (counter > 10)
    {
        return false;
    }

    Console.WriteLine("Hello World {0}!");
    counter++;
    return true;
});