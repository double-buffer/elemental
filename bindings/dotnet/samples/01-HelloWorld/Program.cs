using Elemental;

static void LogMessageHandler(LogMessageType messageType, LogMessageCategory category, ReadOnlySpan<byte> function, ReadOnlySpan<byte> message) 
{
    var mainForegroundColor = messageType switch
    {
        LogMessageType.Warning => ConsoleColor.Yellow,
        LogMessageType.Error => ConsoleColor.Red,
        _ => ConsoleColor.Gray
    };

    Console.Write("[");
    Console.ForegroundColor = ConsoleColor.Cyan;
    Console.Write($"{category}");
    Console.ForegroundColor = ConsoleColor.Gray;
    Console.Write("]");

    Console.ForegroundColor = ConsoleColor.Green;
    Console.Write($" {System.Text.UTF8Encoding.UTF8.GetString(function)}");

    Console.ForegroundColor = mainForegroundColor;
    Console.WriteLine($" {System.Text.UTF8Encoding.UTF8.GetString(message)}");
    Console.ForegroundColor = ConsoleColor.Gray;
}

var counter = 0;

var applicationService = new ApplicationService();
applicationService.ConfigureLogHandler(LogMessageHandler);

using var application = applicationService.CreateApplication("Hello World"u8);

applicationService.RunApplication(application, (status) =>
{
    if (counter > 10)// || !status.IsActive)
    {
        return false;
    }

    Console.WriteLine($"Hello World {counter}!");
    counter++;
    return true;
});
